# r41 段階 2 完遂 → 段階 3 着手境界 handoff (2026-05-28)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-2-4-complete.md` (sub-step 2.1b/2.2/2.3/2.4 完遂 → 2.5 着手境界)
**本 handoff の位置付け**: 段階 2 (lldrawpool Vulkan command buffer 化) 全 sub-step 完遂境界。次は段階 3 (llrender 主要 5 file の state machine → PSO 化、charter §2 高 risk 領域) 着手前 sub-doc `03-state-machine-pso.md` (仮称) 起草 + scope 確認。

---

## 1. 段階 2 完遂 state (2026-05-28)

### 1.1 sub-step 2.5 完遂宣言

- 達成内容:
  - `indra/llrender/llvkloader.cpp` L54 `#ifndef LL_RELEASE_FOR_DOWNLOAD` ブロック一時除去 → validation strict force-enable build → AYA 実機起動 (login 直前 drop) → log grep → 復元
  - validation layer `VK_LAYER_KHRONOS_validation` 1.3.275.0 を AYA 環境に新規 install (`sudo apt install vulkan-validationlayers` Ubuntu 24.04 package)
  - 復元後 `git diff -- indra/llrender/llvkloader.cpp` 差分 0 件確認、commit 不要
- 達成 marker (検証 log evidence):
  - L83: `Vulkan instance created (validation=enabled)` ← layer 正常 load
  - L82-L93: Vulkan init 全 message 出現 (loader 1.4.319 / device select RTX 5090 / queue family / device create / command pool / offscreen image 64x64 / render pass / framebuffer)
  - **`VALIDATION|VK_DEBUG|VK_LAYER` 真の 0 件** (L1586 hit は LLInventory の `validation_info` 統計、誤 hit / Vulkan 無関係)
  - L2628-L2629: shutdown `Vulkan device destroyed` + `Vulkan instance destroyed` clean
  - 12 pool 中 11 marker 出現 (Sky / WLSky / WaterExclusion / GLTFPBR / Simple / Bump / Materials / Alpha / Terrain / Water / Avatar、Tree は login 前で scene-dependent pool が mPools 未登録、code review 済 配線 OK)
- commit: なし (復元差分ゼロ、本 sub-step は実機検証のみ)

### 1.2 段階 2 全 sub-step 完遂 evidence table

| sub-step | 内容 | commit | state |
|---|---|---|---|
| 2.1a | command 基盤 + minimal render pass + per-frame 空 record cycle | `49b9f36427` | 完遂 2026-05-28 ✓ |
| 2.1b | base orchestrator bridging + 軽量 4 pool record 配線 | `a69e6753b9` | 完遂 2026-05-28 ✓ |
| 2.2 | 標準 5 pool (alpha + tree + bump + materials + water) | `7534318ca0` | 完遂 2026-05-28 ✓ |
| 2.3 | atmospherics pool (wlsky) | `a03e76cb88` | 完遂 2026-05-28 ✓ |
| 2.4 | 特殊対応 pool (terrain + avatar) | `61bd502445` | 完遂 2026-05-28 ✓ |
| **2.5** | 段階 2 self-check + validation strict 検証 + handoff doc 作成 | (検証は no-commit、handoff doc は本 doc commit) | **完遂 2026-05-28** ✓ |

### 1.3 段階 2 acceptance 4 件 self-trace (02-portage-execution.md §4.1)

**重要**: 前 handoff `handoff-substep-2-1a-complete.md` §2.2 で **(1) hook wiring only** approach を採用 (AYA 承認「OK」)。この approach の trade-off により段階 2 acceptance は次のように satisfy / scope refine される:

| acceptance criterion | 段階 2 sub-step での state | 判定 |
|---|---|---|
| **#1-段階 2 (lldrawpool 13 file の GL call 0 件)** | drawpool 内 GL call は **render path で生存継続中**、段階 2 sub-step では `recordPoolDraws(VkCommandBuffer)` hook を配線したのみ。GL call 除去は **段階 3 PSO 完成時に hook body に PSO bind + vkCmdDraw* 投入 + render path から GL call 削除** という流れに scope refine | **未達 → 段階 3 と一体運用** (charter §7.5 boundary refine 範囲内) |
| **#3-段階 2 (Vulkan command buffer record 動作 + validation 0 件)** | 12 pool 全 hook が `LLPipeline::recordVulkanPools()` dispatcher 経由で per-frame 1 回 fire (11/12 実測、Tree は scene 依存)、validation strict build (layer 1.3.275 force-enable) で `VALIDATION|VK_DEBUG|VK_LAYER` 真の 0 件達成 | **達成** ✓ |
| **特殊対応 (terrain glTexGen 廃止 + avatar skinning SSBO)** | terrain.cpp / avatar.cpp は record hook 配線のみ。`glTexGen` 直接呼出 + bone matrix uniform は **render path で生存継続中**、本実装は段階 3 と一体運用に scope refine | **未達 → 段階 3 と一体運用** |
| **regression (段階 1 動作維持)** | 段階 1 acceptance (Vulkan instance + device 動作 + viewer 起動 + AYAstorm 機能 regression 0 件) 全 sub-step 通して維持、validation strict build でも AYA login 直前まで正常起動 | **達成** ✓ |

### 1.4 段階 2 完遂宣言の解釈

- 段階 2 全 sub-step (2.1a〜2.5) は **完遂** (acceptance #3 + regression 達成 + validation 厳格検証 PASS)
- ただし acceptance #1 + 特殊対応は **段階 2 sub-step では未達**、`(1) hook wiring only` trade-off の帰結として **段階 3 と一体運用** で satisfy する scope refine 状態
- charter §7.5 boundary refine 範囲内であり、charter §3 全体 acceptance #1 (GL 除去) は段階 3 完遂時に「全 13 pool render path で hook body に PSO bind + vkCmdDraw* 配線 → GL call 廃止」で達成見込
- **段階 3 着手は GO** (bridging template 確立 + validation 0 件 evidence 取得済)

### 1.5 status close / next active

| doc / memory | status |
|---|---|
| `handoff-substep-2-1a-complete.md` | **役割完了 2026-05-28** (2.1b 着手 satisfy) |
| `handoff-substep-2-4-complete.md` | **役割完了 2026-05-28** (2.5 着手 satisfy) |
| `handoff-stage-2-complete.md` (本 handoff) | 新規作成 (段階 2 完遂 → 段階 3 着手境界) |
| sub-doc `02-portage-execution.md` | **役割完了 2026-05-28** (段階 2 完遂で参照役割完了、acceptance #1 / 特殊対応の段階 3 一体運用も charter §7.5 範囲内で本 handoff §1.3 に記録) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 |

---

## 2. 段階 3 着手前の scope 確認 (次 session 最優先)

### 2.1 段階 3 charter scope (charter §2 領域 3 + §6 並走方針)

- **対象**: llrender 主要 5 file (`llgl.{cpp,h}` / `llrender.{cpp,h}` / `llimagegl.{cpp,h}` / `llrendertarget.{cpp,h}` / `llpostprocess.{cpp,h}`) の state machine → Vulkan PSO 化
- **risk**: **高** (charter §2 領域 3 最大 risk 領域、1.50 PM、charter §6.4 「描画再構築 phase」)
- **依存順序**: 段階 2 完遂後着手 (本 handoff で boundary 確定)、領域 6 (shader SPIR-V) + 領域 7 (descriptor / render pass) と並走必須
- **段階 2 からの引継ぎ**:
  - 12 pool の `recordPoolDraws(VkCommandBuffer)` hook 内に **PSO bind + vkCmdDraw* 投入** で render path から GL call 排除
  - terrain glTexGen 廃止 + shader 側 explicit UV 化 (領域 6 並走)
  - avatar bone matrix SSBO 化 (領域 6 / 7 並走)

### 2.2 sub-doc 03 起草が次 deliverable

- doc path: `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` (charter §7.4 sub-doc 構成、Pattern α 一括 outline draft → AYA review boundary)
- 段階 3 内 sub-step 分割は段階 2 の 5 sub-step 範式継承想定 (基盤 + 軽量 + 標準 + 特殊 + self-check)、charter §7.5 で boundary refine 可
- 起草 deliverable:
  - 段階 3 scope plan (charter §2 領域 3 + 段階 2 引継ぎ反映)
  - 5 file の port 順序 + dependency graph
  - sub-step list (3.1 〜 3.5 等、段階 2 範式継承)
  - 段階 3 completion criteria
  - 段階 2 acceptance #1 + 特殊対応の段階 3 内 satisfy 計画 (本 handoff §1.3 引継ぎ記録)

### 2.3 段階 3 着手前の AYA 擦り合わせ事項

- 段階 3 sub-doc 03 の draft pattern (α 一括 vs β 細分化) 選択
- 段階 2 acceptance #1 + 特殊対応の段階 3 統合運用 (本 handoff §1.4 解釈) の AYA 承認
- 領域 6 (shader SPIR-V 化) 進捗状況の確認 — 段階 3 PSO + shader 配線整合性に影響
- 領域 7 (descriptor / render pass) 起草着手判断 — 段階 3 PSO bind は descriptor set 必須、領域 7 sub-doc 並走起草が望ましい

### 2.4 次 session 最初のアクション

1. **本 handoff 確認** (`handoff-stage-2-complete.md`) を Read
2. **段階 2 acceptance #1 / 特殊対応の段階 3 一体運用** を §1.3-1.4 で AYA 承認 (質問 1 件)
3. **sub-doc 03 起草着手判断**:
   - Pattern α 一括 outline draft → AYA review boundary (charter §7.4 標準範式)
   - 起草対象: §1 段階 3 scope plan、§2 port 順序 + dependency graph、§3 sub-step 分割、§4 completion criteria、§5 関連 doc / memory
4. **領域 6 / 領域 7 並走起草判断** (sub-doc 03 と同タイミングで領域 6 / 7 sub-doc も着手するか、段階 3 着手後の追加判断とするか)

---

## 3. 検証 evidence (本 sub-step 2.5 で取得した log 抜粋)

### 3.1 validation strict build 起動 log (cache clear 後の clean run)

```
L81: 2026-05-28T19:18:29Z INFO #Vulkan# llvkloader.cpp(418) initVulkan : Initializing Vulkan loader...
L82: 2026-05-28T19:18:29Z INFO #Vulkan# llvkloader.cpp(420) initVulkan : Vulkan loader version 1.4.319
L83: 2026-05-28T19:18:29Z INFO #Vulkan# llvkloader.cpp(96)  createInstance : Vulkan instance created (validation=enabled)
L84: 2026-05-28T19:18:29Z INFO #Vulkan# llvkloader.cpp(125) selectPhysicalDevice : Found 2 physical device(s)
L85: 2026-05-28T19:18:29Z INFO #Vulkan# llvkloader.cpp(146) selectPhysicalDevice :   Candidate: NVIDIA GeForce RTX 5090 (type=2, score=100)
L86: 2026-05-28T19:18:29Z INFO #Vulkan# llvkloader.cpp(146) selectPhysicalDevice :   Candidate: llvmpipe (LLVM 20.1.2, 256 bits) (type=4, score=10)
L87: 2026-05-28T19:18:29Z INFO #Vulkan# llvkloader.cpp(166) selectPhysicalDevice : Selected physical device: NVIDIA GeForce RTX 5090
L88: 2026-05-28T19:18:29Z INFO #Vulkan# llvkloader.cpp(182) selectQueueFamily : Graphics queue family: 0 (queueCount=16)
L89: 2026-05-28T19:18:29Z INFO #Vulkan# llvkloader.cpp(219) createDevice : Vulkan device created (graphics queue family 0)
L90: 2026-05-28T19:18:29Z INFO #Vulkan# llvkloader.cpp(268) createCommandPool : Command pool + primary command buffer created
L91: 2026-05-28T19:18:29Z INFO #Vulkan# llvkloader.cpp(336) createOffscreenImage : Offscreen image 64x64 created (16384 bytes)
L92: 2026-05-28T19:18:29Z INFO #Vulkan# llvkloader.cpp(376) createRenderPass : Minimal render pass created (1 color attachment)
L93: 2026-05-28T19:18:29Z INFO #Vulkan# llvkloader.cpp(398) createFramebuffer : Framebuffer created
```

### 3.2 pool record hook fire log (12 pool 中 11 marker、Tree は scene 依存で inactive)

```
L977-L982: WaterExclusion / Simple / Bump / Materials / GLTFPBR / Alpha pool hook fired (one-shot)
L1458-L1459: Terrain / Water pool hook fired
L1537-L1538: Sky / WLSky pool hook fired
L1583: Avatar pool hook fired
```

### 3.3 shutdown clean log

```
L2628: 2026-05-28T19:18:47Z INFO #Vulkan# llvkloader.cpp(492) shutdownVulkan : Vulkan device destroyed
L2629: 2026-05-28T19:18:47Z INFO #Vulkan# llvkloader.cpp(500) shutdownVulkan : Vulkan instance destroyed
```

### 3.4 VALIDATION grep 結果

- `grep -nE 'VALIDATION|VK_DEBUG|VK_LAYER' ~/.ayastorm_x64/logs/AYAstorm.log` 真の hit: **0 件**
- 1 件の文字列 hit (L1586) は LLInventory の `validation_info` 統計 (`duplicate_system_folders_count`, `orphaned_count`, etc.)、Vulkan 無関係 → **誤 hit**

---

## 4. deferred item 持ち越し

### 4.1 Mesa RADV 動作確認 (`handoff-stage-1-complete.md` §3.1.2 + `handoff-substep-2-4-complete.md` §4.1 継承)

- AYA 環境に AMD discrete GPU 不在 → Mesa RADV 動作未確認
- 段階 3-9 進行中は据置、段階 10 driver matrix polish で改めて testbed 確保

### 4.2 Tree pool marker login 後検証

- 段階 2 全 verify は login 前 drop で実施、Tree pool は scene-dependent (mPools 動的登録) で login 前は inactive
- code review (commit `7534318ca0` lldrawpooltree.{h,cpp}) で配線は確認済
- 段階 3 sub-step 3.1 等での実 PSO bind + draw 投入時に login 後 sustained session で Tree pool fire 確認可、segment 化不要

### 4.3 macOS / Windows 検証

- 段階 1〜2 全て Linux 検証のみ、`project_ayastorm_three_platforms.md` 「Linux 先行例外」 (`feedback_bd_port_autonomous_exec.md` 期間継承の autonomous build 全権内)
- charter §6 並走方針で macOS / Windows は段階 9 統合 verify でまとめて検証

---

## 5. 次 session 開始 action cadence

### 5.1 次 session 開始時の最初のアクション

1. **handoff 確認**: 本 handoff (`handoff-stage-2-complete.md`) を Read
2. **段階 2 acceptance #1 / 特殊対応の段階 3 一体運用解釈** (本 handoff §1.3-1.4) の AYA 承認 (質問 1 件)
3. **sub-doc 03 起草着手**: Pattern α 一括 outline draft → AYA review (charter §7.4 標準範式)
4. **領域 6 / 7 並走起草判断**: sub-doc 03 と同タイミング起草するか、段階 3 着手後判断とするか (質問 1 件)

### 5.2 段階 3 着手中の注意事項 (`handoff-substep-2-4-complete.md` §5.2 継承)

- **Linux first-class baseline 厳守**: NVIDIA proprietary on RTX 5090 で動作確認、Mesa RADV は段階 10 polish
- **parity 不要**: charter §1 thesis 維持、AYAstorm 改変 13 file shader は r42-α/β/γ で port
- **acceptance satisfy は実機検証**: `feedback_build_only_verified.md` 遵守、推論 ban
- **仮説 2 連続外れ rule**: `feedback_admit_unknown.md` 反映
- **sub-step 完遂時 self-trace + handoff**: `feedback_self_verify_before_handoff.md` + `feedback_proactive_handoff.md` 継承
- **commit / push cadence**: commit は AYA 指示後、push は AYA 手動 (`feedback_release_flow.md` 遵守)
- **検証用 force-enable は出荷物に残さない**: 本 sub-step 2.5 で `#ifndef LL_RELEASE_FOR_DOWNLOAD` 復元実施、段階 3 以降も同様 (`feedback_remove_verification_logs.md`)

### 5.3 段階 3 着手で新規発生する注意事項

- **bridging code 肥大耐性**: 段階 3 で GL state machine → PSO 化、段階 2 で残した bridging code が一旦肥大化する可能性、PSO 完成で解消する流れ (charter §2 領域 3 + 02 §1.1 risk 解説)
- **descriptor set 配線設計**: 領域 7 並走着手判断と関連、descriptor set 3 階層 (frame / material / object) の概念設計が段階 3 PSO 配線に必要
- **avatar skinning SSBO の段階 3 内 timing**: charter §2 領域 2 + 02 §1.2 #3 で「bone matrix → SSBO 基本実装は段階 2、compute shader 化は段階 3+」と切り分けたが、段階 2 では未着手 (hook 配線のみ)、段階 3 内で SSBO 基本実装 + PSO 配線を同時実施

---

## 6. 関連 doc / memory

### 関連 doc (r41 章)

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter 本体 (closed 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/01-foundation.md` — sub-doc foundation (closed 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/02-portage-execution.md` — sub-doc 段階 2 (役割完了 2026-05-28、本 handoff §1.5 で close)
- `docs/specs/ayastorm-r41-gl-removal/handoff-r41-charter-complete.md` — r41 charter 完成 → 段階 1 着手前 prep
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-1-complete.md` — 段階 1 完遂 → 段階 2 着手前 prep
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-2-1a-complete.md` — sub-step 2.1a 完遂 → 2.1b 着手境界 (役割完了 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-2-4-complete.md` — sub-step 2.1b/2.2/2.3/2.4 完遂 → 2.5 着手境界 (役割完了 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-2-complete.md` — **本 handoff** (段階 2 完遂 → 段階 3 着手境界)

### 関連 commit (本 session で積まれた branch 上 commit)

- `cad99415b9` — `docs(r41): sub-step 2.1b/2.2/2.3/2.4 完遂 → 2.5 着手境界 handoff doc 作成` (前 session 完成 doc)
- (本 sub-step 2.5 は no-code-commit、本 handoff doc commit のみ予定)

### 関連 commit (段階 2 全 sub-step、参照用)

- `49b9f36427` — sub-step 2.1a
- `a69e6753b9` — sub-step 2.1b
- `7534318ca0` — sub-step 2.2
- `a03e76cb88` — sub-step 2.3
- `61bd502445` — sub-step 2.4

### 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone active
- `project_ayastorm_three_platforms.md` — 3 OS 大前提 + Linux 先行例外
- `feedback_self_verify_before_handoff.md` — sub-step 2.5 validation strict 検証本 handoff §1.1 + §3 反映
- `feedback_proactive_handoff.md` — 段階 2 完遂境界 handoff 本 doc で実施
- `feedback_no_auto_commit.md` — 全 sub-step commit は AYA 明示指示後
- `feedback_release_flow.md` — push は AYA 手動
- `feedback_build_only_verified.md` — §1.1 達成 marker は実機検証 evidence (log 抜粋本 handoff §3)
- `feedback_remove_verification_logs.md` — §1.1 force-enable 復元実施
- `feedback_admit_unknown.md` — §1.3 `hook wiring only` trade-off の正直記録 + 段階 3 一体運用 scope refine の正直表明
- `feedback_one_step_at_a_time.md` — 各 sub-step 1 cycle 完結
