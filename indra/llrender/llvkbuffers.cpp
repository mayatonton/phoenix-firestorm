/**
* @file llvkbuffers.cpp
* @brief AYAstorm r42 Vulkan loader — buffer create / mega-buffer / vertex staging / indirect ring / alloc domains
*
* $LicenseInfo:firstyear=2026&license=viewerlgpl$
* AYAstorm Viewer Source Code
* Copyright (C) 2025-2026 Ishikawa AYA (github: mayatonton, mayatonton1994@gmail.com)
*
* このファイルは Ishikawa AYA が新規に作成した独自著作物である。
* 著作権は Ishikawa AYA が保持し、パブリックドメインには置かない。
* All rights reserved by the author except as licensed below.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
* $/LicenseInfo$
*
* 光の国のひとたちと共にわたしはここにいる　彩
*/
#include "linden_common.h"
#include "llvkloader.h"
#include "llvkcontract.h"

#include "volk.h"
#include "lldir.h"
#include "llcontrol.h"
#include "llwindow.h"
#include "llimagegl.h"
#include "llglslshader.h"
#include "llrendertarget.h"
#include "llthread.h"

#include <vector>
#include <string>
#include <climits>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <chrono>
#include <mutex>
#include <set>
#include <queue>
#include <algorithm>
#include <condition_variable>
#include <deque>
#include <thread>
#include <pthread.h>
#if LL_LINUX
#include <cstdio>
#include <sys/resource.h>
#endif

extern LLControlGroup gSavedSettings;

extern S32 gGLViewport[4];

#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-variable"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#  pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "vk_mem_alloc.h"

extern bool gCubeSnapshot;
extern bool gHeroProbeMirrorRender;
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#endif
#include "llvkloaderinternal.h"

namespace LLVKLoader
{

namespace
{

    constexpr U32            INDIRECT_RING_COMMANDS_PER_FRAME        = 524288;
} // namespace

namespace LLVKLoaderInternal
{

    bool createBufferVkImpl(U32                 size_bytes,
                            VkBufferUsageFlags  usage,
                            VkBuffer&           out_buffer,
                            void*&              out_allocation,
                            void**              out_mapped,
                            bool                prefer_device)
    {
        out_buffer     = VK_NULL_HANDLE;
        out_allocation = nullptr;
        if (out_mapped)
        {
            *out_mapped = nullptr;
        }

        if (size_bytes == 0)
        {
            return false;
        }
        if (sAllocator == VK_NULL_HANDLE)
        {
            return false;
        }

        VkBufferCreateInfo bci = {};
        bci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bci.size        = size_bytes;
        bci.usage       = usage;
        bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo aci = {};
        aci.usage         = prefer_device ? VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
                                          : VMA_MEMORY_USAGE_AUTO;
        aci.flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                          | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        aci.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        VkBuffer       buffer     = VK_NULL_HANDLE;
        VmaAllocation  allocation = VK_NULL_HANDLE;
        VmaAllocationInfo info    = {};
        VkResult r = vmaCreateBuffer(sAllocator, &bci, &aci, &buffer, &allocation, &info);
        if (r != VK_SUCCESS)
        {
            if (buffer != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(sAllocator, buffer, allocation);
            }
            LL_WARNS("Vulkan") << "createBufferVkImpl failed size=" << size_bytes
                               << " usage=0x" << std::hex << (U32)usage << std::dec
                               << " prefer_device=" << (prefer_device ? 1 : 0)
                               << " result=" << (S32)r << LL_ENDL;
            return false;
        }

        if (prefer_device)
        {
            static bool s_logged_prefer_device_memtype = false;
            if (!s_logged_prefer_device_memtype)
            {
                s_logged_prefer_device_memtype = true;
                VkMemoryPropertyFlags mem_flags = 0;
                vmaGetAllocationMemoryProperties(sAllocator, allocation, &mem_flags);
                LL_INFOS("Vulkan") << "first prefer_device buffer: size=" << size_bytes
                                   << " memflags=0x" << std::hex << (U32)mem_flags << std::dec
                                   << " device_local=" << ((mem_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ? 1 : 0)
                                   << LL_ENDL;
            }
        }

        if (out_mapped && info.pMappedData == nullptr)
        {
            LL_WARNS("Vulkan") << "createBufferVkImpl mapped=null size=" << size_bytes
                               << " usage=0x" << std::hex << (U32)usage << std::dec
                               << " prefer_device=" << (prefer_device ? 1 : 0) << LL_ENDL;
        }

        {
            VkMemoryPropertyFlags mem_props = 0;
            vmaGetAllocationMemoryProperties(sAllocator, allocation, &mem_props);
            if ((mem_props & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            {
                LLVKContract::cause(LLVKContract::C_ALLOC_NONCOHERENT);
                LL_WARNS("Vulkan") << "createBufferVkImpl noncoherent mapped allocation size=" << size_bytes
                                   << " usage=0x" << std::hex << (U32)usage
                                   << " memflags=0x" << (U32)mem_props << std::dec
                                   << " prefer_device=" << (prefer_device ? 1 : 0) << LL_ENDL;
            }
        }

        out_buffer     = buffer;
        out_allocation = reinterpret_cast<void*>(allocation);
        if (out_mapped)
        {
            *out_mapped = info.pMappedData;
        }
        noteBufferHandleCreated(buffer);
        return true;
    }
} // namespace LLVKLoaderInternal


void destroyBufferVk(VkBuffer buffer, void* allocation)
{
    noteDeferredEnqueueThread();
    if (buffer == VK_NULL_HANDLE && allocation == nullptr)
    {
        return;
    }
    if (sAllocator == VK_NULL_HANDLE)
    {
        return;
    }
    PendingBufferFree pending;
    pending.buffer        = buffer;
    pending.allocation    = reinterpret_cast<VmaAllocation>(allocation);
    pending.enqueue_frame = sMonotonicFrameCount;
    sPendingBufferFrees.push_back(pending);
}

void tickDeferredBufferFreeQueue()
{
    if (sAllocator == VK_NULL_HANDLE)
    {
        return;
    }
    std::unordered_set<U64> dead_bufs;
    size_t w = 0;
    const size_t n = sPendingBufferFrees.size();
    for (size_t r = 0; r < n; ++r)
    {
        PendingBufferFree& e = sPendingBufferFrees[r];
        if (reapReady(e.enqueue_frame))
        {
            vmaDestroyBuffer(sAllocator, e.buffer, e.allocation);
            if (e.buffer != VK_NULL_HANDLE)
            {
                dead_bufs.insert((U64)e.buffer);
                std::lock_guard<std::mutex> lk(sDeadHandleMutex);
                sDeadBufferHandles.insert((U64)e.buffer);
            }
        }
        else
        {
            if (w != r)
            {
                sPendingBufferFrees[w] = e;
            }
            ++w;
        }
    }
    sPendingBufferFrees.resize(w);

    purgePerDrawDeadHandles({}, dead_bufs);
}

namespace
{

    struct MegaChunk
    {
        VkBuffer buffer     = VK_NULL_HANDLE;
        void*    allocation = nullptr;
        U8*      mapped     = nullptr;
        U32      typemask   = 0;
        U32      capacity   = 0;
        U32      used       = 0;
        U32      byte_size  = 0;
        bool     vertex     = false;
        U32      region_offsets[16] = {};
        std::vector<std::pair<U32, U32>> free_ranges;
        U64      id = 0;
        U32      domainId = 0;
    };
} // namespace

namespace LLVKLoaderInternal
{

    std::vector<U32> sMegaTypeSizes;
} // namespace LLVKLoaderInternal

namespace
{
    std::unordered_map<U32, std::vector<MegaChunk*>> sMegaVertexPools;
    std::vector<MegaChunk*> sMegaIndexPool;
    std::unordered_map<U64, MegaChunk*> sMegaChunksById;
    U64 sMegaChunkNextId = 1;

    std::atomic<U64>             sMegaOwnerNextToken{1};
    std::mutex                   sMegaOwnerMutex;
    std::unordered_map<U64, U64> sMegaRangeOwner;

    bool megaOwnerCheckEnabled()
    {
        static const bool s_on = []() -> bool {
            const char* s = getenv("AYASTORM_MEGA_OWNER");
            return s != nullptr && atoi(s) != 0;
        }();
        return s_on;
    }

    U64 megaOwnerKey(U64 chunk_id, U32 first)
    {
        return (chunk_id << 32) | (U64)first;
    }

    struct PendingMegaFree
    {
        U64 chunk;
        U32 first;
        U32 count;
        U32 enqueue_frame;
    };
    std::vector<PendingMegaFree> sPendingMegaFrees;

    struct MegaDomain
    {
        std::atomic<U32>                                 mMegaOwner{0};
        std::unordered_map<U32, std::vector<MegaChunk*>> mVtxChunks;
        std::vector<MegaChunk*>                          mIdxChunks;
        std::vector<PendingMegaFree>                     mPendMega;
        std::mutex                                       mPendMutex;
        U32                                              mId = 0;
        explicit MegaDomain(U32 id) : mId(id) {}
    };
    MegaDomain                    sMegaMain{0};
    std::vector<MegaDomain*>      sMegaDomains;
    struct MegaDomainInit
    {
        MegaDomainInit()
        {
            sMegaDomains.reserve(256);
            sMegaDomains.push_back(&sMegaMain);
        }
    } sMegaDomainInit;
    MegaDomain* megaDomainFor(U32 id)
    {
        return (id < sMegaDomains.size()) ? sMegaDomains[id] : &sMegaMain;
    }

    constexpr U32 MEGA_V_CHUNK_INITIAL = 65536;
    constexpr U32 MEGA_V_CHUNK_MAX     = 1048576;
    constexpr U32 MEGA_I_CHUNK_INITIAL = 1u << 20;
    constexpr U32 MEGA_I_CHUNK_MAX     = 16u << 20;

    U32 megaVertexChunkBytes(U32 typemask, U32 capacity, U32* offsets)
    {
        U32 offset = 0;
        for (U32 i = 0; i < (U32)sMegaTypeSizes.size(); ++i)
        {
            if (typemask & (1u << i))
            {
                offsets[i] = offset;
                offset += sMegaTypeSizes[i] * capacity;
                offset = (offset + 0xF) & ~0xFu;
            }
        }
        return offset;
    }

    bool megaAllocRange(MegaChunk* c, U32 count, U32& out_first)
    {
        for (size_t i = 0; i < c->free_ranges.size(); ++i)
        {
            auto& fr = c->free_ranges[i];
            if (fr.second >= count)
            {
                out_first = fr.first;
                fr.first  += count;
                fr.second -= count;
                if (fr.second == 0)
                {
                    c->free_ranges.erase(c->free_ranges.begin() + i);
                }
                c->used += count;
                return true;
            }
        }
        return false;
    }

    void megaFreeRange(MegaChunk* c, U32 first, U32 count)
    {
        c->used -= count;
        if (megaOwnerCheckEnabled())
        {
            std::lock_guard<std::mutex> lk(sMegaOwnerMutex);
            sMegaRangeOwner.erase(megaOwnerKey(c->id, first));
        }
        auto& v = c->free_ranges;
        size_t i = 0;
        while (i < v.size() && v[i].first < first)
        {
            ++i;
        }
        v.insert(v.begin() + i, { first, count });
        if (i + 1 < v.size() && v[i].first + v[i].second == v[i + 1].first)
        {
            v[i].second += v[i + 1].second;
            v.erase(v.begin() + i + 1);
        }
        if (i > 0 && v[i - 1].first + v[i - 1].second == v[i].first)
        {
            v[i - 1].second += v[i].second;
            v.erase(v.begin() + i);
        }
    }

    MegaChunk* megaNewChunk(U32 typemask, U32 min_capacity, bool vertex_chunk)
    {
        MegaChunk* c = new MegaChunk();
        c->typemask  = typemask;
        c->vertex    = vertex_chunk;

        U32 bytes;
        if (vertex_chunk)
        {
            U32 grow = MEGA_V_CHUNK_INITIAL;
            auto& pool = sMegaVertexPools[typemask];
            if (!pool.empty())
            {
                grow = llmin(pool.back()->capacity * 2, MEGA_V_CHUNK_MAX);
            }
            c->capacity = llmax(grow, min_capacity);
            bytes = megaVertexChunkBytes(typemask, c->capacity, c->region_offsets);
        }
        else
        {
            U32 grow = MEGA_I_CHUNK_INITIAL;
            if (!sMegaIndexPool.empty())
            {
                grow = llmin(sMegaIndexPool.back()->capacity * 2, MEGA_I_CHUNK_MAX);
            }
            c->capacity = llmax(grow, min_capacity);
            bytes = c->capacity;
        }
        c->byte_size = bytes;

        void* mapped = nullptr;
        const VkBufferUsageFlags usage = (vertex_chunk ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                                                       : VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
                                         | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        if (!createBufferVkImpl(bytes, usage, c->buffer, c->allocation, &mapped, true)
            || mapped == nullptr)
        {
            if (c->buffer != VK_NULL_HANDLE || c->allocation != nullptr)
            {
                destroyBufferVk(c->buffer, c->allocation);
            }
            delete c;
            LL_WARNS("Vulkan") << "megabuf chunk creation failed bytes=" << bytes << LL_ENDL;
            return nullptr;
        }
        c->mapped = (U8*)mapped;
        c->free_ranges.push_back({ 0, c->capacity });
        c->id = sMegaChunkNextId++;
        sMegaChunksById[c->id] = c;
        if (vertex_chunk)
        {
            sMegaVertexPools[typemask].push_back(c);
        }
        else
        {
            sMegaIndexPool.push_back(c);
        }
        return c;
    }

    MegaChunk* megaGrowChunk(MegaDomain* md, U32 typemask, U32 min_capacity, bool vertex_chunk)
    {
        std::lock_guard<std::mutex> lk(sAllocGrowthMutex);
        MegaChunk* c = megaNewChunk(typemask, min_capacity, vertex_chunk);
        if (c != nullptr)
        {
            c->domainId = md->mId;
            if (vertex_chunk)
            {
                md->mVtxChunks[typemask].push_back(c);
            }
            else
            {
                md->mIdxChunks.push_back(c);
            }
        }
        return c;
    }

    void megaEnqueueFree(U64 chunkId, U32 first, U32 count)
    {
        std::lock_guard<std::mutex> lk(sAllocGrowthMutex);
        auto it = sMegaChunksById.find(chunkId);
        if (it == sMegaChunksById.end())
        {
            return;
        }
        MegaDomain* md = megaDomainFor(it->second->domainId);
        std::lock_guard<std::mutex> pl(md->mPendMutex);
        md->mPendMega.push_back({ chunkId, first, count, sMonotonicFrameCount });
    }
} // namespace


void megabufInit(const U32* type_sizes, U32 type_count)
{
    sMegaTypeSizes.assign(type_sizes, type_sizes + type_count);
}

namespace
{

    struct VbStagingChunk
    {
        VkBuffer buffer     = VK_NULL_HANDLE;
        void*    allocation = nullptr;
        U8*      mapped     = nullptr;
        U32      capacity   = 0;
        U32      cursor     = 0;
    };
    struct VbStagingSlot
    {
        std::vector<VbStagingChunk> chunks;
        U32 frame_stamp = 0;
    };
    VbStagingSlot sVbStaging[FRAMES_IN_FLIGHT];

    VbStagingChunk* vbStagingAlloc(U32 bytes, U32& out_offset)
    {
        VbStagingSlot& slot = sVbStaging[sFrameIndex];
        if (slot.frame_stamp != sMonotonicFrameCount)
        {
            slot.frame_stamp = sMonotonicFrameCount;
            for (VbStagingChunk& c : slot.chunks)
            {
                c.cursor = 0;
            }
        }
        for (VbStagingChunk& c : slot.chunks)
        {
            const U32 off = (c.cursor + 15u) & ~15u;
            if (off + bytes <= c.capacity)
            {
                c.cursor   = off + bytes;
                out_offset = off;
                return &c;
            }
        }
        VbStagingChunk c;
        c.capacity = llmax(bytes, 1u << 20);
        void* mapped = nullptr;
        if (!createBufferVkImpl(c.capacity, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                c.buffer, c.allocation, &mapped, false)
            || mapped == nullptr)
        {
            if (c.buffer != VK_NULL_HANDLE || c.allocation != nullptr)
            {
                destroyBufferVk(c.buffer, c.allocation);
            }
            LL_WARNS("Vulkan") << "vb staging chunk creation failed bytes=" << c.capacity << LL_ENDL;
            return nullptr;
        }
        c.mapped   = (U8*)mapped;
        c.cursor   = bytes;
        out_offset = 0;
        slot.chunks.push_back(c);
        return &slot.chunks.back();
    }

    VkCommandBuffer sVbUploadCommandBuffers[FRAMES_IN_FLIGHT] = {};
    uint64_t        sVbUploadTimelineValue[FRAMES_IN_FLIGHT] = {};
    bool            sVbUploadPending[FRAMES_IN_FLIGHT]       = {};
    bool            sVbUploadRecording                       = false;
} // namespace

namespace LLVKLoaderInternal
{

    void vbUploadFrameReset()
    {
        if (sFrameIndex < FRAMES_IN_FLIGHT && sVbUploadPending[sFrameIndex])
        {
            waitTimeline(sVbUploadTimelineValue[sFrameIndex]);
            sVbUploadPending[sFrameIndex] = false;
        }
        sVbUploadRecording = false;
    }
} // namespace LLVKLoaderInternal

namespace
{

    VkCommandBuffer vbUploadCmd()
    {
        if (!sInFrame || sFrameIndex >= FRAMES_IN_FLIGHT)
        {
            return VK_NULL_HANDLE;
        }
        VkCommandBuffer& cmd = sVbUploadCommandBuffers[sFrameIndex];
        if (sVbUploadRecording)
        {
            return cmd;
        }
        if (cmd == VK_NULL_HANDLE)
        {
            VkCommandBufferAllocateInfo ai = {};
            ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            ai.commandPool        = sCommandPool;
            ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            ai.commandBufferCount = 1;
            if (vkAllocateCommandBuffers(sDevice, &ai, &cmd) != VK_SUCCESS)
            {
                cmd = VK_NULL_HANDLE;
                return VK_NULL_HANDLE;
            }
        }
        vkResetCommandBuffer(cmd, 0);
        VkCommandBufferBeginInfo bi = {};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (vkBeginCommandBuffer(cmd, &bi) != VK_SUCCESS)
        {
            return VK_NULL_HANDLE;
        }
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 0, nullptr);
        sVbUploadRecording = true;
        return cmd;
    }
} // namespace

namespace LLVKLoaderInternal
{

    void vbUploadSubmit()
    {
        if (!sVbUploadRecording)
        {
            return;
        }
        sVbUploadRecording = false;
        VkCommandBuffer cmd = sVbUploadCommandBuffers[sFrameIndex];
        if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
        {
            return;
        }
        PEJob job;
        job.slot = sFrameIndex;
        job.cmd  = cmd;
        sVbUploadPending[sFrameIndex] = true;
        sVbUploadTimelineValue[sFrameIndex] = peEnqueue(std::move(job));
    }
} // namespace LLVKLoaderInternal


bool vbStageCopyVk(VkBuffer dst_buffer, const VbCopyRegion* regions, U32 region_count)
{
    constexpr U32 kMaxRegions = 16;
    if (dst_buffer == VK_NULL_HANDLE || region_count == 0 || region_count > kMaxRegions)
    {
        return false;
    }
    if (!on_main_thread())
    {
        LLVKContract::cause(LLVKContract::C_VBSTAGE_OFFMAIN);
        return false;
    }
    VkCommandBuffer cmd = vbUploadCmd();
    if (cmd == VK_NULL_HANDLE)
    {
        return false;
    }
    U32 total = 0;
    for (U32 i = 0; i < region_count; ++i)
    {
        total = ((total + 15u) & ~15u) + regions[i].bytes;
    }
    U32 base = 0;
    VbStagingChunk* chunk = vbStagingAlloc(total, base);
    if (chunk == nullptr)
    {
        return false;
    }
    VkBufferCopy copies[kMaxRegions];
    U32 off = base;
    for (U32 i = 0; i < region_count; ++i)
    {
        off = (off + 15u) & ~15u;
        std::memcpy(chunk->mapped + off, regions[i].src, regions[i].bytes);
        copies[i].srcOffset = off;
        copies[i].dstOffset = regions[i].dst_offset;
        copies[i].size      = regions[i].bytes;
        off += regions[i].bytes;
    }
    vkCmdCopyBuffer(cmd, chunk->buffer, dst_buffer, region_count, copies);
    VkMemoryBarrier mb = {};
    mb.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    mb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    mb.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT
                       | VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 1, &mb, 0, nullptr, 0, nullptr);
    ++gVkPerf.vb_orphan;
    gVkPerf.vb_copy_bytes += total;
    return true;
}

void megabufShutdown()
{
    for (VbStagingSlot& slot : sVbStaging)
    {
        for (VbStagingChunk& c : slot.chunks)
        {
            destroyBufferVk(c.buffer, c.allocation);
        }
        slot.chunks.clear();
        slot.frame_stamp = 0;
    }
    sPendingMegaFrees.clear();
    for (auto& it : sMegaChunksById)
    {
        MegaChunk* c = it.second;
        destroyBufferVk(c->buffer, c->allocation);
        delete c;
    }
    sMegaChunksById.clear();
    sMegaVertexPools.clear();
    sMegaIndexPool.clear();
    for (MegaDomain* md : sMegaDomains)
    {
        md->mVtxChunks.clear();
        md->mIdxChunks.clear();
        std::lock_guard<std::mutex> pl(md->mPendMutex);
        md->mPendMega.clear();
    }
}

bool megabufAcquireVertex(U32 typemask, U32 nverts, MegaSliceV& out)
{
    out = MegaSliceV();
    if (nverts == 0 || sMegaTypeSizes.empty())
    {
        return false;
    }
    const U32 count = (nverts + 3u) & ~3u;
    MegaDomain* md = megaDomainFor(tAllocDomain->mId);
    VkcRaceProbe probe(md->mMegaOwner, LLVKContract::C_MEGA_RACE);
    MegaChunk* chunk = nullptr;
    U32 first = 0;
    auto pit = md->mVtxChunks.find(typemask);
    if (pit != md->mVtxChunks.end())
    {
        for (MegaChunk* c : pit->second)
        {
            if (megaAllocRange(c, count, first))
            {
                chunk = c;
                break;
            }
        }
    }
    if (chunk == nullptr)
    {
        chunk = megaGrowChunk(md, typemask, count, true);
        if (chunk == nullptr || !megaAllocRange(chunk, count, first))
        {
            return false;
        }
    }
    out.buffer         = chunk->buffer;
    out.mapped         = chunk->mapped;
    out.first          = first;
    out.count          = count;
    out.region_offsets = chunk->region_offsets;
    out.chunk          = chunk->id;
    if (megaOwnerCheckEnabled())
    {
        const U64 tok = sMegaOwnerNextToken.fetch_add(1, std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> lk(sMegaOwnerMutex);
            sMegaRangeOwner[megaOwnerKey(chunk->id, first)] = tok;
        }
        out.owner_token = tok;
    }
    return true;
}

void megabufReleaseVertex(const MegaSliceV& slice)
{
    if (slice.chunk == 0)
    {
        return;
    }
    megaEnqueueFree(slice.chunk, slice.first, slice.count);
}

U64 megaCurrentRangeOwner(U64 chunk_id, U32 first)
{
    if (!megaOwnerCheckEnabled())
    {
        return 0;
    }
    std::lock_guard<std::mutex> lk(sMegaOwnerMutex);
    auto it = sMegaRangeOwner.find(megaOwnerKey(chunk_id, first));
    return (it != sMegaRangeOwner.end()) ? it->second : 0;
}

bool megabufAcquireIndex(U32 size_bytes, MegaSliceI& out)
{
    out = MegaSliceI();
    if (size_bytes == 0)
    {
        return false;
    }
    const U32 count = (size_bytes + 3u) & ~3u;
    MegaDomain* md = megaDomainFor(tAllocDomain->mId);
    VkcRaceProbe probe(md->mMegaOwner, LLVKContract::C_MEGA_RACE);
    MegaChunk* chunk = nullptr;
    U32 first = 0;
    for (MegaChunk* c : md->mIdxChunks)
    {
        if (megaAllocRange(c, count, first))
        {
            chunk = c;
            break;
        }
    }
    if (chunk == nullptr)
    {
        chunk = megaGrowChunk(md, 0, count, false);
        if (chunk == nullptr || !megaAllocRange(chunk, count, first))
        {
            return false;
        }
    }
    out.buffer = chunk->buffer;
    out.mapped = chunk->mapped;
    out.offset = first;
    out.size   = count;
    out.chunk  = chunk->id;
    return true;
}

void megabufReleaseIndex(const MegaSliceI& slice)
{
    if (slice.chunk == 0)
    {
        return;
    }
    megaEnqueueFree(slice.chunk, slice.offset, slice.size);
}

void megaReclaimDomain(MegaDomain* md)
{
    VkcRaceProbe probe(md->mMegaOwner, LLVKContract::C_MEGA_RACE);
    std::vector<PendingMegaFree> ready;
    {
        std::lock_guard<std::mutex> lk(md->mPendMutex);
        size_t w = 0;
        const size_t n = md->mPendMega.size();
        for (size_t r = 0; r < n; ++r)
        {
            PendingMegaFree& e = md->mPendMega[r];
            if (reapReady(e.enqueue_frame))
            {
                ready.push_back(e);
            }
            else
            {
                if (w != r)
                {
                    md->mPendMega[w] = e;
                }
                ++w;
            }
        }
        md->mPendMega.resize(w);
    }
    if (ready.empty())
    {
        return;
    }
    std::lock_guard<std::mutex> gl(sAllocGrowthMutex);
    for (const PendingMegaFree& e : ready)
    {
        auto it = sMegaChunksById.find(e.chunk);
        if (it != sMegaChunksById.end())
        {
            megaFreeRange(it->second, e.first, e.count);
        }
    }
}

void drawDataReclaimDomain(AllocDomain* d)
{
    VkcRaceProbe probe(d->mSlotOwner, LLVKContract::C_DRAWDATA_RACE);
    std::lock_guard<std::mutex> lk(d->mPendMutex);
    size_t w = 0;
    const size_t n = d->mPendSlot.size();
    for (size_t r = 0; r < n; ++r)
    {
        PendingSlotFree& e = d->mPendSlot[r];
        if (reapReady(e.enqueue_frame))
        {
            d->mSlotFree.push_back(e.slot);
        }
        else
        {
            if (w != r)
            {
                d->mPendSlot[w] = e;
            }
            ++w;
        }
    }
    d->mPendSlot.resize(w);
}

U32 createRenderDomain()
{
    std::lock_guard<std::mutex> lk(sAllocGrowthMutex);
    U32 id = (U32)sAllocDomains.size();
    sAllocDomains.push_back(new AllocDomain(id));
    sMegaDomains.push_back(new MegaDomain(id));
    return id;
}

void renderDomainReclaim(U32 id)
{
    if (id < sAllocDomains.size())
    {
        drawDataReclaimDomain(sAllocDomains[id]);
    }
    if (id < sMegaDomains.size())
    {
        megaReclaimDomain(sMegaDomains[id]);
    }
}

void setThreadAllocDomain(U32 id)
{
    tAllocDomain = (id < sAllocDomains.size()) ? sAllocDomains[id] : &sMainDomain;
}

void tickMegaFreeQueue()
{
    static const bool s_selftest = []() -> bool {
        const char* e = getenv("AYASTORM_ALLOC_SELFTEST");
        return e != nullptr && e[0] != '\0' && e[0] != '0';
    }();
    if (s_selftest)
    {
        static bool s_ran = false;
        if (!s_ran)
        {
            s_ran = true;
            allocDomainSelfTest();
        }
    }
    megaReclaimDomain(&sMegaMain);
}

void megabufStats(U64& chunks, U64& capacity_bytes, U64& used_bytes)
{
    chunks = sMegaChunksById.size();
    capacity_bytes = 0;
    used_bytes = 0;
    for (auto& it : sMegaChunksById)
    {
        MegaChunk* c = it.second;
        capacity_bytes += c->byte_size;
        if (c->vertex)
        {
            used_bytes += (U64)c->used * (c->capacity ? (c->byte_size / c->capacity) : 0);
        }
        else
        {
            used_bytes += c->used;
        }
    }
}

bool isIndirectDrawEnabled()
{
    return sMultiDrawIndirectEnabled && sDrawIndirectFirstInstanceEnabled;
}

void tickIndirectRing()
{
    if (sDevice == VK_NULL_HANDLE)
    {
        return;
    }
    if (sIndirectRingBuffer == VK_NULL_HANDLE)
    {
        void* mapped = nullptr;
        if (!createBufferVkImpl(FRAMES_IN_FLIGHT * INDIRECT_RING_COMMANDS_PER_FRAME
                                    * (U32)sizeof(VkDrawIndexedIndirectCommand),
                                VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                                sIndirectRingBuffer, sIndirectRingAllocation, &mapped, true)
            || mapped == nullptr)
        {
            static bool warned = false;
            if (!warned)
            {
                LL_WARNS("Vulkan") << "VKIndirect: ring buffer creation failed" << LL_ENDL;
                warned = true;
            }
            if (sIndirectRingBuffer != VK_NULL_HANDLE)
            {
                destroyBufferVk(sIndirectRingBuffer, sIndirectRingAllocation);
                sIndirectRingBuffer     = VK_NULL_HANDLE;
                sIndirectRingAllocation = nullptr;
            }
            return;
        }
        sIndirectRingMapped = reinterpret_cast<U8*>(mapped);
    }
    if (sIndirectRingFrame != sMonotonicFrameCount)
    {
        sIndirectRingFrame = sMonotonicFrameCount;
        sIndirectRingCursor.store(0, std::memory_order_relaxed);
    }
}

bool indirectRingAlloc(U32 count, VkBuffer& out_buffer, VkDeviceSize& out_offset, void*& out_mapped)
{
    if (count == 0 || count > INDIRECT_RING_COMMANDS_PER_FRAME)
    {
        ++gVkPerf.mdi_full;
        return false;
    }
    if (sIndirectRingBuffer == VK_NULL_HANDLE || sIndirectRingMapped == nullptr)
    {
        return false;
    }
    const U32 local = sIndirectRingCursor.fetch_add(count, std::memory_order_relaxed);
    if (local + count > INDIRECT_RING_COMMANDS_PER_FRAME)
    {
        ++gVkPerf.mdi_full;
        return false;
    }
    const U32 region = (sFrameIndex < FRAMES_IN_FLIGHT) ? sFrameIndex : 0;
    const U64 base   = ((U64)region * INDIRECT_RING_COMMANDS_PER_FRAME + local)
                     * sizeof(VkDrawIndexedIndirectCommand);
    out_buffer = sIndirectRingBuffer;
    out_offset = (VkDeviceSize)base;
    out_mapped = sIndirectRingMapped + base;
    return true;
}

} // namespace LLVKLoader
