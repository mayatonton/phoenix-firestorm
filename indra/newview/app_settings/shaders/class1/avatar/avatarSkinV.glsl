/**
 * @file avatarSkinV.glsl
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
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-5 (c): weight attribute guard wrap (B?-η-1 §3.1 範式 variable-level 応用)。
// avatarSkinV.glsl (auto-attach via hasSkinning) と avatarVelocityV.glsl 双方が
// `layout(location=9) in vec4 weight` を独立宣言、Vulkan/glslang strict mode で redefinition。
// 先 attach (本 file) が guard sentinel を define、後 attach (avatarVelocityV.glsl) は skip。
// GL path `#else` 側は byte-for-byte 不変 (charter §3 #1 担保)。
#ifndef WEIGHT_LOCATION_DEFINED
#define WEIGHT_LOCATION_DEFINED 1
layout(location=9) in vec4 weight;
#endif
#else
in vec4 weight;
#endif

#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-3: PerDrawUBO per-group 固有化 (B?-η-1 patch refinement、§3.1 scope refinement 3rd-level、Group C = avatar_skin)
#ifndef PER_DRAW_UBO_AVATAR_SKIN_DEFINED
#define PER_DRAW_UBO_AVATAR_SKIN_DEFINED 1
layout(set=2, binding=0, std140) uniform PerDrawUBO_AvatarSkin {
    vec4 matrixPalette[45];
};
#endif
#else
uniform vec4 matrixPalette[45];
#endif

mat4 getSkinnedTransform()
{
    mat4 ret;
    int i = int(floor(weight.x));
    float x = fract(weight.x);

    ret[0] = mix(matrixPalette[i+0], matrixPalette[i+1], x);
    ret[1] = mix(matrixPalette[i+15],matrixPalette[i+16], x);
    ret[2] = mix(matrixPalette[i+30],matrixPalette[i+31], x);
    ret[3] = vec4(0,0,0,1);

    return ret;

#ifdef IS_AMD_CARD
    // If it's AMD make sure the GLSL compiler sees the arrays referenced once by static index. Otherwise it seems to optimise the storage awawy which leads to unfun crashes and artifacts.
    vec4 dummy1 = matrixPalette[0];
    vec4 dummy2 = matrixPalette[44];
#endif
}
