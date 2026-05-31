# r41 sub-step 4.3-β 着手前 prep + scope refine (2026-05-31)

**status**: **draft 2026-05-31 (Pattern α 一括 draft、AYA review 待ち、案 D 採用下起草)**
**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-alpha-complete.md` (sub-step 4.3-α 完遂 → 4.3-β 着手境界、active 継続)
**親 charter**: `docs/specs/ayastorm-r41-gl-removal/00-charter.md` (§2 領域 4 + §7.5 boundary refine 可)
**親 sub-doc**: `docs/specs/ayastorm-r41-gl-removal/04-frame-context.md` (§3.4 per-pool 実 scene draw 移植経路 spec + §5.3 partial 配線注記)
**起草根拠**: AYA さん「D お願いします」承認 2026-05-31 (案 D = 上位 scope refine 提案採用)、trace 結果で 4.3-β scope spec が infrastructure 前提誤りで現状 implementable でない判明 → handoff doc + sub-doc 04 §3.4 の 4.3-β/γ/δ/ε/ζ cadence を再設計

---

## 1. 起草目的 + 案 D 採用根拠

### 1.1 起草目的

sub-step 4.3-α 完遂 (commit `9f13302078` = sCurCameraID accessor 配線 + ScopedCameraID nested RAII 配置、AYA launch verify PASS) 後、4.3-β 着手前 trace で **handoff doc + sub-doc 04 §3.4 spec の 4.3-β scope (Sky+WLSky+WaterExclusion per-pool 実 scene draw 移植) が infrastructure 前提誤りで現状 implementable でない**ことが判明。本 prep doc は **scope refine 提案** + 新 cadence 再設計 + AYA 承認境界再設定提案 を行う。

### 1.2 案 D 採用根拠 (前 session 提示 4 案検討再掲)

| 案 | scope | 採否 | 理由 |
|---|---|---|---|
| **A: spec 通り full 移植** | LLVertexBuffer Vulkan 化 + 4 WLSky shader SPIR-V + per-pool PSO + descriptor 本実装 全部着手 | **reject** | 領域 6 sub-step 6.1 本格着手 = AYA 承認違反 (段階 4 sub-step 4.5 完遂後縛り)、scope explosion |
| **B: 4.3-β skip / 4.4 直行** | 4.3 全 sub-step skip して 4.4 (12 RAII dead-store) 着手 | **reject** | handoff doc spec の literal scope 違反 = scope shrink、`feedback_no_scope_shrink.md` 違反 |
| **C: partial 配線 (sub-doc 04 §5.3 注記適用)** | 4.3-β を「LLCullResult visible list iterate → drawable 数だけ placeholder PSO で繰り返し draw」に縮小 | **fallback** | sub-doc 04 §5.3 「partial 配線許容」literal 適用で structural valid だが、「実 scene draw 移植」ではない (Sky=stub なので iterate しても 0 件、視覚効果ゼロ) = `feedback_self_bug_no_defer_option.md` 精神に反する defer 提案 |
| **D: 上位 scope refine 提案** | handoff doc 4.3-β/γ/δ/ε/ζ 5 段分割を再設計、領域 5+6+7 一体運用 sub-doc 起草 → AYA 承認後実装着手 | **採用 (AYA 2026-05-31 承認)** | 前提整合性回復、案 A と異なり計画的、AYA 承認境界を明示的に再設定 |

---

## 2. trace 結果サマリ (3 agent 並列 trace 統合、2026-05-31)

### 2.1 3 pool 既存 GL 描画 path 現状 (Agent A trace 結果)

| Pool | file:line | GL render path 現状 | 移植難度 |
|---|---|---|---|
| **LLDrawPoolSky** | `lldrawpoolsky.cpp:42-56` | `prerender()/render()/renderSkyFace()/endRenderPass()` **全て empty stub**、GL 描画ロジック無し、`mShader` 宣言のみ bind 呼出無し | **極低** (white slate) |
| **LLDrawPoolWLSky** | `lldrawpoolwlsky.cpp:473-500` | **`renderDeferred()` 完全実装**、4 sub-helper sequential call = `renderSkyHazeDeferred()` (L145) + `renderHeavenlyBodies()` (L356) + `renderStarsDeferred()` (L221) + `renderSkyCloudsDeferred()` (L292)、4 shader bind (sky/cloud/sun/moon)、dome iteration (`gSky.mVOWLSkyp->drawDome()` L126)、face render (`face->renderIndexed()` L411/L461)、stars (`mStarsVerts->drawArrays(TRIANGLES)`)、gGL matrix stack (pushMatrix/translatef/rotatef/scalef)、複数 uniform 設定、**既存 `LLPipelineFrameContext::isReflectionPass()` accessor 利用済** (L105) | **極高** (4 sub-path + dome iter + 4 shader + matrix stack + reflection view dispatch + texture cycle) |
| **LLDrawPoolWaterExclusion** | `lldrawpoolwaterexclusion.cpp:44-83` | **`render()` 完全実装**、`gDrawColorProgram` bind + uniform 設定 + `LLDrawPoolWater::pushWaterPlanes()` (L62/L65 間接呼出) + `pushBatches(PASS_INVISIBLE, false, false)` (L73 LLSpatialGroup batch iterate)、`LLGLDepthTest depth(GL_TRUE)` (L54) + `LLGLDisable cullface(GL_CULL_FACE)` (L61) RAII state | **中** (water pool 間接参照 + batch culling + RAII state scope) |

**共通**: 全 3 pool で `recordPoolDraws(VkCommandBuffer)` hook 配線済、`LLVKLoader::recordPlaceholderPoolDraw()` 経由 fullscreen NDC 三角形描き動作。**既存 GL render() 系は手付かず**で並走状態。

### 2.2 Vulkan placeholder 配線現状 (Agent B trace 結果)

| 項目 | file:line | 状態 |
|---|---|---|
| `recordPlaceholderPoolDraw` helper | `llvkloader.cpp:2694` | 整備済 (PSO bind + set=0 PerFrame + set=1 PerMaterial + push constant 64B + `vkCmdDraw(3,1,0,0)`) |
| dispatcher `recordVulkanPools()` | `pipeline.cpp:4956` | 整備済 (mPools 反復走査 → 各 pool `recordPoolDraws(cmd_buf)`) |
| 12 pool `recordPoolDraws()` override chain | 全 pool 配線済 | 整備済 (3 pool 全て placeholder 呼出) |
| 既存 PSO | `sSkySmokePipeline` / `sAvatarBonePipeline` / `sPlaceholderPipeline` | **3 placeholder のみ** |
| descriptor set=0 PerFrame UBO | `llvkloader.cpp:1340+` | 整備済 (layout + bind helper) |
| descriptor set=1 PerMaterial | `llvkloader.cpp:871-957` | **stub** (白 placeholder tex × 7、`bindPerMaterialDescriptorSet()` 框のみ) |
| descriptor set=2 PerDraw push descriptor | `llvkloader.cpp:1905+` | avatar bone 専用 (VK_KHR_push_descriptor) |
| `vkCmdDrawIndexed` caller | (検出無し) | **0 件** |
| `vkCmdBindVertexBuffers` / `BindIndexBuffer` caller | (検出無し) | **0 件** |

### 2.3 Vulkan infrastructure 現状 (Agent C trace 結果)

| 項目 | file:line | 状態 |
|---|---|---|
| `LLCullResult` visible list iterator API | `llspatialpartition.h:550-572` | 整備済 (`beginVisibleList()` / `endVisibleList()` / `getVisibleList()` / `drawable_iterator` 型定義) |
| `LLVertexBuffer` Vulkan 経路 | `llvertexbuffer.h:92-200` | **未整備** (GL 流儀のみ、VkBuffer / `vkCmdBindVertexBuffers` 未配線) |
| PSO compile helper `compileGraphicsPipeline()` | `llvkloader.cpp:2585` | 整備済 |
| per-pool 専用 PSO registry / cache | (検出無し) | **未整備** (placeholder 3 件のみ) |
| descriptor set 3 階層 layout | 部分配置 | set=0 整備済 / set=1 stub / set=2 avatar 専用 |
| frame loop dispatcher | `pipeline.cpp:4956` | 整備済 |
| `VkCommandBuffer` 伝播経路 | `LLVKLoader::getCurrentCommandBuffer()` (`llvkloader.cpp:2532`) + `recordPoolDraws(VkCommandBuffer)` 単純引数 | 整備済 (LLPipelineFrameContext には未配信、`getCurrentCommandBuffer()` 経由取得) |

---

## 3. 構造的前提誤り

handoff doc + sub-doc 04 §3.4 spec が想定する 4.3-β scope (per-draw PSO bind + descriptor set bind + vertex/index buffer bind + `vkCmdDrawIndexed`) は、以下の段階 5 / 領域 6 / 領域 7 着手を前提:

### 3.1 領域 5 (依存解決) — LLVertexBuffer Vulkan 化

- charter §2 領域 5 (0.50 PM、`llspatialpartition` / `llviewershadermgr` / `llvertexbuffer` / `llvosky` / `llvowlsky` 残依存 file Vulkan 化)
- **現状未着手** (LLVertexBuffer は GL only、VkBuffer 配線無し)
- 段階 5 全体着手は段階 4 完遂後だが、**LLVertexBuffer Vulkan 化のみは段階 4 sub-step 4.3-β 着手の前提条件**

### 3.2 領域 6 sub-step 6.1 — shader SPIR-V port

- charter §2 領域 6 (4.96 PM、248 GLSL shader SPIR-V 化、base port ~228 file)
- memory `project_ayastorm_r41_vulkan_migration.md` §sub-doc 06 + handoff-stage-4-prelude.md §3.1 = **「sub-step 6.1 本格着手 = 段階 4 sub-step 4.1〜4.5 完遂後 AYA 承認、並走しない」(2026-05-31 確定)**
- WLSky 4 shader (sky/cloud/sun/moon) + WaterExclusion (gDrawColorProgram) の SPIR-V port は 4.3-β scope の前提条件

### 3.3 領域 7 sub-step 7.3/7.4/7.5 — descriptor 本実装

- charter §2 領域 7 (1.50 PM、descriptor set 3 階層 + 7 pass chain + KHR_dynamic_rendering)
- sub-step 7.3 (material cache 本実装) + 7.4 (push descriptor 全配線) + 7.5 (実 attachment 配線) = 段階 4-5 並走着手判断保留中 (memory §sub-doc 07 + handoff-stage-4-prelude.md §3.1)
- 3 pool の per-material descriptor bind は 7.3/7.4 着手前提

### 3.4 AYA 承認境界制約 (memory `project_ayastorm_r41_vulkan_migration` 反映)

| AYA 承認事項 | 確定日 | 影響 |
|---|---|---|
| sub-step 6.1 本格着手 = 段階 4 sub-step 4.5 完遂後着手 AYA 承認、並走しない | 2026-05-31 | 4.3-β 着手で領域 6 sub-step 6.1 を **並走で動かすには AYA 承認 update が必須** |
| 領域 7 sub-step 7.3/7.4/7.5 並走着手判断 = 段階 4-5 並走 (handoff で擦り合わせ) | 保留中 | 4.3-β 着手で並走宣言なら handoff で AYA 擦り合わせ |
| 段階 5 着手は段階 4 完遂後 | charter §2 | LLVertexBuffer Vulkan 化のみ前出しは段階順序の boundary refine 案件 (charter §7.5 範囲) |

---

## 4. 新 cadence 再設計 (sub-step 4.3-β/γ/δ/ε/ζ 役割再定義)

### 4.1 旧 cadence (handoff-substep-4-3-alpha-complete.md §6.2 反映、案 B 6 commit 分割)

| sub-step | 旧 scope |
|---|---|
| 4.3-α (完遂) | sCurCameraID accessor 配線 + ScopedCameraID nested RAII |
| 4.3-β | **Sky+WLSky+WaterExclusion per-pool 実 scene draw 移植** ← **infrastructure 前提誤り** |
| 4.3-γ | 9 pool 一括 per-pool 実 scene draw 移植 |
| 4.3-δ | Avatar bone per-draw |
| 4.3-ε | GLTFPBR per-draw |
| 4.3-ζ | self-check + handoff |

### 4.2 新 cadence (案 D 採用、領域 5+6+7 一体運用前提)

| sub-step | 新 scope | 領域 | 前提条件 |
|---|---|---|---|
| **4.3-β-prep (本 doc)** | scope refine 提案 + AYA 承認境界再設定 (本 doc 起草 + AYA review) | — | sub-step 4.3-α 完遂 ✓ |
| **4.3-β'** | **領域 5 LLVertexBuffer Vulkan 化着手** (VkBuffer + bind helper + `vkCmdBindVertexBuffers` / `vkCmdBindIndexBuffer` caller 配置 + LLVertexBuffer instance lifecycle Vulkan 化) | 領域 5 (前出し) | 4.3-β-prep AYA review PASS、AYA 承認 (段階 5 前出し boundary refine) |
| **4.3-γ'** | **領域 6 sub-step 6.1 本格着手** (autobuild integration 一括化、base port ~228 file 全 SPIR-V port、AYAstorm 改変 13 file は r42-α/β/γ scope 外維持) | 領域 6 (前出し) | 4.3-β' 完遂、AYA 承認 (sub-step 6.1 本格着手 = 段階 4 sub-step 4.5 完遂後縛り解除、4.3-γ' に前出し) |
| **4.3-δ'** | **領域 7 sub-step 7.3/7.4/7.5 本格着手** (material cache 本実装 + push descriptor 全配線 + 実 attachment 配線、3 pool + 9 pool + Avatar + GLTFPBR の per-material descriptor 配信) | 領域 7 (本格化) | 4.3-γ' 完遂、AYA 承認 (段階 4-5 並走着手判断 = 段階 4 内本格化に refine) |
| **4.3-ε'** | **Sky+WLSky+WaterExclusion + 9 pool per-pool 実 scene draw 移植** (旧 β + 旧 γ 一括、領域 5+6+7 整備済前提で機械的移植) | 領域 4 (本格) | 4.3-δ' 完遂 |
| **4.3-ζ'** | **Avatar bone per-draw + GLTFPBR per-draw 移植** (旧 δ + 旧 ε 一括、領域 5+6+7 整備済前提で機械的移植) | 領域 4 (本格) | 4.3-ε' 完遂 |
| **4.3-η'** | self-check + handoff (旧 ζ 範式継承) | — | 4.3-ζ' 完遂 |

### 4.3 新 cadence 採用根拠

- **領域 5+6+7 を 4.3 内で一体運用**: per-pool 実 scene draw 移植 (旧 β/γ/δ/ε) の infrastructure 前提条件を 4.3 内で順次整備
- **per-pool 移植 (旧 β + γ + δ + ε) を 2 commit に集約**: 領域 5+6+7 整備後は機械的移植 (1:1 GL → Vulkan 替換) なので分割 overhead 不要、旧 4 commit を 2 commit (4.3-ε' = Sky+WLSky+WaterExclusion+9 pool / 4.3-ζ' = Avatar+GLTFPBR) に集約
- **段階順序 boundary refine** (charter §7.5 範囲): 領域 5 LLVertexBuffer Vulkan 化 + 領域 6 sub-step 6.1 本格着手を **段階 4 sub-step 4.3 内に前出し**、これは charter §7.5 「AYA 確認なしに変更しない content」(§3 #1-#9 criterion 趣旨 + §1 thesis 3 軸 + §4 plan B trigger 閾値 + §5 依存 milestone 構成) **外**で boundary refine 可 (§7.5 「r41 着手中に refine 可な本 charter content」反映)

### 4.4 旧 cadence からの diff

| 項目 | 旧 | 新 | diff |
|---|---|---|---|
| sub-step 数 | 6 (α/β/γ/δ/ε/ζ) | 7 (α/β-prep/β'/γ'/δ'/ε'/ζ'/η') | +1 (β-prep 追加) |
| 領域 5 着手 | 段階 5 (段階 4 完遂後) | 4.3-β' (段階 4 内前出し) | 段階順序 refine |
| 領域 6 sub-step 6.1 着手 | 段階 4 sub-step 4.5 完遂後 | 4.3-γ' (4.3 内前出し) | AYA 承認境界 update 必要 |
| 領域 7 sub-step 7.3/7.4/7.5 着手 | 段階 4-5 並走判断保留 | 4.3-δ' (4.3 内本格化) | AYA 承認境界 update 必要 |
| per-pool 移植 commit 数 | 4 (β/γ/δ/ε) | 2 (ε'/ζ') | 集約 |

---

## 5. AYA 承認境界再設定提案

### 5.1 update 必要な AYA 承認事項

| AYA 承認事項 | 旧 (memory 反映) | 新 (本 prep 提案) |
|---|---|---|
| sub-step 6.1 本格着手 | 段階 4 sub-step 4.5 完遂後、並走しない | **sub-step 4.3-γ' (段階 4 内前出し)、4.3-β' 完遂後着手** |
| 領域 7 sub-step 7.3/7.4/7.5 着手判断 | 段階 4-5 並走、handoff で擦り合わせ | **sub-step 4.3-δ' (段階 4 内本格化)、4.3-γ' 完遂後着手** |
| 段階 5 着手 | 段階 4 完遂後 | **領域 5 LLVertexBuffer Vulkan 化のみ sub-step 4.3-β' で前出し、残 段階 5 scope は段階 4 完遂後維持** |

### 5.2 boundary refine 根拠 (charter §7.5)

- charter §7.5 「r41 着手中に refine 可な本 charter content」: §3 metric / test procedure + §2 領域別 PM 配分 (実装中実測値 refine 可)
- 本 prep 提案は **§2 領域別 着手順序 refine** で、§3 acceptance criterion 趣旨 (charter §7.5 「AYA 確認なしに変更しない」) は維持
- 段階 4 sub-step 4.3 内で領域 5+6+7 を前出し = **領域 4 acceptance #1/#3/#5/#6 段階 4 内 satisfy 経路は維持** (sub-doc 04 §6.1 反映)

### 5.3 AYA review boundary

本 prep doc AYA review PASS 後の次 action:

1. **memory `project_ayastorm_r41_vulkan_migration.md` update**: AYA 承認境界 5.1 反映 + 新 cadence 4.2 反映 + 4.3-β'/γ'/δ'/ε'/ζ'/η' status 追加
2. **sub-doc 04 update**: §3.4 旧 spec を新 cadence 反映に refine + §5.3 partial 配線注記の適用範囲明確化
3. **sub-step 4.3-β' 着手** = 領域 5 LLVertexBuffer Vulkan 化 (LLVertexBuffer Vulkan 経路 trace + VkBuffer 配線 + bind helper + vkCmdBindVertexBuffers/IndexBuffer caller 配置 + instance lifecycle Vulkan 化)

---

## 6. risks / caveats

### 6.1 段階順序 boundary refine の影響範囲

- charter §2 領域 5 の段階 5 scope = LLVertexBuffer + `llspatialpartition` + `llviewershadermgr` + `llvosky` + `llvowlsky` + PFNGL function pointer declarations 物理削除 (§1.5.2 Cluster D 189 file 波及)
- 本 prep 提案は **LLVertexBuffer Vulkan 化のみ前出し**、残 段階 5 scope は段階 4 完遂後維持
- PFNGL 物理削除 = 段階 5 一体運用維持 (charter §2 領域 5 + sub-doc 04 §6.2 反映)

### 6.2 領域 6 sub-step 6.1 並走着手の risk

- charter §2 領域 6 工数 = 4.96 PM (r41 最大 scope)、本格着手で **段階 4 完遂時期推定大幅 push back**
- ただし per-pool 実 scene draw 移植が 4.3 内で完遂すれば、段階 4 acceptance #3 (PSO bind 動作 + validation 0 件、placeholder → 実 scene draw refine) が 4.3 内 satisfy 経路で達成 → 段階 4 完遂が前倒し
- net effect: 段階 4 単独完遂時期 push back / 但し 段階 4+5 一体完遂時期は前倒し (領域 5+6+7 一部を段階 4 内吸収)

### 6.3 領域 7 sub-step 7.3/7.4/7.5 本格化の risk

- 領域 7 工数 = 1.50 PM、本格着手で **段階 4 完遂時期 push back**
- ただし 3.4-β-1/γ/δ-4 で sub-step 7.1 + 7.3 layout + 7.4 push descriptor 部分内包済 (memory §sub-doc 07 反映)、本格化は incremental 拡張

### 6.4 案 D 採用の代替案との対比

| 代替案 | 採用時の進捗影響 |
|---|---|
| 案 A (full 移植、AYA 承認違反) | 即着手可、ただし feedback_no_scope_shrink 等違反で structural 問題 |
| 案 B (4.3-β skip / 4.4 直行) | 即着手可、ただし scope shrink で defer signal、`feedback_self_bug_no_defer_option.md` 違反 |
| 案 C (partial 配線、視覚効果ゼロ) | 即着手可、structural valid だが「実 scene draw 移植」ではない (Sky=stub iterate 0 件)、handoff doc spec の literal scope 違反 |
| **案 D (本 prep 提案)** | **着手まで 1 session delay (本 prep AYA review)、ただし計画的 + 整合的 + AYA 承認境界明示更新で structural sound** |

### 6.5 case-validity 担保 (sub-doc 04 §5.3 partial 配線注記との関係)

- sub-doc 04 §5.3 注記「partial 配線許容: 領域 7 sub-step 7.3/7.4/7.5 が partial state でも sub-step 4.3 は placeholder material + placeholder descriptor で動作確認可」は **literal valid**、案 C はこの注記を literal 適用
- 但し本 prep 提案は §5.3 注記を **より明示的に再評価** = 領域 5 LLVertexBuffer Vulkan 化前提を §3.4 spec に追記、4.3 内で領域 5+6+7 を順次本格化する設計を §3.4 spec literal 化

### 6.6 context budget concern

- 本 prep doc 起草 + AYA review + memory update + sub-doc 04 update + sub-step 4.3-β' 着手 = **複数 session に跨る可能性高い**
- 各境界で proactive handoff 範式継承 (`feedback_proactive_handoff.md` + `feedback_self_verify_before_handoff.md`)

---

## 7. next action (本 prep AYA review PASS 後)

### 7.1 着手前 cadence (AYA 承認後の最初の sub-step)

1. memory `project_ayastorm_r41_vulkan_migration.md` update (AYA 承認境界 + 新 cadence 反映)
2. sub-doc `04-frame-context.md` update (§3.4 spec refine + §5.3 注記再評価)
3. sub-step 4.3-β' 着手 (LLVertexBuffer Vulkan 化 trace + 実装 + build + AYA launch verify + commit)

### 7.2 sub-step 4.3-β' 着手 task 候補

| task | 詳細 |
|---|---|
| β'-1 | LLVertexBuffer Vulkan 経路 trace (`llvertexbuffer.{h,cpp}` 現状 GL API inventory + VkBuffer 配線設計 + VMA allocator 利用判断) |
| β'-2 | VkBuffer + bind helper 配置 (`createVertexBuffer()` / `createIndexBuffer()` / `bindVertexBuffer()` / `bindIndexBuffer()` Vulkan 経路) |
| β'-3 | `vkCmdBindVertexBuffers` / `vkCmdBindIndexBuffer` caller 配置 (LLVKLoader 経由 helper 配線) |
| β'-4 | LLVertexBuffer instance lifecycle Vulkan 化 (ctor/dtor + map/unmap + draw call 経路) |
| β'-5 | incremental autobuild |
| β'-6 | AYA launch verify (案 B cadence) |
| β'-7 | commit (4.3-β' 単独 commit、ただし build break/launch fail 時は β'-1〜β'-4 個別分割可能性) |

### 7.3 critical reminders (4.3-β'/γ'/δ'/ε'/ζ'/η' 通底)

| reminder | 詳細 |
|---|---|
| **shader 改変禁止** | sub-doc 04 §1.2 範囲、本格 shader port は 4.3-γ' (sub-step 6.1) でのみ |
| **段階 1-4.3-α 動作維持** | regression risk highest watch、特に LLVertexBuffer Vulkan 化は既存 GL drawpool 動作影響範囲広大 |
| **案 B cadence 継承** | measurement log 配線 skip + AYA 短評で satisfy |
| **active camera 配線維持** | 4.3-α accessor 経由で `getCurCameraID()` 取得継続 |
| **AYA 承認境界遵守** | 本 prep AYA review PASS が prerequisite、PASS 前に着手しない (`feedback_no_scope_shrink.md` 反対側 = 「承認前着手」も禁止) |
| **proactive handoff 範式遵守** | context budget 周回境界で能動監視、AYA 指示待たず handoff 起草 |

### 7.4 commit 戦略

- 本 prep doc + memory + sub-doc 04 update = **1 commit想定** (doc-only commit、AYA 承認後実施)
- sub-step 4.3-β'〜ζ'/η' = **各 sub-step 1 commit 想定** (build break/launch fail 時は段階分割可能性、sub-step 3.3-A 範式継承)

---

## 8. 関連 doc / memory cross reference

### 8.1 関連 doc

| doc | 役割 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/00-charter.md` | charter §2 領域 4/5/6/7 + §7.5 boundary refine 可 (本 prep 段階順序 refine 根拠) |
| `docs/specs/ayastorm-r41-gl-removal/04-frame-context.md` | §3.4 旧 spec (本 prep で refine 提案) + §5.3 partial 配線注記 (本 prep で再評価) + §6.1 acceptance #1/#3/#5/#6 段階 4 内 satisfy 計画 |
| `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` | sub-step 6.1 source (本 prep 4.3-γ' で本格着手) |
| `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` | sub-step 7.3/7.4/7.5 source (本 prep 4.3-δ' で本格化) |
| `docs/specs/ayastorm-r41-gl-removal/08-llvkrenderer-skeleton.md` | sub-step 4.4 part B 実装 source (4.4 着手時参照) |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-alpha-complete.md` | 前 handoff (4.3-α 完遂 → 4.3-β 着手境界、active 継続) |
| `docs/specs/ayastorm-r41-gl-removal/handoff-stage-4-prelude.md` | 段階 4 着手前 prelude (役割完了、§3.1 AYA 承認境界 source) |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-beta-prep.md` | **本 prep doc** (新規、4.3-β scope refine 提案) |

### 8.2 関連 memory

| memory | 役割 |
|---|---|
| `project_ayastorm_r41_vulkan_migration.md` | active milestone tracking (本 prep AYA 承認後 update 必要 = AYA 承認境界 + 新 cadence 反映) |
| `feedback_proactive_handoff.md` | context 圧迫時の proactive handoff 範式 |
| `feedback_self_verify_before_handoff.md` | handoff 起草前の self-trace 義務 (本 prep 起草前 trace 完遂遵守) |
| `feedback_no_scope_shrink.md` | scope shrink 禁止 (本 prep 案 D 採用 = 案 B/C reject 根拠) |
| `feedback_self_bug_no_defer_option.md` | defer option 提示禁止 (本 prep 案 D 採用 = 案 B/C reject 根拠) |
| `feedback_no_claude_coauthor.md` | commit message Co-Authored-By: Claude 禁止 |
| `feedback_proactive_diagnostic.md` | log/grep/gdb 系は Claude が直接実行 (本 prep trace = Agent 並列実施) |
| `feedback_use_agents_proactively.md` | Agent 並列 trace 採用 (本 prep §2 trace = 3 agent 並列) |
| `feedback_doubt_self_first.md` | trace 結果を AYA 共有前に自分で再確認 (本 prep 構造的前提誤り発見) |
| `feedback_admit_unknown.md` | 仮説 2 連続外れたら log/canary/bisect 切替 (本 prep 案検討で 4 案提示 + 推奨明示) |
| `feedback_explanation_lead_with_conclusion.md` | 結論ファースト記述 (本 prep §1.2 案 D 採用根拠 + §4.3 新 cadence 採用根拠) |
| `feedback_no_auto_commit.md` | コミットは明示指示後 (本 prep 起草 commit = AYA 指示後) |
| `feedback_one_step_at_a_time.md` | 1 メッセージ 1 アクション cadence |
| `feedback_remove_verification_logs.md` | 案 B では log 配線 skip = 除去対象なし |
| `project_build_procedure.md` | autobuild fullflow + .venv activate + AUTOBUILD_VARIABLES_FILE (4.3-β' build 時参照) |

### 8.3 charter / sub-doc cross reference

- charter §2 領域 4/5/6/7 (本 prep §3 構造的前提誤り根拠 + §4 新 cadence 設計根拠)
- charter §3 #1/#3/#5/#6 acceptance (本 prep §5.2 acceptance criterion 趣旨維持 + §6.1 段階順序 boundary refine 範囲)
- charter §7.5 boundary refine 可 (本 prep §5.2 boundary refine 根拠)
- sub-doc 04 §1.3 並走領域との関係 (本 prep §4 新 cadence で領域 5+6+7 並走着手宣言根拠)
- sub-doc 04 §3.4 per-pool 実 scene draw 移植経路 spec (本 prep §3 構造的前提誤り対象 + §5.3 AYA review 後 refine 対象)
- sub-doc 04 §5.3 partial 配線注記 (本 prep §6.5 case-validity 担保 + §5.3 AYA review 後 再評価対象)
- sub-doc 04 §6.1 charter §3 acceptance 段階 4 satisfy 計画 (本 prep §5.2 維持対象)

---

## prep doc 完成宣言 (2026-05-31)

本 prep doc は **Pattern α 一括 draft 完成 (2026-05-31、AYA review 待ち)**、案 D 採用下起草。

### draft 完成宣言の内訳

- §1 起草目的 + 案 D 採用根拠
- §2 trace 結果サマリ (3 agent 並列 trace 統合)
- §3 構造的前提誤り (領域 5/6/7 + AYA 承認境界制約)
- §4 新 cadence 再設計 (sub-step 4.3-β-prep/β'/γ'/δ'/ε'/ζ'/η' 役割定義 + 旧 cadence diff)
- §5 AYA 承認境界再設定提案 (update 必要事項 + boundary refine 根拠 + AYA review boundary)
- §6 risks / caveats (段階順序 + 領域 6/7 本格化 + 案検討対比 + case-validity + context budget)
- §7 next action (着手前 cadence + 4.3-β' task 候補 + critical reminders + commit 戦略)
- §8 関連 doc / memory cross reference

### AYA review boundary

本 prep doc AYA review PASS 後の次 action:

1. memory `project_ayastorm_r41_vulkan_migration.md` update (AYA 承認境界 §5.1 + 新 cadence §4.2 反映)
2. sub-doc `04-frame-context.md` update (§3.4 spec refine + §5.3 注記再評価)
3. 本 prep doc + memory + sub-doc 04 update を **1 commit (doc-only)** で AYA 指示後実施
4. sub-step 4.3-β' (LLVertexBuffer Vulkan 化) 着手

### 関連 commit (本 prep doc 起草 commit は AYA 指示後実施)

本 prep doc 起草 commit は `feedback_no_auto_commit.md` 遵守で **AYA 明示指示後 Claude が実施**。commit message draft (AYA 指示時に refine 可):

```
docs(r41): handoff-substep-4-3-beta-prep.md 起草 (sub-step 4.3-β scope refine 提案、案 D 採用)

- 4.3-β 着手前 trace で infrastructure 前提誤り発覚 = handoff + sub-doc 04 §3.4 spec の per-pool 実 scene draw 移植は領域 5/6/7 着手前提だが未着手
- 案 D 採用 (AYA 「D お願いします」承認 2026-05-31) = 上位 scope refine 提案、案 A/B/C reject
- §2 trace 結果 (3 agent 並列): 3 pool 現状 (Sky=stub / WLSky=ultra-high / WaterExclusion=medium) + Vulkan infra (placeholder PSO + LLVertexBuffer GL only + descriptor stub)
- §4 新 cadence: 4.3-β-prep / β' (LLVertexBuffer Vk 化) / γ' (sub-step 6.1 shader port) / δ' (7.3/7.4/7.5 本格化) / ε' (Sky+WLSky+WaterExclusion+9 pool 移植) / ζ' (Avatar+GLTFPBR 移植) / η' (self-check + handoff)
- §5 AYA 承認境界 update 必要 = sub-step 6.1 縛り解除 (4.3-γ' 前出し) + 領域 7 sub-step 7.3/7.4/7.5 本格化 (4.3-δ' 内) + 段階 5 LLVertexBuffer 部分前出し
- charter §7.5 boundary refine 範囲内、acceptance criterion 趣旨維持
```

push は AYA 手動 (`feedback_release_flow.md` 遵守)。
