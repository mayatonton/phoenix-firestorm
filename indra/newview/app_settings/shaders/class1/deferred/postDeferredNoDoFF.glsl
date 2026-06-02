/**
 * @file postDeferredNoDoFF.glsl
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

#ifndef HAS_DOF_CHROMA
#define HAS_DOF_CHROMA 0
#endif

#ifdef LL_VULKAN_GLSL
layout(location=0) out vec4 frag_color;
#else
out vec4 frag_color;
#endif

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=4) uniform sampler2D diffuseRect;
#else
uniform sampler2D diffuseRect;
#endif
#ifndef DECL_DEPTH_MAP
#define DECL_DEPTH_MAP
#ifdef LL_VULKAN_GLSL
layout(set=0, binding=3) uniform sampler2D depthMap;
#else
uniform sampler2D depthMap;
#endif
#endif // DECL_DEPTH_MAP

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
#ifdef LL_VULKAN_GLSL
layout(location=1) in vec2 vary_fragcoord;
#else
in vec2 vary_fragcoord;
#endif

uniform float chroma_str;

//=================================
// borrowed noise from:
//  <https://www.shadertoy.com/view/4dS3Wd>
//  By Morgan McGuire @morgan3d, http://graphicscodex.com
//
float hash(float n) { return fract(sin(n) * 1e4); }
float hash(vec2 p) { return fract(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x)))); }

float noise(float x) {
    float i = floor(x);
    float f = fract(x);
    float u = f * f * (3.0 - 2.0 * f);
    return mix(hash(i), hash(i + 1.0), u);
}

float noise(vec2 x) {
    vec2 i = floor(x);
    vec2 f = fract(x);

    // Four corners in 2D of a tile
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    // Simple 2D lerp using smoothstep envelope between the values.
    // return vec3(mix(mix(a, b, smoothstep(0.0, 1.0, f.x)),
    //          mix(c, d, smoothstep(0.0, 1.0, f.x)),
    //          smoothstep(0.0, 1.0, f.y)));

    // Same code, with the clamps in smoothstep and common subexpressions
    // optimized away.
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

//=============================

vec3 clampHDRRange(vec3 color);

// <FS:AYA r30 Phase 3.8 Cinematic mount strategy C>
//   Cinematic — BD original: gradient/luma/depth-weighted radial CA with
//     per-channel dispersion weights (wR=0.60, wG=0.15, wB=1.00) and
//     dFdx/dFdy edge term; clamps to [0,1] before returning.
//   AY        — r30 P4 step 1 radial-offset sampling: per-channel
//     re-sample with radial shift = chroma_str * 0.0005 * r2, no
//     gradient/luma/depth modulation, no clamp.
// Two main() bodies; selection via #if AYASTORM_CINEMATIC.

#if AYASTORM_CINEMATIC

void main()
{
    vec4 diff = texture(diffuseRect, vary_fragcoord.xy);
#if HAS_DOF_CHROMA == 0
    vec3 col = diff.rgb;

    float luma = dot(col, vec3(0.2126, 0.7152, 0.0722));

    vec2 p = (vary_fragcoord.xy / screen_res.xy) * 2.0 - 1.0;
    float r = dot(p, p);

    float depth = texture(depthMap, vary_fragcoord.xy).r;
    float depthWeight = smoothstep(0.2, 0.8, depth);
    depthWeight *= depthWeight;

    vec3 gx = dFdx(col);
    vec3 gy = dFdy(col);
    vec3 grad = gx + gy;
    float edge = length(grad);

    // Blue disperses most, green least
    const float wR = 0.60;
    const float wG = 0.15;
    const float wB = 1.00;

    float ca =
        chroma_str *
        edge *
        smoothstep(0.15, 1.0, luma) *
        smoothstep(0.0, 1.2, r) *
        depthWeight;

    vec2 dir = normalize(p + 1e-5);

    col.r += ca * wR * (grad.r) * dir.x;
    col.g += ca * wG * (grad.g) * dir.x * 0.25; // very subtle
    col.b -= ca * wB * (grad.b) * dir.x;

    col.r += ca * wR * gy.r * 0.30;
    col.b -= ca * wB * gy.b * 0.30;

    diff.rgb = col;
    diff.rgb = clamp(diff.rgb, 0.0, 1.0);
#endif


#ifdef HAS_NOISE
    vec2 tc_q = vary_fragcoord.xy*screen_res*4.0;
    vec3 seed = (diff.rgb+vec3(1.0))*vec3(tc_q.xy, tc_q.x+tc_q.y);
    vec3 nz = vec3(noise(seed.rg), noise(seed.gb), noise(seed.rb));
    diff.rgb += nz*0.003;
#endif

    diff.rgb = clampHDRRange(diff.rgb);
    frag_color = diff;

    gl_FragDepth = texture(depthMap, vary_fragcoord.xy).r;
}

#else // AYASTORM_CINEMATIC

void main()
{
    vec4 diff = texture(diffuseRect, vary_fragcoord.xy);

// <AYAstorm r30 P4 step 1> BD NoDoF radial chromatic aberration
// Active only when HAS_DOF_CHROMA==0 (no DoF chroma coupling). Per-channel
// radial offset sampling: R pulled toward center, B pushed outward, shift
// grows toward screen edges. chroma_str=0 → no shift (free).
#if HAS_DOF_CHROMA == 0
    // vary_fragcoord is already [0,1] texcoord (see postDeferredNoTCV.glsl).
    vec2 p = vary_fragcoord.xy * 2.0 - 1.0;
    float r2 = dot(p, p);                       // 0 center → ~2 corners

    // Per-pixel shift in texcoord space. 0.0005 keeps the relationship
    // monotonic across usable range: chroma_str=10 → ~1% edge shift (subtle),
    // 50 → ~5% (clear), 100 → ~10% (strong). Above ~150 the offset taps
    // become incoherent and the image disintegrates.
    float shift = chroma_str * 0.0005 * r2;
    vec2  dir   = p;                            // radial from center, no normalize

    vec2 tcR = vary_fragcoord.xy - dir * shift;
    vec2 tcB = vary_fragcoord.xy + dir * shift;

    diff.r = texture(diffuseRect, tcR).r;
    // diff.g stays as the already-sampled center value.
    diff.b = texture(diffuseRect, tcB).b;
#endif
// </AYAstorm r30 P4 step 1>

#ifdef HAS_NOISE
    vec2 tc = vary_fragcoord.xy*screen_res*4.0;
    vec3 seed = (diff.rgb+vec3(1.0))*vec3(tc.xy, tc.x+tc.y);
    vec3 nz = vec3(noise(seed.rg), noise(seed.gb), noise(seed.rb));
    diff.rgb += nz*0.003;
#endif

    diff.rgb = clampHDRRange(diff.rgb);
    frag_color = diff;

    gl_FragDepth = texture(depthMap, vary_fragcoord.xy).r;
}

#endif // AYASTORM_CINEMATIC
// </FS:AYA>
