/**
 * @file SMAABlendWeightsF.glsl
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

in vec2 vary_texcoord0;
in vec2 vary_pixcoord;
in vec4 vary_offset[3];

uniform sampler2D edgesTex;
uniform sampler2D areaTex;
uniform sampler2D searchTex;
// <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> BD wires
// subsampleIndices as a uniform for SMAA T2x jitter. AY hard-codes
// vec4(0.0) since the AY T2x path doesn't use the SMAA helper.
#if AYASTORM_CINEMATIC
uniform vec4 subsampleIndices;
#endif
// </FS:AYA>

vec4 SMAABlendingWeightCalculationPS(vec2 texcoord,
                                       vec2 pixcoord,
                                       vec4 offset[3],
                                       sampler2D edgesTex,
                                       sampler2D areaTex,
                                       sampler2D searchTex,
                                       vec4 subsampleIndices);

void main()
{
    frag_color = SMAABlendingWeightCalculationPS(vary_texcoord0,
                                                 vary_pixcoord,
                                                 vary_offset,
                                                 edgesTex,
                                                 areaTex,
                                                 searchTex,
                                                 // <FS:AYA r30 Phase 3.8>
#if AYASTORM_CINEMATIC
                                                 subsampleIndices
#else
                                                 vec4(0.0)
#endif
                                                 // </FS:AYA>
                                                 );
}

