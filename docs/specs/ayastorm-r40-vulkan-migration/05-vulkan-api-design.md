# r40 sub-phase 3 work item (b): Vulkan API 設計

**status**: group A (§4 + §5) draft 追加完成 — **§1-§5 + §9 完成**、§6-§8 + §10 は group B/C で draft
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

### §3.5 pipeline layout 設計

- **descriptor set layout × 3** (set=0/1/2 別々に `VkDescriptorSetLayout` 作成)
- **push constant range**: model matrix を push constant 化 (mat4 = 64 bytes、Vulkan minimum guaranteed 128 bytes 内に収まる)
- **pipeline layout cache**: 同一 layout を共有する PSO を grouping、`vkCreatePipelineLayout` の重複回避

`VkPipelineLayoutCreateInfo` 仮設計:
```
.setLayoutCount = 3
.pSetLayouts = [perFrameLayout, perMaterialLayout, perDrawLayout]
.pushConstantRangeCount = 1
.pPushConstantRanges = [{ VK_SHADER_STAGE_VERTEX_BIT, 0, 64 (mat4 modelMatrix) }]
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

**status**: 次 session で draft

draft 予定の項目:
- §6.1 VMA (Vulkan Memory Allocator, GPUOpen) 採用根拠
- §6.2 memory type 分類 (DEVICE_LOCAL / HOST_VISIBLE / HOST_COHERENT / HOST_CACHED / DEVICE_LOCAL+HOST_VISIBLE)
- §6.3 staging buffer 戦略 (llimagegl.cpp 移行、a-4 §1.1)
- §6.4 vertex buffer / index buffer / UBO / texture の typical allocation pattern
- §6.5 defragmentation 採用判断 (long-lived session での fragmentation 蓄積対策)
- §6.6 budget API (VK_EXT_memory_budget) 経由の VRAM 使用量監視

---

## §7 swapchain / present mode

**status**: 次 session で draft

draft 予定の項目:
- §7.1 present mode 選択 (FIFO default / mailbox optional for VSync OFF / immediate 不採用)
- §7.2 swapchain image 数 (3 image 採用、frame in flight と align)
- §7.3 surface format (sRGB / linear, HDR 検討)
- §7.4 resize / minimize / fullscreen 切替時の re-create 戦略
- §7.5 multi-monitor / DPI scaling
- §7.6 VK_EXT_swapchain_maintenance1 採用検討

---

## §8 3 OS 対応詳細 (Mac MoltenVK 制約含む)

**status**: 次 session で draft (a-4 §6.4.3 の通り Mac 詳細は vk-RC 直前 phase へ後送り、本 §8 は方針のみ)

draft 予定の項目:
- §8.1 Linux: Vulkan native (Mesa RADV / ANV / NVIDIA / AMDGPU-PRO) の driver capability matrix
- §8.2 Windows: NVIDIA / AMD / Intel ICD 仕様の差分
- §8.3 Mac: MoltenVK 経由の portability subset (Vulkan 1.2 core + 一部 1.3 KHR)、§9.4 と整合
- §8.4 WSI: VK_KHR_xcb_surface (Linux X11) / VK_KHR_wayland_surface (Linux Wayland) / VK_KHR_win32_surface (Win) / VK_EXT_metal_surface (Mac)
- §8.5 「Linux 先行 + Mac 互換性 maintain 方針」(§4 (2) Linux 先行 + Mac 後追い の Vulkan 設計反映)

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

**status**: 次 session で draft (a-4 §6.4.3 の通り「分離可能な skeleton」のみ本 (b) で扱う、詳細 interface は r41.5 charter で確定)

draft 予定の項目:
- §10.1 r41.5 milestone の AYAstorm VK repo 分離前提 (charter §6 / §4 (4) Phase 2)
- §10.2 interface skeleton (header のみ公開、本線 ↔ VK repo の API surface)
- §10.3 dynamic link 構成 (本線 LGPL ↔ VK repo 独自 license の合法的境界)
- §10.4 LL UI 変更時の defensibility (charter §7 判断軸 3 (iv) 選択肢の有効化条件)
- §10.5 interface 詳細は r41.5 charter (= 本 doc とは別、r41 達成後に起草) で確定する旨

---

## draft 進行状況の整理

### §1-§5 + §9 で確定した設計 input (work item (c) 工程算定 への引継ぎ事項)

- **Vulkan 基盤 (§1)**: Vulkan 1.3 default + volk loader + LunarG SDK 1.3.x の 3 OS 同梱 = 段階 1 (GL header wrapper 置換) の具体 dependency 確定
- **shader chain (§2)**: glslang single-stage、SPIR-V binary を runtime load = build integration の cmake target 設計 input
- **descriptor (§3)**: descriptor set 3 構成 (per-frame / per-material / per-draw) + push descriptor for per-draw = 段階 3 (state machine → PSO 化) の API 表面確定
- **render pass (§4)**: dynamic rendering 採用確定、7 pass chain (shadow / g-buffer+picker / deferred lighting / forward alpha / sky / post-process / UI) 確定、r14+ post-process sub-chain (7 sub-pass) + r21.1 picker gbuffer3 inline 統合 + LLRenderTarget interface 残置移行マップ確定
- **sync (§5)**: frame in flight = 3 確定、fence / binary / timeline 使い分け確定、image layout transition 表確定、sync2 access mask 細分化方針確定、worker thread upload timeline semaphore 同期確定、per-frame barrier sequence 確定
- **extension (§9)**: KHR 必須 5 + EXT 必須 5 + future 予約 3 = device feature query / `vkCreateInstance` / `vkCreateDevice` の enable list 確定

### §6-§8 + §10 で詰める残り設計 input

- **memory allocator (§6)**: VMA 採用 + memory type 分配 + staging buffer 戦略 + defragmentation 採用判断
- **swapchain (§7)**: present mode / image 数 / surface format / re-create 戦略
- **3 OS (§8)**: Linux / Win driver matrix + Mac MoltenVK portable subset 詳細
- **abstraction (§10)**: r41.5 分離 skeleton (interface skeleton only、詳細は r41.5 charter)

### 次 step

1. 本 doc §1-§5 + §9 draft の AYA review 反映 (修正指示あれば適用)
2. group B (§6 + §7) を draft 化
3. group C (§8 + §10) を draft 化
4. 10 section 揃ったら work item (b) 完了宣言、work item (c) 工程算定 (06-effort-estimation.md) 着手

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
