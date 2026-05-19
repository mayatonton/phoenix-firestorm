/**
 * @file skinnedVelocityAlphaV.glsl
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
// Source: https://github.com/NiranV/Black-Dragon-Viewer @ indra/newview/app_settings/shaders/class1/deferred/skinnedVelocityAlphaV.glsl
// License: LGPL-2.1-only (same as Second Life Viewer Source Code, no relicensing)

// <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> Keep both uniform
// sets so both main() paths link; pick at runtime via AYASTORM_CINEMATIC.
uniform mat4 modelview_projection_matrix;
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform mat4 last_modelview_matrix;
uniform mat4 texture_matrix0;
// </FS:AYA>

in vec3 position;
in vec4 weight4;
in vec4 diffuse_color;
in vec2 texcoord0;

out vec2 vary_texcoord0;
out vec4 vertex_color;

uniform mat3x4 lastMatrixPalette[MAX_JOINTS_PER_MESH_OBJECT];

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

    // <FS:AYA r30 Phase 3.8 Cinematic mount strategy C> Cinematic uses
    // BD original (T-pose velocity for rigged meshes), AY mode keeps
    // the P2 A2.2 skinning-aware fix.
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

    vary_texcoord0 = (texture_matrix0 * vec4(texcoord0, 0, 1)).xy;
    vertex_color = diffuse_color;
}
