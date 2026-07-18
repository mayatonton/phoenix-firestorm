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
#include "llimagegl.h"
#include "llpipelineframecontext.h"
#include "llviewerregion.h"
#include "llviewertexture.h"
#include "llvkloader.h"

#include <algorithm>
#include <array>
#include <memory>
#include <unordered_map>

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

bool isCameraMdiPass(U32 pass)
{
    return pass == LLRenderPass::PASS_SIMPLE
        || pass == LLRenderPass::PASS_FULLBRIGHT;
}

bool emitActive(U32 pass)
{
    return isBucketizedPass(pass)
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
    for (const std::pair<Bucket*, U32>& slot : group->mVkBucketSlots)
    {
        slot.first->mTplDirty = true;
    }
    ++LLVKLoader::gVkPerf.bkt_patch;
}

void evictGroup(LLSpatialGroup* group)
{
    for (const std::pair<Bucket*, U32>& slot : group->mVkBucketSlots)
    {
        Range& range = slot.first->mRanges[slot.second];
        range.mRecords.clear();
        range.mIndexCount = 0;
        slot.first->mTplDirty = true;
    }
    group->mVkBucketIndexCount = 0;
}

namespace
{
    void reclaimDeadBuckets()
    {
        auto& all = allBuckets();
        for (size_t i = 0; i < all.size(); )
        {
            Bucket* bucket = all[i].get();
            if (bucket->mRegion == nullptr
                && bucket->mFreeSlots.size() == bucket->mRanges.size())
            {
                std::vector<Bucket*>& vec = byPass()[bucket->mPass];
                vec.erase(std::remove(vec.begin(), vec.end(), bucket), vec.end());
                all[i] = std::move(all.back());
                all.pop_back();
            }
            else
            {
                ++i;
            }
        }
    }
}

void onGroupDestroyed(LLSpatialGroup* group)
{
    bool dead_region = false;
    for (const std::pair<Bucket*, U32>& slot : group->mVkBucketSlots)
    {
        Range& range = slot.first->mRanges[slot.second];
        range.mRecords.clear();
        range.mIndexCount = 0;
        range.mGroupId = INVALID_GROUP_ID;
        slot.first->mFreeSlots.push_back(slot.second);
        slot.first->mTplDirty = true;
        dead_region |= (slot.first->mRegion == nullptr);
    }
    group->mVkBucketSlots.clear();

    if (group->mVkBucketGroupId != INVALID_GROUP_ID)
    {
        sFreeGroupIds.push_back(group->mVkBucketGroupId);
        group->mVkBucketGroupId = INVALID_GROUP_ID;
    }
    group->mVkBucketIndexCount = 0;

    if (dead_region)
    {
        reclaimDeadBuckets();
    }
}

void onRegionDestroyed(LLViewerRegion* region)
{
    for (const std::unique_ptr<Bucket>& up : allBuckets())
    {
        Bucket* bucket = up.get();
        if (bucket->mRegion == region)
        {
            bucket->mRegion   = nullptr;
            bucket->mTplDirty = true;
            bucket->mTplCommands.clear();
            bucket->mTplGroupIds.clear();
            bucket->mTplRadius.clear();
            bucket->mTplRecords.clear();
            bucket->mTplDyn.clear();
            bucket->mTplDynGroupIds.clear();
            bucket->mTplChunkSpans.clear();
        }
    }
    reclaimDeadBuckets();
}

namespace
{
    U32 heapSlotFor(LLTexture* t)
    {
        return LLImageGL::vkHeapSlotOrDefault(t ? t->getGLTexture() : nullptr);
    }

    void computeRecordSlots(LLDrawInfo* info, U32* slots)
    {
        slots[0] = slots[1] = slots[2] = slots[3] = 0;
        if (info->mTextureList.size() > 1)
        {
            const U32 n = llmin((U32)info->mTextureList.size(), 4u);
            for (U32 i = 0; i < n; ++i)
            {
                slots[i] = heapSlotFor(info->mTextureList[i].get());
            }
        }
        else if (info->mTexture.notNull())
        {
            slots[0] = heapSlotFor(info->mTexture.get());
        }
        else
        {
            slots[0] = heapSlotFor(nullptr);
        }
    }

    bool ensureRecordDrawDataSlot(LLDrawInfo* info)
    {
        U32 slots[4];
        computeRecordSlots(info, slots);
        return info->ensureVkDrawDataSlot(slots);
    }

}

void rebuildTemplateIfDirty(Bucket& bucket)
{
    if (!bucket.mTplDirty)
    {
        return;
    }
    bucket.mTplDirty = false;
    bucket.mTplCommands.clear();
    bucket.mTplGroupIds.clear();
    bucket.mTplRadius.clear();
    bucket.mTplRecords.clear();
    bucket.mTplDyn.clear();
    bucket.mTplDynGroupIds.clear();
    bucket.mTplChunkSpans.clear();

    const LLMatrix4* region_matrix = bucket.mRegion ? &bucket.mRegion->mRenderMatrix : nullptr;
    const bool camera_mdi = isCameraMdiPass(bucket.mPass);

    struct StaticEntry
    {
        LLVertexBuffer* mVB;
        LLDrawInfo*     mInfo;
        U32             mGroupId;
    };
    static std::vector<StaticEntry> s_statics;
    s_statics.clear();

    for (Range& range : bucket.mRanges)
    {
        for (LLPointer<LLDrawInfo>& ptr : range.mRecords)
        {
            LLDrawInfo* info = ptr.get();
            LLVertexBuffer* vb = info->mVertexBuffer.get();
            bool is_static = vb != nullptr
                && info->mCount > 0
                && region_matrix != nullptr
                && info->mModelMatrix == region_matrix
                && info->mTextureMatrix == nullptr
                && info->mAvatar.isNull()
                && vb->getVkVertexSlice().buffer != VK_NULL_HANDLE
                && vb->getVkIndexSlice().buffer != VK_NULL_HANDLE;
            if (is_static && camera_mdi)
            {
                is_static = ensureRecordDrawDataSlot(info);
            }
            if (is_static)
            {
                s_statics.push_back({ vb, info, range.mGroupId });
            }
            else
            {
                info->mVkTplBucket = nullptr;
                bucket.mTplDyn.push_back(info);
                bucket.mTplDynGroupIds.push_back(range.mGroupId);
            }
        }
    }

    std::stable_sort(s_statics.begin(), s_statics.end(),
        [](const StaticEntry& a, const StaticEntry& b)
        {
            const uintptr_t avb = (uintptr_t)a.mVB->getVkVertexSlice().buffer;
            const uintptr_t bvb = (uintptr_t)b.mVB->getVkVertexSlice().buffer;
            if (avb != bvb) return avb < bvb;
            const uintptr_t aib = (uintptr_t)a.mVB->getVkIndexSlice().buffer;
            const uintptr_t bib = (uintptr_t)b.mVB->getVkIndexSlice().buffer;
            if (aib != bib) return aib < bib;
            return a.mVB->getIndicesType() < b.mVB->getIndicesType();
        });

    for (const StaticEntry& entry : s_statics)
    {
        LLVertexBuffer* vb = entry.mVB;
        const LLVKLoader::MegaSliceV& vs = vb->getVkVertexSlice();
        const LLVKLoader::MegaSliceI& is = vb->getVkIndexSlice();

        TplChunkSpan* chunk = bucket.mTplChunkSpans.empty()
                                  ? nullptr
                                  : &bucket.mTplChunkSpans.back();
        LLVertexBuffer* rep_vb = chunk ? chunk->mRep->mVertexBuffer.get() : nullptr;
        if (rep_vb == nullptr
            || rep_vb->getVkVertexSlice().buffer != vs.buffer
            || rep_vb->getVkIndexSlice().buffer != is.buffer
            || rep_vb->getIndicesType() != vb->getIndicesType())
        {
            TplChunkSpan fresh;
            fresh.mFirst = (U32)bucket.mTplCommands.size();
            fresh.mCount = 0;
            fresh.mRep   = entry.mInfo;
            bucket.mTplChunkSpans.push_back(fresh);
            chunk = &bucket.mTplChunkSpans.back();
        }
        ++chunk->mCount;

        LLDrawInfo* info = entry.mInfo;
        info->mVkTplBucket   = &bucket;
        info->mVkTplCmdIndex = (U32)bucket.mTplCommands.size();
        VkDrawIndexedIndirectCommand cmd;
        cmd.indexCount    = info->mCount;
        cmd.instanceCount = 1;
        cmd.firstIndex    = is.offset / vb->getIndicesStride() + info->mOffset;
        cmd.vertexOffset  = (S32)vs.first;
        cmd.firstInstance = (info->mVkDrawDataSlot != 0xFFFFFFFFu)
                                ? info->mVkDrawDataSlot
                                : 0;
        bucket.mTplCommands.push_back(cmd);
        bucket.mTplGroupIds.push_back(entry.mGroupId);
        bucket.mTplRadius.push_back(info->mBoundRadius);
        bucket.mTplRecords.push_back(info);
    }
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
