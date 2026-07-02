/**
 * @file SMAAResolveF.glsl
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

// AYAstorm r30 P2: SMAA T2x resolve pass fragment shader.
// Calls SMAAResolvePS() defined in SMAA.glsl (LL upstream, sourced from
// iryoku/smaa reference impl, Crytek/Jimenez SIGGRAPH 2011). The function
// body is unchanged from upstream; AYAstorm only provides this thin wrapper
// (which BD did not ship — gSMAAResolveProgram references this file but the
// file itself was missing from the BD repo, breaking SMAA T2x on BD).
//
// Inputs:
//   diffuseRect       = current frame post-neighborhood-blend color
//   previousColorTex  = history (previous frame post-resolve color)
//   velocityTex       = velocity buffer (RG = NDC delta, populated by P2 velocity pass)
//
// Output: 2-frame blended color, written to mScreen for downstream tonemap/post.

/*[EXTRA_CODE_HERE]*/

#ifdef LL_VULKAN_GLSL
layout(location = 0) out vec4 frag_color;

layout(location = 0) in vec2 vary_texcoord0;

layout(set = 1, binding = 1) uniform sampler2D diffuseRect;
layout(set = 1, binding = 2) uniform sampler2D previousColorTex;
#if SMAA_REPROJECTION
layout(set = 1, binding = 3) uniform sampler2D velocityTex;
#endif
#else
out vec4 frag_color;

in vec2 vary_texcoord0;

uniform sampler2D diffuseRect;
uniform sampler2D previousColorTex;
#if SMAA_REPROJECTION
uniform sampler2D velocityTex;
#endif
#endif

#define float4 vec4
#define float2 vec2
#define SMAATexture2D(tex) sampler2D tex

float4 SMAAResolvePS(float2 texcoord,
                     SMAATexture2D(currentColorTex),
                     SMAATexture2D(previousColorTex)
                     #if SMAA_REPROJECTION
                     , SMAATexture2D(velocityTex)
                     #endif
                     );

void main()
{
    frag_color = SMAAResolvePS(vary_texcoord0,
                               diffuseRect,
                               previousColorTex
                               #if SMAA_REPROJECTION
                               , velocityTex
                               #endif
                               );
}
