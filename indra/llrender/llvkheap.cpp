/**
* @file llvkheap.cpp
* @brief AYAstorm r42 Vulkan loader — bindless heap / set1 birth ledger / dead handle tracking
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

    VkDescriptorSetLayout    sBindlessHeapLayout                     = VK_NULL_HANDLE;
    VkDescriptorPool         sBindlessHeapPool                       = VK_NULL_HANDLE;
    U32                      sBindlessHeapCount                      = 0;
    VkDescriptorSetLayout    sSkinBaseLayout                         = VK_NULL_HANDLE;
    VkDescriptorPool         sSkinBasePool                           = VK_NULL_HANDLE;
    VkDescriptorSetLayout    sEmptySetLayout                         = VK_NULL_HANDLE;
    VkDescriptorPool         sEmptySetPool                           = VK_NULL_HANDLE;
    U32                      sBindlessSlotNext                       = 1;
    std::vector<VkImageView> sBindlessSlotView;   // β: slot→現ディスクリプタ view の CPU shadow(sBindlessSlotMutex 下で書く)
    bool                     sBindlessActive                         = false;
    VkBuffer                 sDrawDataBuffer                         = VK_NULL_HANDLE;
    void*                    sDrawDataAllocation                     = nullptr;
    VkBuffer                 sSkinPaletteBuffer                      = VK_NULL_HANDLE;
    void*                    sSkinPaletteAllocation                  = nullptr;
    VkBuffer                 sSkinBaseBuffer                         = VK_NULL_HANDLE;
    void*                    sSkinBaseAllocation                     = nullptr;
} // namespace

namespace LLVKLoaderInternal
{
    std::unordered_map<U64, Set1BirthInfo> sSet1BirthLedger;
    std::mutex sSet1BirthMutex;

    std::unordered_set<U64> sDeadViewHandles;
    std::unordered_set<U64> sDeadBufferHandles;
    std::mutex              sDeadHandleMutex;

    std::mutex sVvlCountMutex;
    std::unordered_map<S32, std::pair<std::string, U64>> sVvlCounts;

    void noteViewHandleCreated(VkImageView v)
    {
        if (v == VK_NULL_HANDLE) return;
        std::lock_guard<std::mutex> lk(sDeadHandleMutex);
        sDeadViewHandles.erase((U64)v);
    }

    void noteBufferHandleCreated(VkBuffer b)
    {
        if (b == VK_NULL_HANDLE) return;
        std::lock_guard<std::mutex> lk(sDeadHandleMutex);
        sDeadBufferHandles.erase((U64)b);
    }

    std::string hex64(U64 v)
    {
        char buf[20];
        snprintf(buf, sizeof(buf), "%llx", (unsigned long long)v);
        return std::string(buf);
    }

    std::string set1BirthLookup(const char* msg)
    {
        if (msg == nullptr || std::strstr(msg, "08114") == nullptr)
        {
            return std::string();
        }
        const char* p = std::strstr(msg, "VkDescriptorSet 0x");
        if (p == nullptr)
        {
            return " | SETBIRTH no-handle";
        }
        const U64 h = std::strtoull(p + 16, nullptr, 16);
        std::lock_guard<std::mutex> lk(sSet1BirthMutex);
        auto it = sSet1BirthLedger.find(h);
        if (it == sSet1BirthLedger.end())
        {
            return " | SETBIRTH unknown-set";
        }
        std::string out = " | SETBIRTH path=" + std::to_string(it->second.path)
             + " shader=" + it->second.shader
             + " gen=" + std::to_string(it->second.gen)
             + " bindframe=" + std::to_string(it->second.frame)
             + " buildframe=" + std::to_string(it->second.build_frame)
             + " freedframe=" + std::to_string(it->second.freed_frame);
        const char* bp = std::strstr(msg, ", binding ");
        if (bp != nullptr)
        {
            const U32 bind_no = (U32)std::strtoul(bp + 10, nullptr, 10);
            const std::string tag = "b" + std::to_string(bind_no) + "=";
            const size_t pos = it->second.contents.find(tag);
            if (pos == std::string::npos)
            {
                out += " | b" + std::to_string(bind_no) + "=UNWRITTEN contents{" + it->second.contents + "}";
            }
            else
            {
                const size_t tok_end = it->second.contents.find(',', pos);
                out += " | " + it->second.contents.substr(pos, (tok_end == std::string::npos)
                                                                   ? std::string::npos
                                                                   : tok_end - pos);
                const U64 vh = std::strtoull(it->second.contents.c_str() + pos + tag.size(), nullptr, 16);
                auto dit = sViewDeathLedger.find(vh);
                if (dit != sViewDeathLedger.end())
                {
                    out += " enq@" + std::to_string(dit->second.enqueue_frame)
                         + " dead@" + std::to_string(dit->second.destroy_frame)
                         + " enqfrom=0x" + hex64(dit->second.enqueue_retaddr)
                         + " anchor=0x" + hex64((U64)(uintptr_t)&isVulkanInitialized);
                }
                else
                {
                    out += " no-death-record";
                }
            }
        }
        return out;
    }

    void bindlessWriteSlotInternal(U32 slot, VkImageView view, VkSampler sampler)
    {
        if (!sBindlessActive || slot >= sBindlessHeapCount || sDevice == VK_NULL_HANDLE)
        {
            return;
        }
        if (view == VK_NULL_HANDLE)
        {
            view = sDefaultFallbackImageView;
        }
        if (sampler == VK_NULL_HANDLE)
        {
            sampler = sStandardLinearSampler;
        }
        VkDescriptorImageInfo ii = {};
        ii.sampler     = sampler;
        ii.imageView   = view;
        ii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkWriteDescriptorSet w = {};
        w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet          = sBindlessHeapSet;
        w.dstBinding      = 1;
        w.dstArrayElement = slot;
        w.descriptorCount = 1;
        w.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        w.pImageInfo      = &ii;
        vkUpdateDescriptorSets(sDevice, 1, &w, 0, nullptr);
        if (slot < sBindlessSlotView.size())
        {
            sBindlessSlotView[slot] = view;   // β shadow = ディスクリプタと同一(NULL→fallback 置換後)
        }
    }

    void destroyBindlessHeap()
    {
        if (sDevice == VK_NULL_HANDLE)
        {
            return;
        }
        if (sDrawDataBuffer != VK_NULL_HANDLE)
        {
            destroyBufferVk(sDrawDataBuffer, sDrawDataAllocation);
            sDrawDataBuffer     = VK_NULL_HANDLE;
            sDrawDataAllocation = nullptr;
            sDrawDataMapped     = nullptr;
        }
        if (sSkinPaletteBuffer != VK_NULL_HANDLE)
        {
            destroyBufferVk(sSkinPaletteBuffer, sSkinPaletteAllocation);
            sSkinPaletteBuffer     = VK_NULL_HANDLE;
            sSkinPaletteAllocation = nullptr;
            sSkinPaletteMapped     = nullptr;
        }
        if (sSkinBaseBuffer != VK_NULL_HANDLE)
        {
            destroyBufferVk(sSkinBaseBuffer, sSkinBaseAllocation);
            sSkinBaseBuffer     = VK_NULL_HANDLE;
            sSkinBaseAllocation = nullptr;
            sSkinBaseMapped     = nullptr;
        }
        {
            std::lock_guard<std::mutex> lk(sAllocGrowthMutex);
            sSlotSlabNextIdx = 0;
            for (U32 i = 0; i < DRAWDATA_MAX_SLABS; ++i)
            {
                sSlotSlabOwner[i].store(0xFFFFFFFFu, std::memory_order_relaxed);
            }
            for (AllocDomain* d : sAllocDomains)
            {
                d->mSlotNext = 0;
                d->mSlotEnd  = 0;
                d->mSlotFree.clear();
                std::lock_guard<std::mutex> pl(d->mPendMutex);
                d->mPendSlot.clear();
            }
        }
        if (sBindlessHeapPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(sDevice, sBindlessHeapPool, nullptr);
            sBindlessHeapPool = VK_NULL_HANDLE;
        }
        if (sBindlessHeapLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(sDevice, sBindlessHeapLayout, nullptr);
            sBindlessHeapLayout = VK_NULL_HANDLE;
        }
        if (sSkinBasePool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(sDevice, sSkinBasePool, nullptr);
            sSkinBasePool = VK_NULL_HANDLE;
        }
        if (sSkinBaseLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(sDevice, sSkinBaseLayout, nullptr);
            sSkinBaseLayout = VK_NULL_HANDLE;
        }
        if (sEmptySetPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(sDevice, sEmptySetPool, nullptr);
            sEmptySetPool = VK_NULL_HANDLE;
        }
        if (sEmptySetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(sDevice, sEmptySetLayout, nullptr);
            sEmptySetLayout = VK_NULL_HANDLE;
        }
        sEmptySet          = VK_NULL_HANDLE;
        sSkinBaseSet       = VK_NULL_HANDLE;
        sBindlessHeapSet   = VK_NULL_HANDLE;
        sBindlessHeapCount = 0;
        sBindlessSlotNext  = 1;
        {
            std::lock_guard<std::mutex> guard(sBindlessSlotMutex);
            sBindlessSlotFreeList.clear();
            sPendingSlotFrees.clear();
            sBindlessSlotView.clear();
        }
        sBindlessActive = false;
    }

    bool createBindlessHeap()
    {
        sBindlessActive = false;
        {
            const char* e = getenv("AYASTORM_SKIN_BINDLESS"); // B.2 kill switch: 0 -> rigged skin stays on dynamic UBO
            sSkinBindlessEnabled = !(e != nullptr && e[0] == '0');
        }
        if (!sBindlessCapable || sBindlessHeapCapacity == 0)
        {
            return true;
        }

        const U32 count = sBindlessHeapCapacity;

        VkDescriptorSetLayoutBinding bindings[2] = {};
        bindings[0].binding         = 0;
        bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[1].binding         = 1;
        bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].descriptorCount = count;
        bindings[1].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorBindingFlags bind_flags[2] = {
            0,
              VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
            | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
            | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT
        };

        VkDescriptorSetLayoutBindingFlagsCreateInfo bf = {};
        bf.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        bf.bindingCount  = 2;
        bf.pBindingFlags = bind_flags;

        VkDescriptorSetLayoutCreateInfo li = {};
        li.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        li.pNext        = &bf;
        li.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        li.bindingCount = 2;
        li.pBindings    = bindings;

        if (vkCreateDescriptorSetLayout(sDevice, &li, nullptr, &sBindlessHeapLayout) != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "VKBindless: heap layout creation failed (staying inactive)" << LL_ENDL;
            sBindlessHeapLayout = VK_NULL_HANDLE;
            return true;
        }

        VkDescriptorPoolSize ps[2] = {};
        ps[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        ps[0].descriptorCount = 1; // DrawData(0)
        ps[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        ps[1].descriptorCount = count;

        VkDescriptorPoolCreateInfo pi = {};
        pi.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pi.flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        pi.maxSets       = 1;
        pi.poolSizeCount = 2;
        pi.pPoolSizes    = ps;

        if (vkCreateDescriptorPool(sDevice, &pi, nullptr, &sBindlessHeapPool) != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "VKBindless: heap pool creation failed (staying inactive)" << LL_ENDL;
            destroyBindlessHeap();
            return true;
        }

        {
            VkDescriptorSetLayoutBinding sb[2] = {};
            sb[0].binding         = 0;
            sb[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            sb[0].descriptorCount = 1;
            sb[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
            sb[1].binding         = 1;
            sb[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            sb[1].descriptorCount = 1;
            sb[1].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
            VkDescriptorSetLayoutCreateInfo sli = {};
            sli.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            sli.bindingCount = 2;
            sli.pBindings    = sb;
            if (vkCreateDescriptorSetLayout(sDevice, &sli, nullptr, &sSkinBaseLayout) != VK_SUCCESS)
            {
                LL_WARNS("Vulkan") << "VKBindless: skin base layout creation failed (staying inactive)" << LL_ENDL;
                destroyBindlessHeap();
                return true;
            }
            VkDescriptorPoolSize sps[2] = {};
            sps[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            sps[0].descriptorCount = 1;
            sps[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            sps[1].descriptorCount = 1;
            VkDescriptorPoolCreateInfo spi = {};
            spi.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            spi.maxSets       = 1;
            spi.poolSizeCount = 2;
            spi.pPoolSizes    = sps;
            if (vkCreateDescriptorPool(sDevice, &spi, nullptr, &sSkinBasePool) != VK_SUCCESS)
            {
                LL_WARNS("Vulkan") << "VKBindless: skin base pool creation failed (staying inactive)" << LL_ENDL;
                destroyBindlessHeap();
                return true;
            }
            VkDescriptorSetAllocateInfo sai = {};
            sai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            sai.descriptorPool     = sSkinBasePool;
            sai.descriptorSetCount = 1;
            sai.pSetLayouts        = &sSkinBaseLayout;
            if (vkAllocateDescriptorSets(sDevice, &sai, &sSkinBaseSet) != VK_SUCCESS)
            {
                LL_WARNS("Vulkan") << "VKBindless: skin base set allocation failed (staying inactive)" << LL_ENDL;
                destroyBindlessHeap();
                return true;
            }
        }

        // Fixed-size allocation: binding 1 gets its full `count` descriptors
        // (fixed descriptorCount, not VARIABLE_DESCRIPTOR_COUNT -> no variable-count alloc info).
        VkDescriptorSetAllocateInfo ai = {};
        ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.pNext              = nullptr;
        ai.descriptorPool     = sBindlessHeapPool;
        ai.descriptorSetCount = 1;
        ai.pSetLayouts        = &sBindlessHeapLayout;

        if (vkAllocateDescriptorSets(sDevice, &ai, &sBindlessHeapSet) != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "VKBindless: heap set allocation failed (staying inactive)" << LL_ENDL;
            destroyBindlessHeap();
            return true;
        }

        {
            VkDescriptorSetLayoutCreateInfo eli = {};
            eli.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            eli.bindingCount = 0;
            eli.pBindings    = nullptr;
            if (vkCreateDescriptorSetLayout(sDevice, &eli, nullptr, &sEmptySetLayout) != VK_SUCCESS)
            {
                LL_WARNS("Vulkan") << "VKBindless: empty set layout creation failed (staying inactive)" << LL_ENDL;
                destroyBindlessHeap();
                return true;
            }
            VkDescriptorPoolSize eps = {};
            eps.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            eps.descriptorCount = 1;
            VkDescriptorPoolCreateInfo epi = {};
            epi.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            epi.maxSets       = 1;
            epi.poolSizeCount = 1;
            epi.pPoolSizes    = &eps;
            if (vkCreateDescriptorPool(sDevice, &epi, nullptr, &sEmptySetPool) != VK_SUCCESS)
            {
                LL_WARNS("Vulkan") << "VKBindless: empty set pool creation failed (staying inactive)" << LL_ENDL;
                destroyBindlessHeap();
                return true;
            }
            VkDescriptorSetAllocateInfo eai = {};
            eai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            eai.pNext              = nullptr;
            eai.descriptorPool     = sEmptySetPool;
            eai.descriptorSetCount = 1;
            eai.pSetLayouts        = &sEmptySetLayout;
            if (vkAllocateDescriptorSets(sDevice, &eai, &sEmptySet) != VK_SUCCESS)
            {
                LL_WARNS("Vulkan") << "VKBindless: empty set allocation failed (staying inactive)" << LL_ENDL;
                destroyBindlessHeap();
                return true;
            }
        }

        {
            void* dd_mapped = nullptr;
            if (createBufferVkImpl(DRAWDATA_TOTAL_SLOTS * DRAWDATA_SLOT_UINTS * 4,
                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                   sDrawDataBuffer, sDrawDataAllocation, &dd_mapped, true)
                && dd_mapped != nullptr)
            {
                sDrawDataMapped = reinterpret_cast<U32*>(dd_mapped);
                std::memset(sDrawDataMapped, 0, DRAWDATA_SLOT_UINTS * 4);

                VkDescriptorBufferInfo bi = {};
                bi.buffer = sDrawDataBuffer;
                bi.offset = 0;
                bi.range  = VK_WHOLE_SIZE;
                VkWriteDescriptorSet w = {};
                w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                w.dstSet          = sBindlessHeapSet;
                w.dstBinding      = 0;
                w.dstArrayElement = 0;
                w.descriptorCount = 1;
                w.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                w.pBufferInfo     = &bi;
                vkUpdateDescriptorSets(sDevice, 1, &w, 0, nullptr);
            }
            else
            {
                LL_WARNS("Vulkan") << "VKBindless: DrawData buffer creation failed (heap inactive)" << LL_ENDL;
                destroyBindlessHeap();
                return true;
            }
        }

        // B.0/B.2: skin base SSBO (set=3 b0) + skin palette SSBO (set=3 b1).
        // Bound once (persistent set); palette parallel-filled beside the live dynamic-UBO path.
        {
            const U64 palette_bytes = (U64)SKIN_PALETTE_ENTRY_BYTES * SKIN_ENTRIES_PER_FRAME * FRAMES_IN_FLIGHT;
            const U64 base_bytes    = (U64)DRAWDATA_TOTAL_SLOTS * 4 * FRAMES_IN_FLIGHT;
            void* pal_mapped  = nullptr;
            void* base_mapped = nullptr;
            if (createBufferVkImpl((U32)palette_bytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                   sSkinPaletteBuffer, sSkinPaletteAllocation, &pal_mapped, true)
                && pal_mapped != nullptr
                && createBufferVkImpl((U32)base_bytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                      sSkinBaseBuffer, sSkinBaseAllocation, &base_mapped, true)
                && base_mapped != nullptr)
            {
                sSkinPaletteMapped = reinterpret_cast<U8*>(pal_mapped);
                sSkinBaseMapped    = reinterpret_cast<U32*>(base_mapped);
                // B.2: sentinel = INVALID (0xFFFFFFFF) so unwritten slots fall back to the UBO path.
                std::memset(sSkinBaseMapped, 0xFF, (size_t)base_bytes);
                for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
                {
                    sSkinPaletteCursor[i].store(0, std::memory_order_relaxed);
                }

                VkDescriptorBufferInfo sbi[2] = {};
                sbi[0].buffer = sSkinPaletteBuffer; sbi[0].offset = 0; sbi[0].range = VK_WHOLE_SIZE;
                sbi[1].buffer = sSkinBaseBuffer;    sbi[1].offset = 0; sbi[1].range = (VkDeviceSize)DRAWDATA_TOTAL_SLOTS * 4;
                VkWriteDescriptorSet sw[2] = {};
                sw[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                sw[0].dstSet          = sSkinBaseSet;
                sw[0].dstBinding      = 1;
                sw[0].descriptorCount = 1;
                sw[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                sw[0].pBufferInfo     = &sbi[0];
                sw[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                sw[1].dstSet          = sSkinBaseSet;
                sw[1].dstBinding      = 0;
                sw[1].descriptorCount = 1;
                sw[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
                sw[1].pBufferInfo     = &sbi[1];
                vkUpdateDescriptorSets(sDevice, 2, sw, 0, nullptr);
            }
            else
            {
                LL_WARNS("Vulkan") << "VKBindless: skin bindless buffer creation failed (skin bindless inactive)" << LL_ENDL;
                destroyBindlessHeap();
                return true;
            }
        }

        sBindlessHeapCount = count;
        sBindlessSlotNext  = 1;
        sBindlessActive    = true;
        sBindlessSlotView.assign(count, VK_NULL_HANDLE);   // β shadow
        LLVKContract::mdiInit(DRAWDATA_TOTAL_SLOTS);        // α shadow
        bindlessWriteSlotInternal(0, VK_NULL_HANDLE, VK_NULL_HANDLE);
        LL_INFOS("Vulkan") << "VKBindless: heap active count=" << count << LL_ENDL;
        return true;
    }
} // namespace LLVKLoaderInternal


bool anyViewHandleDead(const void* const* views, U32 count)
{
    std::lock_guard<std::mutex> lk(sDeadHandleMutex);
    for (U32 i = 0; i < count; ++i)
    {
        if (views[i] != nullptr && sDeadViewHandles.count((U64)(uintptr_t)views[i]) != 0)
        {
            return true;
        }
    }
    return false;
}

bool isBindlessActiveVk()
{
    return sBindlessActive;
}

bool skinBindlessEnabled()
{
    return sSkinBindlessEnabled;
}

U32 bindlessAcquireSlot(VkImageView view, VkSampler sampler)
{
    if (!sBindlessActive)
    {
        return BINDLESS_INVALID_SLOT;
    }
    std::lock_guard<std::mutex> guard(sBindlessSlotMutex);
    U32 slot;
    if (!sBindlessSlotFreeList.empty())
    {
        slot = sBindlessSlotFreeList.back();
        sBindlessSlotFreeList.pop_back();
    }
    else if (sBindlessSlotNext < sBindlessHeapCount)
    {
        slot = sBindlessSlotNext++;
    }
    else
    {
        static bool warned = false;
        if (!warned)
        {
            LL_WARNS("Vulkan") << "VKBindless: heap exhausted (count=" << sBindlessHeapCount << ")" << LL_ENDL;
            warned = true;
        }
        return BINDLESS_INVALID_SLOT;
    }
    bindlessWriteSlotInternal(slot, view, sampler);
    return slot;
}

// β: slot に実際に登録されている view（記録スレッドからの読取・latent race 許容 = 別 tex 判別が目的）
VkImageView bindlessSlotView(U32 slot)
{
    return (slot < sBindlessSlotView.size()) ? sBindlessSlotView[slot] : VK_NULL_HANDLE;
}

VkImageView bindlessFallbackView()
{
    return sDefaultFallbackImageView;
}

void bindlessReleaseSlotDeferred(U32 slot)
{
    if (!sBindlessActive || slot == 0 || slot == BINDLESS_INVALID_SLOT || slot >= sBindlessHeapCount)
    {
        return;
    }
    std::lock_guard<std::mutex> guard(sBindlessSlotMutex);
    PendingSlotFree p;
    p.slot          = slot;
    p.enqueue_frame = sMonotonicFrameCount;
    sPendingSlotFrees.push_back(p);
}

VkDescriptorSetLayout getBindlessHeapLayout()
{
    return sBindlessHeapLayout;
}

VkDescriptorSetLayout getSkinBaseLayout()
{
    return sSkinBaseLayout;
}

VkDescriptorSetLayout getEmptySetLayout()
{
    return sEmptySetLayout;
}

U32 drawDataAcquireSlot(const U32* slots4)
{
    if (sDrawDataMapped == nullptr)
    {
        return BINDLESS_INVALID_SLOT;
    }
    AllocDomain* d = tAllocDomain;
    VkcRaceProbe probe(d->mSlotOwner, LLVKContract::C_DRAWDATA_RACE);
    U32 slot;
    if (!d->mSlotFree.empty())
    {
        slot = d->mSlotFree.back();
        d->mSlotFree.pop_back();
    }
    else if (d->mSlotNext < d->mSlotEnd)
    {
        slot = d->mSlotNext++;
    }
    else if (slotSlabGrow(d) && d->mSlotNext < d->mSlotEnd)
    {
        slot = d->mSlotNext++;
    }
    else
    {
        LLVKContract::cause(LLVKContract::C_DRAWDATA_EXHAUSTED);
        static bool warned = false;
        if (!warned)
        {
            LL_WARNS("Vulkan") << "VKBindless: DrawData slots exhausted" << LL_ENDL;
            warned = true;
        }
        return BINDLESS_INVALID_SLOT;
    }
    std::memcpy(sDrawDataMapped + (size_t)slot * DRAWDATA_SLOT_UINTS, slots4, DRAWDATA_SLOT_UINTS * 4);
    return slot;
}

void drawDataReleaseSlotDeferred(U32 slot)
{
    if (sDrawDataMapped == nullptr || slot == 0 || slot == BINDLESS_INVALID_SLOT || slot >= DRAWDATA_PERSISTENT_SLOTS)
    {
        return;
    }
    AllocDomain* d = allocDomainForSlot(slot);
    if (d == nullptr)
    {
        return;
    }
    std::lock_guard<std::mutex> lk(d->mPendMutex);
    d->mPendSlot.push_back({ slot, sMonotonicFrameCount });
}

} // namespace LLVKLoader
