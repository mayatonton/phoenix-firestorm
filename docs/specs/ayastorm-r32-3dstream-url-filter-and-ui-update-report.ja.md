# AYAstorm r32 3D Stream URL Filter and UI Update 修正報告書

作成日: 2026-05-29
更新日: 2026-05-30

## 目次

- [概要](#概要)
- [対象範囲](#対象範囲)
- [修正前の状態](#修正前の状態)
- [修正内容](#修正内容)
- [実機確認結果](#実機確認結果)
- [主な変更ファイル](#主な変更ファイル)
- [修正後の仕様](#修正後の仕様)
- [結論](#結論)

## 概要

AYAstorm の 3D Stream 機能について、オブジェクト Description に記述された URL source が、ユーザー確認を経ずに外部サーバーへ接続され得る問題を確認し、修正した。

修正後は、3D Stream の URL source は再生前に許可確認を行う。ユーザーが許可しない限り、FMOD へ URL を渡さない。これにより、Description タグを置くだけで AYAstorm viewer に外部 URL へ接続させる挙動を防止する。

## 対象範囲

対象は 3D Stream が自分で URL を開く経路である。

- `[3dstream:{url:...}]`
- distributed / linkset 形式の `{url:...}`
- 上記 URL source の reconnect / retry

`[3dstream:{source:media}]` は、既存 MOAP media の audio ring を 3D Stream の定位処理へ渡す経路であり、3D Stream が新しい外部 URL を直接開くものではない。そのため、今回の URL source gate の対象外とした。

通常の MOAP / parcel media については、既存の media filter / whitelist 経路を維持している。

## 修正前の状態

修正前は、以下の経路で Description 由来の URL が FMOD stream へ渡されていた。

- mono URL source:
  - `LLPositionalStreamMgr::evaluateMonoBinding()`
  - `LLPositionalStream::start()`
- distributed URL source:
  - `LLPositionalStreamMgr::evaluateLinkset()`
  - `LLPositionalStreamMulti::start()`
- reconnect / retry:
  - `LLPositionalStreamMgr::update()`

この経路は Firestorm の media filter / MOAP whitelist と独立しており、ユーザー確認前に接続処理へ進み得た。

## 修正内容

### 1. 3D Stream の初期状態を opt-in に変更

`Stream3DEnabled` の初期値を `0` に変更した。

これにより、新規設定状態では 3D Stream は自動的に有効化されない。ユーザーが明示的に 3D Stream を有効化した場合のみ、Description scan と再生候補の評価が進む。

`Stream3DDescriptionScan` は詳細設定として残し、初期値は `1` のままとした。通常ユーザー向けの opt-in は `Stream3DEnabled` に一本化している。

### 2. URL source の許可確認を追加

3D Stream の URL source に対して、再生直前の permission gate を追加した。

実装上は、FMOD `createStream()` に到達する前に URL を検査する。未知 URL の場合は 3D Stream 専用の確認 dialog を表示し、ユーザーが Allow した場合だけ再生処理へ進む。

主な挙動は以下。

- allow list 一致: 再生を許可する。
- deny list 一致: 再生しない。
- 未知 URL: 3D Stream 専用 dialog を表示する。
- Allow Now: viewer session 内で同一 URL を許可する。
- Deny Now: viewer session 内で同一 URL を拒否し、同じ URL で確認を繰り返さない。
- Always Allow / Never Allow: 既存 media filter の domain / URL 記憶モデルに従う。

### 3. reconnect / retry の bypass を防止

初回再生だけでなく、stream failure 後の reconnect / retry にも同じ URL permission gate を適用した。

これにより、初回評価で未許可だった URL が retry 経路から FMOD に渡ることを防止している。

### 4. URL scheme を制限

3D Stream URL source は `http://` と `https://` のみ許可する。

それ以外の scheme は、prompt 前または audio layer 側で拒否する。

拒否対象の例:

- scheme なし
- `file:`
- `ftp:`
- `rtsp:`
- その他 viewer 側で明示的に許可していない scheme

manager 側だけでなく、`LLPositionalStream::start()` / `LLPositionalStreamMulti::start()` 側にも防御的 check を入れている。

### 5. 3D Stream 専用 notification を追加

3D Stream URL source 用に `Stream3DAudioAlert` / `Stream3DAudioAlert2` を追加した。

表示内容は、接続先 domain / URL とユーザー選択に絞り、土地音楽の確認 dialog に近い構成とした。

基本 dialog:

```xml
This object provides a 3D audio stream from:

Domain: [AUDIODOMAIN]
URL: [AUDIOURL]
```

選択肢:

- Allow
- Deny
- Always Allow This Domain
- Always Allow This URL
- Never Allow This Domain
- Never Allow This URL

### 6. 3D Stream UI を追加・調整

ステータスバーの media controls に 3D Stream button を追加した。

- 未再生時: 3D Stream アイコン
- 3D Stream 再生中: pause アイコン
- クリック時: 表示アイコン状態ではなく `Stream3DEnabled` の現在値を反転する

また、ステータスバー右側 panel の幅は広げず、時刻表示幅を調整して既存領域に収めた。これにより、FPS 表示やシミュレーターバージョン表示用の区画情報領域を右へ押し出さない。

### 7. 音量 UI を追加

Preferences の Sound パネルとステータスバーの volume pulldown に 3D Stream の音量行を追加した。

3D Stream 音量行には以下を配置している。

- `Stream3DVolumeMaster` slider
- `MuteStream3D` mute button
- `Stream3DEnabled` checkbox

日本語 XUI では、既存の音量行と同じ幅になるよう `Stream3D Volume` の label / slider 幅を上書きした。

## 実機確認結果

macOS 実機で以下を確認済み。

- 3D Stream が無効の場合、Description scan 由来の 3D Stream URL source は再生されない。
- 3D Stream を有効化した状態で未知 URL source を検出すると、再生前に確認 dialog が出る。
- Deny を選択した URL は接続されない。
- Allow を選択した URL は再生処理へ進む。
- 同一 session 内で Deny した URL は、同じ URL で繰り返し確認されない。
- unsupported scheme は再生前に拒否される。
- 3D Stream 再生中、ステータスバーの 3D Stream button は pause 表示になる。
- volume pulldown の 3D Stream 行は、他の音量行と同じ幅・配置で表示される。
- 通常 MOAP / parcel media の再生経路は維持される。
- `{source:media}` で 3D Stream に渡す media 音声について、実機上の再生挙動に問題がないことを確認した。

## 主な変更ファイル

- `indra/newview/app_settings/settings.xml`
- `indra/newview/llpositionalstreammgr.cpp`
- `indra/newview/llpositionalstreammgr.h`
- `indra/llaudio/llpositionalstream.cpp`
- `indra/llaudio/llpositionalstreammulti.cpp`
- `indra/newview/llviewerparcelmedia.cpp`
- `indra/newview/llviewerparcelmedia.h`
- `indra/newview/llviewercontrol.cpp`
- `indra/newview/llstatusbar.cpp`
- `indra/newview/llstatusbar.h`
- `indra/newview/skins/default/xui/en/notifications.xml`
- `indra/newview/skins/default/xui/en/panel_preferences_sound.xml`
- `indra/newview/skins/default/xui/en/panel_status_bar.xml`
- `indra/newview/skins/default/xui/ja/panel_status_bar.xml`
- `indra/newview/skins/default/xui/en/panel_volume_pulldown.xml`
- `indra/newview/skins/default/xui/ja/panel_volume_pulldown.xml`
- `indra/newview/skins/metaharper/xui/en/panel_volume_pulldown.xml`
- `indra/newview/skins/default/textures/textures.xml`
- `indra/newview/skins/default/textures/icons/3dstream_Off.png`
- `indra/newview/skins/default/textures/icons/3dstream_Over.png`
- `indra/newview/skins/default/textures/icons/3dstream_Press.png`

## 修正後の仕様

修正後の 3D Stream URL source は、以下の仕様で扱う。

1. `Stream3DEnabled` が off の場合、3D Stream の prim-bound 再生は開始しない。
2. `Stream3DEnabled` が on の場合、Description scan は音源候補を評価する。
3. URL source は HTTP/HTTPS のみ受け付ける。
4. URL source は FMOD に渡す前に allow / deny / prompt を通す。
5. Deny または unsupported scheme は fail-closed とする。
6. `{source:media}` は URL source ではなく、既存 MOAP media ring を 3D Stream の定位処理に使う。
7. 通常 MOAP / parcel media の既存 permission model は変更しない。

## 結論

3D Stream URL source が既存 media protection と独立して外部 URL へ進み得た問題について、再生前 permission gate、URL scheme 制限、session allow/deny、UI opt-in を追加した。

これにより、ユーザーが 3D Stream を有効化している場合でも、未知 URL source は確認なしに再生されない状態になった。
