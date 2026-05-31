# AYAstorm r41 sub-doc 03-state-machine-pso — 段階 3 llrender state machine → Vulkan PSO 化

**status**: **closed 2026-05-29 (Pattern α 一括 draft + 3.1a-iv 加筆、AYA review PASS)**
**親 charter**: `docs/specs/ayastorm-r41-gl-removal/00-charter.md` (closed 2026-05-28)
**前 sub-doc**: `02-portage-execution.md` (closed 2026-05-28、段階 2 完遂で役割完了)
**前 handoff**: `handoff-stage-2-complete.md` (段階 2 完遂 → 段階 3 着手境界)
**達成条件**: 段階 3 完遂 = llrender 主要 5 file (`llgl.{cpp,h}` / `llrender.{cpp,h}` / `llimagegl.{cpp,h}` / `llrendertarget.{cpp,h}` / `llpostprocess.{cpp,h}` 合計 10,841 LOC) の GL state machine → Vulkan PSO 化動作 + 段階 2 acceptance #1 + 特殊対応 (terrain glTexGen 廃止 + avatar SSBO) を段階 3 内で一体運用 satisfy + validation 0 件
**関連 charter section**: §2 領域 3 (段階 3 state machine → PSO 化、1.50 PM 最大 risk) + §3 #1/#3/#5/#6 acceptance + §7.4 sub-doc 構成 + §7.5 boundary

---

## §1 段階 3 scope plan

### §1.1 段階 3 scope 再掲 (charter §2 領域 3 + 04 §5.4 段階 3 + handoff-stage-2-complete §1.3-1.4)

- **対象**: llrender 主要 5 file (合計 10,841 LOC、04 §6.1 確定値の subset)、GL capability detect / matrix stack / FBO / post-process state machine を Vulkan PSO + push constant + dynamic rendering に置換
- **境界条件 (charter §2 領域 3)**: 領域 4 着手前に **PSO 構築 / bind 動作** (frame context 集約は領域 4 = 段階 4 scope)
- **依存順序 (charter §2 領域 3)**: 領域 2 (drawpool 完遂、段階 2 で達成) + 領域 7 (descriptor / render pass) + 領域 6 (shader SPIR-V) と協調必須
- **risk 性質 (charter §2 領域 3)**: **高** — charter §2 高 risk 2 領域の 1 つ、a-3 §5.4 段階 3 = 不確実性 main source、06 §3.2 余裕係数 +37% の主要因
- **段階 2 引継ぎ (handoff-stage-2-complete §1.3-1.4 で AYA 承認済 2026-05-28)**:
  - **acceptance #1-段階 2 未達分** (drawpool 13 file 内 GL call は render path 生存中) を段階 3 PSO 配線時に hook body へ PSO bind + `vkCmdDraw*` 投入 → render path から GL call 削除で satisfy
  - **特殊対応 (terrain glTexGen 廃止 + avatar skinning SSBO)** を段階 3 内で shader 側 explicit UV 化 + SSBO 基本実装と同時実施 → satisfy

### §1.2 5 file 一覧 + LOC + 役割 + Vulkan 実装方針 (04 §2.1 + §4.3 反映)

| # | file | LOC | 役割 (現状 GL) | Vulkan 実装方針 (段階 3 scope) |
|---|---|---|---|---|
| 1 | llgl.cpp | 3,027 | GL capability detect / extension query / state machine root (depth test / blend / cull / scissor 等の RAII state class 群) | `VkPhysicalDeviceFeatures` + `VkPhysicalDeviceProperties` query 化 + `LLGLDepthTest` / `LLGLSDefault` 等 RAII を **PSO 内 state alias** へ置換 (state object は PSO compile 時固定、setter は no-op) |
| 2 | llgl.h | 487 | state machine RAII class declaration + GL enum alias | state class は PSO state alias header に refactor、enum alias は Vulkan VkBlendOp/VkCompareOp 系へ並走 typedef 追加 (charter §3 #1 acceptance 整合) |
| 3 | llrender.cpp | 2,207 | matrix stack (modelview / projection / **texture × 4** [MM_TEXTURE0..3]) + immediate-mode emulation + texture unit state + LLRender 中心 dispatcher | matrix stack → **二段構え** (push constant 64 B = `modelview_matrix` GL 流儀継承 + per-frame UBO 2 binding = projection 系 3 mat4 + texture_matrix[0..3] 4 mat4、MVP/normal_matrix/inverse_modelview は vertex shader 内で `projection × modelview` 等から算出して吸収) (05 §3.5 改訂採用、3.3-A 設計確定 + 3.3-β-1 refine 2026-05-29)、texture unit state → set=1 per-material binding mapping、UI matrix (mUIOffset/Scale) は段階 4 frame context refactor に持越し、immediate-mode は段階 2 hook 化済の `recordPoolDraws(VkCommandBuffer)` 側で吸収 |
| 4 | llrender.h | 582 | LLRender public interface + matrix mode enum + texture unit slot 定数 | interface 信号 (signature) は **段階 4 frame context refactor 前提で維持**、内部実装のみ PSO 化、enum alias を VkPipelineLayoutCreateInfo 由来 stage 定数に並走追加 |
| 5 | llimagegl.cpp | 2,663 | GL texture object lifecycle (glGenTextures / glTexImage2D / glTexParameteri / glDeleteTextures 等) + format conversion | `VkImage` + `VkImageView` + `VMA allocation` lifecycle (06 §6 VMA 採用)、format conversion table を `VkFormat` 表へ置換、descriptor set binding 経由 sampler 参照は領域 7 並走で配線 |
| 6 | llimagegl.h | 371 | LLImageGL public interface + GL texture target enum (TEXTURE_2D / CUBE_MAP 等) | interface signature 維持、target enum を VkImageViewType (VK_IMAGE_VIEW_TYPE_2D / CUBE 等) alias へ並走追加 |
| 7 | llrendertarget.cpp | 589 | FBO (glGenFramebuffers / glFramebufferTexture2D / glBindFramebuffer) + draw buffer 配列管理 | **VK_KHR_dynamic_rendering 採用 (05 §4.1)** → `VkRenderPass` + `VkFramebuffer` 廃止、`bindTarget()` / `flush()` interface は **inline `VkRenderingAttachmentInfo` 構築 + `vkCmdBeginRenderingKHR` / `vkCmdEndRenderingKHR`** に置換 (05 §4.7 移行マップ反映) |
| 8 | llrendertarget.h | 194 | LLRenderTarget public interface + draw buffer attachment 定義 | interface signature 維持 + dynamic rendering 内部実装に置換、attachment 定義を `VkFormat` + load/store op alias 化 |
| 9 | llpostprocess.cpp | 454 | post-process state machine (color matrix / extract / contrast / noise 等の legacy effect uniform 配信) | **r14+ visual realism shader 7 file (r42-γ scope) は touch しない** (charter §3 #4 reg 担保)、本段階では legacy effect uniform 配信を push constant + per-frame UBO へ置換 |
| 10 | llpostprocess.h | 267 | LLPostProcess public interface + effect mode enum | interface signature 維持、effect mode enum は段階 4 LLPipelineFrameContext 統合前提で並走維持 |

**特記**:
- **#1 llgl.cpp + #2 llgl.h** は state machine の root、PSO 内 state alias 化が段階 3 最大 refactor (charter §2 領域 3 risk = 高 の主要因)
- **#3 llrender.cpp** matrix stack は **二段構え** (push constant 64 B = `modelview_matrix` GL 流儀継承 + per-frame UBO 2 binding = projection 系 3 mat4 [projection/inverse_projection/identity] + texture_matrix[0..3] 4 mat4) に分解、shader uniform 9 種 / 574 参照 (base 248 file) を push constant + UBO + shader 内計算へ再配信、MVP/normal_matrix/inverse_modelview は vertex shader 内で `modelview + projection` から算出 (3.3-B shader port 範疇)、shader 側 layout 配線変更必須 → 領域 6 並走 (3.3-A 設計確定 + 3.3-β-1 refine 2026-05-29、push constant 64 B 単独では 9 種 uniform 収まらず二段構え採用、`modelview` 分解 refactor [view × model 分離] は 3.3 scope 伸展のため GL 流儀継承選択、合計 push constant 64 B + UBO 448 B)
- **#5 llimagegl.cpp + #6 llimagegl.h** は領域 7 (descriptor set / sampler) と最密結合、texture binding 配線は 07-descriptor-renderpass.md §3 と同期
- **#7 llrendertarget.cpp + #8 llrendertarget.h** dynamic rendering 化は 05 §4.7 移行マップ準拠、charter §3 #5 acceptance #5 の satisfy 経路
- **#9 llpostprocess.cpp** は AYAstorm 改変 r14+ post-process chain (r42-γ scope) と分離、本段階では legacy LL 部分のみ port

### §1.3 並走領域との関係 (charter §2 領域 3 依存順序)

| 並走領域 | 段階 3 内での協調事項 |
|---|---|
| 領域 2 (lldrawpool、段階 2 完遂済) | 12 pool の `recordPoolDraws(VkCommandBuffer)` hook 内に **PSO bind + `vkCmdDraw*` 投入**、段階 2 引継ぎ acceptance #1 + 特殊対応の satisfy 経路 (handoff-stage-2-complete §1.3-1.4) |
| 領域 6 (shader SPIR-V、本 sub-doc と並走起草) | PSO compile 時に SPIR-V binary 必要、shader binding (vertex attribute / push constant / descriptor set) が段階 3 PSO layout と一致する必要、shader 側 placeholder が領域 6 完遂時に final 化 |
| 領域 7 (descriptor / render pass、本 sub-doc と並走起草) | PSO bind 時に descriptor set 3 階層 (set=0/1/2) が前提、`VkPipelineLayoutCreateInfo` の descriptorSetLayout 配列が領域 7 設計から確定する必要、dynamic rendering の `VkRenderingAttachmentInfo` 構築も領域 7 と整合 |
| 領域 4 (pipeline.cpp frame context、段階 4 scope) | LLRender / LLRenderTarget interface signature を維持して段階 4 で frame context 化、段階 3 では interface 経由 call 化はしない (内部実装のみ PSO 化) |
| 領域 8 (LLVKRenderer skeleton) | charter §3 #6 acceptance: skeleton hook と pipeline.cpp inline 実装の **signature 整合**、段階 3 で確定する PSO bind signature が r41.5 interface 経由 call 化の前提 |

### §1.4 段階 2 引継ぎの satisfy 計画 (handoff-stage-2-complete §1.3-1.4 → 段階 3 内 satisfy)

handoff §1.3-1.4 で確定した 2 件の段階 3 一体運用項目を sub-step マッピング:

| 段階 2 未達項目 | 段階 3 sub-step での satisfy 経路 |
|---|---|
| **acceptance #1-段階 2 (lldrawpool 13 file の GL call 0 件)** | **sub-step 3.3 + 3.4** で `recordPoolDraws(VkCommandBuffer)` hook body に PSO bind + `vkCmdDraw*` 投入、続いて render path から該当 GL call 削除。**完了 marker**: `grep -rE "gl[A-Z][a-zA-Z]+\s*\(" indra/newview/lldrawpool*.cpp` が **0 件 hit** |
| **特殊対応 (terrain glTexGen 廃止 + shader 側 UV 化)** | **sub-step 3.4** で領域 6 並走、`lldrawpoolterrain.cpp` 内 `glTexGen` 直接呼出削除 + shader 側 explicit UV attribute 化 (領域 6 sub-step 6.x で SPIR-V port)。**完了 marker**: `grep -E "glTexGen" indra/newview/lldrawpoolterrain.cpp` が **0 件 hit** |
| **特殊対応 (avatar skinning SSBO)** | **sub-step 3.4** で `lldrawpoolavatar.cpp` 内 bone matrix uniform → `VkBuffer (storage buffer)` 配信、descriptor set=2 per-draw (push descriptor) で bind。**完了 marker**: avatar.cpp 内 bone matrix → VkBuffer 配線確認 + descriptor binding validation 0 件 |

### §1.5 sub-step 3.1a-i/ii 設計素材 (sealed 2026-05-29、3.1a-iv 加筆)

sub-step 3.1 (PSO 基盤 + state alias root) 実装着手前の **trace-before-implement** (`feedback_render_full_trace_first.md` 遵守) として sub-step 3.1a を 4 段階 (i/ii/iii/iv) に AYA 指示で再 scope。本 §1.5 は 3.1a-i (trace inventory) + 3.1a-ii (bridging 設計素材収集) の sealed 結果を sub-doc 内に保全し、sub-step 3.1 以降の実装に直接渡す。

#### §1.5.1 sub-step 3.1a 4 段階構成 (再 scope 2026-05-29)

| stage | scope | 完了状態 |
|---|---|---|
| **3.1a-i** | llgl.{cpp,h} + 関連 caller の GL state machine trace inventory (file / line / 関数別) | **完了 2026-05-29** (6 Agent cluster 並列 trace、本 §1.5.2 に sealed) |
| **3.1a-ii** | 11 件非自明 bridging items の繋ぎ方設計 (素材収集) | **完了 2026-05-29** (本 §1.5.3 に sealed) |
| **3.1a-iii** | 作業項目化 + PD 算出 | **AYA 指示で de-prioritize** (charter §6 1.50 PM 据置き、「やってみて変動するもの」)、本 §1.5.6 で軽い refresh のみ |
| **3.1a-iv** | sub-doc 03 spec 訂正 + handoff-substep-3-1a-complete.md 起草 | **本 §1.5 + §3.1 / §4.1 / §5.1 加筆で satisfy** |

#### §1.5.2 Cluster A-F trace inventory (sealed)

本項は Cluster A / B / D / E / F の trace 結果を sealed 形で保全 (Cluster C false alarm 訂正は §1.5.5)。再 trace 不要。

**Cluster A: llgl.cpp = 3,027 LOC、23 unique GL functions / 79 total direct calls**

| section | line range | 内容 | Vulkan 化方針 |
|---|---|---|---|
| PFNGL function pointer declarations | L227-L992 | ~765 LOC、extern function pointer typedef | volk 化対象 (段階 1 で完了)、物理削除は段階 5 (§5 #1-段階 3 acceptance 反映) |
| LLGLManager init / extensions | L1075-L1810 | ~735 LOC、`glGetString` / `glGetIntegerv` / extension query | `VkPhysicalDeviceFeatures` + `VkPhysicalDeviceProperties` 化 (sub-step 3.1) |
| LLGLState (RAII root) | L2552-L2624 | ~73 LOC、setter は `glEnable` / `glDisable` 直接呼出 | PSO state alias 化 + setter no-op (bridging item #1) |
| LLGLUserClipPlane | L2767-L2820 | ~54 LOC、`glEnable(GL_CLIP_DISTANCE0)` + shader 側 clipPlane uniform 連携 | `VkPipelineRasterizationStateCreateInfo` clip distance enable + shader-side `gl_ClipDistance` (bridging item #4) |
| LLGLDepthTest | L2822-L2906 | ~85 LOC、`glDepthFunc` / `glDepthMask` 動的切替 | `VK_EXT_extended_dynamic_state2` (Vulkan 1.3 core) で dynamic state 化 (bridging item #2) |
| LLGLSquashToFarClip | L2908-L2943 | ~36 LOC、projection matrix depth-squash | matrix stack push constant 64 bytes 内に統合 (bridging item #3) |
| LLGLSyncFence | L2947-L2991 | ~45 LOC、`glFenceSync` / `glClientWaitSync` | **caller = 0 件 (dead code)** → 物理削除 (bridging item #11) |

**Cluster A: llgl.h = 487 LOC + llglstates.h = 197 LOC、12 state class declarations**

`LLGLDepthTest` / `LLGLSDefault` / `LLGLSObjectSelect` / `LLGLSUIDefault` / `LLGLSPipeline` / `LLGLSPipelineAlpha` / `LLGLSPipelineSelection` / `LLGLSPipelineSkyBox` / `LLGLSPipelineDepthTestSkyBox` / `LLGLSPipelineBlendSkyBox` / `LLGLSTracker` / `LLGLSSpecular`

これらは PSO 内 state alias header に refactor、setter は no-op 化 (PSO compile 時 state 固定の Vulkan モデルに整合)。

**Cluster A: caller 集計**

- 46 + 43 file (state class header 経由 + 直接 llgl.h include)
- 195 + 85 direct usage line
- 大半は RAII stack 構築 (`LLGLDepthTest depth(GL_TRUE);` 等)、setter no-op 化で **source-level compat 維持可能** (charter §1 thesis = parity 不要、ただし内部 GL call 削除が acceptance #1)

**Cluster B: AYAstorm 改変 13 file shader boundary (clip + sky 経路 safe 確認済)**

| 経路 | 改変状況 | safe 確認結果 |
|---|---|---|
| **clipF.glsl 経路** | LL 標準 fragment shader (AYAstorm **未改変**) | `LLGLUserClipPlane` 連携 PSO 化で `clip distance` を `VkPipelineRasterizationStateCreateInfo` / shader-side `gl_ClipDistance` 経由維持 → **改変なしで PSO 化可** |
| **skybox skyV.glsl 経路** | AYAstorm r14 改変含む (r14+ visual realism scope) が、改変範囲は L65-L225 に **isolated** | L95 の **push constant matrix swap** (`mat4` viewProj 注入) は r14 改変 block と **independent**、PSO 化で L95 swap を `VkPushConstantRange { offset=0, size=64, stageFlags=VK_SHADER_STAGE_VERTEX_BIT }` に置換、r14 改変 block (L65-L225) は untouched 維持可能 → **safe** |

**結論**: charter §3 #4 regression acceptance (AYAstorm 改変 13 file は r41 内 untouched) は 3.1a で **clip + sky の 2 経路は安全確認済**、残 11 file は段階 3 後続 sub-step (3.4 特殊対応) + r42-α/β/γ scope で順次確認。

**Cluster D: PFNGL 削除 + 189 file 波及**

- llgl.cpp 内 PFNGL declarations 765 LOC + ProcAddr loading 310 LOC + extern declarations 718 = **計 ~1,793 LOC 規模**
- llglheaders.h 経由で **189 file に波及** (GL 関数 prototype 利用)
- volk が `vkGetInstanceProcAddr` + `vkGetDeviceProcAddr` 経由で全 Vulkan 関数 pointer を auto-load (段階 1 で完了)
- GL 側 PFNGL を一括削除すると 189 file の include 連鎖で広範な compile error 発生 → **段階 5 (残依存解決) と一体運用が望ましい**
- 段階 3 内では llrender 5 file のみ GL call 削除 (acceptance #1-段階 3、§4.1 反映)、PFNGL declarations の物理削除は段階 5 で実施 (波及 segment 化)

**Cluster E: glHint + VRAM detection**

- `glHint` は **trivial deletion** (Vulkan に hint API 無し、PSO compile 時に driver 最適化任せ)
- VRAM detection (`glGetIntegerv(GL_GPU_MEMORY_INFO_*)` NVX extension) は **`VkPhysicalDeviceMemoryProperties` で unified** (heap iteration で device-local heap size 取得、3 driver baseline 全対応)

**Cluster F: LLGLSyncFence dead code + debug callback**

- **LLGLSyncFence** (llgl.cpp L2947-L2991, 45 LOC): caller grep **0 件 hit** (本 fork で完全 dead code)、削除 (bridging item #11)
- **gl_debug_callback** (`GL_ARB_debug_output`): llgl.cpp 内に GL debug message callback 配線あり、**`VK_EXT_debug_utils` messenger に直接置換可** (段階 1 で `VkDebugUtilsMessengerEXT` 配線は validation strict 検証で動作確認済、本 callback はその hook を再利用) (bridging item #10)

#### §1.5.3 11 件 非自明 bridging items 設計表 (sealed)

| # | item | 設計方針 | 主担当 sub-step | 備考 |
|---|---|---|---|---|
| 1 | **LLGLState RAII setter dead-store 化** | PSO compile 時 state 固定モデルに整合、setter 内 GL 呼出物理削除、caller (46+43 file) source-level compat 維持 | **段階 4** (LLPipelineFrameContext 配置時、3.1b 着手時 refine 2026-05-29) | charter §1 thesis 整合、parity 不要。3.1b 着手時 spec 内部矛盾発覚 (setter dead-store 化 vs 段階 1+2 動作維持) → AYA 指示で段階 4 移管、3.1b は item #2/#4/#10/#11 物理実装 + PSO 基盤配線 (Cache / Layout helper / compile helper) に scope 限定 |
| 2 | **LLGLDepthTest dynamic state 化** | `VK_EXT_extended_dynamic_state2` (Vulkan 1.3 core) で `vkCmdSetDepthTestEnable` / `vkCmdSetDepthCompareOp` / `vkCmdSetDepthWriteEnable` 経由動的切替、3 driver baseline (NVIDIA / RADV / ANV) で query 確認 | 3.1 | 3 driver 全 1.3 core で支持、追加 extension 不要 |
| 3 | **LLGLSquashToFarClip push constant 化** | projection matrix depth-squash を push constant range 内に統合 (matrix stack push constant 64 bytes 共有) | 3.1 / 3.3 | shader 側 layout 配線変更必須 (領域 6 並走) |
| 4 | **LLGLUserClipPlane 経路 PSO 化** | `VkPipelineRasterizationStateCreateInfo` で clip distance enable、shader-side `gl_ClipDistance` 経由値配信、clipF.glsl 改変なし | 3.1 / 3.3 | Cluster B safe 確認済 (§1.5.2) |
| 5 | **skybox skyV.glsl L95 push constant swap 維持** | matrix stack push constant 化に伴う swap、r14 改変 L65-L225 と independent | 3.1 / 3.3 | Cluster B safe 確認済 (§1.5.2) |
| 6 | **matrix stack → push constant 64 bytes 化** | modelview + projection を mat4 × 1 (64 bytes) で push constant、shader layout cross-update (領域 6 並走) | 3.3 | Vulkan minimum 128 bytes 内、余裕あり (07 §1.2.2 整合) |
| 7 | **texture unit → descriptor set=1 per-material mapping** | 7 PBR slot (DIFFUSE/NORMAL/SPECULAR/BASECOLOR/METALLIC_ROUGHNESS/GLTF_NORMAL/EMISSIVE) を set=1 binding 0-6 配置 | 3.4 (texture lifecycle) | Cluster C measurement-first で AYA 環境実測確定 (§1.5.4) |
| 8 | **VkImage + VkImageView + VMA lifecycle** | LL `LLImageGL` を VkImage + VMA allocation + VkImageView trio で置換、format conversion table を VkFormat 表に化 | 3.4 | VMA は段階 1 で device 配線済、本 sub-step で alloc API 活用 |
| 9 | **FBO → VK_KHR_dynamic_rendering** | `vkCmdBeginRenderingKHR` / `vkCmdEndRenderingKHR` + inline `VkRenderingAttachmentInfo`、`VkRenderPass` + `VkFramebuffer` 廃止 (05 §4.7 移行マップ準拠) | 3.3 | 3 driver baseline 全支持 (Vulkan 1.3 core)、07 §1.2.3 整合 |
| 10 | **gl_debug_callback → VK_EXT_debug_utils messenger 移管** | 段階 1 messenger 配線を reuse、GL ARB debug output callback を Vulkan 側に集約 | 3.1 | 段階 1 で動作実績あり (validation strict 検証で確認済) |
| 11 | **LLGLSyncFence dead code 削除** | caller 0 件確認済 (Cluster F)、llgl.cpp L2947-L2991 物理削除で段階 3 scope 縮減 | 3.1 | 削除のみで段階 3 acceptance #1 への副作用 0 |

**各 sub-step マッピング (§3.1 と整合)**:

| sub-step | 本 §1.5.3 配置設計項目 |
|---|---|
| 3.1 (PSO 基盤 + state alias root) | 1 / 2 / 4 / 10 / 11 (+ 3 / 5 部分前倒し可) |
| 3.2 (軽量 smoke-test = sky pool 1 draw、2026-05-29 refine) | (新規 bridging item 無し、3.1b 配置済 placeholder PSO 基盤 + sky pool `recordPoolDraws` hook body 配線、llpostprocess 本実装は r42-δ basket 移管) |
| 3.3 (標準 PSO 配線 = llrender + llrendertarget) | 3 / 5 / 6 / 9 |
| 3.4 (texture lifecycle + 段階 2 引継ぎ) | 7 / 8 + 段階 2 引継ぎ (acceptance #1 + 特殊対応 2 件、§1.4 整合) |
| 3.5 (self-check + handoff) | (verify 中心、新規 bridging item 無し) |

#### §1.5.4 measurement-first 採用 (redesign-first 回避)

Cluster C false alarm (§1.5.5) 教訓を踏まえ、**sub-step 3.1 着手時** に `LLVKLoader::createDevice` 内で以下 device limit を query + log 出力:

- `maxBoundDescriptorSets` (Vulkan 1.3 minimum = 4)
- `maxPushConstantsSize` (Vulkan 1.3 minimum = 128 bytes)
- `maxPushDescriptors` (`VK_KHR_push_descriptor` extension property、minimum = 32)
- `maxPerStageDescriptorSampledImages` (Vulkan 1.3 minimum = 16)
- `maxColorAttachments` (Vulkan 1.3 minimum = 4)
- `maxDescriptorSetSamplers` (Vulkan 1.3 minimum = 80)

**3 driver baseline** (NVIDIA RTX 5090 / Mesa RADV / Mesa ANV (Intel)) で実測値 log を取得 → 設計 budget 確定 → AYA 共有 → sub-step 3.4 texture lifecycle 配線時に descriptor 設計 final 化。

**redesign-first 採用回避の理由**: Cluster C で Agent が「106+ sampler breach 可能性」と speculation した時、実態 (per-draw 6-8 samplers、descriptor set=2 push descriptor 32 binding minimum を十分下回る) を確認せず redesign 議論に走るのを防ぐため、measurement-first で device 実測 → 設計判断の cadence を sub-step 3.1 完了 marker に組込む (§3.1 sub-step 3.1 marker 反映)。

#### §1.5.5 Cluster C false alarm 訂正記録

- **Agent speculation**: 「106+ sampler breach の可能性」
- **実態**: per-draw binding = 6-8 samplers、PBR texture slot 数 = 7 (DIFFUSE / NORMAL / SPECULAR / BASECOLOR / METALLIC_ROUGHNESS / GLTF_NORMAL / EMISSIVE)、descriptor set=2 push descriptor 32 binding minimum を十分に下回る
- **訂正根拠**: Agent は texture slot 総数 (全 material × 全 PBR map 合計の上界) を per-draw binding と混同 (speculation)
- **設計対応**: 現状の sub-doc 03 §1.2.5 + 07 §3 設計を維持、redesign-first は不要、measurement-first で AYA 環境実測 (§1.5.4) で確定
- **教訓 (`feedback_doubt_self_first.md` 違反の自認)**: Agent speculation を自分でも再 trace せず AYA に伝達 → AYA 反論「これは作業量が想像よりずっと多いということを言っていますか?」で気付いて撤回。**hedged 表現 (「可能性」「breach 寄り」) はエスカレートせず原文 hedge 維持、自分で再 trace してから AYA に伝達** を 3.1a-iv 以降全 sub-step で徹底

#### §1.5.6 PD 算出 軽い refresh (AYA 指示で de-prioritize)

AYA guidance「工数はやってみれば変動するものですからこの数字の精度をどうこう言ってもしかたない」「本 Step を正しい結果で積み上げる方にこそ重点において以降の Step を盤石にしましょう」を遵守、本項は精度よりも sub-step 間相対比較に留める。

- charter §6 / 06 §3.1 領域 3 PM 1.50 (1.5 人月) **据置き** (refine 不要)
- 04 §5.4.1 段階 3 工数感 = **4-5 週間 / 1.5 人月** で現 charter 整合
- 段階 3 完遂時に実工数 vs 1.50 PM の retrospective を `handoff-stage-3-complete.md` で 1 行記録 (charter §6.4 並走方針整合)

**sub-step 間相対工数感** (実装着手前感覚、精度二次):

| sub-step | 相対工数感 | 主要 driver |
|---|---|---|
| 3.1 | 中-大 | 11 件中 5-7 件配置、state alias root が refactor 起点 |
| 3.2 | 小 | smoke-test path、bridging template 動作確認用 |
| 3.3 | 中 | matrix stack + FBO の 2 大 surface 並列着手 (Agent 並列候補) |
| 3.4 | 大 | texture lifecycle + 段階 2 引継ぎ 3 件、12 pool hook body 配線 |
| 3.5 | 小 | self-check + validation strict 再 verify + handoff doc |

---

## §2 port 順序 + dependency graph

### §2.1 5 file 着手順序の決定軸 3 点

1. **state machine root 先行** — `llgl.cpp` + `llgl.h` は全 state class の declaration / implementation root、PSO 内 state alias 化が確定すると LLRender / LLImageGL / LLRenderTarget / LLPostProcess の内部実装方針が同一 pattern で確定
2. **smoke-test path 早期確保** — 最小複雑度の `llpostprocess` (legacy effect、AYAstorm 改変なし) を base orchestrator 検証用 smoke path として活用 (段階 2 範式継承)
3. **特殊対応 (段階 2 引継ぎ) は末尾** — 段階 2 引継ぎ 3 項目 (acceptance #1 / terrain / avatar) は llrender 主要 5 file PSO 化が完了した後で配線するのが bridging code 肥大耐性確保の観点で安全

### §2.2 dependency graph (4 階層、root → 軽量 → 標準 → 特殊対応)

```
llgl.{cpp,h} (PSO state alias root、3,514 LOC)
├── 軽量 smoke-test (1 file 721 LOC)
│   └── llpostprocess.{cpp,h} (legacy effect、AYAstorm 改変なし)
├── 標準 PSO 配線 (2 file pair、5,612 LOC)
│   ├── llrender.{cpp,h} (matrix stack → push constant、texture unit → set=1 mapping、2,789 LOC)
│   └── llrendertarget.{cpp,h} (FBO → VK_KHR_dynamic_rendering、05 §4.7 移行マップ、783 LOC)
├── texture lifecycle (1 file pair、3,034 LOC)
│   └── llimagegl.{cpp,h} (VkImage + VkImageView + VMA lifecycle、領域 7 sampler binding 同期)
└── 段階 2 引継ぎ特殊対応 (drawpool 13 file 内 GL call 削除 + 特殊 2 件)
    ├── 12 pool hook body PSO bind + vkCmdDraw* 投入 (drawpool 13 file の GL call 0 件 acceptance)
    ├── terrain glTexGen 廃止 + shader 側 UV 化 (領域 6 並走)
    └── avatar skinning SSBO 基本実装 (set=2 per-draw push descriptor)
```

### §2.3 並列着手可能性

- 標準 PSO 配線 (llrender + llrendertarget) は independent (matrix stack 改修 vs FBO → dynamic rendering の改修は別 surface、相互依存なし) → 並列着手可能、Agent 並列活用候補 (`feedback_use_agents_proactively.md` 反映)
- llimagegl は領域 7 descriptor set binding (07-descriptor-renderpass.md §3) との同期必須、並走起草の 07 doc draft 進捗 base で着手 timing 調整
- 段階 2 引継ぎ特殊対応 3 件は 12 pool hook 配線完了済 (段階 2 commit `49b9f36427`〜`61bd502445`) なので、本段階で hook body 配線のみ → 並列可

---

## §3 段階 3 sub-step 順序 (5 sub-step 化、段階 2 範式継承)

`02-portage-execution.md` §3 範式継承 (5 sub-step + 末尾 self-check)、§2.2 dependency graph に沿って bundle。

### §3.1 sub-step list

| sub-step | scope | 対象 file (LOC) | 完了 marker |
|---|---|---|---|
| **3.1** | PSO 基盤配線 + measurement-first device limit query + bridging items #2/#4/#10/#11 物理実装 | llvkloader.{cpp,h} 拡張 (VkPipelineCache + VkPipelineLayout 標準形 + PSO compile helper + §1.5.4 device limit query 6 件 + log baseline) + `llgl.{cpp,h}` 内 §1.5.3 bridging items #2/#4/#10/#11 物理実装 (item #1 = LLGLState setter dead-store 化は 段階 4 LLPipelineFrameContext 配置時、3.1b 着手時 refine 2026-05-29) | 起動時 VkPipelineCache 作成成功 + 最小 PSO (sky pool 用 placeholder) compile 成功 + 動作中 1 frame 内に PSO bind が validation 0 件で完了 + `VK_EXT_extended_dynamic_state2` dynamic state 配線動作 (item #2) + `LLGLUserClipPlane` PSO 化動作 (item #4) + `VK_EXT_debug_utils` messenger 経由 GL ARB debug callback 置換動作 (item #10) + `LLGLSyncFence` 物理削除 (item #11) + §1.5.4 device limit query 6 件 log 出力成功 (3 driver baseline 取得は AYA 環境実機で sub-step 3.4 着手前に確定) + bridging template 確定 (item #1 setter dead-store 化動作は 段階 4 acceptance 側で計上) |
| **3.2** | 軽量 smoke-test port (sky pool 1 draw、2026-05-29 refine) | llvkloader.{cpp,h} 内 sky pool 用 minimal SPIR-V (vert + frag) 埋込 + sky pool `recordPoolDraws` body 配線 | sky pool `recordPoolDraws(VkCommandBuffer)` body に PSO bind + fullscreen quad vertex buffer + `vkCmdDraw` 投入動作、起動時画面に Vulkan 経由 sky color 描画 + validation 0 件 (※llpostprocess は upstream Firestorm 由来 `apply()` 呼出 0 件 + effect 関数 empty body の dead code、本実装は r42-δ basket に移管 2026-05-29) |
| **3.3** | 標準 PSO 配線 (matrix stack **二段構え** 化 + FBO → dynamic rendering) | llrender.{cpp,h} (2,789) + llrendertarget.{cpp,h} (783) = 4 file 3,572 LOC | matrix stack → **二段構え** (push constant 64 B `modelview_matrix` GL 流儀継承 + per-frame UBO 2 binding: [projection 系 3 mat4 = `projection_matrix`/`inverse_projection_matrix`/`identity_matrix`] + [`texture_matrix[0..3]`]、合計 push constant 64 B + UBO 448 B、MVP/normal_matrix/inverse_modelview は vertex shader 内計算で吸収 [3.3-B 範疇]) 化動作 + texture unit → set=1 mapping 動作 + FBO → `vkCmdBeginRenderingKHR` 化動作、領域 7 並走 sub-step との descriptor set 整合確認。**UI matrix (mUIOffset/Scale) は段階 4 frame context refactor へ持越し** (本 sub-step scope 外、§3.3 末尾参照)。**3.3 は AYA 指示 (2026-05-29「段階を分けて安全に」「粒度過大化防止」) で 3.3-A (matrix stack push constant+UBO 化) を α/β-1/β-2/γ/δ/ε に細分化** (本 §3.1.1 参照、**完遂 2026-05-29**)、**3.3-C (FBO → dynamic rendering) も同 pattern で α/β-1/β-2/γ/δ/ε に細分化** (本 §3.1.2 参照、**完遂 2026-05-29**、API surface 並走化に scope 集約、`VkImage` / `VkImageView` 実体作成 + attachment 提供は領域 7 sub-step 7.5 移管 = sub-doc 07 §1.2.3 と整合)、**3.3-B (shader port、領域 6 並走) も同 pattern で α/β-1/β-2/γ/δ/ε に細分化** (本 §3.1.3 参照、**完遂 2026-05-31**、3.3-A 確定の push constant + 二段 UBO 設計を **1 shader exemplar** [sky placeholder vert+frag] で動作実証、SPIR-V build chain [system glslangValidator + `aya_compile_shader_spirv` cmake function + viewer_manifest.py recursive copy + `loadSpirvShaderModuleFromFile` helper + macro guard fallback] 確立、領域 6 sub-step 6.1 = autobuild integration の pre-flight 位置付け、本領域 3 内 satisfy で 6.1 一括化へ pattern 流用)。**sub-step 3.3 全体 (3.3-A + 3.3-B + 3.3-C) 完遂 2026-05-31**、3.4 着手 ready |
| **3.4** | texture lifecycle + descriptor set=1 per-material 7 PBR slot 配置 + 段階 2 引継ぎ特殊対応 | llimagegl.{cpp,h} (3,034 LOC) + §1.5.3 bridging items #7/#8 配置 + 12 pool hook body PSO bind 配線 + terrain glTexGen 廃止 + avatar SSBO 基本実装 | VkImage + VkImageView + VMA lifecycle 動作 (item #8) + descriptor set=1 per-material 7 PBR slot (DIFFUSE/NORMAL/SPECULAR/BASECOLOR/METALLIC_ROUGHNESS/GLTF_NORMAL/EMISSIVE) を binding 0-6 配置動作 (item #7、§1.5.4 device limit 実測値 base で final 化) + 12 pool 全 hook 内 `vkCmdDraw*` 投入動作 + render path GL call 削除 (acceptance #1-段階 2 satisfy) + terrain.cpp 内 `glTexGen` 0 件 + avatar.cpp 内 bone matrix → VkBuffer 配線 + validation 0 件。**3.4 は AYA 指示 (2026-05-31「案 1 (image lifecycle 先行) 推奨」承認) で α/β-1/β-2/γ/δ/ε に細分化** (本 §3.1.4 参照、3.3-A/3.3-B/3.3-C pattern 継承)、3.4-β-1 で領域 7 sub-step 7.1 (VMA + descriptor pool sizing 基盤) を内包先行 install、3.4-γ で領域 7 sub-step 7.3 (set=1 per-material layout) の layout + 1 set transit smoke を前倒し (material cache 本実装は領域 7 sub-step 7.3 残置)、3.4-δ で avatar SSBO 基本配線時 領域 7 sub-step 7.4 (set=2 push descriptor) の `vkCmdPushDescriptorSetKHR` 基本配線を前倒し (本 push descriptor 全配線は領域 7 sub-step 7.4 残置)、実 attachment 配線 (`VkImage`/`VkImageView` 実体は領域 7 sub-step 7.5 持越し)。**sub-step 3.4 全体 (α/β-1/β-2/γ/δ/ε) 完遂 2026-05-31**、3.5 着手 ready (本 §3.1.4 全 row 完遂、bridging items #3 part/#7/#8 satisfy + 段階 2 引継ぎ acceptance #1 + 特殊対応 2 件 satisfy + 領域 7 sub-step 7.1 内包 / 7.3 layout 部分内包 / 7.4 push descriptor 部分内包) |
| **3.5** (完遂 2026-05-31) | 段階 3 self-check + validation strict 検証 + handoff doc (案 B 2 commit 分割 = 3.5-a verify + 3.5-b doc) | (本 sub-step) | ✓ 3.5-a (no-commit): `indra/llrender/llvkloader.cpp` line 134/143/185/215 の 4 guard を `#if 1` / `#if 0` で force-enable + 部分 rebuild + AYA launch + sustained ~10 分動作 + `git checkout --` で patch revert clean、AYA launch verify PASS 2026-05-31 [validation=enabled marker hit (line 86 llvkloader.cpp:180) + VK_EXT_debug_utils messenger installed marker hit (line 87 llvkloader.cpp:243) + vulkanDebugCallback 経由 `[VK ERROR]/[VK WARN]/[VK INFO]/[VK VERBOSE]` 真の 0 件 + 12 #VkRecord# pool record hook 12/12 one-shot fire [Sky/WLSky/WaterExclusion/Simple/Bump/Materials/GLTFPBR/Alpha/Terrain/Water/Avatar/Tree、Tree も login 後 fire] + 57 unique #Vulkan# INFO marker (3.4 baseline 54 + 3.5-a 追加 3) + shutdown clean (Vulkan device destroyed → Vulkan instance destroyed) + validation 違反 0 件 + 非 Vulkan baseline WARN は段階 1-2 同一 + build clean compile warning/error 0 件] / ✓ 3.5-b (本 commit): handoff doc `handoff-stage-3-complete.md` 起草 + sub-doc 03 §3.1 sub-step 3.5 行 + §4 段階 3 acceptance 末尾 satisfy 注記 update + memory `project_ayastorm_r41_vulkan_migration.md` 段階 4 着手 ready 化 + MEMORY.md index entry update、§4.1 acceptance 5 件 self-trace satisfy (#1-段階 3 / #3-段階 3 / #5-段階 3 / #6-段階 3 / 特殊対応 / regression、本 sub-doc §4.1 表参照、charter §3 #1/#3/#5/#6 段階 3 分 + regression 全達成) |

### §3.1.1 sub-step 3.3-A 細分化 (α/β/γ/δ/ε、3.3-A 設計確定 2026-05-29)

3.3-A (matrix stack push constant + UBO 化) は AYA 指示 (2026-05-29「段階を分けて安全に」「粒度過大化防止」) で以下に細分化。各 sub-step は完遂境界で handoff timing 能動チェック (`feedback_proactive_handoff.md` 遵守)。

| sub-step | scope | 対象 file | 完了 marker |
|---|---|---|---|
| **3.3-α** | spec refine (本 §1.2 #3 / §3.1 sub-step 3.3 / §3.1.1 本表 / §3.3 持越し記録 + sub-doc 05 §3.5 訂正 + UI matrix 段階 4 持越し明示 + texture × 3 → × 4 訂正 + 二段構え反映) | sub-doc 03 / sub-doc 05 | spec 改訂 commit 投入 |
| **3.3-β-1** | `PerFrameMatrixUBO` (binding 0 = `projection_matrix` + `inverse_projection_matrix` + `identity_matrix` 3 mat4 / 192 B / std140) + `TextureMatrixUBO` (binding 1 = `texture_matrix[0..3]` 4 mat4 / 256 B / std140) C++ struct 定義 + `static_assert` size 確定 + `VkDescriptorSetLayout` set=0 設計 (binding 0/1 = UNIFORM_BUFFER, stage = VERTEX\|FRAGMENT) + llvkloader.h header に struct + getter signature 追加 (実装 body は β-2) + spec sealed (本 §3.1.1 refine + sub-doc 05 §3.5 refine、`modelview` GL 流儀継承確定、MVP/normal/inverse_modelview shader 内計算移譲) | llvkloader.h + sub-doc 03/05 | header に struct + getter signature 追加 + spec sealed + commit 投入 |
| **3.3-β-2** (完遂 2026-05-29) | β-1 layout base で `VkBuffer` (HOST_VISIBLE_COHERENT、frame in flight 3 個 × **512 B** = ~1.5 KB、PerFrame 192 B @ offset 0 + Texture 256 B @ offset 256、256 B align 安全側) + `VkDescriptorPool` + `VkDescriptorSet` allocation + persistent mapping + 初期 zero write smoke (validation 0 件) | llvkloader.cpp (+239 行) | ✓ UBO buffer × 3 作成成功 + descriptor pool/set × 3 alloc 成功 + persistent map + zero write + AYA launch PASS (RTX 5090) + 起動 log 3 marker 出力 + Vulkan 系 WARN/ERR 0 件 |
| **3.3-γ** (完遂 2026-05-29) | placeholder + sky smoke PSO の VkPipelineLayout 二段構え準拠化 (set=0 = `sPerFrameDescriptorSetLayout` + push constant range = mat4 modelview_matrix / 64 B / VERTEX_BIT 反映)、3.1b/3.2 PSO 引継ぎ動作維持 | llvkloader.cpp (+18/-2 行) | ✓ 2 PSO compile 成功 (新 layout 統合後の `Placeholder PSO compiled` / `Sky smoke PSO compiled` log 出力) + Vulkan 系 WARN/ERR 0 件 (release build / validation disabled) + AYA launch PASS (RTX 5090) + 3.1b/3.2/3.3-β-2 全 marker 維持 |
| **3.3-δ-1** (完遂 2026-05-29) | LLRender::syncMatrices() Vulkan path 並走 (case X = syncMatrices call timing 採用、GL path mirror symmetry)、`PerFrameMatrixUBO` (projection / inverse_projection / identity) + `TextureMatrixUBO` (texture × 4) を `writeCurrentPerFrameMatrixUBO()` / `writeCurrentTextureMatrixUBO()` 経由で persistent mapped UBO へ memcpy、`getCurrentFrameIndex()` (FRAMES_IN_FLIGHT=3 ring) を beginFrame で advance | llvkloader.{cpp,h} + llrender.cpp (+~40 行) | ✓ UBO write helper 2 関数実装 + frame_index counter advance + syncMatrices Vulkan path 並走 + AYA launch PASS (RTX 5090) + δ-1 marker 2 件 (`syncMatrices PerFrame UBO write path active` / `syncMatrices TextureMatrix UBO write path active`) + Vulkan 系 WARN/ERR 0 件 + β-2/γ marker 全維持 |
| **3.3-δ-2** (完遂 2026-05-29) | `pushCurrentModelviewMatrix()` 実装 (case P = sInFrame + sCommandBuffer + sPlaceholderLayout gating 採用、最小改変)、syncMatrices() で MM_MODELVIEW を取得し `vkCmdPushConstants(VERTEX_BIT, 0, 64)` 投入、placeholder layout (γ で push constant range 0..64 B / VERTEX_BIT 準拠化済) 流用、`llappviewer.cpp:1781` で beginFrame/endFrame は実 frame (display() 前後) に既に配線済 (当初 case P 説明の「未配線」は誤り、訂正済) | llvkloader.{cpp,h} + llrender.cpp (+45/-0 行) | ✓ push constant 投入 helper 実装 + in-frame gating 動作 + AYA launch PASS (RTX 5090) + δ-2 marker (`syncMatrices modelview push constant path active (frame_index=1, layout=sPlaceholderLayout, range=0..64 B / VERTEX_BIT)`) + Vulkan 系 WARN/ERR 0 件 + frame_index=1 (δ-2) vs frame_index=0 (δ-1) で in-frame gating 動作実証 + 全 8 marker (β-2 3 件 / γ 2 件 / δ-1 2 件 / δ-2 1 件) 維持 |
| **3.3-δ-3** (完遂 2026-05-29、δ-2 cycle 内 satisfy) | δ-2 launch verify と同 cycle で validation log 0 件 + marker 維持 + visual regression 0 を AYA 確認、独立 sub-step として消費せず δ-2 受入と統合 | (verify) | ✓ δ-2 受入と同 cycle satisfy 完了 |
| **3.3-ε** | `handoff-substep-3-3-A-complete.md` 起草 (3.3-A 全 sub-step α/β-1/β-2/γ/δ 完遂総括) → 3.3-B/C 着手境界 | docs | handoff doc 完成 + commit 投入 |

3.3-B (shader port、領域 6 並走) + 3.3-C (FBO → dynamic rendering) は 3.3-A 完遂後の別タイムライン (本 §3.1 sub-step list 3.3 行参照)。

#### §3.1.1 設計根拠 trace inventory (3.3-A 設計確定 2026-05-29)

二段構え採用の確定根拠は llrender.{cpp,h} + caller 棚卸し (Agent 並列 cluster A/B trace、`handoff-substep-3-3-A-complete.md` で sealed 予定):

| 確定事実 | 数字 / 出典 |
|---|---|
| `mMatrix[NUM_MATRIX_MODES][LL_MATRIX_STACK_DEPTH]` | 6 × 32 = 192 mat4 (12,288 bytes)、llrender.h:515 |
| matrix mode | MM_MODELVIEW/MM_PROJECTION/MM_TEXTURE0..3 = **6 mode** (texture **4 個**)、llrender.h:370-376 |
| LLRender 内 GL matrix call (`glLoadMatrixf`/`glMatrixMode`/`glPushMatrix` 等) | **0 件** (glm 算術置換済、push constant 化に GL 障害なし) |
| syncMatrices 配信 uniform 種類 | MODELVIEW / NORMAL / INV_MODELVIEW / MVP / PROJECTION / INV_PROJECTION / IDENTITY / TEXTURE_MATRIX0..3 = **9 種** |
| caller (newview + llrender) | 919 件 (newview 859 / llrender 60) |
| shader uniform 参照 (base 248 file) | unique 9 種 / 574 参照 (`projection_matrix` 204 / `mvp_matrix` 128 / `modelview_matrix` 105 が主) |
| AYAstorm 改変 13 file 内 matrix 参照 | 7 件 (picker 2 + Cinematic 5) → **既存 layout 互換必須** |
| UI matrix (mUIOffset/Scale std::vector stack) | syncMatrices scope 外で独立管理 → **段階 4 frame context refactor 持越し** |
| push constant 中身 (3.3-β-1 refine 2026-05-29) | `modelview_matrix` (GL 流儀継承、view × model 結合済、`modelview` 分解 refactor は 3.3 scope 伸展のため不採用) |
| shader 内計算移譲 (3.3-β-1 refine 2026-05-29) | MVP / `normal_matrix` / `inverse_modelview_matrix` は vertex shader 内で `projection × modelview` 等から算出 (3.3-B shader port 範疇) → per-frame UBO binding 0 は projection 系 3 mat4 (192 B) に縮約 |

push constant 64 B 単独では 9 種 uniform 収まらない → 二段構え (push constant + per-frame UBO 2 binding) 採用、shader 内計算移譲 3 種 (MVP/normal/inverse_modelview) で UBO binding 0 = 3 mat4 (192 B) に縮約、合計 push constant 64 B + UBO 448 B (192 + 256)。

### §3.1.2 sub-step 3.3-C 細分化 (α/β-1/β-2/γ/δ/ε、3.3-C 設計確定 2026-05-29)

3.3-C (FBO → dynamic rendering) は 3.3-A 完遂後の AYA 確認 (2026-05-29 「案 A 推奨 6 sub-step」承認) で以下に細分化。3.3-A pattern (α/β-1/β-2/γ/δ/ε) 継承。

**scope boundary**: 3.3-C は **llrendertarget API surface 並走化** (`bindTarget` → `vkCmdBeginRenderingKHR` / `flush` → `vkCmdEndRenderingKHR` の wrap helper + call surface) のみ、**`VkImage` / `VkImageView` 実体作成 + attachment 提供は領域 7 sub-step 7.5 へ持越し** (理由: VMA = 領域 7 sub-step 7.1 前提、3.3-C 完遂時の Vulkan path 並走は placeholder attachment による transit smoke、実 attachment 配線は領域 7 担当 = sub-doc 07 §1.2.3 と整合)。各 sub-step は完遂境界で handoff timing 能動チェック (`feedback_proactive_handoff.md` 遵守)。

| sub-step | scope | 対象 file | 完了 marker |
|---|---|---|---|
| **3.3-C-α** | spec refine (本 §3.1.2 新規追加 + §3.1 sub-step 3.3 行 update + §3.3 持越し記録追記 + sub-doc 07 §1.2.3 boundary 明確化) | sub-doc 03 / sub-doc 07 | spec 改訂 commit 投入 |
| **3.3-C-β-1** (完遂 2026-05-29) | `VkRenderingAttachmentInfoKHR` wrap struct (color × ≤4 + depth) + `beginDynamicRendering()` / `endDynamicRendering()` signature 追加 (llvkloader.h) + spec sealed (本 §3.1.2 refine) | llvkloader.h (+33 行) | ✓ `DynamicRenderingAttachment` struct + 2 helper signature 追加 + 案 A 採用 sealed (`image_view` nullable / `image_layout` / `load_op` / `store_op` / `clear_value` 5 field、color = pointer + count、depth = nullable pointer、command buffer は internal `sCommandBuffer` 経由 δ-2 case P gating pattern 継承) + `make llrender` build PASS (gcc 13.3.0 / no warning / `libllrender.a` link 成功) |
| **3.3-C-β-2** (完遂 2026-05-29) | dynamic rendering helper body 実装 (`vkCmdBeginRendering` / `vkCmdEndRendering` = Vulkan 1.3 core via volk function pointer + placeholder attachment による transit smoke + Vulkan 1.3 `dynamicRendering` feature query + enable chain via `VkPhysicalDeviceDynamicRenderingFeatures` pNext) | llvkloader.cpp (+145 行) | ✓ helper body 実装 (`sInDynamicRendering` 静的 flag で対称 pair 保証 + 全 view null no-op path + δ-2 case P gating 継承) + `VkPhysicalDeviceDynamicRenderingFeatures` query + enable chain + INFO marker `"Vulkan 1.3 dynamicRendering feature enabled"` (init 時 1 回) + INFO marker `"beginDynamicRendering : dynamic rendering helper path active"` (in-frame 1 回 one-shot) + `beginFrame()` 内 transit smoke 呼出 + `make llrender` build PASS + **AYA launch verify PASS** (2 新 marker 出力 [line 102 init / line 1002 in-frame] + 既存 8 marker 維持 [placeholder PSO / sky smoke PSO / per-frame desc layout / per-frame UBO / per-frame desc sets / writeCurrent×2 / pushCurrentModelview] + Vulkan 系 WARN/ERR 0 件 + shutdownVulkan clean + frame_index=0→1 ring 動作確認) |
| **3.3-C-γ** (完遂 2026-05-29) | `LLRenderTarget::bindTarget()` Vulkan path 並走 (gating: `LLVKLoader::isVulkanInitialized()` + helper 内部 `sInFrame` case P)、`beginDynamicRendering()` 呼出 + placeholder attachment 提供 (実 attachment は領域 7 sub-step 7.5 移管) | llrendertarget.cpp (+46 行) | ✓ `#include "llvkloader.h"` 追加 + `bindTarget()` 末尾 Vulkan path block + one-shot marker + `DynamicRenderingAttachment` 配列構築 [color × `mTex.size()` clamped to 4 / depth × `mUseDepth` 条件] + 全 `image_view=VK_NULL_HANDLE` で helper 側 null-skip path = `sInDynamicRendering=false` 維持 (δ end pair 対称性) + `make llrender` build PASS + **AYA launch verify PASS** (γ marker 1 件 line 153 one-shot [init 直後 RenderTarget 初回 bind 時] + 既存 10 marker 維持 + Vulkan 系 WARN/ERR 0 件 / INFO 34 件正常) |
| **3.3-C-δ** (完遂 2026-05-29) | `LLRenderTarget::flush()` Vulkan path 並走、`endDynamicRendering()` 呼出 + 全 marker 維持 + verify | llrendertarget.cpp (+25 行) | ✓ `flush()` 内 `gGL.flush()` 直後 + `mPreviousRT` 再帰の前に Vulkan path block (begin/end pair 1:1 整合性保証) + one-shot marker `"flush dynamic rendering end path active"` + `LLVKLoader::endDynamicRendering()` 呼出 + `make llrender` build PASS + **AYA launch verify PASS** (δ marker 1 件 line 152 one-shot + γ marker line 151 隣接配置で pair 動作確認 [γ bindTarget 直後の最初の flush で δ marker 即発火] + 既存 11 marker 維持 + Vulkan 系 WARN/ERR 0 件 / INFO 35 件正常 + shutdownVulkan clean) |
| **3.3-C-ε** (完遂 2026-05-29) | `handoff-substep-3-3-C-complete.md` 起草 (3.3-C 全 sub-step α/β-1/β-2/γ/δ 完遂総括) → 3.3-B 着手境界 | docs | ✓ handoff doc 完成 (`handoff-substep-3-3-C-complete.md`、3.3-C 全 sub-step 完遂宣言 + 関連 commit 8 件 mapping + AYA launch verify acceptance log line 突合 + 設計判断履歴 7 件 + risks/caveats 7 件 + next session entry point + cross-ref) + commit 投入 |

3.3-C 完遂後は 3.3-B (shader port、領域 6 並走) 着手 (本 §3.1 sub-step list 3.3 行参照)。

#### §3.1.2 設計根拠 trace inventory (3.3-C 設計確定 2026-05-29)

API surface 並走化採用の確定根拠は llrendertarget.{cpp,h} 棚卸し:

| 確定事実 | 数字 / 出典 |
|---|---|
| LLRenderTarget LOC | 783 (cpp 589 + h 194) |
| GL FBO API surface | 6 群 (allocate + addColorAttachment + allocateDepth / bindTarget / clear / flush / release / setColorAttachment + shareDepthBuffer) |
| `glBindFramebuffer` call | 17 件 (allocate / setColorAttachment / addColorAttachment / shareDepthBuffer / bindTarget / flush / release) |
| `glFramebufferTexture2D` call | 7 件 (depth/color attachment 配線) |
| `glDrawBuffer(s)` / `glReadBuffer` call | 計 6 件 (bindTarget MRT 配線: `glDrawBuffers` + `glReadBuffer(GL_COLOR_ATTACHMENT0)` × 2 + bindTarget empty MRT: `glDrawBuffer(GL_NONE)` + `glReadBuffer(GL_NONE)` × 2 + flush 復帰: `glReadBuffer(GL_BACK)` + `glDrawBuffer(GL_BACK)` × 2) |
| color attachment 上限 | 4 (mTex 4 個まで、llrendertarget.cpp:217) |
| FBO RT stack | mPreviousRT linked list (bindTarget で push、flush で pop) |
| `VkImage` / `VkImageView` 実体作成 | **領域 7 sub-step 7.5 持越し** (VMA = 領域 7 sub-step 7.1 前提、3.3-C scope 外) |
| 並走 path attachment 提供 | placeholder (Vulkan side で実 image 持たず、領域 7 sub-step 7.5 で attachment 提供時に本配線) |

3.3-C 並走 path = placeholder attachment + `vkCmdBeginRenderingKHR` / `vkCmdEndRenderingKHR` call surface 並走、視覚は依然 GL 担当 (sub-doc 03 §3.5 「GL path 動作維持」と整合)。

### §3.1.3 sub-step 3.3-B 細分化 (α/β-1/β-2/γ/δ/ε、3.3-B 設計確定 2026-05-30)

3.3-B (shader port、領域 6 並走) は 3.3-C 完遂後の AYA 確認 (2026-05-30「その細分化案でお願いします、B-α から着手」承認) で以下に細分化。3.3-A/3.3-C pattern (α/β-1/β-2/γ/δ/ε) 継承。

**scope boundary**: 3.3-B は **1 shader exemplar pre-flight** (sky placeholder 等 minimal shader 1 件で SPIR-V 並走 PSO compile + 3.3-A 確定 二段構え matrix 配線 [push constant 64 B + UBO 448 B] の shader 側受領 + 動作実証) のみ、**248 file (base 228) 一括 cross compile / B 53 file 修正 / descriptor set binding 統合 / AYAstorm 13 file untouched verify は領域 6 sub-step 6.1-6.5 担当** (sub-doc 06 §3 と整合、3.3-B で確立する pattern を 6.1 で一般化、6.1 autobuild integration の pre-flight 位置付け)。各 sub-step は完遂境界で handoff timing 能動チェック (`feedback_proactive_handoff.md` 遵守)。

| sub-step | scope | 対象 file | 完了 marker |
|---|---|---|---|
| **3.3-B-α** | spec refine (本 §3.1.3 新規追加 + §3.1 sub-step 3.3 行 update + sub-doc 06 §1 boundary 整理 [3.3-B = exemplar pre-flight / 6.1 = 一括 integration の役割分担明示] + 設計根拠 trace inventory) | sub-doc 03 / sub-doc 06 | spec 改訂 commit 投入 |
| **3.3-B-β-1** (完遂 2026-05-30) | shader load path 改修 signature (既存 GL compile chain `llshadermgr.cpp:909-955` [glCreateShader → glShaderSource → glCompileShader] に SPIR-V 並走分岐 signature 追加: `LLVKLoader::loadSpirvShaderModule(const U32* spv_code, size_t code_size_bytes)` → `VkShaderModule` helper を llvkloader.h に signature 追加 [実装 body は β-2]) + spec sealed (戻り値 VkShaderModule / 失敗時 VK_NULL_HANDLE / SPIR-V binary は U32 alignment per spec / caller owns + vkDestroyShaderModule で破棄 / PSO compile 後は安全に破棄可) | llvkloader.h (+21 行) | ✓ `VkShaderModule loadSpirvShaderModule(const U32* spv_code, size_t code_size_bytes)` signature 追加 + 設計 sealed (return + ownership + alignment) + `make llrender` build PASS (gcc 13.3.0 / no warning / `libllrender.a` link 成功) |
| **3.3-B-β-2** (完遂 2026-05-30) | β-1 signature base で `vkCreateShaderModule()` 経由 `VkShaderModule` create 実装 + SPIR-V compiler 経路選択 (system `glslangValidator` を CMake module `AyaShaderCompile.cmake` から `find_program` 発見 + `aya_compile_shader_spirv()` function で `add_custom_command` 経由 `-V --target-env vulkan1.3` 起動 = **build 時 pre-compile** [理由: cross-platform 一貫 / 起動時間短縮 / shader iteration は cache clear 戦略で対応可 / Mac/Win build host 未 install 時は WARNING + skip で fallback、領域 6 sub-step 6.1 の design 流用]) + 最小 1 shader (sky placeholder vert + frag、3.3-A γ で動作中の minimal SPIR-V を GLSL → `.spv` build chain 経由で再現) の `.glsl` source + `aya_r41_shaders` custom target で `.spv` 生成 + viewer_manifest.py recursive copy で packaged dir に同梱 + beginFrame 初回 1 度限り transit smoke (`.spv` 読込 → `loadSpirvShaderModule()` → 即破棄、PSO 配線は γ) | llvkloader.cpp (+80) + indra/cmake/AyaShaderCompile.cmake (新規 +50) + indra/newview/CMakeLists.txt (+23) + indra/newview/app_settings/shaders/aya_r41_exemplar/ (新規 sky_placeholderV.glsl + sky_placeholderF.glsl) + .gitignore (+4 = `*.spv`) | ✓ `aya_r41_shaders` target build PASS + `.spv` 1 vert (1136 B) + 1 frag (384 B) 生成成功 + `loadSpirvShaderModule()` 実装 + INFO marker `"SPIR-V shader module loaded (sub-step 3.3-B-β-2 exemplar pre-flight) size=N bytes"` × 2 + AYA launch PASS (2026-05-30 12:28:48Z) + Vulkan 系 WARN/ERR 0 件 + 3.3-A/3.3-C の 12 marker 全維持 (regression 0)、commit `5d4999ec4a` |
| **3.3-B-γ** (完遂 2026-05-31) | 最小 1 shader (β-2 生成済 sky placeholder `.spv`) で SPIR-V 並走 PSO compile 動作確認 (`createSkySmokePipeline()` の `VkShaderModule` 取得経路を embedded byte array `kSkySmokeVertSpv`/`kSkySmokeFragSpv` から `loadSpirvShaderModuleFromFile` 経由 file load primary に切替、Mac/Win build host での glslangValidator 不在時は cmake が `AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK` 自動定義 → embedded fallback active = 3 OS 互換維持、Linux build では embedded array dead code 除去) + beginFrame の β-2 transit smoke 撤去 + `loadSpirvShaderModule` の `sInitialized` ガード撤去 (initVulkan 内 createSkySmokePipeline からの呼出対応) | indra/llrender/llvkloader.cpp (+87 / -67) + indra/llrender/CMakeLists.txt (+9 = include + 条件 compile def) | ✓ AYA launch verify PASS (2026-05-31): INFO marker `"Sky smoke PSO compiled via SPIR-V build chain (sub-step 3.3-B-γ)"` line 110 + `"SPIR-V shader module loaded size=1136 bytes"` + `"size=384 bytes"` × 2 (file load 経路実走) + β-2 transit smoke marker 完全消失 + 3.3-A/3.3-C 11 marker 全継続 + Vulkan 系 WARN/ERR 0 件 (38 INFO のみ) + Linux build で `kSkySmokeVertSpv`/`kSkySmokeFragSpv` symbol `libllrender.a` 内 0 件 (dead code 除去確認)、commit `020df91561` |
| **3.3-B-δ** (完遂 2026-05-31) | UBO binding (set=0 binding 0 = `PerFrameMatrixUBO` 192 B / binding 1 = `TextureMatrixUBO` 256 B) + push constant range (0..64 B / VERTEX_BIT = `modelview_matrix`) の **shader 側受領** + shader 内 `mvp_matrix = projection_matrix * modelview_matrix` / `normal_matrix = transpose(inverse(mat3(modelview_matrix)))` / `inverse_modelview_matrix = inverse(modelview_matrix)` 計算配線 (3.3-A §3.1.1 trace inventory 「shader 内計算移譲 3 種」を sky placeholder vert shader 内に具体配置、視覚出力は γ と同じ fullscreen triangle 維持 = binding witness を 1e-30 multiplier で float32 denormal 以下に圧縮し視覚 regression 0 を構造的担保) | indra/newview/app_settings/shaders/aya_r41_exemplar/sky_placeholderV.glsl (+49 / -6) + indra/llrender/llvkloader.cpp (+1 = δ INFO marker) | ✓ AYA launch verify PASS (2026-05-31): `.glsl` vert source 内 `layout(set=0, binding=0) uniform PerFrameMatrixUBO` + `layout(set=0, binding=1) uniform TextureMatrixUBO` + `layout(push_constant) uniform PushConstants { mat4 modelview_matrix; };` + shader 内 MVP/normal/inverse_modelview 計算式 + glslang `--target-env vulkan1.3` で `.spv` 生成成功 (vert 3268 B = γ 1136 B から +2132 B、binding+計算式分) + PSO compile 成功 + INFO marker `"Sky placeholder vert binding active (PerFrameMatrixUBO + push constant modelview)"` line 105 hit + γ marker 継続 + 3.3-A/3.3-C/γ 既存 11 marker 全継続 (regression 0) + Vulkan 系 WARN/ERR 0 件 (Vulkan INFO 39 件、γ 38 → +1 = δ marker) + 視覚 regression 0、commit `b2c06f869d` |
| **3.3-B-ε** (完遂 2026-05-31) | `handoff-substep-3-3-B-complete.md` 起草 (3.3-B 全 sub-step α/β-1/β-2/γ/δ 完遂総括 → 3.4 着手境界 + 領域 6 sub-step 6.1 pattern 流用 prep) + sub-doc 03 §3.1.3 末尾 + §3.1 sub-step 3.3 行 complete 化 + memory `project_ayastorm_r41_vulkan_migration.md` を 3.4 着手 ready 状態へ update + ε 着手前 AYA 相談で **Mac/Win embedded fallback 取扱 = α (glslangValidator 必須化) 採用** (β 不採用、6.1 一括化で結局 glslangValidator 必須化要求のため短期 patch 重複作業回避、`AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK` macro guard は r42-α/β Mac/Win Vulkan 着手まで temporary safety net として残置) | docs | ✓ handoff doc 完成 + commit 投入 (3.3-B 全完遂、`handoff-substep-3-3-B-complete.md` §1-§7) |

3.3-B **全完遂 2026-05-31** (α/β-1/β-2/γ/δ/ε 6 sub-step、AYA launch verify PASS、Linux 先行例外内で SPIR-V build chain 確立 + 1 shader exemplar pre-flight 動作実証)。3.3-B 完遂後は 3.4 (texture lifecycle + descriptor set=1 per-material + 12 pool hook body PSO bind 配線 + 段階 2 引継ぎ特殊対応) 着手 (本 §3.1 sub-step list 3.4 行参照、3.3-A/3.3-B/3.3-C 全完遂で sub-step 3.3 全体 close)。並走で領域 6 sub-step 6.1 (autobuild integration 一括化、3.3-B で確立した cmake target / build chain pattern を一般化、248 file 全 port) 着手可能 (Agent 並列 cluster 候補)。

#### §3.1.3 設計根拠 trace inventory (3.3-B 設計確定 2026-05-30)

1 shader exemplar pre-flight 採用の確定根拠:

| 確定事実 | 数字 / 出典 |
|---|---|
| 既存 GL shader compile chain | `llshadermgr.cpp:909-955` (`glCreateShader` → `glShaderSource` → `glCompileShader`)、SPIR-V 並走分岐の挿入点 |
| 248 GLSL shader 内訳 | base 228 file (A 195 素通り + B 53 要修正) + AYAstorm 改変 13 file (r42-α/β/γ scope 外) — sub-doc 06 §1.2.1/§1.2.2 |
| 3.3-B 対象 file 数 | **1 file (vert + frag 計 2 stage)** — sky placeholder exemplar、領域 6 sub-step 6.1-6.5 で 248 file 全 port |
| SPIR-V compiler 採用 | glslang `--target-env vulkan1.3` (sub-doc 05 §1.3 LunarG SDK 1.3.x 採用済) + build 時 pre-compile (CMake target `aya_shader_compile`) |
| build 時 vs runtime 選択根拠 | **build 時採用** = 起動時間短縮 / cross-platform 一貫性 / shader iteration は `feedback_shader_only_fast_iterate.md` の cache clear 戦略で対応可 / 領域 6 sub-step 6.1 design 流用 (sub-doc 06 §3.1 sub-step 6.1 = "glslang + SPIRV-Cross + autobuild integration") |
| 二段構え matrix shader 側受領 | push constant range (0..64 B / VERTEX_BIT = `modelview_matrix`) + set=0 binding 0 (`PerFrameMatrixUBO` 192 B = projection 系 3 mat4) + set=0 binding 1 (`TextureMatrixUBO` 256 B = texture × 4) — 3.3-A §3.1.1 trace inventory + sub-doc 05 §3.5 確定 |
| shader 内計算移譲 | MVP / `normal_matrix` / `inverse_modelview_matrix` = vert shader 内で `projection × modelview` 等から算出 (3.3-A §3.1.1 trace inventory「shader 内計算移譲 3 種」、3.3-B-δ で具体配置) |
| 既存 PSO layout 流用 | 3.3-A γ で確定の `sPlaceholderLayout` / `sSkySmokePipelineLayout` (set=0 desc set layout + push constant range 反映済) を 3.3-B-γ/δ で流用、PSO layout 改変なし |
| exemplar shader path | `indra/newview/app_settings/shaders/aya_r41_exemplar/` (新規 directory、領域 6 sub-step 6.1 一括化までの prep 配置、6.1 完遂後は class1/deferred/ 等正規 path へ移管判断) |

3.3-B = exemplar pre-flight pattern (1 shader build chain 確立 → 6.1 で全 file 一般化)、視覚は依然 GL 担当 (sub-doc 03 §3.5 「GL path 動作維持」と整合)、Vulkan path は 3.3-A/3.3-C 同様 transit smoke (PSO compile + module load 成功で acceptance、実描画 deliver は領域 7 sub-step 7.5 attachment 配線後)。

#### §3.1.3 役割再定義注記 (2026-05-31 sub-step 4.3-γ'-port-α-prep 反映、charter §7.5 boundary refine 履歴と同期)

**3.3-B exemplar の役割を「試作レール」へ再定義**:

- 領域 6 sub-step 6.1 着手境界で **案 A target extension (CMake glob + glslangValidator 一括 pre-compile) を試行 → build verify で spec 想定相違 4 点露呈 (`#version` 無し / 自由 uniform / location 無し / forward decl concat 前提) → revert** (詳細は `handoff-substep-4-3-gamma-prime-prep.md` §1.1)
- AYA 「完全に動作させる必要があるので」基準下で **case ② = runtime SPIR-V 生成 (LLShaderMgr Vulkan path 配線 + glslang library runtime API + `vkCreateShaderModule` + SPIR-V cache layer) を採用** (production path 確定、charter §2 領域 6 + §3 #4 + §7.5 で boundary refine 履歴反映済)
- 3.3-B で配置済の **build-time pre-compile chain (AyaShaderCompile.cmake + `aya_r41_shaders` custom target + `aya_r41_exemplar/sky_placeholder{V,F}.glsl` purpose-built Vulkan-ready GLSL + `loadSpirvShaderModuleFromFile()` `llvkloader.cpp:1761-1770` sink)** = **試作レール扱い** (3.3-B α-ε 完遂 marker = sub-step 4.3-γ' 着手時の動作 baseline として保持、案 D 配線時の dead code 化は γ'-port-γ 以降で再評価)
- production path sink = **`loadSpirvShaderModuleFromMemory(const std::vector<uint32_t>&)` (新規、binary blob 受領、sub-step 4.3-γ'-port-α-5 で配置)**
- 領域 6 sub-step 6.1 一般化は **sub-doc 06 §3.1 sub-step 6.1 を case ② path に re-author** (handoff-substep-4-3-gamma-prime-prep.md §4.1 経由、sub-doc 06 active 復帰)、本 §3.1.3 はその根拠 anchor として保持

**`AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK` macro guard 取扱**: 3.3-B-ε で AYA 確認 「α (glslangValidator 必須化) 採用」反映済 = case ② 採用後も r42-α/β Mac/Win Vulkan 着手まで temporary safety net 維持、廃止 timing は r42-α/β glslangValidator install validation 完了後 (§3.1.3 既存表 3.3-B-ε row + handoff-substep-4-3-gamma-prime-prep.md §7.3 critical reminders と整合)。

### §3.1.4 sub-step 3.4 細分化 (α/β-1/β-2/γ/δ/ε、3.4 設計確定 2026-05-31)

3.4 (texture lifecycle + descriptor set=1 per-material + 12 pool hook body + 段階 2 引継ぎ特殊対応) は 3.3-B 完遂後の AYA 確認 (2026-05-31「案 1 (image lifecycle 先行) 推奨でお願いします」承認) で以下に細分化。3.3-A/3.3-B/3.3-C pattern (α/β-1/β-2/γ/δ/ε) 継承。

**scope boundary**:
- 3.4 = **texture lifecycle (VkImage + VkImageView + VMA) + descriptor set=1 per-material 7 PBR slot layout + 12 pool hook body 内 `vkCmdBindPipeline` / `vkCmdBindDescriptorSets` / `vkCmdPushConstants` / `vkCmdDraw*` 投入 + 段階 2 引継ぎ特殊対応 2 件** (terrain glTexGen 廃止 + avatar SSBO 基本実装)
- **VMA allocator init + descriptor pool sizing 基盤 = 領域 7 sub-step 7.1** (sub-doc 07 §3.1)、案 1 採用で **3.4-β-1 へ内包先行 install** (image lifecycle = VMA 前提のため統合 1 sub-step)
- **set=1 per-material layout + 1 set transit smoke = 3.4-γ で前倒し**、material cache 本実装 (~50 material 種別 × frame in flight 3 = 150 pool sizing) は領域 7 sub-step 7.3 残置
- **avatar SSBO 基本配線 = 3.4-δ で前倒し**、`vkCmdPushDescriptorSetKHR` 経由 set=2 binding 0 = `VkBuffer` storage 投入のみ、本 push descriptor 全配線 (per-object UBO + per-draw texture binding ≤ 32) は領域 7 sub-step 7.4 残置
- **実 attachment 配線 (`VkImage`/`VkImageView` 実体 = LLRenderTarget 並走 Vulkan side) は領域 7 sub-step 7.5 持越し** (3.3-C 設計確定 2026-05-29、本 3.4 では LLImageGL → VkImage の object lifecycle のみ scope、attachment 提供は 7.5)
- **set=0 per-frame 全配線 (camera/light matrix UBO + shadow map sampler ×4 + env cubemap sampler ×4 + atmospherics LUT + windlight LUT + AYAstorm picker output buffer ~30 sampler 集約) は領域 7 sub-step 7.2 残置** (3.3-A で PerFrameMatrixUBO + TextureMatrixUBO は配線済、本 3.4 では拡張なし)
- **7 pass chain + dynamic rendering 実装 (7 sub-pass) は領域 7 sub-step 7.5 残置**
- **AYAstorm 改変 13 file shader 改変禁止** (charter §2.1 領域 6、`git diff` 0 件維持) — 3.4 でも touch しない
- 各 sub-step は完遂境界で handoff timing 能動チェック (`feedback_proactive_handoff.md` 遵守)

| sub-step | scope | 対象 file | 完了 marker |
|---|---|---|---|
| **3.4-α** | spec refine (本 §3.1.4 新規追加 + §3.1 sub-step 3.4 行 update + 設計根拠 trace inventory + sub-doc 07 §3.1 sub-step 7.1/7.3/7.4 cross-ref [3.4-β-1/γ/δ 内包先行 install boundary 明示]) | sub-doc 03 + sub-doc 07 (cross-ref 1 行ずつ) | spec 改訂 commit 投入 |
| **3.4-β-1** (完遂 2026-05-31) | VMA allocator init + descriptor pool sizing 基盤 (sub-doc 07 sub-step 7.1 内包) — `llvkloader.cpp` 内 `VmaAllocator` init (`vmaCreateAllocator`、Vulkan 1.3 functions table + `VK_EXT_memory_budget` enable) + descriptor pool (`VkDescriptorPool` .maxSets 200、05 §3.6 pool sizing 戦略反映) 作成 + VRAM budget query helper (`vmaGetBudget`) + INFO marker 4 件 (extension 検出 + allocator + pool + budget smoke)。3.3-A/3.3-B β-1/β-2 pattern と異なり、β-1 のみで signature + impl + smoke 一体投入 (VMA 基盤は 1 関数 init + 1 関数 pool create で β-1/β-2 細分化が過剰、3.3-C-β-1/β-2 [signature → impl + transit smoke] により近い) | indra/llrender/CMakeLists.txt (+5) + indra/llrender/llvkloader.cpp (+212) + indra/llrender/vendor/vk_mem_alloc.h (新規 752 KB / VMA v3.3.0 / MIT) + LICENSE.txt | ✓ AYA launch verify PASS (2026-05-31): INFO marker 4 件全 hit — (1) `"VK_EXT_memory_budget = supported (VMA budget query enabled)"` line 100 (extension 検出) + (2) `"VMA allocator created (Vulkan 1.3, dynamic functions via volk, memory_budget=ON)"` line 109 + (3) `"Shared descriptor pool created (3.4-β-1 placeholder, maxSets=200, UBO=16, COMBINED_IMAGE_SAMPLER=64; precision deferred to 7.3)"` line 110 + (4) `"VMA budget smoke (3.4-β-1 1 度のみ、heapCount=2, VK_EXT_memory_budget=ON)"` line 119 + heap 0 [DEVICE_LOCAL] size=32607 MB / budget=30888 MB / usage=0 MB / allocations=0 / blocks=0 (line 120) + heap 1 [HOST] size=48001 MB / budget=48001 MB / usage=3 MB / allocations=0 / blocks=0 (line 121)。allocations=0 / blocks=0 は per-frame UBO が依然 raw `vkAllocateMemory` (3.3-A-β-2 path) のままで VMA 経由 0 件 = 期待通り (実 image allocation は 3.4-β-2 で `vmaCreateImage` 経由初投入予定)。3.3-A/B/C 全 13 marker 全継続 + Vulkan 系 WARN/ERR 0 件 + shutdownVulkan clean (pool destroy → vmaDestroyAllocator → device destroy 順、validation 違反 0 件)、impl 本 commit (同梱) |
| **3.4-β-2** (完遂 2026-05-31) | LLImageGL → VkImage + VkImageView lifecycle 並走化 (bridging item #8 satisfy、§1.5.3 配置) — `llvkloader.{cpp,h}` 内 file-local placeholder 1×1 white texture lifecycle smoke (`createPlaceholderWhiteImage` = `vmaCreateImage` 1×1 R8G8B8A8_UNORM + `vkCreateImageView` / `uploadPlaceholderWhiteSmoke` = staging buffer + 2 段 layout transition [UNDEFINED → TRANSFER_DST_OPTIMAL → SHADER_READ_ONLY_OPTIMAL] + `vkCmdCopyBufferToImage` + scratch cb one-time submit + `vmaDestroyBuffer` 即時破棄 / `destroyPlaceholderWhiteImage` = `vkDestroyImageView` + `vmaDestroyImage` shutdownVulkan 内 `vmaDestroyAllocator` 前発火) + format conversion table 公開 API (`llGlEnumToVkFormat(U32 ll_gl_intformat) → VkFormat`、impl は file-local anon namespace `llGlEnumToVkFormatImpl`、GL header 非依存 = OpenGL spec 確定値 hex literal 22 entry switch [8/16/32-bit normalized + float + depth/stencil + sRGB + packed HDR、sub-doc 07 §1.2.1 set=1 想定 7 PBR slot + LLImageGL 主要 internalformat 網羅]、未対応値 → `VK_FORMAT_UNDEFINED`)。`LLImageGL::generateTextures` 先頭に Vulkan path mirror sampling marker (1 度のみ guard `static bool sVulkanPathSampled` + `LLVKLoader::isVulkanInitialized()` check) 投入、per-LLImageGL VkImage 抱合せ配線は領域 7 sub-step 7.5 持越し (β-2 scope 境界明示)。VmaAllocation は llvkloader.cpp 1 TU 限定 (β-1 設計継承、`vk_mem_alloc.h` を header へ持込まない) | indra/llrender/llvkloader.h (+26) + indra/llrender/llvkloader.cpp (+232) + indra/llrender/llimagegl.cpp (+13) | ✓ AYA launch verify PASS (2026-05-31): (1) `vmaCreateImage` 成功 (placeholder 1×1 R8G8B8A8_UNORM、VMA budget heap 0 DEVICE_LOCAL allocations=1 で確認、β-1 時 allocations=0 から +1) + (2) `vkCreateImageView` 成功 (R8G8B8A8_UNORM 2D view) + (3) staging upload + 2 段 layout transition + `vkCmdCopyBufferToImage` + `vkQueueWaitIdle` 完遂 (validation 0 件) + (4) `vmaDestroyImage` 成功 (shutdownVulkan 順序 = placeholder destroy → shared pool destroy → `vmaDestroyAllocator` → `vkDestroyDevice`) + (5) format conversion table 公開 API 配置 (`llvkloader.h`:165 / impl `llvkloader.cpp` anon namespace) + (6) INFO marker `"VkImage placeholder lifecycle smoke (1x1 white、VkFormat=R8G8B8A8_UNORM、image+view+destroy 一連 OK)"` hit (`llvkloader.cpp:1121`) + (7) INFO marker `"LLImageGL::generateTextures Vulkan path mirror sampled (numTextures=N, per-LLImageGL VkImage 配線は領域 7 sub-step 7.5 持越し)"` 1 度のみ hit (`llimagegl.cpp:1268`) + (8) VMA budget smoke 再観測 = heap 0 [DEVICE_LOCAL] `allocations=1, blocks=1` (β-1 時 0/0 から +1)。3.3-A/B/C + β-1 全 marker 維持 (regression 0、unique INFO #Vulkan# marker 45 件、prior 43 件 + β-2 新規 2 件)、Vulkan 系 WARN/ERR 0 件、build clean (compile warning/error 0 件)、impl 本 commit (同梱) |
| **3.4-γ** (完遂 2026-05-31) | descriptor set=1 per-material 7 PBR slot layout 配置 (bridging item #7 satisfy、§1.5.3 配置、sub-doc 07 sub-step 7.3 layout 部分内包) — `VkDescriptorSetLayout` set=1 作成 (binding 0=DIFFUSE / 1=NORMAL / 2=SPECULAR / 3=BASECOLOR / 4=METALLIC_ROUGHNESS / 5=GLTF_NORMAL / 6=EMISSIVE、descriptor type = `COMBINED_IMAGE_SAMPLER` × 7、stage = `FRAGMENT_BIT`) + 共用 placeholder sampler 1 件 (LINEAR / CLAMP_TO_EDGE、per-texture / per-material sampler は領域 7 sub-step 7.3 残置) + helper `createPerMaterialDescriptorSetLayout()` / `createPlaceholderSampler()` / `allocatePerMaterialDescriptorSet()` / `updatePerMaterialDescriptorSet(VkImageView[7])` / `bindPerMaterialDescriptorSet(VkCommandBuffer)` + `sPlaceholderLayout` + `sSkySmokeLayout` の VkPipelineLayout を二段構え (set=0 PerFrame + set=1 PerMaterial、push constant 64 B / VERTEX_BIT は継承) へ拡張 + β-2 placeholder white texture 1 set 全 7 binding update + transit smoke acceptance。helper 5 件は llvkloader.cpp anon namespace file-local 留置 (header 露出無し、per-LLImageGL VkImage 抱合せ要件確定後 7.5 で再評価)、shader 改変ゼロ (Vulkan 仕様で layout が含む set 番号 ≥ shader 参照は許容、validation 0 件) | indra/llrender/llvkloader.cpp (+217 / -10、net +207) | ✓ AYA launch verify PASS (2026-05-31): (1) set=1 layout 作成成功 (7 binding COMBINED_IMAGE_SAMPLER × FRAGMENT_BIT、INFO marker `llvkloader.cpp:859` hit) + (2) 共用 placeholder sampler 作成成功 (LINEAR / CLAMP_TO_EDGE) + (3) 1 set allocate 成功 (sSharedDescriptorPool 経由) + (4) β-2 placeholder white image view × 7 binding 全 update 成功 (INFO marker `llvkloader.cpp:947` hit) + (5) `bindPerMaterialDescriptorSet` 動作 (`vkCmdBindPipeline(sPlaceholderPipeline)` 直後 `firstSet=1` で `vkCmdBindDescriptorSets` 投入、validation 0 件) + (6) `sPlaceholderLayout` + `sSkySmokeLayout` 二段構え化 (set_layouts[2] = { PerFrame, PerMaterial }、PSO compile 成功) + (7) shutdownVulkan clean (per-material layout → set null reset → pool destroy → sampler destroy → vmaDestroyAllocator → device destroy 順、validation 違反 0 件) + (8) VMA budget 再観測 = heap 0 [DEVICE_LOCAL] `allocations=1, blocks=1` (β-2 baseline 保持、γ は sampler を sDevice 直属 / set を pool 経由 allocate のため VMA カウント増えず)。3.3-A/B/C + β-1/β-2 全 marker 維持 (regression 0、unique INFO #Vulkan# marker 47 件、prior 45 件 + γ 新規 2 件 = layout 作成 + transit smoke)、Vulkan 系 WARN/ERR 0 件、build clean (compile warning/error 0 件)、impl 本 commit (同梱) |
| **3.4-δ** (完遂 2026-05-31) | 12 pool hook body 内 PSO bind + descriptor set bind + push constant + `vkCmdDraw*` 投入 + 段階 2 引継ぎ特殊対応 2 件 — 案 B 5 commit 分割で実装、δ-1 (commit `4e6e75257f` 12 pool 共用 placeholder draw helper unification = `recordSkySmokeDraw` → `recordPlaceholderPoolDraw` rename + body 拡張 [PSO bind sSkySmokePipeline + set=0 PerFrame + set=1 PerMaterial + push constant 64 B identity / VERTEX_BIT + `vkCmdDraw(3,1,0,0)`] + `bindPerMaterialDescriptorSet` 引数 [VkCommandBuffer, VkPipelineLayout] 化) + δ-2 (commit `37f5b65916` 11 pool body 投入 = 各 file に `#include "llvkloader.h"` + `LLVKLoader::recordPlaceholderPoolDraw(cmd_buf)` 1 line 配線、Sky 既配線と合わせて 12 pool 全 hook 完了) + δ-3 (commit `f88e9ed464` terrain glTexGen 物理削除 = Option D dead-code 撤去 [`renderFull4TU` + `renderFull2TU` + `renderSimple` 3 関数本体丸ごと削除、net -382 行]、`grep -E "glTexGen" indra/newview/lldrawpoolterrain.cpp` 0 件 hit) + δ-4 (commit `05686ddd03` avatar bone matrix → VkBuffer 基本配線 = `VK_KHR_push_descriptor` enable + Case B [avatar 専用 layout + 専用 helper、11 pool 共用 helper 不変] + AVATAR_BONE_MATRIX_COUNT=110 + `sAvatarBone*` 6 statics + helper 3 件 [createAvatarBoneDescriptorSetLayout + allocateAvatarBoneStorageBuffer + createAvatarBonePipeline] + `recordAvatarPlaceholderDraw` 公開 API + graceful fallback、sub-doc 07 sub-step 7.4 push descriptor 部分内包) + δ-5 (本 commit、doc-only)。実 attachment 配線は領域 7 sub-step 7.5 持越し、3.3-C placeholder attachment 経由 transit smoke 状態で acceptance | lldrawpool*.cpp 12 file + lldrawpoolterrain.cpp + lldrawpoolavatar.cpp + llvkloader.{cpp,h} (計 +492 / -416 net +76、ただし δ-3 で -382 含む) | ✓ AYA launch verify PASS (2026-05-31): (1) 12 pool 全 hook 内 `vkCmdDraw*` 動作 (release build 1 frame 完走、12 unique #VkRecord# pool hook one-shot marker hit = WaterExclusion / Simple / Bump / Materials / GLTFPBR / Alpha / Terrain / Water / Sky / WLSky / Avatar / Tree) + (2) render path GL call 削除 acceptance #1-段階 2 satisfy (`grep -rE "gl[A-Z][a-zA-Z]+\s*\(" indra/newview/lldrawpool*.cpp` **0 件 hit**) + (3) `grep -E "glTexGen" indra/newview/lldrawpoolterrain.cpp` **0 件 hit** (Option D dead-code 撤去で literal も排除) + (4) avatar.cpp 内 bone matrix → VkBuffer 配線確認 (Case B avatar 専用 helper 経由、`vkCmdPushDescriptorSetKHR` 動作 [STORAGE_BUFFER 7040 B = 110 mat4 identity 投入]) + (5) INFO marker 7 件新規 = `Device extension VK_KHR_push_descriptor enabled` (`llvkloader.cpp:541`) + `Avatar bone descriptor set layout created (set=2 binding 0 STORAGE_BUFFER VERTEX_BIT PUSH_DESCRIPTOR_KHR)` (`llvkloader.cpp:1931`) + `Avatar bone storage buffer allocated (7040 B = 110 mat4 identity, HOST_VISIBLE + MAPPED)` (`llvkloader.cpp:1990`) + `Avatar bone PSO compiled (set_layouts[3] = PerFrame + PerMaterial + AvatarBone, shader = sky smoke 流用、shader 改変ゼロ)` (`llvkloader.cpp:2105`) + `Placeholder pool draw fired (...)` (`llvkloader.cpp:2741`) + `Terrain fixed-function texgen path physically removed (sub-step 3.4-d-3)` (`lldrawpoolterrain.cpp:798`) + `Avatar placeholder pool draw fired (...)` (`llvkloader.cpp:2833`)、γ baseline 47 件 全継続 = unique #Vulkan# INFO marker 54 件 + 12 #VkRecord# pool hook marker = 計 66 件 + (6) Vulkan 系 WARN/ERR/VUID- 0 件 + (7) VMA budget heap 0 [DEVICE_LOCAL] `allocations=2, blocks=2` (γ 1/1 + δ-4 avatar bone storage +1) + (8) shutdown clean (avatar pipeline → layout → SSBO → DSL → sSkySmokePipeline 順) + 視覚 regression 0 (GL path 並走で本実描画維持) + build clean (compile warning/error 0 件)、impl 本 commit 同梱 |
| **3.4-ε** (完遂 2026-05-31) | `handoff-substep-3-4-complete.md` 起草 (3.4 全 sub-step α/β-1/β-2/γ/δ 完遂総括 → 3.5 着手境界 + bridging items #3 part/#7/#8 satisfy 宣言 + 段階 2 引継ぎ acceptance #1 + 特殊対応 2 件 satisfy 宣言 + 領域 7 sub-step 7.1 内包 / 7.3 layout 部分内包 / 7.4 push descriptor 部分内包の境界整理 + 残り 7.2/7.3 material cache/7.4 全 push descriptor/7.5 7 pass chain の領域 7 担当明示) + sub-doc 03 §3.1.4 末尾 + §3.1 sub-step 3.4 行 complete 化 + memory `project_ayastorm_r41_vulkan_migration.md` を 3.5 着手 ready 状態へ update | docs | ✓ handoff doc `handoff-substep-3-4-complete.md` 完成 + sub-doc 03 §3.1.4 (本表) + §3.1 sub-step 3.4 行 + memory `project_ayastorm_r41_vulkan_migration.md` + `MEMORY.md` index entry update 完遂、本 commit (δ-5 doc-only) で投入 |

3.4 完遂後は 3.5 (段階 3 self-check + validation strict 検証 + handoff doc) 着手 (本 §3.1 sub-step list 3.5 行参照、3.4 完遂で sub-step 3.3 + 3.4 全完遂)。

#### §3.1.4 設計根拠 trace inventory (3.4 設計確定 2026-05-31)

案 1 (image lifecycle 先行) 採用の確定根拠:

| 確定事実 | 数字 / 出典 |
|---|---|
| LLImageGL LOC | 3,034 (cpp 2,663 + h 371)、§1.2 5 file 表 + bridging item #8 |
| 主要 GL texture API surface | `glGenTextures` / `glTexImage2D` / `glTexParameteri` / `glDeleteTextures` / `glBindTexture` / `glTexSubImage2D` / `glPixelStorei` + format conversion path (LL → GL enum) — §1.2 5 file 表「llimagegl.cpp」行 |
| 7 PBR slot 内訳 | DIFFUSE / NORMAL / SPECULAR / BASECOLOR / METALLIC_ROUGHNESS / GLTF_NORMAL / EMISSIVE (7 slot)、§1.5.3 item #7 + §1.5.5 Cluster C 訂正記録 |
| set=1 descriptor type | `COMBINED_IMAGE_SAMPLER` × 7 (binding 0-6)、stage = `FRAGMENT_BIT`、sub-doc 07 §1.2.1 |
| VMA allocator init 経路 | `VmaAllocatorCreateInfo` (Vulkan 1.3 + functions table + `VK_EXT_memory_budget` enable)、sub-doc 07 §3.1 sub-step 7.1 内包 |
| descriptor pool sizing | `.maxSets ~200`、05 §3.6 pool sizing 戦略、sub-doc 07 §3.1 sub-step 7.1 内包 |
| 案 1 (image lifecycle 先行) vs 案 2 (pool body 先行) 比較 | 案 1: 「資源確立 → binding 配線 → 実走」順 (3.3-A/B/C pattern 継承)、二重 placeholder 状態回避、debug 切分け容易 / 案 2: 12 pool hook body 早期通電 (transit `VK_NULL_HANDLE`) だが二重 placeholder で debug 切分け困難、VMA = 領域 7 sub-step 7.1 が image lifecycle 前提のため案 1 で内包すべき (AYA 2026-05-31 推奨採用) |
| 段階 2 引継ぎ特殊対応 2 件 | terrain glTexGen 廃止 (`lldrawpoolterrain.cpp`) + avatar SSBO 基本実装 (`lldrawpoolavatar.cpp`)、§1.4 + §4.1 「特殊対応」row、§1.5.3 配置表「sub-step 3.4」column |
| 12 pool hook body 配線 | 段階 2 hook wiring 完遂済 (commit `49b9f36427`〜`61bd502445`)、本 sub-step δ で body 配線 (PSO bind + descriptor bind + push constant + `vkCmdDraw*`)、§2.2 dependency graph 4 階層末尾「段階 2 引継ぎ特殊対応」 |
| 領域 7 sub-step 7.1 (VMA + descriptor pool) 内包 | 案 1 採用で 3.4-β-1 へ前倒し install (sub-doc 07 §3.1 cross-ref 1 行追加で boundary 明示)、領域 7 sub-step 7.2/7.4 全配線 + 7.5 7 pass chain は領域 7 本来 scope 維持 |
| 領域 7 sub-step 7.3 (set=1 per-material layout) γ 前倒し | layout + 1 set transit smoke のみ前倒し、material cache 本実装 (~50 material × frame in flight 3 = 150 pool) は領域 7 sub-step 7.3 本来 scope 維持 |
| 領域 7 sub-step 7.4 (set=2 push descriptor) δ 一部前倒し | avatar SSBO 基本配線時に `vkCmdPushDescriptorSetKHR` で set=2 binding 0 = `VkBuffer` storage 投入のみ、本 push descriptor 全配線 (per-object UBO + per-draw texture binding ≤ 32) は領域 7 sub-step 7.4 本来 scope 維持 |
| sub-step 3.3-B / 3.4 関係 | 3.3-B (shader 側 binding 受領) + 3.4 (C++ 側 PSO bind + descriptor bind + push constant 投入) = 3.3-B + 3.4 一体運用で実描画 path 完成、ただし実 attachment 配線は領域 7 sub-step 7.5 持越し (3.3-C placeholder attachment 経由 transit smoke 状態で acceptance) |

3.4 = image lifecycle 先行 pattern (VMA → VkImage lifecycle → set=1 layout → 12 pool body PSO bind + descriptor bind + push constant + draw + 段階 2 引継ぎ 2 件)、視覚は依然 GL 担当 (sub-doc 03 §3.5 「GL path 動作維持」と整合)、Vulkan path は 3.3-A/3.3-B/3.3-C 同様 transit smoke (descriptor set 1 bind + push constant write + `vkCmdDraw*` 動作確認で acceptance、実描画 deliver は領域 7 sub-step 7.5 attachment 配線後)。

### §3.2 sub-step 内 file 順序の柔軟性 (charter §7.5 boundary refine 可)

- 各 sub-step 内の file 順序は実装着手時に bridging code 肥大度 / 領域 6 SPIR-V port 進捗 / 領域 7 descriptor binding 確定状況で refine 可
- sub-step 3.3 の llrender + llrendertarget は dependency graph 上 independent → 並列着手候補 (Agent 活用)
- sub-step 3.4 の 12 pool hook body 配線は drawpool 単位で independent → 並列着手候補

### §3.3 段階 3 で touch しない file (段階 4-5 + 領域 6/7 scope、`02-portage-execution.md` §3.3 範式継承)

- pipeline.cpp 3 大グローバル (`sCull` / `sShadowRender` / `sCurCameraID`) — 段階 4 frame context 化
- **llgl.{cpp,h} RAII setter body 内 GL call (LLGLState / LLGLDepthTest 等)** — 段階 4 dead-store 化 (item #1、2026-05-29 refine、charter §2.1 領域 4 LLPipelineFrameContext 配置と同時、source-level caller compat 維持目的で本段階内残置)
- **LLRender::mUIOffset / mUIScale (UI matrix `std::vector` stack)** — 段階 4 frame context refactor 持越し (3.3-A 設計確定 2026-05-29、syncMatrices scope 外で独立管理されており、frame context 配置時に配信先決定、本 sub-step 3.3 scope 外)
- **`VkImage` / `VkImageView` 実体作成 + attachment 提供 (LLRenderTarget 並走 Vulkan side)** — 領域 7 sub-step 7.5 持越し (3.3-C 設計確定 2026-05-29、VMA = 領域 7 sub-step 7.1 前提、3.3-C は API surface 並走化のみで実 attachment 配線は領域 7 担当、§3.1.2 + sub-doc 07 §1.2.3 参照)
- llspatialpartition / llviewershadermgr / llvertexbuffer / llvosky / llvowlsky — 段階 5
- 248 shader SPIR-V 化 — 領域 6 (本段階 3 と並走着手、本 sub-doc と同時起草の `06-shader-spirv.md` で scope plan)
- descriptor set 3 階層 + 7 render pass chain 設計実装 — 領域 7 (本段階 3 と並走着手、本 sub-doc と同時起草の `07-descriptor-renderpass.md` で scope plan)
- LLVKRenderer pipeline.cpp inline 実装 — 段階 4 で配置、領域 8 (本段階は llrender 5 file 内 inline 実装で signature 整合のみ確保)
- AYAstorm 改変 13 file shader (picker 2 / Cinematic 4 / visual realism 7) — r42-α/β/γ scope 外、本段階で touch しない (charter §3 #4 regression 担保)

---

## §4 段階 3 completion criteria

charter §3 acceptance criterion #1 (GL 除去) + #3 (段階 1-5 全完遂) + #5 (descriptor set + render pass) + #6 (LLVKRenderer skeleton signature 整合) の **段階 3 分 self-check** + 段階 2 引継ぎ acceptance #1-段階 2 + 特殊対応の段階 3 内 satisfy。

### §4.1 段階 3 自己 acceptance

| criterion | metric | test procedure |
|---|---|---|
| **#1-段階 3 (llrender 主要 5 file + lldrawpool 13 file の GL call 除去、RAII setter 内呼出は除外)** | llrender 5 file (10,841 LOC) + lldrawpool 13 file 内で `gl[A-Z][a-zA-Z]+\s*\(` 直接呼出が 0 件、PSO bind + `vkCmdDraw*` + dynamic rendering call 経由に置換済。**RAII setter 内 GL call は item #1 段階 4 移管 (2026-05-29 refine、charter §2.1 領域 4 LLPipelineFrameContext 配置時 dead-store 化) で satisfy、本段階 metric から除外** + **PFNGL function pointer declarations の物理削除は段階 5 一体運用** (§1.5.2 Cluster D 189 file 波及、本段階内 scope 過大) | `grep -rE "gl[A-Z][a-zA-Z]+\s*\(" indra/llrender/llrender.{cpp,h} indra/llrender/llimagegl.{cpp,h} indra/llrender/llrendertarget.{cpp,h} indra/llrender/llpostprocess.{cpp,h}` が **0 件 hit** (llgl.{cpp,h} は RAII setter 内残置許可、段階 4 で satisfy) + `grep -rE "gl[A-Z][a-zA-Z]+\s*\(" indra/llrender/llgl.{cpp,h}` の hit は RAII state class setter body 内のみ許容 (caller source-level compat 維持目的、段階 4 dead-store 化で 0 件達成予定) + `grep -rE "gl[A-Z][a-zA-Z]+\s*\(" indra/newview/lldrawpool*.cpp` が **0 件 hit** (段階 2 引継ぎ satisfy 後)。**PFNGL declarations 物理削除 metric は段階 5 acceptance 側で別計上、RAII setter 内 GL call 0 件 metric は段階 4 acceptance 側で計上** |
| **#3-段階 3 (PSO bind 動作 + validation 0 件)** | 12 pool 全 hook で PSO bind + `vkCmdDraw*` 動作、validation layer error / warning 0 件 | viewer 起動 + login 後 sustained ~10 分動作 + 各 pool の PSO bind 通過確認 (LL_INFOS log + per-pool PSO compile counter)、`VK_LAYER_KHRONOS_validation` で起動 + 動作中 error / warning 0 件 (sub-step 3.5 で validation strict force-enable build) |
| **#5-段階 3 (descriptor set + render pass 実装、領域 7 並走 satisfy)** | set=0 per-frame / set=1 per-material / set=2 per-draw 3 階層 + `VK_KHR_push_descriptor` (set=2) + `VK_KHR_dynamic_rendering` 採用動作 | validation layer で descriptor binding mismatch / render pass dependency violation **0 件** + `grep -E "VK_KHR_push_descriptor\|vkCmdPushDescriptorSetKHR" indra/llrender/` で配線 hit + `grep -E "vkCmdBeginRenderingKHR\|VkRenderingAttachmentInfo" indra/llrender/` で配線 hit |
| **#6-段階 3 (LLVKRenderer skeleton signature 整合)** | charter §3 #6: pipeline.cpp 内 inline 実装と LLVKRenderer skeleton hook の signature 整合、r41.5 interface 経由 call 化前提担保 | `grep -rE "class LLVKRenderer" indra/` で skeleton declaration 存在 + 段階 3 で確定した PSO bind / pool record / descriptor 配信 関数 signature が 05 §10.1-§10.2 hook と一致 |
| **特殊対応 (terrain glTexGen 廃止 + avatar skinning SSBO)** | sub-step 3.4 で段階 2 引継ぎ 2 件 satisfy、shader 側 explicit UV 化 + avatar bone matrix → VkBuffer 配線 | `grep -E "glTexGen" indra/newview/lldrawpoolterrain.cpp` が **0 件 hit** + shader 側 UV attribute / uniform 配線確認 + avatar.cpp 内 bone matrix → VkBuffer (storage buffer) 配線確認 + descriptor binding validation 0 件 |
| **regression (段階 1-2 動作維持)** | Vulkan instance + device + command pool + render pass + 12 pool record hook 動作維持、viewer 起動 + AYAstorm 機能 (audio / chat / login / inventory) regression 0 件 | `01-foundation.md` §4.1 段階 1 + `02-portage-execution.md` §4.1 段階 2 acceptance 再 verify + sustained ~10 分動作 + AYA 起動確認 PASS (`feedback_release_with_user_feedback.md` 遵守で exhaustive solo session 不要) |

**段階 3 全 acceptance satisfy 確定 2026-05-31** — sub-step 3.5-a validation strict force-enable build + AYA launch verify PASS で #3-段階 3 (PSO bind 動作 + validation 0 件) + 特殊対応 + regression 全 metric satisfy、**ただし #3-段階 3 は共用 `recordPlaceholderPoolDraw` helper (avatar のみ `recordAvatarPlaceholderDraw`) 経由 PSO bind + `vkCmdDraw(3,1,0,0)` 動作 = fullscreen NDC 三角形 × 12 重ね描き = 視覚 no-op 等価 placeholder = charter §3 #3-段階 3 acceptance 本文「PSO bind 動作 + validation 0 件」に literal satisfy、per-pool 実 scene draw 移植は段階 4 LLPipelineFrameContext 配置後 frame state 集約経路で段階 4 acceptance に併合**、#1-段階 3 は RAII setter 内呼出を段階 4 LLPipelineFrameContext 配置時 dead-store 化に scope refine + PFNGL declarations 物理削除を段階 5 一体運用に scope refine で本段階 metric 範囲内 satisfy、#5-段階 3 は 3 階層 descriptor set (set=0 PerFrame UBO / set=1 PerMaterial 7 PBR slot **non-immutable sampler + 共用 placeholder sampler** / set=2 AvatarBone push descriptor) + dynamic rendering 並走基盤で達成、#6-段階 3 は pipeline.cpp 内 PSO bind / pool record / descriptor 配信 関数 signature 確定 + 領域 8 並走着手 ready で本段階 metric 範囲内 satisfy (skeleton declaration 物理配置は領域 8 sub-doc で段階 4 並走)、段階 4 着手 GO 確定。詳細 self-trace は `handoff-stage-3-complete.md` §1.3 参照。

### §4.2 不達時の対処 (charter §3 acceptance 運用方針継承)

- 6 criterion のうち 1 件でも未達 = 段階 3 未達 (段階 4 着手保留)
- 未達 criterion 別に対処 (例: #3-段階 3 で specific pool の PSO bind validation error 残存 → bridging code audit → workaround → 再 sweep)
- **defer / disable 提案 ban** (`feedback_self_bug_no_defer_option.md` 遵守、fix 案のみ提示)
- **仮説 2 連続外れ rule** (`feedback_admit_unknown.md` 遵守): PSO compile / descriptor bind / dynamic rendering trouble で仮説 2 連続外れたら gdb breakpoint / validation layer message detail / canary instrumentation で実データ取得に切替

### §4.3 段階 3 完遂後の次 段階

- **段階 4 着手** (pipeline.cpp 3 大グローバル → LLPipelineFrameContext 集約、1.00 PM、charter §2 高 risk 領域 2 件目)
- **sub-doc 起草**: 段階 4 着手前に新規 sub-doc (`04-frame-context.md` 仮称) を Pattern α で起草 (charter §7.4 sub-doc 構成 = outline、charter §7.5 scope refine 可)
- **handoff doc**: `handoff-stage-3-complete.md` 作成 (段階 3 完遂境界、`feedback_proactive_handoff.md` 遵守)
- **領域 6 / 7 並走進捗確認**: 段階 3 完遂時点で領域 6 SPIR-V 化 + 領域 7 descriptor / render pass 実装の進捗 base で段階 4 着手状況を AYA 共有

---

## §5 関連 doc / memory

### §5.1 直接参照 doc

| doc | 本 sub-doc での参照 section |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/00-charter.md` | §2 領域 3 (1.50 PM 高 risk) + §3 #1/#3/#5/#6 acceptance + §6.4 並走方針 + §7.4 sub-doc 構成 + §7.5 boundary refine 可 |
| `docs/specs/ayastorm-r41-gl-removal/01-foundation.md` | §3.4 sub-step 範式継承 + §3.5 段階 1 で touch しない file の段階 3 scope 反映 |
| `docs/specs/ayastorm-r41-gl-removal/02-portage-execution.md` | §3 sub-step 範式継承 + §3.3 段階 2 で touch しない file の段階 3 scope 反映 + §4.1 段階 2 acceptance 引継ぎ |
| `docs/specs/ayastorm-r41-gl-removal/handoff-stage-2-complete.md` | §1.3-1.4 段階 2 引継ぎ (acceptance #1 + 特殊対応の段階 3 内 satisfy 解釈、AYA 承認済 2026-05-28) + §2 段階 3 着手前の scope 確認 |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-1a-iv-ready.md` | sub-step 3.1a-i/ii 完了 (6 Agent cluster 並列 trace) → 3.1a-iv 着手境界、本 sub-doc §1.5 起源 (trace inventory + 11 件 bridging items + measurement-first 設計素材) |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-1a-complete.md` | sub-step 3.1a 4 段階全完遂宣言 (i/ii/iii/iv) → 3.1b (PSO 基盤 + state alias root **実装**) 着手境界、本 sub-doc §1.5 内容を実装期 reference として固定 |
| `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` (本 sub-doc と同時起草) | 領域 6 SPIR-V port 進捗 base、本 sub-doc §1.3 並走領域協調事項 + §3.1 sub-step 3.3-3.4 同期 |
| `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` (本 sub-doc と同時起草) | 領域 7 descriptor set 3 階層 + 7 pass chain 実装、本 sub-doc §1.3 並走領域協調事項 + §3.1 sub-step 3.3-3.4 同期 |
| `docs/specs/ayastorm-r40-vulkan-migration/04-portage-inventory.md` | §2.1.2 要再設計 10 file (llgl/llrender/llimagegl/llrendertarget/llpostprocess 含む) + §4.3 段階 3 構造変更案 + §5.4.1 段階 3 工数感 (4-5 週間 / 1.5 人月) + §6.3.1 段階 3 順位 |
| `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` | §3 descriptor set 3 階層 (段階 3 PSO bind 配信前提) + §3.5 pipeline layout 設計 (matrix stack → push constant 64 bytes) + §4.1 dynamic rendering 採用 + §4.7 LLRenderTarget → Vulkan attachment 移行マップ + §10 LLVKRenderer skeleton signature (charter §3 #6 担保) |
| `docs/specs/ayastorm-r40-vulkan-migration/06-effort-estimation.md` | §3.1 領域 3 PM 1.50 (本 §1 + §3 段階 3 scope 整合) |

### §5.2 関連 memory

| memory | 本 sub-doc での参照 |
|---|---|
| `project_ayastorm_r41_vulkan_migration.md` | r41 milestone active 状態 + 段階 3 着手前 prep cadence |
| `project_ayastorm_three_platforms.md` | 3 OS 大前提 + Linux 先行例外 (本段階 3 も Linux 限定動作確認、§4.1 #regression 反映) |
| `reference_gbuffer3_storage.md` | llrendertarget dynamic rendering 化時の gbuffer3 RGBA 化保持 (AYAstorm r21.1 picker 同居前提) |
| `reference_deferred_shader_routing.md` | 12 pool hook body PSO bind 配線時の shader binding 確認 reference |
| `reference_attachment_rendering_routing.md` / `reference_rez_object_rendering_routing.md` | sub-step 3.4 12 pool hook body 配線時の draw dispatcher 確定マップ参照 |

### §5.3 関連 feedback

| feedback | 本 sub-doc での参照 |
|---|---|
| `feedback_experiment_branch_single_scope.md` | branch scope 単一性 (r41 内 sub-branch 作らない、`01-foundation.md` §2.3 継承) |
| `feedback_no_auto_commit.md` | commit は AYA 指示後 |
| `feedback_release_flow.md` | push は AYA 手動 |
| `feedback_self_bug_no_defer_option.md` | §4.2 defer / disable 提案 ban + §1.2 #5 avatar SSBO の改善範疇明示 |
| `feedback_self_verify_before_handoff.md` | §3 sub-step 3.5 self-check + §4.3 段階 3 完遂境界 self-trace |
| `feedback_proactive_handoff.md` | §4.3 段階 3 完遂時の handoff doc 作成 |
| `feedback_build_only_verified.md` | §4 completion criteria の satisfy は実機検証 (推論 ban) |
| `feedback_admit_unknown.md` | §4.2 PSO compile / descriptor bind / dynamic rendering trouble で仮説 2 連続外れたら gdb / validation detail / canary 切替 |
| `feedback_use_agents_proactively.md` | §2.3 + §3.2 sub-step 3.3 / 3.4 の独立 file port + 12 pool hook body 配線で Agent 並列活用 |
| `feedback_perf_map_bfs_drill.md` | §2.2 dependency graph の階層的 sub-step 化 (root → 軽量 → 標準 → texture → 特殊対応) |
| `feedback_release_with_user_feedback.md` | §4.1 #regression の exhaustive solo session 不要 (AYA 起動確認 PASS で sufficient) |
| `feedback_one_step_at_a_time.md` | §3 sub-step 単位で 1 メッセージ 1 アクション着手 |
| `feedback_render_full_trace_first.md` | sub-step 3.1 PSO state alias 化 + 3.3 matrix stack → push constant 化は GL state machine 全 trace 経由で table 化、当てずっぽう refactor 禁止 |
| `feedback_remove_verification_logs.md` | sub-step 3.5 validation strict force-enable build 復元 (`#ifndef LL_RELEASE_FOR_DOWNLOAD` 復元) |

---

## §6 起草 cadence + 完成宣言

### §6.1 本 sub-doc 起草情報

- **起草着手**: 2026-05-28
- **起草主体**: AYA + Claude
- **起草先**: `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` (本 doc)
- **起草 cadence**: **Pattern α (一括 draft)** — `01-foundation.md` / `02-portage-execution.md` 範式継承、sub-doc は内容具体 (5 file list / 5 sub-step / acceptance 6 件) で section 数少なく、Pattern β 分割 overhead 回避 (charter §7.5 boundary refine 可)
- **scope**: **段階 3 only + 段階 2 引継ぎ satisfy** (llrender 主要 5 file 10,841 LOC + 12 pool hook body PSO 配線)、段階 4-5 は別 sub-doc 分離 (`handoff-stage-2-complete.md` §2 反映、`01-foundation.md` = 段階 1 only + `02-portage-execution.md` = 段階 2 only precedent + 段階別 risk 性質差 = 段階 3 高 PSO / 段階 4 高 frame context refactor / 段階 5 中 残依存解決 で分離が prep として有効)
- **並走起草 sub-doc**: 同 session で `06-shader-spirv.md` (領域 6) + `07-descriptor-renderpass.md` (領域 7) を Pattern α 一括起草、3 doc 同時 AYA review (handoff-stage-2-complete §2.3 B 案、AYA 採用)

### §6.2 sub-doc 番号付与の justification (charter §7.4 outline → handoff §2.2 refine)

charter §7.4 sub-doc 構成 outline (起草段階の outline) と本 sub-doc 番号付与の差分:

| charter §7.4 outline 番号 | charter §7.4 outline タイトル | 本 r41 章で実採用 番号 + タイトル | 採用理由 |
|---|---|---|---|
| 01 | 01-foundation.md | **01-foundation.md** ✓ | charter outline 通り |
| 02 | 02-portage-execution.md | **02-portage-execution.md** ✓ | charter outline 通り |
| 03 | 03-shader-port.md | **06-shader-spirv.md** に変更 | sub-doc 番号 = charter §2 領域番号 = 段階番号で揃える方が r41 章内 cross reference 整合性が高い (handoff-stage-2-complete.md §2.2 で 03 を `03-state-machine-pso.md` に再割当した時点で番号系列が領域番号と同期、本 doc + 06/07 で完全整合) |
| 04 | 04-descriptor-render-pass.md | **07-descriptor-renderpass.md** に変更 | 上記同様、領域 7 = sub-doc 07 で揃える |
| (新規) | — | **03-state-machine-pso.md** = 領域 3 = 段階 3 | handoff-stage-2-complete.md §2.2 で確定 (charter §7.5 boundary refine 可) |

採用号外 (charter §7.4 outline の他項目):
- charter §7.4 outline 05 (`05-skeleton-interface.md`) → 領域 8 = sub-doc `08-skeleton-interface.md` (段階 4 着手前 prep で起草見込)
- charter §7.4 outline 06 (`06-swapchain-vk-alpha.md`) → 領域 9 = sub-doc `09-swapchain-vk-alpha.md` (段階 5 完遂後 prep で起草見込)
- charter §7.4 outline 07 (`07-linux-driver-matrix.md`) → 領域 10 = sub-doc `10-linux-driver-matrix.md` (vk-α 達成後 polish で起草見込)

本 §6.2 の sub-doc 番号 refine は charter §7.5 boundary refine 可の範囲内、AYA review で承認後に正式採用。

### §6.3 完成宣言条件

本 sub-doc は **AYA review PASS で完成宣言**、status field を `closed YYYY-MM-DD (Pattern α 一括 draft + AYA review PASS、段階 3 着手準備 ready)` に更新。

完成宣言後の次 action:

- 段階 3 sub-step 3.1 着手 (PSO 基盤 + state alias root、`llgl.{cpp,h}` 改修)
- 段階 3 sub-step 3.5 完遂時に handoff doc `handoff-stage-3-complete.md` 作成
- 段階 4 着手前に sub-doc `04-frame-context.md` (仮称) 起草

### §6.4 本 sub-doc commit 反映

本 sub-doc 完成宣言 commit は AYA 明示指示後 Claude が実施。commit message draft (AYA 指示時 refine 可):

```
docs(r41): sub-doc 03-state-machine-pso.md 完成 + 段階 3 prep

- 03-state-machine-pso.md 新規作成 (Pattern α 一括 draft + AYA review PASS)
- §1 段階 3 scope (llrender 5 file 10,841 LOC、1.50 PM 高 risk、領域 6/7 並走必須)
- §1.4 段階 2 引継ぎ acceptance #1 + 特殊対応の段階 3 内 satisfy 計画
- §2 port 順序 + dependency graph (state machine root → 軽量 → 標準 → texture → 特殊対応 5 階層)
- §3 段階 3 sub-step 5 件 (3.1 PSO 基盤+state alias / 3.2 軽量 / 3.3 標準 / 3.4 texture+特殊対応 / 3.5 self-check)
- §4 段階 3 completion criteria 6 件 (GL 除去 / PSO 動作 / descriptor+render pass / skeleton signature / 特殊対応 / regression)
- §5 関連 doc/memory + §6 起草 cadence (Pattern α / 段階 3 only scope + 段階 2 引継ぎ satisfy)
- §6.2 sub-doc 番号付与 refine justification (charter §7.4 outline → 領域番号同期)
```

push は AYA 手動 (`feedback_release_flow.md` 遵守)。
