/**
 * @file glowV.glsl
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
uniform mat4 modelview_projection_matrix;
#endif

#ifdef LL_VULKAN_GLSL
layout(location = 0) in vec3 position;
#else
in vec3 position;
#endif

#ifdef LL_VULKAN_GLSL
layout(set = 1, binding = 0, std140) uniform Glow_PerProgramBind
{
    vec2 glowDelta;
    float glowStrength;
#ifndef _AYA_UM__pad0
#define _AYA_UM__pad0 1
    float _pad0;
#else
    float _dup_Glow__pad0;
#endif
};
#else
uniform vec2 glowDelta;
#endif

#ifdef LL_VULKAN_GLSL
layout(location = 0) out vec4 vary_texcoord0;
layout(location = 1) out vec4 vary_texcoord1;
layout(location = 2) out vec4 vary_texcoord2;
layout(location = 3) out vec4 vary_texcoord3;
#else
out vec4 vary_texcoord0;
out vec4 vary_texcoord1;
out vec4 vary_texcoord2;
out vec4 vary_texcoord3;
#endif

void main()
{
    gl_Position = vec4(position, 1.0);

    vec2 texcoord = position.xy * 0.5 + 0.5;


    vary_texcoord0.xy = texcoord + glowDelta*(-3.5);
    vary_texcoord1.xy = texcoord + glowDelta*(-2.5);
    vary_texcoord2.xy = texcoord + glowDelta*(-1.5);
    vary_texcoord3.xy = texcoord + glowDelta*(-0.5);
    vary_texcoord0.zw = texcoord + glowDelta*(0.5);
    vary_texcoord1.zw = texcoord + glowDelta*(1.5);
    vary_texcoord2.zw = texcoord + glowDelta*(2.5);
    vary_texcoord3.zw = texcoord + glowDelta*(3.5);
}
