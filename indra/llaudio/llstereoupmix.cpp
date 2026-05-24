/**
 * @file llstereoupmix.cpp
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

#include "linden_common.h"
#include "llstereoupmix.h"

#include "llmath.h"

#include <algorithm>
#include <cmath>

namespace
{
    // 1/√2 — the power-preserving matrix normaliser. Spec §4.3.2:
    //   C  = (L + R) / √2, S  = (L - R) / √2,
    //   L' = L - C × bleed / √2, R' = R - C × bleed / √2.
    // Doubles as α = sin(ω0)/(2Q) for the Butterworth LFE biquad
    // (Q = 1/√2, so α = sin(ω0) × kInvSqrt2).
    constexpr F32 kInvSqrt2 = 0.7071067811865475f;

    // Hard cap on the rear-delay window. Spec §4.4 lists the user-visible
    // range as 4..32 ms; we size the per-speaker delay line to this max so
    // a settings change that lengthens the tap never reallocates the buffer
    // mid-stream. 32 ms × 48000 = 1536 samples; round up to next pow2.
    constexpr F32   kMaxRearDelayMs = 32.0f;
    constexpr int   kMinSampleRate  = 8000;   // defensive floor for sizing math

    inline std::size_t delaySamples(F32 ms, int sample_rate)
    {
        if (ms < 0.f) ms = 0.f;
        const F32 srf = static_cast<F32>(std::max(sample_rate, kMinSampleRate));
        return static_cast<std::size_t>(ms * 0.001f * srf + 0.5f);
    }

    inline std::size_t nextPow2(std::size_t n)
    {
        std::size_t p = 1;
        while (p < n) p <<= 1;
        return p;
    }

    // RBJ Audio EQ Cookbook — 2nd-order Butterworth LPF (Direct Form II
    // Transposed). Same convention as lllitehrtfdsp.cpp's hi-shelf:
    // α = sin(ω0)/(2Q), Q = 1/√2 for Butterworth → α = sin(ω0) × 1/√2.
    // Coefficients are pre-normalised by a0 so biquadStep does no division.
    struct LpfCoeffs
    {
        F32 b0 { 1.f }, b1 { 0.f }, b2 { 0.f }, a1 { 0.f }, a2 { 0.f };
    };

    LpfCoeffs lfeLpfCoeffs(F32 fs, F32 fc)
    {
        if (fc <= 0.f || fs <= 0.f) return {};
        const F32 omega = 2.f * F_PI * fc / fs;
        const F32 cos_w = std::cos(omega);
        const F32 sin_w = std::sin(omega);
        const F32 alpha = sin_w * kInvSqrt2;          // Q = 1/√2

        const F32 b0 = (1.f - cos_w) * 0.5f;
        const F32 b1 =  1.f - cos_w;
        const F32 b2 = b0;
        const F32 a0 = 1.f + alpha;
        const F32 a1 = -2.f * cos_w;
        const F32 a2 = 1.f - alpha;
        return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
    }

    inline F32 biquadStep(const LpfCoeffs& c, F32 x, F32& z1, F32& z2)
    {
        // Direct Form II Transposed — preferred over plain DF-II for low
        // cutoff / float coefficients because the state variables are
        // sums of past inputs/outputs rather than intermediate w[n]
        // values, which keeps quantisation noise bounded near fc.
        const F32 y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;
        return y;
    }
}

// static
const char* LLStereoUpmix::roleName(UpmixRole role)
{
    switch (role)
    {
    case UpmixRole::FL:  return "FL";
    case UpmixRole::FR:  return "FR";
    case UpmixRole::C:   return "C";
    case UpmixRole::LFE: return "LFE";
    case UpmixRole::SL:  return "SL";
    case UpmixRole::SR:  return "SR";
    }
    return "?";
}

// static
void LLStereoUpmix::splitRearDelay(F32 base_ms, F32 jitter_ms,
                                   F32* out_l_ms, F32* out_r_ms)
{
    // Spec §4.3.4: SL gets base + jitter, SR gets base - jitter so their
    // S taps are temporally decorrelated. Clamp at zero so a degenerate
    // base < jitter setting can't produce a negative delay.
    const F32 l = base_ms + jitter_ms;
    const F32 r = base_ms - jitter_ms;
    if (out_l_ms) *out_l_ms = l > 0.f ? l : 0.f;
    if (out_r_ms) *out_r_ms = r > 0.f ? r : 0.f;
}

void LLStereoUpmix::upmix2chToSpeaker(const F32* in_2ch, F32* out_mono,
                                     std::size_t frames, UpmixRole role,
                                     State& state, const Params& params) const
{
    // Front / center are stateless; their loops just read in_2ch[] pairs
    // and write the matrix-decoded scalar. Rear (SL/SR) pulls through a
    // per-speaker delay line; LFE pulls through a 2nd-order LPF biquad.
    // Both stateful roles persist their delay/filter state across calls
    // via `state`.
    switch (role)
    {
    case UpmixRole::FL:
    {
        const F32 g = params.center_bleed * kInvSqrt2;
        for (std::size_t i = 0; i < frames; ++i)
        {
            const F32 L = in_2ch[i * 2];
            const F32 R = in_2ch[i * 2 + 1];
            const F32 C = (L + R) * kInvSqrt2;
            out_mono[i] = L - C * g;
        }
        break;
    }
    case UpmixRole::FR:
    {
        const F32 g = params.center_bleed * kInvSqrt2;
        for (std::size_t i = 0; i < frames; ++i)
        {
            const F32 L = in_2ch[i * 2];
            const F32 R = in_2ch[i * 2 + 1];
            const F32 C = (L + R) * kInvSqrt2;
            out_mono[i] = R - C * g;
        }
        break;
    }
    case UpmixRole::C:
        for (std::size_t i = 0; i < frames; ++i)
        {
            const F32 L = in_2ch[i * 2];
            const F32 R = in_2ch[i * 2 + 1];
            out_mono[i] = (L + R) * kInvSqrt2;
        }
        break;

    case UpmixRole::LFE:
    {
        // Spec §4.3.5: LFE = (L + R) / 2 → 2nd-order Butterworth LPF at
        // params.lfe_cutoff_hz (default 80 Hz). Coefficients are recomputed
        // per call so a settings change picks up cleanly at the next chunk
        // boundary; the cost is one sin / one cos / five multiplies per
        // chunk, dwarfed by the per-sample loop. State is preserved across
        // calls in state.lpf_state[] (z1, z2 of DF-II Transposed).
        const F32 sr = static_cast<F32>(std::max(params.sample_rate, kMinSampleRate));
        const LpfCoeffs c = lfeLpfCoeffs(sr, params.lfe_cutoff_hz);

        F32 z1 = state.lpf_state[0];
        F32 z2 = state.lpf_state[1];
        // r12.1: gain applied AFTER the LPF so the band shape stays
        // intact (saturation / clipping risk is on the caller side —
        // this DSP is float and the FMOD bus mixer sees float too).
        const F32 g = params.lfe_gain;
        for (std::size_t i = 0; i < frames; ++i)
        {
            const F32 L = in_2ch[i * 2];
            const F32 R = in_2ch[i * 2 + 1];
            const F32 x = (L + R) * 0.5f;
            out_mono[i] = biquadStep(c, x, z1, z2) * g;
        }
        state.lpf_state[0] = z1;
        state.lpf_state[1] = z2;
        break;
    }

    case UpmixRole::SL:
    case UpmixRole::SR:
    {
        // Capacity = max delay window + a safety pad large enough that one
        // FMOD callback's worth of writes never overtakes the read tap.
        // Sized lazily on the first SL/SR call so we honor whatever
        // sample_rate the stream actually opened at.
        const std::size_t max_delay = delaySamples(kMaxRearDelayMs, params.sample_rate);
        const std::size_t want_size = nextPow2(max_delay + 1024);
        if (state.delay_buf.size() < want_size)
        {
            state.delay_buf.assign(want_size, 0.f);
            state.delay_write_idx = 0;
        }

        const std::size_t cap   = state.delay_buf.size();
        const std::size_t mask  = cap - 1; // pow2 size → modulo via mask
        F32* buf                = state.delay_buf.data();

        const F32 sign     = (role == UpmixRole::SL) ? +1.0f : -1.0f;
        const F32 ms       = (role == UpmixRole::SL) ? params.rear_delay_ms_l
                                                     : params.rear_delay_ms_r;
        const std::size_t tap = std::min(delaySamples(ms, params.sample_rate),
                                         cap - 1);

        std::size_t w = state.delay_write_idx;
        for (std::size_t i = 0; i < frames; ++i)
        {
            const F32 L = in_2ch[i * 2];
            const F32 R = in_2ch[i * 2 + 1];
            const F32 S = sign * (L - R) * kInvSqrt2;

            // Write first, then read at the (now-up-to-date) tap position.
            // For tap == 0 this collapses to identity (read returns the
            // sample we just wrote); for tap > 0 the read-side index lands
            // on the slot written `tap` iterations ago. Read-first ordering
            // would output 0 at tap == 0 because the write hadn't happened
            // yet — a real-world tap is always > 0 (default 14/18 ms,
            // settings min 4 ms) so this only matters for the degenerate
            // case, but keeping the math uniformly correct is cheaper than
            // documenting an edge.
            buf[w] = S;
            const std::size_t r_idx = (w + cap - tap) & mask;
            out_mono[i] = buf[r_idx];
            w = (w + 1) & mask;
        }
        state.delay_write_idx = w;
        break;
    }
    }
}
