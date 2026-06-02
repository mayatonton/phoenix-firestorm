/**
 * @file previewV.glsl
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
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-6: previewV non-opaque uniforms UBO wrap (Cluster F)
layout(set=3, binding=40, std140) uniform PreviewVParamUBO_Legacy {
    mat4 texture_matrix0;
    vec4 ambient_color;
    vec4 color;
    vec4 light_position[8];
    vec4 light_direction[8];
    vec4 light_attenuation[8];
    vec4 light_diffuse[8];
};
#else
uniform mat4 texture_matrix0;
uniform vec4 ambient_color; // <FS:Beq/> add ambient color to preview shader
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
layout(location=2) in vec2 texcoord0;
#else
in vec2 texcoord0;
#endif

#ifndef LL_VULKAN_GLSL
uniform vec4 color;
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

#ifndef LL_VULKAN_GLSL
uniform vec4 light_position[8];
uniform vec3 light_direction[8];
uniform vec3 light_attenuation[8];
uniform vec3 light_diffuse[8];
#endif

//===================================================================================================
//declare these here explicitly to separate them from atmospheric lighting elsewhere to work around
//drivers that are picky about functions being declared but not defined even if they aren't called
float calcDirectionalLight(vec3 n, vec3 l)
{
    float a = max(dot(n,l),0.0);
    return a;
}

//====================================================================================================


#ifdef HAS_SKIN
mat4 getObjectSkinnedTransform();
#ifndef LL_VULKAN_GLSL
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
#endif
#endif

void main()
{
    vec3 norm;
#ifdef HAS_SKIN
    mat4 mat = getObjectSkinnedTransform();
    mat = modelview_matrix * mat;
    vec4 pos = mat * vec4(position.xyz, 1.0);
    gl_Position = projection_matrix * pos;
    norm = normalize((mat*vec4(normal.xyz+position.xyz,1.0)).xyz-pos.xyz);
#else
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);
    norm = normalize(normal_matrix * normal);
#endif

    vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;

    // <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> FS:Beq added an
    // ambient_color tint so the preview inherits the scene ambient. BD
    // starts from black so the preview reads as a neutral light study.
    // The ambient_color uniform stays declared in both modes (harmless
    // when unbound on the C++ side).
#if AYASTORM_CINEMATIC
    vec4 col = vec4(0,0,0,1);
#else
    vec4 col = ambient_color; // <FS:Beq/> add ambient color to preview shader
#endif
    // </FS:AYA>

    // Collect normal lights (need to be divided by two, as we later multiply by 2)
    col.rgb += light_diffuse[1].rgb * calcDirectionalLight(norm, light_position[1].xyz);
    col.rgb += light_diffuse[2].rgb * calcDirectionalLight(norm, light_position[2].xyz);
    col.rgb += light_diffuse[3].rgb * calcDirectionalLight(norm, light_position[3].xyz);

    vertex_color = col*color;
}
