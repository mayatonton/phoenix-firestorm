# AYAstorm r13 — Release Announcement Draft

Short text intended to be pasted into the GitHub release page. r13 is a **small increment** over r12.1 (the first public release after the r10→r12.1 jump). The core is a new tag family `[ayastorm:occlude]` plus two independent UI / startup fixes.

Feature details live in the user-facing guide (`doc/3dstream-tag-guide.{ja,en,zh}.md`) and the spec docs. This note is link-only + diff highlights.

---

## AYAstorm r13 — Tag-based OBB occlusion + UI / startup fixes

### Headline: `[ayastorm:occlude]` static OBB occlusion (venue operator / builder)

A third tag family is introduced. Marking **wall / door / floor / ceiling prims** with `[ayastorm:occlude]` makes AYAstorm treat them as **sound-blocking geometry** — when the segment from listener to source crosses the OBB, audio is attenuated and pushed through a low-pass colouration that conveys a "behind the wall" feel.

- **Syntax**: bare `[ayastorm:occlude]` uses defaults (`direct:0.7 reverb:0.5`); `[ayastorm:occlude{direct:0.9}{reverb:0.7}]` overrides per prim. Details → [tag-guide §16](../doc/3dstream-tag-guide.en.md#16-static-obb-occlusion-ayastormocclude-r13) / spec `doc/spec_obb_occlusion.md` / implementation record `docs/ayastorm-r13-occlusion.md`
- **What gets occluded**: `[3dstream:...]` / `[3dstream-stereo:...]` positional streams + `llPlaySound` / attached sounds / child-prim SFX (world SFX). 2D streams / Voice / UI SFX are untouched.
- **Recommended values** (by material impression): stone `0.9/0.7` / wood `0.7/0.5` (default) / curtain `0.6/0.4` / glass `0.3/0.2` / decorative `0.1/0.05`. Details → [tag-guide §16.4](../doc/3dstream-tag-guide.en.md#164-recommended-values-by-material)
- **Dynamic doors follow automatically**: `refreshOccluders` re-reads every occluder prim's position / rotation / scale each tick, so an LSL-animated door / vehicle / moving prim tracks naturally. No dedicated "door tag" needed.
- **Orthogonal to the broadcaster-driven model**: `[3dstream:...]{venue:NAME}` (broadcaster's expressive choice) and `[ayastorm:occlude]` (builder's physical reality) are intentionally independent. The viewer does **not** auto-correct or warn when they disagree (e.g. a cave-shaped prim with `venue:cathedral`).

### Bundled debug settings

Live-tuning knobs for `[ayastorm:occlude]` (no general UI — debug settings only):

| Key | Default | Purpose |
|---|---|---|
| `Stream3DOcclusion` | `-1` (= enabled) | Master sentinel. `0` ignores all occlude tags; previously-muffled audio ramps back to bypass via the smoothing path (no cliff) |
| `Stream3DOccluderRange` | `64.0` m | Distance cull. Listener-source beyond range skips raycast. `0` always raycasts |
| `Stream3DOcclusionRampMs` | `250.0` ms | Crossfade time for direct/reverb factor changes. `0` = binary jump (debug only) |
| `Stream3DShowOccluders` | off | Debug overlay rendering each occluder as an OBB wireframe (Alt+Shift+O, independent of the master sentinel) |

Details → [tag-guide §16.6–§16.8](../doc/3dstream-tag-guide.en.md#166-distance-cull-stream3doccluderrange--64m)

### Independent fixes bundled with r13

- **Startup OS-unresponsive dialog root-cause fix** (commits `f336d43abc` / `5c3487ff06`): the URL pre-resolve introduced in r11 P10 (libcurl HEAD pre-resolve) was a **synchronous 3-second block on `https://` URLs**. Right after login, N prims with `[3dstream:url=https://…]` arriving in the same frame produced N × 3s of main-thread block → the OS "Not Responding" dialog. The path is rewritten as a **fully async worker-thread API** (`LLStream3DUrlResolve::submit/poll/cancel/shutdown`); the main-thread curl sync block is gone. Details → impl record `docs/ayastorm-r13-occlusion.md` §5.4
- **Chat font live-apply fix** (`d66bdb74fc`): on LL-style chat (FS legacy display), `ChatFontSize` / `PlainTextChatHistory` changes failed to take effect until the next utterance. Fixed. **Independent of the 3dstream paths**, picked up via cherry-pick because it surfaced during the occlusion work.

### Existing placements

- All prims placed under r8 / r9 / r10 / r11 / r12 / r12.1 **continue to work without tag edits**
- `[ayastorm:occlude]` is opt-in, so an existing venue with no occlude tags sounds identical to r12.1
- The r12 `[3dstream:...]{venue:NAME}` (venue reverb) and the new `[ayastorm:occlude]` (venue occlusion) are **intentionally orthogonal** — pasting occlude on a `venue:dry` building, or having an `venue:cathedral` prim get occluded by walls, both work cleanly without conflict

### Known limitations

- **256 simultaneous occluders** (`kMaxOccluders` hardcoded). Typical SL venues (~100 prims) have ample headroom; the 257th onward are not registered (`LL_WARNS` logged).
- **OBB approximation**: occlusion is decided per bounding box. Complex shapes (arches, curves, stair railings) incur approximation error — split into panels and tag individually for finer resolution.
- **Bundled FMOD 2.03.07 constraint**: implementation is a viewer-side segment-vs-OBB slab test (the bundled libfmod's `FMOD::Geometry::createGeometry` is non-functional). Transparent to end users.
- **Steam Audio integration / SOFA personal HRTF / occluder sound-transmission curves / per-material preset table** are not in r13 — deferred to r14+. r13's design decision is that the viewer-side raycast plus a single lowpass already conveys the "behind the wall" feel convincingly.

### Documentation

- User-facing guide: `doc/3dstream-tag-guide.ja.md` / `.en.md` / `.zh.md` (§16 added — full OBB occlusion section)
- r13 spec: `doc/spec_obb_occlusion.md`
- r13 implementation record + design rationale: `docs/ayastorm-r13-occlusion.md`
- Roadmap: `docs/ayastorm-stream3d-roadmap.md`
