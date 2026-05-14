/**
 * @file fsselfriggedpicker.h
 * @brief Vertex-distance picker for the agent's own rigged attachments.
 *
 * AYAstorm r21.1 — replacement for the upstream LLPipeline::lineSegmentIntersectInWorld
 * path when picking self rigged attachments. The upstream path mis-aligns the world
 * ray vs the GPU-skinned mesh in some configurations, so we fall back to a brute
 * two-stage scan over gAgentAvatarp's rigged attachment volumes:
 *   - stage 1: ray-triangle pierce test (Möller-Trumbore, two-sided);
 *   - stage 2: per-edge segment-to-segment distance within tolerance.
 * Pierce always wins over near-miss. Among pierces (or among near-misses when no
 * pierce is found anywhere), the surface nearest the camera along the ray wins.
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
    // Find the self rigged attachment that the screen-space ray from
    // (mouse_x, mouse_y) hits. Preference order:
    //   1. Attachments whose triangle the ray actually pierces (Möller-Trumbore,
    //      two-sided). Among those, the closest-to-camera pierce wins.
    //   2. Otherwise, attachments with at least one CPU-skinned triangle edge
    //      within `tolerance` meters of the ray; among those, the closest-to-
    //      camera edge wins.
    // Returns nullptr if neither stage finds a candidate.
    LLViewerObject* findClosestAttachment(S32 mouse_x, S32 mouse_y, F32 tolerance);
}

#endif // FS_SELFRIGGEDPICKER_H
