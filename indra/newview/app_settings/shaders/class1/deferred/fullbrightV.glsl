/**
 * @file fullbrightV.glsl
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


#ifdef LL_VULKAN_GLSL
layout(location = 0) in vec3 position;
void passTextureIndex();
layout(location = 6) in vec4 diffuse_color;
layout(location = 2) in vec2 texcoord0;

void calcAtmospherics(vec3 inPositionEye);

vec3 atmosAmbient();
vec3 atmosAffectDirectionalLight(float lightIntensity);

layout(location = 0) out vec3 vary_position;

layout(location = 1) out vec4 vertex_color;
layout(location = 2) out vec2 vary_texcoord0;
#else
in vec3 position;
void passTextureIndex();
in vec4 diffuse_color;
in vec2 texcoord0;

void calcAtmospherics(vec3 inPositionEye);

vec3 atmosAmbient();
vec3 atmosAffectDirectionalLight(float lightIntensity);

out vec3 vary_position;

out vec4 vertex_color;
out vec2 vary_texcoord0;
#endif

#ifdef HAS_SKIN
mat4 getObjectSkinnedTransform();
#endif

void main()
{
    //transform vertex
    vec4 vert = vec4(position.xyz, 1.0);
    passTextureIndex();

#ifdef HAS_SKIN
    mat4 mat = getObjectSkinnedTransform();
    mat = modelview_matrix * mat;
    vec4 pos = mat * vert;
    gl_Position = projection_matrix * pos;
#else
    vec4 pos = (modelview_matrix * vert);
    gl_Position = modelview_projection_matrix*vec4(position.xyz, 1.0);
#endif

    vary_position = pos.xyz;

    vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;

    calcAtmospherics(pos.xyz);

    vertex_color = diffuse_color;
}
