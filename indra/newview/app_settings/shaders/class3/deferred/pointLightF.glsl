/**
 * @file class3\deferred\pointLightF.glsl
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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
layout(location=0) out vec4 frag_color;
#else
out vec4 frag_color;
#endif

#ifdef LL_VULKAN_GLSL
layout(set=0, binding=5) uniform sampler2D lightFunc;
#else
uniform sampler2D lightFunc;
#endif

#ifndef LL_VULKAN_GLSL
uniform vec3 env_mat[3];
#endif
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2c:
//   pointLightF.glsl ("Deferred Light Shader" F-stage 実 attach) の plain uniform を UBO 化。
//   sun_wash / falloff / viewport / global_light_strength を 1 UBO に統合 (元 L43 / L59 / L95 / L128)。
//   sun_wash は GLSL 本体未参照 dead-uniform だが parse 通過のため UBO 含める。
//   FrameViewProj 内 inv_proj/screen_res、FrameAtmosphere_Lighting 内 classic_mode、
//   PerDrawUBO_LightParams 内 color/size 既参照、二重宣言禁止。
//   V pair = pointLightV.glsl は η-23 で PerProgramUBO_PointLightV set=2 binding=5 既 UBO 化、本群不所持。
#ifndef PER_PROGRAM_UBO_POINT_LIGHT_F_DEFINED
#define PER_PROGRAM_UBO_POINT_LIGHT_F_DEFINED 1
layout(set=2, binding=25, std140) uniform PerProgramUBO_PointLightF {
    vec4  viewport;                // offset 0
    float sun_wash;                // offset 16 (dead uniform、host setter あり / GLSL 本体未参照)
    float falloff;                 // offset 20
    float global_light_strength;   // offset 24
    float _pad0;                   // offset 28 (vec4 boundary 揃え)
};  // total 32
#endif
#else
uniform float sun_wash;
#endif

// light params
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2d-α (Issue B):
//   PerDrawUBO_LightParams の宣言は deferredUtil.glsl L173-185 に 1 元化
//   (η-20 範式の rename `spot_light_color`/`spot_light_size` を source of truth)。
//   同 guard `PER_DRAW_UBO_LIGHT_PARAMS_DEFINED` で deferredUtil 先行 attach 時
//   self UBO 宣言は skip され、deferredUtil L780-783 末尾の `#undef color`/`#undef size`
//   で alias 切断後、本 pointLightF 本体は alias 経路を通さず UBO member 名
//   (`spot_light_color`/`spot_light_size`) を直接参照する形 (本体 rename、§2.1 (2)/(3))。
//   旧記述: η-3 起源の自前 PerDrawUBO_LightParams (anonymous member `color`/`size`) を本 phase で削除。
#else
uniform vec3 color;
uniform float size;
#endif
#ifndef LL_VULKAN_GLSL
uniform float falloff;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=1) in vec4 vary_fragcoord;
#else
in vec4 vary_fragcoord;
#endif
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-25 Phase 1d (第23層 cascade): trans_center
// location 20 → 60 移動 (pointLightV.glsl out 側と同期、η-18 §3.1 50-59 帯使用済で 60 起点)
layout(location=60) in vec3 trans_center;
#else
in vec3 trans_center;
#endif

#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-5: FrameViewProj guard wrap (η-1 §3.1 範式継承)
#ifndef FRAME_VIEW_PROJ_DEFINED
#define FRAME_VIEW_PROJ_DEFINED 1
layout(set=0, binding=0, std140) uniform FrameViewProj {
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    mat4 projection_matrix;
    mat4 inv_proj;
    mat4 proj_mat;
    mat4 last_modelview_matrix;
    mat3 env_mat;
    mat3 normal_matrix;
    vec2 screen_res;
};
#endif
#else
uniform vec2 screen_res;

uniform mat4 inv_proj;
#endif
#ifndef LL_VULKAN_GLSL
uniform vec4 viewport;
#endif
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-4: FrameAtmosphere_Lighting per-group rename (η-3 §3.2 範式)
#ifndef FRAME_ATMOSPHERE_LIGHTING_DEFINED
#define FRAME_ATMOSPHERE_LIGHTING_DEFINED 1
layout(set=0, binding=2, std140) uniform FrameAtmosphere_Lighting {
    vec3  sunlight_color;
    float scene_light_strength;
    vec3  moonlight_color;
    float haze_density;
    vec3  ambient_color;
    float density_multiplier;
    vec3  blue_horizon;
    float distance_multiplier;
    vec3  blue_density;
    float max_y;
    vec3  glow;
    float sky_sunlight_scale;
    float sky_ambient_scale;
    float sky_hdr_scale;
    int   classic_mode;
    int   cube_snapshot;
    float minimum_alpha;
    float max_cof;
    float _pad_atm0;
    float _pad_atm1;
};
#endif
#else
uniform int classic_mode;
#endif

//BD
#ifndef LL_VULKAN_GLSL
uniform float global_light_strength;
#endif

void calcHalfVectors(vec3 lv, vec3 n, vec3 v, out vec3 h, out vec3 l, out float nh, out float nl, out float nv, out float vh, out float lightDist);
float calcLegacyDistanceAttenuation(float distance, float falloff);
vec4 getNorm(vec2 screenpos);
vec4 getPosition(vec2 pos_screen);
vec2 getScreenXY(vec4 clip);
vec2 getScreenCoord(vec4 clip);
vec3 srgb_to_linear(vec3 c);
float getDepth(vec2 tc);

void pbrPunctual(vec3 diffuseColor, vec3 specularColor,
                    float perceptualRoughness,
                    float metallic,
                    vec3 n, // normal
                    vec3 v, // surface point to camera
                    vec3 l, // surface point to light
                    out float nl,
                    out vec3 diff,
                    out vec3 spec);

GBufferInfo getGBuffer(vec2 screenpos);

void main()
{
    vec3 final_color = vec3(0);
    vec2 tc          = getScreenCoord(vary_fragcoord);
    vec3 pos         = getPosition(tc).xyz;
    GBufferInfo gb = getGBuffer(tc);

    vec3 n = gb.normal;

    vec3 diffuse = gb.albedo.rgb;
    vec4 spec    = gb.specular;

    // Common half vectors calcs
    vec3  lv = trans_center.xyz-pos;
    vec3  h, l, v = -normalize(pos);
    float nh, nl, nv, vh, lightDist;
    calcHalfVectors(lv, n, v, h, l, nh, nl, nv, vh, lightDist);

#ifdef LL_VULKAN_GLSL
    // r41 η-28 Phase 2d-α (Issue B): deferredUtil 末尾 #undef size 後の本体参照は UBO member 名直接参照
    if (lightDist >= spot_light_size)
    {
        discard;
    }
    float dist = lightDist / spot_light_size;
#else
    if (lightDist >= size)
    {
        discard;
    }
    float dist = lightDist / size;
#endif
    float dist_atten = calcLegacyDistanceAttenuation(dist, falloff);

    if (GET_GBUFFER_FLAG(gb.gbufferFlag, GBUFFER_FLAG_HAS_PBR))
    {
        vec3 colorEmissive = gb.emissive.rgb;
        vec3 orm = spec.rgb;
        float perceptualRoughness = orm.g;
        float metallic = orm.b;
        vec3 f0 = vec3(0.04);
        vec3 baseColor = diffuse.rgb;

        vec3 diffuseColor = baseColor.rgb*(vec3(1.0)-f0);
        diffuseColor *= 1.0 - metallic;

        vec3 specularColor = mix(f0, baseColor.rgb, metallic);

#ifdef LL_VULKAN_GLSL
        // r41 η-28 Phase 2d-α (Issue B): UBO member 名直接参照
        vec3 intensity = dist_atten * spot_light_color * 3.25; // Legacy attenuation, magic number to balance with legacy materials
#else
        vec3 intensity = dist_atten * color * 3.25; // Legacy attenuation, magic number to balance with legacy materials
#endif

        float nl = 0;
        vec3 diffPunc = vec3(0);
        vec3 specPunc = vec3(0);

        pbrPunctual(diffuseColor, specularColor, perceptualRoughness, metallic, n.xyz, v, normalize(lv), nl, diffPunc, specPunc);

        final_color += intensity* clamp(nl * (diffPunc + specPunc), vec3(0), vec3(10));
    }
    else
    {
        if (nl < 0.0)
        {
            discard;
        }
        diffuse = srgb_to_linear(diffuse);
        spec.rgb = srgb_to_linear(spec.rgb);

        float lit = nl * dist_atten;

#ifdef LL_VULKAN_GLSL
        // r41 η-28 Phase 2d-α (Issue B): UBO member 名直接参照
        final_color = spot_light_color.rgb*lit*diffuse;
#else
        final_color = color.rgb*lit*diffuse;
#endif

        if (spec.a > 0.0)
        {
            lit = min(nl*6.0, 1.0) * dist_atten;

            float sa = nh;
            float fres = pow(1 - vh, 5) * 0.4+0.5;
            float gtdenom = 2 * nh;
            float gt = max(0,(min(gtdenom * nv / vh, gtdenom * nl / vh)));

            if (nh > 0.0)
            {
                float scol = fres*texture(lightFunc, vec2(nh, spec.a)).r*gt/(nh*nl);
#ifdef LL_VULKAN_GLSL
                // r41 η-28 Phase 2d-α (Issue B): UBO member 名直接参照
                final_color += lit*scol*spot_light_color.rgb*spec.rgb;
#else
                final_color += lit*scol*color.rgb*spec.rgb;
#endif
            }
        }

        if (dot(final_color, final_color) <= 0.0)
        {
            discard;
        }
    }

    //BD
    final_color *= global_light_strength;

    float final_scale = 1.0;
    if (classic_mode > 0)
        final_scale = 0.9;
    frag_color.rgb = max(final_color * final_scale, vec3(0));
    frag_color.a = 0.0;
}
