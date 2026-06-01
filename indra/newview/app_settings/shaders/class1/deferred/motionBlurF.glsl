/**
 * @file motionBlurF.glsl
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

// AYAstorm r30 P2 step 5b: imported from BlackDragon Viewer (NiranV Dean), 995a1354d8, 2026-05-17
// Source: https://github.com/NiranV/Black-Dragon-Viewer @ indra/newview/app_settings/shaders/class1/deferred/motionBlurF.glsl
// License: LGPL-2.1-only (same as Second Life Viewer Source Code, no relicensing)

/*[EXTRA_CODE_HERE]*/

#ifdef LL_VULKAN_GLSL
layout(location=0) out vec4 frag_color;
#else
out vec4 frag_color;
#endif

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=4) uniform sampler2D diffuseRect;
layout(set=1, binding=56) uniform sampler2D velocityMap;
#else
uniform sampler2D diffuseRect;
uniform sampler2D velocityMap;
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
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-6: motionBlurF non-opaque uniforms UBO wrap (Cluster E)
layout(set=3, binding=27, std140) uniform MotionBlurFParamUBO_Legacy {
    int motion_blur_strength;
};
#else
uniform int motion_blur_strength;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=1) in vec2 vary_fragcoord;
#else
in vec2 vary_fragcoord;
#endif

// <FS:AYA r30 Phase 3.8 Cinematic mount strategy C>
//   Cinematic — BD original: 0.5 px noise floor, no NaN/Inf guard, no
//     sanity ceiling, no per-sample velocity gate. Trusts the velocity
//     buffer fully — accepts the upstream avatar lastMatrixPalette
//     uninitialized streak artifact as part of the BD look.
//   AY        — r30 P2 step 5b hardening: NaN/Inf guard, 2.0 px noise
//     floor, 2x max_blur sanity ceiling, per-sample velocity gate
//     (rejects samples under the same noise floor so high-velocity
//     neighbors don't smear diffuse from explicitly zero-velocity
//     surfaces, e.g. RenderMotionBlur{Self,Other}Avatars opt-out),
//     zero-total passthrough fallback.
// Two main() bodies; AY mode is preserved verbatim.
#if AYASTORM_CINEMATIC

void main()
{
    vec2 uv = vary_fragcoord;
    vec2 vel = texture(velocityMap, uv).rg;

    // NDC velocity to pixel velocity
    vec2 pixel_vel = vel * screen_res * 0.5;
    float speed = length(pixel_vel);

    // Early out for negligible motion
    if (speed < 0.5)
    {
        frag_color = texture(diffuseRect, uv);
        return;
    }

    // Clamp to max blur length
    float max_blur = float(motion_blur_strength);
    if (speed > max_blur)
    {
        pixel_vel *= max_blur / speed;
    }

    // Step size in UV space per iteration
    vec2 step_uv = (pixel_vel / screen_res) * (2.0 / 32.0);

    // Start sampling ahead of center
    vec2 sample_uv = uv + step_uv * 16.0;

    // 32-sample triangle-weighted blur
    vec3 color = vec3(0.0);
    float total = 0.0;

    for (int i = 0; i < 32; ++i)
    {
        float w = 32.0 - abs(float(i) - 16.0);
        total += w;
        color += texture(diffuseRect, sample_uv).rgb * w;
        sample_uv -= step_uv;
    }

    frag_color = vec4(color / total, 1.0);
}

#else // AYASTORM_CINEMATIC

void main()
{
    vec2 uv = vary_fragcoord;
    vec2 vel = texture(velocityMap, uv).rg;

    // AYAstorm r30 P2 step 5b: garbage-velocity guards. Upstream issues we know
    // about (avatar lastMatrixPalette uninitialized → NaN, per-frame matrix
    // round-off → sub-pixel drift) would otherwise either smear lightning streaks
    // around avatars or blur static buildings. These guards keep the effect
    // honest until those root causes (tracked separately) are fixed.
    if (any(isnan(vel)) || any(isinf(vel)))
    {
        frag_color = texture(diffuseRect, uv);
        return;
    }

    // NDC velocity to pixel velocity
    vec2 pixel_vel = vel * screen_res * 0.5;
    float speed = length(pixel_vel);

    // Noise floor: BD uses 0.5 px; we raise to 2.0 px to suppress static drift.
    if (speed < 2.0)
    {
        frag_color = texture(diffuseRect, uv);
        return;
    }

    // Clamp to max blur length
    float max_blur = float(motion_blur_strength);

    // Sanity ceiling: anything claiming > 2x max_blur is garbage (uninitialized
    // skinning matrix, SIM-boundary interpolation skew, projection blowup, etc)
    // — pass through unblurred. Real motion is already clamped to max_blur below.
    if (speed > max_blur * 2.0)
    {
        frag_color = texture(diffuseRect, uv);
        return;
    }

    if (speed > max_blur)
    {
        pixel_vel *= max_blur / speed;
    }

    // Step size in UV space per iteration
    vec2 step_uv = (pixel_vel / screen_res) * (2.0 / 32.0);

    // Start sampling ahead of center
    vec2 sample_uv = uv + step_uv * 16.0;

    // 32-sample triangle-weighted blur with per-sample velocity gate
    // (AYAstorm r30 P2): reject samples whose own velocity falls under the
    // same 2.0 px noise floor used at the output pixel. This prevents
    // high-velocity neighbors (BG) from sampling diffuse from explicitly
    // zero-velocity surfaces (RenderMotionBlur{Self,Other}Avatars opt-out,
    // or any static no-write region), which would otherwise smear avatar
    // diffuse outward as a halo around opted-out avatars.
    vec3 color = vec3(0.0);
    float total = 0.0;

    for (int i = 0; i < 32; ++i)
    {
        vec2 vel_at_sample = texture(velocityMap, sample_uv).rg;
        vec2 pixel_vel_sample = vel_at_sample * screen_res * 0.5;
        if (length(pixel_vel_sample) >= 2.0)
        {
            float w = 32.0 - abs(float(i) - 16.0);
            total += w;
            color += texture(diffuseRect, sample_uv).rgb * w;
        }
        sample_uv -= step_uv;
    }

    if (total < 1e-3)
    {
        frag_color = texture(diffuseRect, uv);
        return;
    }

    frag_color = vec4(color / total, 1.0);
}

#endif // AYASTORM_CINEMATIC
// </FS:AYA>
