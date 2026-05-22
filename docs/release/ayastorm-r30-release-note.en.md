# AYAstorm r30 — Release Announcement

**r30 is the first release of the cinematic rendering chapter (r30+)** — it adds a new **Cinematic mode** to AYAstorm and ports a new render engine (995a1354d8) **1:1 verbatim** so it runs alongside AYAstorm's own visual extensions. A single cvar (`AYAVisualRealismEnabled`) switches between **Firestorm View**, **AYAstorm View (r14-r20 extensions)**, and **Cinematic** at viewer restart.

> **Distribution**: r30 ships as a standalone tag (β release first).
>
> **Restart-required mode switch**: switching mode requires a viewer restart. Round-trip behaviour (changing the cvar without restarting) is recommended-only in Phase 4 G4; long-term stability awaits β feedback.

Implementation, port strategy, shader mount table, and acceptance gates are persisted under `docs/specs/`. This note is the entry point and diff highlight.

| Doc | Content |
|---|---|
| [`ayastorm-r30-bd-full-port-inventory.md`](../specs/ayastorm-r30-bd-full-port-inventory.md) | Phase 0 inventory (new render engine vs AY delta: 49 shaders / 92 cpp / cvar additions) |
| [`ayastorm-r30-bd-full-port-phase1-audit.md`](../specs/ayastorm-r30-bd-full-port-phase1-audit.md) | Phase 1 audit (REDO/REUSE classification + r14-r20 gating) |
| [`ayastorm-r30-bd-full-port-phase2-spec.md`](../specs/ayastorm-r30-bd-full-port-phase2-spec.md) | Phase 2 architectural (D1-D4 dispatch shape) |
| [`ayastorm-r30-bd-full-port-phase3.2-cpp-dispatch-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.2-cpp-dispatch-spec.md) | Phase 3.2 C++ Cinematic dispatch (53 files) |
| [`ayastorm-r30-bd-full-port-phase3.5-ay-only-render-cvar-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.5-ay-only-render-cvar-spec.md) | Phase 3.5 AY-only Render* cvar (26) dispatch |
| [`ayastorm-r30-bd-full-port-phase3.8-shader-cinematic-mount-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.8-shader-cinematic-mount-spec.md) | Phase 3.8 shader 49-file A/B/C/D mount |
| [`ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md) | Phase 3.9 UI mount (new render engine env floaters + Cinematic Controls integration, sidebar route retired — §0) |
| [`ayastorm-r30-bd-full-port-phase4-verify-spec.md`](../specs/ayastorm-r30-bd-full-port-phase4-verify-spec.md) | Phase 4 3-mode acceptance gates (G1-G5) |
| [`ayastorm-r30-bd-full-port-phase5-cleanup-spec.md`](../specs/ayastorm-r30-bd-full-port-phase5-cleanup-spec.md) | Phase 5 cleanup / release prep |

---

## AYAstorm r30 — new render engine full port + 3-mode integration

### Headline: one viewer, three modes — Firestorm baseline, AYAstorm extensions, new render engine photography

AYAstorm has built up its own visual realism extensions through r14-r20 (atmospheric perspective, cloud volumetric, sun Kelvin modulator, SSS skin marker, translucency, chromatic aberration, SMAA T2x, volumetric godrays, enhanced depth-of-field, etc.). In parallel, a separate render engine has been evolving toward photography in its own direction, and AYAstorm users wanted that photography experience available from within AYAstorm itself.

r30 ports that new render engine **1:1 verbatim** and integrates **three modes** into one viewer:

| `AYAVisualRealismEnabled` | Mode | Render pipeline | Use case |
|---|---|---|---|
| `0` | Firestorm View | Linden / Firestorm baseline (AY extensions off) | Stream watching / work / lower resource usage |
| `1` | AYAstorm View | AY r14-r20 visual realism extensions | r29-era AYAstorm experience |
| `2` | Cinematic (default) | New render engine 995a1354d8 pipeline 1:1 port | Photography / machinima |

### How it works: 4-layer dispatch keeping the three modes in parallel

1. **C++ dispatch (Phase 3.7)**: pipeline.cpp / lldrawpool* / llviewershadermgr and 53 files read `AYAVisualRealismEnabled` and per-mode switch shader bind / render state / draw order
2. **Shader permutation (Phase 3.8)**: 49 REDO shaders mounted across 4 strategies
   - **A**: uniform-fed collapse (5 files, no shader edit, neutral values fold the AY branch)
   - **B**: overwrite (9 files, new render engine baseline replaces AY where AY had no extension)
   - **C**: `#if AYASTORM_CINEMATIC` permutation (26 files, AY and new render engine coexist in the same file)
   - **D**: dual-file mount (2 files placed under `cinematic_bd/` with shadermgr probe order)
3. **Cvar expansion (Phase 3.3-3.6)**: 7 new-render-engine-only cvars added, 7 sky/water/day presets bundled, 26 AY-only Render* cvars pinned to noop values matching the new render engine in Cinematic
4. **UI mount (Phase 3.9)**: Cinematic mode controls live in **`Avatar → Cinematic Controls...` (`Alt+C`) → `floater_aya_cinematic.xml`**. The earlier design ported the new render engine's `panel_machinima.xml` (1021 lines) plus a dedicated sidebar, but we consolidated into the Cinematic Controls floater so AYAstorm's existing floater scheme stays consistent; the sidebar route was retired (see [Phase 3.9 spec §0](../specs/ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md))

### Settings: normal usage

**No action required for normal usage (mode 2 = Cinematic is the default)**. To switch back to AYAstorm View / Firestorm View:

| Cvar | Default | Purpose |
|---|---|---|
| `AYAVisualRealismEnabled` | `2` | `0`=Firestorm View / `1`=AYAstorm View / `2`=Cinematic, **restart required** |
| `RenderShadowAutomaticDistance` | `1` | New-engine-only automatic shadow distance calculation (active in mode 2) |

Per-parameter tuning in Cinematic mode is done from the Cinematic Controls floater opened via `Avatar → Cinematic Controls...` (`Alt+C`).

### Migration notes

- **r30's default is now Cinematic (mode 2)**. To keep the r29-era look (= AYAstorm View), set `AYAVisualRealismEnabled` to `1` in Debug Settings and restart.
- Verify Cinematic mode (mode 2) with the Phase 4 acceptance spec ([`docs/specs/ayastorm-r30-bd-full-port-phase4-verify-spec.md`](../specs/ayastorm-r30-bd-full-port-phase4-verify-spec.md)) gates G1-G5.
- Mode switching is **restart-first** for now. Round-trip switching (cvar change without restart) awaits β feedback for stability confirmation.
- Some Machinima Sidebar sliders may share control names with AY extensions; tune AY extensions while back in mode 1.
- The new render engine's `panel_preferences_render_settings` / `panel_preferences_ui_colors` are placed as **orphans only** — AY's 17-tab preferences layout is preserved (see [Phase 3.9 §3.3](../specs/ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md)).

### Known caveats

- `llfloatereditsky` / `llfloatereditwater` were never registered upstream in the new render engine; we ported them as orphans to preserve 1:1 fidelity.
- Shaders placed under `cinematic_bd/` retain the standard class3→class2→class1 GPU class fallback chain. Older GPUs automatically fall back as before.
- In Cinematic mode the classic / system avatar body (the base mesh — Ruth/Roth shapes and the un-clothed body part of legacy system outfits) is **excluded from motion blur**. This matches the new render engine baseline (where `LLDrawPoolAvatar::renderMotionBlur` is fully `/* ... */` commented out at 995a1354d8). Modern rigged-mesh avatars (hands, hair, clothing, and most attachments) still pick up motion blur via the other pools.

### Acknowledgements

An external photography-oriented render engine implementation (by NiranV Dean) served as the entire reference for this port. The `Copyright (C) 2018, NiranV Dean` header on `bdfunctions` is preserved verbatim.

---
