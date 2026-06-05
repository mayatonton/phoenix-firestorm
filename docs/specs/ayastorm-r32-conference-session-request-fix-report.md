# AYAstorm r32 conference session request fix report

作成日: 2026-06-05
対象ブランチ: `fix/ayastorm-r32-conference-session-request`
起点: `origin/ayastorm-release` (`214054a5a7`)
対象ファイル: `indra/newview/llimview.cpp`

## 目次

- [要約](#要約)
- [発生していた事象](#発生していた事象)
- [問題の切り分け](#問題の切り分け)
  - [conference start request の失敗](#1-conference-start-request-の失敗)
  - [history fetch の `You cannot join chat`](#2-history-fetch-の-you-cannot-join-chat)
- [Firestorm 本家との比較](#firestorm-本家との比較)
- [実装内容](#実装内容)
- [再検証時に見るログ](#再検証時に見るログ)
  - [conference start request](#conference-start-request)
  - [history fetch](#history-fetch)
- [変更しない判断](#変更しない判断)
- [Mac 実機確認](#mac-実機確認)
- [Chat style との関係](#chat-style-との関係)
- [結論](#結論)

## 要約

複数人を招待する conference IM で、`ChatSessionRequest` capability への conference start request が失敗した場合の扱いを改善した。

今回の修正内容:

- `ChatSessionRequest` URL が空の場合は deprecated conference start path へ fallback する。
- `HTTP_BAD_REQUEST` の場合は Firestorm 本家同等に deprecated conference start path へ fallback する。
- その他の conference start 失敗では、HTTP status を warning log に出す。
- fallback できない失敗では、ユーザーに generic request error を表示する。

通常ログは最小限に抑え、cap URL、送信 payload、招待先一覧は出さない。

今回あえて変更しない内容:

- timeout (`Easy_28`) での自動 fallback。
- `You cannot join chat` を返す history fetch の初回スキップ。
- server / simulator / capability contract の変更。

timeout fallback は二重 invite / 二重 conference のリスクがあるため入れていない。history fetch の `You cannot join chat` は Firestorm 本家にもある既存経路のため、今回の修正範囲から外した。

## 発生していた事象

AYAstorm release 実行中に、フレンド複数人を招待した conference IM が正常に作成されないように見えるケースがあった。

初期調査時点の `AYAstorm.log` では、主に次のログが出ていた。

- `LLIMModel startConferenceCoro : Failed to start conference`
- `CoreHTTP cannot POST ... ChatSessionRequest cap ... Timeout was reached`
- `ChatHistory ... 403 ... You cannot join chat`

`SLVoice.log` は 0 bytes だったため、直接原因は voice daemon ではなく IM / `ChatSessionRequest` 経路と判断した。

## 問題の切り分け

ログ上は複数の warning が近いタイミングで出るが、実際には次の 2 系統に分かれる。

### 1. conference start request の失敗

conference 作成時、viewer は `ChatSessionRequest` cap に次の request を送る。

- `method = "start conference"`

ここで timeout (`Easy_28`) などが起きると、conference start request が失敗する。

今回の修正対象はこの系統である。

### 2. history fetch の `You cannot join chat`

conference 作成後、履歴取得が有効な場合、viewer は同じ `ChatSessionRequest` cap に次の request を送る。

- `method = "fetch history"`

この履歴取得が server 側で拒否されると、`403 You cannot join chat` がログに出る。

この warning は「ライブ conference IM に参加できない」という意味ではなく、作成済みセッションに対する history fetch が拒否されたことを示す。実機確認では、history fetch が 403 を返していても conference IM の送受信自体は機能しているように見えた。

## Firestorm 本家との比較

`upstream/master` (`5559686546`) の `indra/newview/llimview.cpp` を確認した。

conference start について、本家 Firestorm も次の動作だった。

- region がある場合は `ChatSessionRequest` cap に `method = "start conference"` を POST する。
- `HTTP_BAD_REQUEST` の場合だけ deprecated conference start path へ fallback する。
- timeout (`Easy_28`) では fallback しない。
- region がない場合は deprecated conference start path を使う。

history fetch についても、本家 Firestorm は conference start 成功後に `FetchGroupChatHistory` が有効であれば `chatterBoxHistoryCoro()` を起動する。新規 conference 作成直後の初回履歴取得を特別にスキップしていない。

そのため、今回の修正では Firestorm 本家の基本挙動から大きく外れない範囲に留めた。

## 実装内容

`startConferenceCoro()` を次のように変更した。

- `ChatSessionRequest` URL が空の場合、warning を出して deprecated conference start path へ fallback する。
- `start conference` POST 失敗時の warning log は、HTTP status のみに抑える。
- `HTTP_BAD_REQUEST` の場合は deprecated conference start path へ fallback する。
- それ以外の失敗では `generic_request_error` を表示し、無言で未初期化セッションに見える状態を減らす。

通常ログでは、timeout / HTTP error の区別に必要な HTTP status だけを残す。

## 再検証時に見るログ

再度 conference IM の作成失敗を検証する場合は、次の 2 系統を分けて確認する。

### conference start request

目的:

- conference 作成本体の `method = "start conference"` が成功しているか確認する。
- 失敗時に timeout、HTTP 400、その他 HTTP error のどれかを確認する。
- fallback が発生したか確認する。

通常ログで確認できる情報:

- `startConferenceCoro`
- `Failed to start conference request`
- HTTP status

一時的に追加ログを出す場合の項目:

- request method: `start conference`
- temp session id
- HTTP status
- fallback reason: empty cap URL / `HTTP_BAD_REQUEST` / no fallback
- user error notification: `generic_request_error` を表示したか

出さない項目:

- `ChatSessionRequest` cap URL
- full `postData`
- invitee / agent UUID 一覧

### history fetch

目的:

- `You cannot join chat` が conference 作成本体ではなく、作成後の `method = "fetch history"` で出ているか確認する。
- live conference IM の失敗と history fetch の 403 を混同しないようにする。

通常ログで確認できる情報:

- `chatterBoxHistoryCoro`
- `Bad HTTP response`
- HTTP status / `403`
- error body: `You cannot join chat`

一時的に追加ログを出す場合の項目:

- request method: `fetch history`
- session id
- 呼び出し元が session start reply 後か、新規 incoming session 処理か
- `FetchGroupChatHistory` の有効 / 無効

出さない項目:

- `ChatSessionRequest` cap URL
- full history request body
- participant list

## 変更しない判断

次の変更は今回は入れていない。

- timeout (`Easy_28`) 時の自動 fallback。
- HTTP retry policy の変更。
- session id 生成ルールの変更。
- chat history fetch の仕様変更。
- `You cannot join chat` の 403 を握りつぶす処理。

timeout 時に deprecated conference start path へ自動 fallback すると、server 側で request が遅延成功していた場合に、二重 invite / 二重 conference になる可能性がある。

`You cannot join chat` は `start conference` ではなく `fetch history` の応答であり、Firestorm 本家も同じ初回履歴取得を行っている。そのため、将来本家側で履歴取得ポリシーや server 応答の扱いが変わる可能性も考慮し、今回は仕様変更しない。

## Mac 実機確認

Mac 実機起動後、conference IM の動作をログ確認した。

確認結果:

- `Multi-person chat` セッションは作成された。
- 実際の IM 送信ログが出た。
- conference IM 自体は Mac 実機上で機能しているように見えた。
- `chatterBoxHistoryCoro` の history fetch は `403 You cannot join chat` を返すケースがあった。
- `startConferenceCoro` が `status i28` timeout を返すケースも確認した。

この結果から、`You cannot join chat` はライブ IM 本体の失敗ではなく、主に chat history / join state confirmation 側の失敗として扱うのが妥当と判断した。

## Chat style との関係

`AYAChatWindowStyle` は主に表示ルーティングに影響する。

- `FS V1 style`: `FSFloaterIM` + plain text 表示
- `FS V7 style`: `FSFloaterIM` + modern headers 表示
- `LL style`: `LLFloaterIMSession` / LL conversations 表示経路

`ChatSessionRequest` による conference start は `llimview.cpp` の共通処理なので、timeout / 403 自体は全 style で起こり得る。

ただし、LL style は `LLFloaterIMSession` 側を通るため、セッション表示、既存セッション検索、履歴再読込などの UI 症状は LL style で目立つ可能性がある。FS V1 / FS V7 は同じ `FSFloaterIM` 経路で、違いは主に表示形式である。

## 結論

今回のブランチは、conference start request 失敗時の safe fallback と最小限の診断情報を残す修正として完了とする。

22:35 木曜に見えていた `Timeout was reached` 自体を必ず成功させる修正ではない。Firestorm 本家も timeout では fallback しておらず、自動 fallback には二重 conference のリスクがあるため、今回は現状維持とした。

`You cannot join chat` は作成後の `fetch history` で発生する本家同等の経路であり、ライブ conference IM 本体の失敗ではないため、今回のブランチでは仕様変更しない。
