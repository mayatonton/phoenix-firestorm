# r40 sub-phase 3 handoff — work item (d) group B (§5 charter outline + §6 charter §6 反映) 完了 = work item (d) §1-§6 全 draft 完成 (2026-05-28)

**前 handoff**: `handoff-work-item-d-group-a-complete.md` (foundation §1+§2 + group A §3+§4 完了 cadence)
**本 handoff の位置付け**: work item (d) §1-§6 全 draft 完成 — AYA review PASS で work item (d) 完了宣言 → work item (e) charter 完成 着手 cadence

---

## 1. 完了した work (本 session)

### work item (d) group B (§5 charter outline + §6 charter §6 反映)

`07-r42-plus-milestone-mapping.md` の以下 2 section を draft 化、本 (d) §1-§6 全 draft 完成:

#### §5 各 milestone の charter 草案 outline

| sub-section | 内容 | 出力 |
|---|---|---|
| **§5.0 算定方針 — charter outline 統一 template** | 8 milestone charter 起草の base 8 section 構成 (header + §1 thesis + §2 work breakdown + §3 acceptance + §4 暦月 + §5 依存 + §6 起草 + §7 詳細化 + §8 関連 doc / memory) + 運用方針 (本 (d) outline base / 実 charter = milestone 着手前詳細化 / cadence = 前 milestone 達成宣言直後) | 統一 template 確定 |
| **§5.1 r41 charter outline** | thesis (GL 除去 + Vulkan 空転 = vk-α, Linux first-class baseline) + work breakdown 11 領域 16.17 PM + acceptance criteria 9 件 + 暦月 ~84.1 / ~2033 中 + 依存 (r40 達成) + 起草 cadence (~2026-06) + 詳細化 + 関連 doc / memory 7 件 | r41 charter outline |
| **§5.2 r41.5 charter outline** | thesis (構造 refactor + VK repo 分離 + 法的 review) + work breakdown 4 領域 1.50 PM + acceptance criteria 8 件 (Vulkan abstraction / VK repo / 物理分離 / dynamic link / ビルド統合 / 法的分離 / LL UI defensibility / regression) + 暦月 ~7.2 / ~2034 初 + 詳細化 + 関連 doc / memory 4 件 | r41.5 charter outline |
| **§5.3.1 r42-α (r21.1 picker port)** | thesis (vk-β 着手 + Win 追加開始) + work 0.65 PM + acceptance 6 件 + ~2.9 暦月 / ~2034 前半 | r42-α charter outline |
| **§5.3.2 r42-β (r30 Cinematic mode port)** | thesis (vk-β 完遂 + vk-δ 部分 + Mac 追加開始) + work 3.15 PM + acceptance 7 件 (BD cvar 13 件 visual A/B 含む) + ~13.9 暦月 / ~2035 中 | r42-β charter outline |
| **§5.3.3 r42-γ (r14+ visual realism port)** | thesis (vk-γ + vk-δ 部分) + work 3.18 PM + acceptance 8 件 (scene buffer alpha invariant + sustained A/B 含む) + ~14.0 暦月 / ~2036 後半 | r42-γ charter outline |
| **§5.3.4 r42-δ (parity 残機能 / vk-RC 直前 polish)** | thesis (vk-γ + vk-δ 完遂 + Mac MoltenVK 詳細化開始) + work 2.25 PM + acceptance 6 件 + ~9.9 暦月 / ~2037 中 | r42-δ charter outline |
| **§5.4.1 r43 charter outline** | thesis (Linux baseline 安定維持 + Win parity 完遂) + work 2.63 PM + acceptance 7 件 (Win driver matrix + WHCK + LunarG SDK Win) + ~20 暦月 / ~2038 中 - 2039 初 | r43 charter outline |
| **§5.4.2 r44 charter outline** | thesis (Mac portable subset 詳細化 + Mac parity 完遂 = vk-RC 達成) + work 6.30 PM + acceptance 9 件 (MoltenVK + UMA + MSL + t-noami workflow) + §3.3 vk-RC 10 軸引継ぎ + ~18 暦月 / ~2039 初 - 2040 後半 | r44 charter outline |
| **§5.5 §5 結論** | 全 8 milestone summary table + 起草 cadence 整合 + 累積 ~35.83 PM / ~170 暦月 確認 ✓ | §5 結論 |

#### §6 charter §6 仮 line up の本 §1-§5 反映 (00-charter.md §6 update diff draft)

| sub-section | 内容 | 出力 |
|---|---|---|
| **§6.0 反映方針** | 4 軸 (仮 line up → 正式区分 / a-4 pull-in → mapping / r41.5 cadence / r45+ 末尾追加) + 運用 (本 §6 = diff draft、実 update は work item (e)) | 反映方針確定 |
| **§6.1 仮 line up 表 → 正式区分置換 draft** | charter §6 line 186-195 before / after 提示 (9 行 milestone 表、r41 / r41.5 / r42-α/β/γ/δ / r43 / r44 / vk-RC 達成 marker / r45+ 範囲外) | 軸 1 diff draft |
| **§6.2 a-4 棚卸し AYAstorm 機能 pull-in 順 → mapping 昇格 draft** | charter §6 line 197-205 before / after 提示 (描画 stage × AYAstorm 機能 × milestone 三軸 mapping 表) | 軸 2 diff draft |
| **§6.3 r41.5 milestone section → §5.2 outline cadence 反映 draft** | charter §6 line 207-229 末尾追加 sub-section (r41.5 charter 起草 cadence + 工数 + acceptance criteria + 詳細化) | 軸 3 diff draft |
| **§6.4 r45+ 範囲外 + 別章 charter 起草指針 → 末尾追加 draft** | charter §6 line 230 以降 新規 sub-section (r45+ scope broad outline 5 領域 + 着手 trigger + 起草 cadence + 別 directory pattern) | 軸 4 diff draft |
| **§6.5 §6 結論 (work item (e) charter 完成への引継ぎ)** | 4 軸 diff draft summary + work item (e) で実 update + AYA review pattern A/B/C | §6 結論 |

#### foundation + group A + group B 確定値 summary 更新

`07-r42-plus-milestone-mapping.md` 末尾 summary section に group B (§5 + §6) 確定値 sub-section 追加:

- §5 charter outline 確定値: 8 milestone × outline section + 累積 35.83 PM / ~170 暦月 / 60 件 acceptance criteria + §3.3 vk-RC 10 軸
- §6 charter §6 反映 確定値: 4 軸 diff draft + 反映の運用 (本 §6 = draft / 実 update = work item (e) / AYA review pattern A/B/C)

#### 各 doc の status update

| doc | 更新内容 |
|---|---|
| `07-r42-plus-milestone-mapping.md` line 3 status header | "foundation group + group A draft 完了、group B 残" → "**§1-§6 全 draft 完成** — AYA review PASS で work item (d) 完了宣言 → work item (e) charter 完成 着手" |
| `03-sub-phase-3-vulkan-plan.md` line 3 status header | "work item (d) foundation group + group A draft 完了、group B 残" → "work item (d) **§1-§6 全 draft 完成** 2026-05-28、AYA review PASS で完了予定 → work item (e) charter 完成 着手" |
| `03-sub-phase-3-vulkan-plan.md` §2 work item table row (d) status | "draft 作成中 (foundation group §1+§2 + group A §3+§4 完了、group B §5+§6 残)" → "**§1-§6 全 draft 完成 (2026-05-28)** (foundation group §1+§2 + group A §3+§4 + group B §5+§6 完了、AYA review PASS で完了予定)" |

---

## 2. self-trace (group B 完了時)

### 2.1 numeric integrity

| 検証項目 | 値 | 整合先 | 整合 |
|---|---|---|---|
| §5 8 milestone PM 合計 | 16.17 + 1.50 + 0.65 + 3.15 + 3.18 + 2.25 + 2.63 + 6.30 = 35.83 | 06 doc §4.5 35.84 (丸め誤差 0.01) | ✓ |
| §5 8 milestone 累積暦月 | ~170 | 06 doc §5.6 marker | ✓ |
| §6.1 milestone 表 base PM 列 | r41=16.17 / r41.5=1.50 / r42-α=0.65 / r42-β=3.15 / r42-γ=3.18 / r42-δ=2.25 / r43=2.63 / r44=6.30 | §5.5 8 milestone table | ✓ |
| §6.1 暦月マーカー列 | ~2033 中 / ~2034 初 / ~2034 前半 / ~2035 中 / ~2036 後半 / ~2037 中 / ~2038 中 - 2039 初 / ~2039 初 - 2040 後半 | §5 各 milestone §4 暦月 | ✓ |
| vk-RC 達成 marker | ~2040 年後半 (r44 達成) | charter §4 (3) 想定 15-30 年帯 下方近接 | ✓ |

### 2.2 cross-reference integrity

| 検証項目 | 整合先 | 整合 |
|---|---|---|
| §5.0 統一 template (8 section) | 本 (d) §2.5 acceptance criteria 運用方針 + §5.1-§5.4.2 全 milestone outline で同 8 section 反映 | ✓ |
| §5 各 milestone work breakdown | 本 (d) §2.1-§2.4 (r42-α/β/γ/δ) + §3.1-§3.2 (r43/r44) 内訳 base 継承 | ✓ |
| §5 各 milestone acceptance criteria | 本 (d) §2 / §3 base + 各 milestone outline で 1 対 1 反映 | ✓ |
| §5 起草 cadence (前 milestone 達成宣言直後) | 本 (d) §2.5 cadence 整合 | ✓ |
| §6.1 before / after | charter §6 line 186-195 仮 line up 表 + 本 §1.2 正式区分 反映 | ✓ |
| §6.2 before / after | charter §6 line 197-205 a-4 pull-in 順 + 本 §1.3 三軸 mapping 昇格 | ✓ |
| §6.3 末尾追加 | charter §6 line 207-229 r41.5 milestone section + 本 §5.2 cadence 反映 | ✓ |
| §6.4 末尾追加 | charter §6 line 230 以降 新規 sub-section + 本 §4 r45+ broad outline 反映 | ✓ |
| 起草先 directory pattern | r45+ は `docs/specs/ayastorm-r45-plus-xxx/`、各 milestone は `docs/specs/ayastorm-rXX-xxx/` で統一 | ✓ |

### 2.3 memory cross-reference (引用整合)

| memory | 引用 section | 整合 |
|---|---|---|
| `project_ayastorm_r41_vulkan_migration.md` | §5.1 r41 charter outline 関連 memory | ✓ |
| `project_ayastorm_three_platforms.md` | §5.1 r41 + §5.4.1 r43 関連 memory | ✓ |
| `project_r30_cinematic_control_tuning_deferred.md` | §5.3.2 r42-β (BD cvar 13 件 visual A/B) | ✓ |
| `project_aya_visual_realism_alpha_protect.md` | §5.3.3 r42-γ (scene buffer alpha invariant) | ✓ |
| `project_atmos_atten_scalarized.md` | §5.3.3 r42-γ sky dome | ✓ |
| `feedback_shader_color_space_correction.md` | §5.3.3 r42-γ shader cross compile | ✓ |
| `feedback_visual_decisions_need_live_ab.md` | §5.3.2 / §5.3.3 visual A/B | ✓ |
| `feedback_instant_ab_vs_sustained.md` | §5.3.3 sustained viewing | ✓ |
| `project_pr69_fallback_switch.md` | §5.3.4 r42-δ (audio Vulkan 非依存確認) | ✓ |
| `feedback_credit_t_noami_equal_billing.md` | §5.3.2 / §5.3.3 / §5.3.4 / §5.4.2 Mac 着手 + workflow | ✓ |
| `feedback_mac_only_fixes_accept_as_is.md` | §5.3.4 / §5.4.1 / §5.4.2 Mac MoltenVK + Win 限定 fix | ✓ |
| `feedback_release_notes_link_only.md` | §5.4.2 r44 release note 整備 | ✓ |
| `feedback_release_note_per_feature.md` | §5.4.2 r44 release note 整備 | ✓ |

### 2.4 outline 統一性 check

- 8 milestone 全てが §5.0 統一 template の 8 section 構成 (header + §1 thesis + §2 work breakdown + §3 acceptance + §4 暦月 + §5 依存 + §6 起草 + §7 詳細化 + §8 関連 doc / memory) に従う ✓
- 起草先 directory pattern が r45+ 別 directory pattern と整合 (`docs/specs/ayastorm-rXX-xxx/00-charter.md`) ✓
- 起草 cadence が前 milestone 達成宣言直後で全 milestone 一貫 ✓

### 2.5 §6 diff draft の network check

| 軸 | source (本 (d)) | target (charter §6) | 整合 |
|---|---|---|---|
| §6.1 | §1.2 正式区分 | line 186-195 仮 line up 表 | ✓ |
| §6.2 | §1.3 三軸 mapping | line 197-205 a-4 pull-in 順 | ✓ |
| §6.3 | §5.2 r41.5 outline | line 207-229 r41.5 milestone section | ✓ |
| §6.4 | §4 r45+ broad outline | line 230 以降 (新規 sub-section) | ✓ |

→ 4 軸 update が 00-charter.md §6 を full coverage (line 186 〜末尾)、欠落なし ✓

---

## 3. 次 session の cadence

### 3.1 AYA review 待ち

本 group B 完了 = work item (d) §1-§6 全 draft 完成。AYA review pattern:

- **Pattern A**: 本 §5 + §6 draft そのまま OK → work item (d) 完了宣言 → work item (e) charter 完成 着手
- **Pattern B**: 本 §5 + §6 一部修正 → 修正後 work item (d) 完了宣言 → work item (e) charter 完成 着手
- **Pattern C**: 本 §5 + §6 大幅変更 → 影響範囲確認 (本 §1-§4 反映 / 03 doc / 07 doc) → 修正後 work item (d) 着手判定

### 3.2 Pattern A の場合 (本 (d) 完了宣言 → work item (e) charter 完成 着手)

#### work item (e) charter 完成 の作業内容

1. **00-charter.md §6 実 update** (本 §6 draft の 4 軸を 00-charter.md §6 に edit 反映):
    - §6.1: line 186-195 仮 line up 表 → 正式区分 表 (9 行 milestone + base PM + 暦月 + 描画 stage + AYAstorm 機能 + repo 構成) 置換
    - §6.2: line 197-205 a-4 棚卸し pull-in 順 → 三軸 mapping 表 昇格
    - §6.3: line 207-229 r41.5 milestone section 末尾に「r41.5 charter 起草 cadence」 sub-section 追加
    - §6.4: line 230 以降 末尾に「r45+ 範囲外 + 別章 charter 起草指針」 sub-section 追加
2. **03 doc 最終 review**: work item table の (a)-(d) 全完了確認 + (e) 着手 status update
3. **04/05/06/07 doc final review**: 整合 + TBD / 仮値の置換 + 各 doc status header 完了表記
4. **00-charter.md 他 section 整合確認**: §3 時間軸 / §4 (1) parity 完遂 goal / §7 LL 着地判断指針 / §8 plan B trigger / §9 work item phase / §10 関連 doc 等の整合 + 本 (d) 結論反映
5. **memory `project_ayastorm_r40_cpu_parallel.md`**: r40 達成 status に更新 (group A handoff で予告済の cadence)

#### r40 達成宣言の条件

- 上記 doc 群が AYA さん review で全 PASS
- AYA さんが明示的に「r40 達成」と宣言
- → r41 charter (`docs/specs/ayastorm-r41-gl-removal/00-charter.md`) 起草へ移行 (本 §5.1 r41 charter outline base)

### 3.3 Pattern B/C の場合

- Pattern B: 修正範囲を確認 → 該当 section の draft を update → AYA 再 review
- Pattern C: 影響範囲確認 (本 §1-§4 反映可否 + 03 doc + 07 doc) → 必要なら group B 再 draft → AYA 再 review

---

## 4. 関連 doc / memory

### 関連 doc

- `00-charter.md` — r40 章 charter (§6 仮 line up + §7/§8 判断指針、本 (d) §6 で diff draft 提示済 = work item (e) で実 update)
- `03-sub-phase-3-vulkan-plan.md` — work item (d) 親 doc (本 handoff で status update 反映)
- `04-portage-inventory.md` — work item (a) 完了 (§5.4 段階 port + §6.3.2 AYAstorm 機能 pull-in 順、本 (d) §1.2/§1.4 input)
- `05-vulkan-api-design.md` — work item (b) 完了 (§10 skeleton + §3 descriptor + §4 render pass + §8 OS 別 + §9.4 MoltenVK、本 (d) §1.4/§2 input)
- `06-effort-estimation.md` — work item (c) 完了 (§3 milestone 別工数 + §4.4 OS 着手 timing + §5.6 marker 暦年、本 (d) §1.2/§2 input)
- `07-r42-plus-milestone-mapping.md` — work item (d) 本 doc (§1-§6 全 draft 完成、本 handoff の対象)
- `handoff-work-item-d-foundation-complete.md` — foundation group cadence handoff (historical)
- `handoff-work-item-d-group-a-complete.md` — group A cadence handoff (historical)
- `handoff-work-item-d-group-b-complete.md` — **本 handoff** (group B 完了 = work item (d) §1-§6 全 draft 完成 cadence)

### 関連 memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory (work item (e) で r40 達成 status に更新予定)
- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (§5.1 charter outline base、r41 charter 起草で詳細化)
- `feedback_proactive_handoff.md` — group 境界 handoff (本 group B 完了で次 session への handoff)
- `feedback_self_verify_before_handoff.md` — group 完了時 self-trace (本 handoff §2 self-trace で実施)
- `feedback_no_auto_commit.md` — commit は AYA 指示後 (本 handoff 後の commit cadence)
- `feedback_no_dual_doc_split.md` — 内部/公開 doc を分けない (本 handoff も docs/specs に同居)
