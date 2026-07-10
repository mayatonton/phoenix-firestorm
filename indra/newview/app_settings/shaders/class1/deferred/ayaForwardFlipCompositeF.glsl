/**
 * @file ayaForwardFlipCompositeF.glsl
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * AYAstorm Viewer Source Code
 * Copyright (C) 2025-2026 Ishikawa AYA (github: mayatonton, mayatonton1994@gmail.com)
 *
 * このファイルは Ishikawa AYA が新規に作成した独自著作物である。
 * 著作権は Ishikawa AYA が保持し、パブリックドメインには置かない。
 * All rights reserved by the author except as licensed below.
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
 * $/LicenseInfo$
 *
 * 光の国のひとたちと共にわたしはここにいる　彩
 */

/*[EXTRA_CODE_HERE]*/

// <AYAstorm r41 forward-flip composite>
// Composite the post-deferred forward WATER plate (mForwardColor) onto
// mRT->screen with a VERTICAL (Y) FLIP. This is the missing Vulkan-specific
// orientation step: opaque reaches screen through the soften fullscreen
// resample (lands upright) whereas the forward water surface draws direct
// (lands upside down). The water plate was rendered NEG (same depth as
// opaque → river carving / occlusion preserved); flipping ONLY here at the
// composite leaves depth/raster untouched, so the carving stays intact and
// only the visible vertical position is corrected.
//
// Sample is taken at (x, 1 - y) to mirror the plate vertically. Coverage is
// derived from the plate contents (water surface writes opaque color where it
// is visible; the plate is cleared to 0 elsewhere) because the water frag
// alpha is spec*water_mask (glow), not coverage. C++ sets the blend func to
// (ONE, ONE_MINUS_SRC_ALPHA) so frag_color.a acts as the over-coverage:
//   dst.rgb = water.rgb + screen.rgb * (1 - cov)
//   dst.a   = cov       + screen.a   * (1 - cov)
// </AYAstorm r41 forward-flip composite>

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
    vec4 plate = texture(diffuseRect, vary_fragcoord);

    // Coverage = was this pixel written by the water surface? The plate is
    // cleared to (0,0,0,0); the water surface writes non-zero color (refraction
    // / reflection / fog blend is essentially never pure black) and/or glow
    // alpha where it is visible.
    float cov = step(0.0001, dot(plate.rgb, vec3(1.0)) + plate.a);

    frag_color = vec4(plate.rgb, cov);
}
