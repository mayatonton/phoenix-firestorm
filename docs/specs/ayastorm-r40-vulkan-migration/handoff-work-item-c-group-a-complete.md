# handoff: r40 sub-phase 3 work item (c) 工程算定 — group A 完了

**作成日**: 2026-05-28
**前 session 状況**: foundation group (§1 per-file + §2 per-shader) draft 完了 (前 session) → group A (§3 per-milestone + §4 3 OS 増分) draft 完了 (本 session)
**branch**: `feature/ayastorm-r40-vulkan-migration`
**親 doc**: `03-sub-phase-3-vulkan-plan.md` work item (c)

---

## 1. 本 session で完了したもの

### 1.1 06-effort-estimation.md §3 per-milestone 工数積算 draft (本 session 前半)

| sub-section | 内容 | 算出値 |
|---|---|---|
| §3.0 | 振り分け方針 (foundation 12.92 PM の milestone 帰属 + a-3 §B.x 残分 + a-3 範囲外新規 milestone + 余裕係数 per-milestone 適用) | foundation 12.92 + 追加 8.15 = base work 21.07 PM |
| §3.1 | r41 (GL 除去 + Vulkan 空転) work 工数 + 余裕係数 +40% | **16.17 PM** |
| §3.2 | r41.5 (VK repo 分離) work 工数 + 余裕係数 +50% | **1.50 PM** |
| §3.3 | r42-α (r21.1 picker port) work 工数 + 余裕係数 +30% | **0.65 PM** |
| §3.4 | r42-β (r30 Cinematic port) work 工数 + 余裕係数 +40% | **3.15 PM** |
| §3.5 | r42-γ (r14+ visual realism port) work 工数 + 余裕係数 +40% | **3.18 PM** |
| §3.6 | r42-δ (parity 残機能 / vk-RC 直前 polish) work 工数 + 余裕係数 +50% | **2.25 PM** |
| §3.7 | r43-r44 (parity 補強 / 性能 polish / Mac portable subset 詳細化) work 工数 + 余裕係数 +50% | **3.00 PM** |
| §3.8 | r45+ (visual realism 次世代) 本算定範囲外宣言 | (予約のみ、charter §3/§6) |
| §3.9 | **milestone work sum (Linux baseline)** + a-3 工数感との突合 | **base 21.07 PM / 余裕係数適用後 29.90 PM** |

### 1.2 06-effort-estimation.md §4 3 OS per-OS 増分 draft (本 session 後半)

| sub-section | 内容 | 算出値 |
|---|---|---|
| §4.0 | 算定方針 (Linux baseline + Win/Mac 増分の OS 軸別積算、§3.7 内 Mac/Win 計上分は重複排除) | — |
| §4.1 | Linux first-class baseline 工数 (§3.9 sum の全体) | 21.07 PM / 29.90 PM |
| §4.2 | Win 増分 (LLWindow Win32 surface 化 + driver matrix 超過分 + 旧 driver fallback + LunarG SDK + Win-specific bug fix 余裕) | **base 1.35 PM / 余裕係数 +40% 適用後 1.89 PM** |
| §4.3 | Mac 増分 (LLWindow Mac surface 化 + MoltenVK 超過分 + UMA 対応 + 1.2 fallback + MSL 確認 + t-noami workflow + Mac bug fix 余裕) | **base 2.70 PM / 余裕係数 +50% 適用後 4.05 PM** |
| §4.4 | OS 別 milestone 着手 timing (r41 Linux only / r42-α Win 追加 / r42-β Mac 追加 / r44 で 3 OS parity 完遂) | — |
| §4.5 | **3 OS 合計** + a-3 / charter §4 (2) との突合 | **base 25.12 PM / 余裕係数適用後 35.84 PM** |

### 1.3 06 doc 内 navigation

- status header: `foundation + group A draft 完了 — §5-§8 は group B/C で順次 draft 予定` に更新
- §5 / §6 status: `次々々 group で draft` → `group B (次) で draft` に更新
- "次 step": group A 完了反映 + group B が次に確定
- "group A 算出値" 表を新規追加 (group B 以降の base 値として §5 暦月変換 input + §6 uncertainty band input)

---

## 2. group A 算出値 (group B 以降に引き継ぐ)

### 2.1 Linux baseline + 3 OS 統合 base 値

| 出処 | 工数 (PM、フルタイム dev、charter §4 (3) 本職並走 ratio + 学習曲線 適用前) |
|---|---|
| §3.9 milestone work sum (Linux baseline、余裕係数前) | **21.07 PM** |
| §3.9 milestone work sum (Linux baseline、余裕係数適用後 平均 +42%) | **29.90 PM** |
| §4.2 Win 増分 (余裕係数適用後 +40%) | **1.89 PM** |
| §4.3 Mac 増分 (余裕係数適用後 +50%) | **4.05 PM** |
| **§4.5 3 OS 合計 (余裕係数適用後)** | **~35.84 PM** |

### 2.2 a-3 工数感 + charter §4 (2) との突き合わせ

| 算定 | C++ + shader | Linux | Win 増分 | Mac 増分 | 3 OS 合計 |
|---|---|---|---|---|---|
| a-3 §5.4 + §B.x (base port + AYAstorm 3 機能、フルタイム dev) | 4.5 + 3.0 = 7.5 PM | 7.5 PM | (明示なし) | (明示なし) | (明示なし) |
| charter §4 (2) (Linux 先行 → Win/Mac 後追い) | — | (明示なし) | (明示なし) | (明示なし) | 3 OS 完遂のみ明示 |
| 本 §3 + §4 算出 (余裕係数適用後) | — | 29.90 PM | 1.89 PM | 4.05 PM | **35.84 PM** |
| 差 (本算定 − a-3) | — | +22.40 (+299%) | +1.89 (新規) | +4.05 (新規) | +28.34 (+378%) |

差の主因 (06 doc §3.9 + §4.5 で明文化):
1. **Linux baseline の per-file 精緻化** (foundation §1.7 + §2.5 で +5.42 PM、§1.7/§2.5 で明文化済)
2. **AYAstorm 3 機能 C++ + テスト残分** (a-3 §B.x で foundation 外計上指示済、§3.0 で取込 +3.65 PM)
3. **a-3 範囲外の新規 milestone** (r41.5 / r42-δ / r43-r44、charter §6 で確定済、+4.50 PM)
4. **余裕係数 +30-50% per-milestone 適用** (charter §4 (3) + 03 doc §5 算定軸 5、本 §3 で初適用、+6.58 PM)
5. **3 OS 増分の OS 軸新規可視化** (Win +1.89 / Mac +4.05、a-3 / charter §4 (2) では Linux 先行明示のみ、本 §4 で per-OS 増分を新規算定)
6. **Mac > Win の根拠** (MoltenVK 1.2 fallback / MSL 経由 / t-noami workflow cycle / macOS 14+ Metal 3 minimum、Mac 約 2.1 倍)

→ 本 §3 + §4 35.84 PM (フルタイム dev、3 OS 余裕係数適用後) は a-3 7.5 PM (Linux only / 余裕係数前) の per-milestone × 3 OS × 余裕係数 適用結果。

---

## 3. self-trace (group B 着手前の整合確認)

本 session で導入した算定値の前後 cross reference を確認:

| 項目 | 出処 | 整合 |
|---|---|---|
| foundation 12.92 PM の milestone 帰属 (r41 11.55 + r42-α 0.10 + r42-β 0.40 + r42-γ 0.87) | foundation §1.7 + §2.5 | ✓ (合計 12.92) |
| a-3 §B.1 picker 0.5 PM = foundation 0.10 (shader) + 追加 0.40 (C++) | a-3 §B.1 + foundation §2.3 注 | ✓ |
| a-3 §B.3 Cinematic 2-3 PM 中央値 2.5 = foundation 0.40 + 追加 1.85 = 2.25 | a-3 §B.3 + foundation §2.3 注 | ✓ (中央値内) |
| a-3 §B.2 visual realism 2-3 PM 中央値 2.5 = foundation 0.87 + 追加 1.40 = 2.27 | a-3 §B.2 + foundation §2.3 注 | ✓ (中央値内) |
| r41.5 / r42-δ / r43-r44 新規 milestone (a-3 範囲外、charter §6 確定) | charter §6 仮 line up + 05 doc §10 / §8 | ✓ |
| 余裕係数 +30-50% per-milestone 適用 (平均 +42%) | charter §4 (3) + 03 doc §5 算定軸 5 | ✓ |
| Win 増分 1.89 PM (余裕係数 +40%) | 05 doc §8.2 + §8.4 | ✓ |
| Mac 増分 4.05 PM (余裕係数 +50%) | 05 doc §8.3 + §9.4 + §8.4 | ✓ |
| §3.7 r43-r44 内 Mac 1.0 + Win 0.5 計上分との重複排除 (§4.2 で Win 0.30 / §4.3 で Mac 0.40 のみ超過分計上) | 06 doc §3.7 + §4.0 注 + §4.2/§4.3 注 | ✓ |
| OS 別 milestone 着手 timing (r41 Linux only / r42-α Win 追加 / r42-β Mac 追加 / r44 で 3 OS parity 完遂) | charter §4 (2) + a-4 §6.3.2 + 05 doc §8.5 | ✓ |
| r45+ 本算定範囲外宣言 | charter §3 / §6 + 05 doc §9.5 | ✓ |
| 3 OS 合計 35.84 PM = Linux 29.90 + Win 1.89 + Mac 4.05 (per-OS 重複排除済) | §4.5 sum | ✓ |

整合 ✓ (全 12 項目、cross reference 漏れなし)。

---

## 4. AYA review 待ちポイント

### 4.1 milestone 振り分けの妥当性

- r41 = 16.17 PM (foundation 大半 + 余裕係数 +40%) の集中度、妥当か
- r41.5 = 1.50 PM (構造 refactor のみ) の見積、license 分離手続 0.30 PM は楽観的すぎないか
- r42-δ = 2.25 PM (parity 残機能 / vk-RC 直前 polish) は a-3 範囲外の新規算定、過小 / 過大評価のリスクあり
- r43-r44 = 3.00 PM (Mac MoltenVK + Win driver matrix + 性能 polish) は §4.3 Mac 増分とどう重複排除しているかが §3.7 注 + §4.0 注 + §4.2/§4.3 注 で説明、追跡可能か

### 4.2 余裕係数の per-milestone 適用方針

- 平均 +42% (charter §4 (3) 30-50% 想定の中央寄り) は妥当か
- per-milestone 適用 (本 §3) vs 全体一括適用 の方針確認、本 §3 では per-milestone 採用 (handoff 4.3 で確認指示済の項目に対する暫定回答)

### 4.3 3 OS 増分の妥当性

- Win 増分 1.89 PM (Linux baseline 29.90 PM の 6.3%) は妥当か (Win は driver matrix が主、現 GL viewer の Win 対応経験を踏まえて)
- Mac 増分 4.05 PM (Linux baseline 29.90 PM の 13.6%、Win 約 2.1 倍) は MoltenVK + t-noami workflow + macOS 14+ minimum の制約を反映した値、妥当か
- §3.7 r43-r44 milestone work 内で Mac MoltenVK 詳細化 1.00 + Win driver matrix 0.50 計上済、§4.2/§4.3 では超過分のみ重複排除して合算した方針、妥当か

### 4.4 group B 着手前の方針確認

- §5 で本職並走 ratio (charter §4 (3) 想定 3-5x) を具体値に確定する際、§4.5 3 OS 合計 35.84 PM に何 x を掛けるか (ratio = 4x 想定で 143 暦月 = ~12 年、5x で ~15 年、charter §3 6-15 人年想定との整合は §5.5 で verify 予定)
- Vulkan 学習曲線の +20-30% (r41 序盤 + r41.5) を §5.2 でどう milestone 別に分配するか

---

## 5. 次 session 着手内容 (group B draft)

### 5.1 §5 各 milestone の所要月数 / 年数 (本職並走前提)

draft 予定 sub-section:
- §5.1 AYA 本職並走 ratio 確定 (フルタイム dev 1 人月 = AYA 並走 N 暦月、charter §4 (3) 想定 3-5x の精緻化)
- §5.2 Vulkan 学習曲線 反映 (r41 序盤 + r41.5 で +20-30% / r42+ 以降は inline 化想定)
- §5.3 milestone 別 所要暦月 (r41 / r41.5 / r42-α/β/γ/δ / r43 / r44) 中央値
- §5.4 milestone 累積 所要暦月 (r41 達成までの year scale + vk-RC parity 完遂までの total year scale)
- §5.5 charter §4 (3) 6-15 人年想定との整合性 verification
- §5.6 marker 暦年 (例: r41 達成は 202X 年頃) — 撤退条件には使わない (charter §8 (A))、進捗 marker のみ

### 5.2 §6 算定の uncertainty band (上方 / 下方)

draft 予定 sub-section:
- §6.1 不確実性要因の分類 (体制変動 / 技術選定 drift / 外部 dependency / scope creep / personal life event)
- §6.2 各要因の振れ幅 (中央値 ±%)
- §6.3 milestone 別 uncertainty band (r41 / r42-α/β/γ / r43-r44 / r45+ は範囲外)
- §6.4 累積 uncertainty band (r41 達成までの band / r44 vk-RC parity 完遂までの band)
- §6.5 上方 (最悪) / 中央 / 下方 (最良) の 3 シナリオ tabulation
- §6.6 band を縮める方策 (sub-milestone 区切り強化 / 早期 prototype / 並走外注検討の閾値)

### 5.3 group C は次々 session 以降

- group C (§7 Doom Blender 比較 + §8 plan B trigger): validation + 撤退条件

---

## 6. AYA review pattern

### Pattern A: group A 算出値そのまま OK → group B 着手

→ 次 session で §5 + §6 draft 着手 (group B)

### Pattern B: 算定方針 / milestone 振り分け / 余裕係数 / 3 OS 増分 を修正したい

→ 06 doc 該当 section を edit (差分 commit) → 修正後の §3.9 / §4.5 合計を本 handoff doc に反映 → group B 着手 base 値を確定

---

## 7. commit log (本 session)

- 前 session 末: `(work item (c) foundation group draft + handoff doc) 完了済 (commit 50d35b6aef)`
- 本 session: 06-effort-estimation.md §3 + §4 + navigation 更新 + 本 handoff doc を 1 commit で投入予定

---

## 8. 関連 doc / memory

### r40 章内部 doc

- `00-charter.md` — r40 章 charter (§4 (3) 6-15 人年 + 並走係数 / §6 r41.5 + vk-β/γ/δ/RC 仮 line up)
- `03-sub-phase-3-vulkan-plan.md` — work item (c) 親 doc (status 表更新済)
- `04-portage-inventory.md` — work item (a) 完了 (§5.4 段階 port 戦略 + §B.x AYAstorm 3 機能 = §3 input source、§6.3.2 OS 別 milestone 着手 timing = §4.4 input)
- `05-vulkan-api-design.md` — work item (b) 完了 (§8 OS 別 = §4.2/§4.3 input、§10 r41.5 skeleton = §3.2 input、§9.4 MoltenVK = §4.3 input)
- `06-effort-estimation.md` — work item (c) draft (foundation + group A 完了 ← 本 session)

### memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory (group A 完了反映を本 session 末に追記予定)
- `project_ayastorm_three_platforms.md` — 3 OS 大前提 (§4.0 算定方針で参照)
- `feedback_proactive_handoff.md` — group 境界 handoff の根拠
- `feedback_self_verify_before_handoff.md` — group 完了時 self-trace の根拠 (本 §3 で実施)
- `feedback_explanation_lead_with_conclusion.md` — handoff doc 構成 (結論ファースト)
- `feedback_credit_t_noami_equal_billing.md` — Mac t-noami workflow の根拠 (§4.3 で参照)
