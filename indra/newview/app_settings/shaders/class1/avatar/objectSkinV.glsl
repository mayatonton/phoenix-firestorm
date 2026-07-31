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
layout(location = 10) in vec4 weight4;
#else
in vec4 weight4;
#endif

#ifdef LL_VULKAN_GLSL
layout(set = 1, binding = 46, std140) uniform ObjectSkin_PerProgramBind
{
#ifndef _AYA_UM_matrixPalette
#define _AYA_UM_matrixPalette 1
    mat3x4 matrixPalette[MAX_JOINTS_PER_MESH_OBJECT];
#else
    mat3x4 _dup_ObjectSkin_matrixPalette[MAX_JOINTS_PER_MESH_OBJECT];
#endif
#ifndef _AYA_UM_lastMatrixPalette
#define _AYA_UM_lastMatrixPalette 1
    mat3x4 lastMatrixPalette[MAX_JOINTS_PER_MESH_OBJECT];
#else
    mat3x4 _dup_ObjectSkin_lastMatrixPalette[MAX_JOINTS_PER_MESH_OBJECT];
#endif
};

// AYAstorm B.2: bindless skin palette. All avatars' palettes live in one SSBO; each
// draw's base entry index arrives via aya_skin_base[gl_InstanceIndex]. A base of
// AYA_SKIN_INVALID means "not published this frame" -> fall back to the dynamic UBO.
// std430 mat3x4[] stride (48B) matches the UBO shadow byte-for-byte.
#ifdef AYA_SKIN_SSBO
#define AYA_SKIN_ENTRY_STRIDE (MAX_JOINTS_PER_MESH_OBJECT * 2)
#define AYA_SKIN_INVALID 0xFFFFFFFFu
layout(set = 3, binding = 1, std430) readonly buffer AyaSkinPaletteBlock { mat3x4 aya_skin_palette[]; };
layout(set = 3, binding = 0, std430) readonly buffer AyaSkinBaseBlock    { uint   aya_skin_base[]; };
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

    mat3x4 p1 = matrixPalette[i1];
    mat3x4 p2 = matrixPalette[i2];
    mat3x4 p3 = matrixPalette[i3];
    mat3x4 p4 = matrixPalette[i4];

#if defined(LL_VULKAN_GLSL) && defined(AYA_SKIN_SSBO)
    uint aya_base = aya_skin_base[gl_InstanceIndex];
    if (aya_base != AYA_SKIN_INVALID)
    {
        uint b = aya_base * uint(AYA_SKIN_ENTRY_STRIDE);
        p1 = aya_skin_palette[b + uint(i1)];
        p2 = aya_skin_palette[b + uint(i2)];
        p3 = aya_skin_palette[b + uint(i3)];
        p4 = aya_skin_palette[b + uint(i4)];
    }
#endif

    mat3 mat = mat3(p1)*w.x;
         mat += mat3(p2)*w.y;
         mat += mat3(p3)*w.z;
         mat += mat3(p4)*w.w;

    vec3 trans = vec3(p1[0].w,p1[1].w,p1[2].w)*w.x;
         trans += vec3(p2[0].w,p2[1].w,p2[2].w)*w.y;
         trans += vec3(p3[0].w,p3[1].w,p3[2].w)*w.z;
         trans += vec3(p4[0].w,p4[1].w,p4[2].w)*w.w;

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

    mat3x4 p1 = lastMatrixPalette[i1];
    mat3x4 p2 = lastMatrixPalette[i2];
    mat3x4 p3 = lastMatrixPalette[i3];
    mat3x4 p4 = lastMatrixPalette[i4];

#if defined(LL_VULKAN_GLSL) && defined(AYA_SKIN_SSBO)
    uint aya_base = aya_skin_base[gl_InstanceIndex];
    if (aya_base != AYA_SKIN_INVALID)
    {
        uint b = aya_base * uint(AYA_SKIN_ENTRY_STRIDE) + uint(MAX_JOINTS_PER_MESH_OBJECT);
        p1 = aya_skin_palette[b + uint(i1)];
        p2 = aya_skin_palette[b + uint(i2)];
        p3 = aya_skin_palette[b + uint(i3)];
        p4 = aya_skin_palette[b + uint(i4)];
    }
#endif

    mat3 mat = mat3(p1)*w.x;
         mat += mat3(p2)*w.y;
         mat += mat3(p3)*w.z;
         mat += mat3(p4)*w.w;

    vec3 trans = vec3(p1[0].w, p1[1].w, p1[2].w)*w.x;
         trans += vec3(p2[0].w, p2[1].w, p2[2].w)*w.y;
         trans += vec3(p3[0].w, p3[1].w, p3[2].w)*w.z;
         trans += vec3(p4[0].w, p4[1].w, p4[2].w)*w.w;

    mat4 ret;
    ret[0] = vec4(mat[0], 0);
    ret[1] = vec4(mat[1], 0);
    ret[2] = vec4(mat[2], 0);
    ret[3] = vec4(trans, 1.0);

    return ret;
}
