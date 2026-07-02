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
#ifndef PER_FRAME_MATRIX_UBO_DEFINED
#define PER_FRAME_MATRIX_UBO_DEFINED 1
layout(set = 0, binding = 0, std140) uniform PerFrameMatrixUBO
{
    mat4 projection_matrix;
    mat4 inverse_projection_matrix;
    mat4 identity_matrix;
    mat4 last_modelview_matrix;
};
#endif // PER_FRAME_MATRIX_UBO_DEFINED
layout(push_constant) uniform ModelviewPushConstant
{
    mat4 modelview_matrix;
};
#else
uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
#endif

#ifdef LL_VULKAN_GLSL
layout(location = 0) in vec3 position;
#else
in vec3 position;
#endif

mat4 getObjectSkinnedTransform();

void main()
{
    mat4 mat = getObjectSkinnedTransform();
    mat = modelview_matrix * mat;
    vec3 pos = (mat * vec4(position.xyz, 1.0)).xyz;
    gl_Position = projection_matrix * vec4(pos, 1.0);
}
