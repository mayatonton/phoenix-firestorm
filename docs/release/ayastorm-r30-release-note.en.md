# AYAstorm r30 — Release Announcement

**r30 is the first release of the cinematic rendering chapter (r30+)** — it adds a new **Cinematic mode** to AYAstorm and ports the BlackDragon Viewer (BD) 995a1354d8 render pipeline **1:1 verbatim** so it runs alongside AYAstorm's own visual extensions. A single cvar (`AYAVisualRealismEnabled`) switches between **Firestorm View**, **AYAstorm View (r14-r20 extensions)**, and **BlackDragon Cinematic** at viewer restart.

> **Distribution**: r30 ships as a standalone tag (β release first).
>
> **Restart-required mode switch**: switching mode requires a viewer restart. Round-trip behaviour (changing the cvar without restarting) is recommended-only in Phase 4 G4; long-term stability awaits β feedback.

Implementation, port strategy, shader mount table, and acceptance gates are persisted under `docs/specs/`. This note is the entry point and diff highlight.

| Doc | Content |
|---|---|
| [`ayastorm-r30-bd-full-port-inventory.md`](../specs/ayastorm-r30-bd-full-port-inventory.md) | Phase 0 inventory (BD vs AY delta: 49 shaders / 92 cpp / cvar additions) |
| [`ayastorm-r30-bd-full-port-phase1-audit.md`](../specs/ayastorm-r30-bd-full-port-phase1-audit.md) | Phase 1 audit (REDO/REUSE classification + r14-r20 gating) |
| [`ayastorm-r30-bd-full-port-phase2-spec.md`](../specs/ayastorm-r30-bd-full-port-phase2-spec.md) | Phase 2 architectural (D1-D4 dispatch shape) |
| [`ayastorm-r30-bd-full-port-phase3.2-cpp-dispatch-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.2-cpp-dispatch-spec.md) | Phase 3.2 C++ Cinematic dispatch (53 files) |
| [`ayastorm-r30-bd-full-port-phase3.5-ay-only-render-cvar-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.5-ay-only-render-cvar-spec.md) | Phase 3.5 AY-only Render* cvar (26) dispatch |
| [`ayastorm-r30-bd-full-port-phase3.8-shader-cinematic-mount-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.8-shader-cinematic-mount-spec.md) | Phase 3.8 shader 49-file A/B/C/D mount |
| [`ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md) | Phase 3.9 BD UI floater + bdsidebar mount |
| [`ayastorm-r30-bd-full-port-phase4-verify-spec.md`](../specs/ayastorm-r30-bd-full-port-phase4-verify-spec.md) | Phase 4 3-mode acceptance gates (G1-G5) |
| [`ayastorm-r30-bd-full-port-phase5-cleanup-spec.md`](../specs/ayastorm-r30-bd-full-port-phase5-cleanup-spec.md) | Phase 5 cleanup / release prep |

---

## AYAstorm r30 — BlackDragon full port + 3-mode integration

### Headline: one viewer, three modes — Firestorm baseline, AYAstorm extensions, BlackDragon photography

AYAstorm has built up its own visual realism extensions through r14-r20 (atmospheric perspective, cloud volumetric, sun Kelvin modulator, SSS skin marker, translucency, chromatic aberration, SMAA T2x, volumetric godrays, enhanced depth-of-field, etc.). In parallel, BlackDragon Viewer has been evolving toward photography in its own direction, and AYAstorm users wanted the BlackDragon photography experience available from within AYAstorm itself.

r30 ports the BD render pipeline **1:1 verbatim** and integrates **three modes** into one viewer:

| `AYAVisualRealismEnabled` | Mode | Render pipeline | Use case |
|---|---|---|---|
| `0` | Firestorm View | Linden / Firestorm baseline (AY extensions off) | Stream watching / work / lower resource usage |
| `1` | AYAstorm View (default) | AY r14-r20 visual realism extensions | Standard AYAstorm experience |
| `2` | Cinematic | BlackDragon 995a1354d8 pipeline 1:1 port | Photography / machinima |

### How it works: 4-layer dispatch keeping the three modes in parallel

1. **C++ dispatch (Phase 3.7)**: pipeline.cpp / lldrawpool* / llviewershadermgr and 53 files read `AYAVisualRealismEnabled` and per-mode switch shader bind / render state / draw order
2. **Shader permutation (Phase 3.8)**: 49 REDO shaders mounted across 4 strategies
   - **A**: uniform-fed collapse (5 files, no shader edit, neutral values fold the AY branch)
   - **B**: overwrite (9 files, BD baseline replaces AY where AY had no extension)
   - **C**: `#if AYASTORM_CINEMATIC` permutation (26 files, AY and BD coexist in the same file)
   - **D**: dual-file mount (2 files placed under `cinematic_bd/` with shadermgr probe order)
3. **Cvar expansion (Phase 3.3-3.6)**: 7 BD-only cvars added, 7 BD-only sky/water/day presets bundled, 26 AY-only Render* cvars pinned to BD-noop values in Cinematic
4. **UI mount (Phase 3.9)**: in Cinematic mode (`MachinimaSidebar=1`) BD's Machinima Sidebar (panel_machinima, 1021 lines) appears on the right edge of the screen, with `gSideBar->refreshGraphicControls()` providing bidirectional cvar binding

### Settings: normal usage

**No action required for normal usage (mode 1 = AYAstorm View is the default)**. To switch to Cinematic:

| Cvar | Default | Purpose |
|---|---|---|
| `AYAVisualRealismEnabled` | `1` | `0`=Firestorm View / `1`=AYAstorm View / `2`=Cinematic, **restart required** |
| `MachinimaSidebar` | `1` | Show BD Machinima Sidebar when in Cinematic. `0` keeps the pipeline BD but hides the sidebar |
| `RenderShadowAutomaticDistance` | `1` | BD-only automatic shadow distance calculation (active in mode 2) |

### Migration notes

- **AYAstorm View (mode 1) renders identically to r29 and earlier** — upgrading to r30 with no setting changes preserves the existing look.
- Verify Cinematic mode (mode 2) with the Phase 4 acceptance spec ([`docs/specs/ayastorm-r30-bd-full-port-phase4-verify-spec.md`](../specs/ayastorm-r30-bd-full-port-phase4-verify-spec.md)) gates G1-G5.
- Mode switching is **restart-first** for now. Round-trip switching (cvar change without restart) awaits β feedback for stability confirmation.
- Some Machinima Sidebar sliders may share control names with AY extensions; tune AY extensions while back in mode 1.
- BD `panel_preferences_render_settings` / `panel_preferences_ui_colors` are placed as **orphans only** — AY's 17-tab preferences layout is preserved (see [Phase 3.9 §3.3](../specs/ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md)).

### Known caveats

- Cinematic mode + the AY-side `floater_aya_cinematic.xml` can coexist, but the right-edge bdsidebar may overlap UI. Use one at a time.
- BD `llfloatereditsky` / `llfloatereditwater` were never registered upstream in BD; we ported them as orphans to preserve 1:1 fidelity.
- Shaders placed under `cinematic_bd/` retain the standard class3→class2→class1 GPU class fallback chain. Older GPUs automatically fall back as before.
- In Cinematic mode the classic / system avatar body (the base mesh — Ruth/Roth shapes and the un-clothed body part of legacy system outfits) is **excluded from motion blur**. This matches BD baseline (where `LLDrawPoolAvatar::renderMotionBlur` is fully `/* ... */` commented out in BD 995a1354d8). Modern rigged-mesh avatars (hands, hair, clothing, and most attachments) still pick up motion blur via the other pools.

### Acknowledgements

The BlackDragon Viewer (NiranV Dean) render pipeline, panel_machinima UI, and shader set served as the entire reference for this port. The `Copyright (C) 2018, NiranV Dean` headers on `bdfunctions` / `bdsidebar` are preserved verbatim.

---
