# sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 handoff: chapter 09 起案完了 + (Q1)-(Q5) AYA 判断仰ぎ + 持越紐付け完了 → AYA commit 指示待ち

**完了日**: 2026-06-03
**位置付け**: 前 handoff (chapter 08) を受けて chapter 09 (phase-roadmap) 起案完了。**AYA commit 指示待ち** → 次 session で commit → chapter 10 (open-questions) 起案 → 設計 chapter 群 01-10 完成 → implementation-phase (= η-29 Phase 0) 入口へ移行。

---

## §0 現在の git state

- **unstaged**: `design/01-overview.md` (§4 進捗表 09 → ✅ 起案済)
- **untracked**: `design/09-phase-roadmap.md` (~330 行) + 本 handoff doc + `tests/` (= **絶対除外**)
- branch = `feature/ayastorm-r41-gl-removal`、commit 13 進み (push 未)

---

## §1 chapter 09 確定形 default (= AYA 判断仰ぎ候補 (Q1)-(Q5) + 持越紐付け)

詳細は **`design/09-phase-roadmap.md` §2.1 / §5.2 / §6.1 / §7.1 / §10 / §11** で集約参照。本 handoff では pointer のみ:

### §1.1 Phase 全体マップ (= chapter 09 §2.1)

| Phase 番号 | scope | sub-step |
|---|---|---|
| Phase 0 | 計測 phase (= 06a-prep §2-§4 実機実施) | η-29 |
| Phase 1 (.A/.B/.C) | codegen + redirect 層整備 | η-30 |
| Phase 2..K | 1 UBO ずつ migration | η-31 以降 |
| Phase K+1/K+2/K+3 | 3 OS 確証 (Linux/Win/Mac) | η-(K+2) 〜 (K+4) |
| Phase K+4 | OpenGL path 撤廃 | η-(K+5) |
| Phase K+5 | release 整備 | η-(K+6) |

**K = (Q1)(Q2) 確定後決定**、想定 5-20。

### §1.2 chapter 09 default 採用案 (= AYA 判断確認次第 §2.1 / §5.2 等を「確定形」に書き換え)

| (Q) | 項目 | default 提案 | 該当節 |
|---|---|---|---|
| (Q1) | 第 1 UBO migration 選定 | Template A = 最小リスク UBO 優先 (singleton → per-Asset → per-Skin → per-program → per-draw) | §5.2 / §11.1 |
| (Q2) | Phase 当たり UBO 数 | A = 1 UBO 厳守 (cluster 例外は AYA 判断) | §11.2 |
| (Q3) | OpenGL path 維持期間 | A = 全 UBO 移行完了まで並走 (Phase K+4 で初撤廃) | §7.1 / §11.3 |
| (Q4) | 3 OS 確証 Phase 順序 | C = Linux 完了後 Win/Mac 並走 | §6.1 / §11.4 |
| (Q5) | Phase 0 計測の Phase 番号化 | A = 独立 Phase η-29 として明示 | §3 / §11.5 |

### §1.3 持越紐付け確定 (= chapter 09 §10)

#### §1.3.1 chapter 07 §12 由来

| 持越 | 解消 Phase | 解消方法 |
|---|---|---|
| (V1') / (V3') / (S3') / (W) | **chapter 10** | chapter 09 内では再判定しない、chapter 10 持越保持 |
| (W2) prealloc N=64 + grow chunk 64 | **Phase 1.C** | default 確定済、Phase 1.C 実装 |
| (R1 = chapter 07 ring buffer) 4 / 16 MB + cvar | **Phase 1.C** | default 確定済、Phase 1.C 実装 |
| (PSC) PSO cache 64 MB | **Phase 1.C** | default 確定済、Phase 1.C 実装 |
| (RF) reflection update fence throttle | **Phase 0** | (H1b) hook と同 build で reflection update 頻度 log 取得 → throttle 判定 |

#### §1.3.2 chapter 08 §17 由来

| 持越 | 解消 Phase | 解消方法 |
|---|---|---|
| (A1)(P)(G/B3)(B1)(B2)(B4)(B5) 7 件 | **Phase 1.A** | Codegen Python 実装 / CMake patch |
| (P-future) unused mask 参照解析 | **Phase 2..K (per UBO)** | 各 UBO migration 時に inventory §7 記録 |
| (cache-grow) cache GC | **Phase 2..K (発動時)** | 増分 build cache 肥大化時 Phase 内 sub-task 発動 |

### §1.4 用語注記

- 本 chapter は AYA 判断仰ぎ候補を (Q1)-(Q5) と表記、chapter 07 §12 持越 (R1 = ring buffer 容量) との表記衝突回避
- handoff §2.3 の前 (chapter 08) では R1-R5 と書いていたが、本 handoff 以降は (Q1)-(Q5) を採用

---

## §2 次 session = AYA commit 指示後 commit phase → chapter 10 起案

### §2.1 commit 単位 (明示パス、`tests/` 厳守除外)

```
git add docs/specs/ayastorm-r41-gl-removal/design/01-overview.md
git add docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md
git add docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-design-chapter-09.md
```

commit subject:
```
docs(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 UBO 全体設計 chapter 09 (phase-roadmap) 起案 + (Q1)-(Q5) AYA 判断仰ぎ + 持越紐付け確定
```

### §2.2 chapter 10 (open-questions) scope

設計 chapter 群 (01-10) 最終 chapter。本 chapter で扱う open question 集約:

1. **chapter 07 §12 持越** (= chapter 10 送り 4 件):
   - (V1') set=1 79 binding → 40/39 split 採用方針
   - (V3') 全 program 共通 layout (V3a) vs program 別 (V3b)
   - (S3') sampler 49 set=3 per-asset 同居 vs 別案
   - (W) `sProgramUboPool` maxSets = 6 vs 1200
2. **chapter 09 §11 AYA 判断仰ぎ候補 (Q1)-(Q5)**:
   - (Q1) 第 1 UBO migration template A/B/C
   - (Q2) 1 UBO 厳守 vs cluster 許可
   - (Q3) OpenGL path 並走期間 A/B/C
   - (Q4) 3 OS 確証順序 A/B/C
   - (Q5) Phase 0 番号化 A/B/C
3. **chapter 09 §12 持越**:
   - K の確定値 (= (Q1)(Q2) 確定後)
   - per-Phase 担当者役割分担
   - (Q1) Template 確定後の具体 UBO 順
4. **chapter 04-08 内 残課題** (= 各 chapter §N 未確定事項 listing で発見されたもの):
   - chapter 04 / 05 / 06a / 06b / 06c / 08 各 chapter の未確定事項を再 listing
   - 06a-prep §7 (P1)(P2)(P3)(P4) = 実装 phase 入口 grep で消化される項目、本 chapter 10 では「実装 phase 入口確定項目」として独立節
5. **inventory §7 残課題** との接続 = inventory live doc の残課題が設計 doc 群でカバー済か再確認、未カバー項目を chapter 10 で「設計 chapter 未カバー」と明示

- 起案完了見込み ~250-350 行 (= chapter 09 より軽量、各 chapter からの収集 + 整理が主作業)

### §2.3 chapter 10 で扱わない事項

- 実装 phase 入口で消化される項目 (= 06a-prep §7 等) は chapter 10 内に **独立節として listing** はするが、判断は持越 (= 実装 phase 入口で解消)
- 各 chapter §N の **既起案完了確定事項** は再収集しない (= chapter 10 は「未決」のみ集約)

### §2.4 chapter 10 完了 → 設計 chapter 群 完成 → implementation-phase 入口

- 設計 chapter 群 01-10 全件起案完了 = design-phase 完了マーク
- 次 phase = implementation-phase 入口 = **η-29 Phase 0 計測**
- η-29 入口で `06a-prep-phase0-measurement.md` を **手順書として実施開始** = (H1b) hook 実装 + 計測 build + log 取得 + 解析 + chapter 反映

---

## §3 必須 Read 2 件 (= **これ以上事前 Read しない**、不足は chapter 10 起案中に現場 pinpoint Read)

1. **本 handoff doc** (= chapter 10 scope / (Q1)-(Q5) / chapter 07 §12 chapter 10 送り 4 件 / 持越紐付けが全 pointer 化済)
2. **`design/09-phase-roadmap.md` §11 + §12 のみ** (= (Q1)-(Q5) 本体 + chapter 10 送り項目本体、chapter 10 主入力)

**意図的に除外**:
- `design/01-overview.md` = §4 進捗表 + 確定事項 13 件は本 handoff §1 内 pointer + handoff §1.3 持越紐付け表で代替済。chapter 10 起案で 01-overview 参照したくなった時のみ現場 pinpoint Read
- chapter 02-08 / 06a-prep / inventory = chapter 10 は各 chapter §N 残課題収集の性格上、**起案中の pinpoint Read 多発が想定**。各 chapter の「未確定事項」「open questions」「持越」節を順次 read、本文全部は読まない (memory `feedback_handoff_minimal_pre_req_read`)
- 前 handoff §3 で本 doc に含んでいた `design/06a-prep-phase0-measurement.md` は本 handoff §5 で必要箇所のみ pointer 化済、chapter 10 起案には不要

---

## §4 規律 (絶対遵守)

- **design-phase**: `indra/` 配下改変ゼロ (memory `feedback_design_phase_no_code_write`)、chapter 10 完了まで design-phase 継続
- **tests/**: commit / 確認 / 言及禁止 (memory `feedback_tests_dir_never_commit`)
- **commit**: AYA 明示指示まで実行しない (memory `feedback_no_auto_commit`)、`git add -A` / `git add .` 禁止 (明示パスのみ)
- **事前 Read**: §3 の 3 件のみ全読、その他は現場 pinpoint Read (memory `feedback_handoff_minimal_pre_req_read`)
- **完了時**: chapter 10 起案完了で AYA に「10 揃いました = 設計 chapter 群完成」報告 → 同様 commit 指示待ち、implementation-phase η-29 Phase 0 入口へ
- **context 残量**: 周回境界で自分から handoff doc 提案 (memory `feedback_proactive_handoff`)

---

## §5 implementation-phase 入口準備 memo (= chapter 10 完了後の次々 session 用)

η-29 Phase 0 入口で必要となる pre-flight check (= chapter 10 完了 → implementation-phase 着手 session で実施):

1. 06a-prep §7 (P1)(P2)(P3)(P4) 4 件を grep で解消:
   - (P1) `LL_INFOS("UBO_CADENCE")` class 名衝突有無
   - (P2) CMake patch 配置先 `00-Common.cmake` vs `LLRender.cmake` 等
   - (P3) frame counter 公開方式 (extern / helper / 既存流用)
   - (P4) 既存 frame counter 流用可能性 (`gFrameCount` 等)
2. AYAstorm build flow 確認 (= memory `project_build_procedure`):
   - configure → build → install → cache clear の完全フロー
   - `--fmodstudio` + `LL_DULLAHAN_AUDIO_CALLBACK` フラグ含む
3. log path 確認 (= memory `reference_log_path`):
   - Linux `~/.ayastorm_x64/logs/AYAstorm.log`
4. 計測 build flag `-DAYASTORM_UBO_CADENCE_HOOK=ON` 配置先確定 (= P2 grep 結果次第)

---

**= 本 handoff doc + §3 の Read 3 件で chapter 10 起案再現可能**。**chapter 10 完了で設計 chapter 群 (01-10) 全件起案済**、implementation-phase (= η-29 Phase 0 計測) 入口に到達。
