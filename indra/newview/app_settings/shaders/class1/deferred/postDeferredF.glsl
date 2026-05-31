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

#ifndef HAS_DOF_CHROMA
#define HAS_DOF_CHROMA 0
#endif

#ifndef FRONT_BLUR
#define FRONT_BLUR 0
#endif

out vec4 frag_color;

uniform sampler2D diffuseRect;

#ifdef LL_VULKAN_GLSL
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
#else
uniform mat4 inv_proj;
uniform vec2 screen_res;
#endif
#ifdef LL_VULKAN_GLSL
layout(set=0, binding=2, std140) uniform FrameAtmosphere {
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
    float _pad_atm0;
    float _pad_atm1;
};
#else
uniform float max_cof;
#endif
uniform float res_scale;

// <AYAstorm r30 P4 step 1> BD chroma uniform (gated by HAS_DOF_CHROMA permutation)
uniform float chroma_str;
// </AYAstorm r30 P4 step 1>

in vec2 vary_fragcoord;

void dofSample(inout vec4 diff, inout float w, float min_sc, vec2 tc)
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

    if (sc > min_sc) //sampled pixel is more "out of focus" than current sample radius
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

    {
        float w = 1.0;

        float sc = (diff.a*2.0-1.0)*max_cof;

        float PI = 3.14159265358979323846264;

        // sample quite uniformly spaced points within a circle, for a circular 'bokeh'
// <AYAstorm r30 P4 step 1> BD FRONT_BLUR: gate the sc>0.5 (front-CoF) branch on permutation
#if FRONT_BLUR
        if (sc > 0.5)
        {
            // Apple Silicon Metal safety: bound the loop in case sc becomes NaN
            // or otherwise fails to converge under the translation layer.
            for (int safety = 0; safety < 32 && sc > 0.5; ++safety)
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
            // Apple Silicon Metal safety: bounded loop (see comment above).
            for (int safety = 0; safety < 32 && sc > 0.5; ++safety)
            {
                int its = int(max(1.0,(sc*3.7)));
                for (int i=0; i<its; ++i)
                {
                    float ang = sc+i*2*PI/its; // sc is added for rotary perturbance
                    float samp_x = sc*sin(ang);
                    float samp_y = sc*cos(ang);
                    // you could test sample coords against an interesting non-circular aperture shape here, if desired.
                    dofSample(diff, w, sc, vary_fragcoord.xy + (vec2(samp_x,samp_y) / screen_res));
                }
                sc -= 1.0;
            }
        }

        diff /= w;
    }

    diff.rgb = clampHDRRange(diff.rgb);
    frag_color = diff;
}
