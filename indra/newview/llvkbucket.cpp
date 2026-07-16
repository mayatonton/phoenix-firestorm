/**
* @file llvkbucket.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llvkbucket.h"

#include "lldrawpool.h"
#include "llpipelineframecontext.h"
#include "llvkloader.h"

#include <array>
#include <memory>

namespace LLVKBucket
{

const U32 kBucketizedPasses[kBucketizedPassCount] = {
    LLRenderPass::PASS_SIMPLE,
    LLRenderPass::PASS_FULLBRIGHT,
    LLRenderPass::PASS_SHINY,
    LLRenderPass::PASS_BUMP,
    LLRenderPass::PASS_FULLBRIGHT_SHINY,
    LLRenderPass::PASS_MATERIAL,
    LLRenderPass::PASS_MATERIAL_ALPHA_EMISSIVE,
    LLRenderPass::PASS_SPECMAP,
    LLRenderPass::PASS_SPECMAP_EMISSIVE,
    LLRenderPass::PASS_NORMMAP,
    LLRenderPass::PASS_NORMMAP_EMISSIVE,
    LLRenderPass::PASS_NORMSPEC,
    LLRenderPass::PASS_NORMSPEC_EMISSIVE,
};

namespace
{
    std::vector<std::unique_ptr<Bucket> >& allBuckets()
    {
        static std::vector<std::unique_ptr<Bucket> > s;
        return s;
    }

    std::vector<Bucket*>* byPass()
    {
        static std::vector<Bucket*> s[LLRenderPass::NUM_RENDER_TYPES];
        return s;
    }

    const std::vector<Bucket*>& noBuckets()
    {
        static const std::vector<Bucket*> s;
        return s;
    }

    U32 sNextGroupId = 0;
    std::vector<U32> sFreeGroupIds;

    U32 allocGroupId()
    {
        if (!sFreeGroupIds.empty())
        {
            U32 id = sFreeGroupIds.back();
            sFreeGroupIds.pop_back();
            return id;
        }
        return sNextGroupId++;
    }

    Range* findRange(LLSpatialGroup* group, U32 pass)
    {
        for (const std::pair<Bucket*, U32>& slot : group->mVkBucketSlots)
        {
            if (slot.first->mPass == pass)
            {
                return &slot.first->mRanges[slot.second];
            }
        }
        return nullptr;
    }

    Range* allocRange(LLSpatialGroup* group, U32 pass, LLViewerRegion* region)
    {
        Bucket* bucket = nullptr;
        for (Bucket* candidate : byPass()[pass])
        {
            if (candidate->mRegion == region)
            {
                bucket = candidate;
                break;
            }
        }
        if (bucket == nullptr)
        {
            allBuckets().emplace_back(new Bucket());
            bucket = allBuckets().back().get();
            bucket->mPass = pass;
            bucket->mRegion = region;
            byPass()[pass].push_back(bucket);
        }

        U32 idx;
        if (!bucket->mFreeSlots.empty())
        {
            idx = bucket->mFreeSlots.back();
            bucket->mFreeSlots.pop_back();
        }
        else
        {
            idx = (U32)bucket->mRanges.size();
            bucket->mRanges.emplace_back();
        }

        if (group->mVkBucketGroupId == INVALID_GROUP_ID)
        {
            group->mVkBucketGroupId = allocGroupId();
        }

        Range& range = bucket->mRanges[idx];
        range.mGroupId = group->mVkBucketGroupId;
        range.mIndexCount = 0;
        range.mRecords.clear();

        group->mVkBucketSlots.emplace_back(bucket, idx);
        return &range;
    }
}

bool enabled()
{
    static const bool s_enabled = []() -> bool {
        const char* e = getenv("AYASTORM_BUCKETS");
        return !(e != nullptr && e[0] == '0');
    }();
    return s_enabled;
}

bool isBucketizedPass(U32 pass)
{
    static const std::array<bool, LLRenderPass::NUM_RENDER_TYPES> s_lut = []() {
        std::array<bool, LLRenderPass::NUM_RENDER_TYPES> lut = {};
        for (U32 p : kBucketizedPasses)
        {
            lut[p] = true;
        }
        return lut;
    }();
    return pass < LLRenderPass::NUM_RENDER_TYPES && s_lut[pass];
}

bool emitActive(U32 pass)
{
    return enabled()
        && isBucketizedPass(pass)
        && !LLPipelineFrameContext::getInstance().isShadowPass()
        && gPipeline.hasRenderType(pass);
}

U32 visWordCount()
{
    return (sNextGroupId + 63) >> 6;
}

void patchGroup(LLSpatialGroup* group)
{
    LLViewerRegion* region = group->getSpatialPartition() ? group->getSpatialPartition()->mRegionp : nullptr;

    U32 total_index_count = 0;
    for (U32 pass : kBucketizedPasses)
    {
        LLSpatialGroup::draw_map_t::iterator it = group->mDrawMap.find(pass);
        const bool has_records = (it != group->mDrawMap.end()) && !it->second.empty();

        Range* range = findRange(group, pass);
        if (!has_records)
        {
            if (range != nullptr)
            {
                range->mRecords.clear();
                range->mIndexCount = 0;
            }
            continue;
        }

        if (range == nullptr)
        {
            range = allocRange(group, pass, region);
        }

        range->mRecords.assign(it->second.begin(), it->second.end());
        U32 index_count = 0;
        for (const LLPointer<LLDrawInfo>& info : range->mRecords)
        {
            index_count += info->mCount;
        }
        range->mIndexCount = index_count;
        total_index_count += index_count;
    }

    group->mVkBucketIndexCount = total_index_count;
    ++LLVKLoader::gVkPerf.bkt_patch;
}

void evictGroup(LLSpatialGroup* group)
{
    for (const std::pair<Bucket*, U32>& slot : group->mVkBucketSlots)
    {
        Range& range = slot.first->mRanges[slot.second];
        range.mRecords.clear();
        range.mIndexCount = 0;
    }
    group->mVkBucketIndexCount = 0;
}

void onGroupDestroyed(LLSpatialGroup* group)
{
    for (const std::pair<Bucket*, U32>& slot : group->mVkBucketSlots)
    {
        Range& range = slot.first->mRanges[slot.second];
        range.mRecords.clear();
        range.mIndexCount = 0;
        range.mGroupId = INVALID_GROUP_ID;
        slot.first->mFreeSlots.push_back(slot.second);
    }
    group->mVkBucketSlots.clear();

    if (group->mVkBucketGroupId != INVALID_GROUP_ID)
    {
        sFreeGroupIds.push_back(group->mVkBucketGroupId);
        group->mVkBucketGroupId = INVALID_GROUP_ID;
    }
    group->mVkBucketIndexCount = 0;
}

const std::vector<Bucket*>& bucketsForPass(U32 pass)
{
    if (pass >= LLRenderPass::NUM_RENDER_TYPES)
    {
        return noBuckets();
    }
    return byPass()[pass];
}

const std::vector<U64>* currentVisBits()
{
    LLCullResult* cull = LLPipelineFrameContext::getInstance().getCullResult();
    return cull != nullptr ? &cull->bucketVisBits() : nullptr;
}

void perfRangeEmit(U64 records)
{
    ++LLVKLoader::gVkPerf.bkt_range;
    LLVKLoader::gVkPerf.bkt_rec += records;
}

void perfRangeSkip()
{
    ++LLVKLoader::gVkPerf.bkt_skip;
}

}
