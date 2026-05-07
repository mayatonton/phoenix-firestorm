# AYAstorm r12: stereo→5.1 upmix 実装工程

**作成日**: 2026-05-07 (r11 完了直後の r12 計画策定時)
**対象**: AYAstorm `feature/aya-r12-stereo-upmix` (予定 / 着手前)
**仕様**: `doc/spec_stereo_upmix.md`
**前リリース**: r11 (`feature/aya-r11-binaural-venue-reverb`、リリース判断保留中)

> **本書の役割**: phase 単位の作業内容と依存関係、検証チェックリストの **テンプレート** を記録する。
> リスク (`spec §7`)、受入条件 (`spec §6`)、r13+ への持ち越し (`spec §9`) は仕様書側を canonical とし、本書では参照する。
> 本書は r12 着手前の **計画スナップショット** であり、実装進行に伴って commit hash / 実測値を埋めていく。

---

## 1. ゴール

r10 で完成した **Layer 1 (per-channel placement)** と r11 で完成した **Layer 2 (lite-HRTF + venue reverb)** を、SL で実際に流通している **stereo 配信** にも届けるリリース。

設計の核は **配信者主導モデルの維持** (r11 で確立): 親プリム Desc に追加した 1 タグキー (`{upmix:on|off}`) が root truth、default `off`、source ch>=6 では auto bypass。listener 側の Preferences UI 改修は一切行わない。実装/検証時の独立 toggle / パラメータ微調整用に debug settings 4 件 (sentinel 1 件 + 実値 default 3 件) のみ提供。

アルゴリズムは **DPL2 系 matrix decode + 帯域分離 (LFE LPF / center bleed 除去 / rear decorrelation) で決め打ち**。SOFA per-source HRTF / Steam Audio integration / VenueReverb CPU 最適化 / air absorption 客観測定 / 個人 HRTF / 公開 README は r12 では非対象、r13+ への保留。spec §1 / §2.2 の構成判断 (旧 r12 計画降格 / アルゴリズム決め打ち) は仕様書側を参照。

---

## 2. フェーズ分解

viewer-only の改修。配信側パイプラインは r9 / r10 / r11 流用。検証材料は r11 流儀で `doc/r12/` に sox/ffmpeg ベースのスクリプトを置く。

### P0: 仕様確定 + roadmap doc 同時更新 + 実装箇所調査

**目的**: 旧 r12 案 (SOFA + Steam Audio + 個人 HRTF + 公開 README + air absorption 客観測定) を spec / roadmap 双方で「stereo→5.1 upmix のみ」に置き換え、r13+ への降格を明文化。同時に StereoUpmixDsp の挿入位置 (A 案: stream-level group の入力段 / B 案: per-stream / per-binding) を実装調査で確定。

**ファイル**:
- `doc/spec_stereo_upmix.md` (新規、本書 §1 / §2 と同構成、初版)
- `docs/ayastorm-stream3d-roadmap.md` §3 r12 (旧 SOFA / Steam Audio 案 → 新仕様、r13+ 項目追加)
- 実装箇所調査メモ (in-conversation、commit には残さない)

**完了条件**: 仕様書 AYA レビュー通過 / roadmap §3 r12 が新仕様で読める / Preferences UI 改修ゼロ方針が明文化 / DSP 挿入位置の A/B 判断 (P1 着手前まで)。

**commit**: (P0 一連、初版 / roadmap 更新 / メモリ更新)

---

### P1: StereoUpmixDsp skeleton (2ch passthrough、未配線)

**目的**: FMOD `FMOD_DSP_DESCRIPTION` 形式の per-stream/group DSP 骨格を作る。最初は 2ch in → 6ch out passthrough (L→FL, R→FR、C/Ls/Rs/LFE は無音) で配線なし、ロジックは P2/P3 で実装。

**ファイル**:
- `indra/llaudio/llstereoupmixdsp.{h,cpp}` (新規)

**完了条件**: DSP create / read callback / release が FMOD 経由で呼び出せる / 2ch in → 6ch out (FL/FR は L/R 通し、C/LFE/SL/SR は 0)。

**commit**: (TBD)

---

### P2: Matrix decode + center bleed 除去 + rear decorrelation 実装

**目的**: spec §4.3.2〜§4.3.4 の DPL2 系 matrix decode 本体。`C = (L+R)/√2`、`S = (L-R)/√2`、`L' = L - C×bleed/√2`、`R' = R - C×bleed/√2`、Ls/Rs は S を short random delay (12〜20ms) で decorrelate。

**ファイル**:
- `indra/llaudio/llstereoupmixdsp.cpp` (read callback 拡張)

**完了条件**: 仕様 §4.3.2〜§4.3.4 の数式通り / `bleed` / `delay` がパラメータ化されており P6 で debug settings から触れる準備済み / 単体テスト (impulse response 形状目視) で center / S / L'/R' / Ls/Rs が想定通り。

**commit**: (TBD)

---

### P3: LFE LPF (Butterworth biquad) 実装

**目的**: spec §4.3.5 の LFE 経路。`LFE = LPF_cutoff((L+R)/2)`、cutoff default 80 Hz、Butterworth 2nd order biquad。bass management 補正は入れない。

**ファイル**:
- `indra/llaudio/llstereoupmixdsp.cpp` (LFE LPF section 追加)

**完了条件**: cutoff default 80Hz で 60Hz 以下が通過、4kHz 以上が -40dB 以上落ちる (FFT 目視) / cutoff パラメータが P6 で debug settings から触れる準備済み。

**commit**: (TBD)

---

### P4: source ch 数判定 + auto bypass

**目的**: spec §4.2.2 の判定経路。stream 開始時に codec layer (r9 確立) から ch 数を取得、`ch == 2 + {upmix:on}` のとき DSP 挿入、`ch >= 6` のときは `{upmix:on}` でも auto bypass + chat 通知 1 回。

**ファイル**:
- `indra/llaudio/llpositionalstreammulti.cpp` または `llaudioengine_fmodstudio.cpp` (P0 で確定した DSP 挿入箇所)
- `indra/newview/llpositionalstreammgr.cpp` (5.1 native + upmix:on 時の chat 通知 throttle)

**完了条件**: source ch == 2 + `{upmix:on}` で DSP 挿入 / source ch >= 6 で `{upmix:on}` でも DSP 非挿入 + chat 通知 / source ch == 1 で DSP 非挿入 (mono 対象外) / source ch == 3/4/5 は到達しない (codec reject) が、到達した場合は安全側で passthrough。

**commit**: (TBD)

---

### P5: `{upmix:on|off}` タグ parser + `Stream3DUpmix` debug 配線

**目的**: 配信者タグ `{upmix:on|off}` parser、debug settings `Stream3DUpmix` (int, sentinel `-1`)、`effectiveUpmix()` 合成 getter、DSP 挿入/削除フックの実装。

**ファイル**:
- `indra/newview/llpositionalstreammgr.{h,cpp}` (parser、`mUpmix`、`effectiveUpmix()`)
- `indra/newview/app_settings/settings.xml` (`Stream3DUpmix` 追加)
- `indra/llaudio/llpositionalstreammulti.cpp` または挿入箇所の DSP attach/detach 経路

**完了条件**: タグ未指定で upmix OFF (default) / `{upmix:on}` で ON / debug `=0/=1/=-1` の 3 sentinel が正しく動作 / `=0` + ChannelGroup 経路 = r10 / r11 完全一致 / live Desc 編集で次の `evaluateLinkset()` から rebuild が走る。

**commit**: (TBD)

---

### P6: debug settings 3 件 (LfeCutoff / CenterBleed / RearDelayMs) 配線

**目的**: spec §4.4 のパラメータ微調整 debug settings 3 件を `settings.xml` に追加し、StereoUpmixDsp が main thread から atomic-write で受け取る経路を作る (r11 P4 と同方針)。

**ファイル**:
- `indra/newview/app_settings/settings.xml` (`Stream3DUpmixLfeCutoff` / `Stream3DUpmixCenterBleed` / `Stream3DUpmixRearDelayMs` 追加)
- `indra/llaudio/llstereoupmixdsp.{h,cpp}` (atomic param fields + per-frame push 経路)
- `indra/newview/llpositionalstreammgr.cpp` (debug settings 値を DSP に push)

**完了条件**: `Stream3DUpmixLfeCutoff` を 80→120Hz に変えると LFE 帯域が広がる / `Stream3DUpmixCenterBleed` を 1.0→0.0 に変えると DPL1 互換 (phantom center 二重像) / `Stream3DUpmixRearDelayMs` を 16→8ms に変えると rear decorrelation が薄くなる / 3 件すべて lock-free atomic write で thread race なし。

**commit**: (TBD)

---

### P7: 検証材料生成スクリプト

**目的**: spec §5 の 4 種素材 (stereo voice / wide stereo music / strong sub bass stereo / 5.1 native 回帰用) を再現可能に生成するスクリプトを `doc/r12/` に置く。

**ファイル**:
- `doc/r12/gen_upmix_test_material.sh` (新規、ffmpeg + sox + espeak-ng + oggenc)

**完了条件**: 4 素材が再現可能に生成 / Vorbis (`oggenc --quality 5`) のみ / r11 の `gen_test_material.sh` と同じ流儀 / AYA hosting 環境 (Range 対応 HTTP server) で再生可能。

**commit**: (TBD)

---

### P8: 配信者 LSL に Upmix UI 追加

**目的**: 配信者が r12 の新タグ (`{upmix}`) を Desc 直接編集ではなく LSL menu / dialog から設定できるようにする。memory `feedback_lsl_mirrors_viewer_tags.md` の原則 (viewer に追加した Desc タグは LSL から全部設定可能) を満たす。

**ファイル**:
- `doc/lsl/aya_3dstream_setup.lsl` (mode token / dialog / handler / fmtCurrent / buildTagBody に Upmix 追加)

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
8. U8: `Stream3DUpmixCenterBleed` 1.0→0.0 で DPL1 互換
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

P0 (仕様 + roadmap doc 同時更新 + 実装箇所調査) は本書策定と同時に実施。P1 (DSP skeleton) は P0 の挿入箇所判断 (A 案 / B 案) を前提とする。P2/P3 は P1 完了後に並行可だが、デバッグ容易性のため P2 → P3 を順次実装推奨。P4 (auto bypass) は P5 (タグ parser) と密結合だが、P4 を先に固めておくと P5 で「タグ ON でも 5.1 source なら DSP 挿入しない」を素直に書ける。P6 (debug 3 件) は P2/P3 のパラメータ化が完了している前提。P7 (検証材料) と P8 (LSL UI) は P6 までと並行可。P9 (検証実行) は P1〜P8 全完了後。P10 で回帰、P11 でクローズ。

r11 と異なり、本リリースでは **追加 phase が想定外に発生する可能性** が以下の点で残る:

- **R6 (DSP 挿入箇所 A/B 案)** が P0 で固まらず P1 着手後に切替が必要になるケース → P1.5 として再構築 phase を追加
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
- [ ] **U8: `Stream3DUpmixCenterBleed` 1.0→0.0 で DPL1 互換** — P9 step 8
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
| 設計判断 (旧 r12 計画降格 / アルゴリズム決め打ち) | `doc/spec_stereo_upmix.md` §1 / §2.2 / §2.3 |
| リスク R1〜R6 (内容 + 縮退策) | `doc/spec_stereo_upmix.md` §7 |
| 受入条件表 (U1〜U9 + 互換 + 安定性) | `doc/spec_stereo_upmix.md` §6 |
| Matrix decode / center bleed / rear decorrelation / LFE LPF 詳細 | `doc/spec_stereo_upmix.md` §4.3 |
| debug settings 4 件 (sentinel + 微調整 3 件) | `doc/spec_stereo_upmix.md` §4.4 |
| r11 既存タグとの並行動作 | `doc/spec_stereo_upmix.md` §4.5 |
| r13+ への持ち越し | `doc/spec_stereo_upmix.md` §9 |
| r11 impl record (本書のテンプレート) | `docs/ayastorm-r11-binaural-venue-reverb.md` |
| r11 spec (前提) | `doc/spec_binaural_venue_reverb.md` |
| r10 spec (前提) | `doc/spec_5_1ch_placement.md` |
| r9 spec (前提) | `doc/spec_5_1ch_source.md` |
| 検証材料生成 | `doc/r12/gen_upmix_test_material.sh` (P7 で作成予定) |
| roadmap | `docs/ayastorm-stream3d-roadmap.md` §3 r12 |
