# AYAstorm r41 milestone charter — OpenGL 除去 + Vulkan 空転 (vk-α)

**status**: **closed 2026-05-28 (r41 charter 完成、r41 着手準備 ready / foundation + group A + group B 全 AYA review PASS、Pattern β group 分割で起草)**
**起草**: 2026-05-28
**達成条件**: AYAstorm 本線から OpenGL を完全除去 + Vulkan で空転動作 (swapchain + 黒画面 + UI 描画) が Linux first-class baseline で動作した段階
**親 charter**: `docs/specs/ayastorm-r40-vulkan-migration/00-charter.md` (r40 章 工程プラン、closed 2026-05-28)
**前置 doc (r40 章成果物 4 件)**:

- `docs/specs/ayastorm-r40-vulkan-migration/04-portage-inventory.md` — §5.4 段階 port 戦略 5 段階 (本 charter §2 work breakdown の implementation backbone)
- `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` — §3 descriptor set / §4 render pass / §7 swapchain / §10 LLVKRenderer skeleton
- `docs/specs/ayastorm-r40-vulkan-migration/06-effort-estimation.md` — §3.1 / §3.2 r41 per-file + per-shader 工数算定 (本 charter §2 base PM 出処)
- `docs/specs/ayastorm-r40-vulkan-migration/07-r42-plus-milestone-mapping.md` — §5.1 r41 charter outline 8 section base (本 charter の構成 base)

---

## §1 milestone thesis

AYAstorm 本線から **OpenGL を完全除去** + **Vulkan で空転動作** (swapchain + render pass + 黒画面 + UI 描画) 達成。描画 stage = **vk-α (空転)**。

### thesis 3 軸

#### (1) GL 完全除去 — 本線同居 Phase 1 完遂 marker

r40 章 charter §4 (4) 2 phase 構成の **Phase 1 完了 marker**。本線 `ayastorm-release` から OpenGL link / runtime call を全 file 完全除去 (`ldd` 等で link 確認可能 + runtime stack trace で GL call ゼロ確認)。04 doc §5.4 段階 1-5 完遂が backbone。

- 段階 1: `llglheaders.h` + `llglstates.h` + `llgltypes.h` 3 file 置換 + volk loader 導入 → 188 file の上流 file が変更不要 (a-4 で判明した abstraction 設計の最大の追い風)
- 段階 2-5: lldrawpool 13 file / llrender 主要 5 file / pipeline.cpp 3 大グローバル / 残依存 file の Vulkan 化

#### (2) Vulkan 空転 — vk-α 達成

viewer 起動 → swapchain 経由で **黒画面 + UI 描画** 動作。05 doc §3-§10 設計の実装:

- **descriptor set 3 階層** (set=0 per-frame / set=1 per-material / set=2 per-draw、per-draw は `VK_KHR_push_descriptor` で pool 不要)
- **render pass 7 chain** (shadow / g-buffer+picker / deferred lighting / forward alpha / sky / post-process / UI)、`VK_KHR_dynamic_rendering` 採用 (VkRenderPass/VkFramebuffer 廃止)
- **LLVKRenderer skeleton hook 配置** (interface 経由 call 化は r41.5、本 r41 段階では pipeline.cpp 内 inline 実装)
- **248 shader SPIR-V 化** base port ~228 file (cross compile A 195 素通り + B 53 要修正、AYAstorm 改変 13 file は r42-α/β/γ で port)

**parity 不要** — AYAstorm r1-r30 機能のうち描画依存機能は vk-α 段階で動作不可、それは r42-α 以降で順次回復。audio (r1-r13) + 描画非依存機能は本 r41 完遂後も動作 (本線 GL 除去後の Vulkan baseline 上で audio / UI / network / scene の非描画 path は維持)。

#### (3) Linux first-class baseline 確立

Mesa RADV / Mesa ANV / NVIDIA proprietary **3 driver** で安定動作。Win/Mac は本 r41 段階では本線 GL 維持 (Vulkan 着手は r42-α / r42-β、r40 章 charter §4 (2) 「Linux 先行」明示指示要件遵守)。

memory `project_ayastorm_three_platforms.md` 「3 OS 揃える、Linux のみ判断は明示指示が無い限り取らない」の **例外として r41 scope で適用** (r40 章 charter §4 (2) の明示指示要件を満たす)。Win/Mac user は本 r41 段階では本線 GL 経由で従来通り動作 (r40 章 charter §4 (6) 凍結保守 critical bugfix 範囲で維持)。

### 完遂後 next milestone への引継ぎ

- **next = r41.5** (VK repo 分離 + Vulkan code abstraction + 法的 review、07 doc §5.2 outline)
- r41 で **skeleton 配置のみ** → r41.5 で **interface 経由 call に詳細化** + dynamic link 構成 (本線 LGPL ↔ VK repo 独自 license)
- **LL UI 変更時の defensibility 確保** (LL 公式 VK engine 採用 + AYAstorm GUI 維持の選択肢、r40 章 charter §7 判断軸 3 (iv)) は r41.5 達成後から有効化

### scope 境界 (parity 不要の明示)

- **動作可**: audio (r1-r13) / chat (r22 tab 等の非描画部分) / network / UI / scene state / picker の CPU 側 (描画は不可)
- **動作不可 (r42 以降で順次回復)**: 描画依存 AYAstorm 機能 — picker GPU side (r42-α) / Cinematic Controls 描画系 (r42-β) / visual realism r14+ 系 (r42-γ) / chat の描画依存部分 (r42-δ 範囲) / **llpostprocess legacy effects (bloom / NightVision / ColorFilter) — upstream Firestorm 由来 dead code stub、本実装は r42-δ basket 移管 2026-05-29 (sub-step 3.2 smoke-test target が sky pool 1 draw に refine された経緯、`03-state-machine-pso.md` §3.1 sub-step 3.2 marker 参照)**
- **本線 GL 維持 (r41 段階)**: Win/Mac (r42-α/β で Vulkan 着手、r41 では本線 GL 維持 — r40 章 charter §4 (2)(6) 遵守)

---

## §2 work breakdown (11 領域 + base 計 + 余裕係数 + r41 total)

07 doc §5.1 §2 table を継承 + 各領域の **境界条件 / 依存順序 / risk 性質** を 1 段詳細化。

### 領域別 work table (06 doc §3.1 / §3.2 + 04 doc §5.4 集計)

| # | 領域 | 工数 (PM) | 主作業 | 出典 |
|---|---|---|---|---|
| 1 | 段階 1: GL header wrapper 置換 + volk loader | 0.50 | 212 GL header → volk-based 置換 + Vulkan instance / device 初期化 | 04 §5.4 / 06 §3.1 |
| 2 | 段階 2: lldrawpool Vulkan 化 (13 file) | 1.00 | lldrawpool 系 13 file の GL call → Vulkan command buffer record 化 | 04 §5.4 |
| 3 | 段階 3: state machine → PSO 化 (llrender 主要 5 file) | 1.50 | llrender state machine の Vulkan PSO 化 + render pass 統合 | 04 §5.4 / 06 §3.2 |
| 4 | 段階 4: pipeline.cpp 3 大グローバル → frame context | 1.00 | `sCull` / `sShadowRender` / `sCurCameraID` 等 → LLPipelineFrameContext 集約 | 04 §5.4 / 06 §3.2 |
| 5 | 段階 5: llspatialpartition / llviewershadermgr / llvertexbuffer 依存解決 | 0.50 | 残依存 file (含 llvosky / llvowlsky) の Vulkan 等価実装 | 04 §5.4 |
| 6 | 248 GLSL shader SPIR-V 化 (base port 分 ~228 file) | 4.96 | base shader の SPIR-V cross compile + descriptor set 整合 (AYAstorm 改変 13 file は r42-α/β/γ) | 06 §3.1 / §3.2 |
| 7 | descriptor set + render pass 設計反映 | 1.50 | 05 §3 + §4 base 実装 (per-frame / per-material / per-draw 3 階層 + 7 pass chain) | 06 §3.1 |
| 8 | Vulkan code abstraction skeleton (interface placeholder) | 0.50 | 05 §10.1-§10.2 LLVKRenderer interface 骨子 (pipeline.cpp 内 inline、interface 経由 call 化は r41.5) | 06 §3.1 / 05 §10.1-§10.2 |
| 9 | swapchain + present + UI 黒画面動作確認 | 0.40 | viewer 起動 → 黒画面 + UI 描画 (vk-α 空転 acceptance) | 06 §3.1 |
| 10 | Linux 限定 baseline polish | 0.36 | Mesa RADV / Mesa ANV / NVIDIA proprietary first-class driver matrix 初期動作確認 | 06 §3.2 |
| | **base work 計** | **11.78** | — | 04 §5.4 + 06 §3.1-§3.2 集計 |
| | 余裕係数 +37% (低 risk path 多くも base port 248 shader cross compile + 3 大グローバル refactor の不確実性反映) | **+4.39** | — | 06 §3.2 |
| | **r41 total (フルタイム dev 換算)** | **~16.17 PM** | — | 06 §3.9 |

### 領域別 境界条件 + 依存順序 + risk 性質

各領域を 04 doc §5.4 段階 1-5 backbone に沿って順番に解説。**境界条件** = 次領域着手前に達成しているべき state、**依存順序** = 並走可否、**risk 性質** = low/mid/high + 主因。

#### 領域 1: 段階 1 — GL header wrapper 置換 + volk loader (0.50 PM)

- **境界条件**: 領域 2 着手前に Vulkan instance / device 列挙 + queue family 選択動作 (FBO/swapchain 不要)
- **依存順序**: 全領域の前提 (領域 2-10 は領域 1 完了後着手)
- **risk**: **低** — header 置換は機械的、volk 導入も標準 pattern、a-4 で判明した「99% wrapper 経由」=  3 file 置換で 188 file 上流対応の追い風

#### 領域 2: 段階 2 — lldrawpool Vulkan 化 13 file (1.00 PM)

- **境界条件**: 領域 3 着手前に lldrawpool 単体で Vulkan command buffer record 動作 (PSO 統合は領域 3)
- **依存順序**: 領域 1 完了後着手、領域 6 (shader) + 領域 7 (descriptor) と並走可
- **risk**: **中** — drawpool 内 per-draw state は領域 3 PSO 化まで state machine 残存、段階 2 段階での bridging code が一時的に肥大

#### 領域 3: 段階 3 — state machine → PSO 化 llrender 主要 5 file (1.50 PM)

- **境界条件**: 領域 4 着手前に PSO 構築 / bind 動作 (frame context 集約は領域 4)
- **依存順序**: 領域 2 (drawpool) + 領域 7 (descriptor/render pass) + 領域 6 (shader SPIR-V) と協調必須
- **risk**: **高** — state machine の PSO 化は最大の refactor、a-3 §5.4 段階 3 = 不確実性 main source、06 §3.2 余裕係数 +37% の主要因

#### 領域 4: 段階 4 — pipeline.cpp 3 大グローバル → frame context + LLGLState RAII setter dead-store 化 (1.00 PM)

- **境界条件**: 領域 5 着手前に LLPipelineFrameContext が render path 全体で機能 + llgl.{cpp,h} RAII state class (LLGLState / LLGLDepthTest / LLGLSDefault 等 12 件) の setter 内 GL call 物理削除 (dead-store 化)
- **依存順序**: 領域 3 PSO 化と並走可 (refactor 対象が独立、PSO は drawpool/llrender 側、frame context は pipeline.cpp 側)、領域 3 完了後に LLGLState setter dead-store 化追加実施 (PSO state alias 基盤 + frame context で render path context 確定後に safe に物理削除)
- **risk**: **高** — sub-phase 1 で「pipeline.cpp 3 大グローバル + cull/stateSort 内 GL 呼出が並列化阻止」確認済の領域、refactor 自体は本線 GL 除去と独立だが r41 完遂 = 並列化準備の前提
- **scope refine 2026-05-29**: sub-step 3.1b 着手時に **bridging item #1 (LLGLState setter dead-store 化)** を本領域 4 に移管 (03 §1.5.3 / §3.1 / §4.1 / §3.3 + handoff §3.1 / §7.1 反映済)。受け入れ理由: PSO state alias 基盤 (領域 3 sub-step 3.1 で配線) + LLPipelineFrameContext (本領域) が揃った後に caller source-level compat 維持しつつ setter 内 GL call 物理削除可能、3.1b 時点で先行実施すると 段階 1+2 動作維持 (sub-doc 03 §3.5) と構造矛盾

#### 領域 5: 段階 5 — llspatialpartition / llviewershadermgr / llvertexbuffer / llvosky / llvowlsky 依存解決 (0.50 PM)

- **境界条件**: GL link / runtime call 完全除去 acceptance (本 r41 達成条件 #1) に直結
- **依存順序**: 領域 1-4 完了後着手、領域 6 (shader) と llviewershadermgr で連携
- **risk**: **中** — file 数限定だが llviewershadermgr は 248 shader 連携、llvosky / llvowlsky は描画依存 path

#### 領域 6: 248 GLSL shader SPIR-V 化 base port ~228 file (4.96 PM)

- **境界条件**: 領域 3 PSO 化 + 領域 7 descriptor set 整合と協調 (shader binding が descriptor set 3 階層に従う)
- **依存順序**: 領域 1 完了後着手、領域 2-3 と並走、AYAstorm 改変 13 file は **本 r41 scope 外** (r42-α picker 2 file / r42-β Cinematic 4 file / r42-γ visual realism 7 file、γ'-1 trace 確定値は 11 file = §3 #4 spec drift 反映)
- **risk**: **中** — sub-step 6.1 着手 model = **LLShaderMgr Vulkan path 配線 + glslang library runtime API + SPIR-V cache layer (case ② 採用 2026-05-31、handoff-substep-4-3-gamma-prime-prep.md §4 経由、§7.5 boundary refine 範囲)**、cross compile 素通り率は 06 §3.1 で base 算定済 (A 195 / B 53)、glslang library 3 OS 配信 (Linux apt / Mac brew / Windows Vulkan SDK) が新規 install path

#### 領域 7: descriptor set + render pass 設計反映 (1.50 PM)

- **境界条件**: 領域 3 PSO 化 + 領域 6 shader binding と協調
- **依存順序**: 領域 1 完了後着手、領域 2/3/6 と並走必須
- **risk**: **中** — 05 doc §3 + §4 で設計済、実装は a-3 § 反映通りだが 7 pass chain の transition + dependency 整合に sync 設計 (05 §5) 反映必要

#### 領域 8: Vulkan code abstraction skeleton (interface placeholder) (0.50 PM)

- **境界条件**: r41.5 着手前に LLVKRenderer skeleton hook 配置完了 (interface signature 詳細化は r41.5)
- **依存順序**: 領域 4 (pipeline.cpp frame context) と協調、interface hook は pipeline.cpp 内 inline 実装に併走
- **risk**: **低** — skeleton 配置のみ、interface 詳細は r41.5

#### 領域 9: swapchain + present + UI 黒画面動作確認 (0.40 PM)

- **境界条件**: **vk-α 達成 marker** (本 r41 達成条件 #2 直結)
- **依存順序**: 領域 1 完了後着手、領域 2-8 完了後に最終統合動作確認
- **risk**: **低** — 05 doc §7 標準的 swapchain 構成 (B8G8R8A8_SRGB / sRGB_NONLINEAR / image=3 / FIFO default)

#### 領域 10: Linux 限定 baseline polish (0.36 PM)

- **境界条件**: r41 達成条件 #3 Mesa RADV / Mesa ANV / NVIDIA proprietary 3 driver 動作達成
- **依存順序**: 領域 1-9 完了後着手 (最終 polish phase)
- **risk**: **中** — driver-specific quirks (Mesa RADV / ANV / NVIDIA) は実装着手前に予測困難、起草時 audit (05 doc §8 OS 別 + driver matrix) で軽減見込み

### work breakdown 総括

- **base work 11.78 PM** + **余裕係数 +37% (+4.39 PM)** = **r41 total 16.17 PM** (フルタイム dev 換算、06 doc §3.9)
- **暦月換算**: 中央値 ~84.1 暦月 (本職並走 4x + 学習曲線 +20-30%、06 doc §5.3)、暦年マーカー ~2033 年中 (06 doc §5.6) — 詳細は本 charter §4 (group A draft 予定)
- **高 risk 領域** = 領域 3 (state machine PSO) + 領域 4 (pipeline.cpp 3 大グローバル) の 2 件、余裕係数 +37% の主要因
- **低 risk 領域** = 領域 1 (wrapper 置換) + 領域 8 (skeleton) + 領域 9 (swapchain) の 3 件、合計 1.40 PM = 全体 12% は確度高い
- **並走可能性**: 領域 1 完了後、領域 2/3/4/6/7/8 は依存関係を満たしつつ並走可、領域 5/9/10 は最終段階 polish

---

## §3 acceptance criteria

07 doc §5.1.3 の 9 件 base を継承 + 各 criterion に **具体 metric / test procedure / regression criteria** を 1 段詳細化。r41 達成判定 = 全 9 件 PASS。

### #1 GL 除去完遂

- **metric**: 本線 binary が OpenGL に link されていない (Linux baseline)
- **test procedure**: `ldd build-linux-x86_64/newview/packaged/ayastorm-bin | grep -E "libGL|libEGL|libGLX"` が **空 (0 line)**、加えて runtime detect (`dlsym` hook で `gl[A-Z]` entry point 監視) で 1 セッション動作中の GL function 呼出 **0 件**
- **regression**: GL header include の grep audit (`grep -rE "^#include.*<GL/" indra/` が **0 件** + `grep -rE "^#include.*llgl" indra/` が **wrapper 内部のみ**)、04 doc §3 GL header 依存 189 file のうち **wrapper 3 file 以外で GL include 0 件**

### #2 Vulkan 空転動作

- **metric**: viewer 起動 → swapchain image acquire + queue submit + present 動作、**黒画面 + UI 描画される** (vk-α 達成)
- **test procedure**: Linux Mesa RADV + Linux NVIDIA proprietary の 2 driver で 30 分間 idle 維持後 swapchain image 取得失敗 **0 件** + validation layer (`VK_LAYER_KHRONOS_validation`) error / warning **0 件**
- **regression**: vk-α 達成後 30 分 sustained 動作で memory leak **0 byte/min** (`VK_EXT_memory_budget` VRAM 監視 + VMA allocator stats)

### #3 段階 1-5 全完遂

- **metric**: 04 doc §5.4 段階 1-5 の対象 file (llrender 51 file / pipeline.cpp + .h / lldrawpool 13 file / llspatialpartition / llviewershadermgr / llvosky + llvowlsky / wrapper 3 file) の GL call が全 file で Vulkan 化済
- **test procedure**: 対象 file 群に対する `grep -rE "gl[A-Z][a-zA-Z]+\s*\(" <file>` audit が **wrapper 3 file 以外で 0 件**、加えて 段階別動作確認 (段階 1 = instance/device 列挙動作 / 段階 2 = drawpool command buffer record 動作 / 段階 3 = PSO bind 動作 / 段階 4 = LLPipelineFrameContext 動作 / 段階 5 = 残依存解決)
- **regression**: 段階 1-5 完遂時点で viewer build pass + 起動 pass + 1 セッション (~30 分) 安定動作 (crash / abort 0 件)

### #4 248 shader SPIR-V 化 base port (~228 file)

- **metric**: base 228 file (cross compile A 195 素通り + B 53 要修正) の **LLShaderMgr Vulkan path 経由** glslang runtime API での SPIR-V binary 生成 + `vkCreateShaderModule` load 成功 + SPIR-V cache layer hit/miss 動作 (case ② 採用 2026-05-31、§7.5 boundary refine 履歴反映)
- **test procedure**: viewer 起動時 LLShaderMgr Vulkan path (`gVK.isEnabled()` 条件下 conditional branch) 経由で 228 file 全 file の SPIR-V binary 生成成功 (初回 cache miss compile + 2 回目以降 `~/.ayastorm_x64/cache/shader_cache/<mShaderHash>_{vert,frag}.spv` cache hit load) + runtime descriptor set binding mismatch validation error **0 件**、AYAstorm 改変 file (γ'-1 trace 確定値 **11 file** = picker 2 / Cinematic 2 / visual realism 7、本 charter 旧記載 13 → 実 11 の spec drift) は **untouched** (r42-α/β/γ で port)
- **regression**: shader compile error / link error **0 件**、AYAstorm 改変 11 file (spec drift 反映) の port が本 r41 scope に含まれていないこと (git diff で 11 file が untouched 確認)

### #5 descriptor set + render pass 設計実装

- **metric**: set=0 per-frame / set=1 per-material / set=2 per-draw の 3 階層動作 + per-draw `VK_KHR_push_descriptor` で pool 不要動作 + 7 pass chain (shadow / g-buffer+picker / deferred lighting / forward alpha / sky / post-process / UI) 動作 + `VK_KHR_dynamic_rendering` 採用 (`VkRenderPass` / `VkFramebuffer` 廃止)
- **test procedure**: validation layer (`VK_LAYER_KHRONOS_validation`) で起動 + 30 分動作中の descriptor binding mismatch / render pass dependency violation **0 件**
- **regression**: 05 doc §3 + §4 設計通りの set 構成 + render pass chain (実装で hard-coded な hack 0 件、05 doc § と 1:1 対応)

### #6 LLVKRenderer interface skeleton

- **metric**: 05 doc §10.1-§10.2 hook 配置済、ただし **pipeline.cpp 内 inline 実装** で動作 (interface 経由 call 化は r41.5)
- **test procedure**: `grep -rE "class LLVKRenderer" indra/` で skeleton declaration 存在 + 05 doc §10.1-§10.2 hook site (`LLVKRenderer::*` 想定 member) の placeholder 配置済
- **regression**: skeleton hook と pipeline.cpp inline 実装の **signature 整合** (r41.5 で interface 経由 call 化する際に signature 不整合での refactor cost が発生しない)

### #7 Linux baseline first-class

- **metric**: Mesa RADV (AMD) + Mesa ANV (Intel) + NVIDIA proprietary の **3 driver** で空転動作
- **test procedure**: 各 driver で起動 + 30 分 idle + 終了が成功、validation error **0 件**、driver matrix table (05 doc §8 + 本 charter で起草時 audit) 更新
- **regression**: driver 別 quirks (Mesa RADV 特有 / Mesa ANV 特有 / NVIDIA 特有) は doc 化 + workaround 反映済 (本 r41 charter §7 詳細化方針で driver-specific quirks list audit 結果を組込)

### #8 regression sweep (描画非依存機能)

- **metric**: AYAstorm r1-r30 機能のうち **audio (r1-r13) + 描画非依存機能が本線 GL 除去後も動作**
- **test procedure**: 動作確認対象 = parcel music (r12.1) + AYAstream M5-M8 + 5.1ch (r9/r10) + binaural (r11) + chat tab r22 非描画部分 + login / asset / friends list / inventory / IM の non-render path
- **regression**: 描画依存機能 (r14+ visual realism / r21 picker GPU side / r30 Cinematic 描画系) の **動作不可は許容** (parity 不要、r42 以降で順次回復)、ただし AYAstorm 機能自体の **CPU 側 state** (cvar / preset / UI) が保存・読込される必要あり (描画再開時に restore 可能な state を維持)

### #9 Win/Mac 未着手宣言

- **metric**: Win/Mac は本 r41 段階で **本線 GL 維持** (Vulkan 着手は r42-α / r42-β、r40 章 charter §4 (2) Linux 先行 明示指示要件遵守)
- **test procedure**: Win build (`autobuild build -A 64 -c ReleaseOS`) + Mac build (t-noami workflow) が本線 GL で従来通り build pass + 起動 pass、r41 段階での Vulkan 化対象は **Linux only**
- **regression**: Win/Mac user は r40 close 時点の本線 GL 動作を継続 (r40 章 charter §4 (6) 凍結保守 critical bugfix 範囲で維持)、本 r41 commit による Win/Mac binary の動作回帰 **0 件**

### acceptance 運用方針

- **判定 cadence**: 段階 port 戦略 5 段階 (本 charter §2 領域 1-5) 各段階完遂時に **段階別 self-check** + r41 全体達成判定は 9 件全 PASS で実施
- **不達時の対処**: 9 件のうち 1 件でも未達 = r41 未達 (達成宣言保留)、未達 criterion 別に対処 (例: #2 swapchain で validation error 残存 → 該当 driver 別 workaround 追加 → 再 sweep)
- **acceptance refine cadence**: r41 着手後の実装中に metric / test procedure を **r41 進行中に refine 可** (charter §7 詳細化方針継承)、ただし criterion 自体 (本 §3 #1-#9 の趣旨) は AYA 確認なしに変更しない

---

## §4 暦月変換 + 暦年マーカー

06 doc §5 + §6 反映、本 charter §2 work breakdown (r41 total 16.17 PM) から **暦月** + **暦年マーカー** + **uncertainty band** + **plan B trigger** を確定。

### §4.1 中央値暦月換算 table (06 doc §5.1-§5.3 反映)

| 換算項目 | 値 | 出処 |
|---|---|---|
| r41 base PM (フルタイム dev) | 16.17 | 本 charter §2 + 06 §3.9 |
| 並走 ratio 中央値 | **4x** | 06 §5.1 (charter §4 (3) 想定 3-5x の中央値、補正要因 3 件合成: 生 work hours 1.60x + Context switch 1.40x + 体調 skip 1.33x ≈ 3.0x、Vulkan 設計 cycle 上方振れで 5x → 中央値 4x) |
| 学習曲線補正 | **+30%** | 06 §5.2 (全 milestone 中最も高い段階、Vulkan 初期学習 cost 反映) |
| r41 weighted PM (学習曲線適用後) | **~21.02** | 16.17 × 1.30 = 21.02 PM |
| r41 中央値暦月 (本職並走 4x 適用後) | **~84.1** | 21.02 × 4 = 84.1 暦月 (06 §5.3) |
| 暦年マーカー (2026-05-28 起算) | **~2033 年中** | 06 §5.6 (累積暦月、r40 章 close 2026-05-28 起算で +7 年) |

### §4.2 uncertainty band (06 doc §6.3 r41 milestone 別 band 反映)

| band | % | 暦月 | 暦年 |
|---|---|---|---|
| **下方 band 下限** (発生確率 5-10%) | -30% | ~58.9 暦月 | ~2031 年中 (~4.9 年) |
| **中央値** (発生確率 50%) | 0% | ~84.1 暦月 | ~2033 年中 (~7.0 年) |
| **上方 band 上限** (発生確率 10-15%) | +120% | ~185 暦月 | ~2041 年末 (~15.4 年) |

不確実性 8 要因 (06 §6.1) のうち r41 特有の高寄与要因 = **Vulkan 学習曲線** + **per-file 偏差** (段階 3 PSO 化 + 段階 4 pipeline.cpp refactor の不確実性反映、本 charter §2 高 risk 2 領域)。

### §4.3 plan B trigger 閾値 (06 doc §8.3 + memory 工程プラン確定事項 (3) 無期限遵守)

| 閾値 | 暦月 | 説明 | trigger 種別 |
|---|---|---|---|
| 早期 warning | ~36 暦月 (3 年) | charter §8 (B) 例 1 継承、中央値 ~43% 進捗段階で達成不能予測 | warning (再算定 cadence 起動) |
| 中央値乖離 warning | ~109 暦月 (~9 年) | 中央値 84.1 × 1.30 = 109 暦月、不確実性 8 要因の複数同時上振れ予兆 | warning |
| **trigger 発動** | **~185 暦月 (~15.4 年)** | r41 上方 band 上限突破、§4.2 上限と一致 | trigger 発火 (対処 5 案検討) |

trigger 発火時の対処順位 (06 §8.5、r41 段階で適用):

1. **(δ) LL 着地 reset** — LL 公式 Vulkan engine 着地で AYAstorm-vk reset/maintain/merge 選別 (最優先、外部条件依存で control 不可だが発火時の reset 価値最大)
2. **(α) scope 縮小** — r41 scope を分割 (段階 1-3 のみ r41 完遂、段階 4-5 を r41.5 に move 等)
3. **(γ) 別 viewer base 接続** — 別 viewer の Vulkan 実装 (例: 他 fork) への base 切替 (defensibility 確保、r41.5 abstraction 完成前提)
4. **(β) quality 緩和** — Linux first-class baseline の driver 縮小 (例: Mesa RADV のみ r41 完遂、ANV/NVIDIA を r41.5 へ)
5. **(ε) 撤退** — r41 撤退 (r40 close 凍結保守継続、最終手段)

### §4.4 plan B trigger 判定 cadence

- **annual review**: 毎年 5 月末 (r40 章 charter 起草日 2026-05-28 起算で 1 年単位)、進捗 vs §4.1 中央値乖離 % で判定
- **milestone 完遂時**: r41 達成宣言時に最終 cadence 整合 (§4.1 中央値 vs 実暦月 + §4.2 band 内位置)
- **emergency**: 外部条件 (LL 着地 / 工程プラン破綻外部 trigger / AYA life plan 変更) で都度判定 (06 §8.6)

### §4.5 暦月換算の含意

- 中央値 ~84 暦月 (~7 年) = charter §4 (3) 無期限 / 6-15 人年想定の **下限近接** (本職並走 4x で人年換算は ~1.75 人年 = フルタイム dev 換算で 16.17 PM = ~1.35 人年)
- 上方 band 上限 ~185 暦月 (~15.4 年) = charter §4 (3) 想定の **下限-中央 帯**
- 下方 band 下限 ~58.9 暦月 (~4.9 年) = 早期完遂シナリオ (LL Vulkan 部分着地 + Vulkan 学習曲線 圧縮)、発生確率 5-10% で過度に楽観視しない

---

## §5 依存 milestone

本 r41 着手の前提条件 = **r40 達成 (2026-05-28 完了済) + 設計 doc 確定 + 環境前提**。interface 確立は本 r41 で skeleton 配置のみ (interface 経由 call 化は r41.5)。

### §5.1 前 milestone

- **r40 達成 (2026-05-28 完了済)**: r40 章 工程プラン完成 = 本 r41 charter 起草の前提条件 = **満たされている**
- r40 章 close 内訳: work item (a) Vulkan portage 棚卸し + (b) Vulkan API 設計 + (c) 工程算定 + (d) r42+ 区切り確定 + (e) charter 完成 全完了 + AYA review PASS
- 本 r41 charter は r40 章成果物 4 件 (04 / 05 / 06 / 07 doc) の **cross reference + 1 段詳細化** で起草、独立した新規設計は r41 段階では含まれない

### §5.2 interface 確立

- **本 r41 段階**: なし (LLVKRenderer interface 経由 call 化は r41.5)
- **本 r41 で配置**: 05 doc §10.1-§10.2 hook site の **skeleton placeholder** のみ (signature 整合は §3 #6 acceptance criterion で担保)
- **r41.5 への引継ぎ**: r41 完遂時点で pipeline.cpp 内 inline 実装が動作、r41.5 で同 signature を interface 経由 call に置換 (refactor cost 最小化のため signature を r41 段階で確定する acceptance criterion #6 = r41.5 着手 prerequisite)

### §5.3 設計 doc 確定 (r40 章成果物 4 件 cross reference)

| doc | 本 r41 charter での参照 section |
|---|---|
| `04-portage-inventory.md` | §5.4 段階 port 戦略 5 段階 (本 charter §2 領域 1-5 backbone) + a-3 §B.x AYAstorm 機能 pull-in 順 (本 r41 scope 外 = r42-α/β/γ で port 確認) |
| `05-vulkan-api-design.md` | §3 descriptor set 3 階層 (本 §3 #5) / §4 render pass 7 chain + KHR_dynamic_rendering (本 §3 #5) / §5 sync (本 §3 #2 swapchain dependency) / §6 memory + VMA (本 §3 #2 leak 監視) / §7 swapchain (本 §3 #2 + §3 #7 driver matrix) / §8 OS 別 (本 §3 #7 Linux baseline + §3 #9 Win/Mac 未着手) / §9 extension (本 §3 #5 KHR/EXT 採用) / §10 LLVKRenderer skeleton (本 §3 #6) |
| `06-effort-estimation.md` | §3.1 per-file 工程算定 (本 §2 領域 1-5 PM) / §3.2 per-shader 工程算定 (本 §2 領域 6 PM) / §3.9 r41 total (本 §2 r41 total 16.17 PM) / §5.1-§5.3 暦月換算 (本 §4.1) / §5.6 暦年マーカー (本 §4.1) / §6 uncertainty band (本 §4.2) / §8 plan B trigger (本 §4.3-§4.4) |
| `07-r42-plus-milestone-mapping.md` | §5.1 r41 charter outline 8 section (本 charter 全体構成 base) / §1.2 正式区分 (r41 → r41.5 → r42-α/β/γ/δ → r43-r44 → vk-RC 連続 milestone の中での本 r41 位置付け) |

### §5.4 memory 確定値 (本 r41 charter の thesis 確定 source)

| memory | 本 r41 charter での参照 |
|---|---|
| `project_ayastorm_r40_cpu_parallel.md` | 工程プラン確定事項 8 件 (1) parity 完遂 / (2) Linux 先行 / (3) 無期限 / (4) 2 phase 構成 / (5) LL 着地時判断 / (6) 凍結保守 / (7) r14+ suspend / (8) r42+ ロードマップ |
| `project_ayastorm_r41_vulkan_migration.md` | r41 scope 確定 = GL 依存除去 + Vulkan 空転 single milestone (本 §1 thesis 3 軸 base) |
| `project_ayastorm_three_platforms.md` | 3 OS 大前提 + Linux 先行 明示指示要件 (本 §1 thesis (3) Linux first-class + §3 #9 Win/Mac 未着手 base) |

### §5.5 環境前提

- **OS / driver**: Linux baseline (Ubuntu 24.04+ 想定) + Mesa RADV (AMD) + Mesa ANV (Intel) + NVIDIA proprietary の 3 driver (§3 #7 acceptance criterion 対象)
- **toolchain**: LunarG SDK 1.3.x + volk loader + glslang (GLSL → SPIR-V) + SPIRV-Cross (必要に応じ shader debug) + VMA (Vulkan Memory Allocator)
- **build**: 既存 AYAstorm autobuild flow (`autobuild configure -A 64 -c ReleaseOS` → `autobuild build -A 64 -c ReleaseOS` → 既存 install flow)、Vulkan link は r41 着手時点で追加
- **dependency 入手**: LunarG SDK は debian/ubuntu apt repo + volk は header-only + VMA は header-only + glslang は LunarG SDK 同梱 → 追加 3rdparty fetch script は r41 着手時に整備
- **branch 戦略**: r40 章 charter §4 (4) Phase 1 構成準拠 = 本線 `ayastorm-release` 内 long-lived feature branch 群 (本 r41 charter 起草 branch = `feature/ayastorm-r40-vulkan-migration`、r41 着手 branch は §6 起草 cadence で確定)

### §5.6 依存達成状況 cross check

| 依存項目 | 状況 | 補足 |
|---|---|---|
| r40 達成 (前 milestone) | **✓ 達成済 (2026-05-28)** | r40 章 close + work item (a)-(e) 全完了 + AYA review PASS |
| 設計 doc 確定 (04/05/06/07) | **✓ 確定済 (2026-05-28)** | 4 file 全 AYA review PASS、本 r41 charter での cross reference 整合 ✓ |
| memory 確定値 (3 件) | **✓ 確定済** | 工程プラン 8 件 + r41 scope + 3 OS 例外 |
| 環境前提 (OS/driver/toolchain) | **未着手 (r41 着手時整備)** | r41 着手時に LunarG SDK + volk + VMA + glslang 統合 + driver matrix 動作確認 |
| interface 確立 | **本 r41 段階では skeleton placeholder のみ** | interface 経由 call 化は r41.5 (本 §5.2) |

→ **本 r41 着手の前提条件は § 5.6 #1-#3 で満たされている** (#4 #5 は本 r41 着手 phase 内 work)、依存 milestone 観点で blocker なし。

## §6 起草 timing / 主体 / cadence

### §6.1 本 charter 起草情報

- **起草着手**: 2026-05-28 (r40 章 close 当日 = work item (e) 完了 + AYA 「review OK」 = r40 達成宣言を兼ねる Pattern P 確定の直後)
- **起草主体**: AYA + Claude
- **起草先**: `docs/specs/ayastorm-r41-gl-removal/00-charter.md` (本 doc、新規 directory 同時作成)
- **起草 base**:
  - 07 doc §5.1 r41 charter outline 8 section (主構成 base)
  - 04 doc §5.4 段階 port 戦略 5 段階 (§2 work breakdown backbone)
  - 05 doc §3 / §4 / §7 / §10 (§3 acceptance criteria 設計反映 base)
  - 06 doc §3.1-§3.2 / §5.3 / §5.6 / §6.3 / §8.3 (§2 PM 値 + §4 暦月 + uncertainty band + plan B trigger base)

### §6.2 本 charter 起草 cadence (Pattern β group 分割)

r40 章 work item (c) / (d) / (e) で実績ある **Pattern β (group 分割)** を継承:

| group | section 範囲 | 完了日 | AYA review |
|---|---|---|---|
| **foundation** | header + §1 thesis + §2 work breakdown | 2026-05-28 | **PASS** (本 charter §6.1 起草着手直後) |
| **group A** | §3 acceptance criteria + §4 暦月変換 + §5 依存 milestone | 2026-05-28 | **PASS** (foundation 直後) |
| **group B** | §6 起草 cadence + §7 詳細化方針 + §8 関連 doc / memory | 2026-05-28 | (本 group、AYA review 待ち) |

cadence 選択理由 (handoff doc `handoff-r40-close-r41-charter-start.md` §3.2 反映):

- **Pattern α (一括 draft)** 不採用: 8 section 全部を 1 session で draft → AYA review burden 大、修正が広範囲に波及
- **Pattern β (group 分割)** 採用: r40 章 work item (c)(d)(e) で 3 group 全 AYA review PASS 実績、group 境界で handoff 可能
- **Pattern γ (section 単位)** 不採用: 1 section ずつ AYA review = session 数過多、本 charter 8 section の中央値 1 section/session × 8 session は overhead

### §6.3 本 charter 完成後の次 action cadence

本 charter group B AYA review PASS 後の次 action 候補 (AYA + Claude で擦り合わせ):

| candidate | 内容 | 想定先行作業 |
|---|---|---|
| **(α) r41 着手** | 本 charter §2 領域 1 (段階 1: GL header wrapper 置換 + volk loader) 着手 | branch 戦略確定 (`feature/ayastorm-r41-gl-removal` 新規 or 現 branch 継承) + 環境前提整備 (LunarG SDK + volk + VMA + glslang 統合) |
| **(β) r41.5 charter 起草** | 07 doc §5.2 r41.5 charter outline base に r41.5 charter 先行起草 | 本 charter r41 完遂後で十分、本 candidate (β) は r41 着手前の選択肢として薄い |
| **(γ) 関連 memory update** | `project_ayastorm_r41_vulkan_migration.md` status を pending → active に更新 + 本 charter cross reference 追加 | 本 charter group B AYA review PASS 直後に実施可能 |
| **(δ) handoff doc 作成** | 本 session 終了境界で `handoff-r41-charter-complete.md` 作成、次 session で r41 着手の base 資料 | group B AYA review PASS 直後 |

cadence 選好は本 charter group B AYA review PASS 直後に AYA + Claude で擦り合わせ。**(γ) + (δ) は本 session 中 mandatory** (memory update + handoff)、**(α)** は次 session 着手想定 (本 session 残量 + AYA 判断次第)。

### §6.4 r41 着手の主体 / 並走方針

- **主体**: AYA + Claude (本職並走 4x ratio、charter §4 (3) 反映)
- **並走方針**:
  - **Claude**: 領域 1-10 の実装 + shader port + 段階 port 戦略 5 段階の機械的 refactor + driver matrix 動作確認 cycle
  - **AYA**: 設計判断 (descriptor set 階層構成詳細 / render pass dependency 詳細 / driver-specific quirks 採用方針) + ビルド + 実機動作確認 + acceptance criteria 判定
  - **協調 cadence**: 各領域完遂時に AYA review + self-trace + handoff (memory `feedback_self_verify_before_handoff.md` + `feedback_proactive_handoff.md` 遵守)
- **commit / push cadence**: `feedback_no_auto_commit.md` (commit は AYA 明示指示) + `feedback_release_flow.md` (push は AYA 手動) 遵守

### §6.5 起草中の rule reminder (本 charter + 後続 r41 着手で適用)

- **Linux first-class baseline 厳守**: 本 charter §1 thesis (3) + §3 #7 acceptance + §3 #9 Win/Mac 未着手、memory `project_ayastorm_three_platforms.md` の **明示指示要件を満たす例外**
- **parity 不要**: 本 charter §1 thesis scope 境界、AYAstorm 改変 13 file shader (picker 2 / Cinematic 4 / visual realism 7) は **r42-α/β/γ で port** (本 r41 段階で touch しない)
- **無期限 / AYA life plan**: charter §4 (3) 反映、§4.3 plan B trigger は外部条件 + 上方 band 上限突破でのみ発動
- **設計倒れ予防**: memory `feedback_build_only_verified.md` 「正しいことを積み上げる」遵守、本 charter で確定した acceptance criteria を r41 着手中に流用する際は **実機検証** で satisfy 確認 (推論ベース判定禁止)
- **checking 仮説 2 連続外れ rule**: memory `feedback_admit_unknown.md` 反映、r41 着手中の driver quirks / shader cross compile issue / state machine PSO 化 trouble で **仮説 2 連続外れたら log/canary/bisect で実データ取得に切替**

---

## §7 詳細化方針

本 charter は r40 章 work item (d) 07 doc §5.1 outline を base に **1 段詳細化** で起草。さらに r41 着手中に詳細化する phase + 詳細化 cadence を本 §7 で確定。

### §7.1 本 charter 起草段階で詳細化済 (group A/B 反映)

07 doc §5.1 outline 8 section base から本 charter 起草中に追加詳細化した内容:

| section | outline base | 本 charter 詳細化 |
|---|---|---|
| §1 thesis | thesis 概要 | 3 軸 ((1) GL 除去 / (2) Vulkan 空転 / (3) Linux baseline) + 完遂後 next milestone 引継ぎ + scope 境界 (parity 不要明示) |
| §2 work breakdown | 11 領域 table | 各領域 詳細化 (境界条件 / 依存順序 / risk 性質) + 高 risk 領域識別 (3 PSO + 4 pipeline.cpp) + 並走可能性 (領域 2/3/4/6/7/8) |
| §3 acceptance criteria | 9 件 draft | 各 criterion に **metric / test procedure / regression criteria** 詳細化 + 判定 cadence + 不達時の対処方針 |
| §4 暦月 + 暦年 | 中央値 84.1 + 暦年 2033 | uncertainty band 3 段 (下方/中央値/上方) + plan B trigger 3 閾値 + 対処 5 案順位 + 判定 cadence 3 種 |
| §5 依存 | 前 milestone + interface + 設計 doc | r40 達成達成済 cross check + 04/05/06/07 doc 11 個別参照 + memory 3 件 + 環境前提 + 依存達成状況 cross check table |
| §6 起草 cadence | 起草 timing / 主体 / 先 | Pattern β cadence + 次 action cadence 4 候補 + 並走方針 + rule reminder |

### §7.2 r41 着手中に詳細化する phase (本 charter 起草段階では outline、着手時 work item 化)

r41 着手中に **段階別** で詳細化する内容 (本 charter §2 領域別の port 戦略 + driver matrix + interface signature 等):

#### (a) 段階 port 戦略 file list 詳細 (本 §2 領域 1-5 関連)

- 04 doc §5.4 段階 1-5 の **各段階内 file list** + dependency graph (どの file を先に port するか)
- r41 着手 phase 内で work item 化 (例: `work-item-a-stage-1-file-list.md` 等の sub-doc)
- 出処: 04 doc §5.4 段階 port 戦略 5 段階 + a-3 §B.x AYAstorm 機能 pull-in 順 + a-4 §6.3 段階 port 戦略 final

#### (b) LLVKRenderer interface signature 詳細 (本 §2 領域 8 関連)

- 05 doc §10.1-§10.2 hook 配置を **signature 詳細化** (関数名 / 引数 / 戻り値 / lifecycle 制約)
- r41 着手中の `pipeline.cpp` 内 inline 実装で signature 確定、r41.5 で interface 経由 call 化の前提 (本 §3 #6 acceptance criterion 担保)
- 出処: 05 doc §10 LLVKRenderer skeleton + r41 着手後の実装中

#### (c) Linux driver-specific quirks audit (本 §2 領域 10 関連)

- Linux Mesa RADV / Mesa ANV / NVIDIA proprietary の **driver-specific quirks list** 起草 (r41 着手時 audit)
- driver 別 workaround の doc 化 (本 §3 #7 acceptance criterion regression 担保)
- 出処: 05 doc §8 OS 別 driver matrix + r41 着手後の実機検証

#### (d) shader cross compile audit (本 §2 領域 6 関連)

- base 228 file の **cross compile coverage audit** (A 195 素通り + B 53 要修正の確定 + 修正内容詳細)
- AYAstorm 改変 13 file の untouched 維持確認 (本 §3 #4 acceptance criterion regression 担保)
- 出処: 06 doc §3.1 + §3.2 + r41 着手後の glslang compile audit

#### (e) descriptor set + render pass 実装詳細 (本 §2 領域 7 関連)

- 05 doc §3 descriptor set + §4 render pass 設計の **実装 mapping 詳細** (どの shader が set=0/1/2 どの slot に bind されるか)
- 7 pass chain の **transition / dependency 詳細** (sync 設計 05 §5 反映)
- 出処: 05 doc §3 + §4 + §5 + r41 着手後の実装中

### §7.3 詳細化 cadence (本 charter 完成後の運用)

| 段階 | 内容 | 結果物 |
|---|---|---|
| **本 charter 完成** | foundation + group A + group B 全 AYA review PASS | `00-charter.md` 確定 |
| **r41 着手前 prep** | 環境前提整備 (LunarG SDK + volk + VMA + glslang 統合) + branch 戦略確定 | branch + 環境 ready |
| **r41 着手後 段階別** | (a) file list / (b) interface signature / (c) driver quirks / (d) shader audit / (e) descriptor mapping を段階別 sub-doc 化 | `01-foundation.md` / `02-portage-execution.md` / 等の sub-doc 群 |
| **r41 達成判定** | §3 #1-#9 acceptance criterion 全 PASS で達成宣言 | r41 完遂 + r41.5 charter 起草移行 |

### §7.4 sub-doc 構成 (r41 着手中に随時追加)

本 charter 完成後、r41 着手中に作成する sub-doc の **想定 outline** (起草中に随時 refine):

| sub-doc | 内容 | 関連 §2 領域 |
|---|---|---|
| `01-foundation.md` | 環境前提整備 + branch 戦略確定 + 段階 1 file list (GL header wrapper) | §2 領域 1 |
| `02-portage-execution.md` | 段階 2-5 file list 詳細 + dependency graph + port 順序 | §2 領域 2-5 |
| `03-shader-port.md` | base 228 file cross compile audit + glslang 統合 + descriptor set mapping | §2 領域 6 |
| `04-descriptor-render-pass.md` | descriptor set 3 階層実装詳細 + 7 pass chain transition + sync 設計反映 | §2 領域 7 |
| `05-skeleton-interface.md` | LLVKRenderer skeleton signature 詳細 + pipeline.cpp 内 inline 実装方針 | §2 領域 8 |
| `06-swapchain-vk-alpha.md` | swapchain + present + UI 黒画面動作確認 + vk-α acceptance | §2 領域 9 |
| `07-linux-driver-matrix.md` | Mesa RADV / Mesa ANV / NVIDIA proprietary 3 driver quirks audit + workaround | §2 領域 10 |

**注**: 本 §7.4 sub-doc 構成は **outline** であり、r41 着手中に AYA + Claude で擦り合わせて refine。本 charter 起草段階では sub-doc 着手しない。

### §7.5 詳細化の boundary (本 charter vs sub-doc + r41 着手)

- **本 charter で確定**: thesis / scope / acceptance criteria 9 件 + metric / 暦月換算 + uncertainty band + plan B trigger + 依存 milestone + 起草 cadence + 詳細化方針 outline
- **r41 着手中 sub-doc で詳細化**: 各段階 file list + interface signature + driver quirks audit + shader cross compile audit + descriptor mapping 実装詳細
- **r41 着手中に refine 可な本 charter content**: §3 metric / test procedure (本 §3 acceptance 運用方針)、§2 領域別 PM 配分 (実装中の実測値で refine 可)
- **AYA 確認なしに変更しない本 charter content**: §3 #1-#9 criterion 趣旨 (§3 acceptance 運用方針 反映) + §1 thesis 3 軸 + §4 plan B trigger 閾値 + §5 依存 milestone 構成
- **boundary refine 履歴** (AYA review PASS 反映、sub-doc 内処理可境界の明示記録):
  - **2026-05-31**: 段階 5 LLVertexBuffer Vk 化を sub-step 4.3 内へ前出し (`handoff-substep-4-3-beta-prep.md` §5、AYA 「OK」承認、sub-step 4.3-β' 完遂 commit `78820a6edf` で literal satisfy)
  - **2026-05-31**: 領域 6 sub-step 6.1 着手 model を **case ② = LLShaderMgr Vulkan path 配線 + glslang library runtime API + SPIR-V cache layer** に refine (`handoff-substep-4-3-gamma-prime-prep.md` §4、案 A target extension build-time pre-compile から path 変更、根拠 = LL/FS shader variant 爆発 model + LLShaderMgr 既存 preprocessing 1 source of truth + 3 OS 整合)、本 §3 #4 acceptance test procedure + §2 領域 6 risk 内訳 を同時 refine、AYA review 待ち

---

## §8 関連 doc / memory

本 charter 完成時点の **all-in-one 関連 list**。本 charter 内で参照する全 doc + memory + feedback を完全列挙。

### §8.1 r40 章 doc (本 r41 charter の前置)

| doc | 本 charter での参照 section |
|---|---|
| `docs/specs/ayastorm-r40-vulkan-migration/00-charter.md` | §1 thesis (3) Linux baseline + §5.4 工程プラン 8 件確定値 + §6.5 rule reminder (charter §4 (3) 無期限 + §7 判断軸) |
| `docs/specs/ayastorm-r40-vulkan-migration/01-sub-phase-1-cpu-perf.md` | §1 thesis (parity 不要は sub-phase 1 root cause = pipeline.cpp 3 大グローバル並列化阻止の解決として Vulkan 化選択の論拠) |
| `docs/specs/ayastorm-r40-vulkan-migration/02-sub-phase-2-extended-falsify.md` | §1 thesis (rocket fuel = sub-phase 2 鉱脈ゼロ falsify が r41 着手前提) |
| `docs/specs/ayastorm-r40-vulkan-migration/03-sub-phase-3-vulkan-plan.md` | §5 依存 milestone (sub-phase 3 = 本 r41 charter 起草の親 phase、closed 2026-05-28) |
| `docs/specs/ayastorm-r40-vulkan-migration/04-portage-inventory.md` | §2 work breakdown 領域 1-5 backbone (§5.4 段階 port 戦略 5 段階) + §3 #3 段階 file 群 + §5.3 doc cross reference |
| `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` | §1 thesis (2) Vulkan 空転 + §2 領域 6/7/8 + §3 #5/#6 acceptance + §5.3 doc cross reference + §5.5 環境前提 |
| `docs/specs/ayastorm-r40-vulkan-migration/06-effort-estimation.md` | §2 work breakdown PM 値 + §4 暦月 + uncertainty band + plan B trigger + §5.3 doc cross reference |
| `docs/specs/ayastorm-r40-vulkan-migration/07-r42-plus-milestone-mapping.md` | §1 thesis next milestone 引継ぎ (r41 → r41.5 → r42-α/β/γ/δ) + §5.3 doc cross reference + §6 起草 base (§5.1 outline) |

### §8.2 r40 章 handoff doc (本 r41 charter 起草の cadence base)

| handoff doc | 本 charter での参照 |
|---|---|
| `docs/specs/ayastorm-r40-vulkan-migration/handoff-work-item-d-foundation-complete.md` | §6.2 Pattern β cadence 実績 (work item (d) foundation §1+§2) |
| `docs/specs/ayastorm-r40-vulkan-migration/handoff-work-item-d-group-a-complete.md` | §6.2 Pattern β cadence 実績 (work item (d) group A §3+§4) |
| `docs/specs/ayastorm-r40-vulkan-migration/handoff-work-item-d-group-b-complete.md` | §6.2 Pattern β cadence 実績 (work item (d) group B §5+§6) |
| `docs/specs/ayastorm-r40-vulkan-migration/handoff-work-item-e-complete.md` | §6.2 Pattern β cadence 実績 (work item (e) 5 sub-step) |
| `docs/specs/ayastorm-r40-vulkan-migration/handoff-r40-close-r41-charter-start.md` | §6 本 charter 起草の境界 handoff (本 doc 起草の direct trigger) |

### §8.3 関連 memory (本 r41 charter の thesis + cadence 確定 source)

| memory | type | 本 charter での参照 |
|---|---|---|
| `project_ayastorm_r40_cpu_parallel.md` | project | §1 thesis + §5.1 r40 達成達成済 + §5.4 工程プラン 8 件確定値 (r40 章 close 2026-05-28) |
| `project_ayastorm_r40_extended.md` | project | §1 thesis (sub-phase 2 falsify の history、本 r41 着手前提) |
| `project_ayastorm_r41_vulkan_migration.md` | project | §1 thesis r41 scope 確定 (GL 除去 + Vulkan 空転 single milestone) + §6.3 (γ) memory update target |
| `project_ayastorm_three_platforms.md` | project | §1 thesis (3) Linux baseline 例外 + §3 #7 #9 acceptance + §6.5 rule reminder |
| `project_ayastorm_release_chapters.md` | project | r41 milestone の release 番号帯位置付け (r30+ 撮影描画章後の Vulkan 化 chapter) |

### §8.4 関連 feedback (r41 着手中に適用される rule)

| feedback | 本 charter での参照 |
|---|---|
| `feedback_falsification_as_progress.md` | §1 thesis (sub-phase 1/2 REJECT/ゼロ の絞り込み成果が r41 着手 thesis 確立) |
| `feedback_proactive_handoff.md` | §6 group 境界 handoff + 本 charter 完成後 handoff (§6.3 (δ)) |
| `feedback_self_verify_before_handoff.md` | §6.2 group 完了時 self-trace + r41 着手中 段階別 self-trace |
| `feedback_no_auto_commit.md` | §6.4 commit は AYA 明示指示 |
| `feedback_release_flow.md` | §6.4 push は AYA 手動 |
| `feedback_no_dual_doc_split.md` | §6 起草先 `docs/specs/ayastorm-r41-gl-removal/` 同居方針 (内部/公開 doc 分離禁止) |
| `feedback_build_only_verified.md` | §6.5 r41 着手中の acceptance satisfy は実機検証 (推論 ban) |
| `feedback_admit_unknown.md` | §6.5 仮説 2 連続外れたら log/canary/bisect 切替 |
| `feedback_one_step_at_a_time.md` | §6.4 並走 cadence (1 メッセージ 1 アクション) |
| `feedback_use_agents_proactively.md` | r41 着手中の trace / grep / 段階別 audit で Agent 活用 |
| `feedback_no_scope_shrink.md` | §3 #1-#9 acceptance criterion 全 PASS で達成 (literal scope 維持) |
| `feedback_self_bug_no_defer_option.md` | §3 #1-#9 unmet 時の対処は fix のみ提示 (defer/disable 提案 ban) |
| `feedback_perf_map_bfs_drill.md` | r41 着手中の段階別 audit (A 全周 → B 1 段深い全周) |
| `feedback_explanation_lead_with_conclusion.md` | §3 acceptance / §4 暦月 / §5 依存 の結論ファースト記述方針 |
| `feedback_release_with_user_feedback.md` | §3 acceptance 完璧主義回避 (実装中 refine 可、AYA 動作確認で補完) |

### §8.5 起草中の本 charter 完成後生成予定 doc (§6.3 反映)

| doc | 生成 timing | 内容 |
|---|---|---|
| `docs/specs/ayastorm-r41-gl-removal/handoff-r41-charter-complete.md` | 本 charter group B AYA review PASS 直後 (§6.3 (δ)) | r41 charter 完成宣言 + r41 着手前 prep cadence + 次 session base 資料 |
| `docs/specs/ayastorm-r41-gl-removal/01-foundation.md` | r41 着手後 (§7.4 sub-doc) | 環境前提整備 + branch 戦略確定 + 段階 1 file list |
| `docs/specs/ayastorm-r41-gl-removal/02-portage-execution.md` | r41 着手後 (§7.4 sub-doc) | 段階 2-5 file list + dependency graph + port 順序 |
| (以下 §7.4 table 参照) | r41 着手中の各段階完遂時 | sub-doc 7 件 (§7.4 outline) |

### §8.6 関連 doc + memory cross reference 整合

- 本 §8.1-§8.4 で参照する全 doc / memory / feedback は本 charter 内 11 section (header + §1-§8 各 sub-section) で **重複 cross reference** + **個別参照** の両方を維持
- r40 章 doc 8 件 + handoff doc 5 件 + memory 5 件 + feedback 15 件 = **計 33 件 cross reference 完備**
- 本 §8 list は本 charter 完成時点で凍結、r41 着手中の追加 doc / memory は §8.5 + §7.4 sub-doc に集約 (§8 本体は本 charter 完成時の snapshot として維持)

---

## charter 完成宣言 (2026-05-28)

本 r41 charter は **foundation + group A + group B 全 AYA review PASS** で完成宣言 (2026-05-28)、status field を **closed 2026-05-28 (r41 charter 完成、r41 着手準備 ready)** に更新済。

### 完成宣言の内訳

- **foundation group** (header + §1 thesis + §2 work breakdown): AYA review PASS (2026-05-28)
- **group A** (§3 acceptance criteria + §4 暦月変換 + §5 依存 milestone): AYA review PASS (2026-05-28)
- **group B** (§6 起草 cadence + §7 詳細化方針 + §8 関連 doc / memory): AYA review PASS (2026-05-28)
- **起草 cadence**: Pattern β (group 分割)、r40 章 work item (c)(d)(e) 実績継承
- **起草所要 session**: 1 session (本 session で foundation → group A → group B → 完成宣言まで通し実施)

### 完成宣言と同時実施 (§6.3 mandatory action)

- **(γ) memory update**: `project_ayastorm_r41_vulkan_migration.md` の status を **pending → active** に更新 + 本 charter cross reference 追加
- **(δ) handoff doc 作成**: `handoff-r41-charter-complete.md` 新規作成、次 session で r41 着手前 prep の base 資料

### r41 着手準備 (本 charter 完成後の次 action)

- **(α) r41 着手** (次 session 想定): 本 charter §2 領域 1 (段階 1: GL header wrapper 置換 + volk loader) 着手、branch 戦略確定 (`feature/ayastorm-r41-gl-removal` 新規 or 現 branch 継承) + 環境前提整備 (LunarG SDK + volk + VMA + glslang 統合)
- **(β) r41.5 charter 起草** (本 r41 完遂後): r41 達成宣言時点で 07 doc §5.2 r41.5 charter outline base で r41.5 charter 先行起草、本 r41 着手前は未着手

### 関連 commit (本 charter 完成 commit は AYA 指示後実施)

本 charter 完成 commit は `feedback_no_auto_commit.md` 遵守で **AYA 明示指示後 Claude が実施**。commit message draft (AYA 指示時に refine 可):

```
docs(r41): r41 charter 完成 (GL 除去 + Vulkan 空転 vk-α) + memory active 化 + handoff doc

- 00-charter.md 新規作成 (foundation + group A + group B 全 AYA review PASS 2026-05-28)
- §1 thesis (GL 除去 / Vulkan 空転 / Linux baseline) + §2 work breakdown 11 領域 16.17 PM
- §3 acceptance criteria 9 件 (metric + test + regression 詳細化) + §4 暦月 ~84 中央値 / ~2033 marker
- §5 依存 milestone (r40 達成済 + 設計 doc 確定 + 環境前提) + §6 起草 cadence Pattern β
- §7 詳細化方針 (sub-doc 7 件 outline) + §8 関連 doc/memory 33 件 cross reference
- memory `project_ayastorm_r41_vulkan_migration.md` status pending → active 更新
- handoff-r41-charter-complete.md 新規作成 (次 session r41 着手前 prep base)
```

push は AYA 手動 (`feedback_release_flow.md` 遵守)。
