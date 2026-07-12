/**
 * @file class1/deferred/shadowUtil.glsl
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

#ifdef LL_VULKAN_GLSL
#ifndef DECL_NORMAL_MAP
#define DECL_NORMAL_MAP
layout(set = 1, binding = 27) uniform sampler2D normalMap;
#endif // DECL_NORMAL_MAP

#if defined(SUN_SHADOW)
layout(set = 1, binding = 32) uniform sampler2DShadow shadowMap0;
layout(set = 1, binding = 33) uniform sampler2DShadow shadowMap1;
layout(set = 1, binding = 34) uniform sampler2DShadow shadowMap2;
layout(set = 1, binding = 35) uniform sampler2DShadow shadowMap3;
#endif

#if defined(SPOT_SHADOW)
layout(set = 1, binding = 36) uniform sampler2DShadow shadowMap4;
layout(set = 1, binding = 37) uniform sampler2DShadow shadowMap5;
#endif

layout(set = 1, binding = 31, std140) uniform ShadowUtil_PerProgramBind
{
    mat4  shadow_matrix[6];
    vec4  shadow_clip;
#ifndef _AYA_UM_sun_dir
#define _AYA_UM_sun_dir 1
    vec3  sun_dir;
#else
    vec3  _dup_ShadowUtil_sun_dir;
#endif
    float shadow_bias;
#ifndef _AYA_UM_moon_dir
#define _AYA_UM_moon_dir 1
    vec3  moon_dir;
#else
    vec3  _dup_ShadowUtil_moon_dir;
#endif
    float shadow_offset;
    vec2  shadow_res;
    vec2  proj_shadow_res;
    float shadow_softness;
    float spot_shadow_bias;
    float spot_shadow_offset;
    float _shadowUtil_pad0;
};
#ifndef WINDLIGHT_ATMOS_UBO_DEFINED
#define WINDLIGHT_ATMOS_UBO_DEFINED 1
layout(set = 1, binding = 8, std140) uniform WindlightAtmos_PerProgramBind
{
    vec3  sunlight_color;
    int   sun_up_factor;
    vec3  moonlight_color;
    int   classic_mode_wl;
#ifndef _AYA_UM_ambient_color
#define _AYA_UM_ambient_color 1
    vec3  ambient_color;
#else
    vec3  _dup_WindlightAtmos_ambient_color;
#endif
    int   aya_visual_realism_enabled;
    vec3  blue_horizon;
    int   aya_r14_volumetric_atmosphere_enabled;
    vec3  blue_density;
    float aya_r14_strength;
    vec3  glow;
    float aya_r16_strength;
    vec3  lightnorm;
    int   aya_r16_aerial_perspective_enabled;
    float haze_density;
    float density_multiplier;
    float distance_multiplier;
    float max_y;
    float haze_horizon;
    float cloud_shadow;
    float sun_moon_glow_factor;
    float sky_sunlight_scale;
    float sky_ambient_scale;
    float _wlAtmos_pad0;
    float _wlAtmos_pad1;
    float _wlAtmos_pad2;
};
#define _classicMode classic_mode_wl
#endif // WINDLIGHT_ATMOS_UBO_DEFINED
#else
#ifndef DECL_NORMAL_MAP
#define DECL_NORMAL_MAP
uniform sampler2D   normalMap;
#endif // DECL_NORMAL_MAP

#if defined(SUN_SHADOW)
uniform sampler2DShadow shadowMap0;
uniform sampler2DShadow shadowMap1;
uniform sampler2DShadow shadowMap2;
uniform sampler2DShadow shadowMap3;
#endif

#if defined(SPOT_SHADOW)
uniform sampler2DShadow shadowMap4;
uniform sampler2DShadow shadowMap5;
#endif

uniform vec3 sun_dir;
uniform vec3 moon_dir;
uniform vec2 shadow_res;
uniform vec2 proj_shadow_res;
uniform mat4 shadow_matrix[6];
uniform vec4 shadow_clip;
uniform float shadow_bias;
uniform float shadow_offset;
uniform float shadow_softness;
uniform float spot_shadow_bias;
uniform float spot_shadow_offset;
uniform mat4 inv_proj;
uniform vec2 screen_res;
uniform int sun_up_factor;
#endif

float pcfShadow(sampler2DShadow shadowMap, vec3 norm, vec4 stc, float bias_mul, vec2 pos_screen, vec3 light_dir)
{
#if defined(SUN_SHADOW)
    float offset = shadow_bias * bias_mul;
    stc.xyz /= stc.w;
    stc.z += offset * 2.0;
    float cs = texture(shadowMap, stc.xyz);
    float shadow = cs * 4.0;
    shadow += texture(shadowMap, stc.xyz+vec3( 1.5*shadow_softness/shadow_res.x,  0.5*shadow_softness/shadow_res.y, 0.0));
    shadow += texture(shadowMap, stc.xyz+vec3( 0.5*shadow_softness/shadow_res.x, -1.5*shadow_softness/shadow_res.y, 0.0));
    shadow += texture(shadowMap, stc.xyz+vec3(-1.5*shadow_softness/shadow_res.x, -0.5*shadow_softness/shadow_res.y, 0.0));
    shadow += texture(shadowMap, stc.xyz+vec3(-0.5*shadow_softness/shadow_res.x,  1.5*shadow_softness/shadow_res.y, 0.0));
    return clamp(shadow * 0.125, 0.0, 1.0);
#else
    return 1.0;
#endif
}

float pcfSpotShadow(sampler2DShadow shadowMap, vec4 stc, float bias_scale, vec2 pos_screen)
{
#if defined(SPOT_SHADOW)
    stc.xyz /= stc.w;
    stc.z += spot_shadow_bias * bias_scale;
    float cs = texture(shadowMap, stc.xyz);
    float shadow = cs;

    vec2 off = 1.0/proj_shadow_res;
    off.y *= 1.5;

    shadow += texture(shadowMap, stc.xyz+vec3(off.x*2.0, off.y, 0.0));
    shadow += texture(shadowMap, stc.xyz+vec3(off.x, -off.y, 0.0));
    shadow += texture(shadowMap, stc.xyz+vec3(-off.x, off.y, 0.0));
    shadow += texture(shadowMap, stc.xyz+vec3(-off.x*2.0, -off.y, 0.0));
    return shadow*0.2;
#else
    return 1.0;
#endif
}

float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen)
{
#if defined(SUN_SHADOW)
    float shadow = 0.0f;
    vec3 light_dir = normalize((sun_up_factor == 1) ? sun_dir : moon_dir);

    float dp_directional_light = max(0.0, dot(norm.xyz, light_dir));
          dp_directional_light = clamp(dp_directional_light, 0.0, 1.0);

    vec3 shadow_pos = pos.xyz;

    vec3 offset = light_dir.xyz * (1.0 - dp_directional_light);

    shadow_pos += offset * shadow_offset * 2.0;

    vec4 spos = vec4(shadow_pos.xyz, 1.0);

    if (spos.z > -shadow_clip.w)
    {
        vec4 lpos;
        vec4 near_split = shadow_clip*-0.75;
        vec4 far_split = shadow_clip*-1.25;
        vec4 transition_domain = near_split-far_split;
        float weight = 0.0;

        if (spos.z < near_split.z)
        {
            lpos = shadow_matrix[3]*spos;

            float w = 1.0;
            w -= max(spos.z-far_split.z, 0.0)/transition_domain.z;
            //w = clamp(w, 0.0, 1.0);
            float contrib = pcfShadow(shadowMap3, norm, lpos, 1.0, pos_screen, light_dir)*w;
            //if (contrib > 0)
            {
                shadow += contrib;
                weight += w;
            }
            shadow += max((pos.z+shadow_clip.z)/(shadow_clip.z-shadow_clip.w)*2.0-1.0, 0.0);
        }

        if (spos.z < near_split.y && spos.z > far_split.z)
        {
            lpos = shadow_matrix[2]*spos;

            float w = 1.0;
            w -= max(spos.z-far_split.y, 0.0)/transition_domain.y;
            w -= max(near_split.z-spos.z, 0.0)/transition_domain.z;
            //w = clamp(w, 0.0, 1.0);
            float contrib = pcfShadow(shadowMap2, norm, lpos, 1.0, pos_screen, light_dir)*w;
            //if (contrib > 0)
            {
                shadow += contrib;
                weight += w;
            }
        }

        if (spos.z < near_split.x && spos.z > far_split.y)
        {
            lpos = shadow_matrix[1]*spos;

            float w = 1.0;
            w -= max(spos.z-far_split.x, 0.0)/transition_domain.x;
            w -= max(near_split.y-spos.z, 0.0)/transition_domain.y;
            //w = clamp(w, 0.0, 1.0);
            float contrib = pcfShadow(shadowMap1, norm, lpos, 1.0, pos_screen, light_dir)*w;
            //if (contrib > 0)
            {
                shadow += contrib;
                weight += w;
            }
        }

        if (spos.z > far_split.x)
        {
            lpos = shadow_matrix[0]*spos;

            float w = 1.0;
            w -= max(near_split.x-spos.z, 0.0)/transition_domain.x;
            //w = clamp(w, 0.0, 1.0);
            float contrib = pcfShadow(shadowMap0, norm, lpos, 1.0, pos_screen, light_dir)*w;
            //if (contrib > 0)
            {
                shadow += contrib;
                weight += w;
            }
        }

        shadow /= weight;
    }
    else
    {
        return 1.0f; // lit beyond the far split...
    }
    //shadow = min(dp_directional_light,shadow);
    return shadow;
#else
    return 1.0;
#endif
}

float sampleSpotShadow(vec3 pos, vec3 norm, int index, vec2 pos_screen)
{
#if defined(SPOT_SHADOW)
    float shadow = 0.0f;
    pos += norm * spot_shadow_offset;

    vec4 spos = vec4(pos,1.0);
    if (spos.z > -shadow_clip.w)
    {
        vec4 lpos;

        vec4 near_split = shadow_clip*-0.75;
        vec4 far_split = shadow_clip*-1.25;
        vec4 transition_domain = near_split-far_split;
        float weight = 0.0;

        {
            float w = 1.0;
            w -= max(spos.z-far_split.z, 0.0)/transition_domain.z;

            if (index == 0)
            {
                lpos = shadow_matrix[4]*spos;
                shadow += pcfSpotShadow(shadowMap4, lpos, 0.8, spos.xy)*w;
            }
            else
            {
                lpos = shadow_matrix[5]*spos;
                shadow += pcfSpotShadow(shadowMap5, lpos, 0.8, spos.xy)*w;
            }
            weight += w;
            shadow += max((pos.z+shadow_clip.z)/(shadow_clip.z-shadow_clip.w)*2.0-1.0, 0.0);
        }

        shadow /= weight;
    }
    else
    {
        shadow = 1.0f;
    }
    return shadow;
#else
    return 1.0;
#endif
}

