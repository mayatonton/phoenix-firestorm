# handoff = r41 Phase 2.α α-3 = CMake input 切替 + blueprint README + 波及 doc 一括更新 entry

**起案日**: 2026-06-06
**位置付け**: Phase 2.α α-3 sub-step 着手 entry handoff。Phase 2.α entry handoff §D.6/§D.7/§D.8 を **主要 source of truth** として pinpoint reference する **pointer doc** (= memory `feedback_no_dual_doc_split` 整合、改修方針詳細は entry handoff §D に集約済)。
**起案契機**: Phase 2.α α-2 完了 commit `b66ec99f72` + AYA literal「別セッションで α-3 進めて」(= 2026-06-06) 受領、memory `feedback_proactive_handoff` 適用で次 session 引き継ぎ。
**起案規律**:
- memory `feedback_proactive_handoff` 適用 (= 周回境界 = α-2 完了)
- memory `feedback_handoff_minimal_pre_req_read` 適用 (= 最低限 3 件 + pinpoint)
- memory `feedback_no_dual_doc_split` 整合 (= 本 doc は pointer 役、source of truth は entry handoff §D)
- memory `feedback_root_cause_no_shortcuts` §9 適用 (= AYA literal「別セッションで α-3 進めて」既発令で着手承認済、preflight 質問なし)
- memory `feedback_doubt_self_first` §5 適用 (= 解決策確定前に設計 doc 整合確認 default)
- memory `feedback_build_only_verified` 適用

---

## §1. α-3 着手目的 + scope

**Phase 2.α α-3** = 案 Z (= 設計 doc 整合修復) を **source code 実装 + 設計 doc 整合化 + per-UBO doc 整合化** で全件実施する sub-step。

scope (= Phase 2.α entry handoff §D.6/§D.7 詳細 cross-ref、全 10 範囲):
- **改修 1** (= §D.6 #1): `indra/cmake/AyaUboCodegen.cmake` = `AYA_UBO_CODEGEN_BLUEPRINT_DIR` 廃止 → `AYA_UBO_CODEGEN_SHADER_SOURCE_DIRS` (= `class1/class2/class3/cinematic_bd` 4 path list) 導入、`--input` 引数群更新、`file(GLOB_RECURSE CONFIGURE_DEPENDS)` pattern 4 path 連合
- **改修 3** (= §D.6 #3): `indra/newview/app_settings/shaders/aya_r41_blueprints/README.md` 新規追加 = 「設計時参考資料 / Phase 1 履歴 / source of truth ではない / 編集禁止 / codegen 入力対象外」明示 (= AYA 指示 #5 + design/01:146 + design/04:967 cross-ref)
- **波及 doc 更新** (= §D.7 範囲、α-3 内一括):
  - 設計 doc 4 件 = `design/04-codegen-ubo.md` + `08-build-codegen-pipeline.md` + `09-phase-roadmap.md` + `10-open-questions.md`
  - per-UBO doc 80+ 件 = `design/ubo/*.md` 内 blueprint 言及
  - source code 5 file = `scripts/ubo_codegen/*.py` docstring 内 blueprint 言及
  - inventory `ayastorm-r41-ubo-current-state-inventory.md:247-249` record 更新
  - 必要時 `design/ubo/WORK_ORDER.md` + `READINESS.md` + `RELATIONS.md` + `INDEX.md` 更新

---

## §2. 必読 doc (= memory `feedback_handoff_minimal_pre_req_read` 適用)

### §2.1 最低限 3 件

1. **本 handoff doc** = α-3 着手 entry pointer
2. **`handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D** = 案 Z 確定 source of truth (= §D.2 設計 doc 精査結果 + §D.6 5 改修詳細 + §D.7 全件波及更新範囲 + §D.8 sub-session 5 整合性、commit `64122994c1`)
3. **`indra/cmake/AyaUboCodegen.cmake`** = 改修 1 対象 source (= 改修前 state 確認、blueprint dir 固定の現状確認)

### §2.2 必要時 pinpoint Read 候補

- `scripts/ubo_codegen/main.py` (= α-2 commit `b66ec99f72` で改修済、`_verify_block_match` + multi-input 対応反映確認、`--input nargs='+'` 仕様確認)
- `scripts/ubo_codegen/tests/test_main.py` (= MultiFileIntegrityTests 7 test 確認)
- `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` (= blueprint 言及箇所、§27 / §932 / §967 等)
- `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` (= §72-74 codegen 入力 想定 literal + §96 DEPENDS literal、巨大 doc ゆえ pinpoint)
- `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` + `10-open-questions.md` (= blueprint 関連項目)
- `docs/specs/ayastorm-r41-gl-removal/design/ubo/*.md` 80+ 件 (= per-UBO doc、blueprint 言及機械的書換)

### §2.3 memory pinpoint

- `feedback_root_cause_no_shortcuts` §10 (= 設計 doc 整合確認なし根治確定の罪、案 Y 撤回根拠)
- `feedback_doubt_self_first` §5 (= 解決策確定前に設計 doc 精査 default)
- `feedback_design_doc_number_literal_verify` (= 構造属性 + codegen/build pipeline 入力 source path verify)
- `feedback_proactive_risk_management` (= AYA リスク管理肩代わり防止)
- `feedback_build_only_verified` (= cold launch 検証で効果 verify)
- `feedback_no_auto_commit` (= commit は AYA literal 指示後のみ)
- `feedback_release_flow` (= push / PR は AYA 側)
- `feedback_no_claude_coauthor` (= Co-Authored-By Claude 行禁止)
- `project_r41_phase2_4_principles` (= 4 原則 + Phase 2.L0 Exit)
- `project_r41_design_principles` (= 2 大設計原則)
- `feedback_ubo_migration_one_at_a_time` (= 1 sub-step 1 commit、各 sub-step 完了で検証挟む)
- `feedback_handoff_minimal_pre_req_read` (= 最低限 3 件 + pinpoint)

---

## §3. α-3 着手順序 + 想定 commit (= memory `feedback_ubo_migration_one_at_a_time` 整合)

| # | 改修内容 | 改変対象 | 想定 commit | 検証 |
|---|---|---|---|---|
| 1 | `aya_r41_blueprints/README.md` 新規追加 (= reference 降格明示) | shader dir + README 1 file 追加 | 1 commit | doc 確認のみ |
| 2 | `indra/cmake/AyaUboCodegen.cmake` 改修 (= input dir 切替) | cmake 1 file | 1 commit | CMake configure 走行 verify (= `cmake -B build-linux-x86_64` で error なし confirm) |
| 3 | codegen 単独走行 verify (= 改修後 codegen で `class*/` + `cinematic_bd/` 入力で `ubo_metadata.inl` 再生成、80 UBO 全件 binding 値が alphabetical sort literal (= step 2-pre2 mapping §5.1) と整合 confirm) | (= verify のみ、build 不要) | 0 commit (= verify 結果は次 commit に同梱 or 別 record) | codegen tool 単独実行 |
| 4 | 設計 doc 4 件 (= 04/08/09/10) blueprint 言及更新 | docs/specs/ 4 file | 1 commit (細分可) | doc 確認 |
| 5 | per-UBO doc 80+ 件 blueprint 言及機械的書換 | design/ubo/*.md 80+ file | 1-3 commit (= sub-batch 分割可、grep + sed 一括 OK) | doc 確認 + git diff review |
| 6 | source code 5 file (= main.py + glsl_parser.py + std140.py + perfect_hash.py + spirv_reflect.py) docstring 内 blueprint 言及更新 | scripts/ubo_codegen/*.py | 1 commit | python test 再実行 PASS confirm (= 138 test) |
| 7 | inventory:247-249 record 更新 + 必要時 design/ubo/WORK_ORDER.md / READINESS.md / RELATIONS.md / INDEX.md 更新 | docs/specs/ 1-5 file | 1 commit | doc 確認 |

各 sub-step 完了で AYA literal commit 指示待ち (= memory `feedback_no_auto_commit` 適用)。連結 commit は AYA literal 指示で可。

---

## §4. α-3 Exit 条件

| # | Exit 項目 | 判定基準 |
|---|---|---|
| 1 | 改修 1 (CMake) + 改修 3 (blueprint README) 完了 | git log + git diff confirm |
| 2 | 波及 doc 全件更新完了 (= §D.7 全 10 範囲) | grep `aya_r41_blueprints` で誤参照 / 古い記述 0 件 (= 設計 doc + per-UBO doc + source code) |
| 3 | codegen 単独走行 verify (= `ubo_metadata.inl` 80 UBO 全件 binding 値整合) | step 2-pre2 mapping §5.1 alphabetical sort literal と整合、git diff `ubo_metadata.inl` で diff 確認 |
| 4 | python test 138 件 PASS (= α-2 で確立) | regression 0 |
| 5 | α-4 着手準備完了 (= cold launch validation handoff entry 起案候補) | doc 起案 + AYA literal 指示 |

---

## §5. 手戻り protocol

- **α-3 内手戻り** = sub-step (= §3 表内) reject → 該当 sub-step 内 logic 再起案
- **α-3 → α-2 戻り** = main.py 改修不備発覚 (= 例: multi-input + `_verify_block_match` で漏れ検出) → α-2 commit `b66ec99f72` に追加修正
- **α-3 → α-1 戻り** = 設計方針破綻 (= 例: CMake 改修で別構造的問題発覚) → Phase 2.α entry handoff §D 再起案

手戻りは **失敗ではなく cycle の正常動作** (= memory `feedback_falsification_as_progress`)。

---

## §6. 起案規律 (= α-3 内維持)

- **AYA literal「別セッションで α-3 進めて」既発令** = preflight 質問なし (= memory `feedback_root_cause_no_shortcuts` §9)
- **設計 doc 整合確認 default** = 解決策確定前に設計 doc 精査 (= memory `feedback_doubt_self_first` §5、案 Y 撤回事例の反復防止)
- **AYA 指示 literal 完全一致 verify** = design/01:146 「85 GLSL UBO blueprint は discard しない」literal 整合維持、blueprint dir 物理保持
- **`mUseUBO` runtime flag default OFF 維持** (= 原則 4 §4.4 O-2)
- **`#ifdef LL_VULKAN_GLSL` C++ 不使用** (= 原則 4 §4.4 O-3)
- **視覚 regression ゼロ死守** (= 原則 4 §5.4、α-4 AYA live verify 必須)
- **3 OS 同一実装** (= 原則 OS-1〜OS-10、cmake + shader file + codegen tool は 3 OS 共通)
- **commit は AYA literal 指示後のみ** (= memory `feedback_no_auto_commit`)
- **push / PR は AYA 側** (= memory `feedback_release_flow`)
- **Co-Authored-By Claude 行禁止** (= memory `feedback_no_claude_coauthor`)

---

## §A. 関連 commit + doc

| 種別 | 内容 |
|---|---|
| 関連 commit | `64122994c1` (= Phase 2.α 案 Z 確定反映 doc 3 件) / `b66ec99f72` (= α-2 main.py + test 改修) / `db5cbcbc36` (= Phase 2.α 起案 + Phase 2.L0 freeze record) / `887ddb5341`〜`e5f57d57ff` (= sub-session 5 step 2-batch-0-a 7 commit、freeze 維持) |
| 関連 doc (本 entry の主要参照先) | `handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D (= 案 Z 確定 source of truth、§D.2 設計 doc 精査結果 + §D.6 5 改修 + §D.7 全件波及更新範囲 + §D.8 sub-session 5 整合) |
| 関連 doc (Phase 2.L0 freeze) | `handoff/phase2/audit/handoff-phase2-l0-1-C-step2-batch-0-a-entry.md` §C (= sub-session 5 freeze record) + `handoff/phase2/audit/handoff-phase2-l0-1-C-step2-pre2-mapping.md` §C (= step 2-pre2 freeze record + §5.1 80 slot alphabetical sort literal、α-3 codegen verify 時の expected 値 source) |
| 関連 source (改修対象) | `indra/cmake/AyaUboCodegen.cmake` (= 改修 1) + `indra/newview/app_settings/shaders/aya_r41_blueprints/README.md` (新規、改修 3) + 設計 doc 4 件 + per-UBO doc 80+ 件 + source code 5 file (= 波及 doc) |
| 関連 memory | (本 doc §2.3 cross-ref) |
| 本 doc | α-3 着手 entry handoff、commit 候補 (= AYA literal 指示後のみ) |

---

## §B. context 引き継ぎ (= memory `feedback_proactive_handoff` 適用)

**Phase 2.α α-2 完了 commit `b66ec99f72` + AYA literal「別セッションで α-3 進めて」(= 2026-06-06) = 自然な session 境界**。次 session で **/clear → 本 handoff doc + 必読 3 件 cold read → α-3 sub-step 1 (= aya_r41_blueprints/README.md 新規追加) 着手** を推奨。

**本 session 引継ぎ事項**:
- 本 handoff doc commit 候補 (= AYA literal 指示後、本 session 内 commit 想定)
- 次 session 初手 AYA 確認 = なし (= AYA literal 既発令で着手承認済、memory `feedback_root_cause_no_shortcuts` §9)
- α-3 完了後 = α-4 cold launch validation (= AYA 立ち会い別 session、α-3 完了 commit 後に α-4 着手 entry handoff 別起案 OR 本 doc §3 表内 #5 で記載済の検証 protocol を α-4 で展開)
- Phase 2.L0 sub-session 5 step 2-batch-0-a 7 commit (= `887ddb5341`〜`e5f57d57ff`) は **freeze 維持** (= class*/ + cinematic_bd/ 14 file 改修済、Phase 2.α 完了後 cold launch + AYA live verify で resume)
- memory 4 件 (= 新規 `feedback_root_cause_no_shortcuts` + 拡張 `feedback_design_doc_number_literal_verify` + 拡張 `feedback_doubt_self_first` §5 + 拡張 `feedback_root_cause_no_shortcuts` §8/§9/§10) は本 session 起案済、次 session 開始時 MEMORY.md index で確認

**次 session 着手 1 手目**: §2.1 必読 3 件 cold read → §3 sub-step 1 (= aya_r41_blueprints/README.md 新規追加) 着手、各 sub-step 完了報告で AYA literal commit 指示受領後 commit。
