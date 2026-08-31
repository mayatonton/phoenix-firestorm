/**
* @file llvkswapchain.cpp
* @brief AYAstorm r42 Vulkan loader — surface / swapchain / sync objects / aux window swapchain
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
#if LL_LINUX
#include <pthread.h>
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

    VkSurfaceKHR sSurface           = VK_NULL_HANDLE;
    LLWindow*    sSurfaceWindow     = nullptr;
    VkFormat                 sSwapchainFormat     = VK_FORMAT_UNDEFINED;

    struct AuxWindowVk
    {
        bool                     active    = false;
        VkSurfaceKHR             surface   = VK_NULL_HANDLE;
        VkSwapchainKHR           swapchain = VK_NULL_HANDLE;
        std::vector<VkImage>     images;
        std::vector<VkImageView> views;
        VkFormat                 format = VK_FORMAT_UNDEFINED;
        VkExtent2D               extent = {0, 0};
        VkSemaphore     imageAvailable[FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
        VkSemaphore     renderFinished[FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
        VkFence         fences[FRAMES_IN_FLIGHT]         = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
        VkCommandBuffer cbs[FRAMES_IN_FLIGHT]            = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
        bool            fenceInFlight[FRAMES_IN_FLIGHT]  = { false, false, false };
        bool            recreatePending = false;
    };
    AuxWindowVk sAuxWindow;
    void*         sSwapchainDepthAlloc  = nullptr;
} // namespace

namespace LLVKLoaderInternal
{

    bool createSyncObjects()
    {
        // fence は全廃(GPU 完了は sGpuTimeline で判定)。ここは present 用 binary semaphore のみ。
        for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
        {
            sProducerFencePending[i] = false;
        }
        sAsyncProducerInFlight = false;

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
                return false;
            }
        }

        return true;
    }

    void destroySyncObjects()
    {
        if (sGpuTimeline != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(sDevice, sGpuTimeline, nullptr);
            sGpuTimeline = VK_NULL_HANDLE;
        }
        sAsyncProducerInFlight = false;
        for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
        {
            sProducerFencePending[i] = false;
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
} // namespace LLVKLoaderInternal

namespace
{

    bool createSwapchain(VkSwapchainKHR old_swapchain = VK_NULL_HANDLE)
    {
        if (sDevice == VK_NULL_HANDLE || sPhysicalDevice == VK_NULL_HANDLE ||
            sSurface == VK_NULL_HANDLE)
        {
            LL_WARNS("Vulkan") << "swapchain prerequisites missing: device="
                               << (sDevice != VK_NULL_HANDLE) << " physical_device="
                               << (sPhysicalDevice != VK_NULL_HANDLE) << " surface="
                               << (sSurface != VK_NULL_HANDLE) << LL_ENDL;
            return false;
        }

#if LL_DARWIN
        if (sSurfaceWindow != nullptr)
        {
            sSurfaceWindow->syncNativePresentationGeometry();
        }
#endif

        VkSurfaceCapabilitiesKHR caps = {};
        VkResult cres = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(sPhysicalDevice,
                                                                   sSurface, &caps);
        if (cres != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed result=" << cres << LL_ENDL;
            return false;
        }

        U32 format_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(sPhysicalDevice, sSurface,
                                              &format_count, nullptr);
        if (format_count == 0)
        {
            LL_WARNS("Vulkan") << "surface reports no usable formats" << LL_ENDL;
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
        {
            U32 w = 1280;
            U32 h = 720;
            if (caps.currentExtent.width != UINT32_MAX)
            {
                w = caps.currentExtent.width;
                h = caps.currentExtent.height;
            }
#if LL_DARWIN
            else
            {
                LLCoordWindow drawable_size;
                if (sSurfaceWindow != nullptr && sSurfaceWindow->getSize(&drawable_size) &&
                    drawable_size.mX > 0 && drawable_size.mY > 0)
                {
                    w = (U32)drawable_size.mX;
                    h = (U32)drawable_size.mY;
                }
            }
#endif
            if (w < caps.minImageExtent.width)  w = caps.minImageExtent.width;
            if (h < caps.minImageExtent.height) h = caps.minImageExtent.height;
            if (w > caps.maxImageExtent.width)  w = caps.maxImageExtent.width;
            if (h > caps.maxImageExtent.height) h = caps.maxImageExtent.height;
            extent.width  = w;
            extent.height = h;
        }

#if LL_DARWIN
        {
            LLCoordWindow backing_size;
            if (sSurfaceWindow != nullptr && sSurfaceWindow->getSize(&backing_size))
            {
                LL_INFOS("Vulkan") << "macOS swapchain extent=" << extent.width << "x" << extent.height
                                   << " surface currentExtent=" << caps.currentExtent.width
                                   << "x" << caps.currentExtent.height
                                   << " window backing=" << backing_size.mX << "x" << backing_size.mY << LL_ENDL;
            }
        }
#endif

        U32 image_count = caps.minImageCount + FRAMES_IN_FLIGHT - 1;
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
            LL_WARNS("Vulkan") << "vkCreateSwapchainKHR failed result=" << res
                               << " extent=" << extent.width << "x" << extent.height << LL_ENDL;
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
                LL_WARNS("Vulkan") << "vkCreateImageView for swapchain image " << i
                                   << " failed result=" << vres << LL_ENDL;
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
} // namespace

namespace LLVKLoaderInternal
{

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

        const U32        reason_mask = sRecreateReasonMask.exchange(0);
        const VkExtent2D old_extent  = sSwapchainExtent;
        const U32 frames_since_last  = (sLastRecreateFrame == 0)
                                           ? 0
                                           : (sMonotonicFrameCount - sLastRecreateFrame);

        const auto drain_t0 = std::chrono::steady_clock::now();
        peDrain();
        const auto drain_t1 = std::chrono::steady_clock::now();
        vkDeviceWaitIdle(sDevice);
        const auto drain_t2 = std::chrono::steady_clock::now();
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

        sLastRecreateFrame        = sMonotonicFrameCount;
        if (ok)
        {
            sSwapchainRecreatePending = false;
        }
        else
        {
            sSwapchainRecreatePending = true;
            LL_WARNS("Vulkan") << "recreateSwapchain failed; re-arming pending recreate" << LL_ENDL;
        }

        std::string reasons;
        if (reason_mask & RECREATE_REASON_RESIZE)          { reasons += "resize,"; }
        if (reason_mask & RECREATE_REASON_ACQ_SUBOPTIMAL)  { reasons += "acq-suboptimal,"; }
        if (reason_mask & RECREATE_REASON_ACQ_OUT_OF_DATE) { reasons += "acq-out-of-date,"; }
        if (reason_mask & RECREATE_REASON_PRS_SUBOPTIMAL)  { reasons += "present-suboptimal,"; }
        if (reason_mask & RECREATE_REASON_PRS_OUT_OF_DATE) { reasons += "present-out-of-date,"; }
        if (reason_mask & RECREATE_REASON_VSYNC_SETTING)   { reasons += "vsync-setting,"; }
        if (reasons.empty())                               { reasons = "unknown,"; }
        reasons.pop_back();
        LL_INFOS("Vulkan") << "recreateSwapchain: reason=" << reasons
                           << " old=" << old_extent.width << "x" << old_extent.height
                           << " new=" << sSwapchainExtent.width << "x" << sSwapchainExtent.height
                           << " frames_since_last=" << frames_since_last
                           << " drain_us=" << std::chrono::duration_cast<std::chrono::microseconds>(drain_t1 - drain_t0).count()
                           << " wait_idle_us=" << std::chrono::duration_cast<std::chrono::microseconds>(drain_t2 - drain_t1).count()
                           << " ok=" << (ok ? 1 : 0) << LL_ENDL;

        U64 fp = (U64)reason_mask;
        fp = fp * 1099511628211ull + old_extent.width;
        fp = fp * 1099511628211ull + old_extent.height;
        fp = fp * 1099511628211ull + sSwapchainExtent.width;
        fp = fp * 1099511628211ull + sSwapchainExtent.height;
        fp = fp * 1099511628211ull + (U64)sActivePresentMode;
        fp = fp * 1099511628211ull + (ok ? 1u : 0u);
        LLVKContract::noteCorrectiveAction("swapchain_recreate", fp);

        return ok;
    }
} // namespace LLVKLoaderInternal


void shutdownSwapchainAndSurface()
{
    if (sDevice != VK_NULL_HANDLE)
    {
        if (!sVkDeviceLost.load(std::memory_order_acquire))
        {
            vkDeviceWaitIdle(sDevice);
        }
        auxWindowShutdownVk();
        destroySwapchain();
    }
    shutdownSurface();
}

bool initSurface(LLWindow* window)
{
    if (!sInitialized)
    {
        LL_WARNS("Vulkan") << "surface initialization requested before Vulkan initialization completed" << LL_ENDL;
        return false;
    }
    if (sSurface != VK_NULL_HANDLE)
    {
        return true;
    }
    if (!window)
    {
        LL_WARNS("Vulkan") << "surface initialization requested without a native window" << LL_ENDL;
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
            LL_WARNS("Vulkan") << "VK_EXT_metal_surface entry point is unavailable" << LL_ENDL;
            return false;
        }
        result = vkCreateMetalSurfaceEXT(sInstance, &ci, nullptr, &sSurface);
    }
    else
    {
        LL_WARNS("Vulkan") << "macOS window did not provide a CAMetalLayer" << LL_ENDL;
        return false;
    }
#else
    LL_WARNS("Vulkan") << "no Vulkan window-surface platform was compiled for this target" << LL_ENDL;
    return false;
#endif

    if (result != VK_SUCCESS || sSurface == VK_NULL_HANDLE)
    {
        LL_WARNS("Vulkan") << "vkCreate*SurfaceKHR failed result=" << result << LL_ENDL;
        sSurface = VK_NULL_HANDLE;
        return false;
    }

    sSurfaceWindow = window;

    LL_INFOS("Vulkan") << "Vulkan presentation surface initialized" << LL_ENDL;

    return true;
}

void shutdownSurface()
{
    if (sSurface != VK_NULL_HANDLE && sInstance != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(sInstance, sSurface, nullptr);
        sSurface = VK_NULL_HANDLE;
    }
    sSurfaceWindow = nullptr;
}

VkSurfaceKHR getSurface()
{
    return sSurface;
}

static bool auxCreateSwapchain()
{
    AuxWindowVk& aw = sAuxWindow;
    if (sDevice == VK_NULL_HANDLE || aw.surface == VK_NULL_HANDLE)
    {
        return false;
    }

    VkSurfaceCapabilitiesKHR caps = {};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(sPhysicalDevice, aw.surface, &caps) != VK_SUCCESS)
    {
        return false;
    }
    const VkImageUsageFlags want_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if ((caps.supportedUsageFlags & want_usage) != want_usage)
    {
        return false;
    }

    U32 format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(sPhysicalDevice, aw.surface, &format_count, nullptr);
    if (format_count == 0)
    {
        return false;
    }
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(sPhysicalDevice, aw.surface, &format_count, formats.data());
    VkSurfaceFormatKHR chosen = formats[0];
    for (const VkSurfaceFormatKHR& f : formats)
    {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            chosen = f;
            break;
        }
    }

    VkExtent2D extent = caps.currentExtent;
    if (extent.width == UINT32_MAX)
    {
        extent.width  = llclamp(640u, caps.minImageExtent.width,  caps.maxImageExtent.width);
        extent.height = llclamp(480u, caps.minImageExtent.height, caps.maxImageExtent.height);
    }
    if (extent.width == 0 || extent.height == 0)
    {
        return false;
    }

    U32 image_count = caps.minImageCount + FRAMES_IN_FLIGHT - 1;
    if (caps.maxImageCount > 0 && image_count > caps.maxImageCount)
    {
        image_count = caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR ci = {};
    ci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface          = aw.surface;
    ci.minImageCount    = image_count;
    ci.imageFormat      = chosen.format;
    ci.imageColorSpace  = chosen.colorSpace;
    ci.imageExtent      = extent;
    ci.imageArrayLayers = 1;
    ci.imageUsage       = want_usage;
    ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.preTransform     = caps.currentTransform;
    ci.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode      = VK_PRESENT_MODE_FIFO_KHR;
    ci.clipped          = VK_TRUE;

    if (vkCreateSwapchainKHR(sDevice, &ci, nullptr, &aw.swapchain) != VK_SUCCESS)
    {
        aw.swapchain = VK_NULL_HANDLE;
        return false;
    }
    aw.format = chosen.format;
    aw.extent = extent;

    U32 actual = 0;
    vkGetSwapchainImagesKHR(sDevice, aw.swapchain, &actual, nullptr);
    aw.images.assign(actual, VK_NULL_HANDLE);
    vkGetSwapchainImagesKHR(sDevice, aw.swapchain, &actual, aw.images.data());
    aw.views.assign(actual, VK_NULL_HANDLE);
    for (U32 i = 0; i < actual; ++i)
    {
        VkImageViewCreateInfo vci = {};
        vci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image                           = aw.images[i];
        vci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        vci.format                          = aw.format;
        vci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        vci.subresourceRange.baseMipLevel   = 0;
        vci.subresourceRange.levelCount     = 1;
        vci.subresourceRange.baseArrayLayer = 0;
        vci.subresourceRange.layerCount     = 1;
        if (vkCreateImageView(sDevice, &vci, nullptr, &aw.views[i]) != VK_SUCCESS)
        {
            for (U32 j = 0; j < i; ++j)
            {
                vkDestroyImageView(sDevice, aw.views[j], nullptr);
            }
            aw.views.clear();
            return false;
        }
    }
    return true;
}

static void auxDestroySwapchain()
{
    AuxWindowVk& aw = sAuxWindow;
    peDrain();
    for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        if (aw.fenceInFlight[i] && aw.fences[i] != VK_NULL_HANDLE)
        {
            vkWaitForFences(sDevice, 1, &aw.fences[i], VK_TRUE, UINT64_MAX);
            vkResetFences(sDevice, 1, &aw.fences[i]);
            aw.fenceInFlight[i] = false;
        }
    }
    for (VkImageView v : aw.views)
    {
        if (v != VK_NULL_HANDLE)
        {
            vkDestroyImageView(sDevice, v, nullptr);
        }
    }
    aw.views.clear();
    aw.images.clear();
    if (aw.swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(sDevice, aw.swapchain, nullptr);
        aw.swapchain = VK_NULL_HANDLE;
    }
    aw.format = VK_FORMAT_UNDEFINED;
    aw.extent = {0, 0};
}

bool auxWindowInitVk(void* native_display, void* native_window)
{
    if (!sInitialized || sAuxWindow.active)
    {
        return sAuxWindow.active;
    }
#if defined(VK_USE_PLATFORM_XLIB_KHR)
    if (native_display == nullptr || native_window == nullptr)
    {
        return false;
    }
    VkXlibSurfaceCreateInfoKHR ci = {};
    ci.sType  = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    ci.dpy    = static_cast<Display*>(native_display);
    ci.window = static_cast<Window>(reinterpret_cast<uintptr_t>(native_window));
    if (vkCreateXlibSurfaceKHR(sInstance, &ci, nullptr, &sAuxWindow.surface) != VK_SUCCESS)
    {
        sAuxWindow.surface = VK_NULL_HANDLE;
        return false;
    }
#else
    return false;
#endif

    if (!auxCreateSwapchain())
    {
        vkDestroySurfaceKHR(sInstance, sAuxWindow.surface, nullptr);
        sAuxWindow.surface = VK_NULL_HANDLE;
        return false;
    }

    VkSemaphoreCreateInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fi = {};
    fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkCommandBufferAllocateInfo ai = {};
    ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool        = sCommandPool;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = FRAMES_IN_FLIGHT;
    bool ok = vkAllocateCommandBuffers(sDevice, &ai, sAuxWindow.cbs) == VK_SUCCESS;
    for (U32 i = 0; ok && i < FRAMES_IN_FLIGHT; ++i)
    {
        ok = ok && vkCreateSemaphore(sDevice, &si, nullptr, &sAuxWindow.imageAvailable[i]) == VK_SUCCESS;
        ok = ok && vkCreateSemaphore(sDevice, &si, nullptr, &sAuxWindow.renderFinished[i]) == VK_SUCCESS;
        ok = ok && vkCreateFence(sDevice, &fi, nullptr, &sAuxWindow.fences[i]) == VK_SUCCESS;
    }
    if (!ok)
    {
        auxWindowShutdownVk();
        return false;
    }
    sAuxWindow.active = true;
    return true;
}

void auxWindowShutdownVk()
{
    AuxWindowVk& aw = sAuxWindow;
    if (sDevice != VK_NULL_HANDLE)
    {
        auxDestroySwapchain();
        for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
        {
            if (aw.imageAvailable[i] != VK_NULL_HANDLE) vkDestroySemaphore(sDevice, aw.imageAvailable[i], nullptr);
            if (aw.renderFinished[i] != VK_NULL_HANDLE) vkDestroySemaphore(sDevice, aw.renderFinished[i], nullptr);
            if (aw.fences[i] != VK_NULL_HANDLE)         vkDestroyFence(sDevice, aw.fences[i], nullptr);
            aw.imageAvailable[i] = VK_NULL_HANDLE;
            aw.renderFinished[i] = VK_NULL_HANDLE;
            aw.fences[i]         = VK_NULL_HANDLE;
            aw.fenceInFlight[i]  = false;
        }
        if (aw.cbs[0] != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(sDevice, sCommandPool, FRAMES_IN_FLIGHT, aw.cbs);
            for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i) aw.cbs[i] = VK_NULL_HANDLE;
        }
    }
    if (aw.surface != VK_NULL_HANDLE && sInstance != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(sInstance, aw.surface, nullptr);
    }
    aw.surface = VK_NULL_HANDLE;
    aw.active  = false;
    aw.recreatePending = false;
}

bool auxWindowActiveVk()
{
    return sAuxWindow.active;
}

void auxWindowNotifyResizeVk()
{
    if (sAuxWindow.active)
    {
        sAuxWindow.recreatePending = true;
    }
}

static bool auxRecycleAcquireSemaphores()
{
    for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        if (sAuxWindow.imageAvailable[i] != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(sDevice, sAuxWindow.imageAvailable[i], nullptr);
            sAuxWindow.imageAvailable[i] = VK_NULL_HANDLE;
        }
        VkSemaphoreCreateInfo si = {};
        si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (vkCreateSemaphore(sDevice, &si, nullptr, &sAuxWindow.imageAvailable[i]) != VK_SUCCESS)
        {
            return false;
        }
    }
    return true;
}

bool auxWindowExtentVk(U32& out_w, U32& out_h)
{
    if (!sAuxWindow.active)
    {
        return false;
    }
    out_w = sAuxWindow.extent.width;
    out_h = sAuxWindow.extent.height;
    return out_w > 0 && out_h > 0;
}

static U32  sAuxUISlot        = 0;
static U32  sAuxUIImageIndex  = 0;
static bool sAuxUIFrameActive = false;
static S32  sAuxUISavedViewport[4] = {0, 0, 0, 0};
static VkCommandBuffer            sAuxUISavedOverride   = VK_NULL_HANDLE;
static bool                       sAuxUISavedInDR       = false;
static VkRenderingAttachmentInfo  sAuxUISavedColor[4]   = {};
static U32                        sAuxUISavedColorCount = 0;
static bool                       sAuxUISavedHasDepth   = false;
static VkRenderingAttachmentInfo  sAuxUISavedDepth      = {};
static U32                        sAuxUISavedAreaHeight = 0;
static U32                        sAuxUISavedRenderW    = 0;
static U32                        sAuxUISavedRenderH    = 0;
static U32                        sAuxUISavedViewMask   = 0;
static U32                        sAuxUISavedLayerCount = 1;

bool auxWindowBeginUIFrameVk()
{
    AuxWindowVk& aw = sAuxWindow;
    if (!aw.active || sAuxUIFrameActive
        || sVkDeviceLost.load(std::memory_order_acquire))
    {
        return false;
    }
    if (aw.recreatePending)
    {
        for (U32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
        {
            if (aw.fenceInFlight[i])
            {
                vkWaitForFences(sDevice, 1, &aw.fences[i], VK_TRUE, UINT64_MAX);
                vkResetFences(sDevice, 1, &aw.fences[i]);
                aw.fenceInFlight[i] = false;
            }
        }
        const VkExtent2D aux_old = aw.extent;
        auxDestroySwapchain();
        const bool aux_ok = auxCreateSwapchain() && auxRecycleAcquireSemaphores();
        U64 aux_fp = (U64)aux_old.width;
        aux_fp = aux_fp * 1099511628211ull + aux_old.height;
        aux_fp = aux_fp * 1099511628211ull + aw.extent.width;
        aux_fp = aux_fp * 1099511628211ull + aw.extent.height;
        aux_fp = aux_fp * 1099511628211ull + (aux_ok ? 1u : 0u);
        LLVKContract::noteCorrectiveAction("aux_swapchain_recreate", aux_fp);
        if (!aux_ok)
        {
            return false;
        }
        aw.recreatePending = false;
    }

    const U32 slot = sFrameIndex % FRAMES_IN_FLIGHT;
    if (aw.fenceInFlight[slot])
    {
        const U64 f0 = vkMonoUs();
        vkWaitForFences(sDevice, 1, &aw.fences[slot], VK_TRUE, UINT64_MAX);
        sAuxBeginFenceUs += vkMonoUs() - f0;
        vkResetFences(sDevice, 1, &aw.fences[slot]);
        aw.fenceInFlight[slot] = false;
    }

    U32 image_index = 0;
    VkResult ar;
    {
        const U64 a0 = vkMonoUs();
        std::lock_guard<std::mutex> lk(sSwapchainAccessMutex);
        ar = vkAcquireNextImageKHR(sDevice, aw.swapchain, UINT64_MAX,
                                   aw.imageAvailable[slot], VK_NULL_HANDLE, &image_index);
        sAuxBeginAcqUs += vkMonoUs() - a0;
    }
    if (ar == VK_ERROR_OUT_OF_DATE_KHR)
    {
        aw.recreatePending = true;
        return false;
    }
    if (ar != VK_SUCCESS && ar != VK_SUBOPTIMAL_KHR)
    {
        return false;
    }
    if (ar == VK_SUBOPTIMAL_KHR)
    {
        aw.recreatePending = true;
    }
    if (image_index >= (U32)aw.views.size() || aw.views[image_index] == VK_NULL_HANDLE)
    {
        return false;
    }

    VkCommandBuffer cmd = aw.cbs[slot];
    vkResetCommandBuffer(cmd, 0);
    ++tCmdRecordEpoch;
    VkCommandBufferBeginInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &bi);

    VkImageMemoryBarrier to_color = {};
    to_color.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    to_color.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    to_color.newLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    to_color.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    to_color.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    to_color.image               = aw.images[image_index];
    to_color.srcAccessMask       = 0;
    to_color.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    to_color.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    to_color.subresourceRange.levelCount = 1;
    to_color.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &to_color);

    sAuxUISavedOverride   = tRecordCmdOverride;
    sAuxUISavedInDR       = sInDynamicRendering;
    sAuxUISavedColorCount = sSavedColorCount;
    sAuxUISavedHasDepth   = sSavedHasDepth;
    sAuxUISavedDepth      = sSavedDepthInfo;
    sAuxUISavedAreaHeight = sCurrentRenderAreaHeight;
    sAuxUISavedRenderW    = sSavedRenderWidth;
    sAuxUISavedRenderH    = sSavedRenderHeight;
    sAuxUISavedViewMask   = sSavedViewMask;
    sAuxUISavedLayerCount = sSavedLayerCount;
    for (U32 i = 0; i < 4; ++i)
    {
        sAuxUISavedColor[i] = sSavedColorInfos[i];
    }

    tRecordCmdOverride  = cmd;
    sInDynamicRendering = false;

    DynamicRenderingAttachment color = {};
    color.image_view   = aw.views[image_index];
    color.image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.load_op      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.store_op     = VK_ATTACHMENT_STORE_OP_STORE;
    color.clear_value  = {};
    color.clear_value.color.float32[3] = 1.0f;
    beginDynamicRendering(aw.extent.width, aw.extent.height, &color, 1, nullptr);
    if (!sInDynamicRendering)
    {
        tRecordCmdOverride  = sAuxUISavedOverride;
        sInDynamicRendering = sAuxUISavedInDR;
        sSavedColorCount    = sAuxUISavedColorCount;
        sSavedHasDepth      = sAuxUISavedHasDepth;
        sSavedDepthInfo     = sAuxUISavedDepth;
        sCurrentRenderAreaHeight = sAuxUISavedAreaHeight;
        sSavedRenderWidth   = sAuxUISavedRenderW;
        sSavedRenderHeight  = sAuxUISavedRenderH;
        sSavedViewMask      = sAuxUISavedViewMask;
        sSavedLayerCount    = sAuxUISavedLayerCount;
        for (U32 i = 0; i < 4; ++i)
        {
            sSavedColorInfos[i] = sAuxUISavedColor[i];
        }
        vkEndCommandBuffer(cmd);
        return false;
    }

    for (U32 i = 0; i < 4; ++i)
    {
        sAuxUISavedViewport[i] = sVkRenderViewport[i];
    }
    sVkRenderViewport[0] = 0;
    sVkRenderViewport[1] = 0;
    sVkRenderViewport[2] = (S32)aw.extent.width;
    sVkRenderViewport[3] = (S32)aw.extent.height;

    sAuxUISlot        = slot;
    sAuxUIImageIndex  = image_index;
    sAuxUIFrameActive = true;
    return true;
}

bool auxWindowEndUIFrameVk()
{
    AuxWindowVk& aw = sAuxWindow;
    if (!sAuxUIFrameActive)
    {
        return false;
    }
    const U32 slot        = sAuxUISlot;
    const U32 image_index = sAuxUIImageIndex;
    VkCommandBuffer cmd   = aw.cbs[slot];

    if (sInDynamicRendering)
    {
        vkCmdEndRendering(cmd);
        sInDynamicRendering = false;
    }
    tRecordCmdOverride  = sAuxUISavedOverride;
    sInDynamicRendering = sAuxUISavedInDR;
    sSavedColorCount    = sAuxUISavedColorCount;
    sSavedHasDepth      = sAuxUISavedHasDepth;
    sSavedDepthInfo     = sAuxUISavedDepth;
    sCurrentRenderAreaHeight = sAuxUISavedAreaHeight;
    sSavedRenderWidth   = sAuxUISavedRenderW;
    sSavedRenderHeight  = sAuxUISavedRenderH;
    sSavedViewMask      = sAuxUISavedViewMask;
    sSavedLayerCount    = sAuxUISavedLayerCount;
    for (U32 i = 0; i < 4; ++i)
    {
        sSavedColorInfos[i] = sAuxUISavedColor[i];
    }
    sAuxUIFrameActive  = false;
    for (U32 i = 0; i < 4; ++i)
    {
        sVkRenderViewport[i] = sAuxUISavedViewport[i];
    }
    ++tCmdRecordEpoch;

    VkImageMemoryBarrier to_present = {};
    to_present.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    to_present.oldLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    to_present.newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    to_present.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    to_present.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    to_present.image               = aw.images[image_index];
    to_present.srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    to_present.dstAccessMask       = 0;
    to_present.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    to_present.subresourceRange.levelCount = 1;
    to_present.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &to_present);

    vkEndCommandBuffer(cmd);

    PEJob job;
    job.cmd              = cmd;
    job.fence            = aw.fences[slot];
    job.wait_semaphore   = aw.imageAvailable[slot];
    job.signal_semaphore = aw.renderFinished[slot];
    PEPresentTarget target;
    target.swapchain      = aw.swapchain;
    target.image_index    = image_index;
    target.wait_semaphore = aw.renderFinished[slot];
    job.presents.push_back(target);
    peEnqueue(std::move(job));
    aw.fenceInFlight[slot] = true;
    return true;
}

bool initSwapchain()
{
    if (!sInitialized)
    {
        LL_WARNS("Vulkan") << "swapchain initialization requested before Vulkan initialization completed" << LL_ENDL;
        return false;
    }
    if (sSurface == VK_NULL_HANDLE)
    {
        LL_WARNS("Vulkan") << "swapchain initialization requested without a presentation surface" << LL_ENDL;
        return false;
    }
    if (sSwapchain != VK_NULL_HANDLE)
    {
        return true;
    }
    return createSwapchain();
}

VkFormat getSwapchainFormat()
{
    return sSwapchainFormat;
}

bool hasSwapchainDepth()
{
    return sSwapchainDepthView != VK_NULL_HANDLE;
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

    sRecreateReasonMask.fetch_or(RECREATE_REASON_RESIZE);
    sSwapchainRecreatePending = true;

}

} // namespace LLVKLoader
