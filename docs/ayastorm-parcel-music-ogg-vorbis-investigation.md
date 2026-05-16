# AYAstorm 土地音楽: Ogg Vorbis live stream 調査メモ

## 現在の位置づけ

この文書は、土地音楽 / FMOD stream 経路の調査用である。

MOAP / CEF / Dullahan の FMOD 2D audio PR とは別系統として扱う。

対象経路:

```text
Parcel music URL
-> LLStreamingAudio_FMODSTUDIO
-> FMOD::System::createStream(url)
-> FMOD internet stream channel
```

AYAstorm では Opus 再生を捨ててはいけない。最終修正は、Opus 再生を維持したまま Ogg Vorbis live stream も再生できる設計にする必要がある。

## 要約

最初に問題になった土地音楽 URL:

```text
http://krominancia.org:7110/live
```

これは CEF / MOAP ではなく、土地音楽として既存の FMOD streaming audio 経路で処理される。

外部 probe では、URL 自体は有効で、次の形式として確認できた。

```text
Icecast 2.4.2
Content-Type: application/ogg
Ogg Vorbis audio
44100 Hz stereo
約 192 kbps
```

## 症状

AYAstorm は土地音楽 URL を受け取り、FMOD stream の開始までは進む。

しかしすぐに FMOD 側で失敗し、再生が止まる。

代表的なログ:

```text
Starting internet stream: http://krominancia.org:7110/live
FMOD::Sound::getOpenState Error: Couldn't perform seek operation.
Internet stream openstate error: open_state = 2 - progress = 0 - starving = 0 - diskbusy = 210
Stopping internet stream: http://krominancia.org:7110/live
```

FMOD header 上、`open_state = 2` は次に相当する。

```text
FMOD_OPENSTATE_ERROR
```

エラー文字列は次に相当する。

```text
FMOD_ERR_FILE_COULDNOTSEEK
```

## AYAstorm 固有問題の可能性

Firestorm 本家では再生できる可能性がある。

AYAstorm では Ogg Opus 再生のために、独自の FMOD codec plugin を登録している。

```cpp
mSystem->registerCodec(FMODGetCodecDescriptionOpus(), &opus_codec_handle, 0);
```

priority `0` は FMOD codec priority として高い。これは AYAstorm の Opus support に必要だった。FMOD 組み込みの Ogg/Vorbis 側が Ogg Opus を先に拾って失敗するのを避けるためである。

ただし、Ogg Vorbis live stream に対してこの設計は危険である。

想定される失敗経路:

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

## これは何ではないか

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
3. `http://krominancia.org:7110/live` を土地音楽として設定した区画に入る。
4. 土地音楽が開始されるか確認する。

もし AYAstorm Opus codec を登録しない状態で Ogg Vorbis 土地音楽が再生できるなら、独自 Opus codec の probe が Ogg Vorbis live stream を壊している可能性が高い。

ただし、この switch は最終製品仕様ではない。Opus を無効化したままにしてはいけない。

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

AYAstorm/FMOD では再生できない。

該当ログ:

```text
Starting internet stream: http://s9.voscast.com:8032/stream.mp3
FMOD::Sound::getOpenState Error: Couldn't perform seek operation.
Internet stream openstate error: open_state = 2 - progress = 0 - starving = 0 - diskbusy = 70
Stopping internet stream: http://s9.voscast.com:8032/stream.mp3
```

この検証時点では、AYAstorm Opus codec registration は一時的に無効化されていた。

そのため、この URL の失敗だけでは「Opus codec が Vorbis を横取りした」とは断定できない。

一方で、FMOD の通常 stream open 経路が、non-seekable Ogg Vorbis live stream、特に URL suffix と実体形式が食い違う stream を開けないケースがあることは確認できた。

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

## あるべき設計

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

FMOD には codec 判定ではなく、output / mixing を担当させる。

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

## 短期対応案

full Viewer-side Ogg demux/decode 層が大きすぎる場合、短期 bridge はあり得る。

短期案:

1. `FMOD::System::createStream()` 前に URL を probe する。
2. `OggS` を検出する。
3. first packet で `OpusHead` または `vorbis` を判定する。
4. Ogg Vorbis と分かったものは AYAstorm Opus codec probe に触らせない。
5. Ogg Opus は AYAstorm Opus support を維持する。

これは理想形ではないが、Opus を維持しながら Ogg Vorbis live stream の破損を減らせる。

## やってはいけないこと

Opus support を削除して解決しない。

`AYAOpusCodecEnable=false` を製品仕様にしない。これは診断用である。

`.mp3` なら MP3、`.ogg` なら Vorbis、という判断をしない。実際の HTTP header と first bytes を正とする。
