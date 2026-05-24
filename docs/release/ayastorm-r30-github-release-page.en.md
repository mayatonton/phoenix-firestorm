🌐 Language: [🇺🇸 English](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r30-github-release-page.en.md) | [🇯🇵 日本語](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r30-github-release-page.ja.md) | [🇨🇳 中文](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r30-github-release-page.zh.md)

# AYAstorm r30 — Photography-grade render engine ships as the new AYAstorm View + parcel music Ogg Vorbis fix + MOAP 3D stream routing + macOS branding + other-avatar rigged picker

A single jump from r24 to r30 bundles five releases (r25, r26, r27, r28, r30) into one tag. r29 was skipped during chapter pivoting. The headline feature is r30 — the photography-grade render engine built across the r30 chapter (P1–P6) now ships as the new "AYAstorm View", with the prior r14–r20 visual-realism layer reachable as opt-in additions on top of it. For the full notes per release, see the per-language release notes in the repo (tag-pinned permalinks — they will not break when docs are updated later).

## Release notes (per release × language)
- 🇺🇸 English: [docs/release/ayastorm-r30-release-note.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r30-release-note.en.md)
- 🇯🇵 日本語: [docs/release/ayastorm-r30-release-note.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r30-release-note.ja.md)
- 🇨🇳 中文: [docs/release/ayastorm-r30-release-note.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r30-release-note.zh.md)

### Photography chapter (r30)
- **r30** — View Mode picker reshuffle: Cinematic promoted to the new AYAstorm View. Velocity buffer / SMAA T2x / Volumetric Light / BD-class DoF chain / Motion Blur / Chromatic Aberration / 35-cvar AYAstorm Controls floater. Restart-required mode switching, one-shot migration from the r14–r20 AYAstorm View: [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r30-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r30-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r30-release-note.zh.md)

### Media audio (parcel music / MOAP)
- **r25** — Parcel music Ogg Vorbis live stream playback fix (broken since r10.x-bugfix-1, Ogg Opus support preserved): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r25-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r25-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r25-release-note.zh.md)
- **r26** — MOAP audio routes through 3D Stream's speaker routing (`{ch:L}/{ch:R}/{ch:FL}/...` distributed stereo / 5.1 placement / HRTF / venue reverb / occlusion now work from MOAP faces): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r26-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r26-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r26-release-note.zh.md)

### Cross-platform polish
- **r27** — macOS branding unification (menu bar / window title / Apple About panel now say "AYAstorm", `(based on Firestorm)` attribution preserved in About): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r27-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r27-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r27-release-note.zh.md)

### UX & picker
- **r28** — Other rigged picker (extends r21 GPU object-ID picker to other avatars' rigged attachments, one armed target at a time, by @t-noami): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r28-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r28-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r28-release-note.zh.md)

## Key documents (tag-pinned)

### Photography chapter (r30)
- r30 release decision (single source of truth): [docs/specs/ayastorm-r30-view-mode-reshuffle.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/ayastorm-r30-view-mode-reshuffle.md)
- r30 chapter status block (P1–P6 lineage, points forward to reshuffle): [docs/specs/ayastorm-r30-cinematic-chapter.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/ayastorm-r30-cinematic-chapter.md)
- P1 restart-switch infrastructure: [docs/specs/ayastorm-r30-p1-view-mode-restart-switch.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/ayastorm-r30-p1-view-mode-restart-switch.md)
- BD live cvar port reference: [docs/specs/ayastorm-r30-bd-full-port-phase6-live-cvar-port-spec.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/ayastorm-r30-bd-full-port-phase6-live-cvar-port-spec.md)
- Cinematic Controls floater audit: [docs/specs/ayastorm-r30-cinematic-controls-cleanup.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/ayastorm-r30-cinematic-controls-cleanup.md)
- Skin SSS user guide (avatar photography): [docs/specs/skin-sss-user-guide.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/skin-sss-user-guide.md) / [.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/skin-sss-user-guide.ja.md) / [.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/skin-sss-user-guide.zh.md)

### Media audio
- r25 parcel music Ogg Vorbis investigation: [docs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md)
- r26 MOAP 3D stream implementation plan: [docs/ayastorm-r26-moap-3d-stream-implementation-plan.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/ayastorm-r26-moap-3d-stream-implementation-plan.md)
- Cumulative 3D stream tag guide (r6 onward): [docs/guides/3dstream-tag-guide.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/guides/3dstream-tag-guide.en.md) / [.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/guides/3dstream-tag-guide.ja.md) / [.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/guides/3dstream-tag-guide.zh.md)

### UX & picker
- r28 other rigged picker spec: [docs/specs/ayastorm-r28-other-rigged-picker.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/ayastorm-r28-other-rigged-picker.md)
- r21 self-rigged picker spec (sibling feature): [docs/specs/ayastorm-r21-self-rigged-picker.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/ayastorm-r21-self-rigged-picker.md)
- Rigged Mesh Picker — GPU object-ID buffer technical spec (for other viewer forks): [docs/specs/rigged-mesh-picker-gpu-buffer.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/rigged-mesh-picker-gpu-buffer.md) / [.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/rigged-mesh-picker-gpu-buffer.ja.md) / [.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/rigged-mesh-picker-gpu-buffer.zh.md)

## Public disclosure: double alpha-block fix (cross-viewer common bug)

A forward alpha BLEND rendering bug common to all SL viewers (LL / Firestorm / Alchemy / Black Dragon) — "double alpha block" — and the 2-line fix AYAstorm adopted is published as a 3-language public spec + verification screenshots, so other viewer forks can pull it in without a PR. A dedicated reference branch `fix/double-alpha-block` is kept as a permanent anchor (HEAD tracks the latest doc revision):

- Primary spec (English): [docs/specs/double-alpha-block-fix.md](https://github.com/mayatonton/phoenix-firestorm/blob/fix/double-alpha-block/docs/specs/double-alpha-block-fix.md)
- 日本語: [docs/specs/double-alpha-block-fix.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/fix/double-alpha-block/docs/specs/double-alpha-block-fix.ja.md)
- 中文: [docs/specs/double-alpha-block-fix.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/fix/double-alpha-block/docs/specs/double-alpha-block-fix.zh.md)
- Reference branch: [fix/double-alpha-block](https://github.com/mayatonton/phoenix-firestorm/tree/fix/double-alpha-block)

## Compatibility with existing setups

All features ship without breaking r24 setups:

- **r30 view-mode reshuffle**: existing r24-era AYAstorm View users (`AYAVisualRealismEnabled = 1`) are silently migrated to the new AYAstorm View (`= 2`) by an idempotent one-shot migration on first launch. No user-facing prompt. The r14–r20 visual-realism layer is reachable as opt-in additions on top of the new engine through the AYAstorm Controls floater (`Alt+C`). Switching to Firestorm View remains available for streaming / low-resource use. Restart required to switch modes.
- **r25 parcel music Ogg Vorbis fix**: Icecast Ogg Vorbis live streams now play correctly. Full Ogg Opus support preserved. No setting required.
- **r26 MOAP 3D stream routing**: existing 3D Stream tag conventions (`{ch:L}/{ch:R}/{ch:FL}/...`) now apply to MOAP face URLs. Streamers without these tags are unaffected.
- **r27 macOS branding**: pure surface-level rename on macOS. No functional change; non-macOS builds unaffected.
- **r28 other rigged picker**: GPU object-ID buffer pass extended to other avatars. Master switch `FSOtherRiggedPickerEnable` defaults ON; `FSOtherRiggedPickerGPU` kill-switch retained. Default-camera gate and 1.0 s arm window keep load bounded. The r21 self picker is unchanged.

## IR license

Venue IRs bundled in r11 (and still shipping) are from OpenAIR (CC-BY 4.0). Sources in [`app_settings/venue_ir/CREDITS.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/indra/newview/app_settings/venue_ir/CREDITS.md).

## Render engine credit

The new AYAstorm View pipeline draws extensively from NiranV Dean's Black Dragon viewer (LGPL-2.1, license matched to viewerlgpl). The port is acknowledged in `floater_about.xml` and BD repository commit references are preserved in port-spec headers.

## Downloads

_To be filled in by @mayatonton after the 3 OS build completes._

- Windows Installer
- macOS Installer
- Linux Installer

## Contributors

@t-noami @mayatonton
