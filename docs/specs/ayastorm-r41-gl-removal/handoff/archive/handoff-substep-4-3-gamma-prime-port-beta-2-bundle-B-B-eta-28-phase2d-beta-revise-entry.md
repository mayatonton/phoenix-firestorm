# handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2d-β-revise 本体着手 entry

**起案日**: 2026-06-03
**起案 commit**: `3174ac4521` (= 前提充足完了 commit)
**前置 commit**: `d212ba44e8` (= Phase 2d-β-revise literal scope 確定 + §6.2 新範式) / `95e690b426` (= 旧 prep REJECT 履歴)

---

## §0 本 handoff の目的 (= 次 session 着手地点)

**Phase 2d-β-revise 本体着手 ready state 到達** = AYA 判断 4 件 (Q22-NUM A' / Q23-K A / Q24-S1 A / Q25-21CNT B) 反映完了 + 設計 chapter 群 修正推奨 20 件 反映完了 (Wave A-G batch) + 残課題 G1→MC1 (= 旧 G1) 機械的 rename 11 件 反映完了 (commit `3174ac4521`)。

次 session = **Phase 2d-β-revise 本体 = chapter 04 (Codegen-UBO) + chapter 08 (codegen pipeline prototype) の設計 chapter 深化 docs 化** に着手。`indra/` 改変ゼロ厳守 (= `feedback_design_phase_no_code_write` 継続)、コーディング (= GLSL UBO 化 + host C++ redirect) は η-29 Phase 0 実機計測 phase 入口まで解禁されない。

---

## §1 pre-requisite 必読 file (= 最低限 3 件、`feedback_handoff_minimal_pre_req_read` 準拠)

### §1.1 必読 1: Phase 2d-β-revise literal scope 起案 commit

- `git show d212ba44e8 -- 'docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-beta-prep.md'`
- = 旧 prep REJECT 履歴 + 新 Phase 2d-β-revise literal scope (Deliverable A/B/C) + §6.2 新範式 (= 設計に合わせて作る前半なくしてエラー取り後半をしない) を定義した commit
- 本 session で初手 Read 推奨

### §1.2 必読 2: chapter 04 + chapter 08 現状 (= 深化対象 source of truth)

- `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md`
- `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md`

### §1.3 必読 3: 直前 prep handoff (= scope context 継承)

- `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-beta-prep.md` (= 本 handoff の前置 prep doc、Deliverable A/B/C の包括 spec)

### §1.4 pinpoint Read (= 深化作業中に必要時のみ)

- chapter 10 §1.6 (= AYA 判断 4 件 反映先 cross-ref) / chapter 09 §5.2 (= K placeholder 注記) / chapter 06a §4.3.1 (= Q24-S1 (S1-代替) 採用確定) / inventory §3.3.1 (= 85 UBO blueprint)
- (= 必読 3 件で context 不足時のみ pinpoint Read、全件 Read 禁止)

---

## §2 Phase 2d-β-revise 本体 literal scope (= 3 deliverable、d212ba44e8 既定義)

### §2.1 Deliverable A: chapter 04 generator algorithm 深化 (3 sub-task)

1. **A-1**: name-based dispatch algorithm 詳細化 (= shader 内 `setUniform("name", ...)` から `<BlockName>Layout` struct field への compile-time 解決経路)
2. **A-2**: std140 calculator 詳細化 (= UBO member alignment / offset / size の精密計算 logic、SPIR-V reflection 二重保証の Codegen 側)
3. **A-3**: perfect hash CHD (Compress-Hash-Displace) algorithm 詳細化 (= name → index 表 lookup の衝突 0 + コンパクト table 設計)

### §2.2 Deliverable B: chapter 08 codegen pipeline prototype 深化 (5 sub-task)

1. **B-1**: SPIR-V reflection 二重保証 (= glslang 経由 reflection 結果と Codegen std140 calculator の照合 mechanism)
2. **B-2**: GLSL parse mini-parser (= UBO block / member 抽出、glslang -E preprocessor 展開と併用、P3 案具体化)
3. **B-3**: 増分 build cache (= `codegen_state.json` mtime + hash 併用、B4a 採用案具体化)
4. **B-4**: CMake DEPENDS 自動 trigger + 手動 target 併設 (= B5a 採用案具体化)
5. **B-5**: 3 OS binary identical 保証 (= Codegen 出力の `ubo_metadata.inl` / perfect hash table が 3 OS で byte-for-byte 同一を保証する mechanism)

### §2.3 Deliverable C: Phase 1.A 入口 1 step state checklist

- Phase 2d-β-revise 完了 → η-29 Phase 0 (= 実機計測) → Phase 1.A (= 第 1 UBO migration、Q1 確定後) 入口に至るまでの 1 step state checklist
- 「次に何があれば Phase 1.A に入れるか」を 1 ページで明示
- = §2.1 / §2.2 の深化結果が **Phase 1.A 入口で必要な doc readiness** に到達したか self-check リスト

---

## §3 着手前提条件 充足確認 (= 本 handoff 起案時点で全件 ✅)

### §3.1 AYA 判断 4 件完了

- Q22-NUM (A') / Q23-K (A) / Q24-S1 (A) / Q25-21CNT (B) = 全件反映済 (= chapter 10 §1.6 ✅ 判断済 batch)

### §3.2 設計 chapter 群 修正推奨 20 件 反映完了

- 第二次査読 §8.1 18 件 + audit §3.1 追加 2 件 = 計 20 件 Wave A-G batch 反映完了 (commit `3174ac4521`)
- 残課題 G1→MC1 (= 旧 G1) 機械的 rename 11 件 (03 / 05 §6.4 / 06b / 06a) 反映完了 (commit `3174ac4521`)

### §3.3 design-phase 中 `indra/` 改変ゼロ厳守

- `feedback_design_phase_no_code_write` 継続適用、Phase 2d-β-revise 本体は docs 化作業のみ

---

## §4 範式継承 + 本 phase 適用範式

### §4.1 継承

- d212ba44e8 §6.2 新範式 = **設計に合わせて作る前半なくしてエラー取り後半をしない** (= η-28 sub-bundle Phase 2a-2c / 2d-α / 2d-β prep の trap 認識)
- = 設計 chapter 群が implementation 可能 readiness に到達するまで、設計起案の手を抜かない
- `feedback_design_phase_no_code_write` (= design-phase 中 `indra/` 改変禁止、計測も doc 化)
- `feedback_handoff_minimal_pre_req_read` (= pre-req 全件読み禁止、最低限 3 件 + pinpoint Read)
- `feedback_no_scope_shrink` (= AYA 指示 literal scope を勝手に縮小しない)

### §4.2 本 phase 適用範式

- **bridge phase 範式** (= 95e690b426 で確立): design-phase ↔ implementation-phase 境界の docs 化 phase
- **literal scope 厳守**: Deliverable A 3 sub-task + Deliverable B 5 sub-task + Deliverable C 1 sub-task = 計 9 sub-task literal 全件反映、scope 縮小禁止

---

## §5 関連 commit / handoff cross-ref

### §5.1 関連 commit (= 本 phase の前提)

- `3174ac4521` (2026-06-03): 本 handoff 起案 = 前提充足完了 commit
- `d212ba44e8` (2026-06-03): Phase 2d-β-revise literal scope 確定 + §6.2 新範式
- `95e690b426` (2026-06-03): 旧 prep (REJECT 履歴、archive only)
- `e886fa92c7` (2026-06-03): Phase 2d-α Fix (= 直前 phase 完了)

### §5.2 関連 handoff (= cross-ref)

- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-beta-prep.md` (= 直前 prep、本 handoff の包括 spec)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-alpha-complete.md` (= 直前 phase 完了 handoff)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-beta-pivot-to-codegen-deepening.md` (= pivot 経緯)

### §5.3 関連 memory

- `feedback_design_phase_no_code_write` / `feedback_handoff_minimal_pre_req_read` / `feedback_no_scope_shrink` / `feedback_proactive_handoff` / `project_ayastorm_r41_vulkan_migration` / `project_ayastorm_r41_design_principles`

---

## §6 完了条件 + 次 step

### §6.1 Phase 2d-β-revise 完了条件

- Deliverable A 3 sub-task (A-1 / A-2 / A-3) = chapter 04 反映完了
- Deliverable B 5 sub-task (B-1 / B-2 / B-3 / B-4 / B-5) = chapter 08 反映完了
- Deliverable C 1 sub-task = Phase 1.A 入口 1 step state checklist 完成 (= 新規 doc or chapter 09 §11 拡張)
- 設計 chapter 群 self-verify (= 04 + 08 + 09 + 10 整合性確認) PASS

### §6.2 完了後の次 step

- **η-29 Phase 0** = 実機計測 phase (= `indra/` 改変 初解禁、LL_INFOS hook 追加 + 3 OS baseline 計測)
- Phase 0 完了 → AYA 判断 (Q1)(Q2) 確定 → K placeholder 置換 → Phase 1.A (= 第 1 UBO migration) 入口

---

## §7 本 handoff の制約 (= 次 session 開始時に Claude が忘れがちな点)

1. `indra/` 改変ゼロ厳守 (= Deliverable A/B/C は全て docs 化、計測 spec も doc 化)
2. pre-requisite は §1.1-§1.3 最低限 3 件 + §1.4 pinpoint Read、全件 Read 禁止 (= context 圧迫回避)
3. literal scope 縮小禁止 (= 9 sub-task 全件反映、「canary だから部分でいい」等の後付け合理化禁止)
4. AYA 判断仰ぎ事項が新規発生したら chapter 10 §1.0 25 件 index 表 + §1.6 (= 判断済 archive) ↔ §1.1-§1.5 (= 未判断 active) の構造で登録
5. AYA 判断 4 件 + 修正推奨 20 件 + 残課題 11 件 = 全件反映済 (commit `3174ac4521`)、再 verify / 再反映 不要
6. handoff doc 起案 / commit / push 系の git 作業は AYA 明示指示時のみ (= `feedback_no_auto_commit` / `feedback_release_flow`)
7. 本 phase 完了後の次 step = η-29 Phase 0 (= `indra/` 改変 初解禁 phase)、Phase 2d-β-revise 内では到達しない
