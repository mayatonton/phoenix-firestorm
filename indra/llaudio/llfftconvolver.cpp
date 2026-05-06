/**
 * @file llfftconvolver.cpp
 * @brief AYAstorm r11 partitioned overlap-save FFT convolver — P7a.
 *
 * Hand-rolled radix-2 FFT (no external dep) + uniform partitioned
 * overlap-save convolution. Sized for a single block size set at init();
 * re-init to change. See header for threading and IR-swap contract.
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

#include "llfftconvolver.h"

#include "llmath.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{
    inline bool isPow2(int x)
    {
        return x > 0 && (x & (x - 1)) == 0;
    }

    inline int log2Int(int x)
    {
        int n = 0;
        while ((1 << n) < x) ++n;
        return n;
    }
}

LLPartitionedConvolver::LLPartitionedConvolver() = default;
LLPartitionedConvolver::~LLPartitionedConvolver() = default;

bool LLPartitionedConvolver::init(int block_size, const F32* ir, int ir_length)
{
    if (!isPow2(block_size) || block_size < 64)
    {
        return false;
    }

    // Default to a unit-impulse if caller passes nothing — keeps the smoke
    // test path (P7a) sensible: wet path == dry input, delayed by one block.
    static const F32 kUnitImpulse = 1.f;
    if (!ir || ir_length <= 0)
    {
        ir = &kUnitImpulse;
        ir_length = 1;
    }

    mBlockSize  = block_size;
    mFftSize    = block_size * 2;
    mFftLog2    = log2Int(mFftSize);
    mIrLength   = ir_length;
    mNumPart    = (ir_length + block_size - 1) / block_size;

    buildTables();

    // Pre-FFT every IR partition.
    mIrFreqRe.assign(mNumPart * mFftSize, 0.f);
    mIrFreqIm.assign(mNumPart * mFftSize, 0.f);
    std::vector<F32> ir_re(mFftSize, 0.f);
    std::vector<F32> ir_im(mFftSize, 0.f);
    for (int p = 0; p < mNumPart; ++p)
    {
        std::fill(ir_re.begin(), ir_re.end(), 0.f);
        std::fill(ir_im.begin(), ir_im.end(), 0.f);
        const int src_off = p * mBlockSize;
        const int copy_n  = std::min(mBlockSize, mIrLength - src_off);
        for (int i = 0; i < copy_n; ++i)
        {
            ir_re[i] = ir[src_off + i];
        }
        // Zero-padded FFT; second half stays zero (overlap-save layout).
        fftForward(ir_re.data(), ir_im.data());
        std::memcpy(&mIrFreqRe[p * mFftSize], ir_re.data(), mFftSize * sizeof(F32));
        std::memcpy(&mIrFreqIm[p * mFftSize], ir_im.data(), mFftSize * sizeof(F32));
    }

    // Per-instance working storage.
    mInRingRe.assign(mNumPart * mFftSize, 0.f);
    mInRingIm.assign(mNumPart * mFftSize, 0.f);
    mRingHead = 0;
    mPrevBlock.assign(mBlockSize, 0.f);
    mWorkRe.assign(mFftSize, 0.f);
    mWorkIm.assign(mFftSize, 0.f);
    mAccRe.assign(mFftSize, 0.f);
    mAccIm.assign(mFftSize, 0.f);

    return true;
}

void LLPartitionedConvolver::reset()
{
    std::fill(mInRingRe.begin(), mInRingRe.end(), 0.f);
    std::fill(mInRingIm.begin(), mInRingIm.end(), 0.f);
    std::fill(mPrevBlock.begin(), mPrevBlock.end(), 0.f);
    mRingHead = 0;
}

void LLPartitionedConvolver::processAdd(const F32* in, F32* out, F32 wet_gain)
{
    if (mBlockSize == 0 || !in || !out) return;
    // Skeleton-default short-circuit: no work, no tail bleed.
    if (wet_gain == 0.f) return;

    // 1. Form 2-block frame (prev || current) into mWorkRe, zero imaginary.
    std::memcpy(mWorkRe.data(),                   mPrevBlock.data(), mBlockSize * sizeof(F32));
    std::memcpy(mWorkRe.data() + mBlockSize,      in,                mBlockSize * sizeof(F32));
    std::memset(mWorkIm.data(), 0, mFftSize * sizeof(F32));

    // 2. FFT in place → store into ring at mRingHead.
    fftForward(mWorkRe.data(), mWorkIm.data());
    F32* ring_re_head = &mInRingRe[mRingHead * mFftSize];
    F32* ring_im_head = &mInRingIm[mRingHead * mFftSize];
    std::memcpy(ring_re_head, mWorkRe.data(), mFftSize * sizeof(F32));
    std::memcpy(ring_im_head, mWorkIm.data(), mFftSize * sizeof(F32));

    // 3. Multiply-accumulate: acc += sum_{p=0..P-1} ring[head-p] ⊙ ir[p].
    std::memset(mAccRe.data(), 0, mFftSize * sizeof(F32));
    std::memset(mAccIm.data(), 0, mFftSize * sizeof(F32));
    for (int p = 0; p < mNumPart; ++p)
    {
        const int ring_idx = (mRingHead - p + mNumPart) % mNumPart;
        const F32* xr = &mInRingRe[ring_idx * mFftSize];
        const F32* xi = &mInRingIm[ring_idx * mFftSize];
        const F32* hr = &mIrFreqRe[p * mFftSize];
        const F32* hi = &mIrFreqIm[p * mFftSize];
        for (int k = 0; k < mFftSize; ++k)
        {
            // Complex MAC: acc += x · h.
            mAccRe[k] += xr[k] * hr[k] - xi[k] * hi[k];
            mAccIm[k] += xr[k] * hi[k] + xi[k] * hr[k];
        }
    }

    // 4. IFFT → take second half (= valid linear convolution under overlap-save).
    fftInverse(mAccRe.data(), mAccIm.data());
    for (int i = 0; i < mBlockSize; ++i)
    {
        out[i] += wet_gain * mAccRe[mBlockSize + i];
    }

    // 5. Slide ring + retain current block as next "prev".
    mRingHead = (mRingHead + 1) % mNumPart;
    std::memcpy(mPrevBlock.data(), in, mBlockSize * sizeof(F32));
}

// ---- FFT primitives ----------------------------------------------------------

void LLPartitionedConvolver::buildTables()
{
    // Bit-reversal permutation indices.
    mBitRev.assign(mFftSize, 0);
    for (int i = 0; i < mFftSize; ++i)
    {
        int x = i, r = 0;
        for (int b = 0; b < mFftLog2; ++b)
        {
            r = (r << 1) | (x & 1);
            x >>= 1;
        }
        mBitRev[i] = r;
    }

    // Twiddle factors W_N^k = exp(-2πi k / N), k in [0, N/2).
    const int half = mFftSize / 2;
    mTwRe.assign(half, 0.f);
    mTwIm.assign(half, 0.f);
    const F32 step = -2.f * F_PI / static_cast<F32>(mFftSize);
    for (int k = 0; k < half; ++k)
    {
        const F32 a = step * static_cast<F32>(k);
        mTwRe[k] = std::cos(a);
        mTwIm[k] = std::sin(a);
    }
}

void LLPartitionedConvolver::fftForward(F32* re, F32* im) const
{
    // Bit-reversal permute.
    for (int i = 0; i < mFftSize; ++i)
    {
        const int j = mBitRev[i];
        if (i < j)
        {
            std::swap(re[i], re[j]);
            std::swap(im[i], im[j]);
        }
    }
    // Iterative Cooley-Tukey radix-2 DIT.
    for (int s = 1; s <= mFftLog2; ++s)
    {
        const int m       = 1 << s;            // butterfly span
        const int m_half  = m >> 1;            // half span
        const int tw_step = mFftSize / m;      // stride into twiddle table
        for (int k = 0; k < mFftSize; k += m)
        {
            for (int j = 0; j < m_half; ++j)
            {
                const int tw_idx = j * tw_step;
                const F32 wr = mTwRe[tw_idx];
                const F32 wi = mTwIm[tw_idx];
                const int idx_a = k + j;
                const int idx_b = k + j + m_half;
                const F32 t_re = wr * re[idx_b] - wi * im[idx_b];
                const F32 t_im = wr * im[idx_b] + wi * re[idx_b];
                re[idx_b] = re[idx_a] - t_re;
                im[idx_b] = im[idx_a] - t_im;
                re[idx_a] = re[idx_a] + t_re;
                im[idx_a] = im[idx_a] + t_im;
            }
        }
    }
}

void LLPartitionedConvolver::fftInverse(F32* re, F32* im) const
{
    // IFFT(x) = (1/N) · conj(FFT(conj(x))).
    for (int i = 0; i < mFftSize; ++i) im[i] = -im[i];
    fftForward(re, im);
    const F32 inv_n = 1.f / static_cast<F32>(mFftSize);
    for (int i = 0; i < mFftSize; ++i)
    {
        re[i] *=  inv_n;
        im[i] *= -inv_n;
    }
}
