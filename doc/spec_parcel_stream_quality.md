# 土地音 (parcel music) 再生品質改善 仕様書 (r12.1)

> **対象**: AYAstorm `v7.2.5-ayastorm-r12.1` (想定)
> **前提**: なし (LL FS 標準の parcel music path に対する改修)
> **roadmap 整合**: AYAstorm 3dstream とは独立した小機能。AYAstream / 5.1 placement / venue reverb のスコープには含まれない

## 1. 目的とスコープ

SL の **土地音 (parcel music)** = `LLStreamingAudio_FMODSTUDIO` 経由で再生される parcel 設定の HTTP stream に対し、配信元 bitrate が高い (256/320 kbps mp3, FLAC-over-HTTP) 環境で発生していた **buffer starvation による pause/resume cycling** を解消する。あわせて、典型的な 128 kbps mp3 配信で痩せがちな高域 (presence band) を viewer 側で僅かに補正し、parcel music の体感品質を底上げする。

LL FS 既定値は **完全に変えない** (default=0 で従来挙動を bit-identical に維持)。debug setting `FSParcelStreamQuality=1` を選んだユーザーだけが AYAstorm 拡張パスに切り替わる **opt-in モデル**。

主目的:

- 高 bitrate parcel stream (256/320 kbps mp3, FLAC-over-HTTP) の **buffer starvation を解消** (本機能の主目的)
- 典型 128 kbps mp3 で削れる **presence band を僅かに補正** (副次効果、副作用なし)
- LL FS の挙動・音量感・音色は **default で完全維持** (= 既存ユーザーへの影響ゼロ)
- viewer 起動中に live で 0 ↔ 1 切替可能 (resampler のみ再起動必須)

非対象 (本仕様では触らない):

- FMOD output sample rate / DSP buffer size のチューニング
- mp3 decoder の追加 hint (codec 内部パラメータ)
- 配信元側 bitrate / mixing 品質改善 (viewer 外)
- parcel music への 3D positioning / binaural / venue reverb 適用 (parcel music は仕様上 2D)
- gain curve の linear 化 (FMOD 標準 `vol*vol` を perceptual approximation として維持)

## 2. 背景

### 2.1 LL FS 標準実装の現状

`LLStreamingAudio_FMODSTUDIO` は parcel music 用の HTTP stream player で、以下のチェインで動作する:

```
HTTP source -> FMOD::Sound (stream mode) -> mStreamGroup -> master mixer -> output
```

LL FS 標準では:

| 項目 | LL FS 既定 | 由来 |
|---|---|---|
| stream buffer hint | 128 kbps × 10 s 想定 | `setStreamBufferSize` で固定 |
| resampler | `FMOD_DSP_RESAMPLER_LINEAR` | `FMODResampleMethod=0` debug setting に従う |
| gain curve | `vol * vol` | FMOD 標準の perceptual approximation |
| DSP chain | なし | `mStreamGroup` に attach なし |

### 2.2 観測された劣化シナリオ

1. **高 bitrate stream で starvation**: 256/320 kbps mp3 や FLAC-over-HTTP の parcel music で buffer (128 kbps × 10 s = ~160 KB) が不足し、pause/resume cycling が発生
2. **44.1 ↔ 48 kHz の SRC で HF aliasing**: LINEAR resampler は最も軽量だが折り返しノイズが出る (= 「ザラつき」)。客観 FFT では確認できるが、人間の主観 ABX では 「相当耳が良くないと聞き分け不可」レベル
3. **128 kbps mp3 の presence 帯域削れ**: 配信側 mp3 エンコードで 6〜10 kHz 周辺が僅かに痩せ、「曇った音」の主観につながる

(2) と (3) は viewer 側で完全には直せない (元データ起因) が、**(1) は viewer 側で確実に解消できる**。これが本機能の主目的。

### 2.3 設計判断 — gain curve は触らない

調査初期に「`vol * vol` の squared gain curve を linear (`vol`) に変えれば低スライダー位置の SNR が改善する」と提案したが、再評価の結果これは **改善ではなく単なる音量増し** と判明したため不採用。

- `vol * vol` は FMOD 標準の **対数聴覚応答に合わせた perceptual curve**
- linear に変えるとスライダー 0.5 で出力 0.5 となり「同位置で 2 倍音量」に聞こえるが、これは loudness 差が出るだけで信号そのものは改善していない
- ヘッドルーム喪失 (clipping 入りやすくなる) のデメリットあり
- 主観で「クリアになった」と感じても loudness-matched A/B では差が消える

よって gain curve は **default 0 / quality=1 とも `vol * vol` を維持**。

## 3. 設計

### 3.1 単一 toggle に集約

3 つの修正を **`FSParcelStreamQuality` U32 debug setting 1 つ** に束ねる:

| 値 | 名称 | 内訳 |
|---|---|---|
| 0 | Original FS path | LINEAR resampler / 128 kbps buffer hint / no DSP (= LL FS 既定、bit-identical) |
| 1 | AYAstorm enhanced | SPLINE resampler / 320 kbps buffer hint / +4 dB high-shelf @ 6 kHz |

複数の独立 toggle に分けない理由:

- 「buffer 改善のみ」「EQ のみ」を選ばせる UX 価値が薄い (3 つは独立に有意)
- AYAstorm の流儀として「設定は最小限・無難なデフォルト優先」(= 配信者主導モデル / sentinel 設計と同じ)
- 後で個別 toggle が必要になればその時点で分割すればよい (現時点で split しない)

### 3.2 反映タイミング

| 修正 | 切替タイミング | 理由 |
|---|---|---|
| **buffer hint** (128 → 320 kbps) | 次 stream 開始時 | `setStreamBufferSize` は次 `createStream` まで反映されない (FMOD API 仕様) |
| **EQ filter type** (DISABLED → HIGHSHELF) | live (即時) | DSP は always attach、`setParameterInt(A_FILTER, ...)` で type 差し替えのみ |
| **resampler** (LINEAR → SPLINE) | viewer 再起動 | `FMOD_ADVANCEDSETTINGS::resamplerMethod` は `System::init` 時に固定 |

debug setting 変更後の挙動 (再起動なし):
- EQ: 即時切替 (live で「曇り」の改善 / 復元を確認可能)
- buffer: 次 parcel 切替や stop/start で反映 (現再生 stream には影響しない)
- resampler: 反映されない (再起動が必要)

設定の Comment にこの区分を明示する。

### 3.3 EQ パラメータの根拠

| 項目 | 値 | 根拠 |
|---|---|---|
| filter type | `FMOD_DSP_MULTIBAND_EQ_FILTER_HIGHSHELF` | shelf は周波数より上を等しく持ち上げる、peak より副作用が出にくい |
| frequency | 6 kHz | 8 kHz (air band) より下、5 kHz より上で「presence」(楽器/ボーカルの輪郭) に当たる帯域 |
| gain | +4 dB | チューニング検証で「ちょっと効いている」と知覚できる最小ゲイン。+5 dB / +6 dB は副作用 (シビランス) リスクが上回ると判断 |
| Q | n/a | shelf には Q 不使用 |

検証経緯で試した代替案 (8 kHz +3 dB / 6 kHz +5 dB / 400 Hz peaking dip / 500 Hz peaking dip) は、いずれも「明確な改善が得られない」「耳の順応で判断不能になる」「副作用が累積する」のいずれかで不採用。**1 band のシンプル構成が最も再現性のある改善** という結論。

### 3.4 quality=0 の bit-identical 性

quality=0 でも EQ DSP は **mStreamGroup に attach されたまま** (DISABLED filter として pass-through)。これは:

- live 切替を簡素化 (DSP の create/destroy を都度行わない)
- 性能影響は「DSP node を 1 つ経由する」のみで実測差ゼロ

DISABLED filter は FMOD 仕様上 pass-through (信号無加工) なので、出力 amplitude / phase は LL FS 既定と完全一致する。

## 4. 実装

### 4.1 ファイル構成

| ファイル | 役割 |
|---|---|
| `indra/llaudio/llstreamingaudio_fmodstudio.h` | `setQuality(U32)` setter / `applyStreamBufferSize()` / `applyStreamEq()` / `mQuality` / `mStreamEqDsp` 宣言 |
| `indra/llaudio/llstreamingaudio_fmodstudio.cpp` | DSP create/attach (ctor) / DSP detach/release (dtor) / EQ filter type 切替 / buffer hint 切替 |
| `indra/llaudio/llaudioengine_fmodstudio.h` | ctor 第 3 引数 (`parcel_stream_quality`) 追加 / `setParcelStreamQuality(U32)` setter |
| `indra/llaudio/llaudioengine_fmodstudio.cpp` | ctor で quality を保持 / `init()` で resampler 選択分岐 / `createDefaultStreamingAudioImpl()` で初期 quality を impl に伝搬 / live setter |
| `indra/newview/llstartup.cpp` | engine ctor に `gSavedSettings.getU32("FSParcelStreamQuality")` を渡す |
| `indra/newview/llviewercontrol.cpp` | `FSParcelStreamQuality` 変更時の live callback を `setParcelStreamQuality()` 経由で発火 |
| `indra/newview/app_settings/settings.xml` | `FSParcelStreamQuality` U32 (default 0) を追加 |

### 4.2 DSP graph

```
HTTP source -> FMOD::Sound (stream) -> mStreamGroup
                                          |
                                          + MULTIBAND_EQ DSP (band A only)
                                          |    quality=0: A_FILTER=DISABLED (pass-through)
                                          |    quality=1: A_FILTER=HIGHSHELF, freq=6kHz, gain=+4dB
                                          v
                                        master mixer -> output
```

EQ DSP は `mStreamGroup->addDSP(0, ...)` で attach。`removeDSP` + `release` は dtor で実施。

### 4.3 live callback

`llviewercontrol.cpp` に handler:

```cpp
static void handleParcelStreamQualityChanged(const LLSD& newvalue)
{
#ifdef LL_FMODSTUDIO
    if (LLAudioEngine_FMODSTUDIO* fmod_engine =
            dynamic_cast<LLAudioEngine_FMODSTUDIO*>(gAudiop))
    {
        fmod_engine->setParcelStreamQuality(static_cast<U32>(newvalue.asInteger()));
    }
#endif
}
```

`#ifdef LL_FMODSTUDIO` で OpenAL ビルド (FMOD 非搭載) を保護。

## 5. 性能影響

| 変更 | CPU | メモリ |
|---|---|---|
| buffer hint 320 kbps | ゼロ | +240 KB 程度 |
| SPLINE resampler | < 0.1% (FMOD SIMD 最適化、SRC は stream 1 本のみ) | ほぼゼロ |
| MULTIBAND_EQ band A | < 0.05% (IIR biquad 1 段 stereo) | ほぼゼロ |
| **合計** | **< 0.2%** (parcel music 再生時のみ) | **< 0.5 MB** |

quality=0 でも DSP node を 1 つ経由するが、DISABLED filter のため処理オーバーヘッドは < 0.01% レベル。実害なし。

低性能 CPU (Atom 等) でも実用上問題なし、レイテンシ増分も BGM 用途では無視可能。

## 6. FS 互換性 / 既知の制約

### 6.1 後方互換性

- `FSParcelStreamQuality=0` (default) は LL FS 既定の挙動と **bit-identical**
- 既存ユーザー (debug setting 未変更) への影響なし
- engine ctor 第 3 引数は `= 0` の default 引数で追加、既存呼び出し箇所 1 つだけ更新

### 6.2 既知の制約

- **resampler は live 切替不可**: `FMOD_ADVANCEDSETTINGS::resamplerMethod` は `System::init` 時に固定。値を変えても効果が出るのは次回 viewer 起動時。Comment にこの旨明記
- **EQ パラメータは固定**: 6 kHz / +4 dB / shelf は決め打ち。配信元の特性による微調整は不可。これは「設定は最小限・無難なデフォルト優先」の方針に沿う
- **OpenAL ビルドでは無効**: `setParcelStreamQuality` は `LL_FMODSTUDIO` ビルドのみ。OpenAL 経路には適用されない
- **`FMODResampleMethod` との関係**: quality=1 のとき `FMODResampleMethod` の値は無視され SPLINE に強制。quality=0 のときのみ `FMODResampleMethod` が効く

### 6.3 非実装 (将来候補)

- Preferences UI 露出: 現時点では debug setting のみ。配信者 / 一般 listener 双方への露出価値が確認されていない
- gain curve の選択肢提供: 上記 §2.3 の理由で見送り
- 別チューニング preset (例: vocal-focused / EDM-focused 等の EQ プリセット): 設定の複雑化を避ける

## 7. 検証経緯

実 listening 検証で以下が確認された:

1. **buffer hint 320 kbps 化**: 高 bitrate stream の dropout 解消は理論上明らか、客観効果として採用
2. **SPLINE vs LINEAR**: 主観 ABX では「相当耳が良くないと聞き分け不可」(検証時の AYA 観察)。理論上の HF aliasing 軽減は客観的事実として残るので採用
3. **6 kHz +4 dB shelf EQ**: 「ちょっと効いている」レベルの subtle improvement。検証中に試した代替 (8 kHz +3 dB / 6 kHz +5 dB / 中域 dip 併用) より副作用が少なく、最も再現性が高かった構成
4. **gain curve 変更案**: loudness-matched 比較で「単なる音量増し」と判明、不採用 (上記 §2.3)

「曇った音」を完全に解消することは viewer 側のみでは不可能 (配信元側の bitrate / mixing が支配的) と確認されたが、**subtle improvement + buffer 改善** という現実的な落としどころに着地した。

## 8. 関連リンク

- 設定キー: `FSParcelStreamQuality` (`indra/newview/app_settings/settings.xml`)
- 既存関連 setting: `FMODResampleMethod` (quality=0 のときのみ有効)
- 実装記録 (commit): `9ec02b7a2f` (r12.1 同梱)
