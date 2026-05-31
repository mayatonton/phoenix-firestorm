/**
 * @file pbropaqueF.glsl
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

// deferred opaque implementation

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=1) uniform sampler2D diffuseMap;
#else
uniform sampler2D diffuseMap;  //always in sRGB space
#endif

uniform float metallicFactor;
uniform float roughnessFactor;
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=0, std140) uniform MaterialUBO {
    mat4  texture_matrix0;
    vec4  texture_base_color_transform[2];
    vec4  texture_emissive_transform[2];
    vec4  color;
    vec3  emissiveColor;
    float _pad_emissive;
};
#else
uniform vec3 emissiveColor;
#endif
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=6) uniform sampler2D bumpMap;
layout(set=1, binding=5) uniform sampler2D emissiveMap;
#else
uniform sampler2D bumpMap;
uniform sampler2D emissiveMap;
#endif
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=3) uniform sampler2D specularMap;
#else
uniform sampler2D specularMap; // Packed: Occlusion, Metal, Roughness
#endif

out vec4 frag_data[4];

in vec3 vary_position;
in vec4 vertex_color;
in vec3 vary_normal;
in vec3 vary_tangent;
flat in float vary_sign;

in vec2 base_color_texcoord;
in vec2 normal_texcoord;
in vec2 metallic_roughness_texcoord;
in vec2 emissive_texcoord;

#ifdef LL_VULKAN_GLSL
layout(set=0, binding=2, std140) uniform FrameAtmosphere {
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
#else
uniform float minimum_alpha; // PBR alphaMode: MASK, See: mAlphaCutoff, setAlphaCutoff()
#endif

// <FS:AYA r20 Phase C> per-draw skin marker: 1.0 if the parent LLViewerObject
// is on the SSS whitelist, 0.0 otherwise. Packed into frag_data[3].a so the
// screen-space SSS pass can gate its blur to skin pixels only.
uniform float aya_sss_skin_flag;
// </FS:AYA>

vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);

uniform vec4 clipPlane;
uniform float clipSign;

void mirrorClip(vec3 pos);
vec4 encodeNormal(vec3 n, float env, float gbuffer_flag);

#ifdef LL_VULKAN_GLSL
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
#else
uniform mat3 normal_matrix;
#endif

void main()
{
    mirrorClip(vary_position);

    vec4 basecolor = texture(diffuseMap, base_color_texcoord.xy).rgba;
    basecolor.rgb = srgb_to_linear(basecolor.rgb);

    basecolor *= vertex_color;

    if (basecolor.a < minimum_alpha)
    {
        discard;
    }

    vec3 col = basecolor.rgb;

    // from mikktspace.com
    vec3 vNt = texture(bumpMap, normal_texcoord.xy).xyz*2.0-1.0;
    float sign = vary_sign;
    vec3 vN = vary_normal;
    vec3 vT = vary_tangent.xyz;

    vec3 vB = sign * cross(vN, vT);
    vec3 tnorm = normalize( vNt.x * vT + vNt.y * vB + vNt.z * vN );

    // RGB = Occlusion, Roughness, Metal
    // default values, see LLViewerTexture::sDefaultPBRORMImagep
    //   occlusion 1.0
    //   roughness 0.0
    //   metal     0.0
    vec3 spec = texture(specularMap, metallic_roughness_texcoord.xy).rgb;

    spec.g *= roughnessFactor;
    spec.b *= metallicFactor;

    vec3 emissive = emissiveColor;
    emissive *= srgb_to_linear(texture(emissiveMap, emissive_texcoord.xy).rgb);

    tnorm *= gl_FrontFacing ? 1.0 : -1.0;

    //spec.rgb = vec3(1,1,0);
    //col = vec3(0,0,0);
    //emissive = vary_tangent.xyz*0.5+0.5;
    //emissive = vec3(sign*0.5+0.5);
    //emissive = vNt * 0.5 + 0.5;
    //emissive = tnorm*0.5+0.5;
    // See: C++: addDeferredAttachments(), GLSL: softenLightF
    frag_data[0] = max(vec4(col, 0.0), vec4(0));                                                   // Diffuse
    frag_data[1] = max(vec4(spec.rgb,0.0), vec4(0));                                    // PBR linear packed Occlusion, Roughness, Metal.
    frag_data[2] = encodeNormal(tnorm, 0, GBUFFER_FLAG_HAS_PBR); // normal, environment intensity, flags

#if defined(HAS_EMISSIVE)
    // <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> AY r20 packs the
    // per-draw SSS skin flag into gbuffer3.a alongside emissive.
    // <FS:AYAstorm r30 BD改善> r20 consolidation 後は Cinematic でも SSS dispatch
    // するため両 mode で skin flag を書く。非 SSS 経路では r20 が cvar OFF で
    // doSkinSSS 早期 return するため write 値は読まれない (実害ゼロ)。
    frag_data[3] = max(vec4(emissive, aya_sss_skin_flag), vec4(0));
    // </FS:AYAstorm>
    // </FS:AYA>
#endif
}

#else

// forward fullbright implementation for HUDs

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=1) uniform sampler2D diffuseMap;
#else
uniform sampler2D diffuseMap;  //always in sRGB space
#endif

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=0, std140) uniform MaterialUBO {
    mat4  texture_matrix0;
    vec4  texture_base_color_transform[2];
    vec4  texture_emissive_transform[2];
    vec4  color;
    vec3  emissiveColor;
    float _pad_emissive;
};
#else
uniform vec3 emissiveColor;
#endif
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=5) uniform sampler2D emissiveMap;
#else
uniform sampler2D emissiveMap;
#endif

out vec4 frag_color;

in vec3 vary_position;
in vec4 vertex_color;

in vec2 base_color_texcoord;
in vec2 emissive_texcoord;

#ifndef LL_VULKAN_GLSL
uniform float minimum_alpha; // PBR alphaMode: MASK, See: mAlphaCutoff, setAlphaCutoff()
#endif

vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);

void main()
{
    vec4 basecolor = texture(diffuseMap, base_color_texcoord.xy).rgba;
    // <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> BD multiplies
    // basecolor.a by vertex_color.a in the HUD path; AY dropped that to
    // keep HUD opacity decoupled from per-vertex tint alpha.
#if AYASTORM_CINEMATIC
    basecolor.a *= vertex_color.a;
#endif
    // </FS:AYA>
    if (basecolor.a < minimum_alpha)
    {
        discard;
    }

    vec3 col = vertex_color.rgb * srgb_to_linear(basecolor.rgb);

    vec3 emissive = emissiveColor;
    emissive *= srgb_to_linear(texture(emissiveMap, emissive_texcoord.xy).rgb);

    col += emissive;

    // HUDs are rendered after gamma correction, output in sRGB space
    frag_color.rgb = linear_to_srgb(col);
    frag_color.a = 0.0;
}

#endif

