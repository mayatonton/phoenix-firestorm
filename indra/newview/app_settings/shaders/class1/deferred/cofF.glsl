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
layout(location = 0) out vec4 frag_color;
#else
out vec4 frag_color;
#endif

#ifdef LL_VULKAN_GLSL
layout(set = 1, binding = 1) uniform sampler2D diffuseRect;
#ifndef DECL_DEPTH_MAP
#define DECL_DEPTH_MAP
layout(set = 1, binding = 2) uniform sampler2D depthMap;
#endif // DECL_DEPTH_MAP
#else
uniform sampler2D diffuseRect;
#ifndef DECL_DEPTH_MAP
#define DECL_DEPTH_MAP
uniform sampler2D depthMap;
#endif // DECL_DEPTH_MAP
#endif

#ifdef LL_VULKAN_GLSL
#ifndef PER_FRAME_MATRIX_UBO_DEFINED
#define PER_FRAME_MATRIX_UBO_DEFINED 1
layout(set = 0, binding = 0, std140) uniform PerFrameMatrixUBO
{
    mat4 projection_matrix;
    mat4 inverse_projection_matrix;
    mat4 identity_matrix;
    mat4 last_modelview_matrix;
};
#define inv_proj inverse_projection_matrix

#endif // PER_FRAME_MATRIX_UBO_DEFINED
layout(set = 1, binding = 0, std140) uniform CofF_PerProgramBind
{
    float focal_distance;
    float blur_constant;
    float tan_pixel_angle;
    float magnification;
#ifndef _AYA_UM_max_cof
#define _AYA_UM_max_cof 1
    float max_cof;
#else
    float _dup_CofF_max_cof;
#endif
#ifndef _AYA_UM__pad0
#define _AYA_UM__pad0 1
    float _pad0;
#else
    float _dup_CofF__pad0;
#endif
#ifndef _AYA_UM__pad1
#define _AYA_UM__pad1 1
    float _pad1;
#else
    float _dup_CofF__pad1;
#endif
#ifndef _AYA_UM__pad2
#define _AYA_UM__pad2 1
    float _pad2;
#else
    float _dup_CofF__pad2;
#endif
};
#else
uniform float depth_cutoff;
uniform float norm_cutoff;
uniform float focal_distance;
uniform float blur_constant;
uniform float tan_pixel_angle;
uniform float magnification;
uniform float max_cof;

uniform mat4 inv_proj;
uniform vec2 screen_res;
#endif

#ifdef LL_VULKAN_GLSL
layout(location = 0) in vec2 vary_fragcoord;
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
