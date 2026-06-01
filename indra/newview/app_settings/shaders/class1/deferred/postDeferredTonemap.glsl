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

in vec2 vary_fragcoord;

#ifdef GAMMA_CORRECT
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-4: FrameAtmosphere_Skybox per-group rename (η-3 §3.2 範式)
#ifndef FRAME_ATMOSPHERE_SKYBOX_DEFINED
#define FRAME_ATMOSPHERE_SKYBOX_DEFINED 1
layout(set=0, binding=2, std140) uniform FrameAtmosphere_Skybox {
    vec3  sunlight_color;
    float scene_light_strength;
    vec3  moonlight_color;
    float haze_density;
    vec3  ambient_color;
    float density_multiplier;
    vec3  blue_horizon;
    float distance_multiplier;
    vec3  blue_density;
    float max_y;
    vec3  glow;
    float sky_sunlight_scale;
    float sky_ambient_scale;
    float sky_hdr_scale;
    int   classic_mode;
    int   cube_snapshot;
    float minimum_alpha;
    float max_cof;
    float haze_horizon;
    float gamma;
    float _pad_atm0;
    float _pad_atm1;
};
#endif
#else
uniform float gamma;
#endif
#endif

uniform float color_saturation;
uniform float color_contrast;
uniform float color_temperature;
uniform float color_brightness;

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=42) uniform sampler3D color_grading_lut;
#else
uniform sampler3D color_grading_lut;
#endif
uniform float color_grading_lut_intensity;
uniform int color_grading_lut_enabled;

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

