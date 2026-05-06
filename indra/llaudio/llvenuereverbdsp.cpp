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
    // r11 P7a: parallel-mix model.
    //   out[i] = dry[i] + wet_gain × (dry ⊛ ir)[i]
    // Step 1 always copies dry to out so the bus is never silent — even if
    // the convolvers are still in their lazy-init window or wet_gain == 0,
    // the listener hears the same signal as the P6 passthrough path.
    if (inchannels == outchannels)
    {
        std::memcpy(out, in, length * outchannels * sizeof(float));
    }
    else
    {
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

    const F32 wet_gain = mWetGain.load(std::memory_order_relaxed);
    if (wet_gain == 0.f) return;

    // We only attempt convolution when both sides are stereo and the block
    // size is a power of 2 (radix-2 FFT requirement). Anything else falls
    // through to dry-only — the bus stays correct, just no wet signal.
    if (inchannels != 2 || outchannels != 2) return;
    const int len = static_cast<int>(length);
    if (len < 64 || (len & (len - 1)) != 0) return;

    // Lazy init: FMOD reveals its actual block size at first call. Guard
    // against a mid-stream change (rare) by re-init'ing if length differs.
    if (!mConvInitialized || mConvBlockSize != len)
    {
        if (!mConvL.init(len, nullptr, 0) || !mConvR.init(len, nullptr, 0))
        {
            // Init failed — surface once and stay in dry-only mode.
            LL_WARNS("Stream3D") << "VenueReverbDsp: convolver init failed at block="
                                 << len << ", dry-only fallback" << LL_ENDL;
            mConvInitialized = false;
            return;
        }
        mScratchInL.assign(len, 0.f);
        mScratchInR.assign(len, 0.f);
        mScratchWetL.assign(len, 0.f);
        mScratchWetR.assign(len, 0.f);
        mConvBlockSize    = len;
        mConvInitialized  = true;
    }

    // De-interleave L/R from the FMOD interleaved stereo input.
    for (int i = 0; i < len; ++i)
    {
        mScratchInL[i] = in[i * 2 + 0];
        mScratchInR[i] = in[i * 2 + 1];
    }

    // Each processAdd accumulates wet_gain × convolved into its 'out'
    // buffer — we zero those first because we only want this block's wet,
    // not whatever stale value the previous block left behind.
    std::memset(mScratchWetL.data(), 0, len * sizeof(F32));
    std::memset(mScratchWetR.data(), 0, len * sizeof(F32));
    mConvL.processAdd(mScratchInL.data(), mScratchWetL.data(), wet_gain);
    mConvR.processAdd(mScratchInR.data(), mScratchWetR.data(), wet_gain);

    // Re-interleave wet into output (parallel-mix add over the dry copy).
    for (int i = 0; i < len; ++i)
    {
        out[i * 2 + 0] += mScratchWetL[i];
        out[i * 2 + 1] += mScratchWetR[i];
    }
}
