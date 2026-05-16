# AYAstorm r25 土地音楽: Ogg Vorbis live stream 調査メモ

## 目次

- [現在の結論](#現在の結論)
- [r25 実装内容](#r25-実装内容)
- [検証結果サマリ](#検証結果サマリ)
- [問題の概要](#問題の概要)
- [r25 以前の原因仮説](#r25-以前の原因仮説)
- [非対象](#非対象)
- [検証用 switch](#検証用-switch)
- [検証ログ](#検証ログ)
- [成功判定と失敗判定](#成功判定と失敗判定)
- [r25 実装方針と実装済み内容](#r25-実装方針と実装済み内容)
- [以前の Opus codec 修正について](#以前の-opus-codec-修正について)
- [設計メモ](#設計メモ)
- [6ch / multichannel](#6ch--multichannel)
- [やってはいけないこと](#やってはいけないこと)

## 現在の結論

この文書は、土地音楽 / FMOD stream 経路で Ogg Vorbis live stream が再生できない問題の調査用である。

このブランチの目的は、AYAstorm の Opus 再生を維持したまま、リリース版で壊れている Ogg Vorbis parcel music 再生を直すことである。

MOAP / CEF / Dullahan の FMOD 2D audio PR とは別系統として扱う。

対象経路:

```text
Parcel music URL
-> LLStreamingAudio_FMODSTUDIO
-> FMOD::System::createStream(url)
-> FMOD internet stream channel
```

AYAstorm では Opus 再生を捨ててはいけない。`AYAOpusCodecEnable=false` は診断用であり、製品仕様ではない。

r25 修正では、Opus 再生を維持したまま Ogg Vorbis live stream も再生できる設計にする。

## r25 実装内容

r25 では、AYAstorm の custom FMOD codec を Ogg Opus 専用 probe から Ogg Opus / Ogg Vorbis 両対応 codec に拡張した。

主な実装:

- `fmod_codec_opus.cpp` を Ogg family codec として拡張した。
- Ogg first packet を見て、`OpusHead` なら既存 Opus decode path、`\x01vorbis` なら新規 Vorbis decode path に分岐する。
- Vorbis と判定した stream は FMOD built-in Vorbis codec へ fallback させず、この custom codec 内で最後まで decode する。
- Vorbis decode state として `vorbis_info` / `vorbis_comment` / `vorbis_dsp_state` / `vorbis_block` を追加した。
- Vorbis audio packet は `vorbis_synthesis()` / `vorbis_synthesis_blockin()` / `vorbis_synthesis_pcmout()` で float PCM に変換し、FMOD へ interleaved `PCMFLOAT` として返す。
- Opus path は維持した。`channel_mapping_family == 0` の mono/stereo と、`channel_mapping_family == 1` の multistream は従来どおり `opus_decoder_create()` / `opus_multistream_decoder_create()` を使う。
- `LLAudioEngine_FMODSTUDIO::init()` の codec 登録は 1 本のまま維持し、登録名を `AYAstorm Ogg Opus/Vorbis codec` に更新した。
- parcel music 経路は維持した。`LLStreamingAudio_FMODSTUDIO` は引き続き `FMOD::System::createStream(url, FMOD_2D | FMOD_NONBLOCKING | FMOD_IGNORETAGS, ...)` を使う。
- 3D Stream 経路も維持した。FMOD plugin codec が `FMOD_SOUND_TYPE_UNKNOWN` として見える場合に備え、`Sound::getName()` の `"Ogg Opus"` / `"Ogg Vorbis"` を `FMOD_SOUND_TYPE_OPUS` / `FMOD_SOUND_TYPE_OGGVORBIS` へ昇格する処理を追加した。
- `AYAOpusCodecEnable` / `AYAOpusCodecPriority` は診断用として残す。通常値は `AYAOpusCodecEnable=true` / priority `0`。

意図:

```text
FMOD createStream(Ogg live URL)
-> AYAstorm Ogg Opus/Vorbis codec が priority 0 で呼ばれる
-> Ogg first packet を一度だけ読む
     OpusHead なら Opus decoder
     \x01vorbis なら Vorbis decoder
-> PCM float を FMOD mixer へ渡す
```

これにより、non-seekable Ogg Vorbis live stream をいったん Opus codec が読んでから `FMOD_ERR_FORMAT` で返し、FMOD built-in Vorbis codec へ fallback しようとして seek failure になる経路を避ける。

変更した主なファイル:

```text
indra/llaudio/fmod_codec_opus.cpp
indra/llaudio/fmod_codec_opus.h
indra/llaudio/llaudioengine_fmodstudio.cpp
indra/llaudio/llpositionalstreammulti.cpp
indra/newview/app_settings/settings.xml
```

Opus 側の channel mapping は変更していない。`OpusHead` の `channel_mapping_family == 1` は従来どおり `opus_multistream_decoder_create()` に渡し、既存の multichannel Opus 再生経路を維持する。

## 検証結果サマリ

2026-05-17 時点で、Mac 版では r25 実装後の Ogg Vorbis parcel music 再生が動作することを確認した。

`http://s9.voscast.com:8032/stream.mp3` も再生できた。この URL は suffix が `.mp3` だが実体は Ogg Vorbis として開かれるため、r25 実装の重要な成功例として扱う。

3D Stream の 6 speaker routing も維持できていることを確認した。2ch Vorbis source を 6 speakers へ routing/upmix する経路に加えて、`http://go-stream-live.com:8030/stream` の Opus 5.1ch surround source も再生成功を確認した。今回の修正は Ogg Vorbis live stream を custom FMOD codec 内で decode できるようにするものであり、既存の Opus multichannel / 3D Stream routing を削るものではない。

検証環境:

```text
Branch: ayastorm-r25-parcel-music-ogg-vorbis-investigation
Base commit at implementation start: 5c86045ad9
Build: macOS arm64 Release ayastorm-bin
Runtime log: ~/Library/Application Support/Firestorm/logs/AYAstorm.log
Runtime version log: Firestorm-AYAstorm-release 7.2.4.261361150 [0dad288735]
Runtime OS log: macOS 15.7.5 / Apple M2 Pro
Codec registration log: Ogg Opus/Vorbis codec registered (handle=53, priority=0)
```

主な検証用 Ogg Vorbis 土地音楽 URL:

```text
https://azuracast.tilderadio.org/radio/8000/radio.ogg
```

これは CEF / MOAP ではなく、土地音楽として既存の FMOD streaming audio 経路で処理される。

`http://krominancia.org:7110/live` は初期調査で問題になった URL だが、配信が止まっていることが多いため、継続検証の主対象からは外す。

外部 probe では、Ogg Vorbis live stream は次の形式として確認できる。

```text
Icecast 2.4.2
Content-Type: application/ogg
Ogg Vorbis audio
44100 Hz stereo
約 192 kbps
```

## 問題の概要

AYAstorm は土地音楽 URL を受け取り、FMOD stream の開始までは進む。

しかしすぐに FMOD 側で失敗し、再生が止まる。

代表的なログ:

```text
Starting internet stream: https://azuracast.tilderadio.org/radio/8000/radio.ogg
FMOD::Sound::getOpenState Error: Couldn't perform seek operation.
Internet stream openstate error: open_state = 2 - progress = 0 - starving = 0 - diskbusy = 210
Stopping internet stream: https://azuracast.tilderadio.org/radio/8000/radio.ogg
```

FMOD header 上、`open_state = 2` は次に相当する。

```text
FMOD_OPENSTATE_ERROR
```

エラー文字列は次に相当する。

```text
FMOD_ERR_FILE_COULDNOTSEEK
```

## r25 以前の原因仮説

Firestorm 本家では再生できる可能性がある。

r25 以前の AYAstorm では Ogg Opus 再生のために、独自の FMOD codec plugin を登録していた。

```cpp
mSystem->registerCodec(FMODGetCodecDescriptionOpus(), &opus_codec_handle, 0);
```

priority `0` は FMOD codec priority として高い。これは AYAstorm の Opus support に必要だった。FMOD 組み込みの Ogg/Vorbis 側が Ogg Opus を先に拾って失敗するのを避けるためである。

ただし、Ogg Vorbis live stream に対してこの設計は危険だった。

想定される失敗経路。これは codec fallback と non-seekable stream の組み合わせからの推論であり、FMOD 内部の codec probe 順を直接 trace したものではない。

```text
FMOD createStream(Ogg Vorbis live URL)
-> AYAstorm Opus codec が先に probe する
-> OggS と最初の Ogg packet を読む
-> 最初の packet は "vorbis" であり "OpusHead" ではない
-> Opus codec は FMOD_ERR_FORMAT を返す
-> FMOD は組み込み Ogg/Vorbis codec へ fallback しようとする
-> live stream は先頭へ巻き戻せない
-> FMOD_ERR_FILE_COULDNOTSEEK
-> FMOD_OPENSTATE_ERROR
-> 土地音楽経路が stream を停止する
```

この仮説は、Ogg Vorbis live stream だけが失敗し、他の土地音楽 stream が再生できる状況と整合する。

ただし、これだけが唯一の原因とは限らない。

後述の追加検証では、Opus codec registration を一時的に無効化しても失敗する Ogg Vorbis live stream があった。

## 非対象

これは CEF / MOAP の FMOD 2D media channel 経路ではない。

MOAP audio の経路:

```text
CEF / Dullahan audio callback
-> media_plugin_cef
-> shared memory audio ring
-> Viewer
-> LLMediaAudioStream
-> FMOD 2D channel
```

今回の土地音楽問題は、`llstreamingaudio_fmodstudio.cpp` の既存 streaming audio 経路で起きている。

## 検証用 switch

Opus は AYAstorm に必須である。

そのため、検証用 switch は Opus 実装を削除するものではない。

仮説検証のため、次の debug setting を追加した。

```text
AYAOpusCodecEnable
AYAOpusCodecPriority
```

通常の期待値:

```text
AYAOpusCodecEnable = true
AYAOpusCodecPriority = 0
```

検証時のみ:

```text
AYAOpusCodecEnable = false
```

検証手順:

1. `AYAOpusCodecEnable` を `false` にする。
2. Viewer を再起動する。
3. `https://azuracast.tilderadio.org/radio/8000/radio.ogg` など、稼働中の Ogg Vorbis live stream を土地音楽として設定した区画に入る。
4. 土地音楽が開始されるか確認する。

もし AYAstorm Opus codec を登録しない状態で Ogg Vorbis 土地音楽が再生できるなら、独自 Opus codec の probe が Ogg Vorbis live stream を壊している可能性が高い。

ただし、この switch は最終製品仕様ではない。Opus を無効化したままにしてはいけない。

## 検証ログ

## 追加で再現した stream

追加で確認した URL:

```text
http://s9.voscast.com:8032/stream.mp3
```

この URL は末尾が `.mp3` だが、実体は MP3 ではない。

サーバ応答:

```text
HTTP/1.0 200 OK
Server: Icecast 2.4.4
Content-Type: application/ogg
ice-audio-info: ice-samplerate=44100;ice-bitrate=128;ice-channels=2
icy-name: Cliff Richard Radio
```

先頭 bytes は Ogg:

```text
OggS ... vorbis
```

`ffprobe` では正常に Ogg Vorbis として判定される。

```text
Input #0, ogg
Audio: vorbis, 44100 Hz, stereo, 128 kb/s
```

r25 実装前の AYAstorm/FMOD built-in 経路では再生できなかった。

r25 実装前の該当ログ:

```text
Starting internet stream: http://s9.voscast.com:8032/stream.mp3
FMOD::Sound::getOpenState Error: Couldn't perform seek operation.
Internet stream openstate error: open_state = 2 - progress = 0 - starving = 0 - diskbusy = 70
Stopping internet stream: http://s9.voscast.com:8032/stream.mp3
```

この検証時点では、AYAstorm Opus codec registration は一時的に無効化されていた。

そのため、この URL の失敗だけでは「Opus codec が Vorbis を横取りした」とは断定できない。

一方で、FMOD の通常 stream open 経路が、non-seekable Ogg Vorbis live stream、特に URL suffix と実体形式が食い違う stream を開けないケースがあることは確認できた。

r25 実装後は、この URL は custom Ogg Opus/Vorbis codec で Vorbis として開けるようになった。

```text
FmodOgg:
  Vorbis stream opened: 2ch 44100Hz PCMFLOAT

3D Stream:
  Multi source ready: http://s9.voscast.com:8032/stream.mp3 44100 Hz x 2 ch, fmt=PCMFLOAT
  Multi decode thread started for http://s9.voscast.com:8032/stream.mp3
  Multi path playing: http://s9.voscast.com:8032/stream.mp3
```

## 追加検証: FMOD built-in 経路で再生できた Ogg Vorbis stream

検証用 arm64 build では、AYAstorm Opus codec registration を一時的に無効化している。

ログ:

```text
LLAudioEngine_FMODSTUDIO::init() AYAstorm Opus codec registration disabled for validation
```

この状態で、FMOD built-in の internet stream 経路だけでも再生開始できる Ogg Vorbis live stream があることを確認した。

### Radio FRO 128 kbps

URL:

```text
http://www.fro.at:8008/fro-128.ogg
```

ログ:

```text
LLViewerParcelMedia::filterAudioUrl : Audio URL filter: no active alert, filtering new URL: http://www.fro.at:8008/fro-128.ogg
Starting internet stream: http://www.fro.at:8008/fro-128.ogg
```

この URL では、次のエラーは出ていない。

```text
FMOD::Sound::getOpenState Error
Internet stream openstate error
```

その後、一度 starvation は出た。

```text
Stream starvation detected! Pausing stream until buffer nearly full.
  (diskbusy=1)
  (progress=49)
```

これは open failure ではなく、FMOD stream buffer の一時的な枯渇として扱う。

### Radio FRO 64 kbps

URL:

```text
http://www.fro.at:8008/fro-64.ogg
```

ログ:

```text
Stopping internet stream: http://www.fro.at:8008/fro-128.ogg
LLViewerParcelMedia::filterAudioUrl : Audio URL filter: no active alert, filtering new URL: http://www.fro.at:8008/fro-64.ogg
Starting internet stream: http://www.fro.at:8008/fro-64.ogg
```

この URL でも、start 直後の `FMOD::Sound::getOpenState Error` は確認されていない。

### tilderadio 192 kbps

URL:

```text
https://azuracast.tilderadio.org/radio/8000/radio.ogg
```

保存ログ:

```text
/private/tmp/ayastorm-tilderadio-parcel-music-log.txt
```

ログ:

```text
Starting internet stream: https://azuracast.tilderadio.org/radio/8000/radio.ogg
Stopping internet stream: https://azuracast.tilderadio.org/radio/8000/radio.ogg
Pushing stream to dead list: https://azuracast.tilderadio.org/radio/8000/radio.ogg
Closed dead stream
LLViewerParcelMedia::filterAudioUrl : Audio URL filter: no active alert, filtering new URL: https://azuracast.tilderadio.org/radio/8000/radio.ogg
Starting internet stream: https://azuracast.tilderadio.org/radio/8000/radio.ogg
Stream starvation detected! Pausing stream until buffer nearly full.
  (diskbusy=1)
  (progress=49)
```

最初の start/stop は、parcel audio filter / allow toast の前後で stream が差し替わったためと見られる。

allow 後の start では、次のエラーは確認されていない。

```text
FMOD::Sound::getOpenState Error
Internet stream openstate error
```

45 秒程度の追加 tail でも、対象 URL について追加の openstate error や stop は出なかった。

## 追加検証: URL 側の問題として扱うもの

次の URL は FMOD 側で `File not found` になった。

```text
http://stream-dc1.radioparadise.com/rp_96m.ogg
http://stream-tx3.radioparadise.com/rp_96.ogg
```

ログ:

```text
FMOD::Sound::getOpenState Error: File not found.
Internet stream openstate error: open_state = 2 - progress = 0 - starving = 0 - diskbusy = 115
```

これは Ogg Vorbis live stream 一般の失敗とは分ける。

現時点では、FMOD が Vorbis を decode できない証拠ではなく、配信 URL / mount point の不整合または現行 availability の問題として扱う。

次の URL は FMOD 側で host 接続失敗になった。

```text
https://radio.lapfoxradio.com/radio/8000/stream-ogg-128.ogg
```

ログ:

```text
FMOD::Sound::getOpenState Error: Couldn't connect to the specified host.
Internet stream openstate error: open_state = 2 - progress = 0 - starving = 0 - diskbusy = 115
```

これも codec 判定失敗とは分ける。

## 追加検証からの暫定結論

Opus codec registration を無効化した検証 build では、FMOD built-in internet stream 経路だけで再生開始できる Ogg Vorbis live stream が複数ある。

したがって、短期的には次の見方が妥当である。

- FMOD built-in Ogg Vorbis が常に壊れているわけではない。
- URL / server / mount point の状態による失敗と、codec probe / non-seekable stream による失敗を分けて扱う必要がある。
- `Stream starvation detected` は open failure ではない。
- `FMOD::Sound::getOpenState Error` と `Internet stream openstate error` の有無を、検証ログ上の失敗判定に使う。

r25 実装前の `AYAOpusCodecEnable=false` 検証での成功扱い:

```text
http://www.fro.at:8008/fro-128.ogg
http://www.fro.at:8008/fro-64.ogg
https://azuracast.tilderadio.org/radio/8000/radio.ogg
```

これらは、少なくとも parcel music URL として FMOD stream start まで到達し、openstate error は確認されていない。

## 追加検証: 3D Stream 経路

同じ Ogg Vorbis live stream を 3D Stream 経路でも確認した。

保存ログ:

```text
/private/tmp/ayastorm-3dstream-tilderadio-log.txt
```

対象 URL:

```text
https://azuracast.tilderadio.org/radio/8000/radio.ogg
```

3D Stream object description:

```text
[3dstream-stereo:{url:https://azuracast.tilderadio.org/radio/8000/radio.ogg}{bin:off}{v:d}{wg:0.0}{upmix:on}]
```

ログ:

```text
Submitted async pre-resolve for 'https://azuracast.tilderadio.org/radio/8000/radio.ogg' (id=1) with 6 speaker(s)
[3dstream-stereo] binding constructed root=0bd1814b-a05f-a617-f65f-f982eeeffb8f url=https://azuracast.tilderadio.org/radio/8000/radio.ogg speakers=6 (dropped=0) stream=started
Multi source ready: https://azuracast.tilderadio.org/radio/8000/radio.ogg 48000 Hz x 2 ch, fmt=PCM16, ring cap 32768 frames x 2 tracks, speakers=6
Multi decode thread started for https://azuracast.tilderadio.org/radio/8000/radio.ogg
Multi path playing: https://azuracast.tilderadio.org/radio/8000/radio.ogg (6 speakers)
```

この経路では、FMOD stream start だけではなく、3D Stream 側の multi source が `ready` になり、decode thread が起動し、`Multi path playing` まで到達した。

つまり `tilderadio` の Ogg Vorbis stream は、3D Stream 経路では成功扱いでよい。

r25 実装前は、3D Stream 側でも `s9.voscast.com` の失敗を再現していた。

URL:

```text
http://s9.voscast.com:8032/stream.mp3
```

ログ:

```text
Opening multi source 'http://s9.voscast.com:8032/stream.mp3' with 6 speaker(s)
[3dstream-stereo] binding constructed root=0bd1814b-a05f-a617-f65f-f982eeeffb8f url=http://s9.voscast.com:8032/stream.mp3 speakers=6 (dropped=0) stream=started
Multi source open failed: http://s9.voscast.com:8032/stream.mp3 (Couldn't perform seek operation.  This is a limitation of the medium (ie netstreams) or the file format.)
[3dstream-stereo] stream for root 0bd1814b-a05f-a617-f65f-f982eeeffb8f failed; scheduling reconnect 1/3 in 5s
[3dstream-stereo] stream for root 0bd1814b-a05f-a617-f65f-f982eeeffb8f exhausted 3 reconnect attempts; dropping binding
[3dstream-stereo] root 0bd1814b-a05f-a617-f65f-f982eeeffb8f gone; tearing down binding
```

3D Stream の観点では、次のように切り分ける。

- `tilderadio` は Ogg Vorbis live stream として decode/play まで成功。
- `s9.voscast.com` は r25 実装前には non-seekable stream / server response / format-probe の組み合わせで seek failure。
- `s9.voscast.com` は r25 実装後には custom Ogg Opus/Vorbis codec で `Vorbis stream opened` まで到達し、3D Stream でも `Multi path playing` まで到達。
- したがって、Ogg Vorbis live stream 全体が 3D Stream で壊れているわけではない。
- 失敗判定は `Multi source open failed` と reconnect exhaustion を見る。

## 成功判定と失敗判定

このブランチの目的は、土地音楽 / parcel music の Ogg Vorbis 再生を、Opus support を壊さずに復旧することである。

リリース版で問題になっているのは、Ogg Vorbis live stream が FMOD openstate error で停止することである。

```text
Parcel music URL
-> FMOD createStream(url)
-> getOpenState()
-> FMOD_OPENSTATE_ERROR
-> FMOD_ERR_FILE_COULDNOTSEEK / File not found / host connection failure
-> stop()
```

この失敗は、AYAstorm の custom Ogg Opus codec registration と FMOD built-in Ogg/Vorbis codec の fallback 競争が、non-seekable HTTP stream で破綻している可能性がある。

したがって、このブランチでは次を両立させる必要がある。

```text
Ogg Opus:
  AYAstorm の Opus 再生を維持する

Ogg Vorbis:
  parcel music として再生できるようにする
  FMOD_OPENSTATE_ERROR で即停止しないようにする
```

3D Stream のログは、同じ Ogg Vorbis URL が別経路でどう扱われるかを見るための補助材料である。ブランチの主対象は parcel music である。

このブランチの成功判定は、まず parcel music 側で行う。

ログ上の成功扱い:

```text
Starting internet stream
FMOD::Sound::getOpenState Error なし
Internet stream openstate error なし
```

実際の再生成功は、上記に加えて、一定時間の再生継続と聴感確認を合わせて判定する。

失敗扱い:

```text
FMOD::Sound::getOpenState Error
Internet stream openstate error
Stopping internet stream
```

`Stream starvation detected` は、このブランチの主対象である openstate failure とは分ける。
starvation は buffer 状態の問題であり、単独では open failure と判定しない。

今回のログから言えること:

```text
fro-128.ogg:
  parcel music start 成功
  openstate error なし

fro-64.ogg:
  parcel music start 成功
  openstate error なし

tilderadio radio.ogg:
  parcel music start 成功
  openstate error なし
  3D Stream でも ready / playing まで到達

s9.voscast.com stream.mp3:
  URL suffix は .mp3 だが実体は Ogg Vorbis
  r25 実装前は parcel music / 3D Stream の両方で seek failure
  r25 実装後は Vorbis stream opened / Multi path playing まで到達
```

したがって、このブランチで次に判断すべきことは、Ogg Vorbis 一般の再生可否だけではない。

重要なのは、Opus support を維持したリリース構成で、Ogg Vorbis parcel music を壊さない修正方針を決めることである。

判断点:

```text
FMOD openstate error を「すべて codec 問題」と扱わず、
URL / server / mount point / non-seekable stream の違いとして分類できるか。

parcel music で start できた Ogg Vorbis stream を成功例として残し、
r25 実装前に seek failure になっていた stream が r25 実装後に改善しているか。

ただし最終的には、AYAstorm Opus codec を有効にしたまま、
確認済み Ogg Vorbis stream が parcel music として再生できる必要がある。
```

## r25 実装方針と実装済み内容

方針は、Opus codec を外すことではない。

`registerCodec()` で登録する Ogg 系 codec を、Opus 専用 probe ではなく Ogg family codec にする。

r25 以前の問題:

```text
FMOD createStream(Ogg Vorbis live URL)
-> AYAstorm Opus codec が priority 0 で先に呼ばれる
-> first packet まで読む
-> "OpusHead" ではなく "\x01vorbis" なので FMOD_ERR_FORMAT
-> FMOD built-in Vorbis に fallback しようとする
-> live stream は巻き戻せない
-> FMOD_OPENSTATE_ERROR / FMOD_ERR_FILE_COULDNOTSEEK
```

r25 ではこれを次の形に変えた。

```text
FMOD createStream(Ogg live URL)
-> AYAstorm Ogg family codec が priority 0 で先に呼ばれる
-> first packet を一度だけ読む
     "OpusHead" なら Opus decoder
     "\x01vorbis" なら Vorbis decoder
     それ以外なら FMOD_ERR_FORMAT
-> fallback 競争なしで PCM を FMOD に渡す
```

r25 で実装した要点:

1. `fmod_codec_opus.cpp` を Ogg family codec へ拡張した。
   - ファイル名は当面そのままでもよいが、内部コメントと登録名は Ogg family に寄せる。
   - 既存の Opus decode 実装は残す。
   - first packet 判定で `OpusHead` と `\x01vorbis` を分岐する。
   - Vorbis と判定した後に FMOD built-in Vorbis へ任せるのではなく、この codec 自身が Vorbis を最後まで decode する。

2. Vorbis decode state を追加した。
   - `vorbis_info`
   - `vorbis_comment`
   - `vorbis_dsp_state`
   - `vorbis_block`
   - header packet count
   - pending PCM buffer
   - sample rate / channel count
   - initialized flag 群。`destroy_state()` で必ず clear できるようにする。

3. Ogg first packet handling を共通化した。
   - Ogg capture pattern `OggS` でないものは 4 bytes だけ読んで reject する。
   - Ogg stream は first BOS packet まで読む。
   - non-Ogg MP3/AAC/HTTP stream を壊さない。
   - 初期実装の scope は simple Icecast Ogg Opus/Vorbis とする。
   - Ogg Skeleton や複数 logical stream の BOS scan は後続課題にする。現実装では chained Ogg / serial change を明示的に再初期化していないため、simple Icecast Ogg Opus/Vorbis を初期 scope とする。

4. Opus path は既存実装を維持した。
   - family 0 mono/stereo
   - family 1 multistream
   - output 48 kHz float PCM
   - `setposition(0)` 以外は受けない

5. Vorbis path を追加した。
   - first packet が `\x01vorbis` なら Vorbis header として受ける。
   - 続く comment/setup header を同じ Ogg stream から読む。
   - `vorbis_synthesis_headerin()` / `vorbis_synthesis_init()` / `vorbis_block_init()` で decode を開始する。
   - audio packet は `vorbis_synthesis()` / `vorbis_synthesis_blockin()` / `vorbis_synthesis_pcmout()` で float PCM にする。
   - FMOD へは interleaved PCMFLOAT として返す。
   - sample rate は Vorbis stream の `vi->rate` を使う。
   - channel count は `vi->channels` を使う。現実装では `channels < 1` のみ reject し、2ch 限定にはしていない。
   - 確認済みの `s9.voscast.com` は 2ch source である。6ch source の Vorbis channel layout 検証は別項目として残す。
   - Vorbis setup header は大きくなる可能性があるため、既存の Opus probe cap `65536` bytes をそのまま全 Vorbis header の hard cap にしない。r25 実装では Vorbis header 読み込み用に別上限を置き、超過時は明示ログを出す。
   - chained Ogg / serial change は初期実装では unsupported とする。現実装では serial change の再初期化までは行わないため、後続で扱う。

6. `LLAudioEngine_FMODSTUDIO::init()` の登録は 1 本にした。
   - `FMODGetCodecDescriptionOpus()` の名前は残してもよいが、実体は Ogg family codec。
   - 可能なら後で `FMODGetCodecDescriptionOggFamily()` に改名する。
   - priority は引き続き `0`。
   - `AYAOpusCodecEnable` は診断用として残すが、通常値は `true`。

7. parcel music 経路は原則そのままにした。
   - `LLStreamingAudio_FMODSTUDIO`
   - `FMOD::System::createStream(url, FMOD_2D | FMOD_NONBLOCKING | FMOD_IGNORETAGS, ...)`
   - Ogg family codec 側で Vorbis/Opus を正しく吸収する。
   - `Stream starvation detected` の pause/resume は別問題として扱う。

8. 3D Stream 経路も同じ恩恵を受ける。
   - 3D Stream も `createStream(url)` を使う箇所がある。
   - Ogg family codec が Vorbis を開けるため、`s9.voscast.com` のような suffix 不一致 Ogg Vorbis も改善対象になった。
   - FMOD plugin codec が `FMOD_SOUND_TYPE_UNKNOWN` として見える場合に備え、`Ogg Opus` / `Ogg Vorbis` を既知 type として扱う promotion を追加した。

ビルド面:

```text
追加 CMake/link は原則不要。
llaudio はすでに ll::vorbis に link 済み。
ll::vorbis は libogg / libvorbis / libvorbisfile / libvorbisenc を含む。
Vorbis は静的 archive なので通常の autobuild packaging 追加は不要。
macOS arm64 Release の ayastorm-bin build は成功。
```

既知の別リスク:

```text
macOS universal build では libopus.dylib が arm64-only だと x86_64 link が失敗する。
これは Vorbis 追加の問題ではなく、Opus support を維持するための既存 packaging 問題。
```

実装前のサブエージェント設計レビューでの結論:

```text
大筋:
  priority 0 の Opus codec を Ogg family codec に拡張する方針は妥当。

必須補正:
  Vorbis branch は判定だけでなく codec read callback で完全 decode する。
  simple Icecast Ogg Opus/Vorbis を初期 scope として明記する。
  Ogg Skeleton / multiplex / chained stream は初期 scope から外すか、明示的に扱う。
  3D Stream 6ch / multi-speaker 経路では type/layout promotion が必要になる。
  r25 実装では Ogg Opus / Ogg Vorbis の promotion を追加済み。
```

検証項目:

```text
Opus:
  既存 Ogg Opus stream / file が再生できること
  OpusHead path が従来どおり 48 kHz PCMFLOAT を返すこと

Vorbis parcel music:
  http://www.fro.at:8008/fro-128.ogg
  https://azuracast.tilderadio.org/radio/8000/radio.ogg

Vorbis suffix mismatch:
  http://s9.voscast.com:8032/stream.mp3

Opus 5.1ch surround:
  http://go-stream-live.com:8030/stream

3D Stream:
  https://azuracast.tilderadio.org/radio/8000/radio.ogg
  http://s9.voscast.com:8032/stream.mp3
  http://go-stream-live.com:8030/stream
```

成功ログ:

```text
Parcel music:
  Starting internet stream
  FMOD::Sound::getOpenState Error なし
  Internet stream openstate error なし

3D Stream:
  Multi source ready
  Multi decode thread started
  Multi path playing
  2ch source -> 6 speaker routing maintained
  Opus 5.1ch surround source playback succeeded
```

追加成功ログ:

```text
FmodOgg:
  Vorbis stream opened: 2ch 44100Hz PCMFLOAT

3D Stream:
  Multi source ready: http://s9.voscast.com:8032/stream.mp3 44100 Hz x 2 ch, fmt=PCMFLOAT
  Multi decode thread started for http://s9.voscast.com:8032/stream.mp3
  Multi path playing: http://s9.voscast.com:8032/stream.mp3
```

失敗ログ:

```text
FMOD::Sound::getOpenState Error
Internet stream openstate error
Multi source open failed
```

この実装なら、Opus support を削らず、Vorbis live stream も FMOD fallback に依存せずに開ける。

## 以前の Opus codec 修正について

以前の修正は、AYAstorm で Ogg Opus を FMOD 経由で再生できるようにする、という目的では意味があった。

問題は Opus 対応そのものではない。

危険だったのは、次の fallback model である。

```text
AYAstorm Opus codec を高 priority で FMOD に登録する
-> Ogg Opus なら AYAstorm codec が開く
-> Ogg Vorbis なら AYAstorm codec が失敗する
-> FMOD 組み込み Vorbis codec に fallback する
```

seek できる file なら、この考え方は成立しやすい。

しかし Icecast live stream のような non-seekable HTTP stream では、先に probe した codec が bytes を消費すると、次の codec が先頭から読み直せない可能性がある。

したがって、Ogg Opus と Ogg Vorbis の切り分けを FMOD の codec fallback 競争に任せる設計は避けるべきである。

## 設計メモ

Ogg 系 stream は、URL 拡張子ではなく、実体を軽く probe してから経路を決めるべきである。

判定順:

```text
Parcel music / direct stream URL
-> HTTP header / first bytes を probe
-> 先頭が OggS なら Ogg container として扱う
-> BOS packet を見る
     OpusHead なら Ogg Opus
     \x01vorbis なら Ogg Vorbis
-> それ以外は従来の FMOD stream 判定へ
```

重要な原則:

```text
URL suffix で判断しない。
```

`http://s9.voscast.com:8032/stream.mp3` が具体的な反例である。

## 長期設計

堅い設計は、Ogg family stream を Viewer 側で decode し、PCM として FMOD に渡すことである。

目標経路:

```text
Ogg Opus / Ogg Vorbis live stream
-> Viewer 側 Ogg demux
-> Opus decoder / Vorbis decoder
-> PCM float
-> FMOD user stream
-> FMOD 2D channel または 3D Stream routing
```

この設計で維持できるもの:

- AYAstorm Ogg Opus support
- Ogg Vorbis live stream support
- 将来の 3D Stream routing
- Viewer 側 volume control

FMOD には codec fallback 競争ではなく、output / mixing を担当させる。

## FMOD だけで済ませる場合の現実的な線

「FMOD だけで済ませる」という方針は、次の 2 種類に分けて考える。

```text
案 A:
FMOD built-in internet stream
つまり createStream(url) 直渡しだけで済ませる

案 B:
FMOD mixer / output は使い続ける
ただし Ogg family は AYAstorm 側 custom codec または OPENUSER で PCM を渡す
```

案 A は変更範囲が最小だが、今回の問題に対しては弱い。

理由:

- non-seekable HTTP live stream は codec probe 後に先頭へ戻せない。
- FMOD built-in codec と AYAstorm Opus codec の fallback 順に依存する。
- URL suffix と実体形式が食い違う stream を安全に分類できない。
- Ogg Vorbis live stream が `FMOD_ERR_FILE_COULDNOTSEEK` で落ちる実例がある。
- Opus support を維持するには AYAstorm codec を高 priority 登録する必要があり、Ogg Vorbis との競合が残る。

したがって、`createStream(url)` 直渡しだけで Ogg Opus と Ogg Vorbis live stream の両方を安定させるのは期待しない。

一方、案 B は FFmpeg / GStreamer を追加しなくても実現できる。

現実的な FMOD-centered 構成:

```text
Parcel music URL
-> AYAstorm stream probe
-> Ogg Opus なら AYAstorm FMOD Opus codec
-> Ogg Vorbis なら AYAstorm FMOD Vorbis codec または OPENUSER PCM
-> MP3 / plain stream は従来の FMOD createStream(url)
-> FMOD mixer / stream channel / volume control
```

この構成では、FMOD は引き続き最終的な再生エンジンである。

ただし、Ogg family の format 判定と decode は FMOD built-in の URL codec fallback に任せない。

外部依存を増やしたくない場合の中期案:

1. `libogg` + `libvorbis` + 既存 `libopus` を使う。
2. AYAstorm custom FMOD codec を Ogg Opus / Ogg Vorbis 用に分ける。
3. first packet の `OpusHead` / `\x01vorbis` を AYAstorm 側で判定する。
4. codec fallback でなく、判定済みの経路へ明示的に流す。
5. 出力は PCMFLOAT とし、FMOD 側で 2D parcel music channel に接続する。

この方針なら「FMOD だけ」と言える範囲を保ちながら、FFmpeg / GStreamer の配布・ライセンス・3OS packaging 問題を避けられる。

ただし実装量は `createStream(url)` の mode 調整より大きい。

最小検証としては、まず現在の `AYAOpusCodecEnable=false` で Ogg Vorbis stream の挙動を確認する。

確認結果の解釈:

- 無効化で再生できる: AYAstorm Opus codec の probe 横取りが主因。
- 無効化しても再生できない: FMOD built-in の Ogg Vorbis live stream 処理自体が対象 stream と相性悪い。

後者の場合、FMOD built-in だけでの解決はさらに難しい。

## 6ch / multichannel

6ch は活かせる設計にするべきである。

そのためには、decode 結果を単なる PCM bytes として扱うだけでは不十分である。

decode 層は次の metadata を保持する。

```text
sample_rate
channel_count
channel_layout
pcm_format
```

例:

```text
48000 Hz
6 channels
FL / FR / C / LFE / SL / SR
float PCM
```

Ogg Opus mapping family 1 は surround layout を表現できる。現在の AYAstorm Opus codec には `opus_multistream_decoder` の実装知見がある。

この知見は使うべきだが、最終経路では channel layout を明示的に保持する必要がある。

必要な挙動:

- Ogg Opus は mapping family と channel mapping を読む。
- Ogg Vorbis は Vorbis-compatible channel order を Viewer 内部 layout へ正規化する。
- 2D parcel music では必要に応じて downmix する。
- 3D Stream では `FL / FR / C / LFE / SL / SR` を speaker routing へ渡す。

6ch を活かすなら、FMOD に Ogg を推測させるより、Viewer 側で demux/decode して channel layout を保持するほうが安全である。

## 採用しなかった短期対応案

full Viewer-side Ogg demux/decode 層が大きすぎる場合、短期 bridge はあり得る。

短期案:

1. `FMOD::System::createStream()` 前に URL を probe する。
2. `OggS` を検出する。
3. first packet で `OpusHead` または `vorbis` を判定する。
4. Ogg Vorbis と分かったものは AYAstorm Opus codec probe に触らせない。
5. Ogg Opus は AYAstorm Opus support を維持する。

これは理想形ではないが、Opus を維持しながら Ogg Vorbis live stream の破損を減らせる。

r25 ではこの案は採用しなかった。parcel music / 3D Stream の `createStream(url)` 経路は維持し、priority 0 の同一 custom FMOD codec 内で Ogg first packet を読んで Opus / Vorbis を分岐する方針を採用した。

## やってはいけないこと

Opus support を削除して解決しない。

`AYAOpusCodecEnable=false` を製品仕様にしない。これは診断用である。

`.mp3` なら MP3、`.ogg` なら Vorbis、という判断をしない。実際の HTTP header と first bytes を正とする。
