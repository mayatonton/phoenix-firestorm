/**
 * @file fullbrightShinyV.glsl
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
uniform mat3 normal_matrix;
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
#endif
uniform mat4 texture_matrix1;
#ifndef LL_VULKAN_GLSL
uniform mat4 modelview_matrix;
uniform mat4 modelview_projection_matrix;
#endif


void calcAtmospherics(vec3 inPositionEye);

uniform vec4 origin;



in vec3 position;
void passTextureIndex();
in vec3 normal;
in vec4 diffuse_color;
in vec2 texcoord0;

out vec4 vertex_color;
out vec2 vary_texcoord0;
out vec3 vary_texcoord1;
out vec3 vary_position;

#ifdef HAS_SKIN
mat4 getObjectSkinnedTransform();
#ifndef LL_VULKAN_GLSL
uniform mat4 projection_matrix;
#endif
#endif

void main()
{
    //transform vertex
    vec4 vert = vec4(position.xyz,1.0);
    passTextureIndex();

#ifdef HAS_SKIN
    mat4 mat = getObjectSkinnedTransform();
    mat = modelview_matrix * mat;
    vec4 pos = mat * vert;
    gl_Position = projection_matrix * pos;
    vec3 norm = normalize((mat*vec4(normal.xyz+position.xyz,1.0)).xyz-pos.xyz);
#else
    vec4 pos = (modelview_matrix * vert);
    gl_Position = modelview_projection_matrix*vec4(position.xyz, 1.0);
    vec3 norm = normalize(normal_matrix * normal);
#endif

    vary_position = pos.xyz;
    vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;

    vary_texcoord1 = norm;

    calcAtmospherics(pos.xyz);

    vertex_color = diffuse_color;
}
