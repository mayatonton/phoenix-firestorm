/**
* @file llvkbucket.h
* @brief AYAstorm VK-native persistent draw bucket (M4)
*
* $LicenseInfo:firstyear=2026&license=viewerlgpl$
* AYAstorm Viewer Source Code
* Copyright (C) 2026 Ishikawa AYA (github: mayatonton, mayatonton1994@gmail.com)
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
* $/LicenseInfo$
*/

#ifndef LL_LLVKBUCKET_H
#define LL_LLVKBUCKET_H

#include "llpointer.h"
#include "llspatialpartition.h"
#include "pipeline.h"

#include <vector>

class LLDrawInfo;
class LLSpatialGroup;
class LLViewerRegion;

namespace LLVKBucket
{
    constexpr U32 INVALID_GROUP_ID = 0xFFFFFFFFu;
    constexpr U32 kBucketizedPassCount = 13;
    extern const U32 kBucketizedPasses[kBucketizedPassCount];

    struct Range
    {
        U32 mGroupId = INVALID_GROUP_ID;
        U32 mIndexCount = 0;
        std::vector<LLPointer<LLDrawInfo> > mRecords;
    };

    struct Bucket
    {
        U32 mPass = 0;
        LLViewerRegion* mRegion = nullptr;
        std::vector<Range> mRanges;
        std::vector<U32> mFreeSlots;
    };

    bool isBucketizedPass(U32 pass);
    bool emitActive(U32 pass);
    U32  visWordCount();

    void patchGroup(LLSpatialGroup* group);
    void evictGroup(LLSpatialGroup* group);
    void onGroupDestroyed(LLSpatialGroup* group);

    const std::vector<Bucket*>& bucketsForPass(U32 pass);
    const std::vector<U64>* currentVisBits();
    void perfRangeEmit(U64 records);
    void perfRangeSkip();

    template <typename FN>
    inline void forEachVisible(U32 pass, FN&& fn)
    {
        const std::vector<U64>* bits = currentVisBits();
        if (bits == nullptr)
        {
            return;
        }
        for (Bucket* bucket : bucketsForPass(pass))
        {
            for (Range& range : bucket->mRanges)
            {
                if (range.mRecords.empty())
                {
                    continue;
                }
                const U32 id = range.mGroupId;
                if ((id >> 6) >= bits->size() || ((*bits)[id >> 6] & (1ULL << (id & 63))) == 0)
                {
                    perfRangeSkip();
                    continue;
                }
                perfRangeEmit(range.mRecords.size());
                for (LLPointer<LLDrawInfo>& info : range.mRecords)
                {
                    fn(*info);
                }
            }
        }
    }

    template <typename FN>
    inline void forEachSource(U32 pass, FN&& fn)
    {
        if (emitActive(pass))
        {
            forEachVisible(pass, fn);
            return;
        }
        auto* begin = gPipeline.beginRenderMap(pass);
        auto* end = gPipeline.endRenderMap(pass);
        for (LLCullResult::drawinfo_iterator i = begin; i != end; )
        {
            LLDrawInfo* info = *i;
            LLCullResult::increment_iterator(i, end);
            fn(*info);
        }
    }
}

#endif
