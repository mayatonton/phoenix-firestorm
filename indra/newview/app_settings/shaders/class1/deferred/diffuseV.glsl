/**
 * @file diffuseV.glsl
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

// r41 sub-step 4.3-γ'-port-β-1 (sub-doc 06 §1.2.4 LL_VULKAN_GLSL macro switch):
// bundle-A (uniform → UBO) + bundle-B (location qualifier) for Vulkan path、GL path 維持
// (set=0/binding=0 PerFrame / set=1/binding=0 MaterialUBO は全 base file 共通既定値、
// sub-doc 07 §3.1 sub-step 7.2/7.3 確定値)。
#ifdef LL_VULKAN_GLSL
layout(set=0, binding=0) uniform PerFrame {
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    mat4 projection_matrix;
    mat3 normal_matrix;
};
layout(set=1, binding=0) uniform MaterialUBO {
    mat4 texture_matrix0;
};

layout(location=0) in vec3 position;
layout(location=1) in vec4 diffuse_color;
layout(location=2) in vec3 normal;
layout(location=3) in vec2 texcoord0;

layout(location=0) out vec3 vary_normal;
layout(location=1) out vec4 vertex_color;
layout(location=2) out vec2 vary_texcoord0;
layout(location=3) out vec3 vary_position;
#else
uniform mat3 normal_matrix;
uniform mat4 texture_matrix0;
uniform mat4 modelview_projection_matrix;

in vec3 position;
in vec4 diffuse_color;
in vec3 normal;
in vec2 texcoord0;

out vec3 vary_normal;

out vec4 vertex_color;
out vec2 vary_texcoord0;
out vec3 vary_position;
#endif

void passTextureIndex();

#ifndef LL_VULKAN_GLSL
uniform mat4 modelview_matrix;
#endif

#ifdef HAS_SKIN
mat4 getObjectSkinnedTransform();
#ifndef LL_VULKAN_GLSL
uniform mat4 projection_matrix;
#endif

#endif

void main()
{
#ifdef HAS_SKIN
    mat4 mat = getObjectSkinnedTransform();
    mat = modelview_matrix * mat;
    vec4 pos = mat * vec4(position.xyz, 1.0);
    vary_position = pos.xyz;
    gl_Position = projection_matrix * pos;
    vary_normal = normalize((mat*vec4(normal.xyz+position.xyz,1.0)).xyz-pos.xyz);
#else
    vary_position = (modelview_matrix * vec4(position.xyz, 1.0)).xyz;
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);
    vary_normal = normalize(normal_matrix * normal);
#endif

    vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;

    passTextureIndex();

    vertex_color = diffuse_color;
}
