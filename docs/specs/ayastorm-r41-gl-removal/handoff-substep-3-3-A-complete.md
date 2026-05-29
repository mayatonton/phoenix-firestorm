# r41 sub-step 3.3-A 全完遂 → 3.3-B/C 着手境界 handoff (2026-05-29)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-gamma-complete.md` (3.3-γ = placeholder + sky smoke 2 PSO の `VkPipelineLayout` 二段構え準拠化 完遂 → 3.3-δ 着手境界)
**本 handoff 位置付け**: sub-step 3.3-A (matrix stack push constant + UBO 化、α/β-1/β-2/γ/δ-1/δ-2/δ-3 全 sub-step) 完遂宣言 + sub-step 3.3-B (shader port、領域 6 並走) + 3.3-C (FBO → dynamic rendering) 着手境界。3.3-A の per-frame matrix UBO write + modelview push constant 投入の Vulkan path は GL path と並走で動作確認済、ここで「matrix 配信経路の二段構え化」を完遂とする。

---

## 1. sub-step 3.3-A 全完遂 status (2026-05-29)

### 1.1 完遂 marker (sub-doc 03 §3.1.1 sub-step 3.3-A 各 sub-step)

| sub-step | 達成 status |
|---|---|
| **3.3-α** (spec refine) | ✓ sub-doc 03 §1.2 #3 / §3.1 sub-step 3.3 / §3.1.1 (α/β/γ/δ/ε 細分化表) / §3.3 持越し記録 + sub-doc 05 §3.5 訂正 + UI matrix 段階 4 持越し明示 + texture × 3 → × 4 訂正 + 二段構え反映、commit `44b19c507e` |
| **3.3-β-1** (struct + getter signature) | ✓ `PerFrameMatrixUBO` (192 B / std140) + `TextureMatrixUBO` (256 B / std140) C++ struct 定義 + `static_assert` size 確定 + `getPerFrameDescriptorSetLayout()` signature 追加 + spec sealed (sub-doc 05 §3.5)、commit `62778dcac4` |
| **3.3-β-2** (VkBuffer × 3 + descriptor + persistent map) | ✓ UBO buffer × 3 (FRAMES_IN_FLIGHT=3 × 512 B = ~1.5 KB) + VkDescriptorPool + VkDescriptorSet × 3 alloc + persistent map + 初期 zero write + Vulkan WARN/ERR 0 件 + AYA launch PASS、commit `57d72d6976` |
| **3.3-γ** (placeholder + sky smoke 2 PSO layout 二段構え準拠化) | ✓ set=0 = `sPerFrameDescriptorSetLayout` + push constant range = mat4 / 64 B / VERTEX_BIT 反映、2 PSO compile log 維持 + Vulkan WARN/ERR 0 件 + AYA launch PASS、commit `eb6c6f0ac5` |
| **3.3-δ-1** (syncMatrices Vulkan path 並走 + UBO write helper + frame_index counter) | ✓ `writeCurrentPerFrameMatrixUBO()` / `writeCurrentTextureMatrixUBO()` 実装 + `getCurrentFrameIndex()` (FRAMES_IN_FLIGHT=3 ring) + `LLRender::syncMatrices()` Vulkan path 並走 (case X = syncMatrices call timing 採用) + δ-1 marker 2 件 + Vulkan WARN/ERR 0 件 + AYA launch PASS、commit `00fcff6eab` |
| **3.3-δ-2** (modelview push constant + in-frame gating) | ✓ `pushCurrentModelviewMatrix()` 実装 (case P = sInFrame + sCommandBuffer + sPlaceholderLayout gating 採用) + syncMatrices() で MM_MODELVIEW 取得 → `vkCmdPushConstants(VERTEX_BIT, 0, 64)` 投入 + δ-2 marker 1 件 + frame_index=1 (in-frame fire) で in-frame gating 動作実証 + AYA launch PASS、commit `f035815f55` |
| **3.3-δ-3** (verify) | ✓ δ-2 launch cycle 内で同時 satisfy 完了 (独立 sub-step 消費せず δ-2 受入と統合) |
| **3.3-ε** (handoff doc 起草) | ✓ 本 handoff doc 作成 (3.3-A 全完遂総括 → 3.3-B/C 着手境界) |

### 1.2 acceptance evidence (`~/.ayastorm_x64/logs/AYAstorm.log` 2026-05-29 04:30 startup)

| evidence | log line |
|---|---|
| δ-1 PerFrame UBO write path active | `2026-05-29T04:30:33Z INFO #Vulkan# llrender/llvkloader.cpp(1552) writeCurrentPerFrameMatrixUBO : syncMatrices PerFrame UBO write path active (frame_index=0)` |
| δ-1 TextureMatrix UBO write path active | `2026-05-29T04:30:33Z INFO #Vulkan# llrender/llvkloader.cpp(1571) writeCurrentTextureMatrixUBO : syncMatrices TextureMatrix UBO write path active (frame_index=0)` |
| δ-2 modelview push constant path active | `2026-05-29T04:30:34Z INFO #Vulkan# llrender/llvkloader.cpp(1600) pushCurrentModelviewMatrix : syncMatrices modelview push constant path active (frame_index=1, layout=sPlaceholderLayout, range=0..64 B / VERTEX_BIT)` |
| β-2 / γ marker 全維持 | Per-frame descriptor set layout / UBO buffers × 3 / descriptor sets × 3 / Placeholder PSO compiled / Sky smoke PSO compiled の 5 件 全出力 |
| #Vulkan# channel WARN/ERR/failed | 0 hit (release build / validation disabled、transit acceptance) |
| frame_index 推移 | δ-1 (frame_index=0, pre-beginFrame の syncMatrices startup 経由) vs δ-2 (frame_index=1, in-frame の syncMatrices 経由) で beginFrame advance + in-frame gating 動作実証 |
| AYA launch PASS | 起動 → 終了 / regression 0 (login 画面到達 + 視覚 unchanged、AYA 2026-05-29 確認) |

### 1.3 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-3-3-gamma-complete.md` | **役割完了** (3.3-δ 着手 satisfy、本 handoff で 3.3-A 全体引継ぎ) |
| `handoff-substep-3-3-beta-1-complete.md` | 役割完了済 (3.3-β-2 着手 satisfy 済) |
| `handoff-substep-3-3-beta-2-complete.md` | 役割完了済 (3.3-γ 着手 satisfy 済) |
| sub-doc `03-state-machine-pso.md` | active 継続 (§3.1.1 δ-1/δ-2/δ-3/ε 行を「完遂 2026-05-29」化済、3.3-A 全体 close、3.3-B/C 着手 ready) |
| sub-doc `05-vulkan-api-design.md` | active 継続 (β-1 で sealed 済、γ/δ では未変更 = 二段構え仕様は β-1 sealed の通り適用) |
| sub-doc `06-shader-spirv.md` | active 継続 (3.3-B shader port 着手で本格活用) |
| sub-doc `07-descriptor-renderpass.md` | active 継続 (3.3-C FBO → dynamic rendering 着手で本格活用) |
| `handoff-substep-3-3-A-complete.md` (本 handoff) | 新規作成 (3.3-A 全完遂 → 3.3-B/C 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 3.3-A 全完遂 / 3.3-B/C 着手 ready 状態へ update) |

---

## 2. 3.3-A 全体実装 summary

### 2.1 関連 commit (時系列)

| commit | sub-step | scope | 変更規模 |
|---|---|---|---|
| `44b19c507e` | 3.3-α | sub-doc 03/05 spec refine (二段構え + texture × 4 + UI matrix 段階 4 持越し + α/β/γ/δ/ε 細分化) | docs only |
| `62778dcac4` | 3.3-β-1 | `PerFrameMatrixUBO`/`TextureMatrixUBO` struct + `getPerFrameDescriptorSetLayout()` 宣言、build PASS | llvkloader.h + sub-doc 03/05 |
| `09a4563fc5` | 3.3-β-1 docs | β-1 handoff doc | docs only |
| `57d72d6976` | 3.3-β-2 | matrix UBO VkBuffer × 3 + descriptor pool/set alloc + persistent map + AYA launch PASS | llvkloader.cpp (+239 / -1) |
| `bd87c220a7` | 3.3-β-2 docs | β-2 handoff doc | docs only |
| `eb6c6f0ac5` | 3.3-γ | placeholder + sky smoke 2 PSO の VkPipelineLayout 二段構え準拠化 + AYA launch PASS | llvkloader.cpp (+18 / -2) |
| `00f515770a` | 3.3-γ docs | γ handoff doc | docs only |
| `00fcff6eab` | 3.3-δ-1 | syncMatrices Vulkan path 並走 + UBO write helper + frame_index counter + AYA launch PASS | llvkloader.{cpp,h} + llrender.cpp |
| `f035815f55` | 3.3-δ-2 | modelview push constant 並走 + in-frame gating + AYA launch PASS | llvkloader.{cpp,h} + llrender.cpp (+45 / -0) |

### 2.2 file 変更 summary (合計)

| file | 3.3-A 内累計修正 | 主内容 |
|---|---|---|
| `indra/llrender/llvkloader.h` | +43 行 (struct PerFrameMatrixUBO/TextureMatrixUBO + 5 getter/helper signature + comment) | header API surface |
| `indra/llrender/llvkloader.cpp` | +301 / -3 行 (β-2 +239 + γ +18 + δ-1/δ-2 +44) | layout / buffer / descriptor / write helper / push helper 実装 |
| `indra/llrender/llrender.cpp` | +35 / -0 行 (δ-1/δ-2 で `#include "llvkloader.h"` + `#include <cstring>` + syncMatrices() Vulkan path 並走 block) | syncMatrices Vulkan path 並走 (case X = syncMatrices call timing) |
| `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` | §3.1.1 α/β-1/β-2/γ/δ-1/δ-2/δ-3/ε 行 8 件 update | sub-step 細分化 + 完遂 marker |
| `docs/specs/ayastorm-r41-gl-removal/05-vulkan-api-design.md` | §3.5 二段構え sealed | β-1 で sealed、γ/δ では未変更 |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-{β-1,β-2,γ,A}-complete.md` | 各 sub-step 完了境界の handoff doc 4 件 | 段階 boundary 記録 |

### 2.3 主要 API surface (3.3-A 完遂時点、`llvkloader.h` 抜粋)

```cpp
namespace LLVKLoader
{
    // β-1: per-frame matrix UBO struct (std140-equivalent)
    struct PerFrameMatrixUBO {
        float projection_matrix[16];
        float inverse_projection_matrix[16];
        float identity_matrix[16];
    }; // 192 B

    struct TextureMatrixUBO {
        float texture_matrix[4][16];
    }; // 256 B

    // β-1: set=0 descriptor set layout (binding 0 = PerFrameMatrixUBO,
    //       binding 1 = TextureMatrixUBO, stage = VERTEX | FRAGMENT)
    VkDescriptorSetLayout getPerFrameDescriptorSetLayout();

    // δ-1: frame in flight index counter (advanced in beginFrame)
    U32 getCurrentFrameIndex();

    // δ-1: per-frame matrix UBO write helper (no-op if Vulkan uninitialized / unmapped)
    void writeCurrentPerFrameMatrixUBO(const PerFrameMatrixUBO& data);
    void writeCurrentTextureMatrixUBO(const TextureMatrixUBO& data);

    // δ-2: modelview push constant helper (in-frame only, no-op otherwise)
    //      Uses sPlaceholderLayout (γ で push constant range 0..64 B / VERTEX_BIT 準拠化済)
    void pushCurrentModelviewMatrix(const float modelview_matrix[16]);
}
```

### 2.4 主要 implementation 抜粋 (`llrender.cpp` `LLRender::syncMatrices()` Vulkan path 並走)

```cpp
// <AYAstorm r41> sub-step 3.3-δ-1: Vulkan path 並走 - per-frame matrix UBO write
if (LLVKLoader::isVulkanInitialized()) {
    LLVKLoader::PerFrameMatrixUBO perframe = {};
    LLVKLoader::TextureMatrixUBO  texmat   = {};

    const glm::mat4& proj_mat = mMatrix[MM_PROJECTION][mMatIdx[MM_PROJECTION]];
    std::memcpy(perframe.projection_matrix, glm::value_ptr(proj_mat), sizeof(perframe.projection_matrix));

    const glm::mat4 inv_proj = glm::inverse(proj_mat);
    std::memcpy(perframe.inverse_projection_matrix, glm::value_ptr(inv_proj), sizeof(perframe.inverse_projection_matrix));

    const glm::mat4 identity = glm::identity<glm::mat4>();
    std::memcpy(perframe.identity_matrix, glm::value_ptr(identity), sizeof(perframe.identity_matrix));

    for (U32 tex = 0; tex < 4; ++tex) {
        const glm::mat4& tex_mat = mMatrix[MM_TEXTURE0 + tex][mMatIdx[MM_TEXTURE0 + tex]];
        std::memcpy(texmat.texture_matrix[tex], glm::value_ptr(tex_mat), sizeof(texmat.texture_matrix[tex]));
    }

    LLVKLoader::writeCurrentPerFrameMatrixUBO(perframe);
    LLVKLoader::writeCurrentTextureMatrixUBO(texmat);

    // <AYAstorm r41> sub-step 3.3-δ-2: modelview push constant 並走投入
    const glm::mat4& modelview_mat = mMatrix[MM_MODELVIEW][mMatIdx[MM_MODELVIEW]];
    LLVKLoader::pushCurrentModelviewMatrix(glm::value_ptr(modelview_mat));
}
// </AYAstorm r41>
```

---

## 3. 設計判断履歴 (3.3-A 全体で AYA 確認した分岐)

### 3.1 3.3-A 細分化 (sub-doc 03 §3.1.1)

AYA 指示 (2026-05-29「段階を分けて安全に」「粒度過大化防止」) で α/β-1/β-2/γ/δ/ε の 6 sub-step に分割。各 sub-step は完遂境界で能動 handoff timing チェック (`feedback_proactive_handoff.md`)。

### 3.2 β-1: 二段構え採用 (push constant 64 B + per-frame UBO 2 binding)

- push constant 64 B 単独では 9 種 uniform 収まらない
- → push constant = modelview_matrix 64 B / UBO binding 0 = projection 系 3 mat4 192 B / UBO binding 1 = texture × 4 256 B
- MVP / normal_matrix / inverse_modelview は vertex shader 内計算で吸収 (3.3-B 範疇)
- 合計 push constant 64 B + UBO 448 B

### 3.3 β-2: descriptor set 3-way duplication (FRAMES_IN_FLIGHT=3)

- frame in flight 同時 update 防止のため VkBuffer / VkDescriptorSet を 3 個に複製
- HOST_VISIBLE_COHERENT memory + persistent mapping で flush 不要
- 256 B align (PerFrame 192 B @ offset 0, Texture 256 B @ offset 256) で alignment 安全側

### 3.4 γ: 案 A (γ = 1 commit) 採用

- placeholder + sky smoke 2 PSO の layout 差替えはほぼ同一作業の対称
- commit 分離による review 価値が低い、案 A (1 commit) で AYA 承認

### 3.5 δ subdivision (δ-1/δ-2/δ-3): 案 B 採用

- AYA 確認: 案 A (1 commit) vs 案 B (δ-1/δ-2/δ-3 split)
- → 案 B 採用、UBO write (データ側) と push constant (構造側) を分離、δ-3 を verify 独立化
- 結果: δ-3 は δ-2 受入 cycle 内で satisfy 完了、独立 sub-step として消費せず

### 3.6 δ-1: case X (syncMatrices call timing) 採用

- AYA 確認: case X (syncMatrices 呼出時に UBO write) vs case Y (per-frame command buffer 開始時に UBO write)
- → case X 採用、GL path の syncMatrices uniform 配信と mirror symmetry
- frame in flight index counter は beginFrame で advance、δ-1 UBO write + δ-2 push が同 index 共有

### 3.7 δ-2: case P (gating via sInFrame + sCommandBuffer) 採用

- AYA 確認: case P (gating) vs case Q (ad-hoc dummy cmd_buf) vs case R (wire beginFrame to real frame entry)
- → case P 採用、最小改変で sInFrame + sCommandBuffer + sPlaceholderLayout を gating
- **当初 case P 説明での「現状 beginFrame 未配線」は誤り**、実 `llappviewer.cpp:1781` で `LLVKLoader::beginFrame()` / `endFrame()` は display() 前後に既に配線済 (γ で wire 済を見落とし)
- 結果: δ-2 push は実 frame で fire (frame_index=1)、in-frame gating は実 frame で動作実証 (transit smoke 経路に限定されず)

### 3.8 δ-2 layout 流用判断

- `sPlaceholderLayout` を流用 (γ で push constant range 0..64 B / VERTEX_BIT 準拠化済)
- δ-2 時点では in-frame で bind 済の PSO layout が sPlaceholderLayout のみ (beginFrame で sPlaceholderPipeline bind)
- → push constant 投入の layout 引数として sPlaceholderLayout が valid
- 将来 実 draw call 経路で別 PSO bind される際は、その PSO 由来 layout を共有/参照する別経路設計が必要 (3.3-B/C 後続 sub-step 範疇)

---

## 4. risks / caveats (3.3-A 完遂後の引継ぎ)

### 4.1 実 draw call 経路への PSO bind / descriptor set bind / push 投入統合は未成立 (3.3-B/C scope)

3.3-A は **GL path 並走の matrix 配信経路** までを成立させた。`vkCmdBindPipeline` (実 draw 用 PSO) → `vkCmdBindDescriptorSets` (set=0 per-frame matrix UBO) → `vkCmdPushConstants` (modelview) → `vkCmdDraw*` という per-draw 経路の組立は **3.3-B (shader port、領域 6 並走) + 3.3-C (FBO → dynamic rendering) + 3.4 (12 pool hook body 配線) で順次実現**。3.3-A 時点では:

- UBO write は syncMatrices() 呼出毎に走る (GL path uniform 配信と同時刻、Vulkan 側は persistent map へ memcpy)
- push constant は in-frame の syncMatrices() 呼出時のみ sPlaceholderLayout 向けに投入 (frame 中 1 度の placeholder PSO bind に対する push、実 draw 経路への波及は無し)
- 実描画は依然 GL path が担当

### 4.2 placeholder layout 流用は γ/δ-2 限定の便宜

`sPlaceholderLayout` の push constant range 0..64 B / VERTEX_BIT は γ で二段構え準拠化済だが、これは「初期 in-frame 経路の便宜的な layout 供給源」であり、**3.3-B 以降の実 draw PSO は各自 layout を持つ**。δ-2 の `pushCurrentModelviewMatrix()` は sPlaceholderLayout 引数固定のため、実 draw 経路の push 投入には **PSO 由来 layout を渡す別 helper / overload** が必要。

### 4.3 syncMatrices() 全 caller で δ-1/δ-2 並走 fire

`LLRender::syncMatrices()` の Vulkan path 並走 block は `if (shader)` 内に配置、全 syncMatrices() call で fire。これは sub-doc 03 §3.5 「GL path 動作維持」に整合 (Vulkan path は並走で副作用無し)、ただし **per-frame matrix UBO の write 頻度は GL path の uniform 配信頻度 = shader bind 毎** となるため、frame in flight ring と整合性が崩れる懸念は無い (frame 中の最後の write 値が GPU 読み出し時の値となる、std140 の coherent write は確定)。

### 4.4 identity_matrix 残置可否

`PerFrameMatrixUBO::identity_matrix` (定数 mat4) は β-1 で「3.3-B shader trace で削除可否再判断」として残置決定済 (β-1 handoff §4.3 引継ぎ)。3.3-B shader port で shader 内に identity 直書きできる場合は UBO から外して 64 B 削減候補。

### 4.5 validation strict 確認は sub-step 3.5 に持越し

3.3-A 全体を通して release build (validation = disabled) で transit acceptance。validation strict (extra resources warn / descriptor binding mismatch / push constant range mismatch 等の厳密検証) は sub-doc 03 §3.5 (段階 3 self-check) で別 build により実施。

### 4.6 LLRender::mUIOffset / mUIScale (UI matrix std::vector stack) は 3.3-A scope 外

3.3-α で確定済の通り、UI matrix は syncMatrices scope 外で独立管理、段階 4 frame context refactor 持越し (sub-doc 03 §3.3 末尾参照)。3.3-A 完遂は UI matrix 持越しと両立。

---

## 5. next session entry point

### 5.1 次 session 着手前の準備

1. `git pull origin feature/ayastorm-r41-gl-removal` (AYA push 後の最新取得)
2. 本 handoff doc 通読
3. sub-doc 03 §3.1 sub-step list 3.3 行 (3.3-A 完遂後の 3.3-B/C 別タイムライン) 確認
4. sub-doc 06 (shader SPIR-V) / sub-doc 07 (descriptor / render pass) の scope plan 再確認
5. memory `project_ayastorm_r41_vulkan_migration.md` status update 反映確認

### 5.2 3.3-B / 3.3-C 着手 scope (sub-doc 03 §3.1 sub-step 3.3 + 別 sub-doc)

| sub-step | 内容 | 想定対象 file | 関連 sub-doc |
|---|---|---|---|
| **3.3-B** | shader SPIR-V port (領域 6 並走、AYAstorm 改変 13 file は touch しない) — GL shader 248 file → SPIR-V 化、UBO binding 整合、shader 内計算 (MVP/normal/inverse_modelview) 配線 | `indra/newview/app_settings/shaders/**` SPIR-V 並走 + shader load path 改修 | `06-shader-spirv.md` |
| **3.3-C** | FBO → dynamic rendering (`VK_KHR_dynamic_rendering`) 化 | `indra/llrender/llrendertarget.{cpp,h}` (783 LOC) | `07-descriptor-renderpass.md` §X (TBD) |

3.3-B は領域 6 SPIR-V port と並走着手候補 (Agent 並列化推奨)。3.3-C は llrendertarget.{cpp,h} 単体で independent。

### 5.3 着手前の AYA 確認候補 (推奨 1 件)

- **3.3-B / 3.3-C どちらを先行するか** (両者 dependency graph 上 independent、並列着手も可)
  - 案 1: 3.3-B 先行 (shader port が完了すると 3.3-A 並走経路を実 PSO bind に統合可能)
  - 案 2: 3.3-C 先行 (FBO → dynamic rendering を先に完了すると 3.4 全 pool hook body 配線時の render pass 経路が確定)
  - 案 3: 並列着手 (Agent 並列 cluster で各 sub-step 細分化を並走起草)
- **3.3-B 細分化案** (B-1 = shader load path 改修 / B-2 = SPIR-V 並走配信 / B-3 = UBO/push constant binding 整合 / B-4 = AYAstorm 改変 13 file 領域 7 並走 trace 等)
- **3.3-C 細分化案** (C-1 = VkRenderingAttachmentInfo 構築 / C-2 = vkCmdBeginRenderingKHR/vkCmdEndRenderingKHR hook / C-3 = depth/stencil attachment 配線 等)

### 5.4 critical reminders

- **AYAstorm 改変 13 file shader 改変禁止** (charter §2.1 領域 6、`git diff` 0 件維持)
- **段階 1 + 段階 2 + 3.1b + 3.2 + 3.3-A 動作維持** (sub-doc 03 §3.5、Vulkan path 並走で GL 描画動作維持)
- **3.3-A 完遂時点で matrix 配信経路は両方向動作確認済**、3.3-B/C 着手時に GL path 残置必須 (sub-doc 03 §3.5、視覚 regression 0 維持)
- **identity_matrix 残置可否は 3.3-B shader trace で再判断** (β-1 引継ぎ、3.3-B 完遂時に決着)
- **validation strict は sub-step 3.5 で別 build により実施** (3.3-B/C 着手時も release build で AYA launch + WARN/ERR 0 件 transit acceptance)
- **3.3-A scope 外の持越し**: UI matrix (mUIOffset/Scale → 段階 4 frame context refactor)、`identity_matrix` 残置可否 (3.3-B)、実 draw 経路 PSO bind / descriptor set bind / push 投入統合 (3.3-B/C/3.4 順次)、validation strict (3.5)、PFNGL function pointer declarations 物理削除 (段階 5)、RAII setter 内 GL call dead-store 化 (段階 4)

---

## 6. 関連 doc / memory cross-ref

### 6.1 関連 doc

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter (3.3-A 完遂で charter §3 #5/#6 段階 3 分の matrix 配信 facet satisfy)
- `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` — 段階 3 sub-doc (本 handoff で sub-step 3.3-A 全完遂 + 3.3-B/C 着手 ready)
- `docs/specs/ayastorm-r41-gl-removal/05-vulkan-api-design.md` — Vulkan API 設計 (§3.5 二段構え、β-1 sealed)
- `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` — shader SPIR-V scope plan (3.3-B 着手時に本格活用)
- `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` — descriptor / render pass scope plan (3.3-C 着手時に本格活用)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-gamma-complete.md` — 直前完了 handoff (役割完了)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-beta-2-complete.md` — β-2 完了 handoff (役割完了済)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-beta-1-complete.md` — β-1 完了 handoff (役割完了済)

### 6.2 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 handoff 完遂で sub-step 3.3-A 全完遂 / 3.3-B/C 着手 ready 状態に update)
- `project_ayastorm_three_platforms.md` — Linux 先行例外を r41 で適用中
- `feedback_self_verify_before_handoff.md` — 本 handoff 起草前 self-trace 実施 (§1.1 acceptance × log evidence 各 line 突合、commit hash × sub-step mapping 確認、配線整合確認)
- `feedback_one_step_at_a_time.md` — 3.3-A 全 sub-step で AYA 1 件確認原則遵守 (γ 案 A / δ 案 B / δ-1 case X / δ-2 case P / ε 案 B)
- `feedback_no_auto_commit.md` — 本 handoff doc commit は本 session 内で実施 (AYA 案 B 明示指示済)
- `feedback_proactive_handoff.md` — sub-step 3.3-A 全完遂境界での能動 handoff 起草 (本 file)
- `feedback_log_reading.md` — AYA launch 後 Claude が `~/.ayastorm_x64/logs/AYAstorm.log` を grep して δ-1/δ-2 marker + Vulkan WARN/ERR 0 件確認
- `feedback_no_scope_shrink.md` — 「段階を踏む」が scope 縮小許可ではないことを 3.3-A 全 sub-step で遵守 (各 sub-step で完全 satisfy)
