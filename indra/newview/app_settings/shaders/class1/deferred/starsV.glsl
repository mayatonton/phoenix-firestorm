/**
 * @file starsV.glsl
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
layout(set=1, binding=0, std140) uniform MaterialUBO {
    mat4  texture_matrix0;
    vec4  texture_base_color_transform[2];
    vec4  texture_emissive_transform[2];
    vec4  color;
    vec3  emissiveColor;
    float _pad_emissive;
};
#else
uniform mat4 texture_matrix0;
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
uniform mat4 modelview_projection_matrix;
#endif
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-6: starsV non-opaque uniforms UBO wrap (Cluster F)
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-20: anonymous member `time` が starsF.glsl の
// StarsFParamUBO_Legacy.time (offset=8) と name 衝突 → glslang link 失敗 (Deferred Star Program)。
// Phase 1 PerDrawUBO_LightParams と同 §3.1 範式: V 側 member を `stars_v_time` に rename し、
// 使用箇所は `#define time stars_v_time` alias で source 不変、後段 attach 文書 (atmosphericsV /
// transportV など) の同名 token 誤 rewrite 防止に file 末尾で #undef。GL 経路 (`uniform float time`)
// は C++ binding 名 "time" を保持するため変更しない。
#ifndef STARS_V_TIME_DEFINED
#define STARS_V_TIME_DEFINED 1
layout(set=3, binding=45, std140) uniform StarsVParamUBO_Legacy {
    float stars_v_time;
};
#endif
#define time stars_v_time
#else
uniform float time;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=0) in vec3 position;
#else
in vec3 position;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=6) in vec4 diffuse_color;
#else
in vec4 diffuse_color;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=2) in vec2 texcoord0;
#else
in vec2 texcoord0;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=2) out vec4 vertex_color;
#else
out vec4 vertex_color;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=0) out vec2 vary_texcoord0;
#else
out vec2 vary_texcoord0;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=20) out vec2 screenpos;
#else
out vec2 screenpos;
#endif

void main()
{
    //transform vertex
    vec4 pos = modelview_projection_matrix * vec4(position, 1.0);


    // smash to far clip plane to
    // avoid rendering on top of moon (do NOT write to gl_FragDepth, it's slow)
    pos.z = pos.w;

    gl_Position = pos;

    float t = mod(time, 1.25f);
    screenpos = position.xy * vec2(t, t);
    vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;
    vertex_color = diffuse_color;
}

// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-20: scope-limit StarsVParamUBO_Legacy alias
//   (`time` → `stars_v_time`) so後段 attach 文書 (atmosphericsV / transportV など) の同名 token が
//   誤 rewrite されない。
#ifdef LL_VULKAN_GLSL
#undef time
#endif
