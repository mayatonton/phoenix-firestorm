# r40 章 close → r41 charter 起草着手 chapter 境界 handoff (2026-05-28)

**前 handoff**: `handoff-work-item-e-complete.md` (work item (e) 5 sub-step 全完了 cadence、draft 完成時点執筆)
**本 handoff の位置付け**: r40 章 close 後の最終 state snapshot + r41 charter 起草着手のための base 資料 + 次 session 開始 action cadence

---

## 1. r40 章 close 確定 state (2026-05-28)

### 1.1 達成宣言

- AYA 「review OK」 (Pattern P 「review OK = r40 達成宣言を兼ねる」確定 2026-05-28)
- work item (a)-(e) 全完了 + AYA review PASS
- r40 達成 = 工程プラン (設計および工程想定) 完成

### 1.2 status close

| doc / memory | status |
|---|---|
| `00-charter.md` L3 | **closed 2026-05-28** (work item (a)-(e) 全完了 + AYA review PASS、r40 達成 = 工程プラン完成、r41 charter 起草へ移行) |
| `03-sub-phase-3-vulkan-plan.md` L1/L3 | **closed 2026-05-28** (work item (a)-(e) 全完了 + AYA review PASS、sub-phase 3 完了 = r40 章 close = 工程プラン完成) |
| memory `project_ayastorm_r40_cpu_parallel.md` | **closed 2026-05-28** (r40 達成 = 工程プラン完成、次は r41 charter 起草) |

### 1.3 commit / push 反映

- commit: `6bb24375db docs(r40): r40 達成 (工程プラン完成) + work item (e) charter 完成 全 5 sub-step + handoff doc`
- diff: 8 file (7 modified + 1 new = handoff-work-item-e-complete.md) / +307 -46
- push: origin/feature/ayastorm-r40-vulkan-migration `656f4ca754..6bb24375db` 反映済 (2026-05-28)
- branch: `feature/ayastorm-r40-vulkan-migration` (HEAD 同期済)

### 1.4 r40 章成果物 (8 doc + 1 memory + 4 handoff)

#### 工程プラン doc 8 件 (`docs/specs/ayastorm-r40-vulkan-migration/`)

| doc | 役割 |
|---|---|
| `00-charter.md` | r40 章 charter 本体 (closed 2026-05-28) |
| `01-sub-phase-1-cpu-perf.md` | sub-phase 1 CPU perf 全 REJECT 記録 |
| `02-sub-phase-2-extended-falsify.md` | sub-phase 2 鉱脈ゼロ falsify 記録 |
| `03-sub-phase-3-vulkan-plan.md` | sub-phase 3 Vulkan 化選択 + 工程プラン (closed 2026-05-28) |
| `04-portage-inventory.md` | work item (a) Vulkan portage 棚卸し |
| `05-vulkan-api-design.md` | work item (b) Vulkan API 設計 |
| `06-effort-estimation.md` | work item (c) 工程算定 (累積 35.83 PM / ~170 暦月 / ~14.2 年) |
| `07-r42-plus-milestone-mapping.md` | work item (d) r42+ 区切り確定 + r41-r44 charter outline 8 件 |

#### memory 1 件

- `project_ayastorm_r40_cpu_parallel.md` (closed 2026-05-28)

#### handoff doc 4 件 (historical + 本 handoff)

| handoff | 完了範囲 |
|---|---|
| `handoff-work-item-d-foundation-complete.md` | (d) foundation §1+§2 |
| `handoff-work-item-d-group-a-complete.md` | (d) group A §3+§4 |
| `handoff-work-item-d-group-b-complete.md` | (d) group B §5+§6 全 §1-§6 draft 完成 |
| `handoff-work-item-e-complete.md` | (e) 5 sub-step 全完了 (draft 完成時点執筆) |
| `handoff-r40-close-r41-charter-start.md` | **本 handoff** (r40 close → r41 起草着手の境界) |

---

## 2. r41 charter 起草の base 資料

### 2.1 起草 base = `07-r42-plus-milestone-mapping.md` §5.1 r41 charter outline

r41 charter 起草の base は 07 doc §5.1。8 section 構成 (統一 template §5.0):

| section | 内容 |
|---|---|
| header | doc title / status / 親 charter / 達成条件 |
| §1 thesis | r41 milestone の本質 (GL 除去 + Vulkan 空転 = vk-α、Linux first-class baseline 確立) |
| §2 work breakdown | 11 領域 16.17 PM (foundation 12.92 + 追加 8.15 のうち r41 配分 + 余裕係数 +40%) |
| §3 acceptance criteria | 9 件 (GL header 完全除去 / Vulkan instance/device 起動 / swapchain 描画 / 7 pass chain 空転 / 248 shader SPIR-V load / Linux Mesa+NVIDIA 動作 / regression / memory leak / 1 か月 sustained) |
| §4 暦月 | base 16.17 PM × 本職並走 4x × 学習曲線 +30% = ~84.1 暦月 (~7 年、~2033 中達成) |
| §5 依存 | r40 達成 (本 session で確定) |
| §6 起草 cadence | r40 達成宣言直後 (= 2026-05-28 直後、本次 session) |
| §7 詳細化方針 | 11 領域 sub-doc 構成 (portage / API / shader / state machine / pipeline / Linux WSI 等を r41 scope で再整理) |
| §8 関連 doc / memory | r40 章 doc 8 件 + r41 memory + 関連 feedback |

### 2.2 起草先 directory + filename pattern

- directory: `docs/specs/ayastorm-r41-gl-removal/` (新規作成)
- main charter: `docs/specs/ayastorm-r41-gl-removal/00-charter.md`
- sub-doc 構成 (起草中に随時追加): `01-foundation.md` / `02-portage-execution.md` / `03-api-skeleton.md` / `04-shader-port.md` 等 (r41 scope で再整理)

### 2.3 関連 doc / memory cross-reference

| doc / memory | r41 起草での参照 |
|---|---|
| `04-portage-inventory.md` | §5.4 段階 port 戦略 5 段階 + a-3 §B.x AYAstorm 機能 pull-in 順 |
| `05-vulkan-api-design.md` | §3 descriptor / §4 render pass / §5 sync / §6 memory / §7 swapchain / §8 OS 別 / §9 extension / §10 abstraction skeleton |
| `06-effort-estimation.md` | §1 per-file / §2 per-shader / §3 milestone 別 (r41 = 16.17 PM) / §5 暦月変換 / §6 uncertainty band |
| `07-r42-plus-milestone-mapping.md` §5.1 | r41 charter outline 8 section base |
| memory `project_ayastorm_r41_vulkan_migration.md` | r41 milestone memory (charter 起草で詳細化) |
| memory `project_ayastorm_r40_cpu_parallel.md` | r40 章 close + r41 移行明記 (history reference) |
| memory `project_ayastorm_three_platforms.md` | Linux first-class baseline + Win/Mac 後追い (charter §4 (2) 明示指示要件) |
| feedback `feedback_self_verify_before_handoff.md` | 起草中 group 完了時 self-trace |
| feedback `feedback_proactive_handoff.md` | group 境界 handoff |
| feedback `feedback_no_auto_commit.md` | commit は AYA 指示後 |
| feedback `feedback_release_flow.md` | push は AYA 手動 |

---

## 3. 次 session 開始 action cadence

### 3.1 次 session 開始時の最初の action

1. **handoff 確認**: 本 handoff (`handoff-r40-close-r41-charter-start.md`) を Read
2. **base 資料確認**: 07 doc §5.1 r41 charter outline を Read
3. **memory 確認**: `project_ayastorm_r41_vulkan_migration.md` + `project_ayastorm_r40_cpu_parallel.md` で r41 milestone の thesis + r40 章 close state を確認
4. **AYA cadence 確認**: r41 charter 起草の進め方を AYA さんに確認 (Pattern α 一括 draft / Pattern β group 分割 / Pattern γ section 単位)
5. **directory + 00-charter.md 着手**: `docs/specs/ayastorm-r41-gl-removal/` 作成 + `00-charter.md` 起草開始

### 3.2 r41 charter 起草の cadence 候補

- **Pattern α (一括 draft)**: 8 section 全部を 1 session で draft、AYA review 受けて修正
- **Pattern β (group 分割)**: foundation (header + §1 thesis + §2 work breakdown) → group A (§3 acceptance + §4 暦月 + §5 依存) → group B (§6 起草 cadence + §7 詳細化方針 + §8 関連 doc) 各 group 完了時 handoff
- **Pattern γ (section 単位)**: 1 section ずつ AYA review、最も丁寧だが session 数が多い

cadence 選好は次 session 開始時に AYA さんと擦り合わせ。本 session の r40 章 work item (c)(d)(e) は Pattern β (group 分割) で進めた実績あり。

### 3.3 r41 起草中の注意事項 (feedback + memory 反映)

- **Linux first-class baseline 厳守**: charter §4 (2) 「Linux 先行 → Win/Mac 後追い」明示指示要件、memory `project_ayastorm_three_platforms.md` の「3 OS 揃える、Linux のみ判断は明示指示が無い限り取らない」の例外として r41 scope で適用
- **r41 達成 = GL 完全除去 + Vulkan 空転 (描画最低限)**: r42-α 以降の AYAstorm 機能 port は scope 外
- **工数は base 16.17 PM (06 doc §3.1)**: 本職並走 4x + 学習曲線 +30% で ~7 年見込み (~2033 中)
- **acceptance criteria 9 件**: 07 doc §5.1.3 から 1 対 1 反映
- **plan B trigger**: r41 中央値 r41 = ~84 暦月 × 1.30 = 109 暦月 (~9 年) で warning、上方 band 上限 ~184 暦月 (~15 年) で trigger 発動 (06 doc §8.3)
- **commit cadence**: `feedback_no_auto_commit.md` 遵守、AYA 明示指示で commit
- **push cadence**: `feedback_release_flow.md` 遵守、push は AYA 手動 (本 session で push は AYA 指示後 Claude 実行で履歴あり)

---

## 4. 関連 doc / memory

### 関連 doc (r40 章)

- `00-charter.md` — r40 章 charter (closed 2026-05-28)
- `01-sub-phase-1-cpu-perf.md` — sub-phase 1 (history)
- `02-sub-phase-2-extended-falsify.md` — sub-phase 2 (history)
- `03-sub-phase-3-vulkan-plan.md` — sub-phase 3 (closed 2026-05-28)
- `04-portage-inventory.md` — work item (a)
- `05-vulkan-api-design.md` — work item (b)
- `06-effort-estimation.md` — work item (c)
- `07-r42-plus-milestone-mapping.md` — work item (d) (r41 起草の base = §5.1)
- `handoff-work-item-d-foundation-complete.md` — (history)
- `handoff-work-item-d-group-a-complete.md` — (history)
- `handoff-work-item-d-group-b-complete.md` — (history)
- `handoff-work-item-e-complete.md` — (history)
- `handoff-r40-close-r41-charter-start.md` — **本 handoff**

### 関連 doc (r41 章、起草で新設)

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter (本次 session で起草着手)
- `docs/specs/ayastorm-r41-gl-removal/` 配下 sub-doc — 起草中に随時追加

### 関連 memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory (closed 2026-05-28)
- `project_ayastorm_r40_extended.md` — sub-phase 2 詳細 (history)
- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone memory (charter 起草で詳細化)
- `project_ayastorm_three_platforms.md` — Linux first-class baseline + Win/Mac 後追い (r41 scope で適用)
- `feedback_falsification_as_progress.md` — sub-phase 1/2 REJECT/ゼロ の絞り込み成果
- `feedback_proactive_handoff.md` — chapter 境界 handoff (本 handoff で実施)
- `feedback_self_verify_before_handoff.md` — group 完了時 self-trace (r41 起草でも適用)
- `feedback_no_auto_commit.md` — commit は AYA 指示後
- `feedback_release_flow.md` — push は AYA 手動
- `feedback_no_dual_doc_split.md` — docs/specs 同居
