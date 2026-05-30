# r41 sub-step 3.4-γ 完遂 → 3.4-δ 着手境界 handoff (2026-05-31)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-4-beta-2-complete.md` (3.4-β-2 完遂 / 3.4-γ 着手境界) + `handoff-session-pause-2026-05-31-after-3-4-beta-2.md` (β-2 完遂後 session pause、resume 後 γ 完遂、本 commit 同梱 case A bundle で γ-1〜γ-3 一括完遂)
**本 handoff 位置付け**: sub-step 3.4-γ (set=1 per-material 7 PBR slot descriptor set layout 配置、bridging item #7 satisfy、sub-doc 07 sub-step 7.3 layout 部分内包) 完遂宣言 + 3.4-δ (12 pool hook body 内 PSO bind + descriptor set bind + push constant + `vkCmdDraw*` 投入 + 段階 2 引継ぎ特殊対応 2 件 = terrain glTexGen 廃止 + avatar SSBO 基本配線) 着手前 scope 確認境界。
**3.4 全体構成 (sub-doc 03 §3.1.4)**: 3.4-α (spec refine、完遂 2026-05-31 commit `a8f8472202`) / 3.4-β-1 (VMA + 共有 pool 基盤、完遂 2026-05-31 commit `4bdadf4eb9`) / 3.4-β-2 (LLImageGL → VkImage lifecycle、完遂 2026-05-31 commit `b84ad42904`) / **3.4-γ (set=1 per-material layout、本 handoff で完遂宣言)** / 3.4-δ (12 pool hook body + 段階 2 引継ぎ特殊対応 2 件) / 3.4-ε (3.4 全完遂 handoff)

---

## 1. sub-step 3.4-γ 完遂 status (2026-05-31)

### 1.1 完遂 marker (sub-doc 03 §3.1.4 3.4-γ)

| acceptance | 達成 status |
|---|---|
| set=1 layout 作成成功 (7 binding) | ✓ `createPerMaterialDescriptorSetLayout` で `VkDescriptorSetLayout` 作成成功 (binding 0=DIFFUSE / 1=NORMAL / 2=SPECULAR / 3=BASECOLOR / 4=METALLIC_ROUGHNESS / 5=GLTF_NORMAL / 6=EMISSIVE、`descriptorType=COMBINED_IMAGE_SAMPLER` × 1 each、`stageFlags=FRAGMENT_BIT`、`pImmutableSamplers=nullptr`)、INFO marker `"Per-material descriptor set layout created (set=1, 7 PBR slot binding 0-6, COMBINED_IMAGE_SAMPLER × 7)"` hit (`llvkloader.cpp:859`) |
| 共用 placeholder sampler 作成成功 | ✓ `createPlaceholderSampler` で `VkSampler` 作成成功 (LINEAR mag/min/mipmap + CLAMP_TO_EDGE U/V/W + `maxLod=VK_LOD_CLAMP_NONE`)。per-texture / per-material sampler (mipmap LOD bias / anisotropy / wrap / border color) は領域 7 sub-step 7.3 残置を spec 内明示 |
| 1 set allocate 成功 | ✓ `allocatePerMaterialDescriptorSet` で `vkAllocateDescriptorSets` 成功 (sSharedDescriptorPool 由来、`descriptorSetCount=1`、`pSetLayouts=&sPerMaterialDescriptorSetLayout`)、`sPerMaterialDescriptorSet` 格納 |
| β-2 placeholder white image view × 7 binding update | ✓ `updatePerMaterialDescriptorSet(VkImageView[7])` で `vkUpdateDescriptorSets` 成功 (7 binding 全部に `sPlaceholderWhiteImageView` + `sPlaceholderSampler` + `imageLayout=SHADER_READ_ONLY_OPTIMAL`)、INFO marker `"Per-material descriptor set transit smoke (white placeholder × 7 binding, bind via vkCmdBindDescriptorSets)"` hit (`llvkloader.cpp:947`) |
| `bindPerMaterialDescriptorSet` 動作 | ✓ `beginFrame` 内 `vkCmdBindPipeline(sPlaceholderPipeline)` 直後に `bindPerMaterialDescriptorSet(sCommandBuffer)` 呼出、`vkCmdBindDescriptorSets(GRAPHICS, sPlaceholderLayout, firstSet=1, descriptorSetCount=1, &sPerMaterialDescriptorSet, 0, nullptr)` 投入動作、Vulkan validation 違反 0 件 |
| `sPlaceholderLayout` + `sSkySmokeLayout` 二段構え化 | ✓ 両 PipelineLayout を `set_layouts[2] = { sPerFrameDescriptorSetLayout, sPerMaterialDescriptorSetLayout }` + `setLayoutCount=2` へ拡張、push constant 64 B / VERTEX_BIT (modelview_matrix) は継承、PSO compile 成功 (`Placeholder PSO compiled` + `Sky smoke PSO compiled via SPIR-V build chain` marker 全継続)。placeholder / sky smoke SPIR-V は set=1 binding 未参照 = Vulkan 仕様 (layout が含む set 番号 ≥ shader 参照は許容) で validation 0 件 |
| shutdownVulkan clean | ✓ teardown 順序 = sPerFrame layout destroy → sPerMaterial layout destroy → sPerMaterialDescriptorSet null reset (pool 経由 auto free) → pipeline cache destroy → command pool destroy → placeholder image destroy → sSharedDescriptorPool destroy (set 自動 free) → sPlaceholderSampler destroy → vmaDestroyAllocator → vkDestroyDevice、validation 違反 0 件 |
| INFO marker `"Per-material descriptor set layout created (set=1, 7 PBR slot binding 0-6, COMBINED_IMAGE_SAMPLER × 7)"` | ✓ AYA log 2026-05-30T21:10:13Z `llvkloader.cpp:859` `createPerMaterialDescriptorSetLayout` から hit (spec text 正確一致) |
| INFO marker `"Per-material descriptor set transit smoke (white placeholder × 7 binding, bind via vkCmdBindDescriptorSets)"` | ✓ AYA log 2026-05-30T21:10:13Z `llvkloader.cpp:947` `updatePerMaterialDescriptorSet` から hit (spec text 正確一致) |
| VMA budget 再観測 (γ 影響無し確認) | ✓ heap 0 [DEVICE_LOCAL] `allocations=1, blocks=1` (β-2 baseline 保持、γ は sampler を sDevice 直属 / set を pool 経由 allocate のため VMA カウント増えず期待通り)、heap 1 [HOST] `allocations=0, blocks=1` (継続) |
| AYA launch PASS | ✓ 2026-05-30T21:10:13Z 起動完了 + 2026-05-30T21:11:29Z shutdown clean (Vulkan device destroyed → Vulkan instance destroyed) |
| Vulkan 系 WARN/ERR 0 件 | ✓ `grep -E "WARNING.*Vulkan\|ERROR.*Vulkan\|VUID-"` 0 件 hit (font/Settings WARNING は無関係) |
| 3.3-A/B/C + β-1/β-2 全 marker 維持 (regression 0) | ✓ unique INFO #Vulkan# marker 47 件 (prior 45 件 + γ 新規 2 件)、prior 45 marker (β-1 + β-2 確立した 45 件 = dynamicRendering enable / VkPipelineCache / Per-frame desc layout / Per-frame UBO / Per-frame descriptor sets / Placeholder PSO / Sky smoke PSO + vert binding / bindTarget begin / flush end / syncMatrices PerFrame UBO / syncMatrices TextureMatrix UBO / syncMatrices modelview push constant / beginDynamicRendering helper / VK_EXT_memory_budget / VMA allocator / Shared descriptor pool / VMA budget smoke 3 line / VkImage placeholder lifecycle smoke / generateTextures mirror 含む) 全 hit |
| build clean | ✓ `autobuild build` exit 0、`grep -E "warning:\|error:"` で `llvkloader.cpp` 由来 0 件 |

### 1.2 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-session-pause-2026-05-31-after-3-4-beta-2.md` | **役割完了** (3.4-γ 完遂で session pause 境界 satisfy、本 handoff で内容引継ぎ) |
| `handoff-substep-3-4-beta-2-complete.md` | **役割完了** (3.4-β-2 完遂 handoff、本 handoff で β-2 → γ transition 引継ぎ) |
| sub-doc `03-state-machine-pso.md` §3.1.4 3.4-γ 行 | active 継続 (本 handoff 起草と同時に「完遂 2026-05-31」marker 追加 + 実装結果反映、3.4-δ 着手 ready) |
| sub-doc `07-descriptor-renderpass.md` §1.2.1 set=1 7 PBR slot | active 継続 (layout 部分は γ で内包、material params UBO + cache 本実装は 7.3 残置を spec 内明示) |
| `handoff-substep-3-4-gamma-complete.md` (本 handoff) | 新規作成 (3.4-γ 完遂 → 3.4-δ 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 3.4-δ 着手 ready 状態へ update) |

---

## 2. 実装内容 (commit 範囲、本 commit 同梱)

### 2.1 `indra/llrender/llvkloader.cpp` (+217 / -10、net +207)

| 区分 | 内容 |
|---|---|
| γ comment block (anon namespace 内) | `// r41 sub-step 3.4-γ (sub-doc 03 §3.1.4 / sub-doc 07 §1.2.1 set=1 / §3.1 sub-step 7.3 layout 部分内包):` ヘッダ + scope (per-material 7 PBR slot layout + 共用 placeholder sampler + 1 transit smoke set / binding 0..6 = DIFFUSE/NORMAL/SPECULAR/BASECOLOR/METALLIC_ROUGHNESS/GLTF_NORMAL/EMISSIVE / descriptor type = COMBINED_IMAGE_SAMPLER × 7 / stage = FRAGMENT_BIT) |
| file-local 静的 (anon namespace) | `constexpr U32 PER_MATERIAL_BINDING_COUNT = 7;` + `VkDescriptorSetLayout sPerMaterialDescriptorSetLayout = VK_NULL_HANDLE;` + `VkSampler sPlaceholderSampler = VK_NULL_HANDLE;` + `VkDescriptorSet sPerMaterialDescriptorSet = VK_NULL_HANDLE;` |
| `createPerMaterialDescriptorSetLayout()` (anon namespace) | 7 binding 全部 COMBINED_IMAGE_SAMPLER × 1 / FRAGMENT_BIT / immutable null で `VkDescriptorSetLayoutBinding[7]` 構築 → `vkCreateDescriptorSetLayout` 発火 → 成功時 INFO marker (`Per-material descriptor set layout created...`)、失敗時 WARN + false 返し |
| `createPlaceholderSampler()` (anon namespace) | `VkSamplerCreateInfo` magFilter/minFilter=LINEAR、mipmapMode=LINEAR、addressMode U/V/W=CLAMP_TO_EDGE、maxLod=VK_LOD_CLAMP_NONE で `vkCreateSampler` 発火、失敗時 WARN + false 返し (INFO marker 無し、γ 範囲外で 1 件 sampler の事実は層内 trivial) |
| `allocatePerMaterialDescriptorSet()` (anon namespace) | sSharedDescriptorPool + sPerMaterialDescriptorSetLayout null guard → `VkDescriptorSetAllocateInfo` で `vkAllocateDescriptorSets` 1 set、失敗時 WARN + false 返し |
| `updatePerMaterialDescriptorSet(const VkImageView image_views[7])` (anon namespace) | sPerMaterialDescriptorSet + sPlaceholderSampler null guard → `VkDescriptorImageInfo[7]` (sampler=sPlaceholderSampler / imageView=image_views[i] / imageLayout=SHADER_READ_ONLY_OPTIMAL) + `VkWriteDescriptorSet[7]` (dstBinding=i / descriptorCount=1 / descriptorType=COMBINED_IMAGE_SAMPLER) → `vkUpdateDescriptorSets` 7 件一括 + INFO marker (`Per-material descriptor set transit smoke...`) |
| `bindPerMaterialDescriptorSet(VkCommandBuffer cmd_buf)` (anon namespace) | cmd_buf / sPlaceholderLayout / sPerMaterialDescriptorSet 3 件 null guard → `vkCmdBindDescriptorSets(GRAPHICS, sPlaceholderLayout, firstSet=1, descriptorSetCount=1, &sPerMaterialDescriptorSet, 0, nullptr)` 投入。in-frame guard / sCommandBuffer 有効性は caller 側 (beginFrame 内 sPlaceholderPipeline bind 直後) 前提 |
| `sPlaceholderLayout` 二段構え化 (line ~1561) | `VkDescriptorSetLayout set_layouts[2] = { sPerFrameDescriptorSetLayout, sPerMaterialDescriptorSetLayout };` + `createStandardPipelineLayout(set_layouts, 2, push_constants, 1)` (push constant 64 B / VERTEX_BIT は継承)。placeholder SPIR-V は set=1 未参照 = Vulkan 仕様で validation 0 件 |
| `sSkySmokeLayout` 二段構え化 (line ~1784) | sPlaceholderLayout と同 pattern、δ で recordSkySmokeDraw 経由 12 pool hook body に組込予定の準備 |
| `initVulkan` 内 wiring | createPerFrameDescriptorSets 完遂後 / createPlaceholderPipeline 前に `createPerMaterialDescriptorSetLayout() && createPlaceholderSampler()` 追加 (失敗 → shutdownVulkan fall-through)、createSkySmokePipeline 完遂後 / logVmaBudgetSmoke 前に `allocatePerMaterialDescriptorSet()` + 直後 brace-scope で `VkImageView views[7]` を `sPlaceholderWhiteImageView` で埋めて `updatePerMaterialDescriptorSet(views)` 投入 |
| `shutdownVulkan` 内 wiring | sPerFrameDescriptorSetLayout destroy 直後に sPerMaterialDescriptorSetLayout destroy + sPerMaterialDescriptorSet null reset (pool 経由 auto free) 追加、sSharedDescriptorPool destroy 直後 / vmaDestroyAllocator 直前に sPlaceholderSampler destroy 追加 |
| `beginFrame` 内 wiring (line ~2218) | `vkCmdBindPipeline(sCommandBuffer, GRAPHICS, sPlaceholderPipeline)` 直後に `bindPerMaterialDescriptorSet(sCommandBuffer)` 呼出を追加 (sub-doc 03 §3.1.4 / sub-doc 07 §1.2.1 set=1 cross-ref comment 付) |

### 2.2 file 変更無し: `llvkloader.h` / `llimagegl.{cpp,h}` / shader / SPIR-V

- `llvkloader.h`: γ helper 5 件は anon namespace file-local 留置、header 露出 0 件 (per-LLImageGL VkImage 抱合せ要件確定後 7.5 で再評価)
- `llimagegl.{cpp,h}`: γ は llimagegl touch せず (β-2 で導入した generateTextures mirror marker は継続)
- shader (`indra/newview/app_settings/shaders/`) / SPIR-V build chain: γ は touch せず (charter §2.1 領域 6 改変禁止)

---

## 3. AYA launch verify 詳細 (2026-05-31)

### 3.1 launch sequence

| 時刻 | event |
|---|---|
| 2026-05-30T21:10:12Z | Settings load (Vulkan 系 WARNING 無し) |
| 2026-05-30T21:10:13Z | `initVulkan` 着手 → loader v1.4.319 取得 → instance/device 作成 → command pool → offscreen image → render pass → framebuffer → VkPipelineCache → VMA allocator → Shared descriptor pool → placeholder image lifecycle smoke → PerFrame layout/UBO/sets → **PerMaterial layout + Placeholder sampler 作成 (γ 新規 1)** → Placeholder PSO compile (二段構え layout 経由) → Sky smoke PSO compile (二段構え layout 経由) → **PerMaterial set allocate + 7 binding update (γ 新規 2 = transit smoke)** → VMA budget smoke |
| 2026-05-30T21:10:13Z | `beginFrame` 進入 → vkCmdBindPipeline(sPlaceholderPipeline) → **bindPerMaterialDescriptorSet (firstSet=1、γ wiring 経路)** → beginDynamicRendering no-op → writeCurrentPerFrameMatrixUBO / writeCurrentTextureMatrixUBO 動作 |
| 2026-05-30T21:10:14Z | LLRenderTarget bindTarget / flush dynamic rendering API surface 並走 (3.3-C-γ/δ 継続) + beginDynamicRendering helper 動作 |
| 2026-05-30T21:10:15Z | pushCurrentModelviewMatrix (frame_index=1) 動作 (3.3-A-δ-2 継続) |
| 2026-05-30T21:11:29Z | AYA 起動終了 → shutdownVulkan 完遂 (`Vulkan device destroyed` + `Vulkan instance destroyed` marker hit、validation 違反 0 件) |

### 3.2 unique INFO #Vulkan# marker 47 件 (frame_index 等正規化済)

prior 45 件 (β-2 baseline) + γ 新規 2 件:
1. **(γ 新規)** `Per-material descriptor set layout created (set=1, 7 PBR slot binding 0-6, COMBINED_IMAGE_SAMPLER × 7)` (`llvkloader.cpp:859`)
2. **(γ 新規)** `Per-material descriptor set transit smoke (white placeholder × 7 binding, bind via vkCmdBindDescriptorSets)` (`llvkloader.cpp:947`)

prior 45 件継続 (代表的なもの、regression 0):
- `Initializing Vulkan loader...` / `Vulkan loader version 1.4.319` / `Vulkan instance created (validation=disabled)` / `Found 2 physical device(s)` + Candidate × 2 + Selected / `Graphics queue family: 0` / Device limit baseline × 8 line / `Device feature shaderClipDistance enabled` / `Vulkan device created` / `Vulkan 1.3 dynamicRendering feature enabled` / `Command pool + primary command buffer created` / `Offscreen image 64x64 created` / `Minimal render pass created` / `Framebuffer created` / `VkPipelineCache created` / `VMA allocator created (Vulkan 1.3, dynamic functions via volk, memory_budget=ON)` / `Shared descriptor pool created (3.4-β-1 placeholder, maxSets=200, UBO=16, COMBINED_IMAGE_SAMPLER=64; precision deferred to 7.3)` / `VkImage placeholder lifecycle smoke (1x1 white、VkFormat=R8G8B8A8_UNORM、image+view+destroy 一連 OK)` / `Per-frame descriptor set layout created (binding 0=PerFrameMatrixUBO, 1=TextureMatrixUBO)` / `Per-frame UBO buffers created (3 frames × 512 B, HOST_VISIBLE_COHERENT + persistent map + zero write)` / `Per-frame descriptor sets allocated + updated (3 sets × 2 binding)` / `Placeholder PSO compiled (vert 752 B / frag 408 B, sky pool placeholder for sub-doc 03 §3.1 acceptance)` / SPIR-V shader module loaded × 2 / `Sky smoke PSO compiled via SPIR-V build chain (sub-step 3.3-B-γ)` / `Sky placeholder vert binding active (PerFrameMatrixUBO + push constant modelview)` / `VMA budget smoke (3.4-β-1 1 度のみ、heapCount=2, VK_EXT_memory_budget=ON):` + heap 0/1 line / `LLImageGL::generateTextures Vulkan path mirror sampled` / `LLRenderTarget::bindTarget : bindTarget dynamic rendering begin path active` / `LLRenderTarget::flush : flush dynamic rendering end path active` / `syncMatrices PerFrame UBO write path active` / `syncMatrices TextureMatrix UBO write path active` / `beginDynamicRendering : dynamic rendering helper path active` / `syncMatrices modelview push constant path active` / `Vulkan device destroyed` / `Vulkan instance destroyed`

### 3.3 Vulkan WARN/ERR 0 件

`grep -E "WARNING.*Vulkan|ERROR.*Vulkan|VUID-" AYAstorm.log` 0 件 hit。出現する WARNING は `#Settings#` (fsdata_defaults.7.2.4.xml not found 等 = startup 既知) + `#` (font system) + `#Texture#` (downscaling 既知) のみ、Vulkan path 由来 0 件。

### 3.4 VMA budget heap 観測

```
heap 0 [DEVICE_LOCAL] size=32607 MB budget=30878 MB usage=32 MB (VMA allocations=1, blocks=1)
heap 1 [HOST]         size=48001 MB budget=48001 MB usage=35 MB (VMA allocations=0, blocks=1)
```

β-2 baseline (heap 0 = 1/1、heap 1 = 0/1) と完全一致。γ 追加 resource = sampler (sDevice 直属 / VMA 非経由) + descriptor set (sSharedDescriptorPool 経由 / 内部 alloc は driver 担当 / VMA 非経由) のため VMA counter 増えず期待通り。

---

## 4. bridging item / spec cross-ref satisfy 状況

### 4.1 §1.5.3 bridging items 9 件 (sub-doc 01)

| # | item | satisfy 状況 |
|---|---|---|
| #1 | `LLGLState` setter dead-store 化 | (段階 4 LLPipelineFrameContext 配置時 satisfy 計画、γ touch 無し) |
| #2 | `VK_EXT_extended_dynamic_state2` dynamic state 配線 | 3.1 完遂時 satisfy 済 |
| #3 | `VK_KHR_push_descriptor` 経由 set=2 push descriptor 全配線 | (δ で部分内包 + 領域 7 sub-step 7.4 残置) |
| #4 | `LLGLUserClipPlane` PSO 化 | 3.1 完遂時 satisfy 済 |
| #5 | shader 11 hit (sub-doc 04 §3) | (段階 4 領域 5 で satisfy 計画) |
| #6 | LL coordinate convention 注釈 | 3.1 完遂時 satisfy 済 (`projection_matrix` doxygen) |
| **#7** | **set=1 per-material 7 PBR slot layout** | **✓ γ で satisfy** (本 sub-step 完遂) |
| #8 | LLImageGL → VkImage + VkImageView lifecycle | ✓ β-2 で satisfy 済 |
| #9 | swap chain present mode | (段階 4 領域 8 で satisfy 計画) |
| #10 | `VK_EXT_debug_utils` messenger 経由 GL ARB debug callback 置換 | 3.1 完遂時 satisfy 済 |
| #11 | `LLGLSyncFence` 物理削除 | 3.1 完遂時 satisfy 済 |

### 4.2 sub-doc 07 §3.1 領域 7 sub-step 内包状況

| sub-step | 内包状況 |
|---|---|
| 7.1 (VMA + descriptor pool sizing 基盤) | ✓ β-1 で内包先行 install 完遂 |
| 7.2 (set=0 per-frame 全配線 ~30 sampler 集約) | (3.3-A で PerFrameMatrixUBO + TextureMatrixUBO 配線済、shadow/env cubemap/LUT は領域 7 残置) |
| **7.3 (set=1 per-material layout)** | **✓ γ で layout 部分内包**、material params UBO + per-material sampler + ~50 material cache (×3 frame in flight = 150 pool sizing) 本実装は 7.3 残置 |
| 7.4 (set=2 push descriptor) | (δ で `vkCmdPushDescriptorSetKHR` 基本配線部分内包予定、本配線は 7.4 残置) |
| 7.5 (7 pass chain + dynamic rendering 実 attachment) | (3.3-C で API surface 並走済、実 attachment は 7.5 残置 = LLImageGL → VkImage の object lifecycle のみ β-2 で satisfy) |

---

## 5. 3.4-δ 着手境界 (本 handoff 役割引継)

### 5.1 3.4-δ scope (sub-doc 03 §3.1.4)

- **12 pool hook body 内 PSO bind + descriptor set bind + push constant + `vkCmdDraw*` 投入**: 3.3-B-δ で受領済 shader 側 binding (set=0 + push constant) + γ で確立 set=1 layout を活用、12 pool 全 `recordPoolDraws` hook body に `vkCmdBindPipeline(sSkySmokePipeline)` + `vkCmdBindDescriptorSets(set=0+set=1)` + `vkCmdPushConstants(modelview 64 B)` + `vkCmdDraw(3, 1, 0, 0)` (fullscreen triangle placeholder) 投入
- **段階 2 引継ぎ特殊対応 2 件**:
  - **terrain glTexGen 物理削除** (`lldrawpoolterrain.cpp`、`grep -E "glTexGen"` 0 件 hit acceptance)
  - **avatar bone matrix → VkBuffer (storage buffer)**: sub-doc 07 sub-step 7.4 push descriptor 部分内包 = `vkCmdPushDescriptorSetKHR` 経由 set=2 binding 0 = `VkBuffer` storage 投入
- **触らない境界**:
  - 実 attachment 配線 (`VkImage`/`VkImageView` 実体 = LLRenderTarget 並走 Vulkan side) は領域 7 sub-step 7.5 残置 (3.3-C placeholder attachment 経由 transit smoke 状態で acceptance)
  - per-material sampler 本配線 + material params UBO + ~50 material cache は領域 7 sub-step 7.3 残置
  - AYAstorm 改変 13 file shader 改変禁止 (charter §2.1 領域 6、`git diff` 0 件維持)

### 5.2 想定 file / LOC

- `indra/newview/lldrawpool*.cpp` 12 file + `lldrawpoolterrain.cpp` + `lldrawpoolavatar.cpp` + `indra/llrender/llvkloader.{cpp,h}` (+~300 行想定)
- 各 pool `recordPoolDraws(VkCommandBuffer)` hook body 内側で 4 件 vkCmd*** 投入 (β-1/β-2/γ で確立した sPlaceholderPipeline / sSkySmokePipeline / sPerFrameDescriptorSet[i] / sPerMaterialDescriptorSet / push constant 64B / 共有 layout を組合せ)

### 5.3 acceptance (sub-doc 03 §3.1.4 3.4-δ)

- 12 pool 全 hook 内 `vkCmdDraw*` 動作 (release build 1 frame 完走)
- render path GL call 削除 (acceptance #1-段階 2 satisfy = `grep -rE "gl[A-Z][a-zA-Z]+\s*\(" indra/newview/lldrawpool*.cpp` **0 件 hit**)
- `grep -E "glTexGen" indra/newview/lldrawpoolterrain.cpp` **0 件 hit**
- avatar.cpp 内 bone matrix → VkBuffer 配線確認
- INFO marker (per-pool draw 1 件 + terrain glTexGen 廃止 1 件 + avatar SSBO 基本配線 1 件)
- AYA launch PASS + Vulkan 系 WARN/ERR 0 件
- 3.3-A/B/C + β-1/β-2/γ 全 marker 維持 (regression 0)
- 視覚 regression 0 (GL path 並走で本実描画維持)

---

## 6. resume entry point (3.4-δ 着手用)

### 6.1 必読 (resume 開幕順)

1. 本 handoff (`handoff-substep-3-4-gamma-complete.md`、3.4-γ 完遂後 entry point)
2. sub-doc 03 §3.1.4 (3.4 細分化 spec、3.4-δ 行)
3. sub-doc 07 §3.1 sub-step 7.4 (set=2 push descriptor、δ 部分内包境界)
4. memory `project_ayastorm_r41_vulkan_migration.md` (sub-step 3.4-δ 着手 ready 状態確認)
5. `indra/llrender/llvkloader.{cpp,h}` (γ 確立 set=1 layout + bind helper、δ で活用)

### 6.2 case A/B 進行粒度 (resume 開幕 AYA 確認 1 件)

3.4-δ は scope が 12 pool 全数 + 段階 2 特殊対応 2 件で β-2/γ より広範。AYA に粒度確認 (案 A = 1 commit bundle、案 B = δ-1 pool hook unification + δ-2 12 pool body + δ-3 terrain glTexGen 廃止 + δ-4 avatar SSBO 基本配線 + δ-5 verify の 5 commit 分割、案 C = pool 6+6 で δ-1/δ-2 二分割)。

---

## 7. proactive handoff チェック

- `feedback_proactive_handoff.md` 遵守: 3.4-γ 完遂境界で本 handoff doc 起草 (AYA 指示待たず能動投入、β-1 → β-2 / β-2 → γ transition と同 pattern 継承)
- 3.4-δ 着手直後の session pause 可能性 (12 pool 全数 + 段階 2 特殊対応 2 件で context 残量逼迫予想): δ 着手直前で再度能動 session pause handoff 起草を検討
