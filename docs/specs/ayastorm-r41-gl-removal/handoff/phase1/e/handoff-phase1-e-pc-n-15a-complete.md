# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.E PC-N-15a complete

**Status**: ✅ **PC-N-15a complete = Phase 1.E 内 5th sub-step 実装完了 = worker
thread infrastructure 配線 = LLUboRingBuffer per-thread instance refactor +
`thread_local` 4 件 + per-thread `VkCommandPool` + secondary `VkCommandBuffer`
allocate + `AYAGltfWorkerThreadEnabled` + `AYAGltfWorkerThreadCount` 2 cvar 新設
+ `initVulkan`/`shutdownVulkan` lifecycle 配線**

**Date**: 2026-06-05
**Branch**: `feature/ayastorm-r41-gl-removal`
**Previous commit**: `631794883d` (PC-N-15a design-lock complete)

---

## §0. PC-N-15a literal scope 実装結果 (= 全件採用案通り実装完了)

PC-N-15a = **Phase 1.E 内 5th sub-step = worker thread infrastructure 配線 =
PC-N-14 design-lock worker thread design ((N14-1) ⭐ A secondary cmdbuf +
`vkCmdExecuteCommands` 集約 pattern) の infrastructure phase**。

PC-N-15 単一 sub-step が現状調査で scope 過大判定 → AYA literal「Bでいきましょう」
record (2026-06-05) で PC-N-15a/b/c 3 sub-phase 分解、本 PC-N-15a は infrastructure
配線 phase ((N15a-1) ⭐ A 採用)。

literal scope 5 項全件実装完了 ((N15a-1)..(N15a-13) 13 件 AYA literal「OK」
record (2026-06-05) 全件採用案通り):

1. ✅ `AYAGltfWorkerThreadEnabled` Boolean + `AYAGltfWorkerThreadCount` U32
   2 cvar 新設 = settings.xml `AYAGltfMultiAssetCanary` 直後並列 ((N15a-7) A
   + (N15a-8) A `U32 default=0 auto detect` + (N15a-9) A 配置)
2. ✅ `thread_local` 化 4 件 = `sCurrentAsset` + `sCurrentSkin` +
   `sCurrentPrimitive` + `sCurrentNodeAssetMatrix` 全 `thread_local` 修飾子
   追加 (= accessor signature 不変、layering 制約完全充足、PC-N-12 Option A
   pivot 教訓踏襲、(N15a-3) A 採用)
3. ✅ per-thread storage 配列新設 = `sWorkerCommandPools`
   `std::vector<VkCommandPool>` + `sWorkerSecondaryCommandBuffers`
   `std::vector<VkCommandBuffer>` + `sWorkerDrawUboRingBuffers`
   `std::vector<std::unique_ptr<LLUboRingBuffer>>` + `sPcn13MultiAssetSeenMutex`
   `std::mutex` ((N15a-2) ⭐ A + (N15a-4) A + (N15a-5) A + (N15a-6) A 採用)
4. ✅ `createWorkerThreadInfra()` + `destroyWorkerThreadInfra()` helper 新設
   = 起動 = cvar guard + worker thread 数決定 (`hardware_concurrency()` auto
   detect or cvar fixed) + per-thread `VkCommandPool` + secondary
   `VkCommandBuffer` + per-thread `LLUboRingBuffer` instance 生成、teardown =
   reverse-init order cleanup ((N15a-10) A LLUboRingBuffer instance 数 = worker
   thread 数 + main thread 1)
5. ✅ `initVulkan`/`shutdownVulkan` lifecycle 配線 = `createDrawUboRingBuffer`
   直後 `createWorkerThreadInfra()` 呼出 + 主 `vkDestroyCommandPool` 直後
   `destroyWorkerThreadInfra()` 呼出 (= sAllocator + sDevice alive 期間内 for
   `vmaCreateBuffer` / `vkDestroyCommandPool` / `vkFreeCommandBuffers`)

---

## §1. 実装結果 = 5 step (a)-(e) 全実装

### §1.1 step (a) settings.xml `AYAGltfWorkerThreadEnabled` + `AYAGltfWorkerThreadCount` 2 cvar 追加 ((N15a-7)/(N15a-8)/(N15a-9) A)

`indra/newview/app_settings/settings.xml`:

- 配置: `AYAGltfMultiAssetCanary` 直後並列 = Phase 1.E cvar group 末尾
  (PC-N-11 `AYAGltfMultiSkinEnabled` → PC-N-12 `AYAGltfRealModelviewEnabled` →
  PC-N-13 `AYAGltfRealLightParamsEnabled` + `AYAGltfMultiAssetCanary` →
  PC-N-15a `AYAGltfWorkerThreadEnabled` + `AYAGltfWorkerThreadCount` の順)
- `AYAGltfWorkerThreadEnabled`: `Type=Boolean` + `Value=0` (= default OFF) +
  `Persist=1` + 単独 Comment (= MUSEUBO-A 整合 = OpenGL 描画 100% 維持)
- `AYAGltfWorkerThreadCount`: `Type=U32` + `Value=0` (= default auto detect via
  `std::thread::hardware_concurrency()` cap 4 最小 1) + `Persist=1` + 単独 Comment
- 包括 `<!-- <AYAstorm r41 PC-N-15a (a)> ... </AYAstorm r41 PC-N-15a (a)> -->`
  tag block で 2 cvar 一括 wrap (= PC-N-13 同形 surgical insertion)

### §1.2 step (b) `llvkloader.cpp` include 追加 + anonymous namespace 内 per-thread storage 配列

`indra/llrender/llvkloader.cpp`:

- 冒頭 include block に `<AYAstorm r41 PC-N-15a (a)>` tag block で
  `#include <thread>` + `#include <mutex>` + `#include <vector>` +
  `#include <memory>` (= worker thread 数決定 + `std::mutex` + 配列 +
  `std::unique_ptr<LLUboRingBuffer>` 用)
- anonymous namespace 内 `sPcn13MultiAssetSeen` 直後に
  `<AYAstorm r41 PC-N-15a (c)> sPcn13MultiAssetSeen mutex 保護` tag block で
  `std::mutex sPcn13MultiAssetSeenMutex` 追加 ((N15a-4) A 採用 = worker thread
  別々の「初回 asset」誤認識を防ぐため、main + worker access を mutex 保護で
  serialize して `size() > 1u` 検出 semantic を維持)
- anonymous namespace 内 `<AYAstorm r41 PC-N-15a (a)> worker thread
  infrastructure storage` tag block で 3 配列 + 1 mutex を新設:
  - `std::vector<VkCommandPool> sWorkerCommandPools` ((N15a-5) A)
  - `std::vector<VkCommandBuffer> sWorkerSecondaryCommandBuffers` ((N15a-6) A)
  - `std::vector<std::unique_ptr<LLUboRingBuffer>> sWorkerDrawUboRingBuffers`
    ((N15a-2) ⭐ A LLUboRingBuffer per-instance 化 = class 改変 0 件 = constructor
    `BufferAllocator`/`BufferDestroyer` `std::function` injection + copy/assignment
    delete + unique_ptr pattern で N independent instance 作成可能、Vulkan device
    非依存 algorithm はそのまま、`indra/llcommon/lluboringbuffer.h` 物理位置 =
    PC-N-14 §1.2 reference 13 の `indra/llrender/` literal 誤記訂正、
    `feedback_admit_unknown` 遵守で本 PC-N-15a doc + design-lock doc §1.2/§2.1
    で明示記録)

### §1.3 step (c) `thread_local` 化 4 件 ((N15a-3) A)

`indra/llrender/llvkloader.cpp` anonymous namespace 内 file-static accessor:

- `sCurrentAsset` (= PC-7γ-2 既配線 owner storage)
- `sCurrentSkin` (= PC-N-8 (e) 既配線 owner storage)
- `sCurrentPrimitive` (= PC-N-8 (e) 既配線 owner storage)
- `sCurrentNodeAssetMatrix` (= PC-N-12 既配線 owner storage)

4 件全てに `thread_local` 修飾子を追加。3 件は `<AYAstorm r41 PC-N-15a (c)>
thread_local 化 ((N15a-10) A 4 件のうち N 件目)` tag block 内で surgical
insertion (1 件 = `sCurrentSkin` は既 `setCurrentSkin` accessor block で
inline 化、他 3 件は明示 tag block 化)。

- accessor signature 不変 (`setCurrentXxx(...)` / `clearCurrentXxx()` /
  `getCurrentXxx()` 3 件 × 4 = 12 件不変) = layering 制約完全充足 = PC-N-12
  Option A pivot 教訓踏襲
- worker thread 内 accessor 呼出時、main thread `setCurrentXxx` で書込まれた
  値は worker thread からは見えない = worker thread が必ず自分で
  `setCurrentXxx` を呼ぶ前提 (= PC-N-15b dispatch 配線時に main thread から
  worker thread に明示的に値を copy 渡しする実装方針) = (N15a-3) A 採用
  根拠明文化

### §1.4 step (d) `createWorkerThreadInfra()` + `destroyWorkerThreadInfra()` helper 新設 ((N15a-10) A 配列要素数 = worker_count、(N15a-5) A `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`、(N15a-6) A secondary、(N15a-2) ⭐ A LLUboRingBuffer factory closure lambda capture `[i]`)

`indra/llrender/llvkloader.cpp` ~line 1585-1762 `<AYAstorm r41 PC-N-15a (d)>`
tag block:

- `createWorkerThreadInfra()` 関数:
  1. `AYAGltfWorkerThreadEnabled` cvar guard = false 時は immediate `return true`
     (= worker storage 確保なし、MUSEUBO-A 整合)
  2. worker thread 数決定 = `AYAGltfWorkerThreadCount`=0 時
     `std::thread::hardware_concurrency() - 1` (= main thread 除外、cap 4 最小 1)、
     >0 時 cvar 値直接使用 ((N15a-8) A の literal 訂正 = U32 cvar = 0 default で
     auto detect、Boolean ではなく U32 で 0 / fixed N の両義性表現)
  3. per-thread `VkCommandPool` create = loop `i in [0, worker_count)`:
     `VkCommandPoolCreateInfo { .flags =
     VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, .queueFamilyIndex =
     sGraphicsQueueFamily }` + `vkCreateCommandPool` → `sWorkerCommandPools[i]`
  4. secondary `VkCommandBuffer` allocate = loop:
     `VkCommandBufferAllocateInfo { .commandPool = sWorkerCommandPools[i],
     .level = VK_COMMAND_BUFFER_LEVEL_SECONDARY, .commandBufferCount = 1 }` +
     `vkAllocateCommandBuffers` → `sWorkerSecondaryCommandBuffers[i]`
  5. per-thread `LLUboRingBuffer` instance 生成 = loop:
     `auto rb = std::make_unique<LLUboRingBuffer>(allocator_closure,
     destroyer_closure)` where allocator_closure は lambda capture `[i]` で
     per-thread sub-map に書込 = `sAllocator` 経由 `vmaCreateBuffer`
     HOST_VISIBLE + MAPPED、Vulkan device 非依存 algorithm はそのまま class
     改変 0 件 ((N15a-2) ⭐ A)、`rb->initialize()` 呼出で internal buffer alloc
     即時起動 (= `sAllocator` alive 必須ゆえ caller side 配線位置で alive を
     保証する step (e) と整合)
  6. 各 step error log: `vkCreateCommandPool`/`vkAllocateCommandBuffers`/
     `rb->initialize()` 失敗時 `LL_WARNS("Vulkan")` で per-worker index +
     error code log → 半端 cleanup なしに次へ進む方針 (= 起動失敗時 viewer
     全体 abort path 経由予定、PC-N-15b で escalation 確定)
  7. 成功時 `LL_INFOS("Vulkan") << "PC-N-15a worker thread infra created:
     worker_count=" << worker_count` (= 通電確認用)

- `destroyWorkerThreadInfra()` 関数: reverse-init order cleanup
  1. `sWorkerDrawUboRingBuffers.clear()` → unique_ptr destructor 連鎖で各 RB
     `shutdown()` → `destroyer_closure` 経由 `vmaDestroyBuffer` (= `sAllocator`
     alive 必須)
  2. loop `i`: `vkFreeCommandBuffers(sDevice, sWorkerCommandPools[i], 1,
     &sWorkerSecondaryCommandBuffers[i])` → `sWorkerSecondaryCommandBuffers.clear()`
  3. loop `i`: `vkDestroyCommandPool(sDevice, sWorkerCommandPools[i], nullptr)`
     → `sWorkerCommandPools.clear()`
  4. (= sAllocator + sDevice alive 期間内必須、step (e) で順序保証)

### §1.5 step (e) `initVulkan` / `shutdownVulkan` lifecycle 配線 ((N15a-13) A indra/ 改変は本実装 phase 唯一)

`indra/llrender/llvkloader.cpp`:

- `initVulkan()` 内 `createDrawUboRingBuffer` 成功直後 (~line 3928-3943)
  `<AYAstorm r41 PC-N-15a (e)> worker thread infrastructure 起動 hook` tag
  block で:
  ```
  if (!createWorkerThreadInfra()) { shutdownVulkan(); return false; }
  ```
  (= sAllocator alive (`createVmaAllocator` 既完了) + sDevice alive + main
  sDrawUboRingBufferMgr alive 状態で起動。LLUboRingBuffer factory closure
  内 `vmaCreateBuffer` が即時起動するため、配置順序が重要。)
- `shutdownVulkan()` 内 main `vkDestroyCommandPool` 直後 (~line 4570-4580)
  `<AYAstorm r41 PC-N-15a (e)> worker thread infrastructure teardown hook` tag
  block で:
  ```
  destroyWorkerThreadInfra();
  ```
  (= sAllocator + sDevice alive、main `sDrawUboRingBufferMgr` destroy 前の
  ため per-thread LLUboRingBuffer destroyer closure 経由 `vmaDestroyBuffer`
  が安全に動く位置)

---

## §2. build verify literal 取得結果 ((N15a-11) A 採用)

- ✅ `llrender` library clean rebuild **PASS** (= warning 0 + error 0)
- ✅ `INTEGRATION_TEST_lluboringbuffer` **11/11 PASS YAY!!**
- ✅ `INTEGRATION_TEST_llassetubopool` **10/10 PASS YAY!!**
- ✅ `INTEGRATION_TEST_llpipelinecachestorage` **13/13 PASS YAY!!**
- ✅ `python3 -m unittest discover` from `scripts/ubo_codegen` = **131 tests, OK**
- ✅ **GATE-B integrity** = `grep -c LL_VULKAN_GLSL indra/llrender/llvkloader.cpp`
  = **6** (= PC-N-14 commit `e96f7e2a68` 同数、`feedback_design_phase_no_code_write`
  → 実装 phase で GATE-B 不変 maintained = `mUseUBO` runtime gate のみ)

---

## §3. PC-N-15a 実装 phase Exit Criteria (= 10 項全充足)

| # | criterion | status |
|---|-----------|--------|
| i | `AYAGltfWorkerThreadEnabled` Boolean cvar + `AYAGltfWorkerThreadCount` U32 cvar 2 件 settings.xml 追加 ((N15a-7)/(N15a-8)/(N15a-9) A) | ✅ |
| ii | `sCurrentAsset` + `sCurrentSkin` + `sCurrentPrimitive` + `sCurrentNodeAssetMatrix` 4 件 `thread_local` 修飾子追加 ((N15a-3) A) | ✅ |
| iii | per-thread `VkCommandPool` 配列 + secondary `VkCommandBuffer` 配列 + `LLUboRingBuffer` 配列 + `sPcn13MultiAssetSeenMutex` storage 新設 ((N15a-2) ⭐ A + (N15a-4) A + (N15a-5) A + (N15a-6) A) | ✅ |
| iv | `createWorkerThreadInfra()` helper = cvar guard + worker count 決定 + per-thread VkCommandPool + secondary VkCommandBuffer + per-thread LLUboRingBuffer (factory lambda capture `[i]` per-thread sub-map 書込) ((N15a-10) A) | ✅ |
| v | `destroyWorkerThreadInfra()` helper = reverse-init order cleanup (= LLUboRingBuffer destroy → secondary VkCommandBuffer free → VkCommandPool destroy) | ✅ |
| vi | `initVulkan` 内 `createDrawUboRingBuffer` 直後 `createWorkerThreadInfra()` 呼出 + `shutdownVulkan` 内主 `vkDestroyCommandPool` 直後 `destroyWorkerThreadInfra()` 呼出 (= sAllocator + sDevice alive 期間内) | ✅ |
| vii | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp=6 不変、PC-N-14 commit `e96f7e2a68` 同数) | ✅ |
| viii | MUSEUBO-A 整合 = `AYAGltfWorkerThreadEnabled=false` default で worker storage 確保なし、main thread 経路維持 + OpenGL 描画 100% 維持 + PC-N-8 (f) 5 段 graceful degrade 内部維持 (= PC-N-15a 段階では ON 切替でも描画経路は不変、storage 確保のみ、worker launch + actual record 経路通電は PC-N-15b 持越し) | ✅ |
| ix | build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 ((N15a-11) A) + cross-platform spec §6 PC-N-15a 行 ✅ 反映 + §A 履歴 1 行追記 + handoff complete doc 起案 | ✅ |
| x | self-verify 9 観点 全 ✅ | ✅ |

---

## §4. 改変 file 4 件

| # | file | 改変概要 | 実 LoC |
|---|------|---------|--------|
| 1 | `indra/llrender/llvkloader.cpp` | step (b)(c)(d)(e) = 冒頭 `<AYAstorm r41 PC-N-15a (a)>` tag block で `#include <thread>` + `#include <mutex>` + `#include <vector>` + `#include <memory>` + anonymous namespace 内 `<AYAstorm r41 PC-N-15a (c)>` tag block で `sCurrentAsset`/`sCurrentSkin`/`sCurrentPrimitive`/`sCurrentNodeAssetMatrix` 4 件 `thread_local` 化 + `<AYAstorm r41 PC-N-15a (c)>` tag block で `sPcn13MultiAssetSeenMutex` `std::mutex` 追加 + `<AYAstorm r41 PC-N-15a (a)>` tag block で per-thread storage 3 配列 (`sWorkerCommandPools` + `sWorkerSecondaryCommandBuffers` + `sWorkerDrawUboRingBuffers`) 新設 + `<AYAstorm r41 PC-N-15a (d)>` tag block で `createWorkerThreadInfra()`/`destroyWorkerThreadInfra()` helper 実装 + `<AYAstorm r41 PC-N-15a (e)>` tag block で `initVulkan` (= `createDrawUboRingBuffer` 直後) + `shutdownVulkan` (= 主 `vkDestroyCommandPool` 直後) 呼出配線 | net +280 |
| 2 | `indra/newview/app_settings/settings.xml` | step (a) `AYAGltfWorkerThreadEnabled` Boolean + `AYAGltfWorkerThreadCount` U32 2 cvar 追加 (`AYAGltfMultiAssetCanary` 直後並列、default=0 Persist=1、各 cvar 単独 Comment、`<AYAstorm r41 PC-N-15a (a)>` 包括 tag block) | +31/-0 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md` | §6 PC-N-15a 行 状態 ✅ 反映 + §A 履歴 1 行追記 (chronological order = design-lock entry → ✅ 反映 entry 順) | +3/-1 |
| 4 | (本 file) `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-...-phase1-e-pc-n-15a-complete.md` | step (e) 末尾 = 新規 PC-N-15a complete handoff doc 起案 | new |

PC-N-14 design-lock 想定 6 件 (`llvkloader.h` + `llvkloader.cpp` + `settings.xml`
+ `gltfscenemanager.cpp` + spec + new handoff) から -2 件 (`llvkloader.h` 改変
0 件 = 全 storage は llvkloader.cpp anonymous namespace 内に閉じ込め + helper 関数は
file-static = header 露出不要、`gltfscenemanager.cpp` 改変 0 件 = PC-N-15a は
infrastructure 配線のみで dispatch 配線は PC-N-15b 持越し) = 4 件 (= header
不要 + caller-side 配線なしの構造的必然)。

shader 改変 0 件、codegen 改変 0 件、CMake 改変 0 件、tests/ 改変 0 件、
`llvkloader.h` 改変 0 件、`gltfscenemanager.cpp` 改変 0 件、`LLUboRingBuffer`
class 改変 0 件 ((N15a-2) ⭐ A)。

---

## §5. PC-N-15a 達成事項 = worker thread infrastructure storage + lifecycle 配線 (dispatch は PC-N-15b 持越し)

### §5.1 infrastructure 起動経路 (= `AYAGltfWorkerThreadEnabled=true` 時、本 sub-step は配線のみ)

1. **lifecycle entry** (本 sub-step 新設): `initVulkan` 内
   `createDrawUboRingBuffer` 成功直後 `createWorkerThreadInfra()` 呼出 =
   sAllocator + sDevice alive 期間内
2. **cvar guard**: `AYAGltfWorkerThreadEnabled=false` default 時 immediate
   `return true` (= worker storage 確保なし、MUSEUBO-A 整合)
3. **worker count 決定**: `AYAGltfWorkerThreadCount`=0 (default) 時
   `std::thread::hardware_concurrency() - 1` auto detect = main thread 除外、
   HW 並列度上限活用、>0 時 cvar 値直接使用 = 開発時 A/B testing 用
4. **per-thread VkCommandPool create**: loop `worker_count` 回、
   `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT` + `sGraphicsQueueFamily`
   = Vulkan spec 厳守
5. **secondary VkCommandBuffer allocate**: loop `worker_count` 回、
   `VK_COMMAND_BUFFER_LEVEL_SECONDARY` + 1 件 / pool = PC-N-15b で
   `VkCommandBufferInheritanceInfo` 経由 main primary cmdbuf 配下で record
6. **per-thread LLUboRingBuffer instance 生成**: loop `worker_count` 回、
   `std::make_unique<LLUboRingBuffer>(allocator_closure[i], destroyer_closure[i])`
   = factory lambda capture `[i]` で per-thread sub-map に書込 = `vmaCreateBuffer`
   HOST_VISIBLE+MAPPED 経路は既 main thread `sDrawUboRingBufferMgr` と同形、
   class 改変 0 件で N independent instance 達成

### §5.2 infrastructure teardown 経路 (= reverse-init order)

1. **lifecycle exit** (本 sub-step 新設): `shutdownVulkan` 内 主
   `vkDestroyCommandPool` 直後 `destroyWorkerThreadInfra()` 呼出 = sAllocator
   + sDevice alive、main `sDrawUboRingBufferMgr` destroy 前
2. **LLUboRingBuffer destroy** (reverse step 1): `sWorkerDrawUboRingBuffers.clear()`
   → unique_ptr destructor 連鎖 → 各 RB `shutdown()` → destroyer closure 経由
   `vmaDestroyBuffer` = sAllocator alive 必須
3. **secondary VkCommandBuffer free** (reverse step 2): loop
   `vkFreeCommandBuffers(sDevice, sWorkerCommandPools[i], 1, ...)`
4. **per-thread VkCommandPool destroy** (reverse step 3): loop
   `vkDestroyCommandPool(sDevice, sWorkerCommandPools[i], nullptr)`

### §5.3 fall-through path (= `AYAGltfWorkerThreadEnabled=false` default 時 = MUSEUBO-A 整合)

- `AYAGltfWorkerThreadEnabled=false` (default): `createWorkerThreadInfra()`
  immediate `return true` で worker storage 確保なし、`destroyWorkerThreadInfra()`
  も 3 件 `clear()` のみで早期 return = steady state cost 0
- main thread 経路 (= 既 `sDrawUboRingBufferMgr` + 既 `sCommandPool` 経路)
  維持 + OpenGL 描画 100% 維持 + PC-N-8 (f) 5 段 graceful degrade 内部維持 +
  PC-N-11/12/13 cvar baseline 不変 + MUSEUBO-A 整合
- **重要**: PC-N-15a 段階では `AYAGltfWorkerThreadEnabled=true` に切替えても
  描画経路は変化しない (= storage 確保のみ、worker launch + actual record
  経路通電は PC-N-15b 持越し)。本 sub-step は infrastructure 配線のみ。

### §5.4 (N15a-2) ⭐ critical = LLUboRingBuffer per-instance 化 = class 改変 0 件 詳細

- **発見**: 現状調査で `LLUboRingBuffer` は単一 instance + mutate 状態 5 件
  (`mBuffer`/`mCurrentSizeBytes`/`mFrameIndex`/`mActiveChunk`/`mChunkBytesUsed`)
  を発見 = β案 mutex 保護「短い寿命だけ並列化」が (E-14) B 「UBO write +
  cmdbuf 両方並列化」整合不可 → α案 per-thread LLUboRingBuffer instance refactor
  採用方針確定
- **class 改変 0 件達成根拠**: constructor が `BufferAllocator`/`BufferDestroyer`
  `std::function` injection を受領 + copy/assignment delete + unique_ptr
  pattern で N independent instance 作成可能 = Vulkan device 非依存 algorithm
  はそのまま。`indra/llcommon/lluboringbuffer.h` 物理位置 = PC-N-14 §1.2
  reference 13 の `indra/llrender/` literal 誤記訂正、`feedback_admit_unknown`
  遵守で本 PC-N-15a doc + design-lock doc §1.2/§2.1 で明示記録
- **factory lambda capture `[i]`**: per-thread sub-map
  `sWorkerDrawUboRingBuffers[i]` に書込で per-thread 独立 instance 化 =
  既 `sDrawUboRingBufferMgr` (main thread instance) と完全独立な N 件の
  LLUboRingBuffer instance を生成、各 instance は自分の `vmaCreateBuffer`
  buffer + 独立 frame state を保持
- **PC-N-15b 持越し**: worker thread からの per-Primitive write + secondary
  cmdbuf record + main thread vkCmdExecuteCommands 集約 = PC-N-15b dispatch 配線
  phase で本 storage を消費

### §5.5 (N15a-4) A = sPcn13MultiAssetSeen mutex 保護で canary semantic 維持

- **問題**: PC-N-14 design-lock (N14-8) A literal「`thread_local` 化で accessor
  signature 不変」を sPcn13MultiAssetSeen にそのまま適用すると、worker thread
  別々に「初回 asset」と認識 = PC-N-13 (b) `size() > 1u` 検出 semantic 破壊
  risk = 2 件以上の asset 同時描画通電を検出不能
- **解決**: sPcn13MultiAssetSeen は thread_local 化「しない」+ `std::mutex
  sPcn13MultiAssetSeenMutex` で main thread + worker thread access を serialize
  で `size() > 1u` 検出 semantic 維持 = (N14-21) 新 ambiguity AYA literal「OK」
  record 2026-06-05 受領
- **PC-N-15b 持越し**: PC-N-15b dispatch 配線 phase で worker thread からの
  insert を `std::lock_guard<std::mutex>` で保護する経路を配線

### §5.6 Phase 1.E 残課題 (= PC-N-15b / PC-N-15c)

- **PC-N-15b** (= 6th sub-step) worker thread dispatch 配線 = `LL::WorkQueue`
  post/drain 配線 + per-Primitive worker dispatch + secondary cmdbuf record
  + `vkCmdExecuteCommands` 集約 + `sPcn13MultiAssetSeen` mutex 保護 +
  `sSkinUboDirty` merge semantics + first-fire marker 3 件
  (`s_first_pcn14_worker_thread_fire` + `s_first_pcn14_secondary_cmdbuf_fire`
  + `s_first_pcn14_ubo_parallel_fire`)
- **PC-N-15c** (= 7th = 最終 sub-step) cleanup + `sGltfStubSkin` sentinel 撤去
  + Phase 1.E complete marker 起案

---

## §6. 残 strict 線形

- ✅ Phase 1.A / 1.B / (Z) SSS / (W) uniform4iv / (Y) Phase 1.C prep
- ✅ PC-0..PC-7ε / PC-N decomposition / PC-N-1..PC-N-4 (= Phase 1.C complete)
- ✅ PC-8 Linux primary marker (= Phase 1.C strict 線形終了)
- ✅ PC-N-5 (= Phase 1.D 着手起点) / Phase 1.D decomposition design-lock
- ✅ PC-N-6 / PC-N-7 / PC-N-8 / PC-N-9 / PC-N-10 (= Phase 1.D complete = 1 GLTF
  asset 完全 Vulkan draw 通電 達成)
- ✅ Phase 1.E decomposition design-lock (commit `094546889b`) + PC-N-11
  design-lock (commit `87560a4dc7`) + PC-N-11 実装 (commit `797332ee81`) +
  PC-N-12 design-lock (commit `9a62f11416`) + PC-N-12 実装 (commit `b6b39bfd9f`)
  + PC-N-13 design-lock (commit `cd253cb754`) + PC-N-13 実装 (commit `289d44b536`)
  + PC-N-14 design-lock (commit `e96f7e2a68`) + PC-N-15a design-lock (commit
  `631794883d`)
- ✅ **PC-N-15a 実装 ✅ 本 commit = Phase 1.E 内 5th sub-step 実装完了 =
  worker thread infrastructure 配線**
- ⏳ PC-N-15b design-lock + 実装 (= worker thread dispatch 配線)
- ⏳ PC-N-15c (= cleanup + `sGltfStubSkin` sentinel 撤去 + Phase 1.E complete
  marker)
- ⏳ Phase 1.E complete → Phase 1 全完了 → Mac/Win 開発者補完 phase

---

## §7. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ +
(Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α..PC-7ε ✅ +
PC-N decomposition design-lock ✅ + PC-N-1..PC-N-4 ✅ = Phase 1.C complete ✅ +
PC-8 Linux primary marker ✅ + PC-N-5 ✅ + Phase 1.D decomposition design-lock ✅
+ PC-N-6 ✅ + PC-N-7 ✅ + PC-N-8 ✅ + PC-N-9 ✅ + PC-N-10 ✅ = Phase 1.D
complete ✅ (= 1 GLTF asset 完全 Vulkan draw 通電 達成) +
Phase 1.E decomposition design-lock ✅ + PC-N-11 design-lock ✅ +
PC-N-11 ✅ + PC-N-12 design-lock ✅ + PC-N-12 ✅ + PC-N-13 design-lock ✅ +
PC-N-13 ✅ + PC-N-14 design-lock ✅ + PC-N-15a design-lock ✅ +
**PC-N-15a ✅ 本 commit = Phase 1.E 内 5th sub-step 実装完了 = worker thread
infrastructure 配線** +
⏳ PC-N-15b design-lock + 実装 + PC-N-15c + Phase 1.E complete + Phase 1 全完了 +
Mac/Win 開発者補完 phase

---

## §8. self-verify 9 観点 全 ✅

1. ✅ Exit Criteria 10 項全充足 (§3)
2. ✅ 必読 1 件 (PC-N-15a design-lock doc) + pinpoint reference 別記 = full file
   dump なし (= `feedback_handoff_minimal_pre_req_read` 整合)
3. ✅ step (a)/(b)/(c)/(d)/(e) 5 step 全実装 (§1)
4. ✅ ambiguity (N15a-1)..(N15a-13) 13 件 AYA literal「OK」record
   (2026-06-05) design-lock 継承 + 本実装で全件採用案通り実装 (特に (N15a-1) ⭐
   A 採用 = PC-N-15 → PC-N-15a/b/c 3 sub-phase 分解、(N15a-2) ⭐ A 採用 =
   LLUboRingBuffer per-instance 化 = class 改変 0 件、(N15a-4) A 採用 =
   sPcn13MultiAssetSeen mutex 保護で canary semantic 維持)
5. ✅ GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp=6
   不変、PC-N-14 commit `e96f7e2a68` 同数)
6. ✅ MUSEUBO-A 整合 = `AYAGltfWorkerThreadEnabled=false` default で worker
   storage 確保なし、main thread 経路維持 + OpenGL 描画 100% 維持 + PC-N-8 (f)
   5 段 graceful degrade 内部維持 + PC-N-11/12/13 cvar baseline 不変、PC-N-15a
   段階では ON 切替でも描画経路は不変 (= storage 確保のみ、worker launch +
   actual record 経路通電は PC-N-15b 持越し)
7. ✅ build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11/11 + 10/10 +
   13/13 + codegen 131/131 全 PASS + GATE-B integrity `LL_VULKAN_GLSL`=6 不変
8. ✅ commit 内容 = 2 modified (indra/) + 1 modified (cross-platform spec) +
   1 new doc (本 complete handoff) + CMake 改変 0 + codegen 改変 0 + shader 改変
   0 + `llvkloader.h` 改変 0 + `gltfscenemanager.cpp` 改変 0 + `LLUboRingBuffer`
   class 改変 0 + tests/ 改変 0 + Co-Authored-By 不在
9. ✅ `feedback_no_scope_shrink` 遵守 = PC-N-15a literal scope 5 件 §0 全件実装、
   PC-N-15 → PC-N-15a/b/c 3 sub-phase 分解は scope 縮小ではなく
   `feedback_ubo_migration_one_at_a_time`「1 つずつ実施・大きすぎたらスコープ
   切り直す」原則踏襲の sub-phase elaboration、PC-N-14 design-lock 全 scope は
   PC-N-15a/b/c 3 phase 累積で完全達成予定、PC-N-15b/PC-N-15c 持越しは別 phase
   分解 = `feedback_ubo_migration_one_at_a_time` 厳格遵守整合

---

## §9. 次 session 着手 1 line

PC-N-15b design-lock 着手 = worker thread dispatch 配線 design (= `LL::WorkQueue`
post/drain 配線 + per-Primitive worker dispatch + secondary cmdbuf record +
`vkCmdExecuteCommands` 集約 + `sPcn13MultiAssetSeen` mutex 保護 +
`sSkinUboDirty` merge semantics + first-fire marker 3 件) + ambiguity 確認 +
実装計画分解 + Exit Criteria 明文化 = `feedback_ubo_migration_one_at_a_time`
厳格遵守で本 PC-N-15a worker thread infrastructure 配線 baseline 上に Phase 1.E
内 6th sub-step として別 session 別途 design-lock。

---

## §A. feedback 遵守 record

- ✅ `feedback_proactive_handoff` (本 PC-N-15a complete handoff doc 起案)
- ✅ `feedback_handoff_minimal_pre_req_read` (必読 1 件 = PC-N-15a design-lock doc
  + pinpoint Read のみ、full file dump なし)
- ✅ `feedback_self_verify_before_handoff` (9 観点 self-verify 全 ✅)
- ✅ `feedback_build_only_verified` (llrender + WARNING 0 + TUT 11/11 + 10/10 +
  13/13 + codegen 131/131 + GATE-B integrity `LL_VULKAN_GLSL`=6 不変で literal
  検証取得)
- ✅ `feedback_no_scope_shrink` (PC-N-15a literal scope 5 件 §0 全件実装、PC-N-15
  → PC-N-15a/b/c 3 sub-phase 分解は `feedback_ubo_migration_one_at_a_time`「1 つ
  ずつ実施・大きすぎたらスコープ切り直す」原則踏襲の sub-phase elaboration、
  PC-N-14 design-lock 全 scope は PC-N-15a/b/c 3 phase 累積で完全達成予定明示)
- ✅ `feedback_doubt_self_first` (design-lock phase で β案 (mutex 保護) 初稿
  提示 → AYA literal「短い寿命だけ並列化で意味なし」literal feedback で α案
  (per-thread instance refactor) 切替 → 3 sub-phase 分解 B 案提示 → AYA
  literal「Bでいきましょう」record で 3 sub-phase 分解確定 → ambiguity 13 件
  発見 + 推奨案提示 + AYA literal「OK」record 後本実装、推測実装なし)
- ✅ `feedback_admit_unknown` (現状調査で `indra/llcommon/lluboringbuffer.h`
  物理位置 = PC-N-14 §1.2 reference 13 `indra/llrender/` literal 誤記訂正を
  発見した時点で「PC-N-14 design-lock doc に literal 誤記あり」と認め、勝手に
  無視せず本 PC-N-15a doc + design-lock doc §1.2/§2.1 で明示記録 +
  LLUboRingBuffer 単一 instance + mutate 状態を発見した時点で「単純な
  per-thread storage 化では並列不可」と認め、AYA に β案 → α案 → 3 sub-phase
  分解 B 案 と段階的 surface)
- ✅ `feedback_confirm_referent_before_acting` (13 件 batch AYA 確認 design-lock
  phase で完了、(N15a-1) ⭐ critical は 3 sub-phase 分解 B 案提示 + AYA「B
  でいきましょう」literal record 受領で確定、(N14-21) 新 ambiguity =
  sPcn13MultiAssetSeen thread_local 化対象外決定も AYA literal「OK」record
  受領で確定、推測実装なし)
- ✅ `feedback_ubo_migration_one_at_a_time` 厳格遵守 (PC-N-15 単一 sub-step →
  PC-N-15a/b/c 3 sub-phase 分解 = 「1 つずつ実施・大きすぎたらスコープ切り直す」
  原則の直接適用、PC-N-15a = infrastructure 配線単独、PC-N-15b
  (= dispatch 配線) + PC-N-15c (= cleanup + Phase 1.E complete marker) は別
  session の別 phase で別途分解)
- ✅ `feedback_design_phase_no_code_write` 整合 (本 PC-N-15a は実装 phase = 改変
  あり、design-lock commit `631794883d` で `indra/` 改変 0 件完了済)
- ✅ `feedback_release_branch_workflow` (feature branch
  `feature/ayastorm-r41-gl-removal` 上 commit)
- ✅ `feedback_no_auto_commit` (AYA 明示 commit 指示受領後 commit 予定)
- ✅ `feedback_no_claude_coauthor` (Co-Authored-By 行不在)
- ✅ `feedback_no_bare_reference_ids` ((N15a-1)..(N15a-13) 各 ID に項目名 /
  採用案内容併記 + (a)..(e) 各 step に作業内容併記)
- ✅ `feedback_tests_dir_never_commit` 整合 (tests/ 改変 0 件、git add 個別
  file 指定予定)
- ✅ memory `project_ayastorm_r41_design_principles` 整合
  ((1) Upstream OpenGL 取り込みやすさ維持 = accessor signature 不変 (12 件
  不変) + thread_local 化のみ + `LLUboRingBuffer` class 改変 0 件 + shader 改変
  ゼロ + (2) Core プロセス分散実現 = per-thread LLUboRingBuffer instance +
  per-thread VkCommandPool + secondary VkCommandBuffer で primitive-level
  granularity 維持 = PC-N-15b dispatch 配線 phase で worker thread から消費)
- ✅ memory `project_r41_phase1b_vulkan_host_gate` 整合 (GATE-B = `#ifdef
  LL_VULKAN_GLSL` 新規追加 0 件、`AYAGltfWorkerThreadEnabled` +
  `AYAGltfWorkerThreadCount` cvar runtime gate のみ、count llvkloader.cpp=6
  不変)
- ✅ memory `project_ayastorm_three_platforms` 整合 (cross-platform spec §6
  PC-N-15a 行状態 ✅ 反映 = host-side worker thread infrastructure +
  per-thread `LLUboRingBuffer` + `thread_local` storage + `std::mutex` は OS
  非依存 + per-thread `VkCommandPool`
  `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT` + secondary
  `VkCommandBuffer` `VK_COMMAND_BUFFER_LEVEL_SECONDARY` は MoltenVK 標準対応
  範囲 + descriptor set 数 5 維持 + `AYAGltfWorkerThreadEnabled` +
  `AYAGltfWorkerThreadCount` cvar XML は OS 非依存 ゆえ macOS 派生 fix 候補
  なし想定 + Windows full Vulkan ゆえ派生 fix 候補なし想定、Linux primary
  完成 → 他者補完 model 整合)
