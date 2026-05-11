/**
 * @file sunDiscF.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2005, Linden Research, Inc.
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

out vec4 frag_data[4];

vec3 srgb_to_linear(vec3 c);

uniform sampler2D diffuseMap;
uniform sampler2D altDiffuseMap;
uniform float blend_factor; // interp factor between sunDisc A/B
uniform float sun_disc_boost;        // AYAstorm r14: HDR boost multiplier
uniform int   sun_disc_boost_enabled; // AYAstorm r14: master sentinel
in vec2 vary_texcoord0;
in float sun_fade;

void main()
{
    vec4 sunDiscA = texture(diffuseMap, vary_texcoord0.xy);
    vec4 sunDiscB = texture(altDiffuseMap, vary_texcoord0.xy);
    vec4 c     = mix(sunDiscA, sunDiscB, blend_factor);

    // AYAstorm r14: sun disc HDR boost. RGB feeds the HDR scene buffer (most
    // is clipped by tonemap); alpha is what generateGlow extracts (pipeline
    // hardcodes GLOW_MIN_LUMINANCE=9999, so the lum path is dead and alpha is
    // the only driver). Boosting alpha is what actually produces the halo.
    if (sun_disc_boost_enabled != 0)
    {
        c.rgb *= sun_disc_boost;
        c.a   = clamp(c.a * sun_disc_boost, 0.0, 1.0);
    }

    // SL-9806 stars poke through
    //c.a *= sun_fade;

    frag_data[0] = vec4(0);
    frag_data[1] = vec4(0.0f);
    frag_data[2] = vec4(0.0, 1.0, 0.0, GBUFFER_FLAG_SKIP_ATMOS);
#if defined(HAS_EMISSIVE)
    frag_data[0] = vec4(0);
    frag_data[3] = c;
#else
    frag_data[0] = c;
#endif
}

