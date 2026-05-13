/**
 * @file class1/deferred/skinSSSF.glsl
 *
 * AYAstorm r20 P0a prototype: avatar skin SSS (subsurface scattering)
 * fragment shader.
 *
 * Reads the post-softenLight HDR scene color and applies a 5-tap separable
 * blur with wavelength-dependent per-channel weights so red light bleeds
 * further than green / blue (SSS-like color separation). The two passes
 * are scheduled by LLPipeline::doSkinSSS:
 *   Pass 1: blur_dir=(1,0), strength=1.0, blend OFF, scratch ← screen
 *   Pass 2: blur_dir=(0,1), strength<1.0, blend SRC_ALPHA/(1-SRC_ALPHA),
 *           screen ← scratch (mix with original screen content)
 *
 * Prototype scope (P0a): NO skin whitelist — the blur applies to every
 * pixel. This is intentional: the goal of P0a is to evaluate whether
 * screen-space SSS actually delivers the look AYAstorm is after, before
 * we invest in LLVOAvatar whitelist detection + GBuffer flag extension.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * $/LicenseInfo$
 */

/*[EXTRA_CODE_HERE]*/

out vec4 frag_color;

in vec2 vary_fragcoord;

uniform sampler2D diffuseRect;       // source color (screen for pass 1, scratch for pass 2)
uniform vec2      screen_res;        // viewport size in pixels
uniform vec2      aya_blur_dir;      // (1,0) horizontal pass 1, (0,1) vertical pass 2
uniform float     aya_strength;      // alpha output (= mix factor when blended)
uniform float     aya_blur_radius;   // tap spacing in pixels
uniform int       aya_visual_realism_enabled;
uniform int       aya_r20_skin_sss_enabled;
// <FS:AYA r20 Phase C> gbuffer3 (DEFERRED_EMISSIVE / "emissiveRect") carries
// the per-pixel skin bit in .a — written by the gbuffer pass for whitelisted
// draws. Bound on pass 2 (composite). On pass 1 (scratch fill) the mask read
// is harmless because pass 1 ignores alpha. Reusing the existing reserved
// uniform name avoids adding a new shader binding plumbing.
uniform sampler2D emissiveRect;
// </FS:AYA>

void main()
{
    vec2 tc = vary_fragcoord.xy;

    // Master / r20 OFF: pass through with alpha=0 so the pass-2 alpha
    // blend (SRC_ALPHA / 1-SRC_ALPHA) leaves screen completely untouched.
    if (aya_visual_realism_enabled <= 0 || aya_r20_skin_sss_enabled <= 0)
    {
        frag_color = vec4(texture(diffuseRect, tc).rgb, 0.0);
        return;
    }

    // 5-tap separable blur. Per-channel weight distribution gives the
    // wavelength-dependent SSS effect: R spreads wide (warm halo around
    // lit areas), B stays tight. All channels are energy-conserving
    // (sum of weights per channel == 1.0).
    //
    // Tap offsets: -2, -1, 0, +1, +2  (uniform spacing)
    //   weights[i].r = red   weight at tap i
    //   weights[i].g = green weight at tap i
    //   weights[i].b = blue  weight at tap i
    const vec3 weights[5] = vec3[5](
        vec3(0.18, 0.10, 0.05),   // tap -2 (far)
        vec3(0.22, 0.22, 0.18),   // tap -1
        vec3(0.20, 0.36, 0.54),   // tap  0 (center)  B dominates
        vec3(0.22, 0.22, 0.18),   // tap +1
        vec3(0.18, 0.10, 0.05)    // tap +2 (far)
    );

    vec2 step = aya_blur_dir * aya_blur_radius / screen_res;

    vec3 sum = vec3(0.0);
    for (int i = 0; i < 5; ++i)
    {
        float t  = float(i) - 2.0;
        vec3  c  = texture(diffuseRect, tc + step * t).rgb;
        sum += c * weights[i];
    }

    // Inner-glow / luminosity gain: real SSS isn't just energy-conserving
    // blur — light penetrates the surface, scatters, and returns slightly
    // boosted in the red wavelength (= the "skin glows from within" feel).
    // We add a small warm bias proportional to local luminance so lit
    // regions get the boost and dark regions stay dark.
    float lit = dot(sum, vec3(0.299, 0.587, 0.114));
    sum += vec3(0.10, 0.04, 0.0) * lit;

    // Alpha = strength. With blend SRC_ALPHA / (1 - SRC_ALPHA):
    //   final = blurred * strength + screen_original * (1 - strength)
    // When strength=1.0 (pass 1 into scratch, blend OFF), the alpha is
    // ignored and the blurred RGB lands raw.
    //
    // <FS:AYA r20 Phase C> mask blur to skin pixels only. gbuffer3.a is
    // written by the gbuffer pass — 1.0 for whitelisted skin draws, 0.0
    // elsewhere. Multiplying strength by the mask zero-blends non-skin
    // pixels (alpha=0 → blend keeps original screen) while preserving the
    // pass-1 behavior since pass 1 writes raw RGB regardless of alpha.
    float skin_mask = texture(emissiveRect, tc).a;
    float masked_strength = aya_strength * skin_mask;
    frag_color = vec4(sum, masked_strength);
    // </FS:AYA>
}
