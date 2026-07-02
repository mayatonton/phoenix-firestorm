/**
 * @file occlusionCubeV.glsl
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

layout(set = 1, binding = 0, std140) uniform OcclusionCube_PerProgramBind
{
#ifndef _AYA_UM_box_center
#define _AYA_UM_box_center 1
    vec3 box_center;
#else
    vec3 _dup_OcclusionCube_box_center;
#endif
#ifndef _AYA_UM__pad0
#define _AYA_UM__pad0 1
    float _pad0;
#else
    float _dup_OcclusionCube__pad0;
#endif
#ifndef _AYA_UM_box_size
#define _AYA_UM_box_size 1
    vec3 box_size;
#else
    vec3 _dup_OcclusionCube_box_size;
#endif
#ifndef _AYA_UM__pad1
#define _AYA_UM__pad1 1
    float _pad1;
#else
    float _dup_OcclusionCube__pad1;
#endif
};
#else
in vec3 position;

uniform vec3 box_center;
uniform vec3 box_size;
#endif

void main()
{
    vec3 p = position*box_size+box_center;
    gl_Position = modelview_projection_matrix * vec4(p.xyz, 1.0);
}

