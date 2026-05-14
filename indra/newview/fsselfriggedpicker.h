/**
 * @file fsselfriggedpicker.h
 * @brief Hybrid picker for the agent's own rigged attachments.
 *
 * AYAstorm r21.1 — replacement for the upstream LLPipeline::lineSegmentIntersectInWorld
 * path when picking self rigged attachments. The upstream path mis-aligns the world
 * ray vs the GPU-skinned mesh in some configurations, so we have a GPU object-ID
 * buffer path (Stage -1) plus pierce-only CPU fallbacks:
 *   - stage -1 (GPU): read the byte quad at the mouse pixel from
 *     LLPipeline::mObjectIDBuffer. Authoritative when id != 0 (exact match
 *     with the visible scene). See findClosestAttachment() docstring for
 *     the hybrid model.
 *   - stage 0 (depth-assist): pierce-only test on the SHORT ray from the
 *     camera to the unprojected depth-buffer sample. Near-miss recovery is
 *     suppressed — only triangles physically pierced by the ray win, so the
 *     aim is needle-precise (no body-region drift). Used when the GPU
 *     stage produced id=0 (sky, alpha-discarded hair, closeup LOD edge case).
 *   - stage 1 (ray pierce-only): full-length camera ray, same pierce-only
 *     rule. Two-sided Möller-Trumbore with a 2% barycentric epsilon. Catches
 *     attachments whose depth wasn't sampled in stage 0 either.
 *   - (no stage 2): the historical edge near-miss recovery is removed in
 *     r21.1 — the GPU ID buffer is the precise path, and widening the legacy
 *     hit zone by tolerance metres made the aim feel sloppy.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * AYAstorm Viewer Source Code
 * $/LicenseInfo$
 */

#ifndef FS_SELFRIGGEDPICKER_H
#define FS_SELFRIGGEDPICKER_H

class LLViewerObject;

namespace FSSelfRiggedPicker
{
    // Find the self rigged attachment that the user is clicking on at
    // (mouse_x, mouse_y). Stage order:
    //   -1. GPU object-ID buffer (gated on FSSelfRiggedPickerGPU): read the
    //       byte quad at the mouse pixel from LLPipeline::mObjectIDBuffer
    //       (re-drawn each frame with each self rigged attachment's LocalID
    //       packed into RGBA8). Hybrid authority (see fsselfriggedpicker.cpp
    //       Stage -1 comment for the closeup-zoom rationale):
    //         - id != 0 + match  → return it (legacy skipped).
    //         - id != 0 + no match (race) → out_gpu_authoritative=true,
    //           return nullptr. Caller may force-to-body.
    //         - id == 0          → fall through to legacy stages.
    //   0. Depth-assist (gated on FSSelfRiggedPickerDepthAssist): pierce-only
    //      test on the short ray from the camera to the unprojected depth-
    //      buffer sample. Closest-to-camera pierce wins. Near-miss recovery
    //      is suppressed in r21.1 (needle-aim).
    //   1. If stage 0 yields no hit (sky / discarded pixel / depth-assist
    //      off), fall back to pierce-only on the full-length camera ray.
    //      Closest-to-camera pierce wins.
    // Returns nullptr if no stage finds a candidate.
    //
    // out_gpu_authoritative: set to true only in the GPU-saw-something-but-
    // table-doesn't-know-what race case. When true + return null, the caller
    // may force the selection to the self avatar (body). For id==0 the flag
    // stays false so legacy result is trusted.
    LLViewerObject* findClosestAttachment(S32 mouse_x, S32 mouse_y, F32 tolerance,
                                          bool& out_gpu_authoritative);
}

#endif // FS_SELFRIGGEDPICKER_H
