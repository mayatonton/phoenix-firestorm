/**
 * @file avatarF.glsl
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

out vec4 frag_data[4];

uniform sampler2D diffuseMap;

uniform float minimum_alpha;

// <FS:AYA r20 Phase C> per-draw skin marker: 1.0 if the parent LLViewerObject
// is on the SSS whitelist, 0.0 otherwise. Written into frag_data[3].a so the
// screen-space SSS pass can gate its blur to skin pixels only.
uniform float aya_sss_skin_flag;
// </FS:AYA>

in vec3 vary_normal;
in vec2 vary_texcoord0;
in vec3 vary_position;

void mirrorClip(vec3 pos);
vec4 encodeNormal(vec3 n, float env, float gbuffer_flag);

void main()
{
    mirrorClip(vary_position);

    vec4 diff = texture(diffuseMap, vary_texcoord0.xy);

    if (diff.a < minimum_alpha)
    {
        discard;
    }

    frag_data[0] = vec4(diff.rgb, 0.0);
    frag_data[1] = vec4(0,0,0,0);
    vec3 nvn = normalize(vary_normal);
    frag_data[2] = encodeNormal(nvn.xyz, 0, GBUFFER_FLAG_HAS_ATMOS);

#if defined(HAS_EMISSIVE)
    // <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> AY r20 writes the
    // per-draw SSS skin flag into gbuffer3.a so the SSS pass can gate to
    // skin pixels. Cinematic has no SSS pass, so it writes BD original
    // vec4(0).
#if AYASTORM_CINEMATIC
    frag_data[3] = vec4(0);
#else
    frag_data[3] = vec4(0, 0, 0, aya_sss_skin_flag);
#endif
    // </FS:AYA>
#endif
}

