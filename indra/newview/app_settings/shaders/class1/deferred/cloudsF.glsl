/**
 * @file class1\deferred\cloudsF.glsl
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
/*[EXTRA_CODE_HERE]*/

#ifdef LL_VULKAN_GLSL
layout(location=0) out vec4 frag_data[4];
#else
out vec4 frag_data[4];
#endif

/////////////////////////////////////////////////////////////////////////
// The fragment shader for the sky
/////////////////////////////////////////////////////////////////////////

#ifdef LL_VULKAN_GLSL
layout(location=26) in vec3 vary_CloudColorSun;
#else
in vec3 vary_CloudColorSun;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=27) in vec3 vary_CloudColorAmbient;
#else
in vec3 vary_CloudColorAmbient;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=22) in float vary_CloudDensity;
#else
in float vary_CloudDensity;
#endif

#ifdef LL_VULKAN_GLSL
layout(set=0, binding=15) uniform sampler2D cloud_noise_texture;
layout(set=0, binding=16) uniform sampler2D cloud_noise_texture_next;
#else
uniform sampler2D cloud_noise_texture;
uniform sampler2D cloud_noise_texture_next;
#endif
#ifdef LL_VULKAN_GLSL
layout(set=3, binding=4, std140) uniform CloudsFParamUBO_Legacy {
    vec3  cloud_pos_density1;
    float blend_factor;
    vec3  cloud_pos_density2;
    float cloud_variance;
    int   aya_r18_cloud_volumetric_enabled;
    float aya_r18_strength;
    float _pad_clouds_f_legacy_0;
    float _pad_clouds_f_legacy_1;
};
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-14: CloudsVParamUBO_Legacy 複製 (Path G-β)
//   cloudsF.main() L127/L132/L133 が cloud_scale を参照するが、Vulkan path の
//   CloudsFParamUBO_Legacy には未収載。cloud_scale は元から CloudsVParamUBO_Legacy
//   (set=3 binding=3) member で、C++ 側 descriptor set は pipeline 単位で両 stage
//   から参照可能なため、fragment 側にも同 layout を guard 付きで複製して解決する。
//   shader-only (charter §3 #1)、C++ struct 改修不要。
#ifndef CLOUDS_V_PARAM_UBO_LEGACY_DEFINED
#define CLOUDS_V_PARAM_UBO_LEGACY_DEFINED 1
layout(set=1, binding=8, std140) uniform CloudsVParamUBO_Legacy {
    vec3  camPosLocal;
    float cloud_scale;
    vec3  cloud_color;
    float _pad_clouds_v_legacy_0;
};
#endif
#else
uniform float blend_factor;
uniform vec3 cloud_pos_density1;
uniform vec3 cloud_pos_density2;
uniform float cloud_scale;
uniform float cloud_variance;
uniform int aya_r18_cloud_volumetric_enabled;  // <FS:AYA r18>
uniform float aya_r18_strength;  // <FS:AYAstorm r30 BD改善> r18 効果強度 (0=legacy / 1=volumetric、enabled 内で lerp)
#endif

#ifdef LL_VULKAN_GLSL
layout(location=0) in vec2 vary_texcoord0;
#else
in vec2 vary_texcoord0;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=5) in vec2 vary_texcoord1;
#else
in vec2 vary_texcoord1;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=16) in vec2 vary_texcoord2;
#else
in vec2 vary_texcoord2;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=17) in vec2 vary_texcoord3;
#else
in vec2 vary_texcoord3;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=23) in float altitude_blend_factor;
#else
in float altitude_blend_factor;
#endif

vec4 cloudNoise(vec2 uv)
{
   vec4 a = texture(cloud_noise_texture, uv);
   vec4 b = texture(cloud_noise_texture_next, uv);
   vec4 cloud_noise_sample = mix(a, b, blend_factor);
   return cloud_noise_sample;
}

void main()
{
    // Set variables
    vec2 uv1 = vary_texcoord0.xy;
    vec2 uv2 = vary_texcoord1.xy;

    vec3 cloudColorSun = vary_CloudColorSun;
    vec3 cloudColorAmbient = vary_CloudColorAmbient;
    float cloudDensity = vary_CloudDensity;
    vec2 uv3 = vary_texcoord2.xy;
    vec2 uv4 = vary_texcoord3.xy;

    if (cloud_scale < 0.001)
    {
        discard;
    }

    vec2 disturbance  = vec2(cloudNoise(uv1 / 8.0f).x, cloudNoise((uv3 + uv1) / 16.0f).x) * cloud_variance * (1.0f - cloud_scale * 0.25f);
    vec2 disturbance2 = vec2(cloudNoise((uv1 + uv3) / 4.0f).x, cloudNoise((uv4 + uv2) / 8.0f).x) * cloud_variance * (1.0f - cloud_scale * 0.25f);

    // Offset texture coords
    uv1 += cloud_pos_density1.xy + (disturbance * 0.2);    //large texture, visible density
    uv2 += cloud_pos_density1.xy;   //large texture, self shadow
    uv3 += cloud_pos_density2.xy;   //small texture, visible density
    uv4 += cloud_pos_density2.xy;   //small texture, self shadow

    float density_variance = min(1.0, (disturbance.x* 2.0 + disturbance.y* 2.0 + disturbance2.x + disturbance2.y) * 4.0);

    cloudDensity *= 1.0 - (density_variance * density_variance);

    // Compute alpha1, the main cloud opacity

    float alpha1;
    // <FS:AYA r18> Cloud Volumetric: slab raymarch で疑似体積化 (A 軸)
    //   既存 2D noise を 4 step、UV 空間を slab 方向に進めながら sample。
    //   Beer-Lambert 風に transmittance を累積、最終 alpha は (1 - trans)。
    //   OFF パスは既存式と数式上完全一致 (preset 互換維持)。
    // <FS:AYAstorm r30 BD改善> r18 強度 lerp: enabled ON 時は legacy alpha と volumetric alpha
    //   を strength で連続補間。0=legacy 等価、1=現状 volumetric。enabled OFF 時は legacy only。
    //   両 path 評価でコストは増えるが (raymarch 4-step + legacy)、slider 操作のシームレスさを優先。
    {
        // Legacy flat path (preset 互換)
        float legacy_alpha = (cloudNoise(uv1).x - 0.5) + (cloudNoise(uv3).x - 0.5) * cloud_pos_density2.z;
        legacy_alpha = min(max(legacy_alpha + cloudDensity, 0.) * 10 * cloud_pos_density1.z, 1.);
        legacy_alpha = 1. - legacy_alpha * legacy_alpha;
        legacy_alpha = 1. - legacy_alpha * legacy_alpha;

        if (aya_r18_cloud_volumetric_enabled != 0)
        {
            const int N = 4;
            const vec2 slab_offset = vec2(0.013, 0.008);  // UV 空間 slab 進行方向 (視線方向 proxy)
            float trans = 1.0;
            for (int i = 0; i < N; i++)
            {
                float t = (float(i) - 1.5) / 3.0;  // -0.5 ~ +0.5
                vec2 du = slab_offset * t;
                float a = (cloudNoise(uv1 + du).x - 0.5) + (cloudNoise(uv3 + du).x - 0.5) * cloud_pos_density2.z;
                a = min(max(a + cloudDensity, 0.) * 10.0 * cloud_pos_density1.z, 1.);
                a = 1.0 - a * a;
                a = 1.0 - a * a;
                trans *= 1.0 - a * 0.45;  // 各 slab で 45% 透過
            }
            float vol_alpha = 1.0 - trans;
            alpha1 = mix(legacy_alpha, vol_alpha, aya_r18_strength);
        }
        else
        {
            alpha1 = legacy_alpha;
        }
    }
    // </FS:AYAstorm>
    // </FS:AYA>

    alpha1 *= altitude_blend_factor;
    alpha1 = clamp(alpha1, 0.0, 1.0);

    // Compute alpha2, for self shadowing effect
    // (1 - alpha2) will later be used as percentage of incoming sunlight
    float alpha2 = (cloudNoise(uv2).x - 0.5);
    alpha2 = min(max(alpha2 + cloudDensity, 0.) * 2.5 * cloud_pos_density1.z, 1.);

    // And smooth
    alpha2 = 1. - alpha2;
    alpha2 = 1. - alpha2 * alpha2;

    // Combine
    vec3 color;
    color = (cloudColorSun*(1.-alpha2) + cloudColorAmbient);
    color.rgb = clamp(color.rgb, vec3(0), vec3(1));
    color.rgb *= 2.0;

    /// Gamma correct for WL (soft clip effect).

    frag_data[1] = vec4(0.0,0.0,0.0,0.0);
    frag_data[2] = vec4(0,0,0,GBUFFER_FLAG_SKIP_ATMOS);

#if defined(HAS_EMISSIVE)
    frag_data[0] = vec4(0);
    // <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> r20 SSS skin
    // mask in gbuffer3.a; clouds are never skin. しかし HAS_EMISSIVE 経路で
    // frag_data[3].a=0 にすると softenLightF SKIP_ATMOS 分岐合成で雲自体が
    // 消えるため alpha1 を維持する。skin_mask 誤発火は skinSSSF 側で
    // GBUFFER_FLAG_SKIP_ATMOS を見て gate する方向で別途対応。
    frag_data[3] = vec4(color.rgb, alpha1);
    // </FS:AYA>
#else
    frag_data[0] = vec4(color.rgb, alpha1);
#endif
}

