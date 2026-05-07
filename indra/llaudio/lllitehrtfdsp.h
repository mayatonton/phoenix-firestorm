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

    // Per-frame param push (main thread). Lock-free single-writer atomics.
    // Wired up by P4.
    void setListenerPos(const LLVector3& p);
    void setSourcePos(const LLVector3& p);
    void setRange(F32 max_dist);  // mSpeakers[i].range; min stays 1.0m
    // r11 P3: listener orientation (at = forward axis, up = up axis). Used
    // by ITD/ILD to derive azimuth/elevation of the source relative to the
    // listener. Defaults are an orthogonal SL-frame sentinel; P4 overwrites.
    void setListenerOrientation(const LLVector3& at, const LLVector3& up);

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
    // P3: listener orientation. Defaults at = +X (east), up = +Z (sky) so
    // the basis is well-formed even before the first P4 push.
    std::atomic<F32> mListenerAtX { 1.f };
    std::atomic<F32> mListenerAtY { 0.f };
    std::atomic<F32> mListenerAtZ { 0.f };
    std::atomic<F32> mListenerUpX { 0.f };
    std::atomic<F32> mListenerUpY { 0.f };
    std::atomic<F32> mListenerUpZ { 1.f };
    std::atomic<F32> mMinDist { 1.0f };
    std::atomic<F32> mMaxDist { 30.0f };  // overwritten by speaker.range in P4

    // ---- mixer-thread state (single-thread access, no atomics) ----
    // Sample rate cached at create() (FMOD::System::getSoftwareFormat).
    F32 mSampleRate { 44100.f };
    // ITD delay line per ear (Lagrange 3rd-order fractional delay).
    // Capacity must comfortably exceed max ITD ≈ (a/c)*(π/2 + 1) seconds.
    // 0.66 ms × 96 kHz = 64 samples → 128 gives ample margin + 4-tap interp.
    static constexpr int kDelayBufFrames = 128;
    F32 mDelayL[kDelayBufFrames] {};
    F32 mDelayR[kDelayBufFrames] {};
    int mDelayWrite { 0 };
    // Per-ear biquad state — air-absorption hi-shelf (knee 4 kHz, distance-driven)
    F32 mAirZ1L { 0.f }, mAirZ2L { 0.f };
    F32 mAirZ1R { 0.f }, mAirZ2R { 0.f };
    // Per-ear biquad state — shadow + elevation hi-shelf (knee 2 kHz,
    // azimuth-side + elevation driven; combined into one shelf per ear).
    F32 mShadZ1L { 0.f }, mShadZ2L { 0.f };
    F32 mShadZ1R { 0.f }, mShadZ2R { 0.f };
};

#endif // LL_LITEHRTFDSP_H
