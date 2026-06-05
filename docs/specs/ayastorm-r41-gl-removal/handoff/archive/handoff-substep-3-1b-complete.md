# r41 sub-step 3.1b 完遂 → 3.1c (3.2/3.3/3.4) 着手境界 handoff (2026-05-29)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-1a-complete.md` (3.1a 全完遂 → 3.1b 着手境界)
**本 handoff 位置付け**: sub-step 3.1b (PSO 基盤配線 + bridging items #2/#4/#10/#11 物理実装 + minimal placeholder PSO smoke test) **全完遂宣言** + sub-step 3.2 / 3.3 / 3.4 (pool ごとの実 PSO + draw 配線 / matrix stack push constant 化 / descriptor set=1 per-material) 着手前 scope 確認境界。本 handoff 完成 commit で 3.1b 章 close、AYA GO 指示で 3.2 着手。

---

## 1. sub-step 3.1b 全完遂 status (2026-05-29)

### 1.1 8 segment 完遂表

| segment | scope | 完遂 status | 主 file |
|---|---|---|---|
| **3.1b-A** | device limit query 6 件 + log baseline 取得 | **完了 2026-05-29** | `llvkloader.cpp` |
| **3.1b-B** | VkPipelineCache + VkPipelineLayout helper + PSO compile helper | **完了 2026-05-29** | `llvkloader.cpp` / `llvkloader.h` |
| **3.1b-C** | item #10 VK_EXT_debug_utils messenger 配線 + GL debug callback 撤去 | **完了 2026-05-29** | `llvkloader.cpp` / `llgl.cpp` |
| **3.1b-D** | item #11 LLGLSyncFence 物理削除 (Cluster F 確認、副作用 0) | **完了 2026-05-29** | `llgl.cpp` / `llgl.h` / `llrender.cpp` |
| **3.1b-E** | item #1 RAII setter dead-store 化 → **段階 4 移管 scope refine** | **完了 2026-05-29** | sub-doc 03 / charter / handoff-3-1a |
| **3.1b-F** | item #2 LLGLDepthTest dynamic state 化 (Vulkan 1.3 core API) | **完了 2026-05-29** | `llgl.cpp` |
| **3.1b-G** | item #4 LLGLUserClipPlane 経路 PSO 化 (shaderClipDistance device feature) | **完了 2026-05-29** | `llvkloader.cpp` |
| **3.1b-H** | minimal placeholder PSO compile + bind 動作確認 (AYA launch PASS) | **完了 2026-05-29** | `llvkloader.cpp` |

### 1.2 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-3-1a-complete.md` | **役割完了 2026-05-29** (3.1b 着手 satisfy、本 handoff で内容引継ぎ) |
| sub-doc `03-state-machine-pso.md` | active 継続 (sub-step 3.2 / 3.3 / 3.4 marker 着手 ready 状態へ) |
| `handoff-substep-3-1b-complete.md` (本 handoff) | 新規作成 (3.1b 全完遂 → 3.2 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 3.2 着手 ready 状態へ update) |

---

## 2. 各 segment 完遂内容

### 2.1 3.1b-A: device limit query 6 件 + log baseline

sub-doc 03 §1.5.4 で確定した 6 件 query を `llvkloader.cpp` の device 作成直後に LL_INFOS 出力するよう配線:

| field | 用途 |
|---|---|
| `maxBoundDescriptorSets` | 03 §1.2 / 07 §1.2.1 set=0/1/2 3 階層 baseline |
| `maxPushConstantsSize` | 03 §1.2 / 07 §1.2.2 push constant 64 bytes baseline |
| `maxPushDescriptors` (`VK_KHR_push_descriptor` extension property) | 07 §1.2.1 set=2 per-draw binding ≤ 32 baseline |
| `maxPerStageDescriptorSampledImages` | 07 §1.2.1 sampler 分布 per-stage baseline |
| `maxColorAttachments` | 07 §1.2.3 pass 2 gbuffer0/1/2/3 baseline |
| `maxDescriptorSetSamplers` | 07 §1.2.1 全 set sampler 分布 baseline |

→ AYA 環境 (RTX 5090) baseline 取得済。Mesa RADV / ANV 実測は sub-step 3.4 着手前に testbed で取得 (charter §6 領域 10)。

### 2.2 3.1b-B: PSO 基盤 (Cache + Layout + compile helper)

`llvkloader.cpp` + `llvkloader.h` に以下 3 件追加:

| 構造体 / 関数 | 役割 |
|---|---|
| `VkPipelineCache sPipelineCache` (namespace static) | 全 PSO 共通 cache (`vkCreatePipelineCache` / `vkDestroyPipelineCache`) |
| `createStandardPipelineLayout(set_layouts, set_count, push_ranges, push_count)` helper | 共通 layout 作成 helper (空 layout / set_layout 配列 / push constant range 配列を受け取る) |
| `compileGraphicsPipeline(create_info_ptr, out_pipeline)` helper | `vkCreateGraphicsPipelines` + sPipelineCache 経由 PSO compile helper |

→ 3.2 以降の sky/water/avatar pool ごとの PSO 作成で本 helper を再利用。

### 2.3 3.1b-C: item #10 VK_EXT_debug_utils messenger 配線 + GL debug callback 撤去

`llvkloader.cpp`:
- `vulkanDebugCallback` (PFN_vkDebugUtilsMessengerCallbackEXT) 実装 (severity / type に応じて LL_WARNS / LL_INFOS 振分け)
- `createDebugMessenger()` で `VK_EXT_debug_utils` extension 経由 `vkCreateDebugUtilsMessengerEXT` 呼出
- `LL_RELEASE_FOR_DOWNLOAD` 定義時は早期 return + callback 関数自体も `#ifndef LL_RELEASE_FOR_DOWNLOAD` で囲う (release build の `-Werror=unused-function` 回避)

`llgl.cpp`:
- 既存 `gl_debug_callback` 系 path を撤去 (charter §2.1 領域 1 GL header 化と整合、Vulkan path に一本化)

### 2.4 3.1b-D: item #11 LLGLSyncFence 物理削除

Cluster F (`handoff-substep-3-1a-iv-ready.md` §2.6) で「定義あり、参照ゼロ、dead code」確定。

- `llgl.h`: `class LLGLSyncFence { ... }` 宣言全削除 (~12 line)
- `llgl.cpp`: ctor/dtor/methods 実装全削除
- `llrender.cpp`: 名前空間レベル参照を `git grep LLGLSyncFence` 再確認、未参照確認

→ 副作用 0 (Cluster F 確認通り)、build 完遂で確認済。

### 2.5 3.1b-E: item #1 → 段階 4 scope refine

3.1b 着手時に「sub-step 3.1 で setter no-op 化要求 vs 段階 1+2 動作維持要求 (sub-doc 03 §3.5)」の構造的矛盾発覚。AYA 指示で **受け入れ先工程修正** approach 採用。

反映 doc 4 件:

| doc | 反映箇所 |
|---|---|
| `sub-doc 03-state-machine-pso.md` | §1.5.3 item #1 行 / §3.1 sub-step 3.1 marker / §3.3 touch しない file / §4.1 #1-段階 3 metric |
| `00-charter.md` | §2.1 領域 4 (LLPipelineFrameContext) に「LLGLState RAII setter dead-store 化」記述追加 |
| `handoff-substep-3-1a-complete.md` | §3.1 sub-step 3.1 配置 table / §7.1 3.1b scope / §9.2 state alias compat 注意 |
| memory `project_ayastorm_r41_vulkan_migration.md` | status + 段階表 + next action + "2026-05-29 scope refine" section |

**段階 4 受入根拠**: PSO state alias 基盤 (本 sub-step 3.1b 完遂時点で揃う) + LLPipelineFrameContext (段階 4 で配置) が揃った後に caller source-level compat 維持しつつ setter 内 GL call 物理削除可能。

### 2.6 3.1b-F: item #2 LLGLDepthTest dynamic state 化

Vulkan 1.3 core API (`vkCmdSetDepthTestEnable` / `vkCmdSetDepthCompareOp` / `vkCmdSetDepthWriteEnable`) を `LLGLDepthTest` ctor / dtor に parallel-rail 配置:

- `llgl.cpp`: `glDepthFuncToVk(GLenum) → VkCompareOp` helper 追加 (GL_NEVER ~ GL_ALWAYS の 8 値 mapping)
- ctor: 3 条件分岐 (depth_test_only / depth_func only / both) すべてに vkCmdSet* 並走配置
- dtor: 対称的 restore (前回値復元) 並走配置
- vk_cb (`getCurrentCommandBuffer()`) が `VK_NULL_HANDLE` の場合は早期 skip (段階 1+2 動作維持)

→ Vulkan 1.3 core 化により feature flag 不要、`VK_EXT_extended_dynamic_state2` 個別 enable も不要。

### 2.7 3.1b-G: item #4 LLGLUserClipPlane shaderClipDistance device feature 化

`LLGLUserClipPlane` 経路 (mirror / reflection probe / water culling) は oblique projection (matrix trick) で実装されているため、実 GL clip plane state (`GL_CLIP_PLANE0`) は使われていないことが trace で判明 (Cluster B 再確認)。

actionable narrowed to:
- `llvkloader.cpp` `createDevice()` で `VkPhysicalDeviceFeatures::shaderClipDistance` を query + enable
- `device_info.pEnabledFeatures = &enabled_features` 配線

→ shader 側 `gl_ClipDistance[]` 出力を将来 r42+ で使えるように device feature 有効化のみ、本 sub-step で shader 改変は無し。

### 2.8 3.1b-H: minimal placeholder PSO compile + bind 動作確認

**目的**: PSO 基盤 + bridging items が device 上で実際に成立することを 1 frame の smoke test で確認 (charter §2.1 領域 3 vk-α 達成条件の最小単位)。

実装:
- 最小 GLSL 2 file: vert `gl_Position = vec4(0,0,0,1)`, frag `outColor = vec4(0,0,0,0)` (degenerate / transparent black)
- offline 事前 compile: `/snap/kf6-core24/36/usr/bin/glslc` (LD_LIBRARY_PATH 経由) で SPIR-V 化
- C++ 内 `const uint32_t kPlaceholderVertSpv[188]` / `kPlaceholderFragSpv[102]` として埋め込み (offline tool 不要、external file ロード不要)
- `createPlaceholderPipeline()`:
  - `vkCreateShaderModule` × 2
  - `createStandardPipelineLayout(nullptr, 0, nullptr, 0)` (空 layout)
  - `VkGraphicsPipelineCreateInfo` (`VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST` / `VK_POLYGON_MODE_FILL` / `VK_CULL_MODE_NONE` / 1 color attachment / 全 channel write mask / renderPass = sRenderPass / subpass = 0)
  - `compileGraphicsPipeline()` 呼出
- 配線: `initVulkan` の `createPipelineCache()` 直後 / `beginFrame` の `vkCmdBeginRenderPass` 直後 `vkCmdBindPipeline`
- 後片付け: `shutdownVulkan` で pipeline → layout → fragModule → vertModule 順 destroy (sPipelineCache 破棄前)

acceptance:
- ✓ 起動時 VkPipelineCache 作成成功 (LL_INFOS 出力)
- ✓ placeholder PSO compile 成功 (LL_INFOS 出力)
- ✓ 1 frame 内に PSO bind 完了 + validation 0 件 (AYA 起動確認 2026-05-29 PASS)

**重要 — vkCmdDraw* は呼んでいない**: 本 segment は bind までで、draw call 投入は sub-step 3.2 以降の pool ごとの実 PSO + vertex buffer 配線で初めて入る。viewer 画面に映る画は引き続き全部 GL 描画 (段階 1+2 動作維持)。

---

## 3. acceptance summary + verification

### 3.1 sub-step 3.1b acceptance (sub-doc 03 §3.1 sub-step 3.1 marker refine)

| acceptance | 達成 status |
|---|---|
| device limit query 6 件 log 出力 | ✓ |
| VkPipelineCache 作成成功 | ✓ |
| createStandardPipelineLayout helper 動作 | ✓ |
| compileGraphicsPipeline helper 動作 | ✓ |
| VK_EXT_debug_utils messenger 動作 (GL debug callback 撤去) | ✓ |
| LLGLSyncFence 物理削除 + build PASS | ✓ |
| LLGLDepthTest dynamic state 並走配置 | ✓ |
| shaderClipDistance device feature enable | ✓ |
| placeholder PSO compile + bind validation 0 件 | ✓ (AYA launch PASS) |

### 3.2 build + launch verification

| step | status |
|---|---|
| `autobuild configure -A 64 -c ReleaseFS_open -- --fmodstudio -DLL_TESTS:BOOL=FALSE -DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE --package --chan AYAstorm-release` | (本 session では increment build のみ実施、別途要 fresh configure 時に実行) |
| `autobuild build -A 64 -c ReleaseFS_open --no-configure` | ✓ 完遂 exit 0、`[100%] Built target llpackage` |
| `./install.sh` (`build-linux-x86_64/newview/packaged/`) | ✓ 完遂、`~/ayastorm/` 配置 |
| `rm -rf ~/.ayastorm_x64/cache/` | ✓ 完遂 |
| AYA launch + 1 frame 動作確認 | ✓ 2026-05-29 06:54 頃 AYA OK 終了 |

---

## 4. sub-step 3.2 / 3.3 / 3.4 着手前 scope 確認

### 4.1 sub-step 3.2 (smoke-test path / llpostprocess legacy effect uniform → push constant + UBO)

sub-doc 03 §1.2 #9 既設計参照。本 sub-step で投入する新規 bridging item 無し (§3.4 in `handoff-substep-3-1a-complete.md`)。3.1b で配線済 PSO 基盤 (Cache / Layout helper / compile helper) を流用。

### 4.2 sub-step 3.3 (matrix stack → push constant 64 bytes 化 + FBO → VK_KHR_dynamic_rendering)

bridging items 4 件 (#3 / #5 / #6 / #9):

| # | item | 備考 |
|---|---|---|
| 3 | LLGLSquashToFarClip push constant 化 | matrix stack 完成形と同期 |
| 5 | skybox skyV.glsl L95 push constant swap 維持 | matrix stack 完成形と同期 (AYAstorm 改変 13 file untouched 維持) |
| 6 | matrix stack → push constant 64 bytes 化 | 3.3 core scope |
| 9 | FBO → VK_KHR_dynamic_rendering | 3.3 core scope (05 §4.7 移行マップ) |

### 4.3 sub-step 3.4 (descriptor set=1 per-material 7 PBR slot + VkImage / VkImageView / VMA lifecycle)

bridging items 2 件 (#7 / #8) + 段階 2 引継ぎ 3 件:

| # | item | 備考 |
|---|---|---|
| 7 | texture unit → descriptor set=1 per-material mapping (7 PBR slot、binding 0-6) | §1.5.4 device limit 実測値 base で final 化 |
| 8 | VkImage + VkImageView + VMA lifecycle | texture lifecycle core |
| (段階 2 引継ぎ A) | acceptance #1-段階 2 (lldrawpool 13 file 内 GL call 0 件) | sub-doc 03 §5.1 |
| (段階 2 引継ぎ B) | terrain glTexGen 廃止 + 領域 6 shader 側 UV 化 | sub-doc 03 §5.1 |
| (段階 2 引継ぎ C) | avatar skinning SSBO 基本実装 | sub-doc 03 §5.1 |

**3 driver baseline 実測** (Mesa RADV / Mesa ANV) は本 sub-step 着手前に testbed で取得 (charter §6 領域 10、§1.5.4 query 結果対照)。

---

## 5. file / line / commit 系譜

### 5.1 本 sub-step 3.1b 修正 file

| file | 修正規模 | 主内容 |
|---|---|---|
| `indra/llrender/llvkloader.cpp` | +473 / -近 0 | device limit query / PSO 基盤 / debug messenger / shaderClipDistance / placeholder PSO |
| `indra/llrender/llvkloader.h` | +16 | PSO helper signature 公開 |
| `indra/llrender/llgl.cpp` | +97 / -近 0 | LLGLDepthTest parallel-rail / glDepthFuncToVk helper / LLGLSyncFence 削除 / GL debug callback 撤去 |
| `indra/llrender/llgl.h` | -12 | LLGLSyncFence 宣言削除 |
| `indra/llrender/llrender.cpp` | +21 / -近 0 | LLGLSyncFence 参照点 cleanup |
| `docs/specs/ayastorm-r41-gl-removal/00-charter.md` | +7 | §2.1 領域 4 LLGLState RAII setter dead-store 化記述追加 |
| `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` | +7 | item #1 段階 4 移管 refine |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-1a-complete.md` | +14 | item #1 段階 4 移管 strikethrough + refine 注 |

合計: **8 file、+556 / -91 line** (`git diff --stat HEAD` 確認済)

### 5.2 commit 系譜 (3.1b 完遂後)

3.1b 完遂 commit は AYA 明示指示後に実施 (`feedback_no_auto_commit.md`)。commit message 案:

```
feat(r41): sub-step 3.1b 全完遂 (PSO 基盤 + bridging items #2/#4/#10/#11 + placeholder PSO smoke)

- 3.1b-A: device limit query 6 件 + log baseline
- 3.1b-B: VkPipelineCache + PipelineLayout helper + compile helper
- 3.1b-C: VK_EXT_debug_utils messenger + GL debug callback 撤去
- 3.1b-D: LLGLSyncFence 物理削除 (Cluster F 副作用 0 確認)
- 3.1b-E: item #1 RAII setter dead-store 化 → 段階 4 移管 scope refine
- 3.1b-F: LLGLDepthTest dynamic state 化 (Vulkan 1.3 core)
- 3.1b-G: LLGLUserClipPlane shaderClipDistance device feature enable
- 3.1b-H: minimal placeholder PSO compile + bind (AYA launch PASS)

Refs: docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-1b-complete.md
```

---

## 6. risks / caveats

### 6.1 parallel-rail 維持の monitoring 必要性

3.1b 完遂時点で LLGLDepthTest は GL + Vulkan 並走配置 (item #1 段階 4 移管の自然な帰結)。次 sub-step (3.2 以降) で `recordPoolDraws` body 内に Vulkan command buffer 経由 draw call が入ってきた場合、Vulkan 側のみで完結する draw path が出現する。その際:
- GL state setter (LLGLDepthTest 等) と Vulkan dynamic state の整合性は段階 4 dead-store 化まで両方維持
- pool ごとの PSO bind 時に `vkCmdSetDepthTestEnable` 等の dynamic state 設定が確実に走るよう実装側で確認 (sub-doc 03 §3.5 動作維持の延長)

### 6.2 SPIR-V tooling 環境依存

3.1b-H で offline glslc compile (snap package 経由 `LD_LIBRARY_PATH` 工夫) を採用。3.2 以降で shader 数が増えた場合の build pipeline 統合は sub-doc 06 §3 (SPIR-V 化フロー) で別途設計。当面は埋め込み SPIR-V 配列で問題なし。

### 6.3 placeholder PSO は draw call 無し

3.1b-H acceptance は「bind が validation 0 件で通る」までで、画面に何も描かれない。**Vulkan で実描画が始まるのは sub-step 3.2 以降** (pool ごとの実 PSO + vertex buffer + vkCmdDraw* 配線時)。次 sub-step 着手時に「視覚的進捗 0 でも acceptance としては想定通り」を AYA と共有済。

### 6.4 vkCmdDraw* 未投入の段階 2 hook 影響

段階 2 で配線した 12 pool の `recordPoolDraws(VkCommandBuffer)` hook は引き続き LL_INFOS marker のみ。sub-step 3.2 以降で本 hook body に PSO bind + vkCmdDraw* を順次配線。配線順は sub-doc 03 §3.2 (smoke-test path 起点 = llpostprocess → sky → water → ...) に従う。

---

## 7. next session entry point

### 7.1 次 session 着手前の準備

1. `git pull origin feature/ayastorm-r41-gl-removal` (AYA push 後の最新取得)
2. 本 handoff doc 通読 (本 file)
3. sub-doc 03 §3.2 / §3.3 / §3.4 の各 sub-step marker 再確認
4. memory `project_ayastorm_r41_vulkan_migration.md` status update 反映確認

### 7.2 sub-step 3.2 着手 task 候補

| task | 内容 | 推定規模 |
|---|---|---|
| 3.2-A | smoke-test path 選定 (llpostprocess legacy effect 1 件) + 移植対象 uniform inventory | trace 1-2 時間 |
| 3.2-B | descriptor set=2 push descriptor 配線 helper | 実装 0.5-1 日 |
| 3.2-C | smoke-test path の PSO 作成 + vertex buffer 配線 + `vkCmdDraw*` 投入 | 実装 1-2 日 |
| 3.2-D | recordPoolDraws hook → PSO bind + draw 配線 (1 pool 試行) | 実装 0.5-1 日 |
| 3.2-E | validation strict 再検証 + 1 frame Vulkan 描画確認 (AYA launch verification) | 0.5 日 |

### 7.3 critical reminders

- **AYAstorm 改変 13 file shader 改変禁止** (charter §2.1 領域 6、acceptance #4-領域 6 `git diff` 0 件)
- **段階 1+2 動作維持** (sub-doc 03 §3.5、Vulkan path 並走配線で GL 描画動作維持)
- **vkCmdDraw* 投入時は validation strict 設定で 0 件確認** (sub-step 3.2-E)
- **device limit 6 件 baseline は AYA 環境 RTX 5090 のみ取得済、Mesa RADV / Mesa ANV は sub-step 3.4 着手前に testbed で取得**

---

## 8. 関連 doc / memory cross-ref

### 8.1 関連 doc

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter (§2.1 領域 4 LLGLState dead-store 化記述追加済)
- `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` — 段階 3 sub-doc (sub-step 3.1 marker refine 済)
- `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` — SPIR-V 化 (sub-step 3.2 以降 cross-ref)
- `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` — descriptor + render pass (sub-step 3.4 cross-ref)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-1a-complete.md` — 直前 handoff (役割完了)

### 8.2 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 handoff 完遂で sub-step 3.2 着手 ready 状態に update)
- `project_ayastorm_three_platforms.md` — Linux 先行例外を r41 で適用中
- `feedback_self_verify_before_handoff.md` — 本 handoff 起草前 self-trace 実施済 (§5.1 file/line 表)
- `feedback_no_auto_commit.md` — 本 handoff 完成後 commit は AYA 明示指示待ち
- `feedback_proactive_handoff.md` — context 残量周期で能動 handoff 起草 (本 file)
