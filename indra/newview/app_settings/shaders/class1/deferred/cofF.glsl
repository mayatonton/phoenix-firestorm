/**
 * @file cofF.glsl
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

/*[EXTRA_CODE_HERE]*/

#ifdef LL_VULKAN_GLSL
layout(location=0) out vec4 frag_color;
#else
out vec4 frag_color;
#endif

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=4) uniform sampler2D diffuseRect;
#else
uniform sampler2D diffuseRect;
#endif
#ifdef LL_VULKAN_GLSL
layout(set=0, binding=3) uniform sampler2D depthMap;
#else
uniform sampler2D depthMap;
#endif

uniform float depth_cutoff;
uniform float norm_cutoff;
uniform float focal_distance;
uniform float blur_constant;
uniform float tan_pixel_angle;
uniform float magnification;
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
uniform float max_cof;
#endif

#ifdef LL_VULKAN_GLSL
layout(set=0, binding=0, std140) uniform FrameViewProj {
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    mat4 projection_matrix;
    mat4 inv_proj;
    mat4 proj_mat;
    mat4 last_modelview_matrix;
    mat3 env_mat;
    mat3 normal_matrix;
    vec2 screen_res;
};
#else
uniform mat4 inv_proj;
uniform vec2 screen_res;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=1) in vec2 vary_fragcoord;
#else
in vec2 vary_fragcoord;
#endif

float calc_cof(float depth)
{
    float sc = (depth-focal_distance)/-depth*blur_constant;

    sc /= magnification;

    // tan_pixel_angle = pixel_length/-depth;
    float pixel_length =  tan_pixel_angle*-focal_distance;

    sc = sc/pixel_length;
    sc *= 1.414;

    return sc;
}

void main()
{
    vec2 tc = vary_fragcoord.xy;

    float z = texture(depthMap, tc).r;
    z = z*2.0-1.0;
    vec4 ndc = vec4(0.0, 0.0, z, 1.0);
    vec4 p = inv_proj*ndc;
    float depth = p.z/p.w;

    vec4 diff = texture(diffuseRect, vary_fragcoord.xy);

    float sc = calc_cof(depth);
    sc = min(sc, max_cof);
    sc = max(sc, -max_cof);

    frag_color.rgb = diff.rgb;
    frag_color.a = sc/max_cof*0.5+0.5;
}
