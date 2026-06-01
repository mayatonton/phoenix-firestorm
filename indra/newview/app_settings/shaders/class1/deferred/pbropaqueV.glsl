/**
 * @file pbropaqueV.glsl
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


#ifndef IS_HUD

//deferred opaque implementation

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
uniform mat4 modelview_matrix;
#endif

#ifdef HAS_SKIN
#ifndef LL_VULKAN_GLSL
uniform mat4 projection_matrix;
#endif
mat4 getObjectSkinnedTransform();
#else
#ifndef LL_VULKAN_GLSL
uniform mat3 normal_matrix;
uniform mat4 modelview_projection_matrix;
#endif
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

uniform vec4[2] texture_base_color_transform;
#endif
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-6 phase 2-C: PbrOpaqueV non-opaque uniforms UBO wrap (η-5 §3.1 (b-1) 範式継承)
layout(set=3, binding=53, std140) uniform PbrOpaqueVParamUBO_Legacy {
    vec4 texture_normal_transform[2];
    vec4 texture_metallic_roughness_transform[2];
};
#else
uniform vec4[2] texture_normal_transform;
uniform vec4[2] texture_metallic_roughness_transform;
#endif
#ifndef LL_VULKAN_GLSL
uniform vec4[2] texture_emissive_transform;
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
layout(location=2) in vec2 texcoord0;
#else
in vec2 texcoord0;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=6) out vec2 base_color_texcoord;
#else
out vec2 base_color_texcoord;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=20) out vec2 normal_texcoord;
#else
out vec2 normal_texcoord;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=21) out vec2 metallic_roughness_texcoord;
#else
out vec2 metallic_roughness_texcoord;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=7) out vec2 emissive_texcoord;
#else
out vec2 emissive_texcoord;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=2) out vec4 vertex_color;
#else
out vec4 vertex_color;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=9) out vec3 vary_tangent;
#else
out vec3 vary_tangent;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=10) flat out float vary_sign;
#else
flat out float vary_sign;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=4) out vec3 vary_normal;
#else
out vec3 vary_normal;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=3) out vec3 vary_position;
#else
out vec3 vary_position;
#endif

vec2 texture_transform(vec2 vertex_texcoord, vec4[2] khr_gltf_transform, mat4 sl_animation_transform);
vec4 tangent_space_transform(vec4 vertex_tangent, vec3 vertex_normal, vec4[2] khr_gltf_transform, mat4 sl_animation_transform);

void main()
{
#ifdef HAS_SKIN
    mat4 mat = getObjectSkinnedTransform();

    mat = modelview_matrix * mat;

    vec3 pos = (mat*vec4(position.xyz,1.0)).xyz;
    vary_position = pos;
    gl_Position = projection_matrix*vec4(pos,1.0);

#else
    vary_position = (modelview_matrix*vec4(position.xyz, 1.0)).xyz;
    //transform vertex
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);
#endif

    base_color_texcoord = texture_transform(texcoord0, texture_base_color_transform, texture_matrix0);
    normal_texcoord = texture_transform(texcoord0, texture_normal_transform, texture_matrix0);
    metallic_roughness_texcoord = texture_transform(texcoord0, texture_metallic_roughness_transform, texture_matrix0);
    emissive_texcoord = texture_transform(texcoord0, texture_emissive_transform, texture_matrix0);

#ifdef HAS_SKIN
    vec3 n = (mat*vec4(normal.xyz+position.xyz,1.0)).xyz-pos.xyz;
    vec3 t = (mat*vec4(tangent.xyz+position.xyz,1.0)).xyz-pos.xyz;
#else //HAS_SKIN
    vec3 n = normal_matrix * normal;
    vec3 t = normal_matrix * tangent.xyz;
#endif

    n = normalize(n);

    vec4 transformed_tangent = tangent_space_transform(vec4(t, tangent.w), n, texture_normal_transform, texture_matrix0);
    vary_tangent = normalize(transformed_tangent.xyz);
    vary_sign = transformed_tangent.w;
    vary_normal = n;

    vertex_color = diffuse_color;
}

#else

// fullbright HUD implementation

#ifndef LL_VULKAN_GLSL
uniform mat4 modelview_projection_matrix;
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

uniform vec4[2] texture_base_color_transform;
uniform vec4[2] texture_emissive_transform;
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
layout(location=6) out vec2 base_color_texcoord;
#else
out vec2 base_color_texcoord;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=7) out vec2 emissive_texcoord;
#else
out vec2 emissive_texcoord;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=2) out vec4 vertex_color;
#else
out vec4 vertex_color;
#endif

vec2 texture_transform(vec2 vertex_texcoord, vec4[2] khr_gltf_transform, mat4 sl_animation_transform);

#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-5 phase 2-B: HUD branch FrameViewProj guard wrap (Agent C scope leak fix)
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
#endif

void main()
{
    //transform vertex
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);

    base_color_texcoord = texture_transform(texcoord0, texture_base_color_transform, texture_matrix0);
    emissive_texcoord = texture_transform(texcoord0, texture_emissive_transform, texture_matrix0);

    vertex_color = diffuse_color;
}

#endif


