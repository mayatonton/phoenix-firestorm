# AYAstorm r27 — Release Announcement

GitHub release page copy. **r27 unifies the leftover "Firestorm" branding on macOS to "AYAstorm"** — the menu bar (app menu / Hide / Quit), initial window title, and the standard Apple About panel now all say AYAstorm, while the derived-from attribution `(based on Firestorm)` stays in the About version string.

The change is a surgical two-file edit, so no permanent spec is added; this release note is the first-class document.

---

## AYAstorm r27 — Unify the macOS "Firestorm" branding to "AYAstorm"

### Headline: kill the last "Firestorm" strings that only macOS still showed

On Linux and Windows the window title already showed "AYAstorm" via `VIEWER_CHANNEL = "AYAstorm Release"`. On macOS, however, the menu bar (app menu / Hide / Quit) and the standard Apple About panel still said "Firestorm".

The CMake side (`MACOSX_BUNDLE_BUNDLE_NAME` / `MACOSX_EXECUTABLE_NAME` / `MACOSX_BUNDLE_INFO_STRING`) already set "AYAstorm". The reason the menu bar still showed Firestorm is that **macOS resolves the runtime `CFBundleName` from `English.lproj/InfoPlist.strings`, which overrides the Info.plist value** — a single line in the .strings file was masking everything CMake set.

r27 patches that .strings file and the main-menu nib source `Firestorm.xib`. Two files, no other surfaces touched.

### How it works

```
indra/newview/English.lproj/InfoPlist.strings
  CFBundleName               : "Firestorm" → "AYAstorm"
  CFBundleShortVersionString : "Firestorm version X.X.X" → "AYAstorm version X.X.X (based on Firestorm)"
  CFBundleGetInfoString      : Firestorm → AYAstorm (derived-from attribution kept)

indra/newview/Firestorm.xib
  Main menu "Firestorm" title       → "AYAstorm"
  Apple submenu "Firestorm" title   → "AYAstorm"
  "About Firestorm"                 → "About AYAstorm"
  "Hide Firestorm"                  → "Hide AYAstorm"
  "Quit Firestorm"                  → "Quit AYAstorm"
  Initial window title="Firestorm"  → "AYAstorm"
  window frameAutosaveName="Firestorm" → "AYAstorm"
```

- The other `.lproj` directories (`Japanese.lproj/` `German.lproj/` `Korean.lproj/` etc.) ship only a `language.txt` and no `InfoPlist.strings`. macOS therefore falls back to `CFBundleDevelopmentRegion` (= English) — meaning **every locale's menu bar shows AYAstorm**
- The derived-from credit `(based on Firestorm)` remains in the Apple About panel's version string, so the attribution surface stays intact

### Firestorm credits and contact URL are intentionally untouched

The only strings r27 rewrites are **macOS OS-level branding strings**. The full AYAstorm About floater (Avatar menu → Help → About AYAstorm, defined in `indra/newview/skins/default/xui/en/floater_about.xml`) is not touched at all. In particular:

- The Firestorm Development Team / Additional Contributors / Translators / UI Artists name lists — preserved
- The "For the latest information about Firestorm, visit <https://www.firestormviewer.org>" support URL — preserved
- Linden Lab credits / Licenses / Starlight skin credit — preserved

The standard Apple About panel (App menu → About AYAstorm) is the OS's fixed layout — icon + app name + version string + a single Copyright line — and has never been the place for developer names or contact URLs. The contact URL and the developer credits all live in the Firestorm About floater. r27 does not create any structure where AYAstorm-side bugs would be misrouted to the Firestorm team, or vice versa.

### Settings

**No user action required.** Just launch r27 and the macOS branding follows. Linux / Windows users see no visible difference (they already showed AYAstorm).

### Migration note

- **macOS users**: launching r27 switches the menu bar / Hide / Quit / Apple About / window title to AYAstorm
- **Linux / Windows users**: no visible change this release
- **Streamers and listeners**: no action required on either side

### Known limitations

- **macOS window position is reset once for existing users**: changing `frameAutosaveName` from `Firestorm` to `AYAstorm` changes the NSWindow autosave key. On the first r27 launch the new key (`AYAstorm`) has no stored data yet, so the window opens at the default position. From the second launch onward the new key is reused and behaviour returns to normal
- **Linux / Windows runtime verification is not needed**: out of scope for this PR
- **macOS runtime verification**: deferred to the next Release binary cut — we will confirm that the standard About panel, menu bar, and window title all show AYAstorm

### Implementation summary

- `indra/newview/English.lproj/InfoPlist.strings` — CFBundleName / CFBundleShortVersionString / CFBundleGetInfoString rewritten to AYAstorm, with `(based on Firestorm)` appended to the version string
- `indra/newview/Firestorm.xib` — main menu item titles (`Firestorm` / `About Firestorm` / `Hide Firestorm` / `Quit Firestorm`), the Apple submenu title, the initial window title, and `frameAutosaveName` rewritten to AYAstorm
- Validation: `plutil -lint indra/newview/English.lproj/InfoPlist.strings` (.strings syntax) and `xmllint --noout indra/newview/Firestorm.xib` (xib XML well-formedness) were run by the PR author
- Intentionally untouched: `indra/newview/Info-Firestorm.plist` (template body, already AYAstorm via CMake substitution), `indra/newview/CMakeLists.txt` `MACOSX_BUNDLE_*` (already AYAstorm), `indra/newview/skins/default/xui/en/floater_about.xml` (preserves Firestorm developer credits and support URL)

### Credits

The r27 implementation (the macOS branding audit, identifying the two files that actually drive the runtime strings, the `(based on Firestorm)` attribution design, and the `plutil` / `xmllint` validation) is by [t-noami](https://github.com/t-noami).

On the AYAstorm side we landed the PR as-is and added only this release note (3 languages).

### Documentation

- r25 parcel music Ogg Vorbis fix: [`docs/release/ayastorm-r25-release-note.en.md`](./ayastorm-r25-release-note.en.md)
- r26 MOAP audio → 3D Stream connection: [`docs/release/ayastorm-r26-release-note.en.md`](./ayastorm-r26-release-note.en.md)
