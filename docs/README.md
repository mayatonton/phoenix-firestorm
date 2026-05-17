# AYAstorm docs

AYAstorm 専用のドキュメント置き場。Firestorm/LL upstream の標準資料はリポジトリ直下の `doc/` (大文字小文字違い) に温存している。

## 構成

| サブディレクトリ | 内容 | 例 |
|---|---|---|
| `release/` | 各 release の release-note と GitHub Release ページ貼り付け用テキスト (en/ja/zh) | `ayastorm-r25-release-note.ja.md`, `ayastorm-r24-github-release-page.en.md` |
| `specs/` | 仕様書・設計参照 (公開向け / 内部向け区別なし)。機能名で参照される永続版と、release 番号で紐づけた版が同居 | `spec_binaural_venue_reverb.md`, `ayastorm-r17-color-temperature.md`, `ayastorm-deferred-shader-routing.md` |
| `guides/` | ユーザー (配信者/スクリプター) 向けガイドと配布する LSL | `3dstream-tag-guide.ja.md`, `lsl/aya_3dstream_setup.lsl` |
| `build/` | AYAstorm 独自のビルド手順 (3 OS) | `building_ayastorm.md`, `building_ayastorm_macos.md` |
| `archive/r{num}/` | 過去 release の作業ログ (survey, progress, 一回限りスクリプト)。resolve 済み、参照優先度は低い | `archive/r11/fetch_venue_irs.sh`, `archive/r17/color_temperature_survey.md` |
| `testplans/` | release 前確認手順 | |
| `images/` | release notes / guides から参照される画像素材 | |

## 追加するときの判断軸

- **release で配る告知文** → `release/`
- **永続する仕様 / 設計参照** → `specs/` (内部向け参照資料も同じ場所に置く、二重メンテはしない)
- **配信者・スクリプター・ユーザーが読む手順書** → `guides/`
- **ビルドの作り方** → `build/`
- **release で使い終わった作業メモ・survey・スクリプト** → `archive/r{num}/`

## 上流との分離

`doc/` (s 無し、リポジトリ直下) は LL/Firestorm upstream の標準資料 (`LGPL-license.txt`, `LICENSE-{logos,source}.txt`, `building_{linux,macos,windows}.md`, `contributions.txt`, `translations.txt`, `firestorm_256.png`)。upstream merge との conflict を避けるため、AYAstorm 独自ファイルは置かない。
