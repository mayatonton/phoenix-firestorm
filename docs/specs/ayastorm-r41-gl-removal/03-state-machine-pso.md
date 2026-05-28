# AYAstorm r41 sub-doc 03-state-machine-pso — 段階 3 llrender state machine → Vulkan PSO 化

**status**: **closed 2026-05-29 (Pattern α 一括 draft、AYA review PASS)**
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
| 3 | llrender.cpp | 2,207 | matrix stack (modelview / projection) + immediate-mode emulation + texture unit state + LLRender 中心 dispatcher | matrix stack → **push constant (mat4 = 64 bytes)** へ移行 (05 §3.5 採用)、texture unit state → set=1 per-material binding mapping、immediate-mode は段階 2 hook 化済の `recordPoolDraws(VkCommandBuffer)` 側で吸収 |
| 4 | llrender.h | 582 | LLRender public interface + matrix mode enum + texture unit slot 定数 | interface 信号 (signature) は **段階 4 frame context refactor 前提で維持**、内部実装のみ PSO 化、enum alias を VkPipelineLayoutCreateInfo 由来 stage 定数に並走追加 |
| 5 | llimagegl.cpp | 2,663 | GL texture object lifecycle (glGenTextures / glTexImage2D / glTexParameteri / glDeleteTextures 等) + format conversion | `VkImage` + `VkImageView` + `VMA allocation` lifecycle (06 §6 VMA 採用)、format conversion table を `VkFormat` 表へ置換、descriptor set binding 経由 sampler 参照は領域 7 並走で配線 |
| 6 | llimagegl.h | 371 | LLImageGL public interface + GL texture target enum (TEXTURE_2D / CUBE_MAP 等) | interface signature 維持、target enum を VkImageViewType (VK_IMAGE_VIEW_TYPE_2D / CUBE 等) alias へ並走追加 |
| 7 | llrendertarget.cpp | 589 | FBO (glGenFramebuffers / glFramebufferTexture2D / glBindFramebuffer) + draw buffer 配列管理 | **VK_KHR_dynamic_rendering 採用 (05 §4.1)** → `VkRenderPass` + `VkFramebuffer` 廃止、`bindTarget()` / `flush()` interface は **inline `VkRenderingAttachmentInfo` 構築 + `vkCmdBeginRenderingKHR` / `vkCmdEndRenderingKHR`** に置換 (05 §4.7 移行マップ反映) |
| 8 | llrendertarget.h | 194 | LLRenderTarget public interface + draw buffer attachment 定義 | interface signature 維持 + dynamic rendering 内部実装に置換、attachment 定義を `VkFormat` + load/store op alias 化 |
| 9 | llpostprocess.cpp | 454 | post-process state machine (color matrix / extract / contrast / noise 等の legacy effect uniform 配信) | **r14+ visual realism shader 7 file (r42-γ scope) は touch しない** (charter §3 #4 reg 担保)、本段階では legacy effect uniform 配信を push constant + per-frame UBO へ置換 |
| 10 | llpostprocess.h | 267 | LLPostProcess public interface + effect mode enum | interface signature 維持、effect mode enum は段階 4 LLPipelineFrameContext 統合前提で並走維持 |

**特記**:
- **#1 llgl.cpp + #2 llgl.h** は state machine の root、PSO 内 state alias 化が段階 3 最大 refactor (charter §2 領域 3 risk = 高 の主要因)
- **#3 llrender.cpp** matrix stack は push constant 64 bytes 制約 (05 §3.5、Vulkan minimum 128 bytes 内) に収まる、ただし shader 側 layout 配線変更必須 → 領域 6 並走
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
| **3.1** | PSO 基盤 + state alias root | llvkloader.{cpp,h} 拡張 (VkPipelineCache + VkPipelineLayout 標準形 + PSO compile helper) + `llgl.{cpp,h}` の RAII state class → PSO state alias 化 (3,514 LOC) | 起動時 VkPipelineCache 作成成功 + 最小 PSO (sky pool 用 placeholder) compile 成功 + 動作中 1 frame 内に PSO bind が validation 0 件で完了、`LLGLDepthTest` / `LLGLSDefault` 等の setter が PSO state alias 経由 no-op 化、bridging template 確定 |
| **3.2** | 軽量 smoke-test port | llpostprocess.{cpp,h} (legacy effect、721 LOC) | legacy post-process effect の uniform 配信 → push constant + per-frame UBO 化、起動 + 1 セッション validation 0 件 |
| **3.3** | 標準 PSO 配線 (matrix stack + FBO → dynamic rendering) | llrender.{cpp,h} (2,789) + llrendertarget.{cpp,h} (783) = 4 file 3,572 LOC | matrix stack → push constant 化動作 + texture unit → set=1 mapping 動作 + FBO → `vkCmdBeginRenderingKHR` 化動作、領域 7 並走 sub-step との descriptor set 整合確認 |
| **3.4** | texture lifecycle + 段階 2 引継ぎ特殊対応 | llimagegl.{cpp,h} (3,034 LOC) + 12 pool hook body PSO bind 配線 + terrain glTexGen 廃止 + avatar SSBO 基本実装 | VkImage + VkImageView + VMA lifecycle 動作 + 12 pool 全 hook 内 `vkCmdDraw*` 投入動作 + render path GL call 削除 (acceptance #1-段階 2 satisfy) + terrain.cpp 内 `glTexGen` 0 件 + avatar.cpp 内 bone matrix → VkBuffer 配線 + validation 0 件 |
| **3.5** | 段階 3 self-check + validation strict 検証 + handoff doc | (本 sub-step) | §4.1 acceptance 5 件 self-trace PASS (charter §3 #1/#3/#5/#6 段階 3 分 + regression) + validation strict force-enable build で validation 0 件再確認 + handoff doc `handoff-stage-3-complete.md` 作成 |

### §3.2 sub-step 内 file 順序の柔軟性 (charter §7.5 boundary refine 可)

- 各 sub-step 内の file 順序は実装着手時に bridging code 肥大度 / 領域 6 SPIR-V port 進捗 / 領域 7 descriptor binding 確定状況で refine 可
- sub-step 3.3 の llrender + llrendertarget は dependency graph 上 independent → 並列着手候補 (Agent 活用)
- sub-step 3.4 の 12 pool hook body 配線は drawpool 単位で independent → 並列着手候補

### §3.3 段階 3 で touch しない file (段階 4-5 + 領域 6/7 scope、`02-portage-execution.md` §3.3 範式継承)

- pipeline.cpp 3 大グローバル (`sCull` / `sShadowRender` / `sCurCameraID`) — 段階 4 frame context 化
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
| **#1-段階 3 (llrender 主要 5 file + lldrawpool 13 file の GL call 除去)** | llrender 5 file (10,841 LOC) + lldrawpool 13 file 内で `gl[A-Z][a-zA-Z]+\s*\(` 直接呼出が 0 件、PSO bind + `vkCmdDraw*` + dynamic rendering call 経由に置換済 | `grep -rE "gl[A-Z][a-zA-Z]+\s*\(" indra/llrender/llgl.{cpp,h} indra/llrender/llrender.{cpp,h} indra/llrender/llimagegl.{cpp,h} indra/llrender/llrendertarget.{cpp,h} indra/llrender/llpostprocess.{cpp,h}` が **0 件 hit** + `grep -rE "gl[A-Z][a-zA-Z]+\s*\(" indra/newview/lldrawpool*.cpp` が **0 件 hit** (段階 2 引継ぎ satisfy 後) |
| **#3-段階 3 (PSO bind 動作 + validation 0 件)** | 12 pool 全 hook で PSO bind + `vkCmdDraw*` 動作、validation layer error / warning 0 件 | viewer 起動 + login 後 sustained ~10 分動作 + 各 pool の PSO bind 通過確認 (LL_INFOS log + per-pool PSO compile counter)、`VK_LAYER_KHRONOS_validation` で起動 + 動作中 error / warning 0 件 (sub-step 3.5 で validation strict force-enable build) |
| **#5-段階 3 (descriptor set + render pass 実装、領域 7 並走 satisfy)** | set=0 per-frame / set=1 per-material / set=2 per-draw 3 階層 + `VK_KHR_push_descriptor` (set=2) + `VK_KHR_dynamic_rendering` 採用動作 | validation layer で descriptor binding mismatch / render pass dependency violation **0 件** + `grep -E "VK_KHR_push_descriptor\|vkCmdPushDescriptorSetKHR" indra/llrender/` で配線 hit + `grep -E "vkCmdBeginRenderingKHR\|VkRenderingAttachmentInfo" indra/llrender/` で配線 hit |
| **#6-段階 3 (LLVKRenderer skeleton signature 整合)** | charter §3 #6: pipeline.cpp 内 inline 実装と LLVKRenderer skeleton hook の signature 整合、r41.5 interface 経由 call 化前提担保 | `grep -rE "class LLVKRenderer" indra/` で skeleton declaration 存在 + 段階 3 で確定した PSO bind / pool record / descriptor 配信 関数 signature が 05 §10.1-§10.2 hook と一致 |
| **特殊対応 (terrain glTexGen 廃止 + avatar skinning SSBO)** | sub-step 3.4 で段階 2 引継ぎ 2 件 satisfy、shader 側 explicit UV 化 + avatar bone matrix → VkBuffer 配線 | `grep -E "glTexGen" indra/newview/lldrawpoolterrain.cpp` が **0 件 hit** + shader 側 UV attribute / uniform 配線確認 + avatar.cpp 内 bone matrix → VkBuffer (storage buffer) 配線確認 + descriptor binding validation 0 件 |
| **regression (段階 1-2 動作維持)** | Vulkan instance + device + command pool + render pass + 12 pool record hook 動作維持、viewer 起動 + AYAstorm 機能 (audio / chat / login / inventory) regression 0 件 | `01-foundation.md` §4.1 段階 1 + `02-portage-execution.md` §4.1 段階 2 acceptance 再 verify + sustained ~10 分動作 + AYA 起動確認 PASS (`feedback_release_with_user_feedback.md` 遵守で exhaustive solo session 不要) |

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
