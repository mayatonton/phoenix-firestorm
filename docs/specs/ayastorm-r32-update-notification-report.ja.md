# AYAstorm r32 Update Notification 実装報告書

**作成日**: 2026-06-01
**対象ブランチ**: `feature/ayastorm-r32-update-notification`
**基点**: `ayastorm-release`
**目的**: 実行中の AYAstorm と GitHub Release 上の最新版が異なる場合に、ユーザーへ「最新版があります」と通知する

## 0. 今回の実装を有効にするために release 時に必要な操作

この update notification は、release build に AYAstorm release tag が埋め込まれていることを前提に動作する。release 担当者は、配布前に以下を実施する。

1. GitHub Release と対応する tag 名を決める

   例:

   ```text
   v7.2.4-ayastorm-r32
   v7.2.4-ayastorm-r32-bugfix-2
   ```

2. その tag が付いた commit から configure する

   ```bash
   git checkout REPLACE_WITH_RELEASE_TAG
   ```

   この場合、CMake が `AYASTORM_RELEASE_TAG` を自動検出する。

3. tag checkout ではない CI / source archive から build する場合は、configure 時に release tag を明示する

   ```bash
   export AYA_RELEASE_TAG="REPLACE_WITH_RELEASE_TAG"
   autobuild configure ... -- ... -DAYASTORM_RELEASE_TAG="$AYA_RELEASE_TAG"
   ```

   `autobuild.xml` に release tag を固定値として直書きする運用にはしない。`autobuild.xml` は build configuration の共通定義であり、release ごとに変わる値を入れると次 release で更新漏れを起こしやすい。tag は `autobuild configure` の `--` 後に CMake 引数として渡すか、CI の `EXTRA_ARGS` / build script の変数から渡す。

4. build tree を再利用する場合は、必要なら configure を再実行する

   既存の CMake cache が `dev` のままだと、コンパイル / リンク / パッケージングだけを再実行しても release tag は更新されない。これは macOS / Windows / Linux いずれの build でも同じ。

5. release 前に build へ埋め込まれた値を確認する

   ```bash
   rg "AYASTORM_RELEASE_TAG|AYASTORM_SOURCE_BRANCH" build-darwin-universal/newview/fsversionvalues.h
   ```

   期待例:

   ```cpp
   const std::string AYASTORM_RELEASE_TAG{"REPLACE_WITH_RELEASE_TAG"};
   ```

   `AYASTORM_RELEASE_TAG{"dev"}` のままなら、その build は release tag 入り build ではない。

`-bugfix-N` 形式の tag も今回の parser / 比較 / 表示対象に含めている。例えば `v7.2.4-ayastorm-r32-bugfix-2` は `AYAstorm r32.2` として扱い、`bugfix-1` より新しい release と判定する。

release tag の埋め込みを行わない場合、About の AYAstorm 表示、Release Notes link、update 判定が release build として正しく機能しない。

詳細は [11. repo 管理者 / release 担当者へのお願い](#11-repo-管理者--release-担当者へのお願い) にも記録している。

### 0.1 付属事項: macOS build project 名

これは update notification 実装固有の要件ではなく、macOS build 手順上の注意事項である。

現在の `autobuild.xml` / CMake 設定では `ROOT_PROJECT_NAME=SecondLife` が使われるため、macOS configure 後に生成される Xcode project は `SecondLife.xcodeproj` になる。古い build tree に `Firestorm.xcodeproj` が残っていても、そちらは使わない。

```bash
xcodebuild -project SecondLife.xcodeproj ...
```

`Firestorm.xcodeproj` を叩くと、古い project に新規 source / target が反映されておらず、`LLAyastormUpdateChecker` などの link error を起こす可能性がある。

これは macOS の Xcode project 名に関する注意であり、Windows / Linux の build 手順を同じ理由で変更するものではない。

## 目次

- [0. 今回の実装を有効にするために release 時に必要な操作](#0-今回の実装を有効にするために-release-時に必要な操作)
  - [0.1 付属事項: macOS build project 名](#01-付属事項-macos-build-project-名)
- [1. 目的](#1-目的)
- [2. 前提](#2-前提)
- [3. 現状確認](#3-現状確認)
- [4. 比較方針](#4-比較方針)
- [5. 通知 UX](#5-通知-ux)
- [6. 設定項目](#6-設定項目)
- [7. 実装構成](#7-実装構成)
- [8. 実装フェーズ](#8-実装フェーズ)
- [9. セキュリティ / プライバシー](#9-セキュリティ--プライバシー)
- [10. 残課題](#10-残課題)
- [11. repo 管理者 / release 担当者へのお願い](#11-repo-管理者--release-担当者へのお願い)
- [12. 実装結果と検証観点](#12-実装結果と検証観点)
- [13. 判断](#13-判断)
- [14. PR 下書き](#14-pr-下書き)

## 1. 目的

AYAstorm 起動後、GitHub Releases に公開されている最新版 release tag を確認し、実行中 app の release tag と異なる場合に通知を出す。

この機能は自動更新ではない。初期実装では、最新版の存在を知らせ、リリースページを開けるようにするだけにする。

## 2. 前提

- 対象 repository は `mayatonton/phoenix-firestorm`
- 通知対象は GitHub Release が付いた tag のみ
- 単なる git tag は対象にしない
- draft release は対象にしない
- 初期実装では stable release を対象にし、prerelease を含めるかは設定で後から切り替え可能にする
- ネットワーク失敗時は黙って失敗し、ログだけ残す
- ログインや viewer 起動をブロックしない
- ダウンロードやインストールは行わない
- 通知の第一候補は、ログイン画面上の viewer native UI banner とする
- Firestorm のログイン HTML 側は AYAstorm から制御できないため、実装範囲に含めない

GitHub REST API は release 情報として `tag_name`, `html_url`, `assets`, `prerelease`, `draft` を返す。初期実装では `GET /repos/{owner}/{repo}/releases?per_page=30` を使い、client 側で AYAstorm classic 用 tag だけを filter する。

`GET /repos/{owner}/{repo}/releases/latest` は repository 全体の最新 release を返すため、将来同一 repository に AYAstorm Vulkan / Metal 系 release が混在した場合に classic viewer が別系統 release を誤検出する可能性がある。そのため使用しない。

参考: https://docs.github.com/en/rest/releases/releases

## 3. 現状確認

既存の viewer には Linden / Firestorm 由来の updater 経路がある。

- `llvelopack.cpp`
  - update download / install 用
  - 今回は使わない
- `notifications.xml`
  - `RequiredUpdate`, `PromptOptionalUpdate`, `OptionalUpdateReady` などが存在
  - これらは既存 updater 用で、今回の「通知だけ」には強すぎる
- `LLVersionInfo`
  - `getShortVersion()` は `7.2.4`
  - `getVersion()` は `7.2.4.<build>`
  - `getGitHash()` は build 時の git hash
  - AYAstorm の release tag である `v7.2.4-ayastorm-r31...` / `v7.2.4-ayastorm-r32...` を直接返す API は現状ない

したがって、正確な比較には「この app がどの AYAstorm release tag 由来か」を build 時に埋め込む必要がある。

### 3.1 ログイン画面の構造

Firestorm のログイン画面には `login_html` という埋め込みブラウザがある。ただし、この HTML は repository 内の静的ファイルではなく、grid 設定または `LoginPage` setting で指定された外部 URL を読み込む。

確認箇所:

- `indra/newview/skins/default/xui/en/panel_fs_nui_login.xml`
  - `web_browser name="login_html"`
- `indra/newview/fspanellogin.cpp`
  - `FSPanelLogin::loadLoginPage()`
  - `LLGridManager::getInstance()->getLoginPage()` の URL に query parameter を付けて `navigateTo()` する
- `indra/newview/fsgridhandler.cpp`
  - `LLGridManager::getLoginPage()`
  - `LoginPage` setting があればそれを優先し、なければ grid の login page を返す
- default login page
  - `https://phoenixviewer.com/app/loginV3/`

このため、ログイン HTML 側の変更は今回の実装範囲から除外する。

- Firestorm 公式の login HTML はこの repository 内で完結して変更できない
- AYAstorm 側で外部 login page を制御できない
- 現在 `loadLoginPage()` から渡している `version` / `channel` だけでは、`v7.2.4-ayastorm-r32` のような AYAstorm release tag を正確に比較できない
- update check の結果、skip 状態、session 内 suppress 状態は viewer 側で管理すべき状態である

したがって、更新判定と表示は viewer native 側で実装する。ログイン HTML 差し替え案は採用しない。

### 3.2 AYAstorm Controls からの手動チェック

ログイン後にもユーザーが明示的に最新版確認できるように、AYAstorm Controls の General tab に `Update Check` ボタンを追加する。

手動チェックは起動時の自動チェックとは扱いを分ける。

- check interval による抑制を受けない
- session 内 `Later` suppress による抑制を受けない
- `Skip Version` 済み tag でも、ユーザーが明示的に押した場合は確認対象にする
- `AYAUpdateNotifyEnabled=false` でも、手動ボタンは確認を実行できる
- check 中の多重実行は避け、すでに実行中なら通知だけ返す

結果表示:

- update あり: ログイン画面で出している更新パネルと同じ情報構成 (`View Update`, `Later`, `Skip Version`) の native floater を表示する
- update なし: 同じ AYAstorm 専用 native floater で「最新版です」を表示し、`OK` だけを出す
- network / parse 失敗: 同じ AYAstorm 専用 native floater で「確認できなかった」を表示し、詳細は `AYAUpdate` ログに残す

## 4. 比較方針

### 4.1 local version

新しく `LLVersionInfo` か AYAstorm 専用 helper に、以下を追加する。

```text
AYASTORM_RELEASE_TAG
```

期待値:

```text
v7.2.4-ayastorm-r32
v7.2.4-ayastorm-r32.1
v7.2.4-ayastorm-r32+bundle-fs.80646
v7.2.4-ayastorm-r31-bugfix-2
v7.2.4-ayastorm-r10.x-bugfix-2
```

local build には release tag とは別に release family も埋め込む。

```text
AYASTORM_RELEASE_FAMILY=classic
```

Vulkan / Metal 版など別系統の viewer は、同じ repository で release する場合でも別 family として扱う。

```text
AYASTORM_RELEASE_FAMILY=vulkan
AYASTORM_RELEASE_FAMILY=metal
```

設定方法:

- release tag が付いた commit から configure した場合は、CMake が `git describe --tags --exact-match --match "v*-ayastorm-r*"` で `AYASTORM_RELEASE_TAG` を自動検出する
- CI / release build では必要に応じて `-DAYASTORM_RELEASE_TAG=...` で明示的に上書きできる
- ローカル開発 build では空文字または `dev` にする

local tag が空または `dev` の場合は、通常ユーザー向け通知を出さない。開発 build で確認したい場合だけ debug setting で強制できるようにする。

### 4.2 remote version

初期実装では以下を取得する。

```text
https://api.github.com/repos/mayatonton/phoenix-firestorm/releases?per_page=30
```

取得する field:

| field | 用途 |
|---|---|
| `tag_name` | 比較対象 |
| `name` | 通知表示用 |
| `html_url` | ボタンで開く URL |
| `published_at` | ログ / 表示補助 |
| `prerelease` | prerelease filter |
| `draft` | 念のため除外 |

GitHub API 失敗時は通知しない。再試行は次回起動または一定時間後に任せる。

### 4.3 tag 命名ルール

AYAstorm classic の通知対象 tag は、以下の形式に限定する。

```text
v<base>-ayastorm-r<r>[.<patch>][+<build-metadata>]
```

例:

```text
v7.2.4-ayastorm-r32
v7.2.4-ayastorm-r32.1
v7.2.4-ayastorm-r32-bugfix-2
v7.2.4-ayastorm-r31-bugfix-2
v7.2.4-ayastorm-r32+bundle-fs.80646
```

正規表現目安:

```text
^v([0-9]+)\.([0-9]+)\.([0-9]+)-ayastorm-r([0-9]+)(?:(?:\.([0-9]+))|(?:\.(x)))?(?:-bugfix-([0-9]+))?(?:\+([A-Za-z0-9._-]+))?$
```

除外する tag:

```text
v7.2.4-ayastorm-vk-r1
v7.2.4-ayastorm-vulkan-r1
v7.2.4-ayastorm-r32-vk
v7.2.4-ayastorm-r32+vk
v7.2.4-ayastorm-metal-r1
v7.2.4-ayastorm-mtl-r1
v7.2.4-ayastorm-r32-metal
v7.2.4-ayastorm-r32+metal
```

方針:

- classic viewer は `AYASTORM_RELEASE_FAMILY=classic` とし、classic tag だけを通知対象にする
- tag 内に `vk` / `vulkan` / `metal` / `mtl` 系の識別子がある release は classic viewer では対象外にする
- Vulkan viewer を作る場合は、`v<base>-ayastorm-vk-r<r>` のように classic と衝突しない tag family を定義する
- Metal viewer を作る場合は、`v<base>-ayastorm-metal-r<r>` のように classic / Vulkan と衝突しない tag family を定義する
- branch 名ではなく tag 名を authoritative とする
- `target_commitish` は補助ログには使えるが、通知対象判定には使わない

このルールにより、同一 repository に classic / Vulkan / Metal の release が混在しても、classic viewer が別系統 release を最新版として表示しない。

### 4.4 比較ルール

基本ルール:

```text
remote.release_family == local.release_family かつ remote.tag_name が local.release_tag より新しいなら update available
```

ただし downgrade 通知を避けるため、以下の順に判定する。

1. remote tag と local tag が完全一致する場合は通知しない
2. local tag が空 / `dev` の場合は通常通知しない
3. local tag から release family を決める。初期実装では `classic`
4. releases list から `draft == false` の release だけを見る
5. `AYAUpdateNotifyIncludePrerelease == false` の場合は `prerelease == true` を除外する
6. remote tag が local family と一致しない場合は除外する
7. 両方が AYAstorm tag として parse できる場合は、`base version`, `r number`, `r patch` を比較する
8. remote が local より新しい場合だけ通知する
9. parse できない場合は安全側で通知しない。ただしログには残す

tag parse 例:

```text
v7.2.4-ayastorm-r32
  base = 7.2.4
  ayastorm_r = 32
  suffix = none

v7.2.4-ayastorm-r31+bundle-fs.80646
  base = 7.2.4
  ayastorm_r = 31
  suffix = bundle-fs.80646

v7.2.4-ayastorm-r31-bugfix-2
  base = 7.2.4
  ayastorm_r = 31
  bugfix = 2
```

`r32.1` のような小数 / patch 表記を使う場合は、`r major = 32`, `r patch = 1` として扱う。`r32-bugfix-2` のような `-bugfix-N` 表記も今回の実装で対応しており、表示上は `AYAstorm r32.2` として扱う。既存 release に存在する `r10.x` は plain `r10` より後の patched r10 系として扱い、`r10.x-bugfix-2` は `r10.x-bugfix-1` より新しいものとして扱う。

remote release は releases list の先頭をそのまま採用せず、filter 後に parsed version で最大のものを選ぶ。GitHub の release 作成日時や latest 設定ではなく、AYAstorm tag rule 上の version を優先する。

## 5. 通知 UX

通知の第一候補は、ログイン画面上に重ねる viewer native UI banner とする。`login_html` の中身を書き換えるのではなく、`panel_fs_nui_login.xml` / `FSPanelLogin` 側に native control を追加する。

方針:

- ログイン画面表示中に update check が完了した場合は、ログイン画面内の banner に表示する
- banner は modal にしない
- ログイン入力欄、grid 選択、connect 操作を塞がない
- network 失敗、parse 失敗、local tag 不明時は何も表示しない
- ユーザーがすぐログインして画面が閉じた場合は、その session では通知しない

表示位置候補:

- `login_html` の上に重ねる native panel
- ログイン画面中央に、幅を絞った compact card として表示する
- 入力欄や login button がある `ui_stack` は塞がない
- タイトルは大きめの font にし、本文も小さくしすぎない
- 横長の帯ではなく、本文とボタンが中央にまとまる 150-170 px 程度の card とする

通知文案:

```text
AYAstorm の新しいバージョンがあります。

現在: [CURRENT_VERSION]
最新版: [LATEST_VERSION]

リリースページを開きますか？
```

ボタン:

- `View Update`
- `Later`
- `Skip This Version`

動作:

- `View Update`
  - `html_url` を外部ブラウザで開く
  - `LastNotifiedTag` を remote tag に更新する
- `Later`
  - その session では再通知しない
  - 永続的には suppress しない
- `Skip This Version`
  - `SkippedTag` に remote tag を保存し、その tag では再通知しない

日本語 locale は最初から入れる。英語文言は fallback 文字列として `en/panel_fs_nui_login.xml` に入れる。

## 6. 設定項目

`settings.xml` に追加する候補:

| setting | default | 用途 |
|---|---:|---|
| `AYAUpdateNotifyEnabled` | `true` | update 通知機能の master switch |
| `AYAUpdateNotifyEndpoint` | GitHub releases list API | release 情報取得先 |
| `AYAUpdateNotifyIncludePrerelease` | `false` | prerelease を対象にするか |
| `AYAUpdateNotifyCheckIntervalHours` | `0` | 同一 install での再確認間隔。今回の検証差分では毎回確認する |
| `AYAUpdateNotifyLastChecked` | `0` | 最終確認時刻 |
| `AYAUpdateNotifyLastNotifiedTag` | empty | 最後に通知した tag |
| `AYAUpdateNotifySkippedTag` | empty | ユーザーが skip した tag |
| `AYAUpdateNotifyForceDevBuild` | `false` | dev build でも通知確認する debug setting。通常出荷では無効 |
| `AYAUpdateNotifyLocalTagOverride` | empty | local release tag の検証用 override。空文字なら build 埋め込み tag を使う |

`LastChecked`, `LastNotifiedTag`, `SkippedTag` は user settings として永続化する。

検証用 build でログイン画面に実際に表示したい場合は、user settings 側で `AYAUpdateNotifyForceDevBuild=true` と `AYAUpdateNotifyLocalTagOverride=v0.0.0-ayastorm-r0` などを一時指定する。これは banner を無条件に出すためのダミー表示ではなく、実行時には `AYAUpdateNotifyEndpoint` の GitHub Releases API へ実際に GET し、取得した release list を tag rule で filter し、local tag と比較した結果として表示する。

release へ入れる前には、`AYAUpdateNotifyCheckIntervalHours` を運用値に戻す。候補は `24`。

## 7. 実装構成

新規クラス:

```text
LLAyastormUpdateChecker
```

配置:

```text
indra/newview/llayaupdatechecker.h
indra/newview/llayaupdatechecker.cpp
```

責務:

- 起動後に一度だけ check を開始
- 設定に従って check 間隔を制御
- GitHub API を非同期 GET
- JSON response を JSON array として parse
- releases list から local release family と一致する tag だけを filter
- local / remote tag を比較
- 必要ならログイン画面 banner 用の結果 state を publish
- banner button callback で release page open / later / skip を処理

起動タイミング:

- login panel 初期化後、`FSPanelLogin` が表示されるタイミング
- UI 初期化後
- network stack が使える状態
- 起動直後の login 操作を block しない

HTTP:

- `LLCoreHttpUtil::HttpCoroutineAdapter` または既存の callback helper を使う
- main thread を block しない
- timeout を短くする
- User-Agent を明示する
- API rate limit を踏んだ場合はログだけ出す

通知:

- `panel_fs_nui_login.xml` に update banner 用 native panel を追加
- `FSPanelLogin` に banner 表示 / 非表示 / button callback を追加
- callback で `LLWeb::loadURLExternal(html_url)` を呼ぶ
- `Skip This Version` は user setting に remote tag を保存

## 8. 実装フェーズ

### M1: local release tag の追加

- CMake に `AYASTORM_RELEASE_TAG` define を追加
- CMake に `AYASTORM_RELEASE_FAMILY` define を追加
- CMake に dev build 用の `AYASTORM_SOURCE_BRANCH` を追加
- `LLVersionInfo` または AYAstorm helper から取得できるようにする
- About の Release Notes は AYAstorm release tag に対応する GitHub release page に向ける
- dev build の Release Notes は build 時の source branch page に向ける

受入条件:

- release build で local release tag が取得できる
- release build で local release family が取得できる
- dev build では `dev` tag と source branch を取得できる

### M2: GitHub release fetch

- `LLAyastormUpdateChecker` を追加
- `GET /repos/mayatonton/phoenix-firestorm/releases?per_page=30` を非同期で呼ぶ
- response array から `tag_name` / `html_url` / `name` / `published_at` / `draft` / `prerelease` を読む
- 失敗時は通知しない

受入条件:

- 正常 response array を parse できる
- network failure / 404 / rate limit で起動が止まらない

### M3: version compare

- `v7.2.4-ayastorm-rNN` 系の parser を追加
- classic / Vulkan / Metal などの release family 判定を追加
- classic viewer では `vk` / `vulkan` / `metal` / `mtl` 系 tag を除外する
- local と remote を比較する
- remote が新しい場合だけ update available とする
- parse 不能時は通知せず log に残す

受入条件:

- `r31` -> `r32` は通知する
- `r32` -> `r31` は通知しない
- 同一 tag は通知しない
- `v7.2.4-ayastorm-vk-r1` は classic viewer では通知しない
- `v7.2.4-ayastorm-metal-r1` は classic viewer では通知しない
- dev build は default では通知しない

### M4: login banner UX

- login panel に update banner を追加
- `View Update`, `Later`, `Skip This Version` を実装
- 1 session 内で連続表示しない
- skip 済み tag は再表示しない

受入条件:

- 最新がある時だけ通知が出る
- login 画面表示中は native banner として出る
- release page が外部 browser で開く
- skip した tag は再通知されない
- Later は次回 check まで抑制される
- login を妨げない

### M5: settings / QA

- settings.xml に項目追加
- debug 用 endpoint override を入れる
- mock JSON / local endpoint で手動検証しやすくする

受入条件:

- endpoint をテスト用 URL に差し替えられる
- notification enabled / disabled が効く
- Windows / macOS / Linux で network failure 時に問題が出ない

### M6: AYAstorm Controls 手動チェック

- AYAstorm Controls の General tab に `Update Check` ボタンを追加する
- button callback は update checker の manual check 経路を呼ぶ
- manual check は interval / session suppress / skipped tag による抑制を受けない
- update ありの場合は、ログイン画面 banner と同じ内容の update floater を開く
- update なしの場合は同じ update floater で `OK` だけを出す
- check 失敗の場合は同じ update floater で `OK` だけを出し、詳細は `AYAUpdate` ログで確認する

受入条件:

- AYAstorm Controls から押すと GitHub Releases API へ実際に確認に行く
- 最新がない場合にユーザーへ「最新版」と分かる AYAstorm 専用パネルが出る
- 最新がある場合に release page を開ける UI が出る
- `Later` はその場の floater を閉じるだけで、次回の手動確認を妨げない
- `Skip Version` は起動時通知と同じ skipped tag に保存する
- ログイン画面の自動通知と同じ tag filter / version compare を使う

## 9. セキュリティ / プライバシー

- GitHub API へ匿名 GET するだけにする
- token は使わない
- 実行中ユーザー名、ログイン情報、grid 情報は送らない
- endpoint は HTTPS のみを default にする
- redirect を許可する場合も GitHub domain から外れたら release page button では開かない
- response の body markdown は viewer 内に描画しない。表示するのは tag / release name / URL だけ

## 10. 残課題

- prerelease を通知対象に含める運用にするかは確認が必要
- release asset の platform 別 URL を通知内で直接出すかは未定。初期実装では release page だけ開く
- 既存の Velopack / updater 通知とは統合しない。将来的に自動更新へ進める場合は別設計にする
- Firestorm の login HTML は制御対象外のため、今回の実装範囲には含めない

## 11. repo 管理者 / release 担当者へのお願い

repo 管理者 / release 担当者は、release build に AYAstorm release tag が正しく埋め込まれていることを確認する必要がある。

この update notification は、実行中 app の `AYASTORM_RELEASE_TAG` と GitHub Releases の tag を比較して動く。ここが `dev` のまま出荷されると、About の AYAstorm 表示、Release Notes link、update 判定が release build として正しく機能しない。

tag 付き commit から configure する場合:

```bash
git checkout REPLACE_WITH_RELEASE_TAG
```

この状態で configure すれば、CMake が `git describe --tags --exact-match --match "v*-ayastorm-r*"` で tag を自動検出する。

CI や source archive など、tag checkout ではない状態から release build する場合:

```bash
export AYA_RELEASE_TAG="REPLACE_WITH_RELEASE_TAG"
autobuild configure ... -- ... -DAYASTORM_RELEASE_TAG="$AYA_RELEASE_TAG"
```

を明示的に渡す。

注意点:

- `autobuild.xml` へ release tag を固定値として直書きしない。release ごとに変わる値は、`autobuild configure` の `--` 後に渡す CMake 引数、CI の `EXTRA_ARGS`、または build script の変数で渡す
- 既存の CMake cache が `dev` のままだと、コンパイル / リンク / パッケージングだけを再実行しても release tag は更新されない
- tag を打ったあと、または `-DAYASTORM_RELEASE_TAG=...` を変えたあとには configure を再実行する
- classic / Vulkan / Metal を同一 repository で扱う場合は、tag 命名ルールを守り、classic build に `vk` / `vulkan` / `metal` 系 tag を混ぜない

release build の確認:

```bash
rg "AYASTORM_RELEASE_TAG|AYASTORM_SOURCE_BRANCH" build-darwin-universal/newview/fsversionvalues.h
```

期待例:

```cpp
const std::string AYASTORM_RELEASE_TAG{"REPLACE_WITH_RELEASE_TAG"};
```

`AYASTORM_RELEASE_TAG{"dev"}` のままなら、その build は release tag 入り build ではない。

`-bugfix-N` 形式の tag も今回の parser / 比較 / 表示対象に含めている。例えば `v7.2.4-ayastorm-r32-bugfix-2` は `AYAstorm r32.2` として扱い、`bugfix-1` より新しい release と判定する。

## 12. 実装結果と検証観点

今回の差分では、以下を実装した。

- `LLVersionInfo` から `AYASTORM_RELEASE_TAG` / `AYASTORM_RELEASE_FAMILY` を取得できるようにした
- `AYASTORM_RELEASE_TAG` は release tag が付いた commit では CMake が自動検出し、CI や特殊 build では `-DAYASTORM_RELEASE_TAG=...` で上書きできるようにした
- ログイン画面の AYAstorm ロゴ下表示と About header の AYAstorm 表示も、同じ `AYASTORM_RELEASE_TAG` から導出するようにした。例: `v7.2.4-ayastorm-r31-bugfix-2` -> `AYAstorm r31.2`
- Firestorm hosted login page に渡す `version` / `channel` query も AYAstorm 表示に差し替えた。右上の `Your version` card は upstream Firestorm build number ではなく `AYAstorm rXX` / `based on FS X.Y.Z` を表示する。
- About の `Release Notes` link は Firestorm wiki ではなく AYAstorm の GitHub URL に向ける。release tag 付き build では該当 tag の release page、dev build では build 時の source branch page を開く。
- `LLAyastormUpdateChecker` を追加し、`https://api.github.com/repos/mayatonton/phoenix-firestorm/releases?per_page=30` を実際に取得するようにした
- GitHub Releases の response から `draft` / `prerelease` / `tag_name` / `name` / `html_url` を読み、classic AYAstorm tag だけを比較対象にした
- ログイン画面に native banner を重ね、`View Update`, `Later`, `Skip Version` を実装した
- AYAstorm Controls からの `Update Check` 手動確認を追加する。手動確認は update がない場合も通知し、update がある場合はログイン画面 banner と同じ内容の native floater を表示する

検証時は `AYAUpdate` ログタグを見る。

期待される流れ:

```text
Checking GitHub releases: endpoint=... embedded_tag=... effective_local_tag=... family=classic
Latest matching AYAstorm release: v...
AYAstorm update available: local=... remote=...
```

このログが出て banner が表示される場合、少なくとも「ログイン画面の見た目だけ」ではなく、ネットワーク取得、release list parse、tag family filter、version compare、native UI 表示まで通っている。

### 12.1 ローカル test tag 検証

PR 前の確認として、remote へ push しないローカル tag を HEAD に付け、release tag 埋め込みと update 判定用 local tag の反映を検証した。

使用した test tag:

```text
v7.2.4-ayastorm-r0+test.update-notification
```

この tag は `v<base>-ayastorm-r<r>[+metadata]` 形式に合致し、classic AYAstorm tag として parser を通る。`r0` として扱われるため、GitHub Releases 側に既存の `r31` / `r32` 系 release が存在する環境では、「remote の方が新しい」経路を検証できる。

確認結果:

```text
build-darwin-universal/newview/fsversionvalues.h:
const std::string AYASTORM_RELEASE_TAG{"v7.2.4-ayastorm-r0+test.update-notification"};
```

生成済み app binary にも同じ tag 文字列が含まれることを確認した。

```text
strings AYAstorm | rg 'v7\.2\.4-ayastorm-r0\+test\.update-notification'
v7.2.4-ayastorm-r0+test.update-notification
```

この状態で macOS arm64 app の差分 build を実行し、`BUILD SUCCEEDED` を確認済み。

## 13. 判断

今回の実装は「最新版があることを知らせる」通知機能に限定する。自動更新、強制更新、download/install、既存 updater との統合は行わない。

最初に解くべき問題は、実行中 app が AYAstorm release tag と release family を正確に持つこと。ここがないと GitHub Releases の一覧から classic / Vulkan / Metal などの別系統 release を安全に分離できず、`7.2.4` 系の同一 base version 内で `r31/r32` の差分も安定して判断できない。

表示方式は、Firestorm の login HTML を変更する方式ではなく、viewer native UI の login banner とする。理由は、Firestorm の外部 login HTML を AYAstorm 側で制御できず、更新判定に必要な local release tag と抑制状態も viewer 側の状態だからである。

## 14. PR 下書き

```md
## 概要

AYAstorm r32 向けに、GitHub Releases 上の最新版を検出して通知する update notification を追加します。

主な変更点:

- AYAstorm release tag を build に埋め込む仕組みを追加
- GitHub Releases から AYAstorm classic 系 release を取得して比較
- ログイン画面に update banner を表示
- AYAstorm Controls に手動 Update Check を追加
- update なし / 取得失敗 / update あり の表示を追加
- About / Release Notes link / ログイン画面の AYAstorm version 表示を release tag 由来に整理
- release tag の運用手順をドキュメント化

## 確認

- Mac 実機で表示・動作確認済み
- Mac arm64 app build 済み
- ローカル test tag `v7.2.4-ayastorm-r0+test.update-notification` で `AYASTORM_RELEASE_TAG` の埋め込みと binary 反映を確認済み
- Windows / Linux は実機確認をお願いします

## 補足

release build では `AYASTORM_RELEASE_TAG` が `dev` のままにならないよう、tag checkout または configure 時の `-DAYASTORM_RELEASE_TAG=...` 指定が必要です。
```
