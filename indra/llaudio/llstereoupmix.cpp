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

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{
    // 1/√2 — the DPL2 power-preserving normaliser. Spec §4.3.2:
    //   C  = (L + R) / √2, S  = (L - R) / √2,
    //   L' = L - C × bleed / √2, R' = R - C × bleed / √2.
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
    // Front / center / side are stateless; their loops just read in_2ch[]
    // pairs and write the matrix-decoded scalar. Rear pulls through a
    // per-speaker delay line. LFE stays silent until P3.
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
        // P3 lands the biquad LPF on (L+R)/2. Until then leave LFE silent
        // so a P2 listener doesn't get unfiltered low-mid bleeding into
        // the sub channel.
        std::memset(out_mono, 0, frames * sizeof(F32));
        break;

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
