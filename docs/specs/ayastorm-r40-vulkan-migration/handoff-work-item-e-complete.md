# r40 sub-phase 3 handoff — work item (e) charter 完成 全 5 sub-step 完了 (2026-05-28)

**前 handoff**: `handoff-work-item-d-group-b-complete.md` (work item (d) §1-§6 全 draft 完成 + AYA review 待ち)
**本 handoff の位置付け**: work item (d) AYA review PASS → work item (e) charter 完成 着手 → Pattern B (5 sub-step 連続実行) で全 sub-step 完了 → AYA「r40 達成」明示宣言待ち

---

## 1. 完了した work (本 session)

### 1.1 work item (d) AYA review PASS 反映

- AYA review 結果: PASS
- `07-r42-plus-milestone-mapping.md` status: **確定 (AYA review PASS 2026-05-28)**
- `03-sub-phase-3-vulkan-plan.md` work item table (d) → **完了 (2026-05-28)** 全 §1-§6 + AYA review PASS

### 1.2 work item (e) charter 完成 5 sub-step 連続実行 (Pattern B)

#### (e)-1 charter §6 update (00-charter.md L180-276 範囲)

`00-charter.md` §6 を `07-r42-plus-milestone-mapping.md` §6 diff draft 4 軸に基づき実 update:

| 軸 | update 内容 |
|---|---|
| 1 | 旧 仮 line up table (9 行) → **正式 line up table 11 row** に置換 (r41 / r41.5 / r42-α / r42-β / r42-γ / r42-δ / r43 / r44 / vk-RC 達成 marker / r45+ 範囲外 + base PM + 暦月 + 描画 stage + AYAstorm 機能 + repo 構成 列) |
| 2 | 旧 a-4 棚卸し pull-in 順 bullets → **三軸 mapping 4 row table** (描画 stage × AYAstorm 機能 × OS) に昇格 |
| 3 | **r41.5 charter 起草 cadence sub-section** を末尾追加 (r41 達成宣言直後 起草 / `docs/specs/ayastorm-r41-5-vk-repo-split/00-charter.md` directory / acceptance 8 件 等) |
| 4 | **r45+ 範囲外 + 別章 charter 起草指針 section** を末尾追加 (broad outline 5 領域 + 着手 trigger + 起草 cadence + 別 directory pattern `docs/specs/ayastorm-r45-plus-xxx/`) |

#### (e)-2 03 doc review

`03-sub-phase-3-vulkan-plan.md`:

- L3 status: 「work item (d) 完了 (AYA review PASS 2026-05-28)、work item (e) charter 完成 着手中」
- §2 work item table:
  - (d) → **完了 (AYA review PASS 2026-05-28)** 全 §1-§6
  - (e) → **着手中** with sub-step progress ((e)-1 / (e)-2 / (e)-3 / (e)-4 完了 + (e)-5 進行中)
- §10 doc list: 4 完了 doc に「完了 2026-05-28」 marker + 07 doc 追加 + handoff doc refs 追加

#### (e)-3 04/05/06/07 doc cross check + status header 完了表記

| doc | 更新内容 |
|---|---|
| `04-portage-inventory.md` L3 | "work item (a) 完了 (AYA review PASS 2026-05-28)" |
| `05-vulkan-api-design.md` L3 | "work item (b) 完了 (AYA review PASS 2026-05-28)" (旧 "AYA review 待ち") |
| `06-effort-estimation.md` L3 | status reorganized (work item (c) 完了 + work item (d) 完了 + work item (e) 着手中) |
| `07-r42-plus-milestone-mapping.md` L3 | "work item (d) 完了 (AYA review PASS 2026-05-28)" + "(e)-1 charter §6 実 update 実施済" note |

#### (e)-4 charter 他 section update

`00-charter.md` 他 section 整合 + 仮表記除去:

| 場所 | 更新内容 |
|---|---|
| L3 status | "(a)(b)(c)(d) 完了 2026-05-28、work item (e) charter 完成 着手中" |
| L39 / L47 / L56 | 「(起草予定)」 marker 除去 |
| §4 (8) table | "Vulkan portage 棚卸し後 TBD" → "work item (d) で確定済 2026-05-28" |
| §4 (8) header | "棚卸し後 TBD" → "work item (d) で確定済" |
| §4 (8) 含意 | 07 doc 参照に更新 |
| §7 判断軸 1 table | 5 row → **6 row + 注** に再構成 (新 milestone mapping 反映 / r43 = vk-RC 進行中 = Win parity 完遂 / r44 = vk-RC 達成 = Mac parity 完遂) |
| §9 work item example list | 07 doc を追加 |
| §10 関連 doc list | 「(起草予定)」 marker 除去 + 07 doc + handoff doc refs 追加 |

`02-sub-phase-2-extended-falsify.md`:

- L150: "(起草予定、active phase)" → "(active 2026-05-28〜)"

#### (e)-5 memory update

`project_ayastorm_r40_cpu_parallel.md` を以下で update:

- work item (d) group A 完了 entry 追加 (§3 r43-r44 + §4 r45+ 範囲外)
- work item (d) group B 完了 entry 追加 (§5 描画 stage 統合 + §6 cadence prep)
- work item (d) **完了宣言** entry 追加 (AYA review PASS 2026-05-28)
- work item (e) charter 完成 着手中 entry 追加 (5 sub-step progress)
- r40 達成セクション update (「AYA 明示宣言で確定」と明示、自動的に r40 達成 mark しない)
- 「関連 doc (策定後作成予定)」 → 「関連 doc」 に rename + 全 8 doc 列挙 + 完了 marker

---

## 2. self-trace (work item (e) 完了時)

### 2.1 sub-step coverage

| sub-step | 範囲 | 完了 |
|---|---|---|
| (e)-1 | 00-charter.md §6 (L180-276) 4 軸 update | ✓ |
| (e)-2 | 03-sub-phase-3-vulkan-plan.md status / work item table / doc list | ✓ |
| (e)-3 | 04/05/06/07 doc status header 完了表記 | ✓ |
| (e)-4 | 00-charter.md L3 / L39/L47/L56 / §4 (8) / §7 判断軸 1 / §9 / §10 + 02 doc L150 | ✓ |
| (e)-5 | memory `project_ayastorm_r40_cpu_parallel.md` work item (d) group A/B/完了宣言 + (e) 着手中 + 関連 doc | ✓ |

### 2.2 cross-reference integrity

| 検証項目 | 整合先 | 整合 |
|---|---|---|
| 00-charter.md §6 正式 line up table | 07 doc §1.2 + §5.5 + §6.1 | ✓ |
| 00-charter.md §6 三軸 mapping table | 07 doc §1.3 + §6.2 | ✓ |
| 00-charter.md §6 r41.5 charter 起草 cadence | 07 doc §5.2 + §6.3 | ✓ |
| 00-charter.md §6 r45+ 範囲外 + 別章 charter 起草指針 | 07 doc §4 + §6.4 | ✓ |
| 00-charter.md §4 (8) → 07 doc 参照 | 07 doc 全体 | ✓ |
| 00-charter.md §7 判断軸 1 (6 row 新版) | 07 doc §1.2 milestone mapping (r43 / r44 / vk-RC 達成 marker) | ✓ |
| 03 doc §2 work item table (a)-(d) 完了 / (e) 着手中 | 04/05/06/07 doc status header + memory entry | ✓ |
| memory entry の暦月 / PM 数値 | 07 doc + 06 doc | ✓ |
| memory 関連 doc 8 件列挙 | 実 directory `docs/specs/ayastorm-r40-vulkan-migration/` | ✓ |

### 2.3 r40 達成宣言の条件保護

- memory `project_ayastorm_r40_cpu_parallel.md` の r40 達成セクションは「AYA 明示宣言で確定」と記述、自動的に達成 mark しない ✓
- handoff §3 で AYA に明示宣言を依頼 ✓
- feedback `feedback_no_auto_commit.md` (commit は AYA 指示後) 遵守 ✓

### 2.4 doc 一貫性 (status header 横断)

| doc | status header | (a)-(e) 状態 | 整合 |
|---|---|---|---|
| 00-charter.md L3 | (a)(b)(c)(d) 完了 + (e) 着手中 | 一致 | ✓ |
| 03-sub-phase-3-vulkan-plan.md L3 | (d) 完了 + (e) 着手中 | 一致 | ✓ |
| 04-portage-inventory.md L3 | (a) 完了 | 一致 | ✓ |
| 05-vulkan-api-design.md L3 | (b) 完了 | 一致 | ✓ |
| 06-effort-estimation.md L3 | (c) 完了 + (d) 完了 + (e) 着手中 | 一致 | ✓ |
| 07-r42-plus-milestone-mapping.md L3 | (d) 完了 + (e)-1 実施済 | 一致 | ✓ |

### 2.5 memory cross-reference

| memory | 引用 / 関連 section | 整合 |
|---|---|---|
| `project_ayastorm_r40_cpu_parallel.md` | r40 章 active memory (本 handoff で update) | ✓ |
| `project_ayastorm_r40_extended.md` | sub-phase 2 履歴 (本 handoff 範囲外) | ✓ |
| `project_ayastorm_r41_vulkan_migration.md` | r41 milestone (charter 起草で詳細化) | ✓ |
| `feedback_falsification_as_progress.md` | sub-phase 1/2 REJECT/ゼロ の意味 | ✓ |
| `feedback_no_auto_commit.md` | commit cadence (本 handoff 後の AYA 指示待ち) | ✓ |
| `feedback_proactive_handoff.md` | work item 境界 handoff (本 handoff で実施) | ✓ |
| `feedback_self_verify_before_handoff.md` | self-trace (本 §2) | ✓ |

---

## 3. 次 session の cadence

### 3.1 AYA review 待ち (work item (e) PASS / Fail)

本 work item (e) 5 sub-step 全完了 = **r40 章 工程プラン doc 完成状態**。AYA review pattern:

- **Pattern A**: 5 sub-step 全 OK → AYA が明示的に「r40 達成」宣言 → r40 章 close → r41 charter 起草へ移行
- **Pattern B**: 一部修正 → 修正後 「r40 達成」 再判定
- **Pattern C**: charter §6 / §7 / §4 (8) のいずれかで大幅変更 → 影響範囲確認 (03/04/05/06/07 doc + memory) → 修正後 「r40 達成」 再判定

### 3.2 Pattern A の場合 (r40 達成宣言 → r41 charter 起草へ)

#### r40 達成宣言の手順

1. AYA さんが明示的に「r40 達成」と宣言
2. 以下を 1 セッションで実施:
   - memory `project_ayastorm_r40_cpu_parallel.md`: status を **closed / 工程プラン完成** に更新 (達成日 2026-05-28)
   - 00-charter.md L3 status: 「**closed 2026-05-28** (工程プラン完成、r41 charter 起草へ移行)」
   - 03-sub-phase-3-vulkan-plan.md L3 status: 「**closed 2026-05-28** (sub-phase 3 完了 = r40 章 close)」
3. r41 charter (`docs/specs/ayastorm-r41-gl-removal/00-charter.md`) 起草着手 (07 doc §5.1 r41 charter outline を base)

#### r41 charter 起草の cadence (参考、r40 達成後)

- §5.1 (07 doc) を outline base に 8 section 構成 (header + §1 thesis + §2 work breakdown + §3 acceptance + §4 暦月 + §5 依存 + §6 起草 + §7 詳細化 + §8 関連 doc / memory)
- 別 directory `docs/specs/ayastorm-r41-gl-removal/` で sub-doc 構成 (04/05 doc に類する portage / API 詳細を r41 scope で再整理)
- 起草 cadence は 2026-06 想定 (07 doc §5.1 起草 cadence)

### 3.3 Pattern B / C の場合

- Pattern B: AYA 指摘の section を修正 → 整合先 doc を update → AYA 再 review
- Pattern C: 影響範囲確認 (本 (e) 5 sub-step + 関連 doc 4-5 件) → 必要なら work item (d) 再 draft → AYA 再 review

### 3.4 commit cadence (本 handoff 後)

`feedback_no_auto_commit.md` 遵守、AYA 指示待ち。commit 候補は以下 3 pattern:

- **Pattern α (一括 commit)**: 本 (e) 5 sub-step + 本 handoff doc を 1 commit に bundle、「docs(r40): work item (e) charter 完成 全 5 sub-step + handoff doc」
- **Pattern β (sub-step 別 commit)**: (e)-1 / (e)-2 / (e)-3 / (e)-4 / (e)-5 を別 commit に分割、最後に handoff doc commit
- **Pattern γ (r40 達成宣言後 1 commit)**: AYA 「r40 達成」明示宣言を受けて、status close update + 本 (e) bundle で 1 commit、「docs(r40): r40 達成 (工程プラン完成) + work item (e) 全 5 sub-step + handoff doc」

cadence 選好は AYA 指示で確定。

---

## 4. 関連 doc / memory

### 関連 doc

- `00-charter.md` — r40 章 charter (本 handoff で §6 + 他 section update 反映済、AYA review 待ち)
- `01-sub-phase-1-cpu-perf.md` — sub-phase 1 CPU perf 全 REJECT 記録 (history)
- `02-sub-phase-2-extended-falsify.md` — sub-phase 2 鉱脈ゼロ falsify 記録 (本 handoff で L150 marker 除去)
- `03-sub-phase-3-vulkan-plan.md` — sub-phase 3 active doc (本 handoff で status / work item table / doc list update)
- `04-portage-inventory.md` — work item (a) 完了 doc (本 handoff で status header 完了表記)
- `05-vulkan-api-design.md` — work item (b) 完了 doc (本 handoff で status header 完了表記)
- `06-effort-estimation.md` — work item (c) 完了 doc (本 handoff で status reorganized)
- `07-r42-plus-milestone-mapping.md` — work item (d) 完了 doc (本 handoff で status / (e)-1 実施済 note)
- `handoff-work-item-d-foundation-complete.md` — work item (d) foundation group cadence handoff (historical)
- `handoff-work-item-d-group-a-complete.md` — work item (d) group A cadence handoff (historical)
- `handoff-work-item-d-group-b-complete.md` — work item (d) group B 完了 cadence handoff (historical)
- `handoff-work-item-e-complete.md` — **本 handoff** (work item (e) 5 sub-step 全完了 cadence)

### 関連 memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory (本 handoff で (d) group A/B/完了宣言 + (e) 着手中 + 関連 doc update)
- `project_ayastorm_r40_extended.md` — sub-phase 2 詳細 memory (履歴)
- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (r40 達成後の次 chapter)
- `feedback_falsification_as_progress.md` — sub-phase 1/2 REJECT/ゼロ の絞り込み成果
- `feedback_proactive_handoff.md` — work item 境界 handoff (本 handoff で実施)
- `feedback_self_verify_before_handoff.md` — handoff 前 self-trace (本 §2 実施)
- `feedback_no_auto_commit.md` — commit は AYA 指示後 (本 §3.4 cadence)
- `feedback_no_dual_doc_split.md` — 内部/公開 doc を分けない (本 handoff も docs/specs に同居)
