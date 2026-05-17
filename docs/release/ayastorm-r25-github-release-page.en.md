🌐 Language: [🇺🇸 English](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-github-release-page.en.md) | [🇯🇵 日本語](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-github-release-page.ja.md) | [🇨🇳 中文](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-github-release-page.zh.md)

# AYAstorm r25 — Parcel music Ogg Vorbis live stream playback fix

r25 is a **single-feature release that fixes parcel music Ogg Vorbis live stream playback**, broken on AYAstorm since r10.x-bugfix-1. Full Ogg Opus support is preserved; Icecast Ogg Vorbis live streams now play correctly on AYAstorm.

## Release notes

- 🇺🇸 English: [docs/ayastorm-r25-release-note.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-release-note.en.md)
- 🇯🇵 日本語: [docs/ayastorm-r25-release-note.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-release-note.ja.md)
- 🇨🇳 中文: [docs/ayastorm-r25-release-note.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-release-note.zh.md)

## Key documents (tag pinned)

- r25 investigation log / verification URLs / failure hypotheses / design notes: [docs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md)
- Prior Opus codec work (r9-opus series historical spec): [docs/specs/spec_5_1ch_opus_decode.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/specs/spec_5_1ch_opus_decode.md)

## Compatibility with existing setups

Ships without disturbing r24 environments:

- **r25 Ogg Vorbis parcel music fix**: no streamer or listener action required. Icecast Ogg Vorbis streams now play on AYAstorm just like on vanilla Firestorm / other viewers — restoring correct behaviour
- **Existing Ogg Opus streams**: unchanged, including 5.1ch surround Opus. `AYAOpusCodecEnable` (default ON) keeps the custom codec active
- **MP3 and other non-Ogg HTTP streams**: still rejected early by the 4-byte `OggS` gate so built-in FMOD codecs handle them. No impact on Parcel Music / 3D Stream HTTP MP3
- **All r24 features** (MOAP audio over FMOD 2D, parcel-bound 3D stream, visual realism chapter r14–r20, tag-based OBB occlusion, GPU self-rigged picker, chat tab split, venue reverb, etc.): preserved

The two new cvars (`AYAOpusCodecEnable` / `AYAOpusCodecPriority`) are diagnostic only. Leave defaults as-is for normal use.

## IR licence

Venue IRs bundled in r11 (and still shipping) are from OpenAIR (CC-BY 4.0). Sources: [`app_settings/venue_ir/CREDITS.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/indra/newview/app_settings/venue_ir/CREDITS.md).

## Downloads

_To be filled in by @mayatonton after the 3 OS build completes._

- Windows Installer: _TBD_
- macOS Installer: _TBD_
- Linux Installer: _TBD_

## Contributors

@t-noami @mayatonton
