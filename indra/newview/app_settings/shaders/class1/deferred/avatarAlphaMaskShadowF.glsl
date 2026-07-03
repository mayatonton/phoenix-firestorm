/**
 * @file avatarAlphaMaskShadowF.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2005, Linden Research, Inc.
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

layout(location = 1) in float target_pos_x;
layout(location = 0) in float pos_w;
layout(location = 2) in vec2 vary_texcoord0;
#else
out vec4 frag_color;

in float target_pos_x;
in float pos_w;
in vec2 vary_texcoord0;
#endif

#ifdef LL_VULKAN_GLSL
layout(push_constant) uniform AvatarAlphaMaskShadowF_AlphaMaskPC
{
    layout(offset = 64) float minimum_alpha;
};
layout(set = 1, binding = 1) uniform sampler2D diffuseMap;
#else
uniform float minimum_alpha;
uniform sampler2D diffuseMap;
#endif

// <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> Cinematic calls
// bayerDitherDiscard (defined in globalF.glsl, restored in step 2).
#if AYASTORM_CINEMATIC
void bayerDitherDiscard(float alpha, float threshold);
#endif
// </FS:AYA>

void main()
{
    float alpha = texture(diffuseMap, vary_texcoord0.xy).a;

    // <FS:AYA r30 Phase 3.8 Cinematic mount strategy C>
#if AYASTORM_CINEMATIC
    bayerDitherDiscard(alpha, minimum_alpha);
#else
    if (alpha < 0.05) // treat as totally transparent
    {
        discard;
    }

    if (alpha < minimum_alpha)
    {
      if (fract(0.5*floor(gl_FragCoord.x)) < 0.25)
      {
        discard;
      }
    }
#endif
    // </FS:AYA>

    frag_color = vec4(1,1,1,1);
}
