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

#ifndef DECL_NORMAL_MAP
#define DECL_NORMAL_MAP
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=2) uniform sampler2D normalMap;
#else
uniform sampler2D   normalMap;
#endif
#endif // DECL_NORMAL_MAP

#if defined(SUN_SHADOW)
#ifdef LL_VULKAN_GLSL
layout(set=0, binding=8) uniform sampler2DShadow shadowMap0;
layout(set=0, binding=9) uniform sampler2DShadow shadowMap1;
layout(set=0, binding=10) uniform sampler2DShadow shadowMap2;
layout(set=0, binding=11) uniform sampler2DShadow shadowMap3;
#else
uniform sampler2DShadow shadowMap0;
uniform sampler2DShadow shadowMap1;
uniform sampler2DShadow shadowMap2;
uniform sampler2DShadow shadowMap3;
#endif
#endif

#if defined(SPOT_SHADOW)
#ifdef LL_VULKAN_GLSL
layout(set=0, binding=12) uniform sampler2DShadow shadowMap4;
layout(set=0, binding=13) uniform sampler2DShadow shadowMap5;
#else
uniform sampler2DShadow shadowMap4;
uniform sampler2DShadow shadowMap5;
#endif
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
#else
uniform vec3 sun_dir;
uniform vec3 moon_dir;
uniform mat4 inv_proj;
uniform vec2 screen_res;
uniform int sun_up_factor;
#endif
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-8: cinematic_bd shadow_* bare uniforms UBO wrap.
// class1/deferred/shadowUtil.glsl では η-1 範式で ShadowUtilParamUBO_Legacy (set=3, binding=7)
// に取込み済だが、cinematic_bd override は未処理で Deferred Avatar Eyes 0:570 'non-opaque
// uniforms outside a block' 直撃。cinematic_bd は class1 と mutually exclusive (loader が
// cinematic_bd_first → class1 fallback) のため binding=7 共有可、std140 layout も
// class1 版と byte-identical (shadow_softness 含む 12 member) で将来 C++ side binding 共有可。
// GL #else path は元 8 bare 宣言を byte-for-byte 維持 (charter §3 #1 担保)。
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=63, std140) uniform ShadowUtilParamUBO_Legacy {
    mat4  shadow_matrix[6];
    vec4  shadow_clip;
    vec2  shadow_res;
    vec2  proj_shadow_res;
    float shadow_bias;
    float shadow_offset;
    float shadow_softness;
    float spot_shadow_bias;
    float spot_shadow_offset;
    float _pad_shadow_util_legacy_0;
    float _pad_shadow_util_legacy_1;
    float _pad_shadow_util_legacy_2;
};
#else
uniform vec2 shadow_res;
uniform vec2 proj_shadow_res;
uniform mat4 shadow_matrix[6];
uniform vec4 shadow_clip;
uniform float shadow_bias;
uniform float shadow_offset;
uniform float spot_shadow_bias;
uniform float spot_shadow_offset;
#endif

// Helper function for optimized PCF sampling
float sampleShadowMap(sampler2DShadow shadowMap, vec2 base_uv, float u, float v, vec2 shadowMapSizeInv, float lightDepth)
{
    vec2 uv = base_uv + vec2(u, v) * shadowMapSizeInv;
    return texture(shadowMap, vec3(uv, lightDepth));
}

// Optimized 4x4 PCF sampling based on The Witness implementation
float pcfShadow(sampler2DShadow shadowMap, vec3 norm, vec4 stc, float bias_mul, vec2 pos_screen, vec3 light_dir)
{
#if defined(SUN_SHADOW)
    stc.xyz /= stc.w;
    float lightDepth = stc.z;
    float offset = shadow_bias * bias_mul;
    lightDepth += offset * 2.0;

    vec2 shadowMapSize = shadow_res;
    vec2 shadowMapSizeInv = 1.0 / shadowMapSize;

    vec2 uv = stc.xy * shadowMapSize; // 1 unit = 1 texel

    vec2 base_uv;
    base_uv.x = floor(uv.x + 0.5);
    base_uv.y = floor(uv.y + 0.5);

    float s = (uv.x + 0.5 - base_uv.x);
    float t = (uv.y + 0.5 - base_uv.y);

    base_uv -= vec2(0.5, 0.5);
    base_uv *= shadowMapSizeInv;

    // 4x4 PCF kernel (FilterSize 5 from The Witness)
    float uw0 = (4.0 - 3.0 * s);
    float uw1 = 7.0;
    float uw2 = (1.0 + 3.0 * s);

    float u0 = (3.0 - 2.0 * s) / uw0 - 2.0;
    float u1 = (3.0 + s) / uw1;
    float u2 = s / uw2 + 2.0;

    float vw0 = (4.0 - 3.0 * t);
    float vw1 = 7.0;
    float vw2 = (1.0 + 3.0 * t);

    float v0 = (3.0 - 2.0 * t) / vw0 - 2.0;
    float v1 = (3.0 + t) / vw1;
    float v2 = t / vw2 + 2.0;

    float sum = 0.0;

    sum += uw0 * vw0 * sampleShadowMap(shadowMap, base_uv, u0, v0, shadowMapSizeInv, lightDepth);
    sum += uw1 * vw0 * sampleShadowMap(shadowMap, base_uv, u1, v0, shadowMapSizeInv, lightDepth);
    sum += uw2 * vw0 * sampleShadowMap(shadowMap, base_uv, u2, v0, shadowMapSizeInv, lightDepth);

    sum += uw0 * vw1 * sampleShadowMap(shadowMap, base_uv, u0, v1, shadowMapSizeInv, lightDepth);
    sum += uw1 * vw1 * sampleShadowMap(shadowMap, base_uv, u1, v1, shadowMapSizeInv, lightDepth);
    sum += uw2 * vw1 * sampleShadowMap(shadowMap, base_uv, u2, v1, shadowMapSizeInv, lightDepth);

    sum += uw0 * vw2 * sampleShadowMap(shadowMap, base_uv, u0, v2, shadowMapSizeInv, lightDepth);
    sum += uw1 * vw2 * sampleShadowMap(shadowMap, base_uv, u1, v2, shadowMapSizeInv, lightDepth);
    sum += uw2 * vw2 * sampleShadowMap(shadowMap, base_uv, u2, v2, shadowMapSizeInv, lightDepth);

    return sum / 144.0;
#else
    return 1.0;
#endif
}

// Helper function for spot shadow PCF sampling
float sampleSpotShadowMap(sampler2DShadow shadowMap, vec2 base_uv, float u, float v, vec2 shadowMapSizeInv, float lightDepth)
{
    vec2 uv = base_uv + vec2(u, v) * shadowMapSizeInv;
    return texture(shadowMap, vec3(uv, lightDepth));
}

// Optimized 4x4 PCF sampling for spot shadows
float pcfSpotShadow(sampler2DShadow shadowMap, vec4 stc, float bias_scale, vec2 pos_screen)
{
#if defined(SPOT_SHADOW)
    stc.xyz /= stc.w;
        float lightDepth = stc.z;
    lightDepth += spot_shadow_bias * bias_scale;

    vec2 shadowMapSize = proj_shadow_res;
    vec2 shadowMapSizeInv = 1.0 / shadowMapSize;

    vec2 uv = stc.xy * shadowMapSize; // 1 unit = 1 texel

    vec2 base_uv;
    base_uv.x = floor(uv.x + 0.5);
    base_uv.y = floor(uv.y + 0.5);

    float s = (uv.x + 0.5 - base_uv.x);
    float t = (uv.y + 0.5 - base_uv.y);

    base_uv -= vec2(0.5, 0.5);
    base_uv *= shadowMapSizeInv;

    // 4x4 PCF kernel (FilterSize 5 from The Witness)
    float uw0 = (4.0 - 3.0 * s);
    float uw1 = 7.0;
    float uw2 = (1.0 + 3.0 * s);

    float u0 = (3.0 - 2.0 * s) / uw0 - 2.0;
    float u1 = (3.0 + s) / uw1;
    float u2 = s / uw2 + 2.0;

    float vw0 = (4.0 - 3.0 * t);
    float vw1 = 7.0;
    float vw2 = (1.0 + 3.0 * t);

    float v0 = (3.0 - 2.0 * t) / vw0 - 2.0;
    float v1 = (3.0 + t) / vw1;
    float v2 = t / vw2 + 2.0;

    float sum = 0.0;

    sum += uw0 * vw0 * sampleSpotShadowMap(shadowMap, base_uv, u0, v0, shadowMapSizeInv, lightDepth);
    sum += uw1 * vw0 * sampleSpotShadowMap(shadowMap, base_uv, u1, v0, shadowMapSizeInv, lightDepth);
    sum += uw2 * vw0 * sampleSpotShadowMap(shadowMap, base_uv, u2, v0, shadowMapSizeInv, lightDepth);

    sum += uw0 * vw1 * sampleSpotShadowMap(shadowMap, base_uv, u0, v1, shadowMapSizeInv, lightDepth);
    sum += uw1 * vw1 * sampleSpotShadowMap(shadowMap, base_uv, u1, v1, shadowMapSizeInv, lightDepth);
    sum += uw2 * vw1 * sampleSpotShadowMap(shadowMap, base_uv, u2, v1, shadowMapSizeInv, lightDepth);

    sum += uw0 * vw2 * sampleSpotShadowMap(shadowMap, base_uv, u0, v2, shadowMapSizeInv, lightDepth);
    sum += uw1 * vw2 * sampleSpotShadowMap(shadowMap, base_uv, u1, v2, shadowMapSizeInv, lightDepth);
    sum += uw2 * vw2 * sampleSpotShadowMap(shadowMap, base_uv, u2, v2, shadowMapSizeInv, lightDepth);

    return sum / 144.0;
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

float nonpcfShadow(sampler2DShadow shadowMap, vec4 stc, vec2 pos_screen, float shad_res, float bias)
{
#if defined(SUN_SHADOW)
    float recip_shadow_res = 1.0 / shad_res;
    stc.xyz /= stc.w;
    stc.z += bias;

    stc.x = floor(stc.x*shad_res + fract(pos_screen.y)) * recip_shadow_res;

    float cs = texture(shadowMap, stc.xyz);
    float shadow = cs * 4.0;
    return shadow;
#else
    return 0.0;
#endif
}

float nonpcfShadowAtPos(vec4 pos_world, vec2 pos_screen)
{
#if defined(SUN_SHADOW)
    //BD - We don't want this otherwise volumetric lighting will fade out over distance where
    //     shadow maps end, this makes Volumetic Lighting reliant on shadow distance.
    //if (pos_world.z < shadow_clip.w) 
    {	
        vec4 near_split = shadow_clip*-0.75;
        vec4 far_split = shadow_clip*-1.25;

        if (pos_world.z < near_split.z) 
        {
            pos_world = shadow_matrix[3]*pos_world;
            return nonpcfShadow(shadowMap3, pos_world, pos_screen, shadow_res.x, shadow_bias); //w
        }
        else if (pos_world.z < near_split.y) 
        {
            pos_world = shadow_matrix[2]*pos_world;
            return nonpcfShadow(shadowMap2, pos_world, pos_screen, shadow_res.x, shadow_bias); //z
        }
        else if (pos_world.z < near_split.x) 
        {
            pos_world = shadow_matrix[1]*pos_world;
            return nonpcfShadow(shadowMap1, pos_world, pos_screen, shadow_res.x, shadow_bias); //y
        }
        else if (pos_world.z > far_split.x) 
        {
            pos_world = shadow_matrix[0]*pos_world;
            return nonpcfShadow(shadowMap0, pos_world, pos_screen, shadow_res.x, shadow_bias); //x
        }
    }
#endif
    return 1.0;
}
