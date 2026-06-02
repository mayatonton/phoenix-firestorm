# r41 sub-step 2.1a 完遂 → 2.1b 着手境界 handoff (2026-05-28)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-stage-1-complete.md` (段階 1 完遂 → 段階 2 着手前 prep の境界)
**本 handoff の位置付け**: 段階 2 sub-step 2.1a (Vulkan command 基盤 + minimal render pass + per-frame 空 record cycle) 完遂境界。次は 2.1b (base orchestrator + 軽量 4 pool record 配線) 着手前 scope 確認 + 実装。

---

## 1. sub-step 2.1a 完遂 state (2026-05-28)

### 1.1 完遂宣言

- sub-step 2.1a 完遂、02-portage-execution.md §3.1 sub-step 2.1 を 2.1a + 2.1b に分割 (charter §7.5 boundary refine) 後の前半 satisfy
- 達成内容:
  - `llvkloader.h` に `volk.h` include + 3 公開 API 追加 (`beginFrame()` / `endFrame()` / `getCurrentCommandBuffer()`)
  - `llvkloader.cpp` +~210 LOC:
    - `VkCommandPool` (`TRANSIENT_BIT` + `RESET_COMMAND_BUFFER_BIT`) + `VkCommandBuffer` 1 本 (primary)
    - `VkImage` 64×64 `R8G8B8A8_UNORM` (color attachment) + `VkDeviceMemory` (`DEVICE_LOCAL`) + `VkImageView`
    - `VkRenderPass` (1 color attachment、`LOAD_OP_CLEAR` / `STORE_OP_STORE`、`COLOR_ATTACHMENT_OPTIMAL` final)
    - `VkFramebuffer` (image view wrap)
    - `beginFrame()`: `vkResetCommandBuffer` → `vkBeginCommandBuffer` → `vkCmdBeginRenderPass`
    - `endFrame()`: `vkCmdEndRenderPass` → `vkEndCommandBuffer` (**submit せず discard**)
    - `shutdownVulkan()` 拡張: `vkDeviceWaitIdle` + 全 resource 逆順 destroy
  - `llappviewer.cpp` L1781 `display()` を `LLVKLoader::beginFrame()` / `LLVKLoader::endFrame()` で挟む (per-frame 空転 hook)
- 達成 marker: viewer 起動 log L89-L92 全 init message 出現 (Command pool / Offscreen image 64x64 / Render pass / Framebuffer)、shutdown 時 L2536-L2537 device + instance 正常 destroy、全 `VkCreate*` の error return 0
- commit: `49b9f36427` (`feat(r41): sub-step 2.1a 完了 ...`)
- branch: `feature/ayastorm-r41-gl-removal`

### 1.2 段階 2 内 sub-step 進捗 (02-portage-execution.md §3.1 反映)

| sub-step | 内容 | commit | state |
|---|---|---|---|
| **2.1a** | command 基盤 + minimal render pass + per-frame 空 record cycle | `49b9f36427` | **完遂 2026-05-28** ✓ |
| **2.1b** | base orchestrator + 軽量 4 pool record 配線 (lldrawpool.cpp + sky + waterexclusion + pbropaque + simple) | (次 session) | **scope 確認 + 着手 pending** |
| 2.2 | 標準 5 pool (alpha + tree + bump + materials + water) | — | 未着手 |
| 2.3 | atmospherics pool (wlsky) | — | 未着手 |
| 2.4 | 特殊対応 pool (terrain + avatar) | — | 未着手 |
| 2.5 | 段階 2 self-check + handoff | — | 未着手 |

### 1.3 status close / next active

| doc / memory | status |
|---|---|
| sub-doc `02-portage-execution.md` | `closed 2026-05-28` (Pattern α 一括 draft + AYA review PASS、段階 2 着手準備 ready) → 本 sub-doc は段階 2 全完遂までの参照 active |
| `handoff-stage-1-complete.md` §3.1.1 loader version log mystery | **解消済 2026-05-28** (warmup 行追加で fix、commit `0ce8ea8df5`) |
| `handoff-substep-2-1a-complete.md` (本 handoff) | 新規作成 (sub-step 2.1a 完遂 → 2.1b 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 |

---

## 2. 2.1b 着手前の scope 確認 (次 session 最優先)

### 2.1 scope の論点

drafted 02-portage-execution.md §3.1 sub-step 2.1b 完了 marker は:

> base orchestrator が 2.1a 基盤を経由 record 配線、軽量 4 pool が `vkCmdDraw*` placeholder record (PSO 統合は段階 3、本段階は dummy pipeline binding で record 単体動作) + validation 0 件、bridging template 確定

**ただし PSO (VkPipeline) は段階 3 領域 3 担当で 2.1b 時点で存在せず**、pipeline bind なしでの `vkCmdDraw*` は validation 上 `unbindPipeline` error 確実。"dummy pipeline binding" 文言は段階 3 PSO 完成時に意味付けされる読み替えが必要。

### 2.2 3 interpretation

| 案 | 内容 | trade-off |
|---|---|---|
| **(1) hook wiring only (推奨)** | `LLDrawPool::recordPoolDraws(VkCommandBuffer)` virtual を base 追加、4 light pool は空 override、base orchestrator dispatcher が hook 呼出。**vkCmd\* 自体は未投入** | validation 0 件 楽に satisfy、bridging template 確立、実 draw 投入は段階 3 PSO 完成時に同 hook body に配線 |
| (2) vkCmdSetViewport / Scissor で presence マーク | 4 pool が dynamic state 命令投入 (pipeline 不要、validation OK) | dynamic state は通常 pipeline 既定で意味薄、空転に近い |
| (3) 段階 3 PSO 完成待ち (schedule swap) | 2.1b を段階 3 後に再配置 | spec dependency 順序違反、recommend NG |

### 2.3 推奨 (1) の根拠

- spec `vkCmdDraw*` 文言は段階 3 PSO 完成時の semantics として読み替え可
- 2.1b 単独の "bridging template 確定" satisfy は wiring 配線で十分
- validation 0 件 acceptance を strict に満たす最短経路
- 段階 3 follow-up commit で hook body に PSO bind + draw 投入 (3 行程度)、2.1b → 段階 3 の橋渡しが clean

### 2.4 次 session 最初のアクション

1. **本 handoff 確認** (`handoff-substep-2-1a-complete.md`) を Read
2. **scope 確定**: AYA に上記 (1) で進めるか、再選択するか確認
3. **2.1b 着手**: 推奨 (1) なら以下:
   - `LLDrawPool::recordPoolDraws(VkCommandBuffer)` virtual method 追加 (`indra/newview/lldrawpool.h`)
   - `lldrawpool.cpp` base orchestrator dispatcher 内で `LLVKLoader::getCurrentCommandBuffer()` 経由で hook 呼出 (per-pool render path 通過時)
   - `lldrawpoolsky.cpp` / `lldrawpoolwaterexclusion.cpp` / `lldrawpoolpbropaque.cpp` / `lldrawpoolsimple.cpp` の 4 file で `recordPoolDraws` 空 override 追加 (LL_INFOS marker 推奨)
   - build → install → AYA 起動 verify (4 pool の marker log 出現 + Vulkan init/shutdown 正常)
   - commit + 進捗 update

---

## 3. deferred item 持ち越し (段階 2 内で消化)

### 3.1 validation strict 検証

- ReleaseFS_open build は `LL_RELEASE_FOR_DOWNLOAD` で validation 自動 disable (L82 `validation=disabled` 確認)
- 2.1a〜2.4 進行中は API-level success (VkCreate* / vkCmd* の error return 0) のみ checkpoint
- 02-portage-execution.md §4.1 #3 acceptance に明記: **validation strict 検証は sub-step 2.5 self-check で一時 force-enable build (`#ifndef LL_RELEASE_FOR_DOWNLOAD` 一時除去) で纏めて実施**
- 2.5 self-check 実施時の手順:
  1. `llvkloader.cpp` の `#ifndef LL_RELEASE_FOR_DOWNLOAD` ブロックを一時除去 (validation 強制 enable)
  2. autobuild build + install
  3. AYA 起動 → ログイン直前で落とす
  4. log grep `VALIDATION\|VK_DEBUG\|VK_LAYER` で error / warning 0 件確認
  5. `#ifndef` ブロック復元 + commit
  6. 段階 2 完遂 handoff doc 作成

### 3.2 Mesa RADV 動作確認 (`handoff-stage-1-complete.md` §3.1.2 継承)

- AYA 環境に AMD discrete GPU 不在 → Mesa RADV 動作未確認
- 段階 2-9 進行中は据置、段階 10 driver matrix polish で改めて testbed 確保

---

## 4. 次 session 開始 action cadence

### 4.1 次 session 開始時の最初のアクション

1. **handoff 確認**: 本 handoff (`handoff-substep-2-1a-complete.md`) を Read
2. **2.1b scope 確認**: AYA に §2 推奨 (1) hook wiring only で進めるか、再選択するか質問 1 件
3. **2.1b 着手**: scope 確定後、§2.4 手順で実装 → build → verify → commit
4. **2.1b 完遂後の cadence**:
   - 2.2 (標準 5 pool) は file 単位 independent → Agent 並列活用候補 (`feedback_use_agents_proactively.md` 反映)
   - 2.1b 完遂時に handoff doc 作成判断 (context 残量 / sub-step 境界 / `feedback_proactive_handoff.md` 遵守)

### 4.2 段階 2 着手中の注意事項 (再掲、`handoff-stage-1-complete.md` §4.3 継承)

- **Linux first-class baseline 厳守**: NVIDIA proprietary on RTX 5090 で動作確認、Mesa RADV は段階 10 polish
- **parity 不要**: charter §1 thesis 維持、AYAstorm 改変 13 file shader (picker 2 / Cinematic 4 / visual realism 7) は r42-α/β/γ で port
- **acceptance satisfy は実機検証**: `feedback_build_only_verified.md` 遵守、推論 ban
- **仮説 2 連続外れ rule**: `feedback_admit_unknown.md` 反映、bridging code trouble で log/canary/bisect 切替
- **sub-step 完遂時 self-trace + handoff**: `feedback_self_verify_before_handoff.md` + `feedback_proactive_handoff.md` 継承
- **commit / push cadence**: commit は AYA 指示後 Claude 実行、push は AYA 手動 (`feedback_release_flow.md` 遵守)

---

## 5. 関連 doc / memory

### 関連 doc (r41 章)

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter 本体 (closed 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/01-foundation.md` — sub-doc foundation (closed 2026-05-28、段階 1 完遂で役割完了)
- `docs/specs/ayastorm-r41-gl-removal/02-portage-execution.md` — sub-doc 段階 2 (closed 2026-05-28、段階 2 進行中の参照 active)
- `docs/specs/ayastorm-r41-gl-removal/handoff-r41-charter-complete.md` — r41 charter 完成 → 段階 1 着手前 prep
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-1-complete.md` — 段階 1 完遂 → 段階 2 着手前 prep
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-2-1a-complete.md` — **本 handoff** (sub-step 2.1a 完遂 → 2.1b 着手境界)

### 関連 commit (本 session で積まれた branch 上 commit)

- `0ce8ea8df5` — `fix(r41): handoff §3.1.1 loader version log mystery 解消 (LLError warmup 行追加)`
- `480952ddea` — `docs(r41): sub-doc 02 §3.1 sub-step 2.1 を 2.1a (基盤) + 2.1b (record 配線) に分割`
- `49b9f36427` — `feat(r41): sub-step 2.1a 完了 (Vulkan command 基盤 + minimal render pass + per-frame 空 record cycle)`

### 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone active
- `project_ayastorm_three_platforms.md` — 3 OS 大前提 + Linux 先行例外
- `feedback_self_verify_before_handoff.md` — sub-step 2.1a self-trace 本 handoff §1.1 反映
- `feedback_proactive_handoff.md` — sub-step 境界 handoff 本 doc で実施
- `feedback_no_auto_commit.md` — commit は AYA 指示後
- `feedback_release_flow.md` — push は AYA 手動
- `feedback_build_only_verified.md` — §1.1 達成 marker は実機検証 evidence
- `feedback_admit_unknown.md` — §3.1.1 loader version log mystery は本 session 解消 (LLError 1st-call swallow quirk)
- `feedback_self_bug_no_defer_option.md` — §2.2 (3) schedule swap 案を明示却下
- `feedback_use_agents_proactively.md` — §4.1 2.2 標準 5 pool independent port で Agent 並列活用候補
