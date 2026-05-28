# r40 sub-phase 3 work item (b): Vulkan API 設計

**status**: **work item (b) 完了 (AYA review PASS 2026-05-28)** — group B (§6 + §7) + group C (§8 + §10) draft 追加完成、全 §1-§10 完成。後続 work item (c)(d) 完了、work item (e) charter 完成 着手中
**親 doc**: `03-sub-phase-3-vulkan-plan.md` work item (b)
**前置 doc**: `04-portage-inventory.md` work item (a) 全完了 (§6.4 設計 input が直接 source)
**達成条件**: §1-§10 全 section draft 完成 + AYA review PASS → work item (c) 工程算定 着手

---

## §1 Vulkan version + loader + SDK 選定根拠

### §1.1 Vulkan 1.3 default 採用

**結論**: Linux/Win native target は **Vulkan 1.3 を default**、Mac (MoltenVK 経由) は **1.2 core + 一部 1.3 portable subset**。

#### Vulkan 1.3 core で得られる feature (本 design で実利用)

| feature | promotion 元 | 用途 |
|---|---|---|
| **dynamic rendering** | VK_KHR_dynamic_rendering | render pass / framebuffer 抽象を簡素化 (§4 で展開)、AYAstorm 描画 stage chain の構築コスト削減 |
| **synchronization2** | VK_KHR_synchronization2 | barrier API 簡素化、access mask 細分化、半永続な image/buffer の barrier 管理を整理 (§5 で展開) |
| **push descriptor** | VK_KHR_push_descriptor | per-draw descriptor set を pool 確保せずに inline 書込み、AYAstorm 描画 per-draw cardinality (mObjectID 等) との親和性 |
| **inline uniform block** | VK_KHR_inline_uniform_block | 小サイズ material 定数を descriptor set に直接埋め込み、UBO 数の削減 |
| **timeline semaphore** | VK_KHR_timeline_semaphore | frame in flight 同期 + worker thread → main thread async upload の整理 |
| **buffer device address** | VK_KHR_buffer_device_address | VMA との連携、GPU-pointer 経由の vertex buffer 参照、shader 側 indirect addressing |

#### driver coverage (2026-05-28 時点の概算 status)

- **Linux**: NVIDIA proprietary 1.3 安定、AMDGPU-PRO 1.3、Mesa RADV / ANV 1.3 (一部 extension は driver により未対応の場合あり、§9 で個別 check)
- **Windows**: NVIDIA / AMD / Intel ともに 1.3 native
- **Mac (MoltenVK)**: 1.2 core + 一部 1.3 extension (KHR_dynamic_rendering / KHR_synchronization2 / KHR_push_descriptor は portable subset 経由で対応、詳細 §9.4)

Linux/Win 先行 (§4 (2) Linux 先行原則) なので、Mac 制約は vk-RC 直前 phase で詳細化、本 design は **Linux/Win 1.3 を first-class、Mac は subset 維持を制約** として進める。

### §1.2 loader: volk 採用

**結論**: **volk** (https://github.com/zeux/volk) を loader として採用、Vulkan SDK 同梱の libvulkan.so 直接 link は使わない。

#### volk 採用根拠

1. **header-only / single-header**: 既存 phoenix-firestorm の build system (cmake + autobuild) との統合容易、`#include <volk.h>` のみで完結
2. **extension 自動 load**: `volkLoadInstance()` / `volkLoadDevice()` 経由で `VkInstance` / `VkDevice` ごとの function pointer を自動取得、`vkGetInstanceProcAddr` の手書きが不要
3. **multiple instance / device 対応**: AYAstorm 単一 device 構成では不要だが、将来の secondary device (audio/compute 分離) 拡張に有利
4. **既存実績**: Doom Eternal / RPCS3 / OBS / WickedEngine / 等 large project で採用、stability 確立
5. **license**: MIT (LGPL viewer base と互換)

#### libvulkan.so 直接 link を避ける理由

- Linux 配布で system Vulkan loader version 依存になる (古い distro で SDK 1.3 が無い)
- volk なら implementation 内蔵で portable
- function pointer 解決を自前で書く必要なし

### §1.3 Vulkan SDK 1.3.x (LunarG)

- **Linux**: LunarG SDK 1.3.x tarball + Vulkan headers / glslang / spirv-cross 同梱、autobuild package 化
- **Windows**: LunarG SDK 1.3.x installer + 同梱 tool chain
- **Mac**: LunarG SDK 1.3.x (MoltenVK 同梱)、ただし MoltenVK 部分の compat は vk-RC 直前 phase

build artifact:
- header (vulkan_core.h, etc.) は volk が再 export
- glslang は §2.5 shader build integration で利用
- spirv-cross は §2 で利用 (MoltenVK が内部使用するため明示 invocation は基本不要、debug 用に保持)

### §1.4 段階 1 (GL header wrapper 置換) 配置

a-4 §6.3.1 段階 1 で確定した 3 file 置換:

| file | 置換内容 |
|---|---|
| `indra/llrender/llglheaders.h` | `#include <GL/gl.h>` 系 → `#include <volk.h>` + Vulkan core type alias |
| `indra/llrender/llglstates.h` | OpenGL state machine wrapper → Vulkan pipeline state placeholder (本格的な PSO 化は段階 3) |
| `indra/llrender/llgltypes.h` | GL type alias (GLuint / GLfloat / etc.) → Vulkan type alias (VkBuffer 等) + 既存 type の `using` 維持で上流 file 影響最小化 |

a-1 §1.2 で確定: 188 file が `llgl.h` wrapper 経由 → **上流 file は変更不要** (abstraction 設計の最大の追い風)。段階 1 完了で 99% の file が Vulkan header world に移行する。

---

## §2 shader cross compile chain (glslang / spirv-cross)

### §2.1 chain 全体図

```
[GLSL source 248 file + AYAstorm 改変 13 file]
        │
        ▼
   [glslang] ───── SPIR-V (.spv binary)
        │
        ├──► [Linux/Win Vulkan native] direct consume (vkCreateShaderModule)
        │
        └──► [Mac MoltenVK] SPIR-V → MSL 内部変換 (spirv-cross は MoltenVK が内蔵)
```

build time に **GLSL → SPIR-V** を pre-compile、runtime は SPIR-V binary のみ load。runtime GLSL compile は不採用 (起動時間 / driver 依存性回避)。

### §2.2 GLSL feature → SPIR-V capability mapping

a-4 §1.3 で確定した shader 種別:

| feature | 使用数 | SPIR-V 対応 |
|---|---|---|
| vertex shader | 110 | OK (core) |
| fragment shader | 124 | OK (core) |
| include / util | 14 | glslang `#include` extension で expand |
| compute shader | **0** | (使用なし) |
| geometry shader | **0** | (使用なし、Mac MoltenVK 非対応問題を回避) |
| tessellation shader | **0** | (使用なし) |
| bindless texture | **0** | (使用なし、descriptor indexing で代替予定なし) |
| atomic / coherent | **0** | (使用なし) |

→ **compute/geometry/tessellation/bindless/atomic 全部ゼロ** = SPIR-V capability set が **core profile + 標準 sampler のみ**、cross compile で **~85% 素直に通る見込み**。

残る 15% の典型 issue:
- `gl_FragCoord` / `gl_PointCoord` 等の built-in 座標系 (Vulkan は y 軸反転、glslang `--invert-y` で吸収)
- `texture2D()` 等の deprecated 関数 → glslang は modern `texture()` に自動変換
- `attribute` / `varying` (legacy GLSL) → `in` / `out` 書き換え必要 (一部 shader で残存の可能性、a-4 §1.3 で要 audit に分類)

### §2.3 248 file + AYAstorm 改変 13 file の取扱

#### base 248 file

a-4 §1.3 directory 別:

| directory | file 数 | 性質 |
|---|---|---|
| class1/deferred | 120 | deferred rendering core (g-buffer / soften lighting / sky / atmospherics) |
| class1/interface | 44 | UI (2D / font / cursor) |
| class3/deferred | 16 | high-end feature (SSAO / DoF / reflection probe 等) |
| class1/objects | 14 | object 描画 (avatar / mesh / terrain) |
| その他 | 54 | windlight / cinematic_bd / 等 |

base 248 file の SPIR-V 化は **a-4 §6.3.1 段階 2-3** の中で進行 (lldrawpool Vulkan 化 + state machine PSO 化 と並列)。

#### AYAstorm 改変 13 file

a-4 §B 系で確定した AYAstorm 機能の shader touchpoint:

| 機能 | shader file 数 | 配置 directory |
|---|---|---|
| r21.1 self-rigged picker | 2 | class1/deferred (objectIDV.glsl / objectIDF.glsl 等) |
| r30 Cinematic mode (DoF state) | 4 | class3/deferred + cinematic_bd |
| r14+ visual realism (post-process pass chain) | 7 | class3/deferred (godrays / volumetricLight / blurLight / vignette 等) |

→ 合計 **13 file**、base 248 と独立した namespace で SPIR-V 化、r42-α / r42-β / r42-γ で patch 合成 (a-4 §6.3.2)。

### §2.4 GLSL extension 依存の audit 要点

a-4 §1.3 段階で **未 audit** の項目:

- `#extension GL_ARB_*` directive の数と内容 → SPIR-V capability に直接 mapping できないものを抽出
- `precision highp/mediump/lowp` qualifier → Vulkan SPIR-V では portability 要 (Mac MoltenVK が precision 依存)
- `layout(location=N)` 既存記述の整合性 → vertex input attribute / output binding と descriptor set の整合

→ work item (b) draft 中の audit task として a-4 §6.4.2 設計検討事項に追加候補、(c) 工程算定の precondition。

### §2.5 build integration

- **cmake target**: `aya_shader_compile` を新規追加、`*.glsl` → `*.spv` を out-of-source build
- **glslang binary**: Vulkan SDK 1.3.x 同梱を使用、autobuild package で 3 OS 同梱
- **runtime load**: `LLGLSLShader` → `LLVulkanShader` (仮称) リネーム、`vkCreateShaderModule(VkShaderModuleCreateInfo{ .pCode = spv_binary, .codeSize = spv_size })` に置換
- **shader hot reload**: 開発時は file watcher で `.spv` re-compile + `vkDestroyShaderModule` + 再 create、release build は事前 compile 固定
- **descriptor set reflection**: glslang `--reflect` output を build time に json 化、§3 の descriptor pool sizing 自動算定の input

---

## §3 descriptor set / pipeline layout 設計図 (sampler 206 個収容)

### §3.1 descriptor set 構成 (3 set 採用)

**結論**: **set=0 per-frame / set=1 per-material / set=2 per-draw** の 3 set 構成。

| set | binding 頻度 | 想定 content | descriptor type |
|---|---|---|---|
| **0: per-frame** | 1 frame に 1 回 | camera matrix / sun position / time / shadow map sampler / env cubemap sampler / noise texture / AYAstorm picker output buffer | UBO + COMBINED_IMAGE_SAMPLER (共通 sampler 群) |
| **1: per-material** | material 種別ごと | diffuse / normal / specular / AO / material params UBO | COMBINED_IMAGE_SAMPLER × ~6 + UBO × 1 |
| **2: per-draw** | draw call ごと | per-object UBO (model matrix / object ID 等) / per-draw texture (avatar / attachment) | UBO + COMBINED_IMAGE_SAMPLER (動的更新) |

descriptor set 数を 2 でなく 3 にした理由:
- per-material 単独 set で **material cache** が成立する (set=1 bind 切替のみで material 切替可)、AYAstorm 描画の dispatcher (lldrawpool 13 file) と相性が良い
- per-draw を独立 set にすることで push descriptor (§3.3) との適用範囲を限定できる

### §3.2 sampler 206 個の分布 (見積)

a-4 §1.3 確定の sampler 206 個を 3 set に分配:

| set | sampler 数 (見積) | 内訳 |
|---|---|---|
| 0: per-frame | ~30 | shadow map ×4 (cascade) / env cubemap ×4 (reflection probe) / noise / blue noise / sky cubemap / atmospherics LUT / windlight LUT / ... |
| 1: per-material | ~70 | diffuse / normal / specular / AO / emissive / etc. × material 種別 (PBR / legacy blinn / 等) |
| 2: per-draw | ~106 | per-object texture (avatar BoM / attachment / 等)、descriptor pool で **動的に確保** (PSO 切替ごとに re-bind) |

per-draw set は **draw call 単位** で更新されるため、descriptor pool allocation 戦略が critical (§3.6 で展開)。

### §3.3 push descriptor 採用 (per-draw set 限定)

**結論**: set=2 per-draw に **VK_KHR_push_descriptor** を採用、set=0/1 は通常の `vkAllocateDescriptorSets`。

#### 採用根拠

- per-draw cardinality = 数千 draw call/frame、descriptor pool 確保 + write + bind の overhead が支配的
- push descriptor は `vkCmdPushDescriptorSetKHR` で command buffer に直接 inline 書込み、pool 不要 / lifetime 管理不要
- set=0/1 は静的 (1 frame に 1-数回 bind) なので通常 descriptor で十分

#### 制約

- push descriptor は 1 set あたり binding 数に制約 (driver 依存、Vulkan 1.3 minimum 32 binding)、set=2 設計時に超過しないよう注意
- Mac MoltenVK は push descriptor に対応 (portable subset 経由、§9.4 で確認)

### §3.4 AYAstorm picker buffer (r21.1) 配置

a-4 §B.1 確定の r21.1 self-rigged picker:

- 現状: GL object-ID buffer (GL_R32UI texture or SSBO)、CPU readback で picking
- Vulkan 化案: **set=0 per-frame の binding として attach**、render pass 内 fragment shader が `outObjectID` 書込み、frame 終了後 `vkCmdCopyImageToBuffer` で CPU readback buffer に転送
- 配置理由:
  - per-frame buffer なので set=0 が自然
  - 1 frame に 1 readback、複数 draw call で書込み (cumulative)
  - 既存 mObjectIDBuffer の lifetime 想定と一致

設計検討事項 a-4 §6.4.2 (5) 「r21.1 picker attachment 統合方針」= **deferred main pass 内 inline attachment** で確定 (別 pass は不採用、frame context overhead 削減のため)。

### §3.5 pipeline layout 設計 (改訂 2026-05-29: r41 sub-step 3.3-A trace + β-1 refine 結果反映)

- **descriptor set layout × 3** (set=0/1/2 別々に `VkDescriptorSetLayout` 作成)
- **push constant range**: `modelview_matrix` (mat4 = 64 bytes、Vulkan minimum guaranteed 128 bytes 内、GL 流儀継承 = view × model 結合済、`modelview` 分解 refactor は r41 sub-step 3.3 scope 伸展のため不採用、3.3-β-1 refine 2026-05-29)
- **per-frame UBO (set=0)**: matrix 配信を **二段構え** (r41 sub-step 3.3-A trace + β-1 refine 確定):
  - **binding 0**: projection 系 3 mat4 (`projection_matrix` / `inverse_projection_matrix` / `identity_matrix`、std140 で 192 bytes)
  - **binding 1**: `texture_matrix[0..3]` (4 mat4 = 256 bytes)
  - **shader 内計算 (3.3-B shader port 範疇)**: MVP / `normal_matrix` / `inverse_modelview_matrix` は vertex shader 内で `projection_matrix × modelview_matrix` 等から算出 (CPU side で per-draw 行列演算するコストを回避、shader uniform 名は base 248 file 側で push constant block / UBO に再 mapping)
  - 二段構え採用根拠: LLRender::syncMatrices() で配信する uniform 9 種 (base 248 shader 574 参照) が push constant 64 B 単独では収まらない、shader 内計算移譲 3 種で UBO binding 0 を 192 B に縮約、合計 push constant 64 B + UBO 448 B (r41 sub-step 3.3-A trace 2026-05-29、`docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` §3.1.1 設計根拠 trace inventory 参照)
- **UI matrix (mUIOffset/Scale)**: LLRender 内 `std::vector` stack で独立管理、syncMatrices scope 外 → r41 段階 4 frame context refactor で配信先決定 (本段階 3 scope 外)
- **pipeline layout cache**: 同一 layout を共有する PSO を grouping、`vkCreatePipelineLayout` の重複回避

`VkPipelineLayoutCreateInfo` 仮設計:
```
.setLayoutCount = 3
.pSetLayouts = [perFrameLayout, perMaterialLayout, perDrawLayout]
.pushConstantRangeCount = 1
.pPushConstantRanges = [{ VK_SHADER_STAGE_VERTEX_BIT, 0, 64 (mat4 modelview_matrix) }]
```

`perFrameLayout` (set=0) descriptor 構成 (3.3-A trace + β-1 refine 確定):
```
binding 0: VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stage = VERTEX | FRAGMENT, count = 1
           (PerFrameMatrixUBO: projection / inverse_projection / identity、192 B)
binding 1: VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stage = VERTEX | FRAGMENT, count = 1
           (TextureMatrixUBO: texture_matrix[0..3]、256 B)
```

### §3.6 descriptor pool sizing 戦略

- **per-frame set (set=0)**: 1 frame に 1 set 確保、pool size = frame in flight 数 × 1 (= 3)
- **per-material set (set=1)**: material 種別数 × frame in flight = ~50 × 3 = 150 (上限見積)
- **per-draw set (set=2)**: push descriptor なので pool 確保不要

`VkDescriptorPoolCreateInfo` 仮設計:
- `.maxSets` = ~200 (frame in flight + material)
- `.poolSizeCount` per type (UBO / COMBINED_IMAGE_SAMPLER) を §3.2 分布から逆算

詳細 size の確定は work item (c) 工程算定 + 実 SPIR-V reflection (§2.5) 後に精緻化。

---

## §4 render pass / framebuffer (deferred g-buffer の Vulkan 表現)

### §4.1 dynamic rendering 採用判断

**結論**: **Vulkan 1.3 dynamic rendering を採用**、`VkRenderPass` + `VkFramebuffer` 明示作成は使わない。

#### 採用根拠

1. **subpass dependency 手動管理の回避**: AYAstorm 描画 stage chain (shadow → deferred g-buffer → deferred lighting → forward alpha → sky → post-process) は subpass 内合成より stage 間明示 barrier で記述する方が見通しが良い (§5.3 layout transition 表と整合)
2. **VkRenderPass / VkFramebuffer object の管理コスト削減**: attachment 構成変更ごとに pass object 再作成不要、`vkCmdBeginRenderingKHR` で attachment を直接指定
3. **r41 段階 4 (pipeline.cpp render stage dispatch → VkRenderPass chain、a-4 §6.3.1) の実装簡素化**: 各 stage を独立 `vkCmdBeginRendering` で記述、frame context (LLPipelineFrameContext 仮称) との結合度低下
4. **Vulkan 1.3 widely available**: NVIDIA / AMD / Intel proprietary / Mesa RADV / ANV 全部対応、Mac MoltenVK は portable subset で対応 (§9.4)

#### subpass 採用しない理由 (trade-off 認識)

- mobile / tile-based GPU の subpass merge optimization は AYAstorm 主 target (PC discrete GPU) では benefit 薄い
- subpass dependency の自動 barrier insertion (tooling 利点) は喪失、ただし synchronization2 (§5.4) で明示 barrier 記述が簡素化されるため許容
- AYAstorm 描画 stage chain は subpass 内で attachment 共有する箇所が少ない (shadow 結果は texture sampling、g-buffer 結果は次 pass で sampler bind)

#### 影響範囲

- llrendertarget.{cpp,h} (a-2 §2.1.2 要再設計): `bindTarget()` / `flush()` interface を維持しつつ内部実装を dynamic rendering に置換 (§4.7)
- pipeline.cpp render stage dispatch (a-2 §2.3): 各 stage の `vkCmdBeginRendering` 呼出位置を確定 (§4.3 chain 図)

### §4.2 deferred g-buffer attachment 配置

#### 標準 attachment 構成

| attachment | VkFormat | 用途 |
|---|---|---|
| gbuffer0 | `VK_FORMAT_R8G8B8A8_UNORM` | diffuse + alpha (legacy / PBR base color) |
| gbuffer1 | `VK_FORMAT_R8G8B8A8_UNORM` | normal (octahedral encode) + smoothness |
| gbuffer2 | `VK_FORMAT_R8G8B8A8_UNORM` | specular / metallic + AO + emissive flag |
| gbuffer3 | `VK_FORMAT_R8G8B8A8_UNORM` | r21.1 picker LocalID/ObjectID (§4.5 / §3.4) |
| depth | `VK_FORMAT_D24_UNORM_S8_UINT` | depth + stencil |

注: 現 GL 実装の `gbuffer3` は LL 標準で alpha 無し (`GL_RGB16F`、memory `reference_gbuffer3_storage`)、AYAstorm では r21.1 picker 採用で RGBA 化済。Vulkan 化でも 4 channel 維持 = picker LocalID/ObjectID を `.rg` / `.ba` に packing 可能。

#### attachment load/store op

- **load op**: `LOAD_OP_CLEAR` (frame 開始時 g-buffer 全 clear)
- **store op**: `STORE_OP_STORE` (次 pass で sampler 経由 read)
- depth: `LOAD_OP_CLEAR` + `STORE_OP_STORE` (post-process / DoF が depth read する)

### §4.3 render pass chain 全体図

dynamic rendering 採用前提の AYAstorm 描画 stage chain:

```
[frame begin]
    │
    ▼
[pass 1: shadow map] ─── cascade × 4 (sun shadow) + spot light shadow
    │   attachment: depth only (4 × depth array texture)
    │   pool: lldrawpoolavatar / lldrawpoolbump 等 (shadow-eligible のみ)
    ▼
[pass 2: deferred g-buffer + picker write] ─── §4.2 attachment + r21.1 (§4.5)
    │   attachment: gbuffer0/1/2/3 + depth
    │   pool: lldrawpoolavatar / bump / materials / pbropaque / terrain / tree
    ▼
[pass 3: deferred lighting (soften)] ─── sun + light list + reflection probe
    │   attachment: HDR color (R16G16B16A16_SFLOAT)
    │   input: g-buffer × 4 + depth + shadow map × 4
    ▼
[pass 4: forward alpha + particles] ─── transparent + emissive
    │   attachment: HDR color (alpha BLEND)、depth READ_ONLY
    │   pool: lldrawpoolalpha / water
    ▼
[pass 5: sky + atmospherics] ─── windlight + r14+ atmospherics
    │   attachment: HDR color (forward write)、depth READ_ONLY
    │   pool: lldrawpoolsky / wlsky (llvosky / llvowlsky)
    ▼
[pass 6: post-process chain] ─── §4.4 詳細
    │   sub-chain: godrays → volumetricLight → blurLight → vignette → DoF (r30) → tonemap
    │   attachment: post-process ping-pong (2 個)
    ▼
[pass 7: UI + 2D] ─── llrender2dutils / font / cursor
    │   attachment: swapchain image (sRGB)
    ▼
[present]
```

各 pass は独立 `vkCmdBeginRenderingKHR` / `vkCmdEndRenderingKHR` でくくる、pass 間の attachment layout transition は §5.3 / §5.4 で詳細化。

### §4.4 r14+ post-process pass chain 統合

a-3 §5.2 で確定した r14+ visual realism shader 7 file (sampler ~25 + mat4 ~30) を pass 6 内 sub-chain として統合:

| sub-pass | shader | input attachment | output |
|---|---|---|---|
| 6-a | `godraysF.glsl` / `godraysV.glsl` (r15) | HDR color + depth + sun shaft sampler | post-process intermediate A |
| 6-b | `volumetricLightF.glsl` (class3, r30 BD import) | intermediate A + shadow map + atmospherics LUT | intermediate B |
| 6-c | `blurLightF.glsl` (r30 P3.8) | intermediate B + SSAO map | intermediate A (ping-pong) |
| 6-d | `atmosphericsF.glsl` (include) | intermediate A + windlight LUT | intermediate B |
| 6-e | vignette (`FSRenderVignette` cvar 配下) | intermediate B | intermediate A |
| 6-f | DoF (r30) — `postDeferredHQDoFF.glsl` / `postDeferredNoDoFF.glsl` / `ayaAlphaPlateCompositeF.glsl` | intermediate A + depth + per-layer alpha plate | intermediate B |
| 6-g | tonemap | intermediate B | swapchain pre-stage |

#### descriptor set 配置

- pass 6 全 sub-pass で sampler ~25 を **set=0 per-frame** (godrays sampler / shadow map / atmospherics LUT / windlight LUT / SSAO map) または **set=1 per-material** (intermediate ping-pong attachment) に分配
- §3.2 sampler 分布 (per-frame ~30 / per-material ~70 / per-draw ~106) の per-frame ~30 のうち post-process chain 関連が支配的
- mat4 ~30 は per-frame UBO に集約、`VK_KHR_inline_uniform_block` (§9.1) で descriptor set 内 inline 化

#### ping-pong attachment 設計

- intermediate A / intermediate B 2 個の HDR color attachment (`VK_FORMAT_R16G16B16A16_SFLOAT`)
- swapchain と同 resolution、`VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT`
- sub-pass ごとに layout transition (`COLOR_ATTACHMENT_OPTIMAL` ↔ `SHADER_READ_ONLY_OPTIMAL`、§5.3)

#### scene buffer alpha 保護 (memory `project_aya_visual_realism_alpha_protect`)

r14+ で additive (`ONE/ONE`) blend する shader (godrays / volumetricLight) は `frag_color.a = 0` 必須、alpha 破壊で sky 真っ白の既知 bug が再発するため Vulkan 化でも同 invariant を SPIR-V cross compile 後の validation で確認 (work item (c) 工程算定の test plan に含む)。

### §4.5 r21.1 picker attachment 統合 (§3.4 を pass 側から記述)

a-4 §6.4.2 (5) 確定: **deferred main pass (pass 2) 内 inline color attachment** で picker を統合、別 pass 不採用。

#### pass 2 attachment への picker 追加

§4.2 attachment 構成の gbuffer3 = `VK_FORMAT_R8G8B8A8_UNORM` を picker LocalID/ObjectID 格納に流用、shader (`fsObjectIDV.glsl` / `fsObjectIDF.glsl` 2 file、a-3 §5.1) の picker fragment output を gbuffer3 への書込みに割当て。

| 用途 | channel | データ |
|---|---|---|
| LocalID 下位 16-bit | gbuffer3.rg | uint16_to_unorm(LocalID & 0xFFFF) |
| LocalID 上位 16-bit | gbuffer3.ba | uint16_to_unorm((LocalID >> 16) & 0xFFFF) |

(または `VK_FORMAT_R32G32_UINT` 等の integer format に変更し、`UINT` storage 直接 write でも可。SPIR-V cross compile 後の format support 確認は (c) 工程算定の検証項目)

#### picker readback

- pass 2 終了後、`vkCmdCopyImageToBuffer` で gbuffer3 (画面領域全体ではなく cursor 周辺の小領域) を staging buffer に copy
- CPU side 読込みは frame in flight = 3 (§5.1) で **3 frame 遅延**、AYAstorm picker 用途 (cursor hover 視覚 feedback) で許容範囲
- 既存 `mObjectIDBuffer` lifetime と互換: 1 frame に 1 readback、複数 draw call で書込み (cumulative)

#### 別 pass 不採用の根拠

- 別 pass にすると frame context overhead (extra render pass setup / attachment bind / barrier × 2) が発生
- gbuffer3 を picker 専用 attachment に流用しても deferred lighting (pass 3) は gbuffer3 を不参照、競合なし
- a-4 §6.4.2 (5) で確定方針

### §4.6 sky / atmospherics pass (pass 5)

#### 対象 file

- `lldrawpoolsky.cpp` (57 LOC) — sky dome 最小描画
- `lldrawpoolwlsky.cpp` (521 LOC) — Windlight sky dome、atmospherics
- `llvosky.cpp` + `llvowlsky.cpp` (合計 2,198 LOC、a-4 §6.1) — sky dome geometry + atmospherics state

#### pass 5 構成

- forward 描画 (deferred lighting 後の HDR color attachment に直接 write)
- depth: `READ_ONLY` (sky は depth 書込まない、far plane で z-test pass)
- attachment: HDR color (pass 3 result と同じ buffer)、depth (READ_ONLY)
- shader: `class1/windlight/` 配下 8 file (a-1 §1.3) + atmospherics LUT
- sampler: sky cubemap × 1 + atmospherics LUT × 1 + windlight LUT × 1 + noise (set=0 per-frame に集約)

#### r14+ visual realism との関係

llvosky / llvowlsky は r14+ visual realism 章 (`project_ayastorm_visual_realism_chapter.md`) の atmospherics quality 改善 stage で touch されてきた基盤。Vulkan 化での要 port 範囲 (a-3 §B.2) は GLSL → SPIR-V 化 + uniform 配信方式変更 (push constant / per-frame UBO) のみ、interface 変更最小。

#### sky pass 順序の trade-off

- 現 GL 実装: forward alpha (pass 4) 後に sky 描画 (深度 z-test で z-far のみ pass)
- alternative: deferred lighting 前に sky 描画 (g-buffer skip、HDR color 初期値として書込み)
- AYAstorm では現順序維持 (pass 4 → pass 5)、変更時の visual regression risk を回避 ((c) 工程算定で別途検討項目)

### §4.7 LLRenderTarget → Vulkan attachment 移行マップ

a-4 §1.1 確定の `llrendertarget.cpp` 589 LOC / 35 GL calls の Vulkan 等価実装マップ。

#### GL call → Vulkan call 対応表

| 現 GL 実装 | Vulkan dynamic rendering 等価 | 備考 |
|---|---|---|
| `glGenFramebuffers` + `glBindFramebuffer` | (不要、dynamic rendering で消滅) | object 概念廃止 |
| `glFramebufferTexture2D` | `VkRenderingAttachmentInfo::imageView` (vkCmdBeginRendering 引数) | 都度指定 |
| `glDrawBuffers` (MRT 指定) | `VkRenderingInfo::colorAttachmentCount` + array | 同上 |
| `glReadBuffer` + `glReadPixels` | `vkCmdCopyImageToBuffer` (transfer queue) | picker readback / screenshot |
| `glBlitFramebuffer` | `vkCmdBlitImage` (or `vkCmdResolveImage` for MSAA) | post-process intermediate copy |
| `glClearColor` + `glClear(GL_COLOR_BUFFER_BIT)` | `LOAD_OP_CLEAR` (`VkRenderingAttachmentInfo::loadOp`) + `clearValue` | render pass 開始時 |
| `glClearDepthf` + `glClear(GL_DEPTH_BUFFER_BIT)` | 同上 (depth attachment) | 同上 |
| `glCheckFramebufferStatus` | (不要、`vkCreateImage` 時 format support check で代替) | validation layer で warning |
| `glGenRenderbuffers` + `glRenderbufferStorage` | `vkCreateImage` + `vkAllocateMemory` (VMA 経由) | depth buffer / multisample |
| `glFramebufferRenderbuffer` | `VkRenderingAttachmentInfo::imageView` (depth) | 同上 |

#### LLRenderTarget interface 変更

a-2 §2.1.2 で「要再設計」確定、interface 残置の方針:

- `LLRenderTarget::allocate(w, h, format, ...)` → 内部実装を `vkCreateImage` + VMA `vmaCreateImage` に置換、`VkImage` + `VkImageView` を member 保持
- `LLRenderTarget::bindTarget()` → frame context (LLPipelineFrameContext) に「次 vkCmdBeginRendering で使う attachment」を queue、実際の `vkCmdBeginRendering` 呼出は pipeline.cpp render stage 側で発行
- `LLRenderTarget::flush()` → frame context から queue を取出し `vkCmdEndRendering` 呼出
- `LLRenderTarget::getTexture(idx)` → `VkImageView` を返す、descriptor set bind 時に caller が利用

#### caller 側影響

llrendertarget interface 残置で **caller の 188 file 上流側変更は不要** (a-1 §1.2 wrapper 局在化と同じ追い風)、pipeline.cpp 内 render stage dispatch のみ書換え対象 (a-4 §6.3.1 段階 4)。

---

## §5 sync 戦略 (state diagram / barrier table)

### §5.1 frame in flight = 3 採用根拠

**結論**: **frame in flight = 3** を採用。

#### 採用根拠

| 候補値 | trade-off |
|---|---|
| 1 (no parallel) | CPU/GPU 完全直列、GPU idle 顕在化、frame time 倍化 |
| 2 | CPU/GPU 並列、ただし CPU bound shift 時に GPU idle、AYAstorm の rendering thread bound 状況で margin 不足 |
| **3** | **CPU/GPU 並列度確保 + input lag 許容範囲** (60fps で 50ms = 3 frame 遅延、AYAstorm 用途 = 撮影描画 / Cinematic で許容) |
| 4+ | input lag 顕在化 (>66ms)、memory cost 増加 (per-frame descriptor pool × 4)、AYAstorm では benefit なし |

#### 周辺数値との align

- §3.6 descriptor pool sizing: per-frame set (set=0) = frame in flight × 1 = **3 set**
- §3.6 per-material set (set=1): material 種別数 × frame in flight = 50 × **3** = 150
- §5.5 worker thread upload: timeline semaphore で main thread 側 frame index と同期、3 並列

#### industry reference

Doom Eternal / Vulkan Samples / volk 同梱 example / RPCS3 等、large project の default。AYAstorm でも違える積極理由なし。

### §5.2 fence / binary semaphore / timeline semaphore の使い分け

#### sync primitive 分類

| primitive | 用途 | AYAstorm 採用箇所 |
|---|---|---|
| `VkFence` | **CPU ↔ GPU sync** (CPU が GPU 完了を wait) | frame in flight 管理 (前 frame N-3 の GPU work 完了待ち) |
| **binary** `VkSemaphore` | **queue 間 GPU-only sync、1 回 signal → 1 回 wait** | swapchain acquire (acquire → render submit)、render → present |
| **timeline** `VkSemaphore` | **monotonic counter で多対多 sync、CPU/GPU 双方 wait 可** | worker thread (texture/mesh upload) → main thread の async 同期 |

#### timeline semaphore 採用の利点 (Vulkan 1.2 core)

- counter 比較で「N 番目まで完了」を判定、binary semaphore の「1 回限り」制約を回避
- CPU 側で `vkWaitSemaphores` でも wait 可、fence の代替として使える場合あり
- worker thread → main thread の upload commit (texture × N + mesh × M) を 1 counter で管理可能 (§5.5 で詳細)

#### 各 frame の sync primitive 一覧

```
per-frame instance (× 3 = frame in flight):
  - inFlightFence: VkFence (前 frame N-3 完了待ち)
  - imageAvailableSemaphore: binary VkSemaphore (swapchain image 取得完了)
  - renderFinishedSemaphore: binary VkSemaphore (render 完了 → present へ)

global (× 1):
  - uploadTimeline: timeline VkSemaphore (worker thread upload 同期)
```

### §5.3 image layout transition 表

§4.3 render pass chain の各 attachment の layout transition を列挙:

#### swapchain image

| timing | layout |
|---|---|
| 初期 (vkAcquireNextImageKHR 直後) | `UNDEFINED` |
| pass 7 (UI) 開始時 | `COLOR_ATTACHMENT_OPTIMAL` (transition) |
| pass 7 終了時 | `PRESENT_SRC_KHR` (transition) |

#### g-buffer (gbuffer0/1/2/3)

| timing | layout |
|---|---|
| frame 開始時 | `UNDEFINED` (`LOAD_OP_CLEAR` で初期化) |
| pass 2 (g-buffer write) 中 | `COLOR_ATTACHMENT_OPTIMAL` |
| pass 3 (deferred lighting read) 開始時 | `SHADER_READ_ONLY_OPTIMAL` (transition) |
| pass 3 終了後 (gbuffer3 picker readback) | `TRANSFER_SRC_OPTIMAL` (transition、§4.5) |

#### depth

| timing | layout |
|---|---|
| 初期 | `UNDEFINED` |
| pass 2 (g-buffer write) 中 | `DEPTH_STENCIL_ATTACHMENT_OPTIMAL` |
| pass 3-6 (read-only depth sample) | `DEPTH_STENCIL_READ_ONLY_OPTIMAL` (transition) |

#### post-process intermediate A / B

| timing | layout |
|---|---|
| pass 6 sub-chain 内 ping-pong | `COLOR_ATTACHMENT_OPTIMAL` ↔ `SHADER_READ_ONLY_OPTIMAL` 交互 |

#### shadow map (cascade × 4)

| timing | layout |
|---|---|
| pass 1 (shadow render) 中 | `DEPTH_STENCIL_ATTACHMENT_OPTIMAL` |
| pass 3 (deferred lighting sample) | `DEPTH_STENCIL_READ_ONLY_OPTIMAL` (transition) |

### §5.4 synchronization2 access mask 細分化方針

Vulkan 1.3 core `synchronization2` (§9.1) で memory dependency を細分化、over-barrier を回避。

#### 採用 access mask (代表例)

| stage | access mask | 用途 |
|---|---|---|
| color attachment write | `VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT` | pass 2 / 3 / 4 / 5 / 6 attachment write |
| depth attachment write | `VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT` | pass 1 / 2 depth write |
| shader sampled read | `VK_ACCESS_2_SHADER_SAMPLED_READ_BIT` | g-buffer / shadow / post-process intermediate sampler read |
| transfer read | `VK_ACCESS_2_TRANSFER_READ_BIT` | picker readback (§4.5) |
| transfer write | `VK_ACCESS_2_TRANSFER_WRITE_BIT` | staging buffer → texture upload (§5.5) |

#### stage mask 細分化

`VK_PIPELINE_STAGE_2_*` の細分化により、`ALL_COMMANDS` のような over-barrier を回避:

- `COLOR_ATTACHMENT_OUTPUT_BIT` (fragment shader output → attachment write)
- `FRAGMENT_SHADER_BIT` (sampler read)
- `EARLY_FRAGMENT_TESTS_BIT` / `LATE_FRAGMENT_TESTS_BIT` (depth test)
- `TRANSFER_BIT` (vkCmdCopy* / vkCmdBlit*)
- `COMPUTE_SHADER_BIT` (将来 GPU-driven culling 等、本 design では予約)

#### barrier batching

複数 attachment の layout transition を 1 回の `vkCmdPipelineBarrier2` にまとめる (pass 境界で頻発)、過剰な barrier call を回避。

### §5.5 worker thread (texture/mesh upload) → main thread async 同期

#### upload path

| upload 対象 | source | Vulkan path |
|---|---|---|
| texture | llimagegl.cpp (LLImageGLThread、a-2 §2.1.2) | staging buffer (VMA HOST_VISIBLE) → `vkCmdCopyBufferToImage` on transfer queue |
| mesh / VBO | llspatialpartition.cpp `rebuildMesh()` (a-1 §1.4) | staging buffer → `vkCmdCopyBuffer` on transfer queue |

#### timeline semaphore による同期

```
worker thread (transfer queue):
  upload N 実行 → vkQueueSubmit2 with signal { uploadTimeline, value = N }

main thread (graphics queue, frame F):
  vkQueueSubmit2 with wait { uploadTimeline, value = uploadCommitVersion_F, stage = FRAGMENT_SHADER }
  → frame F の fragment shader が読み始めるまでに upload N 完了を保証
```

- `uploadCommitVersion_F` は frame F が要求する upload 完了 counter (frame 開始時に確定)
- 1 frame に複数 upload を 1 counter で batch 管理可能、binary semaphore の「1 回限り」制約を回避

#### buffer device address との連携 (Vulkan 1.2 core, §9.1)

- vertex buffer / staging buffer に `VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT` を立てると GPU pointer (`VkDeviceAddress`) を shader から indirect 参照可能
- 将来の GPU-driven rendering / bindless 化の素地 (本 design では capability 確保のみ、利用は r45+ 検討)

#### transfer queue 選択

- discrete GPU (NVIDIA / AMD): 専用 transfer queue family あり、graphics queue と並列実行可能
- integrated GPU (Intel / Apple Silicon via MoltenVK): unified queue、transfer も graphics queue で実行、並列度低下
- `vkGetPhysicalDeviceQueueFamilyProperties2` で transfer queue 存在確認、fallback として graphics queue 使用

### §5.6 swapchain image acquisition → render → present の barrier sequence

#### per-frame sequence (frame F)

```
1. vkWaitForFences(inFlightFence_F)
   ← 前 frame F-3 の GPU work 完了待ち (CPU side)

2. vkAcquireNextImageKHR(swapchain, imageAvailableSemaphore_F, ...)
   ← swapchain image 取得、imageAvailableSemaphore_F signal

3. record command buffer:
   - pass 1 (shadow): vkCmdBeginRendering + draw + vkCmdEndRendering
   - barrier: shadow depth → SHADER_READ_ONLY (for pass 3)
   - pass 2 (g-buffer + picker): vkCmdBeginRendering + draw + vkCmdEndRendering
   - barrier: g-buffer (gbuffer0/1/2) → SHADER_READ_ONLY, gbuffer3 → TRANSFER_SRC
   - vkCmdCopyImageToBuffer (picker readback、async copy to pickerStaging buffer)
   - pass 3 (deferred lighting): vkCmdBeginRendering + draw + vkCmdEndRendering
   - pass 4-6 同様、barrier は §5.3 / §5.4 に従う
   - pass 7 (UI): vkCmdBeginRendering (swapchain image) + draw + vkCmdEndRendering
   - barrier: swapchain image → PRESENT_SRC_KHR

4. vkQueueSubmit2(graphicsQueue,
     wait = { imageAvailableSemaphore_F, COLOR_ATTACHMENT_OUTPUT },
     wait = { uploadTimeline, uploadCommitVersion_F, FRAGMENT_SHADER },
     signal = { renderFinishedSemaphore_F },
     signal_fence = inFlightFence_F)

5. vkQueuePresentKHR(presentQueue,
     wait = { renderFinishedSemaphore_F },
     swapchain image)

6. CPU side で次 frame の prep 進行 (F+1 frame の data prep、F-2 frame の picker readback 結果 consume)
```

#### 注記

- frame F の picker readback (step 3 vkCmdCopyImageToBuffer) の CPU 可視化は frame F+3 以降 (inFlightFence wait 後)、AYAstorm cursor hover 用途で許容
- swapchain re-create (resize / minimize / fullscreen 切替) 時は別 path、§7 で詳細化

---

## §6 memory allocator 方針 (VMA)

### §6.1 VMA (Vulkan Memory Allocator, GPUOpen) 採用判断

**結論**: **AMD GPUOpen VMA (https://gpuopen.com/vulkan-memory-allocator/) を全 allocation で採用**、`vkAllocateMemory` / `vkBindBufferMemory` / `vkBindImageMemory` を直接呼ぶ箇所はゼロにする。

#### 採用根拠

1. **`maxMemoryAllocationCount` 制約への対応**: Vulkan spec minimum guarantee 4096 (driver により実 4096-65536)、AYAstorm 規模で texture 数千 + VBO 数千 + UBO/staging 多数の合計 allocation がこの上限に容易に抵触。VMA は内部で大きい block を確保 → suballocate で 1 vkAllocateMemory に多 resource 詰め込みが default、上限 hit を構造的に回避
2. **memory type 自動選定**: `VMA_MEMORY_USAGE_AUTO` + 利用パターン hint (`VK_BUFFER_USAGE_*`) を渡すと driver の available memory heap から最適 type を自動選定、本線側で memory type table を書かなくて済む
3. **VkMemoryRequirements + alignment 整合自動化**: vkGetBufferMemoryRequirements / vkGetImageMemoryRequirements の alignment / size / memoryTypeBits を VMA 内部で吸収、手書き省略
4. **defragmentation API 標準提供**: §6.5 で展開、long-lived session (AYAstorm は撮影セッション数時間級) の fragmentation 対策が built-in
5. **statistics + budget API 統合**: `vmaGetHeapBudgets` / `vmaCalculateStatistics` 経由で per-heap 使用量 / per-pool 使用量を query、§6.6 VRAM 監視と統合
6. **既存実績**: Doom Eternal (id Tech 7) / RPCS3 / Wicked Engine / Granite / Anki Engine / Niagara / 等の large project で採用、stability 確立。Vulkan tutorial / Vulkan Guide も標準として推奨
7. **license**: MIT (LGPL viewer base と互換)、header-only / single-translation-unit、cmake target 統合容易

#### 自前 allocator を書かない理由

- Vulkan `vkAllocateMemory` 制約 (block 化前提 / alignment / heap 選定 / defrag) の正解実装が VMA で確立済、独自実装は drift risk のみで benefit ゼロ
- AYAstorm は描画 engine ではなく viewer (memory allocation は black box であるべき)、本線開発工数を VMA で吸収

#### VMA 統合 file

- 新規 `indra/llrender/llvkmemoryallocator.h/.cpp` (仮称) に VMA instance (`VmaAllocator`) を集約、`LLVKMemoryAllocator::getInstance()` で他 file から access
- `vmaCreateAllocator` は instance 作成直後 (vkCreateDevice 完了後 + worker thread launch 前) に 1 回呼ぶ
- 既存 `llrender/llvertexbuffer.cpp` / `llrender/llimagegl.cpp` 等の memory allocate 関数を VMA wrapper 経由に置換 (a-4 §6.3.1 段階 2-3 内)

### §6.2 memory type 分類と VMA usage hint mapping

#### Vulkan memory type の 5 種別

| memory property | host 可視 | device 速度 | 用途 | VMA usage |
|---|---|---|---|---|
| `DEVICE_LOCAL` | × | 最速 | GPU 専用 read/write (g-buffer / depth / shadow / cubemap / 完成テクスチャ) | `VMA_MEMORY_USAGE_GPU_ONLY` (deprecated) → 後継 `VMA_MEMORY_USAGE_AUTO` + `VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT` (大型 attachment 用) |
| `HOST_VISIBLE` + `HOST_COHERENT` | ◯ | 遅 | staging buffer (CPU 書込 → GPU copy) / per-frame UBO (mat4 / time 等) | `VMA_MEMORY_USAGE_AUTO_PREFER_HOST` + `VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT` |
| `HOST_VISIBLE` + `HOST_CACHED` | ◯ | 中 | CPU readback (picker readback / screenshot) | `VMA_MEMORY_USAGE_AUTO_PREFER_HOST` + `VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT` |
| `DEVICE_LOCAL + HOST_VISIBLE` (Resizable BAR / ReBAR) | ◯ | 高 | 高頻度 update する UBO / vertex buffer (modern GPU で利用可) | `VMA_MEMORY_USAGE_AUTO` (VMA が ReBAR 可用性検出) |
| `LAZILY_ALLOCATED` | × | tile memory | mobile/integrated GPU で MSAA resolve 等の transient attachment | (AYAstorm 主 target = discrete PC GPU、利用予定なし) |

#### VMA hint 渡し方の原則

a-4 §6.4.1 で確定方針 (VMA 必須採用):

- 本線 code 側は **memory type を直接指定しない**、`VmaAllocationCreateInfo::usage` + `flags` のみ渡す
- `VMA_MEMORY_USAGE_AUTO` (Vulkan 1.3 + VMA 3.x の推奨) を default、特殊用途のみ `_AUTO_PREFER_DEVICE` / `_AUTO_PREFER_HOST` で hint
- ReBAR / unified memory architecture (Apple Silicon via MoltenVK) の差異は VMA が吸収、本線 code は単一 path

#### AYAstorm 描画各 resource の usage 分布

§4 / §5 の結果と統合:

| resource | usage hint | 配置 heap 想定 |
|---|---|---|
| g-buffer 0-3 / depth (§4.2) | `_AUTO` + dedicated | DEVICE_LOCAL |
| post-process intermediate A/B (§4.4) | `_AUTO` + dedicated | DEVICE_LOCAL |
| shadow map cascade × 4 (§4.3 pass 1) | `_AUTO` + dedicated | DEVICE_LOCAL |
| swapchain image (§7) | (VMA 管理外、vkCreateSwapchainKHR が直接管理) | DEVICE_LOCAL |
| texture (BoM / attachment / 完成済) | `_AUTO` | DEVICE_LOCAL |
| vertex buffer / index buffer (mesh) | `_AUTO` | DEVICE_LOCAL or ReBAR (頻度依存、§6.4) |
| per-frame UBO (camera / time / sun) | `_AUTO_PREFER_HOST` + SEQUENTIAL_WRITE | HOST_VISIBLE+COHERENT or ReBAR |
| staging buffer (texture/mesh upload) | `_AUTO_PREFER_HOST` + SEQUENTIAL_WRITE | HOST_VISIBLE+COHERENT |
| picker readback staging (§4.5) | `_AUTO_PREFER_HOST` + RANDOM | HOST_VISIBLE+CACHED |

### §6.3 staging buffer 戦略

#### 移行対象 (a-4 §1.1)

- `llimagegl.cpp` (LLImageGLThread): texture upload (`glTexImage2D` / `glTexSubImage2D`) の Vulkan 化 → staging buffer 経由 `vkCmdCopyBufferToImage`
- `llspatialpartition.cpp` `rebuildMesh()`: VBO / IBO update の Vulkan 化 → staging buffer 経由 `vkCmdCopyBuffer`

#### staging pool 設計

**結論**: **per-thread staging pool** を採用、worker thread (LLImageGLThread 後継) と main thread で独立した `VmaPool` を保持。

| pool | 所有 thread | 用途 | 想定 size |
|---|---|---|---|
| `mainThreadStagingPool` | main render thread | per-frame UBO 更新 / 小サイズ動的 buffer | ~16 MB |
| `uploadThreadStagingPool` | worker upload thread (LLImageGLThread 後継) | texture / mesh upload | ~256 MB (frame 内 cumulative) |
| `readbackPool` | main render thread (consume), GPU writer (produce) | picker readback (§4.5) / screenshot | ~4 MB |

設計理由:
- per-thread pool で `vmaCreateBuffer` / `vmaDestroyBuffer` の thread contention 回避 (`VmaAllocator` 自体は thread-safe だが pool 分離で hot path lockless 化)
- upload thread の staging buffer は frame 内で cumulative に積み上がる (1 frame で N texture + M mesh)、large pool で `vmaCreateBuffer` 失敗 → pool 自動拡張 (`VMA_POOL_CREATE_LINEAR_ALGORITHM_BIT` で ring allocator 化)

#### staging buffer lifecycle

```
worker thread upload N (texture T):
1. vmaCreateBuffer(uploadThreadStagingPool, sizeof(T), TRANSFER_SRC,
                   HOST_ACCESS_SEQUENTIAL_WRITE)
   → 戻り値: VkBuffer + VmaAllocation
2. vmaMapMemory → memcpy(T pixel data) → vmaUnmapMemory
   (HOST_COHERENT なので flush 不要、ReBAR 上なら direct write)
3. transfer queue で vkCmdCopyBufferToImage(staging → device_local_image)
4. vkQueueSubmit2 + uploadTimeline signal(N) (§5.5)
5. frame F (CPU 側 fence wait 完了後) で vmaDestroyBuffer(staging)
```

- staging buffer の lifecycle = upload 完了の確証 (= uploadTimeline N 完了) まで保持必須
- main thread 側で `vmaDestroyBuffer` を呼ぶ前に `vkWaitSemaphores(uploadTimeline, N)` 必須 (§5.5)

#### ring allocator optimization

upload thread 用 staging pool は **`VMA_POOL_CREATE_LINEAR_ALGORITHM_BIT` で linear allocator 化** を採用候補:
- allocate は ring buffer 末尾に append (O(1))
- destroy は順序維持 (FIFO)
- texture upload は時系列順 enqueue 想定なので相性が良い
- ただし長期保持 staging (1 frame 内で完結しない巨大 texture) は別 pool に分離 (`uploadThreadStagingLongLivedPool` 仮称)

### §6.4 typical allocation pattern (vertex / index / UBO / texture)

各 resource 種別の VMA allocate template:

#### vertex buffer / index buffer

```cpp
VkBufferCreateInfo buf{
  .size = N,
  .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
         | VK_BUFFER_USAGE_TRANSFER_DST_BIT
         | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,  // §5.5 将来 BDA
};
VmaAllocationCreateInfo alloc{
  .usage = VMA_MEMORY_USAGE_AUTO,
  .flags = 0,  // 更新頻度低 = DEVICE_LOCAL 期待
};
vmaCreateBuffer(allocator, &buf, &alloc, &vkBuffer, &vmaAlloc, nullptr);
```

頻度高 (mesh が毎 frame 更新される場合) は `_AUTO_PREFER_HOST` + SEQUENTIAL_WRITE で ReBAR 候補化、要 profile (a-4 §B.4 mesh upload 頻度確認)

#### per-frame UBO (camera / sun / time)

```cpp
VkBufferCreateInfo buf{
  .size = sizeof(PerFrameUBO),
  .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
};
VmaAllocationCreateInfo alloc{
  .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
  .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
         | VMA_ALLOCATION_CREATE_MAPPED_BIT,  // 常時 mapped 維持
};
```

frame in flight = 3 (§5.1) なので 3 個確保、frame F の更新は `frameUBO[F % 3]` に書込み。

#### texture (完成済、長期保持)

```cpp
VkImageCreateInfo img{
  .imageType = VK_IMAGE_TYPE_2D,
  .format = format,  // BC7 / R8G8B8A8_UNORM / etc.
  .extent = { w, h, 1 },
  .mipLevels = mipCount,
  .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
  .samples = VK_SAMPLE_COUNT_1_BIT,
};
VmaAllocationCreateInfo alloc{
  .usage = VMA_MEMORY_USAGE_AUTO,
  .flags = 0,  // VMA が suballocate or dedicated 自動選定
};
vmaCreateImage(allocator, &img, &alloc, &vkImage, &vmaAlloc, nullptr);
```

巨大 texture (>= 64 MB 例 環境マップ cubemap) は `VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT` 明示で 1 image = 1 memory allocate に分離 (defrag 対象外化)

#### g-buffer / shadow map / post-process intermediate (attachment)

```cpp
VkImageCreateInfo img{
  .format = VK_FORMAT_R8G8B8A8_UNORM,  // §4.2
  .extent = { screenW, screenH, 1 },
  .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
         | VK_IMAGE_USAGE_SAMPLED_BIT
         | (gbuffer3 ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0),  // §4.5 picker readback
};
VmaAllocationCreateInfo alloc{
  .usage = VMA_MEMORY_USAGE_AUTO,
  .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,  // 大型 + 長期保持
};
```

### §6.5 defragmentation 採用判断

#### 採用方針

**結論**: **defragmentation は採用、ただし AYAstorm では interactive defrag (毎 frame 少しずつ) に限定、stop-the-world は禁止**。

#### 採用根拠

- AYAstorm は撮影セッションで連続 1-6 時間級の long-lived 用途 (撮影者は viewer を立ち上げっぱなしで景観/被写体探索)
- texture / mesh の load/unload が長時間に渡り蓄積、fragmentation で `vmaCreateBuffer` / `vmaCreateImage` 失敗 (=OOM) のリスク
- 1 GB-sized resource を後から確保しようとして fragmentation で確保できない事象は VMA 公式 doc でも典型ケースとして報告

#### 実装方針

```cpp
// 1 frame に 1 回 (idle stage で実行)、budget 内のみ move
VmaDefragmentationInfo defragInfo{
  .flags = VMA_DEFRAGMENTATION_FLAG_ALGORITHM_BALANCED_BIT,
  .maxBytesPerPass = 16 * 1024 * 1024,  // 1 pass 16 MB 上限
  .maxAllocationsPerPass = 32,
};
VmaDefragmentationContext ctx;
vmaBeginDefragmentation(allocator, &defragInfo, &ctx);
// pass loop: vmaBeginDefragmentationPass → 移動対象列挙 → vkCmdCopyImage/Buffer →
//            barrier → vmaEndDefragmentationPass
vmaEndDefragmentation(allocator, &ctx, nullptr);
```

- 1 frame に **16 MB / 32 allocation 上限**、frame budget の影響を avg 0.1ms 程度に抑える (a-4 §6.4.1 budget 整合)
- defrag 対象は per-pool (textures pool / vbo pool / etc.) を周期 rotation、stop-the-world は採らない
- defrag pass で move された resource は `vmaSetAllocationUserData` 経由で記録した `VkImageView` / `VkBufferView` を再生成必要 (descriptor set 再 write も)

#### 非対象

- attachment (g-buffer / depth / shadow / intermediate) は `DEDICATED_MEMORY_BIT` で defrag 対象外、move のリスク無し
- swapchain image (VMA 管理外)

#### 実装着手 timing

- r41 (GL 除去 + Vulkan 空転) では implement skip、interactive defrag は r42-α (基盤描画 stable) 以降で導入
- ただし API surface (header 内 `LLVKMemoryAllocator::scheduleDefragPass()` 仮称) は r41 で予約しておく

### §6.6 budget API (VK_EXT_memory_budget) 経由の VRAM 監視

#### enable 確認

§9.3 で `VK_EXT_memory_budget` を必須 enable 済、VMA `VmaAllocatorCreateInfo::flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT` を立てると VMA 内部で budget query が自動有効化。

#### query API

```cpp
VkPhysicalDeviceMemoryProperties2 props;
VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
vmaGetHeapBudgets(allocator, budgets);
for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i) {
    LLINFOS("vkmemory")
        << "heap " << i
        << " usage=" << budgets[i].usage   // VMA 把握分
        << " budget=" << budgets[i].budget // driver 報告 budget (= 利用可能上限)
        << " (cap=" << memProps.memoryHeaps[i].size << ")"
        << LL_ENDL;
}
```

#### 監視 cadence

- **per-frame query は禁止** (Linux driver で query が重い report あり)
- 1 秒に 1 回 / または `vmaCreateBuffer` 失敗 (OOM) 時の診断 dump で十分
- AYAPerfLog (r30 で導入) または既存 stat infrastructure に統合、HUD overlay の VRAM 使用率表示に reuse 検討

#### 既存 viewer の VRAM stat との関係

- llrender 内に既存 `gMaxVramUsageBytes` 等の GL 経由 query (NVX_gpu_memory_info / WGL_ATI_meminfo) があるが、Vulkan 化で廃止
- 全 VRAM 監視は Vulkan budget API に一本化、driver-specific GL extension への依存解消

#### threshold action

- budget の 90% 超で warning log、AYAstorm 側で texture cache TTL 短縮 / VBO defrag schedule を発火
- 95% 超で critical (texture lod 強制低下 等の AYAstorm 側 graceful degradation policy は r42+ 検討、本 §6.6 では監視 API 確立まで)

---

## §7 swapchain / present mode

### §7.1 present mode 選択

#### Vulkan present mode 一覧と本 design 採否

| mode | tear | latency | GPU 負荷 | AYAstorm 採否 |
|---|---|---|---|---|
| `VK_PRESENT_MODE_FIFO_KHR` | なし | 高 (VSync 待ち) | 低 | **default 採用** (VSync ON 相当) |
| `VK_PRESENT_MODE_FIFO_RELAXED_KHR` | 一部あり (frame drop 時のみ) | 中-高 | 低 | 採用 (VSync ON + 遅延緩和 user 向け) |
| `VK_PRESENT_MODE_MAILBOX_KHR` | なし | 低 | 高 (GPU 余剰描画) | 採用 (low-latency user 向け、VSync OFF 相当) |
| `VK_PRESENT_MODE_IMMEDIATE_KHR` | あり | 最低 | 中 | **不採用** (撮影章 = tear 出る = 採れない) |

#### user 設定 mapping (cvar)

既存 `VSyncMode` (or 類似) cvar を Vulkan present mode に mapping:

| cvar value | present mode | 用途 |
|---|---|---|
| 0 (Always On) | `FIFO_KHR` | default、battery-friendly |
| 1 (Adaptive) | `FIFO_RELAXED_KHR` (driver 未対応時は FIFO_KHR fallback) | 遅延緩和、tear 局所許容 |
| 2 (Off / Triple Buffer) | `MAILBOX_KHR` (driver 未対応時は FIFO_KHR fallback) | 低 latency、撮影/Cinematic mode で benefit |

`IMMEDIATE_KHR` は **撮影章の本旨 (tear 無し撮影品質) に反する** ので cvar 選択肢から除外、user 側で要望が出た場合のみ別途検討。

#### Mac MoltenVK 制約

- Metal layer (CAMetalLayer) は FIFO / MAILBOX サポート、IMMEDIATE は MoltenVK で `displaySyncEnabled` を OFF にして近似 (本 design 採用しないので非該当)
- ProMotion (120 Hz 可変 refresh) 対応は MoltenVK が Metal 側で自動、Vulkan 側追加処理不要 (§8.3 で詳細)

#### support 検出

`vkGetPhysicalDeviceSurfacePresentModesKHR` で driver/surface 支持 mode を query、unsupported なら FIFO_KHR (必須対応 mode) に fallback。

### §7.2 swapchain image 数

#### 結論

**default 3 image 採用**、§5.1 frame in flight = 3 と align。

#### 算定根拠

| present mode | 推奨 image 数 | 理由 |
|---|---|---|
| `FIFO_KHR` | 2 or 3 | 2 = double buffer (VSync 待ちで GPU idle 発生確率高)、**3 = triple buffer (推奨)** |
| `MAILBOX_KHR` | 3 (必須相当) | acquire/present/displayed の 3 個必要 |

`VkSurfaceCapabilitiesKHR::minImageCount` / `maxImageCount` を query、driver 上限内で `max(3, minImageCount)` を選択 (一部 driver は minImageCount = 3、Mac MoltenVK は 2 minimum 報告 case あり)。

#### frame in flight = 3 との align

§5.1 で frame in flight = 3 確定、swapchain image 数 = 3 と一致させると acquire → render → present の 1:1 対応で実装簡素化。

frame in flight != swapchain image 数 のケース (例 FIFO で swapchain 2 + in flight 3) も Vulkan spec 上は許容だが、acquire の blocking 動作で実質 in flight が swapchain image 数に律速されるため、整合させる方が予測可能。

### §7.3 surface format (sRGB / linear / HDR)

#### default format 選定

**結論**: **`VK_FORMAT_B8G8R8A8_SRGB` (or `R8G8B8A8_SRGB`)** を default、`VK_COLOR_SPACE_SRGB_NONLINEAR_KHR` color space と組合せ。

#### 選定根拠

- 全 driver (Linux/Win/Mac) で必須対応 format、fallback 不要
- swapchain attachment への描画時に自動 linear → sRGB 変換 (GPU 側 hardware)、shader は linear で描画して `vkCmdEndRendering` で自動 encoding
- 現 GL 実装の `glEnable(GL_FRAMEBUFFER_SRGB)` 等価動作

#### `VK_FORMAT_*_SRGB` 採用 vs `*_UNORM` + 手動 encoding

| 候補 | shader 出力 | encoding |
|---|---|---|
| `_SRGB` swapchain | linear (現状の lldrawpool 描画と整合) | hardware 自動 |
| `_UNORM` swapchain | linear → sRGB を shader 末尾で手動 (`pow(x, 1/2.2)` 等) | software |

**`_SRGB` 採用**: shader 側 modify ゼロ、現 GL の `GL_FRAMEBUFFER_SRGB` 挙動と互換、AYAstorm の post-process chain (tonemap → swapchain) の output 接続も linear で揃う。

#### HDR (将来検討、本 design では予約のみ)

| HDR format | color space | driver/OS |
|---|---|---|
| `VK_FORMAT_A2B10G10R10_UNORM_PACK32` | `VK_COLOR_SPACE_HDR10_ST2084_EXT` (HDR10) | Win + RTX/RDNA driver + HDR monitor |
| `VK_FORMAT_R16G16B16A16_SFLOAT` | `VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT` (scRGB) | Win + 一部 driver、Mac/Linux 限定的 |

- AYAstorm 撮影章で HDR output 需要は将来あり (r14+ visual realism の延長)、ただし monitor 普及率 / Linux driver 整備未成熟 / camera-OS workflow 確立必要、本 design では予約のみ
- enable 要 `VK_KHR_swapchain_mutable_format` (§9.2) + `VK_EXT_swapchain_colorspace` (要追加検討、本 §9 未収載)
- 採用 timing は r45+ visual realism 次世代 milestone、本 work item (b) では「HDR への昇格余地を消さない format / color space 設計」を要件として残す

#### support 検出

`vkGetPhysicalDeviceSurfaceFormatsKHR` で driver/surface 支持 (format, color space) ペア list を query、優先順:
1. `(B8G8R8A8_SRGB, SRGB_NONLINEAR_KHR)`
2. `(R8G8B8A8_SRGB, SRGB_NONLINEAR_KHR)`
3. list 先頭 (driver 推奨を信頼)

### §7.4 resize / minimize / fullscreen 切替時の re-create 戦略

#### trigger 条件

- window resize event (LLWindow callback → LLPipeline / LLViewerWindow → swapchain re-create)
- minimize / restore (一部 driver は minimize 中 acquire を block、resize 時と同じ path)
- fullscreen ↔ windowed 切替
- monitor 切替 (multi-monitor、§7.5)
- `vkAcquireNextImageKHR` / `vkQueuePresentKHR` が `VK_ERROR_OUT_OF_DATE_KHR` / `VK_SUBOPTIMAL_KHR` を返却

#### re-create sequence

```
1. vkDeviceWaitIdle  (全 in-flight frame の完了確実化、frame in flight = 3 全部 wait)
   ← 例外: VK_EXT_swapchain_maintenance1 利用時は per-image fence 個別 wait で短縮可 (§7.6)

2. vkDestroyImageView × N  (旧 swapchain image view)
3. vkDestroySwapchainKHR(oldSwapchain) で旧 swapchain 解放
   ← alternatively VkSwapchainCreateInfoKHR::oldSwapchain = oldSwapchain で in-place 渡し、driver 内最適化

4. 再 vkGetPhysicalDeviceSurfaceCapabilitiesKHR で新 extent 取得
5. vkCreateSwapchainKHR で新 swapchain 作成
6. vkGetSwapchainImagesKHR で新 image 取得 + vkCreateImageView × N

7. attachment (g-buffer / depth / shadow / post-process intermediate) も resolution 変更時は re-create
   ← VMA 経由 vmaDestroyImage + vmaCreateImage、§6.4 attachment allocate template

8. descriptor set の sampler binding も再 write 必要 (g-buffer / post-process intermediate のように
   resize で recreate された image view を sampler binding していた set)

9. command buffer も全 reset (record した dynamic rendering attachment が無効化)、
   次 frame で再 record
```

#### minimize 時の制約

- `VkSurfaceCapabilitiesKHR::currentExtent = { 0, 0 }` 返却ケース (Win 一部 driver の minimized window)
- swapchain create 不可、`vkAcquireNextImageKHR` も呼べない
- 対応: extent 0 検出時は render loop skip + minimize 解除 wait、`WM_RESTORE` 相当 event で再開

#### fullscreen 切替 (Linux X11 / Wayland)

- X11: 既存 LLWindow flow で対応、swapchain re-create のみ Vulkan 側で対応
- Wayland: `xdg_toplevel.set_fullscreen()` 経由、surface re-create 不要 (extent change のみ)、swapchain re-create で対応
- Win: borderless fullscreen が大半、true fullscreen exclusive は `VK_EXT_full_screen_exclusive` で取得可、ただし AYAstorm では benefit 限定 (撮影章 = borderless で十分) なので本 design では非採用

#### Mac MoltenVK

- CAMetalLayer の `drawableSize` 変更で swapchain 自動 invalidate、`OUT_OF_DATE_KHR` 経路で re-create
- ProMotion (可変 refresh) の refresh rate 変更は MoltenVK 内部処理、Vulkan 側追加処理不要

### §7.5 multi-monitor / DPI scaling

#### multi-monitor 切替

- LLWindow が monitor 切替を検出 → swapchain re-create (§7.4 sequence)
- monitor 間の color space 差 (sRGB monitor ↔ HDR monitor 等) は将来 HDR 採用時に対応、本 design では sRGB 統一 (§7.3)
- monitor ごとの refresh rate 差は FIFO/MAILBOX で driver 側が自動追従

#### DPI scaling

- Win: per-monitor DPI awareness manifest (既存 viewer で設定済 想定)、`GetDpiForWindow` 経由で scale factor 取得、UI 側 (pass 7、§4.3) で対応
- Mac: NSWindow `backingScaleFactor`、CAMetalLayer の `contentsScale` 設定 (MoltenVK が自動)
- Linux X11: `XGetWindowAttributes` + `XRRGetScreenResources` 経由、Wayland: `wl_output::scale`
- swapchain image extent は scale 後の物理 pixel で確保、UI render は scale を反映した projection (LLViewerWindow 既存 path で対応)

Vulkan 側は **swapchain extent = 物理 pixel** で扱う、scale 計算は LLWindow / LLViewerWindow 側責務 (現 GL 実装と同じ役割分担)

### §7.6 VK_EXT_swapchain_maintenance1 採用判断

#### extension の追加機能

- `VK_PRESENT_MODE_FIFO_LATEST_READY_EXT`: FIFO の queue 最後の image を present、tear 無しで latency 緩和
- `VkSwapchainPresentScalingCreateInfoEXT`: swapchain extent と window extent の mismatch を driver scale で吸収 (resize 中の transient 状態緩和)
- `VkSwapchainPresentFenceInfoEXT`: per-image fence で present 完了を CPU side wait、§7.4 の `vkDeviceWaitIdle` を per-image wait に置換可能 (re-create 中の latency 短縮)

#### 採用判断

§9.3 で必須採用 listed 済、本 design で利用する具体機能:

1. **`PresentFenceInfoEXT`**: re-create 時の `vkDeviceWaitIdle` を **per-image fence wait** に置換、resize 時 frame drop を 3 frame → 1 frame 程度に短縮
2. **`PresentScalingCreateInfoEXT`**: resize 中の transient extent mismatch を `VK_PRESENT_SCALING_ONE_TO_ONE_EXT` で driver scale 吸収、ちらつき抑制

`FIFO_LATEST_READY_EXT` は §7.1 cvar mapping の adaptive (= FIFO_RELAXED) 上位互換として、driver 対応駆動で将来導入。

#### driver 対応

2026-05-28 時点:
- Mesa RADV / ANV / NVIDIA proprietary: 対応
- Win NVIDIA / AMD: 対応
- Win Intel: 一部 driver 未対応 (要 runtime feature query で fallback)
- Mac MoltenVK: 未対応 (Metal 側に対応物なし)、`vkDeviceWaitIdle` fallback で機能維持

#### runtime feature query

```cpp
VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT scm1{};
scm1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;
VkPhysicalDeviceFeatures2 f2{ .pNext = &scm1 };
vkGetPhysicalDeviceFeatures2(physDev, &f2);
if (scm1.swapchainMaintenance1) { /* enable + use */ }
else                            { /* fallback to vkDeviceWaitIdle */ }
```

extension enable は § 9.3 で device extension list に追加、未対応 driver は graceful degradation (機能差は re-create latency のみ、user 影響軽微)。

---

## §8 3 OS 対応詳細 (Mac MoltenVK 制約含む)

a-4 §6.4.3 の通り **Mac 詳細は vk-RC 直前 phase で詳細化**、本 §8 は方針 + 既知 capability matrix のみ。Linux/Win は §1.1 driver coverage 表を OS 別に展開し、本 design で採用する extension の対応状況を列挙。

### §8.1 Linux driver capability matrix

#### driver 別 status (2026-05-28 時点)

| driver | Vulkan core | 採用 KHR/EXT 対応 | AYAstorm priority |
|---|---|---|---|
| **Mesa RADV** (AMD open source) | 1.3 full | §9.2/§9.3 全部対応 (`swapchain_maintenance1` Mesa 24.x 以降) | **first-class** (AYAstorm 開発機 = AMD/Linux) |
| **Mesa ANV** (Intel open source) | 1.3 full | 同上、`push_descriptor` `inline_uniform_block` OK | first-class (Intel Arc / iGPU) |
| **NVIDIA proprietary** | 1.3 full | 全部対応、`buffer_device_address` も hardware 加速 | first-class (大半の Linux gamer) |
| **AMDGPU-PRO** (AMD proprietary) | 1.3 | 大半対応 (`swapchain_maintenance1` 確認要)、professional 用途 | second-class (Mesa RADV で十分) |
| **Mesa LLVMpipe** (CPU fallback) | 1.3 (低 perf) | 大半対応、本 design は GPU 前提なので非対応扱い | non-goal |

#### Linux 採用 GPU の現実分布

- AYAstorm 開発機 = AMD RX 7900 XTX + Mesa RADV (本線検証 baseline)
- AYAstorm user base は Linux 比率高めだが NVIDIA proprietary 利用者も多い、RADV / NVIDIA 両 driver で動作確認必須
- Intel ARC / iGPU は AYAstorm 規模描画では perf 不足が想定、対応 (起動 + 描画可) は維持、quality 段は要求しない

#### Vulkan 1.3 minimum requirement の妥当性

- Mesa 22.x 以降で RADV / ANV ともに 1.3 declare、Ubuntu 22.04 LTS 標準で 1.3 利用可
- NVIDIA proprietary 525+ で 1.3 安定 (2022 年末以降)、現行 580+ で full support
- Ubuntu 24.04 LTS / Fedora 40+ / Arch rolling では 1.3 が default
- AYAstorm の Linux user base は rolling / LTS-22 以降が大半、Vulkan 1.3 minimum は妥当 (Ubuntu 20.04 LTS user は対象外、本線 GL build からの強制移行も r41+ で発生するので別途案内)

### §8.2 Windows ICD 仕様の差分

#### driver 別 status

| driver | Vulkan core | 採用 KHR/EXT 対応 | 備考 |
|---|---|---|---|
| **NVIDIA GeForce / Quadro** | 1.3 full | §9.2/§9.3 全部対応 | RTX 20/30/40 系 stable、GTX 10 系も 1.3 declare |
| **AMD Radeon Software (Adrenalin)** | 1.3 full | 全部対応、`swapchain_maintenance1` 対応済 | RDNA 1/2/3 stable |
| **Intel ARC / Iris Xe** | 1.3 | 大半対応、`swapchain_maintenance1` 一部 driver 未対応 | runtime feature query で fallback (§7.6) |

#### Windows 固有事項

- ICD (Installable Client Driver) registry は driver installer が登録、loader (volk) が自動列挙
- `VK_LAYER_KHRONOS_validation` は LunarG SDK installer 経由で global 配置、開発 build で enable
- WHCK (Windows Hardware Compatibility Kit) Vulkan logo program 経由で driver 認定、AYAstorm 対応 driver の minimum 版数を release note に記載

#### swapchain WSI

- `VK_KHR_win32_surface` 経由 `HWND` + `HINSTANCE` で surface 作成
- LLWindow Win32 implementation の HWND を流用 (現 GL は WGL 経由、Vulkan で WGL 不要化、context 削除)
- exclusive fullscreen は `VK_EXT_full_screen_exclusive` で取得可、本 design 非採用 (§7.4)

### §8.3 Mac MoltenVK 経由の portability subset

#### 基本方針

a-4 §6.4.3 + charter §4 (2) で確定:
- **Linux 先行 + Mac 互換性 maintain** が r40 章の前提
- 本 (b) 設計では「Mac portable subset から外れない範囲で設計」が制約
- 詳細 (driver/MoltenVK 版数 / Metal API version / shader 互換) は **vk-RC 直前 phase** (r45+ 相当) で詳細化

#### MoltenVK 経由の Vulkan version

- MoltenVK は **Vulkan 1.2 core + 一部 1.3 KHR extension** を portable subset として export
- 本 design で利用する 1.3 core feature (dynamic rendering / synchronization2 / push descriptor / inline uniform block) は MoltenVK で portable subset 経由対応
- 詳細 mapping は §9.4 で確定済 (MoltenVK 1.2.x 以降)

#### Mac で利用不可 / 制約のある機能

| Vulkan 機能 | Mac MoltenVK status | 本 design 対応 |
|---|---|---|
| geometry shader | 非対応 (Metal なし) | shader 棚卸し (§2.2) でゼロ確定済、影響なし |
| tessellation shader | Metal tessellation で代替、対応 | 棚卸しゼロ、影響なし |
| compute shader | 対応 (Metal compute) | 棚卸しゼロ、本 design 範囲外 |
| `VK_FORMAT_*_D24_UNORM_S8_UINT` | Metal 直接対応なし、`D32_SFLOAT_S8_UINT` に内部置換 | depth attachment format は driver query 後選定 (§4.2 では D24S8 仮、§9.4 vk-RC で確定) |
| transfer queue | unified queue、graphics queue で transfer 兼用 | §5.5 で fallback path 確定済 |
| `VK_EXT_swapchain_maintenance1` | 非対応 | §7.6 で `vkDeviceWaitIdle` fallback 確定済 |
| ray tracing | 非対応 | r45+ 検討項目、本 design 範囲外 |

#### Apple Silicon 固有事項

- M1/M2/M3 系 = unified memory architecture (UMA)、VMA `_AUTO_PREFER_HOST` で実質 ReBAR 相当に動作 (§6.2)
- ProMotion (120 Hz 可変 refresh) = Metal 側で自動対応、Vulkan 側追加処理不要 (§7.4)
- macOS 14+ minimum (t-noami さん Mac 移植時に確定): Metal 3 minimum、MoltenVK 1.2.x 安定動作の前提条件

#### t-noami さん Mac 移植 workflow との連携

- AYAstorm Mac build は t-noami さんが担当 (CLAUDE memory `feedback_credit_t_noami_equal_billing`)、Linux build 完成 → t-noami さん検証 → Mac 固有問題は patch return の現行 flow を Vulkan 化でも維持
- Mac build 用 autobuild 設定 (MoltenVK 同梱、LunarG SDK Mac 版) は r41 終盤 / r42-α 初期で整備
- 本 §8.3 確定方針は t-noami さんに事前共有、Mac portage 着手前に review

### §8.4 WSI (window system integration)

#### platform 別 surface extension

| OS | platform | extension | surface 入力 |
|---|---|---|---|
| Linux X11 | X11 | `VK_KHR_xcb_surface` (xcb) or `VK_KHR_xlib_surface` (Xlib) | `xcb_connection_t*` + `xcb_window_t` |
| Linux Wayland | Wayland | `VK_KHR_wayland_surface` | `wl_display*` + `wl_surface*` |
| Windows | Win32 | `VK_KHR_win32_surface` | `HINSTANCE` + `HWND` |
| Mac | Cocoa + Metal | `VK_EXT_metal_surface` (推奨) or `VK_MVK_macos_surface` (legacy) | `CAMetalLayer*` |

#### Linux X11/Wayland 自動選択

- LLWindow Linux implementation が現在 X11 (SDL2 経由) 採用、Wayland 対応は phoenix-firestorm 系統で部分対応
- Vulkan 化で `VK_KHR_xcb_surface` (X11) と `VK_KHR_wayland_surface` (Wayland) の両方を instance extension に enable、LLWindow 検出した backend に応じて surface create を分岐
- Wayland native 化は AYAstorm r40+ 範囲外、r45+ 検討 (現 SDL2 X11 path を当面維持)

#### Mac `VK_EXT_metal_surface` 採用

- `VK_MVK_macos_surface` は MoltenVK legacy extension、Khronos 標準化された `VK_EXT_metal_surface` (Vulkan SDK 1.2+) を推奨
- LLWindow Mac implementation で NSView → CAMetalLayer 取得、`VkMetalSurfaceCreateInfoEXT` で surface 作成
- t-noami さんの Mac 移植 workflow で確認、現 GL Mac は AGL/CGL 経由、Vulkan 化で MoltenVK 経由に置換

### §8.5 Linux 先行 + Mac 互換性 maintain 方針 (Vulkan 設計反映)

#### charter §4 (2) の Vulkan 設計反映

charter §4 (2)「Linux 先行 (Mac は後追い / 互換性 maintain)」を本 design 各所で具体反映:

| 項目 | Linux 先行で確定 | Mac 互換 maintain 制約 |
|---|---|---|
| Vulkan 1.3 default (§1.1) | Linux/Win native 1.3 | Mac は 1.2 core + portable subset、不一致は §9.4 で明示 |
| dynamic rendering (§4.1) | Linux/Win 1.3 core | Mac portable subset で対応、shader 側 modify 無し |
| `_SRGB` swapchain (§7.3) | Linux/Win full | Mac 同 format 対応 |
| `VK_EXT_swapchain_maintenance1` (§7.6) | Linux/Win 対応 driver で利用 | Mac 未対応 = `vkDeviceWaitIdle` fallback |
| transfer queue (§5.5) | Linux/Win discrete GPU で並列 | Mac unified queue = graphics queue 兼用 fallback |
| depth format D24S8 (§4.2) | Linux/Win 全 driver 対応 | Mac は D32_SFLOAT_S8_UINT に内部置換 (MoltenVK 自動) |
| ray tracing (§9.5) | Linux/Win RTX/RDNA で将来検討 | Mac 非対応 (Metal RT は別 API)、AYAstorm 非採用方向 |

#### 「互換性 maintain」の運用意味

- 「Mac で全機能動く」ではなく「**Mac portable subset で起動 + 描画基本動作する**」が r40 章 minimum bar
- 機能差 (上記表の Mac 制約列) は Mac user 向け release note で明示、Linux/Win first-class 機能を Mac で graceful degradation
- t-noami さん Mac 移植検証で問題発生時は本 §8 / §9.4 の portable subset 前提に照らして対処 (driver 修正待ち / fallback path 追加 / 機能 OFF default Mac のみ)

#### Linux 先行が許容される根拠

- charter §4 (2) で明示確定 (AYAstorm 開発機 = AMD/Linux baseline)
- 3 OS 大前提 (memory `project_ayastorm_three_platforms`) は r40 章で **段階的 (phase 化) に達成**、r41 = Linux only OK、r42-α = Win 追加、r42-β-γ-δ = Mac 追加の順 (a-4 §6.3.2)
- r41 着手時点で Win/Mac は GL build 維持、新 user 体感の連続性は確保

---

## §9 extension 採用 list (vulkan-1.3 base + 必須 extension)

### §9.1 Vulkan 1.3 core feature (extension 化不要)

§1.1 table 再掲、本 design で **必ず enable する 1.3 core feature**:

- dynamic rendering (`VkPhysicalDeviceVulkan13Features::dynamicRendering`)
- synchronization2 (`::synchronization2`)
- push descriptor (`::pushDescriptor` / VK_KHR_push_descriptor は 1.3 で core 化はされていない、§9.2 で KHR として要 enable)
- inline uniform block (`::inlineUniformBlock`)
- timeline semaphore (Vulkan 1.2 core)
- buffer device address (Vulkan 1.2 core)

注: push descriptor は **Vulkan 1.4 で core 化**、Vulkan 1.3 では `VK_KHR_push_descriptor` extension として enable 必須。

### §9.2 必須採用 extension (KHR)

| extension | 用途 | Mac MoltenVK |
|---|---|---|
| **VK_KHR_swapchain** | window surface present | OK (1.0 から) |
| **VK_KHR_surface** | window surface | OK |
| **VK_KHR_xcb_surface** / **VK_KHR_wayland_surface** | Linux WSI | (Linux のみ) |
| **VK_KHR_win32_surface** | Windows WSI | (Win のみ) |
| **VK_EXT_metal_surface** | Mac WSI | (Mac のみ、EXT だが metal-specific) |
| **VK_KHR_push_descriptor** | per-draw set (§3.3) | OK (portable subset) |
| **VK_KHR_swapchain_mutable_format** | HDR / linear-sRGB 切替検討 (§7) | 要確認 |
| **VK_KHR_external_memory_*** (将来) | OBS / NDI 連携 (r25-r29 3D stream) を Vulkan 化する場合 | (r45+ 検討、本 design では予約のみ) |

### §9.3 必須採用 extension (EXT / その他)

| extension | 用途 |
|---|---|
| **VK_EXT_debug_utils** | validation layer 出力 + debug marker (RenderDoc / NSight 連携) |
| **VK_EXT_calibrated_timestamps** | CPU/GPU clock alignment、AYAPerfLog 等の perf 計測連携 |
| **VK_EXT_memory_budget** | VRAM 使用量 query (§6.6) |
| **VK_EXT_swapchain_maintenance1** | swapchain re-create 簡素化 (§7.4) — 1.3 driver で広く対応 |
| **VK_EXT_vertex_input_dynamic_state** | vertex input layout の dynamic 化 (PSO cardinality 削減、§§a-4 §6.4.2 (2)) |

### §9.4 MoltenVK 互換性 check 要点 (vk-RC 直前 phase で詳細化)

a-4 §6.4.3 で本 design では「Linux 先行 + Mac 互換性 maintain 方針のみ」と確定済。check 要点のみ列挙、詳細は vk-RC 直前 phase:

- portable subset (VK_KHR_portability_subset) で MoltenVK が表明する制約と本 design の features が衝突しないか
- 採用 KHR/EXT extension が MoltenVK で実装されているか (push descriptor / dynamic rendering / synchronization2 は portable subset で OK、外れ extension は vk-RC で別実装または fallback)
- MSL (Metal Shading Language) への spirv-cross 変換で全 248 + 13 shader が通るか (compute/geometry/tessellation ゼロ確定なので低リスク)
- macOS 14+ 制約 (Metal 3 minimum) — t-noami さん Mac 移植時の OS minimum

### §9.5 optional / future extension (r45+ 検討、本 design は予約)

- **VK_KHR_ray_tracing_pipeline** + **VK_KHR_acceleration_structure**: r14+ visual realism 高度化 (ray traced reflection / GI) の長期 option、Mac MoltenVK は ray tracing 未対応のため Linux/Win 限定
- **VK_KHR_video_decode_queue**: 3D stream (r25-r29) の H.264/HEVC hardware decode、AYAudio との連携検討は本 design スコープ外
- **VK_EXT_mesh_shader**: traditional vertex pipeline 置換、AYAstorm 規模では benefit 不明 (Doom Eternal / Alan Wake 2 等 large mesh 向け)

---

## §10 abstraction interface 設計 (r41.5 分離 skeleton のみ)

a-4 §6.4.3 + charter §6 / §4 (4) Phase 2 の通り、**詳細 interface は r41.5 charter で確定**。本 §10 は r41 = 本線同居 段階で「**r41.5 分離を阻害しない skeleton を本線に仕込んでおく**」ことが主眼。

### §10.1 r41.5 milestone と AYAstorm VK repo 分離前提

#### charter での位置付け再掲

| milestone | 内容 | 本 §10 との関係 |
|---|---|---|
| r41 (GL 除去 + Vulkan 空転) | Vulkan layer は本線 in-tree、描画は最低限 (clear + present 程度) | 本 §10 skeleton を本線内 `indra/llrender/vk/` 配下に配置 |
| r41.5 (VK repo 分離) | Vulkan layer を別 repo (AYAstorm VK) に切出、本線とは header API + dynamic link で接続 | r41 で仕込んだ skeleton を r41.5 で repo 分離、本線 ↔ VK repo の API 不変 |
| r42-α 以降 (描画機能 port) | VK repo 側で描画機能段階拡張 (g-buffer / deferred lighting / forward alpha 等) | 本線側 unmodified、VK repo 側 update のみで描画進化 |

#### 分離の目的 (charter §4 (4) Phase 2 再掲)

1. **license 境界の明確化**: 本線 = LGPL (LL viewer license 継承)、VK repo = 独自 license (AYAstorm 著作部分 = Vulkan 描画 layer) を dynamic link 境界で合法分離
2. **AYAstorm 描画 layer の独立進化**: LL 上流 merge 影響を受けず、Vulkan 描画 layer 単独で release / version 管理
3. **VK repo の reuse 可能性**: 将来別 viewer base / 別 project でも AYAstorm VK layer 再利用可、ecosystem 化

#### r41 → r41.5 移行の前提

r41 完成時点で本 §10 skeleton が本線にあり、`indra/llrender/vk/` を別 repo 切出 → header API のみ本線残置 (dynamic link 化) の作業が r41.5。

### §10.2 interface skeleton (header surface)

#### skeleton 構成方針

本線 ↔ VK repo 間の API surface を **C++ pure virtual interface** + **C ABI 互換 entry point** の 2 層構成:

| 層 | 役割 | r41 配置 | r41.5 分離後 |
|---|---|---|---|
| pure virtual interface (`ILLVKRenderer` 等) | 本線 code が呼ぶ抽象 interface | 本線 header に置く | 同左 (本線内 header) |
| C ABI entry point (`LLVKRenderer_create()` 等) | shared library boundary、symbol 解決 | static link (r41) | dynamic link (r41.5、`.so` / `.dll` / `.dylib`) |
| concrete 実装 (`LLVKRenderer_Impl`) | Vulkan call の実装本体 | 本線内 `indra/llrender/vk/` | **VK repo 側** に切出 |

#### interface skeleton draft (例)

```cpp
// indra/llrender/llvkrenderer_interface.h (本線、r41/r41.5 両方で同じ)
class ILLVKRenderer {
public:
    virtual ~ILLVKRenderer() = default;

    // lifecycle
    virtual bool initialize(const LLVKInitParams& params) = 0;
    virtual void shutdown() = 0;

    // frame
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    virtual void present() = 0;

    // resource (本線 GL world と互換維持)
    virtual LLVKBufferHandle createVertexBuffer(size_t size, const void* data) = 0;
    virtual LLVKTextureHandle createTexture2D(uint32_t w, uint32_t h, VkFormat fmt, const void* pixels) = 0;
    virtual void destroyBuffer(LLVKBufferHandle h) = 0;
    virtual void destroyTexture(LLVKTextureHandle h) = 0;

    // render pass dispatch (§4.3 7 pass chain)
    virtual void beginPass(LLVKPassID pass) = 0;
    virtual void endPass() = 0;

    // descriptor / pipeline (§3)
    virtual LLVKPipelineHandle createGraphicsPipeline(const LLVKPipelineDesc& desc) = 0;
    virtual void bindPipeline(LLVKPipelineHandle h) = 0;
    virtual void pushDescriptor(uint32_t set, const LLVKDescriptorWrite* writes, uint32_t count) = 0;
    virtual void pushConstants(VkShaderStageFlags stages, uint32_t offset, uint32_t size, const void* data) = 0;
    virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex) = 0;

    // sync (§5)
    virtual void waitIdle() = 0;  // re-create / screenshot 時
};

// indra/llrender/llvkrenderer_factory.h (本線、C ABI 境界)
extern "C" {
    LLVK_API ILLVKRenderer* LLVKRenderer_create();
    LLVK_API void LLVKRenderer_destroy(ILLVKRenderer*);
}
```

注: 上記は **skeleton level の概念図**、具体 API は r41 実装で確定 → r41.5 charter で詳細化。

#### handle 型は不透明 (opaque) 維持

- `LLVKBufferHandle` / `LLVKTextureHandle` / `LLVKPipelineHandle` は `struct LLVKBufferHandle_T*` opaque pointer
- VK repo 側で `VkBuffer + VmaAllocation + VkBufferView` を保持、本線側からは pointer のみ
- これで VK repo 側内部 struct 変更が本線に伝播しない、ABI 安定

#### 本線側依存 (header) の最小化

- 本線 header は **Vulkan header 直接 include しない** (`vulkan.h` / `volk.h` を本線 header に出さない)
- `VkFormat` 等の Vulkan enum は本線側で `LLVKFormat` 等の独自 enum に shadow (header forward declare で逃げる)、実装で variant 変換
- 例外: `VkShaderStageFlags` 等 bit flag は同値の LLVK 別名 enum で代替

→ 本線 188 file (a-1 §1.2 wrapper 局在化対象) は Vulkan header 非露出、r41.5 で VK repo 切出時に header 配布範囲が最小限で済む。

### §10.3 dynamic link 構成 (本線 LGPL ↔ VK repo 独自 license 境界)

#### r41.5 分離後の link 構成

```
[本線 ayastorm executable, LGPL]
  ├── libayastorm_core.so (本線 common, LGPL)
  ├── libayastorm_llrender.so (本線 render abstraction, LGPL)
  │     └── dynamic link → libayastorm_vk.so (VK repo, 独自 license)
  └── libayastorm_vk.so (VK repo 側ビルド成果物、独自 license)
        ├── volk (MIT)
        ├── VMA (MIT)
        ├── LunarG Vulkan SDK headers (Apache 2.0 系)
        └── Vulkan ICD (system installed、Khronos)
```

#### license 境界の合法性

- 本線 = **LGPL** (Linden Lab viewer license 継承)
- VK repo = **独自 license** (AYAstorm 著作物 license、proprietary or permissive 選択は r41.5 charter で確定)
- **dynamic link 境界** は LGPL の「library との link」ルール内、VK repo を proprietary 化しても LGPL 違反にならない (LGPL の core 主張点)
- Vulkan SDK / VMA / volk は MIT 等 permissive、再配布制約なし
- AYAstorm executable distribution には両 binary 同梱、binary download = LGPL source 提供義務 = 本線 + LGPL part の source を別途公開 (現 AYAstorm release で実施済 flow)

#### dynamic link 実装

- Linux: `dlopen("libayastorm_vk.so")` + `dlsym("LLVKRenderer_create")`
- Win: `LoadLibrary("ayastorm_vk.dll")` + `GetProcAddress`
- Mac: `dlopen("libayastorm_vk.dylib")` 同様

LLVK_API symbol 1 個 (`LLVKRenderer_create` / `LLVKRenderer_destroy`) を `extern "C"` で公開、それ以外は internal symbol (visibility hidden)。

#### r41 段階 (本線同居) の link

- r41 では VK repo は本線 in-tree (`indra/llrender/vk/`)、static link
- ただし interface skeleton は §10.2 の通り C ABI + opaque handle で設計、static→dynamic 移行で本線 code 修正不要にしておく

### §10.4 LL UI 変更時の defensibility

charter §7 判断軸 3 (iv) で言及された「LL 上流が UI 大幅変更で AYAstorm の base UI を侵食した場合の選択肢」を本 §10 skeleton で defensibility 確保:

#### 選択肢有効化条件

| LL 上流 状況 | AYAstorm 側 選択肢 | 本 §10 skeleton が必要な理由 |
|---|---|---|
| LL UI 軽微変更 (現状想定) | upstream merge 継続、本線同居維持 | skeleton 不要 (本線 modify で対応) |
| **LL UI 大幅変更 (例 UE 系 UI 全置換)** | **AYAstorm VK repo を別 viewer base に接続切替** | **本 §10 skeleton が前提**、別 base の渡し先で同 interface 維持 |
| LL viewer 開発停止 | AYAstorm 独自 fork 化 + VK repo は AYAstorm fork の描画 layer として継続 | skeleton 不要 (fork 内で自由) |

#### 「VK repo を別 viewer base に接続」の現実性

- 別 viewer base (例: Alchemy / Catznip / 別 metaverse viewer / 自作 viewer) で AYAstorm 描画 layer を流用する場合、§10.2 interface に準じた wrapper を別 base 側に書けば即接続可能
- 別 base が異なる API surface を持つ場合も、wrapper layer 経由で adaptation 可能 = AYAstorm 投資 (Vulkan 描画 layer) を捨てずに済む
- これが AYAstorm の長期戦略的価値 (charter §7 で言及)

#### r41.5 charter での扱い

- 上記 defensibility は r41.5 charter §X (本 (b) で確定ではなく、r41.5 charter 起草時に専用 section で詳細化)
- 本 §10 では「interface skeleton が defensibility の前提」事実のみ確定、詳細条件は r41.5 charter 預け

### §10.5 interface 詳細は r41.5 charter で確定する旨

#### 本 §10 の確定範囲

本 work item (b) で確定したのは以下:

1. r41 = 本線同居、r41.5 = VK repo 分離 (charter 既定方針の Vulkan 設計反映)
2. interface skeleton = C++ pure virtual + C ABI entry point の 2 層 (§10.2)
3. handle 型 opaque + 本線 header 内 Vulkan header 非露出 (§10.2)
4. dynamic link 境界が LGPL ↔ 独自 license の合法分離 (§10.3)
5. defensibility (別 viewer base 接続切替) の skeleton 前提 (§10.4)

#### r41.5 charter で確定する事項 (本 §10 範囲外)

- API surface の全 method list (本 §10.2 skeleton example は概念図、実装で確定)
- API version 管理 (semver / 互換性保証範囲)
- thread safety guarantee の細部
- 別 viewer base 接続の wrapper 設計 (defensibility 詳細)
- VK repo の license 確定 (proprietary / permissive / dual)
- VK repo の test harness (本線非依存で VK repo 単体 test 可能性)
- VK repo の package 配布形式 (autobuild package / 独自 release / debian package 等)

#### 起草 timing

- r41.5 charter は **r41 達成後** に起草 (本 §10 skeleton の実 use 経験を反映)
- r41 達成までに本 §10 skeleton を本線 implement、API surface の実用妥当性を検証
- r41.5 charter は r41 達成宣言と同時 or 直後に起草開始

---

## draft 進行状況の整理

### 全 10 section 完成 (group B + C 追加で finalize)

- **Vulkan 基盤 (§1)**: Vulkan 1.3 default + volk loader + LunarG SDK 1.3.x の 3 OS 同梱 = 段階 1 (GL header wrapper 置換) の具体 dependency 確定
- **shader chain (§2)**: glslang single-stage、SPIR-V binary を runtime load = build integration の cmake target 設計 input
- **descriptor (§3)**: descriptor set 3 構成 (per-frame / per-material / per-draw) + push descriptor for per-draw = 段階 3 (state machine → PSO 化) の API 表面確定
- **render pass (§4)**: dynamic rendering 採用確定、7 pass chain (shadow / g-buffer+picker / deferred lighting / forward alpha / sky / post-process / UI) 確定、r14+ post-process sub-chain (7 sub-pass) + r21.1 picker gbuffer3 inline 統合 + LLRenderTarget interface 残置移行マップ確定
- **sync (§5)**: frame in flight = 3 確定、fence / binary / timeline 使い分け確定、image layout transition 表確定、sync2 access mask 細分化方針確定、worker thread upload timeline semaphore 同期確定、per-frame barrier sequence 確定
- **memory allocator (§6)**: VMA 全 allocation 採用確定、memory type 5 分類 + VMA usage hint mapping 確定、per-thread staging pool 3 種別確定、defragmentation interactive (16MB/32alloc per frame、r42-α 以降) 確定、`VK_EXT_memory_budget` 経由 VRAM 監視確定
- **swapchain (§7)**: present mode 3 候補 (FIFO / FIFO_RELAXED / MAILBOX) cvar mapping 確定、swapchain image = 3 (frame in flight align) 確定、`_SRGB` swapchain + sRGB color space 確定 (HDR 予約)、re-create sequence 確定、`VK_EXT_swapchain_maintenance1` で `vkDeviceWaitIdle` → per-image fence wait 高速化確定
- **3 OS (§8)**: Linux driver matrix (Mesa RADV/ANV/NVIDIA/AMDGPU-PRO) + Win ICD 仕様 + Mac MoltenVK portable subset 制約 + WSI 4 surface extension + Linux 先行 + Mac maintain 方針の Vulkan 設計反映確定
- **extension (§9)**: KHR 必須 5 + EXT 必須 5 + future 予約 3 = device feature query / `vkCreateInstance` / `vkCreateDevice` の enable list 確定
- **abstraction (§10)**: r41 本線同居 → r41.5 VK repo 分離の skeleton 確定、C++ pure virtual interface + C ABI entry point の 2 層、opaque handle で本線側 Vulkan header 非露出、dynamic link 境界で LGPL ↔ 独自 license 合法分離、defensibility (別 viewer base 接続切替) 前提

### 次 step

1. 本 doc §1-§10 draft の AYA review 反映 (修正指示あれば適用)
2. work item (b) 完了宣言、03 doc §2 status を「完了」に更新
3. work item (c) 工程算定 (`06-effort-estimation.md`) 着手

---

## 関連 doc / memory

### r40 章内部 doc

- `00-charter.md` — r40 章 charter (§4 確定事項 / §5 r41 達成基準 / §6 仮 line up + a-4 補足)
- `03-sub-phase-3-vulkan-plan.md` — work item (b) 親 doc (§4 で本 doc の 10 section 構成定義)
- `04-portage-inventory.md` — work item (a) 全完了 (§6.4 設計 input、§6.3 段階 port 戦略、§1.3 shader 棚卸し)

### memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory (work item (a) 完了で a-4 final 値同期済)
- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 design 完了後に着手)
- `feedback_proactive_handoff.md` — foundation group / 次 session 境界 handoff の根拠
