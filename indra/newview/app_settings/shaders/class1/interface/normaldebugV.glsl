/**
 * @file normaldebugV.glsl
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
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 0) out vec4 normal_g;
#ifdef HAS_ATTRIBUTE_TANGENT
layout(location = 8) in vec4 tangent;
layout(location = 1) out vec4 tangent_g;
#endif

layout(set = 1, binding = 0, std140) uniform NormalDebug_PerProgramBind
{
    float debug_normal_draw_length;
#ifndef _AYA_UM__pad0
#define _AYA_UM__pad0 1
    float _pad0;
#else
    float _dup_NormalDebug__pad0;
#endif
#ifndef _AYA_UM__pad1
#define _AYA_UM__pad1 1
    float _pad1;
#else
    float _dup_NormalDebug__pad1;
#endif
#ifndef _AYA_UM__pad2
#define _AYA_UM__pad2 1
    float _pad2;
#else
    float _dup_NormalDebug__pad2;
#endif
};
#else
in vec3 position;
in vec3 normal;
out vec4 normal_g;
#ifdef HAS_ATTRIBUTE_TANGENT
in vec4 tangent;
out vec4 tangent_g;
#endif

uniform float debug_normal_draw_length;
#endif

#ifdef HAS_SKIN
mat4 getObjectSkinnedTransform();
#else
#ifdef LL_VULKAN_GLSL
#define normal_matrix mat3(transpose(inverse(modelview_matrix)))
#else
uniform mat3 normal_matrix;
#endif
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
#else
uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
#endif

// *NOTE: Should use the modelview_projection_matrix here in the non-skinned
// case for efficiency, but opting for the simplier implementation for now as
// this is debug code. Also, the skinned version hasn't beeen tested yet.
// world_pos = mat * vec4(position.xyz, 1.0)
vec4 get_screen_normal(vec3 position, vec4 world_pos, vec3 normal, mat4 mat)
{
    vec4 world_norm = mat * vec4((position + normal), 1.0);
    world_norm.xyz -= world_pos.xyz;
    world_norm.xyz = debug_normal_draw_length * normalize(world_norm.xyz);
    world_norm.xyz += world_pos.xyz;
    return projection_matrix * world_norm;
}

void main()
{
#ifdef HAS_SKIN
    mat4 mat = getObjectSkinnedTransform();
    mat = modelview_matrix * mat;
#else
#define mat modelview_matrix
#endif

    vec4 world_pos = mat * vec4(position.xyz,1.0);

    gl_Position = projection_matrix * world_pos;
    normal_g = get_screen_normal(position.xyz, world_pos, normal.xyz, mat);
#ifdef HAS_ATTRIBUTE_TANGENT
    tangent_g = get_screen_normal(position.xyz, world_pos, tangent.xyz, mat);
#endif
}

