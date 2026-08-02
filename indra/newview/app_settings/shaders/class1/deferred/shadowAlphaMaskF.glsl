/**
 * @file shadowAlphaMaskF.glsl
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

/*[EXTRA_CODE_HERE]*/

#ifdef LL_VULKAN_GLSL
layout(location = 0) out vec4 frag_color;

layout(location = 0) in vec4 post_pos;
layout(location = 1) in float target_pos_x;
layout(location = 2) in vec4 vertex_color;
layout(location = 3) in vec2 vary_texcoord0;
#else
out vec4 frag_color;

in vec4 post_pos;
in float target_pos_x;
in vec4 vertex_color;
in vec2 vary_texcoord0;
#endif

#ifdef LL_VULKAN_GLSL
layout(push_constant) uniform ShadowAlphaMaskF_AlphaMaskPC
{
    layout(offset = 64) float minimum_alpha;
    layout(offset = 68) float object_alpha;
};

const float aya_bayer4x4[16] = float[16](
     0.0,  8.0,  2.0, 10.0,
    12.0,  4.0, 14.0,  6.0,
     3.0, 11.0,  1.0,  9.0,
    15.0,  7.0, 13.0,  5.0);
#else
uniform float minimum_alpha;
#endif

// <FS:AYA r30 Phase 3.8 Cinematic mount strategy C>
#if AYASTORM_CINEMATIC
void bayerDitherDiscard(float alpha, float threshold);
#endif
// </FS:AYA>

void main()
{
#if defined(LL_MULTIVIEW_SHADOW) && defined(AYA_BINDLESS)
    float cutoff = (minimum_alpha >= 0.0) ? minimum_alpha : aya_dd[aya_draw_id].misc.z;
#else
    float cutoff = minimum_alpha;
#endif
    float alpha = diffuseLookup(vary_texcoord0.xy).a;

    if (alpha < cutoff)
    {
        discard;
    }

#if AYASTORM_CINEMATIC
#if !defined(IS_FULLBRIGHT)
    alpha *= vertex_color.a;
#endif
    bayerDitherDiscard(alpha, cutoff);
#else
#ifdef LL_VULKAN_GLSL
    if (object_alpha < 0.996)
    {
        int bx = int(gl_FragCoord.x) & 3;
        int by = int(gl_FragCoord.y) & 3;
        float t = (aya_bayer4x4[by * 4 + bx] + 0.5) * (1.0 / 16.0);
        if (object_alpha < t)
        {
            discard;
        }
    }
#else
#if !defined(IS_FULLBRIGHT)
    alpha *= vertex_color.a;
#endif
    if (alpha < 0.05)
    {
        discard;
    }

    if (alpha < 0.88)
    {
        if (fract(0.5*floor(gl_FragCoord.x)) < 0.25)
        {
            discard;
        }
    }
#endif
#endif

    frag_color = vec4(1,1,1,1);
}
