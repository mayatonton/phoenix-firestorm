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

#ifdef LL_VULKAN_GLSL
layout(location=0) out vec4 frag_color;
#else
out vec4 frag_color;
#endif

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

#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-6 phase 2-A: underWaterF non-opaque uniforms UBO wrap (Cluster F)
// (η-4 §3.2 範式 member-level 拡張) `lightDir` / `eyeVec` は waterV.glsl で
// bare uniform として宣言されており、同一 program (gUnderWaterProgram = waterV +
// underWaterF) に attach されると nameless block member の global scope export
// 衝突 (`nameless block contains a member that already has a name at global
// scope`) が発生する。本 UBO 内側のみ per-block 固有化 rename で衝突解消、
// GL `#else` path は byte-for-byte 不可触 (charter §3 #1 担保)。
// 該当 member は本 file の main() 内で参照されないため #define alias 不要。
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-7 追補:
// `waterFogColor` / `waterFogKS` は waterFogF.glsl `WaterFogUBO_Legacy` (set=3,
// binding=9) member と同名で nameless block member の global scope export 衝突。
// η-6 §3.3 範式 (nameless block × nameless block member collision) 同形 rename で
// 衝突解消。本 file main() で未参照 (`waterFogColorLinear` のみ参照) なので
// #define alias 不要。GL `#else` path 不可触。
layout(set=3, binding=39, std140) uniform UnderWaterFParamUBO_Legacy {
    vec4  fogCol;
    vec3  lightDir_underwater_legacy;
    float lightExp;
    vec3  specular;
    float refScale;
    vec2  fbScale;
    float znear;
    float zfar;
    float kd;
    vec3  eyeVec_underwater_legacy;
    vec4  waterFogColor_underwater_legacy;
    vec3  waterFogColorLinear;
    float waterFogKS_underwater_legacy;
    vec2  screenRes;
};
#else
uniform vec4 fogCol;
uniform vec3 lightDir;
uniform vec3 specular;
uniform float lightExp;
uniform vec2 fbScale;
uniform float refScale;
uniform float znear;
uniform float zfar;
uniform float kd;
#endif
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
uniform vec4 waterPlane;
#endif
#ifndef LL_VULKAN_GLSL
uniform vec3 eyeVec;
uniform vec4 waterFogColor;
uniform vec3 waterFogColorLinear;
uniform float waterFogKS;
uniform vec2 screenRes;
#endif

//bigWave is (refCoord.w, view.w);
#ifdef LL_VULKAN_GLSL
layout(location=26) in vec4 refCoord;
#else
in vec4 refCoord;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=21) in vec4 littleWave;
#else
in vec4 littleWave;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=22) in vec4 view;
#else
in vec4 view;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=3) in vec3 vary_position;
#else
in vec3 vary_position;
#endif

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
