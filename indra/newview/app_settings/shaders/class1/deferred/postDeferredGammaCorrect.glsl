/**
 * @file postDeferredGammaCorrect.glsl
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

out vec4 frag_color;

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=4) uniform sampler2D diffuseRect;
#else
uniform sampler2D diffuseRect;
#endif

#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-24 Phase D: FrameAtmosphere_Skybox 削除
// (η-14 Path G 範式継承)。FrameAtmosphere_Lighting (set=0 binding=2 from
// atmosphericsHelpersV/atmosphericsFuncs) と member 名 sunlight_color 等 17 件
// が global scope で重複 = nameless block name collision。この file の main() で
// 必要な Skybox 単独 member は `gamma` のみ。新規 PerProgramUBO_GammaCorrect
// (set=2, binding=2、η-3 §3.2 PerDrawUBO 範式類) で gamma 専用 UBO を declare。
#ifndef PER_PROGRAM_UBO_GAMMA_CORRECT_DEFINED
#define PER_PROGRAM_UBO_GAMMA_CORRECT_DEFINED 1
layout(set=2, binding=2, std140) uniform PerProgramUBO_GammaCorrect {
    float gamma;
    float _pad_gc0;
    float _pad_gc1;
    float _pad_gc2;
};
#endif
#else
uniform float gamma;
#endif
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-5: FrameViewProj guard wrap (η-1 §3.1 範式継承)
#ifndef FRAME_VIEW_PROJ_DEFINED
#define FRAME_VIEW_PROJ_DEFINED 1
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
#endif
#else
uniform vec2 screen_res;
#endif
#ifndef DECL_VARY_FRAGCOORD
#define DECL_VARY_FRAGCOORD
in vec2 vary_fragcoord;
#endif // DECL_VARY_FRAGCOORD

vec3 linear_to_srgb(vec3 cl);

vec3 legacyGamma(vec3 color)
{
    vec3 c = 1. - clamp(color, vec3(0.), vec3(1.));
    c = 1. - pow(c, vec3(gamma)); // s/b inverted already CPU-side

    return c;
}

void main()
{
    //this is the one of the rare spots where diffuseRect contains linear color values (not sRGB)
    vec4 diff = texture(diffuseRect, vary_fragcoord);
    diff.rgb = linear_to_srgb(diff.rgb);

#ifdef LEGACY_GAMMA
    diff.rgb = legacyGamma(diff.rgb);
#endif

    diff.rgb = clamp(diff.rgb, vec3(0.0), vec3(1.0));
    frag_color = diff;
}

