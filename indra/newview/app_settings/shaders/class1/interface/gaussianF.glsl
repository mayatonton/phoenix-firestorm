/**
 * @file gaussianF.glsl
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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
layout(location=0) out vec4 frag_color;
#else
out vec4 frag_color;
#endif

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=4) uniform sampler2D diffuseRect;
#else
uniform sampler2D diffuseRect;
#endif

#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-7 phase 2: gaussianF bare uniforms UBO wrap
// "Reflection Mip Shader" program 名は gReflectionMipProgram (reflectionmipF.glsl) と
// gGaussianProgram (本 file = gaussianF.glsl) で共有 (llviewershadermgr.cpp L3963/L3977)、
// 本 file の `resScale` `direction` 2 件 bare uniform が 0:1969 non-opaque uniforms
// outside a block error 源。`direction` `resScale` は本 file のみ宣言 (全 shader grep
// 確認済) で nameless block member global scope export 衝突なし。η-6 UBO wrap 範式
// 継承。GL `#else` path は byte-for-byte 不変 (charter §3 #1 担保)。
layout(set=3, binding=58, std140) uniform GaussianFParamUBO_Legacy {
    float resScale;
    vec2 direction;          // std140 vec2 alignment 8 で resScale との間に 4 byte padding 自動挿入
};
#else
uniform float resScale;

// texture direction, will be <1, 0> or <0, 1>
uniform vec2 direction;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=0) in vec2 vary_texcoord0;
#else
in vec2 vary_texcoord0;
#endif

// get linear depth value given a depth buffer sample d and znear and zfar values
float linearDepth(float d, float znear, float zfar);

void main()
{
    vec3 col = vec3(0,0,0);

    float w[9] = float[9]( 0.0002, 0.0060, 0.0606, 0.2417, 0.3829, 0.2417, 0.0606, 0.0060, 0.0002 );

    for (int i = 0; i < 9; ++i)
    {
        vec2 tc = vary_texcoord0 + (i-4)*direction*resScale;
        col += texture(diffuseRect, tc).rgb * w[i];
    }

    frag_color = max(vec4(col, 0.0), vec4(0));
}
