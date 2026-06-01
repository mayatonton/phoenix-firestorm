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
layout(location=20) out vec4 normal_g;
#else
out vec4 normal_g;
#endif
#ifdef HAS_ATTRIBUTE_TANGENT
#ifdef LL_VULKAN_GLSL
layout(location=8) in vec4 tangent;
#else
in vec4 tangent;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=21) out vec4 tangent_g;
#else
out vec4 tangent_g;
#endif
#endif

uniform float debug_normal_draw_length;

#ifdef HAS_SKIN
mat4 getObjectSkinnedTransform();
#else
#ifndef LL_VULKAN_GLSL
uniform mat3 normal_matrix;
#endif
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

