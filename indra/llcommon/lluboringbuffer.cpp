/**
 * @file lluboringbuffer.cpp
 * @brief LLUboRingBuffer implementation
 *        (r41 Phase 1.C PC-4、(RB) 持越項目 = 旧 (R1) rename)
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * AYAstorm Viewer Source Code
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "lluboringbuffer.h"

namespace
{
    constexpr std::uint32_t kBytesPerMB = 1024u * 1024u;

    inline std::uint32_t align_up(std::uint32_t v, std::uint32_t a)
    {
        if (a == 0)
        {
            return v;
        }
        return ((v + a - 1u) / a) * a;
    }
}

LLUboRingBuffer::LLUboRingBuffer(BufferAllocator allocator,
                                 BufferDestroyer destroyer,
                                 std::uint32_t initial_size_mb,
                                 std::uint32_t max_size_mb,
                                 std::uint32_t alignment)
:   mAllocator(std::move(allocator))
,   mDestroyer(std::move(destroyer))
,   mInitialSizeMB(initial_size_mb == 0 ? kInitialSizeMB : initial_size_mb)
,   mMaxSizeMB(max_size_mb == 0 ? kMaxSizeMB : max_size_mb)
,   mAlignment(alignment == 0 ? kDefaultAlignment : alignment)
{
    if (mInitialSizeMB > mMaxSizeMB)
    {
        mInitialSizeMB = mMaxSizeMB;
    }
}

LLUboRingBuffer::~LLUboRingBuffer()
{
    shutdown();
}

bool LLUboRingBuffer::initialize()
{
    if (mInitialized)
    {
        return true;
    }
    BufferHandle h = invokeAllocator(mInitialSizeMB * kBytesPerMB);
    if (h == 0)
    {
        return false;
    }
    mBuffer                  = h;
    mCurrentSizeBytes        = mInitialSizeMB * kBytesPerMB;
    mFrameIndex              = 0;
    mActiveChunk             = 0;
    mChunkBytesUsed          = 0;
    mChunksConsumedThisFrame = 1;
    mInitialized             = true;
    return true;
}

void LLUboRingBuffer::shutdown()
{
    destroyCurrentBuffer();
    mCurrentSizeBytes        = 0;
    mFrameIndex              = 0;
    mActiveChunk             = 0;
    mChunkBytesUsed          = 0;
    mChunksConsumedThisFrame = 1;
    mInitialized             = false;
}

LLUboRingBuffer::AllocateResult LLUboRingBuffer::allocate(std::uint32_t size_bytes)
{
    AllocateResult r;
    if (!mInitialized || size_bytes == 0)
    {
        return r;
    }

    const std::uint32_t aligned    = align_up(size_bytes, mAlignment);
    std::uint32_t       chunk_size = mCurrentSizeBytes / kFramesInFlight;

    // 1 alloc が 1 chunk より大きい場合は ring buffer 全体を grow して再評価
    // (= design 07 §7.5 「chunk 内枯渇 = ring buffer grow」の特殊 case)
    if (aligned > chunk_size)
    {
        if (!tryGrow())
        {
            return r;
        }
        r.grew     = true;
        chunk_size = mCurrentSizeBytes / kFramesInFlight;
        if (aligned > chunk_size)
        {
            return r;
        }
        // grow 直後は新 buffer ゆえ frame 状態 reset
        mActiveChunk             = mFrameIndex % kFramesInFlight;
        mChunkBytesUsed          = 0;
        mChunksConsumedThisFrame = 1;
    }

    // chunk overflow 検知時 = next chunk へ wrap or grow
    if (mChunkBytesUsed + aligned > chunk_size)
    {
        // chunks_used == kFramesInFlight - 1 までは safe wrap (= N-2 GPU 完了済)
        // それを超えると N-1 (= GPU 読込中) に当たるため grow 必須
        if (mChunksConsumedThisFrame + 1 >= kFramesInFlight)
        {
            if (!tryGrow())
            {
                return r;
            }
            r.grew     = true;
            chunk_size = mCurrentSizeBytes / kFramesInFlight;
            mActiveChunk             = mFrameIndex % kFramesInFlight;
            mChunkBytesUsed          = 0;
            mChunksConsumedThisFrame = 1;
            if (mChunkBytesUsed + aligned > chunk_size)
            {
                return r;
            }
        }
        else
        {
            mActiveChunk    = (mActiveChunk + 1) % kFramesInFlight;
            mChunkBytesUsed = 0;
            ++mChunksConsumedThisFrame;
        }
    }

    r.buffer  = mBuffer;
    r.offset  = mActiveChunk * chunk_size + mChunkBytesUsed;
    r.size    = aligned;
    r.success = true;
    mChunkBytesUsed += aligned;
    return r;
}

void LLUboRingBuffer::beginFrame()
{
    if (!mInitialized)
    {
        return;
    }
    ++mFrameIndex;
    mActiveChunk             = mFrameIndex % kFramesInFlight;
    mChunkBytesUsed          = 0;
    mChunksConsumedThisFrame = 1;
}

bool LLUboRingBuffer::tryGrow()
{
    if (mCurrentSizeBytes >= mMaxSizeMB * kBytesPerMB)
    {
        return false;
    }
    std::uint32_t cur_mb     = mCurrentSizeBytes / kBytesPerMB;
    std::uint32_t new_size_mb = cur_mb * 2u;
    if (new_size_mb <= cur_mb)
    {
        new_size_mb = cur_mb + 1u;
    }
    if (new_size_mb > mMaxSizeMB)
    {
        new_size_mb = mMaxSizeMB;
    }
    BufferHandle h = invokeAllocator(new_size_mb * kBytesPerMB);
    if (h == 0)
    {
        return false;
    }
    destroyCurrentBuffer();
    mBuffer           = h;
    mCurrentSizeBytes = new_size_mb * kBytesPerMB;
    return true;
}

LLUboRingBuffer::BufferHandle LLUboRingBuffer::invokeAllocator(std::uint32_t size_bytes)
{
    if (!mAllocator)
    {
        return 0;
    }
    return mAllocator(size_bytes);
}

void LLUboRingBuffer::destroyCurrentBuffer()
{
    if (mBuffer != 0 && mDestroyer)
    {
        mDestroyer(mBuffer);
    }
    mBuffer = 0;
}

std::uint32_t LLUboRingBuffer::getCurrentSizeMB() const noexcept
{
    return mCurrentSizeBytes / kBytesPerMB;
}

bool LLUboRingBuffer::isAtMaxSize() const noexcept
{
    return mCurrentSizeBytes >= mMaxSizeMB * kBytesPerMB;
}
