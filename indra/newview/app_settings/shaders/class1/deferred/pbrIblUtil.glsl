/**
 * @file class1/deferred/pbrIblUtil.glsl
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
#ifndef DECL_BRDF_LUT
#define DECL_BRDF_LUT
layout(set = 1, binding = 29) uniform sampler2D brdfLut;
#endif // DECL_BRDF_LUT
#else
#ifndef DECL_BRDF_LUT
#define DECL_BRDF_LUT
uniform sampler2D brdfLut;
#endif // DECL_BRDF_LUT
#endif

vec3 srgb_to_linear(vec3 cs);
vec3 linear_to_srgb(vec3 cs);
void pbrPunctual(vec3 diffuseColor, vec3 specularColor,
                    float perceptualRoughness,
                    float metallic,
                    vec3 n,
                    vec3 v,
                    vec3 l,
                    out float nl,
                    out vec3 diff,
                    out vec3 spec);

vec2 BRDF(float NoV, float roughness)
{
    return texture(brdfLut, vec2(NoV, roughness)).rg;
}

// set colorDiffuse and colorSpec to the results of GLTF PBR style IBL
void pbrIbl(vec3 diffuseColor,
            vec3 specularColor,
            vec3 radiance, // radiance map sample
            vec3 irradiance, // irradiance map sample
            float ao,       // ambient occlusion factor
            float nv,       // normal dot view vector
            float perceptualRough,
            out vec3 diffuseOut,
            out vec3 specularOut)
{
    // retrieve a scale and bias to F0. See [1], Figure 3
    vec2 brdf = BRDF(clamp(nv, 0, 1), 1.0-perceptualRough);
    vec3 diffuseLight = irradiance;
    vec3 specularLight = radiance;

    vec3 diffuse = diffuseLight * diffuseColor;
    vec3 specular = specularLight * (specularColor * brdf.x + brdf.y);

    diffuseOut = diffuse * ao;
    specularOut = specular * ao;
}

vec3 pbrBaseLight(vec3 diffuseColor, vec3 specularColor, float metallic, vec3 v, vec3 norm, float perceptualRoughness, vec3 light_dir, vec3 sunlit, float scol, vec3 radiance, vec3 irradiance, vec3 colorEmissive, float ao, vec3 additive, vec3 atten)
{
    vec3 color = vec3(0);

    float NdotV = clamp(abs(dot(norm, v)), 0.001, 1.0);
    vec3 iblDiff = vec3(0);
    vec3 iblSpec = vec3(0);
    pbrIbl(diffuseColor, specularColor, radiance, irradiance, ao, NdotV, perceptualRoughness, iblDiff, iblSpec);

    color += iblDiff;

    // For classic mode, we use a special version of pbrPunctual that basically gives us a deconstructed form of the lighting.
    float nl = 0;
    vec3 diffPunc = vec3(0);
    vec3 specPunc = vec3(0);
    pbrPunctual(diffuseColor, specularColor, perceptualRoughness, metallic, norm, v, normalize(light_dir), nl, diffPunc, specPunc);

    // Depending on the sky, we combine these differently.
    if (classic_mode > 0)
    {
        irradiance.rgb = srgb_to_linear(irradiance * 0.9); // BINGO

        // Reconstruct the diffuse lighting that we do for blinn-phong materials here.
        // A special note about why we do some really janky stuff for classic mode.
        // Since adding classic mode, we've moved the lambertian diffuse multiply out from pbrPunctual and instead handle it in the different light type calcs.
        // This will never be 100% correct, but at the very least we can make it look mostly correct with legacy skies and classic mode.

        float da = pow(nl, 1.2);

        vec3 sun_contrib = vec3(min(da, scol));

        // Multiply by PI to account for lambertian diffuse colors.  Otherwise things will be too dark when lit by the sun on legacy skies.
        sun_contrib = srgb_to_linear(linear_to_srgb(sun_contrib) * sunlit * 0.7) * M_PI;

        // Manually recombine everything here.  We have to separate the shading to ensure that lighting is able to more closely match blinn-phong.
        vec3 finalAmbient = irradiance.rgb * diffuseColor.rgb; // BINGO
        vec3 finalSun = clamp(sun_contrib * ((diffPunc.rgb + specPunc.rgb) * scol), vec3(0), vec3(10)); // QUESTIONABLE BINGO?
        color.rgb = srgb_to_linear(linear_to_srgb(finalAmbient) + (linear_to_srgb(finalSun) * 1.1));
        //color.rgb = sun_contrib * diffuseColor.rgb;
    }
    else
    {
        color += clamp(nl * (diffPunc + specPunc), vec3(0), vec3(10)) * sunlit * 3.0 * scol;
    }

    color.rgb += iblSpec.rgb;

    color += colorEmissive;

    return color;
}
