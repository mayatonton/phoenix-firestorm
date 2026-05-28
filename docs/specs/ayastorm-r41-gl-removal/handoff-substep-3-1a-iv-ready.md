# r41 sub-step 3.1a-i/ii 完了 → 3.1a-iv 着手境界 handoff (2026-05-29)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-stage-2-complete.md` (段階 2 完遂 → 段階 3 着手境界)
**本 handoff の位置付け**: sub-step 3.1a (PSO 基盤 + state alias root の **trace + bridging 設計**) を 4 段階 (i/ii/iii/iv) に AYA 指示で再 scope。本 session で **3.1a-i (trace inventory) + 3.1a-ii (bridging 設計素材収集)** を 6 Agent cluster 並列で完遂、**3.1a-iii (PD 算出) は AYA 指示で de-prioritize**、**3.1a-iv (sub-doc 03 spec 訂正 + handoff-substep-3-1a-complete.md 起草)** が次 session 最優先 deliverable。AYA 明示 guidance「工数はやってみれば変動する / 本 step を正しい結果で積み上げて以降の step を盤石に」を反映した境界。

---

## 1. session 経緯 (compaction を跨ぐので明示記録)

### 1.1 本 handoff 直前の session 流れ

1. `handoff-stage-2-complete.md` を起点に段階 3 sub-doc 03/06/07 trio を Pattern α 一括 draft → AYA review PASS → commit `f6a5503c09`
2. sub-step 3.1 着手指示 → 3.1a (trace-before-implement、`feedback_render_full_trace_first.md` 遵守) 着手
3. 初手 trace Agent が「risk: low」と楽観評価 → AYA 反論「素直には繋げられないものが多数という認識でよいですか？」 → Claude 楽観評価を撤回、11 件 非自明 bridging items を列挙
4. AYA 追加指示「つなげるための調査と設計、及び各作業の項目と工数をここで算出して訂正及び加筆が必要ではないでしょうか？」 → **3.1a を 4 段階 (i/ii/iii/iv) に再 scope**:
   - i: trace (inventory)
   - ii: bridging 設計素材収集
   - iii: PD 算出
   - iv: 03 spec 訂正 + handoff doc 起草
5. **6 Agent cluster 並列発射** (Cluster A/B/C/D/E/F) → trace 結果を本 session で吸収
6. Cluster C で Agent が「106+ sampler breach の可能性」とエスカレート → AYA 反論「これは作業量が想像よりずっと多いということを言っていますか?」 → Claude 撤回、`feedback_doubt_self_first.md` 違反を自認 (Agent speculation を自分でも疑わなかった)
7. PD 精度をどうするか質問 → **AYA guidance シフト**: 「工数はやってみれば変動するものですからこの数字の精度をどうこう言ってもしかたないと思っています」「本 Step を正しい結果で積み上げる方にこそ重点において以降の Step を盤石にしましょう」
8. /loop r41 sub-step 2.5 build watch (input mismatch、build 不在) → handoff 提案 → AYA 「OK」 → 本 doc 起草

### 1.2 sub-step 3.1a 4 段階 進捗 (本境界での state)

| stage | scope | state |
|---|---|---|
| **3.1a-i** | llgl.{cpp,h} + 関連 caller の GL state machine trace inventory (file / line / 関数別) | **完了 2026-05-29** (Cluster A/B/D/E/F 並列で trace、本 doc §2 に sealed) |
| **3.1a-ii** | 11 件非自明 bridging items の繋ぎ方設計 (素材収集) | **完了 2026-05-29** (Cluster A〜F trace 結果から本 doc §3 で 11 件設計素材 sealed) |
| **3.1a-iii** | 作業項目化 + PD 算出 | **AYA 指示で de-prioritize** (charter §6 1.50 PM 据置き、やってみて変動)、本 doc §4 で軽い refresh のみ |
| **3.1a-iv** | sub-doc 03-state-machine-pso.md spec 訂正 + handoff-substep-3-1a-complete.md 起草 | **次 session 最優先 deliverable** (本 doc §5 で要訂正 section 一覧と訂正方針を sealed) |

### 1.3 status close / next active

| doc / memory | status |
|---|---|
| `handoff-stage-2-complete.md` | **役割完了 2026-05-29** (段階 3 sub-doc 03/06/07 trio commit + 3.1a 着手 satisfy) |
| `handoff-substep-3-1a-iv-ready.md` (本 handoff) | 新規作成 (3.1a-i/ii 完了 → 3.1a-iv 着手境界) |
| sub-doc `03-state-machine-pso.md` | **active** (本境界での `closed 2026-05-29` status は AYA review PASS 時点の状態、本 doc §5 で §1.5 等を追加加筆する予定 — つまり 3.1a-iv で再 open / re-close) |
| sub-doc `06-shader-spirv.md` / `07-descriptor-renderpass.md` | active (3.1a-iv で関連箇所 cross-update 必要なら同 session 内で並走訂正) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 |

---

## 2. 3.1a-i Trace Inventory (sealed、Agent 並列 trace 結果)

本 section は Cluster A / B / D / E / F の trace 結果を sealed 形で次 session に持ち越すための保全。再 trace 不要。

### 2.1 Cluster A: llgl.cpp GL call inventory + state class implementations

**llgl.cpp = 3,027 LOC、23 unique GL functions / 79 total direct calls**

| section | line range | 内容 |
|---|---|---|
| PFNGL function pointer declarations | L227-992 | ~765 LOC、extern function pointer typedef (volk 化対象、charter §3 #1 acceptance) |
| LLGLManager init / extensions | L1075-1810 | ~735 LOC、`glGetString` / `glGetIntegerv` / extension query (Vulkan は `VkPhysicalDeviceFeatures` + `VkPhysicalDeviceProperties` 化、Cluster E と関連) |
| LLGLState (RAII root) | L2552-2624 | ~73 LOC、setter は `glEnable` / `glDisable` 直接呼出、PSO state alias 化対象 (本 §3 設計項目 1) |
| LLGLUserClipPlane | L2767-2820 | ~54 LOC、`glEnable(GL_CLIP_DISTANCE0)` + shader 側 clipPlane uniform 連携 (Cluster B で clipF.glsl 安全確認済) |
| LLGLDepthTest | L2822-2906 | ~85 LOC、`glDepthFunc` / `glDepthMask` 動的切替、Vulkan は **VK_EXT_extended_dynamic_state2 (1.3 core) で dynamic state 化** (本 §3 設計項目 2) |
| LLGLSquashToFarClip | L2908-2943 | ~36 LOC、projection matrix の depth-squash 用 push constant 化 (本 §3 設計項目 3) |
| LLGLSyncFence | L2947-2991 | ~45 LOC、`glFenceSync` / `glClientWaitSync`、**caller = 0 件 (dead code、Cluster F 確認済)** → 削除候補 (本 §3 設計項目 11) |

### 2.2 Cluster A: llgl.h state class declarations (487 LOC)

llgl.h L? = 12 state class declarations (詳細は llglstates.h 197 LOC に分散):

- `LLGLDepthTest` / `LLGLSDefault` / `LLGLSObjectSelect` / `LLGLSUIDefault` / `LLGLSPipeline` / `LLGLSPipelineAlpha` / `LLGLSPipelineSelection` / `LLGLSPipelineSkyBox` / `LLGLSPipelineDepthTestSkyBox` / `LLGLSPipelineBlendSkyBox` / `LLGLSTracker` / `LLGLSSpecular`

これらは PSO 内 state alias header に refactor、setter は no-op 化 (PSO compile 時 state 固定の Vulkan モデルに整合)。

### 2.3 Cluster A: caller 集計

- 46 + 43 file (state class header 経由 + 直接 llgl.h include)
- 195 + 85 direct usage line
- 大半は RAII stack 構築 (`LLGLDepthTest depth(GL_TRUE);` 等)、setter no-op 化で source-level compat 維持可能 (charter §1 thesis = parity 不要、ただし内部 GL call 削除が acceptance #1)

### 2.4 Cluster B: AYAstorm 改変 13 file shader boundary

**clip_plane 経路**:
- `clipF.glsl` = LL 標準 fragment shader (AYAstorm 未改変)、`LLGLUserClipPlane` 連携 PSO 化で `clip distance` を `VkPipelineRasterizationStateCreateInfo` / shader-side `gl_ClipDistance` 経由維持 → **safe (改変なしで PSO 化可)**

**skybox 経路**:
- `skyV.glsl` = AYAstorm r14 改変含む (r14+ visual realism scope) が、改変範囲は L65-225 に **isolated**
- L95 の **push constant matrix swap** (`mat4` viewProj 注入) は r14 改変 block と independent
- → PSO 化対応で L95 swap を `VkPushConstantRange { offset=0, size=64, stageFlags=VK_SHADER_STAGE_VERTEX_BIT }` に置換、r14 改変 block (L65-225) は untouched 維持可能 → **safe**

**結論**: charter §3 #4 regression acceptance (AYAstorm 改変 13 file は r41 内 untouched) は 3.1a で **clip + sky の 2 経路は安全確認済**、残 11 file は段階 3 後続 sub-step (3.4 特殊対応) + r42-α/β/γ scope で順次確認。

### 2.5 Cluster C: texture unit / descriptor set 配信

**事実 (再確認済)**:
- 74 files × 633 grep hits (texture unit 系 GL call)、PBR 系 texture slot 数 = **7** (DIFFUSE / NORMAL / SPECULAR / BASECOLOR / METALLIC_ROUGHNESS / GLTF_NORMAL / EMISSIVE)
- per-draw binding は 6-8 samplers (avatar attachment 含む)、**VK_KHR_push_descriptor の 32 binding minimum を十分に下回る**

**Cluster C false alarm 訂正 (本 session で確定済)**:
- Agent の「106+ sampler breach 可能性」は texture slot 総数 (全 material × 全 PBR map 合計の上界) を per-draw binding と混同した speculation
- 実際の per-draw は 6-8 samplers、descriptor set=2 push descriptor で十分収容
- 設計対応: **measurement-first** で `VkPhysicalDeviceProperties.limits.maxBoundDescriptorSets` + `maxPushDescriptors` を `LLVKLoader::createDevice` で query → log 出力で baseline 取得、その後で sub-step 3.4 texture lifecycle 配線時に AYA 環境実測で確定
- → **redesign-first は不要**、現状の sub-doc 03 §1.4 + 07 §3 設計を維持

### 2.6 Cluster D: PFNGL 削除 + 189 file 波及

**事実**:
- llgl.cpp 内 PFNGL declarations 765 LOC + ProcAddr loading 310 LOC + extern declarations 718 = **計 ~1,793 LOC 規模**
- llglheaders.h 経由で **189 file に波及** (GL 関数 prototype 利用)

**設計**:
- volk が `vkGetInstanceProcAddr` + `vkGetDeviceProcAddr` 経由で全 Vulkan 関数 pointer を auto-load (段階 1 で完了)
- GL 側 PFNGL を一括削除すると 189 file の include 連鎖で広範な compile error 発生 → **段階 5 (残依存解決) と一体運用が望ましい**
- 段階 3 内では llrender 5 file のみ GL call 削除 (acceptance #1-段階 3)、PFNGL declarations の物理削除は段階 5 で実施 (波及 segment 化)

### 2.7 Cluster E: glHint + VRAM detection

**事実**:
- `glHint` 呼出は llgl.cpp 内に少数 (capability hint、trivial)
- VRAM detection は `glGetIntegerv(GL_GPU_MEMORY_INFO_*)` (NVX extension) を使用、Mesa 環境では分岐

**設計**:
- `glHint` は **trivial deletion** (Vulkan に hint API 無し、PSO compile 時に driver 最適化任せ)
- VRAM detection は **`VkPhysicalDeviceMemoryProperties` で unified** (heap iteration で device-local heap size 取得、3 driver baseline 全対応)

### 2.8 Cluster F: LLGLSyncFence dead code + debug callback

**LLGLSyncFence (llgl.cpp L2947-2991, 45 LOC)**:
- caller grep: **0 件 hit** (本 fork で完全 dead code)
- 削除候補 (本 §3 設計項目 11)、削除すれば段階 3 内で対応不要

**gl_debug_callback (`GL_ARB_debug_output`)**:
- llgl.cpp 内に GL debug message callback 配線あり
- **VK_EXT_debug_utils messenger に直接置換可** (段階 1 で `VkDebugUtilsMessengerEXT` 配線は validation strict 検証で動作確認済、本 callback はその hook を再利用)

---

## 3. 3.1a-ii Bridging 設計素材 (sealed、11 件非自明項目)

本 section は次 session の 3.1a-iv で sub-doc 03 §1.5 (新規追加予定) に転記する設計素材。AYA guidance「正しい結果で積み上げる」を反映、PD 数値は付随情報として軽く付ける。

### 3.1 11 件 非自明 bridging items 一覧

| # | item | 設計方針 | 主担当 sub-step | 備考 |
|---|---|---|---|---|
| 1 | **LLGLState RAII setter no-op 化** | PSO compile 時 state 固定モデルに整合、setter は dead-store 化、caller (46+43 file) source-level compat 維持 | 3.1 (基盤 + state alias root) | charter §1 thesis 整合、parity 不要 |
| 2 | **LLGLDepthTest dynamic state 化** | `VK_EXT_extended_dynamic_state2` (Vulkan 1.3 core) で `vkCmdSetDepthTestEnable` / `vkCmdSetDepthCompareOp` / `vkCmdSetDepthWriteEnable` 経由動的切替、3 driver baseline (NVIDIA / RADV / ANV) で query 確認 | 3.1 | 3 driver 全 1.3 core で支持、追加 extension 不要 |
| 3 | **LLGLSquashToFarClip push constant 化** | projection matrix depth-squash を push constant range 内に統合 (matrix stack push constant 64 bytes 共有) | 3.1 / 3.3 | shader 側 layout 配線変更必須 (領域 6 並走) |
| 4 | **LLGLUserClipPlane 経路 PSO 化** | `VkPipelineRasterizationStateCreateInfo` で clip distance enable、shader-side `gl_ClipDistance` 経由値配信、clipF.glsl 改変なし | 3.1 / 3.3 | Cluster B safe 確認済 |
| 5 | **skybox skyV.glsl L95 push constant swap 維持** | matrix stack push constant 化に伴う swap、r14 改変 L65-225 と independent | 3.1 / 3.3 | Cluster B safe 確認済 |
| 6 | **matrix stack → push constant 64 bytes 化** | modelview + projection を mat4 × 1 (64 bytes) で push constant、shader layout cross-update (領域 6 並走) | 3.3 | Vulkan minimum 128 bytes 内、余裕あり |
| 7 | **texture unit → descriptor set=1 per-material mapping** | 7 PBR slot (DIFFUSE/NORMAL/SPECULAR/BASECOLOR/METALLIC_ROUGHNESS/GLTF_NORMAL/EMISSIVE) を set=1 binding 0-6 配置 | 3.4 (texture lifecycle) | Cluster C measurement-first で AYA 環境実測確定 |
| 8 | **VkImage + VkImageView + VMA lifecycle** | LL `LLImageGL` を VkImage + VMA allocation + VkImageView trio で置換、format conversion table を VkFormat 表に化 | 3.4 | VMA は段階 1 で device 配線済、本 sub-step で alloc API 活用 |
| 9 | **FBO → VK_KHR_dynamic_rendering** | `vkCmdBeginRenderingKHR` / `vkCmdEndRenderingKHR` + inline `VkRenderingAttachmentInfo`、`VkRenderPass` + `VkFramebuffer` 廃止 (05 §4.7 移行マップ準拠) | 3.3 | 3 driver baseline 全支持 (Vulkan 1.3 core) |
| 10 | **gl_debug_callback → VK_EXT_debug_utils messenger 移管** | 段階 1 messenger 配線を reuse、GL ARB debug output callback を Vulkan 側に集約 | 3.1 | 段階 1 で動作実績あり (validation strict 検証で確認済) |
| 11 | **LLGLSyncFence dead code 削除** | caller 0 件確認済、llgl.cpp L2947-2991 物理削除で段階 3 scope 縮減 | 3.1 | Cluster F 確認、削除のみで段階 3 acceptance #1 への副作用 0 |

### 3.2 各 sub-step マッピング (sub-doc 03 §3.1 と整合)

| sub-step (現 03 §3.1) | 本 §3.1 設計項目 |
|---|---|
| 3.1 (PSO 基盤 + state alias root) | 1 / 2 / 4 / 10 / 11 (+ 3 / 5 部分前倒し可) |
| 3.2 (軽量 smoke-test = llpostprocess) | (新規 bridging item 無し、本 sub-doc §3 設計済 effect uniform → push constant + UBO) |
| 3.3 (標準 PSO 配線 = llrender + llrendertarget) | 3 / 5 / 6 / 9 |
| 3.4 (texture lifecycle + 段階 2 引継ぎ) | 7 / 8 + 段階 2 引継ぎ (acceptance #1 + 特殊対応 2 件) |
| 3.5 (self-check + handoff) | (verify 中心、新規 bridging item 無し) |

### 3.3 measurement-first 採用 (redesign-first 回避)

Cluster C false alarm 教訓を踏まえ、sub-step 3.1 着手時に `LLVKLoader::createDevice` 内で以下 device limit を query + log 出力:

- `maxBoundDescriptorSets` (Vulkan 1.3 minimum = 4)
- `maxPushConstantsSize` (Vulkan 1.3 minimum = 128 bytes)
- `maxPushDescriptors` (`VK_KHR_push_descriptor` extension property、minimum = 32)
- `maxPerStageDescriptorSampledImages` (Vulkan 1.3 minimum = 16)
- `maxColorAttachments` (Vulkan 1.3 minimum = 4)
- `maxDescriptorSetSamplers` (Vulkan 1.3 minimum = 80)

NVIDIA RTX 5090 / Mesa RADV / Mesa ANV (Intel) の 3 driver baseline で実測値 log を取得 → 設計 budget 確定 → AYA 共有。

---

## 4. 3.1a-iii PD 算出 (軽い refresh、AYA 指示で de-prioritize)

AYA guidance「工数はやってみれば変動するものですからこの数字の精度をどうこう言ってもしかたない」を遵守、本 section は精度よりも sub-step 間相対比較に留める。

### 4.1 charter §6 / 06 §3.1 領域 3 PM 1.50 (1.5 人月) 据置き

- 04 §5.4.1 段階 3 工数感 = **4-5 週間 / 1.5 人月** で現 charter 整合
- 本 sub-step 3.1a trace 結果で 1.50 PM を refine する必要性 = **無し** (AYA 指示で精度議論 de-prioritize)
- 段階 3 完遂時に実工数 vs 1.50 PM の retrospective を `handoff-stage-3-complete.md` で 1 行記録 (charter §6.4 並走方針整合)

### 4.2 sub-step 間相対比較 (実装着手前の感覚、精度は二次)

| sub-step | 相対工数感 | 主要 driver |
|---|---|---|
| 3.1 | 中-大 | 11 件中 5-7 件配置、state alias root が refactor 起点 |
| 3.2 | 小 | smoke-test path、bridging template 動作確認用 |
| 3.3 | 中 | matrix stack + FBO の 2 大 surface 並列着手 (Agent 並列候補) |
| 3.4 | 大 | texture lifecycle + 段階 2 引継ぎ 3 件、12 pool hook body 配線 |
| 3.5 | 小 | self-check + validation strict 再 verify + handoff doc |

---

## 5. 3.1a-iv 着手 (次 session 最優先 deliverable)

### 5.1 sub-doc 03-state-machine-pso.md 訂正計画

本 sub-doc は `closed 2026-05-29 (Pattern α 一括 draft、AYA review PASS)` の状態。3.1a-iv で以下 section を **加筆 / 訂正** → status を `closed 2026-05-29 (3.1a-iv 加筆 PASS)` に re-close。

| 対象 section | 訂正方針 |
|---|---|
| **新規 §1.5 (sub-step 3.1a-i/ii 設計素材)** | 本 handoff §2 + §3 を sub-doc 03 内に取り込み、11 件 bridging items 設計表 + Cluster A-F trace 結果 sealed を §1.5 として追加 |
| **§3.1 sub-step 3.1 完了 marker** | 本 handoff §3.2 マッピング (1/2/4/10/11) を反映、measurement-first device limit query + log を完了 marker に追加 |
| **§3.1 sub-step 3.4 完了 marker** | descriptor set=1 per-material 7 PBR slot 配置 + VkImage/VMA lifecycle + 段階 2 引継ぎ 3 件の satisfy を marker に統合 |
| **§4.1 #1-段階 3 acceptance** | PFNGL declarations 物理削除は段階 5 一体運用 (Cluster D 波及 189 file) を明示、段階 3 内は llrender 5 file 内 GL call 0 件に scope refine |
| **§5.1 関連 doc 表** | 本 handoff (`handoff-substep-3-1a-iv-ready.md`) と 次 handoff (`handoff-substep-3-1a-complete.md`) を追加 |

### 5.2 handoff-substep-3-1a-complete.md 起草計画

doc path: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-1a-complete.md` (新規)

内容 (precedent: `handoff-substep-2-1a-complete.md` / `handoff-substep-2-4-complete.md`):

- 3.1a 4 段階全完遂宣言 (i/ii/iii/iv、本 handoff §1.2 引継ぎ)
- 3.1b (PSO 基盤 + state alias root **実装**) 着手前 scope 確認
- 11 件 bridging items の 3.1 / 3.3 / 3.4 配置最終 fix
- measurement-first device limit query 結果保全 (3.1b 着手時 AYA 環境実測)
- 段階 2 引継ぎ 3 件 (acceptance #1 + terrain + avatar) の 3.4 配置最終 fix
- charter §3 #1/#3/#5/#6 段階 3 分 acceptance との整合再確認

### 5.3 次 session 開始 cadence

1. **本 handoff (`handoff-substep-3-1a-iv-ready.md`) Read**
2. **status 確認質問 1 件**: AYA 並走作業 (ayastorm-release branch 等の context switch) があれば優先、無ければ 3.1a-iv 着手 GO
3. **3.1a-iv 着手**:
   - sub-doc 03 §1.5 追加 + §3.1 / §4.1 / §5.1 訂正 (本 §5.1 計画)
   - handoff-substep-3-1a-complete.md 起草 (本 §5.2 計画)
   - commit (AYA 明示指示後)
4. **3.1b 着手判断** (handoff-substep-3-1a-complete.md commit 後、AYA GO 指示で着手)

---

## 6. 検証 evidence (本 session で取得した素材)

### 6.1 Cluster A trace 数値

- llgl.cpp = 3,027 LOC、23 unique GL functions / 79 total direct calls
- PFNGL declarations = L227-L992 (~765 LOC)
- LLGLManager init = L1075-L1810 (~735 LOC)
- 12 state class implementations 範囲 = L2552-L2991 (~440 LOC)

### 6.2 Cluster C false alarm 訂正

- Agent speculation: 「106+ sampler breach 可能性」
- 実態: per-draw binding = 6-8 samplers、descriptor set=2 push descriptor 32 binding minimum を十分下回る
- 訂正根拠: PBR texture slot 数 = 7 + 場合により attachment / shadow sampler 数件、合計 ~10 内
- 教訓: `feedback_doubt_self_first.md` 違反 (Agent speculation を自分で疑わずに AYA に伝達した)、本 handoff §1.1 step 6 で正直記録

### 6.3 AYA guidance 履歴

- 「素直には繋げられないものが多数という認識でよいですか？」 → 楽観評価撤回、11 件列挙
- 「つなげるための調査と設計、及び各作業の項目と工数をここで算出して訂正及び加筆が必要ではないでしょうか？」 → 3.1a を 4 段階 (i/ii/iii/iv) に再 scope
- 「これは作業量が想像よりずっと多いということを言っていますか?」 → Cluster C false alarm 訂正
- 「工数はやってみれば変動するものですからこの数字の精度をどうこう言ってもしかたないと思っています」 → 3.1a-iii de-prioritize
- 「本 Step を正しい結果で積み上げる方にこそ重点において以降の Step を盤石にしましょう」 → 3.1a-iv 内容正確性最優先

---

## 7. deferred item 持ち越し

### 7.1 Mesa RADV 動作確認 (`handoff-stage-2-complete.md` §4.1 継承)

- AYA 環境に AMD discrete GPU 不在 → Mesa RADV 動作未確認、段階 10 driver matrix polish で testbed 確保

### 7.2 macOS / Windows 検証 (`handoff-stage-2-complete.md` §4.3 継承)

- 段階 1〜2 全て Linux 検証のみ、`project_ayastorm_three_platforms.md` 「Linux 先行例外」継承
- 段階 9 統合 verify でまとめて検証

### 7.3 PFNGL declarations 物理削除 (Cluster D)

- 189 file 波及で段階 3 内 scope 過大、**段階 5 (残依存解決) と一体運用** に refine
- 3.1a-iv で sub-doc 03 §4.1 acceptance #1-段階 3 に反映

### 7.4 Tree pool marker login 後検証 (`handoff-stage-2-complete.md` §4.2 継承)

- 段階 3 sub-step 3.1 等での実 PSO bind + draw 投入時に login 後 sustained session で fire 確認可、segment 化不要

---

## 8. 次 session 注意事項 (continuation)

### 8.1 `handoff-stage-2-complete.md` §5.2 / §5.3 継承事項

- Linux first-class baseline 厳守
- parity 不要、AYAstorm 改変 13 file shader は r42-α/β/γ で port
- acceptance satisfy は実機検証、推論 ban (`feedback_build_only_verified.md`)
- 仮説 2 連続外れ rule (`feedback_admit_unknown.md`)
- sub-step 完遂時 self-trace + handoff (`feedback_self_verify_before_handoff.md` + `feedback_proactive_handoff.md`)
- commit は AYA 指示後、push は AYA 手動
- 検証用 force-enable は出荷物に残さない (`feedback_remove_verification_logs.md`)
- bridging code 肥大耐性 (charter §2 領域 3 + 02 §1.1 risk 解説)
- descriptor set 配線設計の領域 7 並走整合
- avatar skinning SSBO の段階 3 内 timing (sub-step 3.4)

### 8.2 本 handoff 固有の新規注意事項

- **PD 精度議論 de-prioritize**: AYA guidance「やってみて変動する」遵守、3.1a-iv で sub-doc 03 訂正時も PD 数値より設計内容正確性を優先
- **`feedback_doubt_self_first.md` 再徹底**: Cluster C false alarm 教訓、Agent speculation を AYA に伝達前に自分で再 trace、hedged 表現 (「可能性」「breach 寄り」) はエスカレートせず原文 hedge 維持
- **measurement-first 採用**: descriptor / sampler / push constant budget は redesign 前に Vulkan device query で実測、redesign は実測値 base で判断
- **3.1a-iv の sub-doc 03 訂正は加筆中心**: 既存 §1 - §6 構造を壊さず §1.5 を追加 + §3.1 / §4.1 / §5.1 を refine、Pattern α 一括 draft の枠は維持

---

## 9. 関連 doc / memory

### 9.1 関連 doc (r41 章)

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter 本体 (closed 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/01-foundation.md` — sub-doc foundation (closed 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/02-portage-execution.md` — sub-doc 段階 2 (役割完了 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` — sub-doc 段階 3 (closed 2026-05-29、3.1a-iv で再 open / 加筆 / re-close 予定)
- `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` — sub-doc 領域 6 (closed 2026-05-29、3.1a-iv で cross-update 必要なら同 session 訂正)
- `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` — sub-doc 領域 7 (closed 2026-05-29、同上)
- `docs/specs/ayastorm-r41-gl-removal/handoff-r41-charter-complete.md` — r41 charter 完成 → 段階 1 着手前 prep
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-1-complete.md` — 段階 1 完遂 → 段階 2 着手前 prep
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-2-1a-complete.md` — sub-step 2.1a 完遂 → 2.1b 着手境界 (役割完了 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-2-4-complete.md` — sub-step 2.1b/2.2/2.3/2.4 完遂 → 2.5 着手境界 (役割完了 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-2-complete.md` — 段階 2 完遂 → 段階 3 着手境界 (役割完了 2026-05-29)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-1a-iv-ready.md` — **本 handoff** (3.1a-i/ii 完了 → 3.1a-iv 着手境界)

### 9.2 関連 commit (本境界までに積まれた branch 上 commit)

- `f6a5503c09` — `docs(r41): 段階 3 sub-doc 03/06/07 起草完了 (Pattern α 一括 draft、AYA review PASS)`
- (本 handoff 本体は本 session 末で commit 予定)
- 本境界以後の 3.1a-iv deliverable は次 session で commit

### 9.3 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone active 状態
- `project_ayastorm_three_platforms.md` — 3 OS 大前提 + Linux 先行例外
- `feedback_self_verify_before_handoff.md` — 3.1a 4 段階自己検証
- `feedback_proactive_handoff.md` — context 圧迫境界で能動 handoff 提案 → 本 doc で実施
- `feedback_no_auto_commit.md` — commit は AYA 明示指示後
- `feedback_release_flow.md` — push は AYA 手動
- `feedback_build_only_verified.md` — completion criteria の satisfy は実機検証
- `feedback_admit_unknown.md` — 仮説 2 連続外れたら gdb / validation / canary 切替
- `feedback_use_agents_proactively.md` — 6 Agent cluster 並列 trace で活用済
- `feedback_doubt_self_first.md` — Cluster C false alarm 教訓、本 handoff §1.1 + §6.2 で正直記録
- `feedback_render_full_trace_first.md` — 3.1a-i/ii trace-before-implement 遵守
- `feedback_no_scope_shrink.md` — AYA「すべて」指示 literal scope 維持、本 sub-step 3.1a の 4 段階再 scope も AYA 指示の literal 反映
