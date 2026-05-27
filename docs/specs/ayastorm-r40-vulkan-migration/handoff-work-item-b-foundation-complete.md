# handoff: work item (b) Vulkan API 設計 foundation group §1-§3 + §9 完了 → §4-§8 + §10 着手前

**作成日**: 2026-05-28
**前 session 完了範囲**: work item (b) foundation group §1 / §2 / §3 / §9 draft 完成 + work item (a) → (b) 移行 (charter a-4 反映含む)
**次 session 開始 task**: work item (b) 後半 §4 / §5 / §6 / §7 / §8 / §10 draft 化

---

## 1. 完了済 範囲

### 1.1 commit log (本 session 内)

| commit | SHA | 内容 |
|---|---|---|
| 1 | `d1fe6c36ad` | sub-phase 3 spec docs + work item (a) Vulkan portage 棚卸し 完了 (00 charter + 01/02 sub-phase 1/2 + 03 sub-phase 3 plan + 04 portage inventory + handoff-a) |
| 2 | `a0f233da4d` | charter a-4 final 反映 (portage 真の規模 63K + wrapper 局在化追い風、§4 (2)/(3) + §5 + §6 末尾) |
| 3 | (本 commit、後続) | work item (b) Vulkan API 設計 foundation group draft (§1-§3 + §9) + 03 doc status 更新 + 本 handoff doc |

すべて branch `feature/ayastorm-r40-vulkan-migration` に積み済 (push は AYA 側で実行)。

### 1.2 出力 doc

- `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` — §1-§3 + §9 draft + §4-§8 + §10 placeholder (約 360 行)
- `docs/specs/ayastorm-r40-vulkan-migration/03-sub-phase-3-vulkan-plan.md` — work item (b) status「着手前」→「draft 作成中 (foundation group §1-§3 + §9 進行中)」更新済
- `docs/specs/ayastorm-r40-vulkan-migration/00-charter.md` — a-4 final 反映済 (commit 2)

### 1.3 memory 更新

- `project_ayastorm_r40_cpu_parallel.md` — sub-phase 3 工程プラン (2) scope を a-4 final 値に同期、`### work item (a) Vulkan portage 棚卸し 完了` section を新規追加 (a-4 新発見 6 点記録)

---

## 2. 本 session foundation group の確定事項 (§4-§10 の input)

### 2.1 §1 Vulkan version + loader + SDK

- Linux/Win: **Vulkan 1.3 default**、Mac: 1.2 core + 一部 1.3 portable subset (MoltenVK 経由)
- loader: **volk** (header-only / extension 自動 load / Doom Eternal・RPCS3 等 large project 実績)
- SDK: **LunarG SDK 1.3.x**、3 OS autobuild package 化
- 段階 1 (GL header wrapper 置換): `llglheaders.h` + `llglstates.h` + `llgltypes.h` 3 file 置換で **188 file の上流 file は変更不要** (a-1 §1.2 確定の wrapper 局在化追い風)

### 2.2 §2 shader cross compile chain

- chain: **GLSL → glslang → SPIR-V** (build time pre-compile)、runtime は SPIR-V binary のみ load
- 248 file の compute/geometry/tessellation/bindless/atomic ゼロ → **~85% 素直に通る見込み**、残 15% は y 軸反転 / legacy GLSL 等
- AYAstorm 改変 13 file (r21.1 picker 2 + r30 Cinematic 4 + r14+ visual realism 7) は独立 namespace で SPIR-V 化、r42-α/β/γ で patch 合成
- cmake target `aya_shader_compile` 新規、descriptor set reflection は build time json 化 (§3 pool sizing の input)

### 2.3 §3 descriptor set / pipeline layout

- descriptor set = **3 構成** (set=0 per-frame / set=1 per-material / set=2 per-draw)
- sampler 206 個分布見積 = **30 (per-frame) + 70 (per-material) + 106 (per-draw)**
- per-draw set は **VK_KHR_push_descriptor 採用** (pool 確保不要、command buffer に inline 書込み)
- r21.1 picker buffer は **set=0 per-frame に配置** (deferred main pass inline attachment、a-4 §6.4.2 (5) 確定方針)
- pipeline layout: 3 set + push constant range (**model matrix mat4 64 bytes**)
- descriptor pool sizing 戦略は SPIR-V reflection 後に精緻化

### 2.4 §9 extension 採用 list

- Vulkan 1.3 core feature 6 件: dynamic rendering / synchronization2 / push descriptor (1.3 では KHR 必要) / inline UBO / timeline semaphore / buffer device address
- KHR 必須 5 件: `VK_KHR_swapchain` / `VK_KHR_surface` / WSI × 3OS (xcb+wayland / win32 / metal) / `VK_KHR_push_descriptor` / `VK_KHR_swapchain_mutable_format`
- EXT 必須 5 件: `VK_EXT_debug_utils` / `VK_EXT_calibrated_timestamps` / `VK_EXT_memory_budget` / `VK_EXT_swapchain_maintenance1` / `VK_EXT_vertex_input_dynamic_state`
- future 予約 3 件: ray tracing (KHR) / video_decode (KHR) / mesh_shader (EXT) — r45+ 検討

---

## 3. 次 session で draft 化する section (§4-§8 + §10)

05 doc 内に各 section の **sub list (draft 予定の項目)** を placeholder として残置済。以下は次 session 着手時の早見表。

### §4 render pass / framebuffer (deferred g-buffer の Vulkan 表現)

- §4.1 deferred g-buffer の Vulkan representation (VkRenderPass + VkFramebuffer vs Vulkan 1.3 dynamic rendering 採用判断)
- §4.2 attachment 配置 (color × N + depth/stencil)、subpass 構成
- §4.3 dynamic rendering 採用判断 (subpass 簡素化 vs subpass dependency tooling 喪失)
- §4.4 r14+ post-process pass chain 統合 (godrays / volumetricLight / blurLight / vignette + r30 DoF)
- §4.5 r21.1 picker attachment 統合 (§3.4 を pass 側から記述)
- §4.6 windlight / atmospherics の sky pass 表現
- §4.7 `LLRenderTarget` → Vulkan attachment 移行マップ (a-4 §1.1 LLRenderTarget 35 GL calls 対応)

### §5 sync 戦略 (state diagram / barrier table)

- §5.1 frame in flight = 3 採用根拠
- §5.2 fence / binary semaphore / timeline semaphore の使い分け
- §5.3 image layout transition 表 (color attachment / depth / sampled / present)
- §5.4 synchronization2 access mask 細分化方針
- §5.5 worker thread (texture upload / mesh upload) → main thread の async 同期
- §5.6 swapchain image acquisition → render → present の barrier sequence

### §6 memory allocator 方針 (VMA)

- §6.1 VMA (GPUOpen) 採用根拠
- §6.2 memory type 分類 (DEVICE_LOCAL / HOST_VISIBLE / HOST_COHERENT / HOST_CACHED / DEVICE_LOCAL+HOST_VISIBLE)
- §6.3 staging buffer 戦略 (llimagegl.cpp 移行、a-4 §1.1)
- §6.4 vertex / index / UBO / texture の typical allocation pattern
- §6.5 defragmentation 採用判断 (long-lived session 対策)
- §6.6 `VK_EXT_memory_budget` 経由の VRAM 監視

### §7 swapchain / present mode

- §7.1 present mode (FIFO default / mailbox optional for VSync OFF / immediate 不採用)
- §7.2 swapchain image 数 (3 image、frame in flight と align)
- §7.3 surface format (sRGB / linear、HDR 検討)
- §7.4 resize / minimize / fullscreen 切替時の re-create
- §7.5 multi-monitor / DPI scaling
- §7.6 `VK_EXT_swapchain_maintenance1` 採用検討

### §8 3 OS 対応詳細 (Mac MoltenVK 制約含む)

- §8.1 Linux driver capability matrix (Mesa RADV / ANV / NVIDIA / AMDGPU-PRO)
- §8.2 Windows ICD 仕様の差分 (NVIDIA / AMD / Intel)
- §8.3 Mac MoltenVK portability subset 詳細 (§9.4 と整合)
- §8.4 WSI (xcb / wayland / win32 / metal)
- §8.5 「Linux 先行 + Mac 互換性 maintain」方針 (charter §4 (2) の Vulkan 設計反映)

### §10 abstraction interface 設計 (r41.5 分離 skeleton のみ)

- §10.1 r41.5 milestone の AYAstorm VK repo 分離前提 (charter §6 / §4 (4) Phase 2)
- §10.2 interface skeleton (header のみ公開、本線 ↔ VK repo の API surface)
- §10.3 dynamic link 構成 (本線 LGPL ↔ VK repo 独自 license の合法的境界)
- §10.4 LL UI 変更時 defensibility (charter §7 判断軸 3 (iv) 選択肢の有効化条件)
- §10.5 詳細 interface は r41.5 charter で確定する旨

---

## 4. 次 session 開始時の最初の task list

### task 1: handoff doc + 05 doc + 03 doc の state 確認

1. 本 handoff doc を Read
2. `05-vulkan-api-design.md` foundation group draft (§1-§3 + §9) を Read (整合参照用)
3. `03-sub-phase-3-vulkan-plan.md` §2 status table 確認 (work item (b) が「draft 作成中」のまま)

### task 2: §4-§8 + §10 draft 化

本 doc §3 の section 別 draft 予定 list に従って 05 doc を Edit/拡張。

**推奨 group 分割** (依存関係 + context 量を考慮):

- **group A**: §4 + §5 (render pass + sync) — barrier / attachment / subpass 系の密接 group
- **group B**: §6 + §7 (memory + swapchain) — リソース管理系
- **group C**: §8 + §10 (3 OS + abstraction) — portability + r41.5 skeleton

context 量との見合いで group 単位での進行、必要なら group 境界で再度 handoff。

### task 3: 全 10 section 揃ったら work item (b) 完了宣言

- 03 doc §2 status を「draft 作成中」→「**完了**」に更新
- work item (c) 工程算定 (`06-effort-estimation.md`) 着手前 handoff (foundation + 後半 完成状態)

---

## 5. 設計 input refs (foundation group で確定した内容を §4-§10 が参照)

| §4-§10 で詰める設計 | foundation group での確定 input |
|---|---|
| §4 dynamic rendering 採用判断 | §1.1 で Vulkan 1.3 core feature として確定、§9.1 で enable 必須 |
| §4 attachment 設計 | §3.4 picker buffer (set=0 配置) + §4.4 post-process pass chain で attachment 数想定 |
| §5 sync 戦略 | §1.1 / §9.1 synchronization2 + timeline semaphore 採用済、§9.2 mutable_format で HDR 検討 input |
| §6 VMA | a-4 §6.4.1 で必須採用確定、§3.6 descriptor pool sizing と連携 |
| §7 swapchain | §9.2 `VK_KHR_swapchain` + `VK_EXT_swapchain_maintenance1` 採用済 |
| §8 Mac MoltenVK | §9.4 「vk-RC 直前 phase で詳細化」制約方針を 1 段詳細化 (互換性 maintain のみ、本 (b) では深堀しない) |
| §10 abstraction | charter §6 / §4 (4) Phase 2 / §7 判断軸 3 (iv) を input、(b) では skeleton のみ (r41.5 charter で詳細) |

---

## 6. 内部状態の特記事項 / AYA との合意事項 (前 session 内)

### 6.1 commit 戦略の合意 (本 session)

charter / memory 反映の commit 戦略で **(CA) 2 commit に分割** を採用 (commit 1 = 前 session 産物 / commit 2 = a-4 final 反映)。本 session 第 3 commit (foundation group) も logical unit を 1 commit にまとめる方針継承。

### 6.2 進め方の合意 (本 session)

work item (b) draft 進め方で **(DB) foundation group §1-§3 + §9 を本 session draft、§4-§8 + §10 を次 session に handoff** を採用。section 境界 handoff の根拠は context 量 + 各 section が密接 group (foundation / 描画 path / 資源管理 / portability) に分かれる構造。

### 6.3 self-verify 実施 (前 session 内)

`feedback_self_verify_before_handoff` に従い foundation group draft 完成時に self-trace 実施、確認内容:

- a-4 §6.4 input (必須 7 件 / 設計検討 5 件 / 後送り 3 件) 全消化
- 数字整合 (shader 248 / 改変 13 = 2+4+7 / sampler 206 = 30+70+106 / 段階 1-5)
- 参照矛盾なし (push descriptor は 1.3 で KHR 必須を §1.1 / §9.1 / §9.2 三箇所で一貫)

→ §4-§10 で foundation group 数字を再参照する際は本 self-trace 結果を信頼してよい。

### 6.4 push は AYA 側

`feedback_release_flow` に従い、本 session の 3 commit は local 完了のみ。push / PR / Release は AYA 側で実行。

---

## 7. 関連 doc / memory

### r40 章内部 doc

- `00-charter.md` — r40 章 charter (a-4 final 反映済)
- `01-sub-phase-1-cpu-perf.md` — sub-phase 1 詳細 (CPU perf 全 REJECT)
- `02-sub-phase-2-extended-falsify.md` — sub-phase 2 詳細 (鉱脈ゼロ)
- `03-sub-phase-3-vulkan-plan.md` — work item (b) 親 doc (§2 status table で (b) draft 作成中)
- `04-portage-inventory.md` — work item (a) 全完了 (§6.4 が本 (b) の設計 input source)
- `05-vulkan-api-design.md` — work item (b) draft (§1-§3 + §9 完成、§4-§8 + §10 placeholder)
- `handoff-work-item-a-complete.md` — 前 handoff (work item (a) → (b) 移行)

### memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory (a-4 final 同期 + work item (a) 完了記録済)
- `project_ayastorm_r40_extended.md` — sub-phase 2 詳細
- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 (b)(c)(d)(e) 完了後に着手)
- `feedback_proactive_handoff.md` — foundation group / 次 session 境界 handoff の根拠
- `feedback_self_verify_before_handoff.md` — §1-§3 + §9 self-trace 整合確認の根拠
- `feedback_no_scope_shrink.md` — 「scope shrink せず group 単位で密に draft する」判断根拠

---

## 8. 次 session キックオフ template (AYA → Claude)

次 session 開始時、AYA は以下のような開始メッセージを投げると Claude が context 即把握できる:

```
r40 sub-phase 3 work item (b) foundation group (§1-§3 + §9) draft 完了で前回 handoff した。
handoff doc 確認して、§4-§8 + §10 draft に進む準備をして。
進め方の推奨があれば提示してほしい。
```

Claude 側は:

1. 本 handoff doc を Read
2. 05 doc foundation group (§1-§3 + §9) を Read
3. 03 doc §2 status table 確認
4. §4 の group A / group B / group C 区切りで進行可否を提示
5. AYA judgment 後、選択された path を実行 → §4 着手
