# handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2d-β-revise 本体 complete

**起案日**: 2026-06-03
**起案 commit**: (未 commit、AYA 明示指示待ち = `feedback_no_auto_commit`)
**前置 commit**: `3174ac4521` (= 前提充足完了) / `d212ba44e8` (= literal scope 確定 + §6.2 新範式) / `e886fa92c7` (= Phase 2d-α Fix)
**直前 handoff**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-beta-revise-entry.md` (= 着手 entry handoff)

---

## §0 本 handoff の目的 (= 次 session 着手地点)

**Phase 2d-β-revise 本体完了** = 設計 chapter 04 / 08 / 09 への 9 sub-task 全件深化 + 整合性 self-verify PASS。`indra/` 改変ゼロ厳守 (= `feedback_design_phase_no_code_write` 継続) は完遂、bridge phase 範式 (= design-phase ↔ implementation-phase 境界の docs 化 phase) literal scope 完走。

次 session = **2 つの並列分岐のいずれか、AYA 判断に委ねる**:

- **分岐 X**: 本 phase 成果物 commit (AYA 明示指示後、`feedback_no_auto_commit`) → η-29 Phase 0 (= 実機計測 phase、`indra/` 改変 初解禁) 入口着手
- **分岐 Y**: self-verify 副産物 (= MUST FIX ではない改善候補 3 件) を取り込んでから commit

---

## §1 pre-requisite 必読 file (= 最低限 3 件、`feedback_handoff_minimal_pre_req_read` 準拠)

### §1.1 必読 1: 本 phase entry handoff (= scope source of truth)

- `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-beta-revise-entry.md`
- = literal scope 9 sub-task (Deliverable A 3件 + B 5件 + C 1件) を定義した entry doc
- 本 handoff の補完元

### §1.2 必読 2: 本 phase で深化した設計 chapter 群 (= 成果物 source of truth)

- `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` (= 新規 §4.3.1 / §5.6 / §6.4 追加、計 1004 行)
- `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` (= 新規 §5.2.1 / §5.4.1 / §11.5 / §12.5 / §13.5 追加、計 1970 行)
- `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` (= 新規 §14 追加、計 722 行)
- (全件 Read 禁止、必要箇所 pinpoint Read 推奨)

### §1.3 必読 3: η-29 Phase 0 計測 spec (= 次 phase 入口の前提)

- `docs/specs/ayastorm-r41-gl-removal/design/06a-prep-phase0-measurement.md`
- = 09 §14.3 Stage 1 で参照される計測 spec、η-29 着手時の主要 reference

### §1.4 pinpoint Read (= 必要時のみ)

- 09 §14 (= Phase 1.A 入口 1 step state checklist 全文)
- 10 §1.6 (= 判断済 archive 4 件 Q22-NUM/Q23-K/Q24-S1/Q25-21CNT 確認時)
- 10 §1.1-§1.5 (= 未判断 21 件、η-29 Phase 0 入口で K placeholder 関連 (Q1)(Q2) 確認時)
- 08 §17 (= 持越 (A1)(P)(G/B3)(B1)(B2)(B4)(B5)(P-future)(cache-grow) と本 phase 深化結果の cross-check 時)

---

## §2 本 phase 完了成果物 (= 9 sub-task literal 反映済)

### §2.1 Deliverable A: chapter 04 generator algorithm 深化 (3 sub-task)

| sub-task | 反映先 | 内容 summary | LoC 見積 |
|----------|--------|--------------|----------|
| A-1 | 04 §6.4 | name-based dispatch algorithm 詳細化 (R1 compile-time / R2 runtime dynamic / R3 mUniform[index] pre-cache 3 path 比較表、`UniformLocation::sentinel()` unresolved fallback、C++20 NTTP `template<auto NameLiteral>` 案 = chapter 09 (Q-NTTP) 持越) | (algorithm spec) |
| A-2 | 04 §4.3.1 | std140 calculator 詳細化 (base alignment 決定表 + `compute_layout()` state machine + `compute_struct_type_info()` 再帰 + `arrayify()` corner case + 末尾 padding rule + `pad_to_device_align()` 2-stage + unsupported type error + `verify_against_spirv()` interface) | ~345 |
| A-3 | 04 §5.6 | perfect hash CHD algorithm 詳細化 (2-stage hash + displacement table 構築 + FNV-1a 32-bit + bucket sort + seed reroll + N=880 → ~5ms 性能特性 + `g_chd_displacement[]` / `g_chd_values[]` / `g_chd_key_strings[]` false positive 排除) | ~310 |

### §2.2 Deliverable B: chapter 08 codegen pipeline prototype 深化 (5 sub-task)

| sub-task | 反映先 | 内容 summary | LoC 見積 |
|----------|--------|--------------|----------|
| B-1 | 08 §5.4.1 | SPIR-V reflection 二重保証 (`extract_reflection()` via spirv-cross JSON + `verify_layout_against_spirv()` per-member offset/block_size/array_stride check + format drift resilience + autobuild version pin (glslang 1.3.275) + `AYA_CODEGEN_SKIP_SPIRV_CHECK=1` escape hatch with [WIP] commit prefix rule + set=1a/1b subset verification) | ~275 |
| B-2 | 08 §5.2.1 | GLSL mini-parser 詳細化 (EBNF grammar (ubo_block / struct_def / bare_uniform / sampler_decl) + TOKEN_PATTERNS regex set + `parse_glsl()` top-level state machine + nested struct 2-pass + sampler 4th output + LL GLSL 慣用範囲外 detection 表 + #line directive tracking) | ~275 |
| B-3 | 08 §11.5 | 増分 build cache 詳細化 (`codegen_state.json` schema (codegen_tool_version/environment/input_files/output_files) + `check_cache()` mtime粗判定 → sha256精判定 + 12 invalidation trigger 表 + `sha256_file_normalized()` CRLF→LF + atomic write via temp+rename + GC policy) | ~280 |
| B-4 | 08 §12.5 | CMake DEPENDS 詳細化 (regeneration timing 全枚挙表 + `CONFIGURE_DEPENDS` 採用 + add_dependencies chain (codegen_ubo → llrender/llvkloader) + `codegen_ubo_force` target + `write_outputs_atomic()` partial write 防止 + verbose log 統合 + touch on cache hit) | ~165 |
| B-5 | 08 §13.5 | 3 OS binary identical 保証 (ND1-ND12 非決定性 12 因子 (dict iteration / file enumeration / line ending / path separator / float repr / hash seed / timestamp/path leak / PYTHONHASHSEED / locale / OS newlines) + 明示 sort discipline + `write_inl()` `newline=''` + CMake env var `PYTHONHASHSEED=0` + `sha256sum` cross-OS diff 検証 + `test_deterministic.sh`) | ~55 |

**chapter 08 計 LoC 見積**: 275 + 275 + 280 + 165 + 55 ≈ **1050 行** (08 §3.2 上限 ~500-1000 行 内に収斂、Deliverable B 5 sub-task 整合)

### §2.3 Deliverable C: Phase 1.A 入口 1 step state checklist

| sub-task | 反映先 | 内容 summary |
|----------|--------|--------------|
| C | 09 §14 (新規) | 4-stage readiness model (Stage 0 design-phase完了 / Stage 1 Phase 0 計測 / Stage 2 AYA判断 (Q1)(Q2)(Q4) / Stage 3 Phase 1.A 着手 ready) + §14.2-§14.5 self-check 表 計 40 項目 (12+9+5+14) + §14.6 achievement order diagram + §14.7 cross-chapter integrity pointer + §14.8 self-evaluation (= 本 phase 完了マーク) + 末尾 chapter summary line 更新 |

---

## §3 整合性 self-verify 結果 (= Phase 2d-β-revise 完了条件 §6.1 達成)

### §3.1 判定 summary

**PASS** (= 全 5 観点で整合性確認完了、設計 phase 完了状態到達)

### §3.2 観点別

| # | 観点 | 判定 |
|---|------|------|
| 1 | chapter 04 ↔ chapter 08 cross-ref (`compute_layout()` / `verify_against_spirv()` / `verify_layout_against_spirv()` / UniformLocation / CHD 出力形式) | PASS |
| 2 | chapter 08 内部 pipeline 線形性 (§2 target → §3 tool → §5 parse → §11 cache → §12 trigger → §13 determinism) | PASS |
| 3 | chapter 09 §14 ↔ chapter 04/08 cross-ref (§14.5 Stage 3 checklist 14 項目 全実在) | PASS |
| 4 | chapter 10 整合性 (Q22-Q25 §1.6 archive 配置 + §1.1-§1.5 21 件未判断 数学整合: 4+7+5+4+1=21 ✓) | PASS |
| 5 | 既存 §X.Y 参照 breakage check (新規 section 追加で既存 section 番号 ずれゼロ、cross-ref 影響ゼロ) | なし (= PASS) |

### §3.3 副産物 / 改善候補 (= MUST FIX ではない、次 session optional)

1. **08 §12.3 CMakeLists.txt snippet 明確化**: `ubo_layout_<blockname>.inl` の OUTPUTS 列挙が明示的でない。`file(GLOB ... ubo_layout_*.inl)` 自動 tracking or 明示追加のコメント説明追加候補。
2. **04 / 08 / 09 間の逆参照ポインター追加**: 例: 04 §5.6 (CHD) から「08 §5.6 で G2/B3b として確定」への back-ref。ナビゲーション性向上。
3. **chapter 10 判断結果テーブル検索性**: §1.6 判断済 archive と §1.1-§1.5 未判断 が物理分離。実装 phase 参照時の「どの table を見るか」前置説明追加候補。

(= 上記 3 件は本 phase scope 外、次 session で AYA 判断 = 取込 or skip)

---

## §4 着手前提条件 (= 次 session 開始前 確認事項)

### §4.1 commit 前提 (= 分岐 X 採用時)

- AYA 明示指示で commit OK (= `feedback_no_auto_commit` 解除)
- commit message format: 「docs(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2d-β-revise 本体完了 = 設計 chapter 04 + 08 + 09 への 9 sub-task literal 全件反映 (Deliverable A 3 件 / B 5 件 / C 1 件) + 整合性 self-verify PASS (= 全 5 観点 / 不整合ゼロ / 既存参照 breakage なし)」(概要のみ、詳細は本 handoff 参照)
- push は AYA 側 (= `feedback_release_flow`)

### §4.2 η-29 Phase 0 着手前提 (= 分岐 X 後の次 phase)

- 09 §14.3 Stage 1 self-check 9 項目を η-29 Phase 0 着手時に確認
- 06a-prep §2-§6 (Phase 0 計測 spec) が source of truth
- **重要**: η-29 Phase 0 = `indra/` 改変 初解禁 phase。Phase 2d-β-revise 内では到達しない (= 本 phase は docs only)
- η-29 Phase 0 完了 → (Q1)(Q2)(Q4) AYA 判断 → K placeholder 置換 → Phase 1.A (= 第 1 UBO migration) 入口

### §4.3 self-verify 副産物 3 件 取込判断 (= 分岐 Y 採用時)

- AYA 判断: 副産物 3 件 (§3.3) を本 phase commit 前に取り込む？ or skip して次 phase 持越？
- 推奨: skip (= 本 phase literal scope 9 sub-task は完了済、副産物は scope 外)

---

## §5 範式継承 (= 本 phase で再確認、次 phase 継続)

### §5.1 継承範式

- `feedback_design_phase_no_code_write` (= design-phase 中 `indra/` 改変禁止) **本 phase 完遂、Phase 0 入口で初解禁**
- `feedback_handoff_minimal_pre_req_read` (= pre-req 最低限 3 件 + pinpoint Read) **継続**
- `feedback_no_scope_shrink` (= literal scope 縮小禁止) **本 phase 9 sub-task literal 完走、違反なし**
- `feedback_proactive_handoff` (= context 圧迫時自発 handoff) **本 doc 起案で適用**
- `feedback_no_auto_commit` (= commit は AYA 明示指示後) **継続 = 本 phase 成果物 未 commit 状態**
- `feedback_release_flow` (= push は AYA 側) **継続**

### §5.2 本 phase で確立した範式 (= 引継ぎ)

- **bridge phase 範式** (= 95e690b426 で確立): design-phase ↔ implementation-phase 境界の docs 化 phase = 本 phase で完遂
- **literal scope 厳守**: 9 sub-task literal 全件反映、scope 縮小禁止 = 完遂
- **設計に合わせて作る前半なくしてエラー取り後半をしない** (= d212ba44e8 §6.2): η-29 Phase 0 以降で適用継続

---

## §6 関連 commit / handoff cross-ref

### §6.1 関連 commit

- `3174ac4521` (2026-06-03): 前提充足完了 (= AYA 判断 4 件反映 + 修正推奨 20 件反映 + 残課題 11 件反映)
- `d212ba44e8` (2026-06-03): Phase 2d-β-revise literal scope 確定 + §6.2 新範式 (= 設計に合わせて作る前半なくしてエラー取り後半をしない)
- `95e690b426` (2026-06-03): 旧 prep (REJECT 履歴、archive only)
- `e886fa92c7` (2026-06-03): Phase 2d-α Fix (= 直前 phase 完了)

### §6.2 関連 handoff

- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-beta-revise-entry.md` (= 本 phase 着手 entry、本 doc の対)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-beta-prep.md` (= 旧 prep の包括 spec、scope 起源)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-alpha-complete.md` (= 直前 phase 完了 handoff)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-beta-pivot-to-codegen-deepening.md` (= pivot 経緯)

### §6.3 関連 memory

- `feedback_design_phase_no_code_write` / `feedback_handoff_minimal_pre_req_read` / `feedback_no_scope_shrink` / `feedback_proactive_handoff` / `feedback_no_auto_commit` / `feedback_release_flow` / `project_ayastorm_r41_vulkan_migration` / `project_ayastorm_r41_design_principles`

---

## §7 本 handoff の制約 (= 次 session 開始時に Claude が忘れがちな点)

1. **本 phase 成果物 未 commit** (= 3 file modified、編集後 commit 待ち、`feedback_no_auto_commit` 厳守)
2. **次 phase = η-29 Phase 0** (= 実機計測 phase、`indra/` 改変 初解禁、LL_INFOS hook 追加 + 3 OS baseline 計測)
3. **literal scope 既完走** (= 9 sub-task 全件反映済、再 verify / 再反映 不要)
4. **副産物 3 件** (= MUST FIX ではない、§3.3、AYA 判断で取込 or skip)
5. **handoff doc 起案 / commit / push 系の git 作業は AYA 明示指示時のみ** (= `feedback_no_auto_commit` / `feedback_release_flow`)
6. **pre-requisite は §1.1-§1.3 最低限 3 件 + §1.4 pinpoint Read** (= 全件 Read 禁止、context 圧迫回避)
7. **AYA 判断仰ぎ事項が新規発生したら chapter 10 §1.0 25 件 index 表 + §1.6 ↔ §1.1-§1.5 構造で登録**
