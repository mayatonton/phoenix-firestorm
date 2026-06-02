/**
 * @file WLCloudsV.glsl
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
uniform mat4 modelview_projection_matrix;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=0) in vec3 position;
#else
in vec3 position;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=2) in vec2 texcoord0;
#else
in vec2 texcoord0;
#endif

//////////////////////////////////////////////////////////////////////////
// The vertex shader for creating the atmospheric sky
///////////////////////////////////////////////////////////////////////////////

// Output parameters
#ifdef LL_VULKAN_GLSL
layout(location=26) out vec3 vary_CloudColorSun;
#else
out vec3 vary_CloudColorSun;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=27) out vec3 vary_CloudColorAmbient;
#else
out vec3 vary_CloudColorAmbient;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=22) out float vary_CloudDensity;
#else
out float vary_CloudDensity;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=0) out vec2 vary_texcoord0;
#else
out vec2 vary_texcoord0;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=5) out vec2 vary_texcoord1;
#else
out vec2 vary_texcoord1;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=16) out vec2 vary_texcoord2;
#else
out vec2 vary_texcoord2;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=17) out vec2 vary_texcoord3;
#else
out vec2 vary_texcoord3;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=23) out float altitude_blend_factor;
#else
out float altitude_blend_factor;
#endif

// Inputs
#ifdef LL_VULKAN_GLSL
layout(set=3, binding=3, std140) uniform CloudsVParamUBO_Legacy {
    vec3  camPosLocal;
    float cloud_scale;
    vec3  cloud_color;
    float _pad_clouds_v_legacy_0;
};
#else
uniform vec3 camPosLocal;
#endif

#ifndef LL_VULKAN_GLSL
uniform vec3 lightnorm;
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
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-14: FrameAtmosphere_Skybox 削除 (Path G)
//   skyV.glsl と同様。Lighting (helpersV/atmosphericsFuncs) + AtmoExtraUBO_Legacy
//   (atmosphericsFuncs) で全 member 解決、binding=2 nameless block 名衝突を解消。
#else
uniform vec3 sunlight_color;
uniform vec3 moonlight_color;
uniform int sun_up_factor;
uniform vec3 ambient_color;
uniform vec3 blue_horizon;
uniform vec3 blue_density;
uniform float haze_density;

uniform float density_multiplier;
uniform float max_y;

uniform vec3 glow;
#endif
#ifndef LL_VULKAN_GLSL
uniform float haze_horizon;
#endif

#ifndef LL_VULKAN_GLSL
uniform float cloud_shadow;
#endif

#ifndef LL_VULKAN_GLSL
uniform float sun_moon_glow_factor;
#endif

#ifndef LL_VULKAN_GLSL
uniform vec3 cloud_color;
#endif

#ifndef LL_VULKAN_GLSL
uniform float cloud_scale;
#endif

// NOTE: Keep these in sync!
//       indra\newview\app_settings\shaders\class1\deferred\skyV.glsl
//       indra\newview\app_settings\shaders\class1\deferred\cloudsV.glsl
//       indra\newview\app-settings\shaders\class2\windlight\cloudsV.glsl
//       indra\newview\lllegacyatmospherics.cpp
//       indra\newview\llsettingsvo.cpp
void main()
{
    // World / view / projection
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);

    // Texture coords
    // SL-13084 EEP added support for custom cloud textures -- flip them horizontally to match the preview of Clouds > Cloud Scroll
    vary_texcoord0 = vec2(-texcoord0.x, texcoord0.y);  // See: LLSettingsVOSky::applySpecial

    vary_texcoord0.xy -= 0.5;
    vary_texcoord0.xy /= cloud_scale;
    vary_texcoord0.xy += 0.5;

    vary_texcoord1 = vary_texcoord0;
    vary_texcoord1.x += lightnorm.x * 0.0125;
    vary_texcoord1.y += lightnorm.z * 0.0125;

    vary_texcoord2 = vary_texcoord0 * 16.;
    vary_texcoord3 = vary_texcoord1 * 16.;

    // Get relative position
    vec3 rel_pos = position.xyz - camPosLocal.xyz + vec3(0, 50, 0);

    altitude_blend_factor = clamp((rel_pos.y + 512.0) / max_y, 0.0, 1.0);

    // Set altitude
    if (rel_pos.y > 0)
    {
        rel_pos *= (max_y / rel_pos.y);
    }
    if (rel_pos.y < 0)
    {
        altitude_blend_factor = 0; // SL-11589 Fix clouds drooping below horizon
        rel_pos *= (-32000. / rel_pos.y);
    }

    // Can normalize then
    vec3  rel_pos_norm = normalize(rel_pos);
    float rel_pos_len  = length(rel_pos);

    // Initialize temp variables
    vec3 sunlight = sunlight_color;
    vec3 light_atten;

    // Sunlight attenuation effect (hue and brightness) due to atmosphere
    // this is used later for sunlight modulation at various altitudes
    light_atten = (blue_density + vec3(haze_density * 0.25)) * (density_multiplier * max_y);

    // Calculate relative weights
    vec3 combined_haze = abs(blue_density) + vec3(abs(haze_density));
    vec3 blue_weight   = blue_density / combined_haze;
    vec3 haze_weight   = haze_density / combined_haze;

    // Compute sunlight from rel_pos & lightnorm (for long rays like sky)
    float off_axis = 1.0 / max(1e-6, max(0., rel_pos_norm.y) + lightnorm.y);
    sunlight *= exp(-light_atten * off_axis);

    // Distance
    float density_dist = rel_pos_len * density_multiplier;

    // Transparency (-> combined_haze)
    // ATI Bugfix -- can't store combined_haze*density_dist in a variable because the ati
    // compiler gets confused.
    combined_haze = exp(-combined_haze * density_dist);

    // Compute haze glow
    float haze_glow = 1.0 - dot(rel_pos_norm, lightnorm.xyz);
    // haze_glow is 0 at the sun and increases away from sun
    haze_glow = max(haze_glow, .001);
        // Set a minimum "angle" (smaller glow.y allows tighter, brighter hotspot)
    haze_glow *= glow.x;
        // Higher glow.x gives dimmer glow (because next step is 1 / "angle")
    haze_glow = pow(haze_glow, glow.z);
        // glow.z should be negative, so we're doing a sort of (1 / "angle") function

    haze_glow *= sun_moon_glow_factor;

    // Add "minimum anti-solar illumination"
    // For sun, add to glow.  For moon, remove glow entirely. SL-13768
    haze_glow = (sun_moon_glow_factor < 1.0) ? 0.0 : (haze_glow + 0.25);

    // Increase ambient when there are more clouds
    vec3 tmpAmbient = ambient_color;
    tmpAmbient += (1. - tmpAmbient) * cloud_shadow * 0.5;

    // Dim sunlight by cloud shadow percentage
    sunlight *= (1. - cloud_shadow);

    // Haze color below cloud
    vec3 additiveColorBelowCloud =
        (blue_horizon * blue_weight * (sunlight + tmpAmbient) + (haze_horizon * haze_weight) * (sunlight * haze_glow + tmpAmbient));

    // CLOUDS
    sunlight = sunlight_color;
    off_axis = 1.0 / max(1e-6, lightnorm.y * 2.);
    sunlight *= exp(-light_atten * off_axis);

    // Cloud color out
    vary_CloudColorSun     = (sunlight * haze_glow) * cloud_color;
    vary_CloudColorAmbient = tmpAmbient * cloud_color;

    // Attenuate cloud color by atmosphere
    combined_haze = sqrt(combined_haze);  // less atmos opacity (more transparency) below clouds
    vary_CloudColorSun *= combined_haze;
    vary_CloudColorAmbient *= combined_haze;
    vec3 oHazeColorBelowCloud = additiveColorBelowCloud * (1. - combined_haze);

    // Make a nice cloud density based on the cloud_shadow value that was passed in.
    vary_CloudDensity = 2. * (cloud_shadow - 0.25);

    // Combine these to minimize register use
    vary_CloudColorAmbient += oHazeColorBelowCloud;

    // needs this to compile on mac
    //vary_AtmosAttenuation = vec3(0.0,0.0,0.0);

    // END CLOUDS
}
