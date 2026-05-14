/**
 * @file fsselfriggedpicker.cpp
 * @brief Two-stage picker for the agent's own rigged attachments.
 *        Stage 1: ray-triangle pierce test (closest-to-camera pierce wins).
 *        Stage 2 (fallback): edge near-miss within tolerance (closest-to-camera
 *        edge wins). Pierce always beats near-miss.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * AYAstorm Viewer Source Code
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "fsselfriggedpicker.h"

#include "llvoavatarself.h"
#include "llvovolume.h"
#include "llviewerjointattachment.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"
#include "llvolume.h"
#include "v3math.h"
#include "v4math.h"

namespace
{
    // Two-sided ray-triangle intersect (Möller-Trumbore). Treats the ray as a
    // segment A + t*d with t in [0, 1]. Returns true if the ray pierces the
    // triangle (with a small barycentric tolerance so that fitted-mesh clothing
    // — whose triangles sit only millimeters off the body surface — still
    // qualifies even when the ray clips just outside a strict u/v border).
    // BARY_EPS = 0.02 ≈ 1mm on a 5cm triangle; far below the cm-scale
    // separation that distinguishes e.g. hair triangles from face triangles, so
    // it does not reintroduce the hair-steals-face problem fixed previously.
    bool rayTriangleIntersect(const LLVector3& orig, const LLVector3& dir,
                              const LLVector3& v0, const LLVector3& v1, const LLVector3& v2,
                              F32& t_out)
    {
        const F32 EPS = 1e-8f;
        const F32 BARY_EPS = 0.02f;
        LLVector3 e1 = v1 - v0;
        LLVector3 e2 = v2 - v0;
        LLVector3 pvec = dir % e2;
        F32 det = e1 * pvec;
        if (fabsf(det) < EPS)
        {
            return false; // parallel to triangle plane
        }
        F32 inv_det = 1.f / det;
        LLVector3 tvec = orig - v0;
        F32 u = (tvec * pvec) * inv_det;
        if (u < -BARY_EPS || u > 1.f + BARY_EPS)
        {
            return false;
        }
        LLVector3 qvec = tvec % e1;
        F32 v = (dir * qvec) * inv_det;
        if (v < -BARY_EPS || (u + v) > 1.f + BARY_EPS)
        {
            return false;
        }
        F32 t = (e2 * qvec) * inv_det;
        if (t < 0.f || t > 1.f)
        {
            return false;
        }
        t_out = t;
        return true;
    }

    // Squared distance between two finite line segments S1=[A1, A1+d1] and S2=[A2, A2+d2].
    // d1_sq/d2_sq are precomputed |d|^2. Closed-form clamp-to-[0,1] solution.
    // Returns the foot parameter on S1 in `ray_t_out` (the [0,1] position along
    // the ray where it is closest to the edge — smaller = nearer the camera).
    F32 distSqSegToSeg(const LLVector3& A1, const LLVector3& d1, F32 d1_sq,
                       const LLVector3& A2, const LLVector3& d2, F32 d2_sq,
                       F32& ray_t_out)
    {
        const F32 EPS = 1e-8f;
        LLVector3 r = A1 - A2;
        F32 c = d1 * r;
        F32 f = d2 * r;
        F32 s, t;

        if (d1_sq <= EPS && d2_sq <= EPS)
        {
            ray_t_out = 0.f;
            return r.lengthSquared();
        }
        if (d1_sq <= EPS)
        {
            s = 0.f;
            t = llclamp(f / d2_sq, 0.f, 1.f);
        }
        else if (d2_sq <= EPS)
        {
            t = 0.f;
            s = llclamp(-c / d1_sq, 0.f, 1.f);
        }
        else
        {
            F32 b = d1 * d2;
            F32 denom = d1_sq * d2_sq - b * b;
            if (denom > EPS)
            {
                s = llclamp((b * f - c * d2_sq) / denom, 0.f, 1.f);
            }
            else
            {
                s = 0.f;
            }
            t = (b * s + f) / d2_sq;
            if (t < 0.f)
            {
                t = 0.f;
                s = llclamp(-c / d1_sq, 0.f, 1.f);
            }
            else if (t > 1.f)
            {
                t = 1.f;
                s = llclamp((b - c) / d1_sq, 0.f, 1.f);
            }
        }

        ray_t_out = s;
        LLVector3 closest1 = A1 + d1 * s;
        LLVector3 closest2 = A2 + d2 * t;
        return (closest1 - closest2).lengthSquared();
    }

    // Per-volume aggregation. Two metrics are computed in a single triangle pass:
    //   (1) hits_through: did the ray actually pierce any triangle interior?
    //       If yes, record the smallest pierce t (= closest-to-camera hit).
    //   (2) near_miss: did any triangle edge come within `tolerance` of the ray?
    //       If yes, record the smallest ray foot t at that edge.
    // The caller prefers (1) over (2): a volume that the ray physically passes
    // through always beats one that the ray merely grazes nearby. This stops
    // a hair mesh that hangs in front of the face (its triangle edges within
    // tolerance, but no triangle actually pierced by the ray under the nose)
    // from stealing the click from the face mesh the ray really enters.
    struct VolumeHit
    {
        bool hits_through = false;
        F32 pierce_t = 1.f + 1e-3f; // smallest t among pierced triangles
        bool near_miss = false;
        F32 grazes_t = 1.f + 1e-3f; // smallest ray foot t among in-tolerance edges
    };

    bool scanVolume(LLVOVolume* volp,
                    const LLVector3& ray_a,
                    const LLVector3& ray_dir,
                    F32 ray_len_sq,
                    F32 tol_sq,
                    VolumeHit& out_hit)
    {
        if (!volp || volp->isDead() || volp->isHUDAttachment())
        {
            return false;
        }
        if (!volp->isRiggedMesh())
        {
            return false;
        }

        // Ensure mRiggedVolume is allocated and CPU-skinned for all faces.
        volp->updateRiggedVolume(true, LLRiggedVolume::UPDATE_ALL_FACES, false);
        LLRiggedVolume* rigged = volp->getRiggedVolume();
        if (!rigged)
        {
            return false;
        }

        VolumeHit hit;
        const S32 nfaces = rigged->getNumVolumeFaces();
        for (S32 f = 0; f < nfaces; ++f)
        {
            const LLVolumeFace& face = rigged->getVolumeFace(f);
            if (!face.mPositions || !face.mIndices || face.mNumVertices <= 0 || face.mNumIndices < 3)
            {
                continue;
            }
            const LLVector4a* positions = face.mPositions;
            const U16* indices = face.mIndices;
            const S32 ntris = face.mNumIndices / 3;
            for (S32 tri = 0; tri < ntris; ++tri)
            {
                const U16 i0 = indices[tri * 3 + 0];
                const U16 i1 = indices[tri * 3 + 1];
                const U16 i2 = indices[tri * 3 + 2];
                if (i0 >= face.mNumVertices || i1 >= face.mNumVertices || i2 >= face.mNumVertices)
                {
                    continue;
                }
                const F32* p0 = positions[i0].getF32ptr();
                const F32* p1 = positions[i1].getF32ptr();
                const F32* p2 = positions[i2].getF32ptr();
                LLVector3 v0(p0[0], p0[1], p0[2]);
                LLVector3 v1(p1[0], p1[1], p1[2]);
                LLVector3 v2(p2[0], p2[1], p2[2]);

                // (1) Pierce test.
                F32 pierce_t;
                if (rayTriangleIntersect(ray_a, ray_dir, v0, v1, v2, pierce_t))
                {
                    if (pierce_t < hit.pierce_t)
                    {
                        hit.pierce_t = pierce_t;
                        hit.hits_through = true;
                    }
                }

                // (2) Edge near-miss test (only meaningful if no pierce was found
                // yet for this volume, but cheap enough to always run).
                LLVector3 e[3] = { v1 - v0, v2 - v1, v0 - v2 };
                LLVector3 a[3] = { v0, v1, v2 };
                F32 e_sq[3] = { e[0].lengthSquared(), e[1].lengthSquared(), e[2].lengthSquared() };
                for (int k = 0; k < 3; ++k)
                {
                    F32 ray_t;
                    F32 d2 = distSqSegToSeg(ray_a, ray_dir, ray_len_sq, a[k], e[k], e_sq[k], ray_t);
                    if (d2 <= tol_sq && ray_t < hit.grazes_t)
                    {
                        hit.grazes_t = ray_t;
                        hit.near_miss = true;
                    }
                }
            }
        }

        if (hit.hits_through || hit.near_miss)
        {
            out_hit = hit;
            return true;
        }
        return false;
    }

    struct BestPick
    {
        LLViewerObject* obj = nullptr;
        F32 score_t = 1.f + 1e-3f; // smaller is better
        bool from_pierce = false;  // true means score_t came from a real ray-through hit
    };

    void considerVolume(LLVOVolume* volp, const VolumeHit& hit, BestPick& best)
    {
        if (hit.hits_through)
        {
            // Pierce always beats near-miss. Among pierces, smallest t wins.
            if (!best.from_pierce || hit.pierce_t < best.score_t)
            {
                best.obj = volp;
                best.score_t = hit.pierce_t;
                best.from_pierce = true;
            }
        }
        else if (hit.near_miss && !best.from_pierce)
        {
            // Only a near-miss; only competes against other near-misses.
            if (hit.grazes_t < best.score_t)
            {
                best.obj = volp;
                best.score_t = hit.grazes_t;
                best.from_pierce = false;
            }
        }
    }

    void scanObjectAndChildren(LLViewerObject* obj,
                               const LLVector3& ray_a,
                               const LLVector3& ray_dir,
                               F32 ray_len_sq,
                               F32 tol_sq,
                               BestPick& best)
    {
        if (!obj || obj->isDead())
        {
            return;
        }
        if (obj->getPCode() == LL_PCODE_VOLUME)
        {
            LLVOVolume* volp = dynamic_cast<LLVOVolume*>(obj);
            VolumeHit hit;
            if (volp && scanVolume(volp, ray_a, ray_dir, ray_len_sq, tol_sq, hit))
            {
                considerVolume(volp, hit, best);
            }
        }
        for (LLViewerObject* child : obj->getChildren())
        {
            scanObjectAndChildren(child, ray_a, ray_dir, ray_len_sq, tol_sq, best);
        }
    }
}

LLViewerObject* FSSelfRiggedPicker::findClosestAttachment(S32 mouse_x, S32 mouse_y, F32 tolerance)
{
    if (!isAgentAvatarValid())
    {
        return nullptr;
    }
    if (tolerance <= 0.f)
    {
        return nullptr;
    }

    // Build the world ray in the same agent-space coordinates that LLRiggedVolume
    // stores its CPU-skinned mPositions in (matches LLViewerWindow::cursorIntersect /
    // LLVOVolume::lineSegmentIntersect rigged path, which skips the volume-local
    // transform).
    LLVector3 cam_origin = LLViewerCamera::getInstance()->getOrigin();
    LLVector3 mouse_dir = gViewerWindow->mouseDirectionGlobal(mouse_x, mouse_y);
    const F32 depth = 512.f; // matches upstream cursorIntersect default
    LLVector3 ray_a = cam_origin;
    LLVector3 ray_dir = mouse_dir * depth; // segment vector
    F32 ray_len_sq = ray_dir.lengthSquared();
    if (ray_len_sq <= 0.f)
    {
        return nullptr;
    }
    F32 tol_sq = tolerance * tolerance;

    // Winner selection (see VolumeHit / considerVolume in this file):
    //   1. If any attachment is actually pierced by the ray (triangle interior),
    //      pierces beat all near-misses, and among pierces the smallest t (the
    //      surface nearest the camera) wins. This makes a shirt in front of the
    //      body win over the body underneath, and makes the face mesh win over
    //      hair that drapes nearby but doesn't actually intersect the ray.
    //   2. If no pierce, fall back to "closest edge wins" by ray foot t (this
    //      covers the original ~4.7cm offset rescue case).
    BestPick best;

    for (const auto& it : gAgentAvatarp->mAttachmentPoints)
    {
        LLViewerJointAttachment* att = it.second;
        if (!att || att->getIsHUDAttachment())
        {
            continue;
        }
        for (LLViewerObject* root : att->mAttachedObjects)
        {
            scanObjectAndChildren(root, ray_a, ray_dir, ray_len_sq, tol_sq, best);
        }
    }

    return best.obj;
}
