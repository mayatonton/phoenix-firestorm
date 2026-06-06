# handoff = r41 Phase 2.L0 sub-session 4 = L0-1.C 実装 step 2 着手 entry

**起案日**: 2026-06-06
**位置付け**: Phase 2.L0 sub-session 4 (= L0-1.C 実装 step 2 = shader 側 GLSL 80 件書換) 着手 entry handoff。本 sub-session で **`indra/` 配下改変開始 phase 解除点** (= memory `feedback_design_phase_no_code_write` 解除、step 1 規律 `indra/` 改変ゼロ 終了)。
**起案契機**: step 1 (= sub-session 3 codegen pipeline 修正起案) Exit AYA literal 受領「OK / 修正すべき資料すべて改修してください / 悲報対応工程 OK / 忘れないように資料反映」record 2026-06-06。
**起案規律**:
- memory `feedback_proactive_handoff` 適用 (= 周回境界 = 自然な session 境界、AYA 指示待たず handoff 起案)
- memory `feedback_handoff_minimal_pre_req_read` 適用 (= 最低限 3 件 + pinpoint)
- memory `feedback_ubo_migration_one_at_a_time` 適用 (= 大塊一括 reject、batch-stage 採用で memory 整合)
- memory `feedback_build_only_verified` 適用 (= 効果未確認 commit を積まない)
- memory `feedback_design_doc_number_literal_verify` 適用 (= 数値 claim 前 grep verify、悲報 2 対応で 2026-06-06 新規追加)

---

## §1. 着手目的 (= step 1 結論受領内容)

### §1.1 AYA literal 受領内容 (= 2026-06-06)

| # | step 1 §7 確認 | AYA literal 回答 | 含意 |
|---|---|---|---|
| 1 | shader 側 GLSL 修正方式 | **(i) 手動編集** | 80 件 shader file 内 `layout(set=N, binding=M)` 宣言を 1 file ずつ手動編集 (= codegen 駆動 rewrite reject、shader_loader override reject) |
| 2 | 80 slot 割当方針 | **Plan-A alphabetical sort** | UBO 名 alphabetical sort で 0..79 に割当 (= deterministic、主観排除) |
| 3 | 修正 batch 単位 | **batch-stage** | shader stage 別 (Compute/Vertex/Fragment) batch、各 batch 内全件 alphabetical sort 順 (= memory `feedback_ubo_migration_one_at_a_time` の「1 UBO ずつ」は新規 UBO 化文脈、本件は layout 値修正のみゆえ batch 化リスク低) |
| 4 | overflow strategy | **将来時再判断** | 80 件 = 80 slot 丁度ゆえ overflow 0 件、将来 UBO 追加時に再判断 |
| 追加 | 悲報対応 + 資料反映 | **OK** | memory `feedback_design_doc_number_literal_verify` 追加済、step 2 sub-step 構造に悲報 3 (= source comment) 組込済 |

### §1.2 sub-session 4 着手目的

**shader 側 GLSL 80 件 `layout(set=N, binding=M)` 書換 + `indra/llglslshader.cpp:95` source comment 訂正**:
- step 2-pre1 = source comment 訂正 (= 悲報 3 対応、design/ubo/ doc 内引用箇所との literal 一致回復)
- step 2-pre2 = shader file × UBO consume mapping 確定 (= 最重要悲報の前準備、同 shader file 内複数 UBO 宣言時の整合崩壊予防)
- step 2-batch-1/2/3 = stage 別 shader 書換 + cold launch validation
- step 2-exit = step 2 出力 doc 起案 + step 3 着手承認

---

## §2. 実装 scope (= `indra/` 改変開始 phase)

### §2.1 大塊一括 default reject (= memory `feedback_ubo_migration_one_at_a_time` 整合)

step 2 実装は **複数 sub-step + batch 分割、各 batch 完了で cold launch 検証挟む**。1 session で完走想定せず、sub-step / batch 境界で handoff 切替。

### §2.2 sub-step 構造 (= step 1 doc §C.1 cross-ref)

| sub-step | 内容 | `indra/` 改変 | cold launch | 想定 session |
|---|---|---|---|---|
| **§2.2.1 step 2-pre1** | `indra/llglslshader.cpp:95` source comment 訂正 = `// ubo_metadata.inl で 88 件最大` → `// ubo_metadata.inl で 80 件 (Phase 2.L0 step 1 grep 確定)` 1 行修正 | 1 file 1 行 | 不要 (= comment のみ、機能影響 0) | sub-session 4 内 |
| **§2.2.2 step 2-pre2** | shader file × UBO consume mapping 確定 = 80 件 UBO × 各 shader file `#ifdef LL_VULKAN_GLSL` block 内宣言 grep + mapping table 起案 | ゼロ (= grep + mapping table doc のみ) | 不要 | sub-session 4 内 or sub-session 5 |
| **§2.2.3 step 2-batch-1 (Compute)** | compute shader (`*C.glsl`) 内 UBO `layout(set=N, binding=M)` 書換 (= 件数 step 2-pre2 後確定) | `indra/newview/app_settings/shaders/` 配下 compute shader file | Vulkan validation 0 件 + AYA live verify | sub-session 6 想定 |
| **§2.2.4 step 2-batch-2 (Vertex)** | vertex shader (`*V.glsl`) 内 UBO 書換 (= 件数 step 2-pre2 後確定、推定 ~20-30 件) | 同 vertex shader file | Vulkan validation 0 件 + AYA live verify | sub-session 7 想定 |
| **§2.2.5 step 2-batch-3 (Fragment)** | fragment shader (`*F.glsl`) 内 UBO 書換 (= 件数 step 2-pre2 後確定、推定 ~40-50 件、件数次第で batch-3a/3b/3c 分割可能性) | 同 fragment shader file | Vulkan validation 0 件 + AYA live verify | sub-session 8-10 想定 |
| **§2.2.6 step 2-exit** | step 2 出力 doc 起案 + step 3 着手承認 | - | - | sub-session 11 想定 |

**1 sub-step 1 session 想定** = sub-session 4-11 想定 (= step 1 doc §C 想定 + batch 分割可能性)。

### §2.3 着手前提 (= 悲報対応持越含む)

step 1 で確定した悲報のうち、step 2 着手前に解消すべき項目:

| 悲報 | 対応 | 完了状態 |
|---|---|---|
| 1 = 全 3 set 80 件不整合 (最重要) | step 2-pre2 + batch-1/2/3 で対応 | step 2 着手で対応中 |
| 2 = 設計 doc 起案時数値確認不足の伝播 | memory `feedback_design_doc_number_literal_verify` 追加 | ✅ 完了 2026-06-06 |
| 3 = `indra/llglslshader.cpp:95` source comment 訂正持越 | step 2-pre1 で先行訂正 | step 2 着手で対応 |

---

## §3. 必読 doc (= memory `feedback_handoff_minimal_pre_req_read` 適用、最低限 3 件 + pinpoint)

### §3.1 最低限 3 件

1. **本 handoff doc** = sub-session 4 着手 entry、step 1 結論 + 悲報対応反映
2. **`handoff/phase2/audit/handoff-phase2-l0-1-C-step1-codegen-plan.md`** = step 1 出力 doc、§3 80 件 grep 結果 + §4 割当 plan + §5 確定事項 + §C.1 sub-step 構造
3. **`handoff/phase2/audit/handoff-phase2-l0-1-B-dispatch-trace.md`** = sub-session 2 出力 doc、§3 5 pilot UBO dispatch logic 実装位置 + §5 protocol 整合判定 (= 改修済 80 件版)

### §3.2 必要時 pinpoint Read 候補

- **`build-linux-x86_64/codegen/ubo/ubo_metadata.inl`** = literal 80 件 PerProgram cluster set/binding 値 (= step 2-pre2 mapping 作成時)
- **`indra/llrender/llglslshader.cpp:95`** = source comment 訂正対象 1 行 (= step 2-pre1)
- **`indra/llrender/llvkloader.cpp:858-868`** = V3A_*_BINDINGS literal (= 目的地 set=1a/1b 容量確認)
- **`indra/llrender/llvkloader.cpp:5348-5385`** = `registerProgramUbo` 内 subset 経路 dispatch (= subset=0/1 active 確認、PC-7α' 通電済)
- **`indra/newview/app_settings/shaders/class*/`** = shader file 配下 (= batch-1/2/3 対象 file 群、step 2-pre2 mapping 作成時に rglob 探索)
- **`design/ubo/<UBO_NAME>.md`** = 各 UBO 別 design doc (= shader file 内宣言位置 cross-ref)

### §3.3 memory pinpoint

- `project_r41_phase1b_vulkan_host_gate` = `mUseUBO` runtime flag、`LL_VULKAN_GLSL` C++ 不使用、GLSL block 内のみ使用
- `project_r41_phase2_4_principles` = 4 原則 + Phase 2.L0 Exit 条件 + 視覚 regression ゼロ
- `project_r41_design_principles` = 2 大設計原則 (call site API 温存 + Core 分散)
- `feedback_admit_unknown` / `feedback_doubt_self_first` / `feedback_design_doc_number_literal_verify` (= 2026-06-06 新規)
- `feedback_ubo_migration_one_at_a_time` / `feedback_proactive_risk_management`
- `feedback_build_only_verified` / `feedback_self_verify_before_handoff`
- `feedback_release_flow` (= push / PR は AYA、Claude はローカル commit まで)
- `feedback_no_claude_coauthor` (= commit message Claude 共著行禁止)

---

## §4. sub-session 4 Exit 条件 (= 本 entry handoff §2.2 step 2-pre1 + pre2 完了基準)

| # | Exit 項目 | 判定基準 |
|---|---|---|
| 1 | step 2-pre1 完了 | `indra/llglslshader.cpp:95` source comment 1 行訂正 commit、git diff で confirm |
| 2 | step 2-pre2 完了 | shader file × UBO consume mapping table 起案 (= 出力 doc `handoff-phase2-l0-1-C-step2-pre2-mapping.md`)、80 件 UBO × 各 shader file 内宣言位置 cross-ref |
| 3 | step 2-pre2 mapping から batch-1 件数確定 | compute / vertex / fragment 別件数明示 (= batch-1/2/3 着手見積もり) |
| 4 | AYA literal 確認受領 | step 2-batch-1 着手承認 or 設計再起案指示 |

---

## §5. 手戻り protocol (= step 1 entry handoff §5 cross-ref)

- **step 2 内手戻り** = batch 内 cold launch reject (= Vulkan validation error) → 該当 batch 内 1 UBO ずつ細分化、再 cold launch
- **step 2 → step 1 戻り** = 80 slot 割当 plan 自体破綻 (= 例: shader stage 別件数偏在で Plan-A alphabetical が batch 単位整合 NG) → step 1 doc §4 割当方針再起案
- **step 2 → sub-session 2 戻り** = protocol-C 採用案変更 (= host dispatch logic 不整合露見) → sub-session 2 doc §6 確認 1 再起案
- **step 2 → sub-session 1 戻り** = L0-1 protocol 自体再考 → sub-session 1 戻り

手戻り発生は **失敗ではなく cycle の正常動作** (= memory `feedback_falsification_as_progress`)。

---

## §6. 起案規律 (= sub-session 4 内維持)

- **`indra/` 改変開始 phase** = step 2-pre1 から `indra/` 配下改変開始、design-phase 規律解除
- **大塊一括 default reject** = sub-step / batch 境界で必ず cold launch 検証挟む、複数 sub-step 1 session で完走想定しない
- **`mUseUBO` runtime flag default OFF 維持** (= 原則 4 §4.4 O-2)
- **`#ifdef LL_VULKAN_GLSL` C++ 不使用** (= 原則 4 §4.4 O-3、memory `project_r41_phase1b_vulkan_host_gate`)
- **視覚 regression ゼロ死守** (= 原則 4 §5.4、batch 別 AYA live verify 必須)
- **3 OS 同一実装** (= 原則 OS-1〜OS-10、shader file は 3 OS 共通ゆえ自動整合)
- **数値 literal 確認規律** (= memory `feedback_design_doc_number_literal_verify` 2026-06-06 新規追加適用、shader file 件数 / UBO 件数 / binding 値等を doc に書く前に grep / wc で literal 確定)
- **推奨案 OK 自走承認継続、ただし AYA literal 確認 candidate 省略しない**

---

## §7. AYA 確認 (= 初手)

**最初の AYA 確認**: 「上記 step 2 entry handoff 確認、Phase 2.L0 sub-session 4 = L0-1.C 実装 step 2 = `indra/` 改変開始 phase で着手 OK か?」

**step 2-pre1 着手内容** (= AYA literal「OK」受領後):
- `indra/llglslshader.cpp:95` source comment 1 行訂正 (= `// ubo_metadata.inl で 88 件最大` → `// ubo_metadata.inl で 80 件 (Phase 2.L0 step 1 grep 確定)`)
- cold launch 不要 (= comment のみ)
- 1 commit (= `fix(indra/llrender): r41 Phase 2.L0 step 2-pre1 source comment 訂正` 想定)

**step 2-pre2 着手内容** (= step 2-pre1 完了 + AYA literal「OK」受領後):
- 80 件 UBO × 各 shader file `#ifdef LL_VULKAN_GLSL` block 内宣言 grep + mapping table 起案
- 出力 doc = `handoff-phase2-l0-1-C-step2-pre2-mapping.md`
- batch-1/2/3 stage 別件数明示

---

## §A. 関連 commit + doc

| 種別 | 内容 |
|---|---|
| 関連 doc (本 sub-session 入口) | `handoff/phase2/audit/handoff-phase2-l0-1-C-step1-codegen-plan.md` (= step 1 出力、commit `8e019bd972`、§5 確定事項 + §C.1 sub-step 構造) |
| 関連 doc (step 1 entry) | `handoff/phase2/audit/handoff-phase2-l0-1-C-entry.md` (= sub-session 3 entry、commit `0af8ac4bdb`) |
| 関連 doc (sub-session 2 出力) | `handoff/phase2/audit/handoff-phase2-l0-1-B-dispatch-trace.md` (= commit `0af8ac4bdb` 起案 + commit `8e019bd972` 改修) |
| 関連 doc (sub-session 1 出力) | `handoff/phase2/audit/handoff-phase2-l0-uncertainty-audit.md` (= commit `cecb55ceb1` 起案 + commit `8e019bd972` 改修) |
| 関連 doc (設計) | `design/ubo/WORK_ORDER.md` §2.1 (= 80 件 PerProgram cluster 仕様確定、commit `8e019bd972` 改修済) |
| 関連 source | `indra/llrender/llglslshader.cpp:95` (= step 2-pre1 訂正対象) + `:2143-2150` (= forwardToUboUpload PerProgram case) |
| 関連 source | `indra/llrender/llvkloader.cpp:858-868` V3A_*_BINDINGS + `:2799-2882` createV3aDescriptorSetLayouts + `:5348-5385` subset 経路 dispatch |
| 関連 source | `indra/newview/app_settings/shaders/class*/` 配下 shader file (= step 2-pre2 mapping 対象、batch-1/2/3 改変対象) |
| 関連 source | `build-linux-x86_64/codegen/ubo/ubo_metadata.inl:25-120` (= literal 94 件 g_block_metadata) |
| 関連 memory | (本 doc §3.3 cross-ref) |
| 本 doc | sub-session 4 着手 entry handoff、commit 候補 (= step 1 doc §C 更新 + 本 doc + memory 追加バンドル、AYA literal 指示後のみ) |

---

## §B. context 引き継ぎ (= memory `feedback_proactive_handoff` 適用)

**step 1 完了 (commit `8e019bd972`) = 自然な session 境界**。次 session で **/clear → 本 handoff doc + 必読 3 件 cold read → step 2-pre1 着手** を推奨。

**本 session 引継ぎ事項**:
- step 1 doc §C 更新 + 本 doc + memory `feedback_design_doc_number_literal_verify` 追加の 3 件 (= AYA literal 指示後 commit)
- push / PR は AYA 側 (= memory `feedback_release_flow`)
- 次 session 初手 AYA 確認 = §7 内容
- 最重要悲報 (= 全 3 set 80 件不整合) 対応 = step 2-pre2 mapping + batch-1/2/3 で全件対応する protocol を堅持
