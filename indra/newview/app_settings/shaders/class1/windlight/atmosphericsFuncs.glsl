/**
 * @file class2\windlight\atmosphericsFuncs.glsl
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

uniform vec3  lightnorm;
uniform vec3  sunlight_color;
uniform vec3  moonlight_color;
uniform int   sun_up_factor;
uniform vec3  ambient_color;
uniform vec3  blue_horizon;
uniform vec3  blue_density;
uniform float haze_horizon;
uniform float haze_density;
uniform float cloud_shadow;
uniform float density_multiplier;
uniform float distance_multiplier;
uniform float max_y;
uniform vec3  glow;
uniform float scene_light_strength;
uniform float sun_moon_glow_factor;
uniform float sky_sunlight_scale;
uniform float sky_ambient_scale;
uniform int classic_mode;
uniform int aya_visual_realism_enabled;  // <FS:AYA r14> Visual Realism master switch
uniform int aya_r14_volumetric_atmosphere_enabled;  // <FS:AYAstorm r30 BD改善> r14 個別 gate (AYAstorm View 無条件 ON / Cinematic は cvar opt-in)
uniform float aya_r14_strength;  // <FS:AYAstorm r30 BD改善> r14 効果強度 (0=OFF / 1=ON)
uniform int aya_r16_aerial_perspective_enabled;  // <FS:AYA r16> r16 個別 switch (master 独立)
uniform float aya_r16_strength;  // <FS:AYAstorm r30 BD改善> r16 効果強度 (0=OFF / 1=ON、enabled 内で lerp)

float getAmbientClamp() { return 1.0f; }

vec3 srgb_to_linear(vec3 col);
vec3 linear_to_srgb(vec3 col);  // <FS:AYA r14> scene-referred 積分用

// return colors in sRGB space
void calcAtmosphericVars(vec3 inPositionEye, vec3 light_dir, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 additive,
                         out vec3 atten)
{
    vec3 rel_pos = inPositionEye;

    //(TERRAIN) limit altitude
    if (abs(rel_pos.y) > max_y) rel_pos *= (max_y / rel_pos.y);

    vec3  rel_pos_norm = normalize(rel_pos);
    float rel_pos_len  = length(rel_pos);

    vec3  sunlight     = (sun_up_factor == 1) ? sunlight_color: moonlight_color;

    // <FS:AYA r16 P1.a> aerial perspective: Rayleigh λ^-4 波長依存化 (scene 経路)
    //   値 (1.0, 2.33, 5.71) は 700/550/380 nm の λ^-4 比、Rayleigh 散乱物理近似。
    //   個別 switch AYAR16AerialPerspectiveEnabled で master (r14/r15) と独立に
    //   r16 効果のみ ON/OFF 可能。OFF 時 vec3(1.0) で旧経路等価。
    //   適用先は combined_haze (視線散乱係数) と blue_weight (in-scatter color) のみ。
    //   light_atten (太陽光路) には適用しない: aerial perspective ≠ 夕焼け、
    //   太陽光路への波長依存は近景まで黄ばませる副作用がある (実証済)。
    //   skyV.glsl (sky dome 経路) は触らない: sun disc 消失の原因経路と P0 で確認済。
    //   atmosFragLighting は light *= atten.r でスカラー化するため combined_haze の
    //   波長依存は surface 直接透過には殆ど効かず、additive 経由で効果が出る設計。
    // <FS:AYAstorm r30 BD改善> r16 強度 lerp: 0=旧 (vec3(1.0)) / 1=現状 r16 (vec3(1.0, 2.33, 5.71))
    //   enabled OFF 時は強度に関係なく vec3(1.0) で旧経路、enabled ON 時に strength で連続補間。
    vec3 rayleigh_w = (aya_r16_aerial_perspective_enabled > 0)
        ? mix(vec3(1.0), vec3(1.0, 2.33, 5.71), aya_r16_strength)
        : vec3(1.0);
    // </FS:AYAstorm>
    // </FS:AYA>

    // sunlight attenuation effect (hue and brightness) due to atmosphere
    // this is used later for sunlight modulation at various altitudes
    // <FS:AYA r16 P1.a fix> light_atten は太陽→地表の経路長依存 (= 夕焼け方向),
    //   aerial perspective (= 視線方向の散乱) とは別現象。rayleigh_w 適用を撤回。
    //   常時夕焼け化で近景まで黄ばむ副作用を回避するため。
    vec3 light_atten = (blue_density + vec3(haze_density * 0.25)) * (density_multiplier * max_y);
    // </FS:AYA>
    // I had thought blue_density and haze_density should have equal weighting,
    // but attenuation due to haze_density tends to seem too strong

    vec3 combined_haze = max(blue_density * rayleigh_w + vec3(haze_density), vec3(1e-6));
    // <FS:AYA r16 P1.a> blue_weight にも rayleigh_w を反映: in-scatter coefficient の RGB weight が
    //   短波長強になり、距離伸長と共に additive (in-scatter color) が B 方向にシフトする。
    //   これが「遠景が青くかすむ」aerial perspective の物理メカニズム。
    //   OFF 時 rayleigh_w = vec3(1.0) で旧 `blue_density / combined_haze` と等価。
    vec3 blue_weight   = (blue_density * rayleigh_w) / combined_haze;
    // </FS:AYA>
    vec3 haze_weight   = vec3(haze_density) / combined_haze;

    //(TERRAIN) compute sunlight from lightnorm y component. Factor is roughly cosecant(sun elevation) (for short rays like terrain)
    float above_horizon_factor = 1.0 / max(1e-6, lightnorm.y);
    sunlight *= exp(-light_atten * above_horizon_factor);  // for sun [horizon..overhead] this maps to an exp curve [0..1]

    // main atmospheric scattering line integral
    float density_dist = rel_pos_len * density_multiplier;

    // <FS:AYA r14> altitude density: 視線終点高度に応じて空気密度を勾配化
    // 地表近くは濃く、上空ほど薄く (指数勾配)、scale_height は max_y の半分を経験値として使用
    // rel_pos.y は eye-space Y で、L57 の `if (abs(rel_pos.y) > max_y)` clamp が altitude として扱っているのを踏襲
    if (aya_r14_volumetric_atmosphere_enabled > 0)
    {
        float altitude = max(rel_pos.y, 0.0);
        float scale_height = max(max_y * 0.1, 1.0);  // 0-div 安全 (r14 P1.a tune: 0.5→0.1 で勾配強化)
        float altitude_factor = exp(-altitude / scale_height);
        // <FS:AYAstorm r30 BD改善> r14 強度 lerp: 0=旧 (×1.0) / 1=現状 r14 (×altitude_factor)
        density_dist *= mix(1.0, altitude_factor, aya_r14_strength);
        // </FS:AYAstorm>
    }
    // </FS:AYA>

    // Transparency (-> combined_haze)
    // ATI Bugfix -- can't store combined_haze*density_dist*distance_multiplier in a variable because the ati
    // compiler gets confused.
    combined_haze = exp(-combined_haze * density_dist * distance_multiplier);

    // final atmosphere attenuation factor
    atten = combined_haze.rgb;

    // compute haze glow
    float haze_glow = dot(rel_pos_norm, lightnorm.xyz);

    // dampen sun additive contrib when not facing it...
    // SL-13539: This "if" clause causes an "additive" white artifact at roughly 77 degreees.
    //    if (length(light_dir) > 0.01)
    haze_glow *= max(0.0f, dot(light_dir, rel_pos_norm));

    haze_glow = 1. - haze_glow;
    // haze_glow is 0 at the sun and increases away from sun
    haze_glow = max(haze_glow, .001);  // set a minimum "angle" (smaller glow.y allows tighter, brighter hotspot)
    haze_glow *= glow.x;
    // higher glow.x gives dimmer glow (because next step is 1 / "angle")
    haze_glow = clamp(pow(haze_glow, glow.z), -100000, 100000);
    // glow.z should be negative, so we're doing a sort of (1 / "angle") function

    // add "minimum anti-solar illumination"
    haze_glow += .25;

    haze_glow *= sun_moon_glow_factor;

    vec3 amb_color = ambient_color;

    // increase ambient when there are more clouds
    vec3 tmpAmbient = amb_color + (vec3(1.) - amb_color) * cloud_shadow * 0.5;

    // Similar/Shared Algorithms:
    //     indra\llinventory\llsettingssky.cpp                                        -- LLSettingsSky::calculateLightSettings()
    //     indra\newview\app_settings\shaders\class1\windlight\atmosphericsFuncs.glsl -- calcAtmosphericVars()
    // haze color
    vec3 cs = sunlight.rgb * (1. - cloud_shadow);

    // <FS:AYA r14> scene-referred 積分: 光の混色を linear 空間で行い、出力契約 (sRGB) に合わせて戻す
    // 旧経路は sRGB 空間で乗算/加算しており、blue_horizon/haze_horizon が非線形 sRGB のまま光合成されるため物理整合性が低い。
    // 新経路では preset 色を一旦 linear に展開し、合成後に linear_to_srgb で sRGB に戻して consumer 契約を維持する。
    if (aya_r14_volumetric_atmosphere_enabled > 0)
    {
        vec3 sunlight_lin    = srgb_to_linear(sunlight.rgb);
        vec3 amb_lin         = srgb_to_linear(tmpAmbient.rgb);
        vec3 cs_lin          = sunlight_lin * (1. - cloud_shadow);
        vec3 blue_h_lin      = srgb_to_linear(blue_horizon.rgb);
        vec3 haze_h_lin      = srgb_to_linear(vec3(haze_horizon));

        vec3 additive_lin    = (blue_h_lin * blue_weight.rgb) * (cs_lin + amb_lin)
                             + (haze_h_lin * haze_weight.rgb) * (cs_lin * haze_glow + amb_lin);

        // <FS:AYAstorm r30 BD改善> r14 強度 lerp: 旧 sRGB 経路と新 linear 経路を strength で混合
        vec3 additive_old = (blue_horizon.rgb * blue_weight.rgb) * (cs + tmpAmbient.rgb)
                          + (haze_horizon * haze_weight.rgb) * (cs * haze_glow + tmpAmbient.rgb);
        additive = mix(additive_old, linear_to_srgb(additive_lin), aya_r14_strength);
        // </FS:AYAstorm>
    }
    else
    {
        additive = (blue_horizon.rgb * blue_weight.rgb) * (cs + tmpAmbient.rgb) + (haze_horizon * haze_weight.rgb) * (cs * haze_glow + tmpAmbient.rgb);
    }
    // </FS:AYA>

    // brightness of surface both sunlight and ambient

    sunlit = sunlight.rgb;
    amblit = pow(tmpAmbient.rgb, vec3(0.9)) * 0.57;

    additive *= vec3(1.0 - combined_haze);

    // sanity clamp haze contribution
    additive = min(additive, vec3(10));
}

vec3 srgb_to_linear(vec3 col);

// provide a touch of lighting in the opposite direction of the sun light
    // so areas in shadow don't lose all detail
float ambientLighting(vec3 norm, vec3 light_dir)
{
    float ambient = min(abs(dot(norm.xyz, light_dir.xyz)), 1.0);
    ambient *= 0.5;
    ambient *= ambient;
    ambient = (1.0 - ambient);
    return ambient;
}

// return lit amblit in linear space, leave sunlit and additive in sRGB space
void calcAtmosphericVarsLinear(vec3 inPositionEye, vec3 norm, vec3 light_dir, out vec3 sunlit, out vec3 amblit, out vec3 additive,
                         out vec3 atten)
{
    calcAtmosphericVars(inPositionEye, light_dir, 1.0, sunlit, amblit, additive, atten);

    amblit *= ambientLighting(norm, light_dir);

    if (classic_mode < 1)
    {
        amblit = srgb_to_linear(amblit);
        amblit = vec3(dot(amblit, vec3(0.2126, 0.7152, 0.0722)));
        sunlit = srgb_to_linear(sunlit);
    }

    // multiply to get similar colors as when the "scaleSoftClip" implementation was doubling color values
    // (allows for mixing of light sources other than sunlight e.g. reflection probes)
    sunlit *= sky_sunlight_scale;
    amblit *= sky_ambient_scale;
}
