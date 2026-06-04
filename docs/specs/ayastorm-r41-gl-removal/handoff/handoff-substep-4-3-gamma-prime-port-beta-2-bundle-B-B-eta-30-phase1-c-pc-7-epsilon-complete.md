# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-7ε complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-7ε (= dynamic offset 経路 ring buffer chunk hand-off = `bindV3aStatic` の `pDynamicOffsets[4]` zero placeholder を `sDrawUboRingBufferMgr` 経由 dynamic offset 実値配線 + `sDrawUboSetV3a` ↔ ring buffer `VkBuffer` の `vkUpdateDescriptorSets` 接続) **実装 phase 完了** marker = step (a)-(g) 7 step 全実装 + Exit Criteria 10 項全充足 + build verify (llrender + WARNING 0 + INTEGRATION_TEST 11+10+13 + codegen 131/131) PASS

---

## §0. 本 session 着手契機 + literal scope record

**契機**: AYA 指示「PC-7ε 実装着手お願いします」literal 受領 (2026-06-05、直前 PC-7ε design-lock commit `c4b719463a` 後の継続 session = fresh context、design-lock phase で ambiguity (ε-1)..(ε-6) 6 件 全 AYA literal「Claude 推奨案 OK」record 済 + 実装計画 (a)-(g) 7 step + Exit Criteria 10 項明文化済)。

**PC-7ε literal scope** (= design-lock §0 から継承):
1. **descriptor wiring** = `sDrawUboSetV3a` (1 set 固定) に ring buffer の `VkBuffer` を `vkUpdateDescriptorSets` で 4 binding 分 write (= `initVulkan` 内 1 度のみ決定論的 timing)
2. **dynamic offset hand-off** = `bindV3aStatic` signature 拡張 = `const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS]` 引数化 (= caller 責任)、zero placeholder local 削除
3. **`recordPlaceholderPoolDraw` 1 draw chain** = `sDrawUboRingBufferMgr->allocate(256)` → dummy zero memset → `dynamic_offsets[4] = { offset, offset, offset, offset }` → `bindV3aStatic` 呼出
4. **ring buffer grow log** = `AllocateResult.grew == true` 観測時に `LL_WARNS_ONCE` 出力 (= re-wire 自動化は PC-N 持越)
5. **`bindV3aRigged` 無改変** = set=2 復活 + per-draw 配線は PC-N 持越 (= H10-A avatar bone storage 再配線と一括)、comment 1 行追加で持越記録のみ
6. **session 境界** = 本 session で実装 phase 完了 = `indra/llrender/llvkloader.cpp` 1 file modified (+152 / -12)

---

## §1. 必読 1 件 + pinpoint reference

**次 session 必読 (= PC-7ε 後続 sub-step 着手前)**:

1. **本 complete doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-epsilon-complete.md`

**pinpoint reference (必要時のみ)**:

- **design 07 §7**: `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md` §7 dynamic offset (L2) 実 Vulkan 配線 + ring buffer 容量 (= 本 PC-7ε source)
- **PC-7ε design-lock doc**: `handoff-...-pc-7-epsilon-design-lock.md` = ambiguity 6 件 + 実装計画 7 step + Exit Criteria 10 項
- **lluboringbuffer.h**: public API (= `AllocateResult` struct, `allocate()`, `beginFrame()`, `getBuffer()`)

---

## §2. 実装内容 (= step (a)-(g) 7 step 全実装記録)

### §2.1 編集 1: `indra/llrender/llvkloader.cpp` (+152 / -12)

#### step (a) = `wireDrawUboSetV3aToRingBuffer()` helper 新設 (line 2629-2698、+70 行)

- 匿名 namespace 内 `createV3aDescriptorSets()` 直後に配置 (= line 2629 `<AYAstorm r41 PC-7ε (a)>` tag block)
- guard = `sDevice` / `sDrawUboRingBufferMgr` / `sDrawUboSetV3a` 未初期化で `LL_WARNS_ONCE` + return false
- ring buffer の VkBuffer 取得 = `sDrawUboRingBufferMgr->getBuffer()` → `sDrawUboRingBufferRecords[handle].buffer` lookup
- 4 個 `VkDescriptorBufferInfo` 構築 (= buffer=ring buffer VkBuffer / offset=0 / range=256 placeholder)
- 4 個 `VkWriteDescriptorSet` 構築 (= dstSet=`sDrawUboSetV3a` / dstBinding=0/1/2/3 / descriptorType=`UNIFORM_BUFFER_DYNAMIC`)
- `vkUpdateDescriptorSets(sDevice, V3A_DRAW_SET_BINDINGS, writes, 0, nullptr)` 呼出
- 成功時 `LL_INFOS` log + return true

#### step (b) = `initVulkan` 内呼出 chain 追加 (line 3422-3431、+10 行)

- 既存 `createV3aDescriptorSets()` chain (= line 3331-3338) 直後に `<AYAstorm r41 PC-7ε (b)>` tag block 追加
- `wireDrawUboSetV3aToRingBuffer()` 失敗時 = `LL_WARNS_ONCE` log + initVulkan 続行 (= fatal でない、MUSEUBO-A 整合)

#### step (c) = `bindV3aStatic` signature 拡張 (line 1733-1779、+9 / -10 行)

- 旧: `void bindV3aStatic(VkCommandBuffer cmd_buf, U32 frame_index)`
- 新: `void bindV3aStatic(VkCommandBuffer cmd_buf, U32 frame_index, const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS])`
- 内部 `const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS] = { 0u, 0u, 0u, 0u };` local 削除
- guard 追加 = `dynamic_offsets == nullptr` で early return
- tag block update = `<AYAstorm r41 PC-7δ (g)>` → `<AYAstorm r41 PC-7ε (c)>`
- comment 内 literal scope (= 「pDynamicOffsets は本 PC-7δ では一律 0」) を「caller 責任」literal に更新

#### step (d) = `recordPlaceholderPoolDraw` 内 allocate-chain 配線 (line 5010-5050、+40 行)

- `vkCmdBindPipeline` 直後に `<AYAstorm r41 PC-7ε (d)>` tag block 新設
- MUSEUBO-A guard = `if (!sDrawUboRingBufferMgr) return;`
- `const LLUboRingBuffer::AllocateResult alloc = sDrawUboRingBufferMgr->allocate(256);`
- `alloc.success == false` → `LL_WARNS_ONCE` + return
- `alloc.grew == true` → `LL_WARNS_ONCE` (= (ε-6) A、re-update は PC-N 持越)
- dummy zero memset = `sDrawUboRingBufferRecords` lookup + `std::memset(mapped + alloc.offset, 0, alloc.size)` (= placeholder phase、PC-6ε-3 で real data write 置換予定)
- `dynamic_offsets[V3A_DRAW_SET_BINDINGS] = { alloc.offset, alloc.offset, alloc.offset, alloc.offset };` 構築 (= 同 offset × 4 binding、(ε-2) A)
- `bindV3aStatic(cmd_buf, sFrameIndex, dynamic_offsets);` 呼出

#### step (d') log message 更新 (line 5077-5081)

- 旧: `Placeholder pool draw fired (PSO bind ... + bindV3aStatic set=0/1a/1b/2 + push constant ...)`
- 新: `Placeholder pool draw fired (PSO bind ... + per-draw ring buffer allocate (256 B) + bindV3aStatic set=0/1a/1b/2 (PC-7ε dynamic offset 配線済) + push constant ...)`

#### step (e) = `recordAvatarPlaceholderDraw` 無改変 + PC-N 持越 comment (line 5111-5116、+4 / -0 行)

- `bindV3aRigged(cmd_buf, sFrameIndex)` 呼出維持 (= signature 不変、set=2 skip ゆえ dynamic offset 0 個)
- 直前に `<AYAstorm r41 PC-7ε (e)>` comment 4 行追加 = PC-N 持越記録 ((ε-5) A、H10-A avatar bone storage 再配線と一括)

#### step (f) = ring buffer-set=2 接続維持 code review (改変 0 行)

- code review 完了:
  - `wireDrawUboSetV3aToRingBuffer()` 呼出 = `initVulkan` 1 度のみ ✅
  - ring buffer VkBuffer lifecycle = `createDrawUboRingBuffer()` で確保 (line 1376) → `shutdownVulkan` で destroy (line 3907-3912) ✅
  - `sDrawUboSetV3a` lifecycle = `createV3aDescriptorSets()` (line 2594-2596) で allocate → `shutdownVulkan` で nullify + pool destroy 経由 implicit free (line 3787 + 3789-3792) ✅
  - shutdown 順序 = descriptor set null → pool/layout destroy → ring buffer VkBuffer destroy = Vulkan spec 整合 ✅
  - grow path = LLUboRingBuffer 内 `tryGrow()` で VkBuffer 再確保、本 PC-7ε は `LL_WARNS_ONCE` のみ ((ε-6) A 採用、dummy phase で grow 起きない想定)

### §2.2 改変サマリ

| file | 改変 | tag block 新規 | tag block update |
|---|---|---|---|
| `indra/llrender/llvkloader.cpp` | +152 / -12 (1 file) | `<PC-7ε (a)>` × 1 + `<PC-7ε (b)>` × 1 + `<PC-7ε (d)>` × 1 + `<PC-7ε (e)>` × 1 | `<PC-7δ (g)>` → `<PC-7ε (c)>` × 1 |

**他 file 改変 0 件** = CMake 改変 0 + settings.xml 改変 0 + GLSL 改変 0 + scripts/ 改変 0 + docs/ design doc 改変 0 (本 handoff doc 新規 1 件のみ)。

---

## §3. build verify literal record

### §3.1 llrender build (= `make -j4 llrender`)

```
[ 90%] Building CXX object llrender/CMakeFiles/llrender.dir/llvkloader.cpp.o
[ 90%] Linking CXX static library libllrender.a
[100%] Built target llrender
```

- ERROR 0 件 + WARNING 0 件 (= grep `warning|error` 結果 0 件)
- Linux x86_64 primary (= AYAstorm 3 OS のうち Linux primary、Win/Mac は PC-8 で後段検証)

### §3.2 INTEGRATION_TEST 3 件 (= algorithm 層 regression check)

```
Unit test group_completed name=LLUboRingBuffer
	Total Tests:	11
	Passed Tests:	11	YAY!! \o/
---
Unit test group_completed name=LLAssetUboPool
	Total Tests:	10
	Passed Tests:	10	YAY!! \o/
---
Unit test group_completed name=LLPipelineCacheStorage
	Total Tests:	13
	Passed Tests:	13	YAY!! \o/
```

= PC-3 (LLUboRingBuffer algorithm) / PC-4 (LLAssetUboPool grow-only) / PC-5 (LLPipelineCacheStorage 64 MB cap) **全 PASS** = PC-7ε 改変による algorithm 層 regression 0 件。

### §3.3 codegen unittest (= scripts/ubo_codegen)

```
Ran 131 tests in 0.056s

OK
```

= Phase 1.A / 1.B / 1.C PC-1..PC-7α' 全件 + PC-7α' で新設 `SubsetSplitTests` 1 件 計 131/131 PASS = codegen 層 regression 0 件。

---

## §4. PC-7ε Exit Criteria 10 項全充足

| # | Criteria | 充足 verify 結果 |
|---|----------|---------|
| (i) | `wireDrawUboSetV3aToRingBuffer()` helper 新設 + 4 binding 分 `vkUpdateDescriptorSets` call | ✅ `llvkloader.cpp:2629-2698` 新設、`vkUpdateDescriptorSets(sDevice, V3A_DRAW_SET_BINDINGS, writes, 0, nullptr)` literal |
| (ii) | `initVulkan` 内で ring buffer init + `createV3aDescriptorSets()` 完了後に wire 呼出 | ✅ `llvkloader.cpp:3422-3431` `wireDrawUboSetV3aToRingBuffer()` 呼出追加 (= `createV3aDescriptorSets()` chain 直後) |
| (iii) | `bindV3aStatic` signature 拡張 = `const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS]` 引数化 | ✅ `llvkloader.cpp:1748-1750` signature literal `const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS]` |
| (iv) | `bindV3aStatic` 内 zero placeholder local 削除 (= caller responsibility) | ✅ `llvkloader.cpp` 内 `dynamic_offsets[V3A_DRAW_SET_BINDINGS] = { 0u, 0u, 0u, 0u }` literal 消滅 (= PC-7δ local 削除) |
| (v) | `recordPlaceholderPoolDraw` 内 allocate → dummy write → bind chain 配線 | ✅ `llvkloader.cpp:5010-5050` `sDrawUboRingBufferMgr->allocate(256)` + `dynamic_offsets[4] = { alloc.offset, ... }` + `bindV3aStatic(..., dynamic_offsets)` literal |
| (vi) | `recordAvatarPlaceholderDraw` 無改変 = `bindV3aRigged` + set=2 復活は PC-N 持越 | ✅ `llvkloader.cpp:5111-5116` bind 経路改変 0 件 (= comment 4 行追加のみ、signature 不変) |
| (vii) | ring buffer grow 観測時 `LL_WARNS_ONCE` log のみ | ✅ `llvkloader.cpp:5036-5040` `alloc.grew` 判定 + `LL_WARNS_ONCE` literal、自動 re-update path 無し |
| (viii) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 | ✅ `git diff HEAD` で新規 `#ifdef LL_VULKAN_GLSL` 行 grep 0 件 |
| (ix) | MUSEUBO-A 整合 = mUseUBO=false default で既存 OpenGL 描画 100% 維持 | ✅ `recordPlaceholderPoolDraw` (= sky offscreen FBO 経路) + `wireDrawUboSetV3aToRingBuffer` (= Vulkan initVulkan 経路) のみ改変、setter / forwardToUboUpload / OpenGL draw 経路不変 + `sDrawUboRingBufferMgr` nullptr guard で Vulkan 未起動時 early return |
| (x) | build verify = llrender + warning 0 + TUT 11+10+13 + codegen 131/131 全 PASS | ✅ §3 literal 記録 (llrender Linking PASS + WARNING 0 + TUT YAY × 3 + codegen Ran 131 tests OK) |

---

## §5. 残 strict 線形

- **PC-7α''** = (optional) codegen 追加調整、現状 N=0 件 subset=1 = 実 blueprint 拡張時の必要性次第
- **PC-8** = 3 OS build verify (Linux primary ✅ 本 PC-7ε で取得済、Win/Mac は後段)
- **PC-N** = Phase 1.C complete marker + 実 GLTF Vulkan draw 通電 + avatar bone storage 経路再配線 (= H10-A 持越 + `bindV3aRigged` set=2 復活 + ring buffer grow 自動 re-wire) + `flushDrawUbos` real per-draw data write 通電 (= PC-6ε-3 持越) + Phase 1.D / Phase 2 着手起点

---

## §6. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + **PC-7ε ✅ 本 commit** + PC-7α'' (optional) ⏳ + PC-8..PC-N ⏳ 次 session 以降

---

## §7. self-verify 9 観点 全 ✅

1. **Exit Criteria 10 項全充足** ✅ §4 表
2. **必読 1 件 (本 complete doc) 起案 + pinpoint reference 3 件 別記** ✅ §1
3. **step (a)..(g) 7 step 全実装** ✅ §2.1 = (a) wireDrawUboSetV3aToRingBuffer 新設 + (b) initVulkan chain + (c) bindV3aStatic signature 拡張 + (d) recordPlaceholderPoolDraw allocate-chain + (e) recordAvatarPlaceholderDraw 無改変 + comment + (f) shutdown 順序 review + (g) build verify
4. **ambiguity (ε-1)..(ε-6) 6 件 AYA literal「Claude 推奨案 OK」record (2026-06-05)** ✅ design-lock §3 継承 + 本 complete §2.1 各 step に採用案 ID 併記
5. **GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件** ✅ git diff 確認
6. **MUSEUBO-A 整合 = mUseUBO=false default 経路不変** ✅ = `recordPlaceholderPoolDraw` (= sky offscreen FBO 経路、視覚 no-op 等価維持) + `wireDrawUboSetV3aToRingBuffer` (= Vulkan initVulkan 経路) のみ改変 + `sDrawUboRingBufferMgr` nullptr guard で early return
7. **llrender build + WARNING 0 + TUT 11+10+13 + codegen 131/131 全 PASS** ✅ §3 literal
8. **commit 内容** = 1 modified (`llvkloader.cpp`) + 1 new doc (本 complete handoff) + 新 file 0 + CMake 改変 0 + settings.xml 改変 0 + diff stat +152/-12 + Co-Authored-By 不在
9. **feedback_no_scope_shrink 遵守** = PC-7ε literal scope 6 件 §0 全件実施、(ε-2) 1 allocate placeholder + (ε-5) bindV3aRigged 無改変 + (ε-6) grow re-wire 持越 は AYA literal「Claude 推奨案 OK」(2026-06-05) record 済段階分離 = 縮小ではない

---

## §8. 次 session 着手 1 line

**PC-7α'' 着手 (optional、現状 N=0 件 subset=1 = blueprint 追加時に必要次第)** または **PC-8 = 3 OS build verify (Win/Mac)** または **PC-N = 実 GLTF Vulkan draw 通電 + avatar bone storage 再配線 + flushDrawUbos real per-draw write 通電 (= PC-6ε-3 + H10-A + ring buffer grow auto re-wire の集約 sub-step)**。AYA さん判断待ち。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 complete handoff doc 起案 (= 次 session 引継 marker)
- **feedback_handoff_minimal_pre_req_read** 遵守 = 次 session 必読 1 件 + pinpoint reference 3 件 別記 §1
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §7
- **feedback_build_only_verified** 遵守 = llrender build + WARNING 0 + INTEGRATION_TEST 11+10+13 + codegen 131/131 全 literal 記録 §3
- **feedback_no_scope_shrink** 遵守 = PC-7ε literal scope 6 件 §0 全件実施
- **feedback_doubt_self_first** 遵守 = design-lock phase で ambiguity 6 件発見 + 推奨案提示 + AYA literal「Claude 推奨案 OK」確認後本実装、推測実装なし
- **feedback_confirm_referent_before_acting** 遵守 = (ε-1)..(ε-6) 6 件 + source doc 訂正 1 件 batch AYA 確認 (design-lock phase 完了、2026-06-05)
- **feedback_ubo_migration_one_at_a_time** 遵守 = PC-7ε = dynamic offset hand-off + descriptor wiring 単独 sub-step、bindV3aRigged set=2 復活 (PC-N) + flushDrawUbos real per-draw write (PC-6ε-3) + ring buffer grow auto re-wire (PC-N) は分離
- **feedback_design_phase_no_code_write** 整合 = 本 PC-7ε は実装 phase (= design-lock 別 commit `c4b719463a` で `indra/` 改変 0 件 完了済)、本 session で `indra/llrender/llvkloader.cpp` 改変は実装 phase ゆえ整合
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「commit してください」literal 受領まで commit せず
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件 + git add 個別 file 指定予定 + `git add -A` 不使用
- **feedback_no_bare_reference_ids** 遵守 = (ε-1)..(ε-6) 各 ID に項目名 / 採用案内容併記 §2.1 + §4

---
