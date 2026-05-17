🌐 Language: [🇺🇸 English](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-github-release-page.en.md) | [🇯🇵 日本語](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-github-release-page.ja.md) | [🇨🇳 中文](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-github-release-page.zh.md)

# AYAstorm r25 — 土地音乐 Ogg Vorbis live stream 播放修复

r25 是一个 **单功能 release,修复了 AYAstorm r10.x-bugfix-1 以后一直损坏的土地音乐 Ogg Vorbis live stream 播放**。Ogg Opus 支持完整保留,Icecast Ogg Vorbis live stream 在 AYAstorm 上也能正确播放。

## Release notes

- 🇺🇸 English: [docs/ayastorm-r25-release-note.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-release-note.en.md)
- 🇯🇵 日本語: [docs/ayastorm-r25-release-note.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-release-note.ja.md)
- 🇨🇳 中文: [docs/ayastorm-r25-release-note.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-release-note.zh.md)

## 主要文档 (tag pin)

- r25 调查日志 / 验证 URL / 失败假设 / 设计笔记: [docs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md)
- 既往 Opus codec 相关 (r9-opus 系列 historical spec): [docs/specs/spec_5_1ch_opus_decode.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/specs/spec_5_1ch_opus_decode.md)

## 与既有环境的兼容性

不破坏 r24 环境出货:

- **r25 Ogg Vorbis 土地音乐修复**: 配信者、listener 双方均无需操作。Icecast Ogg Vorbis stream 在 AYAstorm 上也能正常收听,与 vanilla Firestorm / 其他 viewer 的行为一致 — 回归正确
- **既有 Ogg Opus 配信**: 包含 5.1ch surround Opus 在内行为不变。`AYAOpusCodecEnable` (default ON) 持续启用 custom codec
- **MP3 等非 Ogg HTTP stream**: 维持 4-byte `OggS` gate 提前 reject 的设计,built-in FMOD codec 照常处理。对 Parcel Music / 3D Stream HTTP MP3 无影响
- **r24 之前的全部功能** (MOAP audio FMOD 2D、parcel-bound 3D stream、视觉真实感章 r14〜r20、tag-based OBB occlusion、GPU self-rigged picker、chat tab 分离、venue reverb 等): 全部保留

新增的 2 个 cvar (`AYAOpusCodecEnable` / `AYAOpusCodecPriority`) 仅供诊断用,正常使用请保持默认值。

## IR 许可

r11 同捆的 venue IR (现在仍在出货) 来自 OpenAIR (CC-BY 4.0)。来源: [`app_settings/venue_ir/CREDITS.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/indra/newview/app_settings/venue_ir/CREDITS.md)。

## 下载

_3 OS 构建完成后, @mayatonton 填写。_

- Windows Installer: _TBD_
- macOS Installer: _TBD_
- Linux Installer: _TBD_

## Contributer

@t-noami @mayatonton
