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

#include <vector>
#include <string>
#include <climits>

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

    VkRenderPass sRenderPass = VK_NULL_HANDLE;
    VkImage sOffscreenImage = VK_NULL_HANDLE;
    VkDeviceMemory sOffscreenMemory = VK_NULL_HANDLE;
    VkImageView sOffscreenImageView = VK_NULL_HANDLE;
    VkFramebuffer sFramebuffer = VK_NULL_HANDLE;
    bool sInFrame = false;

    constexpr U32 OFFSCREEN_WIDTH = 64;
    constexpr U32 OFFSCREEN_HEIGHT = 64;
    constexpr VkFormat OFFSCREEN_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;

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

        VkDeviceCreateInfo device_info = {};
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
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

        sPlaceholderLayout = createStandardPipelineLayout(nullptr, 0, nullptr, 0);
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

    if (!createPlaceholderPipeline())
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

} // namespace LLVKLoader
