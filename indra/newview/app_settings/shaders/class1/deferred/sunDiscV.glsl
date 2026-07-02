/**
 * @file sunDiscV.glsl
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
layout(location = 2) in vec2 texcoord0;
#else
in vec3 position;
in vec2 texcoord0;
#endif

#ifdef LL_VULKAN_GLSL
layout(location = 0) out vec2 vary_texcoord0;
layout(location = 1) out float sun_fade;
#else
out vec2 vary_texcoord0;
out float sun_fade;
#endif

void calcAtmospherics(vec3 eye_pos);

void main()
{
    //transform vertex
    vec3 offset = vec3(0, 0, 50);
    vec4 vert = vec4(position.xyz - offset, 1.0);
    vec4 pos  = modelview_projection_matrix*vert;

    sun_fade = smoothstep(0.3, 1.0, (position.z + 50) / 512.0f);

    // smash to *almost* far clip plane -- behind clouds but in front of stars
    pos.z = pos.w*0.999999;
    gl_Position = pos;

    calcAtmospherics(pos.xyz);

    vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;
}
