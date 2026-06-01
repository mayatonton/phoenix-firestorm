/**
 * @file class3\deferred\pointLightV.glsl
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
uniform mat4 modelview_projection_matrix;
uniform mat4 modelview_matrix;
#endif

in vec3 position;

uniform vec3 center;
uniform float size;

#ifdef LL_VULKAN_GLSL
layout(location=1) out vec4 vary_fragcoord;
#else
out vec4 vary_fragcoord;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=20) out vec3 trans_center;
#else
out vec3 trans_center;
#endif

void main()
{
    //transform vertex
    vec3 p = position*size+center;
    vec4 pos = modelview_projection_matrix * vec4(p.xyz, 1.0);
    vary_fragcoord = pos;
    trans_center = (modelview_matrix*vec4(center.xyz, 1.0)).xyz;
    gl_Position = pos;
}
