/**
 * @file llpipelinecachestorage.cpp
 * @brief LLPipelineCacheStorage implementation
 *        (r41 Phase 1.C PC-5、(PSC) 持越項目)
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
#include "llpipelinecachestorage.h"

namespace
{
    constexpr std::size_t kBytesPerMB = 1024u * 1024u;
}

LLPipelineCacheStorage::LLPipelineCacheStorage(FileReader    reader,
                                               FileWriter    writer,
                                               std::string   file_path,
                                               std::uint32_t max_size_mb)
:   mReader(std::move(reader))
,   mWriter(std::move(writer))
,   mFilePath(std::move(file_path))
,   mMaxSizeMB(max_size_mb == 0 ? kDefaultMaxSizeMB : max_size_mb)
{
}

LLPipelineCacheStorage::~LLPipelineCacheStorage()
{
    shutdown();
}

bool LLPipelineCacheStorage::initialize()
{
    if (mInitialized)
    {
        return true;
    }
    if (!mReader)
    {
        return false;
    }

    CacheBlob loaded;
    bool      ok = mReader(mFilePath, loaded);

    // file 不在 / read 失敗は許容 = 起動初回想定で empty blob で start
    if (ok && !loaded.empty())
    {
        // (e1) load 時 cap enforcement = 64 MB 超過なら破棄
        if (loaded.size() > getMaxSizeBytes())
        {
            loaded.clear();
            loaded.shrink_to_fit();
        }
        mCacheBlob = std::move(loaded);
    }

    mInitialized = true;
    return true;
}

void LLPipelineCacheStorage::shutdown()
{
    mCacheBlob.clear();
    mCacheBlob.shrink_to_fit();
    mInitialized = false;
}

bool LLPipelineCacheStorage::updateBlob(CacheBlob new_blob)
{
    mCacheBlob = std::move(new_blob);
    return true;
}

bool LLPipelineCacheStorage::persistToDisk()
{
    if (!mInitialized || !mWriter)
    {
        return false;
    }
    // (e1) persist 時 size > max なら writer 不呼出 + false return
    // = caller (PC-6 wire up) が skip / delete / truncate を判断
    if (mCacheBlob.size() > getMaxSizeBytes())
    {
        return false;
    }
    return mWriter(mFilePath, mCacheBlob);
}

std::uint32_t LLPipelineCacheStorage::getBlobSizeMB() const noexcept
{
    return static_cast<std::uint32_t>(mCacheBlob.size() / kBytesPerMB);
}

std::size_t LLPipelineCacheStorage::getMaxSizeBytes() const noexcept
{
    return static_cast<std::size_t>(mMaxSizeMB) * kBytesPerMB;
}

bool LLPipelineCacheStorage::isWithinLimit() const noexcept
{
    return mCacheBlob.size() <= getMaxSizeBytes();
}
