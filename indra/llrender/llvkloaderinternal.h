/**
* @file llvkloaderinternal.h
* @brief AYAstorm r42 Vulkan loader internal shared declarations (split TUs)
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

#ifndef LL_LLVKLOADERINTERNAL_H
#define LL_LLVKLOADERINTERNAL_H

#include "llvkloader.h"
#include "volk.h"
#ifndef VMA_STATIC_VULKAN_FUNCTIONS
#define VMA_STATIC_VULKAN_FUNCTIONS  0
#endif
#ifndef VMA_DYNAMIC_VULKAN_FUNCTIONS
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#endif
#include "vk_mem_alloc.h"

#include <vector>
#include <string>
#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <mutex>
#include <set>
#include <queue>
#include <condition_variable>
#include <deque>
#include <thread>

class LLWindow;

namespace LLVKLoader
{
namespace LLVKLoaderInternal
{

extern VkInstance sInstance;
extern VkPhysicalDevice sPhysicalDevice;
extern VkDevice sDevice;
extern VkQueue sGraphicsQueue;
extern U32 sGraphicsQueueFamily;
extern bool sInitialized;
extern bool sCheckpointsEnabled;
extern bool sDeviceFaultEnabled;
extern VkDebugUtilsMessengerEXT sDebugMessenger;
extern VkCommandPool sCommandPool;
extern VkPipelineCache sPipelineCache;
extern VkQueryPool sOcclusionQueryPool;
extern U32 sOcclusionQueryCapacity;
extern std::queue<uint32_t> sOcclusionQueryFree;
extern VkQueryPool sTimestampQueryPool;
extern U32 sTimestampPairCapacity;
extern bool sTimestampSupported;
extern float sTimestampPeriodNs;
extern U32 sTimestampValidBits;
extern std::queue<uint32_t> sTimestampPairFree;
extern VkSwapchainKHR sSwapchain;
extern std::vector<VkImage> sSwapchainImages;
extern std::vector<VkImageView> sSwapchainImageViews;
extern VkExtent2D sSwapchainExtent;
extern VkImage sDefaultFallbackImage;
extern VkDeviceMemory sDefaultFallbackMemory;
extern VkImageView sDefaultFallbackImageView;
extern VkImage sWhiteImage;
extern VkDeviceMemory sWhiteMemory;
extern VkImageView sWhiteImageView;
extern VkImage sDefaultFallbackCubeArrayImage;
extern VkImageView sDefaultFallbackCubeArrayImageView;
extern void* sDefaultFallbackCubeArrayAlloc;
extern VkImage sDefaultFallbackCubeImage;
extern VkImageView sDefaultFallbackCubeImageView;
extern void* sDefaultFallbackCubeAlloc;
extern VkImage sDefaultFallback3DImage;
extern VkImageView sDefaultFallback3DImageView;
extern void* sDefaultFallback3DAlloc;
extern VkImage sDefaultFallbackShadowImage;
extern VkDeviceMemory sDefaultFallbackShadowMemory;
extern VkImageView sDefaultFallbackShadowImageView;
extern bool sInFrame;
extern thread_local bool sInDynamicRendering;
extern thread_local S32 sVkRenderViewport[4];
extern thread_local U32 sCurrentRenderAreaHeight;
extern thread_local VkRenderingAttachmentInfo sSavedColorInfos[4];
extern thread_local U32 sSavedColorCount;
extern thread_local VkRenderingAttachmentInfo sSavedDepthInfo;
extern thread_local bool sSavedHasDepth;
extern thread_local U32 sSavedRenderWidth;
extern thread_local U32 sSavedRenderHeight;
extern thread_local U32 sSavedViewMask;
extern thread_local U32 sSavedLayerCount;
extern VmaAllocator sAllocator;
extern VkDescriptorSetLayout sPerFrameDescriptorSetLayout;
extern VkBuffer sPerFrameUboBuffer[FRAMES_IN_FLIGHT];
extern VkDeviceMemory sPerFrameUboMemory[FRAMES_IN_FLIGHT];
extern void* sPerFrameUboMapped[FRAMES_IN_FLIGHT];
    struct MatrixRingChunk
    {
        VkBuffer       buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void*          mapped = nullptr;
    };

extern std::vector<MatrixRingChunk> sMatrixRingChunks[FRAMES_IN_FLIGHT];
extern std::vector<VkDescriptorSet> sPerFrameRingSets[FRAMES_IN_FLIGHT];
extern std::atomic<U32> sPerFrameRingSetCount[FRAMES_IN_FLIGHT];
extern std::vector<VkDescriptorPool> sPerFrameRingPools;
extern U32 sRingPoolUsedInLast;
extern std::atomic<U32> sMatrixRingUsedThisFrame[FRAMES_IN_FLIGHT];
extern thread_local U32 sMatrixRingCurrentSlot[FRAMES_IN_FLIGHT];
extern thread_local bool sMatrixRingHasCurrent[FRAMES_IN_FLIGHT];
extern thread_local VkCommandBuffer tRecordCmdOverride;
extern thread_local U64 tCmdRecordEpoch;
U32 vkcRaceSelf();
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

extern VkSampler sStandardLinearSampler;
extern std::unordered_map<U32, VkSampler> sSamplerCache;
extern bool sSamplerAnisotropyEnabled;
extern float sMaxSamplerAnisotropy;
extern VkPhysicalDeviceProperties sPhysicalDeviceProperties;
extern bool sBindlessCapable;
extern U32 sBindlessHeapCapacity;
extern bool sMultiDrawIndirectEnabled;
extern bool sDrawIndirectFirstInstanceEnabled;
extern VkDescriptorSet sBindlessHeapSet;
extern VkDescriptorSet sSkinBaseSet;
extern VkDescriptorSet sEmptySet;
extern std::vector<U32> sBindlessSlotFreeList;
extern std::mutex sBindlessSlotMutex;
    struct PendingSlotFree
    {
        U32 slot;
        U32 enqueue_frame;
    };

extern std::vector<PendingSlotFree> sPendingSlotFrees;
    constexpr U32            DRAWDATA_TOTAL_SLOTS                    = 1048576;

    constexpr U32            DRAWDATA_SCRATCH_PER_FRAME              = 32768;

    constexpr U32            DRAWDATA_PERSISTENT_SLOTS               = DRAWDATA_TOTAL_SLOTS - 3 * DRAWDATA_SCRATCH_PER_FRAME;

extern U32* sDrawDataMapped;
extern std::mutex sAllocGrowthMutex;
    // --- B.0/B.2 skin bindless base (SSBO palette + per-draw base index) ---
    constexpr U32            SKIN_PALETTE_ENTRY_BYTES                = 10560; // ObjectSkin_PerProgramBind (mat3x4[110] x2)

    constexpr U32            SKIN_ENTRIES_PER_FRAME                  = 1024;

extern U8* sSkinPaletteMapped;
extern U32* sSkinBaseMapped;
extern std::atomic<U32> sSkinPaletteCursor[FRAMES_IN_FLIGHT];
extern bool sSkinBindlessEnabled;
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

extern AllocDomain sMainDomain;
extern std::vector<AllocDomain*> sAllocDomains;
extern std::atomic<U32> sSlotSlabOwner[DRAWDATA_MAX_SLABS];
extern U32 sSlotSlabNextIdx;
extern thread_local AllocDomain* tAllocDomain;
bool slotSlabGrow(AllocDomain* d);
AllocDomain* allocDomainForSlot(U32 slot);
extern std::atomic<U32> sDrawDataScratchCursor;
extern VkBuffer sIndirectRingBuffer;
extern void* sIndirectRingAllocation;
extern U8* sIndirectRingMapped;
extern std::atomic<U32> sIndirectRingCursor;
extern U32 sIndirectRingFrame;
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
        std::vector<std::pair<U64, std::list<ScenePerDrawCacheKey>::iterator>> rev_views;
        std::vector<std::pair<U64, std::list<ScenePerDrawCacheKey>::iterator>> rev_bufs;
    };

extern U64 sScenePerDrawCacheEpoch;
    struct ScenePerDrawDeferredFreeEntry
    {
        VkDescriptorSet sets[FRAMES_IN_FLIGHT];
        U32             pool_index;
        U32             enqueue_frame;
    };

    struct PerDrawDescLane
    {
        std::vector<VkDescriptorPool> pools;
        std::unordered_map<ScenePerDrawCacheKey, ScenePerDrawCacheEntry, ScenePerDrawCacheKeyHash> cache;
        std::list<ScenePerDrawCacheKey> lru;
        std::vector<ScenePerDrawDeferredFreeEntry> deferred_free;
        std::unordered_map<U64, std::list<ScenePerDrawCacheKey>> by_view;
        std::unordered_map<U64, std::list<ScenePerDrawCacheKey>> by_buf;
    };

extern PerDrawDescLane sPerDrawDescLanes[MAX_RECORD_LANES];
    struct DeferredUtilOverrideSlot
    {
        VkBuffer buffer     = VK_NULL_HANDLE;
        void*    allocation = nullptr;
        void*    mapped     = nullptr;
    };

extern std::vector<DeferredUtilOverrideSlot> sDuOverrideRing[FRAMES_IN_FLIGHT];
extern U32 sDuOverrideIdx[FRAMES_IN_FLIGHT];
extern U64 sDuOverrideFrame[FRAMES_IN_FLIGHT];
extern std::vector<DeferredUtilOverrideSlot> sDeferredUtilRing[FRAMES_IN_FLIGHT];
extern U32 sDeferredUtilRingIdx[FRAMES_IN_FLIGHT];
extern U64 sDeferredUtilRingFrame[FRAMES_IN_FLIGHT];
extern VkBuffer sCurDeferredUtilBuf[FRAMES_IN_FLIGHT];
extern void* sCurDeferredUtilMapped[FRAMES_IN_FLIGHT];
extern std::vector<DeferredUtilOverrideSlot> sShadowUtilRing[FRAMES_IN_FLIGHT];
extern U32 sShadowUtilRingIdx[FRAMES_IN_FLIGHT];
extern U64 sShadowUtilRingFrame[FRAMES_IN_FLIGHT];
extern VkBuffer sCurShadowUtilBuf[FRAMES_IN_FLIGHT];
extern void* sCurShadowUtilMapped[FRAMES_IN_FLIGHT];
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

extern PerDrawUBOArena sPerDrawUBOArena[FRAMES_IN_FLIGHT];
extern U32 sFrameIndex;
extern VkCommandBuffer sCommandBuffers[FRAMES_IN_FLIGHT];
extern VkCommandBuffer sConsumerCommandBuffers[FRAMES_IN_FLIGHT];
extern VkCommandBuffer sAsyncProducerCommandBuffer;
extern bool sAsyncProducerInFlight;
extern bool sProducerFencePending[FRAMES_IN_FLIGHT];
extern VkSemaphore sGpuTimeline;
extern VkSemaphore sImageAvailableSemaphores[FRAMES_IN_FLIGHT];
extern VkSemaphore sRenderFinishedSemaphores[FRAMES_IN_FLIGHT];
extern VkImage sSwapchainDepthImage;
extern VkImageView sSwapchainDepthView;
extern VkImageLayout sSwapchainDepthLayout;
extern std::atomic<bool> sSwapchainRecreatePending;
    enum : U32
    {
        RECREATE_REASON_RESIZE          = 1,
        RECREATE_REASON_ACQ_SUBOPTIMAL  = 2,
        RECREATE_REASON_ACQ_OUT_OF_DATE = 4,
        RECREATE_REASON_PRS_SUBOPTIMAL  = 8,
        RECREATE_REASON_PRS_OUT_OF_DATE = 16,
        RECREATE_REASON_VSYNC_SETTING   = 32
    };

extern std::atomic<U32> sRecreateReasonMask;
extern U32 sPendingResizeWidth;
extern U32 sPendingResizeHeight;
extern U32 sMonotonicFrameCount;
extern U32 sLastRecreateFrame;
    constexpr U32 STARTUP_FRAME_GATE         = 30;

    struct PendingBufferFree
    {
        VkBuffer      buffer;
        VmaAllocation allocation;
        U32           enqueue_frame;
    };

extern std::vector<PendingBufferFree> sPendingBufferFrees;
    struct PendingImageFree
    {
        VkImage       image;
        VkImageView   view;
        VmaAllocation allocation;
        U32           enqueue_frame;
    };

extern std::vector<PendingImageFree> sPendingImageFrees;
extern std::mutex sPendingImageFreeMutex;
    struct PendingOneShotFree
    {
        VkCommandBuffer cmd        = VK_NULL_HANDLE;
        VkCommandPool   pool       = VK_NULL_HANDLE;
        VkBuffer        buffer     = VK_NULL_HANDLE;
        VmaAllocation   allocation = VK_NULL_HANDLE;
        U64             staging_bytes = 0;
        uint64_t        timeline_value = 0;   // この submit の GPU 完了 = timeline >= この値
    };

extern std::vector<PendingOneShotFree> sPendingOneShotFrees;
extern std::mutex sOneShotMutex;
extern std::unordered_map<VkCommandPool, std::vector<VkCommandBuffer>> sRetiredByPool;
extern std::atomic<U64> sOneShotStagingBytes;
extern thread_local VkCommandPool t_cmdPool;
VkCommandPool threadCmdPool();
extern void (*sGeoWorkerStopHook)();
extern void (*sBakeWorkerStopHook)();
extern U32 sLastCompletedMonotonic;
extern U32 sFrameSubmittedMonotonic[FRAMES_IN_FLIGHT];
    enum ReapMode { REAP_CHURN = 0, REAP_CLOSE = 1, REAP_LOST = 2, REAP_ALLOC_FAIL = 3 };

extern bool sReapForceAll;
extern bool sProducersQuiesced;
extern void (*sDeviceLostHook)();
extern bool sDeviceLostSignaled;
bool reapReady(U32 enqueue_frame);
VkCommandBuffer currentRecordCmd();
void gpuCheckpointImpl(const char* label);
void dumpCheckpointsOnDeviceLost();
void dumpDeviceFaultOnDeviceLost();
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
        // Captured when the consumer command buffer is queued, rather than
        // when the PE thread eventually calls vkQueuePresentKHR.
        U64            scene_id       = 0;
    };

    struct DisplayTimingIntervalStats
    {
        bool enabled = false;
        U64 actual_count = 0;
        U64 actual_interval_count = 0;
        U64 actual_interval_ns = 0;
        U64 fresh_count = 0;
        U64 fresh_interval_count = 0;
        U64 fresh_interval_ns = 0;
        U64 duplicate_count = 0;
        U64 unknown_scene_count = 0;
        U64 mapping_dropped = 0;
        U64 history_query_errors = 0;
        S32 last_history_query_result = VK_SUCCESS;
        U32 pending_mappings = 0;
        std::vector<U64> present_margin_ns;
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
        VkCommandBuffer cmd              = VK_NULL_HANDLE;
        VkFence         fence            = VK_NULL_HANDLE;
        VkSemaphore     wait_semaphore   = VK_NULL_HANDLE;
        VkSemaphore     signal_semaphore = VK_NULL_HANDLE;
        std::vector<PEPresentTarget> presents;
        bool            is_frame          = false;
        bool            is_oneshot        = false;
        bool            is_async_producer = false;
        bool            wait_idle         = false;
        U32             slot              = 0;
        PESyncPoint*    sync              = nullptr;
        uint64_t        timeline_value    = 0;   // この submit が signal する GPU タイムライン値
#if LL_DARWIN
        // Host-side attribution for the MoltenVK device-lost submit trace.
        const char*     oneshot_source    = nullptr;
        U64             oneshot_staging_bytes = 0;
#endif
    };

extern std::atomic<U32> sPESlotState[FRAMES_IN_FLIGHT];
extern bool sPERunning;
extern bool sPEThreaded;
extern std::mutex sSwapchainAccessMutex;
extern std::mutex sPEFailedMutex;
extern std::vector<uint64_t> sPEFailedOneShotValues; // 失敗した oneshot submit の timeline 値(回収用);
extern std::atomic<bool> sVkDeviceLost;
#if LL_DARWIN
void recordDarwinSubmitTrace(const PEJob& job, VkResult result);
#endif
#if LL_DARWIN
void dumpDarwinSubmitTraceOnDeviceLost(const char* trigger);
#endif
extern std::atomic<U64> sPESubmitUs;
extern std::atomic<U64> sPEPresentUs;
extern std::atomic<U64> sPEPrsMainUs;
extern std::atomic<U64> sPEPrsAuxUs;
extern std::atomic<U64> sPEPrsLockUs;
extern std::atomic<U64> sPEPwMainUs;
extern std::atomic<U64> sPEPwAuxUs;
// Per AYASTORM_PERF_LOG interval. `done` is meaningful only while
// VK_KHR_present_wait is enabled; otherwise Vulkan cannot report scanout.
extern std::atomic<U64> sMainSwapchainAcquireCount;
extern std::atomic<U64> sMainPresentCallCount;
extern std::atomic<U64> sMainPresentAcceptedCount;
// Present-wait availability is negotiated separately. These count actual calls.
extern std::atomic<U64> sMainPresentWaitAttemptCount;
extern std::atomic<U64> sMainPresentDoneCount;
extern std::atomic<U64> sMainPresentWaitTimeoutCount;
extern std::atomic<U64> sMainPresentWaitErrorCount;
extern std::atomic<U64> sAuxBeginFenceUs;
extern std::atomic<U64> sAuxBeginAcqUs;
extern std::atomic<U64> sProdEnqToSubUs;
extern std::atomic<U32> sProdSubCount;
extern std::atomic<U64> sProdEnqMonoUs;
U64 vkMonoUs();
extern bool sPresentWaitEnabled;
extern bool sDisplayTimingRequested;
extern bool sDisplayTimingEnabled;
extern VkPresentModeKHR sActivePresentMode;
extern std::atomic<bool> sVsyncEnabled;
U64 currentDisplayTimingSceneId();
void advanceDisplayTimingSceneId();
void resetDisplayTimingHistory();
DisplayTimingIntervalStats collectDisplayTimingIntervalStats();
uint64_t gpuTimelineValue();
void waitTimeline(uint64_t v);
uint64_t peEnqueue(PEJob&& job);
VkResult peSubmitBlocking(VkCommandBuffer cmd, VkFence fence, bool wait_idle);
void peDrain();
U32 peQueueDepth();
void peStart();
void peStop();
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

extern std::unordered_map<U64, Set1BirthInfo> sSet1BirthLedger;
extern std::mutex sSet1BirthMutex;
extern std::unordered_set<U64> sDeadViewHandles;
extern std::unordered_set<U64> sDeadBufferHandles;
extern std::mutex sDeadHandleMutex;
extern std::mutex sVvlCountMutex;
extern std::unordered_map<S32, std::pair<std::string, U64>> sVvlCounts;
void noteViewHandleCreated(VkImageView v);
void noteBufferHandleCreated(VkBuffer b);
    struct ViewDeathInfo
    {
        U64 enqueue_frame   = 0;
        U64 destroy_frame   = 0;
        U64 enqueue_retaddr = 0;
    };

extern std::unordered_map<U64, ViewDeathInfo> sViewDeathLedger;
std::string hex64(U64 v);
bool vkValidationRequested();
std::string set1BirthLookup(const char* msg);
bool createInstance();
bool selectPhysicalDevice();
bool selectQueueFamily();
bool createDevice();
S32 findMemoryType(U32 type_filter, VkMemoryPropertyFlags properties);
bool createCommandPool();
bool createDefaultFallbackImage();
bool createWhiteImage();
bool createDefaultFallbackShadowImage();
bool createDefaultFallbackCubeArrayImage();
bool createDefaultFallbackCubeImage();
bool createDefaultFallback3DImage();
void bindlessWriteSlotInternal(U32 slot, VkImageView view, VkSampler sampler);
void destroyBindlessHeap();
bool createBindlessHeap();
bool createPipelineCacheStorage();
bool createPipelineCache();
bool createVmaAllocator();
bool createPerFrameDescriptorSetLayout();
bool createPerFrameUbos();
bool createPerFrameDescriptorSets();
bool createScenePerDrawDescriptorPool(VkDescriptorPool* out_pool);
bool createStandardSampler();
U32 vkFormatBytesPerPixelImpl(VkFormat format);
bool createBufferVkImpl(U32 size_bytes, VkBufferUsageFlags usage, VkBuffer& out_buffer, void*& out_allocation, void** out_mapped, bool prefer_device = false);
void setVkObjectName(U64 handle, VkObjectType type, const char* name);
bool createSyncObjects();
void destroySyncObjects();
void destroySwapchain();
bool recreateSwapchain();
extern std::vector<U32> sMegaTypeSizes;
void vbUploadFrameReset();
void vbUploadSubmit();
#define LLVK_SHARED_UBO_RING_STORAGE_DECL(BindName)                                                     \
    extern std::vector<DeferredUtilOverrideSlot> s##BindName##Ring[FRAMES_IN_FLIGHT];                   \
    extern U32      s##BindName##RingIdx[FRAMES_IN_FLIGHT];                                             \
    extern U64      s##BindName##RingFrame[FRAMES_IN_FLIGHT];                                           \
    extern VkBuffer sCur##BindName##Buf[FRAMES_IN_FLIGHT];                                              \
    extern void*    sCur##BindName##Mapped[FRAMES_IN_FLIGHT];
LLVK_SHARED_UBO_RING_STORAGE_DECL(WindlightSky)
LLVK_SHARED_UBO_RING_STORAGE_DECL(WindlightAtmos)
LLVK_SHARED_UBO_RING_STORAGE_DECL(AoUtil)
LLVK_SHARED_UBO_RING_STORAGE_DECL(GlobalF)
LLVK_SHARED_UBO_RING_STORAGE_DECL(WaterFog)
LLVK_SHARED_UBO_RING_STORAGE_DECL(WaterV)
LLVK_SHARED_UBO_RING_STORAGE_DECL(ReflectionProbe)
LLVK_SHARED_UBO_RING_STORAGE_DECL(ReflectionProbes)
LLVK_SHARED_UBO_RING_STORAGE_DECL(Lights)
LLVK_SHARED_UBO_RING_STORAGE_DECL(LightsSpecular)
LLVK_SHARED_UBO_RING_STORAGE_DECL(PbrTerrainF)
LLVK_SHARED_UBO_RING_STORAGE_DECL(PbrTerrain)
#undef LLVK_SHARED_UBO_RING_STORAGE_DECL

} // namespace LLVKLoaderInternal
using namespace LLVKLoaderInternal;

namespace pcache
{
    using CacheBlob = std::vector<uint8_t>;
    extern std::string sFilePath;
    extern CacheBlob   sBlob;
    extern std::size_t sMaxSizeBytes;
    extern bool        sInitialized;
    bool writeBlobToDisk(const std::string& path, const CacheBlob& blob);
}

void reclaimDeferredOnAllocFailure();
void tickPerDrawUBOArena();
void teardownSharedLatchedUBOs();
bool initSharedDynamicPersistentUBOs();
void teardownSharedDynamicPersistentUBOs();
void tickSharedDynamicPersistentUBOs();
void purgePerDrawDeadHandles(const std::unordered_set<U64>& dead_views, const std::unordered_set<U64>& dead_bufs);
void tickIndirectRing();
bool initSharedDynamicPersistentUBOs();
void noteDeferredEnqueueThread();
void tickDeferredQueryReleaseQueue();
void tickScenePerDrawDescriptorCache();
U32 skinBaseDynamicOffsetBytes();
void tickDeferredBufferFreeQueue();
void tickOneShotFreeQueue();
void drawDataReclaimDomain(AllocDomain* d);
void tickMegaFreeQueue();
void tickDeferredImageFreeQueue();
bool allocDomainSelfTest();
void shutdownSurface();
} // namespace LLVKLoader

#endif // LL_LLVKLOADERINTERNAL_H
