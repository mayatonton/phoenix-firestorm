/**
 * @file llocclusiongeometrymgr.cpp
 * @brief AYAstorm r13 spike: see header for design intent.
 */

#include "linden_common.h"
#include "llocclusiongeometrymgr.h"

#include "fmodstudio/fmod.hpp"

#include "llgl.h"
#include "llpositionalstreammgr.h"
#include "llrender.h"
#include "llstring.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewercontrol.h"
#include "lltimer.h"
#include "llvolume.h"
#include "v3dmath.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace
{
    // Tag prefix. Bare `[ayastorm:occlude]` means defaults; the full form
    // `[ayastorm:occlude{direct:0.7}{reverb:0.5}]` overrides per prim.
    // Separator within `{...}` is `:` to match the rest of the AYAstorm
    // tag family (`{upmix:on}`, `{ch:FL}`, `{venue:hall_medium}`, etc.).
    // Matched case-insensitively; see parseOccludeTag().
    constexpr const char* kOccludePrefix = "[ayastorm:occlude";

    // Defaults applied when the parameterized form omits a field, or when
    // the bare form is used.
    constexpr F32 kDefaultDirect = 0.7f;
    constexpr F32 kDefaultReverb = 0.5f;

    // r13 final cap. Spec §4.7 originally suggested 200; bumped to 256 for a
    // power-of-two cap with ~25% headroom over typical SL venue counts (~100
    // occluder prims). Cost is still O(N × N_channels × frame_rate) but at
    // 256 × 64 channels × 60 Hz ≈ 1 M slab tests/sec — comfortably under
    // 1 ms/sec on modern CPUs (each test is a few mul/cmp).
    constexpr int kMaxOccluders = 256;

    // r13 P15.2 cap on per-occluder triangle count. Typical SL building
    // prims tessellate to <200 tris; mesh prims can reach ~21k but that's
    // a pathological case for occlusion (would also be a poor acoustic
    // panel). When a prim exceeds the cap we fall back to OBB-only (leave
    // tris empty) and warn once per registration — better than partial
    // mesh which would produce inconsistent "some holes detected, others
    // not" behaviour.
    constexpr int kMaxTrisPerOccluder = 2000;

    // r13 P15.9: per-tick cap on triangle extracts drained from the pending
    // queue. TP/login bursts can deliver ~100 tagged prims in 1-2 frames; at
    // worst a 2000-tri mesh prim takes ~1-2 ms to walk, so a synchronous
    // burst can hitch 100-200 ms. 6/tick spreads a 100-prim burst over ~17
    // ticks (~0.3 s at 60 Hz) — invisible to the user. OBB pre-cull still
    // works on pending entries (empty tris ⇒ segmentHitsShape returns the
    // OBB result), so audio is correct from the first tick.
    constexpr int kExtractPerTickBudget = 6;

    LLVector3 toFloatVec(const LLVector3d& v)
    {
        return LLVector3(static_cast<F32>(v.mdV[0]),
                         static_cast<F32>(v.mdV[1]),
                         static_cast<F32>(v.mdV[2]));
    }

    // ASCII case-insensitive substring search. Matches the convention used
    // in llpositionalstreammgr.cpp (r5-r12 tag family) so `[Ayastorm:Occlude]`
    // typed by a building owner parses identically to the canonical form.
    size_t findCaseInsensitive(const std::string& haystack, const std::string& needle)
    {
        if (needle.empty() || haystack.size() < needle.size()) return std::string::npos;
        const size_t end = haystack.size() - needle.size();
        for (size_t i = 0; i <= end; ++i)
        {
            bool match = true;
            for (size_t j = 0; j < needle.size(); ++j)
            {
                const unsigned char a = static_cast<unsigned char>(haystack[i + j]);
                const unsigned char b = static_cast<unsigned char>(needle[j]);
                if (std::tolower(a) != std::tolower(b)) { match = false; break; }
            }
            if (match) return i;
        }
        return std::string::npos;
    }

    std::string toLowerAscii(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    // Walk every well-formed `{key:value}` block within [content_start, end)
    // of `desc` and invoke `onPair(lowered_key, trimmed_val)`. Anything between
    // blocks (whitespace, separators, or junk) is ignored, matching the
    // r5-r12 tag family's tolerance. Unknown keys are silently dropped by
    // the caller (= spec §4.1 "未知タグ silent ignore").
    template <typename F>
    void forEachKeyValue(const std::string& desc, size_t content_start, size_t end, F&& onPair)
    {
        size_t cursor = content_start;
        while (cursor < end)
        {
            const size_t ob = desc.find('{', cursor);
            if (ob == std::string::npos || ob >= end) break;
            const size_t cb = desc.find('}', ob + 1);
            if (cb == std::string::npos || cb > end) break;

            std::string inner = desc.substr(ob + 1, cb - ob - 1);
            cursor = cb + 1;

            const size_t colon = inner.find(':');
            if (colon == std::string::npos) continue;

            std::string key = inner.substr(0, colon);
            std::string val = inner.substr(colon + 1);
            LLStringUtil::trim(key);
            LLStringUtil::trim(val);
            key = toLowerAscii(key);

            onPair(key, val);
        }
    }

    bool tryParseFloat(const std::string& s, F32& out)
    {
        if (s.empty()) return false;
        try
        {
            size_t consumed = 0;
            const F32 v = std::stof(s, &consumed);
            if (consumed == 0) return false;
            out = v;
            return true;
        }
        catch (const std::exception&) { return false; }
    }

    // Returns true iff the description carries the occlude tag. Populates
    // direct/reverb with parsed values (or defaults when fields are absent).
    // Format rules follow the r5-r12 tag family (case-insensitive prefix /
    // key, value whitespace-trimmed, unknown keys silent-ignored). See
    // spec §4.1 / §4.4 for the per-prim override semantics.
    bool parseOccludeTag(const std::string& desc, F32& direct, F32& reverb)
    {
        direct = kDefaultDirect;
        reverb = kDefaultReverb;

        const std::string prefix = kOccludePrefix;
        const size_t p = findCaseInsensitive(desc, prefix);
        if (p == std::string::npos) return false;

        const size_t after = p + prefix.size();
        if (after > desc.size()) return false;

        // Disambiguate from hypothetical sibling tags that would extend the
        // prefix with letters/digits (e.g. `[ayastorm:occluder]`). The spec
        // only defines `[ayastorm:occlude]` and `[ayastorm:occlude{...}]`,
        // so anything other than ']', '{', or trailing whitespace right
        // after the prefix is rejected.
        size_t scan = after;
        while (scan < desc.size() && std::isspace(static_cast<unsigned char>(desc[scan]))) ++scan;
        if (scan >= desc.size()) return false;
        const char c = desc[scan];
        if (c != ']' && c != '{') return false;

        const size_t end = desc.find(']', scan);
        if (end == std::string::npos) return false;

        forEachKeyValue(desc, scan, end,
            [&](const std::string& key, const std::string& val)
            {
                F32 v = 0.f;
                if (key == "direct" && tryParseFloat(val, v)) direct = llclamp(v, 0.f, 1.f);
                else if (key == "reverb" && tryParseFloat(val, v)) reverb = llclamp(v, 0.f, 1.f);
            });
        return true;
    }
}

LLOcclusionGeometryMgr::LLOcclusionGeometryMgr() = default;
LLOcclusionGeometryMgr::~LLOcclusionGeometryMgr() = default;

void LLOcclusionGeometryMgr::onObjectPropertiesReceived(const LLUUID& id,
                                                        const std::string& description)
{
    F32 direct, reverb;
    const bool tagged = parseOccludeTag(description, direct, reverb);

    // Tag-absent path: do NOT auto-unregister. ObjectPropertiesFamily for
    // child prims (and some other code paths) can deliver an empty / partial
    // Description, which would otherwise spuriously erase live entries. The
    // refresh pass already drops dead prims; tag removal mid-session is an
    // acceptable miss for the r13 spike (relog clears state).
    if (!tagged) return;

    LLViewerObject* obj = gObjectList.findObject(id);
    if (!obj || obj->isDead()) return;

    // r13 P14: tagged-root linksets nudge their children's Description into
    // the cache so child occluders register without a user touch. Sim filters
    // Description from ObjectPropertiesFamily for child prims; this select
    // bootstrap is the same workaround [3dstream-stereo:...] speaker scan
    // uses in evaluateLinkset. Idempotent — the helper dedups via
    // mPendingChildDeselect, so we don't need to track scanned roots.
    if (obj->isRoot())
    {
        LLPositionalStreamMgr::instance().bootstrapChildDescriptions(obj);
    }

    auto it = mOccluders.find(id);

    if (it == mOccluders.end())
    {
        if (mOccluders.size() >= static_cast<size_t>(kMaxOccluders))
        {
            LL_WARNS("Stream3D") << "[ayastorm:occlude] cap (" << kMaxOccluders
                                  << ") reached, skipping " << id << LL_ENDL;
            return;
        }
        OccluderShape shape;
        shape.obb.center = toFloatVec(obj->getPositionGlobal());
        shape.obb.half   = obj->getScale() * 0.5f;
        shape.obb.rot    = obj->getRotationRegion();
        shape.direct     = direct;
        shape.reverb     = reverb;
        // r13 P15.9 (A): defer extractTriangles to refreshOccluders' per-tick
        // drain. A TP/login burst of N tagged prims would otherwise call
        // extract synchronously N times on the main thread; mesh prims near
        // the 2000-tri cap can hitch 100-200 ms total. Pre-cull works on
        // empty tris (segmentHitsShape returns the OBB hit), so audio is
        // correct from frame 1 — just shape-less (no path-cut openings)
        // until the queue catches up, typically under 0.5 s.
        mOccluders[id]   = std::move(shape);
        mPendingExtract.insert(id);
        const OccluderShape& reg = mOccluders[id];
        LL_INFOS("Stream3D") << "[ayastorm:occlude] registered prim " << id
                              << " at " << reg.obb.center
                              << " half=" << reg.obb.half
                              << " direct=" << direct << " reverb=" << reverb
                              << " (extract queued, count=" << mOccluders.size() << ")" << LL_ENDL;
    }
    else
    {
        // r13 P15.9 (B): pick up Desc edits (direct/reverb) and the current
        // transform unconditionally, but only queue a re-extract when the
        // shape might have changed. TP often re-delivers the same Desc with
        // the same scale; re-running extractTriangles there wastes ~50us-2ms
        // without altering the raycast. Build-floater live edits are still
        // caught by the isSelected() path in refreshOccluders. Re-extract
        // also fires when we registered OBB-only (tris empty — e.g. mesh
        // not yet loaded at first Desc arrival) so the next Desc round-trip
        // gives us another chance.
        const LLVector3 new_half = obj->getScale() * 0.5f;
        const bool scale_changed = (new_half - it->second.obb.half).lengthSquared() > 1e-8f;
        it->second.obb.center = toFloatVec(obj->getPositionGlobal());
        it->second.obb.half   = new_half;
        it->second.obb.rot    = obj->getRotationRegion();
        it->second.direct     = direct;
        it->second.reverb     = reverb;
        if (scale_changed || it->second.tris.empty())
        {
            mPendingExtract.insert(id);
        }
    }
}

void LLOcclusionGeometryMgr::refreshOccluders()
{
    // Update tick dt regardless of whether occluders are present, so the
    // ramp logic in applyToChannel sees a sane delta on the very first
    // frame an occluder appears.
    const F64 now = LLTimer::getTotalSeconds();
    if (mLastTickTime <= 0.0)
    {
        mTickDt = 0.f;
    }
    else
    {
        mTickDt = static_cast<F32>(now - mLastTickTime);
    }
    mLastTickTime = now;

    if (mOccluders.empty()) return;

    // LLViewerObject does not cache Description, so we cannot re-parse
    // direct/reverb here. That's fine: Desc edits round-trip through the
    // sim as ObjectProperties, which lands in onObjectPropertiesReceived
    // and updates the cached values directly. This pass only refreshes the
    // per-frame-mutable transform (position/rotation/scale) and drops dead
    // prims.
    for (auto it = mOccluders.begin(); it != mOccluders.end(); )
    {
        LLViewerObject* obj = gObjectList.findObject(it->first);
        if (!obj || obj->isDead())
        {
            mPendingExtract.erase(it->first);
            it = mOccluders.erase(it);
            continue;
        }
        it->second.obb.center = toFloatVec(obj->getPositionGlobal());
        const LLVector3 new_half = obj->getScale() * 0.5f;
        const bool scale_changed = (new_half - it->second.obb.half).lengthSquared() > 1e-8f;
        it->second.obb.half = new_half;
        it->second.obb.rot  = obj->getRotationRegion();
        // r13 P15.5: re-extract while the prim is selected so Path Cut /
        // Hollow / Sculpt edits in the build floater reflect live in both
        // the cyan overlay and the audio raycast. Without this, mid-edit
        // changes only round-trip via ObjectProperties after the edit
        // window closes (sim doesn't broadcast every drag). Selection is
        // typically 1-3 prims, so the per-tick re-extract cost stays in
        // the μs range even at the 2000-tri cap.
        if (scale_changed || obj->isSelected())
        {
            extractTriangles(obj, it->second);
            // P15.9: this path already produced fresh tris; drop any pending
            // entry so the drain loop below doesn't redo the same work.
            mPendingExtract.erase(it->first);
        }
        ++it;
    }

    // r13 P15.9 (A): drain a few queued triangle extracts per tick to
    // smooth out TP/login bursts. Stale UUIDs (prim died or got
    // un-tagged before its turn) are dropped silently. The inline
    // scale_changed / isSelected path above may have already extracted
    // for some entries — those were erased from the queue there, so we
    // only do real work here for prims that registered OBB-only and
    // haven't been touched since.
    int budget = kExtractPerTickBudget;
    for (auto pit = mPendingExtract.begin();
         pit != mPendingExtract.end() && budget > 0; )
    {
        const LLUUID id = *pit;
        auto oit = mOccluders.find(id);
        if (oit == mOccluders.end())
        {
            pit = mPendingExtract.erase(pit);
            continue;
        }
        LLViewerObject* obj = gObjectList.findObject(id);
        if (!obj || obj->isDead())
        {
            pit = mPendingExtract.erase(pit);
            continue;
        }
        extractTriangles(obj, oit->second);
        --budget;
        pit = mPendingExtract.erase(pit);
    }
}

bool LLOcclusionGeometryMgr::firstHit(const LLVector3& a, const LLVector3& b,
                                       F32& out_direct, F32& out_reverb) const
{
    // Multiplicative pass-through accumulation: each hit wall contributes
    // (1 - direct) to the surviving direct path, so two walls of 0.7 yield
    // an effective 0.91 (stacks intuitively). reverb behaves the same.
    bool any = false;
    F32 pass_d = 1.f;
    F32 pass_r = 1.f;
    for (const auto& kv : mOccluders)
    {
        if (segmentHitsShape(a, b, kv.second))
        {
            any = true;
            pass_d *= (1.f - kv.second.direct);
            pass_r *= (1.f - kv.second.reverb);
        }
    }
    if (any)
    {
        out_direct = 1.f - pass_d;
        out_reverb = 1.f - pass_r;
    }
    return any;
}

// static
bool LLOcclusionGeometryMgr::segmentHitsOBB(const LLVector3& a, const LLVector3& b, const OBB& obb)
{
    // Transform segment endpoints into the OBB's local frame so the test
    // reduces to segment-vs-AABB. ~q is the conjugate, which equals the
    // inverse for unit quaternions (which getRotationRegion returns).
    const LLQuaternion inv_rot = ~obb.rot;
    const LLVector3 la = (a - obb.center) * inv_rot;
    const LLVector3 lb = (b - obb.center) * inv_rot;
    const LLVector3 d  = lb - la;

    F32 t_min = 0.0f;
    F32 t_max = 1.0f;
    for (int i = 0; i < 3; ++i)
    {
        const F32 di = d.mV[i];
        const F32 pi = la.mV[i];
        const F32 h  = obb.half.mV[i];
        if (fabsf(di) < 1e-6f)
        {
            if (pi < -h || pi > h) return false;
            continue;
        }
        F32 t1 = (-h - pi) / di;
        F32 t2 = ( h - pi) / di;
        if (t1 > t2) std::swap(t1, t2);
        if (t1 > t_min) t_min = t1;
        if (t2 < t_max) t_max = t2;
        if (t_min > t_max) return false;
    }
    return true;
}

namespace
{
    // Möller–Trumbore segment-triangle intersection. la/lb are the segment
    // endpoints in the same local frame as v0/v1/v2 (OBB-local for our
    // pipeline). Returns true iff the segment crosses the triangle plane
    // strictly between the endpoints (t ∈ [0,1]). Backface-agnostic: we
    // care about occlusion, not winding.
    bool segmentHitsTriangle(const LLVector3& la, const LLVector3& lb,
                             const LLVector3& v0, const LLVector3& v1, const LLVector3& v2)
    {
        constexpr F32 kEps = 1e-7f;
        const LLVector3 d  = lb - la;
        const LLVector3 e1 = v1 - v0;
        const LLVector3 e2 = v2 - v0;
        const LLVector3 p  = d % e2;
        const F32 det = e1 * p;
        if (fabsf(det) < kEps) return false;
        const F32 inv_det = 1.f / det;

        const LLVector3 tvec = la - v0;
        const F32 u = (tvec * p) * inv_det;
        if (u < 0.f || u > 1.f) return false;

        const LLVector3 q = tvec % e1;
        const F32 v = (d * q) * inv_det;
        if (v < 0.f || u + v > 1.f) return false;

        const F32 t = (e2 * q) * inv_det;
        return t >= 0.f && t <= 1.f;
    }
}

// static
bool LLOcclusionGeometryMgr::segmentHitsShape(const LLVector3& a, const LLVector3& b,
                                              const OccluderShape& shape)
{
    // OBB pre-cull rejects ~95% of mismatched (segment, occluder) pairs
    // with a few mul/cmp. Triangle iteration only runs for the small
    // fraction that pass.
    if (!segmentHitsOBB(a, b, shape.obb)) return false;

    // Empty tris (extraction skipped / over-cap / non-volume prim): the
    // OBB pass already counted as a hit, preserving P15.1 OBB-only
    // behaviour for occluders we couldn't tessellate.
    if (shape.tris.empty()) return true;

    // Transform segment to OBB-local space once, then test every triangle
    // in the same space (tris are stored OBB-local-with-scale at populate
    // time so we don't redo the scale per ray).
    const LLQuaternion inv_rot = ~shape.obb.rot;
    const LLVector3 la = (a - shape.obb.center) * inv_rot;
    const LLVector3 lb = (b - shape.obb.center) * inv_rot;
    for (const auto& tri : shape.tris)
    {
        if (segmentHitsTriangle(la, lb, tri.v0, tri.v1, tri.v2)) return true;
    }
    return false;
}

// static
void LLOcclusionGeometryMgr::extractTriangles(LLViewerObject* obj, OccluderShape& shape)
{
    shape.tris.clear();
    if (!obj) return;

    LLVolume* vol = obj->getVolume();
    if (!vol) return;

    // Walk every face, count triangles first so we can early-out before
    // any allocation when over the cap.
    const S32 num_faces = vol->getNumVolumeFaces();
    S32 total_tris = 0;
    for (S32 f = 0; f < num_faces; ++f)
    {
        total_tris += vol->getVolumeFace(f).mNumIndices / 3;
    }
    if (total_tris == 0) return;
    if (total_tris > kMaxTrisPerOccluder)
    {
        LL_WARNS_ONCE("Stream3D") << "[ayastorm:occlude] prim " << obj->getID()
                                   << " has " << total_tris << " tris (cap "
                                   << kMaxTrisPerOccluder << "), falling back to OBB-only"
                                   << LL_ENDL;
        return;
    }

    // LLVolume positions are normalised to prim shape space (typically
    // -0.5..0.5 for a default cube). Multiplying per-axis by the world
    // scale brings us into OBB-local space where the OBB half-extents
    // are obb.half — same frame segmentHitsOBB tests in, so triangle
    // and pre-cull share coordinates with zero extra transform per ray.
    const LLVector3 scale = obj->getScale();
    shape.tris.reserve(total_tris);
    for (S32 f = 0; f < num_faces; ++f)
    {
        const LLVolumeFace& face = vol->getVolumeFace(f);
        if (!face.mPositions || !face.mIndices) continue;
        for (S32 i = 0; i + 2 < face.mNumIndices; i += 3)
        {
            const F32* p0 = face.mPositions[face.mIndices[i + 0]].getF32ptr();
            const F32* p1 = face.mPositions[face.mIndices[i + 1]].getF32ptr();
            const F32* p2 = face.mPositions[face.mIndices[i + 2]].getF32ptr();
            Tri t;
            t.v0.set(p0[0] * scale.mV[0], p0[1] * scale.mV[1], p0[2] * scale.mV[2]);
            t.v1.set(p1[0] * scale.mV[0], p1[1] * scale.mV[1], p1[2] * scale.mV[2]);
            t.v2.set(p2[0] * scale.mV[0], p2[1] * scale.mV[1], p2[2] * scale.mV[2]);
            shape.tris.push_back(t);
        }
    }
}

void LLOcclusionGeometryMgr::applyToChannel(FMOD::Channel* channel,
                                            FMOD::DSP* lowpass,
                                            const LLVector3& listener,
                                            const LLVector3& source)
{
    if (!channel) return;

    // Compute target factors: 0/0 when no occluders, master sentinel off,
    // out of range, or no segment hit; otherwise the strongest direct/reverb
    // among hit OBBs.
    //
    // Stream3DOcclusion (S32, -1 default = enabled, 0 = disabled, other =
    // enabled) is the master sentinel for live-toggling the whole feature.
    // When disabled we keep running smoothing + DSP push on the 0/0 target,
    // so toggling the setting mid-play ramps the cutoff back to bypass over
    // Stream3DOcclusionRampMs instead of a hard cliff.
    //
    // The distance cull (Stream3DOccluderRange, default 64 m) is a finer
    // optimisation that skips firstHit when the source is far enough that
    // audibility is already negligible from distance attenuation alone.
    F32 target_d = 0.f;
    F32 target_r = 0.f;
    bool hit = false;
    bool in_range = true;
    const S32 master = gSavedSettings.getS32("Stream3DOcclusion");
    const bool occlusion_enabled = (master != 0);
    if (occlusion_enabled && !mOccluders.empty())
    {
        const F32 range = gSavedSettings.getF32("Stream3DOccluderRange");
        if (range > 0.f)
        {
            const F32 dist_sq = (source - listener).lengthSquared();
            in_range = dist_sq <= range * range;
        }
        if (in_range)
        {
            hit = firstHit(listener, source, target_d, target_r);
        }
    }

    // Linear ramp from current → target so a wall entering / leaving the
    // segment crossfades over Stream3DOcclusionRampMs (default 250 ms)
    // instead of jumping in one frame. ramp_ms == 0 disables smoothing.
    Smoothing& sm = mSmoothing[channel];
    const F32 ramp_ms = gSavedSettings.getF32("Stream3DOcclusionRampMs");
    if (ramp_ms <= 0.f || mTickDt <= 0.f)
    {
        sm.direct = target_d;
        sm.reverb = target_r;
    }
    else
    {
        const F32 max_step = mTickDt * (1000.f / ramp_ms);
        auto step = [max_step](F32 cur, F32 tgt)
        {
            const F32 d = tgt - cur;
            if (d > max_step)  return cur + max_step;
            if (d < -max_step) return cur - max_step;
            return tgt;
        };
        sm.direct = step(sm.direct, target_d);
        sm.reverb = step(sm.reverb, target_r);
    }
    channel->set3DOcclusion(sm.direct, sm.reverb);

    // r13: muffle the post-pan signal by lowering the LOWPASS_SIMPLE cutoff
    // as direct increases. Exponential mapping from 22 kHz (= effective
    // bypass at direct=0) to 300 Hz (= heavy muffle at direct=1) keeps
    // perceived steps even in dB-space. Skip the push when no DSP was
    // attached for this speaker (creation failed) — set3DOcclusion alone
    // still attenuates volume.
    F32 cutoff_hz = 22000.f;
    if (lowpass)
    {
        constexpr F32 kCutoffMaxHz = 22000.f;
        constexpr F32 kCutoffMinHz = 300.f;
        cutoff_hz = kCutoffMaxHz * std::pow(kCutoffMinHz / kCutoffMaxHz, sm.direct);
        lowpass->setParameterFloat(FMOD_DSP_LOWPASS_SIMPLE_CUTOFF, cutoff_hz);
    }

    // Throttled spike diagnostic (~2s per process).
    static F64 s_last_log = 0.0;
    const F64 now = LLTimer::getTotalSeconds();
    if (now - s_last_log > 2.0)
    {
        s_last_log = now;
        const F32 first_d = mOccluders.empty() ? -1.f : mOccluders.begin()->second.direct;
        const F32 first_r = mOccluders.empty() ? -1.f : mOccluders.begin()->second.reverb;
        LL_INFOS("Stream3D") << "[ayastorm:occlude] tick listener=" << listener
                              << " source=" << source
                              << " occluders=" << mOccluders.size()
                              << " first_d/r=" << first_d << "/" << first_r
                              << " master=" << master
                              << " in_range=" << (in_range ? 1 : 0)
                              << " hit=" << (hit ? 1 : 0)
                              << " target_d/r=" << target_d << "/" << target_r
                              << " applied_d/r=" << sm.direct << "/" << sm.reverb
                              << " cutoff_hz=" << cutoff_hz
                              << " dt=" << mTickDt
                              << LL_ENDL;
    }
}

void LLOcclusionGeometryMgr::renderDebug() const
{
    if (mOccluders.empty()) return;

    // Per-triangle outward halo: push each vertex along the triangle's
    // face normal so the cyan layer doesn't sink into the prim surface
    // it's coplanar with (z-fight). Direction is forced outward from the
    // OBB centre so it works regardless of LLVolume's triangle winding.
    // Render-only — segmentHitsShape still uses the un-offset tris.
    constexpr F32 kHaloPad = 0.02f;
    auto offsetOf = [](const Tri& tri) -> LLVector3
    {
        LLVector3 n = (tri.v1 - tri.v0) % (tri.v2 - tri.v0);
        const F32 len = n.length();
        if (len < 1e-6f) return LLVector3::zero;
        n *= (kHaloPad / len);
        const LLVector3 centroid = (tri.v0 + tri.v1 + tri.v2) * (1.f / 3.f);
        if (n * centroid < 0.f) n = -n;
        return n;
    };

    LLGLEnable blend(GL_BLEND);
    LLGLDisable cull(GL_CULL_FACE);
    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    // First pass: translucent cyan fill so the volume reads at a glance,
    // including from inside (cull off). Depth test stays on so fills of
    // different occluders sort naturally; alpha keeps overlaps legible.
    gGL.begin(LLRender::TRIANGLES);
    for (const auto& kv : mOccluders)
    {
        const OccluderShape& shape = kv.second;
        if (shape.tris.empty()) continue;
        LLViewerObject* obj = gObjectList.findObject(kv.first);
        if (!obj || obj->isDead()) continue;
        const LLVector3 center = obj->getPositionAgent();
        const OBB& obb = shape.obb;
        gGL.color4f(0.f, 1.f, 1.f, 0.25f);
        for (const auto& tri : shape.tris)
        {
            const LLVector3 off = offsetOf(tri);
            const LLVector3 w0 = (tri.v0 + off) * obb.rot + center;
            const LLVector3 w1 = (tri.v1 + off) * obb.rot + center;
            const LLVector3 w2 = (tri.v2 + off) * obb.rot + center;
            gGL.vertex3fv(w0.mV);
            gGL.vertex3fv(w1.mV);
            gGL.vertex3fv(w2.mV);
        }
    }
    gGL.end();
    gGL.flush();

    // Second pass: solid cyan triangle edges on top of the fill. Depth
    // test off so the outline stays sharp even when the offset isn't
    // quite enough to clear a textured / shiny prim face.
    LLGLDisable depth(GL_DEPTH_TEST);
    gGL.begin(LLRender::LINES);
    for (const auto& kv : mOccluders)
    {
        const OccluderShape& shape = kv.second;
        if (shape.tris.empty()) continue;
        LLViewerObject* obj = gObjectList.findObject(kv.first);
        if (!obj || obj->isDead()) continue;
        const LLVector3 center = obj->getPositionAgent();
        const OBB& obb = shape.obb;
        gGL.color4f(0.f, 1.f, 1.f, 1.f);
        for (const auto& tri : shape.tris)
        {
            const LLVector3 off = offsetOf(tri);
            const LLVector3 w0 = (tri.v0 + off) * obb.rot + center;
            const LLVector3 w1 = (tri.v1 + off) * obb.rot + center;
            const LLVector3 w2 = (tri.v2 + off) * obb.rot + center;
            gGL.vertex3fv(w0.mV); gGL.vertex3fv(w1.mV);
            gGL.vertex3fv(w1.mV); gGL.vertex3fv(w2.mV);
            gGL.vertex3fv(w2.mV); gGL.vertex3fv(w0.mV);
        }
    }
    gGL.end();
    gGL.flush();
}
