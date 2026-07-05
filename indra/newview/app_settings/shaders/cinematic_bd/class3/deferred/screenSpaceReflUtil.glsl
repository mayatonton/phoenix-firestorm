/**
 * @file class3/deferred/screenSpaceReflUtil.glsl
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

// Based on https://imanolfotia.com/blog/1

#ifdef LL_VULKAN_GLSL
// sceneMap 重複宣言 (reflectionProbeF.glsl と) 回避 = #ifndef guard 先勝ち。
#ifndef SCENEMAP_DECLARED
#define SCENEMAP_DECLARED 1
layout(set = 1, binding = 43) uniform sampler2D sceneMap;
#endif // SCENEMAP_DECLARED
layout(set = 1, binding = 47) uniform sampler2D sceneDepth;
#else
uniform sampler2D sceneMap;
uniform sampler2D sceneDepth;
#endif

#ifdef LL_VULKAN_GLSL
#ifndef PER_FRAME_MATRIX_UBO_DEFINED
#define PER_FRAME_MATRIX_UBO_DEFINED 1
layout(set = 0, binding = 0, std140) uniform PerFrameMatrixUBO
{
    mat4 projection_matrix;
    mat4 inverse_projection_matrix;
    mat4 identity_matrix;
    mat4 last_modelview_matrix;
};
#define inv_proj inverse_projection_matrix

#endif // PER_FRAME_MATRIX_UBO_DEFINED
layout(set = 1, binding = 49, std140) uniform SSRUtil_PerProgramBind
{
    vec2  _ssr_screen_res;
    float iterationCount;
    float rayStep;
    mat4  modelview_delta;
    mat4  inv_modelview_delta;
    float distanceBias;
    float depthRejectBias;
    float adaptiveStepMultiplier;
    float glossySampleCount;
    vec3  splitParamsStart;
    float noiseSine;
    vec3  splitParamsEnd;
    float maxZDepth;
    float maxRoughness;
    float _ssrUtil_pad0;
    float _ssrUtil_pad1;
    float _ssrUtil_pad2;
};
#define ssr_screen_res _ssr_screen_res
#else
uniform vec2 screen_res;
uniform mat4 projection_matrix;
uniform mat4 inv_proj;
uniform mat4 modelview_delta;
uniform mat4 inv_modelview_delta;

uniform float iterationCount;
uniform float rayStep;
uniform float distanceBias;
uniform float depthRejectBias;
uniform float adaptiveStepMultiplier;
uniform vec3 splitParamsStart;
uniform vec3 splitParamsEnd;
uniform float glossySampleCount;
uniform float noiseSine;
uniform float maxZDepth;
uniform float maxRoughness;
#define ssr_screen_res screen_res
#endif

// Ray march parameters wired to AYA scalar controls.
// distanceBias is used as hit thickness; max step is derived from max depth
// and iteration count so rayStep/Iterations still control reach.
#define STEP_SIZE       rayStep
#define STEP_GROWTH     adaptiveStepMultiplier
#define MAX_STEP_SIZE   max(STEP_SIZE, maxZDepth / max(iterationCount, 1.0))
#define MAX_THICKNESS   max(distanceBias, STEP_SIZE * 0.5)
#define DEPTH_BIAS      depthRejectBias
const int   BINARY_STEPS    = 8;

vec4 getPositionWithDepth(vec2 pos_screen, float depth);

float random(vec2 uv)
{
    return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453123);
}

vec2 generateProjectedPosition(vec3 pos)
{
    vec4 samplePosition = projection_matrix * vec4(pos, 1.0);
    samplePosition.xy = (samplePosition.xy / samplePosition.w) * 0.5 + 0.5;
    return samplePosition.xy;
}

float getLinearDepth(vec2 tc)
{
    float depth = texture(sceneDepth, tc).r;
    vec4 pos = getPositionWithDepth(tc, depth);
    return -pos.z;
}

vec3 binarySearch(vec3 dir, inout vec3 hitCoord, inout float dDepth)
{
    float depth;
    float initialStepLen = length(dir);

    for (int i = 0; i < BINARY_STEPS; i++)
    {
        vec2 projectedCoord = generateProjectedPosition(hitCoord);
        depth = getLinearDepth(projectedCoord);
        dDepth = abs(hitCoord.z) - depth;

        dir *= 0.5;
        if (dDepth > 0.0)
            hitCoord -= dir;
        else
            hitCoord += dir;
    }

    vec2 projectedCoord = generateProjectedPosition(hitCoord);

    // After 8 binary steps, precision is initialStep / 256.
    // Scale acceptance with depth — precision degrades at distance.
    float depthScale = max(1.0, depth * 0.01);
    float maxError = max(initialStepLen * 0.1, 0.05) * depthScale;
    if (abs(dDepth) > maxError)
        return vec3(-1.0, -1.0, depth);

    return vec3(projectedCoord, depth);
}

vec3 rayMarch(vec3 dir, inout vec3 hitCoord, out float dDepth, float startDepth)
{
    dir *= STEP_SIZE;

    for (int i = 0; i < int(iterationCount); i++)
    {
        hitCoord += dir;

        vec2 projectedCoord = generateProjectedPosition(hitCoord);

        if (projectedCoord.x < 0.0 || projectedCoord.x > 1.0 ||
            projectedCoord.y < 0.0 || projectedCoord.y > 1.0)
            return vec3(-1.0);

        float depth = getLinearDepth(projectedCoord);
        dDepth = abs(hitCoord.z) - depth;

        if (i < 1)
            continue;

        if (depth > maxZDepth)
            return vec3(-1.0);

        if (dDepth > 0.0)
        {
            float stepLen = length(dir);
            float thickness = max(MAX_THICKNESS, stepLen * 1.5);
            if (dDepth > thickness)
            {
                dir = normalize(dir) * min(stepLen * STEP_GROWTH, MAX_STEP_SIZE);
                continue;
            }
            return binarySearch(dir, hitCoord, dDepth);
        }

        // Grow step but cap at max to avoid skipping geometry
        dir = normalize(dir) * min(length(dir) * STEP_GROWTH, MAX_STEP_SIZE);
    }

    return vec3(-1.0);
}

float calculateEdgeFade(vec2 screenPos)
{
    vec2 distFromCenter = abs(screenPos * 2.0 - 1.0);
    vec2 fade = smoothstep(0.85, 1.0, distFromCenter);
    return 1.0 - max(fade.x, fade.y);
}

float tapScreenSpaceReflection(
    int totalSamples,
    vec2 tc,
    vec3 viewPos,
    vec3 n,
    inout vec4 collectedColor,
    sampler2D source,
    float glossiness)
{
#ifdef TRANSPARENT_SURFACE
    collectedColor = vec4(1, 0, 1, 1);
    return 0;
#endif

    float roughness = 1.0 - glossiness;

    if (roughness >= maxRoughness)
        return 0.0;

    vec3 viewDir = normalize(viewPos);
    vec3 normal = normalize(n);

    float viewDotNormal = dot(-viewDir, normal);
    if (viewDotNormal <= 0.0)
    {
        collectedColor = vec4(0.0);
        return 0.0;
    }

    vec2 distFromCenter = abs(tc * 2.0 - 1.0);
    float baseEdgeFade = 1.0 - smoothstep(0.85, 1.0, max(distFromCenter.x, distFromCenter.y));
    if (baseEdgeFade <= 0.001)
    {
        collectedColor = vec4(0.0);
        return 0.0;
    }

    // Bias the ray origin along the normal, scaled by distance.
    // Prevents grazing-angle rays from scraping the originating surface
    // at distance where depth precision breaks down.
    float depthBias = max(0.01, -viewPos.z * DEPTH_BIAS);
    vec3 biasedPos = viewPos - normal * depthBias;

    vec3 transformedPos = (inv_modelview_delta * vec4(biasedPos, 1.0)).xyz;
    float startDepth = -transformedPos.z;

    if (startDepth > maxZDepth)
    {
        collectedColor = vec4(0.0);
        return 0.0;
    }

    vec3 perfectReflDir = normalize(reflect(viewDir, normal));

    vec3 reflTarget = viewPos + perfectReflDir;
    vec3 transformedTarget = (inv_modelview_delta * vec4(reflTarget, 1.0)).xyz;
    vec3 transformedReflDir = normalize(transformedTarget - transformedPos);

    if (transformedReflDir.z >= 0.5)
    {
        collectedColor = vec4(0.0);
        return 0.0;
    }

    vec3 hitCoord = transformedPos;
    float dDepth;
    vec3 result = rayMarch(transformedReflDir, hitCoord, dDepth, startDepth);

    if (result.x < 0.0)
    {
        collectedColor = vec4(0.0);
        return 0.0;
    }

    vec2 hitTC = result.xy;
    float hitDepth = result.z;

    float edgeFade = calculateEdgeFade(hitTC);

    float zFadeStart = maxZDepth * 0.8;
    float zFade = 1.0 - smoothstep(zFadeStart, maxZDepth, hitDepth);

    float rayLength = length(hitCoord - transformedPos);
    float maxMipLevels = floor(log2(max(1.0, max(ssr_screen_res.x, ssr_screen_res.y))));
    float distanceFactor = clamp(rayLength / maxZDepth, 0.0, 1.0);
    float effectiveRoughness = clamp(roughness + distanceFactor * roughness, 0.0, 1.0);
    float mipLevel = maxMipLevels * effectiveRoughness;
    vec4 sampledColor = textureLod(source, hitTC, mipLevel);

    float rayFade = 1.0 - smoothstep(maxZDepth * 0.6, maxZDepth, rayLength);
    float sampleFade = edgeFade * zFade * rayFade;

    float remappedRoughness = clamp((roughness - (maxRoughness * 0.6)) / (maxRoughness - (maxRoughness * 0.6)), 0.0, 1.0);
    float roughnessFade = 1.0 - remappedRoughness;

    float combinedFade = sampleFade * roughnessFade * baseEdgeFade;

    collectedColor = vec4(sampledColor.rgb, combinedFade);
    return 1.0;
}
