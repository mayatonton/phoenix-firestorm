/**
 * @file postDeferredNoDoFF.glsl
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

/*[EXTRA_CODE_HERE]*/

#ifdef LL_VULKAN_GLSL
layout(location = 0) out vec4 frag_color;

layout(set = 1, binding = 1) uniform sampler2D diffuseRect;

layout(set = 1, binding = 0, std140) uniform PostVisualizeBuffers_PerProgramBind
{
#ifndef _AYA_UM_mipLevel
#define _AYA_UM_mipLevel 1
    float mipLevel;
#else
    float _dup_PostVisualizeBuffers_mipLevel;
#endif
};

layout(location = 0) in vec2 vary_fragcoord;
#else
out vec4 frag_color;

uniform sampler2D diffuseRect;

uniform float mipLevel;

in vec2 vary_fragcoord;
#endif

void main()
{
    vec4 diff = textureLod(diffuseRect, vary_fragcoord.xy, mipLevel);

    // <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> BD remaps signed
    // float gbuffer visualizations into [0,1] for inspection. AY shows
    // the raw value so HDR debug stays comparable across views.
#if AYASTORM_CINEMATIC
    frag_color = (diff * 0.5 + 0.5);
#else
    frag_color = diff;
#endif
    // </FS:AYA>
}

