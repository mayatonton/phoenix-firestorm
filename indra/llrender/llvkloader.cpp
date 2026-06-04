/**
* @file llvkloader.cpp
* @brief AYAstorm r41 Vulkan loader + instance + device lifecycle (volk-based)
*
* $LicenseInfo:firstyear=2026&license=viewerlgpl$
* AYAstorm Viewer Source Code
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
* $/LicenseInfo$
*/

#include "linden_common.h"
#include "llvkloader.h"

#include "volk.h"
#include "lldir.h"
#include "llassetubopool.h"
#include "lluboringbuffer.h"
#include "llpipelinecachestorage.h"
#include "llcontrol.h"

#include <vector>
#include <string>
#include <climits>
#include <cstring>
#include <fstream>
#include <memory>
#include <unordered_map>

extern LLControlGroup gSavedSettings;

// r41 sub-step 3.4-β-1 (sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.1 内包):
// VMA (Vulkan Memory Allocator) v3.3.0 を本 translation unit に impl 展開。
// volk と組み合わせるため static vulkan functions は無効化、
// dynamic vulkan functions で vkGetInstanceProcAddr / vkGetDeviceProcAddr 経由 auto-resolve。
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
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#endif

namespace LLVKLoader
{

namespace
{
    VkInstance sInstance = VK_NULL_HANDLE;
    VkPhysicalDevice sPhysicalDevice = VK_NULL_HANDLE;
    VkDevice sDevice = VK_NULL_HANDLE;
    VkQueue sGraphicsQueue = VK_NULL_HANDLE;
    U32 sGraphicsQueueFamily = UINT_MAX;
    std::string sDeviceName;
    bool sInitialized = false;
    bool sValidationEnabled = false;

    VkDebugUtilsMessengerEXT sDebugMessenger = VK_NULL_HANDLE;

    VkCommandPool sCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer sCommandBuffer = VK_NULL_HANDLE;
    VkPipelineCache sPipelineCache = VK_NULL_HANDLE;

    // r41 sub-step 3.1b item #8: minimal placeholder PSO (sky pool 用 placeholder、
    // sub-doc 03 §3.1 sub-step 3.1 完了 marker「最小 PSO compile 成功 + 1 frame 内 bind validation 0 件」)
    VkShaderModule   sPlaceholderVertModule = VK_NULL_HANDLE;
    VkShaderModule   sPlaceholderFragModule = VK_NULL_HANDLE;
    VkPipelineLayout sPlaceholderLayout     = VK_NULL_HANDLE;
    VkPipeline       sPlaceholderPipeline   = VK_NULL_HANDLE;

    // r41 sub-step 3.2 (refine 2026-05-29): sky pool smoke-test PSO (sub-doc 03 §3.1 sub-step 3.2、
    // fullscreen triangle + 定数色 frag、vkCmdDraw(3,1,0,0) 投入 smoke、descriptor 不要)
    VkShaderModule   sSkySmokeVertModule = VK_NULL_HANDLE;
    VkShaderModule   sSkySmokeFragModule = VK_NULL_HANDLE;
    VkPipelineLayout sSkySmokeLayout     = VK_NULL_HANDLE;
    VkPipeline       sSkySmokePipeline   = VK_NULL_HANDLE;

    // r41 sub-step 3.4-δ-4 (sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.4 push descriptor 部分内包):
    // avatar bone matrix SSBO + push descriptor 配線。set=2 binding 0 = STORAGE_BUFFER (mat4 × 110 bones
    // = 7040 B)、VERTEX_BIT、VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR flag (set 本体は
    // pool 不要、vkCmdPushDescriptorSetKHR で in-frame 投入)。
    // VK_KHR_push_descriptor 未支援 device では layout/buffer/pipeline 作成 skip し、
    // recordAvatarPlaceholderDraw は recordPlaceholderPoolDraw に fallback (機能 graceful degrade)。
    constexpr U32         AVATAR_BONE_MATRIX_COUNT          = 110; // SL viewer max bone count 想定
    VkDescriptorSetLayout sAvatarBoneDescriptorSetLayout    = VK_NULL_HANDLE;
    VkPipelineLayout      sAvatarBoneLayout                 = VK_NULL_HANDLE;
    VkPipeline            sAvatarBonePipeline               = VK_NULL_HANDLE;
    VkBuffer              sAvatarBoneStorageBuffer          = VK_NULL_HANDLE;
    VmaAllocation         sAvatarBoneStorageAllocation      = VK_NULL_HANDLE;
    void*                 sAvatarBoneStorageMapped          = nullptr;

    VkRenderPass sRenderPass = VK_NULL_HANDLE;
    VkImage sOffscreenImage = VK_NULL_HANDLE;
    VkDeviceMemory sOffscreenMemory = VK_NULL_HANDLE;
    VkImageView sOffscreenImageView = VK_NULL_HANDLE;
    VkFramebuffer sFramebuffer = VK_NULL_HANDLE;
    bool sInFrame = false;

    // r41 sub-step 3.3-C-β-2: dynamic rendering begin/end pair tracker (sub-doc 03 §3.1.2)。
    // beginDynamicRendering() で実 vkCmdBeginRendering 発火時のみ true、endDynamicRendering()
    // で対称 reset。begin が no-op early return した場合 end も no-op skip して pair 維持。
    bool sInDynamicRendering = false;

    constexpr U32 OFFSCREEN_WIDTH = 64;
    constexpr U32 OFFSCREEN_HEIGHT = 64;
    constexpr VkFormat OFFSCREEN_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;

    // r41 sub-step 3.3-β-2: per-frame matrix UBO buffer + descriptor (sub-doc 03 §3.1.1)
    // 二段構え binding 0 (PerFrameMatrixUBO 192 B) + binding 1 (TextureMatrixUBO 256 B) を
    // 単一 VkBuffer に offset 配置 (256 B align 安全側) × frame in flight 3 個。
    constexpr U32          FRAMES_IN_FLIGHT      = 3;
    constexpr VkDeviceSize PERFRAME_UBO_OFFSET   = 0;
    constexpr VkDeviceSize PERFRAME_UBO_SIZE     = sizeof(PerFrameMatrixUBO);  // 192
    constexpr VkDeviceSize TEXTURE_UBO_OFFSET    = 256;                        // 256 B align (vendor 安全側)
    constexpr VkDeviceSize TEXTURE_UBO_SIZE      = sizeof(TextureMatrixUBO);   // 256
    constexpr VkDeviceSize UBO_BUFFER_SIZE_FRAME = TEXTURE_UBO_OFFSET + TEXTURE_UBO_SIZE;  // 512

    VkDescriptorSetLayout sPerFrameDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool      sPerFrameDescriptorPool      = VK_NULL_HANDLE;
    VkBuffer              sPerFrameUboBuffer[FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    VkDeviceMemory        sPerFrameUboMemory[FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    void*                 sPerFrameUboMapped[FRAMES_IN_FLIGHT] = { nullptr, nullptr, nullptr };
    VkDescriptorSet       sPerFrameDescriptorSet[FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };

    // r41 sub-step 3.3-δ-1: frame in flight index counter (sub-doc 03 §3.1.1 δ)。
    // beginFrame() で advance、syncMatrices Vulkan path / δ-2 push constant が共有参照。
    U32 sFrameIndex = 0;

    bool createInstance()
    {
        std::vector<const char*> layers;
#ifndef LL_RELEASE_FOR_DOWNLOAD
        layers.push_back("VK_LAYER_KHRONOS_validation");
        sValidationEnabled = true;
#endif

        std::vector<const char*> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
        };
#ifndef LL_RELEASE_FOR_DOWNLOAD
        // r41 sub-step 3.1b item #10: VK_EXT_debug_utils messenger 配線 (GL ARB debug callback 移管先)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

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

        VkResult result = vkCreateInstance(&create_info, nullptr, &sInstance);
        if (result == VK_ERROR_LAYER_NOT_PRESENT && sValidationEnabled)
        {
            LL_WARNS("Vulkan") << "Validation layer not present, retrying without validation" << LL_ENDL;
            create_info.enabledLayerCount = 0;
            sValidationEnabled = false;
            result = vkCreateInstance(&create_info, nullptr, &sInstance);
        }

        if (result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkCreateInstance failed: " << (S32)result << LL_ENDL;
            return false;
        }

        volkLoadInstanceOnly(sInstance);
        LL_INFOS("Vulkan") << "Vulkan instance created (validation="
                           << (sValidationEnabled ? "enabled" : "disabled") << ")" << LL_ENDL;
        return true;
    }

#ifndef LL_RELEASE_FOR_DOWNLOAD
    VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
        VkDebugUtilsMessageTypeFlagsEXT             /*types*/,
        const VkDebugUtilsMessengerCallbackDataEXT* cb_data,
        void*                                       /*user_data*/)
    {
        const char* msg = (cb_data && cb_data->pMessage) ? cb_data->pMessage : "(null)";
        if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            LL_WARNS("Vulkan") << "[VK ERROR] " << msg << LL_ENDL;
        }
        else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            LL_WARNS("Vulkan") << "[VK WARN] " << msg << LL_ENDL;
        }
        else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        {
            LL_INFOS("Vulkan") << "[VK INFO] " << msg << LL_ENDL;
        }
        else
        {
            LL_DEBUGS("Vulkan") << "[VK VERBOSE] " << msg << LL_ENDL;
        }
        return VK_FALSE;
    }
#endif // !LL_RELEASE_FOR_DOWNLOAD

    bool createDebugMessenger()
    {
#ifdef LL_RELEASE_FOR_DOWNLOAD
        return true;
#else
        if (!sValidationEnabled)
        {
            return true;
        }
        if (vkCreateDebugUtilsMessengerEXT == nullptr)
        {
            LL_WARNS("Vulkan") << "vkCreateDebugUtilsMessengerEXT not loaded; skipping messenger" << LL_ENDL;
            return true;
        }

        VkDebugUtilsMessengerCreateInfoEXT info = {};
        info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        info.pfnUserCallback = vulkanDebugCallback;

        VkResult result = vkCreateDebugUtilsMessengerEXT(sInstance, &info, nullptr, &sDebugMessenger);
        if (result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkCreateDebugUtilsMessengerEXT failed: " << (S32)result << LL_ENDL;
            return true; // non-fatal
        }
        LL_INFOS("Vulkan") << "VK_EXT_debug_utils messenger installed" << LL_ENDL;
        return true;
#endif
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
            LL_WARNS("Vulkan") << "No physical devices found" << LL_ENDL;
            return false;
        }

        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(sInstance, &count, devices.data());
        LL_INFOS("Vulkan") << "Found " << count << " physical device(s)" << LL_ENDL;

        VkPhysicalDevice best = VK_NULL_HANDLE;
        int best_score = -1;
        std::string best_name;

        for (auto dev : devices)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(dev, &props);

            if (props.apiVersion < VK_API_VERSION_1_3)
            {
                LL_INFOS("Vulkan") << "  Skipping " << props.deviceName
                                   << " (Vulkan " << VK_VERSION_MAJOR(props.apiVersion)
                                   << "." << VK_VERSION_MINOR(props.apiVersion)
                                   << ", requires 1.3)" << LL_ENDL;
                continue;
            }

            int score = scoreDeviceType(props.deviceType);
            LL_INFOS("Vulkan") << "  Candidate: " << props.deviceName
                               << " (type=" << (S32)props.deviceType
                               << ", score=" << score << ")" << LL_ENDL;

            if (score > best_score)
            {
                best_score = score;
                best = dev;
                best_name = props.deviceName;
            }
        }

        if (best == VK_NULL_HANDLE)
        {
            LL_WARNS("Vulkan") << "No suitable physical device (Vulkan 1.3 required)" << LL_ENDL;
            return false;
        }

        sPhysicalDevice = best;
        sDeviceName = best_name;
        LL_INFOS("Vulkan") << "Selected physical device: " << sDeviceName << LL_ENDL;
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
                LL_INFOS("Vulkan") << "Graphics queue family: " << i
                                   << " (queueCount=" << families[i].queueCount << ")" << LL_ENDL;
                return true;
            }
        }

        LL_WARNS("Vulkan") << "No graphics queue family found" << LL_ENDL;
        return false;
    }

    struct DeviceLimits
    {
        U32  maxBoundDescriptorSets             = 0;
        U32  maxPushConstantsSize               = 0;
        U32  maxPushDescriptors                 = 0; // 0 = VK_KHR_push_descriptor unsupported
        U32  maxPerStageDescriptorSampledImages = 0;
        U32  maxColorAttachments                = 0;
        U32  maxDescriptorSetSamplers           = 0;
        bool pushDescriptorSupported            = false;
        // r41 sub-step 3.4-β-1 (sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.1 内包):
        // VK_EXT_memory_budget が device に存在すれば VMA に渡し、vmaGetBudget() で
        // heap 毎の budget/usage を logVmaBudgetSmoke() で 1 回 INFO 出力する。
        // unsupported の場合は VMA 側 fallback path (allocation 累積) が動作する。
        bool memoryBudgetSupported              = false;
    };
    DeviceLimits sDeviceLimits;

    // r41 sub-step 3.4-β-1 (sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.1 内包):
    // VMA allocator + 段階 3 段階共有 descriptor pool 雛形 + budget 1 回 smoke 出力 flag。
    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6α (W2):
    // per-asset (set=3) descriptor pool grow 機構実 wire up。
    // design/07-vulkan-api-state.md §6.1 / §6.3 / §6.4:
    //   per-pool sizing (= 1 物理 pool = 64 asset × FRAMES_IN_FLIGHT=3 frame):
    //     maxSets               = 64 × 3 = 192
    //     UBO  descriptorCount  = 3 binding × 64 × 3 = 576
    //     SAMPLER descriptorCount = 49 binding × 64 × 3 = 9408
    //   FREE_DESCRIPTOR_SET_BIT は付けない (= grow only、cleanup は cadence
    //   単位 shutdown 時 reverse 順 destroy)。LLAssetUboPool 側 algorithm が
    //   acquire 枯渇 detect で factory 再呼出して grow chunk pool を追加する
    //   ため、本 wire up では callback を提供するだけ (実 pool 数は scene 依存)。
    constexpr U32 ASSET_POOL_UBO_BINDINGS_PER_ASSET     = 3;   // design 07 §6.3
    constexpr U32 ASSET_POOL_SAMPLER_BINDINGS_PER_ASSET = 49;  // design 07 §6.3
    std::unique_ptr<LLAssetUboPool> sAssetUboPoolMgr;

    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6β (RB):
    // per-frame / per-pass cadence UBO ring buffer 実 wire up。LLUboRingBuffer は
    // Vulkan device 非依存 bookkeeping algorithm で BufferHandle = std::uint64_t
    // opaque 単独保持。実 VkBuffer + VmaAllocation + mapped pointer 三組は本 TU
    // 内 side table (sDrawUboRingBufferRecords) で handle → record 解決する。
    // factory は vmaCreateBuffer (HOST_VISIBLE + HOST_COHERENT + MAPPED)、
    // destroyer は vmaDestroyBuffer。grow 時は invokeAllocator → destroyCurrent
    // 順で一時的に 2 record が並走するため map 構造を採用 (= 1 entry slot 不可)。
    // design 07 §7.2 (initial=4 MB / max=16 MB) / §7.3 (alignment 256 safe) /
    // §7.5 (3 chunk wrap + grow on chunk 内枯渇) / §8.4 (FRAMES_IN_FLIGHT 同期 rotate)。
    struct DrawUboRingBufferRecord
    {
        VkBuffer      buffer     = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        void*         mapped     = nullptr;
    };
    std::unordered_map<LLUboRingBuffer::BufferHandle, DrawUboRingBufferRecord>
        sDrawUboRingBufferRecords;
    std::unique_ptr<LLUboRingBuffer> sDrawUboRingBufferMgr;

    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6γ (PSC):
    // VkPipelineCache blob の disk persist 機構 (= LLPipelineCacheStorage)。
    // 起動時に file (= gDirUtilp LL_PATH_CACHE + "pipeline_cache.bin") から
    // blob を load し、createPipelineCache() が VkPipelineCacheCreateInfo.
    // pInitialData に投入することで PSO compile hit を確保。shutdownVulkan()
    // で vkGetPipelineCacheData → updateBlob → persistToDisk により次回起動
    // 向け blob を上書き保存。64 MB 上限は LLPipelineCacheStorage 側で
    // enforce ((e1) = load 時 size > cap で blob 破棄 + persist 時 size > cap
    // で writer 不呼出 + false return)、cvar AYAPipelineCacheSizeMB で配信。
    // design 07 §9.3 (PSO cache 戦略) + §12 (PSC) 整合。
    std::unique_ptr<LLPipelineCacheStorage> sPipelineCacheStorageMgr;

    // sSharedDescriptorPool は sub-step 3.4-γ 以降で set=1 (per-material 7 PBR slot)
    // および set=2 (per-draw push descriptor fallback) を割り当てる雛形 pool。
    // 暫定 sizing は overshoot (UBO 16 + COMBINED_IMAGE_SAMPLER 64, maxSets=200)、
    // 最終精度は領域 7 sub-step 7.3 で確定する (sub-doc 05 §3.6 参照)。
    VmaAllocator     sAllocator                 = VK_NULL_HANDLE;
    VkDescriptorPool sSharedDescriptorPool      = VK_NULL_HANDLE;
    bool             sSharedVmaBudgetLogged     = false;

    // r41 sub-step 3.4-β-2 (sub-doc 03 §3.1.4): 1×1 white placeholder texture transit smoke。
    // per-LLImageGL 抱合せは領域 7 sub-step 7.5 持越し、本 sub-step は llvkloader 単独で
    // vmaCreateImage + vkCreateImageView + (β-2-3) staging upload + 2 段 layout transition +
    // 破棄まで一連動作実証する file-local placeholder。VmaAllocation は本 TU 限定 (header 不露出)。
    VkImage       sPlaceholderWhiteImage      = VK_NULL_HANDLE;
    VkImageView   sPlaceholderWhiteImageView  = VK_NULL_HANDLE;
    VmaAllocation sPlaceholderWhiteAllocation = VK_NULL_HANDLE;

    // r41 sub-step 3.4-γ (sub-doc 03 §3.1.4 / sub-doc 07 §1.2.1 set=1 / §3.1 sub-step 7.3 layout 部分内包):
    // per-material 7 PBR slot descriptor set layout + 共用 placeholder sampler + 1 transit smoke set。
    // binding 0=DIFFUSE / 1=NORMAL / 2=SPECULAR / 3=BASECOLOR / 4=METALLIC_ROUGHNESS / 5=GLTF_NORMAL /
    // 6=EMISSIVE、descriptor type = COMBINED_IMAGE_SAMPLER × 7、stage = FRAGMENT_BIT。
    // sampler 作成は本 sub-step 内 1 件のみ (placeholder linear/clamp 共用)、per-texture sampler 配信
    // (mipmap / anisotropy) は領域 7 sub-step 7.3 残置。material cache 本実装 (~50 material × frame
    // in flight 3 = 150 pool sizing) も 7.3 残置、本 sub-step は layout + 1 set transit smoke のみ scope。
    constexpr U32         PER_MATERIAL_BINDING_COUNT      = 7;
    VkDescriptorSetLayout sPerMaterialDescriptorSetLayout = VK_NULL_HANDLE;
    VkSampler             sPlaceholderSampler             = VK_NULL_HANDLE;
    VkDescriptorSet       sPerMaterialDescriptorSet       = VK_NULL_HANDLE;

    bool queryAndLogDeviceLimits()
    {
        // r41 sub-step 3.1b measurement-first cadence (sub-doc 03 §1.5.4):
        // 6 device limit を log 出力 → AYA 環境実測値 base に sub-step 3.4 で descriptor 設計 final 化。
        U32 ext_count = 0;
        vkEnumerateDeviceExtensionProperties(sPhysicalDevice, nullptr, &ext_count, nullptr);
        std::vector<VkExtensionProperties> exts(ext_count);
        if (ext_count > 0)
        {
            vkEnumerateDeviceExtensionProperties(sPhysicalDevice, nullptr, &ext_count, exts.data());
        }

        bool push_desc_supported = false;
        bool mem_budget_supported = false;
        for (const auto& e : exts)
        {
            const std::string name(e.extensionName);
            if (name == VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME)
            {
                push_desc_supported = true;
            }
            else if (name == VK_EXT_MEMORY_BUDGET_EXTENSION_NAME)
            {
                mem_budget_supported = true;
            }
        }
        sDeviceLimits.memoryBudgetSupported = mem_budget_supported;

        VkPhysicalDevicePushDescriptorPropertiesKHR push_desc_props = {};
        push_desc_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR;

        VkPhysicalDeviceProperties2 props2 = {};
        props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        if (push_desc_supported)
        {
            props2.pNext = &push_desc_props;
        }

        vkGetPhysicalDeviceProperties2(sPhysicalDevice, &props2);

        const VkPhysicalDeviceLimits& l = props2.properties.limits;
        sDeviceLimits.maxBoundDescriptorSets             = l.maxBoundDescriptorSets;
        sDeviceLimits.maxPushConstantsSize               = l.maxPushConstantsSize;
        sDeviceLimits.maxPerStageDescriptorSampledImages = l.maxPerStageDescriptorSampledImages;
        sDeviceLimits.maxColorAttachments                = l.maxColorAttachments;
        sDeviceLimits.maxDescriptorSetSamplers           = l.maxDescriptorSetSamplers;
        sDeviceLimits.maxPushDescriptors                 = push_desc_supported ? push_desc_props.maxPushDescriptors : 0;
        sDeviceLimits.pushDescriptorSupported            = push_desc_supported;

        LL_INFOS("Vulkan") << "Device limit baseline (r41 sub-step 3.1b measurement-first):" << LL_ENDL;
        LL_INFOS("Vulkan") << "  maxBoundDescriptorSets             = "
                           << sDeviceLimits.maxBoundDescriptorSets
                           << " (Vulkan 1.3 minimum 4)" << LL_ENDL;
        LL_INFOS("Vulkan") << "  maxPushConstantsSize               = "
                           << sDeviceLimits.maxPushConstantsSize
                           << " bytes (Vulkan 1.3 minimum 128)" << LL_ENDL;
        if (push_desc_supported)
        {
            LL_INFOS("Vulkan") << "  maxPushDescriptors                 = "
                               << sDeviceLimits.maxPushDescriptors
                               << " (VK_KHR_push_descriptor minimum 32)" << LL_ENDL;
        }
        else
        {
            LL_INFOS("Vulkan") << "  maxPushDescriptors                 = (VK_KHR_push_descriptor NOT supported)" << LL_ENDL;
        }
        LL_INFOS("Vulkan") << "  maxPerStageDescriptorSampledImages = "
                           << sDeviceLimits.maxPerStageDescriptorSampledImages
                           << " (Vulkan 1.3 minimum 16)" << LL_ENDL;
        LL_INFOS("Vulkan") << "  maxColorAttachments                = "
                           << sDeviceLimits.maxColorAttachments
                           << " (Vulkan 1.3 minimum 4)" << LL_ENDL;
        LL_INFOS("Vulkan") << "  maxDescriptorSetSamplers           = "
                           << sDeviceLimits.maxDescriptorSetSamplers
                           << " (Vulkan 1.3 minimum 80)" << LL_ENDL;
        LL_INFOS("Vulkan") << "  VK_EXT_memory_budget               = "
                           << (sDeviceLimits.memoryBudgetSupported ? "supported (VMA budget query enabled)"
                                                                   : "NOT supported (VMA fallback path)")
                           << LL_ENDL;
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

        // r41 sub-step 3.1b item #4: enable shaderClipDistance for LLGLUserClipPlane /
        // LLGLEnable(GL_CLIP_PLANE0) shader-side `gl_ClipDistance[N]` 経路。
        VkPhysicalDeviceFeatures supported_features = {};
        vkGetPhysicalDeviceFeatures(sPhysicalDevice, &supported_features);

        VkPhysicalDeviceFeatures enabled_features = {};
        if (supported_features.shaderClipDistance)
        {
            enabled_features.shaderClipDistance = VK_TRUE;
            LL_INFOS("Vulkan") << "Device feature shaderClipDistance enabled" << LL_ENDL;
        }
        else
        {
            LL_WARNS("Vulkan") << "Device feature shaderClipDistance not supported "
                                  "(LLGLUserClipPlane / LLGLEnable(GL_CLIP_PLANE0) 経路は "
                                  "shader gl_ClipDistance 配信不可、oblique projection trick で fallback 想定)"
                               << LL_ENDL;
        }

        // r41 sub-step 3.3-C-β-2: enable Vulkan 1.3 dynamicRendering feature
        // (LLRenderTarget::bindTarget/flush の Vulkan path 並走 = vkCmdBeginRendering /
        //  vkCmdEndRendering、sub-doc 03 §3.1.2 + sub-doc 07 §1.2.3)。
        // Vulkan 1.3 では dynamicRendering は mandatory feature (spec 保証)。
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
        else
        {
            LL_WARNS("Vulkan") << "Device feature dynamicRendering NOT supported "
                                  "(Vulkan 1.3 spec 保証違反、sub-step 3.3-C 並走 path 無効化)"
                               << LL_ENDL;
        }

        // r41 sub-step 3.4-β-1 (sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.1 内包):
        // device extension の最小 enable set。
        //  - VK_EXT_memory_budget : 支援される場合のみ enable、VMA に渡して
        //    heap 毎 budget/usage 取得を有効化。未支援 device では fallback path。
        // 注: VK_KHR_push_descriptor は sub-step 3.4-δ-4 で enable (set=2 avatar bone SSBO
        //     push descriptor 配線、vkCmdPushDescriptorSetKHR 経由)。未支援 device では
        //     extension push スキップ + recordAvatarPlaceholderDraw が recordPlaceholderPoolDraw へ
        //     graceful fallback (機能 degrade、device 作成は継続)。
        std::vector<const char*> device_extensions;
        if (sDeviceLimits.memoryBudgetSupported)
        {
            device_extensions.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
        }
        if (sDeviceLimits.pushDescriptorSupported)
        {
            device_extensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
            LL_INFOS("Vulkan") << "Device extension VK_KHR_push_descriptor enabled (sub-step 3.4-d-4 avatar bone SSBO foundation)" << LL_ENDL;
        }

        VkDeviceCreateInfo device_info = {};
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.pNext = &dr_features_enable;  // r41 3.3-C-β-2: chain dynamic rendering feature
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &queue_info;
        device_info.enabledExtensionCount = (U32)device_extensions.size();
        device_info.ppEnabledExtensionNames = device_extensions.empty() ? nullptr : device_extensions.data();
        device_info.pEnabledFeatures = &enabled_features;

        VkResult result = vkCreateDevice(sPhysicalDevice, &device_info, nullptr, &sDevice);
        if (result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkCreateDevice failed: " << (S32)result << LL_ENDL;
            return false;
        }

        volkLoadDevice(sDevice);
        vkGetDeviceQueue(sDevice, sGraphicsQueueFamily, 0, &sGraphicsQueue);

        LL_INFOS("Vulkan") << "Vulkan device created (graphics queue family "
                           << sGraphicsQueueFamily << ")" << LL_ENDL;

        // r41 sub-step 3.3-C-β-2 marker: dynamicRendering feature status (LLRenderTarget Vulkan
        // 並走基盤、sub-doc 03 §3.1.2)
        if (dr_features_enable.dynamicRendering)
        {
            LL_INFOS("Vulkan") << "Vulkan 1.3 dynamicRendering feature enabled "
                                  "(LLRenderTarget bindTarget/flush Vulkan path 並走基盤、"
                                  "sub-step 3.3-C-β-2)"
                               << LL_ENDL;
        }
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
            LL_WARNS("Vulkan") << "vkCreateCommandPool failed: " << (S32)result << LL_ENDL;
            return false;
        }

        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = sCommandPool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = 1;

        result = vkAllocateCommandBuffers(sDevice, &alloc_info, &sCommandBuffer);
        if (result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkAllocateCommandBuffers failed: " << (S32)result << LL_ENDL;
            return false;
        }

        LL_INFOS("Vulkan") << "Command pool + primary command buffer created" << LL_ENDL;
        return true;
    }

    bool createOffscreenImage()
    {
        VkImageCreateInfo image_info = {};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = OFFSCREEN_FORMAT;
        image_info.extent = { OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT, 1 };
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkResult result = vkCreateImage(sDevice, &image_info, nullptr, &sOffscreenImage);
        if (result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkCreateImage failed: " << (S32)result << LL_ENDL;
            return false;
        }

        VkMemoryRequirements mem_req;
        vkGetImageMemoryRequirements(sDevice, sOffscreenImage, &mem_req);

        S32 mem_type = findMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (mem_type < 0)
        {
            LL_WARNS("Vulkan") << "No suitable memory type for offscreen image" << LL_ENDL;
            return false;
        }

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_req.size;
        alloc_info.memoryTypeIndex = (U32)mem_type;

        result = vkAllocateMemory(sDevice, &alloc_info, nullptr, &sOffscreenMemory);
        if (result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkAllocateMemory failed: " << (S32)result << LL_ENDL;
            return false;
        }

        vkBindImageMemory(sDevice, sOffscreenImage, sOffscreenMemory, 0);

        VkImageViewCreateInfo view_info = {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = sOffscreenImage;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = OFFSCREEN_FORMAT;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        result = vkCreateImageView(sDevice, &view_info, nullptr, &sOffscreenImageView);
        if (result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkCreateImageView failed: " << (S32)result << LL_ENDL;
            return false;
        }

        LL_INFOS("Vulkan") << "Offscreen image " << OFFSCREEN_WIDTH << "x" << OFFSCREEN_HEIGHT
                           << " created (" << (S32)mem_req.size << " bytes)" << LL_ENDL;
        return true;
    }

    bool createRenderPass()
    {
        VkAttachmentDescription color_attachment = {};
        color_attachment.format = OFFSCREEN_FORMAT;
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_ref = {};
        color_ref.attachment = 0;
        color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_ref;

        VkRenderPassCreateInfo rp_info = {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rp_info.attachmentCount = 1;
        rp_info.pAttachments = &color_attachment;
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &subpass;

        VkResult result = vkCreateRenderPass(sDevice, &rp_info, nullptr, &sRenderPass);
        if (result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkCreateRenderPass failed: " << (S32)result << LL_ENDL;
            return false;
        }

        LL_INFOS("Vulkan") << "Minimal render pass created (1 color attachment)" << LL_ENDL;
        return true;
    }

    bool createFramebuffer()
    {
        VkFramebufferCreateInfo fb_info = {};
        fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.renderPass = sRenderPass;
        fb_info.attachmentCount = 1;
        fb_info.pAttachments = &sOffscreenImageView;
        fb_info.width = OFFSCREEN_WIDTH;
        fb_info.height = OFFSCREEN_HEIGHT;
        fb_info.layers = 1;

        VkResult result = vkCreateFramebuffer(sDevice, &fb_info, nullptr, &sFramebuffer);
        if (result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkCreateFramebuffer failed: " << (S32)result << LL_ENDL;
            return false;
        }

        LL_INFOS("Vulkan") << "Framebuffer created" << LL_ENDL;
        return true;
    }

    bool createPipelineCache()
    {
        // r41 sub-step 3.1b: persistent VkPipelineCache (sub-doc 03 §3.1 sub-step 3.1 marker)。
        // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6γ (PSC):
        // sPipelineCacheStorageMgr が存在 + 起動時 load 済 blob が空でなければ
        // pInitialData に投入 (= PSO compile cache hit、初回起動時は miss = empty)。
        // shutdown 時の vkGetPipelineCacheData → updateBlob → persistToDisk は
        // shutdownVulkan() 側で実装、本 helper では投入のみ。
        VkPipelineCacheCreateInfo info = {};
        info.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        info.initialDataSize = 0;
        info.pInitialData    = nullptr;
        if (sPipelineCacheStorageMgr)
        {
            const auto& blob = sPipelineCacheStorageMgr->getBlob();
            if (!blob.empty())
            {
                info.initialDataSize = blob.size();
                info.pInitialData    = blob.data();
            }
        }

        VkResult result = vkCreatePipelineCache(sDevice, &info, nullptr, &sPipelineCache);
        if (result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkCreatePipelineCache failed: " << (S32)result << LL_ENDL;
            return false;
        }

        LL_INFOS("Vulkan") << "VkPipelineCache created (PC-6γ PSC initial blob="
                           << (S32)info.initialDataSize << " bytes)" << LL_ENDL;
        return true;
    }

    // r41 sub-step 3.4-β-1 (sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.1 内包):
    // VMA (Vulkan Memory Allocator) v3.3.0 を初期化。
    // - vulkan API version は Vulkan 1.3 (instance/device 共通)。
    // - VMA_STATIC_VULKAN_FUNCTIONS=0 / VMA_DYNAMIC_VULKAN_FUNCTIONS=1 のため、
    //   vkGetInstanceProcAddr / vkGetDeviceProcAddr 2 entry のみ fill すれば VMA が残りを auto-resolve。
    // - VK_EXT_memory_budget enable 済の場合は VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT を立て、
    //   vmaGetBudget 経由で heap 毎 budget/usage を取得可能にする。
    // INFO marker #1: "VMA allocator created ..."
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
            LL_WARNS("Vulkan") << "vmaCreateAllocator failed: " << (S32)result << LL_ENDL;
            return false;
        }

        LL_INFOS("Vulkan") << "VMA allocator created (Vulkan 1.3, dynamic functions via volk, memory_budget="
                           << (sDeviceLimits.memoryBudgetSupported ? "ON" : "OFF")
                           << ")" << LL_ENDL;
        return true;
    }

    // r41 sub-step 3.4-β-1 (sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.1 内包):
    // 段階 3 共有 descriptor pool 雛形 (sub-doc 05 §3.6)。
    // 暫定 overshoot sizing:
    //   - UNIFORM_BUFFER: 16 (3.4-γ 以降の追加 UBO 余裕)
    //   - COMBINED_IMAGE_SAMPLER: 64 (set=1 per-material 7 PBR slot × ~9 set 想定)
    //   - maxSets: 200 (sub-doc 05 §3.6 strategy 準拠、~150 + 余裕)
    // 最終精度は領域 7 sub-step 7.3 で確定する。FREE_DESCRIPTOR_SET_BIT は付けない
    // (段階 3 phase は pool reset 一括戦略前提)。
    // INFO marker #2: "Shared descriptor pool created ..."
    bool createSharedDescriptorPool()
    {
        VkDescriptorPoolSize pool_sizes[2] = {};
        pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_sizes[0].descriptorCount = 16;
        pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[1].descriptorCount = 64;

        VkDescriptorPoolCreateInfo info = {};
        info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.maxSets       = 200;
        info.poolSizeCount = 2;
        info.pPoolSizes    = pool_sizes;

        VkResult result = vkCreateDescriptorPool(sDevice, &info, nullptr, &sSharedDescriptorPool);
        if (result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkCreateDescriptorPool (shared 3.4-β-1) failed: " << (S32)result << LL_ENDL;
            return false;
        }

        LL_INFOS("Vulkan") << "Shared descriptor pool created (3.4-β-1 placeholder, maxSets=200, "
                              "UBO=16, COMBINED_IMAGE_SAMPLER=64; precision deferred to 7.3)"
                           << LL_ENDL;
        return true;
    }

    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6α (W2):
    // per-asset (set=3) descriptor pool grow 機構実 wire up。LLAssetUboPool は
    // Vulkan device 非依存 bookkeeping algorithm = 実 pool 生成 / 破棄を caller
    // injected callback (PoolFactory / PoolDestroyer) 経由で受ける。本 helper で
    // sDevice / vkCreateDescriptorPool を closure capture した callback を渡し、
    // initialize() で起動時 prealloc 1 物理 pool 作成 + 以降 acquire 枯渇時
    // factory 再呼出で grow chunk pool 追加。FREE_DESCRIPTOR_SET_BIT 不要
    // (design 07 §6.4)。
    bool createAssetUboPool()
    {
        const U32 frames    = FRAMES_IN_FLIGHT;
        const U32 prealloc  = LLAssetUboPool::kPreallocAssetCount;
        const U32 grow      = LLAssetUboPool::kGrowChunkAssetCount;

        auto factory = [frames]() -> LLAssetUboPool::PoolHandle {
            if (sDevice == VK_NULL_HANDLE)
            {
                LL_WARNS("Vulkan") << "createAssetUboPool factory: sDevice == VK_NULL_HANDLE" << LL_ENDL;
                return 0;
            }

            VkDescriptorPoolSize pool_sizes[2] = {};
            pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            pool_sizes[0].descriptorCount = ASSET_POOL_UBO_BINDINGS_PER_ASSET *
                                            LLAssetUboPool::kPreallocAssetCount * frames;
            pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            pool_sizes[1].descriptorCount = ASSET_POOL_SAMPLER_BINDINGS_PER_ASSET *
                                            LLAssetUboPool::kPreallocAssetCount * frames;

            VkDescriptorPoolCreateInfo info = {};
            info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            info.maxSets       = LLAssetUboPool::kPreallocAssetCount * frames;
            info.poolSizeCount = 2;
            info.pPoolSizes    = pool_sizes;
            info.flags         = 0;

            VkDescriptorPool pool = VK_NULL_HANDLE;
            VkResult result = vkCreateDescriptorPool(sDevice, &info, nullptr, &pool);
            if (result != VK_SUCCESS)
            {
                LL_WARNS("Vulkan") << "vkCreateDescriptorPool (asset PC-6α) failed: "
                                   << (S32)result << LL_ENDL;
                return 0;
            }
            return reinterpret_cast<LLAssetUboPool::PoolHandle>(pool);
        };

        auto destroyer = [](LLAssetUboPool::PoolHandle handle) {
            if (handle == 0 || sDevice == VK_NULL_HANDLE)
            {
                return;
            }
            VkDescriptorPool pool = reinterpret_cast<VkDescriptorPool>(handle);
            vkDestroyDescriptorPool(sDevice, pool, nullptr);
        };

        sAssetUboPoolMgr = std::make_unique<LLAssetUboPool>(factory, destroyer, prealloc, grow);
        if (!sAssetUboPoolMgr->initialize())
        {
            LL_WARNS("Vulkan") << "LLAssetUboPool::initialize() failed (PC-6α)" << LL_ENDL;
            sAssetUboPoolMgr.reset();
            return false;
        }

        LL_INFOS("Vulkan") << "Asset UBO pool wired up (PC-6α W2, prealloc="
                           << prealloc << " asset × " << frames << " frame = "
                           << (prealloc * frames) << " set / pool, "
                           << "UBO=" << (ASSET_POOL_UBO_BINDINGS_PER_ASSET * prealloc * frames)
                           << ", SAMPLER=" << (ASSET_POOL_SAMPLER_BINDINGS_PER_ASSET * prealloc * frames)
                           << ", grow chunk=" << grow << " asset, pool count="
                           << sAssetUboPoolMgr->getPoolCount() << ")" << LL_ENDL;
        return true;
    }

    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6β (RB):
    // per-frame / per-pass cadence UBO ring buffer 実 wire up + cvar
    // AYARingBufferSizeMB 読込 hookup。LLUboRingBuffer algorithm 層に
    // (a) vmaCreateBuffer (HOST_VISIBLE + HOST_COHERENT + MAPPED) を closure
    // capture した BufferAllocator + (b) vmaDestroyBuffer destroyer + (c)
    // cvar 読込値 initial_mb (default 4) を ctor に渡し、initialize() で
    // 起動時 1 物理 buffer (= 4 MB / chunk size = 4 MB / 3 ≈ 1.33 MB) prealloc。
    // PC-6 後続で 5 cadence update site から allocate() / beginFrame() 呼出
    // が走り始める (= PC-6δ scope)。design 07 §7.2 / §7.3 / §7.5 / §8.4 整合、
    // cvar 値は静的 LLCachedControl<U32> で起動時 1 度 lookup (= settings.xml
    // PC-4 block comment 「変更には viewer 再起動が必要」と整合)。
    bool createDrawUboRingBuffer()
    {
        static LLCachedControl<U32> sRingBufferSizeMB(
            gSavedSettings, "AYARingBufferSizeMB", LLUboRingBuffer::kInitialSizeMB);
        const U32 initial_mb = (U32)sRingBufferSizeMB;

        auto factory = [](std::uint32_t size_bytes) -> LLUboRingBuffer::BufferHandle {
            if (sAllocator == VK_NULL_HANDLE)
            {
                LL_WARNS("Vulkan") << "createDrawUboRingBuffer factory: sAllocator == VK_NULL_HANDLE" << LL_ENDL;
                return 0;
            }

            VkBufferCreateInfo bci = {};
            bci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bci.size        = size_bytes;
            bci.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo aci = {};
            aci.usage         = VMA_MEMORY_USAGE_AUTO;
            aci.flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                              | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            aci.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            VkBuffer          buffer     = VK_NULL_HANDLE;
            VmaAllocation     allocation = VK_NULL_HANDLE;
            VmaAllocationInfo info       = {};
            VkResult r = vmaCreateBuffer(sAllocator, &bci, &aci, &buffer, &allocation, &info);
            if (r != VK_SUCCESS || info.pMappedData == nullptr)
            {
                LL_WARNS("Vulkan") << "createDrawUboRingBuffer vmaCreateBuffer failed: " << (S32)r
                                   << " size=" << (S32)size_bytes << LL_ENDL;
                if (buffer != VK_NULL_HANDLE)
                {
                    vmaDestroyBuffer(sAllocator, buffer, allocation);
                }
                return 0;
            }

            const LLUboRingBuffer::BufferHandle handle =
                static_cast<LLUboRingBuffer::BufferHandle>(reinterpret_cast<std::uintptr_t>(buffer));
            DrawUboRingBufferRecord rec;
            rec.buffer     = buffer;
            rec.allocation = allocation;
            rec.mapped     = info.pMappedData;
            sDrawUboRingBufferRecords[handle] = rec;
            return handle;
        };

        auto destroyer = [](LLUboRingBuffer::BufferHandle handle) {
            if (handle == 0 || sAllocator == VK_NULL_HANDLE)
            {
                return;
            }
            auto it = sDrawUboRingBufferRecords.find(handle);
            if (it == sDrawUboRingBufferRecords.end())
            {
                return;
            }
            vmaDestroyBuffer(sAllocator, it->second.buffer, it->second.allocation);
            sDrawUboRingBufferRecords.erase(it);
        };

        sDrawUboRingBufferMgr = std::make_unique<LLUboRingBuffer>(factory, destroyer, initial_mb);
        if (!sDrawUboRingBufferMgr->initialize())
        {
            LL_WARNS("Vulkan") << "LLUboRingBuffer::initialize() failed (PC-6β)" << LL_ENDL;
            sDrawUboRingBufferMgr.reset();
            return false;
        }

        LL_INFOS("Vulkan") << "Draw UBO ring buffer wired up (PC-6β RB, cvar AYARingBufferSizeMB="
                           << initial_mb << ", initial=" << sDrawUboRingBufferMgr->getCurrentSizeMB()
                           << " MB, max=" << sDrawUboRingBufferMgr->getMaxSizeMB()
                           << " MB, chunk=" << sDrawUboRingBufferMgr->getChunkSizeBytes()
                           << " B (×" << LLUboRingBuffer::kFramesInFlight
                           << " frame), alignment=" << sDrawUboRingBufferMgr->getAlignment()
                           << " B, HOST_VISIBLE + HOST_COHERENT + MAPPED)" << LL_ENDL;
        return true;
    }

    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6γ (PSC):
    // VkPipelineCache 用 disk-persist storage (= LLPipelineCacheStorage) を立ち上げる。
    // cvar AYAPipelineCacheSizeMB (default 64) を起動時 1 度 LLCachedControl 経由
    // で lookup し、64 MB cap として storage に注入。FileReader / FileWriter lambda
    // で std::ifstream / std::ofstream による file I/O を closure capture (algorithm
    // 層は file system 非依存維持)。initialize() で file 存在時 blob を load、
    // (e1) cap 超過時は破棄 = 起動初回 / cap 切替直後の 0 cache start。本 helper は
    // createPipelineCache() より前に init chain で呼出さなければならない (= blob を
    // VkPipelineCacheCreateInfo.pInitialData に投入するため、storage が先行)。
    // shutdown は shutdownVulkan() 内で vkGetPipelineCacheData → updateBlob →
    // persistToDisk → vkDestroyPipelineCache の順、本 helper では teardown 不要。
    // design 07 §9.3 (PSO cache 戦略) + §12 (PSC) 整合。
    bool createPipelineCacheStorage()
    {
        static LLCachedControl<U32> sPipelineCacheSizeMB(
            gSavedSettings, "AYAPipelineCacheSizeMB",
            LLPipelineCacheStorage::kDefaultMaxSizeMB);
        const U32 cap_mb = (U32)sPipelineCacheSizeMB;

        std::string file_path =
            gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "pipeline_cache.bin");

        auto reader = [](const std::string& path,
                         LLPipelineCacheStorage::CacheBlob& out) -> bool {
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
        };

        auto writer = [](const std::string& path,
                         const LLPipelineCacheStorage::CacheBlob& data) -> bool {
            std::ofstream f(path, std::ios::binary | std::ios::trunc);
            if (!f.is_open())
            {
                LL_WARNS("Vulkan") << "createPipelineCacheStorage writer: open failed path="
                                   << path << LL_ENDL;
                return false;
            }
            if (!data.empty())
            {
                f.write(reinterpret_cast<const char*>(data.data()),
                        static_cast<std::streamsize>(data.size()));
            }
            return f.good();
        };

        sPipelineCacheStorageMgr = std::make_unique<LLPipelineCacheStorage>(
            reader, writer, file_path, cap_mb);
        if (!sPipelineCacheStorageMgr->initialize())
        {
            LL_WARNS("Vulkan") << "LLPipelineCacheStorage::initialize() failed (PC-6γ)" << LL_ENDL;
            sPipelineCacheStorageMgr.reset();
            return false;
        }

        LL_INFOS("Vulkan") << "Pipeline cache storage wired up (PC-6γ PSC, cvar AYAPipelineCacheSizeMB="
                           << cap_mb << " MB, path=" << file_path
                           << ", initial blob=" << sPipelineCacheStorageMgr->getBlobSize()
                           << " bytes (" << sPipelineCacheStorageMgr->getBlobSizeMB()
                           << " MB), within limit="
                           << (sPipelineCacheStorageMgr->isWithinLimit() ? "yes" : "no")
                           << ")" << LL_ENDL;
        return true;
    }

    // r41 sub-step 3.4-γ (sub-doc 03 §3.1.4 / sub-doc 07 §1.2.1 set=1 / §3.1 sub-step 7.3 layout 部分内包):
    // set=1 per-material 7 PBR slot descriptor set layout 作成。binding 0..6 すべて
    // COMBINED_IMAGE_SAMPLER × 1、stage = FRAGMENT_BIT、immutable sampler は本 sub-step 範囲外
    // (per-material sampler 配信は 7.3 残置)。material params UBO 統合も 7.3 残置で本 sub-step 外。
    bool createPerMaterialDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding bindings[PER_MATERIAL_BINDING_COUNT] = {};
        for (U32 i = 0; i < PER_MATERIAL_BINDING_COUNT; ++i)
        {
            bindings[i].binding            = i;
            bindings[i].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[i].descriptorCount    = 1;
            bindings[i].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[i].pImmutableSamplers = nullptr;
        }

        VkDescriptorSetLayoutCreateInfo info = {};
        info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = PER_MATERIAL_BINDING_COUNT;
        info.pBindings    = bindings;

        VkResult result = vkCreateDescriptorSetLayout(sDevice, &info, nullptr, &sPerMaterialDescriptorSetLayout);
        if (result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkCreateDescriptorSetLayout (per-material set=1 3.4-γ) failed: "
                               << (S32)result << LL_ENDL;
            return false;
        }

        LL_INFOS("Vulkan") << "Per-material descriptor set layout created "
                              "(set=1, 7 PBR slot binding 0-6, COMBINED_IMAGE_SAMPLER × 7)"
                           << LL_ENDL;
        return true;
    }

    // r41 sub-step 3.4-γ (sub-doc 03 §3.1.4): 共用 placeholder sampler 1 件 (linear / clamp)。
    // 7 binding 全部に同 sampler を bind して transit smoke、per-texture / per-material sampler
    // (mipmap LOD bias / anisotropy / wrap / border color) は領域 7 sub-step 7.3 残置。
    bool createPlaceholderSampler()
    {
        VkSamplerCreateInfo info = {};
        info.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter    = VK_FILTER_LINEAR;
        info.minFilter    = VK_FILTER_LINEAR;
        info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.maxLod       = VK_LOD_CLAMP_NONE;

        VkResult result = vkCreateSampler(sDevice, &info, nullptr, &sPlaceholderSampler);
        if (result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkCreateSampler (placeholder 3.4-γ) failed: "
                               << (S32)result << LL_ENDL;
            return false;
        }
        return true;
    }

    // r41 sub-step 3.4-γ (sub-doc 03 §3.1.4): sSharedDescriptorPool から set=1 layout の
    // 1 set を allocate。material cache (~50 material × frame in flight 3 = 150 pool sizing) は
    // 領域 7 sub-step 7.3 残置、本 sub-step は 1 set transit smoke のみ。
    bool allocatePerMaterialDescriptorSet()
    {
        if (sSharedDescriptorPool == VK_NULL_HANDLE || sPerMaterialDescriptorSetLayout == VK_NULL_HANDLE)
        {
            LL_WARNS("Vulkan") << "allocatePerMaterialDescriptorSet: pool/layout not ready" << LL_ENDL;
            return false;
        }

        VkDescriptorSetAllocateInfo info = {};
        info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.descriptorPool     = sSharedDescriptorPool;
        info.descriptorSetCount = 1;
        info.pSetLayouts        = &sPerMaterialDescriptorSetLayout;

        VkResult result = vkAllocateDescriptorSets(sDevice, &info, &sPerMaterialDescriptorSet);
        if (result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkAllocateDescriptorSets (per-material 3.4-γ) failed: "
                               << (S32)result << LL_ENDL;
            return false;
        }
        return true;
    }

    // r41 sub-step 3.4-γ (sub-doc 03 §3.1.4): 7 binding 全部に同 VkImageView + sPlaceholderSampler を
    // bind して transit smoke (β-2 placeholder white texture × 7 binding 共用)。per-texture sampler /
    // mipmap / anisotropy 配信は領域 7 sub-step 7.3 残置、本 sub-step は VkImageView[7] 受け取り
    // (現状は呼出側で sPlaceholderWhiteImageView × 7 を渡す) で transit smoke 配線のみ。
    void updatePerMaterialDescriptorSet(const VkImageView image_views[PER_MATERIAL_BINDING_COUNT])
    {
        if (sPerMaterialDescriptorSet == VK_NULL_HANDLE || sPlaceholderSampler == VK_NULL_HANDLE)
        {
            return;
        }

        VkDescriptorImageInfo image_infos[PER_MATERIAL_BINDING_COUNT] = {};
        VkWriteDescriptorSet  writes[PER_MATERIAL_BINDING_COUNT]      = {};
        for (U32 i = 0; i < PER_MATERIAL_BINDING_COUNT; ++i)
        {
            image_infos[i].sampler     = sPlaceholderSampler;
            image_infos[i].imageView   = image_views[i];
            image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            writes[i].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet          = sPerMaterialDescriptorSet;
            writes[i].dstBinding      = i;
            writes[i].dstArrayElement = 0;
            writes[i].descriptorCount = 1;
            writes[i].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[i].pImageInfo      = &image_infos[i];
        }

        vkUpdateDescriptorSets(sDevice, PER_MATERIAL_BINDING_COUNT, writes, 0, nullptr);

        LL_INFOS("Vulkan") << "Per-material descriptor set transit smoke "
                              "(white placeholder × 7 binding, bind via vkCmdBindDescriptorSets)"
                           << LL_ENDL;
    }

    // r41 sub-step 3.4-γ (sub-doc 03 §3.1.4) / 3.4-δ-1 layout 引数化:
    // set=1 per-material descriptor set bind helper。caller 側 layout (sPlaceholderLayout for
    // beginFrame transit smoke / sSkySmokeLayout for recordPlaceholderPoolDraw) を受取、
    // firstSet=1 で sPerMaterialDescriptorSet を bind。両 layout は γ 二段構え準拠
    // (set=0 PerFrame + set=1 PerMaterial + push constant 64 B) で descriptor set 互換性確保。
    // in-frame guard / sCommandBuffer 有効性は caller 側前提、本 helper は handle null guard のみ。
    void bindPerMaterialDescriptorSet(VkCommandBuffer cmd_buf, VkPipelineLayout layout)
    {
        if (cmd_buf == VK_NULL_HANDLE ||
            layout == VK_NULL_HANDLE ||
            sPerMaterialDescriptorSet == VK_NULL_HANDLE)
        {
            return;
        }
        vkCmdBindDescriptorSets(cmd_buf,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                layout,
                                /*firstSet=*/1,
                                /*descriptorSetCount=*/1,
                                &sPerMaterialDescriptorSet,
                                /*dynamicOffsetCount=*/0,
                                /*pDynamicOffsets=*/nullptr);
    }

    // r41 sub-step 3.4-β-1 (sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.1 内包):
    // VMA budget smoke 1 回出力。VK_EXT_memory_budget 支援時は実 budget/usage を取得、
    // 未支援時は VMA fallback の累積 statistics を表示。INFO marker #3。
    // FRAMES_IN_FLIGHT loop 内で誤って毎フレーム呼ばないよう sSharedVmaBudgetLogged で 1 回固定。
    void logVmaBudgetSmoke()
    {
        if (sSharedVmaBudgetLogged || sAllocator == VK_NULL_HANDLE)
        {
            return;
        }
        sSharedVmaBudgetLogged = true;

        const VkPhysicalDeviceMemoryProperties* mem_props = nullptr;
        vmaGetMemoryProperties(sAllocator, &mem_props);
        const U32 heap_count = mem_props ? mem_props->memoryHeapCount : 0;

        if (heap_count == 0)
        {
            LL_INFOS("Vulkan") << "VMA budget smoke: heapCount=0 (skip)" << LL_ENDL;
            return;
        }

        std::vector<VmaBudget> budgets(heap_count);
        vmaGetHeapBudgets(sAllocator, budgets.data());

        LL_INFOS("Vulkan") << "VMA budget smoke (3.4-β-1 1 度のみ、heapCount=" << heap_count
                           << ", VK_EXT_memory_budget="
                           << (sDeviceLimits.memoryBudgetSupported ? "ON" : "OFF (fallback)")
                           << "):" << LL_ENDL;
        for (U32 i = 0; i < heap_count; ++i)
        {
            const VmaBudget& b = budgets[i];
            const U64 heap_size = mem_props->memoryHeaps[i].size;
            const bool is_device_local =
                (mem_props->memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0;
            LL_INFOS("Vulkan") << "  heap " << i
                               << (is_device_local ? " [DEVICE_LOCAL]" : " [HOST]")
                               << " size=" << (heap_size / (1024 * 1024)) << " MB"
                               << " budget=" << (b.budget / (1024 * 1024)) << " MB"
                               << " usage=" << (b.usage / (1024 * 1024)) << " MB"
                               << " (VMA allocations=" << b.statistics.allocationCount
                               << ", blocks=" << b.statistics.blockCount << ")"
                               << LL_ENDL;
        }
    }

    // r41 sub-step 3.4-β-2 (sub-doc 03 §3.1.4): LL GL internalformat → VkFormat 集約変換 impl。
    // llvkloader は GL header から独立するため、OpenGL spec 確定値を hex literal で照合。
    // 20 entry: 8/16/32-bit normalized + float + depth/stencil + sRGB + packed HDR
    // (sub-doc 07 §1.2.1 set=1 想定 7 PBR slot + LLImageGL 主要 internalformat 網羅)。
    VkFormat llGlEnumToVkFormatImpl(U32 ll_gl_intformat)
    {
        switch (ll_gl_intformat)
        {
            // 8-bit normalized (UNORM)
            case 0x8229: return VK_FORMAT_R8_UNORM;            // GL_R8
            case 0x822B: return VK_FORMAT_R8G8_UNORM;          // GL_RG8
            case 0x8051: return VK_FORMAT_R8G8B8_UNORM;        // GL_RGB8
            case 0x8058: return VK_FORMAT_R8G8B8A8_UNORM;      // GL_RGBA8

            // 16-bit normalized (UNORM)
            case 0x822A: return VK_FORMAT_R16_UNORM;           // GL_R16
            case 0x822C: return VK_FORMAT_R16G16_UNORM;        // GL_RG16
            case 0x805B: return VK_FORMAT_R16G16B16A16_UNORM;  // GL_RGBA16

            // 16-bit float (SFLOAT)
            case 0x822D: return VK_FORMAT_R16_SFLOAT;          // GL_R16F
            case 0x822F: return VK_FORMAT_R16G16_SFLOAT;       // GL_RG16F
            case 0x881B: return VK_FORMAT_R16G16B16_SFLOAT;    // GL_RGB16F
            case 0x881A: return VK_FORMAT_R16G16B16A16_SFLOAT; // GL_RGBA16F

            // 32-bit float (SFLOAT)
            case 0x822E: return VK_FORMAT_R32_SFLOAT;          // GL_R32F
            case 0x8230: return VK_FORMAT_R32G32_SFLOAT;       // GL_RG32F
            case 0x8814: return VK_FORMAT_R32G32B32A32_SFLOAT; // GL_RGBA32F

            // sRGB
            case 0x8C41: return VK_FORMAT_R8G8B8_SRGB;         // GL_SRGB8
            case 0x8C43: return VK_FORMAT_R8G8B8A8_SRGB;       // GL_SRGB8_ALPHA8

            // Depth / Stencil
            case 0x81A5: return VK_FORMAT_D16_UNORM;           // GL_DEPTH_COMPONENT16
            case 0x81A6: return VK_FORMAT_X8_D24_UNORM_PACK32; // GL_DEPTH_COMPONENT24
            case 0x8CAC: return VK_FORMAT_D32_SFLOAT;          // GL_DEPTH_COMPONENT32F
            case 0x88F0: return VK_FORMAT_D24_UNORM_S8_UINT;   // GL_DEPTH24_STENCIL8
            case 0x8CAD: return VK_FORMAT_D32_SFLOAT_S8_UINT;  // GL_DEPTH32F_STENCIL8

            // Packed HDR
            case 0x8C3A: return VK_FORMAT_B10G11R11_UFLOAT_PACK32; // GL_R11F_G11F_B10F

            default:     return VK_FORMAT_UNDEFINED;
        }
    }

    // r41 sub-step 3.4-β-2 (sub-doc 03 §3.1.4): 1×1 white placeholder texture lifecycle smoke。
    // vmaCreateImage + vkCreateImageView を初期化 path で 1 度発行、shutdownVulkan で破棄。
    // β-2-3 で staging upload + 2 段 layout transition を本関数の後段に追加予定。
    bool createPlaceholderWhiteImage()
    {
        if (sAllocator == VK_NULL_HANDLE || sDevice == VK_NULL_HANDLE)
        {
            LL_WARNS("Vulkan") << "createPlaceholderWhiteImage: allocator/device not ready" << LL_ENDL;
            return false;
        }

        VkImageCreateInfo image_ci{};
        image_ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_ci.imageType     = VK_IMAGE_TYPE_2D;
        image_ci.format        = VK_FORMAT_R8G8B8A8_UNORM;
        image_ci.extent.width  = 1;
        image_ci.extent.height = 1;
        image_ci.extent.depth  = 1;
        image_ci.mipLevels     = 1;
        image_ci.arrayLayers   = 1;
        image_ci.samples       = VK_SAMPLE_COUNT_1_BIT;
        image_ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
        image_ci.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo alloc_ci{};
        alloc_ci.usage = VMA_MEMORY_USAGE_AUTO;

        VkResult create_result = vmaCreateImage(sAllocator, &image_ci, &alloc_ci,
                                                &sPlaceholderWhiteImage,
                                                &sPlaceholderWhiteAllocation, nullptr);
        if (create_result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vmaCreateImage failed (placeholder white) result=" << create_result << LL_ENDL;
            sPlaceholderWhiteImage      = VK_NULL_HANDLE;
            sPlaceholderWhiteAllocation = VK_NULL_HANDLE;
            return false;
        }

        VkImageViewCreateInfo view_ci{};
        view_ci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_ci.image                           = sPlaceholderWhiteImage;
        view_ci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        view_ci.format                          = VK_FORMAT_R8G8B8A8_UNORM;
        view_ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        view_ci.subresourceRange.baseMipLevel   = 0;
        view_ci.subresourceRange.levelCount     = 1;
        view_ci.subresourceRange.baseArrayLayer = 0;
        view_ci.subresourceRange.layerCount     = 1;

        VkResult view_result = vkCreateImageView(sDevice, &view_ci, nullptr, &sPlaceholderWhiteImageView);
        if (view_result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkCreateImageView failed (placeholder white) result=" << view_result << LL_ENDL;
            vmaDestroyImage(sAllocator, sPlaceholderWhiteImage, sPlaceholderWhiteAllocation);
            sPlaceholderWhiteImage      = VK_NULL_HANDLE;
            sPlaceholderWhiteAllocation = VK_NULL_HANDLE;
            sPlaceholderWhiteImageView  = VK_NULL_HANDLE;
            return false;
        }

        return true;
    }

    // r41 sub-step 3.4-β-2-3 (sub-doc 03 §3.1.4): staging buffer 経由 1×1 white pixel upload。
    // 2 段 layout transition (UNDEFINED → TRANSFER_DST_OPTIMAL → SHADER_READ_ONLY_OPTIMAL) +
    // vkCmdCopyBufferToImage を 1 度限りの scratch command buffer (sCommandPool 由来) で実行、
    // vkQueueWaitIdle で完了同期後 staging buffer / scratch cb を即時破棄。
    bool uploadPlaceholderWhiteSmoke()
    {
        if (sAllocator == VK_NULL_HANDLE || sDevice == VK_NULL_HANDLE ||
            sCommandPool == VK_NULL_HANDLE || sGraphicsQueue == VK_NULL_HANDLE ||
            sPlaceholderWhiteImage == VK_NULL_HANDLE)
        {
            LL_WARNS("Vulkan") << "uploadPlaceholderWhiteSmoke: prerequisites missing" << LL_ENDL;
            return false;
        }

        // staging buffer (4 byte = 1px RGBA8) を host-visible で確保。
        const VkDeviceSize staging_size = 4;
        VkBuffer       staging_buf   = VK_NULL_HANDLE;
        VmaAllocation  staging_alloc = VK_NULL_HANDLE;

        VkBufferCreateInfo buf_ci{};
        buf_ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_ci.size        = staging_size;
        buf_ci.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo buf_alloc_ci{};
        buf_alloc_ci.usage = VMA_MEMORY_USAGE_AUTO;
        buf_alloc_ci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                             VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo staging_info{};
        VkResult buf_result = vmaCreateBuffer(sAllocator, &buf_ci, &buf_alloc_ci,
                                              &staging_buf, &staging_alloc, &staging_info);
        if (buf_result != VK_SUCCESS || staging_info.pMappedData == nullptr)
        {
            LL_WARNS("Vulkan") << "uploadPlaceholderWhiteSmoke: vmaCreateBuffer failed result="
                               << buf_result << LL_ENDL;
            if (staging_buf != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(sAllocator, staging_buf, staging_alloc);
            }
            return false;
        }

        // 1×1 white pixel (RGBA = 0xFF, 0xFF, 0xFF, 0xFF) 書込み。
        const U32 white_pixel = 0xFFFFFFFFu;
        memcpy(staging_info.pMappedData, &white_pixel, sizeof(white_pixel));

        // scratch one-time command buffer (sCommandPool 由来、submit 後即 free)。
        VkCommandBufferAllocateInfo cb_ai{};
        cb_ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cb_ai.commandPool        = sCommandPool;
        cb_ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cb_ai.commandBufferCount = 1;

        VkCommandBuffer cb = VK_NULL_HANDLE;
        VkResult cb_result = vkAllocateCommandBuffers(sDevice, &cb_ai, &cb);
        if (cb_result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "uploadPlaceholderWhiteSmoke: vkAllocateCommandBuffers failed result="
                               << cb_result << LL_ENDL;
            vmaDestroyBuffer(sAllocator, staging_buf, staging_alloc);
            return false;
        }

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cb, &begin_info);

        // Barrier 1: UNDEFINED → TRANSFER_DST_OPTIMAL
        VkImageMemoryBarrier to_xfer{};
        to_xfer.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        to_xfer.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        to_xfer.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_xfer.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        to_xfer.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        to_xfer.image                           = sPlaceholderWhiteImage;
        to_xfer.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        to_xfer.subresourceRange.baseMipLevel   = 0;
        to_xfer.subresourceRange.levelCount     = 1;
        to_xfer.subresourceRange.baseArrayLayer = 0;
        to_xfer.subresourceRange.layerCount     = 1;
        to_xfer.srcAccessMask                   = 0;
        to_xfer.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cb,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &to_xfer);

        // CopyBufferToImage
        VkBufferImageCopy copy_region{};
        copy_region.bufferOffset                    = 0;
        copy_region.bufferRowLength                 = 0;
        copy_region.bufferImageHeight               = 0;
        copy_region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_region.imageSubresource.mipLevel       = 0;
        copy_region.imageSubresource.baseArrayLayer = 0;
        copy_region.imageSubresource.layerCount     = 1;
        copy_region.imageOffset                     = {0, 0, 0};
        copy_region.imageExtent                     = {1, 1, 1};
        vkCmdCopyBufferToImage(cb, staging_buf, sPlaceholderWhiteImage,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

        // Barrier 2: TRANSFER_DST_OPTIMAL → SHADER_READ_ONLY_OPTIMAL
        VkImageMemoryBarrier to_read{};
        to_read.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        to_read.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_read.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        to_read.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        to_read.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        to_read.image                           = sPlaceholderWhiteImage;
        to_read.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        to_read.subresourceRange.baseMipLevel   = 0;
        to_read.subresourceRange.levelCount     = 1;
        to_read.subresourceRange.baseArrayLayer = 0;
        to_read.subresourceRange.layerCount     = 1;
        to_read.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_read.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cb,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &to_read);

        vkEndCommandBuffer(cb);

        VkSubmitInfo submit{};
        submit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers    = &cb;
        VkResult submit_result = vkQueueSubmit(sGraphicsQueue, 1, &submit, VK_NULL_HANDLE);
        if (submit_result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "uploadPlaceholderWhiteSmoke: vkQueueSubmit failed result="
                               << submit_result << LL_ENDL;
            vkFreeCommandBuffers(sDevice, sCommandPool, 1, &cb);
            vmaDestroyBuffer(sAllocator, staging_buf, staging_alloc);
            return false;
        }
        vkQueueWaitIdle(sGraphicsQueue);

        vkFreeCommandBuffers(sDevice, sCommandPool, 1, &cb);
        vmaDestroyBuffer(sAllocator, staging_buf, staging_alloc);

        LL_INFOS("Vulkan") << "VkImage placeholder lifecycle smoke (1x1 white、"
                              "VkFormat=R8G8B8A8_UNORM、image+view+destroy 一連 OK)" << LL_ENDL;
        return true;
    }

    void destroyPlaceholderWhiteImage()
    {
        if (sPlaceholderWhiteImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(sDevice, sPlaceholderWhiteImageView, nullptr);
            sPlaceholderWhiteImageView = VK_NULL_HANDLE;
        }
        if (sPlaceholderWhiteImage != VK_NULL_HANDLE)
        {
            vmaDestroyImage(sAllocator, sPlaceholderWhiteImage, sPlaceholderWhiteAllocation);
            sPlaceholderWhiteImage      = VK_NULL_HANDLE;
            sPlaceholderWhiteAllocation = VK_NULL_HANDLE;
        }
    }

    // r41 sub-step 3.3-β-2: per-frame matrix UBO descriptor set layout (sub-doc 03 §3.1.1)
    // binding 0 = PerFrameMatrixUBO (192 B, VERTEX|FRAGMENT)
    // binding 1 = TextureMatrixUBO  (256 B, VERTEX|FRAGMENT)
    bool createPerFrameDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding bindings[2] = {};
        bindings[0].binding         = 0;
        bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[1].binding         = 1;
        bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo info = {};
        info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 2;
        info.pBindings    = bindings;

        VkResult result = vkCreateDescriptorSetLayout(sDevice, &info, nullptr, &sPerFrameDescriptorSetLayout);
        if (result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkCreateDescriptorSetLayout (per-frame) failed: " << (S32)result << LL_ENDL;
            return false;
        }

        LL_INFOS("Vulkan") << "Per-frame descriptor set layout created (binding 0=PerFrameMatrixUBO, 1=TextureMatrixUBO)" << LL_ENDL;
        return true;
    }

    // r41 sub-step 3.3-β-2: per-frame UBO buffer × FRAMES_IN_FLIGHT
    // 単一 VkBuffer (512 B) に PerFrame (offset 0 / 192 B) + Texture (offset 256 / 256 B) 配置、
    // HOST_VISIBLE_COHERENT + persistent mapping、起動時 zero write。
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
                LL_WARNS("Vulkan") << "vkCreateBuffer (per-frame UBO " << frame << ") failed: " << (S32)result << LL_ENDL;
                return false;
            }

            VkMemoryRequirements mem_req;
            vkGetBufferMemoryRequirements(sDevice, sPerFrameUboBuffer[frame], &mem_req);

            S32 mem_type = findMemoryType(mem_req.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            if (mem_type < 0)
            {
                LL_WARNS("Vulkan") << "No HOST_VISIBLE_COHERENT memory type for per-frame UBO" << LL_ENDL;
                return false;
            }

            VkMemoryAllocateInfo alloc_info = {};
            alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize  = mem_req.size;
            alloc_info.memoryTypeIndex = (U32)mem_type;

            result = vkAllocateMemory(sDevice, &alloc_info, nullptr, &sPerFrameUboMemory[frame]);
            if (result != VK_SUCCESS)
            {
                LL_WARNS("Vulkan") << "vkAllocateMemory (per-frame UBO " << frame << ") failed: " << (S32)result << LL_ENDL;
                return false;
            }

            result = vkBindBufferMemory(sDevice, sPerFrameUboBuffer[frame], sPerFrameUboMemory[frame], 0);
            if (result != VK_SUCCESS)
            {
                LL_WARNS("Vulkan") << "vkBindBufferMemory (per-frame UBO " << frame << ") failed: " << (S32)result << LL_ENDL;
                return false;
            }

            result = vkMapMemory(sDevice, sPerFrameUboMemory[frame], 0, VK_WHOLE_SIZE, 0, &sPerFrameUboMapped[frame]);
            if (result != VK_SUCCESS)
            {
                LL_WARNS("Vulkan") << "vkMapMemory (per-frame UBO " << frame << ") failed: " << (S32)result << LL_ENDL;
                return false;
            }

            // 初期 zero write smoke (β-2 完了 marker)
            std::memset(sPerFrameUboMapped[frame], 0, (size_t)UBO_BUFFER_SIZE_FRAME);
        }

        LL_INFOS("Vulkan") << "Per-frame UBO buffers created (" << FRAMES_IN_FLIGHT
                           << " frames × " << UBO_BUFFER_SIZE_FRAME
                           << " B, HOST_VISIBLE_COHERENT + persistent map + zero write)" << LL_ENDL;
        return true;
    }

    // r41 sub-step 3.3-β-2: descriptor pool (3 set × 2 binding) + 3 set alloc + vkUpdateDescriptorSets
    bool createPerFrameDescriptorSets()
    {
        VkDescriptorPoolSize pool_size = {};
        pool_size.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_size.descriptorCount = FRAMES_IN_FLIGHT * 2;  // 2 binding × 3 set

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.maxSets       = FRAMES_IN_FLIGHT;
        pool_info.poolSizeCount = 1;
        pool_info.pPoolSizes    = &pool_size;

        VkResult result = vkCreateDescriptorPool(sDevice, &pool_info, nullptr, &sPerFrameDescriptorPool);
        if (result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkCreateDescriptorPool (per-frame) failed: " << (S32)result << LL_ENDL;
            return false;
        }

        VkDescriptorSetLayout layouts[FRAMES_IN_FLIGHT] = {
            sPerFrameDescriptorSetLayout,
            sPerFrameDescriptorSetLayout,
            sPerFrameDescriptorSetLayout,
        };

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool     = sPerFrameDescriptorPool;
        alloc_info.descriptorSetCount = FRAMES_IN_FLIGHT;
        alloc_info.pSetLayouts        = layouts;

        result = vkAllocateDescriptorSets(sDevice, &alloc_info, sPerFrameDescriptorSet);
        if (result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkAllocateDescriptorSets (per-frame) failed: " << (S32)result << LL_ENDL;
            return false;
        }

        for (U32 frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            VkDescriptorBufferInfo perframe_info = {};
            perframe_info.buffer = sPerFrameUboBuffer[frame];
            perframe_info.offset = PERFRAME_UBO_OFFSET;
            perframe_info.range  = PERFRAME_UBO_SIZE;

            VkDescriptorBufferInfo texture_info = {};
            texture_info.buffer = sPerFrameUboBuffer[frame];
            texture_info.offset = TEXTURE_UBO_OFFSET;
            texture_info.range  = TEXTURE_UBO_SIZE;

            VkWriteDescriptorSet writes[2] = {};
            writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet          = sPerFrameDescriptorSet[frame];
            writes[0].dstBinding      = 0;
            writes[0].descriptorCount = 1;
            writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[0].pBufferInfo     = &perframe_info;

            writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet          = sPerFrameDescriptorSet[frame];
            writes[1].dstBinding      = 1;
            writes[1].descriptorCount = 1;
            writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[1].pBufferInfo     = &texture_info;

            vkUpdateDescriptorSets(sDevice, 2, writes, 0, nullptr);
        }

        LL_INFOS("Vulkan") << "Per-frame descriptor sets allocated + updated ("
                           << FRAMES_IN_FLIGHT << " sets × 2 binding)" << LL_ENDL;
        return true;
    }

    // r41 sub-step 3.1b item #8: minimal placeholder PSO (sky pool placeholder)。
    // GLSL source は本 source の上の comment block で sealed (offline glslc compile)、
    // vert/frag SPIR-V を C++ const array で embed。本 PSO は sRenderPass (1 color attachment) 互換、
    // vertex input 無し / depth test 無し / blend 無し / 1 subpass。
    //
    // GLSL source (offline compiled、glslc 2023.8 / Target: SPIR-V 1.0):
    //   placeholder.vert: #version 450  void main() { gl_Position = vec4(0); }
    //   placeholder.frag: #version 450  layout(location=0) out vec4 outColor;
    //                     void main() { outColor = vec4(0); }
    static const uint32_t kPlaceholderVertSpv[] = {
        0x07230203, 0x00010000, 0x000d000b, 0x00000015, 0x00000000, 0x00020011,
        0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
        0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0006000f, 0x00000000,
        0x00000004, 0x6e69616d, 0x00000000, 0x0000000d, 0x00030003, 0x00000002,
        0x000001c2, 0x000a0004, 0x475f4c47, 0x4c474f4f, 0x70635f45, 0x74735f70,
        0x5f656c79, 0x656e696c, 0x7269645f, 0x69746365, 0x00006576, 0x00080004,
        0x475f4c47, 0x4c474f4f, 0x6e695f45, 0x64756c63, 0x69645f65, 0x74636572,
        0x00657669, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00060005,
        0x0000000b, 0x505f6c67, 0x65567265, 0x78657472, 0x00000000, 0x00060006,
        0x0000000b, 0x00000000, 0x505f6c67, 0x7469736f, 0x006e6f69, 0x00070006,
        0x0000000b, 0x00000001, 0x505f6c67, 0x746e696f, 0x657a6953, 0x00000000,
        0x00070006, 0x0000000b, 0x00000002, 0x435f6c67, 0x4470696c, 0x61747369,
        0x0065636e, 0x00070006, 0x0000000b, 0x00000003, 0x435f6c67, 0x446c6c75,
        0x61747369, 0x0065636e, 0x00030005, 0x0000000d, 0x00000000, 0x00050048,
        0x0000000b, 0x00000000, 0x0000000b, 0x00000000, 0x00050048, 0x0000000b,
        0x00000001, 0x0000000b, 0x00000001, 0x00050048, 0x0000000b, 0x00000002,
        0x0000000b, 0x00000003, 0x00050048, 0x0000000b, 0x00000003, 0x0000000b,
        0x00000004, 0x00030047, 0x0000000b, 0x00000002, 0x00020013, 0x00000002,
        0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020,
        0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040015, 0x00000008,
        0x00000020, 0x00000000, 0x0004002b, 0x00000008, 0x00000009, 0x00000001,
        0x0004001c, 0x0000000a, 0x00000006, 0x00000009, 0x0006001e, 0x0000000b,
        0x00000007, 0x00000006, 0x0000000a, 0x0000000a, 0x00040020, 0x0000000c,
        0x00000003, 0x0000000b, 0x0004003b, 0x0000000c, 0x0000000d, 0x00000003,
        0x00040015, 0x0000000e, 0x00000020, 0x00000001, 0x0004002b, 0x0000000e,
        0x0000000f, 0x00000000, 0x0004002b, 0x00000006, 0x00000010, 0x00000000,
        0x0004002b, 0x00000006, 0x00000011, 0x3f800000, 0x0007002c, 0x00000007,
        0x00000012, 0x00000010, 0x00000010, 0x00000010, 0x00000011, 0x00040020,
        0x00000013, 0x00000003, 0x00000007, 0x00050036, 0x00000002, 0x00000004,
        0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x00050041, 0x00000013,
        0x00000014, 0x0000000d, 0x0000000f, 0x0003003e, 0x00000014, 0x00000012,
        0x000100fd, 0x00010038,
    };

    static const uint32_t kPlaceholderFragSpv[] = {
        0x07230203, 0x00010000, 0x000d000b, 0x0000000c, 0x00000000, 0x00020011,
        0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
        0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0006000f, 0x00000004,
        0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x00030010, 0x00000004,
        0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x000a0004, 0x475f4c47,
        0x4c474f4f, 0x70635f45, 0x74735f70, 0x5f656c79, 0x656e696c, 0x7269645f,
        0x69746365, 0x00006576, 0x00080004, 0x475f4c47, 0x4c474f4f, 0x6e695f45,
        0x64756c63, 0x69645f65, 0x74636572, 0x00657669, 0x00040005, 0x00000004,
        0x6e69616d, 0x00000000, 0x00050005, 0x00000009, 0x4374756f, 0x726f6c6f,
        0x00000000, 0x00040047, 0x00000009, 0x0000001e, 0x00000000, 0x00020013,
        0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006,
        0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020,
        0x00000008, 0x00000003, 0x00000007, 0x0004003b, 0x00000008, 0x00000009,
        0x00000003, 0x0004002b, 0x00000006, 0x0000000a, 0x00000000, 0x0007002c,
        0x00000007, 0x0000000b, 0x0000000a, 0x0000000a, 0x0000000a, 0x0000000a,
        0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8,
        0x00000005, 0x0003003e, 0x00000009, 0x0000000b, 0x000100fd, 0x00010038,
    };

    bool createPlaceholderPipeline()
    {
        VkShaderModuleCreateInfo vs_info = {};
        vs_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vs_info.codeSize = sizeof(kPlaceholderVertSpv);
        vs_info.pCode    = kPlaceholderVertSpv;
        if (vkCreateShaderModule(sDevice, &vs_info, nullptr, &sPlaceholderVertModule) != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "Placeholder vertex shader module create failed" << LL_ENDL;
            return false;
        }

        VkShaderModuleCreateInfo fs_info = {};
        fs_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        fs_info.codeSize = sizeof(kPlaceholderFragSpv);
        fs_info.pCode    = kPlaceholderFragSpv;
        if (vkCreateShaderModule(sDevice, &fs_info, nullptr, &sPlaceholderFragModule) != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "Placeholder fragment shader module create failed" << LL_ENDL;
            return false;
        }

        // r41 sub-step 3.4-γ: 二段構え準拠 layout を set=0 PerFrame + set=1 PerMaterial に拡張
        // (sub-doc 03 §3.1.4 / sub-doc 07 §1.2.1)。push constant range 0..64 B / VERTEX_BIT (modelview_matrix)
        // は 3.3-γ 設計継承。placeholder SPIR-V は set=1 binding 未参照のため shader 改変不要
        // (Vulkan は layout が含む set 番号より shader 参照が少ない構成を許容、validation 0 件)。
        VkDescriptorSetLayout set_layouts[2]     = { sPerFrameDescriptorSetLayout, sPerMaterialDescriptorSetLayout };
        VkPushConstantRange   push_constants[1]  = {};
        push_constants[0].stageFlags             = VK_SHADER_STAGE_VERTEX_BIT;
        push_constants[0].offset                 = 0;
        push_constants[0].size                   = 64; // mat4 modelview_matrix
        sPlaceholderLayout = createStandardPipelineLayout(set_layouts, 2, push_constants, 1);
        if (sPlaceholderLayout == VK_NULL_HANDLE)
        {
            LL_WARNS("Vulkan") << "Placeholder pipeline layout create failed" << LL_ENDL;
            return false;
        }

        VkPipelineShaderStageCreateInfo stages[2] = {};
        stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = sPlaceholderVertModule;
        stages[0].pName  = "main";
        stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = sPlaceholderFragModule;
        stages[1].pName  = "main";

        VkPipelineVertexInputStateCreateInfo vi = {};
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkPipelineInputAssemblyStateCreateInfo ia = {};
        ia.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkViewport viewport = { 0.0f, 0.0f, (F32)OFFSCREEN_WIDTH, (F32)OFFSCREEN_HEIGHT, 0.0f, 1.0f };
        VkRect2D   scissor  = { { 0, 0 }, { OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT } };

        VkPipelineViewportStateCreateInfo vp = {};
        vp.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vp.viewportCount = 1;
        vp.pViewports    = &viewport;
        vp.scissorCount  = 1;
        vp.pScissors     = &scissor;

        VkPipelineRasterizationStateCreateInfo rs = {};
        rs.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode    = VK_CULL_MODE_NONE;
        rs.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rs.lineWidth   = 1.0f;

        VkPipelineMultisampleStateCreateInfo ms = {};
        ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState cba = {};
        cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                           | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo cb = {};
        cb.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.attachmentCount = 1;
        cb.pAttachments    = &cba;

        VkGraphicsPipelineCreateInfo ci = {};
        ci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        ci.stageCount          = 2;
        ci.pStages             = stages;
        ci.pVertexInputState   = &vi;
        ci.pInputAssemblyState = &ia;
        ci.pViewportState      = &vp;
        ci.pRasterizationState = &rs;
        ci.pMultisampleState   = &ms;
        ci.pColorBlendState    = &cb;
        ci.layout              = sPlaceholderLayout;
        ci.renderPass          = sRenderPass;
        ci.subpass             = 0;

        if (!compileGraphicsPipeline(ci, sPlaceholderPipeline))
        {
            LL_WARNS("Vulkan") << "Placeholder graphics pipeline compile failed" << LL_ENDL;
            return false;
        }

        LL_INFOS("Vulkan") << "Placeholder PSO compiled (vert " << sizeof(kPlaceholderVertSpv)
                           << " B / frag " << sizeof(kPlaceholderFragSpv)
                           << " B, sky pool placeholder for sub-doc 03 §3.1 acceptance)" << LL_ENDL;
        return true;
    }

    // r41 sub-step 3.3-B-γ: SPIR-V file loader helper (sub-doc 03 §3.1.3)
    // β-2 で確立した build chain (glslangValidator → .spv → viewer_manifest deploy) で
    // packaged dir 配置の .spv binary を読込み → loadSpirvShaderModule 経由で
    // VkShaderModule 化。caller が vkDestroyShaderModule で破棄、PSO compile 後は安全に破棄可。
    VkShaderModule loadSpirvShaderModuleFromFile(const char* rel_path)
    {
        const std::string path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, rel_path);
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f)
        {
            LL_WARNS("Vulkan") << "SPIR-V file not found: " << path << LL_ENDL;
            return VK_NULL_HANDLE;
        }
        const std::streamsize size = f.tellg();
        if (size <= 0 || (size % 4) != 0)
        {
            LL_WARNS("Vulkan") << "SPIR-V bad size: " << path
                               << " size=" << (S32)size << LL_ENDL;
            return VK_NULL_HANDLE;
        }
        f.seekg(0, std::ios::beg);
        std::vector<U32> spv(static_cast<size_t>(size) / 4);
        if (!f.read(reinterpret_cast<char*>(spv.data()), size))
        {
            LL_WARNS("Vulkan") << "SPIR-V read failed: " << path << LL_ENDL;
            return VK_NULL_HANDLE;
        }
        return loadSpirvShaderModule(spv.data(), static_cast<size_t>(size));
    }

#ifdef AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK
    // r41 sub-step 3.3-B-γ: Mac/Win build host で glslangValidator 不在時の embedded fallback (sub-doc 03 §3.1.3)。
    // Linux build (glslangValidator 検出時) では cmake が macro 未定義化 = dead code として binary から除外。
    // (元 r41 sub-step 3.2 smoke-test SPIR-V、refine 2026-05-29、GLSL source: fullscreen triangle vert +
    //  定数色 frag = vec4(0.4, 0.6, 0.9, 1.0)、descriptor/push constant 不要。)
    static const uint32_t kSkySmokeVertSpv[] = {
        0x07230203, 0x00010000, 0x000d000b, 0x0000002c, 0x00000000, 0x00020011,
        0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
        0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0007000f, 0x00000000,
        0x00000004, 0x6e69616d, 0x00000000, 0x0000000c, 0x0000001d, 0x00040047,
        0x0000000c, 0x0000000b, 0x0000002a, 0x00050048, 0x0000001b, 0x00000000,
        0x0000000b, 0x00000000, 0x00050048, 0x0000001b, 0x00000001, 0x0000000b,
        0x00000001, 0x00050048, 0x0000001b, 0x00000002, 0x0000000b, 0x00000003,
        0x00050048, 0x0000001b, 0x00000003, 0x0000000b, 0x00000004, 0x00030047,
        0x0000001b, 0x00000002, 0x00020013, 0x00000002, 0x00030021, 0x00000003,
        0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007,
        0x00000006, 0x00000002, 0x00040015, 0x0000000a, 0x00000020, 0x00000001,
        0x00040020, 0x0000000b, 0x00000001, 0x0000000a, 0x0004003b, 0x0000000b,
        0x0000000c, 0x00000001, 0x0004002b, 0x0000000a, 0x0000000e, 0x00000001,
        0x0004002b, 0x0000000a, 0x00000010, 0x00000002, 0x00040017, 0x00000017,
        0x00000006, 0x00000004, 0x00040015, 0x00000018, 0x00000020, 0x00000000,
        0x0004002b, 0x00000018, 0x00000019, 0x00000001, 0x0004001c, 0x0000001a,
        0x00000006, 0x00000019, 0x0006001e, 0x0000001b, 0x00000017, 0x00000006,
        0x0000001a, 0x0000001a, 0x00040020, 0x0000001c, 0x00000003, 0x0000001b,
        0x0004003b, 0x0000001c, 0x0000001d, 0x00000003, 0x0004002b, 0x0000000a,
        0x0000001e, 0x00000000, 0x0004002b, 0x00000006, 0x00000020, 0x40000000,
        0x0004002b, 0x00000006, 0x00000022, 0x3f800000, 0x0004002b, 0x00000006,
        0x00000025, 0x00000000, 0x00040020, 0x00000029, 0x00000003, 0x00000017,
        0x0005002c, 0x00000007, 0x0000002b, 0x00000022, 0x00000022, 0x00050036,
        0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005,
        0x0004003d, 0x0000000a, 0x0000000d, 0x0000000c, 0x000500c4, 0x0000000a,
        0x0000000f, 0x0000000d, 0x0000000e, 0x000500c7, 0x0000000a, 0x00000011,
        0x0000000f, 0x00000010, 0x0004006f, 0x00000006, 0x00000012, 0x00000011,
        0x000500c7, 0x0000000a, 0x00000014, 0x0000000d, 0x00000010, 0x0004006f,
        0x00000006, 0x00000015, 0x00000014, 0x00050050, 0x00000007, 0x00000016,
        0x00000012, 0x00000015, 0x0005008e, 0x00000007, 0x00000021, 0x00000016,
        0x00000020, 0x00050083, 0x00000007, 0x00000024, 0x00000021, 0x0000002b,
        0x00050051, 0x00000006, 0x00000026, 0x00000024, 0x00000000, 0x00050051,
        0x00000006, 0x00000027, 0x00000024, 0x00000001, 0x00070050, 0x00000017,
        0x00000028, 0x00000026, 0x00000027, 0x00000025, 0x00000022, 0x00050041,
        0x00000029, 0x0000002a, 0x0000001d, 0x0000001e, 0x0003003e, 0x0000002a,
        0x00000028, 0x000100fd, 0x00010038,
    };

    static const uint32_t kSkySmokeFragSpv[] = {
        0x07230203, 0x00010000, 0x000d000b, 0x0000000f, 0x00000000, 0x00020011,
        0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
        0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0006000f, 0x00000004,
        0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x00030010, 0x00000004,
        0x00000007, 0x00040047, 0x00000009, 0x0000001e, 0x00000000, 0x00020013,
        0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006,
        0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020,
        0x00000008, 0x00000003, 0x00000007, 0x0004003b, 0x00000008, 0x00000009,
        0x00000003, 0x0004002b, 0x00000006, 0x0000000a, 0x3ecccccd, 0x0004002b,
        0x00000006, 0x0000000b, 0x3f19999a, 0x0004002b, 0x00000006, 0x0000000c,
        0x3f666666, 0x0004002b, 0x00000006, 0x0000000d, 0x3f800000, 0x0007002c,
        0x00000007, 0x0000000e, 0x0000000a, 0x0000000b, 0x0000000c, 0x0000000d,
        0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8,
        0x00000005, 0x0003003e, 0x00000009, 0x0000000e, 0x000100fd, 0x00010038,
    };
#endif // AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK

    bool createSkySmokePipeline()
    {
        // r41 sub-step 3.3-B-γ: SPIR-V build chain 経由 file load primary (sub-doc 03 §3.1.3)。
        // β-2 build chain (glslangValidator → .spv → viewer_manifest deploy) で同梱した
        // sky_placeholder{V,F}.spv を loadSpirvShaderModuleFromFile 経由で読込み。
        // Mac/Win build host で glslangValidator 不在時 (cmake が AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK 定義)
        // は embedded byte array (kSkySmokeVertSpv/kSkySmokeFragSpv) で fallback。
        sSkySmokeVertModule = loadSpirvShaderModuleFromFile("shaders/aya_r41_exemplar/sky_placeholderV.spv");
        sSkySmokeFragModule = loadSpirvShaderModuleFromFile("shaders/aya_r41_exemplar/sky_placeholderF.spv");

#ifdef AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK
        if (sSkySmokeVertModule == VK_NULL_HANDLE)
        {
            LL_INFOS("Vulkan") << "Sky smoke vert: file load failed, using embedded byte array fallback" << LL_ENDL;
            VkShaderModuleCreateInfo vs_info = {};
            vs_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            vs_info.codeSize = sizeof(kSkySmokeVertSpv);
            vs_info.pCode    = kSkySmokeVertSpv;
            vkCreateShaderModule(sDevice, &vs_info, nullptr, &sSkySmokeVertModule);
        }
        if (sSkySmokeFragModule == VK_NULL_HANDLE)
        {
            LL_INFOS("Vulkan") << "Sky smoke frag: file load failed, using embedded byte array fallback" << LL_ENDL;
            VkShaderModuleCreateInfo fs_info = {};
            fs_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            fs_info.codeSize = sizeof(kSkySmokeFragSpv);
            fs_info.pCode    = kSkySmokeFragSpv;
            vkCreateShaderModule(sDevice, &fs_info, nullptr, &sSkySmokeFragModule);
        }
#endif

        if (sSkySmokeVertModule == VK_NULL_HANDLE)
        {
            LL_WARNS("Vulkan") << "Sky smoke vertex shader module create failed" << LL_ENDL;
            return false;
        }
        if (sSkySmokeFragModule == VK_NULL_HANDLE)
        {
            LL_WARNS("Vulkan") << "Sky smoke fragment shader module create failed" << LL_ENDL;
            return false;
        }

        // r41 sub-step 3.4-γ: 二段構え準拠 layout を set=0 PerFrame + set=1 PerMaterial に拡張
        // (sub-doc 03 §3.1.4 / sub-doc 07 §1.2.1)。push constant range 0..64 B / VERTEX_BIT (modelview_matrix)
        // は 3.3-γ 設計継承。sky smoke SPIR-V は set=1 binding 未参照のため shader 改変不要
        // (sub-step 3.4-δ-1 で recordPlaceholderPoolDraw 経由 12 pool hook body に組込)。
        VkDescriptorSetLayout set_layouts[2]     = { sPerFrameDescriptorSetLayout, sPerMaterialDescriptorSetLayout };
        VkPushConstantRange   push_constants[1]  = {};
        push_constants[0].stageFlags             = VK_SHADER_STAGE_VERTEX_BIT;
        push_constants[0].offset                 = 0;
        push_constants[0].size                   = 64; // mat4 modelview_matrix
        sSkySmokeLayout = createStandardPipelineLayout(set_layouts, 2, push_constants, 1);
        if (sSkySmokeLayout == VK_NULL_HANDLE)
        {
            LL_WARNS("Vulkan") << "Sky smoke pipeline layout create failed" << LL_ENDL;
            return false;
        }

        VkPipelineShaderStageCreateInfo stages[2] = {};
        stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = sSkySmokeVertModule;
        stages[0].pName  = "main";
        stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = sSkySmokeFragModule;
        stages[1].pName  = "main";

        VkPipelineVertexInputStateCreateInfo vi = {};
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkPipelineInputAssemblyStateCreateInfo ia = {};
        ia.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // Dynamic viewport/scissor: 実 swapchain サイズに合わせて recordPoolDraws 内で
        // vkCmdSetViewport/Scissor 配信予定 (本 sub-step では offscreen 64x64 を流用、
        // sub-step 3.3 で swapchain 配線時に dynamic 化を活かす)。
        VkViewport viewport = { 0.0f, 0.0f, (F32)OFFSCREEN_WIDTH, (F32)OFFSCREEN_HEIGHT, 0.0f, 1.0f };
        VkRect2D   scissor  = { { 0, 0 }, { OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT } };

        VkPipelineViewportStateCreateInfo vp = {};
        vp.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vp.viewportCount = 1;
        vp.pViewports    = &viewport;
        vp.scissorCount  = 1;
        vp.pScissors     = &scissor;

        VkPipelineRasterizationStateCreateInfo rs = {};
        rs.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode    = VK_CULL_MODE_NONE;
        rs.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rs.lineWidth   = 1.0f;

        VkPipelineMultisampleStateCreateInfo ms = {};
        ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo ds = {};
        ds.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds.depthTestEnable  = VK_FALSE;
        ds.depthWriteEnable = VK_FALSE;
        ds.depthCompareOp   = VK_COMPARE_OP_ALWAYS;

        VkPipelineColorBlendAttachmentState cba = {};
        cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                           | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo cb = {};
        cb.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.attachmentCount = 1;
        cb.pAttachments    = &cba;

        VkGraphicsPipelineCreateInfo ci = {};
        ci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        ci.stageCount          = 2;
        ci.pStages             = stages;
        ci.pVertexInputState   = &vi;
        ci.pInputAssemblyState = &ia;
        ci.pViewportState      = &vp;
        ci.pRasterizationState = &rs;
        ci.pMultisampleState   = &ms;
        ci.pDepthStencilState  = &ds;
        ci.pColorBlendState    = &cb;
        ci.layout              = sSkySmokeLayout;
        ci.renderPass          = sRenderPass;
        ci.subpass             = 0;

        if (!compileGraphicsPipeline(ci, sSkySmokePipeline))
        {
            LL_WARNS("Vulkan") << "Sky smoke graphics pipeline compile failed" << LL_ENDL;
            return false;
        }

        LL_INFOS("Vulkan") << "Sky smoke PSO compiled via SPIR-V build chain (sub-step 3.3-B-γ)" << LL_ENDL;
        LL_INFOS("Vulkan") << "Sky placeholder vert binding active (PerFrameMatrixUBO + push constant modelview)" << LL_ENDL;
        return true;
    }

    // r41 sub-step 3.4-δ-4 (sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.4):
    // set=2 binding 0 = STORAGE_BUFFER (mat4 × N bones)、VERTEX_BIT、
    // VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR flag (pool 不要、
    // vkCmdPushDescriptorSetKHR で in-frame 投入)。VK_KHR_push_descriptor 未支援
    // device では skip (success return)、recordAvatarPlaceholderDraw 側で fallback。
    bool createAvatarBoneDescriptorSetLayout()
    {
        if (!sDeviceLimits.pushDescriptorSupported)
        {
            LL_INFOS("Vulkan") << "Avatar bone descriptor set layout skipped (VK_KHR_push_descriptor unsupported, fallback to placeholder pool draw)" << LL_ENDL;
            return true;
        }

        VkDescriptorSetLayoutBinding binding = {};
        binding.binding         = 0;
        binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo lci = {};
        lci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        lci.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
        lci.bindingCount = 1;
        lci.pBindings    = &binding;

        VkResult r = vkCreateDescriptorSetLayout(sDevice, &lci, nullptr, &sAvatarBoneDescriptorSetLayout);
        if (r != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "createAvatarBoneDescriptorSetLayout failed: " << (S32)r << LL_ENDL;
            return false;
        }
        LL_INFOS("Vulkan") << "Avatar bone descriptor set layout created (set=2 binding 0 STORAGE_BUFFER VERTEX_BIT PUSH_DESCRIPTOR_KHR)" << LL_ENDL;
        return true;
    }

    // r41 sub-step 3.4-δ-4 (sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.4):
    // HOST_VISIBLE + MAPPED な storage buffer を 1 件確保し、110 mat4 = 7040 B を
    // identity matrix で初期化 (1 度限り)。実 rigged draw 経路の bone matrix 上書きは
    // 段階 4 本実装で各 frame 毎 mDrawInfo.mSkin->mInvBindMatrix 経由で writeBoneMatrices
    // 系 helper (本 sub-step では未実装) を介する想定。
    bool allocateAvatarBoneStorageBuffer()
    {
        if (!sDeviceLimits.pushDescriptorSupported)
        {
            return true;
        }
        if (sAllocator == VK_NULL_HANDLE)
        {
            LL_WARNS("Vulkan") << "allocateAvatarBoneStorageBuffer: VMA allocator not ready" << LL_ENDL;
            return false;
        }

        constexpr VkDeviceSize buffer_size = AVATAR_BONE_MATRIX_COUNT * 64; // mat4 = 64 B

        VkBufferCreateInfo bci = {};
        bci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bci.size        = buffer_size;
        bci.usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo aci = {};
        aci.usage         = VMA_MEMORY_USAGE_AUTO;
        aci.flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                          | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        aci.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        VmaAllocationInfo info = {};
        VkResult r = vmaCreateBuffer(sAllocator, &bci, &aci,
                                     &sAvatarBoneStorageBuffer,
                                     &sAvatarBoneStorageAllocation,
                                     &info);
        if (r != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "allocateAvatarBoneStorageBuffer vmaCreateBuffer failed: " << (S32)r << LL_ENDL;
            return false;
        }
        sAvatarBoneStorageMapped = info.pMappedData;

        if (sAvatarBoneStorageMapped)
        {
            float* data = static_cast<float*>(sAvatarBoneStorageMapped);
            for (U32 i = 0; i < AVATAR_BONE_MATRIX_COUNT; ++i)
            {
                float* m = data + i * 16;
                m[0]  = 1.f; m[1]  = 0.f; m[2]  = 0.f; m[3]  = 0.f;
                m[4]  = 0.f; m[5]  = 1.f; m[6]  = 0.f; m[7]  = 0.f;
                m[8]  = 0.f; m[9]  = 0.f; m[10] = 1.f; m[11] = 0.f;
                m[12] = 0.f; m[13] = 0.f; m[14] = 0.f; m[15] = 1.f;
            }
        }
        LL_INFOS("Vulkan") << "Avatar bone storage buffer allocated (" << (U32)buffer_size
                           << " B = 110 mat4 identity, HOST_VISIBLE + MAPPED)" << LL_ENDL;
        return true;
    }

    // r41 sub-step 3.4-δ-4 (sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.4):
    // avatar 専用 PipelineLayout (set_layouts[3] = { PerFrame, PerMaterial, AvatarBone })
    // と PSO (sSkySmoke vert/frag shader 流用、shader 改変ゼロ = Vulkan 仕様 layout-set ≥
    // shader-set 許容)。push constant 64 B / VERTEX_BIT は γ 二段構え準拠継承。
    bool createAvatarBonePipeline()
    {
        if (!sDeviceLimits.pushDescriptorSupported)
        {
            return true;
        }
        if (sAvatarBoneDescriptorSetLayout == VK_NULL_HANDLE)
        {
            LL_WARNS("Vulkan") << "createAvatarBonePipeline: avatar bone descriptor set layout not ready" << LL_ENDL;
            return false;
        }
        if (sSkySmokeVertModule == VK_NULL_HANDLE || sSkySmokeFragModule == VK_NULL_HANDLE)
        {
            LL_WARNS("Vulkan") << "createAvatarBonePipeline: sky smoke shader modules not ready" << LL_ENDL;
            return false;
        }

        VkDescriptorSetLayout set_layouts[3]    = {
            sPerFrameDescriptorSetLayout,
            sPerMaterialDescriptorSetLayout,
            sAvatarBoneDescriptorSetLayout,
        };
        VkPushConstantRange   push_constants[1] = {};
        push_constants[0].stageFlags            = VK_SHADER_STAGE_VERTEX_BIT;
        push_constants[0].offset                = 0;
        push_constants[0].size                  = 64;
        sAvatarBoneLayout = createStandardPipelineLayout(set_layouts, 3, push_constants, 1);
        if (sAvatarBoneLayout == VK_NULL_HANDLE)
        {
            LL_WARNS("Vulkan") << "Avatar bone pipeline layout create failed" << LL_ENDL;
            return false;
        }

        VkPipelineShaderStageCreateInfo stages[2] = {};
        stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = sSkySmokeVertModule;
        stages[0].pName  = "main";
        stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = sSkySmokeFragModule;
        stages[1].pName  = "main";

        VkPipelineVertexInputStateCreateInfo vi = {};
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkPipelineInputAssemblyStateCreateInfo ia = {};
        ia.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkViewport viewport = { 0.0f, 0.0f, (F32)OFFSCREEN_WIDTH, (F32)OFFSCREEN_HEIGHT, 0.0f, 1.0f };
        VkRect2D   scissor  = { { 0, 0 }, { OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT } };

        VkPipelineViewportStateCreateInfo vp = {};
        vp.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vp.viewportCount = 1;
        vp.pViewports    = &viewport;
        vp.scissorCount  = 1;
        vp.pScissors     = &scissor;

        VkPipelineRasterizationStateCreateInfo rs = {};
        rs.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode    = VK_CULL_MODE_NONE;
        rs.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rs.lineWidth   = 1.0f;

        VkPipelineMultisampleStateCreateInfo ms = {};
        ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo ds = {};
        ds.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds.depthTestEnable  = VK_FALSE;
        ds.depthWriteEnable = VK_FALSE;
        ds.depthCompareOp   = VK_COMPARE_OP_ALWAYS;

        VkPipelineColorBlendAttachmentState cba = {};
        cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                           | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo cb = {};
        cb.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.attachmentCount = 1;
        cb.pAttachments    = &cba;

        VkGraphicsPipelineCreateInfo ci = {};
        ci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        ci.stageCount          = 2;
        ci.pStages             = stages;
        ci.pVertexInputState   = &vi;
        ci.pInputAssemblyState = &ia;
        ci.pViewportState      = &vp;
        ci.pRasterizationState = &rs;
        ci.pMultisampleState   = &ms;
        ci.pDepthStencilState  = &ds;
        ci.pColorBlendState    = &cb;
        ci.layout              = sAvatarBoneLayout;
        ci.renderPass          = sRenderPass;
        ci.subpass             = 0;

        if (!compileGraphicsPipeline(ci, sAvatarBonePipeline))
        {
            LL_WARNS("Vulkan") << "Avatar bone graphics pipeline compile failed" << LL_ENDL;
            return false;
        }
        LL_INFOS("Vulkan") << "Avatar bone PSO compiled (set_layouts[3] = PerFrame + PerMaterial + AvatarBone, shader = sky smoke 流用、shader 改変ゼロ)" << LL_ENDL;
        return true;
    }
}

bool initVulkan()
{
    if (sInitialized)
    {
        LL_WARNS("Vulkan") << "initVulkan called twice" << LL_ENDL;
        return true;
    }

    VkResult result = volkInitialize();
    if (result != VK_SUCCESS)
    {
        LL_WARNS("Vulkan") << "volkInitialize failed: " << (S32)result << LL_ENDL;
        return false;
    }

    LL_INFOS("Vulkan") << "Initializing Vulkan loader..." << LL_ENDL;
    U32 instance_version = volkGetInstanceVersion();
    LL_INFOS("Vulkan") << "Vulkan loader version "
                       << VK_VERSION_MAJOR(instance_version) << "."
                       << VK_VERSION_MINOR(instance_version) << "."
                       << VK_VERSION_PATCH(instance_version) << LL_ENDL;

    if (!createInstance())
    {
        return false;
    }

    createDebugMessenger();

    if (!selectPhysicalDevice() || !selectQueueFamily() || !createDevice())
    {
        if (sInstance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(sInstance, nullptr);
            sInstance = VK_NULL_HANDLE;
        }
        return false;
    }

    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6γ (PSC):
    // VkPipelineCache 用 disk-persist storage を createPipelineCache() より前に
    // 立ち上げる (= storage->getBlob() を VkPipelineCacheCreateInfo.pInitialData に
    // 投入するため順序必須)。本 helper は file load (存在時) + (e1) 64 MB cap 超過
    // blob 破棄を実施、blob 不在は許容 (= 起動初回想定で empty 開始)。
    if (!createPipelineCacheStorage())
    {
        shutdownVulkan();
        return false;
    }

    if (!createCommandPool() || !createOffscreenImage() || !createRenderPass() || !createFramebuffer() || !createPipelineCache())
    {
        shutdownVulkan();
        return false;
    }

    // r41 sub-step 3.4-β-1 (sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.1 内包):
    // VMA allocator + 段階 3 共有 descriptor pool 雛形を per-frame UBO 構築前に立ち上げる。
    // 段階 3 phase の per-frame UBO 自体は既存 raw vkAllocateMemory path で継続 (置換は領域 7 で実施)。
    if (!createVmaAllocator() || !createSharedDescriptorPool())
    {
        shutdownVulkan();
        return false;
    }

    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6α (W2):
    // per-asset (set=3) descriptor pool grow 機構を sSharedDescriptorPool 直後に立ち上げる
    // (design 07 §6.3 / §6.4 = 4 cadence pool split、asset pool は grow only)。
    // 起動時 1 物理 pool (= 64 asset × 3 frame = 192 set) prealloc、PC-6 後続で
    // 5 cadence update site から acquire 呼出が走り始める。
    if (!createAssetUboPool())
    {
        shutdownVulkan();
        return false;
    }

    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6β (RB):
    // per-frame / per-pass cadence UBO ring buffer (= dynamic offset 投入経路、
    // design 07 §7.2 / §7.4) を sAssetUboPoolMgr 直後に立ち上げる。
    // cvar AYARingBufferSizeMB (default 4 MB) 経由で起動時 prealloc 容量配信、
    // PC-6 後続 (PC-6δ) で allocate() / beginFrame() 呼出が走り始める。
    if (!createDrawUboRingBuffer())
    {
        shutdownVulkan();
        return false;
    }

    // r41 sub-step 3.4-β-2 (sub-doc 03 §3.1.4): 1×1 white placeholder image lifecycle smoke。
    // VMA allocator 立ち上げ直後に発行、shutdownVulkan で vmaDestroyAllocator 前に破棄する。
    // β-2-3: staging buffer 経由 1px upload + 2 段 layout transition を 1 度限り実行 (INFO marker 出力)。
    if (!createPlaceholderWhiteImage() || !uploadPlaceholderWhiteSmoke())
    {
        shutdownVulkan();
        return false;
    }

    // r41 sub-step 3.3-β-2: per-frame matrix UBO descriptor + buffer + set (sub-doc 03 §3.1.1)
    if (!createPerFrameDescriptorSetLayout() || !createPerFrameUbos() || !createPerFrameDescriptorSets())
    {
        shutdownVulkan();
        return false;
    }

    // r41 sub-step 3.4-γ (sub-doc 03 §3.1.4 / sub-doc 07 §1.2.1 / §3.1 sub-step 7.3 layout 部分内包):
    // set=1 per-material 7 PBR slot descriptor set layout + 共用 placeholder sampler を
    // PSO 作成前に立ち上げる (sPlaceholderLayout / sSkySmokeLayout が二段構え参照する前提)。
    if (!createPerMaterialDescriptorSetLayout() || !createPlaceholderSampler())
    {
        shutdownVulkan();
        return false;
    }

    if (!createPlaceholderPipeline())
    {
        shutdownVulkan();
        return false;
    }

    if (!createSkySmokePipeline())
    {
        LL_WARNS("Vulkan") << "Sky smoke PSO creation failed" << LL_ENDL;
        shutdownVulkan();
        return false;
    }

    // r41 sub-step 3.4-δ-4 (sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.4 push descriptor 部分内包):
    // avatar bone descriptor set layout + storage buffer + pipeline layout + PSO 配線。
    // VK_KHR_push_descriptor 未支援 device では各 helper 内で skip (success return)、
    // recordAvatarPlaceholderDraw は recordPlaceholderPoolDraw へ graceful fallback。
    if (!createAvatarBoneDescriptorSetLayout() || !allocateAvatarBoneStorageBuffer() || !createAvatarBonePipeline())
    {
        LL_WARNS("Vulkan") << "Avatar bone foundation creation failed" << LL_ENDL;
        shutdownVulkan();
        return false;
    }

    // r41 sub-step 3.4-γ (sub-doc 03 §3.1.4): set=1 transit smoke = sSharedDescriptorPool から
    // 1 set allocate + 7 binding 全部に β-2 placeholder white image view + 共用 sampler を bind。
    // beginFrame で sPlaceholderPipeline bind 後に bindPerMaterialDescriptorSet 経由で transit。
    if (!allocatePerMaterialDescriptorSet())
    {
        shutdownVulkan();
        return false;
    }
    {
        VkImageView views[PER_MATERIAL_BINDING_COUNT];
        for (U32 i = 0; i < PER_MATERIAL_BINDING_COUNT; ++i)
        {
            views[i] = sPlaceholderWhiteImageView;
        }
        updatePerMaterialDescriptorSet(views);
    }

    // r41 sub-step 3.4-β-1: VMA budget 1 度 smoke 出力 (INFO marker #3)。
    logVmaBudgetSmoke();

    sInitialized = true;
    return true;
}

void shutdownVulkan()
{
    if (sDevice != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(sDevice);

        if (sFramebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(sDevice, sFramebuffer, nullptr);
            sFramebuffer = VK_NULL_HANDLE;
        }
        if (sOffscreenImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(sDevice, sOffscreenImageView, nullptr);
            sOffscreenImageView = VK_NULL_HANDLE;
        }
        if (sOffscreenImage != VK_NULL_HANDLE)
        {
            vkDestroyImage(sDevice, sOffscreenImage, nullptr);
            sOffscreenImage = VK_NULL_HANDLE;
        }
        if (sOffscreenMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(sDevice, sOffscreenMemory, nullptr);
            sOffscreenMemory = VK_NULL_HANDLE;
        }
        if (sRenderPass != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(sDevice, sRenderPass, nullptr);
            sRenderPass = VK_NULL_HANDLE;
        }
        // r41 sub-step 3.4-δ-4 (sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.4):
        // avatar bone artifacts teardown。pipeline → layout → SSBO (VMA) → descriptor set layout の順、
        // shader 流用元 sSkySmokePipeline / sSkySmokeLayout より先に発火 (PSO は shader module 非依存)。
        if (sAvatarBonePipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(sDevice, sAvatarBonePipeline, nullptr);
            sAvatarBonePipeline = VK_NULL_HANDLE;
        }
        if (sAvatarBoneLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(sDevice, sAvatarBoneLayout, nullptr);
            sAvatarBoneLayout = VK_NULL_HANDLE;
        }
        if (sAvatarBoneStorageBuffer != VK_NULL_HANDLE && sAllocator != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(sAllocator, sAvatarBoneStorageBuffer, sAvatarBoneStorageAllocation);
            sAvatarBoneStorageBuffer     = VK_NULL_HANDLE;
            sAvatarBoneStorageAllocation = VK_NULL_HANDLE;
            sAvatarBoneStorageMapped     = nullptr;
        }
        if (sAvatarBoneDescriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(sDevice, sAvatarBoneDescriptorSetLayout, nullptr);
            sAvatarBoneDescriptorSetLayout = VK_NULL_HANDLE;
        }
        if (sSkySmokePipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(sDevice, sSkySmokePipeline, nullptr);
            sSkySmokePipeline = VK_NULL_HANDLE;
        }
        if (sSkySmokeLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(sDevice, sSkySmokeLayout, nullptr);
            sSkySmokeLayout = VK_NULL_HANDLE;
        }
        if (sSkySmokeFragModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(sDevice, sSkySmokeFragModule, nullptr);
            sSkySmokeFragModule = VK_NULL_HANDLE;
        }
        if (sSkySmokeVertModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(sDevice, sSkySmokeVertModule, nullptr);
            sSkySmokeVertModule = VK_NULL_HANDLE;
        }
        if (sPlaceholderPipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(sDevice, sPlaceholderPipeline, nullptr);
            sPlaceholderPipeline = VK_NULL_HANDLE;
        }
        if (sPlaceholderLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(sDevice, sPlaceholderLayout, nullptr);
            sPlaceholderLayout = VK_NULL_HANDLE;
        }
        if (sPlaceholderFragModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(sDevice, sPlaceholderFragModule, nullptr);
            sPlaceholderFragModule = VK_NULL_HANDLE;
        }
        if (sPlaceholderVertModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(sDevice, sPlaceholderVertModule, nullptr);
            sPlaceholderVertModule = VK_NULL_HANDLE;
        }
        // r41 sub-step 3.3-β-2: per-frame matrix UBO teardown (descriptor pool 経由で set は自動 free)
        if (sPerFrameDescriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(sDevice, sPerFrameDescriptorPool, nullptr);
            sPerFrameDescriptorPool = VK_NULL_HANDLE;
            for (U32 frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
            {
                sPerFrameDescriptorSet[frame] = VK_NULL_HANDLE;
            }
        }
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
        }
        if (sPerFrameDescriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(sDevice, sPerFrameDescriptorSetLayout, nullptr);
            sPerFrameDescriptorSetLayout = VK_NULL_HANDLE;
        }
        // r41 sub-step 3.4-γ (sub-doc 03 §3.1.4 / sub-doc 07 §1.2.1 set=1):
        // per-material descriptor set layout teardown。descriptor set 自体は sSharedDescriptorPool 経由で自動 free。
        if (sPerMaterialDescriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(sDevice, sPerMaterialDescriptorSetLayout, nullptr);
            sPerMaterialDescriptorSetLayout = VK_NULL_HANDLE;
        }
        sPerMaterialDescriptorSet = VK_NULL_HANDLE;
        // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6γ (PSC):
        // pipeline cache の disk persist。vkDestroyPipelineCache 前に
        // vkGetPipelineCacheData → updateBlob → persistToDisk で次回起動向 cache
        // を保存。64 MB cap 超過時は LLPipelineCacheStorage::persistToDisk() が
        // writer 不呼出 + false return (= (e1) 戦略)、本層は LL_INFOS で観察可能。
        if (sPipelineCache != VK_NULL_HANDLE && sPipelineCacheStorageMgr)
        {
            std::size_t blob_size = 0;
            VkResult sz_res = vkGetPipelineCacheData(sDevice, sPipelineCache, &blob_size, nullptr);
            if (sz_res == VK_SUCCESS && blob_size > 0)
            {
                LLPipelineCacheStorage::CacheBlob blob(blob_size);
                VkResult get_res = vkGetPipelineCacheData(sDevice, sPipelineCache,
                                                          &blob_size, blob.data());
                if (get_res == VK_SUCCESS || get_res == VK_INCOMPLETE)
                {
                    blob.resize(blob_size);
                    sPipelineCacheStorageMgr->updateBlob(std::move(blob));
                    const bool persisted = sPipelineCacheStorageMgr->persistToDisk();
                    LL_INFOS("Vulkan") << "Pipeline cache shutdown persist (PC-6γ PSC, blob="
                                       << sPipelineCacheStorageMgr->getBlobSize()
                                       << " bytes, persisted="
                                       << (persisted ? "yes" : "no (cap exceeded or write failed)")
                                       << ")" << LL_ENDL;
                }
                else
                {
                    LL_WARNS("Vulkan") << "vkGetPipelineCacheData (data) failed: "
                                       << (S32)get_res << LL_ENDL;
                }
            }
            else if (sz_res != VK_SUCCESS)
            {
                LL_WARNS("Vulkan") << "vkGetPipelineCacheData (size) failed: "
                                   << (S32)sz_res << LL_ENDL;
            }
        }
        if (sPipelineCache != VK_NULL_HANDLE)
        {
            vkDestroyPipelineCache(sDevice, sPipelineCache, nullptr);
            sPipelineCache = VK_NULL_HANDLE;
        }
        // PC-6γ (PSC): storage manager teardown。disk persist は vkDestroyPipelineCache
        // 前に実施済、shutdown() は blob clear + state reset のみ (= auto-persist せず)。
        if (sPipelineCacheStorageMgr)
        {
            sPipelineCacheStorageMgr->shutdown();
            sPipelineCacheStorageMgr.reset();
        }
        if (sCommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(sDevice, sCommandPool, nullptr);
            sCommandPool = VK_NULL_HANDLE;
            sCommandBuffer = VK_NULL_HANDLE;
        }

        // r41 sub-step 3.4-β-2 (sub-doc 03 §3.1.4): placeholder image teardown。
        // vmaDestroyImage は sAllocator 生存中に呼ぶ必要があるため、共有 pool 破棄前に発行。
        destroyPlaceholderWhiteImage();

        // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6β (RB):
        // draw UBO ring buffer teardown。LLUboRingBuffer::shutdown() (~dtor 経由)
        // が現 buffer を destroyer callback で vmaDestroyBuffer する。
        // init reverse 順 = sAssetUboPoolMgr より先、sAllocator 生存中に発火。
        // destroyer は sAllocator 早期 return guard 済 (= 万一の二重 shutdown
        // 後でも safe)、record map は handle 経由でだけ参照されるため、shutdown
        // 完走後 map は空になる。
        if (sDrawUboRingBufferMgr)
        {
            sDrawUboRingBufferMgr->shutdown();
            sDrawUboRingBufferMgr.reset();
        }
        sDrawUboRingBufferRecords.clear();

        // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6α (W2):
        // per-asset descriptor pool teardown。LLAssetUboPool::shutdown() が内部の
        // 全 grow pool を destroyer callback 経由で逆順 destroy するため、
        // sSharedDescriptorPool 破棄前 / sDevice 生存中に発火する。
        if (sAssetUboPoolMgr)
        {
            sAssetUboPoolMgr->shutdown();
            sAssetUboPoolMgr.reset();
        }

        // r41 sub-step 3.4-β-1 (sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.1 内包):
        // 共有 descriptor pool teardown (set は pool 経由で自動 free)。
        if (sSharedDescriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(sDevice, sSharedDescriptorPool, nullptr);
            sSharedDescriptorPool = VK_NULL_HANDLE;
        }
        // r41 sub-step 3.4-γ (sub-doc 03 §3.1.4): per-material 共用 placeholder sampler teardown。
        // sSharedDescriptorPool 破棄後 / sAllocator 破棄前に発行 (sampler は VMA 非経由・sDevice 直属)。
        if (sPlaceholderSampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(sDevice, sPlaceholderSampler, nullptr);
            sPlaceholderSampler = VK_NULL_HANDLE;
        }
        // r41 sub-step 3.4-β-1: VMA allocator teardown (sDevice 破棄前必須)。
        if (sAllocator != VK_NULL_HANDLE)
        {
            vmaDestroyAllocator(sAllocator);
            sAllocator = VK_NULL_HANDLE;
        }
        sSharedVmaBudgetLogged = false;

        vkDestroyDevice(sDevice, nullptr);
        sDevice = VK_NULL_HANDLE;
        sGraphicsQueue = VK_NULL_HANDLE;
        sGraphicsQueueFamily = UINT_MAX;
        LL_INFOS("Vulkan") << "Vulkan device destroyed" << LL_ENDL;
    }
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
        sDeviceName.clear();
        LL_INFOS("Vulkan") << "Vulkan instance destroyed" << LL_ENDL;
    }
    if (sInitialized)
    {
        volkFinalize();
        sInitialized = false;
        sValidationEnabled = false;
    }
}

bool beginFrame()
{
    if (!sInitialized || sInFrame)
    {
        return false;
    }

    // r41 sub-step 3.3-δ-1: frame in flight index advance (sub-doc 03 §3.1.1 δ)。
    // syncMatrices Vulkan path / δ-2 push constant / 将来 descriptor set bind が共有参照。
    sFrameIndex = (sFrameIndex + 1) % FRAMES_IN_FLIGHT;

    vkResetCommandBuffer(sCommandBuffer, 0);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkResult result = vkBeginCommandBuffer(sCommandBuffer, &begin_info);
    if (result != VK_SUCCESS)
    {
        LL_WARNS("Vulkan") << "vkBeginCommandBuffer failed: " << (S32)result << LL_ENDL;
        return false;
    }

    VkClearValue clear_value = {};
    clear_value.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

    VkRenderPassBeginInfo rp_begin = {};
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.renderPass = sRenderPass;
    rp_begin.framebuffer = sFramebuffer;
    rp_begin.renderArea.offset = { 0, 0 };
    rp_begin.renderArea.extent = { OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT };
    rp_begin.clearValueCount = 1;
    rp_begin.pClearValues = &clear_value;

    vkCmdBeginRenderPass(sCommandBuffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    // r41 sub-step 3.1b item #8: minimal PSO bind smoke test (sub-doc 03 §3.1 sub-step 3.1
    // 完了 marker「動作中 1 frame 内に PSO bind が validation 0 件で完了」)。draw call は実装せず、
    // bind 動作 + validation 0 件のみ確認。実 sky pool PSO bind は sub-step 3.4 で配線。
    if (sPlaceholderPipeline != VK_NULL_HANDLE)
    {
        vkCmdBindPipeline(sCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, sPlaceholderPipeline);

        // r41 sub-step 3.4-γ (sub-doc 03 §3.1.4 / sub-doc 07 §1.2.1 set=1):
        // per-material descriptor set transit smoke。sPlaceholderLayout 二段構え (set=0 per-frame +
        // set=1 per-material) に揃え、firstSet=1 で 7 PBR slot 白 placeholder × 7 を bind。draw 未発行のため
        // descriptor 参照は発生しないが、bind 経路の validation 0 件 + INFO marker 経由で 4 transit
        // (allocate / update / bindFirstSet / firstSet=1 hit) を 1 度限り emit。実 per-material 更新は
        // 領域 7 sub-step 7.5 (LLImageGL → VkImage 抱合せ後) で本配信。
        bindPerMaterialDescriptorSet(sCommandBuffer, sPlaceholderLayout);
    }

    sInFrame = true;

    // r41 sub-step 3.3-C-β-2: dynamic rendering helper transit smoke (sub-doc 03 §3.1.2)。
    // no-op early return (all-null args) で in-frame gating + 1 度限りの marker emit を verify。
    // 実 attachment 配線は γ (LLRenderTarget::bindTarget Vulkan 並走) + 領域 7 sub-step 7.5 で本配信。
    // 注: legacy vkCmdBeginRenderPass の内側だが、all-null path は vkCmdBeginRendering を発行せず、
    //     no-op で帰るため legacy render pass と衝突しない。
    beginDynamicRendering(0, 0, nullptr, 0, nullptr);
    endDynamicRendering();

    // r41 sub-step 3.3-B-γ (2026-05-31): β-2 transit smoke 撤去済。
    // SPIR-V build chain 経路は createSkySmokePipeline 経由で initVulkan 内 1 度実行
    // (loadSpirvShaderModuleFromFile → loadSpirvShaderModule → vkCreateShaderModule)、
    // beginFrame 内の transit smoke は不要。

    return true;
}

bool endFrame()
{
    if (!sInitialized || !sInFrame)
    {
        return false;
    }

    vkCmdEndRenderPass(sCommandBuffer);

    VkResult result = vkEndCommandBuffer(sCommandBuffer);
    if (result != VK_SUCCESS)
    {
        LL_WARNS("Vulkan") << "vkEndCommandBuffer failed: " << (S32)result << LL_ENDL;
        sInFrame = false;
        return false;
    }

    sInFrame = false;
    return true;
}

VkCommandBuffer getCurrentCommandBuffer()
{
    return sInFrame ? sCommandBuffer : VK_NULL_HANDLE;
}

bool isVulkanInitialized()
{
    return sInitialized;
}

bool isValidationEnabled()
{
    return sValidationEnabled;
}

VkDevice getDevice()
{
    return sDevice;
}

VkPipelineCache getPipelineCache()
{
    return sPipelineCache;
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
        LL_WARNS("Vulkan") << "vkCreatePipelineLayout failed: " << (S32)result << LL_ENDL;
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
        LL_WARNS("Vulkan") << "vkCreateGraphicsPipelines failed: " << (S32)result << LL_ENDL;
        out_pipeline = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

// r41 sub-step 3.3-β-2: per-frame matrix descriptor set layout getter (sub-doc 03 §3.1.1)
// 所有は LLVKLoader (initVulkan で create、shutdownVulkan で destroy)、caller は破棄しない。
VkDescriptorSetLayout getPerFrameDescriptorSetLayout()
{
    return sPerFrameDescriptorSetLayout;
}

// r41 sub-step 3.3-δ-1: frame in flight index getter (sub-doc 03 §3.1.1 δ)
U32 getCurrentFrameIndex()
{
    return sFrameIndex;
}

// r41 sub-step 3.3-δ-1: per-frame matrix UBO write helper (sub-doc 03 §3.1.1 δ)
// LLRender::syncMatrices() Vulkan path 並走で呼出、sPerFrameUboMapped[sFrameIndex] へ memcpy。
// 未初期化 / 未 map 時は no-op (GL path 単独動作環境で safe)。
void writeCurrentPerFrameMatrixUBO(const PerFrameMatrixUBO& data)
{
    if (!sInitialized || sPerFrameUboMapped[sFrameIndex] == nullptr)
    {
        return;
    }
    std::memcpy(static_cast<U8*>(sPerFrameUboMapped[sFrameIndex]) + PERFRAME_UBO_OFFSET,
                &data,
                sizeof(PerFrameMatrixUBO));

    static bool s_first_write = true;
    if (s_first_write)
    {
        s_first_write = false;
        LL_INFOS("Vulkan") << "syncMatrices PerFrame UBO write path active (frame_index="
                           << sFrameIndex << ")" << LL_ENDL;
    }
}

void writeCurrentTextureMatrixUBO(const TextureMatrixUBO& data)
{
    if (!sInitialized || sPerFrameUboMapped[sFrameIndex] == nullptr)
    {
        return;
    }
    std::memcpy(static_cast<U8*>(sPerFrameUboMapped[sFrameIndex]) + TEXTURE_UBO_OFFSET,
                &data,
                sizeof(TextureMatrixUBO));

    static bool s_first_write = true;
    if (s_first_write)
    {
        s_first_write = false;
        LL_INFOS("Vulkan") << "syncMatrices TextureMatrix UBO write path active (frame_index="
                           << sFrameIndex << ")" << LL_ENDL;
    }
}

// r41 sub-step 3.3-δ-2: modelview push constant 投入 helper (sub-doc 03 §3.1.1 δ)
// LLRender::syncMatrices() Vulkan path 並走で呼出、in-frame (beginFrame...endFrame 間) のみ
// sCommandBuffer に vkCmdPushConstants(mat4 modelview_matrix / 64 B / VERTEX_BIT) 投入。
// 未初期化 / out-of-frame / layout 未確定時は no-op (案 P 安全 gating)。
// 現状 sPlaceholderLayout は γ で二段構え準拠化済 (push constant range 0..64 B / VERTEX_BIT)、
// shader 側は本 push を未参照だが Vulkan validation 仕様で extra resources として許容。
void pushCurrentModelviewMatrix(const float modelview_matrix[16])
{
    if (!sInitialized || !sInFrame || sPlaceholderLayout == VK_NULL_HANDLE ||
        sCommandBuffer == VK_NULL_HANDLE)
    {
        return;
    }
    vkCmdPushConstants(sCommandBuffer,
                       sPlaceholderLayout,
                       VK_SHADER_STAGE_VERTEX_BIT,
                       0,
                       64,
                       modelview_matrix);

    static bool s_first_push = true;
    if (s_first_push)
    {
        s_first_push = false;
        LL_INFOS("Vulkan") << "syncMatrices modelview push constant path active (frame_index="
                           << sFrameIndex
                           << ", layout=sPlaceholderLayout, range=0..64 B / VERTEX_BIT)"
                           << LL_ENDL;
    }
}

// ------------------------------------------------------------------
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6δ:
// 5 cadence flush 関数 (空 dummy 書込 = ring buffer allocate / beginFrame 経路通電)。
//
// 共通動作 = (1) sDrawUboRingBufferMgr 未初期化 = 即時 return (GL 単独動作 / Vulkan
//   未起動時の MUSEUBO-A 整合保証) + (2) allocate(256) で 1 record 確保 + 256 B
//   align_up + chunk 内 wrap or grow trigger 評価 + (3) AllocateResult.buffer を
//   side-table lookup → mapped pointer + offset 経由で memset 0 (= test UBO 空書込)。
//
// 設計根拠 = design 06b §2.2 (5 cadence 分類) / §4.1 (flush 駆動関数 名前) /
//           §4.3 (駆動位置) / §5.3 (mUseUBO runtime gate と dirty propagation)、
//           design 07 §7.2 (4 MB / 16 MB) / §7.3 (256 B alignment) / §7.5 (3 chunk
//           wrap + grow) / §8.4 (FRAMES_IN_FLIGHT=3 同期 rotate)。
//
// 本 PC-6δ では shader / asset / skin 引数は受信のみで body 内未参照
// (= 将来 PC-6ε で per-program / per-asset / per-skin dirty map lookup の key 化、
//  本 sub では空書込で hash 不要)。(void) cast で unused-parameter 警告抑止。
// ------------------------------------------------------------------
namespace
{
    // 共通 helper: AllocateResult から sDrawUboRingBufferRecords 経由で mapped pointer 解決し
    // offset 位置に memset 0 (= test UBO 空 dummy 書込 256 B)。
    // sDrawUboRingBufferMgr 未初期化時は即時 return (= GL 単独動作 / Vulkan 未起動の MUSEUBO-A 保証)。
    void flushDummyUboWrite(const char* cadence_label)
    {
        if (!sDrawUboRingBufferMgr)
        {
            return;
        }

        const LLUboRingBuffer::AllocateResult result = sDrawUboRingBufferMgr->allocate(256);
        if (!result.success || result.buffer == 0)
        {
            return;
        }

        auto it = sDrawUboRingBufferRecords.find(result.buffer);
        if (it == sDrawUboRingBufferRecords.end() || it->second.mapped == nullptr)
        {
            return;
        }

        std::memset(static_cast<U8*>(it->second.mapped) + result.offset, 0, result.size);

        // 1 回だけ marker log (= cadence 毎に first-fire 検知できるよう label 引数で区別)。
        // 2 回目以降は no-op、運用 log noise 抑止。
        static std::unordered_map<std::string, bool> s_first_fire;
        const std::string key(cadence_label);
        if (!s_first_fire[key])
        {
            s_first_fire[key] = true;
            LL_INFOS("Vulkan") << "PC-6δ " << cadence_label
                               << " flush path active (buffer=0x" << std::hex << result.buffer
                               << std::dec << ", offset=" << result.offset
                               << ", size=" << result.size
                               << ", grew=" << (result.grew ? "yes" : "no")
                               << ", frame=" << sDrawUboRingBufferMgr->getFrameIndex()
                               << ", chunk=" << sDrawUboRingBufferMgr->getActiveChunk()
                               << ")" << LL_ENDL;
        }
    }
}

void flushFrameUbos()
{
    if (!sDrawUboRingBufferMgr)
    {
        return;
    }
    // per-frame cadence のみ beginFrame() で frame index advance + chunk reset
    // (design 06b §4.3 + design 07 §8.4 FRAMES_IN_FLIGHT 同期 rotate)。
    sDrawUboRingBufferMgr->beginFrame();
    flushDummyUboWrite("flushFrameUbos");
}

void flushProgramUbos(LLGLSLShader* shader)
{
    (void)shader; // PC-6ε で per-program dirty map key 化、本 sub では未参照
    flushDummyUboWrite("flushProgramUbos");
}

void flushDrawUbos()
{
    flushDummyUboWrite("flushDrawUbos");
}

void flushAssetUbos(LL::GLTF::Asset* asset)
{
    (void)asset; // PC-6ε で per-asset dirty map key 化、本 sub では未参照
    flushDummyUboWrite("flushAssetUbos");
}

void flushSkinUbos(LL::GLTF::Skin* skin)
{
    (void)skin; // PC-6ε で per-skin dirty map key 化、本 sub では未参照
    flushDummyUboWrite("flushSkinUbos");
}

// ------------------------------------------------------------------
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6ε-1:
// singleton cadence (= 第 6 cadence) flush 関数実装。flushDummyUboWrite
// helper 経由で 5 cadence と同形 (= MUSEUBO-A guard / 256 B allocate /
// side-table lookup / memset 0 / first-fire LL_INFOS marker) を継承。
//
// per-frame cadence のみ beginFrame() を呼ぶ (= frame index advance + chunk
// reset、design 07 §8.4)。singleton は per-frame 跨ぎ持続なので beginFrame()
// 呼出は不要 (= flushFrameUbos が一手に担当)。
//
// 設計根拠 = design 02 §3 (`Global_` prefix = singleton cadence 明示分類) +
//          design 06c §2.2 (Global_ReflectionProbes singleton 配置例) +
//          design 06a §3.3 (CadenceTag enum singleton 含む)。
// ------------------------------------------------------------------
void flushSingletonUbos()
{
    flushDummyUboWrite("flushSingletonUbos");
}

// r41 sub-step 3.4-δ-1 (sub-doc 03 §3.1.4): 12 pool 共用 placeholder draw helper
// (旧名 recordSkySmokeDraw、3.2 sky-smoke 由来を 12 pool 共用へ unification)。
// PSO bind (sSkySmokePipeline、fullscreen triangle + 定数色 frag = sky blue 0.4/0.6/0.9/1.0) +
// set=0 PerFrame descriptor (sPerFrameDescriptorSet[sFrameIndex]) + set=1 PerMaterial descriptor
// (γ 確立 sPerMaterialDescriptorSet、layout 引数化済 helper 経由) + push constant 64 B identity
// modelview / VERTEX_BIT + vkCmdDraw(3, 1, 0, 0)。各 pool recordPoolDraws hook body から呼出、
// in-frame 前提 (caller 側 sCommandBuffer 有効性確認)。
void recordPlaceholderPoolDraw(VkCommandBuffer cmd_buf)
{
    if (cmd_buf == VK_NULL_HANDLE ||
        sSkySmokePipeline == VK_NULL_HANDLE ||
        sSkySmokeLayout == VK_NULL_HANDLE)
    {
        return;
    }

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, sSkySmokePipeline);

    // set=0 PerFrame descriptor set (γ 二段構え layout 経由、sFrameIndex の set を bind)
    if (sPerFrameDescriptorSet[sFrameIndex] != VK_NULL_HANDLE)
    {
        vkCmdBindDescriptorSets(cmd_buf,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                sSkySmokeLayout,
                                /*firstSet=*/0,
                                /*descriptorSetCount=*/1,
                                &sPerFrameDescriptorSet[sFrameIndex],
                                /*dynamicOffsetCount=*/0,
                                /*pDynamicOffsets=*/nullptr);
    }

    // set=1 PerMaterial descriptor set (γ helper、3.4-δ-1 layout 引数化で sSkySmokeLayout 経路)
    bindPerMaterialDescriptorSet(cmd_buf, sSkySmokeLayout);

    // push constant: modelview_matrix = identity (4x4)、fullscreen triangle は NDC 直書きで identity OK
    const float identity_modelview[16] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
    vkCmdPushConstants(cmd_buf,
                       sSkySmokeLayout,
                       VK_SHADER_STAGE_VERTEX_BIT,
                       /*offset=*/0,
                       /*size=*/64,
                       identity_modelview);

    vkCmdDraw(cmd_buf, 3, 1, 0, 0);

    static bool s_first_call = true;
    if (s_first_call)
    {
        s_first_call = false;
        LL_INFOS("Vulkan") << "Placeholder pool draw fired (PSO bind sSkySmokePipeline + "
                              "set=0 PerFrame + set=1 PerMaterial + push constant 64 B identity / "
                              "VERTEX_BIT + vkCmdDraw(3,1,0,0))"
                           << LL_ENDL;
    }
}

// r41 sub-step 3.4-δ-4 (sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.4 push descriptor 部分内包):
// avatar pool 専用 placeholder draw helper。基本 placeholder draw に加え set=2 binding 0 へ
// avatar bone storage buffer (mat4 × 110 identity) を vkCmdPushDescriptorSetKHR 経由で投入する。
// VK_KHR_push_descriptor 未支援 device / avatar foundation 未整備時は recordPlaceholderPoolDraw
// へ graceful fallback (機能 degrade、validation 違反 0 件維持)。
//
// 段階 4 本実装で各 rigged mesh draw 毎に LLMeshSkinInfo の bone matrix × N を
// sAvatarBoneStorageMapped に書込→本 helper の bone size 引数化拡張で投入する経路の foundation。
void recordAvatarPlaceholderDraw(VkCommandBuffer cmd_buf)
{
    // VK_KHR_push_descriptor 未支援 device / avatar foundation 未整備時の fallback。
    if (!sDeviceLimits.pushDescriptorSupported ||
        sAvatarBonePipeline == VK_NULL_HANDLE ||
        sAvatarBoneLayout == VK_NULL_HANDLE ||
        sAvatarBoneStorageBuffer == VK_NULL_HANDLE ||
        vkCmdPushDescriptorSetKHR == nullptr)
    {
        recordPlaceholderPoolDraw(cmd_buf);
        return;
    }

    if (cmd_buf == VK_NULL_HANDLE)
    {
        return;
    }

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, sAvatarBonePipeline);

    // set=0 PerFrame descriptor set (γ 二段構え layout 経由、sFrameIndex の set を bind)
    if (sPerFrameDescriptorSet[sFrameIndex] != VK_NULL_HANDLE)
    {
        vkCmdBindDescriptorSets(cmd_buf,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                sAvatarBoneLayout,
                                /*firstSet=*/0,
                                /*descriptorSetCount=*/1,
                                &sPerFrameDescriptorSet[sFrameIndex],
                                /*dynamicOffsetCount=*/0,
                                /*pDynamicOffsets=*/nullptr);
    }

    // set=1 PerMaterial descriptor set (γ helper、layout 引数化済み)
    bindPerMaterialDescriptorSet(cmd_buf, sAvatarBoneLayout);

    // set=2 binding 0 = avatar bone storage buffer (push descriptor 経路)
    VkDescriptorBufferInfo bone_buffer_info = {};
    bone_buffer_info.buffer = sAvatarBoneStorageBuffer;
    bone_buffer_info.offset = 0;
    bone_buffer_info.range  = AVATAR_BONE_MATRIX_COUNT * 64;

    VkWriteDescriptorSet bone_write = {};
    bone_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    bone_write.dstSet          = VK_NULL_HANDLE; // push descriptor: dstSet ignored
    bone_write.dstBinding      = 0;
    bone_write.descriptorCount = 1;
    bone_write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bone_write.pBufferInfo     = &bone_buffer_info;

    vkCmdPushDescriptorSetKHR(cmd_buf,
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              sAvatarBoneLayout,
                              /*set=*/2,
                              /*descriptorWriteCount=*/1,
                              &bone_write);

    // push constant: modelview_matrix = identity (4x4)
    const float identity_modelview[16] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
    vkCmdPushConstants(cmd_buf,
                       sAvatarBoneLayout,
                       VK_SHADER_STAGE_VERTEX_BIT,
                       /*offset=*/0,
                       /*size=*/64,
                       identity_modelview);

    vkCmdDraw(cmd_buf, 3, 1, 0, 0);

    static bool s_first_avatar_call = true;
    if (s_first_avatar_call)
    {
        s_first_avatar_call = false;
        LL_INFOS("Vulkan") << "Avatar placeholder pool draw fired (PSO bind sAvatarBonePipeline + "
                              "set=0 PerFrame + set=1 PerMaterial + set=2 BoneStorage push descriptor "
                              "(vkCmdPushDescriptorSetKHR, 110 mat4 identity) + push constant 64 B / "
                              "VERTEX_BIT + vkCmdDraw(3,1,0,0))"
                           << LL_ENDL;
    }
}

// ============================================================
// r41 sub-step 3.3-C-β-2: dynamic rendering helper (sub-doc 03 §3.1.2 + sub-doc 07 §1.2.3)
//
// LLRenderTarget::bindTarget() / flush() の Vulkan path 並走で使用 (γ で wire up)。
// in-frame (beginFrame...endFrame 間) かつ Vulkan 初期化済 + sCommandBuffer 有効時のみ
// vkCmdBeginRendering / vkCmdEndRendering を発火、それ以外 no-op (案 P gating pattern 継承)。
// β-2 transit smoke は all-null args (color_count=0 + depth=nullptr) で no-op 早期 return
// path のみ動作実証 + 1 度限りの marker emit、実 attachment 提供 + 実 begin/end 発火は
// 領域 7 sub-step 7.5 で本配線 (本 helper を γ/δ 経由で呼ぶ LLRenderTarget も同様)。
// ============================================================
void beginDynamicRendering(U32                               width,
                           U32                               height,
                           const DynamicRenderingAttachment* color_attachments,
                           U32                               color_count,
                           const DynamicRenderingAttachment* depth_attachment)
{
    if (!sInitialized || !sInFrame || sCommandBuffer == VK_NULL_HANDLE)
    {
        return;  // out-of-frame or pre-init: no-op (GL path 単独動作環境で safe)
    }

    static bool s_first_call = true;
    if (s_first_call)
    {
        s_first_call = false;
        LL_INFOS("Vulkan") << "beginDynamicRendering : dynamic rendering helper "
                              "path active (Vulkan 1.3 dynamicRendering / in-frame guard PASS / "
                              "β-2 transit smoke = all-null args no-op early return)"
                           << LL_ENDL;
    }

    // attachment infos 組立 (β-2 transit smoke では null view を skip)
    VkRenderingAttachmentInfo color_infos[4] = {};
    U32 valid_color_count = 0;
    for (U32 i = 0; i < color_count && i < 4; ++i)
    {
        if (color_attachments == nullptr || color_attachments[i].image_view == VK_NULL_HANDLE)
        {
            continue;  // β-2 transit smoke: null view を skip
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
        return;  // 全 view null: no-op (β-2 transit smoke 受入、対称 endDynamicRendering も no-op)
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

    vkCmdBeginRendering(sCommandBuffer, &rendering_info);
    sInDynamicRendering = true;
}

void endDynamicRendering()
{
    if (!sInitialized || !sInFrame || sCommandBuffer == VK_NULL_HANDLE ||
        !sInDynamicRendering)
    {
        return;  // begin が no-op だった場合は対称で skip (pair 維持)
    }

    vkCmdEndRendering(sCommandBuffer);
    sInDynamicRendering = false;
}

// r41 sub-step 3.3-B-β-2/γ: SPIR-V shader module load helper (sub-doc 03 §3.1.3)
// build 時 glslangValidator で pre-compile した .spv binary を VkShaderModule 化。
// 領域 6 sub-step 6.1 一括化までの 1 shader exemplar pre-flight (sky placeholder)。
// γ 改修 (2026-05-31): initVulkan 内 createSkySmokePipeline からの呼出に対応するため
// sInitialized ガードを撤去 (sDevice の null check のみで safety 担保、PSO compile は
// initVulkan の sInitialized=true 設定前に実行されるため)。
VkShaderModule loadSpirvShaderModule(const U32* spv_code, size_t code_size_bytes)
{
    if (sDevice == VK_NULL_HANDLE)
    {
        return VK_NULL_HANDLE;
    }
    if (spv_code == nullptr || code_size_bytes == 0 || (code_size_bytes % 4) != 0)
    {
        LL_WARNS("Vulkan") << "loadSpirvShaderModule: invalid args"
                           << " (spv_code=" << (spv_code ? "non-null" : "null")
                           << " size=" << (S32)code_size_bytes << " bytes)" << LL_ENDL;
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
        LL_WARNS("Vulkan") << "vkCreateShaderModule failed: " << (S32)result
                           << " (size=" << (S32)code_size_bytes << " bytes)" << LL_ENDL;
        return VK_NULL_HANDLE;
    }

    LL_INFOS("Vulkan") << "SPIR-V shader module loaded"
                       << " size=" << (S32)code_size_bytes << " bytes" << LL_ENDL;
    return module;
}

// r41 sub-step 4.3-γ'-port-α-5 (sub-doc 06 §3.1 case ② runtime path):
// production SPIR-V sink。LLShaderMgr Vulkan path + SPIR-V cache layer 案 C の
// memory-resident blob 共通 sink、loadSpirvShaderModule (primitive sink) 委譲。
// 3.3-B exemplar (試作レール) の loadSpirvShaderModuleFromFile() とは sink 分離。
VkShaderModule loadSpirvShaderModuleFromMemory(const std::vector<unsigned int>& spirv)
{
    if (spirv.empty())
    {
        LL_WARNS("Vulkan") << "loadSpirvShaderModuleFromMemory: empty SPIR-V blob" << LL_ENDL;
        return VK_NULL_HANDLE;
    }
    return loadSpirvShaderModule(spirv.data(), spirv.size() * sizeof(unsigned int));
}

// r41 sub-step 3.4-β-2 (sub-doc 03 §3.1.4): LLGLenum → VkFormat 集約変換 公開 API。
// 実体は file-local llGlEnumToVkFormatImpl (GL header 非依存、hex literal 照合)。
VkFormat llGlEnumToVkFormat(U32 ll_gl_intformat)
{
    return llGlEnumToVkFormatImpl(ll_gl_intformat);
}

// ------------------------------------------------------------------
// r41 sub-step 4.3-β': LLVertexBuffer Vulkan 化 (charter §7.5 boundary refine)
// sub-doc 04 §3.4 案 D hybrid 採用 (AYA 確定 2026-05-31)。
//
// β' = placement のみ。bind/draw fire は 4.3-ε' 範囲で配線。
// HOST_VISIBLE + MAPPED で確保し、persistent mapped pointer を caller (LLVertexBuffer)
// へ返却。GL VBO/IBO の mMappedData 経路と parallel 動作 (lazy upload 接続は 4.3-ε')。
// VmaAllocation handle は void* opaque で公開 (vk_mem_alloc.h header 持込み回避、
// 既存 llvkloader.h:192 / sub-step 3.4-β-2 設計継承)。
// ------------------------------------------------------------------
namespace
{
    bool createBufferVkImpl(U32                 size_bytes,
                            VkBufferUsageFlags  usage,
                            const char*         tag,
                            VkBuffer&           out_buffer,
                            void*&              out_allocation,
                            void**              out_mapped)
    {
        out_buffer     = VK_NULL_HANDLE;
        out_allocation = nullptr;
        if (out_mapped)
        {
            *out_mapped = nullptr;
        }

        if (size_bytes == 0)
        {
            LL_WARNS("Vulkan") << tag << ": size_bytes=0 (skip)" << LL_ENDL;
            return false;
        }
        if (sAllocator == VK_NULL_HANDLE)
        {
            // Vulkan 未初期化 / VMA 未確保時は静かに false (caller 側 GL fallback 想定)。
            return false;
        }

        VkBufferCreateInfo bci = {};
        bci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bci.size        = size_bytes;
        bci.usage       = usage;
        bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo aci = {};
        aci.usage         = VMA_MEMORY_USAGE_AUTO;
        aci.flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                          | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        aci.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        VkBuffer       buffer     = VK_NULL_HANDLE;
        VmaAllocation  allocation = VK_NULL_HANDLE;
        VmaAllocationInfo info    = {};
        VkResult r = vmaCreateBuffer(sAllocator, &bci, &aci, &buffer, &allocation, &info);
        if (r != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << tag << ": vmaCreateBuffer failed result=" << (S32)r
                               << " size=" << (S32)size_bytes << LL_ENDL;
            if (buffer != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(sAllocator, buffer, allocation);
            }
            return false;
        }

        out_buffer     = buffer;
        out_allocation = reinterpret_cast<void*>(allocation);
        if (out_mapped)
        {
            *out_mapped = info.pMappedData;
        }
        return true;
    }
}

bool createVertexBufferVk(U32       size_bytes,
                          VkBuffer& out_buffer,
                          void*&    out_allocation,
                          void**    out_mapped)
{
    return createBufferVkImpl(size_bytes,
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                              "createVertexBufferVk",
                              out_buffer, out_allocation, out_mapped);
}

bool createIndexBufferVk(U32       size_bytes,
                         VkBuffer& out_buffer,
                         void*&    out_allocation,
                         void**    out_mapped)
{
    return createBufferVkImpl(size_bytes,
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                              "createIndexBufferVk",
                              out_buffer, out_allocation, out_mapped);
}

void destroyBufferVk(VkBuffer buffer, void* allocation)
{
    if (buffer == VK_NULL_HANDLE && allocation == nullptr)
    {
        return;
    }
    if (sAllocator == VK_NULL_HANDLE)
    {
        // Vulkan 未初期化 / 既に shutdown 後 (caller 側対称呼出を想定、no-op で許容)。
        return;
    }
    vmaDestroyBuffer(sAllocator, buffer, reinterpret_cast<VmaAllocation>(allocation));
}

// r41 sub-step 4.3-β'-3: vkCmdBindVertexBuffers / vkCmdBindIndexBuffer wrap 配置。
// 段階 β' は placement のみ (caller fire は 4.3-ε' per-pool draw 配線時)。
void bindVertexBufferVk(VkCommandBuffer cmd_buf, VkBuffer buffer, VkDeviceSize offset)
{
    if (cmd_buf == VK_NULL_HANDLE || buffer == VK_NULL_HANDLE)
    {
        return;
    }
    VkBuffer     buffers[1] = { buffer };
    VkDeviceSize offsets[1] = { offset };
    vkCmdBindVertexBuffers(cmd_buf,
                           /*firstBinding=*/0,
                           /*bindingCount=*/1,
                           buffers,
                           offsets);
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

} // namespace LLVKLoader
