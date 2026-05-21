/**
 * @file dofCombineF.glsl
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
uniform sampler2D lightMap;

uniform mat4 inv_proj;
uniform vec2 screen_res;

uniform float max_cof;
uniform float res_scale;
uniform float dof_width;
uniform float dof_height;

// <AYAstorm r30 P5 transparent-DoF C-(a)>
// Alpha BLEND plate (gPipeline.mAYAAlphaColor) — premultiplied color/coverage
// from forward alpha pass redirected away from mRT->screen so the opaque-only
// scene goes through cofF/HQDoFF unobstructed. Composited back as sharp "over"
// on top of the DoF result here. Guarded by aya_alpha_plate_enabled so the
// shader is a no-op when C-(a) is not allocated (aux / probe paths).
uniform sampler2D aya_alpha_plate;
uniform bool      aya_alpha_plate_enabled;
// </AYAstorm r30 P5 transparent-DoF C-(a)>

in vec2 vary_fragcoord;

vec4 dofSample(sampler2D tex, vec2 tc)
{
    tc.x = min(tc.x, dof_width);
    tc.y = min(tc.y, dof_height);

    return texture(tex, tc);
}

void main()
{
    vec2 tc = vary_fragcoord.xy;

    vec4 dof = dofSample(diffuseRect, vary_fragcoord.xy*res_scale);

    vec4 diff = texture(lightMap, vary_fragcoord.xy);

    float a = min(abs(diff.a*2.0-1.0) * max_cof*res_scale*res_scale, 1.0);

    if (a > 0.25 && a < 0.75)
    { //help out the transition a bit
        float sc = a/res_scale;

        vec4 col;
        col = texture(lightMap, vary_fragcoord.xy+vec2(sc,sc)/screen_res);
        col += texture(lightMap, vary_fragcoord.xy+vec2(-sc,sc)/screen_res);
        col += texture(lightMap, vary_fragcoord.xy+vec2(sc,-sc)/screen_res);
        col += texture(lightMap, vary_fragcoord.xy+vec2(-sc,-sc)/screen_res);

        diff = mix(diff, col*0.25, a);
    }

    frag_color = mix(diff, dof, a);

    // <AYAstorm r30 P5 transparent-DoF C-(a)> Over-blend the alpha plate.
    // mAYAAlphaColor was filled with forward alpha BLEND using factor
    // (SRC_ALPHA, 1-SRC_ALPHA) for color + (ONE, 1-SRC_ALPHA) for alpha into a
    // clear-to-(0,0,0,0) target, which produces premultiplied output:
    //   plate.rgb = src.rgb * src.a (accumulated)
    //   plate.a   = src.a           (accumulated as coverage)
    // Standard "over" composite is therefore:
    //   out.rgb = plate.rgb + scene.rgb * (1 - plate.a)
    // Preserve frag_color.a (= scene glow signal) untouched — the alpha plate
    // has no glow contribution and downstream combineGlow reads .a from this RT.
    if (aya_alpha_plate_enabled)
    {
        vec4 plate = texture(aya_alpha_plate, vary_fragcoord.xy);
        frag_color.rgb = plate.rgb + frag_color.rgb * (1.0 - plate.a);
    }
    // </AYAstorm r30 P5 transparent-DoF C-(a)>
}
