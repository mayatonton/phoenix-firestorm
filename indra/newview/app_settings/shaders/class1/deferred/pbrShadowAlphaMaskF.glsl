/**
 * @file pbrShadowAlphaMaskF.glsl
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
layout(push_constant) uniform PbrShadowAlphaMaskF_PC
{
    layout(offset = 64) float minimum_alpha;
    layout(offset = 68) float object_alpha;
};
layout(set = 1, binding = 1) uniform sampler2D diffuseMap;
#else
uniform sampler2D diffuseMap;
uniform float minimum_alpha;
#endif

void main()
{
    float alpha = texture(diffuseMap,vary_texcoord0.xy).a * vertex_color.a;
#ifdef LL_VULKAN_GLSL
    alpha *= object_alpha;
#endif

    if (alpha < minimum_alpha)
    {
        discard;
    }

    frag_color = vec4(1,1,1,1);
}
