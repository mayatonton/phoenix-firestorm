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

### §1.2 根治 thesis = 設計 doc 整合修復 (= 案 Z、2026-06-06 設計 doc 精査結果反映、§D 参照)

`class*/` + `cinematic_bd/` 配下 GLSL を codegen 入力に変更 (= **設計 doc 08:72-74/96 想定整合復元** = 「入力: `app_settings/shaders/class*/{deferred,interface,...}/**.glsl`」literal、現状 AyaUboCodegen.cmake `aya_r41_blueprints/` 固定は設計乖離)。**blueprint dir は reference 降格で保持** (= **AYA 指示 #5 「85 GLSL UBO blueprint は discard しない」literal 整合**、design/01-overview.md:146 + design/04-codegen-ubo.md §967「blueprint は discard せず再利用」)。1 件の source from `class*/`、no 同期 protocol。今後 UBO 追加 / binding 変更で blueprint 同期忘れ悲報消滅、blueprint dir は設計時参考資料 / Phase 1 履歴として保持。

**前案 (= 案 Y blueprint 完全廃止) 撤回 record**: 2026-06-06 §7.1 で α-blueprint-1 (= 完全廃止) を Claude 推奨案として提示、AYA literal「これをわたしに確認すること自体が腹立たしい / どうすれば根治するか確定して改修してください / 波及する資料もすべて更新 / **この穴を作った原因元資料の精査はしないんですか？それが間違っていたから今間違えてるんじゃないんですか？解決方法は本当にこれで正しいのですか？**」受領で設計 doc 精査着手 = AYA 指示 #5 違反 (= 完全廃止案) 確定 + 設計 doc 08:72 想定見落し確定、案 Z (= 設計 doc 整合修復) に確定切替。詳細 = §D。

---

## §2. Phase 2.α 改修 scope

### §2.1 改修内容 5 件

| # | 改修対象 | 改修内容 |
|---|---|---|
| 1 | `indra/cmake/AyaUboCodegen.cmake` | `AYA_UBO_CODEGEN_BLUEPRINT_DIR` 廃止 → `AYA_UBO_CODEGEN_SHADER_SOURCE_DIRS` (= `class*/` + `cinematic_bd/` 配下) に変更、`file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` pattern 変更、`--input` 引数を新 dir 群に切替、入力 path 一元化 |
| 2 | `scripts/ubo_codegen/main.py` | 同 UBO 複数 file 検出 + 整合検証 logic 追加 (= 同名 UBO 複数 declaration を集約、binding + std140 layout + member 全件一致 verify、不一致時 `CodegenError` abort)、blueprint「1 file 1 UBO」前提撤廃 (= 同名 UBO 複数 file declaration 許容、最初の declaration を spec として採用、他は整合 check のみ) |
| 3 | `aya_r41_blueprints/` | **reference 降格で保持** (= 案 Z 確定 2026-06-06、§D 参照、AYA 指示 #5 整合): dir 保持、codegen 入力対象外、内部 README 更新で「設計時参考資料 / Phase 1 履歴 / source of truth ではない / 編集禁止」明示。**完全廃止案 (= 前案 Y) は AYA 指示 #5 違反確定で撤回**。 |
| 4 | `scripts/ubo_codegen/tests/test_main.py` | 新規 logic 対応 test 追加 (= 複数 file 同名 UBO 整合 verify test、mismatch detect test、binding 値変更 verify test、`cinematic_bd/` 上書き path test)、blueprint 入力前提 test 撤廃 |
| 5 | 80 UBO 全件 cold launch validation | 改修後 codegen で `class*/` + `cinematic_bd/` 配下 80 UBO declaration を再生成、SPIR-V ↔ `ubo_metadata.inl` 整合 confirm (= Vulkan validation 0 件 + AYA live verify) |

### §2.2 sub-step 構造 (= 大塊一括 default reject)

| sub-step | 内容 | 改変対象 | 検証 | 想定 session |
|---|---|---|---|---|
| **α-1 設計** | Phase 2.α 改修方針確定 (= CMake input pattern + main.py 整合 verify logic + blueprint 役割 + test 構造 全件 doc 化) ⇒ **本 entry handoff §D で代替実施完了** (= 2026-06-06、案 Z 反映 + 設計 doc 精査結果 + 案 Y 撤回 record + 5 改修方針 §D.6 + α-3 全件波及更新範囲 §D.7、memory `feedback_no_dual_doc_split` 整合で別 α-1 doc 起案省略) | doc のみ | ✅ §D で代替実施完了 | ✅ 本 entry 内で完了 |
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

## §7. AYA literal 確認 = **本 section 撤回** (= 2026-06-06、§D 参照)

§7.1 blueprint 役割再定義 candidate 並列 + §7.2 着手承認確認は **撤回**。理由:
- §7.1 (= α-blueprint-1 完全廃止 / α-blueprint-2 reference 降格 candidate 並列) = memory `feedback_root_cause_no_shortcuts` §8「根治徹底度の差を candidate 並列で出すのは対症療法バリアント」違反、かつ完全廃止案 (= α-blueprint-1) は **AYA 指示 #5「85 GLSL UBO blueprint は discard しない」literal 違反確定** (= design/01-overview.md:146)
- §7.2 (= 着手承認確認) = memory `feedback_root_cause_no_shortcuts` §9「既 AYA literal 根治意思表示済 (= 「Phase2.α とでも...着手」literal) で改修方針内細部 candidate 確認も先送り signal」違反

確定:
- **blueprint 役割 = reference 降格保持** (= 案 Z 単独、§2.1 改修 3 反映、AYA 指示 #5 整合)
- **着手承認** = AYA literal 「Phase2.α とでも...着手、終わったら現在時点で戻って L0 作業再開」(= 2026-06-06) で既発令、preflight 質問不要
- 各 sub-step (= α-1 / α-2 / α-3 / α-4) 完了報告で reject 機会を残す protocol (= 自走着手)

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
- 本 entry handoff doc + Phase 2.L0 sub-session 5 entry handoff freeze record + step 2-pre2 mapping doc freeze record の 3 doc 改修 + memory 4 件 (= 新規 `feedback_root_cause_no_shortcuts` + 拡張 `feedback_design_doc_number_literal_verify` + 拡張 `feedback_doubt_self_first` §5 + 拡張 `feedback_root_cause_no_shortcuts` §10 + 3 件目発火 record) を 1 commit にバンドル候補 (= AYA literal 指示後、memory は git 追跡外ゆえ doc 3 件のみ commit)
- 次 session 初手 AYA 確認 = なし (= §7 撤回、案 Z 確定済)、α-1 設計 sub-step 直接着手

---

## §D. 設計 doc 精査結果 + 案 Y 撤回 + 案 Z 確定 record (= 2026-06-06 追記)

### §D.1 起案契機 = AYA literal 3 件受領 (= 2026-06-06)

本 entry handoff §7.1 で α-blueprint-1 (= 完全廃止) / α-blueprint-2 (= reference 降格) candidate 並列提示後、AYA literal 3 連続受領:
1. 「これをわたしに確認すること自体が腹立たしいのですが、どうすれば根治するか確定して改修してください」
2. 「波及する資料もすべて更新するのを忘れないでください。また誤解して同じ穴に落ちます。」
3. 「この穴を作った原因元資料の精査はしないんですか？それが間違っていたから今間違えてるんじゃないんですか？解決方法は本当にこれで正しいのですか？」

3 連続受領で **設計 doc (= source of truth) 精査着手**、案 Y (= blueprint 完全廃止) が AYA 指示違反 + 設計乖離見落しと確定、案 Z (= 設計 doc 整合修復) に切替。

### §D.2 設計 doc 精査結果 = 重要発見 3 件

| # | source | literal | 含意 |
|---|---|---|---|
| 1 | **design/01-overview.md:146** | 「**既存 85 GLSL UBO blueprint は discard しない** (parse error 解消の蓄積を温存)、出典 = AYA 指示」(= §1.2 確定方針 #5) | **案 Y (= 完全廃止) は AYA 指示 #5 違反確定** |
| 2 | **design/08-build-codegen-pipeline.md:72-74/96** | `codegen_ubo` 入力 = `app_settings/shaders/class*/{deferred,interface,...}/**.glsl`、DEPENDS = `app_settings/shaders/**.glsl` literal | **設計時想定 = class*/ + cinematic_bd/ が codegen 入力**、現状 AyaUboCodegen.cmake の `aya_r41_blueprints/` 固定は **設計乖離** = 二重 source 構造の真の原因 |
| 3 | **ayastorm-r41-ubo-current-state-inventory.md:247-249** | 「同名 UBO が複数 file で再宣言されている例あり: CloudsVParamUBO_Legacy / WaterVParamUBO_Legacy / ShadowUtilParamUBO_Legacy (cinematic_bd 含む)」(= 2026-06-03 起案時 record) | **同名 UBO 複数 file 再宣言は 2026-06-03 既認識**、対応方針未明示 = inventory 検討漏れ、main.py 同名 UBO 整合 verify logic 追加で構造的検出可能 |

### §D.3 案 Y (= blueprint 完全廃止) 撤回根拠

- AYA 指示 #5 違反確定 (= §D.2 #1)
- 「dir 残存 = 将来誤参照リスク残存」推奨理由 (= §7.1) は **設計 doc 精査怠った推測** = design/01:146 / design/08:72 を精査していれば不要だった
- memory `feedback_doubt_self_first` §5 + `feedback_root_cause_no_shortcuts` §10 (= 2026-06-06 追加項目) 該当事例

### §D.4 案 Z (= 設計 doc 整合修復) 確定根拠

| 軸 | 案 Z 整合根拠 |
|---|---|
| AYA 指示 #5 | ✅ blueprint dir 保持 (= discard しない literal 整合) |
| 設計 doc 08:72-74/96 想定 | ✅ codegen 入力 = class*/ + cinematic_bd/ 復元 (= 設計時想定通り) |
| 設計 doc 04:967 「blueprint は discard せず再利用」 | ✅ blueprint dir 保持 (= 設計時参考資料 / Phase 1 履歴) |
| memory `feedback_root_cause_no_shortcuts` 整合 | ✅ 案 Z 単独提示、対症療法案並列なし |
| inventory:247-249 既知悲報の構造的検出 | ✅ main.py 同名 UBO 複数 file 整合 verify logic 追加 |
| sub-session 5 step 2-batch-0-a 7 commit 整合 | ✅ class*/ + cinematic_bd/ 14 file 改修済 = codegen 入力切替で binding 値自動反映、freeze 維持 |

### §D.5 memory 追加 + 拡張 record (= 2026-06-06)

- 新規: `feedback_root_cause_no_shortcuts` (= 起案、本日 1 件目発火時)
- 拡張: `feedback_design_doc_number_literal_verify` (= 構造属性に codegen/build pipeline 入力 source path 追加、適用範囲再拡張)
- 拡張: `feedback_root_cause_no_shortcuts` §8 (= 「根治徹底度の差」candidate 並列禁止、2 件目発火受領)
- 拡張: `feedback_root_cause_no_shortcuts` §9 (= 既 AYA literal 根治意思表示済で preflight 質問禁止、2 件目発火受領)
- 拡張: `feedback_root_cause_no_shortcuts` §10 (= 設計 doc 整合確認なしで根治確定する罪、3 件目発火受領)
- 拡張: `feedback_doubt_self_first` §5 (= 解決策確定前に設計 doc 精査 default、3 件目発火受領)

### §D.6 案 Z 改修方針 (= §2.1 5 改修の確定詳細、α-1 設計 sub-step で詳細化予定)

| # | 改修対象 | 改修内容 (= 案 Z 確定) |
|---|---|---|
| 1 | `indra/cmake/AyaUboCodegen.cmake` | `AYA_UBO_CODEGEN_BLUEPRINT_DIR` 廃止 (= 変数名 + 値とも削除) → 新規 `AYA_UBO_CODEGEN_SHADER_SOURCE_DIRS` 導入 = `${CMAKE_SOURCE_DIR}/newview/app_settings/shaders/class1` + `class2` + `class3` + `cinematic_bd` の 4 path list、`file(GLOB_RECURSE CONFIGURE_DEPENDS)` pattern 4 path 連合、`--input` 引数群を新 dir 群に対応 (= main.py multi-input 対応要)、設計 doc 08:72-74/96 想定整合復元 |
| 2 | `scripts/ubo_codegen/main.py` | (a) `--input` 複数指定対応 (= argparse `nargs='+'` or 複数 argument) (b) 同 UBO 複数 file 検出 + 整合検証 logic 追加 (= 同名 UBO 複数 declaration を集約、binding + std140 layout + member 全件一致 verify、不一致時 `CodegenError` abort、source_file が `cinematic_bd/` 配下の場合は class*/ 上書き path として許容)、blueprint「1 file 1 UBO」前提撤廃 |
| 3 | `aya_r41_blueprints/` | **reference 降格保持** (= 案 Z 確定): dir + 全 .glsl file 維持、codegen 入力対象外、内部 `README.md` 新規追加 (= 「設計時参考資料 / Phase 1 履歴 / source of truth ではない / 編集禁止 / codegen 入力対象外」明示、AYA 指示 #5 + design/01:146 + design/04:967 cross-ref) |
| 4 | `scripts/ubo_codegen/tests/test_main.py` | (a) 複数 input dir + 複数 file 同名 UBO 整合 verify test 追加 (b) mismatch detect test (= binding 不一致 / member 不一致) (c) `cinematic_bd/` 上書き path test (= 同名 UBO 2 file 同値で PASS) (d) class*/ + cinematic_bd/ 入力で既存 codegen 出力 (= `ubo_metadata.inl` 等) 互換性 verify test、blueprint 入力前提 test 撤廃 |
| 5 | 80 UBO 全件 cold launch validation | 改修後 codegen で `class*/` + `cinematic_bd/` 配下 80 UBO declaration を再生成、SPIR-V ↔ `ubo_metadata.inl` 整合 confirm (= Vulkan validation 0 件 + AYA live verify) = sub-session 5 step 2-batch-0-a 7 UBO 新 binding 反映 + 73 UBO 未改修 binding 反映 |

### §D.7 α-3 全件波及更新範囲 record (= 次 phase で実施、忘却防止)

α-3 (= CMake 改修 + codegen 入力切替) sub-step で同時に **設計 doc + per-UBO doc + source code** 内の blueprint 関連記述を「reference 降格 + codegen 入力対象外」前提に揃える。範囲:

| # | 対象 | 内容 |
|---|---|---|
| 1 | `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` | `BLUEPRINT_DIR` 言及全件、§5 走行 logic / §12 CMake wiring の入力 source 記述を「class*/ + cinematic_bd/」整合に更新、blueprint 言及を「reference 資料」位置付けに修正 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` | blueprint 言及 (= §27 / §932 / §967 等) を「reference 資料」位置付けに維持しつつ、codegen 入力 path 言及を「class*/ + cinematic_bd/」に統一 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | blueprint 言及確認 + Phase 2.α 反映 (= Phase 2 内 codegen 入力 source 修復 sub-phase として記録) |
| 4 | `docs/specs/ayastorm-r41-gl-removal/design/10-open-questions.md` | blueprint 関連 open question あれば closed 化 (= Phase 2.α で確定済) |
| 5 | `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md` + `READINESS.md` + `RELATIONS.md` + `INDEX.md` | blueprint 言及確認 + 「reference 資料」位置付け統一 |
| 6 | `docs/specs/ayastorm-r41-gl-removal/design/ubo/*.md` 80+ 件 per-UBO doc | 各 doc 内 blueprint 言及を「reference 資料」位置付け統一 (= 機械的書換可能、自動化 candidate) |
| 7 | `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md` | §247-249 同名 UBO 複数 file 認識 record を「Phase 2.α 案 Z で main.py 整合 verify logic 追加で構造的検出済」反映 |
| 8 | `indra/cmake/AyaUboCodegen.cmake` | 改修 1 (= §D.6 #1) 本体改修 (= α-3 内) |
| 9 | `scripts/ubo_codegen/main.py` + `glsl_parser.py` + `perfect_hash.py` + `tests/test_main.py` + `tests/test_build_cache.py` | docstring / comment 内 blueprint 言及を「reference 資料 / codegen 入力対象外」位置付けに更新 (= 機能改修は §D.6 #2 + #4) |
| 10 | `aya_r41_blueprints/README.md` 新規追加 | 改修 3 (= §D.6 #3) 本体 README 追加 |

**phase1 archive doc + handoff/archive/ + archive/** = 履歴 doc ゆえ更新対象外 (= 時系列で「phase1 完了時点の状態」として読まれる、誤読リスク小)。

### §D.8 sub-session 5 step 2-batch-0-a 7 commit 整合性

案 Z 採用で sub-session 5 step 2-batch-0-a 7 commit (= class*/ + cinematic_bd/ 14 file の set/binding 書換) は **freeze 維持**、Phase 2.α α-3 完了後の codegen 入力切替 (= class*/ + cinematic_bd/ 入力) で `ubo_metadata.inl` に新 binding 値が自動反映 = sub-session 5 続行点 (= cold launch + AYA live verify) 直接 resume 可能。revert 不要。

---

## §D.9 案 Z 撤回 + 案 Z' 撤回 + 案 X 確定 record (= 2026-06-06 4-5 件目発火、案 Z 全撤回 source of truth)

### §D.9.1 起案契機 = AYA literal 4-5 件目受領 (= 2026-06-06)

§D 案 Z 確定 → improvement 1 (= blueprint dir README 「reference 降格」 commit `862f9cb983`) + improvement 2 (= cmake input 切替 commit `246535626e`) 通過 → improvement 3 codegen 単独走行 verify で **parse error 第 1 階層発覚** = `MAX_JOINTS_PER_MESH_OBJECT` (= AYAstorm C++ runtime `llviewershadermgr.cpp:870` `addPermutation()` 経由 dynamic #define) が actual `class1/avatar/objectSkinV.glsl:49` で unresolved。

AYA literal 4 件目「ミスの上にミスの上にミスの上のミス、わたしじゃ回答出来ない、根治案を出してといってこれです、はい ミス、どうしたらいいですか？って聞かれてもわからない」受領 = 「選択肢 1/2/3 + どうしたらいいですか」を AYA に投げた罪 (= memory `feedback_proactive_risk_management` + `feedback_self_bug_no_defer_option` + `feedback_explanation_lead_with_conclusion` 違反)、memory `feedback_root_cause_no_shortcuts` §11 起案。

案 Z' = 案 Z + C++ runtime emulation 層追加 (= dump file + main.py `--defines-file` + cmake DEPENDS) を §11 自走精査 4 件 (= 設計 doc 全文逐語 + 実装 full trace + structural property + 影響範囲) 適用したつもりで確定、improvement 1.5.a-c commit 通過、improvement 1.5.d 改修途中で codegen 単独走行 verify で **parse error 第 2 階層発覚** = `#version` 不在 + `AYASTORM_CINEMATIC` 等 AYAstorm runtime `loadShaderFile()` prepend chain 全件 emulate scope は dump file 1 つを超過 (= `HAS_DIFFUSE_LOOKUP` + tex0..N stub + GBUFFER_FLAG_* 等)。

AYA literal 5 件目「了解、どうしたらいいのかわたしにはもうわからないんですが、どうしたいんですか？」受領 = parse error 第 2 階層発覚後「案 X 採否ご判断ください」を AYA に投げた罪 (= §11 違反の再発火)、memory `feedback_root_cause_no_shortcuts` §12 起案。

### §D.9.2 blueprint dir 単独走行 evidence = 案 X 真の根治確定

§12 自走精査 sandbox 実証 protocol 適用で blueprint dir 単独走行を試行:

```
python3 scripts/ubo_codegen/main.py \
  --input indra/newview/app_settings/shaders/aya_r41_blueprints \
  --output /tmp/aya_r41_codegen_verify_blueprint \
  --glslang-bin /usr/bin/glslangValidator \
  --spirv-cross-bin /usr/bin/spirv-cross
```

結果: **94 .glsl input(s) discovered → emitted 99 file(s) for 94 block(s) / 386 member(s) in 10903 ms**、**dump file なし**、**parse error 0 件**。

`frame_view_proj.glsl` 確認:
```glsl
#version 450
layout(std140, set = 0, binding = 0) uniform FrameViewProj { ... };
void main() {}
```

⇒ blueprint dir 内 .glsl は **`#version 450` + 自己完結 1 UBO declaration**、AYAstorm runtime prepend chain (= `loadShaderFile()` 経由 `addPermutation` / `AYASTORM_CINEMATIC` / `IS_AMD_CARD` / `HAS_DIFFUSE_LOOKUP` 等) を **一切必要としない最小単位**。

### §D.9.3 設計 doc 04 §2.2 literal 誤り発覚

設計 doc `04-codegen-ubo.md §2.2` literal「Codegen と glslang は **同じ GLSL 入力に対して 2 系統並列の build process**」自体が **実装と矛盾する誤った文言** だった = 設計時の真の想定は **別 GLSL 並列 build process**:

| 系統 | GLSL 入力 | 役割 |
|---|---|---|
| codegen Python tool | `aya_r41_blueprints/` (= 本 dir、self-contained) | build-time に `ubo_metadata.inl` 等 generate |
| AYAstorm shader runtime | `class*/` + `cinematic_bd/` (= actual shader) | viewer 起動時に `loadShaderFile()` prepend chain 適用後 glCompileShader / SPIR-V 化 |

両者は **別 file**、両者間の UBO declaration 整合は **二重 source 同期 protocol** (= main.py `_verify_block_match` 拡張) で build-time check。

AYA 指示 #5 `design/01-overview.md:146` 「85 GLSL UBO blueprint は **discard しない** (parse error 解消の蓄積を温存)」literal の真意 = **codegen 入力 source of truth 保護指示** (= 案 Y/Z/Z' で読み違えた「reference 降格保持」ではない)。「parse error 解消の蓄積」literal 真意 = AYAstorm runtime prepend chain 不要の自己完結 GLSL snapshot 群。

### §D.9.4 案 Y/Z/Z' 撤回 + 案 X 確定 経緯

| 案 | 内容 | 結果 | 撤回根拠 |
|---|---|---|---|
| 案 Y | blueprint 完全廃止 | 撤回 | AYA 指示 #5 違反確定 |
| 案 Z | codegen 入力切替 = `class*/` + `cinematic_bd/` + blueprint reference 降格 | 撤回 | improvement 3 codegen 単独走行で parse error 第 1 階層発覚 (= `MAX_JOINTS_PER_MESH_OBJECT` unresolved) |
| 案 Z' | 案 Z + C++ runtime emulation 層追加 (= dump file + `--defines-file` + cmake DEPENDS) | 撤回 | improvement 3 再走行で parse error 第 2 階層発覚 (= `#version` 不在 + `AYASTORM_CINEMATIC` 等 prepend chain 全件 emulate scope 超過、`HAS_DIFFUSE_LOOKUP` + tex stub 等 emulate 不可) |
| **案 X (確定)** | **blueprint dir = codegen 入力 source of truth、`class*/` + `cinematic_bd/` = runtime compile target、別 GLSL 並列 build process + 二重 source 同期 protocol formal化** | **確定** | (a) blueprint dir 単独走行 = 94 .glsl → 94 UBO emit 成功 + dump file なし + parse error 0 件 evidence、(b) 設計 doc 04 §2.2 literal 誤り訂正 = 別 GLSL 並列 build process 整合、(c) AYA 指示 #5 真意 = source of truth 保護指示 整合、(d) 二重 source 構造の正当な共存 |

### §D.9.5 6 commit revert + 案 X 採用 (= 2026-06-06 commit `df38b7c994`)

| revert 対象 commit | 改修内容 | 案 X での扱い |
|---|---|---|
| `9c3b3f3d72` | improvement 1.5.c main.py + build_cache.py + tests + 08 doc | **revert** (= `--defines-file` 関連全件削除) |
| `a97b3e1b6b` | improvement 1.5.b dump file 起案 | **revert** (= dump file 削除) |
| `9abae83730` | improvement 4 設計 doc 5 件改修 (= C++ runtime emulation 層 § 新規追加 + 案 Z' 反映) | **revert** (= 案 X 確定で設計 doc 5 件は別途改修、§D.9.6 全件波及更新範囲) |
| `bfacb1f50f` | improvement 1.5.a handoff doc 2 件改訂 (= 案 Z' source of truth) | **revert** (= 本 §D.9 で案 X 確定 source of truth 再起案) |
| `246535626e` | improvement 2 cmake input dir 切替 (= `BLUEPRINT_DIR` → `SHADER_SOURCE_DIRS`) | **revert** (= `BLUEPRINT_DIR` 復元) |
| `862f9cb983` | improvement 1 blueprint dir README (= reference 降格明示) | **revert** (= phase B で「codegen 入力 source of truth」literal で全面書換、commit `f95182ded5`) |

revert 後 base state = test 138 件全 PASS + blueprint dir 単独走行 94 UBO emit 成功 = 案 X 動作 evidence 再現済。

### §D.9.6 案 X 改修方針 (= §D.6 案 Z 改修方針を全面置換)

| # | 改修対象 | 改修内容 (= 案 X 確定) |
|---|---|---|
| 1 | `aya_r41_blueprints/README.md` (= 本 dir README) | **新規追加** = 「**codegen 入力 source of truth**」literal で全面書換 (= phase B 完了 commit `f95182ded5`)、§0 位置付け + §1 役割 + §2 二重 source 同期 protocol + §3 Phase 履歴 9 sub-step + §4 参照優先順位 + §5 編集規律 + §A 関連 doc cross-ref |
| 2 | `indra/cmake/AyaUboCodegen.cmake` | `AYA_UBO_CODEGEN_BLUEPRINT_DIR` 復元 (= revert で base state 完了)、追加改修なし (= 案 X = base state 整合) |
| 3 | `scripts/ubo_codegen/main.py` | (a) `_verify_block_match` 拡張 (= phase F) = blueprint と actual の対称的整合 verify、(b) `--input` に blueprint dir + actual shader path 両方受領、(c) `--verify-target-paths` option 新規追加 = blueprint と actual を区別して二重 source 整合 verify、(d) 同名 UBO 複数 file (= blueprint + actual の cross-source pair) 検出 + 整合検証 logic |
| 4 | `scripts/ubo_codegen/tests/test_main.py` | 拡張 test 追加 = blueprint + actual 二重 source 整合 verify test + mismatch detect test (= binding 不一致 / member 不一致) + cinematic_bd 上書き path test 互換性確認、案 Z'+ test (= `RuntimeEmulationDefinesTests`) は revert 済で対象外 |
| 5 | sub-session 5 step 2-batch-0-a 7 commit 整合 = blueprint dir 内 7 UBO 14 file 同期書換 (= phase E) | actual class*/ + cinematic_bd/ 14 file (= sub-session 5 既改修) の新 set/binding を blueprint dir 内 7 UBO 14 file に同期反映 (= 案 Z'+ で「actual = codegen 入力で binding 自動反映」と想定したのを、案 X で「blueprint 側を手動同期 + verify で確認」に切替) |
| 6 | 設計 doc 5 件 (= 04/06a/08/09/10) | (a) 04 §2.2 literal 訂正 = 「同じ GLSL 入力」誤りを「別 GLSL 並列 build process」literal に修正、(b) 04 §4.4 同名 UBO 複数 GLSL 宣言の literal 維持 (= blueprint + actual 二重 source の整合 verify 設計、`_verify_block_match` 拡張対応)、(c) 案 Z'+ 由来 「C++ runtime emulation 層」§ (= 04 §4.5 / 06a §4.5 / 08 §5.0) は revert で削除済、案 X では追加 § 不要 (= base state 整合)、(d) 09 Phase 2.α 案 X 確定反映、(e) 10 open questions 案 X 確定で blueprint 関連 closed 化 + 二重 source 同期関連 open question 棚卸し |
| 7 | per-UBO doc 80+ 件 (= `design/ubo/*.md`) | blueprint 言及確認 + 「**codegen 入力 source of truth**」位置付けで literal 統一 (= 案 Z'+ revert で旧「reference 降格」literal は base state 復元済) + 二重 source 同期 protocol (= actual との対応) cross-ref 追加 |
| 8 | `ayastorm-r41-ubo-current-state-inventory.md` | §247-249 同名 UBO 複数 file 認識 record を「Phase 2.α 案 X で main.py `_verify_block_match` 拡張で blueprint + actual の二重 source 整合 verify」に更新 |
| 9 | 80 UBO 全件 cold launch validation | 改修後 codegen で blueprint dir 入力で `ubo_metadata.inl` 等再生成、SPIR-V ↔ `ubo_metadata.inl` 整合 confirm (= Vulkan validation 0 件 + AYA live verify) + actual class*/ + cinematic_bd/ との二重 source 整合 confirm |

### §D.9.7 案 X 全件波及更新範囲 record (= §D.7 全面置換)

| # | 対象 | 内容 |
|---|---|---|
| 1 | `aya_r41_blueprints/README.md` | **完了** (= phase B commit `f95182ded5`) |
| 2 | Phase 2.α handoff doc 2 件 (= 本 doc + α-3 entry) | **phase C で改修** (= §D.9 本節新規追加 + α-3 entry §3 案 X 用 sub-step 再起案) |
| 3 | 設計 doc 5 件 (= 04/06a/08/09/10) | phase D で改修 (= 04 §2.2 literal 訂正 + 05 §4.4 維持 + 案 Z'+ § revert 確認 + 09 案 X 反映 + 10 open questions 棚卸し) |
| 4 | blueprint dir 内 7 UBO 14 file 同期書換 | phase E で改修 (= sub-session 5 改修 7 UBO の actual 新 set/binding を blueprint 側に同期反映) |
| 5 | `scripts/ubo_codegen/main.py` `_verify_block_match` 拡張 + `--verify-target-paths` option | phase F で改修 (= blueprint + actual 二重 source 整合 verify formal化) |
| 6 | per-UBO doc 80+ 件 (= `design/ubo/*.md`) | phase G で改修 (= blueprint 言及更新 + 二重 source 同期 cross-ref) |
| 7 | source code 5+ file (= scripts/ubo_codegen/*.py) docstring | phase G で改修 |
| 8 | `ayastorm-r41-ubo-current-state-inventory.md` §247-249 record | phase G で改修 |
| 9 | design/ubo/WORK_ORDER.md / READINESS.md / RELATIONS.md / INDEX.md | phase G で改修 (= 二重 source 同期 protocol 反映) |
| 10 | 全件 grep 走査残漏れ 0 件 confirm | phase G 最終で実施 |

### §D.9.8 既保持 commit 整合性 (= revert 対象外)

| commit | 内容 | 案 X での扱い |
|---|---|---|
| `b66ec99f72` | α-2 main.py multi-input + `_verify_block_match` | **保持** (= 案 X phase F で `_verify_block_match` 拡張 baseline) |
| `64122994c1` | Phase 2.α 案 Z 確定反映 doc 3 件 | **保持** (= 案 Z 履歴として残存、本 §D.9 で案 X 確定 record 上書き) |
| `db5cbcbc36` | Phase 2.α 起案 + Phase 2.L0 freeze record | **保持** (= 起案契機 + freeze 記録は案 X でも同) |
| `310d58b556` | α-3 entry handoff 起案 | **保持** (= α-3 entry §3 のみ phase C で再起案) |
| `887ddb5341`〜`e5f57d57ff` | sub-session 5 step 2-batch-0-a 7 commit | **freeze 維持** (= 案 X phase E で blueprint dir 側 14 file 同期書換実施、actual 改修は freeze 維持) |

### §D.9.9 §11 §12 反省 + 5 段落ち防止 protocol 強化

5 段連続同じ穴落ちの根本原因:
- §11 自走精査 checklist B (= 実装 full trace) を subagent 1 結果「`#ifdef`/`#if` の中で parse error 至らず」を盲信 = 私の責任
- §11 自走精査 checklist C (= blueprint / actual structural property) を blueprint dir 内 .glsl 単体構造未確認 = 私の責任
- §12 sandbox 実証 protocol を案確定前に実施せず = 私の責任
- §12 設計 doc literal 誤り疑念を持たず 04 §2.2 literal を盲信 = 私の責任

memory `feedback_root_cause_no_shortcuts` §11 + §12 適用、案 X 進行中も sub-step 完了毎に **sandbox 実証 + 設計 doc literal 独立 verify + subagent 結果は raw evidence のみ信任** 義務化。
