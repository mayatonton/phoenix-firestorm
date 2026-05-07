/**
 * @file llfftconvolver.h
 * @brief AYAstorm r11 partitioned overlap-save FFT convolver (mono kernel).
 *
 * P7a: self-contained radix-2 FFT + uniform partitioned overlap-save engine.
 * Used by LLVenueReverbDsp to convolve the binaural-mixed Stream3D bus with a
 * room IR. Each instance handles ONE channel — venue reverb uses two (L, R)
 * with independent kernels.
 *
 * Block size = FMOD DSP block (typically 1024). FFT size = 2 × block. CPU is
 * O(P · N · log N) per block where P = ceil(IR_len / block) and N = FFT size,
 * so multi-second IRs (cathedral / hall_large) stay flat in cost.
 *
 * Threading: init() runs on the main thread before the DSP is attached.
 * processAdd() / reset() run on the FMOD mixer thread. There is no shared
 * state between threads inside this class — outer code (LLVenueReverbDsp)
 * is responsible for synchronising IR swaps via a ready flag (P7b).
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

#ifndef LL_FFT_CONVOLVER_H
#define LL_FFT_CONVOLVER_H

#include "stdtypes.h"

#include <vector>

class LLPartitionedConvolver
{
public:
    LLPartitionedConvolver();
    ~LLPartitionedConvolver();

    LLPartitionedConvolver(const LLPartitionedConvolver&) = delete;
    LLPartitionedConvolver& operator=(const LLPartitionedConvolver&) = delete;

    // Initialize with a single-channel IR. block_size must be a power of 2
    // and >= 64. ir may be nullptr (treated as unit-impulse of length 1) so
    // P7a smoke testing has a sane default before P7b plumbs file loads.
    // Returns false on bad block_size only — null IR is not an error.
    bool init(int block_size, const F32* ir, int ir_length);

    // Process one block (length must equal block_size from init).
    // Adds wet_gain × (input ⊛ ir) to 'out' (parallel-mix model — caller
    // copies dry in first). wet_gain == 0 short-circuits to a no-op so the
    // skeleton default keeps the bus quiet.
    void processAdd(const F32* in, F32* out, F32 wet_gain);

    // Zero internal ring + overlap. Call when the channel restarts so we
    // don't bleed stale tail into the next stream.
    void reset();

    int blockSize() const { return mBlockSize; }
    int irLength() const { return mIrLength; }

private:
    // ---- FFT primitives (in-place radix-2 Cooley-Tukey, DIT) ----
    void buildTables();
    void fftForward(F32* re, F32* im) const;
    void fftInverse(F32* re, F32* im) const;

    int mBlockSize { 0 };
    int mFftSize   { 0 };   // = 2 × mBlockSize
    int mFftLog2   { 0 };   // log2(mFftSize)
    int mNumPart   { 0 };   // ceil(mIrLength / mBlockSize)
    int mIrLength  { 0 };

    // FFT tables.
    std::vector<F32> mTwRe;       // size mFftSize / 2
    std::vector<F32> mTwIm;       // size mFftSize / 2
    std::vector<int> mBitRev;     // size mFftSize

    // IR partitions in frequency domain. Layout: partition p occupies
    // [p × mFftSize, (p+1) × mFftSize). Both Re and Im arrays the same shape.
    std::vector<F32> mIrFreqRe;
    std::vector<F32> mIrFreqIm;

    // Frequency-domain ring of past input blocks (also one entry per partition).
    std::vector<F32> mInRingRe;
    std::vector<F32> mInRingIm;
    int mRingHead { 0 };          // index of newest entry in [0, mNumPart)

    // Time-domain previous block (for overlap concatenation: prev + current).
    std::vector<F32> mPrevBlock;  // size mBlockSize

    // Working buffers (mFftSize each).
    mutable std::vector<F32> mWorkRe;
    mutable std::vector<F32> mWorkIm;
    mutable std::vector<F32> mAccRe;
    mutable std::vector<F32> mAccIm;
};

#endif // LL_FFT_CONVOLVER_H
