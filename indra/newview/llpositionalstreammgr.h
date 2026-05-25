/**
 * @file llpositionalstreammgr.h
 * @brief Manager for prim-bound 3D positional audio streams (AYAstorm).
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Phoenix Firestorm Project, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * $/LicenseInfo$
 */

#ifndef LL_POSITIONAL_STREAM_MGR_H
#define LL_POSITIONAL_STREAM_MGR_H

#include "lluuid.h"
#include "stdtypes.h"
#include "v3math.h"

#include <boost/signals2/connection.hpp>

#include <deque>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

class LLPositionalStream;
class LLPositionalStreamStereo;
class LLPositionalStreamMulti;
class LLViewerMediaImpl;
class LLViewerObject;

class LLPositionalStreamMgr
{
public:
    static LLPositionalStreamMgr& instance();

    // Per-frame: poll FMOD async opens, refresh positions of bound prims,
    // drop bindings whose object disappeared.
    void update();

    // Called from the message handlers that decode ObjectProperties /
    // ObjectPropertiesFamily. Caches the description, and (re)evaluates
    // whether this prim should be bound to a stream.
    void onObjectPropertiesReceived(const LLUUID& id, const std::string& description);

    // Apply a global default rolloff to all currently-active streams.
    // Per-prim tags that explicitly set min/max are NOT overridden.
    void applyDefaultRolloff(F32 default_min, F32 default_max);

    // M7: Apply the global master volume (0..1) to every active stream.
    // Driven by Stream3DVolumeMaster signal; new bindings also pick up the
    // current setting at bind time.
    void applyMasterVolume(F32 volume);

    // M8: Tear down every prim binding (mono + stereo) but leave debug streams
    // alone. Used when Stream3DDescriptionScan is toggled off.
    void shutdownPrimBindings();

    // M8: Tear down everything — prim bindings AND debug streams. Used when
    // the master Stream3DEnabled kill switch is toggled off.
    void shutdownAll();

    // r7: Reset every prim's last-polled stamp so the next scan tick treats
    // them all as un-polled and (re)issues ObjectPropertiesFamily requests.
    // Used when Stream3DEnabled is flipped back on, so we don't make the user
    // wait up to Stream3DPollInterval seconds before tagged prims rebind.
    void forceRescan();

    // r13: Force a one-shot ObjectSelect for every child of `root_obj` so the
    // sim emits full ObjectProperties (carrying Description) for them. The
    // sim filters Description from ObjectPropertiesFamily for *child* prims
    // (project_sim_objectproperties_family_child_filter), so without this
    // nudge a tagged child stays invisible until the user touches it.
    // Public so sibling subsystems (LLOcclusionGeometryMgr, etc.) can use the
    // same boot-time discovery path that [3dstream-stereo:...] speaker scan
    // already relies on internally.
    void bootstrapChildDescriptions(LLViewerObject* root_obj);

    // Called immediately before a MOAP media plugin source is destroyed.
    // Media-ring backed 3D streams must detach before the plugin shared
    // memory is unmapped; waiting for the next per-frame update leaves the
    // decode thread with a stale raw ring pointer.
    void onMediaSourceDestroying(LLViewerMediaImpl* media);

    // Debug toggle stream (driven by Stream3DDebugPlay). Independent of
    // the prim binding map.
    void startDebug(const std::string& url, const LLVector3& world_pos);
    void stopDebug();

    // Stereo debug stream (driven by Stream3DDebugStereoPlay, M5-a spike).
    // Pulls PCM from a source HTTP stream and feeds two OPENUSER 3D mono
    // sounds. In M5-a both sides receive the same downmixed signal.
    void startDebugStereo(const std::string& url,
                          const LLVector3& l_pos,
                          const LLVector3& r_pos);
    void stopDebugStereo();

    // Mono tag parsed from a prim Description ([3dstream:{...}]; legacy
    // [ayastream:{...}] also accepted).
    struct TagData
    {
        std::string url;
        std::optional<F32> min;
        std::optional<F32> max;
    };

    // Returns parsed [3dstream:{...}{...}] tag, or nullopt if none /
    // malformed / missing URL. (Legacy [ayastream:...] is also accepted.)
    static std::optional<TagData> parseTag(const std::string& description);

    // r8: distributed-description stereo (1 source → N speakers).
    // Channel assignment of one speaker prim.
    //
    // r9 (spec_5_1ch_source.md §4.6): renamed from DistChannel and split into
    // a standalone enum class so r10 can append FL/FR/C/LFE/SL/SR for 5.1
    // venue placement by editing this declaration + parseChannelKind() +
    // downmix table only — the parser, evaluator and reconfig switches are
    // generic over the value.
    enum class ChannelKind : U8
    {
        L = 0,
        R,
        M,
        // r10 (spec_5_1ch_placement.md §4.1): 5.1 venue placement values.
        // Compatibility matrix (§4.2) decides what each value plays per
        // source channel count — handled in the audio-side reader, not
        // here. r8/r9 viewers receive these as unknown and silent-ignore.
        FL,
        FR,
        C,
        LFE,
        SL,
        SR,
        BL,
        BR,
    };

    enum class DistSourceKind
    {
        Url,
        Media,
    };

    struct SourceBindingKey
    {
        DistSourceKind kind = DistSourceKind::Url;
        std::string url;
        LLUUID media_object_id;
        LLUUID media_id;
        S32 face = -1;
        S32 media_source_channels = 0;

        bool operator==(const SourceBindingKey& rhs) const
        {
            return kind == rhs.kind &&
                   url == rhs.url &&
                   media_object_id == rhs.media_object_id &&
                   media_id == rhs.media_id &&
                   face == rhs.face &&
                   media_source_channels == rhs.media_source_channels;
        }
    };

    // r8: parsed [3dstream-stereo:{url:...}{range:...}{ch:...}{volume:...}] tag.
    // AYAstorm media-source extension also accepts
    // {source:media}{link:N}{face:N}.
    // A single prim's description may declare:
    //   - source only (root):         {url} or {source:media} [+ {range}]
    //   - speaker only (root/child):  {ch}       [+ {range} + {volume}]
    //   - source + self-speaker:      source + {ch} [+ {range} + {volume}]
    // For {source:media}, the source declaration still lives on the root,
    // but the media face may live on the root or one child prim in the same
    // linkset. {link:N} narrows the search to one prim, and {face:N}
    // narrows it to one media face.
    // The same {range} field, when present, fills both range_default (source
    // role) and range_speaker (speaker role) of the same prim — the spec
    // §4.3 treats it as a single shared field rather than two separate keys.
    struct DistStereoTagData
    {
        // r31: true when this distributed tag was written with the canonical
        // [3dstream:...] prefix instead of the legacy-explicit
        // [3dstream-stereo:...] form. This lets [3dstream:{url:...}] remain
        // a mono fallback when no speaker {ch:...} exists in the linkset.
        bool unified_3dstream_prefix = false;

        // Source declaration fields (set when {url:...} or {source:media}
        // is present).
        std::optional<DistSourceKind> source_kind;
        std::optional<std::string> url;
        std::optional<S32> media_link;
        std::optional<S32> media_face;
        S32 media_source_channels = 0; // 0 = non-media/unset, otherwise 2/6/8
        std::optional<F32> range_default;
        // r11 P5: lite-HRTF toggle ({binaural:on|off}). Source-side property
        // — meaningful only on the root prim (= same prim as {url}).
        // nullopt = unspecified (defaults to on per spec §4.1.0).
        std::optional<bool> binaural;

        // r12 P5: 2ch→5.1 upmix toggle ({upmix:on|off}). Source-side
        // property — meaningful only on the root prim. nullopt =
        // unspecified (defaults to off per spec §4.1, line 110/138/147 —
        // upmix is opt-in because it changes the audible image of every
        // existing 2ch broadcast). Same parser shape as {binaural}.
        std::optional<bool> upmix;

        // r11 P8: venue selection ({venue:NAME}). Source-side property —
        // child prim values are silently ignored (spec §4.1 line 154).
        // nullopt = unspecified (defaults to "dry" per spec §4.1.1).
        // Validated at parse time against LLVenueReverbDsp::knownVenues();
        // unknown values land in bad_venue_value below instead of here so
        // the tag overall stays valid (spec §4.1 line 174 — silent-ignore +
        // chat warn, not full-tag reject).
        std::optional<std::string> venue;
        std::optional<std::string> bad_venue_value;

        // r11 P9: venue reverb wet/dry mix multiplier ({wetgain:N}).
        // Source-side property — child prim values are silently ignored
        // (same rule as {venue}). nullopt = unspecified (defaults to 1.0
        // per spec §4.1.0 line 152). Numeric values outside [0.0, 2.0]
        // are clamped at parse time (spec §4.1 line 152). Non-numeric
        // input is a full-tag-reject error (BadWetGain) like {volume}
        // and {binaural} — there is no separate "bad value" field.
        std::optional<F32> wetgain;

        // r12.1: LFE band gain multiplier ({lfegain:N}). Applied to the
        // LFE channel after the LPF in Upmix path, and to the LFE feed
        // in 5.1 native path. Source-side / root-only (child values
        // silently ignored, same as {wetgain}). nullopt = unspecified
        // (defaults to 1.0 = passthrough). [0.0, 3.0]; clamped at parse
        // time. Non-numeric input is full-tag-reject (BadLfeGain).
        std::optional<F32> lfegain;

        // Speaker declaration fields (set only when {ch:...} is present).
        std::optional<ChannelKind> ch;
        std::optional<F32> range_speaker;
        std::optional<F32> volume; // 0.0 .. 1.0
    };

    // r8: parse-time error categories. The error string carries the offending
    // input value (e.g. "X" for {ch:X}, "1.5" for {volume:1.5}) so the F4
    // throttled notifier can echo it back to the user.
    //
    // Note: `Ok` (not `None`) for the no-error sentinel — X11 headers define
    // `None` as a macro (0L), and llpositionalstreammgr.h is reached through
    // PCH paths that include X11/Xlib.h.
    enum class DistParseError
    {
        Ok,
        BadCh,        // {ch:...} value not L/R/M (case-insensitive)
        BadRange,     // {range:N} not > 0 or unparseable
        BadVolume,    // {volume:N} not in [0.0, 1.0] or unparseable
        EmptyUrl,     // {url:} empty
        // r11 P5: {binaural:...} value not on/off (case-insensitive)
        BadBinaural,
        // r11 P9: {wetgain:N} value not parseable as F32 (out-of-range
        // is clamped silently, not reported here)
        BadWetGain,
        // r12 P5: {upmix:...} value not on/off (case-insensitive). Same
        // shape as BadBinaural — the tag is full-rejected so a typo
        // doesn't silently fall back to "off" and confuse the publisher.
        BadUpmix,
        // r12.1: {lfegain:N} value not parseable as F32 (out-of-range
        // is clamped silently to [0.0, 3.0], not reported here).
        BadLfeGain,
        // {source:...} value not recognized. Initial supported value: media.
        BadSource,
        // {url:...} and {source:media} are mutually exclusive source
        // declarations.
        ConflictingSource,
        // {link:N} / {face:N} value not parseable as a non-negative integer.
        BadLink,
        BadFace,
    };

    struct DistParseResult
    {
        // data has value only when the tag has at least one of {url} /
        // {source} / {ch} and all present fields parse cleanly. nullopt
        // with error==Ok means "no recognizable [3dstream-stereo:...] tag at
        // all"; nullopt with error!=Ok means "tag present but a field is
        // malformed".
        std::optional<DistStereoTagData> data;
        DistParseError error = DistParseError::Ok;
        std::string bad_value;
    };

    // r8: parse the [3dstream-stereo:...] body for the distributed format.
    //
    // Returns:
    //   - tag absent              → {nullopt, Ok, ""}
    //   - tag present, all valid  → {data,    Ok, ""}
    //   - tag present, field bad  → {nullopt, <error>, bad_value}
    //
    // The tag is recognized when {url}, {source}, or {ch} is present. The
    // legacy {l:N}{r:N} format (r5–r7) is no longer supported in r8.
    static DistParseResult parseDistributedStereoTag(const std::string& description);

    // r9 (§4.6): {ch:...} value → enum. Case-insensitive ASCII match against
    // the L/R/M alphabet; returns nullopt for any unknown token. Surfaced as
    // a member so parser unit tests and r10's 5.1 placement code share the
    // exact same alphabet without the parser leaking its lowercase trick.
    static std::optional<ChannelKind> parseChannelKind(std::string_view s);

    // r11 P5: combine the publisher's {binaural:on|off} tag with the debug
    // override `Stream3DBinauralRender` (-1 sentinel = follow tag, 0 = force
    // OFF, 1+ = force ON) into the final on/off decision used at channel
    // bring-up time. Spec §4.1.0 / §6 — debug value wins when not the
    // sentinel; otherwise the tag value (or `true` if unspecified) is used.
    static bool effectiveBinaural(std::optional<bool> tag_value);

    // r11 P8: combine the publisher's {venue:NAME} tag with the debug
    // override `Stream3DVenueOverride` (empty = follow tag, non-empty =
    // force this venue) into the final name fed to LLVenueReverbDsp::
    // setVenue(). Spec §4.5 — debug value wins when non-empty; otherwise
    // the tag value (or "dry" if unspecified) is used. Returned name is
    // not re-validated against the catalog here; callers handle unknown
    // names via the setVenue return value (= notifies IRNotLoaded).
    static std::string effectiveVenue(const std::optional<std::string>& tag_value);

    // r11 P9: combine the publisher's {wetgain:N} tag with the debug
    // override `Stream3DVenueWetGain`. Spec §4.5 line 385 — debug value
    // wins when ≥ 0.0; otherwise the tag value (or 1.0 if unspecified)
    // is used. Result is clamped to [0.0, 2.0]. The DSP's internal range
    // is enforced by setWetGain() too, but clamping here keeps the
    // binding's recorded value coherent with what was actually pushed.
    static F32 effectiveWetGain(std::optional<F32> tag_value);

    // r12 P5: combine the publisher's {upmix:on|off} tag with the debug
    // override `Stream3DUpmix` (-1 sentinel = follow tag, 0 = force OFF,
    // 1+ = force ON) into the final on/off decision. Spec §4.1 / §6.3
    // — debug value wins when not the sentinel; otherwise the tag value
    // (or `false` if unspecified, because upmix is opt-in) is used.
    // Auto-bypass on >= 6ch native sources is enforced separately at
    // resolveReadOp dispatch time, not here.
    static bool effectiveUpmix(std::optional<bool> tag_value);

    // r12.1: combine the publisher's {lfegain:N} tag with the debug
    // override `Stream3DLfeGain`. Same shape as effectiveWetGain — debug
    // value wins when >= 0.0; otherwise the tag value (or 1.0 = pass-
    // through if unspecified) is used. Result is clamped to [0.0, 3.0].
    static F32 effectiveLfeGain(std::optional<F32> tag_value);

    // r34: render-side mouselook suppression must not hide or stall prims
    // participating in a live 3D Stream binding, especially source:media
    // surfaces whose audio ring depends on media/plugin updates.
    bool isStream3DPrimOrRoot(const LLUUID& id) const;

private:
    LLPositionalStreamMgr();
    ~LLPositionalStreamMgr();
    LLPositionalStreamMgr(const LLPositionalStreamMgr&) = delete;
    LLPositionalStreamMgr& operator=(const LLPositionalStreamMgr&) = delete;

    struct Binding
    {
        std::string url;
        // Values explicitly set by the prim's tag — nullopt means "fall
        // back to the global default" so global slider changes still apply.
        std::optional<F32> tag_min;
        std::optional<F32> tag_max;
        F32 applied_min = 1.f;
        F32 applied_max = 20.f;
        // M7: reconnect bookkeeping. reconnect_attempts counts consecutive
        // failed retries; reset to 0 on successful playback. next_retry_time
        // is the monotonic-seconds timestamp at which the next retry should
        // fire (0 = no retry pending).
        S32 reconnect_attempts = 0;
        F64 next_retry_time = 0.0;
        // M8: set true once the stream has reached Playing for the first time
        // since this Binding was created. Suppresses duplicate "now playing"
        // toasts on every reconnect-success transition (only the *initial*
        // bind notifies; rebinds inherit a fresh false because new Binding).
        bool notified_played = false;
        // r23: source prim is an attachment (= attached to an avatar). When
        // true, the source position moves per-frame and the parcel gate is
        // re-evaluated in update() every tick (Tier 2). When false, the
        // source is static so the gate is refreshed only on agent
        // parcel-change events (Tier 1) and cached here.
        bool is_attached = false;
        // r23: cached canHearSound() result for this binding's source vs.
        // the listener parcel. True = audible (default = audible so an
        // un-evaluated binding plays rather than silently muting).
        bool parcel_audible = true;
        // r23: idempotent guard for setVolume churn. The per-poll push
        // skips when the new effective volume equals the last pushed value.
        // Sentinel NaN means "never pushed", so the first push always goes.
        F32 last_pushed_volume = std::numeric_limits<F32>::quiet_NaN();
        std::unique_ptr<LLPositionalStream> stream;
    };

    // r8 F2-a: one entry per speaker prim that participates in a distributed
    // binding. `range` is the per-speaker resolved value (slot {range} → root
    // {range} → settings.xml fallback) — F3 reads it directly when calling
    // FMOD set3DMinMaxDistance, so the precedence resolution lives only in
    // evaluateLinkset, not in the streaming layer.
    struct SpeakerSlot
    {
        LLUUID prim_id;
        ChannelKind ch = ChannelKind::M;
        F32 range = 20.f;
        F32 volume = 1.f;
    };

    // r8 F2-a: aggregated linkset state for one distributed-stereo source.
    // Keyed by root_id in mDistributedBindings.
    struct DistributedStereoBinding
    {
        LLUUID root_id;
        SourceBindingKey source_key;
        std::string url;
        F32 range_default = 20.f;
        // r11 P5: publisher's {binaural:on|off} tag value (nullopt =
        // unspecified). Combined with the debug override at channel bring-up
        // via effectiveBinaural(). Tracked here so a fingerprint comparison
        // in evaluateLinkset can detect a tag-only edit (e.g.
        // {binaural:off} → {binaural:on}) and rebuild the FMOD stream.
        std::optional<bool> binaural_tag;
        // r12 P5: publisher's {upmix:on|off} tag value (nullopt =
        // unspecified, treated as off by effectiveUpmix). Same fingerprint
        // role as binaural_tag — a tag flip or debug-toggle change
        // rebuilds the stream so resolveReadOp re-picks OpKind. Spec
        // §4.5.x: upmix toggle is in the rebuild tier, not the engine-
        // level live-update tier (which is venue / wetgain).
        std::optional<bool> upmix_tag;
        // Snapshot of effectiveBinaural() at the moment we last (re)started
        // this binding's stream. Combined with binaural_tag in the
        // fingerprint so a debug-toggle change between evals also rebuilds.
        bool binaural_effective_applied = true;
        // r11 P9: publisher's {wetgain:N} tag value (nullopt = unspecified,
        // defaults to 1.0). Tracked alongside venue because the wet-mix
        // multiplier is also engine-level (same DSP) and shares the
        // "atomic store, no stream rebuild" flow.
        std::optional<F32> wetgain_tag;
        // Last value actually pushed to the engine's setWetGain() on
        // this binding's behalf. Sentinel NaN means "never pushed yet" —
        // any first applyWetGainToBinding() will go through.
        F32 wetgain_effective_applied = std::numeric_limits<F32>::quiet_NaN();
        // r12.1: publisher's {lfegain:N} tag value (nullopt = unspecified,
        // defaults to 1.0 = passthrough). Stream-level (per-binding) atomic
        // store on the LLPositionalStreamMulti — same flow as wetgain but
        // pushed to the stream object itself, not to a bus DSP.
        std::optional<F32> lfegain_tag;
        // Last value actually pushed to the stream's setLfeGain(). Sentinel
        // NaN so the first applyLfeGainToBinding() always goes through.
        F32 lfegain_effective_applied = std::numeric_limits<F32>::quiet_NaN();
        // r11 P8: publisher's {venue:NAME} tag value (nullopt = unspecified).
        // Resolved via effectiveVenue() on every evaluate; the resolved
        // name is pushed to the engine's bus-level VenueReverbDsp on
        // transition. Tracked here separately from binaural because venue
        // is engine-level (one DSP for the whole Stream3D bus) — changing
        // it does NOT need a stream rebuild, just an atomic slot swap on
        // the DSP. So venue is intentionally NOT in the rebuild fingerprint.
        std::optional<std::string> venue_tag;
        // Last name actually pushed to the engine's setVenue() on this
        // binding's behalf. Compared against the new effective on each
        // evaluate so we only push on change (avoids redundant atomic
        // stores when desc-poll re-fires with the same value).
        std::string venue_effective_applied = "dry";
        std::vector<SpeakerSlot> speakers;
        // Count of speakers truncated by the per-binding cap. Surfaced in
        // F4 throttled notification; F2-a only logs.
        S32 dropped_speakers = 0;
        // r8 F3-3: live FMOD-backed multi-speaker stream. nullptr while the
        // linkset is still incomplete (priority-poll pending) or when the
        // last (re)evaluation deferred a restart. evaluateLinkset compares
        // the new (url, speakers) tuple against the old; identical tuples
        // keep the existing stream so unrelated tag edits in the linkset
        // don't audibly bounce the audio.
        std::unique_ptr<LLPositionalStreamMulti> stream;
        // r8 F7: reconnect bookkeeping mirrors the mono Binding loop so a
        // socket-level outage rebuilds the stream instead of leaving the
        // linkset silent.
        S32 reconnect_attempts = 0;
        F64 next_retry_time = 0.0;
        bool notified_played = false;
        // r10 P5: routing-diagnostic throttle key per spec §4.4.2.
        // Recomputed each tick once the stream has reached Playing; if
        // unchanged from the value last logged, the diagnostic is suppressed.
        // Empty until the first emission.
        std::string last_diagnostic_key;

        // r12 P4: snapshot of the resolved {upmix} effective at the moment
        // we last (re)started this binding's stream. False at construction
        // (= no upmix request) and stays false until the P5 tag parser /
        // debug Stream3DUpmix sentinel set it. The stream-side mUpmixEnabled
        // is plumbed from this same value via setUpmixEnabled() at start.
        bool upmix_effective_applied = false;

        // r12 P4: throttle key for the auto-bypass chat notice (5.1 native
        // source observed while upmix was requested). Same shape as
        // last_diagnostic_key — same key, no re-emit. Emptied alongside
        // last_diagnostic_key on structural rebuild so a fresh start
        // (re)announces the bypass once.
        std::string last_upmix_notice_key;

        // r23: same parcel-gate fields as the mono Binding above. See
        // those comments for full semantics; behaviour is identical
        // (source = root prim of the linkset, gate decision applied
        // uniformly to every channel of the multi stream).
        bool is_attached = false;
        bool parcel_audible = true;
        // Media source volume policy. When a linkset has exactly one media
        // face, keep 2D media semantics and multiply by media/global volume.
        // When multiple media faces exist and one is explicitly selected for
        // 3D, treat that selected media as source gain 1.0 so the other media
        // faces can continue using the normal media volume slider.
        bool media_source_uses_viewer_volume = true;
        F32 last_pushed_volume = std::numeric_limits<F32>::quiet_NaN();
    };

    // r10 P5 / r10.x P2: routing-diagnostic emitter. Called from update()
    // once a distributed-stereo binding's stream has reached Playing (so the
    // observed source channel count is settled). Computes the throttle key
    // (root_id + url + ch_count + prim_set_signature) per spec §4.4.2 and
    // bails out if it matches `b.last_diagnostic_key`. Otherwise it walks the
    // §4.2 compatibility matrix once for the current (source_channels,
    // speakers) pair and emits one chat line per fallback case via
    // notifyStream3D (spec §4.4.1). Gated globally by the
    // `Stream3DRoutingDiagnostic` setting (Preferences > Sound checkbox) —
    // when false, the key is still updated so toggling the setting on
    // doesn't replay a stale snapshot for a binding that hasn't actually
    // changed since.
    void emitRoutingDiagnostic(DistributedStereoBinding& b);

    // r12 P4: spec §4.2.2 auto-bypass notice. Fires once per (root, url,
    // source_channels, upmix_effective) tuple when the publisher's
    // {upmix:on} request collides with a multi-channel native source —
    // i.e. the dispatch layer is silently keeping r10 placement / Bs775
    // because the source already covers the 5.1 pipeline natively. Hooked
    // into the same update tick as emitRoutingDiagnostic so a freshly
    // played binding gets both notices ordered consistently. Throttled
    // via b.last_upmix_notice_key (cleared on structural rebuild). Until
    // P5 wires the upmix tag parser, b.upmix_effective_applied is always
    // false and this is a single key compare on the no-op path.
    void emitUpmixAutoBypassNotice(DistributedStereoBinding& b);

    // r8 F4: throttled error notification. Keyed by (prim_id, kind) so the
    // user gets one toast per failure mode per 30 seconds even if the parse /
    // start path is hit on every poll tick. spec §4.9.
    enum class DistErrorKind
    {
        BadCh,
        BadRange,
        BadVolume,
        EmptyUrl,
        NoSpeakers,
        SpeakerOverLimit,
        StreamStartFailed,
        // r9 P6: source decoded but its channel count or codec layout is not
        // accepted (1/2/6 only, 6ch only Vorbis/Opus/FLAC). detail = the raw
        // mismatch summary captured by LLPositionalStreamMulti::failDetail()
        // (e.g. "channels=4" or "channels=6 codec_type=11").
        UnsupportedSourceFormat,
        // r11 P5: {binaural:...} value not on/off.
        BadBinaural,
        // r12 P5: {upmix:...} value not on/off.
        BadUpmix,
        // r11 P8: {venue:NAME} value not in LLVenueReverbDsp::knownVenues.
        BadVenue,
        // r11 P8: known venue name but its IR file failed to load at
        // engine init (missing / wrong format / sample rate mismatch).
        // Surfaced when setVenue() returns false at apply time.
        IRNotLoaded,
        // r11 P9: {wetgain:N} value not parseable as F32 (e.g. "abc").
        // Out-of-range numeric values (e.g. "5.0") are silently clamped
        // to [0.0, 2.0] per spec §4.1 line 152, NOT reported here.
        BadWetGain,
        // r12.1: {lfegain:N} value not parseable as F32 (out-of-range
        // is silently clamped to [0.0, 3.0], NOT reported here).
        BadLfeGain,
        BadSource,
        ConflictingSource,
        BadLink,
        BadFace,
        MediaFaceNotFound,
        MediaFaceAmbiguous,
        MediaSourceNotReady,
        MediaSourceInUse,
    };

    // detail carries the raw bad value (e.g. "X" for {ch:X}, "1.5" for
    // {volume:1.5}) or an over-limit count, depending on kind. Empty for
    // kinds that don't have a useful payload (NoSpeakers).
    void notifyDistributedError(const LLUUID& prim_id, DistErrorKind kind,
                                const std::string& detail);

    void evaluateBinding(const LLUUID& id);
    void evaluateMonoBinding(const LLUUID& id, const TagData& tag);

    // r23: register the parcel-change callback once gAgent is alive. Called
    // lazily on the first update() tick. Tier 1 (static-source bindings)
    // refresh their cached parcel_audible only on this signal, so the
    // per-frame cost stays zero in the common case.
    void ensureParcelCallbackRegistered();

    // r23: slot for `gAgent.addParcelChangedCallback`. Walks every binding
    // and refreshes its cached `parcel_audible`. Cheap — one canHearSound
    // call per binding (one parcel overlay lookup each, ~O(1)).
    void onAgentParcelChanged();

    // r23: evaluate canHearSound() at the given source prim's world
    // position. Returns the previous cached value when the prim is gone
    // or its position is unavailable, so a transient lookup miss doesn't
    // mute a binding spuriously.
    bool computeParcelAudible(const LLUUID& source_id, bool fallback) const;

    // r8 F2-a: (re)build the distributed-stereo binding rooted at root_id by
    // walking the linkset and harvesting whatever speaker descriptions are
    // already in mDescriptionCache. r8 F2-b: any participating prim whose
    // description is not yet cached is enqueued onto mPriorityPollQueue so
    // the binding completes within a few poll ticks rather than waiting for
    // round-robin discovery.
    // root_id is taken by value: evaluateLinkset() may erase entries from
    // mPrimToRoot whose `.second` is the very reference a caller would pass
    // (e.g. evaluateAndDispatch hands us `pr_it->second`). Copying defuses
    // that use-after-free without auditing every call site.
    void evaluateLinkset(LLUUID root_id);
    void teardownDistributedBinding(const LLUUID& root_id);

    // r11 P8: push the resolved venue name to the engine's bus-level
    // VenueReverbDsp and update binding bookkeeping. Idempotent — bails
    // out if the resolved name matches what we already pushed for this
    // binding. On engine setVenue() failure for a non-"dry" name, fires
    // an IRNotLoaded notification and forces "dry" so the bus stays
    // audible (silent fallback per spec §4.5).
    void applyVenueToBinding(DistributedStereoBinding& binding,
                             const std::optional<std::string>& venue_tag);

    // r11 P9: push the resolved wet-mix multiplier to the engine's
    // bus-level VenueReverbDsp. Idempotent (skips when the new value
    // matches the previously pushed one). Engine absent → records the
    // value but does nothing (mirrors applyVenueToBinding semantics).
    void applyWetGainToBinding(DistributedStereoBinding& binding,
                               std::optional<F32> wetgain_tag);

    // r12.1: push the resolved LFE gain multiplier to the binding's
    // stream object (LLPositionalStreamMulti::setLfeGain). Idempotent
    // (skips when value unchanged). Stream absent (binding still being
    // constructed) → records the value so the next call still pushes.
    void applyLfeGainToBinding(DistributedStereoBinding& binding,
                               std::optional<F32> lfegain_tag);

    // r8 F2-b: push id onto mPriorityPollQueue if not already queued.
    // Linear scan dedup is fine — the queue is bounded by ~16 speakers per
    // pending linkset and drains every poll tick.
    void enqueuePriorityPoll(const LLUUID& id);

    // r8 F11: send a bare ObjectSelect packet for `child` directly to its
    // region — bypassing LLSelectMgr so the user's selection state, edit
    // menu, and selection beam are untouched. The sim replies with a full
    // ObjectProperties (carrying Description), which already feeds
    // onObjectPropertiesReceived via llselectmgr.cpp:processObjectProperties.
    // The matching ObjectDeselect is queued in mPendingChildDeselect and
    // sent by drainChildDeselects() one tick later, so the sim doesn't keep
    // the prim "selected" on our behalf indefinitely.
    void requestChildDescViaSelect(LLViewerObject* child);
    void drainChildDeselects(F64 now_seconds);

    // r8 F2-c: scan mPrimToRoot looking for prims whose getRootEdit() no
    // longer matches the registered root (link / unlink / death). Both the
    // stale and the current root are re-evaluated, which transparently
    // moves the speaker slot between linksets or tears down a binding when
    // a participating prim is gone. Cheap: O(mPrimToRoot.size()) per tick,
    // bounded by max_speakers × active bindings.
    void detectLinksetStructureChanges();

    // M3b: walk in-range prims and re-poll RequestObjectPropertiesFamily for
    // any whose Description we haven't seen recently. Throttled so we never
    // burst more than a handful of requests per second at the sim. r8 F2-b:
    // mPriorityPollQueue is drained first (within the same per-tick budget)
    // so distributed-stereo linksets complete promptly.
    void pollObjectPropertiesFamily(F64 now_seconds);

    struct CacheEntry
    {
        std::string description;
        // Monotonic seconds (LLTimer::getElapsedSeconds) of the most recent
        // request send. 0 means "never sent". Round-robin re-poll uses this
        // to space sends out at Stream3DPollInterval. Priority drain uses it
        // to space retries at kPriorityRetryWait while waiting for the first
        // reply.
        F64 last_polled = 0.0;
        // r8 F8: time of the most recent reply. 0 means "never replied".
        // Distinct from last_polled so the priority queue can keep retrying
        // a prim whose request was sent but whose reply hasn't arrived
        // (sim drop / interest-list miss) without bumping into the 30 s
        // round-robin throttle.
        F64 last_replied = 0.0;
        // r8 F8: count of consecutive priority sends without a reply.
        // Reset to 0 by onObjectPropertiesReceived. Capped at
        // kPriorityRetryCap so a prim the sim refuses to answer for falls
        // back to the slower round-robin cadence instead of burning the
        // whole priority budget every tick.
        S32 priority_retries = 0;
    };

    std::map<LLUUID, CacheEntry> mDescriptionCache;
    std::map<LLUUID, Binding> mBindings;
    // r8 F2-a: distributed-stereo bindings, keyed by root prim id.
    std::map<LLUUID, DistributedStereoBinding> mDistributedBindings;
    // r8 F2-a: reverse index for O(log N) child→root resolution from
    // onObjectPropertiesReceived. Populated/cleared by evaluateLinkset and
    // teardownDistributedBinding so it stays in sync with mDistributedBindings.
    std::map<LLUUID, LLUUID> mPrimToRoot;
    std::unique_ptr<LLPositionalStream> mDebugStream;
    std::unique_ptr<LLPositionalStreamStereo> mDebugStereoStream;

    // M3b: throttle for the per-frame poll scan.
    F64 mLastPollScanTime = 0.0;
    // M3b: round-robin cursor into gObjectList so per-pass budget doesn't
    // starve prims past the first slice when num_objects > one pass can cover.
    S32 mPollCursor = 0;
    // r8 F2-b: prims that evaluateLinkset wants polled ahead of the
    // round-robin scan because they belong to a linkset where a source
    // declaration was just observed. Drained at the head of every
    // pollObjectPropertiesFamily tick, sharing the same per-tick budget.
    std::deque<LLUUID> mPriorityPollQueue;
    // r8 F8: linksets with at least one onObjectPropertiesReceived since the
    // last update() drain. Drained once per frame so a burst of replies (e.g.
    // a selection-induced ObjectProperties carrying 16 child descs at once)
    // coalesces into a single evaluateLinkset / stream rebuild instead of
    // N consecutive ones — N rebuilds blocked the main thread visibly.
    std::set<LLUUID> mPendingLinksetEval;

    // r8 F11: deferred ObjectDeselect for child prims we briefly selected
    // (via requestChildDescViaSelect) to force a full ObjectProperties reply.
    // Key = child prim id, value = monotonic-seconds at which to send the
    // ObjectDeselect. The drain in update() releases the slot ~1s after the
    // select so the sim has time to process the ObjectSelect and queue its
    // ObjectProperties reply before we tell it to forget us.
    std::map<LLUUID, F64> mPendingChildDeselect;

    // M8: edge-trigger for the "max concurrent reached" toast. Set true the
    // first time we refuse a binding due to the cap; reset to false whenever
    // total binding count drops back below the cap, so the user sees the
    // notification once per "newly full" event rather than every poll cycle.
    bool mCapNotified = false;

    // r8 F4: last-notified timestamp per (prim, kind). 30s suppression.
    // The map can in principle accumulate entries indefinitely (one per prim
    // that ever produced an error) but each entry is small (~32 B) and the
    // population in practice tracks the user's tagged prim count, so a prune
    // pass is not yet required. Cleared by shutdownAll().
    std::map<std::pair<LLUUID, DistErrorKind>, F64> mErrorThrottle;

    // r23: parcel-change signal connection. Connected lazily on the first
    // update() tick (gAgent is guaranteed alive by then). scoped_connection
    // auto-disconnects in the singleton's destructor at process exit.
    boost::signals2::scoped_connection mParcelChangedConn;

    // r9 P6.5: roots whose source URL was permanently rejected by
    // LLPositionalStreamMulti (FailReason::FormatUnsupported, e.g. 5ch
    // source or 6ch in an unsupported codec). evaluateLinkset checks this
    // at entry and skips re-opening the stream when the root still
    // declares the same bad URL — without this, every desc poll would
    // rebuild the binding, re-open FMOD, fail again, and tear down,
    // looping at the poll cadence. Invalidated when the root's URL
    // changes (description edit) and cleared by shutdownAll().
    std::map<LLUUID, std::string> mFormatFailedUrl;
};

#endif // LL_POSITIONAL_STREAM_MGR_H
