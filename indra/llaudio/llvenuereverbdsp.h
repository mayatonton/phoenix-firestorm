/**
 * @file llvenuereverbdsp.h
 * @brief AYAstorm r11 venue convolution reverb DSP (stereo IR convolver).
 *
 * P6 skeleton: stereo passthrough only — no convolution math, no IR loaded.
 * P7 adds the actual partitioned-convolution engine. P8 wires the DSP into
 * the Stream3D ChannelGroup behind {venue:...} / {wetgain:N} tags so the
 * mixed binaural bus is fed through one shared room IR (per-speaker DSPs
 * stay dry; reverb is bus-level for cost + correct early-reflection sum).
 * P9 bundles the default IR set.
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
    // P6 stores only — readCallback ignores until P7 wires the convolver.
    void setWetGain(F32 g);  // 0.0 = dry, 1.0 = unity wet mix
    F32  getWetGain() const { return mWetGain.load(std::memory_order_relaxed); }

    // P8 will swap the active IR by path; P6 stores the request only.
    // Heavy work (file load + FFT prep) belongs off the audio thread, so
    // future implementation will stage the IR on the main thread and hand
    // off to the mixer through a ready-flag, not parse it inline here.
    void setIRPath(const std::string& path);
    const std::string& getIRPath() const { return mIRPath; }

private:
    static FMOD_RESULT F_CALL readCallback(FMOD_DSP_STATE* dsp_state,
                                            float* inbuffer, float* outbuffer,
                                            unsigned int length,
                                            int inchannels, int* outchannels);

    void process(const float* in, float* out, unsigned int length,
                 int inchannels, int outchannels);

    FMOD::System* mSystem { nullptr };
    FMOD::DSP* mDsp { nullptr };
    FMOD_DSP_DESCRIPTION* mDspDesc { nullptr };

    // Lock-free single-writer (main) / single-reader (mixer).
    std::atomic<F32> mWetGain { 0.f };  // P6 default 0 = effectively dry
    // IR path is only read by main thread (mixer just consults a ready flag
    // once P7 lands), so a plain std::string is fine here.
    std::string mIRPath;

    // Sample rate cached at create() (FMOD::System::getSoftwareFormat).
    F32 mSampleRate { 44100.f };

    // P7a: per-channel partitioned-overlap-save convolvers. Lazily init'd
    // on the first read callback once FMOD reveals its actual block size
    // (System::getDSPBufferSize would also work, but lazy init is cheaper
    // than guessing wrong and re-allocating). Kernels default to a single
    // unit-impulse sample → wet path is just a one-block delayed dry, so
    // even before P7b/P7c provide real IRs the math is sane.
    LLPartitionedConvolver mConvL;
    LLPartitionedConvolver mConvR;
    bool mConvInitialized { false };
    int  mConvBlockSize { 0 };
    // De-interleave/interleave temp buffers (mixer thread only — no atomics).
    // Need both in & wet sets because LLPartitionedConvolver::processAdd does
    // not support aliased in/out (the trailing prev-block save would clobber).
    std::vector<F32> mScratchInL;
    std::vector<F32> mScratchInR;
    std::vector<F32> mScratchWetL;
    std::vector<F32> mScratchWetR;
};

#endif // LL_VENUEREVERBDSP_H
