/**
 * @file llvenuereverbdsp.cpp
 * @brief AYAstorm r11 venue convolution reverb DSP — P7b.
 *
 * Two pre-init'd convolver slots; main thread loads + primes the inactive
 * one and publishes via a release-store on the active pointer. Mixer thread
 * does an acquire-load once per process() and runs the entire block on that
 * snapshot. No locks; no audio-thread allocation past create(). See header
 * for the slot-swap contract in detail.
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

#include "llirloader.h"

#include <cstring>

namespace
{
    inline bool isPow2(int x)
    {
        return x > 0 && (x & (x - 1)) == 0;
    }
}

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

    // Discover FMOD's DSP block size up-front so we can pre-init the
    // convolver slots and never allocate from the audio thread. Falls back
    // to 1024 if FMOD can't tell us — that's a sane modern default and the
    // read callback still guards against a mismatch.
    unsigned int dsp_block = 1024;
    int          dsp_numbuf = 0;
    if (mSystem->getDSPBufferSize(&dsp_block, &dsp_numbuf) != FMOD_OK || !isPow2(static_cast<int>(dsp_block)))
    {
        dsp_block = 1024;
    }
    mBlockSize = static_cast<int>(dsp_block);

    // Prime both slots with a unit-impulse IR (wet path = one-block-delayed
    // dry) so the convolver is ready before any setIRPath call lands.
    static const F32 kUnitImpulse = 1.f;
    if (!primeSlot(mSlots[0], &kUnitImpulse, 1, &kUnitImpulse, 1) ||
        !primeSlot(mSlots[1], &kUnitImpulse, 1, &kUnitImpulse, 1))
    {
        LL_WARNS("Stream3D") << "VenueReverbDsp: convolver pre-init failed at block="
                             << mBlockSize << ", releasing DSP" << LL_ENDL;
        release();
        return false;
    }
    mActiveSlot.store(&mSlots[0], std::memory_order_release);
    mInactiveIndex = 1;
    mActiveIRPath.clear();

    mScratchInL.assign(mBlockSize, 0.f);
    mScratchInR.assign(mBlockSize, 0.f);
    mScratchWetL.assign(mBlockSize, 0.f);
    mScratchWetR.assign(mBlockSize, 0.f);

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
    mActiveSlot.store(nullptr, std::memory_order_release);
}

void LLVenueReverbDsp::setWetGain(F32 g)
{
    mWetGain.store(g, std::memory_order_relaxed);
}

bool LLVenueReverbDsp::primeSlot(Slot& slot,
                                 const F32* ir_l, int ir_l_len,
                                 const F32* ir_r, int ir_r_len)
{
    if (mBlockSize <= 0) return false;
    if (!slot.convL.init(mBlockSize, ir_l, ir_l_len)) return false;
    if (!slot.convR.init(mBlockSize, ir_r, ir_r_len)) return false;
    return true;
}

bool LLVenueReverbDsp::setIRPath(const std::string& path)
{
    // Empty path is a no-op — wet-gain is the right knob for muting.
    if (path.empty())
    {
        return false;
    }
    if (mBlockSize <= 0)
    {
        LL_WARNS("Stream3D") << "VenueReverbDsp::setIRPath called before create()" << LL_ENDL;
        return false;
    }

    LLIRData ir;
    LLIRLoader::Result lr = LLIRLoader::loadWav(path, ir);
    if (lr != LLIRLoader::Result::Ok)
    {
        LL_WARNS("Stream3D") << "VenueReverbDsp: IR load failed for '" << path
                             << "': " << LLIRLoader::resultString(lr) << LL_ENDL;
        return false;
    }

    // Resampling is out of scope here — bundled IRs ship at the viewer's
    // mixing rate. Mismatch is loud and explicit rather than silently wrong.
    const int target_rate = static_cast<int>(mSampleRate + 0.5f);
    if (ir.sample_rate != target_rate)
    {
        LL_WARNS("Stream3D") << "VenueReverbDsp: IR sample rate " << ir.sample_rate
                             << " Hz does not match mixer rate " << target_rate
                             << " Hz, IR not loaded" << LL_ENDL;
        return false;
    }

    Slot& target = mSlots[mInactiveIndex];
    if (!primeSlot(target,
                   ir.samples_l.data(), static_cast<int>(ir.samples_l.size()),
                   ir.samples_r.data(), static_cast<int>(ir.samples_r.size())))
    {
        LL_WARNS("Stream3D") << "VenueReverbDsp: convolver prime failed for '"
                             << path << "'" << LL_ENDL;
        return false;
    }

    // Publish the swap. release here pairs with the mixer's acquire-load
    // in process(), so all of the partition tables written by primeSlot()
    // are visible to the audio thread before it starts using them.
    mActiveSlot.store(&target, std::memory_order_release);
    mInactiveIndex ^= 1;
    mActiveIRPath = path;

    LL_INFOS("Stream3D") << "VenueReverbDsp: IR loaded path='" << path
                         << "' rate=" << ir.sample_rate
                         << " Hz frames=" << ir.samples_l.size()
                         << " src_ch=" << ir.source_channels << LL_ENDL;
    return true;
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
    // r11 P7a: parallel-mix model.  out[i] = dry[i] + wet_gain × (dry ⊛ ir)[i]
    // Step 1: always copy dry first so the bus is never silent — even if
    // the wet path falls through (block-size mismatch, wet_gain==0, slot
    // not yet ready), the listener hears the same signal as P6 passthrough.
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

    // Wet path requires stereo↔stereo and a length matching the block size
    // pre-init'd in create(). FMOD almost never changes the block size
    // mid-stream, but if it does we just stay dry-only for that block.
    if (inchannels != 2 || outchannels != 2) return;
    const int len = static_cast<int>(length);
    if (len != mBlockSize) return;

    // Snapshot the active slot for the duration of this block. acquire
    // pairs with release in setIRPath()/create() so the IR partitions are
    // visible before we use them.
    Slot* slot = mActiveSlot.load(std::memory_order_acquire);
    if (!slot) return;

    // De-interleave L/R from the FMOD interleaved stereo input.
    for (int i = 0; i < len; ++i)
    {
        mScratchInL[i] = in[i * 2 + 0];
        mScratchInR[i] = in[i * 2 + 1];
    }

    // processAdd() *adds* its result, so zero the wet scratch first — we
    // only want this block's wet, not whatever stale value sat there.
    std::memset(mScratchWetL.data(), 0, len * sizeof(F32));
    std::memset(mScratchWetR.data(), 0, len * sizeof(F32));
    slot->convL.processAdd(mScratchInL.data(), mScratchWetL.data(), wet_gain);
    slot->convR.processAdd(mScratchInR.data(), mScratchWetR.data(), wet_gain);

    // Re-interleave wet over the dry copy (parallel-mix add).
    for (int i = 0; i < len; ++i)
    {
        out[i * 2 + 0] += mScratchWetL[i];
        out[i * 2 + 1] += mScratchWetR[i];
    }
}
