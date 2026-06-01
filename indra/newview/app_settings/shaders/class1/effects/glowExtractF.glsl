/**
 * @file glowExtractF.glsl
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

/*[EXTRA_CODE_HERE]*/

#ifndef HAS_NOISE
#define HAS_NOISE 0
#endif

#ifdef LL_VULKAN_GLSL
layout(location=0) out vec4 frag_color;
#else
out vec4 frag_color;
#endif

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=1) uniform sampler2D diffuseMap;
#else
uniform sampler2D diffuseMap;
#endif
#if HAS_NOISE
#ifdef LL_VULKAN_GLSL
layout(set=0, binding=17) uniform sampler2D glowNoiseMap;
#else
uniform sampler2D glowNoiseMap;
#endif
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
uniform vec2 screen_res;
#endif
#endif
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-6: Glow non-opaque uniforms UBO wrap (Cluster B)
layout(set=3, binding=20, std140) uniform GlowExtractFParamUBO_Legacy {
    vec3 lumWeights;
    float minLuminance;
    vec3 warmthWeights;
    float maxExtractAlpha;
    float warmthAmount;
};
#else
uniform float minLuminance;
uniform float maxExtractAlpha;
uniform vec3 lumWeights;
uniform vec3 warmthWeights;
uniform float warmthAmount;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=0) in vec2 vary_texcoord0;
#else
in vec2 vary_texcoord0;
#endif

void main()
{
    vec4 col = texture(diffuseMap, vary_texcoord0.xy);
    /// CALCULATING LUMINANCE (Using NTSC lum weights)
    /// http://en.wikipedia.org/wiki/Luma_%28video%29
    float lum = smoothstep(minLuminance, minLuminance+1.0, dot(col.rgb, lumWeights ) );
    float warmth = smoothstep(minLuminance, minLuminance+1.0, max(col.r * warmthWeights.r, max(col.g * warmthWeights.g, col.b * warmthWeights.b)) );

#if HAS_NOISE
    float TRUE_NOISE_RES = 128; // See mTrueNoiseMap
    // *NOTE: Usually this is vary_fragcoord not vary_texcoord0, but glow extraction is in screen space
    vec3 glow_noise = texture(glowNoiseMap, vary_texcoord0.xy * (screen_res / TRUE_NOISE_RES)).xyz;
    // Dithering. Reduces banding effects in the reduced precision glow buffer.
    float NOISE_DEPTH = 64.0;
    col.rgb += glow_noise / NOISE_DEPTH;
    col.rgb = max(col.rgb, vec3(0));
#endif
    frag_color.rgb = col.rgb;
    frag_color.a = max(col.a, mix(lum, warmth, warmthAmount) * maxExtractAlpha);

}
