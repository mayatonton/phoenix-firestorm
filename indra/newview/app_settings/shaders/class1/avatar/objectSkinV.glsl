/**
 * @file objectSkinV.glsl
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
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-5 (c): weight4 attribute guard wrap (B?-η-1 §3.1 範式 variable-level 応用)。
// objectSkinV.glsl (auto-attach via hasObjectSkinning) と skinnedVelocityV.glsl 双方が
// `layout(location=10) in vec4 weight4` を独立宣言、Vulkan/glslang strict mode で redefinition。
// 先 attach (本 file) が guard sentinel を define、後 attach (skinnedVelocityV.glsl) は skip。
// GL path `#else` 側は byte-for-byte 不変 (charter §3 #1 担保)。
#ifndef WEIGHT4_LOCATION_DEFINED
#define WEIGHT4_LOCATION_DEFINED 1
layout(location=10) in vec4 weight4;
#endif
#else
in vec4 weight4;
#endif

#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-3: PerDrawUBO per-group 固有化 (B?-η-1 patch refinement、§3.1 scope refinement 3rd-level、Group D = object_skin)
#ifndef PER_DRAW_UBO_OBJECT_SKIN_DEFINED
#define PER_DRAW_UBO_OBJECT_SKIN_DEFINED 1
layout(set=2, binding=0, std140) uniform PerDrawUBO_ObjectSkin {
    mat3x4 matrixPalette[MAX_JOINTS_PER_MESH_OBJECT];
    mat3x4 lastMatrixPalette[MAX_JOINTS_PER_MESH_OBJECT];
};
#endif
#else
uniform mat3x4 matrixPalette[MAX_JOINTS_PER_MESH_OBJECT];
uniform mat3x4 lastMatrixPalette[MAX_JOINTS_PER_MESH_OBJECT];
#endif

mat4 getObjectSkinnedTransform()
{
    int i;

    vec4 w = fract(weight4);
    vec4 index = floor(weight4);

    index = min(index, vec4(MAX_JOINTS_PER_MESH_OBJECT-1));
    index = max(index, vec4( 0.0));

    w *= 1.0/(w.x+w.y+w.z+w.w);

    int i1 = int(index.x);
    int i2 = int(index.y);
    int i3 = int(index.z);
    int i4 = int(index.w);

    mat3 mat = mat3(matrixPalette[i1])*w.x;
         mat += mat3(matrixPalette[i2])*w.y;
         mat += mat3(matrixPalette[i3])*w.z;
         mat += mat3(matrixPalette[i4])*w.w;

    vec3 trans = vec3(matrixPalette[i1][0].w,matrixPalette[i1][1].w,matrixPalette[i1][2].w)*w.x;
         trans += vec3(matrixPalette[i2][0].w,matrixPalette[i2][1].w,matrixPalette[i2][2].w)*w.y;
         trans += vec3(matrixPalette[i3][0].w,matrixPalette[i3][1].w,matrixPalette[i3][2].w)*w.z;
         trans += vec3(matrixPalette[i4][0].w,matrixPalette[i4][1].w,matrixPalette[i4][2].w)*w.w;

    mat4 ret;

    ret[0] = vec4(mat[0], 0);
    ret[1] = vec4(mat[1], 0);
    ret[2] = vec4(mat[2], 0);
    ret[3] = vec4(trans, 1.0);

    return ret;

#ifdef IS_AMD_CARD
   // If it's AMD make sure the GLSL compiler sees the arrays referenced once by static index. Otherwise it seems to optimise the storage awawy which leads to unfun crashes and artifacts.
   mat3x4 dummy1 = matrixPalette[0];
   mat3x4 dummy2 = matrixPalette[MAX_JOINTS_PER_MESH_OBJECT-1];
#endif

}

mat4 getLastObjectSkinnedTransform()
{
    vec4 w = fract(weight4);
    vec4 index = floor(weight4);

    index = min(index, vec4(MAX_JOINTS_PER_MESH_OBJECT-1));
    index = max(index, vec4(0.0));

    w *= 1.0/(w.x+w.y+w.z+w.w);

    int i1 = int(index.x);
    int i2 = int(index.y);
    int i3 = int(index.z);
    int i4 = int(index.w);

    mat3 mat = mat3(lastMatrixPalette[i1])*w.x;
         mat += mat3(lastMatrixPalette[i2])*w.y;
         mat += mat3(lastMatrixPalette[i3])*w.z;
         mat += mat3(lastMatrixPalette[i4])*w.w;

    vec3 trans = vec3(lastMatrixPalette[i1][0].w, lastMatrixPalette[i1][1].w, lastMatrixPalette[i1][2].w)*w.x;
         trans += vec3(lastMatrixPalette[i2][0].w, lastMatrixPalette[i2][1].w, lastMatrixPalette[i2][2].w)*w.y;
         trans += vec3(lastMatrixPalette[i3][0].w, lastMatrixPalette[i3][1].w, lastMatrixPalette[i3][2].w)*w.z;
         trans += vec3(lastMatrixPalette[i4][0].w, lastMatrixPalette[i4][1].w, lastMatrixPalette[i4][2].w)*w.w;

    mat4 ret;
    ret[0] = vec4(mat[0], 0);
    ret[1] = vec4(mat[1], 0);
    ret[2] = vec4(mat[2], 0);
    ret[3] = vec4(trans, 1.0);

    return ret;
}
