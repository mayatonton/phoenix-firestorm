# AYAstorm r13 — Release Announcement Draft

Short text intended to be pasted into the GitHub release page. r13 is a **small increment** over r12.1 (the first public release after the r10→r12.1 jump). The core is a new tag family `[ayastorm:occlude]` (exact-shape mesh raycast) plus two independent UI / startup fixes.

Feature details live in the user-facing guide (`docs/guides/3dstream-tag-guide.{ja,en,zh}.md`) and the spec docs. This note is link-only + diff highlights.

---

## AYAstorm r13 — Tag-based occlusion (exact-shape mesh raycast) + UI / startup fixes

### Headline: `[ayastorm:occlude]` static occlusion (venue operator / builder)

A third tag family is introduced. Marking **wall / door / floor / ceiling prims** with `[ayastorm:occlude]` makes AYAstorm treat them as **sound-blocking geometry** — when the segment from listener to source crosses the **prim's actual triangle mesh** (path cut openings, hollow interiors, full mesh shapes all included), audio is attenuated and pushed through a low-pass colouration that conveys a "behind the wall" feel.

- **Syntax**: bare `[ayastorm:occlude]` uses defaults (`direct:0.7 reverb:0.5`); `[ayastorm:occlude{direct:0.9}{reverb:0.7}]` overrides per prim. Details → [tag-guide §16](../guides/3dstream-tag-guide.en.md#16-static-occlusion-ayastormocclude-r13) / spec `docs/specs/spec_obb_occlusion.md` / implementation record `docs/ayastorm-r13-occlusion.md`
- **What gets occluded**: `[3dstream:...]` / `[3dstream-stereo:...]` positional streams + `llPlaySound` / attached sounds / child-prim SFX (world SFX). 2D streams / Voice / UI SFX are untouched.
- **Exact-shape decision**: sound passes through Path Cut openings / Hollow interiors and is muffled only by the wall body itself; mesh prims use their precise geometry. A two-stage test — bounding-OBB pre-cull (~95% reject) → Möller-Trumbore triangle raycast — keeps CPU low while preserving full-shape accuracy (details → [tag-guide §16.2](../guides/3dstream-tag-guide.en.md#162-behaviour-model)).
- **Multi-prim accumulation**: direct / reverb factors **stack multiplicatively** — e.g. two walls of `direct=0.7` produce an effective `1 - (1-0.7)² ≈ 0.91`, matching the intuition that more walls = more muffling.
- **Recommended values** (by material impression): stone `0.9/0.7` / wood `0.7/0.5` (default) / curtain `0.6/0.4` / glass `0.3/0.2` / decorative `0.1/0.05`. Details → [tag-guide §16.4](../guides/3dstream-tag-guide.en.md#164-recommended-values-by-material)
- **Dynamic doors follow automatically**: `refreshOccluders` re-reads every occluder prim's position / rotation / scale each tick, so an LSL-animated door / vehicle / moving prim tracks naturally. No dedicated "door tag" needed. A prim **selected in the build floater** also gets its shape re-extracted live while Path Cut / Hollow / Sculpt sliders are dragged, so the cyan overlay and audio raycast both follow without waiting for the edit window to close.
- **Orthogonal to the broadcaster-driven model**: `[3dstream:...]{venue:NAME}` (broadcaster's expressive choice) and `[ayastorm:occlude]` (builder's physical reality) are intentionally independent. The viewer does **not** auto-correct or warn when they disagree (e.g. a cave-shaped prim with `venue:cathedral`).

### Bundled debug settings

Live-tuning knobs for `[ayastorm:occlude]` (no general UI — debug settings only):

| Key | Default | Purpose |
|---|---|---|
| `Stream3DOcclusion` | `-1` (= enabled) | Master sentinel. `0` ignores all occlude tags; previously-muffled audio ramps back to bypass via the smoothing path (no cliff) |
| `Stream3DOccluderRange` | `64.0` m | Distance cull. Listener-source beyond range skips raycast. `0` always raycasts |
| `Stream3DOcclusionRampMs` | `250.0` ms | Crossfade time for direct/reverb factor changes. `0` = binary jump (debug only) |
| `Stream3DShowOccluders` | off | Debug overlay rendering each occluder as a **cyan triangle mesh** (translucent fill + wireframe, the exact triangles used by the raycast), Alt+Shift+O, independent of the master sentinel. Selected prims update live during build-floater edits |

Details → [tag-guide §16.6–§16.8](../guides/3dstream-tag-guide.en.md#166-distance-cull-stream3doccluderrange--64m)

### Independent fixes bundled with r13

- **Startup OS-unresponsive dialog root-cause fix** (commits `f336d43abc` / `5c3487ff06`): the URL pre-resolve introduced in r11 P10 (libcurl HEAD pre-resolve) was a **synchronous 3-second block on `https://` URLs**. Right after login, N prims with `[3dstream:url=https://…]` arriving in the same frame produced N × 3s of main-thread block → the OS "Not Responding" dialog. The path is rewritten as a **fully async worker-thread API** (`LLStream3DUrlResolve::submit/poll/cancel/shutdown`); the main-thread curl sync block is gone. Details → impl record `docs/ayastorm-r13-occlusion.md` §5.4
- **Chat font live-apply fix** (`d66bdb74fc`): on LL-style chat (FS legacy display), `ChatFontSize` / `PlainTextChatHistory` changes failed to take effect until the next utterance. Fixed. **Independent of the 3dstream paths**, picked up via cherry-pick because it surfaced during the occlusion work.
- **V3 skin on-screen chat console default alignment** (`617716ced8`): only V3 skin had `FSUseNearbyChatConsole` defaulting to `0` (initially off); aligned with the other skins (firestorm / phoenix / text / hybrid) to default to `1` (initially on). Fresh installs and skin switches now behave consistently across skins.

### Independent features bundled with r13

- **`[parcelhide]` altitude gate** (`1dfa52d0e9`): writing `[parcelhide:{altitude:1000-2000,3000-4000}]` in a parcel description fires the hide only when your Z (altitude) is inside one of the listed ranges. Both endpoints inclusive, hyphen-separated, comma-separated for multiple ranges. Use cases include hiding only a specific skybox floor for photo work. The legacy bare `parcelhide` (no argument) behaviour is unchanged.
- **Ignore IMs from other residents' objects (`FSIgnoreObjectIM`)** (`e99d7c9abf`): a global switch that silently drops IMs from objects rezzed by other residents (vendor ads, fishing announcements, etc.). **IMs from objects you own (HUDs, your own rezzers) pass through** (`permYouOwner()` check). A checkbox is added under Preferences → Notifications → People; default off.

### Existing placements

- All prims placed under r8 / r9 / r10 / r11 / r12 / r12.1 **continue to work without tag edits**
- `[ayastorm:occlude]` is opt-in, so an existing venue with no occlude tags sounds identical to r12.1
- The r12 `[3dstream:...]{venue:NAME}` (venue reverb) and the new `[ayastorm:occlude]` (venue occlusion) are **intentionally orthogonal** — pasting occlude on a `venue:dry` building, or having an `venue:cathedral` prim get occluded by walls, both work cleanly without conflict

### Known limitations

- **256 simultaneous occluders** (`kMaxOccluders` hardcoded). Typical SL venues (~100 prims) have ample headroom; the 257th onward are not registered (`LL_WARNS` logged).
- **2000-triangle cap per occluder** (`kMaxTrisPerOccluder` hardcoded). A mesh prim exceeding the cap skips triangle extraction and falls back to bounding-OBB-only occlusion (`LL_WARNS_ONCE` logged; the §16.8 cyan overlay shows nothing for that prim — a useful visual cue). Standard SL prims and architectural mesh prims are well within budget.
- **Bundled FMOD 2.03.07 constraint**: implementation is a viewer-side OBB pre-cull + Möller-Trumbore triangle raycast (the bundled libfmod's `FMOD::Geometry::createGeometry` is non-functional). Transparent to end users.

### Intentionally out of scope (r13 design decision)

The following are **not "yet to come"** — they are explicit decisions to keep them out of r13 (see [`docs/ayastorm-stream3d-roadmap.md`](./ayastorm-stream3d-roadmap.md) for the full rationale).

- **Steam Audio integration / diffraction / reflection / resonance physics simulation**: reflection and resonance are already covered by the r11 convolution venue reverb (9 IRs); diffraction is approximated perceptually by the r13 occlusion lowpass + attenuation. Carrying a physics engine for marginal gain isn't worth the 3-OS binary distribution debt and engine-dependency surface.
- **SOFA per-source HRTF / personal HRTF**: r11 lite-HRTF (ITD + ILD shadow + air absorption) already crosses the realism threshold AYAstorm aims for. SOFA is CPU-expensive and only meaningful for users with personal measurements; the redistribution-license investigation overhead is also avoided.
- **Occluder sound-transmission curves / per-material preset table**: in practice the SL material flag and the desired occlusion value correlate weakly. Routing builders to the §16.4 recommended-values table (stone 0.9/0.7 etc.) keeps operation simpler than a preset map.
- If a future use case proves any of these warrant a fresh spike, they'll be re-evaluated then — but the r13 design says "behind-the-wall feel", "venue room sound", and "spatial localisation" are already at a sufficient level of realism.

### Documentation

- User-facing guide: `docs/guides/3dstream-tag-guide.ja.md` / `.en.md` / `.zh.md` (§16 added — full static occlusion section)
- r13 spec: `docs/specs/spec_obb_occlusion.md`
- r13 implementation record + design rationale: `docs/ayastorm-r13-occlusion.md`
- Roadmap: `docs/ayastorm-stream3d-roadmap.md`
- Rendering-performance survey note (discussion draft): `docs/ayastorm-render-perf-survey.md` (`33c3afaf62`) — a cross-layer observation note over LL core + Firestorm + AYAstorm rendering hotspots. Not an r13 feature; bundled as a discussion baseline for future work.
