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

    bool createDevice()
    {
        F32 priorities[] = { 1.0f };

        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = sGraphicsQueueFamily;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = priorities;

        VkDeviceCreateInfo device_info = {};
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &queue_info;
        device_info.enabledExtensionCount = 0;
        device_info.ppEnabledExtensionNames = nullptr;

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

    U32 instance_version = volkGetInstanceVersion();
    LL_INFOS("Vulkan") << "Vulkan loader version "
                       << VK_VERSION_MAJOR(instance_version) << "."
                       << VK_VERSION_MINOR(instance_version) << "."
                       << VK_VERSION_PATCH(instance_version) << LL_ENDL;

    if (!createInstance())
    {
        return false;
    }

    if (!selectPhysicalDevice() || !selectQueueFamily() || !createDevice())
    {
        if (sInstance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(sInstance, nullptr);
            sInstance = VK_NULL_HANDLE;
        }
        return false;
    }

    sInitialized = true;
    return true;
}

void shutdownVulkan()
{
    if (sDevice != VK_NULL_HANDLE)
    {
        vkDestroyDevice(sDevice, nullptr);
        sDevice = VK_NULL_HANDLE;
        sGraphicsQueue = VK_NULL_HANDLE;
        sGraphicsQueueFamily = UINT_MAX;
        LL_INFOS("Vulkan") << "Vulkan device destroyed" << LL_ENDL;
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

bool isVulkanInitialized()
{
    return sInitialized;
}

bool isValidationEnabled()
{
    return sValidationEnabled;
}

} // namespace LLVKLoader
