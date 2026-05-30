# AYAstorm r31-bugfix-2 — Release Announcement

> [!IMPORTANT]
> **r31-bugfix-2 is a release that, on the AYAstorm side, stops two structural behaviors that exist across the entire Firestorm viewer family.** It prevents recurrence of the AO-set loss now observed at ~1000-user scale, and provides a one-way defense against future LSL Bridge mutual-destruction caused by version drift.
>
> These are not AYAstorm-specific issues; they originate from the shared `#Firestorm` inventory root used by all Firestorm-derived viewers (whether they count as bugs or by-design is upstream's call to make).

Alongside the AO + Bridge fix, r31-bugfix-2 also delivers a **second major fix** — attachment N-BL prim alpha render-order via 3-pass dispatch (PR #122) — plus three other bundled fixes that landed after r31-bugfix-1: 3D Stream URL filter + UI update (PR #121), a Cinematic-mode glow min-luminance bugfix (PR #123), and an underwater alpha plate redirect fix (PR #124). See the per-feature sections below.

Implementation details, scope analysis, and the recovery procedure live permanently under `docs/specs/` and `docs/guides/`. This note is the entry point and highlight summary.

---

## AYAstorm r31-bugfix-2 — AO delete-behavior recovery + LSL Bridge collision defense

### Headline: Replace AO set "Delete" with non-destructive Hide; one-way defense against LSL Bridge mutual deletion

In Firestorm-family viewers (upstream Firestorm / older AYAstorm builds / other FS-derived viewers), pressing "Delete" on an AO set used to call `purgeFolder` on the inventory entity (the folder under `#Firestorm/#AO` along with every animation and notecard inside it). Because `#Firestorm` is the shared root used by every Firestorm-derived viewer, a delete in one viewer would propagate: log into another viewer (including upstream Firestorm) and the AO was still gone. This is a cross-viewer cascading deletion.

In r31-bugfix-2, the normal AO-set "Delete" operation no longer deletes real inventory. It records a per-account hidden flag and suppresses the set from the UI. The only remaining destructive path is the explicit `Delete selected` action inside the Hidden manager, behind a confirmation dialog. Alongside this, we fix the LSL Bridge auto-recreate-on-version-mismatch logic so that a future minor-version bump in upstream Firestorm will not silently destroy the AYAstorm-side Bridge.

This behavior was not introduced in r31. It has been structurally present in the Firestorm viewer family for a long time, affecting every FS-derived viewer including AYAstorm (whether it counts as a bug or by-design is upstream's call to make).

### Background — why it happened

**AO delete behavior**:
- `aoengine.cpp::removeSet()` calls `purgeFolder(catID, true)`, which **recursively deletes the real inventory folder**
- The `#Firestorm/#AO/<set name>` folder is gone from the server
- Because `#Firestorm` is the shared root across Firestorm-derived viewers, the deletion propagates to every viewer
- Once it happens, the viewer cannot recover the data (it is gone on the SL server)

**LSL Bridge version collision**:
- In `fslslbridge.cpp:239`, any received bridge version string that doesn't exactly match `mCurrentFullName` triggers `recreateBridge()`
- `finishBridge()` then calls `cleanUpOldVersions()`, which deletes every older-versioned Bridge object from `#Firestorm/#LSL Bridge`
- Upstream Firestorm and AYAstorm are currently both at `v2.29`, so this has not fired yet — but the moment Firestorm bumps to `v2.30`, AYAstorm-side Bridges will be destroyed

### How the fix works

**AO delete → soft hide**:
- Completely rewrote `removeSet()`; the `purgeFolder` call is gone
- The set's inventory UUID is appended to a per-account setting `FSAOHiddenSets` (LLSD array, Persist=1)
- During AO enumeration (`update()`), hidden filter excludes them from the UI
- New "Manage hidden sets" floater added; supports per-UUID restore as well as restore-all
- Hidden / visible name-collision guards prevent creating or importing a set whose name is currently hidden, and prevent restoring a hidden set over an already-visible set with the same name
- The Hidden manager now has `Delete selected`. This is separate from normal Remove: after a confirmation dialog, it permanently deletes only the selected hidden set's real inventory folder
- Delete dialog re-worded in 3 languages; the normal AO-set action is presented as "Hide" rather than "Delete", with explicit text that inventory is preserved
- The AO-set soft-hide button now uses a visibility-off icon instead of a trash icon

**LSL Bridge one-way fix**:
- Parse the received version string into `major.minor` and compare numerically
- Received > ours ⇒ **adopt without destruction** (update `mBridgeUUID` / `mCurrentURL` only; do not call `recreateBridge`)
- Received == ours ⇒ existing behavior
- Received < ours ⇒ existing behavior (`recreateBridge` to update)
- Parse failure ⇒ existing behavior (fail safe)
- The same version comparison is also used during startup attach / detach decisions, so a newer bridge is not detached before its `BridgeVer` message arrives
- The newer-bridge adopt path now shares the normal handshake completion path, including `URL Confirmed` and first-time settings sync

### Migration note

- **No user-side configuration change required.** Users on r31 can simply install r31-bugfix-2 over the top
- All r31 features remain intact (3D Stream unified tag / AYAstorm View / parcel music Vorbis fix / MOAP routing / macOS branding / other-rigged picker)
- The r31-bugfix-1 SSS pink-shadow fix is also retained
- For already-affected users, the inventory data **cannot be recovered on the viewer side**. The procedure to get AO functionality working again lives permanently at [`docs/guides/ao-data-recovery-guide.en.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.en.md)

### Known limitations / future work

- **One-way defense only**: if AYAstorm version-leads upstream Firestorm, upstream Firestorm (still unpatched) will continue to destroy AYAstorm-side Bridges. AYA leading FS is rare, but the long-term fix is either an upstream PR or root separation (`#Firestorm/` → `#AYAstorm/`)
- **Upstream Firestorm "Delete AO" is still destructive**: recommended workflow is to centralize AO edit/delete on AYAstorm r31.2+ and treat upstream Firestorm as read-only for AO (documented in the recovery guide)
- **UI fallback for hidden sets**: if the new UI is broken in some environment, opening Debug Settings (`Ctrl+Alt+Shift+S`) and clearing `FSAOHiddenSets` to an empty array restores every hidden set. This cannot restore a folder that was explicitly permanently deleted through `Delete selected` in the Hidden manager

### Implementation summary

- `indra/newview/aoengine.cpp` / `aoengine.h` — `removeSet()` soft hide, `getHiddenSets()` / `unhideSet()` / `unhideAllSets()` / `isSetHidden()`, hidden filter in `update()`, plus permanent delete and name-collision helpers for hidden sets
- `indra/newview/ao.cpp` / `ao.h` — `FloaterAOHiddenSets` controller + Manage hidden sets / Restore / Delete selected button wiring
- `indra/newview/llviewerfloaterreg.cpp` — register `ao_hidden_sets` floater
- `indra/newview/fslslbridge.cpp` / `fslslbridge.h` — bridge-version comparison helpers, startup attach acceptance for newer bridges, adopt path, shared handshake completion
- `indra/newview/app_settings/settings_per_account.xml` — `FSAOHiddenSets` (LLSD, Persist=1)
- `indra/newview/skins/default/xui/{en,ja,zh}/notifications.xml` — `RemoveAOSet` re-worded, hidden-set conflict notifications, permanent-delete confirmation
- `indra/newview/skins/default/xui/{en,ja,zh}/panel_ao.xml` — "Manage hidden sets" button, AO-set soft-hide icon / tooltip adjustments
- `indra/newview/skins/default/xui/{en,ja,zh}/floater_ao_hidden_sets.xml` — new floater (3 languages)
- `indra/newview/skins/default/xui/en/floater_ao.xml` — floater height adjustments
- `docs/specs/ayastorm-r31-2-ao-bridge-recovery.md` — technical spec (new)
- `docs/guides/ao-data-recovery-guide.{en,ja,zh}.md` — user-facing recovery procedure (new, 3 languages)

### Credits

- [@t-noami](https://github.com/t-noami) — macOS build for r31-bugfix-2, plus continuing implementation contributions to AYAstorm overall (r24 Dullahan audio callback / r25 Ogg Vorbis codec / r26 3D Stream media ring / r27 macOS branding, and more).
- [@mayatonton](https://github.com/mayatonton) — r31-bugfix-2 AO soft hide / LSL Bridge collision defense implementation, scope analysis, and 3-language recovery-guide authoring.

### Documentation

- Technical spec: [`docs/specs/ayastorm-r31-2-ao-bridge-recovery.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-r31-2-ao-bridge-recovery.md)
- User recovery guide (English): [`docs/guides/ao-data-recovery-guide.en.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.en.md)
- User recovery guide (Japanese): [`docs/guides/ao-data-recovery-guide.ja.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.ja.md)
- User recovery guide (Traditional Chinese): [`docs/guides/ao-data-recovery-guide.zh.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.zh.md)

---

## For users whose AO sets are already gone — AO re-setup procedure

> [!IMPORTANT]
> If your AO sets have already been erased by the deletion event described above, **the AO data itself cannot be brought back, either by the viewer or by the SL server**. The AO functionality itself, however, can be back in normal working order after a simple re-setup. **Step-by-step recovery guides are published in 3 languages — please use the one matching your environment:**
>
> - 🇺🇸 [**English Recovery Guide**](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.en.md)
> - 🇯🇵 [**日本語復旧手順**](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.ja.md)
> - 🇨🇳 [**繁體中文復原指南**](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.zh.md)
>
> Installing r31-bugfix-2 itself **stops the same event from recurring on the AYAstorm side**. Recurrence on upstream Firestorm / other FS-derived viewers needs each viewer to be patched on its own — workarounds for that case are spelled out inside the recovery guide.

---

## Attachment N-BL prim alpha render-order — 3-pass dispatch (PR [#122](https://github.com/mayatonton/phoenix-firestorm/pull/122))

### Headline: Second major fix in r31-bugfix-2 — POST_WATER forward pass split into SIM N-BL → R-BL → attachment N-BL using per-draw discriminator

This is the **second major fix** bundled in r31-bugfix-2 alongside the AO + Bridge recovery work. The r30 §5 render-order swap (POST_WATER all non-rigged → all rigged) resolved sky-bleed through rigged hair, but caused attachment N-BL prims (eyelash prims, etc.) to be drawn before rigged hair and then over-blended away, producing a visible regression on certain avatars (S1 / S2 in the spec terminology). The POST_WATER forward pass is now split into three sub-passes:

- **pass 1**: `forwardRender(false, ATTACHMENT_NONE)` — SIM rezz N-BL only
- **pass 2**: `forwardRender(true)` — all R-BL (rigged hair, etc.)
- **pass 3**: `forwardRender(false, ATTACHMENT_ONLY)` — attachment N-BL prims only

The per-draw discriminator is `LLDrawInfo::mAttachedToAvatar.notNull()`. Moving pass 3 after rigged places attachment prims in front of hair, while pass 1 still draws SIM-side N-BL before rigged so the original §5 swap fix (sky-bleed through hair) is preserved. PRE_WATER stays rigged-first for water-fog coherence. HUD uses a single forwardRender call and is out of scope.

The falsified alternatives (independent depth on the alpha plate, etc.) and the structural-compare argument for the 3-pass approach are recorded in `docs/specs/ayastorm-double-alpha-c-plan-extension.md §3`.

### Implementation summary

- `indra/newview/lldrawpoolalpha.cpp` / `lldrawpoolalpha.h` — `AttachmentFilter` enum and `forwardRender(rigged, filter)` overload; POST_WATER split into 3 sub-passes; per-draw `LLDrawInfo::mAttachedToAvatar` discriminator
- `docs/specs/ayastorm-double-alpha-c-plan-extension.md` — structural compare against falsified A/B/C alternatives (new)
- `docs/specs/ayastorm-six-category-render-order-trace.md` — full 6-category render order trace used to derive the 3-pass boundary (new)

### Credits

- [@mayatonton](https://github.com/mayatonton) — 3-pass dispatch design, implementation, falsification analysis (A/B/C alternatives), and spec authoring.

### Special thanks

- neria (neriamm) — alpha render-order issue investigation and verification help. Reproduction setups across multiple SL avatars and hands-on verification of the candidate fixes meaningfully shortened the structural compare and the final implementation choice. neria is a Second Life resident contributor (not a GitHub account).

### Documentation

- Structural compare (extension report): [`docs/specs/ayastorm-double-alpha-c-plan-extension.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-double-alpha-c-plan-extension.md)
- 6-category render order trace: [`docs/specs/ayastorm-six-category-render-order-trace.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-six-category-render-order-trace.md)

---

## 3D Stream URL filter and UI update (PR [#121](https://github.com/mayatonton/phoenix-firestorm/pull/121))

### Headline: 3D Stream ships disabled by default; URL-play prompts now show the sending object's identity

The 3D Stream feature (introduced in r26 / formalised under unified tag in r31) is shipped with `Stream3DEnabled = false` from r31.2 onward. Users who were already running with 3D Stream enabled keep their persisted setting; only fresh installs land on default OFF. The URL-play confirmation prompt that opens when an in-world object sends a stream URL now displays the source object name and owner, so each viewer can decide whether to accept the URL based on who is sending it. Unintended auto-play from rezzed objects is also blocked at the source-of-truth level, not after-the-fact in the UI.

### How the fix works

- `settings.xml` ships `Stream3DEnabled` default `false`. Existing users keep their persisted value
- The URL-play confirmation dialog text now includes the source object name + owner (3 languages)
- Source-of-truth enforcement: when 3D Stream is disabled, rezzed-object URL emit is dropped before reaching the auto-play path, not after

### Credits

- [@mayatonton](https://github.com/mayatonton) — 3D Stream URL filter and UI update implementation, plus 3-language prompt localization.

### Documentation

- Technical report (Japanese): [`docs/specs/ayastorm-r31-2-3dstream-url-filter-and-ui-update-report.ja.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-r31-2-3dstream-url-filter-and-ui-update-report.ja.md)
- User guide (English): [`docs/specs/3dstream-user-guide.en.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/3dstream-user-guide.en.md)
- User guide (Japanese): [`docs/specs/3dstream-user-guide.ja.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/3dstream-user-guide.ja.md)
- User guide (Traditional Chinese): [`docs/specs/3dstream-user-guide.zh.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/3dstream-user-guide.zh.md)

---

## Cinematic glow min-luminance bugfix (PR [#123](https://github.com/mayatonton/phoenix-firestorm/pull/123))

### Headline: Raise Cinematic-mode `RenderGlowMinLuminance` from 0.0 to 0.5; one-shot migration force-corrects shipped users

The BD-parity overlay used in Cinematic mode carried `RenderGlowMinLuminance = 0.0`, which lowered the bloom-extract threshold in HDR linear space. `glowExtractF.glsl` computes bloom contribution via `smoothstep(min, min+1.0, x)`, so with `min = 0.0` the middle-lit pixels in the HDR `0..1.0` range fire bloom, and the `warmth = max(r*0.75, g*0.6, b*0.712)` branch lets color-tinted prims (blank texture + color picker) light up even with Glow set to zero on the prim. Symptoms were reported on both attachment and SIM rez objects, while textured prims and white-colored prims were not affected.

The threshold is raised to `0.5`. Strong Cinematic bloom sources (sky, intense reflections, strong emissives — anything above HDR `0.5`) still fire; the unintended middle-lit prim bloom is cut. A one-shot migration (sentinel `AYAR31GlowMinLuminanceMigrationVersion`) force-corrects users who have the `0.0` value persisted from r31.0 / r31.1, but only on the next Cinematic-mode boot — Firestorm-mode-only users are not touched (LL default `1.0` stays in place; the migration is skipped and re-checked on the next Cinematic boot).

### How the fix works

- `settings_cinematic_bd.xml`: `RenderGlowMinLuminance` `0.0` → `0.5`
- `settings.xml`: `AYAR31GlowMinLuminanceMigrationVersion` sentinel (S32, Persist=1, default 0) added
- `llcinematicoverlay.{h,cpp}`: `applyR31GlowMinLuminanceMigrationIfNeeded()` implemented. Guard: if `AYAVisualRealismEnabled != 2` (Firestorm mode), skip without bumping the version, so the next Cinematic boot re-checks
- `llappviewer.cpp`: migration call added to the startup sequence after the existing `applyR15GodraysCinematicMigrationIfNeeded()` call

### Implementation summary

- `indra/newview/app_settings/settings_cinematic_bd.xml` — `RenderGlowMinLuminance` threshold raise
- `indra/newview/app_settings/settings.xml` — migration sentinel
- `indra/newview/llcinematicoverlay.cpp` / `llcinematicoverlay.h` — `applyR31GlowMinLuminanceMigrationIfNeeded()` with mode==2 guard
- `indra/newview/llappviewer.cpp` — startup-sequence integration

### Credits

- [@mayatonton](https://github.com/mayatonton) — bloom threshold investigation, fix design, and one-shot migration implementation.

---

## Underwater alpha plate redirect fix (PR [#124](https://github.com/mayatonton/phoenix-firestorm/pull/124))

### Headline: Gate `mAYAAlphaColor` redirect on `!sUnderWaterRender`; underwater forward alpha goes straight to main RT (FS-compatible)

The r30 P5 transparent-DoF C-(a) work introduced an `mAYAAlphaColor` redirect that routes forward alpha BLEND writes to a separate alpha plate, then composites the plate over the main RT before tonemapping (`GL_ONE / GL_ONE_MINUS_SRC_ALPHA`). The condition that enables this redirect (`use_alpha_rt`) did not gate on `LLPipeline::sUnderWaterRender`. Underwater, the main RT carries the underwater fog-tinted opaque scene while the separated alpha plate is cleared to `(0,0,0,0)`; the pre-tonemap composite then lets the plate paint over the underwater-tinted main RT. Eyelash / brow attachment alpha prims and SIM particle transparent regions rendered as solid black underwater, and the same break transiently reappeared for a few frames after surfacing (the camera ascends, the redirect briefly returns to its above-water state, then drifts back into the broken underwater path until the next surface event).

The fix is a single-condition gate: `use_alpha_rt` now additionally requires `!LLPipeline::sUnderWaterRender`. Underwater, the redirect is skipped and forward alpha writes directly to the main RT — matching upstream FS behavior underwater. Above water, behavior is identical to before.

### How the fix works

- `indra/newview/lldrawpoolalpha.cpp`: `use_alpha_rt` condition adds `!LLPipeline::sUnderWaterRender &&` immediately before the existing `gPipeline.mAYAAlphaColor.isComplete()` check
- When the gate suppresses the redirect, the alpha plate stays at its cleared `(0,0,0,0)`. The pre-tonemap composite is then a no-op against the main RT (`A_plate * 1 + RT * 1 = RT`), so no underwater-specific composite path is needed
- The above-water transparent-DoF C-(a) plate composite is unchanged. Underwater bokeh is structurally invisible (full-frame fog), so dropping the plate composite underwater has no perceptible cost

### Implementation summary

- `indra/newview/lldrawpoolalpha.cpp` — `use_alpha_rt` gated on `!LLPipeline::sUnderWaterRender`; inline comment documents the underwater no-op composite reasoning

### Credits

- [@mayatonton](https://github.com/mayatonton) — underwater symptom investigation, single-gate fix, side-effect trace across the 5 connection points (`mForwardToAlphaRT`, alpha blend factors, emissive routing, plate clear, plate composite).
