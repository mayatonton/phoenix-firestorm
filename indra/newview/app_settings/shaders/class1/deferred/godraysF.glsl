/**
 * @file class1/deferred/godraysF.glsl
 *
 * AYAstorm r15: godrays (screen-space light shaft) fragment shader.
 *
 * Per pixel: reconstruct view-space position from depth, ray-march toward
 * the eye along the view ray in N steps, shadow-test each step against the
 * sun cascaded shadow map, accumulate un-shadowed fraction modulated by
 * a forward-peak phase function, output as additive sun radiance.
 *
 * Master switch (aya_visual_realism_enabled) skips the integration when
 * disabled to retain pre-r15 visuals.
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

// view-space sun / moon dir, set via LLPipeline::bindDeferredShader
uniform vec3 sun_dir;
uniform vec3 moon_dir;
uniform int  sun_up_factor;

// atmospheric color uniforms set via LLSettingsVOSky shader binding flow
uniform vec3 sunlight_color;
uniform vec3 moonlight_color;

// inverse projection for far-plane reconstruction when depth==1.0 (sky)
uniform mat4 inv_proj;

// Cascaded sun shadow far-clip vector. Redeclared here so we can skip
// ray-march samples that fall *beyond* the cascade range — sampleDirectional-
// Shadow() returns 1.0 (lit) for those positions, which is correct for
// surface shading but produces a constant full-screen additive term in our
// godrays accumulator (scene whites out / greens shift yellow).
uniform vec4 shadow_clip;

// AYAstorm r15 個別 gate (AYAstorm View 無条件 ON / Cinematic は cvar opt-in)
// <FS:AYAstorm r30 BD改善> master ではなく r15 個別 uniform を見る (Cinematic で master OFF のまま r15 だけ ON 可能)
uniform int aya_r15_godrays_enabled;

// helpers provided by deferred/deferredUtil.glsl + deferred/shadowUtil.glsl
float getDepth(vec2 pos_screen);
vec4  getPositionWithDepth(vec2 pos_screen, float depth);
float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen);

void main()
{
    if (aya_r15_godrays_enabled <= 0)
    {
        frag_color = vec4(0.0);
        return;
    }

    vec2  tc    = vary_fragcoord.xy;
    float depth = getDepth(tc);

    // March endpoint in view-space:
    //   geometry pixel: reconstruct from depth
    //   sky pixel    : push to far plane (use NDC z just shy of 1.0)
    vec3 view_end;
    if (depth >= 1.0)
    {
        vec4 ndc = vec4(tc * 2.0 - 1.0, 0.999, 1.0);
        vec4 v   = inv_proj * ndc;
        view_end = v.xyz / v.w;
    }
    else
    {
        vec4 v   = getPositionWithDepth(tc, depth);
        view_end = v.xyz;
    }

    float dist = length(view_end);
    if (dist < 0.01)
    {
        frag_color = vec4(0.0);
        return;
    }

    vec3 view_dir   = view_end / dist;
    vec3 light_dir  = (sun_up_factor == 1) ? sun_dir : moon_dir;
    vec3 light_color = (sun_up_factor == 1) ? sunlight_color : moonlight_color * 0.7;

    // Shadow-driven ray-march from eye (origin in view-space) to view_end.
    // Each step samples the cascaded sun shadow at the world point along
    // the view ray; lit fraction sums to the un-shadowed path length.
    const int N = 16;
    float dt = dist / float(N);

    // Bayer-ish hash jitter to break banding between adjacent pixels.
    float jitter = fract(sin(dot(tc, vec2(12.9898, 78.233))) * 43758.5453);

    float accum = 0.0;
    for (int i = 0; i < N; ++i)
    {
        vec3 p = view_dir * (float(i) + jitter) * dt;
        // Skip samples that are *beyond* the cascade far split. sampleDirec-
        // tionalShadow's surface-shading semantics return 1.0 (lit) there,
        // which over-accumulates into a full-screen additive haze for us.
        // view-space z is negative going forward, shadow_clip.w is positive.
        if (p.z <= -shadow_clip.w)
        {
            continue;
        }
        // sampleDirectionalShadow returns 1.0 when lit, 0.0 when shadowed.
        // We pass light_dir as the surrogate normal so the bias / PCF use
        // a light-facing offset (no real surface normal at mid-air points).
        // Guard: hasShadows=false (shadow detail = 0) skips shadowUtil
        // attach in llviewershadermgr, so the symbol is undefined unless
        // HAS_SUN_SHADOW is set. Fall back to fully lit so the godrays
        // pass still produces a halo (driven by phase only) without
        // breaking link.
#ifdef HAS_SUN_SHADOW
        float lit = sampleDirectionalShadow(p, light_dir, tc);
#else
        float lit = 1.0;
#endif
        // shadowUtil cascade fallthrough: when a sample sits between near
        // splits and matches none of the four cascade branches, weight=0
        // and `shadow /= weight` yields NaN/+Inf. Godrays' mid-air sample
        // pattern hits this often (close-eye points, between-cascade gaps).
        // Guard so the accumulator stays finite.
        if (isnan(lit) || isinf(lit))
        {
            lit = 0.0;
        }
        lit = clamp(lit, 0.0, 1.0);
        accum += lit;
    }
    accum /= float(N);

    // Mie-style forward peak. Exponent controls how tightly the contribution
    // hugs the sun direction. 8 = visibly wide halo, 16 = moderate, 32 = thin
    // shaft only when looking ~5° from the sun.
    float cos_theta = clamp(dot(view_dir, light_dir), 0.0, 1.0);
    float phase     = pow(cos_theta, 8.0);

    // r15 MVP intensity. AYA 体感調整: 0.5 = loud, 0.2 = ちょっと強い、
    // 0.15 もまだ少し強い、0.10 で「うっすら空気の主張」を狙う。
    const float strength = 0.10;

    vec3 godrays = light_color * accum * phase * strength;

    // Alpha は必ず 0。scene buffer の alpha は doAtmospherics / sky 合成が
    // mask として使うので、blendFunc ONE/ONE のもとで alpha=1.0 を書くと
    // sky 領域が真っ白に潰れる (P1 初回試作で観測された症状)。
    frag_color = vec4(godrays, 0.0);
}
