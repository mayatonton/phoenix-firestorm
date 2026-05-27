# handoff: r40 sub-phase 3 work item (c) 工程算定 — group C 完了 + work item (c) 全 §1-§8 draft 完成

**作成日**: 2026-05-28
**前 session 状況**: group B (§5 暦月変換 + §6 uncertainty band) draft 完了 (前 session) → group C (§7 Doom/Blender 比較 + §8 plan B trigger) draft 完了 (本 session) = **work item (c) 全 §1-§8 draft 完成**
**branch**: `feature/ayastorm-r40-vulkan-migration`
**親 doc**: `03-sub-phase-3-vulkan-plan.md` work item (c)

---

## 1. 本 session で完了したもの

### 1.1 06-effort-estimation.md §7 Doom / Blender 参照点との比較 draft (本 session 前半)

| sub-section | 内容 | 結論 |
|---|---|---|
| §7.0 | 算定方針 (charter §4 (3) 参照点 (Doom + Blender) と本算定 §5/§6 の cross check、本算定 broad scale 妥当性 verification、§8 trigger 閾値設定の参照基盤) | — |
| §7.1 | Doom 2016 (id Tech 6 OpenGL → Vulkan) 比較 (体制 3 名経験者 + clean abstraction × 6-12 か月 = 実工数 18-36 PM)、本算定 35.84 PM (3 OS 余裕係数込) | **Doom 上限 36 PM の 99%、broad scale 整合 ✓** |
| §7.2 | Blender Vulkan 比較 (2019 着手 → 2026 現在 7 年未完、推定残 3-5 年 = 10-12 年規模)、本算定 14.17 年 | **本算定 +2-4 年 (体制 1 名固定 / abstraction 後追い / 3 OS parity / AYAstorm 撮影描画用途) で説明可能、broad scale 整合 ✓** |
| §7.3 | charter §4 (3) 6-15 人年 / 15-30 年並走想定の妥当性 cross check | **中央値 14 年 = charter 下限 15 年の 94% + 上方 27 年 = charter 上限 30 年の 90%、下限-上限 範囲内** |
| §7.4 | 本算定中央値 vs 参照点 のズレ要因分析 (Doom +99% / Blender +2-4 年 / charter 下限近接 / 上限近接) | abstraction 不在 / 13 file 追加 / 3 OS / 本職並走 / 単独 1 名体制 の 5 要因で説明可能 |
| §7.5 | 参照点に無い AYAstorm 固有要因 5 件 ((i) 撮影描画 +6.33 PM / (ii) 13 file +5.6 PM / (iii) Mac +4.05 PM / (iv) 並走 4x / (v) 1 名体制) | 参照点との差を **明示的に説明可能**、本算定 §6 band +90% 上方の根拠 |

### 1.2 06-effort-estimation.md §8 plan B trigger 条件 draft (本 session 後半)

| sub-section | 内容 | 結論 |
|---|---|---|
| §8.1 | charter §8 (A)(C)(D)(E) との切り分け方針 (本 §8 は (B) 工程プラン破綻のみ、(A)(D)(E) は外部条件で別途扱い、(C) AYA life plan は AYA 判断) | (B) のみ本算定 §5/§6 から定量閾値設定可能 |
| §8.2 | plan B trigger 閾値の定義方針 (5 軸: (a) 絶対暦月 / (b) 中央値乖離 % / (c) §6 上方 band 上限 / (d) portage 規模乖離 / (e) sub-milestone 完遂率) | 単独軸 = warning、複数軸 = trigger 発動、(c) 単独で trigger 発動 |
| §8.3 | r41 達成までの trigger 閾値 (5 軸別表、warning + trigger 発動) | warning 3 年 (charter §8 (B) 例 1) / trigger 10 年 (中央値 7 年の 143%) / band 上限突破 15 年 |
| §8.4 | r41.5 / r42-α/β/γ/δ / r43-r44 / vk-RC trigger 閾値 (機能 milestone 別 7 milestone 表) | 各 milestone 中央値 × 1.30-1.41 = warning、§6 上方 band 上限 × 1.05-1.25 = trigger、vk-RC 累積 28 年 (上方 27 年の 104%) で trigger |
| §8.5 | trigger 発火時の対処 5 案 ((α) scope 縮小 / (β) quality 緩和 / (γ) 別 viewer base 接続 / (δ) LL 着地 reset / (ε) 撤退) + 優先順位 | (δ) > (α) > (γ) > (β) > (ε)、判断は AYA さん、Claude は判断材料提供 |
| §8.6 | trigger 判定 cadence 4 種 (annual review / milestone 完遂時 / 棚卸し再評価時 / emergency) + §6.6 方策との整合 | annual = (a) sub-milestone 区切り強化 / milestone 完遂時 = 全 (a)-(g) / 棚卸し再評価 = (g) shader 自動化 / emergency = (α)-(ε) 発動 |
| §8.7 | progress log (operational placeholder、本 (c) 完了宣言には含めない) | annual / milestone / emergency 各 placeholder のみ |

### 1.3 06 doc 内 navigation

- status header: `foundation + group A + group B + group C draft 完了 — work item (c) 全 §1-§8 draft 完成 (AYA review → work item (d) r42+ 区切り確定 着手)` に更新
- "次 step" 表: group C 完了反映 + work item (c) 完了宣言 → work item (d) 着手が次に確定
- "group B 算出値" 表: タイトルを「group C draft で消化済」に更新、§7/§8 への引継ぎ用途を各行に追記
- "group C 算出値" 表を新規追加 (work item (c) 完了宣言の妥当性根拠 + work item (d) r42+ 区切り確定 の input)

---

## 2. group C 算出値 (work item (c) 完了宣言 base 値 + work item (d) の input)

### 2.1 §7 参照点比較の整合判定

| 出処 | 値 | 結論 |
|---|---|---|
| §7.1 Doom 整合判定 | 本算定 35.84 PM = Doom 18-36 PM 上限の **99%** | broad scale 整合 ✓ |
| §7.2 Blender 整合判定 | 本算定 14.17 年 = Blender 10-12 年規模との **+2-4 年差** | broad scale 整合 ✓ (体制 / abstraction / 3 OS / 撮影描画用途 で説明可能) |
| §7.3 charter §4 (3) cross check | 中央値 14 年 = 下限 94% + 上方 27 年 = 上限 90% | charter 想定 15-30 年帯の **下限-上限 範囲内**、整合 ✓ |
| §7.4 ズレ要因 5 要因 | abstraction 不在 / 13 file 追加 / 3 OS / 本職並走 / 単独 1 名体制 | 全ズレが **明示的に説明可能** |
| §7.5 AYAstorm 固有要因 | (i) 撮影描画 +6.33 PM / (ii) 13 file +5.6 PM / (iii) Mac +4.05 PM / (iv) 並走 4x / (v) 1 名体制 | 参照点に無い work +16 PM + calendar 4x 拡大 + band +90% 上方 に直接寄与 |

### 2.2 §8 plan B trigger 閾値の確定値

| 出処 | 値 | 用途 |
|---|---|---|
| §8.2 trigger 閾値 5 軸 | (a) 絶対暦月 / (b) 中央値乖離 % / (c) §6 上方 band 上限 / (d) portage 規模乖離 / (e) sub-milestone 完遂率 | charter §8 (B) plan B 判定の運用 base |
| §8.3 r41 trigger | warning 3 年 (charter §8 (B) 例 1 継承) / trigger 10 年 / **band 上限突破 15 年** | r41 着手後 annual review base |
| §8.4 r41.5 trigger | warning 8 か月 (中央値 7.2 の 110%) / trigger 16 か月 (band 上限 13 の 123%) | r41.5 着手後 milestone 完遂時 review base |
| §8.4 r42-α trigger | warning 4 か月 (中央値 2.86 の 140%) / trigger 8 か月 (band 上限 4.3 の 186%) | r42-α 着手後 milestone 完遂時 review base |
| §8.4 r42-β trigger | warning 18 か月 (中央値 13.86 の 130%) / trigger 30 か月 (band 上限 22.2 の 135%) | r42-β 着手後 milestone 完遂時 review base |
| §8.4 r42-γ trigger | warning 18 か月 (中央値 13.99 の 129%) / trigger 32 か月 (band 上限 23.8 の 134%) | r42-γ 着手後 milestone 完遂時 review base |
| §8.4 r42-δ trigger | warning 14 か月 (中央値 9.90 の 141%) / trigger 26 か月 (band 上限 18.8 の 138%) | r42-δ 着手後 milestone 完遂時 review base |
| §8.4 r43-r44 trigger | warning 16 か月 (中央値 12 の 133%) / trigger 30 か月 (band 上限 24 の 125%) | r43-r44 着手後 milestone 完遂時 review base |
| §8.4 vk-RC 累積 trigger | warning 18 年 (中央値 14 の 129%) / **trigger 28 年 (band 上限 27 の 104%、charter 上限 30 年の 93%)** | r40 章 evaluation の最大閾値 |
| §8.5 対処 5 案 | (α) scope 縮小 / (β) quality 緩和 / (γ) 別 viewer base 接続 / (δ) LL 着地 reset / (ε) 撤退 | trigger 発動時の AYA 判断材料 |
| §8.5 対処優先順位 | (δ) > (α) > (γ) > (β) > (ε) | trigger 発動時の Claude 推奨順 |
| §8.6 判定 cadence | annual review (5 月末) + milestone 完遂時 + 棚卸し再評価時 + emergency | r41 着手後の運用 base |

### 2.3 work item (c) 完了宣言の妥当性根拠

| 観点 | 確認 |
|---|---|
| §1-§4 棚卸し → per-unit/per-milestone 工数 確定 | ✓ foundation §1.7 = 7.77 PM + §2.5 = 5.15 PM、group A §3.9 Linux baseline = 29.90 PM (余裕係数込) + §4.5 3 OS 合計 = 35.84 PM (余裕係数込) |
| §5 中央値 暦月変換 確定 | ✓ group B §5.3 vk-RC 3 OS parity 完遂 = 170 暦月 (14.17 年)、§5.6 marker r41 = ~2033 年中 / vk-RC = ~2040 年後半 |
| §6 uncertainty band 確定 | ✓ group B §6.2 base band +90% / -30%、§6.4 累積 vk-RC 上方 27 年 / 下方 10 年、§6.6 band 縮小方策 (a)-(g) 7 件 |
| §7 参照点 cross check | ✓ group C §7.1 Doom 99% 整合、§7.2 Blender +2-4 年 broad scale 整合、§7.3 charter §4 (3) 下限-上限 範囲内 |
| §8 plan B trigger 確定 | ✓ group C §8.2 trigger 5 軸、§8.3-§8.4 milestone 別閾値、§8.5 対処 5 案、§8.6 判定 cadence 4 種 |

→ work item (c) は **全 §1-§8 draft 完成、AYA review 待ち**。AYA review PASS で work item (c) 完了宣言 → work item (d) r42+ 区切り確定 着手。

---

## 3. self-trace (work item (c) 完了宣言前の整合確認)

本 session で導入した算定値の前後 cross reference を確認:

### 3.1 §7 numeric 整合

| 項目 | 出処 | 整合 |
|---|---|---|
| §7.1 本算定 35.84 PM = Doom 上限 36 PM の 99% | §4.5 3 OS 合計 + charter §4 (3) Doom 18-36 PM | ✓ |
| §7.1 abstraction 不在 +1-2 PM = pipeline.cpp 3 大グローバル → frame context 集約 work | §3.1 r41 5.5 PM 内訳 (pipeline.cpp 5.5 PM のうち 3 大グローバル refactor は ~1-2 PM 推定) | ✓ |
| §7.1 AYAstorm 13 file +5-6 PM = §3.3 r42-α + §3.4 r42-β + §3.5 r42-γ 合計 | §3.9 r42-α 0.65 + r42-β 3.15 + r42-γ 3.18 = ~7.0 PM (うち shader 部分 + 機能 port 合算で 5-6 PM の broad estimate) | ✓ |
| §7.1 3 OS 増分 +6 PM = §4.2 Win 1.89 + §4.3 Mac 4.05 = 5.94 PM | §4.2 + §4.3 | ✓ |
| §7.2 Blender 84 暦月 (7 年) 未完 = 2019 着手 → 2026 現在 | 一般公表情報 (Blender 4.x release note) | ✓ |
| §7.2 本算定 14.17 年 / Blender 10-12 年規模 = +2-4 年差 | §5.3 vk-RC 中央値 14.17 年 + Blender 推定残 3-5 年 = 10-12 年 | ✓ |
| §7.3 charter §4 (3) 下限 72 PM = 6 人年 × 12 か月 | charter §4 (3) | ✓ |
| §7.3 本算定上方 68.1 PM = 72 PM × 95% | §6.3 3 OS 合計 上方 68.1 PM / charter §4 (3) 下限 72 PM | ✓ |
| §7.3 本算定中央値 14.17 年 = charter 下限 15 年 × 94% | §5.3 / charter §4 (3) 下限 15 年 | ✓ |
| §7.3 本算定上方 26.9 年 = charter 上限 30 年 × 90% | §6.4 vk-RC 上方 26.9 年 / charter §4 (3) 上限 30 年 | ✓ |
| §7.5 (i) 撮影描画 +6.33 PM = §3.4 r42-β 3.15 + §3.5 r42-γ 3.18 | §3.9 r42-β + r42-γ | ✓ |
| §7.5 (ii) 13 file +5.6 PM = §2.3 0.6 + §3.3-§3.5 5 PM | §2.3 + §3.3-§3.5 | ✓ |
| §7.5 (iii) Mac +4.05 PM = §4.3 Mac 増分 余裕係数込 | §4.3 | ✓ |

### 3.2 §8 numeric 整合

| 項目 | 出処 | 整合 |
|---|---|---|
| §8.3 r41 warning 3 年 = charter §8 (B) 例 1 「3 年経過未達」継承 | charter §8 (B) | ✓ |
| §8.3 r41 trigger 10 年 = 中央値 7 年 × 143% | §5.3 r41 中央値 84 暦月 (7 年) | ✓ |
| §8.3 r41 band 上限突破 15 年 = §6.4 r41 上方 15.4 年 | §6.4 | ✓ |
| §8.4 r41.5 warning 8 か月 = 中央値 7.2 か月 × 110% | §5.3 r41.5 中央値 7.20 暦月 | ✓ |
| §8.4 r41.5 trigger 16 か月 = §6 上方 band 13 か月 × 123% | §5.3 r41.5 中央値 7.20 × (1+0.80) = 13 暦月 | ✓ |
| §8.4 r42-α warning 4 か月 = 中央値 2.86 × 140% | §5.3 r42-α 中央値 2.86 | ✓ |
| §8.4 r42-α trigger 8 か月 = band 上限 4.3 か月 × 186% | §5.3 r42-α × (1+0.50) = 4.29 暦月 | ✓ |
| §8.4 r42-β warning 18 か月 = 中央値 13.86 × 130% | §5.3 r42-β 中央値 13.86 | ✓ |
| §8.4 r42-β trigger 30 か月 = band 上限 22.2 か月 × 135% | §5.3 r42-β × (1+0.60) = 22.18 暦月 | ✓ |
| §8.4 r42-γ warning 18 か月 = 中央値 13.99 × 129% | §5.3 r42-γ 中央値 13.99 | ✓ |
| §8.4 r42-γ trigger 32 か月 = band 上限 23.8 か月 × 134% | §5.3 r42-γ × (1+0.70) = 23.78 暦月 | ✓ |
| §8.4 r42-δ warning 14 か月 = 中央値 9.90 × 141% | §5.3 r42-δ 中央値 9.90 | ✓ |
| §8.4 r42-δ trigger 26 か月 = band 上限 18.8 か月 × 138% | §5.3 r42-δ × (1+0.90) = 18.81 暦月 | ✓ |
| §8.4 r43-r44 warning 16 か月 = 中央値 12 × 133% | §5.3 r43-r44 中央値 12.00 | ✓ |
| §8.4 r43-r44 trigger 30 か月 = band 上限 24 か月 × 125% | §5.3 r43-r44 × (1+1.00) = 24.00 暦月 | ✓ |
| §8.4 vk-RC warning 18 年 = 中央値 14 × 129% | §5.3 vk-RC 中央値 14.17 年 | ✓ |
| §8.4 vk-RC trigger 28 年 = §6 上方 band 27 年 × 104% | §6.4 vk-RC 上方 26.9 年 | ✓ |
| §8.5 対処 5 案 = (α)-(ε) + 優先順位 (δ)>(α)>(γ)>(β)>(ε) | charter §7 + §8 + §6.6 方策 | ✓ |
| §8.6 cadence 4 種 = annual + milestone + 棚卸し + emergency | charter §8 (B) + §6.6 方策 | ✓ |

### 3.3 数値精度の注

- §8.3 r41 trigger (a) 絶対暦月 = 10 年 / (b) 中央値乖離 trigger = 12 年 で逆転矛盾しないか確認: (a) 単独 trigger / (b) 「進捗 X% 以下」併用 trigger の二重判定で、(a) trigger 発火 (10 年) が (b) trigger (12 年) より先に来る順序整合 ✓
- §8.4 vk-RC 累積 trigger 28 年 = charter §4 (3) 上限 30 年の 93%、charter 上限を超えない閾値設定で plan B 発動を charter 想定範囲内に閉じる設計 ✓
- §6.4 累積 band の cumulative weighted 積算と各 milestone 個別 band 積算の差 ~3-4% は band 推定の精度範囲内、§7/§8 で大きく外れる影響なし ✓

整合 ✓ (§7: 13 項目 / §8: 21 項目、cross reference 漏れなし、numeric arithmetic 整合)。

---

## 4. AYA review 待ちポイント

### 4.1 §7 参照点比較の妥当性

#### Doom 比較 (§7.1)

- 体制 3 名経験者 × 6-12 か月 = 18-36 PM = charter §4 (3) 提示値
- 本算定 35.84 PM が Doom 上限 36 PM の 99% (整合 ✓) が AYAstorm 規模感として妥当か
- 「abstraction 不在 +1-2 PM」「AYAstorm 13 file +5-6 PM」「3 OS 増分 +6 PM」「余裕係数 +30-50%」の合計が **Doom 上限近接** の合理的説明として成立しているか

#### Blender 比較 (§7.2)

- Blender 2019-2026 = 7 年未完 + 推定残 3-5 年 = 10-12 年規模 の broad estimate が妥当か (Blender 公表情報の解釈次第)
- 本算定 14.17 年 = Blender 10-12 年 + 2-4 年差 が「体制 1 名固定 / abstraction 後追い / 3 OS parity / AYAstorm 撮影描画用途」で説明可能か
- AYAstorm が Blender より +2-4 年長いことが **decidedly 楽観寄り** ではない確信が持てるか (Blender でさえ 7 年で未完)

#### charter §4 (3) cross check (§7.3)

- 本算定中央値 14 年 = charter 下限 15 年の 94% で **下限近接** が妥当か (charter 下限を下回ることへの懸念)
- 本算定上方 27 年 = charter 上限 30 年の 90% で **上限近接** が妥当か (charter §4 (3) 不確実性 2-3x の下限 ~2x との整合)
- 本算定上方 68.1 PM = charter 下限 72 PM の 95% で **本算定上方が charter 下限に接続** する整合が妥当か

### 4.2 §8 plan B trigger 閾値の妥当性

#### r41 trigger (§8.3)

- warning 3 年 = charter §8 (B) 例 1 継承で問題ない
- **trigger 10 年** は中央値 7 年の 143%、band 上限 15.4 年の 65%。「中央値の 1.4x で trigger」の保守度合いが適切か (より広めの 12 年 = 中央値 171% も検討余地あり)
- band 上限突破 15 年 trigger は §6.4 r41 上方 15.4 年と一致、本算定の最悪シナリオ上限突破 = 算定 base 完全外し signal として明確

#### 機能 milestone trigger (§8.4)

- 各 milestone trigger 閾値 = §6 上方 band 上限の 105-186% の幅で設定 (短期 milestone は trigger margin 大 = r42-α の 186%、長期 milestone は trigger margin 小 = r42-β の 135%)
- r42-α trigger 186% の margin は「foundation 帰属が大半で long delay は別要因」の反映、reasonable か
- vk-RC 累積 trigger 28 年 = charter §4 (3) 上限 30 年の 93%、上限を超えない閾値設定で plan B を charter 想定範囲内に閉じる方針が妥当か

#### 対処 5 案 + 優先順位 (§8.5)

- 対処 5 案 (α)-(ε) の網羅性: scope 縮小 / quality 緩和 / 別 viewer base / LL reset / 撤退 で十分か
- 優先順位 (δ) > (α) > (γ) > (β) > (ε) の妥当性: 「LL 着地 reset 最優先」「撤退最終手段」が AYA 判断の方向性と整合するか
- (γ) 別 viewer base 接続 = r41.5 達成後の defensibility 活用、Mac 等の specific OS 局所適用 が現実的な選択肢か

#### 判定 cadence (§8.6)

- annual review (毎年 5 月末 = r40 着手 2026-05-28 周年) の cadence が妥当か
- milestone 完遂時 review + 棚卸し再評価時 review + emergency の 4 種で運用十分か
- judgement 主体 = AYA さん、Claude = 判断材料提供 (charter §7 整合) で問題ないか

### 4.3 work item (c) 完了宣言の妥当性

- §1-§8 全 draft が揃った状態で work item (c) 完了宣言 → work item (d) r42+ 区切り確定 着手 で問題ないか
- 残された operational 区分 (§8.7 progress log) は (c) 完了宣言には含めない、運用 phase で継続更新 で問題ないか
- 06 doc 全体 (~1937 行) の AYA review が必要か、それとも group 単位の review (§1-§2 foundation / §3-§4 group A / §5-§6 group B / §7-§8 group C) で十分か

---

## 5. 次 session 着手内容 (work item (c) 完了宣言 → work item (d) draft)

### 5.1 AYA review pattern

#### Pattern A: 全 §1-§8 そのまま OK → work item (c) 完了宣言 → work item (d) 着手

- 06 doc を確定状態に切替 (status header の「draft 完了」→「確定」)
- work item (c) 完了宣言を 03-sub-phase-3-vulkan-plan.md の work item 表に反映
- work item (d) r42+ 区切り確定 draft 着手 (07-r42-plus-milestone-mapping.md 仮称 + handoff)

#### Pattern B: §7 / §8 の 一部 を修正したい

- 06 doc 該当 section を edit (差分 commit) → 修正後の §7/§8 結論を本 handoff doc に反映 → work item (c) 完了宣言再判定

#### Pattern C: §1-§6 まで遡って修正したい

- 該当 group の handoff doc (foundation / group A / group B / group C) を参照 → 該当 section edit → 影響範囲確認 (§3 → §5 → §6 → §7 / §8 への propagation 確認) → 修正後に work item (c) 完了宣言再判定

### 5.2 work item (d) r42+ 区切り確定 draft の方針

charter §6 仮 line up + a-4 §6.3.2 AYAstorm 機能 pull-in 順 (r42-α picker / r42-β Cinematic / r42-γ visual realism) を **正式 mapping** する。

draft 予定の content:

- **§1 r42 区切りの algorithm 化**: charter §6 仮 line up (vk-α / vk-β / vk-γ / vk-δ / vk-RC) + AYAstorm 機能 pull-in 順 (r42-α/β/γ/δ) の正式 mapping、05 doc §10 skeleton (r41.5) との時系列整合
- **§2 r42 milestone 内訳**: r42-α (picker port) / r42-β (Cinematic) / r42-γ (visual realism) / r42-δ (parity 残機能 / vk-RC 直前 polish) の sub-milestone 構成、各 sub-milestone acceptance criteria draft
- **§3 r43-r44 区切り**: r43 (parity 補強 + Mac portable subset 詳細化) / r44 (性能 polish + 全機能 parity 完遂 = vk-RC) の区切り、charter §6 vk-γ/δ/RC 流れとの mapping
- **§4 r45+ 区切り (本算定範囲外)**: visual realism 次世代 / ray tracing / HDR / GPU-driven 等の broad placeholder、charter §3 「時間軸では撤退条件設けない」遵守
- **§5 各 milestone の charter 草案 outline**: r41 charter / r41.5 charter / r42-α charter / r42-β charter / r42-γ charter / r42-δ charter / r43 charter / r44 charter の outline (各 milestone 着手前に詳細化)
- **§6 charter §6 仮 line up の本 §1-§5 反映**: charter §6 を本 (d) 確定 mapping で更新 (work item (d) → work item (e) charter 完成 への引継ぎ)

### 5.3 work item (d) と work item (e) の関係

- (d) 完了で 03-sub-phase-3-vulkan-plan.md + 00-charter.md §6 の正式 mapping 反映
- (e) charter 完成 = 00-charter.md final review + sub-phase 3 全 work item ((a)-(d)) の statement of completion で **r40 達成宣言**
- (e) 完了で **r40 章 close** → **r41 着手** (`docs/specs/ayastorm-r41-gl-removal/00-charter.md` 起草)

### 5.4 group C で確定した §8 plan B trigger を r41 charter 起草時に反映

- §8.6 判定 cadence の **annual review (5 月末) cadence** を r41 charter 起草時に組込
- §8.3 r41 trigger 閾値 (warning 3 年 / trigger 10 年 / band 上限 15 年) を r41 charter §8 plan B 節に反映
- §6.6 方策 (a) sub-milestone 区切り強化を r41 charter §5 acceptance criteria の sub-milestone 区切り設計に反映

---

## 6. AYA review pattern (再掲)

### Pattern A: group C 算出値そのまま OK → work item (c) 完了宣言 → work item (d) 着手

→ 次 session で work item (d) r42+ 区切り確定 draft 着手 (07-r42-plus-milestone-mapping.md 仮称)

### Pattern B: §7 / §8 の 一部 を修正したい

→ 06 doc 該当 section を edit (差分 commit) → 修正後の §7/§8 結論を本 handoff doc に反映 → work item (c) 完了宣言再判定

### Pattern C: §1-§6 まで遡って修正したい

→ 該当 group の handoff doc 参照 → 該当 section edit → 影響範囲確認 → 修正後に work item (c) 完了宣言再判定

---

## 7. commit log (本 session)

- 前 session 末: `(work item (c) group B draft + handoff doc) 完了済 (commit a38b1a66b9)`
- 本 session: 06-effort-estimation.md §7 + §8 + navigation 更新 + 本 handoff doc を 1 commit で投入予定

---

## 8. 関連 doc / memory

### r40 章内部 doc

- `00-charter.md` — r40 章 charter (§3 時間軸では撤退条件設けない / §4 (3) 6-15 人年 + 並走係数 + 学習曲線 + 不確実性 2-3x / §7 LL 着地時判断指針 / §8 (B) plan B trigger 閾値の元定義)
- `03-sub-phase-3-vulkan-plan.md` — work item (c) 親 doc (本 handoff で完了宣言提案、work item (d) 着手準備)
- `04-portage-inventory.md` — work item (a) 完了 (§5.4 段階 port 戦略 + §B.x AYAstorm 3 機能 = §1/§3/§7 input)
- `05-vulkan-api-design.md` — work item (b) 完了 (§10 skeleton = §6.6 方策 (f) abstraction interface 前倒し / §9.4 MoltenVK = §6.3 Mac 増分 band / §7.5 (i)(iii) AYAstorm 固有要因 の根拠)
- `06-effort-estimation.md` — work item (c) 全 §1-§8 draft 完成 (本 session)

### memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory (group C 完了 + work item (c) 完了反映を本 session 末に追記予定)
- `project_ayastorm_three_platforms.md` — 3 OS 大前提 (§7.5 (iii) Mac t-noami workflow / §8.4 r43-r44 trigger 反映)
- `feedback_credit_t_noami_equal_billing.md` — Mac t-noami workflow の根拠 (§7.5 (iii) で参照)
- `feedback_mac_only_fixes_accept_as_is.md` — Mac 限定 fix 受入の根拠 (§7.5 (iii) で参照)
- `feedback_proactive_handoff.md` — group 境界 handoff の根拠
- `feedback_self_verify_before_handoff.md` — group 完了時 self-trace の根拠 (本 §3 で 34 項目実施)
- `feedback_explanation_lead_with_conclusion.md` — handoff doc 構成 (結論ファースト)
- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 work item (c)(d)(e) 完了後に着手、§5.4 で §8 plan B trigger 反映方針を明記)
