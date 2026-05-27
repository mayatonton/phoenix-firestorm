# r40 sub-phase 3 work item (b): Vulkan API 設計

**status**: foundation group **§1 + §2 + §3 + §9 draft 完成 (本 session)**、§4-§8 + §10 は次 session で draft
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

**status**: **次 session で draft** (本 session foundation group §1-§3 + §9 まで)

draft 予定の項目:
- §4.1 deferred g-buffer の Vulkan representation (VkRenderPass + VkFramebuffer vs Vulkan 1.3 dynamic rendering)
- §4.2 attachment 配置 (color × N + depth/stencil)、subpass 構成
- §4.3 dynamic rendering 採用判断 (subpass 簡素化 vs subpass dependency tooling 喪失)
- §4.4 r14+ post-process pass chain 統合 (godrays / volumetricLight / blurLight / vignette + r30 DoF)
- §4.5 r21.1 picker attachment 統合 (§3.4 を pass 側から記述)
- §4.6 windlight / atmospherics の sky pass 表現
- §4.7 LLRenderTarget → Vulkan attachment 移行マップ (a-4 §1.1 LLRenderTarget 35 GL calls 対応)

---

## §5 sync 戦略 (state diagram / barrier table)

**status**: 次 session で draft

draft 予定の項目:
- §5.1 frame in flight = 3 採用根拠 (低遅延 vs CPU/GPU 並列度)
- §5.2 fence / binary semaphore / timeline semaphore の使い分け
- §5.3 image layout transition 表 (color attachment / depth / sampled / present)
- §5.4 synchronization2 access mask 細分化方針
- §5.5 worker thread (texture upload / mesh upload) → main thread の async 同期 (timeline semaphore)
- §5.6 swapchain image acquisition → render → present の barrier sequence

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

## 本 session draft 後の整理

### §1-§3 + §9 で確定した設計 input (work item (c) 工程算定 への引継ぎ事項)

- Vulkan 1.3 default + volk loader + LunarG SDK 1.3.x の 3 OS 同梱 = 段階 1 (GL header wrapper 置換) の具体 dependency 確定
- shader cross compile chain は glslang single-stage、SPIR-V binary を runtime load = build integration の cmake target 設計 input
- descriptor set 3 構成 (per-frame / per-material / per-draw) + push descriptor for per-draw = 段階 3 (state machine → PSO 化) の API 表面確定
- 採用 extension 一覧 (KHR 必須 5 + EXT 必須 5 + future 予約 3) = device feature query / `vkCreateInstance` / `vkCreateDevice` の enable list 確定

### §4-§8 + §10 で詰める残り設計 input

- VkRenderPass / dynamic rendering 採用判断 + deferred g-buffer の Vulkan 表現 (§4)
- frame in flight + barrier 戦略 (§5)
- VMA 採用 + memory type 分配 (§6)
- swapchain / present mode (§7)
- 3 OS 詳細 + Mac MoltenVK 制約 (§8)
- r41.5 abstraction interface skeleton (§10)

### 次 session 開始時の最初の task list

1. 本 doc §1-§3 + §9 draft の AYA review 反映 (修正指示あれば適用)
2. §4-§8 + §10 を draft 化
3. 10 section 揃ったら work item (b) 完了宣言、work item (c) 工程算定 (06-effort-estimation.md) 着手

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
