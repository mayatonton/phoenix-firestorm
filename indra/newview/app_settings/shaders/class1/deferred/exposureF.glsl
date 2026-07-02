/**
 * @file exposureF.glsl
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

/*[EXTRA_CODE_HERE]*/

#ifdef LL_VULKAN_GLSL
layout(location = 0) out vec4 frag_color;
#else
out vec4 frag_color;
#endif

#ifdef LL_VULKAN_GLSL
layout(set = 1, binding = 1) uniform sampler2D emissiveRect;
#ifdef USE_LAST_EXPOSURE
layout(set = 1, binding = 2) uniform sampler2D exposureMap;
#endif
#else
uniform sampler2D emissiveRect;
#ifdef USE_LAST_EXPOSURE
uniform sampler2D exposureMap;
#endif
#endif

#ifdef LL_VULKAN_GLSL
layout(set = 1, binding = 0, std140) uniform ExposureF_PerProgramBind
{
    vec4 dynamic_exposure_params;
    vec4 dynamic_exposure_params2;
    float dt;
#ifndef _AYA_UM__pad0
#define _AYA_UM__pad0 1
    float _pad0;
#else
    float _dup_ExposureF__pad0;
#endif
#ifndef _AYA_UM__pad1
#define _AYA_UM__pad1 1
    float _pad1;
#else
    float _dup_ExposureF__pad1;
#endif
#ifndef _AYA_UM__pad2
#define _AYA_UM__pad2 1
    float _pad2;
#else
    float _dup_ExposureF__pad2;
#endif
};
#else
uniform float dt;
uniform vec2 noiseVec;

uniform vec4 dynamic_exposure_params;
uniform vec4 dynamic_exposure_params2;
#endif

float lum(vec3 col)
{
    vec3 l = vec3(0.2126, 0.7152, 0.0722);
    return dot(l, col);
}

void main()
{
    vec2 tc = vec2(0.5,0.5);

    float L = textureLod(emissiveRect, tc, 8).r;
    float max_L = dynamic_exposure_params.x;
    L = clamp(L, 0.0, max_L);
    L /= max_L;
    L = pow(L, 2.0);
    float s = mix(dynamic_exposure_params.z, dynamic_exposure_params.y, L);
#ifdef USE_LAST_EXPOSURE
    float prev = texture(exposureMap, vec2(0.5,0.5)).r;

    float speed = -log(dynamic_exposure_params.w) / dynamic_exposure_params2.w;
    s = mix(prev, s, 1 - exp(-speed * dt));
#endif

    frag_color = max(vec4(s, s, s, dt), vec4(0.0));
}

