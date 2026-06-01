# AYAstorm 3D Stream URL Source 音切れ対策 修正報告書

**作成日**: 2026-05-31
**最終更新**: 2026-06-01
**対象ブランチ**: `fix/ayastorm-r32-3dstream-url-buffer`
**対象 app**: `build-darwin-universal/newview/Release/AYAstorm.app`
**報告対象**: 3D Stream URL Source の Ogg Opus / Vorbis live stream 再生安定化
**パッケージ化**: 未実施

**修正対象コード**:
- `indra/llaudio/llpositionalstreammulti.cpp`
- `indra/llaudio/llpositionalstreammulti.h`
- `indra/llaudio/fmod_codec_ogg.cpp`

**調査対象コード**:
- `indra/llaudio/llstreamingaudio_fmodstudio.cpp`

## 目次

- [0. 修正報告サマリ](#0-修正報告サマリ)
  - [0.1 結論](#01-結論)
  - [0.2 修正概要](#02-修正概要)
  - [0.3 現在の実装値](#03-現在の実装値)
  - [0.8 再検証用ログメモ](#08-再検証用ログメモ)
- [1. 問題定義](#1-問題定義)
- [2. 調査開始時のバッファ構造](#2-調査開始時のバッファ構造)
  - [2.1 3D Stream URL Source](#21-3d-stream-url-source)
  - [2.2 Parcel Music の 163,840 bytes との違い](#22-parcel-music-の-163840-bytes-との違い)
- [3. 根本原因仮説](#3-根本原因仮説)
- [4. 修正方針](#4-修正方針)
- [5. 実装計画](#5-実装計画)
  - [5.1 2026-06-01 実測ログによる更新](#51-2026-06-01-実測ログによる更新)
- [6. 2026-06-01 実装修正](#6-2026-06-01-実装修正)
  - [6.1 Ogg Opus codec](#61-ogg-opus-codec)
  - [6.2 3D Stream URL Source](#62-3d-stream-url-source)
  - [6.10 CBR target buffer retune](#610-2026-06-01-cbr-target-buffer-retune)
  - [6.14 5.1 / 7.1 scope](#614-51--71-scope)
  - [6.15 将来設計: audio streaming core の共通化](#615-将来設計-audio-streaming-core-の共通化)
  - [6.17 ayastorm-release 比の 3D Stream 負荷見積もり](#617-ayastorm-release-比の-3d-stream-負荷見積もり)
  - [6.18 MOAP / Dullahan / CEF 経由で同じ現象が出にくい理由](#618-moap--dullahan--cef-経由で同じ現象が出にくい理由)
- [7. 受入条件](#7-受入条件)
- [8. 判断ポイント](#8-判断ポイント)

## 0. 修正報告サマリ

### 0.1 結論

今回の 3D Stream URL Source 音切れは、単純な「無音検知」ではない。配信側は Opus CBR を基本方針にする。AYAstorm 側は CBR の連続 packet 供給を前提に、FMOD `readData()` の数秒 block を decoded PCM ring で吸収する。

VBR/CVBR 無音で Ogg EOS page が見えていない `FMOD_ERR_FILE_EOF + 0 bytes` 相当になるケースへの防御は残すが、主経路ではない。CBR でも `readData()` が数秒 block した実ログがあるため、FMOD/HTTP 側の block を吸収できる decoded PCM ring を持つ。

通常音量時の短い途切れは別系統で、`read_bytes == 0` ではなく、AYAstorm 側の URL Source `readData()` が数秒 block し、ring が枯れたことが原因。2026-06-01 の再計測では `262144 frames = 約 5.46 秒` でも実際には満杯付近まで積めず、3.8 秒級の `readData()` block で dropout が残った。そのため Parcel Music の体感 10 秒程度の遅延を許容範囲とし、短期対策として ring capacity を `524288 frames = 約 10.92 秒` に拡張し、定常目標を `393216 frames = 約 8.19 秒` にする。

ただし 2026-06-01T09:00-09:15Z の CBR 実ログでは、ring capacity 拡張後も `readData()` が平均 3.26 秒、最大 4.77 秒 block し、`zero/notready/eof/errors` は 0 のまま dropout が発生した。つまり CBR 方針は必要だが、それだけで品質目標を満たすとは言えない。

### 0.2 修正概要

| 項目 | 報告内容 |
|---|---|
| 発生事象 | 3D Stream URL Source で最大 10 秒級の音切れ、通常音量時にも短い途切れ |
| 送信側方針 | Opus は CBR を基本方針にする |
| AYAstorm 側原因 | FMOD `readData()` が数秒 block し、decoded PCM ring が枯れる |
| 0 byte / EOF 対策 | Ogg EOS 未検出の `FMOD_ERR_FILE_EOF + 0 bytes` を即 EOF 扱いしない |
| NOTREADY 対策 | `FMOD_ERR_NOTREADY` は一時 starvation として扱い、ring 残量と underrun で reconnect 判断 |
| バッファ対策 | URL Source PCM ring を `524288 frames`、target / startup prebuffer を `393216 frames` に拡張 |
| ログ整理 | 調査用の詳細ログは本番コードから削除。再検証用ログ項目のみ本書に残す |
| 検証結果 | arm64 Release build 成功。CBR 実ログではまだ `readData()` block と dropout が残る |
| 残課題 | 根本対策は FMOD 同期 `readData()` 依存を外す Audio Streaming Core 化 |

### 0.3 現在の実装値

| 項目 | 現在値 | 意味 |
|---|---:|---|
| URL Source PCM ring capacity | `524288 frames` | 48 kHz で約 10.92 秒 |
| URL Source PCM target | `393216 frames` | 定常目標。48 kHz で約 8.19 秒 |
| startup prebuffer | `393216 frames` | 48 kHz で約 8.19 秒 |
| FMOD raw stream buffer | `163840 bytes` | compressed input 側の buffer |
| zero read empty ring grace | `0.50 sec` | 0 byte read + underrun 後の reconnect 猶予 |
| NOTREADY empty ring grace | `0.50 sec` | NOTREADY + underrun 後の reconnect 猶予 |
| final zero-fill safety valve | `10.0 sec` | 最終安全弁 |

CBR 前提では `393216 frames` で再生開始し、定常時も `393216 frames` 前後を維持する。開始待ちは約 8 秒台へ増えるが、FMOD `readData()` の平均 3.26 秒 / 最大 4.77 秒級 block への耐性を上げる短期対策として、起動時から深く積む。実ログではこの設定後も dropout が残っているため、これは品質目標を満たす最終対策ではない。

### 0.4 実装済み変更

- Ogg/Opus codec は Ogg EOS page を見ていない `FMOD_ERR_FILE_EOF + 0 bytes` を即 EOF にしない
- Ogg EOS 未検出で PCM を返せない場合は `FMOD_ERR_NOTREADY` を返す
- 3D Stream 側は `FMOD_ERR_NOTREADY` を一時 starvation として扱う
- ring が残っている間は reconnect しない
- ring が空に近く、speaker callback 側の underrun が増えたら、`0.50 sec` 後に reconnect へ進む
- URL Source PCM ring capacity を `32768` から `524288` frames に拡張
- CBR 前提の定常 target buffer として `393216` frames を追加
- startup prebuffer を `393216` frames に設定
- FMOD raw stream buffer を URL Source open 前に `163840 bytes` へ設定
- 調査中に追加した詳細診断ログは本番コードから削除済み。再検証時に戻すログタグと項目は `0.8` に残す

### 0.5 実機ログで確認済みのこと

- 新ビルドでは `ring cap 524288 frames x 6 tracks` として起動する
- 4 秒前後の pump 停滞で ring が残るケースもあるが、CBR 実ログでは ring が 0.17-1.8 秒程度まで落ち、dropout も発生した
- 18:02:07Z に `Ogg feed starved ... bytes=0 eos=0` が発生し、Ogg EOS 未検出の starvation として扱えた
- 旧 `NOTREADY` 猶予 10 秒では、ring が空になった後も数秒 zero-fill が残った
- そのため `kNotReadyEmptyRingGraceSec` は `0.50 sec` へ短縮済み

### 0.6 ビルド状態

2026-06-01 に arm64 専用 app を差分ビルド済み。

```text
build-darwin-universal/newview/Release/AYAstorm.app
```

確認結果:

```text
** BUILD SUCCEEDED **
lipo: arm64 non-fat
codesign --verify --deep --strict: valid
```

パッケージ化はしていない。

### 0.7 残リスク

- 配信側が長時間 compressed bytes を完全に止める場合、AYAstorm 側だけで無音 PCM を復元することはできない
- その場合は reconnect で復帰させるか、受信側で HTTP/Ogg demux を自前化して PCM silence を補完する必要がある
- 配信側 CBR 化は引き続き有効な回避策。AYAstorm 側の修正は、VBR/低ビットレート無音で即 EOF 誤判定しないための耐性強化
- CBR でも FMOD `readData()` が 3-4 秒級 block を周期的に起こす実ログがあるため、`524288 frames` ring / `393216 frames` target は短期対策であり最終解ではない
- 2026-06-01T09:00-09:15Z の CBR 実ログでは最大 4.77 秒 block と dropout が残ったため、FMOD `readData()` 依存を外す将来設計の優先度は高い

### 0.8 再検証用ログメモ

今回の詳細診断ログは通常ビルドから削除する。再度切り分けが必要になった場合だけ、一時的に以下を戻す。

対象タグ:

- `Stream3D`
- `FmodOgg`

`Stream3D` で戻す候補:

- `Multi pump stats`
  - `win`
  - `pump_calls`
  - `max_gap`
  - `reads`
  - `bytes`
  - `frames_read`
  - `frames_written`
  - `readData_total`
  - `readData_max`
  - `readData_slow`
  - `notready`
  - `zero`
  - `eof_zero`
  - `errors`
  - `ring_full`
  - `buffered`
  - `write_avail`
  - `underrun_callbacks`
  - `codec`
- `Multi source readData blocked`
  - `elapsed`
  - `rr`
  - `read_bytes`
  - `want_bytes`
  - `buffered_before`
  - `write_avail_before`
  - `codec`
- `Multi source not ready`
  - `elapsed`
  - `buffered frames/sec`
  - `underrun_callbacks_delta`
  - `codec`
- `Multi source returned 0 bytes`
  - `elapsed`
  - `rr`
  - `openstate`
  - `buffered frames/sec`
  - `underrun_callbacks_delta`
  - `codec`
- `Multi dropout`
  - 既存の underrun / dropout 確認用ログ。詳細診断を戻す場合もこれと突き合わせる

`FmodOgg` で戻す候補:

- `Ogg feed starved`
  - `r`
  - `bytes=0`
  - `eos`
  - `streak`
- `Ogg feed returned 0 bytes without EOF`
  - `eos`
  - `streak`

注意:

- これらは検証用であり、通常ビルドへ常時入れない
- `readData()` block と ring 枯渇の相関を見る場合は、`Multi pump stats` と `Multi source readData blocked` を同時に戻す
- VBR/CVBR 無音由来の Ogg starvation を見る場合は、`FmodOgg` の starvation ログと `Stream3D` の `NOTREADY` / `buffered` を同時に戻す

### 0.9 本書の構成

- `0`: 修正報告サマリ。提出・共有用の結論
- `1`-`4`: 調査開始時の問題定義、旧実装、原因仮説、修正方針
- `5`-`6`: 実装計画、実測ログ、実装修正、負荷見積もり
- `7`-`8`: 受入条件と判断ポイント

以降は調査時系列を含む詳細資料として残す。

## 1. 問題定義

3D Stream URL Source で、Ogg Opus 配信に対して最大約 10 秒の音切れが報告された。ストリーミングサーバー側には異常がないため、AYAstorm クライアント側の URL Source 取り込み、Ogg Opus decode、3D Stream ring buffer、reconnect 判定を調査対象とする。

調査開始時の実装では `LLPositionalStreamMulti::pumpSource()` が `FMOD::Sound::readData()` から `read_bytes == 0` を受け取ると、`kZeroFillStreakLimit = 10.0` 秒までは即失敗にせず待っていた。

この間に 3D Stream ring が枯れると、`pcmReadCallback()` は不足分をゼロ埋めするため、ユーザーには無音として聞こえる。

## 2. 調査開始時のバッファ構造

### 2.1 3D Stream URL Source

`LLPositionalStreamMulti` の URL Source は、FMOD URL stream から PCM を読み出し、自前の `LLMultiTailRing` に書き込む。

調査開始時 / `ayastorm-release` 値:

| 定数 | 値 | 意味 |
|---|---:|---|
| `kRingFrames` | `32768` frames | URL Source の 3D Stream PCM ring 容量 |
| `kPrebufferFrames` | `4096` frames | 再生開始前に最低限ためる量 |
| `kZeroFillStreakLimit` | `10.0` sec | `read_bytes == 0` が続いても待つ最大時間 |
| `kMaxFramesPerPump` | `8192` frames | 1 回の source pump で読む最大量 |

48 kHz の場合:

| 値 | 時間 |
|---|---:|
| `32768` frames | 約 0.683 秒 |
| `4096` frames | 約 0.085 秒 |

つまり、3D Stream URL Source の自前 PCM ring は 1 秒未満しかない。一方で `read_bytes == 0` の許容時間は 10 秒であり、ring 容量に対して長すぎる。

### 2.2 Parcel Music の 163,840 bytes との違い

Parcel Music は `LLStreamingAudio_FMODSTUDIO` で FMOD の stream buffer を `FMOD_TIMEUNIT_RAWBYTES` として設定している。

既定 quality 0:

```text
128 kbps * 10 sec * 128 = 163,840 bytes
```

これは圧縮ストリーム側の raw bytes buffer であり、3D Stream の自前 PCM ring と同じ意味ではない。

3D Stream ring は decode 後の PCM frames を保持する。仮に `163,840 bytes` を decode 後 PCM ring として扱うと、保持時間はチャンネル数とサンプル形式で大きく変わる。

48 kHz PCMFLOAT の場合:

| Source | bytes/frame | 163,840 bytes 相当 |
|---|---:|---:|
| mono | 4 | 約 0.853 秒 |
| stereo | 8 | 約 0.427 秒 |
| 5.1ch | 24 | 約 0.142 秒 |

そのため `163,840 bytes` をそのまま 3D Stream の解決策として適用するのは不適切。3D Stream 側では bytes ではなく frames/sec と ring 残量で判断する必要がある。

## 3. 根本原因仮説

現時点で最も疑うべき箇所は `read_bytes == 0` そのものではなく、その後の扱い。

現行:

1. `mSourceSound->readData()` が `read_bytes == 0` を返す
2. `mZeroFillStreakStart` を開始または継続
3. 10 秒未満なら `return 0`
4. ring に新規 PCM が供給されない
5. ring が枯れる
6. `pcmReadCallback()` がゼロ埋めする
7. ユーザーには無音として聞こえる
8. 10 秒経過後に `setFailed(FailReason::Network, "zero-fill streak")`
9. manager 側 reconnect cascade に進む

問題は、3D Stream ring が約 0.68 秒しかないのに、ring 残量と無関係に 10 秒待つこと。

## 4. 修正方針

### 4.1 やらないこと

- `FMOD_ERR_FILE_EOF && read_bytes == 0` だけで即 reconnect しない
- ring に残っている PCM を無条件に捨てない
- Parcel Music の `163,840 bytes` を 3D Stream PCM ring にそのまま流用しない
- Ogg Opus をサーバー問題と決めつけない

### 4.2 やること

`read_bytes == 0` の判定を buffer-aware にする。

判断材料:

| 情報 | 目的 |
|---|---|
| `FMOD_RESULT rr` | `FMOD_OK`, `FMOD_ERR_FILE_EOF`, `FMOD_ERR_NOTREADY` を区別 |
| `FMOD_OPENSTATE` | FMOD source が ready/playing/error か確認 |
| `mRing.readAvailable(0)` | 3D Stream ring の残PCM量を確認 |
| `buffered_sec` | 残PCMを秒単位に換算 |
| `mUnderrunCallbacks` delta | 実際にユーザー向け callback が不足しているか確認 |
| source codec name | Ogg Opus / Ogg Vorbis / others をログで区別 |

## 5. 実装計画

### M1: instrumentation

目的: まず 0 byte が発生した瞬間の状態をログで確定する。

対象:
- `LLPositionalStreamMulti::pumpSource()`

追加ログ:

```text
readData zero:
  rr=<FMOD result>
  openstate=<FMOD_OPENSTATE>
  buffered_frames=<mRing.readAvailable(0)>
  buffered_sec=<buffered_frames / sample_rate>
  sample_rate=<mSampleRate>
  source_channels=<mSourceChannels>
  source_type=<mSourceType>
  underrun_callbacks_delta=<delta>
  url=<mUrl>
```

ログは 1 秒 debounce し、音声 callback ごとに連発しない。

完了条件:
- 実機ログで `read_bytes == 0` 発生時に ring 残量と openstate が確認できる
- 通常再生中にログが過剰出力されない

### M2: buffer-aware zero-byte policy

目的: ring が残っている間は切らず、ring が枯れて実害が出たら 10 秒待たず reconnect に進める。

案:

```cpp
if (read_bytes == 0)
{
    const size_t buffered = mRing.readAvailable(0);
    const F64 buffered_sec = (mSampleRate > 0)
                           ? (F64)buffered / (F64)mSampleRate
                           : 0.0;

    const bool eof_zero = (rr == FMOD_ERR_FILE_EOF);
    const bool ring_nearly_empty = buffered_sec <= kZeroReadMinBufferedSec;
    const bool underrun_seen = underrunCallbacksIncreasedSinceZeroStart();

    if (eof_zero && ring_nearly_empty)
    {
        setFailed(FailReason::Network, "eof zero with empty ring");
        return 0;
    }

    if (ring_nearly_empty && underrun_seen &&
        now - mZeroFillStreakStart >= kZeroReadEmptyRingGraceSec)
    {
        setFailed(FailReason::Network, "zero read with underrun");
        return 0;
    }

    if (now - mZeroFillStreakStart >= kZeroFillStreakLimit)
    {
        setFailed(FailReason::Network, "zero-fill streak");
        return 0;
    }
}
```

初期候補値:

| 定数 | 値 | 理由 |
|---|---:|---|
| `kZeroReadMinBufferedSec` | `0.20` sec | mixer callback 数回分の余裕は残す |
| `kZeroReadEmptyRingGraceSec` | `0.50` sec | 一瞬の decoder gap は許容し、実音切れは短くする |
| `kZeroFillStreakLimit` | `10.0` sec | 最終安全弁として残す |

完了条件:
- ring に PCM が残っている場合は即 reconnect しない
- ring が枯れて underrun が出た場合、10 秒待たず reconnect へ進む
- 短い `FMOD_ERR_NOTREADY` / 一時的 0 byte で過剰 reconnect しない

### M3: Ogg Opus codec error accounting

目的: `fmod_codec_ogg.cpp` 側の仕様と実装のズレを解消する。

現状:
- 仕様書では「decode 失敗は一定回数まで吸収し、連続失敗で EOF」としている
- 現実装には明示的な連続失敗カウンタがない

作業:
- `OggCodecState` に `consecutive_decode_failures` と `consecutive_feed_failures` を追加
- `opus_decode_float()` / `opus_multistream_decode_float()` が `frames <= 0` の場合にカウント
- 正常 decode でカウントをリセット
- 一定回数を超えたら `state->eof = true` にして `opusRead()` が明確に EOF を返す
- 失敗ログには packet bytes と Opus error code を出す

完了条件:
- 単発 packet failure では stream を落とさない
- 連続 failure では 3D Stream 側が原因を判別できる形で EOF/reconnect に進む
- Ogg Vorbis path に回帰を入れない

### M4: ring capacity policy review

目的: 3D Stream URL Source の PCM ring 容量が実運用に対して十分か判断する。

現行 `32768 frames` は 48 kHz で約 0.68 秒。ネットワーク jitter buffer としては浅い。

候補:

| 候補 | 48 kHz時間 | 2-track PCMFLOAT memory | 6-track PCMFLOAT memory |
|---|---:|---:|---:|
| `32768` frames | 0.68 sec | 256 KiB | 768 KiB |
| `65536` frames | 1.37 sec | 512 KiB | 1.5 MiB |
| `131072` frames | 2.73 sec | 1 MiB | 3 MiB |
| `196608` frames | 4.10 sec | 1.5 MiB | 4.5 MiB |
| `262144` frames | 5.46 sec | 2 MiB | 6 MiB |
| `393216` frames | 8.19 sec | 3 MiB | 9 MiB |
| `524288` frames | 10.92 sec | 4 MiB | 12 MiB |

方針:
- reconnect 判定修正を先に行う
- それでも短時間 gap が残る場合は URL Source ring の拡張を行う
- 実測で約 1.9-2.0 秒の pump 停滞が出ているため、`65536 frames` は不足する可能性が高い
- `131072`, `196608`, `262144 frames` でも CBR 実測で dropout / ほぼ枯渇が残ったため、短期対策は ring capacity 自体を `524288 frames` へ拡張する
- Parcel Music の体感 10 秒程度の遅延が許容範囲であるため、定常 target は `393216 frames = 約 8.19 sec` とする
- これ以上はさらに ring capacity を増やすか、FMOD 同期 `readData()` 依存を外す必要がある
- 5.1ch でも memory cost は数 MiB以内なので、同時 stream 数上限と合わせて評価する

完了条件:
- ring 拡張が必要か、M1/M2 の実測で判断できる
- 拡張する場合は URL Source のみ対象にし、MediaRing source の低遅延設計には触れない

## 5.1 2026-06-01 実測ログによる更新

M1/M2 実装後の `Multi pump stats` で、通常音量再生中の途切れを確認した。

重要な点:

- 該当区間では `zero=0`, `eof_zero=0`, `errors=0`, `notready=0`
- つまり `read_bytes == 0` / EOF / socket error / FMOD not-ready ではない
- 途切れ直前に `win` が約 1.9-2.0 秒まで伸びる
- 同じ区間で `reads=5`, `frames_read=20480` まで落ちる
- ring 残量が `4096 frames = 0.085 sec` まで落ちる
- その後 `Multi dropout` が記録される

代表ログ:

```text
2026-05-31T17:02:12Z Multi pump stats:
  win=1.99856s reads=5 frames_read=20480 frames_written=20480
  zero=0 eof_zero=0 errors=0 notready=0
  buffered=4096 (0.0853333s) underrun_callbacks=72

2026-05-31T17:02:19Z Multi dropout:
  294912 zero-fill frames across 72 callbacks

2026-05-31T17:02:21Z Multi pump stats:
  win=1.93936s reads=5 frames_read=20480 frames_written=20480
  zero=0 eof_zero=0 errors=0 notready=0
  buffered=4096 (0.0853333s) underrun_callbacks=66

2026-05-31T17:02:29Z Multi dropout:
  270336 zero-fill frames across 66 callbacks
```

このため、通常音量時の途切れについては、少なくとも上記ログ上ではサーバー切断や Opus DTX/VAD ではなく、AYAstorm 側の URL Source decode/pump が一時的に約 2 秒停滞し、現行 ring `32768 frames = 0.68 sec` が吸収できずに枯れている。

更新後の判断:

1. `read_bytes == 0` / EOF 系の 10 秒無音問題には M2 の buffer-aware reconnect が必要
2. 通常音量時の短い途切れには URL Source ring 拡張が必要
3. 実測上 `65536 frames = 1.37 sec` は約 2 秒停滞を吸収できないため不十分
4. 次の実装修正候補は `kRingFrames` を `131072` 以上にすること。CBR 実測後の現行値は `kRingFrames=524288`, `kTargetBufferedFrames=393216`, `kPrebufferFrames=393216` とする
5. 追加で `readData()` 呼び出し時間を測り、FMOD 内でブロックしているのか decode thread scheduling が止まっているのかを分離する

## 6. 2026-06-01 実装修正

今回の修正では、配信側を CBR に逃がすだけではなく、AYAstorm 側でも低ビットレート VBR 無音を EOF と誤判定しにくい形へ変更した。

### 6.1 Ogg Opus codec

対象: `indra/llaudio/fmod_codec_ogg.cpp`

- `FMOD_ERR_FILE_EOF && bytes_read == 0` を即 `state->eof = true` にしない
- libogg が Ogg EOS page を見た場合だけ、0 byte EOF を実 EOF として扱う
- Ogg EOS を見ていない 0 byte EOF は、ライブ HTTP stream の一時的な feed starvation として扱う
- PCM frame を 1 frame も返せないが実 EOF でもない場合、`opusRead()` は `FMOD_ERR_NOTREADY` を返す
- starvation は `Ogg feed starved ... eos=... streak=...` としてログに出す

狙いは、VBR 無音で compressed bytes が極端に少ない区間を「ファイル終端」と断定せず、3D Stream 側に「まだ次の Ogg page を待っている状態」として伝えること。

### 6.2 3D Stream URL Source

対象:
- `indra/llaudio/llpositionalstreammulti.cpp`
- `indra/llaudio/llpositionalstreammulti.h`

変更値:

| 定数 | 旧値 | 新値 | 意味 |
|---|---:|---:|---|
| `kRingFrames` | `32768` | `524288` | URL Source decoded PCM ring capacity |
| `kTargetBufferedFrames` | 未設定 | `393216` | CBR 前提の定常 decoded PCM target |
| `kFmodStreamBufferBytes` | 未設定 | `163840` | FMOD raw compressed stream buffer |
| `kPrebufferFrames` | `4096` | `393216` | 再生開始前の最低 PCM 量 |
| `kNotReadyEmptyRingGraceSec` | 未設定 | `0.50 sec` | NOTREADY 中に ring が空で underrun した後の reconnect 猶予 |

48 kHz では:

| 値 | 時間 |
|---|---:|
| `kRingFrames = 524288` | 約 10.92 秒 |
| `kTargetBufferedFrames = 393216` | 約 8.19 秒 |
| `kPrebufferFrames = 393216` | 約 8.19 秒 |

ring capacity は増やしたが、定常時に満杯まで読まない。CBR 前提では `kTargetBufferedFrames` 付近で読みを止める。startup も `393216 frames` まで積んでから開始するため、意図した開始待ちは約 8.19 秒 + FMOD raw buffer 側の待ち。定常遅延も約 8.19 秒を見込む。

補足: 一時的に FMOD raw stream buffer を `1048576 bytes` に上げた版では、CBR 384 kbps 前後の配信で `openSourceStream` から `Multi source ready` まで約 23 秒かかった。これは `1048576 bytes / 48000 bytes/sec = 約 21.8 sec` と整合するため、live stream の起動待ちとしては過大。Parcel Music 相当の `163840 bytes` へ戻し、compressed 側は約 3.4 秒目安にする。

### 6.3 NOTREADY handling

`FMOD_ERR_NOTREADY` は即失敗にせず、VBR 無音または一時的な compressed input starvation として扱う。

ただし無限に待たない。以下を満たした場合は reconnect へ進める。

- ring 残量が `0.20 sec` 以下
- speaker callback 側で underrun が実際に増えている
- NOTREADY が `0.50 sec` 以上継続

2026-06-01 の旧猶予設定での実機ログでは、`NOTREADY` 開始から約 6 秒で ring が空になり、その後 `10.0 sec` 猶予まで待ったため、約 4 秒の audible zero-fill が残った。そのため現行設定では `kNotReadyEmptyRingGraceSec` を `0.50 sec` に下げる。ring に PCM が残っている間は条件を満たさないため、正常な一時 starvation では reconnect しない。

ログ:

```text
Multi source not ready for <sec>s for <url>
  buffered=<frames> (<sec>s)
  underrun_callbacks_delta=<n>
  codec=<codec>
```

### 6.4 zero-byte EOF handling

`read_bytes == 0` は引き続き buffer-aware に処理する。

- `FMOD_ERR_FILE_EOF && read_bytes == 0` でも ring に PCM が残っていれば即 reconnect しない
- ring が `0.20 sec` 以下なら EOF zero は reconnect 寄りにする
- EOF ではない 0 byte は、ring が空に近く、underrun が出て、`0.50 sec` 継続したら reconnect
- 最終安全弁として `10.0 sec` の zero-fill streak は残す

### 6.5 検証で見るログ

VBR 無音テストで見るべき結果:

- 正常: `Ogg feed starved ... eos=0` が出ても、すぐ `zero-fill streak` / socket failure / reconnect へ進まない
- 正常: `Multi pump stats` の `notready` が増えても、`buffered` が残っている間は dropout しない
- 異常: `buffered` が `0.20 sec` 以下へ落ち、`underrun_callbacks_delta` が増え、NOTREADY が 0.50 秒以上続く
- 異常: `eos=1` の後の EOF zero は実終端として扱われる

### 6.6 Build verification

2026-06-01 に arm64 専用 app としてビルド済み。

生成物:

```text
build-darwin-universal/newview/Release/AYAstorm.app
```

ビルドコマンド:

```text
DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer xcodebuild \
  -project build-darwin-universal/Firestorm.xcodeproj \
  -scheme ayastorm-bin \
  -configuration Release \
  -derivedDataPath build-darwin-universal/DerivedData \
  ARCHS=arm64 ONLY_ACTIVE_ARCH=YES build
```

結果:

```text
** BUILD SUCCEEDED **
```

確認:

```text
lipo -info build-darwin-universal/newview/Release/AYAstorm.app/Contents/MacOS/AYAstorm
=> Non-fat file: ... is architecture: arm64

codesign --verify --deep --strict --verbose=2 build-darwin-universal/newview/Release/AYAstorm.app
=> valid on disk
=> satisfies its Designated Requirement
```

注意:

- パッケージ化はしていない
- 署名は ad-hoc 署名
- 実機確認では、VBR 無音区間で `Ogg feed starved`, `Multi source not ready`, `Multi pump stats` を中心に見る

### 6.7 2026-06-01 runtime log follow-up

修正ビルド実行後、18:02:07Z に VBR/無音系の starvation を確認した。

代表ログ:

```text
2026-05-31T18:02:07Z Ogg feed starved ... bytes=0 eos=0 streak=1
2026-05-31T18:02:07Z Multi source not ready ... buffered=258048 frames (5.376s) underrun_callbacks_delta=0
2026-05-31T18:02:12Z Multi source not ready ... buffered=20480 frames (0.426667s) underrun_callbacks_delta=0
2026-05-31T18:02:13Z Multi source not ready ... buffered=0 frames (0s) underrun_callbacks_delta=42
2026-05-31T18:02:17Z Multi source not ready for 10s with empty ring and active underruns ... transitioning to Failed for reconnect
```

判定:

- Ogg EOS は出ていないため、実ファイル終端ではない
- ただし FMOD/Ogg feed は 0 byte EOF 相当を継続しており、compressed input が入っていない
- 5.46 秒 ring により、従来より約 5 秒は吸収できた
- ring が空になった後も旧 `10.0 sec` NOTREADY 猶予で待ったため、audible zero-fill が残った
- `kNotReadyEmptyRingGraceSec = 0.50 sec` へ下げ、empty ring + underrun 後は速やかに reconnect させる

### 6.8 2026-06-01 readData timing instrumentation

CBR でも以下のような pump stall が残った。

```text
18:23:09 Multi pump stats:
  win=5.96505s
  reads=7
  notready=0 zero=0 eof_zero=0 errors=0
  buffered=4096 (0.0853333s)
  underrun_callbacks=0
```

このログだけでは、次のどちらかを分離できない。

1. decode thread 自体が数秒間 `pumpSource()` を呼べていない
2. `pumpSource()` は呼ばれているが、`FMOD::Sound::readData()` の中でブロックしている

そのため `Multi pump stats` に以下を追加した。

| field | 意味 |
|---|---|
| `pump_calls` | 集計窓内で `pumpSource()` に入った回数 |
| `max_gap` | `pumpSource()` 呼び出し間隔の最大値 |
| `readData_total` | 集計窓内で `readData()` に費やした合計時間 |
| `readData_max` | 1 回の `readData()` 最大所要時間 |
| `readData_slow` | `readData()` が `0.25 sec` 以上かかった回数 |

追加で、`readData()` が `0.25 sec` 以上ブロックした場合は個別にログを出す。

```text
Multi source readData blocked for <sec>s
  rr=<FMOD result>
  read_bytes=<bytes>
  want_bytes=<bytes>
  buffered_before=<frames>
  write_avail_before=<frames>
  codec=<codec>
```

判定:

- `win` が長く、`max_gap` も長い場合: decode thread scheduling / pump loop 停止寄り
- `win` が長く、`max_gap` は短いが `readData_max/readData_total` が長い場合: FMOD `readData()` 内部 block 寄り
- `ring_full` が多く、`max_gap/readData_max` が短い場合: ring が満杯で読む必要がなかっただけ
- `readData_slow > 0` が出る場合: FMOD/HTTP/Ogg decode 側の block を優先して調査する

### 6.9 2026-06-01 CBR test: startup wait and readData block

計測入りビルドで CBR 配信を追跡した結果:

```text
18:35:26 openSourceStream
18:35:49 Multi source ready
18:35:49 Multi path playing
```

`openSourceStream` から `Multi source ready` まで約 23 秒。再生開始後の通常時は `readData_max` が約 1ms、`zero/eof/notready/errors` は 0。

ただし再生後に以下を確認した。

```text
18:36:33 Multi source readData blocked for 5.53109s
  rr=No errors.
  read_bytes=98304
  want_bytes=98304
  buffered_before=258048 frames
  write_avail_before=4096 frames

18:36:33 Multi pump stats:
  win=5.97165s
  pump_calls=73
  max_gap=0.007509s
  readData_total=5.53538s
  readData_max=5.53109s
  readData_slow=1
  buffered=4096 (0.0853333s)
  underrun_callbacks=6
```

このケースでは `max_gap` は短く、decode thread 自体は止まっていない。`readData()` 内部で約 5.5 秒ブロックし、その間に decoded PCM ring がほぼ空になっている。したがって CBR 時の途切れは `0 bytes / EOF` ではなく、FMOD `readData()` blocking が直接原因。

### 6.10 2026-06-01 CBR target buffer retune

CBR 前提版で `kFmodStreamBufferBytes=163840`, `kTargetBufferedFrames=65536` を試した結果:

```text
18:45:56 openSourceStream
18:46:00 Multi source ready
18:46:00 Multi path playing
```

`1048576 bytes` 版の約 23 秒 startup は解消し、ready まで約 4 秒に短縮した。

一方で、定常 target `65536 frames = 約 1.37 sec` では以下の block を吸収しきれなかった。

```text
18:46:07 readData blocked for 1.65332s
  buffered=8192 (0.170667s)
  underrun_callbacks=30

18:46:10 Multi dropout:
  122880 zero-fill frames across 30 callbacks
```

その後も `readData blocked for 0.3-1.2s` が継続。`zero/eof/notready/errors` は 0 のため、transport EOF ではなく FMOD `readData()` block が原因。

対応:

- FMOD raw buffer は `163840 bytes` のまま維持する
- decoded PCM target を `65536` から `131072 frames` へ上げる
- startup prebuffer は `32768 frames` のまま

これにより 1-1.6 秒級の `readData()` block を吸収しつつ、`262144 frames` 満杯運用より定常レイテンシーを抑える。

後続の CBR 実測では `131072 frames` でも 3 秒級 block と dropout が残ったため、一旦 `196608 frames` に更新した。ただし `196608 frames` でも 3.8 秒級 block でほぼ枯渇し、さらに `262144 frames` でも dropout が残った。Parcel Music の体感 10 秒程度の遅延は許容範囲と判断し、短期対策値は `kRingFrames=524288`, `kTargetBufferedFrames=393216`, `kPrebufferFrames=393216` に更新する。

### 6.11 2026-06-01 CBR retune verification

`kFmodStreamBufferBytes=163840`, `kTargetBufferedFrames=131072`, `kRingFrames=262144` の arm64 build を起動して CBR 配信を追跡した。

Startup:

```text
18:54:26 Multi source ready
18:54:26 Multi path playing
18:54:27 Multi pump stats:
  reads=20
  frames_written=163840
  buffered=131072 (2.73067s)
  underrun_callbacks=0
  zero=0 eof_zero=0 notready=0 errors=0
```

再生開始自体は短く、定常 target まで素早く積めている。

ただし直後に以下を確認した。

```text
18:54:29 readData blocked for 2.20545s
  buffered_before=122880 frames
  buffered=24576 (0.512s)
  underrun_callbacks=0

18:54:33 readData blocked for 3.10264s
  buffered_before=122880 frames
  buffered=8192 (0.170667s)
  underrun_callbacks=36

18:54:36 Multi dropout:
  147456 zero-fill frames across 36 callbacks
```

その後も `1.47s`, `1.70s`, `1.78s`, `2.50s`, `2.58s` 級の `readData()` block が継続した。全区間で `notready=0`, `zero=0`, `eof_zero=0`, `errors=0`。

判定:

- transport EOF / 0 byte / NOTREADY ではない
- CBR でも FMOD `readData()` がライブ HTTP で数秒 block する
- `131072 frames = 約 2.73 sec` は 1-2.5 秒級は概ね吸収できるが、3 秒超の block は吸収しきれない
- 単に target を増やすと再生開始と定常レイテンシーが伸びるため、品質目標次第で `target/ring capacity` を上げるか、FMOD 同期 `readData()` 依存を減らす設計に進む必要がある

19:00 JST 付近に聴感上怪しい音が出たタイミングでも、同じ傾向を確認した。

```text
19:00:02 readData blocked for 1.6809s
  buffered=53248 (1.10933s)
  underrun_callbacks=5

19:00:06 readData blocked for 2.55948s
  buffered=8192 (0.170667s)
  underrun_callbacks=5

19:00:07 Multi dropout:
  20480 zero-fill frames across 5 callbacks

19:00:13 readData blocked for 2.56741s
  buffered=8192 (0.170667s)
  underrun_callbacks=0
```

この時点でも `notready=0`, `zero=0`, `eof_zero=0`, `errors=0`。したがって、直近の怪しい音も EOF/0 byte ではなく、FMOD `readData()` block による ring 枯渇寄り。

対応:

- 配信を CBR 前提にしても、AYAstorm 側では `readData()` block を主原因として扱う
- `kTargetBufferedFrames` を `262144 frames = 約 5.46 sec` へ上げる。ただし再生開始後の定常遅延は増える
- 根本対策は HTTP/Ogg/Opus 受信と decode を FMOD の同期 `readData()` から分離し、decoded PCM を時間単位 jitter buffer で管理すること

### 6.12 2026-06-01 CBR retune verification at 196608 frames

`kTargetBufferedFrames=196608`, `kRingFrames=262144` の arm64 build を起動して CBR 配信を追跡した。

Startup:

```text
19:04:32 openSourceStream
19:04:36 Multi source ready
19:04:36 Multi path playing
```

ready まで約 4 秒で、startup は維持できている。

ただし、再生開始直後から 3 秒超の `readData()` block が発生した。

```text
19:04:39 readData blocked for 3.07107s
  buffered_before=172032 frames
  buffered=28672 (0.597333s)
  underrun_callbacks=0

19:04:43 readData blocked for 3.65425s
  buffered_before=155648 frames
  buffered=8192 (0.170667s)
  underrun_callbacks=30

19:04:46 Multi dropout:
  122880 zero-fill frames across 30 callbacks
```

その後も `2.8s` と `3.8s` 前後の block を周期的に繰り返した。

```text
19:05:07 readData blocked for 3.87337s buffered=12288
19:05:17 readData blocked for 3.79930s buffered=8192
19:05:24 readData blocked for 3.80098s buffered=8192
19:05:34 readData blocked for 3.87043s buffered=8192
19:05:41 readData blocked for 3.88205s buffered=8192
19:05:51 readData blocked for 3.87782s buffered=12288
```

全区間で `notready=0`, `zero=0`, `eof_zero=0`, `errors=0`。したがって `196608 frames = 約 4.10 sec` でも、FMOD `readData()` block による枯渇リスクは残る。

対応:

- `kTargetBufferedFrames` を `262144 frames = 約 5.46 sec` に上げる
- これ以上の短期対策は ring capacity 自体の増加になる
- 根本対策は FMOD 同期 `readData()` 依存の解消

### 6.13 2026-06-01 CBR retune verification at 262144 frames

`kTargetBufferedFrames=262144`, `kRingFrames=262144` の arm64 build を起動して CBR 配信を追跡した。

Startup:

```text
19:10:52 openSourceStream
19:10:57 Multi source ready
19:10:57 Multi path playing
```

ただし、`262144 frames = 約 5.46 sec` でも `readData()` block と dropout が残った。

```text
19:11:04 readData blocked for 3.6664s
  buffered_before=155648 frames
  buffered=8192 (0.170667s)
  underrun_callbacks=30

19:11:07 Multi dropout:
  122880 zero-fill frames across 30 callbacks
```

その後も 3.8-3.9 秒級の block が継続した。

```text
19:11:11 readData blocked for 3.89715s buffered=8192
19:11:28 readData blocked for 3.87327s buffered=12288
19:11:38 readData blocked for 3.88500s buffered=8192
19:11:45 readData blocked for 3.89665s buffered=8192
19:11:55 readData blocked for 3.80628s buffered=12288
```

全区間で `notready=0`, `zero=0`, `eof_zero=0`, `errors=0`。`target=262144` にしても、実際には `buffered_before=155648-192512` 程度で次の `readData()` block に入り、満杯近くまで安定して積めていない。

対応:

- ring capacity 自体を `524288 frames = 約 10.92 sec` に拡張する
- 定常 target は `393216 frames = 約 8.19 sec`
- startup prebuffer は `393216 frames = 約 8.19 sec`
- この遅延は Parcel Music の体感 10 秒程度と同じ範囲として許容する

### 6.14 5.1 / 7.1 scope

今回の URL Source CBR 修正の対象は 5.1 Ogg/Opus を主経路にする。

現状:

- Ogg/Opus codec plugin 自体は Opus channel mapping family 1 の 1-8ch を decode できる
- URL Source の `LLPositionalStreamMulti` は `1/2/6` のみ受け付け、8ch URL Source は `FormatUnsupported` にする
- MOAP/media ring 側には 8ch 内部 routing がある
- `{ch:BL}` / `{ch:BR}` token は parser と speaker dispatch に存在する

注意:

Opus/Vorbis mapping family 1 の 7.1 order は RFC 7845 上 `FL, C, FR, SL, SR, BL, BR, LFE`。AYAstorm の media ring 内部 8ch order は `FL, FR, C, LFE, SL, SR, BL, BR`。そのため URL Source 7.1 を有効化する場合は、単に `mSourceChannels == 8` を許可するだけでは不可。URL Source decode 後に明示的な reorder table を入れ、L/R/M downmix も 7.1 用に追加する。

結論:

- 今回の CBR 方針版は 5.1 を対象に完成させる
- 7.1 は将来対応として、channel order / downmix / fallback notice / test stream を別作業で追加する
- buffer 設計は frames 基準なので、7.1 化しても時間計算は破綻しない。ただしメモリ量と `readData()` byte count は 8ch 分に増える

### 6.15 将来設計: audio streaming core の共通化

短期対策としては、3D Stream URL Source の decoded PCM ring を大きくし、配信側を CBR 基本方針にする。ただしこれは FMOD 同期 `readData()` block を吸収するための対症療法であり、最終設計ではない。

根本対策を進める場合、Ogg Opus だけを場当たり的に別実装へ切り出すと、Parcel Music と 3D Stream で入力経路が分裂し、以下の保守リスクが増える。

- 同じ URL が Parcel Music では鳴るが 3D Stream では鳴らない、またはその逆が起きる
- reconnect / timeout / redirect / TLS / proxy / ICY metadata の挙動が経路ごとにずれる
- codec 対応表と channel layout 対応表が二重管理になる
- ログ項目と障害切り分け手順が Parcel Music と 3D Stream で分かれる
- 将来 MP3 / AAC / FLAC / 7.1 を追加するたびに片方だけ未対応になる

したがって将来設計では、入力側を `Audio Streaming Core` として共通化し、出力 sink だけを Parcel Music と 3D Stream で分ける。

```text
URL
  ↓
Audio Streaming Core
  - URL resolve / redirect
  - HTTP / Icecast / Shoutcast receive
  - reconnect / timeout / stall detection
  - demux
  - decode
  - channel layout / reorder
  - decoded PCM jitter buffer
  - stats / logging
  ↓
Sink
  - ParcelMusicSink: 2D stereo / parcel music output
  - Stream3DSink: multi-speaker 3D output
```

この形にすれば、CBR / CVBR / VBR の違い、Ogg page size、MP3 frame size、受信 byte 数を再生クロックの基準にしない。共通 core は decoded PCM sample count / buffered ms を基準に状態を持ち、sink はその PCM をどう鳴らすかだけを担当する。

移行順序:

1. 共通 core の interface を先に定義する
   - `start(url)`, `stop()`
   - `readPcm(frames)`, `bufferedFrames()`, `bufferedMs()`
   - `state()`, `lastError()`, `stats()`
   - stream state は bytes ではなく decoded PCM frames を基準にする
2. 最初の利用者は 3D Stream URL Source にする
   - 現在問題が出ているのがここであり、PCM 分配のために FMOD 直 `playSound()` では足りない
   - 既存 Parcel Music を同時に置き換えず、比較対象として残す
3. codec 対応は段階的に載せる
   - まず Ogg Opus / Ogg Vorbis
   - 次に MP3 / Shoutcast
   - 次に AAC / AAC+ / FLAC
4. 3D Stream で十分安定した後、Parcel Music も同じ core の `ParcelMusicSink` へ移行する
   - この段階で入力経路を一本化する
   - FMOD は最終的に出力 mixer / 3D spatialization 側の役割へ寄せる

注意点:

- いきなり Parcel Music まで置き換えない。既存の FMOD 直再生は安定比較用として残す
- ただし設計上は Parcel Music と 3D Stream の入力処理を別物にしない
- 自前 HTTP / demux / decode を持つ場合、壊れた stream、巨大 header、壊れた Ogg page、不正 ICY metadata、TLS/redirect/proxy の扱いを AYAstorm 側で守る必要がある
- CPU 負荷よりも、リアルタイム性、stall isolation、jitter buffer 制御、channel layout の正確性を優先して設計する

将来の channel mapping 検証:

- 今回は 5.1 channel order の実音テストまでは実施しない
- 送信側の制作 / 入力順は一般的に `FL, FR, C, LFE, SL, SR`
- Ogg Opus / Vorbis mapping family 1 の 5.1 decoded order は `FL, C, FR, SL, SR, LFE`
- AYAstorm URL Source は現状 `FL=0, FR=2, C=1, LFE=5, SL=3, SR=4` として読み替える
- SurroundStreamer 側が `5.1(side)` 入力を Opus-compatible `5.1` 出力へ変換していれば、現状の AYAstorm mapping と整合する
- 将来の Audio Streaming Core / 7.1 対応時は、6ch test tone を `FL, FR, C, LFE, SL, SR` の順に送出し、AYAstorm の `{ch:FL}`, `{ch:FR}`, `{ch:C}`, `{ch:LFE}`, `{ch:SL}`, `{ch:SR}` が正しく鳴ることを受入条件に入れる
- 7.1 対応では 5.1 より channel order 差分が大きいため、decode 後の明示的な reorder table と実音テストを必須にする

結論:

短期版は `FMOD createStream + readData()` を維持し、大きめ decoded PCM ring で吸収する。長期版は Parcel Music と 3D Stream の入力を共通 `Audio Streaming Core` に寄せ、出力 sink だけを分ける。3D Stream だけを完全な別経路として固定化するのは避ける。

### 6.16 2026-06-01 CBR verification log at 524288 frames

対象:

- 配信: CBR
- URL: `http://go-stream-live.com:8030/stream`
- codec: Opus
- source format: 6ch / 48000 Hz / PCMFLOAT
- AYAstorm ring: `524288 frames x 6 tracks`
- target: `393216 frames`
- startup prebuffer: `393216 frames`
- log window: `2026-06-01T09:00:00Z` - `2026-06-01T09:15:59Z`

集計:

```text
readData blocked count: 280
min block: 2.18171s
max block: 4.77154s
avg block: 3.25728s
```

代表ログ:

```text
09:00:08 readData blocked 3.88425s, buffered_before=192512 frames
09:04:50 pump stats: readData_max=3.63395s, buffered=8192 frames (0.170667s), underrun_callbacks=24
09:04:53 Multi dropout: 98304 zero-fill frames across 24 callbacks
09:09:32 pump stats: readData_max=3.80524s, buffered=12288 frames (0.256s), underrun_callbacks=6
09:09:34 Multi dropout: 24576 zero-fill frames across 6 callbacks
09:09:44 Multi dropout: 24576 zero-fill frames across 6 callbacks
09:12:23 pump stats: readData_max=4.77154s, buffered=8192 frames (0.170667s), underrun_callbacks=48
09:12:24 Multi dropout: 196608 zero-fill frames across 48 callbacks
09:15:52 readData blocked 3.14622s, buffered_before=221184 frames
```

重要な点:

- `readData()` は `rr=No errors`, `read_bytes=196608`, `want_bytes=196608` で戻っている
- `notready=0`, `zero=0`, `eof_zero=0`, `errors=0`
- つまり CBR でも transport EOF / Ogg starvation / 0 byte read ではない
- 問題は FMOD `readData()` が同期的に 2.8-4.8 秒 block し、その間に decoded PCM ring が 8192-12288 frames まで落ちること
- `524288 frames` ring は容量としては大きいが、現行 pump が `readData()` block に捕まるため、target 近くまで安定して積めていない
- このログは「CBR 方針は正しいが、FMOD `readData()` 依存を残す限り品質目標に届かない」根拠になる
- この結果を受け、起動直後の低水位再生を避けるため startup prebuffer も `393216 frames` へ引き上げる

判断:

- 配信側 CBR は継続する
- 短期実装の詳細診断ログは削除する
- この CBR 実ログは将来の `Audio Streaming Core` / 自前受信 decode pipeline を検討する根拠として残す

### 6.17 ayastorm-release 比の 3D Stream 負荷見積もり

比較対象:

```text
ayastorm-release:
  kRingFrames      = 32768 frames
  kPrebufferFrames = 4096 frames
  kTargetBufferedFrames = none
  kFmodStreamBufferBytes = none in 3D Stream URL Source

current branch:
  kRingFrames      = 524288 frames
  kPrebufferFrames = 393216 frames
  kTargetBufferedFrames = 393216 frames
  kFmodStreamBufferBytes = 163840 bytes
```

メモリ:

| source layout | ayastorm-release ring | current ring | 差分 |
|---|---:|---:|---:|
| 2ch PCMFLOAT | 約 256 KiB | 約 4 MiB | 約 +3.75 MiB / URL Source |
| 6ch PCMFLOAT | 約 768 KiB | 約 12 MiB | 約 +11.25 MiB / URL Source |
| 8ch PCMFLOAT 将来対応 | 約 1 MiB | 約 16 MiB | 約 +15 MiB / URL Source |

FMOD raw compressed buffer は `163840 bytes` なので、decoded PCM ring と比べれば小さい。3D Stream URL Source 1本では許容範囲だが、同時 URL Source 数が増えると ring memory は線形に増える。6ch URL Source を 5本同時に開くと、旧比で約 `56 MiB` 増える計算になる。

CPU:

- 定常時の decode 量は ayastorm-release とほぼ同じ。最終的に再生する PCM frames/sec は変わらない
- `kPumpChunkFrames = 1024` と `kMaxFramesPerPump = 8192` は変えていないため、1回の変換 loop 粒度は同じ
- 現行は `kTargetBufferedFrames` まで積んだら追加 read を止めるため、常時 524288 frames 全体を走査するわけではない
- 起動時は `4096 frames` ではなく `393216 frames` まで積むため、再生開始前の decode / copy / downmix 準備量は増える。ただしライブ HTTP では受信待ちが支配的で、CPU スパイクより開始待ちの増加として見える可能性が高い
- `readData()` block 中は CPU を使っているのではなく待たされている状態なので、今回の対策は CPU 負荷増よりも latency / memory 増の性格が強い

メモリ帯域 / cache:

- ring が大きくなるため working set は増える
- ただし read/write は基本的に連続アクセスで、毎 callback 全 ring を走査しない
- 6ch direct routing の per-callback 処理量は再生 frames 数に比例し、ring capacity には比例しない
- したがって cache 面の悪化はあり得るが、主要リスクは CPU ではなくメモリ消費と起動遅延

レイテンシー:

- ayastorm-release は `4096 frames = 約 85 ms` で再生開始できた
- current は `393216 frames = 約 8.19 sec` を起動前に要求する
- 体感差として最も大きいのはここ。安定性のために、Parcel Music 相当の「数秒から 10 秒程度の待ち」を許容する方針

MOAP / MediaRing への影響:

- MediaRing source は `kMediaPrebufferFrames`, `kMediaTargetBufferedFrames`, `kMediaRingFrames` を使うため、今回の URL Source ring 拡張の対象外
- したがって MOAP 3D redirect の低遅延設計には直接影響しない

判断:

- 3D Stream URL Source 1本の通常利用では、負荷増は許容範囲
- 6ch URL Source の多重同時再生ではメモリ増が無視できないため、将来的には同時 URL Source 数の上限、または inactive source の早期解放を検討する
- CPU が問題になる可能性は現時点では低い。今回の実ログで問題になっているのは CPU 飽和ではなく、FMOD `readData()` の同期 block と ring 枯渇
- ayastorm-release 比で犠牲にしているのは、主に起動レイテンシーと ring memory。得ているものは、CBR 配信時の数秒級 `readData()` block への耐性

### 6.18 MOAP / Dullahan / CEF 経由で同じ現象が出にくい理由

MOAP source の 3D Stream redirect は、3D Stream URL Source と入力経路が違う。

URL Source:

```text
HTTP URL
  -> FMOD createStream
  -> FMOD / Ogg codec
  -> LLPositionalStreamMulti::pumpSource()
  -> FMOD::Sound::readData()
  -> decoded PCM ring
  -> per-speaker FMOD OPENUSER sounds
```

MOAP / CEF source:

```text
Dullahan / CEF media pipeline
  -> CEF audio callback
  -> plugin shared-memory audio ring
  -> LLPositionalStreamMulti::pumpMediaRingSource()
  -> decoded PCM ring
  -> per-speaker FMOD OPENUSER sounds
```

根拠:

- `LLPluginClassMedia::ensureAudioSharedMemory()` は media plugin 用の audio shared memory を作り、`audio_shm_set` で CEF plugin へ渡す
- `MediaPluginCEF::onAudioStreamStartedCallback()` は Dullahan の audio stream format を受け、shared memory ring の sample rate / channels / float format を設定する
- `MediaPluginCEF::onAudioStreamPacketCallback()` は Dullahan / CEF から来た audio packet を `writeAudioPacketToRing()` で shared memory ring へ書く
- `LLViewerMediaImpl::getAudioRingForStream3D()` はその shared memory ring を 3D Stream manager へ渡す
- `LLPositionalStreamMulti::pumpSource()` は URL Source の場合だけ `mSourceSound->readData()` を呼ぶ
- `LLPositionalStreamMulti::pumpSource()` は `SourceKind::MediaRing` の場合、即 `pumpMediaRingSource()` へ分岐する
- `pumpMediaRingSource()` は shared memory ring の `mWriteFrame` / `mReadFrame` 差分から利用可能な decoded PCM frames を読み、FMOD `readData()` を呼ばない

したがって、今回 CBR 実ログで確認した「FMOD `readData()` が 2.8-4.8 秒同期 block し、その間に 3D Stream URL Source の decoded PCM ring が枯れる」現象は、MOAP / Dullahan / CEF 経由の 3D redirect には同じ形では発生しない。MOAP 側では CBR / CVBR / VBR、Ogg page size、HTTP compressed byte 数、`FMOD_ERR_FILE_EOF + read_bytes == 0` は 3D Stream 側の直接入力ではなく、CEF がすでに decode した PCM frame の増減として見える。

注意:

- MOAP が絶対に音切れしないという意味ではない
- CEF media pipeline 側のネットワーク stall、JavaScript player の停止、タブ / priority / autoplay policy、plugin process 停止、shared memory ring 消失では別の音切れは起こり得る
- ただしその場合の原因は `FMOD::Sound::readData()` block ではなく、CEF audio callback から shared memory ring へ decoded PCM が供給されないこと
- MediaRing source は `kMediaPrebufferFrames`, `kMediaTargetBufferedFrames`, `kMediaRingFrames` を使うため、今回の URL Source ring 拡張とは別設計で動く

## 7. 受入条件

### 7.1 動作確認

- Ogg Opus URL Source で `read_bytes == 0` が発生しても、ring に 0.2 秒超の PCM が残っている時は reconnect しない
- `zero-fill streak` に到達する前に、empty ring + underrun の条件で reconnect が走る
- VBR 無音区間で Ogg EOS 未検出の 0 byte EOF を即 EOF として扱わない
- `FMOD_ERR_NOTREADY` は一時 starvation として扱い、empty ring + underrun + 0.50 秒継続で reconnect へ進む
- 詳細な切り分けが必要な場合のみ、`0.8` の検証用ログを一時的に戻す

### 7.2 聴感確認

- サーバー正常時に最大 10 秒の無音が再現しないこと。2026-06-01T09:00-09:15Z の CBR 実ログでは dropout が残っているため、この条件は未達として扱う
- 短い jitter で過剰 reconnect しない
- reconnect 発生時、残ringを不必要に捨てたことによる余計な音切れが増えない

### 7.3 回帰確認

- Ogg Opus mono / stereo / 5.1ch URL Source
- Ogg Vorbis URL Source
- MP3/AAC 等の非 Ogg URL Source
- MOAP/media source 3D redirect
- Parcel Music

## 8. 判断ポイント

この修正で見るべき核心は以下。

1. `read_bytes == 0` が起きた時、ring に何秒分残っているか
2. ring が枯れた後、現行の 10 秒待機が無音を作っているか
3. Ogg Opus codec が PCM を返せない状態を EOF/error として正しく上げているか
4. ring 容量不足なのか、zero-byte policy が悪いのかを再検証用ログで分離できるか

結論として、`read_bytes == 0` / `FMOD_ERR_NOTREADY` の扱いは ring 残量・underrun・FMOD state と結びつける必要がある。2026-06-01 の実測では通常音量時に EOF ではない 3-4 秒級の `readData()` block が発生しており、旧 URL Source ring `32768 frames` は明確に不足していた。現在は URL Source ring を `524288 frames` へ拡張し、定常 target を `393216 frames`、startup prebuffer も `393216 frames` とする。Ogg EOS 未検出の VBR starvation は NOTREADY として扱い、empty ring + underrun 後は `0.50 sec` で reconnect へ進める。
