/**
 * @file llocclusiongeometrymgr.h
 * @brief AYAstorm r13 spike: tag-based static OBB occlusion.
 *
 * Scans prim Description for [ayastorm:occlude] and stores an OBB. The
 * shipped libfmod 2.03.07 has a non-functional createGeometry (returns
 * FMOD_ERR_INTERNAL even for the smallest allocation), so we cannot
 * delegate raycasting to FMOD::Geometry. Instead we keep the OBB set
 * locally and run our own segment-vs-OBB slab test per audio frame,
 * applying the result via Channel::set3DOcclusion. This keeps the
 * tag-driven scope (r13 cap = 256, power-of-two with ~25% headroom over
 * typical SL venue counts) so the work stays O(N_occluders × N_speakers ×
 * frame_rate) which, with the cap, is comfortably under 1 ms/sec on modern
 * CPUs.
 */

#ifndef LL_OCCLUSIONGEOMETRYMGR_H
#define LL_OCCLUSIONGEOMETRYMGR_H

#include "llsingleton.h"
#include "llquaternion.h"
#include "lluuid.h"
#include "v3math.h"

#include <map>
#include <string>
#include <vector>

class LLViewerObject;

namespace FMOD
{
    class Channel;
    class DSP;
}

class LLOcclusionGeometryMgr : public LLSingleton<LLOcclusionGeometryMgr>
{
    LLSINGLETON(LLOcclusionGeometryMgr);
    ~LLOcclusionGeometryMgr() override;

public:
    // Hooked from LLSelectMgr::processObjectProperties /
    // processObjectPropertiesFamily. Adds, updates, or removes the
    // OBB depending on the current tag state of the description.
    void onObjectPropertiesReceived(const LLUUID& id, const std::string& description);

    // Called once per LLPositionalStreamMgr::update() tick. Drops dead /
    // de-tagged prims, refreshes position/rotation/scale, and re-parses
    // direct/reverb fields. ~40us at the 256-occluder cap (still negligible).
    void refreshOccluders();

    // Called per frame per speaker from LLPositionalStreamMgr::update.
    // Ray-casts (listener, source) against the OBB set, applies
    // Channel::set3DOcclusion(direct, reverb), and (if a per-speaker
    // LOWPASS_SIMPLE DSP exists) pushes a cutoff-Hz mapped from the
    // smoothed direct factor so wall-occluded audio also sounds muffled
    // instead of just quieter. lowpass may be null when DSP creation
    // failed; the cutoff push is skipped in that case.
    void applyToChannel(FMOD::Channel* channel,
                        FMOD::DSP* lowpass,
                        const LLVector3& listener,
                        const LLVector3& source);

    // r13: debug overlay — draws every registered OBB as a coloured
    // wireframe (orange = registered, default tag values; red = direct
    // ≥ 0.9 i.e. "near-opaque wall"). Called from LLPipeline::renderDebug
    // when the `Stream3DShowOccluders` debug setting is true. Caller is
    // responsible for binding gDebugProgram before / unbinding after.
    void renderDebug() const;

private:
    // Bounding OBB in world (region-relative) space. Cheap pre-cull: a
    // segment that misses the box can never hit any triangle inside it.
    struct OBB
    {
        LLVector3    center;
        LLVector3    half;
        LLQuaternion rot;
    };

    // Triangle in prim-local space (vertices delivered by LLVolume::getVolumeFace,
    // typically within -0.5..0.5 before the prim's scale is applied). Stored
    // per-occluder so segmentHits can do triangle-soup raycast inside the OBB
    // pre-cull pass; populated in P15.2 by reading LLVolume::getVolumeFace.
    struct Tri
    {
        LLVector3 v0;
        LLVector3 v1;
        LLVector3 v2;
    };

    // r13 P15.1: extended container — bounding OBB (pre-cull) + (P15.2)
    // triangle list reflecting the prim's actual tessellated shape
    // (path cut / hollow / sculpt / mesh) + material (direct / reverb
    // factors fed to FMOD::Channel::set3DOcclusion). P15.1 ships the
    // new layout with tris empty, preserving the OBB-only raycast
    // semantics so the refactor is a behaviour-preserving rename.
    struct OccluderShape
    {
        OBB              obb;
        std::vector<Tri> tris;
        F32              direct = 0.7f;  // set3DOcclusion direct factor
        F32              reverb = 0.5f;  // set3DOcclusion reverb factor
    };

    // Per-channel ramp state. Smooths transitions so a door opening / closing
    // (occluder entering or leaving the segment) crossfades over
    // Stream3DOcclusionRampMs instead of jumping in one frame. Keyed by the
    // raw FMOD::Channel pointer; entries leak benignly when channels are
    // recycled (relog clears).
    struct Smoothing
    {
        F32 direct = 0.f;
        F32 reverb = 0.f;
    };

    bool firstHit(const LLVector3& a, const LLVector3& b,
                  F32& out_direct, F32& out_reverb) const;
    static bool segmentHitsOBB(const LLVector3& a, const LLVector3& b, const OBB& obb);

    // r13 P15.2: OBB pre-cull → triangle-soup raycast. When shape.tris is
    // empty (extraction skipped or LOD-evicted), falls back to segmentHitsOBB
    // so unpopulated occluders keep working as r13 P15.1 baseline.
    static bool segmentHitsShape(const LLVector3& a, const LLVector3& b,
                                 const OccluderShape& shape);

    // Walks the prim's LLVolume and copies triangle vertices into
    // shape.tris in OBB-local space (scale applied). Skipped silently for
    // prims without a volume (e.g. avatars). Triangles exceeding
    // kMaxTrisPerOccluder cause a one-shot fallback to OBB-only.
    static void extractTriangles(LLViewerObject* obj, OccluderShape& shape);

    std::map<LLUUID, OccluderShape>   mOccluders;
    std::map<FMOD::Channel*, Smoothing> mSmoothing;
    F64                               mLastTickTime = 0.0;
    F32                               mTickDt = 0.f;
};

#endif // LL_OCCLUSIONGEOMETRYMGR_H
