🌐 Language: [🇺🇸 English](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r31-github-release-page.en.md) | [🇯🇵 日本語](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r31-github-release-page.ja.md) | [🇨🇳 中文](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r31-github-release-page.zh.md)

# AYAstorm r31+bundle-fs.80646 — 3D Stream unified tag + 撮影品質描画エンジン (AYAstorm View) + parcel music Ogg Vorbis fix + MOAP 3D stream routing + macOS branding 統一 + 他人 avatar rigged picker + Firestorm upstream FS-7.1.18.80646 取込

r24 から r31 への一括移行 tag。6 release (r25 / r26 / r27 / r28 / r30 / r31) を 1 つの tag に同梱し、あわせて Firestorm upstream FS-7.1.18.80646 を取込みます。r29 は章移行中に skip されました。今回の柱は r31 — 3D Stream のタグを `[3dstream:...]` に一本化し、mono URL stream と linkset 分散 routing (stereo / 5.1 / MOAP) を同一 prefix で書けるようにしています。r30 の撮影品質描画エンジン (AYAstorm View)、r25〜r28 の audio / branding / picker 機能も同梱出荷します。各 release の完全な note は repo 内の per-language release note を参照 (tag-pinned permalink — 後日 docs が更新されても壊れません)。

## Release notes (release × 言語)

### 今回の柱: 3D Stream chapter (r31)
- 🇺🇸 English: [docs/release/ayastorm-r31-release-note.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r31-release-note.en.md)
- 🇯🇵 日本語: [docs/release/ayastorm-r31-release-note.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r31-release-note.ja.md)
- 🇨🇳 中文: [docs/release/ayastorm-r31-release-note.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r31-release-note.zh.md)

### 3D Stream chapter (r31)
- **r31** — 3D Stream unified tag: `[3dstream:...]` 1 種類で mono URL stream と linkset 分散 routing (stereo / 5.1 / MOAP) を表記。既存 `[3dstream-stereo:...]` / `[ayastream:...]` / `[ayastream-stereo:...]` は全て互換受付。`{bin}` / `{binaural}` の既定値が off へ変更 (会場側 opt-in)。5.1ch codec 案内を Vorbis 6ch / Opus 6ch / FLAC 6ch の現状に整理: [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r31-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r31-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r31-release-note.zh.md)

### 撮影描画章 (r30)
- **r30** — View Mode picker reshuffle: Cinematic を新しい AYAstorm View へ promote。velocity buffer / SMAA T2x / Volumetric Light / BD クラス DoF chain / Motion Blur / Chromatic Aberration / 35 cvar の AYAstorm Controls floater。再起動必須のモード切替、r14〜r20 AYAstorm View からの 1-shot migration: [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r30-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r30-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r30-release-note.zh.md)

### Media audio (parcel music / MOAP)
- **r25** — parcel music Ogg Vorbis live stream 再生 fix (r10.x-bugfix-1 以降の regression、Ogg Opus サポートは維持): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r25-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r25-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r25-release-note.zh.md)
- **r26** — MOAP audio を 3D Stream の speaker routing 経由に (`{ch:L}/{ch:R}/{ch:FL}/...` 分散ステレオ / 5.1 配置 / HRTF / venue reverb / occlusion が MOAP 面でも動作): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r26-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r26-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r26-release-note.zh.md)

### Cross-platform polish
- **r27** — macOS branding 統一 (menu bar / window title / Apple About panel が「AYAstorm」表記に、About 内の `(based on Firestorm)` 表記は維持): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r27-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r27-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r27-release-note.zh.md)

### UX & picker
- **r28** — 他人 rigged picker (r21 GPU object-ID picker を他人 avatar の rigged attachment へ拡張、同時 armed は 1 人、by @t-noami): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r28-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r28-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/release/ayastorm-r28-release-note.zh.md)

### Firestorm upstream 取込
- **Firestorm FS-7.1.18.80646 bundle** — Firestorm upstream FS-7.1.18.80646 を AYAstorm に取込。viewer 基盤の platform refresh で、専用の release note はなく、umbrella tag に同梱されます。Firestorm 側の change list は [Firestorm release notes](https://wiki.firestormviewer.org/release_notes) を参照

## 主要ドキュメント (tag-pinned)

### 3D Stream chapter (r31)
- r31 unified tag spec (単一の真実): [docs/specs/ayastorm-r31-3dstream-unified-tag.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r31-3dstream-unified-tag.md)
- 3D stream tag guide (r6 以降の累積): [docs/guides/3dstream-tag-guide.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/guides/3dstream-tag-guide.en.md) / [.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/guides/3dstream-tag-guide.ja.md) / [.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/guides/3dstream-tag-guide.zh.md)
- 3D stream user guide (使い方): [docs/specs/3dstream-user-guide.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/3dstream-user-guide.en.md) / [.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/3dstream-user-guide.ja.md) / [.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/3dstream-user-guide.zh.md)

### 撮影描画章 (r30)
- r30 release 判断の単一の真実: [docs/specs/ayastorm-r30-view-mode-reshuffle.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r30-view-mode-reshuffle.md)
- r30 章 status block (P1–P6 系譜、reshuffle へ forward 参照): [docs/specs/ayastorm-r30-cinematic-chapter.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r30-cinematic-chapter.md)
- P1 再起動切替インフラ: [docs/specs/ayastorm-r30-p1-view-mode-restart-switch.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r30-p1-view-mode-restart-switch.md)
- BD live cvar port reference: [docs/specs/ayastorm-r30-bd-full-port-phase6-live-cvar-port-spec.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r30-bd-full-port-phase6-live-cvar-port-spec.md)
- Cinematic Controls floater audit: [docs/specs/ayastorm-r30-cinematic-controls-cleanup.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r30-cinematic-controls-cleanup.md)
- Skin SSS ユーザーガイド (アバター撮影向け): [docs/specs/skin-sss-user-guide.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/skin-sss-user-guide.md) / [.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/skin-sss-user-guide.ja.md) / [.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/skin-sss-user-guide.zh.md)

### Media audio
- r25 parcel music Ogg Vorbis investigation: [docs/specs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md)
- r26 MOAP 3D stream implementation plan: [docs/specs/ayastorm-r26-moap-3d-stream-implementation-plan.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r26-moap-3d-stream-implementation-plan.md)

### UX & picker
- r28 他人 rigged picker spec: [docs/specs/ayastorm-r28-other-rigged-picker.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r28-other-rigged-picker.md)
- r21 self rigged picker spec (姉妹機能): [docs/specs/ayastorm-r21-self-rigged-picker.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/ayastorm-r21-self-rigged-picker.md)
- Rigged Mesh Picker — GPU object-ID buffer 技術資料 (他 viewer 取込用): [docs/specs/rigged-mesh-picker-gpu-buffer.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/rigged-mesh-picker-gpu-buffer.md) / [.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/rigged-mesh-picker-gpu-buffer.ja.md) / [.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/docs/specs/rigged-mesh-picker-gpu-buffer.zh.md)

## 公開資料: 二重 alpha block fix (他 SL viewer 共通 bug の公開)

LL / Firestorm / Alchemy / Black Dragon 全 SL viewer に共通する forward alpha BLEND 描画 bug (「二重 alpha block」) と、AYAstorm が採用した 2 行修正を、他 viewer fork が PR 不要で取込めるよう 3 言語の公開資料 + 検証スクショとして公開しています。専用 reference branch `fix/double-alpha-block` を永続的に保持しています (HEAD は最新 doc revision に追従):

- 一次資料 (英語): [docs/specs/double-alpha-block-fix.md](https://github.com/mayatonton/phoenix-firestorm/blob/fix/double-alpha-block/docs/specs/double-alpha-block-fix.md)
- 日本語: [docs/specs/double-alpha-block-fix.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/fix/double-alpha-block/docs/specs/double-alpha-block-fix.ja.md)
- 中文: [docs/specs/double-alpha-block-fix.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/fix/double-alpha-block/docs/specs/double-alpha-block-fix.zh.md)
- Reference branch: [fix/double-alpha-block](https://github.com/mayatonton/phoenix-firestorm/tree/fix/double-alpha-block)

## 既存設定との互換性

r24 setup を壊さずに全機能が出荷されます:

- **r31 3D Stream unified tag**: 新規作成では `[3dstream:...]` を推奨。既存 `[3dstream-stereo:...]` / `[ayastream:...]` / `[ayastream-stereo:...]` は引き続き受け付けられ、既存コンテンツの書き換えは不要。`{bin}` / `{binaural}` の既定値変更は新規 `[3dstream:...]` タグでの挙動で、binaural を効かせる会場は `{bin:on}` を明示
- **r30 view-mode reshuffle**: r24 時代の AYAstorm View ユーザー (`AYAVisualRealismEnabled = 1`) は初回起動時の冪等 1-shot migration で新しい AYAstorm View (`= 2`) に静かに移行されます。ユーザー向けプロンプトはなし。r14〜r20 の視覚的リアリズム層は AYAstorm Controls floater (`Alt+C`) から新エンジン上の opt-in 追加機能として到達可能。Firestorm View 切替は配信/低リソース用途向けに引き続き利用可。モード切替には再起動が必須
- **r25 parcel music Ogg Vorbis fix**: Icecast Ogg Vorbis live stream が正常再生されます。Ogg Opus サポートは完全維持。設定不要
- **r26 MOAP 3D stream routing**: 既存の 3D Stream tag 規約 (`{ch:L}/{ch:R}/{ch:FL}/...`) が MOAP 面 URL にも適用されます。タグを使っていない配信者には影響なし
- **r27 macOS branding**: macOS 上の純表面 rename。機能変更なし、非 macOS build には影響なし
- **r28 他人 rigged picker**: GPU object-ID buffer pass を他人 avatar に拡張。マスタースイッチ `FSOtherRiggedPickerEnable` は default ON、`FSOtherRiggedPickerGPU` kill-switch も維持。デフォルトカメラ gate と 1.0 秒 arm window で負荷を抑制。r21 self picker は不変
- **Firestorm upstream FS-7.1.18.80646**: viewer 基盤の platform refresh。AYAstorm 独自機能と既存 setup には影響なし

## IR ライセンス

r11 で同梱して以来出荷を続けている venue IR は OpenAIR (CC-BY 4.0) を起源とします。出典は [`app_settings/venue_ir/CREDITS.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31+bundle-fs.80646/indra/newview/app_settings/venue_ir/CREDITS.md)。

## 描画エンジン credit

AYAstorm View pipeline は NiranV Dean 氏の Black Dragon viewer (LGPL-2.1、viewerlgpl と互換) から大量に拝借しています。credit は `floater_about.xml` に明記、port spec の header に BD repository commit ref を保持しています。

## Downloads

> **Linux / Windows ユーザーの方は [AYAstorm r31-bugfix-1](https://github.com/mayatonton/phoenix-firestorm/releases/tag/v7.2.4-ayastorm-r31-bugfix-1) をご利用ください。** r31 に存在していた FullBright prim 越し SSS pink-shadow 透けの構造バグを r31-bugfix-1 で修正済みです。macOS は r32 で fix を同梱するまで本 r31 ビルドのままです (r10.x の前例どおり、bugfix-N リリースは Linux/Windows のみ提供)。

- Linux Installer → [r31-bugfix-1 release を使用](https://github.com/mayatonton/phoenix-firestorm/releases/tag/v7.2.4-ayastorm-r31-bugfix-1)
- Windows Installer → [r31-bugfix-1 release を使用](https://github.com/mayatonton/phoenix-firestorm/releases/tag/v7.2.4-ayastorm-r31-bugfix-1)
- [macOS Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31+bundle-fs.80646/Phoenix-FirestormOS-AYAstorm-release_arm64-7-2-4-81208.dmg) — 現行 macOS ビルド (r32 で SSS leak fix を同梱予定)

## Contributors

@t-noami @mayatonton
