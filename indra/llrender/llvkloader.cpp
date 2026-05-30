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

#include <vector>
#include <string>
#include <climits>
#include <cstring>
#include <fstream>

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
    };
    DeviceLimits sDeviceLimits;

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
        for (const auto& e : exts)
        {
            if (std::string(e.extensionName) == VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME)
            {
                push_desc_supported = true;
                break;
            }
        }

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

        VkDeviceCreateInfo device_info = {};
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.pNext = &dr_features_enable;  // r41 3.3-C-β-2: chain dynamic rendering feature
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &queue_info;
        device_info.enabledExtensionCount = 0;
        device_info.ppEnabledExtensionNames = nullptr;
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
        // disk persist は別 phase (起動高速化要件発生時) で追加、本段では empty cache で起動。
        VkPipelineCacheCreateInfo info = {};
        info.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        info.initialDataSize = 0;
        info.pInitialData    = nullptr;

        VkResult result = vkCreatePipelineCache(sDevice, &info, nullptr, &sPipelineCache);
        if (result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkCreatePipelineCache failed: " << (S32)result << LL_ENDL;
            return false;
        }

        LL_INFOS("Vulkan") << "VkPipelineCache created" << LL_ENDL;
        return true;
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

        // r41 sub-step 3.3-γ: 二段構え準拠 layout (set=0 = per-frame matrix UBO + push constant = modelview_matrix mat4 / VERTEX_BIT)
        // sub-doc 05 §3.5 / sub-doc 03 §3.1.1。実 descriptor set bind / push constant 投入は δ で初実施、γ は layout signature 統合のみ。
        // placeholder SPIR-V は uniform 未参照のため shader 改変不要 (compile/bind は通る)。
        VkDescriptorSetLayout set_layouts[1]      = { sPerFrameDescriptorSetLayout };
        VkPushConstantRange   push_constants[1]  = {};
        push_constants[0].stageFlags             = VK_SHADER_STAGE_VERTEX_BIT;
        push_constants[0].offset                 = 0;
        push_constants[0].size                   = 64; // mat4 modelview_matrix
        sPlaceholderLayout = createStandardPipelineLayout(set_layouts, 1, push_constants, 1);
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

    // r41 sub-step 3.2 smoke-test (refine 2026-05-29): sky pool 1 draw PSO。
    // GLSL source (offline compiled、glslc -O / Target: SPIR-V 1.0):
    //   sky_smoke.vert: fullscreen triangle、gl_VertexIndex 0/1/2 で
    //                   (-1,-1)/(3,-1)/(-1,3) を生成、vertex input binding 不要
    //   sky_smoke.frag: layout(location=0) out vec4 outColor;
    //                   void main() { outColor = vec4(0.4, 0.6, 0.9, 1.0); }
    // descriptor / push constant 不要、depth test/write OFF、blend OFF、cull NONE。
    // 完遂 marker (sub-doc 03 §3.1 sub-step 3.2): sky pool recordPoolDraws() で
    // vkCmdBindPipeline + vkCmdDraw(3,1,0,0) 投入、validation 0 件。
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

    bool createSkySmokePipeline()
    {
        VkShaderModuleCreateInfo vs_info = {};
        vs_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vs_info.codeSize = sizeof(kSkySmokeVertSpv);
        vs_info.pCode    = kSkySmokeVertSpv;
        if (vkCreateShaderModule(sDevice, &vs_info, nullptr, &sSkySmokeVertModule) != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "Sky smoke vertex shader module create failed" << LL_ENDL;
            return false;
        }

        VkShaderModuleCreateInfo fs_info = {};
        fs_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        fs_info.codeSize = sizeof(kSkySmokeFragSpv);
        fs_info.pCode    = kSkySmokeFragSpv;
        if (vkCreateShaderModule(sDevice, &fs_info, nullptr, &sSkySmokeFragModule) != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "Sky smoke fragment shader module create failed" << LL_ENDL;
            return false;
        }

        // r41 sub-step 3.3-γ: 二段構え準拠 layout (set=0 = per-frame matrix UBO + push constant = modelview_matrix mat4 / VERTEX_BIT)
        // sub-doc 05 §3.5 / sub-doc 03 §3.1.1。実 descriptor set bind / push constant 投入は δ で初実施、γ は layout signature 統合のみ。
        // sky smoke SPIR-V は uniform 未参照のため shader 改変不要 (compile/bind は通る)。
        VkDescriptorSetLayout set_layouts[1]      = { sPerFrameDescriptorSetLayout };
        VkPushConstantRange   push_constants[1]  = {};
        push_constants[0].stageFlags             = VK_SHADER_STAGE_VERTEX_BIT;
        push_constants[0].offset                 = 0;
        push_constants[0].size                   = 64; // mat4 modelview_matrix
        sSkySmokeLayout = createStandardPipelineLayout(set_layouts, 1, push_constants, 1);
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

        LL_INFOS("Vulkan") << "Sky smoke PSO compiled (vert " << sizeof(kSkySmokeVertSpv)
                           << " B / frag " << sizeof(kSkySmokeFragSpv)
                           << " B, sub-doc 03 §3.1 sub-step 3.2 sky pool 1 draw)" << LL_ENDL;
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

    if (!createCommandPool() || !createOffscreenImage() || !createRenderPass() || !createFramebuffer() || !createPipelineCache())
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
        if (sPipelineCache != VK_NULL_HANDLE)
        {
            vkDestroyPipelineCache(sDevice, sPipelineCache, nullptr);
            sPipelineCache = VK_NULL_HANDLE;
        }
        if (sCommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(sDevice, sCommandPool, nullptr);
            sCommandPool = VK_NULL_HANDLE;
            sCommandBuffer = VK_NULL_HANDLE;
        }

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
    }

    sInFrame = true;

    // r41 sub-step 3.3-C-β-2: dynamic rendering helper transit smoke (sub-doc 03 §3.1.2)。
    // no-op early return (all-null args) で in-frame gating + 1 度限りの marker emit を verify。
    // 実 attachment 配線は γ (LLRenderTarget::bindTarget Vulkan 並走) + 領域 7 sub-step 7.5 で本配信。
    // 注: legacy vkCmdBeginRenderPass の内側だが、all-null path は vkCmdBeginRendering を発行せず、
    //     no-op で帰るため legacy render pass と衝突しない。
    beginDynamicRendering(0, 0, nullptr, 0, nullptr);
    endDynamicRendering();

    // r41 sub-step 3.3-B-β-2: SPIR-V exemplar load transit smoke (sub-doc 03 §3.1.3)。
    // 1 度限り (first frame) sky_placeholder{V,F}.spv を読み込み → loadSpirvShaderModule
    // → 即破棄。build chain (glslangValidator → .spv → vkCreateShaderModule) 全段の動作確認。
    // 実 PSO compile への移行は γ で配線、現状は build chain 実証のみ。
    static bool s_spirv_exemplar_smoke_done = false;
    if (!s_spirv_exemplar_smoke_done)
    {
        s_spirv_exemplar_smoke_done = true;

        auto try_load = [](const char* rel_path)
        {
            const std::string path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, rel_path);
            std::ifstream f(path, std::ios::binary | std::ios::ate);
            if (!f)
            {
                LL_WARNS("Vulkan") << "SPIR-V exemplar not found: " << path << LL_ENDL;
                return;
            }
            const std::streamsize size = f.tellg();
            if (size <= 0 || (size % 4) != 0)
            {
                LL_WARNS("Vulkan") << "SPIR-V exemplar bad size: " << path
                                   << " size=" << (S32)size << LL_ENDL;
                return;
            }
            f.seekg(0, std::ios::beg);
            std::vector<U32> spv(static_cast<size_t>(size) / 4);
            if (!f.read(reinterpret_cast<char*>(spv.data()), size))
            {
                LL_WARNS("Vulkan") << "SPIR-V exemplar read failed: " << path << LL_ENDL;
                return;
            }
            VkShaderModule mod = loadSpirvShaderModule(spv.data(), static_cast<size_t>(size));
            if (mod != VK_NULL_HANDLE)
            {
                vkDestroyShaderModule(sDevice, mod, nullptr);
            }
        };
        try_load("shaders/aya_r41_exemplar/sky_placeholderV.spv");
        try_load("shaders/aya_r41_exemplar/sky_placeholderF.spv");
    }

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

// r41 sub-step 3.2 smoke-test (refine 2026-05-29): sky pool 1 draw 投入
// (sub-doc 03 §3.1 sub-step 3.2、LLDrawPoolSky::recordPoolDraws から呼出、
// fullscreen triangle で sky blue (0.4, 0.6, 0.9, 1.0) 出力)。
void recordSkySmokeDraw(VkCommandBuffer cmd_buf)
{
    if (cmd_buf == VK_NULL_HANDLE || sSkySmokePipeline == VK_NULL_HANDLE)
    {
        return;
    }
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, sSkySmokePipeline);
    vkCmdDraw(cmd_buf, 3, 1, 0, 0);
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

// r41 sub-step 3.3-B-β-2: SPIR-V shader module load helper (sub-doc 03 §3.1.3)
// build 時 glslangValidator で pre-compile した .spv binary を VkShaderModule 化。
// 領域 6 sub-step 6.1 一括化までの 1 shader exemplar pre-flight (sky placeholder)。
VkShaderModule loadSpirvShaderModule(const U32* spv_code, size_t code_size_bytes)
{
    if (!sInitialized || sDevice == VK_NULL_HANDLE)
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

    LL_INFOS("Vulkan") << "SPIR-V shader module loaded (sub-step 3.3-B-β-2 exemplar pre-flight)"
                       << " size=" << (S32)code_size_bytes << " bytes" << LL_ENDL;
    return module;
}

} // namespace LLVKLoader
