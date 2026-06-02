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
uniform mat4 modelview_matrix;
uniform mat4 modelview_projection_matrix;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=0) in vec3 position;
#else
in vec3 position;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=1) in vec3 normal;
#else
in vec3 normal;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=6) in vec4 diffuse_color;
#else
in vec4 diffuse_color;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=3) in vec2 texcoord1;
#else
in vec2 texcoord1;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=26) out vec3 pos;
#else
out vec3 pos;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=4) out vec3 vary_normal;
#else
out vec3 vary_normal;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=0) out vec4 vary_texcoord0;
#else
out vec4 vary_texcoord0;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=5) out vec4 vary_texcoord1;
#else
out vec4 vary_texcoord1;
#endif

// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-15: TerrainVParamUBO_Legacy wrap (η-8 §3.4 範式継承)
#ifdef LL_VULKAN_GLSL
#ifndef TERRAIN_V_PARAM_UBO_LEGACY_DEFINED
#define TERRAIN_V_PARAM_UBO_LEGACY_DEFINED 1
layout(set=3, binding=61, std140) uniform TerrainVParamUBO_Legacy {
    vec4 object_plane_s;
    vec4 object_plane_t;
};
#endif
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
