/**
 * @file lllitehrtfdsp.cpp
 * @brief AYAstorm r11 lite-HRTF DSP — P2 skeleton.
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

#include "lllitehrtfdsp.h"

#include "fmodstudio/fmod.hpp"
#include "fmodstudio/fmod_errors.h"

#include <cmath>
#include <cstring>

LLLiteHrtfDsp::LLLiteHrtfDsp() = default;

LLLiteHrtfDsp::~LLLiteHrtfDsp()
{
    release();
}

bool LLLiteHrtfDsp::create(FMOD::System* system)
{
    if (!system) return false;
    if (mDsp) return true;

    mSystem = system;
    mDspDesc = new FMOD_DSP_DESCRIPTION();
    std::memset(mDspDesc, 0, sizeof(*mDspDesc));
    std::strncpy(mDspDesc->name, "LiteHrtfDsp", sizeof(mDspDesc->name) - 1);
    mDspDesc->pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
    mDspDesc->read = &LLLiteHrtfDsp::readCallback;

    FMOD_RESULT result = mSystem->createDSP(mDspDesc, &mDsp);
    if (result != FMOD_OK)
    {
        LL_WARNS("Stream3D") << "FMOD::System::createDSP(LiteHrtfDsp) failed: "
                             << FMOD_ErrorString(result) << LL_ENDL;
        delete mDspDesc;
        mDspDesc = nullptr;
        mSystem = nullptr;
        return false;
    }

    mDsp->setUserData(this);

    FMOD_SPEAKERMODE mode = FMOD_SPEAKERMODE_DEFAULT;
    int frequency = 44100;
    if (mSystem->getSoftwareFormat(&frequency, &mode, nullptr) == FMOD_OK)
    {
        mDsp->setChannelFormat(FMOD_CHANNELMASK_STEREO, 2, mode);
    }

    return true;
}

void LLLiteHrtfDsp::release()
{
    if (mDsp)
    {
        mDsp->release();
        mDsp = nullptr;
    }
    if (mDspDesc)
    {
        delete mDspDesc;
        mDspDesc = nullptr;
    }
    mSystem = nullptr;
}

void LLLiteHrtfDsp::setListenerPos(const LLVector3& p)
{
    mListenerX.store(p.mV[0], std::memory_order_relaxed);
    mListenerY.store(p.mV[1], std::memory_order_relaxed);
    mListenerZ.store(p.mV[2], std::memory_order_relaxed);
}

void LLLiteHrtfDsp::setSourcePos(const LLVector3& p)
{
    mSourceX.store(p.mV[0], std::memory_order_relaxed);
    mSourceY.store(p.mV[1], std::memory_order_relaxed);
    mSourceZ.store(p.mV[2], std::memory_order_relaxed);
}

void LLLiteHrtfDsp::setRange(F32 max_dist)
{
    mMaxDist.store(max_dist, std::memory_order_relaxed);
}

FMOD_RESULT F_CALL LLLiteHrtfDsp::readCallback(FMOD_DSP_STATE* dsp_state,
                                                float* inbuffer, float* outbuffer,
                                                unsigned int length,
                                                int inchannels, int* outchannels)
{
    if (outchannels) *outchannels = 2;
    if (!outbuffer || length == 0) return FMOD_OK;

    LLLiteHrtfDsp* self = nullptr;
    FMOD::DSP* this_dsp = static_cast<FMOD::DSP*>(dsp_state->instance);
    this_dsp->getUserData(reinterpret_cast<void**>(&self));
    if (!self || !inbuffer)
    {
        std::memset(outbuffer, 0, length * 2 * sizeof(float));
        return FMOD_OK;
    }
    self->process(inbuffer, outbuffer, length, inchannels);
    return FMOD_OK;
}

void LLLiteHrtfDsp::process(const float* in, float* out, unsigned int length, int inchannels)
{
    // r11 P2 skeleton: mono → stereo with linear-square distance attenuation
    // (r10 FMOD_3D_LINEARSQUAREROLLOFF reproduction, spec §4.3.3).
    // P3 will add ITD + ILD shadow + air absorption.
    // P4 will start pushing real listener / source pos via setListenerPos /
    // setSourcePos; until then both default to origin so distance = 0 and
    // the DSP behaves as a transparent mono→stereo splitter.
    const F32 lx = mListenerX.load(std::memory_order_relaxed);
    const F32 ly = mListenerY.load(std::memory_order_relaxed);
    const F32 lz = mListenerZ.load(std::memory_order_relaxed);
    const F32 sx = mSourceX.load(std::memory_order_relaxed);
    const F32 sy = mSourceY.load(std::memory_order_relaxed);
    const F32 sz = mSourceZ.load(std::memory_order_relaxed);
    const F32 dx = sx - lx;
    const F32 dy = sy - ly;
    const F32 dz = sz - lz;
    const F32 d  = std::sqrt(dx * dx + dy * dy + dz * dz);
    const F32 min_d = mMinDist.load(std::memory_order_relaxed);
    const F32 max_d = mMaxDist.load(std::memory_order_relaxed);

    F32 gain;
    if (d <= min_d)
    {
        gain = 1.0f;
    }
    else if (max_d <= min_d || d >= max_d)
    {
        gain = (d >= max_d) ? 0.0f : 1.0f;
    }
    else
    {
        const F32 t = (d - min_d) / (max_d - min_d);
        gain = 1.0f - t * t;
    }

    if (inchannels <= 0) inchannels = 1;
    const F32 inv_ch = (inchannels > 1) ? (1.0f / static_cast<F32>(inchannels)) : 1.0f;
    for (unsigned int i = 0; i < length; ++i)
    {
        F32 sample = 0.f;
        for (int c = 0; c < inchannels; ++c)
        {
            sample += in[i * inchannels + c];
        }
        sample *= inv_ch;
        sample *= gain;
        out[i * 2 + 0] = sample;
        out[i * 2 + 1] = sample;
    }
}
