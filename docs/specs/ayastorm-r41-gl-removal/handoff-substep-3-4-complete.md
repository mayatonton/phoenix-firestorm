# r41 sub-step 3.4 全完遂 → 3.5 着手境界 handoff (2026-05-31)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-4-gamma-complete.md` (3.4-γ 完遂 / 3.4-δ 着手境界)
**本 handoff 位置付け**: sub-step 3.4 (texture lifecycle + descriptor set=1 per-material + 12 pool hook body + 段階 2 引継ぎ特殊対応 2 件) **全完遂宣言** + sub-step 3.5 (段階 3 self-check + validation strict 検証 + `handoff-stage-3-complete.md` 起草) 着手前 scope 確認境界。
**3.4 全体構成 (sub-doc 03 §3.1.4)**: 3.4-α (spec refine、commit `a8f8472202`) / 3.4-β-1 (VMA + 共有 pool 基盤、commit `4bdadf4eb9`) / 3.4-β-2 (LLImageGL → VkImage lifecycle、commit `b84ad42904`) / 3.4-γ (set=1 per-material layout、commit `8f0a8eaa82`) / 3.4-δ (12 pool hook body + 段階 2 引継ぎ特殊対応 2 件) / **3.4-ε (本 handoff doc + sub-doc/memory complete 化)**

**3.4-δ 分割構成 (案 B 5 commit)**:
- 3.4-δ-1 (commit `4e6e75257f`): 12 pool 共用 placeholder draw helper unification (`recordSkySmokeDraw` → `recordPlaceholderPoolDraw`、`bindPerMaterialDescriptorSet` 引数化 [VkCommandBuffer + VkPipelineLayout])
- 3.4-δ-2 (commit `37f5b65916`): 11 pool body 投入 (Sky 以外、各 file に `#include "llvkloader.h"` + `LLVKLoader::recordPlaceholderPoolDraw(cmd_buf)` 1 line 配線)
- 3.4-δ-3 (commit `f88e9ed464`): terrain glTexGen 物理削除 (Option D = `renderFull4TU` + `renderFull2TU` + `renderSimple` 3 関数本体丸ごと撤去、net -382 行)
- 3.4-δ-4 (commit `05686ddd03`): avatar bone matrix → VkBuffer (storage) 基本配線 (`VK_KHR_push_descriptor` enable + Case B [avatar 専用 layout + 専用 helper、11 pool 共用 helper 不変])
- 3.4-δ-5 (**本 commit、doc-only**): AYA launch verify PASS 確認 + sub-doc 03 §3.1.4 / §3.1 update + handoff doc 起草 + memory update

---

## 1. sub-step 3.4 全完遂 status (2026-05-31)

### 1.1 完遂 marker (sub-doc 03 §3.1.4)

| sub-step | 完遂 marker |
|---|---|
| **3.4-α** | ✓ spec refine commit `a8f8472202` 投入 (sub-doc 03 §3.1.4 新規 + 設計根拠 trace inventory 13 行 + sub-doc 07 §3.1 sub-step 7.1/7.3/7.4 cross-ref) |
| **3.4-β-1** | ✓ VMA v3.3.0 vendor 配置 + `VmaAllocator` init + 共有 descriptor pool 雛形 + `VK_EXT_memory_budget` 検出/enable + 4 INFO marker (commit `4bdadf4eb9`、領域 7 sub-step 7.1 内包先行 install) |
| **3.4-β-2** | ✓ LLImageGL → VkImage + VkImageView lifecycle 並走化 + format conversion 22 entry 公開 API + `LLImageGL::generateTextures` mirror marker + VMA budget heap 0 `allocations=1` 確認 (commit `b84ad42904`、bridging item #8 satisfy) |
| **3.4-γ** | ✓ set=1 per-material 7 PBR slot descriptor set layout 配置 + 共用 placeholder sampler + 5 helper + `sPlaceholderLayout` / `sSkySmokeLayout` 二段構え化 + `bindPerMaterialDescriptorSet` 配線 + 7 binding 全 white placeholder transit smoke (commit `8f0a8eaa82`、bridging item #7 satisfy、領域 7 sub-step 7.3 layout 部分内包) |
| **3.4-δ-1** | ✓ 12 pool 共用 placeholder draw helper unification (`recordSkySmokeDraw` → `recordPlaceholderPoolDraw`、body 拡張 = PSO bind + set=0 PerFrame + set=1 PerMaterial + push constant 64 B + `vkCmdDraw(3,1,0,0)`、commit `4e6e75257f`) |
| **3.4-δ-2** | ✓ 11 pool body 投入 (alpha/pbropaque/simple/water/bump/terrain/avatar/materials/tree/waterexclusion/wlsky に `LLVKLoader::recordPlaceholderPoolDraw(cmd_buf)` 配線 + `#include "llvkloader.h"` 追加、commit `37f5b65916`) |
| **3.4-δ-3** | ✓ terrain glTexGen 物理削除 (Option D dead-code 撤去、`renderFull4TU` + `renderFull2TU` + `renderSimple` 3 関数本体丸ごと削除、`grep -E "glTexGen" indra/newview/lldrawpoolterrain.cpp` **0 件 hit**、net -382 行、commit `f88e9ed464`) |
| **3.4-δ-4** | ✓ avatar bone matrix → VkBuffer (storage) 基本配線 (`VK_KHR_push_descriptor` enable + Case B [avatar 専用 layout + 専用 helper、11 pool 共用 helper 不変] + AVATAR_BONE_MATRIX_COUNT=110 + `sAvatarBone*` 6 statics + 3 helper + `recordAvatarPlaceholderDraw` 公開 API、commit `05686ddd03`、領域 7 sub-step 7.4 push descriptor 部分内包) |
| **3.4-ε** (本 commit) | ✓ AYA launch verify PASS 確認 + sub-doc 03 §3.1.4 / §3.1 sub-step 3.4 行 complete 化 + memory `project_ayastorm_r41_vulkan_migration.md` を 3.5 着手 ready 状態へ update + 本 handoff doc 起草 |
| **render path GL call 削除** | ✓ acceptance #1-段階 2 satisfy (`grep -rE "gl[A-Z][a-zA-Z]+\s*\(" indra/newview/lldrawpool*.cpp` 0 件 hit、段階 2 hook wiring + 段階 3 sub-step 3.4-δ で body 配線完了) |
| **段階 2 引継ぎ特殊対応 2 件** | ✓ terrain glTexGen 廃止 (δ-3) + avatar bone matrix → VkBuffer (δ-4) 両件 satisfy |
| **AYA launch verify PASS** | ✓ 2026-05-30T22:20:23Z 起動完了 + 2026-05-30T22:21:40Z shutdown clean (Vulkan device destroyed → Vulkan instance destroyed、validation 違反 0 件) |
| **Vulkan 系 WARN/ERR 0 件** | ✓ `grep -E "WARNING.*Vulkan|ERROR.*Vulkan|VUID-"` 0 件 hit |
| **3.3-A/B/C + β-1/β-2/γ 全 marker 維持 (regression 0)** | ✓ unique INFO #Vulkan# marker 54 件 (γ baseline 47 件 + δ 新規 7 件) + 12 #VkRecord# pool hook one-shot marker 全 hit |
| **build clean** | ✓ `autobuild build` exit 0、`compile warning/error` 0 件、package tar.xz 生成完了 |

### 1.2 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-3-4-gamma-complete.md` | **役割完了** (3.4-γ 完遂 → 3.4-δ 着手境界 charter、本 handoff で δ 完遂 + 3.4 全完遂引継ぎ) |
| `handoff-session-pause-2026-05-31-after-3-4-beta-1.md` | **役割完了** (β-1 → β-2 transition、prior session で内容引継ぎ済) |
| `handoff-session-pause-2026-05-31-after-3-4-beta-2.md` | **役割完了** (β-2 → γ transition、prior session で内容引継ぎ済) |
| sub-doc `03-state-machine-pso.md` §3.1.4 3.4-δ 行 + 3.4-ε 行 | 本 commit で「完遂 2026-05-31」marker 追加 + 実装結果反映 |
| sub-doc `03-state-machine-pso.md` §3.1 3.4 行 | 本 commit で「完遂 2026-05-31」反映、3.5 着手 ready |
| sub-doc `07-descriptor-renderpass.md` §3.1 sub-step 7.4 | (本領域 7 sub-step 7.4 着手時の前倒し記録は既 spec 内記述済 line 128、追加更新不要) |
| `handoff-substep-3-4-complete.md` (本 handoff) | 新規作成 (3.4 全完遂 → 3.5 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | 本 commit で 3.5 着手 ready 状態へ update |
| `MEMORY.md` index entry | 本 commit で 3.4 全完遂 + 段階 2 引継ぎ全 satisfy 反映 |

---

## 2. 実装内容 (commit 範囲)

### 2.1 3.4-δ-1 (commit `4e6e75257f`、`llvkloader.cpp` +96 / -29)

| 区分 | 内容 |
|---|---|
| `recordSkySmokeDraw` → `recordPlaceholderPoolDraw` rename | 12 pool 共用 placeholder draw helper として一般化 (sky 限定 naming 撤去) |
| `recordPlaceholderPoolDraw` body 拡張 | PSO bind `sSkySmokePipeline` + set=0 PerFrame descriptor (`sPerFrameDescriptorSet[sFrameIndex]`) + set=1 PerMaterial descriptor (`bindPerMaterialDescriptorSet(cmd_buf, sSkySmokeLayout)`) + push constant 64 B identity modelview / VERTEX_BIT + `vkCmdDraw(3, 1, 0, 0)` + 1 度のみ INFO marker `"Placeholder pool draw fired (PSO bind sSkySmokePipeline + set=0 PerFrame + set=1 PerMaterial + push constant 64 B identity / VERTEX_BIT + vkCmdDraw(3,1,0,0))"` |
| `bindPerMaterialDescriptorSet` 引数 (VkCommandBuffer, VkPipelineLayout) 化 | 両 layout (sPlaceholderLayout / sSkySmokeLayout) は γ 二段構え準拠 (set=0 PerFrame + set=1 PerMaterial + push constant 64 B) で descriptor set 互換性確保、beginFrame caller は sPlaceholderLayout 継承、recordPlaceholderPoolDraw caller は sSkySmokeLayout |
| `lldrawpoolsky.cpp` 既存呼出 rename | `recordSkySmokeDraw` → `recordPlaceholderPoolDraw` 反映 (1 line + sub-doc 引用 comment) |

### 2.2 3.4-δ-2 (commit `37f5b65916`、11 file +33 / -0)

| 区分 | 内容 |
|---|---|
| 11 pool file (alpha / pbropaque / simple / water / bump / terrain / avatar / materials / tree / waterexclusion / wlsky) | 各 `recordPoolDraws(VkCommandBuffer cmd_buf)` hook body 内 既存 one-shot VkRecord marker 直後に `LLVKLoader::recordPlaceholderPoolDraw(cmd_buf)` 1 line + sub-doc 引用 comment 追加 |
| `#include "llvkloader.h"` 追加 | 各 file の `#include "lldrawpool*.h"` 直後配置 |
| 12 pool 全 hook 配線完了 | Sky 既配線 (δ-1) と合わせて pipeline.cpp:4928 dispatcher 経由 per-frame loop 内 12 回 `recordPlaceholderPoolDraw` 呼出 = PSO bind sSkySmokePipeline + set=0 PerFrame descriptor + set=1 PerMaterial descriptor + push constant 64 B identity modelview + vkCmdDraw(3,1,0,0) × 12 = 12 fullscreen triangle sky blue (0.4/0.6/0.9/1.0) 重ね描き、視覚 no-op 等価、Vulkan validation 0 件期待 |

### 2.3 3.4-δ-3 (commit `f88e9ed464`、`lldrawpoolterrain.cpp` -379 / +0、`lldrawpoolterrain.h` -3 / +0、net -382 行)

| 区分 | 内容 |
|---|---|
| `renderFull4TU` 削除 (182 行) | 4-TU multi-pass detail blending、段階 2 Core profile 化で fixed-function texgen path として既に死蔵 (internal dispatch 0 件 + 外部 caller 0 件、grep 2 段確認済) |
| `renderFull2TU` 削除 (166 行) | 2-TU 4-pass detail blending、同上 |
| `renderSimple` 削除 (33 行) | single-TU base terrain、同上 (pathing lib の `renderSimpleShapes` は別物) |
| header declaration 3 行削除 | `lldrawpoolterrain.h` 内 `void renderSimple()` / `void renderFull2TU()` / `void renderFull4TU()` declaration を物理削除 |
| INFO marker 1 件追加 | `recordPoolDraws` hook 内 既存 VkRecord one-shot marker 直後 / 同一 logged_once guard 内 `LL_INFOS("Vulkan") << "Terrain fixed-function texgen path physically removed (sub-step 3.4-d-3)"` 追加 |
| `glTexGen` 全消滅確認 | `grep -E "glTexGen" indra/newview/lldrawpoolterrain.cpp` 0 件 hit (cpp INFO marker 文字列内も "glTexGen" literal 排除 = "fixed-function texgen path" と表記、§3.1.4 acceptance 両件 satisfy) |

### 2.4 3.4-δ-4 (commit `05686ddd03`、`llvkloader.cpp` +364、`llvkloader.h` +12、`lldrawpoolavatar.cpp` +6/-4、計 +378 / -4)

| 区分 | 内容 |
|---|---|
| `VK_KHR_push_descriptor` extension 検出/enable | device extension 列挙時に `sDeviceLimits.pushDescriptorSupported` 検出 + 支援 device で `device_extensions` に `VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME` 追加 + 1 度のみ INFO marker `"Device extension VK_KHR_push_descriptor enabled (sub-step 3.4-d-4 avatar bone SSBO foundation)"` 発火、未支援 device は extension push スキップ + helper 内部 fallback |
| static state 6 件追加 | `AVATAR_BONE_MATRIX_COUNT = 110` (sub-doc 07 §1.2.2 想定値) + `sAvatarBoneDescriptorSetLayout` + `sAvatarBoneLayout` + `sAvatarBonePipeline` + `sAvatarBoneStorageBuffer` + `sAvatarBoneStorageAllocation` + `sAvatarBoneStorageMapped` |
| anon namespace helper × 3 | `createAvatarBoneDescriptorSetLayout` (set=2 binding 0 STORAGE_BUFFER VERTEX_BIT + `VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR` flag、未支援 device skip 成功 return) / `allocateAvatarBoneStorageBuffer` (`vmaCreateBuffer` USAGE_STORAGE_BUFFER 7040 B (110 mat4) + HOST_VISIBLE + HOST_ACCESS_SEQUENTIAL_WRITE + MAPPED、110 mat4 identity 初期化、未支援 device skip) / `createAvatarBonePipeline` (`set_layouts[3] = { PerFrame, PerMaterial, AvatarBone }` + push constant 64 B VERTEX_BIT、`sSkySmoke` shader 共用 [sub-doc 07 sub-step 7.4 で本格 avatar shader 配信時に差替]、未支援 device skip) |
| Case B 採用 (avatar 専用 layout + 専用 helper) | 11 pool 共用 placeholder draw helper (`recordPlaceholderPoolDraw` / `sSkySmokeLayout` 二段構え) **不変** + avatar pool だけ三段構え layout で別経路 = 段階 4 per-pool layout 化の template + 11 pool side regression risk ゼロ |
| `initVulkan` wiring | `createSkySmokePipeline` 完遂後で `createAvatarBoneDescriptorSetLayout` + `allocateAvatarBoneStorageBuffer` + `createAvatarBonePipeline` 連鎖、失敗時 shutdownVulkan 経由 graceful fail |
| `shutdownVulkan` teardown | `sSkySmokePipeline` destroy 前で pipeline → layout → SSBO (`vmaDestroyBuffer` + handle null) → descriptor set layout 順、null guard 完備 |
| `recordAvatarPlaceholderDraw` 公開 API | `llvkloader.h` +12 行 = 宣言 + sub-doc 引用付 comment、anon namespace 経由 namespace `LLVKLoader` 直下 free function |
| helper body (graceful fallback) | guard = `pushDescriptorSupported` false / `sAvatarBonePipeline` null / `sAvatarBoneLayout` null / `sAvatarBoneStorageBuffer` null / `vkCmdPushDescriptorSetKHR` null のいずれかで `recordPlaceholderPoolDraw` へ委譲 (機能 graceful degrade、validation 違反 0 件維持) → PSO bind `sAvatarBonePipeline` → set=0 PerFrame descriptor (`sPerFrameDescriptorSet[sFrameIndex]`) → set=1 PerMaterial descriptor (`bindPerMaterialDescriptorSet(cmd_buf, sAvatarBoneLayout)`) → set=2 binding 0 = `sAvatarBoneStorageBuffer` 全域 7040 B を `VkWriteDescriptorSet` (`descriptorType=STORAGE_BUFFER`) で `vkCmdPushDescriptorSetKHR` 投入 → push constant 64 B identity modelview / VERTEX_BIT → `vkCmdDraw(3, 1, 0, 0)` → 1 度のみ INFO marker `"Avatar placeholder pool draw fired (PSO bind sAvatarBonePipeline + set=0 PerFrame + set=1 PerMaterial + set=2 BoneStorage push descriptor (vkCmdPushDescriptorSetKHR, 110 mat4 identity) + push constant 64 B / VERTEX_BIT + vkCmdDraw(3,1,0,0))"` |
| `lldrawpoolavatar.cpp` call site swap | `recordPoolDraws` hook body 内 `LLVKLoader::recordPlaceholderPoolDraw` → `LLVKLoader::recordAvatarPlaceholderDraw`、3 行 sub-doc 引用 comment 追加、既存 one-shot VkRecord marker / `logged_once` guard はそのまま継承 |
| 12 pool 配線最終形 | 11 pool (sky / simple / bump / water / waterexclusion / alpha / tree / terrain / wlsky / pbropaque / materials) → `recordPlaceholderPoolDraw` / 1 pool (avatar) → `recordAvatarPlaceholderDraw`、`grep -E "recordPlaceholderPoolDraw\|recordAvatarPlaceholderDraw" indra/newview/lldrawpool*.cpp` = 12 hit |
| shader 改変ゼロ | Vulkan 仕様 layout-set ≥ shader-set 許容で `sAvatarBonePipeline` は 3-set layout だが `sSkySmoke` 共用 2-set shader を再利用、charter §2.1 領域 6 改変禁止遵守 |

### 2.5 3.4-ε (本 commit、doc-only)

| 区分 | 内容 |
|---|---|
| 本 handoff doc | `handoff-substep-3-4-complete.md` 新規起草 |
| sub-doc 03 §3.1.4 3.4-δ 行 | 「完遂 2026-05-31」marker + verify result 反映 |
| sub-doc 03 §3.1.4 3.4-ε 行 | 「完遂 2026-05-31」marker + verify result 反映 |
| sub-doc 03 §3.1 3.4 行 | 「完遂 2026-05-31」反映、3.5 着手 ready |
| memory `project_ayastorm_r41_vulkan_migration.md` | 3.4 全完遂 + 3.5 着手 ready 状態へ update |
| `MEMORY.md` index entry | 3.4 全完遂 + 段階 2 引継ぎ全 satisfy 反映 |

### 2.6 file 変更無し

- shader (`indra/newview/app_settings/shaders/`) / SPIR-V build chain: 3.4 全 sub-step touch せず (charter §2.1 領域 6 改変禁止、Vulkan 仕様 layout-set ≥ shader-set 許容で `sAvatarBonePipeline` 3-set layout でも sky smoke 共用 2-set shader 再利用可能)

---

## 3. AYA launch verify 詳細 (2026-05-31)

### 3.1 launch sequence

| 時刻 | event |
|---|---|
| 2026-05-30T22:20:23Z | Settings load (Vulkan 系 WARNING 無し) |
| 2026-05-30T22:20:23Z | `initVulkan` 着手 → loader v1.4.319 取得 → instance 作成 → physical device 列挙 (RTX 5090 selected) → device limit baseline 取得 (8 line、`maxPushDescriptors=32`含む) → `VK_EXT_memory_budget` 検出 → **`VK_KHR_push_descriptor` 検出/enable (δ-4 新規)** → device 作成 + dynamicRendering enable → command pool → offscreen image → render pass → framebuffer → VkPipelineCache → VMA allocator → Shared descriptor pool → placeholder image lifecycle smoke → PerFrame layout/UBO/sets → PerMaterial layout (γ) → Placeholder PSO compile → Sky smoke PSO compile + vert binding → **Avatar bone DSL + SSBO + PSO 連鎖 (δ-4 新規 3)** → PerMaterial set allocate + 7 binding update (γ transit smoke) → VMA budget smoke |
| 2026-05-30T22:20:23Z | `beginFrame` 進入 → vkCmdBindPipeline(sPlaceholderPipeline) → bindPerMaterialDescriptorSet (firstSet=1、γ wiring) → beginDynamicRendering helper → writeCurrentPerFrameMatrixUBO / writeCurrentTextureMatrixUBO 動作 |
| 2026-05-30T22:20:24Z | `LLRenderTarget::bindTarget` / `flush` dynamic rendering API surface 並走 (3.3-C-γ/δ 継続) → **12 pool hook fire 開始**: WaterExclusion (line 1012) → **Placeholder pool draw fired (δ-1 新規、line 1013)** → Simple (1014) → Bump (1015) → Materials (1016) → GLTFPBR (1017) → Alpha (1018) → ... |
| 2026-05-30T22:20:25Z | `syncMatrices modelview push constant path active` (3.3-A-δ-2 継続) |
| 2026-05-30T22:20:33Z | Terrain pool hook fire (line 1500) → **Terrain fixed-function texgen path physically removed (δ-3 新規、line 1501)** → Water pool (line 1502) |
| 2026-05-30T22:20:36Z | Sky pool / WLSky pool hook fire → Avatar pool hook fire (line 1639) → **Avatar placeholder pool draw fired (δ-4 新規、line 1640)** |
| 2026-05-30T22:20:47Z | Tree pool hook fire (line 2252、最後の 12th pool) |
| 2026-05-30T22:21:40Z | AYA 起動終了 → shutdownVulkan 完遂 (`Vulkan device destroyed` + `Vulkan instance destroyed` marker hit、validation 違反 0 件) |

### 3.2 unique INFO #Vulkan# marker 54 件 (γ baseline 47 + δ 新規 7)

**δ 新規 7 件**:
1. **(δ-4 新規)** `Device extension VK_KHR_push_descriptor enabled (sub-step 3.4-d-4 avatar bone SSBO foundation)` (`llvkloader.cpp:541`)
2. **(δ-4 新規)** `Avatar bone descriptor set layout created (set=2 binding 0 STORAGE_BUFFER VERTEX_BIT PUSH_DESCRIPTOR_KHR)` (`llvkloader.cpp:1931`)
3. **(δ-4 新規)** `Avatar bone storage buffer allocated (7040 B = 110 mat4 identity, HOST_VISIBLE + MAPPED)` (`llvkloader.cpp:1990`)
4. **(δ-4 新規)** `Avatar bone PSO compiled (set_layouts[3] = PerFrame + PerMaterial + AvatarBone, shader = sky smoke 流用、shader 改変ゼロ)` (`llvkloader.cpp:2105`)
5. **(δ-1 新規)** `Placeholder pool draw fired (PSO bind sSkySmokePipeline + set=0 PerFrame + set=1 PerMaterial + push constant 64 B identity / VERTEX_BIT + vkCmdDraw(3,1,0,0))` (`llvkloader.cpp:2741`)
6. **(δ-3 新規)** `Terrain fixed-function texgen path physically removed (sub-step 3.4-d-3)` (`lldrawpoolterrain.cpp:798`)
7. **(δ-4 新規)** `Avatar placeholder pool draw fired (PSO bind sAvatarBonePipeline + set=0 PerFrame + set=1 PerMaterial + set=2 BoneStorage push descriptor (vkCmdPushDescriptorSetKHR, 110 mat4 identity) + push constant 64 B / VERTEX_BIT + vkCmdDraw(3,1,0,0))` (`llvkloader.cpp:2833`)

**γ baseline 47 件 全継続** (regression 0、代表的なもの): `Initializing Vulkan loader...` / `Vulkan loader version 1.4.319` / `Vulkan instance created` / `Found 2 physical device(s)` + Candidate × 2 + Selected / `Graphics queue family: 0` / Device limit baseline × 8 line / `Device feature shaderClipDistance enabled` / `Vulkan device created` / `Vulkan 1.3 dynamicRendering feature enabled` / `Command pool` / `Offscreen image` / `Minimal render pass` / `Framebuffer` / `VkPipelineCache` / `VMA allocator created` / `Shared descriptor pool created` / `VkImage placeholder lifecycle smoke` / `Per-frame descriptor set layout created` / `Per-frame UBO buffers created` / `Per-frame descriptor sets allocated + updated` / `Per-material descriptor set layout created` / `Placeholder PSO compiled` / SPIR-V shader module loaded × 2 / `Sky smoke PSO compiled via SPIR-V build chain` / `Sky placeholder vert binding active` / `Per-material descriptor set transit smoke` / `VMA budget smoke` + heap 0/1 line / `LLImageGL::generateTextures Vulkan path mirror sampled` / `LLRenderTarget::bindTarget : bindTarget dynamic rendering begin path active` / `LLRenderTarget::flush : flush dynamic rendering end path active` / `syncMatrices PerFrame UBO write path active` / `syncMatrices TextureMatrix UBO write path active` / `beginDynamicRendering : dynamic rendering helper path active` / `syncMatrices modelview push constant path active` / `Vulkan device destroyed` / `Vulkan instance destroyed`

### 3.3 unique INFO #VkRecord# pool hook marker 12 件 (12 pool 全 hit)

| pool | line | one-shot marker |
|---|---|---|
| WaterExclusion | 1012 | `WaterExclusion pool recordPoolDraws hook fired (one-shot)` |
| Simple | 1014 | `Simple pool recordPoolDraws hook fired (one-shot)` |
| Bump | 1015 | `Bump pool recordPoolDraws hook fired (one-shot)` |
| Materials | 1016 | `Materials pool recordPoolDraws hook fired (one-shot)` |
| GLTFPBR (pbropaque) | 1017 | `GLTFPBR pool recordPoolDraws hook fired (one-shot)` |
| Alpha | 1018 | `Alpha pool recordPoolDraws hook fired (one-shot)` |
| Terrain | 1500 | `Terrain pool recordPoolDraws hook fired (one-shot)` |
| Water | 1502 | `Water pool recordPoolDraws hook fired (one-shot)` |
| Sky | 1580 | `Sky pool recordPoolDraws hook fired (one-shot)` |
| WLSky | 1581 | `WLSky pool recordPoolDraws hook fired (one-shot)` |
| Avatar | 1639 | `Avatar pool recordPoolDraws hook fired (one-shot)` |
| Tree | 2252 | `Tree pool recordPoolDraws hook fired (one-shot)` |

### 3.4 Vulkan WARN/ERR 0 件

`grep -E "WARNING.*Vulkan|ERROR.*Vulkan|VUID-" AYAstorm.log` 0 件 hit。出現する WARNING は `#Settings#` (fsdata_defaults.7.2.4.xml not found 等 = startup 既知) + `#` (font system) + `#Texture#` (downscaling 既知) のみ、Vulkan path 由来 0 件。

### 3.5 VMA budget heap 観測

```
heap 0 [DEVICE_LOCAL] size=32607 MB budget=30769 MB usage=64 MB (VMA allocations=2, blocks=2)
heap 1 [HOST]         size=48001 MB budget=48001 MB usage=35 MB (VMA allocations=0, blocks=1)
```

γ baseline (heap 0 = 1/1) から +1 allocation = δ-4 avatar bone storage buffer (7040 B = 110 mat4 identity、HOST_VISIBLE + HOST_ACCESS_SEQUENTIAL_WRITE + MAPPED) の分。期待通り。heap 1 [HOST] は staging 系が VMA 経由でなく `vkAllocateMemory` 直接のため allocations=0 維持 (β-1 baseline 継承)。

### 3.6 build clean

- `autobuild build -A 64 -c ReleaseFS_open --no-configure` exit 0
- `[100%] Built target llpackage`
- `Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261501942.tar.xz` 生成完了
- `grep -E "warning:|error:"` で `llvkloader.cpp` / `lldrawpool*.cpp` 由来 0 件

---

## 4. bridging item / spec cross-ref satisfy 状況

### 4.1 §1.5.3 bridging items 11 件 (sub-doc 01)

| # | item | satisfy 状況 |
|---|---|---|
| #1 | `LLGLState` setter dead-store 化 | (段階 4 LLPipelineFrameContext 配置時 satisfy 計画、3.4 touch 無し) |
| #2 | `VK_EXT_extended_dynamic_state2` dynamic state 配線 | 3.1 完遂時 satisfy 済 |
| **#3** | **`VK_KHR_push_descriptor` 経由 set=2 push descriptor 配線** | **✓ δ-4 で基本配線 satisfy** (avatar SSBO 限定)、本 push descriptor 全配線 (per-object UBO + per-draw texture binding ≤ 32) は領域 7 sub-step 7.4 残置 |
| #4 | `LLGLUserClipPlane` PSO 化 | 3.1 完遂時 satisfy 済 |
| #5 | shader 11 hit (sub-doc 04 §3) | (段階 4 領域 5 で satisfy 計画) |
| #6 | LL coordinate convention 注釈 | 3.1 完遂時 satisfy 済 (`projection_matrix` doxygen) |
| **#7** | **set=1 per-material 7 PBR slot layout** | **✓ γ で satisfy** |
| **#8** | **LLImageGL → VkImage + VkImageView lifecycle** | **✓ β-2 で satisfy** |
| #9 | swap chain present mode | (段階 4 領域 8 で satisfy 計画) |
| #10 | `VK_EXT_debug_utils` messenger 経由 GL ARB debug callback 置換 | 3.1 完遂時 satisfy 済 |
| #11 | `LLGLSyncFence` 物理削除 | 3.1 完遂時 satisfy 済 |

**3.4 で新規 satisfy = #3 (part) / #7 / #8 計 2.5 件**。本 sub-step 完遂で **段階 3 sub-step 3.4 scope 全 acceptance satisfy** (3.5 self-check に進む準備整う)。

### 4.2 段階 2 引継ぎ acceptance 全 satisfy

| acceptance | satisfy 状況 |
|---|---|
| **#1 段階 2** (drawpool 内 GL call 0 件) | **✓ 3.4-δ-1/δ-2 で satisfy** (`grep -rE "gl[A-Z][a-zA-Z]+\s*\(" indra/newview/lldrawpool*.cpp` 0 件 hit、12 pool 全 hook body PSO bind + descriptor bind + push constant + draw 配線完了) |
| **特殊対応 terrain glTexGen 廃止** | **✓ 3.4-δ-3 で satisfy** (Option D dead-code 撤去、3 関数本体丸ごと削除、net -382 行、`grep -E "glTexGen" indra/newview/lldrawpoolterrain.cpp` 0 件) |
| **特殊対応 avatar bone matrix → VkBuffer** | **✓ 3.4-δ-4 で satisfy** (Case B avatar 専用 layout + 専用 helper + `vkCmdPushDescriptorSetKHR` 経由 set=2 binding 0 STORAGE_BUFFER、110 mat4 identity HOST_VISIBLE+MAPPED) |

### 4.3 sub-doc 07 §3.1 領域 7 sub-step 内包状況

| sub-step | 内包状況 |
|---|---|
| 7.1 (VMA + descriptor pool sizing 基盤) | ✓ β-1 で内包先行 install 完遂 |
| 7.2 (set=0 per-frame 全配線 ~30 sampler 集約) | (3.3-A で PerFrameMatrixUBO + TextureMatrixUBO 配線済、shadow/env cubemap/LUT + AYAstorm picker output buffer は領域 7 残置) |
| 7.3 (set=1 per-material layout) | ✓ γ で layout 部分内包、material params UBO + per-material sampler + ~50 material cache (×3 frame in flight = 150 pool sizing) 本実装は 7.3 残置 |
| **7.4 (set=2 push descriptor)** | **✓ δ-4 で push descriptor 基本配線部分内包** (`VK_KHR_push_descriptor` extension enable + avatar SSBO 限定 `vkCmdPushDescriptorSetKHR` 経由 set=2 binding 0 STORAGE_BUFFER)、本配線 (per-object UBO + per-draw texture binding ≤ 32) は 7.4 残置 |
| 7.5 (7 pass chain + dynamic rendering 実 attachment) | (3.3-C で API surface 並走済、実 attachment は 7.5 残置 = LLImageGL → VkImage の object lifecycle のみ β-2 で satisfy) |

---

## 5. 3.5 着手境界 (本 handoff 役割引継)

### 5.1 3.5 scope (sub-doc 03 §3.1)

- **段階 3 self-check + validation strict 検証**: §4.1 acceptance 5 件 self-trace PASS (charter §3 #1/#3/#5/#6 段階 3 分 + regression) + `VK_LAYER_KHRONOS_validation` force-enable build で validation 0 件再確認 + 起動 + login 後 sustained ~10 分動作 + 各 pool の PSO bind 通過確認 (LL_INFOS log + per-pool PSO compile counter)
- **handoff doc**: `handoff-stage-3-complete.md` 作成 (段階 3 sub-step 3.1a/3.1b/3.2/3.3-A/3.3-B/3.3-C/3.4 全完遂総括 → 段階 4 着手境界 + 段階 4 受入根拠 整理 + 段階 5+ 並走可能性確認)
- **触らない境界**:
  - 段階 4 LLPipelineFrameContext 配置 (item #1 LLGLState setter dead-store 化、pipeline.cpp 3 大グローバル → frame context 化、LLVKRenderer 配置) は段階 4 移管
  - 領域 6 sub-step 6.1 (autobuild integration 一括化 248 file 全 port) は並走着手可能だが、本 sub-step 3.5 scope 外
  - 領域 7 sub-step 7.2/7.3/7.4/7.5 全配線は領域 7 本来 scope 維持

### 5.2 想定 file / LOC

- (主に doc-only) `docs/specs/ayastorm-r41-gl-removal/handoff-stage-3-complete.md` 新規起草 + sub-doc 03 §4.1 / §3.1 末尾 「sub-step 3.5 完遂」反映 + memory `project_ayastorm_r41_vulkan_migration.md` を段階 4 着手 ready 状態へ update
- (validation 確認) `VK_LAYER_KHRONOS_validation` force-enable build (`#ifndef LL_RELEASE_FOR_DOWNLOAD` 復元) で起動 + 動作中 error / warning 0 件確認

### 5.3 acceptance (sub-doc 03 §3.1 3.5 行)

- §4.1 acceptance 5 件 self-trace PASS (charter §3 #1 GL 依存除去 / #3 PSO bind 動作 + validation 0 件 / #5 descriptor set 3 階層 + push descriptor 動作 [段階 3 分] / #6 LLVKRenderer skeleton [段階 4 持越し記録] + regression)
- validation strict force-enable build で起動 + 動作中 error / warning 0 件再確認 (sustained ~10 分動作)
- handoff doc `handoff-stage-3-complete.md` 完成 + commit 投入

---

## 6. resume entry point (3.5 着手用)

### 6.1 必読 (resume 開幕順)

1. 本 handoff (`handoff-substep-3-4-complete.md`、3.4 全完遂後 entry point)
2. sub-doc 03 §3.1 sub-step 3.5 行 + §4.1 acceptance 5 件
3. sub-doc 03 §3.5 「GL path 動作維持」 / §3.3 「UI matrix 段階 4 持越し」 / §1.5.3 bridging item 一覧
4. memory `project_ayastorm_r41_vulkan_migration.md` (sub-step 3.5 着手 ready 状態確認)
5. `feedback_self_verify_before_handoff.md` (段階 3 完遂境界 self-trace 必須)
6. `feedback_remove_verification_logs.md` (3.5 で `#ifndef LL_RELEASE_FOR_DOWNLOAD` 復元) — ただし本 r41 段階では INFO marker は acceptance log として残置 (段階 4 / 領域 7 完遂時に再検討)

### 6.2 case A/B 進行粒度 (resume 開幕 AYA 確認 1 件)

3.5 は scope が **doc + validation strict 確認** 中心で 3.3/3.4 より狭い。AYA に粒度確認 (案 A = 1 commit bundle、案 B = 3.5-a [validation strict 確認] + 3.5-b [handoff doc 起草] の 2 commit 分割)。

### 6.3 領域 6 sub-step 6.1 並走着手判断

3.3-B で確立した SPIR-V build chain pattern (system glslangValidator + `aya_compile_shader_spirv` cmake function + viewer_manifest.py recursive copy + `loadSpirvShaderModuleFromFile` helper + macro guard fallback) を 248 file 全 shader port に一般化する sub-step 6.1 は、3.4 完遂で前提条件 (PSO + descriptor set + push constant 配線) 揃ったため並走着手可能。Agent 並列 cluster 候補 (`feedback_use_agents_proactively.md` 遵守)。3.5 着手境界判断時に AYA 並走 GO/NO-GO 確認。

---

## 7. proactive handoff チェック

- `feedback_proactive_handoff.md` 遵守: 3.4 全完遂境界で本 handoff doc 起草 (AYA 指示待たず能動投入、β-1 → β-2 / β-2 → γ / γ → δ transition と同 pattern 継承)
- 3.5 着手直後の session pause 可能性 (validation strict 確認 + handoff doc 起草で context 残量逼迫予想低): 必要なら 3.5 着手直前で再度能動 session pause handoff 起草を検討
- segments 段階 4 + 領域 6 sub-step 6.1 並走判断は 3.5 完遂境界の `handoff-stage-3-complete.md` で詳細整理予定
