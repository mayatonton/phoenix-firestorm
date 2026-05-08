# r12 P0: stereo→5.1 upmix 挿入位置調査

**日付**: 2026-05-07
**目的**: `doc/spec_stereo_upmix.md` §4.2.1 で「P0 で確定」とした DSP 挿入位置 (A 案 / B 案) を実コードで判定する。
**結論**: **A 案 / B 案いずれも本実装には不適合**。実装は **C 案 (SpeakerCallback OpKind 拡張) を採用** とし、spec §4.2.1 / §4.3 と impl record P1〜P4 を更新する。

---

## 1. 現状のデータフロー (実コード読解)

`indra/llaudio/llpositionalstream*.{h,cpp}` を読んで判明した r10/r11 の実フロー:

```
[Source stream (FMOD createStream)]                ← 2ch or 6ch
        │
        ├─ LLPositionalStreamStereo::pumpSource()   (decode thread)
        │  → LLMultiTailRing::writeFrames(N tracks)
        │     ・2ch source: track[0]=L, track[1]=R
        │     ・6ch source: track[0..5] = FL/FR/C/LFE/SL/SR (codec layout)
        ▼
[mRing (per-track ring buffer, N tracks)]
        │
        ▼ (pcmReadCallback per speaker、mixer thread)
[SpeakerCallback::OpKind dispatch] ─── (llpositionalstreammulti.cpp:527)
        │   Silent:    skipFrames + memset 0
        │   Track:     readFramesTrack(track_idx)
        │   StereoSum: readFramesMonoSum (= (track0+track1)/2)
        │   Bs775:     readFramesRaw + mix6chToMono(role) (6ch→1ch downmix)
        ▼
[Per-speaker mono output (1ch)]
        │
        ▼ ← FMOD::Channel (numchannels=1, OPENUSER)
        │
        │ ┌─ r11 LiteHrtfDsp (Channel::addDSP, head)  // 1ch in/out
        │ ▼
        │ FMOD built-in 3D panner (set3DLevel=0.0 で hrtf 時 bypass)
        ▼
[FMOD::ChannelGroup "Stream3D"] ─── (llaudioengine_fmodstudio.cpp:380)
        │
        │ ┌─ r11 VenueReverbDsp (Group::addDSP, tail)  // mix した結果に reverb
        │ ▼
        ▼
[Master group → output]
```

**核心ポイント**:

1. **per-speaker channel は mono** (createUserSounds で `ex.numchannels = 1`、llpositionalstreamstereo.cpp:365)。FMOD DSP として channel に挿入するものは「1ch in / 1ch out」しかありえない。
2. **per-speaker 出力は mono → multi-tail ring から自分の役割の出力を pcmReadCallback で個別計算する**。Bs775 dispatch (6ch→1ch) はその実例で、6ch source を track[0..5] として ring に持ち、各 speaker callback が `mix6chToMono(role)` で自分の 1ch を作る。
3. **Stream3D ChannelGroup の入力は 6 個の mono channel** (= 6 spk placement)。group の view は「N mono の合成」であって「2ch source」ではない。

---

## 2. spec §4.2.1 の A 案 / B 案がなぜ不適合か

spec の挿入位置候補:

- **A 案**: `LLAudioEngine_FMODSTUDIO::createStream3DGroup()` で stream-level (= group の入力段) に upmix DSP を挿入
- **B 案**: `LLPositionalStreamMulti::makeChannelForBinding()` で per-stream / per-binding に挿入

**A 案がダメな理由**:
Stream3D group の入力は 6 個の **既に upmix 後** であるべき mono channel。group 入力段に DSP を置いても、その DSP が見るのは「6 個の per-speaker mono channel が mix された FMOD の中間表現」であり、source の 2ch 入力には到達できない。**2ch を観測できる位置に居ない**。

**B 案がダメな理由**:
per-binding (= per-speaker) channel は **mono**。その channel に attach する DSP は 1ch in / 1ch out。「2ch in 6ch out」を materialize する余地がない。仮に「6 個 すべての per-speaker channel に 2→1 の upmix DSP」を分散して attach する案も考えられるが、それは結局「各 speaker callback が 2ch source を見て自分の 1ch を生成する」という C 案の二重実装になり、しかも source の 2ch にどうアクセスするか (= ring buffer 経由しかない) で C 案と同じ問題に帰着する。

両者とも「FMOD DSP として upmix を実現する」という framing が成立しない。spec はこの framing で書かれていたが、**実コードを読むと FMOD DSP の世界では解けない問題** だった。

---

## 3. C 案: SpeakerCallback OpKind 拡張 (採用)

**設計**:

`SpeakerCallback::OpKind` に `Upmix` を追加。`mSourceChannels == 2 + effectiveUpmix() == on` のとき `resolveReadOp` で各 speaker に対応する upmix role を割り当てる。pcmReadCallback の dispatch で 2 track ring から 2ch を pull し、speaker 固有の役割 (FL/FR/C/LFE/SL/SR) に upmix matrix + 帯域分離 + 状態 (LPF / delay) を適用して 1ch 出力する。

**Bs775 dispatch (6ch→1ch role 別 downmix) と完全並行**:

| 既存 (r10 Bs775) | 新規 (r12 Upmix) |
|---|---|
| ring tracks: 6 (FL/FR/C/LFE/SL/SR、6ch source as-is) | ring tracks: 2 (L/R、2ch source as-is) |
| op_role: L / R / MonoLR (3 値) | op_role: FL / FR / C / LFE / SL / SR (6 値) |
| transform: `LLMultichannelDownmix::mix6chToMono` | transform: `LLStereoUpmix::upmix2chToSpeaker` (新設) |
| state: stateless (per-frame matrix) | state: per-speaker biquad LPF (LFE) / delay line (Ls/Rs) |
| 入口: createUserSounds で resolveReadOp 確定 | 同左 |

**ファイル変更プラン**:

- 新規: `indra/llaudio/llstereoupmix.{h,cpp}` — `LLMultichannelDownmix` と並行構造のヘルパクラス
  - `bool isSupported()` — 常に true (DPL2 系決め打ちなのでフォーマット依存なし)
  - `enum class UpmixRole { FL, FR, C, LFE, SL, SR }`
  - per-speaker state: biquad LPF (Direct Form II)、delay line (1 ring で 2 read tap)
  - `void upmix2chToSpeaker(const F32* track_l, const F32* track_r, F32* out, size_t n_frames, UpmixRole role, ...)` — speaker 固有出力 1ch 生成
- 修正: `indra/llaudio/llpositionalstreammulti.h`
  - `OpKind::Upmix` 追加、`op_role_upmix` field 追加 (Bs775 と並行)
  - `SpeakerRuntime` に upmix 状態保持の field 追加 (LPF state、delay buffer)
- 修正: `indra/llaudio/llpositionalstreammulti.cpp`
  - `pcmReadCallback` switch に `case Upmix:` 追加 (Bs775 と並行構造)
  - `resolveReadOp` で `mSourceChannels == 2` 分岐に「upmix on のとき Upmix dispatch」追加
- 修正: `indra/newview/llpositionalstreammgr.{h,cpp}`
  - `{upmix:on|off}` parser、`mUpmix`、`effectiveUpmix()` (P5)
- 修正: `indra/newview/app_settings/settings.xml`
  - `Stream3DUpmix` (sentinel `-1`) + 微調整 3 件 (P5/P6)

**Bs775 と完全に並行する点で実装難易度は高くない**。Bs775 は r10 P4 で 1 phase で済んでおり、Upmix も同等の規模感に収まる。

---

## 4. C 案の優位性 (vs spec の A/B)

- **データフローが現状と整合**: 既存 OpKind の延長線上にあり、pcmReadCallback dispatch / createUserSounds resolve / SpeakerRuntime の bring up すべて Bs775 経路を踏襲できる
- **FMOD DSP に頼らない**: 2→6 という非対称 channel 変換を FMOD DSP framework に押し込もうとすると無理が生じるが、callback 内で素直に 2 track 読み込んで 1ch 計算するのは自然
- **state の locality**: speaker 固有の biquad / delay state が SpeakerCallback に乗るので、thread race / lifetime / synchronization の追加考慮が要らない (r10 Bs775 と同じ流儀)
- **r10/r11 互換**: `mSourceChannels == 2` で `{upmix:off}` または未指定なら resolveReadOp は r10 と全く同じ Op (Track 0, Track 1, StereoSum, Silent) を吐く。デフォルト経路完全同一
- **5.1 native auto bypass が自然**: `mSourceChannels == 6` のとき resolveReadOp は r10 と同じ Bs775 / Track dispatch を返す。`{upmix:on}` の値は見ない (= タグを尊重しない、auto bypass)。chat 通知 1 回は別経路 (mgr 側) で実装

---

## 5. spec / impl record への反映

本調査結果を以下のドキュメントに反映する (P0 commit 第 2 弾):

### `doc/spec_stereo_upmix.md` 修正

- §4.2.1 「DSP 挿入位置」を **「SpeakerCallback OpKind 拡張」に書き換え**。「FMOD DSP として挿入する」という framing を撤廃し、Bs775 dispatch の並行構造として記述する
- §4.3 「StereoUpmixDsp の処理」を **「LLStereoUpmix の処理」に rename**。ファイル名 `llstereoupmixdsp.{h,cpp}` を `llstereoupmix.{h,cpp}` に変更
- §4.3.1 「入力 / 出力」を「FMOD DSP block size」表現から「pcmReadCallback datalen 単位」に書き換え
- §7 リスク R6 「DSP 挿入箇所 A/B 案」を「OpKind 拡張で確定 (P0 調査済)」に解消マーク
- §10 変更履歴に P0 調査での確定を追記

### `docs/ayastorm-r12-stereo-upmix.md` 修正

- P1「StereoUpmixDsp skeleton (2ch passthrough、未配線)」を **「LLStereoUpmix helper class skeleton (LLMultichannelDownmix と並行構造)」** に書き換え
- P2「Matrix decode... read callback 拡張」を「`pcmReadCallback` に `OpKind::Upmix` dispatch 追加 + LLStereoUpmix::upmix2chToSpeaker 実装」に書き換え
- P4「source ch 数判定 + auto bypass」を **「`resolveReadOp` 分岐拡張 (`mSourceChannels == 2 + effectiveUpmix() == on` で `OpKind::Upmix` 設定)」** に書き換え。FMOD DSP attach/detach の話を消す
- P5「タグ parser + Stream3DUpmix debug 配線」は変更小 (DSP 挿入/削除フックの話を resolveReadOp re-evaluation に書き換え)
- §3 マイルストーン依存関係に「P0 で C 案確定」を追記
- ファイル一覧 (`llstereoupmixdsp.{h,cpp}` → `llstereoupmix.{h,cpp}`) を全 phase で修正

### `docs/ayastorm-stream3d-roadmap.md` 軽微修正

- §3 r12 entry の「stereo→5.1 upmix DSP (`llstereoupmixdsp.{h,cpp}` 新設)」を `llstereoupmix.{h,cpp}` に rename。「DSP」という呼称も「helper class」に変更
- §5 RR10 「DSP 挿入位置 (A 案 vs B 案) で想定外の DSP chain 不整合」は「P0 調査で C 案 (OpKind 拡張) として確定、A/B 案は廃案」と解消マーク

---

## 6. 残課題 / r13+ への影響

- **state 量の確認**: per-speaker LPF (4 floats) + delay buffer (~706 samples × 1 = 2824 bytes) × 6 speakers = ~17KB。問題ない量
- **dropout 影響**: pcmReadCallback での計算量増加分 (matrix + biquad + delay tap) は Bs775 の `mix6chToMono` 並みかそれ以下。CPU 受入条件 (+3pp 未満) の達成可能性は現実的
- **r13+ で SOFA を載せる場合**: 今回の C 案で確立した「OpKind 拡張で 6 spk placement の前段に整形を入れる」パターンは、r13 で SOFA HRTF を per-channel に載せる場合にも流用可能 (= speaker channel が mono のまま、DSP head に SOFA convolution を addDSP)。SOFA は per-channel 1ch in/1ch out の形になるので、A/B 案と違って FMOD DSP に素直に乗る
- **アルゴリズム多択化 (r13+)**: 現 C 案では `LLStereoUpmix` が DPL2 決め打ち。r13+ で Logic 7 / SRS 等を入れる場合、`LLStereoUpmix` の interface はそのまま、内部 algorithm を strategy pattern 化すれば済む

---

## 7. 結論

C 案 (SpeakerCallback OpKind 拡張) で実装する。spec / impl record / roadmap の関連箇所を本調査結果で書き換える。

P1 着手は spec / impl record 更新後。
