/**
 * @file indexedTextureV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2007, Linden Research, Inc.
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

// indexedTextureV cluster helper layout(location = N) in/out 注入
#ifdef LL_VULKAN_GLSL
layout(location = 13) in int texture_index;

layout(location = 15) flat out int vary_texture_index;
#else
in int texture_index;

flat out int vary_texture_index;
#endif

#ifdef AYA_BINDLESS
layout(location = 19) flat out int aya_draw_id;
#endif

void passTextureIndex()
{
    vary_texture_index = texture_index;
#ifdef AYA_BINDLESS
    aya_draw_id = gl_InstanceIndex;
#endif
}

