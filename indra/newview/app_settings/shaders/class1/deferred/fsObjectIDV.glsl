/**
 * @file fsObjectIDV.glsl
 * @brief AYAstorm r21.1 — GPU self-rigged picker, vertex stage.
 *
 * Re-skins the rigged attachment exactly as the visible draw does, using
 * objectSkinV.glsl's matrixPalette path. No varyings, no normals — we only
 * need clip-space position so the fragment stage can write the object ID
 * at the same pixels the real attachment covers. Depth is shared with
 * mRT->deferredScreen, so the depth test naturally hides points the real
 * scene has already occluded.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * AYAstorm Viewer Source Code
 * $/LicenseInfo$
 */

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
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
#endif

in vec3 position;

mat4 getObjectSkinnedTransform();

void main()
{
    mat4 mat = getObjectSkinnedTransform();
    mat = modelview_matrix * mat;
    vec3 pos = (mat * vec4(position.xyz, 1.0)).xyz;
    gl_Position = projection_matrix * vec4(pos, 1.0);
}
