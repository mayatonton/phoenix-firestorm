# AYAstorm r15 — Release Announcement

**r15 is the next public release after r12.1 (the first release of the r10→r12.1 jump), bundling three steps (r13 + r14 + r15) into a single jump** — the final pillar of the audio-expression chapter (tag-based occlusion) together with the first two installments of the new "visual realism" chapter (atmospheric volume + light shafts).

Feature details live in the user-facing guide (`docs/guides/3dstream-tag-guide.{ja,en,zh}.md`) and the spec / roadmap docs. This note is link-only + diff highlights.

---

## AYAstorm r15 — Tag-based occlusion + visual-realism chapter installments 1+2 (atmospheric volume + light shafts)

A single r12.1 → r15 jump delivers the following. **r13 and r14 were not shipped as standalone releases**; the close-out of the audio-expression chapter (r13) and the opening of the visual-realism chapter (r14+r15) are consolidated into one release.

### r13-origin — Tag-based occlusion (exact-shape mesh raycast) + UI / startup fixes

#### r13 headline: `[ayastorm:occlude]` static occlusion (venue operator / builder)

A third tag family is introduced. Marking **wall / door / floor / ceiling prims** with `[ayastorm:occlude]` makes AYAstorm treat them as **sound-blocking geometry** — when the segment from listener to source crosses the **prim's actual triangle mesh** (path cut openings, hollow interiors, full mesh shapes all included), audio is attenuated and pushed through a low-pass colouration that conveys a "behind the wall" feel.

- **Syntax**: bare `[ayastorm:occlude]` uses defaults (`direct:0.7 reverb:0.5`); `[ayastorm:occlude{direct:0.9}{reverb:0.7}]` overrides per prim. Details → [tag-guide §16](../guides/3dstream-tag-guide.en.md#16-static-occlusion-ayastormocclude-r13) / spec `docs/specs/spec_obb_occlusion.md` / implementation record `docs/ayastorm-r13-occlusion.md`
- **What gets occluded**: `[3dstream:...]` / `[3dstream-stereo:...]` positional streams + `llPlaySound` / attached sounds / child-prim SFX (world SFX). 2D streams / Voice / UI SFX are untouched.
- **Exact-shape decision**: sound passes through Path Cut openings / Hollow interiors and is muffled only by the wall body itself; mesh prims use their precise geometry. A two-stage test — bounding-OBB pre-cull (~95% reject) → Möller-Trumbore triangle raycast — keeps CPU low while preserving full-shape accuracy (details → [tag-guide §16.2](../guides/3dstream-tag-guide.en.md#162-behaviour-model)).
- **Multi-prim accumulation**: direct / reverb factors **stack multiplicatively** — e.g. two walls of `direct=0.7` produce an effective `1 - (1-0.7)² ≈ 0.91`, matching the intuition that more walls = more muffling.
- **Recommended values** (by material impression): stone `0.9/0.7` / wood `0.7/0.5` (default) / curtain `0.6/0.4` / glass `0.3/0.2` / decorative `0.1/0.05`. Details → [tag-guide §16.4](../guides/3dstream-tag-guide.en.md#164-recommended-values-by-material)
- **Dynamic doors follow automatically**: `refreshOccluders` re-reads every occluder prim's position / rotation / scale each tick, so an LSL-animated door / vehicle / moving prim tracks naturally. No dedicated "door tag" needed. A prim **selected in the build floater** also gets its shape re-extracted live while Path Cut / Hollow / Sculpt sliders are dragged, so the cyan overlay and audio raycast both follow without waiting for the edit window to close.
- **Orthogonal to the broadcaster-driven model**: `[3dstream:...]{venue:NAME}` (broadcaster's expressive choice) and `[ayastorm:occlude]` (builder's physical reality) are intentionally independent. The viewer does **not** auto-correct or warn when they disagree (e.g. a cave-shaped prim with `venue:cathedral`).

#### Bundled debug settings (occlusion)

Live-tuning knobs for `[ayastorm:occlude]` (no general UI — debug settings only):

| Key | Default | Purpose |
|---|---|---|
| `Stream3DOcclusion` | `-1` (= enabled) | Master sentinel. `0` ignores all occlude tags; previously-muffled audio ramps back to bypass via the smoothing path (no cliff) |
| `Stream3DOccluderRange` | `64.0` m | Distance cull. Listener-source beyond range skips raycast. `0` always raycasts |
| `Stream3DOcclusionRampMs` | `250.0` ms | Crossfade time for direct/reverb factor changes. `0` = binary jump (debug only) |
| `Stream3DShowOccluders` | off | Debug overlay rendering each occluder as a **cyan triangle mesh** (translucent fill + wireframe, the exact triangles used by the raycast), Alt+Shift+O, independent of the master sentinel. Selected prims update live during build-floater edits |

Details → [tag-guide §16.6–§16.8](../guides/3dstream-tag-guide.en.md#166-distance-cull-stream3doccluderrange--64m)

#### Independent fixes bundled with r13

- **Startup OS-unresponsive dialog root-cause fix** (commits `f336d43abc` / `5c3487ff06`): the URL pre-resolve introduced in r11 P10 (libcurl HEAD pre-resolve) was a **synchronous 3-second block on `https://` URLs**. Right after login, N prims with `[3dstream:url=https://…]` arriving in the same frame produced N × 3s of main-thread block → the OS "Not Responding" dialog. The path is rewritten as a **fully async worker-thread API** (`LLStream3DUrlResolve::submit/poll/cancel/shutdown`); the main-thread curl sync block is gone. Details → impl record `docs/ayastorm-r13-occlusion.md` §5.4
- **Chat font live-apply fix** (`d66bdb74fc`): on LL-style chat (FS legacy display), `ChatFontSize` / `PlainTextChatHistory` changes failed to take effect until the next utterance. Fixed. **Independent of the 3dstream paths**, picked up via cherry-pick because it surfaced during the occlusion work.
- **V3 skin on-screen chat console default alignment** (`617716ced8`): only V3 skin had `FSUseNearbyChatConsole` defaulting to `0` (initially off); aligned with the other skins (firestorm / phoenix / text / hybrid) to default to `1` (initially on). Fresh installs and skin switches now behave consistently across skins.

#### Independent features bundled with r13

- **`[parcelhide]` altitude gate** (`1dfa52d0e9`): writing `[parcelhide:{altitude:1000-2000,3000-4000}]` in a parcel description fires the hide only when your Z (altitude) is inside one of the listed ranges. Both endpoints inclusive, hyphen-separated, comma-separated for multiple ranges. Use cases include hiding only a specific skybox floor for photo work. The legacy bare `parcelhide` (no argument) behaviour is unchanged.
- **Ignore IMs from other residents' objects (`FSIgnoreObjectIM`)** (`e99d7c9abf`): a global switch that silently drops IMs from objects rezzed by other residents (vendor ads, fishing announcements, etc.). **IMs from objects you own (HUDs, your own rezzers) pass through** (`permYouOwner()` check). A checkbox is added under Preferences → Notifications → People; default off.

### r14-origin — Visual-realism chapter installment 1: atmospheric volume (volumetric atmosphere)

Closing the audio-expression chapter (r8–r13), the visual-realism chapter opens at r14. **The chapter's core thesis is "air and space worth photographing"** / "the actual colour, air, and mood that matter present". LUT / tone-grading "photo-look" cheats and AAA-style darkness-dependent realism are **explicitly rejected** in this chapter; the approach is to rebuild scene-referred physical calculations on the inside (chapter thesis → `docs/ayastorm-visual-realism-roadmap.md` §1).

r14 is the first installment along axis A (atmosphere / air), targeting **"air visible as a volume according to distance and altitude"**.

- **Master switch `AYAVisualRealismEnabled`** (default TRUE): one switch toggles the entire visual-realism chapter (r14 onward) on/off. With it OFF, visuals revert to pre-r13 behaviour. Per-axis / per-feature cvars are deliberately not multiplied in this chapter (`feedback_prefer_defaults_over_config.md`).
- **Altitude density (physical altitude attenuation)**: a scale-height model where atmospheric density decays exponentially with altitude is inserted in `calcAtmosphericVars`. Distant scenery from high altitudes and low-sky haze shift from a "tinted filter" look to natural volumetric attenuation. Details → `docs/ayastorm-r14-volumetric-atmosphere.md` §3-§4
- **Scene-referred integration** (inside the sky shader; `skyV.glsl` haze composition split into blue / haze channels and integrated in linear space): morning / evening warm tones, blue-sky cast, and overcast hues are rebuilt by reinterpreting preset values (Blue Density / Haze Density / Haze Horizon) as **physical parameters**. Preset compatibility is preserved (morning preset still reads as morning warmth, evening still as evening), but the physical plausibility of the air goes up.
- **WindLight presets are not lost**: morning / evening / night / region-specific presets continue to feed in as input numbers. Estate operator / broadcaster / AYA's own preset assets are not invalidated.
- **deferred → HDR scene buffer → tonemap → LDR skeleton is preserved**: the merge path with upstream Linden is maintained to avoid AYAstorm-only fork-maintenance collapse. Only the inside (atmospherics math, sun-halo / haze-glow generation) is rewritten.

#### Intentionally deferred from r14

- **Preetham (1999) path-length physicalisation for the sun direction** (`docs/ayastorm-r14-volumetric-atmosphere.md` §4 P2.b/c): deferred due to a "sun disc disappears" side effect. To be reinstated together with a sun-disc-preserving design.
- **Sun disc HDR boost** (alpha-driven halo, premise wrong, reverted in `69cf280f43`): attempted once in r14 P1 with zero perceptual gain and unground. Not re-attempted in r14; will fold into r17 (time-of-day colour temperature + clouds).

### r15-origin — Visual-realism chapter installment 2: light shafts piercing space (godrays)

With "air visible as a volume" in place from r14, r15 adds the sensation of **light shafts running through that air**.

- **Shadow-map-driven screen-space godrays pass** (new): a fullscreen pass inserted in `renderGeomPostDeferred` right after atmospherics. The existing cascaded sun shadow is reused (no new shadow buffer is added); a 16-sample ray-march along the view ray integrates "is sunlight not occluded here?", a Mie forward-peak phase (`cos^8`) shapes the contribution into a veil along the sun direction, and the result is added on top of the HDR scene buffer additively. Details → `docs/ayastorm-r15-godrays.md`
- **Strength is fixed at `strength = 0.10`**: no tuning cvar is introduced (single master switch toggles the entire chapter). AYA's perceptual tuning stepped down 0.5 → 0.2 → 0.15 → 0.10, landing at a level that lays a subtle veil along the sun direction without breaking sky or ground colour.
- **Master switch shared with r14**: `AYAVisualRealismEnabled = FALSE` skips the godrays pass as well. With it OFF, visuals revert to pre-r13 behaviour.
- **No new cvars / no new UI**: the broadcaster-driven philosophy (r11+) is carried over to the visual side. **No Preferences UI additions** ship from r15 in this release.
- **No collision with existing glow / bloom**: godrays operate on the HDR scene buffer; glow runs a bright pass after tonemap. There is no logical compositing collision; the two add additively.

#### Knowledge persisted from r15 P1

When writing a post-pass to the scene buffer with additive blending (`ONE/ONE`), the shader **must** force `frag_color.a = 0.0`. Pushing alpha=1 accumulates into the scene buffer's alpha channel — which `doAtmospherics` / sky composition reads as a sky mask — so after tonemap **the sky blows out to pure white**. The symptom was observed on the very first r15 P1 deploy → root cause identified → persisted as memory `project_aya_visual_realism_alpha_protect.md` (required knowledge for any future r16+ visual-realism post-pass).

### Existing placements

- All prims placed under r8 / r9 / r10 / r11 / r12 / r12.1 **continue to work without tag edits**
- `[ayastorm:occlude]` is a new opt-in tag, so an existing venue with no occlude tags sounds identical to r12.1 (occlusion-wise)
- The r14 / r15 visual-realism chapter applies via the `AYAVisualRealismEnabled` master switch (default TRUE) globally, but **with it OFF visuals revert exactly to pre-r13**. WindLight / Environment / region-specific presets continue to feed in as input — no preset assets are invalidated.
- The r12 `[3dstream:...]{venue:NAME}` (venue reverb) and the new r13 `[ayastorm:occlude]` (venue occlusion) are **intentionally orthogonal** — pasting occlude on a `venue:dry` building, or having a `venue:cathedral` prim get occluded by walls, both work cleanly without conflict.

### Known limitations

- **Occlusion (r13-origin):**
  - **256 simultaneous occluders** (`kMaxOccluders` hardcoded). Typical SL venues (~100 prims) have ample headroom; the 257th onward are not registered (`LL_WARNS` logged).
  - **2000-triangle cap per occluder** (`kMaxTrisPerOccluder` hardcoded). A mesh prim exceeding the cap skips triangle extraction and falls back to bounding-OBB-only occlusion (`LL_WARNS_ONCE` logged; the §16.8 cyan overlay shows nothing for that prim — a useful visual cue). Standard SL prims and architectural mesh prims are well within budget.
  - **Bundled FMOD 2.03.07 constraint**: implementation is a viewer-side OBB pre-cull + Möller-Trumbore triangle raycast (the bundled libfmod's `FMOD::Geometry::createGeometry` is non-functional). Transparent to end users.
- **Visual-realism chapter (r14 / r15-origin):**
  - **Depends on shadow detail**: r15 godrays require the cascaded sun shadow (RenderShadowDetail ≥ 1). With shadows fully off, the godrays pass contributes effectively zero — not a visual break, just invisible.
  - **Limited to a cone around the sun**: the `cos^8` phase exponent means meaningful contribution only within roughly 30° of the sun direction. View rays facing fully away from the sun get no godrays (by design).
  - **Pixel-exact parity with old screenshots is not preserved**: r14's atmospherics rewrite changes how colour is composited, so screenshots taken before r13 will not match pixel-for-pixel. The master switch OFF reverts to pre-r13 visuals.

### Intentionally out of scope (r14 / r15 design decision)

The following are **not "yet to come"** — they are explicit decisions to keep them out (details → [`docs/ayastorm-visual-realism-roadmap.md`](./ayastorm-visual-realism-roadmap.md)).

- **"Photo-look" cheats via LUT / tonemap / colour grade**: scene-referred physical refinement is taken instead. Explicitly rejected in this chapter.
- **AAA-style darkness dependence for realism**: AYAstorm aims for atmosphere that holds up in bright daylight / overcast / rain. "Just make it darker" is not adopted.
- **Heavy per-frame full-screen volumetric ray-march**: the distribution / GPU-load cost doesn't fit AYAstorm's discipline (1 viewer / 3 OS). r15 godrays is the lightweight 16-sample shadow-driven variant.
- **Per-axis / per-feature debug-settings sprawl**: the entire chapter toggles via a single master switch `AYAVisualRealismEnabled` (`feedback_prefer_defaults_over_config.md`).

To follow in subsequent releases:
- **r16 aerial perspective** (colour change with distance): depth-based scattering integration; reinterpret preset Distance Multiplier as a physical coefficient.
- **r17 time-of-day colour temperature + cloud realism**: colour-temperature interpretation of Sun/Ambient; lightweight volumetric clouds (not heavy raymarch).
- **r18+ material colour (B axis)** / **r20+ camera expression (C axis)**.

### Documentation

- User-facing guide: `docs/guides/3dstream-tag-guide.ja.md` / `.en.md` / `.zh.md` (§16 added — full static occlusion section)
- r13 spec: `docs/specs/spec_obb_occlusion.md`
- r13 implementation record + design rationale: `docs/ayastorm-r13-occlusion.md`
- r14 spec: `docs/ayastorm-r14-volumetric-atmosphere.md`
- r15 spec: `docs/ayastorm-r15-godrays.md`
- Visual-realism chapter roadmap: `docs/ayastorm-visual-realism-roadmap.md` (r14-r17 A-axis, r18+ B-axis, r20+ C-axis long-range plan)
- Audio-expression chapter roadmap (closed at r13): `docs/ayastorm-stream3d-roadmap.md`
- Rendering-performance survey note (discussion draft): `docs/ayastorm-render-perf-survey.md` — a cross-layer observation note over LL core + Firestorm + AYAstorm rendering hotspots. Not an r13-r15 feature; bundled as a discussion baseline for future work.
