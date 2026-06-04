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
#include "llglslshader.h"

// <AYAstorm r41 PC-7γ-1> ubo_metadata.inl 取込 (= cadence_tag=PER_FRAME 全 block
// enumerate + block_size 参照に必要、AYA (W6-A) 確認 2026-06-05、initVulkan の
// sFrameUboInstances allocate 配線で g_block_metadata[] walk)。
// </AYAstorm r41 PC-7γ-1>
#include "ubo/ubo_metadata.inl"

#include <vector>
#include <string>
#include <climits>
#include <cstring>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <utility>
#include <atomic>

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
        // <AYAstorm r41 PC-7α> design 07 §3.1 / §3.2 = V3a 5-set unified layout の device
        // limit 適合判定に必要な 4 field。本 PC-7α では query + log のみで分岐は実施せず
        // (= V1' default 80→40/40 split 採用済、本 limit は startup smoke 観察用)。
        // 実 fail-safe (= set=1 不適合 device で起動 abort や追加 split) は PC-7δ で
        // device limit ≥ 40 を assert (= Vulkan 1.3 spec 最小 72 ≥ 40 で全 device 適合)。
        U32  maxDescriptorSetUniformBuffers        = 0; // Vulkan 1.3 minimum 72
        U32  maxDescriptorSetUniformBuffersDynamic = 0; // Vulkan 1.3 minimum 8
        U32  maxPerStageDescriptorUniformBuffers   = 0; // Vulkan 1.3 minimum 12
        U32  minUniformBufferOffsetAlignment       = 0; // typical 256 (= UBO_BUFFER_ALIGNMENT_SAFE)
        // </AYAstorm r41 PC-7α>
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

    // <AYAstorm r41 PC-N-4 (a)> grow flag 新設 (= writeDrawUbo grow 観測 →
    //   endFrame() 末尾 hook 経由 re-wire)。AYA literal「推奨案採用 OK」確認
    //   2026-06-05、ambiguity (N4-1) A 採用 = anonymous namespace 内 file-static
    //   std::atomic<bool>、writeDrawUbo で store(true)、endFrame 内 exchange(false)
    //   で atomic に read+reset。LLUboRingBuffer 改変回避 (= algorithm 層汚染なし)、
    //   既存 LL_WARNS_ONCE site 流用最小、PC-N-1 hook placeholder comment 同形。
    std::atomic<bool> sDrawUboRingBufferGrewThisFrame{false};
    // </AYAstorm r41 PC-N-4 (a)>

    // ------------------------------------------------------------------
    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6ε-2:
    // per-program / per-asset / per-skin cadence dirty propagation 機構。
    //
    // design 06b §3.2.3 で確定の UboInstance owner 概念形 (= std::atomic<bool>
    // dirty 単独) を minimal 形で先行新設。chapter 07 (= PC-7+) で VkBuffer /
    // mapped_ptr / size の member append 拡充予定 (= 本 PC-6ε-2 では placeholder
    // comment、struct shape は不変)。
    //
    // 二段階 dedup 構造 (design 06b §3.2):
    //   stage 1 = setter 入口 mValue cache check (= 既存、値 dedup、06a §5.2)
    //   stage 3 = UBO physical instance dirty bit (= 本 UboInstance::dirty、
    //             upload dedup、本 PC-6ε-2 で wire up)
    //   ※ stage 2 = forwardToUboUpload routing (= 06a §5.4 / 06b §5.2) は
    //                Phase 1.B 既存 stub のまま、本格化は PC-7 で実施
    //
    // gate 配置 (design 06b §5.3 + AYA 確認 2026-06-04):
    //   - per-program = entry gate `if (!shader->mUseUBO) return;` (= 明示 gate、
    //                   shader 個別 mUseUBO 直接参照、design 06a §3.2 整合)
    //   - per-asset / per-skin = 構造的 gate (= setter 側 mUseUBO 分岐で
    //                   forwardToUboUpload 不呼出 → dirty map 空のまま →
    //                   flush で dirty.exchange(false) == false で no-op)
    //   - per-frame / per-draw / singleton = gate 無し (= GATE-B 整合維持、
    //                   PC-6α..ε-1 既存形踏襲)
    //
    // std::atomic<bool> は move / copy 不可、std::unordered_map<K, UboInstance>
    // への値 insert は operator[] (C++17 = piecewise default construct、try_emplace
    // と等価) または try_emplace(key) 経由のみ。本 sub では flush 側 find(key)
    // のみで値 insert は呼ばないため (= dirty=true 経路は PC-7 で完成)、map は
    // 空のまま flush で no-op になる (= 設計上の構造的 gate)。
    // ------------------------------------------------------------------
    // <AYAstorm r41 PC-7β> UboInstance member 拡充 = triple-buffer (FRAMES_IN_FLIGHT=3)
    // 一括確保形 (= design 06b §3.2.3 完成形 + design 07 §8.3 / §8.4 + AYA 確認
    // 2026-06-05 (Y1-A)(Y2-A)(Y3-A)(Y4-A)):
    //   (Y1-A) VkBuffer vk_buffer[FRAMES_IN_FLIGHT] = 3 別 buffer (= sPerFrameUboBuffer
    //          同形、frame rotate write race 回避)
    //   (Y2-A) per-block size = ubo_metadata.inl `block_size` field 個別 allocate
    //          (= memory 節約 + source of truth 整合、PC-7γ allocate call site で
    //           size を渡す)
    //   (Y3-A) allocate/destroy helper のみ wired、actual allocate call site は
    //          PC-7γ で per-owner register hook 配線時に追加 (= 本 PC-7β は
    //          teardown 経路 + helper skeleton のみ)
    //   (Y4-A) vmaCreateBuffer (HOST_VISIBLE + HOST_COHERENT + MAPPED + SEQUENTIAL_WRITE)
    //          = sDrawUboRingBufferRecords factory / LLAssetUboPool 同形、persistent
    //          map で memcpy 直書き化準備
    //
    // member layout 注:
    //   dirty       = std::atomic<bool> = move/copy 不可、map 値 insert は
    //                 try_emplace(key) / operator[] (C++17 piecewise default
    //                 construct) 経由のみ (= PC-6ε-2 確定)
    //   vk_buffer / allocation / mapped_ptr = trivially destructible、
    //                 lifecycle 管理は allocateUboInstanceBuffers /
    //                 destroyUboInstanceBuffers helper で集約 (= struct 単独で
    //                 owner 化はせず、shutdown teardown 経路で sAllocator 生存
    //                 中に明示 destroy)
    //   size        = uint32_t 0 init = allocate 未実施 sentinel (= destroy 側
    //                 でも size==0 で no-op safe)
    // </AYAstorm r41 PC-7β>
    struct UboInstance
    {
        std::atomic<bool> dirty{false};
        VkBuffer          vk_buffer[FRAMES_IN_FLIGHT]  = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
        VmaAllocation     allocation[FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
        void*             mapped_ptr[FRAMES_IN_FLIGHT] = { nullptr, nullptr, nullptr };
        uint32_t          size                         = 0;
    };
    // <AYAstorm r41 PC-7γ-1> per-program UBO physical instance key (= owner shader
    // + block_hash の組)。design 06b §3.2.3 + design 09 §4.1 + AYA (W1-A)(K2)
    // 確認 2026-06-05 整合:
    //   1 program (shader) × N block_hash の組 = 物理 UBO instance 単位で dirty
    //   propagation を行う (= 二段階 dedup 構造 stage 3、block 単位の重複 upload
    //   抑止)。PC-6ε-2 では shader 単位 single dirty 形だったが、PC-7γ-1 で
    //   block_hash dimension を追加 (= 1 shader が複数 block_hash を持つ場合に
    //   block 個別に dirty 立て可能化)。
    //
    // sAssetUboDirty / sSkinUboDirty は PC-7γ-2 で UboAssetKey / UboSkinKey
    // (= <Owner*, block_hash> 対称構造) に key 拡張済 (= 本 file 下記
    // <AYAstorm r41 PC-7γ-2> tag block 参照、AYA (D3-A) 確認 2026-06-05)。
    // </AYAstorm r41 PC-7γ-1>
    using UboInstanceKey = std::pair<LLGLSLShader*, U32 /*block_hash*/>;
    struct UboInstanceKeyHash
    {
        std::size_t operator()(const UboInstanceKey& k) const noexcept
        {
            // std::hash<void*>(owner) ^ std::hash<U32>(block_hash) で combine。
            // owner pointer は 8 byte (64-bit) / 4 byte (32-bit) いずれも分散十分、
            // block_hash は FNV-1a 既分散ゆえ単純 XOR で衝突実用上問題なし
            // (= shader pool 上限が数百規模、block 上限 91)。
            return std::hash<void*>{}(static_cast<void*>(k.first))
                 ^ (std::hash<U32>{}(k.second) << 1);
        }
    };

    // <AYAstorm r41 PC-7γ-2> per-asset / per-skin UBO physical instance key
    // (= owner (Asset*/Skin*) + block_hash の組)。design 06b §2.4 / §2.5 + AYA
    // (D3-A) 確認 2026-06-05 整合 = PC-7γ-1 UboInstanceKey と対称構造で UBO
    // physical instance 単位独立性確保 (= 1 asset が複数 block_hash を持つ場合に
    // block 個別 dirty 立て可能化、PC-6ε-2 の owner 単一 key で発生する dirty
    // 共有 = false sharing を回避)。
    //
    // 注: 現 codegen で PER_ASSET (=3) / PER_SKIN (=4) cadence_tag の entry は
    // 0 件 (= ubo_metadata.inl 2026-06-05 確認)、Asset_*/Skin_* prefix block も
    // 0 件 = 本 PC-7γ-2 は defensive 配線 only。本格化 (= codegen Asset_*/Skin_*
    // block 追加 or synthetic ID scheme + bare OpenGL UBO 置換 + lifecycle hook)
    // は PC-7γ-3 scope。
    // </AYAstorm r41 PC-7γ-2>
    using UboAssetKey = std::pair<LL::GLTF::Asset*, U32 /*block_hash*/>;
    using UboSkinKey  = std::pair<LL::GLTF::Skin*,  U32 /*block_hash*/>;
    struct UboAssetKeyHash
    {
        std::size_t operator()(const UboAssetKey& k) const noexcept
        {
            return std::hash<void*>{}(static_cast<void*>(k.first))
                 ^ (std::hash<U32>{}(k.second) << 1);
        }
    };
    struct UboSkinKeyHash
    {
        std::size_t operator()(const UboSkinKey& k) const noexcept
        {
            return std::hash<void*>{}(static_cast<void*>(k.first))
                 ^ (std::hash<U32>{}(k.second) << 1);
        }
    };

    std::unordered_map<UboInstanceKey, UboInstance, UboInstanceKeyHash> sProgramUboDirty;
    std::unordered_map<UboAssetKey,    UboInstance, UboAssetKeyHash>    sAssetUboDirty;
    std::unordered_map<UboSkinKey,     UboInstance, UboSkinKeyHash>     sSkinUboDirty;

    // <AYAstorm r41 PC-7γ-1> per-frame UBO physical instances (= block_hash → UboInstance、
    // owner 概念無し global static)。design 06b §2.1 + design 07 §8.2 + AYA (W6-A)
    // 確認 2026-06-05 整合:
    //   per-frame cadence (cadence_tag=0) は全 shader で共有、block_hash 単位で
    //   一意な physical instance を持つ。initVulkan 時に ubo_metadata.inl の
    //   g_block_metadata[] を walk して cadence_tag=PER_FRAME (=0) の全 block を
    //   try_emplace + allocateUboInstanceBuffers (block_size 個別) で先回り確保
    //   (= 3 block × FRAMES_IN_FLIGHT=3 = 9 buffer)。shutdownVulkan で対称 destroy。
    //
    // sProgramUboDirty と独立 (= PER_PROGRAM は shader × block_hash key、PER_FRAME は
    // block_hash 単独 key) ゆえ別 map で管理、forwardToUboUpload switch case で
    // cadence_tag に応じて分岐参照。
    // </AYAstorm r41 PC-7γ-1>
    std::unordered_map<U32 /*block_hash*/, UboInstance> sFrameUboInstances;

    // <AYAstorm r41 PC-7γ-2> per-asset / per-skin current owner tracking
    // (= sCurrentAsset / sCurrentSkin static)。design 06b §5.2 + §5.4.1 + AYA
    // (D2-A) 確認 2026-06-05 整合 = main thread 専有ゆえ atomic 不要、static
    // で thread-safe。
    //
    // 役割: forwardToUboUpload PER_ASSET / PER_SKIN case が "current owner" を
    //       解決する経路。gltfscenemanager.cpp の asset/skin draw 直前で set、
    //       直後 / loop end で clear。forwardToUboUpload 内側で getCurrentAsset
    //       / getCurrentSkin accessor 経由読出 (= anonymous ns 直接参照不可)。
    //
    // 注: 現 codegen で PER_ASSET / PER_SKIN cadence_tag は 0 件で実走しない
    //     (= defensive 配線)、本 PC-7γ-2 は将来 PC-7γ-3 通電時の owner 解決
    //     経路を先回り確立。sCurrent* が null の場合 forwardToUboUpload 側で
    //     LL_WARNS_ONCE + return (= 06b §5.4.1 main thread 専有前提下の安全側)。
    // </AYAstorm r41 PC-7γ-2>
    LL::GLTF::Asset* sCurrentAsset = nullptr;
    LL::GLTF::Skin*  sCurrentSkin  = nullptr;

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

    // ------------------------------------------------------------------
    // <AYAstorm r41 PC-7α> V3a 5-set unified descriptor set layout 群
    // = design 06c §2 (descriptor set 4 帯 cadence 別配置 + V1' split 採用で
    //   set=1 → set=1a + set=1b の論理 5 帯) + design 07 §3.2 (V1') + §9.1
    //   (sAYAStandardLayout pipeline layout = 5 set layout array + push constant
    //   64 B) + §6.3 (4 pool 容量) 整合。
    //
    // 本 PC-7α は **scaffolding 純化** = VkObject 化のみ (= vkCreateDescriptorSetLayout
    // / vkCreateDescriptorPool / vkCreatePipelineLayout 5+4+1=10 件)、bind 経路は
    // PC-7δ で通電 (= vkCmdBindDescriptorSets 呼出 + set=3 swap、本 PC-7α では
    // 既存 placeholder bind path 不変)。
    //
    // 既存 (Phase 1.B / PC-6α-β-γ) descriptor object 群との並存方針 (= AYA 確認
    // 2026-06-05 Z1-B + Z3-A):
    //   - sPerFrameDescriptorSetLayout (2 binding) : Phase 1.B PerFrameMatrixUBO 用、
    //     PC-7α では不変 (= sFrameUboLayoutV3a 別新設で並存)
    //   - sPerMaterialDescriptorSetLayout (7 binding COMBINED_IMAGE_SAMPLER) : 同上、
    //     PC-7α では不変 (= sDrawUboLayoutV3a 別新設で並存、X2-B sampler 除外で UBO のみ)
    //   - sAssetUboPoolMgr (LLAssetUboPool grow-only) : Phase 1.B PC-6α (W2) 用、
    //     PC-7α では不変 (= sAssetUboPoolV3a 別新設で並存)
    //   - sDrawUboRingBufferMgr (LLUboRingBuffer) : Phase 1.B PC-6β (RB) 用、
    //     PC-7α では不変 (= sDrawUboPoolV3a 別新設で並存)
    //
    // X2-B (= sampler 除外) 整合: V3a 5-set は全 binding `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
    // または `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC` (set=2 のみ)、COMBINED_IMAGE_SAMPLER
    // は PC-7α scope 外 (= S3' design 07 §5.2 / §10 持越、PC-7+ で別 sub-step)。
    //
    // GATE-B 整合: 本 scaffolding は Vulkan init 層単独動作、mUseUBO runtime gate 不参照、
    // #ifdef LL_VULKAN_GLSL 新規追加 0 件 (= PC-6α..ζ 同形)。
    // MUSEUBO-A 整合: bind 未通電 = mUseUBO=false default で既存 OpenGL 描画 100% 維持。
    // ------------------------------------------------------------------
    constexpr U32 V3A_FRAME_SET_BINDINGS     = 4;   // set=0: FrameViewProj + FrameLights + FrameAtmosphere + Global_ReflectionProbes
    constexpr U32 V3A_PROGRAM_SET_A_BINDINGS = 40;  // set=1a: Program_* 40 個 (= V1' split first half)
    constexpr U32 V3A_PROGRAM_SET_B_BINDINGS = 40;  // set=1b: Program_* 40 個 (= V1' split second half、Q22-NUM 解消 A' 反映で 24+54=78 → 39+39 だが + 余地で 40 一律確保)
    constexpr U32 V3A_DRAW_SET_BINDINGS      = 4;   // set=2: Draw_LightParams + Draw_MultiLight + MaterialUBO + MaterialLegacyBlinn (UBO_DYNAMIC)
    constexpr U32 V3A_ASSET_SET_BINDINGS     = 3;   // set=3: Asset_GLTFNodes + Asset_GLTFMaterials + Skin_GLTFJoints (X2-B sampler 除外)

    // design 07 §6.3 4 pool 初期容量 (= 本 PC-7α では bind 未通電のため maxSets は scaffold 最小値)
    constexpr U32 V3A_FRAME_POOL_MAX_SETS     = FRAMES_IN_FLIGHT;            // 3 (= per-frame 1 set × FRAMES_IN_FLIGHT)
    constexpr U32 V3A_PROGRAM_POOL_MAX_SETS   = 6;                            // 1 active program × FRAMES_IN_FLIGHT=3 × 2 帯 (1a/1b)、design 07 §6.2 (b)
    constexpr U32 V3A_DRAW_POOL_MAX_SETS      = 1;                            // ring buffer + dynamic offset rotation で 1 set 固定 (= L1+L2、design 07 §7.4)
    constexpr U32 V3A_ASSET_POOL_MAX_SETS_INI = 64u * FRAMES_IN_FLIGHT;       // 64 asset × 3 frame = 192 (= LLAssetUboPool 初期 chunk と同 sizing)

    VkDescriptorSetLayout sFrameUboLayoutV3a    = VK_NULL_HANDLE;  // set=0 (per-frame + singleton)
    VkDescriptorSetLayout sProgramUboLayoutA    = VK_NULL_HANDLE;  // set=1a (per-program first half)
    VkDescriptorSetLayout sProgramUboLayoutB    = VK_NULL_HANDLE;  // set=1b (per-program second half)
    VkDescriptorSetLayout sDrawUboLayoutV3a     = VK_NULL_HANDLE;  // set=2 (per-draw UBO_DYNAMIC)
    VkDescriptorSetLayout sAssetUboLayoutV3a    = VK_NULL_HANDLE;  // set=3 (per-asset + per-skin、X2-B sampler 除外)

    VkDescriptorPool      sFrameUboPoolV3a      = VK_NULL_HANDLE;
    VkDescriptorPool      sProgramUboPoolV3a    = VK_NULL_HANDLE;
    VkDescriptorPool      sDrawUboPoolV3a       = VK_NULL_HANDLE;
    VkDescriptorPool      sAssetUboPoolV3a      = VK_NULL_HANDLE;

    VkPipelineLayout      sAYAStandardLayout    = VK_NULL_HANDLE;  // 5 set layout array + push constant 64 B
    // </AYAstorm r41 PC-7α>

    // ------------------------------------------------------------------
    // <AYAstorm r41 PC-7δ> V3a 5-set 実体 (= VkDescriptorSet array) + per-singleton
    // UboInstance map = PC-7δ scope (vkCmdBindDescriptorSets 通電 + set=3 swap +
    // sAYAStandardLayout 経由 bind + SINGLETON case 本格化) で新設。
    //
    // 設計根拠 (= 2026-06-05 AYA literal「OK」 record + design 06c §2-§5 +
    // design 07 §4.4.1 / §6.4 / §8.4 / §9.1 / §9.2):
    //   (H2-A) initVulkan で V3a pool から eager allocate (= FRAMES_IN_FLIGHT=3
    //          × cadence 固定数を一括 = 計 13 set)。grow only pool 整合
    //          (= design 07 §6.4) + per-frame churn 回避。
    //   (H3-A) vkUpdateDescriptorSets timing = register*Ubo 内 = UboInstance 確保
    //          直後に update (= per-instance pair、hot path 除外)。
    //   (H5-A) sSingletonUboInstances は sFrameUboInstances 別 map (= cadence
    //          隔離、map key 単純化 = block_hash 単独)。writeSingletonUbo +
    //          flushSingletonUbos 経由 setter ↔ flush 2 経路統合。
    //
    // 13 set 内訳:
    //   set=0 sFrameUboSetV3a    × FRAMES_IN_FLIGHT (= 3) (per-frame + singleton 同居)
    //   set=1a sProgramUboSetA   × FRAMES_IN_FLIGHT (= 3) (per-program first half)
    //   set=1b sProgramUboSetB   × FRAMES_IN_FLIGHT (= 3) (per-program second half)
    //   set=2 sDrawUboSetV3a     × 1                       (ring buffer + dynamic offset で 1 set 固定)
    //   set=3 sAssetUboSetV3a    × FRAMES_IN_FLIGHT (= 3) (per-asset + per-skin 同居)
    //
    // shutdownVulkan teardown は pool destroy 時に implicit free
    // (= FREE_DESCRIPTOR_SET_BIT 不付与 grow only pool、design 07 §6.4 整合)、
    // 別途 vkFreeDescriptorSets 呼出不要。array は VK_NULL_HANDLE reset のみ。
    //
    // GATE-B 整合: #ifdef LL_VULKAN_GLSL 新規追加 0 件 (= host C++ Vulkan init 層単独)
    // MUSEUBO-A 整合: bind 実発火は placeholder draw 経路 (= sFramebuffer offscreen
    //                FBO、画面到達なし)、mUseUBO=false default で setter→
    //                forwardToUboUpload 不到達、既存 OpenGL 描画 100% 維持。
    // ------------------------------------------------------------------
    VkDescriptorSet sFrameUboSetV3a [FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    VkDescriptorSet sProgramUboSetA [FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    VkDescriptorSet sProgramUboSetB [FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    VkDescriptorSet sDrawUboSetV3a                     = VK_NULL_HANDLE;
    VkDescriptorSet sAssetUboSetV3a [FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };

    // sSingletonUboInstances = block_hash 単独 key の per-singleton UboInstance map
    // (= sFrameUboInstances と独立)。現 codegen で SINGLETON 1 件
    // (= Global_ReflectionProbes、block_hash=0xabdfdb31、set=0 binding=3、size=256)、
    // initVulkan で先回り allocate + vkUpdateDescriptorSets で sFrameUboSetV3a の
    // binding=3 に紐付け。
    std::unordered_map<U32 /*block_hash*/, UboInstance> sSingletonUboInstances;
    // </AYAstorm r41 PC-7δ>

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
        // <AYAstorm r41 PC-7α> design 07 §3.1 / §3.2: V3a 5-set 適合判定 4 field 取込み。
        sDeviceLimits.maxDescriptorSetUniformBuffers        = l.maxDescriptorSetUniformBuffers;
        sDeviceLimits.maxDescriptorSetUniformBuffersDynamic = l.maxDescriptorSetUniformBuffersDynamic;
        sDeviceLimits.maxPerStageDescriptorUniformBuffers   = l.maxPerStageDescriptorUniformBuffers;
        sDeviceLimits.minUniformBufferOffsetAlignment       = (U32)l.minUniformBufferOffsetAlignment;
        // </AYAstorm r41 PC-7α>

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
        // <AYAstorm r41 PC-7α> design 07 §3.1 / §3.2: V3a 5-set 適合判定 4 field log。
        // set=1 80 binding split 採用 (V1' 40/40)、Vulkan 1.3 spec 最小 72 ≥ 40 で
        // 全 device 必ず 1 set 適合 (= 本 PC-7α では fail-safe 不要)。
        LL_INFOS("Vulkan") << "  maxDescriptorSetUniformBuffers        = "
                           << sDeviceLimits.maxDescriptorSetUniformBuffers
                           << " (Vulkan 1.3 minimum 72、V1' split 40/40 で適合)" << LL_ENDL;
        LL_INFOS("Vulkan") << "  maxDescriptorSetUniformBuffersDynamic = "
                           << sDeviceLimits.maxDescriptorSetUniformBuffersDynamic
                           << " (Vulkan 1.3 minimum 8、set=2 dynamic offset 上限)" << LL_ENDL;
        LL_INFOS("Vulkan") << "  maxPerStageDescriptorUniformBuffers   = "
                           << sDeviceLimits.maxPerStageDescriptorUniformBuffers
                           << " (Vulkan 1.3 minimum 12)" << LL_ENDL;
        LL_INFOS("Vulkan") << "  minUniformBufferOffsetAlignment       = "
                           << sDeviceLimits.minUniformBufferOffsetAlignment
                           << " bytes (typical 256、ring buffer chunk alignment 整合)" << LL_ENDL;
        // </AYAstorm r41 PC-7α>
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

    // <AYAstorm r41 PC-7β> UboInstance triple-buffer allocate / destroy helper
    //
    // = design 06b §3.2.3 完成形 (VkBuffer / mapped_ptr / size 実体化) + design 07
    //   §8.3 (per-program/asset/skin cadence triple-buffering 必須) + §8.4 (frame
    //   index 共有) + AYA 確認 2026-06-05 (Y3-A) + (Y4-A) 整合。
    //
    // (Y3-A) actual allocate call site は PC-7γ で per-owner register hook 配線時に
    //   追加。本 PC-7β は helper skeleton + shutdown teardown 経路のみ wired
    //   (= 起動時全件先回り allocate / map insert path は本 sub 範囲外)。
    //
    // (Y4-A) vmaCreateBuffer (HOST_VISIBLE + HOST_COHERENT + MAPPED + SEQUENTIAL_WRITE)
    //   = sDrawUboRingBufferRecords factory 同形、persistent map 経由で memcpy
    //   直書き化準備 (PC-7γ scope)。failure 時は確保済 frame を destroy で巻き戻し
    //   して全件 false return = 部分 allocate state を残さない。
    //
    // GATE-B 整合: 本 helper は Vulkan init 層単独動作、mUseUBO runtime gate 不参照
    // (= PC-6α..ζ 同形)。MUSEUBO-A 整合: 本 PC-7β では call site 未配線
    // (= dirty map 空のまま) で既存 OpenGL 描画 100% 維持。
    //
    // PC-7γ-1 で [[maybe_unused]] 撤去 = registerProgramUbo / initVulkan
    // sFrameUboInstances allocate path から呼出開始 (= AYA (W4-A)(W6-A)(W8-A)
    // 確認 2026-06-05、Exit Criteria 9 項目 (vi) 整合)。destroy 側は
    // shutdownVulkan + unregisterProgramUbo 経路から呼出。
    bool allocateUboInstanceBuffers(UboInstance& ubo, uint32_t size, const char* owner_tag)
    {
        if (sAllocator == VK_NULL_HANDLE || size == 0)
        {
            LL_WARNS("Vulkan") << "allocateUboInstanceBuffers: invalid args (sAllocator="
                               << (sAllocator == VK_NULL_HANDLE ? "NULL" : "OK")
                               << ", size=" << (S32)size
                               << ", owner_tag=" << (owner_tag ? owner_tag : "?") << ")" << LL_ENDL;
            return false;
        }

        for (U32 frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            VkBufferCreateInfo bci = {};
            bci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bci.size        = size;
            bci.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo aci = {};
            aci.usage         = VMA_MEMORY_USAGE_AUTO;
            aci.flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                              | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            aci.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            VmaAllocationInfo info = {};
            VkResult r = vmaCreateBuffer(sAllocator, &bci, &aci,
                                         &ubo.vk_buffer[frame], &ubo.allocation[frame], &info);
            if (r != VK_SUCCESS || info.pMappedData == nullptr)
            {
                LL_WARNS("Vulkan") << "allocateUboInstanceBuffers vmaCreateBuffer failed: result="
                                   << (S32)r << " size=" << (S32)size << " frame=" << frame
                                   << " owner_tag=" << (owner_tag ? owner_tag : "?") << LL_ENDL;
                if (ubo.vk_buffer[frame] != VK_NULL_HANDLE)
                {
                    vmaDestroyBuffer(sAllocator, ubo.vk_buffer[frame], ubo.allocation[frame]);
                    ubo.vk_buffer[frame]  = VK_NULL_HANDLE;
                    ubo.allocation[frame] = VK_NULL_HANDLE;
                    ubo.mapped_ptr[frame] = nullptr;
                }
                for (U32 prev = 0; prev < frame; ++prev)
                {
                    vmaDestroyBuffer(sAllocator, ubo.vk_buffer[prev], ubo.allocation[prev]);
                    ubo.vk_buffer[prev]  = VK_NULL_HANDLE;
                    ubo.allocation[prev] = VK_NULL_HANDLE;
                    ubo.mapped_ptr[prev] = nullptr;
                }
                return false;
            }
            ubo.mapped_ptr[frame] = info.pMappedData;
        }
        ubo.size = size;
        return true;
    }

    void destroyUboInstanceBuffers(UboInstance& ubo)
    {
        if (sAllocator == VK_NULL_HANDLE || ubo.size == 0)
        {
            return;
        }
        for (U32 frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            if (ubo.vk_buffer[frame] != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(sAllocator, ubo.vk_buffer[frame], ubo.allocation[frame]);
            }
            ubo.vk_buffer[frame]  = VK_NULL_HANDLE;
            ubo.allocation[frame] = VK_NULL_HANDLE;
            ubo.mapped_ptr[frame] = nullptr;
        }
        ubo.size = 0;
    }
    // </AYAstorm r41 PC-7β>

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

    // <AYAstorm r41 PC-7δ> bindV3aStatic / bindV3aRigged = V3a 5-set bind 経路 helper
    // (= design 07 §4.4.1 「論理 5 set → bind 時 4 set 縮減」 + §9.2 set=3 swap
    // configuration、maxBoundDescriptorSets=4 死守)。
    //
    // bind 構成:
    //   static draw : set=0 (frame+singleton) + set=1a (program A) + set=1b (program B) + set=2 (per-draw)
    //   rigged draw : set=0                  + set=1a              + set=1b              + set=3 (per-asset+per-skin)
    //
    // 単一 vkCmdBindDescriptorSets 呼出で firstSet=0 + descriptorSetCount=4 を渡す
    // (= contiguous 4 set bind、Vulkan spec で firstSet 起点の連続 set を 1 呼出で bind)。
    // ただし static は set=0/1a/1b/2 (= 連続)、rigged は set=0/1a/1b/3 (= set=2 skip + set=3)
    // ゆえ rigged は 2 回 vkCmdBindDescriptorSets 呼出に分割
    // (= set=0..1b 3 set bind 1 回 + set=3 1 set bind 1 回)。
    //
    // <AYAstorm r41 PC-7ε (c)> bindV3aStatic signature 拡張 = caller (= recordPlaceholderPoolDraw)
    // から sDrawUboRingBufferMgr->allocate() の AllocateResult.offset を 4 binding 分受領
    // (= (ε-3) A 採用 = caller 責任分離 / allocate ↔ bind を call site で連結明示、AYA
    // 確認 2026-06-05)。本 PC-7ε は per-draw chain で同 offset × 4 binding を渡す
    // placeholder 形 (= 1 allocate per draw、(ε-2) A)、PC-N 実 GLTF 通電時に 4 個独立
    // offset へ拡張。
    //
    // set=2 (sDrawUboLayoutV3a) は UBO_DYNAMIC で V3A_DRAW_SET_BINDINGS=4 個分
    // dynamic offset 必要 (= Vulkan spec: descriptorSetCount で参照する set 群が
    // 持つ UBO_DYNAMIC binding 数の総和 = pDynamicOffsets 配列長)。
    //
    // 早期 return: cmd_buf / layout NULL + 必要な set 群が VK_NULL_HANDLE な状態は
    // initVulkan 失敗 or scaffolding 未通電を意味、no-op safe (= MUSEUBO-A 整合)。
    // dynamic_offsets == nullptr は caller 側 ring buffer 未初期化 → 同様 no-op safe。
    void bindV3aStatic(VkCommandBuffer cmd_buf,
                       U32             frame_index,
                       const U32       dynamic_offsets[V3A_DRAW_SET_BINDINGS])
    {
        if (cmd_buf == VK_NULL_HANDLE ||
            sAYAStandardLayout == VK_NULL_HANDLE ||
            frame_index >= FRAMES_IN_FLIGHT ||
            sFrameUboSetV3a[frame_index]    == VK_NULL_HANDLE ||
            sProgramUboSetA[frame_index]    == VK_NULL_HANDLE ||
            sProgramUboSetB[frame_index]    == VK_NULL_HANDLE ||
            sDrawUboSetV3a                  == VK_NULL_HANDLE ||
            dynamic_offsets                 == nullptr)
        {
            return;
        }

        const VkDescriptorSet sets[4] = {
            sFrameUboSetV3a[frame_index],
            sProgramUboSetA[frame_index],
            sProgramUboSetB[frame_index],
            sDrawUboSetV3a,
        };

        vkCmdBindDescriptorSets(cmd_buf,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                sAYAStandardLayout,
                                /*firstSet=*/0,
                                /*descriptorSetCount=*/4,
                                sets,
                                /*dynamicOffsetCount=*/V3A_DRAW_SET_BINDINGS,
                                dynamic_offsets);
    }
    // </AYAstorm r41 PC-7ε (c)>

    // <AYAstorm r41 PC-N-2 (a)> bindV3aRigged signature 拡張 = bindV3aStatic 同形
    //   (= const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS] 引数追加 + guard 拡張
    //   sDrawUboSetV3a + dynamic_offsets nullptr 追加、(N2-1) A + (N2-5) A 採用、
    //   AYA literal「Claude 推奨案 OK」確認 2026-06-05)。
    // <AYAstorm r41 PC-N-2 (b)> body 内 set=2 bind 復活 = 第 1 vkCmdBindDescriptorSets
    //   を 3 set → 4 set (= set=0/1a/1b/2) + pDynamicOffsets に引数 wire、第 2 call
    //   (= set=3 swap) は構造維持 ((N2-1) A + (N2-3) A + (N2-4) A 採用)。
    void bindV3aRigged(VkCommandBuffer cmd_buf,
                       U32             frame_index,
                       const U32       dynamic_offsets[V3A_DRAW_SET_BINDINGS])
    {
        if (cmd_buf == VK_NULL_HANDLE ||
            sAYAStandardLayout == VK_NULL_HANDLE ||
            frame_index >= FRAMES_IN_FLIGHT ||
            sFrameUboSetV3a[frame_index]    == VK_NULL_HANDLE ||
            sProgramUboSetA[frame_index]    == VK_NULL_HANDLE ||
            sProgramUboSetB[frame_index]    == VK_NULL_HANDLE ||
            sDrawUboSetV3a                  == VK_NULL_HANDLE ||
            sAssetUboSetV3a[frame_index]    == VK_NULL_HANDLE ||
            dynamic_offsets                 == nullptr)
        {
            return;
        }

        // set=0/1a/1b/2 4 set 連続 bind (= bindV3aStatic 同形、(N2-1) A)
        const VkDescriptorSet sets_0_to_2[V3A_DRAW_SET_BINDINGS] = {
            sFrameUboSetV3a[frame_index],
            sProgramUboSetA[frame_index],
            sProgramUboSetB[frame_index],
            sDrawUboSetV3a,
        };
        vkCmdBindDescriptorSets(cmd_buf,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                sAYAStandardLayout,
                                /*firstSet=*/0,
                                /*descriptorSetCount=*/V3A_DRAW_SET_BINDINGS,
                                sets_0_to_2,
                                /*dynamicOffsetCount=*/V3A_DRAW_SET_BINDINGS,
                                dynamic_offsets);

        // set=3 単独 bind (= set=2 ↔ set=3 swap 実走、(N2-4) A 構造維持)
        vkCmdBindDescriptorSets(cmd_buf,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                sAYAStandardLayout,
                                /*firstSet=*/3,
                                /*descriptorSetCount=*/1,
                                &sAssetUboSetV3a[frame_index],
                                /*dynamicOffsetCount=*/0,
                                /*pDynamicOffsets=*/nullptr);
    }
    // </AYAstorm r41 PC-N-2 (a)+(b)>

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

    // ------------------------------------------------------------------
    // <AYAstorm r41 PC-7α> V3a 5-set unified descriptor set layout / pool /
    // pipeline layout scaffolding helpers
    //
    // 設計準拠:
    //   - design 06c §2.2 set=0 (per-frame + singleton) — 4 binding UBO
    //   - design 06c §2.3 / 07 §3.2 V1' set=1 split → set=1a (40 binding) +
    //                                              set=1b (40 binding) UBO
    //   - design 06c §2.4 set=2 (per-draw + Material MC1 統合) — 4 binding
    //                                          UBO_DYNAMIC (= L1+L2 ring buffer)
    //   - design 06c §2.5 set=3 (per-asset + per-skin) — 3 binding UBO
    //                                          (X2-B sampler 除外、PC-7+ 別 sub-step)
    //   - design 07 §6.3 4 pool 容量 (= V3A_*_POOL_MAX_SETS_*)
    //   - design 07 §9.1 sAYAStandardLayout pipeline layout = 5 set + push const 64 B
    //
    // 本 PC-7α では Vulkan object 化のみ (= bind 経路未通電、vkCmdBindDescriptorSets
    // 呼出は PC-7δ scope)。layout / pool / pipeline layout の object は createDevice
    // 後 → PSO 作成前に立ち上げ、shutdownVulkan() で逆順 destroy。
    // ------------------------------------------------------------------
    bool createV3aDescriptorSetLayouts()
    {
        // 共通 stage = VERTEX | FRAGMENT (= 既存 sPerFrameDescriptorSetLayout と整合、
        // shader 側 layout(set=N, binding=M) 宣言は VK_SHADER_STAGE_ALL_GRAPHICS 想定
        // でも host 側は 2 stage flag で十分、PSO bind 時に shader 側で参照される
        // binding のみ visible、未参照 binding は no-op で safe)。
        constexpr VkShaderStageFlags kV3aStageFlags =
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        auto build_ubo_layout = [&](VkDescriptorType descriptor_type,
                                    U32              binding_count,
                                    VkDescriptorSetLayout& out_layout,
                                    const char*      label) -> bool
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings(binding_count);
            for (U32 i = 0; i < binding_count; ++i)
            {
                bindings[i].binding         = i;
                bindings[i].descriptorType  = descriptor_type;
                bindings[i].descriptorCount = 1;
                bindings[i].stageFlags      = kV3aStageFlags;
                bindings[i].pImmutableSamplers = nullptr;
            }

            VkDescriptorSetLayoutCreateInfo info = {};
            info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            info.bindingCount = binding_count;
            info.pBindings    = bindings.data();

            VkResult result = vkCreateDescriptorSetLayout(sDevice, &info, nullptr, &out_layout);
            if (result != VK_SUCCESS)
            {
                LL_WARNS("Vulkan") << "vkCreateDescriptorSetLayout (" << label
                                   << ") failed: " << (S32)result << LL_ENDL;
                return false;
            }
            return true;
        };

        if (!build_ubo_layout(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                              V3A_FRAME_SET_BINDINGS,
                              sFrameUboLayoutV3a,
                              "V3a set=0 per-frame+singleton"))
        {
            return false;
        }
        if (!build_ubo_layout(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                              V3A_PROGRAM_SET_A_BINDINGS,
                              sProgramUboLayoutA,
                              "V3a set=1a per-program first-half"))
        {
            return false;
        }
        if (!build_ubo_layout(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                              V3A_PROGRAM_SET_B_BINDINGS,
                              sProgramUboLayoutB,
                              "V3a set=1b per-program second-half"))
        {
            return false;
        }
        if (!build_ubo_layout(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                              V3A_DRAW_SET_BINDINGS,
                              sDrawUboLayoutV3a,
                              "V3a set=2 per-draw UBO_DYNAMIC"))
        {
            return false;
        }
        if (!build_ubo_layout(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                              V3A_ASSET_SET_BINDINGS,
                              sAssetUboLayoutV3a,
                              "V3a set=3 per-asset+per-skin"))
        {
            return false;
        }

        LL_INFOS("Vulkan") << "V3a 5-set descriptor set layouts created"
                           << " (set=0 binding=" << V3A_FRAME_SET_BINDINGS
                           << " / set=1a binding=" << V3A_PROGRAM_SET_A_BINDINGS
                           << " / set=1b binding=" << V3A_PROGRAM_SET_B_BINDINGS
                           << " / set=2 UBO_DYNAMIC binding=" << V3A_DRAW_SET_BINDINGS
                           << " / set=3 binding=" << V3A_ASSET_SET_BINDINGS
                           << ", X2-B sampler 除外)" << LL_ENDL;
        return true;
    }

    bool createV3aDescriptorPools()
    {
        auto build_pool = [&](VkDescriptorType descriptor_type,
                              U32              max_sets,
                              U32              descriptor_count,
                              VkDescriptorPool& out_pool,
                              const char*      label) -> bool
        {
            VkDescriptorPoolSize pool_size = {};
            pool_size.type            = descriptor_type;
            pool_size.descriptorCount = descriptor_count;

            VkDescriptorPoolCreateInfo info = {};
            info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            info.maxSets       = max_sets;
            info.poolSizeCount = 1;
            info.pPoolSizes    = &pool_size;
            // FREE_DESCRIPTOR_SET_BIT 不付与 (= grow only、shutdown 時 reverse 順 destroy で十分、
            //                                  design 07 §6.4 整合)
            info.flags         = 0;

            VkResult result = vkCreateDescriptorPool(sDevice, &info, nullptr, &out_pool);
            if (result != VK_SUCCESS)
            {
                LL_WARNS("Vulkan") << "vkCreateDescriptorPool (" << label
                                   << ") failed: " << (S32)result << LL_ENDL;
                return false;
            }
            return true;
        };

        // sFrameUboPool: per-frame 1 set × FRAMES_IN_FLIGHT (= 3 set rotate、design 07 §7 / §8.4)
        if (!build_pool(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        V3A_FRAME_POOL_MAX_SETS,
                        V3A_FRAME_SET_BINDINGS * V3A_FRAME_POOL_MAX_SETS,
                        sFrameUboPoolV3a,
                        "V3a sFrameUboPool"))
        {
            return false;
        }

        // sProgramUboPool: 1 active program × 3 frame × 2 帯 = 6 set (= design 07 §6.2 (b))
        // descriptor count = (40 + 40) × 3 frame = 240 (= 1a 40×3 + 1b 40×3、本 pool は両 layout 共有)
        if (!build_pool(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        V3A_PROGRAM_POOL_MAX_SETS,
                        (V3A_PROGRAM_SET_A_BINDINGS + V3A_PROGRAM_SET_B_BINDINGS) * FRAMES_IN_FLIGHT,
                        sProgramUboPoolV3a,
                        "V3a sProgramUboPool"))
        {
            return false;
        }

        // sDrawUboPool: ring buffer + dynamic offset で 1 set 固定 (= L1+L2、design 07 §7.4)
        if (!build_pool(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                        V3A_DRAW_POOL_MAX_SETS,
                        V3A_DRAW_SET_BINDINGS * V3A_DRAW_POOL_MAX_SETS,
                        sDrawUboPoolV3a,
                        "V3a sDrawUboPool"))
        {
            return false;
        }

        // sAssetUboPool: 64 asset × 3 frame 初期 (= design 07 §6.3、scene 拡大時は将来 PC-7+ で grow 配線)
        if (!build_pool(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        V3A_ASSET_POOL_MAX_SETS_INI,
                        V3A_ASSET_SET_BINDINGS * V3A_ASSET_POOL_MAX_SETS_INI,
                        sAssetUboPoolV3a,
                        "V3a sAssetUboPool"))
        {
            return false;
        }

        LL_INFOS("Vulkan") << "V3a 4 descriptor pools created"
                           << " (sFrameUboPool maxSets=" << V3A_FRAME_POOL_MAX_SETS
                           << " / sProgramUboPool maxSets=" << V3A_PROGRAM_POOL_MAX_SETS
                           << " / sDrawUboPool maxSets=" << V3A_DRAW_POOL_MAX_SETS
                           << " / sAssetUboPool maxSets=" << V3A_ASSET_POOL_MAX_SETS_INI
                           << ")" << LL_ENDL;
        return true;
    }

    bool createAYAStandardPipelineLayout()
    {
        // design 07 §9.1: 5 set layout array (set=0/1a/1b/2/3) + push constant 64 B (= modelview matrix)
        // vkCmdBindDescriptorSets 時の同時 bind 数は 4 set 上限 (= maxBoundDescriptorSets=4 死守、
        // design 07 §2.3 / §4.4 / §9.2 set=3 swap で対応)。本 pipeline layout は **5 set 全て** を
        // 包含する logical layout (= shader 側 layout(set=N, binding=M) 宣言の最大集合に一致)、
        // 実 bind 時は set=0..2 + (set=1a/1b/2 のいずれか × 4 slot) で 4 set 構成。
        const VkDescriptorSetLayout v3a_set_layouts[5] = {
            sFrameUboLayoutV3a,    // set=0
            sProgramUboLayoutA,    // set=1a
            sProgramUboLayoutB,    // set=1b
            sDrawUboLayoutV3a,     // set=2
            sAssetUboLayoutV3a,    // set=3
        };

        VkPushConstantRange push_range = {};
        push_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        push_range.offset     = 0;
        push_range.size       = 64;  // modelview matrix 1 件 (= 4×4 mat × 4 B)

        sAYAStandardLayout = createStandardPipelineLayout(v3a_set_layouts, 5, &push_range, 1);
        if (sAYAStandardLayout == VK_NULL_HANDLE)
        {
            LL_WARNS("Vulkan") << "createAYAStandardPipelineLayout failed (sAYAStandardLayout)" << LL_ENDL;
            return false;
        }

        LL_INFOS("Vulkan") << "sAYAStandardLayout pipeline layout created (5 set + push constant 64 B、"
                           << "bind 経路未通電 = PC-7δ scope)" << LL_ENDL;
        return true;
    }

    // <AYAstorm r41 PC-7δ> V3a 5-set 実体を 4 V3a pool から eager allocate。
    // 計 13 set (= sFrameUboSetV3a×3 + sProgramUboSetA×3 + sProgramUboSetB×3 +
    // sDrawUboSetV3a×1 + sAssetUboSetV3a×3) を initVulkan で 1 度確保。
    // sFrameUboPool maxSets=3 / sProgramUboPool maxSets=6 / sDrawUboPool
    // maxSets=1 / sAssetUboPool maxSets=192 で全件収容可能 (= design 07 §6.3
    // 整合)。
    //
    // 失敗時は本 helper が false return + 既 allocate 済 set は VK_NULL_HANDLE
    // 状態維持 (= 個別 vkFreeDescriptorSets 呼出は不要 = grow only pool、
    // shutdownVulkan の vkDestroyDescriptorPool で implicit free)。
    bool createV3aDescriptorSets()
    {
        if (sDevice == VK_NULL_HANDLE)
        {
            return false;
        }

        auto allocate_sets = [](VkDescriptorPool          pool,
                                VkDescriptorSetLayout     layout,
                                U32                       count,
                                VkDescriptorSet*          out_sets,
                                const char*               label) -> bool
        {
            if (pool == VK_NULL_HANDLE || layout == VK_NULL_HANDLE || count == 0)
            {
                LL_WARNS("Vulkan") << "createV3aDescriptorSets allocate_sets ("
                                   << (label ? label : "?")
                                   << "): invalid pool/layout/count" << LL_ENDL;
                return false;
            }

            // 同一 layout を count 件分の array で渡す (= vkAllocateDescriptorSets spec:
            // pSetLayouts は descriptorSetCount 要素配列)。
            std::vector<VkDescriptorSetLayout> layouts(count, layout);
            VkDescriptorSetAllocateInfo info = {};
            info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            info.descriptorPool     = pool;
            info.descriptorSetCount = count;
            info.pSetLayouts        = layouts.data();

            VkResult r = vkAllocateDescriptorSets(sDevice, &info, out_sets);
            if (r != VK_SUCCESS)
            {
                LL_WARNS("Vulkan") << "vkAllocateDescriptorSets (" << (label ? label : "?")
                                   << ") failed: result=" << (S32)r
                                   << " count=" << count << LL_ENDL;
                return false;
            }
            return true;
        };

        if (!allocate_sets(sFrameUboPoolV3a, sFrameUboLayoutV3a,
                           FRAMES_IN_FLIGHT, sFrameUboSetV3a,
                           "set=0 frame+singleton"))
        {
            return false;
        }
        if (!allocate_sets(sProgramUboPoolV3a, sProgramUboLayoutA,
                           FRAMES_IN_FLIGHT, sProgramUboSetA,
                           "set=1a per-program A"))
        {
            return false;
        }
        if (!allocate_sets(sProgramUboPoolV3a, sProgramUboLayoutB,
                           FRAMES_IN_FLIGHT, sProgramUboSetB,
                           "set=1b per-program B"))
        {
            return false;
        }
        if (!allocate_sets(sDrawUboPoolV3a, sDrawUboLayoutV3a,
                           1, &sDrawUboSetV3a,
                           "set=2 per-draw"))
        {
            return false;
        }
        if (!allocate_sets(sAssetUboPoolV3a, sAssetUboLayoutV3a,
                           FRAMES_IN_FLIGHT, sAssetUboSetV3a,
                           "set=3 per-asset+per-skin"))
        {
            return false;
        }

        LL_INFOS("Vulkan") << "V3a 13 descriptor sets allocated"
                           << " (set=0 × " << FRAMES_IN_FLIGHT
                           << " / set=1a × " << FRAMES_IN_FLIGHT
                           << " / set=1b × " << FRAMES_IN_FLIGHT
                           << " / set=2 × 1"
                           << " / set=3 × " << FRAMES_IN_FLIGHT
                           << ")" << LL_ENDL;
        return true;
    }
    // </AYAstorm r41 PC-7δ>
    // </AYAstorm r41 PC-7α>

    // <AYAstorm r41 PC-7ε (a)> sDrawUboSetV3a (set=2 per-draw UBO_DYNAMIC) ↔ ring
    //   buffer VkBuffer 接続 helper。design 07 §7.4 + ambiguity (ε-1) A + (ε-4) A
    //   (= initVulkan 内 1 度のみ決定論的 timing) 採用 (AYA 確認 2026-06-05)。
    //
    //   PC-7δ 完了時点で sDrawUboSetV3a は allocate 済 (createV3aDescriptorSets
    //   line 2594-2596) だが、ring buffer の VkBuffer を vkUpdateDescriptorSets で
    //   write していない状態 = bindV3aStatic 時に空 descriptor set を参照 (Vulkan
    //   validation layer enable 時 VUID 発火)。本 helper で 4 binding × UNIFORM_BUFFER_DYNAMIC
    //   全件 ring buffer 同一 VkBuffer + offset=0 + range=256 placeholder write、
    //   per-draw 実 offset は bindV3aStatic 第 3 引数 dynamic_offsets[4] で投入 (= caller 責任、(ε-3) A)。
    //
    //   range=256 placeholder = (ε-2) A 採用 = 4 binding 同 offset / 256 B 同 size、
    //   PC-N 実 GLTF 通電時に binding 別 size + 4 個独立 allocate へ拡張。
    //
    //   ring buffer grow 時の自動 re-update = (ε-6) A 採用で PC-N 持越、本 PC-7ε
    //   は recordPlaceholderPoolDraw 内 alloc.grew 観測時 LL_WARNS_ONCE log のみ
    //   (dummy phase 数 KB/frame ゆえ grow 起きない想定)。
    bool wireDrawUboSetV3aToRingBuffer()
    {
        if (sDevice == VK_NULL_HANDLE ||
            !sDrawUboRingBufferMgr ||
            sDrawUboSetV3a == VK_NULL_HANDLE)
        {
            LL_WARNS_ONCE("Vulkan") << "wireDrawUboSetV3aToRingBuffer (PC-7ε): prerequisite missing"
                                    << " (sDevice=" << (sDevice != VK_NULL_HANDLE)
                                    << " sDrawUboRingBufferMgr=" << (bool)sDrawUboRingBufferMgr
                                    << " sDrawUboSetV3a=" << (sDrawUboSetV3a != VK_NULL_HANDLE)
                                    << ")" << LL_ENDL;
            return false;
        }

        const LLUboRingBuffer::BufferHandle handle = sDrawUboRingBufferMgr->getBuffer();
        if (handle == 0)
        {
            LL_WARNS_ONCE("Vulkan") << "wireDrawUboSetV3aToRingBuffer (PC-7ε): ring buffer handle == 0"
                                    << LL_ENDL;
            return false;
        }
        auto it = sDrawUboRingBufferRecords.find(handle);
        if (it == sDrawUboRingBufferRecords.end() || it->second.buffer == VK_NULL_HANDLE)
        {
            LL_WARNS_ONCE("Vulkan") << "wireDrawUboSetV3aToRingBuffer (PC-7ε): VkBuffer lookup miss"
                                    << LL_ENDL;
            return false;
        }

        VkDescriptorBufferInfo buffer_infos[V3A_DRAW_SET_BINDINGS] = {};
        VkWriteDescriptorSet   writes[V3A_DRAW_SET_BINDINGS]       = {};
        for (U32 i = 0; i < V3A_DRAW_SET_BINDINGS; ++i)
        {
            buffer_infos[i].buffer = it->second.buffer;
            buffer_infos[i].offset = 0;
            buffer_infos[i].range  = 256;

            writes[i].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet          = sDrawUboSetV3a;
            writes[i].dstBinding      = i;
            writes[i].dstArrayElement = 0;
            writes[i].descriptorCount = 1;
            writes[i].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            writes[i].pBufferInfo     = &buffer_infos[i];
        }
        vkUpdateDescriptorSets(sDevice, V3A_DRAW_SET_BINDINGS, writes, 0, nullptr);

        LL_INFOS("Vulkan") << "PC-7ε: sDrawUboSetV3a wired to ring buffer VkBuffer ("
                           << V3A_DRAW_SET_BINDINGS
                           << " binding × UNIFORM_BUFFER_DYNAMIC, offset=0, range=256 placeholder)"
                           << LL_ENDL;
        return true;
    }
    // </AYAstorm r41 PC-7ε (a)>

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

        // <AYAstorm r41 PC-7δ (i)> sSkySmokeLayout を sAYAStandardLayout (V3a 5-set
        //   + push constant 64 B) で alias 共用 = sSkySmokePipeline は sAYAStandardLayout
        //   経路で bind/draw、recordPlaceholderPoolDraw 内の vkCmdBindDescriptorSets を
        //   bindV3aStatic 経由 set=0/1a/1b/2 4-set bind に migrate。sky smoke SPIR-V は
        //   set=N binding 未参照ゆえ shader 改変不要 (= H8 MUSEUBO-A 整合 = placeholder
        //   offscreen FBO 経路で実 OpenGL 描画への影響ゼロ)。
        // 旧 layout (set=0 PerFrame + set=1 PerMaterial + push 64 B) は廃止、shutdownVulkan
        //   teardown は alias 検出 (= != sAYAStandardLayout) で double-destroy 回避。
        if (sAYAStandardLayout == VK_NULL_HANDLE)
        {
            LL_WARNS("Vulkan") << "Sky smoke pipeline: sAYAStandardLayout not initialized" << LL_ENDL;
            return false;
        }
        sSkySmokeLayout = sAYAStandardLayout;

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
        if (sSkySmokeVertModule == VK_NULL_HANDLE || sSkySmokeFragModule == VK_NULL_HANDLE)
        {
            LL_WARNS("Vulkan") << "createAvatarBonePipeline: sky smoke shader modules not ready" << LL_ENDL;
            return false;
        }

        // <AYAstorm r41 PC-7δ (j)> sAvatarBoneLayout を sAYAStandardLayout で alias 共用
        // (H10-A: push descriptor 経路 disable + V3a 5-set layout に統一)。
        // avatar bone storage buffer 経路は PC-N 実 GLTF avatar Vulkan draw 通電時に再配線。
        if (sAYAStandardLayout == VK_NULL_HANDLE)
        {
            LL_WARNS("Vulkan") << "createAvatarBonePipeline: sAYAStandardLayout not initialized" << LL_ENDL;
            return false;
        }
        sAvatarBoneLayout = sAYAStandardLayout;

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

    // <AYAstorm r41 PC-7α> V3a 5-set unified descriptor scaffolding
    // = design 06c §2 + 07 §3.2 V1' + §6.3 + §9.1 整合
    // <AYAstorm r41 PC-7δ (c)> createV3aDescriptorSets() を chain に追加 = 13 set
    //   eager allocate (set=0 × 3 + set=1a × 3 + set=1b × 3 + set=2 × 1 + set=3 × 3)
    if (!createV3aDescriptorSetLayouts() ||
        !createV3aDescriptorPools()      ||
        !createAYAStandardPipelineLayout() ||
        !createV3aDescriptorSets())
    {
        shutdownVulkan();
        return false;
    }
    // </AYAstorm r41 PC-7δ (c)>
    // <AYAstorm r41 PC-7ε (b)> sDrawUboSetV3a ↔ ring buffer VkBuffer 接続を
    //   initVulkan 内 1 度のみ実行 (= (ε-4) A 採用 = 決定論的 timing)。
    //   失敗時は LL_WARNS_ONCE で診断 log のみ + initVulkan 続行 (= bind 経路は
    //   mUseUBO=false default で到達しないため fatal でない、MUSEUBO-A 整合)。
    if (!wireDrawUboSetV3aToRingBuffer())
    {
        LL_WARNS_ONCE("Vulkan") << "wireDrawUboSetV3aToRingBuffer failed (PC-7ε); "
                                   "Vulkan placeholder draw でも bindV3aStatic は空 descriptor set 参照になる "
                                   "(mUseUBO=false default では到達しないため continue)"
                                << LL_ENDL;
    }
    // </AYAstorm r41 PC-7ε (b)>
    // </AYAstorm r41 PC-7α>

    // <AYAstorm r41 PC-7γ-1> per-frame UBO physical instance 先回り allocate。
    // = design 06b §2.1 + design 07 §8.2 + AYA (W6-A) 確認 2026-06-05 整合:
    //   ubo_metadata.inl g_block_metadata[] を walk して cadence_tag=PER_FRAME
    //   (= 0) の block を全件 try_emplace + allocateUboInstanceBuffers (block_size
    //   個別、FRAMES_IN_FLIGHT=3 triple-buffer)。本 sub 時点 (2026-06-05) で
    //   PER_FRAME block は 3 件 (FrameAtmosphere_Lighting / FrameLights /
    //   FrameViewProj = 3 block × 3 frame = 9 buffer)。
    //
    // failure 時は allocateUboInstanceBuffers 内で確保済 frame の vmaDestroyBuffer
    // 巻き戻し済 (= 部分 allocate state 残さない)、ここでは shutdownVulkan で
    // 全体 teardown (= sFrameUboInstances 内 entry は size==0 sentinel ゆえ
    // destroy 側 no-op safe)。
    //
    // MUSEUBO-A 整合: 本 allocate は Vulkan init 層単独動作、mUseUBO runtime gate
    // 不参照 (= scaffolding object 化、bind 経路は PC-7δ scope)、mUseUBO=false
    // default で forwardToUboUpload 経由 write は発生せず buffer は idle。
    // GATE-B 整合: #ifdef LL_VULKAN_GLSL 新規追加 0、Vulkan init 層単独。
    // </AYAstorm r41 PC-7γ-1>
    for (U32 i = 0; i < ubo::g_block_count; ++i)
    {
        const ubo::BlockMetadata& meta = ubo::g_block_metadata[i];
        if (meta.cadence_tag != 0u /*PER_FRAME*/) continue;
        UboInstance& ubo_inst = sFrameUboInstances[meta.block_hash];
        if (ubo_inst.size != 0)
        {
            continue; // 既 allocate (= 二重 init safe)
        }
        if (!allocateUboInstanceBuffers(ubo_inst, meta.block_size, meta.block_name))
        {
            LL_WARNS("Vulkan") << "PC-7γ-1: sFrameUboInstances allocate failed for "
                               << meta.block_name << " (block_size=" << meta.block_size << ")" << LL_ENDL;
            sFrameUboInstances.erase(meta.block_hash);
            shutdownVulkan();
            return false;
        }
    }

    // <AYAstorm r41 PC-7δ (d)> PER_FRAME 3 block の set=0 descriptor set
    // (sFrameUboSetV3a[3]) を vkUpdateDescriptorSets で初期化 = binding 0/1/2 を
    // 各 frame index 別の vk_buffer[frame_idx] に bind。register-once / bind-many
    // 規律 (= H3-A 整合)、本 update 後 buffer 内容を memcpy しても descriptor
    // 更新は不要 (= persistent map 経由)。
    {
        std::vector<VkDescriptorBufferInfo> buffer_infos;
        std::vector<VkWriteDescriptorSet>   writes;
        buffer_infos.reserve(FRAMES_IN_FLIGHT * V3A_FRAME_SET_BINDINGS);
        writes.reserve(FRAMES_IN_FLIGHT * V3A_FRAME_SET_BINDINGS);
        for (U32 i = 0; i < ubo::g_block_count; ++i)
        {
            const ubo::BlockMetadata& meta = ubo::g_block_metadata[i];
            if (meta.cadence_tag != 0u /*PER_FRAME*/) continue;
            auto it = sFrameUboInstances.find(meta.block_hash);
            if (it == sFrameUboInstances.end()) continue;
            UboInstance& ubo_inst = it->second;
            for (U32 f = 0; f < FRAMES_IN_FLIGHT; ++f)
            {
                if (ubo_inst.vk_buffer[f] == VK_NULL_HANDLE) continue;
                buffer_infos.push_back({ ubo_inst.vk_buffer[f], 0, ubo_inst.size });
                VkWriteDescriptorSet w = {};
                w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                w.dstSet          = sFrameUboSetV3a[f];
                w.dstBinding      = meta.binding;
                w.dstArrayElement = 0;
                w.descriptorCount = 1;
                w.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                w.pBufferInfo     = &buffer_infos.back();
                writes.push_back(w);
            }
        }
        if (!writes.empty())
        {
            vkUpdateDescriptorSets(sDevice, static_cast<U32>(writes.size()), writes.data(), 0, nullptr);
            LL_INFOS("Vulkan") << "PC-7δ (d) PER_FRAME descriptor set updated ("
                               << writes.size() << " write entries)" << LL_ENDL;
        }
    }
    // </AYAstorm r41 PC-7δ (d)>

    // <AYAstorm r41 PC-7δ (m)> SINGLETON cadence allocate ループ =
    // sSingletonUboInstances に cadence_tag=5 (SINGLETON) block 全件 allocate
    // (= 2026-06-05 時点 1 件 Global_ReflectionProbes、set=0 binding=3)、
    // FRAMES_IN_FLIGHT=3 triple-buffer (= PER_FRAME と同形ゆえ frame-rotate write
    // race 回避明示)。allocate 完了後 vkUpdateDescriptorSets で set=0 binding=3 を
    // 各 frame 別 vk_buffer[f] に bind。
    for (U32 i = 0; i < ubo::g_block_count; ++i)
    {
        const ubo::BlockMetadata& meta = ubo::g_block_metadata[i];
        if (meta.cadence_tag != 5u /*SINGLETON*/) continue;
        UboInstance& ubo_inst = sSingletonUboInstances[meta.block_hash];
        if (ubo_inst.size != 0)
        {
            continue;
        }
        if (!allocateUboInstanceBuffers(ubo_inst, meta.block_size, meta.block_name))
        {
            LL_WARNS("Vulkan") << "PC-7δ (m): sSingletonUboInstances allocate failed for "
                               << meta.block_name << " (block_size=" << meta.block_size << ")" << LL_ENDL;
            sSingletonUboInstances.erase(meta.block_hash);
            shutdownVulkan();
            return false;
        }
    }
    {
        std::vector<VkDescriptorBufferInfo> buffer_infos;
        std::vector<VkWriteDescriptorSet>   writes;
        for (U32 i = 0; i < ubo::g_block_count; ++i)
        {
            const ubo::BlockMetadata& meta = ubo::g_block_metadata[i];
            if (meta.cadence_tag != 5u /*SINGLETON*/) continue;
            auto it = sSingletonUboInstances.find(meta.block_hash);
            if (it == sSingletonUboInstances.end()) continue;
            UboInstance& ubo_inst = it->second;
            buffer_infos.reserve(buffer_infos.size() + FRAMES_IN_FLIGHT);
            writes.reserve(writes.size() + FRAMES_IN_FLIGHT);
            for (U32 f = 0; f < FRAMES_IN_FLIGHT; ++f)
            {
                if (ubo_inst.vk_buffer[f] == VK_NULL_HANDLE) continue;
                buffer_infos.push_back({ ubo_inst.vk_buffer[f], 0, ubo_inst.size });
                VkWriteDescriptorSet w = {};
                w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                w.dstSet          = sFrameUboSetV3a[f];
                w.dstBinding      = meta.binding;
                w.dstArrayElement = 0;
                w.descriptorCount = 1;
                w.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                w.pBufferInfo     = &buffer_infos.back();
                writes.push_back(w);
            }
        }
        if (!writes.empty())
        {
            vkUpdateDescriptorSets(sDevice, static_cast<U32>(writes.size()), writes.data(), 0, nullptr);
            LL_INFOS("Vulkan") << "PC-7δ (m) SINGLETON descriptor set updated ("
                               << writes.size() << " write entries)" << LL_ENDL;
        }
    }
    // </AYAstorm r41 PC-7δ (m)>

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
        // <AYAstorm r41 PC-7δ (j)> sAvatarBoneLayout は sAYAStandardLayout alias 共用ゆえ
        //   alias 検出で double-destroy 回避 (= sAYAStandardLayout は別箇所で destroy)。
        if (sAvatarBoneLayout != VK_NULL_HANDLE && sAvatarBoneLayout != sAYAStandardLayout)
        {
            vkDestroyPipelineLayout(sDevice, sAvatarBoneLayout, nullptr);
        }
        sAvatarBoneLayout = VK_NULL_HANDLE;
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
        // <AYAstorm r41 PC-7δ (i)> sSkySmokeLayout は sAYAStandardLayout alias 共用ゆえ
        //   alias 検出で double-destroy 回避 (= sAYAStandardLayout は別箇所で destroy)。
        if (sSkySmokeLayout != VK_NULL_HANDLE && sSkySmokeLayout != sAYAStandardLayout)
        {
            vkDestroyPipelineLayout(sDevice, sSkySmokeLayout, nullptr);
        }
        sSkySmokeLayout = VK_NULL_HANDLE;
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

        // <AYAstorm r41 PC-7α> V3a 5-set unified scaffolding teardown
        // = pipeline layout → 4 pool → 5 layout の reverse 順 destroy。
        //   pool 経由 allocate された descriptor set は pool destroy で自動 free
        //   (= 本 PC-7α では allocate していないが PC-7δ 配線後も同形で問題なし)。
        // <AYAstorm r41 PC-7δ (f)> 13 set handle は pool destroy で implicit free、
        //   defensive nulling で再 init safe (= PC-7δ (b) createV3aDescriptorSets
        //   が再呼出された case で前回 handle 残りによる UAF を防止)。
        for (U32 f = 0; f < FRAMES_IN_FLIGHT; ++f)
        {
            sFrameUboSetV3a[f] = VK_NULL_HANDLE;
            sProgramUboSetA[f] = VK_NULL_HANDLE;
            sProgramUboSetB[f] = VK_NULL_HANDLE;
            sAssetUboSetV3a[f] = VK_NULL_HANDLE;
        }
        sDrawUboSetV3a = VK_NULL_HANDLE;
        // </AYAstorm r41 PC-7δ (f)>
        if (sAYAStandardLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(sDevice, sAYAStandardLayout, nullptr);
            sAYAStandardLayout = VK_NULL_HANDLE;
        }
        if (sAssetUboPoolV3a != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(sDevice, sAssetUboPoolV3a, nullptr);
            sAssetUboPoolV3a = VK_NULL_HANDLE;
        }
        if (sDrawUboPoolV3a != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(sDevice, sDrawUboPoolV3a, nullptr);
            sDrawUboPoolV3a = VK_NULL_HANDLE;
        }
        if (sProgramUboPoolV3a != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(sDevice, sProgramUboPoolV3a, nullptr);
            sProgramUboPoolV3a = VK_NULL_HANDLE;
        }
        if (sFrameUboPoolV3a != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(sDevice, sFrameUboPoolV3a, nullptr);
            sFrameUboPoolV3a = VK_NULL_HANDLE;
        }
        if (sAssetUboLayoutV3a != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(sDevice, sAssetUboLayoutV3a, nullptr);
            sAssetUboLayoutV3a = VK_NULL_HANDLE;
        }
        if (sDrawUboLayoutV3a != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(sDevice, sDrawUboLayoutV3a, nullptr);
            sDrawUboLayoutV3a = VK_NULL_HANDLE;
        }
        if (sProgramUboLayoutB != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(sDevice, sProgramUboLayoutB, nullptr);
            sProgramUboLayoutB = VK_NULL_HANDLE;
        }
        if (sProgramUboLayoutA != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(sDevice, sProgramUboLayoutA, nullptr);
            sProgramUboLayoutA = VK_NULL_HANDLE;
        }
        if (sFrameUboLayoutV3a != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(sDevice, sFrameUboLayoutV3a, nullptr);
            sFrameUboLayoutV3a = VK_NULL_HANDLE;
        }
        // </AYAstorm r41 PC-7α>
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

        // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6ε-2 +
        // <AYAstorm r41 PC-7β>:
        // per-program / per-asset / per-skin dirty map teardown。
        //   PC-6ε-2 = UboInstance::dirty std::atomic<bool> 単独 (外部 resource なし)
        //   PC-7β  = UboInstance::vk_buffer[FRAMES_IN_FLIGHT] / allocation[] /
        //             mapped_ptr[] / size を triple-buffer 一括確保 (= AYA (Y1-A)
        //             (Y3-A)(Y4-A) 確認、本 PC-7β は helper skeleton + teardown
        //             のみ wired = call site は PC-7γ で配線)。
        // entry 走査で destroyUboInstanceBuffers を呼出し vmaDestroyBuffer × 3 を
        // sAllocator 生存中に発火 (= init reverse 順 = sAssetUboPoolMgr / sAllocator
        // destroy より前)、その後 map.clear()。本 PC-7β scope では allocate call
        // site 未配線で entry は空のまま (= ubo.size==0 sentinel で no-op safe)
        // だが、PC-7γ 通電後の forward-safe な teardown 形を本 sub で確立する。
        // </AYAstorm r41 PC-7β>
        for (auto& kv : sProgramUboDirty) { destroyUboInstanceBuffers(kv.second); }
        for (auto& kv : sAssetUboDirty)   { destroyUboInstanceBuffers(kv.second); }
        for (auto& kv : sSkinUboDirty)    { destroyUboInstanceBuffers(kv.second); }
        sProgramUboDirty.clear();
        sAssetUboDirty.clear();
        sSkinUboDirty.clear();

        // <AYAstorm r41 PC-7γ-1> sFrameUboInstances teardown = initVulkan の対称
        // destroy。sAllocator 生存中に発火 (= sProgramUboDirty 等 PC-7β 既存 dirty
        // map teardown と同列、init reverse 順)。entry は initVulkan で先回り
        // allocate 済 (= 3 PER_FRAME block)、size!=0 ゆえ destroy 側で 3 buffer ×
        // FRAMES_IN_FLIGHT=3 = 9 vmaDestroyBuffer 発火。
        // </AYAstorm r41 PC-7γ-1>
        for (auto& kv : sFrameUboInstances) { destroyUboInstanceBuffers(kv.second); }
        sFrameUboInstances.clear();

        // <AYAstorm r41 PC-7δ (n)> sSingletonUboInstances teardown = initVulkan の対称
        // destroy。sAllocator 生存中に発火、entry は PC-7δ (m) で SINGLETON 全件
        // allocate 済、size!=0 ゆえ destroy 側で N buffer × FRAMES_IN_FLIGHT=3 発火。
        for (auto& kv : sSingletonUboInstances) { destroyUboInstanceBuffers(kv.second); }
        sSingletonUboInstances.clear();
        // </AYAstorm r41 PC-7δ (n)>

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

    // <AYAstorm r41 PC-N-4 (c)> ring buffer grow 自動 re-wire hook。
    //   writeDrawUbo 内 alloc.grew 観測時に sDrawUboRingBufferGrewThisFrame set、
    //   本 hook で frame 末尾 (= vkEndCommandBuffer 後 sInFrame=false 直後) に
    //   wireDrawUboSetV3aToRingBuffer() 再呼出 = sDrawUboSetV3a を新 VkBuffer に再 wire。
    //   AYA literal「推奨案採用 OK」確認 2026-06-05、ambiguity (N4-2) D + (N4-4) A +
    //   (N4-6) A 採用:
    //     - (N4-2) D timing = endFrame 末尾 (= command buffer recording 終了状態
    //       確定 + flushDrawUbos 全 site 終了後集約)
    //     - (N4-4) A helper = 既存 wireDrawUboSetV3aToRingBuffer() 再呼出 (= PC-7ε
    //       helper 再利用、4 binding × UNIFORM_BUFFER_DYNAMIC + offset=0 + range=256
    //       同一 binding 構造を grow 後再適用)
    //     - (N4-6) A reset = exchange(false) で atomic に read+reset (= re-wire 成功
    //       失敗問わず reset、失敗は LL_WARNS_ONCE で重複抑制)
    //   (N4-7) A 同 frame 内 stale 描画 1 frame 許容 (= grow は initial 4 MB → 8 MB
    //   + 8 MB → 16 MB の起動初期数 frame のみ発火想定、design 07 §7.5)。
    if (sDrawUboRingBufferGrewThisFrame.exchange(false, std::memory_order_acq_rel))
    {
        static std::atomic<bool> s_first_rewire{true};
        if (s_first_rewire.exchange(false, std::memory_order_acq_rel))
        {
            LL_INFOS("Vulkan") << "PC-N-4 (c) endFrame: ring buffer grow detected, "
                                  "re-wiring sDrawUboSetV3a to new VkBuffer (first fire)"
                               << LL_ENDL;
        }
        wireDrawUboSetV3aToRingBuffer();
    }
    // </AYAstorm r41 PC-N-4 (c)>

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

// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6ε-2:
// per-program cadence flush = entry gate (mUseUBO 明示) + dirty map key 化。
// shader 引数を key として sProgramUboDirty を find、dirty.exchange(false) で
// 1 度だけ flush 実行 (= upload dedup、design 06b §3.2 stage 3)。
//
// dirty=true 経路 = forwardToUboUpload (= 06a §5.4 / 06b §5.2、PC-7 で完成)。
// 本 PC-6ε-2 では map insert path 無し → 空 map → 構造的 no-op (= mUseUBO=true
// でも dirty 立たないため flush 走らず、PC-7 forwardToUboUpload 完成時に有効化)。
//
// entry gate `if (!shader || !shader->mUseUBO) return;` は design 06b §5.3 で
// 確定 = shader 個別 mUseUBO 直接参照 (= LLGLSLShader::mUseUBO bool member、
// 06a §3.2)。shader=nullptr 防御は PC-6δ 既存形 (= (void)shader cast 廃止に伴う
// 安全側追加)。
void flushProgramUbos(LLGLSLShader* shader)
{
    if (!shader || !shader->mUseUBO)
    {
        return;
    }
    // <AYAstorm r41 PC-7γ-1> sProgramUboDirty key 化 (= shader + block_hash 組) に
    // 伴う walk 形変更 = 同一 shader が複数 block_hash を持つため、entry を
    // shader pointer 一致で走査 + dirty exchange (= 1 shader N block 個別 dedup)。
    // PC-7β 既存形 (= shader 単独 key find) を PC-7γ-1 で key 拡張 (W1-A 整合)。
    // 本 PC-7γ-1 では dirty=true 経路 (= writeProgramUbo 経由) で entry が生まれて
    // も、GPU 側 bind は PC-7δ scope (= W3-B「memcpy までで stop」) ゆえ flush
    // 時 dirty exchange + flushDummyUboWrite 既存形維持 (= GPU upload は別 sub)。
    // </AYAstorm r41 PC-7γ-1>
    bool any_dirty = false;
    for (auto& kv : sProgramUboDirty)
    {
        if (kv.first.first != shader) continue;
        if (kv.second.dirty.exchange(false, std::memory_order_acq_rel))
        {
            any_dirty = true;
        }
    }
    if (!any_dirty)
    {
        return;
    }
    flushDummyUboWrite("flushProgramUbos");
}

void flushDrawUbos()
{
    // <AYAstorm r41 PC-N-1 (d)> flushDrawUbos 役割整理 (= AYA literal「OK」確認
    //   2026-06-05、ambiguity (N1-6) B first-fire log 維持 + PC-N-4 hook 用
    //   placeholder 採用)。
    //
    //   PC-N-1 段階で per-draw write 経路は forwardToUboUpload PER_DRAW case 内
    //   writeDrawUbo (= setter 内 immediate allocate、(N1-2) A) で完結する。flush
    //   側は no-op 等価 (= ring buffer per-allocate per-frame chunk rotate 自体が
    //   hazard 回避 + clean state 維持、(N1-4) A no dirty)。
    //
    //   旧 PC-6δ の flushDummyUboWrite("flushDrawUbos") (= 256 B dummy allocate +
    //   memset 0 + first-fire marker) は撤去 (= recordPlaceholderPoolDraw 経由で
    //   writeDrawUbo 通電に置換済、二重 allocate 不要)。first-fire LL_INFOS は
    //   経路通電確認用に局所保持。
    //
    //   <AYAstorm r41 PC-N-4 (d)> ring buffer grow 自動 re-wire は PC-N-4 で
    //   endFrame() 末尾 hook 経由実装済 (= frame 末尾集約、AYA literal「推奨案
    //   採用 OK」確認 2026-06-05、ambiguity (N4-2) D timing 採用)。
    //   flushDrawUbos 経由 hook は不要 (= per-pool 14 site redundant 回避、
    //   vkUpdateDescriptorSets は command buffer recording 終了状態 = endFrame
    //   末尾で発火が VUID 整合)。design 07 §7 grow safety net 整備済。
    //   </AYAstorm r41 PC-N-4 (d)>
    if (!sDrawUboRingBufferMgr)
    {
        return;
    }
    static std::atomic<bool> s_first_fire{true};
    if (s_first_fire.exchange(false, std::memory_order_acq_rel))
    {
        LL_INFOS("Vulkan") << "PC-N-1 (d) flushDrawUbos first fire (per-draw write は "
                              "setter 内 writeDrawUbo immediate allocate ゆえ flush 側は "
                              "no-op 等価、PC-N-4 grow re-wire は endFrame() 末尾 hook で実装済)"
                           << LL_ENDL;
    }
    // </AYAstorm r41 PC-N-1 (d)>
}

// r41 PC-6ε-2: per-asset cadence flush = 構造的 gate + dirty map key 化。
// PC-7γ-2 refactor: sAssetUboDirty key を UboAssetKey (= <Asset*, block_hash>) に
// 拡張 (= AYA (D3-A) 確認 2026-06-05、UboInstanceKey 対称構造)、1 asset N block
// 個別 dedup 対応。asset 引数で walk + kv.first.first == asset match check
// (= PC-7γ-1 flushProgramUbos 同形)。
//
// 注: 現 codegen で PER_ASSET cadence_tag entry 0 件ゆえ sAssetUboDirty 空のまま、
// 本 walk は no-op (= defensive 配線 only)。PC-7γ-3 (= GLTF host write 置換 +
// lifecycle hook + codegen Asset_*/Skin_* block 追加 or synthetic ID scheme) で
// 実走経路通電予定。
//
// asset 引数文脈 mUseUBO 不在 (= shader 引数無し) のため entry gate 不可、
// 代わりに setter 側 mUseUBO 分岐で forwardToUboUpload 不呼出 → sAssetUboDirty
// 空のまま → flush で no-op (= 構造的 gate)。
// asset=nullptr 防御は安全側追加 (= 06b §2.4 でも asset 単位 owner table 引き)。
void flushAssetUbos(LL::GLTF::Asset* asset)
{
    if (!asset)
    {
        return;
    }
    bool any_dirty = false;
    for (auto& kv : sAssetUboDirty)
    {
        if (kv.first.first != asset) continue;
        if (kv.second.dirty.exchange(false, std::memory_order_acq_rel))
        {
            any_dirty = true;
        }
    }
    if (!any_dirty)
    {
        return;
    }
    flushDummyUboWrite("flushAssetUbos");
}

// r41 PC-6ε-2: per-skin cadence flush (= per-asset 同形、構造的 gate + key 化)。
// PC-7γ-2 refactor: sSkinUboDirty key を UboSkinKey (= <Skin*, block_hash>) に
// 拡張 (= AYA (D3-A) 確認 2026-06-05)、walk + match check (= flushAssetUbos 同形)。
void flushSkinUbos(LL::GLTF::Skin* skin)
{
    if (!skin)
    {
        return;
    }
    bool any_dirty = false;
    for (auto& kv : sSkinUboDirty)
    {
        if (kv.first.first != skin) continue;
        if (kv.second.dirty.exchange(false, std::memory_order_acq_rel))
        {
            any_dirty = true;
        }
    }
    if (!any_dirty)
    {
        return;
    }
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
// <AYAstorm r41 PC-7δ (o)> flushSingletonUbos 本格化 = sSingletonUboInstances
// walk + dirty exchange、entry 単位 dedup (= flushProgramUbos 対称形)。dirty=true
// 経路は writeSingletonUbo 経由 (= forwardToUboUpload SINGLETON case 本格化、(p))、
// flush 側は entry 走査 + dirty.exchange(false, acq_rel) で 1 度だけ flush。
// PER_FRAME と異なり beginFrame() 不要 (= process 跨ぎ持続、frame index advance
// は flushFrameUbos が一手担当)。
void flushSingletonUbos()
{
    bool any_dirty = false;
    for (auto& kv : sSingletonUboInstances)
    {
        if (kv.second.dirty.exchange(false, std::memory_order_acq_rel))
        {
            any_dirty = true;
        }
    }
    if (!any_dirty)
    {
        return;
    }
    flushDummyUboWrite("flushSingletonUbos");
}

// ------------------------------------------------------------------
// <AYAstorm r41 PC-7γ-1> per-program register / unregister hook +
// per-frame / per-program write bridge helper の 4 entry point。
//
// 設計根拠: design 06b §3.2 二段階 dedup 構造 + §3.2.3 UboInstance dirty bit
// + §5.2 forwardToUboUpload routing の bridge。LLGLSLShader::forwardToUboUpload
// は llglslshader.cpp 側 member 関数で this 参照を持つが、llvkloader.cpp の
// anonymous ns 内 static map (= sFrameUboInstances / sProgramUboDirty) に直接
// access 不能ゆえ、wrapper 経由で TU 隔離維持。
//
// AYA 確認 2026-06-05 records:
//   (W1-A) UboInstanceKey = std::pair<LLGLSLShader*, U32 block_hash>
//   (W4-A) register hook = mapUniforms() / unloadInternal() 対称配線
//   (W6-A) per-frame UBO 配線 = sFrameUboInstances + initVulkan allocate
//   (W7-C) per-program 専念 (per-asset/skin は PC-7γ-2 持越)
//   (W8-A) [[maybe_unused]] 撤去 = register hook で call site 配線完了
//
// MUSEUBO-A 整合: 呼出側 (= mapUniforms()/unloadInternal()) で `if (mUseUBO)`
//   gate 配置、write は forwardToUboUpload entry gate で多重保証 = mUseUBO=false
//   default で既存 OpenGL 描画 100% 維持 (= 本 helper 群は呼出されない)。
// GATE-B 整合: 本 helper 群は Vulkan init 層単独動作、#ifdef LL_VULKAN_GLSL 不参照。
// ------------------------------------------------------------------
bool registerProgramUbo(LLGLSLShader* shader, U32 block_hash, U32 block_size)
{
    if (!shader || block_size == 0)
    {
        return false;
    }
    UboInstanceKey key{ shader, block_hash };
    auto [it, inserted] = sProgramUboDirty.try_emplace(key);
    if (inserted)
    {
        if (!allocateUboInstanceBuffers(it->second, block_size, "PER_PROGRAM"))
        {
            sProgramUboDirty.erase(it);
            return false;
        }
        // <AYAstorm r41 PC-7α' (e)> register-once + bind-many = UboInstance 確保直後に
        //   vkUpdateDescriptorSets で set=1a/1b 経路 descriptor を triple-buffer 一括 update。
        // PC-7α' codegen V1' split 通電済 = meta.subset (= 0:1a / 1:1b) を single source of
        //   truth として参照、binding<40 / binding>=40 の heuristic は撤去。
        //   subset 値は codegen `main.py _derive_subset` 経由で
        //   `(descriptor_set==1 && binding>=V3A_PROGRAM_SET_A_BINDINGS) ? 1 : 0` 自動決定。
        if (sDevice != VK_NULL_HANDLE)
        {
            const ubo::BlockMetadata* meta = nullptr;
            for (U32 i = 0; i < ubo::g_block_count; ++i)
            {
                if (ubo::g_block_metadata[i].block_hash == block_hash)
                {
                    meta = &ubo::g_block_metadata[i];
                    break;
                }
            }
            if (meta)
            {
                U32 src_binding = meta->binding;
                VkDescriptorSet* set_array = nullptr;
                U32 dst_binding = 0;
                if (meta->subset == 0)
                {
                    set_array = sProgramUboSetA;
                    dst_binding = src_binding;
                }
                else if (meta->subset == 1)
                {
                    set_array = sProgramUboSetB;
                    dst_binding = src_binding - V3A_PROGRAM_SET_A_BINDINGS;
                }
                else
                {
                    LL_WARNS_ONCE("Vulkan") << "PC-7α' (e) registerProgramUbo: meta.subset "
                                            << meta->subset
                                            << " out of V3a {0:1a, 1:1b} range, skip vkUpdateDescriptorSets"
                                            << LL_ENDL;
                    set_array = nullptr;
                }
                if (set_array != nullptr)
                {
                    VkDescriptorBufferInfo binfo[FRAMES_IN_FLIGHT];
                    VkWriteDescriptorSet   writes[FRAMES_IN_FLIGHT];
                    U32 write_count = 0;
                    for (U32 f = 0; f < FRAMES_IN_FLIGHT; ++f)
                    {
                        if (it->second.vk_buffer[f] == VK_NULL_HANDLE) continue;
                        if (set_array[f] == VK_NULL_HANDLE) continue;
                        binfo[write_count] = { it->second.vk_buffer[f], 0, it->second.size };
                        VkWriteDescriptorSet& w = writes[write_count];
                        w = {};
                        w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        w.dstSet          = set_array[f];
                        w.dstBinding      = dst_binding;
                        w.dstArrayElement = 0;
                        w.descriptorCount = 1;
                        w.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        w.pBufferInfo     = &binfo[write_count];
                        ++write_count;
                    }
                    if (write_count > 0)
                    {
                        vkUpdateDescriptorSets(sDevice, write_count, writes, 0, nullptr);
                    }
                }
            }
        }
    }
    // 既存 entry の場合 idempotent 成功 (= mapUniforms 再呼出 case safe)
    return true;
}

void unregisterProgramUbo(LLGLSLShader* shader, U32 block_hash)
{
    if (!shader)
    {
        return;
    }
    UboInstanceKey key{ shader, block_hash };
    auto it = sProgramUboDirty.find(key);
    if (it == sProgramUboDirty.end())
    {
        // shader が mUseUBO=false で未 register / 既 unregister 済 safe
        return;
    }
    destroyUboInstanceBuffers(it->second);
    sProgramUboDirty.erase(it);
}

void writeFrameUbo(U32 block_hash, U32 offset, const void* data, size_t size)
{
    if (!data || size == 0)
    {
        return;
    }
    auto it = sFrameUboInstances.find(block_hash);
    if (it == sFrameUboInstances.end())
    {
        LL_WARNS_ONCE("Vulkan") << "PC-7γ-1 writeFrameUbo: block_hash 0x"
                                << std::hex << block_hash << std::dec
                                << " not registered in sFrameUboInstances" << LL_ENDL;
        return;
    }
    UboInstance& ubo_inst = it->second;
    if (ubo_inst.size == 0 || ubo_inst.mapped_ptr[sFrameIndex] == nullptr)
    {
        return; // allocate 失敗 / sentinel state safe
    }
    if (offset + size > ubo_inst.size)
    {
        LL_WARNS_ONCE("Vulkan") << "PC-7γ-1 writeFrameUbo: out of range write (block_hash=0x"
                                << std::hex << block_hash << std::dec
                                << ", offset=" << offset << ", size=" << size
                                << ", capacity=" << ubo_inst.size << ")" << LL_ENDL;
        return;
    }
    std::memcpy(static_cast<U8*>(ubo_inst.mapped_ptr[sFrameIndex]) + offset, data, size);
    ubo_inst.dirty.store(true, std::memory_order_release);
}

// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-7δ (l):
// SINGLETON cadence write entry point。block_hash 単独 key (= shader-agnostic、
// Global_ReflectionProbes 等 process-wide UBO)。writeFrameUbo signature 同形 +
// sSingletonUboInstances target。bringupTestUBO 経由 flushSingletonUbos の対称
// write 経路 (= PC-6ε-1 で既設 flush と組合せて完成)。
void writeSingletonUbo(U32 block_hash, U32 offset, const void* data, size_t size)
{
    if (!data || size == 0)
    {
        return;
    }
    auto it = sSingletonUboInstances.find(block_hash);
    if (it == sSingletonUboInstances.end())
    {
        LL_WARNS_ONCE("Vulkan") << "PC-7δ writeSingletonUbo: block_hash 0x"
                                << std::hex << block_hash << std::dec
                                << " not registered in sSingletonUboInstances" << LL_ENDL;
        return;
    }
    UboInstance& ubo_inst = it->second;
    if (ubo_inst.size == 0 || ubo_inst.mapped_ptr[sFrameIndex] == nullptr)
    {
        return;
    }
    if (offset + size > ubo_inst.size)
    {
        LL_WARNS_ONCE("Vulkan") << "PC-7δ writeSingletonUbo: out of range write (block_hash=0x"
                                << std::hex << block_hash << std::dec
                                << ", offset=" << offset << ", size=" << size
                                << ", capacity=" << ubo_inst.size << ")" << LL_ENDL;
        return;
    }
    std::memcpy(static_cast<U8*>(ubo_inst.mapped_ptr[sFrameIndex]) + offset, data, size);
    ubo_inst.dirty.store(true, std::memory_order_release);
}

void writeProgramUbo(LLGLSLShader* shader, U32 block_hash, U32 offset, const void* data, size_t size)
{
    if (!shader || !data || size == 0)
    {
        return;
    }
    UboInstanceKey key{ shader, block_hash };
    auto it = sProgramUboDirty.find(key);
    if (it == sProgramUboDirty.end())
    {
        LL_WARNS_ONCE("Vulkan") << "PC-7γ-1 writeProgramUbo: shader=" << shader
                                << " block_hash=0x" << std::hex << block_hash << std::dec
                                << " not registered in sProgramUboDirty" << LL_ENDL;
        return;
    }
    UboInstance& ubo_inst = it->second;
    if (ubo_inst.size == 0 || ubo_inst.mapped_ptr[sFrameIndex] == nullptr)
    {
        return;
    }
    if (offset + size > ubo_inst.size)
    {
        LL_WARNS_ONCE("Vulkan") << "PC-7γ-1 writeProgramUbo: out of range write (shader="
                                << shader << ", block_hash=0x" << std::hex << block_hash
                                << std::dec << ", offset=" << offset << ", size=" << size
                                << ", capacity=" << ubo_inst.size << ")" << LL_ENDL;
        return;
    }
    std::memcpy(static_cast<U8*>(ubo_inst.mapped_ptr[sFrameIndex]) + offset, data, size);
    ubo_inst.dirty.store(true, std::memory_order_release);
}
// </AYAstorm r41 PC-7γ-1>

// <AYAstorm r41 PC-N-1 (a)> PER_DRAW cadence write entry point。
//   dynamic offset 経路 ring buffer chunk hand-off (= sDrawUboRingBufferMgr 経由
//   1 allocate per writer call、block_hash → block_size lookup で full block 確保、
//   memcpy 後 alloc.offset を caller へ返却 = bind 時に dynamic_offsets[] へ展開)。
//
//   AYA literal「OK」確認 2026-06-05、ambiguity (N1-1) A + (N1-2) A + (N1-3) A
//   + (N1-4) A + (N1-8) B 採用:
//     - (N1-1) A signature = writeFrameUbo / writeSingletonUbo 同形 +
//       out_dynamic_offset 追加引数
//     - (N1-2) A immediate allocate = setter 内 (= forwardToUboUpload PER_DRAW
//       case 内) で本 helper 呼出 → 即時 ring buffer allocate + memcpy
//     - (N1-3) A key = block_hash 単独 (= draw 内 in-place 上書き許容)
//     - (N1-4) A no dirty (= per-allocate per-frame chunk rotate 自動 hazard 回避)
//     - (N1-8) B = g_block_metadata 線形 walk (= 既存 PC-7γ-1
//       lookup_block_size_by_hash 同パターン)
//
//   grow 観測時は LL_WARNS_ONCE + sDrawUboRingBufferGrewThisFrame.store(true)
//   ((N1-6) B + PC-N-4 (b) 整合、sDrawUboSetV3a 再 wire は endFrame() 末尾 hook
//   経由実装済)。MUSEUBO-A 整合 = sDrawUboRingBufferMgr nullptr early return。
void writeDrawUbo(U32 block_hash, U32 offset, const void* data, size_t size, U32& out_dynamic_offset)
{
    out_dynamic_offset = 0u;
    if (!data || size == 0)
    {
        return;
    }
    if (!sDrawUboRingBufferMgr)
    {
        LL_WARNS_ONCE("Vulkan") << "PC-N-1 writeDrawUbo: sDrawUboRingBufferMgr null (Vulkan 未起動 or pre-init)"
                                << LL_ENDL;
        return;
    }

    // (N1-8) B = g_block_metadata 線形 walk で block_size 解決。block 数 94 ゆえ
    // 線形 walk で問題なし (= 既存 PC-7γ-1 lookup_block_size_by_hash 同形)。
    const ubo::BlockMetadata* meta = nullptr;
    for (U32 i = 0; i < ubo::g_block_count; ++i)
    {
        if (ubo::g_block_metadata[i].block_hash == block_hash)
        {
            meta = &ubo::g_block_metadata[i];
            break;
        }
    }
    if (!meta)
    {
        LL_WARNS_ONCE("Vulkan") << "PC-N-1 writeDrawUbo: block_hash 0x"
                                << std::hex << block_hash << std::dec
                                << " not in g_block_metadata (codegen drift?)" << LL_ENDL;
        return;
    }
    if (offset + size > meta->block_size)
    {
        LL_WARNS_ONCE("Vulkan") << "PC-N-1 writeDrawUbo: out of range (block_hash=0x"
                                << std::hex << block_hash << std::dec
                                << ", offset=" << offset << ", size=" << size
                                << ", block_size=" << meta->block_size << ")" << LL_ENDL;
        return;
    }

    // (N1-2) A = block 全体 size で 1 chunk allocate (= bind 時 dynamic_offsets
    // が chunk 先頭を指す前提、shader は chunk 先頭から block 全体を読む)。
    const LLUboRingBuffer::AllocateResult alloc =
        sDrawUboRingBufferMgr->allocate(meta->block_size);
    if (!alloc.success)
    {
        LL_WARNS_ONCE("Vulkan") << "PC-N-1 writeDrawUbo: ring buffer allocate failed (block_hash=0x"
                                << std::hex << block_hash << std::dec
                                << ", block_size=" << meta->block_size << ")" << LL_ENDL;
        return;
    }
    if (alloc.grew)
    {
        // <AYAstorm r41 PC-N-4 (b)> grow flag set (= endFrame() 末尾 hook で
        //   re-wire 発火)。AYA literal「推奨案採用 OK」確認 2026-06-05、ambiguity
        //   (N4-5) A 採用 = writeDrawUbo 内 1 箇所のみ (= PC-N-1 で
        //   recordPlaceholderPoolDraw + PC-N-2 で recordAvatarPlaceholderDraw の
        //   allocate は両方 writeDrawUbo 経由化済、生 sDrawUboRingBufferMgr->
        //   allocate 呼出は 0 件)。PC-N-4 で実装済 (= endFrame() 末尾 hook、
        //   sDrawUboRingBufferGrewThisFrame flag 経由)。
        LL_WARNS_ONCE("Vulkan") << "PC-N-1 writeDrawUbo: ring buffer grew (block_hash=0x"
                                << std::hex << block_hash << std::dec
                                << "); sDrawUboSetV3a may reference stale VkBuffer "
                                   "(PC-N-4 で実装済 = endFrame() 末尾 hook、"
                                   "sDrawUboRingBufferGrewThisFrame flag 経由)"
                                << LL_ENDL;
        sDrawUboRingBufferGrewThisFrame.store(true, std::memory_order_release);
        // </AYAstorm r41 PC-N-4 (b)>
    }

    auto it = sDrawUboRingBufferRecords.find(alloc.buffer);
    if (it == sDrawUboRingBufferRecords.end() || it->second.mapped == nullptr)
    {
        LL_WARNS_ONCE("Vulkan") << "PC-N-1 writeDrawUbo: side-table mapped pointer lookup failed (buffer=0x"
                                << std::hex << alloc.buffer << std::dec << ")" << LL_ENDL;
        return;
    }

    // (N1-3) A + (N1-4) A = in-place 上書き許容 + no dirty (per-allocate hazard 回避)。
    std::memcpy(static_cast<U8*>(it->second.mapped) + alloc.offset + offset, data, size);

    // (N1-1) A = caller (= bind 経路) へ dynamic offset 返却。
    out_dynamic_offset = alloc.offset;
}
// </AYAstorm r41 PC-N-1 (a)>

// ------------------------------------------------------------------
// <AYAstorm r41 PC-7γ-2> per-asset / per-skin register / unregister hook +
// write bridge helper + sCurrentAsset / sCurrentSkin tracking accessor。
//
// 設計根拠: design 06b §2.4 (per-asset cadence) + §2.5 (per-skin) + §5.2
// (forwardToUboUpload routing PER_ASSET/PER_SKIN case) + §5.4.1 (main thread
// 専有 = atomic 不要、static で thread-safe) の bridge。PC-7γ-1 4 method
// (= registerProgramUbo / unregisterProgramUbo / writeFrameUbo /
// writeProgramUbo) と対称展開。
//
// AYA 確認 2026-06-05 records:
//   (D1-A) PER_ASSET/PER_SKIN write 経路 = forwardToUboUpload defensive +
//          GLTF host write 置換 両方 in scope (本 PC-7γ-2 は前者のみ通電)
//   (D2-A) sCurrentAsset / sCurrentSkin static (main thread 専有)
//   (D3-A) map key 拡張 = UboAssetKey / UboSkinKey (上方 anonymous ns 参照)
//   (D4-A) lifecycle hook = PC-7γ-3 持越し (= D5-rev 整合)
//   (D5-rev) PC-7γ-2 を defensive 配線 only に再定義 + PC-7γ-3 後段新設
//
// 注: 現 codegen で PER_ASSET (=3) / PER_SKIN (=4) cadence_tag entry 0 件
//   (= ubo_metadata.inl 2026-06-05 確認)、Asset_*/Skin_* prefix block 0 件
//   ゆえ本 PC-7γ-2 commit 時点で 6 method の call site 不在。PC-7γ-3 で
//   codegen 追加 + bare OpenGL UBO 置換 + lifecycle hook 配線で呼出開始。
//
// MUSEUBO-A 整合: register/unregister の call site は PC-7γ-3 で GLTF path
//   内配線、write は forwardToUboUpload entry gate + sCurrent* null check で
//   多重保証 = mUseUBO=false default で既存 OpenGL 描画 100% 維持。
// GATE-B 整合: 本 helper 群は Vulkan init 層単独動作、#ifdef LL_VULKAN_GLSL
//   不参照 (= PC-7γ-1 同形)。
// ------------------------------------------------------------------
bool registerAssetUbo(LL::GLTF::Asset* asset, U32 block_hash, U32 block_size)
{
    if (!asset || block_size == 0)
    {
        return false;
    }
    UboAssetKey key{ asset, block_hash };
    auto [it, inserted] = sAssetUboDirty.try_emplace(key);
    if (inserted)
    {
        if (!allocateUboInstanceBuffers(it->second, block_size, "PER_ASSET"))
        {
            sAssetUboDirty.erase(it);
            return false;
        }
        // <AYAstorm r41 PC-7δ (e)> register-once + bind-many = sAssetUboSetV3a (set=3) を
        //   triple-buffer 一括 update。複数 asset が同一 set=3 を共有するため、後続
        //   asset register 時に descriptor は上書きされる (= H4-B 整合、実 swap 発火は
        //   PC-N 実 GLTF Vulkan draw 通電時)。
        if (sDevice != VK_NULL_HANDLE)
        {
            const ubo::BlockMetadata* meta = nullptr;
            for (U32 i = 0; i < ubo::g_block_count; ++i)
            {
                if (ubo::g_block_metadata[i].block_hash == block_hash)
                {
                    meta = &ubo::g_block_metadata[i];
                    break;
                }
            }
            if (meta && meta->binding < V3A_ASSET_SET_BINDINGS)
            {
                VkDescriptorBufferInfo binfo[FRAMES_IN_FLIGHT];
                VkWriteDescriptorSet   writes[FRAMES_IN_FLIGHT];
                U32 write_count = 0;
                for (U32 f = 0; f < FRAMES_IN_FLIGHT; ++f)
                {
                    if (it->second.vk_buffer[f] == VK_NULL_HANDLE) continue;
                    if (sAssetUboSetV3a[f] == VK_NULL_HANDLE) continue;
                    binfo[write_count] = { it->second.vk_buffer[f], 0, it->second.size };
                    VkWriteDescriptorSet& w = writes[write_count];
                    w = {};
                    w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    w.dstSet          = sAssetUboSetV3a[f];
                    w.dstBinding      = meta->binding;
                    w.dstArrayElement = 0;
                    w.descriptorCount = 1;
                    w.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    w.pBufferInfo     = &binfo[write_count];
                    ++write_count;
                }
                if (write_count > 0)
                {
                    vkUpdateDescriptorSets(sDevice, write_count, writes, 0, nullptr);
                }
            }
        }
    }
    // 既存 entry の場合 idempotent 成功
    return true;
}

void unregisterAssetUbo(LL::GLTF::Asset* asset, U32 block_hash)
{
    if (!asset)
    {
        return;
    }
    UboAssetKey key{ asset, block_hash };
    auto it = sAssetUboDirty.find(key);
    if (it == sAssetUboDirty.end())
    {
        return;
    }
    destroyUboInstanceBuffers(it->second);
    sAssetUboDirty.erase(it);
}

void writeAssetUbo(LL::GLTF::Asset* asset, U32 block_hash, U32 offset, const void* data, size_t size)
{
    if (!asset || !data || size == 0)
    {
        return;
    }
    UboAssetKey key{ asset, block_hash };
    auto it = sAssetUboDirty.find(key);
    if (it == sAssetUboDirty.end())
    {
        LL_WARNS_ONCE("Vulkan") << "PC-7γ-2 writeAssetUbo: asset=" << asset
                                << " block_hash=0x" << std::hex << block_hash << std::dec
                                << " not registered in sAssetUboDirty" << LL_ENDL;
        return;
    }
    UboInstance& ubo_inst = it->second;
    if (ubo_inst.size == 0 || ubo_inst.mapped_ptr[sFrameIndex] == nullptr)
    {
        return;
    }
    if (offset + size > ubo_inst.size)
    {
        LL_WARNS_ONCE("Vulkan") << "PC-7γ-2 writeAssetUbo: out of range write (asset="
                                << asset << ", block_hash=0x" << std::hex << block_hash
                                << std::dec << ", offset=" << offset << ", size=" << size
                                << ", capacity=" << ubo_inst.size << ")" << LL_ENDL;
        return;
    }
    std::memcpy(static_cast<U8*>(ubo_inst.mapped_ptr[sFrameIndex]) + offset, data, size);
    ubo_inst.dirty.store(true, std::memory_order_release);
}

bool registerSkinUbo(LL::GLTF::Skin* skin, U32 block_hash, U32 block_size)
{
    if (!skin || block_size == 0)
    {
        return false;
    }
    UboSkinKey key{ skin, block_hash };
    auto [it, inserted] = sSkinUboDirty.try_emplace(key);
    if (inserted)
    {
        if (!allocateUboInstanceBuffers(it->second, block_size, "PER_SKIN"))
        {
            sSkinUboDirty.erase(it);
            return false;
        }
        // <AYAstorm r41 PC-7δ (e)> register-once + bind-many = sAssetUboSetV3a (set=3)
        //   共有、Skin_GLTFJoints は set=3 binding=2 (X2-B sampler 除外整合)。
        if (sDevice != VK_NULL_HANDLE)
        {
            const ubo::BlockMetadata* meta = nullptr;
            for (U32 i = 0; i < ubo::g_block_count; ++i)
            {
                if (ubo::g_block_metadata[i].block_hash == block_hash)
                {
                    meta = &ubo::g_block_metadata[i];
                    break;
                }
            }
            if (meta && meta->binding < V3A_ASSET_SET_BINDINGS)
            {
                VkDescriptorBufferInfo binfo[FRAMES_IN_FLIGHT];
                VkWriteDescriptorSet   writes[FRAMES_IN_FLIGHT];
                U32 write_count = 0;
                for (U32 f = 0; f < FRAMES_IN_FLIGHT; ++f)
                {
                    if (it->second.vk_buffer[f] == VK_NULL_HANDLE) continue;
                    if (sAssetUboSetV3a[f] == VK_NULL_HANDLE) continue;
                    binfo[write_count] = { it->second.vk_buffer[f], 0, it->second.size };
                    VkWriteDescriptorSet& w = writes[write_count];
                    w = {};
                    w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    w.dstSet          = sAssetUboSetV3a[f];
                    w.dstBinding      = meta->binding;
                    w.dstArrayElement = 0;
                    w.descriptorCount = 1;
                    w.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    w.pBufferInfo     = &binfo[write_count];
                    ++write_count;
                }
                if (write_count > 0)
                {
                    vkUpdateDescriptorSets(sDevice, write_count, writes, 0, nullptr);
                }
            }
        }
    }
    return true;
}

void unregisterSkinUbo(LL::GLTF::Skin* skin, U32 block_hash)
{
    if (!skin)
    {
        return;
    }
    UboSkinKey key{ skin, block_hash };
    auto it = sSkinUboDirty.find(key);
    if (it == sSkinUboDirty.end())
    {
        return;
    }
    destroyUboInstanceBuffers(it->second);
    sSkinUboDirty.erase(it);
}

void writeSkinUbo(LL::GLTF::Skin* skin, U32 block_hash, U32 offset, const void* data, size_t size)
{
    if (!skin || !data || size == 0)
    {
        return;
    }
    UboSkinKey key{ skin, block_hash };
    auto it = sSkinUboDirty.find(key);
    if (it == sSkinUboDirty.end())
    {
        LL_WARNS_ONCE("Vulkan") << "PC-7γ-2 writeSkinUbo: skin=" << skin
                                << " block_hash=0x" << std::hex << block_hash << std::dec
                                << " not registered in sSkinUboDirty" << LL_ENDL;
        return;
    }
    UboInstance& ubo_inst = it->second;
    if (ubo_inst.size == 0 || ubo_inst.mapped_ptr[sFrameIndex] == nullptr)
    {
        return;
    }
    if (offset + size > ubo_inst.size)
    {
        LL_WARNS_ONCE("Vulkan") << "PC-7γ-2 writeSkinUbo: out of range write (skin="
                                << skin << ", block_hash=0x" << std::hex << block_hash
                                << std::dec << ", offset=" << offset << ", size=" << size
                                << ", capacity=" << ubo_inst.size << ")" << LL_ENDL;
        return;
    }
    std::memcpy(static_cast<U8*>(ubo_inst.mapped_ptr[sFrameIndex]) + offset, data, size);
    ubo_inst.dirty.store(true, std::memory_order_release);
}

// sCurrentAsset / sCurrentSkin tracking accessor (= forwardToUboUpload PER_ASSET/
// PER_SKIN case が "current owner" を解決する経路、AYA (D2-A) 確認 2026-06-05)。
// gltfscenemanager.cpp の asset/skin draw 直前で setCurrent*、直後 / scope end
// で clearCurrent* 配線。main thread 専有 (= design 06b §5.4.1) ゆえ atomic 不要。
void setCurrentAsset(LL::GLTF::Asset* asset)
{
    sCurrentAsset = asset;
}

void clearCurrentAsset()
{
    sCurrentAsset = nullptr;
}

LL::GLTF::Asset* getCurrentAsset()
{
    return sCurrentAsset;
}

void setCurrentSkin(LL::GLTF::Skin* skin)
{
    sCurrentSkin = skin;
}

void clearCurrentSkin()
{
    sCurrentSkin = nullptr;
}

LL::GLTF::Skin* getCurrentSkin()
{
    return sCurrentSkin;
}
// </AYAstorm r41 PC-7γ-2>

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

    // <AYAstorm r41 PC-N-1 (c)> per-draw ring buffer allocate-chain を
    //   writeDrawUbo helper (= PC-N-1 (a) 新設) 経由 API path に置換
    //   (= AYA literal「OK」確認 2026-06-05、ambiguity (N1-5) B zero write 経由置換
    //   採用、real value 構築は PC-N-2 set=2 復活時 recordAvatarPlaceholderDraw
    //   側で本格化)。
    //
    //   旧 PC-7ε (d) との差分:
    //     - 旧: sDrawUboRingBufferMgr->allocate(256) + 直接 mapped 書込 + memset 0
    //     - 新: writeDrawUbo(PerDrawUBO_LightParams, 0, zero_buf, 256, dyn_off)
    //       で API path 通電 (= 内部 metadata lookup → allocate → memcpy + dynamic
    //       offset 返却)
    //
    //   placeholder phase ゆえ data は zero buffer 維持 ((N1-5) B literal scope =
    //   API 経路通電が本質、placeholder PSO 用 real value 構築は別 sub-step、real
    //   draw 経路 = PC-N-2 set=2 復活時に recordAvatarPlaceholderDraw 側で
    //   bindV3aRigged 経由 set=2 dynamic offset 配線と一括)。
    //
    //   配線対象 block = PerDrawUBO_LightParams (= 0x9ebc071fu, 256 B, set=2,
    //   binding=0) を placeholder 代表として採用 (= PER_DRAW cadence_tag=2 集合
    //   中で最小 size + binding=0 で shader 未参照でも GPU error なし)。
    //
    //   4 binding 同 dynamic offset 構築 = (ε-2) A pattern 継承 (= placeholder
    //   phase は 1 allocate で実装簡略化、PC-N-5 実 GLTF 通電時に 4 独立 allocate へ
    //   拡張、binding=2/3 配置 + 4 binding 再分配は (N1-7) A 採用で別 sub-step 持越)。
    //
    //   MUSEUBO-A guard = sDrawUboRingBufferMgr nullptr で early return (=
    //   bindV3aStatic / vkCmdDraw 不発火 = OpenGL 描画影響ゼロ、writeDrawUbo 内部も
    //   nullptr guard 持つが二重 safety で skip draw)。
    if (!sDrawUboRingBufferMgr)
    {
        return;
    }
    static const U8 zero_buf[256] = {};
    U32 dynamic_offset = 0u;
    LLVKLoader::writeDrawUbo(
        ubo::block_hash::PerDrawUBO_LightParams,
        /*offset=*/0u,
        zero_buf,
        sizeof(zero_buf),
        dynamic_offset);
    const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS] = {
        dynamic_offset, dynamic_offset, dynamic_offset, dynamic_offset,
    };
    bindV3aStatic(cmd_buf, sFrameIndex, dynamic_offsets);
    // </AYAstorm r41 PC-N-1 (c)>

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
                              "writeDrawUbo(PerDrawUBO_LightParams, 256 B zero) API 経路通電 + "
                              "bindV3aStatic set=0/1a/1b/2 + push constant 64 B identity / "
                              "VERTEX_BIT + vkCmdDraw(3,1,0,0); PC-N-1 (c) zero write 通電、"
                              "real value 構築は PC-N-2 持越)"
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
    // <AYAstorm r41 PC-7δ (j)> H10-A 採用: push descriptor 経路 (STORAGE_BUFFER) を本 PC-7δ で disable +
    //   sAvatarBoneLayout は sAYAStandardLayout alias 共用 + bindV3aRigged で set=0/1a/1b + set=3 swap 実走。
    //   placeholder ゆえ視覚 no-op 等価維持。avatar bone storage 経路は PC-N 実 GLTF avatar Vulkan draw 通電時に再配線。
    if (sAvatarBonePipeline == VK_NULL_HANDLE ||
        sAvatarBoneLayout == VK_NULL_HANDLE ||
        sAYAStandardLayout == VK_NULL_HANDLE)
    {
        recordPlaceholderPoolDraw(cmd_buf);
        return;
    }

    if (cmd_buf == VK_NULL_HANDLE)
    {
        return;
    }

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, sAvatarBonePipeline);

    // <AYAstorm r41 PC-N-2 (c)> per-draw ring buffer allocate-chain を writeDrawUbo
    //   helper (= PC-N-1 (a) 新設) 経由 API path に置換 + bindV3aRigged 拡張 signature
    //   呼出で set=2 復活通電 (= AYA literal「Claude 推奨案 OK」確認 2026-06-05、
    //   ambiguity (N2-1) A 1 call 4 set bind + (N2-2) A PerDrawUBO_LightParams 共有
    //   + (N2-3) A 4 個同一 offset + (N2-6) A sDrawUboRingBufferMgr nullptr 時
    //   recordPlaceholderPoolDraw fallback)。
    //
    // <AYAstorm r41 PC-N-2 (d)> set=2 復活 = 本 PC-N-2 で実施 (= bindV3aRigged signature
    //   拡張 + recordAvatarPlaceholderDraw allocate-chain 配線、(N2-1)..(N2-9) AYA literal
    //   「Claude 推奨案 OK」確認 2026-06-05)。H10-A avatar bone storage 再配線 (=
    //   writeAvatarBoneStorage helper 新設 + set=3 経由再 wire) は PC-N-3 持越し、ring
    //   buffer grow 自動 re-wire は PC-N-4 持越し (= PC-N decomposition design-lock commit
    //   cf7b0b99b0、AYA literal「OK」確認 2026-06-05)。
    //
    // placeholder phase ゆえ zero data 維持、real avatar data 構築 (= PerDrawUBO_AvatarSkin
    //   等) は PC-N-5 実 GLTF avatar Vulkan draw 通電持越し。
    //
    // MUSEUBO-A guard = sDrawUboRingBufferMgr nullptr で recordPlaceholderPoolDraw
    //   fallback (= 多重 graceful degrade、視覚 no-op 等価維持)。
    if (!sDrawUboRingBufferMgr)
    {
        recordPlaceholderPoolDraw(cmd_buf);
        return;
    }
    static const U8 zero_buf[256] = {};
    U32 dynamic_offset = 0u;
    LLVKLoader::writeDrawUbo(
        ubo::block_hash::PerDrawUBO_LightParams,
        /*offset=*/0u,
        zero_buf,
        sizeof(zero_buf),
        dynamic_offset);
    const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS] = {
        dynamic_offset, dynamic_offset, dynamic_offset, dynamic_offset,
    };
    bindV3aRigged(cmd_buf, sFrameIndex, dynamic_offsets);
    // </AYAstorm r41 PC-N-2 (c)+(d)>

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
        LL_INFOS("Vulkan") << "Avatar placeholder pool draw fired (PC-N-2 set=2 復活通電済: "
                              "writeDrawUbo(PerDrawUBO_LightParams, zero 256B) → "
                              "dynamic_offsets[4] (= 4 個同一 offset) → "
                              "bindV3aRigged (set=0 Frame V3a + set=1a/1b ProgramUbo + "
                              "set=2 DrawUbo V3a + set=3 AssetUbo, "
                              "push descriptor 経路 disable 維持) + push constant 64 B / "
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
