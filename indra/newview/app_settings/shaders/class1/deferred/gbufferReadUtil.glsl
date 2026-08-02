/**
 * @file class1/deferred/gbufferReadUtil.glsl
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

#ifdef LL_VULKAN_GLSL
#ifndef DECL_NORMAL_MAP
#define DECL_NORMAL_MAP
layout(set = 1, binding = 27) uniform sampler2D normalMap;
#endif // DECL_NORMAL_MAP
#ifndef DECL_DEPTH_MAP
#define DECL_DEPTH_MAP
layout(set = 1, binding = 24) uniform sampler2D depthMap;
#endif // DECL_DEPTH_MAP
#else
#ifndef DECL_NORMAL_MAP
#define DECL_NORMAL_MAP
uniform sampler2D normalMap;
#endif // DECL_NORMAL_MAP
#ifndef DECL_DEPTH_MAP
#define DECL_DEPTH_MAP
uniform sampler2D depthMap;
#endif // DECL_DEPTH_MAP
#endif

vec4 decodeNormal(vec4 norm);
vec2 getScreenCoordinate(vec2 screenpos);

vec4 getNorm(vec2 screenpos)
{
    vec4 norm = decodeNormal(texture(normalMap, screenpos.xy));
    return norm;
}

vec4 getNormRaw(vec2 screenpos)
{
    vec4 norm = texture(normalMap, screenpos.xy);
    return norm;
}

float getDepth(vec2 pos_screen)
{
    float depth = texture(depthMap, pos_screen).r;
    return depth;
}

vec4 getPosition(vec2 pos_screen)
{
    float depth = getDepth(pos_screen);
    vec2 sc = getScreenCoordinate(pos_screen);
    vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
    vec4 pos = inv_proj * ndc;
    pos /= pos.w;
    pos.w = 1.0;
    return pos;
}
