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

out vec4 frag_color;

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=4) uniform sampler2D diffuseRect;
#else
uniform sampler2D diffuseRect;
#endif

#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2a: PerProgramUBO_VisualizeBuffersF (η-3 §3.2 PerDrawUBO 派生範式)
#ifndef PER_PROGRAM_UBO_VISUALIZE_BUFFERS_F_DEFINED
#define PER_PROGRAM_UBO_VISUALIZE_BUFFERS_F_DEFINED 1
layout(set=2, binding=16, std140) uniform PerProgramUBO_VisualizeBuffersF {
    float mipLevel;
    float _pad_visbuf0;
    float _pad_visbuf1;
    float _pad_visbuf2;
};
#endif
#else
uniform float mipLevel;
#endif

in vec2 vary_fragcoord;

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

