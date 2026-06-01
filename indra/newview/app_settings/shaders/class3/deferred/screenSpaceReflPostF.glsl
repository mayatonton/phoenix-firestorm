/**
 * @file class3/deferred/screenSpaceReflPostF.glsl
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

/*[EXTRA_CODE_HERE]*/

#ifdef LL_VULKAN_GLSL
layout(location=0) out vec4 frag_color;
#else
out vec4 frag_color;
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
uniform mat4 projection_matrix;
uniform mat4 inv_proj;
#endif
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-6: screenSpaceReflPostF non-opaque uniforms UBO wrap (Cluster E)
layout(set=3, binding=28, std140) uniform ScreenSpaceReflPostFParamUBO_Legacy {
    float zNear;
    float zFar;
};
#else
uniform float zNear;
uniform float zFar;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=1) in vec2 vary_fragcoord;
#else
in vec2 vary_fragcoord;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=20) in vec3 camera_ray;
#else
in vec3 camera_ray;
#endif

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=12) uniform sampler2D specularRect;
layout(set=1, binding=4) uniform sampler2D diffuseRect;
#else
uniform sampler2D specularRect;
uniform sampler2D diffuseRect;
#endif
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=1) uniform sampler2D diffuseMap;
#else
uniform sampler2D diffuseMap;
#endif

vec4 getNorm(vec2 screenpos);
float getDepth(vec2 pos_screen);
float linearDepth(float d, float znear, float zfar);
float linearDepth01(float d, float znear, float zfar);

vec4 getPositionWithDepth(vec2 pos_screen, float depth);
vec4 getPosition(vec2 pos_screen);

float random (vec2 uv);

float tapScreenSpaceReflection(int totalSamples, vec2 tc, vec3 viewPos, vec3 n, inout vec4 collectedColor, sampler2D source, float glossiness);

void main()
{
    vec2  tc = vary_fragcoord.xy;
    float depth = linearDepth01(getDepth(tc), zNear, zFar);
    vec4 norm = getNorm(tc); // need `norm.w` for GET_GBUFFER_FLAG()
    vec3 pos = getPositionWithDepth(tc, getDepth(tc)).xyz;
    vec4 spec    = texture(specularRect, tc);
    vec2 hitpixel;

    vec4 diffuse = texture(diffuseRect, tc);
    vec3 specCol = spec.rgb;

    vec4 fcol = texture(diffuseMap, tc);

    if (GET_GBUFFER_FLAG(norm.w, GBUFFER_FLAG_HAS_PBR))
    {
        vec3 orm = specCol.rgb;
        float perceptualRoughness = orm.g;
        float metallic = orm.b;
        vec3 f0 = vec3(0.04);
        vec3 baseColor = diffuse.rgb;

        vec3 diffuseColor = baseColor.rgb*(vec3(1.0)-f0);

        specCol = mix(f0, baseColor.rgb, metallic);
    }

    vec4 collectedColor = vec4(0);

    float w = tapScreenSpaceReflection(4, tc, pos, norm.xyz, collectedColor, diffuseMap, 0.f);

    collectedColor.rgb *= specCol.rgb;

    fcol += collectedColor * w;
    frag_color = max(fcol, vec4(0));
}
