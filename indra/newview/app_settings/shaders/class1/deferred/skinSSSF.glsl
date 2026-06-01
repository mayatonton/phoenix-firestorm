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

#ifdef LL_VULKAN_GLSL
layout(location=0) out vec4 frag_color;
#else
out vec4 frag_color;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=1) in vec2 vary_fragcoord;
#else
in vec2 vary_fragcoord;
#endif

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=4) uniform sampler2D diffuseRect;       // source color (screen for pass 1, scratch for pass 2)
#else
uniform sampler2D diffuseRect;       // source color (screen for pass 1, scratch for pass 2)
#endif
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
uniform vec2      screen_res;        // viewport size in pixels
#endif
uniform vec2      aya_blur_dir;      // (1,0) horizontal pass 1, (0,1) vertical pass 2
uniform float     aya_strength;      // alpha output (= mix factor when blended)
uniform float     aya_blur_radius;   // tap spacing in pixels at ref_dist=1m (world-scaled per-pixel by depth)
uniform float     aya_glow_gain;     // <FS:AYA r20 Phase D> highlight restore strength
uniform vec3      aya_glow_color;    // <FS:AYA r20 Phase D> highlight restore tint
uniform int       aya_visual_realism_enabled;
uniform int       aya_r20_skin_sss_enabled;
// <FS:AYA r20 Phase C> gbuffer3 (DEFERRED_EMISSIVE / "emissiveRect") carries
// the per-pixel skin bit in .a — written by the gbuffer pass for whitelisted
// draws. Bound on pass 2 (composite). On pass 1 (scratch fill) the mask read
// is harmless because pass 1 ignores alpha. Reusing the existing reserved
// uniform name avoids adding a new shader binding plumbing.
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=7) uniform sampler2D emissiveRect;
#else
uniform sampler2D emissiveRect;
#endif
// </FS:AYA>

// <FS:AYA r20 Phase D world-scale blur> screen-space SSS は blur 半径が
// pixel 固定のため、遠距離で顔輪郭ごと舐めて破綻する (近接=良 / 遠=ぼやけ
// のぼやけ)。Jimenez "Separable SSS" 流のアプローチで、blur 半径を世界
// 座標で固定 (= eye_dist で逆スケール) し、遠距離では半径が < 1 px に
// 縮退して自動的に no-op になるようにする。
// 中心 pixel の linear eye depth (= 視点からの距離 [m]) を depth map +
// inv_proj NDC 復元で取得 (cofF.glsl と同手順)。inv_proj は llrender が
// reserved uniform として自動 bind。depthMap は doSkinSSS が DEFERRED_DEPTH
// 経由で bind する。
// ref_dist=1.0m hard-code: 1m 以内は aya_blur_radius がそのまま使われ
// (近接ロールプレイ距離の見えを保つ)、1m を超えると逆スケール。
// 上限を aya_blur_radius に固定することで、超近接 (< 1m) でも blur が
// 暴走しないようにする。
#ifdef LL_VULKAN_GLSL
layout(set=0, binding=3) uniform sampler2D depthMap;
#else
uniform sampler2D depthMap;
#endif
#ifndef LL_VULKAN_GLSL
uniform mat4      inv_proj;
#endif
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

    // <FS:AYA r20 Phase D world-scale blur> per-pixel depth から eye_dist を
    // 復元し、blur 半径を 1m での aya_blur_radius を基準に逆スケール。
    // eye_dist < 1m では aya_blur_radius (上限) で頭打ち。
    float d_raw = texture(depthMap, tc).r;

    // <FS:AYAstorm r30> Sky safety net: gbuffer3.a is the SSS skin mask, but
    // the deferred clear color is (1,0,1,1) so gbuffer3.a starts at 1.0 at
    // sky pixels and is only reset to 0.0 when the sky shader path is
    // compiled with HAS_EMISSIVE. That permutation depends on
    // RenderEnableEmissiveBuffer and on the shader having been recompiled
    // since the cvar was read — both are observable failure modes, and a
    // skin_mask of 1.0 at sky pixels drives MAX SSS over the whole sky
    // (= the "sky turns red when r20 is ON" repro). Depth==1.0 (far plane)
    // is a domain-correct opt-out: sky cannot be a SSS target by
    // definition. Pass 1 (blend off, scratch fill) writes vec4(0) so the
    // scratch buffer holds 0 at sky pixels; pass 2 (composite, blend
    // SRC_ALPHA / 1-SRC_ALPHA) writes alpha=0 so the screen passes through
    // untouched. Both passes are made safe by the same early return.
    if (d_raw >= 0.9999)
    {
        frag_color = vec4(0.0);
        return;
    }
    // </FS:AYAstorm>

    vec4  ndc4  = vec4(0.0, 0.0, d_raw * 2.0 - 1.0, 1.0);
    vec4  vp    = inv_proj * ndc4;
    float eye_dist = abs(vp.z / vp.w);
    float r_eff = aya_blur_radius / max(eye_dist, 1.0);

    vec2 step = aya_blur_dir * r_eff / screen_res;
    // </FS:AYA>

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

    // <FS:AYA r20 Phase D> glow restore: SSS blur で hi-light が眠くなるので、
    // lit を power curve で持ち上げて peak だけ additive で戻す。
    // 低 lit 領域 (lit^3 << 1) は無変化、明るい所だけ非対称に強調する。
    // 色は AYAR20AvatarSkinSSSGlowColor で配信者が選択 (default warm salmon、
    // 真っ白だと「肌が発光してる」感が出るので血色寄りの暖色を初期値に)。
    sum += aya_glow_color * pow(clamp(lit, 0.0, 1.0), 3.0) * aya_glow_gain;
    // </FS:AYA>

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
    // <FS:AYAstorm r30> skin_mask is written by the gbuffer pass as a
    // binary marker (0.0 or 1.0). Threshold at 0.5 to be defensive against
    // partial / interpolated values from shaders that may set .a to
    // something other than exactly {0, aya_sss_skin_flag}.
    float skin_mask = texture(emissiveRect, tc).a;
    float skin_bit  = (skin_mask >= 0.5) ? 1.0 : 0.0;
    frag_color = vec4(sum, aya_strength * skin_bit);
    // </FS:AYA>
}
