# handoff: work item (b) Vulkan API 設計 全 10 section draft 完成 → work item (c) 工程算定 着手前

**作成日**: 2026-05-28
**前 session 完了範囲**: foundation group (§1-§3 + §9) + group A (§4 + §5) draft (`handoff-work-item-b-foundation-complete.md` / `handoff-work-item-b-group-a-complete.md` 参照)
**本 session 完了範囲**: work item (b) group B (§6 memory allocator + §7 swapchain) + group C (§8 3 OS + §10 abstraction) draft 追加完成 → **全 10 section 揃った**
**次 session 開始 task**: work item (b) AYA review 完了後 → work item (c) 工程算定 (`06-effort-estimation.md`) 着手

---

## 1. 完了済 範囲

### 1.1 commit log

| commit | SHA | 内容 |
|---|---|---|
| 1 (前々 session) | `d1fe6c36ad` | sub-phase 3 spec docs + work item (a) Vulkan portage 棚卸し 完了 |
| 2 (前々 session) | `a0f233da4d` | charter a-4 final 反映 |
| 3 (前々 session) | `d5d064fa6f` | work item (b) foundation group (§1-§3 + §9) draft + handoff-foundation doc |
| 4 (前 session) | `656f4ca754` | work item (b) group A (§4 + §5) draft + handoff-group-a doc |
| 5 (本 session、後続) | (本 commit) | work item (b) group B (§6 + §7) + group C (§8 + §10) draft + 03/05 doc status 更新 + 本 handoff doc |

branch `feature/ayastorm-r40-vulkan-migration` を origin に push 済 (本 session 冒頭 AYA 指示で commit 4 まで push、本 commit 5 は通常通り AYA 側 push 待ち `feedback_release_flow`)。

### 1.2 出力 doc

- `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` — **全 §1-§10 draft 完成 (1540 行)**、AYA review 待ち
- `docs/specs/ayastorm-r40-vulkan-migration/03-sub-phase-3-vulkan-plan.md` — work item (b) status「draft 全 10 section 完成、AYA review 待ち」更新済

### 1.3 memory 更新

本 session は memory 更新なし (work item (b) draft 進行中状態を継承、AYA review PASS → work item (c) 着手の段階で `project_ayastorm_r40_cpu_parallel.md` を更新予定)。

---

## 2. 本 session group B + C で確定した設計事項

### 2.1 §6 memory allocator (VMA)

- **VMA 全 allocation 採用** (`vkAllocateMemory` 直接呼出ゼロ): `maxMemoryAllocationCount` 制約 + memory type 自動選定 + alignment 整合 + defragmentation 標準提供 + budget API 統合 + 大規模 project 実績の 6 理由
- **memory type 5 分類**: DEVICE_LOCAL / HOST_VISIBLE+COHERENT / HOST_VISIBLE+CACHED / ReBAR / LAZILY_ALLOCATED と VMA usage hint mapping 確定
- **per-thread staging pool 3 種別**: mainThread (~16MB) / uploadThread (~256MB、ring allocator) / readback (~4MB)、§5.5 worker thread upload と統合
- **typical allocation pattern**: vertex/index/UBO/texture/attachment 各 5 種別の VMA template 確定
- **interactive defragmentation**: 1 frame に 16MB / 32 allocation 上限、stop-the-world 禁止、r42-α 以降 implement (r41 は API surface 予約のみ)
- **VRAM 監視**: `VK_EXT_memory_budget` 経由 `vmaGetHeapBudgets`、1 秒に 1 回 query、90%/95% threshold action は r42+ 検討

### 2.2 §7 swapchain / present mode

- **present mode**: FIFO_KHR (default) / FIFO_RELAXED_KHR (adaptive) / MAILBOX_KHR (low-latency) を cvar (VSyncMode 等) で mapping、IMMEDIATE_KHR は撮影章本旨に反するため不採用
- **swapchain image 数 = 3**: §5.1 frame in flight = 3 と align、`max(3, minImageCount)` で driver 制約吸収
- **surface format**: `VK_FORMAT_B8G8R8A8_SRGB` + `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR` を default、HDR (HDR10 / scRGB) は r45+ visual realism 次世代 milestone で予約
- **re-create sequence**: window resize / minimize / fullscreen 切替 / monitor 切替 / `OUT_OF_DATE_KHR` 検出時に sequence 9 step (vkDeviceWaitIdle → ImageView destroy → swapchain destroy → cap query → swapchain create → ImageView create → attachment re-create → descriptor set re-write → command buffer reset)
- **multi-monitor / DPI**: swapchain extent は物理 pixel、scale 計算は LLWindow/LLViewerWindow 側責務 (現 GL 役割分担維持)
- **`VK_EXT_swapchain_maintenance1`**: `PresentFenceInfoEXT` で `vkDeviceWaitIdle` → per-image fence wait 高速化、resize frame drop 3→1 short、Mac MoltenVK 未対応で fallback path 確定

### 2.3 §8 3 OS 対応詳細

- **Linux driver matrix**: Mesa RADV (first-class、AYAstorm 開発機) / Mesa ANV (first-class、Intel) / NVIDIA proprietary (first-class) / AMDGPU-PRO (second-class) / LLVMpipe (non-goal)、Vulkan 1.3 minimum の妥当性根拠 (Mesa 22.x / NVIDIA 525+ / Ubuntu 22.04 LTS)
- **Windows ICD**: NVIDIA GeForce/Quadro / AMD Radeon Software / Intel ARC/Iris Xe の Vulkan 1.3 対応 + WSI WGL 不要化
- **Mac MoltenVK**: Vulkan 1.2 core + 1.3 portable subset、利用不可/制約機能表 (geometry shader / tessellation / D24S8 / transfer queue / swapchain_maintenance1 / ray tracing)、Apple Silicon UMA + ProMotion 対応、macOS 14+ Metal 3 minimum、t-noami さん移植 workflow と整合
- **WSI**: `VK_KHR_xcb_surface` (X11) / `VK_KHR_wayland_surface` / `VK_KHR_win32_surface` / `VK_EXT_metal_surface` (Mac MVK_macos_surface legacy 不採用)
- **Linux 先行 + Mac maintain**: charter §4 (2) を本 §1.1 / §4.1 / §5.5 / §7.3 / §7.6 / §9.5 各所で具体反映、Mac は graceful degradation で互換性維持、r42-α=Win 追加 / r42-β-γ-δ=Mac 追加の段階対応

### 2.4 §10 abstraction interface (r41.5 分離 skeleton)

- **milestone 関係**: r41 (本線同居) → r41.5 (VK repo 分離) → r42-α 以降 (描画機能 port)、本 §10 skeleton は r41 で本線 `indra/llrender/vk/` 配下に implement、r41.5 で repo 切出
- **interface skeleton 2 層**: C++ pure virtual interface (`ILLVKRenderer`) + C ABI entry point (`LLVKRenderer_create`)、handle 型 opaque pointer (`LLVKBufferHandle` 等)、本線 header 内 Vulkan header 非露出 (`VkFormat` 等は `LLVKFormat` shadow enum)
- **dynamic link 構成**: 本線 LGPL ↔ VK repo 独自 license を dynamic link 境界で合法分離 (LGPL core 主張点準拠)、`dlopen` / `LoadLibrary` 経由、`extern "C"` symbol 1 個公開
- **defensibility**: LL 上流 UI 大幅変更時に AYAstorm VK repo を別 viewer base に接続切替可能 (interface skeleton 前提)、r41.5 charter 詳細
- **r41.5 charter 預け事項**: API 全 method list / semver / thread safety 細部 / wrapper 設計 / license 確定 / test harness / package 配布形式

---

## 3. 設計 input refs (group B+C 含む全 §1-§10 cross reference)

### 3.1 self-trace 結果 (本 session 完了時)

`feedback_self_verify_before_handoff` に従い group B+C draft 完成時に self-trace 実施、確認内容:

- §6.2 VMA usage hint mapping ↔ §5.5 staging buffer + §4.2 attachment + §3.6 descriptor pool 整合
- §6.5 defragmentation r42-α 以降 ↔ §10.1 milestone phase 整合
- §6.6 `VK_EXT_memory_budget` 利用 ↔ §9.3 enable 整合
- §7.2 swapchain image = 3 ↔ §5.1 frame in flight = 3 ↔ §3.6 descriptor pool size 整合
- §7.6 `VK_EXT_swapchain_maintenance1` 利用 ↔ §9.3 enable + §8.3 Mac fallback 整合
- §8.1 Linux driver matrix ↔ §1.1 driver coverage 詳細化整合
- §8.3 Mac MoltenVK 制約表 ↔ §9.4 portable subset + §5.5 transfer queue + §7.6 maintenance1 + §4.2 D24S8 整合
- §8.4 WSI 4 surface extension ↔ §9.2 KHR/EXT surface list 整合
- §8.5 Linux 先行 + Mac maintain ↔ §1.1 / §4.1 / §5.5 / §7.3 / §7.6 / §9.5 / §9.4 全部整合 + charter §4 (2) 反映
- §10.1 milestone 関係 ↔ charter §6 / §4 (4) Phase 2 反映
- §10.2 interface skeleton ↔ §3 descriptor + §4 pass + §5 sync + §6 memory + §7 swapchain の API surface 統合
- §10.3 dynamic link 境界 ↔ Vulkan SDK Apache 2.0 + VMA MIT + volk MIT + LGPL 本線の license stack 整合

→ work item (c) 工程算定で本 self-trace 結果を信頼して per-section 工数算定可。

### 3.2 group A → group B+C で確定した内容の cross-reference (本 session の追加分のみ)

| 確定事項 | 本 session 追加根拠 |
|---|---|
| VMA per-thread pool 3 種別 | §5.5 worker thread upload 経路 + §4.5 picker readback 用途で thread 境界が明確化、§6.3 で 3 pool 確定 |
| swapchain image = 3 | §5.1 frame in flight = 3 と align、§7.2 で driver 制約 (`minImageCount`) 込み確定 |
| Mac MoltenVK depth format | §4.2 で D24S8 仮置き、§8.3 で D32_SFLOAT_S8_UINT 内部置換確定 (vk-RC で driver query 後最終確定) |
| Mac MoltenVK transfer queue | §5.5 fallback 言及、§8.3 で unified queue = graphics queue 兼用確定 |
| swapchain re-create per-image fence | §5.6 sequence で言及、§7.6 で `swapchain_maintenance1` enable 駆動確定 |
| interface skeleton C++ + C ABI 2 層 | §10.2 で全 API surface 統合 (§3/§4/§5/§6/§7 概念合成) |

---

## 4. 次 session で進める task (AYA review → work item (c))

### task 1: AYA review 反映 (もし修正指示あれば)

- 05 doc 全 §1-§10 を AYA review、修正指示があれば該当 section 反映
- review PASS で work item (b) 完了宣言

### task 2: work item (b) 完了宣言 + status 更新

- 03 doc §2 status table を「draft 全 10 section 完成、AYA review 待ち」→ 「**完了**」に更新
- memory `project_ayastorm_r40_cpu_parallel.md` で work item (b) 完了を反映 (work item (c) 着手記録併せて)

### task 3: work item (c) 工程算定 着手

- 03 doc §5 work item (c) 通り、`06-effort-estimation.md` を新規作成
- 03 doc §5 算定軸 8 項目 (per-file 工数 / per-shader 工数 / per-milestone 工数積算 / 3 OS per-OS 増分 / 各 milestone 月数 / uncertainty band / Doom Blender 参照点比較 / plan B trigger 条件) に沿って draft 作成
- 入力 source: 04 doc 棚卸し + 05 doc 全 10 section + charter §4 (3) 参照点

### task 4: 工程算定 完了 → work item (d) → (e)

- work item (c) 完了 → work item (d) charter §6 仮 line up を本 line up に格上げ → work item (e) charter 完成 → **r40 達成宣言**

---

## 5. 関連 doc / memory

### r40 章内部 doc

- `00-charter.md` — r40 章 charter (a-4 final 反映済、本 (b) 完了で §6 仮 line up 更新は work item (d) で実施)
- `01-sub-phase-1-cpu-perf.md` — sub-phase 1 詳細 (CPU perf 全 REJECT)
- `02-sub-phase-2-extended-falsify.md` — sub-phase 2 詳細 (鉱脈ゼロ)
- `03-sub-phase-3-vulkan-plan.md` — work item (a)-(e) 親 doc (§2 status table で (b) draft 全 10 section 完成 AYA review 待ち)
- `04-portage-inventory.md` — work item (a) 全完了 (§6.4 設計 input source)
- `05-vulkan-api-design.md` — work item (b) draft **全 10 section 完成** (1540 行、AYA review 待ち)
- `06-effort-estimation.md` — work item (c) 出力 (次 session で起草予定)
- `handoff-work-item-a-complete.md` — work item (a) → (b) 移行 handoff
- `handoff-work-item-b-foundation-complete.md` — work item (b) foundation group 完了 handoff (前々 session)
- `handoff-work-item-b-group-a-complete.md` — work item (b) group A 完了 handoff (前 session)
- `handoff-work-item-b-complete.md` — 本 handoff doc

### memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory (work item (b) 完了 + (c) 着手で次 session 更新予定)
- `project_ayastorm_r40_extended.md` — sub-phase 2 詳細
- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 (b)(c)(d)(e) 完了後に着手)
- `feedback_proactive_handoff.md` — group B+C / 次 session 境界 handoff の根拠
- `feedback_self_verify_before_handoff.md` — group B+C self-trace 整合確認の根拠
- `feedback_no_scope_shrink.md` — 「scope shrink せず B+C 1 session で密に draft する」判断根拠 (AYA 「OK」承認後実行)

---

## 6. 内部状態の特記事項 / AYA との合意事項 (本 session 内)

### 6.1 group B + C 1 session 完遂の合意

本 session 開始時に Claude から「group B + C 1 session 完遂を推奨 (各 ~300 行、合計 ~600 行、foundation + group A の 760 行と同 scale)、context 余裕逼迫したら group 境界で再 handoff」を提示、AYA 「OK」承認で実行。

実際の draft 行数: group B = ~430 行 (§6=~240 + §7=~190)、group C = ~340 行 (§8=~190 + §10=~150)、合計 770 行追加、foundation + group A 760 行と同 scale 内に収まった。context 圧迫無く完遂、再 handoff 不要。

### 6.2 push 状況

- 本 session 開始時 (`feature/ayastorm-r40-vulkan-migration` upstream 未設定) に AYA 「最初に push して」指示で commit 4 (656f4ca754) まで push 済 (`set-upstream` 設定込み)
- 本 commit 5 以降は通常通り AYA 側 push 待ち (`feedback_release_flow`)

### 6.3 self-verify 実施

`feedback_self_verify_before_handoff` に従い本 handoff doc §3.1 で self-trace を 12 項目実施、cross-reference 全部整合確認済。AYA review で確認お願いする際の負担を軽減。

### 6.4 「draft 進行状況の整理」section の更新

05 doc 末尾の「draft 進行状況の整理」section が group A handoff 時点の「§6-§10 placeholder」記述のままだったので、本 session で「全 10 section 完成」+「§6-§10 確定要旨」+「次 step (AYA review → work item (b) 完了宣言 → work item (c) 着手)」に更新済。

---

## 7. 次 session キックオフ template (AYA → Claude)

次 session 開始時、AYA は以下のような開始メッセージを投げると Claude が context 即把握できる:

### pattern A: AYA review が PASS の場合

```
r40 sub-phase 3 work item (b) 全 10 section draft 完成で前回 handoff した。
05 doc review した、修正指示無し、PASS。work item (b) 完了宣言して、(c) 工程算定に進んで。
```

Claude 側は:
1. 本 handoff doc を Read
2. 03 doc §2 status を「完了」に更新
3. memory `project_ayastorm_r40_cpu_parallel.md` を work item (b) 完了 + (c) 着手で更新
4. 06 doc (`06-effort-estimation.md`) 新規作成、03 doc §5 算定軸 8 項目に沿って draft 開始

### pattern B: AYA review で修正指示がある場合

```
r40 sub-phase 3 work item (b) 全 10 section draft、こことここを修正してほしい (修正点指示).
```

Claude 側は:
1. 本 handoff doc を Read (context 再把握)
2. 修正指示反映、05 doc の該当 section を Edit
3. 修正 commit → AYA 再 review
