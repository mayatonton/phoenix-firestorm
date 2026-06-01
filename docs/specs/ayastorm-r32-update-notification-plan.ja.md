# AYAstorm r32 Update Notification 実装計画

**作成日**: 2026-06-01
**対象ブランチ**: `feature/ayastorm-r32-update-notification`
**基点**: `ayastorm-release`
**目的**: 実行中の AYAstorm と GitHub Release 上の最新版が異なる場合に、ユーザーへ「最新版があります」と通知する

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

GitHub REST API は release 情報として `tag_name`, `html_url`, `assets`, `prerelease`, `draft` を返す。初期実装では `GET /repos/{owner}/{repo}/releases/latest` を使う。prerelease を通知対象にする場合は `GET /repos/{owner}/{repo}/releases` に切り替え、client 側で filter する。

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
```

設定方法:

- CI / release build では `AYASTORM_RELEASE_TAG` を明示的に渡す
- ローカル開発 build では空文字または `dev` にする
- 可能なら CMake 側で `git describe --tags --exact-match` を fallback とする

local tag が空または `dev` の場合は、通常ユーザー向け通知を出さない。開発 build で確認したい場合だけ debug setting で強制できるようにする。

### 4.2 remote version

初期実装では以下を取得する。

```text
https://api.github.com/repos/mayatonton/phoenix-firestorm/releases/latest
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

### 4.3 比較ルール

基本ルール:

```text
remote.tag_name != local.release_tag なら update available
```

ただし downgrade 通知を避けるため、以下の順に判定する。

1. remote tag と local tag が完全一致する場合は通知しない
2. local tag が空 / `dev` の場合は通常通知しない
3. 両方が AYAstorm tag として parse できる場合は、`base version` と `r number` を比較する
4. remote が local より新しい場合だけ通知する
5. parse できない場合は安全側で通知しない。ただしログには残す

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
```

`r32.1` のような小数 / patch 表記を使う場合は、`r major = 32`, `r patch = 1` として扱う。

## 5. 通知 UX

通知は modal ではなく通常 alert / toast 寄りにする。ログインや作業を止めない。

通知文案:

```text
AYAstorm の新しいバージョンがあります。

現在: [CURRENT_VERSION]
最新版: [LATEST_VERSION]

リリースページを開きますか？
```

ボタン:

- `Open Release Page`
- `Later`
- `Skip This Version`

動作:

- `Open Release Page`
  - `html_url` を外部ブラウザで開く
  - `LastNotifiedTag` を remote tag に更新する
- `Later`
  - その session では再通知しない
  - 永続的には suppress しない
- `Skip This Version`
  - `SkippedTag` に remote tag を保存し、その tag では再通知しない

日本語 locale は最初から入れる。英語文言は fallback として `en/notifications.xml` に入れる。

## 6. 設定項目

`settings.xml` に追加する候補:

| setting | default | 用途 |
|---|---:|---|
| `AYAUpdateNotifyEnabled` | `true` | update 通知機能の master switch |
| `AYAUpdateNotifyEndpoint` | GitHub latest release API | release 情報取得先 |
| `AYAUpdateNotifyIncludePrerelease` | `false` | prerelease を対象にするか |
| `AYAUpdateNotifyCheckIntervalHours` | `24` | 同一 install での再確認間隔 |
| `AYAUpdateNotifyLastChecked` | `0` | 最終確認時刻 |
| `AYAUpdateNotifyLastNotifiedTag` | empty | 最後に通知した tag |
| `AYAUpdateNotifySkippedTag` | empty | ユーザーが skip した tag |
| `AYAUpdateNotifyForceDevBuild` | `false` | dev build でも通知確認する debug setting |

`LastChecked`, `LastNotifiedTag`, `SkippedTag` は user settings として永続化する。

## 7. 実装候補

新規クラス:

```text
LLAyastormUpdateChecker
```

配置候補:

```text
indra/newview/llayaupdatechecker.h
indra/newview/llayaupdatechecker.cpp
```

責務:

- 起動後に一度だけ check を開始
- 設定に従って check 間隔を制御
- GitHub API を非同期 GET
- JSON response を LLSD として parse
- local / remote tag を比較
- 必要なら `LLNotificationsUtil::add()` で通知
- notification callback で release page open / later / skip を処理

起動タイミング:

- `LLAppViewer::setOnLoginCompletedCallback()` 相当の login completed 後
- UI / notification channel 初期化後
- network stack が使える状態

HTTP:

- `LLCoreHttpUtil::HttpCoroutineAdapter` または既存の callback helper を使う
- main thread を block しない
- timeout を短くする
- User-Agent を明示する
- API rate limit を踏んだ場合はログだけ出す

通知:

- `notifications.xml` に `AYAstormUpdateAvailable` を追加
- callback で `LLWeb::loadURLExternal(html_url)` を呼ぶ
- `Skip This Version` は user setting に remote tag を保存

## 8. 実装手順

### M1: local release tag の追加

- CMake に `AYASTORM_RELEASE_TAG` define を追加
- `LLVersionInfo` または AYAstorm helper から取得できるようにする
- About / log に出せるようにするかは別判断
- dev build の fallback を決める

受入条件:

- release build で local release tag が取得できる
- dev build では空 / `dev` として扱える

### M2: GitHub release fetch

- `LLAyastormUpdateChecker` を追加
- `GET /repos/mayatonton/phoenix-firestorm/releases/latest` を非同期で呼ぶ
- response から `tag_name` / `html_url` / `name` / `published_at` を読む
- 失敗時は通知しない

受入条件:

- 正常 response を parse できる
- network failure / 404 / rate limit で起動が止まらない

### M3: version compare

- `v7.2.4-ayastorm-rNN` 系の parser を追加
- local と remote を比較する
- remote が新しい場合だけ update available とする
- parse 不能時は通知せず log に残す

受入条件:

- `r31` -> `r32` は通知する
- `r32` -> `r31` は通知しない
- 同一 tag は通知しない
- dev build は default では通知しない

### M4: notification UX

- `AYAstormUpdateAvailable` notification を追加
- `Open Release Page`, `Later`, `Skip This Version` を実装
- 1 session 内で連続表示しない
- skip 済み tag は再表示しない

受入条件:

- 最新がある時だけ通知が出る
- release page が外部 browser で開く
- skip した tag は再通知されない
- Later は次回 check まで抑制される

### M5: settings / QA

- settings.xml に項目追加
- debug 用 endpoint override を入れる
- mock JSON / local endpoint で手動検証しやすくする

受入条件:

- endpoint をテスト用 URL に差し替えられる
- notification enabled / disabled が効く
- Windows / macOS / Linux で network failure 時に問題が出ない

## 9. セキュリティ / プライバシー

- GitHub API へ匿名 GET するだけにする
- token は使わない
- 実行中ユーザー名、ログイン情報、grid 情報は送らない
- endpoint は HTTPS のみを default にする
- redirect を許可する場合も GitHub domain から外れたら release page button では開かない
- response の body markdown は viewer 内に描画しない。表示するのは tag / release name / URL だけ

## 10. 残課題

- GitHub latest release が prerelease を含まない場合、AYAstorm の配布運用に合うか確認が必要
- release asset の platform 別 URL を通知内で直接出すかは未定。初期実装では release page だけ開く
- 既存の Velopack / updater 通知とは統合しない。将来的に自動更新へ進める場合は別設計にする
- local release tag の CI 注入方法を release workflow 側で決める必要がある

## 11. 判断

今回の実装は「最新版があることを知らせる」通知機能に限定する。自動更新、強制更新、download/install、既存 updater との統合は行わない。

最初に解くべき問題は、実行中 app が AYAstorm release tag を正確に持つこと。ここがないと GitHub Release の latest tag と比較しても、`7.2.4` 系の同一 base version 内で `r31/r32` の差分を安定して判断できない。
