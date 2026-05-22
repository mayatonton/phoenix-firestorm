# AYAstorm r24 — リリース告知

**r24 は MOAP / Web / HTML5 / YouTube など CEF / Dullahan 経路の media audio を viewer の FMOD 2D channel 経由に切り替えるリリース** — viewer の Media volume / mute がこれら CEF media にも素直に効くようになります。

> **配信形態**: r24 は r13〜r24 を一括配信するタグの 1 機能として出荷されます。同梱される他リリースの release note は GitHub Release ページから直接リンクされます。

実装・既知 limits・メンテナ指摘への回答・Win/Linux 担当者向け確認手順は永続資料 (`docs/ayastorm-r24-moap-audio-to-fmod-2d.md`) に常駐させ、本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r24 — MOAP audio to FMOD 2D channel

### r24 の柱: CEF/MOAP audio を viewer の FMOD 2D channel に乗せる

r23 までの AYAstorm では、MOAP / Web / HTML5 / YouTube など CEF / Dullahan 経路の media audio は **CEF native output (OS audio device 直行)** で再生され、viewer の Media volume / mute は `VolumeCatcher` 経由の間接制御に依存していました。Media volume を絞っても完全に止まらない・mute との挙動が不一致になる、といった現象がここから来ています。

r24 では Dullahan fork の audio callback API を経由して CEF が decode した PCM を viewer 側へ取り出し、**viewer 内の FMOD 2D channel** で再生します。Media volume / mute は FMOD channel に直接反映されるため、Parcel Music / Streaming Music と同じ感覚で MOAP / YouTube の音量を扱えます。

### 仕組み

新しい主経路:

```
MOAP / Web / HTML5 / YouTube
  → CEF / Dullahan
  → Dullahan audio callback (fork API)
  → media_plugin_cef
  → shared memory audio ring
  → viewer
  → LLMediaAudioStream
  → FMOD 2D channel (OPENUSER stream)
  → audio device
```

音量制御:

```
viewer Media volume / mute
  → LLViewerMediaImpl
  → LLMediaAudioStream
  → FMOD channel volume
```

macOS では CEF が process 内で開く native output AudioUnit を `VolumeCatcher` で常時 mute し、FMOD 経路と二重再生にならないようにしています。

### Build-time fallback switch: `LL_DULLAHAN_AUDIO_CALLBACK`

新規 CMake オプション `-DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE/FALSE` で 2 つのビルドモードを切替します。

| flag | Dullahan | audio path |
|---|---|---|
| `FALSE` (**default**) | upstream `secondlife/dullahan` v1.26.0-CEF_139.0.40 | CEF native output → OS audio device 直行 (r23 以前と同じ) |
| `TRUE` | `t-noami/dullahan` fork v1.26.0-CEF_139.0.40-ayastorm-audio-callback.3 | viewer 側 FMOD 2D channel 経由 |

`autobuild.xml` には 2 つの installable (`dullahan` / `dullahan_aya_audio`) が並んでおり、`indra/cmake/CEFPlugin.cmake` が flag を見て選んだ installable 側だけを fetch します。switch を変えて configure し直すだけで `autobuild uninstall <other>` と sentinel reset が自動で走るので、ファイル衝突は起きません。

**なぜ fallback 必要か**: fork は personal release で配布リスクがあり、fork が一時的に取り下げられた場合でも upstream `secondlife/dullahan` だけでビルド可能な状態を維持したいためです。Audio callback 経路は `#if LL_DULLAHAN_AUDIO_CALLBACK` で compile out されるので、OFF モードでは追加コードは一切入りません。

### Sample rate / channel 切替への追随

CEF media format 切替 (例: 44.1 kHz → 48 kHz) を検出するため、audio ring header に `mFormatSerial` を持たせています。`media_plugin_cef` の writer 側は新しい stream の開始時に sample rate / channel count / serial を進めます。`LLMediaAudioStream::update()` の reader 側は既存の FMOD channel の作成値とのずれを検出すると `stop()` → `start()` で FMOD sound を作り直すため、format 切替後の古い設定で PCM を読み続けることはありません。

ring buffer は writer (CEF audio callback、単一 thread) が `mWriteFrame` を release store、reader (FMOD callback、単一 thread) が `mReadFrame` を release store する **single-writer / single-reader** 設計です。writer が reader の pointer を動かさないため、frame 計算中に read pointer が変わる競合は発生しません。ring が満杯のときは writer 側で書き込まずに drop し、`mTotalFramesDropped` を増やします。

### macOS VolumeCatcher の役割

このリリースで残している `mac_volume_catcher.cpp` (CoreServices / AudioUnit 版) は、**CEF native output を常時 mute するためだけ** に使っています。Media volume の制御主経路は FMOD channel 側です。`mac_volume_catcher_null.cpp` (no-op) のままだと CEF native output が鳴り続け、FMOD 経路と二重に聞こえてしまうため、ON モードでは null 版に戻していません。

QuickTime 依存版 (旧来) には戻していません — Apple Silicon arm64 ビルドおよび現行 macOS SDK では現実的でないためです。将来的に Dullahan / CEF 側で native audio output 自体を生成しない設計に移行できれば、VolumeCatcher への依存は削減できる見込みです。

### 設定

**意図的にゼロ。** Viewer 側に新 cvar / UI 追加はありません。

- 切替はビルド時オプション (`LL_DULLAHAN_AUDIO_CALLBACK`) のみ
- Runtime での切替不可 (異なる installable / 異なる Dullahan API を要するため)
- ユーザー操作は既存の Media volume / mute スライダーで一貫

### 既知の制約

- **Windows / Linux 実機検証**: r24 push 時点で macOS のみ ON モードでの実機 PASS。Windows / Linux の ON モードビルド・動作確認はリリース後に随時実施
- **libVLC direct media / `.mp3` `.mp4` 直 URL / Linux GStreamer**: 対象外。CEF/MOAP 経路だけが FMOD 2D に切り替わる
- **3D stream / parcel music / Voice**: 既存経路を維持
- **長時間再生**: short-term で `ring_dropped=0` / `frames_silenced=0` 確認済。多時間継続テストは未実施

### 実装概要

- 新規 file: `indra/llaudio/llmediaaudiostream.cpp/.h` (FMOD 2D OPENUSER stream)、`indra/llcommon/llpluginaudio.h` (ring header struct)
- 変更 file: `llpluginclassmedia.cpp/.h` (`ensureAudioSharedMemory()` / `getAudioData()`)、`media_plugin_cef.cpp` (callback 登録、`audio_shm_set` message、`writeAudioPacketToRing`)、`llviewermedia.cpp/.h` (`mMediaAudioStream` メンバ追加)、`mac_volume_catcher.cpp` (CEF native output mute)
- `autobuild.xml`: 2 installable (`dullahan` / `dullahan_aya_audio`) 併記
- `indra/CMakeLists.txt` / `indra/cmake/CEFPlugin.cmake`: flag による切替分岐
- macOS 実機ビルド + ON モード動作確認 PASS、Linux / Windows ON モードはリリース後検証

### Credits

r24 の本実装 (Dullahan fork audio callback + viewer 側 FMOD 2D 接続 + shared memory ring + macOS VolumeCatcher 戦略) は [t-noami](https://github.com/t-noami) さんによるものです。AYAstorm 側では、fork が利用不可になった場合でも upstream `secondlife/dullahan` だけでビルドできるよう `LL_DULLAHAN_AUDIO_CALLBACK` build-time fallback switch を追加して取り込みました。

### ドキュメント

- r24 full spec / メンテナ指摘への回答 / Dullahan package / Windows / Linux 担当者向け確認手順: `docs/ayastorm-r24-moap-audio-to-fmod-2d.md`
- Dullahan fork release: `https://github.com/t-noami/dullahan/releases/tag/v1.26.0-CEF_139.0.40-ayastorm-audio-callback.3`
