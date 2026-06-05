# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.D **PC-N-7 design-lock complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: Phase 1.D 内 2nd sub-step (PC-N-7 = stub index buffer Vulkan 経路通電、(N7-1) B 採用 = file-static `sGltfStubIndexBuffer` を VMA buffer 経由 allocate + initial upload + `vkCmdBindIndexBuffer` 配線 + `vkCmdDrawIndexed` 置換 + `sGltfStubAssetPipeline` 再利用 ((N7-8) A、index buffer は Vulkan dynamic state ゆえ pipeline 不変) + `AYAGltfStubIndexBufferEnabled` 段階 cvar) の **design-lock 完了 marker** = ambiguity (N7-1)..(N7-13) 13 件 全 AYA literal「すべて推奨でお願いします」record (2026-06-05) + 実装計画 (a)-(g) 7 step 分解 + Exit Criteria 10 項明文化。`indra/` 改変 0 件 (= `feedback_design_phase_no_code_write` 厳格遵守、`feedback_ubo_migration_one_at_a_time` 厳格遵守)。

> **本 doc 位置付け**: Phase 1.D decomposition design-lock (= `handoff-...-phase1-d-decomposition-design-lock.md`) で確定された PC-N-6..PC-N-10 5 sub-step のうち **2nd sub-step (PC-N-7)** の **詳細実装 design-lock** 起案。Phase 1.D decomposition §4.2 の literal「実 LL::GLTF::Asset 経由」は PC-N-6 (N6-1) B 採用 (= file-static stub minimum unit) との整合で本 PC-N-7 では **stub baseline 拡張 (file-static stub index data) で通電単独 sub-step 化**、実 LL::GLTF::Asset 経由 index data 取込は PC-N-8..PC-N-10 持越し (`feedback_ubo_migration_one_at_a_time` 厳格遵守、PC-N-6 precedent 継承)。PC-N-8..PC-N-10 は別 session で個別 design-lock 起案。

---

## §0. 本 session 着手契機 + literal scope record

**契機**: AYA 指示「r41 Phase 1.D PC-N-7 design-lock 着手お願いします」literal 受領 (2026-06-05、PC-N-6 complete commit `7e90245d10` 後の継続 session = 別 session の fresh context) + 必読 1 件 (PC-N-6 complete handoff doc) Read + pinpoint reference 10 件 Read → ambiguity (N7-1)..(N7-13) 13 件 + 推奨案 + 採用根拠提示 → AYA literal「すべて推奨でお願いします」一括確認受領 (2026-06-05) で本 design-lock doc 起案。

**PC-N-7 literal scope** (= (N7-1) B + (N7-2) A + (N7-3) A + (N7-4) A + (N7-5) A + (N7-6) A + (N7-7) B + (N7-8) A + (N7-9) A + (N7-10) A + (N7-11) B 採用後):

1. **`sGltfStubIndexBuffer` file-static VMA buffer 新設** = LLVKLoader anonymous namespace 内、`VkBuffer + VmaAllocation + void* mapped` 3 件 + count/stride/size const、PC-N-6 `sGltfStubVertexBuffer` 直後並列 file-static lifecycle ((N7-4) A + (N7-6) A)
2. **stub index data initial upload** = `static const U32 sGltfStubIndexData[3] = { 0, 1, 2 }` (= UINT32、3 indices = 12 B、CCW order、PC-N-6 vertex buffer の v0/v1/v2 1-to-1 reference)、`VMA_MEMORY_USAGE_AUTO + VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT + VMA_ALLOCATION_CREATE_MAPPED_BIT` (= 永続 mapped、PC-N-6 同形)、initVulkan で 1 回 memcpy ((N7-3) A + (N7-5) A + (N7-6) A + (N7-7) B)
3. **`sGltfStubAssetPipeline` 再利用** = PC-N-6 で確立済 pipeline をそのまま bind、新 pipeline 不要 (= Vulkan 仕様: `vkCmdBindIndexBuffer` + index type は dynamic state、PSO state ではない) ((N7-8) A)
4. **shutdownVulkan vmaDestroyBuffer** = `sGltfStubIndexBuffer` 対称 lifecycle ((N7-6) A 整合、PC-N-6 (d) tag block 直後並列)
5. **`recordGltfAssetDraw` 改変** = signature 不変、内部で `AYAGltfStubIndexBufferEnabled` cvar 分岐追加、cvar=true 時のみ第 3 stub draw 経路発火 = `vkCmdBindPipeline(sGltfStubAssetPipeline)` (= PC-N-6 同 pipeline) + 既存 UBO bind sequence 維持 + `bindVertexBufferVk(sGltfStubVertexBuffer, 0)` (= PC-N-6 同 vertex buffer) + `bindIndexBufferVk(sGltfStubIndexBuffer, 0, VK_INDEX_TYPE_UINT32)` (= PC-N-7 核心差分) + `vkCmdDrawIndexed(sGltfStubIndexCount, 1, 0, 0, 0)` (= 任意 N、初期値 3) ((N7-2) A + (N7-8) A + (N7-9) A + (N7-10) A + (N7-11) B)
6. **`AYAGltfStubIndexBufferEnabled` cvar 新設** = settings.xml Boolean default false Persist=1、`AYAGltfStubVertexBufferEnabled` 隣に配置 ((N7-10) A)
7. **build verify** = llrender + WARNING 0 + TUT 11+10+13 + codegen 131/131 (Linux primary) ((N7-12) A)

**Phase 1.D 境界**: PC-N-7 = Phase 1.D 2nd sub-step、実 LL::GLTF::Asset 経由 index/vertex data 取込は PC-N-8..PC-N-10 持越し、Phase 1.D complete = PC-N-10 完了時 marker。

**3 stub 経路並走状態 (= PC-N-7 完了後)**: (1) PC-N-5 経路 = `AYAGltfStubDrawEnabled` cvar + `sAvatarBonePipeline` + shader generate 3 vertex (vkCmdDraw(3,1,0,0)) + (2) PC-N-6 経路 = `AYAGltfStubVertexBufferEnabled` cvar + `sGltfStubAssetPipeline` + `sGltfStubVertexBuffer` bind + vkCmdDraw(N,1,0,0) + (3) PC-N-7 経路 = `AYAGltfStubIndexBufferEnabled` cvar + `sGltfStubAssetPipeline` (= PC-N-6 reuse) + `sGltfStubVertexBuffer` (= PC-N-6 reuse) + `sGltfStubIndexBuffer` bind + vkCmdDrawIndexed(M,1,0,0,0) = 3 stub draw 経路独立 cvar gate で並走、PC-N-10 で 3 stub 経路 cleanup ((N7-9) A)。

---

## §1. 必読 1 件 + pinpoint reference

**次 session 必読 (= PC-N-7 実装 phase 着手前)**:

1. **本 PC-N-7 design-lock doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-d-pc-n-7-design-lock.md`

**pinpoint reference (実装時に必要分のみ)**:

- **PC-N-6 complete handoff doc**: `handoff-...-phase1-d-pc-n-6-complete.md` = PC-N-6 実装内容 (a)-(g) + sGltfStubVertexBuffer file-static 配置 site + createGltfStubAssetPipeline 配置 site + initVulkan/shutdownVulkan (c)/(d) tag block + recordGltfAssetDraw (e) cvar 分岐 + AYAGltfStubVertexBufferEnabled cvar 配置 site
- **Phase 1.D decomposition design-lock doc**: `handoff-...-phase1-d-decomposition-design-lock.md` = PC-N-7 scope §4.2 (= 実 LL::GLTF::Asset 経由 index buffer upload + vkCmdBindIndexBuffer + vkCmdDrawIndexed) + 依存関係 §2.3 + (D-3) A 確認 record
- **`recordGltfAssetDraw` 現状**: `indra/llrender/llvkloader.cpp:5751-5849` (PC-N-6 (e) cvar 分岐 + 別 pipeline bind + bindVertexBufferVk + vkCmdDraw(N) 配線済)、PC-N-7 では PC-N-6 (e) 分岐の **直後 or 直前** に PC-N-7 (e) cvar 分岐追加 (= 第 3 stub draw 経路)
- **`sGltfStubVertexBuffer` declare**: `indra/llrender/llvkloader.cpp:603-634` (PC-N-6 (a) tag block)、PC-N-7 で同 anonymous namespace 内直後並列に `sGltfStubIndexBuffer` declare 追加
- **`createGltfStubAssetPipeline` 関数**: `indra/llrender/llvkloader.cpp:3430-3552` (PC-N-6 (b) tag block)、PC-N-7 では **再利用** = 新 pipeline 関数追加なし ((N7-8) A、index buffer は Vulkan dynamic state)
- **initVulkan VMA allocate site**: `indra/llrender/llvkloader.cpp:3982-4047` (PC-N-6 (c) tag block) = sGltfStubVertexBuffer allocate + initial upload + createGltfStubAssetPipeline call、PC-N-7 (c) tag block は本 (c) tag block 直後並列配置 = sGltfStubIndexBuffer VMA allocate + initial upload
- **shutdownVulkan 対称破棄 site**: `indra/llrender/llvkloader.cpp:4366-4382` (PC-N-6 (d) tag block) = vkDestroyPipeline + vmaDestroyBuffer、PC-N-7 (d) tag block は本 (d) tag block 直後並列配置 = sGltfStubIndexBuffer vmaDestroyBuffer
- **`bindIndexBufferVk` wrap**: `indra/llrender/llvkloader.cpp` 内 `bindIndexBufferVk(VkCommandBuffer, VkBuffer, VkDeviceSize offset, VkIndexType)` signature、PC-N-7 が初 caller、`VK_INDEX_TYPE_UINT32` 指定 ((N7-7) B)
- **`AYAGltfStubVertexBufferEnabled` cvar**: `indra/newview/app_settings/settings.xml:10432` 近傍 (PC-N-6 (e') 配置)、PC-N-7 で同 site 直後並列に `AYAGltfStubIndexBufferEnabled` cvar 追加
- **LL::GLTF::Primitive 構造体**: `indra/newview/gltf/primitive.h:58-86` (= `std::vector<U32> mIndexArray` line 66 = UINT32 confirmation = (N7-7) B forward compat 根拠)、**PC-N-7 では参照のみ、改変なし**、PC-N-8..PC-N-10 で実 data source として参照
- **cross-platform spec §6 PC-N-7 行**: `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md:108` (= 本 commit 更新済)
- **GATE-B literal**: memory `project_r41_phase1b_vulkan_host_gate` (= `#ifdef LL_VULKAN_GLSL` 新規追加 0 件維持)
- **設計原則**: memory `project_ayastorm_r41_design_principles` (1) Upstream OpenGL 取り込みやすさ維持 + (2) Core プロセス分散実現

---

## §2. 現状調査結果 (= pinpoint Read + PC-N-6 complete doc 確認)

### §2.1 現 code 状態 (indra/、PC-N-6 commit `7e90245d10` 後)

| # | 項目 | file:line | 現状要約 |
|---|------|-----------|---------|
| 1 | `recordGltfAssetDraw` body | `llvkloader.cpp:5700-5850` | PC-N-6 完了、(e) tag block 内 `AYAGltfStubVertexBufferEnabled` cvar 分岐 + 別 pipeline bind + bindVertexBufferVk + vkCmdDraw(N) 配線済、PC-N-5 経路 (shader generate) も並走、PC-N-7 で第 3 cvar 分岐追加 |
| 2 | `sGltfStubVertexBuffer` file-static | `llvkloader.cpp:603-634` | PC-N-6 完了、VkBuffer + VmaAllocation + void* mapped + count/stride/size const 配置、sGltfStubVertexData[9] CCW triangle hardcoded、PC-N-7 で同 anonymous namespace 内直後並列に sGltfStubIndexBuffer declare 追加 |
| 3 | `sGltfStubAssetPipeline` 関数 | `llvkloader.cpp:3430-3552` (`createGltfStubAssetPipeline`) | PC-N-6 完了、vertex input state 拡張 (1 binding stride=12 + 1 attribute R32G32B32_SFLOAT) 配置済、index buffer は Vulkan dynamic state ゆえ PC-N-7 で再利用 = pipeline 改変なし ((N7-8) A) |
| 4 | initVulkan VMA allocate site | `llvkloader.cpp:3982-4047` | PC-N-6 (c) tag block 完了、sGltfStubVertexBuffer VMA allocate + initial memcpy + createGltfStubAssetPipeline call 配線、PC-N-7 (c) tag block は本 block 直後並列配置 |
| 5 | shutdownVulkan 対称破棄 | `llvkloader.cpp:4366-4382` | PC-N-6 (d) tag block 完了、vkDestroyPipeline + vmaDestroyBuffer 配線、PC-N-7 (d) tag block は本 block 直後並列配置 (= sGltfStubIndexBuffer のみ、pipeline は PC-N-6 再利用ゆえ追加 vkDestroyPipeline なし) |
| 6 | `bindIndexBufferVk` wrap | `llvkloader.cpp` 内既配置 | signature `(VkCommandBuffer, VkBuffer, VkDeviceSize offset, VkIndexType)` 確認済、caller 0 件、PC-N-7 が初 caller、`VK_INDEX_TYPE_UINT32` 指定 ((N7-7) B) |
| 7 | `AYAGltfStubVertexBufferEnabled` cvar | `settings.xml:10432` 近傍 | PC-N-6 (e') 完了、Boolean default false Persist=1、PC-N-7 で同 site 直後並列に `AYAGltfStubIndexBufferEnabled` cvar 追加 |
| 8 | PC-N-5 経路 + PC-N-6 経路 並走 | `llvkloader.cpp:5700-5850` recordGltfAssetDraw | PC-N-5 経路 (shader generate vkCmdDraw(3)) + PC-N-6 経路 (vertex buffer bind + vkCmdDraw(N)) 別 cvar gate で並走済、PC-N-7 経路 (index buffer + vkCmdDrawIndexed) は第 3 並走経路として追加 ((N7-9) A) |

### §2.2 LL::GLTF 構造体 (PC-N-7 では参照のみ、改変なし)

| # | 項目 | file:line | PC-N-7 関連性 |
|---|------|-----------|--------------|
| 9 | `LL::GLTF::Primitive::mIndexArray` | `gltf/primitive.h:66` | `std::vector<U32> mIndexArray` = UINT32 (= (N7-7) B 採用根拠、PC-N-8..PC-N-10 で実 data source として参照時に rework 回避)、PC-N-7 では参照なし |
| 10 | `LL::GLTF::Asset::uploadTransforms` PC-7γ-3 dual-write | `gltf/asset.cpp:164-232` | UBO ONLY (Asset_GLTFNodes/Asset_GLTFMaterials/Skin_GLTFJoints)、vertex/index buffer Vulkan 用 infrastructure 不在、PC-N-7 では参照なし、PC-N-8 で同形 infrastructure 拡張時に参照 |

### §2.3 PC-N-7 内部の step 依存関係

```
(a) sGltfStubIndexBuffer file-static declare + sGltfStubIndexData const
    ↓
(b) (PC-N-8 reuse ゆえ pipeline 関数追加なし、(a) の直後で initVulkan へ進む)
    ↓
(c) initVulkan 内 VMA allocate + initial upload (PC-N-6 (c) tag block 直後並列配置)
    ↓
(d) shutdownVulkan 内 vmaDestroyBuffer 対称配置 (PC-N-6 (d) tag block 直後並列配置)
    ↓
(e) recordGltfAssetDraw 改変 = cvar 分岐 + 同 pipeline bind + bindVertexBufferVk + bindIndexBufferVk + vkCmdDrawIndexed(M) (cvar gate)
    + AYAGltfStubIndexBufferEnabled cvar 新設 (settings.xml + LLCachedControl<bool> 配置)
    ↓
(f) build verify = llrender + warning 0 + TUT 11+10+13 + codegen 131/131
    ↓
(g) handoff complete doc 起案 + Exit Criteria 10 項 self-verify + AYA commit 指示後 commit
```

---

## §3. ambiguity (N7-1)..(N7-13) 13 件 AYA literal「すべて推奨でお願いします」record (2026-06-05) + 採用根拠

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N7-1) | PC-N-7 literal scope 最小単位 (= 実 LL::GLTF::Asset 経路 vs file-static stub) | **B**: file-static stub index data 経路 = PC-N-6 stub vertex buffer baseline 拡張、実 LL::GLTF::Asset 経路は PC-N-8..PC-N-10 持越し | OK (2026-06-05) | PC-N-6 (N6-1) B precedent 継承 = stub pattern minimum unit、`feedback_ubo_migration_one_at_a_time` 厳格遵守、A (= 実 Asset 経路) は data acquisition + ownership lifecycle + register/write infrastructure 全新規ゆえ scope 肥大 (= 5 sub-step 分の作業を 1 sub-step に圧縮)、C (= helper 配置のみ caller 不在) は通電なしで Phase 1.D 各 sub-step 着手起点原則違反、B は PC-N-6 と完全同形 pattern ゆえ verifiable + minimum unit |
| (N7-2) | `recordGltfAssetDraw` signature | **A**: 不変 = `recordGltfAssetDraw(VkCommandBuffer cmd_buf)` | OK (2026-06-05) | (N7-1) B 採用ゆえ stub index data は file-static literal、Asset* 引数不要、PC-N-8 で signature 拡張持越し、PC-N-6 (N6-2) A precedent 整合 |
| (N7-3) | Stub index data source | **A**: file-static `const U32 sGltfStubIndexData[3] = { 0, 1, 2 }` | OK (2026-06-05) | 最小限通電原則、PC-N-6 sGltfStubVertexData[9] CCW triangle (= v0/v1/v2) と 1-to-1 reference、ascending CCW order、vkCmdDrawIndexed(3,1,0,0,0) で実 vertex generate される事実確立 |
| (N7-4) | Index buffer 配置 site | **A**: LLVKLoader anonymous namespace 内 file-static stub = `sGltfStubIndexBuffer` + VMA allocation + lifecycle | OK (2026-06-05) | PC-N-6 sGltfStubVertexBuffer 同 anonymous namespace 内直後並列配置、layering 制約遵守、PC-N-8..PC-N-10 で実 Asset 経路移行時に asset.cpp 側 infrastructure 新設で migration 容易、PC-N-6 (N6-4) B precedent 整合 |
| (N7-5) | VMA usage flag | **A**: PC-N-6 同形 = `VMA_MEMORY_USAGE_AUTO` + `VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT` + `VMA_ALLOCATION_CREATE_MAPPED_BIT` (host-visible 永続 mapped) | OK (2026-06-05) | stub index data 12 B 極小、staging buffer overkill、PC-N-6 (N6-5) B precedent 完全同形 = initVulkan 1 回 memcpy で完結、PC-7β UBO ring buffer 同形 pattern |
| (N7-6) | Index buffer cadence | **A**: 1 回 initVulkan で allocate + initial upload (file-static、shutdown で destroy) | OK (2026-06-05) | stub index data 変更なし固定 literal、ring buffer overkill、PC-N-6 (N6-6) A precedent 完全同形 lifecycle |
| (N7-7) | Index format | **B**: `VK_INDEX_TYPE_UINT32` (= sGltfStubIndexData は U32) | OK (2026-06-05) | LL::GLTF::Primitive::mIndexArray = `std::vector<U32>` (`primitive.h:66`) ゆえ PC-N-8..PC-N-10 で実 Asset 経路移行時に rework 回避 (forward compatibility)、UINT16 (= A) は stub 3 index 範囲では問題ないが将来 mesh で 65535 超 index で rework 必須 = future-proof 選択、Vulkan 仕様: `VK_INDEX_TYPE_UINT32` は core 1.0 必須サポート |
| (N7-8) | `sGltfStubAssetPipeline` 再利用要否 | **A**: 再利用 = PC-N-6 で確立済 pipeline をそのまま bind、新 pipeline 不要 | OK (2026-06-05) | **Vulkan 仕様 (= VkSpec §10.4 Pipeline State)**: `vkCmdBindIndexBuffer` と index type (`VK_INDEX_TYPE_UINT32` 等) は dynamic state、PSO immutable state ではない (= Pipeline State Object に含まれない、command buffer level state)、ゆえ同 pipeline で vkCmdDraw / vkCmdDrawIndexed 両方 issue 可能、別 pipeline 不要 = (N7-1) B 採用と整合、PC-N-6 完了状態を最大活用、graceful degrade 経路も PC-N-6 と共用 |
| (N7-9) | PC-N-6 / PC-N-5 経路との関係 | **A**: 並走維持 = PC-N-5 経路 (shader generate) + PC-N-6 経路 (vertex buffer bind) + PC-N-7 経路 (index buffer + drawIndexed) 3 cvar gate で並走 | OK (2026-06-05) | PC-N-6 (N6-8) A precedent 継承、PC-N-5 sentinel + PC-N-6 vertex buffer lifecycle 不変、PC-N-7 は第 3 stub draw 経路として並走、PC-N-10 で 3 stub 経路 cleanup、`feedback_visual_decisions_need_live_ab` 整合 |
| (N7-10) | `AYAGltfStubIndexBufferEnabled` cvar 新設 timing | **A**: PC-N-7 で新設 = settings.xml Boolean default false Persist=1 | OK (2026-06-05) | PC-N-6 (N6-9) C precedent 継承 = stub 経路毎の独立 cvar gate pattern、PC-N-5 `AYAGltfStubDrawEnabled` + PC-N-6 `AYAGltfStubVertexBufferEnabled` + PC-N-7 `AYAGltfStubIndexBufferEnabled` で 3 独立 gate、PC-N-10 で `AYAGltfRealDrawEnabled` literal 新設 + 3 stub cvar deprecate |
| (N7-11) | vkCmdDrawIndexed 実 index count | **B**: 任意 N (`sGltfStubIndexCount` const、初期値 3) | OK (2026-06-05) | (N7-3) A integer triangle index と整合、PC-N-8..PC-N-10 実 Asset 経路移行時 extensibility、PC-N-6 (N6-10) B precedent 整合 (= sGltfStubVertexCount = 任意 N、初期値 3) |
| (N7-12) | build verify scope | **A**: PC-N-6 同形 = llrender + WARNING 0 + TUT 11+10+13 + codegen 131/131 (Linux primary) | OK (2026-06-05) | PC-N-6 (N6-11) A precedent 継承、PC-8 Linux primary marker 採用後標準、(D-9) A 整合、cold launch literal は AYA 環境依存ゆえ各 sub-step 個別判断 |
| (N7-13) | Exit Criteria + step 分解粒度 | **A**: 10 項 + 7 step (a)-(g) (PC-N-6 同形 template) | OK (2026-06-05) | PC-N-6 (N6-12) A + (N6-13) A precedent 完全同形 template、consistency |

---

## §4. 実装計画 (a)-(g) 7 step ((N7-13) A 採用)

> **注**: 本 §4 は実装 phase 着手用設計、本 design-lock phase では **`indra/` 改変 0 件** (= `feedback_design_phase_no_code_write` 厳格遵守)。

### §4.1 (a) `sGltfStubIndexBuffer` file-static declare + `sGltfStubIndexData` const

**配置 site**: `indra/llrender/llvkloader.cpp` anonymous namespace 内、PC-N-6 (a) tag block (line 603-634 近傍 = `sGltfStubVertexData[9]` const 直後) と直接連続させて配置。

**declare**:

```cpp
// <AYAstorm r41 PC-N-7 (a)> GLTF stub index buffer file-static + VMA allocation。
//   (N7-4) A + (N7-6) A 採用 = LLVKLoader 内 file-static + 1 回 initVulkan 配置 +
//   shutdownVulkan 対称破棄。PC-N-6 sGltfStubVertexBuffer 並列 lifecycle、同 anonymous
//   namespace 内直後配置。
//
//   (N7-3) A 採用 = U32 ascending CCW order { 0, 1, 2 } hardcoded (= 3 indices = 12 B、
//   PC-N-6 sGltfStubVertexData[9] CCW triangle v0/v1/v2 と 1-to-1 reference)。
//   (N7-7) B 採用 = VK_INDEX_TYPE_UINT32 = forward compat with mIndexArray = std::vector<U32>。
VkBuffer       sGltfStubIndexBuffer       = VK_NULL_HANDLE;
VmaAllocation  sGltfStubIndexAllocation   = VK_NULL_HANDLE;
void*          sGltfStubIndexMapped       = nullptr;
constexpr U32  sGltfStubIndexCount        = 3u;  // (N7-11) B 採用 = 任意 N 経路、初期値 3
constexpr U32  sGltfStubIndexStride       = 4u;  // (N7-7) B UINT32 = 4 B per index
constexpr U32  sGltfStubIndexBufferSize   = sGltfStubIndexCount * sGltfStubIndexStride; // 12 B
// </AYAstorm r41 PC-N-7 (a)>
```

**stub index data const**:

```cpp
// <AYAstorm r41 PC-N-7 (a)> stub index data initial literal (= ascending CCW order、
//   sGltfStubVertexData[9] の v0/v1/v2 1-to-1 reference)。
static const U32 sGltfStubIndexData[3] = {
    0u, 1u, 2u,  // ascending CCW = v0 → v1 → v2 (= sGltfStubVertexData CCW triangle order)
};
static_assert(sizeof(sGltfStubIndexData) == sGltfStubIndexBufferSize,
              "PC-N-7 (a) sGltfStubIndexData size mismatch sGltfStubIndexBufferSize");
// </AYAstorm r41 PC-N-7 (a)>
```

### §4.2 (b) pipeline 関数追加なし ((N7-8) A 採用 = sGltfStubAssetPipeline 再利用)

**根拠**: Vulkan 仕様 (= VkSpec §10.4 Pipeline State) で `vkCmdBindIndexBuffer` と index type は dynamic state (= command buffer level state)、PSO immutable state に含まれない。ゆえ PC-N-6 で確立済 `sGltfStubAssetPipeline` を bind したまま、vkCmdDraw (PC-N-6 経路) と vkCmdDrawIndexed (PC-N-7 経路) 両方 issue 可能 = pipeline 関数追加なし、PC-N-6 完了状態を最大活用、graceful degrade 経路 (4 段) も PC-N-6 と共用。

**実装上の作業**: (b) step 自体は **作業なし** (= 既存 sGltfStubAssetPipeline 流用) = 直接 (c) step に進む。

### §4.3 (c) initVulkan VMA allocate + initial upload

**配置 site**: `indra/llrender/llvkloader.cpp` `initVulkan` 内、PC-N-6 (c) tag block (line 3982-4047 近傍) **直後並列**配置 (= sGltfStubVertexBuffer VMA allocate + initial memcpy + createGltfStubAssetPipeline call 直後)。

**code**:

```cpp
// <AYAstorm r41 PC-N-7 (c)> GLTF stub index buffer VMA allocate + initial upload。
//   (N7-5) A 採用 = PC-N-6 同形 host-visible 永続 mapped、(N7-6) A 採用 = 1 回 initVulkan 配置。
//   sAllocator nullptr guard で MUSEUBO-A graceful degrade (= cvar=false default で no-op)。
if (sAllocator != VK_NULL_HANDLE && sGltfStubIndexBuffer == VK_NULL_HANDLE)
{
    VkBufferCreateInfo buf_ci = {};
    buf_ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_ci.size        = sGltfStubIndexBufferSize;
    buf_ci.usage       = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;  // (N7-7) B INDEX_BUFFER usage
    buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_ci = {};
    alloc_ci.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_ci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                   | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo alloc_info = {};
    if (vmaCreateBuffer(sAllocator, &buf_ci, &alloc_ci,
                        &sGltfStubIndexBuffer,
                        &sGltfStubIndexAllocation,
                        &alloc_info) == VK_SUCCESS)
    {
        sGltfStubIndexMapped = alloc_info.pMappedData;
        if (sGltfStubIndexMapped)
        {
            memcpy(sGltfStubIndexMapped, sGltfStubIndexData, sGltfStubIndexBufferSize);
            LL_INFOS("Vulkan") << "PC-N-7 (c) sGltfStubIndexBuffer allocated + initial upload: "
                                  "size=" << sGltfStubIndexBufferSize
                               << " B, indices=" << sGltfStubIndexCount
                               << ", stride=" << sGltfStubIndexStride
                               << " (UINT32)"
                               << ", mapped=" << sGltfStubIndexMapped << LL_ENDL;
        }
        else
        {
            LL_WARNS_ONCE("Vulkan") << "PC-N-7 (c) sGltfStubIndexBuffer mapped=nullptr "
                                       "(VMA host-visible mapped flag fail、cvar=true 時 silent no-op)" << LL_ENDL;
        }
    }
    else
    {
        LL_WARNS_ONCE("Vulkan") << "PC-N-7 (c) sGltfStubIndexBuffer vmaCreateBuffer fail "
                                   "(cvar=true 時 silent no-op)" << LL_ENDL;
    }
}
// </AYAstorm r41 PC-N-7 (c)>
```

### §4.4 (d) shutdownVulkan vmaDestroyBuffer 対称配置

**配置 site**: `indra/llrender/llvkloader.cpp` `shutdownVulkan` 内、PC-N-6 (d) tag block (line 4366-4382 近傍) **直後並列**配置 (= sGltfStubVertexBuffer vmaDestroyBuffer 直後)。

**code**:

```cpp
// <AYAstorm r41 PC-N-7 (d)> GLTF stub index buffer 対称破棄
//   (= (N7-6) A 採用 = 1 回 initVulkan 配置 / 1 回 shutdownVulkan 破棄)。
//   pipeline は PC-N-6 (d) で破棄済 ((N7-8) A 再利用ゆえ追加 vkDestroyPipeline なし)。
if (sGltfStubIndexBuffer != VK_NULL_HANDLE && sAllocator != VK_NULL_HANDLE)
{
    vmaDestroyBuffer(sAllocator, sGltfStubIndexBuffer, sGltfStubIndexAllocation);
    sGltfStubIndexBuffer     = VK_NULL_HANDLE;
    sGltfStubIndexAllocation = VK_NULL_HANDLE;
    sGltfStubIndexMapped     = nullptr;
}
// </AYAstorm r41 PC-N-7 (d)>
```

### §4.5 (e) `recordGltfAssetDraw` 改変 + cvar gate 配線

**配置 site**: `indra/llrender/llvkloader.cpp:5700-5850` `recordGltfAssetDraw` 関数 body 内、PC-N-6 (e) tag block (= `AYAGltfStubVertexBufferEnabled` cvar 分岐) **直後並列**配置 = 第 3 cvar 分岐挿入。

**signature**: 不変 ((N7-2) A) = `void recordGltfAssetDraw(VkCommandBuffer cmd_buf)`。

**改変方針** ((N7-9) A 並走維持):

```cpp
// PC-N-5 既存 guard (line 5499-5506 近傍) 維持。
if (cmd_buf == VK_NULL_HANDLE || ...) { return; }

// PC-N-6 (e) cvar 分岐 (= AYAGltfStubVertexBufferEnabled) 維持、PC-N-6 経路は不変。

// <AYAstorm r41 PC-N-7 (e)> stub index buffer 経路の cvar 分岐
//   ((N7-9) A 並走維持 + (N7-10) A `AYAGltfStubIndexBufferEnabled` 段階 cvar)。
//   cvar=true 時のみ第 3 stub draw 経路発火 = sGltfStubAssetPipeline 再利用 +
//   sGltfStubVertexBuffer bind + sGltfStubIndexBuffer bind + vkCmdDrawIndexed(M,1,0,0,0)、
//   cvar=false default 時は PC-N-6 経路 (cvar=false 時は PC-N-5 経路) 継続 =
//   MUSEUBO-A 整合 + live A/B 経路独立。
static LLCachedControl<bool> gltf_stub_ib_enabled(
    gSavedSettings, "AYAGltfStubIndexBufferEnabled", false);

if (gltf_stub_ib_enabled
    && sGltfStubAssetPipeline != VK_NULL_HANDLE
    && sGltfStubVertexBuffer  != VK_NULL_HANDLE
    && sGltfStubIndexBuffer   != VK_NULL_HANDLE)
{
    // PC-N-7 (e) 同 pipeline bind + vertex buffer bind + index buffer bind + drawIndexed 経路。
    //   ((N7-8) A) sGltfStubAssetPipeline 再利用 = Vulkan 仕様で index buffer は dynamic state ゆえ
    //   PC-N-6 と同 PSO で vkCmdDrawIndexed issue 可能。UBO bind sequence は PC-N-6 / PC-N-5 と
    //   完全同形 (= writeDrawUbo + writeSkinUbo + flushSkinUbos + bindV3aRigged)、
    //   bindIndexBufferVk + vkCmdDrawIndexed 差分のみ。
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, sGltfStubAssetPipeline);

    // PC-N-6 / PC-N-5 と同形 per-draw UBO 配線 (= line 5510-5522 同形 copy)。
    static const U8 stub_draw_zero_buf[256] = {};
    U32 stub_dynamic_offset = 0u;
    LLVKLoader::writeDrawUbo(
        ubo::block_hash::PerDrawUBO_LightParams,
        /*offset=*/0u,
        stub_draw_zero_buf,
        sizeof(stub_draw_zero_buf),
        stub_dynamic_offset);
    const U32 stub_dynamic_offsets[V3A_DRAW_SET_BINDINGS] = {
        stub_dynamic_offset, stub_dynamic_offset, stub_dynamic_offset, stub_dynamic_offset,
    };

    // PC-N-6 / PC-N-5 と同形 per-Skin UBO 配線 (= line 5524-5552 同形 copy)。
    static const F32 stub_identity_skin_buf[64] = { /* identity mat4 + zero padding 256 B */ };
    LLVKLoader::writeSkinUbo(
        sGltfStubSkin,
        ubo::block_hash::Skin_GLTFJoints,
        /*offset=*/0u,
        reinterpret_cast<const U8*>(stub_identity_skin_buf),
        sizeof(stub_identity_skin_buf));
    LLVKLoader::flushSkinUbos(sGltfStubSkin);
    bindV3aRigged(cmd_buf, sFrameIndex, stub_dynamic_offsets);

    // PC-N-6 / PC-N-5 と同形 push constant (= line 5556-5569 同形 copy)。
    const float stub_identity_modelview[16] = { /* identity */ };
    vkCmdPushConstants(cmd_buf, sAvatarBoneLayout, VK_SHADER_STAGE_VERTEX_BIT,
                       /*offset=*/0, /*size=*/64, stub_identity_modelview);

    // PC-N-7 (e) vertex buffer bind + index buffer bind + vkCmdDrawIndexed (= PC-N-7 核心差分)。
    LLVKLoader::bindVertexBufferVk(cmd_buf, sGltfStubVertexBuffer, /*offset=*/0);
    LLVKLoader::bindIndexBufferVk(cmd_buf, sGltfStubIndexBuffer, /*offset=*/0,
                                  VK_INDEX_TYPE_UINT32);  // (N7-7) B UINT32
    vkCmdDrawIndexed(cmd_buf, sGltfStubIndexCount, 1, 0, 0, 0);

    // PC-N-7 (e) first-fire LL_INFOS marker。
    static std::atomic<bool> s_first_pcn7_fire{true};
    if (s_first_pcn7_fire.exchange(false, std::memory_order_acq_rel))
    {
        LL_INFOS("Vulkan") << "PC-N-7 (e) GLTF stub index buffer draw 通電 (first fire): "
                              "sGltfStubAssetPipeline (= PC-N-6 reuse) + sGltfStubVertexBuffer (= PC-N-6 reuse) + "
                              "sGltfStubIndexBuffer (= UINT32 ascending CCW { 0, 1, 2 }、12 B、VMA host-visible mapped) = "
                              "bindV3aRigged → bindVertexBufferVk → bindIndexBufferVk(UINT32) → vkCmdDrawIndexed("
                           << sGltfStubIndexCount << ",1,0,0,0)、sAvatarBonePipeline 並走温存、PC-N-5/PC-N-6 経路並走" << LL_ENDL;
    }
    return; // PC-N-6 / PC-N-5 経路はスキップ = 別 cvar gate で 3 並走分離
}
// </AYAstorm r41 PC-N-7 (e)>

// PC-N-6 (e) 既存 cvar 分岐 (= AYAGltfStubVertexBufferEnabled) 維持、line 5751-5849 不変。
// PC-N-5 (b) 既存経路 (= sAvatarBonePipeline + shader generate 3 vertex) 不変。
```

> **挿入順 note**: PC-N-7 (e) 分岐は PC-N-6 (e) 分岐 **直前**配置 = (PC-N-7 cvar=true → PC-N-6 cvar 評価せず PC-N-7 経路 fire) で 3 cvar 優先順位確定。PC-N-10 cleanup で全 stub cvar deprecate 時に削除順序明確化。

### §4.6 (e') `AYAGltfStubIndexBufferEnabled` cvar 新設 (settings.xml)

**配置 site**: `indra/newview/app_settings/settings.xml`、`AYAGltfStubVertexBufferEnabled` (PC-N-6 (e')、line 10432 近傍) 直後並列配置。

**XML**:

```xml
<key>AYAGltfStubIndexBufferEnabled</key>
<map>
  <key>Comment</key>
  <string>r41 PC-N-7 stub GLTF asset index buffer Vulkan draw 経路発火切替 (= AYAGltfStubDrawEnabled + AYAGltfStubVertexBufferEnabled と並走の第 3 stub 経路、PC-N-6 sGltfStubAssetPipeline + sGltfStubVertexBuffer 再利用 + 新 sGltfStubIndexBuffer (UINT32) 経由 vkCmdDrawIndexed、live A/B、PC-N-10 で deprecate 予定)</string>
  <key>Persist</key>
  <integer>1</integer>
  <key>Type</key>
  <string>Boolean</string>
  <key>Value</key>
  <integer>0</integer>
</map>
```

### §4.7 (f) build verify ((N7-12) A 採用)

実装 phase 末尾で取得予定:

- `make -j4 llrender` PASS
- ERROR 0 / WARNING 0
- `INTEGRATION_TEST_lluboringbuffer` 11/11
- `INTEGRATION_TEST_llassetubopool` 10/10
- `INTEGRATION_TEST_llpipelinecachestorage` 13/13
- codegen unittest 131/131

### §4.8 (g) handoff complete doc 起案

実装 phase 末尾で `handoff-...-phase1-d-pc-n-7-complete.md` 起案 + Exit Criteria 10 項 self-verify + AYA commit 指示後 commit。

### §4.9 GATE-B 整合 (memory `project_r41_phase1b_vulkan_host_gate`)

**`#ifdef LL_VULKAN_GLSL` 新規追加 0 件** = `AYAGltfStubIndexBufferEnabled` は `LLCachedControl<bool>` runtime cvar 経路ゆえ `#ifdef` 非依存 = GATE-B 違反なし。新 VMA buffer + 新 cvar 全 runtime gate のみ、新 pipeline 追加なし ((N7-8) A 再利用) ゆえ shader 改変も 0 件。

### §4.10 MUSEUBO-A 整合

`mUseUBO=false` default で既存 OpenGL 描画 100% 維持。`AYAGltfStubIndexBufferEnabled=false` default で本 PC-N-7 新経路発火なし = PC-N-6 完了状態 (= PC-N-5 + PC-N-6 stub draw 経路 cvar 経由) と機能等価。多段 graceful degrade:

1. `sAllocator nullptr` (= Vulkan 未初期化) → VMA allocate 自体スキップ
2. `sGltfStubIndexBuffer == VK_NULL_HANDLE` (= allocate fail) → cvar=true でも分岐内 nullptr guard で silent fall-through
3. `sGltfStubVertexBuffer == VK_NULL_HANDLE` (= PC-N-6 (c) fail 持越し) → 同上
4. `sGltfStubAssetPipeline == VK_NULL_HANDLE` (= PC-N-6 (b) compile fail) → 同上
5. `sGltfStubIndexMapped == nullptr` (= mapped fail) → memcpy スキップ + LL_WARNS_ONCE、bind は valid buffer ゆえ vkCmdDrawIndexed 自体は success (= zero-content draw)

### §4.11 設計原則整合 (memory `project_ayastorm_r41_design_principles`)

- **(1) Upstream OpenGL 取り込みやすさ維持** = `recordGltfAssetDraw` signature 不変 + 新 pipeline 追加なし ((N7-8) A 再利用) + `sAvatarBonePipeline` 不変 + shader 改変ゼロ + `GLTFSceneManager::render` 改変 0 件 = upstream LL からの取り込み時に call site 不変
- **(2) Core プロセス分散実現** = stub index buffer は file-static の 1 回配置ゆえ本 sub-step では並列化 baseline 拡張なし、PC-N-8 以降の per-Primitive index buffer ownership design で実現

---

## §5. PC-N-7 design-lock Exit Criteria 9 項

| # | Criteria |
|---|----------|
| (i) | PC-N-7 literal scope §0 明文化 ((N7-1) B + (N7-3) A + (N7-4) A + (N7-7) B + (N7-8) A + (N7-10) A 採用後の 7 件) |
| (ii) | 必読 1 件 §1 + pinpoint reference 13 件別記 |
| (iii) | ambiguity (N7-1)..(N7-13) 13 件 AYA literal「すべて推奨でお願いします」record (2026-06-05) §3 |
| (iv) | 採用根拠 13 件 §3 明文化 |
| (v) | 実装計画 (a)-(g) 7 step 分解 §4 + 各 step に具体 code stub example 添付 |
| (vi) | 実装 phase Exit Criteria 10 項 §6 明文化 |
| (vii) | GATE-B 整合 §4.9 + MUSEUBO-A 整合 §4.10 + 設計原則整合 §4.11 |
| (viii) | 想定改変 file 3 件 §6.2 明文化 |
| (ix) | `indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合 (cross-platform spec §6 PC-N-7 行追記のみ) |

---

## §6. 実装 phase Exit Criteria 10 項 ((N7-13) A 採用)

| # | Criteria |
|---|----------|
| (i) | `sGltfStubIndexBuffer` file-static + VMA allocation declare 配置 + `sGltfStubIndexData[3]` const 配置 (PC-N-7 (a) tag block) |
| (ii) | initVulkan VMA allocate + initial upload (= memcpy `sGltfStubIndexData` to mapped) 配線 (PC-N-7 (c) tag block) |
| (iii) | `sGltfStubAssetPipeline` 再利用 ((N7-8) A) = 新 pipeline 関数追加 0 件、PC-N-6 で確立済 PSO に直接 vkCmdDrawIndexed issue |
| (iv) | shutdownVulkan 内 `vmaDestroyBuffer` 対称破棄配線 (PC-N-7 (d) tag block) = pipeline は PC-N-6 (d) で破棄済ゆえ追加 `vkDestroyPipeline` なし |
| (v) | `recordGltfAssetDraw` 内 `AYAGltfStubIndexBufferEnabled` cvar 分岐 + 同 pipeline bind + `bindVertexBufferVk` + `bindIndexBufferVk(UINT32)` + `vkCmdDrawIndexed(M)` 配線 (PC-N-7 (e) tag block、PC-N-6 (e) 分岐直前並列) |
| (vi) | `AYAGltfStubIndexBufferEnabled` cvar 新設 (settings.xml) + LLCachedControl<bool> default false Persist=1 |
| (vii) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= cvar runtime gate のみ) |
| (viii) | MUSEUBO-A 整合 = `mUseUBO=false` default + `AYAGltfStubIndexBufferEnabled=false` default 経路不変、5 段 graceful degrade (= sAllocator / sGltfStubIndexBuffer / sGltfStubVertexBuffer / sGltfStubAssetPipeline / sGltfStubIndexMapped 各 nullptr guard) |
| (ix) | build verify llrender + WARNING 0 + TUT 11+10+13 + codegen 131/131 全 PASS ((N7-12) A) |
| (x) | tag block 統一 PC-N-7 (a)/(c)/(d)/(e) + LL_INFOS first-fire marker PC-N-7 (e) 通電 literal 追加 + handoff complete doc 起案 |

### §6.1 GATE-B / MUSEUBO-A integrity check (実装 phase 末尾検証項)

- `git grep "LL_VULKAN_GLSL" indra/` 件数が PC-N-6 commit (`7e90245d10`) 時点と同数 = GATE-B 違反なし
- `AYAGltfStubIndexBufferEnabled=false` default で `recordGltfAssetDraw` 経路は PC-N-6 完了状態と機能等価 = MUSEUBO-A 整合
- 5 段 graceful degrade を Vulkan 未初期化条件下で各個 trip させて silent no-op 確認 (= cold launch + `AYAGltfStubIndexBufferEnabled=true` でクラッシュなし)

### §6.2 想定改変 file 3 件 (= 実装 phase = 別 session)

1. `indra/llrender/llvkloader.cpp` (+95〜120 / -0 推定) = (a) sGltfStubIndexBuffer file-static declare + sGltfStubIndexData const + (c) initVulkan VMA allocate + initial upload + (d) shutdownVulkan vmaDestroyBuffer + (e) recordGltfAssetDraw cvar 分岐 + 同 pipeline bind + bindVertexBufferVk + bindIndexBufferVk + vkCmdDrawIndexed(M) = 4 編集 step、新 pipeline 関数追加 0 件 ((N7-8) A 再利用)
2. `indra/newview/app_settings/settings.xml` (+19 / -0 推定) = AYAGltfStubIndexBufferEnabled Boolean cvar 1 件追加 ((N7-10) A)
3. `indra/llrender/llvkloader.h` 改変 0 件想定 (= 新 declare 全 anonymous namespace 内、register/unregister API signature 変更なし、bindIndexBufferVk 既配置)

### §6.3 想定 build verify command (= 実装 phase 末尾、Linux primary)

```
make -j4 llrender
ctest -R INTEGRATION_TEST_lluboringbuffer    # 11/11 PASS 確認
ctest -R INTEGRATION_TEST_llassetubopool     # 10/10 PASS 確認
ctest -R INTEGRATION_TEST_llpipelinecachestorage  # 13/13 PASS 確認
ctest -R codegen   # 131/131 PASS 確認
```

---

## §7. 着手手順 (= 次 session で PC-N-7 実装 phase 着手)

1. AYA 指示「PC-N-7 実装着手お願いします」literal 受領待ち
2. 本 PC-N-7 design-lock doc 全文 Read (= 必読 1 件)
3. pinpoint Read = PC-N-6 (a) sGltfStubVertexBuffer declare 近傍 (603-634) + PC-N-6 (c) initVulkan VMA allocate (3982-4047) + PC-N-6 (d) shutdownVulkan (4366-4382) + PC-N-6 (e) recordGltfAssetDraw cvar 分岐 (5751-5849) + bindIndexBufferVk wrap signature + settings.xml `AYAGltfStubVertexBufferEnabled` 配置近傍
4. step (a) → (c) → (d) → (e) → (e') 順で実装 (= §4 stub example を base に、PC-N-6 同形 tag block style 踏襲、indra/llrender/llvkloader.cpp + settings.xml 2 file 改変、(b) は再利用ゆえ作業なし)
5. build verify literal 取得 (= §6.3 command 実行 + 各 PASS 数 record)
6. Exit Criteria 10 項 §6 self-verify + GATE-B / MUSEUBO-A integrity check §6.1
7. handoff complete doc 起案 (= PC-N-6 complete 同形 template) + cross-platform spec §6 PC-N-7 状態 ✅ 反映 + §A 更新履歴追記
8. AYA commit 指示後 commit (= feature branch `feature/ayastorm-r41-gl-removal` 上、`indra/llrender/llvkloader.cpp` + `indra/newview/app_settings/settings.xml` + 新 doc 1 件 + cross-platform spec 1 件改変 = 4 件 git add 個別指定、`git add -A` 不使用)

---

## §8. 残 strict 線形

PC-N-7 design-lock ✅ 本 commit → PC-N-7 実装 (= 次 session 着手、step (a)/(c)/(d)/(e)/(e')/(f)/(g) 実施 + Exit Criteria 10 項 self-verify + handoff complete doc 起案) → PC-N-8 design-lock (= 実 LL::GLTF::Asset 経由 vertex/index buffer Vulkan infrastructure 新設 + register/write asset.cpp 側、別 session で個別 design-lock) → PC-N-8 実装 → PC-N-9 design-lock + 実装 (= GLTFSceneManager::render 統合 + `AYAGltfRealDrawEnabled` cvar gate) → PC-N-10 design-lock + 実装 (= cleanup + 3 stub cvar deprecate) → **Phase 1.D complete** → Phase 1 全完了 → Mac/Win 開発者補完 phase。

---

## §9. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1 design-lock ✅ + PC-N-1 ✅ + PC-N-2 design-lock ✅ + PC-N-2 ✅ + PC-N-4 design-lock ✅ + PC-N-4 ✅ + PC-N-3 design-lock ✅ + PC-N-3 ✅ = Phase 1.C complete ✅ + PC-8 Linux primary marker ✅ = Phase 1.C strict 線形終了 ✅ + PC-N-5 design-lock ✅ + PC-N-5 ✅ = Phase 1.D 着手起点 実装完了 ✅ + Phase 1.D decomposition design-lock ✅ + PC-N-6 design-lock ✅ + PC-N-6 ✅ = Phase 1.D 内 1st sub-step 実装完了 ✅ + **PC-N-7 design-lock ✅ 本 commit** + PC-N-7 実装 ⏳ 次 session + PC-N-8..PC-N-10 各 design-lock + 実装 ⏳ = Phase 1.D complete ⏳ + Phase 1.E (multi-asset / multi-skin / worker thread) ⏳

---

## §10. self-verify 9 観点 全 ✅

1. **PC-N-7 literal scope §0 完全分解 7 件** = (a) sGltfStubIndexBuffer file-static + (b) 再利用ゆえ pipeline 関数追加なし + (c) initVulkan VMA allocate + initial upload + (d) shutdownVulkan 対称破棄 + (e) recordGltfAssetDraw cvar 分岐 + (e') AYAGltfStubIndexBufferEnabled cvar 新設 + (f)/(g) build verify + handoff ✅
2. **必読 1 件 §1 + pinpoint reference 13 件別記** = PC-N-6 complete + Phase 1.D decomposition + recordGltfAssetDraw + sGltfStubVertexBuffer declare + createGltfStubAssetPipeline + initVulkan VMA site + shutdownVulkan 対称破棄 site + bindIndexBufferVk wrap + AYAGltfStubVertexBufferEnabled cvar + Primitive 構造体 + cross-platform spec + GATE-B literal + 設計原則 ✅
3. **現状調査 §2 10 項網羅** = code 8 項 (recordGltfAssetDraw + sGltfStubVertexBuffer + sGltfStubAssetPipeline + initVulkan + shutdownVulkan + bindIndexBufferVk + cvar + 3 経路並走状態) + LL::GLTF 2 項 + step 依存関係 ✅
4. **ambiguity (N7-1)..(N7-13) 13 件 AYA literal「すべて推奨でお願いします」record (2026-06-05) §3** ✅
5. **採用根拠 13 件明文化 §3** ✅
6. **実装計画 (a)-(g) 7 step §4 分解 + 各 step に具体 code stub example 添付** ((b) は再利用ゆえ作業なし note 明示) ✅
7. **GATE-B 整合 §4.9 (= `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、cvar runtime gate のみ) + MUSEUBO-A 整合 §4.10 (= `mUseUBO=false` + `AYAGltfStubIndexBufferEnabled=false` default 経路不変、5 段 graceful degrade) + 設計原則整合 §4.11 (= (1) call site 不変 + shader 改変ゼロ + sGltfStubAssetPipeline 再利用 + (2) PC-N-8 以降の per-Primitive ownership design 余地確保)** ✅
8. **design-lock Exit Criteria 9 項 §5 + 実装 phase Exit Criteria 10 項 §6 + 想定改変 file 3 件 §6.2 明文化** ✅
9. **`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合 (= cross-platform spec §6 PC-N-7 行追記 + 本 design-lock doc 1 件起案のみ)** ✅

---

## §11. 次 session 着手 1 line

**PC-N-7 実装着手** = step (a)/(c)/(d)/(e)/(e')/(f)/(g) 実施 = (a) `sGltfStubIndexBuffer` file-static + `sGltfStubIndexData[3]` const 配置 (PC-N-6 (a) tag block 直後並列) + (b) 作業なし (= sGltfStubAssetPipeline 再利用 (N7-8) A) + (c) initVulkan VMA allocate (host-visible mapped) + initial memcpy (PC-N-6 (c) tag block 直後並列配置) + (d) shutdownVulkan vmaDestroyBuffer 対称配置 (PC-N-6 (d) tag block 直後並列、pipeline は再利用ゆえ追加 vkDestroyPipeline なし) + (e) `recordGltfAssetDraw` 内 `AYAGltfStubIndexBufferEnabled` cvar 分岐 + 同 pipeline bind + `bindVertexBufferVk` + `bindIndexBufferVk(UINT32)` + `vkCmdDrawIndexed(M)` (PC-N-6 (e) 分岐直前並列配置) + (e') settings.xml cvar 1 件追加 + (f) build verify (= llrender + warning 0 + TUT 11+10+13 + codegen 131/131) + (g) handoff complete doc 起案 + Exit Criteria 10 項 self-verify + AYA commit 指示受領後 commit。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 PC-N-7 design-lock handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 次 session 必読 1 件 + pinpoint reference 13 件別記、本 session も Read pinpoint のみ (= PC-N-6 complete doc 全文 + Phase 1.D decomposition design-lock doc 全文 + PC-N-6 design-lock doc 全文 (template) + recordGltfAssetDraw 現状 + asset.cpp uploadTransforms 確認 + cross-platform spec §6)、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §10
- **feedback_build_only_verified** 遵守 = design-lock phase は `indra/` 改変 0 件で build verify 対象外、実装 phase で literal 検証取得予定 ((N7-12) A 採用)
- **feedback_no_scope_shrink** 遵守 = PC-N-7 literal scope §0 完全分解 7 件 ((N7-1) B 採用は「PC-N-7 scope 最小単位 = PC-N-6 stub baseline 拡張」の定義確定ゆえ縮小ではない = PC-N-6 (N6-1) B precedent 継承 + Phase 1.D decomposition §4.2 literal「実 LL::GLTF::Asset 経由」は `feedback_ubo_migration_one_at_a_time` 厳格遵守整合で PC-N-8..PC-N-10 段階分離、Phase 1.D 全体 scope は 5 sub-step 分解で完全保持)
- **feedback_doubt_self_first** 遵守 = ambiguity 13 件発見で停止 + 推奨案提示 + AYA literal「すべて推奨でお願いします」一括確認後本 design-lock doc 起案、推測実装なし、特に (N7-1) PC-N-7 scope literal 解釈 (= 実 Asset 経由 vs file-static stub) は 3 候補全列挙 + Phase 1.D decomposition §4.2 literal との tension 明示 + PC-N-6 precedent 根拠提示後 AYA 確認、(N7-8) 別 pipeline 要否は Vulkan 仕様 dynamic state 根拠明示
- **feedback_confirm_referent_before_acting** 遵守 = 13 件 batch AYA 確認 (2026-06-05)、各候補 + 推奨案 + 根拠明示後 AYA literal 一括「すべて推奨でお願いします」record 受領で確定、推測実装なし
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = PC-N-7 = stub index buffer 経路通電単独 sub-step = file-static + pipeline 再利用 + cvar 切替、PC-N-8..PC-N-10 残 3 sub-step は分離 (= 実 LL::GLTF::Asset 経由 vertex/index buffer infrastructure 新設は PC-N-8 で別 design-lock)、本 doc 起案も PC-N-7 単独 design-lock のみ
- **feedback_design_phase_no_code_write** 厳格遵守 = 本 PC-N-7 design-lock phase は doc 起案のみ、`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 (= cross-platform spec §6 PC-N-7 行追記 + 本 design-lock doc 1 件起案のみ)
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「commit してください」literal 受領待ち
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
- **feedback_no_bare_reference_ids** 遵守 = (N7-1)..(N7-13) 各 ID に項目名 / 採用案内容併記 §3 + (a)..(g) 各 step に作業内容併記 §4
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、git add 個別 file 指定予定
- **memory `project_ayastorm_r41_design_principles`** 整合 = (1) Upstream OpenGL 取り込みやすさ維持 = (N7-2) A signature 不変 + (N7-8) A 新 pipeline 追加なし (再利用) + sAvatarBonePipeline 不変温存 + GLTFSceneManager::render 改変 0 件 + shader 改変ゼロ §4.11 + (2) Core プロセス分散実現 = PC-N-8 以降の per-Primitive index buffer ownership design で実現 §4.11
- **memory `project_r41_phase1b_vulkan_host_gate`** 整合 = GATE-B = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、cvar runtime gate のみ §4.9
- **memory `project_ayastorm_three_platforms`** 整合 = cross-platform spec §6 PC-N-7 行追記で macOS / Windows 派生 fix 候補欄起案 (= VK_INDEX_TYPE_UINT32 は MoltenVK 標準対応 + INDEX_BUFFER_BIT usage は MoltenVK 標準対応 ゆえ派生 fix 候補なし)、Linux primary 完成 → 他者補完 model 整合

---
