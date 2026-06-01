/**
 * @file class1\lighting\sumLightsSpecularV.glsl
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



float calcDirectionalLightSpecular(inout vec4 specular, vec3 view, vec3 n, vec3 l, vec3 lightCol, float da);
vec3 atmosAmbient();
vec3 atmosAffectDirectionalLight(float lightIntensity);
vec3 atmosGetDiffuseSunlightColor();
vec3 scaleDownLight(vec3 light);

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
uniform vec4 light_position[8];
uniform vec3 light_diffuse[8];
#endif

vec4 sumLightsSpecular(vec3 pos, vec3 norm, vec4 color, inout vec4 specularColor)
{
    vec4 col = vec4(0,0,0, color.a);

    vec3 view = normalize(pos);

    /// collect all the specular values from each calcXXXLightSpecular() function
    vec4 specularSum = vec4(0.0);

    col.rgb += light_diffuse[1].rgb * calcDirectionalLightSpecular(specularColor, view, norm, light_position[1].xyz,light_diffuse[1].rgb, 1.0);
    col.rgb = scaleDownLight(col.rgb);
    col.rgb += atmosAmbient();
    col.rgb += atmosAffectDirectionalLight(calcDirectionalLightSpecular(specularSum, view, norm, light_position[0].xyz, atmosGetDiffuseSunlightColor(), 1.0));

    col.rgb = min(col.rgb * color.rgb, 1.0);
    specularColor.rgb = min(specularColor.rgb * specularSum.rgb, 1.0);

    col.rgb += specularColor.rgb;

    return col;
}
