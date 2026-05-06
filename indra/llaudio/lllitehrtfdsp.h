/**
 * @file lllitehrtfdsp.h
 * @brief AYAstorm r11 lite-HRTF DSP (mono in → stereo out, per-channel binaural).
 *
 * P2 skeleton: only mono→stereo with linear-square distance attenuation.
 * P3 adds ITD + ILD shadow + air absorption. P4 adds per-frame param push
 * from main thread. P5 wires this DSP into LLPositionalStreamMulti speaker
 * channels via Channel::addDSP().
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

#ifndef LL_LITEHRTFDSP_H
#define LL_LITEHRTFDSP_H

#include "stdtypes.h"
#include "v3math.h"

#include <atomic>

#include "fmodstudio/fmod_common.h"

namespace FMOD
{
    class System;
    class DSP;
}

class LLLiteHrtfDsp
{
public:
    LLLiteHrtfDsp();
    ~LLLiteHrtfDsp();

    LLLiteHrtfDsp(const LLLiteHrtfDsp&) = delete;
    LLLiteHrtfDsp& operator=(const LLLiteHrtfDsp&) = delete;

    bool create(FMOD::System* system);
    void release();

    FMOD::DSP* getDsp() const { return mDsp; }

    // Per-frame param push (main thread). P2 skeleton stores them but only
    // distance gain reads source/listener pos; ITD/ILD/air abs land in P3.
    void setListenerPos(const LLVector3& p);
    void setSourcePos(const LLVector3& p);
    void setRange(F32 max_dist);  // mSpeakers[i].range; min stays 1.0m

private:
    static FMOD_RESULT F_CALL readCallback(FMOD_DSP_STATE* dsp_state,
                                            float* inbuffer, float* outbuffer,
                                            unsigned int length,
                                            int inchannels, int* outchannels);

    void process(const float* in, float* out, unsigned int length, int inchannels);

    FMOD::System* mSystem { nullptr };
    FMOD::DSP* mDsp { nullptr };
    FMOD_DSP_DESCRIPTION* mDspDesc { nullptr };

    // Lock-free single-writer (main) / single-reader (mixer) per param.
    std::atomic<F32> mListenerX { 0.f };
    std::atomic<F32> mListenerY { 0.f };
    std::atomic<F32> mListenerZ { 0.f };
    std::atomic<F32> mSourceX { 0.f };
    std::atomic<F32> mSourceY { 0.f };
    std::atomic<F32> mSourceZ { 0.f };
    std::atomic<F32> mMinDist { 1.0f };
    std::atomic<F32> mMaxDist { 30.0f };  // overwritten by speaker.range in P4
};

#endif // LL_LITEHRTFDSP_H
