# AYAstorm r41 sub-doc 02-portage-execution — 段階 2 lldrawpool Vulkan command buffer 化

**status**: **closed 2026-05-28 (Pattern α 一括 draft + AYA review PASS、段階 2 着手準備 ready)**
**親 charter**: `docs/specs/ayastorm-r41-gl-removal/00-charter.md` (closed 2026-05-28 = 完成)
**前 sub-doc**: `01-foundation.md` (closed 2026-05-28、段階 1 完遂で役割完了)
**前 handoff**: `handoff-stage-1-complete.md` (段階 1 完遂 → 段階 2 着手前 prep の境界)
**達成条件**: 段階 2 完遂 = lldrawpool 13 file 全 file で Vulkan command buffer 記録動作 (PSO 統合は段階 3 で完成、本段階は record 単体動作 + validation 0 件)
**関連 charter section**: §2 領域 2 (段階 2 lldrawpool Vulkan 化、1.00 PM) + §3 #1/#3 acceptance + §7.4 sub-doc 構成 + §7.5 boundary

---

## §1 段階 2 scope plan

### §1.1 段階 2 scope 再掲 (charter §2 領域 2 + 04 §5.4 段階 2)

- **対象**: `indra/newview/lldrawpool*.cpp` 13 file (合計 7,907 LOC、04 §2.2 確定)、各 pool の draw call emission を Vulkan command buffer record に置換
- **境界条件 (charter §2 領域 2)**: 領域 3 着手前に lldrawpool 単体で Vulkan command buffer record 動作 (PSO 統合は領域 3、本段階は record 単体動作)
- **依存順序 (charter §2 領域 2)**: 領域 1 完了後着手、領域 6 (shader SPIR-V) + 領域 7 (descriptor / render pass) と並走可
- **risk 性質 (charter §2 領域 2)**: **中** — drawpool 内 per-draw state は領域 3 PSO 化まで GL state machine 残存、段階 2 段階での bridging code が一時的に肥大 (段階 3 完成で解消)

### §1.2 13 file 一覧 + verdict + Vulkan 実装方針 (04 §2.2 反映)

| # | file | LOC | 役割 | Vulkan 実装方針 (段階 2 scope) |
|---|---|---|---|---|
| 1 | lldrawpool.cpp | 1,186 | base pool orchestrator、state reset | render pass state setup → command buffer prepare、派生 pool の bridging template 確定 |
| 2 | lldrawpoolalpha.cpp | 1,169 | alpha blending、transparency | blend state placeholder (PSO 統合は段階 3、本段階は bridging code で state machine 経由) |
| 3 | lldrawpoolavatar.cpp | 1,110 | avatar mesh + skinning | bone matrix uniform → SSBO 基本実装 (compute shader 化 / GPU skinning 最適化は段階 3+) |
| 4 | lldrawpoolbump.cpp | 1,122 | bump map + normal map | multi-texture → descriptor set placeholder (set 統合は領域 7 並走) |
| 5 | lldrawpoolmaterials.cpp | 367 | material property pool | PBR uniform → push constant / SSBO placeholder |
| 6 | lldrawpoolpbropaque.cpp | 148 | PBR opaque pool | materials pool 同様、軽量 |
| 7 | lldrawpoolsimple.cpp | 380 | simple geometry (non-deferred) | basic draw → `vkCmdDraw*` 経由 record |
| 8 | lldrawpoolsky.cpp | 57 | sky dome | minimal state、single draw、smoke-test path |
| 9 | lldrawpoolterrain.cpp | 1,167 | terrain quad + `glTexGen` + polygon offset | **shader 側 explicit UV 計算に変更必須** (glTexGen 廃止、領域 6 shader port と並走) + polygon offset → `depthBiasConstantFactor` |
| 10 | lldrawpooltree.cpp | 243 | tree、alpha-test | alpha test → fragment shader `discard` |
| 11 | lldrawpoolwater.cpp | 358 | water surface、dual-layer texture | descriptor set reuse placeholder |
| 12 | lldrawpoolwaterexclusion.cpp | 79 | water exclusion boundary | simple quad render、smoke-test path |
| 13 | lldrawpoolwlsky.cpp | 521 | Windlight sky dome、atmosphere | shader + uniform placeholder (atmospherics shader 統合は領域 6 並走) |

**特記**:
- **#9 terrain.cpp** `glTexGen` は Vulkan 対応物なし → shader 側 explicit UV 計算で対応 (interface 影響は terrain pool 内に閉じる、領域 6 shader port と並走必須)
- **#3 avatar.cpp** skinning は最重量、段階 2 では bone matrix → SSBO 基本実装、compute shader 化は段階 3+ で個別判断 (`feedback_self_bug_no_defer_option.md` 範式: 段階 2 では fix 案で SSBO 基本実装、compute 化は段階 3+ の改善範疇)
- 全 13 file が **要 port** verdict (要再設計は terrain glTexGen 部分のみ)、interface 影響は drawpool 内に閉じる (上流 newview file への波及なし)

### §1.3 並走領域との関係 (charter §2 領域 2 依存順序)

| 並走領域 | 段階 2 内での協調事項 |
|---|---|
| 領域 6 (shader SPIR-V 化) | terrain shader explicit UV 化が領域 6 範囲、drawpool emit と shader 側 binding が一致する必要、段階 2 sub-step 完遂時に shader 側 placeholder で acceptance verify 可 (final shader 統合は領域 6 完遂時) |
| 領域 7 (descriptor / render pass) | drawpool が emit する command buffer 内の descriptor set 想定は領域 7 設計に依存、段階 2 では「将来の descriptor set 配線想定」level の placeholder で済ます |
| 領域 3 (state machine → PSO 化) | drawpool 内 per-draw state は state machine bridging code (GL state machine 経由)、段階 3 完成で bridging 廃止 + PSO 化、段階 2 は bridging 込での acceptance 達成 |
| 領域 8 (LLVKRenderer skeleton) | drawpool 内 inline 実装で signature 整合維持、interface 経由 call 化は r41.5 (charter §3 #6 acceptance criterion 担保) |

---

## §2 port 順序 + dependency graph

### §2.1 13 file 着手順序の決定軸 3 点

1. **base orchestrator 先行** — `lldrawpool.cpp` は全 pool の inheritance base、render pass setup の標準 entry point。base 確定で派生 12 pool の bridging code template が決まる
2. **smoke-test path 早期確保** — 最小複雑度の sky / waterexclusion / simple / pbropaque を先行 port、validation layer 0 件 acceptance の早期 signal として bridging template を検証
3. **特殊対応は末尾** — terrain (glTexGen 改修) + avatar (skinning SSBO) は他 file 経験 + 領域 6 shader port 進捗 base で着手、bridging code 肥大 risk を最後送り

### §2.2 dependency graph (4 階層、base → 軽量 → 標準 → 特殊)

```
lldrawpool.cpp (base orchestrator、1,186 LOC)
├── 軽量 pool (smoke-test path、4 file 664 LOC)
│   ├── lldrawpoolsky.cpp (57)
│   ├── lldrawpoolwaterexclusion.cpp (79)
│   ├── lldrawpoolpbropaque.cpp (148)
│   └── lldrawpoolsimple.cpp (380)
├── 標準 pool (中量、independent 並列可、5 file 3,259 LOC)
│   ├── lldrawpoolalpha.cpp (1,169、blend state)
│   ├── lldrawpooltree.cpp (243、alpha-test)
│   ├── lldrawpoolbump.cpp (1,122、multi-texture)
│   ├── lldrawpoolmaterials.cpp (367、PBR uniform)
│   └── lldrawpoolwater.cpp (358、dual-layer)
├── atmospherics pool (中量、領域 6 shader 並走、1 file 521 LOC)
│   └── lldrawpoolwlsky.cpp (521、Windlight sky)
└── 特殊対応 pool (末尾、領域 6 shader port 進捗 base、2 file 2,277 LOC)
    ├── lldrawpoolterrain.cpp (1,167、glTexGen 廃止 + shader 側 UV 化)
    └── lldrawpoolavatar.cpp (1,110、skinning SSBO 化)
```

### §2.3 並列着手可能性

- base + 軽量 4 pool 完了後、標準 5 pool は independent (相互依存なし) → 並列着手可能、Agent 並列活用候補 (`feedback_use_agents_proactively.md` 反映)
- atmospherics pool (wlsky) は領域 6 shader port (atmospherics shader 群) と並走、shader 側 placeholder で acceptance verify
- 特殊対応 2 pool (terrain + avatar) は base + 軽量 + 標準完了後着手推奨 (drawpool port pattern 確立後、bridging code 肥大耐性確保)

---

## §3 段階 2 sub-step 順序 (5 sub-step 化)

`01-foundation.md` §3.4 範式継承、13 file を §2.2 dependency graph に沿って bundle、5 sub-step に集約。Pattern β 細分化 (file 単位 13 sub-step) は overhead 大、bundle が implementation cadence と整合。

### §3.1 sub-step list

| sub-step | scope | 対象 file (LOC) | 完了 marker |
|---|---|---|---|
| **2.1a** | Vulkan command 基盤 + minimal render pass | `llvkloader.{cpp,h}` 拡張 (VkCommandPool + VkCommandBuffer + minimal VkRenderPass + minimal VkFramebuffer (offscreen attachment)、record begin/end の空転動作) | viewer 起動時 command pool / cmd buffer / render pass / framebuffer 作成成功 + per-frame `vkBeginCommandBuffer` → `vkCmdBeginRenderPass` → `vkCmdEndRenderPass` → `vkEndCommandBuffer` の空 record サイクル動作 + validation layer error 0 件 + submit せず discard (本段階で表示変化なし) |
| **2.1b** | base orchestrator + 軽量 pool 4 件 | lldrawpool.cpp (1,186) + sky (57) + waterexclusion (79) + pbropaque (148) + simple (380) = 5 file 1,850 LOC | base orchestrator が 2.1a 基盤を経由 record 配線、軽量 4 pool が `vkCmdDraw*` placeholder record (PSO 統合は段階 3、本段階は dummy pipeline binding で record 単体動作) + validation 0 件、bridging template 確定 |
| **2.2** | 標準 pool 中量 5 件 | alpha (1,169) + tree (243) + bump (1,122) + materials (367) + water (358) = 5 file 3,259 LOC | 5 pool 独立 port 完遂、blend / alpha-test / multi-texture / PBR uniform / dual-layer の bridging code が validation 0 件で動作 |
| **2.3** | atmospherics pool 1 件 | wlsky (521) = 1 file 521 LOC | Windlight sky の atmospherics shader binding placeholder で record 完遂、shader 側 final 化は領域 6 完遂時 |
| **2.4** | 特殊対応 pool 2 件 | terrain (1,167) + avatar (1,110) = 2 file 2,277 LOC | terrain glTexGen 廃止 + shader 側 UV 化整合 (領域 6 並走)、avatar skinning SSBO 基本実装、bridging code 肥大は許容 (段階 3 で解消) |
| **2.5** | 段階 2 self-check + AYA review + handoff doc | (本 sub-step) | §4.1 acceptance 4 件 self-trace PASS + handoff doc `handoff-stage-2-complete.md` 作成 |

**sub-step 2.1 分割の経緯 (2026-05-28 boundary refine、charter §7.5 反映)**:
- 段階 1 完遂時点で構築済の Vulkan resource は `VkInstance` + `VkPhysicalDevice` + `VkDevice` + `VkQueue` の 4 件のみ
- §4.1 #3 acceptance「13 pool の `render()` が VkCommandBuffer に record + validation 0 件」を満たすには command pool / command buffer / render pass / framebuffer が前提として必要
- 当初 sub-step 2.1 単体で 5 file 一括 port を想定していたが、record 対象 (command buffer) 不在で着手不可と判明 → **2.1a (基盤工事) と 2.1b (base orchestrator + 軽量 4 pool record 配線) に分割**
- 2.1a は llrender 層拡張で、lldrawpool.cpp の編集なし。**LOC 影響**: 段階 2 全体の対象 LOC (7,907) は不変、新規追加は llvkloader.{cpp,h} +~150 LOC 想定 (command pool 作成 + render pass 作成 + per-frame cycle hook)

### §3.2 sub-step 内 file 順序の柔軟性 (charter §7.5 boundary refine 可)

- 各 sub-step 内の file 順序は実装着手時に bridging code 肥大度 / 領域 6 shader 進捗 / validation error 出現傾向で refine 可
- sub-step 2.2 の 5 file は dependency graph 上 independent → Agent 並列活用候補
- sub-step 2.4 の特殊対応 2 file 順序 (terrain → avatar or avatar → terrain) は着手時に AYA + Claude 擦り合わせで決定 (terrain は shader 改修 mid、avatar は SSBO 設計 mid、難度同等)

### §3.3 段階 2 で touch しない file (段階 3-5 + 領域 6/7 scope、`01-foundation.md` §3.5 範式継承)

- llrender 主要 5 file の state machine (`llgl` / `llrender` / `llimagegl` / `llrendertarget` / `llpostprocess`) — 段階 3 PSO 化 (`llglstates.h` Vulkan state alias 整備もここ)
- pipeline.cpp 3 大グローバル (`sCull` / `sShadowRender` / `sCurCameraID`) — 段階 4 frame context 化
- llspatialpartition / llviewershadermgr / llvertexbuffer / llvosky / llvowlsky — 段階 5
- 248 shader SPIR-V 化 (base 228 file) — 領域 6 (段階 2 と並走着手、terrain / wlsky / avatar の shader 側は領域 6 内で port)
- descriptor set 3 階層 + 7 render pass chain — 領域 7 (段階 2 並走着手、配線詳細は段階 3 完成後)
- LLVKRenderer pipeline.cpp inline 実装 — 段階 4 で配置、領域 8 (本段階は drawpool 内 inline 実装で signature 整合のみ確保)

---

## §4 段階 2 completion criteria

charter §3 acceptance criterion #1 (GL 除去) + #3 (段階 1-5 全完遂) の **段階 2 分の self-check**。

### §4.1 段階 2 自己 acceptance

| criterion | metric | test procedure |
|---|---|---|
| **#1-段階 2 (lldrawpool 13 file の GL call 除去)** | `lldrawpool*.cpp` 13 file 内で `gl[A-Z][a-zA-Z]+\s*\(` 直接呼出が 0 件、Vulkan command buffer record 経由に置換済 | `grep -rE "gl[A-Z][a-zA-Z]+\s*\(" indra/newview/lldrawpool*.cpp` が **0 件 hit** (wrapper 経由間接呼出は §3.3 の段階 3-5 scope で順次除去、本段階は drawpool 内直接 GL call のみ判定) |
| **#3-段階 2 (Vulkan command buffer record 動作)** | 13 pool の `render()` / `prerender()` / `endRenderPass()` が Vulkan command buffer に record、validation layer error 0 件 | viewer 起動 + 各 pool の render path 通過確認 (LL_INFOS log)、`VK_LAYER_KHRONOS_validation` で record 中 error / warning 0 件。**validation 検証 cadence**: ReleaseFS_open build は `LL_RELEASE_FOR_DOWNLOAD` で validation 自動 disable のため、2.1a〜2.4 進行中は API-level success (VkCreate* / vkCmd* の error return 0) のみ checkpoint、validation strict 検証は **sub-step 2.5 self-check で一時 force-enable build で纏めて実施** |
| **特殊対応 (terrain glTexGen 廃止 + avatar skinning SSBO)** | terrain shader 側 explicit UV 計算 placeholder 配線、avatar bone matrix SSBO 基本実装 | `grep -E "glTexGen" indra/newview/lldrawpoolterrain.cpp` が **0 件 hit** + shader 側 UV attribute / uniform 配線確認、avatar.cpp 内 bone matrix → VkBuffer (storage buffer) 配線確認 |
| **regression (段階 1 動作維持)** | Vulkan instance + device 動作維持、viewer 起動 + 1 セッション動作維持、AYAstorm 機能 (audio / chat / login) regression 0 件 | `01-foundation.md` §4.1 段階 1 acceptance 4 件再 verify + 段階 2 commit 後 sustained session ~10 分動作 + AYA 起動確認 PASS (`feedback_release_with_user_feedback.md` 遵守で exhaustive solo session 不要) |

### §4.2 不達時の対処 (charter §3 acceptance 運用方針継承)

- 4 criterion のうち 1 件でも未達 = 段階 2 未達 (段階 3 着手保留)
- 未達 criterion 別に対処 (例: #3-段階 2 で alpha pool validation error 残存 → blend state bridging code audit → workaround → 再 sweep)
- **defer / disable 提案 ban** (`feedback_self_bug_no_defer_option.md` 遵守、fix 案のみ提示)
- **仮説 2 連続外れ rule** (`feedback_admit_unknown.md` 遵守): bridging code trouble で仮説 2 連続外れたら gdb breakpoint / validation layer message detail / canary instrumentation で実データ取得に切替

### §4.3 段階 2 完遂後の次 段階

- **段階 3 着手** (llrender 主要 5 file の state machine → PSO 化、1.50 PM、charter §2 高 risk 領域)
- **sub-doc 起草**: 段階 3 着手前に新規 sub-doc (`03-state-machine-pso.md` 仮称) を Pattern α で起草 (charter §7.4 sub-doc 構成 = outline、scope refine 可)
- **handoff doc**: `handoff-stage-2-complete.md` 作成 (段階 2 完遂境界、`feedback_proactive_handoff.md` 遵守)
- **領域 6 並走進捗確認**: 段階 2 完遂時点で 248 shader SPIR-V 化 (領域 6) の進捗 base で着手状況を AYA 共有
- **deferred item 持ち越し確認** (`handoff-stage-1-complete.md` §3.1.1 loader version log mystery): 段階 2 sub-step 2.1 等価の最初の作業として gdb breakpoint at L228 で 30 分以内に原因絞り込み試行

---

## §5 関連 doc / memory

### §5.1 直接参照 doc

| doc | 本 sub-doc での参照 section |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/00-charter.md` | §2 領域 2 (1.00 PM 中 risk) + §3 #1/#3 acceptance + §6.4 並走方針 + §7.4 sub-doc 構成 + §7.5 boundary refine 可 |
| `docs/specs/ayastorm-r41-gl-removal/01-foundation.md` | §3.4 sub-step 範式継承 + §3.5 段階 1 で touch しない file の段階 2 scope 反映 |
| `docs/specs/ayastorm-r41-gl-removal/handoff-stage-1-complete.md` | §3.2 段階 2 scope 確認 + §3.3 周辺修正の文脈 (llviewerstats Vulkan stub 撤去) + §3.4 段階 1 で確定した sub-doc 修正履歴 + §4.4 deferred item (loader version log) |
| `docs/specs/ayastorm-r40-vulkan-migration/04-portage-inventory.md` | §2.2 lldrawpool 13 file per-file verdict + §5.4.1 段階 2 工数感 (3-4 週間 / 1 人月、フルタイム dev) + §6.3.1 段階 2 順位 (base port 戦略 #2 段階) |
| `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` | §3 descriptor set 3 階層 (段階 2 並走前提) + §4 render pass 7 chain (段階 2 record 想定 base) + §10 LLVKRenderer skeleton (段階 4 で配置、段階 2 では interface 経由 call 化保留) |
| `docs/specs/ayastorm-r40-vulkan-migration/06-effort-estimation.md` | §3.1 領域 2 PM 1.00 (本 §1 + §3 段階 2 scope 整合) |

### §5.2 関連 memory

| memory | 本 sub-doc での参照 |
|---|---|
| `project_ayastorm_r41_vulkan_migration.md` | r41 milestone active 状態 + 段階 2 着手前 prep cadence |
| `project_ayastorm_three_platforms.md` | 3 OS 大前提 + Linux 先行例外 (本段階 2 も Linux 限定動作確認、§4.1 #regression 反映) |

### §5.3 関連 feedback

| feedback | 本 sub-doc での参照 |
|---|---|
| `feedback_experiment_branch_single_scope.md` | branch scope 単一性 (r41 内 sub-branch 作らない、`01-foundation.md` §2.3 継承) |
| `feedback_no_auto_commit.md` | commit は AYA 指示後 |
| `feedback_release_flow.md` | push は AYA 手動 |
| `feedback_self_bug_no_defer_option.md` | §4.2 defer / disable 提案 ban + §1.2 #3 avatar compute 化の改善範疇明示 |
| `feedback_self_verify_before_handoff.md` | §3 sub-step 2.5 self-check + §4.3 段階 2 完遂境界 self-trace |
| `feedback_proactive_handoff.md` | §4.3 段階 2 完遂時の handoff doc 作成 |
| `feedback_build_only_verified.md` | §4 completion criteria の satisfy は実機検証 (推論 ban) |
| `feedback_admit_unknown.md` | §4.2 bridging code trouble で仮説 2 連続外れたら gdb / validation detail / canary 切替 |
| `feedback_use_agents_proactively.md` | §2.3 + §3.2 sub-step 2.2 の 5 file independent port で Agent 並列活用 |
| `feedback_perf_map_bfs_drill.md` | §2.2 dependency graph の階層的 sub-step 化 (base → 軽量 → 標準 → atmospherics → 特殊) |
| `feedback_release_with_user_feedback.md` | §4.1 #regression の exhaustive solo session 不要 (AYA 起動確認 PASS で sufficient) |
| `feedback_one_step_at_a_time.md` | §3 sub-step 単位で 1 メッセージ 1 アクション着手 |

---

## §6 起草 cadence + 完成宣言

### §6.1 本 sub-doc 起草情報

- **起草着手**: 2026-05-28
- **起草主体**: AYA + Claude
- **起草先**: `docs/specs/ayastorm-r41-gl-removal/02-portage-execution.md` (本 doc)
- **起草 cadence**: **Pattern α (一括 draft)** — `01-foundation.md` 範式継承、sub-doc は内容具体 (13 file list / 5 sub-step / acceptance) で section 数少なく、Pattern β 分割 overhead 回避 (charter §7.5 boundary refine 可)
- **scope**: **段階 2 only** (lldrawpool 13 file)、段階 3-5 は別 sub-doc 分離 (`handoff-stage-1-complete.md` §3.2 反映、`01-foundation.md` = 段階 1 only precedent + 段階別 risk 性質差 = 段階 2 中 / 段階 3 高 PSO / 段階 4 高 frame context refactor で分離が prep として有効)

### §6.2 完成宣言条件

本 sub-doc は **AYA review PASS で完成宣言**、status field を `closed YYYY-MM-DD (Pattern α 一括 draft + AYA review PASS、段階 2 着手準備 ready)` に更新。

完成宣言後の次 action:

- 段階 2 sub-step 2.1a 着手 (command 基盤 + minimal render pass)、続けて 2.1b (base orchestrator + 軽量 4 pool)
- 段階 2 sub-step 2.5 完遂時に handoff doc `handoff-stage-2-complete.md` 作成
- 段階 3 着手前に sub-doc `03-state-machine-pso.md` (仮称) 起草

### §6.3 本 sub-doc commit 反映

本 sub-doc 完成宣言 commit は AYA 明示指示後 Claude が実施。commit message draft (AYA 指示時 refine 可):

```
docs(r41): sub-doc 02-portage-execution.md 完成 + 段階 2 prep

- 02-portage-execution.md 新規作成 (Pattern α 一括 draft + AYA review PASS)
- §1 段階 2 scope (lldrawpool 13 file 7,907 LOC、1.00 PM 中 risk、領域 6/7 並走可)
- §2 port 順序 + dependency graph (base → 軽量 4 → 標準 5 → atmospherics 1 → 特殊 2 の 4 階層)
- §3 段階 2 sub-step 5 件 (2.1 base+軽量 / 2.2 標準 / 2.3 atmospherics / 2.4 特殊 / 2.5 self-check)
- §4 段階 2 completion criteria (GL 除去 / record 動作 / terrain+avatar 特殊対応 / regression)
- §5 関連 doc/memory 33 件 cross reference + §6 起草 cadence (Pattern α / 段階 2 only scope)
```

push は AYA 手動 (`feedback_release_flow.md` 遵守)。
