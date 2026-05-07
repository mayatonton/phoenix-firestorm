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

#include <cstring>

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

void LLStereoUpmix::upmix2chToSpeaker(const F32* in_l, const F32* in_r, F32* out_mono,
                                     std::size_t frames, UpmixRole role,
                                     State& /*state*/) const
{
    // P1 skeleton: FL/FR are L/R passthrough; C/LFE/SL/SR are silent.
    // P2 replaces this with the full DPL2 matrix (C, S, L', R'), the
    // center bleed parameter, and the Ls/Rs delay-line tap.
    // P3 adds the biquad LPF on the LFE branch.
    switch (role)
    {
    case UpmixRole::FL:
        std::memcpy(out_mono, in_l, frames * sizeof(F32));
        break;
    case UpmixRole::FR:
        std::memcpy(out_mono, in_r, frames * sizeof(F32));
        break;
    case UpmixRole::C:
    case UpmixRole::LFE:
    case UpmixRole::SL:
    case UpmixRole::SR:
        std::memset(out_mono, 0, frames * sizeof(F32));
        break;
    }
}
