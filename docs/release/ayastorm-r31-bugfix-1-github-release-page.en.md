🌐 Language: [🇺🇸 English](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/release/ayastorm-r31-bugfix-1-github-release-page.en.md) | [🇯🇵 日本語](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/release/ayastorm-r31-bugfix-1-github-release-page.ja.md) | [🇨🇳 中文](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/release/ayastorm-r31-bugfix-1-github-release-page.zh.md)

# AYAstorm r31-bugfix-1 — FullBright-prim SSS pink-shadow leak fix

r31-bugfix-1 is a **single-bug fix release** that removes the avatar-shaped pink shadow leaking through FullBright prims when SSS skin is enabled. The bug has existed structurally since SSS was first wired up in the r20+ visual realism chapter and survived two prior 3-4 hour investigations.

The fix was found by stepping back from ad-hoc `if`-branch attempts and building **four effect-axis rendering routing maps in parallel** (SSS / FullBright / Glow / Environment). Four independent investigations converged on the same root cause — `gbuffer3.a` staleness after single-RT FB passes — before any pipeline edit was made. The actual fix is a one-block dispatch move in `pipeline.cpp` with no FB shader changes.

## Release notes

- 🇺🇸 English: [docs/release/ayastorm-r31-bugfix-1-release-note.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/release/ayastorm-r31-bugfix-1-release-note.en.md)
- 🇯🇵 日本語: [docs/release/ayastorm-r31-bugfix-1-release-note.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/release/ayastorm-r31-bugfix-1-release-note.ja.md)
- 🇨🇳 中文: [docs/release/ayastorm-r31-bugfix-1-release-note.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/release/ayastorm-r31-bugfix-1-release-note.zh.md)

## Key documents (tag pinned)

- SSS rendering routing: [docs/specs/ayastorm-sss-rendering-routing.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-sss-rendering-routing.md)
- FullBright rendering routing (case D discussion §9.1): [docs/specs/ayastorm-fullbright-rendering-routing.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-fullbright-rendering-routing.md)
- Glow rendering routing: [docs/specs/ayastorm-glow-rendering-routing.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-glow-rendering-routing.md)
- Environment rendering routing: [docs/specs/ayastorm-environment-rendering-routing.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-environment-rendering-routing.md)
- PR #112 (merged): [https://github.com/mayatonton/phoenix-firestorm/pull/112](https://github.com/mayatonton/phoenix-firestorm/pull/112)

## Compatibility with existing setups

Ships without disturbing r31 environments:

- **SSS pink-shadow leak fix**: applies automatically. No user setting change required. Users on r31 install r31-bugfix-1 on top and the FB-prim pink-shadow leak disappears
- **All r31 features** (3D Stream unified tag, AYAstorm View, parcel music Vorbis fix, MOAP audio routing, macOS branding, GPU other-rigged picker, chat tab split, venue reverb, etc.): preserved unchanged
- **Non-SSS users**: no visible change. The dispatch reorder is functionally a no-op when no SSS skin pixels are in scene
- **FB shaders**: completely untouched. The fix is fully reversible

## IR licence

Venue IRs bundled in r11 (and still shipping) are from OpenAIR (CC-BY 4.0). Sources: [`app_settings/venue_ir/CREDITS.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/indra/newview/app_settings/venue_ir/CREDITS.md).

## Downloads

- [Windows Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-1/Phoenix-FirestormOS-AYAstorm-release_AVX2-7-2-4-261441503_Setup.exe)
- [macOS Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-1/Phoenix-FirestormOS-AYAstorm-release_arm64-7-2-4-81209.dmg)
- [Linux Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-1/Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261442337.tar.xz)

## Contributors

@mayatonton
