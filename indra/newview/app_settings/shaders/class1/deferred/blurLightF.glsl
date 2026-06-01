/**
 * @file blurLightF.glsl
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

#ifdef LL_VULKAN_GLSL
layout(set=0, binding=4) uniform sampler2D lightMap;
#else
uniform sampler2D lightMap;
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

uniform float dist_factor;
uniform float blur_size;
uniform vec2 delta;
uniform vec3 kern[4];
uniform float kern_scale;

in vec2 vary_fragcoord;

vec4 getPosition(vec2 pos_screen);
vec4 getNorm(vec2 pos_screen);

// <FS:AYA r30 Phase 3.8 Cinematic mount strategy C>
// Blur directional shadows (R), SSAO (G), and spot shadows (B/A).  BD's
// original Cinematic pass only blurred SSAO, which left RenderShadowBlurSize
// visually inert for shadow edges.
// Two main() bodies; AY mode is preserved verbatim.
#if AYASTORM_CINEMATIC

void main()
{
    vec2 tc = vary_fragcoord.xy;
    vec4 norm = getNorm(tc);
    vec3 pos = getPosition(tc).xyz;
    vec4 ccol = texture(lightMap, tc).rgba;

    vec2 dlt = kern_scale * delta / (1.0+norm.xy*norm.xy);
    dlt /= max(-pos.z*dist_factor, 1.0);

    vec2 defined_weight = kern[0].xy;
    vec4 col = defined_weight.xyxx * ccol;

    float pointplanedist_tolerance_pow2 = pos.z*pos.z*0.00005;

    tc *= screen_res;
    float tc_mod = 0.5*(tc.x + tc.y);
    tc_mod -= floor(tc_mod);
    tc_mod *= 2.0;
    tc += ( (tc_mod - 0.5) * kern[1].z * dlt * 0.5 );

    vec3 k[7];
    k[0] = kern[0];
    k[2] = kern[1];
    k[4] = kern[2];
    k[6] = kern[3];

    k[1] = (k[0]+k[2])*0.5f;
    k[3] = (k[2]+k[4])*0.5f;
    k[5] = (k[4]+k[6])*0.5f;

    for (int i = 1; i < 7; i++)
    {
        vec2 samptc = tc + k[i].z*dlt*2.0;
        samptc /= screen_res;
        vec3 samppos = getPosition(samptc).xyz;

        float d = dot(norm.xyz, samppos.xyz-pos.xyz);

        if (d*d <= pointplanedist_tolerance_pow2)
        {
            vec4 sampcol = texture(lightMap, samptc);
            col += sampcol * k[i].xyxx;
            defined_weight += k[i].xy;
        }
    }

    for (int i = 1; i < 7; i++)
    {
        vec2 samptc = tc - k[i].z*dlt*2.0;
        samptc /= screen_res;
        vec3 samppos = getPosition(samptc).xyz;

        float d = dot(norm.xyz, samppos.xyz-pos.xyz);

        if (d*d <= pointplanedist_tolerance_pow2)
        {
            vec4 sampcol = texture(lightMap, samptc);
            col += sampcol * k[i].xyxx;
            defined_weight += k[i].xy;
        }
    }

    col /= defined_weight.xyxx;

    frag_color = max(col, vec4(0));

#ifdef IS_AMD_CARD
    vec3 dummy1 = kern[0];
    vec3 dummy2 = kern[3];
#endif
}

#else // AYASTORM_CINEMATIC

void main()
{
    vec2 tc = vary_fragcoord.xy;
    vec4 norm = getNorm(tc);
    vec3 pos = getPosition(tc).xyz;
    vec4 ccol = texture(lightMap, tc).rgba;

    vec2 dlt = kern_scale * delta / (1.0+norm.xy*norm.xy);
    dlt /= max(-pos.z*dist_factor, 1.0);

    vec2 defined_weight = kern[0].xy; // special case the first (centre) sample's weight in the blur; we have to sample it anyway so we get it for 'free'
    vec4 col = defined_weight.xyxx * ccol;

    // relax tolerance according to distance to avoid speckling artifacts, as angles and distances are a lot more abrupt within a small screen area at larger distances
    float pointplanedist_tolerance_pow2 = pos.z*pos.z*0.00005;

    // perturb sampling origin slightly in screen-space to hide edge-ghosting artifacts where smoothing radius is quite large
    tc *= screen_res;
    float tc_mod = 0.5*(tc.x + tc.y);
    tc_mod -= floor(tc_mod);
    tc_mod *= 2.0;
    tc += ( (tc_mod - 0.5) * kern[1].z * dlt * 0.5 );

    // TODO: move this to kern instead of building kernel per pixel
    vec3 k[7];
    k[0] = kern[0];
    k[2] = kern[1];
    k[4] = kern[2];
    k[6] = kern[3];

    k[1] = (k[0]+k[2])*0.5f;
    k[3] = (k[2]+k[4])*0.5f;
    k[5] = (k[4]+k[6])*0.5f;

    for (int i = 1; i < 7; i++)
    {
        vec2 samptc = tc + k[i].z*dlt*2.0;
        samptc /= screen_res;
        vec3 samppos = getPosition(samptc).xyz;

        float d = dot(norm.xyz, samppos.xyz-pos.xyz);// dist from plane

        if (d*d <= pointplanedist_tolerance_pow2)
        {
            col += texture(lightMap, samptc)*k[i].xyxx;
            defined_weight += k[i].xy;
        }
    }

    for (int i = 1; i < 7; i++)
    {
        vec2 samptc = tc - k[i].z*dlt*2.0;
        samptc /= screen_res;
        vec3 samppos = getPosition(samptc).xyz;

        float d = dot(norm.xyz, samppos.xyz-pos.xyz);// dist from plane

        if (d*d <= pointplanedist_tolerance_pow2)
        {
            col += texture(lightMap, samptc)*k[i].xyxx;
            defined_weight += k[i].xy;
        }
    }

    col /= defined_weight.xyxx;
    //col.y *= col.y;

    frag_color = max(col, vec4(0));

#ifdef IS_AMD_CARD
    // If it's AMD make sure the GLSL compiler sees the arrays referenced once by static index. Otherwise it seems to optimise the storage awawy which leads to unfun crashes and artifacts.
    vec3 dummy1 = kern[0];
    vec3 dummy2 = kern[3];
#endif
}

#endif // AYASTORM_CINEMATIC
// </FS:AYA>
