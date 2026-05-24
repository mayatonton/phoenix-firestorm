# AYAstorm r12: stereo→5.1 upmix 実装工程

**作成日**: 2026-05-07 (r11 完了直後の r12 計画策定時)
**対象**: AYAstorm `feature/aya-r12-stereo-upmix` (予定 / 着手前)
**仕様**: `docs/specs/spec_stereo_upmix.md`
**前リリース**: r11 (`feature/aya-r11-binaural-venue-reverb`、リリース判断保留中)

> **本書の役割**: phase 単位の作業内容と依存関係、検証チェックリストの **テンプレート** を記録する。
> リスク (`spec §7`)、受入条件 (`spec §6`)、r13+ への持ち越し (`spec §9`) は仕様書側を canonical とし、本書では参照する。
> 本書は r12 着手前の **計画スナップショット** であり、実装進行に伴って commit hash / 実測値を埋めていく。

---

## 1. ゴール

r10 で完成した **Layer 1 (per-channel placement)** と r11 で完成した **Layer 2 (lite-HRTF + venue reverb)** を、SL で実際に流通している **stereo 配信** にも届けるリリース。

設計の核は **配信者主導モデルの維持** (r11 で確立): 親プリム Desc に追加した 1 タグキー (`{upmix:on|off}`) が root truth、default `off`、source ch>=6 では auto bypass。listener 側の Preferences UI 改修は一切行わない。実装/検証時の独立 toggle / パラメータ微調整用に debug settings 4 件 (sentinel 1 件 + 実値 default 3 件) のみ提供。

アルゴリズムは **matrix upmix + 帯域分離 (LFE LPF / center bleed 除去 / rear decorrelation) で決め打ち**。SOFA per-source HRTF / Steam Audio integration / VenueReverb CPU 最適化 / air absorption 客観測定 / 個人 HRTF / 公開 README は r12 では非対象、r13+ への保留。spec §1 / §2.2 の構成判断 (旧 r12 計画降格 / アルゴリズム決め打ち) は仕様書側を参照。

---

## 2. フェーズ分解

viewer-only の改修。配信側パイプラインは r9 / r10 / r11 流用。検証材料は r11 流儀で `docs/archive/r12/` に sox/ffmpeg ベースのスクリプトを置く。

### P0: 仕様確定 + roadmap doc 同時更新 + 実装箇所調査 → C 案確定

**目的**: 旧 r12 案 (SOFA + Steam Audio + 個人 HRTF + 公開 README + air absorption 客観測定) を spec / roadmap 双方で「stereo→5.1 upmix のみ」に置き換え、r13+ への降格を明文化。同時に upmix 処理の挿入位置 (当初 spec §4.2.1 の A 案: stream-level group の入力段 / B 案: per-stream / per-binding DSP) を実コードで判定。

**P0 調査結果** (2026-05-07): 実コード (`indra/llaudio/llpositionalstream*.{h,cpp}`、`llaudioengine_fmodstudio.cpp`) を読んで A 案 / B 案 はいずれも不適合と判明。per-speaker channel が mono (`numchannels=1`、`createUserSounds()`) で 2→6 materialize 不可 (B 案不可)、Stream3D group は per-speaker mono の合成しか見えず source 2ch に到達不可 (A 案不可)。代替として **C 案 = `SpeakerCallback::OpKind` 拡張** を確定: r10 で確立した Bs775 dispatch (6ch source → 1ch per speaker role) の並行構造として `OpKind::Upmix` を追加し、`pcmReadCallback` 内で 2 track ring から L/R を pull、speaker 役割 (FL/FR/C/LFE/SL/SR) に応じて upmix matrix + 帯域分離 + state を適用して 1ch 出力する。新規ヘルパは `LLStereoUpmix` (`LLMultichannelDownmix` の対称構造)。詳細は `docs/archive/r12/dsp_insertion_survey.md`。

**ファイル**:
- `docs/specs/spec_stereo_upmix.md` (新規、本書 §1 / §2 と同構成、初版)
- `docs/ayastorm-stream3d-roadmap.md` §3 r12 (旧 SOFA / Steam Audio 案 → 新仕様、r13+ 項目追加)
- `docs/archive/r12/dsp_insertion_survey.md` (新規、P0 調査記録)
- 上記 spec / impl record / roadmap への C 案反映 (P0 第 2 commit)

**完了条件**: 仕様書 AYA レビュー通過 / roadmap §3 r12 が新仕様で読める / Preferences UI 改修ゼロ方針が明文化 / DSP 挿入位置が C 案 (`OpKind::Upmix` 拡張) として確定 / spec §4.2.1 / §4.3 と本書 P1〜P5 が C 案で書き直されている。

**commit**: (P0 第 1 弾 = 初版 spec / roadmap / メモリ、P0 第 2 弾 = 調査結果反映)

---

### P1: LLStereoUpmix helper class skeleton (LLMultichannelDownmix と並行構造)

**目的**: r10 で確立した `LLMultichannelDownmix` (6ch→1ch per role) と並行構造のヘルパクラス `LLStereoUpmix` の骨格を作る。FMOD DSP ではなく、`pcmReadCallback` から呼ばれるピュア C++ クラス。最初はパススルー (FL→L, FR→R、C/Ls/Rs/LFE は 0) で実装、ロジックは P2/P3 で詰める。

**ファイル**:
- `indra/llaudio/llstereoupmix.{h,cpp}` (新規)
  - `enum class UpmixRole { FL, FR, C, LFE, SL, SR }`
  - `bool isSupported()` (常に true、フォーマット依存なし)
  - `void upmix2chToSpeaker(const F32* l, const F32* r, F32* out, size_t n_frames, UpmixRole role, State& state, ...)` (P2/P3 で本体実装)
  - per-speaker `State` struct (LFE biquad LPF state、Ls/Rs delay buffer、stateless role はメモリゼロ)

**完了条件**: ヘルパクラスがコンパイル通る / impulse / sine 入力で各 role の出力 ch shape が想定どおり (FL=L, FR=R passthrough、C/LFE/SL/SR=0) / ユニットテスト相当のスポット確認可能。

**commit**: (TBD)

---

### P2: pcmReadCallback に OpKind::Upmix dispatch 追加 + LLStereoUpmix::upmix2chToSpeaker 実装

**目的**: spec §4.3.2〜§4.3.4 の matrix upmix 本体を `LLStereoUpmix::upmix2chToSpeaker` に実装し、`SpeakerCallback::pcmReadCallback` の switch に `case Upmix:` を追加 (Bs775 と並行構造)。`C = (L+R)/√2`、`S = (L-R)/√2`、`L' = L - C×bleed/√2`、`R' = R - C×bleed/√2`、Ls/Rs は S を short random delay (12〜20ms) で decorrelate。

**ファイル**:
- `indra/llaudio/llpositionalstreammulti.h` (`OpKind::Upmix` 追加、`UpmixRole op_role_upmix` field 追加 — Bs775 の `op_role_bs775` と並行)
- `indra/llaudio/llpositionalstreammulti.cpp` (`pcmReadCallback` switch に `case Upmix:` 追加、2 track ring から readFramesRaw → `LLStereoUpmix::upmix2chToSpeaker` 呼び出し → 1ch 出力)
- `indra/llaudio/llstereoupmix.cpp` (matrix decode 本体実装、role=FL/FR/C は stateless、SL/SR は delay line 使用)

**完了条件**: 仕様 §4.3.2〜§4.3.4 の数式通り / `bleed` / `delay` がパラメータ化されており P6 で debug settings から触れる準備済み / r10 の Bs775 と同じ呼出単位 (`kReaderChunkFrames` = 1024 framings) で動作 / 単体テスト相当 (impulse response 形状目視) で center / S / L'/R' / Ls/Rs が想定通り。

**commit**: (TBD)

---

### P3: LFE LPF (Butterworth biquad) 実装

**目的**: spec §4.3.5 の LFE 経路。`LFE = LPF_cutoff((L+R)/2)`、cutoff default 80 Hz、Butterworth 2nd order biquad、Direct Form II 状態 (4 floats per speaker)。bass management 補正は入れない。

**ファイル**:
- `indra/llaudio/llstereoupmix.cpp` (LFE role の処理に biquad LPF 追加、`State::lpf_state` 4 floats を Direct Form II で更新)
- `indra/llaudio/llstereoupmix.h` (`State` struct に `lpf_state[4]` 追加)

**完了条件**: cutoff default 80Hz で 60Hz 以下が通過、4kHz 以上が -40dB 以上落ちる (FFT 目視) / cutoff パラメータが P6 で debug settings から触れる準備済み / `State` がきっちり per-speaker (UpmixRole=LFE のときのみ使用) で thread race なし。

**commit**: (TBD)

---

### P4: resolveReadOp 分岐拡張 (auto bypass 含む)

**目的**: spec §4.2.2 の判定経路。`createUserSounds()` の `resolveReadOp()` (per-speaker、stream 開始時 1 回) に「`mSourceChannels == 2 + effectiveUpmix() == on` で `OpKind::Upmix` + speaker `ch` 値から `UpmixRole` 設定」分岐を追加。`mSourceChannels >= 6` のときは upmix on でも C 案 dispatch を返さず r10 と同じ Bs775 / Track 経路を維持 (= auto bypass)。

**ファイル**:
- `indra/llaudio/llpositionalstreammulti.cpp` (`resolveReadOp` の `mSourceChannels == 2` 分岐を `effectiveUpmix()` で 2 ルートに分割)
- `indra/llaudio/llpositionalstreammulti.h` (`mapChToUpmixRole(ch)` ヘルパ追加 — FL/FR/C/LFE/SL/SR/L/R/M を `UpmixRole` にマップ。L→FL、R→FR、M→C は spec §4.3.6 表どおり)
- `indra/newview/llpositionalstreammgr.cpp` (5.1 native + `{upmix:on}` 時の chat 通知 throttle、stream 開始時 1 回)

**完了条件**: source ch == 2 + `{upmix:on}` で `op_kind = Upmix` + 各 speaker に正しい role 割当 / source ch >= 6 で `{upmix:on}` でも r10 と同じ op (Bs775 / Track) + chat 通知 1 回 / source ch == 1 で `op_kind = Track 0` (mono 対象外、r8 経路維持) / source ch == 3/4/5 は到達しない (codec reject) が、到達した場合は安全側で Silent / Track にフォールバック。

**commit**: (TBD)

---

### P5: `{upmix:on|off}` タグ parser + `Stream3DUpmix` debug 配線

**目的**: 配信者タグ `{upmix:on|off}` parser、debug settings `Stream3DUpmix` (int, sentinel `-1`)、`effectiveUpmix()` 合成 getter、live toggle 時の rebuild 経路。C 案では DSP 挿入/削除ではなく **`resolveReadOp` 再評価による stream rebuild** で toggle が反映される (= placement rebuild と同じ tier、spec §4.5)。

**ファイル**:
- `indra/newview/llpositionalstreammgr.{h,cpp}` (parser、`mUpmix`、`effectiveUpmix()`)
- `indra/newview/app_settings/settings.xml` (`Stream3DUpmix` 追加、sentinel `-1`)
- `indra/llaudio/llpositionalstreammulti.cpp` (live toggle 時の rebuild trigger — タグ変化を `evaluateLinkset()` が拾い、`resolveReadOp` 再実行で `OpKind::Upmix` ↔ r10 op を切替)

**完了条件**: タグ未指定で upmix OFF (default) / `{upmix:on}` で ON / debug `=0/=1/=-1` の 3 sentinel が正しく動作 / `=0` 状態 = r10 / r11 完全一致 (resolveReadOp が r10 と同じ op を返す) / live Desc 編集で次の `evaluateLinkset()` から rebuild が走り `OpKind` が再割当てされる。

**commit**: (TBD)

---

### P6: debug settings 3 件 (LfeCutoff / CenterBleed / RearDelayMs) 配線

**目的**: spec §4.4 のパラメータ微調整 debug settings 3 件を `settings.xml` に追加し、`LLStereoUpmix` が main thread から atomic-write で受け取る経路を作る (r11 P4 と同方針、ただし対象は FMOD DSP ではなく helper class の atomic field)。

**ファイル**:
- `indra/newview/app_settings/settings.xml` (`Stream3DUpmixLfeCutoff` / `Stream3DUpmixCenterBleed` / `Stream3DUpmixRearDelayMs` 追加)
- `indra/llaudio/llstereoupmix.{h,cpp}` (`std::atomic<F32>` のグローバル param または per-stream の atomic fields + `upmix2chToSpeaker` で per-call snapshot 取得)
- `indra/newview/llpositionalstreammgr.cpp` (debug settings 値を helper の atomic に push、stream 起動時 + 設定変更時)

**完了条件**: `Stream3DUpmixLfeCutoff` を 80→120Hz に変えると LFE 帯域が広がる / `Stream3DUpmixCenterBleed` を 1.0→0.0 に変えると center 除去なしになる (phantom center 二重像) / `Stream3DUpmixRearDelayMs` を 16→8ms に変えると rear decorrelation が薄くなる / 3 件すべて lock-free atomic write で thread race なし (mixer thread は read-only snapshot)。

**commit**: (TBD)

---

### P7: 検証材料生成スクリプト

**目的**: spec §5 の 4 種素材 (stereo voice / wide stereo music / strong sub bass stereo / 5.1 native 回帰用) を再現可能に生成するスクリプトを `docs/archive/r12/` に置く。

**ファイル**:
- `docs/archive/r12/gen_upmix_test_material.sh` (新規、ffmpeg + sox + espeak-ng + oggenc)

**完了条件**: 4 素材が再現可能に生成 / Vorbis (`oggenc --quality 5`) のみ / r11 の `gen_test_material.sh` と同じ流儀 / AYA hosting 環境 (Range 対応 HTTP server) で再生可能。

**commit**: (TBD)

---

### P8: 配信者 LSL に Upmix UI 追加

**目的**: 配信者が r12 の新タグ (`{upmix}`) を Desc 直接編集ではなく LSL menu / dialog から設定できるようにする。memory `feedback_lsl_mirrors_viewer_tags.md` の原則 (viewer に追加した Desc タグは LSL から全部設定可能) を満たす。

**ファイル**:
- `docs/guides/lsl/aya_3dstream_setup.lsl` (mode token / dialog / handler / fmtCurrent / buildTagBody に Upmix 追加)

**実装**:
- ROOT メニュー: 11 → 12 ボタン (Upmix 追加)
- Upmix dialog: `On` / `Off` / `Default` の 3 ボタン (`{upmix:on}` / `{upmix:off}` / `dropField` で削除して viewer default に戻す)
- 「Default」は viewer default (= sentinel 経由で debug setting `Stream3DUpmix` が効く状態) に戻す

**完了条件**: 全 r12 タグが LSL menu / dialog から編集可 / Default ボタンで viewer default に戻せる / r11 までの 11 ボタン (Url/Ch/Range/Volume/Stop/Start/Save/Restore/Binaural/Venue/WetGain) との並びが整理されている。

**commit**: (TBD)

---

### P9: spec §6.1 検証手順 全 9 ステップ実行 (U1〜U9)

**目的**: spec §6.1 の U1〜U9 を順次実行し受入判定。

**ステップ概要** (spec §6.1 詳細):
1. U1: stereo voice で center 抽出 (center spk から鳴る、front L/R から消える)
2. U2: wide stereo music で rear 展開 (Ls/Rs に side 成分)
3. U3: bass 強調素材で LFE 経路 (LFE spk から低音、front L/R に残らない)
4. U4: 5.1 native + `{upmix:on}` で auto bypass + chat 通知
5. U5: `{upmix:on}` ↔ `{upmix:off}` の Desc 編集 live 切替
6. U6: `Stream3DUpmix` debug 3 sentinel (`-1`/`0`/`1`)
7. U7: `Stream3DUpmixLfeCutoff` 80→120Hz で LFE 帯域変化
8. U8: `Stream3DUpmixCenterBleed` 1.0→0.0 で center 除去なし
9. U9: `Stream3DUpmixRearDelayMs` 16→8ms で decorrelation 薄化

**完了条件**: spec §6.1 受入表 9 行が全 PASS / 必要なら debug settings の default 値を P11 close-out 時に再調整 (R2/R3/R4 縮退策)。

(in-conversation 実行、commit なし)

---

### P10: r10 / r11 回帰確認

**目的**: `{upmix:off}` (default) または未指定で r10 / r11 完全互換が成立することを実機確認。

**手順**:
- 6ch source (test_routing_5_1.ogg) を r10 placement に流し、`{upmix:on}` でも auto bypass で r10 と完全一致を確認
- stereo source (r10 までの 2ch 配信) を `{upmix:off}` で再生、r11 までと同じ「2 spk のみ鳴る」挙動を確認
- r10 §5.3 受入条件全行を再実行 (placement / dropout / 互換マトリクス / routing 診断)
- r11 §5.5 受入条件全行を再実行 (lite-HRTF / venue reverb / wetgain / 9 venue 段階差)
- r9 / r8 §5 全項目をスポット回帰

**完了条件**: r10 / r11 と挙動完全一致 / 既存配置のユーザは r12 viewer でも `{upmix}` タグ未指定なら何も変わらず動作 / 5.1 native 配信は upmix の有無に関わらず r10/r11 と完全同一動作。

(in-conversation 実行、commit なし)

---

### P11: CPU benchmark + spec close-out

**目的**: spec §6.3 の CPU 受入条件 (r11 baseline 比 +3pp 未満) を実測値で埋め、spec §10 変更履歴に close-out を追記。

**手順**:
- `top -b -d 10 -n 7` で 60 秒サンプリング (初回 0% は捨て 6 サンプル平均)
- PID は `pgrep -f 'do-not-directly-run-ayastorm-bin' | head -n1`
- 4 構成を順次計測:
  - r11 baseline (`{upmix:off}` + r11 default = binaural ON / venue dry)
  - upmix only (`{upmix:on}` + binaural OFF + venue dry) — DSP 単体コスト
  - upmix + binaural ON + venue dry
  - upmix + binaural ON + venue=hall_medium (フルチェイン)
- 5min dropout 0 / URL 切替 ×10 を 4 構成すべてで実施
- 結果を spec §6 / §10 に追記

**完了条件**: spec §6.3 全項目記入完了 / +3pp 目標との実測差を spec に明記 / リリース判断材料そろう。

(in-conversation 実行、spec doc 編集のみ。commit 化は AYA 判断)

---

## 3. マイルストーン依存関係

```
P0 ─→ P1 ─→ P2 ─→ P3 ─→ P4 ─→ P5 ─→ P6 ─┬─→ P9 ─→ P10 ─→ P11
                                          │
                                  P7 ────→│ (P9 開始前まで)
                                  P8 ────→│ (P9 開始前まで、P5 完了後)
```

P0 (仕様 + roadmap doc 同時更新 + 実装箇所調査) は本書策定と同時に実施。**P0 第 2 弾で C 案 (`OpKind::Upmix` 拡張) を確定済み** (`docs/archive/r12/dsp_insertion_survey.md`)、A 案 / B 案は廃案。P1 (`LLStereoUpmix` skeleton) は C 案前提で着手。P2/P3 は P1 完了後に並行可だが、デバッグ容易性のため P2 → P3 を順次実装推奨。P4 (resolveReadOp 分岐 + auto bypass) は P5 (タグ parser) と密結合だが、P4 を先に固めておくと P5 で「タグ ON でも 5.1 source なら upmix dispatch を返さない」を素直に書ける。P6 (debug 3 件) は P2/P3 のパラメータ化が完了している前提。P7 (検証材料) と P8 (LSL UI) は P6 までと並行可。P9 (検証実行) は P1〜P8 全完了後。P10 で回帰、P11 でクローズ。

r11 と異なり、本リリースでは **追加 phase が想定外に発生する可能性** が以下の点で残る:

- ~~**R6 (DSP 挿入箇所 A/B 案)** が P0 で固まらず P1 着手後に切替が必要になるケース~~ → **P0 第 2 弾で C 案として既に解消** (A/B 案は実コード読解で不適合と判明、C 案は Bs775 dispatch の並行構造で実装難易度小)
- **R2/R3/R4 (各 default 値の聴感調整)** が P11 close-out 時に R 項目で記載した範囲を超えて再 tune が必要になるケース → P11.x として default 調整 phase を追加
- **P9 検証中に新タグ (例: bass-only mode 等) が必要と判明** したケース → 仕様書改定 + P12 として後追い phase を追加 (r11 P7c-C / P15 と同じ流儀)

---

## 4. 動作確認チェックリスト (P9 / P10 / P11 結果の P 単位 trace)

`[x]` 実機実測で通過 / `[~]` コードレビューのみ / `[ ]` 未実施 (リリース後の運用観察に委ねる場合あり)。

安定性指標の実測値 (CPU / dropout / URL 切替成功率) は **spec §6.3** を、debug settings の最終 default 値 (R2/R3/R4 縮退で調整した場合) は **spec §4.4** を参照。本セクションは「どの P でどの観点を埋めたか」の trace。

### 4.1 r12 新規 (P1〜P8 で実装、P9/P10/P11 で検証)

- [ ] **U1: stereo voice center 抽出** (center spk から鳴る、front L/R から消える) — P9 step 1
- [ ] **U2: wide stereo music rear 展開** (Ls/Rs に side 成分) — P9 step 2
- [ ] **U3: LFE 経路** (LFE spk から低音、front L/R に残らない) — P9 step 3
- [ ] **U4: 5.1 native + `{upmix:on}` で auto bypass + chat 通知** — P9 step 4
- [ ] **U5: タグ live 切替** (`{upmix:on}` ↔ `{upmix:off}` で次の `evaluateLinkset()` から rebuild) — P9 step 5
- [ ] **U6: `Stream3DUpmix` 3 sentinel** (`-1`/`0`/`1`) — P9 step 6
- [ ] **U7: `Stream3DUpmixLfeCutoff` 80→120Hz で LFE 帯域変化** — P9 step 7
- [ ] **U8: `Stream3DUpmixCenterBleed` 1.0→0.0 で center 除去なし** — P9 step 8
- [ ] **U9: `Stream3DUpmixRearDelayMs` 16→8ms で decorrelation 薄化** — P9 step 9
- [ ] **5min dropout 0** (stereo + upmix + venue=hall_medium + binaural ON) — P11
- [ ] **URL 切替 ×10** (stereo upmix ↔ 5.1 native ↔ stereo non-upmix の組合せ) — P11
- [ ] **CPU r11 比 +3pp 未満** — P11
- [ ] **配信者 LSL から `{upmix}` タグ編集可** (Default 含めて menu/dialog で完結) — P9 step 5 / P8

### 4.2 r10 / r11 互換 (回帰確認、P10)

- [ ] r11 §5.5 受入条件全行が回帰なし (`{upmix:off}` または未指定で完全互換)
- [ ] r10 §5.3 受入条件全行が回帰なし (5.1 native は upmix の有無に関わらず同一)
- [ ] r9 / r8 互換 (既存配置のタグ無改修動作) スポット回帰
- [~] codec 別: Vorbis のみ end-to-end 実機回し、Opus / FLAC は r9 確立経路の流用でコードレビューのみ

---

## 5. 参照リンク

| 項目 | 参照先 |
|---|---|
| 設計判断 (旧 r12 計画降格 / アルゴリズム決め打ち) | `docs/specs/spec_stereo_upmix.md` §1 / §2.2 / §2.3 |
| リスク R1〜R6 (内容 + 縮退策) | `docs/specs/spec_stereo_upmix.md` §7 |
| 受入条件表 (U1〜U9 + 互換 + 安定性) | `docs/specs/spec_stereo_upmix.md` §6 |
| Matrix decode / center bleed / rear decorrelation / LFE LPF 詳細 | `docs/specs/spec_stereo_upmix.md` §4.3 |
| debug settings 4 件 (sentinel + 微調整 3 件) | `docs/specs/spec_stereo_upmix.md` §4.4 |
| r11 既存タグとの並行動作 | `docs/specs/spec_stereo_upmix.md` §4.5 |
| r13+ への持ち越し | `docs/specs/spec_stereo_upmix.md` §9 |
| r11 impl record (本書のテンプレート) | `docs/ayastorm-r11-binaural-venue-reverb.md` |
| r11 spec (前提) | `docs/specs/spec_binaural_venue_reverb.md` |
| r10 spec (前提) | `docs/specs/spec_5_1ch_placement.md` |
| r9 spec (前提) | `docs/specs/spec_5_1ch_source.md` |
| 検証材料生成 | `docs/archive/r12/gen_upmix_test_material.sh` (P7 で作成予定) |
| roadmap | `docs/ayastorm-stream3d-roadmap.md` §3 r12 |

---

## 6. r12.1 follow-on (2026-05-09)

r12 リリース直後に AYA からの実 listening フィードバックで判明した 3 件を r12.1 として後追い対応。phase 番号は付けず単一 commit でまとめた (テスト規模が小さく phase 分割するメリットが薄い)。

### 6.1 `{lfegain:N}` (短縮形 `lg`) 追加

`{ch:LFE}` 経路と `{upmix:on}` 時の LFE band に対する root prim タグ (値域 0.0〜4.0、default 1.0)。spec §4.7.1 で定義、tag-guide §7.4 として詳細記述。listener 側 sentinel `Stream3DLfeGain` (sentinel `-1.0` = タグ通り) も同期追加 (spec §4.7.2)。

実装は `LLPositionalStreamMulti` 側で per-speaker callback の出力に乗算 (mix 後段)。dry/wet とは独立 — frequency response には触らず純粋な linear gain。LSL setup script の dialog にも `lg` 設定を追加。

### 6.2 `wetgain` default `1.0` → `0.2`

実 listening で hall_medium / cathedral 等の長尾 venue では `1.0` が音楽的に飽和することが判明。musical range 0.1〜0.5 を反映して default を `0.2` に下げ、LSL UI の quick-pick も `0.5` / `1.0` / ... 系列から `0.1`〜`0.5` 細刻みに刷新。spec §4.7 / tag-guide §7.3 改訂履歴に明記。

### 6.3 listener 側 debug settings の live-tuning 回帰修正

r12 リリース時、以下 7 setting が値変更後にプリムタッチまで反映されない仕様回帰があった:

- `Stream3DUpmixLfeCutoff` / `Stream3DUpmixCenterBleed` / `Stream3DUpmixRearDelayMs`
- `Stream3DVenueOverride` / `Stream3DVenueWetGain` / `Stream3DLfeGain`
- `Stream3DVolumeMaster`

原因は `applyLfeGainToBinding` / `applyVenueToBinding` / `applyWetGainToBinding` および `setUpmixTuning` / `setVolume` の push がイベント駆動経路 (Description parse / 新規 binding 生成) からしか呼ばれていなかったこと。`LLPositionalStreamMgr::update()` のポーリングループ (distributed bindings 反復および mono bindings 反復) に **per-poll push** を追加して「次フレーム反映」に戻した。push 自体は idempotent setter (= 値が変わっていなければ早期 return) なので追加コスト無視可。spec §4.7.3 / tag-guide §12.3 の注記に詳細記述。

### 6.4 既知の限界 (r12.x / r13 へ繰越)

bus-tail に置かれた `VenueReverbDsp` は **stereo IR convolver** で、`if (inchannels != 2 \|\| outchannels != 2) return;` の guard により Stream3DGroup の 2ch master mix に対してのみ wet を生成する。FMOD speaker mode は `llaudioengine_fmodstudio.cpp:356` で `STEREO` に固定されており、6 spk placement や upmix 経路の各 channel を独立に処理する設計にはなっていない。これにより、配信者が venue=hall_medium 等を有効化したとき「FL/FR からのみ reverb が聴こえ、C/SL/SR/LFE プリムには wet が乗らない」ように知覚されるケースが出る (実は 6 spk すべて同じ Stream3DGroup を経由しているが、wet が master stereo 像として固定されるため per-spk 的な空間表現が出ない)。

設計上の architectural limitation として r12.1 では受け入れる。改善候補は r13+ で検討 (per-channel reverb / pre-3D send model / L/R wet decorrelation の 3 案)。

### 6.5 commit

`a6389421c8` (feature/r12.1-lfegain): 8 files changed, 525 insertions(+), 513 deletions(-)。viewer / LSL / settings.xml / コメント更新を一括。
