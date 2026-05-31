/**
 * @file class3\environment\underWaterF.glsl
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

out vec4 frag_color;

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=6) uniform sampler2D bumpMap;
layout(set=1, binding=50) uniform sampler2D exclusionTex;
#else
uniform sampler2D bumpMap;
uniform sampler2D exclusionTex;
#endif

#ifdef TRANSPARENT_WATER
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=54) uniform sampler2D screenTex;
#else
uniform sampler2D screenTex;
#endif
#endif

uniform vec4 fogCol;
uniform vec3 lightDir;
uniform vec3 specular;
uniform float lightExp;
uniform vec2 fbScale;
uniform float refScale;
uniform float znear;
uniform float zfar;
uniform float kd;
#ifdef LL_VULKAN_GLSL
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
#else
uniform vec4 waterPlane;
#endif
uniform vec3 eyeVec;
uniform vec4 waterFogColor;
uniform vec3 waterFogColorLinear;
uniform float waterFogKS;
uniform vec2 screenRes;

//bigWave is (refCoord.w, view.w);
in vec4 refCoord;
in vec4 littleWave;
in vec4 view;
in vec3 vary_position;

vec4 applyWaterFogViewLinearNoClip(vec3 pos, vec4 color);
void mirrorClip(vec3 position);

void main()
{
    mirrorClip(vary_position);
    vec2 screen_tc = (refCoord.xy/refCoord.z) * 0.5 + 0.5;
    float water_mask = texture(exclusionTex, screen_tc).r;

    vec4 color;

    //get detail normals
    vec3 wave1 = texture(bumpMap, vec2(refCoord.w, view.w)).xyz*2.0-1.0;
    vec3 wave2 = texture(bumpMap, littleWave.xy).xyz*2.0-1.0;
    vec3 wave3 = texture(bumpMap, littleWave.zw).xyz*2.0-1.0;
    vec3 wavef = normalize(wave1+wave2+wave3);

    //figure out distortion vector (ripply)
    vec2 distort = screen_tc;
    distort = mix(distort, distort+wavef.xy*refScale, water_mask);

#ifdef TRANSPARENT_WATER
    vec4 fb = texture(screenTex, distort);
#else
    vec4 fb = vec4(waterFogColorLinear, 0.0);
#endif

    fb = applyWaterFogViewLinearNoClip(vary_position, fb);

    frag_color = max(fb, vec4(0));
}
