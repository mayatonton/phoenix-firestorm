/**
 * @file llassetubopool.cpp
 * @brief LLAssetUboPool implementation (r41 Phase 1.C PC-3、(W2) 持越項目)
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
#include "llassetubopool.h"

LLAssetUboPool::LLAssetUboPool(PoolFactory   factory,
                               PoolDestroyer destroyer,
                               std::uint32_t prealloc_count,
                               std::uint32_t grow_chunk)
:   mFactory(std::move(factory))
,   mDestroyer(std::move(destroyer))
,   mPreallocCount(prealloc_count == 0 ? kPreallocAssetCount : prealloc_count)
,   mGrowChunkCount(grow_chunk == 0 ? kGrowChunkAssetCount : grow_chunk)
{
}

LLAssetUboPool::~LLAssetUboPool()
{
    shutdown();
}

bool LLAssetUboPool::initialize()
{
    if (mInitialized)
    {
        return true;
    }
    if (!growOnce(mPreallocCount))
    {
        return false;
    }
    mInitialized = true;
    return true;
}

void LLAssetUboPool::shutdown()
{
    if (mDestroyer)
    {
        for (auto it = mPools.rbegin(); it != mPools.rend(); ++it)
        {
            mDestroyer(*it);
        }
    }
    mPools.clear();
    mPoolCapacities.clear();
    mUsedPerPool.clear();
    mInitialized = false;
}

bool LLAssetUboPool::growOnce(std::uint32_t capacity)
{
    if (!mFactory)
    {
        return false;
    }
    PoolHandle handle = mFactory();
    if (handle == 0)
    {
        return false;
    }
    mPools.push_back(handle);
    mPoolCapacities.push_back(capacity);
    mUsedPerPool.push_back(0);
    return true;
}

LLAssetUboPool::AcquireResult LLAssetUboPool::acquire()
{
    AcquireResult result;
    if (!mInitialized)
    {
        return result;
    }
    for (std::size_t i = 0; i < mPools.size(); ++i)
    {
        if (mUsedPerPool[i] < mPoolCapacities[i])
        {
            result.pool         = mPools[i];
            result.slot_in_pool = mUsedPerPool[i];
            result.success      = true;
            ++mUsedPerPool[i];
            return result;
        }
    }
    if (!growOnce(mGrowChunkCount))
    {
        return result;
    }
    std::size_t last    = mPools.size() - 1;
    result.pool         = mPools[last];
    result.slot_in_pool = 0;
    result.success      = true;
    mUsedPerPool[last]  = 1;
    return result;
}

void LLAssetUboPool::resetAllSlots()
{
    for (auto& u : mUsedPerPool)
    {
        u = 0;
    }
}

std::uint32_t LLAssetUboPool::getTotalAssetCapacity() const noexcept
{
    std::uint32_t total = 0;
    for (auto c : mPoolCapacities)
    {
        total += c;
    }
    return total;
}

std::uint32_t LLAssetUboPool::getUsedAssetCount() const noexcept
{
    std::uint32_t total = 0;
    for (auto u : mUsedPerPool)
    {
        total += u;
    }
    return total;
}
