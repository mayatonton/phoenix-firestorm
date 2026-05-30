# r41 sub-step 3.4-β-2 完遂 → 3.4-γ 着手境界 handoff (2026-05-31)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-4-beta-1-complete.md` (3.4-β-1 完遂 / 3.4-β-2 着手境界) + `handoff-session-pause-2026-05-31-after-3-4-beta-1.md` (β-1 完遂後 session pause、resume 後 β-2 完遂、本 commit 同梱 case A bundle で β-2-1〜β-2-4 一括完遂)
**本 handoff 位置付け**: sub-step 3.4-β-2 (LLImageGL → VkImage + VkImageView lifecycle 並走化、bridging item #8 satisfy) 完遂宣言 + 3.4-γ (set=1 per-material 7 PBR slot layout 配置、bridging item #7 satisfy、sub-doc 07 sub-step 7.3 layout 部分内包) 着手前 scope 確認境界。
**3.4 全体構成 (sub-doc 03 §3.1.4)**: 3.4-α (spec refine、完遂 2026-05-31 commit `a8f8472202`) / 3.4-β-1 (VMA + 共有 pool 基盤、完遂 2026-05-31 commit `4bdadf4eb9`) / **3.4-β-2 (LLImageGL → VkImage lifecycle、本 handoff で完遂宣言)** / 3.4-γ (set=1 per-material layout) / 3.4-δ (12 pool hook body + 段階 2 引継ぎ特殊対応 2 件) / 3.4-ε (3.4 全完遂 handoff)

---

## 1. sub-step 3.4-β-2 完遂 status (2026-05-31)

### 1.1 完遂 marker (sub-doc 03 §3.1.4 3.4-β-2)

| acceptance | 達成 status |
|---|---|
| `vmaCreateImage` 成功 (placeholder 1×1 white) | ✓ `createPlaceholderWhiteImage` で `vmaCreateImage` 成功 (VkImageCreateInfo: 2D / R8G8B8A8_UNORM / 1×1×1 / mip=1 / array=1 / SAMPLE_COUNT_1 / TILING_OPTIMAL / USAGE_TRANSFER_DST+SAMPLED / SHARING_EXCLUSIVE / initialLayout=UNDEFINED、VmaAllocationCreateInfo: USAGE_AUTO)。VMA budget heap 0 DEVICE_LOCAL `allocations=1, blocks=1` で確認 (β-1 時 0/0 から +1) |
| `vkCreateImageView` 成功 | ✓ R8G8B8A8_UNORM 2D view、aspect=COLOR、mip 0/level 1、array 0/layer 1、`sPlaceholderWhiteImageView` 格納 |
| staging upload + 2 段 layout transition | ✓ `uploadPlaceholderWhiteSmoke`: staging buffer (4 byte = 1px RGBA white 0xFFFFFFFF) `vmaCreateBuffer` (USAGE_TRANSFER_SRC + HOST_ACCESS_SEQUENTIAL_WRITE + MAPPED) → scratch 1-time submit command buffer (`sCommandPool` 由来) → barrier UNDEFINED→TRANSFER_DST_OPTIMAL (TOP_OF_PIPE → TRANSFER) → `vkCmdCopyBufferToImage` (1×1 region) → barrier TRANSFER_DST_OPTIMAL→SHADER_READ_ONLY_OPTIMAL (TRANSFER → FRAGMENT_SHADER) → `vkQueueSubmit` + `vkQueueWaitIdle` → scratch cb free + `vmaDestroyBuffer` 即時破棄、Vulkan validation 違反 0 件 |
| `vmaDestroyImage` 成功 | ✓ `destroyPlaceholderWhiteImage` を `shutdownVulkan` 内 `vmaDestroyAllocator` 前に発火 (順序 = placeholder destroy → shared pool destroy → `vmaDestroyAllocator` → `vkDestroyDevice`)、validation 違反 / leak 系 WARN 0 件 |
| format conversion table 配置 | ✓ 公開 API `LLVKLoader::llGlEnumToVkFormat(U32 ll_gl_intformat) → VkFormat` を `llvkloader.h` に追加、impl は `llvkloader.cpp` anon namespace 内 `llGlEnumToVkFormatImpl` (GL header 非依存 = OpenGL spec 確定値 hex literal 照合)。**22 entry** switch (8/16/32-bit normalized + float + depth/stencil + sRGB + packed HDR、sub-doc 07 §1.2.1 set=1 想定 7 PBR slot + LLImageGL 主要 internalformat 網羅)、未対応値 → `VK_FORMAT_UNDEFINED` (caller fallback) |
| INFO marker `"VkImage placeholder lifecycle smoke (1x1 white、VkFormat=R8G8B8A8_UNORM、image+view+destroy 一連 OK)"` | ✓ AYA log L? `llvkloader.cpp:1121` `uploadPlaceholderWhiteSmoke` から hit (spec text 正確一致) |
| INFO marker `"LLImageGL::generateTextures Vulkan path mirror sampled (numTextures=N, per-LLImageGL VkImage 配線は領域 7 sub-step 7.5 持越し)"` | ✓ AYA log `llimagegl.cpp:1268` `generateTextures` から 1 度のみ hit (`static bool sVulkanPathSampled` + `LLVKLoader::isVulkanInitialized()` guard、起動時 numTextures=1 で初回 sampling) |
| VMA budget smoke 再観測 (allocations≥1) | ✓ heap 0 [DEVICE_LOCAL] size=32607 MB / budget=30883 MB / usage=32 MB / `allocations=1` / `blocks=1` (β-1 時 0/0 から placeholder image +1)、heap 1 [HOST] は依然 0/1 (staging buffer は upload 直後に破棄済 = budget smoke 時点で計上されず、期待通り) |
| AYA launch PASS | ✓ 2026-05-30T20:31:51Z 起動完了 + shutdown clean (placeholder destroy + shared pool destroy + `vmaDestroyAllocator` + `vkDestroyDevice` 順) |
| Vulkan 系 WARN/ERR 0 件 | ✓ `grep -E "WARN.*Vulkan\|ERROR.*Vulkan\|VUID-" AYAstorm.log` 0 件 hit (validation=disabled の INFO marker のみ、勘違い grep を除外) |
| 3.3-A/B/C + β-1 全 marker 維持 (regression 0) | ✓ unique INFO #Vulkan# marker 45 件 (prior 43 件 + β-2 新規 2 件)、dynamicRendering enable / VkPipelineCache / Per-frame desc layout / Per-frame UBO buffers / Per-frame descriptor sets / Placeholder PSO / Sky smoke PSO (3.3-B-γ) / Sky placeholder vert binding (3.3-B-δ) / bindTarget dynamic rendering begin (3.3-C-γ) / flush dynamic rendering end (3.3-C-δ) / syncMatrices PerFrame UBO write / syncMatrices TextureMatrix UBO write / syncMatrices modelview push constant (3.3-A-δ-2) / beginDynamicRendering helper (3.3-C-β-2) / VK_EXT_memory_budget 検出 (β-1) / VMA allocator (β-1) / Shared descriptor pool (β-1) / VMA budget smoke 3 line (β-1) 全 hit |
| build clean | ✓ `autobuild build` exit 0、`grep -E "warning:|error:"` で `llvkloader.cpp` / `llimagegl.cpp` 由来 0 件 |

### 1.2 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-session-pause-2026-05-31-after-3-4-beta-1.md` | **役割完了** (3.4-β-2 完遂で session pause 境界 satisfy、本 handoff で内容引継ぎ) |
| `handoff-substep-3-4-beta-1-complete.md` | **役割完了** (3.4-β-1 完遂 handoff、本 handoff で β-1 → β-2 transition 引継ぎ) |
| sub-doc `03-state-machine-pso.md` §3.1.4 3.4-β-2 行 | active 継続 (本 handoff 起草と同時に「完遂 2026-05-31」marker 追加 + 実装結果反映、3.4-γ 着手 ready) |
| sub-doc `07-descriptor-renderpass.md` §1.2.1 set=1 7 PBR slot | active 継続 (本 sub-step touch なし、3.4-γ で layout 配置着手) |
| `handoff-substep-3-4-beta-2-complete.md` (本 handoff) | 新規作成 (3.4-β-2 完遂 → 3.4-γ 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 3.4-γ 着手 ready 状態へ update) |

---

## 2. 実装内容 (commit 範囲、本 commit 同梱)

### 2.1 `indra/llrender/llvkloader.h` (+26)

| 区分 | 内容 |
|---|---|
| β-2 comment block | `// r41 sub-step 3.4-β-2: LLImageGL → VkImage + VkImageView lifecycle 並走化` ヘッダ + scope (format conversion table 公開 / placeholder transit smoke / per-LLImageGL VkImage は 7.5 持越し) + 設計境界 (VmaAllocation TU 限定 / VkImageResource file-local 留置) |
| 公開 API 宣言 | `VkFormat llGlEnumToVkFormat(U32 ll_gl_intformat);` (LLGLenum 互換 U32 受け、未対応 → `VK_FORMAT_UNDEFINED`) |

### 2.2 `indra/llrender/llvkloader.cpp` (+232)

| 区分 | 内容 |
|---|---|
| file-local 静的 (anon namespace) | `sPlaceholderWhiteImage` / `sPlaceholderWhiteImageView` / `sPlaceholderWhiteAllocation` (`VmaAllocation` 型、本 TU 限定 = β-1 設計継承) |
| `llGlEnumToVkFormatImpl(U32)` (anon namespace) | 22 entry switch (GL hex literal → VkFormat): R/RG/RGB/RGBA8 UNORM、R/RG/RGBA16 UNORM、R/RG/RGB/RGBA16F SFLOAT、R/RG/RGBA32F SFLOAT、SRGB8 / SRGB8_ALPHA8、DEPTH_COMPONENT16/24/32F、DEPTH24_STENCIL8、DEPTH32F_STENCIL8、R11F_G11F_B10F、default → UNDEFINED |
| `createPlaceholderWhiteImage()` (anon namespace) | `vmaCreateImage` (1×1 R8G8B8A8_UNORM / USAGE_TRANSFER_DST+SAMPLED) + `vkCreateImageView` (2D / aspect=COLOR)、失敗時 partial cleanup |
| `uploadPlaceholderWhiteSmoke()` (anon namespace) | staging buffer (`vmaCreateBuffer` 4 byte / HOST_ACCESS_SEQUENTIAL_WRITE+MAPPED) → 1px RGBA white 0xFFFFFFFF memcpy → scratch cb `vkAllocateCommandBuffers` (sCommandPool 由来) → barrier UNDEFINED→TRANSFER_DST_OPTIMAL → `vkCmdCopyBufferToImage` (1×1 region) → barrier TRANSFER_DST_OPTIMAL→SHADER_READ_ONLY_OPTIMAL → `vkQueueSubmit` + `vkQueueWaitIdle` → scratch cb `vkFreeCommandBuffers` + `vmaDestroyBuffer` + INFO marker `"VkImage placeholder lifecycle smoke (1x1 white、VkFormat=R8G8B8A8_UNORM、image+view+destroy 一連 OK)"` |
| `destroyPlaceholderWhiteImage()` (anon namespace) | `vkDestroyImageView` + `vmaDestroyImage`、null-guard 付 |
| 公開 API `llGlEnumToVkFormat(U32)` wrapper | namespace LLVKLoader 直下、impl `llGlEnumToVkFormatImpl` 委譲 |
| `initVulkan` 内 wiring | `createVmaAllocator() && createSharedDescriptorPool()` 直後に `createPlaceholderWhiteImage() && uploadPlaceholderWhiteSmoke()` を追加 (failure → `shutdownVulkan` fall-through) |
| `shutdownVulkan` 内 wiring | command pool destroy 直後 / shared pool destroy 前に `destroyPlaceholderWhiteImage()` を call (VMA constraint = sAllocator 生存中に vmaDestroyImage 必須) |

### 2.3 `indra/llrender/llimagegl.cpp` (+13)

| 区分 | 内容 |
|---|---|
| `#include "llvkloader.h"` | `#include "llrender.h"` の直後 (アルファベット順遵守) |
| `LLImageGL::generateTextures` 先頭 hook | `static bool sVulkanPathSampled = false;` + `LLVKLoader::isVulkanInitialized()` check 後 1 度のみ INFO marker `"LLImageGL::generateTextures Vulkan path mirror sampled (numTextures=N, per-LLImageGL VkImage 配線は領域 7 sub-step 7.5 持越し)"` 出力、以降 no-op (per-LLImageGL VkImage 抱合せ配線は領域 7 sub-step 7.5 移管) |

### 2.4 docs / memory

| file | 内容 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` | §3.1.4 3.4-β-2 行 complete 化 (`完遂 2026-05-31` marker + 8 acceptance 実装結果反映、prior 43 marker + β-2 新規 2 marker 集計、build clean 記載) |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-4-beta-2-complete.md` (本 file) | 新規作成 |
| memory `project_ayastorm_r41_vulkan_migration.md` | sub-step 3.4-γ 着手 ready 状態へ update (本 commit と同 cycle で memory file 直接編集) |

---

## 3. 設計判断 trace (本 sub-step 着手後に発生した判断、後続 sub-step で参照対象)

### 3.1 内部 helper 配置 (新規 file 作成せず llvkloader.{cpp,h} 内に packing)

| 判断軸 | 内容 |
|---|---|
| 規模 | β-2 全体で +271 行 (header +26 / cpp +232 / llimagegl.cpp +13)、handoff threshold (~200 行) 周辺だが新規 file 作成不要 |
| 設計境界継承 | β-1 が新規 file 作成せず `llvkloader.cpp` 内 anon namespace 拡張で済ませた前例を継承 (`createVmaAllocator` / `createSharedDescriptorPool` / `logVmaBudgetSmoke` と同 pattern) |
| 過剰抽象化回避 | per-LLImageGL VkImage 抱合せ型 (`LLImageGLVkAdapter` 等の公開型) は 7.5 抱合せ要件確定後に検討 = 本 β-2 では file-local `sPlaceholderWhiteImage` × 3 静的留置で十分 (placeholder smoke 専用)、premature abstraction 回避 |

### 3.2 VmaAllocation の TU 隔離

| 判断軸 | 内容 |
|---|---|
| header 不露出 | `VmaAllocation` 型は `vk_mem_alloc.h` 専属、`llvkloader.h` には持込まない (β-1 設計継承)、公開 API は `llGlEnumToVkFormat(U32) → VkFormat` のみ |
| 集約 struct | `VkImageResource` (VkImage + VkImageView + VmaAllocation 集約) は file-local 留置 (placeholder smoke 専用)、per-LLImageGL 抱合せ型の header 露出は 7.5 で抱合せ要件確定後に検討 |

### 3.3 GL header 非依存実装 (`llGlEnumToVkFormatImpl`)

| 判断軸 | 内容 |
|---|---|
| GL header 非 include | `llvkloader` は GL header (glew.h / GL.h) から独立する設計 = format 変換 switch は OpenGL spec 確定値を hex literal 直書き (`0x8058` = `GL_RGBA8` 等)、コメントで GL enum 名併記 |
| caller 側 U32 受け | LLImageGL は `llgltypes.h` 経由で `LLGLenum = U32` 運用、本 API は `LLGLenum` 互換 U32 受けで GL header 不要 |
| 22 entry 確定 | spec ~20 entry に対し 22 entry (8/16/32-bit normalized + float + depth/stencil + sRGB + packed HDR)、sub-doc 07 §1.2.1 set=1 想定 7 PBR slot + LLImageGL 主要 internalformat 網羅 (`GL_RGBA8` / `GL_RGB16F` / `GL_DEPTH24_STENCIL8` / `GL_SRGB8_ALPHA8` 等) |

### 3.4 staging upload を init 時 1 度限り (scratch cb 経由)

| 判断軸 | 内容 |
|---|---|
| `sCommandBuffer` 流用回避 | `sCommandBuffer` は per-frame primary buffer (`beginFrame` / `endFrame` で使用)、init 時 upload に流用すると per-frame と混在 = init は dedicated scratch cb を `sCommandPool` から `vkAllocateCommandBuffers(commandBufferCount=1)` で 1 回限り確保、submit + `vkQueueWaitIdle` 完了後 `vkFreeCommandBuffers` で即時返却 = standard "one-time submit" pattern 準拠 |
| 2 段 barrier 厳密化 | barrier 1 = `TOP_OF_PIPE → TRANSFER` (`src=0`, `dst=TRANSFER_WRITE`)、barrier 2 = `TRANSFER → FRAGMENT_SHADER` (`src=TRANSFER_WRITE`, `dst=SHADER_READ`) = SHADER_READ_ONLY_OPTIMAL 移行で fragment shader sampling 可能状態へ (3.4-γ で set=1 binding 配信時に再利用) |

### 3.5 `LLImageGL::generateTextures` mirror sampling guard

| 判断軸 | 内容 |
|---|---|
| 1 度のみ guard | `static bool sVulkanPathSampled = false;` + `LLVKLoader::isVulkanInitialized()` check で初回 1 件のみ log、以降 no-op = log 汚染回避 + Vulkan path 通過事実の永続 evidence |
| GL path 単独動作環境で safe | `LLVKLoader::isVulkanInitialized()` が `false` (GL only build / Vulkan init 失敗) の場合は marker skip = GL path 単独動作環境で挙動変化 0 件 |
| per-LLImageGL VkImage 配線の 7.5 持越し明示 | marker text 末尾に `"per-LLImageGL VkImage 配線は領域 7 sub-step 7.5 持越し"` を含め、本 sub-step scope 境界を log 上で明示 (mirror sampling のみ = actual VkImage 配線ではない) |

---

## 4. git / commit / branch status

### 4.1 branch

- `feature/ayastorm-r41-gl-removal` (release branch ではない、`feedback_release_branch_workflow.md` 遵守)
- 本 commit 投入後に AYA push 想定 (`feedback_release_flow.md` 遵守、Claude は local commit まで)

### 4.2 commit 構成 (本 commit 1 件 = case A bundle)

- subject: `feat(r41): sub-step 3.4-β-2 完遂 (VkImage placeholder lifecycle smoke + format conv table 公開 API + LLImageGL Vulkan path mirror marker)`
- body: 完遂 evidence 8 acceptance summary + 設計境界 (案 1 image lifecycle 先行、per-LLImageGL 配線は 7.5 持越し) + spec doc + handoff doc + memory update を案 A 1 commit で投入
- `Co-Authored-By: Claude` 含めない (`feedback_no_claude_coauthor.md` 遵守)

### 4.3 file 変更 summary (本 commit に同梱)

| file | 変更行数 (概算) | 種別 |
|---|---|---|
| `indra/llrender/llvkloader.h` | +26 | impl (β-2 comment block + format conv API 宣言) |
| `indra/llrender/llvkloader.cpp` | +232 | impl (file-local 静的 + 22 entry switch + 3 helper + initVulkan / shutdownVulkan wiring + 公開 wrapper) |
| `indra/llrender/llimagegl.cpp` | +13 | impl (`#include "llvkloader.h"` + generateTextures 1 度 mirror marker) |
| `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` | ~+50 (β-2 row complete 化) | spec |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-4-beta-2-complete.md` | 新規 ~230 行 | docs |

---

## 5. 残作業 / 次 session entry (3.4-γ 着手境界)

### 5.1 次 session 開始時の最低限手順

1. `git fetch origin` + `git status` で AYA push 後の状態同期確認 (`feedback_git_fetch_first.md`)
2. `git log --oneline -5` で本 commit が origin と同期しているか確認
3. 本 handoff doc 通読
4. sub-doc 03 §3.1.4 3.4-γ 行再確認
5. memory `project_ayastorm_r41_vulkan_migration.md` status update 反映確認

### 5.2 sub-step 3.4-γ 着手内容 (sub-doc 03 §3.1.4)

| 項目 | 内容 |
|---|---|
| **scope** | descriptor set=1 per-material 7 PBR slot layout 配置 (bridging item #7 satisfy、§1.5.3 配置、sub-doc 07 sub-step 7.3 layout 部分内包) — `VkDescriptorSetLayout` set=1 作成 (binding 0=DIFFUSE / 1=NORMAL / 2=SPECULAR / 3=BASECOLOR / 4=METALLIC_ROUGHNESS / 5=GLTF_NORMAL / 6=EMISSIVE、descriptor type = `COMBINED_IMAGE_SAMPLER` × 7、stage = `FRAGMENT_BIT`) + helper `allocatePerMaterialDescriptorSet()` / `updatePerMaterialDescriptorSet(VkImageView[7])` / `bindPerMaterialDescriptorSet(VkCommandBuffer)` + sky smoke PSO の VkPipelineLayout を二段構え (set=0 PerFrame + set=1 PerMaterial、push constant 64 B) へ拡張 + β-2 placeholder white texture 1 set 全 7 binding update + transit smoke acceptance |
| **対象 file** | `llvkloader.{cpp,h}` (+~120 行想定) |
| **完了 marker** | set=1 layout 作成成功 (7 binding) + 1 set allocate + β-2 placeholder white texture 7 binding 全 update + `bindPerMaterialDescriptorSet` 動作 + INFO marker `"Per-material descriptor set layout created (set=1, 7 PBR slot binding 0-6, COMBINED_IMAGE_SAMPLER × 7)"` + `"Per-material descriptor set transit smoke (white placeholder × 7 binding, bind via vkCmdBindDescriptorSets)"` + AYA launch PASS + Vulkan 系 WARN/ERR 0 件 + 3.3-A/B/C + β-1/β-2 全 marker 維持 (regression 0) |

### 5.3 3.4-γ 着手 task 候補 (推定 sub-step 内 step、必要なら細分化案を AYA 確認)

| step | 内容 | 推定規模 |
|---|---|---|
| γ-1 | `VkDescriptorSetLayout` set=1 作成 (7 binding = COMBINED_IMAGE_SAMPLER × 7、stage=FRAGMENT_BIT) + INFO marker 1 件 (`"Per-material descriptor set layout created (set=1, 7 PBR slot binding 0-6, COMBINED_IMAGE_SAMPLER × 7)"`) | 30-45 分 |
| γ-2 | sky smoke PSO の VkPipelineLayout を二段構え化 (set=0 PerFrame + set=1 PerMaterial + push constant 64 B)、既存 sPlaceholderLayout は廃止 or 別名化判断 | 45-60 分 |
| γ-3 | `allocatePerMaterialDescriptorSet` / `updatePerMaterialDescriptorSet(VkImageView[7])` / `bindPerMaterialDescriptorSet(VkCommandBuffer)` 3 helper + β-2 placeholder white texture 1 set 全 7 binding update + transit smoke INFO marker (`"Per-material descriptor set transit smoke (white placeholder × 7 binding, bind via vkCmdBindDescriptorSets)"`) | 60-90 分 |
| γ-4 | AYA launch verify + log INFO marker 2 件 (γ-1 + γ-3) + 既存 marker 全継続 verify + spec doc + handoff + commit | 30-45 分 |

着手前に **AYA 確認候補 1 件**: 「3.4-γ を A (γ-1〜γ-4 一括 commit) / B (γ-1+γ-2 / γ-3 / γ-4 verify の 3 commit に細分化) のどちらで進めるか」 (`feedback_one_step_at_a_time.md` 遵守、AYA literal scope 確認、case A は β-2 と同 pattern)。

### 5.4 critical reminders

- **AYAstorm 改変 13 file shader 改変禁止** (charter §2.1 領域 6、`git diff` 0 件維持) — 3.4-γ は llvkloader のみ touch、shader 不変
- **段階 1 + 段階 2 + 3.1b + 3.2 + 3.3-A + 3.3-B + 3.3-C + 3.4-β-1 + 3.4-β-2 動作維持** (sub-doc 03 §3.5、Vulkan path 並走で GL 描画動作維持)
- **3.4-δ (12 pool hook body + 段階 2 引継ぎ特殊対応 2 件) は γ 完遂後着手**、γ で確立 set=1 layout を δ で活用
- **実 per-material descriptor 完全配線 (material cache + ≥80% hit rate + 7 PBR texture 実 binding) は領域 7 sub-step 7.3 持越し**、本 3.4-γ では layout + 1 set transit smoke のみ scope
- **実 attachment 配線 + actual draw 経路統合は領域 7 sub-step 7.5 持越し**
- **validation strict は sub-step 3.5 で別 build により実施** (3.4-γ 着手時も release build で AYA launch + WARN/ERR 0 件 transit acceptance)
- **検証用 LL_INFOS hook は出荷物に残さない** (`feedback_remove_verification_logs.md`、本 sub-step 2 INFO marker は spec 由来の永続 marker = OK、temp diagnostic hook は別途)
- **AYA 確認は 1 件** (3.4-γ 細分化判断、`feedback_one_step_at_a_time.md` 遵守)、決着後に順次次の確認

---

## 6. 関連 doc / memory cross-ref

### 6.1 関連 doc

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter
- `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` — 段階 3 sub-doc (本 handoff で sub-step 3.4-β-2 行 complete 化 + 3.4-γ marker active)
- `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` — 領域 7 sub-doc (sub-step 7.1 内包先行 install 完遂継承、7.3 layout 部分内包は 3.4-γ で着手、7.5 実 attachment 配線は持越し)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-4-beta-1-complete.md` — 3.4-β-1 完遂 handoff (役割完了済、本 handoff で β-1 → β-2 transition 引継ぎ)
- `docs/specs/ayastorm-r41-gl-removal/handoff-session-pause-2026-05-31-after-3-4-beta-1.md` — session pause handoff (役割完了済、本 handoff で 3.4-β-2 完遂を受けて 3.4-γ entry point へ更新)
- `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` — §3.5 二段構え matrix 設計 / §3.6 pool sizing 戦略

### 6.2 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 handoff 完遂で sub-step 3.4-γ 着手 ready 状態に update)
- `project_ayastorm_three_platforms.md` — Linux 先行例外を r41 で適用中、3 OS 共通 vendor / impl 維持
- `feedback_build_only_verified.md` — 本 sub-step は 8 acceptance + 45 marker (prior 43 + 新規 2) + WARN/ERR 0 件 + shutdown clean を AYA launch verify で実機検証済
- `feedback_self_verify_before_handoff.md` — 本 handoff 起草前に AYA log の INFO marker 2 件 + prior 43 marker + Vulkan WARN/ERR 0 件 + VMA allocations=1 + shutdown clean を Claude が grep して self-trace 完了
- `feedback_no_scope_shrink.md` — 3.4-β-2 spec の ~200 行想定 + ~20 entry を縮小せず +271 行 / 22 entry で実装 (scope 拡大方向、縮小ではない)
- `feedback_one_step_at_a_time.md` — 本 commit は AYA 明示指示 (session resume 後の handoff prompt 内 5 step 指示 + case A bundle 承認) に基づく、次 AYA 確認は 3.4-γ 細分化 1 件
- `feedback_no_auto_commit.md` — 本 commit は session resume 後の handoff prompt 内 commit 指示受領後実施
- `feedback_proactive_handoff.md` — 3.4 sub-step 境界での能動 handoff 起草 (本 file)
- `feedback_log_reading.md` — AYA launch 後 Claude が `~/.ayastorm_x64/logs/AYAstorm.log` を grep して acceptance marker + Vulkan WARN/ERR 0 件 + 既存 43 marker regression 確認
- `feedback_no_claude_coauthor.md` — 本 commit message に Co-Authored-By: Claude 含めない
- `feedback_release_branch_workflow.md` — 現在 feature branch (`feature/ayastorm-r41-gl-removal`) で作業、release branch 直接 commit ではない
