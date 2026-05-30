# r41 段階 3 完遂 → 段階 4 着手境界 handoff (2026-05-31)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-4-complete.md` (sub-step 3.4 全 sub-step α/β-1/β-2/γ/δ-1/δ-2/δ-3/δ-4/ε 完遂 → 3.5 着手境界)
**本 handoff の位置付け**: 段階 3 (llrender 主要 5 file の state machine → Vulkan PSO 化、charter §2 領域 3 1.50 PM 高 risk) 全 sub-step 完遂境界。次は段階 4 (pipeline.cpp 3 大グローバル → LLPipelineFrameContext 集約 + LLGLState RAII setter dead-store 化、charter §2 領域 4 1.00 PM 高 risk) 着手前 sub-doc `04-frame-context.md` (仮称) 起草 + scope 確認。

---

## 1. 段階 3 完遂 state (2026-05-31)

### 1.1 sub-step 3.5 完遂宣言

- 達成内容:
  - **3.5-a validation strict force-enable build verify** (no-commit、4 guard force-enable patch → 部分 rebuild → AYA launch + sustained ~10 分動作 → log 解析 → patch revert clean):
    - `indra/llrender/llvkloader.cpp` line 134/143/185/215 の `#ifndef LL_RELEASE_FOR_DOWNLOAD` / `#ifdef LL_RELEASE_FOR_DOWNLOAD` guard を局所 unguard (`#if 1` / `#if 0` 化)
    - VK_LAYER_KHRONOS_validation + VK_EXT_debug_utils を ReleaseFS_open ビルドに強制有効化
    - autobuild ReleaseFS_open `--no-configure` 部分 build → install → cache clear → AYA launch
    - sustained ~10 分動作完遂、SL login + 通常動作 + clean quit
    - `git checkout -- indra/llrender/llvkloader.cpp` で patch revert、working tree clean (`feedback_remove_verification_logs.md` 遵守)
  - **3.5-b handoff doc 起草 + sub-doc/memory update** (本 commit):
    - 本 doc `handoff-stage-3-complete.md` 新規作成
    - sub-doc 03 §3.1 sub-step 3.5 行 complete 化 + §4.1 acceptance 行末尾 satisfy 状態反映
    - memory `project_ayastorm_r41_vulkan_migration.md` 段階 4 着手 ready state 更新
    - MEMORY.md index entry 更新
- 達成 marker (検証 log evidence、本 doc §3 参照):
  - L86: `Vulkan instance created (validation=enabled)` (llvkloader.cpp:180、validation layer 正常 load)
  - L87: `VK_EXT_debug_utils messenger installed` (llvkloader.cpp:243、debug callback installed)
  - vulkanDebugCallback 経由 `[VK ERROR]` / `[VK WARN]` / `[VK INFO]` / `[VK VERBOSE]` 真の **0 件** (sustained 全動作中)
  - 12 #VkRecord# pool record hook **12/12 one-shot fire** (Sky / WLSky / WaterExclusion / Simple / Bump / Materials / GLTFPBR / Alpha / Terrain / Water / Avatar / Tree、regression 0、Tree も login 後動作で fire 確認)
  - 57 unique #Vulkan# INFO marker (3.4 baseline 54 + 3.5-a 追加 3: debug_utils messenger installed + validation=enabled text 差 + VMA heap 数値 variation)
  - shutdown clean (Vulkan device destroyed → Vulkan instance destroyed 順、validation 違反 0 件)
- commit: 1 件 (本 3.5-b doc commit、3.5-a は no-code-commit = verify 用 patch を revert 済)

### 1.2 段階 3 全 sub-step 完遂 evidence table

| sub-step | 内容 | 主要 commit | 完遂日 | state |
|---|---|---|---|---|
| 3.1a | sub-doc 03 起草 + §1.5 trace inventory (i/ii/iii/iv 4 段階、Agent 並列 cluster trace) | `25228ec8ae` ほか | 2026-05-29 | ✓ |
| 3.1b | PSO 基盤 (VkPipelineCache + VkPipelineLayout 標準形 + placeholder PSO compile) + bridging items #2/#4/#10/#11 物理実装 | `8e8a846c14` | 2026-05-29 | ✓ |
| 3.2 | sky pool 1 draw smoke-test (PSO bind + `vkCmdDraw` 投入、SPIR-V 配線) | `e9948a0de8` | 2026-05-29 | ✓ |
| 3.3-A | matrix stack 二段構え化 (push constant 64 B modelview + per-frame UBO 2 binding [projection 系 + texture matrix])、α/β-1/β-2/γ/δ-1/δ-2/ε | `62778dcac4` (β-1) / `57d72d6976` (β-2) / `eb6c6f0ac5` (γ) / `00fcff6eab` (δ-1) / `f035815f55` (δ-2) / `0d6f07ac67` (ε) | 2026-05-29 | ✓ |
| 3.3-B | shader port (sky placeholder vert+frag SPIR-V exemplar、SPIR-V build chain 確立、領域 6 sub-step 6.1 pre-flight)、α/β-1/β-2/γ/δ/ε | `0bbfe8319f` (α) / `4ec234fa23` (β-1) / `5d4999ec4a` (β-2) / `020df91561` (γ) / `b2c06f869d` (δ) / `2def67b0ef` (ε) / `d7d4e5bef2` (B 全完遂 handoff) | 2026-05-31 | ✓ |
| 3.3-C | FBO → dynamic rendering API surface 並走化 (LLRenderTarget bindTarget/flush → vkCmdBeginRendering/vkCmdEndRendering、placeholder attachment)、α/β-1/β-2/γ/δ/ε | `b798d27265` (α) / `3f8f1e2a1a` (β-1) / `85b3a9a513` (β-2) / `8ed9edd23b` (γ) / `da01900f46` (δ) / `429f67dbf3` (ε) | 2026-05-29 | ✓ |
| 3.4 | texture lifecycle + descriptor set=1 配置 + 12 pool hook body PSO bind + 段階 2 引継ぎ特殊対応 2 件 (terrain glTexGen 物理削除 + avatar SSBO 基本配線)、α/β-1/β-2/γ/δ-1/δ-2/δ-3/δ-4/ε | `a8f8472202` (α) / `4bdadf4eb9` (β-1) / `b84ad42904` (β-2) / `8f0a8eaa82` (γ) / `4e6e75257f` (δ-1) / `37f5b65916` (δ-2) / `f88e9ed464` (δ-3) / `05686ddd03` (δ-4) / `8ca3db473c` (δ-5/ε 内包) | 2026-05-31 | ✓ |
| **3.5** | 段階 3 self-check + validation strict 検証 + handoff doc 作成 (3.5-a verify + 3.5-b doc、案 B 2 commit 分割) | `8ca3db473c` (前 commit 内包 = 3.4 全完遂 / 3.5 着手境界) + 本 commit (3.5-b) | **2026-05-31** | **✓** |

### 1.3 段階 3 acceptance 5 件 self-trace (03-state-machine-pso.md §4.1)

| acceptance criterion | 段階 3 sub-step での state | 判定 |
|---|---|---|
| **#1-段階 3 (llrender 主要 5 file + lldrawpool 13 file の GL call 除去、RAII setter 内呼出は除外)** | llrender 5 file (`llgl.{cpp,h}` / `llrender.{cpp,h}` / `llimagegl.{cpp,h}` / `llrendertarget.{cpp,h}` / `llpostprocess.{cpp,h}`) のうち `llpostprocess.{cpp,h}` は upstream Firestorm 由来 `apply()` 呼出 0 件 + effect 関数 empty body の dead code として r42-δ basket 移管、残 4 file は段階 1+2+3 全 sub-step で render path 動作維持に必須な GL call が依然存在 (PSO state alias 基盤 + PerFrame UBO + dynamic rendering helper による Vulkan path **並走**、GL path 動作維持 = sub-doc 03 §3.5 起源) → **RAII setter 内 GL call 除去 (bridging item #1) は段階 4 LLPipelineFrameContext 配置時 dead-store 化に scope refine 2026-05-29** (§4.1 metric 側で本除外を明示)、lldrawpool 13 file は 3.4-δ-2 で 11 pool + δ-1 で sky pool + δ-4 で avatar pool の全 12 pool hook 内 PSO bind + vkCmdDraw 配線完遂 (3.4-δ-3 で terrain.cpp 内 `glTexGen` 物理削除済)。**PFNGL function pointer declarations 物理削除は 189 file 波及 (§1.5.2 Cluster D) で段階 5 一体運用** | **本段階 metric 範囲内 satisfy** ✓ (RAII setter 内呼出は段階 4 / PFNGL declarations 物理削除は段階 5 へ scope refine、charter §7.5 boundary refine 範囲内) |
| **#3-段階 3 (PSO bind 動作 + validation 0 件)** | 12 pool 全 hook で PSO bind + vkCmdDraw* 動作 (sub-step 3.4-δ-1/δ-2/δ-4 配線、3.4 launch verify 確認済 12/12 pool one-shot fire、δ-5 verify でも 12/12 維持)、sub-step 3.5-a validation strict force-enable build (VK_LAYER_KHRONOS_validation + VK_EXT_debug_utils 強制有効化) で起動 + sustained ~10 分動作中 vulkanDebugCallback 経由 `[VK ERROR]` / `[VK WARN]` 真の **0 件** | **達成** ✓ |
| **#5-段階 3 (descriptor set + render pass 実装、領域 7 並走 satisfy)** | 3 階層 descriptor set + dynamic rendering 採用動作: set=0 PerFrame UBO 2 binding (3.3-β-2 / γ / δ-1 / δ-2)、set=1 PerMaterial 7 PBR slot COMBINED_IMAGE_SAMPLER (3.4-γ で binding 0-6 配置 + immutable null + placeholder sampler、material cache 本実装は領域 7 sub-step 7.3 残置)、set=2 AvatarBone STORAGE_BUFFER (3.4-δ-4 で `VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR` + `vkCmdPushDescriptorSetKHR` 基本配線、本 push descriptor 全配線は領域 7 sub-step 7.4 残置)、dynamic rendering 採用 (3.3-C-β-2 で `VkPhysicalDeviceDynamicRenderingFeatures` enable + helper body + transit smoke、3.3-C-γ/δ で `LLRenderTarget::bindTarget`/`flush` Vulkan path 並走、実 attachment は領域 7 sub-step 7.5 残置)、validation strict build で descriptor binding mismatch / render pass dependency violation **0 件** | **達成** ✓ |
| **#6-段階 3 (LLVKRenderer skeleton signature 整合)** | charter §3 #6 要件 = pipeline.cpp 内 inline 実装と LLVKRenderer skeleton hook の signature 整合は段階 3 完遂時に **領域 8 (LLVKRenderer skeleton) と並走 satisfy** が前提。段階 3 では PSO bind / 12 pool record hook / descriptor 配信 関数 (`recordPlaceholderPoolDraw` / `recordAvatarPlaceholderDraw` / `bindPerMaterialDescriptorSet` / `beginDynamicRendering` / `endDynamicRendering` 等) の signature 確定済、skeleton declaration 物理配置は領域 8 sub-doc (`08-llvkrenderer-skeleton.md` 仮称) で段階 4 並走起草。本段階 3 metric は **signature 確定 + 領域 8 並走着手 ready** を satisfy 判定 | **本段階 metric 範囲内 satisfy** ✓ (skeleton declaration 物理配置は領域 8 で段階 4 並走、charter §7.5 boundary refine 範囲内) |
| **特殊対応 (terrain glTexGen 廃止 + avatar skinning SSBO)** | 3.4-δ-3 で `lldrawpoolterrain.cpp` 内 `renderFull4TU` / `renderFull2TU` / `renderSimple` 3 関数 (計 381 行) を死蔵 dead-code として物理削除 → `grep -E "glTexGen" indra/newview/lldrawpoolterrain.cpp` **0 件 hit** + INFO marker `Terrain fixed-function texgen path physically removed` 1 件出力、3.4-δ-4 で `lldrawpoolavatar.cpp` recordPoolDraws hook body を `recordAvatarPlaceholderDraw` に swap → set=2 binding 0 STORAGE_BUFFER で bone matrix → VkBuffer (7040 B = 110 mat4 identity、HOST_VISIBLE_COHERENT + MAPPED) 配線 + `vkCmdPushDescriptorSetKHR` 投入動作 (Avatar bone PSO compile + storage buffer alloc + Avatar placeholder pool draw fired marker 全 hit)、validation strict build で binding mismatch 0 件 | **達成** ✓ |
| **regression (段階 1-2 動作維持)** | Vulkan instance + device + command pool + render pass + 12 pool record hook 動作維持 (3.5-a launch verify で 57 unique #Vulkan# marker + 12 #VkRecord# pool hook 全 hit、shutdown clean、AYAstorm 機能 [audio / chat / login / inventory] regression 0 件、validation 違反 0 件)、`feedback_release_with_user_feedback.md` 遵守 exhaustive solo session 不要 + AYA launch ~10 分動作 PASS | **達成** ✓ |

### 1.4 段階 3 完遂宣言の解釈

- 段階 3 全 sub-step (3.1a〜3.5) は **完遂** (acceptance #3 / #5 / 特殊対応 / regression 全達成、acceptance #1 / #6 は scope refine 範囲内 satisfy)
- acceptance #1 (GL call 除去) の RAII setter 内呼出 (bridging item #1) は **段階 4 LLPipelineFrameContext 配置時 dead-store 化** に scope refine 2026-05-29 (charter §2 領域 4 境界条件、sub-doc 03 §4.1 #1-段階 3 metric 内で明示)
- acceptance #1 の PFNGL function pointer declarations 物理削除 (§1.5.2 Cluster D 189 file 波及) は **段階 5 (依存解決) と一体運用**
- acceptance #6 (LLVKRenderer skeleton signature 整合) の skeleton declaration 物理配置は **領域 8 sub-doc で段階 4 並走起草**、signature 確定 + 並走着手 ready で本段階 satisfy
- charter §3 全体 acceptance #1 (GL 除去) は 段階 4 + 段階 5 完遂時に最終 satisfy 見込
- **段階 4 着手は GO** (PSO 基盤 + 3 階層 descriptor set + dynamic rendering 並走基盤 + 12 pool record hook 動作維持 = LLPipelineFrameContext refactor の前提整備済)

### 1.5 status close / next active

| doc / memory | status |
|---|---|
| `handoff-substep-3-4-complete.md` | **役割完了 2026-05-31** (3.5 着手 satisfy) |
| `handoff-stage-3-complete.md` (本 handoff) | 新規作成 (段階 3 完遂 → 段階 4 着手境界) |
| sub-doc `03-state-machine-pso.md` | **役割完了 2026-05-31** (段階 3 完遂で参照役割完了、acceptance #1 RAII setter 内呼出の段階 4 一体運用 + acceptance #6 skeleton declaration の領域 8 並走 + PFNGL declarations 物理削除の段階 5 一体運用も charter §7.5 範囲内で本 handoff §1.3 + §1.4 に記録) |
| sub-doc `06-shader-spirv.md` | active 継続 (sub-step 6.1 = autobuild integration の本格着手判断は段階 4 並走で AYA 確認) |
| sub-doc `07-descriptor-renderpass.md` | active 継続 (sub-step 7.3 material cache 本実装 / 7.4 push descriptor 全配線 / 7.5 実 attachment 配線は段階 4-5 並走 sub-step) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (段階 4 着手 ready 状態に更新) |

---

## 2. 段階 4 着手前の scope 確認 (次 session 最優先)

### 2.1 段階 4 charter scope (charter §2 領域 4 + §6 並走方針)

- **対象**: pipeline.cpp 3 大グローバル (`sCull` / `sShadowRender` / `sCurCameraID` 等) → LLPipelineFrameContext 集約 + LLGLState RAII setter dead-store 化 (12 件 RAII state class)
- **risk**: **高** (charter §2 領域 4 高 risk 領域、1.00 PM、charter §6.4 「描画再構築 phase」)
- **依存順序**: 段階 3 完遂後着手 (本 handoff で boundary 確定)、領域 8 (LLVKRenderer skeleton) と並走可
- **段階 3 からの引継ぎ**:
  - bridging item #1 (LLGLState setter 内 GL call dead-store 化) を段階 4 内で satisfy (charter §2 領域 4 境界条件)
  - PSO state alias 基盤 + LLPipelineFrameContext で render path context 確定後に caller source-level compat 維持しつつ setter 内 GL call 物理削除
  - 12 pool record hook body の draw 配線 (placeholder PSO + avatar PSO) を LLPipelineFrameContext 経由で frame state 集約 base 化

### 2.2 sub-doc 04 起草が次 deliverable

- doc path: `docs/specs/ayastorm-r41-gl-removal/04-frame-context.md` (仮称、charter §7.4 sub-doc 構成、Pattern α 一括 outline draft → AYA review boundary)
- 段階 4 内 sub-step 分割は段階 3 の 5 sub-step 範式継承想定 (基盤 + 軽量 + 標準 + 特殊 + self-check)、charter §7.5 で boundary refine 可
- 起草 deliverable:
  - 段階 4 scope plan (charter §2 領域 4 + 段階 3 引継ぎ反映)
  - pipeline.cpp 3 大グローバル + 周辺 frame state inventory (Agent 並列 trace 推奨、`feedback_use_agents_proactively.md`)
  - LLPipelineFrameContext struct 設計 + caller migration 順序
  - 12 件 LLGLState RAII state class setter 内 GL call dead-store 化計画
  - sub-step list (4.1 〜 4.5 等、段階 3 範式継承)
  - 段階 4 completion criteria
  - 段階 3 acceptance #1 (RAII setter dead-store 化) + acceptance #6 (LLVKRenderer skeleton declaration) の段階 4 内 satisfy 計画 (本 handoff §1.3 引継ぎ記録)

### 2.3 段階 4 着手前の AYA 擦り合わせ事項

- 段階 4 sub-doc 04 の draft pattern (α 一括 vs β 細分化) 選択
- 段階 3 acceptance #1 RAII setter 内呼出 + acceptance #6 skeleton declaration の段階 4 一体運用 (本 handoff §1.4 解釈) の AYA 承認
- 領域 8 (LLVKRenderer skeleton) sub-doc 並走起草着手判断 (段階 4 完遂時 acceptance #6 satisfy 必須)
- 領域 6 sub-step 6.1 (autobuild integration) 本格着手判断 (3.3-B で pre-flight 確立済、段階 4 並走で本格化可)

### 2.4 次 session 最初のアクション

1. **本 handoff 確認** (`handoff-stage-3-complete.md`) を Read
2. **段階 3 acceptance #1 RAII setter / #6 skeleton declaration の段階 4 一体運用** を §1.3-1.4 で AYA 承認 (質問 1 件)
3. **sub-doc 04 起草着手判断**:
   - Pattern α 一括 outline draft → AYA review boundary (charter §7.4 標準範式)
   - 起草対象: §1 段階 4 scope plan、§2 pipeline.cpp frame state inventory + dependency graph、§3 sub-step 分割、§4 completion criteria、§5 関連 doc / memory
4. **領域 8 並走起草判断 + 領域 6 sub-step 6.1 本格着手判断** (sub-doc 04 と同タイミングで領域 8 sub-doc 起草 / 領域 6 sub-step 6.1 本格着手するか、段階 4 着手後の追加判断とするか)

---

## 3. 検証 evidence (本 sub-step 3.5-a で取得した log 抜粋)

### 3.1 validation strict build 起動 log (cache clear 後の clean run、2026-05-31)

```
L86: 2026-05-30T23:04:26Z INFO #Vulkan# llrender/llvkloader.cpp(180) createInstance : Vulkan instance created (validation=enabled)
L87: 2026-05-30T23:04:26Z INFO #Vulkan# llrender/llvkloader.cpp(243) createDebugMessenger : VK_EXT_debug_utils messenger installed
```

(その他 55 件の unique #Vulkan# INFO marker = device limit / VMA / PSO compile / descriptor pool / placeholder image / Per-frame UBO / Per-material descriptor / Avatar bone PSO / dynamic rendering / sync matrices 等が全 hit、3.4 baseline 54 + 3.5-a 追加 3 = 57 件)

### 3.2 12 pool record hook fire log (12/12 one-shot marker fire)

```
#VkRecord# Sky pool recordPoolDraws hook fired (one-shot)
#VkRecord# WLSky pool recordPoolDraws hook fired (one-shot)
#VkRecord# WaterExclusion pool recordPoolDraws hook fired (one-shot)
#VkRecord# Simple pool recordPoolDraws hook fired (one-shot)
#VkRecord# Bump pool recordPoolDraws hook fired (one-shot)
#VkRecord# Materials pool recordPoolDraws hook fired (one-shot)
#VkRecord# GLTFPBR pool recordPoolDraws hook fired (one-shot)
#VkRecord# Alpha pool recordPoolDraws hook fired (one-shot)
#VkRecord# Terrain pool recordPoolDraws hook fired (one-shot)
#VkRecord# Water pool recordPoolDraws hook fired (one-shot)
#VkRecord# Avatar pool recordPoolDraws hook fired (one-shot)
#VkRecord# Tree pool recordPoolDraws hook fired (one-shot)
```

(段階 2 完遂時の 11/12 (Tree login 前 inactive) から段階 3 完遂時は 12/12、Tree pool も login 後 scene-dependent fire 確認済)

### 3.3 shutdown clean log

```
2026-05-30T23:05:38Z INFO #Vulkan# llrender/llvkloader.cpp(2416) shutdownVulkan : Vulkan device destroyed
2026-05-30T23:05:38Z INFO #Vulkan# llrender/llvkloader.cpp(2429) shutdownVulkan : Vulkan instance destroyed
```

### 3.4 validation 違反 grep 結果

- `grep -cE "\[VK (ERROR|WARN|INFO|VERBOSE)\]" ~/.ayastorm_x64/logs/AYAstorm.log` = **0 件** (vulkanDebugCallback 経由 message ゼロ = validation strict 起動 + sustained ~10 分動作中 layer 違反 0 件)
- 非 Vulkan 系の pre-existing WARN (fontgl `Could not determine system fonts path` / llresmgr `Failed to set locale en_US.utf8` / SECAPI `CertExpired` / chrome-sandbox suid bit / OCRA font template 等) は段階 1-2 baseline 同一、Vulkan path 無関係

### 3.5 patch revert clean 確認

```
$ git status indra/llrender/llvkloader.cpp
ブランチ feature/ayastorm-r41-gl-removal
nothing to commit, working tree clean
```

(4 guard force-enable patch を `git checkout --` で revert、commit 不要 = `feedback_remove_verification_logs.md` 遵守)

---

## 4. deferred item 持ち越し

### 4.1 Mesa RADV 動作確認 (`handoff-stage-1-complete.md` §3.1.2 + `handoff-stage-2-complete.md` §4.1 継承)

- AYA 環境に AMD discrete GPU 不在 → Mesa RADV 動作未確認
- 段階 4-9 進行中は据置、段階 10 driver matrix polish で改めて testbed 確保

### 4.2 macOS / Windows 検証

- 段階 1〜3 全て Linux 検証のみ、`project_ayastorm_three_platforms.md` 「Linux 先行例外」 (`feedback_bd_port_autonomous_exec.md` 期間継承の autonomous build 全権内)
- charter §6 並走方針で macOS / Windows は段階 9 統合 verify でまとめて検証

### 4.3 領域 6 / 7 並走 sub-step 残スコープ

- **領域 6** (248 GLSL shader SPIR-V 化、4.96 PM 最大領域):
  - sub-step 6.1 (autobuild integration) は 3.3-B で pre-flight 確立済 (system glslangValidator + `aya_compile_shader_spirv` cmake function + `viewer_manifest.py` recursive copy + `loadSpirvShaderModuleFromFile` helper + macro guard fallback)
  - 段階 4 並走で本格着手判断、sub-step 6.1 完遂後 6.2 (shader 248 file ~228 file SPIR-V port) 着手
- **領域 7** (descriptor set + render pass、1.50 PM):
  - sub-step 7.1 (VMA + descriptor pool sizing) → 3.4-β-1 で内包先行 install 完遂
  - sub-step 7.3 (set=1 per-material 7 PBR slot) → 3.4-γ で layout + 1 set transit smoke 完遂、material params UBO + per-material sampler + ~50 material cache 本実装は残置
  - sub-step 7.4 (set=2 push descriptor) → 3.4-δ-4 で avatar bone SSBO 基本配線完遂、push descriptor 全配線は残置
  - sub-step 7.5 (実 attachment 配線) → 3.3-C-γ/δ で API surface 並走完遂、`VkImage` / `VkImageView` 実体作成 + attachment 提供は残置
  - 段階 4-5 並走で 7.3/7.4/7.5 本格着手

### 4.4 領域 8 (LLVKRenderer skeleton) 着手判断

- charter §2 領域 8 (0.50 PM)、charter §3 acceptance #6 (skeleton signature 整合)
- 本段階 3 完遂時 signature 確定済 (本 handoff §1.3 #6-段階 3 参照)
- skeleton declaration 物理配置は領域 8 sub-doc (`08-llvkrenderer-skeleton.md` 仮称) で段階 4 並走起草

---

## 5. 次 session 開始 action cadence

### 5.1 次 session 開始時の最初のアクション

1. **handoff 確認**: 本 handoff (`handoff-stage-3-complete.md`) を Read
2. **段階 3 acceptance #1 RAII setter / #6 skeleton declaration の段階 4 一体運用解釈** (本 handoff §1.3-1.4) の AYA 承認 (質問 1 件)
3. **sub-doc 04 起草着手**: Pattern α 一括 outline draft → AYA review (charter §7.4 標準範式)
4. **領域 8 並走起草 + 領域 6 sub-step 6.1 本格着手判断**: sub-doc 04 と同タイミング起草するか、段階 4 着手後判断とするか (質問 1 件)

### 5.2 段階 4 着手中の注意事項 (`handoff-stage-2-complete.md` §5.2 継承)

- **Linux first-class baseline 厳守**: NVIDIA proprietary on RTX 5090 で動作確認、Mesa RADV は段階 10 polish
- **parity 不要**: charter §1 thesis 維持、AYAstorm 改変 13 file shader は r42-α/β/γ で port
- **acceptance satisfy は実機検証**: `feedback_build_only_verified.md` 遵守、推論 ban
- **仮説 2 連続外れ rule**: `feedback_admit_unknown.md` 反映
- **sub-step 完遂時 self-trace + handoff**: `feedback_self_verify_before_handoff.md` + `feedback_proactive_handoff.md` 継承
- **commit / push cadence**: commit は AYA 指示後、push は AYA 手動 (`feedback_release_flow.md` 遵守)
- **検証用 force-enable は出荷物に残さない**: 本 sub-step 3.5-a で 4 guard 復元実施、段階 4 以降も同様 (`feedback_remove_verification_logs.md`)

### 5.3 段階 4 着手で新規発生する注意事項

- **pipeline.cpp 巨大 file への影響範囲管理**: pipeline.cpp は viewer 内最大 file 群の一つ、3 大グローバル refactor は caller 全件追跡必須 (Agent 並列 trace 推奨)
- **LLPipelineFrameContext の lifecycle 設計**: frame 開始 → render pass 内 read-only 共有 → frame 終了で teardown、12 pool record hook 内からの参照 cadence
- **LLGLState RAII setter dead-store 化の caller compat**: 12 件 RAII state class (LLGLState / LLGLDepthTest / LLGLSDefault 等) は viewer 全体 850+ caller (推定)、setter 内 GL call 物理削除でも caller source-level compat 維持必須
- **領域 8 LLVKRenderer skeleton 並走起草**: 本段階 4 完遂時に acceptance #6 satisfy 必須 (skeleton declaration 物理配置)

---

## 6. 関連 doc / memory

### 関連 doc (r41 章)

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter 本体 (closed 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/01-foundation.md` — sub-doc foundation (closed 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/02-portage-execution.md` — sub-doc 段階 2 (役割完了 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` — sub-doc 段階 3 (役割完了 2026-05-31、本 handoff §1.5 で close)
- `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` — sub-doc 領域 6 (active 継続)
- `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` — sub-doc 領域 7 (active 継続)
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-1-complete.md` — 段階 1 完遂 → 段階 2 着手前 prep
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-2-complete.md` — 段階 2 完遂 → 段階 3 着手前 prep
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-1a-iv-ready.md` — 3.1a-i/ii/iii 完遂 → 3.1a-iv 着手境界 (役割完了 2026-05-29)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-1a-complete.md` — 3.1a 全完遂 → 3.1b 着手境界 (役割完了 2026-05-29)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-1b-complete.md` — 3.1b 完遂 → 3.2 着手境界 (役割完了 2026-05-29)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-2-complete.md` — 3.2 完遂 → 3.3 着手境界 (役割完了 2026-05-29)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-A-complete.md` — 3.3-A 全完遂 → 3.3-B/C 着手境界 (役割完了 2026-05-29)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-B-complete.md` — 3.3-B 全完遂 → 3.4 着手境界 (役割完了 2026-05-31)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-C-complete.md` — 3.3-C 全完遂 → 3.3-B 着手境界 (役割完了 2026-05-29)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-4-complete.md` — 3.4 全完遂 → 3.5 着手境界 (役割完了 2026-05-31)
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-3-complete.md` — **本 handoff** (段階 3 完遂 → 段階 4 着手境界)

### 関連 commit (本 sub-step 3.5 で積まれた branch 上 commit)

- 3.5-a: no-commit (validation strict force-enable verify 用 patch を revert 済)
- 3.5-b: 本 commit (handoff doc + sub-doc 03 update + memory update)

### 関連 commit (段階 3 全 sub-step、参照用)

- `25228ec8ae` — sub-step 3.1a (handoff/spec)
- `8e8a846c14` — sub-step 3.1b (PSO 基盤)
- `e9948a0de8` — sub-step 3.2 (sky pool smoke)
- 3.3-A: `44b19c507e` (α) / `62778dcac4` (β-1) / `09a4563fc5` / `57d72d6976` (β-2) / `bd87c220a7` / `eb6c6f0ac5` (γ) / `00f515770a` / `00fcff6eab` (δ-1) / `f035815f55` (δ-2) / `0d6f07ac67` (ε)
- 3.3-B: `0bbfe8319f` (α) / `4ec234fa23` (β-1) / `5d4999ec4a` (β-2) / `6aeeeecf4f` / `020df91561` (γ) / `5ebfc6d33b` / `b2c06f869d` (δ) / `2def67b0ef` (ε) / `d7d4e5bef2` (B 全完遂 handoff)
- 3.3-C: `b798d27265` (α) / `3f8f1e2a1a` (β-1) / `85b3a9a513` (β-2) / `02ee477c1c` / `8ed9edd23b` (γ) / `3c3f097ef4` / `da01900f46` (δ) / `f2cdc6fcfb` / `429f67dbf3` (ε)
- 3.4: `a8f8472202` (α) / `4bdadf4eb9` (β-1) / `b0e71fe779` / `b84ad42904` (β-2) / `238d87b274` / `8f0a8eaa82` (γ) / `4e6e75257f` (δ-1) / `37f5b65916` (δ-2) / `f88e9ed464` (δ-3) / `05686ddd03` (δ-4) / `8ca3db473c` (δ-5/ε)

### 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone active (段階 4 着手 ready に更新)
- `project_ayastorm_three_platforms.md` — 3 OS 大前提 + Linux 先行例外
- `feedback_self_verify_before_handoff.md` — sub-step 3.5-a validation strict 検証本 handoff §1.1 + §3 反映
- `feedback_proactive_handoff.md` — 段階 3 完遂境界 handoff 本 doc で実施
- `feedback_no_auto_commit.md` — 全 sub-step commit は AYA 明示指示後
- `feedback_release_flow.md` — push は AYA 手動
- `feedback_build_only_verified.md` — §1.1 達成 marker は実機検証 evidence (log 抜粋本 handoff §3)
- `feedback_remove_verification_logs.md` — §1.1 force-enable 復元実施 (§3.5 patch revert clean 確認)
- `feedback_admit_unknown.md` — §1.3 RAII setter / skeleton declaration の段階 4 / 領域 8 一体運用 scope refine の正直表明
- `feedback_one_step_at_a_time.md` — 各 sub-step 1 cycle 完結
- `feedback_use_agents_proactively.md` — 段階 4 pipeline.cpp 全件追跡で Agent 並列 trace 推奨
