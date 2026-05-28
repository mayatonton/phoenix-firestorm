🌐 Language: [🇺🇸 English](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-github-release-page.en.md) | [🇯🇵 日本語](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-github-release-page.ja.md) | [🇨🇳 中文](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-github-release-page.zh.md)

# AYAstorm r31-bugfix-2 — AO 削除事象救済 + LSL Bridge 衝突防御

> [!IMPORTANT]
> **r31-bugfix-2 は Firestorm 系 viewer 全体に存在する 2 件の構造的な振る舞いを AYAstorm 側で食い止めるリリースです。**
>
> AYAstorm 固有の問題ではなく、Firestorm 派生 viewer 全体で共有される inventory root に起因する構造的な振る舞いへの対応です (バグか仕様かの判断は upstream にあります)。

1. **AO 削除事象**: Firestorm 系 viewer (Firestorm 本家 / 旧版 AYAstorm / その他 FS 派生) で AO セットを「削除」すると、共有 inventory root `#Firestorm` 配下の AO データが永久消去され、別の viewer でログインしても消えたままになる cross-viewer 連鎖事象。1000 人規模で観測済。r31-bugfix-2 では通常の AO セット削除を per-account 設定の隠しフラグによる UI 非表示に置き換え、実 inventory を削除しません。Hidden 管理画面の「選択を削除」 (`Delete selected`) だけは、確認 dialog 後の明示的な完全削除として残します
2. **LSL Bridge version 衝突**: `fslslbridge.cpp` の version 不一致時自動再作成ロジックが、Firestorm 本家のマイナーバンプ時に AYAstorm Bridge を巻き添えで削除しうる構造。現状 v2.29 同一で未発火だが、片方向防御として `受信 version > 自分なら adopt` ロジックを導入し、起動時 attach でも newer bridge を `BridgeVer` 受信前に detach しないようにしました

修正は Firestorm 系 viewer 全体に存在する構造的振る舞いへの対応です。AYAstorm 側では通常の AO 削除から `#Firestorm` root への破壊的操作を外します。Firestorm 本家 / 他派生 viewer での再発はそれぞれの viewer が patch される必要があります (推奨運用と回避策は recovery guide に明記)。

## すでに AO セットが消えてしまった方へ — AO 機能の再セットアップ手順

Firestorm 系 viewer (Firestorm 本家 / 旧版 AYAstorm / 他 FS 派生) ですでに AO セットを「削除」してしまった方の **AO データそのものは viewer 側でも SL サーバ側でも取り戻せません**。ただし AO 機能自体は再セットアップで普通に使えるようになります。手順を 3 言語で公開しています:

- 🇯🇵 [日本語復旧手順](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.ja.md)
- 🇺🇸 [English Recovery Guide](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.en.md)
- 🇨🇳 [繁體中文復原指南](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.zh.md)

## Release notes

- 🇺🇸 English: [docs/release/ayastorm-r31-bugfix-2-release-note.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-release-note.en.md)
- 🇯🇵 日本語: [docs/release/ayastorm-r31-bugfix-2-release-note.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-release-note.ja.md)
- 🇨🇳 中文: [docs/release/ayastorm-r31-bugfix-2-release-note.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-release-note.zh.md)

## Key documents (tag pinned)

- 技術 spec: [docs/specs/ayastorm-r31-2-ao-bridge-recovery.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-r31-2-ao-bridge-recovery.md)
- ユーザー復旧手順 (3 言語): [docs/guides/ao-data-recovery-guide.{en,ja,zh}.md](https://github.com/mayatonton/phoenix-firestorm/tree/v7.2.4-ayastorm-r31-bugfix-2/docs/guides)

## 既存環境との互換性

r31 / r31-bugfix-1 環境を乱さずに出荷:

- **AO 削除事象 fix**: 自動適用。設定変更不要。r31 / r31-bugfix-1 install 済の方は r31-bugfix-2 を上書き install するだけで、通常の AO セット削除は非破壊の Hide になります
- **LSL Bridge 衝突防御**: 自動適用。Firestorm 本家のマイナーバンプ時に AYAstorm Bridge が削除される事象を防止 (現状未発火、将来防御)
- **r31 / r31-bugfix-1 の全機能** (3D Stream unified tag / AYAstorm View / parcel music Vorbis fix / MOAP audio routing / macOS branding / GPU other-rigged picker / chat tab split / venue reverb / SSS pink-shadow 修正等): そのまま保持されます
- **AO を編集 / 削除しない方**: 見た目の変化はほぼありません。AO set の Trash icon は非表示 icon になり、Dialog 文言が Hide 前提に変わります
- **AO を整理したい方**: Hidden 管理画面の「選択を削除」 (`Delete selected`) から、確認後に選択済み hidden set の実 inventory folder を完全削除できます。これは元に戻せません
- **すでに AO セットが消えてしまった方**: r31-bugfix-2 を入れていただくと **以降は同じ事象は起きません**。すでに消えた AO データ自体は戻せませんが、上の再セットアップ手順で AO 機能はすぐ使える状態に戻せます

## IR licence

r11 で同梱し以降も出荷中の venue IR は OpenAIR (CC-BY 4.0) 由来です。出典: [`app_settings/venue_ir/CREDITS.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/indra/newview/app_settings/venue_ir/CREDITS.md)

## Downloads

- [Windows Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-2/Phoenix-FirestormOS-AYAstorm-release_AVX2-7-2-4-261481144_Setup.exe)
- [macOS Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-2/Phoenix-FirestormOS-AYAstorm-release_arm64-7-2-4-81209.dmg)
- [Linux Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-2/Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261481144.tar.xz)

## Contributors

@t-noami @mayatonton
