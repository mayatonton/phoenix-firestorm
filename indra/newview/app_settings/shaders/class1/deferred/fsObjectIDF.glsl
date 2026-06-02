/**
 * @file fsObjectIDF.glsl
 * @brief AYAstorm r21.1 — GPU self-rigged picker, fragment stage.
 *
 * Writes the attachment's LocalID packed across four 8-bit channels into
 * pipeline.mObjectIDBuffer at every pixel the rigged mesh covers. The
 * picker reads the byte quad at the mouse pixel, recombines it into a
 * U32 LocalID, and resolves it by walking gAgentAvatarp's attachment tree
 * (see fsselfriggedpicker.cpp). Because both this pass and the visible
 * scene share GPU skinning + depth, there is no CPU/GPU drift, and
 * alpha-discarded triangles never reach the ID buffer.
 *
 * The host packs the LocalID like this:
 *   r = ((id >>  0) & 0xff) / 255.0
 *   g = ((id >>  8) & 0xff) / 255.0
 *   b = ((id >> 16) & 0xff) / 255.0
 *   a = ((id >> 24) & 0xff) / 255.0
 * and the picker reads back the four bytes and reassembles in the same
 * little-endian order. The clear value (0,0,0,0) decodes to LocalID 0,
 * which we reserve as "no self rigged attachment here".
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * AYAstorm Viewer Source Code
 * $/LicenseInfo$
 */

out vec4 frag_color;

#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2a: PerProgramUBO_FsObjectIdF (η-3 §3.2 PerDrawUBO 派生範式)
#ifndef PER_PROGRAM_UBO_FS_OBJECT_ID_F_DEFINED
#define PER_PROGRAM_UBO_FS_OBJECT_ID_F_DEFINED 1
layout(set=2, binding=13, std140) uniform PerProgramUBO_FsObjectIdF {
    vec4 object_id_packed;
};
#endif
#else
uniform vec4 object_id_packed;
#endif

void main()
{
    frag_color = object_id_packed;
}
