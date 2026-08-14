/**
* @file llvkdevice.cpp
* @brief AYAstorm r42 Vulkan loader — instance / physical device / logical device / queue / command pool / pipeline cache / VMA allocator
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
    bool                 sHostQueryResetEnabled  = false;
    bool                     sVertexStoresAtomicsEnabled             = false; // B.2: skin A/B oracle atomics
    float                    sMaxLineWidth                           = 1.0f;
    bool                     sGeometryShaderEnabled                  = false;
    bool                     sImageCubeArrayEnabled                  = false;
    std::string                sDriverName;
    std::string                sDriverInfo;
    U32                        sDeviceLocalMemoryMB                  = 0;

    bool                     sProvokingVertexLastEnabled             = false;
    bool                     sMultiviewEnabled                       = false;

    // ── GPU 完了タイムライン(fence 群を置換する単一の完了機構)──
    // 各 submit が単調値 v を signal。完了照会 = vkGetSemaphoreCounterValue >= v で
    // 任意スレッドから race 無く可能(fence の host external-sync 制約が無い)。
    bool                  sTimelineSemaphoreEnabled = false;

    struct DeviceLimits
    {
        bool memoryBudgetSupported              = false;
    };
    DeviceLimits sDeviceLimits;
} // namespace

namespace LLVKLoaderInternal
{

    bool vkValidationRequested()
    {
        static const bool s_requested = []
        {
            const char* e = getenv("AYASTORM_VK_VALIDATION");
            return e != nullptr && e[0] != '\0' && e[0] != '0';
        }();
        return s_requested;
    }
} // namespace LLVKLoaderInternal

namespace
{

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
} // namespace

namespace LLVKLoaderInternal
{

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

        U32 extension_count = 0;
        VkResult extension_result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
        if (extension_result != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "instance extension enumeration failed result=" << extension_result << LL_ENDL;
            return false;
        }

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        if (extension_count)
        {
            extension_result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count,
                                                                        available_extensions.data());
            if (extension_result != VK_SUCCESS)
            {
                LL_WARNS("Vulkan") << "instance extension enumeration data failed result=" << extension_result << LL_ENDL;
                return false;
            }
        }

        const auto has_instance_extension = [&available_extensions](const char* name) {
            return std::any_of(available_extensions.begin(), available_extensions.end(),
                               [name](const VkExtensionProperties& property) {
                                   return strcmp(property.extensionName, name) == 0;
                               });
        };

#if LL_DARWIN
        if (!has_instance_extension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
        {
            LL_WARNS("Vulkan") << "VK_KHR_portability_enumeration unavailable" << LL_ENDL;
            return false;
        }
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        LL_INFOS("Vulkan") << "VK_KHR_portability_enumeration enabled" << LL_ENDL;
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

            have_debug_utils = has_instance_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

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
#if LL_DARWIN
        create_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
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
            LL_WARNS("Vulkan") << "vkCreateInstance failed result=" << result << LL_ENDL;
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
} // namespace LLVKLoaderInternal

namespace
{

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
} // namespace

namespace LLVKLoaderInternal
{

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
} // namespace LLVKLoaderInternal

namespace
{

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
} // namespace

namespace LLVKLoaderInternal
{

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
        bool display_timing_supported = false;
        const bool display_timing_requested = []() -> bool {
            const char* vkc = getenv("AYASTORM_VKC");
            const char* perf = getenv("AYASTORM_PERF_LOG");
            if (vkc == nullptr || std::strcmp(vkc, "1") != 0 || perf == nullptr)
            {
                return false;
            }
            char* end = nullptr;
            const F64 interval = strtod(perf, &end);
            return end != perf && end != nullptr && *end == '\0' && interval > 0.0;
        }();
        sDisplayTimingRequested = display_timing_requested;
        sDisplayTimingEnabled = false;
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
                else if (std::strcmp(e.extensionName, VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME) == 0)
                {
                    display_timing_supported = true;
                }
            }
        }
        VkPhysicalDeviceFaultFeaturesEXT fault_features_enable = {};
        fault_features_enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT;
        if (device_fault_supported)
        {
            VkPhysicalDeviceFaultFeaturesEXT fault_query = {};
            fault_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT;
            VkPhysicalDeviceFeatures2 fault_f2 = {};
            fault_f2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            fault_f2.pNext = &fault_query;
            vkGetPhysicalDeviceFeatures2(sPhysicalDevice, &fault_f2);
            if (fault_query.deviceFault)
            {
                device_extensions.push_back("VK_EXT_device_fault");
                fault_features_enable.deviceFault = VK_TRUE;
                sDeviceFaultEnabled = true;
            }
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
        if (display_timing_requested && display_timing_supported)
        {
            device_extensions.push_back(VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME);
            sDisplayTimingEnabled = true;
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
            if (vk12_query.timelineSemaphore)
            {
                // GPU 完了タイムライン: host 照会がスレッド安全(fence の external-sync 制約無し)。
                vk12_features_enable.timelineSemaphore = VK_TRUE;
                sTimelineSemaphoreEnabled = true;
            }
        }

        VkPhysicalDeviceVulkan11Features vk11_features_enable = {};
        vk11_features_enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        U32 multiview_max_view_count = 0;
        {
            VkPhysicalDeviceVulkan11Features vk11_query = {};
            vk11_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
            VkPhysicalDeviceFeatures2 vk11_f2 = {};
            vk11_f2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            vk11_f2.pNext = &vk11_query;
            vkGetPhysicalDeviceFeatures2(sPhysicalDevice, &vk11_f2);

            VkPhysicalDeviceMultiviewProperties mv_props = {};
            mv_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES;
            VkPhysicalDeviceProperties2 mv_p2 = {};
            mv_p2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            mv_p2.pNext = &mv_props;
            vkGetPhysicalDeviceProperties2(sPhysicalDevice, &mv_p2);
            multiview_max_view_count = mv_props.maxMultiviewViewCount;

            if (vk11_query.multiview && multiview_max_view_count >= 4u)
            {
                vk11_features_enable.multiview = VK_TRUE;
                sMultiviewEnabled = true;
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
        if (sDeviceFaultEnabled)
        {
            fault_features_enable.pNext = const_cast<void*>(device_info.pNext);
            device_info.pNext           = &fault_features_enable;
        }
        if (sMultiviewEnabled)
        {
            vk11_features_enable.pNext = const_cast<void*>(device_info.pNext);
            device_info.pNext          = &vk11_features_enable;
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
                           << " multiview=" << (sMultiviewEnabled?1:0) << " mvMaxViews=" << multiview_max_view_count
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

        // GPU 完了タイムラインは volkLoadDevice の後(device 関数 load 後)かつ
        // init 時の one-shot submit より前に作る。
        if (sTimelineSemaphoreEnabled)
        {
            VkSemaphoreTypeCreateInfo tci = {};
            tci.sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            tci.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            tci.initialValue  = 0;
            VkSemaphoreCreateInfo sci = {};
            sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            sci.pNext = &tci;
            if (vkCreateSemaphore(sDevice, &sci, nullptr, &sGpuTimeline) != VK_SUCCESS)
            {
                LL_WARNS("Vulkan") << "timeline semaphore creation failed" << LL_ENDL;
                sGpuTimeline = VK_NULL_HANDLE;
                return false;
            }
        }

        if (sCheckpointsEnabled && (vkCmdSetCheckpointNV == nullptr || vkGetQueueCheckpointDataNV == nullptr))
        {
            sCheckpointsEnabled = false;
        }
        if (sDeviceFaultEnabled && vkGetDeviceFaultInfoEXT == nullptr)
        {
            sDeviceFaultEnabled = false;
        }
        if (sDisplayTimingEnabled && vkGetPastPresentationTimingGOOGLE == nullptr)
        {
            LL_WARNS("Vulkan") << "display_timing extension was enabled but vkGetPastPresentationTimingGOOGLE is unavailable; disabling diagnostic" << LL_ENDL;
            sDisplayTimingEnabled = false;
        }
        LL_INFOS("Vulkan") << "GPU breadcrumb checkpoints enabled=" << (sCheckpointsEnabled ? 1 : 0)
                           << " device_fault enabled=" << (sDeviceFaultEnabled ? 1 : 0) << LL_ENDL;
        LL_INFOS("Vulkan") << "present_wait (vsync sleep) enabled=" << (sPresentWaitEnabled ? 1 : 0) << LL_ENDL;
        if (display_timing_requested)
        {
            LL_INFOS("Vulkan") << "display_timing diagnostic requested=1"
                               << " extension=" << (display_timing_supported ? 1 : 0)
                               << " enabled=" << (sDisplayTimingEnabled ? 1 : 0) << LL_ENDL;
        }
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
} // namespace LLVKLoaderInternal

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
} // namespace pcache

namespace LLVKLoaderInternal
{

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
        if (result == VK_SUCCESS)
        {
            return true;
        }

        // Pipeline-cache contents are implementation specific. In particular,
        // a cache written by a prior MoltenVK/Metal driver must not prevent a
        // Vulkan-only launch after either component changes.
        if (info.initialDataSize != 0)
        {
            LL_WARNS("Vulkan") << "vkCreatePipelineCache rejected cached data result=" << result
                               << "; retrying with an empty cache" << LL_ENDL;
            pcache::sBlob.clear();
            info.initialDataSize = 0;
            info.pInitialData    = nullptr;
            result = vkCreatePipelineCache(sDevice, &info, nullptr, &sPipelineCache);
            if (result == VK_SUCCESS)
            {
                return true;
            }
        }

        LL_WARNS("Vulkan") << "vkCreatePipelineCache failed result=" << result << LL_ENDL;
        sPipelineCache = VK_NULL_HANDLE;
        return false;
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
} // namespace LLVKLoaderInternal

static VkShaderModule loadSpirvShaderModule(const U32* spv_code, size_t code_size_bytes);

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

bool isProvokingVertexLastEnabled()
{
    return sProvokingVertexLastEnabled;
}

bool isGeometryShaderEnabledVk()
{
    return sGeometryShaderEnabled;
}

bool isMultiviewEnabled()
{
    return sMultiviewEnabled;
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

} // namespace LLVKLoader
