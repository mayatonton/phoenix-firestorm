/**
 * @file bumpV.glsl
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
#define normal_matrix mat3(transpose(inverse(modelview_matrix)))
#else
uniform mat3 normal_matrix;
#endif
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
layout(location = 0) in vec3 position;
layout(location = 6) in vec4 diffuse_color;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord0;
layout(location = 8) in vec4 tangent;

layout(location = 0) out vec3 vary_mat0;
layout(location = 1) out vec3 vary_mat1;
layout(location = 2) out vec3 vary_mat2;
layout(location = 3) out vec4 vertex_color;
layout(location = 4) out vec2 vary_texcoord0;
layout(location = 5) out vec3 vary_position;
#else
in vec3 position;
in vec4 diffuse_color;
in vec3 normal;
in vec2 texcoord0;
in vec4 tangent;

out vec3 vary_mat0;
out vec3 vary_mat1;
out vec3 vary_mat2;
out vec4 vertex_color;
out vec2 vary_texcoord0;
out vec3 vary_position;
#endif

#ifdef HAS_SKIN
mat4 getObjectSkinnedTransform();
#endif

void main()
{
    //transform vertex
#ifdef HAS_SKIN
    mat4 mat = getObjectSkinnedTransform();
    mat = modelview_matrix * mat;
    vec3 pos = (mat*vec4(position.xyz, 1.0)).xyz;
    vary_position = pos;
    gl_Position = projection_matrix*vec4(pos, 1.0);

    vec3 n = normalize((mat * vec4(normal.xyz+position.xyz, 1.0)).xyz-pos.xyz);
    vec3 t = normalize((mat * vec4(tangent.xyz+position.xyz, 1.0)).xyz-pos.xyz);
#else
    vary_position = (modelview_matrix*vec4(position.xyz, 1.0)).xyz;
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);
    vec3 n = normalize(normal_matrix * normal);
    vec3 t = normalize(normal_matrix * tangent.xyz);
#endif

    vec3 b = cross(n, t) * tangent.w;
    vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;

    vary_mat0 = vec3(t.x, b.x, n.x);
    vary_mat1 = vec3(t.y, b.y, n.y);
    vary_mat2 = vec3(t.z, b.z, n.z);

    vertex_color = diffuse_color;
}
