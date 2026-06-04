# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-N-2 complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-N-2 (= `bindV3aRigged` set=2 復活 = signature 拡張 (`const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS]` 引数化、`bindV3aStatic` 同形、PC-7ε パターン) + `recordAvatarPlaceholderDraw` 内 allocate-chain 配線 (= PC-N-1 `recordPlaceholderPoolDraw` パターン同形)) の **実装 phase 完了** marker = step (a)-(g) 7 step 全実装 + Exit Criteria 10 項全充足。`indra/llrender/llvkloader.cpp` 1 編集 (+65 / -19 = signature + body + allocate-chain + comment + log)。

> **本 doc 位置付け**: PC-N-2 design-lock (= 直前 commit `0c3d0ac292`) で確定した ambiguity (N2-1)..(N2-9) 9 件 + G-1 + step (a)-(g) 7 step + Exit Criteria 10 項に基づく実装 phase 完了 doc。次 session で PC-N-4 design-lock 着手 (= AllocateResult.grew 観測 frame 末尾で `vkUpdateDescriptorSets` 再発火 deferred、PC-N decomposition §4.4)。

---

## §0. 本 session 着手契機 + literal scope record

**契機**: AYA 指示「PC-N-2 実装着手お願いします」literal 受領 (2026-06-05、design-lock commit `0c3d0ac292` 後の継続 session = 別 session の fresh context) + 必読 1 件 = `handoff-substep-...-pc-n-2-design-lock.md` (= ambiguity (N2-1)..(N2-9) 9 件 + G-1 全 AYA literal「Claude 推奨案 OK」record 済) 全文 Read + pinpoint reference (`bindV3aStatic` PC-7ε 拡張形 + `recordPlaceholderPoolDraw` PC-N-1 新形 + `bindV3aRigged` 現 body + `recordAvatarPlaceholderDraw` 現 body) Read → step (a)-(g) 7 step 実装 → build verify 全 PASS → 本 complete handoff doc 起案。

**実装した PC-N-2 literal scope 5 件** (= PC-N decomposition §4.2 継承):

1. **`bindV3aRigged` signature 拡張** = 旧 `void bindV3aRigged(VkCommandBuffer cmd_buf, U32 frame_index)` → 新 `void bindV3aRigged(VkCommandBuffer cmd_buf, U32 frame_index, const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS])` + guard 拡張 (`sDrawUboSetV3a` nullptr + `dynamic_offsets` nullptr 追加) ((N2-1) A + (N2-5) A 採用)
2. **`bindV3aRigged` body 内 set=2 bind 復活** = 第 1 `vkCmdBindDescriptorSets` を 3 set → 4 set (= set=0/1a/1b/2、`V3A_DRAW_SET_BINDINGS=4`) + `pDynamicOffsets=dynamic_offsets` wire、第 2 call (= set=3 swap) 構造維持 ((N2-1) A + (N2-3) A + (N2-4) A 採用)
3. **`recordAvatarPlaceholderDraw` allocate-chain 配線** = `sDrawUboRingBufferMgr` nullptr → `recordPlaceholderPoolDraw` fallback + `writeDrawUbo(PerDrawUBO_LightParams, 0, zero_buf, 256, dynamic_offset)` + `dynamic_offsets[V3A_DRAW_SET_BINDINGS]={dynamic_offset×4}` 構築 + `bindV3aRigged(cmd_buf, sFrameIndex, dynamic_offsets)` 呼出 ((N2-2) A + (N2-3) A + (N2-6) A 採用)
4. **PC-7ε (e) comment 整合更新 (G-1 解消)** = 旧 `<AYAstorm r41 PC-7ε (e)>` tag block (= 「PC-N 実 GLTF avatar Vulkan draw 通電時に H10-A avatar bone storage 再配線と一括」literal) を `<AYAstorm r41 PC-N-2 (c)+(d)>` tag block に置換 (= 「set=2 復活 = PC-N-2 で実施、H10-A bone storage 再配線 = PC-N-3、ring buffer grow 自動 re-wire = PC-N-4 持越」literal) ((N2-7) A + (G-1) 採用)
5. **log message 更新** = `recordAvatarPlaceholderDraw` first-fire LL_INFOS marker を「PC-N-2 set=2 復活通電済: writeDrawUbo(PerDrawUBO_LightParams, zero 256B) → dynamic_offsets[4] (= 4 個同一 offset) → bindV3aRigged (set=0 Frame V3a + set=1a/1b ProgramUbo + set=2 DrawUbo V3a + set=3 AssetUbo, push descriptor 経路 disable 維持) + push constant 64 B / VERTEX_BIT + vkCmdDraw(3,1,0,0)」literal に更新 ((N2-7) A 採用)

**`.h` 改変 0 件**: `bindV3aRigged` は `llvkloader.cpp:66` 内匿名 `namespace { ... }` のローカル helper、`llvkloader.h` に公開宣言なし (= grep verify 済)。design-lock doc §0 の `.h` 改変想定は事実訂正 → 実装 phase で確認し `.cpp` のみで完結。

---

## §1. 必読 1 件 + pinpoint reference (次 session = PC-N-4 design-lock 着手向け)

**次 session 必読**:

1. **本 PC-N-2 complete doc 全文**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-n-2-complete.md`

**pinpoint reference**:

- **PC-N decomposition design-lock**: `handoff-...-pc-n-decomposition-design-lock.md` §4.4 = PC-N-4 scope 規定 (= AllocateResult.grew 観測 frame 末尾で `vkUpdateDescriptorSets` 再発火 deferred、Phase 境界、依存関係)
- **`writeDrawUbo` 実装**: `indra/llrender/llvkloader.cpp:4711-4790` (= `<AYAstorm r41 PC-N-1 (a)>` tag block) = ring buffer grow 観測経路 (`LL_WARNS_ONCE` + grow flag、PC-N-4 hook 起点)
- **`flushDrawUbos` 実装**: `indra/llrender/llvkloader.cpp:4376-4413` (= PC-N-1 で実装済 first-fire log + PC-N-4 hook placeholder)
- **`LLUboRingBuffer::AllocateResult.grew` 仕様**: `indra/llrender/lluboringbuffer.h` (= grow 観測 flag、PC-N-4 vkUpdateDescriptorSets re-fire 起点)
- **design 07 §7.5 (grow 時 descriptor 再 wire)**: `design/07-vulkan-api-state.md` = grow flag → vkUpdateDescriptorSets re-fire spec

---

## §2. 実装内容 (step (a)-(g) 7 step 全実装記録)

### §2.1 改変サマリ表

| step | file | line range | 改変 | tag block |
|------|------|------------|------|-----------|
| (a) | `indra/llrender/llvkloader.cpp` | 1781-1828 (= 旧 1781-1819 から +9 行拡張) | `bindV3aRigged` signature 拡張 (= `const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS]` 引数追加) + guard 拡張 (= `sDrawUboSetV3a` + `dynamic_offsets` nullptr 追加) | `<AYAstorm r41 PC-N-2 (a)>` |
| (b) | `indra/llrender/llvkloader.cpp` | 1804-1820 (= 同 function body) | 第 1 `vkCmdBindDescriptorSets` を 3 set → 4 set (= set=0/1a/1b/2、`sDrawUboSetV3a` 追加) + `pDynamicOffsets=dynamic_offsets` wire + 第 2 call (= set=3 swap) 構造維持 | `<AYAstorm r41 PC-N-2 (b)>` |
| (c) | `indra/llrender/llvkloader.cpp` | 5253-5290 (= 旧 5243-5248 を +32 行拡張) | `recordAvatarPlaceholderDraw` 内 allocate-chain 配線 (= `sDrawUboRingBufferMgr` nullptr fallback + `writeDrawUbo(PerDrawUBO_LightParams, zero 256B)` + `dynamic_offsets[4]={offset×4}` + `bindV3aRigged(拡張呼出)`) | `<AYAstorm r41 PC-N-2 (c)>` |
| (d) | `indra/llrender/llvkloader.cpp` | 5255-5269 (= 旧 PC-7ε (e) tag block 内 comment 完全置換) | G-1 解消 = 旧「PC-N 実 GLTF avatar Vulkan draw 通電時に H10-A avatar bone storage 再配線と一括」literal を「set=2 復活 = PC-N-2、bone storage 再配線 = PC-N-3、ring buffer grow re-wire = PC-N-4」literal に置換 | `<AYAstorm r41 PC-N-2 (d)>` |
| (e) | `indra/llrender/llvkloader.cpp` | 5313-5320 (= first-fire LL_INFOS marker 更新) | 「PC-N-2 set=2 復活通電済」literal 追加 + bind set 列挙更新 (= set=3 → set=2 DrawUbo + set=3) + `writeDrawUbo` API path 経路明示 | (e) は (c)+(d) tag block 内に統合 |
| (f) | `build-linux-x86_64/` | n/a | build verify 全 PASS (= llrender + warning 0 + TUT 11+10+13 + codegen 131/131) | n/a |
| (g) | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-...-pc-n-2-complete.md` | new file | 本 doc 起案 + AYA commit 指示後 commit 予定 | n/a |

### §2.2 改変 file 2 件 (= 1 file modified + 1 new doc)

- `indra/llrender/llvkloader.cpp` 1 編集 (+65 / -19) = step (a)+(b) `bindV3aRigged` signature + body 拡張 + step (c)+(d) `recordAvatarPlaceholderDraw` allocate-chain + comment 更新 + step (e) log message 更新
- `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-...-pc-n-2-complete.md` 1 新規 = 本 doc

**改変 0 件 (= PC-N-2 scope 外)**:
- `indra/llrender/llvkloader.h` 改変 0 件 (= `bindV3aRigged` は anonymous namespace ローカル、`.h` 公開なし、design-lock §0 想定の `.h` 改変は事実訂正で `.cpp` のみで完結)
- codegen 出力 = (N1-7) A pattern 踏襲、binding=2/3 配置 + 4 binding 再分配は別 sub-step (= PC-N-5 持越)
- shader = set=2 binding 0/1 既存参照のみ、改変なし
- avatar bone storage 経路 = PC-N-3 持越
- CMake / settings.xml / 新 file 0 (除本 doc)

---

## §3. build verify 結果 (step (f))

| # | 検証項目 | 結果 |
|---|----------|------|
| 1 | `make -j4 llrender` | PASS + ERROR 0 + WARNING 0 |
| 2 | `INTEGRATION_TEST_lluboringbuffer` | 11/11 PASS (= PC-3 algorithm 層 regression なし) |
| 3 | `INTEGRATION_TEST_llassetubopool` | 10/10 PASS (= PC-4 algorithm 層 regression なし) |
| 4 | `INTEGRATION_TEST_llpipelinecachestorage` | 13/13 PASS (= PC-5 algorithm 層 regression なし) |
| 5 | codegen unittest (= `scripts/ubo_codegen/tests/`) | 131/131 PASS (= Phase 1.A / 1.B / 1.C PC-1..PC-N-1 regression なし) |

**実行 command** (= 履歴記録、`python3` で codegen unittest 実行):

```bash
cd /home/ishikawa/work_firestorm/phoenix-firestorm/build-linux-x86_64
make -j4 llrender
make -j4 INTEGRATION_TEST_lluboringbuffer INTEGRATION_TEST_llassetubopool INTEGRATION_TEST_llpipelinecachestorage
./sharedlibs/bin/INTEGRATION_TEST_lluboringbuffer
./sharedlibs/bin/INTEGRATION_TEST_llassetubopool
./sharedlibs/bin/INTEGRATION_TEST_llpipelinecachestorage
cd /home/ishikawa/work_firestorm/phoenix-firestorm/scripts/ubo_codegen
python3 -m unittest discover -s tests -v
```

---

## §4. PC-N-2 Exit Criteria 10 項全充足

| # | Criteria | 充足 |
|---|----------|------|
| (i) | `bindV3aRigged` signature 拡張 = `const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS]` 引数追加 + guard 拡張 (= `sDrawUboSetV3a` + `dynamic_offsets` nullptr 追加、`bindV3aStatic` 同形、(N2-1) A + (N2-5) A) | ✅ |
| (ii) | `bindV3aRigged` body 内 set=2 bind 復活 = 第 1 `vkCmdBindDescriptorSets` を 3 set → 4 set 拡張 (= set=0/1a/1b/2) + `pDynamicOffsets` に引数 wire、第 2 call (= set=3 swap) 構造維持 ((N2-1) A + (N2-4) A) | ✅ |
| (iii) | `recordAvatarPlaceholderDraw` allocate-chain 配線 = `sDrawUboRingBufferMgr` nullptr → `recordPlaceholderPoolDraw` fallback + `writeDrawUbo(PerDrawUBO_LightParams, 0, zero_buf, 256, dynamic_offset)` + `dynamic_offsets[V3A_DRAW_SET_BINDINGS]={dynamic_offset×4}` + `bindV3aRigged(cmd_buf, sFrameIndex, dynamic_offsets)` 呼出 ((N2-2) A + (N2-3) A + (N2-6) A) | ✅ |
| (iv) | PC-7ε (e) comment 整合更新 = `llvkloader.cpp` の G-1 該当 comment を「set=2 復活 = PC-N-2、bone storage 再配線 = PC-N-3」literal に更新 ((N2-7) A + (G-1)) | ✅ |
| (v) | log message 更新 = `recordAvatarPlaceholderDraw` first-fire LL_INFOS に「PC-N-2 set=2 復活通電済」literal 追加 ((N2-7) A) | ✅ |
| (vi) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 | ✅ |
| (vii) | MUSEUBO-A 整合 = `mUseUBO=false` default で既存 OpenGL 描画 100% 維持 + `sDrawUboRingBufferMgr` nullptr 多重 fallback | ✅ |
| (viii) | build verify = `llrender` build + ERROR 0 / WARNING 0 + INTEGRATION_TEST 11+10+13 全 PASS + codegen unittest 131/131 PASS ((N2-8) A) | ✅ |
| (ix) | tag block 統一 = `<AYAstorm r41 PC-N-2 (a)>` + `<AYAstorm r41 PC-N-2 (b)>` + `<AYAstorm r41 PC-N-2 (c)>` + `<AYAstorm r41 PC-N-2 (d)>` + log marker `PC-N-2 set=2 復活通電済` | ✅ |
| (x) | handoff complete doc 起案 + AYA commit 指示後 commit (= Co-Authored-By 不在 + 個別 file 指定 + 新 file 0 除本 doc + CMake 改変 0 + settings.xml 改変 0 + codegen 改変 0) | ✅ (commit は AYA 指示待ち) |

---

## §5. 残 strict 線形

- PC-N-4 design-lock (= **次 session 着手**、AllocateResult.grew 観測 frame 末尾で `vkUpdateDescriptorSets` 再発火 deferred、`flushDrawUbos` PC-N-1 hook placeholder 実装化)
- PC-N-4 実装
- PC-N-3 design-lock (= `writeAvatarBoneStorage` helper 新設 + bone storage 経路再配線 H10-A 持越)
- PC-N-3 実装 = **Phase 1.C complete**
- PC-8 (3 OS build verify、Linux primary 各 sub-step で取得済 + Win/Mac AYA 環境依頼)
- PC-N-5 = **Phase 1.D 着手起点** (= 実 GLTF Vulkan draw 通電 1 stub)

---

## §6. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1 design-lock ✅ + PC-N-1 ✅ + PC-N-2 design-lock ✅ + **PC-N-2 ✅ 本 commit** + PC-N-4 design-lock ⏳ 次 session + PC-N-4 実装 ⏳ + PC-N-3 design-lock ⏳ + PC-N-3 実装 ⏳ = Phase 1.C complete ⏳ + PC-8 (3 OS build verify) ⏳ + PC-N-5 = Phase 1.D 着手起点 ⏳

---

## §7. self-verify 9 観点 全 ✅

1. **Exit Criteria 10 項全充足** §4 ✅
2. **必読 1 件 (本 complete handoff doc) 起案 + pinpoint reference 5 件 別記** §1 (= PC-N decomposition §4.4 + `writeDrawUbo` 実装 + `flushDrawUbos` PC-N-1 hook + `AllocateResult.grew` 仕様 + design 07 §7.5) ✅
3. **step (a)..(g) 7 step 全実装** §2.1 = (a) signature 拡張 + (b) set=2 bind 復活 + (c) allocate-chain 配線 + (d) PC-7ε comment 更新 + (e) log marker + (f) build verify + (g) handoff doc 起案 ✅
4. **ambiguity (N2-1)..(N2-9) 9 件 + G-1 AYA literal「Claude 推奨案 OK」record (2026-06-05)** §2.1 各 step に採用 ambiguity 併記 ✅
5. **GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件** = `bindV3aRigged` + `recordAvatarPlaceholderDraw` 改変は Vulkan-only code 内、`#ifdef` 不要 ✅
6. **MUSEUBO-A 整合 = `mUseUBO=false` default 経路不変** = `bindV3aRigged` は Vulkan 描画 path 内のみ呼ばれる + `recordAvatarPlaceholderDraw` は Vulkan placeholder offscreen FBO 経路 + 多重 nullptr fallback (= `sAvatarBonePipeline` / `sAvatarBoneLayout` / `sAYAStandardLayout` / `sDrawUboRingBufferMgr` nullptr 時 `recordPlaceholderPoolDraw` fallback) ✅
7. **llrender build + WARNING 0 + TUT 11+10+13 + codegen 131/131 全 PASS** §3 ✅
8. **commit 内容 1 modified (llvkloader.cpp +65 / -19) + 1 new doc (handoff complete) + 新 file 0 (除 doc) + CMake 改変 0 + settings.xml 改変 0 + Co-Authored-By 不在** ✅
9. **feedback_no_scope_shrink 遵守** = PC-N-2 literal scope 5 件 §0 全件実施、(N2-2) A (= `PerDrawUBO_LightParams` 共有) + (N2-3) A (= 1 allocate 4 binding 同 offset) は AYA literal「Claude 推奨案 OK」(2026-06-05) record 済段階分離 = 縮小ではない (= PC-N-1 (N1-5) B + PC-7ε (ε-2) A パターン同形、real avatar data + 4 独立 allocate は PC-N-5 持越 record 済) ✅

---

## §8. 次 session 着手 1 line

**PC-N-4 design-lock 着手** = AllocateResult.grew 観測 frame 末尾で `vkUpdateDescriptorSets` 再発火 deferred (= PC-N decomposition §4.4)、`flushDrawUbos` PC-N-1 hook placeholder を実 trigger 化、design-lock phase 着手要 (= 本 commit 完了後、別 session の fresh context で ambiguity 確認 + 実装計画分解後の実装 phase)。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 complete handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 必読 1 件 + pinpoint reference 5 件 別記、本 session も Read 4 file pinpoint のみ (= PC-N-2 design-lock doc + `bindV3aStatic` PC-7ε 拡張形 + `recordPlaceholderPoolDraw` PC-N-1 新形 + `bindV3aRigged` + `recordAvatarPlaceholderDraw` 現 body)、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §7
- **feedback_build_only_verified** 遵守 = llrender build + TUT 11/11 + 10/10 + 13/13 + codegen 131/131 で literal 検証取得 §3
- **feedback_no_scope_shrink** 遵守 = §7 (9) 記載通り
- **feedback_doubt_self_first** 遵守 = design-lock phase で ambiguity 9 件 + G-1 発見 + 推奨案提示 + AYA 確認 + literal「Claude 推奨案 OK」受領後本実装、推測実装なし
- **feedback_confirm_referent_before_acting** 遵守 = 9 件 + G-1 batch AYA 確認 design-lock phase で完了 (2026-06-05)
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = PC-N-2 = `bindV3aRigged` set=2 復活 + `recordAvatarPlaceholderDraw` allocate-chain 配線単独 sub-step、PC-N-3 (bone storage 再配線) + PC-N-4 (grow auto re-wire) + PC-N-5 (実 GLTF avatar draw) は分離
- **feedback_design_phase_no_code_write** 整合 = 本 PC-N-2 は実装 phase = design-lock commit `0c3d0ac292` で `indra/` 改変 0 件完了済、本 session で `indra/llrender/llvkloader.cpp` 改変は実装 phase ゆえ整合
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「commit してください」literal 受領まで commit せず
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
- **feedback_no_bare_reference_ids** 遵守 = (N2-1)..(N2-9) 各 ID に項目名 / 採用案内容併記 + (a)..(g) 各 step に作業内容併記 §2.1
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、git add 個別 file 指定 + `git add -A` 不使用予定

---
