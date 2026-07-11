/**
 * @file shadowAlphaMaskV.glsl
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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
layout(set = 0, binding = 1, std140) uniform TextureMatrixUBO
{
    mat4 texture_matrix[4];
};
#define texture_matrix0 texture_matrix[0]
#else
uniform mat4 texture_matrix0;
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
#if defined(HAS_SKIN)
mat4 getObjectSkinnedTransform();
#endif

#ifdef LL_VULKAN_GLSL
layout(set = 1, binding = 53, std140) uniform ShadowParams_PerShaderBind
{
    float shadow_target_width;
#ifndef _AYA_UM__pad0
#define _AYA_UM__pad0 1
    float _pad0;
#else
    float _dup_ShadowParams__pad0;
#endif
#ifndef _AYA_UM__pad1
#define _AYA_UM__pad1 1
    float _pad1;
#else
    float _dup_ShadowParams__pad1;
#endif
#ifndef _AYA_UM__pad2
#define _AYA_UM__pad2 1
    float _pad2;
#else
    float _dup_ShadowParams__pad2;
#endif
};
#else
uniform float shadow_target_width;
#endif

#ifdef LL_VULKAN_GLSL
layout(location = 0) in vec3 position;
layout(location = 2) in vec2 texcoord0;

layout(location = 0) out vec4 post_pos;
layout(location = 1) out float target_pos_x;
layout(location = 2) out vec4 vertex_color;
layout(location = 3) out vec2 vary_texcoord0;
#else
in vec3 position;
in vec2 texcoord0;

out vec4 post_pos;
out float target_pos_x;
out vec4 vertex_color;
out vec2 vary_texcoord0;
#endif

void passTextureIndex();

void main()
{
    //transform vertex
#if defined(HAS_SKIN)
    vec4 pre_pos = vec4(position.xyz, 1.0);
    mat4 mat = getObjectSkinnedTransform();
    mat = modelview_matrix * mat;
    vec4 pos = mat * pre_pos;
    pos = projection_matrix * pos;
#else
    vec4 pre_pos = vec4(position.xyz, 1.0);
    vec4 pos = modelview_projection_matrix * pre_pos;
#endif

    target_pos_x = 0.5 * (shadow_target_width - 1.0) * pos.x;

    post_pos = pos;

    gl_Position = pos;

    passTextureIndex();

    vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;
    vertex_color = vec4(0.0, 0.0, 0.0, 1.0);
}
