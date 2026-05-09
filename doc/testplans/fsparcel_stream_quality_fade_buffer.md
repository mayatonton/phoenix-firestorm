# FSParcelStreamQuality 統合検証レポート

作成日: 2026-05-10 JST

## 目次

- [1. 要約](#1-要約)
  - [検証対象](#検証対象)
  - [横断結論](#横断結論)
- [2. テスト実装と検証方針](#2-テスト実装と検証方針)
  - [結論](#結論)
  - [理由](#理由)
  - [修正前の問題](#修正前の問題)
  - [テスト実装](#テスト実装)
  - [修正前に検討した運用回避策](#修正前に検討した運用回避策)
  - [EQ連動への影響](#eq連動への影響)
  - [確認ログ](#確認ログ)
  - [検証観点](#検証観点)
- [3. 修正前調査結果と根拠](#3-修正前調査結果と根拠)
  - [修正前の実測結果一覧](#修正前の実測結果一覧)
  - [バッファ計算](#バッファ計算)
  - [修正前の該当コード](#修正前の該当コード)
- [4. 計測方法](#4-計測方法)
- [5. 検証ログ抜粋](#5-検証ログ抜粋)
  - [5.1 128kbpsより高いビットレートの修正前検証](#51-128kbpsより高いビットレートの修正前検証)
  - [5.2 128kbpsストリームの修正前検証](#52-128kbpsストリームの修正前検証)
- [6. GUI設定](#6-gui設定)
- [7. 参考案・不採用案](#7-参考案不採用案)
  - [7.1 参考案: 実ビットレート自動判定](#71-参考案-実ビットレート自動判定)
  - [7.2 参考資料: 採用しない旧修正案](#72-参考資料-採用しない旧修正案)
- [8. 残課題](#8-残課題)

## 1. 要約

### 検証対象

このレポートは、`FSParcelStreamQuality` と `FSFadeAudioStream` が parcel music stream の FMOD stream buffer size に与える影響を、128kbpsストリームと128kbpsより高いビットレートのストリームの両方で整理した統合版である。

### 横断結論

`FSParcelStreamQuality=1` は、起動時の FMOD 初期化では有効に動作している。

- `resampler=4` になり、FMOD の `FMOD_DSP_RESAMPLER_SPLINE` が選ばれる
- `FSParcelStreamQuality=1` の初期バッファ指定として `409,600 bytes` が FMOD に設定される
- EQ 経路も `quality=1` 側に切り替わる想定

ただし、修正前の `FSFadeAudioStream=1` の通常フェード経路では、ストリーム開始直前に `setBufferSizes(7000, 1000)` が走り、FMOD の stream buffer が `114,688 bytes` に上書きされていた。

一方、`FSFadeAudioStream=0` の場合はこの上書き経路を通らず、`FSParcelStreamQuality=1` の `409,600 bytes` が `createStream()` 直前まで残ることを実測で確認した。

128kbpsストリームでは、`114,688 bytes` は約7.17秒分、`163,840 bytes` は約10.24秒分、`409,600 bytes` は約25.6秒分に相当する。

128kbpsより高いストリームでは、`114,688 bytes` はかなり短いバッファになる。

- 320kbpsでは約2.9秒
- 192kbpsでは約4.8秒

このため「320kbps向けのバッファ改善」を fade 有効時にも効かせたい場合は、`setBufferSizes()` 側にも `FSParcelStreamQuality` を反映する必要があると判断した。

## 2. テスト実装と検証方針

### 結論

今回のテスト実装では、**`FSParcelStreamQuality=1` のときに、`FSFadeAudioStream=1` のフェード経路でも `409,600 bytes` の stream buffer を維持すること**を検証対象にした。

つまり、以下の挙動をテスト実装後の期待値とした。

```text
FSParcelStreamQuality=0:
  現行互換を優先する
  fade off -> 163,840 bytes
  fade on  -> 114,688 bytes

FSParcelStreamQuality=1:
  320kbps / 10秒バッファを優先する
  fade off -> 409,600 bytes
  fade on  -> 409,600 bytes
```

### 理由

`FSParcelStreamQuality=1` は、現行実装上すでに以下の方針を持っている。

```text
estimated_bitrate_kbps = 320
buffer_seconds = 10
requested_size = 409,600 bytes
```

計算:

```text
320kbps * 10秒 * 128 = 409,600 bytes
```

この値を標準ベースにすると、主要なストリームでは以下の余裕になる。

```text
128kbps -> 約25.6秒分
192kbps -> 約17.1秒分
256kbps -> 約12.8秒分
320kbps -> 約10.2秒分
```

今回の目的は、高ビットレートの parcel music stream で発生する starvation / pause-resume を避けることにある。その目的に対しては、320kbps以下を安全側に吸収する `409,600 bytes` を `FSParcelStreamQuality=1` の意味として維持するのが最も現実的である。

一方、実ビットレート自動判定は理想ではあるが、`createStream()` 前に安定して取得するには HTTP/ICY preflight が必要になる。配信サーバーによって `HEAD` 非対応、HTTP/0.9、接続リセット、`icy-br` 欠落があり得るため、今回のテスト実装としては実装リスクが高い。

したがって今回のテスト実装では、`FSParcelStreamQuality=1` を「320kbpsまでを安全側に扱うモード」と定義し、fade 経路でもその値を潰さないことを優先した。

### 修正前の問題

修正前は `FSFadeAudioStream=1` の場合、`LLViewerAudio` の fade 経路から以下が呼ばれる。

```cpp
stream->setBufferSizes(FMODEX_STREAM_BUFFER_SIZE, FMODEX_DECODE_BUFFER_SIZE);
```

定数:

```text
FMODEX_STREAM_BUFFER_SIZE = 7000 ms
FMODEX_DECODE_BUFFER_SIZE = 1000 ms
```

修正前の `LLStreamingAudio_FMODSTUDIO::setBufferSizes()` は、常に 128kbps 前提で stream buffer を計算していた。

```cpp
const U32 stream_buffer_size = streambuffertime / 1000 * 128 * 128;
```

その結果:

```text
7000 / 1000 * 128 * 128 = 114,688 bytes
```

つまり、`FSParcelStreamQuality=1` で一度 `409,600 bytes` が設定されても、fade 経路では `114,688 bytes` に上書きされる。

この `114,688 bytes` は以下に相当する。

```text
128kbps -> 約7.17秒分
192kbps -> 約4.78秒分
320kbps -> 約2.87秒分
```

高ビットレート stream では明らかに短く、`FSParcelStreamQuality=1` の目的と衝突している。

### テスト実装

今回のテスト実装では、`LLStreamingAudio_FMODSTUDIO::setBufferSizes()` で decode buffer 設定は従来通り維持しつつ、`mQuality == 1` の場合だけ stream buffer を `applyStreamBufferSize()` で戻す。

実装内容:

```cpp
void LLStreamingAudio_FMODSTUDIO::setBufferSizes(U32 streambuffertime, U32 decodebuffertime)
{
    const U32 stream_buffer_size = streambuffertime / 1000 * 128 * 128;
    if (Check_FMOD_Error(mSystem->setStreamBufferSize(stream_buffer_size, FMOD_TIMEUNIT_RAWBYTES),
                         "FMOD::System::setStreamBufferSize"))
    {
        return;
    }

    FMOD_ADVANCEDSETTINGS settings;
    memset(&settings, 0, sizeof(settings));
    settings.cbSize = sizeof(settings);
    settings.defaultDecodeBufferSize = decodebuffertime;
    Check_FMOD_Error(mSystem->setAdvancedSettings(&settings), "FMOD::System::setAdvancedSettings");

    if (mQuality == 1)
    {
        applyStreamBufferSize();
    }
}
```

この実装では、`FSParcelStreamQuality=0` の挙動は基本的に現行互換のまま維持される。一方、`FSParcelStreamQuality=1` では、fade 経路ありでも最終的な `before createStream` が `409,600 bytes` になる。

実装状況:

```text
2026-05-10:
  テスト実装として、以下に適用済み。

  indra/llaudio/llstreamingaudio_fmodstudio.cpp
    LLStreamingAudio_FMODSTUDIO::setBufferSizes()

  Release app テストビルド済み。
  build-darwin-universal/newview/Release/AYAstorm.app
```

初回テスト結果:

```text
条件:
  FSParcelStreamQuality=1
  FSFadeAudioStream=1
  stream: http://209.222.109.253:7746
  stream実測: 320kbps MP3 / 48kHz / stereo

結果:
  setBufferSizes 後に一度 114,688 bytes になる
  その直後に applyStreamBufferSize() が走り 409,600 bytes へ戻る
  before createStream でも 409,600 bytes を確認
```

該当ログ:

```text
2026-05-09T18:26:03Z setBufferSizes ... requested_size=114688
2026-05-09T18:26:03Z FMOD stream buffer size [after setBufferSizes]: size=114688 type=8
2026-05-09T18:26:03Z FSParcelStreamQuality buffer hint: quality=1 estimated_bitrate_kbps=320 seconds=10 requested_size=409600
2026-05-09T18:26:03Z FMOD stream buffer size [after applyStreamBufferSize]: size=409600 type=8
2026-05-09T18:26:03Z Starting internet stream: http://209.222.109.253:7746
2026-05-09T18:26:03Z FMOD stream buffer size [before createStream]: size=409600 type=8
```

このテストでは、再生開始後に starvation が1回発生した。

```text
2026-05-09T18:26:29Z Stream starvation detected! Pausing stream until buffer nearly full.
2026-05-09T18:26:29Z   (diskbusy=1)
2026-05-09T18:26:29Z   (progress=49)
```

ただし、この starvation は今回の `409,600 bytes` 再適用漏れではない。ログ上、`before createStream` は `409,600 bytes` であり、修正自体は期待どおり動作している。

外部確認では、この配信は `ICY 200 OK` で始まる SHOUTcast/ICY 系の応答であり、通常の HTTP/1.x ヘッダとしては扱いにくい。`curl` では通常接続だと HTTP/0.9 扱いで拒否され、`--http0.9` 許可時に受信できた。受信データを `ffprobe` で確認した結果は `320kbps MP3 / 48kHz / stereo` だった。

したがって、starvation については本修正の成否判定からはいったん切り分ける。必要になった場合は、別課題として `getOpenState()` の `open_state` / `progress` / `starving` / `diskbusy` と、初回 `setPaused(false)` 時点の `progress` を追加計測する。

追加テスト結果: 192kbps stream

```text
条件:
  FSParcelStreamQuality=1
  FSFadeAudioStream=1
  stream: http://209.222.109.253:7746
  stream実測: 192kbps MP3 / 48kHz / stereo

注意:
  URLは320kbpsテスト時と同一。
  配信側で同一URLのまま bitrate が 320kbps から 192kbps に変更された。
  appログ上は同一URLのため `same URL as previous` と表示される。

結果:
  setBufferSizes 後に一度 114,688 bytes になる
  その直後に applyStreamBufferSize() が走り 409,600 bytes へ戻る
  before createStream でも 409,600 bytes を確認
```

該当ログ:

```text
2026-05-09T18:33:26Z Audio URL filter: no active alert, same URL as previous: http://209.222.109.253:7746
2026-05-09T18:33:26Z setBufferSizes ... requested_size=114688
2026-05-09T18:33:26Z FMOD stream buffer size [after setBufferSizes]: size=114688 type=8
2026-05-09T18:33:26Z FSParcelStreamQuality buffer hint: quality=1 estimated_bitrate_kbps=320 seconds=10 requested_size=409600
2026-05-09T18:33:26Z FMOD stream buffer size [after applyStreamBufferSize]: size=409600 type=8
2026-05-09T18:33:26Z Starting internet stream: http://209.222.109.253:7746
2026-05-09T18:33:26Z FMOD stream buffer size [before createStream]: size=409600 type=8
```

外部確認:

```text
ICY 200 OK
icy-name:CLUB Beatrice
content-type:audio/mpeg
icy-br:192

ffprobe:
  codec_name=mp3
  sample_rate=48000
  channels=2
  bit_rate=192000
  format_name=mp3
  format bit_rate=192155
```

判定:

```text
FSParcelStreamQuality=1
FSFadeAudioStream=1
192kbps stream
before createStream = 409,600 bytes
```

これにより、`FSParcelStreamQuality=1` の fade 経路で `409,600 bytes` を維持する挙動は、320kbps stream と 192kbps stream の両方で確認できた。

追加テスト結果: 128kbps stream / 現行互換確認

```text
条件:
  FSParcelStreamQuality=0
  FSFadeAudioStream=1
  stream: http://209.222.109.253:7746
  stream実測: 128kbps MP3 / 48kHz / stereo

注意:
  URLは320kbps / 192kbps テスト時と同一。
  配信側で同一URLのまま bitrate が 128kbps に変更された。

結果:
  setBufferSizes 後に 114,688 bytes になる
  FSParcelStreamQuality=0 のため、applyStreamBufferSize() による 409,600 bytes 再適用は発生しない
  before createStream は 114,688 bytes のまま
```

該当ログ:

```text
2026-05-09T18:36:30Z setBufferSizes ... requested_size=114688
2026-05-09T18:36:30Z FMOD stream buffer size [after setBufferSizes]: size=114688 type=8
2026-05-09T18:36:30Z Starting internet stream: http://209.222.109.253:7746
2026-05-09T18:36:30Z FMOD stream buffer size [before createStream]: size=114688 type=8

2026-05-09T18:36:41Z FMOD stream buffer size [before createStream]: size=114688 type=8
2026-05-09T18:36:48Z FMOD stream buffer size [before createStream]: size=114688 type=8
```

外部確認:

```text
ICY 200 OK
icy-name:CLUB Beatrice
content-type:audio/mpeg
icy-br:128

ffprobe:
  codec_name=mp3
  sample_rate=48000
  channels=2
  bit_rate=128000
  format_name=mp3
  format bit_rate=128121
```

判定:

```text
FSParcelStreamQuality=0
FSFadeAudioStream=1
128kbps stream
before createStream = 114,688 bytes
```

これにより、今回のテスト実装は `mQuality == 1` の場合だけ `409,600 bytes` を再適用し、`FSParcelStreamQuality=0` の fade 経路では従来通り `114,688 bytes` を維持することを確認できた。

この128kbpsテストでも、FMOD側で以下の openstate error が出た。

```text
Internet stream openstate error
The specified resource requires authentication or is forbidden.
```

これは同一URLの SHOUTcast/ICY 応答形式との相性による可能性が高く、今回のバッファ再適用修正の成否判定からはいったん切り分ける。

### 修正前に検討した運用回避策

コード修正を入れない場合は、修正前の挙動を仕様として明記し、運用回避策として案内する案も検討した。

この場合、`FSParcelStreamQuality=1` を有効にしたうえで、GUI側の `FSFadeAudioStream` を切り替えることで、最終的な stream buffer size が変わることを明記する。

修正前の挙動:

```text
FSParcelStreamQuality=1:
  FSFadeAudioStream=0 / fade off -> 409,600 bytes
  FSFadeAudioStream=1 / fade on  -> 114,688 bytes
```

GUI上では、`FSFadeAudioStream` は以下の項目で切り替えられる。

```text
環境設定 / Preferences
  -> サウンド & メディア
  -> 音楽
  -> 区画音声のフェードを有効にする：
```

修正前は、高ビットレート stream の安定性を優先する場合に、このチェックをOFFにすることで `FSParcelStreamQuality=1` の `409,600 bytes` バッファを維持できた。

ただし、この案で変わるのは主に stream buffer size と再生安定性であり、`FSParcelStreamQuality=1` による `resampler=4` や high-shelf EQ が GUI の fade on/off で切り替わるわけではない。

また、この案はコード上の不整合を修正するものではない。`FSFadeAudioStream=1` のときに `FSParcelStreamQuality=1` の 320kbps / 10秒バッファが `114,688 bytes` に上書きされる問題は残るため、根本対処としては前述のテスト実装と同じ方向のコード修正を優先する。

テスト実装後は、`FSParcelStreamQuality=1` かつ `FSFadeAudioStream=1` でも `before createStream` が `409,600 bytes` になるため、この運用回避策は主対策ではなく参考情報として扱う。

### EQ連動への影響

`FSParcelStreamQuality=1` の場合、バッファ拡張だけでなく stream EQ も有効になる。

現行実装では `LLStreamingAudio_FMODSTUDIO::setQuality()` が以下を呼ぶ。

```cpp
mQuality = quality;
applyStreamBufferSize();
applyStreamEq();
```

EQ 側の `applyStreamEq()` は `mQuality` を見て、以下のように切り替える。

```text
mQuality == 0 -> FMOD_DSP_MULTIBAND_EQ_FILTER_DISABLED
mQuality == 1 -> FMOD_DSP_MULTIBAND_EQ_FILTER_HIGHSHELF
```

`FSParcelStreamQuality=1` では、stream group に追加済みの `FMOD_DSP_TYPE_MULTIBAND_EQ` が high-shelf として動作する。

設定内容:

```text
filter = HIGHSHELF
frequency = 6000 Hz
gain = +4 dB
```

今回のテスト実装は `setBufferSizes()` 後に `mQuality == 1` の場合だけ `applyStreamBufferSize()` を再適用するものであり、`mQuality` を変更しない。そのため、`FSParcelStreamQuality=1` で有効になっている EQ はそのまま維持される。

つまり、テスト実装後の `FSParcelStreamQuality=1` は以下を同時に満たす。

```text
fade off -> 409,600 bytes + high-shelf EQ
fade on  -> 409,600 bytes + high-shelf EQ
```

#### EQ連動が壊れないかの追加検証

サブエージェント2系統で独立にコード経路を確認した結果、今回のテスト実装で stream EQ が壊れる可能性は低い、という結論で一致した。

根拠は以下。

- `mStreamGroup` は `LLStreamingAudio_FMODSTUDIO` の生成時に一度だけ作成される
- `FMOD_DSP_TYPE_MULTIBAND_EQ` も同じ生成時に作成され、`mStreamGroup->addDSP()` で stream group に接続される
- `start()` / pending start / fade 経路はいずれも同じ `mStreamGroup` を `LLAudioStreamManagerFMODSTUDIO` に渡す
- 実際の再生は `playSound(..., mChannelGroup, ...)` で行われるため、stream group に接続された EQ DSP を通る
- `setBufferSizes()` は `setStreamBufferSize()` と `setAdvancedSettings()` を呼ぶだけで、`mStreamGroup`、`mStreamEqDsp`、DSP chain を作り直さない
- テスト実装で追加する `applyStreamBufferSize()` も `setStreamBufferSize()` の再適用だけであり、`applyStreamEq()` や EQ DSP の parameter を変更しない

したがって、`FSParcelStreamQuality=1` で `applyStreamEq()` が一度実行され、high-shelf EQ が有効になっていれば、fade 経路で `setBufferSizes()` が走った後に `applyStreamBufferSize()` を再適用しても、EQ状態は維持されると判断できる。

ただし、これは Firestorm 側のコード経路に基づく判断である。FMOD内部で `System::setAdvancedSettings()` や `System::setStreamBufferSize()` が既存DSP graphへ副作用を持つ可能性までは、コード探索だけでは完全には否定できない。厳密に確認する場合は、テスト実装後のビルドで `mStreamEqDsp->getParameterInt(FMOD_DSP_MULTIBAND_EQ_A_FILTER, ...)` をログ出力し、fade on/off の両方で `FMOD_DSP_MULTIBAND_EQ_FILTER_HIGHSHELF` が維持されることを実測する。

### 確認ログ

テスト実装後、`FSParcelStreamQuality=1`, `FSFadeAudioStream=1` では以下のログを確認する。

```text
setBufferSizes stream buffer hint: ... requested_size=114688
FMOD stream buffer size [after setBufferSizes]: size=114688 type=8

FSParcelStreamQuality buffer hint: quality=1 estimated_bitrate_kbps=320 seconds=10 requested_size=409600
FMOD stream buffer size [after applyStreamBufferSize]: size=409600 type=8

Starting internet stream: ...
FMOD stream buffer size [before createStream]: size=409600 type=8
```

`FSParcelStreamQuality=0`, `FSFadeAudioStream=1` では、現行互換として以下を期待する。

```text
setBufferSizes stream buffer hint: ... requested_size=114688
FMOD stream buffer size [after setBufferSizes]: size=114688 type=8

Starting internet stream: ...
FMOD stream buffer size [before createStream]: size=114688 type=8
```

### 検証観点

テスト実装後は、最低限以下を確認する。

- `FSParcelStreamQuality=1`, `FSFadeAudioStream=1`, 320kbps stream
- `FSParcelStreamQuality=1`, `FSFadeAudioStream=1`, 192kbps stream
- `FSParcelStreamQuality=1`, `FSFadeAudioStream=1`, 128kbps stream
- `FSParcelStreamQuality=0`, `FSFadeAudioStream=1`, 128kbps stream
- `FSParcelStreamQuality=0`, `FSFadeAudioStream=0`, 192kbps stream

確認すべきログ:

```text
FSParcelStreamQuality buffer hint
setBufferSizes stream buffer hint
FMOD stream buffer size [after setBufferSizes]
FMOD stream buffer size [after applyStreamBufferSize]
FMOD stream buffer size [before createStream]
Stream starvation detected
Internet stream openstate error
```

## 3. 修正前調査結果と根拠

この章は、今回のテスト実装を入れる前に確認した実測値を整理したものである。テスト実装後の合格ログは、前章の「初回テスト結果」「追加テスト結果」に記録している。

### 修正前の実測結果一覧

| 条件 | ストリーム | resampler | `setBufferSizes()` | `before createStream` の実バッファ | 換算秒数 |
|---|---|---:|---:|---:|---:|
| `FSParcelStreamQuality=1`, `FSFadeAudioStream=1` | 192kbps | `4` | あり | `114,688 bytes` | 約4.8秒 |
| `FSParcelStreamQuality=1`, `FSFadeAudioStream=0` | 192kbps | `4` | なし | `409,600 bytes` | 約17.1秒 |
| `FSParcelStreamQuality=0`, `FSFadeAudioStream=0` | 192kbps | `2` | なし | `163,840 bytes` | 約6.8秒 |
| `FSParcelStreamQuality=0`, `FSFadeAudioStream=1` | 高ビットレート側 | `2` | あり | `114,688 bytes` | 320kbpsで約2.9秒 / 192kbpsで約4.8秒 |
| `FSParcelStreamQuality=0`, `FSFadeAudioStream=0` | 128kbps | `2` | なし | `163,840 bytes` | 約10.24秒 |
| `FSParcelStreamQuality=1`, `FSFadeAudioStream=0` | 128kbps | `4` | なし | `409,600 bytes` | 約25.6秒 |
| `FSParcelStreamQuality=1`, `FSFadeAudioStream=1` | 128kbps | `4` | あり | `114,688 bytes` | 約7.17秒 |
| `FSParcelStreamQuality=0`, `FSFadeAudioStream=1` | 128kbps | `2` | あり | `114,688 bytes` | 約7.17秒 |

### バッファ計算

`FSParcelStreamQuality=1` の意図した設定:

```text
320 kbps * 10 seconds * 128 bytes/kbit = 409,600 bytes
```

`FSParcelStreamQuality=0` の初期設定:

```text
128 kbps * 10 seconds * 128 bytes/kbit = 163,840 bytes
```

フェード経路の `setBufferSizes(7000, 1000)` による上書き:

```text
7000 / 1000 * 128 * 128 = 114,688 bytes
```

### 修正前の該当コード

`LLStreamingAudio_FMODSTUDIO::applyStreamBufferSize()` は `FSParcelStreamQuality` を見てバッファサイズを設定する。

```cpp
const U32 buffer_seconds = 10;
const U32 estimated_bitrate = (mQuality == 1) ? 320u : 128u;
const U32 stream_buffer_size = estimated_bitrate * buffer_seconds * 128;
```

修正前の `LLStreamingAudio_FMODSTUDIO::setBufferSizes()` は固定で 128 kbps 前提の計算を行っていた。

```cpp
const U32 stream_buffer_size = streambuffertime / 1000 * 128 * 128;
```

`FSFadeAudioStream=1` の場合、`LLViewerAudio` がストリーム開始前に `setBufferSizes(FMODEX_STREAM_BUFFER_SIZE, FMODEX_DECODE_BUFFER_SIZE)` を呼ぶため、修正前は `applyStreamBufferSize()` の結果が上書きされていた。

## 4. 計測方法

`indra/llaudio/llstreamingaudio_fmodstudio.cpp` に診断ログを追加した。PR向けには通常ログを汚さないよう、診断ログは `LL_DEBUGS("AudioImpl")` に落としている。

FMOD から実際の stream buffer size を取得する API:

```cpp
mSystem->getStreamBufferSize(&file_buffer_size, &file_buffer_size_type)
```

追加した主なログ:

```text
FSParcelStreamQuality buffer hint: quality=...
setBufferSizes stream buffer hint: streambuffertime_ms=...
FMOD stream buffer size [after applyStreamBufferSize]: size=... type=...
FMOD stream buffer size [after setBufferSizes]: size=... type=...
FMOD stream buffer size [before createStream]: size=... type=...
```

計測用テストビルド:

```text
build-darwin-universal/newview/Release/AYAstorm.app
```

ビルド確認:

- `ayastorm-bin` Release build 成功
- app binary 更新時刻: `May 9 23:55:47 2026`
- `codesign --verify --deep --strict --verbose=2` 成功

## 5. 検証ログ抜粋

### 5.1 128kbpsより高いビットレートの修正前検証
#### 検証対象

この章は主に **128kbpsより高いビットレートの parcel music stream** を対象にした検証である。

実測で使用した高ビットレート側のストリームは以下。

- 320kbps: `http://live.na2.lightmanstreams.com:9295`
- 192kbps: `http://chantal.day-zero-music.com:8500`

#### 検証ログ抜粋

##### 1. `FSParcelStreamQuality=1`, `FSFadeAudioStream=1`

計測ビルド:

```text
Version: Firestorm-AYAstorm-release 7.2.4.261281529 [ba5150aa3c]
resampler=4
```

FMOD バッファ推移:

```text
quality=0 applyStreamBufferSize: requested_size=163840
after quality=0 applyStreamBufferSize: size=163840 type=8

quality=1 applyStreamBufferSize: requested_size=409600
after quality=1 applyStreamBufferSize: size=409600 type=8

setBufferSizes(7000, 1000): requested_size=114688
after setBufferSizes: size=114688 type=8

before createStream: size=114688 type=8
```

ストリーム:

```text
Starting internet stream: http://chantal.day-zero-music.com:8500
icy-br:192
icy-sr:44100
content-type: audio/mpeg
```

判定:

```text
FSParcelStreamQuality=1 で一度 409,600 bytes になるが、
fade 経路の setBufferSizes() により createStream() 直前では 114,688 bytes。
```

##### 2. `FSParcelStreamQuality=1`, `FSFadeAudioStream=0`

設定:

```text
FSParcelStreamQuality = 1
FSFadeAudioStream = 0
```

起動:

```text
Version: Firestorm-AYAstorm-release 7.2.4.261281529 [ba5150aa3c]
resampler=4
```

FMOD バッファ推移:

```text
quality=0 applyStreamBufferSize: requested_size=163840
after quality=0 applyStreamBufferSize: size=163840 type=8

quality=1 applyStreamBufferSize: requested_size=409600
after quality=1 applyStreamBufferSize: size=409600 type=8

before createStream: size=409600 type=8
```

この起動では `setBufferSizes stream buffer hint` 行は出ていない。

ストリーム:

```text
Starting internet stream: http://chantal.day-zero-music.com:8500
icy-br:192
icy-sr:44100
content-type: audio/mpeg
```

判定:

```text
Fade off では setBufferSizes() の上書きが発生せず、
createStream() 直前の実バッファは 409,600 bytes。
```

追加追跡で starvation が1回出た。

```text
2026-05-09T15:05:45Z Stream starvation detected! Pausing stream until buffer nearly full.
diskbusy=0
progress=50
```

その後の約60秒の追跡では、追加の stream start / buffer size change / starvation は確認されなかった。

##### 3. `FSParcelStreamQuality=0`, `FSFadeAudioStream=0`

設定:

```text
FSParcelStreamQuality = 0
FSFadeAudioStream = 0
```

起動:

```text
Version: Firestorm-AYAstorm-release 7.2.4.261281529 [ba5150aa3c]
resampler=2
```

FMOD バッファ推移:

```text
quality=0 applyStreamBufferSize: requested_size=163840
after quality=0 applyStreamBufferSize: size=163840 type=8

before createStream: size=163840 type=8
```

この起動でも `setBufferSizes stream buffer hint` 行は出ていない。

ストリーム:

```text
Starting internet stream: http://chantal.day-zero-music.com:8500
icy-br:192
icy-sr:44100
content-type: audio/mpeg
```

判定:

```text
FSParcelStreamQuality=0 かつ Fade off では、
createStream() 直前の実バッファは 163,840 bytes。
```

その後の約30秒の追跡では、追加の stream start / buffer size change / starvation は確認されなかった。

##### 4. `FSParcelStreamQuality=0`, fade 経路あり

起動時に `FSParcelStreamQuality` の保存値がなく、デフォルト `0` で起動したケース。

```text
resampler=2
FSParcelStreamQuality buffer hint: quality=0
```

FMOD バッファ推移:

```text
after applyStreamBufferSize: size=163840 type=8
after setBufferSizes:        size=114688 type=8
before createStream:         size=114688 type=8
```

判定:

```text
quality=0 でも fade 経路を通ると、初期値 163,840 bytes から
setBufferSizes() により 114,688 bytes へ上書きされる。
```

#### ストリーム別の秒数換算

`114,688 bytes`:

- 320 kbps では約 2.9 秒
- 192 kbps では約 4.8 秒

`163,840 bytes`:

- 320 kbps では約 4.1 秒
- 192 kbps では約 6.8 秒

`409,600 bytes`:

- 320 kbps では約 10.2 秒
- 192 kbps では約 17.1 秒

#### 高ビットレート側の修正前判断

`FSParcelStreamQuality=1` の実装は、SPLINE resampler と 320 kbps 用バッファ指定そのものは機能している。

ただし修正前の `FSFadeAudioStream=1` の経路では、後続の `setBufferSizes()` が `FSParcelStreamQuality` を考慮していなかったため、実際の FMOD stream buffer は `114,688 bytes` に戻っていた。

このため「320 kbps 向けのバッファ改善」を常に有効にしたい場合は、`setBufferSizes()` 側にも `FSParcelStreamQuality` を反映する必要があると判断した。

### 5.2 128kbpsストリームの修正前検証
#### 検証対象

この章は **128kbps の parcel music stream** を対象にした追跡結果である。

対象ストリーム:

```text
http://209.222.109.253:7746
```

このストリームは `curl -I` では接続リセットされ、通常の ICY ヘッダ取得ができなかった。`ffprobe` でストリーム本体を解析した。

`ffprobe` 結果:

```text
codec_name=mp3
codec_type=audio
sample_rate=48000
channels=2
bit_rate=128000
format_name=mp3
duration=N/A
bit_rate=128000
```

判定:

```text
128kbps MP3 / 48kHz / 2ch
```

#### `FSParcelStreamQuality=0`, `FSFadeAudioStream=0`

現在の検証条件では、128kbpsストリームに対して `FSParcelStreamQuality=0` / `FSFadeAudioStream=0` の実バッファは `163,840 bytes` だった。

この値は 128kbps 前提の 10秒バッファに相当するため、128kbpsストリームでは設計意図どおりのサイズになっている。

追跡中、同じ128kbpsストリームが再開始されたが、`before createStream` の実バッファは引き続き `163,840 bytes` だった。対象ログの範囲では starvation は確認されていない。

ユーザー設定:

```text
FSFadeAudioStream = 0
FSParcelStreamQuality = 0
```

補足:

- `FSParcelStreamQuality` はユーザー設定ファイルに保存項目がなく、アプリ同梱デフォルト `0` として動作していた
- `FSFadeAudioStream=0` はユーザー設定に保存されていた
- そのため `setBufferSizes(7000, 1000)` の fade 経路上書きは発生していない

起動ログ:

```text
Version: Firestorm-AYAstorm-release 7.2.4.261281529 [ba5150aa3c]
resampler=2
```

`resampler=2` は `FSParcelStreamQuality=0` 側の通常経路である。

起動時の `applyStreamBufferSize()`:

```text
FSParcelStreamQuality buffer hint: quality=0 estimated_bitrate_kbps=128 seconds=10 requested_size=163840
FMOD stream buffer size [after applyStreamBufferSize]: size=163840 type=8
```

ストリーム開始時:

```text
Starting internet stream: http://209.222.109.253:7746
FMOD stream buffer size [before createStream]: size=163840 type=8
```

同じストリームが再開始された時:

```text
Stopping internet stream: http://209.222.109.253:7746
Starting internet stream: http://209.222.109.253:7746
FMOD stream buffer size [before createStream]: size=163840 type=8
```

バッファ秒数:

```text
163,840 bytes / 16,000 bytes/sec = 10.24 sec
```

つまり、128kbpsストリームでは約10秒分のバッファが確保されている。

対象ログ:

```text
Starting internet stream: http://209.222.109.253:7746
FMOD stream buffer size [before createStream]: size=163840 type=8
```

追加追跡:

- 約30秒間、対象イベントなし
- その後、同じストリームが stop/start された
- 再開始時も `before createStream: size=163840 type=8`
- 追跡範囲では `Stream starvation detected` は確認されていない

判定:

128kbpsストリームに対しては、`FSParcelStreamQuality=0` の `163,840 bytes` が実測上そのまま `createStream()` 直前まで残っている。

この条件では、128kbpsストリームに対して約10秒分のバッファが確保されており、`FSParcelStreamQuality=1` の 320kbps向け拡張バッファを使わなくても設計上は不足していない。

#### `FSParcelStreamQuality=1`, `FSFadeAudioStream=0`

app 再起動後、同じ128kbpsストリームで `FSParcelStreamQuality=1` の条件を追跡した。

設定:

```text
FSFadeAudioStream = 0
FSParcelStreamQuality = 1
```

起動ログ:

```text
Version: Firestorm-AYAstorm-release 7.2.4.261281529 [ba5150aa3c]
resampler=4
```

ストリーム解析:

```text
codec_name=mp3
sample_rate=48000
channels=2
bit_rate=128000
format_name=mp3
bit_rate=128000
```

FMOD バッファ推移:

```text
FSParcelStreamQuality buffer hint: quality=0 estimated_bitrate_kbps=128 seconds=10 requested_size=163840
FMOD stream buffer size [after applyStreamBufferSize]: size=163840 type=8

FSParcelStreamQuality buffer hint: quality=1 estimated_bitrate_kbps=320 seconds=10 requested_size=409600
FMOD stream buffer size [after applyStreamBufferSize]: size=409600 type=8

Starting internet stream: http://209.222.109.253:7746
FMOD stream buffer size [before createStream]: size=409600 type=8
```

追加追跡中、同じ128kbpsストリームが再開始された。

```text
Stopping internet stream: http://209.222.109.253:7746
Starting internet stream: http://209.222.109.253:7746
FMOD stream buffer size [before createStream]: size=409600 type=8
```

この追跡範囲では `Stream starvation detected` は確認されていない。

判定:

```text
FSParcelStreamQuality=1 / Fade off では、
128kbpsストリームでも createStream() 直前の実バッファは 409,600 bytes。
```

128kbps換算:

```text
409,600 bytes / 16,000 bytes/sec = 25.6 sec
```

つまり、128kbpsストリームでは約25.6秒分のバッファになる。

#### `FSParcelStreamQuality=1`, `FSFadeAudioStream=1`

app 再起動後、同じ128kbpsストリームで fade 有効経路を追跡した。

設定:

```text
FSParcelStreamQuality = 1
FSFadeAudioStream = 1
```

補足:

- `FSParcelStreamQuality=1` はユーザー設定に保存されていた
- `FSFadeAudioStream` はユーザー設定にはなく、アプリ同梱デフォルト `1` が使われた

起動ログ:

```text
Version: Firestorm-AYAstorm-release 7.2.4.261281529 [ba5150aa3c]
resampler=4
```

FMOD バッファ推移:

```text
FSParcelStreamQuality buffer hint: quality=0 estimated_bitrate_kbps=128 seconds=10 requested_size=163840
FMOD stream buffer size [after applyStreamBufferSize]: size=163840 type=8

FSParcelStreamQuality buffer hint: quality=1 estimated_bitrate_kbps=320 seconds=10 requested_size=409600
FMOD stream buffer size [after applyStreamBufferSize]: size=409600 type=8

setBufferSizes stream buffer hint: streambuffertime_ms=7000 decodebuffertime_ms=1000 requested_size=114688
FMOD stream buffer size [after setBufferSizes]: size=114688 type=8

Starting internet stream: http://209.222.109.253:7746
FMOD stream buffer size [before createStream]: size=114688 type=8
```

判定:

```text
FSParcelStreamQuality=1 で一度 409,600 bytes になるが、
FSFadeAudioStream=1 の fade 経路で setBufferSizes() が走り、
128kbpsストリームでも createStream() 直前の実バッファは 114,688 bytes になる。
```

128kbps換算:

```text
114,688 bytes / 16,000 bytes/sec = 7.168 sec
```

この追跡範囲では `Stream starvation detected` と `Internet stream openstate error` は確認されていない。

#### 再起動後 `FSParcelStreamQuality=0`, `FSFadeAudioStream=1`

app 再起動後、同じ128kbpsストリームで再度追跡した。

設定状態:

```text
FSParcelStreamQuality = 0
FSFadeAudioStream = 1
```

補足:

- ユーザー設定 `user_settings/settings.xml` に `FSParcelStreamQuality` は保存されていなかった
- アプリ同梱デフォルトの `FSParcelStreamQuality` は `0`
- アプリ同梱デフォルトの `FSFadeAudioStream` は `1`

該当ログ抜粋:

```text
2026-05-09T15:43:26Z FSParcelStreamQuality buffer hint: quality=0 estimated_bitrate_kbps=128 seconds=10 requested_size=163840
2026-05-09T15:43:26Z FMOD stream buffer size [after applyStreamBufferSize]: size=163840 type=8

2026-05-09T15:44:29Z setBufferSizes stream buffer hint: streambuffertime_ms=7000 decodebuffertime_ms=1000 requested_size=114688
2026-05-09T15:44:29Z FMOD stream buffer size [after setBufferSizes]: size=114688 type=8
2026-05-09T15:44:29Z Starting internet stream: http://209.222.109.253:7746
2026-05-09T15:44:29Z FMOD stream buffer size [before createStream]: size=114688 type=8
```

判定:

```text
FSParcelStreamQuality=0 では初期バッファ指定は 163,840 bytes だが、
FSFadeAudioStream=1 の fade 経路で setBufferSizes() が走り、
createStream() 直前の実バッファは 114,688 bytes になる。
```

128kbps換算:

```text
114,688 bytes / 16,000 bytes/sec = 7.168 sec
```

この追跡範囲では、追加で約1分確認しても `Stream starvation detected`、`FMOD::Sound::getOpenState`、`Internet stream openstate error` は確認されていない。

## 6. GUI設定

フェード経路の有無は `FSFadeAudioStream` で決まる。

この項目はGUI上でも設定できる。場所は以下。

```text
環境設定 / Preferences
  -> サウンド & メディア
  -> 音楽
  -> 区画音声のフェードを有効にする：
```

英語UIでは以下のラベルで表示される。

```text
Preferences
  -> Sound & Media
  -> Music
  -> Enable Parcel Audio Fading:
```

対応するXUI定義:

```text
indra/newview/skins/default/xui/ja/panel_preferences_sound.xml
  <check_box name="FSFadeAudioStream"
             label="区画音声のフェードを有効にする："/>

indra/newview/skins/default/xui/en/panel_preferences_sound.xml
  <check_box name="FSFadeAudioStream"
             control_name="FSFadeAudioStream"
             label="Enable Parcel Audio Fading:"/>
```

このチェックがONの場合、`FSFadeAudioStream=1` となり、今回の検証でいう「fade 経路あり」になる。ストリーム開始前には `setBufferSizes(7000, 1000)` が呼ばれる。

修正前は、この経路で FMOD stream buffer が `114,688 bytes` に上書きされていた。テスト実装後は、`FSParcelStreamQuality=1` の場合だけ直後に `applyStreamBufferSize()` を再適用し、`before createStream` を `409,600 bytes` に戻す。`FSParcelStreamQuality=0` の場合は現行互換として `114,688 bytes` を維持する。

このチェックがOFFの場合、`FSFadeAudioStream=0` となり、今回の検証でいう「fade 経路なし」になる。この場合、`setBufferSizes()` による上書きは発生せず、`applyStreamBufferSize()` で設定された `163,840 bytes` または `409,600 bytes` が `createStream()` 直前まで残る。

同じ音楽設定パネルには、フェード時間を調整する以下のスライダーもある。

```text
フェードイン    -> FSAudioMusicFadeIn
フェードアウト  -> FSAudioMusicFadeOut
```

## 7. 参考案・不採用案

### 7.1 参考案: 実ビットレート自動判定

実ビットレートに完全追従する設計は理想形ではあるが、現時点では実装するには現実的とは言い難い。

したがって、これは採用前提の将来案ではなく、参考案として扱う。

理想形:

```text
再生開始ごとに HTTP/ICY header を確認する
icy-br が取れれば、その値で buffer bytes を計算する
取れなければ FSParcelStreamQuality の推定値へ fallback する
```

例:

```text
128kbps * 10秒 * 128 = 163,840 bytes
192kbps * 10秒 * 128 = 245,760 bytes
256kbps * 10秒 * 128 = 327,680 bytes
320kbps * 10秒 * 128 = 409,600 bytes
```

ただし、この案には以下の課題がある。

- `createStream()` 前に別途 HTTP/ICY preflight が必要
- `HEAD` 非対応の配信サーバーがある
- HTTP/0.9 や接続リセットのように通常のヘッダ取得が失敗する stream がある
- `icy-br` が常に存在するとは限らない
- 同じURLでも配信者やAutoDJ切替で bitrate が変わることがある
- URLキャッシュは参考値にはなるが、同一URLの次回配信が同一bitrateとは限らない
- preflight が失敗した場合の fallback 設計が複雑になる
- 再生開始前に追加通信を入れるため、ストリーム開始の遅延や失敗要因が増える
- FMOD の既存 stream 作成経路とは別に HTTP/ICY 解析経路を持つことになり、保守範囲が広がる

このため、実ビットレート自動判定は初回リリース修正には含めない。現実的な方針としては、`FSParcelStreamQuality=1` を「320kbpsまでを安全側に扱うモード」として定義し、まずはその `409,600 bytes` バッファを fade 経路でも維持する。

### 7.2 参考資料: 採用しない旧修正案

#### 旧案A: `setBufferSizes()` 側で `mQuality` を見て 7秒分を計算する

```cpp
const U32 estimated_bitrate = (mQuality == 1) ? 320u : 128u;
const U32 stream_buffer_size = streambuffertime / 1000 * estimated_bitrate * 128;
```

デメリット:

- `FSParcelStreamQuality=1` でも `7 * 320 * 128 = 286,720 bytes` にしかならない
- 320kbps換算では約7.17秒分で、既存の `409,600 bytes` / 10秒設計より短い
- 実ビットレートを見ているわけではない
- `setBufferSizes()` の責務が quality policy 込みに変わる

#### 旧案B: `LLViewerAudio` 側で quality に応じた stream buffer time を渡す

デメリット:

- `LLViewerAudio` が FMOD の buffer policy を知ることになり、層が混ざる
- 現在2箇所ある `setBufferSizes()` 呼び出しに同じ分岐が必要
- 将来呼び出し箇所が増えた場合に対応漏れが起きやすい
- 実ビットレート自動判定ではない

#### 旧案C: URLごとの bitrate cache を主軸にする

デメリット:

- 同じURLでも次回の配信が同じbitrateとは限らない
- AutoDJ / live切替 / サーバー設定変更で bitrate が変わる可能性がある
- 可変ビットレートでは単一値にしづらい
- キャッシュは fallback の補助にはなるが、主軸にはできない

## 8. 残課題

- テスト実装を入れたビルドを作成する -> 完了
- `FSParcelStreamQuality=1`, `FSFadeAudioStream=1`, 320kbps stream で `before createStream: size=409600` になることを確認する -> 完了
- starvation は本修正の成否判定からはいったん切り分ける
- 192kbps stream でも `FSParcelStreamQuality=1`, `FSFadeAudioStream=1` の `before createStream: size=409600` を確認する -> 完了
- `FSParcelStreamQuality=0` の現行互換が崩れていないことを確認する -> 完了
- 計測用ログを `LL_DEBUGS("AudioImpl")` へ落とす -> 完了
