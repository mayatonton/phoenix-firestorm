/**
 * @file fsselfriggedpicker.cpp
 * @brief Hybrid picker for the agent's own rigged attachments.
 *        Stage 0 (depth-assist): read the GPU depth buffer at the mouse pixel,
 *        unproject to a world point, then choose the rigged attachment whose
 *        CPU-skinned triangle lies closest to that point. This is the only
 *        stage that agrees with what is actually rendered on screen (alpha
 *        discard / depth-mask hair etc. are honored by the depth buffer), and
 *        is also immune to the CPU-skin vs GPU-skin one-frame lag that breaks
 *        the ray stages when an idle animation moves the mesh.
 *        Stage 1 (ray-based fallback): ray-triangle pierce test (closest-to-
 *        camera pierce wins).
 *        Stage 2 (ray-based fallback): edge near-miss within tolerance
 *        (closest-to-camera edge wins). Pierce always beats near-miss.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * AYAstorm Viewer Source Code
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "fsselfriggedpicker.h"

#include "llcontrol.h"
#include "llrender.h"
#include "llrendertarget.h"
#include "llviewercontrol.h"

#include "glm/ext/matrix_projection.hpp" // glm::unProject
#include "llvoavatarself.h"
#include "llvovolume.h"
#include "llviewerjointattachment.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"
#include "llvolume.h"
#include "pipeline.h"
#include "v3math.h"
#include "v4math.h"

namespace
{
    // Two-sided ray-triangle intersect (Möller-Trumbore). Treats the ray as a
    // segment A + t*d with t in [0, 1]. Returns true if the ray pierces the
    // triangle (with a small barycentric tolerance so that fitted-mesh clothing
    // — whose triangles sit only millimeters off the body surface — still
    // qualifies even when the ray clips just outside a strict u/v border).
    // `front_facing_out` distinguishes camera-facing front faces from
    // back-facing geometry: CCW winding + det > 0 means the triangle normal
    // opposes the ray direction (so the triangle is presenting its front side
    // to the camera). The caller prefers front-face pierces over back-face
    // ones; this protects shirt-over-body from CPU-skin numerical noise where
    // a body-side back-face would otherwise win the smallest-t race against
    // the shirt-side front-face that the user actually clicked.
    bool rayTriangleIntersect(const LLVector3& orig, const LLVector3& dir,
                              const LLVector3& v0, const LLVector3& v1, const LLVector3& v2,
                              F32& t_out, bool& front_facing_out)
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
        front_facing_out = (det > 0.f);
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

    // Per-volume aggregation. Three metrics are computed in a single triangle pass:
    //   (1) hits_front: did the ray pierce any front-facing triangle (camera
    //       side)? If yes, record the smallest such pierce t.
    //   (2) hits_back: did the ray pierce any back-facing triangle (the inside
    //       face of the mesh, looking away from camera)? Smallest t recorded.
    //   (3) near_miss: did any triangle edge come within tolerance of the ray?
    //       If yes, record the smallest ray foot t at that edge.
    // The caller prefers (1) > (2) > (3): a volume whose camera-facing surface
    // the ray pierces beats one that the ray only sees the back side of (which
    // happens when CPU-skin numerical noise reorders mm-scale-overlapping mesh
    // surfaces), which in turn beats a volume that the ray only grazes.
    struct VolumeHit
    {
        bool hits_front = false;
        F32 front_pierce_t = 1.f + 1e-3f; // smallest t among pierced front-facing triangles
        bool hits_back = false;
        F32 back_pierce_t = 1.f + 1e-3f; // smallest t among pierced back-facing triangles
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
                bool front_facing;
                if (rayTriangleIntersect(ray_a, ray_dir, v0, v1, v2, pierce_t, front_facing))
                {
                    if (front_facing)
                    {
                        if (pierce_t < hit.front_pierce_t)
                        {
                            hit.front_pierce_t = pierce_t;
                            hit.hits_front = true;
                        }
                    }
                    else
                    {
                        if (pierce_t < hit.back_pierce_t)
                        {
                            hit.back_pierce_t = pierce_t;
                            hit.hits_back = true;
                        }
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

        if (hit.hits_front || hit.hits_back || hit.near_miss)
        {
            out_hit = hit;
            return true;
        }
        return false;
    }

    // Ranking categories — higher beats lower regardless of score_t.
    enum PickCategory
    {
        PICK_NONE       = 0,
        PICK_NEAR_MISS  = 1, // ray grazed an edge only
        PICK_BACK_FACE  = 2, // ray pierced a back-facing triangle
        PICK_FRONT_FACE = 3  // ray pierced a camera-facing triangle (preferred)
    };

    struct BestPick
    {
        LLViewerObject* obj = nullptr;
        F32 score_t = 1.f + 1e-3f; // smaller is better, only compared within the same category
        PickCategory category = PICK_NONE;
    };

    void considerVolume(LLVOVolume* volp, const VolumeHit& hit, BestPick& best, bool allow_near_miss)
    {
        // Front-face pierce: highest tier. A shirt's outer surface in front of
        // a body's outer surface wins here automatically (smaller t).
        if (hit.hits_front)
        {
            if (best.category < PICK_FRONT_FACE || hit.front_pierce_t < best.score_t)
            {
                best.obj = volp;
                best.score_t = hit.front_pierce_t;
                best.category = PICK_FRONT_FACE;
            }
            return;
        }
        // Back-face pierce: only competes if nothing has a front pierce yet.
        // This protects against CPU-skin noise where a body-side back-face
        // would otherwise sneak under the shirt's front-face by a sub-mm
        // pierce_t margin.
        if (hit.hits_back && best.category <= PICK_BACK_FACE)
        {
            if (best.category < PICK_BACK_FACE || hit.back_pierce_t < best.score_t)
            {
                best.obj = volp;
                best.score_t = hit.back_pierce_t;
                best.category = PICK_BACK_FACE;
            }
            return;
        }
        // Near-miss: lowest tier; only competes against other near-misses.
        // Suppressed entirely when allow_near_miss=false (r21.1 needle-aim
        // mode — the GPU ID buffer is the precise path, legacy stages only
        // exist to recover pierces the GPU couldn't sample). Near-miss
        // recoveries widen the hit zone by tolerance metres, which feels
        // like ragdoll aiming on closeup avatars.
        if (allow_near_miss && hit.near_miss && best.category <= PICK_NEAR_MISS)
        {
            if (best.category < PICK_NEAR_MISS || hit.grazes_t < best.score_t)
            {
                best.obj = volp;
                best.score_t = hit.grazes_t;
                best.category = PICK_NEAR_MISS;
            }
        }
    }

    void scanObjectAndChildren(LLViewerObject* obj,
                               const LLVector3& ray_a,
                               const LLVector3& ray_dir,
                               F32 ray_len_sq,
                               F32 tol_sq,
                               BestPick& best,
                               bool allow_near_miss)
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
                considerVolume(volp, hit, best, allow_near_miss);
            }
        }
        for (LLViewerObject* child : obj->getChildren())
        {
            scanObjectAndChildren(child, ray_a, ray_dir, ray_len_sq, tol_sq, best, allow_near_miss);
        }
    }

    // ----- Stage -1 (GPU ID buffer) helper -------------------------------

    // Recursive walk through gAgentAvatarp's attachment tree, looking for the
    // child whose LocalID matches what the GPU object-ID buffer reported at
    // the mouse pixel. We deliberately scope the search to self attachments
    // only — Stage -1 is a self-picker; the upstream picker handles the rest
    // of the scene. This also means we never look up by ip/port and never
    // risk grabbing a non-self object whose LocalID happens to collide.
    LLViewerObject* findSelfAttachmentByLocalID(LLViewerObject* obj, U32 target_id)
    {
        if (!obj)
        {
            return nullptr;
        }
        if (obj->getLocalID() == target_id)
        {
            return obj;
        }
        for (LLViewerObject* child : obj->getChildren())
        {
            if (LLViewerObject* hit = findSelfAttachmentByLocalID(child, target_id))
            {
                return hit;
            }
        }
        return nullptr;
    }

    // ----- Stage 0 (depth-assist) helper ---------------------------------

    // Read the depth buffer at (mouse_x, mouse_y) and unproject back to an
    // agent-space world point. Returns false if depth-assist is unavailable
    // (no deferred RT bound, or the sampled depth is at the far plane = sky).
    bool screenDepthToAgentPoint(S32 mouse_x, S32 mouse_y, LLVector3& out_point)
    {
        if (!gPipeline.mRT)
        {
            return false;
        }
        LLRenderTarget& screen_rt = gPipeline.mRT->screen;
        if (!screen_rt.isComplete())
        {
            return false;
        }

        // The world view rectangle (excluding UI chrome) is the viewport that
        // the deferred geometry pass and the camera matrices were set up with.
        LLRect wvr = gViewerWindow->getWorldViewRectRaw();
        glm::ivec4 viewport(wvr.mLeft, wvr.mBottom, wvr.getWidth(), wvr.getHeight());

        // mouse_x / mouse_y arrive here as LLCoordGL — bottom-origin window
        // pixels in *scaled* (logical) coords. The screen RT is allocated at
        // WorldViewRectRaw dimensions (pipeline.cpp resizeScreenTexture()), so
        // its (0,0) is the world view rect's bottom-left in raw pixels.
        //   1) scaled → raw via DisplayScale
        //   2) subtract WorldViewRectRaw.mLeft/mBottom → buffer-local coord
        // unProject(viewport = WorldViewRectRaw) likewise expects buffer-local
        // pixels (its viewport rectangle has been translated to (mLeft,mBottom)
        // already, so pass the buffer-local coord and unProject re-applies the
        // offset internally).
        const F32 sx = (F32)gViewerWindow->getWindowWidthRaw()  / (F32)gViewerWindow->getWindowWidthScaled();
        const F32 sy = (F32)gViewerWindow->getWindowHeightRaw() / (F32)gViewerWindow->getWindowHeightScaled();
        const S32 mx_win_raw = (S32)llround((F32)mouse_x * sx);
        const S32 my_win_raw = (S32)llround((F32)mouse_y * sy);
        const S32 mx_buf = mx_win_raw - wvr.mLeft;
        const S32 my_buf = my_win_raw - wvr.mBottom;

        if (mx_buf < 0 || my_buf < 0 ||
            mx_buf >= wvr.getWidth() || my_buf >= wvr.getHeight())
        {
            return false;
        }

        F32 depth_sample = 1.f;
        screen_rt.bindTarget();
        glReadPixels(mx_buf, my_buf, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth_sample);
        screen_rt.flush();

        // depth ~= 1.0 means "no geometry written here" (sky / discarded /
        // alpha-blend with depth-write off). Fall back to the ray stages.
        if (depth_sample >= 0.9999f)
        {
            return false;
        }

        // glm::unProject expects window coords matching `viewport`. Since
        // viewport is (wvr.mLeft, wvr.mBottom, wvr.W, wvr.H), pass the
        // window-raw pixel here (not buffer-local) so it lies inside the
        // viewport rect.
        glm::vec3 win_coord((F32)mx_win_raw, (F32)my_win_raw, depth_sample);
        glm::vec3 agent_coord = glm::unProject(win_coord,
                                               get_current_modelview(),
                                               get_current_projection(),
                                               viewport);
        out_point.setVec((F32)agent_coord.x, (F32)agent_coord.y, (F32)agent_coord.z);
        return true;
    }

}

LLViewerObject* FSSelfRiggedPicker::findClosestAttachment(S32 mouse_x, S32 mouse_y, F32 tolerance,
                                                          bool& out_gpu_authoritative)
{
    out_gpu_authoritative = false;
    if (!isAgentAvatarValid())
    {
        return nullptr;
    }
    if (tolerance <= 0.f)
    {
        return nullptr;
    }

    // Stage -1: GPU object-ID buffer. The dedicated pass in renderDeferredLighting
    // re-draws gAgentAvatarp's rigged attachments using the same GPU skinning the
    // visible scene uses, packing each attachment's LocalID into RGBA8. Reading
    // back the byte quad at the mouse pixel gives a pixel-perfect agreement with
    // what is actually on screen — no CPU-skin vs GPU-skin drift, no ray vs
    // skin-frame mismatch, no near-miss heuristics.
    //
    // r21.1 M4.7: fully authoritative. CPU bind-pose mesh ray (the legacy
    // path below) cannot be made accurate for rigged attachments — that is
    // exactly the upstream Linden picker problem r21.1 exists to solve.
    // Falling back to it on GPU id=0 just reintroduces the old ragdoll aim.
    // Outcomes:
    //   - id != 0 + match → return it.
    //   - id != 0 + no match (race: attachment removed mid-frame, stale
    //     value) → out_gpu_authoritative=true, null. Caller may force-to-body.
    //   - id == 0 → "no self rigged attachment at this pixel" is the truth.
    //     out_gpu_authoritative=true, null. Caller may force-to-body when
    //     upstream's worldray picked one of our attachments.
    //
    // Closeup-zoom shirt pixels currently return id=0 even when shirt is
    // visually present — that is a GPU pass deficiency (cull/alpha/skin
    // matrix/LOD state mismatch with deferred opaque) and must be fixed in
    // the pass, NOT papered over with a CPU fallback.
    static LLCachedControl<bool> gpu_enable(gSavedSettings, "FSSelfRiggedPickerGPU", false);
    if (gpu_enable && gPipeline.mObjectIDBuffer.isComplete())
    {
        // Coordinate conversion. mObjectIDBuffer is allocated by
        // resizeScreenTexture() at *WorldViewRectRaw* dimensions — not the
        // full window — so the buffer's (0,0) corresponds to the world
        // view rect's bottom-left in *raw* pixels. LLCoordGL mouse coords
        // are window-relative *scaled* (logical) pixels.
        //   1) scaled → raw via DisplayScale (mWindowRectRaw / mWindowRectScaled)
        //   2) subtract WorldViewRectRaw's mLeft/mBottom origin so the
        //      result is buffer-local.
        // Missing either step (raw mismatch on HiDPI / UI-chrome offset)
        // reads the wrong pixel — typically id=0.
        const LLRect wv_raw = gViewerWindow->getWorldViewRectRaw();
        const F32 sx = (F32)gViewerWindow->getWindowWidthRaw()  / (F32)gViewerWindow->getWindowWidthScaled();
        const F32 sy = (F32)gViewerWindow->getWindowHeightRaw() / (F32)gViewerWindow->getWindowHeightScaled();
        const S32 mx_win_raw = (S32)llround((F32)mouse_x * sx);
        const S32 my_win_raw = (S32)llround((F32)mouse_y * sy);
        const S32 mx_buf = mx_win_raw - wv_raw.mLeft;
        const S32 my_buf = my_win_raw - wv_raw.mBottom;

        // Bounds guard: a click in UI chrome (outside the world view rect)
        // would otherwise pass a negative coord to glReadPixels (undefined).
        if (mx_buf < 0 || my_buf < 0 ||
            mx_buf >= gPipeline.mObjectIDBuffer.getWidth() ||
            my_buf >= gPipeline.mObjectIDBuffer.getHeight())
        {
            out_gpu_authoritative = false;
            LL_INFOS("FSPicker") << "GPU stage -1: mouse=(" << mouse_x << "," << mouse_y
                                 << ") buf=(" << mx_buf << "," << my_buf << ") OUT OF BOUNDS"
                                 << " wv_raw=(" << wv_raw.mLeft << "," << wv_raw.mBottom
                                 << "," << wv_raw.getWidth() << "x" << wv_raw.getHeight() << ")"
                                 << " bufRes=" << gPipeline.mObjectIDBuffer.getWidth()
                                 << "x" << gPipeline.mObjectIDBuffer.getHeight()
                                 << " -> falling through" << LL_ENDL;
        }
        else
        {

        GLubyte rgba[4] = {0, 0, 0, 0};
        gPipeline.mObjectIDBuffer.bindTarget();
        glReadPixels(mx_buf, my_buf, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        gPipeline.mObjectIDBuffer.flush();
        U32 local_id = ((U32)rgba[0])
                     | ((U32)rgba[1] << 8)
                     | ((U32)rgba[2] << 16)
                     | ((U32)rgba[3] << 24);

        // GPU is the source of truth.
        out_gpu_authoritative = true;

        if (local_id != 0)
        {
            for (const auto& it : gAgentAvatarp->mAttachmentPoints)
            {
                LLViewerJointAttachment* att = it.second;
                if (!att || att->getIsHUDAttachment())
                {
                    continue;
                }
                for (LLViewerObject* root : att->mAttachedObjects)
                {
                    if (LLViewerObject* hit = findSelfAttachmentByLocalID(root, local_id))
                    {
                        // <AYAstorm:r21.1-diag> M4.7 / M4.13
                        LL_INFOS("FSPicker") << "GPU stage -1: mouse=(" << mouse_x << "," << mouse_y
                                             << ") buf=(" << mx_buf << "," << my_buf << ")"
                                             << " rgba=(" << (S32)rgba[0] << "," << (S32)rgba[1]
                                             << "," << (S32)rgba[2] << "," << (S32)rgba[3]
                                             << ") id=" << local_id
                                             << " -> resolved id=" << hit->getID() << LL_ENDL;
                        // </AYAstorm:r21.1-diag>
                        return hit;
                    }
                }
            }
            // id != 0 but no matching attachment: race condition. GPU clearly
            // sees something here, but our attachment table no longer has the
            // owning object. authoritative empty so caller can force the body.
            // <AYAstorm:r21.1-diag> M4.7 / M4.13
            LL_INFOS("FSPicker") << "GPU stage -1: mouse=(" << mouse_x << "," << mouse_y
                                 << ") buf=(" << mx_buf << "," << my_buf << ")"
                                 << " rgba=(" << (S32)rgba[0] << "," << (S32)rgba[1]
                                 << "," << (S32)rgba[2] << "," << (S32)rgba[3]
                                 << ") id=" << local_id
                                 << " -> NOT FOUND in attachment table (authoritative empty)" << LL_ENDL;
            // </AYAstorm:r21.1-diag>
        }
        else
        {
            // id == 0: GPU says no self rigged attachment at this pixel. The
            // truth of the visible scene. Authoritative empty.
            // <AYAstorm:r21.1-diag> M4.7 / M4.13
            LL_INFOS("FSPicker") << "GPU stage -1: mouse=(" << mouse_x << "," << mouse_y
                                 << ") buf=(" << mx_buf << "," << my_buf << ")"
                                 << " rgba=(0,0,0,0) id=0 -> authoritative empty (no rigged self at pixel)" << LL_ENDL;
            // </AYAstorm:r21.1-diag>
        }
        // GPU has spoken — legacy stages would only re-introduce CPU bind-pose
        // inaccuracy that r21.1 was built to eliminate. The legacy code below
        // remains for diagnostic comparison if FSSelfRiggedPickerGPU is off.
        return nullptr;
        } // end else (in-bounds branch)
    }

    // Stage 0: depth-assist. Read the GPU depth buffer at the mouse pixel,
    // unproject it to a world point, then run the existing ray-based pierce /
    // near-miss machinery on a SHORT ray from the camera to that point. Why
    // a short ray instead of "nearest triangle to the point":
    //   - shirt-over-body: shirt triangles sit at smaller pierce_t than body
    //     triangles along the same view ray, so the closest-to-camera pierce
    //     naturally wins (point-to-triangle distance, by contrast, lets body
    //     beat shirt in the mm-overlap zone via numerical noise).
    //   - hair-over-face: the depth buffer records the face surface (the hair
    //     mesh's alpha-discarded pixels never wrote depth there), so the ray
    //     ends at the face and any hair triangles further along the ray are
    //     t > 1 and excluded.
    //   - idle-skin drift: a 10cm-class tolerance on this short ray absorbs
    //     the CPU-vs-GPU skin frame drift that broke the long ray stages.
    static LLCachedControl<bool> depth_assist_enable(gSavedSettings, "FSSelfRiggedPickerDepthAssist", true);
    static LLCachedControl<F32>  depth_tol_cv(gSavedSettings, "FSSelfRiggedPickerDepthTolerance", 0.10f);
    if (depth_assist_enable)
    {
        LLVector3 hit_point;
        if (screenDepthToAgentPoint(mouse_x, mouse_y, hit_point))
        {
            F32 depth_tol = (F32)depth_tol_cv;
            if (depth_tol > 0.f)
            {
                LLVector3 cam = LLViewerCamera::getInstance()->getOrigin();
                // 1.001f buffers the t≈1 surface pierces so they don't get
                // rejected by the strict t<=1 check inside rayTriangleIntersect.
                LLVector3 short_dir = (hit_point - cam) * 1.001f;
                F32 short_len_sq = short_dir.lengthSquared();
                if (short_len_sq > 0.f)
                {
                    F32 short_tol_sq = depth_tol * depth_tol;
                    BestPick short_best;
                    for (const auto& it : gAgentAvatarp->mAttachmentPoints)
                    {
                        LLViewerJointAttachment* att = it.second;
                        if (!att || att->getIsHUDAttachment())
                        {
                            continue;
                        }
                        for (LLViewerObject* root : att->mAttachedObjects)
                        {
                            // r21.1 M4.7: this code path only runs when the
                            // GPU stage is unavailable (cvar off, or RT
                            // allocation failed on a weak GPU / software
                            // renderer). In that case we cannot do better
                            // than CPU bind-pose mesh anyway, so restore the
                            // near-miss tolerance recovery that upstream
                            // Linden picker has historically relied on.
                            // Users without a working GPU stage stay at
                            // upstream-compatible "ragdoll Rigged aim" but
                            // never lose the click outright.
                            scanObjectAndChildren(root, cam, short_dir, short_len_sq, short_tol_sq, short_best, /*allow_near_miss*/ true);
                        }
                    }
                    if (short_best.obj)
                    {
                        // <AYAstorm:r21.1-diag> M4.1
                        LL_INFOS("FSPicker") << "Legacy stage 0 (depth-assist): resolved="
                                             << short_best.obj->getID() << LL_ENDL;
                        // </AYAstorm:r21.1-diag>
                        return short_best.obj;
                    }
                }
            }
        }
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
    //   - Front-face pierce wins, then back-face pierce. Within a category,
    //     smallest t (closest surface) wins.
    //   - Near-miss recovery is allowed here (M4.7 — this stage only runs
    //     for users without a working GPU stage, where staying upstream-
    //     compatible matters more than needle-aim precision).
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
            scanObjectAndChildren(root, ray_a, ray_dir, ray_len_sq, tol_sq, best, /*allow_near_miss*/ true);
        }
    }

    // <AYAstorm:r21.1-diag> M4.4
    LL_INFOS("FSPicker") << "Legacy stage 1 (ray pierce-only): resolved="
                         << (best.obj ? best.obj->getID().asString() : std::string("null"))
                         << LL_ENDL;
    // </AYAstorm:r21.1-diag>
    return best.obj;
}
