/**
 * @file llstereoupmix.h
 * @brief 2ch → per-speaker mono upmix for the distributed-stereo reader (r12).
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

#ifndef LL_STEREO_UPMIX_H
#define LL_STEREO_UPMIX_H

#include "stdtypes.h"

#include <cstddef>
#include <vector>

// r12 (spec_stereo_upmix.md §4.2.1 / §4.3): map a 2-channel (L, R) source frame
// to a single mono output channel for one speaker role using DPL2-style matrix
// decode + band split (LFE LPF / center bleed removal / rear decorrelation).
//
// Architecturally the symmetric counterpart of LLMultichannelDownmix. Where
// downmix takes 6ch source → 1ch per speaker (L / R / MonoLR), upmix takes
// 2ch source → 1ch per speaker (FL / FR / C / LFE / SL / SR). Both are called
// from SpeakerCallback::pcmReadCallback on the FMOD mixer thread, dispatched
// through SpeakerCallback::OpKind (Bs775 vs Upmix).
//
// P1: skeleton only. upmix2chToSpeaker() is currently a passthrough — FL and
// FR forward L and R, the other roles emit silence. The full DPL2 matrix +
// LFE biquad LPF + Ls/Rs delay-line decorrelation lands in P2/P3.
class LLStereoUpmix
{
public:
    // r12 spec §4.3.6: the six 5.1 speaker roles a stereo input gets fanned
    // out to. Legacy r8 ch values (L/R/M) are mapped to FL/FR/C by the caller
    // (resolveReadOp), so this enum stays clean to the 5.1 roles only.
    enum class UpmixRole
    {
        FL,
        FR,
        C,
        LFE,
        SL,
        SR,
    };

    // Per-speaker mutable state, owned by the caller (one instance per
    // SpeakerCallback). LFE uses lpf_state[] as a Direct Form II biquad,
    // SL/SR use delay_buf as a delay line for Haas-style decorrelation,
    // FL/FR/C are stateless. Allocated lazily on first use of a stateful
    // role so passthrough callers pay nothing.
    struct State
    {
        // Direct Form II biquad state for the LFE LPF (P3). Two delay
        // taps (z^-1, z^-2). Zero-initialised so the first sample is clean.
        F32 lpf_state[2] = {0.f, 0.f};

        // Ls/Rs delay line (P2). Sized lazily in P2 when the rear delay
        // setting is known; capacity covers the maximum decorrelation
        // window allowed by Stream3DUpmixRearDelayMs (~32 ms @ 44.1 kHz).
        std::vector<F32> delay_buf;
        std::size_t      delay_write_idx = 0;
    };

    LLStereoUpmix() = default;

    // r12: upmix is algorithmically format-agnostic (no codec-dependent
    // channel ordering — caller already handed us track[0]=L, track[1]=R
    // out of the multi-tail ring). Always supported, kept for API symmetry
    // with LLMultichannelDownmix::isSupported().
    bool isSupported() const { return true; }

    // 2-channel deinterleaved F32 input → 1ch F32 output, one sample per
    // frame. `in_l` / `in_r` come from LLMultiTailRing::readFramesRaw on
    // tracks 0 and 1 respectively (= deinterleaved by construction; the
    // ring stores tracks separately, not interleaved). `out_mono` receives
    // `frames` samples for the speaker identified by `role`.
    //
    // P1 behaviour (skeleton): FL passes L through, FR passes R through,
    // C / LFE / SL / SR emit zeros. P2 swaps the front/center/side path
    // for the DPL2 matrix; P3 wires up the LFE biquad.
    void upmix2chToSpeaker(const F32* in_l, const F32* in_r, F32* out_mono,
                           std::size_t frames, UpmixRole role,
                           State& state) const;

    static const char* roleName(UpmixRole role);
};

#endif // LL_STEREO_UPMIX_H
