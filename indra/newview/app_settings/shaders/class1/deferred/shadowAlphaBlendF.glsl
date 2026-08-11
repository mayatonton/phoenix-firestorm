/**
 * @file shadowAlphaBlendF.glsl
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
layout(push_constant) uniform ShadowAlphaBlendF_AlphaMaskPC
{
    layout(offset = 64) float minimum_alpha;
    layout(offset = 68) float object_alpha;
};
#else
uniform float minimum_alpha;
#endif

void bayerDitherDiscard(float alpha, float threshold);

void main()
{
    float alpha = diffuseLookup(vary_texcoord0.xy).a;

#ifdef LL_VULKAN_GLSL
#ifdef AYA_BINDLESS
    float obj_a = (object_alpha >= 0.0) ? object_alpha : aya_dd[aya_draw_id].object_alpha;
#else
    float obj_a = object_alpha;
#endif
    bayerDitherDiscard(alpha * obj_a, 0.996);
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

    frag_color = vec4(1,1,1,1);
}
