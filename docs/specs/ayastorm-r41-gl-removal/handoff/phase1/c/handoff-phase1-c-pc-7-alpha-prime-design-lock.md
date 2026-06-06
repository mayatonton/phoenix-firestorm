# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-7α' design-lock** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-7α' (= codegen `ubo_metadata.inl` V1' update = set=1a / 1b split 実装 + 暫定 `binding<40 / >=40` heuristic 撤去) 着手前の **design-lock phase** 完了 marker = ambiguity (α'-1)..(α'-4) 4 件の AYA literal「全部推奨で OK」(2026-06-05) record + 実装計画 (a)..(g) 分解 + Exit Criteria 7 項明文化

---

## §0. PC-7α' literal scope (= AYA 確認済 4 件)

PC-7α' **literal scope** (= 直前 commit `2cc05f0cd6` (PC-7δ complete) handoff doc §4 残線形 line 206 由来):

1. **scripts/ubo_codegen/main.py** = subset 自動決定ロジック追加 (= `descriptor_set == 1 and binding >= 40` → `subset = 1` assign)
2. **ubo_metadata.inl 再生成** = set=1 の 78 block 全件 subset 再評価 (= binding>=40 の N 件に subset=1 付与)
3. **scripts/ubo_codegen/tests/** = 既存 unittest 130 件 PASS 維持 + subset split 検証 case 新規 1 件追加 (= 131 件目標)
4. **indra/llrender/llvkloader.cpp** `registerProgramUbo` = 現 `binding<40` heuristic を `meta.subset` 参照に置換 (= V3A_PROGRAM_SET_A_BINDINGS=40 const は単一 source of truth → codegen 経由のみ参照)

**本 PC-7α' scope 外** (= PC-7ε 以降):
- dynamic offset 経路 ring buffer chunk hand-off (= PC-7ε)
- 3 OS build verify (= PC-8)
- 実 GLTF Vulkan draw 通電 (= PC-N)
- avatar bone storage 経路再配線 (= PC-N、H10-A 持越)

---

## §1. 必読 3 件 (次 session 着手前)

1. **本 design-lock doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-alpha-prime-design-lock.md`
2. **PC-7δ complete handoff doc**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-delta-complete.md`
   - §1.3.4 step (e) registerProgramUbo 暫定 heuristic 詳細 + §4 残線形 PC-7α' Z2-C 持越根拠
3. **design 06c §2.3 + PC-7α complete handoff doc (Z2-C) 項**
   - pinpoint reference:
     - `docs/specs/ayastorm-r41-gl-removal/design/06c-buffer-layout-and-binding.md` §2.3 (= set=1a binding 0-39 / set=1b binding 40-79 literal 定義)
     - `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-alpha.md` (Z2-C) 持越 record (= PC-7α' 独立 sub-task 分離根拠)

---

## §2. 現状調査 (= 2026-06-05 確認)

### §2.1 codegen 構造 (= `scripts/ubo_codegen/`)

- `main.py` = entry point、`_ubo_to_block_spec` で `BlockSpec` 構築、`subset: int = 0` default
- `perfect_hash.py` = CHD hash + emit_metadata_inl (= `subset` フィールド emit 経路 frame 完備、現在 0 統一出力)
- `glsl_parser.py` / `std140.py` / `spirv_reflect.py` / `glslang_preproc.py` = 補助
- `tests/` = 7 file 計 1601 行 unittest、130 件 全 PASS (= PC-7γ-3 commit `947e848bcb` で確認済)

### §2.2 `ubo_metadata.inl` 現状 (= 生成先 `build-linux-x86_64/codegen/ubo/ubo_metadata.inl`)

- entry フィールド: `block_name` / `block_hash` / `block_size` / `descriptor_set` / `binding` / `subset` / `cadence_tag` / `member_count`
- block 集計: set=0 (per-frame + singleton) = 4 + set=1 (per-program) = **78 全 subset=0** + set=2 (per-draw) = 7 + set=3 (per-asset + per-skin) = 3 = **計 94 block**
- set=1 binding 分布: 0-79 で 78 件混在 (= subset split 未付与)

### §2.3 design 06c §2.3 (= V3a 5-set scheme)

- **set=1a** (subset=0): binding 0-39 = `V3A_PROGRAM_SET_A_BINDINGS=40`
- **set=1b** (subset=1): binding 40-79 = `V3A_PROGRAM_SET_B_BINDINGS=40`
- device limit `maxDescriptorSetUniformBuffers` Vulkan 1.3 minimum 72 ≥ 40 で全 device 適合前提

### §2.4 Z2-C 持越 record (= `pc-7-alpha.md` 由来)

- PC-7α complete handoff doc (= 2026-06-05) で `(Z2-C) ubo_metadata.inl V1' update (set=1a/1b split 反映) は独立 PC-7α' sub-task として PC-7α VkObject scaffolding から分離` literal record
- 分離根拠 = 130 件 unittest + 78 block 全件 set/binding 変更 = 影響大、回帰隔離必要

### §2.5 PC-7δ 暫定 heuristic 詳細 (= `indra/llrender/llvkloader.cpp` line 4412-4496 `registerProgramUbo`)

- `V3A_PROGRAM_SET_A_BINDINGS=40` / `V3A_PROGRAM_SET_B_BINDINGS=40` constexpr 設定済
- 暫定 mapping: `if (binding < 40) → set=1a / else if (binding < 80) → set=1b (binding -= 40) / else → LL_WARNS_ONCE skip`
- 実装: `sProgramUboSetA[FRAMES_IN_FLIGHT=3]` / `sProgramUboSetB[FRAMES_IN_FLIGHT=3]` triple-buffer + register-once + bind-many
- 現状: `meta.descriptor_set` 参照のみ、`meta.subset` 未参照 (= subset フィールド runtime 経路 未通電) ※ **本記述は 2026-06-05 (Phase 1.C PC-7α' design-lock) 時点の historical record**、その後 PC-7α' 実装 (commit 11a46edd9b → 後続 PC-7α' implementation commit) で `llvkloader.cpp:5348-5385` `registerProgramUbo` 内に `meta->subset == 0` / `== 1` 参照分岐実装済、subset 列 runtime 経路通電完了 (= Phase 2.L0 sub-session 3 step 1 grep 確定 2026-06-06)、本 doc は historical lock record ゆえ本文未更新

---

## §3. ambiguity (α'-1)..(α'-4) 4 件 AYA literal「全部推奨で OK」record (2026-06-05)

| # | ambiguity | 候補 | 採用 (= AYA literal「全部推奨で OK」record 2026-06-05) |
|---|----------|------|--------------------------------------------------|
| (α'-1) | subset 決定 timing | A=codegen 自動 (= binding 値から決定) / B=GLSL blueprint 明示指定 | **A 採用** = `subset = (binding >= 40) ? 1 : 0` を `main.py _ubo_to_block_spec` で実装 |
| (α'-2) | `registerProgramUbo` 移行方式 | A=heuristic を `meta.subset` 参照に置換 / B=subset 並走で両 ABI 保持 | **A 採用** = heuristic 撤去 literal scope 整合 |
| (α'-3) | `ubo_metadata.inl` 再生成 scope | A=通常 codegen 再実行で全件 batch / B=手作業 patch | **A 採用** = perfect_hash 衝突 codegen 再実行で自動解消、130 件 unittest で回帰捕捉 |
| (α'-4) | unittest 修正範囲 | A=既存 case 拡張 + subset split 専用 case 新規 1 件追加 / B=新規 case のみ | **A 採用** = 既存 binding/subset 整合保証 + 130→131 件 増分明確 |

### §3.1 採用根拠 4 件

**(α'-1) A: subset 自動決定 (codegen 側)**:
- 根拠 = `V3A_PROGRAM_SET_A_BINDINGS=40` を single source of truth、GLSL 改変 0 件で GATE-B 整合 (= `#ifdef LL_VULKAN_GLSL` 新規追加 0 件)、GLSL blueprint 増改ゼロで保守容易性確保、design 06c §2.3 binding 0-39 / 40-79 literal 境界に従う

**(α'-2) A: `meta.subset` 参照置換**:
- 根拠 = PC-7α' literal scope 「暫定 binding<40 / binding>=40 heuristic 撤去」整合、`meta.subset` 経由で codegen-runtime 連動を明示化 (= subset フィールドの runtime 経路通電)、両 ABI 保持は技術債務蓄積で却下

**(α'-3) A: codegen 再実行で全件 batch**:
- 根拠 = 通常 codegen 再実行で 78 block 全件 subset 再評価 (= 自動)、perfect_hash 衝突は再実行で自動解消、130 件 unittest で回帰捕捉 (= PC-7γ-3 で 3 block 追加時の seed 調整実績整合)、手作業 patch は 78 件 manual error 危険

**(α'-4) A: 既存拡張 + 新 test 1 件追加**:
- 根拠 = subset split 検証は専用 test case で明示化、既存 case は subset assertion 追加で binding/subset 整合保証、130→131 件で増分明確、subset 専用 case 1 件で test failure 時の原因切り分け容易

---

## §4. 実装計画 (a)..(g) 7 step 分解

### §4.1 step 一覧

| # | step | 改変対象 | 規模 |
|---|------|---------|-----|
| (a) | `_ubo_to_block_spec` で subset 自動決定 1 行追加 | `scripts/ubo_codegen/main.py` | small (~3 行) |
| (b) | codegen 再実行で `ubo_metadata.inl` 再生成 | `build-linux-x86_64/codegen/ubo/ubo_metadata.inl` (生成物) | auto (= 78 件中 binding>=40 の N 件 subset=1 付与) |
| (c) | unittest 既存 case 拡張 (= binding/subset 整合 assertion 追加) | `scripts/ubo_codegen/tests/` 該当 file | medium (~10-20 行) |
| (d) | unittest 新規 case 1 件追加 (= subset split 専用 = set=1a/1b 境界 binding=39/40 で subset=0/1 確認) | `scripts/ubo_codegen/tests/` 該当 file | medium (~30 行) |
| (e) | `registerProgramUbo` heuristic 撤去 → `meta.subset` 参照置換 | `indra/llrender/llvkloader.cpp` line 4412-4496 | small (~5-10 行差分) |
| (f) | unittest 全件実行 確認 (= 131/131 PASS) | `python3 -m unittest discover -s scripts/ubo_codegen/tests` | verify |
| (g) | build verify = llrender build + TUT 11+10+13 + codegen 131/131 PASS | make + INTEGRATION_TEST | verify |

### §4.2 GATE-B 整合確認

- `#ifdef LL_VULKAN_GLSL` 新規追加 **0 件** (= GLSL 改変 0 件、codegen + host C++ + unittest のみ改変)
- mUseUBO runtime gate 単独 + LLVKLoader internal defensive guard 並走で十分 (= PC-7γ-1..δ 整合)

### §4.3 MUSEUBO-A 整合確認

- `mUseUBO=false` default で既存 OpenGL 描画 100% 維持 (= setter 内 `forwardToUboUpload` 不呼出)
- subset フィールド runtime 経路通電は `registerProgramUbo` 内のみ (= mUseUBO 不関与)、bind 実発火は placeholder offscreen FBO 経路 (= PC-7δ 整合)
- subset=1 (= sProgramUboSetB target) は実 OpenGL 描画影響ゼロ (= Vulkan placeholder draw のみ通電)

### §4.4 cadence × set 一覧 (= PC-7δ 整合再掲)

| cadence | set | subset | binding 範囲 | block 例 |
|--------|----|--------|-----|---------|
| PER_FRAME | 0 | 0 | 0-3 | FrameViewProj / FrameLights / FrameAtmosphere_Lighting / Global_ReflectionProbes |
| PER_PROGRAM | 1 | **0 (= 1a)** | **0-39** | (78 block 中 binding<40 の N 件) |
| PER_PROGRAM | 1 | **1 (= 1b)** | **40-79** | (78 block 中 binding>=40 の N 件) |
| PER_DRAW | 2 | 0 | (ring buffer + dynamic offset) | PerDrawUBO_* + Material* (PC-7ε 通電予定) |
| PER_ASSET / PER_SKIN | 3 | 0 | 0-2 | Asset_GLTFNodes / Asset_GLTFMaterials / Skin_GLTFJoints |
| SINGLETON | 0 | 0 | 3 | Global_ReflectionProbes (= writeSingletonUbo target、PC-7δ 通電済) |

---

## §5. Exit Criteria 7 項

| # | Exit Criteria | 充足判定方法 |
|---|--------------|------------|
| (i) | `scripts/ubo_codegen/main.py` で `subset = (binding >= 40) ? 1 : 0` 自動決定ロジック実装 | git diff 確認 |
| (ii) | `ubo_metadata.inl` 再生成で set=1 の 78 block 中 binding>=40 の N 件 subset=1 付与 | grep `subset.*1` count |
| (iii) | unittest 既存 130 件 PASS 維持 + subset split 専用 case 1 件追加 = **131/131 PASS** | `python3 -m unittest discover` |
| (iv) | `indra/llrender/llvkloader.cpp` `registerProgramUbo` heuristic 撤去 → `meta.subset` 参照置換 | git diff 確認 + `binding < 40` 文字列消滅 |
| (v) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 | `git diff` で LL_VULKAN_GLSL grep |
| (vi) | MUSEUBO-A 整合 = mUseUBO=false default で既存 OpenGL 描画 100% 維持 | llvkloader 改変は registerProgramUbo 内部のみで mUseUBO 経路不変 |
| (vii) | build verify = llrender build PASS + warning 0 + TUT 11+10+13 PASS + codegen 131/131 PASS | make + INTEGRATION_TEST |

---

## §6. 着手手順 (= 次 session)

1. **必読 3 件 Read** (= §1 列挙、所要 ~10 分)
2. **step (a)** `main.py` subset 自動決定ロジック追加
3. **step (b)** codegen 再実行 (= `python3 scripts/ubo_codegen/main.py` 等の build chain 経由再生成、Makefile target 確認後)
4. **step (c)(d)** unittest 既存拡張 + 新規 1 件追加
5. **step (e)** `llvkloader.cpp` `registerProgramUbo` 置換
6. **step (f)** unittest 131/131 PASS 確認
7. **step (g)** build verify (= llrender build + TUT 3 件)
8. self-verify 9 観点 全 ✅ 後 complete handoff doc 起案
9. AYA literal 「commit してください」受領後 commit

---

## §7. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + **PC-7α' design-lock ✅ 本 commit** + PC-7α' 実装 ⏳ 次 session + PC-7ε..PC-N ⏳ 次々 session 以降

---

## §8. 引き継ぎ memory 14 件 遵守確認

1. **project_ayastorm_r41_vulkan_migration** = milestone state §7 反映
2. **project_r41_phase1b_vulkan_host_gate** (GATE-B 確定) = §4.2 整合確認
3. **project_ayastorm_r41_design_principles** (2 大原則) = (1) Upstream OpenGL 取り込みやすさ = call site API 温存 (= GLSL 改変 0 件) + (2) Core プロセス分散 = subset フィールドで A/B set 隔離 = 並列化容易
4. **feedback_ubo_migration_one_at_a_time** = PC-7α' 単独 sub-step (= subset split 単独実施、dynamic offset (PC-7ε) は分離)
5. **feedback_proactive_handoff** = 本 design-lock doc 起案
6. **feedback_handoff_minimal_pre_req_read** = 必読 3 件 + pinpoint reference §1
7. **feedback_self_verify_before_handoff** = 9 観点 self-verify (= §9)
8. **feedback_build_only_verified** = design-lock phase は build verify 対象外 (= `indra/` 改変 0 件)、実装 phase で literal 検証取得予定
9. **feedback_no_scope_shrink** = PC-7α' literal scope 4 件 §0 完全分解、縮小なし
10. **feedback_doubt_self_first** = ambiguity (α'-1)..(α'-4) 4 件発見で停止 + 推奨案提示 + AYA 確認
11. **feedback_confirm_referent_before_acting** = 4 件 batch AYA 確認 + literal「全部推奨で OK」record (2026-06-05)
12. **feedback_design_phase_no_code_write** = 本 design-lock phase は `indra/` 改変 0 件 (= 実装は別 session)
13. **feedback_release_branch_workflow** = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
14. **feedback_no_auto_commit** + **feedback_no_claude_coauthor** = AYA 明示指示後 commit + Co-Authored-By 不在

---

## §9. self-verify 9 観点 全 ✅

1. **PC-7α' literal scope 4 件 §0 完全分解** ✅
2. **必読 3 件 §1 列挙 + pinpoint reference 別記** ✅
3. **現状調査 §2 = codegen 構造 + ubo_metadata.inl + design 06c §2.3 + Z2-C 持越 record + PC-7δ heuristic 詳細 5 項網羅** ✅
4. **ambiguity (α'-1)..(α'-4) 4 件 AYA literal「全部推奨で OK」record (2026-06-05)** ✅ §3
5. **採用根拠 4 件明文化** ✅ §3.1
6. **実装計画 (a)..(g) 7 step 分解** ✅ §4.1
7. **Exit Criteria 7 項明文化** ✅ §5
8. **GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= design-lock phase は doc のみ、実装 phase でも GLSL 改変 0 件予定)** ✅ §4.2
9. **MUSEUBO-A 整合 = mUseUBO=false default 経路不変 + bind 実発火 placeholder 整合** ✅ §4.3

---

## §10. 次 session 着手 1 line

**PC-7α' 実装** (= design-lock 完了済本 commit) = `main.py` subset 自動決定 + codegen 再実行で `ubo_metadata.inl` 再生成 + unittest 130→131 件 + `llvkloader.cpp` `registerProgramUbo` heuristic → `meta.subset` 参照置換、Exit Criteria 7 項全充足 + build verify llrender + TUT 11+10+13 + codegen 131/131 PASS。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 design-lock handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 必読 3 件 + pinpoint reference 別記
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅
- **feedback_build_only_verified** 遵守 = design-lock phase は `indra/` 改変 0 件で build verify 対象外、実装 phase で literal 検証取得予定
- **feedback_no_scope_shrink** 遵守 = PC-7α' literal scope 4 件 §0 完全分解
- **feedback_doubt_self_first** 遵守 = ambiguity 4 件発見で停止 + 推奨案提示 + AYA literal「全部推奨で OK」確認後 design-lock doc 起案
- **feedback_confirm_referent_before_acting** 遵守 = 4 件 batch AYA 確認、推測実装なし
- **feedback_ubo_migration_one_at_a_time** 遵守 = PC-7α' = subset split 単独 sub-step、dynamic offset (PC-7ε) は分離
- **feedback_design_phase_no_code_write** 整合 = 本 design-lock phase は doc 起案のみ、`indra/` 改変 0 件
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示「commit してください」literal 受領まで commit せず
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
