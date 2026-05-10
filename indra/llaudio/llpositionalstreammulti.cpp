/**
 * @file llpositionalstreammulti.cpp
 * @brief Distributed-description stereo: 1 stream → N speakers (AYAstorm r8).
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Phoenix Firestorm Project, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llpositionalstreammulti.h"

#include "llaudioengine.h"
#include "llaudioengine_fmodstudio.h"
#include "lllitehrtfdsp.h"
#include "llstream3durlresolve.h"
#include "llstring.h"
#include "lltimer.h"

#include "fmodstudio/fmod.hpp"
#include "fmodstudio/fmod_errors.h"

#include <algorithm>
#include <chrono>
#include <cstring>

#if LL_LINUX
#  include <pthread.h>
#endif

namespace
{
    bool checkFmod(FMOD_RESULT result, const char* what)
    {
        if (result == FMOD_OK)
        {
            return false;
        }
        LL_WARNS("Stream3D") << what << " error: " << FMOD_ErrorString(result) << LL_ENDL;
        return true;
    }

    size_t nextPow2(size_t v)
    {
        size_t p = 1;
        while (p < v) p <<= 1;
        return p;
    }
}

// ---------------------------------------------------------------------------
// LLMultiTailRing
// ---------------------------------------------------------------------------

LLMultiTailRing::LLMultiTailRing()
:   mCapacityFrames(0),
    mNumTracks(0),
    mNumReaders(0),
    mWriteFrame(0)
{
}

LLMultiTailRing::~LLMultiTailRing() = default;

void LLMultiTailRing::reset(size_t capacity_frames, size_t n_tracks, size_t n_readers)
{
    mCapacityFrames = capacity_frames;
    mNumTracks = n_tracks;
    mNumReaders = n_readers;
    // (capacity + 1) frames so write_frame == read_frame can mean empty and
    // (write_frame + 1) % size == read_frame can mean full (one slot reserved).
    mBuf.assign((capacity_frames + 1) * n_tracks, 0.f);
    mWriteFrame.store(0, std::memory_order_relaxed);
    mReadFrames.reset(new std::atomic<size_t>[n_readers]);
    for (size_t i = 0; i < n_readers; ++i)
    {
        mReadFrames[i].store(0, std::memory_order_relaxed);
    }
}

void LLMultiTailRing::clear()
{
    mWriteFrame.store(0, std::memory_order_relaxed);
    for (size_t i = 0; i < mNumReaders; ++i)
    {
        mReadFrames[i].store(0, std::memory_order_relaxed);
    }
}

size_t LLMultiTailRing::writeAvailable() const
{
    if (mCapacityFrames == 0 || mNumReaders == 0) return 0;
    const size_t total = mCapacityFrames + 1;
    const size_t w = mWriteFrame.load(std::memory_order_acquire);
    // Writer must stay one slot behind the slowest reader. Compute lag for
    // each reader and take the max; free = capacity - max_lag.
    size_t max_lag = 0;
    for (size_t i = 0; i < mNumReaders; ++i)
    {
        const size_t r = mReadFrames[i].load(std::memory_order_acquire);
        const size_t lag = (w >= r) ? (w - r) : (total - r + w);
        if (lag > max_lag) max_lag = lag;
    }
    return mCapacityFrames - max_lag;
}

size_t LLMultiTailRing::readAvailable(size_t reader_idx) const
{
    if (reader_idx >= mNumReaders || mCapacityFrames == 0) return 0;
    const size_t total = mCapacityFrames + 1;
    const size_t w = mWriteFrame.load(std::memory_order_acquire);
    const size_t r = mReadFrames[reader_idx].load(std::memory_order_acquire);
    return (w >= r) ? (w - r) : (total - r + w);
}

size_t LLMultiTailRing::writeFrames(const F32* src, size_t n_frames)
{
    if (mCapacityFrames == 0 || mNumTracks == 0 || mNumReaders == 0) return 0;
    const size_t free_frames = writeAvailable();
    const size_t to_write = std::min(n_frames, free_frames);
    if (to_write == 0) return 0;

    const size_t total = mCapacityFrames + 1;
    const size_t w = mWriteFrame.load(std::memory_order_relaxed);
    const size_t first_chunk = std::min(to_write, total - w);
    std::memcpy(mBuf.data() + w * mNumTracks,
                src,
                first_chunk * mNumTracks * sizeof(F32));
    if (to_write > first_chunk)
    {
        std::memcpy(mBuf.data(),
                    src + first_chunk * mNumTracks,
                    (to_write - first_chunk) * mNumTracks * sizeof(F32));
    }
    mWriteFrame.store((w + to_write) % total, std::memory_order_release);
    return to_write;
}

size_t LLMultiTailRing::readFramesTrack(size_t reader_idx, F32* dst,
                                        size_t n_frames, size_t track_idx)
{
    if (reader_idx >= mNumReaders || mCapacityFrames == 0 || mNumTracks == 0) return 0;
    if (track_idx >= mNumTracks) track_idx = mNumTracks - 1;
    const size_t avail = readAvailable(reader_idx);
    const size_t to_read = std::min(n_frames, avail);
    if (to_read == 0) return 0;

    const size_t total = mCapacityFrames + 1;
    const size_t r = mReadFrames[reader_idx].load(std::memory_order_relaxed);
    for (size_t i = 0; i < to_read; ++i)
    {
        const size_t f = (r + i) % total;
        dst[i] = mBuf[f * mNumTracks + track_idx];
    }

    mReadFrames[reader_idx].store((r + to_read) % total, std::memory_order_release);
    return to_read;
}

size_t LLMultiTailRing::readFramesMonoSum(size_t reader_idx, F32* dst, size_t n_frames)
{
    if (reader_idx >= mNumReaders || mCapacityFrames == 0 || mNumTracks == 0) return 0;
    const size_t avail = readAvailable(reader_idx);
    const size_t to_read = std::min(n_frames, avail);
    if (to_read == 0) return 0;

    const size_t total = mCapacityFrames + 1;
    const size_t r = mReadFrames[reader_idx].load(std::memory_order_relaxed);
    // 1-track ring (degenerate path): collapse to that single track. All
    // ≥2-track rings sum tracks 0 and 1; ≥3-track rings ignore the rest
    // because callers on 6ch source go through readFramesTrack /
    // mix6chToMono instead.
    const size_t track_l = 0;
    const size_t track_r = (mNumTracks >= 2) ? 1 : 0;

    for (size_t i = 0; i < to_read; ++i)
    {
        const size_t f = (r + i) % total;
        const F32 sl = mBuf[f * mNumTracks + track_l];
        const F32 sr = mBuf[f * mNumTracks + track_r];
        // -6 dB sum-to-mono (spec §4.6). 1ch source is duplicated into
        // both tracks at write time so this collapses to the source sample.
        dst[i] = (sl + sr) * 0.5f;
    }

    mReadFrames[reader_idx].store((r + to_read) % total, std::memory_order_release);
    return to_read;
}

size_t LLMultiTailRing::skipFrames(size_t reader_idx, size_t n_frames)
{
    if (reader_idx >= mNumReaders || mCapacityFrames == 0) return 0;
    const size_t avail = readAvailable(reader_idx);
    const size_t to_skip = std::min(n_frames, avail);
    if (to_skip == 0) return 0;
    const size_t total = mCapacityFrames + 1;
    const size_t r = mReadFrames[reader_idx].load(std::memory_order_relaxed);
    mReadFrames[reader_idx].store((r + to_skip) % total, std::memory_order_release);
    return to_skip;
}

size_t LLMultiTailRing::readFramesRaw(size_t reader_idx, F32* dst, size_t n_frames)
{
    if (reader_idx >= mNumReaders || mCapacityFrames == 0 || mNumTracks == 0) return 0;
    const size_t avail = readAvailable(reader_idx);
    const size_t to_read = std::min(n_frames, avail);
    if (to_read == 0) return 0;

    const size_t total = mCapacityFrames + 1;
    const size_t r = mReadFrames[reader_idx].load(std::memory_order_relaxed);

    // Mirror of writeFrames: split-memcpy across the ring's wrap point.
    const size_t first_chunk = std::min(to_read, total - r);
    std::memcpy(dst,
                mBuf.data() + r * mNumTracks,
                first_chunk * mNumTracks * sizeof(F32));
    if (to_read > first_chunk)
    {
        std::memcpy(dst + first_chunk * mNumTracks,
                    mBuf.data(),
                    (to_read - first_chunk) * mNumTracks * sizeof(F32));
    }
    mReadFrames[reader_idx].store((r + to_read) % total, std::memory_order_release);
    return to_read;
}

// ---------------------------------------------------------------------------
// LLPositionalStreamMulti
// ---------------------------------------------------------------------------

LLPositionalStreamMulti::LLPositionalStreamMulti()
:   mSourceSound(nullptr),
    mSampleRate(0),
    mSourceChannels(0),
    mSourceBytesPerSample(0),
    mSourceIsFloat(false),
    mSourceType(FMOD_SOUND_TYPE_UNKNOWN),
    mVolume(1.f),
    mUpmixLfeCutoffHz(80.f),
    mUpmixCenterBleed(1.f),
    mUpmixRearDelayBaseMs(16.f),
    mLfeGain(1.f),
    mState(State::Idle),
    mDecodeStop(false)
{
}

void LLPositionalStreamMulti::setUpmixTuning(F32 lfe_cutoff_hz, F32 center_bleed,
                                             F32 rear_delay_base_ms)
{
    // r12 P6: clamp to the same windows the settings.xml comments
    // advertise so a hostile / mistyped value can't push the helper
    // outside the validated coefficient range. Relaxed atomics suffice —
    // the FMOD mixer thread re-reads on every callback, so a torn
    // visibility window is at most one chunk (≤ 1024 frames ≈ 23 ms at
    // 44.1 kHz) of stale value.
    const F32 fc    = std::clamp(lfe_cutoff_hz,     20.f, 200.f);
    const F32 bleed = std::clamp(center_bleed,       0.f,   1.f);
    const F32 base  = std::clamp(rear_delay_base_ms, 0.f,  32.f);
    mUpmixLfeCutoffHz.store(fc,    std::memory_order_relaxed);
    mUpmixCenterBleed.store(bleed, std::memory_order_relaxed);
    mUpmixRearDelayBaseMs.store(base, std::memory_order_relaxed);
}

LLPositionalStreamMulti::~LLPositionalStreamMulti()
{
    if (gAudiop)
    {
        stop();
    }
    else
    {
        // r7 M3 shutdown invariant carried into r8: the decode thread holds
        // mSourceSound; if FMOD is already gone we must still join before
        // ~std::thread() calls std::terminate(). releaseAll() is skipped
        // because the FMOD pointers are already invalid.
        stopDecodeThread();
    }
}

FMOD::System* LLPositionalStreamMulti::getFmodSystem() const
{
    if (!gAudiop) return nullptr;
    LLAudioEngine_FMODSTUDIO* engine = dynamic_cast<LLAudioEngine_FMODSTUDIO*>(gAudiop);
    return engine ? engine->getSystem() : nullptr;
}

bool LLPositionalStreamMulti::isPlaying() const
{
    for (const auto& sr : mSpeakerRuntime)
    {
        if (sr.channel) return true;
    }
    return false;
}

bool LLPositionalStreamMulti::start(const std::string& url,
                                    const std::vector<SpeakerConfig>& speakers)
{
    stop();

    if (speakers.empty())
    {
        LL_WARNS("Stream3D") << "Refusing to start multi stream with zero speakers" << LL_ENDL;
        return false;
    }

    std::string clean_url(url);
    LLStringUtil::trim(clean_url);
    if (clean_url.empty())
    {
        LL_WARNS("Stream3D") << "Refusing to start multi stream with empty URL" << LL_ENDL;
        return false;
    }

    FMOD::System* system = getFmodSystem();
    if (!system)
    {
        LL_WARNS("Stream3D") << "FMOD Studio system unavailable" << LL_ENDL;
        return false;
    }

    mUrl = clean_url;
    mSpeakers = speakers;
    mReadFailStreak = 0;
    mLastReadFailLogTime = 0.0;
    mZeroFillStreakStart = 0.0;
    mFailReason.store(FailReason::Ok, std::memory_order_relaxed);
    mFailDetail.clear();

    const FMOD_MODE source_mode = FMOD_2D
                                | FMOD_NONBLOCKING
                                | FMOD_IGNORETAGS;

    // r11 P10: viewer-side URL pre-resolve. FMOD netstream does not follow
    // HTTPS→HTTP cross-protocol redirects (typical of Cloudflare/CDN
    // fronted Shoutcast/Icecast). We probe via libcurl HEAD (with ranged
    // GET fallback) and hand FMOD the post-redirect URL. mUrl stays as
    // the original input so reconnect/log surfaces still show what the
    // tag asked for.
    //
    // r13: skip the probe for plain http:// URLs. The probe was designed
    // for the HTTPS→HTTP redirect case; an http:// source needs no scheme
    // promotion, and FMOD itself follows http→http redirects. Probing
    // anyway costs up to 3 s on the main thread per stream open (long
    // enough to trip the OS unresponsive dialog when the upstream is
    // slow). Easy win to gate it on the scheme.
    std::string createstream_url = clean_url;
    const bool is_http = (clean_url.compare(0, 7, "http://") == 0);
    if (mUrlPreResolveEnabled && !is_http)
    {
        std::string resolved;
        if (LLStream3DUrlResolve::resolveStreamUrl(clean_url, resolved))
        {
            LL_INFOS("Stream3DUrlResolve") << "pre-resolved: " << clean_url
                                             << " -> " << resolved << LL_ENDL;
            createstream_url = resolved;
        }
    }

    if (checkFmod(system->createStream(createstream_url.c_str(), source_mode, nullptr, &mSourceSound),
                  "createStream(source)"))
    {
        mSourceSound = nullptr;
        mUrl.clear();
        mSpeakers.clear();
        return false;
    }

    mState = State::Opening;
    LL_INFOS("Stream3D") << "Opening multi source '" << clean_url
                          << "' with " << mSpeakers.size() << " speaker(s)" << LL_ENDL;
    return true;
}

void LLPositionalStreamMulti::stop()
{
    // r7 M3 invariant: join decode thread before releasing FMOD resources it
    // is reading from.
    stopDecodeThread();
    releaseAll();
    mUrl.clear();
    mSpeakers.clear();
    mState = State::Idle;
}

void LLPositionalStreamMulti::setFailed(FailReason reason, std::string detail)
{
    mFailDetail = std::move(detail);
    // Order matters: detail + reason must be observable before the Failed
    // state publishes. mState's release pairs with isFailed()'s acquire.
    mFailReason.store(reason, std::memory_order_release);
    mState.store(State::Failed, std::memory_order_release);
}

void LLPositionalStreamMulti::releaseAll()
{
    llassert(!mDecodeThread.joinable());

    // r11 P5: peel the LiteHrtfDsp off the channel chain BEFORE stopping,
    // mirroring the wind-DSP cleanup pattern in LLAudioEngine_FMODSTUDIO.
    // DSP::release() does detach internally, but removing while the
    // channel is still valid is the documented order and avoids leaving a
    // recycled-channel slot wired to a soon-to-be-released DSP.
    for (auto& sr : mSpeakerRuntime)
    {
        if (sr.channel && sr.hrtf_dsp)
        {
            if (FMOD::DSP* dsp = sr.hrtf_dsp->getDsp())
            {
                checkFmod(sr.channel->removeDSP(dsp),
                          "Channel::removeDSP(LiteHrtfDsp)");
            }
        }
    }
    // r13: same teardown order for the per-speaker LOWPASS_SIMPLE DSP. We
    // own this one outright (createDSPByType in makeChannelForBinding) so
    // we both removeDSP and release here, then null the pointer.
    for (auto& sr : mSpeakerRuntime)
    {
        if (sr.lowpass_dsp)
        {
            if (sr.channel)
            {
                checkFmod(sr.channel->removeDSP(sr.lowpass_dsp),
                          "Channel::removeDSP(LowpassSimple)");
            }
            checkFmod(sr.lowpass_dsp->release(), "DSP::release(LowpassSimple)");
            sr.lowpass_dsp = nullptr;
        }
    }
    // Channels must be stopped before their backing OPENUSER sounds are
    // released; FMOD will warn (and may briefly stutter) otherwise.
    for (auto& sr : mSpeakerRuntime)
    {
        if (sr.channel)
        {
            checkFmod(sr.channel->stop(), "Channel::stop");
            sr.channel = nullptr;
        }
    }
    for (auto& sr : mSpeakerRuntime)
    {
        if (sr.user_sound)
        {
            checkFmod(sr.user_sound->release(), "Sound::release(user)");
            sr.user_sound = nullptr;
        }
    }
    // SpeakerCallback structs are referenced by the FMOD pcmreadcallback,
    // which won't fire again after the sound is released — safe to drop.
    mSpeakerRuntime.clear();

    if (mSourceSound)
    {
        checkFmod(mSourceSound->release(), "Sound::release(source)");
        mSourceSound = nullptr;
    }
    mRing.clear();
    mSampleRate = 0;
    mSourceChannels = 0;
    mSourceBytesPerSample = 0;
    mSourceIsFloat = false;
    mSourceType = FMOD_SOUND_TYPE_UNKNOWN;
    mDownmix = LLMultichannelDownmix();
    // Don't reset mFailReason / mFailDetail here: releaseAll() runs as part
    // of the transition into Failed and the manager reads these immediately
    // afterwards. They're cleared on the next start() instead.
}

const char* LLPositionalStreamMulti::sourceFormatName() const
{
    // r10 P5: short codec label for the routing diagnostic log. Only the
    // codecs we actually meet on the AYAstorm path are spelled out — anything
    // else falls back to "Unknown" so the diagnostic line still parses.
    switch (mSourceType)
    {
    case FMOD_SOUND_TYPE_OGGVORBIS:
    case FMOD_SOUND_TYPE_VORBIS:    return "Vorbis";
    case FMOD_SOUND_TYPE_OPUS:      return "Opus";
    case FMOD_SOUND_TYPE_FLAC:      return "FLAC";
    case FMOD_SOUND_TYPE_MPEG:      return "MPEG";
    case FMOD_SOUND_TYPE_WAV:       return "WAV";
    case FMOD_SOUND_TYPE_AIFF:      return "AIFF";
    default:                        return "Unknown";
    }
}

void LLPositionalStreamMulti::setSpeakerPosition(size_t idx, const LLVector3& pos)
{
    if (idx >= mSpeakers.size()) return;
    mSpeakers[idx].position = pos;
    if (idx < mSpeakerRuntime.size() && mSpeakerRuntime[idx].channel)
    {
        applyChannelAttributes(mSpeakerRuntime[idx].channel, pos, mSpeakers[idx].range);
    }
    // r11 P4: keep the lite-HRTF source pos in sync with FMOD's set3DAttributes
    // so the DSP and the built-in panner agree on geometry the instant the
    // mgr moves a speaker (rather than waiting for the next update() tick).
    if (idx < mSpeakerRuntime.size() && mSpeakerRuntime[idx].hrtf_dsp)
    {
        mSpeakerRuntime[idx].hrtf_dsp->setSourcePos(pos);
    }
}

void LLPositionalStreamMulti::setVolume(F32 volume)
{
    mVolume = volume;
    for (size_t i = 0; i < mSpeakerRuntime.size() && i < mSpeakers.size(); ++i)
    {
        if (mSpeakerRuntime[i].channel)
        {
            checkFmod(mSpeakerRuntime[i].channel->setVolume(mVolume * mSpeakers[i].volume),
                      "Channel::setVolume");
        }
    }
}

void LLPositionalStreamMulti::setSpeakerVolume(size_t idx, F32 volume)
{
    if (idx >= mSpeakers.size()) return;
    if (mSpeakers[idx].volume == volume) return;
    mSpeakers[idx].volume = volume;
    if (idx < mSpeakerRuntime.size() && mSpeakerRuntime[idx].channel)
    {
        checkFmod(mSpeakerRuntime[idx].channel->setVolume(mVolume * volume),
                  "Channel::setVolume(speaker)");
    }
}

void LLPositionalStreamMulti::forEachActiveSpeaker(const SpeakerVisitor& fn) const
{
    if (!fn) return;
    for (size_t i = 0; i < mSpeakerRuntime.size() && i < mSpeakers.size(); ++i)
    {
        FMOD::Channel* ch = mSpeakerRuntime[i].channel;
        if (!ch) continue;
        fn(ch, mSpeakerRuntime[i].lowpass_dsp, mSpeakers[i].position);
    }
}

void LLPositionalStreamMulti::applyChannelAttributes(FMOD::Channel* channel,
                                                     const LLVector3& pos,
                                                     F32 range)
{
    FMOD_VECTOR fpos = { pos.mV[0], pos.mV[1], pos.mV[2] };
    FMOD_VECTOR fvel = { 0.f, 0.f, 0.f };
    checkFmod(channel->set3DAttributes(&fpos, &fvel), "Channel::set3DAttributes");
    // Spec §4.3 fixes the inner radius to 1.0 m (no per-speaker {min} field
    // exists in r8; min was dropped from the tag format intentionally).
    checkFmod(channel->set3DMinMaxDistance(1.f, std::max(range, 1.1f)),
              "Channel::set3DMinMaxDistance");
}

// static
FMOD_RESULT F_CALL
LLPositionalStreamMulti::pcmReadCallback(FMOD_SOUND* sound, void* data, U32 datalen)
{
    void* ud = nullptr;
    reinterpret_cast<FMOD::Sound*>(sound)->getUserData(&ud);
    auto* cb = static_cast<SpeakerCallback*>(ud);
    F32* out = static_cast<F32*>(data);
    const size_t n = datalen / sizeof(F32);

    if (!(cb && cb->self && cb->speaker_idx < cb->self->mSpeakers.size()))
    {
        std::memset(out, 0, n * sizeof(F32));
        return FMOD_OK;
    }
    LLPositionalStreamMulti* self = cb->self;

    // r10 P4: dispatch the §4.2 compatibility-matrix routing pre-computed
    // for this speaker at createUserSounds(). All three of mSpeakers,
    // mSourceChannels, mDownmix are settled before the FMOD sound (and
    // hence this callback) exists; tag changes go through stop()/start()
    // which tears down the FMOD sound first. So no re-evaluation or
    // synchronisation is needed on this hot path.
    size_t got = 0;
    switch (cb->op_kind)
    {
    case SpeakerCallback::OpKind::Silent:
        // ch:LFE/SL/SR on 1ch / 2ch source. Output is unconditional silence,
        // but we still consume from the ring at the same rate as the other
        // readers — otherwise the writer's lagging-reader bound would stall
        // the whole stream on this one speaker. No underrun is counted
        // (intentional silence is not a dropout).
        self->mRing.skipFrames(cb->speaker_idx, n);
        std::memset(out, 0, n * sizeof(F32));
        return FMOD_OK;

    case SpeakerCallback::OpKind::Track:
        got = self->mRing.readFramesTrack(cb->speaker_idx, out, n,
                                          static_cast<size_t>(cb->op_track));
        break;

    case SpeakerCallback::OpKind::StereoSum:
        got = self->mRing.readFramesMonoSum(cb->speaker_idx, out, n);
        break;

    case SpeakerCallback::OpKind::Bs775:
    {
        // raw_scratch is sized at createUserSounds() to kReaderChunkFrames
        // × 6 floats; loop covers the rare case of a callback datalen
        // that exceeds one chunk.
        const size_t chunk_cap = kReaderChunkFrames;
        F32* raw = cb->raw_scratch.data();
        size_t produced = 0;
        while (produced < n)
        {
            const size_t this_chunk = std::min(n - produced, chunk_cap);
            const size_t pulled = self->mRing.readFramesRaw(
                cb->speaker_idx, raw, this_chunk);
            if (pulled == 0) break;
            self->mDownmix.mix6chToMono(raw, out + produced, pulled, cb->op_role);
            produced += pulled;
            if (pulled < this_chunk) break;  // ring drained
        }
        got = produced;
        break;
    }

    case SpeakerCallback::OpKind::Upmix:
    {
        // r12 P2: parallel to Bs775 but with a 2-track raw read (ring is
        // 2-track for stereo source) and the LLStereoUpmix transform that
        // produces this speaker's role-specific 1ch output. Per-speaker
        // upmix_state carries the SL/SR delay line and the LFE biquad
        // taps; upmix_params carries the bleed / delay / LFE-cutoff tuning.
        //
        // r12 P6: refresh the live-tunable knobs (LfeCutoff / CenterBleed /
        // RearDelayMs) from the per-stream atomic snapshot. A relaxed load
        // is sufficient because each parameter is independently consumed
        // (no inter-knob ordering invariant); a value the main thread
        // racingly stores during the load is materialised on the next
        // chunk boundary, which the user perceives as a smooth ~20 ms
        // ramp rather than a tearing artefact. sample_rate stays as
        // stamped at createUserSounds() time.
        const F32 lfe_fc = self->mUpmixLfeCutoffHz.load(std::memory_order_relaxed);
        const F32 bleed  = self->mUpmixCenterBleed.load(std::memory_order_relaxed);
        const F32 base   = self->mUpmixRearDelayBaseMs.load(std::memory_order_relaxed);
        F32 l_ms = 0.f, r_ms = 0.f;
        LLStereoUpmix::splitRearDelay(base, kRearDelayJitterMs, &l_ms, &r_ms);
        cb->upmix_params.lfe_cutoff_hz   = lfe_fc;
        cb->upmix_params.center_bleed    = bleed;
        cb->upmix_params.rear_delay_ms_l = l_ms;
        cb->upmix_params.rear_delay_ms_r = r_ms;
        // r12.1: same per-callback refresh for the LFE gain. Read every
        // chunk so a mid-stream {lfegain:N} edit shows up on the next
        // chunk boundary without rebuilding the stream. Only the LFE
        // role consumes lfe_gain in upmix2chToSpeaker(); other roles
        // ignore it so this store is harmless on FL/FR/C/SL/SR cb's.
        cb->upmix_params.lfe_gain        = self->mLfeGain.load(std::memory_order_relaxed);

        const size_t chunk_cap = kReaderChunkFrames;
        F32* raw = cb->raw_scratch.data();
        size_t produced = 0;
        while (produced < n)
        {
            const size_t this_chunk = std::min(n - produced, chunk_cap);
            const size_t pulled = self->mRing.readFramesRaw(
                cb->speaker_idx, raw, this_chunk);
            if (pulled == 0) break;
            self->mUpmix.upmix2chToSpeaker(raw, out + produced, pulled,
                                           cb->op_role_upmix,
                                           cb->upmix_state, cb->upmix_params);
            produced += pulled;
            if (pulled < this_chunk) break;  // ring drained
        }
        got = produced;
        break;
    }
    }

    // r12.1: apply per-stream LFE gain to the 5.1-native LFE feed
    // (Track op pulling the source's LFE channel directly). Upmix path
    // already applied gain inside upmix2chToSpeaker via Params::lfe_gain;
    // skip here to avoid double-application. Bs775 maps 6ch → mono for
    // L/R/M roles only (LFE never goes through Bs775), so the only LFE
    // path that reaches this scale is OpKind::Track.
    if (cb->is_lfe && got > 0 && cb->op_kind == SpeakerCallback::OpKind::Track)
    {
        const F32 g = self->mLfeGain.load(std::memory_order_relaxed);
        if (g != 1.f)
        {
            for (size_t i = 0; i < got; ++i) out[i] *= g;
        }
    }

    if (got < n)
    {
        std::memset(out + got, 0, (n - got) * sizeof(F32));
        // r8 F6 acceptance: count the underrun for the dropout metric. Done
        // here, not in the ring methods, so a speaker whose tail reads
        // short still registers even if the ring isn't globally empty.
        self->mUnderrunFrames.fetch_add(static_cast<U64>(n - got),
                                        std::memory_order_relaxed);
        self->mUnderrunCallbacks.fetch_add(1, std::memory_order_relaxed);
    }
    return FMOD_OK;
}

// static
LLStereoUpmix::UpmixRole
LLPositionalStreamMulti::mapChToUpmixRole(Channel ch)
{
    using R = LLStereoUpmix::UpmixRole;
    switch (ch)
    {
    case Channel::FL:  return R::FL;
    case Channel::FR:  return R::FR;
    case Channel::C:   return R::C;
    case Channel::LFE: return R::LFE;
    case Channel::SL:  return R::SL;
    case Channel::SR:  return R::SR;
    // Spec §4.3.6 legacy fold-in: r5–r9 ch values are mapped to their
    // closest 5.1 role so a pre-r10 desc with only L/R/M speakers still
    // gets a sensible upmix (L = front-left, R = front-right, M = center)
    // rather than silence.
    case Channel::L:   return R::FL;
    case Channel::R:   return R::FR;
    case Channel::M:   return R::C;
    }
    return R::FL;  // unreachable; defensive
}

void LLPositionalStreamMulti::resolveReadOp(SpeakerCallback& cb, Channel ch) const
{
    // r10 P4: §4.2 compat matrix dispatch. mSourceChannels and mDownmix are
    // already settled before this is called (createUserSounds runs after
    // the State::Opening → State::Buffering transition).
    using Op = SpeakerCallback::OpKind;
    using Role = LLMultichannelDownmix::MixRole;

    if (mSourceChannels == 6 && mDownmix.isSupported())
    {
        // r12 P4 auto-bypass: even when the publisher requested {upmix:on},
        // a 6ch native source falls through to the r10 placement / Bs775
        // path verbatim. The mgr-side chat notice (emitUpmixAutoBypassNotice)
        // explains the fall-through to the listener; here we simply ignore
        // mUpmixEnabled so the dispatch is bit-identical to r10/r11.
        const auto& idx = mDownmix.indices();
        switch (ch)
        {
        case Channel::L:   cb.op_kind = Op::Bs775; cb.op_role = Role::L;      return;
        case Channel::R:   cb.op_kind = Op::Bs775; cb.op_role = Role::R;      return;
        case Channel::M:   cb.op_kind = Op::Bs775; cb.op_role = Role::MonoLR; return;
        case Channel::FL:  cb.op_kind = Op::Track; cb.op_track = idx.FL;      return;
        case Channel::FR:  cb.op_kind = Op::Track; cb.op_track = idx.FR;      return;
        case Channel::C:   cb.op_kind = Op::Track; cb.op_track = idx.C;       return;
        case Channel::LFE: cb.op_kind = Op::Track; cb.op_track = idx.LFE;     return;
        case Channel::SL:  cb.op_kind = Op::Track; cb.op_track = idx.SL;      return;
        case Channel::SR:  cb.op_kind = Op::Track; cb.op_track = idx.SR;      return;
        }
    }
    else if (mSourceChannels == 2 && mUpmixEnabled)
    {
        // r12 P4: every speaker — including legacy r5–r9 L/R/M placements —
        // gets fanned out via the DPL2 matrix decode. The per-speaker
        // role is mapped from Channel to UpmixRole; the actual matrix +
        // band split lives in LLStereoUpmix::upmix2chToSpeaker().
        cb.op_kind = Op::Upmix;
        cb.op_role_upmix = mapChToUpmixRole(ch);
        return;
    }
    else if (mSourceChannels == 2)
    {
        // r10 path: no upmix → 2-spk stereo (L/FL=track0, R/FR=track1,
        // M/C=stereo sum, 5.1 placement values silent except FL/FR/C).
        switch (ch)
        {
        case Channel::L:
        case Channel::FL:  cb.op_kind = Op::Track; cb.op_track = 0; return;
        case Channel::R:
        case Channel::FR:  cb.op_kind = Op::Track; cb.op_track = 1; return;
        case Channel::M:
        case Channel::C:   cb.op_kind = Op::StereoSum;              return;
        case Channel::LFE:
        case Channel::SL:
        case Channel::SR:  cb.op_kind = Op::Silent;                 return;
        }
    }
    else  // mSourceChannels == 1 (or unexpected — falls through to Silent)
    {
        // r12 P4: 1ch sources do not get upmix dispatched even if
        // mUpmixEnabled — DPL2 decode of a mono signal collapses to
        // (FL=FR=L, C=L×√2, S=0) which is louder than r8/r10 mono and
        // gains nothing acoustically. The r8 mono fan-out (every non-LFE
        // role plays track 0) is the correct degenerate behavior.
        switch (ch)
        {
        case Channel::L:
        case Channel::R:
        case Channel::M:
        case Channel::FL:
        case Channel::FR:
        case Channel::C:   cb.op_kind = Op::Track; cb.op_track = 0; return;
        case Channel::LFE:
        case Channel::SL:
        case Channel::SR:  cb.op_kind = Op::Silent;                 return;
        }
    }
    // Defensive default — every (ch, src_ch) combination above hits a
    // return; reaching here implies an unhandled channel value or a
    // 3/4/5/7/8 ch source slipping past the codec reject.
    cb.op_kind = Op::Silent;
}

bool LLPositionalStreamMulti::createUserSounds()
{
    FMOD::System* system = getFmodSystem();
    if (!system) return false;

    mSpeakerRuntime.clear();
    mSpeakerRuntime.resize(mSpeakers.size());

    for (size_t i = 0; i < mSpeakers.size(); ++i)
    {
        FMOD_CREATESOUNDEXINFO ex = {};
        ex.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
        ex.numchannels = 1;
        ex.defaultfrequency = mSampleRate;
        ex.format = FMOD_SOUND_FORMAT_PCMFLOAT;
        ex.length = static_cast<U32>(mSampleRate) * sizeof(F32) * 5;
        ex.decodebuffersize = 4096;
        ex.pcmreadcallback = &pcmReadCallback;

        const FMOD_MODE mode = FMOD_OPENUSER
                             | FMOD_CREATESTREAM
                             | FMOD_LOOP_NORMAL
                             | FMOD_3D
                             | FMOD_3D_LINEARSQUAREROLLOFF;

        FMOD::Sound* snd = nullptr;
        if (checkFmod(system->createSound(nullptr, mode, &ex, &snd), "createSound(OPENUSER)"))
        {
            return false;
        }

        // Heap-allocated so the FMOD callback's stored pointer survives
        // moves of the speaker runtime vector (which we don't do, but it's
        // cheaper than promising never to).
        auto cb = std::make_unique<SpeakerCallback>();
        cb->self = this;
        cb->speaker_idx = i;
        // r12.1: stamp the LFE flag once from the speaker config so the
        // pcmReadCallback can apply mLfeGain to the LFE feed regardless
        // of which op_kind ends up routing the audio (Track for 5.1
        // native, Bs775 for 5.1 native via 6ch downmix mapping, Upmix
        // for 2ch source — Upmix path uses upmix_params.lfe_gain instead
        // so the test below isn't run there).
        cb->is_lfe = (mSpeakers[i].ch == Channel::LFE);
        resolveReadOp(*cb, mSpeakers[i].ch);
        // r10 P3: raw-read scratch is only needed by the Bs775 op (6ch
        // source + ch:L/R/M). Track / StereoSum / Silent ops leave it empty.
        // r12 P2: Upmix also raw-reads, but from a 2-track ring → 2 floats
        // per frame. Sized at createUserSounds time so the mixer thread
        // never allocates.
        if (cb->op_kind == SpeakerCallback::OpKind::Bs775)
        {
            cb->raw_scratch.assign(kReaderChunkFrames * 6, 0.f);
        }
        else if (cb->op_kind == SpeakerCallback::OpKind::Upmix)
        {
            cb->raw_scratch.assign(kReaderChunkFrames * 2, 0.f);
            // r12 P4 + P6: stamp the only Params field that's per-stream-
            // immutable (sample_rate). The live-tunable bleed / delay /
            // cutoff fields are refreshed per callback from the per-stream
            // atomic snapshot (see SpeakerCallback::OpKind::Upmix branch in
            // pcmReadCallback) so a debug-settings edit takes effect on
            // the next chunk boundary without a stream rebuild.
            cb->upmix_params.sample_rate = mSampleRate;
        }
        checkFmod(snd->setUserData(cb.get()), "Sound::setUserData");

        mSpeakerRuntime[i].user_sound = snd;
        mSpeakerRuntime[i].cb = std::move(cb);

        // r11 P4: per-speaker lite-HRTF DSP. Created here so its lifecycle
        // matches user_sound; seeded with the speaker's range + position so
        // even before update() ticks, defaults are sane. Failure is
        // non-fatal because the DSP isn't wired into the FMOD chain yet
        // (P5 hooks Channel::addDSP behind the {binaural} tag) — audio
        // still plays through FMOD's built-in panner.
        auto dsp = std::make_unique<LLLiteHrtfDsp>();
        if (dsp->create(system))
        {
            dsp->setSourcePos(mSpeakers[i].position);
            dsp->setRange(mSpeakers[i].range);
            mSpeakerRuntime[i].hrtf_dsp = std::move(dsp);
        }
        else
        {
            LL_WARNS("Stream3D") << "LiteHrtfDsp::create() failed for speaker "
                                  << i << "; skipping per-speaker DSP" << LL_ENDL;
        }
    }
    return true;
}

bool LLPositionalStreamMulti::makeChannelForBinding(size_t i)
{
    FMOD::System* system = getFmodSystem();
    if (!system) return false;

    SpeakerRuntime& sr = mSpeakerRuntime[i];
    if (!sr.user_sound) return false;

    if (checkFmod(system->playSound(sr.user_sound, nullptr, true /*paused*/, &sr.channel),
                  "playSound(speaker)"))
    {
        sr.channel = nullptr;
        return false;
    }
    // r8 F9: pin priority to 0 (highest) immediately so the next iteration's
    // playSound() can't recycle this paused channel out from under us. With
    // 16 speakers, FMOD's free-channel pool is too small to hold them all,
    // and the default priority of a paused channel makes it the prime
    // recycle target — symptom is silent speakers whose pcmReadCallback
    // never fires, which then stalls the multi-tail ring writer.
    checkFmod(sr.channel->setPriority(0), "Channel::setPriority(speaker)");

    // r11 P1: route every speaker channel through the dedicated Stream3D
    // ChannelGroup. This is the single insertion point r11 phases use to
    // attach lite-HRTF / venue-reverb DSPs without touching world SFX.
    // Failure is non-fatal — fall back to master so audio is never silent.
    if (LLAudioEngine_FMODSTUDIO* engine = dynamic_cast<LLAudioEngine_FMODSTUDIO*>(gAudiop))
    {
        if (FMOD::ChannelGroup* group = engine->getStream3DGroup())
        {
            checkFmod(sr.channel->setChannelGroup(group), "Channel::setChannelGroup(Stream3D)");
        }
    }

    applyChannelAttributes(sr.channel, mSpeakers[i].position, mSpeakers[i].range);
    checkFmod(sr.channel->setVolume(mVolume * mSpeakers[i].volume),
              "Channel::setVolume(speaker)");

    // r11 P5: lite-HRTF takeover gated by mBinauralEnabled (= the publisher's
    // {binaural} tag combined with the debug Stream3DBinauralRender override
    // by the mgr). When ON, insert the per-speaker LiteHrtfDsp at the head
    // of the channel chain (= source side, before the panner) and flip
    // set3DLevel to 0.0f so FMOD's built-in panner stops doing its own
    // distance / pan attenuation; the DSP owns ITD + ILD shadow + air
    // absorption. When OFF, leave set3DLevel at 1.0f and skip addDSP — the
    // path is bit-equivalent to r10. addDSP failure falls back to the
    // built-in panner with set3DLevel(1.0f) so we never end up with a
    // silent / un-spatialised channel.
    bool dsp_attached = false;
    if (mBinauralEnabled && sr.hrtf_dsp)
    {
        if (FMOD::DSP* dsp = sr.hrtf_dsp->getDsp())
        {
            if (!checkFmod(sr.channel->addDSP(0 /*head*/, dsp),
                           "Channel::addDSP(LiteHrtfDsp)"))
            {
                dsp_attached = true;
            }
        }
    }
    checkFmod(sr.channel->set3DLevel(dsp_attached ? 0.0f : 1.0f),
              "Channel::set3DLevel(speaker)");
    // r11 P5: per-binding gate state. Quiet by default (LL_DEBUGS), enable
    // the "Stream3D" debug category to confirm whether a given speaker is
    // running through the lite-HRTF DSP or FMOD's built-in panner.
    LL_DEBUGS("Stream3D") << "makeChannelForBinding speaker=" << i
                           << " binaural=" << (mBinauralEnabled ? "on" : "off")
                           << " dsp=" << (sr.hrtf_dsp ? "ok" : "null")
                           << " attached=" << (dsp_attached ? "yes" : "no")
                           << " set3DLevel=" << (dsp_attached ? "0.0" : "1.0")
                           << LL_ENDL;

    // r13: per-speaker LOWPASS_SIMPLE DSP for the OBB-occlusion "muffled"
    // tone. Inserted at the tail (= after lite-HRTF if present) so the
    // muffling is applied to the post-pan signal rather than fed through
    // the binaural ITD/ILD chain. Initial cutoff 22 kHz = effective
    // bypass; LLOcclusionGeometryMgr pushes the live cutoff each tick
    // based on the smoothed direct factor. Failure is non-fatal — we just
    // skip the muffle effect for this speaker (audio still plays).
    FMOD::DSP* lpf = nullptr;
    if (!checkFmod(system->createDSPByType(FMOD_DSP_TYPE_LOWPASS_SIMPLE, &lpf),
                   "createDSPByType(LowpassSimple)") && lpf)
    {
        checkFmod(lpf->setParameterFloat(FMOD_DSP_LOWPASS_SIMPLE_CUTOFF, 22000.f),
                  "DSP::setParameterFloat(LowpassSimple cutoff init)");
        if (checkFmod(sr.channel->addDSP(FMOD_CHANNELCONTROL_DSP_TAIL, lpf),
                      "Channel::addDSP(LowpassSimple)"))
        {
            lpf->release();
            lpf = nullptr;
        }
        else
        {
            sr.lowpass_dsp = lpf;
        }
    }
    return true;
}

bool LLPositionalStreamMulti::startUserChannels()
{
    FMOD::System* system = getFmodSystem();
    if (!system) return false;

    for (size_t i = 0; i < mSpeakers.size(); ++i)
    {
        if (!makeChannelForBinding(i))
        {
            return false;
        }
    }

    // Sample-accurate sync across all N channels (spec §4.7). Same trick as
    // Stereo: read the mixer DSP clock from the first channel, schedule every
    // channel to begin at the same future sample index. ~20 ms lead is enough
    // for FMOD to commit the schedule before the first mixer tick consumes it.
    if (!mSpeakerRuntime.empty() && mSpeakerRuntime[0].channel)
    {
        unsigned long long parent_now = 0;
        checkFmod(mSpeakerRuntime[0].channel->getDSPClock(nullptr, &parent_now),
                  "Channel::getDSPClock");
        const unsigned long long lead = static_cast<unsigned long long>(mSampleRate) / 50;
        const unsigned long long start_at = parent_now + lead;
        for (auto& sr : mSpeakerRuntime)
        {
            if (sr.channel)
            {
                checkFmod(sr.channel->setDelay(start_at, 0, false), "Channel::setDelay");
            }
        }
    }

    for (auto& sr : mSpeakerRuntime)
    {
        if (sr.channel)
        {
            checkFmod(sr.channel->setPaused(false), "Channel::setPaused");
        }
    }
    return true;
}

size_t LLPositionalStreamMulti::pumpSource()
{
    if (!mSourceSound || mSampleRate <= 0 || mSourceChannels <= 0) return 0;
    // Once we've declared the stream dead, don't keep banging on a broken
    // source — the manager's reconnect loop will rebuild us.
    if (mState.load(std::memory_order_acquire) == State::Failed) return 0;

    constexpr size_t kMaxFramesPerPump = 8192;

    const size_t free_frames = mRing.writeAvailable();
    const size_t want_frames = std::min(free_frames, kMaxFramesPerPump);
    if (want_frames == 0) return 0;

    const size_t bytes_per_frame = static_cast<size_t>(mSourceBytesPerSample)
                                 * static_cast<size_t>(mSourceChannels);
    const size_t want_bytes = want_frames * bytes_per_frame;
    if (mReadScratch.size() < want_bytes)
    {
        mReadScratch.resize(want_bytes);
    }

    U32 read_bytes = 0;
    FMOD_RESULT rr = mSourceSound->readData(mReadScratch.data(),
                                            static_cast<U32>(want_bytes),
                                            &read_bytes);
    if (rr != FMOD_OK && rr != FMOD_ERR_FILE_EOF)
    {
        // FMOD_ERR_NOTREADY just means "decoder hasn't fed us yet" — not a
        // fault. Real socket / EOF cascades return other codes; those count
        // toward kMaxReadFailStreak so the manager can rebuild this stream.
        if (rr != FMOD_ERR_NOTREADY)
        {
            ++mReadFailStreak;
            const F64 now = LLTimer::getElapsedSeconds();
            if (now - mLastReadFailLogTime >= 1.0)
            {
                LL_WARNS("Stream3D") << "Sound::readData error (" << mReadFailStreak
                                      << " consecutive): " << FMOD_ErrorString(rr) << LL_ENDL;
                mLastReadFailLogTime = now;
            }
            if (mReadFailStreak >= kMaxReadFailStreak)
            {
                LL_WARNS("Stream3D") << "Multi source readData failed " << mReadFailStreak
                                      << " times consecutively for " << mUrl
                                      << "; transitioning to Failed for reconnect" << LL_ENDL;
                setFailed(FailReason::Network, "readData streak");
            }
        }
        return 0;
    }
    if (read_bytes == 0)
    {
        // r10.x: FMOD's HTTP source can return OK with 0 bytes when the
        // upstream Icecast source has died — there's no error to count
        // toward mReadFailStreak, so without this branch we'd zero-fill
        // forever. Time-stamp the streak start; if it persists past the
        // threshold, flip to Failed so the manager's reconnect cascade
        // rebuilds us. Reset below on any non-zero read.
        const F64 now0 = LLTimer::getElapsedSeconds();
        if (mZeroFillStreakStart == 0.0)
        {
            mZeroFillStreakStart = now0;
        }
        else if (now0 - mZeroFillStreakStart >= kZeroFillStreakLimit)
        {
            LL_WARNS("Stream3D") << "Multi source returned 0 bytes for "
                                  << (now0 - mZeroFillStreakStart) << "s for " << mUrl
                                  << "; transitioning to Failed for reconnect" << LL_ENDL;
            setFailed(FailReason::Network, "zero-fill streak");
        }
        return 0;
    }
    // Successful read: reset failure streaks so a later, independent
    // outage gets its own full budget rather than inheriting old strikes.
    mReadFailStreak = 0;
    mZeroFillStreakStart = 0.0;

    const size_t frames_read = read_bytes / bytes_per_frame;
    const size_t n_tracks = mRing.numTracks();

    // Convert source PCM → ring's track layout. Ring n_tracks is decided in
    // update() State::Opening:
    //   - 1ch source: 2-track ring, mono sample duplicated into both tracks
    //     so ch=L/R speakers each see the full signal at their 3D position.
    //   - 2ch source: 2-track ring, 1:1 copy. readFramesMonoSum() returns the
    //     spec-mandated -6 dB sum-to-mono for ch:M speakers.
    //   - 6ch source (r10): 6-track ring, raw per-channel write. The reader
    //     side decides what to do per-prim per spec §4.2 (BS.775 downmix for
    //     ch:L/R/M, direct track read for ch:FL/FR/C/LFE/SL/SR) — see
    //     resolveReadOp() / pcmReadCallback().
    constexpr size_t kChunkFrames = kPumpChunkFrames;
    std::vector<F32> chunk(kChunkFrames * n_tracks, 0.f);

    const U8* sp = mReadScratch.data();
    size_t remaining = frames_read;

    while (remaining > 0)
    {
        const size_t this_chunk = std::min(remaining, kChunkFrames);
        std::fill(chunk.begin(), chunk.begin() + this_chunk * n_tracks, 0.f);

        if (mSourceChannels == 1)
        {
            // Mono source: duplicate into all ring tracks so every reader op
            // (track read / mono-sum) ends up with the original amplitude.
            if (mSourceIsFloat)
            {
                const F32* f32 = reinterpret_cast<const F32*>(sp);
                for (size_t i = 0; i < this_chunk; ++i)
                {
                    const F32 s = f32[i];
                    for (size_t t = 0; t < n_tracks; ++t)
                    {
                        chunk[i * n_tracks + t] = s;
                    }
                }
            }
            else
            {
                const S16* s16 = reinterpret_cast<const S16*>(sp);
                for (size_t i = 0; i < this_chunk; ++i)
                {
                    const F32 s = static_cast<F32>(s16[i]) * (1.f / 32768.f);
                    for (size_t t = 0; t < n_tracks; ++t)
                    {
                        chunk[i * n_tracks + t] = s;
                    }
                }
            }
        }
        else if (mSourceIsFloat)
        {
            const F32* f32 = reinterpret_cast<const F32*>(sp);
            for (size_t i = 0; i < this_chunk; ++i)
            {
                for (int c = 0; c < mSourceChannels && c < (int)n_tracks; ++c)
                {
                    chunk[i * n_tracks + c] = f32[i * mSourceChannels + c];
                }
            }
        }
        else
        {
            const S16* s16 = reinterpret_cast<const S16*>(sp);
            for (size_t i = 0; i < this_chunk; ++i)
            {
                for (int c = 0; c < mSourceChannels && c < (int)n_tracks; ++c)
                {
                    chunk[i * n_tracks + c] =
                        static_cast<F32>(s16[i * mSourceChannels + c]) * (1.f / 32768.f);
                }
            }
        }

        mRing.writeFrames(chunk.data(), this_chunk);

        sp += this_chunk * bytes_per_frame;
        remaining -= this_chunk;
    }

    return read_bytes;
}

void LLPositionalStreamMulti::update()
{
    const State st = mState.load(std::memory_order_acquire);
    if (st == State::Idle || st == State::Failed) return;
    if (!mSourceSound) return;

    if (st == State::Opening)
    {
        FMOD_OPENSTATE state = FMOD_OPENSTATE_LOADING;
        FMOD_RESULT rr = mSourceSound->getOpenState(&state, nullptr, nullptr, nullptr);
        if (rr != FMOD_OK || state == FMOD_OPENSTATE_ERROR)
        {
            LL_WARNS("Stream3D") << "Multi source open failed: " << mUrl
                                  << " (" << FMOD_ErrorString(rr) << ")" << LL_ENDL;
            releaseAll();
            mUrl.clear();
            setFailed(FailReason::Network, FMOD_ErrorString(rr));
            return;
        }
        if (state != FMOD_OPENSTATE_READY && state != FMOD_OPENSTATE_PLAYING)
        {
            return;
        }

        FMOD_SOUND_TYPE type;
        FMOD_SOUND_FORMAT fmt;
        int channels = 0;
        int bits = 0;
        if (checkFmod(mSourceSound->getFormat(&type, &fmt, &channels, &bits), "Sound::getFormat"))
        {
            releaseAll();
            setFailed(FailReason::Network, "getFormat failed");
            return;
        }
        // FMOD reports plugin codecs as UNKNOWN. The AYAstorm Opus codec
        // populates waveformat.name = "Ogg Opus", which Sound::getName surfaces;
        // promote that to FMOD_SOUND_TYPE_OPUS so downstream layout / logging
        // treats it identically to a built-in Opus decoder.
        if (type == FMOD_SOUND_TYPE_UNKNOWN)
        {
            char namebuf[64] = {0};
            if (mSourceSound->getName(namebuf, sizeof(namebuf)) == FMOD_OK
                && std::strcmp(namebuf, "Ogg Opus") == 0)
            {
                type = FMOD_SOUND_TYPE_OPUS;
            }
        }
        float freq = 44100.f;
        int prio = 0;
        if (checkFmod(mSourceSound->getDefaults(&freq, &prio), "Sound::getDefaults"))
        {
            releaseAll();
            setFailed(FailReason::Network, "getDefaults failed");
            return;
        }
        if (fmt == FMOD_SOUND_FORMAT_PCMFLOAT)
        {
            mSourceIsFloat = true;
            mSourceBytesPerSample = 4;
        }
        else if (fmt == FMOD_SOUND_FORMAT_PCM16)
        {
            mSourceIsFloat = false;
            mSourceBytesPerSample = 2;
        }
        else
        {
            LL_WARNS("Stream3D") << "Source format unsupported (got " << (int)fmt
                                  << "); aborting multi path" << LL_ENDL;
            releaseAll();
            setFailed(FailReason::FormatUnsupported,
                      llformat("sample-format=%d", (int)fmt));
            return;
        }

        mSampleRate = static_cast<int>(freq);
        mSourceChannels = channels;
        mSourceType = type;

        // r10 (§4.2): 6ch sources fill a 6-track ring per-channel; the
        // BS.775 downmix moves to the reader side in P3 (forSourceFormat is
        // still consulted here as the codec-layout gate). 1ch / 2ch use the
        // r8 path unchanged. Other channel counts (3/4/5/7/8) and 6ch from
        // unknown codecs are FormatUnsupported — the manager's reconnect
        // cascade reads failReason() and stops the retry budget for these
        // so the user sees one clear notification.
        if (mSourceChannels == 6)
        {
            mDownmix = LLMultichannelDownmix::forSourceFormat(type, mSourceChannels);
            if (!mDownmix.isSupported())
            {
                LL_WARNS("Stream3D") << "Multi source: 6ch from unsupported codec (FMOD type "
                                      << (int)type << ") — only Vorbis/Opus/FLAC accepted: "
                                      << mUrl << LL_ENDL;
                releaseAll();
                setFailed(FailReason::FormatUnsupported,
                          llformat("channels=6 codec_type=%d", (int)type));
                return;
            }
        }
        else if (mSourceChannels != 1 && mSourceChannels != 2)
        {
            // Capture before releaseAll() resets mSourceChannels to 0 — the
            // failDetail string is read by the manager (and shown to the
            // user) after this returns.
            const int ch_for_log = mSourceChannels;
            LL_WARNS("Stream3D") << "Multi source: unsupported channel count "
                                  << ch_for_log << " for " << mUrl
                                  << " (1/2/6 only)" << LL_ENDL;
            releaseAll();
            setFailed(FailReason::FormatUnsupported,
                      llformat("channels=%d", ch_for_log));
            return;
        }

        // r10: 6ch source goes to a 6-track ring so ch:FL/FR/C/LFE/SL/SR
        // prims can read their own track directly (compat matrix dispatch
        // lands in P4). 1ch / 2ch keep the r9 layout: 2 tracks, with mono
        // duplicated at write time so ch:L/R each see the full signal.
        // P3 wired the reader-side BS.775 downmix path: ch:L/R/M readers
        // on a 6-track ring run mix6chToMono() per pcmReadCallback chunk.
        const size_t n_tracks = (mSourceChannels == 6) ? 6 : 2;
        const size_t cap = nextPow2(kRingFrames);
        mRing.reset(cap, n_tracks, mSpeakers.size());
        mState = State::Buffering;

        LL_INFOS("Stream3D") << "Multi source ready: " << mUrl
                              << " " << mSampleRate << " Hz x " << mSourceChannels
                              << " ch, fmt=" << (mSourceIsFloat ? "PCMFLOAT" : "PCM16")
                              << (mSourceChannels == 6
                                  ? std::string(", layout=") + mDownmix.layoutName()
                                  : std::string())
                              << ", ring cap " << cap << " frames × " << n_tracks << " tracks"
                              << ", speakers=" << mSpeakers.size() << LL_ENDL;

        startDecodeThread();
    }

    if (st == State::Buffering)
    {
        // Each speaker has its own reader tail; checking idx 0 alone is
        // enough because no reader has consumed yet (channels are still
        // unpaused below) so all tails read identical readAvailable values.
        if (mRing.readAvailable(0) >= kPrebufferFrames)
        {
            if (!createUserSounds() || !startUserChannels())
            {
                LL_WARNS("Stream3D") << "Failed to start OPENUSER channels for "
                                      << mUrl << LL_ENDL;
                // r7 M3 invariant: decode thread holds mSourceSound, must
                // join before releaseAll() reaches mSourceSound->release().
                stopDecodeThread();
                releaseAll();
                setFailed(FailReason::Network, "createUserSounds/startUserChannels");
                return;
            }
            mState = State::Playing;
            // r8 F6: anchor the warmup window for the dropout metric, and
            // zero the counters so any callbacks during Buffering→Playing
            // transition don't leak into the Playing measurement.
            mPlayingStartTime = LLTimer::getElapsedSeconds();
            mLastUnderrunLogTime = mPlayingStartTime;
            mUnderrunFrames.store(0, std::memory_order_relaxed);
            mUnderrunCallbacks.store(0, std::memory_order_relaxed);
            LL_INFOS("Stream3D") << "Multi path playing: " << mUrl
                                  << " (" << mSpeakers.size() << " speakers)" << LL_ENDL;
        }
    }

    // r11 P4: per-frame lite-HRTF param push. Listener pose is shared across
    // every speaker's DSP; source pos / range come from the per-speaker
    // SpeakerConfig (the same vector applyChannelAttributes feeds to FMOD's
    // built-in panner, so the two stay coherent). Pushed unconditionally
    // when DSPs exist — they're created in createUserSounds() during
    // Buffering, so this is a no-op until then. Atomic stores are
    // single-writer (main thread) / single-reader (mixer); ordering is
    // relaxed because each param only matters as a snapshot at the next
    // process() call.
    if (!mSpeakerRuntime.empty() && gAudiop)
    {
        const LLVector3 lpos = gAudiop->getListenerPos();
        const LLVector3 lat  = gAudiop->getListenerAt();
        const LLVector3 lup  = gAudiop->getListenerUp();
        for (size_t i = 0; i < mSpeakerRuntime.size() && i < mSpeakers.size(); ++i)
        {
            LLLiteHrtfDsp* dsp = mSpeakerRuntime[i].hrtf_dsp.get();
            if (!dsp) continue;
            dsp->setListenerPos(lpos);
            dsp->setListenerOrientation(lat, lup);
            dsp->setSourcePos(mSpeakers[i].position);
            dsp->setRange(mSpeakers[i].range);
        }
    }

    if (st == State::Playing)
    {
        const F64 now = LLTimer::getElapsedSeconds();
        if (now - mPlayingStartTime >= kUnderrunWarmupSec &&
            now - mLastUnderrunLogTime >= kUnderrunLogPeriod)
        {
            const U64 frames = mUnderrunFrames.exchange(0, std::memory_order_relaxed);
            const U64 calls  = mUnderrunCallbacks.exchange(0, std::memory_order_relaxed);
            const F64 win    = now - mLastUnderrunLogTime;
            // Only log when there were actual underruns. Healthy streams
            // would otherwise spam an INFO line every 10 s. The counter
            // window itself still rolls forward so a single dropped frame
            // shows up as the integer it is, not as a fraction.
            if (frames > 0)
            {
                const F64 fps_per_spk = (mSpeakers.empty() || win <= 0.0)
                                      ? 0.0
                                      : static_cast<F64>(frames)
                                        / static_cast<F64>(mSpeakers.size())
                                        / win;
                LL_WARNS("Stream3D") << "Multi dropout (" << mUrl << "): "
                                      << frames << " zero-fill frames across "
                                      << calls << " callbacks over "
                                      << win << "s (" << fps_per_spk
                                      << " frames/spk/s, speakers="
                                      << mSpeakers.size() << ")" << LL_ENDL;
            }
            mLastUnderrunLogTime = now;
        }
    }
}

void LLPositionalStreamMulti::startDecodeThread()
{
    if (mDecodeThread.joinable()) return;
    mDecodeStop.store(false, std::memory_order_release);
    mDecodeThread = std::thread(&LLPositionalStreamMulti::decodeThreadMain, this);
    LL_INFOS("Stream3D") << "Multi decode thread started for " << mUrl << LL_ENDL;
}

void LLPositionalStreamMulti::stopDecodeThread()
{
    if (!mDecodeThread.joinable()) return;
    {
        std::lock_guard<std::mutex> lk(mDecodeMutex);
        mDecodeStop.store(true, std::memory_order_release);
    }
    mDecodeCv.notify_all();
    mDecodeThread.join();
    LL_INFOS("Stream3D") << "Multi decode thread joined for " << mUrl << LL_ENDL;
}

void LLPositionalStreamMulti::decodeThreadMain()
{
#if LL_LINUX
    pthread_setname_np(pthread_self(), "Stream3D-multi");
#endif

    static constexpr auto kPumpInterval = std::chrono::milliseconds(5);

    while (!mDecodeStop.load(std::memory_order_acquire))
    {
        pumpSource();

        std::unique_lock<std::mutex> lk(mDecodeMutex);
        mDecodeCv.wait_for(lk, kPumpInterval,
                           [this] { return mDecodeStop.load(std::memory_order_acquire); });
    }
}
