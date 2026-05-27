# handoff: r40 sub-phase 3 work item (d) r42+ 区切り確定 — foundation group 完了 (§1 r42 区切り algorithm 化 + §2 r42 milestone 内訳 draft 完成)

**作成日**: 2026-05-28
**前 session 状況**: work item (c) 工程算定 全 §1-§8 draft 完成 (前 session, commit c4aa2e3f2d) → AYA review PASS (本 session、Pattern A 「§7/§8 は先のことなので現時点で予見できない」OK 扱い) → **work item (c) 完了宣言 + work item (d) 着手** = 本 session で foundation group (§1 + §2) draft 完了
**branch**: `feature/ayastorm-r40-vulkan-migration`
**親 doc**: `03-sub-phase-3-vulkan-plan.md` work item (d)

---

## 1. 本 session で完了したもの

### 1.1 work item (c) 完了宣言反映 (03 doc + 06 doc)

- `03-sub-phase-3-vulkan-plan.md` header: work item (c) 工程算定 **完了** 2026-05-28 AYA review PASS / work item (d) r42+ 区切り確定 **draft 作成中** に更新
- `03-sub-phase-3-vulkan-plan.md` work item 表: (c) 完了 (2026-05-28) + (d) draft 作成中 + 出力 doc `07-r42-plus-milestone-mapping.md` 追加
- `06-effort-estimation.md` status: **確定 (AYA review PASS 2026-05-28)** + 達成条件 達成 (2026-05-28) に更新

### 1.2 work item (d) foundation group draft 完成 (07-r42-plus-milestone-mapping.md 新規作成)

| section | 内容 | 結論 |
|---|---|---|
| header + 算定方針 | status / 親 doc / 前置 doc / 達成条件 + 区切り 3 軸 (描画 stage / AYAstorm 機能 / 構造 refactor) + 算定 source map | — |
| §1.0 mapping 方針 | charter §6 仮 line up r42 単一 → r42-α/β/γ/δ 4 sub-milestone 細分化、3 軸 align (機能 pull-in + 描画 stage + OS 着手) | — |
| §1.1 charter §6 仮 line up の現状と limitation | charter §6 仮 line up 表 + 4 件 limitation (粗すぎる / AYAstorm pull-in 未確定 / r43-r45+ 意味整理必要 / r41.5 sub-milestone level 順序確定必要) | charter §6 仮 line up を本 (d) で update する根拠 |
| §1.2 06 doc 算定区切りからの正式区分 | r41 / r41.5 / r42-α/β/γ/δ / r43-r44 / vk-RC = r44 達成、各 base PM + 中央値暦月 + 暦年 marker | **正式区分 ✓** |
| §1.3 描画 stage × AYAstorm 機能 × milestone 三軸 mapping | r42-α = vk-β 着手 / r42-β = vk-β 完遂 + vk-δ 部分 / r42-γ = vk-γ + vk-δ 部分 / r42-δ = vk-γ + vk-δ 完遂 / r43-r44 = vk-RC | charter §6 仮 line up より r42 内で大半 stage 進行 |
| §1.4 05 doc §10 skeleton + 04 doc §5.4 段階 port 戦略 との時系列整合 | 05 doc §10 hooks の r41 → r41.5 → r42-α 遷移 + 04 doc §5.4 段階 1-5 = r41 範囲、§B.1/B.2/B.3 = r42-α/β/γ | 時系列整合 ✓ |
| §1.5 r45+ 範囲外宣言 | vk-RC 達成 = r44 達成 = 本算定終了 marker、r45+ は別章 charter、charter §3 時間軸非設定遵守 | r45+ 範囲外 ✓ |
| §1.6 §1 結論 | 6 件 algorithm 確定 (細分化 / 機能 mapping / stage mapping / 時系列整合 / vk-RC marker / r45+ 範囲外) | r42 区切り algorithm 確定 ✓ |
| §2.0 算定方針 | r42-α/β/γ/δ 4 sub-milestone の内訳 (目的 + work breakdown + acceptance criteria draft + OS 着手 timing + 依存 milestone) | — |
| §2.1 r42-α (r21.1 picker port) | 目的 = vk-β 着手 + render pass attachment 統合、work breakdown ~0.65 PM 内訳、acceptance 6 件 draft、Win 追加開始 | r42-α 内訳確定 ✓ |
| §2.2 r42-β (r30 Cinematic mode port) | 目的 = vk-β 完遂 + DoF state enum 化、work breakdown ~3.15 PM 内訳、acceptance 7 件 draft (BD cvar 13 件 visual A/B 含む)、Mac 追加開始 | r42-β 内訳確定 ✓ |
| §2.3 r42-γ (r14+ visual realism port) | 目的 = vk-γ + vk-δ 部分 + sky dome + post-process chain、work breakdown ~3.18 PM 内訳、acceptance 8 件 draft (scene buffer alpha invariant + sustained A/B 含む)、Win/Mac 並走 | r42-γ 内訳確定 ✓ |
| §2.4 r42-δ (parity 残機能 / vk-RC 直前 polish) | 目的 = vk-γ + vk-δ 完遂 + parity 残機能 + r25-r29 3D stream + r1-r13 audio 確認、work breakdown ~2.25 PM 内訳、acceptance 6 件 draft、Mac MoltenVK 詳細化 | r42-δ 内訳確定 ✓ |
| §2.5 acceptance criteria draft の運用方針 | draft → charter 詳細化 cadence (各 milestone 着手前に別 charter 起草) + 詳細化主体 (AYA + Claude) + draft が確定値ではない理由 | acceptance criteria 運用方針確定 ✓ |
| §2.6 §2 結論 | r42 合計 ~9.23 PM / ~40.7 暦月 / ~3.4 年 (06 doc §3.9 + §5.3 整合 ✓)、acceptance 27 件 draft | r42 内訳確定 ✓ |
| §3-§6 placeholder | group A (§3 r43-r44 + §4 r45+) / group B (§5 charter outline + §6 charter §6 反映) の予定 sub-section 提示 | 次 session 着手内容明示 |

### 1.3 07 doc 内 navigation

- status header: foundation group (§1 + §2) draft 完了 — group A / group B は次 session 以降
- 「foundation group 確定値 summary」表を新規追加 (§1 algorithm 確定値 + §2 内訳確定値)
- 「次 step」表で group A / group B / work item (e) / r40 達成宣言 の cadence 明示

---

## 2. foundation group 算出値 (work item (d) 完了宣言の base + group A/B の input)

### 2.1 §1 r42 区切り algorithm 確定値

| 出処 | 値 | 結論 |
|---|---|---|
| §1.2 正式区分 | r41 / r41.5 / r42-α/β/γ/δ / r43-r44 / vk-RC = r44 達成 | charter §6 仮 line up の正式区分 (group B §6 反映 input) |
| §1.3 描画 stage mapping | r42-α = vk-β 着手 / r42-β = vk-β 完遂 + vk-δ 部分 / r42-γ = vk-γ + vk-δ 部分 / r42-δ = vk-γ + vk-δ 完遂 / r43-r44 = vk-RC | charter §6 描画 stage 表の update input |
| §1.4 時系列整合 | 05 doc §10 hooks + 04 doc §5.4 段階 1-5 (r41) + §B.1/B.2/B.3 (r42-α/β/γ) | 整合 ✓ |
| §1.5 r45+ 範囲外 | vk-RC 達成 = r44 達成 = 本算定終了 marker | charter §3 時間軸非設定遵守 |

### 2.2 §2 r42 milestone 内訳確定値

| sub-milestone | base PM (06 doc §3.9) | 暦月 (06 doc §5.3) | 暦年 (06 doc §5.6) | acceptance 件数 | OS 着手 timing (06 doc §4.4) |
|---|---|---|---|---|---|
| r42-α (picker) | 0.65 | 2.9 | ~2034 前半 | 6 件 draft | Linux baseline + **Win 追加開始** |
| r42-β (Cinematic) | 3.15 | 13.9 | ~2035 中 | 7 件 draft (BD cvar 13 件 visual A/B 含む) | Linux + Win 並走 + **Mac 追加開始** |
| r42-γ (visual realism) | 3.18 | 14.0 | ~2036 後半 | 8 件 draft (scene buffer alpha invariant + sustained A/B 含む) | Linux + Win 並走 + Mac 並走 |
| r42-δ (parity 残機能 / vk-RC 直前 polish) | 2.25 | 9.9 | ~2037 中 | 6 件 draft | Linux + Win polish + **Mac MoltenVK 詳細化** |
| **r42 合計** | **9.23** | **40.7** | **~3.4 年** | **27 件 draft** | 3 OS 全並走 |

### 2.3 foundation group が group A/B に渡す input

- **§1.2 正式区分** → group A §3 r43-r44 + §4 r45+ の入口、group B §6 charter §6 反映の入口
- **§1.3 描画 stage mapping** → group A §3 vk-RC 達成 acceptance criteria の入口
- **§1.5 r45+ 範囲外** → group A §4 r45+ broad outline + 別章 charter 起草指針の入口
- **§2 r42 内訳 27 件 acceptance criteria draft** → group B §5 charter outline の base
- **§2.5 acceptance 運用方針** → group B §5 charter outline 統一 template の base

---

## 3. self-trace (foundation group 完了宣言前の整合確認)

本 session で導入した算定値の前後 cross reference を確認:

### 3.1 §1 numeric 整合

| 項目 | 出処 | 整合 |
|---|---|---|
| §1.2 r41 base PM 16.17 / 暦月 84.1 / 暦年 ~2033 中 | 06 doc §3.9 + §5.3 + §5.6 | ✓ |
| §1.2 r41.5 base PM 1.50 / 暦月 7.2 / 暦年 ~2034 初 | 06 doc §3.9 + §5.3 + §5.6 | ✓ |
| §1.2 r42-α base PM 0.65 / 暦月 2.9 / 暦年 ~2034 前半 | 06 doc §3.9 + §5.3 + §5.6 | ✓ |
| §1.2 r42-β base PM 3.15 / 暦月 13.9 / 暦年 ~2035 中 | 06 doc §3.9 + §5.3 + §5.6 | ✓ |
| §1.2 r42-γ base PM 3.18 / 暦月 14.0 / 暦年 ~2036 後半 | 06 doc §3.9 + §5.3 + §5.6 | ✓ |
| §1.2 r42-δ base PM 2.25 / 暦月 9.9 / 暦年 ~2037 中 | 06 doc §3.9 + §5.3 + §5.6 | ✓ |
| §1.2 r43-r44 base PM 3.00 + 1.89 (Win) + 4.05 (Mac) / 暦月 12.0 + 8.3 + 17.8 | 06 doc §3.9 + §4.2 + §4.3 + §5.3 | ✓ |
| §1.2 vk-RC 累積 35.84 PM / 170 暦月 / ~14.17 年 / ~2040 年後半 | 06 doc §3.9 + §4.5 + §5.3 + §5.4 + §5.6 | ✓ |

### 3.2 §2 numeric 整合

| 項目 | 出処 | 整合 |
|---|---|---|
| §2.1 r42-α work breakdown 0.10 + 0.05 + 0.20 + 0.15 = 0.50 + 0.15 (余裕係数) = 0.65 | 06 doc §3.3 | ✓ |
| §2.2 r42-β work breakdown 0.40 + 0.20 + 0.50 + 1.00 + 0.15 = 2.25 + 0.90 (余裕係数) = 3.15 | 06 doc §3.4 | ✓ |
| §2.3 r42-γ work breakdown 0.27 + 0.60 + 0.30 + 0.10 + 0.50 + 0.50 = 2.27 + 0.91 (余裕係数) = 3.18 | 06 doc §3.5 | ✓ |
| §2.4 r42-δ work breakdown 0.50 + 0.30 + 0.70 = 1.50 + 0.75 (余裕係数) = 2.25 | 06 doc §3.6 | ✓ |
| §2.6 r42 合計 PM = 0.65 + 3.15 + 3.18 + 2.25 = 9.23 PM | §2.1-§2.4 集計 (06 doc §3.9 反映、初回算出 8.97 PM は誤算 → 9.23 PM に修正済) | ✓ |
| §2.6 r42 合計 暦月 = 2.9 + 13.9 + 14.0 + 9.9 = 40.7 暦月 | §2.1-§2.4 集計 (06 doc §5.3) | ✓ |
| §2.6 r42 合計 暦年 = ~3.4 年 (40.7 暦月 / 12) | 06 doc §5.4 反映 | ✓ |

### 3.3 acceptance criteria の memory 反映確認

| sub-milestone | 反映した memory | 反映箇所 |
|---|---|---|
| §2.1 r42-α | (なし、新規 acceptance criteria) | — |
| §2.2 r42-β | `project_r30_cinematic_control_tuning_deferred.md` (BD cvar 13 件 visual A/B) | acceptance #4 |
| §2.2 r42-β | `feedback_visual_decisions_need_live_ab.md` (visual 決定 live A/B 必須) | 余裕係数根拠 |
| §2.3 r42-γ | `project_aya_visual_realism_alpha_protect.md` (scene buffer alpha invariant) | acceptance #4 |
| §2.3 r42-γ | `project_atmos_atten_scalarized.md` (atmosFragLighting atten scalarization) | acceptance #2 |
| §2.3 r42-γ | `feedback_shader_color_space_correction.md` (shader 出力 linear / sRGB 逆引き) | acceptance #3 |
| §2.3 r42-γ | `feedback_instant_ab_vs_sustained.md` (sustained viewing 観測) | acceptance #6 |
| §2.4 r42-δ | `project_pr69_fallback_switch.md` (LL_DULLAHAN_AUDIO_CALLBACK 整合確認) | work breakdown 行 r1-r13 audio |
| §2.2-§2.4 | `feedback_credit_t_noami_equal_billing.md` (Mac t-noami workflow) | OS 着手 timing 全 r42 milestone |
| §2.4 r42-δ | `feedback_mac_only_fixes_accept_as_is.md` (Mac 限定 fix 受入) | acceptance #5 + OS 着手 timing |

整合 ✓ (§1: 8 項目 / §2: 7 項目 + memory 反映 10 項目、cross reference 漏れなし、numeric arithmetic 整合)。

---

## 4. AYA review 待ちポイント

### 4.1 §1 r42 区切り algorithm の妥当性

#### §1.2 正式区分 (r41 / r41.5 / r42-α/β/γ/δ / r43-r44 / vk-RC = r44)

- charter §6 仮 line up の r42 単一 → r42-α/β/γ/δ への細分化が **粒度として適切** か
- r43-r44 = vk-RC parity 補強 + 性能 polish + Mac portable subset 詳細化 に振替えるか (charter 仮では r43=vk-γ / r44=vk-δ だったが、r42-β/γ で大半完了するため)
- r45+ = 本算定範囲外 (visual realism 次世代 / ray tracing / HDR / GPU-driven は別章 charter) の方針

#### §1.3 描画 stage mapping (3 軸 align)

- 描画 stage 軸 (vk-α/β/γ/δ/RC) × AYAstorm 機能軸 (r21.1 picker / r30 Cinematic / r14+ visual realism) × OS 着手軸 が **整合的に align** しているか
- r42 内で vk-β 着手 → vk-β 完遂 → vk-γ 着手 → vk-γ + vk-δ 完遂 と 4 段階 stage 進行する設計が妥当か
- r43-r44 = vk-RC (parity 補強 + 性能 polish + Mac portable subset 詳細化) に名称統一して良いか

#### §1.5 r45+ 範囲外宣言

- vk-RC 達成 = r44 達成 = 本算定終了 marker (~2040 年後半) で work item (d) を close する方針が妥当か
- r45+ の broad placeholder (visual realism 次世代 / ray tracing / HDR / GPU-driven) を §4 で扱うが、本 (d) で詳細化しない方針 (別章 charter 起草で扱う)

### 4.2 §2 r42 milestone 内訳の妥当性

#### r42-α (§2.1)

- acceptance criteria 6 件 draft (parity / render pass attachment / read-pick / shader cross compile / Win 追加開始 / regression 無し) で十分か
- Win 追加開始のタイミングが r42-α 着手と同時 (06 doc §4.4 反映) で問題ないか (低 risk milestone での Win 同時並走)

#### r42-β (§2.2)

- acceptance criteria 7 件 draft (parity / DoF state enum 化 / shader cross compile / BD cvar 13 件 visual A/B / Mac 追加開始 / Win 並走 / regression 無し)
- BD cvar 13 件 visual A/B が **r42-β 段階で完遂** 求められるか、それとも r42-β は Vulkan 動作確認のみで visual A/B は別 cycle (live cvar 経由 opt-in) で詳細化するか
- DoF state enum 化が **本線 GL でも同 refactor 維持** invariant を charter で保証するか

#### r42-γ (§2.3)

- acceptance criteria 8 件 draft (parity / sky dome + atmospherics / shader cross compile + linear/sRGB / post-process chain / performance ≤10% / visual A/B sustained / Win-Mac 並走 / regression 無し)
- scene buffer alpha invariant (memory `project_aya_visual_realism_alpha_protect.md`) を charter で **明示的に保証** するか
- post-process chain 7 sub-pass の Vulkan render pass chain 化が r42-γ で完遂 求められるか (charter 仮では r44 で扱う設計だった)

#### r42-δ (§2.4)

- acceptance criteria 6 件 draft (r25-r29 3D stream / r1-r13 audio Vulkan 非依存 / vk-δ 完遂 / vk-RC 直前 polish / 3 OS 状況確認 / regression sweep) で十分か
- r25-r29 3D stream の Vulkan-side hook (05 doc §9.2 `VK_KHR_external_memory_*` 予約) が **r42-δ で最低限** 実装、本格は r45+ 別章 の方針が妥当か
- Mac MoltenVK 詳細化が r42-δ 着手 (Mac 着手は r42-β からだが詳細化は r42-δ で集中) で問題ないか

### 4.3 §2.5 acceptance criteria draft 運用方針

- 各 milestone 着手前に別 charter 起草 (`docs/specs/ayastorm-r4X-xxx/00-charter.md`) で詳細化する cadence が妥当か
- charter 詳細化主体 = AYA + Claude (r41.5 charter は法的 review 関与で AYA 比重大) で問題ないか
- draft が確定値ではない理由 3 件 (時系列長 / AYAstorm 進化追随 / LL 着地 status 反映) で十分な説明か

### 4.4 work item (d) 完了宣言までの cadence

- 本 (d) は foundation (§1+§2) / group A (§3+§4) / group B (§5+§6) の 3 group 分割で進めて問題ないか (work item (c) 4 group とは異なる粒度)
- 本 foundation group 完了 → 次 session group A (§3 r43-r44 + §4 r45+) 着手 で問題ないか
- group A / group B 各 1 session ずつ予定、本 work item (d) 完了は **本 session 含む 3 session で完了** 想定

---

## 5. 次 session 着手内容 (group A 着手予定)

### 5.1 AYA review pattern

#### Pattern A: §1 + §2 そのまま OK → group A (§3 r43-r44 + §4 r45+) 着手

- 07 doc §3 + §4 に group A draft 追加
- handoff doc を group A 完了で更新
- 次々 session で group B 着手 → work item (d) 完了宣言 → work item (e) 着手

#### Pattern B: §1 / §2 の一部 を修正したい

- 07 doc 該当 section を edit (差分 commit) → 修正後の §1/§2 結論を本 handoff doc に反映 → group A 着手

#### Pattern C: §1.2 正式区分 / §1.3 描画 stage mapping の **大幅変更** を入れたい

- 該当 section edit → 影響範囲確認 (§2 work breakdown + acceptance criteria + 06 doc §3 への propagation 確認) → 修正後に group A 着手判定

### 5.2 group A draft の方針

handoff doc §3 + §4 placeholder で提示済の予定 sub-section を base に draft:

#### §3 r43-r44 区切り draft 予定

- §3.0 算定方針 (charter §6 仮 line up の r43-r45+ 振替え logic、本 §1.2 で確定済の正式区分を r43-r44 = vk-RC parity 補強 + 性能 polish + Mac portable subset 詳細化 に再構成)
- §3.1 r43 sub-milestone 構成 (parity 補強 + Win driver matrix polish + Linux baseline 完遂)
- §3.2 r44 sub-milestone 構成 (Mac portable subset 詳細化 + 性能 polish + 3 OS parity 完遂 = vk-RC 達成)
- §3.3 vk-RC 達成宣言の acceptance criteria draft (charter §4 (1) parity 完遂 goal を満たす 3 OS 全機能動作 の具体 metric)
- §3.4 3 OS parity 完遂 marker (Linux: r43 / Win: r44 / Mac: r44、t-noami workflow cycle 完遂 timing)
- §3.5 §3 結論

#### §4 r45+ 区切り (本算定範囲外) draft 予定

- §4.0 算定方針 (charter §3 時間軸非設定遵守 + 06 doc §3.8 範囲外宣言 + 本 §1.5 r45+ 範囲外宣言の再確認)
- §4.1 r45+ scope の broad outline (visual realism 次世代 / ray tracing / HDR / GPU-driven の topic 列挙)
- §4.2 r45+ 着手 trigger 条件 (r44 達成 + AYA judgment、時間軸 trigger 無し)
- §4.3 r45+ charter 起草 timing (r44 達成宣言 + 6 か月以内に AYA さんと擦り合わせて別章 charter 起草)
- §4.4 §4 結論

### 5.3 group A 完了の output

- 07 doc §3 + §4 draft 完成 (本 foundation group + group A で §1-§4 完成)
- handoff doc を group A 完了 ver. に更新
- 次々 session で group B (§5 charter outline + §6 charter §6 反映) 着手 → work item (d) 完了宣言

### 5.4 group B 着手後の cadence (本 session 段階での見込み)

- group B §5 各 milestone charter outline draft (r41 / r41.5 / r42-α/β/γ/δ / r43 / r44 charter outline 8 件)
- group B §6 charter §6 反映 draft (00-charter.md §6 update のための diff 提示)
- group B 完了 → work item (d) 完了宣言 → **work item (e) charter 完成** 着手 (00-charter.md final review + sub-phase 3 全 work item ((a)-(d)) の statement of completion)
- work item (e) 完了 = **r40 章 close** → **r41 着手** (`docs/specs/ayastorm-r41-gl-removal/00-charter.md` 起草)

---

## 6. AYA review pattern (再掲)

### Pattern A: foundation group (§1 + §2) そのまま OK → group A (§3 + §4) 着手

→ 次 session で 07 doc §3 + §4 draft 着手、group A 完了 handoff doc 作成

### Pattern B: §1 / §2 の一部 を修正したい

→ 07 doc 該当 section edit (差分 commit) → 修正後の §1/§2 結論を本 handoff doc 反映 → group A 着手

### Pattern C: §1.2 / §1.3 大幅変更

→ 該当 section edit → 影響範囲確認 (§2 + 06 doc 反映確認) → 修正後に group A 着手判定

---

## 7. commit log (本 session)

- 前 session 末: `(work item (c) group C draft + handoff doc) 完了済 (commit c4aa2e3f2d)`
- 本 session: 03 doc + 06 doc work item (c) 完了反映 + 07 doc 新規作成 + 本 handoff doc を 1 commit で投入予定

---

## 8. 関連 doc / memory

### r40 章内部 doc

- `00-charter.md` — r40 章 charter (§3 時間軸非設定 / §4 (1) parity 完遂 goal / §6 仮 line up = 本 (d) §6 で正式 mapping 反映予定 = group B で完了)
- `03-sub-phase-3-vulkan-plan.md` — work item (c) 完了 + work item (d) draft 作成中 (本 session で更新)
- `04-portage-inventory.md` — work item (a) 完了 (§5.4 段階 port 戦略 + §6.3.2 AYAstorm 機能 pull-in 順 = 本 §1.2/§1.4 input)
- `05-vulkan-api-design.md` — work item (b) 完了 (§10 skeleton 時系列 + §3 descriptor set + §4 render pass + §8 OS 別 + §9.4 MoltenVK = 本 §1.4/§2 input)
- `06-effort-estimation.md` — work item (c) 完了 確定 (§3 milestone 別工数 + §4.4 OS 着手 timing + §5.6 marker 暦年 = 本 §1.2/§2 input)
- `07-r42-plus-milestone-mapping.md` — work item (d) foundation group draft 完成 (本 session 新規作成)

### memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory (本 foundation group 完了反映を本 session 末に追記予定)
- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 §2.1 + §5.1 charter outline で詳細化反映予定)
- `project_ayastorm_three_platforms.md` — 3 OS 大前提 (本 §1.3 + §2 OS 着手 timing で反映)
- `project_r30_cinematic_control_tuning_deferred.md` — r30 BD cvar 13 件 tuning (本 §2.2 r42-β visual A/B 反映)
- `project_aya_visual_realism_alpha_protect.md` — scene buffer alpha invariant (本 §2.3 r42-γ acceptance criteria 反映)
- `project_atmos_atten_scalarized.md` — atmosFragLighting atten scalarization (本 §2.3 r42-γ sky dome 反映)
- `feedback_shader_color_space_correction.md` — shader 出力 linear / sRGB 逆引き (本 §2.3 r42-γ shader cross compile 反映)
- `feedback_visual_decisions_need_live_ab.md` — visual 決定 live A/B 必須 (本 §2.2 / §2.3 visual A/B 反映)
- `feedback_instant_ab_vs_sustained.md` — instant vs sustained A/B (本 §2.3 sustained viewing 反映)
- `feedback_credit_t_noami_equal_billing.md` — Mac t-noami workflow (本 §2.2 / §2.3 / §2.4 Mac 着手 timing 反映)
- `feedback_mac_only_fixes_accept_as_is.md` — Mac 限定 fix 受入 (本 §2.4 Mac MoltenVK 詳細化 反映)
- `feedback_proactive_handoff.md` — group 境界 handoff (本 foundation group 完了で次 session への handoff doc 作成)
- `feedback_self_verify_before_handoff.md` — group 完了時 self-trace (本 foundation group 完了で実施、§3 で 8 + 7 + 10 = 25 項目確認済)
- `feedback_explanation_lead_with_conclusion.md` — handoff doc 構成 (結論ファースト、本 doc §1 で本 session 完了 summary 先出し)
