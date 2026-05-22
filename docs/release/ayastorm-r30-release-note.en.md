# AYAstorm r30 — Release Announcement

**r30 ships the photography-grade render engine that AYAstorm has been building since the r30 chapter opened — and it does so as the new "AYAstorm View"**. The view-mode picker now shows two modes — `Firestorm View` and `AYAstorm View` — and the new engine (velocity buffer + SMAA T2x + Volumetric Light + BD-class DoF chain + Motion Blur + Chromatic Aberration + 35-cvar Cinematic Controls floater) is what AYAstorm View now means.

> **Distribution**: r30 ships as one of the features in the r25–r30 bundle tag. Other release notes for the bundled releases are linked directly from the GitHub Release page.

Implementation lineage (P1 restart-switch infrastructure → P2 velocity buffer → P3 Volumetric Light → P4 BD DoF chain → P5 BD parity gate → P6 live BD cvar port → Phase 6 Controls Cleanup → r30 release picker reshuffle) and the migration / residual-code design are preserved as historical specs under `docs/specs/`. The single source of truth for the r30 release decision is `docs/specs/ayastorm-r30-view-mode-reshuffle.md`. This note is the entry point and diff highlight.

---

## AYAstorm r30 — View Mode picker reshuffle: Cinematic promoted to AYAstorm View

### Headline: AYAstorm View is now the new render engine

Through r14–r20, AYAstorm built a visual-realism extension layer on top of Firestorm's baseline — what users saw as "AYAstorm View" in the r24 picker. Through the r30 chapter (P1–P6), a parallel render-engine port was added as a third mode internally called "Cinematic", aiming to reach BD-class photography quality.

For r30 release we made an editorial call:

- The new engine reached the chapter goal.
- Asking users to remember three modes (Firestorm View / AYAstorm View / Cinematic) for what amounts to "the engine got better" was the wrong framing.

So **the new engine ships as the new AYAstorm View**. The picker is reduced to two visible modes:

| Picker entry | Render pipeline | Use case |
|---|---|---|
| Firestorm View | Linden / Firestorm baseline | Stream watching, work, lower resource usage |
| **AYAstorm View** (default) | New render engine (velocity buffer / SMAA T2x / Volumetric Light / BD-class DoF / Motion Blur / Chromatic Aberration / r14–r20 extensions on opt-in) | Photography, machinima, everyday immersive use |

### What happens on first launch after upgrading

If you were on the r24-era AYAstorm View (persisted as `AYAVisualRealismEnabled = 1`), an idempotent one-shot migration on startup rewrites it to `2` (the new AYAstorm View) and stamps `AYAViewModeMigrationVersion = 1`. You'll see a log line like:

```
View mode migration v0->v1: AYAVisualRealismEnabled 1 (legacy AYAstorm View) -> 2 (new AYAstorm View)
```

There is no user-facing prompt. From your perspective the engine simply got better.

### Mode switching is restart-required

Switching between Firestorm View and AYAstorm View applies after restarting AYAstorm. The pipeline builds once at startup, so a running session always reflects one and only one mode. (Per the r30 chapter design — runtime gates were intentionally not adopted to avoid the kinds of bugs that come from mid-frame pipeline reconfiguration.)

### AYAstorm Controls (Alt+C)

The Cinematic Controls floater that was introduced during the chapter is now reachable as **`AYAstorm → AYAstorm Controls...`** (`Alt+C`). It exposes the new engine's tunables — BD live cvars (shadow, DoF, fullbright, lights, global light, post FX), per-channel shadow tuning, 6 cvars to individually opt-in r14–r20 AYA visual-realism effects on top of the new engine, and so on.

> **Internal naming is preserved.** The internal mode index `AYAVisualRealismEnabled == 2`, the `AYACinematicModeActive` helper cvar, `AYASTORM_CINEMATIC` shader `#define`, `LLCinematicOverlay` namespace, `floater_aya_cinematic.xml` file, and `settings_cinematic_bd.xml` overlay all remain as-is. Renaming them would have been a churn-only refactor with no user-visible benefit — and the BD upstream lineage in commits / comments would lose readability. The promotion is a UI-only rename plus a one-shot migration. See `docs/specs/ayastorm-r30-view-mode-reshuffle.md` §2.2.

### What about the old AYAstorm View?

The r14–r20 AYAstorm View (mode `1`) is **removed from the picker** but the code path is preserved. If you want to revisit it for any reason, you can set `AYAViewModeMigrationVersion = 0` **and** `AYAVisualRealismEnabled = 1` in Debug Settings together and restart. The migration will run again on the next startup and rewrite you back to mode `2` — this is intentional (the chapter's editorial position is that the engine got replaced, not that you have two engines to choose from).

The same individual r14–r20 effects (atmospheric perspective, godrays, aerial perspective, color temperature, cloud volumetric, translucency, avatar skin SSS) are reachable as **opt-in additions on top of the new engine** through the AYAstorm Controls floater. The new AYAstorm View ships with these defaulted to OFF; turn them on if you want the r14–r20 layer back on top of the new pipeline.

### Settings

For normal usage, no action is required (AYAstorm View is the default).

| Cvar | Default | Purpose |
|---|---|---|
| `AYAVisualRealismEnabled` | `2` | `0` = Firestorm View / `2` = AYAstorm View (new engine). **Restart required.** Setting `1` is reachable only via Debug Settings + `AYAViewModeMigrationVersion=0`, and the migration will rewrite it on next restart |
| `AYAViewModeMigrationVersion` | `0` → `1` post-migration | Idempotent gate for the one-shot migration. Do not edit unless you intentionally want to re-trigger migration |

The new engine's per-parameter tuning lives in the **AYAstorm Controls** floater (`Alt+C`).

### Known limitations

- **Restart required for mode switch**: live in-session mode switching was intentionally not adopted. Switching applies on next startup.
- **Old AYAstorm View is unsupported**: reachable via Debug Settings as documented above, but not surface-level supported in r30 — the migration is intentionally irreversible by normal means.
- **System body (Ruth/Roth) excluded from motion blur**: matches the new engine baseline (where `LLDrawPoolAvatar::renderMotionBlur` is fully commented out). Modern rigged-mesh avatars still get motion blur via the other pools.
- **macOS OpenGL deprecation watch**: as with the rest of the chapter, the new engine's shader chains rely on GL features that may need fallbacks in future macOS toolchains. No regressions known at r30 ship.

### Implementation summary

- 10 files changed for the picker reshuffle commit (`6a6b657441`): `settings.xml`, `llcinematicoverlay.{h,cpp}` (migration helper), `llappviewer.cpp` (startup ordering), `panel_preferences_graphics1.xml` (en/ja), `menu_viewer.xml` (en), `floater_aya_cinematic.xml` (en/ja), `floater_about.xml` (en).
- Migration ordering: `applyAYAViewModeMigrationIfNeeded()` runs **before** `applyCinematicOverlayIfNeeded()` so upgrading users get the new-engine overlay applied in the same startup.
- The full lineage (P1 → P6 + Controls Cleanup + view-mode reshuffle) is captured across the r30 specs in `docs/specs/`.

### Credits

The new-engine pipeline draws extensively from an external photography-oriented render engine (NiranV Dean's Black Dragon viewer, LGPL-2.1, license matched to viewerlgpl). The port is acknowledged in `floater_about.xml` and the BD repository commit references are preserved in port-spec headers.

### Documentation

- r30 release decision (single source of truth): `docs/specs/ayastorm-r30-view-mode-reshuffle.md`
- Chapter status block (historical, points forward to reshuffle): `docs/specs/ayastorm-r30-cinematic-chapter.md`
- P1 restart-switch infrastructure (historical): `docs/specs/ayastorm-r30-p1-view-mode-restart-switch.md`
- Phase-specific specs (P2 velocity buffer / P3 Volumetric Light / P4 DoF chain / P5 BD parity / P6 live cvar port / Cinematic Controls Cleanup): `docs/specs/ayastorm-r30-*.md`
- BD live cvar port reference: `docs/specs/ayastorm-r30-bd-full-port-phase6-live-cvar-port-spec.md`
- Cinematic Controls floater audit: `docs/specs/ayastorm-r30-cinematic-controls-cleanup.md`
