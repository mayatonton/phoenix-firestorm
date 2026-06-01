/**
 * @file velocityAlphaF.glsl
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

// AYAstorm r30 P2: imported from BlackDragon Viewer (NiranV Dean), 995a1354d8, 2026-04-19
// Source: https://github.com/NiranV/Black-Dragon-Viewer @ indra/newview/app_settings/shaders/class1/deferred/velocityAlphaF.glsl
// License: LGPL-2.1-only (same as Second Life Viewer Source Code, no relicensing)

/*[EXTRA_CODE_HERE]*/

#ifdef LL_VULKAN_GLSL
layout(location=0) out vec4 frag_color;
#else
out vec4 frag_color;
#endif

vec4 diffuseLookup(vec2 texcoord);
// <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> bayerDitherDiscard
// is defined in globalF.glsl (restored in step 2 strategy B overwrite),
// so Cinematic can call it directly. AY mode keeps the plain cutoff
// fallback that's been live since r30 P2.
#if AYASTORM_CINEMATIC
void bayerDitherDiscard(float alpha, float threshold);
#endif
// </FS:AYA>

#ifdef LL_VULKAN_GLSL
layout(location=12) in vec4 vary_cur_clip;
#else
in vec4 vary_cur_clip;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=13) in vec4 vary_last_clip;
#else
in vec4 vary_last_clip;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=0) in vec2 vary_texcoord0;
#else
in vec2 vary_texcoord0;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=2) in vec4 vertex_color;
#else
in vec4 vertex_color;
#endif

void main()
{
    float alpha = diffuseLookup(vary_texcoord0.xy).a;
    alpha *= vertex_color.a;

    // <FS:AYA r30 Phase 3.8 Cinematic mount strategy C>
#if AYASTORM_CINEMATIC
    bayerDitherDiscard(alpha, 0.88);
#else
    // AYAstorm r30 P2: BD calls bayerDitherDiscard(alpha, 0.88) here, but the helper isn't
    // wired into AY's velocity pipeline. For the velocity buffer the dither pattern isn't
    // load-bearing — we just need to skip fully-transparent pixels so they don't overwrite
    // the velocity of opaque geometry behind them. Plain cutoff is sufficient.
    if (alpha < 0.1) discard;
#endif
    // </FS:AYA>

    vec2 cur_ndc  = vary_cur_clip.xy / vary_cur_clip.w;
    vec2 last_ndc = vary_last_clip.xy / vary_last_clip.w;

    frag_color = vec4(cur_ndc - last_ndc, 0.0, 1.0);
}
