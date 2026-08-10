/**
 * @file class1\deferred\pbralphaF.glsl
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

#ifndef IS_HUD

#ifdef LL_VULKAN_GLSL
layout(set = 1, binding = 0, std140) uniform PBRAlphaF_PerProgramBind
{
    float metallicFactor;
    float roughnessFactor;
    float _pbralphaF_pad0;
    float _pbralphaF_pad1;
    vec3  emissiveColor;
    float _pbralphaF_pad2;
#ifdef HAS_ALPHA_MASK
#ifndef _AYA_UM_minimum_alpha
#define _AYA_UM_minimum_alpha 1
    float minimum_alpha;
#else
    float _dup_PBRAlphaF_minimum_alpha;
#endif
    float _pbralphaF_pad3;
    float _pbralphaF_pad4;
    float _pbralphaF_pad5;
#endif
#ifndef HAS_SUN_SHADOW
#ifndef _AYA_UM_sun_dir
#define _AYA_UM_sun_dir 1
    vec3  sun_dir;
#else
    vec3  _dup_PBRAlphaF_sun_dir;
#endif
    float _pbralphaF_pad6;
#ifndef _AYA_UM_moon_dir
#define _AYA_UM_moon_dir 1
    vec3  moon_dir;
#else
    vec3  _dup_PBRAlphaF_moon_dir;
#endif
    float _pbralphaF_pad7;
#endif
#ifndef _AYA_UM_light_position
#define _AYA_UM_light_position 1
    vec4  light_position[8];
#else
    vec4  _dup_PBRAlphaF_light_position[8];
#endif
#ifndef _AYA_UM_light_direction
#define _AYA_UM_light_direction 1
    vec4  light_direction[8];
#else
    vec4  _dup_PBRAlphaF_light_direction[8];
#endif
#ifndef _AYA_UM_light_attenuation
#define _AYA_UM_light_attenuation 1
    vec4  light_attenuation[8];
#else
    vec4  _dup_PBRAlphaF_light_attenuation[8];
#endif
#ifndef _AYA_UM_light_diffuse
#define _AYA_UM_light_diffuse 1
    vec4  light_diffuse[8];
#else
    vec4  _dup_PBRAlphaF_light_diffuse[8];
#endif
#ifndef _AYA_UM_light_deferred_attenuation
#define _AYA_UM_light_deferred_attenuation 1
    vec4  light_deferred_attenuation[8];
#else
    vec4  _dup_PBRAlphaF_light_deferred_attenuation[8];
#endif
};
layout(set = 1, binding = 1) uniform sampler2D diffuseMap;  // always in sRGB space
layout(set = 1, binding = 2) uniform sampler2D bumpMap;
layout(set = 1, binding = 3) uniform sampler2D emissiveMap;
layout(set = 1, binding = 4) uniform sampler2D specularMap; // PBR: Packed: Occlusion, Metal, Roughness

layout(push_constant) uniform PBRAlphaF_FragPC {
    layout(offset = 72) float waterSign;
    layout(offset = 76) float aya_preview_neutral_atmos;
};
#else
uniform sampler2D diffuseMap;  //always in sRGB space
uniform sampler2D bumpMap;
uniform sampler2D emissiveMap;
uniform sampler2D specularMap; // PBR: Packed: Occlusion, Metal, Roughness

uniform float metallicFactor;
uniform float roughnessFactor;
uniform vec3 emissiveColor;

uniform int sun_up_factor;
uniform vec3 sun_dir;
uniform vec3 moon_dir;
uniform int classic_mode;
uniform float waterSign;

#ifdef HAS_SUN_SHADOW
  uniform vec2 screen_res;
#endif

#ifdef HAS_ALPHA_MASK
uniform float minimum_alpha; // PBR alphaMode: MASK, See: mAlphaCutoff, setAlphaCutoff()
#endif

// Lights
// See: LLRender::syncLightState()
uniform vec4 light_position[8];
uniform vec3 light_direction[8]; // spot direction
uniform vec4 light_attenuation[8]; // linear, quadratic, is omni, unused, See: LLPipeline::setupHWLights() and syncLightState()
uniform vec3 light_diffuse[8];
uniform vec2 light_deferred_attenuation[8]; // light size and falloff
#endif

#ifdef LL_VULKAN_GLSL
layout(location = 0) out vec4 frag_color;

layout(location = 1) in vec3 vary_fragcoord;

layout(location = 0) in vec3 vary_position;

layout(location = 2) in vec2 base_color_texcoord;
layout(location = 3) in vec2 normal_texcoord;
layout(location = 4) in vec2 metallic_roughness_texcoord;
layout(location = 5) in vec2 emissive_texcoord;

layout(location = 6) in vec4 vertex_color;

layout(location = 9) in vec3 vary_normal;
layout(location = 7) in vec3 vary_tangent;
layout(location = 8) flat in float vary_sign;
#else
out vec4 frag_color;

in vec3 vary_fragcoord;

in vec3 vary_position;

in vec2 base_color_texcoord;
in vec2 normal_texcoord;
in vec2 metallic_roughness_texcoord;
in vec2 emissive_texcoord;

in vec4 vertex_color;

in vec3 vary_normal;
in vec3 vary_tangent;
flat in float vary_sign;
#endif

vec3 srgb_to_linear(vec3 c);
vec3 linear_to_srgb(vec3 c);

void calcAtmosphericVarsLinear(vec3 inPositionEye, vec3 norm, vec3 light_dir, out vec3 sunlit, out vec3 amblit, out vec3 atten, out vec3 additive);
vec4 applySkyAndWaterFog(vec3 pos, vec3 additive, vec3 atten, vec4 color);

void calcHalfVectors(vec3 lv, vec3 n, vec3 v, out vec3 h, out vec3 l, out float nh, out float nl, out float nv, out float vh, out float lightDist);
float calcLegacyDistanceAttenuation(float distance, float falloff);
float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen);
void sampleReflectionProbes(inout vec3 ambenv, inout vec3 glossenv,
        vec2 tc, vec3 pos, vec3 norm, float glossiness, bool transparent, vec3 amblit_linear);

void mirrorClip(vec3 pos);
void waterClip(vec3 pos, float waterSign); // waterSign 引数化

void calcDiffuseSpecular(vec3 baseColor, float metallic, inout vec3 diffuseColor, inout vec3 specularColor);

vec3 pbrBaseLight(vec3 diffuseColor,
                  vec3 specularColor,
                  float metallic,
                  vec3 pos,
                  vec3 norm,
                  float perceptualRoughness,
                  vec3 light_dir,
                  vec3 sunlit,
                  float scol,
                  vec3 radiance,
                  vec3 irradiance,
                  vec3 colorEmissive,
                  float ao,
                  vec3 additive,
                  vec3 atten);

vec3 pbrCalcPointLightOrSpotLight(vec3 diffuseColor, vec3 specularColor,
                    float perceptualRoughness,
                    float metallic,
                    vec3 n, // normal
                    vec3 p, // pixel position
                    vec3 v, // view vector (negative normalized pixel position)
                    vec3 lp, // light position
                    vec3 ld, // light direction (for spotlights)
                    vec3 lightColor,
                    float lightSize, float falloff, float is_pointlight, float ambiance);

void main()
{
    mirrorClip(vary_position);

    vec3 color = vec3(0,0,0);

#ifdef LL_VULKAN_GLSL
    // preview neutral atmospherics flag を atmosphericsFuncs (concatenated) の
    //   global へ転送 = calcAtmosphericVars が override 値 (sunlight=white/sun_up=1/density=0) を
    //   使用。 default 0 (= 主 scene) は元値ゆえ不変。
    _aya_preview_neutral_atmos = (aya_preview_neutral_atmos > 0.5) ? 1 : 0;
    int   _aya_eff_sun_up = (_aya_preview_neutral_atmos == 1) ? 1 : sun_up_factor;
    vec3  light_dir   = (_aya_eff_sun_up == 1) ? sun_dir : moon_dir;
#else
    vec3  light_dir   = (sun_up_factor == 1) ? sun_dir : moon_dir;
#endif
    vec3  pos         = vary_position;

    waterClip(pos, waterSign);

    vec4 basecolor = texture(diffuseMap, base_color_texcoord.xy).rgba;
    basecolor.rgb = srgb_to_linear(basecolor.rgb);
#ifdef HAS_ALPHA_MASK
    if (basecolor.a < minimum_alpha)
    {
        discard;
    }
#endif

    vec3 col = vertex_color.rgb * basecolor.rgb;

    vec3 vNt = texture(bumpMap, normal_texcoord.xy).xyz*2.0-1.0;
    float sign = vary_sign;
    vec3 vN = vary_normal;
    vec3 vT = vary_tangent.xyz;

    vec3 vB = sign * cross(vN, vT);
    vec3 norm = normalize( vNt.x * vT + vNt.y * vB + vNt.z * vN );

    norm *= gl_FrontFacing ? 1.0 : -1.0;

    float scol = 1.0;
    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;
    calcAtmosphericVarsLinear(pos.xyz, norm, light_dir, sunlit, amblit, additive, atten);
    if (classic_mode > 0)
        sunlit *= 1.35;
    vec3 sunlit_linear = sunlit;

    vec2 frag = vary_fragcoord.xy/vary_fragcoord.z*0.5+0.5;

#ifdef HAS_SUN_SHADOW
    scol = sampleDirectionalShadow(pos.xyz, norm.xyz, frag);
#endif

    vec3 orm = texture(specularMap, metallic_roughness_texcoord.xy).rgb; //orm is packed into "emissiveRect" to keep the data in linear color space

    float perceptualRoughness = orm.g * roughnessFactor;
    float metallic = orm.b * metallicFactor;
    float ao = orm.r;

    // emissiveColor is the emissive color factor from GLTF and is already in linear space
    vec3 colorEmissive = emissiveColor;
    // emissiveMap here is a vanilla RGB texture encoded as sRGB, manually convert to linear
    colorEmissive *= srgb_to_linear(texture(emissiveMap, emissive_texcoord.xy).rgb);

    // PBR IBL
    float gloss      = 1.0 - perceptualRoughness;
    vec3  irradiance = amblit;
    vec3  radiance  = vec3(0);
    sampleReflectionProbes(irradiance, radiance, vary_position.xy*0.5+0.5, pos.xyz, norm.xyz, gloss, true, amblit);

    vec3 diffuseColor;
    vec3 specularColor;
    calcDiffuseSpecular(col.rgb, metallic, diffuseColor, specularColor);

    vec3 v = -normalize(pos.xyz);

    scol = mix(scol, 1.0, 1.0 - basecolor.a * vertex_color.a);

    color = pbrBaseLight(diffuseColor, specularColor, metallic, v, norm.xyz, perceptualRoughness, light_dir, sunlit_linear, scol, radiance, irradiance, colorEmissive, ao, additive, atten);

    vec3 light = vec3(0);

    // Punctual lights
#define LIGHT_LOOP(i) light += pbrCalcPointLightOrSpotLight(diffuseColor, specularColor, perceptualRoughness, metallic, norm.xyz, pos.xyz, v, light_position[i].xyz, light_direction[i].xyz, light_diffuse[i].rgb, light_deferred_attenuation[i].x, light_deferred_attenuation[i].y, light_attenuation[i].z, light_attenuation[i].w);

    LIGHT_LOOP(1)
    LIGHT_LOOP(2)
    LIGHT_LOOP(3)
    LIGHT_LOOP(4)
    LIGHT_LOOP(5)
    LIGHT_LOOP(6)
    LIGHT_LOOP(7)

    color.rgb += light.rgb;

    color.rgb = applySkyAndWaterFog(pos.xyz, additive, atten, vec4(color, 1.0)).rgb;

    float a = basecolor.a*vertex_color.a;
    float final_scale = 1;
    if (classic_mode > 0)
        final_scale = 1.1;
    frag_color = max(vec4(color.rgb * final_scale,a), vec4(0));
}

#else

#ifdef LL_VULKAN_GLSL
layout(set = 1, binding = 0, std140) uniform PBRAlphaF_PerProgramBind
{
    vec3  emissiveColor;
    float _pbralphaF_hud_pad0;
#ifdef HAS_ALPHA_MASK
#ifndef _AYA_UM_minimum_alpha
#define _AYA_UM_minimum_alpha 1
    float minimum_alpha;
#else
    float _dup_PBRAlphaF_minimum_alpha;
#endif
    float _pbralphaF_hud_pad1;
    float _pbralphaF_hud_pad2;
    float _pbralphaF_hud_pad3;
#endif
};
layout(set = 1, binding = 1) uniform sampler2D diffuseMap;  // always in sRGB space
layout(set = 1, binding = 3) uniform sampler2D emissiveMap;
#else
uniform sampler2D diffuseMap;  //always in sRGB space
uniform sampler2D emissiveMap;

uniform vec3 emissiveColor;

#ifdef HAS_ALPHA_MASK
uniform float minimum_alpha; // PBR alphaMode: MASK, See: mAlphaCutoff, setAlphaCutoff()
#endif
#endif

#ifdef LL_VULKAN_GLSL
layout(location = 0) out vec4 frag_color;

layout(location = 0) in vec3 vary_position;

layout(location = 2) in vec2 base_color_texcoord;
layout(location = 5) in vec2 emissive_texcoord;

layout(location = 6) in vec4 vertex_color;
#else
out vec4 frag_color;

in vec3 vary_position;

in vec2 base_color_texcoord;
in vec2 emissive_texcoord;

in vec4 vertex_color;
#endif

vec3 srgb_to_linear(vec3 c);
vec3 linear_to_srgb(vec3 c);


void main()
{
    vec3 color = vec3(0,0,0);

    vec3  pos         = vary_position;

    vec4 basecolor = texture(diffuseMap, base_color_texcoord.xy).rgba;
    basecolor.rgb = srgb_to_linear(basecolor.rgb);
#ifdef HAS_ALPHA_MASK
    if (basecolor.a < minimum_alpha)
    {
        discard;
    }
#endif

    color = vertex_color.rgb * basecolor.rgb;

    // emissiveColor is the emissive color factor from GLTF and is already in linear space
    vec3 colorEmissive = emissiveColor;
    // emissiveMap here is a vanilla RGB texture encoded as sRGB, manually convert to linear
    colorEmissive *= srgb_to_linear(texture(emissiveMap, emissive_texcoord.xy).rgb);


    float a = basecolor.a*vertex_color.a;
    color += colorEmissive;

    color = linear_to_srgb(color);
    frag_color = max(vec4(color.rgb,a), vec4(0));
}

#endif
