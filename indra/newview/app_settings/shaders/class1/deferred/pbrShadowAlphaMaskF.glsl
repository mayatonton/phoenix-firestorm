/**
 * @file pbrShadowAlphaMaskF.glsl
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
layout(location=0) out vec4 frag_color;
#else
out vec4 frag_color;
#endif

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=1) uniform sampler2D diffuseMap;
#else
uniform sampler2D diffuseMap;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=15) in vec4 post_pos;
#else
in vec4 post_pos;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=8) in float target_pos_x;
#else
in float target_pos_x;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=2) in vec4 vertex_color;
#else
in vec4 vertex_color;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=0) in vec2 vary_texcoord0;
#else
in vec2 vary_texcoord0;
#endif
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-1: FrameAtmosphere guard wrap (B?-ζ §3.1 範式)
#ifndef FRAME_ATMOSPHERE_DEFINED
#define FRAME_ATMOSPHERE_DEFINED 1
layout(set=0, binding=2, std140) uniform FrameAtmosphere {
    vec3  sunlight_color;
    float scene_light_strength;
    vec3  moonlight_color;
    float haze_density;
    vec3  ambient_color;
    float density_multiplier;
    vec3  blue_horizon;
    float distance_multiplier;
    vec3  blue_density;
    float max_y;
    vec3  glow;
    float sky_sunlight_scale;
    float sky_ambient_scale;
    float sky_hdr_scale;
    int   classic_mode;
    int   cube_snapshot;
    float minimum_alpha;
    float max_cof;
    float _pad_atm0;
    float _pad_atm1;
};
#endif
#else
uniform float minimum_alpha;
#endif

void main()
{
    float alpha = texture(diffuseMap,vary_texcoord0.xy).a * vertex_color.a;

    if (alpha < minimum_alpha)
    {
        discard;
    }

    frag_color = vec4(1,1,1,1);
}
