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

out vec4 frag_color;

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=1) uniform sampler2D diffuseMap;
#else
uniform sampler2D diffuseMap;
#endif

in vec4 post_pos;
in float target_pos_x;
in vec4 vertex_color;
in vec2 vary_texcoord0;
#ifdef LL_VULKAN_GLSL
layout(set=0, binding=2, std140) uniform FrameAtmosphere {
    vec3  sunlight_color;
    float scene_light_strength;
    vec3  moonlight_color;
    float haze_density;
    vec3  ambient_color;
    float density_multiplier;
    vec3  blue_horizon;
    float distance_multiplier;
    vec3  blue_density;
    float max_y;
    vec3  glow;
    float sky_sunlight_scale;
    float sky_ambient_scale;
    float sky_hdr_scale;
    int   classic_mode;
    int   cube_snapshot;
    float minimum_alpha;
    float max_cof;
    float _pad_atm0;
    float _pad_atm1;
};
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
    float alpha = texture(diffuseMap,vary_texcoord0.xy).a;

    alpha *= vertex_color.a;

    // <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> Cinematic calls
    // BD's bayerDitherDiscard with the 0.88 threshold; AY mode uses the
    // explicit two-tier discard with the same end result.
#if AYASTORM_CINEMATIC
    bayerDitherDiscard(alpha, minimum_alpha);
#else
    if (alpha < 0.05) // treat as totally transparent
    {
        discard;
    }

    if (alpha < 0.88) // treat as semi-transparent
    {
        if (fract(0.5*floor(target_pos_x / post_pos.w )) < 0.25)
        {
            discard;
        }
    }
#endif
    // </FS:AYA>

    frag_color = vec4(1,1,1,1);
}
