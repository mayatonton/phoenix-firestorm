/**
 * @file avatarF.glsl
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

#ifdef LL_VULKAN_GLSL
layout(location=0) out vec4 frag_data[4];
#else
out vec4 frag_data[4];
#endif

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=1) uniform sampler2D diffuseMap;
#else
uniform sampler2D diffuseMap;
#endif

#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-4: FrameAtmosphere_Lighting per-group rename (η-3 §3.2 範式)
#ifndef FRAME_ATMOSPHERE_LIGHTING_DEFINED
#define FRAME_ATMOSPHERE_LIGHTING_DEFINED 1
layout(set=0, binding=2, std140) uniform FrameAtmosphere_Lighting {
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
#endif
#else
uniform float minimum_alpha;
#endif

// <FS:AYA r20 Phase C> per-draw skin marker: 1.0 if the parent LLViewerObject
// is on the SSS whitelist, 0.0 otherwise. Written into frag_data[3].a so the
// screen-space SSS pass can gate its blur to skin pixels only.
uniform float aya_sss_skin_flag;
// </FS:AYA>

#ifdef LL_VULKAN_GLSL
layout(location=4) in vec3 vary_normal;
#else
in vec3 vary_normal;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=0) in vec2 vary_texcoord0;
#else
in vec2 vary_texcoord0;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=3) in vec3 vary_position;
#else
in vec3 vary_position;
#endif

void mirrorClip(vec3 pos);
vec4 encodeNormal(vec3 n, float env, float gbuffer_flag);

void main()
{
    mirrorClip(vary_position);

    vec4 diff = texture(diffuseMap, vary_texcoord0.xy);

    if (diff.a < minimum_alpha)
    {
        discard;
    }

    frag_data[0] = vec4(diff.rgb, 0.0);
    frag_data[1] = vec4(0,0,0,0);
    vec3 nvn = normalize(vary_normal);
    frag_data[2] = encodeNormal(nvn.xyz, 0, GBUFFER_FLAG_HAS_ATMOS);

#if defined(HAS_EMISSIVE)
    // <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> AY r20 writes the
    // per-draw SSS skin flag into gbuffer3.a so the SSS pass can gate to
    // skin pixels.
    // <FS:AYAstorm r30 BD改善> r20 consolidation 後は Cinematic でも SSS dispatch
    // するため両 mode で skin flag を書く。非 SSS 経路では r20 が cvar OFF で
    // doSkinSSS 早期 return するため write 値は読まれない (実害ゼロ)。
    frag_data[3] = vec4(0, 0, 0, aya_sss_skin_flag);
    // </FS:AYAstorm>
    // </FS:AYA>
#endif
}

