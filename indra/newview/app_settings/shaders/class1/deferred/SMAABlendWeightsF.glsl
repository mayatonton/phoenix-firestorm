/**
 * @file SMAABlendWeightsF.glsl
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

/*[EXTRA_CODE_HERE]*/

#ifdef LL_VULKAN_GLSL
layout(location=0) out vec4 frag_color;
#else
out vec4 frag_color;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=0) in vec2 vary_texcoord0;
#else
in vec2 vary_texcoord0;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=26) in vec2 vary_pixcoord;
#else
in vec2 vary_pixcoord;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=14) in vec4 vary_offset[3];
#else
in vec4 vary_offset[3];
#endif

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=64) uniform sampler2D edgesTex;
layout(set=1, binding=65) uniform sampler2D areaTex;
layout(set=1, binding=63) uniform sampler2D searchTex;
#else
uniform sampler2D edgesTex;
uniform sampler2D areaTex;
uniform sampler2D searchTex;
#endif
// <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> BD wires
// subsampleIndices as a uniform for SMAA T2x jitter. AY hard-codes
// vec4(0.0) since the AY T2x path doesn't use the SMAA helper.
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-15: SMAABlendWeightsFParamUBO_Legacy wrap (η-8 §3.4 範式継承)
#if AYASTORM_CINEMATIC
#ifdef LL_VULKAN_GLSL
#ifndef SMAA_BLEND_WEIGHTS_F_PARAM_UBO_LEGACY_DEFINED
#define SMAA_BLEND_WEIGHTS_F_PARAM_UBO_LEGACY_DEFINED 1
layout(set=3, binding=62, std140) uniform SMAABlendWeightsFParamUBO_Legacy {
    vec4 subsampleIndices;
};
#endif
#else
uniform vec4 subsampleIndices;
#endif
#endif
// </FS:AYA>

vec4 SMAABlendingWeightCalculationPS(vec2 texcoord,
                                       vec2 pixcoord,
                                       vec4 offset[3],
                                       sampler2D edgesTex,
                                       sampler2D areaTex,
                                       sampler2D searchTex,
                                       vec4 subsampleIndices);

void main()
{
    frag_color = SMAABlendingWeightCalculationPS(vary_texcoord0,
                                                 vary_pixcoord,
                                                 vary_offset,
                                                 edgesTex,
                                                 areaTex,
                                                 searchTex,
                                                 // <FS:AYA r30 Phase 3.8>
#if AYASTORM_CINEMATIC
                                                 subsampleIndices
#else
                                                 vec4(0.0)
#endif
                                                 // </FS:AYA>
                                                 );
}

