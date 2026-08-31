/**
* @file llvkimages.cpp
* @brief AYAstorm r42 Vulkan loader — image create-upload-copy-readback / fallback images / one-shot submit
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
#if LL_WINDOWS
#include <intrin.h>
#endif
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

namespace LLVKLoaderInternal
{

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

            peSubmitBlocking(one_cmd, VK_NULL_HANDLE, true);

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
        noteViewHandleCreated(sDefaultFallbackImageView);

        return true;
    }

    bool createWhiteImage()
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

        VkResult result = vkCreateImage(sDevice, &image_info, nullptr, &sWhiteImage);
        if (result != VK_SUCCESS)
        {
            return false;
        }

        VkMemoryRequirements mem_req;
        vkGetImageMemoryRequirements(sDevice, sWhiteImage, &mem_req);

        S32 mem_type = findMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (mem_type < 0)
        {
            return false;
        }

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize  = mem_req.size;
        alloc_info.memoryTypeIndex = (U32)mem_type;

        result = vkAllocateMemory(sDevice, &alloc_info, nullptr, &sWhiteMemory);
        if (result != VK_SUCCESS)
        {
            return false;
        }
        vkBindImageMemory(sDevice, sWhiteImage, sWhiteMemory, 0);

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
            b1.image               = sWhiteImage;
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
            vkCmdCopyBufferToImage(one_cmd, staging_buf, sWhiteImage,
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

            peSubmitBlocking(one_cmd, VK_NULL_HANDLE, true);

            vkFreeCommandBuffers(sDevice, sCommandPool, 1, &one_cmd);
            vkDestroyBuffer(sDevice, staging_buf, nullptr);
            vkFreeMemory(sDevice, staging_mem, nullptr);
        }

        VkImageViewCreateInfo vci = {};
        vci.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image                 = sWhiteImage;
        vci.viewType              = VK_IMAGE_VIEW_TYPE_2D;
        vci.format                = VK_FORMAT_R8G8B8A8_UNORM;
        vci.subresourceRange      = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        if (vkCreateImageView(sDevice, &vci, nullptr, &sWhiteImageView) != VK_SUCCESS)
        {
            return false;
        }
        noteViewHandleCreated(sWhiteImageView);

        return true;
    }

    bool createDefaultFallbackShadowImage()
    {
        VkImageCreateInfo image_info = {};
        image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType     = VK_IMAGE_TYPE_2D;
        image_info.format        = VK_FORMAT_D32_SFLOAT;
        image_info.extent        = { 1, 1, 1 };
        image_info.mipLevels     = 1;
        image_info.arrayLayers   = 1;
        image_info.samples       = VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(sDevice, &image_info, nullptr, &sDefaultFallbackShadowImage) != VK_SUCCESS)
        {
            return false;
        }

        VkMemoryRequirements mem_req;
        vkGetImageMemoryRequirements(sDevice, sDefaultFallbackShadowImage, &mem_req);

        S32 mem_type = findMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (mem_type < 0)
        {
            return false;
        }

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize  = mem_req.size;
        alloc_info.memoryTypeIndex = (U32)mem_type;

        if (vkAllocateMemory(sDevice, &alloc_info, nullptr, &sDefaultFallbackShadowMemory) != VK_SUCCESS)
        {
            return false;
        }
        vkBindImageMemory(sDevice, sDefaultFallbackShadowImage, sDefaultFallbackShadowMemory, 0);

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
            b1.image               = sDefaultFallbackShadowImage;
            b1.subresourceRange    = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
            b1.srcAccessMask       = 0;
            b1.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
            vkCmdPipelineBarrier(one_cmd,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0, 0, nullptr, 0, nullptr, 1, &b1);

            VkClearDepthStencilValue far_depth = { 1.f, 0 };
            VkImageSubresourceRange depth_range = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
            vkCmdClearDepthStencilImage(one_cmd, sDefaultFallbackShadowImage,
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                        &far_depth, 1, &depth_range);

            VkImageMemoryBarrier b2 = b1;
            b2.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            b2.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            b2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            b2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(one_cmd,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0, 0, nullptr, 0, nullptr, 1, &b2);

            vkEndCommandBuffer(one_cmd);

            peSubmitBlocking(one_cmd, VK_NULL_HANDLE, true);

            vkFreeCommandBuffers(sDevice, sCommandPool, 1, &one_cmd);
        }

        VkImageViewCreateInfo vci = {};
        vci.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image                 = sDefaultFallbackShadowImage;
        vci.viewType              = VK_IMAGE_VIEW_TYPE_2D;
        vci.format                = VK_FORMAT_D32_SFLOAT;
        vci.subresourceRange      = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
        if (vkCreateImageView(sDevice, &vci, nullptr, &sDefaultFallbackShadowImageView) != VK_SUCCESS)
        {
            return false;
        }
        noteViewHandleCreated(sDefaultFallbackShadowImageView);

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
        peSubmitBlocking(one_cmd, VK_NULL_HANDLE, true);
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
        peSubmitBlocking(one_cmd, VK_NULL_HANDLE, true);
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
        peSubmitBlocking(one_cmd, VK_NULL_HANDLE, true);
        vkFreeCommandBuffers(sDevice, sCommandPool, 1, &one_cmd);

        return true;
    }
} // namespace LLVKLoaderInternal

namespace
{

    void warnAttachmentAllocFail(const char* tag, VkResult r, U32 width, U32 height, VkFormat format, U32 mips, U32 layers)
    {
        std::string heaps;
        U64 total_blocks = 0, total_allocs = 0;
        if (sAllocator != VK_NULL_HANDLE)
        {
            VmaBudget budgets[VK_MAX_MEMORY_HEAPS] = {};
            vmaGetHeapBudgets(sAllocator, budgets);
            for (U32 i = 0; i < VK_MAX_MEMORY_HEAPS; ++i)
            {
                total_blocks += budgets[i].statistics.blockCount;
                total_allocs += budgets[i].statistics.allocationCount;
                if (budgets[i].budget > 0)
                {
                    heaps += " h" + std::to_string(i) + "="
                           + std::to_string((U64)(budgets[i].usage  >> 20)) + "/"
                           + std::to_string((U64)(budgets[i].budget >> 20)) + "MB";
                }
            }
        }
        U32 pending_frees_count;
        {
            std::lock_guard<std::mutex> guard(sPendingImageFreeMutex);
            pending_frees_count = (U32)sPendingImageFrees.size();
        }
        LL_WARNS("Vulkan") << "attachment image alloc failed tag=" << (tag ? tag : "attImg")
                           << " res=" << width << "x" << height
                           << " fmt=" << (S32)format
                           << " mips=" << mips
                           << " layers=" << layers
                           << " result=" << (S32)r
                           << heaps
                           << " blocks=" << total_blocks
                           << " allocs=" << total_allocs
                           << " pending_frees=" << pending_frees_count
                           << LL_ENDL;
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
                                     U32                mip_levels = 1,
                                     U32                array_layers = 1)
    {
        out_image      = VK_NULL_HANDLE;
        out_view       = VK_NULL_HANDLE;
        out_allocation = nullptr;

        if (width == 0 || height == 0 || format == VK_FORMAT_UNDEFINED)
        {
            warnAttachmentAllocFail(tag, VK_RESULT_MAX_ENUM, width, height, format, mip_levels, array_layers);
            return false;
        }
        if (sAllocator == VK_NULL_HANDLE || sDevice == VK_NULL_HANDLE)
        {
            warnAttachmentAllocFail(tag, VK_ERROR_INITIALIZATION_FAILED, width, height, format, mip_levels, array_layers);
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
        ici.arrayLayers   = (array_layers > 0) ? array_layers : 1;
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
            reclaimDeferredOnAllocFailure();
            r = vmaCreateImage(sAllocator, &ici, &aci, &image, &allocation, nullptr);
            if (r != VK_SUCCESS)
            {
                warnAttachmentAllocFail(tag, r, width, height, format, mip_levels, array_layers);
                return false;
            }
        }

        VkImageViewCreateInfo vci = {};
        vci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image                           = image;
        vci.viewType                        = (array_layers > 1)
                                                  ? VK_IMAGE_VIEW_TYPE_2D_ARRAY
                                                  : VK_IMAGE_VIEW_TYPE_2D;
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
        vci.subresourceRange.layerCount     = (array_layers > 0) ? array_layers : 1;

        VkImageView view = VK_NULL_HANDLE;
        r = vkCreateImageView(sDevice, &vci, nullptr, &view);
        if (r != VK_SUCCESS)
        {
            warnAttachmentAllocFail(tag, r, width, height, format, mip_levels, array_layers);
            vmaDestroyImage(sAllocator, image, allocation);
            return false;
        }
        noteViewHandleCreated(view);

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
} // namespace

static bool           submitOneShotVk(VkCommandBuffer cmd, VkBuffer staging_buffer, VmaAllocation staging_allocation,
                                      const char* source, U64 staging_bytes = 0);
static bool           submitOneShotVkFromPool(VkCommandBuffer cmd, VkCommandPool pool, VkBuffer staging_buffer,
                                              VmaAllocation staging_allocation, U64 staging_bytes,
                                              const char* source, U64 diagnostic_staging_bytes);

bool submitOneShotVk(VkCommandBuffer cmd, VkBuffer staging_buffer, VmaAllocation staging_allocation,
                     const char* source, U64 staging_bytes)
{
    return submitOneShotVkFromPool(cmd, threadCmdPool(), staging_buffer, staging_allocation, staging_bytes,
                                   source, staging_bytes);
}

static VkCommandBuffer beginOneShotCommandBufferVk()
{
    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool        = threadCmdPool();
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(sDevice, &cbai, &cmd) != VK_SUCCESS || cmd == VK_NULL_HANDLE)
    {
        return VK_NULL_HANDLE;
    }
    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &cbbi);
    if (vkValidationRequested())
    {
        setVkObjectName((U64)(uintptr_t)cmd, VK_OBJECT_TYPE_COMMAND_BUFFER,
                        t_cmdPool != VK_NULL_HANDLE ? "oneshot-worker" : "oneshot-main");
    }
    return cmd;
}

bool submitOneShotVkFromPool(VkCommandBuffer cmd, VkCommandPool pool, VkBuffer staging_buffer,
                             VmaAllocation staging_allocation, U64 staging_bytes,
                             const char* source, U64 diagnostic_staging_bytes)
{
#if !LL_DARWIN
    (void)source;
    (void)diagnostic_staging_bytes;
#endif
    if (t_cmdPool != VK_NULL_HANDLE)
    {
        U32 spins = 0;
        while (peQueueDepth() > 32 && ++spins < 25000)
        {
            tickOneShotFreeQueue();
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    }
    tickOneShotFreeQueue();
    const U64 byte_cap = 256ull << 20;
    for (U32 i = 0; i < 20; ++i)
    {
        bool over = false;
        PendingOneShotFree wait_entry;
        bool have_entry = false;
        {
            std::lock_guard<std::mutex> lk(sOneShotMutex);
            over = sPendingOneShotFrees.size() > 256
                   || sOneShotStagingBytes.load() > byte_cap;
            if (over && !sPendingOneShotFrees.empty())
            {
                wait_entry = sPendingOneShotFrees.front();
                sPendingOneShotFrees.erase(sPendingOneShotFrees.begin());
                have_entry = true;
            }
        }
        if (!over)
        {
            break;
        }
        if (!have_entry)
        {
            break;
        }
        if (gpuTimelineValue() >= wait_entry.timeline_value)
        {
            if (wait_entry.buffer != VK_NULL_HANDLE || wait_entry.allocation != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(sAllocator, wait_entry.buffer, wait_entry.allocation);
            }
            sOneShotStagingBytes.fetch_sub(wait_entry.staging_bytes);
            std::lock_guard<std::mutex> lk(sOneShotMutex);
            if (wait_entry.cmd != VK_NULL_HANDLE)
            {
                sRetiredByPool[wait_entry.pool].push_back(wait_entry.cmd);
            }
        }
        else
        {
            {
                std::lock_guard<std::mutex> lk(sOneShotMutex);
                sPendingOneShotFrees.insert(sPendingOneShotFrees.begin(), wait_entry);
            }
            std::this_thread::sleep_for(std::chrono::microseconds(200)); // GPU 完了待ち(backpressure)
        }
        tickOneShotFreeQueue();
    }

    // fence 不要: 完了は timeline 値で判定する(GPU 完了 = timeline >= oneshot_value)。
    uint64_t oneshot_value;
    {
        PEJob job;
        job.cmd        = cmd;
        job.is_oneshot = true;
#if LL_DARWIN
        job.oneshot_source        = source;
        job.oneshot_staging_bytes = diagnostic_staging_bytes;
#endif
        oneshot_value  = peEnqueue(std::move(job));
    }

    PendingOneShotFree pending;
    pending.cmd            = cmd;
    pending.pool           = pool;
    pending.buffer         = staging_buffer;
    pending.allocation     = staging_allocation;
    pending.staging_bytes  = staging_bytes;
    pending.timeline_value = oneshot_value;
    sOneShotStagingBytes.fetch_add(staging_bytes);
    {
        std::lock_guard<std::mutex> lk(sOneShotMutex);
        sPendingOneShotFrees.push_back(pending);
    }
    return true;
}

void tickOneShotFreeQueue()
{
    if (sDevice == VK_NULL_HANDLE)
    {
        return;
    }
    std::vector<uint64_t> failed;
    {
        std::lock_guard<std::mutex> lk(sPEFailedMutex);
        failed.swap(sPEFailedOneShotValues);
    }
    std::vector<VkCommandBuffer> free_now;
    const VkCommandPool free_pool = threadCmdPool();
    const uint64_t cur = gpuTimelineValue();
    {
        std::lock_guard<std::mutex> lk(sOneShotMutex);
        size_t w = 0;
        const size_t n = sPendingOneShotFrees.size();
        for (size_t r = 0; r < n; ++r)
        {
            PendingOneShotFree& e = sPendingOneShotFrees[r];
            const bool submit_failed = !failed.empty() &&
                std::find(failed.begin(), failed.end(), e.timeline_value) != failed.end();
            if (sReapForceAll || submit_failed || cur >= e.timeline_value)
            {
                if (e.buffer != VK_NULL_HANDLE || e.allocation != VK_NULL_HANDLE)
                {
                    vmaDestroyBuffer(sAllocator, e.buffer, e.allocation);
                }
                sOneShotStagingBytes.fetch_sub(e.staging_bytes);
                if (e.cmd != VK_NULL_HANDLE)
                {
                    sRetiredByPool[e.pool].push_back(e.cmd);
                }
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
        auto own = sRetiredByPool.find(free_pool);
        if (own != sRetiredByPool.end())
        {
            free_now.swap(own->second);
        }
    }
    if (!free_now.empty() && free_pool != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(sDevice, free_pool, (U32)free_now.size(), free_now.data());
    }
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
            warnAttachmentAllocFail("createColorAttachmentImageVk", VK_RESULT_MAX_ENUM, width, height, format, mip_levels, 1);
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
        VkResult attach_r = vkCreateImageView(sDevice, &vci, nullptr, &attach_view);
        if (attach_r != VK_SUCCESS)
        {
            warnAttachmentAllocFail("createColorAttachmentImageVk", attach_r, width, height, format, mip_levels, 1);
            return false;
        }
        noteViewHandleCreated(attach_view);
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
            peSubmitBlocking(one_cmd, VK_NULL_HANDLE, true);
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

void createDepthLayerViews(VkImage img, VkFormat fmt, U32 layerCount,
                           std::vector<VkImageView>& out_layer_views)
{
    out_layer_views.clear();
    if (img == VK_NULL_HANDLE || sDevice == VK_NULL_HANDLE)
    {
        return;
    }
    for (U32 k = 0; k < layerCount; ++k)
    {
        VkImageViewCreateInfo vci = {};
        vci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image                           = img;
        vci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        vci.format                          = fmt;
        vci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        vci.subresourceRange.baseMipLevel   = 0;
        vci.subresourceRange.levelCount     = 1;
        vci.subresourceRange.baseArrayLayer = k;
        vci.subresourceRange.layerCount     = 1;

        VkImageView v = VK_NULL_HANDLE;
        if (vkCreateImageView(sDevice, &vci, nullptr, &v) == VK_SUCCESS)
        {
            noteViewHandleCreated(v);
            out_layer_views.push_back(v);
        }
    }
}

bool createLayeredDepthAttachmentImageVk(U32                       width,
                                         U32                       height,
                                         VkFormat                  format,
                                         U32                       layerCount,
                                         VkImage&                  out_image,
                                         VkImageView&              out_array_view,
                                         std::vector<VkImageView>& out_layer_views,
                                         void*&                    out_allocation)
{
    out_image      = VK_NULL_HANDLE;
    out_array_view = VK_NULL_HANDLE;
    out_layer_views.clear();
    out_allocation = nullptr;

    constexpr VkImageUsageFlags usage =
          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
        | VK_IMAGE_USAGE_SAMPLED_BIT
        | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (!createAttachmentImageVkImpl(width, height, format,
                                     usage,
                                     VK_IMAGE_ASPECT_DEPTH_BIT,
                                     "createLayeredDepthAttachmentImageVk",
                                     out_image, out_array_view, out_allocation,
                                     1, layerCount))
    {
        return false;
    }
    createDepthLayerViews(out_image, format, layerCount, out_layer_views);
    return true;
}

void destroyImageVk(VkImage image, VkImageView view, void* allocation)
{
    if (image == VK_NULL_HANDLE && view == VK_NULL_HANDLE && allocation == nullptr)
    {
        return;
    }
    ++gVkViewDestroyGen;
    PendingImageFree pending;
    pending.image         = image;
    pending.view          = view;
    pending.allocation    = reinterpret_cast<VmaAllocation>(allocation);
    pending.enqueue_frame = sInFrame ? sMonotonicFrameCount : (sMonotonicFrameCount + 1);
    {
        std::lock_guard<std::mutex> guard(sPendingImageFreeMutex);
        sPendingImageFrees.push_back(pending);
    }
    if (view != VK_NULL_HANDLE && vkValidationRequested())
    {
        std::lock_guard<std::mutex> lk(sSet1BirthMutex);
        ViewDeathInfo& di  = sViewDeathLedger[(U64)view];
        di.enqueue_frame   = sMonotonicFrameCount;
#if LL_WINDOWS
        di.enqueue_retaddr = (U64)(uintptr_t)_ReturnAddress();
#else
        di.enqueue_retaddr = (U64)(uintptr_t)__builtin_return_address(0);
#endif
    }
}

void tickDeferredImageFreeQueue()
{
    if (sDevice == VK_NULL_HANDLE && sAllocator == VK_NULL_HANDLE)
    {
        return;
    }
    std::unordered_set<U64> dead_views;

    std::vector<PendingImageFree> to_free;
    {
        std::lock_guard<std::mutex> guard(sPendingImageFreeMutex);
        size_t w = 0;
        const size_t n = sPendingImageFrees.size();
        for (size_t r = 0; r < n; ++r)
        {
            PendingImageFree& e = sPendingImageFrees[r];
            if (reapReady(e.enqueue_frame))
            {
                to_free.push_back(e);
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

    for (PendingImageFree& e : to_free)
    {
        if (e.view != VK_NULL_HANDLE && sDevice != VK_NULL_HANDLE)
        {
            vkDestroyImageView(sDevice, e.view, nullptr);
            dead_views.insert((U64)e.view);
            {
                std::lock_guard<std::mutex> lk(sDeadHandleMutex);
                sDeadViewHandles.insert((U64)e.view);
            }
            if (vkValidationRequested())
            {
                std::lock_guard<std::mutex> lk(sSet1BirthMutex);
                sViewDeathLedger[(U64)e.view].destroy_frame = sMonotonicFrameCount;
            }
        }
        if (e.image != VK_NULL_HANDLE && sAllocator != VK_NULL_HANDLE)
        {
            vmaDestroyImage(sAllocator, e.image, e.allocation);
        }
    }

    purgePerDrawDeadHandles(dead_views, {});

    {
        std::lock_guard<std::mutex> guard(sBindlessSlotMutex);
        size_t sw = 0;
        const size_t sn = sPendingSlotFrees.size();
        for (size_t r = 0; r < sn; ++r)
        {
            PendingSlotFree& e = sPendingSlotFrees[r];
            if (reapReady(e.enqueue_frame))
            {
                bindlessWriteSlotInternal(e.slot, VK_NULL_HANDLE, VK_NULL_HANDLE);
                sBindlessSlotFreeList.push_back(e.slot);
            }
            else
            {
                if (sw != r)
                {
                    sPendingSlotFrees[sw] = e;
                }
                ++sw;
            }
        }
        sPendingSlotFrees.resize(sw);
    }

    drawDataReclaimDomain(&sMainDomain);
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
    return created;
}

bool canGenerateMipChainBlitVk(VkFormat format)
{
    if (sPhysicalDevice == VK_NULL_HANDLE || format == VK_FORMAT_UNDEFINED)
    {
        return false;
    }

    VkFormatProperties fp = {};
    vkGetPhysicalDeviceFormatProperties(sPhysicalDevice, format, &fp);
    const VkFormatFeatureFlags required = VK_FORMAT_FEATURE_BLIT_SRC_BIT
                                        | VK_FORMAT_FEATURE_BLIT_DST_BIT
                                        | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
    return (fp.optimalTilingFeatures & required) == required;
}

static bool uploadImageDataVkImpl(VkImage     image,
                                  U32         width,
                                  U32         height,
                                  const void* data,
                                  U32         data_size_bytes,
                                  U32         mip_level,
                                  U32         generate_mip_count,
                                  VkFormat    generate_mip_format)
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
    const bool generate_mip_chain = generate_mip_count > 1;
    if (generate_mip_chain
        && (mip_level != 0 || !canGenerateMipChainBlitVk(generate_mip_format)))
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

    VkCommandBuffer cmd = beginOneShotCommandBufferVk();
    if (cmd == VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        return false;
    }

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
        region.imageSubresource.mipLevel       = mip_level;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;
        region.imageOffset                     = {0, 0, 0};
        region.imageExtent                     = {width, height, 1};
        vkCmdCopyBufferToImage(cmd, staging_buffer, image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    if (!generate_mip_chain)
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
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }
    else
    {
        auto mip_barrier = [&](U32 level, VkImageLayout old_layout, VkImageLayout new_layout,
                               VkAccessFlags src_access, VkAccessFlags dst_access,
                               VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage)
        {
            VkImageMemoryBarrier b = {};
            b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            b.srcAccessMask                   = src_access;
            b.dstAccessMask                   = dst_access;
            b.oldLayout                       = old_layout;
            b.newLayout                       = new_layout;
            b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            b.image                           = image;
            b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            b.subresourceRange.baseMipLevel   = level;
            b.subresourceRange.levelCount     = 1;
            b.subresourceRange.baseArrayLayer = 0;
            b.subresourceRange.layerCount     = 1;
            vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &b);
        };

        S32 mip_w = (S32)width;
        S32 mip_h = (S32)height;
        for (U32 i = 1; i < generate_mip_count; ++i)
        {
            const S32 dst_w = (mip_w > 1) ? (mip_w / 2) : 1;
            const S32 dst_h = (mip_h > 1) ? (mip_h / 2) : 1;

            mip_barrier(i - 1,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
            mip_barrier(i,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        0, VK_ACCESS_TRANSFER_WRITE_BIT,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            VkImageBlit blit = {};
            blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel       = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount     = 1;
            blit.srcOffsets[1]                 = { mip_w, mip_h, 1 };
            blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel       = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount     = 1;
            blit.dstOffsets[1]                 = { dst_w, dst_h, 1 };
            vkCmdBlitImage(cmd,
                           image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &blit, VK_FILTER_LINEAR);

            mip_barrier(i - 1,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            mip_w = dst_w;
            mip_h = dst_h;
        }

        mip_barrier(generate_mip_count - 1,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    vkEndCommandBuffer(cmd);

    return submitOneShotVk(cmd, staging_buffer, staging_allocation,
                           generate_mip_chain ? "image-upload-mips-2d" : "image-upload-2d",
                           data_size_bytes);
}

bool uploadImageDataVk(VkImage     image,
                       U32         width,
                       U32         height,
                       const void* data,
                       U32         data_size_bytes,
                       U32         mip_level)
{
    return uploadImageDataVkImpl(image, width, height, data, data_size_bytes,
                                 mip_level, 1, VK_FORMAT_UNDEFINED);
}

bool uploadImageDataMipChainVk(VkImage     image,
                               U32         width,
                               U32         height,
                               const void* data,
                               U32         data_size_bytes,
                               U32         mip_count,
                               VkFormat    format)
{
    return uploadImageDataVkImpl(image, width, height, data, data_size_bytes,
                                 0, mip_count, format);
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

    if (!canGenerateMipChainBlitVk(format))
    {
        return false;
    }

    VkCommandBuffer cmd = beginOneShotCommandBufferVk();
    if (cmd == VK_NULL_HANDLE)
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

    return submitOneShotVk(cmd, VK_NULL_HANDLE, VK_NULL_HANDLE, "image-mip-blit");
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

    VkCommandBuffer cmd = beginOneShotCommandBufferVk();
    if (cmd == VK_NULL_HANDLE)
    {
        destroyImageVk(new_image, new_view, new_alloc);
        return false;
    }

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

    if (!submitOneShotVk(cmd, VK_NULL_HANDLE, VK_NULL_HANDLE, "image-downscale"))
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

    VkCommandBuffer cmd = beginOneShotCommandBufferVk();
    if (cmd == VK_NULL_HANDLE)
    {
        return false;
    }

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

    return submitOneShotVk(cmd, VK_NULL_HANDLE, VK_NULL_HANDLE, "cube-array-blit");
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

    if (!canGenerateMipChainBlitVk(format))
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
    noteViewHandleCreated(view);

    out_image      = image;
    out_view       = view;
    out_allocation = reinterpret_cast<void*>(allocation);
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

    VkCommandBuffer cmd = beginOneShotCommandBufferVk();
    if (cmd == VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        return false;
    }

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

    return submitOneShotVk(cmd, staging_buffer, staging_allocation,
                           "image-upload-3d", data_size_bytes);
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

    VkCommandBuffer cmd = beginOneShotCommandBufferVk();
    if (cmd == VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        return false;
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
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    vkEndCommandBuffer(cmd);

    return submitOneShotVk(cmd, staging_buffer, staging_allocation,
                           "image-subregion-upload", staging_size);
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
    noteViewHandleCreated(view);

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

    VkCommandBuffer cmd = beginOneShotCommandBufferVk();
    if (cmd == VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        return false;
    }

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

    return submitOneShotVk(cmd, staging_buffer, staging_allocation,
                           "cube-upload", total_size);
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
        LL_WARNS("Vulkan") << "cube-array image prerequisites missing: allocator="
                           << (sAllocator != VK_NULL_HANDLE) << " device=" << (sDevice != VK_NULL_HANDLE)
                           << " command_pool=" << (sCommandPool != VK_NULL_HANDLE)
                           << " graphics_queue=" << (sGraphicsQueue != VK_NULL_HANDLE) << LL_ENDL;
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
        LL_WARNS("Vulkan") << "vmaCreateImage for cube-array fallback failed result=" << r << LL_ENDL;
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
        LL_WARNS("Vulkan") << "vkCreateImageView for cube-array fallback failed result=" << r << LL_ENDL;
        vmaDestroyImage(sAllocator, image, allocation);
        return false;
    }
    noteViewHandleCreated(view);

    VkCommandBuffer cmd = beginOneShotCommandBufferVk();
    if (cmd != VK_NULL_HANDLE)
    {
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
        peSubmitBlocking(cmd, VK_NULL_HANDLE, true);
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

bool createReadbackBufferVk(U32       bytes,
                            VkBuffer& out_buffer,
                            void*&    out_allocation,
                            void*&    out_mapped)
{
    out_buffer     = VK_NULL_HANDLE;
    out_allocation = nullptr;
    out_mapped     = nullptr;
    if (bytes == 0 || sAllocator == VK_NULL_HANDLE)
    {
        return false;
    }

    VkBufferCreateInfo bci = {};
    bci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size        = bytes;
    bci.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo aci = {};
    aci.usage         = VMA_MEMORY_USAGE_AUTO;
    aci.flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
                      | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    aci.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VkBuffer      buffer     = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo info = {};
    VkResult r = vmaCreateBuffer(sAllocator, &bci, &aci, &buffer, &allocation, &info);
    if (r != VK_SUCCESS || buffer == VK_NULL_HANDLE || info.pMappedData == nullptr)
    {
        if (buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(sAllocator, buffer, allocation);
        }
        return false;
    }
    out_buffer     = buffer;
    out_allocation = allocation;
    out_mapped     = info.pMappedData;
    return true;
}

static VkBufferMemoryBarrier readbackDstWawBarrier(VkBuffer buffer)
{
    VkBufferMemoryBarrier b = {};
    b.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    b.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
    b.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.buffer              = buffer;
    b.offset              = 0;
    b.size                = VK_WHOLE_SIZE;
    return b;
}

bool copyColorImageRegionToBufferVk(VkImage       src_image,
                                    VkImageLayout src_layout,
                                    S32           src_x,
                                    S32           src_y,
                                    U32           width,
                                    U32           height,
                                    VkBuffer      dst_buffer)
{
    if (src_image == VK_NULL_HANDLE || dst_buffer == VK_NULL_HANDLE ||
        width == 0 || height == 0 || src_x < 0 || src_y < 0)
    {
        return false;
    }
    if (src_layout == VK_IMAGE_LAYOUT_UNDEFINED)
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
        VkBufferMemoryBarrier bufb = readbackDstWawBarrier(dst_buffer);
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 1, &bufb, 1, &b);
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
        region.imageOffset                     = {src_x, src_y, 0};
        region.imageExtent                     = {width, height, 1};
        vkCmdCopyImageToBuffer(cmd, src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               dst_buffer, 1, &region);
    }

    {
        VkImageMemoryBarrier b = {};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
        b.dstAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
        b.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        b.newLayout                       = src_layout;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = src_image;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
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

    VkCommandBuffer cmd = beginOneShotCommandBufferVk();
    if (cmd == VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        return false;
    }

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
        VkBufferMemoryBarrier bufb = readbackDstWawBarrier(staging_buffer);
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 1, &bufb, 1, &b);
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

    VkResult sr = peSubmitBlocking(cmd, VK_NULL_HANDLE, true);
    if (sr != VK_SUCCESS)
    {
        vkFreeCommandBuffers(sDevice, sCommandPool, 1, &cmd);
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        return false;
    }
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
                                F32*          out_depth,
                                U32           array_layer)
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

    VkCommandBuffer cmd = beginOneShotCommandBufferVk();
    if (cmd == VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        return false;
    }

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
        b.subresourceRange.baseArrayLayer = array_layer;
        b.subresourceRange.layerCount     = 1;
        VkBufferMemoryBarrier bufb = readbackDstWawBarrier(staging_buffer);
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 1, &bufb, 1, &b);
    }

    {
        VkBufferImageCopy region = {};
        region.bufferOffset                    = 0;
        region.bufferRowLength                 = 0;
        region.bufferImageHeight               = 0;
        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = array_layer;
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
        b.subresourceRange.baseArrayLayer = array_layer;
        b.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b);
    }

    vkEndCommandBuffer(cmd);

    VkResult sr = peSubmitBlocking(cmd, VK_NULL_HANDLE, true);
    if (sr != VK_SUCCESS)
    {
        vkFreeCommandBuffers(sDevice, sCommandPool, 1, &cmd);
        vmaDestroyBuffer(sAllocator, staging_buffer, staging_allocation);
        return false;
    }
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

void transitionImageLayoutVk(VkImage              image,
                             VkImageAspectFlags   aspect_mask,
                             VkImageLayout        old_layout,
                             VkImageLayout        new_layout,
                             VkPipelineStageFlags src_stage_mask,
                             VkPipelineStageFlags dst_stage_mask,
                             VkAccessFlags        src_access_mask,
                             VkAccessFlags        dst_access_mask,
                             U32                  layer_count,
                             U32                  level_count)
{
    if (!sInitialized || image == VK_NULL_HANDLE)
    {
        return;
    }

    VkCommandBuffer rec_cmd = currentRecordCmd();
    if (rec_cmd == VK_NULL_HANDLE)
    {
        if (sDevice == VK_NULL_HANDLE || sCommandPool == VK_NULL_HANDLE ||
            sGraphicsQueue == VK_NULL_HANDLE)
        {
            return;
        }
        VkCommandBuffer oneshot_cmd = beginOneShotCommandBufferVk();
        if (oneshot_cmd == VK_NULL_HANDLE)
        {
            return;
        }
        VkImageMemoryBarrier oneshot_barrier = {};
        oneshot_barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        oneshot_barrier.oldLayout                       = old_layout;
        oneshot_barrier.newLayout                       = new_layout;
        oneshot_barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        oneshot_barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        oneshot_barrier.image                           = image;
        oneshot_barrier.subresourceRange.aspectMask     = aspect_mask;
        oneshot_barrier.subresourceRange.baseMipLevel   = 0;
        oneshot_barrier.subresourceRange.levelCount     = level_count;
        oneshot_barrier.subresourceRange.baseArrayLayer = 0;
        oneshot_barrier.subresourceRange.layerCount     = layer_count;
        oneshot_barrier.srcAccessMask                   = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        oneshot_barrier.dstAccessMask                   = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        vkCmdPipelineBarrier(oneshot_cmd,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &oneshot_barrier);
        vkEndCommandBuffer(oneshot_cmd);
        submitOneShotVk(oneshot_cmd, VK_NULL_HANDLE, VK_NULL_HANDLE,
                         "image-layout-transition");
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
    barrier.subresourceRange.levelCount     = level_count;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = layer_count;
    barrier.srcAccessMask                   = src_access_mask;
    barrier.dstAccessMask                   = dst_access_mask;

    const bool was_rendering = sInDynamicRendering;
    VkPipelineStageFlags rec_src_stage = src_stage_mask;
    VkPipelineStageFlags rec_dst_stage = dst_stage_mask;
    VkMemoryBarrier      attachment_sync = {};
    U32                  mem_barrier_count = 0;
    if (was_rendering)
    {
        vkCmdEndRendering(rec_cmd);
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

    vkCmdPipelineBarrier(rec_cmd,
                         rec_src_stage, rec_dst_stage,
                         0,
                         mem_barrier_count, mem_barrier_count ? &attachment_sync : nullptr,
                         0, nullptr,
                         1, &barrier);

    if (was_rendering)
    {
        resumeSavedPass();
    }
}

VkImageView getDefaultFallbackVkImageView()
{
    return sDefaultFallbackImageView;
}

VkImageView getWhiteVkImageView()
{
    return sWhiteImageView;
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

VkImageView getDefaultFallbackShadowVkImageView()
{
    return sDefaultFallbackShadowImageView;
}

} // namespace LLVKLoader
