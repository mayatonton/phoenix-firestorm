/**
 * @file volumetricLightF.glsl
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

// AYAstorm r30 P3: imported from BlackDragon Viewer (NiranV Dean), 995a1354d8, 2026-04-19
// Source: https://github.com/NiranV/Black-Dragon-Viewer @ indra/newview/app_settings/shaders/class1/deferred/volumetricLightF.glsl
// License: LGPL-2.1-only (same as Second Life Viewer Source Code, no relicensing)

/*[EXTRA_CODE_HERE]*/

#ifdef LL_VULKAN_GLSL
layout(location = 0) out vec4 frag_color;
layout(location = 0) in vec2 vary_fragcoord;
#else
out vec4 frag_color;

in vec2 vary_fragcoord;
#endif

void main()
{
    // volumetric は additive overlay 化された (class3 参照)。
    //   class1 = 低 shaderLevel fallback stub ゆえ散乱計算なし → additive identity (vec4(0)) を出力。
    //   旧 diffuseRect 1:1 copy のままだと host blendFunc ONE/ONE 下で screen を二重加算する bug。
    frag_color = vec4(0.0);
}
