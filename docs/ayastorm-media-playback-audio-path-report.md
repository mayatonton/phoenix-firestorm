# AYAstorm MOAP audio to FMOD 2D implementation report

検証ブランチ: `feature/media-playback-validation-release`

検証日: 2026-05-15

文書更新: 2026-05-16

## 目次

- [PR 前サマリ](#pr-前サマリ)
- [このブランチの範囲](#このブランチの範囲)
- [実装後の音声経路](#実装後の音声経路)
- [実装内容](#実装内容)
- [メンテナ指摘への回答](#メンテナ指摘への回答)
- [Dullahan package](#dullahan-package)
- [macOS 検証結果](#macos-検証結果)
- [Windows / Linux 担当者向け確認手順](#windows--linux-担当者向け確認手順)
- [未確認項目](#未確認項目)
- [別ブランチで扱う項目](#別ブランチで扱う項目)

## PR 前サマリ

このブランチの目的は、MOAP / Web / HTML5 / YouTube など CEF 経路の音声を Viewer 側の FMOD 2D channel へ接続し、Viewer の Media volume / mute で制御できるようにすることである。

macOS 実機では、MOAP / YouTube の音声が FMOD 2D channel 経由で鳴り、Viewer の Media volume / mute で調整できることを確認した。実装途中で発生していたプチプチノイズは、FMOD 側の buffer 長調整で解消したと判断している。

このブランチでは libVLC direct media の FMOD 接続は扱わない。libVLC / direct media / MP3 / MP4 の統合は優先度を下げ、別ブランチで改めて検証する。

## このブランチの範囲

対象:

- MOAP
- Shared Media
- Web page media
- HTML5 audio / video
- YouTube など、CEF / Dullahan を通るメディア
- Viewer の Media volume / mute による音量制御
- FMOD 2D channel への接続

対象外:

- libVLC direct media
- URL 末尾が `.mp3` / `.mp4` などの直メディア再生
- Linux の GStreamer direct media
- MOAP 音声の 3D Stream / プリムスピーカー接続
- Parcel Music / Streaming Music の動作変更

## 実装後の音声経路

今回の主経路:

```text
MOAP / Web / HTML5 / YouTube
  -> CEF / Dullahan
  -> Dullahan audio callback
  -> media_plugin_cef
  -> shared memory audio ring
  -> Viewer
  -> LLMediaAudioStream
  -> FMOD 2D channel
  -> audio device
```

音量制御:

```text
Viewer Media volume / mute
  -> LLViewerMediaImpl
  -> LLMediaAudioStream
  -> FMOD channel volume
```

この構成により、macOS の CEF native output や VolumeCatcher に依存せず、Viewer 側で Media 音量を制御できる。

## 実装内容

### Dullahan / CEF

Dullahan fork に CEF audio callback API を追加し、CEF が decode した PCM を Viewer 側へ渡せるようにした。

Viewer 側では `media_plugin_cef` が Dullahan audio callback を登録し、受け取った PCM を shared memory audio ring へ書き込む。

### Shared Memory

media plugin process と Viewer process の間に audio 用 shared memory ring を追加した。

目的:

- media plugin から Viewer へ PCM を渡す
- callback thread と Viewer / FMOD 側の読み取りを分離する
- 一時的な供給揺れを吸収する
- buffer が溜まり続けないよう ring buffer として扱う

### Viewer / FMOD

Viewer 側に `LLMediaAudioStream` を追加し、shared memory ring から PCM を読み出して FMOD 2D `OPENUSER` stream として再生する。

Media volume / mute は FMOD channel 側へ反映する。フェーダー操作時だけ volume update が送られる状態をログで確認済み。

## メンテナ指摘への回答

### media format 切り替え時の FMOD sound 再作成

指摘:

CEF が新しい media に切り替わり、sample rate が 44.1 kHz から 48 kHz へ変わるような場合、`onAudioStreamStartedCallback` は ring の `mSampleRate` を更新する。しかし `LLMediaAudioStream::update()` が `!mChannel` のときだけ `start()` する実装だと、FMOD sound が古い sample rate のまま残るのではないか。

対応:

現在の実装では、audio ring に `mFormatSerial` を追加している。

`media_plugin_cef.cpp` 側では、CEF audio stream の開始 / 停止時に次の値を更新し、`mFormatSerial` を進める。

- `mSampleRate`
- `mChannels`
- `mBytesPerSample`
- `mReadFrame`
- `mWriteFrame`
- `mFormatSerial`

`LLMediaAudioStream::update()` 側では、既存の FMOD channel がある場合でも、ring 側の `sample rate` / `channels` / `format serial` と、現在の FMOD sound 作成時に保持した値を比較する。

差異があれば `stop()` してから `start(engine)` を呼び直すため、FMOD sound は新しい sample rate / channel count で作り直される。

また、FMOD の `pcmreadcallback` 実行中に format 差異を検出した場合は、その callback では silence を返し、prebuffer を要求する。これにより、format 切り替え途中の古い channel / sample rate 前提で PCM を読み続けないようにしている。

### audio ring の read/write pointer 競合

指摘:

`media_plugin_cef.cpp` 側で writer が `mReadFrame` を進めると、reader が frame 計算中に read pointer を変更される危険がある。

対応:

現在の実装では、writer 側から `mReadFrame` を進める処理を削除している。

現在の責務は次の通り。

- writer: CEF audio callback から受け取った PCM を ring に書き、最後に `mWriteFrame` を release store する
- reader: FMOD callback で `mWriteFrame` を acquire load し、読み終えたあと `mReadFrame` を release store する

ring が満杯の場合、writer は `mReadFrame` を再読み込みして空きができているか確認する。それでも満杯なら、未読の古い frame を上書きしたり read pointer を進めたりせず、その時点の入力 frame を書き込まずに捨て、`mTotalFramesDropped` を増やす。

つまり現在は、writer が reader 側の read pointer を強制的に動かさない。これにより、指摘された read/write pointer 競合の主要因を避けている。

この実装は、CEF audio callback 側の単一 writer と、FMOD callback 側の単一 reader を前提にしている。

### macOS VolumeCatcher を残している理由

指摘:

`indra/media_plugins/cef/CMakeLists.txt` で macOS の `mac_volume_catcher_null.cpp` を `mac_volume_catcher.cpp` に切り替え、QuickTime ではなく CoreServices / AudioUnit を使う理由が分かりにくい。Viewer 側で FMOD volume control をするなら VolumeCatcher に依存する必要はないのではないか。

回答:

このブランチでの Viewer 側音量制御は FMOD channel 側で行う。VolumeCatcher は、Media volume のユーザー操作を反映する主経路ではない。

現在 `media_plugin_cef.cpp` の `setVolume()` では、VolumeCatcher に常に `0.0f` を設定している。目的は、CEF native output を無音化し、同じ MOAP 音声が CEF native output と FMOD output の両方から二重再生されるのを防ぐことである。

したがって現在の役割は次の分担になる。

- Viewer Media volume / mute: `LLMediaAudioStream` から FMOD channel に反映する
- macOS VolumeCatcher: CEF native output を mute するためだけに使う

`mac_volume_catcher_null.cpp` のままだと、CEF native output を確実に無音化できず、FMOD 経由の音声と重なって聞こえる可能性がある。そのため macOS では `mac_volume_catcher.cpp` を使っている。

QuickTime へ戻すのは避けるべきである。QuickTime framework は旧来の macOS メディア API であり、現行 macOS SDK / Apple Silicon arm64 build ではビルド、リンク、将来互換性の面で不利になる。さらに今回制御したい対象は QuickTime の再生音ではなく、CEF が process 内で開く native output AudioUnit である。そのため、QuickTime 依存へ戻すよりも AudioUnit 側を mute する現在の方が目的に近い。

ただし、CoreServices / AudioUnit 版の VolumeCatcher も理想的な最終設計ではない。実装としては Component Manager / `CaptureComponent` を使い、process 内で開かれる Default Output AudioUnit を捕捉して volume を下げる互換ワークアラウンドである。

このブランチでの主経路はあくまで Dullahan audio callback から Viewer 側 FMOD 2D channel へ PCM を流す経路であり、VolumeCatcher はその主経路ではない。macOS VolumeCatcher は、CEF native output を mute して二重再生を避けるための補助策として残している。

将来的に Dullahan / CEF 側で native audio output そのものを無効化できる、または audio callback 専用出力に切り替えられるなら、VolumeCatcher への依存は削減するのが望ましい。

今回その方式にしなかった理由:

- この PR の主目的は、CEF audio callback で取得した PCM を Viewer 側 FMOD 2D channel へ接続し、Media volume / mute で制御できることを確認することである。
- CEF native output を生成しない設計に踏み込むと、Dullahan / CEF 側の audio lifecycle、platform ごとの audio backend、autoplay / mute / pause の挙動まで変更範囲が広がる。
- その変更は macOS だけでなく Windows / Linux の CEF 動作にも影響する可能性があり、この PR の macOS 実機検証だけでは十分に安全性を確認できない。
- 現時点では、native output を mute したうえで FMOD 側を主出力にする方が、既存の CEF media 再生挙動を大きく崩さずに目的を検証しやすい。

そのため、この PR では native output 無効化までは扱わず、二重再生防止を VolumeCatcher に任せる実装に留めている。native output を根本的に無効化する設計は、別ブランチで Dullahan 側の変更として検証するのが適切である。

## Dullahan package

Dullahan package はこの Viewer repository には入っていない。

この repository に入っているのは、Viewer 側の実装コードと、外部 package を取得するための `autobuild.xml` の参照設定である。

audio callback 対応 Dullahan package:

```text
https://github.com/t-noami/dullahan/releases/tag/v1.26.0-CEF_139.0.40-ayastorm-audio-callback.3
```

Viewer 側では `autobuild.xml` が上記 release asset を参照する。

## macOS 検証結果

確認済み:

- macOS Viewer build 成功。
- test app rebuild 済み。
- MOAP / YouTube の音声が FMOD 2D channel 経由で再生される。
- Viewer の Media volume / mute が MOAP / YouTube 音声に反映される。
- フェーダーを動かしていない間、volume update が出続ける状態ではない。
- 実装途中のプチプチノイズは FMOD buffer 長調整で解消したと判断。
- `ring_dropped=0` / `frames_silenced=0` の状態を確認済み。

注意:

- CEF / MOAP が対象であり、libVLC direct media の検証結果ではない。
- `media_plugin_cef.cpp` の `GL_RGB` / `GL_BGRA` は CEF の texture params であり、libVLC direct media の不具合とは別系統。

## Windows / Linux 担当者向け確認手順

1. この Viewer branch を checkout する。

```text
feature/media-playback-validation-release
```

2. `autobuild.xml` の `dullahan` dependency が `t-noami/dullahan` の audio callback 対応 release asset を参照していることを確認する。

期待する参照:

```text
https://github.com/t-noami/dullahan/releases/download/v1.26.0-CEF_139.0.40-ayastorm-audio-callback.3/
```

3. 各 OS の通常手順で Viewer build を実行する。

Dullahan package をこの repository に手動コピーする必要はない。`autobuild` が `autobuild.xml` の設定に従って取得する。

4. build が通ったら、MOAP / Web / YouTube など CEF 経路の media を再生する。

確認する動作:

- Viewer の Media volume を下げると MOAP / YouTube 音声が下がる。
- Media volume を 0 または mute にすると MOAP / YouTube 音声が消える。
- CEF native output と FMOD output の二重再生になっていない。
- 長時間再生しても `ring_dropped` / `frames_silenced` が継続的に増えない。

## 未確認項目

- Windows Viewer build。
- Windows 実機での MOAP / YouTube 再生。
- Windows 実機での Media volume / mute。
- Linux Viewer build。
- Linux 実機での MOAP / YouTube 再生。
- Linux 実機での Media volume / mute。
- 長時間再生で audio ring が破綻しないこと。

## 別ブランチで扱う項目

次はこのブランチには含めない。

- libVLC direct media の FMOD 2D 接続。
- `.mp3` / `.mp4` など direct media の再生不良調査。
- Linux GStreamer direct media の FMOD 2D 接続。
- MOAP 音声の 3D Stream / プリムスピーカー接続。

libVLC については、このブランチで入れた試行実装を戻した。優先度を下げ、別ブランチで改めて調査・実装する。
