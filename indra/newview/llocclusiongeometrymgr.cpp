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
        OBB obb;
        obb.center = toFloatVec(obj->getPositionGlobal());
        obb.half   = obj->getScale() * 0.5f;
        obb.rot    = obj->getRotationRegion();
        obb.direct = direct;
        obb.reverb = reverb;
        mOccluders[id] = obb;
        LL_INFOS("Stream3D") << "[ayastorm:occlude] registered prim " << id
                              << " at " << obb.center
                              << " half=" << obb.half
                              << " direct=" << direct << " reverb=" << reverb
                              << " (count=" << mOccluders.size() << ")" << LL_ENDL;
    }
    else
    {
        // Existing entry — pick up Desc edits (direct/reverb) and current
        // transform in one pass.
        it->second.center = toFloatVec(obj->getPositionGlobal());
        it->second.half   = obj->getScale() * 0.5f;
        it->second.rot    = obj->getRotationRegion();
        it->second.direct = direct;
        it->second.reverb = reverb;
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
            it = mOccluders.erase(it);
            continue;
        }
        it->second.center = toFloatVec(obj->getPositionGlobal());
        it->second.half   = obj->getScale() * 0.5f;
        it->second.rot    = obj->getRotationRegion();
        ++it;
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
        if (segmentHitsOBB(a, b, kv.second))
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

    // 12 edges of a box, indexed into the 8-corner array below. Corner
    // index encodes ±half on each axis: bit 0 = X, bit 1 = Y, bit 2 = Z.
    static constexpr int kEdges[12][2] = {
        {0,1},{2,3},{4,5},{6,7},  // X edges
        {0,2},{1,3},{4,6},{5,7},  // Y edges
        {0,4},{1,5},{2,6},{3,7},  // Z edges
    };

    // 6 faces of a box, two triangles each (12 tris total). Each face
    // shares one fixed-axis sign; the other two axes traverse the four
    // corners of that face. Wound CCW when viewed from outside, but back
    // -face culling is disabled below so winding is purely cosmetic.
    static constexpr int kFaces[12][3] = {
        // -X face
        {0,2,6}, {0,6,4},
        // +X face
        {1,5,7}, {1,7,3},
        // -Y face
        {0,4,5}, {0,5,1},
        // +Y face
        {2,3,7}, {2,7,6},
        // -Z face
        {0,1,3}, {0,3,2},
        // +Z face
        {4,6,7}, {4,7,5},
    };

    // First pass: filled translucent faces so the OBB volume is obvious
    // even when the wireframe overlaps real geometry. Blend on, depth
    // write off so faces of different OBBs sort visually without
    // committing to a depth order. Cull off so the box is solid from any
    // viewing angle.
    //
    // Inflation halo: render the box ~5 cm larger than the prim on every
    // axis. The OBB cache itself stores the exact prim half-extent (used
    // by segmentHitsOBB() — must stay tight), but coplanar visualization
    // z-fights against the actual prim faces, which made the wireframe
    // / fill effectively invisible against the underlying prim. The halo
    // is render-only and never feeds back into the occlusion math.
    constexpr F32 kHaloPad = 0.05f;
    LLGLEnable blend(GL_BLEND);
    LLGLDisable cull(GL_CULL_FACE);
    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    gGL.begin(LLRender::TRIANGLES);
    for (const auto& kv : mOccluders)
    {
        // Re-fetch the prim so we render in agent-region space rather
        // than the global frame the OBB cache holds. Cheap: bounded by
        // the spike's 64-occluder cap and lookup is O(log N) on the
        // viewer object map. Skip if the prim went away between
        // refreshOccluders() ticks (entry will be reaped on the next
        // refresh anyway).
        LLViewerObject* obj = gObjectList.findObject(kv.first);
        if (!obj || obj->isDead()) continue;
        const LLVector3 center = obj->getPositionAgent();

        const OBB& obb = kv.second;
        // Colour scales with direct: orange (registered) → red (heavy).
        // direct is already clamped to [0,1] by the parser. Fill alpha
        // ~25 % so multiple overlapping OBBs stay individually legible.
        const F32 cr = 1.f;
        const F32 cg = 0.6f * (1.f - obb.direct);
        const F32 cb = 0.f;
        gGL.color4f(cr, cg, cb, 0.25f);

        LLVector3 corners[8];
        for (int i = 0; i < 8; ++i)
        {
            const F32 sx = (i & 1) ? 1.f : -1.f;
            const F32 sy = (i & 2) ? 1.f : -1.f;
            const F32 sz = (i & 4) ? 1.f : -1.f;
            const LLVector3 local(sx * (obb.half.mV[0] + kHaloPad),
                                  sy * (obb.half.mV[1] + kHaloPad),
                                  sz * (obb.half.mV[2] + kHaloPad));
            corners[i] = center + local * obb.rot;
        }
        for (const auto& f : kFaces)
        {
            gGL.vertex3fv(corners[f[0]].mV);
            gGL.vertex3fv(corners[f[1]].mV);
            gGL.vertex3fv(corners[f[2]].mV);
        }
    }
    gGL.end();
    gGL.flush();

    // Second pass: solid wireframe edges on top of the fill so the box
    // outline reads sharply (the 25 % fill alone is faint).
    gGL.begin(LLRender::LINES);
    for (const auto& kv : mOccluders)
    {
        LLViewerObject* obj = gObjectList.findObject(kv.first);
        if (!obj || obj->isDead()) continue;
        const LLVector3 center = obj->getPositionAgent();

        const OBB& obb = kv.second;
        const F32 cr = 1.f;
        const F32 cg = 0.6f * (1.f - obb.direct);
        const F32 cb = 0.f;
        gGL.color4f(cr, cg, cb, 1.f);

        LLVector3 corners[8];
        for (int i = 0; i < 8; ++i)
        {
            const F32 sx = (i & 1) ? 1.f : -1.f;
            const F32 sy = (i & 2) ? 1.f : -1.f;
            const F32 sz = (i & 4) ? 1.f : -1.f;
            const LLVector3 local(sx * (obb.half.mV[0] + kHaloPad),
                                  sy * (obb.half.mV[1] + kHaloPad),
                                  sz * (obb.half.mV[2] + kHaloPad));
            corners[i] = center + local * obb.rot;
        }
        for (const auto& e : kEdges)
        {
            gGL.vertex3fv(corners[e[0]].mV);
            gGL.vertex3fv(corners[e[1]].mV);
        }
    }
    gGL.end();
    gGL.flush();
}
