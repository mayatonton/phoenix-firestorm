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
uniform mat4 modelview_projection_matrix;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=0) in vec3 position;
#else
in vec3 position;
#endif

#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-6: Glow non-opaque uniforms UBO wrap (Cluster B)
layout(set=3, binding=19, std140) uniform GlowVParamUBO_Legacy {
    vec2 glowDelta;
};
#else
uniform vec2 glowDelta;
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
#ifdef LL_VULKAN_GLSL
layout(location=16) out vec4 vary_texcoord2;
#else
out vec4 vary_texcoord2;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=17) out vec4 vary_texcoord3;
#else
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
