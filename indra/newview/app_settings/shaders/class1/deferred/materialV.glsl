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
uniform mat4 projection_matrix;
uniform mat4 modelview_projection_matrix;
#endif

#ifdef HAS_SKIN
mat4 getObjectSkinnedTransform();
#else
#ifndef LL_VULKAN_GLSL
uniform mat3 normal_matrix;
#endif
#endif

#ifdef LL_VULKAN_GLSL
layout(location=3) out vec3 vary_position;
#else
out vec3 vary_position;
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

#ifdef LL_VULKAN_GLSL
layout(location=0) in vec3 position;
#else
in vec3 position;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=6) in vec4 diffuse_color;
#else
in vec4 diffuse_color;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=1) in vec3 normal;
#else
in vec3 normal;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=2) in vec2 texcoord0;
#else
in vec2 texcoord0;
#endif


#ifdef HAS_NORMAL_MAP
#ifdef LL_VULKAN_GLSL
layout(location=8) in vec4 tangent;
#else
in vec4 tangent;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=3) in vec2 texcoord1;
#else
in vec2 texcoord1;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=9) out vec3 vary_tangent;
#else
out vec3 vary_tangent;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=10) flat out float vary_sign;
#else
flat out float vary_sign;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=4) out vec3 vary_normal;
#else
out vec3 vary_normal;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=5) out vec2 vary_texcoord1;
#else
out vec2 vary_texcoord1;
#endif
#else
#ifdef LL_VULKAN_GLSL
layout(location=4) out vec3 vary_normal;
#else
out vec3 vary_normal;
#endif
#endif

#ifdef HAS_SPECULAR_MAP
#ifdef LL_VULKAN_GLSL
layout(location=4) in vec2 texcoord2;
#else
in vec2 texcoord2;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=16) out vec2 vary_texcoord2;
#else
out vec2 vary_texcoord2;
#endif
#endif

#ifdef LL_VULKAN_GLSL
layout(location=2) out vec4 vertex_color;
#else
out vec4 vertex_color;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=0) out vec2 vary_texcoord0;
#else
out vec2 vary_texcoord0;
#endif

void main()
{
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

