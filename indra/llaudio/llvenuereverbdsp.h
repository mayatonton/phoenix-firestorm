/**
 * @file llvenuereverbdsp.h
 * @brief AYAstorm r11 venue convolution reverb DSP (stereo IR convolver).
 *
 * P7c: N-venue pre-load model. All bundled IRs are loaded once at create()
 * and primed into per-venue convolver slots that live for the DSP's
 * lifetime. setVenue() flips an atomic slot pointer with no file I/O and
 * no allocation, so live venue switches stay clean even on the audio
 * thread cadence. P8 wires {venue:NAME} tags to setVenue(); P9 adds the
 * {wetgain:N} tag.
 *
 * Slot model:
 *   - "dry" is special: it has no slot. Selecting it stores nullptr in
 *     mActiveSlot, and the mixer skips the wet path entirely (CPU zero).
 *   - Each non-dry venue gets one Slot with a primed L+R convolver pair.
 *     Slots that fail to load (file missing / format reject / sample-rate
 *     mismatch) stay marked !loaded and refuse setVenue selection.
 *   - Mixer thread acquire-loads mActiveSlot once per process() call and
 *     uses that snapshot for the entire block. No locks; no allocation.
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
#include "llirloader.h"

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

    // Builds the FMOD DSP, then for every non-dry venue tries to load
    // <ir_dir>/<name>.wav from disk and prime its slot. Missing files are
    // logged once and leave that slot un-selectable. Default active venue
    // is "dry" (= bypass), so the bus is silent-DSP-safe even with zero
    // venue files installed.
    bool create(FMOD::System* system, const std::string& ir_dir);
    void release();

    FMOD::DSP* getDsp() const { return mDsp; }

    void setWetGain(F32 g);  // 0.0 = dry, 1.0 = unity wet mix
    F32  getWetGain() const { return mWetGain.load(std::memory_order_relaxed); }

    // Switch active venue. Returns true on success:
    //   - "dry" always succeeds (publishes nullptr → mixer bypasses wet).
    //   - Any other known venue succeeds only if its IR is loaded.
    // Unknown / unloaded names log a warning and leave the previous active
    // venue in place.
    bool setVenue(const std::string& name);
    const std::string& getActiveVenue() const { return mActiveVenue; }
    bool isVenueLoaded(const std::string& name) const;

    // Hard-coded list of bundled venue names (in catalog order). "dry" is
    // first; the rest are file-backed. P8 tag parser uses this to validate
    // {venue:NAME} input.
    static const std::vector<std::string>& knownVenues();

    // Number of file-backed venue slots (i.e. catalog size minus "dry").
    // Public so the .cpp's catalog table can size itself off the same name.
    static constexpr int kVenueSlotCount = 8;

private:
    static FMOD_RESULT F_CALL readCallback(FMOD_DSP_STATE* dsp_state,
                                            float* inbuffer, float* outbuffer,
                                            unsigned int length,
                                            int inchannels, int* outchannels);

    void process(const float* in, float* out, unsigned int length,
                 int inchannels, int outchannels);

    struct Slot
    {
        LLPartitionedConvolver convL;
        LLPartitionedConvolver convR;
        std::string name;
        bool loaded { false };
    };

    bool primeSlot(Slot& slot, const F32* ir_l, int ir_l_len,
                   const F32* ir_r, int ir_r_len);

    // Loads one venue's WAV from <ir_dir>/<file> and measures its energy
    // (= Σ sample²). Does NOT prime the convolver — create() runs this on
    // every catalog entry first so the energies can be averaged, then
    // applies the per-IR normalization gain before priming. Returns true
    // when the IR is usable (loaded + sample-rate matches the mixer);
    // energy_out is only valid when the function returns true.
    bool stageVenueIR(int slot_idx, const std::string& ir_dir,
                      LLIRData& ir_out, F64& energy_out);

    // Look up a non-dry venue name → slot index, or -1 if not in catalog.
    int findVenueSlot(const std::string& name) const;

    FMOD::System* mSystem { nullptr };
    FMOD::DSP* mDsp { nullptr };
    FMOD_DSP_DESCRIPTION* mDspDesc { nullptr };

    std::atomic<F32> mWetGain { 0.f };

    Slot mSlots[kVenueSlotCount];

    // nullptr = "dry" / bypass. Otherwise points into mSlots.
    std::atomic<Slot*> mActiveSlot { nullptr };
    // Main-thread mirror of the active selection (mixer never reads).
    std::string mActiveVenue { "dry" };

    int mBlockSize { 0 };
    F32 mSampleRate { 44100.f };

    std::vector<F32> mScratchInL;
    std::vector<F32> mScratchInR;
    std::vector<F32> mScratchWetL;
    std::vector<F32> mScratchWetR;
};

#endif // LL_VENUEREVERBDSP_H
