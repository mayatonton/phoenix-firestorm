# AYAstorm r27 — リリース告知

GitHub release ページ貼り付け用の文案。**r27 は macOS で残っていた "Firestorm" 表記を "AYAstorm" に統一するリリース** — menu bar (アプリメニュー / Hide / Quit) / 初期ウィンドウタイトル / Apple 標準 About panel を AYAstorm 表記に揃え、派生元クレジット `(based on Firestorm)` を About のバージョン文字列に残します。

実装は 2 ファイルの surgical な修正のため、独立した永続資料は立てず本ノートを first-class として扱います。

---

## AYAstorm r27 — macOS の Firestorm 表記を AYAstorm に統一

### r27 の柱: macOS だけ残っていた "Firestorm" 表記を消す

Linux / Windows では `VIEWER_CHANNEL = "AYAstorm Release"` 経由で window title が早期から AYAstorm 表記になっていましたが、macOS だけは menu bar (アプリメニュー / Hide / Quit) と Apple 標準 About panel が "Firestorm" のままでした。

CMake 側 (`MACOSX_BUNDLE_BUNDLE_NAME` / `MACOSX_EXECUTABLE_NAME` / `MACOSX_BUNDLE_INFO_STRING`) はすでに AYAstorm を入れていたにもかかわらず Firestorm 表記が消えなかったのは、**macOS が runtime 時に `English.lproj/InfoPlist.strings` の `CFBundleName` を Info.plist の値より優先する localization 仕様** のためで、.strings の 1 行が CMake 設定を全部上書きしていたのが原因です。

r27 ではこの .strings と、main menu の nib コンパイル元になっている `Firestorm.xib` の 2 ファイルだけを書き換えて、macOS の表記を AYAstorm に揃えます。

### 仕組み

```
indra/newview/English.lproj/InfoPlist.strings
  CFBundleName            : "Firestorm" → "AYAstorm"
  CFBundleShortVersionString : "Firestorm version X.X.X" → "AYAstorm version X.X.X (based on Firestorm)"
  CFBundleGetInfoString      : Firestorm → AYAstorm (派生元クレジット併記)

indra/newview/Firestorm.xib
  メイン menu の "Firestorm" タイトル → "AYAstorm"
  Apple submenu の "Firestorm" タイトル → "AYAstorm"
  "About Firestorm" → "About AYAstorm"
  "Hide Firestorm" → "Hide AYAstorm"
  "Quit Firestorm" → "Quit AYAstorm"
  初期 window title="Firestorm" → "AYAstorm"
  window frameAutosaveName="Firestorm" → "AYAstorm"
```

- 他言語 .lproj (`Japanese.lproj/` `German.lproj/` `Korean.lproj/` 等) は `language.txt` だけで `InfoPlist.strings` を持たないため、macOS は CFBundleDevelopmentRegion (= English) の本 .strings に fallback します。結果として **全 locale で menu bar が AYAstorm 表記** になります
- 派生元クレジット `(based on Firestorm)` は Apple 標準 About panel のバージョン文字列に残るため、ライセンス・派生元表示の attribution は保たれます

### Firestorm 開発陣のクレジット / 問い合わせ先は触らない

r27 で書き換えるのは **macOS の OS-level な branding 文字列** だけで、AYAstorm の About フローター本体 (Avatar メニュー → Help → About AYAstorm の中身、`indra/newview/skins/default/xui/en/floater_about.xml`) は 1 文字も touch しません。具体的には:

- Firestorm Development Team / Additional Contributors / Translators / UI Artists の名前一覧 → そのまま残ります
- 「最新情報は <https://www.firestormviewer.org> へ」という support URL → そのまま残ります
- Linden Lab credits / Licenses / Starlight skin 由来クレジット → そのまま残ります

Apple 標準の About panel (アプリメニュー → About AYAstorm) は **icon + アプリ名 + バージョン文字列 + Copyright 1 行** という OS 固定 layout で、もともと開発者名や問い合わせ先が載る場所ではありません。問い合わせ先と開発クレジットは従来どおり Firestorm About フローター側に集約されており、AYAstorm 由来のバグを Firestorm チームに、あるいはその逆に誤って投げる構造にはなりません。

### 設定

**ユーザー操作不要。** viewer 起動だけで macOS の表記が AYAstorm に揃います。Linux / Windows ユーザーは影響を受けません (元から AYAstorm 表記でした)。

### 移行ノート

- **macOS ユーザー**: r27 を起動するだけで menu bar / Hide / Quit / 標準 About / window title が AYAstorm 表記に切り替わります
- **Linux / Windows ユーザー**: 本リリースで体感的な変化はありません
- **配信者 / listener どちらの操作も不要**

### 既知の制約

- **macOS 既存ユーザーの window 位置が一度 reset される**: `frameAutosaveName="Firestorm"` → `"AYAstorm"` の変更で、NSWindow が自動保存している window 位置 / サイズの保存 key が変わります。r27 初回起動時は新 key (`AYAstorm`) に貯蔵されたデータがまだ無いため、デフォルト位置から起動します。2 回目以降は新 key で再保存されるので通常運用に戻ります
- **Linux / Windows 実機検証は不要**: 本 PR の影響範囲外
- **macOS 実機検証**: 次の Release ビルドを切るタイミングで Apple 標準 About panel / menu bar / window title が AYAstorm 表記であることを確認します

### 実装概要

- `indra/newview/English.lproj/InfoPlist.strings` — CFBundleName / CFBundleShortVersionString / CFBundleGetInfoString を AYAstorm 表記に変更、バージョン文字列に `(based on Firestorm)` 併記
- `indra/newview/Firestorm.xib` — main menu の menu item title (`Firestorm` / `About Firestorm` / `Hide Firestorm` / `Quit Firestorm`) と Apple submenu title、初期 window title、`frameAutosaveName` を AYAstorm 表記に変更
- 検証: `plutil -lint indra/newview/English.lproj/InfoPlist.strings` (.strings 構文) + `xmllint --noout indra/newview/Firestorm.xib` (xib XML 整合性) を PR で実施済み
- 触らないもの: `indra/newview/Info-Firestorm.plist` (template の中身、CMake 変数経由ですでに AYAstorm)、`indra/newview/CMakeLists.txt` の `MACOSX_BUNDLE_*` (すでに AYAstorm)、`indra/newview/skins/default/xui/en/floater_about.xml` (Firestorm 開発陣クレジット / support URL を保持)

### Credits

r27 の本実装 (macOS 表記 audit / 修正対象 2 ファイルの特定 / `(based on Firestorm)` 残置の attribution 設計 / `plutil`・`xmllint` 検証) は [t-noami](https://github.com/t-noami) さんによるものです。

AYAstorm 側では PR をそのまま取り込み、本リリースノート (3 言語) のみを追加しています。

### ドキュメント

- r25 までの parcel music Ogg Vorbis 修正: [`docs/release/ayastorm-r25-release-note.ja.md`](./ayastorm-r25-release-note.ja.md)
- r26 の MOAP 音声 → 3D Stream 接続: [`docs/release/ayastorm-r26-release-note.ja.md`](./ayastorm-r26-release-note.ja.md)
