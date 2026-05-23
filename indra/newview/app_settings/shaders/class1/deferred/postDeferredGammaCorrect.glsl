/**
 * @file postDeferredGammaCorrect.glsl
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

out vec4 frag_color;

uniform sampler2D diffuseRect;

// <AYAstorm r30 P5 transparent-DoF C-(a) R-1>
// Symmetric to postDeferredTonemap.glsl for the non-HDR gammaCorrect path.
// See postDeferredTonemap.glsl for the full rationale.
uniform sampler2D aya_alpha_plate;
uniform bool      aya_alpha_plate_enabled;
// </AYAstorm r30 P5 transparent-DoF C-(a) R-1>

uniform float gamma;
uniform vec2 screen_res;
in vec2 vary_fragcoord;

vec3 linear_to_srgb(vec3 cl);

vec3 legacyGamma(vec3 color)
{
    vec3 c = 1. - clamp(color, vec3(0.), vec3(1.));
    c = 1. - pow(c, vec3(gamma)); // s/b inverted already CPU-side

    return c;
}

void main()
{
    //this is the one of the rare spots where diffuseRect contains linear color values (not sRGB)
    vec4 diff = texture(diffuseRect, vary_fragcoord);

    // <AYAstorm r30 P5 transparent-DoF C-(a) R-1> pre-gamma plate over-blend.
    // Composite linear premul plate before linear_to_srgb so the merged
    // result is sRGB-encoded uniformly. See postDeferredTonemap.glsl.
    if (aya_alpha_plate_enabled)
    {
        vec4 plate = texture(aya_alpha_plate, vary_fragcoord);
        diff.rgb = plate.rgb + diff.rgb * (1.0 - plate.a);
    }
    // </AYAstorm r30 P5 transparent-DoF C-(a) R-1>

    diff.rgb = linear_to_srgb(diff.rgb);

#ifdef LEGACY_GAMMA
    diff.rgb = legacyGamma(diff.rgb);
#endif

    diff.rgb = clamp(diff.rgb, vec3(0.0), vec3(1.0));
    frag_color = diff;
}

