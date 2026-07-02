/**
 * @file class2\wl\atmosphericsHelpersV.glsl
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



// Output variables
vec3 getSunlitColor();
vec3 getAmblitColor();
vec3 getAdditiveColor();
vec3 getAtmosAttenuation();

#ifdef LL_VULKAN_GLSL
layout(set = 1, binding = 11, std140) uniform WindlightLight_PerProgramBind
{
    float scene_light_strength;
    float _pad_light0;
    float _pad_light1;
    float _pad_light2;
};
#else
uniform float scene_light_strength;
#endif

vec3 atmosAmbient()
{
    return getAmblitColor();
}

vec3 atmosAffectDirectionalLight(float lightIntensity)
{
    return getSunlitColor() * lightIntensity;
}

vec3 atmosGetDiffuseSunlightColor()
{
    return getSunlitColor();
}

vec3 scaleDownLight(vec3 light)
{
    return (light / scene_light_strength );
}


