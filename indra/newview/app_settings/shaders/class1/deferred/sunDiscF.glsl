/**
 * @file sunDiscF.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2005, Linden Research, Inc.
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

#ifdef LL_VULKAN_GLSL
layout(location = 0) out vec4 frag_data[4];
#else
out vec4 frag_data[4];
#endif

vec3 srgb_to_linear(vec3 c);

#ifdef LL_VULKAN_GLSL
layout(set = 1, binding = 0, std140) uniform SunDiscF_PerProgramBind
{
#ifndef _AYA_UM_blend_factor
#define _AYA_UM_blend_factor 1
    float blend_factor; // interp factor between sunDisc A/B
#else
    float _dup_SunDiscF_blend_factor;
#endif
    float _sunDiscF_pad0;
    float _sunDiscF_pad1;
    float _sunDiscF_pad2;
};
layout(set = 1, binding = 1) uniform sampler2D diffuseMap;
layout(set = 1, binding = 2) uniform sampler2D altDiffuseMap;
#else
uniform sampler2D diffuseMap;
uniform sampler2D altDiffuseMap;
uniform float blend_factor; // interp factor between sunDisc A/B
#endif

#ifdef LL_VULKAN_GLSL
layout(location = 0) in vec2 vary_texcoord0;
layout(location = 1) in float sun_fade;
#else
in vec2 vary_texcoord0;
in float sun_fade;
#endif

void main()
{
    vec4 sunDiscA = texture(diffuseMap, vary_texcoord0.xy);
    vec4 sunDiscB = texture(altDiffuseMap, vary_texcoord0.xy);
    vec4 c     = mix(sunDiscA, sunDiscB, blend_factor);


    // SL-9806 stars poke through
    //c.a *= sun_fade;

    frag_data[0] = vec4(0);
    frag_data[1] = vec4(0.0f);
    frag_data[2] = vec4(0.0, 1.0, 0.0, GBUFFER_FLAG_SKIP_ATMOS);
#if defined(HAS_EMISSIVE)
    frag_data[0] = vec4(0);
    frag_data[3] = c;
#else
    frag_data[0] = c;
#endif
}

