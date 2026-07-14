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
#include <atomic>
#include <chrono>
#include <mutex>
#include <set>
#include <queue>
#include <pthread.h>

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
    bool           sInFrame            = false;

    bool sInDynamicRendering = false;

    S32 sVkRenderViewport[4] = {0, 0, 0, 0};

    U32 sCurrentRenderAreaHeight = 0;

    bool sScissorEnabled     = false;
    S32  sScissorRectGL[4]   = {0, 0, 0, 0};

    VkRenderingAttachmentInfo sSavedColorInfos[4] = {};
    U32                       sSavedColorCount    = 0;
    VkRenderingAttachmentInfo sSavedDepthInfo     = {};
    bool                      sSavedHasDepth      = false;
    U32                       sSavedRenderWidth   = 0;
    U32                       sSavedRenderHeight  = 0;

    VmaAllocator sAllocator = VK_NULL_HANDLE;
    constexpr VkDeviceSize PERFRAME_UBO_SIZE         = sizeof(PerFrameMatrixUBO);
    constexpr VkDeviceSize TEXTURE_UBO_SIZE          = sizeof(TextureMatrixUBO);
    constexpr VkDeviceSize MATRIX_RING_SLOT_SIZE     = PERFRAME_UBO_SIZE + TEXTURE_UBO_SIZE;
    constexpr VkDeviceSize SHADOW_UBO_OFFSET         = 512;
    constexpr VkDeviceSize SHADOW_UBO_SIZE           = sizeof(ShadowParams_PerShaderBind);
    constexpr VkDeviceSize PBRMATERIAL_UBO_OFFSET    = 768;
    constexpr VkDeviceSize PBRMATERIAL_UBO_SIZE      = sizeof(PBRMaterial_PerMaterial);
    constexpr VkDeviceSize PREVIEWAMBIENT_UBO_OFFSET = 1024;
    constexpr VkDeviceSize PREVIEWAMBIENT_UBO_SIZE   = sizeof(PreviewAmbient_PerShaderBind);
    constexpr VkDeviceSize DRAWCOLOR_UBO_OFFSET      = 1280;
    constexpr VkDeviceSize DRAWCOLOR_UBO_SIZE        = sizeof(DrawColor_PerShaderBind);
    constexpr VkDeviceSize STARTIME_UBO_OFFSET       = 1792;
    constexpr VkDeviceSize STARTIME_UBO_SIZE         = sizeof(StarTime_PerShaderBind);
    constexpr VkDeviceSize AVATAR_VELOCITY_PALETTE_UBO_OFFSET = 2048;
    constexpr VkDeviceSize AVATAR_VELOCITY_PALETTE_UBO_SIZE   = sizeof(AvatarVelocityPalette_PerShaderBind);
    constexpr VkDeviceSize CLIPPLANE_UBO_OFFSET      = 2816;
    constexpr VkDeviceSize CLIPPLANE_UBO_SIZE        = sizeof(ClipPlane_PerShaderBind);
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
    std::vector<MatrixRingChunk> sMatrixRingChunks[FRAMES_IN_FLIGHT];
    std::vector<VkDescriptorSet> sPerFrameRingSets[FRAMES_IN_FLIGHT];
    std::vector<VkDescriptorPool> sPerFrameRingPools;
    U32   sRingPoolUsedInLast = 0;
    U32   sMatrixRingUsedThisFrame[FRAMES_IN_FLIGHT] = { 0, 0, 0 };
    U32   sMatrixRingCurrentSlot[FRAMES_IN_FLIGHT]   = { 0, 0, 0 };
    bool  sMatrixRingHasCurrent[FRAMES_IN_FLIGHT]    = { false, false, false };
    float sMatrixRingCurrentProj[FRAMES_IN_FLIGHT][16] = {};
    float sMatrixRingCurrentTexmat[FRAMES_IN_FLIGHT][64] = {};

    VkPipeline       sLastBoundGraphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout sLastDescLayout   = VK_NULL_HANDLE;
    VkDescriptorSet  sLastDescSet0     = VK_NULL_HANDLE;
    VkDescriptorSet  sLastDescSet1     = VK_NULL_HANDLE;
    U32              sLastDescDynCount = 0;
    U32              sLastDescOffsets[LLGLSLShader::MAX_VK_DYNAMIC_BINDINGS] = {};
    VkPipelineLayout sLastMvLayout     = VK_NULL_HANDLE;
    float            sLastMv[16]       = {};
    VkViewport       sLastViewport     = {};
    VkRect2D         sLastScissor      = {};
    bool             sViewportScissorValid = false;

    VkSampler             sStandardLinearSampler                  = VK_NULL_HANDLE;

    std::unordered_map<U32, VkSampler> sSamplerCache;
    bool                     sSamplerAnisotropyEnabled               = false;
    float                    sMaxSamplerAnisotropy                   = 1.0f;
    float                    sMaxLineWidth                           = 1.0f;
    bool                     sGeometryShaderEnabled                  = false;
    bool                     sImageCubeArrayEnabled                  = false;

    VkPhysicalDeviceProperties sPhysicalDeviceProperties             = {};
    std::string                sDriverName;
    std::string                sDriverInfo;
    U32                        sDeviceLocalMemoryMB                  = 0;

    bool                     sProvokingVertexLastEnabled             = false;

    constexpr U32                  SCENE_PER_DRAW_POOL_GROWTH_SETS     = 50000;
    constexpr U32                  SCENE_PER_DRAW_POOL_GROWTH_SAMPLERS = 250000;
    constexpr U32                  SCENE_PER_DRAW_POOL_GROWTH_UBOS     = 50000;
    std::vector<VkDescriptorPool>  sScenePerDrawDescriptorPools;

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
        VkDescriptorSet                              sets[FRAMES_IN_FLIGHT];
        std::list<ScenePerDrawCacheKey>::iterator    lru_pos;
        U32                                          last_used_monotonic_frame;
        U32                                          pool_index;
    };
    std::unordered_map<ScenePerDrawCacheKey, ScenePerDrawCacheEntry, ScenePerDrawCacheKeyHash> sScenePerDrawCache;
    std::list<ScenePerDrawCacheKey>                                                            sScenePerDrawLRUOrder;

    struct ScenePerDrawDeferredFreeEntry
    {
        VkDescriptorSet sets[FRAMES_IN_FLIGHT];
        U32             pool_index;
        U32             enqueue_frame;
    };
    std::vector<ScenePerDrawDeferredFreeEntry> sScenePerDrawDeferredFree;


    VkBuffer              sSharedWindlightHDRUBO                  = VK_NULL_HANDLE;
    void*                 sSharedWindlightHDRUBOAllocation        = nullptr;
    void*                 sSharedWindlightHDRUBOMapped            = nullptr;
    VkBuffer              sSharedWindlightLightUBO                = VK_NULL_HANDLE;
    void*                 sSharedWindlightLightUBOAllocation      = nullptr;
    void*                 sSharedWindlightLightUBOMapped          = nullptr;
    VkBuffer              sSharedTonemapUtilFUBO                  = VK_NULL_HANDLE;
    void*                 sSharedTonemapUtilFUBOAllocation        = nullptr;
    void*                 sSharedTonemapUtilFUBOMapped            = nullptr;
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
    LLVK_SHARED_UBO_RING_STORAGE(ReflectionProbeF)
    LLVK_SHARED_UBO_RING_STORAGE(SSRUtil)
    LLVK_SHARED_UBO_RING_STORAGE(Lights)
    LLVK_SHARED_UBO_RING_STORAGE(LightsSpecular)
    LLVK_SHARED_UBO_RING_STORAGE(PbrTerrainF)
    LLVK_SHARED_UBO_RING_STORAGE(PbrTerrain)
    #undef LLVK_SHARED_UBO_RING_STORAGE
    VkBuffer              sSharedSMAABlendWeightsFUBO             = VK_NULL_HANDLE;
    void*                 sSharedSMAABlendWeightsFUBOAllocation   = nullptr;
    void*                 sSharedSMAABlendWeightsFUBOMapped       = nullptr;

    struct PerDrawUBOArena
    {
        VkBuffer     buffer           = VK_NULL_HANDLE;
        void*        allocation       = nullptr;
        void*        mapped           = nullptr;
        VkDeviceSize capacity         = 0;
        VkDeviceSize cursor           = 0;
        U64          frame            = ~0ull;
        VkDeviceSize pending_capacity = 0;
    };
    PerDrawUBOArena sPerDrawUBOArena[FRAMES_IN_FLIGHT];
    constexpr VkDeviceSize PER_DRAW_UBO_ARENA_INITIAL = 4 * 1024 * 1024;

    U32 sFrameIndex = 0;

    VkCommandBuffer sCommandBuffers[FRAMES_IN_FLIGHT] = {
        VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE
    };
    VkFence sInFlightFences[FRAMES_IN_FLIGHT] = {
        VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE
    };

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

    bool sSwapchainRecreatePending = false;
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
        VkBuffer        buffer     = VK_NULL_HANDLE;
        VmaAllocation   allocation = VK_NULL_HANDLE;
    };
    std::vector<PendingOneShotFree> sPendingOneShotFrees;
    std::vector<VkFence>            sSubmitFencePool;

    U32 sLastCompletedMonotonic = 0;
    U32 sFrameSubmittedMonotonic[FRAMES_IN_FLIGHT] = { 0, 0, 0 };

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
        if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            LL_WARNS("VulkanValidation") << "[VK-ERROR][" << type_str << "][" << id << "] " << msg << LL_ENDL;
        }
        else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            LL_WARNS("VulkanValidation") << "[VK-WARN][" << type_str << "][" << id << "] " << msg << LL_ENDL;
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
            }
        }
        if (device_fault_supported)
        {
            device_extensions.push_back("VK_EXT_device_fault");
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
                if (sProvokingVertexLastEnabled)
                {
                    pv_features_enable.pNext = &hqr_features_enable;
                }
                else
                {
                    dr_features_enable.pNext = &hqr_features_enable;
                }
            }
        }

        VkDeviceCreateInfo device_info = {};
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.pNext = &dr_features_enable;
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

        volkLoadDevice(sDevice);
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

            VkSubmitInfo si = {};
            si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            si.commandBufferCount = 1;
            si.pCommandBuffers    = &one_cmd;
            vkQueueSubmit(sGraphicsQueue, 1, &si, VK_NULL_HANDLE);
            vkQueueWaitIdle(sGraphicsQueue);

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
        VkSubmitInfo si = {};
        si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.commandBufferCount = 1;
        si.pCommandBuffers    = &one_cmd;
        vkQueueSubmit(sGraphicsQueue, 1, &si, VK_NULL_HANDLE);
        vkQueueWaitIdle(sGraphicsQueue);
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
        VkSubmitInfo si = {};
        si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.commandBufferCount = 1;
        si.pCommandBuffers    = &one_cmd;
        vkQueueSubmit(sGraphicsQueue, 1, &si, VK_NULL_HANDLE);
        vkQueueWaitIdle(sGraphicsQueue);
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
        VkSubmitInfo si = {};
        si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.commandBufferCount = 1;
        si.pCommandBuffers    = &one_cmd;
        vkQueueSubmit(sGraphicsQueue, 1, &si, VK_NULL_HANDLE);
        vkQueueWaitIdle(sGraphicsQueue);
        vkFreeCommandBuffers(sDevice, sCommandPool, 1, &one_cmd);

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
        VkDescriptorSetLayoutBinding bindings[10] = {};
        bindings[0].binding         = 0;
        bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[1].binding         = 1;
        bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[2].binding         = 2;
        bindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

        bindings[3].binding         = 3;
        bindings[3].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[3].descriptorCount = 1;
        bindings[3].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

        bindings[4].binding         = 4;
        bindings[4].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[4].descriptorCount = 1;
        bindings[4].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

        bindings[5].binding         = 5;
        bindings[5].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[5].descriptorCount = 1;
        bindings[5].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[6].binding         = 7;
        bindings[6].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[6].descriptorCount = 1;
        bindings[6].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[7].binding         = 8;
        bindings[7].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[7].descriptorCount = 1;
        bindings[7].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

        bindings[8].binding         = 9;
        bindings[8].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[8].descriptorCount = 1;
        bindings[8].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[9].binding         = 10;
        bindings[9].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[9].descriptorCount = 1;
        bindings[9].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo info = {};
        info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 10;
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
        const BindSpec specs[10] = {
            { 0,  matrixBuf,                 matrixOffset,                       PERFRAME_UBO_SIZE },
            { 1,  matrixBuf,                 matrixOffset + PERFRAME_UBO_SIZE,   TEXTURE_UBO_SIZE },
            { 2,  sPerFrameUboBuffer[frame], SHADOW_UBO_OFFSET,                  SHADOW_UBO_SIZE },
            { 3,  sPerFrameUboBuffer[frame], PBRMATERIAL_UBO_OFFSET,             PBRMATERIAL_UBO_SIZE },
            { 4,  sPerFrameUboBuffer[frame], PREVIEWAMBIENT_UBO_OFFSET,          PREVIEWAMBIENT_UBO_SIZE },
            { 5,  sPerFrameUboBuffer[frame], DRAWCOLOR_UBO_OFFSET,               DRAWCOLOR_UBO_SIZE },
            { 7,  sPerFrameUboBuffer[frame], STARTIME_UBO_OFFSET,                STARTIME_UBO_SIZE },
            { 8,  sPerFrameUboBuffer[frame], AVATAR_VELOCITY_PALETTE_UBO_OFFSET, AVATAR_VELOCITY_PALETTE_UBO_SIZE },
            { 9,  sPerFrameUboBuffer[frame], CLIPPLANE_UBO_OFFSET,               CLIPPLANE_UBO_SIZE },
            { 10, sPerFrameUboBuffer[frame], GLOWCOMBINE_UBO_OFFSET,             GLOWCOMBINE_UBO_SIZE },
        };

        VkDescriptorBufferInfo infos[10]  = {};
        VkWriteDescriptorSet   writes[10] = {};
        for (U32 i = 0; i < 10; ++i)
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
        vkUpdateDescriptorSets(sDevice, 10, writes, 0, nullptr);
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
        while (sPerFrameRingSets[frame].size() <= (size_t)slot)
        {
            const U32 new_slot = (U32)sPerFrameRingSets[frame].size();
            const U32 chunk    = new_slot / MATRIX_RING_CHUNK_SLOTS;
            const U32 within   = new_slot % MATRIX_RING_CHUNK_SLOTS;

            while (sMatrixRingChunks[frame].size() <= (size_t)chunk)
            {
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

        out_buffer     = buffer;
        out_allocation = reinterpret_cast<void*>(allocation);
        if (out_mapped)
        {
            *out_mapped = info.pMappedData;
        }
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
static void           tickDeferredQueryReleaseQueue();
static bool           submitOneShotVk(VkCommandBuffer cmd, VkBuffer staging_buffer, VmaAllocation staging_allocation);
static void           tickOneShotFreeQueue();
static void           shutdownSurface();
extern float          sCurrentModelviewMatrix[16];

bool initVulkan()
{
    if (sInitialized)
    {
        return true;
    }

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

    if (!createDefaultFallbackCubeArrayImage() || !createDefaultFallbackCubeImage() || !createDefaultFallback3DImage())
    {
        shutdownVulkan();
        return false;
    }

    if (!createPerFrameDescriptorSetLayout() ||
        !createPerFrameUbos()                ||
        !createPerFrameDescriptorSets())
    {
        shutdownVulkan();
        return false;
    }

    {
        VkDescriptorPool initial_pool = VK_NULL_HANDLE;
        if (createScenePerDrawDescriptorPool(&initial_pool))
        {
            sScenePerDrawDescriptorPools.push_back(initial_pool);
        }
    }

    if (!createStandardSampler())
    {
        shutdownVulkan();
        return false;
    }

    if (!createSyncObjects())
    {
        shutdownVulkan();
        return false;
    }

    sInitialized = true;
    return true;
}

void shutdownVulkan()
{
    if (sDevice != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(sDevice);

        if (sAllocator != VK_NULL_HANDLE)
        {
            for (auto& pending : sPendingBufferFrees)
            {
                vmaDestroyBuffer(sAllocator, pending.buffer, pending.allocation);
            }
        }
        sPendingBufferFrees.clear();
        for (auto& pending : sPendingImageFrees)
        {
            if (pending.view != VK_NULL_HANDLE)
            {
                vkDestroyImageView(sDevice, pending.view, nullptr);
            }
            if (pending.image != VK_NULL_HANDLE && sAllocator != VK_NULL_HANDLE)
            {
                vmaDestroyImage(sAllocator, pending.image, pending.allocation);
            }
        }
        sPendingImageFrees.clear();
        for (auto& pending : sPendingObjectFrees)
        {
            if (pending.pipeline != VK_NULL_HANDLE)
            {
                vkDestroyPipeline(sDevice, pending.pipeline, nullptr);
            }
            if (pending.shader_module != VK_NULL_HANDLE)
            {
                vkDestroyShaderModule(sDevice, pending.shader_module, nullptr);
            }
            if (pending.pipeline_layout != VK_NULL_HANDLE)
            {
                vkDestroyPipelineLayout(sDevice, pending.pipeline_layout, nullptr);
            }
            if (pending.descriptor_set_layout != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorSetLayout(sDevice, pending.descriptor_set_layout, nullptr);
            }
        }
        sPendingObjectFrees.clear();
        sPendingOcclusionQueryReleases.clear();
        for (auto& pending : sPendingOneShotFrees)
        {
            if (sCommandPool != VK_NULL_HANDLE && pending.cmd != VK_NULL_HANDLE)
            {
                vkFreeCommandBuffers(sDevice, sCommandPool, 1, &pending.cmd);
            }
            if (sAllocator != VK_NULL_HANDLE &&
                (pending.buffer != VK_NULL_HANDLE || pending.allocation != VK_NULL_HANDLE))
            {
                vmaDestroyBuffer(sAllocator, pending.buffer, pending.allocation);
            }
            if (pending.fence != VK_NULL_HANDLE)
            {
                vkDestroyFence(sDevice, pending.fence, nullptr);
            }
        }
        sPendingOneShotFrees.clear();
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
            sMatrixRingUsedThisFrame[frame] = 0;
            sMatrixRingCurrentSlot[frame]   = 0;
            sMatrixRingHasCurrent[frame]    = false;
        }

        for (VkDescriptorPool pool : sScenePerDrawDescriptorPools)
        {
            if (pool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(sDevice, pool, nullptr);
            }
        }
        sScenePerDrawDescriptorPools.clear();
        sScenePerDrawCache.clear();
        sScenePerDrawLRUOrder.clear();
        sScenePerDrawDeferredFree.clear();

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
        destroy_shared_ubo(sSharedWindlightHDRUBO,      sSharedWindlightHDRUBOAllocation,      sSharedWindlightHDRUBOMapped);
        destroy_shared_ubo(sSharedWindlightLightUBO,    sSharedWindlightLightUBOAllocation,    sSharedWindlightLightUBOMapped);
        destroy_shared_ubo(sSharedTonemapUtilFUBO,      sSharedTonemapUtilFUBOAllocation,      sSharedTonemapUtilFUBOMapped);
        destroy_shared_ubo(sSharedSMAABlendWeightsFUBO, sSharedSMAABlendWeightsFUBOAllocation, sSharedSMAABlendWeightsFUBOMapped);
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
            LLVK_SHARED_UBO_RING_TEARDOWN(ReflectionProbeF)
            LLVK_SHARED_UBO_RING_TEARDOWN(SSRUtil)
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
            arena = PerDrawUBOArena();
        }
        if (sPerFrameDescriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(sDevice, sPerFrameDescriptorSetLayout, nullptr);
            sPerFrameDescriptorSetLayout = VK_NULL_HANDLE;
        }

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

    LLGLSLShader::sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;
    LLGLSLShader::sCurPerCallVkOffsetsDirty  = true;

    sLastBoundGraphicsPipeline = VK_NULL_HANDLE;
    sLastDescLayout            = VK_NULL_HANDLE;
    sLastDescSet0              = VK_NULL_HANDLE;
    sLastDescSet1              = VK_NULL_HANDLE;
    sLastDescDynCount          = 0;
    sLastMvLayout              = VK_NULL_HANDLE;
    sViewportScissorValid      = false;

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

    sFrameIndex = (sFrameIndex + 1) % FRAMES_IN_FLIGHT;

    if (sInFlightFences[sFrameIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(sDevice, 1, &sInFlightFences[sFrameIndex],
                                            VK_TRUE, UINT64_MAX);
        vkResetFences(sDevice, 1, &sInFlightFences[sFrameIndex]);
        if (sFrameSubmittedMonotonic[sFrameIndex] > sLastCompletedMonotonic)
        {
            sLastCompletedMonotonic = sFrameSubmittedMonotonic[sFrameIndex];
        }
    }

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
        VkResult acquire_res = vkAcquireNextImageKHR(sDevice, sSwapchain, UINT64_MAX,
                                                      sImageAvailableSemaphores[sFrameIndex],
                                                      VK_NULL_HANDLE,
                                                      &sAcquiredImageIndex);
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

    tickDeferredBufferFreeQueue();
    tickDeferredImageFreeQueue();
    tickDeferredObjectFreeQueue();
    tickDeferredQueryReleaseQueue();
    tickOneShotFreeQueue();

    tickScenePerDrawDescriptorCache();

    sInFrame = true;

    if (sImageAcquired &&
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
U32 gVkPerfPassTag = 0;
U32 gVkPerfShadowMapIndex = 0;

bool endFrame()
{
    if (!sInitialized || !sInFrame)
    {
        return false;
    }

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
                const U64 draws = gVkPerf.desc_bind + gVkPerf.desc_skip;
                LL_INFOS("VkPerf") << "frames=" << frames
                                   << " fps=" << ((F64)frames / elapsed)
                                   << " avg_ms=" << (elapsed * 1000.0 / (F64)frames)
                                   << " draws/f=" << (draws / frames)
                                   << " | emit/skip: pipe " << gVkPerf.pipe_bind << "/" << gVkPerf.pipe_skip
                                   << " desc " << gVkPerf.desc_bind << "/" << gVkPerf.desc_skip
                                   << " push " << gVkPerf.mv_push << "/" << gVkPerf.mv_skip
                                   << " vp " << gVkPerf.vp_set << "/" << gVkPerf.vp_skip
                                   << " | set build=" << gVkPerf.set_build
                                   << " reuse=" << gVkPerf.set_reuse
                                   << " populate=" << gVkPerf.populate
                                   << " | syncmat " << gVkPerf.syncmat_build << "/" << gVkPerf.syncmat_call
                                   << " | pass scene=" << gVkPerf.draws_pass[0]
                                   << " shadow=" << gVkPerf.draws_pass[1]
                                   << " occl=" << gVkPerf.draws_pass[2]
                                   << " probe=" << gVkPerf.draws_pass[3]
                                   << " hero=" << gVkPerf.draws_pass[4]
                                   << " | shmap " << gVkPerf.draws_shadow_map[0]
                                   << "/" << gVkPerf.draws_shadow_map[1]
                                   << "/" << gVkPerf.draws_shadow_map[2]
                                   << "/" << gVkPerf.draws_shadow_map[3]
                                   << " spot " << gVkPerf.draws_shadow_map[4]
                                   << "/" << gVkPerf.draws_shadow_map[5]
                                   << " culled=" << gVkPerf.shadow_cull
                                   << " rigged=" << gVkPerf.shadow_rigged
                                   << LL_ENDL;
            }
            gVkPerf = VkPerfCounters();
            s_last_emit  = now;
            s_last_frame = sMonotonicFrameCount;
        }
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

    VkResult result = vkEndCommandBuffer(sCommandBuffers[sFrameIndex]);
    if (result != VK_SUCCESS)
    {
        sInFrame = false;
        sImageAcquired = false;
        return false;
    }

    {
        VkSubmitInfo submit_info = {};
        submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &sCommandBuffers[sFrameIndex];

        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        if (sImageAcquired)
        {
            submit_info.waitSemaphoreCount   = 1;
            submit_info.pWaitSemaphores      = &sImageAvailableSemaphores[sFrameIndex];
            submit_info.pWaitDstStageMask    = &wait_stage;
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores    = &sRenderFinishedSemaphores[sFrameIndex];

        }
        else
        {
            submit_info.waitSemaphoreCount   = 0;
            submit_info.pWaitSemaphores      = nullptr;
            submit_info.pWaitDstStageMask    = nullptr;
            submit_info.signalSemaphoreCount = 0;
            submit_info.pSignalSemaphores    = nullptr;

        }

        VkResult submit_res = vkQueueSubmit(sGraphicsQueue, 1, &submit_info,
                                            sInFlightFences[sFrameIndex]);
        if (submit_res != VK_SUCCESS)
        {
            sInFrame = false;
            sImageAcquired = false;
            return false;
        }
        sFrameSubmittedMonotonic[sFrameIndex] = sMonotonicFrameCount;
    }

    if (sImageAcquired && sSwapchain != VK_NULL_HANDLE)
    {
        VkPresentInfoKHR present_info = {};
        present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores    = &sRenderFinishedSemaphores[sFrameIndex];
        present_info.swapchainCount     = 1;
        present_info.pSwapchains        = &sSwapchain;
        present_info.pImageIndices      = &sAcquiredImageIndex;
        present_info.pResults           = nullptr;


        VkResult present_res = vkQueuePresentKHR(sGraphicsQueue, &present_info);
        if (present_res == VK_ERROR_OUT_OF_DATE_KHR)
        {
            sSwapchainRecreatePending = true;
        }
        else if (present_res == VK_SUBOPTIMAL_KHR)
        {
            sSwapchainRecreatePending = true;
        }
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

    if (sInFlightFences[sFrameIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(sDevice, 1, &sInFlightFences[sFrameIndex],
                                            VK_TRUE, UINT64_MAX);
        vkResetFences(sDevice, 1, &sInFlightFences[sFrameIndex]);
        if (sFrameSubmittedMonotonic[sFrameIndex] > sLastCompletedMonotonic)
        {
            sLastCompletedMonotonic = sFrameSubmittedMonotonic[sFrameIndex];
        }
    }

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

    VkSubmitInfo submit_info = {};
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &sCommandBuffers[sFrameIndex];

    vkQueueSubmit(sGraphicsQueue, 1, &submit_info, sInFlightFences[sFrameIndex]);
    vkQueueWaitIdle(sGraphicsQueue);
    sFrameSubmittedMonotonic[sFrameIndex] = sMonotonicFrameCount;
    if (sMonotonicFrameCount > sLastCompletedMonotonic)
    {
        sLastCompletedMonotonic = sMonotonicFrameCount;
    }

    sInFrame = false;
}

VkCommandBuffer getCurrentCommandBuffer()
{
    return sInFrame ? sCommandBuffers[sFrameIndex] : VK_NULL_HANDLE;
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
        if (e.enqueue_frame <= sLastCompletedMonotonic)
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
                                     VkDescriptorSet*            out_set)
{
    if (!out_set)
    {
        return false;
    }
    *out_set = VK_NULL_HANDLE;

    if (!sInitialized || sScenePerDrawDescriptorPools.empty() ||
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

    auto cache_it = sScenePerDrawCache.find(key);
    if (cache_it != sScenePerDrawCache.end())
    {
        sScenePerDrawLRUOrder.erase(cache_it->second.lru_pos);
        sScenePerDrawLRUOrder.push_back(key);
        cache_it->second.lru_pos                   = std::prev(sScenePerDrawLRUOrder.end());
        cache_it->second.last_used_monotonic_frame = sMonotonicFrameCount;
        *out_set = cache_it->second.sets[getCurrentFrameIndex()];
        return true;
    }

    constexpr size_t SCENE_PER_DRAW_CACHE_MAX_ENTRIES = 50000;
    if (sScenePerDrawCache.size() >= SCENE_PER_DRAW_CACHE_MAX_ENTRIES)
    {
        U32 evicted = 0;
        for (auto lru_it = sScenePerDrawLRUOrder.begin();
             lru_it != sScenePerDrawLRUOrder.end() && evicted < 64; )
        {
            auto cit = sScenePerDrawCache.find(*lru_it);
            if (cit == sScenePerDrawCache.end())
            {
                lru_it = sScenePerDrawLRUOrder.erase(lru_it);
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
                sScenePerDrawDeferredFree.push_back(deferred);

                sScenePerDrawCache.erase(cit);
                lru_it = sScenePerDrawLRUOrder.erase(lru_it);
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
        return vkAllocateDescriptorSets(sDevice, &alloc_info, new_sets) == VK_SUCCESS;
    };

    auto try_alloc_any_pool = [&]() -> bool
    {
        for (U32 pi = 0; pi < sScenePerDrawDescriptorPools.size(); ++pi)
        {
            if (try_alloc_in_pool(sScenePerDrawDescriptorPools[pi]))
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
        for (auto lru_it = sScenePerDrawLRUOrder.begin(); lru_it != sScenePerDrawLRUOrder.end(); )
        {
            auto cit = sScenePerDrawCache.find(*lru_it);
            if (cit == sScenePerDrawCache.end())
            {
                lru_it = sScenePerDrawLRUOrder.erase(lru_it);
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
                sScenePerDrawDeferredFree.push_back(deferred);

                sScenePerDrawCache.erase(cit);
                lru_it = sScenePerDrawLRUOrder.erase(lru_it);
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
            sScenePerDrawDescriptorPools.push_back(new_pool);
            if (!try_alloc_in_pool(new_pool))
            {
                return false;
            }
            alloc_pool_index = (U32)(sScenePerDrawDescriptorPools.size() - 1);
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
        sbuf = VK_NULL_HANDLE; smap = nullptr;
        if (getSharedSSRUtilUBO(sbuf, smap))        add_shared_ubo(49, sbuf, sizeof(SSRUtil_PerProgramBind));

        if (shared_count > 0)
        {
            vkUpdateDescriptorSets(sDevice, shared_count, shared_ubo_writes, 0, nullptr);
        }
    }

    sScenePerDrawLRUOrder.push_back(key);
    ScenePerDrawCacheEntry entry = {};
    for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        entry.sets[i] = new_sets[i];
    }
    entry.lru_pos                   = std::prev(sScenePerDrawLRUOrder.end());
    entry.last_used_monotonic_frame = sMonotonicFrameCount;
    entry.pool_index                = alloc_pool_index;
    sScenePerDrawCache.emplace(key, entry);


    *out_set = entry.sets[getCurrentFrameIndex()];
    return true;
}

void tickScenePerDrawDescriptorCache()
{
    if (!sInitialized)
    {
        return;
    }
    size_t w = 0;
    const size_t n = sScenePerDrawDeferredFree.size();
    for (size_t r = 0; r < n; ++r)
    {
        ScenePerDrawDeferredFreeEntry& e = sScenePerDrawDeferredFree[r];
        if (e.enqueue_frame <= sLastCompletedMonotonic)
        {
            VkDescriptorPool target_pool = (e.pool_index < sScenePerDrawDescriptorPools.size())
                                               ? sScenePerDrawDescriptorPools[e.pool_index]
                                               : VK_NULL_HANDLE;
            if (target_pool != VK_NULL_HANDLE)
            {
                vkFreeDescriptorSets(sDevice, target_pool, FRAMES_IN_FLIGHT, e.sets);
            }
        }
        else
        {
            if (w != r)
            {
                sScenePerDrawDeferredFree[w] = e;
            }
            ++w;
        }
    }
    sScenePerDrawDeferredFree.resize(w);
}

U32 getCurrentFrameIndex()
{
    return sFrameIndex;
}

U32 getMonotonicFrameCount()
{
    return sMonotonicFrameCount;
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

    const U32 slot = sMatrixRingUsedThisFrame[f];
    if (!ensurePerFrameMatrixSlot(f, slot))
    {
        return;
    }
    sMatrixRingUsedThisFrame[f] = slot + 1;

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

void writeCurrentClipPlaneUBO(const ClipPlane_PerShaderBind& data)
{
    if (!sInitialized || sPerFrameUboMapped[sFrameIndex] == nullptr)
    {
        return;
    }
    std::memcpy(static_cast<U8*>(sPerFrameUboMapped[sFrameIndex]) + CLIPPLANE_UBO_OFFSET,
                &data,
                sizeof(ClipPlane_PerShaderBind));

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

float sCurrentModelviewMatrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

void pushCurrentModelviewMatrix(const float modelview_matrix[16])
{
    if (!sInitialized || !sInFrame || sCommandBuffers[sFrameIndex] == VK_NULL_HANDLE)
    {
        return;
    }
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
    if (!sInitialized || !sInFrame || sCommandBuffers[sFrameIndex] == VK_NULL_HANDLE)
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
        vkCmdEndRendering(sCommandBuffers[sFrameIndex]);
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

    vkCmdBeginRendering(sCommandBuffers[sFrameIndex], &rendering_info);
    sInDynamicRendering = true;
}

void endDynamicRendering()
{
    if (!sInitialized || !sInFrame || sCommandBuffers[sFrameIndex] == VK_NULL_HANDLE ||
        !sInDynamicRendering)
    {
        return;
    }

    vkCmdEndRendering(sCommandBuffers[sFrameIndex]);
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

bool createVertexBufferVk(U32       size_bytes,
                          VkBuffer& out_buffer,
                          void*&    out_allocation,
                          void**    out_mapped)
{
    return createBufferVkImpl(size_bytes,
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                              out_buffer, out_allocation, out_mapped, true);
}

bool createIndexBufferVk(U32       size_bytes,
                         VkBuffer& out_buffer,
                         void*&    out_allocation,
                         void**    out_mapped)
{
    return createBufferVkImpl(size_bytes,
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                              out_buffer, out_allocation, out_mapped, true);
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
        }
    }
    return (a.buffer != VK_NULL_HANDLE && a.mapped != nullptr);
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
        return false;
    }
    PerDrawUBOArena& a = sPerDrawUBOArena[f];
    if (!ensurePerDrawUBOArenaCurrent(a))
    {
        return false;
    }
    VkDeviceSize align = sPhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
    if (align < 16)
    {
        align = 16;
    }
    const VkDeviceSize off = (a.cursor + align - 1) & ~(align - 1);
    if (off + size_bytes > a.capacity)
    {
        a.pending_capacity = llmax(a.pending_capacity, llmax(a.capacity * 2, off + size_bytes));
        return false;
    }
    a.cursor   = off + size_bytes;
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

#define LLVK_SHARED_UBO_GETTER(BindName, StructType, StorageBuf, StorageAlloc, StorageMapped, BindingNumber) \
    bool getShared##BindName##UBO(VkBuffer& out_buffer, void*& out_mapped)                                   \
    {                                                                                                       \
        if (!sInitialized) return false;                                                                    \
        if (StorageBuf == VK_NULL_HANDLE)                                                                   \
        {                                                                                                   \
            if (!createBufferVkImpl(sizeof(StructType),                                                     \
                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,                                     \
                                    StorageBuf,                                                             \
                                    StorageAlloc,                                                           \
                                    &StorageMapped))                                                        \
            {                                                                                               \
                return false;                                                                               \
            }                                                                                               \
        }                                                                                                   \
        out_buffer = StorageBuf;                                                                            \
        out_mapped = StorageMapped;                                                                         \
        return true;                                                                                        \
    }

LLVK_SHARED_UBO_GETTER(WindlightHDR,        WindlightHDR_PerProgramBind,        sSharedWindlightHDRUBO,        sSharedWindlightHDRUBOAllocation,        sSharedWindlightHDRUBOMapped,        10)
LLVK_SHARED_UBO_GETTER(WindlightLight,      WindlightLight_PerProgramBind,      sSharedWindlightLightUBO,      sSharedWindlightLightUBOAllocation,      sSharedWindlightLightUBOMapped,      11)
LLVK_SHARED_UBO_GETTER(TonemapUtilF,         TonemapUtilF_PerProgramBind,         sSharedTonemapUtilFUBO,         sSharedTonemapUtilFUBOAllocation,         sSharedTonemapUtilFUBOMapped,         26)
LLVK_SHARED_UBO_GETTER(SMAABlendWeightsF,   SMAABlendWeightsF_PerProgramBind,   sSharedSMAABlendWeightsFUBO,   sSharedSMAABlendWeightsFUBOAllocation,   sSharedSMAABlendWeightsFUBOMapped,   4)

#undef LLVK_SHARED_UBO_GETTER

#define LLVK_SHARED_UBO_WRITER(BindName, StructType, StorageMapped, BindingNumber)                          \
    void writeCurrent##BindName##UBO(const StructType& data)                                                \
    {                                                                                                       \
        if (!sInitialized || StorageMapped == nullptr)                                                      \
        {                                                                                                   \
            return;                                                                                         \
        }                                                                                                   \
        std::memcpy(StorageMapped, &data, sizeof(StructType));                                              \
    }

LLVK_SHARED_UBO_WRITER(WindlightHDR,      WindlightHDR_PerProgramBind,      sSharedWindlightHDRUBOMapped,      10)
LLVK_SHARED_UBO_WRITER(WindlightLight,    WindlightLight_PerProgramBind,    sSharedWindlightLightUBOMapped,    11)
LLVK_SHARED_UBO_WRITER(TonemapUtilF,       TonemapUtilF_PerProgramBind,       sSharedTonemapUtilFUBOMapped,       26)
LLVK_SHARED_UBO_WRITER(SMAABlendWeightsF, SMAABlendWeightsF_PerProgramBind, sSharedSMAABlendWeightsFUBOMapped, 4)

#undef LLVK_SHARED_UBO_WRITER

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

#define LLVK_SHARED_UBO_RING_IMPL(BindName, StructType, BindingNumber)                                  \
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
        DeferredUtilOverrideSlot& slot = s##BindName##Ring[f][idx];                                    \
        std::memcpy(slot.mapped, &data, sizeof(StructType));                                           \
        sCur##BindName##Buf[f]    = slot.buffer;                                                       \
        sCur##BindName##Mapped[f] = slot.mapped;                                                       \
        LLGLSLShader::sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;                                     \
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

LLVK_SHARED_UBO_RING_IMPL(WindlightSky,     WindlightSky_PerProgramBind,     9)
LLVK_SHARED_UBO_RING_IMPL(WindlightAtmos,   WindlightAtmos_PerProgramBind,   8)
LLVK_SHARED_UBO_RING_IMPL(AoUtil,           AoUtil_PerProgramBind,           22)
LLVK_SHARED_UBO_RING_IMPL(GlobalF,          GlobalF_PerProgramBind,          18)
LLVK_SHARED_UBO_RING_IMPL(WaterFog,         WaterFog_PerProgramBind,         14)
LLVK_SHARED_UBO_RING_IMPL(WaterV,           Water_PerProgramBind,            15)
LLVK_SHARED_UBO_RING_IMPL(ReflectionProbe,  ReflectionProbe_PerProgramBind,  16)
LLVK_SHARED_UBO_RING_IMPL(ReflectionProbes, ReflectionProbes_PerProgramBind, 38)
LLVK_SHARED_UBO_RING_IMPL(ReflectionProbeF, ReflectionProbeF_PerProgramBind, 39)
LLVK_SHARED_UBO_RING_IMPL(SSRUtil,          SSRUtil_PerProgramBind,          49)
LLVK_SHARED_UBO_RING_IMPL(Lights,           Lights_PerProgramBind,           12)
LLVK_SHARED_UBO_RING_IMPL(LightsSpecular,   LightsSpecular_PerProgramBind,   12)
LLVK_SHARED_UBO_RING_IMPL(PbrTerrainF,      PbrTerrainF_PerProgramBind,      28)
LLVK_SHARED_UBO_RING_IMPL(PbrTerrain,       PbrTerrain_PerShaderBind,        52)
#undef LLVK_SHARED_UBO_RING_IMPL

#define LLVK_SHARED_UBO_DYNAMIC_IMPL(BindName, StructType)                                              \
    static StructType s##BindName##Shadow;                                                              \
    static U64 s##BindName##WriteGen = 1;                                                               \
    static U64 s##BindName##UpFrame[FRAMES_IN_FLIGHT]  = { ~0ull, ~0ull, ~0ull };                       \
    static U64 s##BindName##UpGen[FRAMES_IN_FLIGHT]    = { 0, 0, 0 };                                   \
    static U32 s##BindName##UpOffset[FRAMES_IN_FLIGHT] = { 0, 0, 0 };                                   \
    static VkBuffer s##BindName##UpBuf[FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE }; \
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
LLVK_SHARED_UBO_DYNAMIC_IMPL(AvatarSkin,   AvatarSkin_PerProgramBind)
LLVK_SHARED_UBO_DYNAMIC_IMPL(PBRMaterial,  PBRMaterial_PerMaterial)
LLVK_SHARED_UBO_DYNAMIC_IMPL(DrawColor,    DrawColor_PerShaderBind)
LLVK_SHARED_UBO_DYNAMIC_IMPL(ShadowParams, ShadowParams_PerShaderBind)
#undef LLVK_SHARED_UBO_DYNAMIC_IMPL

static ObjectSkin_PerProgramBind sObjectSkinShadow;
static U64 sObjectSkinWriteGen = 1;
static U64 sObjectSkinUpFrame[FRAMES_IN_FLIGHT]  = { ~0ull, ~0ull, ~0ull };
static U64 sObjectSkinUpGen[FRAMES_IN_FLIGHT]    = { 0, 0, 0 };
static U32 sObjectSkinUpOffset[FRAMES_IN_FLIGHT] = { 0, 0, 0 };
static VkBuffer sObjectSkinUpBuf[FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };

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
        sObjectSkinUpBuf[f]    = b;
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
        case 45: return ensureAvatarSkinUploaded(out_buf, out_off);
        case 46: return ensureObjectSkinUploaded(out_buf, out_off);
        case 48: return ensurePBRMaterialUploaded(out_buf, out_off);
        case 51: return ensureDrawColorUploaded(out_buf, out_off);
        case 53: return ensureShadowParamsUploaded(out_buf, out_off);
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
        if (e.enqueue_frame <= sLastCompletedMonotonic)
        {
            vmaDestroyBuffer(sAllocator, e.buffer, e.allocation);
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
    tickOneShotFreeQueue();
    if (sPendingOneShotFrees.size() > 256)
    {
        vkWaitForFences(sDevice, 1, &sPendingOneShotFrees.front().fence, VK_TRUE, UINT64_MAX);
        tickOneShotFreeQueue();
    }

    VkFence fence = VK_NULL_HANDLE;
    if (!sSubmitFencePool.empty())
    {
        fence = sSubmitFencePool.back();
        sSubmitFencePool.pop_back();
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

    VkSubmitInfo si = {};
    si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers    = &cmd;
    VkResult sr = vkQueueSubmit(sGraphicsQueue, 1, &si, fence);
    if (sr != VK_SUCCESS)
    {
        if (fence != VK_NULL_HANDLE)
        {
            sSubmitFencePool.push_back(fence);
        }
        vkFreeCommandBuffers(sDevice, sCommandPool, 1, &cmd);
        if (staging_buffer != VK_NULL_HANDLE || staging_allocation != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        }
        return false;
    }

    if (fence == VK_NULL_HANDLE)
    {
        vkQueueWaitIdle(sGraphicsQueue);
        vkFreeCommandBuffers(sDevice, sCommandPool, 1, &cmd);
        if (staging_buffer != VK_NULL_HANDLE || staging_allocation != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        }
        return true;
    }

    PendingOneShotFree pending;
    pending.fence      = fence;
    pending.cmd        = cmd;
    pending.buffer     = staging_buffer;
    pending.allocation = staging_allocation;
    sPendingOneShotFrees.push_back(pending);
    return true;
}

void tickOneShotFreeQueue()
{
    if (sDevice == VK_NULL_HANDLE)
    {
        return;
    }
    size_t w = 0;
    const size_t n = sPendingOneShotFrees.size();
    for (size_t r = 0; r < n; ++r)
    {
        PendingOneShotFree& e = sPendingOneShotFrees[r];
        if (vkGetFenceStatus(sDevice, e.fence) == VK_SUCCESS)
        {
            vkFreeCommandBuffers(sDevice, sCommandPool, 1, &e.cmd);
            if (e.buffer != VK_NULL_HANDLE || e.allocation != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(sAllocator, e.buffer, e.allocation);
            }
            sSubmitFencePool.push_back(e.fence);
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
}

void bindVertexBufferVk(VkCommandBuffer cmd_buf, VkBuffer buffer, VkDeviceSize offset, U32 firstBinding)
{
    if (cmd_buf == VK_NULL_HANDLE || buffer == VK_NULL_HANDLE)
    {
        return;
    }
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
    if (sPerFrameRingSets[f].empty())
    {
        return VK_NULL_HANDLE;
    }
    U32 slot = sMatrixRingHasCurrent[f] ? sMatrixRingCurrentSlot[f] : 0;
    if (slot >= sPerFrameRingSets[f].size())
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
            VkSubmitInfo si = {};
            si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            si.commandBufferCount = 1;
            si.pCommandBuffers    = &one_cmd;
            vkQueueSubmit(sGraphicsQueue, 1, &si, VK_NULL_HANDLE);
            vkQueueWaitIdle(sGraphicsQueue);
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
    LLGLSLShader::sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;
    PendingImageFree pending;
    pending.image         = image;
    pending.view          = view;
    pending.allocation    = reinterpret_cast<VmaAllocation>(allocation);
    pending.enqueue_frame = sMonotonicFrameCount;
    sPendingImageFrees.push_back(pending);
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
        if (e.enqueue_frame <= sLastCompletedMonotonic)
        {
            if (e.view != VK_NULL_HANDLE && sDevice != VK_NULL_HANDLE)
            {
                vkDestroyImageView(sDevice, e.view, nullptr);
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
        if (e.enqueue_frame <= sLastCompletedMonotonic)
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
    if (created)
    {
        LLGLSLShader::sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;
    }
    return created;
}

bool createReadbackImageVk(U32          width,
                           U32          height,
                           VkFormat     format,
                           VkImage&     out_image,
                           VkImageView& out_view,
                           void*&       out_allocation)
{
    VkImageUsageFlags usage =
          VK_IMAGE_USAGE_TRANSFER_DST_BIT
        | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
        | VK_IMAGE_USAGE_SAMPLED_BIT;
    return createAttachmentImageVkImpl(width, height, format,
                                       usage,
                                       VK_IMAGE_ASPECT_COLOR_BIT,
                                       "createReadbackImageVk",
                                       out_image, out_view, out_allocation,
                                       1);
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

    out_image      = image;
    out_view       = view;
    out_allocation = reinterpret_cast<void*>(allocation);
    LLGLSLShader::sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;
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
        VkSubmitInfo si = {};
        si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.commandBufferCount = 1;
        si.pCommandBuffers    = &cmd;
        vkQueueSubmit(sGraphicsQueue, 1, &si, VK_NULL_HANDLE);
        vkQueueWaitIdle(sGraphicsQueue);
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

    VkSubmitInfo si = {};
    si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers    = &cmd;
    VkResult sr = vkQueueSubmit(sGraphicsQueue, 1, &si, VK_NULL_HANDLE);
    if (sr != VK_SUCCESS)
    {
        vkFreeCommandBuffers(sDevice, sCommandPool, 1, &cmd);
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        return false;
    }
    vkQueueWaitIdle(sGraphicsQueue);
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

    VkSubmitInfo si = {};
    si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers    = &cmd;
    VkResult sr = vkQueueSubmit(sGraphicsQueue, 1, &si, VK_NULL_HANDLE);
    if (sr != VK_SUCCESS)
    {
        vkFreeCommandBuffers(sDevice, sCommandPool, 1, &cmd);
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        return false;
    }
    vkQueueWaitIdle(sGraphicsQueue);
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

    if (!sInFrame || sCommandBuffers[sFrameIndex] == VK_NULL_HANDLE)
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
        vkCmdEndRendering(sCommandBuffers[sFrameIndex]);
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

    vkCmdPipelineBarrier(sCommandBuffers[sFrameIndex],
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

        vkCmdBeginRendering(sCommandBuffers[sFrameIndex], &resume_info);
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
    U32 mode = 1;
    if (gSavedSettings.controlExists("RenderBackend"))
    {
        mode = gSavedSettings.getU32("RenderBackend");
    }
    return mode;
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

void setupViewportAndScissor(VkCommandBuffer cmd, bool screen_space_copy)
{
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

    if (sViewportScissorValid
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
    if (pipeline == sLastBoundGraphicsPipeline)
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
    const U32 pass_bucket = gHeroProbeMirrorRender ? 4u
                            : gCubeSnapshot ? 3u
                            : (gVkPerfPassTag < 3u ? gVkPerfPassTag : 0u);
    ++gVkPerf.draws_pass[pass_bucket];
    if (pass_bucket == 1u)
    {
        ++gVkPerf.draws_shadow_map[gVkPerfShadowMapIndex < 6u ? gVkPerfShadowMapIndex : 5u];
    }
    if (layout == sLastDescLayout
        && set0 == sLastDescSet0
        && set1 == sLastDescSet1
        && dyn_count == sLastDescDynCount
        && (dyn_count == 0 || std::memcmp(offsets, sLastDescOffsets, dyn_count * sizeof(U32)) == 0))
    {
        ++gVkPerf.desc_skip;
        return;
    }
    ++gVkPerf.desc_bind;
    VkDescriptorSet sets[2] = { set0, set1 };
    vkCmdBindDescriptorSets(cmd,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            layout,
                            0,
                            2,
                            sets,
                            dyn_count,
                            dyn_count ? offsets : nullptr);
    sLastDescLayout   = layout;
    sLastDescSet0     = set0;
    sLastDescSet1     = set1;
    sLastDescDynCount = dyn_count;
    if (dyn_count > 0)
    {
        std::memcpy(sLastDescOffsets, offsets, dyn_count * sizeof(U32));
    }
}

void pushModelviewOnce(VkCommandBuffer cmd, VkPipelineLayout layout, const float* mv16)
{
    if (layout == sLastMvLayout && std::memcmp(mv16, sLastMv, sizeof(sLastMv)) == 0)
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

}
