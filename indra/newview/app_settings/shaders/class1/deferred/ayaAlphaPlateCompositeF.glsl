/**
 * @file ayaAlphaPlateCompositeF.glsl
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

// <AYAstorm r30 P5 transparent-DoF C-(a) pre-tonemap composite>
// <AYAstorm r41 forward-flip composite>
// Composite mAYAAlphaColor (premultiplied linear alpha BLEND plate) onto
// mRT->screen BEFORE generateLuminance / tonemap. This makes alpha BLEND
// surfaces participate in HDR exposure auto-calibration the same way the
// SL/LMB-on-HUD path does (where alpha was written directly to mRT->screen).
// On the Vulkan path the sample is taken with a VERTICAL (Y) FLIP: POST_WATER
// forward alpha is drawn NEG (negative-height viewport, Vulkan-only) so it
// lands upside-down vs the soften-resampled opaque scene — flipping ONLY here
// at the composite corrects the visible orientation while depth/raster stay
// untouched (alpha still depth-tests against opaque z as before). The GL path
// draws upright (no viewport flip) and samples straight = GL-1:1 preserved.
// plate.a is the true premultiplied
// coverage (forwardRender alpha factor = ONE, 1-Sa when redirected), so the
// C++-side blend func (pipeline.cpp renderFinalize)
//   glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA)
// over-composites RGB and suppresses glow exactly as the GL direct-to-screen
// path did:
//   dst.rgb = plate.rgb + dst.rgb * (1 - plate.a)
//   dst.a   =             dst.a   * (1 - plate.a)   // glow suppression (BF_ZERO)
// </AYAstorm r41 forward-flip composite>
// </AYAstorm r30 P5 transparent-DoF C-(a) pre-tonemap composite>

#ifdef LL_VULKAN_GLSL
layout(location = 0) out vec4 frag_color;

layout(set = 1, binding = 1) uniform sampler2D diffuseRect;

layout(location = 0) in vec2 vary_fragcoord;
#else
out vec4 frag_color;

uniform sampler2D diffuseRect;

in vec2 vary_fragcoord;
#endif

void main()
{
#ifdef LL_VULKAN_GLSL
    // Vulkan only: POST_WATER forward alpha was rasterized with a negative-
    // height viewport (LLVKLoader::setupViewportAndScissor, applied solely on
    // the shouldUseVulkanRender() path in llvertexbuffer.cpp) so the plate is
    // vertically mirrored vs the soften-resampled opaque screen. Sample flipped
    // to restore upright. The GL path applies NO such viewport flip (forward
    // alpha lands upright), so it samples straight below — preserving GL-1:1.
    vec2 tc = vec2(vary_fragcoord.x, 1.0 - vary_fragcoord.y);
#else
    vec2 tc = vary_fragcoord;
#endif
    frag_color = texture(diffuseRect, tc);
}
