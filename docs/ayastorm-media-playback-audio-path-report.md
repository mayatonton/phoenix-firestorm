# AYAstorm MOAP audio to FMOD 2D implementation report

検証ブランチ: `feature/media-playback-validation-release`

検証日: 2026-05-15

文書更新: 2026-05-16

## 目次

- [PR 前サマリ](#pr-前サマリ)
- [このブランチの範囲](#このブランチの範囲)
- [実装後の音声経路](#実装後の音声経路)
- [実装内容](#実装内容)
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
