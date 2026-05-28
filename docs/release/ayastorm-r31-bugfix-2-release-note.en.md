# AYAstorm r31-bugfix-2 — Release Announcement

> [!IMPORTANT]
> **r31-bugfix-2 is a release that, on the AYAstorm side, stops two structural behaviors that exist across the entire Firestorm viewer family.** It prevents recurrence of the AO-set loss now observed at ~1000-user scale, and provides a one-way defense against future LSL Bridge mutual-destruction caused by version drift.
>
> These are not AYAstorm-specific issues; they originate from the shared `#Firestorm` inventory root used by all Firestorm-derived viewers (whether they count as bugs or by-design is upstream's call to make).

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
