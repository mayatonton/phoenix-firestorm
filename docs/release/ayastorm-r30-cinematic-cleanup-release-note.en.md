# AYAstorm r30 Cinematic Cleanup — Release Announcement

**r30 Cinematic Cleanup polishes the Cinematic Controls floater** after the Phase 6 live cvar port — auditing all 35 cvars / 9 tabs end-to-end, then reviving controls whose dispatch was missing in code and removing controls that could never produce a visible effect.

The full audit and per-cvar trace lives in the permanent spec (`docs/specs/ayastorm-r30-cinematic-controls-cleanup.md`). This note is the entry point and migration summary.

---

## AYAstorm r30 Cinematic Cleanup

### Headline: every slider in the floater now actually does something

After Phase 6 shipped 13 live cvars in the Cinematic Controls floater, a full code-trace audit revealed that **five of the surfaced controls had no effect** — either because the cvar value range exceeded what `pipeline.cpp` actually dispatches, or because the cvar was bypassed entirely by another code path. Cleanup resolves all five.

### What changed

| ID | Control | Disposition | Effect |
|---|---|---|---|
| A.1 | Shadow Detail slider max value | **Narrowed from 0–3 to 0–2** | Level 3 was a no-op (LL/BD share the same `mSpotShadow[]` array for spot and projector lights — level 2 already enables both). Tooltip corrected |
| A.2 | `RenderFSAAType = 3` | **Now dispatches SMAA + T2x** | Was a no-op (only 1=FXAA, 2=SMAA had branches). FSAAType is now the single AA selector: 0=Off, 1=FXAA, 2=SMAA, 3=SMAA+T2x |
| A.3 | `RenderShadowResolutionScale` in Cinematic mode | **Now applies to per-cascade Vector4** | Was bypassed by the Cinematic per-channel shadow path. Scale now multiplies into both `RenderShadowResolution` and `RenderProjectorShadowResolution`, with a 64-px floor to prevent zero-size allocation |
| A.4 | "Deferred Rendering" checkbox in General tab | **Removed** | Cinematic mode is deferred-only by design (forward path was never maintained for it). Header text now states the deferred requirement explicitly. The cvar itself is unchanged and remains accessible via Preferences → Graphics → Advanced Lighting Model for non-Cinematic use |
| A.5 | `RenderSMAAT2x` checkbox + cvar | **Removed** | Functionality absorbed into A.2 (`FSAAType = 3`). Single enum is easier to reason about than two parallel toggles |

### Migration note

- **`RenderSMAAT2x` is gone.** If you previously set it to `1` in debug settings, switch to `RenderFSAAType = 3` for the equivalent effect. The old cvar value is silently ignored (no warning) — the `pipeline.cpp` dispatch was already inert in Cinematic mode for BD parity reasons, so the user-visible impact is zero
- **`RenderDeferred` checkbox moved.** Use Preferences → Graphics → Advanced Lighting Model instead. Cinematic mode requires it ON
- **`RenderShadowDetail = 3` no longer selectable from the floater.** Debug settings still allow it, but it is equivalent to `= 2` in dispatch (no behavior change)

### Documentation

- Full cleanup spec + per-cvar audit: [`docs/specs/ayastorm-r30-cinematic-controls-cleanup.md`](../specs/ayastorm-r30-cinematic-controls-cleanup.md)
- Parent spec (r30 chapter): [`docs/specs/ayastorm-r30-cinematic-chapter.md`](../specs/ayastorm-r30-cinematic-chapter.md)
- Phase 6 (live cvar port, the change this cleans up): [`docs/specs/ayastorm-r30-bd-full-port-phase6-live-cvar-port-spec.md`](../specs/ayastorm-r30-bd-full-port-phase6-live-cvar-port-spec.md)
