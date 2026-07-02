/**
 * @file pbgglowV.glsl
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
#ifndef PER_FRAME_MATRIX_UBO_DEFINED
#define PER_FRAME_MATRIX_UBO_DEFINED 1
layout(set = 0, binding = 0, std140) uniform PerFrameMatrixUBO
{
    mat4 projection_matrix;
    mat4 inverse_projection_matrix;
    mat4 identity_matrix;
    mat4 last_modelview_matrix;
};
#endif // PER_FRAME_MATRIX_UBO_DEFINED
layout(push_constant) uniform ModelviewPushConstant
{
    mat4 modelview_matrix;
};
#define modelview_projection_matrix (projection_matrix * modelview_matrix)
#else
uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
uniform mat4 modelview_projection_matrix;
#endif
#ifdef HAS_SKIN
mat4 getObjectSkinnedTransform();
#endif

#ifdef LL_VULKAN_GLSL
layout(set = 0, binding = 1, std140) uniform TextureMatrixUBO
{
    mat4 texture_matrix[4];
};
#define texture_matrix0 texture_matrix[0]
#else
uniform mat4 texture_matrix0;
#endif

#ifdef LL_VULKAN_GLSL
layout(set = 1, binding = 48, std140) uniform PBRMaterial_PerMaterial
{
    vec4 texture_base_color_transform[2];
    vec4 texture_normal_transform[2];
    vec4 texture_metallic_roughness_transform[2];
    vec4 texture_emissive_transform[2];
};
#else
uniform vec4[2] texture_base_color_transform;
uniform vec4[2] texture_emissive_transform;
#endif

#ifdef LL_VULKAN_GLSL
layout(location = 0) in vec3 position;
layout(location = 7) in vec4 emissive;
layout(location = 2) in vec2 texcoord0;

layout(location = 0) out vec2 base_color_texcoord;
layout(location = 1) out vec2 emissive_texcoord;
layout(location = 2) out vec4 vertex_emissive;
#else
in vec3 position;
in vec4 emissive;

in vec2 texcoord0;

out vec2 base_color_texcoord;
out vec2 emissive_texcoord;

out vec4 vertex_emissive;
#endif

vec2 texture_transform(vec2 vertex_texcoord, vec4[2] khr_gltf_transform, mat4 sl_animation_transform);

void main()
{
#ifdef HAS_SKIN
    mat4 mat = getObjectSkinnedTransform();

    mat = modelview_matrix * mat;

    vec3 pos = (mat*vec4(position.xyz,1.0)).xyz;

    gl_Position = projection_matrix*vec4(pos,1.0);
#else
    //transform vertex
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);
#endif

    base_color_texcoord = texture_transform(texcoord0, texture_base_color_transform, texture_matrix0);
    emissive_texcoord = texture_transform(texcoord0, texture_emissive_transform, texture_matrix0);

    vertex_emissive = emissive;
}

