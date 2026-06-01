/**
 * @file sunDiscF.glsl
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

vec3 srgb_to_linear(vec3 c);

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=1) uniform sampler2D diffuseMap;
#else
uniform sampler2D diffuseMap;
#endif
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=11) uniform sampler2D altDiffuseMap;
#else
uniform sampler2D altDiffuseMap;
#endif
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-6: sunDiscF non-opaque uniforms UBO wrap (Cluster F)
layout(set=3, binding=43, std140) uniform SunDiscFParamUBO_Legacy {
    float blend_factor;
};
#else
uniform float blend_factor; // interp factor between sunDisc A/B
#endif
#ifdef LL_VULKAN_GLSL
layout(location=0) in vec2 vary_texcoord0;
#else
in vec2 vary_texcoord0;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=20) in float sun_fade;
#else
in float sun_fade;
#endif

void main()
{
    vec4 sunDiscA = texture(diffuseMap, vary_texcoord0.xy);
    vec4 sunDiscB = texture(altDiffuseMap, vary_texcoord0.xy);
    vec4 c     = mix(sunDiscA, sunDiscB, blend_factor);


    // SL-9806 stars poke through
    //c.a *= sun_fade;

    frag_data[0] = vec4(0);
    frag_data[1] = vec4(0.0f);
    frag_data[2] = vec4(0.0, 1.0, 0.0, GBUFFER_FLAG_SKIP_ATMOS);
#if defined(HAS_EMISSIVE)
    frag_data[0] = vec4(0);
    // <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> AY r20 uses
    // gbuffer3.a as the SSS skin mask, so sun writes 0 to opt out.
    // <FS:AYAstorm r30 BD改善> Cinematic でも r20 SSS dispatch が走るため
    //   alpha leak で太陽縁に skin_mask 誤発火する。両 mode で 0。
    frag_data[3] = vec4(c.rgb, 0.0);
    // </FS:AYAstorm>
    // </FS:AYA>
#else
    frag_data[0] = c;
#endif
}

