# AYAstorm MOAP audio to 3D Stream implementation plan

## 目的

MOAP/CEF 再生音声を、現行の 2D FMOD media audio path ではなく、3D Stream の speaker routing / positional audio path に接続する。

このブランチの目的は「MOAP の音を単に FMOD へ流す」ことではない。既存の MOAP audio callback が生成している PCM を 3D Stream の入力源として扱い、3D Stream 側の speaker prim、channel routing、HRTF、venue reverb、occlusion、volume control を再利用できる形にする。

## 目次

- [結論](#結論)
- [実装状況](#実装状況)
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

ただし、実装は単に parser に `{source:media}` を追加するだけでは不十分である。現行 3D Stream は root source を `{url}` 文字列として扱っており、binding fingerprint、reconnect、diagnostic、format-failed cache、toast 表示まで URL 依存になっている。Media source 対応では、source 種別と source identity を binding 全体に通す必要がある。

## 実装状況

2026-05-17 時点で Phase 0 の一部を実装済み。

- media audio ring の shared memory サイズ計算を helper 化した。
- ring の sentinel frame (`capacity + 1`) 分を確保するよう修正した。
- 3D media source の対応 channel count を `1 / 2 / 6 / 8` として helper 化した。
- media audio の channel order を定義した。
  - 6ch: `FL / FR / C / LFE / SL / SR`
  - 8ch: `FL / FR / C / LFE / SL / SR / BL / BR`
- `llpluginaudio` integration test を追加した。
- 3D Stream tag parser に `{source:media}`、`{link:N}`、`{face:N}` を追加した。
- `{url:...}` と `{source:media}` の同時指定を invalid binding として扱うようにした。
- `{source:media}` の media face を linkset 内から解決し、media audio ring を `LLPositionalStreamMulti` の source として開始できる経路を追加した。
  - tag は root prim の Description に置く。
  - media face は root prim または child prim のどちらにあってもよい。
  - media face が複数ある場合は `{link:N}{face:M}` で対象 prim / face を指定できる。
- media source 有効時は対象 media の通常 2D `LLMediaAudioStream` を停止する suppression を追加した。
- media ring source は `1 / 2 / 6 / 8ch` を受け入れる。
  - 今回の実機確認対象は 5.1ch 再生までとする。
  - 8ch / 7.1ch は将来の speaker 構成拡張に備えた受け口として実装し、今回の合格条件には含めない。
- 同一 media source を複数の 3D Stream binding が同時に読む構成は拒否するようにした。
- media source binding は、media audio ring が存在する限り `sample_rate/channels` が未確定でも維持するようにした。
  - MOAP 読み込み直後、動画切替中、音声なしページでは 3D route を無音待機にする。
  - 音声 format が再度確定したら、同じ 3D route 内で media ring source を再オープンする。
  - 2D `LLMediaAudioStream` suppression は維持し、ブラウザ操作中に 2D と 3D の出力経路が行き来しないようにする。
- media source 専用の低遅延 buffering を追加した。
  - start 時に media ring に残っている古い PCM を破棄し、映像 clock に近い位置から読む。
  - URL stream 用の大きい jitter buffer は維持しつつ、media source は 3D Stream 側の buffered frames を浅く保つ。
  - 初回実機確認で浅すぎる buffer によるざらつきが出たため、media source は prebuffer 2048 frames、target 4096 frames、OPENUSER decode buffer 2048 frames に調整した。
- media source route の volume を media face 数に応じて制御するようにした。
  - linkset 内の media face が 1 つだけなら、media volume / mute は 2D media path と同じ意味の source gain として扱う。
  - linkset 内の media face が複数ある場合、3D Stream に流し込む media は source gain 1.0 として扱い、音量は `Stream3DVolumeMaster` と speaker volume で調整する。
  - 3D Stream に選ばれていない他の media は従来通り media volume で調整できる。
  - URL source の volume path は従来通り `Stream3DVolumeMaster` のみを使う。
- CEF audio callback の format を viewer log 側で確実に確認するため、`media_plugin_cef` から `LLPluginClassMedia` へ `audio_stream_format` message を送る diagnostic を追加した。
  - `started`: `CEF audio stream started: <sample_rate> Hz x <channels> ch (ring max <max_channels> ch)`
  - `stopped`: sample rate / channels / frame count
  - `error`: CEF audio callback error message
  - これにより、plugin process 内の `LL_INFOS("AYAMediaAudio")` がログに出ない場合でも、viewer 側 `AYAstorm.log` で Dullahan callback の実フォーマットを確認できる。
- Dullahan audio callback package を `v1.26.0-CEF_139.0.40-ayastorm-audio-callback.4` として用意した。
  - `GetAudioParameters()` で `CEF_CHANNEL_LAYOUT_7_1` / `48000 Hz` / `1024 frames` を要求する。
  - Windows / Linux は GitHub Actions で package build 済み。
  - macOS はローカルで universal package を作成し、同じ GitHub Release に upload 済み。
  - viewer 側 `autobuild.xml` の `dullahan_aya_audio` は `.4` release を参照する。

まだ未実装:

- 7.1ch speaker 構成での実機確認。

PR 前に必要な実機確認:

2026-05-17 の macOS 実機確認で、今回の PR 前必須項目はすべて合格とする。

1. root prim に tag、child prim に media を置いた構成で再生できること。確認済み。
   - root prim Description: `[3dstream-stereo:{source:media}...]`
   - media face: child prim
   - 結果: child prim media source から speaker prim へ再生できた。
2. media ON/OFF、Nearby Media stop/start、media reload 後に crash せず 3D route が復帰すること。確認済み。
   - 以前の crash は stale shared memory ring pointer が原因だったため、修正後の再実機確認が必要。
   - 結果: media ON/OFF、Nearby Media stop/start、media reload 後も crash せず 3D route が復帰した。
3. 5.1ch source が 6 speaker 構成で破綻なく鳴ること。確認済み。
   - 期待ログ: `CEF audio stream started: 48000 Hz x 8 ch`
   - 期待ログ: `layout=FL/FR/C/LFE/SL/SR/BL/BR ... speakers=6`
   - 結果: 6 speaker 構成で破綻なく鳴っている。今回の合格条件は 5.1ch 再生まで。7.1ch の BL/BR 実音確認は将来項目。
4. 44.1kHz source が問題なく鳴ること。確認済み。
   - 結果: 44.1kHz source でも問題なく鳴っている。
5. media face 上でページ遷移、動画切替、音声なしページへの遷移を行っても、2D audio と 3D audio を行き来しないこと。確認済み。
   - 結果: media source 3D binding 中に 2D audio と 3D audio を行き来する状態にはなっていない。
6. URL source と media 表示を併用する構成が壊れていないこと。確認済み。
   - root tag が `{url:...}` の場合、linkset 内に media があっても speaker は URL stream を鳴らす。
   - 結果: URL source と media 表示の併用構成は壊れていない。
7. A/V sync が体感上破綻しないこと。確認済み。
   - 実機確認では、映像と音声のずれは許容範囲内。

残確認:

- Windows / Linux で native CEF output と FMOD output の二重再生が起きないこと。
- Dullahan audio callback が使えない build で `{source:media}` が指定された場合、crash せず通常の MOAP 2D audio として再生されること。

将来確認:

- BL/BR speaker prim を含む 7.1ch 構成での実音確認。
- 厳密な A/V sync の改善。今回の優先順位は 5.1ch 3D Stream 再生の安定性であり、lip sync の完全一致は対象外。

確認済み:

- `git diff --check`
- `llpluginaudio_test.cpp` の直接コンパイル
- `llaudio` arm64 build
  - `xcodebuild -project build-darwin-universal/Firestorm.xcodeproj -configuration Release -target llaudio ARCHS=arm64 ONLY_ACTIVE_ARCH=YES build`
- macOS arm64 検証ビルド
  - `xcodebuild -project build-darwin-universal/Firestorm.xcodeproj -configuration Release -target viewer ARCHS=arm64 ONLY_ACTIVE_ARCH=YES build`
  - `LL_DULLAHAN_AUDIO_CALLBACK=TRUE`
  - `build-darwin-universal/newview/Release/AYAstorm.app` の staging / codesign まで成功
- CEF audio format diagnostic 追加後の macOS arm64 検証ビルド
  - `xcodebuild -project build-darwin-universal/Firestorm.xcodeproj -configuration Release -target llplugin -target media_plugin_cef -target viewer -arch arm64 build`
  - `build-darwin-universal/newview/Release/AYAstorm.app` へ `media_plugin_cef.dylib` 再コピー / codesign まで成功
- CEF audio format diagnostic 追加後の実機ログ確認
  - `https://non-rem.com/vj/vj_051726_A.html?stream=https://mp3-proxy.onrender.com/http%3A%2F%2Fgo%2Dstream%2Dlive%2Ecom%3A8030%2Fstream`
  - `CEF audio stream started: 44100 Hz x 2 ch (ring max 8 ch)`
  - `Media multi source ready: ... 44100 Hz x 2 ch, fmt=PCMFLOAT, ring cap 16384 frames × 2 tracks, speakers=6`
  - 入力 stream 自体は 6ch/48kHz と確認済みだが、CEF/Dullahan callback へ届く時点では 2ch/44.1kHz になっている。
- Dullahan 側 parameter override の検証 build
  - `dullahan_browser_client::GetAudioParameters()` で `CEF_CHANNEL_LAYOUT_7_1`、`sample_rate = 48000`、`frames_per_buffer = 1024` を指定。
  - Dullahan arm64 `dullahan` target build 成功。
  - viewer 側 prebuilt `libdullahan.a` の arm64 slice を更新。
  - `llcommon` / `llaudio` / `llplugin` / `media_plugin_cef` / `viewer` arm64 build 成功。
- Dullahan override 後の実機ログ確認
  - `https://non-rem.com/vj/vj_051726_A.html`
  - media binding: `[3dstream-stereo:{source:media}{range:30}{volume:1.0}{bin:off}{v:hm}{wg:0.1}{upmix:off}]`
  - speaker prim: `FL / FR / C / LFE / SL / SR` の 6 speaker 構成。
  - `CEF audio stream started: 48000 Hz x 8 ch (ring max 8 ch)`
  - `Media multi source ready: ... 48000 Hz x 8 ch, fmt=PCMFLOAT, layout=FL/FR/C/LFE/SL/SR/BL/BR, ring cap 16384 frames × 8 tracks, speakers=6`
  - Dullahan override により、以前の `44100 Hz x 2 ch` から `48000 Hz x 8 ch` callback へ変化した。
  - 現在の検証 linkset は 6 speaker 構成のため、今回確認済みとするのは 5.1ch 再生まで。BL/BR 専用 speaker を含む 7.1ch 再生は将来確認項目に残す。
- Dullahan package build / release
  - tag: `v1.26.0-CEF_139.0.40-ayastorm-audio-callback.4`
  - Dullahan commit: `455af06 Request 7.1 audio callback format`
  - GitHub Actions run: `25985807911`
  - Linux asset: `dullahan-1.26.0.202605170826_139.0.40_g465474a_chromium-139.0.7258.139-linux64-25985807911.tar.zst`
  - Linux sha1: `17ba90bd9af76c15e97f69c9ea629987cdcedcf6`
  - Windows asset: `dullahan-1.26.0.202605170827_139.0.40_g465474a_chromium-139.0.7258.139-windows64-25985807911.tar.zst`
  - Windows sha1: `0e8cd6fba9c2d184743e95e36ad08e40bc686efd`
  - macOS asset: `dullahan-1.26.0.202605171727_139.0.40_g465474a_chromium-139.0.7258.139-darwin64-261370827.tar.zst`
  - macOS sha1: `2a043c69549411e934ecccf31f789528268ba46c`
  - viewer `autobuild.xml` の `dullahan_aya_audio` を上記 `.4` package に更新。
- media ON/OFF 時クラッシュの調査と修正
  - 実機クラッシュレポート: `AYAstorm-2026-05-17-155411.ips`
  - 例外: `EXC_BAD_ACCESS / SIGSEGV`
  - crash frame: `LLPositionalStreamMulti::validateMediaRingHeader()` → `openMediaRingSource()` → `LLPositionalStreamMulti::update()`
  - 直前ログでは `Media multi source format pending/changed ... holding 3D route open` の後、`SLPlugin` が終了・再起動していた。
  - 原因は、3D Stream 側が media plugin の古い shared memory ring pointer を保持したまま、plugin 終了後に unmapped pointer を dereference したことと判断。
  - 対策として、manager update ごとに現在の `LLViewerMediaImpl` から ring pointer を再取得し、`LLPositionalStreamMulti` へ差し替えるようにした。
  - `LLPluginClassMedia::getAudioData()` は plugin が running の場合だけ audio shared memory address を返すようにした。
  - ring が一時的に `nullptr` の場合は 3D route を Failed にせず、Opening のまま無音待機する。
  - 修正後、`llaudio` / `llplugin` / `viewer` arm64 build 成功。
- 44.1kHz source の実機再生確認
  - Dullahan 側では 48kHz callback を要求している。
  - 44.1kHz source でも問題なく MOAP → 3D Stream 経路で再生できることを確認。
  - 現時点では、Chromium/CEF 側 resample による実用上の問題は確認されていない。
- PR 前必須項目の実機確認
  - root prim tag + child prim media source で再生できることを確認。
  - media ON/OFF、Nearby Media stop/start、media reload 後も crash せず 3D route が復帰することを確認。
  - 5.1ch source が 6 speaker 構成で破綻なく鳴ることを確認。
  - media face 上でページ遷移、動画切替、音声なしページへの遷移を行っても、2D audio と 3D audio を行き来しないことを確認。
  - URL source と media 表示の併用構成が壊れていないことを確認。

未実行:

- `INTEGRATION_TEST_llpluginaudio` の CMake target 実行
- 7.1ch speaker 構成での BL/BR 出力確認
- universal macOS build

未実行理由:

- 現在の `build-darwin-universal` は `LL_TESTS=FALSE`。
- universal build の x86_64 link には decoder symbol を持つ x86_64/universal `libopus` が必要である。
- ローカルにある Homebrew `libopus` と既存 AYAstorm app 同梱 `libopus.dylib` は arm64 のみ。
- 3p-fmodstudio / FSBank 由来の universal `libopus.dylib` は encoder symbol 中心で、`opus_decode_float` / `opus_decoder_create` / `opus_multistream_decoder_create` を export していないため、この branch の Opus codec link には使えない。

## 実装方針

1. 3D Stream の下流を再利用する。
   - `LLPositionalStreamMulti::createUserSounds()`
   - `LLPositionalStreamMulti::pcmReadCallback()`
   - speaker routing / upmix / BS.775 downmix
   - Stream3D ChannelGroup
   - Lite HRTF / venue reverb / occlusion

2. 3D Stream の source 部分を分岐可能にする。
   - 既存: URL source -> `FMOD::createStream()` -> `FMOD::Sound::readData()`
   - 追加: Media PCM source -> `LLPluginAudioRingHeader` -> `LLMultiTailRing`

3. 3D 有効時は対象 MOAP の 2D `LLMediaAudioStream` を止める。
   - 現行 ring は `mReadFrame` を 1 つしか持たない single-reader 構造である。
   - 2D path と 3D path が同じ ring を同時に読むと、片方が先に `mReadFrame` を進めて音切れや無音を起こす。
   - 初期実装では「MOAP 3D 有効時は 2D media audio path を無効化する」を明示ルールにする。

4. Media surface と 3D Stream binding の対応は、既存 linkset 評価に寄せる。
   - 3D Stream speaker linkset の root prim に `{source:media}` がある場合だけ、linkset 内の media を source として扱う。
   - 対象 media は `{link:N}{face:M}` で明示できる。
   - YouTube などの通常 web media も、root prim または child prim の media として再生されていれば対象にする。
   - linkset 内の media を表示しつつ別の URL stream を speaker から鳴らす構成を許可するため、media source は暗黙にはしない。
   - speaker prim の `{ch:...}` や `{range:...}` は既存 `[3dstream-stereo:...]` の概念を流用する。

5. Source identity を URL 前提から切り離す。
   - `DistSourceKind::Url` / `DistSourceKind::Media` を導入する。
   - binding fingerprint、retry、diagnostic key、format-failed cache は URL 文字列ではなく `SourceBindingKey` を使う。
   - `{url:...}` と `{source:media}` の同時指定は invalid binding として通知する。

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

### Media binding

実装では、3D Stream speaker linkset の root prim Description に `{source:media}` がある場合、その linkset 内の media audio を 3D Stream source として扱う。

つまり、root prim または child prim に YouTube / Web ラジオ / 通常 web page の media が貼られていて、その linkset が 3D Stream speaker 構成として成立していれば、root prim の `{source:media}` 指定で media から出る音声を 3D Stream の speaker prim へ流す。

media を source にする場合は `{source:media}` を必須にする。linkset 内に media があるだけでは 3D Stream source にはしない。

これにより、次の 2 つの構成を両方扱える。

- linkset 内の media audio を 3D Stream speaker から鳴らす。
- root prim / child prim の media は画面表示用に使い、speaker からは別の URL stream を鳴らす。

speaker 定義は既存 tag を流用する。

```text
[3dstream-stereo:{source:media}{link:2}{face:0}{ch:FL}{range:20}{volume:1.0}]
[3dstream-stereo:{url:http://example.invalid/stream}{ch:FL}{range:20}{volume:1.0}]
```

URL source と media source は排他にする。`{url:...}` と `{source:media}` が同時に指定された場合は invalid binding として扱い、どちらか一方に修正させる。

media の解決は段階的に行う。

1. root prim の `{source:media}` を source declaration として扱う。
2. media face は root prim と child prim を含む linkset 内から探す。
3. linkset 内に media face が 1 つだけなら `{link:N}` / `{face:M}` は省略可にする。
4. linkset 内に media face が複数ある場合は `{link:N}{face:M}` で対象を絞り込めるようにする。
5. `{link:N}` は SL の link number と同じ意味で、root prim は 1、child prim は 2 以降、単独 prim は 0 として扱う。
6. `{face:M}` は対象 prim の media face を絞り込む。
7. `{link:N}` / `{face:M}` で絞っても複数候補が残る場合は ambiguous として通知する。
8. `{link:N}` / `{face:M}` に該当する media がない場合は invalid binding として通知する。
9. 将来、media texture UUID 指定を追加する。

現行 parser は `{url}` または `{ch}` がある場合だけ `[3dstream-stereo:...]` を認識する。`{source:media}` を source declaration として扱うには、`parseDistributedStereoTag()` の recognized 条件、`DistStereoTagData`、`DistParseError`、`evaluateLinkset()` の root source 判定を変更する必要がある。

実装上の source key は次のように分ける。

```cpp
enum class DistSourceKind
{
    Url,
    Media
};

struct SourceBindingKey
{
    DistSourceKind kind;
    std::string url;      // kind == Url
    LLUUID media_id;      // kind == Media
    S32 face = -1;        // kind == Media
};
```

`DistributedStereoBinding` の `url` 依存箇所は、この `SourceBindingKey` 相当に置き換える。特に fingerprint、reconnect、format-failed cache、routing diagnostic throttle key、now-playing 通知は URL 文字列前提のままだと media source で誤動作する。

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
- ring allocation の capacity と `capacity + 1` index 運用を先に修正する。
  - 現行 code は `mCapacityFrames` に対して reader/writer が `total = capacity + 1` を使う。
  - shared memory の sample 領域は `capacity * max_channels` 分に見えるため、index `capacity` に到達すると範囲外アクセスになり得る。
  - MOAP 3D 実装前に、sample 領域を `(capacity + 1) * max_channels` に増やすか、reader/writer の `total` 定義を `capacity` に合わせる。

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

PCM ring mode は既存の `Opening` state と `mSourceSound` 前提を使い回さない。現行 `update()` は `mSourceSound` から `getOpenState()` / `getFormat()` を読む前提なので、PCM mode では ring header の sample rate / channels / format serial を見て format を確定し、そこから `LLMultiTailRing` 初期化、`createUserSounds()`、`startUserChannels()` へ進む専用 state を持つ。

format handling:

- sample rate は ring の `mSampleRate` を使う。
- source channels は ring の `mChannels` を使う。
- sample format は float PCM 固定。
- channel count は初期実装では `1 / 2 / 6 / 8` を許可する。
- 3 / 4 / 5 / 7ch は fail ではなく、まず unsupported として 2D fallback に戻すのが安全である。
- 6ch media audio は `FL / FR / C / LFE / SL / SR` の channel order として扱い、既存 3D Stream 6ch routing に渡す。
- 8ch media audio は `FL / FR / C / LFE / SL / SR / BL / BR` の 7.1 order として受けられるようにする。ただし、今回の実機合格条件は 5.1ch 再生までである。
- 7.1ch speaker 構成は将来実装の足がかりとして扱い、7.1.4 は現行 ring / CEF callback / routing が 8ch 上限であるため対象外にする。

### Phase 3: LLViewerMediaImpl から Media 3D source を公開する

`LLViewerMediaImpl` に 3D Stream manager が参照できる accessor を追加する。

候補:

```cpp
LLPluginAudioRingHeader* getAudioRingFor3DStream() const;
bool isAudioCallbackActiveFor3DStream() const;
void setAudioRoutedTo3DStream(bool enabled);
```

`setAudioRoutedTo3DStream(true)` の間は `LLMediaAudioStream::update()` を止める、または `mMediaAudioStream->stop()` する。これにより二重再生と ring 二重消費を避ける。

この gate は `LLViewerMediaImpl::update()` 内に必要である。外部 manager が一度 `LLMediaAudioStream::stop()` しても、現行 update loop は毎 frame `setRing()` と `update()` を呼ぶため、gate なしでは次 frame に 2D 再生が復帰する。

`setAudioRoutedTo3DStream(true)` への遷移時は 2D stream を stop する。`false` へ戻す契機は、3D binding teardown、fallback、media impl destroy、plugin exit、object deletion、3D Stream disabled、audio shutdown である。

### Phase 4: LLPositionalStreamMgr に Media binding を追加する

`LLPositionalStreamMgr` の binding 評価に media source を追加する。

初期仕様:

- linkset が 3D Stream speaker 構成として成立していることを前提にする。
- root prim に `{url:...}` がある場合は従来の URL-based 3D Stream source とする。
- root prim に `{source:media}` がある場合は linkset 内の media audio を source とする。
- `{url:...}` と `{source:media}` は同時指定不可にする。
- linkset 内に media があっても `{source:media}` がなければ media audio は 3D Stream source にしない。
- speaker 定義は既存 multi binding と同じ `{ch:...}` を使う。
- linkset 内の media face が 1 つだけなら `{link:N}` / `{face:M}` は省略可にする。
- linkset 内の media face が複数ある場合は `{link:N}{face:M}` で対象を指定できるようにする。
- media impl が解決できなければ 2D fallback に戻す。
- media data update / media impl create-destroy / face media change で root を pending evaluation に入れる。

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
single media face:
  effective volume = media user/global volume * Stream3DVolumeMaster * per-speaker volume

multiple media faces:
  selected 3D media effective volume = Stream3DVolumeMaster * per-speaker volume
  non-selected media volume = existing media volume path
```

media face が 1 つだけの場合、media mute は最優先で silence にする。media face が複数ある場合、3D Stream に選ばれた media の viewer media volume は source gain としては使わず、他の media の 2D volume 操作と分離する。Stream3D master が 0 の場合は 3D channel 側を 0 にするが、media playback 自体は止めない。

3D routed media では、既存 2D media proximity rolloff を掛けない。3D Stream 側が speaker range / rolloff で距離減衰を行うため、media proximity rolloff も掛けると二重減衰になる。

#### media source volume 実装手順

2026-05-17 時点で実装済み。以下は実装内容と確認観点である。

現在の media UI volume は `LLViewerMediaImpl::updateVolume()` で計算され、通常は `LLPluginClassMedia::setVolume()` と 2D `LLMediaAudioStream::setVolume()` に渡される。MOAP → 3D Stream route 中は選択された media の 2D `LLMediaAudioStream` を停止し、CEF callback PCM を `LLPositionalStreamMulti` が読む。linkset 内の media face が 1 つだけなら、media UI の volume / mute を 3D Stream 側へ source gain として渡す。media face が複数ある場合は、選択 media の 3D 側 source gain は 1.0 とし、他の未選択 media の通常 2D volume 操作と干渉しないようにする。

方針:

- media face が 1 つだけなら、media volume は source gain として扱う。
- media face が複数ある場合、3D Stream に選んだ media の media volume は source gain として使わず、1.0 として扱う。
- `Stream3DVolumeMaster` は 3D Stream 全体の master / safety gain として残す。
- `{volume:N}` は speaker prim ごとの補正として残す。
- single-media 構成では、0.0〜1.0 の media gain と Stream3D master は掛け算にする。両方最大でも `1.0 * 1.0 = 1.0` なので過大化しない。
- 1.0 超えを許す per-speaker volume や `lfegain` は既存仕様のまま別段で扱う。

実装内容:

1. `LLViewerMediaImpl` に 3D Stream 用の effective media gain getter を追加した。
   - `F32 getStream3DAudioGain() const`
   - 返す値は `mRequestedVolume * LLViewerMedia::getInstance()->getVolume()` を基本にする。
   - mute 時は `mRequestedVolume` が 0 になる既存挙動を使う。
   - `sOnlyAudibleTextureID` による「現在 audible ではない media」は 0 を返す。
   - 2D media proximity rolloff (`mProximityCamera`) は掛けない。
2. `LLPositionalStreamMgr` の distributed binding volume push で、media source のときだけ media gain policy を適用するようにした。
   - 直近適用済み volume は既存の `last_pushed_volume` で追跡する。
   - media source 以外の URL source binding には影響させない。
   - `media_source_uses_viewer_volume == true` の場合だけ media gain を掛ける。
   - 複数 media face から選択した source では `media_source_uses_viewer_volume == false` になり、source gain 1.0 として扱う。
3. media source binding の update で、現在の `LLViewerMediaImpl` から `media_gain` を読む。
   - plugin restart / child prim media のため、既存の `findMediaFor3DSource()` で現在の media impl を引く。
   - media impl が一時的に見つからない場合は `media_gain = 0.0f` として route は維持する。
4. media source の stream volume は次で計算する。

   ```text
   media_master = clamp(Stream3DVolumeMaster, 0, 1) * (use_media_volume ? clamp(media_gain, 0, 1) : 1.0)
   effective_stream_volume = parcel_audible ? media_master : 0
   ```

   `LLPositionalStreamMulti::setVolume(effective_stream_volume)` に渡す。`LLPositionalStreamMulti` 内では既存通り `effective_stream_volume * speaker.volume` が各 FMOD channel に適用される。
5. `LLViewerMedia::setVolume()` または per-media volume 操作で `LLViewerMediaImpl::updateVolume()` が呼ばれた場合、single-media 構成では次の `LLPositionalStreamMgr::update()` tick で 3D Stream 側へ反映する。
   - signal wiring は追加していない。poll/update 反映で十分と判断する。
   - 即時性が足りない場合だけ、後で manager notify を追加する。
   - multi-media 構成で選択された 3D media は viewer media volume を source gain に使わないため、media volume 操作では 3D 側音量は変わらない。`Stream3DVolumeMaster` または speaker `{volume:N}` で調整する。
6. media source route 中も plugin 側 native output は引き続き 0 にする。
   - `media_plugin_cef` の `setVolume()` は `LL_DULLAHAN_AUDIO_CALLBACK` 時に `mVolumeCatcher.setVolume(0.0f)` を維持する。
   - これにより 2D native output と FMOD 3D output の二重再生を避ける。
7. diagnostic に必要なら、media source ready / routing log へ `media_gain` と `stream_gain` を追加する。
   - 常時 spam しない。変化時または debug setting 有効時だけでよい。

処理負荷:

- media volume 対応で増える処理は軽量である。
  - 3D Stream manager の update tick で、media source binding の場合だけ現在の `LLViewerMediaImpl` を引く。
  - `mRequestedVolume * global media volume` と `std::clamp()` を数回実行する。
  - 実際の `setVolume()` は `last_pushed_volume` と比較し、変化があった場合だけ呼ぶ。
- PCM copy、CEF audio callback、FMOD output、speaker prim 数に比例する routing / spatialization の処理に比べると、volume gain 計算の追加コストは小さい。
- 注意点は `findMediaFor3DSource()` を update 中に呼ぶ回数が media source binding 数に比例すること。
  - 通常の speaker linkset 数では問題になりにくい。
  - 大量の media source speaker を想定する場合だけ、media impl / media gain の cache や dirty flag 更新を検討する。

確認項目:

- single-media 構成では、Nearby Media / media controls の volume slider を下げると MOAP → 3D Stream の音量も下がる。
- single-media 構成では、media mute で MOAP → 3D Stream が無音になる。
- multi-media 構成では、3D Stream に選ばれた media の viewer media volume は 3D 側 source gain に使われない。
- multi-media 構成では、3D Stream に選ばれていない media は従来通り media volume で音量調整できる。
- `Stream3DVolumeMaster` を下げると、URL source と media source の両方が下がる。
- single-media 構成では、media volume 50% × Stream3D 50% が体感 25% になる。
- multi-media 構成では、selected media は Stream3D 50% が体感 50% になる。
- `{volume:N}` は従来通り speaker prim ごとの相対調整として効く。
- media source route 中に 2D native output が復活しない。

### Phase 6: diagnostics

最低限、以下を `Stream3D` または `AYAMediaAudio` log に出す。

- Media 3D binding start / stop
- resolved media impl / object id / link number / face
- source kind / source binding key
- sample rate / channels / format serial
- 2D media audio disabled / restored
- PCM underflow
- ring dropped frames
- unsupported channel count
- fallback to 2D reason
- invalid binding reason (`{url}` + `{source:media}` 同時指定、複数 media face で `{link}` / `{face}` 未指定、link / face 範囲外など)

## 先に解決すべき問題

### ring が single-reader である

`LLPluginAudioRingHeader` は `mReadFrame` を 1 つしか持たない。つまり、現在の構造では 2D `LLMediaAudioStream` と 3D Stream が同じ ring を同時に読むことはできない。

初期実装では multi-reader 化しない。MOAP 3D binding が有効な media だけ 2D path を停止する。将来、同時 2D monitor や複数 3D binding が必要になった場合に tee / multi-reader ring を検討する。

### MOAP media と object/face の対応

MOAP は media texture / face index を中心に管理されている。一方、3D Stream は prim Description / linkset / speaker prim を中心に管理している。

最初から media texture UUID を自由指定にすると、同一 media を複数 object / face が共有する case が難しくなる。初期実装は「3D Stream speaker linkset 内の MOAP を、その linkset の speaker prim に接続する」に限定する。tag は root prim に置くが、media face は child prim 上でもよい。

同一 `LLViewerMediaImpl` が複数 object / face で共有される場合は、初期実装では同時に 1 つの 3D binding だけを許可する。3D binding が media impl の 2D path を止めるため、同じ impl を別 object が共有している場合に別 object 側の 2D 音声まで影響するためである。競合時は media source 3D routing を拒否し、2D fallback に戻す。

### channel count

MOAP ring は最大 8ch まで受けられる。3D Stream multi routing は、media callback source に限り `1ch / 2ch / 6ch / 8ch` を扱う。

MOAP/CEF から 6ch media audio が来る場合、channel order は `FL / FR / C / LFE / SL / SR` として扱う。この order は 3D Stream の既存 6ch routing と一致するため、追加の channel remap は不要である。

MOAP/CEF から 8ch media audio が来る場合は、7.1 source として `FL / FR / C / LFE / SL / SR / BL / BR` を期待する。speaker prim 側は `{ch:BL}` / `{ch:BR}` を追加できるようにしたが、今回の検証ゴールは 5.1ch であり、BL/BR 専用 speaker の実機確認は将来項目に残す。6ch source に対して `{ch:BL}` / `{ch:BR}` prim がある場合は、既存 5.1 の surround left / surround right をそれぞれ back speaker にも割り当てる。

2026-05-17 の外部確認では、検証用 stream は input stream としては 6ch 条件を満たしている。

```text
URL: http://go-stream-live.com:8030/stream
proxy URL: https://mp3-proxy.onrender.com/http://go-stream-live.com:8030/stream
codec_name=opus
sample_rate=48000
channels=6
channel_layout=5.1
```

`https://non-rem.com/vj/vj_051726_A.html` / `SurroundWebPlayer` 側も最大 8ch まで受ける構成とのこと。ただし、実際に 6ch として AYAstorm 側へ届くかは、Chromium / CEF / OS audio device / Dullahan callback の出力 channel 数に依存する。

その確認のため、`audio_stream_format` diagnostic を追加した。更新済み build での実機ログは次の通り。

```text
CEF audio stream started: 44100 Hz x 2 ch (ring max 8 ch)
Media multi source ready: media:... 44100 Hz x 2 ch, fmt=PCMFLOAT, ring cap 16384 frames × 2 tracks, speakers=6
```

この結果から、source stream / web page / AYAstorm media ring capacity ではなく、Chromium/CEF/Dullahan callback へ到達する前後で stereo downmix / resample されている可能性が高い。

当初の Dullahan fork では `dullahan_browser_client::GetAudioParameters()` が `return true;` だけで、CEF が事前設定した `CefAudioParameters` を変更していなかった。そのため、CEF audio callback は Chromium 側の既定 audio output configuration に従って `44100 Hz x 2 ch` になっていたと見るのが自然である。

Dullahan 側のテスト実装では、CEF audio callback に対して 7.1 / 48kHz を要求する。これは Chromium/CEF 側で stereo に落とされることを避け、少なくとも 5.1ch source を 3D Stream 側へ渡すための余裕を確保する目的である。7.1ch speaker 再生そのものは今回の合格条件ではない。

- `GetAudioParameters()` で `params.channel_layout = CEF_CHANNEL_LAYOUT_7_1`、`params.sample_rate = 48000`、`params.frames_per_buffer = 1024` を明示する。
- 44.1kHz source は Chromium/CEF 側で 48kHz callback へ resample される想定で扱う。callback PCM を 44.1kHz のまま受ける設計ではない。
- その Dullahan package を Viewer に組み込み、`CEF audio stream started: 48000 Hz x 8 ch` へ変化するか確認する。
- 変化しない場合は、CEF callback が source PCM ではなく OS/browser output sink 後段の capture である可能性が高く、CEF/Dullahan だけで source multichannel を安定取得できるか再評価する。

実機ログでは、Dullahan override 後に `48000 Hz x 8 ch` callback と `FL/FR/C/LFE/SL/SR/BL/BR` layout が確認できた。今回の speaker 構成は `FL/FR/C/LFE/SL/SR` の 6 speakers であり、5.1ch 再生確認段階としては十分と判断する。

44.1kHz source について:

- Dullahan 側で 48kHz callback を要求しているため、44.1kHz の Web audio / media source は Chromium/CEF 側で 48kHz に resample されて viewer へ届く想定である。
- viewer 側の 3D Stream media path は ring header の `mSampleRate` を見て FMOD user sound を作るため、仮に callback が 44.1kHz で来ても sample rate 自体を理由に reject する設計ではない。
- ただし現在の検証方針は「44.1kHz のまま受ける」ではなく「CEF に 48kHz output を要求し、必要なら Chromium/CEF 側で変換させる」である。
- 実機では 44.1kHz source でも問題なく再生できたため、今回の合格条件上は 44.1kHz source を追加 blocker としない。
- ノイズが出る場合、第一候補は sample rate そのものではなく、resample 後の callback cadence、ring underflow、または media source buffer が浅すぎることである。

初期実装:

- 1ch: mono として既存 routing。
- 2ch: stereo として既存 routing。
- 6ch: FL/FR/C/LFE/SL/SR として既存 routing。
- 8ch: FL/FR/C/LFE/SL/SR/BL/BR として media callback source のみ routing。今回は将来 7.1ch 対応の足がかりであり、実機合格条件は 5.1ch まで。
- その他: 3D route せず 2D fallback。

### A/V sync

MOAP video は media texture update、audio は FMOD callback で進む。3D Stream 側に入れると、`LLMultiTailRing` と speaker OPENUSER の prebuffer が追加される。厳密な lip sync は初期目標にしないが、体感で破綻しない範囲に抑える必要がある。

media source の buffer は短すぎると FMOD mixer 側の読み出しに余裕がなくなり、ざらつきや瞬断として聞こえる。現時点では 48 kHz 換算で約 43 ms の prebuffer、約 85 ms の target buffer とし、映像同期よりも音声の連続性を優先している。

ざらつき / 粒状ノイズが残る場合の次の調整候補:

- media source target buffer を 4096 frames から 6144 または 8192 frames へ増やす。
- OPENUSER decode buffer を 2048 frames から 4096 frames へ増やす。
- underflow / dropped frame diagnostic を追加し、ノイズ発生時に ring 側の供給不足か FMOD 側の読み出し不足かを切り分ける。

buffer を増やすと音声の連続性は上がるが、映像に対する音声遅延は増える。今回の優先順位は、厳密な lip sync よりも 5.1ch 3D Stream 再生が破綻なく続くことである。

実機確認では、現在の buffer 設定で A/V sync は許容範囲内である。厳密な lip sync の完全一致は対象外のままだが、PR 前の合格条件としては満たしている。

### media ON/OFF と plugin lifecycle

Media を ON/OFF したり、Nearby Media / browser 操作で media plugin が stop / restart する場合、`LLPluginClassMedia` が持つ shared memory は unmap される可能性がある。3D Stream 側が古い `LLPluginAudioRingHeader*` を保持したまま header を読むと、unmapped address への access で crash する。

このため、media source route は次の lifecycle rule にする。

- 3D Stream manager は毎 update で現在の media impl から audio ring pointer を再取得する。
- pointer が変わった、または `nullptr` になった場合、stream は speaker runtime を一旦閉じ、route 自体は維持する。
- plugin 再生成後に新しい ring pointer と format が得られたら、同じ binding 内で media ring source を再オープンする。
- `LLPluginClassMedia::getAudioData()` は plugin が running のときだけ pointer を返す。
- ring が一時的にない状態はエラーではなく、MOAP 読み込み直後や media OFF 中と同じ無音待機として扱う。

### platform 差

MOAP audio callback は build option 依存である。macOS では callback ON 時に CEF native output を mute して二重再生を避けている。Windows / Linux でも同等に native output が抑止されるか、別途確認が必要である。

## 検証計画

### unit / small tests

- PCM ring reader の wraparound。
- underflow 時に silence を返すこと。
- format serial change で stream rebuild が要求されること。
- 1ch / 2ch / 6ch / 8ch PCM を `LLMultiTailRing` に投入し、既存 URL source と同じ speaker callback 出力になること。
- unsupported channel count が 2D fallback になること。

### integration

- root prim に `{source:media}` tag を置き、root prim の media face を source として speaker prim から聞こえること。
- root prim に `{source:media}` tag を置き、child prim の media face を source として speaker prim から聞こえること。
- root prim に `{source:media}{link:N}{face:M}` tag を置き、指定 child prim / face の media を source として speaker prim から聞こえること。
- MOAP/CEF page の stereo audio が 2D ではなく speaker prim から聞こえること。
- MOAP/CEF page の 5.1 audio が `FL/FR/C/LFE/SL/SR` として speaker prim から聞こえること。今回の合格条件はここまで。
- Dullahan override 後に viewer log が `CEF audio stream started: 48000 Hz x 8 ch` を出すこと。
- media source ready log に `layout=FL/FR/C/LFE/SL/SR/BL/BR` と `speakers=6` が出ること。
- MOAP を pause / resume / navigate / reload して音声 routing が復帰すること。
- Media ON/OFF / Nearby Media stop-start 後に crash せず、3D route が復帰すること。
- object 移動で speaker 位置が更新されること。
- speaker prim 削除で安全に stop / fallback すること。
- media mute、media volume、Stream3D master volume、per-speaker volume が期待通り合成されること。
- 3D Stream disabled 時に 2D media audio に戻ること。
- media face 上のブラウザ操作で音声なしページと動画ページを切り替えても、3D route が維持され、2D media audio に一時復帰しないこと。
- `{url:...}` と `{source:media}` の同時指定が invalid binding として通知されること。
- linkset 内に media が複数 face ある状態で `{link:N}` / `{face:M}` 未指定なら invalid binding になること。
- URL source + root/child prim media 表示の構成で、speaker は URL stream を鳴らし、media audio は従来 2D path のままになること。
- MOAP/CEF page の 7.1 audio が `FL/FR/C/LFE/SL/SR/BL/BR` として speaker prim から聞こえること。これは将来確認項目であり、今回の合格条件には含めない。

### regression

- 既存 URL-based 3D Stream が壊れていないこと。
- 既存 Ogg Opus / Ogg Vorbis 3D Stream が壊れていないこと。
- MOAP 3D tag がない通常 MOAP は従来通り 2D 再生されること。
- `LL_DULLAHAN_AUDIO_CALLBACK=FALSE` build で compile が壊れないこと。
- macOS / Windows / Linux で native CEF output と FMOD output の二重再生が起きないこと。
- callback build が無効な platform / build では media source 3D routing が 2D fallback になること。

## 対象外

- libVLC / parcel media / non-CEF media の 3D Stream 接続。
- 複数 reader 対応の shared memory ring 再設計。
- MOAP audio の厳密な A/V sync。
- arbitrary / non-7.1 8ch channel layout の routing。
- 7.1.4 以上の object / height channel routing。
- URL-based 3D Stream の parser 全面刷新。
