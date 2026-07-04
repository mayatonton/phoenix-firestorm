/**
 * @file pbrShadowAlphaBlendF.glsl
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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
layout(push_constant) uniform PbrShadowAlphaBlendF_PC
{
    layout(offset = 64) float minimum_alpha;
    layout(offset = 68) float object_alpha;
};

const float aya_bayer4x4[16] = float[16](
     0.0,  8.0,  2.0, 10.0,
    12.0,  4.0, 14.0,  6.0,
     3.0, 11.0,  1.0,  9.0,
    15.0,  7.0, 13.0,  5.0);
layout(set = 1, binding = 1) uniform sampler2D diffuseMap;
#else
uniform sampler2D diffuseMap;
uniform float minimum_alpha;
#endif

// <FS:AYA r30 Phase 3.8 Cinematic mount strategy C>
#if AYASTORM_CINEMATIC
void bayerDitherDiscard(float alpha, float threshold);
#endif
// </FS:AYA>

void main()
{
    float alpha = texture(diffuseMap,vary_texcoord0.xy).a;

#if AYASTORM_CINEMATIC
    alpha *= vertex_color.a;
    bayerDitherDiscard(alpha, minimum_alpha);
#else
#ifdef LL_VULKAN_GLSL
    if (alpha < 0.05)
    {
        discard;
    }
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
    alpha *= vertex_color.a;
    if (alpha < 0.05)
    {
        discard;
    }

    if (alpha < 0.88)
    {
        if (fract(0.5*floor(target_pos_x / post_pos.w )) < 0.25)
        {
            discard;
        }
    }
#endif
#endif

    frag_color = vec4(1,1,1,1);
}
