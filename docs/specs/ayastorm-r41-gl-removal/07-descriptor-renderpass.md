# AYAstorm r41 sub-doc 07-descriptor-renderpass — 領域 7 descriptor set 3 階層 + 7 pass chain + dynamic rendering

**status**: **closed 2026-05-29 (Pattern α 一括 draft、AYA review PASS)**
**親 charter**: `docs/specs/ayastorm-r41-gl-removal/00-charter.md` (closed 2026-05-28)
**並走 sub-doc**: `03-state-machine-pso.md` (段階 3 = 領域 3、PSO 配線で descriptor set 3 階層必要) + `06-shader-spirv.md` (領域 6 = shader 側 `layout(set=N, binding=M)` qualifier 配信が本領域設計と一致)
**前 handoff**: `handoff-stage-2-complete.md` (段階 2 完遂 → 段階 3 着手境界 + 領域 6/7 並走起草判断 B 採用 2026-05-28)
**達成条件**: 領域 7 完遂 = descriptor set 3 階層 (set=0 per-frame / set=1 per-material / set=2 per-draw) 動作 + VK_KHR_push_descriptor (set=2 limited) 動作 + 7 pass chain (shadow / g-buffer+picker / deferred lighting / forward alpha / sky / post-process / UI) 動作 + VK_KHR_dynamic_rendering 採用 (`VkRenderPass` + `VkFramebuffer` 廃止) + validation 0 件
**関連 charter section**: §2 領域 7 (1.50 PM 中 risk) + §3 #5 acceptance + §7.4 sub-doc 構成 + §7.5 boundary

---

## §1 領域 7 scope plan

### §1.1 領域 7 scope 再掲 (charter §2 領域 7 + 05 §3 + §4)

- **対象**: descriptor set 3 階層 + 7 pass chain + dynamic rendering の実装 (05 doc §3 + §4 設計の実装 mapping 詳細化)
- **境界条件 (charter §2 領域 7)**: 領域 3 PSO 化 + 領域 6 shader binding と協調 (shader 側 `layout(set=N, binding=M)` が descriptor set layout と一致)
- **依存順序 (charter §2 領域 7)**: 領域 1 (段階 1) 完了後着手、領域 2 (段階 2 完遂済) + 領域 3 (段階 3) + 領域 6 (本 sub-doc と同時起草) と並走必須
- **risk 性質 (charter §2 領域 7)**: **中** — 05 doc §3 + §4 で設計済、実装は a-3 § 反映通りだが 7 pass chain の transition + dependency 整合に sync 設計 (05 §5) 反映必要
- **採用 Vulkan extension** (05 §1.1 + §9.2 確定): `VK_KHR_push_descriptor` (set=2 per-draw) + `VK_KHR_dynamic_rendering` (Vulkan 1.3 core) + `VK_EXT_memory_budget` (VMA budget query) + `VK_EXT_swapchain_maintenance1` (driver support 検出後採用)

### §1.2 descriptor set 3 階層 + 7 pass chain 実装方針 (05 §3 + §4 反映)

#### §1.2.1 descriptor set 3 階層 (05 §3.1 採用、3 set 構成)

| set | binding 頻度 | 想定 content | descriptor type | 領域 7 実装方針 |
|---|---|---|---|---|
| **0: per-frame** | 1 frame に 1 回 | camera matrix / sun position / time / shadow map sampler ×4 / env cubemap sampler ×4 / noise / blue noise / sky cubemap / atmospherics LUT / windlight LUT / AYAstorm picker output buffer | UBO + COMBINED_IMAGE_SAMPLER (~30 sampler) | `VkDescriptorSetLayout` 1 件で固定、起動時 1 度作成 + per-frame 1 回 `vkUpdateDescriptorSets` + `vkCmdBindDescriptorSets` |
| **1: per-material** | material 種別ごと | diffuse / normal / specular / AO / emissive × material 種別 (~50) + material params UBO | COMBINED_IMAGE_SAMPLER × ~6 + UBO × 1 (~70 sampler) | `VkDescriptorSetLayout` 1 件 + descriptor pool sizing ~50 × frame in flight 3 = 150 (上限見積)、material cache で set=1 bind 切替のみで material 切替可 |
| **2: per-draw** | draw call ごと | per-object UBO (model matrix は push constant に分離) + per-draw texture (avatar BoM / attachment ~106) | UBO + COMBINED_IMAGE_SAMPLER (動的更新) | **VK_KHR_push_descriptor** 採用、`vkCmdPushDescriptorSetKHR` で command buffer 直接 inline 書込み、pool 不要 / lifetime 管理不要、1 set あたり binding 数 ≤ 32 (Vulkan 1.3 minimum) |

#### §1.2.2 pipeline layout 設計 (05 §3.5 採用)

```
VkPipelineLayoutCreateInfo {
    .setLayoutCount         = 3
    .pSetLayouts            = [perFrameLayout, perMaterialLayout, perDrawLayout]
    .pushConstantRangeCount = 1
    .pPushConstantRanges    = [{ VK_SHADER_STAGE_VERTEX_BIT, 0, 64 (mat4 modelMatrix) }]
}
```

pipeline layout cache で同一 layout を共有する PSO を grouping、`vkCreatePipelineLayout` 重複回避。

#### §1.2.3 7 pass chain 構成 (05 §4.3 採用、dynamic rendering 化)

| pass | 内容 | 対象 pool / 対応 stage | attachment | 関連 sub-step |
|---|---|---|---|---|
| **pass 1** | shadow map | cascade × 4 (sun) + spot light shadow、shadow-eligible pool 経由 | depth only (4 × depth array texture) | 7.5-a |
| **pass 2** | deferred g-buffer + picker write | avatar / bump / materials / pbropaque / terrain / tree pool + r21.1 picker | gbuffer0/1/2/3 (RGBA8) + depth (D24S8)、gbuffer3 = r21.1 picker LocalID/ObjectID 格納 | 7.5-b |
| **pass 3** | deferred lighting (soften) | sun + light list + reflection probe | HDR color (R16G16B16A16_SFLOAT)、input: g-buffer ×4 + depth + shadow map ×4 | 7.5-c |
| **pass 4** | forward alpha + particles | alpha / water pool | HDR color (alpha BLEND)、depth READ_ONLY | 7.5-d |
| **pass 5** | sky + atmospherics | sky / wlsky pool + llvosky / llvowlsky | HDR color (forward write)、depth READ_ONLY | 7.5-e |
| **pass 6** | post-process chain | godrays → volumetricLight → blurLight → vignette → DoF (r30) → tonemap (7 sub-pass、r14+ visual realism 関連は r42-γ で port、本領域は legacy LL 部分のみ) | post-process ping-pong (intermediate A/B、HDR color × 2) | 7.5-f |
| **pass 7** | UI + 2D | llrender2dutils / font / cursor | swapchain image (sRGB) | 7.5-g |

各 pass は **独立 `vkCmdBeginRenderingKHR` / `vkCmdEndRenderingKHR`** でくくる、`VkRenderPass` + `VkFramebuffer` 明示作成は使わない (05 §4.1 採用)。

**段階 3 sub-step 3.3-C との boundary 明確化 (2026-05-29 追記)**: 段階 3 sub-step 3.3-C (FBO → dynamic rendering、sub-doc 03 §3.1.2 参照) で `LLRenderTarget::bindTarget()` / `flush()` を `vkCmdBeginRenderingKHR` / `vkCmdEndRenderingKHR` wrap helper (`beginDynamicRendering()` / `endDynamicRendering()`) 経由で並走化、3.3-C 完遂時の Vulkan path は placeholder attachment による transit smoke。本領域 7 sub-step 7.5 で実 attachment (color × N + depth、`VkImage` + `VkImageView` via VMA = sub-step 7.1) を提供して 7 pass chain の final 配線完了 (3.3-C 設計確定 2026-05-29、本領域 sub-step 7.5 で 3.3-C 並走 path の placeholder を実 attachment へ差替)。

### §1.3 並走領域との関係 (charter §2 領域 7 依存順序)

| 並走領域 | 領域 7 内での協調事項 |
|---|---|
| 領域 3 (段階 3 state machine → PSO 化) | PSO bind 時に descriptor set 3 階層が前提、`VkPipelineLayoutCreateInfo` の descriptorSetLayout 配列が本領域設計から確定、段階 3 sub-step 3.3 matrix stack → push constant 化と本領域 push constant range 設計が同期 |
| 領域 6 (shader SPIR-V) | shader 側 `layout(set=N, binding=M)` qualifier 配信が本領域 `VkDescriptorSetLayoutBinding` 設計と一致、領域 6 sub-step 6.4 descriptor binding 統合 phase と本領域 sub-step 7.2-7.4 が同期 |
| 領域 2 (lldrawpool、段階 2 完遂済) | 12 pool hook body PSO bind 時に descriptor set 3 階層 bind 配線 (段階 3 sub-step 3.4 と並走)、pass 2 deferred g-buffer pool 群 (avatar / bump / materials / pbropaque / terrain / tree) の attachment binding 確定 |
| 領域 5 (段階 5 残依存解決) | pass 5 sky + atmospherics は llvosky / llvowlsky 依存、段階 5 完遂前は本領域 sub-step 7.5-e は placeholder で済ます |
| 領域 8 (LLVKRenderer skeleton) | descriptor set 配信 + render pass chain API の signature が r41.5 interface 経由 call 化前提に整合 (charter §3 #6 担保) |

### §1.4 r14+ post-process pass + r21.1 picker の境界 (本領域 vs r42-α/γ scope)

| 項目 | 本領域 7 で実装 | r42 milestone で実装 |
|---|---|---|
| pass 2 deferred g-buffer + r21.1 picker attachment 配置 | gbuffer3 を picker LocalID/ObjectID 格納に流用 (05 §3.4 + §4.5 確定方針) + `vkCmdCopyImageToBuffer` で CPU readback buffer 転送 | **picker shader (objectIDV/F.glsl 2 file) の SPIR-V port は r42-α** (本領域 7 では attachment / readback path のみ整備、shader port は領域 6 untouched scope に含む) |
| pass 6 post-process chain | legacy LL post-process (godrays / volumetricLight 一部 / tonemap) を 7 sub-pass で実装 | **r14+ visual realism 7 shader (godrays / volumetricLight / blurLight / vignette / atmosphericsF / DoF / etc.) は r42-γ** で port (本領域 7 では sub-pass slot のみ確保、shader 側は領域 6 untouched scope) |
| pass 6 Cinematic DoF | r30 BD import 部分の DoF state は段階 4 frame context 統合と並走、本領域では sub-pass slot のみ確保 | **Cinematic DoF shader 4 file は r42-β** で port |

---

## §2 実装順序 + dependency graph

### §2.1 着手順序の決定軸 3 点

1. **VMA + descriptor pool 基盤先行** — set=0/1/2 + 7 pass chain 全てが VkBuffer / VkImage / descriptor pool に依存、VMA + descriptor pool sizing 基盤を先行 (06 §6 VMA 採用 + 05 §3.6 pool sizing)
2. **set=0 per-frame → set=1 per-material → set=2 per-draw の順** — 静的 → 中度動的 → 高度動的の順、set=0 でまず 1 frame に 1 回 bind の baseline を確立、set=1 で material cache、set=2 で push descriptor の漸進的複雑化
3. **7 pass chain は g-buffer (pass 2) + lighting (pass 3) 先行** — deferred の core、shadow (pass 1) は cascade ×4 設計が pass 2 attachment との dependency にも影響、pass 4/5/6/7 は core 確立後

### §2.2 dependency graph (5 階層、VMA → set=0 → set=1 → set=2 + push descriptor → 7 pass chain)

```
VMA + descriptor pool sizing 基盤 (sub-step 7.1)
├── set=0 per-frame 配線 (sub-step 7.2)
│   └── camera/light matrix UBO + shadow map sampler + atmospherics LUT + AYAstorm picker output buffer
├── set=1 per-material 配線 (sub-step 7.3)
│   └── diffuse/normal/specular/AO/emissive + material params UBO + material cache
├── set=2 per-draw + push descriptor 配線 (sub-step 7.4)
│   └── VK_KHR_push_descriptor + per-object UBO (model matrix は push constant) + per-draw texture
└── 7 pass chain + dynamic rendering 実装 (sub-step 7.5)
    ├── 7.5-a pass 1 shadow map (cascade ×4)
    ├── 7.5-b pass 2 deferred g-buffer + picker write (r21.1 attachment 配置)
    ├── 7.5-c pass 3 deferred lighting (soften)
    ├── 7.5-d pass 4 forward alpha + particles
    ├── 7.5-e pass 5 sky + atmospherics (領域 5 並走、placeholder 可)
    ├── 7.5-f pass 6 post-process chain (legacy LL のみ、r42-γ scope 分は sub-pass slot 確保)
    └── 7.5-g pass 7 UI + 2D (swapchain image)
```

### §2.3 並列着手可能性

- sub-step 7.2 / 7.3 / 7.4 (set=0/1/2 配線) は independent (set 別 layout 設計) → 並列着手可能、Agent 並列活用候補
- sub-step 7.5 内 pass 配線は **pass 2 → pass 3 → pass 4/5/6/7** の順序依存 (g-buffer → lighting → alpha/sky/post-process/UI)、pass 1 shadow は pass 2 と並列可、pass 4/5/6/7 は pass 3 完了後並列着手可
- sub-step 7.5-e pass 5 sky は領域 5 (llvosky / llvowlsky 段階 5 scope) 完遂前は placeholder で済ます、段階 5 完遂後に final 配線

---

## §3 領域 7 sub-step 順序 (5 sub-step 化、段階 2/3 + 領域 6 範式継承)

`02-portage-execution.md` §3 + `03-state-machine-pso.md` §3 + `06-shader-spirv.md` §3 範式継承 (5 sub-step + 末尾 self-check)、§2.2 dependency graph に沿って bundle。

### §3.1 sub-step list

| sub-step | scope | 対象 | 完了 marker |
|---|---|---|---|
| **7.1** | VMA + descriptor pool sizing 基盤 | llvkloader.{cpp,h} 拡張 (VMA allocator init + descriptor pool 作成 + budget API `VK_EXT_memory_budget` 統合) + 05 §3.6 pool sizing 戦略反映 (.maxSets ~200) | VMA allocator init 成功 + descriptor pool 作成成功 + VRAM budget query 成功 + validation 0 件 (※段階 3 sub-step 3.4-β-1 で内包先行 install、03 §3.1.4 参照、本 sub-step 7.1 着手時は 3.4-β-1 完遂 evidence を継承して残作業のみ実施) |
| **7.2** | set=0 per-frame 配線 | `VkDescriptorSetLayout` (set=0) 作成 + camera/light matrix UBO + shadow map sampler ×4 + env cubemap sampler ×4 + atmospherics LUT + windlight LUT + AYAstorm picker output buffer (~30 sampler 集約) + 段階 3 sub-step 3.3 matrix stack → push constant 化と同期 | viewer 起動時 set=0 layout 作成成功 + 1 frame 内 `vkUpdateDescriptorSets` + `vkCmdBindDescriptorSets` 動作 + 領域 6 sub-step 6.4 shader 側 `layout(set=0, binding=N)` qualifier 整合 + validation descriptor binding mismatch 0 件 |
| **7.3** | set=1 per-material 配線 + material cache | `VkDescriptorSetLayout` (set=1) 作成 + diffuse/normal/specular/AO/emissive × material 種別 + material params UBO + material cache (set=1 bind 切替のみで material 切替可) | 12 pool (段階 2 完遂済) で material 種別ごと set=1 bind 切替動作 + 領域 6 sub-step 6.4 shader 側 `layout(set=1, binding=N)` 整合 + material cache hit rate ≥ 80% (validation 計測) (※段階 3 sub-step 3.4-γ で layout + 1 set transit smoke を前倒し、03 §3.1.4 参照、本 sub-step 7.3 着手時は material cache 本実装 [~50 material × frame in flight 3 = 150 pool sizing + cache hit rate ≥ 80%] が残作業) |
| **7.4** | set=2 per-draw + push descriptor 配線 | `VK_KHR_push_descriptor` extension enable + set=2 layout (binding ≤ 32) + `vkCmdPushDescriptorSetKHR` 配線 + per-object UBO + per-draw texture (avatar BoM / attachment ~106) + model matrix は push constant 64 bytes (段階 3 sub-step 3.3 と同期) | 段階 3 sub-step 3.4 12 pool hook body 配線時に set=2 push descriptor 動作 + push constant 64 bytes mat4 modelMatrix 受領動作 + 領域 6 sub-step 6.4 shader 側 `layout(set=2, binding=N)` + `layout(push_constant)` qualifier 整合 + validation push descriptor range violation 0 件 (※段階 3 sub-step 3.4-δ で avatar SSBO 基本配線時に `vkCmdPushDescriptorSetKHR` 基本配線を前倒し、03 §3.1.4 参照、本 sub-step 7.4 着手時は per-object UBO + per-draw texture binding ≤ 32 全配線が残作業) |
| **7.5** | 7 pass chain 実装 + dynamic rendering + 領域 7 self-check + handoff doc | 7 sub-pass: 7.5-a shadow / 7.5-b g-buffer+picker / 7.5-c deferred lighting / 7.5-d forward alpha / 7.5-e sky (領域 5 並走、placeholder 可) / 7.5-f post-process (legacy LL のみ、r42-γ shader は領域 6 untouched scope) / 7.5-g UI + 2D + §4.1 acceptance 6 件 self-trace + handoff doc 作成 | 7 pass 全 `vkCmdBeginRenderingKHR` / `vkCmdEndRenderingKHR` 配線動作 + attachment load/store op 整合 + sync barrier (05 §5.3 layout transition 表反映) 動作 + validation render pass dependency violation / image layout mismatch 0 件 + handoff doc `handoff-stage-7-complete.md` 作成 |

### §3.2 sub-step 内 file 順序の柔軟性 (charter §7.5 boundary refine 可)

- sub-step 7.2 / 7.3 / 7.4 (set=0/1/2 配線) は dependency graph 上 independent → 並列着手候補 (Agent 活用)
- sub-step 7.5 の 7 sub-pass は **7.5-b → 7.5-c** が core dependency、7.5-a (shadow) は 7.5-b と並列可、7.5-d/e/f/g は 7.5-c 後並列着手可
- sub-step 7.5-e pass 5 sky は領域 5 (llvosky / llvowlsky 段階 5 scope) 完遂前は placeholder 可、段階 5 完遂後に final 配線 (charter §7.5 boundary refine)
- sub-step 7.5-f pass 6 post-process は r14+ visual realism shader 7 file (r42-γ scope) を本領域で touch しない、sub-pass slot のみ確保し shader 側は領域 6 untouched scope 維持

### §3.3 領域 7 で touch しない file (本領域 scope 外)

- llrender 主要 5 file (段階 3 scope、本領域は llrender 側 interface 経由で descriptor / pass を bind)
- lldrawpool 13 file C++ 側 (段階 2 完遂済 + 段階 3 sub-step 3.4 で hook body PSO bind 配線、本領域は pass 2 attachment 配置のみ)
- pipeline.cpp 3 大グローバル (段階 4 scope)、llspatialpartition / llviewershadermgr / llvertexbuffer / llvosky / llvowlsky (段階 5 scope、本領域 sub-step 7.5-e で連携)
- 248 shader SPIR-V 化 (領域 6 scope、本領域は shader 側 `layout(set=N, binding=M)` qualifier 配信を領域 6 sub-step 6.4 と同期するのみ)
- **r14+ post-process shader 7 file** (r42-γ scope、本領域は pass 6 sub-pass slot 確保のみ、shader port は r42-γ)
- **r21.1 picker shader 2 file** (r42-α scope、本領域は pass 2 gbuffer3 attachment 配置 + readback path のみ、shader port は r42-α)
- **r30 Cinematic DoF shader 4 file** (r42-β scope、本領域は pass 6 sub-pass slot のみ確保)
- LLVKRenderer pipeline.cpp inline 実装 (段階 4 + 領域 8 scope)

---

## §4 領域 7 completion criteria

charter §3 acceptance criterion #5 (descriptor set + render pass 設計実装) + #3 (段階 1-5 全完遂と協調) の **領域 7 分 self-check**。

### §4.1 領域 7 自己 acceptance

| criterion | metric | test procedure |
|---|---|---|
| **#5-領域 7 (descriptor set 3 階層 + push descriptor 動作)** | set=0 per-frame / set=1 per-material / set=2 per-draw 3 階層動作 + set=2 push descriptor (`VK_KHR_push_descriptor`) で pool 不要動作 | `grep -E "VK_KHR_push_descriptor\|vkCmdPushDescriptorSetKHR" indra/llrender/` で配線 hit + validation layer で descriptor binding mismatch / push descriptor range violation **0 件** + material cache hit rate ≥ 80% (validation 計測) |
| **#5-領域 7 (7 pass chain + dynamic rendering 動作)** | 7 pass chain (shadow / g-buffer+picker / deferred lighting / forward alpha / sky / post-process / UI) 動作 + `VK_KHR_dynamic_rendering` 採用 (`VkRenderPass` + `VkFramebuffer` 廃止) | `grep -E "vkCmdBeginRenderingKHR\|VkRenderingAttachmentInfo" indra/llrender/` で配線 hit + `grep -rE "vkCreateRenderPass\|vkCreateFramebuffer" indra/llrender/` が **0 件 hit** (dynamic rendering 採用、明示 RP/FB 廃止 acceptance) + validation render pass dependency violation / image layout mismatch **0 件** |
| **#5-領域 7 (attachment + sync 整合)** | 05 §4.2 attachment 構成 (gbuffer0/1/2/3 + depth + HDR color intermediate A/B + swapchain) + 05 §5.3 image layout transition 表 + 05 §5.4 synchronization2 access mask 整合 | viewer 起動 + sustained ~10 分動作 + validation layer で image layout transition mismatch / access mask violation **0 件** + `VK_EXT_memory_budget` VRAM 監視で leak ≤ 0 byte/min |
| **#5-領域 7 (AYAstorm 改変 13 file の attachment slot 確保のみ)** | pass 2 gbuffer3 (r21.1 picker) + pass 6 sub-pass slot (r14+ visual realism + r30 Cinematic DoF) は attachment / slot 配置のみ実装、shader port は r42-α/β/γ scope | pass 2 gbuffer3 attachment 配置成功 (r21.1 picker LocalID/ObjectID 格納可能 format で format support 確認) + pass 6 7 sub-pass slot 全確保 + r42-α/β/γ 13 shader file は本領域で **touch 0 件** (`git diff` verify) |
| **#5-領域 7 (LLVKRenderer skeleton signature 整合)** | charter §3 #6: descriptor / pass API signature が r41.5 interface 経由 call 化前提担保、05 §10.1-§10.2 hook と一致 | `grep -rE "class LLVKRenderer" indra/` で skeleton declaration 存在 + 領域 7 で確定した descriptor 配信 + dynamic rendering call signature が 05 §10.1-§10.2 hook と一致 |
| **regression (段階 1-3 動作維持)** | 段階 1-3 acceptance 維持 + viewer 起動 + AYAstorm 機能 (audio / chat / login / inventory) regression 0 件 | `01-foundation.md` §4.1 + `02-portage-execution.md` §4.1 + `03-state-machine-pso.md` §4.1 acceptance 再 verify + sustained ~10 分動作 + AYA 起動確認 PASS (`feedback_release_with_user_feedback.md` 遵守) |

### §4.2 不達時の対処 (charter §3 acceptance 運用方針継承)

- 6 criterion のうち 1 件でも未達 = 領域 7 未達 (段階 4 着手保留判断、charter §7.5 boundary refine で領域別 partial pass も可)
- 未達 criterion 別に対処 (例: #5-領域 7 で specific pass の image layout transition mismatch validation error 残存 → 05 §5.3 layout transition 表 audit → barrier batching refine → 再 sweep)
- **defer / disable 提案 ban** (`feedback_self_bug_no_defer_option.md` 遵守、fix 案のみ提示)
- **仮説 2 連続外れ rule** (`feedback_admit_unknown.md` 遵守): descriptor binding mismatch / push descriptor range violation / image layout mismatch で仮説 2 連続外れたら gdb breakpoint at `vkCmdBindDescriptorSets` / validation layer message detail / RenderDoc capture で実データ取得に切替

### §4.3 領域 7 完遂後の次 段階

- **段階 4 着手判断** (pipeline.cpp 3 大グローバル → LLPipelineFrameContext、charter §2 領域 4、本領域 7 完遂で descriptor / pass 基盤確定前提)
- **領域 6 完遂判断**: 本領域 7 と並走の領域 6 sub-doc 06 が completion 状態か AYA 確認 (本領域 sub-step 7.2-7.4 = 領域 6 sub-step 6.4 と同期完遂が前提)
- **handoff doc**: `handoff-stage-7-complete.md` 作成 (領域 7 完遂境界、`feedback_proactive_handoff.md` 遵守)
- **r42-α/β/γ 移行 prep**: r41 完遂後 r42-α/β/γ で AYAstorm 改変 13 file shader port 時、本領域 7 で確立した descriptor set 3 階層 + 7 pass chain attachment 配置を r42 で流用 (pass 2 gbuffer3 picker / pass 6 sub-pass slot)

---

## §5 関連 doc / memory

### §5.1 直接参照 doc

| doc | 本 sub-doc での参照 section |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/00-charter.md` | §2 領域 7 (1.50 PM 中 risk) + §3 #5 acceptance + §7.4 sub-doc 構成 + §7.5 boundary refine 可 |
| `docs/specs/ayastorm-r41-gl-removal/01-foundation.md` | §3.4 sub-step 範式継承 + §3.5 段階 1 で touch しない file の領域 7 並走着手前提反映 |
| `docs/specs/ayastorm-r41-gl-removal/02-portage-execution.md` | §3.3 段階 2 で touch しない file の領域 7 並走前提 + §1.3 並走領域協調事項 (pass 2 deferred g-buffer pool 群 attachment binding 確定) |
| `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` (本 sub-doc と同時起草) | §1.3 並走領域協調事項 + §3 sub-step 3.3 matrix stack → push constant 化と本領域 sub-step 7.4 push constant range 設計同期 + §3 sub-step 3.4 12 pool hook body PSO bind と本領域 set=0/1/2 bind 同期 |
| `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` (本 sub-doc と同時起草) | §3 sub-step 6.4 descriptor set binding 統合と本領域 sub-step 7.2-7.4 同期 (shader 側 `layout(set=N, binding=M)` qualifier 配信が本領域 `VkDescriptorSetLayoutBinding` 設計に従う) |
| `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` | §1 Vulkan 1.3 + extension 採用 (本 §1.1) + §3 descriptor set 3 階層 sampler 206 個分布 (本 §1.2.1) + §3.3 push descriptor 採用 (本 §1.2.1 set=2) + §3.4 r21.1 picker buffer 配置 (本 §1.4) + §3.5 pipeline layout 設計 (本 §1.2.2) + §3.6 descriptor pool sizing (本 §3.1 sub-step 7.1) + §4.1 dynamic rendering 採用判断 (本 §1.2.3) + §4.2 deferred g-buffer attachment 配置 (本 §1.2.3 pass 2) + §4.3 render pass chain 全体図 (本 §1.2.3 7 pass) + §4.4 r14+ post-process pass chain 統合 (本 §1.4 r42-γ scope) + §4.5 r21.1 picker attachment 統合 (本 §1.4 r42-α scope) + §4.6 sky / atmospherics pass (本 §1.2.3 pass 5) + §4.7 LLRenderTarget → Vulkan attachment 移行マップ (段階 3 sub-step 3.3 と同期) + §5 sync 戦略 + §6 VMA 採用 (本 §3.1 sub-step 7.1) + §9 extension 採用 (本 §1.1) |
| `docs/specs/ayastorm-r40-vulkan-migration/06-effort-estimation.md` | §3.1 領域 7 PM 1.50 (本 §1 + §3 領域 7 scope 整合) |
| `docs/specs/ayastorm-r40-vulkan-migration/04-portage-inventory.md` | §5.1 r21.1 self-rigged picker touchpoint (本 §1.4 r42-α scope 境界) + §5.2 r14+ visual realism touchpoint (本 §1.4 r42-γ scope 境界) + §5.3 r30 Cinematic mode touchpoint (本 §1.4 r42-β scope 境界) |

### §5.2 関連 memory

| memory | 本 sub-doc での参照 |
|---|---|
| `project_ayastorm_r41_vulkan_migration.md` | r41 milestone active 状態 + 領域 7 並走着手前提 |
| `project_ayastorm_three_platforms.md` | 3 OS 大前提 + Linux 先行例外 (本領域 7 も Linux 限定動作確認、§4.1 #regression 反映)、Mac MoltenVK は push descriptor + dynamic rendering portable subset 整合 |
| `reference_gbuffer3_storage.md` | pass 2 gbuffer3 attachment 配置時の RGBA 化保持 (AYAstorm r21.1 picker 同居前提、本 §1.4 r42-α scope 境界 + §1.2.3 pass 2) |
| `reference_deferred_shader_routing.md` | pass 2 deferred g-buffer pool 群 attachment binding 確定時の deferred shader routing reference |
| `project_aya_visual_realism_alpha_protect.md` | pass 6 post-process chain (r14+ visual realism、r42-γ scope 外) の `frag_color.a = 0` invariant 認識 (本領域 untouched 維持で保護) |

### §5.3 関連 feedback

| feedback | 本 sub-doc での参照 |
|---|---|
| `feedback_experiment_branch_single_scope.md` | branch scope 単一性 (r41 内 sub-branch 作らない、`01-foundation.md` §2.3 継承) |
| `feedback_no_auto_commit.md` | commit は AYA 指示後 |
| `feedback_release_flow.md` | push は AYA 手動 |
| `feedback_self_bug_no_defer_option.md` | §4.2 defer / disable 提案 ban + r42 scope shader (13 file) touch 0 件維持の選択肢化禁止 |
| `feedback_self_verify_before_handoff.md` | §3 sub-step 7.5 self-check + §4.3 領域 7 完遂境界 self-trace |
| `feedback_proactive_handoff.md` | §4.3 領域 7 完遂時の handoff doc 作成 |
| `feedback_build_only_verified.md` | §4 completion criteria の satisfy は実機検証 (推論 ban) |
| `feedback_admit_unknown.md` | §4.2 descriptor binding mismatch / push descriptor range violation / image layout mismatch で仮説 2 連続外れたら gdb / validation detail / RenderDoc capture 切替 |
| `feedback_use_agents_proactively.md` | §2.3 + §3.2 sub-step 7.2 / 7.3 / 7.4 (set=0/1/2 配線 independent) + sub-step 7.5 内 pass 4/5/6/7 並列着手で Agent 活用 |
| `feedback_perf_map_bfs_drill.md` | §2.2 dependency graph の階層的 sub-step 化 (VMA → set=0 → set=1 → set=2 → 7 pass chain 5 階層) |
| `feedback_release_with_user_feedback.md` | §4.1 #regression の exhaustive solo session 不要 (AYA 起動確認 PASS で sufficient) |
| `feedback_one_step_at_a_time.md` | §3 sub-step 単位で 1 メッセージ 1 アクション着手 |
| `feedback_render_full_trace_first.md` | sub-step 7.5 7 pass chain attachment + sync barrier 配線は 05 §5.3 layout transition 表を上から下まで trace、当てずっぽう barrier insertion 禁止 |

---

## §6 起草 cadence + 完成宣言

### §6.1 本 sub-doc 起草情報

- **起草着手**: 2026-05-28
- **起草主体**: AYA + Claude
- **起草先**: `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` (本 doc)
- **起草 cadence**: **Pattern α (一括 draft)** — `01-foundation.md` / `02-portage-execution.md` / `03-state-machine-pso.md` / `06-shader-spirv.md` 範式継承、sub-doc は内容具体 (3 set + 7 pass / 5 sub-step / acceptance 6 件) で section 数少なく、Pattern β 分割 overhead 回避 (charter §7.5 boundary refine 可)
- **scope**: **領域 7 only + r42 scope shader 13 file untouched 維持** (descriptor set 3 階層 + 7 pass chain + dynamic rendering 実装)、r42-α/β/γ の AYAstorm 改変 13 file shader port は別 milestone scope、本領域では attachment / slot 確保のみ
- **並走起草 sub-doc**: 同 session で `03-state-machine-pso.md` (段階 3 = 領域 3) + `06-shader-spirv.md` (領域 6) を Pattern α 一括起草、3 doc 同時 AYA review (handoff-stage-2-complete §2.3 B 案、AYA 採用)

### §6.2 sub-doc 番号付与の justification (charter §7.4 outline → 領域番号同期 refine)

`03-state-machine-pso.md` §6.2 + `06-shader-spirv.md` §6.2 と同等の justification。本 sub-doc は **`07-descriptor-renderpass.md`** で確定 (charter §7.4 outline `04-descriptor-render-pass.md` から refine、領域番号 = sub-doc 番号同期方針)。

| charter §7.4 outline | 本 r41 章実採用 | 採用理由 |
|---|---|---|
| `04-descriptor-render-pass.md` | **`07-descriptor-renderpass.md`** | sub-doc 番号 = charter §2 領域番号 (領域 7) で統一、r41 章内 cross reference 整合 |

本 §6.2 の番号 refine は charter §7.5 boundary refine 可の範囲内、AYA review で承認後に正式採用。

### §6.3 完成宣言条件

本 sub-doc は **AYA review PASS で完成宣言**、status field を `closed YYYY-MM-DD (Pattern α 一括 draft + AYA review PASS、領域 7 着手準備 ready)` に更新。

完成宣言後の次 action:

- 領域 7 sub-step 7.1 着手 (VMA + descriptor pool sizing 基盤)
- 領域 7 sub-step 7.5 完遂時に handoff doc `handoff-stage-7-complete.md` 作成
- r41 全完遂後の r42-α/β/γ で AYAstorm 改変 13 file shader port (本領域 7 で確立した pass 2 gbuffer3 picker / pass 6 sub-pass slot を流用)

### §6.4 本 sub-doc commit 反映

本 sub-doc 完成宣言 commit は AYA 明示指示後 Claude が実施。commit message draft (AYA 指示時 refine 可):

```
docs(r41): sub-doc 07-descriptor-renderpass.md 完成 + 領域 7 prep

- 07-descriptor-renderpass.md 新規作成 (Pattern α 一括 draft + AYA review PASS)
- §1 領域 7 scope (descriptor set 3 階層 + 7 pass chain + dynamic rendering、1.50 PM 中 risk、領域 3/6 並走必須)
- §1.2 set=0 per-frame / set=1 per-material / set=2 per-draw + VK_KHR_push_descriptor + 7 pass chain
- §1.4 r14+/r21.1/r30 Cinematic shader 13 file は r42-α/β/γ scope (本領域は attachment/slot 確保のみ)
- §2 実装順序 + dependency graph (VMA → set=0 → set=1 → set=2 → 7 pass chain 5 階層)
- §3 領域 7 sub-step 5 件 (7.1 VMA / 7.2 set=0 / 7.3 set=1 / 7.4 set=2 + push descriptor / 7.5 7 pass + self-check)
- §4 領域 7 completion criteria 6 件 (descriptor 3 階層 / 7 pass chain dynamic rendering / attachment+sync / r42 scope untouched / skeleton signature / regression)
- §5 関連 doc/memory + §6 起草 cadence (Pattern α / 領域 7 only + r42 scope 13 file untouched)
- §6.2 sub-doc 番号付与 refine justification (charter §7.4 outline → 領域番号同期)
```

push は AYA 手動 (`feedback_release_flow.md` 遵守)。
