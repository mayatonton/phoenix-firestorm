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
// to a single mono output channel for one speaker role using static matrix
// decode + band split (LFE LPF / center bleed removal / rear decorrelation).
//
// Architecturally the symmetric counterpart of LLMultichannelDownmix. Where
// downmix takes 6ch source → 1ch per speaker (L / R / MonoLR), upmix takes
// 2ch source → 1ch per speaker (FL / FR / C / LFE / SL / SR). Both are called
// from SpeakerCallback::pcmReadCallback on the FMOD mixer thread, dispatched
// through SpeakerCallback::OpKind (Bs775 vs Upmix).
//
// P2: front (FL/FR with center-bleed-removed L'/R'), center (C = (L+R)/√2),
// and rear (SL/SR with delay-line decorrelation of S = (L-R)/√2) are live.
// P3: LFE = (L + R)/2 → 2nd-order Butterworth LPF (default 80 Hz). All six
// 5.1 roles are now functionally complete inside the helper; what remains
// is wiring (P4 resolveReadOp branch, P5 tag, P6 settings push).
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

    // Tuning parameters (spec §4.4). Defaults match the spec table; the
    // owning LLPositionalStreamMulti refreshes the bleed / delay / cutoff
    // fields per FMOD callback from its own atomic snapshot of the
    // Stream3DUpmix{LfeCutoff,CenterBleed,RearDelayMs} debug settings (P6).
    // `sample_rate` is required for both the rear delay tap (frame count)
    // and the LFE biquad coefficients — stamped once at createUserSounds()
    // time and never rewritten.
    //
    // L/R rear delays are split symmetrically around the base
    // Stream3DUpmixRearDelayMs (default 16 ms) by a ±2 ms jitter so SL and
    // SR are decorrelated from each other; resolveReadOp computes the
    // per-speaker value and stows it here.
    struct Params
    {
        F32 center_bleed   = 1.0f;   // 0..1; 1.0 = full center extraction
        F32 rear_delay_ms_l = 18.0f; // base 16 + jitter +2
        F32 rear_delay_ms_r = 14.0f; // base 16 - jitter -2
        F32 lfe_cutoff_hz   = 80.0f; // 2nd-order Butterworth LPF cutoff
        // r12.1: LFE band gain multiplier applied AFTER the LPF.
        // 1.0 = passthrough (default), 2.0 = +6dB. Range [0.0, 3.0]
        // is enforced upstream by the mgr.
        F32 lfe_gain        = 1.0f;
        int sample_rate     = 44100; // delay frame count + LFE LPF coeffs
    };

    // Per-speaker mutable state, owned by the caller (one instance per
    // SpeakerCallback). LFE uses lpf_state[] as a Direct Form II Transposed
    // biquad, SL/SR use delay_buf as a delay line for Haas-style
    // decorrelation, FL/FR/C are stateless. The delay line is allocated
    // lazily on first SL/SR use; the biquad state lives inline so LFE pays
    // nothing extra at construction.
    struct State
    {
        // Direct Form II Transposed biquad state for the LFE LPF.
        // Two accumulator nodes (z1, z2 in the cookbook); zero-initialised
        // so the first sample carries no startup transient.
        F32 lpf_state[2] = {0.f, 0.f};

        // Ls/Rs delay line. Sized in upmix2chToSpeaker() on first SL/SR
        // call so it matches the runtime sample_rate; capacity covers the
        // maximum decorrelation window allowed by Stream3DUpmixRearDelayMs
        // plus one chunk so reads never collide with writes.
        std::vector<F32> delay_buf;
        std::size_t      delay_write_idx = 0;
    };

    LLStereoUpmix() = default;

    // r12: upmix is algorithmically format-agnostic (no codec-dependent
    // channel ordering — caller already handed us [L,R,L,R,...] interleaved
    // out of the multi-tail ring). Always supported, kept for API symmetry
    // with LLMultichannelDownmix::isSupported().
    bool isSupported() const { return true; }

    // 2-channel interleaved F32 input → 1ch F32 output, one output sample
    // per frame. `in_2ch` is [L0,R0,L1,R1,...] of length `frames * 2` —
    // the layout LLMultiTailRing::readFramesRaw produces on a 2-track ring.
    // `out_mono` receives `frames` samples for the speaker identified by
    // `role`. `state` is mutable per-speaker storage; `params` is the
    // tuning bundle (P6 wires it from settings).
    //
    // Behaviour (P2 + P3):
    //   FL  = L - C · bleed / √2,
    //   FR  = R - C · bleed / √2,
    //   C   = (L + R) / √2,
    //   SL  = delay(+S, rear_delay_ms_l),
    //   SR  = delay(-S, rear_delay_ms_r),
    //   LFE = biquad_lpf((L + R) / 2, lfe_cutoff_hz),
    // where S = (L - R) / √2 and the C used in the FL/FR center-bleed
    // term is the same (L + R) / √2 produced for the C role.
    void upmix2chToSpeaker(const F32* in_2ch, F32* out_mono,
                           std::size_t frames, UpmixRole role,
                           State& state, const Params& params) const;

    static const char* roleName(UpmixRole role);

    // Map the spec §4.4 base + jitter convention into Params::rear_delay_ms_l/r.
    // base_ms is Stream3DUpmixRearDelayMs (default 16); jitter_ms is the fixed
    // ±2 ms split spec §4.3.4 calls for. Pulled out of the parser path so P4
    // (resolveReadOp) and P6 (settings push) share the same arithmetic.
    static void splitRearDelay(F32 base_ms, F32 jitter_ms,
                               F32* out_l_ms, F32* out_r_ms);
};

#endif // LL_STEREO_UPMIX_H
