/**
 * @file class1\lighting\sumLightsV.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2005, Linden Research, Inc.
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
layout(set = 1, binding = 12, std140) uniform Lights_PerProgramBind
{
#ifndef _AYA_UM_light_position
#define _AYA_UM_light_position 1
    vec4 light_position[8];
#else
    vec4 _dup_Lights_light_position[8];
#endif
#ifndef _AYA_UM_light_diffuse
#define _AYA_UM_light_diffuse 1
    vec3 light_diffuse[8];
#else
    vec3 _dup_Lights_light_diffuse[8];
#endif
};
#else
uniform vec4 light_position[8];
uniform vec3 light_diffuse[8];
#endif

float calcDirectionalLight(vec3 n, vec3 l);

vec3 atmosAmbient();
vec3 atmosAffectDirectionalLight(float lightIntensity);
vec3 scaleDownLight(vec3 light);

vec4 sumLights(vec3 pos, vec3 norm, vec4 color)
{
    vec4 col = vec4(0);
    col.a = color.a;

    col.rgb = light_diffuse[1].rgb * calcDirectionalLight(norm, light_position[1].xyz);
    col.rgb += light_diffuse[1].rgb * sqrt(calcDirectionalLight(norm, -light_position[1].xyz)*0.5+0.25);

    col.rgb = min(col.rgb*color.rgb, 1.0);
    return col;
}


