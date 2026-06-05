# r41 charter 完成 → r41 着手前 prep / 着手 chapter 境界 handoff (2026-05-28)

**前 handoff**: `docs/specs/ayastorm-r40-vulkan-migration/handoff-r40-close-r41-charter-start.md` (r40 章 close + r41 charter 起草着手の境界)
**本 handoff の位置付け**: r41 charter 完成 (Pattern β group 分割 1 session 起草) 後の最終 state snapshot + r41 着手前 prep / 着手のための base 資料 + 次 session 開始 action cadence

---

## 1. r41 charter 完成 state (2026-05-28)

### 1.1 完成宣言

- AYA review PASS 3 group 連続 (foundation + group A + group B) で **r41 charter 完成宣言 2026-05-28**
- charter `docs/specs/ayastorm-r41-gl-removal/00-charter.md` 新規作成 (~620 行)
- 起草 cadence: **Pattern β (group 分割) 1 session 起草** (r40 章 work item (c)(d)(e) 実績継承)
- 完成宣言と同時実施: memory `project_ayastorm_r41_vulkan_migration.md` status **pending → active** + MEMORY.md index 行 update + 本 handoff doc 作成

### 1.2 status close / active 状態

| doc / memory | status |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/00-charter.md` L3 | **closed 2026-05-28** (r41 charter 完成、r41 着手準備 ready / foundation + group A + group B 全 AYA review PASS) |
| memory `project_ayastorm_r41_vulkan_migration.md` | **active 2026-05-28〜** (charter 完成、r41 着手準備 ready) |
| MEMORY.md L119 r41 index 行 | **Active 2026-05-28〜** (charter 完成、中央値 ~84 暦月 / ~2033 中) |
| MEMORY.md L117 r40 index 行 | **[履歴] Closed 2026-05-28** (工程プラン完成済、r41 着手へ移行) |

### 1.3 commit / push 反映 (未実施、AYA 指示後)

- 本 session の 編集 = 5 file: `00-charter.md` (新規) + `project_ayastorm_r41_vulkan_migration.md` (update) + `MEMORY.md` (2 行 update) + `handoff-r41-charter-complete.md` (新規) + (本 commit 時に追加なら charter status footer)
- commit message draft (charter §charter 完成宣言 footer 参照): `docs(r41): r41 charter 完成 (GL 除去 + Vulkan 空転 vk-α) + memory active 化 + handoff doc`
- push: AYA 手動 (`feedback_release_flow.md` 遵守)
- branch: `feature/ayastorm-r40-vulkan-migration` (現 branch、r41 着手で `feature/ayastorm-r41-gl-removal` 新規 or 現 branch 継承を §6 で擦り合わせ予定)

### 1.4 r41 charter 成果物 (1 doc + 1 memory update + 1 handoff)

#### charter doc 1 件 (`docs/specs/ayastorm-r41-gl-removal/`)

| doc | 役割 | status |
|---|---|---|
| `00-charter.md` | r41 milestone charter 本体 (8 section: header + §1 thesis + §2 work breakdown + §3 acceptance criteria + §4 暦月 + §5 依存 + §6 起草 cadence + §7 詳細化方針 + §8 関連 doc/memory) | closed 2026-05-28 = 完成 |

#### memory 1 件 update

- `project_ayastorm_r41_vulkan_migration.md` (active 2026-05-28〜) + MEMORY.md L119 index 行 update

#### handoff doc 1 件

| handoff | 完了範囲 |
|---|---|
| `handoff-r41-charter-complete.md` | **本 handoff** (r41 charter 完成 → r41 着手前 prep / 着手の境界) |

---

## 2. r41 着手の base 資料

### 2.1 着手 base = `00-charter.md` §2 work breakdown + §3 acceptance criteria

r41 着手の base は本 charter §2 + §3。**11 領域 work breakdown** + **9 件 acceptance criteria** が r41 完遂 metric:

#### §2 work breakdown (11 領域、r41 total 16.17 PM = base 11.78 + 余裕係数 +4.39)

| # | 領域 | PM | 主作業 |
|---|---|---|---|
| 1 | 段階 1: GL header wrapper 置換 + volk loader | 0.50 | 212 GL header → volk-based + Vulkan instance/device 初期化 |
| 2 | 段階 2: lldrawpool Vulkan 化 (13 file) | 1.00 | lldrawpool 系 GL call → command buffer record |
| 3 | 段階 3: state machine → PSO 化 (llrender 主要 5 file) | 1.50 | state machine → Vulkan PSO + render pass 統合 |
| 4 | 段階 4: pipeline.cpp 3 大グローバル → frame context | 1.00 | `sCull` / `sShadowRender` / `sCurCameraID` 集約 |
| 5 | 段階 5: llspatialpartition / llviewershadermgr / llvertexbuffer 依存解決 | 0.50 | 残依存 file Vulkan 等価実装 |
| 6 | 248 GLSL shader SPIR-V 化 (base ~228 file) | 4.96 | glslang cross compile + descriptor set 整合 |
| 7 | descriptor set + render pass 設計反映 | 1.50 | per-frame/material/draw 3 階層 + 7 pass chain |
| 8 | Vulkan code abstraction skeleton | 0.50 | LLVKRenderer interface 骨子 (pipeline.cpp 内 inline) |
| 9 | swapchain + present + UI 黒画面動作確認 | 0.40 | viewer 起動 → 黒画面 + UI 描画 (vk-α 達成) |
| 10 | Linux 限定 baseline polish | 0.36 | Mesa RADV / Mesa ANV / NVIDIA driver matrix 初期動作 |
| | **base 計** | **11.78** | — |
| | 余裕係数 +37% | **+4.39** | — |
| | **r41 total (フルタイム dev 換算)** | **~16.17 PM** | — |

#### §3 acceptance criteria 9 件 (達成判定 = 全 PASS)

1. GL 除去完遂 (`ldd` + dlsym hook + grep audit で 0 件)
2. Vulkan 空転動作 (viewer 起動 → 黒画面 + UI、validation error 0 件、30 分 sustained leak 0)
3. 段階 1-5 全完遂 (対象 file 群の GL call audit 0 件 + 段階別動作確認)
4. 248 shader SPIR-V 化 base port ~228 file (AYAstorm 改変 13 file は untouched = r42 で port)
5. descriptor set + render pass 設計実装 (set=0/1/2 3 階層 + 7 pass chain + KHR_dynamic_rendering)
6. LLVKRenderer interface skeleton (pipeline.cpp 内 inline 実装、signature 整合)
7. Linux baseline first-class (Mesa RADV/ANV/NVIDIA 3 driver で空転動作)
8. regression sweep (audio + 描画非依存機能 動作、描画依存機能の動作不可は許容)
9. Win/Mac 未着手宣言 (本線 GL 維持、Vulkan 着手は r42-α/β)

### 2.2 着手前 prep の work item (charter §6.3 (α) + §5.5 環境前提 + §7.4 sub-doc)

r41 段階 1 着手前に整備する prep work:

| prep item | 内容 |
|---|---|
| **branch 戦略確定** | `feature/ayastorm-r41-gl-removal` 新規 or 現 `feature/ayastorm-r40-vulkan-migration` 継承を AYA + Claude 擦り合わせ |
| **環境前提整備** | LunarG SDK 1.3.x + volk loader + VMA + glslang 統合 (3rdparty fetch script 整備) |
| **driver matrix prep** | Mesa RADV / Mesa ANV / NVIDIA proprietary の動作確認環境準備 |
| **sub-doc `01-foundation.md` 着手** | charter §7.4 sub-doc 構成の最初の doc、環境前提整備 + branch 戦略確定 + 段階 1 file list 詳細化 |

### 2.3 関連 doc / memory cross-reference (charter §8 反映)

| doc / memory | r41 着手での参照 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/00-charter.md` | 本 charter (r41 着手の master reference、全 section 適用) |
| `docs/specs/ayastorm-r40-vulkan-migration/04-portage-inventory.md` | §5.4 段階 port 戦略 5 段階 (本 charter §2 backbone) + a-3 §B.x AYAstorm 機能 pull-in 順 |
| `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` | §3 descriptor / §4 render pass / §7 swapchain / §8 OS 別 / §9 extension / §10 skeleton (本 charter §2 領域 6-10 + §3 #5/#6 base) |
| `docs/specs/ayastorm-r40-vulkan-migration/06-effort-estimation.md` | §3.1-§3.2 工程算定 (本 charter §2 PM base) / §5.3 暦月 + §5.6 marker / §6.3 r41 band / §8.3 r41 plan B trigger |
| `docs/specs/ayastorm-r40-vulkan-migration/07-r42-plus-milestone-mapping.md` | §1.2 正式区分 + §5.1 r41 outline (本 charter 全体構成 base) + §5.2 r41.5 outline (r41 完遂後の next milestone) |
| memory `project_ayastorm_r41_vulkan_migration.md` | r41 milestone active memory (本 charter completion 反映済) |
| memory `project_ayastorm_r40_cpu_parallel.md` | r40 章 close memory (前 milestone 達成確認) |
| memory `project_ayastorm_three_platforms.md` | Linux first-class baseline 例外 (本 charter §1 thesis (3) + §3 #7/#9 base) |
| feedback `feedback_no_auto_commit.md` | commit は AYA 指示後 |
| feedback `feedback_release_flow.md` | push は AYA 手動 |
| feedback `feedback_self_verify_before_handoff.md` | r41 着手中 段階別 self-trace |
| feedback `feedback_proactive_handoff.md` | r41 着手中 段階境界 handoff |
| feedback `feedback_build_only_verified.md` | r41 着手中 acceptance satisfy は実機検証 (推論禁止) |
| feedback `feedback_admit_unknown.md` | 仮説 2 連続外れたら log/canary/bisect 切替 |
| feedback `feedback_use_agents_proactively.md` | r41 着手中の trace / grep / 段階別 audit で Agent 活用 |

---

## 3. 次 session 開始 action cadence

### 3.1 次 session 開始時の最初の action

1. **handoff 確認**: 本 handoff (`handoff-r41-charter-complete.md`) を Read
2. **base 資料確認**: `00-charter.md` 全 8 section + 特に §2 / §3 / §6.3 / §7.4 sub-doc 構成を Read
3. **memory 確認**: `project_ayastorm_r41_vulkan_migration.md` (active) + `project_ayastorm_r40_cpu_parallel.md` (closed) で r41 + r40 章 state 確認
4. **AYA cadence 確認**: r41 着手前 prep の進め方を AYA さんに確認 (branch 戦略 + 環境前提整備 + sub-doc `01-foundation.md` 着手 cadence)
5. **commit 確認**: 本 session の編集 5 file (charter + memory + MEMORY.md + handoff doc) の commit を AYA 指示後 Claude 実行

### 3.2 r41 着手前 prep の cadence 候補

- **Pattern (a) full prep 先行**: 環境前提整備 + branch 戦略 + sub-doc `01-foundation.md` 起草を全部終えてから段階 1 着手
- **Pattern (b) 段階 1 着手と並走**: 環境前提整備 + branch 戦略確定 → sub-doc `01-foundation.md` は段階 1 着手と並走で起草
- **Pattern (c) minimum prep 着手**: branch 戦略のみ確定 → 段階 1 着手 → 段階 1 完遂時に sub-doc `01-foundation.md` retroactive 起草

cadence 選好は次 session 開始時に AYA さんと擦り合わせ。**推奨は Pattern (a)** (環境前提整備 + branch 戦略 + sub-doc 起草で着手 base を固める)、ただし環境前提整備が想定外に長引く場合 Pattern (b) に切替可。

### 3.3 r41 着手中の注意事項 (charter §6.5 + memory + feedback 反映)

- **Linux first-class baseline 厳守**: charter §1 thesis (3) + §3 #7 acceptance + §3 #9 Win/Mac 未着手、memory `project_ayastorm_three_platforms.md` の明示指示要件を満たす例外
- **parity 不要**: charter §1 thesis scope 境界、AYAstorm 改変 13 file shader (picker 2 / Cinematic 4 / visual realism 7) は **r42-α/β/γ で port** (r41 段階で touch しない)
- **無期限 / AYA life plan**: charter §4 (3) 反映、§4.3 plan B trigger は外部条件 + 上方 band 上限突破でのみ発動
- **acceptance satisfy は実機検証**: memory `feedback_build_only_verified.md` 遵守、charter §3 acceptance criterion を推論 ベース 判定しない
- **仮説 2 連続外れ rule**: memory `feedback_admit_unknown.md` 反映、driver quirks / shader cross compile / state machine PSO 化 trouble で log/canary/bisect 切替
- **段階境界 handoff**: 段階 1-5 各完遂時に self-trace + AYA review + handoff doc (charter §6.4 並走方針)
- **commit / push cadence**: commit は AYA 指示後 Claude 実行、push は AYA 手動

### 3.4 r41 着手の段階別 cadence (charter §2 領域別)

| 段階 | 領域 | 着手順序 |
|---|---|---|
| 段階 1 | 領域 1: GL header wrapper 置換 + volk loader | 最初 (全領域の前提) |
| 段階 2 | 領域 2: lldrawpool Vulkan 化 | 段階 1 完遂後、領域 6/7 と並走可 |
| 段階 3 | 領域 3: state machine → PSO 化 | 領域 2 + 6 + 7 と協調必須、最大の refactor |
| 段階 4 | 領域 4: pipeline.cpp 3 大グローバル → frame context | 領域 3 と並走可 (refactor 対象独立) |
| 段階 5 | 領域 5: 残依存解決 (llspatialpartition 等) | 領域 1-4 完了後 |
| 並走 | 領域 6: 248 shader SPIR-V 化 | 領域 1 完了後、領域 2/3 と並走必須 |
| 並走 | 領域 7: descriptor set + render pass | 領域 1 完了後、領域 2/3/6 と並走必須 |
| 並走 | 領域 8: LLVKRenderer skeleton | 領域 4 と協調、skeleton 配置のみ |
| 最終 | 領域 9: swapchain + present + UI 黒画面 | 領域 1-8 完了後、vk-α 達成 marker |
| 最終 | 領域 10: Linux baseline polish | 領域 1-9 完了後、driver matrix 動作確認 |

### 3.5 r41 完遂後の cadence (charter §6.3 (β) + §7.3)

- **r41 達成判定**: 9 件 acceptance criterion 全 PASS で **r41 達成宣言**
- **次 milestone**: r41.5 (07 doc §5.2 outline = VK repo 分離 + Vulkan code abstraction + 法的 review)、r41.5 charter 起草 cadence は **r41 達成宣言直後**
- **memory + handoff cadence**: r41 達成宣言時に `project_ayastorm_r41_vulkan_migration.md` を **closed**、`project_ayastorm_r41_5_*.md` 等 (r41.5 memory) を **active** に、`handoff-r41-complete-r41.5-start.md` を新規作成

---

## 4. 関連 doc / memory

### 関連 doc (r41 章)

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter 本体 (closed 2026-05-28 = 完成)
- `docs/specs/ayastorm-r41-gl-removal/handoff-r41-charter-complete.md` — **本 handoff** (r41 charter 完成 → r41 着手前 prep / 着手の境界)
- (r41 着手後に追加) `01-foundation.md` / `02-portage-execution.md` / `03-shader-port.md` / `04-descriptor-render-pass.md` / `05-skeleton-interface.md` / `06-swapchain-vk-alpha.md` / `07-linux-driver-matrix.md` — charter §7.4 sub-doc 構成 7 件 outline

### 関連 doc (r40 章、本 r41 charter の前置)

- `docs/specs/ayastorm-r40-vulkan-migration/00-charter.md` — r40 章 charter (closed 2026-05-28)
- `docs/specs/ayastorm-r40-vulkan-migration/01-sub-phase-1-cpu-perf.md` — sub-phase 1 (history)
- `docs/specs/ayastorm-r40-vulkan-migration/02-sub-phase-2-extended-falsify.md` — sub-phase 2 (history)
- `docs/specs/ayastorm-r40-vulkan-migration/03-sub-phase-3-vulkan-plan.md` — sub-phase 3 (closed 2026-05-28)
- `docs/specs/ayastorm-r40-vulkan-migration/04-portage-inventory.md` — portage 棚卸し
- `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` — Vulkan API 設計
- `docs/specs/ayastorm-r40-vulkan-migration/06-effort-estimation.md` — 工程算定
- `docs/specs/ayastorm-r40-vulkan-migration/07-r42-plus-milestone-mapping.md` — r42+ milestone mapping
- `docs/specs/ayastorm-r40-vulkan-migration/handoff-work-item-d-foundation-complete.md` — (history)
- `docs/specs/ayastorm-r40-vulkan-migration/handoff-work-item-d-group-a-complete.md` — (history)
- `docs/specs/ayastorm-r40-vulkan-migration/handoff-work-item-d-group-b-complete.md` — (history)
- `docs/specs/ayastorm-r40-vulkan-migration/handoff-work-item-e-complete.md` — (history)
- `docs/specs/ayastorm-r40-vulkan-migration/handoff-r40-close-r41-charter-start.md` — 前 handoff (r40 close → r41 起草着手)

### 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone active (2026-05-28〜)
- `project_ayastorm_r40_cpu_parallel.md` — r40 章 closed 2026-05-28
- `project_ayastorm_r40_extended.md` — r40 sub-phase 2 (history)
- `project_ayastorm_three_platforms.md` — 3 OS 大前提 + Linux 先行例外 (r41 適用)
- `project_ayastorm_release_chapters.md` — release 番号帯 (r30+ 撮影描画章後の Vulkan 化 chapter)
- `feedback_falsification_as_progress.md` — sub-phase 1/2 REJECT/ゼロ の絞り込み成果 (r41 着手 thesis 確立)
- `feedback_proactive_handoff.md` — chapter 境界 handoff (本 handoff で実施)
- `feedback_self_verify_before_handoff.md` — group 完了時 self-trace (r41 着手中 段階別 self-trace で適用)
- `feedback_no_auto_commit.md` — commit は AYA 指示後
- `feedback_release_flow.md` — push は AYA 手動
- `feedback_no_dual_doc_split.md` — docs/specs 同居
- `feedback_build_only_verified.md` — r41 着手中 acceptance satisfy は実機検証
- `feedback_admit_unknown.md` — 仮説 2 連続外れたら log/canary/bisect 切替
- `feedback_use_agents_proactively.md` — r41 着手中の trace / grep / 段階別 audit で Agent 活用
