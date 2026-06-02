/**
 * @file postDeferredTonemap.glsl
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

out vec4 frag_color;

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=4) uniform sampler2D diffuseRect;
#else
uniform sampler2D diffuseRect;
#endif

#ifndef DECL_VARY_FRAGCOORD
#define DECL_VARY_FRAGCOORD
in vec2 vary_fragcoord;
#endif // DECL_VARY_FRAGCOORD

#ifdef GAMMA_CORRECT
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-24 Phase D: FrameAtmosphere_Skybox 削除
// (η-14 Path G 範式継承)。FrameAtmosphere_Lighting (set=0 binding=2 from
// atmosphericsHelpersV/atmosphericsFuncs) と member 名 sunlight_color 等 17 件
// が global scope で重複 = nameless block name collision。この file の main() で
// 必要な Skybox 単独 member は `gamma` のみ (legacyGamma() 内、#ifdef GAMMA_CORRECT
// で gated)。新規 PerProgramUBO_GammaCorrect (set=2, binding=2、η-3 §3.2 PerDrawUBO
// 範式類、postDeferredGammaCorrect.glsl と同 layout) で gamma 専用 UBO を declare。
#ifndef PER_PROGRAM_UBO_GAMMA_CORRECT_DEFINED
#define PER_PROGRAM_UBO_GAMMA_CORRECT_DEFINED 1
layout(set=2, binding=2, std140) uniform PerProgramUBO_GammaCorrect {
    float gamma;
    float _pad_gc0;
    float _pad_gc1;
    float _pad_gc2;
};
#endif
#else
uniform float gamma;
#endif
#endif

#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-25 Phase 1b: color grading 6 件 bare uniform を
// PerProgramUBO_ColorGrading (set=2, binding=4) に集約。applyColorGrading() 経由で main() 参照
// (color_saturation/color_contrast/color_temperature/color_brightness/color_grading_lut_intensity/
//  color_grading_lut_enabled) で dead でないため η-23 §3.1 GL-only wrap 不適用、η-24 §3.2
// PerProgramUBO_GammaCorrect 派生範式類。set=2 namespace 連番継続
// (η-24 binding=2 GammaCorrect / η-25 binding=3 AlphaParams / η-25 binding=4 ColorGrading)。
// float×5 + int×1 + float×2 pad = 32-byte (2 vec4 chunk) std140 整合。
#ifndef PER_PROGRAM_UBO_COLOR_GRADING_DEFINED
#define PER_PROGRAM_UBO_COLOR_GRADING_DEFINED 1
layout(set=2, binding=4, std140) uniform PerProgramUBO_ColorGrading {
    float color_saturation;
    float color_contrast;
    float color_temperature;
    float color_brightness;
    float color_grading_lut_intensity;
    int   color_grading_lut_enabled;
    float _pad_cg0;
    float _pad_cg1;
};
#endif
#else
uniform float color_saturation;
uniform float color_contrast;
uniform float color_temperature;
uniform float color_brightness;
#endif

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=42) uniform sampler3D color_grading_lut;
#else
uniform sampler3D color_grading_lut;
uniform float color_grading_lut_intensity;
uniform int color_grading_lut_enabled;
#endif

vec3 applyLUT(sampler3D lut, vec3 color, int size)
{
    float scale  = float(size - 1) / float(size);
    float offset = 0.5 / float(size);
    vec3 coord   = clamp(color, 0.0, 1.0) * scale + offset;
    return texture(lut, coord).rgb;
}

vec3 applyColorGrading(vec3 color)
{
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color = mix(vec3(luma), color, color_saturation);
    color = clamp((color - 0.5) * color_contrast + 0.5, 0.0, 1.0);
    color.r *= 1.0 + color_temperature * 0.3;
    color.b *= 1.0 - color_temperature * 0.3;
    color = clamp(color + color_brightness, 0.0, 1.0);
    if (color_grading_lut_enabled != 0)
    {
        vec3 lut_color = applyLUT(color_grading_lut, color, 33);
        color = mix(color, lut_color, color_grading_lut_intensity);
    }
    return color;
}

vec3 linear_to_srgb(vec3 cl);
vec3 toneMap(vec3 color);

vec3 clampHDRRange(vec3 color);

#ifdef GAMMA_CORRECT
vec3 legacyGamma(vec3 color)
{
    vec3 c = 1. - clamp(color, vec3(0.), vec3(1.));
    c = 1. - pow(c, vec3(gamma)); // s/b inverted already CPU-side

    return c;
}
#endif

void main()
{
    //this is the one of the rare spots where diffuseRect contains linear color values (not sRGB)
    vec4 diff = texture(diffuseRect, vary_fragcoord);

#ifndef NO_POST
    diff.rgb = toneMap(diff.rgb);
#else
    diff.rgb = clamp(diff.rgb, vec3(0.0), vec3(1.0));
#endif

    diff.rgb = applyColorGrading(diff.rgb);

#ifdef GAMMA_CORRECT
    diff.rgb = linear_to_srgb(diff.rgb);

#ifdef LEGACY_GAMMA
    diff.rgb = legacyGamma(diff.rgb);
#endif

#endif

    diff.rgb = clamp(diff.rgb, vec3(0.0), vec3(1.0)); // We should always be 0-1 past this point

    //debugExposure(diff.rgb);
    frag_color = diff;
}

