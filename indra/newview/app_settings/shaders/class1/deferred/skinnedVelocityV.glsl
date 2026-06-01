/**
 * @file skinnedVelocityV.glsl
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
// Source: https://github.com/NiranV/Black-Dragon-Viewer @ indra/newview/app_settings/shaders/class1/deferred/skinnedVelocityV.glsl
// License: LGPL-2.1-only (same as Second Life Viewer Source Code, no relicensing)

// <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> BD only declares
// modelview_projection_matrix here; AY declares modelview_matrix +
// projection_matrix separately so it can compose with skinning. Both
// modes keep all three so the alternative main() path always links.
#ifdef LL_VULKAN_GLSL
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
#else
uniform mat4 modelview_projection_matrix;
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform mat4 last_modelview_matrix;
#endif
// </FS:AYA>

#ifdef LL_VULKAN_GLSL
layout(location=0) in vec3 position;
#else
in vec3 position;
#endif
#ifdef LL_VULKAN_GLSL
layout(location=10) in vec4 weight4;
#else
in vec4 weight4;
#endif

#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-1: PerDrawUBO guard wrap (B?-ζ §3.1 範式)
#ifndef PER_DRAW_UBO_DEFINED
#define PER_DRAW_UBO_DEFINED 1
layout(set=2, binding=0, std140) uniform PerDrawUBO {
    mat3x4 lastMatrixPalette[MAX_JOINTS_PER_MESH_OBJECT];
};
#endif
#else
uniform mat3x4 lastMatrixPalette[MAX_JOINTS_PER_MESH_OBJECT];
#endif

mat4 getObjectSkinnedTransform();
mat4 getLastObjectSkinnedTransform();

void writeVaryVelocity(vec4 pos, vec4 last_pos);

// <FS:AYA r30 Phase 3.8 Cinematic mount fix> Phase 3.8 step 2 overwrote
// objectSkinV.glsl with the BD baseline, which now provides
// getLastObjectSkinnedTransform(). Keeping a local definition here causes
// a GLSL C1013 duplicate-definition link error against objectSkinV's copy.
// Forward declaration only — link picks up the implementation from
// objectSkinV.glsl in both AY and Cinematic modes.
// </FS:AYA>

void main()
{
    vec4 pos = vec4(position.xyz, 1.0);

    // <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> AY r30 P2 A2.2
    // documented that BD's original current_clip skipped object skinning
    // (rigged meshes rasterized at T-pose into the velocity buffer).
    // Cinematic restores BD exactly for 1:1 parity (motion-blur velocity
    // for rigged meshes may regress to T-pose under Cinematic); AY mode
    // keeps the skinning-aware fix.
#if AYASTORM_CINEMATIC
    vec4 current_clip = modelview_projection_matrix * pos;
#else
    mat4 cur_mat = getObjectSkinnedTransform();
    vec4 current_clip = projection_matrix * (modelview_matrix * (cur_mat * pos));
#endif
    // </FS:AYA>

    mat4 last_mat = getLastObjectSkinnedTransform();
    vec4 last_clip = projection_matrix * (last_modelview_matrix * (last_mat * pos));

    gl_Position = current_clip;

    writeVaryVelocity(current_clip, last_clip);
}
