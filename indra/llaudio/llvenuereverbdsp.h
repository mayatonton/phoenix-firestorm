/**
 * @file llvenuereverbdsp.h
 * @brief AYAstorm r11 venue convolution reverb DSP (stereo IR convolver).
 *
 * P7 status: parallel-mix convolution + double-buffered IR slot swap.
 * P8 wires the DSP into the Stream3D ChannelGroup behind {venue:...} /
 * {wetgain:N} tags. P9 bundles the default IR set.
 *
 * Slot model:
 *   Two pre-initialized convolver slots live for the lifetime of the DSP.
 *   The mixer thread reads `mActiveSlot` (acquire) once per process() call
 *   and uses that slot for the entire block. The main thread loads/parses
 *   a new IR, fully primes the inactive slot, then publishes the swap with
 *   a release-store. No locks; no audio-thread allocation.
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

#ifndef LL_VENUEREVERBDSP_H
#define LL_VENUEREVERBDSP_H

#include "stdtypes.h"

#include <atomic>
#include <string>
#include <vector>

#include "fmodstudio/fmod_common.h"

#include "llfftconvolver.h"

namespace FMOD
{
    class System;
    class DSP;
}

class LLVenueReverbDsp
{
public:
    LLVenueReverbDsp();
    ~LLVenueReverbDsp();

    LLVenueReverbDsp(const LLVenueReverbDsp&) = delete;
    LLVenueReverbDsp& operator=(const LLVenueReverbDsp&) = delete;

    bool create(FMOD::System* system);
    void release();

    FMOD::DSP* getDsp() const { return mDsp; }

    // Per-frame param push (main thread). Lock-free single-writer atomic.
    void setWetGain(F32 g);  // 0.0 = dry, 1.0 = unity wet mix
    F32  getWetGain() const { return mWetGain.load(std::memory_order_relaxed); }

    // Loads the WAV synchronously on the calling (= main) thread, primes
    // the inactive slot, and atomically swaps it active. Returns false and
    // leaves the previous slot active on any failure (open / parse / sample
    // rate mismatch). Empty path is treated as a no-op — use setWetGain(0)
    // to mute the wet path.
    bool setIRPath(const std::string& path);
    const std::string& getIRPath() const { return mActiveIRPath; }

private:
    static FMOD_RESULT F_CALL readCallback(FMOD_DSP_STATE* dsp_state,
                                            float* inbuffer, float* outbuffer,
                                            unsigned int length,
                                            int inchannels, int* outchannels);

    void process(const float* in, float* out, unsigned int length,
                 int inchannels, int outchannels);

    // Each Slot owns one full L+R partitioned-convolution state — IR
    // partitions, input ring, prev-block tail. Two slots are pre-init'd so
    // the swap path never allocates on the audio thread.
    struct Slot
    {
        LLPartitionedConvolver convL;
        LLPartitionedConvolver convR;
    };

    // Re-init both convolvers in `slot` from the given IR pair. Called only
    // on the main thread (create() and setIRPath()).
    bool primeSlot(Slot& slot, const F32* ir_l, int ir_l_len,
                   const F32* ir_r, int ir_r_len);

    FMOD::System* mSystem { nullptr };
    FMOD::DSP* mDsp { nullptr };
    FMOD_DSP_DESCRIPTION* mDspDesc { nullptr };

    // Lock-free single-writer (main) / single-reader (mixer).
    std::atomic<F32> mWetGain { 0.f };

    // Two slots, A/B. Mixer reads `mActiveSlot` (acquire); main thread
    // primes the OTHER slot then publishes via release-store.
    Slot mSlots[2];
    std::atomic<Slot*> mActiveSlot { nullptr };
    int mInactiveIndex { 1 };       // main-thread only

    // Convolution block size — fixed at create() from FMOD's DSP buffer
    // size. The read callback validates `length` against this and falls
    // through to dry-only on the rare mid-stream change.
    int mBlockSize { 0 };
    F32 mSampleRate { 44100.f };

    // Path of the IR currently active in mActiveSlot. Read by main thread
    // only (mixer never touches), so a plain std::string is fine.
    std::string mActiveIRPath;

    // De-interleave/interleave temp buffers (mixer thread only).
    std::vector<F32> mScratchInL;
    std::vector<F32> mScratchInR;
    std::vector<F32> mScratchWetL;
    std::vector<F32> mScratchWetR;
};

#endif // LL_VENUEREVERBDSP_H
