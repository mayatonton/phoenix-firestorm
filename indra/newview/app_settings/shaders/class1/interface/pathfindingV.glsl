/**
 * @file pathfindingV.glsl
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
uniform mat4 modelview_projection_matrix;
#endif

#ifdef LL_VULKAN_GLSL
layout(location = 0) in vec3 position;
layout(location = 6) in vec4 diffuse_color;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec4 vertex_color;

layout(set = 1, binding = 0, std140) uniform Pathfinding_PerProgramBind
{
    float tint;
    float ambiance;
    float alpha_scale;
#ifndef _AYA_UM__pad0
#define _AYA_UM__pad0 1
    float _pad0;
#else
    float _dup_Pathfinding__pad0;
#endif
};
#else
in vec3 position;
in vec4 diffuse_color;
in vec3 normal;

out vec4 vertex_color;

uniform float tint;
uniform float ambiance;
uniform float alpha_scale;
#endif

void main()
{
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);

    vec3 l1 = vec3(-0.75, 1, 1.0)*0.5;
    vec3 l2 = vec3(0.5, -0.6, 0.4)*0.25;
    vec3 l3 = vec3(0.5, -0.8, 0.3)*0.5;

    float lit = max(dot(normal, l1), 0.0);
    lit += max(dot(normal, l2), 0.0);
    lit += max(dot(normal, l3), 0.0);

    lit = clamp(lit, ambiance, 1.0);

    vertex_color = vec4(diffuse_color.rgb * tint * lit, diffuse_color.a*alpha_scale);
}

