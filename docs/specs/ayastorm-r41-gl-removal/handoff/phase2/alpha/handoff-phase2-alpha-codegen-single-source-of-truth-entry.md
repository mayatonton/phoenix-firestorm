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

### §D.9 「同じ穴に二度落ちた」事例 + 案 Z' 確定 record (= 2026-06-06 4 件目発火)

#### §D.9.1 4 件目発火経緯

案 Z 確定 (= §D.6) を受けて improvement 1 (= blueprint dir README 新規 = commit `862f9cb983`) + improvement 2 (= AyaUboCodegen.cmake `class*/` + `cinematic_bd/` 入力切替 = commit `246535626e`) を完了、improvement 3 (= codegen 単独走行 verify) で **parse error 発覚** = `MAX_JOINTS_PER_MESH_OBJECT` が `class1/avatar/objectSkinV.glsl:49` で unresolved。

原因 = AYAstorm C++ runtime (= `llviewershadermgr.cpp:870`) が `addPermutation()` で **dynamic #define** (= `LLSkinningUtil::getMaxJointCount()` = 110) を inject していて、codegen 単独走行ではそれが解決できない。

AYA literal 4 件目「ミスの上にミスの上にミスの上のミス、わたしじゃ回答出来ない、根治案を出してといってこれです、はい ミス、どうしたらいいですか？って聞かれてもわからない」受領 = parse error 発覚後「選択肢 1/2/3 + どうしたらいいですか」を AYA に投げた = memory `feedback_proactive_risk_management` + `feedback_self_bug_no_defer_option` + `feedback_explanation_lead_with_conclusion` 違反、AYA リスク管理肩代わり拒否 literal。

#### §D.9.2 §11 memory 起案 (= `feedback_root_cause_no_shortcuts` §11)

「同じ穴に二度落ちた」事例として memory 起案、**自走精査網羅性 checklist 4 件義務化**:

| # | checklist 項目 | 内容 |
|---|---|---|
| A | 設計 doc 全文逐語精査 | source of truth 全 literal + 全想定の明示確認 (= literal を見るだけでなく、その想定が何を前提にしているかまで literal で確認) |
| B | 実装側 full trace | AYAstorm 独自実装 (= include resolver / addPermutation / dynamic #define / その他 macro) 全件 enumerate |
| C | blueprint / actual 差分 structural property 棚卸し | hardcode dynamic 値 / include 文 / 独自 syntax / preprocessor pragma 等 |
| D | 影響範囲計測 | 該当 file 数 + UBO 影響範囲 + 構造的整合の証拠付き確認 |

4 件全件 verify が揃わない限り根治案を確定しない。

#### §D.9.3 自走精査 result + 案 Z' 確定根拠

##### §D.9.3.1 AYAstorm shader runtime full trace (= checklist B)

| 項目 | 件数 | 主要 evidence |
|---|---|---|
| `addPermutation()` 全呼出 | 108 件 | `LLGLSLShader::addPermutation()` @ `llglslshader.cpp:1754-1757`、`mDefines[name] = value` + `loadShaderFile()` で prepend |
| `#include` directive | **0 件** | LL は `attachShaderFeatures()` (`llshadermgr.cpp:85-394`) で C++ 側 file 単位 attach、`#include` 使わない方式 |
| dynamic `#define` injection 系統 | 10 系統 | `AYASTORM_CINEMATIC` / `FRAGMENT_SHADER|VERTEX_SHADER` / `GBUFFER_FLAG_*` / `IS_AMD_CARD` / `HAS_DIFFUSE_LOOKUP` + tex0..N / `OLD_SELECT` / `LL_VULKAN_GLSL` / `mDefines` (= addPermutation 値) / `#version` 動的選択 / `[EXTRA_CODE_HERE]` marker |
| その他 AYAstorm 独自操作 | `#extension` 静的記述 18 ヒット / 6 file + `cinematic_bd/` path probe + 文字列 mutation 0 件 |

parse error 直撃 = **identifier 値として配列 size 等に直接埋め込まれる** macro = `MAX_JOINTS_PER_MESH_OBJECT` / `MAX_NODES_PER_GLTF_OBJECT` / `MAX_MATERIALS_PER_GLTF_OBJECT` / `MAX_UBO_VEC4S` / `LIGHT_COUNT` / `REFMAP_LEVEL` / `REF_SAMPLE_COUNT` / `PROBE_FILTER_SAMPLES` / `FXAA_QUALITY__PRESET` / `TERRAIN_PBR_*` / etc。

##### §D.9.3.2 設計 doc 04 + 08 + 06a 全文逐語精査 (= checklist A)

| literal 引用 | 確定 |
|---|---|
| `08-build-codegen-pipeline.md §2.1:72-74` literal 「入力: `app_settings/shaders/class*/{deferred,interface,...}/**.glsl`」 | codegen 入力 = actual shader 確定 (= 候補 B) |
| `08 §2.2` literal 「Codegen と glslang は **同じ GLSL 入力に対して 2 系統並列の build process**」 | codegen + runtime 同一 GLSL 入力 |
| `04 §8:967` literal 「blueprint は **discard せず再利用** ... Codegen 生成 layout に redirect 層が値を流せば実体化」 | blueprint dir は履歴温存、actual shader が source of truth |
| `04 §4.4` literal 「同名 block を複数 GLSL で再宣言 ... 全宣言が **同一 member 構成** であることを build-time check で保証」 | sub-session 5 step 2-batch-0-a 14 file 同期書換は §4.4 literal 整合性要件の必然 |
| `04 / 08 / 06a` 全文に **C++ runtime emulation を扱う § literal 不在** | **設計時構造的見落とし** (= §11 で見落とした真因) |

##### §D.9.3.3 影響範囲計測 (= checklist D)

| 軸 | 計測値 |
|---|---|
| shader 総 file 数 (`class*/` + `cinematic_bd/`) | 246 file |
| 項目 `MAX_JOINTS_PER_MESH_OBJECT` 影響 file | 3 file (`objectSkinV.glsl` / `skinnedVelocityV.glsl` / `skinnedVelocityAlphaV.glsl`) |
| parse error 直撃 file 数 | **約 25-35 file** (= identifier 値として配列 size 等に直接埋め込まれる macro 含有 file) |
| 影響 UBO 数 (= 80 UBO 中) | **約 4-8 UBO** (= `PerDrawUBO_ObjectSkin` / `PerDrawUBO_SkinnedVelocity` 系 / GLTF PBR UBO 系等) |

#### §D.9.4 案 Z' (= 案 Z + C++ runtime emulation 層追加) 確定根拠

設計 doc literal 4 件で **案 Z 方向 (= class*/ + cinematic_bd/ 入力) は正しい**。「同じ穴」の真因は案 Z 方向ではなく、案 Z 実装時の **C++ runtime emulation 層必要性見落とし**。

| 軸 | 案 Z' 整合根拠 |
|---|---|
| AYA 指示 #5 (= design/01:146 「discard しない」) | ✅ blueprint dir 物理保持 (= 案 Z と同) |
| 設計 doc 08:72-74 想定 | ✅ codegen 入力 = `class*/` + `cinematic_bd/` (= 案 Z 方向、improvement 2 commit `246535626e` 保持) |
| 設計 doc 04:967 「blueprint は discard せず再利用」 | ✅ blueprint dir = 履歴温存 (= 案 Z と同、improvement 1 commit `862f9cb983` 保持) |
| 設計 doc 04 §2.2 「同じ GLSL 入力に対して 2 系統並列」 | ✅ codegen + runtime SPIR-V 化が同一 GLSL を入力 |
| **C++ runtime emulation 層 (= 設計時 literal 空白)** | ✅ 新規 improvement 1.5 で codegen Python tool に **C++ 定数 dump file (= `aya_r41_codegen_defines.toml` 等) + main.py `--defines-file` option + glslang -E に `-D<key>=<value>` prepend** を追加 (= 設計時 literal 空白を埋める) |
| memory `feedback_root_cause_no_shortcuts` 整合 | ✅ 案 Z' 単独提示、対症療法案並列なし、選択肢 1/2/3 を AYA に投げない |
| sub-session 5 step 2-batch-0-a 7 commit 整合 | ✅ class*/ + cinematic_bd/ 14 file 改修済 = 案 Z' 採用で codegen 入力切替後 binding 値自動反映、freeze 維持 |

#### §D.9.5 案 Z' 改修方針 (= §D.6 案 Z 改修方針を置換)

| # | 改修対象 | 改修内容 (= 案 Z' 確定) |
|---|---|---|
| 1 | `indra/cmake/AyaUboCodegen.cmake` | **改修 1-A (= 既 commit `246535626e` 保持)**: `BLUEPRINT_DIR` 廃止 → `SHADER_SOURCE_DIRS` 4 path list 化、`GLOB_RECURSE` 連合、`--input` list 化、STATUS message 更新。**改修 1-B (= 新規 improvement 1.5.d)**: dump file path (= `AYA_UBO_CODEGEN_DEFINES_FILE`) 定義 + `add_custom_command DEPENDS` に追加 + `--defines-file` 引数を `_aya_codegen_common_args` に追加 |
| 2 | `scripts/ubo_codegen/main.py` | **改修 2-A (= 既 commit `b66ec99f72` 保持)**: `--input nargs='+'` 多入力対応 + `_verify_block_match` 同名 UBO 複数 file 整合 verify logic。**改修 2-B (= 新規 improvement 1.5.c)**: `--defines-file <path>` option 追加 + dump file 読込 (= toml/json parser) + glslang -E に `-D<key>=<value>` で prepend + 138 test PASS 維持 |
| 3 | `aya_r41_blueprints/` | **reference 降格保持 (= 既 commit `862f9cb983` 保持)**: dir + 全 .glsl file 維持、codegen 入力対象外、内部 `README.md` 反映済 (= 案 Z' で内容補足の必要性は次 sub-step で評価) |
| 4 | **新規 = `scripts/ubo_codegen/aya_r41_codegen_defines.toml` (or .json) 起案 (= improvement 1.5.b)** | AYAstorm C++ 定数群を static dump = `MAX_JOINTS_PER_MESH_OBJECT=110` (= `lljoint.h` 経由) + 影響範囲 4-8 UBO で必要な全 macro 全件 enumerate、CMake DEPENDS 追加で改訂時 reconfigure 自動 trigger |
| 5 | `scripts/ubo_codegen/tests/test_main.py` | (a) `--defines-file` option test 追加 (b) glslang -E への `-D<key>=<value>` prepend test (c) 不在時 fallback test (= dump file 未指定で従来 behavior) (d) class*/ + cinematic_bd/ 入力で 80 UBO codegen 出力整合 test |
| 6 | **新規 = 設計 doc 04 + 08 + 06a に「C++ runtime emulation 層」§ 新規追加 (= improvement 4 拡張)** | literal 空白を埋める (= §11「設計 doc literal 空白を見落とした罪」防止策)、04 §3 / §4 / §5.2.1 / §6 / §11 のいずれかに新 § 追加 + 08 §3.3 / §4.3 / §5.2 に C++ runtime 注入 emulation 層 追加 + 06a §0.1 「addPermutation」の build-time variant 展開の literal 補完 |
| 7 | 80 UBO 全件 cold launch validation | 改修後 codegen で `class*/` + `cinematic_bd/` 配下 80 UBO declaration を再生成、SPIR-V ↔ `ubo_metadata.inl` 整合 confirm (= Vulkan validation 0 件 + AYA live verify) = sub-session 5 step 2-batch-0-a 7 UBO 新 binding 反映 + 73 UBO 未改修 binding 反映 |

#### §D.9.6 案 Z' 全件波及更新範囲 record (= §D.7 拡張)

| # | 対象 | 内容 (= 案 Z' 追加分は **太字**) |
|---|---|---|
| 1 | `design/08-build-codegen-pipeline.md` | `BLUEPRINT_DIR` 言及全件、§5 走行 logic / §12 CMake wiring の入力 source 記述を「class*/ + cinematic_bd/」整合に更新、blueprint 言及を「reference 資料」位置付けに修正 + **§3.3 / §4.3 / §5.2 に C++ runtime emulation 層 § 新規追加** |
| 2 | `design/04-codegen-ubo.md` | blueprint 言及 (= §27 / §932 / §967 等) を「reference 資料」位置付けに維持、codegen 入力 path 言及を「class*/ + cinematic_bd/」に統一 + **§3 / §4 / §5.2.1 / §6 / §11 のいずれかに C++ runtime emulation 層 § 新規追加** |
| 3 | **`design/06a-host-redirect-layer.md`** (= 案 Z' 新規追加) | **§0.1 「`addPermutation` build-time variant 展開」literal を「= codegen Python tool が dump file 経由 emulate」literal で補完**、新 § で「C++ runtime emulation 層」の役割明示 |
| 4 | `design/09-phase-roadmap.md` | blueprint 言及確認 + Phase 2.α 反映 + **案 Z' 確定反映 (= sub-phase α-1.5 = C++ runtime emulation 層追加)** |
| 5 | `design/10-open-questions.md` | blueprint 関連 open question あれば closed 化 + **C++ runtime emulation 層関連 open question 棚卸し** |
| 6 | `design/ubo/WORK_ORDER.md` + `READINESS.md` + `RELATIONS.md` + `INDEX.md` | blueprint 言及確認 + 「reference 資料」位置付け統一 + **影響 4-8 UBO (= dynamic #define 含有 UBO) の dependency entry 追加** |
| 7 | `design/ubo/*.md` 80+ 件 per-UBO doc | 各 doc 内 blueprint 言及を「reference 資料」位置付け統一 + **影響 4-8 UBO (= `PerDrawUBO_ObjectSkin` 等) に C++ runtime emulation 層 dependency 明示** |
| 8 | `ayastorm-r41-ubo-current-state-inventory.md` | §247-249 同名 UBO 複数 file 認識 record を「案 Z で main.py 整合 verify logic 追加」反映 + **C++ runtime emulation 層 dependency record 追加** |
| 9 | `indra/cmake/AyaUboCodegen.cmake` | 改修 1-A (= 既 commit) + **改修 1-B (= dump file DEPENDS + --defines-file 引数追加、improvement 1.5.d)** |
| 10 | `scripts/ubo_codegen/main.py` + `glsl_parser.py` + `perfect_hash.py` + `tests/test_main.py` + `tests/test_build_cache.py` | docstring / comment 内 blueprint 言及を「reference 資料 / codegen 入力対象外」位置付けに更新 (= 機能改修は §D.9.5 #2 + #5) + **C++ runtime emulation 層 docstring 反映** |
| 11 | `aya_r41_blueprints/README.md` | 改修 3 (= 既 commit) + **案 Z' での「C++ runtime emulation 層 dump file 由来 C++ const 整合」literal 補足の必要性評価 (= sub-step 内で判断)** |
| 12 | **`scripts/ubo_codegen/aya_r41_codegen_defines.toml` (or .json)** (= 案 Z' 新規追加) | **新規追加 (= improvement 1.5.b)**、AYAstorm C++ 定数群 static dump |
| 13 | **Phase 2.α handoff doc 2 件** (= 本 doc + α-3 entry) | **§D.9 (本節) 追記 + α-3 entry §3 sub-step 順序を案 Z' 用に再起案 (= improvement 1.5.a)** |

**phase1 archive doc + handoff/archive/ + archive/** = 履歴 doc ゆえ更新対象外 (= 時系列で「phase1 完了時点の状態」として読まれる、誤読リスク小)。

#### §D.9.7 既 commit 整合性 (= revert 不要 record)

| commit | 内容 | 案 Z' での扱い |
|---|---|---|
| `862f9cb983` | improvement 1 = blueprint dir README 新規追加 (= reference 降格明示) | **保持** (= 案 Z' で AYA 指示 #5 + design/04:967 整合維持) |
| `246535626e` | improvement 2 = AyaUboCodegen.cmake `class*/` + `cinematic_bd/` 入力切替 | **保持** (= 案 Z' で design/08:72-74 整合維持、新規追加は dump file DEPENDS + `--defines-file` 引数のみ) |
| `b66ec99f72` | Phase 2.α α-2 = main.py multi-input + `_verify_block_match` 追加 + 7 test 追加 | **保持** (= 案 Z' で integrity check 維持、新規追加は `--defines-file` option のみ) |
| `64122994c1` | Phase 2.α 案 Z 確定 doc 3 件改修 | **§D 追記で更新** (= 本節 §D.9 が案 Z' 確定 source of truth) |
| `db5cbcbc36` | Phase 2.α 起案 + Phase 2.L0 freeze record | **保持** (= 起案契機 + freeze 記録は案 Z' でも同) |
| `887ddb5341`〜`e5f57d57ff` | sub-session 5 step 2-batch-0-a 7 commit (= class*/ + cinematic_bd/ 14 file 改修) | **freeze 維持** (= 案 Z' での codegen 入力切替で binding 値自動反映、resume 可能) |
