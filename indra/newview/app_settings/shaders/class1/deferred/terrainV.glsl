/**
 * @file class1\environment\terrainV.glsl
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
layout(location = 1) in vec3 normal;
layout(location = 3) in vec2 texcoord1;
#else
in vec3 position;
in vec3 normal;
in vec2 texcoord1;
#endif

#ifdef LL_VULKAN_GLSL
layout(location = 0) out vec3 pos;
layout(location = 1) out vec3 vary_normal;
layout(location = 2) out vec4 vary_texcoord0;
layout(location = 3) out vec4 vary_texcoord1;
#else
out vec3 pos;
out vec3 vary_normal;
out vec4 vary_texcoord0;
out vec4 vary_texcoord1;
#endif

#ifdef LL_VULKAN_GLSL
layout(set = 1, binding = 0, std140) uniform TerrainV_PerProgramBind
{
    vec4 object_plane_s;
    vec4 object_plane_t;
};
#else
uniform vec4 object_plane_s;
uniform vec4 object_plane_t;
#endif

vec2 texgen_object(vec4 vpos, mat4 mat, vec4 tp0, vec4 tp1)
{
    vec4 tcoord;

    tcoord.x = dot(vpos, tp0);
    tcoord.y = dot(vpos, tp1);
    tcoord.z = 0;
    tcoord.w = 1;

    tcoord = mat * tcoord;

    return tcoord.xy;
}

void main()
{
    //transform vertex
    vec4 pre_pos = vec4(position.xyz, 1.0);
    vec4 t_pos = modelview_projection_matrix * pre_pos;

    gl_Position = t_pos;
    pos = (modelview_matrix*pre_pos).xyz;

    vary_normal = normalize(normal_matrix * normal);

    // Transform and pass tex coords
    vary_texcoord0.xy = texgen_object(vec4(position, 1.0), texture_matrix0, object_plane_s, object_plane_t);

    vec4 t = vec4(texcoord1,0,1);

    vary_texcoord0.zw = t.xy;
    vary_texcoord1.xy = t.xy-vec2(2.0, 0.0);
    vary_texcoord1.zw = t.xy-vec2(1.0, 0.0);
}
