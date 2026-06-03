# handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-29 Stage 3 sub-task 5+6+7+8 完了 = Stage 3 14/14 ✅ 全完走 = 設計 phase 完了

**境界**: η-29 Stage 3 sub-task 5 → 8 連続消化 完了 + Stage 3 14/14 ✅ 全完走宣言 → η-30 Phase 1.A 実装 entry へ。

---

## §0 state 一行 summary

- η-29 Stage 3 = **14/14 ✅ 全完走** (= 3-1/3-2/3-3/3-4/3-5/3-6/3-7/3-8/3-9/3-10/3-11/3-12/3-13/3-14 全件 ✅)。
- **設計 phase 完了** = `feedback_design_phase_no_code_write` 完全解禁条件 (= 全 14 項目 ✅ + AYA 承認 (Q-NTTP)=A) 達成。
- 次 session 着手 = η-30 Phase 1.A 実装 entry handoff doc `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-entry.md` §3 PA-1 (= autobuild manifest pin = `glslang` / `spirv-cross` / `Python` 3 dependency entry 追加 + Linux/Win/Mac 3 platform 配信 URL pin)。
- `indra/` 改変ゼロ厳守継続中 (= 設計 phase scope 中の last `git status indra/` = modified 0 件)。Phase 1.A PA-1 entry で完全解禁、初実装 task で `autobuild.xml` 編集解禁。

---

## §1 pre-req 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

### §1.1 最小読み 3 件 (= 必読)

1. **本 handoff doc** = `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-29-stage3-sub-task5-8-complete.md` (= 本ファイル)
2. **η-30 Phase 1.A entry handoff doc** = `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-entry.md` (= 157 行、PA-1 〜 PA-8 構成表 + Exit Criteria + 紐付け持越項目)
3. **chapter 09 §4 + §14.4 末尾 + §14.5 + §14.8** = Phase 1.A scope + Exit Criteria + Stage 3 14/14 ✅ 全完走宣言 + 設計 phase 完了 verdict

### §1.2 pinpoint Read reference (= 必要時のみ局所参照)

- `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` §6.4.7 (= (Q-NTTP) A 確定 paragraph、C++17 維持根拠)
- `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` §11.6 (= (Q-NTTP) 確定形)、§14.4 末尾 (= ST-7 sub-task 1〜8 全 paragraph)
- `docs/specs/ayastorm-r41-gl-removal/design/10-open-questions.md` §1.0 (= 29 件 / 12 件判断済 / 17 件未判断 index)、§1.3 末尾 (= ST-7 sub-task 8 batch verdict)
- `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` §5.4.1 / §5.2.1 / §11.5 / §12.5 / §13.5 (= Phase 1.A PA-2/3/4/5/6/7/8 実装 source of truth)
- `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` §6.4 / §4.3.1 / §5.6 (= Deliverable A-1/A-2/A-3 = R3 path + std140 calculator + perfect hash CHD spec)
- `autobuild.xml` (= 4249 行、PA-1 で `glslang` / `spirv-cross` / `Python` 3 dependency entry 追加先)

---

## §2 本 session 成果 (= sub-task 5 → 8 連続消化 + Stage 3 14/14 ✅ 全完走)

### §2.1 commit 履歴 (= 本 session 4 commit)

| commit | sub-task | 内容 | file 差分 |
|---|---|---|---|
| `cf070c8fbc` | 5 | §14.5 3-1 + 3-2 ✅ retroactive verify (= Stage 0 全 12 + Stage 1 全 9 件 + 06a-prep §6 反映 flow 6 行) | 1 file +4/-2 |
| `90d054a158` | 6 | §14.5 3-11 ✅ (= autobuild manifest pin 仕様確定 + 実 pin 追加 Phase 1.A 内 task 化) | 1 file +3/-1 |
| `70b1828634` | (retro) | sub-task 1+2+3+4 handoff doc 起案 retroactive commit | 1 file new 216 |
| `c0e3457324` | 7 | §14.5 3-12 + 3-13 ✅ (= Phase 1.A handoff doc 物理起案 + `indra/` 解禁 timing 3 段階整理) | 2 file +161/-2 |
| `3424fb4c26` | 8 | §14.5 3-14 ✅ + Stage 3 14/14 ✅ 全完走 (= (Q-NTTP) AYA「A」応答 = A 確定 = R1 不採用 / C++17 維持) | 3 file +18/-10 |

### §2.2 sub-task 別 verdict 概要

- **sub-task 5** (= row 3-1 + 3-2 retroactive verify): Stage 0 §14.2 全 12 row verify pass (= 全 chapter section 存在 + (Q-NTTP) 実 entry 確認 + `git status indra/` modified 0 件) + Stage 1 §14.3 全 9 row verify pass (= Phase 0 全 5 step + hook 配線 `c27733ae79` + revert `4e40fd2ab0` + Pre-hook Static Analysis 全件解消 + per-program/per-draw 境界 verify + (RF) reflection fence 取得 + 除去) + 06a-prep §6 反映 flow 6 行全行「反映済」確認。
- **sub-task 6** (= row 3-11 = autobuild manifest pin): `autobuild.xml` 現状 grep verify (case-insensitive) = `glslang` / `spirv-cross` / `Python` entry **0 件 = 未追加状態** + chapter 08 §5.4.1.5 line 703 で pin 仕様 + 意図 + tag 値 (= 1.3.275) + timing 完備 + §11.5.1 cache key environment + §11.5.2 cache invalidation trigger 完備。**verdict**: 設計 phase scope では充足 ✅、実 pin 追加 (= 3 dependency entry + Linux/Win/Mac 3 platform 配信 URL pin) は **Phase 1.A 入口 PA-1 で実施**。
- **sub-task 7** (= row 3-12 解禁 timing 3 段階整理 + row 3-13 Phase 1.A handoff doc 起案): (a) Stage 1 Phase 0 Step 2 hook 配線で**一時解禁** / (b) Phase 0 Step 5 hook revert で**再封** / (c) Phase 1.A 入口到達 = 全 14 ✅ + AYA 承認で**完全解禁**。Phase 1.A handoff doc `handoff-substep-...-eta-30-phase1-a-entry.md` 157 行 起案完了 (= §0 state + §1 pre-req + §2 scope + Exit Criteria literal 継承 + §3 sub-task PA-1〜PA-8 + §4 紐付け持越項目 + §5 規律 + §6 self-verify + §7 memory + §8 着手 1 line)。
- **sub-task 8** (= row 3-14 (Q-NTTP) AYA 判断本体): AYA「A」応答 = **A 確定 (= R1 不採用 / R3 name-based dispatch のみ、C++17 維持)** = chapter 09 §11.6 default 採用継続 = R3 + perfect hash CHD + frozen-table で十分高速、C++20 切替 cost (= 3 OS toolchain 確認 + dependent module re-validation + autobuild manifest 変更) 回避、R1 は Phase K+4 以降 polish 候補保留可、**Phase 1.A handoff doc §3 PA-0 (= C++20 切替 task) 不要 = PA-1 から即着手可**。

### §2.3 Stage 3 14/14 ✅ 全完走 詳細

| # | 項目 | sub-task | verdict |
|---|---|---|---|
| 3-1 | Stage 0 全 12 ✅ | ST-7 sub-task 5 | retroactive verify pass |
| 3-2 | Stage 1 全 9 ✅ + 06a-prep §6 反映 flow | ST-7 sub-task 5 | retroactive verify pass |
| 3-3 | Stage 2 (Q1)(Q2) AYA 判断 ✅ | ST-5 batch | A/A 確定 |
| 3-4 | chapter 04 §6.4 / §4.3.1 / §5.6 反映済 | ST-7 sub-task 2 | 8+7+7 sub-subsection 起案済 verify |
| 3-5 | chapter 08 §5.4.1 / §5.2.1 / §11.5 / §12.5 / §13.5 反映済 | ST-7 sub-task 2 | 8+8 sub-subsection + 3 section 起案済 verify |
| 3-6 | chapter 09 §14 反映済 + 04 §6.4.7 NTTP 判定材料 | ST-7 sub-task 3 | 全 8 subsection + 7 line 起案済 |
| 3-7 | chapter 06a §3 mUniformUBOLoc cache + §5 16 method setter 設計起案済 | ST-7 sub-task 3 | 06a doc 495 行で要件物理充足 |
| 3-8 | chapter 06b cadence + 06c descriptor set bind 起案済 | ST-6 | 06b 441 行 + 06c 513 行 (B 案採用) |
| 3-9 | chapter 02 §2.4 naming + chapter 07 set 帯 5 化 / 256B 出力契約 | ST-7 sub-task 4 | 4 種生成識別子 + §11 padding 256B + §3.1/§7.3 alignment evidence |
| 3-10 | chapter 10 持越項目 14 件登録済 | ST-7 sub-task 4 | 13/14 + (NTTP) gap remediation で 14/14 ✅ |
| 3-11 | autobuild manifest pin 状態確認 | ST-7 sub-task 6 | 仕様完備 ✅ / 実 pin 追加は Phase 1.A 内 task |
| 3-12 | `indra/` 改変解禁 = `feedback_design_phase_no_code_write` 解除点 | ST-7 sub-task 7 | 解禁 timing 3 段階整理確定 + Phase 1.A 入口 PA-1 で完全解禁 |
| 3-13 | Phase 1.A handoff doc 起案 | ST-7 sub-task 7 | `handoff-...-eta-30-phase1-a-entry.md` 157 行 起案完了 |
| 3-14 | C++ standard 確認 (= (Q-NTTP) AYA 判断本体) | ST-7 sub-task 8 | A 確定 (R1 不採用 / C++17 維持) |

---

## §3 次 session 着手地点 (= η-30 Phase 1.A 実装 entry)

### §3.1 着手 1 line

η-30 Phase 1.A entry handoff doc `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-entry.md` §3 **PA-1 (= autobuild manifest pin)** から着手。**`indra/` 改変解禁状態** = `autobuild.xml` 編集が初実装 task。

### §3.2 PA-1 scope (= 引用)

- `autobuild.xml` (4249 行) に **3 dependency entry 追加**:
  - `glslang` version 1.3.275 (= chapter 08 §5.4.1.5 確定 tag 値)
  - `spirv-cross` version 2023-12-07 (= chapter 08 §11.5.1 cache key environment)
  - `Python` version 3.11.5 (= 同 cache key environment)
- 各 entry に **Linux / Win / Mac 3 platform 配信 URL pin** (= 既存 `SDL2` / `gstreamer10` 等 dependency と同形式)
- Exit Criteria: `autobuild install` 完走 + 3 dependency installed dir 配下に展開確認 + cache key environment 完備

### §3.3 PA-1 完了後の次 task

- PA-2 = Python script base (= `scripts/ubo_codegen/` 起案)
- PA-3 + PA-4 のみ並列可 (mini-parser + glslang -E / std140 calculator + SPIR-V reflection 二重保証)
- PA-5/6/7/8 = strict 線形順序 (perfect hash CHD frozen-table → 増分 build cache → CMake DEPENDS + 手動 target → 85 UBO blueprint 実行 + Exit 充足検証)

### §3.4 PA-1 判断不要 (= Claude 自走、AYA 確認不要)

PA-1 は仕様完備 (= chapter 08 §5.4.1.5 + §11.5.1 + §11.5.2) + 確定 version 値 (= 1.3.275 / 2023-12-07 / 3.11.5) + 既存 dependency entry 形式参照可で、AYA 判断 batch 不要。`autobuild install` 完走確認は AYA build session に投げる。

---

## §4 self-verify (= 12 観点 PASS)

1. **commit 履歴整合性**: 本 session 4 commit (cf070c8fbc / 90d054a158 / c0e3457324 / 3424fb4c26) + retroactive 1 commit (70b1828634) = §2.1 表と一致 ✅
2. **Stage 3 14/14 ✅ 表整合**: §2.3 表 14 row 全件 sub-task / verdict 記述ある ✅
3. **chapter 09 §14.5 14 row 全 ✅ mark 反映**: 14 row 全件 inline ✅ mark + sub-task ID 記載 ✅
4. **chapter 09 §14.4 sub-task 1〜8 paragraph chronological 順序**: sub-task 1 (line 684) → 2 (line 686) → 3 (line 688) → 4 (line 690) → 5 (line 692) → 6 (line 694) → 7 (line 696) → 8 (line 698) で順序正 ✅
5. **chapter 09 §14.8 Stage 3 entry verdict 達成 mark**: 「2026-06-03 ST-7 sub-task 8 batch で達成 = 14/14 ✅ 全完走 = 設計 phase 完了」反映済 ✅
6. **chapter 10 §1.0 count 整合**: index header 「内 12 件判断済」 + count 内訳「12 件判断済 = §1.6 4 + §1.3 6 + §1.5 2」「17 件未判断」整合 ✅
7. **chapter 10 §1.3 (Q-NTTP) row + 末尾 paragraph 反映**: row default → ✅ A 確定形 + §1.3 末尾 ST-7 sub-task 8 batch verdict paragraph 反映済 ✅
8. **chapter 04 §6.4.7 確定形 paragraph 反映**: 末尾「✅ 2026-06-03 ST-7 sub-task 8 batch 確定 = A」paragraph 新設済 ✅
9. **`indra/` ゼロ改変厳守**: `git status indra/` = modified 0 件 / 本 session 4 commit 全 doc 配下のみ ✅
10. **Co-Authored-By: Claude 行不在**: 本 session 4 commit message 全件で共著行不在 ✅
11. **handoff doc 命名整合**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-29-stage3-sub-task5-8-complete.md` = sub-task 1-4 retroactive handoff (`...stage3-sub-task1-4-complete.md`) と対称形 ✅
12. **次 session 着手地点明示**: §3.1 で η-30 Phase 1.A handoff doc PA-1 から着手と 1 line 化済 ✅

---

## §5 引き継ぎ済 memory (= 13 件、Phase 1.A 実装 entry を見据えた追加)

設計 phase scope 中 (η-29) と実装 phase entry (η-30) の両 phase で適用される memory を列挙。

1. **`feedback_handoff_minimal_pre_req_read`** = handoff の pre-req 全件読み禁止、最低限 3 件 + pinpoint Read
2. **`feedback_design_phase_no_code_write`** = 設計 phase 中 `indra/` 改変禁止。Phase 1.A 入口 PA-1 で完全解禁 → 解除後の実装 task 範囲は実装 phase の正常範囲
3. **`feedback_no_scope_shrink`** = AYA「すべて」「全部」literal 解釈、scope 縮小許可と誤読しない
4. **`feedback_self_verify_before_handoff`** = AYA に投げる前に Claude 自走で全 parameter 整合確認
5. **`feedback_no_claude_coauthor`** = 全 commit message で Claude 共著行不在
6. **`feedback_one_step_at_a_time`** = 1 メッセージ 1 アクション、複数質問・並列確認指示しない
7. **`feedback_doubt_self_first`** = AYA 情報を疑わず自分の P0 調査・hypothesis を先に検証 (本 session sub-task 4 で (NTTP) gap 検出 → remediation 実施で発動済)
8. **`feedback_proactive_handoff`** = context 圧迫時は能動的 handoff 提案、AYA 指示待たず
9. **`feedback_no_auto_commit`** = 修正依頼は編集まで、commit は AYA 明示指示後
10. **`project_ayastorm_r41_vulkan_migration`** = active milestone、source of truth は handoff doc chain
11. **`project_ayastorm_r41_design_principles`** = (1) upstream OpenGL 取り込みやすさ維持 + (2) Core プロセス分散実現
12. **`feedback_ubo_migration_one_at_a_time`** = UBO 化作業は 1 つずつ、大塊 batch 禁止 (= (Q2) A 確定根拠)
13. **`feedback_build_only_verified`** = Phase 1.A 実装 entry 移行で重要度 ↑ = 効果未確認 commit を積まない、確認できないものは revert (= (Q3) A 確定根拠 = REJECT 時 baseline 確保)

---

## §6 次 session 着手 1 line

> η-30 Phase 1.A entry handoff doc `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-entry.md` §3 PA-1 (= autobuild manifest `autobuild.xml` に `glslang` / `spirv-cross` / `Python` 3 dependency entry 追加 + Linux/Win/Mac 3 platform 配信 URL pin、`indra/` 解禁後の初実装 task) から着手。Pre-req 最小読み 3 件 = 本 handoff + Phase 1.A entry handoff + chapter 09 §4。
