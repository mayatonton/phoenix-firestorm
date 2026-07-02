/**
 * @file llgltypes.h
 * @brief LLGL definition
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LLGLTYPES_H
#define LLGLTYPES_H

#define MAX_GL_TEXTURE_UNITS 16

typedef U32 LLGLenum;
typedef U32 LLGLuint;
typedef S32 LLGLint;
typedef F32 LLGLfloat;
typedef F64 LLGLdouble;
typedef U8 LLGLboolean;

// AYAstorm r41 段階 1: Vulkan 型 alias 並走追加 (placeholder typedef)
// 段階 2-5 の per-file port で type 一貫性確保 (charter §2 領域 2-5)
#include "volk.h"

typedef VkBuffer        LLVkBuffer;
typedef VkImage         LLVkImage;
typedef VkImageView     LLVkImageView;
typedef VkDeviceMemory  LLVkDeviceMemory;
typedef VkSampler       LLVkSampler;
typedef VkShaderModule  LLVkShaderModule;
typedef VkPipeline      LLVkPipeline;
typedef VkFormat        LLVkFormat;

#endif
