/**
 * @file onetexturefilterF.glsl
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

layout(set = 1, binding = 0, std140) uniform OneTextureFilter_PerProgramBind
{
    float tolerance;
#ifndef _AYA_UM__pad0
#define _AYA_UM__pad0 1
    float _pad0;
#else
    float _dup_OneTextureFilter__pad0;
#endif
#ifndef _AYA_UM__pad1
#define _AYA_UM__pad1 1
    float _pad1;
#else
    float _dup_OneTextureFilter__pad1;
#endif
#ifndef _AYA_UM__pad2
#define _AYA_UM__pad2 1
    float _pad2;
#else
    float _dup_OneTextureFilter__pad2;
#endif
};

layout(location = 0) in vec2 vary_texcoord0;
#else
out vec4 frag_color;

uniform sampler2D tex0;
uniform float tolerance;

in vec2 vary_texcoord0;
#endif

void main()
{
    frag_color = texture(tex0, vary_texcoord0.xy);

    if(frag_color[0] + frag_color[1] + frag_color[2] < tolerance)
    {
        discard;
    }
    else
    {
        frag_color[3] = 0.95f;
    }
}
