/**
* @file llvksampler.cpp
* @brief AYAstorm r42 Vulkan loader — samplers / GL-enum to VK conversions
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

namespace LLVKLoaderInternal
{

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
} // namespace LLVKLoaderInternal

namespace
{

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
} // namespace

namespace LLVKLoaderInternal
{

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
} // namespace LLVKLoaderInternal

namespace
{

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
} // namespace


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

} // namespace LLVKLoader
