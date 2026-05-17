# AYAstorm r11: バイノーラル + 会場残響 実装工程

**作成日**: 2026-05-07 (リリース直前 retrospective)
**対象**: AYAstorm `feature/aya-r11-binaural-venue-reverb` (PR 未提出 / リリース判断保留中)
**仕様**: `docs/specs/spec_binaural_venue_reverb.md`
**前リリース**: r10.x (`v7.2.5-ayastorm-r10.x`)

> **本書の役割**: phase 単位の作業内容と依存関係、検証チェックリストを記録する。
> リスク (`spec §8`)、受入結果 / 安定性指標 / 既知の制約 / r12+ への持ち越し (`spec §13`) は仕様書側を canonical とし、本書では参照する。

---

## 1. ゴール

r10 までで完成した **Layer 1 (placement)** に **Layer 2 (rendering)** を載せる初のリリース。Layer 2 = ITD/ILD ベースの **lite-HRTF** + **venue convolution reverb**。

設計の核は **配信者主導モデル**: 親プリム Desc に追加した 3 タグキー (`{binaural}` / `{venue:NAME}` / `{wetgain:N}`) が root truth。listener viewer はタグを忠実にレンダリングし、Preferences UI 改修は一切行わない。実装/検証時の独立 toggle 用に debug settings 3 件 (sentinel 値 = 「タグ通り」) のみ提供。

SOFA 個人 HRTF / Steam Audio integration は r11 では非対象、r12+ への保留。spec §1 の構成判断 (旧 r5 NG1 反転 / 旧 Steam Audio 案棄却) は仕様書側を参照。

---

## 2. フェーズ分解

viewer-only の改修。配信側パイプラインは r9 / r10 流用。P11 でテスト材料生成スクリプトのみ追加。

### P0: 仕様確定 + roadmap doc 同時更新

**目的**: 旧 r11 案 (Steam Audio + 任意 SOFA) を spec / roadmap 双方で「lite-HRTF + venue convolution reverb (配信者主導モデル)」に置き換え、以降を進める誰もが同じ前提で動ける状態にする。

**ファイル**:
- `docs/specs/spec_binaural_venue_reverb.md` (新規、初版 → 配信者主導モデル全面書き換え → URL pre-resolve P10 組込み)
- `docs/ayastorm-stream3d-roadmap.md` §3 r11 (Steam Audio 案 → 新仕様)

**完了条件**: 仕様書 AYA レビュー通過 / roadmap §3 r11 が新仕様で読める / Preferences UI 改修ゼロ方針が明文化。

**commit**: `fb14ac3c5a` (初版) → `f1d6d7468e` (配信者主導モデル) → `dea58e56c7` (URL pre-resolve 組込み) → `9a0fc812bc` / `945319f3bc` (roadmap)

---

### P1: Stream3D ChannelGroup 分離 (R5 リスク早期検証)

**目的**: 3D Stream の per-speaker channel が r10 まで master 直結だった状態を正常化し、`Stream3D` 専用 ChannelGroup を作成。lite-HRTF / venue reverb DSP の挿入点を確保。

**ファイル**:
- `indra/llaudio/llaudioengine_fmodstudio.{h,cpp}` (Stream3D group 作成)
- `indra/llaudio/llpositionalstreammulti.cpp` (`makeChannelForBinding()` で `setChannelGroup`)

**完了条件**: 起動時 Stream3D group 作成成功 / 全 3D Stream channel が group 配下 / **master volume / mute / 全体 setVolume が Stream3D group に正しく伝播** (R5 早期検証クリア) / 既存 voice / UI / SFX / ambient world sound に影響なし。

**commit**: `077e672a2d` → `abeffc9a7d` (PR #42 経由マージ)

---

### P2: LiteHrtfDsp skeleton (mono → stereo + 距離減衰)

**目的**: FMOD `FMOD_DSP_DESCRIPTION` 形式の per-channel DSP 骨格を作る。最初は距離減衰 (linear-square) のみ実装し、未配線で動作確認。

**ファイル**:
- `indra/llaudio/lllitehrtfdsp.{h,cpp}` (新規)

**完了条件**: DSP create / read callback / release が FMOD 経由で呼び出せる / 1ch mono in → 2ch stereo out (L/R 同値) / `set3DMinMaxDistance` 値の linear-square 距離減衰が r10 と数値一致。

**commit**: `95cd25f904`

---

### P3: ITD + ILD shadow + air absorption + elevation 実装

**目的**: lite-HRTF の本体。ITD (Woodworth-Schlosberg)、ILD shadow (cos(θ/2)^0.5 + 1st-order IIR low-shelf)、air absorption (-0.5 dB/m hi-shelf @ 4kHz, cap -25 dB)、elevation tilt (±1 dB, ±30°) を実装。

**ファイル**:
- `indra/llaudio/lllitehrtfdsp.cpp` (read callback 拡張)

**完了条件**: 仕様 §4.3.4〜§4.3.7 の数式通り / 3rd-order Lagrange fractional delay (ITD) / 1st-order IIR (air / ILD shadow) / 単体テスト (impulse response 形状目視) で各 DSP 段が想定通り。

**commit**: `5e64cd0155`

---

### P4: per-frame param push 経路

**目的**: main thread から DSP (mixer thread) へ source pos / listener pose / range を atomic-write で per-frame 渡す経路を作る。

**ファイル**:
- `indra/llaudio/lllitehrtfdsp.{h,cpp}` (atomic param fields)
- `indra/llaudio/llpositionalstreammulti.cpp` (per-update push)

**完了条件**: lock-free single-writer atomic で thread race なし / mixer thread 側で 1 frame 古い値の読み込みが視聴上問題なし / mutex / condition variable 不使用 (r7 P5 と同方針)。

**commit**: `c4a4cc7700`

---

### P5: `{binaural}` タグ + `Stream3DBinauralRender` + Channel::addDSP 配線

**目的**: 配信者タグ `{binaural:on|off}` parser、debug settings `Stream3DBinauralRender` (int, sentinel `-1`)、`effectiveBinaural()` 合成 getter、`Channel::set3DLevel` の hook 反転 (= ON で `0.0f`、OFF で `1.0f` のまま)、`Channel::addDSP(LiteHrtfDsp)` 挿入を実装。

**ファイル**:
- `indra/newview/llpositionalstreammgr.{h,cpp}` (parser、`mBinaural`、`effectiveBinaural()`)
- `indra/newview/app_settings/settings.xml` (`Stream3DBinauralRender` 追加)
- `indra/llaudio/llpositionalstreammulti.cpp` (`makeChannelForBinding()` の hook 反転 + addDSP)

**完了条件**: タグ未指定で binaural ON / `{binaural:off}` で OFF / debug `=0/=1/=-1` の 3 sentinel が正しく動作 / `=0` + ChannelGroup 分離 OFF 等価で r10 完全一致。

**commit**: `e79c0aeec1`

#### P5.1 (sub): LL_DEBUGS gate 観測

binaural gate 状態 (タグ値 / debug 値 / 実効値) を `LL_DEBUGS("Stream3DBinaural")` で出力できるようにし、検証時の挙動 trace を可能に。

**commit**: `939e2bff0a`

---

### P6: VenueReverbDsp skeleton (stereo passthrough、未配線)

**目的**: group 末尾 DSP の骨格。最初は stereo passthrough (実質 no-op) で配線なし、ロジックは P7 で実装。

**ファイル**:
- `indra/llaudio/llvenuereverbdsp.{h,cpp}` (新規)

**完了条件**: DSP create / read callback / release が FMOD 経由で呼び出せる / stereo in → stereo out passthrough。

**commit**: `258dc275e0`

---

### P7: VenueReverb 完成 (a/b/c/c-B/c-C の 5 サブステップ)

partitioned overlap-save convolution + WAV mini-reader + slot model + 8 venue IR 投入 + Stream3D group 末尾 attach。

**P7a — partitioned overlap-save convolution + 自前 radix-2 FFT** (`2480e46b84`):
- 1024-sample block の partitioned FFT convolution (uniform partition)
- 自前 radix-2 FFT (vendor 依存ゼロ、KissFFT / FMOD 内蔵 FFT 不採用)
- stereo IR ≤ 3s

**P7b — WAV mini-reader + slot model 化 (lock-free IR swap)** (`bdddb3ebd0`):
- WAV 16-bit PCM / 32-bit float の mini-reader
- IR slot model: 起動時に全 venue IR を RAM ロード、main thread が venue index atomic を更新で mixer thread が次 callback で slot 切替
- 200ms crossfade で wave-front 不連続吸収 (R7)

**P7c — N-slot 再設計 + Stream3D group 末尾 attach + IR bundle 骨格** (`8cb0b52fa9`):
- 9 venue (dry + 8 IR) を全 slot 抱え、index で選択
- `ChannelGroup::addDSP(end, VenueReverbDsp)` を Stream3D group に挿入
- `app_settings/venue_ir/` ディレクトリ骨格 + IR bundle メタデータ

**P7c-B — 8 venue IR 投入 + CREDITS.md** (`3f945d5d85`):
- OpenAIR (CC-BY 4.0) から 8 IR を選定 / DL / 変換 (48kHz resample, ≤ 3s truncate)
- `app_settings/venue_ir/CREDITS.md` に出典明記
- `docs/archive/r11/fetch_venue_irs.sh` で再現可能な DL/変換 helper

**P7c-C — venue IR unity-gain 正規化** (`057591334a`):
- `LLVenueReverbDsp::create()` 内に正規化 pass を追加。各 IR を Σs² == 1 per channel に scale し、convolution operator gain を unity に揃える。
- 連続入力 RMS=R に対して wet 出力 RMS≈R が出るので、`{wetgain:N}` は純粋な dry/wet 比 (1.0 = wet equal to dry, 0.5 = half-mix) として効くようになり、venue を切り替えても標準値を retune する必要がない。
- `loadVenueSlot()` を `stageVenueIR()` (load + Σs² 測定のみ) に分割し、`create()` を 2-pass 化 (Pass 1: 全 venue stage / Pass 2: norm = 1/√energy で in-place scale → primeSlot)。
- 起動ログに per-venue `energy=X norm=Y` を出力 (検証用 trace)。
- きっかけ: P12 検証中に「venue 横断で wetgain の標準値が決められない (cathedral overload / room 薄)」と判明。仕様 §4.4.6 で正規化基準を明文化。

**完了条件 (全 P7 step 完了時)**: 9 venue で会場感の段階差が出る / venue index 切替が live で反映 / IR ロード失敗時の chat 通知 throttle / CREDITS.md attribution 完備 / unity-gain 正規化により venue 横断で wetgain 同一値が同じ濃さで効く。

---

### P8: `{venue:NAME}` タグ + `Stream3DVenueOverride` 配線

**目的**: 配信者タグ `{venue:NAME}` parser、debug `Stream3DVenueOverride` (string, sentinel `""`)、`effectiveVenue()` 合成 getter、不正値 chat 通知 (r10.x `notifyStream3D()` helper 経由)。

**ファイル**:
- `indra/newview/llpositionalstreammgr.{h,cpp}` (parser、`mVenue`、`effectiveVenue()`、9 venue 名 LUT)
- `indra/newview/app_settings/settings.xml` (`Stream3DVenueOverride` 追加)

**完了条件**: 9 venue 名 (`dry` / `room_small` / ... / `cathedral` / `outdoor`) が大文字小文字区別なしで認識 / 不正値 silent ignore + chat 通知 1 回 / debug が空文字でタグ通り / venue 名で listener override / `"dry"` で reverb DSP 完全 bypass。

**commit**: `98b6de18b8`

---

### P9: `{wetgain:N}` タグ + `Stream3DVenueWetGain` 配線

**目的**: 配信者タグ `{wetgain:N}` parser (F32, 0.0〜2.0 clamp)、debug `Stream3DVenueWetGain` (F32, sentinel `-1.0`)、`effectiveWetGain()` 合成 getter。

**ファイル**:
- `indra/newview/llpositionalstreammgr.{h,cpp}` (parser、`mWetGain`、`effectiveWetGain()`)
- `indra/newview/app_settings/settings.xml` (`Stream3DVenueWetGain` 追加)

**完了条件**: タグ未指定で wet=1.0 / `{wetgain:0.5}` `{wetgain:1.5}` で wet 強度可変 / debug `=-1.0` でタグ通り、`>=0.0` で強制上書き / dry signal は zero-latency bypass 保持。

**commit**: `21ec078504`

---

### P10: viewer-side URL pre-resolve (HTTPS→HTTP cross-protocol redirect)

**目的**: 配信者音源が HTTPS URL を返し HTTP に redirect する構成 (Icecast 等) で FMOD `createStream` が直渡しで落ちる問題を viewer 側で先解決。

**ファイル**:
- `indra/llaudio/llpositionalstreamstereo.cpp` (URL pre-resolve helper)
- `indra/newview/llpositionalstreammgr.cpp` (start 前に pre-resolve 呼出)

**完了条件**: HTTPS → HTTP redirect が viewer で吸収され、FMOD には最終 HTTP URL のみ渡る / pre-resolve 失敗時は元 URL でフォールバック / 既存 HTTP / HTTPS-only 構成は無影響。

**commit**: `c1e03498e7`

(spec §11 では当初フェーズになかったが、実装中に必要性が判明し追加。spec doc も `dea58e56c7` で P10 を組み込んだ書き換え済み)

---

### P11: 検証材料生成スクリプト + AYA hosting

**目的**: spec §5.1 の binaural 検証材料 3 種 (pink_noise_30s / voice_panning_test / click_train_440Hz) と venue 検証材料の生成スクリプトを repo に置き、AYA 環境で hosting。

**ファイル**:
- `docs/archive/r11/gen_test_material.sh` (sox + espeak-ng + oggenc)
- `docs/archive/r11/fetch_venue_irs.sh` (P7c-B 由来、IR DL/変換 helper)

**完了条件 (前段)**: 3 素材 + venue 検証材料が再現可能に生成 / Vorbis (`oggenc --quality 5`) のみ。

**完了条件 (後段)**: AYA LAN 環境で 4 本 (3 binaural + 1 r10 5.1ch placement test_routing_5_1.ogg) が viewer から再生可能。

**commit**: `ff74d6a535`

(後段は当初 Icecast 配信を想定したが、Icecast は HTTP HEAD 400 / Range 400 を返し FMOD `createStream` の content probe / seek が落ちる。代替として Python `http.server` も Range 非対応で同症状 → AYA 環境専用の Range 対応 HTTP server `/tmp/serve_range2.py` を立て、`192.168.0.2:8080` で素材 hosting。Icecast 構成は破棄、本番運用想定の HTTPS→HTTP redirect は P10 で吸収済)

---

### P12: §5.3 検証手順 全 8 ステップ実行

**目的**: spec §5.3 の 8 ステップ検証を順次実行し受入判定。

**ステップ概要** (spec §5.3 詳細):
1. binaural 単体 (voice_panning A/B) — vanilla 3D positioning が残る前提で PASS w/ note
2. air absorption (pink_noise 5m/25m/50m) — 主観 PASS、客観 FFT は r11.x へ
3. ITD (click_train_440Hz 真横) — PASS
4. r10 placement 合わせ込み (test_routing_5_1.ogg) — P13 と統合
5. venue 段階差 + wetgain 段階差 — PASS (wetgain で派手さ可変確認)
6. debug settings 3 件 sentinel 動作 — PASS (空文字 / venue 名 / "dry" 全パターン)
7. Stream3D group 隔離 (voice / UI / ambient に reverb 漏れない) — PASS
8. 回帰 (P13 へ移譲)

**完了条件**: spec §5.5 受入表 14 行のうち、§5.3 由来 9 行が全 PASS (CPU / dropout / URL 切替 / r10 互換は P13/P14)。

(in-conversation 実行、commit なし)

---

### P13: r8/r9/r10 回帰確認

**目的**: debug `Stream3DBinauralRender=0` + `Stream3DVenueOverride="dry"` で r10 完全互換が成立することを実機確認。

**手順**:
- 6ch source (test_routing_5_1.ogg) を ch:M downmix path (`mix6chToMono` / BS.775) で受ける
- `Stream3DBinauralRender=0` で LiteHrtfDsp 完全 detach、`Stream3DVenueOverride="dry"` で VenueReverbDsp 完全 bypass
- r10 §5.3 受入条件全行を再実行 (placement / dropout / 互換マトリクス / routing 診断)
- r9 / r8 §5 全項目をスポット回帰

**完了条件**: r10 と挙動完全一致 / 既存配置のユーザは r11 viewer でも何も変わらず動作。

(in-conversation 実行、commit なし)

---

### P14: CPU benchmark + spec §13 close-out

**目的**: spec §5.5 の CPU 受入条件を実測値で埋め、spec §13 受入クローズを書き上げる。

**手順**:
- `top -b -d 10 -n 7` で 60 秒サンプリング (初回 0% は捨て 6 サンプル平均)
- PID は `pgrep -f 'do-not-directly-run-ayastorm-bin' | head -n1` (Linux Firestorm/AYAstorm の本体プロセス名)
- 6 構成を順次計測: baseline (binaural=0/venue=dry) / LiteHrtf only (binaural=-1/venue=dry) / room_small / hall_medium / hall_large / cathedral
- 結果を spec §5.5 / §13 に追記

**実測結果** (詳細 spec §5.5 / §13.5):
- baseline: 43.2% / LiteHrtf only: 43.2% (+0.0pp) / room_small: 43.3% (+0.1pp) / hall_medium: 50.9% (+7.7pp) / hall_large: 52.8% (+9.6pp) / cathedral: 53.4% (+10.2pp)
- LiteHrtf 自体は実質ゼロコスト、cost は VenueReverb が支配的で IR 長に比例
- spec 「+10% 未満」を相対比で読むと hall_medium 以上で超過。**ただし絶対値・dropout 共に問題なく、配信者がタグで opt-in する選択肢として全 9 venue 出荷判断**
- 5min 連続再生 dropout 0 / URL 切替 ×10 (5.1 ↔ 2ch) 全成功

**完了条件**: spec §5.5 / §13 全項目記入完了 / release note 「hall_medium 以上 CPU 重い、低スペック機は room 系推奨」明記。

(in-conversation 実行、spec doc 編集のみ。commit 化は AYA 判断)

---

### P15: 配信者 LSL 拡張 + urlsave 廃止

**目的**: 配信者が r11 の新タグ (`{binaural}` / `{venue}` / `{wetgain}`) を Desc 直接編集ではなく LSL menu / dialog から設定できるようにする。同時に LSL 内の `urlsave` 自動退避/復元機構を撤去し、書いた URL がそのまま反映される透明性モデルに揃える。

**背景**: r11 viewer 側は P5/P8/P9 でタグ実装済みだが、配信者向け sample LSL (`docs/guides/lsl/aya_3dstream_setup.lsl`) が r10 までのキー (url/ch/range/volume) しか UI を持たず、新タグだけは Desc 手書きが必要だった。配信者向け原則「viewer に追加した Desc タグは LSL から全部設定可能」(memory `feedback_lsl_mirrors_viewer_tags.md`) を満たすため P15 として後付け。urlsave 廃止は P12 検証中に「URL を変えたのに前 URL に戻る」報告があり、原因が LSL の自動退避/復元と判明したため同 commit で撤去。

**ファイル**:
- `docs/guides/lsl/aya_3dstream_setup.lsl` (mode token / dialog / handler / fmtCurrent / buildTagBody / handleStop / handleStart)

**実装**:
- ROOT メニュー: 8 → 11 ボタン (Binaural / Venue / WetGain 追加)
- 各 dialog の "Default" は `dropField` で当該タグを削除し viewer 側 default (= sentinel 経由で debug setting が効く状態) に戻す
- `{wetgain:N}` の Custom は `llTextBox` で任意値入力 (LSL 仕様上、初期値はセット不可)
- urlsave 廃止: Stop は `{url:}` を `dropField` するだけ、Start は url が既存なら no-op

**完了条件**: 全 r11 タグが LSL menu / dialog から編集可 / Default ボタンで viewer default に戻せる / Stop/Start で urlsave 退避復元しない / 書き込んだ URL がそのまま Desc に維持される。

**commit**: `dd23d7bf34`

---

## 3. マイルストーン依存関係

```
P0 ─→ P1 ─→ ┬─→ P2 ─→ P3 ─→ P4 ─→ P5 ──┐
            │                              │
            └→ P6 ─→ P7 ─────────────────→ ├─→ P12 ─→ P13 ─→ P14 ─→ P7c-C ─→ P15
                     P8 (P5 と並行可) ───→  │
                     P9 (P7/P8 完了後) ───→ │
                     P10 (P5/P8 と並行可) → │
                     P11 (P12 開始前まで) ┘
```

P0 (仕様 + roadmap doc 同時更新) は本書策定と同タイミングで roadmap §3 を整合させ、以降を進める誰もが同じ前提で動ける状態を作った。P1 (Stream3D group 分離) は両 DSP の前提なので最先着、**P1 完了直後に R5 (master volume 伝播) の早期検証** を実施しリスクを潰した。P2-P5 (LiteHrtfDsp 系統) と P6-P9 (VenueReverbDsp 系統) は P1 完了後に独立並行可。P10 (URL pre-resolve) は当初 phase 表になく、検証段階での実需で追加。P12 (検証) は実装全完了後、P13 で回帰、P14 でクローズ。**P7c-C (IR unity-gain 正規化) と P15 (配信者 LSL 拡張 + urlsave 廃止) は P14 後の追加 phase で、P12 検証中に判明した課題 (wetgain 標準値が venue 横断で決まらない / LSL から r11 タグを編集できない / URL 自動復元による配信者の混乱) を後追いで埋めるもの。**

---

## 4. 動作確認チェックリスト (P12 / P13 / P14 結果の P 単位 trace)

`[x]` 実機実測で通過 / `[~]` コードレビューのみ / `[ ]` リリース後の運用観察に委ねる。

安定性指標の実測値 (CPU / dropout / URL 切替成功率) は **spec §5.5** を、CPU の venue 別ブレークダウン / 既知制約は **spec §13.5** を参照。本セクションは「どの P でどの観点を埋めたか」の trace。

### 4.1 r11 新規 (P2〜P11 で実装、P12/P13/P14 で検証、P7c-C/P15 で後追い改善)

- [x] **binaural 360° 追従** (voice_panning_test、avatar 回転): 配信者タグ `{binaural}` 未指定で頭の向き追従 (P12 step 1)
- [x] **binaural on/off A/B**: vanilla 3D positioning が binaural off でも残る挙動を確認、voice/pink_noise では差は体感しにくいが PASS (P12 step 1, memory `feedback_binaural_off_residual_3d_positioning.md` 参照)
- [~] **air absorption 客観 -15dB @ 4kHz**: P12 step 2 で主観 PASS、客観 FFT 測定は r11.x へ持ち越し
- [x] **ITD 真横 click 着信差**: click_train_440Hz を avatar 真横に配置、左右耳着信差を聴感確認 (P12 step 3)
- [x] **9 venue 段階差**: dry → room_medium → hall_medium → cathedral の順で会場感が段階的に増す (P12 step 5)
- [x] **wetgain 段階差**: `{wetgain:0.5/1.0/1.5}` で wet 強度可変 (P12 step 5)
- [x] **タグ live 切替**: `{binaural}` / `{venue}` / `{wetgain}` の Desc 編集が次の evaluateLinkset から効く (P12 step 5/6)
- [x] **Stream3DBinauralRender 3 sentinel**: `-1`/`0`/`1` で「タグ通り / 強制 OFF / 強制 ON」(P12 step 6)
- [x] **Stream3DVenueOverride 3 状態**: 空文字 / venue 名 / `"dry"` で「タグ通り / 強制差し替え / 強制 dry」(P12 step 6)
- [x] **Stream3DVenueWetGain sentinel**: `-1.0` でタグ通り、`>=0.0` で強制上書き (P12 step 6 中で確認)
- [x] **Stream3D group 隔離**: voice / UI sound / ambient world sound に reverb 漏れない (P12 step 7)
- [x] **5min dropout 0**: cathedral + binaural ON で 5 分連続再生、stutter / 無音化なし (P14)
- [x] **URL 切替 ×10**: 5.1ch ↔ 2ch を 10 回切替で crash / 二重再生 / 残留音 / log エラー繰り返しなし (P14)
- [⚠] **CPU r10 比 +10% 未満**: 計測値は spec §5.5 / §13.5 参照。spec 閾値を相対比で読むと hall_medium 以上で超過するが、絶対値・dropout 共に問題なく ship-with-note 判断 (P14)
- [x] **venue 横断で wetgain が同じ濃さ**: IR unity-gain 正規化 (Σs²=1 per channel) により room_small / cathedral / outdoor で `{wetgain:1.0}` が同等の dry/wet 比に揃う (P7c-C)
- [x] **配信者 LSL から r11 タグ全部編集可**: `aya_3dstream_setup.lsl` の menu / dialog から `{binaural}` / `{venue}` / `{wetgain}` を Default 含めて設定可、Desc 直接編集不要 (P15)
- [x] **書いた URL がそのまま維持される**: LSL urlsave 自動復元を廃止、再生失敗時に裏で前 URL に戻らない (P15)

### 4.2 r10 / r9 / r8 互換 (回帰確認、P13)

- [x] r10 §5.3 受入条件全行が回帰なし (debug `Stream3DBinauralRender=0` + `Stream3DVenueOverride="dry"` で r10 完全一致)
- [x] 6ch source × ch:M (BS.775 downmix) が r10 と同等
- [x] r9 / r8 互換 (既存配置のタグ無改修動作) スポット回帰 OK
- [~] codec 別: Vorbis のみ end-to-end 実機回し、Opus / FLAC は r9 確立経路の流用でコードレビューのみ (spec §5.4)

---

## 5. 参照リンク

| 項目 | 参照先 |
|---|---|
| 設計判断 (旧 r5 NG1 反転 / 旧 Steam Audio 案棄却) | `docs/specs/spec_binaural_venue_reverb.md` §1 / §2 |
| リスク R1〜R8 (内容 + 対策方針) | `docs/specs/spec_binaural_venue_reverb.md` §8 |
| 受入条件表 (実測値含む) | `docs/specs/spec_binaural_venue_reverb.md` §5.5 |
| 実装サマリ + DSP 詳細 | `docs/specs/spec_binaural_venue_reverb.md` §13.1 / §4.3 / §4.4 |
| §5.4 安定性指標まとめ | `docs/specs/spec_binaural_venue_reverb.md` §13.2 |
| codec 別 end-to-end 検証状況 | `docs/specs/spec_binaural_venue_reverb.md` §13.4 / §5.4 |
| 既知の運用上の制約 (CPU note 含む) | `docs/specs/spec_binaural_venue_reverb.md` §13.5 |
| r12+ への持ち越し | `docs/specs/spec_binaural_venue_reverb.md` §13.6 / §9 |
| GitHub release 告知文案 | `docs/ayastorm-r11-release-note.md` |
| IR ライセンス / attribution | `indra/newview/app_settings/venue_ir/CREDITS.md` |
| 検証材料生成 | `docs/archive/r11/gen_test_material.sh` / `docs/archive/r11/fetch_venue_irs.sh` |
