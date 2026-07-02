/**
 * @file twotexturecompareF.glsl
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

#ifdef LL_VULKAN_GLSL
layout(location = 0) out vec4 frag_color;

layout(set = 1, binding = 1) uniform sampler2D tex0;
layout(set = 1, binding = 2) uniform sampler2D tex1;
layout(set = 1, binding = 3) uniform sampler2D dither_tex;

layout(set = 1, binding = 0, std140) uniform TwoTextureCompare_PerProgramBind
{
    float dither_scale;
    float dither_scale_s;
    float dither_scale_t;
#ifndef _AYA_UM__pad0
#define _AYA_UM__pad0 1
    float _pad0;
#else
    float _dup_TwoTextureCompare__pad0;
#endif
};

layout(location = 0) in vec2 vary_texcoord0;
layout(location = 1) in vec2 vary_texcoord1;
#else
out vec4 frag_color;

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D dither_tex;
uniform float dither_scale;
uniform float dither_scale_s;
uniform float dither_scale_t;

in vec2 vary_texcoord0;
in vec2 vary_texcoord1;
#endif

void main()
{
    frag_color = abs(texture(tex0, vary_texcoord0.xy) - texture(tex1, vary_texcoord0.xy));

    vec2 dither_coord;
    dither_coord[0] = vary_texcoord0[0] * dither_scale_s;
    dither_coord[1] = vary_texcoord0[1] * dither_scale_t;
    vec4 dither_vec = texture(dither_tex, dither_coord.xy);

    for(int i = 0; i < 3; i++)
    {
        if(frag_color[i] < dither_vec[i] * dither_scale)
        {
            frag_color[i] = 0.f;
        }
    }
}
