/**
 * @file copyF.glsl
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

#ifdef LL_VULKAN_GLSL
layout(location=20) in vec2 tc;
#else
in vec2 tc;
#endif

#if defined(COPY_DEPTH)
#ifndef DECL_DEPTH_MAP
#define DECL_DEPTH_MAP
#ifdef LL_VULKAN_GLSL
layout(set=0, binding=3) uniform sampler2D depthMap;
#else
uniform sampler2D depthMap;
#endif
#endif // DECL_DEPTH_MAP
#endif

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=1) uniform sampler2D diffuseMap;
#else
uniform sampler2D diffuseMap;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=0) out vec4 frag_color;
#else
out vec4 frag_color;
#endif

void main()
{
    frag_color = texture(diffuseMap, tc);
#if defined(COPY_DEPTH)
    gl_FragDepth = texture(depthMap, tc).r;
#endif
}

