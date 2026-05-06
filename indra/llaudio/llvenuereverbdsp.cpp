/**
 * @file llvenuereverbdsp.cpp
 * @brief AYAstorm r11 venue convolution reverb DSP — P6 skeleton.
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

#include "llvenuereverbdsp.h"

#include "fmodstudio/fmod.hpp"
#include "fmodstudio/fmod_errors.h"

#include <cstring>

LLVenueReverbDsp::LLVenueReverbDsp() = default;

LLVenueReverbDsp::~LLVenueReverbDsp()
{
    release();
}

bool LLVenueReverbDsp::create(FMOD::System* system)
{
    if (!system) return false;
    if (mDsp) return true;

    mSystem = system;
    mDspDesc = new FMOD_DSP_DESCRIPTION();
    std::memset(mDspDesc, 0, sizeof(*mDspDesc));
    std::strncpy(mDspDesc->name, "VenueReverbDsp", sizeof(mDspDesc->name) - 1);
    mDspDesc->pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
    mDspDesc->read = &LLVenueReverbDsp::readCallback;

    FMOD_RESULT result = mSystem->createDSP(mDspDesc, &mDsp);
    if (result != FMOD_OK)
    {
        LL_WARNS("Stream3D") << "FMOD::System::createDSP(VenueReverbDsp) failed: "
                             << FMOD_ErrorString(result) << LL_ENDL;
        delete mDspDesc;
        mDspDesc = nullptr;
        mSystem = nullptr;
        return false;
    }

    mDsp->setUserData(this);

    // Stereo in / stereo out — the upstream is the Stream3D ChannelGroup
    // mix (already binaural-stereo from per-speaker LiteHrtfDsps).
    FMOD_SPEAKERMODE mode = FMOD_SPEAKERMODE_DEFAULT;
    int frequency = 44100;
    if (mSystem->getSoftwareFormat(&frequency, &mode, nullptr) == FMOD_OK)
    {
        mDsp->setChannelFormat(FMOD_CHANNELMASK_STEREO, 2, mode);
        if (frequency > 0)
        {
            mSampleRate = static_cast<F32>(frequency);
        }
    }

    return true;
}

void LLVenueReverbDsp::release()
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

void LLVenueReverbDsp::setWetGain(F32 g)
{
    mWetGain.store(g, std::memory_order_relaxed);
}

void LLVenueReverbDsp::setIRPath(const std::string& path)
{
    // P6 stores only; P7 will trigger the actual file load + FFT prep on
    // the main thread and hand the prepared IR to the mixer via ready flag.
    mIRPath = path;
}

FMOD_RESULT F_CALL LLVenueReverbDsp::readCallback(FMOD_DSP_STATE* dsp_state,
                                                   float* inbuffer, float* outbuffer,
                                                   unsigned int length,
                                                   int inchannels, int* outchannels)
{
    const int out_ch = outchannels ? *outchannels : 2;
    if (outchannels) *outchannels = out_ch;
    if (!outbuffer || length == 0) return FMOD_OK;

    LLVenueReverbDsp* self = nullptr;
    FMOD::DSP* this_dsp = static_cast<FMOD::DSP*>(dsp_state->instance);
    this_dsp->getUserData(reinterpret_cast<void**>(&self));
    if (!self || !inbuffer)
    {
        std::memset(outbuffer, 0, length * out_ch * sizeof(float));
        return FMOD_OK;
    }
    self->process(inbuffer, outbuffer, length, inchannels, out_ch);
    return FMOD_OK;
}

void LLVenueReverbDsp::process(const float* in, float* out, unsigned int length,
                               int inchannels, int outchannels)
{
    // P6 skeleton: passthrough. Convolution math arrives in P7.
    // When in/out channel counts agree, this is a single memcpy; otherwise
    // we copy what we can and zero-fill the tail (defensive — the real path
    // is always stereo in / stereo out per the channel-format set in create()).
    if (inchannels == outchannels)
    {
        std::memcpy(out, in, length * outchannels * sizeof(float));
        return;
    }
    const int min_ch = (inchannels < outchannels) ? inchannels : outchannels;
    for (unsigned int i = 0; i < length; ++i)
    {
        for (int c = 0; c < min_ch; ++c)
        {
            out[i * outchannels + c] = in[i * inchannels + c];
        }
        for (int c = min_ch; c < outchannels; ++c)
        {
            out[i * outchannels + c] = 0.f;
        }
    }
}
