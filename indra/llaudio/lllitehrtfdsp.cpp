/**
 * @file lllitehrtfdsp.cpp
 * @brief AYAstorm r11 lite-HRTF DSP — P2 skeleton.
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

#include "lllitehrtfdsp.h"

#include "llmath.h"
#include "fmodstudio/fmod.hpp"
#include "fmodstudio/fmod_errors.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{
    // RBJ Audio EQ Cookbook — high-shelf biquad (Direct Form II Transposed).
    // S = 1 (default shelf slope) → α = sin(ω0)/√2.
    struct BiquadCoeffs
    {
        F32 b0 { 1.f }, b1 { 0.f }, b2 { 0.f }, a1 { 0.f }, a2 { 0.f };
    };

    BiquadCoeffs hiShelfCoeffs(F32 fs, F32 fc, F32 gain_db)
    {
        if (fc <= 0.f || fs <= 0.f) return {};
        const F32 A      = std::pow(10.f, gain_db / 40.f);
        const F32 omega  = 2.f * F_PI * fc / fs;
        const F32 cos_w  = std::cos(omega);
        const F32 sin_w  = std::sin(omega);
        const F32 alpha  = sin_w / std::sqrt(2.f);          // S = 1
        const F32 sqrtA  = std::sqrt(A);
        const F32 t      = 2.f * sqrtA * alpha;

        const F32 b0 =     A * ((A + 1.f) + (A - 1.f) * cos_w + t);
        const F32 b1 = -2.f * A * ((A - 1.f) + (A + 1.f) * cos_w);
        const F32 b2 =     A * ((A + 1.f) + (A - 1.f) * cos_w - t);
        const F32 a0 =          (A + 1.f) - (A - 1.f) * cos_w + t;
        const F32 a1 =  2.f *      ((A - 1.f) - (A + 1.f) * cos_w);
        const F32 a2 =          (A + 1.f) - (A - 1.f) * cos_w - t;
        return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
    }

    inline F32 biquadStep(const BiquadCoeffs& c, F32 x, F32& z1, F32& z2)
    {
        // Direct Form II Transposed
        const F32 y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;
        return y;
    }

    // Lagrange 3rd-order interpolation over 4 consecutive samples.
    // Fractional offset d ∈ [0, 1] relative to x0.
    inline F32 lagrange4(F32 xm1, F32 x0, F32 x1, F32 x2, F32 d)
    {
        const F32 dm1 = d - 1.f;
        const F32 dm2 = d - 2.f;
        const F32 dp1 = d + 1.f;
        const F32 c_m1 = -d   * dm1 * dm2 / 6.f;
        const F32 c_0  =  dp1 * dm1 * dm2 / 2.f;
        const F32 c_1  = -dp1 * d   * dm2 / 2.f;
        const F32 c_2  =  dp1 * d   * dm1 / 6.f;
        return xm1 * c_m1 + x0 * c_0 + x1 * c_1 + x2 * c_2;
    }
}

LLLiteHrtfDsp::LLLiteHrtfDsp() = default;

LLLiteHrtfDsp::~LLLiteHrtfDsp()
{
    release();
}

bool LLLiteHrtfDsp::create(FMOD::System* system)
{
    if (!system) return false;
    if (mDsp) return true;

    mSystem = system;
    mDspDesc = new FMOD_DSP_DESCRIPTION();
    std::memset(mDspDesc, 0, sizeof(*mDspDesc));
    std::strncpy(mDspDesc->name, "LiteHrtfDsp", sizeof(mDspDesc->name) - 1);
    mDspDesc->pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
    mDspDesc->read = &LLLiteHrtfDsp::readCallback;

    FMOD_RESULT result = mSystem->createDSP(mDspDesc, &mDsp);
    if (result != FMOD_OK)
    {
        LL_WARNS("Stream3D") << "FMOD::System::createDSP(LiteHrtfDsp) failed: "
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

    return true;
}

void LLLiteHrtfDsp::release()
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

void LLLiteHrtfDsp::setListenerPos(const LLVector3& p)
{
    mListenerX.store(p.mV[0], std::memory_order_relaxed);
    mListenerY.store(p.mV[1], std::memory_order_relaxed);
    mListenerZ.store(p.mV[2], std::memory_order_relaxed);
}

void LLLiteHrtfDsp::setSourcePos(const LLVector3& p)
{
    mSourceX.store(p.mV[0], std::memory_order_relaxed);
    mSourceY.store(p.mV[1], std::memory_order_relaxed);
    mSourceZ.store(p.mV[2], std::memory_order_relaxed);
}

void LLLiteHrtfDsp::setRange(F32 max_dist)
{
    mMaxDist.store(max_dist, std::memory_order_relaxed);
}

void LLLiteHrtfDsp::setListenerOrientation(const LLVector3& at, const LLVector3& up)
{
    mListenerAtX.store(at.mV[0], std::memory_order_relaxed);
    mListenerAtY.store(at.mV[1], std::memory_order_relaxed);
    mListenerAtZ.store(at.mV[2], std::memory_order_relaxed);
    mListenerUpX.store(up.mV[0], std::memory_order_relaxed);
    mListenerUpY.store(up.mV[1], std::memory_order_relaxed);
    mListenerUpZ.store(up.mV[2], std::memory_order_relaxed);
}

FMOD_RESULT F_CALL LLLiteHrtfDsp::readCallback(FMOD_DSP_STATE* dsp_state,
                                                float* inbuffer, float* outbuffer,
                                                unsigned int length,
                                                int inchannels, int* outchannels)
{
    if (outchannels) *outchannels = 2;
    if (!outbuffer || length == 0) return FMOD_OK;

    LLLiteHrtfDsp* self = nullptr;
    FMOD::DSP* this_dsp = static_cast<FMOD::DSP*>(dsp_state->instance);
    this_dsp->getUserData(reinterpret_cast<void**>(&self));
    if (!self || !inbuffer)
    {
        std::memset(outbuffer, 0, length * 2 * sizeof(float));
        return FMOD_OK;
    }
    self->process(inbuffer, outbuffer, length, inchannels);
    return FMOD_OK;
}

void LLLiteHrtfDsp::process(const float* in, float* out, unsigned int length, int inchannels)
{
    // r11 P3: mono → stereo with linear-square distance attenuation
    // (spec §4.3.3) + air absorption (§4.3.4) + ITD (§4.3.5) + ILD shadow
    // (§4.3.6) + elevation HF tilt (§4.3.7). Coefficients are computed once
    // per process() call (block-rate); biquad / delay-line state runs at
    // sample rate. P4 will start pushing real listener / source pose; until
    // then defaults yield d = 0 and az = elev = 0 (transparent stereo).
    const F32 lx = mListenerX.load(std::memory_order_relaxed);
    const F32 ly = mListenerY.load(std::memory_order_relaxed);
    const F32 lz = mListenerZ.load(std::memory_order_relaxed);
    const F32 sx = mSourceX.load(std::memory_order_relaxed);
    const F32 sy = mSourceY.load(std::memory_order_relaxed);
    const F32 sz = mSourceZ.load(std::memory_order_relaxed);
    const F32 dx = sx - lx;
    const F32 dy = sy - ly;
    const F32 dz = sz - lz;
    const F32 d  = std::sqrt(dx * dx + dy * dy + dz * dz);
    const F32 min_d = mMinDist.load(std::memory_order_relaxed);
    const F32 max_d = mMaxDist.load(std::memory_order_relaxed);

    // §4.3.3 distance gain (linear-square rolloff)
    F32 dist_gain;
    if (d <= min_d)
    {
        dist_gain = 1.f;
    }
    else if (max_d <= min_d || d >= max_d)
    {
        dist_gain = (d >= max_d) ? 0.f : 1.f;
    }
    else
    {
        const F32 t = (d - min_d) / (max_d - min_d);
        dist_gain = 1.f - t * t;
    }

    // Listener basis: at = forward, up = up, right = at × up.
    const F32 ax = mListenerAtX.load(std::memory_order_relaxed);
    const F32 ay = mListenerAtY.load(std::memory_order_relaxed);
    const F32 az = mListenerAtZ.load(std::memory_order_relaxed);
    const F32 ux = mListenerUpX.load(std::memory_order_relaxed);
    const F32 uy = mListenerUpY.load(std::memory_order_relaxed);
    const F32 uz = mListenerUpZ.load(std::memory_order_relaxed);
    F32 rx = ay * uz - az * uy;
    F32 ry = az * ux - ax * uz;
    F32 rz = ax * uy - ay * ux;
    const F32 r_len = std::sqrt(rx * rx + ry * ry + rz * rz);
    if (r_len > 1e-6f)
    {
        const F32 inv = 1.f / r_len;
        rx *= inv; ry *= inv; rz *= inv;
    }

    // Project source-relative vector onto basis.
    const F32 right_proj   = dx * rx + dy * ry + dz * rz;
    const F32 forward_proj = dx * ax + dy * ay + dz * az;
    const F32 up_proj      = dx * ux + dy * uy + dz * uz;
    // Azimuth: 0 = front, +π/2 = right, ±π = back. Source on right → +ITD on L.
    const F32 azimuth   = std::atan2(right_proj, forward_proj);
    // Elevation: 0 = horizontal, +π/2 = above.
    const F32 elev_sin  = (d > 1e-6f) ? std::clamp(up_proj / d, -1.f, 1.f) : 0.f;
    const F32 elevation = std::asin(elev_sin);
    const F32 az_abs    = std::fabs(azimuth);

    // §4.3.5 ITD via Woodworth-Schlosberg (head radius a = 0.0875 m, c = 343 m/s).
    constexpr F32 kHeadRadius  = 0.0875f;
    constexpr F32 kSpeedOfSnd  = 343.f;
    // 1.5-sample baseline keeps the Lagrange 4-tap interp window strictly in
    // the past for both ears (otherwise an ipsilateral delay = 0 would tap
    // mDelayWrite+1 = stale ring data, contaminating the contralateral).
    // Applied equally to L and R, so binaural delta is unchanged.
    constexpr F32 kInterpBaseline = 1.5f;
    const F32 itd_seconds = (kHeadRadius / kSpeedOfSnd) * (std::sin(az_abs) + az_abs);
    F32 itd_samples = itd_seconds * mSampleRate;
    const F32 max_itd = static_cast<F32>(kDelayBufFrames - 5) - kInterpBaseline;
    if (itd_samples > max_itd) itd_samples = max_itd;
    if (itd_samples < 0.f)     itd_samples = 0.f;
    const bool source_on_right = (azimuth >= 0.f);
    const F32 left_delay  = kInterpBaseline + (source_on_right ? itd_samples : 0.f);
    const F32 right_delay = kInterpBaseline + (source_on_right ? 0.f         : itd_samples);

    // §4.3.6 ILD broadband gain. Ipsilateral 1.0, contralateral cos(|az|/2)^0.5.
    const F32 contra_bb = std::sqrt(std::max(0.f, std::cos(az_abs * 0.5f)));
    const F32 left_bb_gain  = source_on_right ? contra_bb : 1.f;
    const F32 right_bb_gain = source_on_right ? 1.f       : contra_bb;

    // §4.3.6 shadow shelf gain (contralateral only): 0 dB → -6 dB by sin(|az|).
    const F32 shadow_db = -6.f * std::sin(az_abs);
    // §4.3.7 elevation HF tilt: ±1 dB outside ±30°, applied to both ears.
    F32 elev_db = 0.f;
    constexpr F32 kElevHi = F_PI / 6.f;  // 30°
    if (elevation >  kElevHi) elev_db = +1.f;
    if (elevation < -kElevHi) elev_db = -1.f;

    const F32 left_shelf_db  = elev_db + (source_on_right ? shadow_db : 0.f);
    const F32 right_shelf_db = elev_db + (source_on_right ? 0.f       : shadow_db);

    // §4.3.4 air absorption shelf: -0.5 dB/m, capped at -25 dB.
    constexpr F32 kAirCap = -25.f;
    F32 air_db = -0.5f * d;
    if (air_db < kAirCap) air_db = kAirCap;

    const BiquadCoeffs air_co  = hiShelfCoeffs(mSampleRate, 4000.f, air_db);
    const BiquadCoeffs shad_lo = hiShelfCoeffs(mSampleRate, 2000.f, left_shelf_db);
    const BiquadCoeffs shad_ro = hiShelfCoeffs(mSampleRate, 2000.f, right_shelf_db);

    if (inchannels <= 0) inchannels = 1;
    const F32 inv_ch = (inchannels > 1) ? (1.f / static_cast<F32>(inchannels)) : 1.f;

    for (unsigned int i = 0; i < length; ++i)
    {
        // Mono-fold (defensive: only mono is expected in the r11 path).
        F32 mono = 0.f;
        for (int c = 0; c < inchannels; ++c)
        {
            mono += in[i * inchannels + c];
        }
        mono *= inv_ch;

        // Push into both ear delay lines.
        mDelayL[mDelayWrite] = mono;
        mDelayR[mDelayWrite] = mono;

        // Lagrange 3rd-order fractional-delay tap per ear.
        auto tapDelay = [this](const F32* buf, F32 delay) -> F32 {
            const int floorD   = static_cast<int>(delay);
            const F32 frac     = delay - static_cast<F32>(floorD);
            // Indices (newest sample is at mDelayWrite). Read backwards.
            const int base     = mDelayWrite - floorD;
            const int idx_xm1  = (base + 1 + kDelayBufFrames) % kDelayBufFrames;
            const int idx_x0   = (base     + kDelayBufFrames) % kDelayBufFrames;
            const int idx_x1   = (base - 1 + kDelayBufFrames) % kDelayBufFrames;
            const int idx_x2   = (base - 2 + kDelayBufFrames) % kDelayBufFrames;
            return lagrange4(buf[idx_xm1], buf[idx_x0], buf[idx_x1], buf[idx_x2], frac);
        };

        F32 lout = tapDelay(mDelayL, left_delay);
        F32 rout = tapDelay(mDelayR, right_delay);

        // Shelf chain: air absorption (knee 4 kHz) → shadow + elev (knee 2 kHz).
        lout = biquadStep(air_co,  lout, mAirZ1L,  mAirZ2L);
        rout = biquadStep(air_co,  rout, mAirZ1R,  mAirZ2R);
        lout = biquadStep(shad_lo, lout, mShadZ1L, mShadZ2L);
        rout = biquadStep(shad_ro, rout, mShadZ1R, mShadZ2R);

        // Broadband ILD + distance gain.
        lout *= left_bb_gain  * dist_gain;
        rout *= right_bb_gain * dist_gain;

        out[i * 2 + 0] = lout;
        out[i * 2 + 1] = rout;

        mDelayWrite = (mDelayWrite + 1) % kDelayBufFrames;
    }
}
