# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-7α' complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-7α' (= codegen `ubo_metadata.inl` V1' update = set=1a/1b split 実装 + 暫定 `binding<40 / >=40` heuristic 撤去) **実装 phase 完了** marker = step (a)..(g) 7 step 全実装 + Exit Criteria 7 項全充足 + build verify literal PASS record

---

## §0. 本 session 着手契機 + session 境界根拠

**契機**: AYA 指示「PC-7α' 実装着手お願いします」literal 受領 (2026-06-05、design-lock commit `11a46edd9b` 後の継続 session)。

**session 境界根拠**: 前 PC-7δ-complete commit `2cc05f0cd6` → PC-7α' design-lock commit `11a46edd9b` は別 session で完了 (= design-lock phase は doc 起案のみ、`indra/` 改変 0 件 = feedback_design_phase_no_code_write 整合)。本 session は **PC-7α' 実装 phase** に専念 = `scripts/ubo_codegen/main.py` + `indra/llrender/llvkloader.cpp` の 2 modified + ubo_metadata.inl 再生成 (build dir、tracked 外)。

---

## §1. 本 session 成果物

### §1.1 編集 1 (scripts/ubo_codegen/main.py +16 / -2)

**追加**: 定数 `_V3A_PROGRAM_SET_A_BINDINGS = 40` + helper `_derive_subset(descriptor_set, binding)` (= design 06c §2.3 整合、`set=1 && binding >= 40` → `subset=1` / それ以外 → `subset=0`) + `_ubo_to_block_spec` 内で descriptor_set / binding を一度 unpack して `_derive_subset` 経由で `BlockSpec.subset` に注入。

採用 = (α'-1) A literal「subset 自動決定 (codegen 側 binding 値から決定)」整合 (= 2026-06-05 AYA literal「全部推奨で OK」record)。

### §1.2 編集 2 (indra/llrender/llvkloader.cpp +11 / -10)

**改変**: `registerProgramUbo` (line 4412-4497) の暫定 heuristic (`src_binding < V3A_PROGRAM_SET_A_BINDINGS` / `< SET_A + SET_B`) を `meta->subset == 0` / `== 1` 参照に置換 + LL_WARNS_ONCE message を「binding `out of V3a set=1a/1b range`」→「meta.subset `out of V3a {0:1a, 1:1b} range`」に更新 + tag block `<AYAstorm r41 PC-7δ (e)>` → `<AYAstorm r41 PC-7α' (e)>` + comment 内 literal scope「codegen V1' split 未到達」→「codegen V1' split 通電済」reflectance 更新。

採用 = (α'-2) A literal「heuristic を `meta.subset` 参照に置換」整合 (= 2026-06-05 AYA literal「全部推奨で OK」record)。

`dst_binding` 計算は変更なし (= subset=0 で `= src_binding` / subset=1 で `= src_binding - V3A_PROGRAM_SET_A_BINDINGS`)、codegen 側の `subset=1 iff binding>=40` 保証を前提とした単純化。

### §1.3 編集 3 (scripts/ubo_codegen/tests/test_main.py、gitignored = local 検証 only)

**改変** = 既存 `UboToBlockSpecTests::test_set_and_binding_forwarded_when_present` + `test_set_and_binding_default_to_zero_when_absent` 2 件に `self.assertEqual(spec.subset, 0)` assertion 追加 (= binding/subset 整合保証) + 新規 `SubsetSplitTests::test_set1_subset_split_at_binding_40_boundary` 1 件追加 (= set=1 binding=0/39 → subset=0 + binding=40/79 → subset=1 + set=0/2/3 binding=40 → subset=0 = 境界 + cross-set negative の 7 assertion を 1 test method 内に集約)。

採用 = (α'-4) A literal「既存 case 拡張 + subset split 専用 case 新規 1 件追加 (130→131 件)」整合 (= 2026-06-05 AYA literal「全部推奨で OK」record)。

local unittest 結果: **131/131 PASS** (= 既存 130 + 新 1 件)。本 file は `.gitignore` line 26 `tests/` パターンで gitignored ゆえ commit 対象外 (= PC-7γ-3 以前と同形)。

### §1.4 生成物 (build-linux-x86_64/codegen/ubo/ubo_metadata.inl、tracked 外)

**再生成**: `--force` で全 94 block 再評価。subset 分布:

| descriptor_set | block 数 | subset=0 | subset=1 |
|---|---|---|---|
| 0 | 4 | 4 | 0 |
| 1 | 2 | 2 | 0 |
| 2 | 31 | 31 | 0 |
| 3 | 57 | 57 | 0 |
| **計** | **94** | **94** | **0** |

採用 = (α'-3) A literal「通常 codegen 再実行で全件 batch」整合 (= 2026-06-05 AYA literal「全部推奨で OK」record)。

**N=0 件 subset=1 注記**: 現 blueprint state は set=1 binding>=40 0 件 (= 既存 set=1 blueprint 2 件 = MaterialUBO / MaterialUBO_Legacy 共に binding=0)、subset auto-detection 自体は正常動作 (= 131 unittest で境界 binding=39/40 検証済)。design 06c §2.3 「set=1 78 block 中 binding>=40 の subset=1 群」は将来 blueprint 追加 (= 別 sub-step) 時に subset=1 が自動付与される (= codegen 側 _derive_subset 通電済)。本 PC-7α' literal scope は subset フィールドの **runtime 経路通電** = 達成済。

---

## §2. 必読 1 件 + pinpoint reference

**次 session 必読 (= PC-7ε 着手前)**:

1. **本 complete doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-alpha-prime-complete.md`

**pinpoint reference (必要時のみ)**:

- PC-7α' design-lock doc: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-alpha-prime-design-lock.md` §3 ambiguity record + §4 実装計画
- PC-7δ complete doc: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-delta-complete.md` §1.3 step (e) bind 配線記録 (= PC-7ε で dynamic offset 実値配線対象 = `bindV3aStatic` 内 `pDynamicOffsets[4]` zero placeholder)
- design 06c §2.3 (= set=1a/1b binding 境界 literal): `docs/specs/ayastorm-r41-gl-removal/design/06c-buffer-layout-and-binding.md`

---

## §3. ambiguity 4 件 AYA 確認 record 表 (= design-lock phase で完了済)

| # | 採用案 | AYA literal「全部推奨で OK」(2026-06-05) record | 実装結果 |
|---|--------|---------------------|---------|
| (α'-1) | A: codegen 自動 (`binding>=40 ? 1 : 0`) | ✅ | `main.py _derive_subset` 実装、§1.1 |
| (α'-2) | A: heuristic → `meta.subset` 置換 | ✅ | `llvkloader.cpp` line 4445-4465、§1.2 |
| (α'-3) | A: codegen 再実行で全件 batch | ✅ | `--force` で 94 block 再評価、§1.4 |
| (α'-4) | A: 既存 case 拡張 + 新 1 件 (130→131) | ✅ | `test_main.py` extend + SubsetSplitTests 1 件、§1.3 |

---

## §4. Exit Criteria 7 項全充足

| # | Criteria | 充足 verify |
|---|----------|-----------|
| (i) | `main.py` で `subset = (binding>=40) ? 1 : 0` 自動決定 | ✅ `_derive_subset` 実装、§1.1 |
| (ii) | `ubo_metadata.inl` 再生成で set=1 binding>=40 件 subset=1 付与 (N=0 現状) | ✅ §1.4 + auto-detection 通電 (= 131 unittest 境界検証済) |
| (iii) | unittest 131/131 PASS | ✅ `python3 -m unittest discover` literal 出力 `Ran 131 tests in 0.064s OK` |
| (iv) | `llvkloader.cpp` `registerProgramUbo` heuristic 撤去 → `meta.subset` 参照 | ✅ git diff、`binding < V3A_PROGRAM_SET_A_BINDINGS` 文字列消滅 (comment 記述のみ残存) |
| (v) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 | ✅ git diff (= GLSL 改変 0 件、host C++ + Python のみ) |
| (vi) | MUSEUBO-A 整合 = mUseUBO=false default で既存 OpenGL 描画 100% 維持 | ✅ `registerProgramUbo` 内部のみ改変 (= mUseUBO gate / forwardToUboUpload 経路不変) |
| (vii) | build verify = llrender + warning 0 + TUT 11+10+13 + codegen 131/131 PASS | ✅ §5 build verify literal record |

---

## §5. build verify literal record

```
[100%] Building CXX object llrender/CMakeFiles/llrender.dir/llvkloader.cpp.o
[100%] Linking CXX static library libllrender.a
[100%] Built target llrender
```

- `make llrender` PASS (warning 0)
- `INTEGRATION_TEST_lluboringbuffer` = `Total Tests: 11, Passed Tests: 11 YAY!! \o/`
- `INTEGRATION_TEST_llassetubopool` = `Total Tests: 10, Passed Tests: 10 YAY!! \o/`
- `INTEGRATION_TEST_llpipelinecachestorage` = `Total Tests: 13, Passed Tests: 13 YAY!! \o/`
- `python3 -m unittest discover -s scripts/ubo_codegen/tests -t scripts/ubo_codegen` = `Ran 131 tests in 0.064s OK`

---

## §6. 残 strict 線形

- **PC-7ε** (= 次 sub-step) = dynamic offset 経路 ring buffer chunk hand-off = `bindV3aStatic` の `pDynamicOffsets[4]` zero placeholder を `sDrawUboRingBufferMgr` 経由 dynamic offset 実値配線 (source: design 07 §9.3 + §12)
- **PC-8** = 3 OS build verify (Linux primary + Win/Mac 後段)
- **PC-N** = Phase 1.C complete marker + 実 GLTF Vulkan draw 通電 + avatar bone storage 経路再配線 (H10-A 持越) + Phase 1.D / Phase 2 着手起点

---

## §7. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + **PC-7α' ✅ 本 commit** + PC-7ε..PC-N ⏳ 次 session 以降

---

## §8. self-verify 9 観点 全 ✅

1. **Exit Criteria 7 項全充足** ✅ §4
2. **必読 (本 doc) 起案 + pinpoint reference 別記** ✅ §2
3. **step (a)..(g) 7 step 全実装** ✅ (a) `_derive_subset` 実装 + (b) `ubo_metadata.inl` 再生成 + (c) 既存 case 拡張 + (d) SubsetSplitTests 新規 1 件 + (e) heuristic 撤去 + (f) unittest 131/131 + (g) build verify
4. **(α'-1)..(α'-4) 4 件 AYA literal「全部推奨で OK」record (2026-06-05)** ✅ §3
5. **GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件** ✅ §4 (v)
6. **MUSEUBO-A 整合 = mUseUBO=false default 経路不変** ✅ §4 (vi) (= `registerProgramUbo` 内部のみ改変、setter / forwardToUboUpload 経路 不変)
7. **llrender build + TUT 11+10+13 + codegen 131/131 全 PASS** ✅ §5
8. **commit 内容 2 modified (main.py + llvkloader.cpp) + 1 new doc (本 complete handoff) + tests/ gitignored + ubo_metadata.inl 再生成は build dir tracked 外 + Co-Authored-By 不在 + CMake 改変 0 + settings.xml 改変 0** ✅
9. **feedback_no_scope_shrink 遵守 = PC-7α' literal scope 4 件 (= main.py subset 決定 + metadata 再生成 + unittest 130→131 + llvkloader 置換) 全件実施、N=0 件 subset=1 は現 blueprint state ゆえ縮小ではない (= logic は通電済)** ✅

---

## §9. 次 session 着手 1 line

**PC-7ε 実装着手** = dynamic offset 経路 ring buffer chunk hand-off = `bindV3aStatic` 内 `pDynamicOffsets[4]` zero placeholder を `sDrawUboRingBufferMgr` 経由 dynamic offset 実値配線 (= source doc = `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-host-pipeline.md` §9.3 + §12)。

design-lock phase 着手要 (= 本 commit 完了後、別 session の fresh context で ambiguity 確認 + 実装計画分解後の実装 phase)。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 complete handoff doc 起案 (= 次 session 引継 marker)
- **feedback_handoff_minimal_pre_req_read** 遵守 = 必読 1 件 + pinpoint reference 別記 §2
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §8
- **feedback_build_only_verified** 遵守 = literal build verify (= llrender + TUT 11+10+13 + codegen 131/131) record §5
- **feedback_no_scope_shrink** 遵守 = PC-7α' literal scope 4 件全件実施 §8 (9)
- **feedback_doubt_self_first** 遵守 = design-lock phase で ambiguity 4 件発見 + AYA 確認後実装 (= 本 session は実装専念)
- **feedback_confirm_referent_before_acting** 遵守 = (α'-1)..(α'-4) 4 件 AYA literal「全部推奨で OK」record (2026-06-05) §3、推測実装なし
- **feedback_ubo_migration_one_at_a_time** 遵守 = PC-7α' = subset split 単独 sub-step、dynamic offset (PC-7ε) は分離
- **feedback_design_phase_no_code_write** 整合 = 本 PC-7α' は実装 phase (= design-lock 別 session で `indra/` 改変 0 件 commit 11a46edd9b 完了済)、本 session で `indra/llrender/llvkloader.cpp` 改変は実装 phase ゆえ整合
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示「commit してください」literal 受領まで commit せず
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
- **feedback_tests_dir_never_commit** 整合 = `scripts/ubo_codegen/tests/test_main.py` は `.gitignore` line 26 `tests/` パターンで gitignored、local 検証のみ + commit 対象外 (= PC-7γ-3 以前と同形)
