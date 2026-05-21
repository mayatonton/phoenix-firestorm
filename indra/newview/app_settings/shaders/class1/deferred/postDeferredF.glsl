/**
 * @file postDeferredF.glsl
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

// <AYAstorm r30 BD-preserving alpha edge guard> depthMap uniform を追加。
// gDeferredPostProgram は既に pipeline.cpp:9804 で DEFERRED_DEPTH を bind 済
// (BD HQ path 用に r30 P4 step 4 で追加された)。標準 postDeferredF からも同じ
// depth texture を利用できる。Cinematic OFF / 非 AYAstorm では HAS_ALPHA_EDGE_GUARD
// permutation が立たないので uniform は宣言のみで未参照、ドライバ最適化で剥がれる。
#if HAS_ALPHA_EDGE_GUARD
uniform sampler2D depthMap;
#endif
// </AYAstorm>

uniform mat4 inv_proj;
uniform vec2 screen_res;
uniform float max_cof;
uniform float res_scale;

// <AYAstorm r30 P4 step 1> BD chroma uniform (gated by HAS_DOF_CHROMA permutation)
uniform float chroma_str;
// </AYAstorm r30 P4 step 1>

in vec2 vary_fragcoord;

void dofSample(inout vec4 diff, inout float w, float min_sc, vec2 tc
// <AYAstorm r30 BD-preserving alpha edge guard> center pixel depth を引数で渡す。
// guard 式 (s.a <= depth*0.50) は BD HQ path postDeferredHQDoFF.glsl:65 と同一。
#if HAS_ALPHA_EDGE_GUARD
    , float depth
#endif
// </AYAstorm>
)
{
    vec4 s = texture(diffuseRect, tc);

    float sc = abs(s.a*2.0-1.0)*max_cof;

// <AYAstorm r30 P4 step 1> BD HAS_DOF_CHROMA: per-channel R/G/B offset sampling
#if HAS_DOF_CHROMA
    vec3 col_offset = vec3(0.0015, 0.0000, 0.0005);
    float mult = sc * (chroma_str * 0.2);
    col_offset *= vec3(mult);

    s.r = texture(diffuseRect, tc + vec2(col_offset.x)).r;
    s.g = texture(diffuseRect, tc + vec2(col_offset.y)).g;
    s.b = texture(diffuseRect, tc + vec2(col_offset.z)).b;
    s.a = texture(diffuseRect, tc).a;
#endif
// </AYAstorm r30 P4 step 1>

    if (sc > min_sc //sampled pixel is more "out of focus" than current sample radius
// <AYAstorm r30 BD-preserving alpha edge guard> AND で BD HQ 由来 depth guard。
// 既存式 `sc > min_sc` は一切書き換えず、AND 1 条件のみ追加。permutation OFF で完全 no-op。
#if HAS_ALPHA_EDGE_GUARD
        && (s.a <= depth*0.50)
#endif
// </AYAstorm>
        )
    {
        float wg = 0.25;

        // de-weight dull areas to make highlights 'pop'
        wg += s.r+s.g+s.b;

        diff += wg*s;

        w += wg;
    }
}

void dofSampleNear(inout vec4 diff, inout float w, float min_sc, vec2 tc)
{
    vec4 s = texture(diffuseRect, tc);

    float wg = 0.25;

    // de-weight dull areas to make highlights 'pop'
    wg += s.r+s.g+s.b;

    diff += wg*s;

    w += wg;
}

vec3 clampHDRRange(vec3 color);

void main()
{
    vec2 tc = vary_fragcoord.xy;

    vec4 diff = texture(diffuseRect, vary_fragcoord.xy);

// <AYAstorm r30 BD-preserving alpha edge guard>
#if HAS_ALPHA_EDGE_GUARD
    float depth = texture(depthMap, tc).r;
#endif
// </AYAstorm>

    {
        float w = 1.0;

        float sc = (diff.a*2.0-1.0)*max_cof;

        float PI = 3.14159265358979323846264;

        // sample quite uniformly spaced points within a circle, for a circular 'bokeh'
// <AYAstorm r30 P4 step 1> BD FRONT_BLUR: gate the sc>0.5 (front-CoF) branch on permutation
#if FRONT_BLUR
        if (sc > 0.5)
        {
            while (sc > 0.5)
            {
                int its = int(max(1.0,(sc*3.7)));
                for (int i=0; i<its; ++i)
                {
                    float ang = sc+i*2*PI/its; // sc is added for rotary perturbance
                    float samp_x = sc*sin(ang);
                    float samp_y = sc*cos(ang);
                    // you could test sample coords against an interesting non-circular aperture shape here, if desired.
                    dofSampleNear(diff, w, sc, vary_fragcoord.xy + (vec2(samp_x,samp_y) / screen_res));
                }
                sc -= 1.0;
            }
        }
        else if (sc < -0.5)
#else
        if (sc < -0.5)
#endif
// </AYAstorm r30 P4 step 1>
        {
            sc = abs(sc);
            while (sc > 0.5)
            {
                int its = int(max(1.0,(sc*3.7)));
                for (int i=0; i<its; ++i)
                {
                    float ang = sc+i*2*PI/its; // sc is added for rotary perturbance
                    float samp_x = sc*sin(ang);
                    float samp_y = sc*cos(ang);
                    // you could test sample coords against an interesting non-circular aperture shape here, if desired.
                    dofSample(diff, w, sc, vary_fragcoord.xy + (vec2(samp_x,samp_y) / screen_res)
// <AYAstorm r30 BD-preserving alpha edge guard>
#if HAS_ALPHA_EDGE_GUARD
                        , depth
#endif
// </AYAstorm>
                    );
                }
                sc -= 1.0;
            }
        }

        diff /= w;
    }

    diff.rgb = clampHDRRange(diff.rgb);
    frag_color = diff;
}
