# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.D **PC-N-7 complete = stub index buffer Vulkan 経路通電** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: Phase 1.D 内 **2nd sub-step (PC-N-7)** = stub index buffer Vulkan 経路通電 (= file-static `sGltfStubIndexBuffer` (= UINT32 ascending CCW `{ 0, 1, 2 }`、12 B) を VMA host-visible mapped buffer 経由 1 回 initVulkan allocate + initial memcpy + `vkCmdBindIndexBuffer` 配線 + `vkCmdDrawIndexed` 置換、`sGltfStubAssetPipeline` 再利用 ((N7-8) A、Vulkan 仕様 §10.4 で index buffer は dynamic state ゆえ PC-N-6 同 PSO で issue 可能)、`AYAGltfStubIndexBufferEnabled` 段階 cvar 新設) の **実装完了 marker**。step (a)/(c)/(d)/(e)/(e') 5 site 実装 + Exit Criteria 10 項全充足 + build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131。

> **本 doc 位置付け**: PC-N-7 design-lock (= `handoff-...-phase1-d-pc-n-7-design-lock.md`、commit `6a75b61924`) で確定された実装計画 (a)-(g) 7 step (b 除く、(b) は pipeline 再利用ゆえ作業なし) を順次実施 → build verify literal 取得 → Exit Criteria 10 項 self-verify 完了の **実装完了 handoff doc**。PC-N-8..PC-N-10 残 3 sub-step は別 session で個別 design-lock + 実装。

---

## §0. 本 session 着手契機 + literal scope record

**契機**: AYA 指示「r41 Phase 1.D PC-N-7 実装着手お願いします」literal 受領 (2026-06-05、PC-N-7 design-lock commit `6a75b61924` 後の継続 session = 別 session の fresh context) + 必読 1 件 (PC-N-7 design-lock handoff doc) 全文 Read + pinpoint reference 6 件 Read (= PC-N-6 (a) sGltfStubVertexBuffer declare 近傍 + PC-N-6 (c) initVulkan VMA allocate + createGltfStubAssetPipeline + PC-N-6 (d) shutdownVulkan + PC-N-6 (e) recordGltfAssetDraw cvar 分岐 + bindIndexBufferVk wrap signature + AYAGltfStubVertexBufferEnabled cvar 配置近傍) → step (a)/(c)/(d)/(e)/(e') 5 site 順次実装 → build verify literal 取得 → 本 complete doc 起案。

**PC-N-7 literal scope** (= design-lock §0 7 件、本 commit 全件実装):

1. **`sGltfStubIndexBuffer` file-static VMA buffer 新設** ✅ `llvkloader.cpp:636-668` (PC-N-7 (a) tag block)
2. **stub index data initial upload** ✅ `llvkloader.cpp` initVulkan PC-N-7 (c) tag block
3. **`sGltfStubAssetPipeline` 再利用** ✅ ((N7-8) A、新 pipeline 関数追加 0 件)
4. **shutdownVulkan vmaDestroyBuffer 対称配置** ✅ `llvkloader.cpp` shutdownVulkan PC-N-7 (d) tag block
5. **`recordGltfAssetDraw` 改変** ✅ `llvkloader.cpp` PC-N-7 (e) tag block (= PC-N-6 (e) 直前並列配置、3 cvar 優先順位確定 PC-N-7 > PC-N-6 > PC-N-5)
6. **`AYAGltfStubIndexBufferEnabled` cvar 新設** ✅ `settings.xml` PC-N-7 (e') (= AYAGltfStubVertexBufferEnabled 直後並列配置)
7. **build verify** ✅ llrender PASS + WARNING 0 + TUT 11/11 + 10/10 + 13/13 + codegen 131/131

---

## §1. 実装内容 5 site

### §1.1 (a) sGltfStubIndexBuffer file-static declare + sGltfStubIndexData const

**配置**: `indra/llrender/llvkloader.cpp` anonymous namespace 内、PC-N-6 (a) `sGltfStubVertexData` static_assert 直後。

**code**:

- `VkBuffer sGltfStubIndexBuffer = VK_NULL_HANDLE`
- `VmaAllocation sGltfStubIndexAllocation = VK_NULL_HANDLE`
- `void* sGltfStubIndexMapped = nullptr`
- `constexpr U32 sGltfStubIndexCount = 3u` ((N7-11) B)
- `constexpr U32 sGltfStubIndexStride = 4u` ((N7-7) B UINT32)
- `constexpr U32 sGltfStubIndexBufferSize = 12u`
- `const U32 sGltfStubIndexData[3] = { 0u, 1u, 2u }` ((N7-3) A ascending CCW)
- `static_assert` sGltfStubIndexData size 一致確認

### §1.2 (b) pipeline 関数追加なし ((N7-8) A 採用)

**根拠**: Vulkan 仕様 (VkSpec §10.4) で `vkCmdBindIndexBuffer` + index type は dynamic state、PSO immutable state に含まれない。PC-N-6 `sGltfStubAssetPipeline` をそのまま bind して vkCmdDrawIndexed issue 可能。新 pipeline 関数追加 0 件。

### §1.3 (c) initVulkan VMA allocate + initial upload

**配置**: `indra/llrender/llvkloader.cpp` `initVulkan` 内、PC-N-6 (c) tag block 直後並列。

- `sAllocator nullptr` guard + `sGltfStubIndexBuffer == VK_NULL_HANDLE` guard
- `VkBufferCreateInfo` = `INDEX_BUFFER_BIT` ((N7-7) B) + `EXCLUSIVE` sharing
- `VmaAllocationCreateInfo` = `VMA_MEMORY_USAGE_AUTO` + `HOST_ACCESS_SEQUENTIAL_WRITE_BIT` + `MAPPED_BIT` ((N7-5) A、PC-N-6 同形)
- `vmaCreateBuffer` success → `alloc_info.pMappedData` 取得 → `memcpy(sGltfStubIndexMapped, sGltfStubIndexData, sGltfStubIndexBufferSize)` ((N7-6) A 1 回)
- LL_INFOS 通電 marker (size + indices + stride + mapped pointer)
- LL_WARNS_ONCE 3 段 graceful degrade (vmaCreateBuffer fail / mapped nullptr / sAllocator nullptr)

### §1.4 (d) shutdownVulkan vmaDestroyBuffer 対称配置

**配置**: `indra/llrender/llvkloader.cpp` `shutdownVulkan` 内、PC-N-6 (d) tag block 直後並列。

- `sGltfStubIndexBuffer != VK_NULL_HANDLE && sAllocator != VK_NULL_HANDLE` guard
- `vmaDestroyBuffer(sAllocator, sGltfStubIndexBuffer, sGltfStubIndexAllocation)`
- 3 storage (buffer / allocation / mapped) nullify
- pipeline は PC-N-6 (d) で破棄済ゆえ追加 `vkDestroyPipeline` なし ((N7-8) A 整合)

### §1.5 (e) recordGltfAssetDraw cvar 分岐 + bindIndexBufferVk + vkCmdDrawIndexed

**配置**: `indra/llrender/llvkloader.cpp` `recordGltfAssetDraw` 関数 body 内、**PC-N-6 (e) tag block 直前並列**配置 (= 3 cvar 優先順位 PC-N-7 > PC-N-6 > PC-N-5 確定)。

- `static LLCachedControl<bool> sAyastormGltfStubIbEnabled(gSavedSettings, "AYAGltfStubIndexBufferEnabled", false)`
- 4 guard: cvar true + sGltfStubAssetPipeline ≠ NULL + sGltfStubVertexBuffer ≠ NULL + sGltfStubIndexBuffer ≠ NULL
- `vkCmdBindPipeline(cmd_buf, GRAPHICS, sGltfStubAssetPipeline)` ((N7-8) A PC-N-6 reuse)
- PC-N-6 同形 per-draw UBO 配線 (= writeDrawUbo PerDrawUBO_LightParams 256B zero + dynamic_offsets[4])
- PC-N-6 同形 per-Skin UBO 配線 (= writeSkinUbo sGltfStubSkin Skin_GLTFJoints identity 256B + flushSkinUbos + bindV3aRigged)
- PC-N-6 同形 push constant 64B identity / VERTEX_BIT
- **PC-N-7 核心差分**: `bindVertexBufferVk(sGltfStubVertexBuffer, 0)` + `bindIndexBufferVk(sGltfStubIndexBuffer, 0, VK_INDEX_TYPE_UINT32)` + `vkCmdDrawIndexed(cmd_buf, sGltfStubIndexCount, 1, 0, 0, 0)`
- first-fire LL_INFOS marker (= "PC-N-7 (e) GLTF stub index buffer draw 通電 (first fire): ..."): pipeline + buffers + bind sequence + vkCmdDrawIndexed args literal
- `return` で PC-N-6 / PC-N-5 経路スキップ (別 cvar gate で 3 並走分離)

### §1.6 (e') AYAGltfStubIndexBufferEnabled cvar 新設 (settings.xml)

**配置**: `indra/newview/app_settings/settings.xml`、PC-N-6 `AYAGltfStubVertexBufferEnabled` 直後並列。

- `<key>AYAGltfStubIndexBufferEnabled</key>` Boolean default 0 Persist=1
- Comment 内に cvar 優先順位 (PC-N-7 > PC-N-6 > PC-N-5) + PC-N-6 経路依存性 + PC-N-10 deprecate 予定明示
- `<!-- <FS:AYAstorm r41 Phase 1.D PC-N-7> ... -->` tag block で囲む

---

## §2. build verify literal 取得 ((N7-12) A 採用)

### §2.1 llrender build

```
make -j4 llrender
```

→ **PASS / ERROR 0 / WARNING 0**。新 site (= sGltfStubIndexBuffer file-static + PC-N-7 (c)/(d)/(e) tag block + sAyastormGltfStubIbEnabled cvar) 全 compile success。

### §2.2 INTEGRATION_TEST 3 件

- `INTEGRATION_TEST_lluboringbuffer` → **11/11 PASS YAY**
- `INTEGRATION_TEST_llassetubopool` → **10/10 PASS YAY**
- `INTEGRATION_TEST_llpipelinecachestorage` → **13/13 PASS YAY**

### §2.3 codegen unittest

```
cd scripts/ubo_codegen && python3 -m unittest discover tests
```

→ **131/131 OK**。PC-N-1..PC-N-6 同形維持、(N7-12) A 整合。

### §2.4 GATE-B integrity check

`grep -c "LL_VULKAN_GLSL" indra/llrender/llvkloader.cpp` = **6** = PC-N-6 commit `a68a45f5fd` 時点と同数 = **GATE-B 違反なし** (= `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、cvar runtime gate のみ)。

---

## §3. PC-N-7 Exit Criteria 10 項全充足 ((N7-13) A 採用)

| # | Criteria | 充足 |
|---|----------|------|
| (i) | sGltfStubIndexBuffer file-static + VMA allocation declare + sGltfStubIndexData[3] const 配置 (PC-N-7 (a) tag block) | ✅ `llvkloader.cpp` |
| (ii) | initVulkan VMA allocate + initial upload (= memcpy `sGltfStubIndexData` to mapped) 配線 (PC-N-7 (c) tag block) | ✅ `llvkloader.cpp` |
| (iii) | `sGltfStubAssetPipeline` 再利用 ((N7-8) A) = 新 pipeline 関数追加 0 件、PC-N-6 で確立済 PSO に直接 vkCmdDrawIndexed issue | ✅ |
| (iv) | shutdownVulkan 内 `vmaDestroyBuffer` 対称破棄配線 (PC-N-7 (d) tag block) = pipeline は PC-N-6 (d) で破棄済ゆえ追加 `vkDestroyPipeline` なし | ✅ `llvkloader.cpp` |
| (v) | `recordGltfAssetDraw` 内 `AYAGltfStubIndexBufferEnabled` cvar 分岐 + 同 pipeline bind + `bindVertexBufferVk` + `bindIndexBufferVk(UINT32)` + `vkCmdDrawIndexed(M)` 配線 (PC-N-7 (e) tag block、PC-N-6 (e) 分岐直前並列) | ✅ `llvkloader.cpp` |
| (vi) | `AYAGltfStubIndexBufferEnabled` cvar 新設 (settings.xml) + LLCachedControl<bool> default false Persist=1 | ✅ `settings.xml` |
| (vii) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= cvar runtime gate のみ) | ✅ count=6 不変 |
| (viii) | MUSEUBO-A 整合 = `mUseUBO=false` default + `AYAGltfStubIndexBufferEnabled=false` default 経路不変、5 段 graceful degrade (= sAllocator / sGltfStubIndexBuffer / sGltfStubVertexBuffer / sGltfStubAssetPipeline / sGltfStubIndexMapped 各 nullptr guard) | ✅ |
| (ix) | build verify llrender + WARNING 0 + TUT 11+10+13 + codegen 131/131 全 PASS ((N7-12) A) | ✅ |
| (x) | tag block 統一 PC-N-7 (a)/(c)/(d)/(e) + LL_INFOS first-fire marker PC-N-7 (e) 通電 literal 追加 + handoff complete doc 起案 | ✅ 本 commit |

---

## §4. 改変 file list

1. **`indra/llrender/llvkloader.cpp`** (+185 / -0) = (a) sGltfStubIndexBuffer file-static declare + sGltfStubIndexData const + (c) initVulkan VMA allocate + initial upload + (d) shutdownVulkan vmaDestroyBuffer + (e) recordGltfAssetDraw cvar 分岐 + 同 pipeline bind + bindVertexBufferVk + bindIndexBufferVk(UINT32) + vkCmdDrawIndexed(M,1,0,0,0) = 4 編集 step、新 pipeline 関数追加 0 件 ((N7-8) A 再利用)
2. **`indra/newview/app_settings/settings.xml`** (+23 / -0) = AYAGltfStubIndexBufferEnabled Boolean cvar 1 件追加 ((N7-10) A)
3. **`docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md`** (+2 / -1) = §6 PC-N-7 行状態 ✅ 反映 + §A 更新履歴 1 行追記
4. **新 doc 1 件**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-...-phase1-d-pc-n-7-complete.md` = 本 doc

**改変 0 件**: `indra/llrender/llvkloader.h` (= 新 declare 全 anonymous namespace 内、register/unregister API signature 変更なし、bindIndexBufferVk 既配置) + CMake 改変 0 + codegen 改変 0 + shader 改変 0 + tests/ 改変 0。

---

## §5. 残 strict 線形

PC-N-7 complete ✅ 本 commit → **PC-N-8 design-lock** ⏳ 次 session (= 実 LL::GLTF::Asset 経由 vertex/index buffer Vulkan infrastructure 新設 + register/write asset.cpp 側、Phase 1.D 3rd sub-step、design-lock phase 着手要) → PC-N-8 実装 ⏳ + PC-N-9 design-lock + 実装 (= GLTFSceneManager::render 統合 + `AYAGltfRealDrawEnabled` cvar gate) ⏳ + PC-N-10 design-lock + 実装 (= cleanup + 3 stub cvar deprecate) ⏳ → **Phase 1.D complete** ⏳ → Phase 1 全完了 → Mac/Win 開発者補完 phase (= ayastorm-r41-cross-platform-port-spec.md 確定形提供)。

---

## §6. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1 design-lock ✅ + PC-N-1 ✅ + PC-N-2 design-lock ✅ + PC-N-2 ✅ + PC-N-4 design-lock ✅ + PC-N-4 ✅ + PC-N-3 design-lock ✅ + PC-N-3 ✅ = Phase 1.C complete ✅ + PC-8 Linux primary marker ✅ = Phase 1.C strict 線形終了 ✅ + PC-N-5 design-lock ✅ + PC-N-5 ✅ = Phase 1.D 着手起点 実装完了 ✅ + Phase 1.D decomposition design-lock ✅ + PC-N-6 design-lock ✅ + PC-N-6 ✅ = Phase 1.D 内 1st sub-step 実装完了 ✅ + PC-N-7 design-lock ✅ + **PC-N-7 ✅ 本 commit = Phase 1.D 内 2nd sub-step 実装完了** + PC-N-8 design-lock ⏳ 次 session + PC-N-8..PC-N-10 各 design-lock + 実装 ⏳ + Phase 1.D complete ⏳ + Phase 1.E (multi-asset / multi-skin / worker thread) ⏳

---

## §7. self-verify 9 観点 全 ✅

1. **Exit Criteria 10 項全充足** (= §3 全件 ✅) ✅
2. **必読 1 件 §0 + pinpoint reference 6 件別記** (= PC-N-6 (a) sGltfStubVertexBuffer declare + PC-N-6 (c) initVulkan VMA + PC-N-6 (d) shutdownVulkan + PC-N-6 (e) recordGltfAssetDraw cvar 分岐 + bindIndexBufferVk wrap signature + AYAGltfStubVertexBufferEnabled cvar 配置) ✅
3. **step (a)/(c)/(d)/(e)/(e') 5 site 全実装** ((b) は pipeline 再利用ゆえ作業なし note 明示) ✅
4. **ambiguity (N7-1)..(N7-13) 13 件 AYA literal「すべて推奨でお願いします」record (2026-06-05) design-lock 継承 + 本実装で全件採用案通り実装** ✅
5. **GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件** (= count=6 不変、PC-N-6 commit a68a45f5fd と同数) ✅
6. **MUSEUBO-A 整合 = `AYAGltfStubIndexBufferEnabled=false` default で発火なし + PC-N-6 完了状態と機能等価 + `mUseUBO=false` default で OpenGL 描画 100% 維持 + 5 段 graceful degrade** ✅
7. **build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11/11 + 10/10 + 13/13 + codegen 131/131 全 PASS** ✅
8. **commit 内容 = 1 modified (`llvkloader.cpp` +185) + 1 modified (`settings.xml` +23) + 1 modified (cross-platform spec +2/-1) + 1 new doc (本 complete handoff) + 新 file 0 (除 doc) + CMake 改変 0 + codegen 改変 0 + shader 改変 0 + `.h` 改変 0 + Co-Authored-By 不在** ✅
9. **`feedback_no_scope_shrink` 遵守 = PC-N-7 literal scope 7 件 §0 全件実装、(N7-1) B 採用は AYA literal「すべて推奨でお願いします」record 済段階分離 (= 「PC-N-7 literal scope 最小単位」確定、実 LL::GLTF::Asset 経路は PC-N-8..PC-N-10 持越)、縮小ではなく `feedback_ubo_migration_one_at_a_time` 厳格遵守整合** ✅

---

## §8. 次 session 着手 1 line

**PC-N-8 design-lock 着手** = 実 LL::GLTF::Asset 経由 vertex/index buffer Vulkan infrastructure 新設 + register/write asset.cpp 側 (= Phase 1.D 内 3rd sub-step) = ambiguity 確認 + 実装計画分解 + Exit Criteria 明文化、`feedback_ubo_migration_one_at_a_time` 厳格遵守で本 PC-N-7 stub index buffer 経路を baseline に実 Asset data 取込で拡張。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 PC-N-7 complete handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 必読 1 件 + pinpoint reference 6 件別記、本 session も Read pinpoint のみ (= PC-N-7 design-lock doc 全文 + PC-N-6 (a) sGltfStubVertexBuffer declare + PC-N-6 (c) initVulkan VMA + PC-N-6 (d) shutdownVulkan + PC-N-6 (e) recordGltfAssetDraw cvar 分岐 + bindIndexBufferVk wrap signature + settings.xml AYAGltfStubVertexBufferEnabled cvar 配置)、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §7
- **feedback_build_only_verified** 遵守 = llrender build + WARNING 0 + TUT 11/11 + 10/10 + 13/13 + codegen 131/131 で literal 検証取得 §2
- **feedback_no_scope_shrink** 遵守 = PC-N-7 literal scope 7 件 §0 全件実装、(N7-1) B 採用は「PC-N-7 scope 最小単位 = PC-N-6 stub baseline 拡張」の定義確定ゆえ縮小ではない (= PC-N-6 (N6-1) B precedent 継承)、Phase 1.D 全体 scope は 5 sub-step 分解で完全保持
- **feedback_doubt_self_first** 遵守 = design-lock phase で ambiguity 13 件発見 + 推奨案提示 + AYA literal「すべて推奨でお願いします」受領後本実装、本実装中も PC-N-6 tag block site + bindIndexBufferVk signature + settings.xml 配置 pattern を Read で literal 確認後配線、推測実装なし
- **feedback_confirm_referent_before_acting** 遵守 = 13 件 batch AYA 確認 design-lock phase で完了、本実装中も挿入順「PC-N-6 (e) 直前並列」は design-lock §4.5 literal 確認後採用
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = PC-N-7 = stub index buffer 経路通電単独 sub-step = file-static + pipeline 再利用 + cvar 切替、PC-N-8..PC-N-10 残 3 sub-step は分離
- **feedback_design_phase_no_code_write** 整合 = 本 PC-N-7 は実装 phase = design-lock commit `6a75b61924` で `indra/` 改変 0 件完了済、本 session で `indra/llrender/llvkloader.cpp` + `indra/newview/app_settings/settings.xml` 改変は実装 phase ゆえ整合
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit 予定
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「commit してください」literal 受領待ち
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
- **feedback_no_bare_reference_ids** 遵守 = (N7-1)..(N7-13) 各 ID に項目名 / 採用案内容併記 + (a)..(g) 各 step に作業内容併記
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、git add 個別 file 指定 + `git add -A` 不使用予定
- **memory `project_ayastorm_r41_design_principles`** 整合 = (1) Upstream OpenGL 取り込みやすさ維持 = `recordGltfAssetDraw` signature 不変 + (N7-8) A 新 pipeline 追加なし (再利用) + `sAvatarBonePipeline` 不変温存 + `GLTFSceneManager::render` 改変 0 件 + shader 改変ゼロ + (2) Core プロセス分散実現 = PC-N-8 以降の per-Primitive index buffer ownership design で実現
- **memory `project_r41_phase1b_vulkan_host_gate`** 整合 = GATE-B = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、cvar runtime gate のみ §2.4
- **memory `project_ayastorm_three_platforms`** 整合 = cross-platform spec §6 PC-N-7 行 ✅ 状態更新済 (本 commit 内)、MoltenVK 標準対応範囲 (`VK_INDEX_TYPE_UINT32` + `INDEX_BUFFER_BIT` usage) ゆえ macOS 派生 fix 候補なし + Windows full Vulkan ゆえ派生 fix 候補なし、Linux primary 完成 → 他者補完 model と整合

---
