🌐 Language: [🇺🇸 English](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-github-release-page.en.md) | [🇯🇵 日本語](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-github-release-page.ja.md) | [🇨🇳 中文](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-github-release-page.zh.md)

# AYAstorm r31-bugfix-2 — AO delete-behavior recovery + LSL Bridge collision defense

> [!IMPORTANT]
> **r31-bugfix-2 is a release that, on the AYAstorm side, stops two structural behaviors that exist across the entire Firestorm viewer family.**
>
> These are not AYAstorm-specific issues; they originate from the shared inventory root used by every Firestorm-derived viewer (whether they are bugs or by-design is upstream's call to make).

1. **AO delete behavior**: in Firestorm-family viewers (upstream Firestorm / older AYAstorm builds / other FS-derived viewers), pressing "Delete" on an AO set permanently erases AO data under the shared inventory root `#Firestorm`, so logging in from a different viewer still shows it gone — a cross-viewer cascading deletion. Observed at ~1000-user scale. In r31-bugfix-2 the actual inventory operation is stopped entirely on the viewer side, and a per-account hidden flag suppresses the set from the UI only
2. **LSL Bridge version collision**: the version-mismatch auto-recreate logic in `fslslbridge.cpp` is structured so that a future minor-version bump on upstream Firestorm could take AYAstorm-side Bridges down with it. Both sides are currently at `v2.29` so it has not fired yet; we ship a one-way defense — `if received version > ours, adopt` — to prevent it

These fixes address structural behaviors that exist across the Firestorm viewer family. We only stop the destructive operation on `#Firestorm` from the AYAstorm side; recurrence in upstream Firestorm or other derived viewers needs each viewer to be patched on its own (the recommended workflow and workarounds are spelled out in the recovery guide).

## For users whose AO sets are already gone — AO re-setup procedure

If you have already pressed "Delete" on an AO set in a Firestorm-family viewer (upstream Firestorm / older AYAstorm / other FS-derived), **the AO data itself cannot be brought back, either by the viewer or by the SL server**. The AO functionality itself, however, can be back in normal working order after a simple re-setup. Procedure is published in 3 languages:

- 🇺🇸 [English Recovery Guide](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.en.md)
- 🇯🇵 [日本語復旧手順](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.ja.md)
- 🇨🇳 [繁體中文復原指南](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.zh.md)

## Release notes

- 🇺🇸 English: [docs/release/ayastorm-r31-bugfix-2-release-note.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-release-note.en.md)
- 🇯🇵 日本語: [docs/release/ayastorm-r31-bugfix-2-release-note.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-release-note.ja.md)
- 🇨🇳 中文: [docs/release/ayastorm-r31-bugfix-2-release-note.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-release-note.zh.md)

## Key documents (tag pinned)

- Technical spec: [docs/specs/ayastorm-r31-2-ao-bridge-recovery.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-r31-2-ao-bridge-recovery.md)
- User recovery guide (3 languages): [docs/guides/ao-data-recovery-guide.{en,ja,zh}.md](https://github.com/mayatonton/phoenix-firestorm/tree/v7.2.4-ayastorm-r31-bugfix-2/docs/guides)

## Compatibility with existing setups

Ships without disturbing r31 / r31-bugfix-1 environments:

- **AO delete-behavior fix**: applies automatically. No setting change required. Users on r31 / r31-bugfix-1 install r31-bugfix-2 on top and the event stops recurring
- **LSL Bridge collision defense**: applies automatically. Prevents AYAstorm-side Bridges from being destroyed when upstream Firestorm bumps the Bridge minor version (not yet firing today, defensive for the future)
- **All r31 / r31-bugfix-1 features** (3D Stream unified tag / AYAstorm View / parcel music Vorbis fix / MOAP audio routing / macOS branding / GPU other-rigged picker / chat tab split / venue reverb / SSS pink-shadow fix, etc.): preserved unchanged
- **Users who don't edit or delete AO sets**: no visible change. The "Delete" button is relabeled "Hide" and the dialog wording changes; that's all
- **Users whose AO sets are already gone**: installing r31-bugfix-2 **stops the same event from recurring**. Lost AO data itself cannot be restored, but the re-setup procedure above gets AO functionality back into working order right away

## IR licence

Venue IRs bundled since r11 (and still shipping) are from OpenAIR (CC-BY 4.0). Sources: [`app_settings/venue_ir/CREDITS.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/indra/newview/app_settings/venue_ir/CREDITS.md)

## Downloads

- [Windows Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-2/Phoenix-FirestormOS-AYAstorm-release_AVX2-7-2-4-261481144_Setup.exe)
- [macOS Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-2/Phoenix-FirestormOS-AYAstorm-release_arm64-7-2-4-81209.dmg)
- [Linux Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-2/Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261481144.tar.xz)

## Contributors

@t-noami @mayatonton
