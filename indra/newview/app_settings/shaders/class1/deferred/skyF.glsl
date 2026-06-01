/**
 * @file class1/deferred/skyF.glsl
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

// Inputs
in vec3 vary_HazeColor;
in float vary_LightNormPosDot;

#ifdef HAS_HDRI
in vec4 vary_position;
in vec3 vary_rel_pos;
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
uniform float sky_hdr_scale;
#endif
uniform float hdri_split_screen;
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
uniform mat3 env_mat;
#endif
#ifdef LL_VULKAN_GLSL
layout(set=0, binding=6) uniform sampler2D environmentMap;
#else
uniform sampler2D environmentMap;
#endif
#endif

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=59) uniform sampler2D rainbow_map;
layout(set=1, binding=58) uniform sampler2D halo_map;
#else
uniform sampler2D rainbow_map;
uniform sampler2D halo_map;
#endif

uniform float moisture_level;
uniform float droplet_radius;
uniform float ice_level;

out vec4 frag_data[4];

vec3 srgb_to_linear(vec3 c);
vec3 linear_to_srgb(vec3 c);

#define PI 3.14159265

/////////////////////////////////////////////////////////////////////////
// The fragment shader for the sky
/////////////////////////////////////////////////////////////////////////


vec3 rainbow(float d)
{
    // 'Interesting' values of d are -0.75 .. -0.825, i.e. when view vec nearly opposite of sun vec
    // Rainbox tex is mapped with REPEAT, so -.75 as tex coord is same as 0.25.  -0.825 -> 0.175. etc.
    // SL-13629
    // Unfortunately the texture is inverted, so we need to invert the y coord, but keep the 'interesting'
    // part within the same 0.175..0.250 range, i.e. d = (1 - d) - 1.575
    d         = clamp(-0.575 - d, 0.0, 1.0);

    // With the colors in the lower 1/4 of the texture, inverting the coords leaves most of it inaccessible.
    // So, we can stretch the texcoord above the colors (ie > 0.25) to fill the entire remaining coordinate
    // space. This improves gradation, reduces banding within the rainbow interior. (1-0.25) / (0.425/0.25) = 4.2857
    float interior_coord = max(0.0, d - 0.25) * 4.2857;
    d = clamp(d, 0.0, 0.25) + interior_coord;

    float rad = (droplet_radius - 5.0f) / 1024.0f;
    return pow(texture(rainbow_map, vec2(rad+0.5, d)).rgb, vec3(1.8)) * moisture_level;
}

vec3 halo22(float d)
{
    d       = clamp(d, 0.1, 1.0);
    float v = sqrt(clamp(1 - (d * d), 0, 1));
    return texture(halo_map, vec2(0, v)).rgb * ice_level;
}

void main()
{
    vec3 color;
#ifdef HAS_HDRI
    vec3 frag_coord = vary_position.xyz/vary_position.w;
    if (-frag_coord.x > ((1.0-hdri_split_screen)*2.0-1.0))
    {
        vec3 pos = normalize(vary_rel_pos);
        pos = env_mat * pos;
        vec2 texCoord = vec2(atan(pos.z, pos.x) + PI, acos(pos.y)) / vec2(2.0 * PI, PI);
        color = textureLod(environmentMap, texCoord.xy, 0).rgb * sky_hdr_scale;
        color = min(color, vec3(8192*8192*16)); // stupidly large value arrived at by binary search -- avoids framebuffer corruption from some HDRIs

        frag_data[2] = vec4(0.0,0.0,0.0,GBUFFER_FLAG_HAS_HDRI);
    }
    else
#endif
    {
        // Potential Fill-rate optimization.  Add cloud calculation
        // back in and output alpha of 0 (so that alpha culling kills
        // the fragment) if the sky wouldn't show up because the clouds
        // are fully opaque.

        color = vary_HazeColor;

        float  rel_pos_lightnorm = vary_LightNormPosDot;
        float optic_d = rel_pos_lightnorm;
        vec3  halo_22 = halo22(optic_d);
        color.rgb += rainbow(optic_d);
        color.rgb += halo_22;
        color.rgb *= 2.;
        color.rgb = clamp(color.rgb, vec3(0), vec3(5));

        frag_data[2] = vec4(0.0,0.0,0.0,GBUFFER_FLAG_SKIP_ATMOS);
    }

    frag_data[1] = vec4(0);

#if defined(HAS_EMISSIVE)
    frag_data[0] = vec4(0);
    // <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> AY r20 uses
    // gbuffer3.a as the SSS skin mask, so sky writes 0 to opt out of
    // the screen-space SSS blur.
    // <FS:AYAstorm r30 BD改善> Cinematic mode でも r20 SSS dispatch が
    //   走るため (r20 consolidation で mode 1/2 共通)、alpha=1 leak で
    //   horizon / 半透明 SIM 装飾物に skin_mask 誤発火する。alpha は
    //   SSS skin_mask に専有し、cinematic でも 0 を書く。
    frag_data[3] = vec4(color.rgb, 0.0);
    // </FS:AYAstorm>
    // </FS:AYA>
#else
    frag_data[0] = vec4(color.rgb, 1.0);
#endif
}

