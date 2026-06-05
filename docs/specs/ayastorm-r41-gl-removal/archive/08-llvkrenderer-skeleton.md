# AYAstorm r41 sub-doc 08-llvkrenderer-skeleton — 段階 4 領域 8 LLVKRenderer skeleton declaration 物理配置 + signature 整合 (sub-doc 04 並走)

**status**: **draft 2026-05-31 (Pattern α 一括 draft、charter §7.4 標準範式、AYA review 待ち)**
**親 charter**: `docs/specs/ayastorm-r41-gl-removal/00-charter.md` (closed 2026-05-28)
**並走 sub-doc**: `04-frame-context.md` (draft 2026-05-31、AYA review 完了 PASS、段階 4 領域 4)
**前 handoff**: `handoff-stage-3-complete.md` (段階 3 完遂 → 段階 4 着手境界)
**達成条件**: charter §3 #6 acceptance 段階 4 内 satisfy = `class LLVKRenderer` skeleton declaration 物理配置 + 既存 namespace `LLVKLoader` (llvkloader.{h,cpp}) の inline 実装と signature 整合 + sub-doc 04 `LLPipelineFrameContext` caller 配線整合 + r41.5 で interface 経由 call 化への signature 確定
**関連 charter section**: §2 領域 8 (Vulkan code abstraction skeleton、0.50 PM 低 risk、領域 4 と協調) + §3 #6 acceptance (LLVKRenderer interface skeleton) + §3 関連 #1 #3 #5 (GL 除去 / 段階 1-5 / descriptor) + §7.4 sub-doc 構成 + §7.5 boundary

---

## §1 領域 8 scope plan

### §1.1 領域 8 scope 再掲 (charter §2 領域 8)

**charter §2 領域 8 literal (charter:129-134 行抜粋)**:

```
#### 領域 8: Vulkan code abstraction skeleton (interface placeholder) (0.50 PM)
- 境界条件: r41.5 着手前に LLVKRenderer skeleton hook 配置完了 (interface signature 詳細化は r41.5)
- 依存順序: 領域 4 (pipeline.cpp frame context) と協調、interface hook は pipeline.cpp 内 inline 実装に併走
- risk: 低 — skeleton 配置のみ、interface 詳細は r41.5
```

**§7.4 sub-doc 構成 (charter:456-464 行) との divergence note**:

- charter §7.4 では領域 8 対応 sub-doc を `05-skeleton-interface.md` と記載
- handoff-stage-3-complete §5.1 cadence では領域 8 sub-doc 仮称を `08-llvkrenderer-skeleton.md` と記載 (2026-05-31 AYA 承認済)
- **本 sub-doc は handoff §5.1 仮称を採用** (理由: 既存 `03-state-machine-pso.md` / `04-frame-context.md` 命名も charter §7.4 outline と divergent、実装中 refine の結果として番号 = 領域番号運用に収束済、本 sub-doc も同範式)
- charter §7.5 「r41 着手中に refine 可な本 charter content」範囲内 refine、§7.4 outline は初期構想 + 実装中 refine 許容範囲

### §1.2 sub-doc 04 並走 起草 (handoff §5.1 cadence 4. + 2026-05-31 AYA 推奨経路)

- **並走起草経緯**: sub-doc 04 (`04-frame-context.md`) AYA review PASS 直後の 2026-05-31 AYA 推奨採用 (sub-doc 04 完遂後判断で並走起草 GO)
- **並走理由 (本 sub-doc 草稿時点記録)**:
  - charter §3 acceptance #6 (LLVKRenderer skeleton declaration) は段階 3 で「領域 8 scope refine」として一度延ばした項目、段階 4 内で satisfy 求められる
  - sub-doc 04 §3 で導入する `LLPipelineFrameContext` の caller 側 = LLVKRenderer skeleton になるため、両 sub-doc を並走 draft して struct API 整合を取った方が手戻り無い
  - 領域 8 は charter §2 で 0.50 PM / 低 risk 評価、領域 4 (1.00 PM / 高 risk) より軽い → sub-doc 04 review 通過直後の並走 draft 着手で周回境界を歪めない
  - 段階 4 sub-step 4.1 着手前に両 sub-doc 揃えれば、実装 phase で「struct shape を sub-doc 04 で固めたが LLVKRenderer 側で破綻」リスクを構造でゼロにできる

### §1.3 段階 3 引継ぎ (handoff §1.3 #6 反映)

- **handoff §1.3 acceptance #6-段階 3 status**: 「領域 8 scope refine」として段階 3 から段階 4 内 satisfy へ移管済
- **段階 3 で確定済 signature 前提**: handoff §1.3 #6-段階 3 で「pipeline.cpp inline 実装 signature 確定」、本 sub-doc では既存 namespace LLVKLoader API (§2 inventory) を skeleton declaration の signature source とする
- **段階 4 内 satisfy target**: `class LLVKRenderer` skeleton declaration 物理配置 + 既存 inline 実装 (namespace LLVKLoader) との signature 整合 (charter §3 #6 metric / test / regression)

### §1.4 起草 cadence (Pattern α 一括 draft → AYA review boundary)

- **起草 pattern**: Pattern α 一括 draft (charter §7.4 標準範式)、sub-doc 03 (closed 2026-05-31) / sub-doc 04 (draft 2026-05-31) 範式継承
- **起草 deliverable**:
  - §1 領域 8 scope plan (charter §2 領域 8 + sub-doc 04 並走 + handoff §1.3 #6 反映)
  - §2 既存 namespace LLVKLoader inventory (Agent B 並列 trace sealed)
  - §3 `class LLVKRenderer` skeleton declaration 設計 (placement / class outline / signature)
  - §4 sub-doc 04 `LLPipelineFrameContext` との配線 (caller 整合)
  - §5 sub-step list (本 sub-doc は段階 4 sub-step 4.4 内 part B として運用、独立 sub-step 8.x は採用しない)
  - §6 完遂条件 (acceptance #6 段階 4 内 satisfy + 関連 #1 #3 #5)
  - §7 関連 doc / memory
- **AYA review boundary**: 本 draft 完成後 AYA review、PASS 後に sub-step 4.1 着手 (sub-doc 04 と同タイミング)

---

## §2 既存 namespace LLVKLoader inventory (Agent B 並列 trace sealed 2026-05-31)

本 §2 は段階 4 着手前 trace-before-implement (`feedback_render_full_trace_first.md` 遵守) として Agent B 並列 trace で取得した既存 stub inventory を sealed 形で保全。skeleton declaration の signature source とする。

### §2.1 ファイル概観 (Agent B sealed)

| ファイル | 行数 | 役割 |
|---|---|---|
| `indra/llrender/llvkloader.h` | 211 行 | namespace LLVKLoader 公開 API declaration |
| `indra/llrender/llvkloader.cpp` | 2981 行 | namespace LLVKLoader 実装 (initVulkan / shutdownVulkan / beginFrame / endFrame / placeholder draw / descriptor / UBO / dynamic rendering / SPIR-V loader 全実装) |

**`class LLVKRenderer` 現状**:

- `grep -rE "class\s+LLVKRenderer" indra/` で **hit 0 件 confirmed** (Agent B sealed)
- docs/specs 内に class reference 3 件存在 (本 sub-doc 起草前の設計文書内)
- **段階 4 sub-step 4.4 (part B) で物理配置 = charter §3 #6 acceptance test procedure satisfy 経路**

### §2.2 既存 namespace LLVKLoader API inventory (skeleton declaration signature source)

下表は段階 3 完遂時点 (handoff-stage-3-complete §1.3 #6 確定 signature) で namespace LLVKLoader が公開する API、`class LLVKRenderer` skeleton declaration の **member function signature source** として採用する。

| # | API | 宣言 file:line | signature | 役割 |
|---|---|---|---|---|
| 1 | `initVulkan` | llvkloader.h:近傍 (Agent B sealed) | `bool initVulkan()` | Vulkan instance / device / VMA / placeholder image / sampler / PerFrame UBO / 12 PSO / avatar bone SSBO 初期化 |
| 2 | `shutdownVulkan` | llvkloader.h:近傍 | `bool shutdownVulkan()` | 14 段階順 teardown (sub-doc 03 §3.3 確定) |
| 3 | `isVulkanInitialized` | llvkloader.h:近傍 | `bool isVulkanInitialized()` | initVulkan 成否 query |
| 4 | `isValidationEnabled` | llvkloader.h:近傍 | `bool isValidationEnabled()` | validation layer enable 状態 query |
| 5 | `beginFrame` | llvkloader.h:近傍 | `bool beginFrame()` | per-frame begin (acquire swapchain image + cmd buf begin) |
| 6 | `endFrame` | llvkloader.h:近傍 | `bool endFrame()` | per-frame end (cmd buf submit + present + fence wait) |
| 7 | `getCurrentCommandBuffer` | llvkloader.h:近傍 | `VkCommandBuffer getCurrentCommandBuffer()` | 当該 frame の primary command buffer 取得 |
| 8 | `getDevice` | llvkloader.h:近傍 | `VkDevice getDevice()` | logical device handle 取得 |
| 9 | `getPipelineCache` | llvkloader.h:近傍 | `VkPipelineCache getPipelineCache()` | PSO cache handle 取得 |
| 10 | `createStandardPipelineLayout` | llvkloader.h:38-42 | `VkPipelineLayout createStandardPipelineLayout(...)` | 標準 pipeline layout 生成 (set=0 PerFrame + set=1 PerMaterial + push constant 64 B) |
| 11 | `compileGraphicsPipeline` | llvkloader.h:46 | `bool compileGraphicsPipeline(const VkGraphicsPipelineCreateInfo& ci, VkPipeline& out_pipeline)` | graphics PSO compile (vkCreateGraphicsPipelines + cache) |
| 12 | **`recordPlaceholderPoolDraw`** | llvkloader.h:53 / .cpp:2694 | `void recordPlaceholderPoolDraw(VkCommandBuffer cmd_buf)` | 11 pool 共用 placeholder draw (PSO bind sSkySmokePipeline + vkCmdDraw(3,1,0,0) fullscreen NDC 三角形、視覚 no-op 等価) |
| 13 | **`recordAvatarPlaceholderDraw`** | llvkloader.h:65 / .cpp:2756 | `void recordAvatarPlaceholderDraw(VkCommandBuffer cmd_buf)` | avatar pool 専用 placeholder draw (PSO bind sAvatarBonePipeline + set=2 push descriptor SSBO + push constant + vkCmdDraw(3,1,0,0)、avatar 専用 layout) |
| 14 | `getCurrentFrameIndex` | llvkloader.h:107 | `U32 getCurrentFrameIndex()` | sFrameIndex 取得 (PerFrame double-buffer index) |
| 15 | `writeCurrentPerFrameMatrixUBO` | llvkloader.h:112 | `void writeCurrentPerFrameMatrixUBO(const PerFrameMatrixUBO& data)` | PerFrame matrix UBO 書込 (3×mat4, 192 B std140) |
| 16 | `writeCurrentTextureMatrixUBO` | llvkloader.h:113 | `void writeCurrentTextureMatrixUBO(const TextureMatrixUBO& data)` | Texture matrix UBO 書込 (4×mat4, 256 B std140) |
| 17 | `pushCurrentModelviewMatrix` | llvkloader.h:120 | `void pushCurrentModelviewMatrix(const float modelview_matrix[16])` | modelview push constant 64 B 投入 |
| 18 | `beginDynamicRendering` | llvkloader.h:151-155 | `void beginDynamicRendering(...)` | VK_KHR_dynamic_rendering 経由 render-pass begin (color/depth attachment 動的指定) |
| 19 | `endDynamicRendering` | llvkloader.h:156 | `void endDynamicRendering()` | dynamic rendering end |
| 20 | `loadSpirvShaderModule` | llvkloader.h:177 | `VkShaderModule loadSpirvShaderModule(const U32* spv_code, size_t code_size_bytes)` | SPIR-V binary から VkShaderModule 生成 |
| 21 | `llGlEnumToVkFormat` | llvkloader.h:208 | `VkFormat llGlEnumToVkFormat(U32 ll_gl_intformat)` | GL internal format → VkFormat 変換 (段階 2-5 移行 helper) |

**特記 (skeleton declaration design 反映項目)**:

- API は **21 件**、namespace LLVKLoader 内 free function 形式
- placeholder draw helper 2 件 (#12 / #13) が段階 3 確定 12 pool 配線 (11 共用 + 1 avatar 専用) を支える
- matrix UBO 書込 (#15 / #16) + push constant 投入 (#17) は **matrix stack 二段構え** (sub-doc 03 §1.5.4 範式) の caller 側 API
- dynamic rendering API (#18 / #19) は VK_KHR_dynamic_rendering 経由 per-pass attachment 切替
- **段階 4 内 skeleton declaration では 21 件全 API を `class LLVKRenderer` static member として thin wrapper 化** (sub-doc 04 caller 配線 = LLVKRenderer 経由 call、namespace LLVKLoader inline 実装は維持)

### §2.3 acceptance #6 metric 整合確認

charter §3 #6 acceptance literal:

```
#6 LLVKRenderer interface skeleton
- metric: 05 doc §10.1-§10.2 hook 配置済、ただし pipeline.cpp 内 inline 実装で動作 (interface 経由 call 化は r41.5)
- test procedure: grep -rE "class LLVKRenderer" indra/ で skeleton declaration 存在 + 05 doc §10.1-§10.2 hook site (LLVKRenderer::* 想定 member) の placeholder 配置済
- regression: skeleton hook と pipeline.cpp inline 実装の signature 整合 (r41.5 で interface 経由 call 化する際に signature 不整合での refactor cost が発生しない)
```

**本 sub-doc 段階 4 内 satisfy 経路**:

| acceptance 構成 | 段階 4 内 satisfy 経路 |
|---|---|
| **metric** (05 doc §10.1-§10.2 hook 配置済) | 段階 3 完遂時点で既達 (handoff §1.3 #6-段階 3、12 pool hook + avatar 専用 hook 物理配置済、namespace LLVKLoader inline 実装で動作中) |
| **test procedure** (`grep -rE "class LLVKRenderer" indra/` で skeleton declaration 存在) | 段階 4 sub-step 4.4 part B で `class LLVKRenderer` declaration 物理配置 = grep hit 1 件以上に変化、本 sub-doc §3 で placement + class outline 詳細化 |
| **test procedure** (05 doc §10.1-§10.2 hook site の placeholder 配置済) | §2.2 inventory の 21 件 API が hook site = `class LLVKRenderer::*` 想定 member の placeholder source、段階 4 内 satisfy 完了時点で **declaration + namespace LLVKLoader inline 実装の signature 整合** で配置 |
| **regression** (signature 整合) | 本 sub-doc §3.2 class outline で 21 件 API を skeleton declaration の static member signature として 1:1 mapping、r41.5 で interface 経由 call 化する際の refactor cost ゼロを構造で保証 |

---

## §3 LLVKRenderer skeleton declaration 設計

### §3.1 placement 戦略

| placement 候補 | 採否 | 理由 |
|---|---|---|
| **新規 file `indra/llrender/llvkrenderer.h` + `llvkrenderer.cpp`** (case A) | **採用** | charter §3 #6 metric「skeleton declaration」と直結、grep `class LLVKRenderer` で hit する形を素直に達成、既存 namespace LLVKLoader の implementation file (`llvkloader.cpp`) と分離して skeleton は宣言のみに集中、r41.5 で interface 経由 call 化する際の refactor 影響範囲が `llvkrenderer.h` 内に閉じる |
| 既存 `llvkloader.h` 内に追記 (case B) | 不採用 | namespace LLVKLoader と class LLVKRenderer を同 file 同梱で命名 / scope 混乱、r41.5 refactor 時に file 切出し再 commit |
| `pipeline.h` 内に追記 (case C) | 不採用 | pipeline.cpp inline 実装 (charter §3 #6 metric) と同一 file 配置で skeleton と implementation の境界が壊れる、charter §2 領域 4 と領域 8 の boundary を spec で確立する目的に反する |

**採用詳細**:

- 新規 `indra/llrender/llvkrenderer.h` : `class LLVKRenderer` declaration のみ (impl 無し)、`llvkloader.h` を include して既存 namespace LLVKLoader 21 件 API の signature 引用
- 新規 `indra/llrender/llvkrenderer.cpp` : `class LLVKRenderer` static member の **thin wrapper 実装**、各 static member は対応する namespace LLVKLoader free function を 1 行で forward (例: `void LLVKRenderer::recordPlaceholderPoolDraw(VkCommandBuffer cmd_buf) { LLVKLoader::recordPlaceholderPoolDraw(cmd_buf); }`)
- **段階 4 内 caller migration は採用しない** (charter §3 #6 metric「pipeline.cpp 内 inline 実装で動作」維持、interface 経由 call 化は r41.5 で実施)
- **CMake / autobuild 配線**: `indra/llrender/CMakeLists.txt` に `llvkrenderer.cpp` + header 追加、既存 `llvkloader.cpp` 配線範式継承
- **autobuild build verify**: 段階 4 sub-step 4.5 self-check 内で `class LLVKRenderer` declaration を含む build PASS 確認 (charter §3 #6 metric satisfy 証跡)

### §3.2 class outline (skeleton declaration、§2.2 inventory 1:1 mapping)

下記は `class LLVKRenderer` skeleton declaration の outline。21 件 API を static member として thin wrapper 化、namespace LLVKLoader inline 実装と signature 1:1 整合。

```cpp
// indra/llrender/llvkrenderer.h (skeleton declaration、段階 4 sub-step 4.4 part B で物理配置)

#ifndef LL_LLVKRENDERER_H
#define LL_LLVKRENDERER_H

#include "llvkloader.h"  // namespace LLVKLoader 21 件 API signature source

// AYAstorm r41 sub-doc 08-llvkrenderer-skeleton §3.2 outline
// charter §3 #6 acceptance: skeleton declaration 物理配置、namespace LLVKLoader inline 実装と signature 整合
// 段階 4 内では declaration のみ、caller migration は r41.5 で実施

class LLVKRenderer
{
public:
    // §2.2 #1-#9 lifecycle / handle
    static bool initVulkan();
    static bool shutdownVulkan();
    static bool isVulkanInitialized();
    static bool isValidationEnabled();
    static bool beginFrame();
    static bool endFrame();
    static VkCommandBuffer getCurrentCommandBuffer();
    static VkDevice getDevice();
    static VkPipelineCache getPipelineCache();

    // §2.2 #10-#11 PSO / layout
    static VkPipelineLayout createStandardPipelineLayout(/* sub-doc 03 §3.3 inline 実装 signature 継承 */);
    static bool compileGraphicsPipeline(const VkGraphicsPipelineCreateInfo& ci, VkPipeline& out_pipeline);

    // §2.2 #12-#13 placeholder draw helper (段階 3 確定 12 pool 配線、段階 4 sub-step 4.3 で実 scene draw へ移植)
    static void recordPlaceholderPoolDraw(VkCommandBuffer cmd_buf);
    static void recordAvatarPlaceholderDraw(VkCommandBuffer cmd_buf);

    // §2.2 #14-#17 PerFrame index / matrix UBO / push constant
    static U32 getCurrentFrameIndex();
    static void writeCurrentPerFrameMatrixUBO(const LLVKLoader::PerFrameMatrixUBO& data);
    static void writeCurrentTextureMatrixUBO(const LLVKLoader::TextureMatrixUBO& data);
    static void pushCurrentModelviewMatrix(const float modelview_matrix[16]);

    // §2.2 #18-#19 VK_KHR_dynamic_rendering
    static void beginDynamicRendering(/* sub-doc 03 + sub-doc 07 inline 実装 signature 継承 */);
    static void endDynamicRendering();

    // §2.2 #20-#21 SPIR-V loader + format converter
    static VkShaderModule loadSpirvShaderModule(const U32* spv_code, size_t code_size_bytes);
    static VkFormat llGlEnumToVkFormat(U32 ll_gl_intformat);
};

#endif  // LL_LLVKRENDERER_H
```

```cpp
// indra/llrender/llvkrenderer.cpp (thin wrapper 実装、段階 4 sub-step 4.4 part B で物理配置)
// charter §3 #6 regression: signature 整合保証、r41.5 interface 経由 call 化での refactor cost ゼロ

#include "llvkrenderer.h"

bool LLVKRenderer::initVulkan() { return LLVKLoader::initVulkan(); }
bool LLVKRenderer::shutdownVulkan() { return LLVKLoader::shutdownVulkan(); }
// ... 21 件 全 forward (1 行 forward × 21 件 = 21 行 thin wrapper)
```

**特記**:

- `PerFrameMatrixUBO` / `TextureMatrixUBO` は **namespace LLVKLoader 内 struct** (llvkloader.h:81-86, 91-96)、skeleton declaration の signature で `LLVKLoader::PerFrameMatrixUBO` qualified 参照 (struct 自体は skeleton に移管しない、r41.5 で判断)
- `createStandardPipelineLayout` / `beginDynamicRendering` の variadic 部分は inline 実装の signature を **literal 継承** (本 sub-doc では `/* … */` で省略、sub-step 4.4 part B 実装時に inline 実装の exact signature を写経)
- **caller migration は段階 4 内では実施しない** (charter §3 #6 metric「pipeline.cpp 内 inline 実装で動作」維持、12 pool hook body 内の `LLVKLoader::recordPlaceholderPoolDraw` 呼出は段階 4 では namespace LLVKLoader 経由のまま)

### §3.3 charter §3 #6 acceptance test procedure satisfy 経路

| test procedure 構成 | 段階 4 sub-step 4.4 part B 内 satisfy 操作 |
|---|---|
| `grep -rE "class LLVKRenderer" indra/` で skeleton declaration 存在 | `llvkrenderer.h` 内 `class LLVKRenderer` 行 1 件で hit、autobuild build PASS で当該 file が configure / build 系に組込確認 |
| 05 doc §10.1-§10.2 hook site の placeholder 配置済 | §2.2 inventory 21 件 API を 1:1 mapping した static member declaration が 21 行配置、namespace LLVKLoader inline 実装と signature 整合 |
| **regression** (signature 整合) | `llvkrenderer.cpp` thin wrapper 実装が 21 件全 forward、compile clean = signature 整合保証、r41.5 で interface 経由 call 化する際の refactor cost ゼロ |

---

## §4 sub-doc 04 LLPipelineFrameContext との配線 (caller 整合)

### §4.1 sub-doc 04 §3 struct outline 引用

sub-doc 04 §3 で導入する `LLPipelineFrameContext` struct の **caller 側 = LLVKRenderer 経由 invocation 想定 (r41.5)**、段階 4 内では pipeline.cpp inline 実装が namespace LLVKLoader を直接 call (charter §3 #6 metric)。

| 段階 | caller / callee 関係 |
|---|---|
| **段階 4 内 (本 sub-doc 範囲)** | `pipeline.cpp` inline 実装 → `namespace LLVKLoader::*` 直接 call、`class LLVKRenderer` skeleton declaration は宣言のみで caller 無し (grep hit 達成 + signature 整合の証跡として配置) |
| **段階 4 完遂後 r41.5** | `pipeline.cpp` (または別 file) → `LLVKRenderer::*` 経由 call、interface placeholder 化、`namespace LLVKLoader` は **internal impl** に格下げ (header から external 削除、`llvkrenderer.cpp` 内 forward 実装でのみ参照) |

### §4.2 sub-doc 04 §3 caller migration 順序との協調

sub-doc 04 §5 sub-step 4.1〜4.5 と本 sub-doc は **sub-step 4.4 part B** で交差:

| sub-step (sub-doc 04 §5) | sub-doc 08 関与 |
|---|---|
| **4.1** PSO 基盤相当 (`LLPipelineFrameContext` struct 配置 + sCull/mRT migration、low-medium risk) | 関与無し (sub-doc 04 単独範囲) |
| **4.2** 軽量 (10 bool flag migration、medium risk) | 関与無し |
| **4.3** 標準 (sCurCameraID accessor + per-pool 実 scene draw 移植、high risk) | 関与無し (placeholder draw helper の caller は段階 4 内では namespace LLVKLoader 経由のまま) |
| **4.4** 特殊 (12 件 RAII dead-store + LLVKRenderer skeleton、high risk) | **本 sub-doc part B** = `class LLVKRenderer` skeleton declaration + thin wrapper 実装 物理配置、12 件 RAII dead-store は sub-doc 04 §4 単独範囲 |
| **4.5** self-check + handoff | 本 sub-doc satisfy 証跡 (`grep -rE "class LLVKRenderer" indra/` hit + autobuild build PASS) を sub-doc 04 §5 self-check で同時確認 |

### §4.3 r41.5 で interface 経由 call 化への前進準備

- **段階 4 完遂時点で残る作業**: caller migration (pipeline.cpp 内 inline 実装の `LLVKLoader::*` 呼出 → `LLVKRenderer::*` 呼出への replace)
- **r41.5 で実施判断**: charter §3 #6 acceptance「interface 経由 call 化は r41.5」literal、段階 4 範囲外
- **段階 4 内で備えた基盤** (本 sub-doc satisfy 後):
  - `class LLVKRenderer` declaration + thin wrapper 実装 (21 件 forward)
  - namespace LLVKLoader inline 実装と signature 整合 (regression 防止)
  - caller migration は **mechanical replace** で完遂可 (sed / IDE find-replace 範囲、refactor cost ゼロ)

---

## §5 sub-step list (sub-doc 04 §5 範式継承、本 sub-doc は part B 配置)

本 sub-doc は **独立 sub-step 8.x を採用しない**、sub-doc 04 §5 sub-step 4.4 内 part B として運用 (charter §2 領域 8 「領域 4 と協調」literal 反映、segment 一体運用)。

| sub-step | 本 sub-doc 配置 |
|---|---|
| **4.4 (特殊)** part A | sub-doc 04 §4 12 件 LLGLState RAII dead-store 化 (sub-doc 04 単独範囲) |
| **4.4 (特殊)** **part B** | **本 sub-doc**: `class LLVKRenderer` skeleton declaration + thin wrapper 実装 物理配置 (`indra/llrender/llvkrenderer.{h,cpp}` 新規 + CMake 配線) |
| **4.5** self-check + handoff | sub-doc 04 §5 + 本 sub-doc 共通 self-check (autobuild build PASS + grep hit + AYA launch verify + handoff-stage-4-complete.md 起草) |

**sub-step 4.4 part B 内訳** (実装 cadence、案 B 2 commit 分割想定):

| commit | 内容 |
|---|---|
| **part B / 1** | `indra/llrender/llvkrenderer.h` 新規 (`class LLVKRenderer` skeleton declaration 21 件 static member) + `indra/llrender/CMakeLists.txt` 追記 + autobuild build clean confirm |
| **part B / 2** | `indra/llrender/llvkrenderer.cpp` 新規 (21 件 forward 実装) + autobuild build clean confirm + `grep -rE "class LLVKRenderer" indra/` hit 1 件以上 confirm + AYA launch verify (段階 3 範式継承、12 pool hook 動作維持 + validation 0 件) |

---

## §6 完遂条件 (acceptance #6 段階 4 内 satisfy + 関連 #1 #3 #5)

### §6.1 charter §3 #6 acceptance 段階 4 内 satisfy 構成

| acceptance 構成 (charter §3 #6 literal) | 段階 4 sub-step 4.4 part B 完遂後の satisfy 証跡 |
|---|---|
| **metric**: 05 doc §10.1-§10.2 hook 配置済、pipeline.cpp 内 inline 実装で動作 | 既達 (段階 3 完遂時点、handoff §1.3 #6-段階 3) + 段階 4 で `class LLVKRenderer` 配置でも inline 実装維持 |
| **test procedure**: `grep -rE "class LLVKRenderer" indra/` で skeleton declaration 存在 | `llvkrenderer.h:近傍` で 1 件 hit (段階 4 sub-step 4.4 part B 完遂後) |
| **test procedure**: 05 doc §10.1-§10.2 hook site (`LLVKRenderer::*` 想定 member) の placeholder 配置済 | §2.2 inventory 21 件 API を §3.2 class outline で 1:1 mapping した static member 21 行配置 |
| **regression**: skeleton hook と pipeline.cpp inline 実装の signature 整合 | §3.2 thin wrapper (21 件 forward) で signature 整合保証、autobuild build clean = compile-level 整合確定 |

### §6.2 関連 acceptance との整合

| 関連 acceptance | 段階 4 sub-step 4.4 part B での影響 |
|---|---|
| **#1 GL 除去完遂** (Linux baseline で OpenGL link 無し) | 本 sub-doc では新規 GL call 追加無し、`llvkrenderer.{h,cpp}` は Vulkan API のみ参照 |
| **#3 段階 1-5 全完遂** (段階別動作確認) | 段階 4 = LLPipelineFrameContext 動作 (sub-doc 04 単独範囲) + skeleton declaration 配置 (本 sub-doc) で段階 4 範囲 satisfy |
| **#5 descriptor set 3 階層** (set=0/1/2 + push descriptor) | 既達 (段階 3 完遂時点、handoff §1.3 #5)、本 sub-doc では skeleton declaration 経由で descriptor API placeholder 配置 (§3.2 #15-#17 + #18-#19) |

### §6.3 regression 0 件保証 (段階 4 sub-step 4.4 part B 完遂後)

- **段階 3 範式継承**: autobuild build clean (compile warning / error 0 件) + AYA launch verify (12 pool hook + avatar 専用 hook 動作維持 + validation 0 件 + #VkRecord# 12 件 hit + #Vulkan# INFO marker baseline +N 想定) + shutdown clean
- **新規 regression**: `class LLVKRenderer` 21 件 static member declaration の追加で既存 namespace LLVKLoader inline 実装 / caller 側 (pipeline.cpp 経由) に影響無し (caller migration は段階 4 範囲外 = r41.5)

---

## §7 関連 doc / memory

### §7.1 関連 sub-doc (cross reference)

| sub-doc | 関係 |
|---|---|
| `00-charter.md` (closed 2026-05-28) | 親 charter、§2 領域 8 + §3 #6 acceptance + §7.4 sub-doc 構成 + §7.5 boundary |
| `03-state-machine-pso.md` (closed 2026-05-31) | 段階 3 = PSO state alias 基盤 + 12 pool hook + matrix stack 二段構え + descriptor set 3 階層 + dynamic rendering 並走基盤、本 sub-doc §2.2 inventory の signature source |
| `04-frame-context.md` (draft 2026-05-31) | 段階 4 領域 4 並走、`LLPipelineFrameContext` struct 配置 + caller migration + 12 件 RAII dead-store 化 (本 sub-doc は sub-step 4.4 part B として交差) |

### §7.2 関連 handoff

| handoff | 関係 |
|---|---|
| `handoff-stage-3-complete.md` (closed 2026-05-31) | 段階 3 完遂 + 段階 4 着手境界 + §1.3 acceptance #6-段階 3 (skeleton declaration 物理配置を段階 4 内 satisfy へ移管) + §5.1 cadence (本 sub-doc 起草指示) |

### §7.3 関連 memory

| memory | 関係 |
|---|---|
| `project_ayastorm_r41_vulkan_migration.md` | r41 milestone Active、段階 3 全完遂 + 段階 4 着手境界、本 sub-doc 起草完了で「次 = 段階 4 sub-step 4.1 + 4.4 part B 着手」へ更新 |
| `feedback_proactive_handoff.md` | コンテキスト圧迫時の handoff 起草 absolute rule、本 sub-doc は handoff-stage-3-complete §5.1 cadence 経由 proactive 起草 |
| `feedback_render_full_trace_first.md` | 描画系は推論禁止・完全 trace 優先、本 sub-doc §2 inventory は Agent B 並列 trace で sealed |
| `feedback_one_step_at_a_time.md` | 1 メッセージ 1 アクション、本 sub-doc 起草は Pattern α 一括 draft → AYA review boundary |

### §7.4 charter §7.4 sub-doc 構成との divergence (本 sub-doc 起草時点)

charter §7.4 sub-doc 構成 (charter:456-464 行):

```
| sub-doc | 内容 | 関連 §2 領域 |
|---|---|---|
| 05-skeleton-interface.md | LLVKRenderer skeleton signature 詳細 + pipeline.cpp 内 inline 実装方針 | §2 領域 8 |
```

**現実の sub-doc 命名**:

- `03-state-machine-pso.md` (段階 3 / 領域 3 PSO + state machine) — charter §7.4 には `03-shader-port.md` = 領域 6
- `04-frame-context.md` (段階 4 / 領域 4 frame context) — charter §7.4 には `04-descriptor-render-pass.md` = 領域 7
- `08-llvkrenderer-skeleton.md` (本 sub-doc、段階 4 領域 8) — charter §7.4 には `05-skeleton-interface.md`

**divergence note** (charter §7.5 「r41 着手中に refine 可な本 charter content」範囲内):

- 番号体系を「sub-doc 通番 (charter §7.4)」 から「番号 = 領域番号」へ refine
- 既存 03 / 04 命名と整合、新規 08 も同範式
- charter §7.4 outline は初期構想、実装中に番号意味論を refine (charter §7.5 boundary 「AYA 確認なしに変更しない本 charter content」= §3 #1-#9 criterion 趣旨 + §1 thesis + §4 plan B trigger + §5 依存 milestone 構成、§7.4 outline は明示外)
- 本 sub-doc AYA review 時に divergence の AYA 承認を求める (charter §7.5 「r41 着手中に refine 可な本 charter content」内 refine としての公式化)

---

## 起草 cadence 完了宣言

- 本 sub-doc Pattern α 一括 draft 起草完了 (2026-05-31)
- AYA review 待ち、PASS 後に段階 4 sub-step 4.1 (sub-doc 04 単独範囲) 着手 + sub-step 4.4 part B (本 sub-doc 範囲) は sub-step 4.4 内で sub-doc 04 part A (12 件 RAII dead-store) と並走実施

**deferred to AYA 擦り合わせ** (本 sub-doc PASS 後 cadence 残):

1. 領域 6 sub-step 6.1 (autobuild Vulkan integration) 本格着手是非
2. 段階 4 sub-step 4.1 着手 GO (`LLPipelineFrameContext` struct 配置 + sCull/mRT migration、low-medium risk)
3. (本 sub-doc divergence note の公式承認)

**commit message draft** (本 sub-doc 起草分):

```
docs(r41): sub-doc 08-llvkrenderer-skeleton.md draft (段階 4 領域 8 LLVKRenderer skeleton declaration 物理配置 + signature 整合、Pattern α 一括 draft、sub-doc 04 並走起草、charter §2 領域 8 0.50 PM 低 risk、§3 #6 acceptance 段階 4 内 satisfy 計画、既存 namespace LLVKLoader 21 件 API を class LLVKRenderer static member thin wrapper 化、§7.4 命名 divergence note 反映、AYA review 待ち、feedback_render_full_trace_first / feedback_one_step_at_a_time / feedback_no_claude_coauthor 遵守) → AYA review boundary
```
