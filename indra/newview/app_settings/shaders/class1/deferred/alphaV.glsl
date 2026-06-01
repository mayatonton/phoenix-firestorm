/**
 * @file alphaV.glsl
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

#define INDEXED 1
#define NON_INDEXED 2
#define NON_INDEXED_NO_COLOR 3

#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-5: FrameViewProj guard wrap (η-1 §3.1 範式継承)
#ifndef FRAME_VIEW_PROJ_DEFINED
#define FRAME_VIEW_PROJ_DEFINED 1
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
#endif
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
#ifndef LL_VULKAN_GLSL
uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
uniform mat4 modelview_projection_matrix;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=0) in vec3 position;
#else
in vec3 position;
#endif

#ifdef USE_INDEXED_TEX
void passTextureIndex();
#endif

#ifdef LL_VULKAN_GLSL
layout(location=1) in vec3 normal;
#else
in vec3 normal;
#endif

#ifdef USE_VERTEX_COLOR
#ifdef LL_VULKAN_GLSL
layout(location=6) in vec4 diffuse_color;
#else
in vec4 diffuse_color;
#endif
#endif

#ifdef LL_VULKAN_GLSL
layout(location=2) in vec2 texcoord0;
#else
in vec2 texcoord0;
#endif

#ifdef HAS_SKIN
mat4 getObjectSkinnedTransform();
#else
#ifdef IS_AVATAR_SKIN
mat4 getSkinnedTransform();
#endif
#endif

#ifdef LL_VULKAN_GLSL
layout(location=1) out vec3 vary_fragcoord;
#else
out vec3 vary_fragcoord;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=3) out vec3 vary_position;
#else
out vec3 vary_position;
#endif

#ifdef USE_VERTEX_COLOR
#ifdef LL_VULKAN_GLSL
layout(location=2) out vec4 vertex_color;
#else
out vec4 vertex_color;
#endif
#endif

#ifdef LL_VULKAN_GLSL
layout(location=0) out vec2 vary_texcoord0;
#else
out vec2 vary_texcoord0;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=20) out vec3 vary_norm;
#else
out vec3 vary_norm;
#endif

uniform float near_clip;

void main()
{
    vec4 pos;
    vec3 norm;

    //transform vertex
#ifdef HAS_SKIN
    mat4 trans = getObjectSkinnedTransform();
    trans = modelview_matrix * trans;

    pos = trans * vec4(position.xyz, 1.0);

    norm = position.xyz + normal.xyz;
    norm = normalize((trans * vec4(norm, 1.0)).xyz - pos.xyz);
    vec4 frag_pos = projection_matrix * pos;
    gl_Position = frag_pos;
#else

#ifdef IS_AVATAR_SKIN
    mat4 trans = getSkinnedTransform();
    vec4 pos_in = vec4(position.xyz, 1.0);
    pos.x = dot(trans[0], pos_in);
    pos.y = dot(trans[1], pos_in);
    pos.z = dot(trans[2], pos_in);
    pos.w = 1.0;

    norm.x = dot(trans[0].xyz, normal);
    norm.y = dot(trans[1].xyz, normal);
    norm.z = dot(trans[2].xyz, normal);
    norm = normalize(norm);

    vec4 frag_pos = projection_matrix * pos;
    gl_Position = frag_pos;
#else
    norm = normalize(normal_matrix * normal);
    vec4 vert = vec4(position.xyz, 1.0);
    pos = (modelview_matrix * vert);
    gl_Position = modelview_projection_matrix*vec4(position.xyz, 1.0);
#endif //IS_AVATAR_SKIN

#endif // HAS_SKIN

#ifdef USE_INDEXED_TEX
    passTextureIndex();
#endif

    vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;

    vary_norm = norm;
    vary_position = pos.xyz;

#ifdef USE_VERTEX_COLOR
    vertex_color = diffuse_color;
#endif

#ifdef HAS_SKIN
    vary_fragcoord.xyz = frag_pos.xyz + vec3(0,0,near_clip);
#else

#ifdef IS_AVATAR_SKIN
    vary_fragcoord.xyz = pos.xyz + vec3(0,0,near_clip);
#else
    pos = modelview_projection_matrix * vert;
    vary_fragcoord.xyz = pos.xyz + vec3(0,0,near_clip);
#endif

#endif

}

