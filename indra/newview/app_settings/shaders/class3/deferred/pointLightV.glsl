/**
 * @file class3\deferred\pointLightV.glsl
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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
uniform mat4 modelview_projection_matrix;
uniform mat4 modelview_matrix;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=0) in vec3 position;
#else
in vec3 position;
#endif

#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-25 Phase 1c: center / size bare uniform を
// PerProgramUBO_PointLightV (set=2, binding=5) に集約。main() で position*size+center として
// vertex 変換に使用、dead でないため η-23 §3.1 GL-only wrap 不適用、η-24 §3.2
// PerProgramUBO_GammaCorrect 派生範式類。set=2 namespace 連番継続
// (η-24 binding=2 GammaCorrect / η-25 binding=3 AlphaParams / η-25 binding=4 ColorGrading /
//  η-25 binding=5 PointLightV)。vec3 + float = 16-byte (1 vec4 chunk) std140 整合。
// Deferred Light + Deferred SpotLight 2 program 共通 vertex source 救済。
#ifndef PER_PROGRAM_UBO_POINT_LIGHT_V_DEFINED
#define PER_PROGRAM_UBO_POINT_LIGHT_V_DEFINED 1
layout(set=2, binding=5, std140) uniform PerProgramUBO_PointLightV {
    vec3  center;
    float size;
};
#endif
#else
uniform vec3 center;
uniform float size;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=1) out vec4 vary_fragcoord;
#else
out vec4 vary_fragcoord;
#endif
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-25 Phase 1d (第23層 cascade): trans_center
// location 20 → 60 移動 (Phase 1c で center/size bare uniform を UBO 化 → parser advance
// → atmosphericsVarsV.glsl vary_AdditiveColor location=20 と overlap 露呈、η-18 §3.1
// 50-59 帯使用済のため 60 起点)。V↔F 鎖 3 file (pointLightV/pointLightF/spotLightF) 同期。
layout(location=60) out vec3 trans_center;
#else
out vec3 trans_center;
#endif

void main()
{
    //transform vertex
    vec3 p = position*size+center;
    vec4 pos = modelview_projection_matrix * vec4(p.xyz, 1.0);
    vary_fragcoord = pos;
    trans_center = (modelview_matrix*vec4(center.xyz, 1.0)).xyz;
    gl_Position = pos;
}
