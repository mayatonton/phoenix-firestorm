🌐 Language: [🇺🇸 English](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-github-release-page.en.md) | [🇯🇵 日本語](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-github-release-page.ja.md) | [🇨🇳 中文](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-github-release-page.zh.md)

# AYAstorm r25 — Parcel music Ogg Vorbis live stream 再生修正

r25 は AYAstorm r10.x-bugfix-1 以降で壊れていた **parcel music の Ogg Vorbis live stream 再生を修正する単機能リリース** です。Ogg Opus 再生支援は維持したまま、Icecast Ogg Vorbis live stream が AYAstorm 上でも正しく再生されるようになります。

## リリースノート

- 🇺🇸 English: [docs/ayastorm-r25-release-note.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-release-note.en.md)
- 🇯🇵 日本語: [docs/ayastorm-r25-release-note.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-release-note.ja.md)
- 🇨🇳 中文: [docs/ayastorm-r25-release-note.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-release-note.zh.md)

## 主要ドキュメント (タグ pin)

- r25 投資ログ / 検証 URL / 失敗仮説 / 設計メモ: [docs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md)
- 過去の Opus codec 関連 (r9-opus 系列 historical spec): [docs/specs/spec_5_1ch_opus_decode.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/docs/specs/spec_5_1ch_opus_decode.md)

## 既存環境との互換性

r24 環境を壊さずに出荷します:

- **r25 Ogg Vorbis parcel music 修正**: 配信者・listener 双方とも操作不要。Icecast Ogg Vorbis stream を AYAstorm でも普通に聞けるようになり、vanilla Firestorm / 他 viewer と同等の挙動に戻ります
- **既存の Ogg Opus 配信**: 5.1ch surround Opus を含めて挙動変化なし。`AYAOpusCodecEnable` (default ON) が custom codec を引き続き有効化します
- **MP3 など非 Ogg HTTP stream**: 4-byte `OggS` gate で早期 reject される設計を維持。built-in FMOD codec が処理するため、Parcel Music / 3D Stream HTTP MP3 への影響なし
- **r24 までの全機能** (MOAP audio FMOD 2D、parcel-bound 3D stream、視覚的リアリティ章 r14〜r20、タグベース OBB occlusion、GPU self-rigged picker、chat tab 分離、venue reverb 等): すべて維持

新 cvar 2 件 (`AYAOpusCodecEnable` / `AYAOpusCodecPriority`) は診断専用で、通常運用では既定値のままにしてください。

## IR ライセンス

r11 で同梱した venue IR (現在も出荷中) は OpenAIR (CC-BY 4.0) より。出典は [`app_settings/venue_ir/CREDITS.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r25/indra/newview/app_settings/venue_ir/CREDITS.md)。

## ダウンロード

_3 OS ビルド完了後、@mayatonton が記入します。_

- Windows Installer: _TBD_
- macOS Installer: _TBD_
- Linux Installer: _TBD_

## Contributer

@t-noami @mayatonton
