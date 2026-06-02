/**
 * @file class1\environment\pbrterrainV.glsl
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

#define TERRAIN_PBR_DETAIL_EMISSIVE 0
#define TERRAIN_PBR_DETAIL_OCCLUSION -1
#define TERRAIN_PBR_DETAIL_NORMAL -2
#define TERRAIN_PBR_DETAIL_METALLIC_ROUGHNESS -3

#define TERRAIN_PAINT_TYPE_HEIGHTMAP_WITH_NOISE 0
#define TERRAIN_PAINT_TYPE_PBR_PAINTMAP 1

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
uniform mat3 normal_matrix;
#endif
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
#ifndef LL_VULKAN_GLSL
uniform mat4 modelview_matrix;
uniform mat4 modelview_projection_matrix;
#endif
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2c:
//   PBR terrain V-stage terrain_texture_transforms + region_scale を UBO 化 (元 L178 + L70 を統合)。
//   host = pipeline.cpp PBR terrain pass (TERRAIN_TEXTURE_TRANSFORMS + REGION_SCALE reserved)。
//   2 permutation (HEIGHTMAP_WITH_NOISE / PBR_PAINTMAP) で同 UBO 共有、両 field 常時含める
//   (region_scale は heightmap 側 declared-but-unused 容認)。
//   F pair = pbrterrainF.glsl は η-21+ で TerrainDetailUBO set=1 binding=17 に UBO 化済 → 別 descriptor。
//   η-28-C 範式の type 3 (同 file 内 preprocessor permutation 共有)。
#ifndef PER_PROGRAM_UBO_PBR_TERRAIN_V_DEFINED
#define PER_PROGRAM_UBO_PBR_TERRAIN_V_DEFINED 1
layout(set=2, binding=24, std140) uniform PerProgramUBO_PbrTerrainV {
    vec4  terrain_texture_transforms[5];  // offset 0 (vec4 array stride 16 × 5 = 80 bytes)
    float region_scale;                   // offset 80
    float _pad0;                          // offset 84 (vec4 boundary 揃え)
    float _pad1;                          // offset 88
    float _pad2;                          // offset 92
};  // total 96
#endif
#else
#if TERRAIN_PAINT_TYPE == TERRAIN_PAINT_TYPE_PBR_PAINTMAP
uniform float region_scale;
#endif
#endif

#ifdef LL_VULKAN_GLSL
layout(location=0) in vec3 position;
#else
in vec3 position;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=1) in vec3 normal;
#else
in vec3 normal;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=8) in vec4 tangent;
#else
in vec4 tangent;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=6) in vec4 diffuse_color;
#else
in vec4 diffuse_color;
#endif
#if TERRAIN_PAINT_TYPE == TERRAIN_PAINT_TYPE_HEIGHTMAP_WITH_NOISE
#ifdef LL_VULKAN_GLSL
layout(location=3) in vec2 texcoord1;
#else
in vec2 texcoord1;
#endif
#endif

#ifdef LL_VULKAN_GLSL
layout(location=3) out vec3 vary_position;
#else
out vec3 vary_position;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=4) out vec3 vary_normal;
#else
out vec3 vary_normal;
#endif
#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-24 Phase C: vary_vertex_normal location 20 → 50
// (atmosphericsVarsV.glsl vary_AdditiveColor at location=20 との overlap 解消、pbrterrainUtilF.glsl L59
//  既存 location=50 と V↔F 整合、η-18 §3.1 範式類)
layout(location=50) out vec3 vary_vertex_normal; // Used by pbrterrainUtilF.glsl
#else
out vec3 vary_vertex_normal; // Used by pbrterrainUtilF.glsl
#endif
#endif
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_NORMAL)
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-24 第2巡 Phase C: vary_tangents location 21 → 52
// (atmosphericsVarsV.glsl vary_AtmosAttenuation at location=21 との overlap 解消、η-18 §3.1 範式類)
// vec3[4] = 4 slots (52,53,54,55)、location 50 vary_vertex_normal / 51 vary_norm の直後で連続
// 予測 cascade: pbrterrainF.glsl L206 vary_tangents loc=20 (vary_AdditiveColor との overlap +
//   V↔F mismatch) は次 η iteration で resolve
layout(location=52) out vec3 vary_tangents[4];
#else
out vec3 vary_tangents[4];
#endif
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-25 Phase 2: vary_signs location 25 → 56
// (pbrterrainF.glsl L211 旧 loc=24 と V↔F mismatch 解消、η-24 §5.6 56 帯使用、
//  vary_tangents[4] = slots 52-55 直後で連続、η-18 §3.1 範式類)
layout(location=56) flat out float vary_signs[4];
#else
flat out float vary_signs[4];
#endif
#endif

// vary_texcoord* are used for terrain composition, vary_coords are used for terrain UVs
#if TERRAIN_PAINT_TYPE == TERRAIN_PAINT_TYPE_HEIGHTMAP_WITH_NOISE
#ifdef LL_VULKAN_GLSL
layout(location=0) out vec4 vary_texcoord0;
#else
out vec4 vary_texcoord0;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=5) out vec4 vary_texcoord1;
#else
out vec4 vary_texcoord1;
#endif
#elif TERRAIN_PAINT_TYPE == TERRAIN_PAINT_TYPE_PBR_PAINTMAP
#ifdef LL_VULKAN_GLSL
layout(location=29) out vec2 vary_texcoord;
#else
out vec2 vary_texcoord;
#endif
#endif
#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
#ifdef LL_VULKAN_GLSL
layout(location=30) out vec4[10] vary_coords;
#else
out vec4[10] vary_coords;
#endif
#elif TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 1
#ifdef LL_VULKAN_GLSL
layout(location=30) out vec4[2] vary_coords;
#else
out vec4[2] vary_coords;
#endif
#endif

// *HACK: Each material uses only one texture transform, but the KHR texture
// transform spec allows handling texture transforms separately for each
// individual texture info.
#ifndef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2c:
//   Vulkan 側は PerProgramUBO_PbrTerrainV (binding 24、region_scale と統合) に移動済。
uniform vec4[5] terrain_texture_transforms;
#endif

vec2 terrain_texture_transform(vec2 vertex_texcoord, vec4[2] khr_gltf_transform);
vec4 terrain_tangent_space_transform(vec4 vertex_tangent, vec3 vertex_normal, vec4[2] khr_gltf_transform);

void main()
{
    //transform vertex
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);
    vary_position = (modelview_matrix*vec4(position.xyz, 1.0)).xyz;

    vec3 n = normal_matrix * normal;
#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
    vary_vertex_normal = normal;
#endif
    vec3 t = normal_matrix * tangent.xyz;

#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_NORMAL)
    {
        vec4[2] ttt;
        vec4 transformed_tangent;
        // material 1
        ttt[0].xyz = terrain_texture_transforms[0].xyz;
        ttt[1].x = terrain_texture_transforms[0].w;
        ttt[1].y = terrain_texture_transforms[1].x;
        transformed_tangent = terrain_tangent_space_transform(vec4(t, tangent.w), n, ttt);
        vary_tangents[0] = normalize(transformed_tangent.xyz);
        vary_signs[0] = transformed_tangent.w;
        // material 2
        ttt[0].xyz = terrain_texture_transforms[1].yzw;
        ttt[1].xy = terrain_texture_transforms[2].xy;
        transformed_tangent = terrain_tangent_space_transform(vec4(t, tangent.w), n, ttt);
        vary_tangents[1] = normalize(transformed_tangent.xyz);
        vary_signs[1] = transformed_tangent.w;
        // material 3
        ttt[0].xy = terrain_texture_transforms[2].zw;
        ttt[0].z = terrain_texture_transforms[3].x;
        ttt[1].xy = terrain_texture_transforms[3].yz;
        transformed_tangent = terrain_tangent_space_transform(vec4(t, tangent.w), n, ttt);
        vary_tangents[2] = normalize(transformed_tangent.xyz);
        vary_signs[2] = transformed_tangent.w;
        // material 4
        ttt[0].x = terrain_texture_transforms[3].w;
        ttt[0].yz = terrain_texture_transforms[4].xy;
        ttt[1].xy = terrain_texture_transforms[4].zw;
        transformed_tangent = terrain_tangent_space_transform(vec4(t, tangent.w), n, ttt);
        vary_tangents[3] = normalize(transformed_tangent.xyz);
        vary_signs[3] = transformed_tangent.w;
    }
#endif
    vary_normal = normalize(n);

    // Transform and pass tex coords
    {
        vec4[2] ttt;
#define transform_xy()             terrain_texture_transform(position.xy,               ttt)
#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
// Don't care about upside-down (transform_xy_flipped())
#define transform_yz()             terrain_texture_transform(position.yz,               ttt)
#define transform_negx_z()         terrain_texture_transform(position.xz * vec2(-1, 1), ttt)
#define transform_yz_flipped()     terrain_texture_transform(position.yz * vec2(-1, 1), ttt)
#define transform_negx_z_flipped() terrain_texture_transform(position.xz,               ttt)
        // material 1
        ttt[0].xyz = terrain_texture_transforms[0].xyz;
        ttt[1].x = terrain_texture_transforms[0].w;
        ttt[1].y = terrain_texture_transforms[1].x;
        vary_coords[0].xy = transform_xy();
        vary_coords[0].zw = transform_yz();
        vary_coords[1].xy = transform_negx_z();
        vary_coords[1].zw = transform_yz_flipped();
        vary_coords[2].xy = transform_negx_z_flipped();
        // material 2
        ttt[0].xyz = terrain_texture_transforms[1].yzw;
        ttt[1].xy = terrain_texture_transforms[2].xy;
        vary_coords[2].zw = transform_xy();
        vary_coords[3].xy = transform_yz();
        vary_coords[3].zw = transform_negx_z();
        vary_coords[4].xy = transform_yz_flipped();
        vary_coords[4].zw = transform_negx_z_flipped();
        // material 3
        ttt[0].xy = terrain_texture_transforms[2].zw;
        ttt[0].z = terrain_texture_transforms[3].x;
        ttt[1].xy = terrain_texture_transforms[3].yz;
        vary_coords[5].xy = transform_xy();
        vary_coords[5].zw = transform_yz();
        vary_coords[6].xy = transform_negx_z();
        vary_coords[6].zw = transform_yz_flipped();
        vary_coords[7].xy = transform_negx_z_flipped();
        // material 4
        ttt[0].x = terrain_texture_transforms[3].w;
        ttt[0].yz = terrain_texture_transforms[4].xy;
        ttt[1].xy = terrain_texture_transforms[4].zw;
        vary_coords[7].zw = transform_xy();
        vary_coords[8].xy = transform_yz();
        vary_coords[8].zw = transform_negx_z();
        vary_coords[9].xy = transform_yz_flipped();
        vary_coords[9].zw = transform_negx_z_flipped();
#elif TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 1
        // material 1
        ttt[0].xyz = terrain_texture_transforms[0].xyz;
        ttt[1].x = terrain_texture_transforms[0].w;
        ttt[1].y = terrain_texture_transforms[1].x;
        vary_coords[0].xy = transform_xy();
        // material 2
        ttt[0].xyz = terrain_texture_transforms[1].yzw;
        ttt[1].xy = terrain_texture_transforms[2].xy;
        vary_coords[0].zw = transform_xy();
        // material 3
        ttt[0].xy = terrain_texture_transforms[2].zw;
        ttt[0].z = terrain_texture_transforms[3].x;
        ttt[1].xy = terrain_texture_transforms[3].yz;
        vary_coords[1].xy = transform_xy();
        // material 4
        ttt[0].x = terrain_texture_transforms[3].w;
        ttt[0].yz = terrain_texture_transforms[4].xy;
        ttt[1].xy = terrain_texture_transforms[4].zw;
        vary_coords[1].zw = transform_xy();
#endif
    }

#if TERRAIN_PAINT_TYPE == TERRAIN_PAINT_TYPE_HEIGHTMAP_WITH_NOISE
    vec2 tc = texcoord1.xy;
    vary_texcoord0.zw = tc.xy;
    vary_texcoord1.xy = tc.xy-vec2(2.0, 0.0);
    vary_texcoord1.zw = tc.xy-vec2(1.0, 0.0);
#elif TERRAIN_PAINT_TYPE == TERRAIN_PAINT_TYPE_PBR_PAINTMAP
    vary_texcoord = position.xy / region_scale;
#endif
}
