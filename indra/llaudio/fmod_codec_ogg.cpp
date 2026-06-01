/**
 * @file fmod_codec_ogg.cpp
 * @brief AYAstorm FMOD codec plugin for Ogg Opus and Ogg Vorbis live streams.
 *
 * Bundled libfmod ships without an Opus codec. We extend FMOD via the
 * registerCodec() API: libogg handles the Ogg framing. libopus handles
 * Opus packets, and libvorbis handles Vorbis packets.
 *
 * Scope:
 *   - Channel mapping family 0 (native mono / stereo) via opus_decoder.
 *   - Channel mapping family 1 (Vorbis-compatible 3..8 ch surround) via
 *     opus_multistream_decoder. Mapping table is parsed from OpusHead.
 *   - Opus PCMFLOAT output at 48 kHz (FMOD resamples downstream as needed).
 *   - Vorbis PCMFLOAT output at the source stream sample rate.
 *   - Streamed playback: only setposition(0) is accepted (FMOD prebuffer);
 *     real seeking and getlength are not advertised.
 */

#include "fmod_codec_ogg.h"
#include "fmodstudio/fmod_errors.h"
#include "llerror.h"

#include <ogg/ogg.h>
#include <opus/opus.h>
#include <opus/opus_multistream.h>
#include <vorbis/codec.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

namespace
{
    constexpr int kOpusOutputRate      = 48000;
    constexpr int kOpusMaxFrameSamples = 5760;   // 120 ms @ 48 kHz, per channel
    constexpr int kOpusMaxChannels     = 8;      // family 1 supports up to 8 ch
    constexpr unsigned int kFeedChunkSize = 4096;
    constexpr unsigned int kOggCapturePatternSize = 4;
    constexpr unsigned int kProbeMaxBytes = 65536;
    constexpr unsigned int kVorbisHeaderMaxBytes = 1024 * 1024;

    enum class CodecKind
    {
        Unknown,
        Opus,
        Vorbis
    };

    struct OggCodecState
    {
        ogg_sync_state   oy{};
        ogg_stream_state os{};

        CodecKind        kind = CodecKind::Unknown;

        // Exactly one of these is non-null after a successful Opus open:
        // family 0 uses opus_decoder, family 1 uses opus_multistream_decoder.
        OpusDecoder*     decoder = nullptr;
        OpusMSDecoder*   ms_decoder = nullptr;

        vorbis_info      vorbis_info_state{};
        vorbis_comment   vorbis_comment_state{};
        vorbis_dsp_state vorbis_dsp_state_value{};
        vorbis_block     vorbis_block_value{};
        bool             vorbis_info_init_done = false;
        bool             vorbis_comment_init_done = false;
        bool             vorbis_dsp_init_done = false;
        bool             vorbis_block_init_done = false;

        int              channels = 0;
        int              sample_rate = 0;
        int              pre_skip_remaining = 0;
        bool             ogg_sync_init_done = false;
        bool             stream_init_done   = false;
        int              header_packets_seen = 0;
        bool             eof = false;
        bool             saw_ogg_eos = false;

        std::vector<float> pending;
        size_t           pending_pos = 0;

        FMOD_CODEC_WAVEFORMAT waveformat{};
    };

    inline std::uint16_t read_u16le(const unsigned char* p)
    {
        return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
    }

    bool feed_ogg_sync(FMOD_CODEC_STATE* codec, OggCodecState* state, unsigned int sizebytes, unsigned int* bytes_read_out = nullptr)
    {
        if (bytes_read_out) *bytes_read_out = 0;
        char* buf = ogg_sync_buffer(&state->oy, static_cast<long>(sizebytes));
        if (!buf)
        {
            LL_WARNS("FmodOgg") << "ogg_sync_buffer returned null (size=" << sizebytes << ")" << LL_ENDL;
            return false;
        }

        unsigned int bytes_read = 0;
        FMOD_RESULT r = FMOD_CODEC_FILE_READ(codec, buf, sizebytes, &bytes_read);
        if (bytes_read_out) *bytes_read_out = bytes_read;
        if (r == FMOD_ERR_FILE_EOF)
        {
            if (bytes_read == 0)
            {
                // Live HTTP Ogg/Opus can temporarily expose no compressed bytes
                // while the logical stream is still alive, especially during
                // very low bitrate VBR silence. Treat this as starvation unless
                // libogg has already seen a real EOS page.
                if (state->saw_ogg_eos)
                {
                    state->eof = true;
                }
                return false;
            }
        }
        else if (r != FMOD_OK)
        {
            state->eof = true;
            return false;
        }
        if (bytes_read == 0)
        {
            return false;
        }
        ogg_sync_wrote(&state->oy, static_cast<long>(bytes_read));
        return true;
    }

    void destroy_state(OggCodecState* state)
    {
        if (!state) return;
        if (state->decoder)
        {
            opus_decoder_destroy(state->decoder);
            state->decoder = nullptr;
        }
        if (state->ms_decoder)
        {
            opus_multistream_decoder_destroy(state->ms_decoder);
            state->ms_decoder = nullptr;
        }
        if (state->vorbis_block_init_done)
        {
            vorbis_block_clear(&state->vorbis_block_value);
            state->vorbis_block_init_done = false;
        }
        if (state->vorbis_dsp_init_done)
        {
            vorbis_dsp_clear(&state->vorbis_dsp_state_value);
            state->vorbis_dsp_init_done = false;
        }
        if (state->vorbis_comment_init_done)
        {
            vorbis_comment_clear(&state->vorbis_comment_state);
            state->vorbis_comment_init_done = false;
        }
        if (state->vorbis_info_init_done)
        {
            vorbis_info_clear(&state->vorbis_info_state);
            state->vorbis_info_init_done = false;
        }
        if (state->stream_init_done)
        {
            ogg_stream_clear(&state->os);
            state->stream_init_done = false;
        }
        if (state->ogg_sync_init_done)
        {
            ogg_sync_clear(&state->oy);
            state->ogg_sync_init_done = false;
        }
        delete state;
    }

    FMOD_RESULT F_CALL opusOpen(FMOD_CODEC_STATE* codec,
                                FMOD_MODE /*usermode*/,
                                FMOD_CREATESOUNDEXINFO* /*userexinfo*/)
    {
        if (!codec) return FMOD_ERR_FORMAT;

        OggCodecState* state = new OggCodecState();
        ogg_sync_init(&state->oy);
        state->ogg_sync_init_done = true;

        ogg_page page;
        ogg_packet first_packet;
        bool got_first_packet = false;
        unsigned int total_probed = kOggCapturePatternSize;

        // Keep this codec safe at priority 0. FMOD does not rewind
        // non-seekable HTTP streams after a failed codec probe, so a broad
        // libogg sync probe breaks MP3/Icecast streams before FMOD's built-in
        // MPEG codec can see the header. A valid Ogg stream starts with the
        // Ogg capture pattern; reject everything else after only four bytes.
        char* capture = ogg_sync_buffer(&state->oy, kOggCapturePatternSize);
        if (!capture)
        {
            destroy_state(state);
            return FMOD_ERR_FORMAT;
        }
        unsigned int bytes_read = 0;
        FMOD_RESULT read_result = FMOD_CODEC_FILE_READ(codec, capture, kOggCapturePatternSize, &bytes_read);
        if (read_result != FMOD_OK || bytes_read != kOggCapturePatternSize ||
            std::memcmp(capture, "OggS", kOggCapturePatternSize) != 0)
        {
            destroy_state(state);
            return FMOD_ERR_FORMAT;
        }
        ogg_sync_wrote(&state->oy, static_cast<long>(bytes_read));

        while (!got_first_packet)
        {
            int sync = ogg_sync_pageout(&state->oy, &page);
            if (sync == 1)
            {
                if (!ogg_page_bos(&page))
                {
                    destroy_state(state);
                    return FMOD_ERR_FORMAT;
                }
                if (ogg_stream_init(&state->os, ogg_page_serialno(&page)) != 0)
                {
                    destroy_state(state);
                    return FMOD_ERR_FORMAT;
                }
                state->stream_init_done = true;

                if (ogg_stream_pagein(&state->os, &page) != 0)
                {
                    destroy_state(state);
                    return FMOD_ERR_FORMAT;
                }
                if (ogg_stream_packetout(&state->os, &first_packet) != 1)
                {
                    destroy_state(state);
                    return FMOD_ERR_FORMAT;
                }
                got_first_packet = true;
                break;
            }
            // sync == 0 or -1: keep feeding more bytes.
            if (total_probed >= kProbeMaxBytes)
            {
                destroy_state(state);
                return FMOD_ERR_FORMAT;
            }
            if (!feed_ogg_sync(codec, state, kFeedChunkSize))
            {
                destroy_state(state);
                return FMOD_ERR_FORMAT;
            }
            total_probed += kFeedChunkSize;
        }

        const unsigned char* p = first_packet.packet;

        if (first_packet.bytes >= 19 && std::memcmp(p, "OpusHead", 8) == 0)
        {
            unsigned char version = p[8];
            if ((version & 0xF0) != 0)
            {
                destroy_state(state);
                return FMOD_ERR_FORMAT;
            }
            int channel_count = p[9];
            int pre_skip      = read_u16le(p + 10);
            // p[12..15] = original input sample rate (informational)
            std::int16_t output_gain_q8 = static_cast<std::int16_t>(read_u16le(p + 16));
            unsigned char channel_mapping_family = p[18];

            if (channel_count < 1 || channel_count > kOpusMaxChannels)
            {
                destroy_state(state);
                return FMOD_ERR_FORMAT;
            }

            int err = 0;
            if (channel_mapping_family == 0)
            {
                // Family 0: native mono / stereo, no mapping table.
                if (channel_count > 2)
                {
                    destroy_state(state);
                    return FMOD_ERR_FORMAT;
                }
                state->decoder = opus_decoder_create(kOpusOutputRate, channel_count, &err);
                if (!state->decoder || err != OPUS_OK)
                {
                    LL_WARNS("FmodOgg") << "opus_decoder_create failed err=" << err << LL_ENDL;
                    destroy_state(state);
                    return FMOD_ERR_FORMAT;
                }
                if (output_gain_q8 != 0)
                {
                    opus_decoder_ctl(state->decoder, OPUS_SET_GAIN(output_gain_q8));
                }
            }
            else if (channel_mapping_family == 1)
            {
                // Family 1: Vorbis-compatible mapping. OpusHead extends past the
                // 19-byte fixed header with stream_count, coupled_count, and a
                // per-output-channel mapping table.
                const int kMapHeaderSize = 2;   // stream_count + coupled_count
                const int needed = 19 + kMapHeaderSize + channel_count;
                if (first_packet.bytes < needed)
                {
                    destroy_state(state);
                    return FMOD_ERR_FORMAT;
                }
                int stream_count  = p[19];
                int coupled_count = p[20];
                const unsigned char* mapping = p + 21;
                // libopus validates stream_count / coupled_count / mapping itself;
                // any inconsistency surfaces as a non-OK err below.
                state->ms_decoder = opus_multistream_decoder_create(
                    kOpusOutputRate,
                    channel_count,
                    stream_count,
                    coupled_count,
                    mapping,
                    &err);
                if (!state->ms_decoder || err != OPUS_OK)
                {
                    LL_WARNS("FmodOgg") << "opus_multistream_decoder_create failed err=" << err
                                          << " ch=" << channel_count
                                          << " streams=" << stream_count
                                          << " coupled=" << coupled_count << LL_ENDL;
                    destroy_state(state);
                    return FMOD_ERR_FORMAT;
                }
                if (output_gain_q8 != 0)
                {
                    opus_multistream_decoder_ctl(state->ms_decoder, OPUS_SET_GAIN(output_gain_q8));
                }
                LL_INFOS("FmodOgg") << "Opus multistream: ch=" << channel_count
                                      << " streams=" << stream_count
                                      << " coupled=" << coupled_count << LL_ENDL;
            }
            else
            {
                // Family 2 / 3 (ambisonic) deferred — would land alongside HRTF/SOFA
                // work, not r9 5.1 source receiving.
                destroy_state(state);
                return FMOD_ERR_FORMAT;
            }

            state->kind                = CodecKind::Opus;
            state->channels            = channel_count;
            state->sample_rate         = kOpusOutputRate;
            state->pre_skip_remaining  = pre_skip;
            state->header_packets_seen = 1;   // OpusHead consumed; OpusTags drained lazily

            // Match the FMOD raw-codec example shape: only fill the fields that
            // are unambiguous for a streamed source. Letting pcmblocksize / mode /
            // channelmask / channelorder / lengthbytes stay at 0 lets FMOD pick
            // safe defaults rather than rejecting the descriptor for a mismatch.
            state->waveformat.name         = "Ogg Opus";
            state->waveformat.format       = FMOD_SOUND_FORMAT_PCMFLOAT;
            state->waveformat.channels     = channel_count;
            state->waveformat.frequency    = kOpusOutputRate;
            state->waveformat.lengthpcm    = 0xFFFFFFFFu;   // unknown / streaming

            codec->waveformat   = &state->waveformat;
            codec->plugindata   = state;
            codec->numsubsounds = 0;

            LL_INFOS("FmodOgg") << "Opus stream opened: "
                                  << channel_count << "ch 48000Hz PCMFLOAT pre_skip=" << pre_skip << LL_ENDL;
            return FMOD_OK;
        }

        if (first_packet.bytes >= 7 && first_packet.packet[0] == 0x01 &&
            std::memcmp(first_packet.packet + 1, "vorbis", 6) == 0)
        {
            state->kind = CodecKind::Vorbis;
            vorbis_info_init(&state->vorbis_info_state);
            state->vorbis_info_init_done = true;
            vorbis_comment_init(&state->vorbis_comment_state);
            state->vorbis_comment_init_done = true;

            if (vorbis_synthesis_headerin(&state->vorbis_info_state,
                                           &state->vorbis_comment_state,
                                           &first_packet) != 0)
            {
                LL_WARNS("FmodOgg") << "vorbis first header rejected" << LL_ENDL;
                destroy_state(state);
                return FMOD_ERR_FORMAT;
            }
            state->header_packets_seen = 1;

            unsigned int header_bytes = total_probed;
            while (state->header_packets_seen < 3)
            {
                ogg_packet packet;
                int packet_rc = ogg_stream_packetout(&state->os, &packet);
                if (packet_rc == 1)
                {
                    if (vorbis_synthesis_headerin(&state->vorbis_info_state,
                                                   &state->vorbis_comment_state,
                                                   &packet) != 0)
                    {
                        LL_WARNS("FmodOgg") << "vorbis header " << state->header_packets_seen
                                              << " rejected" << LL_ENDL;
                        destroy_state(state);
                        return FMOD_ERR_FORMAT;
                    }
                    state->header_packets_seen++;
                    continue;
                }

                ogg_page next_page;
                int sync = ogg_sync_pageout(&state->oy, &next_page);
                if (sync == 1)
                {
                    if (ogg_stream_pagein(&state->os, &next_page) != 0)
                    {
                        destroy_state(state);
                        return FMOD_ERR_FORMAT;
                    }
                    continue;
                }
                if (sync < 0)
                {
                    continue;
                }
                if (header_bytes >= kVorbisHeaderMaxBytes)
                {
                    LL_WARNS("FmodOgg") << "vorbis headers exceeded "
                                          << kVorbisHeaderMaxBytes << " bytes" << LL_ENDL;
                    destroy_state(state);
                    return FMOD_ERR_FORMAT;
                }
                unsigned int fed = 0;
                if (!feed_ogg_sync(codec, state, kFeedChunkSize, &fed))
                {
                    destroy_state(state);
                    return FMOD_ERR_FORMAT;
                }
                header_bytes += fed;
            }

            if (vorbis_synthesis_init(&state->vorbis_dsp_state_value,
                                       &state->vorbis_info_state) != 0)
            {
                LL_WARNS("FmodOgg") << "vorbis_synthesis_init failed" << LL_ENDL;
                destroy_state(state);
                return FMOD_ERR_FORMAT;
            }
            state->vorbis_dsp_init_done = true;
            if (vorbis_block_init(&state->vorbis_dsp_state_value,
                                  &state->vorbis_block_value) != 0)
            {
                LL_WARNS("FmodOgg") << "vorbis_block_init failed" << LL_ENDL;
                destroy_state(state);
                return FMOD_ERR_FORMAT;
            }
            state->vorbis_block_init_done = true;

            state->channels = state->vorbis_info_state.channels;
            state->sample_rate = static_cast<int>(state->vorbis_info_state.rate);
            if (state->channels < 1 || state->sample_rate <= 0)
            {
                LL_WARNS("FmodOgg") << "invalid Vorbis format: ch=" << state->channels
                                      << " rate=" << state->sample_rate << LL_ENDL;
                destroy_state(state);
                return FMOD_ERR_FORMAT;
            }

            state->waveformat.name         = "Ogg Vorbis";
            state->waveformat.format       = FMOD_SOUND_FORMAT_PCMFLOAT;
            state->waveformat.channels     = state->channels;
            state->waveformat.frequency    = state->sample_rate;
            state->waveformat.lengthpcm    = 0xFFFFFFFFu;

            codec->waveformat   = &state->waveformat;
            codec->plugindata   = state;
            codec->numsubsounds = 0;

            LL_INFOS("FmodOgg") << "Vorbis stream opened: "
                                  << state->channels << "ch "
                                  << state->sample_rate << "Hz PCMFLOAT" << LL_ENDL;
            return FMOD_OK;
        }

        destroy_state(state);
        return FMOD_ERR_FORMAT;
    }

    FMOD_RESULT F_CALL opusClose(FMOD_CODEC_STATE* codec)
    {
        if (codec && codec->plugindata)
        {
            destroy_state(static_cast<OggCodecState*>(codec->plugindata));
            codec->plugindata = nullptr;
        }
        return FMOD_OK;
    }

    // Pull one packet from the Ogg stream and turn it into PCM in state->pending.
    // Returns true if a packet was processed (audio or header), false if no
    // packet was available without more input from the file.
    bool decode_next_packet(OggCodecState* state)
    {
        ogg_packet packet;
        int rc = ogg_stream_packetout(&state->os, &packet);
        if (rc != 1) return false;

        if (state->kind == CodecKind::Opus)
        {
            // Drop OpusTags transparently; everything afterwards is audio.
            if (state->header_packets_seen < 2)
            {
                state->header_packets_seen++;
                return true;
            }

            std::vector<float> tmp(static_cast<size_t>(kOpusMaxFrameSamples) * static_cast<size_t>(state->channels));
            int frames = 0;
            if (state->ms_decoder)
            {
                frames = opus_multistream_decode_float(state->ms_decoder,
                                                       packet.packet,
                                                       static_cast<opus_int32>(packet.bytes),
                                                       tmp.data(),
                                                       kOpusMaxFrameSamples,
                                                       0);
            }
            else
            {
                frames = opus_decode_float(state->decoder,
                                           packet.packet,
                                           static_cast<opus_int32>(packet.bytes),
                                           tmp.data(),
                                           kOpusMaxFrameSamples,
                                           0);
            }
            if (frames <= 0)
            {
                LL_WARNS("FmodOgg") << "opus_decode_float -> " << frames
                                      << " (packet bytes=" << packet.bytes << ")" << LL_ENDL;
                return true;
            }

            int frames_offset = 0;
            int frames_keep   = frames;
            if (state->pre_skip_remaining > 0)
            {
                int skip = std::min(state->pre_skip_remaining, frames);
                frames_offset             += skip;
                frames_keep               -= skip;
                state->pre_skip_remaining -= skip;
            }
            if (frames_keep > 0)
            {
                const size_t off = static_cast<size_t>(frames_offset) * static_cast<size_t>(state->channels);
                const size_t n   = static_cast<size_t>(frames_keep)   * static_cast<size_t>(state->channels);
                state->pending.assign(tmp.begin() + off, tmp.begin() + off + n);
                state->pending_pos = 0;
            }
            return true;
        }

        if (state->kind == CodecKind::Vorbis)
        {
            if (vorbis_synthesis(&state->vorbis_block_value, &packet) == 0)
            {
                vorbis_synthesis_blockin(&state->vorbis_dsp_state_value,
                                         &state->vorbis_block_value);
            }
            else
            {
                LL_WARNS("FmodOgg") << "vorbis_synthesis rejected packet bytes="
                                      << packet.bytes << LL_ENDL;
                return true;
            }

            float** pcm = nullptr;
            int frames = 0;
            while ((frames = vorbis_synthesis_pcmout(&state->vorbis_dsp_state_value, &pcm)) > 0)
            {
                const size_t base = state->pending.size();
                state->pending.resize(base + static_cast<size_t>(frames) * static_cast<size_t>(state->channels));
                for (int i = 0; i < frames; ++i)
                {
                    for (int ch = 0; ch < state->channels; ++ch)
                    {
                        state->pending[base + static_cast<size_t>(i) * static_cast<size_t>(state->channels) + static_cast<size_t>(ch)] = pcm[ch][i];
                    }
                }
                vorbis_synthesis_read(&state->vorbis_dsp_state_value, frames);
            }
            state->pending_pos = 0;
            return true;
        }

        return true;
    }

    FMOD_RESULT F_CALL opusRead(FMOD_CODEC_STATE* codec,
                                void* buffer,
                                unsigned int samples_in,
                                unsigned int* samples_out)
    {
        if (!codec || !codec->plugindata || !buffer)
        {
            if (samples_out) *samples_out = 0;
            return FMOD_ERR_FORMAT;
        }
        OggCodecState* state = static_cast<OggCodecState*>(codec->plugindata);
        float* out = static_cast<float*>(buffer);

        unsigned int frames_written = 0;

        while (frames_written < samples_in)
        {
            // 1) Drain any leftover decoded PCM.
            if (state->pending_pos < state->pending.size())
            {
                const size_t avail_floats = state->pending.size() - state->pending_pos;
                const size_t avail_frames = avail_floats / static_cast<size_t>(state->channels);
                const size_t want_frames  = static_cast<size_t>(samples_in - frames_written);
                const size_t take_frames  = std::min(avail_frames, want_frames);
                const size_t take_floats  = take_frames * static_cast<size_t>(state->channels);

                std::memcpy(out + static_cast<size_t>(frames_written) * static_cast<size_t>(state->channels),
                            state->pending.data() + state->pending_pos,
                            take_floats * sizeof(float));
                state->pending_pos += take_floats;
                frames_written     += static_cast<unsigned int>(take_frames);

                if (state->pending_pos >= state->pending.size())
                {
                    state->pending.clear();
                    state->pending_pos = 0;
                }
                continue;
            }

            // 2) Try to decode the next packet from the stream.
            if (decode_next_packet(state)) continue;

            // 3) No packet ready — pull more pages.
            ogg_page page;
            int sync = ogg_sync_pageout(&state->oy, &page);
            if (sync == 1)
            {
                if (ogg_page_eos(&page))
                {
                    state->saw_ogg_eos = true;
                }
                ogg_stream_pagein(&state->os, &page);
                continue;
            }
            if (sync < 0)
            {
                // Hole/desync — let libogg resync from existing buffer.
                continue;
            }

            // 4) Need more bytes from the file.
            if (state->eof) break;
            if (!feed_ogg_sync(codec, state, kFeedChunkSize)) break;
        }

        if (samples_out) *samples_out = frames_written;
        if (frames_written > 0)
        {
            return FMOD_OK;
        }
        return state->eof ? FMOD_ERR_FILE_EOF : FMOD_ERR_NOTREADY;
    }

    FMOD_RESULT F_CALL opusSetPosition(FMOD_CODEC_STATE* /*codec*/,
                                       int /*subsound*/,
                                       unsigned int position,
                                       FMOD_TIMEUNIT /*postype*/)
    {
        // FMOD's prebuffer flow may issue setposition(0) before the first
        // read; treat that as a no-op so streamed playback isn't rejected.
        // Real seeking is deferred to a later phase (offline files).
        if (position == 0) return FMOD_OK;
        return FMOD_ERR_FILE_COULDNOTSEEK;
    }

    FMOD_CODEC_DESCRIPTION sOggCodec =
    {
        FMOD_CODEC_PLUGIN_VERSION,
        "AYAstorm Ogg Opus/Vorbis codec",
        0x00000003,                  // 0.0.0.3 (adds Vorbis to avoid unsafe fallback)
        1,                           // defaultasstream: streamed by default
        FMOD_TIMEUNIT_PCM,
        &opusOpen,
        &opusClose,
        &opusRead,
        nullptr,                     // getlength: unknown for streams
        &opusSetPosition,
        nullptr,                     // getposition
        nullptr,                     // soundcreate
        nullptr                      // getwaveformat
    };
}

FMOD_CODEC_DESCRIPTION* F_CALL FMODGetCodecDescriptionOgg()
{
    return &sOggCodec;
}
