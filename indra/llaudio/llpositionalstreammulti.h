/**
 * @file llpositionalstreammulti.h
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

#ifndef LL_POSITIONAL_STREAM_MULTI_H
#define LL_POSITIONAL_STREAM_MULTI_H

#include "llmultichanneldownmix.h"
#include "llstereoupmix.h"
#include "llstream3durlresolve.h"
#include "stdtypes.h"
#include "v3math.h"

#include "fmodstudio/fmod_common.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace FMOD
{
    class Sound;
    class Channel;
    class System;
    class DSP;
}

class LLLiteHrtfDsp;  // r11 P4: per-speaker lite-HRTF DSP (forward decl)
struct LLPluginAudioRingHeader;

// r8: multi-tail SPSC ring buffer for the distributed-stereo decode thread.
//
// Storage layout: capacity_frames × n_tracks F32 samples, interleaved per
// frame (L0,R0,L1,R1,... for n_tracks=2). The writer is the decode thread
// (Sound::readData() → push frames). Readers are the per-speaker FMOD mixer
// callbacks; each reader keeps its own tail index and pulls a single track
// (or a sum-to-mono of two tracks) out of the shared frame stream.
//
// Multi-tail vs. one-ring-per-speaker: a per-speaker ring would force the
// decode thread to write the same frame N times and would let readers drift
// out of sync if any one of them blocked. Sharing the underlying frames and
// keeping per-reader tails costs O(N) tails but keeps L/R/M phase-coherent
// for free.
class LLMultiTailRing
{
public:
    LLMultiTailRing();
    ~LLMultiTailRing();
    LLMultiTailRing(const LLMultiTailRing&) = delete;
    LLMultiTailRing& operator=(const LLMultiTailRing&) = delete;

    // Allocate buffer for capacity_frames frames × n_tracks tracks, with
    // n_readers independent tails. All tails reset to 0.
    void reset(size_t capacity_frames, size_t n_tracks, size_t n_readers);
    void clear();

    size_t capacityFrames() const { return mCapacityFrames; }
    size_t numTracks() const { return mNumTracks; }
    size_t numReaders() const { return mNumReaders; }

    // Writer-side: how many frames may be appended before catching up to the
    // slowest reader. Determined by the lagging reader (min over tails).
    size_t writeAvailable() const;

    // Reader-side: how many frames are queued for `reader_idx` to consume.
    size_t readAvailable(size_t reader_idx) const;

    // Append n_frames of n_tracks-interleaved F32 samples from src. Returns
    // frames actually written (capped by writeAvailable()).
    size_t writeFrames(const F32* src, size_t n_frames);

    // r10 P4: pull n_frames worth of one specific track from the ring into
    // dst (mono F32 output). track_idx is clamped to numTracks()-1.
    // Returns frames actually read.
    size_t readFramesTrack(size_t reader_idx, F32* dst,
                           size_t n_frames, size_t track_idx);

    // r9: pull n_frames worth of (track0 + track1) * 0.5 mono samples — the
    // sum-to-mono mix used by ch:M (and ch:C fallback) when source is 2ch.
    // For a 1-track ring this collapses to track0; for ≥3-track rings only
    // the first two tracks are summed (callers on 6ch source go through
    // mix6chToMono / readFramesTrack instead). Returns frames actually read.
    size_t readFramesMonoSum(size_t reader_idx, F32* dst, size_t n_frames);

    // r10 P3: pull n_frames worth of raw interleaved samples (n_frames ×
    // n_tracks F32 written to dst) for a single reader. Used by the
    // reader-side BS.775 downmix path on a 6-track ring. dst length must
    // be ≥ n_frames * numTracks(). Returns frames actually read.
    size_t readFramesRaw(size_t reader_idx, F32* dst, size_t n_frames);

    // r10 P4: advance one reader's tail by up to n_frames without copying
    // any samples. Used by the Silent op (ch:LFE/SL/SR with 1ch / 2ch
    // source) so the writer's lagging-reader bound doesn't stall on a
    // speaker that intentionally outputs silence. Returns frames skipped.
    size_t skipFrames(size_t reader_idx, size_t n_frames);

private:
    // One slot reserved (capacity_frames + 1) so full vs. empty stays
    // distinguishable per reader the same way LLFloatRing does.
    std::vector<F32> mBuf;            // size = (capacity_frames + 1) * n_tracks
    size_t mCapacityFrames;           // logical capacity in frames (excl. slot)
    size_t mNumTracks;
    size_t mNumReaders;
    std::atomic<size_t> mWriteFrame;  // frame index of next write
    std::unique_ptr<std::atomic<size_t>[]> mReadFrames; // n_readers tails
};

// r8: distributed-description stereo stream. One source HTTP stream feeds N
// per-speaker FMOD channels via a shared multi-tail ring.
//
// Architecture mirrors LLPositionalStreamStereo but generalises the L/R pair
// to a vector of speakers, each carrying its own channel role (L / R / M),
// rolloff range, volume and 3D position. The state machine, decode thread
// and source-pump path are intentionally the same shape as Stereo so r7 M3's
// shutdown invariants still apply unchanged.
//
// F3-1 scope: source open + decode thread that fills the multi-tail ring.
// No OPENUSER speaker sounds yet; speakers are stored but not instantiated.
// F3-2 will add the per-speaker FMOD wiring + sample-accurate setDelay() sync.
class LLPositionalStreamMulti
{
public:
    // Mirrors LLPositionalStreamMgr::ChannelKind; duplicated here so llaudio
    // does not depend on indra/newview. r10 added the 5.1 placement values
    // (spec_5_1ch_placement.md §4.1); the per-source-channel-count behavior
    // (§4.2 compatibility matrix) is decided in pcmReadCallback.
    enum class Channel { L, R, M, FL, FR, C, LFE, SL, SR };

    struct SpeakerConfig
    {
        Channel ch = Channel::M;
        F32 range = 20.f;       // FMOD set3DMinMaxDistance() max
        F32 volume = 1.f;       // 0..1, multiplied with global mVolume
        LLVector3 position;
    };

    LLPositionalStreamMulti();
    ~LLPositionalStreamMulti();
    LLPositionalStreamMulti(const LLPositionalStreamMulti&) = delete;
    LLPositionalStreamMulti& operator=(const LLPositionalStreamMulti&) = delete;

    // Begin opening url. speakers describes every output point (≥ 1, ≤ cap).
    bool start(const std::string& url, const std::vector<SpeakerConfig>& speakers);
    // Begin reading an already-decoded media plugin PCM ring as the source.
    // The caller owns the ring lifetime and must keep the media plugin alive
    // while this stream is active.
    bool startMedia(LLPluginAudioRingHeader* ring,
                    const std::string& label,
                    const std::vector<SpeakerConfig>& speakers);
    void stop();

    bool isOpen() const { return mSourceSound != nullptr; }
    bool isPlaying() const;
    bool isFailed() const { return mState.load(std::memory_order_acquire) == State::Failed; }

    // r9 P6: distinguish "retryable failure" (network) from "fatal format
    // mismatch" so the manager can stop the reconnect cascade for sources
    // that will never succeed (3/4/5/7/8ch, or 6ch from a codec we don't
    // know the channel layout of). The detail string carries the offending
    // numbers (e.g. "channels=4") for the user-facing notification.
    //
    // `Ok` (not `None`) for the no-failure sentinel — this header reaches
    // PCH paths that include X11/Xlib.h, which defines `None` as a macro.
    enum class FailReason
    {
        Ok,
        Network,
        FormatUnsupported,
    };
    FailReason failReason() const { return mFailReason.load(std::memory_order_acquire); }
    // Snapshot of the detail string captured at the moment of the Failed
    // transition. Caller is expected to read this only after isFailed()
    // returned true; the value is stable from that point on.
    std::string failDetail() const { return mFailDetail; }

    // Source-format observation, available after the source has reached
    // State::Playing (sourceChannels() returns 0 / sourceFormatName() returns
    // "Unknown" before that). r10 P5 routing diagnostic reads these so the
    // mgr-side log can echo "6ch Vorbis from <url>" without the audio module
    // having to know about LL_WARNS / chat / settings gating itself.
    int sourceChannels() const { return mSourceChannels; }
    const char* sourceFormatName() const;

    // Update one speaker's 3D position (caller is responsible for indexing
    // the same way it set up the speaker vector in start()).
    void setSpeakerPosition(size_t idx, const LLVector3& pos);

    // r12.1: live update of one speaker's per-channel volume (the tag
    // {volume:N} value). Idempotent — early-returns when the cached
    // mSpeakers[idx].volume already matches, so the per-poll push from
    // the mgr costs one float compare in the steady state. Pushed via
    // FMOD::Channel::setVolume(mVolume * volume) so master and per-spk
    // multiply consistently with the start()-time path.
    void setSpeakerVolume(size_t idx, F32 volume);

    // Global volume multiplier on top of per-speaker volume.
    void setVolume(F32 volume);

    // r13: visit every active (channel, lowpass-DSP, source-position) tuple
    // so a caller owning newview-side occlusion state can apply
    // Channel::set3DOcclusion AND push the muffling cutoff into the per-
    // speaker LOWPASS_SIMPLE DSP. Skips speakers that have no live channel
    // (still buffering / failed). lowpass_dsp may be null when DSP creation
    // failed — visitor is responsible for the null-check. Const-correct:
    // this method itself does not mutate any FMOD state — the visitor is
    // free to.
    using SpeakerVisitor = std::function<void(FMOD::Channel*,
                                              FMOD::DSP* lowpass_dsp,
                                              const LLVector3& source_pos)>;
    void forEachActiveSpeaker(const SpeakerVisitor& fn) const;

    // r11 P5: enable/disable the per-speaker lite-HRTF DSP (= the
    // {binaural} tag's resolved effective value, computed by the mgr).
    // When ON, makeChannelForBinding() inserts the DSP at the head of the
    // FMOD chain and flips Channel::set3DLevel to 0.0f so the FMOD
    // built-in panner stops attenuating; when OFF, the DSP stays
    // detached and FMOD continues to do its own 3D panning (= r10
    // behavior). Setting this between speaker bring-ups (e.g. caller
    // changes its mind before start() succeeds) is fine; mid-stream
    // toggles are handled by the mgr rebuilding the stream entirely.
    void setBinauralEnabled(bool on) { mBinauralEnabled = on; }
    bool isBinauralEnabled() const { return mBinauralEnabled; }

    // r12 P4: enable/disable the 2ch→5.1 upmix dispatch (= the publisher's
    // {upmix} tag combined with the debug Stream3DUpmix override, computed
    // by the mgr via effectiveUpmix()). When ON and the source is 2ch,
    // resolveReadOp() routes every per-speaker callback through
    // OpKind::Upmix instead of the r10 Track / StereoSum path; when the
    // source is ≥ 6ch the flag is honored as a request but ignored at
    // dispatch time (auto-bypass — r10 native placement always wins,
    // mgr-side fires a one-shot chat notice). Like setBinauralEnabled,
    // mid-stream toggles are handled by the mgr rebuilding the stream
    // entirely so resolveReadOp's per-speaker decision is only consulted
    // at createUserSounds() time.
    void setUpmixEnabled(bool on) { mUpmixEnabled = on; }
    bool isUpmixEnabled() const { return mUpmixEnabled; }

    // r12 P6: live-tunable knobs for the 2ch→5.1 upmix path. Pushed by
    // the mgr each poll from Stream3DUpmixLfeCutoff /
    // Stream3DUpmixCenterBleed / Stream3DUpmixRearDelayMs. Lock-free
    // atomic write here, lock-free atomic read on the FMOD mixer thread
    // (resolveReadOp's per-callback param refresh). The rear delay is
    // split into per-speaker SL/SR taps by ±kRearDelayJitterMs at read
    // time, not push time, so the jitter direction is implicit in the
    // speaker's UpmixRole (SL gets +, SR gets −).
    void setUpmixTuning(F32 lfe_cutoff_hz, F32 center_bleed,
                        F32 rear_delay_base_ms);

    // r12.1: LFE channel gain multiplier. Pushed by the mgr from the
    // {lfegain:N} tag combined with Stream3DLfeGain debug override
    // (effectiveLfeGain). Read on the FMOD mixer thread by both the
    // Upmix LFE branch (after the 80Hz LPF) and the 5.1 native LFE feed
    // (Track op + cb->is_lfe). 1.0 = passthrough, 2.0 = +6dB. Final
    // value is clamped to [0.0, 3.0] by the mgr; we trust the input.
    void setLfeGain(F32 gain) { mLfeGain.store(gain, std::memory_order_relaxed); }

    // r11 P10: viewer-side URL pre-resolve toggle. When enabled (default),
    // start() runs the source URL through LLStream3DUrlResolve before
    // calling FMOD::createStream so HTTPS→HTTP cross-protocol redirects
    // (Cloudflare/CDN fronted Shoutcast/Icecast) get followed up front.
    // The mgr reads `Stream3DUrlPreResolve` from settings and pushes
    // the resolved boolean here before each start(); set this before
    // start() to take effect on the next stream open.
    void setUrlPreResolveEnabled(bool on) { mUrlPreResolveEnabled = on; }

    // Per-frame: drives source state, transitions opening→buffering→playing.
    void update();

private:
    // r13 C: Resolving sits between Idle and Opening. While in Resolving the
    // background curl worker (LLStream3DUrlResolve) is probing the source
    // URL for HTTPS→HTTP redirects; update() polls the resolve result and
    // transitions to Opening once the URL is settled. Pre-r13 the resolve
    // ran synchronously inside start() so this state did not exist.
    enum class State { Idle, Resolving, Opening, Buffering, Playing, Failed };

    // Per-sound user-data passed to FMOD's pcmreadcallback. Heap-allocated
    // because FMOD stores the pointer; the speaker_idx lets one static thunk
    // service all N speakers without scanning a sound→speaker map.
    struct SpeakerCallback
    {
        LLPositionalStreamMulti* self = nullptr;
        size_t speaker_idx = 0;

        // r10 P4: pre-computed routing decision per the §4.2 compatibility
        // matrix. Determined once at createUserSounds() — when both
        // mSourceChannels and mDownmix are settled — so the mixer thread
        // never re-evaluates the matrix.
        enum class OpKind
        {
            Silent,    // zero-fill (ch:LFE/SL/SR on 1ch / 2ch source)
            Track,     // direct read of mRing track[op_track]
            StereoSum, // (track0 + track1)/2 — ch:M/C on 2ch source
            Bs775,     // mix6chToMono(op_role) — ch:L/R/M on 6ch source
            // r12 P2 + P4: 2ch source + effectiveUpmix() == on →
            // DPL2-style matrix decode + band split (FL/FR/C/LFE/SL/SR
            // per role). Parallel to Bs775: a 2-track raw read followed
            // by a stateless transform with per-speaker state (LPF /
            // delay) carried in upmix_state. resolveReadOp() emits this
            // when mSourceChannels == 2 && mUpmixEnabled; the per-speaker
            // role lives in op_role_upmix below.
            Upmix,
        };
        OpKind op_kind = OpKind::Silent;
        int op_track = 0;
        LLMultichannelDownmix::MixRole op_role
            = LLMultichannelDownmix::MixRole::L;

        // r12 P2: per-speaker UpmixRole resolved from SpeakerConfig::ch.
        // Only meaningful when op_kind == Upmix; otherwise ignored.
        LLStereoUpmix::UpmixRole op_role_upmix
            = LLStereoUpmix::UpmixRole::FL;

        // r12 P2: per-speaker tuning + state for the Upmix op. params is
        // populated at createUserSounds() (P4) from settings + per-speaker
        // jitter; state holds the LPF taps (P3) and Ls/Rs delay line.
        // Both stay zero-cost for non-Upmix speakers.
        LLStereoUpmix::Params upmix_params;
        LLStereoUpmix::State  upmix_state;

        // r10 P3 (extended r12 P2): scratch for the raw-read → mono path.
        // Sized at createUserSounds() to kReaderChunkFrames × 6 floats when
        // op_kind is Bs775, kReaderChunkFrames × 2 floats when op_kind is
        // Upmix; left empty for Track / StereoSum / Silent. Only ever
        // accessed by the FMOD mixer thread for this one speaker, so no
        // synchronisation is needed.
        std::vector<F32> raw_scratch;

        // r12.1: true when the speaker's role is LFE (5.1 placement),
        // regardless of op_kind. Used by pcmReadCallback to apply the
        // per-stream mLfeGain to the LFE feed for both 5.1 native paths
        // (Track / Bs775 with role=LFE) and the Upmix path (where the
        // lfe_gain is plumbed through upmix_params instead). Stamped
        // once at createUserSounds() from the SpeakerConfig::ch.
        bool is_lfe = false;
    };

    struct SpeakerRuntime
    {
        FMOD::Sound* user_sound = nullptr;
        FMOD::Channel* channel = nullptr;
        std::unique_ptr<SpeakerCallback> cb;
        // r11 P4: per-speaker lite-HRTF DSP. Created in createUserSounds()
        // alongside user_sound, fed by per-frame param push from update().
        // Not yet inserted into the FMOD signal chain — P5 will gate
        // Channel::addDSP behind the {binaural} tag. unique_ptr lets the
        // implicit SpeakerRuntime dtor stay valid in the .cpp where the
        // LiteHrtfDsp type is complete.
        std::unique_ptr<LLLiteHrtfDsp> hrtf_dsp;
        // r13: per-speaker LOWPASS_SIMPLE DSP for OBB-occlusion "muffled"
        // tone. Created and addDSP'd alongside the channel; cutoff is
        // pushed by LLOcclusionGeometryMgr each tick based on the smoothed
        // direct factor. Default cutoff (~22 kHz) is effectively bypass.
        FMOD::DSP* lowpass_dsp = nullptr;
    };

    static FMOD_RESULT F_CALL pcmReadCallback(FMOD_SOUND* sound, void* data, U32 datalen);

    enum class SourceKind { Url, MediaRing };

    FMOD::System* getFmodSystem() const;

    // r9 P6: capture fail reason + detail before publishing State::Failed so
    // a reader synchronised by isFailed() (acquire) sees both. Always called
    // in place of plain `mState = State::Failed` from this point.
    void setFailed(FailReason reason, std::string detail = {});

    void releaseAll();
    // r13 C: extracted from the original start() body. Calls
    // System::createStream(FMOD_NONBLOCKING) on `url` and, on success,
    // publishes State::Opening. Returns false on FMOD failure (caller
    // decides whether to clear+return-false or setFailed). Used by both
    // the synchronous start path (when pre-resolve is gated off or the
    // worker can't be started) and by the Resolving→Opening transition
    // in update().
    bool openSourceStream(const std::string& url);
    bool validateMediaRing(U32& sample_rate, U32& channels, U32& format_serial) const;
    bool openMediaRingSource();
    bool createUserSounds();
    bool startUserChannels();
    void applyChannelAttributes(FMOD::Channel* channel, const LLVector3& pos, F32 range);

    // r10 P7 (§4.5.2 r11 hook point): bring up one speaker's FMOD::Channel —
    // playSound (paused), priority pin, 3D attributes, volume, and the
    // explicit Channel::set3DLevel(1.0f) that r11 will flip to 0.0f when
    // Steam Audio takes over the binaural pan. Centralising the whole
    // per-speaker bring-up here keeps the takeover edit a single-function
    // change rather than a hunt across startUserChannels(). Returns false
    // if any FMOD call fails; the caller is expected to abort the start.
    bool makeChannelForBinding(size_t i);
    size_t pumpSource();
    size_t pumpMediaRingSource();

    // r10 P4: resolve §4.2 compat matrix into a SpeakerCallback::OpKind +
    // parameters for one speaker, given the current mSourceChannels and
    // mDownmix. r12 P4: also consults mUpmixEnabled to pick OpKind::Upmix
    // over the r10 Track / StereoSum path on 2ch sources. Called once per
    // speaker at createUserSounds() time.
    void resolveReadOp(SpeakerCallback& cb, Channel ch) const;

    // r12 P4: map a Channel placement value to the LLStereoUpmix::UpmixRole
    // the upmix dispatch should produce for it. Spec §4.3.6 table:
    //   FL/FR/C/LFE/SL/SR → identity,
    //   L → FL, R → FR, M → C
    // (legacy r5–r9 ch values absorbed into the closest 5.1 role so a
    // pre-r10 desc still gets 2-spk stereo when {upmix:on}). Static
    // because no member data is consulted.
    static LLStereoUpmix::UpmixRole mapChToUpmixRole(Channel ch);

    void startDecodeThread();
    void stopDecodeThread();
    void decodeThreadMain();

    FMOD::Sound* mSourceSound;
    SourceKind mSourceKind = SourceKind::Url;
    LLPluginAudioRingHeader* mMediaRing = nullptr;
    U32 mMediaFormatSerial = 0;
    int mSampleRate;
    int mSourceChannels;       // 1, 2, or (r9) 6
    int mSourceBytesPerSample; // 2 for PCM16, 4 for PCMFLOAT
    bool mSourceIsFloat;
    // r10 P5: codec type captured at getFormat() time. Used by the mgr-side
    // routing diagnostic log to print a human-readable codec name. Reset to
    // FMOD_SOUND_TYPE_UNKNOWN by releaseAll() so a stale value never bleeds
    // into a later open of a different URL.
    FMOD_SOUND_TYPE mSourceType;

    // r9: populated at format detection when mSourceChannels == 6. The ring
    // is still allocated with n_tracks = 2 — pumpSource() converts each 6ch
    // frame to interleaved L/R via mDownmix before writing.
    LLMultichannelDownmix mDownmix;

    // r12 P2: stateless 2ch→1ch upmix helper, the C 案 counterpart to
    // mDownmix. The instance carries no per-stream data (per-speaker LPF /
    // delay lives in SpeakerCallback::upmix_state); kept as a member only
    // for call-site symmetry with mDownmix and so future codec-aware
    // routing (e.g. Atmos) has a natural extension point.
    LLStereoUpmix mUpmix;

    // Ring is sized at Opening→Buffering. r10: 1ch / 2ch sources use a
    // 2-track ring (mono is duplicated into both tracks at write time so
    // ch=L/R each see the full signal); 6ch sources use a 6-track ring with
    // raw per-channel write so ch:FL/FR/C/LFE/SL/SR can read their own track
    // directly. P3 wired the reader-side BS.775 downmix path for ch:L/R/M on
    // a 6-track ring (pcmReadCallback → readFramesRaw → mix6chToMono); the
    // §4.2 compat matrix dispatch for the placement values lands in P4.
    LLMultiTailRing mRing;

    std::vector<SpeakerConfig> mSpeakers;
    std::vector<SpeakerRuntime> mSpeakerRuntime;

    F32 mVolume;
    // r11 P5: publisher's lite-HRTF intent (after debug override). Owned
    // by the mgr via setBinauralEnabled(); the mixer thread never reads
    // this — gating happens at channel bring-up on the main thread.
    bool mBinauralEnabled = false;
    // r12 P4: publisher's {upmix} intent (after debug override / auto-
    // bypass on >= 6ch source). Owned by the mgr via setUpmixEnabled();
    // resolveReadOp consults it once per speaker at createUserSounds()
    // and the mixer thread never looks at it again.
    bool mUpmixEnabled = false;

    // r12 P6: live snapshot of the 3 Stream3DUpmix* tuning settings.
    // Defaults match settings.xml so the very first FMOD callback already
    // sees correct values even if setUpmixTuning() hasn't been called yet
    // (atomic ctor doesn't take initializers in C++17, so the .cpp
    // constructor seeds them).
    std::atomic<F32> mUpmixLfeCutoffHz;
    std::atomic<F32> mUpmixCenterBleed;
    std::atomic<F32> mUpmixRearDelayBaseMs;
    // r12.1: LFE channel gain multiplier (1.0 = passthrough). Read on
    // the FMOD mixer thread per pcmReadCallback for both Upmix-path
    // and 5.1-native LFE branches. Seeded to 1.0 in the .cpp ctor.
    std::atomic<F32> mLfeGain;
    // r11 P10: viewer-side URL pre-resolve gate. Default true so a caller
    // that forgets to call the setter still gets the redirect-following
    // behavior (matches the settings.xml sentinel default of "enabled").
    bool mUrlPreResolveEnabled = true;
    // r13 C: id of the in-flight async resolve, kInvalidRequestId when
    // none is pending. Set in start() when we transition to State::
    // Resolving, cleared in update()/stop() when the result is consumed
    // or cancelled. Single-threaded (main-thread only) so no atomic.
    LLStream3DUrlResolve::RequestId mResolveRequestId
        = LLStream3DUrlResolve::kInvalidRequestId;
    std::string mUrl;

    std::atomic<State> mState;

    // r9 P6: written before mState is published as Failed (acquire/release on
    // mState orders the visibility). mFailDetail is read-only from the moment
    // mState becomes Failed onwards.
    std::atomic<FailReason> mFailReason{FailReason::Ok};
    std::string mFailDetail;

    std::vector<U8> mReadScratch;

    std::thread mDecodeThread;
    std::atomic<bool> mDecodeStop;
    std::mutex mDecodeMutex;
    std::condition_variable mDecodeCv;

    // r8 F7: detect a dead source so the manager's reconnect loop can rebuild
    // us. Counted on the decode thread; State::Failed is observed by the
    // manager via isFailed() with acquire ordering.
    int mReadFailStreak = 0;
    F64 mLastReadFailLogTime = 0.0;

    // r10.x: when the upstream Icecast source dies, FMOD's HTTP source can
    // return OK with 0 bytes — no error to count toward mReadFailStreak,
    // and the stream stays zero-filling forever. Time-stamp the start of a
    // sustained zero-byte run so we can flip to Failed after a threshold
    // and let the manager's reconnect cascade rebuild us. Reset on any
    // successful (non-zero) read.
    F64 mZeroFillStreakStart = 0.0;

    // r8 F6 acceptance instrumentation: count frames the FMOD mixer callback
    // had to zero-fill because the ring drained (decode thread fell behind
    // or the source stalled). Bumped from the FMOD mixer thread (multiple
    // threads, one per speaker), drained from update() on the main thread.
    // Logged at kUnderrunLogPeriod cadence after a warmup window so
    // prebuffer-drain transients don't show up as "dropouts".
    std::atomic<U64> mUnderrunFrames{0};
    std::atomic<U64> mUnderrunCallbacks{0};
    F64 mPlayingStartTime = 0.0;
    F64 mLastUnderrunLogTime = 0.0;

    static constexpr size_t kPrebufferFrames = 4096;
    static constexpr size_t kRingFrames      = 1 << 15; // ~0.74 s at 44.1 kHz
    // r9: chunk granularity for the source → ring conversion loop. Sized so a
    // single chunk fits comfortably in L1 (1024 frames × 6 ch × 4 B = 24 KB).
    static constexpr size_t kPumpChunkFrames = 1024;
    // r10 P3: chunk size for the reader-side BS.775 downmix. Bounds the per-
    // SpeakerCallback raw-scratch (1024 frames × 6 ch × 4 B = 24 KB). The
    // FMOD mixer callback's datalen is bounded by the OPENUSER decodebuffer
    // (4096 bytes ≈ 1024 mono floats), so a single iteration usually
    // handles the whole callback; the loop is there for safety.
    static constexpr size_t kReaderChunkFrames = 1024;
    // ~1s of pumpSource failures (200 Hz pump) before declaring the stream
    // dead. Generous so brief network hiccups don't trip a teardown.
    static constexpr int kMaxReadFailStreak  = 200;
    // r10.x: how long pumpSource may keep returning OK-with-0-bytes before
    // we declare the transport dead. Generous to ride out decoder warmup
    // and brief upstream stalls; mgr's reconnect cascade picks up after.
    static constexpr F64 kZeroFillStreakLimit = 10.0;
    // r8 F6: skip the first second after Playing transition to discount
    // prebuffer warmup; emit the rolling counter every 10s thereafter.
    static constexpr F64 kUnderrunWarmupSec  = 1.0;
    static constexpr F64 kUnderrunLogPeriod  = 10.0;
    // r12 P6: fixed L/R jitter applied around mUpmixRearDelayBaseMs to
    // produce SL = base + jitter, SR = base − jitter. Per spec §4.3.4 /
    // §4.4 the jitter is intentionally non-tunable (the user-visible knob
    // is the base only); kept compile-time constant so the per-callback
    // path doesn't pay an extra atomic load.
    static constexpr F32 kRearDelayJitterMs  = 2.0f;
};

#endif // LL_POSITIONAL_STREAM_MULTI_H
