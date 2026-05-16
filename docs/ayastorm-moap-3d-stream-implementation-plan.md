# AYAstorm MOAP audio to 3D Stream implementation plan

## 目的

MOAP/CEF 再生音声を、現行の 2D FMOD media audio path ではなく、3D Stream の speaker routing / positional audio path に接続する。

このブランチの目的は「MOAP の音を単に FMOD へ流す」ことではない。既存の MOAP audio callback が生成している PCM を 3D Stream の入力源として扱い、3D Stream 側の speaker prim、channel routing、HRTF、venue reverb、occlusion、volume control を再利用できる形にする。

## 目次

- [結論](#結論)
- [実装方針](#実装方針)
- [現行仕様: MOAP audio](#現行仕様-moap-audio)
- [現行仕様: 3D Stream](#現行仕様-3d-stream)
- [接続設計](#接続設計)
- [実装フェーズ](#実装フェーズ)
- [先に解決すべき問題](#先に解決すべき問題)
- [検証計画](#検証計画)
- [対象外](#対象外)

## 結論

推奨実装は、`LLPositionalStreamMulti` に URL source と並ぶ PCM ring source を追加する方式である。

現在の 3D Stream は URL を `FMOD::System::createStream()` で開き、decode thread が `FMOD::Sound::readData()` で PCM を取得し、その PCM を `LLMultiTailRing` に投入している。その下流では、speaker prim ごとに `FMOD_OPENUSER | FMOD_3D` の mono sound を作り、既存の routing matrix で L/R/M/FL/FR/C/LFE/SL/SR へ分配している。

MOAP audio はすでに `media_plugin_cef` から shared memory ring へ float PCM として届いている。したがって、MOAP を 3D Stream に接続する場合は、FMOD に URL decode させる上流部分だけを差し替え、MOAP の `LLPluginAudioRingHeader` から読んだ PCM を 3D Stream の ring に投入するのが最も自然である。

`LLMediaAudioStream` の FMOD channel を 3D 化する案は採用しない。単一位置で鳴らすだけなら短いが、3D Stream の speaker prim 分配、5.1 routing、HRTF、venue reverb、routing diagnostic を通らないため、このブランチの目的に合わない。

## 実装方針

1. 3D Stream の下流を再利用する。
   - `LLPositionalStreamMulti::createUserSounds()`
   - `LLPositionalStreamMulti::pcmReadCallback()`
   - speaker routing / upmix / BS.775 downmix
   - Stream3D ChannelGroup
   - Lite HRTF / venue reverb / occlusion

2. 3D Stream の source 部分を分岐可能にする。
   - 既存: URL source -> `FMOD::createStream()` -> `FMOD::Sound::readData()`
   - 追加: MOAP PCM source -> `LLPluginAudioRingHeader` -> `LLMultiTailRing`

3. 3D 有効時は対象 MOAP の 2D `LLMediaAudioStream` を止める。
   - 現行 ring は `mReadFrame` を 1 つしか持たない single-reader 構造である。
   - 2D path と 3D path が同じ ring を同時に読むと、片方が先に `mReadFrame` を進めて音切れや無音を起こす。
   - 初期実装では「MOAP 3D 有効時は 2D media audio path を無効化する」を明示ルールにする。

4. MOAP surface と 3D Stream binding の対応は、既存 linkset 評価に寄せる。
   - 初期案は 3D Stream speaker linkset の root prim に設定された MOAP media を source として扱う。
   - YouTube などの通常 web media も、root prim の MOAP として再生されていれば対象にする。
   - speaker prim の `{ch:...}` や `{range:...}` は既存 `[3dstream-stereo:...]` の概念を流用する。

## 現行仕様: MOAP audio

`LL_DULLAHAN_AUDIO_CALLBACK=TRUE` の場合、CEF/Dullahan の audio callback が有効になる。OFF の場合は従来通り CEF native output が OS audio device へ直接出る。

現在の MOAP audio path は次の通り。

```text
CEF/Dullahan
  -> media_plugin_cef audio callbacks
  -> LLPluginAudioRingHeader shared memory ring
  -> LLViewerMediaImpl
  -> LLMediaAudioStream
  -> FMOD OPENUSER 2D stream
```

根拠となる実装:

- `media_plugin_cef.cpp`
  - `onAudioStreamStartedCallback()` が sample rate / channels / format serial を ring に記録する。
  - `onAudioStreamPacketCallback()` が `writeAudioPacketToRing()` に PCM を渡す。
  - `writeAudioPacketToRing()` は Dullahan から来る float PCM を shared memory ring に書く。
- `llpluginaudio.h`
  - `LLPluginAudioRingHeader` は `mWriteFrame` と `mReadFrame` を持つ。
  - 最大 channel 数は `LL_PLUGIN_AUDIO_RING_MAX_CHANNELS = 8`。
- `llpluginclassmedia.cpp`
  - `LLPluginClassMedia::ensureAudioSharedMemory()` が media plugin 用 shared memory を作る。
- `llviewermedia.cpp`
  - `LLViewerMediaImpl::update()` が `mMediaSource->getAudioData()` を `LLMediaAudioStream` に渡す。
- `llmediaaudiostream.cpp`
  - `LLMediaAudioStream::start()` が `FMOD_OPENUSER | FMOD_CREATESTREAM | FMOD_LOOP_NORMAL | FMOD_2D` で FMOD sound を作る。
  - `LLMediaAudioStream::readPCM()` が ring から PCM を読み、underflow 時は silence を返して prebuffer に戻す。

現行 MOAP 音声は「3D positional source」ではない。object 位置は media interest / rolloff 的な判定には使われているが、音声そのものは FMOD 2D channel で再生されている。

## 現行仕様: 3D Stream

3D Stream の現行 multi path は、object Description の `[3dstream-stereo:...]` または alias の `[ayastream-stereo:...]` を `LLPositionalStreamMgr` が解析して開始する。

現在の 3D Stream multi path は次の通り。

```text
Object Description
  -> LLPositionalStreamMgr
  -> LLPositionalStreamMulti::start(url, speakers)
  -> FMOD createStream(url)
  -> decode thread / FMOD Sound::readData()
  -> LLMultiTailRing
  -> speaker-specific FMOD OPENUSER 3D mono sounds
  -> Stream3D ChannelGroup
```

主な仕様:

- source は URL 前提である。
- speaker は prim Description の `{ch:L}`, `{ch:R}`, `{ch:FL}`, `{ch:FR}`, `{ch:C}`, `{ch:LFE}`, `{ch:SL}`, `{ch:SR}` などで定義する。
- 1ch / 2ch / 6ch source を扱う。
- 6ch source は codec/layout gate を通ったものだけを扱う。
- speaker channel は dedicated `Stream3D` ChannelGroup に入る。
- `{binaural:on}` で per-speaker Lite HRTF を挿す。
- venue reverb、occlusion、per-speaker volume、Stream3D master volume が既存 path にある。

重要なのは、3D Stream の価値は URL decode ではなく、その後段の speaker routing と 3D 出力である。MOAP 接続では、この後段をそのまま使うべきである。

## 接続設計

### 推奨案: PCM source mode を追加する

`LLPositionalStreamMulti` に、URL source とは別の PCM source mode を追加する。

概念上の API は次のような形にする。

```cpp
bool LLPositionalStreamMulti::startFromPcmRing(
    LLPluginAudioRingHeader* ring,
    const std::vector<SpeakerConfig>& speakers,
    const std::string& label);
```

内部では次を分ける。

- URL source
  - `openSourceStream(url)`
  - `FMOD::Sound::readData()`
- PCM ring source
  - `openPcmRingSource(ring)`
  - `pumpPcmRingSource()`

下流は共通化する。

- source format 確定
- `LLMultiTailRing` 初期化
- `createUserSounds()`
- `startUserChannels()`
- `pcmReadCallback()`
- speaker position update
- failure / reconnect / stop lifecycle

### MOAP binding

初期実装では、3D Stream speaker linkset の root prim に MOAP media が設定されている場合、その MOAP media を 3D Stream source として扱う。

つまり、root prim に YouTube / Web ラジオ / 通常 web page の MOAP が貼られていて、その linkset が 3D Stream speaker 構成として成立していれば、MOAP から出る音声を 3D Stream の speaker prim へ流す。

source を明示する `{source:moap}` は必須にしない。root prim に URL source がなく、root prim に MOAP media がある場合は、MOAP を暗黙の source とする。

speaker 定義は既存 tag を流用する。

```text
[3dstream-stereo:{ch:FL}{range:20}{volume:1.0}]
```

URL と MOAP source の混在は避ける。root prim に `{url:...}` がある場合は従来の URL-based 3D Stream を優先する。root prim に `{url:...}` がなく、root prim に MOAP media がある場合だけ MOAP source を使う。

media の解決は段階的に行う。

1. root prim の media face を使う。
2. 必要なら `{face:N}` を追加する。
3. 将来、media texture UUID 指定を追加する。

### 2D fallback

MOAP 3D binding が成立しない場合は、現行の `LLMediaAudioStream` 2D path を維持する。

成立しない条件:

- media plugin がない。
- audio shared memory がない。
- callback build ではない。
- sample rate / channel count が未確定。
- object / face / media impl の対応が解決できない。
- 3D Stream が disabled。

この fallback がないと、MOAP 3D tag の不備や platform 差で media audio が完全に失われる。

## 実装フェーズ

### Phase 0: 前提整理

- `LL_DULLAHAN_AUDIO_CALLBACK=TRUE` build で実装する。
- 既存 2D MOAP audio と 3D Stream の両方が動く状態を基準にする。
- ring allocation の capacity と `capacity + 1` index 運用を確認する。
  - 現行 code は `mCapacityFrames` に対して reader/writer が `total = capacity + 1` を使う。
  - shared memory の sample 領域が本当に `capacity + 1` frames 分確保されているか確認し、必要なら先に修正する。

### Phase 1: PCM ring reader を分離する

`LLMediaAudioStream::readPCM()` に閉じている ring 読み出し logic を、3D Stream からも使える小さな reader に分離する。

必要な責務:

- ring header validation
- sample rate / channels / format serial の取得
- available frames 計算
- wraparound read
- underflow detection
- format serial change detection
- stats collection

この reader は初期実装では single-reader のままでよい。3D 有効時に 2D path を止める前提なら、multi-reader 化は不要である。

### Phase 2: LLPositionalStreamMulti に PCM source mode を追加する

`LLPositionalStreamMulti` に source 種別を持たせる。

```cpp
enum class SourceKind
{
    Url,
    PcmRing
};
```

URL mode では現行通り `openSourceStream()` と `pumpSource()` を使う。PCM ring mode では `FMOD::createStream(url)` を呼ばず、ring reader から `LLMultiTailRing` に float PCM を投入する。

format handling:

- sample rate は ring の `mSampleRate` を使う。
- source channels は ring の `mChannels` を使う。
- sample format は float PCM 固定。
- channel count は初期実装では 1 / 2 / 6 のみ許可する。
- 3 / 4 / 5 / 7 / 8ch は fail ではなく、まず unsupported として 2D fallback に戻すのが安全である。

### Phase 3: LLViewerMediaImpl から MOAP 3D source を公開する

`LLViewerMediaImpl` に 3D Stream manager が参照できる accessor を追加する。

候補:

```cpp
LLPluginAudioRingHeader* getAudioRingFor3DStream() const;
bool isAudioCallbackActiveFor3DStream() const;
void setAudioRoutedTo3DStream(bool enabled);
```

`setAudioRoutedTo3DStream(true)` の間は `LLMediaAudioStream::update()` を止める、または `mMediaAudioStream->stop()` する。これにより二重再生と ring 二重消費を避ける。

### Phase 4: LLPositionalStreamMgr に MOAP binding を追加する

`LLPositionalStreamMgr` の binding 評価に MOAP source を追加する。

初期仕様:

- linkset が 3D Stream speaker 構成として成立していることを前提にする。
- root prim に `{url:...}` がある場合は従来の URL-based 3D Stream を優先する。
- root prim に `{url:...}` がなく、root prim に MOAP media がある場合は、その MOAP を source とする。
- speaker 定義は既存 multi binding と同じ `{ch:...}` を使う。
- media face 未指定時は root prim の最初の media face を使う。
- media impl が解決できなければ 2D fallback に戻す。

manager は object deletion、Description change、media impl change、teleport、audio shutdown 時に `setAudioRoutedTo3DStream(false)` を必ず戻す。

### Phase 5: lifecycle と volume を詰める

対応する lifecycle:

- media URL navigation
- media reload
- sample rate / channel count / format serial change
- media plugin exit
- object delete
- speaker prim delete
- 3D Stream disabled
- Nearby Media / media mute / parcel media preference

volume rule:

```text
effective volume = media volume * Stream3DVolumeMaster * per-speaker volume
```

media mute は最優先で silence にする。Stream3D master が 0 の場合は 3D channel 側を 0 にするが、media playback 自体を止めるかは別途判断する。

### Phase 6: diagnostics

最低限、以下を `Stream3D` または `AYAMediaAudio` log に出す。

- MOAP 3D binding start / stop
- resolved media impl / object id / face
- sample rate / channels / format serial
- 2D media audio disabled / restored
- PCM underflow
- ring dropped frames
- unsupported channel count
- fallback to 2D reason

## 先に解決すべき問題

### ring が single-reader である

`LLPluginAudioRingHeader` は `mReadFrame` を 1 つしか持たない。つまり、現在の構造では 2D `LLMediaAudioStream` と 3D Stream が同じ ring を同時に読むことはできない。

初期実装では multi-reader 化しない。MOAP 3D binding が有効な media だけ 2D path を停止する。将来、同時 2D monitor や複数 3D binding が必要になった場合に tee / multi-reader ring を検討する。

### MOAP media と object/face の対応

MOAP は media texture / face index を中心に管理されている。一方、3D Stream は prim Description / linkset / speaker prim を中心に管理している。

最初から media texture UUID を自由指定にすると、同一 media を複数 object / face が共有する case が難しくなる。初期実装は「3D Stream speaker linkset の root prim 上の MOAP を、その linkset の speaker prim に接続する」に限定する。

### channel count

MOAP ring は最大 8ch まで受けられるが、3D Stream multi routing は実質 1ch / 2ch / 6ch を前提にしている。

初期実装:

- 1ch: mono として既存 routing。
- 2ch: stereo として既存 routing。
- 6ch: FL/FR/C/LFE/SL/SR として既存 routing。
- その他: 3D route せず 2D fallback。

### A/V sync

MOAP video は media texture update、audio は FMOD callback で進む。3D Stream 側に入れると、`LLMultiTailRing` と speaker OPENUSER の prebuffer が追加される。厳密な lip sync は初期目標にしないが、体感で破綻しない範囲に抑える必要がある。

### platform 差

MOAP audio callback は build option 依存である。macOS では callback ON 時に CEF native output を mute して二重再生を避けている。Windows / Linux でも同等に native output が抑止されるか、別途確認が必要である。

## 検証計画

### unit / small tests

- PCM ring reader の wraparound。
- underflow 時に silence を返すこと。
- format serial change で stream rebuild が要求されること。
- 1ch / 2ch / 6ch PCM を `LLMultiTailRing` に投入し、既存 URL source と同じ speaker callback 出力になること。
- unsupported channel count が 2D fallback になること。

### integration

- MOAP/CEF page の stereo audio が 2D ではなく speaker prim から聞こえること。
- MOAP を pause / resume / navigate / reload して音声 routing が復帰すること。
- object 移動で speaker 位置が更新されること。
- speaker prim 削除で安全に stop / fallback すること。
- media mute、media volume、Stream3D master volume、per-speaker volume が期待通り合成されること。
- 3D Stream disabled 時に 2D media audio に戻ること。

### regression

- 既存 URL-based 3D Stream が壊れていないこと。
- 既存 Ogg Opus / Ogg Vorbis 3D Stream が壊れていないこと。
- MOAP 3D tag がない通常 MOAP は従来通り 2D 再生されること。
- `LL_DULLAHAN_AUDIO_CALLBACK=FALSE` build で compile が壊れないこと。

## 対象外

- libVLC / parcel media / non-CEF media の 3D Stream 接続。
- 複数 reader 対応の shared memory ring 再設計。
- MOAP audio の厳密な A/V sync。
- arbitrary 8ch channel layout の routing。
- URL-based 3D Stream の parser 全面刷新。
