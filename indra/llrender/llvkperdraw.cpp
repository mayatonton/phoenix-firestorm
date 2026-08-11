/**
* @file llvkperdraw.cpp
* @brief AYAstorm r42 Vulkan loader — scene per-draw descriptor cache / per-frame descriptors / shared+per-draw UBOs / skin palette
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
    constexpr VkDeviceSize PERFRAME_UBO_SIZE         = sizeof(PerFrameMatrixUBO);
    constexpr VkDeviceSize TEXTURE_UBO_SIZE          = sizeof(TextureMatrixUBO);
    constexpr VkDeviceSize MATRIX_RING_SLOT_SIZE     = PERFRAME_UBO_SIZE + TEXTURE_UBO_SIZE;
    constexpr VkDeviceSize PREVIEWAMBIENT_UBO_OFFSET = 1024;
    constexpr VkDeviceSize PREVIEWAMBIENT_UBO_SIZE   = sizeof(PreviewAmbient_PerShaderBind);
    constexpr VkDeviceSize STARTIME_UBO_OFFSET       = 1792;
    constexpr VkDeviceSize STARTIME_UBO_SIZE         = sizeof(StarTime_PerShaderBind);
    constexpr VkDeviceSize AVATAR_VELOCITY_PALETTE_UBO_OFFSET = 2048;
    constexpr VkDeviceSize AVATAR_VELOCITY_PALETTE_UBO_SIZE   = sizeof(AvatarVelocityPalette_PerShaderBind);
    constexpr VkDeviceSize GLOWCOMBINE_UBO_OFFSET    = 3072;
    constexpr VkDeviceSize GLOWCOMBINE_UBO_SIZE      = sizeof(GlowCombine_PerShaderBind);
    constexpr VkDeviceSize UBO_BUFFER_SIZE_FRAME     = GLOWCOMBINE_UBO_OFFSET + GLOWCOMBINE_UBO_SIZE;

    constexpr U32 MATRIX_RING_CHUNK_SLOTS = 64;
    constexpr U32 MATRIX_RING_MAX_SLOTS  = 16384;
    constexpr U32 MATRIX_RING_MAX_CHUNKS = MATRIX_RING_MAX_SLOTS / MATRIX_RING_CHUNK_SLOTS;
    std::mutex                   sMatrixRingGrowthMutex;
    thread_local float sMatrixRingCurrentProj[FRAMES_IN_FLIGHT][16] = {};
    thread_local float sMatrixRingCurrentTexmat[FRAMES_IN_FLIGHT][64] = {};
} // namespace

namespace LLVKLoaderInternal
{
    bool slotSlabGrow(AllocDomain* d)
    {
        std::lock_guard<std::mutex> lk(sAllocGrowthMutex);
        const U32 idx = sSlotSlabNextIdx;
        const U64 first = (U64)idx * DRAWDATA_DOMAIN_SLAB;
        if (idx >= DRAWDATA_MAX_SLABS || first + DRAWDATA_DOMAIN_SLAB > DRAWDATA_PERSISTENT_SLOTS)
        {
            return false;
        }
        sSlotSlabNextIdx = idx + 1;
        sSlotSlabOwner[idx].store(d->mId, std::memory_order_release);
        d->mSlotNext = (idx == 0) ? 1u : (U32)first;
        d->mSlotEnd  = (U32)(first + DRAWDATA_DOMAIN_SLAB);
        return true;
    }
    AllocDomain* allocDomainForSlot(U32 slot)
    {
        const U32 si = slot / DRAWDATA_DOMAIN_SLAB;
        if (si >= DRAWDATA_MAX_SLABS)
        {
            return nullptr;
        }
        const U32 id = sSlotSlabOwner[si].load(std::memory_order_acquire);
        if (id == 0xFFFFFFFFu || id >= sAllocDomains.size())
        {
            return nullptr;
        }
        return sAllocDomains[id];
    }
} // namespace LLVKLoaderInternal

namespace
{

    constexpr U32                  SCENE_PER_DRAW_POOL_GROWTH_SETS     = 50000;
    constexpr U32                  SCENE_PER_DRAW_POOL_GROWTH_SAMPLERS = 250000;
    constexpr U32                  SCENE_PER_DRAW_POOL_GROWTH_UBOS     = 50000;
} // namespace

namespace LLVKLoaderInternal
{
    U64 sScenePerDrawCacheEpoch = 1;
    PerDrawDescLane sPerDrawDescLanes[MAX_RECORD_LANES];
} // namespace LLVKLoaderInternal

namespace
{

    static void laneIndexHandle(std::unordered_map<U64, std::list<ScenePerDrawCacheKey>>& index,
                                U64 handle, const ScenePerDrawCacheKey& key,
                                std::vector<std::pair<U64, std::list<ScenePerDrawCacheKey>::iterator>>& rev)
    {
        std::list<ScenePerDrawCacheKey>& lst = index[handle];
        lst.push_front(key);
        rev.emplace_back(handle, lst.begin());
    }

    static void laneIndexKey(PerDrawDescLane& lane, const ScenePerDrawCacheKey& key,
                             ScenePerDrawCacheEntry& entry)
    {
        for (U32 i = 0; i < key.sampler_count; ++i)
        {
            if (key.sampler_views[i] != VK_NULL_HANDLE)
            {
                laneIndexHandle(lane.by_view, (U64)key.sampler_views[i], key, entry.rev_views);
            }
        }
        if (key.ubo != VK_NULL_HANDLE)
        {
            laneIndexHandle(lane.by_buf, (U64)key.ubo, key, entry.rev_bufs);
        }
        for (U32 i = 0; i < key.ubo_count; ++i)
        {
            if (key.ubo_write_bufs[i] != VK_NULL_HANDLE)
            {
                laneIndexHandle(lane.by_buf, (U64)key.ubo_write_bufs[i], key, entry.rev_bufs);
            }
        }
    }

    static void laneUnindexSide(std::unordered_map<U64, std::list<ScenePerDrawCacheKey>>& index,
                                std::vector<std::pair<U64, std::list<ScenePerDrawCacheKey>::iterator>>& rev)
    {
        for (auto& p : rev)
        {
            auto mit = index.find(p.first);
            if (mit == index.end())
            {
                continue;
            }
            mit->second.erase(p.second);
            if (mit->second.empty())
            {
                index.erase(mit);
            }
        }
        rev.clear();
    }

    static void laneUnindexEntry(PerDrawDescLane& lane, ScenePerDrawCacheEntry& entry)
    {
        laneUnindexSide(lane.by_view, entry.rev_views);
        laneUnindexSide(lane.by_buf, entry.rev_bufs);
    }
} // namespace

namespace LLVKLoaderInternal
{
    std::vector<DeferredUtilOverrideSlot> sDuOverrideRing[FRAMES_IN_FLIGHT];
    U32 sDuOverrideIdx[FRAMES_IN_FLIGHT]   = { 0, 0, 0 };
    U64 sDuOverrideFrame[FRAMES_IN_FLIGHT] = { 0, 0, 0 };

    std::vector<DeferredUtilOverrideSlot> sDeferredUtilRing[FRAMES_IN_FLIGHT];
    U32      sDeferredUtilRingIdx[FRAMES_IN_FLIGHT]   = { 0, 0, 0 };
    U64      sDeferredUtilRingFrame[FRAMES_IN_FLIGHT] = { 0, 0, 0 };
    VkBuffer sCurDeferredUtilBuf[FRAMES_IN_FLIGHT]    = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    void*    sCurDeferredUtilMapped[FRAMES_IN_FLIGHT] = { nullptr, nullptr, nullptr };
} // namespace LLVKLoaderInternal

namespace
{
    VkBuffer sDuOverrideActiveBuf    = VK_NULL_HANDLE;
    void*    sDuOverrideActiveMapped = nullptr;
} // namespace

namespace LLVKLoaderInternal
{
    std::vector<DeferredUtilOverrideSlot> sShadowUtilRing[FRAMES_IN_FLIGHT];
    U32      sShadowUtilRingIdx[FRAMES_IN_FLIGHT]   = { 0, 0, 0 };
    U64      sShadowUtilRingFrame[FRAMES_IN_FLIGHT] = { 0, 0, 0 };
    VkBuffer sCurShadowUtilBuf[FRAMES_IN_FLIGHT]    = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    void*    sCurShadowUtilMapped[FRAMES_IN_FLIGHT] = { nullptr, nullptr, nullptr };

    #define LLVK_SHARED_UBO_RING_STORAGE(BindName)                                                      \
        std::vector<DeferredUtilOverrideSlot> s##BindName##Ring[FRAMES_IN_FLIGHT];                      \
        U32      s##BindName##RingIdx[FRAMES_IN_FLIGHT]   = { 0, 0, 0 };                                \
        U64      s##BindName##RingFrame[FRAMES_IN_FLIGHT] = { 0, 0, 0 };                                \
        VkBuffer sCur##BindName##Buf[FRAMES_IN_FLIGHT]    = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE }; \
        void*    sCur##BindName##Mapped[FRAMES_IN_FLIGHT] = { nullptr, nullptr, nullptr };
    LLVK_SHARED_UBO_RING_STORAGE(WindlightSky)
    LLVK_SHARED_UBO_RING_STORAGE(WindlightAtmos)
    LLVK_SHARED_UBO_RING_STORAGE(AoUtil)
    LLVK_SHARED_UBO_RING_STORAGE(GlobalF)
    LLVK_SHARED_UBO_RING_STORAGE(WaterFog)
    LLVK_SHARED_UBO_RING_STORAGE(WaterV)
    LLVK_SHARED_UBO_RING_STORAGE(ReflectionProbe)
    LLVK_SHARED_UBO_RING_STORAGE(ReflectionProbes)
    LLVK_SHARED_UBO_RING_STORAGE(Lights)
    LLVK_SHARED_UBO_RING_STORAGE(LightsSpecular)
    LLVK_SHARED_UBO_RING_STORAGE(PbrTerrainF)
    LLVK_SHARED_UBO_RING_STORAGE(PbrTerrain)
    #undef LLVK_SHARED_UBO_RING_STORAGE

    PerDrawUBOArena sPerDrawUBOArena[FRAMES_IN_FLIGHT];
} // namespace LLVKLoaderInternal

namespace
{
    constexpr VkDeviceSize PER_DRAW_UBO_ARENA_INITIAL = 4 * 1024 * 1024;
    std::mutex sPerDrawArenaGrowthMutex;
} // namespace

namespace LLVKLoaderInternal
{

    bool createPerFrameDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding bindings[6] = {};
        bindings[0].binding         = 0;
        bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[1].binding         = 1;
        bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[2].binding         = 4;
        bindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

        bindings[3].binding         = 7;
        bindings[3].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[3].descriptorCount = 1;
        bindings[3].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[4].binding         = 8;
        bindings[4].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[4].descriptorCount = 1;
        bindings[4].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

        bindings[5].binding         = 10;
        bindings[5].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[5].descriptorCount = 1;
        bindings[5].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo info = {};
        info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 6;
        info.pBindings    = bindings;

        VkResult result = vkCreateDescriptorSetLayout(sDevice, &info, nullptr, &sPerFrameDescriptorSetLayout);
        if (result != VK_SUCCESS)
        {
            return false;
        }

        return true;
    }

    bool createPerFrameUbos()
    {
        for (U32 frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            VkBufferCreateInfo buf_info = {};
            buf_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buf_info.size        = UBO_BUFFER_SIZE_FRAME;
            buf_info.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VkResult result = vkCreateBuffer(sDevice, &buf_info, nullptr, &sPerFrameUboBuffer[frame]);
            if (result != VK_SUCCESS)
            {
                return false;
            }

            VkMemoryRequirements mem_req;
            vkGetBufferMemoryRequirements(sDevice, sPerFrameUboBuffer[frame], &mem_req);

            S32 mem_type = findMemoryType(mem_req.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            if (mem_type < 0)
            {
                return false;
            }

            VkMemoryAllocateInfo alloc_info = {};
            alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize  = mem_req.size;
            alloc_info.memoryTypeIndex = (U32)mem_type;

            result = vkAllocateMemory(sDevice, &alloc_info, nullptr, &sPerFrameUboMemory[frame]);
            if (result != VK_SUCCESS)
            {
                return false;
            }

            result = vkBindBufferMemory(sDevice, sPerFrameUboBuffer[frame], sPerFrameUboMemory[frame], 0);
            if (result != VK_SUCCESS)
            {
                return false;
            }

            result = vkMapMemory(sDevice, sPerFrameUboMemory[frame], 0, VK_WHOLE_SIZE, 0, &sPerFrameUboMapped[frame]);
            if (result != VK_SUCCESS)
            {
                return false;
            }

            std::memset(sPerFrameUboMapped[frame], 0, (size_t)UBO_BUFFER_SIZE_FRAME);
        }

        return true;
    }
} // namespace LLVKLoaderInternal

namespace
{

    void writePerFrameSetBindings(VkDescriptorSet set, U32 frame, VkBuffer matrixBuf, VkDeviceSize matrixOffset)
    {
        struct BindSpec { U32 binding; VkBuffer buf; VkDeviceSize off; VkDeviceSize range; };
        const BindSpec specs[6] = {
            { 0,  matrixBuf,                 matrixOffset,                       PERFRAME_UBO_SIZE },
            { 1,  matrixBuf,                 matrixOffset + PERFRAME_UBO_SIZE,   TEXTURE_UBO_SIZE },
            { 4,  sPerFrameUboBuffer[frame], PREVIEWAMBIENT_UBO_OFFSET,          PREVIEWAMBIENT_UBO_SIZE },
            { 7,  sPerFrameUboBuffer[frame], STARTIME_UBO_OFFSET,                STARTIME_UBO_SIZE },
            { 8,  sPerFrameUboBuffer[frame], AVATAR_VELOCITY_PALETTE_UBO_OFFSET, AVATAR_VELOCITY_PALETTE_UBO_SIZE },
            { 10, sPerFrameUboBuffer[frame], GLOWCOMBINE_UBO_OFFSET,             GLOWCOMBINE_UBO_SIZE },
        };

        VkDescriptorBufferInfo infos[6]  = {};
        VkWriteDescriptorSet   writes[6] = {};
        for (U32 i = 0; i < 6; ++i)
        {
            infos[i].buffer = specs[i].buf;
            infos[i].offset = specs[i].off;
            infos[i].range  = specs[i].range;

            writes[i].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet          = set;
            writes[i].dstBinding      = specs[i].binding;
            writes[i].descriptorCount = 1;
            writes[i].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[i].pBufferInfo     = &infos[i];
        }
        vkUpdateDescriptorSets(sDevice, 6, writes, 0, nullptr);
    }

    bool createMatrixRingChunk(U32 frame)
    {
        MatrixRingChunk chunk;

        VkBufferCreateInfo buf_info = {};
        buf_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_info.size        = (VkDeviceSize)MATRIX_RING_CHUNK_SLOTS * MATRIX_RING_SLOT_SIZE;
        buf_info.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(sDevice, &buf_info, nullptr, &chunk.buffer) != VK_SUCCESS)
        {
            return false;
        }

        VkMemoryRequirements mem_req;
        vkGetBufferMemoryRequirements(sDevice, chunk.buffer, &mem_req);

        S32 mem_type = findMemoryType(mem_req.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (mem_type < 0)
        {
            vkDestroyBuffer(sDevice, chunk.buffer, nullptr);
            return false;
        }

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize  = mem_req.size;
        alloc_info.memoryTypeIndex = (U32)mem_type;

        if (vkAllocateMemory(sDevice, &alloc_info, nullptr, &chunk.memory) != VK_SUCCESS)
        {
            vkDestroyBuffer(sDevice, chunk.buffer, nullptr);
            return false;
        }

        vkBindBufferMemory(sDevice, chunk.buffer, chunk.memory, 0);
        vkMapMemory(sDevice, chunk.memory, 0, VK_WHOLE_SIZE, 0, &chunk.mapped);
        std::memset(chunk.mapped, 0, (size_t)buf_info.size);

        sMatrixRingChunks[frame].push_back(chunk);
        return true;
    }

    VkDescriptorSet allocRingDescriptorSet()
    {
        const U32 pool_capacity = FRAMES_IN_FLIGHT * MATRIX_RING_CHUNK_SLOTS;
        if (sPerFrameRingPools.empty() || sRingPoolUsedInLast >= pool_capacity)
        {
            VkDescriptorPoolSize pool_size = {};
            pool_size.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            pool_size.descriptorCount = pool_capacity * 12;

            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.maxSets       = pool_capacity;
            pool_info.poolSizeCount = 1;
            pool_info.pPoolSizes    = &pool_size;

            VkDescriptorPool pool = VK_NULL_HANDLE;
            if (vkCreateDescriptorPool(sDevice, &pool_info, nullptr, &pool) != VK_SUCCESS)
            {
                return VK_NULL_HANDLE;
            }
            sPerFrameRingPools.push_back(pool);
            sRingPoolUsedInLast = 0;
        }

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool     = sPerFrameRingPools.back();
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts        = &sPerFrameDescriptorSetLayout;

        VkDescriptorSet set = VK_NULL_HANDLE;
        if (vkAllocateDescriptorSets(sDevice, &alloc_info, &set) != VK_SUCCESS)
        {
            return VK_NULL_HANDLE;
        }
        ++sRingPoolUsedInLast;
        return set;
    }

    bool ensurePerFrameMatrixSlot(U32 frame, U32 slot)
    {
        if (slot >= MATRIX_RING_MAX_SLOTS)
        {
            static U32 s_ring_cap_hits = 0;
            ++s_ring_cap_hits;
            if ((s_ring_cap_hits & (s_ring_cap_hits - 1)) == 0)
            {
                LL_WARNS("Vulkan") << "matrix ring slot cap reached (slot=" << slot
                                   << " cap=" << MATRIX_RING_MAX_SLOTS
                                   << " hits=" << s_ring_cap_hits << ")" << LL_ENDL;
            }
            return false;
        }
        std::lock_guard<std::mutex> lk(sMatrixRingGrowthMutex);
        if (sPerFrameRingSets[frame].capacity() < MATRIX_RING_MAX_SLOTS)
        {
            sPerFrameRingSets[frame].reserve(MATRIX_RING_MAX_SLOTS);
            sMatrixRingChunks[frame].reserve(MATRIX_RING_MAX_CHUNKS);
        }
        while (sPerFrameRingSets[frame].size() <= (size_t)slot)
        {
            const U32 new_slot = (U32)sPerFrameRingSets[frame].size();
            const U32 chunk    = new_slot / MATRIX_RING_CHUNK_SLOTS;
            const U32 within   = new_slot % MATRIX_RING_CHUNK_SLOTS;

            while (sMatrixRingChunks[frame].size() <= (size_t)chunk)
            {
                if (sMatrixRingChunks[frame].size() >= MATRIX_RING_MAX_CHUNKS)
                {
                    return false;
                }
                if (!createMatrixRingChunk(frame))
                {
                    return false;
                }
            }

            VkDescriptorSet set = allocRingDescriptorSet();
            if (set == VK_NULL_HANDLE)
            {
                return false;
            }

            writePerFrameSetBindings(set, frame,
                                     sMatrixRingChunks[frame][chunk].buffer,
                                     (VkDeviceSize)within * MATRIX_RING_SLOT_SIZE);
            sPerFrameRingSets[frame].push_back(set);
            sPerFrameRingSetCount[frame].store((U32)sPerFrameRingSets[frame].size(),
                                               std::memory_order_release);
        }
        return true;
    }
} // namespace

namespace LLVKLoaderInternal
{

    bool createPerFrameDescriptorSets()
    {
        for (U32 frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            if (!ensurePerFrameMatrixSlot(frame, 0))
            {
                return false;
            }

            PerFrameMatrixUBO init = {};
            for (U32 m = 0; m < 16; ++m)
            {
                init.projection_matrix[m] = (m % 5 == 0) ? 1.0f : 0.0f;
            }
            std::memcpy(init.inverse_projection_matrix, init.projection_matrix, sizeof(init.projection_matrix));
            std::memcpy(init.identity_matrix,           init.projection_matrix, sizeof(init.projection_matrix));
            std::memcpy(init.last_modelview_matrix,     init.projection_matrix, sizeof(init.projection_matrix));
            std::memcpy(sMatrixRingChunks[frame][0].mapped, &init, sizeof(init));

            TextureMatrixUBO tex_init = {};
            for (U32 t = 0; t < 4; ++t)
            {
                for (U32 m = 0; m < 16; ++m)
                {
                    tex_init.texture_matrix[t][m] = (m % 5 == 0) ? 1.0f : 0.0f;
                }
            }
            std::memcpy(static_cast<U8*>(sMatrixRingChunks[frame][0].mapped) + PERFRAME_UBO_SIZE,
                        &tex_init, sizeof(tex_init));

            sMatrixRingUsedThisFrame[frame] = 0;
            sMatrixRingCurrentSlot[frame]   = 0;
            sMatrixRingHasCurrent[frame]    = false;
        }

        return true;
    }

    bool createScenePerDrawDescriptorPool(VkDescriptorPool* out_pool)
    {
        if (out_pool == nullptr)
        {
            return false;
        }
        *out_pool = VK_NULL_HANDLE;

        VkDescriptorPoolSize pool_sizes[3] = {};
        pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[0].descriptorCount = SCENE_PER_DRAW_POOL_GROWTH_SAMPLERS;
        pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_sizes[1].descriptorCount = SCENE_PER_DRAW_POOL_GROWTH_UBOS;
        pool_sizes[2].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        pool_sizes[2].descriptorCount = SCENE_PER_DRAW_POOL_GROWTH_SETS;

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets       = SCENE_PER_DRAW_POOL_GROWTH_SETS;
        pool_info.poolSizeCount = 3;
        pool_info.pPoolSizes    = pool_sizes;

        VkResult result = vkCreateDescriptorPool(sDevice, &pool_info, nullptr, out_pool);
        if (result != VK_SUCCESS)
        {
            *out_pool = VK_NULL_HANDLE;
            return false;
        }

        return true;
    }
} // namespace LLVKLoaderInternal


U64 getScenePerDrawCacheEpoch()
{
    return sScenePerDrawCacheEpoch;
}

void pinScenePerDrawEntry(void* token)
{
    if (token != nullptr)
    {
        static_cast<ScenePerDrawCacheEntry*>(token)->refs.fetch_add(1, std::memory_order_relaxed);
    }
}

void releaseScenePerDrawEntry(void* token, U64 epoch)
{
    if (token != nullptr && epoch == sScenePerDrawCacheEpoch)
    {
        static_cast<ScenePerDrawCacheEntry*>(token)->refs.fetch_sub(1, std::memory_order_relaxed);
    }
}

VkDescriptorSetLayout getPerFrameDescriptorSetLayout()
{
    return sPerFrameDescriptorSetLayout;
}

bool ensureScenePerDrawDescriptorSet(const ScenePerDrawBindings& b,
                                     VkDescriptorSet*            out_set,
                                     void**                      out_token)
{
    if (!out_set)
    {
        return false;
    }
    *out_set = VK_NULL_HANDLE;
    if (out_token)
    {
        *out_token = nullptr;
    }

    PerDrawDescLane& lane = sPerDrawDescLanes[0];
    if (sInitialized && lane.pools.empty())
    {
        VkDescriptorPool lazy_pool = VK_NULL_HANDLE;
        if (createScenePerDrawDescriptorPool(&lazy_pool))
        {
            lane.pools.push_back(lazy_pool);
        }
    }
    if (!sInitialized || lane.pools.empty() ||
        b.layout == VK_NULL_HANDLE || b.sampler == VK_NULL_HANDLE)
    {
        return false;
    }

    U32 clamped_count = b.sampler_count;
    if (clamped_count > ScenePerDrawBindings::MAX_SAMPLERS)
    {
        clamped_count = ScenePerDrawBindings::MAX_SAMPLERS;
    }

    ScenePerDrawCacheKey key = {};
    key.layout        = b.layout;
    key.ubo           = b.ubo;
    key.ubo_binding   = (b.ubo != VK_NULL_HANDLE) ? b.ubo_binding : 0;
    key.ubo_size      = (b.ubo != VK_NULL_HANDLE) ? b.ubo_size : 0;
    key.sampler       = b.sampler;
    key.sampler_count = clamped_count;
    for (U32 i = 0; i < clamped_count; ++i)
    {
        key.sampler_bindings[i] = b.sampler_bindings[i];
        key.sampler_views[i]    = b.sampler_views[i];
        key.sampler_samplers[i] = (b.sampler_samplers[i] != VK_NULL_HANDLE) ? b.sampler_samplers[i] : b.sampler;
    }
    U32 clamped_ubo_count = b.ubo_count;
    if (clamped_ubo_count > ScenePerDrawBindings::MAX_UBO_WRITES)
    {
        clamped_ubo_count = ScenePerDrawBindings::MAX_UBO_WRITES;
    }
    key.ubo_count = clamped_ubo_count;
    for (U32 i = 0; i < clamped_ubo_count; ++i)
    {
        key.ubo_write_bindings[i] = b.ubo_writes[i].binding;
        key.ubo_write_bufs[i]     = b.ubo_writes[i].buf;
    }

    auto cache_it = lane.cache.find(key);
    if (cache_it != lane.cache.end())
    {
        bool stale_handle = false;
        {
            std::lock_guard<std::mutex> lk(sDeadHandleMutex);
            if (key.ubo != VK_NULL_HANDLE && sDeadBufferHandles.count((U64)key.ubo) != 0)
            {
                stale_handle = true;
            }
            for (U32 i = 0; !stale_handle && i < clamped_ubo_count; ++i)
            {
                if (sDeadBufferHandles.count((U64)key.ubo_write_bufs[i]) != 0)
                {
                    stale_handle = true;
                }
            }
            for (U32 i = 0; !stale_handle && i < clamped_count; ++i)
            {
                if (sDeadViewHandles.count((U64)key.sampler_views[i]) != 0)
                {
                    stale_handle = true;
                }
            }
        }
        if (stale_handle)
        {
            if (cache_it->second.refs.load(std::memory_order_relaxed) == 0)
            {
                ScenePerDrawDeferredFreeEntry deferred = {};
                for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
                {
                    deferred.sets[i] = cache_it->second.sets[i];
                }
                deferred.pool_index    = cache_it->second.pool_index;
                deferred.enqueue_frame = sMonotonicFrameCount;
                lane.deferred_free.push_back(deferred);
                lane.lru.erase(cache_it->second.lru_pos);
                laneUnindexEntry(lane, cache_it->second);
                lane.cache.erase(cache_it);
                cache_it = lane.cache.end();
            }
            else
            {
                LL_WARNS_ONCE("Vulkan") << "scene set cache: stale handle collision on pinned entry"
                                        << " (served as-is; SETBIRTH will name it if consumed dead)"
                                        << LL_ENDL;
            }
        }
    }
    if (cache_it != lane.cache.end())
    {
        lane.lru.erase(cache_it->second.lru_pos);
        lane.lru.push_back(key);
        cache_it->second.lru_pos                   = std::prev(lane.lru.end());
        cache_it->second.last_used_monotonic_frame = sMonotonicFrameCount;
        if (out_token)
        {
            *out_token = &cache_it->second;
        }
        *out_set = cache_it->second.sets[getCurrentFrameIndex()];
        ++gVkPerf.ens_hit;
        return true;
    }

    constexpr size_t SCENE_PER_DRAW_CACHE_MAX_ENTRIES = 150000;
    if (lane.cache.size() >= SCENE_PER_DRAW_CACHE_MAX_ENTRIES)
    {
        U32 evicted = 0;
        U32 scanned = 0;
        for (auto lru_it = lane.lru.begin();
             lru_it != lane.lru.end() && evicted < 64 && scanned < 256; ++scanned)
        {
            auto cit = lane.cache.find(*lru_it);
            if (cit == lane.cache.end())
            {
                lru_it = lane.lru.erase(lru_it);
                continue;
            }
            if (cit->second.refs.load(std::memory_order_relaxed) > 0)
            {
                auto next_it = std::next(lru_it);
                lane.lru.splice(lane.lru.end(), lane.lru, lru_it);
                cit->second.lru_pos = std::prev(lane.lru.end());
                lru_it = next_it;
                continue;
            }
            if (cit->second.last_used_monotonic_frame + FRAMES_IN_FLIGHT <= sMonotonicFrameCount)
            {
                ScenePerDrawDeferredFreeEntry deferred = {};
                for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
                {
                    deferred.sets[i] = cit->second.sets[i];
                }
                deferred.pool_index    = cit->second.pool_index;
                deferred.enqueue_frame = sMonotonicFrameCount;
                lane.deferred_free.push_back(deferred);

                laneUnindexEntry(lane, cit->second);
                lane.cache.erase(cit);
                lru_it = lane.lru.erase(lru_it);
                ++evicted;
                continue;
            }
            break;
        }
    }

    VkDescriptorSetLayout layouts[FRAMES_IN_FLIGHT];
    for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        layouts[i] = b.layout;
    }

    VkDescriptorSet new_sets[FRAMES_IN_FLIGHT] = {};
    for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        new_sets[i] = VK_NULL_HANDLE;
    }
    U32 alloc_pool_index = 0;

    auto try_alloc_in_pool = [&](VkDescriptorPool pool) -> bool
    {
        if (pool == VK_NULL_HANDLE)
        {
            return false;
        }
        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool     = pool;
        alloc_info.descriptorSetCount = FRAMES_IN_FLIGHT;
        alloc_info.pSetLayouts        = layouts;
        const bool alloc_ok = vkAllocateDescriptorSets(sDevice, &alloc_info, new_sets) == VK_SUCCESS;
        if (alloc_ok && vkValidationRequested())
        {
            std::lock_guard<std::mutex> lk(sSet1BirthMutex);
            for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
            {
                Set1BirthInfo& bi = sSet1BirthLedger[(U64)new_sets[i]];
                bi.build_frame = sMonotonicFrameCount;
                bi.freed_frame = 0;
            }
        }
        return alloc_ok;
    };

    auto try_alloc_any_pool = [&]() -> bool
    {
        for (U32 pi = 0; pi < lane.pools.size(); ++pi)
        {
            if (try_alloc_in_pool(lane.pools[pi]))
            {
                alloc_pool_index = pi;
                return true;
            }
        }
        return false;
    };

    if (!try_alloc_any_pool())
    {
        bool evicted = false;
        for (auto lru_it = lane.lru.begin(); lru_it != lane.lru.end(); )
        {
            auto cit = lane.cache.find(*lru_it);
            if (cit == lane.cache.end())
            {
                lru_it = lane.lru.erase(lru_it);
                continue;
            }
            if (cit->second.refs.load(std::memory_order_relaxed) == 0 &&
                cit->second.last_used_monotonic_frame + FRAMES_IN_FLIGHT <= sMonotonicFrameCount)
            {
                ScenePerDrawDeferredFreeEntry deferred = {};
                for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
                {
                    deferred.sets[i] = cit->second.sets[i];
                }
                deferred.pool_index    = cit->second.pool_index;
                deferred.enqueue_frame = sMonotonicFrameCount;
                lane.deferred_free.push_back(deferred);

                laneUnindexEntry(lane, cit->second);
                lane.cache.erase(cit);
                lru_it = lane.lru.erase(lru_it);
                evicted = true;
                break;
            }
            ++lru_it;
        }

        if (!evicted || !try_alloc_any_pool())
        {
            VkDescriptorPool new_pool = VK_NULL_HANDLE;
            if (!createScenePerDrawDescriptorPool(&new_pool))
            {
                return false;
            }
            lane.pools.push_back(new_pool);
            if (!try_alloc_in_pool(new_pool))
            {
                return false;
            }
            alloc_pool_index = (U32)(lane.pools.size() - 1);
        }
    }

    for (U32 slot = 0; slot < FRAMES_IN_FLIGHT; ++slot)
    {
        VkDescriptorSet target_set = new_sets[slot];

        VkWriteDescriptorSet   writes[1 + ScenePerDrawBindings::MAX_SAMPLERS] = {};
        VkDescriptorBufferInfo ubo_info                                       = {};
        VkDescriptorImageInfo  image_infos[ScenePerDrawBindings::MAX_SAMPLERS] = {};
        U32                    write_count                                    = 0;

        if (b.ubo != VK_NULL_HANDLE && b.ubo_size > 0)
        {
            ubo_info.buffer = b.ubo;
            ubo_info.offset = 0;
            ubo_info.range  = b.ubo_size;

            writes[write_count].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[write_count].dstSet          = target_set;
            writes[write_count].dstBinding      = b.ubo_binding;
            writes[write_count].dstArrayElement = 0;
            writes[write_count].descriptorCount = 1;
            writes[write_count].descriptorType  = (b.ubo_binding < 64 && ((b.dynamic_mask >> b.ubo_binding) & 1))
                                                      ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
                                                      : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[write_count].pBufferInfo     = &ubo_info;
            ++write_count;
        }

        for (U32 i = 0; i < clamped_count; ++i)
        {
            if (b.sampler_views[i] == VK_NULL_HANDLE)
            {
                LL_ERRS("Vulkan") << "ensureScenePerDrawDescriptorSet: sampler_views["
                                  << (S32)i << "] = VK_NULL_HANDLE = caller bug = RULE 4 違反"
                                  << " (= getDefaultFallbackVkImageView() 経由 view 配備必須)"
                                  << LL_ENDL;
            }

            image_infos[i].sampler     = (b.sampler_samplers[i] != VK_NULL_HANDLE) ? b.sampler_samplers[i] : b.sampler;
            image_infos[i].imageView   = b.sampler_views[i];
            image_infos[i].imageLayout =
                (sInDynamicRendering && sSavedHasDepth &&
                 b.sampler_views[i] == sSavedDepthInfo.imageView &&
                 sSavedDepthInfo.imageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
                    ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                    : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            writes[write_count].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[write_count].dstSet          = target_set;
            writes[write_count].dstBinding      = b.sampler_bindings[i];
            writes[write_count].dstArrayElement = 0;
            writes[write_count].descriptorCount = 1;
            writes[write_count].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[write_count].pImageInfo      = &image_infos[i];
            ++write_count;
        }

        if (write_count > 0)
        {
            vkUpdateDescriptorSets(sDevice, write_count, writes, 0, nullptr);
        }
        if (vkValidationRequested())
        {
            std::string dump;
            for (U32 i = 0; i < clamped_count; ++i)
            {
                dump += "b" + std::to_string(b.sampler_bindings[i]) + "="
                      + hex64((U64)b.sampler_views[i])
                      + ":" + (b.sampler_sources[i] != '\0' ? std::string(1, b.sampler_sources[i]) : std::string("?"))
                      + ",";
            }
            std::lock_guard<std::mutex> lk(sSet1BirthMutex);
            sSet1BirthLedger[(U64)target_set].contents = dump;
        }

        {
            VkDescriptorBufferInfo ubo_w_infos[ScenePerDrawBindings::MAX_UBO_WRITES] = {};
            VkWriteDescriptorSet   ubo_w_writes[ScenePerDrawBindings::MAX_UBO_WRITES] = {};
            U32 ubo_w_count = 0;
            for (U32 i = 0; i < clamped_ubo_count; ++i)
            {
                if (b.ubo_writes[i].buf == VK_NULL_HANDLE || b.ubo_writes[i].size == 0)
                {
                    continue;
                }
                const U32  wb          = b.ubo_writes[i].binding;
                const bool is_dynamic0 = (wb < 64 && ((b.dynamic_mask >> wb) & 1));
                ubo_w_infos[ubo_w_count].buffer = b.ubo_writes[i].buf;
                ubo_w_infos[ubo_w_count].offset = b.ubo_writes[i].offset;
                ubo_w_infos[ubo_w_count].range  = b.ubo_writes[i].size;

                ubo_w_writes[ubo_w_count].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                ubo_w_writes[ubo_w_count].dstSet          = target_set;
                ubo_w_writes[ubo_w_count].dstBinding      = b.ubo_writes[i].binding;
                ubo_w_writes[ubo_w_count].dstArrayElement = 0;
                ubo_w_writes[ubo_w_count].descriptorCount = 1;
                ubo_w_writes[ubo_w_count].descriptorType  = is_dynamic0
                                                                ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
                                                                : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                ubo_w_writes[ubo_w_count].pBufferInfo     = &ubo_w_infos[ubo_w_count];
                ++ubo_w_count;
            }
            if (ubo_w_count > 0)
            {
                vkUpdateDescriptorSets(sDevice, ubo_w_count, ubo_w_writes, 0, nullptr);
            }
        }

        // `b.ubo_writes` is populated from every UBO entry in the exact
        // descriptor-set layout by LLGLSLShader. Do not issue a second,
        // hard-coded update for a subset of shared UBO bindings here: it
        // rewrites descriptors already set above and bypasses that layout's
        // declared descriptor type. MoltenVK crashed while processing this
        // duplicate update for the deferred terrain layout.
    }

    lane.lru.push_back(key);
    ScenePerDrawCacheEntry& entry = lane.cache.try_emplace(key).first->second;
    laneIndexKey(lane, key, entry);
    for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        entry.sets[i] = new_sets[i];
    }
    entry.lru_pos                   = std::prev(lane.lru.end());
    entry.last_used_monotonic_frame = sMonotonicFrameCount;
    entry.pool_index                = alloc_pool_index;

    if (out_token)
    {
        *out_token = &entry;
    }
    *out_set = entry.sets[getCurrentFrameIndex()];
    ++gVkPerf.ens_alloc;
    return true;
}

void tickScenePerDrawDescriptorCache()
{
    if (!sInitialized)
    {
        return;
    }
    for (PerDrawDescLane& lane : sPerDrawDescLanes)
    {
        size_t w = 0;
        const size_t n = lane.deferred_free.size();
        for (size_t r = 0; r < n; ++r)
        {
            ScenePerDrawDeferredFreeEntry& e = lane.deferred_free[r];
            if (reapReady(e.enqueue_frame))
            {
                VkDescriptorPool target_pool = (e.pool_index < lane.pools.size())
                                                   ? lane.pools[e.pool_index]
                                                   : VK_NULL_HANDLE;
                if (target_pool != VK_NULL_HANDLE)
                {
                    vkFreeDescriptorSets(sDevice, target_pool, FRAMES_IN_FLIGHT, e.sets);
                    if (vkValidationRequested())
                    {
                        std::lock_guard<std::mutex> lk(sSet1BirthMutex);
                        for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
                        {
                            sSet1BirthLedger[(U64)e.sets[i]].freed_frame = sMonotonicFrameCount;
                        }
                    }
                }
            }
            else
            {
                if (w != r)
                {
                    lane.deferred_free[w] = e;
                }
                ++w;
            }
        }
        lane.deferred_free.resize(w);
    }
}

void writeCurrentPerFrameMatrixUBO(const PerFrameMatrixUBO& data, const TextureMatrixUBO& texdata)
{
    if (!sInitialized || sFrameIndex >= FRAMES_IN_FLIGHT)
    {
        return;
    }
    const U32 f = sFrameIndex;

    const bool changed = !sMatrixRingHasCurrent[f] ||
                         std::memcmp(sMatrixRingCurrentProj[f], data.projection_matrix,
                                     sizeof(data.projection_matrix)) != 0 ||
                         std::memcmp(sMatrixRingCurrentTexmat[f], texdata.texture_matrix,
                                     sizeof(texdata.texture_matrix)) != 0;
    if (!changed)
    {
        return;
    }

    const U32 slot = sMatrixRingUsedThisFrame[f].fetch_add(1, std::memory_order_relaxed);
    if (!ensurePerFrameMatrixSlot(f, slot))
    {
        return;
    }

    const U32 chunk  = slot / MATRIX_RING_CHUNK_SLOTS;
    const U32 within = slot % MATRIX_RING_CHUNK_SLOTS;
    U8* slot_ptr = static_cast<U8*>(sMatrixRingChunks[f][chunk].mapped) + (size_t)within * MATRIX_RING_SLOT_SIZE;
    std::memcpy(slot_ptr,                             &data,    sizeof(PerFrameMatrixUBO));
    std::memcpy(slot_ptr + (size_t)PERFRAME_UBO_SIZE, &texdata, sizeof(TextureMatrixUBO));

    sMatrixRingCurrentSlot[f] = slot;
    std::memcpy(sMatrixRingCurrentProj[f],   data.projection_matrix, sizeof(data.projection_matrix));
    std::memcpy(sMatrixRingCurrentTexmat[f], texdata.texture_matrix, sizeof(texdata.texture_matrix));
    sMatrixRingHasCurrent[f] = true;

}

void writeCurrentPreviewAmbientUBO(const PreviewAmbient_PerShaderBind& data)
{
    if (!sInitialized || sPerFrameUboMapped[sFrameIndex] == nullptr)
    {
        return;
    }
    std::memcpy(static_cast<U8*>(sPerFrameUboMapped[sFrameIndex]) + PREVIEWAMBIENT_UBO_OFFSET,
                &data,
                sizeof(PreviewAmbient_PerShaderBind));

}

void writeCurrentStarTimeUBO(const StarTime_PerShaderBind& data)
{
    if (!sInitialized || sPerFrameUboMapped[sFrameIndex] == nullptr)
    {
        return;
    }
    std::memcpy(static_cast<U8*>(sPerFrameUboMapped[sFrameIndex]) + STARTIME_UBO_OFFSET,
                &data,
                sizeof(StarTime_PerShaderBind));

}

void writeCurrentGlowCombineUBO(const GlowCombine_PerShaderBind& data)
{
    if (!sInitialized || sPerFrameUboMapped[sFrameIndex] == nullptr)
    {
        return;
    }
    std::memcpy(static_cast<U8*>(sPerFrameUboMapped[sFrameIndex]) + GLOWCOMBINE_UBO_OFFSET,
                &data,
                sizeof(GlowCombine_PerShaderBind));

}

bool createPerProgramUBOVk(U32       size_bytes,
                           VkBuffer& out_buffer,
                           void*&    out_allocation,
                           void**    out_mapped)
{
    return createBufferVkImpl(size_bytes,
                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              out_buffer, out_allocation, out_mapped);
}

static bool ensurePerDrawUBOArenaCurrent(PerDrawUBOArena& a)
{
    if (a.frame != sMonotonicFrameCount)
    {
        a.frame  = sMonotonicFrameCount;
        a.cursor = 0;
        if (!a.overflow.empty())
        {
            for (PerDrawUBOOverflowBlock& ob : a.overflow)
            {
                destroyBufferVk(ob.buffer, ob.allocation);
            }
            a.overflow.clear();
            ++gVkPerDrawTopologyGen;
        }
        if (a.buffer == VK_NULL_HANDLE || a.pending_capacity > a.capacity)
        {
            VkDeviceSize want = llmax(a.pending_capacity, PER_DRAW_UBO_ARENA_INITIAL);
            if (a.buffer != VK_NULL_HANDLE)
            {
                destroyBufferVk(a.buffer, a.allocation);
                a.buffer     = VK_NULL_HANDLE;
                a.allocation = nullptr;
                a.mapped     = nullptr;
                a.capacity   = 0;
            }
            void* mapped = nullptr;
            if (createBufferVkImpl((U32)want,
                                   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   a.buffer, a.allocation, &mapped))
            {
                a.mapped   = mapped;
                a.capacity = want;
            }
            a.pending_capacity = 0;
            ++gVkPerDrawTopologyGen;
        }
    }
    return (a.buffer != VK_NULL_HANDLE && a.mapped != nullptr);
}

void tickPerDrawUBOArena()
{
    if (!sInitialized)
    {
        return;
    }
    const U32 f = sFrameIndex;
    if (f >= FRAMES_IN_FLIGHT)
    {
        return;
    }
    ensurePerDrawUBOArenaCurrent(sPerDrawUBOArena[f]);
}

bool allocPerDrawUBOSlice(U32 size_bytes, VkBuffer& out_buffer, U32& out_offset, void*& out_mapped)
{
    out_buffer = VK_NULL_HANDLE;
    out_offset = 0;
    out_mapped = nullptr;
    if (!sInitialized || size_bytes == 0)
    {
        return false;
    }
    const U32 f = sFrameIndex;
    if (f >= FRAMES_IN_FLIGHT)
    {
        LLVKContract::cause(LLVKContract::C_UBO_SLICE_FAIL);
        return false;
    }
    PerDrawUBOArena& a = sPerDrawUBOArena[f];
    if (!ensurePerDrawUBOArenaCurrent(a))
    {
        LLVKContract::cause(LLVKContract::C_UBO_SLICE_FAIL);
        return false;
    }
    VkDeviceSize align = sPhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
    if (align < 16)
    {
        align = 16;
    }
    VkDeviceSize expected = a.cursor.load(std::memory_order_relaxed);
    VkDeviceSize off;
    for (;;)
    {
        off = (expected + align - 1) & ~(align - 1);
        const VkDeviceSize next = off + size_bytes;
        if (next > a.capacity)
        {
            std::lock_guard<std::mutex> lk(sPerDrawArenaGrowthMutex);
            a.pending_capacity = llmax(a.pending_capacity, llmax(a.capacity * 2, next));
            if (!a.overflow.empty())
            {
                PerDrawUBOOverflowBlock& blk = a.overflow.back();
                VkDeviceSize boff = (blk.cursor + align - 1) & ~(align - 1);
                if (boff + size_bytes <= blk.capacity)
                {
                    blk.cursor = boff + size_bytes;
                    out_buffer = blk.buffer;
                    out_offset = (U32)boff;
                    out_mapped = (U8*)blk.mapped + boff;
                    return true;
                }
            }
            PerDrawUBOOverflowBlock nb;
            VkDeviceSize want = llmax((VkDeviceSize)size_bytes,
                                      llmax(a.capacity, PER_DRAW_UBO_ARENA_INITIAL));
            void* nb_mapped = nullptr;
            if (!createBufferVkImpl((U32)want,
                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                    nb.buffer, nb.allocation, &nb_mapped))
            {
                LLVKContract::cause(LLVKContract::C_UBO_SLICE_FAIL);
                return false;
            }
            nb.mapped   = nb_mapped;
            nb.capacity = want;
            nb.cursor   = size_bytes;
            a.overflow.push_back(nb);
            out_buffer = nb.buffer;
            out_offset = 0;
            out_mapped = (U8*)nb_mapped;
            return true;
        }
        if (a.cursor.compare_exchange_weak(expected, next, std::memory_order_relaxed))
        {
            break;
        }
    }
    out_buffer = a.buffer;
    out_offset = (U32)off;
    out_mapped = (U8*)a.mapped + off;
    return true;
}

VkBuffer getPerDrawUBOArenaBuffer()
{
    if (!sInitialized)
    {
        return VK_NULL_HANDLE;
    }
    const U32 f = sFrameIndex;
    if (f >= FRAMES_IN_FLIGHT)
    {
        return VK_NULL_HANDLE;
    }
    PerDrawUBOArena& a = sPerDrawUBOArena[f];
    if (!ensurePerDrawUBOArenaCurrent(a))
    {
        return VK_NULL_HANDLE;
    }
    return a.buffer;
}

void ensurePerAssetUBOVk(U32       needed_size,
                         VkBuffer& inout_buffer,
                         void*&    inout_allocation,
                         void*&    inout_mapped,
                         U32&      inout_size)
{
    if (!sInitialized)
    {
        return;
    }
    if (needed_size == inout_size && inout_buffer != VK_NULL_HANDLE)
    {
        return;
    }
    if (inout_buffer != VK_NULL_HANDLE)
    {
        destroyBufferVk(inout_buffer, inout_allocation);
        inout_buffer     = VK_NULL_HANDLE;
        inout_allocation = nullptr;
        inout_mapped     = nullptr;
        inout_size       = 0;
    }
    if (needed_size == 0)
    {
        return;
    }
    void* mapped_ptr = nullptr;
    if (createBufferVkImpl(needed_size,
                           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                           inout_buffer,
                           inout_allocation,
                           &mapped_ptr))
    {
        inout_mapped = mapped_ptr;
        inout_size   = needed_size;
    }
}

#define LLVK_SHARED_UBO_LATCHED_IMPL(BindName, StructType)                                              \
    static StructType sLatched##BindName##Shadow;                                                       \
    static U64      sLatched##BindName##Gen = 1;                                                        \
    static U64      sLatched##BindName##SlotGen[FRAMES_IN_FLIGHT] = { 0, 0, 0 };                        \
    static VkBuffer sLatched##BindName##Buf[FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE }; \
    static void*    sLatched##BindName##Alloc[FRAMES_IN_FLIGHT]  = { nullptr, nullptr, nullptr };       \
    static void*    sLatched##BindName##Mapped[FRAMES_IN_FLIGHT] = { nullptr, nullptr, nullptr };       \
    void writeCurrent##BindName##UBO(const StructType& data)                                            \
    {                                                                                                  \
        sLatched##BindName##Shadow = data;                                                              \
        ++sLatched##BindName##Gen;                                                                      \
    }                                                                                                  \
    bool getShared##BindName##UBO(VkBuffer& out_buffer, void*& out_mapped)                              \
    {                                                                                                  \
        if (!sInitialized) return false;                                                               \
        const U32 f = sFrameIndex;                                                                     \
        if (f >= FRAMES_IN_FLIGHT) return false;                                                        \
        if (sLatched##BindName##Buf[f] == VK_NULL_HANDLE)                                              \
        {                                                                                              \
            if (!createBufferVkImpl((U32)sizeof(StructType),                                            \
                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,                                 \
                                    sLatched##BindName##Buf[f],                                         \
                                    sLatched##BindName##Alloc[f],                                       \
                                    &sLatched##BindName##Mapped[f]))                                    \
            {                                                                                          \
                return false;                                                                          \
            }                                                                                          \
        }                                                                                              \
        if (sLatched##BindName##SlotGen[f] != sLatched##BindName##Gen)                                 \
        {                                                                                              \
            std::memcpy(sLatched##BindName##Mapped[f], &sLatched##BindName##Shadow,                     \
                        sizeof(StructType));                                                            \
            sLatched##BindName##SlotGen[f] = sLatched##BindName##Gen;                                  \
        }                                                                                              \
        out_buffer = sLatched##BindName##Buf[f];                                                        \
        out_mapped = sLatched##BindName##Mapped[f];                                                     \
        return true;                                                                                   \
    }

LLVK_SHARED_UBO_LATCHED_IMPL(WindlightHDR,      WindlightHDR_PerProgramBind)
LLVK_SHARED_UBO_LATCHED_IMPL(WindlightLight,    WindlightLight_PerProgramBind)
LLVK_SHARED_UBO_LATCHED_IMPL(TonemapUtilF,       TonemapUtilF_PerProgramBind)
LLVK_SHARED_UBO_LATCHED_IMPL(SMAABlendWeightsF, SMAABlendWeightsF_PerProgramBind)

#undef LLVK_SHARED_UBO_LATCHED_IMPL

void teardownSharedLatchedUBOs()
{
    auto destroy_latched = [&](VkBuffer* bufs, void** allocs, void** mappeds)
    {
        for (U32 f = 0; f < FRAMES_IN_FLIGHT; ++f)
        {
            if (bufs[f] != VK_NULL_HANDLE && sAllocator != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(sAllocator, bufs[f], reinterpret_cast<VmaAllocation>(allocs[f]));
            }
            bufs[f]    = VK_NULL_HANDLE;
            allocs[f]  = nullptr;
            mappeds[f] = nullptr;
        }
    };
    destroy_latched(sLatchedWindlightHDRBuf,      sLatchedWindlightHDRAlloc,      sLatchedWindlightHDRMapped);
    destroy_latched(sLatchedWindlightLightBuf,    sLatchedWindlightLightAlloc,    sLatchedWindlightLightMapped);
    destroy_latched(sLatchedTonemapUtilFBuf,       sLatchedTonemapUtilFAlloc,       sLatchedTonemapUtilFMapped);
    destroy_latched(sLatchedSMAABlendWeightsFBuf, sLatchedSMAABlendWeightsFAlloc, sLatchedSMAABlendWeightsFMapped);
}

static bool ensureShadowUtilRingSlot(U32 f, U32 idx)
{
    while (sShadowUtilRing[f].size() <= (size_t)idx)
    {
        DeferredUtilOverrideSlot slot;
        if (!createPerProgramUBOVk((U32)sizeof(ShadowUtil_PerProgramBind),
                                   slot.buffer, slot.allocation, &slot.mapped))
        {
            return false;
        }
        sShadowUtilRing[f].push_back(slot);
    }
    return true;
}

void writeCurrentShadowUtilUBO(const ShadowUtil_PerProgramBind& data)
{
    if (!sInitialized)
    {
        return;
    }
    const U32 f = sFrameIndex;
    if (f >= FRAMES_IN_FLIGHT)
    {
        return;
    }
    if (sShadowUtilRingFrame[f] != sMonotonicFrameCount)
    {
        sShadowUtilRingFrame[f] = sMonotonicFrameCount;
        sShadowUtilRingIdx[f]   = 0;
    }
    const U32 idx = sShadowUtilRingIdx[f];
    if (!ensureShadowUtilRingSlot(f, idx))
    {
        return;
    }
    sShadowUtilRingIdx[f] = idx + 1;
    if ((U64)(idx + 1) > gVkPerf.ring_hw[RINGHW_SHADOWUTIL].load())
        gVkPerf.ring_hw[RINGHW_SHADOWUTIL] = (U64)(idx + 1);
    DeferredUtilOverrideSlot& slot = sShadowUtilRing[f][idx];
    std::memcpy(slot.mapped, &data, sizeof(ShadowUtil_PerProgramBind));
    sCurShadowUtilBuf[f]    = slot.buffer;
    sCurShadowUtilMapped[f] = slot.mapped;

}

bool getSharedShadowUtilUBO(VkBuffer& out_buffer, void*& out_mapped)
{
    if (!sInitialized)
    {
        return false;
    }
    const U32 f = sFrameIndex;
    if (f >= FRAMES_IN_FLIGHT)
    {
        return false;
    }
    if (sShadowUtilRingFrame[f] != sMonotonicFrameCount)
    {
        sShadowUtilRingFrame[f] = sMonotonicFrameCount;
        sShadowUtilRingIdx[f]   = 0;
        sCurShadowUtilBuf[f]    = VK_NULL_HANDLE;
        sCurShadowUtilMapped[f] = nullptr;
    }
    if (sCurShadowUtilBuf[f] == VK_NULL_HANDLE)
    {
        if (!ensureShadowUtilRingSlot(f, 0))
        {
            return false;
        }
        sCurShadowUtilBuf[f]    = sShadowUtilRing[f][0].buffer;
        sCurShadowUtilMapped[f] = sShadowUtilRing[f][0].mapped;
    }
    out_buffer = sCurShadowUtilBuf[f];
    out_mapped = sCurShadowUtilMapped[f];
    return true;
}

static bool ensureDeferredUtilRingSlot(U32 f, U32 idx)
{
    while (sDeferredUtilRing[f].size() <= (size_t)idx)
    {
        DeferredUtilOverrideSlot slot;
        if (!createPerProgramUBOVk((U32)sizeof(DeferredUtil_PerProgramBind),
                                   slot.buffer, slot.allocation, &slot.mapped))
        {
            return false;
        }
        sDeferredUtilRing[f].push_back(slot);
    }
    return true;
}

void writeCurrentDeferredUtilUBO(const DeferredUtil_PerProgramBind& data)
{
    if (!sInitialized)
    {
        return;
    }
    const U32 f = sFrameIndex;
    if (f >= FRAMES_IN_FLIGHT)
    {
        return;
    }
    if (sDeferredUtilRingFrame[f] != sMonotonicFrameCount)
    {
        sDeferredUtilRingFrame[f] = sMonotonicFrameCount;
        sDeferredUtilRingIdx[f]   = 0;
    }
    const U32 idx = sDeferredUtilRingIdx[f];
    if (!ensureDeferredUtilRingSlot(f, idx))
    {
        return;
    }
    sDeferredUtilRingIdx[f] = idx + 1;
    if ((U64)(idx + 1) > gVkPerf.ring_hw[RINGHW_DEFERREDUTIL].load())
        gVkPerf.ring_hw[RINGHW_DEFERREDUTIL] = (U64)(idx + 1);
    DeferredUtilOverrideSlot& slot = sDeferredUtilRing[f][idx];
    std::memcpy(slot.mapped, &data, sizeof(DeferredUtil_PerProgramBind));
    sCurDeferredUtilBuf[f]    = slot.buffer;
    sCurDeferredUtilMapped[f] = slot.mapped;

}

bool getSharedDeferredUtilUBO(VkBuffer& out_buffer, void*& out_mapped)
{
    if (!sInitialized)
    {
        return false;
    }
    const U32 f = sFrameIndex;
    if (f >= FRAMES_IN_FLIGHT)
    {
        return false;
    }
    if (sDuOverrideActiveBuf != VK_NULL_HANDLE)
    {
        out_buffer = sDuOverrideActiveBuf;
        out_mapped = sDuOverrideActiveMapped;
        return true;
    }
    if (sDeferredUtilRingFrame[f] != sMonotonicFrameCount)
    {
        sDeferredUtilRingFrame[f] = sMonotonicFrameCount;
        sDeferredUtilRingIdx[f]   = 0;
        sCurDeferredUtilBuf[f]    = VK_NULL_HANDLE;
        sCurDeferredUtilMapped[f] = nullptr;
    }
    if (sCurDeferredUtilBuf[f] == VK_NULL_HANDLE)
    {
        if (!ensureDeferredUtilRingSlot(f, 0))
        {
            return false;
        }
        sCurDeferredUtilBuf[f]    = sDeferredUtilRing[f][0].buffer;
        sCurDeferredUtilMapped[f] = sDeferredUtilRing[f][0].mapped;
    }
    out_buffer = sCurDeferredUtilBuf[f];
    out_mapped = sCurDeferredUtilMapped[f];
    return true;
}

void setDeferredUtilOverrideSlot(VkBuffer buf, void* mapped)
{
    sDuOverrideActiveBuf    = buf;
    sDuOverrideActiveMapped = mapped;
}
void clearDeferredUtilOverrideSlot()
{
    sDuOverrideActiveBuf    = VK_NULL_HANDLE;
    sDuOverrideActiveMapped = nullptr;
}

#define LLVK_SHARED_UBO_RING_IMPL(BindName, StructType, BindingNumber, RingHwEnum)                       \
    static bool ensure##BindName##RingSlot(U32 f, U32 idx)                                              \
    {                                                                                                  \
        while (s##BindName##Ring[f].size() <= (size_t)idx)                                             \
        {                                                                                              \
            DeferredUtilOverrideSlot slot;                                                              \
            if (!createPerProgramUBOVk((U32)sizeof(StructType), slot.buffer, slot.allocation, &slot.mapped)) \
            {                                                                                          \
                return false;                                                                          \
            }                                                                                          \
            s##BindName##Ring[f].push_back(slot);                                                       \
        }                                                                                              \
        return true;                                                                                   \
    }                                                                                                  \
    void writeCurrent##BindName##UBO(const StructType& data)                                           \
    {                                                                                                  \
        if (!sInitialized) return;                                                                     \
        const U32 f = sFrameIndex;                                                                     \
        if (f >= FRAMES_IN_FLIGHT) return;                                                             \
        if (s##BindName##RingFrame[f] != sMonotonicFrameCount)                                         \
        {                                                                                              \
            s##BindName##RingFrame[f] = sMonotonicFrameCount;                                          \
            s##BindName##RingIdx[f]   = 0;                                                             \
        }                                                                                              \
        const U32 idx = s##BindName##RingIdx[f];                                                       \
        if (!ensure##BindName##RingSlot(f, idx)) return;                                               \
        s##BindName##RingIdx[f] = idx + 1;                                                             \
        if ((U64)(idx + 1) > gVkPerf.ring_hw[RingHwEnum].load())                                       \
            gVkPerf.ring_hw[RingHwEnum] = (U64)(idx + 1);                                              \
        DeferredUtilOverrideSlot& slot = s##BindName##Ring[f][idx];                                    \
        std::memcpy(slot.mapped, &data, sizeof(StructType));                                           \
        sCur##BindName##Buf[f]    = slot.buffer;                                                       \
        sCur##BindName##Mapped[f] = slot.mapped;                                                       \
    }                                                                                                  \
    bool getShared##BindName##UBO(VkBuffer& out_buffer, void*& out_mapped)                             \
    {                                                                                                  \
        if (!sInitialized) return false;                                                               \
        const U32 f = sFrameIndex;                                                                     \
        if (f >= FRAMES_IN_FLIGHT) return false;                                                        \
        if (s##BindName##RingFrame[f] != sMonotonicFrameCount)                                         \
        {                                                                                              \
            s##BindName##RingFrame[f] = sMonotonicFrameCount;                                          \
            s##BindName##RingIdx[f]   = 0;                                                             \
            sCur##BindName##Buf[f]    = VK_NULL_HANDLE;                                                 \
            sCur##BindName##Mapped[f] = nullptr;                                                       \
        }                                                                                              \
        if (sCur##BindName##Buf[f] == VK_NULL_HANDLE)                                                  \
        {                                                                                              \
            if (!ensure##BindName##RingSlot(f, 0)) return false;                                       \
            sCur##BindName##Buf[f]    = s##BindName##Ring[f][0].buffer;                                \
            sCur##BindName##Mapped[f] = s##BindName##Ring[f][0].mapped;                                \
        }                                                                                              \
        out_buffer = sCur##BindName##Buf[f];                                                            \
        out_mapped = sCur##BindName##Mapped[f];                                                        \
        return true;                                                                                   \
    }

LLVK_SHARED_UBO_RING_IMPL(WindlightSky,     WindlightSky_PerProgramBind,     9,  RINGHW_WLSKY)
LLVK_SHARED_UBO_RING_IMPL(WindlightAtmos,   WindlightAtmos_PerProgramBind,   8,  RINGHW_WLATMOS)
LLVK_SHARED_UBO_RING_IMPL(AoUtil,           AoUtil_PerProgramBind,           22, RINGHW_AOUTIL)
LLVK_SHARED_UBO_RING_IMPL(GlobalF,          GlobalF_PerProgramBind,          18, RINGHW_GLOBALF)
LLVK_SHARED_UBO_RING_IMPL(WaterFog,         WaterFog_PerProgramBind,         14, RINGHW_WATERFOG)
LLVK_SHARED_UBO_RING_IMPL(WaterV,           Water_PerProgramBind,            15, RINGHW_WATERV)
LLVK_SHARED_UBO_RING_IMPL(ReflectionProbe,  ReflectionProbe_PerProgramBind,  16, RINGHW_RP)
LLVK_SHARED_UBO_RING_IMPL(ReflectionProbes, ReflectionProbes_PerProgramBind, 38, RINGHW_RPS)
LLVK_SHARED_UBO_RING_IMPL(Lights,           Lights_PerProgramBind,           12, RINGHW_LIGHTS)
LLVK_SHARED_UBO_RING_IMPL(LightsSpecular,   LightsSpecular_PerProgramBind,   12, RINGHW_LIGHTSSPEC)
LLVK_SHARED_UBO_RING_IMPL(PbrTerrainF,      PbrTerrainF_PerProgramBind,      28, RINGHW_PBRTERRAINF)
LLVK_SHARED_UBO_RING_IMPL(PbrTerrain,       PbrTerrain_PerShaderBind,        52, RINGHW_PBRTERRAIN)
#undef LLVK_SHARED_UBO_RING_IMPL

#define LLVK_SHARED_UBO_DYNAMIC_IMPL(BindName, StructType)                                              \
    static thread_local StructType s##BindName##Shadow;                                                 \
    static thread_local U64 s##BindName##WriteGen = 1;                                                  \
    static thread_local U64 s##BindName##UpFrame[FRAMES_IN_FLIGHT]  = { ~0ull, ~0ull, ~0ull };          \
    static thread_local U64 s##BindName##UpGen[FRAMES_IN_FLIGHT]    = { 0, 0, 0 };                      \
    static thread_local U32 s##BindName##UpOffset[FRAMES_IN_FLIGHT] = { 0, 0, 0 };                      \
    static thread_local VkBuffer s##BindName##UpBuf[FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE }; \
    static thread_local U64 s##BindName##UpHash[FRAMES_IN_FLIGHT] = { 0, 0, 0 };                        \
    static bool peek##BindName##State(const void*& out_shadow, U32& out_size,                           \
                                      U32& out_off, bool& out_current, U64& out_up_hash)                \
    {                                                                                                  \
        const U32 f = sFrameIndex;                                                                      \
        if (f >= FRAMES_IN_FLIGHT)                                                                      \
        {                                                                                              \
            return false;                                                                               \
        }                                                                                              \
        out_shadow  = &s##BindName##Shadow;                                                             \
        out_size    = (U32)sizeof(StructType);                                                          \
        out_off     = s##BindName##UpOffset[f];                                                         \
        out_up_hash = s##BindName##UpHash[f];                                                           \
        out_current = (s##BindName##UpFrame[f] == sMonotonicFrameCount                                  \
                       && s##BindName##UpGen[f] == s##BindName##WriteGen);                              \
        return true;                                                                                   \
    }                                                                                                  \
    void writeCurrent##BindName##UBO(const StructType& data)                                            \
    {                                                                                                  \
        s##BindName##Shadow = data;                                                                     \
        ++s##BindName##WriteGen;                                                                        \
        LLGLSLShader::sCurPerCallVkOffsetsDirty = true;                                                 \
    }                                                                                                  \
    static bool ensure##BindName##Uploaded(VkBuffer& out_buf, U32& out_off)                             \
    {                                                                                                  \
        out_buf = VK_NULL_HANDLE;                                                                       \
        out_off = 0;                                                                                    \
        if (!sInitialized)                                                                              \
        {                                                                                              \
            return false;                                                                               \
        }                                                                                              \
        const U32 f = sFrameIndex;                                                                      \
        if (f >= FRAMES_IN_FLIGHT)                                                                      \
        {                                                                                              \
            return false;                                                                               \
        }                                                                                              \
        if (s##BindName##UpFrame[f] != sMonotonicFrameCount                                             \
            || s##BindName##UpGen[f] != s##BindName##WriteGen)                                          \
        {                                                                                              \
            VkBuffer b = VK_NULL_HANDLE;                                                                \
            U32      o = 0;                                                                             \
            void*    p = nullptr;                                                                       \
            if (!allocPerDrawUBOSlice((U32)sizeof(StructType), b, o, p))                                \
            {                                                                                          \
                return false;                                                                           \
            }                                                                                          \
            std::memcpy(p, &s##BindName##Shadow, sizeof(StructType));                                   \
            s##BindName##UpBuf[f]    = b;                                                               \
            s##BindName##UpHash[f]   = LLVKContract::verboseEnabled()                                    \
                ? sharedUBOContentHash(&s##BindName##Shadow, (U32)sizeof(StructType)) : 0;              \
            s##BindName##UpOffset[f] = o;                                                               \
            s##BindName##UpFrame[f]  = sMonotonicFrameCount;                                            \
            s##BindName##UpGen[f]    = s##BindName##WriteGen;                                           \
        }                                                                                              \
        out_buf = s##BindName##UpBuf[f];                                                                \
        out_off = s##BindName##UpOffset[f];                                                             \
        return true;                                                                                   \
    }                                                                                                  \
    bool getShared##BindName##UBO(VkBuffer& out_buffer, void*& out_mapped)                              \
    {                                                                                                  \
        out_buffer = VK_NULL_HANDLE;                                                                    \
        out_mapped = &s##BindName##Shadow;                                                              \
        return true;                                                                                   \
    }
LLVK_SHARED_UBO_DYNAMIC_IMPL(AvatarSkin,       AvatarSkin_PerProgramBind)
LLVK_SHARED_UBO_DYNAMIC_IMPL(PBRMaterial,      PBRMaterial_PerMaterial)
LLVK_SHARED_UBO_DYNAMIC_IMPL(DrawColor,        DrawColor_PerShaderBind)
LLVK_SHARED_UBO_DYNAMIC_IMPL(ShadowParams,     ShadowParams_PerShaderBind)
LLVK_SHARED_UBO_DYNAMIC_IMPL(ShadowViewProj,   ShadowViewProj_PerPass)
#undef LLVK_SHARED_UBO_DYNAMIC_IMPL

static constexpr U32 kSharedUBOPersistentInitialWrites = 256;
#define LLVK_SHARED_UBO_DYNAMIC_PERSISTENT_IMPL(BindName, StructType)                                    \
    static StructType s##BindName##Shadow;                                                              \
    static U64 s##BindName##WriteGen = 1;                                                               \
    static U64 s##BindName##UpFrame[FRAMES_IN_FLIGHT]  = { ~0ull, ~0ull, ~0ull };                       \
    static U64 s##BindName##UpGen[FRAMES_IN_FLIGHT]    = { 0, 0, 0 };                                   \
    static U32 s##BindName##UpOffset[FRAMES_IN_FLIGHT] = { 0, 0, 0 };                                   \
    static std::atomic<U32> s##BindName##Cursor[FRAMES_IN_FLIGHT];                                      \
    static std::atomic<U32> s##BindName##Wanted[FRAMES_IN_FLIGHT];                                      \
    static U32 s##BindName##SliceStride = 0;                                                            \
    static U32 s##BindName##Capacity[FRAMES_IN_FLIGHT] = { 0, 0, 0 };                                   \
    static VkBuffer s##BindName##UpBuf[FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE }; \
    static U64 s##BindName##UpHash[FRAMES_IN_FLIGHT] = { 0, 0, 0 };                                     \
    static VkBuffer s##BindName##PersistBuf[FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE }; \
    static void* s##BindName##PersistAlloc[FRAMES_IN_FLIGHT]  = { nullptr, nullptr, nullptr };          \
    static void* s##BindName##PersistMapped[FRAMES_IN_FLIGHT] = { nullptr, nullptr, nullptr };          \
    static bool create##BindName##PersistentBuffers()                                                   \
    {                                                                                                  \
        VkDeviceSize align = sPhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment;          \
        if (align < 16) { align = 16; }                                                                 \
        const U32 stride = (U32)(((VkDeviceSize)sizeof(StructType) + align - 1) & ~(align - 1));         \
        s##BindName##SliceStride = stride;                                                              \
        for (U32 f = 0; f < FRAMES_IN_FLIGHT; ++f)                                                      \
        {                                                                                              \
            void* mapped = nullptr;                                                                     \
            const U32 cap = stride * kSharedUBOPersistentInitialWrites;                                 \
            if (!createBufferVkImpl(cap,                                                                \
                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,                                 \
                                    s##BindName##PersistBuf[f],                                         \
                                    s##BindName##PersistAlloc[f],                                       \
                                    &mapped))                                                           \
            {                                                                                          \
                return false;                                                                           \
            }                                                                                          \
            s##BindName##PersistMapped[f] = mapped;                                                     \
            s##BindName##Capacity[f]      = cap;                                                        \
            s##BindName##Cursor[f].store(0);                                                            \
            s##BindName##Wanted[f].store(0);                                                            \
        }                                                                                              \
        return true;                                                                                    \
    }                                                                                                  \
    static void destroy##BindName##PersistentBuffers()                                                  \
    {                                                                                                  \
        for (U32 f = 0; f < FRAMES_IN_FLIGHT; ++f)                                                      \
        {                                                                                              \
            if (s##BindName##PersistBuf[f] != VK_NULL_HANDLE)                                           \
            {                                                                                          \
                destroyBufferVk(s##BindName##PersistBuf[f], s##BindName##PersistAlloc[f]);              \
            }                                                                                          \
            s##BindName##PersistBuf[f]    = VK_NULL_HANDLE;                                             \
            s##BindName##PersistAlloc[f]  = nullptr;                                                    \
            s##BindName##PersistMapped[f] = nullptr;                                                    \
            s##BindName##Capacity[f]      = 0;                                                          \
            s##BindName##UpBuf[f]         = VK_NULL_HANDLE;                                             \
            s##BindName##UpFrame[f]       = ~0ull;                                                      \
        }                                                                                              \
    }                                                                                                  \
    static void tick##BindName##Persistent()                                                           \
    {                                                                                                  \
        if (!sInitialized) return;                                                                     \
        const U32 f = sFrameIndex;                                                                      \
        if (f >= FRAMES_IN_FLIGHT) return;                                                             \
        const U32 wanted = s##BindName##Wanted[f].load();                                               \
        if (wanted > s##BindName##Capacity[f] && s##BindName##PersistBuf[f] != VK_NULL_HANDLE)          \
        {                                                                                              \
            const U32 want = wanted * 2;                                                                \
            VkBuffer nb = VK_NULL_HANDLE; void* na = nullptr; void* nm = nullptr;                        \
            if (createBufferVkImpl(want, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, nb, na, &nm))              \
            {                                                                                          \
                destroyBufferVk(s##BindName##PersistBuf[f], s##BindName##PersistAlloc[f]);              \
                s##BindName##PersistBuf[f]    = nb;                                                     \
                s##BindName##PersistAlloc[f]  = na;                                                     \
                s##BindName##PersistMapped[f] = nm;                                                     \
                s##BindName##Capacity[f]      = want;                                                   \
                ++gVkPerDrawTopologyGen;                                                                \
                LL_INFOS("Vulkan") << "grew persistent dynamic UBO " #BindName                          \
                                   << " frame-slot " << f << " to " << want << " bytes" << LL_ENDL;     \
            }                                                                                          \
        }                                                                                              \
        s##BindName##Cursor[f].store(0);                                                                \
        s##BindName##Wanted[f].store(0);                                                                \
    }                                                                                                  \
    static bool peek##BindName##State(const void*& out_shadow, U32& out_size,                           \
                                      U32& out_off, bool& out_current, U64& out_up_hash)                \
    {                                                                                                  \
        const U32 f = sFrameIndex;                                                                      \
        if (f >= FRAMES_IN_FLIGHT)                                                                      \
        {                                                                                              \
            return false;                                                                               \
        }                                                                                              \
        out_shadow  = &s##BindName##Shadow;                                                             \
        out_size    = (U32)sizeof(StructType);                                                          \
        out_off     = s##BindName##UpOffset[f];                                                         \
        out_up_hash = s##BindName##UpHash[f];                                                           \
        out_current = (s##BindName##UpFrame[f] == sMonotonicFrameCount                                  \
                       && s##BindName##UpGen[f] == s##BindName##WriteGen);                              \
        return true;                                                                                   \
    }                                                                                                  \
    void writeCurrent##BindName##UBO(const StructType& data)                                            \
    {                                                                                                  \
        if (std::memcmp(&s##BindName##Shadow, &data, sizeof(StructType)) != 0)                           \
        {                                                                                              \
            s##BindName##Shadow = data;                                                                 \
            ++s##BindName##WriteGen;                                                                    \
            LLGLSLShader::sCurPerCallVkOffsetsDirty = true;                                             \
        }                                                                                              \
    }                                                                                                  \
    static bool ensure##BindName##Uploaded(VkBuffer& out_buf, U32& out_off)                             \
    {                                                                                                  \
        out_buf = VK_NULL_HANDLE;                                                                       \
        out_off = 0;                                                                                    \
        if (!sInitialized)                                                                              \
        {                                                                                              \
            return false;                                                                               \
        }                                                                                              \
        const U32 f = sFrameIndex;                                                                      \
        if (f >= FRAMES_IN_FLIGHT || s##BindName##PersistBuf[f] == VK_NULL_HANDLE)                      \
        {                                                                                              \
            return false;                                                                               \
        }                                                                                              \
        if (s##BindName##UpFrame[f] != sMonotonicFrameCount                                             \
            || s##BindName##UpGen[f] != s##BindName##WriteGen)                                          \
        {                                                                                              \
            const U32 stride = s##BindName##SliceStride;                                                \
            U32 off = s##BindName##Cursor[f].fetch_add(stride);                                         \
            const U32 needed = off + stride;                                                            \
            U32 prev = s##BindName##Wanted[f].load(std::memory_order_relaxed);                          \
            while (needed > prev                                                                        \
                   && !s##BindName##Wanted[f].compare_exchange_weak(prev, needed,                       \
                                                                    std::memory_order_relaxed)) {}      \
            if (needed > s##BindName##Capacity[f])                                                      \
            {                                                                                          \
                off = s##BindName##Capacity[f] - stride;                                                \
            }                                                                                          \
            std::memcpy((U8*)s##BindName##PersistMapped[f] + off,                                        \
                        &s##BindName##Shadow, sizeof(StructType));                                       \
            s##BindName##UpBuf[f]    = s##BindName##PersistBuf[f];                                      \
            s##BindName##UpHash[f]   = LLVKContract::verboseEnabled()                                    \
                ? sharedUBOContentHash(&s##BindName##Shadow, (U32)sizeof(StructType)) : 0;              \
            s##BindName##UpOffset[f] = off;                                                             \
            s##BindName##UpFrame[f]  = sMonotonicFrameCount;                                            \
            s##BindName##UpGen[f]    = s##BindName##WriteGen;                                           \
        }                                                                                              \
        out_buf = s##BindName##UpBuf[f];                                                                \
        out_off = s##BindName##UpOffset[f];                                                             \
        return true;                                                                                   \
    }                                                                                                  \
    bool getShared##BindName##UBO(VkBuffer& out_buffer, void*& out_mapped)                              \
    {                                                                                                  \
        out_buffer = VK_NULL_HANDLE;                                                                    \
        out_mapped = &s##BindName##Shadow;                                                              \
        return true;                                                                                   \
    }
LLVK_SHARED_UBO_DYNAMIC_PERSISTENT_IMPL(ReflectionProbeF, ReflectionProbeF_PerProgramBind)
LLVK_SHARED_UBO_DYNAMIC_PERSISTENT_IMPL(SSRUtil,          SSRUtil_PerProgramBind)
#undef LLVK_SHARED_UBO_DYNAMIC_PERSISTENT_IMPL

bool initSharedDynamicPersistentUBOs()
{
    return createReflectionProbeFPersistentBuffers()
        && createSSRUtilPersistentBuffers();
}

void teardownSharedDynamicPersistentUBOs()
{
    destroyReflectionProbeFPersistentBuffers();
    destroySSRUtilPersistentBuffers();
}

void tickSharedDynamicPersistentUBOs()
{
    tickReflectionProbeFPersistent();
    tickSSRUtilPersistent();
}

static thread_local ObjectSkin_PerProgramBind sObjectSkinShadow;
static thread_local U64 sObjectSkinWriteGen = 1;
static thread_local U64 sObjectSkinUpFrame[FRAMES_IN_FLIGHT]  = { ~0ull, ~0ull, ~0ull };
static thread_local U64 sObjectSkinUpGen[FRAMES_IN_FLIGHT]    = { 0, 0, 0 };
static thread_local U32 sObjectSkinUpOffset[FRAMES_IN_FLIGHT] = { 0, 0, 0 };
static thread_local VkBuffer sObjectSkinUpBuf[FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
static thread_local U64 sObjectSkinUpHash[FRAMES_IN_FLIGHT] = { 0, 0, 0 };

struct ObjectSkinFrameCacheKey
{
    const void* avatar;
    U64         skin_hash;
    bool operator==(const ObjectSkinFrameCacheKey& o) const
    {
        return avatar == o.avatar && skin_hash == o.skin_hash;
    }
};
struct ObjectSkinFrameCacheKeyHash
{
    std::size_t operator()(const ObjectSkinFrameCacheKey& k) const
    {
        std::size_t h = std::hash<const void*>()(k.avatar);
        h ^= std::hash<U64>()(k.skin_hash) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        return h;
    }
};
struct ObjectSkinFrameCacheVal
{
    VkBuffer buf;
    U32      off;
    U32      skin_entry; // B.0: bindless skin palette entry (BINDLESS_INVALID_SLOT if unfilled)
};
static std::unordered_map<ObjectSkinFrameCacheKey, ObjectSkinFrameCacheVal, ObjectSkinFrameCacheKeyHash> sObjectSkinFrameCache;
static U64 sObjectSkinFrameCacheStamp = ~0ull;
static std::mutex sObjectSkinFrameCacheMutex;

static void objectSkinFrameCacheGuardLocked()
{
    if (sObjectSkinFrameCacheStamp != sMonotonicFrameCount)
    {
        sObjectSkinFrameCache.clear();
        sObjectSkinFrameCacheStamp = sMonotonicFrameCount;
    }
}

// B.0: copy the current skin shadow (10560 B) into the frame's bindless palette
// region and return its absolute entry index. Parallel to the dynamic-UBO upload;
// does NOT affect draws (shader still reads the UBO). Overflow -> INVALID (harmless).
U32 skinBindlessStorePalette(const void* shadow_10560)
{
    if (sSkinPaletteMapped == nullptr || shadow_10560 == nullptr)
    {
        return BINDLESS_INVALID_SLOT;
    }
    const U32 f = sFrameIndex;
    if (f >= FRAMES_IN_FLIGHT)
    {
        return BINDLESS_INVALID_SLOT;
    }
    const U32 local = sSkinPaletteCursor[f].fetch_add(1, std::memory_order_relaxed);
    if (local >= SKIN_ENTRIES_PER_FRAME)
    {
        gVkPerf.skin_bl_of.fetch_add(1, std::memory_order_relaxed);
        return BINDLESS_INVALID_SLOT;
    }
    const U32 entry = f * SKIN_ENTRIES_PER_FRAME + local;
    std::memcpy(sSkinPaletteMapped + (size_t)entry * SKIN_PALETTE_ENTRY_BYTES,
                shadow_10560, SKIN_PALETTE_ENTRY_BYTES);
    gVkPerf.skin_bl_fill.fetch_add(1, std::memory_order_relaxed);
    return entry;
}

static bool peekObjectSkinState(const void*& out_shadow, U32& out_size,
                                U32& out_off, bool& out_current, U64& out_up_hash)
{
    const U32 f = sFrameIndex;
    if (f >= FRAMES_IN_FLIGHT)
    {
        return false;
    }
    out_shadow  = &sObjectSkinShadow;
    out_size    = (U32)sizeof(ObjectSkin_PerProgramBind);
    out_off     = sObjectSkinUpOffset[f];
    out_up_hash = sObjectSkinUpHash[f];
    out_current = (sObjectSkinUpFrame[f] == sMonotonicFrameCount
                   && sObjectSkinUpGen[f] == sObjectSkinWriteGen);
    return true;
}

static bool ensureObjectSkinUploaded(VkBuffer& out_buf, U32& out_off)
{
    out_buf = VK_NULL_HANDLE;
    out_off = 0;
    if (!sInitialized)
    {
        return false;
    }
    const U32 f = sFrameIndex;
    if (f >= FRAMES_IN_FLIGHT)
    {
        return false;
    }
    if (sObjectSkinUpFrame[f] != sMonotonicFrameCount || sObjectSkinUpGen[f] != sObjectSkinWriteGen)
    {
        VkBuffer b = VK_NULL_HANDLE;
        U32      o = 0;
        void*    p = nullptr;
        if (!allocPerDrawUBOSlice((U32)sizeof(ObjectSkin_PerProgramBind), b, o, p))
        {
            return false;
        }
        std::memcpy(p, &sObjectSkinShadow, sizeof(ObjectSkin_PerProgramBind));
        gVkPerf.skin_up.fetch_add(1, std::memory_order_relaxed);
        sObjectSkinUpBuf[f]    = b;
        sObjectSkinUpHash[f]   = LLVKContract::verboseEnabled()
            ? sharedUBOContentHash(&sObjectSkinShadow, (U32)sizeof(ObjectSkin_PerProgramBind)) : 0;
        sObjectSkinUpOffset[f] = o;
        sObjectSkinUpFrame[f]  = sMonotonicFrameCount;
        sObjectSkinUpGen[f]    = sObjectSkinWriteGen;
    }
    out_buf = sObjectSkinUpBuf[f];
    out_off = sObjectSkinUpOffset[f];
    return true;
}

bool getSharedDynamicUBOForBinding(U32 binding, VkBuffer& out_buf, U32& out_off)
{
    switch (binding)
    {
        case 39: return ensureReflectionProbeFUploaded(out_buf, out_off);
        case 45: return ensureAvatarSkinUploaded(out_buf, out_off);
        case 46: return ensureObjectSkinUploaded(out_buf, out_off);
        case 48: return ensurePBRMaterialUploaded(out_buf, out_off);
        case 49: return ensureSSRUtilUploaded(out_buf, out_off);
        case 51: return ensureDrawColorUploaded(out_buf, out_off);
        case 53: return ensureShadowParamsUploaded(out_buf, out_off);
        case 54: return ensureShadowViewProjUploaded(out_buf, out_off);
        default: return false;
    }
}

U64 sharedUBOContentHash(const void* p, U32 n)
{
    const U8* b = (const U8*)p;
    U64 h = 0xcbf29ce484222325ull;
    while (n >= 8)
    {
        U64 v;
        std::memcpy(&v, b, 8);
        h = (h ^ v) * 0x100000001b3ull;
        b += 8;
        n -= 8;
    }
    while (n--)
    {
        h = (h ^ *b++) * 0x100000001b3ull;
    }
    return h;
}

bool peekSharedDynamicUBO(U32 binding, const void*& out_shadow, U32& out_size,
                          U32& out_off, bool& out_current, U64& out_up_hash)
{
    switch (binding)
    {
        case 39: return peekReflectionProbeFState(out_shadow, out_size, out_off, out_current, out_up_hash);
        case 45: return peekAvatarSkinState(out_shadow, out_size, out_off, out_current, out_up_hash);
        case 46: return peekObjectSkinState(out_shadow, out_size, out_off, out_current, out_up_hash);
        case 48: return peekPBRMaterialState(out_shadow, out_size, out_off, out_current, out_up_hash);
        case 49: return peekSSRUtilState(out_shadow, out_size, out_off, out_current, out_up_hash);
        case 51: return peekDrawColorState(out_shadow, out_size, out_off, out_current, out_up_hash);
        case 53: return peekShadowParamsState(out_shadow, out_size, out_off, out_current, out_up_hash);
        case 54: return peekShadowViewProjState(out_shadow, out_size, out_off, out_current, out_up_hash);
        default: return false;
    }
}

void* rotateObjectSkinSlotForWrite()
{
    if (!sInitialized) return nullptr;
    ++sObjectSkinWriteGen;
    LLGLSLShader::sCurPerCallVkOffsetsDirty = true;
    return &sObjectSkinShadow;
}

bool objectSkinTryAdopt(const void* avatar, U64 skin_hash)
{
    if (!sInitialized)
    {
        return false;
    }
    if (LLVKContract::verboseEnabled())
    {
        return false;
    }
    const U32 f = sFrameIndex;
    if (f >= FRAMES_IN_FLIGHT)
    {
        return false;
    }
    ObjectSkinFrameCacheVal val;
    {
        std::lock_guard<std::mutex> lk(sObjectSkinFrameCacheMutex);
        objectSkinFrameCacheGuardLocked();
        auto it = sObjectSkinFrameCache.find(ObjectSkinFrameCacheKey{ avatar, skin_hash });
        if (it == sObjectSkinFrameCache.end())
        {
            return false;
        }
        val = it->second;
    }
    sObjectSkinUpBuf[f]    = val.buf;
    sObjectSkinUpOffset[f] = val.off;
    sObjectSkinUpFrame[f]  = sMonotonicFrameCount;
    ++sObjectSkinWriteGen;
    sObjectSkinUpGen[f]    = sObjectSkinWriteGen;
    sObjectSkinUpHash[f]   = 0;
    LLGLSLShader::sCurPerCallVkOffsetsDirty = true;
    return true;
}

void objectSkinStoreCache(const void* avatar, U64 skin_hash)
{
    if (!sInitialized)
    {
        return;
    }
    const U32 f = sFrameIndex;
    if (f >= FRAMES_IN_FLIGHT)
    {
        return;
    }
    VkBuffer buf = VK_NULL_HANDLE;
    U32      off = 0;
    const bool current = (sObjectSkinUpFrame[f] == sMonotonicFrameCount
                          && sObjectSkinUpGen[f] == sObjectSkinWriteGen);
    if (current)
    {
        buf = sObjectSkinUpBuf[f];
        off = sObjectSkinUpOffset[f];
    }
    else if (!ensureObjectSkinUploaded(buf, off))
    {
        return;
    }
    // B.0: parallel-fill the bindless palette from the same shadow the UBO used.
    // One fill per unique (avatar,skin_hash) this frame == skin_up (dedup-consistent).
    const U32 skin_entry = skinBindlessStorePalette(&sObjectSkinShadow);
    {
        std::lock_guard<std::mutex> lk(sObjectSkinFrameCacheMutex);
        objectSkinFrameCacheGuardLocked();
        sObjectSkinFrameCache[ObjectSkinFrameCacheKey{ avatar, skin_hash }] =
            ObjectSkinFrameCacheVal{ buf, off, skin_entry };
    }
}

// B.2: look up this frame's bindless palette entry for (avatar, skin_hash).
// Returns BINDLESS_INVALID_SLOT if not filled this frame (draw then falls back to UBO).
U32 objectSkinLookupEntry(const void* avatar, U64 skin_hash)
{
    if (!sInitialized)
    {
        return BINDLESS_INVALID_SLOT;
    }
    std::lock_guard<std::mutex> lk(sObjectSkinFrameCacheMutex);
    objectSkinFrameCacheGuardLocked();
    auto it = sObjectSkinFrameCache.find(ObjectSkinFrameCacheKey{ avatar, skin_hash });
    return (it != sObjectSkinFrameCache.end()) ? it->second.skin_entry : BINDLESS_INVALID_SLOT;
}

// B.2: write the per-draw skin base index at the DrawData slot the shader reads as
// gl_InstanceIndex. Kill switch (or unfilled palette) -> INVALID, forcing UBO fallback.
void writeDrawSkinBase(U32 draw_id, U32 skin_entry)
{
    if (sSkinBaseMapped == nullptr || draw_id == BINDLESS_INVALID_SLOT || draw_id >= DRAWDATA_TOTAL_SLOTS)
    {
        return;
    }
    const U32 f = (sFrameIndex < FRAMES_IN_FLIGHT) ? sFrameIndex : 0;
    const U32 v = sSkinBindlessEnabled ? skin_entry : BINDLESS_INVALID_SLOT;
    sSkinBaseMapped[(size_t)f * DRAWDATA_TOTAL_SLOTS + draw_id] = v;
    if (v != BINDLESS_INVALID_SLOT)
    {
        gVkPerf.skin_base_wr.fetch_add(1, std::memory_order_relaxed);
    }
}

U32 skinBaseDynamicOffsetBytes()
{
    const U32 f = (sFrameIndex < FRAMES_IN_FLIGHT) ? sFrameIndex : 0;
    return f * DRAWDATA_TOTAL_SLOTS * 4u;
}

bool getSharedObjectSkinUBO(VkBuffer& out_buffer, void*& out_mapped)
{
    if (!sInitialized) return false;
    out_buffer = VK_NULL_HANDLE;
    out_mapped = &sObjectSkinShadow;
    return true;
}

bool acquireDeferredUtilOverrideSlot(VkBuffer& out_buf, void*& out_mapped)
{
    if (!isVulkanInitialized())
    {
        return false;
    }
    const U32 f = sFrameIndex;
    if (f >= FRAMES_IN_FLIGHT)
    {
        return false;
    }
    VkBuffer  frame_buf    = VK_NULL_HANDLE;
    void*     frame_mapped = nullptr;
    if (!getSharedDeferredUtilUBO(frame_buf, frame_mapped) || frame_mapped == nullptr)
    {
        return false;
    }
    if (sDuOverrideFrame[f] != sMonotonicFrameCount)
    {
        sDuOverrideFrame[f] = sMonotonicFrameCount;
        sDuOverrideIdx[f]   = 0;
    }
    const U32 idx = sDuOverrideIdx[f];
    while (sDuOverrideRing[f].size() <= (size_t)idx)
    {
        DeferredUtilOverrideSlot slot;
        if (!createPerProgramUBOVk((U32)sizeof(DeferredUtil_PerProgramBind),
                                   slot.buffer, slot.allocation, &slot.mapped))
        {
            return false;
        }
        sDuOverrideRing[f].push_back(slot);
    }
    sDuOverrideIdx[f] = idx + 1;
    DeferredUtilOverrideSlot& slot = sDuOverrideRing[f][idx];
    std::memcpy(slot.mapped, frame_mapped, sizeof(DeferredUtil_PerProgramBind));
    out_buf    = slot.buffer;
    out_mapped = slot.mapped;
    return true;
}

void purgePerDrawDeadHandles(const std::unordered_set<U64>& dead_views,
                                    const std::unordered_set<U64>& dead_bufs)
{
    if (dead_views.empty() && dead_bufs.empty())
    {
        return;
    }
    if (!dead_views.empty())
    {
        LLGLSLShader::purgePerDrawPinsForDeadViews(dead_views);
    }
    bool pinned_leftover = false;
    for (int pass = 0; pass < 2; ++pass)
    {
        pinned_leftover = false;
        for (PerDrawDescLane& lane : sPerDrawDescLanes)
        {
            auto purge_index = [&](auto& index, const std::unordered_set<U64>& dead)
            {
                for (U64 h : dead)
                {
                    auto mit = index.find(h);
                    if (mit == index.end())
                    {
                        continue;
                    }
                    std::vector<ScenePerDrawCacheKey> keys(mit->second.begin(), mit->second.end());
                    for (const ScenePerDrawCacheKey& k : keys)
                    {
                        auto cit = lane.cache.find(k);
                        if (cit == lane.cache.end())
                        {
                            continue;
                        }
                        if (cit->second.refs.load(std::memory_order_relaxed) > 0)
                        {
                            pinned_leftover = true;
                            continue;
                        }
                        ScenePerDrawDeferredFreeEntry deferred = {};
                        for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
                        {
                            deferred.sets[i] = cit->second.sets[i];
                        }
                        deferred.pool_index    = cit->second.pool_index;
                        deferred.enqueue_frame = sMonotonicFrameCount;
                        lane.deferred_free.push_back(deferred);
                        lane.lru.erase(cit->second.lru_pos);
                        laneUnindexEntry(lane, cit->second);
                        lane.cache.erase(cit);
                        ++gVkPerf.set1_dead_purge;
                    }
                }
            };
            purge_index(lane.by_view, dead_views);
            purge_index(lane.by_buf, dead_bufs);
        }
        if (!pinned_leftover)
        {
            break;
        }
        if (pass == 0)
        {
            LLGLSLShader::purgeAllPerDrawPins();
        }
    }
    if (pinned_leftover)
    {
        LL_WARNS_ONCE("Vulkan") << "per-draw cache purge: pinned entry with dead handle survived full pin purge" << LL_ENDL;
    }
}

VkDescriptorSet getCurrentPerFrameDescriptorSet()
{
    if (sFrameIndex >= FRAMES_IN_FLIGHT)
    {
        return VK_NULL_HANDLE;
    }
    const U32 f = sFrameIndex;
    const U32 count = sPerFrameRingSetCount[f].load(std::memory_order_acquire);
    if (count == 0)
    {
        return VK_NULL_HANDLE;
    }
    U32 slot = sMatrixRingHasCurrent[f] ? sMatrixRingCurrentSlot[f] : 0;
    if (slot >= count)
    {
        slot = 0;
    }
    return sPerFrameRingSets[f][slot];
}

bool allocDomainSelfTest()
{
    if (sDrawDataMapped == nullptr || sMegaTypeSizes.empty())
    {
        LL_WARNS("Vulkan") << "allocSelfTest skip: allocators not ready" << LL_ENDL;
        return false;
    }
    U32 idA = createRenderDomain();
    U32 idB = createRenderDomain();
    AllocDomain* sdA = sAllocDomains[idA];
    AllocDomain* sdB = sAllocDomains[idB];
    constexpr U32 ITER = 3000;
    std::atomic<U32> fails{0};
    auto run = [&](AllocDomain* sd)
    {
        tAllocDomain = sd;
        std::vector<U32> slots;
        std::vector<MegaSliceV> vslices;
        std::vector<MegaSliceI> islices;
        for (U32 i = 0; i < ITER; ++i)
        {
            U32 dd[DRAWDATA_SLOT_UINTS] = { sd->mId, i, 0xA5A5A5A5u, i * 7u + 1u };
            U32 s = drawDataAcquireSlot(dd);
            if (s == BINDLESS_INVALID_SLOT)
            {
                fails.fetch_add(1);
            }
            else
            {
                if (allocDomainForSlot(s) != sd)
                {
                    fails.fetch_add(1);
                }
                slots.push_back(s);
            }
            if ((i & 3u) == 0u)
            {
                MegaSliceV mv;
                if (megabufAcquireVertex(0x1u, 48, mv))
                {
                    vslices.push_back(mv);
                }
                MegaSliceI mi;
                if (megabufAcquireIndex(96, mi))
                {
                    islices.push_back(mi);
                }
            }
            if ((i & 15u) == 15u && !slots.empty())
            {
                drawDataReleaseSlotDeferred(slots.back());
                slots.pop_back();
            }
        }
        for (U32 s : slots)
        {
            drawDataReleaseSlotDeferred(s);
        }
        for (const MegaSliceV& mv : vslices)
        {
            megabufReleaseVertex(mv);
        }
        for (const MegaSliceI& mi : islices)
        {
            megabufReleaseIndex(mi);
        }
    };
    std::thread ta([&]{ run(sdA); });
    std::thread tb([&]{ run(sdB); });
    ta.join();
    tb.join();
    renderDomainReclaim(idA);
    renderDomainReclaim(idB);
    const U32 f = fails.load();
    LL_INFOS("Vulkan") << "allocSelfTest domA=" << sdA->mId << " domB=" << sdB->mId
                       << " iters=" << ITER << " routing_fails=" << f
                       << (f == 0u ? " OK" : " CORRUPTION") << LL_ENDL;
    return f == 0u;
}

static thread_local U32 tDrawDataScratchMemoFrame   = 0xFFFFFFFFu;
static thread_local U32 tDrawDataScratchMemoSlot    = 0;
static thread_local U32 tDrawDataScratchMemoVals[DRAWDATA_SLOT_UINTS] = {};

U32 drawDataWriteScratch(const U32* slots4)
{
    if (sDrawDataMapped == nullptr)
    {
        return 0;
    }
    if (tDrawDataScratchMemoFrame == sMonotonicFrameCount
        && std::memcmp(tDrawDataScratchMemoVals, slots4, DRAWDATA_SLOT_UINTS * 4) == 0)
    {
        return tDrawDataScratchMemoSlot;
    }
    U32 local = sDrawDataScratchCursor.fetch_add(1, std::memory_order_relaxed);
    if (local >= DRAWDATA_SCRATCH_PER_FRAME)
    {
        LLVKContract::cause(LLVKContract::C_DRAWDATA_SCRATCH_WRAP);
        static std::atomic<bool> warned{false};
        if (!warned.exchange(true))
        {
            LL_WARNS("Vulkan") << "VKBindless: DrawData scratch wrapped" << LL_ENDL;
        }
        local %= DRAWDATA_SCRATCH_PER_FRAME;
    }
    const U32 region = (sFrameIndex < FRAMES_IN_FLIGHT) ? sFrameIndex : 0;
    const U32 slot = DRAWDATA_PERSISTENT_SLOTS + region * DRAWDATA_SCRATCH_PER_FRAME + local;
    std::memcpy(sDrawDataMapped + (size_t)slot * DRAWDATA_SLOT_UINTS, slots4, DRAWDATA_SLOT_UINTS * 4);
    tDrawDataScratchMemoFrame = sMonotonicFrameCount;
    tDrawDataScratchMemoSlot  = slot;
    std::memcpy(tDrawDataScratchMemoVals, slots4, DRAWDATA_SLOT_UINTS * 4);
    return slot;
}

U32 publishDrawSkinBase(U32 draw_id, const void* avatar, U64 skin_hash)
{
    const U32 skin_entry = (avatar != nullptr)
                               ? objectSkinLookupEntry(avatar, skin_hash)
                               : BINDLESS_INVALID_SLOT;
    writeDrawSkinBase(draw_id, skin_entry);
    return skin_entry;
}

void commitPerDrawID(U32 id, bool publish_skin, const void* avatar, U64 skin_hash)
{
    const U32 draw_id = (id == BINDLESS_INVALID_SLOT) ? 0 : id;
    LLVKContract::stashDrawDataID(draw_id);
    LLVKContract::markPerDrawIDCommitted();
    if (publish_skin)
    {
        publishDrawSkinBase(draw_id, avatar, skin_hash);
    }
}

} // namespace LLVKLoader
