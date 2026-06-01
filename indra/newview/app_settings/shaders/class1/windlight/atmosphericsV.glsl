/**
 * @file class2\wl\atmosphericsV.glsl
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

// out param funcs


#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-2: FrameLights guard wrap (B?-ζ §3.1 範式)
#ifndef FRAME_LIGHTS_DEFINED
#define FRAME_LIGHTS_DEFINED 1
layout(set=0, binding=1, std140) uniform FrameLights {
    int  sun_up_factor;
    vec3 sun_dir;
    vec3 moon_dir;
    vec4 waterPlane;
    vec4 light_position[8];
    vec3 light_direction[8];
    vec4 light_attenuation[8];
    vec3 light_diffuse[8];
    vec2 light_deferred_attenuation[8];
};
#endif
#else
uniform vec3 sun_dir;
uniform vec3 moon_dir;
uniform int sun_up_factor;
#endif

void setSunlitColor(vec3 v);
void setAmblitColor(vec3 v);
void setAdditiveColor(vec3 v);
void setAtmosAttenuation(vec3 v);
void setPositionEye(vec3 v);

vec3 getAdditiveColor();

void calcAtmosphericVars(vec3 inPositionEye, vec3 light_dir, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 additive, out vec3 atten);

void calcAtmospherics(vec3 inPositionEye) {
    vec3 P = inPositionEye;
    vec3 tmpsunlit = vec3(1);
    vec3 tmpamblit = vec3(1);
    vec3 tmpaddlit = vec3(1);
    vec3 tmpattenlit = vec3(1);
    vec3 light_dir = (sun_up_factor == 1) ? sun_dir : moon_dir;
    calcAtmosphericVars(inPositionEye, light_dir, 1, tmpsunlit, tmpamblit, tmpaddlit, tmpattenlit);
    setSunlitColor(tmpsunlit);
    setAmblitColor(tmpamblit);
    setAdditiveColor(tmpaddlit);
    setAtmosAttenuation(tmpattenlit);
}
