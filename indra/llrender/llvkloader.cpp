/**
* @file llvkloader.cpp
* @brief AYAstorm r42 Vulkan loader — init+shutdown drive / frame begin-end / pass management / shared state definitions
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
#include "llvkloaderinternal.h"

namespace LLVKLoader
{

static void pollAsyncProducerCompletion();
static void refreshCompletedWatermark();

namespace LLVKLoaderInternal
{
    VkInstance       sInstance            = VK_NULL_HANDLE;
    VkPhysicalDevice sPhysicalDevice      = VK_NULL_HANDLE;
    VkDevice         sDevice              = VK_NULL_HANDLE;
    VkQueue          sGraphicsQueue       = VK_NULL_HANDLE;
    U32              sGraphicsQueueFamily = UINT_MAX;
    bool             sInitialized         = false;
    bool             sCheckpointsEnabled  = false;
    bool             sDeviceFaultEnabled  = false;

    VkDebugUtilsMessengerEXT sDebugMessenger = VK_NULL_HANDLE;

    VkCommandPool   sCommandPool   = VK_NULL_HANDLE;
    VkPipelineCache sPipelineCache = VK_NULL_HANDLE;

    VkQueryPool          sOcclusionQueryPool     = VK_NULL_HANDLE;
    U32                  sOcclusionQueryCapacity = 0;
    std::queue<uint32_t> sOcclusionQueryFree;

    VkQueryPool          sTimestampQueryPool     = VK_NULL_HANDLE;
    U32                  sTimestampPairCapacity  = 0;
    bool                 sTimestampSupported     = false;
    float                sTimestampPeriodNs      = 0.0f;
    U32                  sTimestampValidBits     = 0;
    std::queue<uint32_t> sTimestampPairFree;

    VkSwapchainKHR           sSwapchain           = VK_NULL_HANDLE;
    std::vector<VkImage>     sSwapchainImages;
    std::vector<VkImageView> sSwapchainImageViews;
    VkExtent2D               sSwapchainExtent     = {0, 0};

    VkImage        sDefaultFallbackImage     = VK_NULL_HANDLE;
    VkDeviceMemory sDefaultFallbackMemory    = VK_NULL_HANDLE;
    VkImageView    sDefaultFallbackImageView = VK_NULL_HANDLE;
    VkImage        sWhiteImage     = VK_NULL_HANDLE;
    VkDeviceMemory sWhiteMemory    = VK_NULL_HANDLE;
    VkImageView    sWhiteImageView = VK_NULL_HANDLE;
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

    thread_local VkRenderingAttachmentInfo sSavedColorInfos[4] = {};
    thread_local U32                       sSavedColorCount    = 0;
    thread_local VkRenderingAttachmentInfo sSavedDepthInfo     = {};
    thread_local bool                      sSavedHasDepth      = false;
    thread_local U32                       sSavedRenderWidth   = 0;
    thread_local U32                       sSavedRenderHeight  = 0;
    thread_local U32                       sSavedViewMask   = 0;
    thread_local U32                       sSavedLayerCount = 1;

    VmaAllocator sAllocator = VK_NULL_HANDLE;

    VkDescriptorSetLayout sPerFrameDescriptorSetLayout            = VK_NULL_HANDLE;
    VkBuffer              sPerFrameUboBuffer[FRAMES_IN_FLIGHT]    = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    VkDeviceMemory        sPerFrameUboMemory[FRAMES_IN_FLIGHT]    = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    void*                 sPerFrameUboMapped[FRAMES_IN_FLIGHT]    = { nullptr,        nullptr,        nullptr        };
    std::vector<MatrixRingChunk> sMatrixRingChunks[FRAMES_IN_FLIGHT];
    std::vector<VkDescriptorSet> sPerFrameRingSets[FRAMES_IN_FLIGHT];
    std::atomic<U32>             sPerFrameRingSetCount[FRAMES_IN_FLIGHT] = {};
    std::vector<VkDescriptorPool> sPerFrameRingPools;
    U32   sRingPoolUsedInLast = 0;
    std::atomic<U32> sMatrixRingUsedThisFrame[FRAMES_IN_FLIGHT] = {};
    thread_local U32   sMatrixRingCurrentSlot[FRAMES_IN_FLIGHT]   = { 0, 0, 0 };
    thread_local bool  sMatrixRingHasCurrent[FRAMES_IN_FLIGHT]    = { false, false, false };

    thread_local VkCommandBuffer  tRecordCmdOverride = VK_NULL_HANDLE;

    thread_local U64              tCmdRecordEpoch = 1;
} // namespace LLVKLoaderInternal

namespace
{

    std::atomic<U32> sVkcSlotOwner{0};
    std::atomic<U32> sVkcMegaOwner{0};
} // namespace

namespace LLVKLoaderInternal
{

    VkSampler             sStandardLinearSampler                  = VK_NULL_HANDLE;

    std::unordered_map<U32, VkSampler> sSamplerCache;
    bool                     sSamplerAnisotropyEnabled               = false;
    float                    sMaxSamplerAnisotropy                   = 1.0f;

    VkPhysicalDeviceProperties sPhysicalDeviceProperties             = {};

    bool                     sBindlessCapable                        = false;
    U32                      sBindlessHeapCapacity                   = 0;
    bool                     sMultiDrawIndirectEnabled               = false;
    bool                     sDrawIndirectFirstInstanceEnabled       = false;
    VkDescriptorSet          sBindlessHeapSet                        = VK_NULL_HANDLE;
    VkDescriptorSet          sSkinBaseSet                            = VK_NULL_HANDLE;
    VkDescriptorSet          sEmptySet                               = VK_NULL_HANDLE;
    std::vector<U32>         sBindlessSlotFreeList;
    std::mutex               sBindlessSlotMutex;
    std::vector<PendingSlotFree> sPendingSlotFrees;
    U32*                     sDrawDataMapped                         = nullptr;
    std::mutex               sAllocGrowthMutex;
    U8*                      sSkinPaletteMapped                      = nullptr;
    U32*                     sSkinBaseMapped                         = nullptr;
    std::atomic<U32>         sSkinPaletteCursor[FRAMES_IN_FLIGHT]    = {};
    bool                     sSkinBindlessEnabled                    = true; // B.2 kill switch (AYASTORM_SKIN_BINDLESS=0)
    AllocDomain              sMainDomain{0};
    std::vector<AllocDomain*> sAllocDomains;
    std::atomic<U32>         sSlotSlabOwner[DRAWDATA_MAX_SLABS];
    U32                      sSlotSlabNextIdx                        = 0;
    thread_local AllocDomain* tAllocDomain                          = &sMainDomain;
} // namespace LLVKLoaderInternal

namespace
{
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
} // namespace

namespace LLVKLoaderInternal
{
    std::atomic<U32>         sDrawDataScratchCursor{0};
    VkBuffer                 sIndirectRingBuffer                     = VK_NULL_HANDLE;
    void*                    sIndirectRingAllocation                 = nullptr;
    U8*                      sIndirectRingMapped                     = nullptr;
    std::atomic<U32>         sIndirectRingCursor{0};
    U32                      sIndirectRingFrame                      = 0xFFFFFFFFu;

    U32 sFrameIndex = 0;

    VkCommandBuffer sCommandBuffers[FRAMES_IN_FLIGHT] = {
        VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE
    };
    VkCommandBuffer sConsumerCommandBuffers[FRAMES_IN_FLIGHT] = {
        VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE
    };
} // namespace LLVKLoaderInternal

namespace
{
    constexpr bool sUISceneSplit = true;
    constexpr bool sUISceneAsync = true;
    bool sConsumerActiveThisFrame = false;
} // namespace

namespace LLVKLoaderInternal
{

    VkCommandBuffer sAsyncProducerCommandBuffer = VK_NULL_HANDLE;
    bool            sAsyncProducerInFlight      = false;
} // namespace LLVKLoaderInternal

namespace
{
    bool            sAsyncProducerFlipPending   = false;
    U32             sAsyncProducerBackIndex     = 0;
    bool            sAsyncRenderSceneThisFrame  = false;
    bool            sAsyncFrameEngaged          = false;
    U32             sAsyncProducerSubmitMonotonic = 0;
    U32             sAsyncProducerRecordSlot      = 0;
    U32             sAsyncProducerSubmitCount     = 0;
} // namespace

namespace LLVKLoaderInternal
{

    bool sProducerFencePending[FRAMES_IN_FLIGHT] = { false, false, false };
} // namespace LLVKLoaderInternal

namespace
{
    bool sProducerPresentActive = false;
} // namespace

namespace LLVKLoaderInternal
{
    VkSemaphore           sGpuTimeline              = VK_NULL_HANDLE;
} // namespace LLVKLoaderInternal

namespace
{
    uint64_t sFrameTimelineValue[FRAMES_IN_FLIGHT]    = { 0, 0, 0 }; // slot 直近 frame submit の値
    uint64_t sProducerTimelineValue[FRAMES_IN_FLIGHT] = { 0, 0, 0 };
    uint64_t sAsyncTimelineValue = 0;
} // namespace

namespace LLVKLoaderInternal
{

    VkSemaphore sImageAvailableSemaphores[FRAMES_IN_FLIGHT] = {
        VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE
    };
    VkSemaphore sRenderFinishedSemaphores[FRAMES_IN_FLIGHT] = {
        VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE
    };
} // namespace LLVKLoaderInternal

namespace
{
    U32  sAcquiredImageIndex         = 0;
    bool sImageAcquired              = false;
    bool sFrameWantsPresent          = false;
    bool sVulkanPresentationEnabled  = true;

    bool sSwapchainClearedThisFrame  = false;
} // namespace

namespace LLVKLoaderInternal
{

    VkImage       sSwapchainDepthImage  = VK_NULL_HANDLE;
    VkImageView   sSwapchainDepthView   = VK_NULL_HANDLE;
    VkImageLayout sSwapchainDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    std::atomic<bool> sSwapchainRecreatePending{false};
    std::atomic<U32> sRecreateReasonMask{0};
    U32  sPendingResizeWidth       = 0;
    U32  sPendingResizeHeight      = 0;
    U32  sMonotonicFrameCount      = 0;
    U32  sLastRecreateFrame        = 0;
} // namespace LLVKLoaderInternal

namespace
{
    constexpr U32 RECREATE_COOLDOWN_FRAMES   = 30;
} // namespace

namespace LLVKLoaderInternal
{
    std::vector<PendingBufferFree> sPendingBufferFrees;
    std::vector<PendingImageFree> sPendingImageFrees;
    std::mutex                    sPendingImageFreeMutex;
} // namespace LLVKLoaderInternal

namespace
{

    struct PendingObjectFree
    {
        VkPipeline            pipeline              = VK_NULL_HANDLE;
        VkShaderModule        shader_module         = VK_NULL_HANDLE;
        VkPipelineLayout      pipeline_layout       = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
        U32                   enqueue_frame         = 0;
    };
    std::vector<PendingObjectFree> sPendingObjectFrees;
} // namespace

namespace LLVKLoaderInternal
{
    std::vector<PendingOneShotFree> sPendingOneShotFrees;
    std::mutex                      sOneShotMutex;
    std::unordered_map<VkCommandPool, std::vector<VkCommandBuffer>> sRetiredByPool;
    std::atomic<U64>                sOneShotStagingBytes{0};
    thread_local VkCommandPool      t_cmdPool = VK_NULL_HANDLE;

    std::atomic<U32>        sPESlotState[FRAMES_IN_FLIGHT] = {};
    bool                    sPERunning       = false;
    bool                    sPEThreaded      = false;

    std::mutex              sSwapchainAccessMutex;

    std::mutex              sPEFailedMutex;
    std::vector<uint64_t>   sPEFailedOneShotValues;   // 失敗した oneshot submit の timeline 値(回収用)
    std::atomic<bool>       sVkDeviceLost{false};

    std::atomic<U64>        sPESubmitUs{0};
    std::atomic<U64>        sPEPresentUs{0};
    std::atomic<U64>        sPEPrsMainUs{0};
    std::atomic<U64>        sPEPrsAuxUs{0};
    std::atomic<U64>        sPEPrsLockUs{0};
    std::atomic<U64>        sPEPwMainUs{0};
    std::atomic<U64>        sPEPwAuxUs{0};
    std::atomic<U64>        sAuxBeginFenceUs{0};
    std::atomic<U64>        sAuxBeginAcqUs{0};
    std::atomic<U64>        sProdEnqToSubUs{0};
    std::atomic<U32>        sProdSubCount{0};
    std::atomic<U64>        sProdEnqMonoUs{0};
} // namespace LLVKLoaderInternal

namespace
{
    std::atomic<U32>        sProdCheckFirstReady{0};
    std::atomic<U32>        sProdCheckTotalReady{0};
    std::atomic<U32>        sProdChecksSinceSubmit{0};
} // namespace

namespace LLVKLoaderInternal
{
    bool                    sPresentWaitEnabled = false;
    VkPresentModeKHR        sActivePresentMode = VK_PRESENT_MODE_FIFO_KHR;
    std::atomic<bool>       sVsyncEnabled{true};
    std::unordered_map<U64, ViewDeathInfo> sViewDeathLedger;
} // namespace LLVKLoaderInternal

static void           endSwapchainRendering();
static void           tickDeferredObjectFreeQueue();
static void           reapAllDeferred(ReapMode mode);

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

#if LL_DARWIN
    const std::string manifest_path = gDirUtilp->getAppRODataDir() + gDirUtilp->getDirDelimiter()
                                      + "vulkan/icd.d/MoltenVK_icd.json";
    if (!gDirUtilp->fileExists(manifest_path))
    {
        LL_WARNS("Vulkan") << "missing bundled driver manifest: " << manifest_path << LL_ENDL;
        return false;
    }
    if (setenv("VK_DRIVER_FILES", manifest_path.c_str(), 1) != 0)
    {
        LL_WARNS("Vulkan") << "could not set VK_DRIVER_FILES errno=" << errno << LL_ENDL;
        return false;
    }
    LL_INFOS("Vulkan") << "macOS Loader driver manifest: " << manifest_path << LL_ENDL;
#endif

    VkResult result = volkInitialize();
    if (result != VK_SUCCESS)
    {
        LL_WARNS("Vulkan") << "volkInitialize failed result=" << result << LL_ENDL;
        return false;
    }

    if (!createInstance())
    {
        LL_WARNS("Vulkan") << "instance creation failed" << LL_ENDL;
        return false;
    }

    if (!selectPhysicalDevice() || !selectQueueFamily() || !createDevice())
    {
        LL_WARNS("Vulkan") << "physical device, graphics queue, or logical device initialization failed" << LL_ENDL;
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
        LL_WARNS("Vulkan") << "initialization failed: pipeline cache storage" << LL_ENDL;
        shutdownVulkan();
        return false;
    }

    if (!createCommandPool())
    {
        LL_WARNS("Vulkan") << "initialization failed: command pool" << LL_ENDL;
        shutdownVulkan();
        return false;
    }
    if (!createDefaultFallbackImage())
    {
        LL_WARNS("Vulkan") << "initialization failed: 2D fallback image" << LL_ENDL;
        shutdownVulkan();
        return false;
    }
    if (!createWhiteImage())
    {
        LL_WARNS("Vulkan") << "initialization failed: white image" << LL_ENDL;
        shutdownVulkan();
        return false;
    }
    if (!createPipelineCache())
    {
        LL_WARNS("Vulkan") << "initialization failed: pipeline cache" << LL_ENDL;
        shutdownVulkan();
        return false;
    }

    if (!createVmaAllocator())
    {
        LL_WARNS("Vulkan") << "initialization failed: VMA allocator" << LL_ENDL;
        shutdownVulkan();
        return false;
    }

    if (!createDefaultFallbackCubeArrayImage())
    {
        LL_WARNS("Vulkan") << "initialization failed: cube-array fallback image" << LL_ENDL;
        shutdownVulkan();
        return false;
    }
    if (!createDefaultFallbackCubeImage())
    {
        LL_WARNS("Vulkan") << "initialization failed: cube fallback image" << LL_ENDL;
        shutdownVulkan();
        return false;
    }
    if (!createDefaultFallback3DImage())
    {
        LL_WARNS("Vulkan") << "initialization failed: 3D fallback image" << LL_ENDL;
        shutdownVulkan();
        return false;
    }
    if (!createDefaultFallbackShadowImage())
    {
        LL_WARNS("Vulkan") << "initialization failed: shadow fallback image" << LL_ENDL;
        shutdownVulkan();
        return false;
    }

    if (!createPerFrameDescriptorSetLayout())
    {
        LL_WARNS("Vulkan") << "initialization failed: per-frame descriptor layout" << LL_ENDL;
        shutdownVulkan();
        return false;
    }
    if (!createPerFrameUbos())
    {
        LL_WARNS("Vulkan") << "initialization failed: per-frame UBOs" << LL_ENDL;
        shutdownVulkan();
        return false;
    }
    if (!initSharedDynamicPersistentUBOs())
    {
        LL_WARNS("Vulkan") << "initialization failed: shared persistent UBOs" << LL_ENDL;
        shutdownVulkan();
        return false;
    }
    if (!createPerFrameDescriptorSets())
    {
        LL_WARNS("Vulkan") << "initialization failed: per-frame descriptor sets" << LL_ENDL;
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
        LL_WARNS("Vulkan") << "initialization failed: standard sampler" << LL_ENDL;
        shutdownVulkan();
        return false;
    }

    createBindlessHeap();

    if (!createSyncObjects())
    {
        LL_WARNS("Vulkan") << "initialization failed: synchronization objects" << LL_ENDL;
        shutdownVulkan();
        return false;
    }

    peStart();

    sInitialized = true;
    LL_INFOS("Vulkan") << "initialized device=" << sPhysicalDeviceProperties.deviceName << LL_ENDL;
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
    peStop();
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
            if (vkDeviceWaitIdle(sDevice) != VK_SUCCESS)
            {
                device_lost = true;
                sVkDeviceLost.store(true, std::memory_order_release);
            }
        }
        reapAllDeferred(device_lost ? REAP_LOST : REAP_CLOSE);

        auxWindowShutdownVk();
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
        if (sWhiteImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(sDevice, sWhiteImageView, nullptr);
            sWhiteImageView = VK_NULL_HANDLE;
        }
        if (sWhiteImage != VK_NULL_HANDLE)
        {
            vkDestroyImage(sDevice, sWhiteImage, nullptr);
            sWhiteImage = VK_NULL_HANDLE;
        }
        if (sWhiteMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(sDevice, sWhiteMemory, nullptr);
            sWhiteMemory = VK_NULL_HANDLE;
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
            lane.by_view.clear();
            lane.by_buf.clear();
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
            {
                std::lock_guard<std::mutex> guard(sPendingImageFreeMutex);
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
            }
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

static void beginCommandRecording()
{
    ++tCmdRecordEpoch;
    LLGLSLShader::sCurPerCallVkOffsetsDirty = true;
}

static void reapAllDeferred(ReapMode mode)
{
    sReapForceAll = (mode != REAP_CHURN && mode != REAP_ALLOC_FAIL);
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
        tickIndirectRing();
        tickScenePerDrawDescriptorCache();
    }
    sReapForceAll = false;
}

static void pollAsyncProducerCompletion()
{
    if (sAsyncProducerInFlight &&
        gpuTimelineValue() >= sAsyncTimelineValue)
    {
        sAsyncProducerInFlight    = false;
        sAsyncProducerFlipPending = true;
    }
}

static void refreshCompletedWatermark()
{
    pollAsyncProducerCompletion();
    const uint64_t cur = gpuTimelineValue();
    for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        if (sFrameTimelineValue[i] != 0 &&
            cur >= sFrameTimelineValue[i] &&
            sFrameSubmittedMonotonic[i] > sLastCompletedMonotonic)
        {
            sLastCompletedMonotonic = sFrameSubmittedMonotonic[i];
        }
    }
    if (sAsyncProducerInFlight && sAsyncProducerSubmitMonotonic > 0 &&
        sLastCompletedMonotonic >= sAsyncProducerSubmitMonotonic)
    {
        sLastCompletedMonotonic = sAsyncProducerSubmitMonotonic - 1;
    }
}

void reclaimDeferredOnAllocFailure()
{
    if (!on_main_thread())
    {
        return;
    }
    refreshCompletedWatermark();
    reapAllDeferred(REAP_ALLOC_FAIL);
}

U32 getPendingImageFreeCount()
{
    std::lock_guard<std::mutex> guard(sPendingImageFreeMutex);
    return (U32)sPendingImageFrees.size();
}

U32 getVmaTotalBlockCount()
{
    if (sAllocator == VK_NULL_HANDLE)
    {
        return 0;
    }
    VmaBudget budgets[VK_MAX_MEMORY_HEAPS] = {};
    vmaGetHeapBudgets(sAllocator, budgets);
    U32 total = 0;
    for (U32 i = 0; i < VK_MAX_MEMORY_HEAPS; ++i)
    {
        total += budgets[i].statistics.blockCount;
    }
    return total;
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
    if (sAsyncProducerFlipPending)
    {
        sAsyncProducerFlipPending = false;
        return true;
    }
    if (!sAsyncProducerInFlight)
    {
        return false;
    }
    const U32 checks = sProdChecksSinceSubmit.fetch_add(1) + 1;
    if (gpuTimelineValue() >= sAsyncTimelineValue)
    {
        ++sProdCheckTotalReady;
        if (checks == 1)
        {
            ++sProdCheckFirstReady;
        }
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

bool isSwapchainImageAcquired()
{
    return sImageAcquired;
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
            sRecreateReasonMask.fetch_or(RECREATE_REASON_RESIZE);
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
            if (!recreateSwapchain())
            {
                return false;
            }
        }
    }

    ++sMonotonicFrameCount;
    LLVKContract::frameBegin();

    sFrameIndex = (sFrameIndex + 1) % FRAMES_IN_FLIGHT;
    sSkinPaletteCursor[sFrameIndex].store(0, std::memory_order_relaxed); // B.0: reset skin palette region ring
    sDrawDataScratchCursor.store(0, std::memory_order_relaxed);

    // slot 再利用前に、この slot の前回 frame の GPU 完了を timeline で待つ(fence 不要)。
    {
        VkPerfMainScope mlp_fence(1);
        waitTimeline(sFrameTimelineValue[sFrameIndex]);
    }
    refreshCompletedWatermark();
    if (sProducerFencePending[sFrameIndex])
    {
        waitTimeline(sProducerTimelineValue[sFrameIndex]);
        sProducerFencePending[sFrameIndex] = false;
    }
    if (sAsyncProducerInFlight && sAsyncProducerRecordSlot == sFrameIndex)
    {
        waitTimeline(sAsyncTimelineValue);
    }
    sPESlotState[sFrameIndex].store(PE_SLOT_IDLE);
    vbUploadFrameReset();

    if (sFrameIndex < FRAMES_IN_FLIGHT)
    {
        sMatrixRingUsedThisFrame[sFrameIndex] = 0;
        sMatrixRingHasCurrent[sFrameIndex]    = false;
    }

    sImageAcquired = false;
    sFrameWantsPresent = acquire_swapchain && sVulkanPresentationEnabled;
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
                sRecreateReasonMask.fetch_or(RECREATE_REASON_ACQ_SUBOPTIMAL);
                sSwapchainRecreatePending = true;
            }
        }
        else if (acquire_res == VK_ERROR_OUT_OF_DATE_KHR)
        {
            sRecreateReasonMask.fetch_or(RECREATE_REASON_ACQ_OUT_OF_DATE);
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
thread_local U32 gVkPerfShadowCtx = 0;
thread_local U32 gVkPerfShadowSection = 0;

std::atomic<U64> gVkPerDrawTopologyGen{1};
std::atomic<U64> gVkViewDestroyGen{1};
std::atomic<U64> gVkReloadEpoch{1};

bool endFrame()
{
    if (!sInitialized || !sInFrame)
    {
        return false;
    }

    VkPerfMainScope mlp_total(4);

    gpuCheckpointImpl("frame:tail");

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
                if (isUISceneAsync())
                {
                    static U32 s_last_prod = 0;
                    const U32 prod = sAsyncProducerSubmitCount - s_last_prod;
                    s_last_prod = sAsyncProducerSubmitCount;
                    LL_INFOS("VkPerf") << "uiscene consumer_fps=" << ((F64)frames / elapsed)
                                       << " producer_fps=" << ((F64)prod / elapsed)
                                       << " ready1st=" << sProdCheckFirstReady.exchange(0)
                                       << "/" << sProdCheckTotalReady.exchange(0)
                                       << " enq2sub_ms=" << ((F64)sProdEnqToSubUs.exchange(0) / 1000.0)
                                       << "/" << sProdSubCount.exchange(0)
                                       << " prs_main=" << ((F64)sPEPrsMainUs.exchange(0) / 1000.0)
                                       << " prs_aux=" << ((F64)sPEPrsAuxUs.exchange(0) / 1000.0)
                                       << " prs_lock=" << ((F64)sPEPrsLockUs.exchange(0) / 1000.0)
                                       << " pw_main=" << ((F64)sPEPwMainUs.exchange(0) / 1000.0)
                                       << " pw_aux=" << ((F64)sPEPwAuxUs.exchange(0) / 1000.0)
                                       << " auxfw=" << ((F64)sAuxBeginFenceUs.exchange(0) / 1000.0)
                                       << " auxacq=" << ((F64)sAuxBeginAcqUs.exchange(0) / 1000.0)
                                       << LL_ENDL;
                }
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
                                   << " vbcp " << gVkPerf.vb_orphan.load()
                                   << "/" << (gVkPerf.vb_copy_bytes.load() >> 10) << "k"
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
                                   << " mv=" << gVkPerf.draws_shadow_map[6].load()
                                   << " spot " << gVkPerf.draws_shadow_map[4].load()
                                   << "/" << gVkPerf.draws_shadow_map[5].load()
                                   << " culled=" << gVkPerf.shadow_cull.load()
                                   << " rigged=" << gVkPerf.shadow_rigged.load()
                                   << " rigmap " << gVkPerf.shadow_rigged_map[0].load()
                                   << "/" << gVkPerf.shadow_rigged_map[1].load()
                                   << "/" << gVkPerf.shadow_rigged_map[2].load()
                                   << "/" << gVkPerf.shadow_rigged_map[3].load()
                                   << " mv=" << gVkPerf.shadow_rigged_map[6].load()
                                   << " spot " << gVkPerf.shadow_rigged_map[4].load()
                                   << "/" << gVkPerf.shadow_rigged_map[5].load()
                                   << [](){ std::string s;
                                        static const char* ctx_names[VKPERF_SHCTX_COUNT] = {
                                            "?", "mv", "rest", "fb", "spot" };
                                        static const char* sec_names[VKPERF_SHSEC_COUNT] = {
                                            "other","op","opR","ter","tree","av",
                                            "am","amR","ab","abR","fbm","fbmR",
                                            "gm","gmR","ga","gaR","pbr","pbrR" };
                                        for (U32 c = 0; c < VKPERF_SHCTX_COUNT; ++c) {
                                            for (U32 k = 0; k < VKPERF_SHSEC_COUNT; ++k) {
                                                const U64 n = gVkPerf.draws_shadow_site[c][k].load();
                                                if (n == 0) continue;
                                                s += s.empty() ? " | shsite " : " ";
                                                s += ctx_names[c]; s += "."; s += sec_names[k];
                                                s += "="; s += std::to_string(n);
                                            }
                                        }
                                        return s; }()
                                   << " | bkt patch=" << gVkPerf.bkt_patch.load()
                                   << " range=" << gVkPerf.bkt_range.load()
                                   << " rec=" << gVkPerf.bkt_rec.load()
                                   << " skip=" << gVkPerf.bkt_skip.load()
                                   << " | mat=" << gVkPerf.mat_draws.load() << "/" << gVkPerf.mat_bindless_draws.load()
                                   << [](){ std::string s;
                                        static const char* cen_names[12] = {
                                            "m","am","ae","s","sm","se",
                                            "n","nm","ne","ns","nsm","nse" };
                                        for (U32 i = 0; i < 12; ++i) {
                                            const U64 tot = gVkPerf.mat_cen[i][0].load();
                                            if (tot == 0) continue;
                                            s += s.empty() ? " | matcen " : " ";
                                            s += cen_names[i];
                                            s += "="; s += std::to_string(tot);
                                            s += "/"; s += std::to_string(gVkPerf.mat_cen[i][1].load());
                                        }
                                        return s; }()
                                   << [](){ std::string s;
                                        static const char* sn[8] = { "am","fbm","gm","ab","amR","fbmR","gmR","opR" };
                                        for (U32 i = 0; i < 8; ++i) {
                                            const U64 sp = gVkPerf.shamdi[i][0].load();
                                            const U64 dy = gVkPerf.shamdi[i][1].load();
                                            const U64 wk = gVkPerf.shamdi[i][2].load();
                                            if (sp == 0 && dy == 0 && wk == 0) continue;
                                            s += s.empty() ? " | shamdi " : " ";
                                            s += sn[i];
                                            s += "="; s += std::to_string(sp);
                                            s += "/"; s += std::to_string(dy);
                                            s += "/"; s += std::to_string(wk);
                                        }
                                        return s; }()
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
                                        s += llformat(" dpurge=%llu",
                                                      (unsigned long long)gVkPerf.set1_dead_purge.load());
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
                                   << llformat(" | e3 rig=%.2f/%.2f/%.2f pal=%.2f/%.2f/%.2f",
                                        gVkPerf.e3_rig_us[0].load() / 1000.0,
                                        gVkPerf.e3_rig_us[1].load() / 1000.0,
                                        gVkPerf.e3_rig_us[2].load() / 1000.0,
                                        gVkPerf.e3_pal_us[0].load() / 1000.0,
                                        gVkPerf.e3_pal_us[1].load() / 1000.0,
                                        gVkPerf.e3_pal_us[2].load() / 1000.0)
                                   << " | tex enq=" << gVkPerf.tex_enq.load()
                                   << " pub=" << gVkPerf.tex_pub.load()
                                   << " fail=" << gVkPerf.tex_fail.load()
                                   << " dec=" << gVkPerf.tex_dec.load()
                                   << " floor=" << gVkPerf.tex_floor.load()
                                   << " strand=" << gVkPerf.tex_strand.load()
                                   << " stg_mb=" << (sOneShotStagingBytes.load() >> 20)
                                   << " | geo enq=" << gVkPerf.geo_enq.load()
                                   << " pub=" << gVkPerf.geo_pub.load()
                                   << " orph=" << gVkPerf.geo_orphan.load()
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
                                   << " | gup pub=" << (gVkPerf.gupd_us[0].load() / 1000.0)
                                   << " avp=" << (gVkPerf.gupd_us[1].load() / 1000.0)
                                   << " crt=" << (gVkPerf.gupd_us[2].load() / 1000.0)
                                   << " pq=" << (gVkPerf.gupd_us[3].load() / 1000.0)
                                   << " upg=" << (gVkPerf.gupd_us[4].load() / 1000.0)
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

    vbUploadSubmit();

    if (sUISceneSplit && sConsumerActiveThisFrame && isUISceneAsync())
    {
        PEJob cjob;
        cjob.is_frame = true;
        cjob.slot     = sFrameIndex;
        cjob.cmd      = sConsumerCommandBuffers[sFrameIndex];
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
        sFrameTimelineValue[sFrameIndex] = peEnqueue(std::move(cjob));

        PEJob pjob;
        pjob.is_frame = false;
        pjob.slot     = sFrameIndex;
        pjob.cmd      = sCommandBuffers[sFrameIndex];
        sProducerFencePending[sFrameIndex] = true;
        sProducerTimelineValue[sFrameIndex] = peEnqueue(std::move(pjob));

        if (sAsyncRenderSceneThisFrame)
        {
            vkEndCommandBuffer(sAsyncProducerCommandBuffer);
            PEJob apjob;
            apjob.is_frame          = false;
            apjob.is_async_producer = true;
            apjob.cmd               = sAsyncProducerCommandBuffer;
            sProdEnqMonoUs.store(vkMonoUs());
            sProdChecksSinceSubmit.store(0);
            sAsyncTimelineValue = peEnqueue(std::move(apjob));
            sAsyncProducerInFlight        = true;
            sAsyncProducerSubmitMonotonic = sMonotonicFrameCount;
            ++sAsyncProducerSubmitCount;
            sAsyncRenderSceneThisFrame    = false;
        }
    }
    else
    {
        PEJob job;
        job.is_frame = true;
        job.slot     = sFrameIndex;
        job.cmd      = sCommandBuffers[sFrameIndex];
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
        sFrameTimelineValue[sFrameIndex] = peEnqueue(std::move(job));
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

    waitTimeline(sFrameTimelineValue[sFrameIndex]);
    refreshCompletedWatermark();
    if (sProducerFencePending[sFrameIndex])
    {
        waitTimeline(sProducerTimelineValue[sFrameIndex]);
        sProducerFencePending[sFrameIndex] = false;
    }
    if (sAsyncProducerInFlight && sAsyncProducerRecordSlot == sFrameIndex)
    {
        waitTimeline(sAsyncTimelineValue);
    }
    sPESlotState[sFrameIndex].store(PE_SLOT_IDLE);
    vbUploadFrameReset();

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

    vbUploadSubmit();
    peSubmitBlocking(sCommandBuffers[sFrameIndex], VK_NULL_HANDLE, true);
    sFrameTimelineValue[sFrameIndex] = gpuTimelineValue();
    pollAsyncProducerCompletion();
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

bool isVulkanInitialized()
{
    return sInitialized;
}

void setVsyncEnabled(bool enabled)
{
    if (sVsyncEnabled.load() == enabled)
    {
        return;
    }
    sVsyncEnabled.store(enabled);
    sRecreateReasonMask.fetch_or(RECREATE_REASON_VSYNC_SETTING);
    sSwapchainRecreatePending = true;
}

void seedVsyncEnabled(bool enabled)
{
    sVsyncEnabled.store(enabled);
}

bool isInFrame()
{
    return sInFrame;
}

bool frameCanRecord()
{
    return sInFrame && (!sFrameWantsPresent || sImageAcquired);
}

VkDevice getDevice()
{
    return sDevice;
}

U32 getCurrentFrameIndex()
{
    return sFrameIndex;
}

static std::atomic<bool> sShadowRecordPhaseFlag{false};

void setShadowRecordPhase(bool active)
{
    sShadowRecordPhaseFlag.store(active, std::memory_order_relaxed);
}

bool isShadowRecordPhase()
{
    return sShadowRecordPhaseFlag.load(std::memory_order_relaxed);
}

U32 getMonotonicFrameCount()
{
    return sMonotonicFrameCount;
}

U32 getLastCompletedMonotonic()
{
    return sLastCompletedMonotonic;
}

void beginDynamicRendering(U32                               width,
                           U32                               height,
                           const DynamicRenderingAttachment* color_attachments,
                           U32                               color_count,
                           const DynamicRenderingAttachment* depth_attachment,
                           U32                               view_mask)
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
    rendering_info.viewMask             = view_mask;
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
    sSavedViewMask = view_mask;
    sSavedLayerCount = rendering_info.layerCount;

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

void resumeSavedPass()
{
    VkCommandBuffer rec_cmd = currentRecordCmd();
    if (!sInitialized || rec_cmd == VK_NULL_HANDLE || sInDynamicRendering)
    {
        return;
    }
    if (!(sSavedColorCount > 0 || sSavedHasDepth))
    {
        return;
    }

    VkRenderingAttachmentInfo color_resume[4] = {};
    for (U32 i = 0; i < sSavedColorCount && i < 4; ++i)
    {
        color_resume[i]        = sSavedColorInfos[i];
        color_resume[i].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    }
    VkRenderingAttachmentInfo depth_resume = sSavedDepthInfo;
    depth_resume.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

    VkRenderingInfo resume_info      = {};
    resume_info.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    resume_info.renderArea.offset    = { 0, 0 };
    resume_info.renderArea.extent    = { sSavedRenderWidth, sSavedRenderHeight };
    resume_info.layerCount           = sSavedLayerCount;
    resume_info.viewMask             = sSavedViewMask;
    resume_info.colorAttachmentCount = sSavedColorCount;
    resume_info.pColorAttachments    = (sSavedColorCount > 0 ? color_resume : nullptr);
    resume_info.pDepthAttachment     = (sSavedHasDepth ? &depth_resume : nullptr);
    resume_info.pStencilAttachment   = nullptr;

    vkCmdBeginRendering(rec_cmd, &resume_info);
    sInDynamicRendering = true;
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

bool isUploadWorkerThread()
{
    return t_cmdPool != VK_NULL_HANDLE;
}

bool isFrameInFlightVk(U32 monotonic_frame)
{
    return monotonic_frame > sLastCompletedMonotonic;
}

bool registerGpuUploadWorker()
{
    if (sDevice == VK_NULL_HANDLE || !sPERunning)
    {
        return false;
    }
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = sGraphicsQueueFamily;
    VkCommandPool pool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(sDevice, &pool_info, nullptr, &pool) != VK_SUCCESS)
    {
        return false;
    }
    t_cmdPool = pool;
    return true;
}

void unregisterGpuUploadWorker()
{
    const VkCommandPool pool = t_cmdPool;
    if (pool == VK_NULL_HANDLE)
    {
        return;
    }
    uint64_t max_tv = 0;
    {
        std::lock_guard<std::mutex> lk(sOneShotMutex);
        for (const PendingOneShotFree& e : sPendingOneShotFrees)
        {
            if (e.pool == pool && e.timeline_value > max_tv)
            {
                max_tv = e.timeline_value;
            }
        }
    }
    U32 spins = 0;
    while (gpuTimelineValue() < max_tv && !sReapForceAll)
    {
        tickOneShotFreeQueue();
        std::this_thread::sleep_for(std::chrono::microseconds(200));
        if (++spins > 25000)
        {
            LL_WARNS("Vulkan") << "unregisterGpuUploadWorker: drain timeout, pool left undestroyed" << LL_ENDL;
            t_cmdPool = VK_NULL_HANDLE;
            return;
        }
    }
    tickOneShotFreeQueue();
    {
        std::lock_guard<std::mutex> lk(sOneShotMutex);
        auto it = sRetiredByPool.find(pool);
        if (it != sRetiredByPool.end())
        {
            if (!it->second.empty())
            {
                vkFreeCommandBuffers(sDevice, pool, (U32)it->second.size(), it->second.data());
            }
            sRetiredByPool.erase(it);
        }
    }
    vkDestroyCommandPool(sDevice, pool, nullptr);
    t_cmdPool = VK_NULL_HANDLE;
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

void destroyPipelineVk(VkPipeline pipeline)
{
    noteDeferredEnqueueThread();
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
    noteDeferredEnqueueThread();
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
    noteDeferredEnqueueThread();
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
    noteDeferredEnqueueThread();
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

bool isImageViewCurrentAttachment(VkImageView view)
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

bool isImageViewActivePassAttachment(VkImageView view)
{
    if (!sInDynamicRendering || view == VK_NULL_HANDLE)
    {
        return false;
    }
    if (sSavedHasDepth && sSavedDepthInfo.imageView == view)
    {
        return sSavedDepthInfo.imageLayout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
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

U32 currentRenderViewMask()
{
    return sSavedViewMask;
}

VkImageView currentRenderDepthView()
{
    return sSavedHasDepth ? sSavedDepthInfo.imageView : VK_NULL_HANDLE;
}

U32 currentRenderColorCount()
{
    return sSavedColorCount;
}

VkImageView currentRenderColorView(U32 i)
{
    return (i < sSavedColorCount && i < 4) ? sSavedColorInfos[i].imageView : VK_NULL_HANDLE;
}

} // namespace LLVKLoader
