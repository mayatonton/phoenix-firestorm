# r41 sub-step 3.1a 全完遂 → 3.1b 着手境界 handoff (2026-05-29)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-1a-iv-ready.md` (3.1a-i/ii 完了 → 3.1a-iv 着手境界)
**本 handoff 位置付け**: sub-step 3.1a 4 段階 (i: trace / ii: bridging 設計素材 / iii: PD 算出 / iv: sub-doc 03 加筆 + 本 handoff 起草) **全完遂宣言** + sub-step 3.1b (PSO 基盤 + state alias root の **実装**) 着手前 scope 確認境界。本 handoff 完成 commit で 3.1a 章 close、AYA GO 指示で 3.1b 着手。

---

## 1. sub-step 3.1a 4 段階全完遂 status (2026-05-29)

### 1.1 4 段階完遂表

| stage | scope | 完遂 status | sealed 先 |
|---|---|---|---|
| **3.1a-i** | llgl.{cpp,h} + 関連 caller の GL state machine trace inventory (file / line / 関数別) | **完了** (6 Agent cluster 並列 trace、Cluster A/B/C/D/E/F) | sub-doc 03 §1.5.2 + `handoff-substep-3-1a-iv-ready.md` §2 |
| **3.1a-ii** | 11 件非自明 bridging items の繋ぎ方設計 (素材収集) | **完了** | sub-doc 03 §1.5.3 + `handoff-substep-3-1a-iv-ready.md` §3 |
| **3.1a-iii** | 作業項目化 + PD 算出 | **完了 (AYA 指示で de-prioritize、軽い refresh のみ)** — charter §6 領域 3 PM 1.50 据置き、sub-step 間相対比較に留める | sub-doc 03 §1.5.6 + `handoff-substep-3-1a-iv-ready.md` §4 |
| **3.1a-iv** | sub-doc 03 spec 訂正 + 本 handoff 起草 | **完了 2026-05-29** | sub-doc 03 §1.5 / §3.1 / §4.1 / §5.1 加筆 + 本 handoff |

### 1.2 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-3-1a-iv-ready.md` | **役割完了 2026-05-29** (3.1a-iv 着手 satisfy、本 handoff で内容引継ぎ) |
| sub-doc `03-state-machine-pso.md` | **re-close pending** (status 行 = `closed 2026-05-29 → 3.1a-iv 加筆 pending AYA review`、AYA PASS で `closed 2026-05-29 (Pattern α 一括 draft + 3.1a-iv 加筆、AYA review PASS)` に re-close) |
| sub-doc `06-shader-spirv.md` / `07-descriptor-renderpass.md` | active 維持 (cross-update 不要、本 §2.2 で根拠説明) |
| `handoff-substep-3-1a-complete.md` (本 handoff) | 新規作成 (3.1a 全完遂 → 3.1b 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (段階 3 sub-step 3.1b 着手 ready 状態へ update) |

---

## 2. sub-doc 03 加筆 4 点 (3.1a-iv 反映)

### 2.1 加筆内容一覧

| section | 加筆内容 |
|---|---|
| **status 行** | `closed 2026-05-29 (Pattern α 一括 draft AYA review PASS) → 3.1a-iv 加筆 pending AYA review` に更新 (AYA PASS で re-close) |
| **新規 §1.5** | sub-step 3.1a-i/ii 設計素材 (4 段階構成 / Cluster A-F trace inventory / 11 件 bridging items / measurement-first / Cluster C false alarm 訂正 / PD refresh) を sealed 形で sub-doc 内に保全 |
| **§3.1 sub-step 3.1 marker refine** | measurement-first device limit query 6 件 + log baseline 取得 / bridging items #1/#2/#4/#10/#11 配置 / dynamic state 配線 / clip 経路 PSO 化 / debug callback 移管 / LLGLSyncFence 物理削除を完了 marker に明示 |
| **§3.1 sub-step 3.4 marker refine** | descriptor set=1 per-material 7 PBR slot (binding 0-6) 配置 / bridging items #7/#8 配置 / VkImage + VkImageView + VMA lifecycle / §1.5.4 device limit 実測値 base で final 化を marker に明示 |
| **§4.1 #1-段階 3 acceptance refine** | PFNGL function pointer declarations の物理削除を **段階 5 一体運用** (§1.5.2 Cluster D 189 file 波及、本段階内 scope 過大) に scope refine、本段階内は直接呼出 GL call 削除 metric のみ |
| **§5.1 関連 doc 表** | `handoff-substep-3-1a-iv-ready.md` + 本 `handoff-substep-3-1a-complete.md` の 2 件追加 |

### 2.2 06 / 07 cross-update 不要根拠

本 session 内で 06 / 07 cross-update を実施しない判断:

| 既存設計 | 11 件 bridging items との整合 | 判定 |
|---|---|---|
| **06 §1.3 並走領域協調事項** (領域 3) | matrix stack → push constant 化 / terrain shader 配信は既に同期記述あり | 既整合 |
| **06 §3.1 sub-step 6.4** (descriptor set binding 統合) | shader 側 `layout(set=N, binding=M)` qualifier 配信が領域 7 と同期、本 §1.5.3 item #6/#7 と整合 | 既整合 |
| **06 §4.1 #4-領域 6 (AYAstorm 改変 13 file untouched)** | Cluster B 確認結果 (clipF.glsl 改変なし + skyV.glsl L95 swap は r14 改変 L65-L225 と independent) は acceptance metric `git diff` 0 件確認に吸収済、新規制約追加なし | 既整合 |
| **07 §1.2.1** (descriptor set 3 階層 / set=1 sampler ×6 / push descriptor 32 binding minimum) | per-draw 6-8 samplers + 7 PBR slot は既設計の sampler ×6 + material 種別の枠内、Cluster C false alarm 訂正で既設計を破壊する redesign 不要 | 既整合 |
| **07 §1.2.2** (pipeline layout、push constant 64 bytes) | matrix stack → push constant 化と一致 (本 §1.5.3 item #6) | 既整合 |
| **07 §1.2.3** (7 pass chain / dynamic rendering) | FBO → VK_KHR_dynamic_rendering と一致 (本 §1.5.3 item #9) | 既整合 |

→ 06 / 07 への新規設計追加 0 件、既存設計の補強/確認のみ。**3.1a-iv で 06 / 07 への加筆は不要**、sub-doc 03 §1.5 内に局所化で十分 (本 handoff 内 cross-link で参照可)。

---

## 3. 11 件 bridging items 3.1 / 3.3 / 3.4 配置最終 fix

sub-doc 03 §1.5.3 の最終配置を本 handoff で固定。3.1b 着手時に implementation reference として参照。

### 3.1 sub-step 3.1 配置 (5 件 + 部分前倒し 2 件、計 7 件 max)

| # | item | 主担当 |
|---|---|---|
| 1 | LLGLState RAII setter no-op 化 | **必須** (state alias root) |
| 2 | LLGLDepthTest dynamic state 化 (VK_EXT_extended_dynamic_state2 / Vulkan 1.3 core) | **必須** (state alias root) |
| 4 | LLGLUserClipPlane 経路 PSO 化 | **必須** (state alias root) |
| 10 | gl_debug_callback → VK_EXT_debug_utils messenger 移管 | **必須** (段階 1 hook reuse) |
| 11 | LLGLSyncFence dead code 削除 | **必須** (Cluster F 確認、副作用 0) |
| 3 | LLGLSquashToFarClip push constant 化 (部分前倒し可) | 可 (3.3 と同期) |
| 5 | skybox skyV.glsl L95 push constant swap 維持 (部分前倒し可) | 可 (3.3 と同期) |

### 3.2 sub-step 3.3 配置 (4 件)

| # | item | 主担当 |
|---|---|---|
| 3 | LLGLSquashToFarClip push constant 化 | **必須** (matrix stack 完成形と同期) |
| 5 | skybox skyV.glsl L95 push constant swap 維持 | **必須** (matrix stack 完成形と同期) |
| 6 | matrix stack → push constant 64 bytes 化 | **必須** (3.3 core scope) |
| 9 | FBO → VK_KHR_dynamic_rendering | **必須** (3.3 core scope、05 §4.7 移行マップ) |

### 3.3 sub-step 3.4 配置 (2 件 + 段階 2 引継ぎ 3 件)

| # | item | 主担当 |
|---|---|---|
| 7 | texture unit → descriptor set=1 per-material mapping (7 PBR slot、binding 0-6) | **必須** (§4 device limit 実測値 base で final 化) |
| 8 | VkImage + VkImageView + VMA lifecycle | **必須** (texture lifecycle core) |
| (段階 2 引継ぎ A) | acceptance #1-段階 2 (lldrawpool 13 file 内 GL call 0 件) | **必須** (本 §5.1) |
| (段階 2 引継ぎ B) | terrain glTexGen 廃止 + 領域 6 shader 側 UV 化 | **必須** (本 §5.1) |
| (段階 2 引継ぎ C) | avatar skinning SSBO 基本実装 | **必須** (本 §5.1) |

### 3.4 sub-step 3.2 / 3.5 配置 (新規 bridging item 無し)

- **3.2**: smoke-test path (llpostprocess legacy effect uniform → push constant + UBO)、03 §1.2 #9 既設計
- **3.5**: validation strict re-verify + handoff doc、verify 中心

---

## 4. measurement-first device limit query 結果保全 (3.1b 着手時 AYA 環境実測 ready)

sub-doc 03 §1.5.4 で確定した sub-step 3.1 着手時 query 6 件:

| device limit field | Vulkan 1.3 minimum | 目的 |
|---|---|---|
| `maxBoundDescriptorSets` | 4 | 03 §1.2 / 07 §1.2.1 set=0/1/2 3 階層が minimum 4 内に収まることを baseline 確認 |
| `maxPushConstantsSize` | 128 bytes | 03 §1.2 / 07 §1.2.2 push constant 64 bytes (matrix stack) が minimum 128 bytes 内に収まることを baseline 確認 |
| `maxPushDescriptors` (`VK_KHR_push_descriptor` extension property) | 32 | 07 §1.2.1 set=2 per-draw binding ≤ 32 minimum 内 baseline 確認 |
| `maxPerStageDescriptorSampledImages` | 16 | 07 §1.2.1 set=1 ~6 samplers + set=0 ~30 samplers 分布が per-stage minimum 内 baseline 確認 |
| `maxColorAttachments` | 4 | 07 §1.2.3 pass 2 gbuffer0/1/2/3 が minimum 4 内に収まることを baseline 確認 |
| `maxDescriptorSetSamplers` | 80 | 07 §1.2.1 全 set 合計 sampler ~206 個分布の minimum baseline 確認 (set 別配置で個別 set あたり < 80 確認) |

**3 driver baseline 実測** (sub-step 3.4 着手前):

| driver | env | 取得 timing |
|---|---|---|
| **NVIDIA RTX 5090** (AYA Linux 環境) | AYA 環境で sub-step 3.1 完遂時に log 出力 | sub-step 3.1 完遂境界 (AYA 起動確認時) |
| Mesa RADV | AYA 環境に AMD discrete GPU 不在 (deferred §6.1 継承) | 段階 10 driver matrix polish で testbed 確保 |
| Mesa ANV (Intel) | AYA 環境に Intel GPU 不在の可能性 (要確認) | sub-step 3.4 着手前に Intel 環境確保確認 |

**設計 final 化 cadence**:
1. sub-step 3.1 完遂時に NVIDIA RTX 5090 baseline 取得 → AYA 共有
2. sub-step 3.4 着手前に Intel 環境確保確認 → 確保できれば ANV baseline 追加取得
3. RADV baseline は段階 10 で確定、本段階内は **NVIDIA baseline** で descriptor 設計 final 化、他 driver は段階 10 で portability subset 整合再確認

---

## 5. 段階 2 引継ぎ 3 件 3.4 配置最終 fix

`handoff-stage-2-complete.md` §1.3-1.4 で AYA 承認済 2 件 (acceptance #1 + 特殊対応 2 件、合計 3 件) を sub-step 3.4 内に最終配置。

### 5.1 3 件配置最終表

| 引継ぎ項目 | sub-step 3.4 内 配置 | 完了 marker |
|---|---|---|
| **A. acceptance #1-段階 2 (lldrawpool 13 file 内 GL call 0 件)** | 12 pool 全 `recordPoolDraws(VkCommandBuffer)` hook body に PSO bind + `vkCmdDraw*` 投入、続いて render path から該当 GL call 削除 | `grep -rE "gl[A-Z][a-zA-Z]+\s*\(" indra/newview/lldrawpool*.cpp` が **0 件 hit** |
| **B. terrain glTexGen 廃止 + shader 側 explicit UV 化** | `lldrawpoolterrain.cpp` 内 `glTexGen` 直接呼出削除 + 領域 6 sub-step 6.x で terrain shader explicit UV attribute 化 (領域 6 並走) | `grep -E "glTexGen" indra/newview/lldrawpoolterrain.cpp` が **0 件 hit** + 領域 6 shader 側 UV attribute 配線確認 |
| **C. avatar skinning SSBO 基本実装** | `lldrawpoolavatar.cpp` 内 bone matrix uniform → `VkBuffer (storage buffer)` 配信、descriptor set=2 per-draw (push descriptor) で bind | avatar.cpp 内 bone matrix → VkBuffer 配線確認 + descriptor binding validation 0 件 |

### 5.2 並走領域 (領域 6 / 領域 7) との同期

| 並走領域 | 3 件配置時の同期事項 |
|---|---|
| 領域 6 (06 §3.1 sub-step 6.x terrain shader port) | 引継ぎ B の shader 側 explicit UV 化は領域 6 並走、06 sub-step 6.3 (B 53 file 修正) 内 terrain shader bundle と同期 |
| 領域 7 (07 §3.1 sub-step 7.4 set=2 per-draw + push descriptor) | 引継ぎ C の avatar bone matrix VkBuffer は領域 7 sub-step 7.4 push descriptor 経由 bind、07 §1.2.1 set=2 binding 設計と整合 |

---

## 6. charter §3 acceptance 段階 3 分整合再確認

sub-step 3.1a-iv 完了時点で charter §3 acceptance との整合再確認 (sub-doc 03 §4.1 base):

| acceptance | 段階 3 分 metric | 3.1a-iv 完了時点 整合状況 |
|---|---|---|
| **#1 (GL 除去)** | llrender 5 file 内 GL 直接呼出 0 件 + lldrawpool 13 file 内 0 件、PFNGL declarations 物理削除は段階 5 一体運用 | sub-doc 03 §4.1 #1-段階 3 反映済、本 handoff §5.1 引継ぎ A 配置済 |
| **#3 (段階 1-5 全完遂)** | 段階 3 = PSO bind + `vkCmdDraw*` + dynamic rendering 動作 + validation 0 件 | sub-doc 03 §4.1 #3-段階 3 反映済、3.1b 以降の実装で satisfy |
| **#4 (228 file SPIR-V + 13 file untouched)** | 段階 3 直接担当 file 内では 13 file 改変なし | sub-doc 06 §4.1 #4-領域 6 で担当、本段階 3 では Cluster B 確認結果 (clipF / skyV L95) で safe 担保 |
| **#5 (descriptor set + render pass)** | descriptor set 3 階層 + push descriptor + dynamic rendering 動作、領域 7 並走 satisfy | sub-doc 03 §4.1 #5-段階 3 反映済、領域 7 並走完遂で satisfy |
| **#6 (LLVKRenderer skeleton signature)** | pipeline.cpp inline 実装と skeleton hook の signature 整合 | sub-doc 03 §4.1 #6-段階 3 反映済、段階 4 skeleton 配置時に satisfy |
| **regression (段階 1-2 動作維持)** | 段階 1 + 段階 2 acceptance 再 verify + AYA 起動確認 PASS | sub-doc 03 §4.1 #regression 反映済、sub-step 3.5 verify |

→ charter §3 acceptance 整合 OK、3.1b 着手前 prep complete。

---

## 7. 3.1b 着手前 scope 確認

### 7.1 sub-step 3.1b scope (PSO 基盤 + state alias root 実装)

sub-doc 03 §3.1 sub-step 3.1 完了 marker (本 handoff §2.1 で refine 済) を実装:

1. **llvkloader.{cpp,h} 拡張**:
   - VkPipelineCache 作成 + persist
   - VkPipelineLayout 標準形 helper
   - PSO compile helper
   - §1.5.4 device limit query 6 件 + log baseline 出力
2. **llgl.{cpp,h} 改修 (3,514 LOC)**:
   - RAII state class (12 件) → PSO state alias 化、setter no-op (item #1)
   - LLGLDepthTest dynamic state 化 (item #2、`VK_EXT_extended_dynamic_state2` Vulkan 1.3 core)
   - LLGLUserClipPlane PSO 化 (item #4、`VkPipelineRasterizationStateCreateInfo` clip distance)
   - gl_debug_callback → VK_EXT_debug_utils messenger 置換 (item #10)
   - LLGLSyncFence (L2947-L2991, 45 LOC) 物理削除 (item #11)
3. **3 / 5 (LLGLSquashToFarClip / skyV.glsl L95 swap) 部分前倒し** (3.3 と同期判断は実装時 refine 可)
4. **最小 PSO compile + bind 動作確認** (sky pool 用 placeholder PSO で 1 frame 内 validation 0 件)

### 7.2 3.1b 着手 prerequisite

- ✓ sub-doc 03 §1.5 設計素材 sealed 済 (本 handoff §1.1)
- ✓ 11 件 bridging items 3.1 / 3.3 / 3.4 配置最終 fix 済 (本 handoff §3)
- ✓ measurement-first device limit query 6 件 confirmed (本 handoff §4)
- ✓ 段階 2 引継ぎ 3 件 sub-step 3.4 配置確定 (本 handoff §5)
- ✓ charter §3 acceptance 段階 3 分整合再確認 (本 handoff §6)
- ✓ 06 / 07 cross-update 不要根拠 sealed (本 handoff §2.2)

### 7.3 3.1b 着手 GO 判断

本 handoff commit (AYA 明示指示後) + AYA GO 指示で 3.1b 着手 (`feedback_no_auto_commit.md` 遵守)。3.1b 着手後の next handoff は **sub-step 3.1b 完遂境界** (`handoff-substep-3-1b-complete.md` 仮称、段階 3 全体の handoff は sub-step 3.5 完遂時の `handoff-stage-3-complete.md`)。

---

## 8. deferred item 持ち越し (`handoff-substep-3-1a-iv-ready.md` §7 継承)

### 8.1 Mesa RADV 動作確認

AYA 環境に AMD discrete GPU 不在 → Mesa RADV 動作未確認、段階 10 driver matrix polish で testbed 確保 (本 handoff §4 で配置最終 fix 済)。

### 8.2 Mesa ANV (Intel) 動作確認

AYA 環境 Intel GPU 不在の可能性 → sub-step 3.4 着手前に確認、確保できれば ANV baseline 取得、不可なら段階 10 で取得 (本 handoff §4)。

### 8.3 macOS / Windows 検証

段階 1〜2 全て Linux 検証のみ、`project_ayastorm_three_platforms.md` 「Linux 先行例外」継承、段階 9 統合 verify でまとめて検証。

### 8.4 PFNGL declarations 物理削除 (Cluster D)

189 file 波及で段階 3 内 scope 過大、**段階 5 (残依存解決) と一体運用** に refine、sub-doc 03 §4.1 #1-段階 3 反映済 (本 handoff §2.1)。

### 8.5 Tree pool marker login 後検証 (`handoff-stage-2-complete.md` §4.2 継承)

段階 3 sub-step 3.1 等での実 PSO bind + draw 投入時に login 後 sustained session で fire 確認可、segment 化不要。

---

## 9. 次 session 注意事項 (`handoff-substep-3-1a-iv-ready.md` §8 継承 + 本 sub-step 特有)

### 9.1 既往継承事項

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
- PD 精度議論 de-prioritize (AYA guidance「やってみて変動する」)
- `feedback_doubt_self_first.md` 再徹底 (Cluster C false alarm 教訓、Agent speculation 再 trace 必須)
- measurement-first 採用 (redesign-first 回避、device query で実測 → 設計判断)

### 9.2 本 handoff 固有 (3.1b 着手前)

- **PSO compile + bind 失敗時の対処**: 仮説 2 連続外れたら validation layer message detail (`VK_LAYER_KHRONOS_validation` strict force-enable) + gdb breakpoint at `vkCreateGraphicsPipelines` + RenderDoc capture で実データ取得に切替、speculation 継続禁止
- **state alias 化の source-level compat 維持**: 46 + 43 file caller の RAII stack 構築 (`LLGLDepthTest depth(GL_TRUE);` 等) は variable name 含めて改変しない、setter のみ no-op 化、charter §1 thesis 整合
- **LLGLSyncFence 物理削除時の確認**: caller 0 件 grep を再確認 (3.1b 着手時に再 grep `grep -rE "LLGLSyncFence" indra/`) してから削除、Cluster F sealed 結果を信任しすぎない
- **`VK_EXT_extended_dynamic_state2` driver support 確認**: 3 driver baseline (NVIDIA / RADV / ANV) で Vulkan 1.3 core promotion 確認、device query で `apiVersion >= VK_API_VERSION_1_3` baseline 取得 → log 出力で確認 → 仮に 1.2 driver 対応必要が出た場合は `VK_EXT_extended_dynamic_state2` extension 単独 enable に fallback (本 handoff §4 measurement-first cadence の枠内)

---

## 10. 関連 doc / memory / commit

### 10.1 関連 doc (r41 章)

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter 本体 (closed 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/01-foundation.md` — sub-doc foundation (closed 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/02-portage-execution.md` — sub-doc 段階 2 (役割完了 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` — sub-doc 段階 3 (3.1a-iv 加筆 PASS で re-close 予定、本 handoff §2 で加筆 4 点反映)
- `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` — sub-doc 領域 6 (closed 2026-05-29、本 handoff §2.2 で cross-update 不要根拠)
- `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` — sub-doc 領域 7 (closed 2026-05-29、本 handoff §2.2 で cross-update 不要根拠)
- `docs/specs/ayastorm-r41-gl-removal/handoff-r41-charter-complete.md` — r41 charter 完成 → 段階 1 着手前 prep
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-1-complete.md` — 段階 1 完遂 → 段階 2 着手前 prep
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-2-1a-complete.md` — sub-step 2.1a 完遂 → 2.1b 着手境界 (役割完了 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-2-4-complete.md` — sub-step 2.1b/2.2/2.3/2.4 完遂 → 2.5 着手境界 (役割完了 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-2-complete.md` — 段階 2 完遂 → 段階 3 着手境界 (役割完了 2026-05-29)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-1a-iv-ready.md` — sub-step 3.1a-i/ii 完了 → 3.1a-iv 着手境界 (役割完了 2026-05-29)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-1a-complete.md` — **本 handoff** (3.1a 全完遂 → 3.1b 着手境界)

### 10.2 関連 commit (本境界までに積まれた branch 上 commit)

- `f6a5503c09` — `docs(r41): 段階 3 sub-doc 03/06/07 起草完了 (Pattern α 一括 draft、AYA review PASS)`
- `152b84c7b2` — `docs(r41): sub-step 3.1a-i/ii 完了 → 3.1a-iv 着手境界 handoff doc 作成`
- (本 handoff 本体 + sub-doc 03 加筆は本 session 末で commit 予定、AYA 明示指示後)

### 10.3 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone active 状態 (3.1b 着手 ready 状態に update 予定)
- `project_ayastorm_three_platforms.md` — 3 OS 大前提 + Linux 先行例外
- `feedback_self_verify_before_handoff.md` — 3.1a 4 段階全完遂 self-trace
- `feedback_proactive_handoff.md` — context 圧迫境界で能動 handoff 提案 → 本 handoff で実施
- `feedback_no_auto_commit.md` — commit は AYA 明示指示後
- `feedback_release_flow.md` — push は AYA 手動
- `feedback_build_only_verified.md` — completion criteria の satisfy は実機検証
- `feedback_admit_unknown.md` — 仮説 2 連続外れたら gdb / validation / RenderDoc 切替
- `feedback_use_agents_proactively.md` — 6 Agent cluster 並列 trace で活用済 (3.1a-i/ii)
- `feedback_doubt_self_first.md` — Cluster C false alarm 教訓、3.1b 以降全 sub-step で徹底
- `feedback_render_full_trace_first.md` — 3.1a 4 段階 trace-before-implement 完遂、3.1b 以降の実装は本 handoff §3 配置 base
- `feedback_no_scope_shrink.md` — AYA「すべて」指示 literal scope 維持、3.1a 4 段階再 scope も AYA 指示の literal 反映
