/**
 * @file velocityFuncV.glsl
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

// AYAstorm r30 P2: imported from BlackDragon Viewer (NiranV Dean), 995a1354d8, 2026-04-19
// Source: https://github.com/NiranV/Black-Dragon-Viewer @ indra/newview/app_settings/shaders/class1/deferred/velocityFuncV.glsl
// License: LGPL-2.1-only (same as Second Life Viewer Source Code, no relicensing)

#ifdef LL_VULKAN_GLSL
layout(location = 0) out vec4 vary_cur_clip;
layout(location = 1) out vec4 vary_last_clip;
#else
out vec4 vary_cur_clip;
out vec4 vary_last_clip;
#endif

void writeVaryVelocity(vec4 pos, vec4 last_pos)
{
    vary_cur_clip = pos;
    vary_last_clip = last_pos;
}
