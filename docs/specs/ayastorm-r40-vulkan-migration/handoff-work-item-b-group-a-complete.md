# handoff: work item (b) Vulkan API 設計 group A (§4 + §5) 完了 → group B (§6 + §7) + group C (§8 + §10) 着手前

**作成日**: 2026-05-28
**前 session 完了範囲**: foundation group (§1-§3 + §9) draft (`handoff-work-item-b-foundation-complete.md` 参照)
**本 session 完了範囲**: work item (b) group A (§4 render pass + §5 sync) draft 追加完成
**次 session 開始 task**: work item (b) group B (§6 memory allocator + §7 swapchain) + group C (§8 3 OS + §10 abstraction) draft 化

---

## 1. 完了済 範囲

### 1.1 commit log

| commit | SHA | 内容 |
|---|---|---|
| 1 (前 session) | `d1fe6c36ad` | sub-phase 3 spec docs + work item (a) Vulkan portage 棚卸し 完了 |
| 2 (前 session) | `a0f233da4d` | charter a-4 final 反映 |
| 3 (前 session) | `d5d064fa6f` | work item (b) foundation group (§1-§3 + §9) draft + handoff-foundation doc |
| 4 (本 session、後続) | (本 commit) | work item (b) group A (§4 + §5) draft 追加 + 03 doc status 更新 + 本 handoff doc |

すべて branch `feature/ayastorm-r40-vulkan-migration` に積み済 (push は AYA 側で実行)。

### 1.2 出力 doc

- `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` — §1-§5 + §9 draft 完成 (約 760 行)、§6-§8 + §10 placeholder
- `docs/specs/ayastorm-r40-vulkan-migration/03-sub-phase-3-vulkan-plan.md` — work item (b) status「group A 完了で §1-§5 + §9 完成、§6-§8 + §10 残」更新済

### 1.3 memory 更新

本 session は memory 更新なし (work item (b) draft 進行中状態を継承、project_ayastorm_r40_cpu_parallel.md は前 session で更新済)。

---

## 2. 本 session group A の確定事項 (§6-§10 の input)

### 2.1 §4 render pass / framebuffer

- **dynamic rendering 採用** (Vulkan 1.3 core): `VkRenderPass` + `VkFramebuffer` 明示作成廃止、`vkCmdBeginRenderingKHR` で attachment 直接指定
- **deferred g-buffer 構成**: gbuffer0 (diffuse) / gbuffer1 (normal) / gbuffer2 (specular+AO) / **gbuffer3 (R8G8B8A8 = r21.1 picker LocalID 流用)** / depth (D24S8)
- **7 pass chain**: shadow → g-buffer+picker (pass 2) → deferred lighting → forward alpha → sky → post-process → UI
- **r14+ post-process sub-chain (pass 6)**: godrays → volumetricLight → blurLight → atmospherics → vignette → DoF (r30) → tonemap の 7 sub-pass、ping-pong A/B 2 attachment
- **r21.1 picker 統合**: pass 2 内 gbuffer3 inline attachment (a-4 §6.4.2 (5) 確定方針)、pass 終了後 `vkCmdCopyImageToBuffer` で staging buffer copy、CPU readback は 3 frame 遅延 (許容)
- **sky pass (pass 5)**: lldrawpoolsky 57 + lldrawpoolwlsky 521 + llvosky/llvowlsky 2,198 LOC、forward 描画 / depth READ_ONLY
- **LLRenderTarget interface 残置**: 35 GL call の Vulkan 等価マップ確定、`bindTarget()` / `flush()` / `getTexture()` interface 維持で caller 188 file 変更不要

### 2.2 §5 sync 戦略

- **frame in flight = 3** 採用 (§3.6 descriptor pool per-frame × 3 と align、industry default)
- **sync primitive 使い分け**: VkFence (CPU↔GPU)、binary VkSemaphore (swapchain acquire / present)、timeline VkSemaphore (worker thread upload 同期)
- **image layout transition 表**: swapchain (UNDEFINED → COLOR_ATTACHMENT → PRESENT_SRC) / g-buffer (UNDEFINED → COLOR_ATTACHMENT → SHADER_READ / gbuffer3 → TRANSFER_SRC) / depth / shadow map / post-process intermediate ping-pong
- **synchronization2 access mask 細分化**: `VK_ACCESS_2_*` + `VK_PIPELINE_STAGE_2_*` で over-barrier 回避、複数 attachment transition は `vkCmdPipelineBarrier2` で batching
- **worker thread upload**: llimagegl.cpp / llspatialpartition.cpp `rebuildMesh()` → staging buffer (VMA HOST_VISIBLE) → transfer queue、timeline semaphore (`uploadTimeline`) で main thread と同期
- **per-frame sequence**: vkWaitForFences → vkAcquireNextImageKHR → 7 pass record + barrier → vkQueueSubmit2 (wait acquire + upload timeline, signal renderFinished + fence) → vkQueuePresentKHR

---

## 3. 次 session で draft 化する section (group B + group C)

05 doc 内に各 section の **sub list (draft 予定の項目)** を placeholder として残置済。以下は次 session 着手時の早見表。

### group B (§6 memory allocator + §7 swapchain)

#### §6 memory allocator (VMA)

- §6.1 VMA (GPUOpen) 採用根拠 (a-4 §6.4.1 で必須採用確定)
- §6.2 memory type 分類 (DEVICE_LOCAL / HOST_VISIBLE / HOST_COHERENT / HOST_CACHED / DEVICE_LOCAL+HOST_VISIBLE)
- §6.3 staging buffer 戦略 (llimagegl.cpp 移行、a-4 §1.1)
- §6.4 vertex / index / UBO / texture の typical allocation pattern
- §6.5 defragmentation 採用判断 (long-lived session 対策)
- §6.6 `VK_EXT_memory_budget` 経由の VRAM 監視 (§9.3 で enable 済)

#### §7 swapchain / present mode

- §7.1 present mode (FIFO default / mailbox optional for VSync OFF / immediate 不採用)
- §7.2 swapchain image 数 (3 image、§5.1 frame in flight = 3 と align)
- §7.3 surface format (sRGB / linear、HDR 検討、`VK_KHR_swapchain_mutable_format` 経由)
- §7.4 resize / minimize / fullscreen 切替時の re-create
- §7.5 multi-monitor / DPI scaling
- §7.6 `VK_EXT_swapchain_maintenance1` 採用検討 (§9.3 で enable 済)

### group C (§8 3 OS + §10 abstraction)

#### §8 3 OS 対応詳細

- §8.1 Linux driver capability matrix (Mesa RADV / ANV / NVIDIA / AMDGPU-PRO)
- §8.2 Windows ICD 仕様の差分 (NVIDIA / AMD / Intel)
- §8.3 Mac MoltenVK portability subset 詳細 (§9.4 と整合、本 §8 は方針のみ詳細化、深堀は vk-RC 直前 phase)
- §8.4 WSI (xcb / wayland / win32 / metal)
- §8.5 「Linux 先行 + Mac 互換性 maintain」方針 (charter §4 (2) の Vulkan 設計反映)

#### §10 abstraction interface (r41.5 分離 skeleton)

- §10.1 r41.5 milestone の AYAstorm VK repo 分離前提 (charter §6 / §4 (4) Phase 2)
- §10.2 interface skeleton (header のみ公開、本線 ↔ VK repo の API surface)
- §10.3 dynamic link 構成 (本線 LGPL ↔ VK repo 独自 license の合法的境界)
- §10.4 LL UI 変更時 defensibility (charter §7 判断軸 3 (iv) 選択肢の有効化条件)
- §10.5 詳細 interface は r41.5 charter で確定する旨

---

## 4. 次 session 開始時の最初の task list

### task 1: handoff doc + 05 doc + 03 doc の state 確認

1. 本 handoff doc を Read
2. `05-vulkan-api-design.md` §1-§5 + §9 完成範囲を Read (整合参照用)
3. `03-sub-phase-3-vulkan-plan.md` §2 status table 確認 (work item (b) が「group A 完了で §1-§5 + §9 完成」のまま)

### task 2: group B (§6 + §7) draft 化

§6 memory allocator + §7 swapchain を 05 doc に Edit/拡張。

- §6 は VMA 採用 + memory type 分配 + staging buffer 戦略の order で draft 推奨
- §7 は present mode → image 数 → format → re-create の order で draft 推奨 (依存順)

### task 3: group C (§8 + §10) draft 化

§8 3 OS + §10 abstraction skeleton を 05 doc に Edit/拡張。

- §8 は方針詳細化のみ (深堀は vk-RC 直前 phase へ後送り)
- §10 は r41.5 charter (= 本 doc とは別、r41 達成後に起草) で確定する前提で skeleton のみ

context 量との見合いで 1 session で group B + group C 完遂可能の見込み、必要なら group 境界で再度 handoff。

### task 4: 全 10 section 揃ったら work item (b) 完了宣言

- 03 doc §2 status を「draft 作成中」→「**完了**」に更新
- work item (c) 工程算定 (`06-effort-estimation.md`) 着手前 handoff

---

## 5. 設計 input refs (group A で確定した内容を group B/C が参照)

| group B/C で詰める設計 | group A での確定 input |
|---|---|
| §6 VMA staging buffer | §5.5 worker thread upload で staging buffer (VMA HOST_VISIBLE) → transfer queue が確定、§6.3 で staging buffer 詳細 (lifecycle / sizing) を詰める |
| §6 memory type 分配 | §4.2 g-buffer attachment (DEVICE_LOCAL) / §5.5 staging buffer (HOST_VISIBLE) / §4.4 ping-pong intermediate (DEVICE_LOCAL) で type 分布が確定済、§6.2 で具体 type 列挙 |
| §6.6 budget API | §9.3 `VK_EXT_memory_budget` enable 済、§6.6 で VRAM 監視 API 利用パターン |
| §7.2 image 数 | §5.1 frame in flight = 3 + §5.6 vkAcquireNextImageKHR sequence と align、image 数 = 3 確定方向 |
| §7.4 re-create | §5.6 swapchain barrier sequence + §4.7 LLRenderTarget allocate を re-create 時に発火、interface 残置で caller 影響最小 |
| §8 driver matrix | §9.1 / §9.2 / §9.3 採用 extension の driver 対応状況を OS 別に整理、Linux driver は §1.1 driver coverage 表を詳細化 |
| §10 interface skeleton | §4.7 LLRenderTarget interface 残置パターン + §5.2 sync primitive 集約パターンを skeleton 設計の参考、§10 で API surface 化 |

---

## 6. 内部状態の特記事項 / AYA との合意事項 (本 session 内)

### 6.1 group A 進行の合意 (本 session)

work item (b) draft 進め方で **group A (§4 + §5) 着手 OK** を AYA 判断。foundation group との依存 (§4.1 dynamic rendering は §1.1 / §9.1 enable 必須、§5 sync2/timeline は §1.1 / §9.1 enable 必須、§4.5 picker は §3.4 配置確定、§5.1 frame in flight = §3.6 pool sizing と align) を §4 / §5 各所で参照確定。

### 6.2 group A → handoff 推奨判断 (本 session)

group A draft 完成後の続行可否で「推奨で」と AYA 判断委譲、Claude 側で **commit + handoff → 次 session で group B + group C** を推奨。根拠:
- foundation + group A で合計 ~760 行 draft、context 圧迫前の周回境界 (memory `feedback_proactive_handoff`)
- group B / group C は別 group として handoff 境界が自然 (foundation handoff doc §3 で既に group A/B/C 分けを明示)
- 1 session で 3 group 全部やると quality が落ちる risk

### 6.3 self-verify 実施 (本 session 内)

`feedback_self_verify_before_handoff` に従い group A draft 完成時に self-trace 実施、確認内容:

- §4.1 dynamic rendering ↔ §1.1 / §9.1 1.3 core enable 必須整合
- §4.5 picker gbuffer3 inline ↔ §3.4 picker (set=0 per-frame) + a-4 §6.4.2 (5) 確定方針整合
- §5.1 frame in flight = 3 ↔ §3.6 descriptor pool per-frame × 3 整合
- §5.4 sync2 / §5.5 timeline / BDA ↔ §1.1 / §9.1 必須 enable 整合
- AYAstorm 改変 13 file (r21.1 picker 2 + r30 Cinematic 4 + r14+ visual realism 7) を §4.4 / §4.5 に完全 mapping
- llvosky/llvowlsky 2,198 LOC → §4.6 / LLRenderTarget 35 calls → §4.7 移行マップ整合

→ group B/C で group A 数字を再参照する際は本 self-trace 結果を信頼してよい。

### 6.4 push は AYA 側

本 session 開始時に AYA 明示指示で前 session までの 3 commit を push 済 (origin `feature/ayastorm-r40-vulkan-migration` 新規 branch 作成)。本 commit 4 以降は通常通り AYA 側 push 待ち (`feedback_release_flow`)。

---

## 7. 関連 doc / memory

### r40 章内部 doc

- `00-charter.md` — r40 章 charter (a-4 final 反映済)
- `01-sub-phase-1-cpu-perf.md` — sub-phase 1 詳細 (CPU perf 全 REJECT)
- `02-sub-phase-2-extended-falsify.md` — sub-phase 2 詳細 (鉱脈ゼロ)
- `03-sub-phase-3-vulkan-plan.md` — work item (b) 親 doc (§2 status table で (b) draft 作成中)
- `04-portage-inventory.md` — work item (a) 全完了 (§6.4 が本 (b) の設計 input source)
- `05-vulkan-api-design.md` — work item (b) draft (§1-§5 + §9 完成、§6-§8 + §10 placeholder)
- `handoff-work-item-a-complete.md` — work item (a) → (b) 移行 handoff
- `handoff-work-item-b-foundation-complete.md` — work item (b) foundation group 完了 handoff (前 session)

### memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory (a-4 final 同期 + work item (a) 完了記録済)
- `project_ayastorm_r40_extended.md` — sub-phase 2 詳細
- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 (b)(c)(d)(e) 完了後に着手)
- `feedback_proactive_handoff.md` — group A / 次 session 境界 handoff の根拠
- `feedback_self_verify_before_handoff.md` — group A self-trace 整合確認の根拠
- `feedback_no_scope_shrink.md` — 「scope shrink せず group 単位で密に draft する」判断根拠

---

## 8. 次 session キックオフ template (AYA → Claude)

次 session 開始時、AYA は以下のような開始メッセージを投げると Claude が context 即把握できる:

```
r40 sub-phase 3 work item (b) group A (§4 + §5) draft 完了で前回 handoff した。
handoff doc 確認して、group B (§6 + §7) + group C (§8 + §10) draft に進む準備をして。
進め方の推奨があれば提示してほしい。
```

Claude 側は:

1. 本 handoff doc を Read
2. 05 doc §1-§5 + §9 完成範囲を Read
3. 03 doc §2 status table 確認
4. group B 着手 or 1 session で B+C 完遂可否を提示
5. AYA judgment 後、選択された path を実行 → §6 着手
