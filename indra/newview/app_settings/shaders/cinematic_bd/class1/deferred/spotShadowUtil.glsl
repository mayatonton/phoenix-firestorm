/**
 * @file cinematic_bd/class1/deferred/spotShadowUtil.glsl
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
#if defined(SPOT_SHADOW)
layout(set = 1, binding = 36) uniform sampler2DShadow shadowMap4;
layout(set = 1, binding = 37) uniform sampler2DShadow shadowMap5;
#endif
#else
#if defined(SPOT_SHADOW)
uniform sampler2DShadow shadowMap4;
uniform sampler2DShadow shadowMap5;
#endif
#endif

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
