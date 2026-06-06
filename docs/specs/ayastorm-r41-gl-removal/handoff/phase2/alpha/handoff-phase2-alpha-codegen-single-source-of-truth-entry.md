# handoff = r41 Phase 2.α 着手 entry = UBO codegen single source of truth 化

**起案日**: 2026-06-06
**位置付け**: r41 Phase 2.α (= 独立 phase) 着手 entry handoff。Phase 2.L0 freeze 中、Phase 2.α 完了後 L0 sub-session 5 (= step 2-batch-0-a の codegen 再生成 + cold launch) 直接 resume。
**起案契機**: Phase 2.L0 sub-session 5 step 2-batch-0-a 実施中に **codegen 入力 source の二重 source 構造発覚** (= `class*/` + `cinematic_bd/` 配下 GLSL は SPIR-V binary 入力、`aya_r41_blueprints/` 配下 GLSL は codegen `ubo_metadata.inl` 入力、整合断裂で Vulkan validation error 高確率)。AYA literal 2026-06-06「根治最優先、Phase2.α とでも...着手、終わったら現在時点で戻って L0 作業再開」受領。
**起案規律**:
- memory `feedback_root_cause_no_shortcuts` 適用 (= 根治のみ提示、対症療法案並列禁止、2026-06-06 新規 memory)
- memory `feedback_design_doc_number_literal_verify` 適用 (= 構造属性 + codegen/build pipeline 入力 source path 含む verify、2026-06-06 適用範囲再拡張済)
- memory `feedback_proactive_handoff` 適用 (= 周回境界 handoff)
- memory `feedback_handoff_minimal_pre_req_read` 適用 (= 最低限 3 件 + pinpoint)
- memory `feedback_ubo_migration_one_at_a_time` 適用 (= Phase 2.α 内も大塊一括 reject、sub-step 境界で test / cold launch 検証挟む)
- memory `feedback_build_only_verified` 適用

---

## §1. Phase 2.α 着手目的

**UBO codegen single source of truth 化** = `class*/` + `cinematic_bd/` 配下 GLSL を **唯一の source of truth** に統一、`aya_r41_blueprints/` 配下 blueprint dir を廃止 / reference 降格、二重 source 構造を構造的に解消。

### §1.1 根本悲報の構造

| 層 | 役割 | source path | 状態 (= Phase 2.L0 sub-session 5 中断時点) |
|---|---|---|---|
| SPIR-V binary 内 binding | shader compile 入力 | `indra/newview/app_settings/shaders/class*/` + `cinematic_bd/` | sub-session 5 で 7 UBO 書換済 (= class*/ 14 file 7 commit `887ddb5341`〜`e5f57d57ff`) |
| `ubo_metadata.inl` 内 binding | host C++ pipeline layout | `indra/newview/app_settings/shaders/aya_r41_blueprints/` | 旧 binding 残存 (= 7 UBO 全件未書換) |

⇒ **class*/ ↔ blueprint 二重 source、同期断裂 = Vulkan validation error 高確率**。本質は **構造的 fragility**、blueprint 同期 protocol で対症療法を組むと今後 UBO 追加 / binding 変更で同種悲報再発、出戻り工数 4-5 倍化 (= memory `feedback_root_cause_no_shortcuts` 該当事例)。

### §1.2 根治 thesis = single source of truth

`class*/` + `cinematic_bd/` 配下 GLSL を codegen 入力に変更、blueprint dir 廃止 / reference 降格。1 件の source from `class*/`、no 同期 protocol。今後 UBO 追加 / binding 変更で blueprint 同期忘れ悲報消滅。

---

## §2. Phase 2.α 改修 scope

### §2.1 改修内容 5 件

| # | 改修対象 | 改修内容 |
|---|---|---|
| 1 | `indra/cmake/AyaUboCodegen.cmake` | `AYA_UBO_CODEGEN_BLUEPRINT_DIR` 廃止 → `AYA_UBO_CODEGEN_SHADER_SOURCE_DIRS` (= `class*/` + `cinematic_bd/` 配下) に変更、`file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` pattern 変更、`--input` 引数を新 dir 群に切替、入力 path 一元化 |
| 2 | `scripts/ubo_codegen/main.py` | 同 UBO 複数 file 検出 + 整合検証 logic 追加 (= 同名 UBO 複数 declaration を集約、binding + std140 layout + member 全件一致 verify、不一致時 `CodegenError` abort)、blueprint「1 file 1 UBO」前提撤廃 (= 同名 UBO 複数 file declaration 許容、最初の declaration を spec として採用、他は整合 check のみ) |
| 3 | `aya_r41_blueprints/` | 役割再定義 (= AYA literal 確認 candidate、§7.1)、完全廃止 (= `git rm -r`) / reference 降格 (= codegen 入力対象外、設計参照のみ) どちらか |
| 4 | `scripts/ubo_codegen/tests/test_main.py` | 新規 logic 対応 test 追加 (= 複数 file 同名 UBO 整合 verify test、mismatch detect test、binding 値変更 verify test、`cinematic_bd/` 上書き path test)、blueprint 入力前提 test 撤廃 |
| 5 | 80 UBO 全件 cold launch validation | 改修後 codegen で `class*/` + `cinematic_bd/` 配下 80 UBO declaration を再生成、SPIR-V ↔ `ubo_metadata.inl` 整合 confirm (= Vulkan validation 0 件 + AYA live verify) |

### §2.2 sub-step 構造 (= 大塊一括 default reject)

| sub-step | 内容 | 改変対象 | 検証 | 想定 session |
|---|---|---|---|---|
| **α-1 設計** | Phase 2.α 改修方針確定 (= CMake input pattern + main.py 整合 verify logic + blueprint 役割 + test 構造 全件 doc 化) | doc のみ | AYA literal 承認 | 本 entry 後 1 session |
| **α-2 main.py 改修 + test** | 同 UBO 複数 file 検出 + 整合検証 logic 実装 + test 追加、`scripts/ubo_codegen/` 配下のみ改変、blueprint 入力で既存 test PASS 維持確認後新 logic 追加 | scripts/ubo_codegen/ | python test 単独実行 PASS | 1 session |
| **α-3 CMake 改修 + codegen 入力切替** | `AyaUboCodegen.cmake` で入力 dir 切替、blueprint dir 廃止 or reference 降格 | indra/cmake/ + (廃止採用時) aya_r41_blueprints/ 削除 | configure + codegen 単独走行 verify (= 改修後 `ubo_metadata.inl` を旧版と diff、80 UBO 全件 binding 整合 confirm) | 1 session |
| **α-4 80 UBO cold launch validation** | configure + build + cold launch + Vulkan validation + AYA live verify (= class*/ 既書換 7 UBO + 未書換 73 UBO 全件) | 0 (= 既存 class*/ source) | full cold launch (= AYA live) | 1 session |
| **α-exit** | Phase 2.α 完了 doc 起案 + Phase 2.L0 sub-session 5 resume 承認 | doc のみ | AYA literal 承認 | 本 session 内 or 別 session |

各 sub-step 完了で cold launch / test 検証挟む。1 session で完走想定せず。memory `feedback_ubo_migration_one_at_a_time` 適用、Phase 2.α 内も同規律。

---

## §3. 必読 doc (= memory `feedback_handoff_minimal_pre_req_read` 適用、最低限 3 件 + pinpoint)

### §3.1 最低限 3 件

1. **本 handoff doc** = Phase 2.α 着手 entry、scope + sub-step 構造 + freeze/resume protocol
2. **`indra/cmake/AyaUboCodegen.cmake`** = 現状 codegen pipeline wiring (= blueprint 入力 path 確定 source、§2.1 改修 1 source)
3. **`scripts/ubo_codegen/main.py`** = codegen entry point (= `_ubo_to_block_spec` (line 211-221), `_derive_subset` (line 200-208), `_process_glsl_file` (line 226-260) logic 確認、§2.1 改修 2 source)

### §3.2 必要時 pinpoint Read 候補

- `scripts/ubo_codegen/tests/test_main.py` = 既存 test pattern (= α-2 test 追加時)
- `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` = codegen pipeline 設計 doc (= §12 CMake wiring、§5 走行 logic)
- `indra/newview/app_settings/shaders/aya_r41_blueprints/` 配下 = blueprint file structure (= α-3 廃止 / 降格判断時)
- `handoff/phase2/audit/handoff-phase2-l0-1-C-step2-batch-0-a-entry.md` = Phase 2.L0 sub-session 5 entry (= freeze 元状態、resume protocol 参照)
- `handoff/phase2/audit/handoff-phase2-l0-1-C-step2-pre2-mapping.md` = 80 UBO 全件 alphabetical sort literal (= α-4 cold launch validation 時の expected binding 値 source)
- `indra/llrender/llvkloader.cpp:858-868` V3A_*_BINDINGS + `:5348-5385` `registerProgramUbo` subset 経路 dispatch (= α-4 host C++ pipeline layout 整合源)

### §3.3 memory pinpoint

- `feedback_root_cause_no_shortcuts` (= 根治のみ、本 Phase 起案根拠、2026-06-06 新規)
- `feedback_design_doc_number_literal_verify` (= 構造属性 + codegen/build pipeline 入力 source path 含む verify、適用範囲再拡張済)
- `project_r41_phase1b_vulkan_host_gate` / `project_r41_phase2_4_principles` / `project_r41_design_principles`
- `feedback_ubo_migration_one_at_a_time` (= Phase 2.α 内も大塊一括 reject)
- `feedback_admit_unknown` / `feedback_doubt_self_first` / `feedback_proactive_risk_management` / `feedback_no_scope_shrink`
- `feedback_build_only_verified` / `feedback_self_verify_before_handoff`
- `feedback_release_flow` / `feedback_no_claude_coauthor`

---

## §4. Phase 2.α Exit 条件

| # | Exit 項目 | 判定基準 |
|---|---|---|
| 1 | α-1 設計確定 | AyaUboCodegen.cmake + main.py + blueprint 役割 + test 構造の改修方針 doc 起案 + AYA literal 承認 |
| 2 | α-2 main.py 改修 + test PASS | 同名 UBO 複数 file 整合検証 logic 実装、test PASS (= 既存 + 新規 test 全件)、`scripts/ubo_codegen/` 配下のみ改変 |
| 3 | α-3 CMake 改修 + codegen 入力切替 | configure 通過、codegen 単独走行で `ubo_metadata.inl` が `class*/` + `cinematic_bd/` 配下 80 UBO 全件 (= step 2-pre2 mapping doc §5.1 alphabetical sort literal 整合) 出力、blueprint dir 廃止 / 降格処理完了 |
| 4 | α-4 80 UBO cold launch validation | 全 80 UBO で SPIR-V ↔ `ubo_metadata.inl` binding 整合、Vulkan validation 0 件、AYA live verify 視覚 regression 0 件 |
| 5 | α-exit + Phase 2.L0 resume 承認 | Phase 2.α 完了 doc 起案、Phase 2.L0 sub-session 5 codegen 再生成 + cold launch resume protocol 確定、AYA literal 承認 |

---

## §5. 手戻り protocol

- **Phase 2.α 内手戻り** = sub-step cold launch / test reject → 該当 sub-step 内 logic 再起案 (= 1 sub-step 1 session 想定で完走想定しない)
- **Phase 2.α → Phase 2.L0 戻り** = Phase 2.α 改修方針自体破綻 (= codegen pipeline 改修不可、別 approach 必要) → Phase 2.L0 sub-session 5 revert 必要性判断 (= class*/ 14 file 7 commit revert or 維持で別根治起案)
- **Phase 2.α → 別 root cause 戻り** = Phase 2.L0 sub-session 5 の 7 commit が想定外副作用 (= class*/ 書換単独で他 module 影響) → revert + Phase 2.α scope 拡張

手戻りは **失敗ではなく cycle の正常動作** (= memory `feedback_falsification_as_progress`)。

---

## §6. 起案規律

- **根治のみ提示** (= memory `feedback_root_cause_no_shortcuts`、対症療法案並列禁止、「scope 逸脱、別 phase 推奨」も先送り signal として禁止)
- **`mUseUBO` runtime flag default OFF 維持** (= 原則 4 §4.4 O-2)
- **`#ifdef LL_VULKAN_GLSL` C++ 不使用** (= 原則 4 §4.4 O-3、memory `project_r41_phase1b_vulkan_host_gate`)
- **視覚 regression ゼロ死守** (= 原則 4 §5.4、α-4 AYA live verify 必須)
- **3 OS 同一実装** (= 原則 OS-1〜OS-10、shader file + codegen tool は 3 OS 共通)
- **数値 + 構造属性 + codegen/build pipeline 入力 source path verify** (= memory `feedback_design_doc_number_literal_verify` 適用範囲拡張済)
- **Phase 2.L0 sub-session 5 freeze 維持** (= class*/ 14 file 7 commit `887ddb5341`〜`e5f57d57ff` 改変なし、cold launch せず)
- **commit は AYA literal 指示後のみ** (= memory `feedback_no_auto_commit`)
- **push / PR は AYA 側** (= memory `feedback_release_flow`)

---

## §7. AYA literal 確認

### §7.1 blueprint 役割再定義 (= 根治内実装方針バリアント)

| candidate | 内容 | pros | cons |
|---|---|---|---|
| **α-blueprint-1** | 完全廃止 (= `git rm -r aya_r41_blueprints/`) | repo 簡素化、二重 source 物理消滅、誤参照リスク 0、将来新 UBO 追加時に blueprint 書く誘惑消滅 | 過去設計時 reference 喪失 (= git log + 過去 commit に保存) |
| **α-blueprint-2** | reference 降格 (= dir 保持、codegen 入力対象外、設計参照のみ、README 更新で「source of truth ではない」明示) | Phase 1 履歴 file として保持、設計時参照可能 | repo 内に二重 declaration 残存 = 将来の誤参照 / 誤 sync リスク = 根治不徹底、新 UBO 追加時 blueprint も書く誘惑 |

**Claude 推奨** = **α-blueprint-1 (完全廃止)**

理由:
- reference 降格 (= α-blueprint-2) は repo に旧 source 残存 = 将来の誤参照 / 誤 sync リスク = **根治不徹底** (= memory `feedback_root_cause_no_shortcuts` 該当)
- Phase 1 履歴は git log + 過去 commit (= `cecb55ceb1` 等) に保存済、blueprint file 自体は不要
- 設計時参照は `class*/` 配下 GLSL + design doc (= `docs/specs/ayastorm-r41-gl-removal/design/`) で代替可能 (= aya_r41_blueprints/ は historical extract で、原本ではない)

**AYA literal 確認要請**: α-blueprint-1 / α-blueprint-2 のどちらを採用?

### §7.2 Phase 2.α 着手承認

AYA literal「α-blueprint-1 / α-blueprint-2」回答受領 + 着手承認 → Phase 2.α sub-step α-1 (= 設計確定) 着手:
1. AyaUboCodegen.cmake + main.py + test 改修方針 doc 起案 (= `handoff/phase2/alpha/handoff-phase2-alpha-1-design.md`)
2. AYA literal 確認 + α-2 (= main.py 改修 + test) 着手承認

---

## §A. 関連 commit + doc

| 種別 | 内容 |
|---|---|
| Phase 2.L0 freeze 元 | sub-session 5 step 2-batch-0-a 7 commit (= `887ddb5341` CloudsVParamUBO_Legacy / `e539b384ed` PerProgramUBO_GammaCorrect / `f91cda6677` PerProgramUBO_PointLightV / `d4cabb8cfc` PerProgramUBO_PostDeferredF / `5d21f1af9c` PerProgramUBO_WaterHazeV / `c9620edcc7` ShadowUtilParamUBO_Legacy / `e5f57d57ff` WaterVParamUBO_Legacy) |
| Phase 2.L0 sub-session 5 entry | `handoff/phase2/audit/handoff-phase2-l0-1-C-step2-batch-0-a-entry.md` (= commit `fa1dec9566`、freeze 中、Phase 2.α 完了後 resume) |
| Phase 2.L0 step 2-pre2 mapping | `handoff/phase2/audit/handoff-phase2-l0-1-C-step2-pre2-mapping.md` (= commit `d89cfb684b`、80 UBO alphabetical sort literal 全件 source) |
| 関連 source (改修対象) | `indra/cmake/AyaUboCodegen.cmake` + `scripts/ubo_codegen/main.py` + `scripts/ubo_codegen/tests/test_main.py` + `indra/newview/app_settings/shaders/aya_r41_blueprints/` |
| 設計 doc | `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` |
| 関連 memory | `feedback_root_cause_no_shortcuts` (= 本 Phase 起案根拠、2026-06-06 新規) / `feedback_design_doc_number_literal_verify` (= 拡張済) / 他 (本 doc §3.3 cross-ref) |
| 本 doc | Phase 2.α 着手 entry handoff、commit 候補 (= AYA literal 指示後のみ) |

---

## §B. context 引き継ぎ (= memory `feedback_proactive_handoff` 適用)

**Phase 2.L0 sub-session 5 step 2-batch-0-a 7 commit 完了 + Phase 2.α 起案 = 自然な session 境界**。次 session で **/clear → 本 handoff doc + 必読 3 件 cold read → Phase 2.α sub-step α-1 (= 設計確定) 着手** を推奨。

**Phase 2.L0 resume protocol** (= Phase 2.α 完了後):
1. Phase 2.α α-exit 完了 commit
2. Phase 2.L0 sub-session 5 entry handoff §B 更新 (= resume status)
3. Phase 2.L0 sub-session 5 codegen 再生成 (= 改修済 codegen pipeline 経由、`class*/` 入力で 7 UBO 新 binding が `ubo_metadata.inl` に自動反映) + cold launch + AYA live verify
4. Phase 2.L0 sub-session 5 Exit 条件全件 satisfy 後 sub-session 6 (= step 2-batch-0-b MaterialUBO 49 file 単独) 着手

**本 session 引継ぎ事項**:
- 本 entry handoff doc + Phase 2.L0 sub-session 5 entry handoff freeze record + step 2-pre2 mapping doc freeze record の 3 doc 改修 + memory 2 件 (= 新規 `feedback_root_cause_no_shortcuts` + 拡張 `feedback_design_doc_number_literal_verify`) を 1 commit にバンドル候補 (= AYA literal 指示後、memory は git 追跡外ゆえ doc 3 件のみ commit)
- 次 session 初手 AYA 確認 = §7.1 blueprint 役割再定義 + §7.2 Phase 2.α 着手承認
