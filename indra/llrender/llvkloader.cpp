/**
* @file llvkloader.cpp
* @brief AYAstorm r41 Vulkan loader + instance + device lifecycle (volk-based)
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

#include <vector>
#include <string>
#include <climits>
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
#define VMA_IMPLEMENTATION
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

namespace LLVKLoader
{

namespace
{
    VkInstance       sInstance            = VK_NULL_HANDLE;
    VkPhysicalDevice sPhysicalDevice      = VK_NULL_HANDLE;
    VkDevice         sDevice              = VK_NULL_HANDLE;
    VkQueue          sGraphicsQueue       = VK_NULL_HANDLE;
    U32              sGraphicsQueueFamily = UINT_MAX;
    bool             sInitialized         = false;
    bool             sCheckpointsEnabled  = false;

    VkDebugUtilsMessengerEXT sDebugMessenger = VK_NULL_HANDLE;

    VkCommandPool   sCommandPool   = VK_NULL_HANDLE;
    VkPipelineCache sPipelineCache = VK_NULL_HANDLE;

    VkQueryPool          sOcclusionQueryPool     = VK_NULL_HANDLE;
    U32                  sOcclusionQueryCapacity = 0;
    bool                 sHostQueryResetEnabled  = false;
    std::queue<uint32_t> sOcclusionQueryFree;

    VkQueryPool          sTimestampQueryPool     = VK_NULL_HANDLE;
    U32                  sTimestampPairCapacity  = 0;
    bool                 sTimestampSupported     = false;
    float                sTimestampPeriodNs      = 0.0f;
    U32                  sTimestampValidBits     = 0;
    std::queue<uint32_t> sTimestampPairFree;

    VkSurfaceKHR sSurface           = VK_NULL_HANDLE;

    VkSwapchainKHR           sSwapchain           = VK_NULL_HANDLE;
    std::vector<VkImage>     sSwapchainImages;
    std::vector<VkImageView> sSwapchainImageViews;
    VkFormat                 sSwapchainFormat     = VK_FORMAT_UNDEFINED;
    VkExtent2D               sSwapchainExtent     = {0, 0};

    VkImage        sDefaultFallbackImage     = VK_NULL_HANDLE;
    VkDeviceMemory sDefaultFallbackMemory    = VK_NULL_HANDLE;
    VkImageView    sDefaultFallbackImageView = VK_NULL_HANDLE;
    VkImage        sDefaultFallbackCubeArrayImage     = VK_NULL_HANDLE;
    VkImageView    sDefaultFallbackCubeArrayImageView = VK_NULL_HANDLE;
    void*          sDefaultFallbackCubeArrayAlloc     = nullptr;
    VkImage        sDefaultFallbackCubeImage     = VK_NULL_HANDLE;
    VkImageView    sDefaultFallbackCubeImageView = VK_NULL_HANDLE;
    void*          sDefaultFallbackCubeAlloc     = nullptr;
    VkImage        sDefaultFallback3DImage     = VK_NULL_HANDLE;
    VkImageView    sDefaultFallback3DImageView = VK_NULL_HANDLE;
    void*          sDefaultFallback3DAlloc     = nullptr;
    VkImage        sDefaultFallbackShadowImage     = VK_NULL_HANDLE;
    VkDeviceMemory sDefaultFallbackShadowMemory    = VK_NULL_HANDLE;
    VkImageView    sDefaultFallbackShadowImageView = VK_NULL_HANDLE;
    bool           sInFrame            = false;

    thread_local bool sInDynamicRendering = false;

    thread_local S32 sVkRenderViewport[4] = {0, 0, 0, 0};

    thread_local U32 sCurrentRenderAreaHeight = 0;

    thread_local bool sScissorEnabled     = false;
    thread_local S32  sScissorRectGL[4]   = {0, 0, 0, 0};

    thread_local VkRenderingAttachmentInfo sSavedColorInfos[4] = {};
    thread_local U32                       sSavedColorCount    = 0;
    thread_local VkRenderingAttachmentInfo sSavedDepthInfo     = {};
    thread_local bool                      sSavedHasDepth      = false;
    thread_local U32                       sSavedRenderWidth   = 0;
    thread_local U32                       sSavedRenderHeight  = 0;

    VmaAllocator sAllocator = VK_NULL_HANDLE;
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

    VkDescriptorSetLayout sPerFrameDescriptorSetLayout            = VK_NULL_HANDLE;
    VkBuffer              sPerFrameUboBuffer[FRAMES_IN_FLIGHT]    = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    VkDeviceMemory        sPerFrameUboMemory[FRAMES_IN_FLIGHT]    = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    void*                 sPerFrameUboMapped[FRAMES_IN_FLIGHT]    = { nullptr,        nullptr,        nullptr        };

    constexpr U32 MATRIX_RING_CHUNK_SLOTS = 64;
    struct MatrixRingChunk
    {
        VkBuffer       buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void*          mapped = nullptr;
    };
    constexpr U32 MATRIX_RING_MAX_SLOTS  = 16384;
    constexpr U32 MATRIX_RING_MAX_CHUNKS = MATRIX_RING_MAX_SLOTS / MATRIX_RING_CHUNK_SLOTS;
    std::vector<MatrixRingChunk> sMatrixRingChunks[FRAMES_IN_FLIGHT];
    std::vector<VkDescriptorSet> sPerFrameRingSets[FRAMES_IN_FLIGHT];
    std::atomic<U32>             sPerFrameRingSetCount[FRAMES_IN_FLIGHT] = {};
    std::mutex                   sMatrixRingGrowthMutex;
    std::vector<VkDescriptorPool> sPerFrameRingPools;
    U32   sRingPoolUsedInLast = 0;
    std::atomic<U32> sMatrixRingUsedThisFrame[FRAMES_IN_FLIGHT] = {};
    thread_local U32   sMatrixRingCurrentSlot[FRAMES_IN_FLIGHT]   = { 0, 0, 0 };
    thread_local bool  sMatrixRingHasCurrent[FRAMES_IN_FLIGHT]    = { false, false, false };
    thread_local float sMatrixRingCurrentProj[FRAMES_IN_FLIGHT][16] = {};
    thread_local float sMatrixRingCurrentTexmat[FRAMES_IN_FLIGHT][64] = {};

    thread_local VkPipeline       sLastBoundGraphicsPipeline = VK_NULL_HANDLE;
    thread_local VkPipelineLayout sLastDescLayout   = VK_NULL_HANDLE;
    thread_local VkDescriptorSet  sLastDescSet0     = VK_NULL_HANDLE;
    thread_local VkDescriptorSet  sLastDescSet1     = VK_NULL_HANDLE;
    thread_local VkDescriptorSet  sLastDescSet2     = VK_NULL_HANDLE;
    thread_local U32              sLastDescDynCount = 0;
    thread_local U32              sLastDescOffsets[LLGLSLShader::MAX_VK_DYNAMIC_BINDINGS] = {};
    thread_local VkPipelineLayout sLastMvLayout     = VK_NULL_HANDLE;
    thread_local float            sLastMv[16]       = {};
    thread_local VkViewport       sLastViewport     = {};
    thread_local VkRect2D         sLastScissor      = {};
    thread_local bool             sViewportScissorValid = false;

    thread_local VkCommandBuffer  tRecordCmdOverride = VK_NULL_HANDLE;

    thread_local VkCommandBuffer  tMemoCmd = VK_NULL_HANDLE;

    thread_local U64              tCmdRecordEpoch = 1;
    thread_local U64              tMemoEpoch      = 0;

    thread_local VkCommandBuffer  tVBMemoCmd   = VK_NULL_HANDLE;
    thread_local U64              tVBMemoEpoch = 0;
    thread_local VkBuffer         tVBMemoBuf[16] = {};
    thread_local VkDeviceSize     tVBMemoOff[16] = {};

    thread_local VkCommandBuffer  tIBMemoCmd   = VK_NULL_HANDLE;
    thread_local U64              tIBMemoEpoch = 0;
    thread_local VkBuffer         tIBMemoBuf   = VK_NULL_HANDLE;
    thread_local VkDeviceSize     tIBMemoOff   = 0;
    thread_local VkIndexType      tIBMemoType  = VK_INDEX_TYPE_MAX_ENUM;

    std::atomic<U32> sVkcScratchOwner{0};
    std::atomic<U32> sVkcSlotOwner{0};
    std::atomic<U32> sVkcMegaOwner{0};

    U32 vkcRaceSelf()
    {
        static std::atomic<U32> s_next{0};
        static thread_local U32 s_id = ++s_next;
        return s_id;
    }

    struct VkcRaceProbe
    {
        std::atomic<U32>& mOwner;
        bool              mOwned;
        VkcRaceProbe(std::atomic<U32>& owner, LLVKContract::ECause c)
            : mOwner(owner)
        {
            U32 expected = 0;
            mOwned = owner.compare_exchange_strong(expected, vkcRaceSelf(), std::memory_order_acquire);
            if (!mOwned)
            {
                LLVKContract::cause(c);
            }
        }
        ~VkcRaceProbe()
        {
            if (mOwned)
            {
                mOwner.store(0, std::memory_order_release);
            }
        }
    };

    bool vkCmdMemoEnabled()
    {
        static const bool s_enabled = []() -> bool {
            const char* e = getenv("AYASTORM_VKCMD_MEMO");
            return (e == nullptr) || (atof(e) != 0.0);
        }();
        return s_enabled;
    }

    VkSampler             sStandardLinearSampler                  = VK_NULL_HANDLE;

    std::unordered_map<U32, VkSampler> sSamplerCache;
    bool                     sSamplerAnisotropyEnabled               = false;
    bool                     sVertexStoresAtomicsEnabled             = false; // B.2: skin A/B oracle atomics
    float                    sMaxSamplerAnisotropy                   = 1.0f;
    float                    sMaxLineWidth                           = 1.0f;
    bool                     sGeometryShaderEnabled                  = false;
    bool                     sImageCubeArrayEnabled                  = false;

    VkPhysicalDeviceProperties sPhysicalDeviceProperties             = {};
    std::string                sDriverName;
    std::string                sDriverInfo;
    U32                        sDeviceLocalMemoryMB                  = 0;

    bool                     sProvokingVertexLastEnabled             = false;

    bool                     sBindlessCapable                        = false;
    U32                      sBindlessHeapCapacity                   = 0;
    bool                     sMultiDrawIndirectEnabled               = false;
    bool                     sDrawIndirectFirstInstanceEnabled       = false;

    VkDescriptorSetLayout    sBindlessHeapLayout                     = VK_NULL_HANDLE;
    VkDescriptorPool         sBindlessHeapPool                       = VK_NULL_HANDLE;
    VkDescriptorSet          sBindlessHeapSet                        = VK_NULL_HANDLE;
    U32                      sBindlessHeapCount                      = 0;
    VkDescriptorSetLayout    sSkinBaseLayout                         = VK_NULL_HANDLE;
    VkDescriptorPool         sSkinBasePool                           = VK_NULL_HANDLE;
    VkDescriptorSet          sSkinBaseSet                            = VK_NULL_HANDLE;
    U32                      sBindlessSlotNext                       = 1;
    std::vector<U32>         sBindlessSlotFreeList;
    std::mutex               sBindlessSlotMutex;
    bool                     sBindlessActive                         = false;
    struct PendingSlotFree
    {
        U32 slot;
        U32 enqueue_frame;
    };
    std::vector<PendingSlotFree> sPendingSlotFrees;

    constexpr U32            DRAWDATA_TOTAL_SLOTS                    = 1048576;
    constexpr U32            DRAWDATA_SCRATCH_PER_FRAME              = 32768;
    constexpr U32            DRAWDATA_PERSISTENT_SLOTS               = DRAWDATA_TOTAL_SLOTS - 3 * DRAWDATA_SCRATCH_PER_FRAME;
    VkBuffer                 sDrawDataBuffer                         = VK_NULL_HANDLE;
    void*                    sDrawDataAllocation                     = nullptr;
    U32*                     sDrawDataMapped                         = nullptr;
    std::mutex               sAllocGrowthMutex;

    // --- B.0/B.2 skin bindless base (SSBO palette + per-draw base index) ---
    constexpr U32            SKIN_PALETTE_ENTRY_BYTES                = 10560; // ObjectSkin_PerProgramBind (mat3x4[110] x2)
    constexpr U32            SKIN_ENTRIES_PER_FRAME                  = 1024;
    VkBuffer                 sSkinPaletteBuffer                      = VK_NULL_HANDLE;
    void*                    sSkinPaletteAllocation                  = nullptr;
    U8*                      sSkinPaletteMapped                      = nullptr;
    VkBuffer                 sSkinBaseBuffer                         = VK_NULL_HANDLE;
    void*                    sSkinBaseAllocation                     = nullptr;
    U32*                     sSkinBaseMapped                         = nullptr;
    std::atomic<U32>         sSkinPaletteCursor[FRAMES_IN_FLIGHT]    = {};
    bool                     sSkinBindlessEnabled                    = true; // B.2 kill switch (AYASTORM_SKIN_BINDLESS=0)
    constexpr U32            DRAWDATA_DOMAIN_SLAB                    = 2048;
    constexpr U32            DRAWDATA_MAX_SLABS                      = DRAWDATA_TOTAL_SLOTS / DRAWDATA_DOMAIN_SLAB;
    struct AllocDomain
    {
        std::atomic<U32>             mSlotOwner{0};
        U32                          mSlotNext = 0;
        U32                          mSlotEnd  = 0;
        std::vector<U32>             mSlotFree;
        std::vector<PendingSlotFree> mPendSlot;
        std::mutex                   mPendMutex;
        U32                          mId = 0;
        explicit AllocDomain(U32 id) : mId(id) {}
    };
    AllocDomain              sMainDomain{0};
    std::vector<AllocDomain*> sAllocDomains;
    std::atomic<U32>         sSlotSlabOwner[DRAWDATA_MAX_SLABS];
    U32                      sSlotSlabNextIdx                        = 0;
    thread_local AllocDomain* tAllocDomain                          = &sMainDomain;
    struct AllocDomainInit
    {
        AllocDomainInit()
        {
            sAllocDomains.reserve(256);
            sAllocDomains.push_back(&sMainDomain);
            for (U32 i = 0; i < DRAWDATA_MAX_SLABS; ++i)
            {
                sSlotSlabOwner[i].store(0xFFFFFFFFu, std::memory_order_relaxed);
            }
        }
    } sAllocDomainInit;
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
    U32                      sDrawDataScratchCursor                  = 0;
    U32                      sDrawDataScratchFrame                   = 0xFFFFFFFFu;
    thread_local U32         tCurrentDrawDataID                      = 0;

    constexpr U32            INDIRECT_RING_COMMANDS_PER_FRAME        = 524288;
    VkBuffer                 sIndirectRingBuffer                     = VK_NULL_HANDLE;
    void*                    sIndirectRingAllocation                 = nullptr;
    U8*                      sIndirectRingMapped                     = nullptr;
    U32                      sIndirectRingCursor                     = 0;
    U32                      sIndirectRingFrame                      = 0xFFFFFFFFu;

    constexpr U32                  SCENE_PER_DRAW_POOL_GROWTH_SETS     = 50000;
    constexpr U32                  SCENE_PER_DRAW_POOL_GROWTH_SAMPLERS = 250000;
    constexpr U32                  SCENE_PER_DRAW_POOL_GROWTH_UBOS     = 50000;

    struct ScenePerDrawCacheKey
    {
        VkDescriptorSetLayout layout;
        VkBuffer              ubo;
        U32                   ubo_binding;
        VkDeviceSize          ubo_size;
        VkSampler             sampler;
        U32                   sampler_count;
        U32                   sampler_bindings[ScenePerDrawBindings::MAX_SAMPLERS];
        VkImageView           sampler_views[ScenePerDrawBindings::MAX_SAMPLERS];
        VkSampler             sampler_samplers[ScenePerDrawBindings::MAX_SAMPLERS] = {};
        U32                   ubo_count = 0;
        U32                   ubo_write_bindings[ScenePerDrawBindings::MAX_UBO_WRITES] = {};
        VkBuffer              ubo_write_bufs[ScenePerDrawBindings::MAX_UBO_WRITES]     = {};

        bool operator==(const ScenePerDrawCacheKey& other) const
        {
            if (layout != other.layout) return false;
            if (ubo != other.ubo) return false;
            if (ubo != VK_NULL_HANDLE &&
                (ubo_binding != other.ubo_binding || ubo_size != other.ubo_size))
            {
                return false;
            }
            if (sampler != other.sampler) return false;
            if (sampler_count != other.sampler_count) return false;
            for (U32 i = 0; i < sampler_count; ++i)
            {
                if (sampler_bindings[i] != other.sampler_bindings[i]) return false;
                if (sampler_views[i] != other.sampler_views[i]) return false;
                if (sampler_samplers[i] != other.sampler_samplers[i]) return false;
            }
            if (ubo_count != other.ubo_count) return false;
            for (U32 i = 0; i < ubo_count; ++i)
            {
                if (ubo_write_bindings[i] != other.ubo_write_bindings[i]) return false;
                if (ubo_write_bufs[i]     != other.ubo_write_bufs[i])     return false;
            }
            return true;
        }
    };
    struct ScenePerDrawCacheKeyHash
    {
        std::size_t operator()(const ScenePerDrawCacheKey& k) const noexcept
        {
            std::size_t h = std::hash<const void*>()(reinterpret_cast<const void*>(k.layout));
            h ^= std::hash<const void*>()(reinterpret_cast<const void*>(k.ubo))     + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<const void*>()(reinterpret_cast<const void*>(k.sampler)) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<U32>()(k.sampler_count)                                  + 0x9e3779b9 + (h << 6) + (h >> 2);
            for (U32 i = 0; i < k.sampler_count; ++i)
            {
                h ^= std::hash<U32>()(k.sampler_bindings[i])                                   + 0x9e3779b9 + (h << 6) + (h >> 2);
                h ^= std::hash<const void*>()(reinterpret_cast<const void*>(k.sampler_views[i])) + 0x9e3779b9 + (h << 6) + (h >> 2);
                h ^= std::hash<const void*>()(reinterpret_cast<const void*>(k.sampler_samplers[i])) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
            h ^= std::hash<U32>()(k.ubo_count) + 0x9e3779b9 + (h << 6) + (h >> 2);
            for (U32 i = 0; i < k.ubo_count; ++i)
            {
                h ^= std::hash<U32>()(k.ubo_write_bindings[i])                                   + 0x9e3779b9 + (h << 6) + (h >> 2);
                h ^= std::hash<const void*>()(reinterpret_cast<const void*>(k.ubo_write_bufs[i])) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
            return h;
        }
    };
    struct ScenePerDrawCacheEntry
    {
        VkDescriptorSet                              sets[FRAMES_IN_FLIGHT] = {};
        std::list<ScenePerDrawCacheKey>::iterator    lru_pos;
        U32                                          last_used_monotonic_frame = 0;
        U32                                          pool_index = 0;
        std::atomic<U32>                             refs{0};
    };
    U64 sScenePerDrawCacheEpoch = 1;
    struct ScenePerDrawDeferredFreeEntry
    {
        VkDescriptorSet sets[FRAMES_IN_FLIGHT];
        U32             pool_index;
        U32             enqueue_frame;
    };

    thread_local U32 tRecordLaneIndex = 0;

    struct PerDrawDescLane
    {
        std::vector<VkDescriptorPool> pools;
        std::unordered_map<ScenePerDrawCacheKey, ScenePerDrawCacheEntry, ScenePerDrawCacheKeyHash> cache;
        std::list<ScenePerDrawCacheKey> lru;
        std::vector<ScenePerDrawDeferredFreeEntry> deferred_free;
    };
    PerDrawDescLane sPerDrawDescLanes[MAX_RECORD_LANES];


    struct DeferredUtilOverrideSlot
    {
        VkBuffer buffer     = VK_NULL_HANDLE;
        void*    allocation = nullptr;
        void*    mapped     = nullptr;
    };
    std::vector<DeferredUtilOverrideSlot> sDuOverrideRing[FRAMES_IN_FLIGHT];
    U32 sDuOverrideIdx[FRAMES_IN_FLIGHT]   = { 0, 0, 0 };
    U64 sDuOverrideFrame[FRAMES_IN_FLIGHT] = { 0, 0, 0 };

    std::vector<DeferredUtilOverrideSlot> sDeferredUtilRing[FRAMES_IN_FLIGHT];
    U32      sDeferredUtilRingIdx[FRAMES_IN_FLIGHT]   = { 0, 0, 0 };
    U64      sDeferredUtilRingFrame[FRAMES_IN_FLIGHT] = { 0, 0, 0 };
    VkBuffer sCurDeferredUtilBuf[FRAMES_IN_FLIGHT]    = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    void*    sCurDeferredUtilMapped[FRAMES_IN_FLIGHT] = { nullptr, nullptr, nullptr };
    VkBuffer sDuOverrideActiveBuf    = VK_NULL_HANDLE;
    void*    sDuOverrideActiveMapped = nullptr;
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

    struct PerDrawUBOOverflowBlock
    {
        VkBuffer     buffer     = VK_NULL_HANDLE;
        void*        allocation = nullptr;
        void*        mapped     = nullptr;
        VkDeviceSize capacity   = 0;
        VkDeviceSize cursor     = 0;
    };
    struct PerDrawUBOArena
    {
        VkBuffer     buffer           = VK_NULL_HANDLE;
        void*        allocation       = nullptr;
        void*        mapped           = nullptr;
        VkDeviceSize capacity         = 0;
        std::atomic<VkDeviceSize> cursor{0};
        U64          frame            = ~0ull;
        VkDeviceSize pending_capacity = 0;
        std::vector<PerDrawUBOOverflowBlock> overflow;
    };
    PerDrawUBOArena sPerDrawUBOArena[FRAMES_IN_FLIGHT];
    constexpr VkDeviceSize PER_DRAW_UBO_ARENA_INITIAL = 4 * 1024 * 1024;
    std::mutex sPerDrawArenaGrowthMutex;

    U32 sFrameIndex = 0;

    VkCommandBuffer sCommandBuffers[FRAMES_IN_FLIGHT] = {
        VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE
    };
    VkCommandBuffer sConsumerCommandBuffers[FRAMES_IN_FLIGHT] = {
        VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE
    };
    bool sUISceneSplit = (getenv("AYASTORM_UISCENE") != nullptr);
    bool sUISceneAsync = (getenv("AYASTORM_UISCENE_ASYNC") != nullptr);
    bool sConsumerActiveThisFrame = false;

    VkCommandBuffer sAsyncProducerCommandBuffer = VK_NULL_HANDLE;
    VkFence         sAsyncProducerFence         = VK_NULL_HANDLE;
    bool            sAsyncProducerInFlight      = false;
    U32             sAsyncProducerBackIndex     = 0;
    bool            sAsyncRenderSceneThisFrame  = false;
    bool            sAsyncFrameEngaged          = false;
    U32             sAsyncProducerSubmitMonotonic = 0;
    U32             sAsyncProducerRecordSlot      = 0;
    VkFence sInFlightFences[FRAMES_IN_FLIGHT] = {
        VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE
    };

    VkFence sProducerFences[FRAMES_IN_FLIGHT] = {
        VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE
    };
    bool sProducerFencePending[FRAMES_IN_FLIGHT] = { false, false, false };
    bool sProducerPresentActive = false;

    VkSemaphore sImageAvailableSemaphores[FRAMES_IN_FLIGHT] = {
        VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE
    };
    VkSemaphore sRenderFinishedSemaphores[FRAMES_IN_FLIGHT] = {
        VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE
    };
    U32  sAcquiredImageIndex         = 0;
    bool sImageAcquired              = false;
    bool sVulkanPresentationEnabled  = true;

    bool sSwapchainClearedThisFrame  = false;

    VkImage       sSwapchainDepthImage  = VK_NULL_HANDLE;
    VkImageView   sSwapchainDepthView   = VK_NULL_HANDLE;
    void*         sSwapchainDepthAlloc  = nullptr;
    VkImageLayout sSwapchainDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    std::atomic<bool> sSwapchainRecreatePending{false};
    U32  sPendingResizeWidth       = 0;
    U32  sPendingResizeHeight      = 0;
    U32  sMonotonicFrameCount      = 0;
    U32  sLastRecreateFrame        = 0;
    constexpr U32 STARTUP_FRAME_GATE         = 30;
    constexpr U32 RECREATE_COOLDOWN_FRAMES   = 30;

    struct PendingBufferFree
    {
        VkBuffer      buffer;
        VmaAllocation allocation;
        U32           enqueue_frame;
    };
    std::vector<PendingBufferFree> sPendingBufferFrees;

    struct PendingImageFree
    {
        VkImage       image;
        VkImageView   view;
        VmaAllocation allocation;
        U32           enqueue_frame;
    };
    std::vector<PendingImageFree> sPendingImageFrees;

    struct PendingObjectFree
    {
        VkPipeline            pipeline              = VK_NULL_HANDLE;
        VkShaderModule        shader_module         = VK_NULL_HANDLE;
        VkPipelineLayout      pipeline_layout       = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
        U32                   enqueue_frame         = 0;
    };
    std::vector<PendingObjectFree> sPendingObjectFrees;

    struct PendingOneShotFree
    {
        VkFence         fence      = VK_NULL_HANDLE;
        VkCommandBuffer cmd        = VK_NULL_HANDLE;
        VkCommandPool   pool       = VK_NULL_HANDLE;
        VkBuffer        buffer     = VK_NULL_HANDLE;
        VmaAllocation   allocation = VK_NULL_HANDLE;
        U32             staging_bytes = 0;
    };
    std::vector<PendingOneShotFree> sPendingOneShotFrees;
    std::vector<VkFence>            sSubmitFencePool;
    std::mutex                      sOneShotMutex;
    std::vector<VkCommandBuffer>    sRetiredMainOneShotCmds;
    std::vector<VkCommandBuffer>    sRetiredTexOneShotCmds;
    std::atomic<U64>                sOneShotStagingBytes{0};
    VkCommandPool                   sTexWorkerCommandPool = VK_NULL_HANDLE;
    thread_local bool               tTexWorkerThread = false;
    void (*sTexWorkerStopHook)()   = nullptr;
    void (*sGeoWorkerStopHook)()   = nullptr;
    void (*sBakeWorkerStopHook)()  = nullptr;

    U32 sLastCompletedMonotonic = 0;
    U32 sFrameSubmittedMonotonic[FRAMES_IN_FLIGHT] = { 0, 0, 0 };

    enum ReapMode { REAP_CHURN = 0, REAP_CLOSE = 1, REAP_LOST = 2 };
    bool sReapForceAll = false;
    bool sProducersQuiesced = false;
    void (*sDeviceLostHook)() = nullptr;
    bool sDeviceLostSignaled = false;
    static inline bool reapReady(U32 enqueue_frame)
    {
        return sReapForceAll || enqueue_frame <= sLastCompletedMonotonic;
    }

    VkCommandBuffer currentRecordCmd()
    {
        if (tRecordCmdOverride != VK_NULL_HANDLE)
        {
            return tRecordCmdOverride;
        }
        return sInFrame ? sCommandBuffers[sFrameIndex] : VK_NULL_HANDLE;
    }

    void gpuCheckpointImpl(const char* label)
    {
        if (!sCheckpointsEnabled)
        {
            return;
        }
        VkCommandBuffer cmd = currentRecordCmd();
        if (cmd != VK_NULL_HANDLE)
        {
            vkCmdSetCheckpointNV(cmd, (const void*)label);
        }
    }

    void dumpCheckpointsOnDeviceLost()
    {
        if (!sCheckpointsEnabled || sGraphicsQueue == VK_NULL_HANDLE)
        {
            return;
        }
        U32 n = 0;
        vkGetQueueCheckpointDataNV(sGraphicsQueue, &n, nullptr);
        if (n == 0)
        {
            LL_WARNS("Vulkan") << "GPU breadcrumb: no checkpoint data on device-lost" << LL_ENDL;
            return;
        }
        std::vector<VkCheckpointDataNV> data(n);
        for (auto& d : data)
        {
            d.sType = VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;
            d.pNext = nullptr;
        }
        vkGetQueueCheckpointDataNV(sGraphicsQueue, &n, data.data());
        for (const auto& d : data)
        {
            const char* lbl = (const char*)d.pCheckpointMarker;
            LL_WARNS("Vulkan") << "GPU breadcrumb @devlost: stage=0x" << std::hex << (U32)d.stage << std::dec
                               << " last-reached=" << (lbl ? lbl : "(null)") << LL_ENDL;
        }
    }

    enum : U32
    {
        PE_SLOT_IDLE      = 0,
        PE_SLOT_PENDING   = 1,
        PE_SLOT_SUBMITTED = 2,
        PE_SLOT_FAILED    = 3
    };

    struct PEPresentTarget
    {
        VkSwapchainKHR swapchain      = VK_NULL_HANDLE;
        U32            image_index    = 0;
        VkSemaphore    wait_semaphore = VK_NULL_HANDLE;
    };

    struct PESyncPoint
    {
        std::mutex              m;
        std::condition_variable cv;
        bool                    done   = false;
        VkResult                result = VK_ERROR_UNKNOWN;
    };

    struct PEJob
    {
        std::vector<VkCommandBuffer> pre_cmds;
        VkCommandBuffer cmd              = VK_NULL_HANDLE;
        VkFence         fence            = VK_NULL_HANDLE;
        VkSemaphore     wait_semaphore   = VK_NULL_HANDLE;
        VkSemaphore     signal_semaphore = VK_NULL_HANDLE;
        std::vector<PEPresentTarget> presents;
        bool            is_frame   = false;
        bool            is_oneshot = false;
        bool            wait_idle  = false;
        U32             slot       = 0;
        PESyncPoint*    sync       = nullptr;
    };

    std::atomic<U32>        sPESlotState[FRAMES_IN_FLIGHT] = {};
    std::mutex              sPESlotMutex;
    std::condition_variable sPESlotCv;

    std::mutex              sPEQueueMutex;
    std::condition_variable sPEQueueCv;
    std::deque<PEJob>       sPEJobs;
    bool                    sPEBusy          = false;
    bool                    sPEStopRequested = false;
    bool                    sPERunning       = false;
    bool                    sPEThreaded      = false;
    std::thread             sPEThread;

    std::mutex              sSwapchainAccessMutex;

    std::mutex              sPEFailedMutex;
    std::vector<VkFence>    sPEFailedOneShotFences;
    std::atomic<bool>       sVkDeviceLost{false};

    std::atomic<U64>        sPESubmitUs{0};
    std::atomic<U64>        sPEPresentUs{0};
    bool                    sPresentWaitEnabled = false;
    std::atomic<U64>        sPresentIdCounter{0};
    VkPresentModeKHR        sActivePresentMode = VK_PRESENT_MODE_FIFO_KHR;
    std::atomic<bool>       sVsyncEnabled{true};

    void peExecute(PEJob& job)
    {
        VkSubmitInfo si = {};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        std::vector<VkCommandBuffer> submit_cmds;
        if (!job.pre_cmds.empty())
        {
            submit_cmds = job.pre_cmds;
            if (job.cmd != VK_NULL_HANDLE)
            {
                submit_cmds.push_back(job.cmd);
            }
            si.commandBufferCount = (U32)submit_cmds.size();
            si.pCommandBuffers    = submit_cmds.data();
        }
        else if (job.cmd != VK_NULL_HANDLE)
        {
            si.commandBufferCount = 1;
            si.pCommandBuffers    = &job.cmd;
        }
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        if (job.wait_semaphore != VK_NULL_HANDLE)
        {
            si.waitSemaphoreCount = 1;
            si.pWaitSemaphores    = &job.wait_semaphore;
            si.pWaitDstStageMask  = &wait_stage;
        }
        if (job.signal_semaphore != VK_NULL_HANDLE)
        {
            si.signalSemaphoreCount = 1;
            si.pSignalSemaphores    = &job.signal_semaphore;
        }

        const auto t0 = std::chrono::steady_clock::now();
        VkResult sr = vkQueueSubmit(sGraphicsQueue, 1, &si, job.fence);
        if (sr == VK_SUCCESS && job.wait_idle)
        {
            vkQueueWaitIdle(sGraphicsQueue);
        }
        sPESubmitUs += (U64)std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - t0).count();

        if (job.is_frame)
        {
            {
                std::lock_guard<std::mutex> lk(sPESlotMutex);
                sPESlotState[job.slot].store((sr == VK_SUCCESS) ? PE_SLOT_SUBMITTED : PE_SLOT_FAILED);
            }
            sPESlotCv.notify_all();
        }
        else if (sr != VK_SUCCESS && job.is_oneshot && job.fence != VK_NULL_HANDLE)
        {
            std::lock_guard<std::mutex> lk(sPEFailedMutex);
            sPEFailedOneShotFences.push_back(job.fence);
        }

        if (sr != VK_SUCCESS)
        {
            if (sr == VK_ERROR_DEVICE_LOST)
            {
                static bool s_bc_dumped = false;
                if (!s_bc_dumped)
                {
                    s_bc_dumped = true;
                    dumpCheckpointsOnDeviceLost();
                }
                sVkDeviceLost.store(true, std::memory_order_release);
            }
            LL_WARNS("Vulkan") << "PresentEngine submit failed sr=" << (S32)sr
                               << " frame=" << (job.is_frame ? 1 : 0) << LL_ENDL;
        }
        else
        {
            for (PEPresentTarget& t : job.presents)
            {
                if (t.swapchain == VK_NULL_HANDLE)
                {
                    continue;
                }
                VkPresentInfoKHR present_info = {};
                present_info.sType          = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                present_info.swapchainCount = 1;
                present_info.pSwapchains    = &t.swapchain;
                present_info.pImageIndices  = &t.image_index;
                if (t.wait_semaphore != VK_NULL_HANDLE)
                {
                    present_info.waitSemaphoreCount = 1;
                    present_info.pWaitSemaphores    = &t.wait_semaphore;
                }
                // ============================================================================
                // 3-OS MAINTAINERS — PLEASE VERIFY / IMPLEMENT ON YOUR PLATFORM:
                // This VK_KHR_present_wait path replaces the graphics driver's FIFO vsync
                // BUSY-WAIT (which pegs a whole CPU core spinning on clock_gettime) with a real
                // SLEEP via vkWaitForPresentKHR. Verified on Linux+NVIDIA (present thread CPU
                // 93% -> 2%, fps unchanged, tear-free). It is gated on sPresentWaitEnabled so it
                // silently no-ops where the extension is absent.
                //   * WINDOWS maintainer: NVIDIA/AMD/Intel Windows drivers expose
                //     VK_KHR_present_wait — confirm it is enabled (device-creation block in
                //     initVulkan) and that the present thread stops spinning; add a Win32
                //     fallback (e.g. driver knob / waitable swapchain) if the driver still spins.
                //   * macOS (MoltenVK) maintainer: VK_KHR_present_wait support is
                //     MoltenVK-version dependent. If unsupported, keep the no-op fallback and
                //     add a Metal-side present-pacing (CAMetalLayer / MTLDrawable) sleep so
                //     macOS does not busy-wait either.
                // Do NOT ship a platform that busy-waits vsync — return the core.
                // ============================================================================
                VkPresentIdKHR present_id_info = {};
                uint64_t this_present_id = 0;
                if (sPresentWaitEnabled && sActivePresentMode == VK_PRESENT_MODE_FIFO_KHR)
                {
                    this_present_id = sPresentIdCounter.fetch_add(1, std::memory_order_relaxed) + 1;
                    present_id_info.sType          = VK_STRUCTURE_TYPE_PRESENT_ID_KHR;
                    present_id_info.swapchainCount = 1;
                    present_id_info.pPresentIds    = &this_present_id;
                    present_id_info.pNext          = present_info.pNext;
                    present_info.pNext             = &present_id_info;
                }
                const auto p0 = std::chrono::steady_clock::now();
                VkResult pr;
                {
                    std::lock_guard<std::mutex> lk(sSwapchainAccessMutex);
                    pr = vkQueuePresentKHR(sGraphicsQueue, &present_info);
                }
                sPEPresentUs += (U64)std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - p0).count();
                if (pr == VK_ERROR_OUT_OF_DATE_KHR || pr == VK_SUBOPTIMAL_KHR)
                {
                    sSwapchainRecreatePending = true;
                }
                else if (pr == VK_ERROR_DEVICE_LOST)
                {
                    sVkDeviceLost.store(true, std::memory_order_release);
                }
                else if (sPresentWaitEnabled && pr == VK_SUCCESS && this_present_id != 0)
                {
                    VkResult wr = vkWaitForPresentKHR(sDevice, t.swapchain, this_present_id, 100000000ull);
                    if (wr == VK_ERROR_DEVICE_LOST)
                    {
                        sVkDeviceLost.store(true, std::memory_order_release);
                    }
                }
            }
        }

        if (job.sync != nullptr)
        {
            {
                std::lock_guard<std::mutex> lk(job.sync->m);
                job.sync->done   = true;
                job.sync->result = sr;
            }
            job.sync->cv.notify_all();
        }
    }

    void peThreadMain()
    {
#if LL_LINUX
        pthread_setname_np(pthread_self(), "aya-present");
#endif
        for (;;)
        {
            PEJob job;
            {
                std::unique_lock<std::mutex> lk(sPEQueueMutex);
                sPEQueueCv.wait(lk, [] { return sPEStopRequested || !sPEJobs.empty(); });
                if (sPEJobs.empty())
                {
                    return;
                }
                job = std::move(sPEJobs.front());
                sPEJobs.pop_front();
                sPEBusy = true;
            }
            peExecute(job);
            {
                std::lock_guard<std::mutex> lk(sPEQueueMutex);
                sPEBusy = false;
            }
            sPEQueueCv.notify_all();
        }
    }

    void peEnqueue(PEJob&& job)
    {
        if (!sPERunning)
        {
            peExecute(job);
            return;
        }
        {
            std::lock_guard<std::mutex> lk(sPEQueueMutex);
            sPEJobs.push_back(std::move(job));
        }
        sPEQueueCv.notify_one();
    }

    VkResult peSubmitBlocking(VkCommandBuffer cmd, VkFence fence, bool wait_idle)
    {
        PESyncPoint sync;
        PEJob job;
        job.cmd       = cmd;
        job.fence     = fence;
        job.wait_idle = wait_idle;
        job.sync      = &sync;
        if (!sPERunning)
        {
            peExecute(job);
            return sync.result;
        }
        {
            std::lock_guard<std::mutex> lk(sPEQueueMutex);
            sPEJobs.push_back(std::move(job));
        }
        sPEQueueCv.notify_one();
        std::unique_lock<std::mutex> lk(sync.m);
        sync.cv.wait(lk, [&] { return sync.done; });
        return sync.result;
    }

    bool peWaitSlotSubmitted(U32 slot)
    {
        for (;;)
        {
            const U32 st = sPESlotState[slot].load();
            if (st != PE_SLOT_PENDING)
            {
                return st == PE_SLOT_SUBMITTED;
            }
            std::unique_lock<std::mutex> lk(sPESlotMutex);
            sPESlotCv.wait(lk, [slot] { return sPESlotState[slot].load() != PE_SLOT_PENDING; });
        }
    }

    void peDrain()
    {
        if (!sPERunning)
        {
            return;
        }
        std::unique_lock<std::mutex> lk(sPEQueueMutex);
        sPEQueueCv.wait(lk, [] { return sPEJobs.empty() && !sPEBusy; });
    }

    void peStart()
    {
        const char* e = getenv("AYASTORM_MT_THREADS");
        sPEThreaded = (e == nullptr) || (atoi(e) > 1);
        if (!sPEThreaded)
        {
            return;
        }
        sPEStopRequested = false;
        sPERunning       = true;
        sPEThread        = std::thread(peThreadMain);
    }

    void peStop()
    {
        if (!sPERunning)
        {
            return;
        }
        {
            std::lock_guard<std::mutex> lk(sPEQueueMutex);
            sPEStopRequested = true;
        }
        sPEQueueCv.notify_all();
        if (sPEThread.joinable())
        {
            sPEThread.join();
        }
        sPERunning  = false;
        sPEThreaded = false;
    }

    struct RecordJob
    {
        std::function<void(VkCommandBuffer)> body;
        U32                                  seq = 0;
    };

    struct RecordLaneCmds
    {
        VkCommandPool                pool[FRAMES_IN_FLIGHT] = {};
        std::vector<VkCommandBuffer> bufs[FRAMES_IN_FLIGHT];
        U32                          used[FRAMES_IN_FLIGHT] = {};
        U32                          reset_frame[FRAMES_IN_FLIGHT] = { ~0u, ~0u, ~0u };
    };
    RecordLaneCmds sRecordLaneCmds[MAX_RECORD_LANES];

    std::mutex               sRWQueueMutex;
    std::condition_variable  sRWQueueCv;
    std::deque<RecordJob>    sRWJobs;
    bool                     sRWStopRequested = false;
    std::vector<std::thread> sRWThreads;
    bool                     sRWStarted = false;

    std::mutex                   sRWDoneMutex;
    std::condition_variable      sRWDoneCv;
    U32                          sRWDispatched = 0;
    U32                          sRWCompleted  = 0;
    std::vector<VkCommandBuffer> sRWFrameCmds;
    std::vector<VkCommandBuffer> sPendingPreFrameCmds;

    thread_local bool tInRecordJob = false;

    U32 rwDesiredWorkerCount()
    {
        static const U32 s_count = []() -> U32 {
            const char* e = getenv("AYASTORM_MT_THREADS");
            if (e != nullptr && atoi(e) <= 1)
            {
                return 0;
            }
            const U32 hw  = (U32)std::thread::hardware_concurrency();
            const U32 cap = (hw > 2) ? (hw - 2) : 1u;
            U32 want = 4u;
            if (e != nullptr)
            {
                const int v = atoi(e) - 1;
                want = (v > 0) ? (U32)v : 1u;
            }
            return llmin(llmin(want, 4u), cap);
        }();
        return s_count;
    }

    void rwResetRecordThreadLocals()
    {
        sLastBoundGraphicsPipeline = VK_NULL_HANDLE;
        sLastDescLayout            = VK_NULL_HANDLE;
        sLastDescSet0              = VK_NULL_HANDLE;
        sLastDescSet1              = VK_NULL_HANDLE;
        sLastDescSet2              = VK_NULL_HANDLE;
        sLastDescDynCount          = 0;
        sLastMvLayout              = VK_NULL_HANDLE;
        sViewportScissorValid      = false;
        sInDynamicRendering        = false;
        sSavedColorCount           = 0;
        sSavedHasDepth             = false;
        sScissorEnabled            = false;
        for (U32 f = 0; f < FRAMES_IN_FLIGHT; ++f)
        {
            sMatrixRingHasCurrent[f] = false;
        }
    }

    VkCommandBuffer rwAcquireLaneCmd()
    {
        const U32 lane = tRecordLaneIndex;
        const U32 f    = sFrameIndex;
        if (lane >= MAX_RECORD_LANES || f >= FRAMES_IN_FLIGHT || sDevice == VK_NULL_HANDLE)
        {
            return VK_NULL_HANDLE;
        }
        RecordLaneCmds& lc = sRecordLaneCmds[lane];
        if (lc.pool[f] == VK_NULL_HANDLE)
        {
            VkCommandPoolCreateInfo ci = {};
            ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            ci.queueFamilyIndex = sGraphicsQueueFamily;
            if (vkCreateCommandPool(sDevice, &ci, nullptr, &lc.pool[f]) != VK_SUCCESS)
            {
                return VK_NULL_HANDLE;
            }
        }
        if (lc.reset_frame[f] != sMonotonicFrameCount)
        {
            vkResetCommandPool(sDevice, lc.pool[f], 0);
            lc.used[f]        = 0;
            lc.reset_frame[f] = sMonotonicFrameCount;
        }
        if (lc.used[f] == (U32)lc.bufs[f].size())
        {
            VkCommandBufferAllocateInfo ai = {};
            ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            ai.commandPool        = lc.pool[f];
            ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            ai.commandBufferCount = 1;
            VkCommandBuffer nb = VK_NULL_HANDLE;
            if (vkAllocateCommandBuffers(sDevice, &ai, &nb) != VK_SUCCESS || nb == VK_NULL_HANDLE)
            {
                return VK_NULL_HANDLE;
            }
            lc.bufs[f].push_back(nb);
        }
        VkCommandBuffer cmd = lc.bufs[f][lc.used[f]];
        ++lc.used[f];
        VkCommandBufferBeginInfo bi = {};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (vkBeginCommandBuffer(cmd, &bi) != VK_SUCCESS)
        {
            return VK_NULL_HANDLE;
        }
        return cmd;
    }

    void rwExecute(RecordJob& job)
    {
        VkCommandBuffer cmd = rwAcquireLaneCmd();
        if (cmd != VK_NULL_HANDLE)
        {
            rwResetRecordThreadLocals();
            LLGLSLShader::resetPerThreadRecordState();
            tInRecordJob       = true;
            tRecordCmdOverride = cmd;
            job.body(cmd);
            if (sInDynamicRendering)
            {
                vkCmdEndRendering(cmd);
                sInDynamicRendering = false;
            }
            tRecordCmdOverride = VK_NULL_HANDLE;
            tInRecordJob       = false;
            if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
            {
                LL_WARNS("Vulkan") << "record job vkEndCommandBuffer failed lane=" << tRecordLaneIndex << LL_ENDL;
                cmd = VK_NULL_HANDLE;
            }
            rwResetRecordThreadLocals();
            LLGLSLShader::resetPerThreadRecordState();
        }
        else
        {
            LL_WARNS("Vulkan") << "record job cmd acquire failed lane=" << tRecordLaneIndex << LL_ENDL;
        }
        {
            std::lock_guard<std::mutex> lk(sRWDoneMutex);
            if (job.seq >= (U32)sRWFrameCmds.size())
            {
                sRWFrameCmds.resize(job.seq + 1, VK_NULL_HANDLE);
            }
            sRWFrameCmds[job.seq] = cmd;
            ++sRWCompleted;
        }
        sRWDoneCv.notify_all();
    }

    void rwThreadMain(U32 lane)
    {
#if LL_LINUX
        char nm[16];
        snprintf(nm, sizeof(nm), "aya-rec%u", lane);
        pthread_setname_np(pthread_self(), nm);
#endif
        tRecordLaneIndex = lane;
        for (;;)
        {
            RecordJob job;
            {
                std::unique_lock<std::mutex> lk(sRWQueueMutex);
                sRWQueueCv.wait(lk, [] { return sRWStopRequested || !sRWJobs.empty(); });
                if (sRWJobs.empty())
                {
                    return;
                }
                job = std::move(sRWJobs.front());
                sRWJobs.pop_front();
            }
            rwExecute(job);
        }
    }

    void rwStart()
    {
        if (sRWStarted)
        {
            return;
        }
        sRWStarted = true;
        const U32 n = rwDesiredWorkerCount();
        for (U32 i = 0; i < n; ++i)
        {
            sRWThreads.emplace_back(rwThreadMain, i + 1);
        }
    }

    void rwStop()
    {
        if (!sRWStarted)
        {
            return;
        }
        {
            std::lock_guard<std::mutex> lk(sRWQueueMutex);
            sRWStopRequested = true;
        }
        sRWQueueCv.notify_all();
        for (std::thread& t : sRWThreads)
        {
            if (t.joinable())
            {
                t.join();
            }
        }
        sRWThreads.clear();
        sRWStarted       = false;
        sRWStopRequested = false;
    }

    void rwDestroyLanePools()
    {
        for (RecordLaneCmds& lc : sRecordLaneCmds)
        {
            for (U32 f = 0; f < FRAMES_IN_FLIGHT; ++f)
            {
                if (lc.pool[f] != VK_NULL_HANDLE)
                {
                    vkDestroyCommandPool(sDevice, lc.pool[f], nullptr);
                    lc.pool[f] = VK_NULL_HANDLE;
                }
                lc.bufs[f].clear();
                lc.used[f]        = 0;
                lc.reset_frame[f] = ~0u;
            }
        }
    }

    struct PendingQueryRelease
    {
        uint32_t index;
        U32      enqueue_frame;
    };
    std::vector<PendingQueryRelease> sPendingOcclusionQueryReleases;

    struct DeviceLimits
    {
        bool memoryBudgetSupported              = false;
    };
    DeviceLimits sDeviceLimits;

    struct Set1BirthInfo
    {
        U32         path        = 0;
        U64         gen         = 0;
        U64         frame       = 0;
        U64         build_frame = 0;
        U64         freed_frame = 0;
        std::string shader;
        std::string contents;
    };
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

    struct ViewDeathInfo
    {
        U64 enqueue_frame   = 0;
        U64 destroy_frame   = 0;
        U64 enqueue_retaddr = 0;
    };
    std::unordered_map<U64, ViewDeathInfo> sViewDeathLedger;

    std::string hex64(U64 v)
    {
        char buf[20];
        snprintf(buf, sizeof(buf), "%llx", (unsigned long long)v);
        return std::string(buf);
    }

    bool vkValidationRequested()
    {
        static const bool s_requested = []
        {
            const char* e = getenv("AYASTORM_VK_VALIDATION");
            return e != nullptr && e[0] != '\0' && e[0] != '0';
        }();
        return s_requested;
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

    VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
        VkDebugUtilsMessageTypeFlagsEXT             types,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void*                                       user_data)
    {
        const char* type_str = (types & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)  ? "VALIDATION" :
                               (types & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) ? "PERF"       :
                                                                                          "GENERAL";
        const char* id  = (data && data->pMessageIdName) ? data->pMessageIdName : "";
        const char* msg = (data && data->pMessage) ? data->pMessage : "";
        if (severity & (VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT))
        {
            U64 n = 0;
            {
                std::lock_guard<std::mutex> lk(sVvlCountMutex);
                auto& entry = sVvlCounts[data ? data->messageIdNumber : 0];
                if (entry.first.empty())
                {
                    entry.first = id;
                }
                n = ++entry.second;
            }
            if (n > 16 && (n & (n - 1)) != 0)
            {
                return VK_FALSE;
            }
            const char* sev_str = (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) ? "[VK-ERROR]" : "[VK-WARN]";
            LL_WARNS("VulkanValidation") << sev_str << "[" << type_str << "][" << id << "] n=" << n << " " << msg
                                         << ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
                                                 ? set1BirthLookup(msg) : std::string())
                                         << LL_ENDL;
        }
        else
        {
            LL_INFOS("VulkanValidation") << "[VK-INFO][" << type_str << "][" << id << "] " << msg << LL_ENDL;
        }
        return VK_FALSE;
    }

    bool createInstance()
    {
        std::vector<const char*> layers;

        std::vector<const char*> extensions;
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if defined(VK_USE_PLATFORM_XLIB_KHR)
        extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#endif
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
#if defined(VK_USE_PLATFORM_METAL_EXT)
        extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#endif

        bool want_validation = false;
        {
            const char* env = getenv("AYASTORM_VK_VALIDATION");
            want_validation = (env != nullptr && env[0] != '\0' && env[0] != '0');
        }

        bool have_validation_layer = false;
        bool have_debug_utils       = false;
        if (want_validation)
        {
            U32 layer_count = 0;
            vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
            std::vector<VkLayerProperties> avail_layers(layer_count);
            if (layer_count)
            {
                vkEnumerateInstanceLayerProperties(&layer_count, avail_layers.data());
            }
            for (const auto& lp : avail_layers)
            {
                if (strcmp(lp.layerName, "VK_LAYER_KHRONOS_validation") == 0)
                {
                    have_validation_layer = true;
                    break;
                }
            }

            U32 ext_count = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr);
            std::vector<VkExtensionProperties> avail_exts(ext_count);
            if (ext_count)
            {
                vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, avail_exts.data());
            }
            for (const auto& ep : avail_exts)
            {
                if (strcmp(ep.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
                {
                    have_debug_utils = true;
                    break;
                }
            }

            if (have_validation_layer)
            {
                layers.push_back("VK_LAYER_KHRONOS_validation");
            }
            if (have_debug_utils)
            {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
            LL_INFOS("VulkanValidation") << "requested=1 layer=" << (have_validation_layer ? 1 : 0)
                                         << " debug_utils=" << (have_debug_utils ? 1 : 0) << LL_ENDL;
        }

        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "AYAstorm";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "AYAstorm-r41";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;
        create_info.enabledLayerCount = (U32)layers.size();
        create_info.ppEnabledLayerNames = layers.data();
        create_info.enabledExtensionCount = (U32)extensions.size();
        create_info.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT dbg_ci = {};
        dbg_ci.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        dbg_ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        dbg_ci.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        dbg_ci.pfnUserCallback = vkDebugCallback;

        VkValidationFeatureEnableEXT enabled_features[] = {
            VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
        };
        VkValidationFeaturesEXT val_features = {};
        val_features.sType                          = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
        val_features.enabledValidationFeatureCount  = 1;
        val_features.pEnabledValidationFeatures     = enabled_features;

        if (want_validation && have_debug_utils)
        {
            val_features.pNext = &dbg_ci;
            create_info.pNext  = &val_features;
        }

        VkResult result = vkCreateInstance(&create_info, nullptr, &sInstance);

        if (result != VK_SUCCESS)
        {
            return false;
        }

        volkLoadInstanceOnly(sInstance);

        if (want_validation && have_debug_utils && vkCreateDebugUtilsMessengerEXT != nullptr)
        {
            VkResult dbg_res = vkCreateDebugUtilsMessengerEXT(sInstance, &dbg_ci, nullptr, &sDebugMessenger);
            if (dbg_res != VK_SUCCESS)
            {
                sDebugMessenger = VK_NULL_HANDLE;
            }
        }
        return true;
    }

    int scoreDeviceType(VkPhysicalDeviceType type)
    {
        switch (type)
        {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   return 100;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return 50;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    return 20;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:            return 10;
        default:                                     return 0;
        }
    }

    bool selectPhysicalDevice()
    {
        U32 count = 0;
        vkEnumeratePhysicalDevices(sInstance, &count, nullptr);
        if (count == 0)
        {
            return false;
        }

        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(sInstance, &count, devices.data());

        VkPhysicalDevice best = VK_NULL_HANDLE;
        int best_score = -1;

        for (auto dev : devices)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(dev, &props);

            if (props.apiVersion < VK_API_VERSION_1_3)
            {
                continue;
            }

            int score = scoreDeviceType(props.deviceType);

            if (score > best_score)
            {
                best_score = score;
                best = dev;
            }
        }

        if (best == VK_NULL_HANDLE)
        {
            return false;
        }

        sPhysicalDevice = best;

        vkGetPhysicalDeviceProperties(sPhysicalDevice, &sPhysicalDeviceProperties);

        {
            VkPhysicalDeviceDriverProperties driver_props = {};
            driver_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;

            VkPhysicalDeviceProperties2 props2 = {};
            props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            props2.pNext = &driver_props;

            vkGetPhysicalDeviceProperties2(sPhysicalDevice, &props2);

            sDriverName = driver_props.driverName;
            sDriverInfo = driver_props.driverInfo;
        }

        {
            VkPhysicalDeviceMemoryProperties mem_props = {};
            vkGetPhysicalDeviceMemoryProperties(sPhysicalDevice, &mem_props);

            VkDeviceSize largest_device_local = 0;
            for (U32 i = 0; i < mem_props.memoryHeapCount; i++)
            {
                if ((mem_props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) &&
                    mem_props.memoryHeaps[i].size > largest_device_local)
                {
                    largest_device_local = mem_props.memoryHeaps[i].size;
                }
            }
            sDeviceLocalMemoryMB = (U32)(largest_device_local / (1024 * 1024));
        }

        return true;
    }

    bool selectQueueFamily()
    {
        U32 count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(sPhysicalDevice, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(sPhysicalDevice, &count, families.data());

        for (U32 i = 0; i < count; i++)
        {
            if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                sGraphicsQueueFamily = i;
                sTimestampValidBits  = families[i].timestampValidBits;
                return true;
            }
        }

        return false;
    }

    bool queryAndLogDeviceLimits()
    {
        U32 ext_count = 0;
        vkEnumerateDeviceExtensionProperties(sPhysicalDevice, nullptr, &ext_count, nullptr);
        std::vector<VkExtensionProperties> exts(ext_count);
        if (ext_count > 0)
        {
            vkEnumerateDeviceExtensionProperties(sPhysicalDevice, nullptr, &ext_count, exts.data());
        }

        bool mem_budget_supported = false;
        for (const auto& e : exts)
        {
            const std::string name(e.extensionName);
            if (name == VK_EXT_MEMORY_BUDGET_EXTENSION_NAME)
            {
                mem_budget_supported = true;
            }
        }
        sDeviceLimits.memoryBudgetSupported = mem_budget_supported;

        return true;
    }

    bool createDevice()
    {
        queryAndLogDeviceLimits();

        F32 priorities[] = { 1.0f };

        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = sGraphicsQueueFamily;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = priorities;

        VkPhysicalDeviceFeatures supported_features = {};
        vkGetPhysicalDeviceFeatures(sPhysicalDevice, &supported_features);

        VkPhysicalDeviceFeatures enabled_features = {};
        if (supported_features.shaderClipDistance)
        {
            enabled_features.shaderClipDistance = VK_TRUE;
        }

        if (supported_features.depthClamp)
        {
            enabled_features.depthClamp = VK_TRUE;
        }

        if (supported_features.wideLines)
        {
            enabled_features.wideLines = VK_TRUE;
            sMaxLineWidth = sPhysicalDeviceProperties.limits.lineWidthRange[1];
        }
        else
        {
            sMaxLineWidth = 1.0f;
        }

        if (supported_features.imageCubeArray)
        {
            enabled_features.imageCubeArray = VK_TRUE;
            sImageCubeArrayEnabled = true;
        }

        if (supported_features.fillModeNonSolid)
        {
            enabled_features.fillModeNonSolid = VK_TRUE;
        }

        if (supported_features.geometryShader)
        {
            enabled_features.geometryShader = VK_TRUE;
            sGeometryShaderEnabled = true;
        }
        else
        {
            sGeometryShaderEnabled = false;
        }

        if (supported_features.samplerAnisotropy)
        {
            enabled_features.samplerAnisotropy = VK_TRUE;
            sSamplerAnisotropyEnabled = true;
            sMaxSamplerAnisotropy = sPhysicalDeviceProperties.limits.maxSamplerAnisotropy;
        }
        else
        {
            sSamplerAnisotropyEnabled = false;
            sMaxSamplerAnisotropy     = 1.0f;
        }

        // B.2: vertex-stage SSBO atomics for the skin A/B oracle (diagnostic self-check).
        if (supported_features.vertexPipelineStoresAndAtomics)
        {
            enabled_features.vertexPipelineStoresAndAtomics = VK_TRUE;
            sVertexStoresAtomicsEnabled = true;
        }
        LL_INFOS("Vulkan") << "B.2 skin A/B: vertexPipelineStoresAndAtomics supported="
                           << (int)supported_features.vertexPipelineStoresAndAtomics
                           << " enabled=" << (int)sVertexStoresAtomicsEnabled << LL_ENDL;

        VkPhysicalDeviceDynamicRenderingFeatures dr_features_query = {};
        dr_features_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;

        VkPhysicalDeviceFeatures2 features2_query = {};
        features2_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features2_query.pNext = &dr_features_query;

        vkGetPhysicalDeviceFeatures2(sPhysicalDevice, &features2_query);

        VkPhysicalDeviceDynamicRenderingFeatures dr_features_enable = {};
        dr_features_enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
        if (dr_features_query.dynamicRendering)
        {
            dr_features_enable.dynamicRendering = VK_TRUE;
        }

        std::vector<const char*> device_extensions;
        if (sDeviceLimits.memoryBudgetSupported)
        {
            device_extensions.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
        }
        device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        bool device_fault_supported = false;
        bool provoking_vertex_supported = false;
        bool checkpoints_supported = false;
        bool present_id_supported = false;
        bool present_wait_supported = false;
        {
            U32 ext_count = 0;
            vkEnumerateDeviceExtensionProperties(sPhysicalDevice, nullptr, &ext_count, nullptr);
            std::vector<VkExtensionProperties> exts(ext_count);
            vkEnumerateDeviceExtensionProperties(sPhysicalDevice, nullptr, &ext_count, exts.data());
            for (const auto& e : exts)
            {
                if (std::strcmp(e.extensionName, "VK_EXT_device_fault") == 0)
                {
                    device_fault_supported = true;
                }
                else if (std::strcmp(e.extensionName, VK_EXT_PROVOKING_VERTEX_EXTENSION_NAME) == 0)
                {
                    provoking_vertex_supported = true;
                }
                else if (std::strcmp(e.extensionName, "VK_NV_device_diagnostic_checkpoints") == 0)
                {
                    checkpoints_supported = true;
                }
                else if (std::strcmp(e.extensionName, "VK_KHR_present_id") == 0)
                {
                    present_id_supported = true;
                }
                else if (std::strcmp(e.extensionName, "VK_KHR_present_wait") == 0)
                {
                    present_wait_supported = true;
                }
            }
        }
        if (device_fault_supported)
        {
            device_extensions.push_back("VK_EXT_device_fault");
        }
        if (checkpoints_supported)
        {
            device_extensions.push_back("VK_NV_device_diagnostic_checkpoints");
            sCheckpointsEnabled = true;
        }

        VkPhysicalDevicePresentIdFeaturesKHR present_id_features_enable = {};
        present_id_features_enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR;
        VkPhysicalDevicePresentWaitFeaturesKHR present_wait_features_enable = {};
        present_wait_features_enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR;
        if (present_id_supported && present_wait_supported)
        {
            VkPhysicalDevicePresentIdFeaturesKHR pid_query = {};
            pid_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR;
            VkPhysicalDevicePresentWaitFeaturesKHR pwait_query = {};
            pwait_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR;
            pid_query.pNext = &pwait_query;
            VkPhysicalDeviceFeatures2 pw_f2 = {};
            pw_f2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            pw_f2.pNext = &pid_query;
            vkGetPhysicalDeviceFeatures2(sPhysicalDevice, &pw_f2);
            if (pid_query.presentId && pwait_query.presentWait)
            {
                device_extensions.push_back("VK_KHR_present_id");
                device_extensions.push_back("VK_KHR_present_wait");
                present_id_features_enable.presentId = VK_TRUE;
                present_wait_features_enable.presentWait = VK_TRUE;
                sPresentWaitEnabled = true;
            }
        }

        VkPhysicalDeviceProvokingVertexFeaturesEXT pv_features_enable = {};
        pv_features_enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_FEATURES_EXT;
        if (provoking_vertex_supported)
        {
            VkPhysicalDeviceProvokingVertexFeaturesEXT pv_query = {};
            pv_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_FEATURES_EXT;
            VkPhysicalDeviceFeatures2 f2 = {};
            f2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            f2.pNext = &pv_query;
            vkGetPhysicalDeviceFeatures2(sPhysicalDevice, &f2);
            if (pv_query.provokingVertexLast)
            {
                device_extensions.push_back(VK_EXT_PROVOKING_VERTEX_EXTENSION_NAME);
                pv_features_enable.provokingVertexLast = VK_TRUE;
                sProvokingVertexLastEnabled = true;
            }
        }

        if (sProvokingVertexLastEnabled)
        {
            dr_features_enable.pNext = &pv_features_enable;
        }

        VkPhysicalDeviceHostQueryResetFeatures hqr_features_enable = {};
        hqr_features_enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES;
        {
            VkPhysicalDeviceHostQueryResetFeatures hqr_query = {};
            hqr_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES;
            VkPhysicalDeviceFeatures2 hqr_f2 = {};
            hqr_f2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            hqr_f2.pNext = &hqr_query;
            vkGetPhysicalDeviceFeatures2(sPhysicalDevice, &hqr_f2);
            if (hqr_query.hostQueryReset)
            {
                hqr_features_enable.hostQueryReset = VK_TRUE;
                sHostQueryResetEnabled = true;
            }
        }

        VkPhysicalDeviceVulkan12Features vk12_features_enable = {};
        vk12_features_enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        {
            VkPhysicalDeviceVulkan12Features vk12_query = {};
            vk12_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
            VkPhysicalDeviceFeatures2 vk12_f2 = {};
            vk12_f2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            vk12_f2.pNext = &vk12_query;
            vkGetPhysicalDeviceFeatures2(sPhysicalDevice, &vk12_f2);

            const bool bindless_ok =
                vk12_query.runtimeDescriptorArray &&
                vk12_query.shaderSampledImageArrayNonUniformIndexing &&
                vk12_query.descriptorBindingPartiallyBound &&
                vk12_query.descriptorBindingSampledImageUpdateAfterBind &&
                vk12_query.descriptorBindingUpdateUnusedWhilePending &&
                vk12_query.descriptorBindingVariableDescriptorCount;

            if (bindless_ok)
            {
                vk12_features_enable.runtimeDescriptorArray                    = VK_TRUE;
                vk12_features_enable.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
                vk12_features_enable.descriptorBindingPartiallyBound           = VK_TRUE;
                vk12_features_enable.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
                vk12_features_enable.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
                vk12_features_enable.descriptorBindingVariableDescriptorCount  = VK_TRUE;
                sBindlessCapable = true;

                VkPhysicalDeviceVulkan12Properties vk12_props = {};
                vk12_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
                VkPhysicalDeviceProperties2 vk12_p2 = {};
                vk12_p2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
                vk12_p2.pNext = &vk12_props;
                vkGetPhysicalDeviceProperties2(sPhysicalDevice, &vk12_p2);
                sBindlessHeapCapacity = llmin(vk12_props.maxDescriptorSetUpdateAfterBindSampledImages, 65536u);
            }

            if (supported_features.multiDrawIndirect)
            {
                enabled_features.multiDrawIndirect = VK_TRUE;
                sMultiDrawIndirectEnabled = true;
            }
            if (supported_features.drawIndirectFirstInstance)
            {
                enabled_features.drawIndirectFirstInstance = VK_TRUE;
                sDrawIndirectFirstInstanceEnabled = true;
            }
        }

        VkDeviceCreateInfo device_info = {};
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        if (sBindlessCapable)
        {
            vk12_features_enable.hostQueryReset = sHostQueryResetEnabled ? VK_TRUE : VK_FALSE;
            vk12_features_enable.pNext = &dr_features_enable;
            device_info.pNext = &vk12_features_enable;
        }
        else
        {
            if (sHostQueryResetEnabled)
            {
                if (sProvokingVertexLastEnabled)
                {
                    pv_features_enable.pNext = &hqr_features_enable;
                }
                else
                {
                    dr_features_enable.pNext = &hqr_features_enable;
                }
            }
            device_info.pNext = &dr_features_enable;
        }
        if (sPresentWaitEnabled)
        {
            present_id_features_enable.pNext   = const_cast<void*>(device_info.pNext);
            present_wait_features_enable.pNext = &present_id_features_enable;
            device_info.pNext                  = &present_wait_features_enable;
        }
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &queue_info;
        device_info.enabledExtensionCount = (U32)device_extensions.size();
        device_info.ppEnabledExtensionNames = device_extensions.empty() ? nullptr : device_extensions.data();
        device_info.pEnabledFeatures = &enabled_features;

        VkResult result = vkCreateDevice(sPhysicalDevice, &device_info, nullptr, &sDevice);
        if (result != VK_SUCCESS)
        {
            return false;
        }

        LL_INFOS("Vulkan") << "VKCaps: bindless=" << (sBindlessCapable ? 1 : 0)
                           << " heap=" << sBindlessHeapCapacity
                           << " mdi=" << (sMultiDrawIndirectEnabled ? 1 : 0)
                           << " mdi_fi=" << (sDrawIndirectFirstInstanceEnabled ? 1 : 0)
                           << " dynUBO=" << sPhysicalDeviceProperties.limits.maxDescriptorSetUniformBuffersDynamic
                           << LL_ENDL;
        if (sPhysicalDeviceProperties.limits.maxDescriptorSetUniformBuffersDynamic < LLGLSLShader::MAX_VK_DYNAMIC_BINDINGS)
        {
            LL_WARNS("Vulkan") << "dynamic UBO budget: device maxDescriptorSetUniformBuffersDynamic="
                               << sPhysicalDeviceProperties.limits.maxDescriptorSetUniformBuffersDynamic
                               << " < required " << LLGLSLShader::MAX_VK_DYNAMIC_BINDINGS
                               << " (scene per-draw dynamic bindings may exceed device limit)" << LL_ENDL;
        }

        volkLoadDevice(sDevice);
        if (sCheckpointsEnabled && (vkCmdSetCheckpointNV == nullptr || vkGetQueueCheckpointDataNV == nullptr))
        {
            sCheckpointsEnabled = false;
        }
        LL_INFOS("Vulkan") << "GPU breadcrumb checkpoints enabled=" << (sCheckpointsEnabled ? 1 : 0) << LL_ENDL;
        LL_INFOS("Vulkan") << "present_wait (vsync sleep) enabled=" << (sPresentWaitEnabled ? 1 : 0) << LL_ENDL;
        vkGetDeviceQueue(sDevice, sGraphicsQueueFamily, 0, &sGraphicsQueue);


        return true;
    }

    S32 findMemoryType(U32 type_filter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties mem_props;
        vkGetPhysicalDeviceMemoryProperties(sPhysicalDevice, &mem_props);

        for (U32 i = 0; i < mem_props.memoryTypeCount; i++)
        {
            if ((type_filter & (1u << i)) &&
                (mem_props.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return (S32)i;
            }
        }
        return -1;
    }

    bool createCommandPool()
    {
        VkCommandPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                          VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = sGraphicsQueueFamily;

        VkResult result = vkCreateCommandPool(sDevice, &pool_info, nullptr, &sCommandPool);
        if (result != VK_SUCCESS)
        {
            return false;
        }

        if (sHostQueryResetEnabled)
        {
            VkQueryPoolCreateInfo qp_info = {};
            qp_info.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            qp_info.queryType  = VK_QUERY_TYPE_OCCLUSION;
            qp_info.queryCount = 65536;
            VkResult qp_result = vkCreateQueryPool(sDevice, &qp_info, nullptr, &sOcclusionQueryPool);
            if (qp_result == VK_SUCCESS)
            {
                sOcclusionQueryCapacity = qp_info.queryCount;
                vkResetQueryPool(sDevice, sOcclusionQueryPool, 0, sOcclusionQueryCapacity);
                for (U32 i = 0; i < sOcclusionQueryCapacity; ++i)
                {
                    sOcclusionQueryFree.push(i);
                }
            }
            else
            {
                sOcclusionQueryPool = VK_NULL_HANDLE;
            }
        }

        {
            sTimestampPeriodNs = sPhysicalDeviceProperties.limits.timestampPeriod;

            if (sHostQueryResetEnabled && sTimestampValidBits > 0 && sTimestampPeriodNs > 0.0f)
            {
                const U32 pair_capacity = 256;
                VkQueryPoolCreateInfo tqp_info = {};
                tqp_info.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
                tqp_info.queryType  = VK_QUERY_TYPE_TIMESTAMP;
                tqp_info.queryCount = pair_capacity * 2;
                VkResult tqp_result = vkCreateQueryPool(sDevice, &tqp_info, nullptr, &sTimestampQueryPool);
                if (tqp_result == VK_SUCCESS)
                {
                    sTimestampPairCapacity = pair_capacity;
                    vkResetQueryPool(sDevice, sTimestampQueryPool, 0, tqp_info.queryCount);
                    for (U32 i = 0; i < sTimestampPairCapacity; ++i)
                    {
                        sTimestampPairFree.push(i);
                    }
                    sTimestampSupported = true;
                }
                else
                {
                    sTimestampQueryPool = VK_NULL_HANDLE;
                    sTimestampSupported = false;
                }
            }
            else
            {
                sTimestampSupported = false;
            }
        }

        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = sCommandPool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = FRAMES_IN_FLIGHT;

        result = vkAllocateCommandBuffers(sDevice, &alloc_info, sCommandBuffers);
        if (result != VK_SUCCESS)
        {
            return false;
        }

        result = vkAllocateCommandBuffers(sDevice, &alloc_info, sConsumerCommandBuffers);
        if (result != VK_SUCCESS)
        {
            return false;
        }

        VkCommandBufferAllocateInfo async_alloc = {};
        async_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        async_alloc.commandPool = sCommandPool;
        async_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        async_alloc.commandBufferCount = 1;
        result = vkAllocateCommandBuffers(sDevice, &async_alloc, &sAsyncProducerCommandBuffer);
        if (result != VK_SUCCESS)
        {
            return false;
        }

        return true;
    }

    bool createDefaultFallbackImage()
    {
        VkImageCreateInfo image_info = {};
        image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType     = VK_IMAGE_TYPE_2D;
        image_info.format        = VK_FORMAT_R8G8B8A8_UNORM;
        image_info.extent        = { 1, 1, 1 };
        image_info.mipLevels     = 1;
        image_info.arrayLayers   = 1;
        image_info.samples       = VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkResult result = vkCreateImage(sDevice, &image_info, nullptr, &sDefaultFallbackImage);
        if (result != VK_SUCCESS)
        {
            return false;
        }

        VkMemoryRequirements mem_req;
        vkGetImageMemoryRequirements(sDevice, sDefaultFallbackImage, &mem_req);

        S32 mem_type = findMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (mem_type < 0)
        {
            return false;
        }

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize  = mem_req.size;
        alloc_info.memoryTypeIndex = (U32)mem_type;

        result = vkAllocateMemory(sDevice, &alloc_info, nullptr, &sDefaultFallbackMemory);
        if (result != VK_SUCCESS)
        {
            return false;
        }
        vkBindImageMemory(sDevice, sDefaultFallbackImage, sDefaultFallbackMemory, 0);

        VkBuffer staging_buf = VK_NULL_HANDLE;
        VkDeviceMemory staging_mem = VK_NULL_HANDLE;
        {
            VkBufferCreateInfo bi = {};
            bi.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bi.size        = 4;
            bi.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            if (vkCreateBuffer(sDevice, &bi, nullptr, &staging_buf) != VK_SUCCESS)
            {
                return false;
            }
            VkMemoryRequirements smr;
            vkGetBufferMemoryRequirements(sDevice, staging_buf, &smr);
            S32 smt = findMemoryType(smr.memoryTypeBits,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            if (smt < 0)
            {
                return false;
            }
            VkMemoryAllocateInfo sai = {};
            sai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            sai.allocationSize  = smr.size;
            sai.memoryTypeIndex = (U32)smt;
            if (vkAllocateMemory(sDevice, &sai, nullptr, &staging_mem) != VK_SUCCESS)
            {
                return false;
            }
            vkBindBufferMemory(sDevice, staging_buf, staging_mem, 0);
            void* mapped = nullptr;
            vkMapMemory(sDevice, staging_mem, 0, 4, 0, &mapped);
            const U32 white_pixel = 0xFFFFFFFFu;
            memcpy(mapped, &white_pixel, 4);
            vkUnmapMemory(sDevice, staging_mem);
        }

        {
            VkCommandBufferAllocateInfo cba = {};
            cba.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cba.commandPool        = sCommandPool;
            cba.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cba.commandBufferCount = 1;
            VkCommandBuffer one_cmd = VK_NULL_HANDLE;
            vkAllocateCommandBuffers(sDevice, &cba, &one_cmd);

            VkCommandBufferBeginInfo cbbi = {};
            cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(one_cmd, &cbbi);

            VkImageMemoryBarrier b1 = {};
            b1.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            b1.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
            b1.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            b1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b1.image               = sDefaultFallbackImage;
            b1.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            b1.srcAccessMask       = 0;
            b1.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
            vkCmdPipelineBarrier(one_cmd,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0, 0, nullptr, 0, nullptr, 1, &b1);

            VkBufferImageCopy region = {};
            region.bufferOffset      = 0;
            region.imageSubresource  = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
            region.imageExtent       = { 1, 1, 1 };
            vkCmdCopyBufferToImage(one_cmd, staging_buf, sDefaultFallbackImage,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            VkImageMemoryBarrier b2 = b1;
            b2.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            b2.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            b2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            b2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(one_cmd,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0, 0, nullptr, 0, nullptr, 1, &b2);

            vkEndCommandBuffer(one_cmd);

            peSubmitBlocking(one_cmd, VK_NULL_HANDLE, true);

            vkFreeCommandBuffers(sDevice, sCommandPool, 1, &one_cmd);
            vkDestroyBuffer(sDevice, staging_buf, nullptr);
            vkFreeMemory(sDevice, staging_mem, nullptr);
        }

        VkImageViewCreateInfo vci = {};
        vci.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image                 = sDefaultFallbackImage;
        vci.viewType              = VK_IMAGE_VIEW_TYPE_2D;
        vci.format                = VK_FORMAT_R8G8B8A8_UNORM;
        vci.subresourceRange      = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        if (vkCreateImageView(sDevice, &vci, nullptr, &sDefaultFallbackImageView) != VK_SUCCESS)
        {
            return false;
        }
        noteViewHandleCreated(sDefaultFallbackImageView);

        return true;
    }

    bool createDefaultFallbackShadowImage()
    {
        VkImageCreateInfo image_info = {};
        image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType     = VK_IMAGE_TYPE_2D;
        image_info.format        = VK_FORMAT_D32_SFLOAT;
        image_info.extent        = { 1, 1, 1 };
        image_info.mipLevels     = 1;
        image_info.arrayLayers   = 1;
        image_info.samples       = VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(sDevice, &image_info, nullptr, &sDefaultFallbackShadowImage) != VK_SUCCESS)
        {
            return false;
        }

        VkMemoryRequirements mem_req;
        vkGetImageMemoryRequirements(sDevice, sDefaultFallbackShadowImage, &mem_req);

        S32 mem_type = findMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (mem_type < 0)
        {
            return false;
        }

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize  = mem_req.size;
        alloc_info.memoryTypeIndex = (U32)mem_type;

        if (vkAllocateMemory(sDevice, &alloc_info, nullptr, &sDefaultFallbackShadowMemory) != VK_SUCCESS)
        {
            return false;
        }
        vkBindImageMemory(sDevice, sDefaultFallbackShadowImage, sDefaultFallbackShadowMemory, 0);

        {
            VkCommandBufferAllocateInfo cba = {};
            cba.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cba.commandPool        = sCommandPool;
            cba.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cba.commandBufferCount = 1;
            VkCommandBuffer one_cmd = VK_NULL_HANDLE;
            vkAllocateCommandBuffers(sDevice, &cba, &one_cmd);

            VkCommandBufferBeginInfo cbbi = {};
            cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(one_cmd, &cbbi);

            VkImageMemoryBarrier b1 = {};
            b1.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            b1.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
            b1.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            b1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b1.image               = sDefaultFallbackShadowImage;
            b1.subresourceRange    = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
            b1.srcAccessMask       = 0;
            b1.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
            vkCmdPipelineBarrier(one_cmd,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0, 0, nullptr, 0, nullptr, 1, &b1);

            VkClearDepthStencilValue far_depth = { 1.f, 0 };
            VkImageSubresourceRange depth_range = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
            vkCmdClearDepthStencilImage(one_cmd, sDefaultFallbackShadowImage,
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                        &far_depth, 1, &depth_range);

            VkImageMemoryBarrier b2 = b1;
            b2.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            b2.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            b2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            b2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(one_cmd,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0, 0, nullptr, 0, nullptr, 1, &b2);

            vkEndCommandBuffer(one_cmd);

            peSubmitBlocking(one_cmd, VK_NULL_HANDLE, true);

            vkFreeCommandBuffers(sDevice, sCommandPool, 1, &one_cmd);
        }

        VkImageViewCreateInfo vci = {};
        vci.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image                 = sDefaultFallbackShadowImage;
        vci.viewType              = VK_IMAGE_VIEW_TYPE_2D;
        vci.format                = VK_FORMAT_D32_SFLOAT;
        vci.subresourceRange      = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
        if (vkCreateImageView(sDevice, &vci, nullptr, &sDefaultFallbackShadowImageView) != VK_SUCCESS)
        {
            return false;
        }
        noteViewHandleCreated(sDefaultFallbackShadowImageView);

        return true;
    }

    bool createDefaultFallbackCubeArrayImage()
    {
        if (!createCubeArrayImageVk(1, 1, 1, VK_FORMAT_R8G8B8A8_UNORM,
                                    sDefaultFallbackCubeArrayImage,
                                    sDefaultFallbackCubeArrayImageView,
                                    sDefaultFallbackCubeArrayAlloc))
        {
            return false;
        }

        VkCommandBufferAllocateInfo cba = {};
        cba.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cba.commandPool        = sCommandPool;
        cba.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cba.commandBufferCount = 1;
        VkCommandBuffer one_cmd = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(sDevice, &cba, &one_cmd);

        VkCommandBufferBeginInfo cbbi = {};
        cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(one_cmd, &cbbi);

        VkImageSubresourceRange full = {};
        full.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        full.baseMipLevel   = 0;
        full.levelCount     = 1;
        full.baseArrayLayer = 0;
        full.layerCount     = 6;

        VkImageMemoryBarrier to_dst = {};
        to_dst.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        to_dst.oldLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        to_dst.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_dst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_dst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_dst.image               = sDefaultFallbackCubeArrayImage;
        to_dst.subresourceRange    = full;
        to_dst.srcAccessMask       = VK_ACCESS_SHADER_READ_BIT;
        to_dst.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(one_cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &to_dst);

        VkClearColorValue black = {};
        vkCmdClearColorImage(one_cmd, sDefaultFallbackCubeArrayImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             &black, 1, &full);

        VkImageMemoryBarrier to_read = to_dst;
        to_read.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_read.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        to_read.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_read.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(one_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &to_read);

        vkEndCommandBuffer(one_cmd);
        peSubmitBlocking(one_cmd, VK_NULL_HANDLE, true);
        vkFreeCommandBuffers(sDevice, sCommandPool, 1, &one_cmd);

        return true;
    }

    bool createDefaultFallbackCubeImage()
    {
        if (!createCubeImageVk(1, VK_FORMAT_R8G8B8A8_UNORM, 1,
                               sDefaultFallbackCubeImage,
                               sDefaultFallbackCubeImageView,
                               sDefaultFallbackCubeAlloc))
        {
            return false;
        }

        VkCommandBufferAllocateInfo cba = {};
        cba.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cba.commandPool        = sCommandPool;
        cba.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cba.commandBufferCount = 1;
        VkCommandBuffer one_cmd = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(sDevice, &cba, &one_cmd);

        VkCommandBufferBeginInfo cbbi = {};
        cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(one_cmd, &cbbi);

        VkImageSubresourceRange full = {};
        full.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        full.baseMipLevel   = 0;
        full.levelCount     = 1;
        full.baseArrayLayer = 0;
        full.layerCount     = 6;

        VkImageMemoryBarrier to_dst = {};
        to_dst.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        to_dst.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
        to_dst.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_dst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_dst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_dst.image               = sDefaultFallbackCubeImage;
        to_dst.subresourceRange    = full;
        to_dst.srcAccessMask       = 0;
        to_dst.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(one_cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &to_dst);

        VkClearColorValue black = {};
        vkCmdClearColorImage(one_cmd, sDefaultFallbackCubeImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             &black, 1, &full);

        VkImageMemoryBarrier to_read = to_dst;
        to_read.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_read.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        to_read.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_read.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(one_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &to_read);

        vkEndCommandBuffer(one_cmd);
        peSubmitBlocking(one_cmd, VK_NULL_HANDLE, true);
        vkFreeCommandBuffers(sDevice, sCommandPool, 1, &one_cmd);

        return true;
    }

    bool createDefaultFallback3DImage()
    {
        if (!createTexture3DImageVk(1, 1, 1, VK_FORMAT_R8G8B8A8_UNORM,
                                    sDefaultFallback3DImage,
                                    sDefaultFallback3DImageView,
                                    sDefaultFallback3DAlloc))
        {
            return false;
        }

        VkCommandBufferAllocateInfo cba = {};
        cba.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cba.commandPool        = sCommandPool;
        cba.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cba.commandBufferCount = 1;
        VkCommandBuffer one_cmd = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(sDevice, &cba, &one_cmd);

        VkCommandBufferBeginInfo cbbi = {};
        cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(one_cmd, &cbbi);

        VkImageSubresourceRange full = {};
        full.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        full.baseMipLevel   = 0;
        full.levelCount     = 1;
        full.baseArrayLayer = 0;
        full.layerCount     = 1;

        VkImageMemoryBarrier to_dst = {};
        to_dst.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        to_dst.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
        to_dst.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_dst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_dst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_dst.image               = sDefaultFallback3DImage;
        to_dst.subresourceRange    = full;
        to_dst.srcAccessMask       = 0;
        to_dst.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(one_cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &to_dst);

        VkClearColorValue black = {};
        vkCmdClearColorImage(one_cmd, sDefaultFallback3DImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             &black, 1, &full);

        VkImageMemoryBarrier to_read = to_dst;
        to_read.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_read.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        to_read.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_read.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(one_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &to_read);

        vkEndCommandBuffer(one_cmd);
        peSubmitBlocking(one_cmd, VK_NULL_HANDLE, true);
        vkFreeCommandBuffers(sDevice, sCommandPool, 1, &one_cmd);

        return true;
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
    }

    bool createBufferVkImpl(U32                 size_bytes,
                            VkBufferUsageFlags  usage,
                            VkBuffer&           out_buffer,
                            void*&              out_allocation,
                            void**              out_mapped,
                            bool                prefer_device);

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
        sSkinBaseSet       = VK_NULL_HANDLE;
        sBindlessHeapSet   = VK_NULL_HANDLE;
        sBindlessHeapCount = 0;
        sBindlessSlotNext  = 1;
        sBindlessSlotFreeList.clear();
        sPendingSlotFrees.clear();
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

        VkDescriptorSetLayoutBinding bindings[3] = {};
        bindings[0].binding         = 0;
        bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[1].binding         = 1;
        bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].descriptorCount = count;
        bindings[1].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[2].binding         = 2;
        bindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorBindingFlags bind_flags[3] = {
            0,
              VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
            | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
            | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
            0
        };

        VkDescriptorSetLayoutBindingFlagsCreateInfo bf = {};
        bf.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        bf.bindingCount  = 3;
        bf.pBindingFlags = bind_flags;

        VkDescriptorSetLayoutCreateInfo li = {};
        li.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        li.pNext        = &bf;
        li.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        li.bindingCount = 3;
        li.pBindings    = bindings;

        if (vkCreateDescriptorSetLayout(sDevice, &li, nullptr, &sBindlessHeapLayout) != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "VKBindless: heap layout creation failed (staying inactive)" << LL_ENDL;
            sBindlessHeapLayout = VK_NULL_HANDLE;
            return true;
        }

        VkDescriptorPoolSize ps[2] = {};
        ps[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        ps[0].descriptorCount = 2; // DrawData(0) + skin palette(2)
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
            VkDescriptorSetLayoutBinding sb = {};
            sb.binding         = 0;
            sb.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            sb.descriptorCount = 1;
            sb.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
            VkDescriptorSetLayoutCreateInfo sli = {};
            sli.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            sli.bindingCount = 1;
            sli.pBindings    = &sb;
            if (vkCreateDescriptorSetLayout(sDevice, &sli, nullptr, &sSkinBaseLayout) != VK_SUCCESS)
            {
                LL_WARNS("Vulkan") << "VKBindless: skin base layout creation failed (staying inactive)" << LL_ENDL;
                destroyBindlessHeap();
                return true;
            }
            VkDescriptorPoolSize sps = {};
            sps.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            sps.descriptorCount = 1;
            VkDescriptorPoolCreateInfo spi = {};
            spi.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            spi.maxSets       = 1;
            spi.poolSizeCount = 1;
            spi.pPoolSizes    = &sps;
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

        // Fixed-size allocation: binding 1 gets its full `count` descriptors (no
        // variable-count info, since binding 1 is no longer the highest binding).
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
            void* dd_mapped = nullptr;
            if (createBufferVkImpl(DRAWDATA_TOTAL_SLOTS * 16,
                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                   sDrawDataBuffer, sDrawDataAllocation, &dd_mapped, true)
                && dd_mapped != nullptr)
            {
                sDrawDataMapped = reinterpret_cast<U32*>(dd_mapped);
                std::memset(sDrawDataMapped, 0, 16);

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

        // B.0/B.2: skin palette SSBO (b2) + skin base SSBO (b3).
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
                sw[0].dstSet          = sBindlessHeapSet;
                sw[0].dstBinding      = 2;
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
                if (sSkinPaletteBuffer != VK_NULL_HANDLE)
                {
                    destroyBufferVk(sSkinPaletteBuffer, sSkinPaletteAllocation);
                    sSkinPaletteBuffer = VK_NULL_HANDLE; sSkinPaletteAllocation = nullptr; sSkinPaletteMapped = nullptr;
                }
                if (sSkinBaseBuffer != VK_NULL_HANDLE)
                {
                    destroyBufferVk(sSkinBaseBuffer, sSkinBaseAllocation);
                    sSkinBaseBuffer = VK_NULL_HANDLE; sSkinBaseAllocation = nullptr; sSkinBaseMapped = nullptr;
                }
            }
        }

        sBindlessHeapCount = count;
        sBindlessSlotNext  = 1;
        sBindlessActive    = true;
        bindlessWriteSlotInternal(0, VK_NULL_HANDLE, VK_NULL_HANDLE);
        LL_INFOS("Vulkan") << "VKBindless: heap active count=" << count << LL_ENDL;
        return true;
    }

    namespace pcache
    {
        using CacheBlob = std::vector<uint8_t>;

        std::string sFilePath;
        CacheBlob   sBlob;
        std::size_t sMaxSizeBytes = 0;
        bool        sInitialized  = false;

        constexpr U32 kDefaultMaxSizeMB = 64;

        bool readBlobFromDisk(const std::string& path, CacheBlob& out)
        {
            std::ifstream f(path, std::ios::binary | std::ios::ate);
            if (!f.is_open())
            {
                return false;
            }
            const std::streamsize size = f.tellg();
            if (size <= 0)
            {
                return false;
            }
            f.seekg(0, std::ios::beg);
            out.resize(static_cast<std::size_t>(size));
            if (!f.read(reinterpret_cast<char*>(out.data()), size))
            {
                out.clear();
                out.shrink_to_fit();
                return false;
            }
            return true;
        }

        bool writeBlobToDisk(const std::string& path, const CacheBlob& data)
        {
            std::ofstream f(path, std::ios::binary | std::ios::trunc);
            if (!f.is_open())
            {
                return false;
            }
            if (!data.empty())
            {
                f.write(reinterpret_cast<const char*>(data.data()),
                        static_cast<std::streamsize>(data.size()));
            }
            return f.good();
        }
    }

    bool createPipelineCacheStorage()
    {
        static LLCachedControl<U32> sPipelineCacheSizeMB(
            gSavedSettings, "AYAPipelineCacheSizeMB", pcache::kDefaultMaxSizeMB);
        const U32 cap_mb = (U32)sPipelineCacheSizeMB;

        pcache::sFilePath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "pipeline_cache.bin");
        pcache::sMaxSizeBytes = static_cast<std::size_t>(cap_mb) * 1024u * 1024u;
        pcache::sBlob.clear();
        pcache::sInitialized = true;

        if (!pcache::readBlobFromDisk(pcache::sFilePath, pcache::sBlob))
        {
            pcache::sBlob.clear();
        }
        if (pcache::sMaxSizeBytes > 0 && pcache::sBlob.size() > pcache::sMaxSizeBytes)
        {
            pcache::sBlob.clear();
        }

        return true;
    }

    bool createPipelineCache()
    {
        VkPipelineCacheCreateInfo info = {};
        info.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        info.initialDataSize = 0;
        info.pInitialData    = nullptr;
        if (pcache::sInitialized && !pcache::sBlob.empty())
        {
            info.initialDataSize = pcache::sBlob.size();
            info.pInitialData    = pcache::sBlob.data();
        }

        VkResult result = vkCreatePipelineCache(sDevice, &info, nullptr, &sPipelineCache);
        if (result != VK_SUCCESS)
        {
            return false;
        }

        return true;
    }

    bool createVmaAllocator()
    {
        VmaVulkanFunctions vk_funcs = {};
        vk_funcs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vk_funcs.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo info = {};
        info.vulkanApiVersion = VK_API_VERSION_1_3;
        info.instance         = sInstance;
        info.physicalDevice   = sPhysicalDevice;
        info.device           = sDevice;
        info.pVulkanFunctions = &vk_funcs;
        if (sDeviceLimits.memoryBudgetSupported)
        {
            info.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        }

        VkResult result = vmaCreateAllocator(&info, &sAllocator);
        if (result != VK_SUCCESS)
        {
            return false;
        }

        return true;
    }

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

    bool createStandardSampler()
    {
        VkSamplerCreateInfo sci = {};
        sci.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sci.magFilter               = VK_FILTER_LINEAR;
        sci.minFilter               = VK_FILTER_LINEAR;
        sci.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sci.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sci.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sci.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sci.mipLodBias              = 0.0f;
        sci.anisotropyEnable        = VK_FALSE;
        sci.maxAnisotropy           = 1.0f;
        sci.compareEnable           = VK_FALSE;
        sci.compareOp               = VK_COMPARE_OP_NEVER;
        sci.minLod                  = 0.0f;
        sci.maxLod                  = VK_LOD_CLAMP_NONE;
        sci.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        sci.unnormalizedCoordinates = VK_FALSE;

        VkResult result = vkCreateSampler(sDevice, &sci, nullptr, &sStandardLinearSampler);
        if (result != VK_SUCCESS)
        {
            return false;
        }

        return true;
    }

    U32 encodeSamplerStateKey(U32 address_mode, U32 filter_option, bool has_mipmaps, bool compare)
    {
        return (address_mode & 0x3u) | ((filter_option & 0x3u) << 2) | ((has_mipmaps ? 1u : 0u) << 4)
             | ((compare ? 1u : 0u) << 5);
    }

    VkSampler getSamplerForStateImpl(U32 address_mode, U32 filter_option, bool has_mipmaps, bool compare)
    {
        const U32 key = encodeSamplerStateKey(address_mode, filter_option, has_mipmaps, compare);
        auto it = sSamplerCache.find(key);
        if (it != sSamplerCache.end())
        {
            return it->second;
        }

        VkSamplerAddressMode vk_addr;
        switch (address_mode)
        {
            case 1:  vk_addr = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT; break;
            case 2:  vk_addr = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;   break;
            default: vk_addr = VK_SAMPLER_ADDRESS_MODE_REPEAT;          break;
        }

        const VkFilter mag = (filter_option == 0) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;

        VkFilter            vk_min;
        VkSamplerMipmapMode vk_mip;
        if (filter_option >= 2 && has_mipmaps)
        {
            vk_min = VK_FILTER_LINEAR;  vk_mip = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        }
        else if (filter_option >= 1)
        {
            vk_min = VK_FILTER_LINEAR;  vk_mip = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        }
        else
        {
            vk_min = VK_FILTER_NEAREST; vk_mip = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        }

        const bool use_aniso = sSamplerAnisotropyEnabled && (filter_option == 3);

        VkSamplerCreateInfo sci = {};
        sci.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sci.magFilter               = mag;
        sci.minFilter               = vk_min;
        sci.mipmapMode              = vk_mip;
        sci.addressModeU            = vk_addr;
        sci.addressModeV            = vk_addr;
        sci.addressModeW            = vk_addr;
        sci.mipLodBias              = 0.0f;
        sci.anisotropyEnable        = use_aniso ? VK_TRUE : VK_FALSE;
        sci.maxAnisotropy           = use_aniso ? sMaxSamplerAnisotropy : 1.0f;
        sci.compareEnable           = compare ? VK_TRUE : VK_FALSE;
        sci.compareOp               = compare ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_NEVER;
        sci.minLod                  = 0.0f;
        sci.maxLod                  = VK_LOD_CLAMP_NONE;
        sci.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        sci.unnormalizedCoordinates = VK_FALSE;

        VkSampler sampler = VK_NULL_HANDLE;
        VkResult  result  = vkCreateSampler(sDevice, &sci, nullptr, &sampler);
        if (result != VK_SUCCESS || sampler == VK_NULL_HANDLE)
        {
            return sStandardLinearSampler;
        }

        sSamplerCache[key] = sampler;
        return sampler;
    }

    VkFormat llGlEnumToVkFormatImpl(U32 ll_gl_intformat)
    {
        switch (ll_gl_intformat)
        {
            case 0x8229: return VK_FORMAT_R8_UNORM;
            case 0x822B: return VK_FORMAT_R8G8_UNORM;
            case 0x8051: return VK_FORMAT_R8G8B8A8_UNORM;
            case 0x8058: return VK_FORMAT_R8G8B8A8_UNORM;

            case 0x1907: return VK_FORMAT_R8G8B8A8_UNORM;
            case 0x1908: return VK_FORMAT_R8G8B8A8_UNORM;

            case 0x803C: return VK_FORMAT_R8_UNORM;
            case 0x8040: return VK_FORMAT_R8_UNORM;
            case 0x8045: return VK_FORMAT_R8G8_UNORM;
            case 0x1906: return VK_FORMAT_R8_UNORM;
            case 0x1909: return VK_FORMAT_R8_UNORM;
            case 0x190A: return VK_FORMAT_R8G8_UNORM;

            case 0x822A: return VK_FORMAT_R16_UNORM;
            case 0x822C: return VK_FORMAT_R16G16_UNORM;
            case 0x805B: return VK_FORMAT_R16G16B16A16_UNORM;

            case 0x822D: return VK_FORMAT_R16_SFLOAT;
            case 0x822F: return VK_FORMAT_R16G16_SFLOAT;
            case 0x881B: return VK_FORMAT_R16G16B16A16_SFLOAT;
            case 0x881A: return VK_FORMAT_R16G16B16A16_SFLOAT;

            case 0x822E: return VK_FORMAT_R32_SFLOAT;
            case 0x8230: return VK_FORMAT_R32G32_SFLOAT;
            case 0x8815: return VK_FORMAT_R32G32B32A32_SFLOAT;
            case 0x8814: return VK_FORMAT_R32G32B32A32_SFLOAT;

            case 0x8C41: return VK_FORMAT_R8G8B8A8_SRGB;
            case 0x8C43: return VK_FORMAT_R8G8B8A8_SRGB;
            case 0x8C40: return VK_FORMAT_R8G8B8A8_SRGB;
            case 0x8C42: return VK_FORMAT_R8G8B8A8_SRGB;

            case 0x81A5: return VK_FORMAT_D16_UNORM;
            case 0x81A6: return VK_FORMAT_X8_D24_UNORM_PACK32;
            case 0x8CAC: return VK_FORMAT_D32_SFLOAT;
            case 0x88F0: return VK_FORMAT_D24_UNORM_S8_UINT;
            case 0x8CAD: return VK_FORMAT_D32_SFLOAT_S8_UINT;
            case 0x1902: return VK_FORMAT_D24_UNORM_S8_UINT;
            case 0x84F9: return VK_FORMAT_D24_UNORM_S8_UINT;

            case 0x8C3A: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
            case 0x8059: return VK_FORMAT_A2B10G10R10_UNORM_PACK32;

            case 0x8F94: return VK_FORMAT_R8_SNORM;
            case 0x8F95: return VK_FORMAT_R8G8_SNORM;
            case 0x8F96: return VK_FORMAT_R8G8B8A8_SNORM;
            case 0x8F97: return VK_FORMAT_R8G8B8A8_SNORM;

            case 0x83F1: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
            case 0x83F2: return VK_FORMAT_BC2_UNORM_BLOCK;
            case 0x83F3: return VK_FORMAT_BC3_UNORM_BLOCK;
            case 0x8C4D: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
            case 0x8C4E: return VK_FORMAT_BC2_SRGB_BLOCK;
            case 0x8C4F: return VK_FORMAT_BC3_SRGB_BLOCK;

            default:
            {
                return VK_FORMAT_UNDEFINED;
            }
        }
    }

    VkCompareOp llGlEnumToVkCompareOpImpl(U32 ll_gl_func)
    {
        switch (ll_gl_func)
        {
            case 0x0200: return VK_COMPARE_OP_NEVER;
            case 0x0201: return VK_COMPARE_OP_LESS;
            case 0x0202: return VK_COMPARE_OP_EQUAL;
            case 0x0203: return VK_COMPARE_OP_LESS_OR_EQUAL;
            case 0x0204: return VK_COMPARE_OP_GREATER;
            case 0x0205: return VK_COMPARE_OP_NOT_EQUAL;
            case 0x0206: return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case 0x0207: return VK_COMPARE_OP_ALWAYS;
            default:     return VK_COMPARE_OP_LESS;
        }
    }

    VkStencilOp llGlEnumToVkStencilOpImpl(U32 ll_gl_op)
    {
        switch (ll_gl_op)
        {
            case 0x0000: return VK_STENCIL_OP_ZERO;
            case 0x1E00: return VK_STENCIL_OP_KEEP;
            case 0x1E01: return VK_STENCIL_OP_REPLACE;
            case 0x1E02: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
            case 0x1E03: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
            case 0x150A: return VK_STENCIL_OP_INVERT;
            case 0x8507: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
            case 0x8508: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
            default:     return VK_STENCIL_OP_KEEP;
        }
    }

    U32 vkFormatBytesPerPixelImpl(VkFormat format)
    {
        switch (format)
        {
            case VK_FORMAT_R8_UNORM:
            case VK_FORMAT_R8_SNORM:
                return 1;
            case VK_FORMAT_R8G8_UNORM:
            case VK_FORMAT_R8G8_SNORM:
            case VK_FORMAT_R16_UNORM:
            case VK_FORMAT_R16_SFLOAT:
            case VK_FORMAT_D16_UNORM:
                return 2;
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_R8G8B8A8_SNORM:
            case VK_FORMAT_R8G8B8A8_SRGB:
            case VK_FORMAT_B8G8R8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_SRGB:
            case VK_FORMAT_R16G16_UNORM:
            case VK_FORMAT_R16G16_SFLOAT:
            case VK_FORMAT_R32_SFLOAT:
            case VK_FORMAT_X8_D24_UNORM_PACK32:
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
                return 4;
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return 8;
            case VK_FORMAT_R16G16B16A16_UNORM:
            case VK_FORMAT_R16G16B16A16_SFLOAT:
            case VK_FORMAT_R32G32_SFLOAT:
                return 8;
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return 16;
            default:
                return 0;
        }
    }

    U32 llGlFormatSourceComponentsImpl(U32 ll_gl_format)
    {
        switch (ll_gl_format)
        {
            case 0x1903:
            case 0x1906:
            case 0x1909:
            case 0x1902:
                return 1;
            case 0x8227:
            case 0x190A:
            case 0x84F9:
                return 2;
            case 0x1907:
            case 0x80E0:
            case 0x8C40:
                return 3;
            case 0x1908:
            case 0x80E1:
            case 0x8C42:
                return 4;
            default:
                return 4;
        }
    }

    bool createBufferVkImpl(U32                 size_bytes,
                            VkBufferUsageFlags  usage,
                            VkBuffer&           out_buffer,
                            void*&              out_allocation,
                            void**              out_mapped,
                            bool                prefer_device = false)
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

    void setVkObjectName(U64 handle, VkObjectType type, const char* name)
    {
        if (sDevice == VK_NULL_HANDLE || handle == 0 || name == nullptr ||
            vkSetDebugUtilsObjectNameEXT == nullptr)
        {
            return;
        }
        VkDebugUtilsObjectNameInfoEXT info = {};
        info.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        info.objectType   = type;
        info.objectHandle = handle;
        info.pObjectName  = name;
        vkSetDebugUtilsObjectNameEXT(sDevice, &info);
    }

    bool createAttachmentImageVkImpl(U32                width,
                                     U32                height,
                                     VkFormat           format,
                                     VkImageUsageFlags  usage,
                                     VkImageAspectFlags aspect,
                                     const char*        tag,
                                     VkImage&           out_image,
                                     VkImageView&       out_view,
                                     void*&             out_allocation,
                                     U32                mip_levels = 1)
    {
        out_image      = VK_NULL_HANDLE;
        out_view       = VK_NULL_HANDLE;
        out_allocation = nullptr;

        if (width == 0 || height == 0 || format == VK_FORMAT_UNDEFINED)
        {
            return false;
        }
        if (sAllocator == VK_NULL_HANDLE || sDevice == VK_NULL_HANDLE)
        {
            return false;
        }

        VkImageCreateInfo ici = {};
        ici.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ici.imageType     = VK_IMAGE_TYPE_2D;
        ici.format        = format;
        ici.extent.width  = width;
        ici.extent.height = height;
        ici.extent.depth  = 1;
        ici.mipLevels     = (mip_levels > 0) ? mip_levels : 1;
        ici.arrayLayers   = 1;
        ici.samples       = VK_SAMPLE_COUNT_1_BIT;
        ici.tiling        = VK_IMAGE_TILING_OPTIMAL;
        ici.usage         = usage;
        ici.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo aci = {};
        aci.usage         = VMA_MEMORY_USAGE_AUTO;
        aci.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VkImage       image      = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkResult r = vmaCreateImage(sAllocator, &ici, &aci, &image, &allocation, nullptr);
        if (r != VK_SUCCESS)
        {
            return false;
        }

        VkImageViewCreateInfo vci = {};
        vci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image                           = image;
        vci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        vci.format                          = format;
        const bool is_attachment =
            (usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                      | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) != 0;
        if (!is_attachment && format == VK_FORMAT_R8_UNORM)
        {
            vci.components.r = VK_COMPONENT_SWIZZLE_R;
            vci.components.g = VK_COMPONENT_SWIZZLE_R;
            vci.components.b = VK_COMPONENT_SWIZZLE_R;
            vci.components.a = VK_COMPONENT_SWIZZLE_R;
        }
        else if (!is_attachment && format == VK_FORMAT_R8G8_UNORM)
        {
            vci.components.r = VK_COMPONENT_SWIZZLE_R;
            vci.components.g = VK_COMPONENT_SWIZZLE_R;
            vci.components.b = VK_COMPONENT_SWIZZLE_R;
            vci.components.a = VK_COMPONENT_SWIZZLE_G;
        }
        else
        {
            vci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            vci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            vci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            vci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        }
        vci.subresourceRange.aspectMask     = aspect;
        vci.subresourceRange.baseMipLevel   = 0;
        vci.subresourceRange.levelCount     = (mip_levels > 0) ? mip_levels : 1;
        vci.subresourceRange.baseArrayLayer = 0;
        vci.subresourceRange.layerCount     = 1;

        VkImageView view = VK_NULL_HANDLE;
        r = vkCreateImageView(sDevice, &vci, nullptr, &view);
        if (r != VK_SUCCESS)
        {
            vmaDestroyImage(sAllocator, image, allocation);
            return false;
        }
        noteViewHandleCreated(view);

        out_image      = image;
        out_view       = view;
        out_allocation = reinterpret_cast<void*>(allocation);

        if (vkSetDebugUtilsObjectNameEXT != nullptr)
        {
            char namebuf[160];
            snprintf(namebuf, sizeof(namebuf), "%s %ux%u fmt%d mip%u",
                     (tag ? tag : "attImg"), width, height, (int)format,
                     (mip_levels > 0 ? mip_levels : 1));
            setVkObjectName((U64)image, VK_OBJECT_TYPE_IMAGE, namebuf);
            setVkObjectName((U64)view, VK_OBJECT_TYPE_IMAGE_VIEW, namebuf);
        }
        return true;
    }

    bool createSyncObjects()
    {
        VkFenceCreateInfo fence_info = {};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
        {
            VkResult result = vkCreateFence(sDevice, &fence_info, nullptr, &sInFlightFences[i]);
            if (result != VK_SUCCESS)
            {
                for (U32 j = 0; j < i; ++j)
                {
                    vkDestroyFence(sDevice, sInFlightFences[j], nullptr);
                    sInFlightFences[j] = VK_NULL_HANDLE;
                }
                return false;
            }
        }

        VkFenceCreateInfo producer_fence_info = {};
        producer_fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
        {
            sProducerFencePending[i] = false;
            VkResult result = vkCreateFence(sDevice, &producer_fence_info, nullptr, &sProducerFences[i]);
            if (result != VK_SUCCESS)
            {
                for (U32 j = 0; j < i; ++j)
                {
                    vkDestroyFence(sDevice, sProducerFences[j], nullptr);
                    sProducerFences[j] = VK_NULL_HANDLE;
                }
                for (U32 j = 0; j < FRAMES_IN_FLIGHT; ++j)
                {
                    vkDestroyFence(sDevice, sInFlightFences[j], nullptr);
                    sInFlightFences[j] = VK_NULL_HANDLE;
                }
                return false;
            }
        }

        sAsyncProducerInFlight = false;
        if (vkCreateFence(sDevice, &producer_fence_info, nullptr, &sAsyncProducerFence) != VK_SUCCESS)
        {
            for (U32 j = 0; j < FRAMES_IN_FLIGHT; ++j)
            {
                vkDestroyFence(sDevice, sProducerFences[j], nullptr);
                sProducerFences[j] = VK_NULL_HANDLE;
                vkDestroyFence(sDevice, sInFlightFences[j], nullptr);
                sInFlightFences[j] = VK_NULL_HANDLE;
            }
            return false;
        }


        VkSemaphoreCreateInfo sem_info = {};
        sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
        {
            VkResult ia_res = vkCreateSemaphore(sDevice, &sem_info, nullptr,
                                                &sImageAvailableSemaphores[i]);
            if (ia_res != VK_SUCCESS)
            {
                for (U32 j = 0; j < i; ++j)
                {
                    vkDestroySemaphore(sDevice, sImageAvailableSemaphores[j], nullptr);
                    sImageAvailableSemaphores[j] = VK_NULL_HANDLE;
                    vkDestroySemaphore(sDevice, sRenderFinishedSemaphores[j], nullptr);
                    sRenderFinishedSemaphores[j] = VK_NULL_HANDLE;
                }
                for (U32 j = 0; j < FRAMES_IN_FLIGHT; ++j)
                {
                    vkDestroyFence(sDevice, sInFlightFences[j], nullptr);
                    sInFlightFences[j] = VK_NULL_HANDLE;
                }
                return false;
            }

            VkResult rf_res = vkCreateSemaphore(sDevice, &sem_info, nullptr,
                                                &sRenderFinishedSemaphores[i]);
            if (rf_res != VK_SUCCESS)
            {
                vkDestroySemaphore(sDevice, sImageAvailableSemaphores[i], nullptr);
                sImageAvailableSemaphores[i] = VK_NULL_HANDLE;
                for (U32 j = 0; j < i; ++j)
                {
                    vkDestroySemaphore(sDevice, sImageAvailableSemaphores[j], nullptr);
                    sImageAvailableSemaphores[j] = VK_NULL_HANDLE;
                    vkDestroySemaphore(sDevice, sRenderFinishedSemaphores[j], nullptr);
                    sRenderFinishedSemaphores[j] = VK_NULL_HANDLE;
                }
                for (U32 j = 0; j < FRAMES_IN_FLIGHT; ++j)
                {
                    vkDestroyFence(sDevice, sInFlightFences[j], nullptr);
                    sInFlightFences[j] = VK_NULL_HANDLE;
                }
                return false;
            }
        }

        return true;
    }

    void destroySyncObjects()
    {
        for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
        {
            if (sInFlightFences[i] != VK_NULL_HANDLE)
            {
                vkDestroyFence(sDevice, sInFlightFences[i], nullptr);
                sInFlightFences[i] = VK_NULL_HANDLE;
            }
            if (sProducerFences[i] != VK_NULL_HANDLE)
            {
                vkDestroyFence(sDevice, sProducerFences[i], nullptr);
                sProducerFences[i] = VK_NULL_HANDLE;
            }
            sProducerFencePending[i] = false;
            if (i == 0 && sAsyncProducerFence != VK_NULL_HANDLE)
            {
                vkDestroyFence(sDevice, sAsyncProducerFence, nullptr);
                sAsyncProducerFence = VK_NULL_HANDLE;
                sAsyncProducerInFlight = false;
            }
            if (sImageAvailableSemaphores[i] != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(sDevice, sImageAvailableSemaphores[i], nullptr);
                sImageAvailableSemaphores[i] = VK_NULL_HANDLE;
            }
            if (sRenderFinishedSemaphores[i] != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(sDevice, sRenderFinishedSemaphores[i], nullptr);
                sRenderFinishedSemaphores[i] = VK_NULL_HANDLE;
            }
        }
    }

    bool createSwapchain(VkSwapchainKHR old_swapchain = VK_NULL_HANDLE)
    {
        if (sDevice == VK_NULL_HANDLE || sPhysicalDevice == VK_NULL_HANDLE ||
            sSurface == VK_NULL_HANDLE)
        {
            return false;
        }

        VkSurfaceCapabilitiesKHR caps = {};
        VkResult cres = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(sPhysicalDevice,
                                                                   sSurface, &caps);
        if (cres != VK_SUCCESS)
        {
            return false;
        }

        U32 format_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(sPhysicalDevice, sSurface,
                                              &format_count, nullptr);
        if (format_count == 0)
        {
            return false;
        }
        std::vector<VkSurfaceFormatKHR> formats(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(sPhysicalDevice, sSurface,
                                              &format_count, formats.data());

        VkSurfaceFormatKHR chosen_format = formats[0];
        for (const VkSurfaceFormatKHR& f : formats)
        {
            if (f.format == VK_FORMAT_B8G8R8A8_UNORM &&
                f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                chosen_format = f;
                break;
            }
        }

        VkPresentModeKHR chosen_present_mode = VK_PRESENT_MODE_FIFO_KHR;
        {
            VkPresentModeKHR desired = VK_PRESENT_MODE_FIFO_KHR;
            const char* pm = getenv("AYASTORM_PRESENT_MODE");
            if (pm != nullptr)
            {
                if (std::strcmp(pm, "immediate") == 0)   desired = VK_PRESENT_MODE_IMMEDIATE_KHR;
                else if (std::strcmp(pm, "mailbox") == 0) desired = VK_PRESENT_MODE_MAILBOX_KHR;
            }
            else if (!sVsyncEnabled.load())
            {
                desired = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
            if (desired != VK_PRESENT_MODE_FIFO_KHR)
            {
                U32 pm_count = 0;
                vkGetPhysicalDeviceSurfacePresentModesKHR(sPhysicalDevice, sSurface, &pm_count, nullptr);
                std::vector<VkPresentModeKHR> pm_avail(pm_count);
                if (pm_count > 0)
                {
                    vkGetPhysicalDeviceSurfacePresentModesKHR(sPhysicalDevice, sSurface, &pm_count, pm_avail.data());
                }
                bool supported = false;
                for (VkPresentModeKHR m : pm_avail) { if (m == desired) { supported = true; break; } }
                if (supported)
                {
                    chosen_present_mode = desired;
                }
                else
                {
                    LL_WARNS("Vulkan") << "AYASTORM_PRESENT_MODE=" << pm
                                       << " unsupported by surface — falling back to FIFO (vsync)" << LL_ENDL;
                }
            }
        }
        sActivePresentMode = chosen_present_mode;
        LL_INFOS("Vulkan") << "present mode = "
                           << (chosen_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR ? "IMMEDIATE (vsync OFF)"
                             : chosen_present_mode == VK_PRESENT_MODE_MAILBOX_KHR   ? "MAILBOX (vsync, tear-free)"
                             : "FIFO (vsync ON)") << LL_ENDL;

        VkExtent2D extent;
        if (caps.currentExtent.width != UINT32_MAX)
        {
            extent = caps.currentExtent;
        }
        else
        {
            U32 w = 1280;
            U32 h = 720;
            if (w < caps.minImageExtent.width)  w = caps.minImageExtent.width;
            if (h < caps.minImageExtent.height) h = caps.minImageExtent.height;
            if (w > caps.maxImageExtent.width)  w = caps.maxImageExtent.width;
            if (h > caps.maxImageExtent.height) h = caps.maxImageExtent.height;
            extent.width  = w;
            extent.height = h;
        }

        U32 image_count = caps.minImageCount + 1;
        if (caps.maxImageCount > 0 && image_count > caps.maxImageCount)
        {
            image_count = caps.maxImageCount;
        }

        VkSwapchainCreateInfoKHR ci = {};
        ci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        ci.surface          = sSurface;
        ci.minImageCount    = image_count;
        ci.imageFormat      = chosen_format.format;
        ci.imageColorSpace  = chosen_format.colorSpace;
        ci.imageExtent      = extent;
        ci.imageArrayLayers = 1;
        ci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ci.preTransform     = caps.currentTransform;
        ci.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        ci.presentMode      = chosen_present_mode;
        ci.clipped          = VK_TRUE;
        ci.oldSwapchain     = old_swapchain;

        VkResult res = vkCreateSwapchainKHR(sDevice, &ci, nullptr, &sSwapchain);
        if (res != VK_SUCCESS)
        {
            sSwapchain = VK_NULL_HANDLE;
            return false;
        }

        sSwapchainFormat = chosen_format.format;
        sSwapchainExtent = extent;

        U32 actual_image_count = 0;
        vkGetSwapchainImagesKHR(sDevice, sSwapchain, &actual_image_count, nullptr);
        sSwapchainImages.assign(actual_image_count, VK_NULL_HANDLE);
        vkGetSwapchainImagesKHR(sDevice, sSwapchain, &actual_image_count,
                                sSwapchainImages.data());

        sSwapchainImageViews.assign(actual_image_count, VK_NULL_HANDLE);
        for (U32 i = 0; i < actual_image_count; ++i)
        {
            VkImageViewCreateInfo vci = {};
            vci.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            vci.image    = sSwapchainImages[i];
            vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            vci.format   = sSwapchainFormat;
            vci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            vci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            vci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            vci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            vci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            vci.subresourceRange.baseMipLevel   = 0;
            vci.subresourceRange.levelCount     = 1;
            vci.subresourceRange.baseArrayLayer = 0;
            vci.subresourceRange.layerCount     = 1;

            VkResult vres = vkCreateImageView(sDevice, &vci, nullptr,
                                              &sSwapchainImageViews[i]);
            if (vres != VK_SUCCESS)
            {
                for (U32 j = 0; j < i; ++j)
                {
                    if (sSwapchainImageViews[j] != VK_NULL_HANDLE)
                    {
                        vkDestroyImageView(sDevice, sSwapchainImageViews[j], nullptr);
                        sSwapchainImageViews[j] = VK_NULL_HANDLE;
                    }
                }
                sSwapchainImageViews.clear();
                sSwapchainImages.clear();
                vkDestroySwapchainKHR(sDevice, sSwapchain, nullptr);
                sSwapchain = VK_NULL_HANDLE;
                sSwapchainFormat = VK_FORMAT_UNDEFINED;
                sSwapchainExtent = {0, 0};
                return false;
            }
            noteViewHandleCreated(sSwapchainImageViews[i]);
        }

        if (sSwapchainDepthImage != VK_NULL_HANDLE)
        {
            destroyImageVk(sSwapchainDepthImage, sSwapchainDepthView, sSwapchainDepthAlloc);
            sSwapchainDepthImage = VK_NULL_HANDLE;
            sSwapchainDepthView  = VK_NULL_HANDLE;
            sSwapchainDepthAlloc = nullptr;
        }
        sSwapchainDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        createDepthAttachmentImageVk(extent.width, extent.height,
                                     VK_FORMAT_D24_UNORM_S8_UINT,
                                     sSwapchainDepthImage,
                                     sSwapchainDepthView,
                                     sSwapchainDepthAlloc);

        return true;
    }

    void destroySwapchain()
    {
        if (sDevice == VK_NULL_HANDLE)
        {
            sSwapchainImageViews.clear();
            sSwapchainImages.clear();
            sSwapchain = VK_NULL_HANDLE;
            sSwapchainFormat = VK_FORMAT_UNDEFINED;
            sSwapchainExtent = {0, 0};
            sSwapchainDepthImage  = VK_NULL_HANDLE;
            sSwapchainDepthView   = VK_NULL_HANDLE;
            sSwapchainDepthAlloc  = nullptr;
            sSwapchainDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            return;
        }
        for (VkImageView& view : sSwapchainImageViews)
        {
            if (view != VK_NULL_HANDLE)
            {
                vkDestroyImageView(sDevice, view, nullptr);
                view = VK_NULL_HANDLE;
            }
        }
        sSwapchainImageViews.clear();
        sSwapchainImages.clear();

        if (sSwapchainDepthImage != VK_NULL_HANDLE)
        {
            destroyImageVk(sSwapchainDepthImage, sSwapchainDepthView, sSwapchainDepthAlloc);
        }
        sSwapchainDepthImage  = VK_NULL_HANDLE;
        sSwapchainDepthView   = VK_NULL_HANDLE;
        sSwapchainDepthAlloc  = nullptr;
        sSwapchainDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (sSwapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(sDevice, sSwapchain, nullptr);
            sSwapchain = VK_NULL_HANDLE;
        }
        sSwapchainFormat = VK_FORMAT_UNDEFINED;
        sSwapchainExtent = {0, 0};
    }

    bool recreateSwapchain()
    {
        if (sDevice == VK_NULL_HANDLE || sSurface == VK_NULL_HANDLE)
        {
            sSwapchainRecreatePending = false;
            return false;
        }

        peDrain();
        vkDeviceWaitIdle(sDevice);
        for (VkImageView& view : sSwapchainImageViews)
        {
            if (view != VK_NULL_HANDLE)
            {
                vkDestroyImageView(sDevice, view, nullptr);
                view = VK_NULL_HANDLE;
            }
        }
        sSwapchainImageViews.clear();
        sSwapchainImages.clear();

        VkSwapchainKHR old_swapchain = sSwapchain;
        sSwapchain                   = VK_NULL_HANDLE;
        sSwapchainFormat             = VK_FORMAT_UNDEFINED;
        sSwapchainExtent             = {0, 0};

        bool ok = createSwapchain(old_swapchain);

        if (old_swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(sDevice, old_swapchain, nullptr);
        }

        sSwapchainRecreatePending = false;
        sLastRecreateFrame        = sMonotonicFrameCount;


        return ok;
    }

}

static void           tickScenePerDrawDescriptorCache();
static void           endSwapchainRendering();
static VkShaderModule loadSpirvShaderModule(const U32* spv_code, size_t code_size_bytes);
static void           tickDeferredBufferFreeQueue();
static void           tickDeferredImageFreeQueue();
static void           tickDeferredObjectFreeQueue();
void                  tickMegaFreeQueue();
static void           tickDeferredQueryReleaseQueue();
static bool           submitOneShotVk(VkCommandBuffer cmd, VkBuffer staging_buffer, VmaAllocation staging_allocation);
static bool           submitOneShotVkFromPool(VkCommandBuffer cmd, VkCommandPool pool, VkBuffer staging_buffer,
                                              VmaAllocation staging_allocation, U32 staging_bytes);
static void           tickOneShotFreeQueue();
static void           shutdownSurface();
static bool           initSharedDynamicPersistentUBOs();
static void           teardownSharedDynamicPersistentUBOs();
static void           tickSharedDynamicPersistentUBOs();
static void           tickPerDrawUBOArena();
static void           reapAllDeferred(ReapMode mode);
static void           teardownSharedLatchedUBOs();
extern thread_local float sCurrentModelviewMatrix[16];

bool initVulkan()
{
    if (sInitialized)
    {
        return true;
    }
    sProducersQuiesced = false;

    if (getRenderBackendMode() == 0)
    {
        return false;
    }


    VkResult result = volkInitialize();
    if (result != VK_SUCCESS)
    {
        return false;
    }

    if (!createInstance())
    {
        return false;
    }

    if (!selectPhysicalDevice() || !selectQueueFamily() || !createDevice())
    {
        if (sDebugMessenger != VK_NULL_HANDLE && vkDestroyDebugUtilsMessengerEXT != nullptr)
        {
            vkDestroyDebugUtilsMessengerEXT(sInstance, sDebugMessenger, nullptr);
            sDebugMessenger = VK_NULL_HANDLE;
        }
        if (sInstance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(sInstance, nullptr);
            sInstance = VK_NULL_HANDLE;
        }
        return false;
    }

    if (!createPipelineCacheStorage())
    {
        shutdownVulkan();
        return false;
    }

    if (!createCommandPool() || !createDefaultFallbackImage() || !createPipelineCache())
    {
        shutdownVulkan();
        return false;
    }

    if (!createVmaAllocator())
    {
        shutdownVulkan();
        return false;
    }

    if (!createDefaultFallbackCubeArrayImage() || !createDefaultFallbackCubeImage() || !createDefaultFallback3DImage()
        || !createDefaultFallbackShadowImage())
    {
        shutdownVulkan();
        return false;
    }

    if (!createPerFrameDescriptorSetLayout() ||
        !createPerFrameUbos()                ||
        !initSharedDynamicPersistentUBOs()   ||
        !createPerFrameDescriptorSets())
    {
        shutdownVulkan();
        return false;
    }

    {
        VkDescriptorPool initial_pool = VK_NULL_HANDLE;
        if (createScenePerDrawDescriptorPool(&initial_pool))
        {
            sPerDrawDescLanes[0].pools.push_back(initial_pool);
        }
    }

    if (!createStandardSampler())
    {
        shutdownVulkan();
        return false;
    }

    createBindlessHeap();

    if (!createSyncObjects())
    {
        shutdownVulkan();
        return false;
    }

    peStart();

    sInitialized = true;
    if (getenv("AYASTORM_PAR_SELFTEST") != nullptr)
    {
        LLVKContract::runParallelSelfTest();
    }
    return true;
}

void vkQuiesceProducers()
{
    if (sProducersQuiesced)
    {
        return;
    }
    sProducersQuiesced = true;
    if (sBakeWorkerStopHook != nullptr)
    {
        void (*hook)() = sBakeWorkerStopHook;
        sBakeWorkerStopHook = nullptr;
        hook();
    }
    if (sGeoWorkerStopHook != nullptr)
    {
        void (*hook)() = sGeoWorkerStopHook;
        sGeoWorkerStopHook = nullptr;
        hook();
    }
    if (sTexWorkerStopHook != nullptr)
    {
        void (*hook)() = sTexWorkerStopHook;
        sTexWorkerStopHook = nullptr;
        hook();
    }
    rwStop();
    peStop();
    texWorkerShutdown();
}

void shutdownVulkan(bool device_lost)
{
    device_lost = device_lost || sVkDeviceLost.load(std::memory_order_acquire);
    {
        std::lock_guard<std::mutex> lk(sVvlCountMutex);
        for (const auto& entry : sVvlCounts)
        {
            LL_INFOS("VulkanValidation") << "VVL-TOTAL id=" << entry.second.first
                                         << " n=" << entry.second.second << LL_ENDL;
        }
        sVvlCounts.clear();
    }
    vkQuiesceProducers();
    if (sDevice != VK_NULL_HANDLE)
    {
        if (!device_lost)
        {
            vkDeviceWaitIdle(sDevice);
        }
        rwDestroyLanePools();

        reapAllDeferred(device_lost ? REAP_LOST : REAP_CLOSE);
        for (VkFence pooled_fence : sSubmitFencePool)
        {
            vkDestroyFence(sDevice, pooled_fence, nullptr);
        }
        sSubmitFencePool.clear();

        destroySwapchain();

        if (sDefaultFallbackImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(sDevice, sDefaultFallbackImageView, nullptr);
            sDefaultFallbackImageView = VK_NULL_HANDLE;
        }
        if (sDefaultFallbackImage != VK_NULL_HANDLE)
        {
            vkDestroyImage(sDevice, sDefaultFallbackImage, nullptr);
            sDefaultFallbackImage = VK_NULL_HANDLE;
        }
        if (sDefaultFallbackMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(sDevice, sDefaultFallbackMemory, nullptr);
            sDefaultFallbackMemory = VK_NULL_HANDLE;
        }
        if (sDefaultFallbackShadowImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(sDevice, sDefaultFallbackShadowImageView, nullptr);
            sDefaultFallbackShadowImageView = VK_NULL_HANDLE;
        }
        if (sDefaultFallbackShadowImage != VK_NULL_HANDLE)
        {
            vkDestroyImage(sDevice, sDefaultFallbackShadowImage, nullptr);
            sDefaultFallbackShadowImage = VK_NULL_HANDLE;
        }
        if (sDefaultFallbackShadowMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(sDevice, sDefaultFallbackShadowMemory, nullptr);
            sDefaultFallbackShadowMemory = VK_NULL_HANDLE;
        }
        if (sDefaultFallbackCubeArrayImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(sDevice, sDefaultFallbackCubeArrayImageView, nullptr);
            sDefaultFallbackCubeArrayImageView = VK_NULL_HANDLE;
        }
        if (sDefaultFallbackCubeArrayImage != VK_NULL_HANDLE)
        {
            vmaDestroyImage(sAllocator, sDefaultFallbackCubeArrayImage,
                            reinterpret_cast<VmaAllocation>(sDefaultFallbackCubeArrayAlloc));
            sDefaultFallbackCubeArrayImage = VK_NULL_HANDLE;
            sDefaultFallbackCubeArrayAlloc = nullptr;
        }
        if (sDefaultFallbackCubeImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(sDevice, sDefaultFallbackCubeImageView, nullptr);
            sDefaultFallbackCubeImageView = VK_NULL_HANDLE;
        }
        if (sDefaultFallbackCubeImage != VK_NULL_HANDLE)
        {
            vmaDestroyImage(sAllocator, sDefaultFallbackCubeImage,
                            reinterpret_cast<VmaAllocation>(sDefaultFallbackCubeAlloc));
            sDefaultFallbackCubeImage = VK_NULL_HANDLE;
            sDefaultFallbackCubeAlloc = nullptr;
        }
        if (sDefaultFallback3DImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(sDevice, sDefaultFallback3DImageView, nullptr);
            sDefaultFallback3DImageView = VK_NULL_HANDLE;
        }
        if (sDefaultFallback3DImage != VK_NULL_HANDLE)
        {
            vmaDestroyImage(sAllocator, sDefaultFallback3DImage,
                            reinterpret_cast<VmaAllocation>(sDefaultFallback3DAlloc));
            sDefaultFallback3DImage = VK_NULL_HANDLE;
            sDefaultFallback3DAlloc = nullptr;
        }

        for (VkDescriptorPool pool : sPerFrameRingPools)
        {
            if (pool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(sDevice, pool, nullptr);
            }
        }
        sPerFrameRingPools.clear();
        sRingPoolUsedInLast = 0;
        for (U32 frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            sPerFrameRingSets[frame].clear();
            sPerFrameRingSetCount[frame]    = 0;
            sMatrixRingUsedThisFrame[frame] = 0;
            sMatrixRingCurrentSlot[frame]   = 0;
            sMatrixRingHasCurrent[frame]    = false;
        }

        ++sScenePerDrawCacheEpoch;
        for (PerDrawDescLane& lane : sPerDrawDescLanes)
        {
            for (VkDescriptorPool pool : lane.pools)
            {
                if (pool != VK_NULL_HANDLE)
                {
                    vkDestroyDescriptorPool(sDevice, pool, nullptr);
                }
            }
            lane.pools.clear();
            lane.cache.clear();
            lane.lru.clear();
            lane.deferred_free.clear();
        }

        if (sStandardLinearSampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(sDevice, sStandardLinearSampler, nullptr);
            sStandardLinearSampler = VK_NULL_HANDLE;
        }

        for (auto& kv : sSamplerCache)
        {
            if (kv.second != VK_NULL_HANDLE)
            {
                vkDestroySampler(sDevice, kv.second, nullptr);
            }
        }
        sSamplerCache.clear();

        auto destroy_shared_ubo = [&](VkBuffer& buf, void*& alloc, void*& mapped)
        {
            if (buf != VK_NULL_HANDLE && sAllocator != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(sAllocator, buf, reinterpret_cast<VmaAllocation>(alloc));
            }
            buf    = VK_NULL_HANDLE;
            alloc  = nullptr;
            mapped = nullptr;
        };
        teardownSharedLatchedUBOs();
        for (U32 frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            if (sPerFrameUboMemory[frame] != VK_NULL_HANDLE && sPerFrameUboMapped[frame] != nullptr)
            {
                vkUnmapMemory(sDevice, sPerFrameUboMemory[frame]);
                sPerFrameUboMapped[frame] = nullptr;
            }
            if (sPerFrameUboBuffer[frame] != VK_NULL_HANDLE)
            {
                vkDestroyBuffer(sDevice, sPerFrameUboBuffer[frame], nullptr);
                sPerFrameUboBuffer[frame] = VK_NULL_HANDLE;
            }
            if (sPerFrameUboMemory[frame] != VK_NULL_HANDLE)
            {
                vkFreeMemory(sDevice, sPerFrameUboMemory[frame], nullptr);
                sPerFrameUboMemory[frame] = VK_NULL_HANDLE;
            }

            for (auto& chunk : sMatrixRingChunks[frame])
            {
                if (chunk.memory != VK_NULL_HANDLE && chunk.mapped != nullptr)
                {
                    vkUnmapMemory(sDevice, chunk.memory);
                    chunk.mapped = nullptr;
                }
                if (chunk.buffer != VK_NULL_HANDLE)
                {
                    vkDestroyBuffer(sDevice, chunk.buffer, nullptr);
                    chunk.buffer = VK_NULL_HANDLE;
                }
                if (chunk.memory != VK_NULL_HANDLE)
                {
                    vkFreeMemory(sDevice, chunk.memory, nullptr);
                    chunk.memory = VK_NULL_HANDLE;
                }
            }
            sMatrixRingChunks[frame].clear();

            for (auto& slot : sDuOverrideRing[frame])
            {
                if (slot.buffer != VK_NULL_HANDLE)
                {
                    destroyBufferVk(slot.buffer, slot.allocation);
                }
            }
            sDuOverrideRing[frame].clear();
            sDuOverrideIdx[frame]   = 0;
            sDuOverrideFrame[frame] = 0;

            for (auto& slot : sShadowUtilRing[frame])
            {
                if (slot.buffer != VK_NULL_HANDLE)
                {
                    destroyBufferVk(slot.buffer, slot.allocation);
                }
            }
            sShadowUtilRing[frame].clear();
            sShadowUtilRingIdx[frame]   = 0;
            sShadowUtilRingFrame[frame] = 0;
            sCurShadowUtilBuf[frame]    = VK_NULL_HANDLE;
            sCurShadowUtilMapped[frame] = nullptr;

            for (auto& slot : sDeferredUtilRing[frame])
            {
                if (slot.buffer != VK_NULL_HANDLE)
                {
                    destroyBufferVk(slot.buffer, slot.allocation);
                }
            }
            sDeferredUtilRing[frame].clear();
            sDeferredUtilRingIdx[frame]   = 0;
            sDeferredUtilRingFrame[frame] = 0;
            sCurDeferredUtilBuf[frame]    = VK_NULL_HANDLE;
            sCurDeferredUtilMapped[frame] = nullptr;

            #define LLVK_SHARED_UBO_RING_TEARDOWN(BindName)                                            \
                for (auto& slot : s##BindName##Ring[frame])                                            \
                {                                                                                      \
                    if (slot.buffer != VK_NULL_HANDLE) destroyBufferVk(slot.buffer, slot.allocation);  \
                }                                                                                      \
                s##BindName##Ring[frame].clear();                                                      \
                s##BindName##RingIdx[frame]   = 0;                                                     \
                s##BindName##RingFrame[frame] = 0;                                                     \
                sCur##BindName##Buf[frame]    = VK_NULL_HANDLE;                                        \
                sCur##BindName##Mapped[frame] = nullptr;
            LLVK_SHARED_UBO_RING_TEARDOWN(WindlightSky)
            LLVK_SHARED_UBO_RING_TEARDOWN(WindlightAtmos)
            LLVK_SHARED_UBO_RING_TEARDOWN(AoUtil)
            LLVK_SHARED_UBO_RING_TEARDOWN(GlobalF)
            LLVK_SHARED_UBO_RING_TEARDOWN(WaterFog)
            LLVK_SHARED_UBO_RING_TEARDOWN(WaterV)
            LLVK_SHARED_UBO_RING_TEARDOWN(ReflectionProbe)
            LLVK_SHARED_UBO_RING_TEARDOWN(ReflectionProbes)
            LLVK_SHARED_UBO_RING_TEARDOWN(Lights)
            LLVK_SHARED_UBO_RING_TEARDOWN(LightsSpecular)
            LLVK_SHARED_UBO_RING_TEARDOWN(PbrTerrainF)
            LLVK_SHARED_UBO_RING_TEARDOWN(PbrTerrain)
            #undef LLVK_SHARED_UBO_RING_TEARDOWN

            PerDrawUBOArena& arena = sPerDrawUBOArena[frame];
            if (arena.buffer != VK_NULL_HANDLE)
            {
                destroyBufferVk(arena.buffer, arena.allocation);
            }
            arena.buffer           = VK_NULL_HANDLE;
            arena.allocation       = nullptr;
            arena.mapped           = nullptr;
            arena.capacity         = 0;
            arena.cursor           = 0;
            arena.frame            = ~0ull;
            arena.pending_capacity = 0;
        }
        teardownSharedDynamicPersistentUBOs();
        if (sPerFrameDescriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(sDevice, sPerFrameDescriptorSetLayout, nullptr);
            sPerFrameDescriptorSetLayout = VK_NULL_HANDLE;
        }
        if (sIndirectRingBuffer != VK_NULL_HANDLE)
        {
            destroyBufferVk(sIndirectRingBuffer, sIndirectRingAllocation);
            sIndirectRingBuffer     = VK_NULL_HANDLE;
            sIndirectRingAllocation = nullptr;
            sIndirectRingMapped     = nullptr;
            sIndirectRingCursor     = 0;
            sIndirectRingFrame      = 0xFFFFFFFFu;
        }
        destroyBindlessHeap();

        if (sPipelineCache != VK_NULL_HANDLE && pcache::sInitialized)
        {
            std::size_t blob_size = 0;
            VkResult sz_res = vkGetPipelineCacheData(sDevice, sPipelineCache, &blob_size, nullptr);
            if (sz_res == VK_SUCCESS && blob_size > 0)
            {
                pcache::CacheBlob blob(blob_size);
                VkResult get_res = vkGetPipelineCacheData(sDevice, sPipelineCache,
                                                          &blob_size, blob.data());
                if (get_res == VK_SUCCESS || get_res == VK_INCOMPLETE)
                {
                    blob.resize(blob_size);
                    bool persisted = false;
                    if (pcache::sMaxSizeBytes == 0 || blob.size() <= pcache::sMaxSizeBytes)
                    {
                        persisted = pcache::writeBlobToDisk(pcache::sFilePath, blob);
                    }
                }
            }
        }
        if (sPipelineCache != VK_NULL_HANDLE)
        {
            vkDestroyPipelineCache(sDevice, sPipelineCache, nullptr);
            sPipelineCache = VK_NULL_HANDLE;
        }
        pcache::sBlob.clear();
        pcache::sBlob.shrink_to_fit();
        pcache::sInitialized = false;

        destroySyncObjects();

        if (sOcclusionQueryPool != VK_NULL_HANDLE)
        {
            vkDestroyQueryPool(sDevice, sOcclusionQueryPool, nullptr);
            sOcclusionQueryPool     = VK_NULL_HANDLE;
            sOcclusionQueryCapacity = 0;
            std::queue<uint32_t> empty;
            std::swap(sOcclusionQueryFree, empty);
        }

        if (sTimestampQueryPool != VK_NULL_HANDLE)
        {
            vkDestroyQueryPool(sDevice, sTimestampQueryPool, nullptr);
            sTimestampQueryPool    = VK_NULL_HANDLE;
            sTimestampPairCapacity = 0;
            std::queue<uint32_t> empty;
            std::swap(sTimestampPairFree, empty);
        }
        sTimestampSupported = false;
        sTimestampValidBits = 0;
        sTimestampPeriodNs  = 0.0f;

        if (sCommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(sDevice, sCommandPool, nullptr);
            sCommandPool = VK_NULL_HANDLE;
            for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
            {
                sCommandBuffers[i] = VK_NULL_HANDLE;
            }
        }

        if (sAllocator != VK_NULL_HANDLE)
        {
            for (auto& pending : sPendingBufferFrees)
            {
                vmaDestroyBuffer(sAllocator, pending.buffer, pending.allocation);
            }
            sPendingBufferFrees.clear();
            for (auto& pending : sPendingImageFrees)
            {
                if (pending.view != VK_NULL_HANDLE && sDevice != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(sDevice, pending.view, nullptr);
                }
                if (pending.image != VK_NULL_HANDLE)
                {
                    vmaDestroyImage(sAllocator, pending.image, pending.allocation);
                }
            }
            sPendingImageFrees.clear();
            vmaDestroyAllocator(sAllocator);
            sAllocator = VK_NULL_HANDLE;
        }

        vkDestroyDevice(sDevice, nullptr);
        sDevice = VK_NULL_HANDLE;
        sGraphicsQueue = VK_NULL_HANDLE;
        sGraphicsQueueFamily = UINT_MAX;
    }
    shutdownSurface();
    if (sDebugMessenger != VK_NULL_HANDLE && vkDestroyDebugUtilsMessengerEXT != nullptr)
    {
        vkDestroyDebugUtilsMessengerEXT(sInstance, sDebugMessenger, nullptr);
        sDebugMessenger = VK_NULL_HANDLE;
    }
    if (sInstance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(sInstance, nullptr);
        sInstance = VK_NULL_HANDLE;
        sPhysicalDevice = VK_NULL_HANDLE;
    }
    if (sInitialized)
    {
        volkFinalize();
        sInitialized = false;
    }
}

void shutdownSwapchainAndSurface()
{
    if (sDevice != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(sDevice);
        destroySwapchain();
    }
    shutdownSurface();
}

static void beginCommandRecording()
{
    ++tCmdRecordEpoch;
    LLGLSLShader::sCurPerCallVkOffsetsDirty = true;
}

static void reapAllDeferred(ReapMode mode)
{
    sReapForceAll = (mode != REAP_CHURN);
    tickDeferredBufferFreeQueue();
    tickDeferredImageFreeQueue();
    tickDeferredObjectFreeQueue();
    tickMegaFreeQueue();
    tickDeferredQueryReleaseQueue();
    tickOneShotFreeQueue();
    if (mode == REAP_CHURN)
    {
        tickSharedDynamicPersistentUBOs();
        tickPerDrawUBOArena();
        tickScenePerDrawDescriptorCache();
    }
    sReapForceAll = false;
}

bool isUISceneSplit()
{
    return sUISceneSplit;
}

bool isUISceneAsync()
{
    return sUISceneSplit && sUISceneAsync;
}

bool asyncProducerTryComplete()
{
    if (!sAsyncProducerInFlight || sAsyncProducerFence == VK_NULL_HANDLE)
    {
        return false;
    }
    if (vkGetFenceStatus(sDevice, sAsyncProducerFence) == VK_SUCCESS)
    {
        vkResetFences(sDevice, 1, &sAsyncProducerFence);
        sAsyncProducerInFlight = false;
        return true;
    }
    return false;
}

bool isAsyncProducerInFlight()
{
    return sAsyncProducerInFlight;
}

U32 asyncProducerBackIndex()
{
    return sAsyncProducerBackIndex;
}

bool asyncShouldRenderScene()
{
    return !sAsyncFrameEngaged || sAsyncRenderSceneThisFrame;
}

void setAsyncFrameEngaged(bool on)
{
    sAsyncFrameEngaged = on;
}

bool asyncFrameEngaged()
{
    return sAsyncFrameEngaged;
}

void asyncProducerBeginScene(U32 back_index)
{
    if (!sInFrame || sAsyncProducerCommandBuffer == VK_NULL_HANDLE)
    {
        return;
    }
    sAsyncProducerBackIndex    = back_index;
    sAsyncProducerRecordSlot   = sFrameIndex;
    sAsyncRenderSceneThisFrame = true;
    vkResetCommandBuffer(sAsyncProducerCommandBuffer, 0);
    VkCommandBufferBeginInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(sAsyncProducerCommandBuffer, &bi);
    tRecordCmdOverride = sAsyncProducerCommandBuffer;
}

void setProducerPresentActive(bool on)
{
    sProducerPresentActive = on;
}

bool producerSwapchainFallbackShouldSkip()
{
    return sUISceneSplit && sProducerPresentActive && sInFrame &&
           tRecordCmdOverride != sConsumerCommandBuffers[sFrameIndex];
}

void recordToConsumer(bool on)
{
    if (on && sUISceneSplit && sInFrame)
    {
        tRecordCmdOverride     = sConsumerCommandBuffers[sFrameIndex];
        sConsumerActiveThisFrame = true;
    }
    else
    {
        tRecordCmdOverride = VK_NULL_HANDLE;
    }
}

bool beginFrame(bool acquire_swapchain)
{
    if (!sInitialized)
    {
        return false;
    }
    if (sInFrame)
    {
        return false;
    }

    if (sVkDeviceLost.load(std::memory_order_acquire))
    {
        if (!sDeviceLostSignaled)
        {
            sDeviceLostSignaled = true;
            LL_WARNS("Vulkan") << "GPU device lost (VK_ERROR_DEVICE_LOST) — requesting graceful shutdown."
                               << " Check kernel log for NVIDIA Xid details." << LL_ENDL;
            if (sDeviceLostHook != nullptr)
            {
                sDeviceLostHook();
            }
        }
        return false;
    }

    VkPerfMainScope mlp_total(3);

    beginCommandRecording();

    sSwapchainClearedThisFrame = false;

    if (sSwapchain != VK_NULL_HANDLE &&
        sMonotonicFrameCount >= STARTUP_FRAME_GATE &&
        sPendingResizeWidth != 0 && sPendingResizeHeight != 0)
    {
        if (sPendingResizeWidth == sSwapchainExtent.width &&
            sPendingResizeHeight == sSwapchainExtent.height)
        {
            sPendingResizeWidth  = 0;
            sPendingResizeHeight = 0;
        }
        else
        {
            sSwapchainRecreatePending = true;
        }
    }

    if (sSwapchainRecreatePending && acquire_swapchain)
    {
        const U32 frames_since_last = (sLastRecreateFrame == 0)
                                          ? RECREATE_COOLDOWN_FRAMES
                                          : (sMonotonicFrameCount - sLastRecreateFrame);
        if (frames_since_last >= RECREATE_COOLDOWN_FRAMES)
        {
            recreateSwapchain();
            return false;
        }
    }

    ++sMonotonicFrameCount;
    LLVKContract::frameBegin();

    sFrameIndex = (sFrameIndex + 1) % FRAMES_IN_FLIGHT;
    sSkinPaletteCursor[sFrameIndex].store(0, std::memory_order_relaxed); // B.0: reset skin palette region ring

    bool slot_submitted;
    {
        VkPerfMainScope mlp_slot(0);
        slot_submitted = peWaitSlotSubmitted(sFrameIndex);
    }
    if (sInFlightFences[sFrameIndex] != VK_NULL_HANDLE)
    {
        VkPerfMainScope mlp_fence(1);
        if (slot_submitted)
        {
            vkWaitForFences(sDevice, 1, &sInFlightFences[sFrameIndex],
                                                VK_TRUE, UINT64_MAX);
            if (sFrameSubmittedMonotonic[sFrameIndex] > sLastCompletedMonotonic)
            {
                sLastCompletedMonotonic = sFrameSubmittedMonotonic[sFrameIndex];
            }
        }
        vkResetFences(sDevice, 1, &sInFlightFences[sFrameIndex]);
    }
    if (sProducerFencePending[sFrameIndex] && sProducerFences[sFrameIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(sDevice, 1, &sProducerFences[sFrameIndex], VK_TRUE, UINT64_MAX);
        vkResetFences(sDevice, 1, &sProducerFences[sFrameIndex]);
        sProducerFencePending[sFrameIndex] = false;
    }
    if (sAsyncProducerInFlight && sAsyncProducerSubmitMonotonic > 0 &&
        sLastCompletedMonotonic >= sAsyncProducerSubmitMonotonic)
    {
        sLastCompletedMonotonic = sAsyncProducerSubmitMonotonic - 1;
    }
    if (sAsyncProducerInFlight && sAsyncProducerRecordSlot == sFrameIndex &&
        sAsyncProducerFence != VK_NULL_HANDLE)
    {
        vkWaitForFences(sDevice, 1, &sAsyncProducerFence, VK_TRUE, UINT64_MAX);
    }
    sPESlotState[sFrameIndex].store(PE_SLOT_IDLE);

    if (sFrameIndex < FRAMES_IN_FLIGHT)
    {
        sMatrixRingUsedThisFrame[sFrameIndex] = 0;
        sMatrixRingHasCurrent[sFrameIndex]    = false;
    }

    sImageAcquired = false;
    if (acquire_swapchain &&
        sVulkanPresentationEnabled &&
        sSwapchain != VK_NULL_HANDLE &&
        sImageAvailableSemaphores[sFrameIndex] != VK_NULL_HANDLE)
    {
        VkPerfMainScope mlp_acq(2);
        VkResult acquire_res;
        {
            std::lock_guard<std::mutex> lk(sSwapchainAccessMutex);
            acquire_res = vkAcquireNextImageKHR(sDevice, sSwapchain, UINT64_MAX,
                                                sImageAvailableSemaphores[sFrameIndex],
                                                VK_NULL_HANDLE,
                                                &sAcquiredImageIndex);
        }
        if (acquire_res == VK_SUCCESS || acquire_res == VK_SUBOPTIMAL_KHR)
        {
            sImageAcquired = true;
            if (acquire_res == VK_SUBOPTIMAL_KHR)
            {
                sSwapchainRecreatePending = true;
            }
        }
        else if (acquire_res == VK_ERROR_OUT_OF_DATE_KHR)
        {
            sSwapchainRecreatePending = true;
        }
    }

    vkResetCommandBuffer(sCommandBuffers[sFrameIndex], 0);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkResult result = vkBeginCommandBuffer(sCommandBuffers[sFrameIndex], &begin_info);
    if (result != VK_SUCCESS)
    {
        return false;
    }

    sConsumerActiveThisFrame = false;
    sProducerPresentActive   = false;
    sAsyncRenderSceneThisFrame = false;
    sAsyncFrameEngaged         = false;
    if (sUISceneSplit)
    {
        vkResetCommandBuffer(sConsumerCommandBuffers[sFrameIndex], 0);
        VkResult cresult = vkBeginCommandBuffer(sConsumerCommandBuffers[sFrameIndex], &begin_info);
        if (cresult != VK_SUCCESS)
        {
            return false;
        }
    }

    reapAllDeferred(REAP_CHURN);

    sInFrame = true;

    if (!sUISceneSplit &&
        sImageAcquired &&
        sAcquiredImageIndex < (U32)sSwapchainImages.size() &&
        sSwapchainImages[sAcquiredImageIndex] != VK_NULL_HANDLE)
    {
        transitionImageLayoutVk(sSwapchainImages[sAcquiredImageIndex],
                                VK_IMAGE_ASPECT_COLOR_BIT,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                0,
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

    }

    return true;
}

VkPerfCounters gVkPerf;
std::atomic<U64> gVkGeoInflightBytes{0};
thread_local U32 gVkPerfPassTag = 0;
thread_local U32 gVkPerfSetPath = 0;
thread_local U32 gVkPerfSetCause = 0;
thread_local U32 gVkPerfValFailKind = 0;

U32 perfPassBucket()
{
    return gHeroProbeMirrorRender ? 4u
           : gCubeSnapshot ? 3u
           : (gVkPerfPassTag < 3u ? gVkPerfPassTag : 0u);
}
thread_local U32 gVkPerfShadowMapIndex = 0;

std::atomic<U64> gVkPerDrawTopologyGen{1};
std::atomic<U64> gVkViewDestroyGen{1};
std::atomic<U64> gVkReloadEpoch{1};

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

U32 recordWorkerCount()
{
    return rwDesiredWorkerCount();
}

U32 getCurrentRecordLane()
{
    return tRecordLaneIndex;
}

bool isRecordJobActive()
{
    return tInRecordJob;
}

bool dispatchRecordJob(std::function<void(VkCommandBuffer)> body)
{
    if (!sInitialized || !sInFrame || tInRecordJob || !body)
    {
        return false;
    }
    rwStart();
    RecordJob job;
    job.body = std::move(body);
    job.seq  = sRWDispatched;
    ++sRWDispatched;
    if (sRWThreads.empty())
    {
        rwExecute(job);
        return true;
    }
    {
        std::lock_guard<std::mutex> lk(sRWQueueMutex);
        sRWJobs.push_back(std::move(job));
    }
    sRWQueueCv.notify_one();
    return true;
}

void joinRecordJobs()
{
    if (sRWDispatched == 0)
    {
        return;
    }
    {
        std::unique_lock<std::mutex> lk(sRWDoneMutex);
        sRWDoneCv.wait(lk, [] { return sRWCompleted >= sRWDispatched; });
    }
    for (VkCommandBuffer c : sRWFrameCmds)
    {
        if (c != VK_NULL_HANDLE)
        {
            sPendingPreFrameCmds.push_back(c);
        }
    }
    sRWFrameCmds.clear();
    sRWDispatched = 0;
    sRWCompleted  = 0;
}

bool endFrame()
{
    if (!sInitialized || !sInFrame)
    {
        return false;
    }

    VkPerfMainScope mlp_total(4);

    static const F64 s_perf_interval = []() -> F64 {
        const char* e = getenv("AYASTORM_PERF_LOG");
        if (e == nullptr)
        {
            return 0.0;
        }
        const F64 v = atof(e);
        return (v > 0.0) ? v : 5.0;
    }();
    if (s_perf_interval > 0.0)
    {
        static std::chrono::steady_clock::time_point s_last_emit = std::chrono::steady_clock::now();
        static U32 s_last_frame = sMonotonicFrameCount;
        const auto now = std::chrono::steady_clock::now();
        const F64 elapsed = std::chrono::duration<F64>(now - s_last_emit).count();
        if (elapsed >= s_perf_interval)
        {
            const U32 frames = sMonotonicFrameCount - s_last_frame;
            if (frames > 0)
            {
                const U64 draws = gVkPerf.desc_bind.load() + gVkPerf.desc_skip.load();
                LL_INFOS("VkPerf") << "frames=" << frames
                                   << " fps=" << ((F64)frames / elapsed)
                                   << " avg_ms=" << (elapsed * 1000.0 / (F64)frames)
                                   << " draws/f=" << (draws / frames)
                                   << " | emit/skip: pipe " << gVkPerf.pipe_bind.load() << "/" << gVkPerf.pipe_skip.load()
                                   << " desc " << gVkPerf.desc_bind.load() << "/" << gVkPerf.desc_skip.load()
                                   << " push " << gVkPerf.mv_push.load() << "/" << gVkPerf.mv_skip.load()
                                   << " vp " << gVkPerf.vp_set.load() << "/" << gVkPerf.vp_skip.load()
                                   << " | set build=" << gVkPerf.set_build.load()
                                   << " memo=" << gVkPerf.set_memo.load()
                                   << "/" << gVkPerf.set_memo_fill.load()
                                   << " populate=" << gVkPerf.populate.load()
                                   << " populate_us=" << (gVkPerf.populate_us.load() / 1000.0)
                                   << " bl=" << gVkPerf.populate_bl.load()
                                   << " pl=" << gVkPerf.populate_pl.load()
                                   << " blhit=" << gVkPerf.populate_blhit.load()
                                   << " ihit=" << gVkPerf.populate_hit.load()
                                   << " imiss=" << gVkPerf.populate_miss.load()
                                   << " | syncmat " << gVkPerf.syncmat_build.load() << "/" << gVkPerf.syncmat_call.load()
                                   << " | vbbind " << gVkPerf.vb_bind.load() << "/" << gVkPerf.vb_skip.load()
                                   << " ibbind " << gVkPerf.ib_bind.load() << "/" << gVkPerf.ib_skip.load()
                                   << " | pass scene=" << gVkPerf.draws_pass[0].load()
                                   << " shadow=" << gVkPerf.draws_pass[1].load()
                                   << " occl=" << gVkPerf.draws_pass[2].load()
                                   << " probe=" << gVkPerf.draws_pass[3].load()
                                   << " hero=" << gVkPerf.draws_pass[4].load()
                                   << " | shmap " << gVkPerf.draws_shadow_map[0].load()
                                   << "/" << gVkPerf.draws_shadow_map[1].load()
                                   << "/" << gVkPerf.draws_shadow_map[2].load()
                                   << "/" << gVkPerf.draws_shadow_map[3].load()
                                   << " spot " << gVkPerf.draws_shadow_map[4].load()
                                   << "/" << gVkPerf.draws_shadow_map[5].load()
                                   << " culled=" << gVkPerf.shadow_cull.load()
                                   << " rigged=" << gVkPerf.shadow_rigged.load()
                                   << " rigmap " << gVkPerf.shadow_rigged_map[0].load()
                                   << "/" << gVkPerf.shadow_rigged_map[1].load()
                                   << "/" << gVkPerf.shadow_rigged_map[2].load()
                                   << "/" << gVkPerf.shadow_rigged_map[3].load()
                                   << " spot " << gVkPerf.shadow_rigged_map[4].load()
                                   << "/" << gVkPerf.shadow_rigged_map[5].load()
                                   << " | bkt patch=" << gVkPerf.bkt_patch.load()
                                   << " range=" << gVkPerf.bkt_range.load()
                                   << " rec=" << gVkPerf.bkt_rec.load()
                                   << " skip=" << gVkPerf.bkt_skip.load()
                                   << " | mdi call=" << gVkPerf.mdi_call.load()
                                   << " rec=" << gVkPerf.mdi_rec.load()
                                   << " zero=" << gVkPerf.mdi_zero.load()
                                   << " dyn=" << gVkPerf.mdi_dyn.load()
                                   << " full=" << gVkPerf.mdi_full.load()
                                   << " | alp run=" << gVkPerf.alp_run.load()
                                   << " col=" << gVkPerf.alp_col.load()
                                   << " inl=" << gVkPerf.alp_inl.load()
                                   << [](){ std::string s;
                                        static const char* names[12] = {
                                            "mtx","sel","pal","tex","bld","ubo",
                                            "set","drw","app","flu","emi","prep" };
                                        for (U32 i = 0; i < 12; ++i) {
                                            const U64 us = gVkPerf.alpha_us[i].load();
                                            if (us != 0) {
                                                s += llformat(" %s=%.1f", names[i], us / 1000.0);
                                            }
                                        }
                                        return s; }()
                                   << [](){ std::string s = " | als";
                                        static const char* pn[4] = { "ear","mem","h1","bld" };
                                        for (U32 i = 0; i < 4; ++i) {
                                            s += llformat(" %s=%llu/%.1f", pn[i],
                                                          (unsigned long long)gVkPerf.als_n[i].load(),
                                                          gVkPerf.als_us[i].load() / 1000.0);
                                        }
                                        s += " cz";
                                        static const char* cn[8] = { "nb","gltf","lane","set","topo","val","mhdr","mval" };
                                        for (U32 i = 0; i < 8; ++i) {
                                            s += llformat(" %s=%llu", cn[i],
                                                          (unsigned long long)gVkPerf.als_cause[i].load());
                                        }
                                        s += llformat(" vp=%llu/%llu/%llu/%llu/%llu vk=r%llu/l%llu",
                                                      (unsigned long long)gVkPerf.als_val_pass[0].load(),
                                                      (unsigned long long)gVkPerf.als_val_pass[1].load(),
                                                      (unsigned long long)gVkPerf.als_val_pass[2].load(),
                                                      (unsigned long long)gVkPerf.als_val_pass[3].load(),
                                                      (unsigned long long)gVkPerf.als_val_pass[4].load(),
                                                      (unsigned long long)gVkPerf.als_val_ring.load(),
                                                      (unsigned long long)gVkPerf.als_val_l3.load());
                                        s += llformat(" prf s=%llu lv=%llu cv=%llu",
                                                      (unsigned long long)gVkPerf.pin_store.load(),
                                                      (unsigned long long)gVkPerf.pin_refuse_live.load(),
                                                      (unsigned long long)gVkPerf.pin_refuse_cover.load());
                                        s += llformat(" | setb asm=%.1f dyn=%.1f ens=%.1f fill=%.1f ehit=%llu ealloc=%llu",
                                                      gVkPerf.setb_us[0].load() / 1000.0,
                                                      gVkPerf.setb_us[1].load() / 1000.0,
                                                      gVkPerf.setb_us[2].load() / 1000.0,
                                                      gVkPerf.setb_us[3].load() / 1000.0,
                                                      (unsigned long long)gVkPerf.ens_hit.load(),
                                                      (unsigned long long)gVkPerf.ens_alloc.load());
                                        return s; }()
                                   << [](){ std::string s = " | rhw";
                                        static const char* rn[16] = {
                                            "atm","sky","ao","glb","wfg","wtv","rp","rps",
                                            "rpf","ssr","lt","lts","ptf","pt","shu","dfu" };
                                        for (U32 i = 0; i < 16; ++i) {
                                            const U64 hw = gVkPerf.ring_hw[i].load();
                                            if (hw != 0) {
                                                s += llformat(" %s=%llu", rn[i],
                                                              (unsigned long long)hw);
                                            }
                                        }
                                        return s; }()
                                   << " | emi grp=" << gVkPerf.emi_grp.load()
                                   << " rtn=" << gVkPerf.emi_rtn.load()
                                   << " n=" << gVkPerf.emi_n[0].load()
                                   << "/" << gVkPerf.emi_n[1].load()
                                   << "/" << gVkPerf.emi_n[2].load()
                                   << "/" << gVkPerf.emi_n[3].load()
                                   << [](){ std::string s;
                                        static const char* names[8] = {
                                            "rt","lgt","bnd","pal","tex","mat","buf","drw" };
                                        for (U32 i = 0; i < 8; ++i) {
                                            const U64 us = gVkPerf.emi_us[i].load();
                                            if (us != 0) {
                                                s += llformat(" %s=%.1f", names[i], us / 1000.0);
                                            }
                                        }
                                        return s; }()
                                   << " | lgt " << [](){ std::string s;
                                        static const char* names[8] = {
                                            "sun","blur","atm","loc","spot","fsl","fwd","bindD" };
                                        for (U32 i = 0; i < 8; ++i) {
                                            const U64 us = gVkPerf.lgt_us[i].load();
                                            if (us != 0) {
                                                s += llformat(" %s=%.1f", names[i], us / 1000.0);
                                            }
                                        }
                                        U64 disjoint = 0;
                                        for (U32 i = 0; i < 7; ++i) disjoint += gVkPerf.lgt_us[i].load();
                                        const U64 ph = gVkPerf.phase_us[11].load();
                                        const F64 misc = (ph > disjoint) ? (ph - disjoint) / 1000.0 : 0.0;
                                        s += llformat(" misc=%.1f nl=%llu ns=%llu", misc,
                                            (unsigned long long)gVkPerf.lgt_nl.load(),
                                            (unsigned long long)gVkPerf.lgt_ns.load());
                                        return s; }()
                                   << llformat(" | e3 rig=%.2f/%.2f/%.2f pal=%.2f",
                                        gVkPerf.e3_rig_us[0].load() / 1000.0,
                                        gVkPerf.e3_rig_us[1].load() / 1000.0,
                                        gVkPerf.e3_rig_us[2].load() / 1000.0,
                                        gVkPerf.e3_pal_us.load() / 1000.0)
                                   << " | tex enq=" << gVkPerf.tex_enq.load()
                                   << " pub=" << gVkPerf.tex_pub.load()
                                   << " fail=" << gVkPerf.tex_fail.load()
                                   << " dec=" << gVkPerf.tex_dec.load()
                                   << " stg_mb=" << (sOneShotStagingBytes.load() >> 20)
                                   << " | geo enq=" << gVkPerf.geo_enq.load()
                                   << " pub=" << gVkPerf.geo_pub.load()
                                   << " pub_ms=" << (gVkPerf.geo_pub_us.load() / 1000.0)
                                   << " dis=" << gVkPerf.geo_dis.load()
                                   << " inl=" << gVkPerf.geo_inl.load()
                                   << " defer=" << gVkPerf.geo_defer.load()
                                   << " rsnA=" << gVkPerf.geo_rsn_alpha.load()
                                   << "/" << gVkPerf.geo_rsn_afill.load()
                                   << " rsnG=" << gVkPerf.geo_rsn_geom.load()
                                   << "/" << gVkPerf.geo_rsn_gfill.load()
                                   << " rsnGB=" << gVkPerf.geo_rsn_geomb.load()
                                   << "/" << gVkPerf.geo_rsn_gbfill.load()
                                   << " snap_mb=" << (gVkPerf.geo_snap_bytes.load() >> 20)
                                   << " mb=" << (gVkGeoInflightBytes.load() >> 20)
                                   << " | gds " << [](){ std::string s;
                                        static const char* names[24] = {
                                            "animset","animclr","octadd","octrem","requeue","texanim",
                                            "dirtySG","sss","part","texdirty","octtrav","flexi",
                                            "vol","lod","grass","octaddB","octremB","facemap","sculpt","color",
                                            "pokeOK","pokeFB","x22","x23" };
                                        for (U32 i = 0; i < 24; ++i) {
                                            const U64 v = gVkPerf.geo_dirty_site[i].load();
                                            if (v != 0) {
                                                s += llformat("%s=%llu ", names[i], (unsigned long long)v);
                                            }
                                        }
                                        return s; }()
                                   << " | bake enq=" << gVkPerf.bake_enq.load()
                                   << " pub=" << gVkPerf.bake_pub.load()
                                   << " defer=" << gVkPerf.bake_defer.load()
                                   << " drain_ms=" << (gVkPerf.bake_drain_us.load() / 1000.0)
                                   << " | fam " << [](){ std::string s;
                                        static const char* pool_names[24] = {
                                            "p0","sky","wexcl","wlsky","simple","fbright","bump","mat",
                                            "pbr","terrain","grass","pbrmask","tree","amask","fbmask","avatar",
                                            "ctrlav","glow","alphaPre","voidwtr","water","alphaPost","alpha","p23" };
                                        for (U32 i = 0; i < 24; ++i) {
                                            const U64 us = gVkPerf.fam_us[i].load();
                                            const U64 d  = gVkPerf.fam_draws[i].load();
                                            if (us != 0 || d != 0) {
                                                s += llformat("%s=%lluus/%llud ", pool_names[i],
                                                    (unsigned long long)us, (unsigned long long)d);
                                            }
                                        }
                                        s += llformat("rig=%llu skin_up=%llu sk_bl=%llu/%llu sk_base=%llu",
                                            (unsigned long long)gVkPerf.rigged_rec.load(),
                                            (unsigned long long)gVkPerf.skin_up.load(),
                                            (unsigned long long)gVkPerf.skin_bl_fill.load(),
                                            (unsigned long long)gVkPerf.skin_bl_of.load(),
                                            (unsigned long long)gVkPerf.skin_base_wr.load());
                                        return s; }()
                                   << " | ph " << [](){ std::string s;
                                        static const char* names[16] = {
                                            "idle","disp","probe","hero","gupd","cull","shad","imp",
                                            "img","sort","geom","light","ui","swap","x14","x15" };
                                        for (U32 i = 0; i < 16; ++i) {
                                            const U64 us = gVkPerf.phase_us[i].load();
                                            if (us != 0) {
                                                s += llformat("%s=%.1f ", names[i], us / 1000.0);
                                            }
                                        }
                                        return s; }()
                                   << " | idl " << [](){ std::string s;
                                        static const char* names[32] = {
                                            "tmr","gltf","work","agt","net","stat","cb","ui",
                                            "mov","obj","dead","hud","vlm","wld","upmv","part",
                                            "cam","misc","lod","avnfo","aud","dObj","dDrw","oAv",
                                            "oNav","oFlex","oTanim","oMisc","aChar","aMisc","aName","aPre" };
                                        for (U32 i = 0; i < 32; ++i) {
                                            const U64 us = gVkPerf.idle_us[i].load();
                                            if (us != 0) {
                                                s += llformat("%s=%.1f ", names[i], us / 1000.0);
                                            }
                                        }
                                        return s; }()
                                   << " | mlp " << [](){ std::string s;
                                        static const char* names[16] = {
                                            "slot","fence","acq","beg","end","coro","pump","rld",
                                            "snap","tio","mesh","trc","x12","x13","x14","x15" };
                                        for (U32 i = 0; i < 16; ++i) {
                                            const U64 us = gVkPerf.mlp_us[i].load();
                                            if (us != 0) {
                                                s += llformat("%s=%.1f ", names[i], us / 1000.0);
                                            }
                                        }
                                        return s; }()
                                   << " | img " << [](){ std::string s;
                                        static const char* names[12] = {
                                            "cls","bmp","fc","fet","pri","ftc",
                                            "crt","drn","cbk","mat","x10","x11" };
                                        for (U32 i = 0; i < 12; ++i) {
                                            const U64 us = gVkPerf.img_us[i].load();
                                            if (us != 0) {
                                                s += llformat("%s=%.1f ", names[i], us / 1000.0);
                                            }
                                        }
                                        s += llformat("psk=%llu/%llu",
                                            (unsigned long long)gVkPerf.img_pri_skip.load(),
                                            (unsigned long long)gVkPerf.img_pri_full.load());
                                        return s; }()
                                   << " | mega " << [](){ U64 c,cap,use; megabufStats(c,cap,use);
                                        return llformat("chunks=%llu used=%.1f/%.1fMB",
                                            (unsigned long long)c, use/1048576.0, cap/1048576.0); }()
                                   << " | pe sub_ms=" << ((F64)sPESubmitUs.exchange(0) / 1000.0)
                                   << " prs_ms=" << ((F64)sPEPresentUs.exchange(0) / 1000.0)
                                   << " mt=" << (sPEThreaded ? 1 : 0)
                                   << " rw=" << recordWorkerCount()
                                   << LL_ENDL;
            }
            gVkPerf.reset();
#if LL_LINUX
            {
                static U64 s_prev_busy[32]  = {};
                static U64 s_prev_total[32] = {};
                static U64 s_prev_thread_us = 0;
                FILE* f = fopen("/proc/stat", "r");
                if (f != nullptr)
                {
                    std::string cpu_line;
                    char lbuf[256];
                    while (fgets(lbuf, sizeof(lbuf), f) != nullptr)
                    {
                        U32 idx = 0;
                        unsigned long long u, n, s, i, w, irq, sirq, st;
                        if (sscanf(lbuf, "cpu%u %llu %llu %llu %llu %llu %llu %llu %llu",
                                   &idx, &u, &n, &s, &i, &w, &irq, &sirq, &st) == 9
                            && idx < 32)
                        {
                            const U64 busy  = u + n + s + irq + sirq + st;
                            const U64 total = busy + i + w;
                            const U64 db = busy - s_prev_busy[idx];
                            const U64 dt = total - s_prev_total[idx];
                            s_prev_busy[idx]  = busy;
                            s_prev_total[idx] = total;
                            if (dt > 0)
                            {
                                char pb[8];
                                snprintf(pb, sizeof(pb), "%s%u", cpu_line.empty() ? "" : "/",
                                         (U32)(db * 100 / dt));
                                cpu_line += pb;
                            }
                        }
                    }
                    fclose(f);

                    struct rusage ru;
                    U32 main_pct = 0;
                    if (getrusage(RUSAGE_THREAD, &ru) == 0)
                    {
                        const U64 thread_us = (U64)ru.ru_utime.tv_sec * 1000000ull + ru.ru_utime.tv_usec
                                            + (U64)ru.ru_stime.tv_sec * 1000000ull + ru.ru_stime.tv_usec;
                        const F64 wall_us = elapsed * 1000000.0;
                        if (s_prev_thread_us > 0 && wall_us > 0)
                        {
                            main_pct = (U32)llclamp((F64)(thread_us - s_prev_thread_us) * 100.0 / wall_us, 0.0, 100.0);
                        }
                        s_prev_thread_us = thread_us;
                    }

                    LL_INFOS("VkPerf") << "cpu main=" << main_pct << "% cores=" << cpu_line << LL_ENDL;
                }
            }
#endif
            s_last_emit  = now;
            s_last_frame = sMonotonicFrameCount;
        }
    }

    if (!sConsumerActiveThisFrame)
    {
        endSwapchainRendering();

        if (sImageAcquired &&
            sAcquiredImageIndex < (U32)sSwapchainImages.size() &&
            sSwapchainImages[sAcquiredImageIndex] != VK_NULL_HANDLE)
        {
            const VkImageLayout present_old =
                (sUISceneSplit && !sSwapchainClearedThisFrame)
                    ? VK_IMAGE_LAYOUT_UNDEFINED
                    : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            transitionImageLayoutVk(sSwapchainImages[sAcquiredImageIndex],
                                    VK_IMAGE_ASPECT_COLOR_BIT,
                                    present_old,
                                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                    0);
        }
    }

    if (sUISceneSplit)
    {
        tRecordCmdOverride = VK_NULL_HANDLE;
        VkResult cend = vkEndCommandBuffer(sConsumerCommandBuffers[sFrameIndex]);
        if (cend != VK_SUCCESS)
        {
            sInFrame = false;
            sImageAcquired = false;
            return false;
        }
    }

    VkResult result = vkEndCommandBuffer(sCommandBuffers[sFrameIndex]);
    if (result != VK_SUCCESS)
    {
        sInFrame = false;
        sImageAcquired = false;
        return false;
    }

    if (sUISceneSplit && sConsumerActiveThisFrame && isUISceneAsync())
    {
        PEJob cjob;
        cjob.is_frame = true;
        cjob.slot     = sFrameIndex;
        cjob.pre_cmds = std::move(sPendingPreFrameCmds);
        sPendingPreFrameCmds.clear();
        cjob.cmd      = sConsumerCommandBuffers[sFrameIndex];
        cjob.fence    = sInFlightFences[sFrameIndex];
        if (sImageAcquired)
        {
            cjob.wait_semaphore   = sImageAvailableSemaphores[sFrameIndex];
            cjob.signal_semaphore = sRenderFinishedSemaphores[sFrameIndex];
            if (sSwapchain != VK_NULL_HANDLE)
            {
                PEPresentTarget target;
                target.swapchain      = sSwapchain;
                target.image_index    = sAcquiredImageIndex;
                target.wait_semaphore = sRenderFinishedSemaphores[sFrameIndex];
                cjob.presents.push_back(target);
            }
        }
        sFrameSubmittedMonotonic[sFrameIndex] = sMonotonicFrameCount;
        sPESlotState[sFrameIndex].store(PE_SLOT_PENDING);
        peEnqueue(std::move(cjob));

        PEJob pjob;
        pjob.is_frame = false;
        pjob.slot     = sFrameIndex;
        pjob.cmd      = sCommandBuffers[sFrameIndex];
        pjob.fence    = sProducerFences[sFrameIndex];
        sProducerFencePending[sFrameIndex] = true;
        peEnqueue(std::move(pjob));

        if (sAsyncRenderSceneThisFrame)
        {
            vkEndCommandBuffer(sAsyncProducerCommandBuffer);
            PEJob apjob;
            apjob.is_frame = false;
            apjob.cmd      = sAsyncProducerCommandBuffer;
            apjob.fence    = sAsyncProducerFence;
            peEnqueue(std::move(apjob));
            sAsyncProducerInFlight        = true;
            sAsyncProducerSubmitMonotonic = sMonotonicFrameCount;
            sAsyncRenderSceneThisFrame    = false;
        }
    }
    else if (sUISceneSplit && sConsumerActiveThisFrame)
    {
        PEJob pjob;
        pjob.is_frame = false;
        pjob.slot     = sFrameIndex;
        pjob.pre_cmds = std::move(sPendingPreFrameCmds);
        sPendingPreFrameCmds.clear();
        pjob.cmd      = sCommandBuffers[sFrameIndex];
        pjob.fence    = sProducerFences[sFrameIndex];

        PEJob cjob;
        cjob.is_frame = true;
        cjob.slot     = sFrameIndex;
        cjob.cmd      = sConsumerCommandBuffers[sFrameIndex];
        cjob.fence    = sInFlightFences[sFrameIndex];
        if (sImageAcquired)
        {
            cjob.wait_semaphore   = sImageAvailableSemaphores[sFrameIndex];
            cjob.signal_semaphore = sRenderFinishedSemaphores[sFrameIndex];
            if (sSwapchain != VK_NULL_HANDLE)
            {
                PEPresentTarget target;
                target.swapchain      = sSwapchain;
                target.image_index    = sAcquiredImageIndex;
                target.wait_semaphore = sRenderFinishedSemaphores[sFrameIndex];
                cjob.presents.push_back(target);
            }
        }
        sFrameSubmittedMonotonic[sFrameIndex] = sMonotonicFrameCount;
        sPESlotState[sFrameIndex].store(PE_SLOT_PENDING);
        sProducerFencePending[sFrameIndex] = true;
        peEnqueue(std::move(cjob));
        peEnqueue(std::move(pjob));
    }
    else
    {
        PEJob job;
        job.is_frame = true;
        job.slot     = sFrameIndex;
        job.pre_cmds = std::move(sPendingPreFrameCmds);
        sPendingPreFrameCmds.clear();
        job.cmd      = sCommandBuffers[sFrameIndex];
        job.fence    = sInFlightFences[sFrameIndex];
        if (sImageAcquired)
        {
            job.wait_semaphore   = sImageAvailableSemaphores[sFrameIndex];
            job.signal_semaphore = sRenderFinishedSemaphores[sFrameIndex];
            if (sSwapchain != VK_NULL_HANDLE)
            {
                PEPresentTarget target;
                target.swapchain      = sSwapchain;
                target.image_index    = sAcquiredImageIndex;
                target.wait_semaphore = sRenderFinishedSemaphores[sFrameIndex];
                job.presents.push_back(target);
            }
        }
        sFrameSubmittedMonotonic[sFrameIndex] = sMonotonicFrameCount;
        sPESlotState[sFrameIndex].store(PE_SLOT_PENDING);
        peEnqueue(std::move(job));
    }

    sImageAcquired = false;
    sInFrame = false;
    return true;
}

bool beginOffscreenFrameVk()
{
    if (!sInitialized)
    {
        return false;
    }
    if (sInFrame)
    {
        return false;
    }

    beginCommandRecording();

    const bool slot_submitted = peWaitSlotSubmitted(sFrameIndex);
    if (sInFlightFences[sFrameIndex] != VK_NULL_HANDLE)
    {
        if (slot_submitted)
        {
            vkWaitForFences(sDevice, 1, &sInFlightFences[sFrameIndex],
                                                VK_TRUE, UINT64_MAX);
            if (sFrameSubmittedMonotonic[sFrameIndex] > sLastCompletedMonotonic)
            {
                sLastCompletedMonotonic = sFrameSubmittedMonotonic[sFrameIndex];
            }
        }
        vkResetFences(sDevice, 1, &sInFlightFences[sFrameIndex]);
    }
    if (sProducerFencePending[sFrameIndex] && sProducerFences[sFrameIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(sDevice, 1, &sProducerFences[sFrameIndex], VK_TRUE, UINT64_MAX);
        vkResetFences(sDevice, 1, &sProducerFences[sFrameIndex]);
        sProducerFencePending[sFrameIndex] = false;
    }
    if (sAsyncProducerInFlight && sAsyncProducerSubmitMonotonic > 0 &&
        sLastCompletedMonotonic >= sAsyncProducerSubmitMonotonic)
    {
        sLastCompletedMonotonic = sAsyncProducerSubmitMonotonic - 1;
    }
    if (sAsyncProducerInFlight && sAsyncProducerRecordSlot == sFrameIndex &&
        sAsyncProducerFence != VK_NULL_HANDLE)
    {
        vkWaitForFences(sDevice, 1, &sAsyncProducerFence, VK_TRUE, UINT64_MAX);
    }
    sPESlotState[sFrameIndex].store(PE_SLOT_IDLE);

    if (sFrameIndex < FRAMES_IN_FLIGHT)
    {
        sMatrixRingUsedThisFrame[sFrameIndex] = 0;
        sMatrixRingHasCurrent[sFrameIndex]    = false;
    }

    vkResetCommandBuffer(sCommandBuffers[sFrameIndex], 0);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkResult result = vkBeginCommandBuffer(sCommandBuffers[sFrameIndex], &begin_info);
    if (result != VK_SUCCESS)
    {
        return false;
    }

    sInFrame = true;
    return true;
}

void endOffscreenFrameVk()
{
    if (!sInitialized || !sInFrame)
    {
        return;
    }

    if (sInDynamicRendering)
    {
        vkCmdEndRendering(sCommandBuffers[sFrameIndex]);
        sInDynamicRendering = false;
    }

    VkResult result = vkEndCommandBuffer(sCommandBuffers[sFrameIndex]);
    if (result != VK_SUCCESS)
    {
        sInFrame = false;
        return;
    }

    {
        PESyncPoint sync;
        PEJob job;
        job.pre_cmds  = std::move(sPendingPreFrameCmds);
        sPendingPreFrameCmds.clear();
        job.cmd       = sCommandBuffers[sFrameIndex];
        job.fence     = sInFlightFences[sFrameIndex];
        job.wait_idle = true;
        job.sync      = &sync;
        if (!sPERunning)
        {
            peExecute(job);
        }
        else
        {
            {
                std::lock_guard<std::mutex> lk(sPEQueueMutex);
                sPEJobs.push_back(std::move(job));
            }
            sPEQueueCv.notify_one();
            std::unique_lock<std::mutex> lk(sync.m);
            sync.cv.wait(lk, [&] { return sync.done; });
        }
    }
    sFrameSubmittedMonotonic[sFrameIndex] = sMonotonicFrameCount;
    if (sMonotonicFrameCount > sLastCompletedMonotonic)
    {
        sLastCompletedMonotonic = sMonotonicFrameCount;
    }

    sInFrame = false;
}

VkCommandBuffer getCurrentCommandBuffer()
{
    return currentRecordCmd();
}

uint32_t acquireOcclusionQueryVk()
{
    if (sOcclusionQueryPool == VK_NULL_HANDLE)
    {
        return 0;
    }
    if (sOcclusionQueryFree.empty())
    {
        static U32 s_exhausted_count = 0;
        ++s_exhausted_count;
        if ((s_exhausted_count & (s_exhausted_count - 1)) == 0)
        {
            LL_WARNS("Vulkan") << "occlusion query pool exhausted (capacity="
                               << sOcclusionQueryCapacity
                               << " pending_release=" << (S32)sPendingOcclusionQueryReleases.size()
                               << " total_misses=" << s_exhausted_count << ")" << LL_ENDL;
        }
        return 0;
    }
    uint32_t index = sOcclusionQueryFree.front();
    sOcclusionQueryFree.pop();
    vkResetQueryPool(sDevice, sOcclusionQueryPool, index, 1);
    return index + 1;
}

void releaseOcclusionQueryVk(uint32_t handle)
{
    if (handle == 0 || sOcclusionQueryPool == VK_NULL_HANDLE)
    {
        return;
    }
    uint32_t index = handle - 1;
    if (index < sOcclusionQueryCapacity)
    {
        PendingQueryRelease pending;
        pending.index         = index;
        pending.enqueue_frame = sMonotonicFrameCount;
        sPendingOcclusionQueryReleases.push_back(pending);
    }
}

void tickDeferredQueryReleaseQueue()
{
    size_t w = 0;
    const size_t n = sPendingOcclusionQueryReleases.size();
    for (size_t r = 0; r < n; ++r)
    {
        PendingQueryRelease& e = sPendingOcclusionQueryReleases[r];
        if (reapReady(e.enqueue_frame))
        {
            sOcclusionQueryFree.push(e.index);
        }
        else
        {
            if (w != r)
            {
                sPendingOcclusionQueryReleases[w] = e;
            }
            ++w;
        }
    }
    sPendingOcclusionQueryReleases.resize(w);
}

void cmdBeginOcclusionQueryVk(VkCommandBuffer cmd, uint32_t handle)
{
    if (handle == 0 || cmd == VK_NULL_HANDLE || sOcclusionQueryPool == VK_NULL_HANDLE)
    {
        return;
    }
    vkCmdBeginQuery(cmd, sOcclusionQueryPool, handle - 1, 0);
}

void cmdEndOcclusionQueryVk(VkCommandBuffer cmd, uint32_t handle)
{
    if (handle == 0 || cmd == VK_NULL_HANDLE || sOcclusionQueryPool == VK_NULL_HANDLE)
    {
        return;
    }
    vkCmdEndQuery(cmd, sOcclusionQueryPool, handle - 1);
}

bool getOcclusionQueryResultVk(uint32_t handle, bool& available, uint64_t& samples)
{
    available = false;
    samples   = 0;
    if (handle == 0 || sOcclusionQueryPool == VK_NULL_HANDLE)
    {
        return false;
    }
    uint64_t results[2] = { 0, 0 };
    VkResult r = vkGetQueryPoolResults(sDevice, sOcclusionQueryPool, handle - 1, 1,
                                       sizeof(results), results, sizeof(results),
                                       VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
    if (r == VK_SUCCESS && results[1] != 0)
    {
        available = true;
        samples   = results[0];
    }
    return true;
}

bool isTimestampSupportedVk()
{
    return sTimestampSupported;
}

uint32_t acquireTimestampPairVk()
{
    if (sTimestampQueryPool == VK_NULL_HANDLE || !sTimestampSupported || sTimestampPairFree.empty())
    {
        return 0;
    }
    uint32_t index = sTimestampPairFree.front();
    sTimestampPairFree.pop();
    vkResetQueryPool(sDevice, sTimestampQueryPool, index * 2, 2);
    return index + 1;
}

void releaseTimestampPairVk(uint32_t handle)
{
    if (handle == 0 || sTimestampQueryPool == VK_NULL_HANDLE)
    {
        return;
    }
    uint32_t index = handle - 1;
    if (index < sTimestampPairCapacity)
    {
        sTimestampPairFree.push(index);
    }
}

void cmdWriteTimestampBeginVk(VkCommandBuffer cmd, uint32_t handle)
{
    if (handle == 0 || cmd == VK_NULL_HANDLE || sTimestampQueryPool == VK_NULL_HANDLE)
    {
        return;
    }
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, sTimestampQueryPool, (handle - 1) * 2);
}

void cmdWriteTimestampEndVk(VkCommandBuffer cmd, uint32_t handle)
{
    if (handle == 0 || cmd == VK_NULL_HANDLE || sTimestampQueryPool == VK_NULL_HANDLE)
    {
        return;
    }
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, sTimestampQueryPool, (handle - 1) * 2 + 1);
}

bool getTimestampElapsedNsVk(uint32_t handle, bool& available, uint64_t& elapsed_ns)
{
    available  = false;
    elapsed_ns = 0;
    if (handle == 0 || sTimestampQueryPool == VK_NULL_HANDLE)
    {
        return false;
    }
    uint32_t index = handle - 1;
    uint64_t results[4] = { 0, 0, 0, 0 };
    VkResult r = vkGetQueryPoolResults(sDevice, sTimestampQueryPool, index * 2, 2,
                                       sizeof(results), results, sizeof(uint64_t) * 2,
                                       VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
    if (r == VK_SUCCESS && results[1] != 0 && results[3] != 0)
    {
        uint64_t mask = (sTimestampValidBits >= 64) ? ~0ull : ((1ull << sTimestampValidBits) - 1);
        uint64_t diff = (results[2] - results[0]) & mask;
        elapsed_ns = (uint64_t)((double)diff * (double)sTimestampPeriodNs);
        available  = true;
    }
    return true;
}

bool isVulkanInitialized()
{
    return sInitialized;
}

void setVsyncEnabled(bool enabled)
{
    sVsyncEnabled.store(enabled);
    sSwapchainRecreatePending = true;
}

bool isInFrame()
{
    return sInFrame;
}

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

VkDevice getDevice()
{
    return sDevice;
}

VkPipelineLayout createStandardPipelineLayout(
    const VkDescriptorSetLayout* descriptor_set_layouts,
    U32                          descriptor_set_layout_count,
    const VkPushConstantRange*   push_constant_ranges,
    U32                          push_constant_range_count)
{
    if (sDevice == VK_NULL_HANDLE)
    {
        return VK_NULL_HANDLE;
    }

    VkPipelineLayoutCreateInfo info = {};
    info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount         = descriptor_set_layout_count;
    info.pSetLayouts            = descriptor_set_layouts;
    info.pushConstantRangeCount = push_constant_range_count;
    info.pPushConstantRanges    = push_constant_ranges;

    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkResult result = vkCreatePipelineLayout(sDevice, &info, nullptr, &layout);
    if (result != VK_SUCCESS)
    {
        return VK_NULL_HANDLE;
    }
    return layout;
}

bool compileGraphicsPipeline(const VkGraphicsPipelineCreateInfo& ci, VkPipeline& out_pipeline)
{
    if (sDevice == VK_NULL_HANDLE)
    {
        return false;
    }

    VkResult result = vkCreateGraphicsPipelines(sDevice, sPipelineCache, 1, &ci, nullptr, &out_pipeline);
    if (result != VK_SUCCESS)
    {
        out_pipeline = VK_NULL_HANDLE;
        return false;
    }
    return true;
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

    PerDrawDescLane& lane = sPerDrawDescLanes[tRecordLaneIndex];
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
            image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

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

        VkDescriptorBufferInfo shared_ubo_infos[11]  = {};
        VkWriteDescriptorSet   shared_ubo_writes[11] = {};
        U32                    shared_count          = 0;

        auto add_shared_ubo = [&](U32 binding, VkBuffer buf, VkDeviceSize size)
        {
            if (buf == VK_NULL_HANDLE || size == 0)
            {
                return;
            }
            if (binding < 64 && ((b.layout_binding_mask >> binding) & 1) == 0)
            {
                return;
            }
            shared_ubo_infos[shared_count].buffer = buf;
            shared_ubo_infos[shared_count].offset = 0;
            shared_ubo_infos[shared_count].range  = size;

            shared_ubo_writes[shared_count].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            shared_ubo_writes[shared_count].dstSet          = target_set;
            shared_ubo_writes[shared_count].dstBinding      = binding;
            shared_ubo_writes[shared_count].dstArrayElement = 0;
            shared_ubo_writes[shared_count].descriptorCount = 1;
            shared_ubo_writes[shared_count].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            shared_ubo_writes[shared_count].pBufferInfo     = &shared_ubo_infos[shared_count];
            ++shared_count;
        };

        VkBuffer sbuf = VK_NULL_HANDLE; void* smap = nullptr;
        if (getSharedWindlightAtmosUBO(sbuf, smap)) add_shared_ubo(8,  sbuf, sizeof(WindlightAtmos_PerProgramBind));
        sbuf = VK_NULL_HANDLE; smap = nullptr;
        if (getSharedWindlightSkyUBO(sbuf, smap))   add_shared_ubo(9,  sbuf, sizeof(WindlightSky_PerProgramBind));
        sbuf = VK_NULL_HANDLE; smap = nullptr;
        if (getSharedWindlightHDRUBO(sbuf, smap))   add_shared_ubo(10, sbuf, sizeof(WindlightHDR_PerProgramBind));
        sbuf = VK_NULL_HANDLE; smap = nullptr;
        if (getSharedWindlightLightUBO(sbuf, smap)) add_shared_ubo(11, sbuf, sizeof(WindlightLight_PerProgramBind));
        sbuf = VK_NULL_HANDLE; smap = nullptr;
        if (getSharedWaterFogUBO(sbuf, smap))       add_shared_ubo(14, sbuf, sizeof(WaterFog_PerProgramBind));
        sbuf = VK_NULL_HANDLE; smap = nullptr;
        if (getSharedGlobalFUBO(sbuf, smap))        add_shared_ubo(18, sbuf, sizeof(GlobalF_PerProgramBind));
        sbuf = VK_NULL_HANDLE; smap = nullptr;
        if (getSharedAoUtilUBO(sbuf, smap))         add_shared_ubo(22, sbuf, sizeof(AoUtil_PerProgramBind));
        sbuf = VK_NULL_HANDLE; smap = nullptr;
        if (getSharedTonemapUtilFUBO(sbuf, smap))   add_shared_ubo(26, sbuf, sizeof(TonemapUtilF_PerProgramBind));
        sbuf = VK_NULL_HANDLE; smap = nullptr;
        if (getSharedDeferredUtilUBO(sbuf, smap))   add_shared_ubo(30, sbuf, sizeof(DeferredUtil_PerProgramBind));
        sbuf = VK_NULL_HANDLE; smap = nullptr;
        if (getSharedShadowUtilUBO(sbuf, smap))     add_shared_ubo(31, sbuf, sizeof(ShadowUtil_PerProgramBind));

        if (shared_count > 0)
        {
            vkUpdateDescriptorSets(sDevice, shared_count, shared_ubo_writes, 0, nullptr);
        }
    }

    lane.lru.push_back(key);
    ScenePerDrawCacheEntry& entry = lane.cache.try_emplace(key).first->second;
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

U32 getCurrentFrameIndex()
{
    return sFrameIndex;
}

U32 getMonotonicFrameCount()
{
    return sMonotonicFrameCount;
}

U32 getLastCompletedMonotonic()
{
    return sLastCompletedMonotonic;
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

thread_local float sCurrentModelviewMatrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

void pushCurrentModelviewMatrix(const float modelview_matrix[16])
{
    if (modelview_matrix != nullptr)
    {
        std::memcpy(sCurrentModelviewMatrix, modelview_matrix, sizeof(sCurrentModelviewMatrix));
    }
}

const float* getCurrentModelviewMatrix()
{
    return sCurrentModelviewMatrix;
}

void beginDynamicRendering(U32                               width,
                           U32                               height,
                           const DynamicRenderingAttachment* color_attachments,
                           U32                               color_count,
                           const DynamicRenderingAttachment* depth_attachment)
{
    VkCommandBuffer rec_cmd = currentRecordCmd();
    if (!sInitialized || rec_cmd == VK_NULL_HANDLE)
    {
        return;
    }


    VkRenderingAttachmentInfo color_infos[4] = {};
    U32 valid_color_count = 0;
    for (U32 i = 0; i < color_count && i < 4; ++i)
    {
        if (color_attachments == nullptr || color_attachments[i].image_view == VK_NULL_HANDLE)
        {
            continue;
        }
        color_infos[valid_color_count].sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_infos[valid_color_count].imageView   = color_attachments[i].image_view;
        color_infos[valid_color_count].imageLayout = color_attachments[i].image_layout;
        color_infos[valid_color_count].loadOp      = color_attachments[i].load_op;
        color_infos[valid_color_count].storeOp     = color_attachments[i].store_op;
        color_infos[valid_color_count].clearValue  = color_attachments[i].clear_value;
        ++valid_color_count;
    }

    VkRenderingAttachmentInfo depth_info = {};
    bool has_depth = false;
    if (depth_attachment != nullptr && depth_attachment->image_view != VK_NULL_HANDLE)
    {
        depth_info.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depth_info.imageView   = depth_attachment->image_view;
        depth_info.imageLayout = depth_attachment->image_layout;
        depth_info.loadOp      = depth_attachment->load_op;
        depth_info.storeOp     = depth_attachment->store_op;
        depth_info.clearValue  = depth_attachment->clear_value;
        has_depth = true;
    }

    if (valid_color_count == 0 && !has_depth)
    {
        static U32 s_no_attachment_begins = 0;
        ++s_no_attachment_begins;
        if ((s_no_attachment_begins & (s_no_attachment_begins - 1)) == 0)
        {
            LL_WARNS("Vulkan") << "beginDynamicRendering: 有効 attachment ゼロで begin 不能 = 直前の pass が開いたままなら後続 draw は誤 target に落ちる"
                               << " req=" << width << "x" << height
                               << " colors=" << color_count
                               << " in_pass=" << (sInDynamicRendering ? 1 : 0)
                               << " count=" << s_no_attachment_begins << LL_ENDL;
        }
        return;
    }

    VkRenderingInfo rendering_info = {};
    rendering_info.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.renderArea.offset    = { 0, 0 };
    rendering_info.renderArea.extent    = { width, height };
    rendering_info.layerCount           = 1;
    rendering_info.viewMask             = 0;
    rendering_info.colorAttachmentCount = valid_color_count;
    rendering_info.pColorAttachments    = (valid_color_count > 0 ? color_infos : nullptr);
    rendering_info.pDepthAttachment     = (has_depth ? &depth_info : nullptr);
    rendering_info.pStencilAttachment   = nullptr;

    if (sInDynamicRendering)
    {
        vkCmdEndRendering(rec_cmd);
        sInDynamicRendering = false;
    }

    sSavedColorCount = valid_color_count;
    for (U32 i = 0; i < valid_color_count; ++i)
    {
        sSavedColorInfos[i] = color_infos[i];
    }
    sSavedHasDepth     = has_depth;
    sSavedDepthInfo    = depth_info;
    sSavedRenderWidth  = width;
    sSavedRenderHeight = height;

    sCurrentRenderAreaHeight = height;

    vkCmdBeginRendering(rec_cmd, &rendering_info);
    sInDynamicRendering = true;
}

void endDynamicRendering()
{
    VkCommandBuffer rec_cmd = currentRecordCmd();
    if (!sInitialized || rec_cmd == VK_NULL_HANDLE ||
        !sInDynamicRendering)
    {
        return;
    }

    vkCmdEndRendering(rec_cmd);
    sInDynamicRendering = false;
}

void beginSwapchainRendering()
{
    if (!sInitialized || !sInFrame || !sVulkanPresentationEnabled || !sImageAcquired ||
        sAcquiredImageIndex >= (U32)sSwapchainImageViews.size() ||
        sSwapchainImageViews[sAcquiredImageIndex] == VK_NULL_HANDLE)
    {
        return;
    }

    if (sInDynamicRendering)
    {
        vkCmdEndRendering(sCommandBuffers[sFrameIndex]);
        sInDynamicRendering = false;
    }

    const bool first_use_this_frame = !sSwapchainClearedThisFrame;

    if (sUISceneSplit && first_use_this_frame &&
        sAcquiredImageIndex < (U32)sSwapchainImages.size() &&
        sSwapchainImages[sAcquiredImageIndex] != VK_NULL_HANDLE)
    {
        transitionImageLayoutVk(sSwapchainImages[sAcquiredImageIndex],
                                VK_IMAGE_ASPECT_COLOR_BIT,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                0,
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
    }

    DynamicRenderingAttachment color = {};
    color.image_view   = sSwapchainImageViews[sAcquiredImageIndex];
    color.image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    if (first_use_this_frame)
    {
        color.load_op      = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color.store_op     = VK_ATTACHMENT_STORE_OP_STORE;
        color.clear_value  = {};
        color.clear_value.color.float32[0] = 0.0f;
        color.clear_value.color.float32[1] = 0.0f;
        color.clear_value.color.float32[2] = 0.0f;
        color.clear_value.color.float32[3] = 1.0f;
        sSwapchainClearedThisFrame = true;
    }
    else
    {
        color.load_op      = VK_ATTACHMENT_LOAD_OP_LOAD;
        color.store_op     = VK_ATTACHMENT_STORE_OP_STORE;
        color.clear_value  = {};
    }

    DynamicRenderingAttachment depth = {};
    if (sSwapchainDepthView != VK_NULL_HANDLE)
    {
        if (sSwapchainDepthLayout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            transitionImageLayoutVk(
                sSwapchainDepthImage,
                VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                sSwapchainDepthLayout,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                0,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
            sSwapchainDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        depth.image_view   = sSwapchainDepthView;
        depth.image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth.store_op     = VK_ATTACHMENT_STORE_OP_STORE;
        if (first_use_this_frame)
        {
            depth.load_op                          = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depth.clear_value                      = {};
            depth.clear_value.depthStencil.depth   = 1.0f;
            depth.clear_value.depthStencil.stencil = 0;
        }
        else
        {
            depth.load_op     = VK_ATTACHMENT_LOAD_OP_LOAD;
            depth.clear_value = {};
        }
    }

    beginDynamicRendering(sSwapchainExtent.width, sSwapchainExtent.height,
                          &color, 1,
                          sSwapchainDepthView != VK_NULL_HANDLE ? &depth : nullptr);

}

void endSwapchainRendering()
{
    if (!sInitialized || !sInFrame || !sVulkanPresentationEnabled || !sImageAcquired)
    {
        return;
    }

    endDynamicRendering();

}

void finalizeConsumerSwapchain()
{
    if (!sUISceneSplit || !sConsumerActiveThisFrame || !sInFrame)
    {
        return;
    }

    endSwapchainRendering();

    if (sImageAcquired &&
        sAcquiredImageIndex < (U32)sSwapchainImages.size() &&
        sSwapchainImages[sAcquiredImageIndex] != VK_NULL_HANDLE)
    {
        transitionImageLayoutVk(sSwapchainImages[sAcquiredImageIndex],
                                VK_IMAGE_ASPECT_COLOR_BIT,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                0);
    }

    tRecordCmdOverride = VK_NULL_HANDLE;
}

VkShaderModule loadSpirvShaderModule(const U32* spv_code, size_t code_size_bytes)
{
    if (sDevice == VK_NULL_HANDLE)
    {
        return VK_NULL_HANDLE;
    }
    if (spv_code == nullptr || code_size_bytes == 0 || (code_size_bytes % 4) != 0)
    {
        return VK_NULL_HANDLE;
    }

    VkShaderModuleCreateInfo info = {};
    info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code_size_bytes;
    info.pCode    = spv_code;

    VkShaderModule module = VK_NULL_HANDLE;
    VkResult result = vkCreateShaderModule(sDevice, &info, nullptr, &module);
    if (result != VK_SUCCESS)
    {
        return VK_NULL_HANDLE;
    }

    return module;
}

VkShaderModule loadSpirvShaderModuleFromMemory(const std::vector<unsigned int>& spirv)
{
    if (spirv.empty())
    {
        return VK_NULL_HANDLE;
    }
    return loadSpirvShaderModule(spirv.data(), spirv.size() * sizeof(unsigned int));
}

VkFormat llGlEnumToVkFormat(U32 ll_gl_intformat)
{
    return llGlEnumToVkFormatImpl(ll_gl_intformat);
}

VkCompareOp llGlEnumToVkCompareOp(U32 ll_gl_func)
{
    return llGlEnumToVkCompareOpImpl(ll_gl_func);
}

VkStencilOp llGlEnumToVkStencilOp(U32 ll_gl_op)
{
    return llGlEnumToVkStencilOpImpl(ll_gl_op);
}

U32 vkFormatBytesPerPixel(VkFormat format)
{
    return vkFormatBytesPerPixelImpl(format);
}

U32 llGlFormatSourceComponents(U32 ll_gl_format)
{
    return llGlFormatSourceComponentsImpl(ll_gl_format);
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

static void tickPerDrawUBOArena()
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

static void teardownSharedLatchedUBOs()
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
    static StructType s##BindName##Shadow;                                                              \
    static U64 s##BindName##WriteGen = 1;                                                               \
    static U64 s##BindName##UpFrame[FRAMES_IN_FLIGHT]  = { ~0ull, ~0ull, ~0ull };                       \
    static U64 s##BindName##UpGen[FRAMES_IN_FLIGHT]    = { 0, 0, 0 };                                   \
    static U32 s##BindName##UpOffset[FRAMES_IN_FLIGHT] = { 0, 0, 0 };                                   \
    static VkBuffer s##BindName##UpBuf[FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE }; \
    static U64 s##BindName##UpHash[FRAMES_IN_FLIGHT] = { 0, 0, 0 };                                     \
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

static bool initSharedDynamicPersistentUBOs()
{
    return createReflectionProbeFPersistentBuffers()
        && createSSRUtilPersistentBuffers();
}

static void teardownSharedDynamicPersistentUBOs()
{
    destroyReflectionProbeFPersistentBuffers();
    destroySSRUtilPersistentBuffers();
}

static void tickSharedDynamicPersistentUBOs()
{
    tickReflectionProbeFPersistent();
    tickSSRUtilPersistent();
}

static ObjectSkin_PerProgramBind sObjectSkinShadow;
static U64 sObjectSkinWriteGen = 1;
static U64 sObjectSkinUpFrame[FRAMES_IN_FLIGHT]  = { ~0ull, ~0ull, ~0ull };
static U64 sObjectSkinUpGen[FRAMES_IN_FLIGHT]    = { 0, 0, 0 };
static U32 sObjectSkinUpOffset[FRAMES_IN_FLIGHT] = { 0, 0, 0 };
static VkBuffer sObjectSkinUpBuf[FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
static U64 sObjectSkinUpHash[FRAMES_IN_FLIGHT] = { 0, 0, 0 };

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

static void objectSkinFrameCacheGuard()
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
    objectSkinFrameCacheGuard();
    auto it = sObjectSkinFrameCache.find(ObjectSkinFrameCacheKey{ avatar, skin_hash });
    if (it == sObjectSkinFrameCache.end())
    {
        return false;
    }
    sObjectSkinUpBuf[f]    = it->second.buf;
    sObjectSkinUpOffset[f] = it->second.off;
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
    objectSkinFrameCacheGuard();
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
    sObjectSkinFrameCache[ObjectSkinFrameCacheKey{ avatar, skin_hash }] =
        ObjectSkinFrameCacheVal{ buf, off, skin_entry };
}

// B.2: look up this frame's bindless palette entry for (avatar, skin_hash).
// Returns BINDLESS_INVALID_SLOT if not filled this frame (draw then falls back to UBO).
U32 objectSkinLookupEntry(const void* avatar, U64 skin_hash)
{
    if (!sInitialized)
    {
        return BINDLESS_INVALID_SLOT;
    }
    objectSkinFrameCacheGuard();
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

void destroyBufferVk(VkBuffer buffer, void* allocation)
{
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
}

bool submitOneShotVk(VkCommandBuffer cmd, VkBuffer staging_buffer, VmaAllocation staging_allocation)
{
    return submitOneShotVkFromPool(cmd, sCommandPool, staging_buffer, staging_allocation, 0);
}

bool submitOneShotVkFromPool(VkCommandBuffer cmd, VkCommandPool pool, VkBuffer staging_buffer,
                             VmaAllocation staging_allocation, U32 staging_bytes)
{
    tickOneShotFreeQueue();
    const U64 byte_cap = tTexWorkerThread ? (256ull << 20) : ~0ull;
    for (U32 i = 0; i < 20; ++i)
    {
        bool over = false;
        PendingOneShotFree wait_entry;
        bool have_entry = false;
        {
            std::lock_guard<std::mutex> lk(sOneShotMutex);
            over = sPendingOneShotFrees.size() > 256
                   || sOneShotStagingBytes.load() > byte_cap;
            if (over && !sPendingOneShotFrees.empty())
            {
                wait_entry = sPendingOneShotFrees.front();
                sPendingOneShotFrees.erase(sPendingOneShotFrees.begin());
                have_entry = true;
            }
        }
        if (!over)
        {
            break;
        }
        if (!have_entry)
        {
            break;
        }
        vkWaitForFences(sDevice, 1, &wait_entry.fence, VK_TRUE, 100000000ull);
        if (vkGetFenceStatus(sDevice, wait_entry.fence) == VK_SUCCESS)
        {
            if (wait_entry.buffer != VK_NULL_HANDLE || wait_entry.allocation != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(sAllocator, wait_entry.buffer, wait_entry.allocation);
            }
            sOneShotStagingBytes.fetch_sub(wait_entry.staging_bytes);
            std::lock_guard<std::mutex> lk(sOneShotMutex);
            sSubmitFencePool.push_back(wait_entry.fence);
            if (wait_entry.cmd != VK_NULL_HANDLE)
            {
                if (wait_entry.pool == sTexWorkerCommandPool && sTexWorkerCommandPool != VK_NULL_HANDLE)
                {
                    sRetiredTexOneShotCmds.push_back(wait_entry.cmd);
                }
                else
                {
                    sRetiredMainOneShotCmds.push_back(wait_entry.cmd);
                }
            }
        }
        else
        {
            std::lock_guard<std::mutex> lk(sOneShotMutex);
            sPendingOneShotFrees.insert(sPendingOneShotFrees.begin(), wait_entry);
        }
        tickOneShotFreeQueue();
    }

    VkFence fence = VK_NULL_HANDLE;
    {
        std::lock_guard<std::mutex> lk(sOneShotMutex);
        if (!sSubmitFencePool.empty())
        {
            fence = sSubmitFencePool.back();
            sSubmitFencePool.pop_back();
        }
    }
    if (fence != VK_NULL_HANDLE)
    {
        vkResetFences(sDevice, 1, &fence);
    }
    else
    {
        VkFenceCreateInfo fci = {};
        fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        if (vkCreateFence(sDevice, &fci, nullptr, &fence) != VK_SUCCESS)
        {
            fence = VK_NULL_HANDLE;
        }
    }

    if (fence == VK_NULL_HANDLE)
    {
        VkResult sr = peSubmitBlocking(cmd, VK_NULL_HANDLE, true);
        vkFreeCommandBuffers(sDevice, pool, 1, &cmd);
        if (staging_buffer != VK_NULL_HANDLE || staging_allocation != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        }
        return sr == VK_SUCCESS;
    }

    {
        PEJob job;
        job.cmd        = cmd;
        job.fence      = fence;
        job.is_oneshot = true;
        peEnqueue(std::move(job));
    }

    PendingOneShotFree pending;
    pending.fence         = fence;
    pending.cmd           = cmd;
    pending.pool          = pool;
    pending.buffer        = staging_buffer;
    pending.allocation    = staging_allocation;
    pending.staging_bytes = staging_bytes;
    sOneShotStagingBytes.fetch_add(staging_bytes);
    {
        std::lock_guard<std::mutex> lk(sOneShotMutex);
        sPendingOneShotFrees.push_back(pending);
    }
    return true;
}

void tickOneShotFreeQueue()
{
    if (sDevice == VK_NULL_HANDLE)
    {
        return;
    }
    std::vector<VkFence> failed;
    {
        std::lock_guard<std::mutex> lk(sPEFailedMutex);
        failed.swap(sPEFailedOneShotFences);
    }
    std::vector<VkCommandBuffer> free_now;
    const VkCommandPool free_pool = tTexWorkerThread ? sTexWorkerCommandPool : sCommandPool;
    {
        std::lock_guard<std::mutex> lk(sOneShotMutex);
        size_t w = 0;
        const size_t n = sPendingOneShotFrees.size();
        for (size_t r = 0; r < n; ++r)
        {
            PendingOneShotFree& e = sPendingOneShotFrees[r];
            const bool submit_failed = !failed.empty() &&
                std::find(failed.begin(), failed.end(), e.fence) != failed.end();
            if (sReapForceAll || submit_failed || vkGetFenceStatus(sDevice, e.fence) == VK_SUCCESS)
            {
                if (e.buffer != VK_NULL_HANDLE || e.allocation != VK_NULL_HANDLE)
                {
                    vmaDestroyBuffer(sAllocator, e.buffer, e.allocation);
                }
                sOneShotStagingBytes.fetch_sub(e.staging_bytes);
                sSubmitFencePool.push_back(e.fence);
                if (e.cmd != VK_NULL_HANDLE)
                {
                    if (e.pool == sTexWorkerCommandPool && sTexWorkerCommandPool != VK_NULL_HANDLE)
                    {
                        sRetiredTexOneShotCmds.push_back(e.cmd);
                    }
                    else
                    {
                        sRetiredMainOneShotCmds.push_back(e.cmd);
                    }
                }
            }
            else
            {
                if (w != r)
                {
                    sPendingOneShotFrees[w] = e;
                }
                ++w;
            }
        }
        sPendingOneShotFrees.resize(w);
        if (tTexWorkerThread)
        {
            free_now.swap(sRetiredTexOneShotCmds);
        }
        else
        {
            free_now.swap(sRetiredMainOneShotCmds);
        }
    }
    if (!free_now.empty() && free_pool != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(sDevice, free_pool, (U32)free_now.size(), free_now.data());
    }
}

bool texWorkerInit()
{
    if (sDevice == VK_NULL_HANDLE)
    {
        return false;
    }
    if (sTexWorkerCommandPool != VK_NULL_HANDLE)
    {
        return true;
    }
    VkCommandPoolCreateInfo ci = {};
    ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.queueFamilyIndex = sGraphicsQueueFamily;
    return vkCreateCommandPool(sDevice, &ci, nullptr, &sTexWorkerCommandPool) == VK_SUCCESS;
}

void texWorkerMarkThread()
{
    tTexWorkerThread = true;
}

void setVkTexWorkerStopHook(void (*fn)())
{
    sTexWorkerStopHook = fn;
}

void setVkGeoWorkerStopHook(void (*fn)())
{
    sGeoWorkerStopHook = fn;
}

void setVkBakeWorkerStopHook(void (*fn)())
{
    sBakeWorkerStopHook = fn;
}

void setVkDeviceLostHook(void (*fn)())
{
    sDeviceLostHook = fn;
}

void parWorkerForbiddenCheck()
{
    if (LLVKContract::isWorkerThread())
    {
        LLVKContract::cause(LLVKContract::C_PAR_WORKER_FORBIDDEN);
    }
}

void texWorkerShutdown()
{
    if (sTexWorkerCommandPool == VK_NULL_HANDLE)
    {
        return;
    }
    for (;;)
    {
        PendingOneShotFree e;
        bool have = false;
        {
            std::lock_guard<std::mutex> lk(sOneShotMutex);
            for (size_t i = 0; i < sPendingOneShotFrees.size(); ++i)
            {
                if (sPendingOneShotFrees[i].pool == sTexWorkerCommandPool)
                {
                    e = sPendingOneShotFrees[i];
                    sPendingOneShotFrees.erase(sPendingOneShotFrees.begin() + i);
                    have = true;
                    break;
                }
            }
        }
        if (!have)
        {
            break;
        }
        vkWaitForFences(sDevice, 1, &e.fence, VK_TRUE, 1000000000ull);
        if (e.buffer != VK_NULL_HANDLE || e.allocation != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(sAllocator, e.buffer, e.allocation);
        }
        sOneShotStagingBytes.fetch_sub(e.staging_bytes);
        {
            std::lock_guard<std::mutex> lk(sOneShotMutex);
            sSubmitFencePool.push_back(e.fence);
        }
    }
    {
        std::lock_guard<std::mutex> lk(sOneShotMutex);
        sRetiredTexOneShotCmds.clear();
    }
    vkDestroyCommandPool(sDevice, sTexWorkerCommandPool, nullptr);
    sTexWorkerCommandPool = VK_NULL_HANDLE;
}

bool uploadTextureOneShotVk(U32          width,
                            U32          height,
                            VkFormat     format,
                            const void*  data,
                            U32          data_size_bytes,
                            U32&         mip_count,
                            VkImage&     out_image,
                            VkImageView& out_view,
                            void*&       out_allocation)
{
    out_image      = VK_NULL_HANDLE;
    out_view       = VK_NULL_HANDLE;
    out_allocation = nullptr;
    if (!tTexWorkerThread || sTexWorkerCommandPool == VK_NULL_HANDLE)
    {
        return false;
    }
    if (data == nullptr || data_size_bytes == 0 || width == 0 || height == 0)
    {
        return false;
    }
    if (sDevice == VK_NULL_HANDLE || sAllocator == VK_NULL_HANDLE || sGraphicsQueue == VK_NULL_HANDLE)
    {
        return false;
    }

    U32 mips = (mip_count > 0) ? mip_count : 1;
    if (mips > 1)
    {
        VkFormatProperties fp = {};
        vkGetPhysicalDeviceFormatProperties(sPhysicalDevice, format, &fp);
        if (!(fp.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        {
            mips = 1;
        }
    }
    mip_count = mips;

    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (mips > 1)
    {
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    VkImage     image      = VK_NULL_HANDLE;
    VkImageView view       = VK_NULL_HANDLE;
    void*       allocation = nullptr;
    if (!createAttachmentImageVkImpl(width, height, format, usage,
                                     VK_IMAGE_ASPECT_COLOR_BIT,
                                     "texWorkerUpload",
                                     image, view, allocation, mips))
    {
        return false;
    }

    VkBuffer      staging_buffer     = VK_NULL_HANDLE;
    VmaAllocation staging_allocation = VK_NULL_HANDLE;
    void*         staging_mapped     = nullptr;
    {
        VkBufferCreateInfo bci = {};
        bci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bci.size        = data_size_bytes;
        bci.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo aci = {};
        aci.usage         = VMA_MEMORY_USAGE_AUTO;
        aci.flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                          | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        aci.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        VmaAllocationInfo info = {};
        VkResult r = vmaCreateBuffer(sAllocator, &bci, &aci, &staging_buffer,
                                     &staging_allocation, &info);
        if (r != VK_SUCCESS || staging_buffer == VK_NULL_HANDLE || info.pMappedData == nullptr)
        {
            if (staging_buffer != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
            }
            vkDestroyImageView(sDevice, view, nullptr);
            vmaDestroyImage(sAllocator, image, reinterpret_cast<VmaAllocation>(allocation));
            return false;
        }
        staging_mapped = info.pMappedData;
    }

    memcpy(staging_mapped, data, data_size_bytes);

    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool        = sTexWorkerCommandPool;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(sDevice, &cbai, &cmd) != VK_SUCCESS || cmd == VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        vkDestroyImageView(sDevice, view, nullptr);
        vmaDestroyImage(sAllocator, image, reinterpret_cast<VmaAllocation>(allocation));
        return false;
    }

    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &cbbi);

    auto mip_barrier = [&](U32 level, VkImageLayout oldL, VkImageLayout newL,
                           VkAccessFlags srcA, VkAccessFlags dstA,
                           VkPipelineStageFlags srcS, VkPipelineStageFlags dstS)
    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.image                           = image;
        b.oldLayout                       = oldL;
        b.newLayout                       = newL;
        b.srcAccessMask                   = srcA;
        b.dstAccessMask                   = dstA;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = level;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd, srcS, dstS, 0, 0, nullptr, 0, nullptr, 1, &b);
    };

    mip_barrier(0,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                0, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    {
        VkBufferImageCopy region = {};
        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;
        region.imageExtent                     = {width, height, 1};
        vkCmdCopyBufferToImage(cmd, staging_buffer, image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    if (mips > 1)
    {
        S32 mw = (S32)width;
        S32 mh = (S32)height;
        for (U32 i = 1; i < mips; ++i)
        {
            const S32 dw = (mw > 1) ? (mw / 2) : 1;
            const S32 dh = (mh > 1) ? (mh / 2) : 1;

            mip_barrier(i - 1,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
            mip_barrier(i,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        0, VK_ACCESS_TRANSFER_WRITE_BIT,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            VkImageBlit blit = {};
            blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel       = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount     = 1;
            blit.srcOffsets[0]                 = { 0, 0, 0 };
            blit.srcOffsets[1]                 = { mw, mh, 1 };
            blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel       = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount     = 1;
            blit.dstOffsets[0]                 = { 0, 0, 0 };
            blit.dstOffsets[1]                 = { dw, dh, 1 };
            vkCmdBlitImage(cmd,
                           image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &blit, VK_FILTER_LINEAR);

            mw = dw;
            mh = dh;
        }

        for (U32 i = 0; i < mips; ++i)
        {
            const bool is_last = (i == mips - 1);
            mip_barrier(i,
                        is_last ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        is_last ? VK_ACCESS_TRANSFER_WRITE_BIT : VK_ACCESS_TRANSFER_READ_BIT,
                        VK_ACCESS_SHADER_READ_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        }
    }
    else
    {
        mip_barrier(0,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    vkEndCommandBuffer(cmd);

    if (!submitOneShotVkFromPool(cmd, sTexWorkerCommandPool, staging_buffer, staging_allocation, data_size_bytes))
    {
        vkDestroyImageView(sDevice, view, nullptr);
        vmaDestroyImage(sAllocator, image, reinterpret_cast<VmaAllocation>(allocation));
        return false;
    }

    out_image      = image;
    out_view       = view;
    out_allocation = allocation;
    return true;
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

    std::vector<U32> sMegaTypeSizes;
    std::unordered_map<U32, std::vector<MegaChunk*>> sMegaVertexPools;
    std::vector<MegaChunk*> sMegaIndexPool;
    std::unordered_map<U64, MegaChunk*> sMegaChunksById;
    U64 sMegaChunkNextId = 1;

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
        const VkBufferUsageFlags usage = vertex_chunk ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                                                      : VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
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
}

void megabufInit(const U32* type_sizes, U32 type_count)
{
    sMegaTypeSizes.assign(type_sizes, type_sizes + type_count);
}

void gpuCheckpoint(const char* label)
{
    gpuCheckpointImpl(label);
}

void megabufShutdown()
{
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

bool allocDomainSelfTest();

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

void bindVertexBufferVk(VkCommandBuffer cmd_buf, VkBuffer buffer, VkDeviceSize offset, U32 firstBinding)
{
    if (cmd_buf == VK_NULL_HANDLE || buffer == VK_NULL_HANDLE)
    {
        return;
    }
    if (tVBMemoCmd != cmd_buf || tVBMemoEpoch != tCmdRecordEpoch)
    {
        tVBMemoCmd   = cmd_buf;
        tVBMemoEpoch = tCmdRecordEpoch;
        std::memset(tVBMemoBuf, 0, sizeof(tVBMemoBuf));
        std::memset(tVBMemoOff, 0, sizeof(tVBMemoOff));
    }
    if (firstBinding < 16
        && tVBMemoBuf[firstBinding] == buffer
        && tVBMemoOff[firstBinding] == offset)
    {
        ++gVkPerf.vb_skip;
        return;
    }
    if (firstBinding < 16)
    {
        tVBMemoBuf[firstBinding] = buffer;
        tVBMemoOff[firstBinding] = offset;
    }
    ++gVkPerf.vb_bind;
    VkBuffer     buffers[1] = { buffer };
    VkDeviceSize offsets[1] = { offset };
    vkCmdBindVertexBuffers(cmd_buf,
                           firstBinding,
                           1,
                           buffers,
                           offsets);
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

void bindIndexBufferVk(VkCommandBuffer cmd_buf,
                       VkBuffer        buffer,
                       VkDeviceSize    offset,
                       VkIndexType     index_type)
{
    if (cmd_buf == VK_NULL_HANDLE || buffer == VK_NULL_HANDLE)
    {
        return;
    }
    if (tIBMemoCmd != cmd_buf || tIBMemoEpoch != tCmdRecordEpoch)
    {
        tIBMemoCmd   = cmd_buf;
        tIBMemoEpoch = tCmdRecordEpoch;
        tIBMemoBuf   = VK_NULL_HANDLE;
        tIBMemoOff   = 0;
        tIBMemoType  = VK_INDEX_TYPE_MAX_ENUM;
    }
    if (tIBMemoBuf == buffer && tIBMemoOff == offset && tIBMemoType == index_type)
    {
        ++gVkPerf.ib_skip;
        return;
    }
    tIBMemoBuf  = buffer;
    tIBMemoOff  = offset;
    tIBMemoType = index_type;
    ++gVkPerf.ib_bind;
    vkCmdBindIndexBuffer(cmd_buf, buffer, offset, index_type);
}

bool createColorAttachmentImageVk(U32          width,
                                  U32          height,
                                  VkFormat     format,
                                  VkImage&     out_image,
                                  VkImageView& out_view,
                                  void*&       out_allocation,
                                  U32          mip_levels,
                                  VkImageView* out_sample_view)
{
    VkImageUsageFlags usage =
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        | VK_IMAGE_USAGE_SAMPLED_BIT
        | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
        | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (!createAttachmentImageVkImpl(width, height, format,
                                     usage,
                                     VK_IMAGE_ASPECT_COLOR_BIT,
                                     "createColorAttachmentImageVk",
                                     out_image, out_view, out_allocation,
                                     mip_levels))
    {
        return false;
    }

    if (mip_levels > 1)
    {
        if (out_sample_view == nullptr)
        {
            return false;
        }
        *out_sample_view = out_view;

        VkImageViewCreateInfo vci = {};
        vci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image                           = out_image;
        vci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        vci.format                          = format;
        vci.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        vci.subresourceRange.baseMipLevel   = 0;
        vci.subresourceRange.levelCount     = 1;
        vci.subresourceRange.baseArrayLayer = 0;
        vci.subresourceRange.layerCount     = 1;
        VkImageView attach_view = VK_NULL_HANDLE;
        if (vkCreateImageView(sDevice, &vci, nullptr, &attach_view) != VK_SUCCESS)
        {
            return false;
        }
        noteViewHandleCreated(attach_view);
        out_view = attach_view;
    }

    {
        VkCommandBufferAllocateInfo cba = {};
        cba.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cba.commandPool        = sCommandPool;
        cba.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cba.commandBufferCount = 1;
        VkCommandBuffer one_cmd = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(sDevice, &cba, &one_cmd);
        if (one_cmd != VK_NULL_HANDLE)
        {
            VkCommandBufferBeginInfo cbbi = {};
            cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(one_cmd, &cbbi);

            VkImageSubresourceRange full = {};
            full.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            full.baseMipLevel   = 0;
            full.levelCount     = (mip_levels > 0) ? mip_levels : 1;
            full.baseArrayLayer = 0;
            full.layerCount     = 1;

            VkImageMemoryBarrier to_dst = {};
            to_dst.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            to_dst.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
            to_dst.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            to_dst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            to_dst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            to_dst.image               = out_image;
            to_dst.subresourceRange    = full;
            to_dst.srcAccessMask       = 0;
            to_dst.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
            vkCmdPipelineBarrier(one_cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0, 0, nullptr, 0, nullptr, 1, &to_dst);

            VkClearColorValue black = {};
            vkCmdClearColorImage(one_cmd, out_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 &black, 1, &full);

            VkImageMemoryBarrier to_read = to_dst;
            to_read.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            to_read.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            to_read.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            to_read.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(one_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0, 0, nullptr, 0, nullptr, 1, &to_read);

            vkEndCommandBuffer(one_cmd);
            peSubmitBlocking(one_cmd, VK_NULL_HANDLE, true);
            vkFreeCommandBuffers(sDevice, sCommandPool, 1, &one_cmd);
        }
    }
    return true;
}

bool createDepthAttachmentImageVk(U32          width,
                                  U32          height,
                                  VkFormat     format,
                                  VkImage&     out_image,
                                  VkImageView& out_view,
                                  void*&       out_allocation)
{
    constexpr VkImageUsageFlags usage =
          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
        | VK_IMAGE_USAGE_SAMPLED_BIT
        | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    return createAttachmentImageVkImpl(width, height, format,
                                       usage,
                                       VK_IMAGE_ASPECT_DEPTH_BIT,
                                       "createDepthAttachmentImageVk",
                                       out_image, out_view, out_allocation);
}

void destroyImageVk(VkImage image, VkImageView view, void* allocation)
{
    if (image == VK_NULL_HANDLE && view == VK_NULL_HANDLE && allocation == nullptr)
    {
        return;
    }
    ++gVkViewDestroyGen;
    PendingImageFree pending;
    pending.image         = image;
    pending.view          = view;
    pending.allocation    = reinterpret_cast<VmaAllocation>(allocation);
    pending.enqueue_frame = sMonotonicFrameCount;
    sPendingImageFrees.push_back(pending);
    if (view != VK_NULL_HANDLE && vkValidationRequested())
    {
        std::lock_guard<std::mutex> lk(sSet1BirthMutex);
        ViewDeathInfo& di  = sViewDeathLedger[(U64)view];
        di.enqueue_frame   = sMonotonicFrameCount;
        di.enqueue_retaddr = (U64)(uintptr_t)__builtin_return_address(0);
    }
}

void tickDeferredImageFreeQueue()
{
    if (sDevice == VK_NULL_HANDLE && sAllocator == VK_NULL_HANDLE)
    {
        return;
    }
    size_t w = 0;
    const size_t n = sPendingImageFrees.size();
    for (size_t r = 0; r < n; ++r)
    {
        PendingImageFree& e = sPendingImageFrees[r];
        if (reapReady(e.enqueue_frame))
        {
            if (e.view != VK_NULL_HANDLE && sDevice != VK_NULL_HANDLE)
            {
                vkDestroyImageView(sDevice, e.view, nullptr);
                {
                    std::lock_guard<std::mutex> lk(sDeadHandleMutex);
                    sDeadViewHandles.insert((U64)e.view);
                }
                if (vkValidationRequested())
                {
                    std::lock_guard<std::mutex> lk(sSet1BirthMutex);
                    sViewDeathLedger[(U64)e.view].destroy_frame = sMonotonicFrameCount;
                }
            }
            if (e.image != VK_NULL_HANDLE && sAllocator != VK_NULL_HANDLE)
            {
                vmaDestroyImage(sAllocator, e.image, e.allocation);
            }
        }
        else
        {
            if (w != r)
            {
                sPendingImageFrees[w] = e;
            }
            ++w;
        }
    }
    sPendingImageFrees.resize(w);

    {
        std::lock_guard<std::mutex> guard(sBindlessSlotMutex);
        size_t sw = 0;
        const size_t sn = sPendingSlotFrees.size();
        for (size_t r = 0; r < sn; ++r)
        {
            PendingSlotFree& e = sPendingSlotFrees[r];
            if (reapReady(e.enqueue_frame))
            {
                bindlessWriteSlotInternal(e.slot, VK_NULL_HANDLE, VK_NULL_HANDLE);
                sBindlessSlotFreeList.push_back(e.slot);
            }
            else
            {
                if (sw != r)
                {
                    sPendingSlotFrees[sw] = e;
                }
                ++sw;
            }
        }
        sPendingSlotFrees.resize(sw);
    }

    drawDataReclaimDomain(&sMainDomain);
}

void destroyPipelineVk(VkPipeline pipeline)
{
    if (pipeline == VK_NULL_HANDLE || sDevice == VK_NULL_HANDLE)
    {
        return;
    }
    PendingObjectFree pending;
    pending.pipeline      = pipeline;
    pending.enqueue_frame = sMonotonicFrameCount;
    sPendingObjectFrees.push_back(pending);
}

void destroyShaderModuleVk(VkShaderModule shader_module)
{
    if (shader_module == VK_NULL_HANDLE || sDevice == VK_NULL_HANDLE)
    {
        return;
    }
    PendingObjectFree pending;
    pending.shader_module = shader_module;
    pending.enqueue_frame = sMonotonicFrameCount;
    sPendingObjectFrees.push_back(pending);
}

void destroyPipelineLayoutVk(VkPipelineLayout pipeline_layout)
{
    if (pipeline_layout == VK_NULL_HANDLE || sDevice == VK_NULL_HANDLE)
    {
        return;
    }
    PendingObjectFree pending;
    pending.pipeline_layout = pipeline_layout;
    pending.enqueue_frame   = sMonotonicFrameCount;
    sPendingObjectFrees.push_back(pending);
}

void destroyDescriptorSetLayoutVk(VkDescriptorSetLayout descriptor_set_layout)
{
    if (descriptor_set_layout == VK_NULL_HANDLE || sDevice == VK_NULL_HANDLE)
    {
        return;
    }
    PendingObjectFree pending;
    pending.descriptor_set_layout = descriptor_set_layout;
    pending.enqueue_frame         = sMonotonicFrameCount;
    sPendingObjectFrees.push_back(pending);
}

void tickDeferredObjectFreeQueue()
{
    if (sDevice == VK_NULL_HANDLE)
    {
        return;
    }
    size_t w = 0;
    const size_t n = sPendingObjectFrees.size();
    for (size_t r = 0; r < n; ++r)
    {
        PendingObjectFree& e = sPendingObjectFrees[r];
        if (reapReady(e.enqueue_frame))
        {
            if (e.pipeline != VK_NULL_HANDLE)
            {
                vkDestroyPipeline(sDevice, e.pipeline, nullptr);
            }
            if (e.shader_module != VK_NULL_HANDLE)
            {
                vkDestroyShaderModule(sDevice, e.shader_module, nullptr);
            }
            if (e.pipeline_layout != VK_NULL_HANDLE)
            {
                vkDestroyPipelineLayout(sDevice, e.pipeline_layout, nullptr);
            }
            if (e.descriptor_set_layout != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorSetLayout(sDevice, e.descriptor_set_layout, nullptr);
            }
        }
        else
        {
            if (w != r)
            {
                sPendingObjectFrees[w] = e;
            }
            ++w;
        }
    }
    sPendingObjectFrees.resize(w);
}

bool createTextureImageVk(U32          width,
                          U32          height,
                          VkFormat     format,
                          VkImage&     out_image,
                          VkImageView& out_view,
                          void*&       out_allocation,
                          U32          mip_levels)
{
    VkImageUsageFlags usage =
          VK_IMAGE_USAGE_TRANSFER_DST_BIT
        | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (mip_levels > 1)
    {
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    const bool created = createAttachmentImageVkImpl(width, height, format,
                                                     usage,
                                                     VK_IMAGE_ASPECT_COLOR_BIT,
                                                     "createTextureImageVk",
                                                     out_image, out_view, out_allocation,
                                                     mip_levels);
    return created;
}

bool uploadImageDataVk(VkImage     image,
                       U32         width,
                       U32         height,
                       const void* data,
                       U32         data_size_bytes,
                       U32         mip_level)
{
    if (image == VK_NULL_HANDLE || data == nullptr || data_size_bytes == 0)
    {
        return false;
    }
    if (sDevice == VK_NULL_HANDLE || sAllocator == VK_NULL_HANDLE ||
        sCommandPool == VK_NULL_HANDLE || sGraphicsQueue == VK_NULL_HANDLE)
    {
        return false;
    }

    VkBuffer      staging_buffer     = VK_NULL_HANDLE;
    VmaAllocation staging_allocation = VK_NULL_HANDLE;
    void*         staging_mapped     = nullptr;
    {
        VkBufferCreateInfo bci = {};
        bci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bci.size        = data_size_bytes;
        bci.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo aci = {};
        aci.usage         = VMA_MEMORY_USAGE_AUTO;
        aci.flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                          | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        aci.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        VmaAllocationInfo info = {};
        VkResult r = vmaCreateBuffer(sAllocator, &bci, &aci, &staging_buffer,
                                     &staging_allocation, &info);
        if (r != VK_SUCCESS || staging_buffer == VK_NULL_HANDLE || info.pMappedData == nullptr)
        {
            if (staging_buffer != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
            }
            return false;
        }
        staging_mapped = info.pMappedData;
    }

    memcpy(staging_mapped, data, data_size_bytes);

    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool        = sCommandPool;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkResult cr = vkAllocateCommandBuffers(sDevice, &cbai, &cmd);
    if (cr != VK_SUCCESS || cmd == VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        return false;
    }

    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &cbbi);

    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
        b.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        b.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        b.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = image;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = mip_level;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    {
        VkBufferImageCopy region = {};
        region.bufferOffset                    = 0;
        region.bufferRowLength                 = 0;
        region.bufferImageHeight               = 0;
        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = mip_level;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;
        region.imageOffset                     = {0, 0, 0};
        region.imageExtent                     = {width, height, 1};
        vkCmdCopyBufferToImage(cmd, staging_buffer, image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        b.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
        b.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = image;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = mip_level;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    vkEndCommandBuffer(cmd);

    return submitOneShotVk(cmd, staging_buffer, staging_allocation);
}

bool generateMipChainBlitVk(VkImage image, U32 base_w, U32 base_h, U32 mip_count, VkFormat format)
{
    if (image == VK_NULL_HANDLE || mip_count <= 1 || base_w == 0 || base_h == 0)
    {
        return false;
    }
    if (sDevice == VK_NULL_HANDLE || sCommandPool == VK_NULL_HANDLE || sGraphicsQueue == VK_NULL_HANDLE)
    {
        return false;
    }

    VkFormatProperties fp = {};
    vkGetPhysicalDeviceFormatProperties(sPhysicalDevice, format, &fp);
    if (!(fp.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        return false;
    }

    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool        = sCommandPool;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(sDevice, &cbai, &cmd) != VK_SUCCESS)
    {
        return false;
    }
    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &cbbi);

    auto mip_barrier = [&](U32 level, VkImageLayout oldL, VkImageLayout newL,
                           VkAccessFlags srcA, VkAccessFlags dstA,
                           VkPipelineStageFlags srcS, VkPipelineStageFlags dstS)
    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.image                           = image;
        b.oldLayout                       = oldL;
        b.newLayout                       = newL;
        b.srcAccessMask                   = srcA;
        b.dstAccessMask                   = dstA;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = level;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd, srcS, dstS, 0, 0, nullptr, 0, nullptr, 1, &b);
    };

    S32 mw = (S32)base_w;
    S32 mh = (S32)base_h;
    for (U32 i = 1; i < mip_count; ++i)
    {
        const S32 dw = (mw > 1) ? (mw / 2) : 1;
        const S32 dh = (mh > 1) ? (mh / 2) : 1;

        mip_barrier(i - 1,
                    (i == 1) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    (i == 1) ? VK_ACCESS_SHADER_READ_BIT : VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_ACCESS_TRANSFER_READ_BIT,
                    (i == 1) ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT);
        mip_barrier(i,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    0, VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkImageBlit blit = {};
        blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel       = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount     = 1;
        blit.srcOffsets[0]                 = { 0, 0, 0 };
        blit.srcOffsets[1]                 = { mw, mh, 1 };
        blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel       = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount     = 1;
        blit.dstOffsets[0]                 = { 0, 0, 0 };
        blit.dstOffsets[1]                 = { dw, dh, 1 };
        vkCmdBlitImage(cmd,
                       image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blit, VK_FILTER_LINEAR);

        mw = dw;
        mh = dh;
    }

    for (U32 i = 0; i < mip_count; ++i)
    {
        const bool is_last = (i == mip_count - 1);
        mip_barrier(i,
                    is_last ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    is_last ? VK_ACCESS_TRANSFER_WRITE_BIT : VK_ACCESS_TRANSFER_READ_BIT,
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    vkEndCommandBuffer(cmd);

    return submitOneShotVk(cmd, VK_NULL_HANDLE, VK_NULL_HANDLE);
}

bool downscaleImageVk(VkImage      src_image,
                      U32          src_mip,
                      U32          src_w,
                      U32          src_h,
                      U32          dst_w,
                      U32          dst_h,
                      VkFormat     format,
                      U32          dst_mip_levels,
                      VkImage&     out_image,
                      VkImageView& out_view,
                      void*&       out_allocation)
{
    out_image      = VK_NULL_HANDLE;
    out_view       = VK_NULL_HANDLE;
    out_allocation = nullptr;

    if (src_image == VK_NULL_HANDLE || src_w == 0 || src_h == 0 || dst_w == 0 || dst_h == 0
        || format == VK_FORMAT_UNDEFINED)
    {
        return false;
    }
    if (sDevice == VK_NULL_HANDLE || sCommandPool == VK_NULL_HANDLE || sGraphicsQueue == VK_NULL_HANDLE)
    {
        return false;
    }

    const U32 mip_levels = (dst_mip_levels > 0) ? dst_mip_levels : 1;

    VkImage     new_image = VK_NULL_HANDLE;
    VkImageView new_view  = VK_NULL_HANDLE;
    void*       new_alloc = nullptr;
    if (!createTextureImageVk(dst_w, dst_h, format, new_image, new_view, new_alloc, mip_levels))
    {
        return false;
    }

    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool        = sCommandPool;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(sDevice, &cbai, &cmd) != VK_SUCCESS)
    {
        destroyImageVk(new_image, new_view, new_alloc);
        return false;
    }
    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &cbbi);

    auto image_barrier = [&](VkImage img, U32 level, VkImageLayout oldL, VkImageLayout newL,
                             VkAccessFlags srcA, VkAccessFlags dstA,
                             VkPipelineStageFlags srcS, VkPipelineStageFlags dstS)
    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.image                           = img;
        b.oldLayout                       = oldL;
        b.newLayout                       = newL;
        b.srcAccessMask                   = srcA;
        b.dstAccessMask                   = dstA;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = level;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd, srcS, dstS, 0, 0, nullptr, 0, nullptr, 1, &b);
    };

    image_barrier(src_image, src_mip,
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                  VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    image_barrier(new_image, 0,
                  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  0, VK_ACCESS_TRANSFER_WRITE_BIT,
                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkImageBlit blit = {};
    blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel       = src_mip;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount     = 1;
    blit.srcOffsets[0]                 = { 0, 0, 0 };
    blit.srcOffsets[1]                 = { (S32)src_w, (S32)src_h, 1 };
    blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel       = 0;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount     = 1;
    blit.dstOffsets[0]                 = { 0, 0, 0 };
    blit.dstOffsets[1]                 = { (S32)dst_w, (S32)dst_h, 1 };
    vkCmdBlitImage(cmd,
                   src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   new_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &blit, VK_FILTER_LINEAR);

    image_barrier(new_image, 0,
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                  VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    image_barrier(src_image, src_mip,
                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                  VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    vkEndCommandBuffer(cmd);

    if (!submitOneShotVk(cmd, VK_NULL_HANDLE, VK_NULL_HANDLE))
    {
        destroyImageVk(new_image, new_view, new_alloc);
        return false;
    }

    if (mip_levels > 1)
    {
        generateMipChainBlitVk(new_image, dst_w, dst_h, mip_levels, format);
    }

    out_image      = new_image;
    out_view       = new_view;
    out_allocation = new_alloc;
    return true;
}

bool blitCubeArrayVk(VkImage       src,
                     VkImageLayout src_layout,
                     U32           src_res,
                     VkImage       dst,
                     VkImageLayout dst_layout,
                     U32           dst_res,
                     U32           layer_count)
{
    if (src == VK_NULL_HANDLE || dst == VK_NULL_HANDLE
        || src_res == 0 || dst_res == 0 || layer_count == 0)
    {
        return false;
    }
    if (sDevice == VK_NULL_HANDLE || sCommandPool == VK_NULL_HANDLE || sGraphicsQueue == VK_NULL_HANDLE)
    {
        return false;
    }

    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool        = sCommandPool;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(sDevice, &cbai, &cmd) != VK_SUCCESS)
    {
        return false;
    }
    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &cbbi);

    auto image_barrier = [&](VkImage img, VkImageLayout oldL, VkImageLayout newL,
                             VkAccessFlags srcA, VkAccessFlags dstA,
                             VkPipelineStageFlags srcS, VkPipelineStageFlags dstS)
    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.image                           = img;
        b.oldLayout                       = oldL;
        b.newLayout                       = newL;
        b.srcAccessMask                   = srcA;
        b.dstAccessMask                   = dstA;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = layer_count;
        vkCmdPipelineBarrier(cmd, srcS, dstS, 0, 0, nullptr, 0, nullptr, 1, &b);
    };

    image_barrier(src, src_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                  VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    image_barrier(dst, dst_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkImageBlit blit = {};
    blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel       = 0;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount     = layer_count;
    blit.srcOffsets[0]                 = { 0, 0, 0 };
    blit.srcOffsets[1]                 = { (S32)src_res, (S32)src_res, 1 };
    blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel       = 0;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount     = layer_count;
    blit.dstOffsets[0]                 = { 0, 0, 0 };
    blit.dstOffsets[1]                 = { (S32)dst_res, (S32)dst_res, 1 };
    vkCmdBlitImage(cmd,
                   src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &blit, VK_FILTER_LINEAR);

    image_barrier(src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, src_layout,
                  VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    image_barrier(dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_layout,
                  VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    vkEndCommandBuffer(cmd);

    return submitOneShotVk(cmd, VK_NULL_HANDLE, VK_NULL_HANDLE);
}

bool generateMipChainInFrameVk(VkImage        image,
                               U32            base_w,
                               U32            base_h,
                               U32            mip_count,
                               VkFormat       format,
                               VkImageLayout  mip0_src_layout)
{
    if (image == VK_NULL_HANDLE || mip_count <= 1 || base_w == 0 || base_h == 0)
    {
        return false;
    }
    VkCommandBuffer cmd = getCurrentCommandBuffer();
    if (cmd == VK_NULL_HANDLE)
    {
        return false;
    }

    VkFormatProperties fp = {};
    vkGetPhysicalDeviceFormatProperties(sPhysicalDevice, format, &fp);
    if (!(fp.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        return false;
    }

    auto mip_barrier = [&](U32 level, VkImageLayout oldL, VkImageLayout newL,
                           VkAccessFlags srcA, VkAccessFlags dstA,
                           VkPipelineStageFlags srcS, VkPipelineStageFlags dstS)
    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.image                           = image;
        b.oldLayout                       = oldL;
        b.newLayout                       = newL;
        b.srcAccessMask                   = srcA;
        b.dstAccessMask                   = dstA;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = level;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd, srcS, dstS, 0, 0, nullptr, 0, nullptr, 1, &b);
    };

    VkAccessFlags        m0_srcA = 0;
    VkPipelineStageFlags m0_srcS = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    if (mip0_src_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        m0_srcA = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        m0_srcS = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else if (mip0_src_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        m0_srcA = VK_ACCESS_SHADER_READ_BIT;
        m0_srcS = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    mip_barrier(0, mip0_src_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                m0_srcA, VK_ACCESS_TRANSFER_READ_BIT,
                m0_srcS, VK_PIPELINE_STAGE_TRANSFER_BIT);

    S32 mw = (S32)base_w;
    S32 mh = (S32)base_h;
    for (U32 i = 1; i < mip_count; ++i)
    {
        const S32 dw = (mw > 1) ? (mw / 2) : 1;
        const S32 dh = (mh > 1) ? (mh / 2) : 1;

        mip_barrier(i,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    0, VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkImageBlit blit = {};
        blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel       = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount     = 1;
        blit.srcOffsets[0]                 = { 0, 0, 0 };
        blit.srcOffsets[1]                 = { mw, mh, 1 };
        blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel       = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount     = 1;
        blit.dstOffsets[0]                 = { 0, 0, 0 };
        blit.dstOffsets[1]                 = { dw, dh, 1 };
        vkCmdBlitImage(cmd,
                       image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blit, VK_FILTER_LINEAR);

        mip_barrier(i,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        mw = dw;
        mh = dh;
    }

    for (U32 i = 0; i < mip_count; ++i)
    {
        mip_barrier(i,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    return true;
}

bool createTexture3DImageVk(U32          width,
                            U32          height,
                            U32          depth,
                            VkFormat     format,
                            VkImage&     out_image,
                            VkImageView& out_view,
                            void*&       out_allocation)
{
    out_image      = VK_NULL_HANDLE;
    out_view       = VK_NULL_HANDLE;
    out_allocation = nullptr;

    if (width == 0 || height == 0 || depth == 0 || format == VK_FORMAT_UNDEFINED)
    {
        return false;
    }
    if (sAllocator == VK_NULL_HANDLE || sDevice == VK_NULL_HANDLE)
    {
        return false;
    }

    VkImageCreateInfo ici = {};
    ici.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType     = VK_IMAGE_TYPE_3D;
    ici.format        = format;
    ici.extent.width  = width;
    ici.extent.height = height;
    ici.extent.depth  = depth;
    ici.mipLevels     = 1;
    ici.arrayLayers   = 1;
    ici.samples       = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ici.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ici.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo aci = {};
    aci.usage         = VMA_MEMORY_USAGE_AUTO;
    aci.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VkImage       image      = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkResult r = vmaCreateImage(sAllocator, &ici, &aci, &image, &allocation, nullptr);
    if (r != VK_SUCCESS)
    {
        return false;
    }

    VkImageViewCreateInfo vci = {};
    vci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vci.image                           = image;
    vci.viewType                        = VK_IMAGE_VIEW_TYPE_3D;
    vci.format                          = format;
    vci.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    vci.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    vci.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    vci.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    vci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    vci.subresourceRange.baseMipLevel   = 0;
    vci.subresourceRange.levelCount     = 1;
    vci.subresourceRange.baseArrayLayer = 0;
    vci.subresourceRange.layerCount     = 1;

    VkImageView view = VK_NULL_HANDLE;
    r = vkCreateImageView(sDevice, &vci, nullptr, &view);
    if (r != VK_SUCCESS)
    {
        vmaDestroyImage(sAllocator, image, allocation);
        return false;
    }
    noteViewHandleCreated(view);

    out_image      = image;
    out_view       = view;
    out_allocation = reinterpret_cast<void*>(allocation);
    return true;
}

bool uploadImageData3DVk(VkImage     image,
                         U32         width,
                         U32         height,
                         U32         depth,
                         const void* data,
                         U32         data_size_bytes)
{
    if (image == VK_NULL_HANDLE || data == nullptr || data_size_bytes == 0 ||
        width == 0 || height == 0 || depth == 0)
    {
        return false;
    }
    if (sDevice == VK_NULL_HANDLE || sAllocator == VK_NULL_HANDLE ||
        sCommandPool == VK_NULL_HANDLE || sGraphicsQueue == VK_NULL_HANDLE)
    {
        return false;
    }

    VkBuffer      staging_buffer     = VK_NULL_HANDLE;
    VmaAllocation staging_allocation = VK_NULL_HANDLE;
    void*         staging_mapped     = nullptr;
    {
        VkBufferCreateInfo bci = {};
        bci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bci.size        = data_size_bytes;
        bci.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo aci = {};
        aci.usage         = VMA_MEMORY_USAGE_AUTO;
        aci.flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                          | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        aci.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        VmaAllocationInfo info = {};
        VkResult r = vmaCreateBuffer(sAllocator, &bci, &aci, &staging_buffer,
                                     &staging_allocation, &info);
        if (r != VK_SUCCESS || staging_buffer == VK_NULL_HANDLE || info.pMappedData == nullptr)
        {
            if (staging_buffer != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
            }
            return false;
        }
        staging_mapped = info.pMappedData;
    }

    memcpy(staging_mapped, data, data_size_bytes);

    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool        = sCommandPool;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkResult cr = vkAllocateCommandBuffers(sDevice, &cbai, &cmd);
    if (cr != VK_SUCCESS || cmd == VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        return false;
    }

    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &cbbi);

    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
        b.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        b.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        b.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = image;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    {
        VkBufferImageCopy region = {};
        region.bufferOffset                    = 0;
        region.bufferRowLength                 = 0;
        region.bufferImageHeight               = 0;
        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;
        region.imageOffset                     = {0, 0, 0};
        region.imageExtent                     = {width, height, depth};
        vkCmdCopyBufferToImage(cmd, staging_buffer, image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        b.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
        b.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = image;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    vkEndCommandBuffer(cmd);

    return submitOneShotVk(cmd, staging_buffer, staging_allocation);
}

bool uploadImageSubregionVk(VkImage     image,
                            VkFormat    format,
                            U32         x_pos,
                            U32         y_pos,
                            U32         sub_width,
                            U32         sub_height,
                            const void* data,
                            U32         data_width,
                            U32         data_height)
{
    if (image == VK_NULL_HANDLE || data == nullptr || sub_width == 0 || sub_height == 0)
    {
        return false;
    }
    if (x_pos + sub_width > data_width || y_pos + sub_height > data_height)
    {
        return false;
    }
    if (sDevice == VK_NULL_HANDLE || sAllocator == VK_NULL_HANDLE ||
        sCommandPool == VK_NULL_HANDLE || sGraphicsQueue == VK_NULL_HANDLE)
    {
        return false;
    }

    const U32 bytes_per_pixel = vkFormatBytesPerPixelImpl(format);
    if (bytes_per_pixel == 0)
    {
        return false;
    }
    const U32 sub_row_bytes  = sub_width * bytes_per_pixel;
    const U32 data_row_bytes = data_width * bytes_per_pixel;
    const U32 staging_size   = sub_row_bytes * sub_height;

    VkBuffer      staging_buffer     = VK_NULL_HANDLE;
    VmaAllocation staging_allocation = VK_NULL_HANDLE;
    void*         staging_mapped     = nullptr;
    {
        VkBufferCreateInfo bci = {};
        bci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bci.size        = staging_size;
        bci.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo aci = {};
        aci.usage         = VMA_MEMORY_USAGE_AUTO;
        aci.flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                          | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        aci.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        VmaAllocationInfo info = {};
        VkResult r = vmaCreateBuffer(sAllocator, &bci, &aci, &staging_buffer,
                                     &staging_allocation, &info);
        if (r != VK_SUCCESS || staging_buffer == VK_NULL_HANDLE || info.pMappedData == nullptr)
        {
            if (staging_buffer != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
            }
            return false;
        }
        staging_mapped = info.pMappedData;
    }

    {
        const U8* src_base = (const U8*)data + (y_pos * data_width + x_pos) * bytes_per_pixel;
        U8*       dst      = (U8*)staging_mapped;
        for (U32 row = 0; row < sub_height; ++row)
        {
            memcpy(dst + row * sub_row_bytes,
                   src_base + row * data_row_bytes,
                   sub_row_bytes);
        }
    }

    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool        = sCommandPool;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkResult cr = vkAllocateCommandBuffers(sDevice, &cbai, &cmd);
    if (cr != VK_SUCCESS || cmd == VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        return false;
    }

    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &cbbi);

    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
        b.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        b.oldLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = image;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    {
        VkBufferImageCopy region = {};
        region.bufferOffset                    = 0;
        region.bufferRowLength                 = 0;
        region.bufferImageHeight               = 0;
        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;
        region.imageOffset                     = {(S32)x_pos, (S32)y_pos, 0};
        region.imageExtent                     = {sub_width, sub_height, 1};
        vkCmdCopyBufferToImage(cmd, staging_buffer, image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        b.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
        b.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = image;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    vkEndCommandBuffer(cmd);

    return submitOneShotVk(cmd, staging_buffer, staging_allocation);
}

bool createCubeImageVk(U32          resolution,
                       VkFormat     format,
                       U32          mip_count,
                       VkImage&     out_image,
                       VkImageView& out_view,
                       void*&       out_allocation)
{
    out_image      = VK_NULL_HANDLE;
    out_view       = VK_NULL_HANDLE;
    out_allocation = nullptr;

    if (resolution == 0 || format == VK_FORMAT_UNDEFINED)
    {
        return false;
    }
    if (sAllocator == VK_NULL_HANDLE || sDevice == VK_NULL_HANDLE)
    {
        return false;
    }

    const U32 safe_mip_count = (mip_count == 0) ? 1 : mip_count;

    VkImageCreateInfo ici = {};
    ici.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.flags         = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    ici.imageType     = VK_IMAGE_TYPE_2D;
    ici.format        = format;
    ici.extent.width  = resolution;
    ici.extent.height = resolution;
    ici.extent.depth  = 1;
    ici.mipLevels     = safe_mip_count;
    ici.arrayLayers   = 6;
    ici.samples       = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ici.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                      | ((safe_mip_count > 1) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0);
    ici.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo aci = {};
    aci.usage         = VMA_MEMORY_USAGE_AUTO;
    aci.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VkImage       image      = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkResult r = vmaCreateImage(sAllocator, &ici, &aci, &image, &allocation, nullptr);
    if (r != VK_SUCCESS)
    {
        return false;
    }

    VkImageViewCreateInfo vci = {};
    vci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vci.image                           = image;
    vci.viewType                        = VK_IMAGE_VIEW_TYPE_CUBE;
    vci.format                          = format;
    vci.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    vci.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    vci.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    vci.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    vci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    vci.subresourceRange.baseMipLevel   = 0;
    vci.subresourceRange.levelCount     = safe_mip_count;
    vci.subresourceRange.baseArrayLayer = 0;
    vci.subresourceRange.layerCount     = 6;

    VkImageView view = VK_NULL_HANDLE;
    r = vkCreateImageView(sDevice, &vci, nullptr, &view);
    if (r != VK_SUCCESS)
    {
        vmaDestroyImage(sAllocator, image, allocation);
        return false;
    }
    noteViewHandleCreated(view);

    out_image      = image;
    out_view       = view;
    out_allocation = reinterpret_cast<void*>(allocation);
    return true;
}

bool uploadCubeImageDataVk(VkImage           image,
                           U32               resolution,
                           const void* const face_data[6],
                           U32               face_size_bytes)
{
    if (image == VK_NULL_HANDLE || face_data == nullptr || face_size_bytes == 0)
    {
        return false;
    }
    for (U32 f = 0; f < 6; ++f)
    {
        if (face_data[f] == nullptr)
        {
            return false;
        }
    }
    if (sDevice == VK_NULL_HANDLE || sAllocator == VK_NULL_HANDLE ||
        sCommandPool == VK_NULL_HANDLE || sGraphicsQueue == VK_NULL_HANDLE)
    {
        return false;
    }

    const VkDeviceSize total_size = (VkDeviceSize)face_size_bytes * 6;

    VkBuffer      staging_buffer     = VK_NULL_HANDLE;
    VmaAllocation staging_allocation = VK_NULL_HANDLE;
    void*         staging_mapped     = nullptr;
    {
        VkBufferCreateInfo bci = {};
        bci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bci.size        = total_size;
        bci.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo aci = {};
        aci.usage         = VMA_MEMORY_USAGE_AUTO;
        aci.flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                          | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        aci.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        VmaAllocationInfo info = {};
        VkResult r = vmaCreateBuffer(sAllocator, &bci, &aci, &staging_buffer,
                                     &staging_allocation, &info);
        if (r != VK_SUCCESS || staging_buffer == VK_NULL_HANDLE || info.pMappedData == nullptr)
        {
            if (staging_buffer != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
            }
            return false;
        }
        staging_mapped = info.pMappedData;
    }

    {
        U8* dst = (U8*)staging_mapped;
        for (U32 f = 0; f < 6; ++f)
        {
            memcpy(dst + (size_t)f * face_size_bytes, face_data[f], face_size_bytes);
        }
    }

    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool        = sCommandPool;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkResult cr = vkAllocateCommandBuffers(sDevice, &cbai, &cmd);
    if (cr != VK_SUCCESS || cmd == VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        return false;
    }

    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &cbbi);

    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
        b.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        b.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        b.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = image;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 6;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    {
        VkBufferImageCopy regions[6] = {};
        for (U32 f = 0; f < 6; ++f)
        {
            regions[f].bufferOffset                    = (VkDeviceSize)f * face_size_bytes;
            regions[f].bufferRowLength                 = 0;
            regions[f].bufferImageHeight               = 0;
            regions[f].imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            regions[f].imageSubresource.mipLevel       = 0;
            regions[f].imageSubresource.baseArrayLayer = f;
            regions[f].imageSubresource.layerCount     = 1;
            regions[f].imageOffset                     = {0, 0, 0};
            regions[f].imageExtent                     = {resolution, resolution, 1};
        }
        vkCmdCopyBufferToImage(cmd, staging_buffer, image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, regions);
    }

    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        b.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
        b.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = image;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 6;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    vkEndCommandBuffer(cmd);

    return submitOneShotVk(cmd, staging_buffer, staging_allocation);
}

bool createCubeArrayImageVk(U32          resolution,
                            U32          count,
                            U32          mips,
                            VkFormat     format,
                            VkImage&     out_image,
                            VkImageView& out_view,
                            void*&       out_allocation)
{
    out_image      = VK_NULL_HANDLE;
    out_view       = VK_NULL_HANDLE;
    out_allocation = nullptr;

    if (resolution == 0 || count == 0 || mips == 0 || format == VK_FORMAT_UNDEFINED)
    {
        return false;
    }
    if (sAllocator == VK_NULL_HANDLE || sDevice == VK_NULL_HANDLE ||
        sCommandPool == VK_NULL_HANDLE || sGraphicsQueue == VK_NULL_HANDLE)
    {
        return false;
    }

    const U32 layer_count = count * 6;

    VkImageCreateInfo ici = {};
    ici.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.flags         = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    ici.imageType     = VK_IMAGE_TYPE_2D;
    ici.format        = format;
    ici.extent.width  = resolution;
    ici.extent.height = resolution;
    ici.extent.depth  = 1;
    ici.mipLevels     = mips;
    ici.arrayLayers   = layer_count;
    ici.samples       = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ici.usage         = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ici.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo aci = {};
    aci.usage         = VMA_MEMORY_USAGE_AUTO;
    aci.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VkImage       image      = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkResult r = vmaCreateImage(sAllocator, &ici, &aci, &image, &allocation, nullptr);
    if (r != VK_SUCCESS)
    {
        return false;
    }

    VkImageViewCreateInfo vci = {};
    vci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vci.image                           = image;
    vci.viewType                        = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    vci.format                          = format;
    vci.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    vci.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    vci.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    vci.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    vci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    vci.subresourceRange.baseMipLevel   = 0;
    vci.subresourceRange.levelCount     = mips;
    vci.subresourceRange.baseArrayLayer = 0;
    vci.subresourceRange.layerCount     = layer_count;

    VkImageView view = VK_NULL_HANDLE;
    r = vkCreateImageView(sDevice, &vci, nullptr, &view);
    if (r != VK_SUCCESS)
    {
        vmaDestroyImage(sAllocator, image, allocation);
        return false;
    }
    noteViewHandleCreated(view);

    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool        = sCommandPool;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(sDevice, &cbai, &cmd) == VK_SUCCESS && cmd != VK_NULL_HANDLE)
    {
        VkCommandBufferBeginInfo cbbi = {};
        cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &cbbi);

        VkImageSubresourceRange full_range = {};
        full_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        full_range.baseMipLevel   = 0;
        full_range.levelCount     = mips;
        full_range.baseArrayLayer = 0;
        full_range.layerCount     = layer_count;

        VkImageMemoryBarrier to_clear = {};
        to_clear.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        to_clear.srcAccessMask        = 0;
        to_clear.dstAccessMask        = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_clear.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
        to_clear.newLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_clear.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
        to_clear.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
        to_clear.image                = image;
        to_clear.subresourceRange     = full_range;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &to_clear);

        VkClearColorValue black = {};
        vkCmdClearColorImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             &black, 1, &full_range);

        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        b.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
        b.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = image;
        b.subresourceRange                = full_range;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
        vkEndCommandBuffer(cmd);
        peSubmitBlocking(cmd, VK_NULL_HANDLE, true);
        vkFreeCommandBuffers(sDevice, sCommandPool, 1, &cmd);
    }

    out_image      = image;
    out_view       = view;
    out_allocation = reinterpret_cast<void*>(allocation);

    if (vkSetDebugUtilsObjectNameEXT != nullptr)
    {
        char namebuf[160];
        snprintf(namebuf, sizeof(namebuf), "cubeArray res%u count%u mips%u fmt%d",
                 resolution, count, mips, (int)format);
        setVkObjectName((U64)image, VK_OBJECT_TYPE_IMAGE, namebuf);
        setVkObjectName((U64)view, VK_OBJECT_TYPE_IMAGE_VIEW, namebuf);
    }
    return true;
}

bool copyColorImageToCubeArrayLayerVk(VkImage       src_image,
                                      VkImageLayout src_layout,
                                      VkImage       dst_cube_array,
                                      U32           dst_layer,
                                      U32           dst_mip,
                                      U32           width,
                                      U32           height,
                                      U32           src_y)
{
    if (src_image == VK_NULL_HANDLE || dst_cube_array == VK_NULL_HANDLE ||
        width == 0 || height == 0)
    {
        return false;
    }
    if (sDevice == VK_NULL_HANDLE || sGraphicsQueue == VK_NULL_HANDLE)
    {
        return false;
    }

    VkCommandBuffer cmd = getCurrentCommandBuffer();
    if (cmd == VK_NULL_HANDLE)
    {
        return false;
    }

    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        b.dstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
        b.oldLayout                       = src_layout;
        b.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = src_image;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }
    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
        b.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        b.oldLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = dst_cube_array;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = dst_mip;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = dst_layer;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    {
        VkImageBlit blit = {};
        blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel       = 0;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount     = 1;
        blit.srcOffsets[0]                 = { 0, (S32)src_y, 0 };
        blit.srcOffsets[1]                 = { (S32)width, (S32)(src_y + height), 1 };
        blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel       = dst_mip;
        blit.dstSubresource.baseArrayLayer = dst_layer;
        blit.dstSubresource.layerCount     = 1;
        blit.dstOffsets[0]                 = { 0, (S32)height, 0 };
        blit.dstOffsets[1]                 = { (S32)width, 0, 1 };
        vkCmdBlitImage(cmd, src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       dst_cube_array, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);
    }

    {
        VkImageMemoryBarrier bb[2] = {};
        bb[0].sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        bb[0].srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        bb[0].dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
        bb[0].oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        bb[0].newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        bb[0].srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        bb[0].dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        bb[0].image                           = dst_cube_array;
        bb[0].subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        bb[0].subresourceRange.baseMipLevel   = dst_mip;
        bb[0].subresourceRange.levelCount     = 1;
        bb[0].subresourceRange.baseArrayLayer = dst_layer;
        bb[0].subresourceRange.layerCount     = 1;
        bb[1].sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        bb[1].srcAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
        bb[1].dstAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
        bb[1].oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        bb[1].newLayout                       = src_layout;
        bb[1].srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        bb[1].dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        bb[1].image                           = src_image;
        bb[1].subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        bb[1].subresourceRange.baseMipLevel   = 0;
        bb[1].subresourceRange.levelCount     = 1;
        bb[1].subresourceRange.baseArrayLayer = 0;
        bb[1].subresourceRange.layerCount     = 1;
        const U32 bcount = (src_layout == VK_IMAGE_LAYOUT_UNDEFINED) ? 1u : 2u;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             0, 0, nullptr, 0, nullptr, bcount, bb);
    }

    return true;
}

bool copyColorImageRegionToImage2DVk(VkImage       src_image,
                                     VkImageLayout src_layout,
                                     S32           src_x,
                                     S32           src_y,
                                     VkImage       dst_image,
                                     VkImageLayout dst_current_layout,
                                     S32           dst_x,
                                     S32           dst_y,
                                     U32           width,
                                     U32           height)
{
    if (src_image == VK_NULL_HANDLE || dst_image == VK_NULL_HANDLE ||
        width == 0 || height == 0 ||
        src_x < 0 || src_y < 0 || dst_x < 0 || dst_y < 0)
    {
        return false;
    }
    if (sDevice == VK_NULL_HANDLE || sGraphicsQueue == VK_NULL_HANDLE)
    {
        return false;
    }

    VkCommandBuffer cmd = getCurrentCommandBuffer();
    if (cmd == VK_NULL_HANDLE)
    {
        return false;
    }

    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        b.dstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
        b.oldLayout                       = src_layout;
        b.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = src_image;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }
    {
        const bool dst_undefined = (dst_current_layout == VK_IMAGE_LAYOUT_UNDEFINED);
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = dst_undefined ? 0 : VK_ACCESS_SHADER_READ_BIT;
        b.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        b.oldLayout                       = dst_current_layout;
        b.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = dst_image;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd,
                             dst_undefined ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    {
        VkImageCopy region = {};
        region.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.mipLevel       = 0;
        region.srcSubresource.baseArrayLayer = 0;
        region.srcSubresource.layerCount     = 1;
        region.srcOffset                     = { src_x, src_y, 0 };
        region.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.mipLevel       = 0;
        region.dstSubresource.baseArrayLayer = 0;
        region.dstSubresource.layerCount     = 1;
        region.dstOffset                     = { dst_x, dst_y, 0 };
        region.extent                        = { width, height, 1 };
        vkCmdCopyImage(cmd, src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    {
        VkImageMemoryBarrier bb[2] = {};
        bb[0].sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        bb[0].srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        bb[0].dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
        bb[0].oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        bb[0].newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        bb[0].srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        bb[0].dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        bb[0].image                           = dst_image;
        bb[0].subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        bb[0].subresourceRange.baseMipLevel   = 0;
        bb[0].subresourceRange.levelCount     = 1;
        bb[0].subresourceRange.baseArrayLayer = 0;
        bb[0].subresourceRange.layerCount     = 1;
        bb[1].sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        bb[1].srcAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
        bb[1].dstAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
        bb[1].oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        bb[1].newLayout                       = src_layout;
        bb[1].srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        bb[1].dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        bb[1].image                           = src_image;
        bb[1].subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        bb[1].subresourceRange.baseMipLevel   = 0;
        bb[1].subresourceRange.levelCount     = 1;
        bb[1].subresourceRange.baseArrayLayer = 0;
        bb[1].subresourceRange.layerCount     = 1;
        const U32 bcount = (src_layout == VK_IMAGE_LAYOUT_UNDEFINED) ? 1u : 2u;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             0, 0, nullptr, 0, nullptr, bcount, bb);
    }

    return true;
}

bool createReadbackBufferVk(U32       bytes,
                            VkBuffer& out_buffer,
                            void*&    out_allocation,
                            void*&    out_mapped)
{
    out_buffer     = VK_NULL_HANDLE;
    out_allocation = nullptr;
    out_mapped     = nullptr;
    if (bytes == 0 || sAllocator == VK_NULL_HANDLE)
    {
        return false;
    }

    VkBufferCreateInfo bci = {};
    bci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size        = bytes;
    bci.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo aci = {};
    aci.usage         = VMA_MEMORY_USAGE_AUTO;
    aci.flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
                      | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    aci.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VkBuffer      buffer     = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo info = {};
    VkResult r = vmaCreateBuffer(sAllocator, &bci, &aci, &buffer, &allocation, &info);
    if (r != VK_SUCCESS || buffer == VK_NULL_HANDLE || info.pMappedData == nullptr)
    {
        if (buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(sAllocator, buffer, allocation);
        }
        return false;
    }
    out_buffer     = buffer;
    out_allocation = allocation;
    out_mapped     = info.pMappedData;
    return true;
}

bool copyColorImageRegionToBufferVk(VkImage       src_image,
                                    VkImageLayout src_layout,
                                    S32           src_x,
                                    S32           src_y,
                                    U32           width,
                                    U32           height,
                                    VkBuffer      dst_buffer)
{
    if (src_image == VK_NULL_HANDLE || dst_buffer == VK_NULL_HANDLE ||
        width == 0 || height == 0 || src_x < 0 || src_y < 0)
    {
        return false;
    }
    if (src_layout == VK_IMAGE_LAYOUT_UNDEFINED)
    {
        return false;
    }
    if (sDevice == VK_NULL_HANDLE || sGraphicsQueue == VK_NULL_HANDLE)
    {
        return false;
    }

    VkCommandBuffer cmd = getCurrentCommandBuffer();
    if (cmd == VK_NULL_HANDLE)
    {
        return false;
    }

    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        b.dstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
        b.oldLayout                       = src_layout;
        b.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = src_image;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    {
        VkBufferImageCopy region = {};
        region.bufferOffset                    = 0;
        region.bufferRowLength                 = 0;
        region.bufferImageHeight               = 0;
        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;
        region.imageOffset                     = {src_x, src_y, 0};
        region.imageExtent                     = {width, height, 1};
        vkCmdCopyImageToBuffer(cmd, src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               dst_buffer, 1, &region);
    }

    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
        b.dstAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
        b.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        b.newLayout                       = src_layout;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = src_image;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    return true;
}

bool readbackColorImageRegionVk(VkImage       image,
                                VkImageLayout current_layout,
                                S32           x,
                                S32           y,
                                U32           width,
                                U32           height,
                                U32           bytes_per_pixel,
                                void*         out_pixels)
{
    if (image == VK_NULL_HANDLE || out_pixels == nullptr ||
        width == 0 || height == 0 || bytes_per_pixel == 0 ||
        x < 0 || y < 0)
    {
        return false;
    }
    if (current_layout == VK_IMAGE_LAYOUT_UNDEFINED)
    {
        return false;
    }
    if (sDevice == VK_NULL_HANDLE || sAllocator == VK_NULL_HANDLE ||
        sCommandPool == VK_NULL_HANDLE || sGraphicsQueue == VK_NULL_HANDLE)
    {
        return false;
    }

    const VkDeviceSize read_size = (VkDeviceSize)width * (VkDeviceSize)height * (VkDeviceSize)bytes_per_pixel;

    VkBuffer      staging_buffer     = VK_NULL_HANDLE;
    VmaAllocation staging_allocation = VK_NULL_HANDLE;
    void*         staging_mapped     = nullptr;
    {
        VkBufferCreateInfo bci = {};
        bci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bci.size        = read_size;
        bci.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo aci = {};
        aci.usage         = VMA_MEMORY_USAGE_AUTO;
        aci.flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
                          | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        aci.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        VmaAllocationInfo info = {};
        VkResult r = vmaCreateBuffer(sAllocator, &bci, &aci, &staging_buffer,
                                     &staging_allocation, &info);
        if (r != VK_SUCCESS || staging_buffer == VK_NULL_HANDLE || info.pMappedData == nullptr)
        {
            if (staging_buffer != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
            }
            return false;
        }
        staging_mapped = info.pMappedData;
    }

    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool        = sCommandPool;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkResult cr = vkAllocateCommandBuffers(sDevice, &cbai, &cmd);
    if (cr != VK_SUCCESS || cmd == VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        return false;
    }

    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &cbbi);

    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_MEMORY_WRITE_BIT;
        b.dstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
        b.oldLayout                       = current_layout;
        b.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = image;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    {
        VkBufferImageCopy region = {};
        region.bufferOffset                    = 0;
        region.bufferRowLength                 = 0;
        region.bufferImageHeight               = 0;
        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;
        region.imageOffset                     = {x, y, 0};
        region.imageExtent                     = {width, height, 1};
        vkCmdCopyImageToBuffer(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               staging_buffer, 1, &region);
    }

    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
        b.dstAccessMask                   = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        b.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        b.newLayout                       = current_layout;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = image;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    vkEndCommandBuffer(cmd);

    VkResult sr = peSubmitBlocking(cmd, VK_NULL_HANDLE, true);
    if (sr != VK_SUCCESS)
    {
        vkFreeCommandBuffers(sDevice, sCommandPool, 1, &cmd);
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        return false;
    }
    vkFreeCommandBuffers(sDevice, sCommandPool, 1, &cmd);

    memcpy(out_pixels, staging_mapped, (size_t)read_size);
    vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
    return true;
}

bool readbackDepthImageRegionVk(VkImage       image,
                                VkImageLayout current_layout,
                                S32           x,
                                S32           y,
                                U32           width,
                                U32           height,
                                VkFormat      format,
                                F32*          out_depth)
{
    if (image == VK_NULL_HANDLE || out_depth == nullptr ||
        width == 0 || height == 0 || x < 0 || y < 0)
    {
        return false;
    }
    if (current_layout == VK_IMAGE_LAYOUT_UNDEFINED)
    {
        return false;
    }
    if (format != VK_FORMAT_D24_UNORM_S8_UINT &&
        format != VK_FORMAT_X8_D24_UNORM_PACK32 &&
        format != VK_FORMAT_D32_SFLOAT &&
        format != VK_FORMAT_D32_SFLOAT_S8_UINT)
    {
        return false;
    }
    if (sDevice == VK_NULL_HANDLE || sAllocator == VK_NULL_HANDLE ||
        sCommandPool == VK_NULL_HANDLE || sGraphicsQueue == VK_NULL_HANDLE)
    {
        return false;
    }

    const VkDeviceSize read_size = (VkDeviceSize)width * (VkDeviceSize)height * 4;

    VkBuffer      staging_buffer     = VK_NULL_HANDLE;
    VmaAllocation staging_allocation = VK_NULL_HANDLE;
    void*         staging_mapped     = nullptr;
    {
        VkBufferCreateInfo bci = {};
        bci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bci.size        = read_size;
        bci.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo aci = {};
        aci.usage         = VMA_MEMORY_USAGE_AUTO;
        aci.flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
                          | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        aci.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        VmaAllocationInfo info = {};
        VkResult r = vmaCreateBuffer(sAllocator, &bci, &aci, &staging_buffer,
                                     &staging_allocation, &info);
        if (r != VK_SUCCESS || staging_buffer == VK_NULL_HANDLE || info.pMappedData == nullptr)
        {
            if (staging_buffer != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
            }
            return false;
        }
        staging_mapped = info.pMappedData;
    }

    const bool has_stencil = (format == VK_FORMAT_D24_UNORM_S8_UINT ||
                              format == VK_FORMAT_D32_SFLOAT_S8_UINT);
    const VkImageAspectFlags barrier_aspect =
        has_stencil ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)
                    : VK_IMAGE_ASPECT_DEPTH_BIT;

    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool        = sCommandPool;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkResult cr = vkAllocateCommandBuffers(sDevice, &cbai, &cmd);
    if (cr != VK_SUCCESS || cmd == VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        return false;
    }

    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &cbbi);

    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_MEMORY_WRITE_BIT;
        b.dstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
        b.oldLayout                       = current_layout;
        b.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = image;
        b.subresourceRange.aspectMask     = barrier_aspect;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    {
        VkBufferImageCopy region = {};
        region.bufferOffset                    = 0;
        region.bufferRowLength                 = 0;
        region.bufferImageHeight               = 0;
        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;
        region.imageOffset                     = {x, y, 0};
        region.imageExtent                     = {width, height, 1};
        vkCmdCopyImageToBuffer(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               staging_buffer, 1, &region);
    }

    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
        b.dstAccessMask                   = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        b.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        b.newLayout                       = current_layout;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = image;
        b.subresourceRange.aspectMask     = barrier_aspect;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    vkEndCommandBuffer(cmd);

    VkResult sr = peSubmitBlocking(cmd, VK_NULL_HANDLE, true);
    if (sr != VK_SUCCESS)
    {
        vkFreeCommandBuffers(sDevice, sCommandPool, 1, &cmd);
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        return false;
    }
    vkFreeCommandBuffers(sDevice, sCommandPool, 1, &cmd);

    const size_t texel_count = (size_t)width * (size_t)height;
    if (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT)
    {
        memcpy(out_depth, staging_mapped, texel_count * sizeof(F32));
    }
    else
    {
        const U32* src = reinterpret_cast<const U32*>(staging_mapped);
        for (size_t i = 0; i < texel_count; ++i)
        {
            out_depth[i] = (F32)(src[i] & 0x00FFFFFFu) / 16777215.0f;
        }
    }
    vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
    return true;
}

VkSampler getStandardLinearSampler()
{
    return sStandardLinearSampler;
}

VkSampler getSamplerForState(U32 address_mode, U32 filter_option, bool has_mipmaps, bool compare)
{
    if (sDevice == VK_NULL_HANDLE)
    {
        return VK_NULL_HANDLE;
    }
    return getSamplerForStateImpl(address_mode, filter_option, has_mipmaps, compare);
}

bool isProvokingVertexLastEnabled()
{
    return sProvokingVertexLastEnabled;
}

bool isGeometryShaderEnabledVk()
{
    return sGeometryShaderEnabled;
}

static U64 phaseNowUs()
{
    return (U64)std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

VkPerfPhaseScope::VkPerfPhaseScope(U32 idx)
: mT0(phaseNowUs()), mIdx(idx)
{
}

VkPerfPhaseScope::~VkPerfPhaseScope()
{
    if (mIdx < 16)
    {
        gVkPerf.phase_us[mIdx] += phaseNowUs() - mT0;
    }
}

VkPerfIdleScope::VkPerfIdleScope(U32 idx)
: mT0(phaseNowUs()), mIdx(idx)
{
}

VkPerfIdleScope::~VkPerfIdleScope()
{
    if (mIdx < 32)
    {
        gVkPerf.idle_us[mIdx] += phaseNowUs() - mT0;
    }
}

VkPerfImgScope::VkPerfImgScope(U32 idx)
: mT0(phaseNowUs()), mIdx(idx)
{
}

VkPerfImgScope::~VkPerfImgScope()
{
    if (mIdx < 12)
    {
        gVkPerf.img_us[mIdx] += phaseNowUs() - mT0;
    }
}

VkPerfMainScope::VkPerfMainScope(U32 idx)
: mT0(phaseNowUs()), mIdx(idx)
{
}

VkPerfMainScope::~VkPerfMainScope()
{
    if (mIdx < 16)
    {
        gVkPerf.mlp_us[mIdx] += phaseNowUs() - mT0;
    }
}

bool perfLogEnabled()
{
    static const bool s_enabled = (getenv("AYASTORM_PERF_LOG") != nullptr);
    return s_enabled;
}

bool isIndirectDrawEnabled()
{
    return sMultiDrawIndirectEnabled && sDrawIndirectFirstInstanceEnabled;
}

bool indirectRingAlloc(U32 count, VkBuffer& out_buffer, VkDeviceSize& out_offset, void*& out_mapped)
{
    if (count == 0 || count > INDIRECT_RING_COMMANDS_PER_FRAME)
    {
        ++gVkPerf.mdi_full;
        return false;
    }
    if (sIndirectRingBuffer == VK_NULL_HANDLE)
    {
        if (sDevice == VK_NULL_HANDLE)
        {
            return false;
        }
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
            return false;
        }
        sIndirectRingMapped = reinterpret_cast<U8*>(mapped);
    }
    if (sIndirectRingFrame != sMonotonicFrameCount)
    {
        sIndirectRingFrame  = sMonotonicFrameCount;
        sIndirectRingCursor = 0;
    }
    if (sIndirectRingCursor + count > INDIRECT_RING_COMMANDS_PER_FRAME)
    {
        ++gVkPerf.mdi_full;
        return false;
    }
    const U32 region = (sFrameIndex < FRAMES_IN_FLIGHT) ? sFrameIndex : 0;
    const U64 base   = ((U64)region * INDIRECT_RING_COMMANDS_PER_FRAME + sIndirectRingCursor)
                     * sizeof(VkDrawIndexedIndirectCommand);
    sIndirectRingCursor += count;
    out_buffer = sIndirectRingBuffer;
    out_offset = (VkDeviceSize)base;
    out_mapped = sIndirectRingMapped + base;
    return true;
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

void bindlessUpdateSlot(U32 slot, VkImageView view, VkSampler sampler)
{
    if (!sBindlessActive || slot == 0 || slot == BINDLESS_INVALID_SLOT || slot >= sBindlessHeapCount)
    {
        return;
    }
    std::lock_guard<std::mutex> guard(sBindlessSlotMutex);
    bindlessWriteSlotInternal(slot, view, sampler);
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
    std::memcpy(sDrawDataMapped + (size_t)slot * 4, slots4, 16);
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
            U32 dd[4] = { sd->mId, i, 0xA5A5A5A5u, i * 7u + 1u };
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
static thread_local U32 tDrawDataScratchMemoVals[4] = {};

U32 drawDataWriteScratch(const U32* slots4)
{
    if (sDrawDataMapped == nullptr)
    {
        return 0;
    }
    if (tDrawDataScratchMemoFrame == sMonotonicFrameCount
        && std::memcmp(tDrawDataScratchMemoVals, slots4, 16) == 0)
    {
        return tDrawDataScratchMemoSlot;
    }
    VkcRaceProbe probe(sVkcScratchOwner, LLVKContract::C_DRAWDATA_RACE);
    if (sDrawDataScratchFrame != sMonotonicFrameCount)
    {
        sDrawDataScratchFrame  = sMonotonicFrameCount;
        sDrawDataScratchCursor = 0;
    }
    if (sDrawDataScratchCursor >= DRAWDATA_SCRATCH_PER_FRAME)
    {
        LLVKContract::cause(LLVKContract::C_DRAWDATA_SCRATCH_WRAP);
        static bool warned = false;
        if (!warned)
        {
            LL_WARNS("Vulkan") << "VKBindless: DrawData scratch wrapped" << LL_ENDL;
            warned = true;
        }
        sDrawDataScratchCursor = 0;
    }
    const U32 region = (sFrameIndex < FRAMES_IN_FLIGHT) ? sFrameIndex : 0;
    const U32 slot = DRAWDATA_PERSISTENT_SLOTS + region * DRAWDATA_SCRATCH_PER_FRAME + sDrawDataScratchCursor++;
    std::memcpy(sDrawDataMapped + (size_t)slot * 4, slots4, 16);
    tDrawDataScratchMemoFrame = sMonotonicFrameCount;
    tDrawDataScratchMemoSlot  = slot;
    std::memcpy(tDrawDataScratchMemoVals, slots4, 16);
    return slot;
}

void setCurrentDrawDataID(U32 id)
{
    tCurrentDrawDataID = (id == BINDLESS_INVALID_SLOT) ? 0 : id;
}

U32 getCurrentDrawDataID()
{
    return tCurrentDrawDataID;
}

void transitionImageLayoutVk(VkImage              image,
                             VkImageAspectFlags   aspect_mask,
                             VkImageLayout        old_layout,
                             VkImageLayout        new_layout,
                             VkPipelineStageFlags src_stage_mask,
                             VkPipelineStageFlags dst_stage_mask,
                             VkAccessFlags        src_access_mask,
                             VkAccessFlags        dst_access_mask)
{
    if (!sInitialized || image == VK_NULL_HANDLE)
    {
        return;
    }

    VkCommandBuffer rec_cmd = currentRecordCmd();
    if (rec_cmd == VK_NULL_HANDLE)
    {
        if (sDevice == VK_NULL_HANDLE || sCommandPool == VK_NULL_HANDLE ||
            sGraphicsQueue == VK_NULL_HANDLE)
        {
            return;
        }
        VkCommandBufferAllocateInfo cbai = {};
        cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cbai.commandPool        = sCommandPool;
        cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbai.commandBufferCount = 1;
        VkCommandBuffer oneshot_cmd = VK_NULL_HANDLE;
        if (vkAllocateCommandBuffers(sDevice, &cbai, &oneshot_cmd) != VK_SUCCESS ||
            oneshot_cmd == VK_NULL_HANDLE)
        {
            return;
        }
        VkCommandBufferBeginInfo cbbi = {};
        cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(oneshot_cmd, &cbbi);
        VkImageMemoryBarrier oneshot_barrier = {};
        oneshot_barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        oneshot_barrier.oldLayout                       = old_layout;
        oneshot_barrier.newLayout                       = new_layout;
        oneshot_barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        oneshot_barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        oneshot_barrier.image                           = image;
        oneshot_barrier.subresourceRange.aspectMask     = aspect_mask;
        oneshot_barrier.subresourceRange.baseMipLevel   = 0;
        oneshot_barrier.subresourceRange.levelCount     = 1;
        oneshot_barrier.subresourceRange.baseArrayLayer = 0;
        oneshot_barrier.subresourceRange.layerCount     = 1;
        oneshot_barrier.srcAccessMask                   = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        oneshot_barrier.dstAccessMask                   = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        vkCmdPipelineBarrier(oneshot_cmd,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &oneshot_barrier);
        vkEndCommandBuffer(oneshot_cmd);
        submitOneShotVk(oneshot_cmd, VK_NULL_HANDLE, VK_NULL_HANDLE);
        return;
    }


    VkImageMemoryBarrier barrier = {};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = old_layout;
    barrier.newLayout                       = new_layout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image;
    barrier.subresourceRange.aspectMask     = aspect_mask;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.srcAccessMask                   = src_access_mask;
    barrier.dstAccessMask                   = dst_access_mask;

    const bool was_rendering = sInDynamicRendering;
    VkPipelineStageFlags rec_src_stage = src_stage_mask;
    VkPipelineStageFlags rec_dst_stage = dst_stage_mask;
    VkMemoryBarrier      attachment_sync = {};
    U32                  mem_barrier_count = 0;
    if (was_rendering)
    {
        vkCmdEndRendering(rec_cmd);
        sInDynamicRendering = false;

        const VkPipelineStageFlags attachment_stages =
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        rec_src_stage |= attachment_stages;
        rec_dst_stage |= attachment_stages;

        attachment_sync.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        attachment_sync.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        attachment_sync.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                                        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        mem_barrier_count = 1;
    }

    vkCmdPipelineBarrier(rec_cmd,
                         rec_src_stage, rec_dst_stage,
                         0,
                         mem_barrier_count, mem_barrier_count ? &attachment_sync : nullptr,
                         0, nullptr,
                         1, &barrier);

    if (was_rendering && (sSavedColorCount > 0 || sSavedHasDepth))
    {
        VkRenderingAttachmentInfo color_resume[4] = {};
        for (U32 i = 0; i < sSavedColorCount && i < 4; ++i)
        {
            color_resume[i]        = sSavedColorInfos[i];
            color_resume[i].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        }
        VkRenderingAttachmentInfo depth_resume = sSavedDepthInfo;
        depth_resume.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        VkRenderingInfo resume_info       = {};
        resume_info.sType                 = VK_STRUCTURE_TYPE_RENDERING_INFO;
        resume_info.renderArea.offset     = { 0, 0 };
        resume_info.renderArea.extent     = { sSavedRenderWidth, sSavedRenderHeight };
        resume_info.layerCount            = 1;
        resume_info.viewMask              = 0;
        resume_info.colorAttachmentCount  = sSavedColorCount;
        resume_info.pColorAttachments     = (sSavedColorCount > 0 ? color_resume : nullptr);
        resume_info.pDepthAttachment      = (sSavedHasDepth ? &depth_resume : nullptr);
        resume_info.pStencilAttachment    = nullptr;

        vkCmdBeginRendering(rec_cmd, &resume_info);
        sInDynamicRendering = true;
    }
}

bool initSurface(LLWindow* window)
{
    if (!sInitialized)
    {
        return false;
    }
    if (sSurface != VK_NULL_HANDLE)
    {
        return true;
    }
    if (!window)
    {
        return false;
    }

    LLWindow::LLNativeWindowHandles handles = window->getNativeWindowHandles();

    VkResult result = VK_ERROR_INITIALIZATION_FAILED;
#if defined(VK_USE_PLATFORM_XLIB_KHR)
    if (handles.native_display && handles.native_window)
    {
        VkXlibSurfaceCreateInfoKHR ci = {};
        ci.sType  = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
        ci.dpy    = static_cast<Display*>(handles.native_display);
        ci.window = static_cast<Window>(reinterpret_cast<uintptr_t>(handles.native_window));
        result = vkCreateXlibSurfaceKHR(sInstance, &ci, nullptr, &sSurface);
    }
    else
    {
        return false;
    }
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    if (handles.native_window)
    {
        VkWin32SurfaceCreateInfoKHR ci = {};
        ci.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        ci.hinstance = static_cast<HINSTANCE>(handles.native_display);
        ci.hwnd      = static_cast<HWND>(handles.native_window);
        result = vkCreateWin32SurfaceKHR(sInstance, &ci, nullptr, &sSurface);
    }
    else
    {
        return false;
    }
#elif defined(VK_USE_PLATFORM_METAL_EXT)
    if (handles.native_window)
    {
        VkMetalSurfaceCreateInfoEXT ci = {};
        ci.sType  = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        ci.pLayer = static_cast<const CAMetalLayer*>(handles.native_window);
        if (vkCreateMetalSurfaceEXT == nullptr)
        {
            return false;
        }
        result = vkCreateMetalSurfaceEXT(sInstance, &ci, nullptr, &sSurface);
    }
    else
    {
        return false;
    }
#else
    return false;
#endif

    if (result != VK_SUCCESS || sSurface == VK_NULL_HANDLE)
    {
        sSurface = VK_NULL_HANDLE;
        return false;
    }

    return true;
}

void shutdownSurface()
{
    if (sSurface != VK_NULL_HANDLE && sInstance != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(sInstance, sSurface, nullptr);
        sSurface = VK_NULL_HANDLE;
    }
}

VkSurfaceKHR getSurface()
{
    return sSurface;
}

bool initSwapchain()
{
    if (!sInitialized)
    {
        return false;
    }
    if (sSurface == VK_NULL_HANDLE)
    {
        return false;
    }
    if (sSwapchain != VK_NULL_HANDLE)
    {
        return true;
    }
    return createSwapchain();
}


bool isVulkanPresentationEnabled()
{
    return sVulkanPresentationEnabled;
}

void setVulkanPresentationEnabled(bool enabled)
{
    sVulkanPresentationEnabled = enabled;
}

U32 getRenderBackendMode()
{
    static const U32 s_mode = []() -> U32 {
        U32 mode = 1;
        if (gSavedSettings.controlExists("RenderBackend"))
        {
            mode = gSavedSettings.getU32("RenderBackend");
        }
        return mode;
    }();
    return s_mode;
}

bool shouldUseVulkanRender()
{
    return isVulkanInitialized() && getRenderBackendMode() != 0;
}

void resetVulkanRenderSuspend()
{
}

VkFormat getSwapchainFormat()
{
    return sSwapchainFormat;
}

bool hasSwapchainDepth()
{
    return sSwapchainDepthView != VK_NULL_HANDLE;
}

bool isInRenderPassScope()
{
    return sInDynamicRendering;
}

U64 currentPassAttachmentSig()
{
    U64 sig = sInDynamicRendering ? 0x9E3779B97F4A7C15ull : 0;
    if (sInDynamicRendering)
    {
        if (sSavedHasDepth)
        {
            sig = sig * 0x100000001B3ull ^ (U64)(uintptr_t)sSavedDepthInfo.imageView;
        }
        for (U32 i = 0; i < sSavedColorCount && i < 4; ++i)
        {
            sig = sig * 0x100000001B3ull ^ (U64)(uintptr_t)sSavedColorInfos[i].imageView;
        }
    }
    return sig;
}

bool isImageViewActivePassAttachment(VkImageView view)
{
    if (!sInDynamicRendering || view == VK_NULL_HANDLE)
    {
        return false;
    }
    if (sSavedHasDepth && sSavedDepthInfo.imageView == view)
    {
        return true;
    }
    for (U32 i = 0; i < sSavedColorCount && i < 4; ++i)
    {
        if (sSavedColorInfos[i].imageView == view)
        {
            return true;
        }
    }
    return false;
}

void setRenderViewport(S32 x, S32 y, S32 w, S32 h)
{
    sVkRenderViewport[0] = x;
    sVkRenderViewport[1] = y;
    sVkRenderViewport[2] = w;
    sVkRenderViewport[3] = h;
}

F32 getMaxLineWidth()
{
    return sMaxLineWidth;
}

bool getDeviceCapsVk(DeviceCapsVk& out)
{
    if (sPhysicalDevice == VK_NULL_HANDLE)
    {
        return false;
    }

    const VkPhysicalDeviceProperties& props = sPhysicalDeviceProperties;

    out.device_name                         = props.deviceName;
    out.driver_name                         = sDriverName;
    out.driver_info                         = sDriverInfo;
    out.vendor_id                           = props.vendorID;
    out.api_version_major                   = VK_API_VERSION_MAJOR(props.apiVersion);
    out.api_version_minor                   = VK_API_VERSION_MINOR(props.apiVersion);
    out.device_local_memory_mb              = sDeviceLocalMemoryMB;
    out.max_image_dimension_2d              = props.limits.maxImageDimension2D;
    out.max_uniform_buffer_range            = props.limits.maxUniformBufferRange;
    out.max_per_stage_sampled_images        = props.limits.maxPerStageDescriptorSampledImages;
    out.max_vertex_output_components        = props.limits.maxVertexOutputComponents;
    out.max_sample_mask_words               = props.limits.maxSampleMaskWords;
    out.framebuffer_color_sample_counts     = (U32)props.limits.framebufferColorSampleCounts;
    out.framebuffer_depth_sample_counts     = (U32)props.limits.framebufferDepthSampleCounts;
    out.sampled_image_integer_sample_counts = (U32)props.limits.sampledImageIntegerSampleCounts;
    out.max_sampler_anisotropy              = sMaxSamplerAnisotropy;
    out.sampler_anisotropy_enabled          = sSamplerAnisotropyEnabled;
    out.image_cube_array_enabled            = sImageCubeArrayEnabled;

    return true;
}

static void memoSyncCmd(VkCommandBuffer cmd)
{
    if (cmd == tMemoCmd && tCmdRecordEpoch == tMemoEpoch)
    {
        return;
    }
    tMemoCmd                   = cmd;
    tMemoEpoch                 = tCmdRecordEpoch;
    sLastBoundGraphicsPipeline = VK_NULL_HANDLE;
    sLastDescLayout            = VK_NULL_HANDLE;
    sLastMvLayout              = VK_NULL_HANDLE;
    sViewportScissorValid      = false;
}

void setupViewportAndScissor(VkCommandBuffer cmd, bool screen_space_copy)
{
    memoSyncCmd(cmd);
    const S32 fb_height = (S32)sCurrentRenderAreaHeight;

    VkViewport viewport = {};
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    if (screen_space_copy)
    {
        viewport.x      = (float)sVkRenderViewport[0];
        viewport.y      = (float)sVkRenderViewport[1];
        viewport.width  = (float)sVkRenderViewport[2];
        viewport.height = (float)sVkRenderViewport[3];
    }
    else
    {
        viewport.x      = (float)sVkRenderViewport[0];
        viewport.y      = (float)(fb_height - sVkRenderViewport[1]);
        viewport.width  = (float)sVkRenderViewport[2];
        viewport.height = -(float)sVkRenderViewport[3];
    }

    S32 sc_x, sc_y, sc_w, sc_h;
    if (sScissorEnabled)
    {
        sc_x = sScissorRectGL[0];
        sc_y = sScissorRectGL[1];
        sc_w = sScissorRectGL[2];
        sc_h = sScissorRectGL[3];
    }
    else
    {
        sc_x = sVkRenderViewport[0];
        sc_y = sVkRenderViewport[1];
        sc_w = sVkRenderViewport[2];
        sc_h = sVkRenderViewport[3];
    }

    VkRect2D scissor = {};
    scissor.offset.x      = llmax(sc_x, 0);
    scissor.offset.y      = llmax(screen_space_copy ? sc_y : fb_height - (sc_y + sc_h), 0);
    scissor.extent.width  = (U32)llmax(sc_w, 0);
    scissor.extent.height = (U32)llmax(sc_h, 0);

    if (vkCmdMemoEnabled()
        && sViewportScissorValid
        && std::memcmp(&viewport, &sLastViewport, sizeof(viewport)) == 0
        && std::memcmp(&scissor, &sLastScissor, sizeof(scissor)) == 0)
    {
        ++gVkPerf.vp_skip;
        return;
    }
    ++gVkPerf.vp_set;
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    sLastViewport         = viewport;
    sLastScissor          = scissor;
    sViewportScissorValid = true;
}

void bindGraphicsPipelineOnce(VkCommandBuffer cmd, VkPipeline pipeline)
{
    memoSyncCmd(cmd);
    if (vkCmdMemoEnabled() && pipeline == sLastBoundGraphicsPipeline)
    {
        ++gVkPerf.pipe_skip;
        return;
    }
    ++gVkPerf.pipe_bind;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    sLastBoundGraphicsPipeline = pipeline;
}

void bindDrawDescriptorSetsOnce(VkCommandBuffer cmd, VkPipelineLayout layout,
                                VkDescriptorSet set0, VkDescriptorSet set1,
                                U32 dyn_count, const U32* offsets)
{
    memoSyncCmd(cmd);
    if (vkValidationRequested())
    {
        LLGLSLShader* sh = LLGLSLShader::sCurBoundShaderPtr;
        std::lock_guard<std::mutex> lk(sSet1BirthMutex);
        if (set1 != VK_NULL_HANDLE)
        {
            Set1BirthInfo& bi = sSet1BirthLedger[(U64)set1];
            bi.path   = gVkPerfSetPath;
            bi.gen    = gVkPerDrawTopologyGen.load(std::memory_order_relaxed);
            bi.frame  = sMonotonicFrameCount;
            bi.shader = (sh != nullptr) ? sh->mName : std::string();
        }
        if (set0 != VK_NULL_HANDLE)
        {
            Set1BirthInfo& bi = sSet1BirthLedger[(U64)set0];
            bi.path   = 99;
            bi.gen    = gVkPerDrawTopologyGen.load(std::memory_order_relaxed);
            bi.frame  = sMonotonicFrameCount;
            bi.shader = (sh != nullptr) ? sh->mName : std::string();
        }
    }
    const U32 pass_bucket = perfPassBucket();
    ++gVkPerf.draws_pass[pass_bucket];
    if (pass_bucket == 1u)
    {
        ++gVkPerf.draws_shadow_map[gVkPerfShadowMapIndex < 6u ? gVkPerfShadowMapIndex : 5u];
    }
    VkDescriptorSet set2 = VK_NULL_HANDLE;
    {
        LLGLSLShader* sh = LLGLSLShader::sCurBoundShaderPtr;
        if (sh != nullptr && sh->mVkUsesBindlessHeap)
        {
            set2 = sBindlessHeapSet;
        }
    }
    LLGLSLShader::vkVerifyPerCallBindingsAtBind(offsets, dyn_count);

    U32        eff_local[LLGLSLShader::MAX_VK_DYNAMIC_BINDINGS];
    const U32* eff_offsets = offsets;
    U32        eff_count   = dyn_count;
    if (set2 != VK_NULL_HANDLE && dyn_count < LLGLSLShader::MAX_VK_DYNAMIC_BINDINGS)
    {
        if (dyn_count > 0)
        {
            std::memcpy(eff_local, offsets, dyn_count * sizeof(U32));
        }
        eff_local[dyn_count] = skinBaseDynamicOffsetBytes();
        eff_offsets          = eff_local;
        eff_count            = dyn_count + 1;
    }

    if (vkCmdMemoEnabled()
        && layout == sLastDescLayout
        && set0 == sLastDescSet0
        && set1 == sLastDescSet1
        && set2 == sLastDescSet2
        && eff_count == sLastDescDynCount
        && (eff_count == 0 || std::memcmp(eff_offsets, sLastDescOffsets, eff_count * sizeof(U32)) == 0))
    {
        ++gVkPerf.desc_skip;
        return;
    }
    ++gVkPerf.desc_bind;
    VkDescriptorSet set3 = (set2 != VK_NULL_HANDLE) ? sSkinBaseSet : VK_NULL_HANDLE;
    VkDescriptorSet sets[4] = { set0, set1, set2, set3 };
    vkCmdBindDescriptorSets(cmd,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            layout,
                            0,
                            (set2 != VK_NULL_HANDLE) ? 4u : 2u,
                            sets,
                            eff_count,
                            eff_count ? eff_offsets : nullptr);
    sLastDescLayout   = layout;
    sLastDescSet0     = set0;
    sLastDescSet1     = set1;
    sLastDescSet2     = set2;
    sLastDescDynCount = eff_count;
    if (eff_count > 0)
    {
        std::memcpy(sLastDescOffsets, eff_offsets, eff_count * sizeof(U32));
    }
}

void pushModelviewOnce(VkCommandBuffer cmd, VkPipelineLayout layout, const float* mv16)
{
    memoSyncCmd(cmd);
    if (vkCmdMemoEnabled() && layout == sLastMvLayout && std::memcmp(mv16, sLastMv, sizeof(sLastMv)) == 0)
    {
        ++gVkPerf.mv_skip;
        return;
    }
    ++gVkPerf.mv_push;
    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, mv16);
    sLastMvLayout = layout;
    std::memcpy(sLastMv, mv16, sizeof(sLastMv));
}

bool perFrameMatrixNeedsWrite()
{
    if (!sInitialized || sFrameIndex >= FRAMES_IN_FLIGHT)
    {
        return false;
    }
    return !sMatrixRingHasCurrent[sFrameIndex];
}

void setScissor(S32 x, S32 y, S32 w, S32 h)
{
    sScissorRectGL[0] = x;
    sScissorRectGL[1] = y;
    sScissorRectGL[2] = w;
    sScissorRectGL[3] = h;
    sScissorEnabled   = true;
}

void disableScissor()
{
    sScissorEnabled = false;
}

void notifyWindowResize(U32 width, U32 height)
{
    if (!sInitialized)
    {
        return;
    }
    if (sSurface == VK_NULL_HANDLE || sSwapchain == VK_NULL_HANDLE)
    {
        return;
    }

    sPendingResizeWidth  = width;
    sPendingResizeHeight = height;

    if (sMonotonicFrameCount < STARTUP_FRAME_GATE)
    {
        return;
    }

    if (width == sSwapchainExtent.width && height == sSwapchainExtent.height)
    {
        return;
    }

    sSwapchainRecreatePending = true;

}

VkImageView getDefaultFallbackVkImageView()
{
    return sDefaultFallbackImageView;
}

VkImageView getDefaultFallbackCubeArrayVkImageView()
{
    return sDefaultFallbackCubeArrayImageView;
}

VkImageView getDefaultFallbackCubeVkImageView()
{
    return sDefaultFallbackCubeImageView;
}

VkImageView getDefaultFallback3DVkImageView()
{
    return sDefaultFallback3DImageView;
}

VkImageView getDefaultFallbackShadowVkImageView()
{
    return sDefaultFallbackShadowImageView;
}

}
