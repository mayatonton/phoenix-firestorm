/**
 * @file llvenuereverbdsp.cpp
 * @brief AYAstorm r11 venue convolution reverb DSP — P7c.
 *
 * N-venue pre-load: every bundled IR is loaded once at create() and primed
 * into a per-venue slot. setVenue() flips an atomic slot pointer with no
 * file I/O and no allocation, so live venue switches never touch the audio
 * thread's allocator. See header for the slot model contract.
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

    // Catalog of file-backed venues. Index here aligns 1:1 with
    // LLVenueReverbDsp::mSlots. "dry" is special and not in this table.
    struct VenueDef
    {
        const char* name;
        const char* file;
    };
    constexpr VenueDef kVenueDefs[LLVenueReverbDsp::kVenueSlotCount] = {
        { "room_small",   "room_small.wav"   },
        { "room_medium",  "room_medium.wav"  },
        { "hall_small",   "hall_small.wav"   },
        { "hall_medium",  "hall_medium.wav"  },
        { "hall_large",   "hall_large.wav"   },
        { "club",         "club.wav"         },
        { "cathedral",    "cathedral.wav"    },
        { "outdoor",      "outdoor.wav"      },
    };
}

LLVenueReverbDsp::LLVenueReverbDsp() = default;

LLVenueReverbDsp::~LLVenueReverbDsp()
{
    release();
}

const std::vector<std::string>& LLVenueReverbDsp::knownVenues()
{
    static const std::vector<std::string> kNames = []
    {
        std::vector<std::string> v;
        v.reserve(kVenueSlotCount + 1);
        v.emplace_back("dry");
        for (const auto& def : kVenueDefs)
        {
            v.emplace_back(def.name);
        }
        return v;
    }();
    return kNames;
}

int LLVenueReverbDsp::findVenueSlot(const std::string& name) const
{
    for (int i = 0; i < kVenueSlotCount; ++i)
    {
        if (name == kVenueDefs[i].name) return i;
    }
    return -1;
}

bool LLVenueReverbDsp::create(FMOD::System* system, const std::string& ir_dir)
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

    // Match the convolver block size to FMOD's DSP buffer so primeSlot()
    // can be called once per venue at startup with no audio-thread surprise.
    unsigned int dsp_block = 1024;
    int          dsp_numbuf = 0;
    if (mSystem->getDSPBufferSize(&dsp_block, &dsp_numbuf) != FMOD_OK || !isPow2(static_cast<int>(dsp_block)))
    {
        dsp_block = 1024;
    }
    mBlockSize = static_cast<int>(dsp_block);

    mScratchInL.assign(mBlockSize, 0.f);
    mScratchInR.assign(mBlockSize, 0.f);
    mScratchWetL.assign(mBlockSize, 0.f);
    mScratchWetR.assign(mBlockSize, 0.f);

    // Pre-load every file-backed venue. Failures are non-fatal; the slot
    // simply stays !loaded and refuses to be selected later.
    int loaded_count = 0;
    for (int i = 0; i < kVenueSlotCount; ++i)
    {
        mSlots[i].name = kVenueDefs[i].name;
        if (loadVenueSlot(i, ir_dir))
        {
            ++loaded_count;
        }
    }

    // Default selection: "dry" (= bypass). P8 tag parser will switch.
    mActiveSlot.store(nullptr, std::memory_order_release);
    mActiveVenue = "dry";

    LL_INFOS("Stream3D") << "VenueReverbDsp: ready, block=" << mBlockSize
                         << " sample_rate=" << static_cast<int>(mSampleRate + 0.5f)
                         << " venues_loaded=" << loaded_count << "/" << kVenueSlotCount
                         << " (default=dry)" << LL_ENDL;
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

bool LLVenueReverbDsp::loadVenueSlot(int slot_idx, const std::string& ir_dir)
{
    if (slot_idx < 0 || slot_idx >= kVenueSlotCount) return false;
    Slot& slot = mSlots[slot_idx];
    slot.loaded = false;

    // Build path. Caller is expected to pass an absolute dir (engine uses
    // gDirUtilp->getExpandedFilename); we just append the venue's filename.
    std::string path = ir_dir;
    if (!path.empty() && path.back() != '/' && path.back() != '\\')
    {
        path += '/';
    }
    path += kVenueDefs[slot_idx].file;

    LLIRData ir;
    LLIRLoader::Result lr = LLIRLoader::loadWav(path, ir);
    if (lr != LLIRLoader::Result::Ok)
    {
        // Quiet at INFO; venue files may legitimately be missing during
        // dev / partial bundle. P8 / P7c-B will surface the catalog gap.
        LL_INFOS("Stream3D") << "VenueReverbDsp: venue '" << kVenueDefs[slot_idx].name
                             << "' IR not loaded (" << LLIRLoader::resultString(lr)
                             << " at " << path << ")" << LL_ENDL;
        return false;
    }

    const int target_rate = static_cast<int>(mSampleRate + 0.5f);
    if (ir.sample_rate != target_rate)
    {
        LL_WARNS("Stream3D") << "VenueReverbDsp: venue '" << kVenueDefs[slot_idx].name
                             << "' sample rate " << ir.sample_rate
                             << " Hz != mixer rate " << target_rate
                             << " Hz, slot disabled" << LL_ENDL;
        return false;
    }

    if (!primeSlot(slot,
                   ir.samples_l.data(), static_cast<int>(ir.samples_l.size()),
                   ir.samples_r.data(), static_cast<int>(ir.samples_r.size())))
    {
        LL_WARNS("Stream3D") << "VenueReverbDsp: venue '" << kVenueDefs[slot_idx].name
                             << "' convolver prime failed" << LL_ENDL;
        return false;
    }

    slot.loaded = true;
    LL_INFOS("Stream3D") << "VenueReverbDsp: venue '" << kVenueDefs[slot_idx].name
                         << "' loaded (" << ir.samples_l.size()
                         << " frames, src_ch=" << ir.source_channels << ")" << LL_ENDL;
    return true;
}

bool LLVenueReverbDsp::setVenue(const std::string& name)
{
    if (name == "dry")
    {
        mActiveSlot.store(nullptr, std::memory_order_release);
        mActiveVenue = "dry";
        return true;
    }
    const int idx = findVenueSlot(name);
    if (idx < 0)
    {
        LL_WARNS("Stream3D") << "VenueReverbDsp::setVenue: unknown venue '" << name
                             << "'" << LL_ENDL;
        return false;
    }
    if (!mSlots[idx].loaded)
    {
        LL_WARNS("Stream3D") << "VenueReverbDsp::setVenue: venue '" << name
                             << "' IR not loaded, selection refused" << LL_ENDL;
        return false;
    }
    mActiveSlot.store(&mSlots[idx], std::memory_order_release);
    mActiveVenue = name;
    return true;
}

bool LLVenueReverbDsp::isVenueLoaded(const std::string& name) const
{
    if (name == "dry") return true;
    const int idx = findVenueSlot(name);
    if (idx < 0) return false;
    return mSlots[idx].loaded;
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
    // Always copy dry first so the bus is never silent — wet path joins in
    // only when an active slot exists and wet_gain is non-zero.
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

    if (inchannels != 2 || outchannels != 2) return;
    const int len = static_cast<int>(length);
    if (len != mBlockSize) return;

    Slot* slot = mActiveSlot.load(std::memory_order_acquire);
    if (!slot) return;  // "dry" bypass.

    for (int i = 0; i < len; ++i)
    {
        mScratchInL[i] = in[i * 2 + 0];
        mScratchInR[i] = in[i * 2 + 1];
    }

    std::memset(mScratchWetL.data(), 0, len * sizeof(F32));
    std::memset(mScratchWetR.data(), 0, len * sizeof(F32));
    slot->convL.processAdd(mScratchInL.data(), mScratchWetL.data(), wet_gain);
    slot->convR.processAdd(mScratchInR.data(), mScratchWetR.data(), wet_gain);

    for (int i = 0; i < len; ++i)
    {
        out[i * 2 + 0] += mScratchWetL[i];
        out[i * 2 + 1] += mScratchWetR[i];
    }
}
