# r41 sub-step 3.3-C 全完遂 → 3.3-B 着手境界 handoff (2026-05-29)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-A-complete.md` (3.3-A = matrix stack push constant + UBO 化 全完遂 → 3.3-B/C 着手境界)
**本 handoff 位置付け**: sub-step 3.3-C (FBO → dynamic rendering、α/β-1/β-2/γ/δ 全 sub-step) 完遂宣言 + sub-step 3.3-B (shader port、領域 6 並走) 着手境界。3.3-C の LLRenderTarget::bindTarget / flush Vulkan path 並走は GL path と並走で動作確認済、ここで「FBO API surface 並走化」を完遂とする。実 attachment 配線 (`VkImage` / `VkImageView` 実体作成) は領域 7 sub-step 7.5 持越し (sub-doc 07 §1.2.3 と整合)。

---

## 1. sub-step 3.3-C 全完遂 status (2026-05-29)

### 1.1 完遂 marker (sub-doc 03 §3.1.2 sub-step 3.3-C 各 sub-step)

| sub-step | 達成 status |
|---|---|
| **3.3-C-α** (spec refine) | ✓ sub-doc 03 §3.1.2 新規追加 (6 sub-step 細分化 α/β-1/β-2/γ/δ/ε + scope boundary + 設計根拠 trace inventory) + §3.1 sub-step 3.3 行 update + §3.3 持越し記録追記 + sub-doc 07 §1.2.3 boundary 明確化、commit `b798d27265` |
| **3.3-C-β-1** (struct + helper signature) | ✓ `DynamicRenderingAttachment` struct (`image_view` nullable / `image_layout` / `load_op` / `store_op` / `clear_value` 5 field) + `beginDynamicRendering()` / `endDynamicRendering()` signature 追加 (llvkloader.h +33 行) + 案 A 採用 sealed (color = pointer + count、depth = nullable pointer、command buffer は internal `sCommandBuffer` 経由 δ-2 case P gating pattern 継承) + build PASS、commit `3f8f1e2a1a` |
| **3.3-C-β-2** (helper body + Vulkan 1.3 feature enable + transit smoke) | ✓ `sInDynamicRendering` 静的 flag (begin/end 対称 pair 保証) + `VkPhysicalDeviceDynamicRenderingFeatures` query + enable chain (Vulkan 1.3 core feature 有効化) + INFO marker 2 件 (init 時 `Vulkan 1.3 dynamicRendering feature enabled` + in-frame 1 回 one-shot `beginDynamicRendering : dynamic rendering helper path active`) + helper body 実装 + `beginFrame()` 内 transit smoke 呼出 (`beginDynamicRendering(0, 0, nullptr, 0, nullptr); endDynamicRendering();`) + AYA launch PASS、impl commit `85b3a9a513` / 完遂 commit `02ee477c1c` |
| **3.3-C-γ** (LLRenderTarget::bindTarget Vulkan 並走) | ✓ `#include "llvkloader.h"` 追加 + `bindTarget()` 末尾 Vulkan path block (`isVulkanInitialized()` gate + one-shot marker `"bindTarget dynamic rendering begin path active"` + `DynamicRenderingAttachment` 配列構築 [color × `mTex.size()` clamped to 4 / depth × `mUseDepth` 条件] + 全 `image_view=VK_NULL_HANDLE` で helper 側 null-skip path = `sInDynamicRendering=false` 維持 = δ end pair 対称性) + AYA launch PASS、impl commit `8ed9edd23b` / 完遂 commit `3c3f097ef4` |
| **3.3-C-δ** (LLRenderTarget::flush Vulkan 並走) | ✓ `flush()` 内 `gGL.flush()` 直後 + `mPreviousRT` 再帰の前に Vulkan path block (begin/end pair 1:1 整合性保証) + one-shot marker `"flush dynamic rendering end path active"` + `LLVKLoader::endDynamicRendering()` 呼出 + γ marker line 151 / δ marker line 152 隣接配置で pair 動作確認 + AYA launch PASS、impl commit `da01900f46` / 完遂 commit `f2cdc6fcfb` |
| **3.3-C-ε** (handoff doc 起草) | ✓ 本 handoff doc 作成 (3.3-C 全完遂総括 → 3.3-B 着手境界) |

### 1.2 acceptance evidence (`~/.ayastorm_x64/logs/AYAstorm.log` 2026-05-29 05:34 startup)

| evidence | log line |
|---|---|
| β-2 Vulkan 1.3 feature enable | line 98 `2026-05-29T05:34:16Z INFO #Vulkan# llrender/llvkloader.cpp(471) createDevice : Vulkan 1.3 dynamicRendering feature enabled (LLRenderTarget bindTarget/flush Vulkan path 並走基盤、sub-step 3.3-C-β-2)` |
| γ bindTarget dynamic rendering begin path | line 151 `2026-05-29T05:34:17Z INFO #Vulkan# llrender/llrendertarget.cpp(466) bindTarget : LLRenderTarget::bindTarget : bindTarget dynamic rendering begin path active (sub-step 3.3-C-γ API surface 並走、placeholder attachment image_view=VK_NULL_HANDLE / 実 attachment 配線は領域 7 sub-step 7.5 移管)` |
| δ flush dynamic rendering end path | line 152 `2026-05-29T05:34:17Z INFO #Vulkan# llrender/llrendertarget.cpp(566) flush : LLRenderTarget::flush : flush dynamic rendering end path active (sub-step 3.3-C-δ API surface 並走、endDynamicRendering 呼出 / γ begin と pair 対称 / 実 vkCmdEndRendering は sInDynamicRendering guard で no-op = 領域 7 sub-step 7.5 実 attachment 配線後に発火)` |
| β-2 dynamic rendering helper path | line 1000 `2026-05-29T05:34:18Z INFO #Vulkan# llrender/llvkloader.cpp(1696) beginDynamicRendering : beginDynamicRendering : dynamic rendering helper path active (Vulkan 1.3 dynamicRendering / in-frame guard PASS / β-2 transit smoke = all-null args no-op early return)` |
| 3.3-A 全 marker 維持 | placeholder PSO compiled / sky smoke PSO compiled / per-frame desc layout / per-frame UBO buffers × 3 / per-frame desc sets × 3 / writeCurrentPerFrameMatrixUBO / writeCurrentTextureMatrixUBO / pushCurrentModelviewMatrix の 8 件 全出力 |
| #Vulkan# channel WARN/ERR/failed | 0 hit (release build / validation disabled、transit acceptance、INFO 35 件全て正常) |
| γ/δ pair 動作確認 | γ line 151 → δ line 152 隣接配置 = γ bindTarget 直後の最初の flush で δ marker 即発火 = begin/end pair 1:1 整合性 (sub-doc 03 §3.1.2 設計と一致) |
| AYA launch PASS | 起動 → ログイン → ワールド → 終了 / regression 0 / shutdownVulkan clean (Vulkan device destroyed → Vulkan instance destroyed) (AYA 2026-05-29 確認) |

### 1.3 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-3-3-A-complete.md` | **役割完了** (3.3-C 着手 satisfy、本 handoff で 3.3-C 全体引継ぎ) |
| sub-doc `03-state-machine-pso.md` | active 継続 (§3.1.2 α/β-1/β-2/γ/δ 行を「完遂 2026-05-29」化済、3.3-C 全体 close、3.3-B 着手 ready) |
| sub-doc `05-vulkan-api-design.md` | active 継続 (β-1 で sealed 済、3.3-C では未変更) |
| sub-doc `06-shader-spirv.md` | active 継続 (3.3-B shader port 着手で本格活用) |
| sub-doc `07-descriptor-renderpass.md` | active 継続 (3.3-C-α で §1.2.3 boundary 明確化済、実 attachment 配線は sub-step 7.5 で本配線) |
| `handoff-substep-3-3-C-complete.md` (本 handoff) | 新規作成 (3.3-C 全完遂 → 3.3-B 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 3.3-C 全完遂 / 3.3-B 着手 ready 状態へ update) |

---

## 2. 3.3-C 全体実装 summary

### 2.1 関連 commit (時系列)

| commit | sub-step | scope | 変更規模 |
|---|---|---|---|
| `b798d27265` | 3.3-C-α | sub-doc 03 §3.1.2 新規 + §3.1 行 update + §3.3 持越し + sub-doc 07 §1.2.3 boundary 明確化 | docs only |
| `3f8f1e2a1a` | 3.3-C-β-1 | `DynamicRenderingAttachment` struct + 2 helper signature 追加、build PASS | llvkloader.h (+33 行) |
| `85b3a9a513` | 3.3-C-β-2 (impl) | helper body + `sInDynamicRendering` flag + Vulkan 1.3 feature query/enable chain + transit smoke + 2 marker | llvkloader.cpp (+145 行) + sub-doc 03 §3.1.2 |
| `02ee477c1c` | 3.3-C-β-2 完遂 | β-2 marker update (AYA launch verify PASS) | docs only |
| `8ed9edd23b` | 3.3-C-γ (impl) | `LLRenderTarget::bindTarget()` Vulkan path 並走 + 1 marker | llrendertarget.cpp (+46 行) + sub-doc 03 §3.1.2 |
| `3c3f097ef4` | 3.3-C-γ 完遂 | γ marker update (AYA launch verify PASS) | docs only |
| `da01900f46` | 3.3-C-δ (impl) | `LLRenderTarget::flush()` Vulkan path 並走 + 1 marker | llrendertarget.cpp (+25 行) + sub-doc 03 §3.1.2 |
| `f2cdc6fcfb` | 3.3-C-δ 完遂 | δ marker update (AYA launch verify PASS) | docs only |

### 2.2 file 変更 summary (合計)

| file | 3.3-C 内累計修正 | 主内容 |
|---|---|---|
| `indra/llrender/llvkloader.h` | +33 行 (`DynamicRenderingAttachment` struct + `beginDynamicRendering`/`endDynamicRendering` signature + comment) | dynamic rendering API surface |
| `indra/llrender/llvkloader.cpp` | +145 行 (β-2: helper body + `sInDynamicRendering` flag + `VkPhysicalDeviceDynamicRenderingFeatures` query/enable chain + transit smoke + 2 marker) | dynamic rendering helper 本体 + Vulkan 1.3 feature 有効化 |
| `indra/llrender/llrendertarget.cpp` | +71 行 (γ: bindTarget Vulkan path 並走 +46、δ: flush Vulkan path 並走 +25) | API surface 並走化 (bindTarget → beginDynamicRendering、flush → endDynamicRendering) |
| `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` | §3.1 sub-step 3.3 行 update + §3.1.2 新規 (6 sub-step 細分化 + scope boundary + 設計根拠 trace inventory) + §3.3 持越し記録追記 + §3.1.2 α/β-1/β-2/γ/δ 行 5 件 update | sub-step 細分化 + 完遂 marker |
| `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` | §1.2.3 boundary 明確化 (3.3-C との分担: API surface = 3.3-C / 実 attachment = 7.5) | scope boundary 明示 |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-C-complete.md` | 新規作成 (本 handoff) | 段階 boundary 記録 |

### 2.3 主要 API surface (3.3-C 完遂時点、`llvkloader.h` 抜粋)

```cpp
namespace LLVKLoader
{
    // β-1: dynamic rendering attachment 1 件記述 (VkRenderingAttachmentInfo の wrap)
    struct DynamicRenderingAttachment
    {
        VkImageView         image_view;     // VK_NULL_HANDLE 可 (helper 側で skip)
        VkImageLayout       image_layout;   // 通例 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL 等
        VkAttachmentLoadOp  load_op;        // VK_ATTACHMENT_LOAD_OP_CLEAR / LOAD / DONT_CARE
        VkAttachmentStoreOp store_op;       // VK_ATTACHMENT_STORE_OP_STORE / DONT_CARE
        VkClearValue        clear_value;    // load_op == CLEAR の時のみ参照
    };

    // β-2: dynamic rendering 開始 / 終了 helper (δ-2 case P gating pattern 継承)
    // in-frame (beginFrame...endFrame 間) かつ Vulkan 初期化済 + sCommandBuffer 有効時のみ
    // vkCmdBeginRendering / vkCmdEndRendering を発行、それ以外 no-op
    void beginDynamicRendering(U32                               width,
                               U32                               height,
                               const DynamicRenderingAttachment* color_attachments,
                               U32                               color_count,
                               const DynamicRenderingAttachment* depth_attachment);
    void endDynamicRendering();
}
```

### 2.4 主要 implementation 抜粋 (`llrendertarget.cpp` LLRenderTarget::bindTarget / flush Vulkan path 並走)

```cpp
// r41 sub-step 3.3-C-γ: Vulkan path 並走 (sub-doc 03 §3.1.2 / sub-doc 07 §1.2.3 boundary)
// 実 VkImageView は領域 7 sub-step 7.5 移管 (VMA = sub-step 7.1 前提)
// γ は API surface 並走化 = placeholder attachment (image_view=VK_NULL_HANDLE) 提供のみ
if (LLVKLoader::isVulkanInitialized())
{
    static bool s_first_bind_active = true;
    if (s_first_bind_active) {
        s_first_bind_active = false;
        LL_INFOS("Vulkan") << "LLRenderTarget::bindTarget : bindTarget dynamic rendering "
                              "begin path active (...)" << LL_ENDL;
    }

    LLVKLoader::DynamicRenderingAttachment color_attachments[4] = {};
    U32 color_count = static_cast<U32>(mTex.size() < 4 ? mTex.size() : 4);
    for (U32 i = 0; i < color_count; ++i) {
        color_attachments[i].image_view   = VK_NULL_HANDLE;  // 領域 7 sub-step 7.5 移管
        color_attachments[i].image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachments[i].load_op      = VK_ATTACHMENT_LOAD_OP_LOAD;
        color_attachments[i].store_op     = VK_ATTACHMENT_STORE_OP_STORE;
    }

    LLVKLoader::DynamicRenderingAttachment depth_attachment = {};
    depth_attachment.image_view   = VK_NULL_HANDLE;  // 領域 7 sub-step 7.5 移管
    depth_attachment.image_layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth_attachment.load_op      = VK_ATTACHMENT_LOAD_OP_LOAD;
    depth_attachment.store_op     = VK_ATTACHMENT_STORE_OP_STORE;

    LLVKLoader::beginDynamicRendering(
        mResX, mResY,
        color_count > 0 ? color_attachments : nullptr,
        color_count,
        mUseDepth ? &depth_attachment : nullptr);
}

// r41 sub-step 3.3-C-δ: flush 側 Vulkan path 並走
// γ bindTarget の beginDynamicRendering と pair 対称、mPreviousRT 再帰の前に end を完了
if (LLVKLoader::isVulkanInitialized())
{
    static bool s_first_flush_active = true;
    if (s_first_flush_active) {
        s_first_flush_active = false;
        LL_INFOS("Vulkan") << "LLRenderTarget::flush : flush dynamic rendering end path "
                              "active (...)" << LL_ENDL;
    }
    LLVKLoader::endDynamicRendering();
}
```

---

## 3. 設計判断履歴 (3.3-C 全体で AYA 確認した分岐)

### 3.1 3.3-C 細分化 (案 A = 6 sub-step、AYA 承認 2026-05-29)

3.3-A pattern (α/β-1/β-2/γ/δ/ε) 継承で 6 sub-step に分割。各 sub-step は完遂境界で能動 handoff timing チェック (`feedback_proactive_handoff.md`)。

### 3.2 β-1 案 A 採用 (struct + helper signature の 5 field 構成、AYA 承認 2026-05-29)

`DynamicRenderingAttachment` を以下の 5 field で sealed:
- `image_view` (nullable、β-2 transit smoke / 領域 7 sub-step 7.5 前は VK_NULL_HANDLE)
- `image_layout`
- `load_op`
- `store_op`
- `clear_value`

helper signature は: color = pointer + count (≤4)、depth = nullable pointer、width/height 別引数、command buffer は internal `sCommandBuffer` 経由 (δ-2 case P gating pattern 継承)。

### 3.3 β-2 scope boundary: API surface 並走化のみ、実 attachment は領域 7 sub-step 7.5 へ移管

3.3-C scope は `bindTarget` → `vkCmdBeginRendering` / `flush` → `vkCmdEndRendering` の wrap helper + call surface 並走のみ。`VkImage` / `VkImageView` 実体作成は VMA = 領域 7 sub-step 7.1 前提のため、領域 7 sub-step 7.5 持越し (sub-doc 07 §1.2.3 と整合)。3.3-C 完遂時の Vulkan path 並走は placeholder attachment (`image_view=VK_NULL_HANDLE`) による transit smoke。

### 3.4 β-2 Vulkan 1.3 dynamicRendering feature enable 必須

`vkCmdBeginRendering` / `vkCmdEndRendering` は Vulkan 1.3 core で promoted (旧 `VK_KHR_dynamic_rendering` extension)、`VkPhysicalDeviceDynamicRenderingFeatures` feature enable が必須。`createDevice` 内で `vkGetPhysicalDeviceFeatures2` query → `dr_features_enable.dynamicRendering = VK_TRUE` → `device_info.pNext` chain で有効化。未 support 時は WARN (Vulkan 1.3 spec violation case)。

### 3.5 β-2 begin/end pair 対称性: `sInDynamicRendering` 静的 flag

`vkCmdEndRendering` の独立発火は validation error (matching begin 必須)。これを `sInDynamicRendering` 静的 flag で gating:
- `beginDynamicRendering` 実 `vkCmdBeginRendering` 発火時のみ true、null-skip path no-op return 時は false 維持
- `endDynamicRendering` は `sInDynamicRendering` guard で no-op (begin 未発火時は対称で skip)

これにより 3.3-C-γ/δ 並走で placeholder attachment (null view) に当てている間も pair 整合性違反が発生しない。

### 3.6 γ/δ: API surface 並走化 placement

- γ: `bindTarget()` 末尾 (GL 既存 logic 完了後、`sBoundTarget = this` の後) に Vulkan path block 配置 = GL bind 状態確立後に Vulkan begin
- δ: `flush()` 内 `gGL.flush()` 直後 + `mPreviousRT` 再帰の前 + `mGenerateMipMaps` 操作の前に Vulkan path block 配置 = mPreviousRT 経由の次の begin が発行される前に現 RT の end を完了 = begin/end ペア 1:1 整合性

### 3.7 transit smoke acceptance: β-2 で `beginFrame()` 内 全 null 呼出

β-2 で `beginFrame()` 内 `sInFrame = true` 直後に `beginDynamicRendering(0, 0, nullptr, 0, nullptr); endDynamicRendering();` を配置。これにより:
- in-frame guard 通過確認 (case P gating 動作実証)
- 全 view null path で `sInDynamicRendering = false` 維持 (legacy render pass `vkCmdBeginRenderPass` 内でも safe)
- 領域 7 sub-step 7.5 で実 attachment 配線後は同 path が `vkCmdBeginRendering` 発火経路に変化

---

## 4. risks / caveats (3.3-C 完遂後の引継ぎ)

### 4.1 実 attachment 配線は領域 7 sub-step 7.5 持越し

3.3-C は **API surface 並走化** までを成立させた。`VkImage` / `VkImageView` 実体作成 + `vkCmdBeginRendering` 実発火 + 描画 / present 経路統合は **領域 7 sub-step 7.1 (VMA) → 7.5 (実 attachment 配線) で順次実現**。3.3-C 時点では:

- `bindTarget` Vulkan path = placeholder attachment 提供 (全 `image_view=VK_NULL_HANDLE`) → helper 側 null-skip path で no-op
- `flush` Vulkan path = `endDynamicRendering` 呼出 → helper 側 `sInDynamicRendering=false` 維持で no-op
- 実描画は依然 GL path が担当 (sub-doc 03 §3.5 「GL path 動作維持」)

### 4.2 placeholder attachment の layout/load_op/store_op は仮値

γ で配置した `DynamicRenderingAttachment` の `image_layout` (`VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL` / `VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL`) と `load_op=LOAD` / `store_op=STORE` は **領域 7 sub-step 7.5 着手時に実 attachment の usage pattern (clear / load / discard) に応じて再選定** する。clear が必要な call site (例: depth-only pre-pass) では `load_op=CLEAR` + `clear_value` 設定が必要。

### 4.3 `mGenerateMipMaps` GL_TEXTURE_2D mipmap 生成は Vulkan 未対応

`flush()` 内 `mGenerateMipMaps == LLTexUnit::TMG_AUTO` 時の `glGenerateMipmap(GL_TEXTURE_2D)` は GL 専用 path。Vulkan 化時は `vkCmdBlitImage` + manual mipmap chain による相当処理が必要 (領域 7 sub-step 7.5 範疇)。3.3-C 時点では Vulkan path 並走で `endDynamicRendering` のみ実装し、mipmap 生成は GL に委ねる。

### 4.4 RT stack (`mPreviousRT` linked list) は Vulkan path で再現

GL FBO の `mPreviousRT` linked list による push/pop pattern を Vulkan path でも維持。`flush()` で `mPreviousRT->bindTarget()` 再帰呼出が発生する際、Vulkan path は:
1. 現 RT の `endDynamicRendering` (δ 側) で `sInDynamicRendering=false`
2. 前 RT の `bindTarget()` で再度 `beginDynamicRendering` 呼出 = pair 整合性維持

placeholder 状態 (γ/δ no-op) では実 vk call は発生しないが、領域 7 sub-step 7.5 で実 attachment 配線後は 入れ子 begin/end pattern が動作する。

### 4.5 vk_swiftshader_icd.json 同梱 (NVIDIA path 別)

packaging で `vk_swiftshader_icd.json` を `bin/` + `lib/` に同梱 (strip 対象外、`file format not recognized` warning は無害)。実行時は NVIDIA driver path が優先される (line 91 `Selected physical device: NVIDIA GeForce RTX 5090`)、swiftshader は fallback 用。

### 4.6 validation strict 確認は sub-step 3.5 に持越し

3.3-C 全体を通して release build (validation = disabled) で transit acceptance。validation strict (begin/end pair / image layout transition / attachment compatibility 等の厳密検証) は sub-doc 03 §3.5 (段階 3 self-check) で別 build により実施。3.3-C placeholder 状態では実 vk call が発火しないため validation 通過は trivially satisfied、領域 7 sub-step 7.5 後の strict check が本検証。

### 4.7 領域 7 sub-step 7.5 への引継ぎ要点

3.3-C placeholder 経路から実 attachment 配線への transition で **必須対応**:
1. `LLRenderTarget` 内に `VkImageView mColorViews[4]` / `VkImageView mDepthView` を保持 (VMA `VkImage` から create)
2. `bindTarget` Vulkan path で placeholder の `image_view=VK_NULL_HANDLE` を実 view に差替え
3. `flush` Vulkan path で実 `vkCmdEndRendering` 発火確認 (`sInDynamicRendering=true` 経路通過)
4. layout transition (`vkCmdPipelineBarrier`) を attachment usage に応じて配置
5. validation strict で begin/end pair / attachment compatibility / layout 連続性 全 PASS 確認

---

## 5. next session entry point

### 5.1 次 session 着手前の準備

1. `git pull origin feature/ayastorm-r41-gl-removal` (AYA push 後の最新取得)
2. 本 handoff doc 通読
3. sub-doc 03 §3.1 sub-step list 3.3 行 (3.3-A 完遂後 + 3.3-C 完遂後の 3.3-B 着手境界) 確認
4. sub-doc 06 (shader SPIR-V) scope plan 再確認
5. memory `project_ayastorm_r41_vulkan_migration.md` status update 反映確認

### 5.2 3.3-B 着手 scope (sub-doc 03 §3.1 sub-step 3.3 + sub-doc 06)

| sub-step | 内容 | 想定対象 file | 関連 sub-doc |
|---|---|---|---|
| **3.3-B** | shader SPIR-V port (領域 6 並走、AYAstorm 改変 13 file は touch しない) — GL shader 248 file → SPIR-V 化、UBO binding 整合、shader 内計算 (MVP/normal/inverse_modelview) 配線 | `indra/newview/app_settings/shaders/**` SPIR-V 並走 + shader load path 改修 | `06-shader-spirv.md` |

3.3-B は領域 6 SPIR-V port と並走着手 (Agent 並列化推奨)。3.3-C 完遂で `LLRenderTarget` API surface 並走化は確立、3.3-B の shader port + 3.3-A の matrix UBO/push constant を統合することで 3.4 (12 pool hook body 配線) の準備が整う。

### 5.3 着手前の AYA 確認候補 (推奨 1 件)

- **3.3-B 細分化案** (3.3-A/3.3-C と同様の α/β-1/β-2/γ/δ/ε pattern 採用候補)
  - B-α: spec refine (sub-doc 03 §3.1.3 新規 + sub-doc 06 scope 整理)
  - B-β-1: shader load path 改修 signature (GL shader source compile path に SPIR-V 並走分岐追加)
  - B-β-2: shaderc / glslang 等 SPIR-V compiler 経路 (build 時 pre-compile vs runtime compile 選択)
  - B-γ: 最小 1 shader (sky placeholder 等) で SPIR-V 並走 PSO compile 動作確認
  - B-δ: UBO binding (set=0 binding 0/1) + push constant range (0..64 B / VERTEX_BIT) 整合 + shader 内 MVP/normal/inverse_modelview 計算配線
  - B-ε: handoff doc 起草

### 5.4 critical reminders

- **AYAstorm 改変 13 file shader 改変禁止** (charter §2.1 領域 6、`git diff` 0 件維持)
- **段階 1 + 段階 2 + 3.1b + 3.2 + 3.3-A + 3.3-C 動作維持** (sub-doc 03 §3.5、Vulkan path 並走で GL 描画動作維持)
- **3.3-C 完遂時点で FBO API surface は両方向動作確認済**、3.3-B 着手時に GL path 残置必須 (sub-doc 03 §3.5、視覚 regression 0 維持)
- **実 attachment 配線は領域 7 sub-step 7.5 で実現**、3.3-B 着手時点では依然 placeholder 状態
- **validation strict は sub-step 3.5 で別 build により実施** (3.3-B 着手時も release build で AYA launch + WARN/ERR 0 件 transit acceptance)
- **3.3-C scope 外の持越し**: `VkImage` / `VkImageView` 実体作成 (sub-step 7.5)、layout transition `vkCmdPipelineBarrier` (sub-step 7.5)、mipmap 生成 Vulkan 化 (sub-step 7.5)、validation strict (3.5)、PFNGL function pointer declarations 物理削除 (段階 5)

---

## 6. 関連 doc / memory cross-ref

### 6.1 関連 doc

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter (3.3-C 完遂で charter §3 #5/#6 段階 3 分の FBO API surface 並走化 facet satisfy)
- `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` — 段階 3 sub-doc (本 handoff で sub-step 3.3-C 全完遂 + 3.3-B 着手 ready)
- `docs/specs/ayastorm-r41-gl-removal/05-vulkan-api-design.md` — Vulkan API 設計 (3.3-C では未変更、β-1 sealed の通り適用)
- `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` — shader SPIR-V scope plan (3.3-B 着手時に本格活用)
- `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` — descriptor / render pass scope plan (3.3-C-α で §1.2.3 boundary 明確化済、実 attachment 配線は sub-step 7.5 で本配線)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-A-complete.md` — 直前完了 handoff (役割完了)

### 6.2 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 handoff 完遂で sub-step 3.3-C 全完遂 / 3.3-B 着手 ready 状態に update)
- `project_ayastorm_three_platforms.md` — Linux 先行例外を r41 で適用中
- `feedback_self_verify_before_handoff.md` — 本 handoff 起草前 self-trace 実施 (§1.1 acceptance × log evidence 各 line 突合、commit hash × sub-step mapping 確認、配線整合確認)
- `feedback_one_step_at_a_time.md` — 3.3-C 全 sub-step で AYA 1 件確認原則遵守 (α 推奨 / β-1 案 A / β-2 直行 / γ 直行 / δ 直行)
- `feedback_proactive_handoff.md` — sub-step 3.3-C 全完遂境界での能動 handoff 起草 (本 file)
- `feedback_log_reading.md` — AYA launch 後 Claude が `~/.ayastorm_x64/logs/AYAstorm.log` を grep して β-2/γ/δ marker + Vulkan WARN/ERR 0 件確認
- `feedback_no_scope_shrink.md` — 「段階を踏む」が scope 縮小許可ではないことを 3.3-C 全 sub-step で遵守 (各 sub-step で完全 satisfy)
- `feedback_build_only_verified.md` — placeholder 状態の no-op path も AYA launch + log marker で動作実証してから完遂宣言
