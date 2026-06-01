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
uniform mat4 modelview_matrix;
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
uniform vec4 color;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=0) in vec3 position;
#else
in vec3 position;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=2) out vec4 vertex_color;
#else
out vec4 vertex_color;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=20) out vec4 vertex_position;
#else
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
