# AYAstorm r23 — Release Announcement

**r23 brings AYAstorm 3D stream (positional / 5.1ch / distributed-stereo / binaural — r6 onward) into compliance with SL's `PARCEL_FLAG_SOUND_LOCAL` boundary**, the same rule gestures and object sounds have always followed. No new cvar, no inverse tag — the parcel flag itself is the source of truth.

> **Distribution**: r23 ships as the headline feature of the r13–r23 bundle tag. Other release notes for the bundled releases are linked directly from the GitHub Release page.

Implementation details / known limits / configuration reference live in the permanent spec (`docs/ayastorm-r23-parcel-bound-3d-stream.md`). This note is the entry point and diff highlight.

---

## AYAstorm r23 — Parcel-bound 3D stream

### Headline: 3D streams now stop at the parcel boundary, the way SL has always intended

Through r22, AYAstorm's 3D stream (positional / distributed-stereo / 5.1ch placement / venue reverb / binaural — r6 onward) could be heard **across parcel boundaries unconditionally**. SL's vanilla flow for gestures, object sounds, and parcel music already respects `PARCEL_FLAG_SOUND_LOCAL` ("Restrict gestures and object sounds to this parcel") on either the source or listener parcel. 3D stream was an exception — and that exception was a problem for streamers running events while not wanting to spill into neighbouring parcels.

r23 brings 3D stream onto the same rule. No new cvar, no streamer-side opt-in tag, no escape hatch — the parcel flag itself is the source of truth, matching what listeners already expect from gesture and object sound behaviour.

### How it works

A single call to `LLViewerParcelMgr::canHearSound(pos_global)` is added to the 3D stream channel's volume-update hook. Behaviour:

1. Listener and source in the **same parcel** → audible (unchanged)
2. Different parcels, **agent parcel** has SOUND_LOCAL → muted
3. Different parcels, **source parcel** has SOUND_LOCAL → muted
4. Otherwise → audible (unchanged)

This is the **same logic** SL already uses for gestures, object sounds, and some attached sounds — we're not inventing a new rule, just bringing 3D stream into compliance.

### Per-channel position vs source position

For 5.1ch placement and distributed-stereo (r8–r10), each FMOD channel has its own spatial position — but **parcel judgment uses one point: the 3D stream root prim's position**. The result applies uniformly to every channel (all play or all mute).

- Listener inside source parcel → all channels audible, even if a 5.1 FR speaker's spatial position physically extends into a neighbour parcel
- Listener in neighbour parcel → all channels muted, even if a spatial FR position is near the listener

Per-channel parcel judgment would create a "lopsided audio image" effect that's musically unpleasant, so it's intentionally not adopted (spec §3.2).

### Mute strategy: volume=0, not paused

The mute is implemented by `setVolume(0)` rather than pausing the FMOD channel. This:
- Keeps channel position / spatial attenuation calculations running, so re-entering the parcel resumes smoothly
- Avoids the click-noise / re-seek risk of pause/resume
- Lets `parcel_change` signals do the heavy lifting (with an idempotent guard so FMOD isn't called when state hasn't changed)

### Two-tier evaluation cadence

**Tier 1 (stationary sources, the majority)**: only re-evaluated on `LLAgent::addParcelChangedCallback` — fires when the avatar crosses a parcel line. Per-frame cost is zero.

**Tier 2 (worn streams, where source position = wearer position)**: `LLViewerObject::isAttachment()` flag in the binding triggers per-frame `canHearSound()` evaluation. The Tier 2 loop only runs when worn streams exist; even then, "1 query × 60fps × few attachments" is cheap.

### Settings

**Intentionally none.** No new cvar is introduced.

- An escape-hatch cvar (e.g. "FSParcelBoundStream3D") would let streamers bypass the rule, but that defeats the point of conforming to parcel regulation
- A streamer-side inverse tag (e.g. "ignore parcel boundary") would also bypass the rule and require everyone to be aware of it
- Streamers who want global broadcast can simply leave their parcel's SOUND_LOCAL off — exactly the SL native pathway

### Migration note for streamers

If you're a streamer using AYAstorm 3D stream and your event setup currently relies on neighbour-parcel listeners hearing you:

- The default parcel flag is **off** — if you haven't enabled SOUND_LOCAL, **nothing changes** for you
- If you have enabled SOUND_LOCAL on your parcel, neighbour-parcel listeners will now correctly stop hearing the stream — this matches what SL has always done for gestures and object sounds, the 3D stream was simply an unintentional exception

### Known limitations

- **Region-crossing TP**: state is correctly reapplied immediately after the parcel-changed signal fires for the new region. Brief audible window during very fast crossings is possible but bounded.
- **Worn stream walking out of parcel**: the wearer themselves stays in the same parcel as the source (always audible). Bystanders only hear the wearer while in the same parcel as the wearer.
- **Venue reverb tail**: when the main stream mutes, reverb input stops; the tail decays naturally. No special handling needed.

### Implementation summary

- `llpositionalstreammgr.cpp` / `.h` — volume-update hook now calls `canHearSound()`
- No new cvar, no new uniform, no UI changes
- 3 OS (Linux / macOS / Windows) build + verification passed

### Documentation

- r23 full spec / source position / two-tier evaluation / risk register: `docs/ayastorm-r23-parcel-bound-3d-stream.md`
- Prior 3D stream releases (r6–r12, listener bindings): see in-tree spec docs and prior release notes (r10 5.1ch placement, r11 binaural + venue reverb, etc.)
