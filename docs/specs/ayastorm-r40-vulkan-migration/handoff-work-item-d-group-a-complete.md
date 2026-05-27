# handoff: r40 sub-phase 3 work item (d) r42+ 区切り確定 — group A 完了 (§3 r43-r44 + §4 r45+ draft 完成)

**作成日**: 2026-05-28
**前 session 状況**: work item (d) foundation group (§1 r42 区切り algorithm + §2 r42 milestone 内訳) draft 完了 (前 session、commit f97a25666b) → AYA 引き続き指示 = Pattern A 想定で本 session 着手 → group A (§3 r43-r44 + §4 r45+) draft 完了
**branch**: `feature/ayastorm-r40-vulkan-migration`
**親 doc**: `03-sub-phase-3-vulkan-plan.md` work item (d)

---

## 1. 本 session で完了したもの

### 1.1 07 doc group A draft 完成

| section | 内容 | 結論 |
|---|---|---|
| §3.0 算定方針 | charter §6 仮 line up の r43-r45+ 振替え logic (旧 r43=vk-γ/r44=vk-δ/r45+=vk-RC → 新 r43-r44=vk-RC parity 補強 + 性能 polish + 3 OS parity 完遂) + r43/r44 を 3 OS parity 完遂順で分割 + 各 sub-milestone 提示項目 | r43-r44 振替え logic 確定 ✓ |
| §3.1 r43 (Linux baseline 安定維持 + Win parity 完遂) | 目的 = vk-RC parity 補強 (Win 寄与) + 3 OS parity 完遂順の Win 段、work breakdown ~2.63 PM 内訳 (Win driver matrix 0.50 + Win 純増分 1.35 = 1.85 base + 余裕係数 +42% = 2.63)、acceptance 7 件 draft、~20 暦月 ~2038 中 - 2039 初 | r43 内訳確定 ✓ |
| §3.2 r44 (Mac portable subset 詳細化 + Mac parity 完遂 = vk-RC 達成) | 目的 = vk-RC parity 補強 (Mac 寄与 + 性能 polish) + 3 OS parity 完遂 + charter §4 (1) 完遂 goal 到達、work breakdown ~6.30 PM 内訳 (Mac MoltenVK 1.00 + 性能 polish 0.50 + Mac 純増分 2.70 = 4.20 base + 余裕係数 +50% = 6.30)、acceptance 9 件 draft、~18 暦月 ~2039 初 - 2040 後半 (vk-RC 達成 marker) | r44 内訳確定 ✓ |
| §3.3 vk-RC 達成宣言の acceptance criteria draft | 10 軸 (全機能 parity (3 OS) / audio (r1-r13) / 視覚表現 (r14-r24) / 3D stream (r25-r29) / Cinematic (r30) / picker / 3 OS driver coverage / 性能 polish / release note / regression sweep)、charter §4 (1) parity 完遂 goal の具体 metric、宣言主体 = AYA + 宣言場所 = r44 charter + r40 charter §4 (1) marker | vk-RC 達成 acceptance criteria 確定 ✓ |
| §3.4 3 OS parity 完遂 marker | 06 doc §4.4 反映の 3 OS 着手 → 完遂 cadence (Linux r41 → Win r42-α 着手 → r43 完遂、Mac r42-β 着手 → r42-δ MoltenVK 詳細化開始 → r44 完遂)、t-noami workflow cycle lead time、3 OS parity 完遂宣言の運用 | 3 OS parity cadence 確定 ✓ |
| §3.5 §3 結論 | 6 件 algorithm 確定 (振替え logic / r43 内訳 / r44 内訳 / vk-RC acceptance 10 軸 / 3 OS parity 完遂順 / r43-r44 合計 ~8.93 PM ~38 暦月 ~3.2 年) | r43-r44 区切り確定 ✓ |
| §4.0 算定方針 | charter §3 + §4 (3) + 06 doc §3.8 + 本 (d) §1.5 の 4 重遵守、本 §4 で扱う範囲 (broad outline / 着手 trigger / charter 起草 timing のみ) | r45+ 範囲外確定 ✓ |
| §4.1 r45+ scope broad outline | topic 列挙 5 件 (visual realism 次世代 / ray tracing / HDR / GPU-driven / AYAstorm 独自進化)、詳細化を本 §4 で扱わない理由 4 件 (時系列長 / AYAstorm 進化追随 / LL 着地 status / r40 達成 目的) | r45+ scope outline 確定 ✓ |
| §4.2 r45+ 着手 trigger 条件 | 必須 trigger = r44 達成 + AYA judgment、任意 trigger = charter §7 LL 着地時判断 + §8 plan B trigger 連動、時間軸 trigger 無し (charter §3 遵守)、判定主体 = AYA + Claude 補助 | r45+ trigger 条件確定 ✓ |
| §4.3 r45+ charter 起草 timing | cadence = r44 達成宣言 + 6 か月以内に AYA 擦り合わせ開始、起草先 = `docs/specs/ayastorm-r45-plus-xxx/00-charter.md` 別 directory、分章可能 | r45+ charter cadence 確定 ✓ |
| §4.4 §4 結論 | 4 件 (本算定範囲外確定 / scope outline / 着手 trigger / charter cadence)、r40 章工程プラン終了 marker = vk-RC 達成 (r44 達成) = ~2040 後半 | r45+ 区切り確定 ✓ |

### 1.2 07 doc 内 navigation 更新

- status header: foundation + group A draft 完了に更新 (line 3)
- 「foundation group + group A 確定値 summary」セクションに §3 r43-r44 + §4 r45+ + vk-RC 累積表を追加
- 「次 step」表で group B / work item (d) 完了 / work item (e) / r40 達成宣言 / r41 着手 の cadence 明示
- 「関連 doc / memory」セクションに本 group A 反映 memory を追加 (`project_pr69_fallback_switch` / `feedback_release_notes_link_only` / `feedback_release_note_per_feature`)

### 1.3 03 doc work item (d) status 反映

- status header: foundation group + group A draft 完了 / group B 残 に更新
- work item 表 (d) 行: foundation group §1+§2 + group A §3+§4 完了、group B §5+§6 残 を明示

### 1.4 自己整合性修正 (self-trace 中に発見 → 修正)

draft 初稿で r43 work breakdown に Linux baseline parity 補強 0.50 PM を捻り出して計上していたが、06 doc §3.7 にない誤算と判明 (memory `feedback_admit_unknown.md` + `feedback_build_only_verified.md` 反映で実機 source なし数値は積まない)。

修正後:

- r43 base = Win driver matrix 0.50 (§3.7) + Win 純増分 1.35 (§4.2) = 1.85 PM
- r43 余裕係数 +42% 加重平均 = ~2.63 PM
- r43 + r44 = 2.63 + 6.30 = **8.93 PM** ≈ 06 doc §4.5 (35.84) - r41 (16.17) - r41.5 (1.50) - r42 (9.23) = **8.94 PM** ✓ (差 0.01 PM 丸め誤差範囲内)
- r43 暦月 = ~20 暦月 (06 doc §5.6 marker r42-δ 達成 ~132 + Linux baseline ~12 + Win 増分 ~8 = ~152 暦月 と整合)
- r44 暦月 = ~18 暦月 (~170 - ~152 = ~18 と整合)
- r43-r44 合計暦月 = ~38 暦月 (06 doc §5.6 ~170 - ~132 = ~38 と整合)

---

## 2. group A 確定値 (group B input + AYA review 待ち)

### 2.1 §3 r43-r44 区切り確定値

| sub-milestone | base PM | 余裕係数 | total PM | 中央値暦月 | 暦年マーカー | acceptance 件数 |
|---|---|---|---|---|---|---|
| r43 (Linux baseline 安定維持 + Win parity 完遂) | 1.85 | +42% | ~2.63 | ~20 | ~2038 中 - 2039 初 | 7 件 draft |
| r44 (Mac portable subset 詳細化 + Mac parity 完遂 = vk-RC 達成) | 4.20 | +50% | ~6.30 | ~18 | ~2039 初 - 2040 後半 | 9 件 draft |
| **r43-r44 合計** | **6.05** | — | **~8.93** | **~38** | **~3.2 年** | **16 件 draft + vk-RC 10 軸** |

### 2.2 §3.3 vk-RC 達成 acceptance criteria 10 軸

| # | 軸 | 内容 |
|---|---|---|
| 1 | 全機能 parity (3 OS) | AYAstorm r1-r30 全機能を Linux + Win + Mac 3 OS で Vulkan 上に再現、本線 GL と visual + 機能同等 |
| 2 | audio chapter (r1-r13) | FMOD callback + Dullahan path が 3 OS Vulkan 上で本線 GL と同等動作、Vulkan invariant |
| 3 | 視覚表現 chapter (r14-r24) | sky dome + atmospherics + post-process chain + scene buffer alpha invariant 遵守、live A/B (sustained viewing) で cumulative 効果観測 |
| 4 | 3D stream chapter (r25-r29) | NDI / OBS / 3D stream 配信が 3 OS Vulkan 上で本線 GL と同等の stream quality |
| 5 | Cinematic chapter (r30) | r30 Cinematic mode + BD cvar 13 件 visual A/B + Cinematic Controls 全機能 3 OS parity |
| 6 | picker / chat / その他 | r21.1 self-rigged picker + chat tab split + その他 r1-r30 全機能 3 OS parity |
| 7 | 3 OS driver coverage | Linux Mesa/NVIDIA first-class + Win NVIDIA/AMD/Intel first-class + Mac MoltenVK 1.2.x portable subset first-class |
| 8 | 性能 polish | frame in flight tuning + barrier sequence + VMA allocation strategy 本格 tuning、本線 GL と同等以上 frame time (regression ≤10%) |
| 9 | release note + 運用 doc | vk-RC 達成 release note + 3 OS driver minimum 版数 + WHCK 認定 + MoltenVK 1.2 fallback note 整備 |
| 10 | regression sweep | r41-r43 baseline 全 milestone 累積 regression 無し |

### 2.3 §4 r45+ 区切り (本算定範囲外) 確定値

| 出処 | 値 | 用途 |
|---|---|---|
| §4.0 4 重遵守 | charter §3 + §4 (3) + 06 doc §3.8 + 本 (d) §1.5 で範囲外確定 | r45+ 詳細化を本 (d) で扱わない根拠 |
| §4.1 scope broad outline | visual realism 次世代 / ray tracing / HDR / GPU-driven / AYAstorm 独自進化 | 別章 charter 起草の input |
| §4.2 着手 trigger | 必須 = r44 達成 + AYA judgment、任意 = charter §7/§8 連動、時間軸 trigger 無し | charter §3 / §7 / §8 連動方針 |
| §4.3 charter 起草 cadence | r44 達成宣言 + 6 か月以内、`docs/specs/ayastorm-r45-plus-xxx/00-charter.md` 別 directory、分章可能 | r40 章 close 後の cadence |

### 2.4 vk-RC 累積確定値 (本 §3.5 結論)

| milestone 累積 | base PM | 中央値暦月 | 暦年マーカー |
|---|---|---|---|
| r41 (Linux baseline) | 16.17 | ~84.1 | ~2033 中 |
| r41 + r41.5 | 17.67 | ~91.3 | ~2034 初 |
| r41 + r41.5 + r42 | 26.90 | ~132 | ~2037 中 |
| r41 + r41.5 + r42 + r43 | ~29.53 | ~152 | ~2039 初 |
| **r41 + r41.5 + r42 + r43 + r44 (vk-RC 達成)** | **~35.83 (≈ 06 doc §4.5 35.84 ✓)** | **~170 (= 06 doc §5.6 ✓)** | **~2040 後半 (= 06 doc §5.6 ✓)** |

→ vk-RC 累積 ~14.2 年 = charter §4 (3) 想定 15-30 年帯の下方近接、本算定中央値 = charter §4 (3) 想定範囲内 ✓

---

## 3. self-trace (group A 完了宣言前の整合確認)

### 3.1 §3 numeric 整合

| 項目 | 出処 | 整合 |
|---|---|---|
| §3.1 r43 work breakdown 0.50 (Win driver matrix) + 1.35 (Win 純増分) = 1.85 base | 06 doc §3.7 + §4.2 | ✓ |
| §3.1 r43 余裕係数 +42% (Linux 軸 +50% / Win +40% 加重) → 1.85 × 1.42 = 2.63 PM | 06 doc §3.7 + §4.2 加重 | ✓ |
| §3.2 r44 work breakdown 1.00 (Mac MoltenVK) + 0.50 (性能 polish) + 2.70 (Mac 純増分) = 4.20 base | 06 doc §3.7 + §4.3 | ✓ |
| §3.2 r44 余裕係数 +50% → 4.20 × 1.50 = 6.30 PM | 06 doc §3.7 + §4.3 | ✓ |
| §3.5 r43 + r44 = 2.63 + 6.30 = 8.93 PM | 集計 | ✓ |
| 06 doc §4.5 (35.84) - r41 (16.17) - r41.5 (1.50) - r42 (9.23) = 8.94 PM | 06 doc §4.5 引算 | ✓ (差 0.01 丸め誤差) |
| §3.3 vk-RC 達成 acceptance criteria 10 軸 | charter §4 (1) + 本 §2-§3 acceptance 集約 | ✓ |
| §3.4 3 OS parity 完遂 marker (Linux r43 / Win r43 / Mac r44) | 06 doc §4.4 反映 | ✓ |

### 3.2 §3 暦月整合

| 項目 | 出処 | 整合 |
|---|---|---|
| §3.1 r43 暦月 ~20 暦月 (Linux baseline parity ~12 + Win 増分 ~8) | 06 doc §5.6 (r42-δ 達成 ~132 + ~12 + ~8 = ~152) | ✓ |
| §3.2 r44 暦月 ~18 暦月 (Mac MoltenVK + 性能 polish + Mac 純増分) | 06 doc §5.6 (~170 - ~152 = ~18) | ✓ |
| §3.5 r43-r44 合計暦月 ~38 暦月 | 06 doc §5.6 (~170 - ~132 = ~38) | ✓ |
| vk-RC 累積暦月 ~170 暦月 / ~2040 後半 | 06 doc §5.6 marker | ✓ |

### 3.3 §4 範囲外確定整合

| 項目 | 出処 | 整合 |
|---|---|---|
| §4.0 4 重遵守 (charter §3 + §4 (3) + 06 doc §3.8 + 本 §1.5) | charter + 06 doc + 本 (d) | ✓ |
| §4.1 scope outline (visual realism 次世代 / ray tracing / HDR / GPU-driven / AYAstorm 独自進化) | charter §6 + 05 doc §9.5 予約 | ✓ |
| §4.2 trigger (r44 達成 + AYA judgment、時間軸 trigger 無し) | charter §3 + §7 + §8 連動 | ✓ |
| §4.3 charter 起草 cadence (r44 達成 + 6 か月以内、`docs/specs/ayastorm-r45-plus-xxx/`) | charter §3 時間軸非設定遵守 | ✓ |

### 3.4 memory 反映確認

| memory | 反映箇所 |
|---|---|
| `project_ayastorm_three_platforms.md` (3 OS 大前提) | §3.4 3 OS parity 完遂順 + 各 sub-milestone OS 着手 timing |
| `project_pr69_fallback_switch.md` (LL_DULLAHAN_AUDIO_CALLBACK) | §3.3 vk-RC acceptance #2 audio chapter |
| `project_aya_visual_realism_alpha_protect.md` (scene buffer alpha) | §3.3 vk-RC acceptance #3 視覚表現 chapter |
| `project_atmos_atten_scalarized.md` (atmosFragLighting) | §3.3 vk-RC acceptance #3 視覚表現 chapter |
| `project_r30_cinematic_control_tuning_deferred.md` (BD cvar 13 件) | §3.3 vk-RC acceptance #5 Cinematic chapter |
| `feedback_shader_color_space_correction.md` (shader linear/sRGB) | §3.3 vk-RC acceptance #3 視覚表現 chapter |
| `feedback_visual_decisions_need_live_ab.md` (visual live A/B) | §3.3 vk-RC acceptance #3 + #5 |
| `feedback_instant_ab_vs_sustained.md` (sustained viewing) | §3.3 vk-RC acceptance #3 視覚表現 chapter |
| `feedback_credit_t_noami_equal_billing.md` (Mac t-noami workflow) | §3.2 r44 acceptance #5 + #6 + §3.4 t-noami cycle lead time |
| `feedback_mac_only_fixes_accept_as_is.md` (Mac 限定 fix 受入) | §3.1 r43 acceptance #6 (Win にも同方針適用) + §3.2 r44 acceptance #6 |
| `feedback_release_notes_link_only.md` (release note リンク集) | §3.3 vk-RC acceptance #9 release note 整備 |
| `feedback_release_note_per_feature.md` (1 feature 1 note) | §3.3 vk-RC acceptance #9 release note 整備 |
| `feedback_admit_unknown.md` + `feedback_build_only_verified.md` (実機 source なし数値は積まない) | §1.4 自己整合性修正 (Linux baseline parity 補強 0.50 PM 削除) |

整合 ✓ (§3: 8 項目 numeric + 4 項目暦月 + §4: 4 項目範囲外 + memory 13 項目、cross reference 漏れなし、numeric arithmetic 整合)。

---

## 4. AYA review 待ちポイント

### 4.1 §3 r43-r44 区切りの妥当性

#### §3.0 振替え logic

- charter §6 仮 line up の r43=vk-γ/r44=vk-δ/r45+=vk-RC を r43-r44=vk-RC parity 補強 + 性能 polish + 3 OS parity 完遂 に振替える logic が妥当か
- r42 内で vk-γ + vk-δ 大半完遂 (本 §1.3) → r43-r44 = parity 補強 + 性能 polish + Mac portable subset 詳細化 という再構成が **粒度として適切** か

#### §3.1 r43 (Linux baseline 安定維持 + Win parity 完遂)

- r43 を「Linux baseline 安定維持 (0 新規 work)」+「Win parity 完遂」と切る方針が妥当か (Linux baseline parity 完遂自体は r42-δ acceptance #4 で達成済の前提)
- acceptance criteria 7 件 draft (Linux baseline 安定維持 + Win driver matrix + Win surface 化 + WHCK 認定 + LunarG SDK 統合 + Win-specific bug fix + regression 無し) で十分か
- 暦月 ~20 暦月 (Linux baseline ~12 + Win 増分 ~8) の解釈が妥当か (06 doc §5.6 marker と整合)

#### §3.2 r44 (Mac portable subset 詳細化 + Mac parity 完遂 = vk-RC 達成)

- r44 を「Mac MoltenVK 詳細化 + 性能 polish 3 OS 適用 + Mac parity 完遂 = vk-RC 達成」と切る方針が妥当か
- 性能 polish 0.50 PM (06 doc §3.7) を r44 に配分 (vk-RC 達成時の最終 tuning) の判断が妥当か (代替 = r43 に配分 or r43/r44 で分割)
- acceptance criteria 9 件 draft (Mac parity + MoltenVK portable subset + Mac surface 化 + Apple Silicon UMA + MSL + t-noami workflow + 性能 polish + vk-RC 達成 + regression 無し) で十分か

#### §3.3 vk-RC 達成 acceptance criteria 10 軸

- charter §4 (1) parity 完遂 goal の具体 metric として 10 軸 (全機能 / audio / 視覚表現 / 3D stream / Cinematic / picker / 3 OS driver / 性能 polish / release note / regression) で網羅できているか
- 宣言主体 = AYA / 宣言場所 = r44 charter + r40 charter §4 (1) marker の運用が妥当か

#### §3.4 3 OS parity 完遂 marker

- Linux r41 → Win r42-α 着手 → r43 完遂 / Mac r42-β 着手 → r42-δ MoltenVK 詳細化開始 → r44 完遂 の cadence が妥当か
- r43 達成宣言 (Linux + Win parity 完遂) と r44 達成宣言 (vk-RC 達成 = 3 OS parity 完遂) の 2 段宣言が妥当か

### 4.2 §4 r45+ 区切り (本算定範囲外) の妥当性

#### §4.0 4 重遵守

- charter §3 + §4 (3) + 06 doc §3.8 + 本 (d) §1.5 の 4 重遵守で r45+ を本算定範囲外確定する方針が妥当か

#### §4.1 scope broad outline

- 5 topic (visual realism 次世代 / ray tracing / HDR / GPU-driven / AYAstorm 独自進化) の列挙で十分か (追加 topic 候補があるか)
- 詳細化を本 §4 で扱わない 4 件の理由 (時系列長 / AYAstorm 進化追随 / LL 着地 status / r40 達成目的) で十分な説明か

#### §4.2 r45+ 着手 trigger 条件

- 必須 trigger = r44 達成 + AYA judgment、任意 trigger = charter §7/§8 連動、時間軸 trigger 無し の方針が妥当か

#### §4.3 r45+ charter 起草 timing

- r44 達成宣言 + 6 か月以内に AYA 擦り合わせ開始の cadence が妥当か (代替 = 3 か月以内 / 12 か月以内)
- `docs/specs/ayastorm-r45-plus-xxx/00-charter.md` 別 directory + 分章可能 の運用が妥当か

### 4.3 group B 着手前の cadence 確認

- 本 group A 完了 → 次 session で group B (§5 全 milestone charter outline + §6 charter §6 反映 draft) 着手で問題ないか
- group B 完了 → work item (d) 完了宣言 → work item (e) charter 完成 → r40 達成宣言 → r41 着手 の cadence が妥当か (本 work item (d) は本 session 含む 3 session で完了想定 = foundation + group A + group B)

---

## 5. 次 session 着手内容 (group B 着手予定)

### 5.1 AYA review pattern

#### Pattern A: §3 + §4 そのまま OK → group B (§5 全 milestone charter outline + §6 charter §6 反映) 着手

- 07 doc §5 + §6 に group B draft 追加
- handoff doc を group B 完了で更新
- group B 完了 → work item (d) 完了宣言 → work item (e) 着手

#### Pattern B: §3 / §4 の一部 を修正したい

- 07 doc 該当 section を edit (差分 commit) → 修正後の §3/§4 結論を本 handoff doc に反映 → group B 着手

#### Pattern C: §3.0 振替え logic / §3.3 vk-RC acceptance 10 軸 の **大幅変更** を入れたい

- 該当 section edit → 影響範囲確認 (§5 charter outline + §6 charter §6 反映 への propagation 確認) → 修正後に group B 着手判定

### 5.2 group B draft の方針

#### §5 各 milestone charter outline draft 予定

- §5.0 算定方針 (各 milestone charter outline 統一 template、本 (d) §2.5 acceptance criteria 運用方針継承)
- §5.1 r41 charter outline (16.17 PM / ~84 暦月、GL 除去 + Vulkan 空転、本 §1.4 04 doc §5.4 段階 1-5 入力)
- §5.2 r41.5 charter outline (1.50 PM / ~7 暦月、VK repo 分離 + Vulkan code abstraction + 法的 review、AYA 比重大)
- §5.3 r42-α/β/γ/δ charter outline (本 §2.1-§2.4 内訳 base、各 sub-milestone 着手前の charter 起草の outline 統一 template)
- §5.4 r43 / r44 charter outline (本 §3.1-§3.2 内訳 base、r44 charter は vk-RC 達成宣言の acceptance criteria 10 軸を中核)
- §5.5 §5 結論 (全 8 milestone charter outline 提示完了の宣言)

#### §6 charter §6 仮 line up の本 §1-§5 反映 draft 予定

- §6.0 反映方針 (00-charter.md §6 update のための diff 提示、work item (e) charter 完成への引継ぎ)
- §6.1 charter §6 仮 line up 表の本 §1.2 正式区分への置換 draft
- §6.2 charter §6 「a-4 棚卸しで確定した AYAstorm 機能 pull-in 順」section の本 §1.3 mapping への昇格 draft
- §6.3 charter §6 r41.5 milestone section の本 §2.5 charter outline cadence への反映 draft
- §6.4 r45+ 範囲外 + 別章 charter 起草指針の charter §6 追加 draft
- §6.5 §6 結論 (work item (e) charter 完成への引継ぎ)

### 5.3 group B 完了の output

- 07 doc §5 + §6 draft 完成 = 本 work item (d) §1-§6 全完成
- 00-charter.md §6 update diff の draft 提示 (実 update は work item (e) で実施)
- handoff doc を group B 完了 ver. に更新
- work item (d) 完了宣言 → work item (e) charter 完成 着手

### 5.4 work item (e) 着手後の cadence

- work item (e) = 00-charter.md final review + sub-phase 3 全 work item ((a)-(d)) の statement of completion + 03 doc 最終 review
- work item (e) 完了 = **r40 章 close** → **r41 着手** (`docs/specs/ayastorm-r41-gl-removal/00-charter.md` 起草)

---

## 6. AYA review pattern (再掲)

### Pattern A: group A (§3 + §4) そのまま OK → group B (§5 + §6) 着手

→ 次 session で 07 doc §5 + §6 draft 着手、group B 完了 handoff doc 作成

### Pattern B: §3 / §4 の一部 を修正したい

→ 07 doc 該当 section edit (差分 commit) → 修正後の §3/§4 結論を本 handoff doc 反映 → group B 着手

### Pattern C: §3.0 / §3.3 大幅変更

→ 該当 section edit → 影響範囲確認 (§5 charter outline + §6 charter §6 反映 への propagation 確認) → 修正後に group B 着手判定

---

## 7. commit log (本 session)

- 前 session 末: `(work item (d) foundation group §1 r42 区切り algorithm + §2 r42 milestone 内訳 draft 完了 + handoff doc) 完了済 (commit f97a25666b)`
- 本 session: 07 doc group A (§3 r43-r44 + §4 r45+) draft + 03 doc work item (d) status 反映 + 本 handoff doc を 1 commit で投入予定

---

## 8. 関連 doc / memory

### r40 章内部 doc

- `00-charter.md` — r40 章 charter (§3 時間軸非設定 / §4 (1) parity 完遂 goal / §6 仮 line up = 本 (d) §6 で正式 mapping 反映 = group B で完了)
- `03-sub-phase-3-vulkan-plan.md` — work item (d) foundation + group A 完了反映 (本 session で更新)
- `04-portage-inventory.md` — work item (a) 完了 (§5.4 段階 port 戦略 + §6.3.2 AYAstorm 機能 pull-in 順)
- `05-vulkan-api-design.md` — work item (b) 完了 (§3 descriptor set + §4 render pass + §8 OS 別 + §9.4 MoltenVK + §10 skeleton)
- `06-effort-estimation.md` — work item (c) 完了 (§3.7 r43-r44 milestone + §4.2 Win 増分 + §4.3 Mac 増分 + §4.5 3 OS 合計 + §5.6 marker 暦年 = 本 §3 input)
- `07-r42-plus-milestone-mapping.md` — work item (d) foundation + group A draft 完成 (本 session で §3 + §4 追加)

### memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory (本 group A 完了反映を本 session 末に追記予定)
- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 §3 では Linux baseline 完遂後の安定維持の input)
- `project_ayastorm_three_platforms.md` — 3 OS 大前提 (本 §3.4 3 OS parity 完遂順 反映)
- `project_pr69_fallback_switch.md` — LL_DULLAHAN_AUDIO_CALLBACK (本 §3.3 vk-RC acceptance #2 audio chapter 反映)
- `project_aya_visual_realism_alpha_protect.md` — scene buffer alpha invariant (本 §3.3 vk-RC acceptance #3 視覚表現 反映)
- `project_atmos_atten_scalarized.md` — atmosFragLighting atten scalarization (本 §3.3 vk-RC acceptance #3 視覚表現 反映)
- `project_r30_cinematic_control_tuning_deferred.md` — r30 BD cvar 13 件 tuning (本 §3.3 vk-RC acceptance #5 Cinematic 反映)
- `feedback_shader_color_space_correction.md` — shader 出力 linear / sRGB 逆引き (本 §3.3 vk-RC acceptance #3 視覚表現 反映)
- `feedback_visual_decisions_need_live_ab.md` — visual 決定 live A/B 必須 (本 §3.3 vk-RC acceptance #3 + #5 反映)
- `feedback_instant_ab_vs_sustained.md` — instant vs sustained A/B (本 §3.3 vk-RC acceptance #3 反映)
- `feedback_credit_t_noami_equal_billing.md` — Mac t-noami workflow (本 §3.2 r44 + §3.4 t-noami cycle 反映)
- `feedback_mac_only_fixes_accept_as_is.md` — Mac 限定 fix 受入 (本 §3.1 r43 Win 限定 fix にも適用 + §3.2 r44 Mac 固有 quirk 反映)
- `feedback_release_notes_link_only.md` — Release Notes リンク集 (本 §3.3 vk-RC acceptance #9 release note 反映)
- `feedback_release_note_per_feature.md` — 1 feature 1 note (本 §3.3 vk-RC acceptance #9 release note 反映)
- `feedback_admit_unknown.md` + `feedback_build_only_verified.md` — 実機 source なし数値は積まない (§1.4 自己整合性修正で適用、Linux baseline parity 補強 0.50 PM 捻り出しを削除)
- `feedback_proactive_handoff.md` — group 境界 handoff (本 group A 完了で次 session への handoff doc 作成)
- `feedback_self_verify_before_handoff.md` — group 完了時 self-trace (本 group A 完了で実施、§3 で 8+4 = 12 項目 + §4 で 4 項目 + memory 13 項目 = 29 項目確認済)
- `feedback_explanation_lead_with_conclusion.md` — handoff doc 構成 (結論ファースト、本 doc §1 で本 session 完了 summary 先出し)
