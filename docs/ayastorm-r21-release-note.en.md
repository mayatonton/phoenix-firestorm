# AYAstorm r21 — Release Announcement Draft

Short text intended to be pasted into the GitHub release page. **r21 is a single-feature UX release** outside the visual-realism chapter (r14-r20) — a complete redesign of the right-click picker for the user's own rigged attachments, switching from upstream CPU bind-pose mesh-ray to a GPU-rendered object-ID buffer.

Implementation details / known limits / configuration reference live in the permanent spec (`docs/ayastorm-r21-self-rigged-picker.md`). This note is link-only + diff highlights.

---

## AYAstorm r21 — GPU self-rigged picker

### Headline: right-clicking your own attachments now picks what you see

Until r20, right-clicking on your own rigged attachments (Mesh body / clothing / hair) used Firestorm / Linden's upstream picker, which intersects a world ray against the **CPU bind-pose mesh**. That produced three structural symptoms:

- **Closeup zoom** click on a shirt → resolved to your avatar body instead of the shirt
- **Alpha-cutout hair** in front of the face → ray pierces the discarded triangles and grabs the hair
- **Idle animation frames** with skin drift → CPU bind-pose and GPU skinning diverge by ~4-5 cm, ray misses entirely

r21 routes self attachment picking through a **dedicated GPU object-ID buffer** (`mObjectIDBuffer`) that re-renders `gAgentAvatarp`'s rigged attachments with the **same skinning matrices used by the visible scene**, packing each prim's LocalID into four 8-bit channels. The right-click handler reads back the byte quad at the mouse pixel and resolves it by walking the local attachment tree. CPU/GPU drift, alpha-cutout mismatch, and idle skin lag **cannot occur by construction** — the picker sees exactly what the screen shows.

Details → spec `docs/ayastorm-r21-self-rigged-picker.md`

### Settings

| Key | Default | Purpose |
|---|---|---|
| `FSSelfRiggedPickerEnable` | `1` | Master switch for the picker. `0` reverts to pre-r21 upstream behaviour entirely |
| `FSSelfRiggedPickerGPU` | `1` | Kill-switch for the GPU buffer pass. `0` = picker no-op, upstream worldray is used unchanged. Provided as escape hatch for environments where the GPU pass misbehaves (Mac software OpenGL etc.) |
| `FSSelfRiggedPickerArmedMode` (experimental) | `1` | Experimental gate: only render the GPU ID pass while the self avatar or one of its attachments is hovered. `0` returns to continuous rendering |
| `FSSelfRiggedPickerArmSeconds` (experimental) | `3.0` | Seconds the ID pass remains active after the last hover (shorter = quicker shutdown after hover ends; longer = less right-click ready latency at the cost of more wasted draw time) |

> **CPU fallback is intentionally not provided.** Either the GPU pass is active (`GPU=1`) or the picker is disabled (`GPU=0`, equivalent to pre-r21 behaviour). This is a deliberate design choice — see memory `feedback_root_cause_not_dump.md` (no half-working workarounds) and `feedback_feature_value_in_main_usecase.md` (judge features by the main use case).

### Additional fixes folded into r21

- **"Couldn't find object ... selected." warning resolved** (selection handoff fix): The previous structure sent two temporary selections per right click — first the stale upstream worldray pick, then the GPU-corrected one. The first one's `ObjectProperties` response arrived after the selection had already been replaced, producing large warning bursts from `LLSelectMgr` (887 entries in one verification session). `LLToolSelect::handleObjectSelection()` is now called exactly once, after the AYA GPU picker block finalises `mPick`. Details: `docs/ayastorm-r21-selection-handoff-investigation.md`.
- **armed mode (experimental)**: An optional gate that limits the otherwise-continuous GPU ID pass to the period when the self avatar / a self attachment is hovered. Cost outside of hover drops to near zero; during hover the cost matches the always-on configuration. Evaluation log (cursor-off 20-second trace / mouselook behaviour / continuous-hover observations) and tuning candidates: `docs/ayastorm-r21-picker-armed-mode.md`.

### Known limitations

- **Alpha-blended hair in front of the face**: GPU buffer relies on depth, so alpha-discarded triangles don't reach the picker — but truly alpha-blended hair that doesn't write depth still pierces. AYA-evaluated and accepted (2026-05-14).
- **HUD attachments**: bypassed (HUD camera is screen-space, not present in `mObjectIDBuffer`).
- **Non-rigged self attachments** (piercings, single jewelry prims): the GPU buffer only covers `PASS_*_RIGGED`, so non-rigged attachments fall back to the upstream worldray result. Handled transparently via `isRiggedMesh()` guard in `lltoolpie`.
- **Other avatars' rigged attachments**: out of scope for r21 (self-only). May be considered in r22+.

### Documentation

- r21 spec / architecture / known limits / risk register: `docs/ayastorm-r21-self-rigged-picker.md`
- Selection handoff fix investigation / comparison log: `docs/ayastorm-r21-selection-handoff-investigation.md`
- armed mode (experimental) load estimate / measured logs / tuning candidates: `docs/ayastorm-r21-picker-armed-mode.md`
- BoM body rig-hash collision rationale (M4.17): memory `project_skin_hash_collision_bom_body.md`
- Deferred shader routing reference: `docs/ayastorm-deferred-shader-routing.md`
