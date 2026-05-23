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
// Composite mAYAAlphaColor (premultiplied linear alpha BLEND plate) onto
// mRT->screen BEFORE generateLuminance / tonemap. This makes alpha BLEND
// surfaces participate in HDR exposure auto-calibration the same way the
// SL/LMB-on-HUD path does (where alpha was written directly to mRT->screen).
// Trivial passthrough: blend func is set on the C++ side to
//   glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA)
// so the fixed-function blender does the premultiplied over-blend:
//   dst.rgb = plate.rgb + dst.rgb * (1 - plate.a)
//   dst.a   = plate.a   + dst.a   * (1 - plate.a)
// </AYAstorm r30 P5 transparent-DoF C-(a) pre-tonemap composite>

out vec4 frag_color;

uniform sampler2D diffuseRect;

in vec2 vary_fragcoord;

void main()
{
    frag_color = texture(diffuseRect, vary_fragcoord);
}
