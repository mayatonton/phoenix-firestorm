# stereo→5.1 upmix 仕様書 (r12)

> **対象**: AYAstorm `v7.2.5-ayastorm-r12` (想定)
> **前提**: `doc/spec_5_1ch_placement.md` (r10 5.1ch placement 仕様)
> **前提**: `doc/spec_5_1ch_source.md` (r9 5.1ch ソース受入)
> **前提**: `doc/spec_binaural_venue_reverb.md` (r11 lite-HRTF + venue convolution reverb、配信者主導モデル)
> **背景文書**: `docs/ayastorm-positional-stream.md` (M1〜Post-M9 実装記録)
> **roadmap 整合**: `docs/ayastorm-stream3d-roadmap.md` §3 r12 (本書策定で「stereo→5.1 upmix のみ」に確定、SOFA / Steam Audio 等は r13+ に降格)

## 1. 目的とスコープ

r12 は **r10 で完成した 6 spk placement (Layer 1) の体験を、現実に世間で行われている stereo 配信にも届ける** リリース。

r10 で 6 spk per-channel placement を、r11 で venue convolution reverb と lite-HRTF を載せた結果、AYAstorm は「5.1 配信を 6 prim に配置 → venue reverb → lite-HRTF」のフルチェインで「ホール体験」を成立させられる。**ただしこの体験は 5.1 配信 (= 6ch source) を前提としており、世間の SL 配信ほぼ全部を占める stereo 配信では 6 spk placement の実力が発揮されない**。最初に AYAstorm を試す配信者・listener は間違いなく stereo 配信から入るので、ここで「6 spk 配置を体感できるか」が体験品質の分岐点になる。

r12 はこの分岐点を埋める。viewer 内 DSP として **stereo→5.1 upmix (matrix decode + 帯域分離)** を実装し、stereo 配信を 6ch 化して r10 の per-channel placement に流す。これにより:

- stereo 配信でも 6 spk placement の効果が出る (= 機会損失の回避)
- 5.1 配信は従来どおり (= upmix は明示的に bypass)
- 配信者は何もしなくても恩恵を受けられる (default off だが LSL menu から 1 操作で on にできる)

主目的:

- stereo 配信を viewer 内で 5.1 化し、r10 6 spk placement の体験を全配信に届ける
- 5.1 native 配信 (source ch>=6) は upmix を自動 bypass、二重処理を防止
- 配信者主導モデル (r11) を維持: 制御は配信者プリム Desc タグ (`{upmix:on|off}`) で完結、listener 側 Preferences UI 改修ゼロ
- アルゴリズムは決め打ち (DPL2 系 matrix decode + 帯域分離) で「配信者は on/off だけ判断」できる UX を維持

**設計思想 — 配信者主導モデルの維持**:

r11 で確立した「配信者がスピーカープリム Desc に書いた表現意図を listener viewer は忠実にレンダリングする」モデルを継承する。upmix 制御も `{upmix:on|off}` タグで完結し、listener 側の Preferences UI 経由の override は提供しない。実装/検証用の debug settings (sentinel = 「タグ通り」default) のみ残す。

**アルゴリズム多択化はしない**: 「DPL2 / Logic7 / band-steering どれを使うか」を配信者に選ばせない。配信者の選択は `{upmix:on|off}` のみで、ある音 = 再現される音を担保する。微調整 (LFE cutoff / center bleed / rear delay) は listener 側 debug settings に出すが、配信者タグには出さない。これは r11 の `Stream3DBinauralRender` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` と同じ流儀。

**過去仕様との関係**:

- `doc/spec_5_1ch_placement.md` (r10) で確立した per-channel placement の経路は本書でも維持。upmix DSP は **6ch 出力を生成して既存 r10 placement の入力にそのまま流す** 構造で挿入する (= placement 側に変更ゼロ)
- `doc/spec_binaural_venue_reverb.md` (r11) の lite-HRTF / venue reverb DSP chain との順序は §4.2.3 で明記
- `docs/ayastorm-stream3d-roadmap.md` §3 r12 (旧 SOFA per-source HRTF + Steam Audio) の項目は本書策定で再定義され、SOFA / Steam Audio / VenueReverb CPU 最適化 / air absorption 客観測定 / 個人 HRTF measurement / 公開 README は **r13+ へ降格**。降格理由は §2.2 で詳述

非対象 (r12 では触らない):

- アルゴリズムの多択化 (DPL2 系決め打ち、Logic7 / SRS / 周波数帯別 steering 等は r13+)
- 動的 steering (入力解析に基づくマトリックス係数の時変調整)
- ML / Spatial Audio AI 系 upmix
- 5.1 native 配信への upmix 適用 (source ch>=6 で auto bypass)
- 配信者向けの細かいパラメータ調整タグ (LFE cutoff / center bleed 等)
- listener 側 Preferences UI 設定
- SOFA per-source HRTF — r13+
- Steam Audio integration — r13+
- VenueReverb CPU 最適化 (NUPC) — r13+
- air absorption 客観 FFT 測定 — r13+
- 公開 README / changelog 一括開示 — r13+ (機能成熟後)

---

## 2. 問題定義

### 2.1 r11 までの体感の限界

r11 完了時点で AYAstorm は **5.1 配信を前提とした** 「6 spk placement + venue reverb + lite-HRTF」のフルチェインを持つ。受入指標 (r11 §5.5 / §13) はすべて達成しているが、**5.1 配信そのものが SL ではほぼ存在しない** という現実がある:

- 一般的な配信ソフト (butt / Mixxx / OBS Audio Output / SAM Broadcaster 等) は **stereo までしか対応していない** ものが多数派
- Icecast 2.4+ + Opus surround の 5.1 配信構成は技術的には可能だが、配信者側の知識と機材コストが高い
- AYAstorm を初めて試す配信者は **まず手元の stereo 配信ソフトで投げる** のが自然な導線
- その瞬間 listener viewer の 6 spk placement は、stereo source の左右 2 prim にしか流れず、残り 4 prim (C / Ls / Rs / LFE) は無音
- → r10 で作った 6 spk placement の効果が体感できない (= 機会損失)

これは技術的な不足ではなく、**「配信エコシステムと viewer の前提のズレ」** が原因。viewer 側で stereo→5.1 upmix を備えれば、配信エコシステムの現状を変えずに 6 spk placement の体験を全配信に届けられる。

### 2.2 旧 r12 計画 (SOFA / Steam Audio) を採らなかった理由

旧 roadmap (§3 r12) は **SOFA per-source HRTF + Steam Audio integration** を r12 メインに想定していた。本書策定で再評価した結果、以下の理由で r12 メインから外し r13+ へ降格する:

- **配布負債が大きい**: SOFA HRTF DSP の C++ 実装に加え、KU100 等の個人 SOFA は **再配布制限** がある可能性が高く、viewer 同梱の可否を要調査。Steam Audio は Win/Mac/Linux 3 platform で binary build / 同梱 / 検証が必要 (roadmap RR1 で「Linux が鬼門」と既知)
- **r11 lite-HRTF で頭追従感は既に取れている**: SOFA はその上の「個人化」で、限界効用が r11 lite-HRTF より小さい。**非個人 SOFA は逆効果** (前後/上下の曖昧化を増やす) という既知問題もあり (RR2)、ベタな KU100 デフォルトで全 listener を満足させられない
- **Steam Audio は SL 世界 mesh を食わせる経路が未整備**: occlusion / geometry-based reflection は機能としては魅力だが、SL の prim/mesh をリアルタイムで Steam Audio に渡す経路を新設しないと半分しか活きない
- **stereo upmix は配布負債ゼロ + 既存配信全部に効く**: viewer 内 DSP 完結 (新 binary 不要)、既存配置 (r8/r10 で置かれた全 prim) も再配置不要、SL 配信文化の現状そのものを救う ROI が大きい
- **r10/r11 投資の元を取る**: r10 で作った 6 spk placement と r11 の venue reverb + lite-HRTF が、stereo 配信でも全部活きる状態にするのが、これまでの投資への最大 ROI

旧 r12 計画は捨てるのではなく r13+ への降格とする (§9 で詳述)。

### 2.3 アルゴリズムを決め打ちする根拠

stereo→5.1 upmix のアルゴリズムは複数の選択肢がある (DPL2 系 matrix decode、Logic 7 系 multi-band steering、SRS Circle Surround 系の心理音響モデル、ML 系 upmix 等)。本書では **DPL2 系 matrix decode + 帯域分離 (LFE LPF / center bleed 除去 / rear decorrelation)** を採用し、決め打ちする。理由:

- 配信者がアルゴリズムを選ぶタグまで増やすと「DPL2 / Logic7 / band-steering どれが正解?」と迷わせる。配信者主導モデルでは「配信者が表現意図を確定的に指定できる」ことが価値で、アルゴリズム選択肢は表現の不確定性を増やす方向に働く
- listener 側に切替 UI を出すと r11 までの「listener UI 改修ゼロ」design philosophy が崩れる
- 「ある音 = 再現される音」になる方が配信者にも listener にも認知負荷が低い
- DPL2 系は古典的で実装が確立しており、phase 依存性こそあるが SL 配信音源 (一般的なステレオ音楽 / トーク) では十分機能する。動的 steering (Logic 7 系) は実装コストが大きく、SL 配信の音源特性 (急峻な phase 変化が少ない) を考えると ROI が低い
- ML 系は CPU / モデル配布 / 推論レイテンシで配布負債が深刻

将来アルゴリズムを差し替えたく / 増やしたくなったら、そのタイミングで判断 (= r13+ 持ち越し)。最初から多択にしないのが r5 / r11 の流儀でもある。

### 2.4 r13 着手前にやっておきたいこと

r12 で済ませておくと r13+ の SOFA / Steam Audio 着手時に手戻りが少ない事項:

- upmix DSP の挿入位置は **r10 placement DSP の前段** で確立 (= source ch 変換は placement より前で完結)。これにより r13+ で SOFA を導入しても upmix → SOFA の順序で素直に積める
- source ch 数の判定経路 (r9 codec layer) を upmix 側でも参照、auto bypass の判定材料に使う。r13+ で「ch 数依存の DSP 挿入分岐」を増やす場合に同じ経路が流用できる
- 配信者主導モデルの拡張パターン (= 新タグキー追加 + sentinel 付き debug setting + LSL UI 拡張) を r12 で踏襲することで、r13+ で SOFA タグ (`{binaural:lite|sofa|off}` 等) を導入する際のテンプレートが固まる

---

## 3. ゴール / 非ゴール

### ゴール

- G1. **stereo→5.1 upmix DSP 実装**: 2ch input から 6ch output (L/R/C/Ls/Rs/LFE) を DPL2 系 matrix decode + 帯域分離で生成
- G2. **配信者タグ `{upmix:on|off}` 追加**: default `off`。配信者が明示的に on にすることで upmix 有効化
- G3. **5.1 native 配信の auto bypass**: source ch>=6 のときは `{upmix:on}` であっても upmix を自動 bypass、二重処理を防止
- G4. **debug settings 経由の listener 側微調整**: LFE cutoff / center bleed / rear decorrelation 量を debug settings で調整可。default はアルゴリズム標準値 (= タグ通り sentinel ではなく実値 default)
- G5. **配信者 LSL に Upmix UI 追加**: r11 の binaural / venue / wetgain と同じ流儀で `{upmix:on|off}` を menu / dialog から設定可能
- G6. **既存配置の自動恩恵**: r8 / r10 で過去に置かれた全 prim は、配信者が `{upmix:on}` を明示的に追加した瞬間から 6 spk placement の体験を得る (再配置不要)
- G7. **r10 / r11 受入条件すべて維持**: dropout / CPU / URL 切替 / 互換マトリクス / 回帰、すべて r11 から劣化なし
- G8. **r13+ で SOFA per-source HRTF を載せる場合の hook point** を仕様書とコードに明記

### 非ゴール

- NG1. **アルゴリズム多択化**: DPL2 系決め打ち、Logic 7 / SRS / 周波数帯別 steering 等は配信者にも listener にも選ばせない (r13+ で再検討)
- NG2. **配信者向けの細かいパラメータ調整タグ**: LFE cutoff / center bleed / rear delay は配信者タグに出さない (debug settings のみ)
- NG3. **動的 steering** (時変マトリックス係数): 入力解析に基づくリアルタイム重み付け。r13+ で再検討
- NG4. **ML / Spatial Audio AI 系 upmix**: r13+ で外部依存込みで再検討
- NG5. **5.1 native 配信への upmix 適用**: source ch>=6 で auto bypass 強制。配信者が `{upmix:on}` を誤って付けても二重処理しない
- NG6. **listener 側 UI 設定 / Preferences > Sound タブ追加**: 配信者主導モデルを維持、debug settings のみ
- NG7. **公開 README / changelog 開示**: 機能成熟後 r13+ で一括 (r11 NG9 と同じ方針)

---

## 4. 設計

### 4.1 タグ書式 — `upmix` キー追加

#### 4.1.0 新規タグキー一覧

| キー | 値 | default | 効果 |
|---|---|---|---|
| `upmix` | `on` / `off` | `off` | `on` のとき viewer 側で stereo→5.1 upmix を有効化。source ch>=6 では本タグ値に関わらず auto bypass |

書式は r11 までと同様、親プリム Desc に `[3dstream-stereo:{upmix:on}]` のように追加。r11 までの既存タグ (`{venue}` / `{binaural}` / `{wetgain}` / `{ch}` / `{range}` / `{volume}`) と並列指定可。

#### 4.1.1 upmix 許容値

許容値 (大文字小文字区別なし、parser は `LLStringUtil::toLower` で正規化):

- `on`: stereo→5.1 upmix を有効化 (source ch=2 の stream に対して DSP 挿入)
- `off`: upmix 無効 (default)。stereo source は r10 までと同じく 2ch そのまま per-channel placement (= mono ch / L+R 振り分け) に流れる

不正値 (上記以外) は **silent ignore + chat 通知 1 回** (r11 の不正タグ通知 throttle 機構を流用)。

#### 4.1.2 設計判断: `upmix` を子プリムに書かなかった理由 (r11 と同様)

`upmix` は親プリム Desc にのみ書く。子プリムへの記述は parser が読まない。理由は r11 §4.1.3 と同じ:

- upmix は **配信ストリーム全体の表現意図** であって、子プリム (= 個別スピーカー位置) ごとに変える概念ではない
- 子プリムにも書ける構造にすると配信者が混乱する (どこに書けば効くか不明瞭)
- 親プリムに集約することで `evaluateLinkset()` の単純化と reload コスト最小化

### 4.2 viewer 内部経路

#### 4.2.1 挿入位置 — SpeakerCallback OpKind 拡張 (= C 案)

**背景**: 当初の本書 (旧 §4.2.1) では「FMOD DSP として upmix を挿入する A 案 (group input) / B 案 (per-binding)」を候補としたが、P0 で実コードを読んだ結果、両案ともアーキテクチャ的に不適合と判明 (詳細: `doc/r12/dsp_insertion_survey.md`)。理由は以下:

- r10 の per-speaker channel は **mono** (`numchannels=1`、`createUserSounds()`)。FMOD DSP として channel に attach するものは 1ch in / 1ch out しかありえず、2→6 を materialize できない (B 案不可)
- Stream3D ChannelGroup の入力は **6 個の per-speaker mono channel** であって、source の 2ch には到達できない (A 案不可)
- 6 spk への split は既に `pcmReadCallback` 内で `OpKind` dispatch (Silent/Track/StereoSum/Bs775) として実装されている。upmix も同じ場所での dispatch 拡張が自然

**採用案 (C 案)**: `SpeakerCallback::OpKind` に `Upmix` を追加し、`pcmReadCallback` の dispatch で 2 track ring から L/R を pull、speaker 固有の役割 (FL/FR/C/LFE/SL/SR) に応じて upmix matrix + 帯域分離 + 状態 (LPF / delay) を適用して 1ch を生成する。これは r10 で確立した **Bs775 dispatch (6ch source → 1ch downmix per speaker role)** の対称構造。

| 既存 (r10 Bs775) | 新規 (r12 Upmix) |
|---|---|
| ring tracks: 6 (FL/FR/C/LFE/SL/SR、6ch source as-is) | ring tracks: 2 (L/R、2ch source as-is) |
| op_role: L / R / MonoLR (3 値) | op_role: FL / FR / C / LFE / SL / SR (6 値) |
| transform: `LLMultichannelDownmix::mix6chToMono` | transform: `LLStereoUpmix::upmix2chToSpeaker` (新設) |
| state: stateless (per-frame matrix) | state: per-speaker biquad LPF (LFE) / delay line (Ls/Rs) |
| 入口: `createUserSounds()` で `resolveReadOp()` 確定 | 同左 |

**主要ファイル**:

- 新規: `indra/llaudio/llstereoupmix.{h,cpp}` (`LLMultichannelDownmix` と並行構造のヘルパ)
- 修正: `indra/llaudio/llpositionalstreammulti.{h,cpp}` (`OpKind::Upmix` 追加、`pcmReadCallback` switch 拡張、`resolveReadOp` 分岐拡張、`SpeakerCallback` に upmix state field)

**選ばなかった案** (参考):

- ~~A 案: `createStream3DGroup()` の group 入力段に DSP 挿入~~ → group は per-speaker mono channel の合成しか見えず source 2ch に到達不可
- ~~B 案: `makeChannelForBinding()` で per-binding channel に DSP attach~~ → channel が mono なので 2→6 の materialize が不可能

#### 4.2.2 source ch 数判定 / auto bypass

source ch 数 (= codec が報告する channel count、`LLPositionalStreamStereo` で `mSourceSound->getFormat()` から取得し `mSourceChannels` に保存) は stream 開始時 (State::Buffering 完了直前) に確定し、以後 stream のライフタイム中は変わらない。判定は `createUserSounds()` の `resolveReadOp()` で 1 回行い、結果を per-speaker `SpeakerCallback::op_kind` に保存。mixer thread からの hot path での再評価は不要。

判定ルール:

- `mSourceChannels == 1` (mono): upmix 経路に入らない。r8 mono ch の Track dispatch のまま (`op_kind = Track, op_track = 0`)。upmix 対象外
- `mSourceChannels == 2` (stereo) + `effectiveUpmix() == on`: `op_kind = Upmix` を設定し、speaker の `ch` 値 (FL/FR/C/LFE/SL/SR/L/R/M) を upmix role にマップ
- `mSourceChannels == 2` + `effectiveUpmix() == off` (default): r10 と同じ Op (Track 0 / Track 1 / StereoSum / Silent) を維持。完全 r10 互換
- `mSourceChannels >= 6` (5.1 native): タグ `{upmix:on}` であっても **`op_kind = Upmix` を設定しない** (auto bypass)。r10 Bs775 / Track dispatch を維持。`{upmix:on}` が指定されていた場合のみ chat 通知 1 回 (「5.1 配信に upmix:on が付いていますが、自動 bypass されました」相当、mgr 側で発火)

source ch == 3/4/5 のような半端な値は: r9 で codec layer がそもそも reject するので upmix までは到達しない。仮に到達した場合は upmix 経路に入らず Silent / Track などにフォールバック (= 安全側)。

#### 4.2.3 r11 既存 DSP との順序

データフロー (= 信号が source から speaker output に至るまでの順序):

```
[Source stream (FMOD createStream)]               ← 2ch or 6ch
        ↓ pumpSource() / decode thread
[mRing (per-track ring buffer)]                   ← 2 or 6 tracks
        ↓ pcmReadCallback (mixer thread, per speaker)
[OpKind dispatch]                                 ← r12 で Upmix 追加
        ・Silent / Track / StereoSum / Bs775 (r10)
        ・Upmix (r12 新規、stereo source + {upmix:on} のとき)
        ↓ 1ch output
[FMOD::Channel (mono, OPENUSER, per speaker)]
        ↓ Channel::addDSP(head)
[r11 LiteHrtfDsp]                                 ← per-channel mono in/out
        ↓ Channel built-in panner (set3DLevel)
[FMOD::ChannelGroup "Stream3D"]                   ← 6 mono channel の合成
        ↓ Group::addDSP(tail)
[r11 VenueReverbDsp]
        ↓
[Master group → output]
```

upmix の 1 speaker output は r10 Track / Bs775 と完全に同じ「per-speaker mono channel」として下流に流れる。lite-HRTF / venue reverb は upmix の有無を意識しない (= placement 以降は r11 と完全互換、r10/r11 受入条件すべて維持)。

### 4.3 LLStereoUpmix の処理

#### 4.3.1 入力 / 出力 / 呼出単位

- **入力**: 2ch float PCM (mRing の track 0 = L、track 1 = R から `readFramesRaw()` で pull)
- **出力**: speaker 固有の 1ch float PCM (= role に応じて FL/FR/C/LFE/SL/SR のいずれか)
- **呼出単位**: `pcmReadCallback` の `datalen / sizeof(F32)` フレーム数。Bs775 dispatch と同じく `kReaderChunkFrames` 単位 (1024) での内部チャンク処理
- **state**: per-speaker (= per-`SpeakerCallback`)。LFE は biquad LPF state (Direct Form II、4 floats)、Ls/Rs は delay line buffer (16ms @ 44.1kHz ≈ 706 samples、固定 jitter で L/R ±2ms)、FL/FR/C は stateless

#### 4.3.2 Matrix decode (DPL2 ベース)

DPL2 (Dolby Pro Logic II) 系の静的マトリックス decode を採用:

```
C  = (L + R) / √2          ← 中央成分 (phantom center 抽出)
S  = (L - R) / √2          ← 側方成分 (Side info)
L' = L - C / √2            ← center bleed 除去後の L
R' = R - C / √2            ← center bleed 除去後の R
```

`L'` / `R'` を front L/R 出力、`S` を decorrelate して Ls / Rs に振る (§4.3.4)。係数 `1/√2` は power-preserving (合算前後でエネルギー保存)。

**center bleed 除去** (`L' = L - C/√2` 等) を入れる理由は §4.3.3 で詳述。

#### 4.3.3 Center bleed 除去

DPL2 系で C 成分を抽出した後、front L/R に元の L/R をそのまま流すと **「phantom center が center spk と front L/R 両方から鳴る」二重像** が発生し、定位が不安定になる。これを防ぐため、front L/R から C 成分の一部を引く:

```
L' = L - C × bleed_amount / √2
R' = R - C × bleed_amount / √2
```

`bleed_amount` の default は `1.0` (= フル除去、phantom center が center spk のみから鳴る)。listener 側 debug settings (`Stream3DUpmixCenterBleed` F32 0.0-1.0) で調整可。`0.0` にすると DPL1 互換 (front L/R も full-range)。

#### 4.3.4 Rear decorrelation

`S = (L-R)/√2` をそのまま Ls / Rs に振ると、Ls / Rs が **完全相関 (Ls = -Rs)** になり「rear 全体から鳴る」感ではなく「真後ろの 1 点から鳴る」感になる。これを decorrelate して空間感を出す:

- 短い random delay (12〜20ms) を Ls / Rs にそれぞれ独立に適用 (Haas effect の範囲、知覚的には同時に近いが decorrelate される)
- もしくは all-pass filter で phase 撹拌

実装は **短い random delay** で開始 (実装シンプル / CPU 軽量)。listener 側 debug settings (`Stream3DUpmixRearDelayMs` F32, default 16.0) で調整可。

```
Ls = delay(S, delay_l_ms)
Rs = delay(-S, delay_r_ms)
```

`delay_l_ms` / `delay_r_ms` は base value `Stream3DUpmixRearDelayMs` を中心に固定 jitter (例: ±2ms) で L/R 分離。

#### 4.3.5 LFE LPF (Butterworth biquad)

LFE は L+R sum を low-pass filter して生成:

```
LFE = LPF_cutoff((L + R) / 2)
```

cutoff の default は **80 Hz** (THX 推奨)。実装は biquad LPF (Butterworth 2nd order、-12dB/oct で十分。SL 配信音源で more steep が必要な状況は想定しない)。listener 側 debug settings (`Stream3DUpmixLfeCutoff` F32, default 80.0、許容範囲 20.0-200.0) で調整可。

LFE 出力レベル: `(L+R)/2` の振幅で LFE ch に流す。家庭 AV の bass management に相当する処理 (sub に振る分の振幅補正) は **入れない**。理由は SL 内の prim spk は物理的な sub 制約がないので、bass management は配置側の演出に委ねる。

#### 4.3.6 出力 role と r10 6 spk slot との整合 + 旧 ch 値 (L/R/M) の扱い

r10 の 6 spk placement は `{ch:FL}` / `{ch:FR}` / `{ch:C}` / `{ch:LFE}` / `{ch:SL}` / `{ch:SR}` の 6 slot を持つ。upmix の `op_role` (UpmixRole) は speaker の `ch` 値から `resolveReadOp()` 内で 1 対 1 に割り当てる:

| speaker `ch` | UpmixRole | 出力計算 |
|---|---|---|
| `FL` | `FL` | `L - C × bleed / √2` |
| `FR` | `FR` | `R - C × bleed / √2` |
| `C` | `C` | `(L + R) / √2` |
| `LFE` | `LFE` | `LPF((L+R)/2, cutoff)` |
| `SL` | `SL` | `delay(S, base + jitter)` (S = (L-R)/√2) |
| `SR` | `SR` | `delay(-S, base − jitter)` |
| `L` (旧 r8 stereo) | `FL` | (= 旧 ch:L 配置の listener も DPL2 matrix decode の恩恵を受ける) |
| `R` (旧 r8 stereo) | `FR` | 同上 |
| `M` (旧 r8 mono) | `C` | (= mono 中心配置の listener は phantom center を center 役で受ける) |

これにより r8 旧配置 (ch:L/R/M のみ) の listener も `{upmix:on}` で center bleed 除去 / rear decorrelation の恩恵を受ける (ただし 6 spk full surround を体感するには r10 配置が必要)。

### 4.4 debug settings 経由の強制 override (= 平時は不使用)

平時は **配信者がプリム Desc に書いた `{upmix:on|off}` タグが root truth** として動作する。listener viewer は自動的にタグから値を読み出し、Preferences UI から override する経路は提供しない (NG6)。

実装/検証時の独立 toggle / 微調整用途のみ、debug settings 経由の **強制 override / パラメータ微調整** を 4 件提供する。`Stream3DUpmix` のみ sentinel 値 (= 「タグ通り」default) を持ち、残り 3 件はパラメータの実値 default を持つ:

| キー | 型 | default | 効果 |
|---|---|---|---|
| `Stream3DUpmix` | int | `-1` (sentinel = タグ通り) | `0` = upmix を強制 OFF (タグ `{upmix:on}` を無視) / `1` = 強制 ON (タグ `{upmix:off}` でも有効化、ただし source ch>=6 の auto bypass は有効) |
| `Stream3DUpmixLfeCutoff` | F32 | `80.0` Hz | LFE LPF cutoff 周波数 (許容 20.0〜200.0)。配信者タグでは出さない。listener 側の DAW 的微調整用 |
| `Stream3DUpmixCenterBleed` | F32 | `1.0` | front L/R から center 成分を引く割合 (`0.0` = DPL1 互換 / `1.0` = フル除去、phantom center が center spk のみ) |
| `Stream3DUpmixRearDelayMs` | F32 | `16.0` ms | rear decorrelation 用の base delay。L/R 分離は ±2ms の固定 jitter |

**動作優先順位** (先に評価される側ほど強い):

1. source ch >= 6 → upmix 強制 bypass (どの設定/タグでも override 不可)
2. `Stream3DUpmix` に強制値あり (sentinel `-1` 以外) → 強制値を採用
3. プリム Desc タグ `{upmix:on|off}` → タグ値を採用
4. タグ未指定 → §4.1.0 表の default (`off`)

`LfeCutoff` / `CenterBleed` / `RearDelayMs` はアルゴリズム内部のパラメータなので「タグ通り sentinel」概念がない。配信者は触れないし、listener も平時は触らない。実装/検証時の調整専用。

### 4.5 r11 既存タグとの並行動作

r11 までのタグ (`{venue}` / `{binaural}` / `{wetgain}` / `{ch}` / `{range}` / `{volume}`) と並列指定可。例:

```
[3dstream-stereo:{upmix:on}{binaural:on}{venue:hall_medium}{wetgain:1.2}]
```

配信ストリーム = stereo source、upmix で 6 spk に展開、各 spk に lite-HRTF + venue=hall_medium reverb (wetgain 1.2倍) が乗る。

データフローは §4.2.3 のとおり source → mRing → OpKind dispatch (upmix or r10) → per-channel → r11 lite-HRTF → r11 venue reverb で、upmix は `pcmReadCallback` 内で完結する。upmix の有効/無効が変わると `OpKind` 再割当てが必要なので、これは r10 の placement rebuild と同じ tier (= stream rebuild が走る、live update ではない)。tag だけ on→off / off→on の toggle で rebuild 走るのは **意図通り**。

### 4.6 r10 / r11 受入条件への影響

r10 受入 (`spec_5_1ch_placement.md` §13) / r11 受入 (`spec_binaural_venue_reverb.md` §13) はすべて **upmix 無効状態 (= default off)** で従来通り維持される。upmix 有効状態は新規受入として §6 で追加。

---

## 5. 検証材料

stereo upmix の効果を主観的に確認するための検証材料:

- **stereo voice**: dialog のみのモノローグ (中央定位)。center bleed 除去で center spk から鳴ること、front L/R から phantom center が消えることを確認
- **wide stereo music**: instrumental で L/R に楽器が広く振られた素材。Ls / Rs に「ambience」として展開されることを確認 (= 音場の広がり)
- **stereo with strong sub bass**: 60Hz 以下に明確な bass 成分を持つ素材。LFE spk から低音だけが鳴ること、front L/R には残らないことを確認
- **5.1 native 配信 (回帰確認)**: r10/r11 で使った 6ch source。`{upmix:on}` を付けても auto bypass されること、chat 通知が出ること、6 spk 配置が r10 と完全一致することを確認

検証材料生成は `doc/r12/gen_upmix_test_material.sh` (新規) で `ffmpeg` ベースに作成。r11 の `gen_test_material.sh` の流儀を踏襲。

---

## 6. 受入条件

### 6.1 r12 新規

| # | 条件 | 判定方法 |
|---|---|---|
| U1 | stereo voice で `{upmix:on}` 時、center 成分が center spk から鳴る (front L/R から消える) | center bleed 除去の主観確認 |
| U2 | wide stereo music で `{upmix:on}` 時、Ls/Rs に side 成分が展開され「音場広がり」感が出る | rear decorrelation の主観確認 |
| U3 | bass 強調素材で `{upmix:on}` 時、LFE spk から低音だけが鳴り、front L/R には残らない | LFE LPF の主観確認 |
| U4 | 5.1 native (source ch>=6) で `{upmix:on}` を付けても auto bypass されチャット通知 1 回 | 通知文言確認 + 出力 ch が r10 と一致 |
| U5 | `{upmix:on}` ↔ `{upmix:off}` の Desc 編集 live 切替で次の `evaluateLinkset()` から効く (= rebuild 走る) | LSL から toggle して聴感差確認 |
| U6 | `Stream3DUpmix` debug = `0` で強制 OFF / `1` で強制 ON (`-1` = タグ通り) | 各設定で動作確認 |
| U7 | `Stream3DUpmixLfeCutoff` を 80→120Hz に変えると LFE 帯域が広がる | 主観確認 |
| U8 | `Stream3DUpmixCenterBleed` を 1.0→0.0 に変えると DPL1 互換 (phantom center が二重像) | 主観確認 |
| U9 | `Stream3DUpmixRearDelayMs` を 16→8ms に変えると rear decorrelation が薄くなる | 主観確認 |

### 6.2 r10 / r11 互換 (回帰)

- r10 §5.3 受入条件全行が回帰なし (`{upmix:off}` または未指定で完全互換)
- r11 §5.5 受入条件全行が回帰なし (`{upmix:off}` 状態で lite-HRTF / venue reverb / wetgain 動作維持)
- 5.1 native 配信は upmix の有無に関わらず r10/r11 と完全同一動作

### 6.3 安定性 / CPU

- 5min 連続再生 dropout 0 (stereo source + `{upmix:on}` + venue=hall_medium + binaural=on)
- URL 切替 ×10 で crash / 二重再生なし
- CPU 増分: r11 baseline (= upmix off) との比較で **+3pp 未満** を目標 (Matrix decode + biquad LPF + delay line のみで重い処理ゼロ)

---

## 7. リスク

| ID | 内容 | 縮退策 |
|---|---|---|
| R1 | DPL2 の **phase 依存性** で特定の stereo 素材 (例: vocal が片側 only の cinematic mix) で center 抽出が不自然 | 縮退 A: `{upmix:off}` を配信者がタグで明示することで個別 stream を回避できる (アルゴリズム内固定、プリセット切替なし) |
| R2 | center bleed 除去で **front L/R が薄く感じる** (= bleed=1.0 が強すぎる) | 縮退 B: `Stream3DUpmixCenterBleed` default を 0.5〜0.7 に下げる (P11 検証で決定) |
| R3 | rear decorrelation で **rear が "強すぎ / 薄すぎ"** | 縮退 C: `Stream3DUpmixRearDelayMs` default を 12〜20ms 範囲で調整 (P11 検証で決定) |
| R4 | LFE LPF 80Hz が **配信音源によってボワつく / スカスカ** | 縮退 D: `Stream3DUpmixLfeCutoff` default を 100〜120Hz に変更 (P11 検証で決定) |
| R5 | source ch 判定が **stream 開始タイミングで間に合わない** (codec layer の遅延) | 縮退 E: ch 数判定 timeout を設定、判定不可なら upmix 無効 (= 安全側、5.1 として誤動作させない) |
| ~~R6~~ | ~~upmix DSP 挿入箇所 (A 案 vs B 案) で **想定外の DSP chain 不整合** が発生~~ | **解消 (2026-05-07)**: P0 調査で C 案 (`SpeakerCallback::OpKind` 拡張) として確定、A/B 案は廃案 (詳細: `doc/r12/dsp_insertion_survey.md`)。Bs775 dispatch の並行構造で実装するため DSP chain 不整合の余地なし |

---

## 8. 実装フェーズ概要 (詳細は impl record)

詳細フェーズ分解と依存関係は `docs/ayastorm-r12-stereo-upmix.md` を参照。本書では概要のみ:

- **P0**: 仕様確定 + roadmap doc 同時更新 + 実装箇所調査 → **C 案 (`OpKind::Upmix` 拡張) として確定** (`doc/r12/dsp_insertion_survey.md`)
- **P1**: `LLStereoUpmix` helper class skeleton (`LLMultichannelDownmix` と並行構造、未配線)
- **P2**: `pcmReadCallback` に `OpKind::Upmix` dispatch + `LLStereoUpmix::upmix2chToSpeaker` (matrix decode + center bleed 除去 + rear decorrelation) 実装
- **P3**: LFE LPF (biquad) 実装 (`LLStereoUpmix` 内、role=LFE のとき有効)
- **P4**: `resolveReadOp` 分岐拡張 (`mSourceChannels == 2 + effectiveUpmix() == on` で `OpKind::Upmix` 設定、auto bypass は `mSourceChannels >= 6` で C 案 dispatch を返さない)
- **P5**: `{upmix:on|off}` タグ parser + `Stream3DUpmix` debug 配線 (toggle 時は stream rebuild で resolveReadOp 再評価)
- **P6**: debug settings 3 件 (LfeCutoff / CenterBleed / RearDelayMs) 配線
- **P7**: 検証材料生成スクリプト
- **P8**: 配信者 LSL に Upmix UI 追加
- **P9**: 検証実行 (U1〜U9)
- **P10**: r10 / r11 回帰確認
- **P11**: CPU benchmark + spec close-out

工数感: **1〜2 週間** (実装 5〜7 日 + 検証 2〜3 日)。

---

## 9. r13+ への持ち越し

旧 r12 計画から降格した項目 + 本書策定時に新規発生した持ち越し:

### 9.1 r13: SOFA per-source HRTF (旧 r12 main)

- KU100 等 reference HRTF を per-channel convolution に挿入、lite-HRTF を SOFA HRTF に置き換え
- 配信者タグ `{binaural:lite|sofa|off}` 等 enum 化を r13 着手時に検討
- 個人 SOFA 再配布制限の調査 (RR2 の解像度を上げる)
- r12 で確立した DSP 挿入経路 (upmix → placement → HRTF → venue) の HRTF slot を SOFA に差し替え

### 9.2 r13+: Steam Audio integration

- world geometry を Steam Audio に食わせ occlusion / reflection を simulate
- Linux / macOS の binary build 問題 (RR1) を再評価
- venue convolution reverb (r11) との二重残響回避: Steam Audio reflection ON 時は convolution reverb を auto disable する条件分岐が必要

### 9.3 r13+: VenueReverb CPU 最適化

- NUPC (non-uniform partition convolution) 等で hall_medium 以上の +8〜10pp を低減
- r11 spec §13.5 / §13.6 で識別済み、r13+ で着手

### 9.4 r13+: air absorption の客観 FFT 測定

- `-15dB @ 4kHz` を実機で対 dry スペクトル比較
- r11 P12 で主観 PASS、客観測定は r11.x or r13+ へ

### 9.5 r13+: 個人 HRTF measurement / personalization

- 自家計測 (microphone-in-ear) の HRTF を SOFA 形式で読み込む経路
- r13 (SOFA HRTF) 完了後に自然に拡張可能

### 9.6 r13+: 公開 README / changelog 一括開示

- README / 公開ドキュメントで r8〜r12 の書式と使い方を一括開示
- 配置者向けガイド (venue 種別の選び方、配信側 dry 推奨、upmix の opt-in 推奨など) を公開

### 9.7 r13+: アルゴリズム多択化 (本書策定時に新規発生)

- DPL2 → Logic 7 系 multi-band steering / SRS / ML 系の代替アルゴリズム検討
- 配信者タグ `{upmix:dpl2|logic7|...}` の enum 化を検討。ただし NG1 で「決め打ち」と確定したので、AYAstorm 側のエコシステム成熟と聴感ベース評価をしてから判断

### 9.8 r13+: 動的 steering

- 入力解析に基づく時変マトリックス係数調整 (Logic 7 系)
- 動的 panning detection / dialog detection / ambience detection による intelligent up-mix

### 9.9 r13+: ML / Spatial Audio AI 系 upmix

- ML model による高品位 stereo→5.1 推論
- viewer 同梱の場合 model size / 配布負債 / CPU 負荷を要検討

---

## 10. 変更履歴

- 2026-05-07: 初版作成。r11 完了直後の議論で AYA さんから「世間の SL 配信はほぼ stereo、6 spk placement の元を取りたい」提案。旧 r12 計画 (SOFA per-source HRTF + Steam Audio) は本書策定で「stereo upmix のみ」に再定義、SOFA / Steam Audio / VenueReverb CPU 最適化 / air absorption 客観測定 / 個人 HRTF / 公開 README は r13+ に降格 (詳細は §2.2 / §9)。アルゴリズムは DPL2 系 matrix decode + 帯域分離で決め打ち (§2.3)。配信者主導モデル (r11 で確立) を維持、新タグ `{upmix:on|off}` (default off)、debug settings 4 件 (sentinel 1 件 + 微調整 3 件)。
- 2026-05-07: P0 調査結果を反映。実コード (`indra/llaudio/llpositionalstream*.{h,cpp}`) を読んで DSP 挿入位置を確定。当初 §4.2.1 で候補とした A 案 (`createStream3DGroup` 入力段) / B 案 (`makeChannelForBinding` per-binding) はいずれも実アーキテクチャに不適合と判明 — per-speaker channel が mono (`numchannels=1`) で 2→6 materialize 不可、Stream3D group は per-speaker mono の合成しか見えず source 2ch に到達不可。代わりに r10 の `SpeakerCallback::OpKind` (Bs775 dispatch) を拡張する **C 案** として確定: `OpKind::Upmix` を追加し、`pcmReadCallback` 内で 2 track ring から 2ch を pull、speaker 役割に応じて upmix matrix + 帯域分離 + 状態を適用して 1ch 出力。新規ヘルパは `LLStereoUpmix` (`indra/llaudio/llstereoupmix.{h,cpp}`、`LLMultichannelDownmix` 並行構造)。これに伴い §4.2.1 / §4.2.2 / §4.2.3 / §4.3 (タイトル + §4.3.1 / §4.3.6) / §4.5 / §7 R6 を改訂。詳細は `doc/r12/dsp_insertion_survey.md`。
