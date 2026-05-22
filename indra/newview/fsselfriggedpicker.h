/**
 * @file fsselfriggedpicker.h
 * @brief GPU picker for the agent's own rigged attachments.
 *
 * AYAstorm r21.1 — replacement for the upstream LLPipeline::lineSegmentIntersectInWorld
 * path when picking self rigged attachments. The upstream worldray mis-aligns
 * vs the GPU-skinned mesh in some configurations (closeup zoom, alpha-discard
 * hair, idle-skin drift), so we re-draw self rigged attachments into a
 * dedicated GPU object-ID buffer each frame and resolve clicks by reading back
 * the byte quad at the mouse pixel. The buffer matches what is actually on
 * screen, so picks are pixel-perfect.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * AYAstorm Viewer Source Code
 * $/LicenseInfo$
 */

#ifndef FS_SELFRIGGEDPICKER_H
#define FS_SELFRIGGEDPICKER_H

class LLViewerObject;
class LLVOAvatar;

namespace FSSelfRiggedPicker
{
    // Find the self rigged attachment that the user is clicking on at
    // (mouse_x, mouse_y). Reads LLPipeline::mObjectIDBuffer (re-drawn each
    // frame with each self rigged attachment's LocalID packed into RGBA8)
    // and resolves the sampled LocalID back to the owning LLViewerObject.
    //
    // Gated on FSSelfRiggedPickerGPU; returns null if the cvar is off or the
    // buffer isn't allocated (e.g. cube snapshot path).
    //
    // out_gpu_authoritative is set to true whenever the GPU stage ran, even
    // when it returns null:
    //   - id != 0 + match           → returns the attachment, authoritative=true.
    //   - id != 0 + no match (race) → returns null, authoritative=true.
    //                                 Caller may force-to-body for rigged picks.
    //   - id == 0                    → returns null, authoritative=true.
    //                                 Caller may force-to-body for rigged picks.
    //   - cvar off / RT not ready    → returns null, authoritative=false.
    LLViewerObject* findClosestAttachment(S32 mouse_x, S32 mouse_y,
                                          bool& out_gpu_authoritative);

    // AYAstorm r28: same GPU readback path, scoped to one non-self avatar that
    // was armed by hover. Returns null without authority if the target avatar's
    // object-ID buffer is not ready.
    LLViewerObject* findClosestAttachmentForAvatar(S32 mouse_x, S32 mouse_y,
                                                   LLVOAvatar* avatar,
                                                   bool& out_gpu_authoritative);
}

#endif // FS_SELFRIGGEDPICKER_H
