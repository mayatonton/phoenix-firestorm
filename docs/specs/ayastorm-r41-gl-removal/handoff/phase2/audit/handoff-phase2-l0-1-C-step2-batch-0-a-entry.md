# handoff = r41 Phase 2.L0 sub-session 5 = L0-1.C 実装 step 2-batch-0-a = multi-file UBO 7 件着手 entry

**起案日**: 2026-06-06
**位置付け**: Phase 2.L0 sub-session 5 (= L0-1.C 実装 step 2-batch-0-a = multi-file UBO 7 件 = MaterialUBO 除く小規模、14 file 改変) 着手 entry handoff。本 sub-session で **shader `layout(set=N, binding=M)` 実書換開始 phase**。
**起案契機**: sub-session 4 Exit + AYA literal 2026-06-06「2 i = 本 session 内で next entry handoff 起案」受領。
**起案規律**:
- memory `feedback_proactive_handoff` 適用 (= 周回境界 = sub-session 4 Exit、AYA literal 指示後 handoff 起案)
- memory `feedback_handoff_minimal_pre_req_read` 適用 (= 最低限 3 件 + pinpoint)
- memory `feedback_ubo_migration_one_at_a_time` 適用 (= 1 UBO 1 commit、AYA literal「b-1」確定)
- memory `feedback_build_only_verified` 適用 (= cold launch 検証で効果 verify)
- memory `feedback_design_doc_number_literal_verify` 適用 (= 数値 + 構造属性 verify、適用範囲拡張済)

---

## §1. sub-session 5 着手目的

step 2-batch-0-a 着手 = **multi-file UBO 7 件 (= MaterialUBO 除く小規模、計 14 file)** の `layout(set=N, binding=M)` 値を alphabetical sort 順に **1 UBO 1 commit で全 declaration file 同期書換** (= AYA literal「b-1」採用、mapping doc §4.4)。

本 sub-session 5 で **multi-file UBO 群 小規模 7 件** を完了、次 sub-session 6 で **MaterialUBO 49 file 単独 (= step 2-batch-0-b)** に挑む order (= AYA literal「f-1」MaterialUBO 最後単独採用)。

---

## §2. 実装 scope (= step 2-batch-0-a)

### §2.1 改変対象 = multi-file UBO 7 件 × 14 file

mapping doc §2.3 multi-file UBO 8 件のうち **MaterialUBO (slot=20、49 file) を除く 7 件**:

| # | UBO | slot | 現状 set:binding | 新規 set:binding | subset | 改変 file 2 件 |
|---|---|---|---|---|---|---|
| 1 | **CloudsVParamUBO_Legacy** | 8 | 3:3 | **1:8** | 0 (set=1a) | `class1/deferred/cloudsF.glsl` + `class1/deferred/cloudsV.glsl` |
| 2 | **PerProgramUBO_GammaCorrect** | 39 | 2:2 | **1:39** | 0 (set=1a 最終) | `class1/deferred/postDeferredGammaCorrect.glsl` + `class1/deferred/postDeferredTonemap.glsl` |
| 3 | **PerProgramUBO_PointLightV** | 44 | 2:5 | **1:44** | 1 (set=1b) | `class3/deferred/pointLightV.glsl` + `class3/deferred/spotLightF.glsl` |
| 4 | **PerProgramUBO_PostDeferredF** | 45 | 2:20 | **1:45** | 1 (set=1b) | `class1/deferred/postDeferredF.glsl` + `class1/deferred/postDeferredHQDoFF.glsl` |
| 5 | **PerProgramUBO_WaterHazeV** | 55 | 2:15 | **1:55** | 1 (set=1b) | `class3/deferred/waterHazeF.glsl` + `class3/deferred/waterHazeV.glsl` |
| 6 | **ShadowUtilParamUBO_Legacy** | 63 | 3:7 | **1:63** | 1 (set=1b) | `cinematic_bd/class1/deferred/shadowUtil.glsl` + `class1/deferred/shadowUtil.glsl` (= cinematic_bd 上書き path) |
| 7 | **WaterVParamUBO_Legacy** | 79 | 3:60 | **1:79** | 1 (set=1b 最終) | `class1/environment/waterV.glsl` + `class3/environment/waterF.glsl` |
| 合計 | 7 UBO | | | | | **14 file** |

### §2.2 着手順序 = alphabetical sort 順 (= slot 8 → 39 → 44 → 45 → 55 → 63 → 79)

mapping doc §5.1 alphabetical sort 80 slot 割当 literal 整合。各 UBO 完了後に次 UBO に進む。

### §2.3 改変内容 = 各 file 内 `layout(set=N, binding=M, std140) uniform <UBO> {` 1 行修正

各 file で **`set=N, binding=M`** 値を新規 set/binding に書換 (= UBO 名 + std140 + block 中身は不変)。

**例**: CloudsVParamUBO_Legacy 改修:
```glsl
// 旧 (= step 1 §3.2 現状)
layout(set=3, binding=3, std140) uniform CloudsVParamUBO_Legacy { ... };

// 新 (= alphabetical sort slot 8)
layout(set=1, binding=8, std140) uniform CloudsVParamUBO_Legacy { ... };
```

各 file 内 declaration 1 箇所 (= `#ifdef LL_VULKAN_GLSL` block 内) を Edit、UBO 名 + 内部 member は不変。

### §2.4 commit 単位 = 1 UBO 1 commit (= AYA literal「b-1」確定)

- 1 UBO の **全 declaration file (2 file)** を **同期書換** + **1 commit**
- 7 UBO = **7 commit** 想定
- 各 commit 内で同 UBO の 2 file 同期 (= SPIR-V binary 内 binding 値整合担保)

### §2.5 cold launch 頻度 (= AYA literal 確認 candidate g)

| candidate | 内容 | pros | cons |
|---|---|---|---|
| **g-1** | 1 UBO 1 commit + 1 cold launch (= 7 commit 7 cold launch、慎重) | 各 UBO 単独 verify、failure isolation 完璧、root cause 特定容易 | cold launch 7 回 = AYA 工数 (= ~35 分以上 × 7 = 推定 30-60 分 × 7) |
| **g-2** | 1 UBO 1 commit + B0-a 全 7 commit 完了で 1 cold launch (= 7 commit 1 cold launch、効率) | AYA 工数 1 サイクル、commit 単位明確 | failure 時 7 UBO のうち失敗箇所特定要 (= 各 commit revert で bisect 可能) |

**Claude 推奨**: **g-2** = 1 UBO 1 commit + B0-a 全 7 commit 完了で 1 cold launch。

理由:
- memory `feedback_ubo_migration_one_at_a_time` の「cold launch 検証挟む」は新規 UBO 化 + host C++ redirect 層整備の文脈、本件は **既通電 UBO の layout 値修正のみ** (= 動作 logic 無改変、register/write 経路無改変)
- step 1 §4.4 で AYA literal「batch-stage」採用案 (= 元 §4.4 候補 3) と整合、batch 単位 cold launch protocol
- failure 時の bisect = `git revert` で各 UBO commit 単独 rollback 可能 (= isolation 担保)
- AYA 工数最小化 (= cold launch 1 回で 7 UBO 検証完了)

ただし AYA literal「g-1」採用時は 1 UBO 1 cold launch 慎重 protocol で実行 (= 失敗 1 件即座捕捉)。

**AYA literal 確認要請**: g-1 / g-2 のどちらを採用?

---

## §3. 必読 doc (= memory `feedback_handoff_minimal_pre_req_read` 適用)

### §3.1 最低限 3 件

1. **本 handoff doc** = sub-session 5 着手 entry、batch-0-a scope + 7 UBO 14 file literal + cold launch protocol
2. **`handoff/phase2/audit/handoff-phase2-l0-1-C-step2-pre2-mapping.md`** = step 2-pre2 出力、§4.1 5 batch 構造 + §5.1 80 slot literal 全件 + §4.5 step 3 持越 record
3. **`handoff/phase2/audit/handoff-phase2-l0-1-C-step2-entry.md`** = sub-session 4 entry、§2.2 sub-step 構造 (= step 2-pre2 後 5 batch 構造再策定済)

### §3.2 必要時 pinpoint Read 候補

- **`indra/newview/app_settings/shaders/class*/<対象 file>`** = 各 UBO 改修対象 file (= §2.1 14 file)、Edit 直前 grep で現状 binding 値 verify (= AYA literal「a-1〜f-1」採用ゆえ前提崩壊リスク低、ただし `feedback_doubt_self_first` 適用で必須)
- **`build-linux-x86_64/codegen/ubo/ubo_metadata.inl:25-120`** = literal 80 件 PerProgram cluster set/binding 値 (= Edit 後 codegen 再生成時の expect 値)
- **`indra/llrender/llvkloader.cpp:858-868`** = V3A_*_BINDINGS literal (= 目的地 set=1a/1b 容量 verify)
- **`indra/llrender/llvkloader.cpp:5348-5385`** = `registerProgramUbo` 内 subset 経路 dispatch (= subset=0/1 active 確認、PC-7α' 通電済)

### §3.3 memory pinpoint

- `project_r41_phase1b_vulkan_host_gate` = `mUseUBO` runtime flag、`LL_VULKAN_GLSL` C++ 不使用
- `project_r41_phase2_4_principles` = 4 原則 + Phase 2.L0 Exit
- `project_r41_design_principles` = 2 大設計原則
- `feedback_ubo_migration_one_at_a_time` = 1 UBO 1 commit (= 本 sub-session 5 = 7 commit)
- `feedback_design_doc_number_literal_verify` = 数値 + 構造属性 verify (= 2026-06-06 適用範囲拡張済)
- `feedback_build_only_verified` = cold launch で effect verify
- `feedback_admit_unknown` / `feedback_doubt_self_first` = Edit 直前 grep verify (= 前提崩壊リスク回避)
- `feedback_proactive_risk_management` = AYA リスク管理肩代わり防止
- `feedback_no_scope_shrink` = 7 UBO 全件改変は不変、batch 切り方の調整のみ
- `feedback_release_flow` = push / PR は AYA 側
- `feedback_no_claude_coauthor` = commit message Claude 共著行禁止

---

## §4. sub-session 5 Exit 条件

| # | Exit 項目 | 判定基準 |
|---|---|---|
| 1 | 7 UBO × 14 file shader `layout(set=N, binding=M)` 書換完了 | 7 commit (= 1 UBO 1 commit) git log + git diff で confirm |
| 2 | codegen 再生成 (= `build-linux-x86_64/codegen/ubo/ubo_metadata.inl` 更新) | `set/binding` 値が新規 alphabetical sort 値と整合、CHD perfect hash regenerate 完了 |
| 3 | cold launch (= AYA literal「g-1」 or 「g-2」採用に応じて) Vulkan validation 0 件 + AYA live verify | (g-1) 各 UBO commit 後 / (g-2) 全 7 commit 後 |
| 4 | 視覚 regression 0 件 (= AYA live verify) | 各 UBO 関連 shader 描画 (= 雲 / gamma correct / point light / postDeferred / water haze / shadow / water) 視覚比較で regression なし |
| 5 | sub-session 6 (= step 2-batch-0-b MaterialUBO 49 file 単独) 着手承認 | AYA literal |

---

## §5. 手戻り protocol

- **sub-session 5 内手戻り** = 1 UBO 単独 cold launch reject (= Vulkan validation error) → 該当 UBO commit revert + 設計再 check (= mapping doc §5.1 slot 値再確認 + 該当 UBO の 2 file 同期書換確認)
- **sub-session 5 → step 2-pre2 戻り** = batch 戦略破綻 (= 例: shader compile pipeline 仕様の制約発覚) → mapping doc §4.1 5 batch 構造再策定
- **sub-session 5 → step 1 戻り** = alphabetical sort plan 自体破綻 → step 1 doc §4.3 Plan-A 再考
- **sub-session 5 → sub-session 2 戻り** = protocol-C 採用案変更 → sub-session 2 doc §6 確認 1 再起案

手戻りは **失敗ではなく cycle の正常動作** (= memory `feedback_falsification_as_progress`)。

---

## §6. 起案規律 (= sub-session 5 内維持)

- **1 UBO 1 commit** (= AYA literal「b-1」確定、mapping doc §4.4 record、memory `feedback_ubo_migration_one_at_a_time`)
- **同 UBO 2 file 同期書換** (= 1 commit 内、SPIR-V binary binding 整合担保)
- **alphabetical sort 順** (= slot 8 → 39 → 44 → 45 → 55 → 63 → 79、mapping doc §5.1)
- **`mUseUBO` runtime flag default OFF 維持** (= 原則 4 §4.4 O-2)
- **`#ifdef LL_VULKAN_GLSL` C++ 不使用** (= 原則 4 §4.4 O-3)
- **視覚 regression ゼロ死守** (= 原則 4 §5.4、AYA live verify 必須)
- **3 OS 同一実装** (= 原則 OS-1〜OS-10、shader file は 3 OS 共通ゆえ自動整合)
- **Edit 直前 grep verify** (= memory `feedback_doubt_self_first`、前提崩壊リスク回避、現状 binding 値 confirm 後 Edit)
- **cold launch protocol** = AYA literal「g-1」or「g-2」採用に応じて

---

## §7. AYA literal 確認 (= 初手)

### §7.1 確認 candidate 1 件 (= cold launch 頻度)

**確認要請**:
- **g-1 / g-2 のどちら採用?** (= §2.5 table)
  - g-1 = 1 UBO 1 commit + 1 cold launch (= 7 commit 7 cold launch、慎重)
  - g-2 = 1 UBO 1 commit + B0-a 全 7 commit 完了で 1 cold launch (= 7 commit 1 cold launch、効率)
- **Claude 推奨**: **g-2** (= 既通電 UBO の layout 値修正のみ、failure 時 bisect 可能、AYA 工数最小化)

### §7.2 sub-session 5 着手承認

AYA literal「g-1 / g-2」回答受領 + 着手承認 → sub-session 5 step 2-batch-0-a 着手:
1. 1 UBO 目 = CloudsVParamUBO_Legacy (= slot 8、2 file `cloudsF.glsl` + `cloudsV.glsl`)
2. 各 file 内 `layout(set=3, binding=3, std140) uniform CloudsVParamUBO_Legacy {` → `layout(set=1, binding=8, std140) uniform CloudsVParamUBO_Legacy {` Edit
3. git diff confirm
4. commit (= AYA literal 指示後のみ)
5. (g-1 採用時) cold launch + AYA live verify → 次 UBO
6. (g-2 採用時) 直接次 UBO に進む、7 UBO 全 commit 完了で 1 cold launch

---

## §A. 関連 commit + doc

| 種別 | 内容 |
|---|---|
| 関連 commit | `d89cfb684b` (= step 2-pre2 mapping doc + entry handoff §2.2 訂正) |
| 関連 commit | `4ed9c61095` (= step 2-pre1 source comment 訂正) |
| 関連 doc (本 entry の入口) | `handoff/phase2/audit/handoff-phase2-l0-1-C-step2-pre2-mapping.md` (= commit `d89cfb684b`、§4.1 5 batch + §5.1 80 slot literal + §4.5 step 3 持越) |
| 関連 doc (sub-session 4 entry) | `handoff/phase2/audit/handoff-phase2-l0-1-C-step2-entry.md` (= 改修済 commit `d89cfb684b`、§2.2 5 batch 構造再策定済) |
| 関連 doc (step 1 出力) | `handoff/phase2/audit/handoff-phase2-l0-1-C-step1-codegen-plan.md` (= commit `8e019bd972`) |
| 関連 source | `indra/newview/app_settings/shaders/class1/deferred/cloudsF.glsl` + `cloudsV.glsl` (= UBO 1 = CloudsVParamUBO_Legacy) |
| 関連 source | `indra/newview/app_settings/shaders/class1/deferred/postDeferredGammaCorrect.glsl` + `postDeferredTonemap.glsl` (= UBO 2 = PerProgramUBO_GammaCorrect) |
| 関連 source | `indra/newview/app_settings/shaders/class3/deferred/pointLightV.glsl` + `spotLightF.glsl` (= UBO 3 = PerProgramUBO_PointLightV) |
| 関連 source | `indra/newview/app_settings/shaders/class1/deferred/postDeferredF.glsl` + `postDeferredHQDoFF.glsl` (= UBO 4 = PerProgramUBO_PostDeferredF) |
| 関連 source | `indra/newview/app_settings/shaders/class3/deferred/waterHazeF.glsl` + `waterHazeV.glsl` (= UBO 5 = PerProgramUBO_WaterHazeV) |
| 関連 source | `indra/newview/app_settings/shaders/cinematic_bd/class1/deferred/shadowUtil.glsl` + `class1/deferred/shadowUtil.glsl` (= UBO 6 = ShadowUtilParamUBO_Legacy、cinematic_bd 上書き path 同期要) |
| 関連 source | `indra/newview/app_settings/shaders/class1/environment/waterV.glsl` + `class3/environment/waterF.glsl` (= UBO 7 = WaterVParamUBO_Legacy) |
| 関連 source | `indra/llrender/llvkloader.cpp:858-868` V3A_*_BINDINGS + `:5348-5385` `registerProgramUbo` subset dispatch |
| 関連 source | `build-linux-x86_64/codegen/ubo/ubo_metadata.inl:25-120` (= 80 件 g_block_metadata、Edit 後 codegen 再生成で更新) |
| 関連 memory | (本 doc §3.3 cross-ref) |
| 本 doc | sub-session 5 着手 entry handoff、commit 候補 (= AYA literal 指示後のみ) |

---

## §B. context 引き継ぎ (= memory `feedback_proactive_handoff` 適用)

**sub-session 4 完了 (= commit `d89cfb684b`) = 自然な session 境界**。次 session で **/clear → 本 handoff doc + 必読 3 件 cold read → sub-session 5 step 2-batch-0-a 着手** を推奨。

**本 session 引継ぎ事項**:
- 本 handoff doc commit 候補 (= AYA literal 指示後、本 commit が sub-session 4 Exit 完了 commit)
- push / PR は AYA 側 (= memory `feedback_release_flow`)
- 次 session 初手 AYA 確認 = §7.1 g-1 / g-2 採用判断 + §7.2 sub-session 5 着手承認
- multi-file UBO 7 件 (= B0-a) 完了後、sub-session 6 で **MaterialUBO 49 file 単独 (= B0-b)** に進む (= AYA literal「f-1」採用、MaterialUBO 最後単独 protocol)
- B0-b (= MaterialUBO) は 49 file 1 commit 内同期書換、書換後 grep verify (= `grep -c "set=1, binding=20" 49 files == 49`) で漏れ検出可能
- step 3 持越 record = mapping doc §4.5 `cinematic_bd/` 影響評価項目を step 3 で `design/ubo/WORK_ORDER.md` 等に追加

---

## §C. Phase 2.L0 freeze status (= 2026-06-06 追記)

**本 sub-session 5 step 2-batch-0-a 7 commit (= `887ddb5341` CloudsVParamUBO_Legacy / `e539b384ed` PerProgramUBO_GammaCorrect / `f91cda6677` PerProgramUBO_PointLightV / `d4cabb8cfc` PerProgramUBO_PostDeferredF / `5d21f1af9c` PerProgramUBO_WaterHazeV / `c9620edcc7` ShadowUtilParamUBO_Legacy / `e5f57d57ff` WaterVParamUBO_Legacy) 完了時点で Phase 2.L0 freeze**。

### §C.1 freeze 原因

codegen 入力 source の **二重 source 構造発覚** (= `indra/cmake/AyaUboCodegen.cmake:56-86` で `AYA_UBO_CODEGEN_BLUEPRINT_DIR = aya_r41_blueprints/` を codegen 入力に固定、`class*/` + `cinematic_bd/` 配下 GLSL は SPIR-V binary 入力、両者の binding 値同期断裂で Vulkan validation error 高確率)。本 sub-session 5 7 commit は `class*/` + `cinematic_bd/` 側 14 file 書換完了、blueprint 側 7 UBO は旧 binding 残存 = mismatch。

**真の原因 = 設計乖離確定** (= 2026-06-06 設計 doc 精査結果、Phase 2.α entry handoff §D 参照): **design/08-build-codegen-pipeline.md:72-74/96 で codegen 入力 = `app_settings/shaders/class*/{deferred,interface,...}/**.glsl` literal 想定**、現状 AyaUboCodegen.cmake の `aya_r41_blueprints/` 固定は **設計乖離** = 二重 source 構造はこの乖離が origin。修復方針 = codegen 入力を class*/ + cinematic_bd/ に切替 (= 設計時想定整合復元)。

### §C.2 対応 = Phase 2.α 独立起案

Phase 2.α = UBO codegen single source of truth 化 を独立 phase で起案、`class*/` + `cinematic_bd/` 配下 GLSL を codegen 入力に統一 (= **設計 doc 08:72-74/96 想定整合復元**)、**blueprint dir reference 降格保持** (= 案 Z 確定 2026-06-06、AYA 指示 #5「85 GLSL UBO blueprint は discard しない」literal 整合 = design/01-overview.md:146、設計時参考資料 / Phase 1 履歴として保持、Phase 2.α entry handoff §D 参照)、二重 source 構造解消。

**前案 (= 案 Y blueprint 完全廃止) 撤回 record** (= 2026-06-06): Phase 2.α entry handoff §7.1 で Claude 推奨案として完全廃止 (= α-blueprint-1) 提示、AYA literal 3 連続受領 (= 「腹立たしい」「波及する資料も更新」「原因元資料の精査 / 解決方法は本当にこれで正しいのですか？」) で設計 doc 精査着手、AYA 指示 #5 違反 + 設計乖離見落し確定で案 Z (= 設計 doc 整合修復) に切替。

**Phase 2.α entry handoff doc**: `docs/specs/ayastorm-r41-gl-removal/handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md`

### §C.3 freeze 中 protocol

- 本 sub-session 5 entry handoff §2.1 改修対象 14 file の **追加改変禁止** (= 7 commit 維持、cold launch せず Phase 2.α 完了待ち)
- Phase 2.L0 sub-session 6 以降 (= step 2-batch-0-b MaterialUBO / B2 Vertex / B3 Fragment / B4 util/lib) も **freeze 中着手禁止**
- Phase 2.α 完了後 = 本 sub-session 5 続行点 (= codegen 再生成 + cold launch + AYA live verify) 直接 resume
- Phase 2.α 内手戻り (= 改修方針破綻時) は本 sub-session 5 の 7 commit revert 必要性判断対象 (= Phase 2.α entry handoff §5 手戻り protocol cross-ref)

### §C.4 AYA literal record (= 2026-06-06)

**1 件目発火 (= 2 件目 memory 起案前、Phase 2.α 起案契機)**:
- 「ちょっと毎回根本解決をさけて適当に今だけしのいで先に進もうとするのをやめてもらわわないと戻り作業が莫大に増えて結局工数増大するので、この課題は Phase2.α とでもして着手、終わったら現在時点で戻って L0 作業再開の形をとってください。」
- 「作業選択肢も工数を下げて根本解決を避けるのを２度としないでください。根治最優先です。」
- 「これが何度も起きて出戻りだらけなんです。工数が４倍５倍になるのはこれが理由です。このようなことはもう２度と選択推奨しないでください。」

**2 件目発火 (= 同日後発、Phase 2.α §7.1 candidate 並列罪)**:
- 「これをわたしに確認すること自体が腹立たしいのですが、どうすれば根治するか確定して改修してください」
- 「波及する資料もすべて更新するのを忘れないでください。また誤解して同じ穴に落ちます。」

**3 件目発火 (= 同日後発、設計 doc 精査要請)**:
- 「この穴を作った原因元資料の精査はしないんですか？それが間違っていたから今間違えてるんじゃないんですか？解決方法は本当にこれで正しいのですか？」

⇒ memory 更新 (= 永続化):
- 新規: `feedback_root_cause_no_shortcuts` (= 1 件目発火時起案)
- 拡張: `feedback_design_doc_number_literal_verify` (= 構造属性に **codegen/build pipeline 入力 source path** 追加、1 件目発火時)
- 拡張: `feedback_root_cause_no_shortcuts` §8/§9 (= 「根治徹底度の差」candidate 並列禁止 + 既 AYA literal 根治意思表示済で preflight 質問禁止、2 件目発火時)
- 拡張: `feedback_root_cause_no_shortcuts` §10 (= 設計 doc 整合確認なしで根治確定する罪、3 件目発火時)
- 拡張: `feedback_doubt_self_first` §5 (= 解決策確定前に設計 doc 精査 default、3 件目発火時)

### §C.5 設計 doc 整合確認規律追加 (= 2026-06-06)

本 freeze record + Phase 2.α entry handoff §C.4 構造属性 verify 規律拡張 (= 既存) に加えて、**設計 doc (= source of truth) 整合確認** を verify 規律として正式追加 (= 3 件目発火受領反映):

- 解決策提案前に **design/{00-charter, 01-overview, ..., 10-open-questions} + design/ubo/{WORK_ORDER, READINESS, RELATIONS, INDEX} + ayastorm-r41-ubo-current-state-inventory.md + ayastorm-r41-cross-platform-port-spec.md** の関連箇所を grep / Read で精査
- 特に **AYA 指示 literal** (= design/01-overview.md §1.2 確定方針 #1〜#13、AYA 指示由来項目) は **literal 完全一致 verify**、推測ベース解決策で違反しないこと
- 設計 doc 内 chapter 間 cross-ref (= 「chapter 04 §967 → chapter 01 §5 #5」等) も追跡、孤立解釈で誤読しない
- memory `feedback_doubt_self_first` §5 + `feedback_root_cause_no_shortcuts` §10 適用
