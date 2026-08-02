/**
 * @file shadowSkinnedV.glsl
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
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

#ifdef LL_MULTIVIEW_SHADOW
#extension GL_EXT_multiview : enable
layout(set = 1, binding = 54, std140) uniform ShadowViewProjUBO { mat4 shadow_viewproj[4]; };
#endif

#ifdef LL_VULKAN_GLSL
#ifndef PER_FRAME_MATRIX_UBO_DEFINED
#define PER_FRAME_MATRIX_UBO_DEFINED 1
layout(set = 0, binding = 0, std140) uniform PerFrameMatrixUBO
{
    mat4 projection_matrix;
    mat4 inverse_projection_matrix;
    mat4 identity_matrix;
    mat4 last_modelview_matrix;
};
#endif // PER_FRAME_MATRIX_UBO_DEFINED
layout(push_constant) uniform ModelviewPushConstant
{
    mat4 modelview_matrix;
};
#else
uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
#endif

#ifdef LL_VULKAN_GLSL
layout(location = 0) in vec3 position;
#else
in vec3 position;
#endif

mat4 getObjectSkinnedTransform();

void main()
{
    //transform vertex
    mat4 mat = getObjectSkinnedTransform();

    mat = modelview_matrix * mat;
#ifdef LL_MULTIVIEW_SHADOW
    gl_Position = shadow_viewproj[gl_ViewIndex] * mat * vec4(position.xyz, 1.0);
#else
    vec4 pos = (mat*vec4(position.xyz, 1.0));
    pos = projection_matrix*pos;

    gl_Position = pos;
#endif
}
