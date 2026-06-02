/**
 * @file volumetricLightF.glsl
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

// AYAstorm r30 P3: imported from BlackDragon Viewer (NiranV Dean), 995a1354d8, 2026-04-19
// Source: https://github.com/NiranV/Black-Dragon-Viewer @ indra/newview/app_settings/shaders/class3/deferred/volumetricLightF.glsl
// License: LGPL-2.1-only (same as Second Life Viewer Source Code, no relicensing)

#extension GL_ARB_texture_rectangle : enable
#extension GL_ARB_shader_texture_lod : enable

/*[EXTRA_CODE_HERE]*/

#ifndef GODRAYS_FADE
#define GODRAYS_FADE 0
#endif

out vec4 frag_color;

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
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-2: FrameLights guard wrap (B?-ζ §3.1 範式)
#ifndef FRAME_LIGHTS_DEFINED
#define FRAME_LIGHTS_DEFINED 1
layout(set=0, binding=1, std140) uniform FrameLights {
    int  sun_up_factor;
    vec3 sun_dir;
    vec3 moon_dir;
    vec4 waterPlane;
    vec4 light_position[8];
    vec3 light_direction[8];
    vec4 light_attenuation[8];
    vec3 light_diffuse[8];
    vec2 light_deferred_attenuation[8];
};
#endif
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
uniform vec2 screen_res;
uniform vec3 sun_dir;
uniform vec3 blue_density;
uniform float haze_density;
uniform vec3 sunlight_color;
#endif

in vec2 vary_fragcoord;

#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2b:
//   BD-borrow godrays composite uniform を UBO 化。host = pipeline.cpp doRenderGodrays
//   (uniform1i GODRAY_RES / uniform1f GODRAY_MULTIPLIER / uniform1f FALLOFF_MULTIPLIER)。
//   seconds60 は宣言のみで本体未使用 (BD legacy)、host setter も無いが parse 通過のため UBO 含める。
#ifndef PER_PROGRAM_UBO_VOLUMETRIC_LIGHT_F_DEFINED
#define PER_PROGRAM_UBO_VOLUMETRIC_LIGHT_F_DEFINED 1
layout(set=2, binding=18, std140) uniform PerProgramUBO_VolumetricLightF {
    int   godray_res;            // offset 0,  size 4 + 12 pad
    float godray_multiplier;     // offset 16, size 4 + 12 pad
    float falloff_multiplier;    // offset 32, size 4 + 12 pad
    float seconds60;             // offset 48, size 4 + 12 pad (BD legacy dead uniform、host setter なし)
};  // total 64
#endif
#else
uniform int godray_res;
uniform float godray_multiplier;
uniform float falloff_multiplier;

uniform float seconds60;
#endif

float rand(vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

// <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> Two main() bodies:
//   Cinematic — BD original. Forward-declares nonpcfShadowAtPos(vec4,
//     vec2) and runs godrays. REQUIRES shadowUtil to supply
//     nonpcfShadowAtPos (step 2 strategy B overwrite or step 4 strategy
//     D dual-file mount must satisfy this). Gated on HAS_SUN_SHADOW
//     because features.hasShadows = use_sun_shadow and shadowUtil is
//     not attached when use_sun_shadow=false; without the gate, the
//     unconditional call to nonpcfShadowAtPos would fail to link when
//     the user sets RenderShadowDetail=0. Godrays without shadow
//     contrast carry no signal anyway, so this is graceful degradation.
//   AY — r30 P3 fallback. Remaps to sampleDirectionalShadow (in
//     class1/deferred/shadowUtil.glsl, attached via features.hasShadows)
//     and skips the whole body when HAS_SUN_SHADOW is undefined.
#if AYASTORM_CINEMATIC
#ifdef HAS_SUN_SHADOW
float nonpcfShadowAtPos(vec4 pos_world, vec2 pos_screen);
#endif
#else
#ifdef HAS_SUN_SHADOW
float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen);
#endif
#endif
// </FS:AYA>

vec4 getPosition(vec2 pos_screen);

#if AYASTORM_CINEMATIC

void main()
{
    vec2 tc = vary_fragcoord.xy;
    vec4 diff = texture(diffuseRect, tc);
#ifdef HAS_SUN_SHADOW
    vec4 pos = getPosition(tc);
    float depth = texture(depthMap, tc).r;
    depth *= pow(depth, 100.0);

    vec3 haze_weight = vec3(1,1,1);
    vec3 temp1 = blue_density + vec3(haze_density);
    haze_weight = vec3(haze_density) / temp1;

    // craptacular rays
    float roffset = rand(tc);
    vec3 farpos = pos.xyz;
    farpos *= min(-pos.z, 512.0) / -pos.z;

    float shadamount = 0.0;
    float shaftify = 0.0;
    float last_shadsample = 0.0;

    for (int i=godray_res-1; i>0; --i)
    {
      vec4 spos = vec4(mix(vec3(0,0,0), farpos, (i-roffset)/(godray_res)), 1.0);
      float this_shadsample = 0.275 * nonpcfShadowAtPos(spos, tc);
      float this_shaftify = 0.15 * (abs(this_shadsample + last_shadsample));
      last_shadsample = this_shadsample;
      this_shadsample *= i;
      this_shaftify /= i;
      shadamount += this_shadsample;
      shaftify += this_shaftify;
    }

    shadamount /= godray_res;
    shaftify /= godray_res;
    shadamount *= clamp(depth, 0.0 , 0.5);

    float fade = max(falloff_multiplier / max(depth, 1e-4), 1.0);
    shaftify = (shaftify / fade) * godray_multiplier;
#if GODRAYS_FADE
    fade = 0.0;
    if(sun_dir.z < 0.0)
    {
        fade = clamp(1 - dot(sun_dir.xy * 1.2, sun_dir.xy * 1.8), 0, 1);
    }
    shaftify *= fade;
#endif
    diff.rgb += ((shaftify * haze_weight.x) * shadamount) * sunlight_color;
#endif // HAS_SUN_SHADOW

    frag_color = diff;
}

#else // AYASTORM_CINEMATIC

void main()
{
    vec2 tc = vary_fragcoord.xy;
    vec4 diff = texture(diffuseRect, tc);

#ifdef HAS_SUN_SHADOW
    vec4 pos = getPosition(tc);
    float depth = texture(depthMap, tc).r;
    depth *= pow(depth, 100.0);

    vec3 haze_weight = vec3(1,1,1);
    vec3 temp1 = blue_density + vec3(haze_density);
    haze_weight = vec3(haze_density) / temp1;

    // craptacular rays
    float roffset = rand(tc);
    vec3 farpos = pos.xyz;
    farpos *= min(-pos.z, 512.0) / -pos.z;

    float shadamount = 0.0;
    float shaftify = 0.0;
    float last_shadsample = 0.0;

    for (int i=godray_res-1; i>0; --i)
    {
      vec4 spos = vec4(mix(vec3(0,0,0), farpos, (i-roffset)/(godray_res)), 1.0);
      float this_shadsample = 0.275 * sampleDirectionalShadow(spos.xyz, sun_dir, tc);
      float this_shaftify = 0.15 * (abs(this_shadsample + last_shadsample));
      last_shadsample = this_shadsample;
      this_shadsample *= i;
      this_shaftify /= i;
      shadamount += this_shadsample;
      shaftify += this_shaftify;
    }

    shadamount /= godray_res;
    shaftify /= godray_res;
    shadamount *= clamp(depth, 0.0 , 0.5);

    float fade = max(falloff_multiplier / max(depth, 1e-4), 1.0);
    shaftify = (shaftify / fade) * godray_multiplier;
#if GODRAYS_FADE
    fade = 0.0;
    if(sun_dir.z < 0.0)
    {
        fade = clamp(1 - dot(sun_dir.xy * 1.2, sun_dir.xy * 1.8), 0, 1);
    }
    shaftify *= fade;
#endif
    diff.rgb += ((shaftify * haze_weight.x) * shadamount) * sunlight_color;
#endif

    frag_color = diff;
}

#endif // AYASTORM_CINEMATIC
