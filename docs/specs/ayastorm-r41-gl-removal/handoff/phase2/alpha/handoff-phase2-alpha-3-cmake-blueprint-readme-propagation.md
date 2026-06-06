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

**2026-06-06 案 X 確定で sub-step 順序全面再起案** (= 案 Z'+ 全撤回 + revert + 案 X 採用、entry handoff §D.9 案 X 確定 source of truth、6 commit revert で base state 復元済)。

### §3.1 phase A = revert (= 完了済)

| # | 改修内容 | 改変対象 | 想定 commit | 検証 |
|---|---|---|---|---|
| A | 案 Z'+ 全撤回 = 6 commit revert (= `9c3b3f3d72` + `a97b3e1b6b` + `9abae83730` + `bfacb1f50f` + `246535626e` + `862f9cb983`) | 14 file revert | 完了 (= commit `df38b7c994`) | test 138 件全 PASS + blueprint dir 単独走行 94 UBO emit 成功 |

### §3.2 phase B = blueprint dir README 全面書換 (= 完了済)

| # | 改修内容 | 改変対象 | 想定 commit | 検証 |
|---|---|---|---|---|
| B | `aya_r41_blueprints/README.md` 全面書換 = 「codegen 入力 source of truth」literal 確定 | shader dir + README 1 file 新規追加 | 完了 (= commit `f95182ded5`) | doc 確認 |

### §3.3 phase C = handoff doc 2 件改訂 (= 本 sub-step、案 X 確定 source of truth)

| # | 改修内容 | 改変対象 | 想定 commit | 検証 |
|---|---|---|---|---|
| C | entry handoff §D.9 (= 案 X 確定 source of truth) 新規追加 + α-3 entry §3 sub-step 順序全面再起案 (= 本表) | handoff doc 2 file | 1 commit (= 本 sub-step) | doc 確認 + cross-ref 整合 confirm |

### §3.4 phase D = 設計 doc 5 件改修 (= 案 X 反映)

| # | 改修内容 | 改変対象 | 想定 commit | 検証 |
|---|---|---|---|---|
| D | 設計 doc 5 件改修 = (a) 04 §2.2 literal 訂正 = 「同じ GLSL 入力」誤りを「**別 GLSL 並列 build process**」literal に修正、(b) 04 §4.4 同名 UBO 複数 GLSL 宣言の literal 維持 (= blueprint + actual 二重 source 整合 verify 設計、`_verify_block_match` 拡張対応)、(c) 案 Z'+ 由来 C++ runtime emulation 層 § (= 04 §4.5 + 06a §4.5 + 08 §5.0) は revert で削除済 = 案 X では追加 § 不要 (= base state 整合)、(d) 09 Phase 2.α 案 X 確定反映、(e) 10 open questions 案 X 確定で blueprint 関連 closed 化 + 二重 source 同期関連 open question 棚卸し | docs/specs/ 5 file | 1-2 commit | doc 確認 + grep 04 §2.2 literal 訂正確認 |

### §3.5 phase E = blueprint dir 7 UBO 14 file 同期書換

| # | 改修内容 | 改変対象 | 想定 commit | 検証 |
|---|---|---|---|---|
| E | Phase 2.L0 sub-session 5 step 2-batch-0-a 7 commit で actual `class*/` + `cinematic_bd/` **14 file** 改修済 (= 同名 UBO 複数 file 構造ゆえ) の新 set/binding を **blueprint dir 内 対応 7 UBO の 7 file (= blueprint dir = 1 UBO 1 file 構造) に同期反映** (= 二重 source 同期断裂解消)、対象 UBO = CloudsVParamUBO_Legacy slot 8 / PerProgramUBO_GammaCorrect slot 39 / PerProgramUBO_PointLightV slot 44 / PerProgramUBO_PostDeferredF slot 45 / PerProgramUBO_WaterHazeV slot 55 / ShadowUtilParamUBO_Legacy slot 63 / WaterVParamUBO_Legacy slot 79 | blueprint dir 内 **7 file** (= 旧記載「14 file」は誤り、actual class*/ + cinematic_bd/ 側の 14 file との混同、blueprint dir 構造で 7 UBO = 7 file 整合) | 1 commit | codegen 単独走行で `ubo_metadata.inl` 新 binding 反映 confirm |

### §3.6 phase F = main.py `_verify_block_match` 拡張 (= 二重 source 同期 protocol formal化)

| # | 改修内容 | 改変対象 | 想定 commit | 検証 |
|---|---|---|---|---|
| F | `scripts/ubo_codegen/main.py` 拡張 = (a) `_verify_block_match` 拡張 = blueprint と actual の対称的整合 verify、(b) `--input` で blueprint dir + actual shader path 両方受領、(c) `--verify-target-paths` option 新規追加 = blueprint と actual を区別して二重 source 整合 verify、(d) 同名 UBO 複数 file (= blueprint + actual の cross-source pair) 検出 + 整合検証 logic + test 追加 | scripts/ubo_codegen/ | 1 commit | python test 拡張 PASS (= 既 138 + 新規 5-7 件) + codegen 走行で blueprint + actual 整合 verify confirm |

### §3.7 phase G = 波及 doc 全件改修

| # | 改修内容 | 改変対象 | 想定 commit | 検証 |
|---|---|---|---|---|
| G-1 | per-UBO doc 80+ 件 (= `design/ubo/*.md`) blueprint 言及更新 = 「**codegen 入力 source of truth**」位置付け literal 統一 + 二重 source 同期 (= actual との対応) cross-ref 追加 | design/ubo/*.md 80+ file | 1-3 commit (= sub-batch 分割可、grep + sed 一括 OK) | doc 確認 + git diff review |
| G-2 | source code 5+ file (= main.py + glsl_parser.py + std140.py + perfect_hash.py + spirv_reflect.py) docstring 内 blueprint 言及更新 = 案 X 確定後の「codegen 入力 source of truth」literal 反映 | scripts/ubo_codegen/*.py | 1 commit | python test 再実行 PASS confirm |
| G-3 | inventory:247-249 record 更新 = 「Phase 2.α 案 X で main.py `_verify_block_match` 拡張で blueprint + actual の二重 source 整合 verify」反映 + 必要時 `design/ubo/WORK_ORDER.md` / `READINESS.md` / `RELATIONS.md` / `INDEX.md` 更新 | docs/specs/ 1-5 file | 1 commit | doc 確認 |
| G-final | 全件 grep 走査残漏れ 0 件 confirm (= keyword = `blueprint` 言及 / `BLUEPRINT_DIR` / `class\*/` / `addPermutation` / `runtime emulation` 等) | (= verify のみ) | 0 commit (= verify 結果は inventory に追記) | grep 走査 |

各 sub-step 完了で AYA literal commit 指示待ち (= memory `feedback_no_auto_commit` 適用)。連結 commit は AYA literal 指示で可。

---

## §4. α-3 Exit 条件 (= 案 X で再起案)

| # | Exit 項目 | 判定基準 |
|---|---|---|
| 1 | phase A revert + phase B blueprint README + phase C handoff doc 完了 | git log + git diff confirm (= `df38b7c994` + `f95182ded5` + 本 commit) |
| 2 | phase D 設計 doc 5 件改修完了 = 04 §2.2 literal「同じ GLSL 入力」→「別 GLSL 並列 build process」訂正 + 案 Z'+ 由来 § revert 確認 + 09/10 案 X 反映 | grep 04 §2.2 確認 + cross-ref 整合 confirm |
| 3 | phase E blueprint dir 7 UBO 14 file 同期書換完了 | codegen 単独走行で `ubo_metadata.inl` 新 binding 反映 confirm (= step 2-pre2 mapping §5.1 alphabetical sort literal と整合) |
| 4 | phase F main.py `_verify_block_match` 拡張完了 = blueprint + actual 二重 source 整合 verify formal化 | python test 拡張 PASS (= 既 138 + 新規 5-7 件) + codegen 走行で 整合 verify confirm |
| 5 | phase G 波及 doc 全件改修完了 = per-UBO doc 80+ + source code + inventory + WORK_ORDER 等 + 全件 grep 走査残漏れ 0 件 confirm | grep keyword 全件確認 (= blueprint / BLUEPRINT_DIR / `class\*/` / addPermutation / runtime emulation) |
| 6 | α-4 着手準備完了 (= cold launch validation handoff entry 起案候補) | doc 起案 + AYA literal 指示 |

---

## §5. 手戻り protocol

- **α-3 内手戻り** = sub-step (= §3 表内) reject → 該当 sub-step 内 logic 再起案
- **α-3 → α-2 戻り** = main.py 改修不備発覚 (= 例: `_verify_block_match` 拡張で漏れ検出) → α-2 commit `b66ec99f72` に追加修正
- **α-3 → α-1 戻り** = 設計方針破綻 (= 例: 構造的問題発覚) → Phase 2.α entry handoff §D 再起案
- **「同じ穴」事例 record** (= 2026-06-06 5 連続落ち):
  - 案 Y (= blueprint 完全廃止) AYA 指示 #5 違反で撤回
  - 案 Z (= class*/ + cinematic_bd/ 入力切替) parse error 第 1 階層発覚で撤回
  - 案 Z' (= 案 Z + C++ runtime emulation 層追加) parse error 第 2 階層発覚で撤回
  - 案 X (= blueprint dir = codegen 入力 source of truth) **確定** = blueprint dir 単独走行で 94 UBO emit + dump file なし + parse error 0 件 evidence、設計 doc 04 §2.2 literal 誤り訂正、AYA 指示 #5 真意 = source of truth 保護指示
  - memory `feedback_root_cause_no_shortcuts` §11 + §12 適用 (= 自走精査網羅性 checklist 4 件 + sandbox 実証 protocol + 採否ご判断ください禁忌 + subagent 結果盲信禁止 + 設計 doc literal 誤り疑念)

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
| 関連 commit | **保持**: `64122994c1` (= 案 Z 履歴) / `b66ec99f72` (= α-2 main.py + test 改修、案 X phase F baseline) / `db5cbcbc36` (= Phase 2.α 起案 + freeze record) / `310d58b556` (= α-3 entry handoff 起案) / `887ddb5341`〜`e5f57d57ff` (= sub-session 5 step 2-batch-0-a 7 commit、freeze 維持)。 **revert 済**: `9c3b3f3d72` / `a97b3e1b6b` / `9abae83730` / `bfacb1f50f` / `246535626e` / `862f9cb983` (= 案 Z'+ 6 件、revert `df38b7c994` で base state 復元)。 **案 X 確定 commit**: `df38b7c994` (= phase A) / `f95182ded5` (= phase B blueprint README 全面書換) |
| 関連 doc (本 entry の主要参照先) | `handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` **§D.9 (= 案 X 確定 source of truth、§D.9.1 4-5 件目発火経緯 + §D.9.2 blueprint dir 単独走行 evidence + §D.9.3 設計 doc 04 §2.2 literal 誤り発覚 + §D.9.4 案 Y/Z/Z' 撤回 + 案 X 確定経緯 + §D.9.5 6 commit revert + §D.9.6 案 X 改修方針 9 件 + §D.9.7 全件波及更新範囲 10 件 + §D.9.8 既保持 commit 整合性 + §D.9.9 §11+§12 反省)** + §D (= 案 Z 履歴) |
| 関連 doc (Phase 2.L0 freeze) | `handoff/phase2/audit/handoff-phase2-l0-1-C-step2-batch-0-a-entry.md` §C (= sub-session 5 freeze record) + `handoff/phase2/audit/handoff-phase2-l0-1-C-step2-pre2-mapping.md` §C (= step 2-pre2 freeze record + §5.1 80 slot alphabetical sort literal、α-3 codegen verify 時の expected 値 source) |
| 関連 source (改修対象、案 X) | **既 commit 保持/復元**: `indra/cmake/AyaUboCodegen.cmake` (= `BLUEPRINT_DIR` 復元、base state) + `aya_r41_blueprints/README.md` (= phase B commit `f95182ded5` 全面書換) + `scripts/ubo_codegen/main.py` (= α-2 commit `b66ec99f72`)。 **案 X 新規/拡張**: 設計 doc 5 件 (= 04 §2.2 literal 訂正等、phase D) + blueprint dir 内 7 UBO 14 file 同期書換 (= phase E) + main.py `_verify_block_match` 拡張 + `--verify-target-paths` option (= phase F) + per-UBO doc 80+ 件 + inventory + WORK_ORDER 等 (= phase G) |
| 関連 memory | (本 doc §2.3 cross-ref) |
| 本 doc | α-3 着手 entry handoff、commit 候補 (= AYA literal 指示後のみ) |

---

## §B. context 引き継ぎ (= memory `feedback_proactive_handoff` 適用)

**Phase 2.α α-2 完了 commit `b66ec99f72` + AYA literal「別セッションで α-3 進めて」(= 2026-06-06) = 自然な session 境界**。次 session で **/clear → 本 handoff doc + 必読 3 件 cold read → α-3 sub-step 1 (= aya_r41_blueprints/README.md 新規追加) 着手** を推奨。

**本 session 引継ぎ事項** (= 案 X 確定後の record):
- 本 session 内 phase A-C 完了 commit = `df38b7c994` (= revert) + `f95182ded5` (= blueprint README) + 本 sub-step commit (= handoff doc 2 件改訂)
- 次 session 初手 AYA 確認 = なし (= AYA literal 既発令で着手承認済、memory `feedback_root_cause_no_shortcuts` §9 / §11 / §12)
- α-3 完了後 = α-4 cold launch validation (= AYA 立ち会い別 session、α-3 完了 commit 後に α-4 着手 entry handoff 別起案 OR 本 doc §3.7 phase G verify protocol を α-4 で展開)
- Phase 2.L0 sub-session 5 step 2-batch-0-a 7 commit (= `887ddb5341`〜`e5f57d57ff`) は **freeze 維持** (= class*/ + cinematic_bd/ 14 file 改修済、案 X phase E で blueprint dir 側 14 file 同期書換実施、Phase 2.α 完了後 cold launch + AYA live verify で resume)
- memory 6 件 (= 新規 `feedback_root_cause_no_shortcuts` + 拡張 `feedback_design_doc_number_literal_verify` + 拡張 `feedback_doubt_self_first` §5 + 拡張 `feedback_root_cause_no_shortcuts` §8/§9/§10 + 拡張 §11 (= 自走精査網羅性 checklist 4 件) + 拡張 §12 (= sandbox 実証 protocol + 採否ご判断ください禁忌 + subagent 結果盲信禁止 + 設計 doc literal 誤り疑念)) は本 session 起案済、次 session 開始時 MEMORY.md index で確認

**次 session 着手 1 手目 (= 案 X 採用後)**: §2.1 必読 3 件 cold read + entry handoff §D.9 案 X source of truth 確認 → §3.1-§3.3 phase A-C 完了確認 → §3.4 phase D (= 設計 doc 5 件改修 = 04 §2.2 literal 訂正 + 案 Z'+ § revert 確認 + 09/10 案 X 反映) 着手、各 sub-step 完了報告で AYA literal commit 指示受領後 commit。
