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
layout(set = 0, binding = 4, std140) uniform PreviewAmbient_PerShaderBind
{
#ifndef _AYA_UM_ambient_color
#define _AYA_UM_ambient_color 1
    vec4 ambient_color;
#else
    vec4 _dup_PreviewAmbient_ambient_color;
#endif
};
#else
uniform vec4 ambient_color; // <FS:Beq/> add ambient color to preview shader
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
layout(location = 2) in vec2 texcoord0;
#else
in vec3 position;
in vec3 normal;
in vec2 texcoord0;
#endif

#ifdef LL_VULKAN_GLSL
layout(set = 1, binding = 51, std140) uniform DrawColor_PerShaderBind
{
#ifndef _AYA_UM_color
#define _AYA_UM_color 1
    vec4 color;
#else
    vec4 _dup_DrawColor_color;
#endif
};
#else
uniform vec4 color;
#endif

#ifdef LL_VULKAN_GLSL
layout(location = 0) out vec4 vertex_color;
layout(location = 1) out vec2 vary_texcoord0;
#else
out vec4 vertex_color;
out vec2 vary_texcoord0;
#endif

#ifdef LL_VULKAN_GLSL
layout(set = 1, binding = 0, std140) uniform Preview_PerProgramBind
{
#ifndef _AYA_UM_light_position
#define _AYA_UM_light_position 1
    vec4 light_position[8];
#else
    vec4 _dup_Preview_light_position[8];
#endif
#ifndef _AYA_UM_light_diffuse
#define _AYA_UM_light_diffuse 1
    vec3 light_diffuse[8];
#else
    vec3 _dup_Preview_light_diffuse[8];
#endif
};
#else
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
