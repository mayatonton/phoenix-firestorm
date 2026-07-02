/**
 * @file class3/deferred/waterHazeV.glsl
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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
layout(location = 0) in vec3 position;
#else
in vec3 position;
#endif

out vec4 vary_fragcoord;

// forwards
void setAtmosAttenuation(vec3 c);
void setAdditiveColor(vec3 c);

#ifdef LL_VULKAN_GLSL
layout(set = 1, binding = 0, std140) uniform WaterHazeV_PerProgramBind
{
    vec2  screen_res_wh;
    int   above_water_wh;
    float _waterHazeV_pad0;
    vec4  waterPlane_wh;
};
#define _aboveWaterWH above_water_wh
#else
uniform vec2 screen_res;
uniform vec4 waterPlane;
uniform int above_water;
#define _aboveWaterWH above_water
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
#define modelview_projection_matrix (projection_matrix * modelview_matrix)
#else
uniform mat4 modelview_projection_matrix;
#endif

void main()
{
    //transform vertex
    vec4 pos = vec4(position.xyz, 1.0);

    if (_aboveWaterWH > 0)
    {
        pos = modelview_projection_matrix*pos;
    }

    gl_Position = pos;

    // appease OSX GLSL compiler/linker by touching all the varyings we said we would
    setAtmosAttenuation(vec3(1));
    setAdditiveColor(vec3(0));

    vary_fragcoord = pos;
}
