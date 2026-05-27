# handoff: r40 sub-phase 3 work item (c) 工程算定 — foundation group 完了

**作成日**: 2026-05-28
**前 session 状況**: work item (c) skeleton + 算定方針 commit 済 (前 session) → foundation group (§1 per-file + §2 per-shader) draft 完了 (本 session)
**branch**: `feature/ayastorm-r40-vulkan-migration`
**親 doc**: `03-sub-phase-3-vulkan-plan.md` work item (c)

---

## 1. 本 session で完了したもの

### 1.1 06-effort-estimation.md §1 per-file 工数算定 draft (前 session 末で着手済)

| sub-section | 内容 | 算出値 |
|---|---|---|
| §1.1 | a-3 段階 port 戦略を per-file 単位に分解、verdict × LOC density 原単位表 | 要 port 0.05-0.10 / 要再設計 0.25-0.35 / pipeline.cpp 1.0 PM/file |
| §1.2 | indra/llrender/ 51 file 工数 (要 port 16 + 要再設計 10 + 不要 port 3 + 判定保留 5) | **3.53 PM** |
| §1.3 | pipeline.cpp + .h 15,954 LOC per-section | **1.05 PM** |
| §1.4 | lldrawpool*.cpp 13 file per-file | **0.97 PM** |
| §1.5 | llspatialpartition 0.50 + llviewershadermgr 0.45 + llvosky/llvowlsky 0.27 | **1.22 PM** |
| §1.6 | GL header wrapper 3 file 0.5 + platform-specific 10 file 0.5 | **1.0 PM** |
| §1.7 | **C++ critical path 合計** + a-3 4.5 PM との +73% 差分分析 | **~7.77 PM** |

### 1.2 06-effort-estimation.md §2 per-shader 工数算定 draft (本 session 主作業)

| sub-section | 内容 | 算出値 |
|---|---|---|
| §2.1 | per-shader 工数原単位の 3 階層分類 (A 素通り 0.005 / B 要修正 0.025 / C AYAstorm 改変別計上) | 階層別 per-file 工数 |
| §2.2 | directory 別 per-file 工数 (class1/deferred 120 + class1/interface 44 + class3/deferred 16 + class1/objects 14 + その他) | **2.32 PM** |
| §2.3 | AYAstorm 改変 13 file (r21.1 picker 2 / r30 Cinematic 4 / r14+ visual realism 7) shader 抽出 | **1.10 PM** |
| §2.4 | descriptor set 再設計 (05 doc §3) の全 261 file 波及 + 1 度の整備 | **1.73 PM** |
| §2.5 | **shader 合計** + a-3 工数感 (shader 抽出推定 1-1.5 PM) との整合確認 | **~5.15 PM** |

### 1.3 06 doc 内 navigation

- status header: `foundation group draft 完了` に更新
- "draft 進行状況の整理" → "次 step" → §2 完了反映 + foundation 算出値表追加 (group A 以降の base 値として)

---

## 2. foundation group 算出値 (group A 以降に引き継ぐ)

### 2.1 C++ + shader 統合 base 値

| 出処 | 工数 (PM、フルタイム dev、charter §4 (3) 本職並走 ratio + 学習曲線 適用前) |
|---|---|
| §1.7 C++ critical path | **7.77 PM** |
| §2.5 shader 合計 | **5.15 PM** |
| **foundation 合計** | **~12.92 PM** |

### 2.2 a-3 工数感との突き合わせ

| 算定 | C++ | shader | 合計 |
|---|---|---|---|
| a-3 §5.4.1 段階 1-5 + §B AYAstorm 3 機能 (shader 抽出 ~1-1.5 PM 推定) | 4.5 PM | ~1-1.5 PM | ~5.5-6 PM |
| 本 §1 + §2 算出 | 7.77 PM | 5.15 PM | **12.92 PM** |
| 差 | +73% | +245% (a-3 未明示分含む) | +110% (全体) |

差の主因 (06 doc §1.7 + §2.5 で明文化):
1. **a-3 が「main file 絞り」 vs §1 は「全 file 列挙」** (+1.5 PM)
2. **platform-specific 10 file の潜伏 cost 可視化** (+0.5 PM、§1.6)
3. **領域別 per-file 算出** (llspatialpartition / llviewershadermgr / llvosky/llvowlsky 個別、+0.72 PM)
4. **shader 側 SPIR-V cross compile + descriptor 反映を a-3 で未明示** (+4.05 PM、§2.2 + §2.4)
5. **判定保留 utility 計上** (+0.15 PM、§1.2)

→ a-3 4.5 PM は「最大コア絞りでの概算」、本 §1+§2 12.92 PM は「全 file 列挙 + shader 反映工数可視化」の **per-file 精緻化結果**。a-3 §5.4.3 「絶対値の精緻化は work item (c) で実施」の宣言と整合。

---

## 3. self-trace (次 group 着手前の整合確認)

本 session で導入した算定値の前後 cross reference を確認:

| 項目 | 出処 | 整合 |
|---|---|---|
| 要 port 16 file 6,518 LOC | 04 doc §6.2 (a-4) | ✓ |
| 要再設計 10 file 7,937 LOC | 04 doc §6.2 (a-4) | ✓ |
| pipeline.cpp + .h 15,954 LOC | 04 doc §6.2 (a-4) | ✓ |
| lldrawpool 13 file 7,907 LOC | 04 doc §6.2 (a-4) | ✓ |
| llspatialpartition 4,416 LOC | 04 doc §6.2 (a-4) | ✓ |
| llviewershadermgr 4,423 LOC | 04 doc §6.2 (a-4) | ✓ |
| llvosky+llvowlsky 2,198 LOC | 04 doc §6.2 (a-4) | ✓ |
| 188 file (wrapper 経由) | 04 doc §1.2 (a-1) | ✓ |
| platform-specific 10 file (llwindow 5 + media_plugins 5) | 04 doc §1.2 (a-1) 例外条項 | ✓ |
| shader 248 file | 04 doc §1.3 (a-1) | ✓ |
| shader directory 別 (class1/deferred 120 等) | 04 doc §1.3 (a-1) | ✓ |
| ~85% cross compile 通る見込 | 04 doc §1.3 (a-1) + 05 doc §2 | ✓ |
| AYAstorm 改変 13 file (picker 2 / Cinematic 4 / visual realism 7) | 04 doc §B.1-§B.3 (a-3) | ✓ |
| descriptor set 3 構成 (set=0/1/2) | 05 doc §3.1 | ✓ |
| sampler 206 個 分配 (30/80/96) | 05 doc §3.2 | ✓ |
| push descriptor / inline uniform block | 05 doc §3.1 + §9.1 | ✓ |

整合 ✓ (全 15 項目、cross reference 漏れなし)。

---

## 4. AYA review 待ちポイント

### 4.1 foundation 算出値の妥当性

- C++ 7.77 PM (a-3 4.5 PM 比 +73%) の差分根拠は 4 項目で説明 (06 doc §1.7)、納得感あるか
- shader 5.15 PM の a-3 未明示分 (+4.05 PM) は 新規可視化、過大計上していないか
- AYAstorm 改変 shader per-file 平均 0.085 PM (B 階層の 3.4 倍) は妥当か

### 4.2 算定 grain (per-file 単位での原単位化) の妥当性

- 要 port 0.05-0.10 PM / 要再設計 0.25-0.35 PM / pipeline.cpp 1.0 PM/file という階層分けは合理的か
- LOC density 補正 ±50% の閾値 (LOC 5K / GL call 40) は妥当か

### 4.3 group A 着手前の方針確認

- §3 milestone 積算で foundation §1+§2 = 12.92 PM を r41 / r41.5 / r42-α/β/γ/δ / r43 / r44 に振り分ける際、各 milestone への配分 + 余裕係数 +30-50% の適用方針 (per-milestone か全体一括か) を確認

---

## 5. 次 session 着手内容 (group A draft)

### 5.1 §3 per-milestone 工数積算

draft 予定 sub-section:
- §3.1 r41 (GL 除去 + Vulkan 空転) work 工数 + 余裕係数
- §3.2 r41.5 (VK repo 分離) work 工数 + 余裕係数
- §3.3 r42-α (r21.1 picker port) work 工数 + 余裕係数
- §3.4 r42-β (r30 Cinematic port) work 工数 + 余裕係数
- §3.5 r42-γ (r14+ visual realism port) work 工数 + 余裕係数
- §3.6 r42-δ (parity 残機能 / vk-RC 直前 polish) work 工数 + 余裕係数
- §3.7 r43-r44 (parity 補強 / 性能 polish / Mac portable subset 詳細化) work 工数 + 余裕係数
- §3.8 r45+ (visual realism 次世代 / ray tracing / HDR / GPU-driven) 範囲外
- §3.9 milestone 工数積算 sum (フルタイム dev 換算)

### 5.2 §4 3 OS per-OS 増分

draft 予定 sub-section:
- §4.1 Linux first-class baseline 工数 (§3 milestone work 工数の全体)
- §4.2 Win 追加 増分工数 (WSI win32 / driver matrix 対応 / Win-specific bug fix 余裕)
- §4.3 Mac 追加 増分工数 (MoltenVK 経由 + portable subset 制約対応 + t-noami さん移植 workflow との連携)
- §4.4 OS 別 milestone 着手 timing 反映
- §4.5 3 OS 合計 工数 (フルタイム dev 換算)

### 5.3 group B / group C は次々 session 以降

- group B (§5 月数 + §6 uncertainty): time 軸変換 (人月 → 暦月)、本職並走 ratio + 学習曲線 + 振れ幅
- group C (§7 Doom Blender 比較 + §8 plan B trigger): validation + 撤退条件

---

## 6. AYA review pattern

### Pattern A: foundation 算出値そのまま OK → group A 着手

→ 次 session で §3 + §4 draft 着手 (group A)

### Pattern B: 算定方針 / 個別工数 を修正したい

→ 06 doc 該当 section を edit (差分 commit) → 修正後の §1.7 / §2.5 合計を本 handoff doc に反映 → group A 着手 base 値を確定

---

## 7. commit log (本 session)

- 前 session 末: `(work item (b) 完了 commit + handoff doc) 完了済`
- 本 session: 06-effort-estimation.md skeleton + §1 + §2 draft (foundation group 完了) を 1 commit で投入予定 (本 handoff doc 同梱)

---

## 8. 関連 doc / memory

### r40 章内部 doc

- `00-charter.md` — r40 章 charter
- `03-sub-phase-3-vulkan-plan.md` — work item (c) 親 doc (status 表更新済)
- `04-portage-inventory.md` — work item (a) 完了 (§1+§2 の input source)
- `05-vulkan-api-design.md` — work item (b) 完了 (§2 descriptor set 反映 input)
- `06-effort-estimation.md` — work item (c) draft (foundation group 完了 ← 本 session)

### memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory (group A 着手前に foundation 完了を追記予定)
- `feedback_proactive_handoff.md` — group 境界 handoff の根拠
- `feedback_self_verify_before_handoff.md` — group 完了時 self-trace の根拠 (本 §3 で実施)
- `feedback_explanation_lead_with_conclusion.md` — handoff doc 構成 (結論ファースト)
