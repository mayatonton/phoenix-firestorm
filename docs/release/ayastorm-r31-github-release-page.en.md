🌐 Language: [🇺🇸 English](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r31-github-release-page.en.md) | [🇯🇵 日本語](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r31-github-release-page.ja.md) | [🇨🇳 中文](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r31-github-release-page.zh.md)

# AYAstorm r31+bundle-fs.80646 — 3D Stream unified tag + photography-grade render engine (AYAstorm View) + parcel music Ogg Vorbis fix + MOAP 3D stream routing + macOS branding + other-avatar rigged picker + Firestorm upstream FS-7.1.18.80646 carry-over

A single jump from r24 to r31 bundles six releases (r25, r26, r27, r28, r30, r31) into one tag, together with the Firestorm upstream FS-7.1.18.80646 carry-over. r29 was skipped during chapter pivoting. The headline this time is r31 — the 3D Stream tag is unified under `[3dstream:...]`, so a single prefix now covers mono URL streams and linkset distributed routing (stereo / 5.1 / MOAP). The r30 photography-grade render engine (AYAstorm View) and the r25–r28 audio / branding / picker features ship in the same bundle. For the full notes per release, see the per-language release notes in the repo (tag-pinned permalinks — they will not break when docs are updated later).

## Release notes (per release × language)

### Headline: 3D Stream chapter (r31)
- 🇺🇸 English: [docs/release/ayastorm-r31-release-note.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r31-release-note.en.md)
- 🇯🇵 日本語: [docs/release/ayastorm-r31-release-note.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r31-release-note.ja.md)
- 🇨🇳 中文: [docs/release/ayastorm-r31-release-note.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r31-release-note.zh.md)

### 3D Stream chapter (r31)
- **r31** — 3D Stream unified tag: a single `[3dstream:...]` prefix now covers both mono URL streams and linkset distributed routing (stereo / 5.1 / MOAP). Legacy `[3dstream-stereo:...]` / `[ayastream:...]` / `[ayastream-stereo:...]` are still accepted. `{bin}` / `{binaural}` default changes to `off` (venues opt in). 5.1ch codec guidance refreshed for Vorbis 6ch / Opus 6ch / FLAC 6ch: [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r31-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r31-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r31-release-note.zh.md)

### Photography chapter (r30)
- **r30** — View Mode picker reshuffle: Cinematic promoted to the new AYAstorm View. Velocity buffer / SMAA T2x / Volumetric Light / BD-class DoF chain / Motion Blur / Chromatic Aberration / 35-cvar AYAstorm Controls floater. Restart-required mode switching, one-shot migration from the r14–r20 AYAstorm View: [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r30-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r30-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r30-release-note.zh.md)

### Media audio (parcel music / MOAP)
- **r25** — Parcel music Ogg Vorbis live stream playback fix (broken since r10.x-bugfix-1, Ogg Opus support preserved): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r25-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r25-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r25-release-note.zh.md)
- **r26** — MOAP audio routes through 3D Stream's speaker routing (`{ch:L}/{ch:R}/{ch:FL}/...` distributed stereo / 5.1 placement / HRTF / venue reverb / occlusion now work from MOAP faces): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r26-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r26-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r26-release-note.zh.md)

### Cross-platform polish
- **r27** — macOS branding unification (menu bar / window title / Apple About panel now say "AYAstorm", `(based on Firestorm)` attribution preserved in About): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r27-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r27-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r27-release-note.zh.md)

### UX & picker
- **r28** — Other rigged picker (extends r21 GPU object-ID picker to other avatars' rigged attachments, one armed target at a time, by @t-noami): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r28-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r28-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r28-release-note.zh.md)

### Firestorm upstream carry-over
- **Firestorm FS-7.1.18.80646 bundle** — Firestorm upstream FS-7.1.18.80646 carried over into AYAstorm. This is a platform refresh of the viewer base with no dedicated release note; it ships inside the umbrella tag. For the Firestorm-side change list, see the [Firestorm release notes](https://wiki.firestormviewer.org/release_notes).

## Key documents (tag-pinned)

### 3D Stream chapter (r31)
- r31 unified tag spec (single source of truth): [docs/specs/ayastorm-r31-3dstream-unified-tag.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r31-3dstream-unified-tag.md)
- Cumulative 3D stream tag guide (r6 onward): [docs/guides/3dstream-tag-guide.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/guides/3dstream-tag-guide.en.md) / [.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/guides/3dstream-tag-guide.ja.md) / [.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/guides/3dstream-tag-guide.zh.md)
- 3D stream user-facing how-to: [docs/specs/3dstream-user-guide.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/3dstream-user-guide.en.md) / [.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/3dstream-user-guide.ja.md) / [.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/3dstream-user-guide.zh.md)

### Photography chapter (r30)
- r30 release decision (single source of truth): [docs/specs/ayastorm-r30-view-mode-reshuffle.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r30-view-mode-reshuffle.md)
- r30 chapter status block (P1–P6 lineage, points forward to reshuffle): [docs/specs/ayastorm-r30-cinematic-chapter.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r30-cinematic-chapter.md)
- P1 restart-switch infrastructure: [docs/specs/ayastorm-r30-p1-view-mode-restart-switch.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r30-p1-view-mode-restart-switch.md)
- BD live cvar port reference: [docs/specs/ayastorm-r30-bd-full-port-phase6-live-cvar-port-spec.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r30-bd-full-port-phase6-live-cvar-port-spec.md)
- Cinematic Controls floater audit: [docs/specs/ayastorm-r30-cinematic-controls-cleanup.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r30-cinematic-controls-cleanup.md)
- Skin SSS user guide (avatar photography): [docs/specs/skin-sss-user-guide.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/skin-sss-user-guide.md) / [.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/skin-sss-user-guide.ja.md) / [.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/skin-sss-user-guide.zh.md)

### Media audio
- r25 parcel music Ogg Vorbis investigation: [docs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md)
- r26 MOAP 3D stream implementation plan: [docs/specs/ayastorm-r26-moap-3d-stream-implementation-plan.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r26-moap-3d-stream-implementation-plan.md)

### UX & picker
- r28 other rigged picker spec: [docs/specs/ayastorm-r28-other-rigged-picker.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r28-other-rigged-picker.md)
- r21 self-rigged picker spec (sibling feature): [docs/specs/ayastorm-r21-self-rigged-picker.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r21-self-rigged-picker.md)
- Rigged Mesh Picker — GPU object-ID buffer technical spec (for other viewer forks): [docs/specs/rigged-mesh-picker-gpu-buffer.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/rigged-mesh-picker-gpu-buffer.md) / [.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/rigged-mesh-picker-gpu-buffer.ja.md) / [.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/rigged-mesh-picker-gpu-buffer.zh.md)

## Public disclosure: double alpha-block fix (cross-viewer common bug)

A forward alpha BLEND rendering bug common to all SL viewers (LL / Firestorm / Alchemy / Black Dragon) — "double alpha block" — and the 2-line fix AYAstorm adopted is published as a 3-language public spec + verification screenshots, so other viewer forks can pull it in without a PR. A dedicated reference branch `fix/double-alpha-block` is kept as a permanent anchor (HEAD tracks the latest doc revision):

- Primary spec (English): [docs/specs/double-alpha-block-fix.md](https://github.com/mayatonton/phoenix-firestorm/blob/fix/double-alpha-block/docs/specs/double-alpha-block-fix.md)
- 日本語: [docs/specs/double-alpha-block-fix.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/fix/double-alpha-block/docs/specs/double-alpha-block-fix.ja.md)
- 中文: [docs/specs/double-alpha-block-fix.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/fix/double-alpha-block/docs/specs/double-alpha-block-fix.zh.md)
- Reference branch: [fix/double-alpha-block](https://github.com/mayatonton/phoenix-firestorm/tree/fix/double-alpha-block)

## Compatibility with existing setups

All features ship without breaking r24 setups:

- **r31 3D Stream unified tag**: prefer `[3dstream:...]` for new content. Legacy `[3dstream-stereo:...]` / `[ayastream:...]` / `[ayastream-stereo:...]` continue to be accepted, so existing content does not need rewriting. The `{bin}` / `{binaural}` default change applies only to the new `[3dstream:...]` tag — venues that want binaural should opt in with `{bin:on}`.
- **r30 view-mode reshuffle**: existing r24-era AYAstorm View users (`AYAVisualRealismEnabled = 1`) are silently migrated to the new AYAstorm View (`= 2`) by an idempotent one-shot migration on first launch. No user-facing prompt. The r14–r20 visual-realism layer is reachable as opt-in additions on top of the new engine through the AYAstorm Controls floater (`Alt+C`). Switching to Firestorm View remains available for streaming / low-resource use. Restart required to switch modes.
- **r25 parcel music Ogg Vorbis fix**: Icecast Ogg Vorbis live streams now play correctly. Full Ogg Opus support preserved. No setting required.
- **r26 MOAP 3D stream routing**: existing 3D Stream tag conventions (`{ch:L}/{ch:R}/{ch:FL}/...`) now apply to MOAP face URLs. Streamers without these tags are unaffected.
- **r27 macOS branding**: pure surface-level rename on macOS. No functional change; non-macOS builds unaffected.
- **r28 other rigged picker**: GPU object-ID buffer pass extended to other avatars. Master switch `FSOtherRiggedPickerEnable` defaults ON; `FSOtherRiggedPickerGPU` kill-switch retained. Default-camera gate and 1.0 s arm window keep load bounded. The r21 self picker is unchanged.
- **Firestorm upstream FS-7.1.18.80646**: viewer base platform refresh. AYAstorm-specific features and existing setups are unaffected.

## IR license

Venue IRs bundled in r11 (and still shipping) are from OpenAIR (CC-BY 4.0). Sources in [`app_settings/venue_ir/CREDITS.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/indra/newview/app_settings/venue_ir/CREDITS.md).

## Render engine credit

The AYAstorm View pipeline draws extensively from NiranV Dean's Black Dragon viewer (LGPL-2.1, license matched to viewerlgpl). The port is acknowledged in `floater_about.xml` and BD repository commit references are preserved in port-spec headers.

## Downloads

_To be filled in by @mayatonton after the 3 OS build completes._

- Windows Installer
- macOS Installer
- Linux Installer

## Contributors

@t-noami @mayatonton
