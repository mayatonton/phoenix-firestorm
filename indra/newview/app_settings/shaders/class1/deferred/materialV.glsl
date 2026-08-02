/**
 * @file materialV.glsl
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

#define DIFFUSE_ALPHA_MODE_IGNORE 0
#define DIFFUSE_ALPHA_MODE_BLEND 1
#define DIFFUSE_ALPHA_MODE_MASK 2
#define DIFFUSE_ALPHA_MODE_EMISSIVE 3

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

#ifdef HAS_SKIN
mat4 getObjectSkinnedTransform();
#else
#ifdef LL_VULKAN_GLSL
#define normal_matrix mat3(transpose(inverse(modelview_matrix)))
#else
uniform mat3 normal_matrix;
#endif
#endif

#ifdef LL_VULKAN_GLSL
layout(location = 0) out vec3 vary_position;
#else
out vec3 vary_position;
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
#else
in vec3 position;
in vec4 diffuse_color;
in vec3 normal;
in vec2 texcoord0;
#endif


#ifdef HAS_NORMAL_MAP
#ifdef LL_VULKAN_GLSL
layout(location = 8) in vec4 tangent;
layout(location = 3) in vec2 texcoord1;
layout(location = 2) out vec3 vary_tangent;
layout(location = 3) flat out float vary_sign;
layout(location = 1) out vec3 vary_normal;
layout(location = 4) out vec2 vary_texcoord1;
#else
in vec4 tangent;
in vec2 texcoord1;
out vec3 vary_tangent;
flat out float vary_sign;
out vec3 vary_normal;
out vec2 vary_texcoord1;
#endif
#else
#ifdef LL_VULKAN_GLSL
layout(location = 1) out vec3 vary_normal;
#else
out vec3 vary_normal;
#endif
#endif

#ifdef HAS_SPECULAR_MAP
#ifdef LL_VULKAN_GLSL
layout(location = 4) in vec2 texcoord2;
layout(location = 5) out vec2 vary_texcoord2;
#else
in vec2 texcoord2;
out vec2 vary_texcoord2;
#endif
#endif

#if defined(AYA_BINDLESS_MAT) && (defined(HAS_NORMAL_MAP) || defined(HAS_SPECULAR_MAP))
#define AYA_MAT_HEAP 1
#endif

#ifdef LL_VULKAN_GLSL
layout(location = 6) out vec4 vertex_color;
layout(location = 7) out vec2 vary_texcoord0;
#if defined(AYA_MAT_HEAP) || (defined(AYA_BINDLESS_MAT) && DIFFUSE_ALPHA_MODE != DIFFUSE_ALPHA_MODE_BLEND)
layout(location = 19) flat out int aya_draw_id;
#endif
#else
out vec4 vertex_color;
out vec2 vary_texcoord0;
#endif

void main()
{
#if defined(LL_VULKAN_GLSL) && (defined(AYA_MAT_HEAP) || (defined(AYA_BINDLESS_MAT) && DIFFUSE_ALPHA_MODE != DIFFUSE_ALPHA_MODE_BLEND))
    aya_draw_id = gl_InstanceIndex;
#endif
#ifdef HAS_SKIN
    mat4 mat = getObjectSkinnedTransform();

    mat = modelview_matrix * mat;

    vec3 pos = (mat*vec4(position.xyz,1.0)).xyz;

    vary_position = pos;

    gl_Position = projection_matrix*vec4(pos,1.0);

#else
    //transform vertex
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);

#endif

    vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;

#ifdef HAS_NORMAL_MAP
    vary_texcoord1 = (texture_matrix0 * vec4(texcoord1,0,1)).xy;
#endif

#ifdef HAS_SPECULAR_MAP
    vary_texcoord2 = (texture_matrix0 * vec4(texcoord2,0,1)).xy;
#endif

#ifdef HAS_SKIN
    vec3 n = normalize((mat*vec4(normal.xyz+position.xyz,1.0)).xyz-pos.xyz);
#ifdef HAS_NORMAL_MAP
    vec3 t = normalize((mat*vec4(tangent.xyz+position.xyz,1.0)).xyz-pos.xyz);

    vary_tangent = t;
    vary_sign = tangent.w;
    vary_normal = n;
#else //HAS_NORMAL_MAP
    vary_normal  = n;
#endif //HAS_NORMAL_MAP
#else //HAS_SKIN
    vec3 n = normalize(normal_matrix * normal);
#ifdef HAS_NORMAL_MAP
    vec3 t = normalize(normal_matrix * tangent.xyz);

    vary_tangent = t;
    vary_sign = tangent.w;
    vary_normal = n;
#else //HAS_NORMAL_MAP
    vary_normal = n;
#endif //HAS_NORMAL_MAP
#endif //HAS_SKIN

    vertex_color = diffuse_color;

#if !defined(HAS_SKIN)
    vary_position = (modelview_matrix*vec4(position.xyz, 1.0)).xyz;
#endif
}

