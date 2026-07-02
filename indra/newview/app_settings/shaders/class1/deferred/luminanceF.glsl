/**
 * @file luminanceF.glsl
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


/*[EXTRA_CODE_HERE]*/

// take a luminance sample of diffuseRect and emissiveRect

#ifdef LL_VULKAN_GLSL
layout(location = 0) out vec4 frag_color;

layout(location = 0) in vec2 vary_fragcoord;
#else
out vec4 frag_color;

in vec2 vary_fragcoord;
#endif

#ifdef LL_VULKAN_GLSL
layout(set = 1, binding = 1) uniform sampler2D diffuseRect;
layout(set = 1, binding = 2) uniform sampler2D emissiveRect;
#ifndef DECL_NORMAL_MAP
#define DECL_NORMAL_MAP
layout(set = 1, binding = 3) uniform sampler2D normalMap;
#endif // DECL_NORMAL_MAP
#else
uniform sampler2D diffuseRect;
uniform sampler2D emissiveRect;
#ifndef DECL_NORMAL_MAP
#define DECL_NORMAL_MAP
uniform sampler2D normalMap;
#endif // DECL_NORMAL_MAP
#endif

#ifdef LL_VULKAN_GLSL
layout(set = 1, binding = 0, std140) uniform LuminanceF_PerProgramBind
{
    float diffuse_luminance_scale;
#ifndef _AYA_UM__pad0
#define _AYA_UM__pad0 1
    float _pad0;
#else
    float _dup_LuminanceF__pad0;
#endif
#ifndef _AYA_UM__pad1
#define _AYA_UM__pad1 1
    float _pad1;
#else
    float _dup_LuminanceF__pad1;
#endif
#ifndef _AYA_UM__pad2
#define _AYA_UM__pad2 1
    float _pad2;
#else
    float _dup_LuminanceF__pad2;
#endif
};
#else
uniform float diffuse_luminance_scale;
#endif

float lum(vec3 col)
{
    vec3 l = vec3(0.2126, 0.7152, 0.0722);
    return dot(l, col);
}

void main()
{
    vec2 tc = vary_fragcoord*0.6+0.2;
    tc.y -= 0.1; // HACK - nudge exposure sample down a little bit to favor ground over sky
    vec3 c = texture(diffuseRect, tc).rgb;

    vec4  norm         = texture(normalMap, tc);

    if (!GET_GBUFFER_FLAG(norm.w, GBUFFER_FLAG_HAS_HDRI) &&
        !GET_GBUFFER_FLAG(norm.w, GBUFFER_FLAG_SKIP_ATMOS))
    {
        // Apply the diffuse luminance scale to objects but not the sky
        // Prevents underexposing when looking at bright environments
        // while still allowing for realistically bright skies.
        c *= diffuse_luminance_scale;
    }

    c += texture(emissiveRect, tc).rgb;

    float L = lum(c);
    frag_color = vec4(max(L, 0.0));
}

