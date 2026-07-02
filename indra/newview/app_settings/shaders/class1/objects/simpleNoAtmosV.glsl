/**
 * @file simpleNoAtmosV.glsl
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
uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
uniform mat4 modelview_projection_matrix;
#endif

#ifdef LL_VULKAN_GLSL
layout(set = 1, binding = 51, std140) uniform DrawColor_PerShaderBind
{
#ifndef _AYA_UM_color
#define _AYA_UM_color 1
    vec4 color;
#else
    vec4 _dup_DrawColor_color;
#endif
};
#else
uniform vec4 color;
#endif

#ifdef LL_VULKAN_GLSL
layout(location = 0) in vec3 position;

layout(location = 0) out vec4 vertex_color;
layout(location = 1) out vec4 vertex_position;
#else
in vec3 position;

out vec4 vertex_color;
out vec4 vertex_position;
#endif

void main()
{
    //transform vertex
    vec4 pos = (modelview_matrix * vec4(position.xyz, 1.0));
    vertex_position = modelview_projection_matrix * vec4(position.xyz, 1.0);
    gl_Position = vertex_position;
    vertex_color = color;
}
