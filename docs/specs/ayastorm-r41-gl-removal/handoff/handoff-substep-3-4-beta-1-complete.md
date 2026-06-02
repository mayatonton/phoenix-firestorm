# r41 sub-step 3.4-β-1 完遂 → 3.4-β-2 着手境界 handoff (2026-05-31)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-B-complete.md` (3.3-B 全完遂 / 3.4 着手境界) + `handoff-session-pause-2026-05-31-after-3-3-B.md` (3.3-B 完遂後 session pause、resume 後 3.4-α 完遂 commit `a8f8472202`)
**本 handoff 位置付け**: sub-step 3.4-β-1 (VMA allocator init + descriptor pool sizing 基盤 + 領域 7 sub-step 7.1 内包先行 install) 完遂宣言 + 3.4-β-2 (LLImageGL → VkImage + VkImageView lifecycle、bridging item #8 satisfy) 着手前 scope 確認境界。
**3.4 全体構成 (sub-doc 03 §3.1.4)**: 3.4-α (spec refine、完遂 2026-05-31 commit `a8f8472202`) / **3.4-β-1 (VMA + 共有 pool 基盤、本 handoff で完遂宣言)** / 3.4-β-2 (LLImageGL → VkImage lifecycle) / 3.4-γ (set=1 per-material layout) / 3.4-δ (12 pool hook body + 段階 2 引継ぎ特殊対応 2 件) / 3.4-ε (3.4 全完遂 handoff)

---

## 1. sub-step 3.4-β-1 完遂 status (2026-05-31)

### 1.1 完遂 marker (sub-doc 03 §3.1.4 3.4-β-1)

| acceptance | 達成 status |
|---|---|
| VMA library (v3.3.0, MIT) vendor 配置 + CMake include path 配線 | ✓ `indra/llrender/vendor/vk_mem_alloc.h` 752 KB + `LICENSE.txt` 配置、`CMakeLists.txt` の `target_include_directories(llrender PRIVATE ...vendor)` で本 TU に限定可視化 |
| `VmaAllocator` init 成功 (Vulkan 1.3 + volk dynamic functions + VK_EXT_memory_budget enable) | ✓ AYA log L109 `"VMA allocator created (Vulkan 1.3, dynamic functions via volk, memory_budget=ON)"` |
| VK_EXT_memory_budget 検出 + device extension enable | ✓ AYA log L100 `"VK_EXT_memory_budget = supported (VMA budget query enabled)"` (queryAndLogDeviceLimits) + createDevice で extension list に追加 + `VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT` 立て済 |
| `VkDescriptorPool` 共有 pool 作成 (maxSets=200, UBO=16, COMBINED_IMAGE_SAMPLER=64) | ✓ AYA log L110 `"Shared descriptor pool created (3.4-β-1 placeholder, maxSets=200, UBO=16, COMBINED_IMAGE_SAMPLER=64; precision deferred to 7.3)"` |
| VMA budget query helper (`vmaGetBudget`) 1 度 smoke 出力 | ✓ AYA log L119 `"VMA budget smoke (3.4-β-1 1 度のみ、heapCount=2, VK_EXT_memory_budget=ON):"` + L120 heap 0 [DEVICE_LOCAL] size=32607 MB / budget=30888 MB / usage=0 MB / allocations=0 / blocks=0 + L121 heap 1 [HOST] size=48001 MB / budget=48001 MB / usage=3 MB / allocations=0 / blocks=0 |
| INFO marker 4 件 (extension + allocator + pool + budget smoke) | ✓ 全 hit (L100/L109/L110/L119-121) |
| AYA launch PASS | ✓ 2026-05-30T19:47:04Z 起動完了 + 2026-05-30T19:48:32Z shutdown clean (`Vulkan device destroyed` / `Vulkan instance destroyed`) |
| Vulkan 系 WARN/ERR 0 件 | ✓ `grep -E "Vulkan.*WARN\|Vulkan.*ERR" AYAstorm.log` 0 件 hit (非 Vulkan WARNING は font/HTTP/Settings の起動既知 baseline のみ、regression なし) |
| 3.3-A/3.3-B/3.3-C 既存 13 marker 全継続 (regression 0) | ✓ dynamicRendering enable / VkPipelineCache / Per-frame desc layout (PerFrameMatrixUBO + TextureMatrixUBO) / Per-frame UBO buffers / Per-frame descriptor sets / Placeholder PSO / Sky smoke PSO (3.3-B-γ) / Sky placeholder vert binding (3.3-B-δ) / bindTarget dynamic rendering begin (3.3-C-γ) / flush dynamic rendering end (3.3-C-δ) / syncMatrices PerFrame UBO write / syncMatrices TextureMatrix UBO write / syncMatrices modelview push constant (3.3-A-δ-2) / beginDynamicRendering helper (3.3-C-β-2) 全 hit |
| VMA allocations=0 / blocks=0 (期待通り) | ✓ 両 heap で 0 (per-frame UBO は依然 raw `vkAllocateMemory` 経路 = 3.3-A-β-2 path、VMA 経由 allocation は 3.4-β-2 で `vmaCreateImage` 初投入予定) |
| shutdown 順序 (pool destroy → vmaDestroyAllocator → device destroy) | ✓ `shutdownVulkan` で `vkDestroyDescriptorPool(sDevice, sSharedDescriptorPool, ...)` → `vmaDestroyAllocator(sAllocator)` → `vkDestroyDevice(sDevice, ...)` 順、validation 違反 / leak 系 WARN 0 件 |

### 1.2 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-session-pause-2026-05-31-after-3-3-B.md` | **役割完了** (3.4-α + 3.4-β-1 完遂で session pause 境界 satisfy、本 handoff で内容引継ぎ) |
| sub-doc `03-state-machine-pso.md` §3.1.4 3.4-β-1 行 | active 継続 (本 handoff 起草と同時に「完遂 2026-05-31」marker 追加 + 実装結果反映、3.4-β-2 着手 ready) |
| sub-doc `07-descriptor-renderpass.md` §3.1 sub-step 7.1 行 | active 継続 (本 sub-step 内包先行 install 完遂、本格 install は領域 7 sub-step 7.1 着手時に「3.4-β-1 完遂 evidence を継承して残作業のみ」で完遂宣言可) |
| sub-doc `06-shader-spirv.md` | active 継続 (本 sub-step touch なし、6.1 一括化判断は別 thread) |
| `handoff-substep-3-4-beta-1-complete.md` (本 handoff) | 新規作成 (3.4-β-1 完遂 → 3.4-β-2 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 3.4-β-2 着手 ready 状態へ update) |

---

## 2. 実装内容 (commit 範囲、本 commit 同梱)

### 2.1 vendor 配置 (`indra/llrender/vendor/`、新規 directory)

| file | 規模 | 内容 |
|---|---|---|
| `vk_mem_alloc.h` | 752 KB (752307 bytes) | VMA (Vulkan Memory Allocator) v3.3.0、header-only single-file library、MIT license。GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator 公式配布 release tag v3.3.0 (Vulkan 1.3 + memory_budget 対応) |
| `LICENSE.txt` | 1099 bytes | MIT license 本文 (Advanced Micro Devices, Inc.、再配布要件遵守) |

scope 限定: `target_include_directories(... PRIVATE ...)` で本 vendor は llrender target 内のみ可視化、他 module 拡散しない。実 `VMA_IMPLEMENTATION` 展開は `llvkloader.cpp` 1 translation unit 限定。

### 2.2 build 配線 (`indra/llrender/CMakeLists.txt` +5 行)

| 変更 | 内容 |
|---|---|
| vendor include path 追加 | `add_library(llrender ...)` 直後に `target_include_directories(llrender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor)` 1 行 + 4 行 comment block (sub-step 3.4-β-1 / sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.1 内包 cross-ref + scope 説明: VMA は llvkloader.cpp 1 TU 限定 `#define VMA_IMPLEMENTATION` 経由) |

### 2.3 runtime 実装 (`indra/llrender/llvkloader.cpp` +212 行)

| 変更 block | 行数 | 内容 |
|---|---|---|
| VMA include block (TU 先頭) | +19 | `VMA_STATIC_VULKAN_FUNCTIONS 0` + `VMA_DYNAMIC_VULKAN_FUNCTIONS 1` (volk と共存) + `VMA_IMPLEMENTATION` (本 TU で実体展開) + gcc/clang diagnostic push/pop で VMA 内 unused-variable / unused-parameter / missing-field-initializers / unused-function を局所抑制 + `#include "vk_mem_alloc.h"` |
| `DeviceLimits::memoryBudgetSupported` field 追加 | +5 | VK_EXT_memory_budget 検出結果格納用 bool field + 用途 comment (VMA 連携 + budget query 分岐) |
| 静的 storage 追加 | +11 | `VmaAllocator sAllocator = VK_NULL_HANDLE` + `VkDescriptorPool sSharedDescriptorPool = VK_NULL_HANDLE` + `bool sSharedVmaBudgetLogged = false` (per-frame ループ 1 回固定 flag) + 雛形 sizing と 7.3 最終精度移譲 comment |
| `queryAndLogDeviceLimits()` 内 VK_EXT_memory_budget 検出 | +12 | extension enumerate loop に `VK_EXT_MEMORY_BUDGET_EXTENSION_NAME` 検出分岐追加 (既存の `VK_KHR_push_descriptor` 検出 loop を `break` 削除して 2 extension 同時走査に refactor) + `sDeviceLimits.memoryBudgetSupported` set + INFO marker 1 行 (extension status) |
| `createDevice()` 内 device extension list 追加 | +14 | `std::vector<const char*> device_extensions` 構築 + `VK_EXT_memory_budget` 支援時のみ push + `enabledExtensionCount` / `ppEnabledExtensionNames` を空 → 動的化 + 4 行 comment (3.4-γ 以降の push_descriptor enable 先送り根拠 + device 作成失敗 risk 最小化) |
| `createVmaAllocator()` 新規 helper | +38 | `VmaVulkanFunctions` 2 entry fill (`vkGetInstanceProcAddr` + `vkGetDeviceProcAddr`) → VMA が残り auto-resolve、`VmaAllocatorCreateInfo` で Vulkan 1.3 + instance + physicalDevice + device + `VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT` (支援時のみ)、`vmaCreateAllocator` 呼出 + 失敗時 WARN + INFO marker 1 行 |
| `createSharedDescriptorPool()` 新規 helper | +30 | `VkDescriptorPoolSize` 2 件 (UNIFORM_BUFFER=16 + COMBINED_IMAGE_SAMPLER=64) + `VkDescriptorPoolCreateInfo` (maxSets=200, poolSizeCount=2、FREE_DESCRIPTOR_SET_BIT は意図的に立てず段階 3 phase reset 戦略前提) + `vkCreateDescriptorPool` 呼出 + 失敗時 WARN + INFO marker 1 行 |
| `logVmaBudgetSmoke()` 新規 helper | +43 | `sSharedVmaBudgetLogged` guard で 1 度限り、`vmaGetMemoryProperties` で heap 数取得 → `vmaGetHeapBudgets` で per-heap budget/usage 取得 → DEVICE_LOCAL/HOST 別判定 + MB 換算で INFO 出力 (heap 番号 + size + budget + usage + VMA allocations + blocks) |
| `initVulkan()` 内配線 | +9 | `createPipelineCache()` 成功後、`createPerFrameDescriptorSetLayout()` 直前に `createVmaAllocator() && createSharedDescriptorPool()` 連結呼出 + 失敗時 `shutdownVulkan()` early return + 配線位置の根拠 comment (per-frame UBO 構築前に立ち上げ、segment 3 phase の per-frame UBO 自体は raw vkAllocateMemory 継続) |
| `initVulkan()` 末尾 budget smoke 呼出 | +3 | 全 init 成功後 `sInitialized = true` 直前に `logVmaBudgetSmoke()` 1 度呼出 (initVulkan の最後で 1 回固定) |
| `shutdownVulkan()` 内 teardown | +15 | command buffer free 後、physical device cleanup 前に: (1) `sSharedDescriptorPool` 破棄 (`vkDestroyDescriptorPool`)、(2) `sAllocator` 破棄 (`vmaDestroyAllocator`)。順序は **sDevice 破棄前必須** (VMA 規約)、sub-step 3.4-β-1 cross-ref comment 付き |

### 2.4 file 変更 summary

| file | 修正規模 | 主内容 |
|---|---|---|
| `indra/llrender/CMakeLists.txt` | +5 / -0 | vendor include path 配線 (sub-step 3.4-β-1) |
| `indra/llrender/llvkloader.cpp` | +212 / -4 | VMA include + DeviceLimits 拡張 + memory_budget 検出 + 3 helper + initVulkan/shutdownVulkan 配線 |
| `indra/llrender/vendor/vk_mem_alloc.h` | 新規 752 KB | VMA v3.3.0 (MIT) header-only |
| `indra/llrender/vendor/LICENSE.txt` | 新規 1099 B | MIT license 本文 |

合計: **2 modified file + 2 新規 vendor file、code diff +217 / -4 line (net +213)**。

---

## 3. 設計判断履歴 (本 sub-step 着手時)

### 3.1 案 1 (image lifecycle 先行) 採用継承 + VMA 内包先行 install

3.4-α 完遂時点 (commit `a8f8472202`) で AYA 「案 1 (image lifecycle 先行) 推奨」承認受領。本 sub-step は 3.4-α 設計確定の通り **領域 7 sub-step 7.1 (VMA + descriptor pool sizing 基盤) を 3.4-β-1 へ前倒し install**。image lifecycle = VMA 前提のため、3.4-β-2 (`vmaCreateImage` 利用) の prerequisite として β-1 で立ち上げ。

### 3.2 VMA v3.3.0 + volk dynamic functions 採用

volk loader (sub-step 1.x で導入済) と VMA を共存させるため、VMA static functions は **無効化** (`VMA_STATIC_VULKAN_FUNCTIONS 0`)、dynamic functions 経由で `vkGetInstanceProcAddr` + `vkGetDeviceProcAddr` 2 entry のみ明示 fill。残りの function pointer は VMA 内で auto-resolve = volk が dispatch している vk function pointer を VMA が独自に dlsym し直す形になり、3 OS 共通 path で `LD_LIBRARY_PATH` / `vk_layer_path` 干渉を回避。

代案検討 (採用せず):
- **静的リンク**: VMA を別 `.cpp` で `VMA_IMPLEMENTATION` 展開 → 1 file 増、本 sub-step scope (1 TU 限定) と齟齬。
- **VMA static functions enable**: volk と vulkan-1.dll/.so の二重 export 衝突 risk、3 OS 検証 cost 増。

### 3.3 VK_EXT_memory_budget は支援時のみ enable (driver fallback path)

device extension list の生成は `sDeviceLimits.memoryBudgetSupported` で gating、未支援 device では VMA は内部 fallback path (memory allocation 累積 statistics、`vmaGetBudget` 戻り値の `budget` field は 0 / `usage` のみ valid) で動作。Mesa RADV / Mesa ANV / NVIDIA RTX 系は支援 (RTX 5090 = AYA 環境で `supported` 確認)、未支援は VMA が透過 fallback。

### 3.4 共有 pool 暫定 sizing (overshoot、最終精度は 7.3 移譲)

`maxSets=200, UBO=16, COMBINED_IMAGE_SAMPLER=64` は sub-doc 05 §3.6 pool sizing 戦略の overshoot 値。3.4-γ (set=1 per-material layout、7 PBR slot × placeholder 1 set) + 3.4-δ (12 pool hook body 配線) を吸収する暫定容量、material cache 本実装 (~50 material × frame in flight 3 = 150 pool) + per-frame UBO/per-draw push descriptor 全配線 は領域 7 sub-step 7.3 で最終精度確定。

`FREE_DESCRIPTOR_SET_BIT` は意図的に立てない: 段階 3 phase は pool reset 一括戦略 (frame 境界で `vkResetDescriptorPool`) を前提、個別 free の overhead を回避。

### 3.5 INFO marker 4 件配置 (検出 + allocator + pool + budget smoke)

3.4-α spec 案 (init + pool + budget の 3 marker) から **+1 marker** (extension 検出 status を `queryAndLogDeviceLimits()` 内に追加) で 4 marker 化。追加根拠:
- VK_EXT_memory_budget 支援/未支援は VMA fallback path 切替判断の起点 evidence
- queryAndLogDeviceLimits は既存 device limit log block の延長で自然に同居、独立 marker より block 内同居の方が log trace 1 箇所集約
- 3 marker (init + pool + budget) のうち budget marker は heap 数 × heap 行 (heap 0/1 で 2 行) なので実 INFO 出力行数は 4 件 + 2 heap 行 = 計 6 INFO line になり、4 marker count は spec acceptance テキストで適切に表現可能

### 3.6 shutdown 順序 (pool → allocator → device)

VMA 規約: `vmaDestroyAllocator` は `vkDestroyDevice` 前必須。共有 pool は VMA 非関連だが、descriptor pool も `vkDestroyDevice` 前破棄必須 (Vulkan 規約)。本 sub-step は **pool destroy → vmaDestroyAllocator → device destroy** の 3 段 teardown を `shutdownVulkan()` 内 sCommandBuffer free と sDevice 破棄の間に挟込み、validation 違反 0 件動作確認済。

---

## 4. risks / caveats

### 4.1 per-frame UBO は依然 raw `vkAllocateMemory` 経路 (3.3-A-β-2 path)、置換は領域 7 で実施

本 sub-step では VMA 基盤を立ち上げただけで、既存 per-frame UBO (3.3-A-β-2 で配置の 3 frame × 512 B HOST_VISIBLE_COHERENT) は raw `vkAllocateMemory` のまま。VMA budget smoke の **allocations=0 / blocks=0** は期待通り (置換が領域 7 で本格化するまで VMA 経由 allocation は発生しない)。3.4-β-2 で `vmaCreateImage` 初投入時に allocations/blocks が増加する想定 = 次 sub-step での budget smoke 再観測候補。

### 4.2 暫定 sizing overshoot の妥当性は 3.4-γ/δ で再評価

`maxSets=200, UBO=16, COMBINED_IMAGE_SAMPLER=64` は 3.4-γ (set=1 per-material 7 PBR slot × 1 set 配置) + 3.4-δ (12 pool hook body 配線) を吸収する想定値、領域 7 sub-step 7.3 で実 material cache 配線時に再評価 (~50 material × 3 frame = 150 set + alpha)。本 sub-step 完遂時点では actual descriptor set allocation 0 件のため、overshoot が過剰 / 不足の判断は 3.4-γ 着手後の transit smoke で確認。

### 4.3 VMA v3.3.0 と volk の機能 update tracking

VMA v3.3.0 は Vulkan 1.3 + memory_budget 対応の最新 stable。VMA upstream が新 extension (VK_KHR_maintenance5 等) を取り込んだ際、本 vendor 配置 file の更新が必要 = r42-α/β Mac/Win Vulkan 着手 + 領域 7 sub-step 7.5 着手前に upstream tag 確認 candidate (現状の base は v3.3.0、release tag-pinned で再現性確保)。

### 4.4 暫定 pool sizing で validation strict build 時 over-allocation 検出可能性

`maxSets=200` を 3.4-γ/δ で実 allocate 超過した場合、sub-step 3.5 validation strict force-enable build で `VK_ERROR_OUT_OF_POOL_MEMORY` validation error 発生可能性。本 sub-step では allocate 0 件 = validation 0 件、3.4-γ 配線時の transit smoke 観測で early warn。

### 4.5 vendor file の re-distribution と LICENSE 注意

VMA は MIT license、再配布要件は LICENSE.txt 同梱で satisfy 済。AYAstorm release bundle で `indra/llrender/vendor/LICENSE.txt` が viewer_manifest 経由で含まれない場合 = r41 close 前に bundle 内 license 集約 prep (現状 viewer_manifest は llrender vendor を touch しない、license 集約は別 sub-step で対応)。

---

## 5. next session entry point

### 5.1 次 session 着手前の準備

1. `git fetch origin` + `git status` で AYA push 後の状態同期確認 (`feedback_git_fetch_first.md`)
2. `git log --oneline -5` で本 commit が origin と同期しているか確認
3. 本 handoff doc 通読
4. sub-doc 03 §3.1.4 3.4-β-2 行再確認
5. memory `project_ayastorm_r41_vulkan_migration.md` status update 反映確認

### 5.2 sub-step 3.4-β-2 着手内容 (sub-doc 03 §3.1.4)

| 項目 | 内容 |
|---|---|
| **scope** | LLImageGL → VkImage + VkImageView lifecycle 並走化 (`llimagegl.{cpp,h}` 3,034 LOC、bridging item #8 satisfy、§1.5.3 配置) — `createTexture` / `bindTexture` / `destroyTexture` 等 API surface に Vulkan path 並走分岐挿入 (`vmaCreateImage` + `vkCreateImageView` + `vmaDestroyImage`)、format conversion table (`LLGLenum` → `VkFormat` 静的表 ~20 entry: `GL_RGBA8`→`VK_FORMAT_R8G8B8A8_UNORM` 等) を `llvkloader.h` に配置、placeholder texture 1 件 (1×1 white) の transit smoke で動作実証 |
| **対象 file** | `llimagegl.{cpp,h}` + `llvkloader.{cpp,h}` (+~200 行想定) |
| **完了 marker** | `vmaCreateImage` 成功 (placeholder 1×1 white) + `vkCreateImageView` 成功 + `vmaDestroyImage` 成功 + format conversion table 配置 + INFO marker `"VkImage placeholder lifecycle smoke (1×1 white、VkFormat=R8G8B8A8_UNORM、image+view+destroy 一連 OK)"` + AYA launch PASS + Vulkan 系 WARN/ERR 0 件 + 3.3-A/B/C + β-1 全 marker 維持 (regression 0) |

### 5.3 3.4-β-2 着手 task 候補 (推定 sub-step 内 step、必要なら細分化案を AYA 確認)

| step | 内容 | 推定規模 |
|---|---|---|
| β-2-1 | format conversion table `LLGLenum → VkFormat` を `llvkloader.h` に配置 (~20 entry: GL_RGBA8/RGB8/R16F/R32F/D24S8/sRGB 系等) | 30-45 分 |
| β-2-2 | `vmaCreateImage` + `vkCreateImageView` + `vmaDestroyImage` 1×1 white placeholder transit smoke を `llvkloader.cpp` に追加 + INFO marker 1 件 | 45-60 分 |
| β-2-3 | LLImageGL `createTexture` / `bindTexture` / `destroyTexture` API surface に Vulkan path 並走分岐挿入 (gating: `LLVKLoader::isVulkanInitialized()`、3.3-C-γ/δ pattern 継承)、本 sub-step は 1 placeholder 動作実証のみ、actual draw 経路統合は領域 7 sub-step 7.5 移管 | 60-90 分 |
| β-2-4 | AYA launch verify + log INFO marker 確認 + 既存 17 marker (13 base + 3.4-β-1 4 件) 全継続 verify | 15-20 分 |

着手前に **AYA 確認候補 1 件**: 「3.4-β-2 を A (β-2-1〜β-2-4 一括 commit) / B (β-2-1+β-2-2 / β-2-3 / β-2-4 verify の 3 commit に細分化) のどちらで進めるか」 (`feedback_one_step_at_a_time.md` 遵守、AYA literal scope 確認)。

### 5.4 critical reminders

- **AYAstorm 改変 13 file shader 改変禁止** (charter §2.1 領域 6、`git diff` 0 件維持) — 3.4-β-2 は llimagegl + llvkloader のみ touch、shader 不変
- **段階 1 + 段階 2 + 3.1b + 3.2 + 3.3-A + 3.3-B + 3.3-C + 3.4-β-1 動作維持** (sub-doc 03 §3.5、Vulkan path 並走で GL 描画動作維持)
- **3.4-γ (set=1 per-material layout) は β-2 完遂後着手**、β-2 で配置の format conversion table を 7 PBR slot 配信で再利用
- **実 attachment 配線 + actual draw 経路統合は領域 7 sub-step 7.5 持越し** (本 3.4-β-2 では LLImageGL → VkImage の object lifecycle のみ scope、actual draw 経路への実 attachment 統合は 7.5 担当)
- **validation strict は sub-step 3.5 で別 build により実施** (3.4-β-2 着手時も release build で AYA launch + WARN/ERR 0 件 transit acceptance)
- **検証用 LL_INFOS hook は出荷物に残さない** (`feedback_remove_verification_logs.md`、本 sub-step 4 INFO marker は spec 由来の永続 marker = OK、temp diagnostic hook は別途)
- **AYA 確認は 1 件** (3.4-β-2 細分化判断、`feedback_one_step_at_a_time.md` 遵守)、決着後に順次次の確認

---

## 6. 関連 doc / memory cross-ref

### 6.1 関連 doc

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter
- `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` — 段階 3 sub-doc (本 handoff で sub-step 3.4-β-1 行 complete 化 + 3.4-β-2 marker active)
- `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` — 領域 7 sub-doc (sub-step 7.1 内包先行 install 完遂、残作業の本格 install は領域 7 sub-step 7.1 着手時)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-B-complete.md` — 3.3-B 全完遂 handoff (active 参照)
- `docs/specs/ayastorm-r41-gl-removal/handoff-session-pause-2026-05-31-after-3-3-B.md` — session pause handoff (役割完了済、本 handoff で 3.4-α + 3.4-β-1 完遂を受けて 3.4-β-2 entry point へ更新)
- `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` — §3.6 pool sizing 戦略 (本 sub-step 暫定 sizing の根拠)

### 6.2 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 handoff 完遂で sub-step 3.4-β-2 着手 ready 状態に update)
- `project_ayastorm_three_platforms.md` — Linux 先行例外を r41 で適用中、VMA v3.3.0 は 3 OS 共通 vendor、Mac/Win build 検証は r42-α/β で
- `feedback_build_only_verified.md` — 本 sub-step は 4 INFO marker + 13 prior marker + WARN/ERR 0 件 + shutdown clean を AYA launch verify で実機検証済
- `feedback_self_verify_before_handoff.md` — 本 handoff 起草前に AYA log の INFO marker 4 件 + 13 prior marker + Vulkan WARN/ERR 0 件 + shutdown 2 marker を Claude が grep して self-trace 完了
- `feedback_no_scope_shrink.md` — 3.4-α 設計確定の VMA 内包先行 install scope を縮小せず実装 (extension 検出 marker 追加で 3→4 marker は scope 拡大方向、縮小ではない)
- `feedback_one_step_at_a_time.md` — 本 commit は AYA 明示指示 (session resume 後の handoff prompt 内 5 step 指示) に基づく、次 AYA 確認は 3.4-β-2 細分化 1 件
- `feedback_no_auto_commit.md` — 本 commit は session resume 後の handoff prompt 内 commit 指示受領後実施
- `feedback_proactive_handoff.md` — 3.4 sub-step 境界での能動 handoff 起草 (本 file)
- `feedback_log_reading.md` — AYA launch 後 Claude が `~/.ayastorm_x64/logs/AYAstorm.log` を grep して acceptance marker + Vulkan WARN/ERR 0 件 + 既存 13 marker regression 確認
- `feedback_no_claude_coauthor.md` — 本 commit message に Co-Authored-By: Claude 含めない
- `feedback_release_branch_workflow.md` — 現在 feature branch (`feature/ayastorm-r41-gl-removal`) で作業、release branch 直接 commit ではない
