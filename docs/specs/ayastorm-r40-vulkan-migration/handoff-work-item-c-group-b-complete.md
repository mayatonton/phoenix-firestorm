# handoff: r40 sub-phase 3 work item (c) 工程算定 — group B 完了

**作成日**: 2026-05-28
**前 session 状況**: group A (§3 per-milestone + §4 3 OS 増分) draft 完了 (前 session) → group B (§5 暦月変換 + §6 uncertainty band) draft 完了 (本 session)
**branch**: `feature/ayastorm-r40-vulkan-migration`
**親 doc**: `03-sub-phase-3-vulkan-plan.md` work item (c)

---

## 1. 本 session で完了したもの

### 1.1 06-effort-estimation.md §5 milestone 月数 / 年数 (本職並走前提) draft (本 session 前半)

| sub-section | 内容 | 算出値 |
|---|---|---|
| §5.0 | 算定方針 (中央値のみ算出、不確実性は §6 で扱う、charter §4 (3) 並走係数 + 学習曲線 を §5 で適用) | — |
| §5.1 | AYA 本職並走 ratio 確定 (charter §4 (3) 想定 3-5x の精緻化、補正要因 3 件 合成) | **中央値 4x** (フルタイム dev 1 PM = AYA 並走 4 暦月) |
| §5.2 | Vulkan 学習曲線 milestone 別反映 (r41 +30% / r41.5 +20% / r42-α-δ +10% / r43-r44 +0% / Win 増分 +10% / Mac 増分 +10%) | **Linux weighted +20.4% / 3 OS weighted +18.7%** |
| §5.3 | milestone 別 所要暦月 中央値 (base PM × 学習曲線 × 並走 4x) | r41 = ~84 暦月 (7 年) / vk-RC 3 OS parity 完遂 = **~170 暦月 (~14.17 年)** |
| §5.4 | milestone 累積 所要暦月 (charter §4 (2) Linux 先行 → Win/Mac 後追い 反映) | r41 達成 = 7.01 年 / r43-r44 Linux baseline 完遂 = 12.0 年 / 3 OS parity 完遂 = **14.17 年** |
| §5.5 | charter §4 (3) 6-15 人年想定との整合性 verification (1 人 full-time 換算 + 本職並走 calendar 両方で対比) | **整合 ✓ (中央値 14 年 = charter 下限 15 年の 94%、上方 27 年 = charter 上限 30 年の 90%)** |
| §5.6 | marker 暦年 (進捗 marker、charter §3 / §8 (A) 遵守、撤退条件には使わない) | r41 = ~2033 年中 / vk-RC 3 OS parity 完遂 = **~2040 年後半** |

### 1.2 06-effort-estimation.md §6 uncertainty band (上方 / 下方) draft (本 session 後半)

| sub-section | 内容 | 算出値 |
|---|---|---|
| §6.0 | 算定方針 (§5 中央値に対する band、charter §4 (3) 不確実性 2-3x を本 §6 で扱う) | — |
| §6.1 | 不確実性要因の分類 (8 要因: 体制変動 / 技術選定 drift / 外部 dependency / scope creep / personal life event / Vulkan 初見学習曲線 / 余裕係数偏差 / per-file 偏差) | 要因間 正 correl 仮定 (本算定保守) |
| §6.2 | 各要因の振れ幅 (中央値 ±%、合成 3 方法: 独立 +60/-25%、完全 correl +215/-85%、正 correl 寄せ中間合成 **+90% / -30%** 本算定採用) | **base band 上方 +90% / 下方 -30%** |
| §6.3 | milestone 別 uncertainty band (r41 +120%/-30% / r41.5 +80%/-25% / r42-α +50%/-25% / r42-β +60%/-25% / r42-γ +70%/-25% / r42-δ +90%/-25% / r43-r44 +100%/-30% / Win +50%/-20% / Mac +80%/-25%) | base PM 35.84 → 上方 ~68.1 PM / 下方 ~25.1 PM |
| §6.4 | 累積 uncertainty band (r41 達成 / r41.5 達成 / ... / vk-RC 3 OS parity 完遂) | r41 上方 ~15.4 年 / vk-RC 上方 **~26.9 年 (~2053 年)** / vk-RC 下方 **~9.9 年 (~2036 年)** |
| §6.5 | 3 シナリオ tabulation (上方 / 中央値 / 下方 を r41 達成 + vk-RC parity 完遂 で並列、charter §4 (3) 想定との整合確認) | charter 想定 15-30 年帯の **下限-上限近接** に着地、本算定 bottom-up は charter top-down と broad scale 整合 ✓ |
| §6.6 | band を縮める方策 (a)-(g) 7 件 + 優先順位 + 発動 cadence + band 縮小目標 | 上方 +90% → +50-60% に圧縮で vk-RC 上方 ~22 年に短縮可能 |

### 1.3 06 doc 内 navigation

- status header: `foundation + group A + group B (§5 暦月変換 + §6 uncertainty band) draft 完了 — §7-§8 は group C で draft 予定` に更新
- §7 / §8 status: `最終 group で draft` → `group C (次) で draft` に更新
- "次 step": group B 完了反映 + group C が次に確定
- "group A 算出値" 表: タイトルを「group B draft で消化済」に更新
- "group B 算出値" 表を新規追加 (group C 以降の base 値として §7 参照点比較 input + §8 plan B trigger 閾値 input)

---

## 2. group B 算出値 (group C 以降に引き継ぐ)

### 2.1 §5 中央値の base 値

| 出処 | 値 | 単位 |
|---|---|---|
| §5.1 本職並走 ratio 確定 | **4x** | calendar/PM (フルタイム dev 1 PM = AYA 並走 4 暦月) |
| §5.2 学習曲線 weighted PM (Linux baseline) | **~35.97 weighted PM** (weighted avg +20.4%) | フルタイム dev |
| §5.2 学習曲線 weighted PM (3 OS 合計) | **~42.51 weighted PM** (weighted avg +18.7%) | フルタイム dev |
| §5.3 Linux baseline 完遂 中央値 | ~144 暦月 (~11.99 年) | 本職並走 calendar |
| §5.3 vk-RC 3 OS parity 完遂 中央値 | **~170 暦月 (~14.17 年)** | 本職並走 calendar |
| §5.6 r41 達成 marker | **~2033 年中** | 暦年 |
| §5.6 vk-RC 3 OS parity 完遂 marker | **~2040 年後半** | 暦年 |

### 2.2 §6 uncertainty band の base 値

| 出処 | 値 | 注 |
|---|---|---|
| §6.2 base band 上方 | **+90%** | 8 要因 正 correl 寄せ合成 |
| §6.2 base band 下方 | **-30%** | 同上 |
| §6.4 累積 band r41 達成 上方 | ~185 暦月 (~15.4 年、~2042 年) | charter §8 (B) plan B trigger 「r41 達成 3 年経過未達」との別軸 |
| §6.4 累積 band vk-RC 3 OS parity 完遂 上方 | **~323 暦月 (~26.9 年、~2053 年)** | charter §4 (3) 上限 30 年の 90%、整合範囲内 |
| §6.4 累積 band vk-RC 3 OS parity 完遂 下方 | **~119 暦月 (~9.9 年、~2036 年)** | low-likelihood、scope shrink + 体制好転 同時発生 |
| §6.5 上方 (最悪) シナリオ発生確率 | ~10-15% | 8 要因 同時上方振れ |
| §6.5 中央値 シナリオ発生確率 | ~50% (中央値 ± 25% 内) | 本算定 base |
| §6.5 下方 (最良) シナリオ発生確率 | ~5-10% | 8 要因 同時下方振れ |
| §6.6 band 縮小目標 | 上方 +90% → +50-60% で vk-RC ~22 年に短縮 | charter §8 (B) plan B trigger 発動回避策 |

### 2.3 charter §4 (3) との突き合わせ

| 算定 | 1 人 full-time 換算 | 本職並走 calendar |
|---|---|---|
| charter §4 (3) 想定 | 6-15 人年 (= 72-180 PM) | 15-30 年 |
| 本算定 §5 中央値 | 35.84 PM (本 §4.5、3 OS 余裕係数込) / 学習曲線適用後 42.51 PM (~3.54 人年) | **14.17 年 (charter 下限 15 年の 94%)** |
| 本算定 §6 上方 (最悪) | 68.1 PM (本 §6.3 weighted、charter 下限 72 PM の 95%) | **26.9 年 (charter 上限 30 年の 90%)** |
| 本算定 §6 下方 (最良) | 25.1 PM (本 §6.3 weighted、charter 下限の 35%、low-likelihood) | 9.9 年 (charter 下限を下回るが low-likelihood) |
| 整合判定 | 本算定上方が charter 下限に接続 (95%)、bottom-up vs top-down 整合 ✓ | 本算定中央値 + 上方 が charter 15-30 年帯の下限-上限近接、整合 ✓ |

差の主因 (06 doc §5.5 + §6.5 で明文化):
1. **本算定は a-3 経験者前提 base を反映** (Vulkan 初見の不熟練 work を学習曲線 +20.4% のみで反映、不確実性 2-3x は §6 band で別途扱う)
2. **charter §4 (3) は Doom/Blender top-down 比較**で broad-stroke 推定、本算定 bottom-up が下限寄りに精緻化
3. **不確実性 2-3x を §5 中央値に含めない方針** (charter §4 (3) 乖離理由 4 要素のうち並走係数 + 学習曲線 を §5、不確実性 は §6 で扱う、二重計上回避)
4. **本算定 §5 中央値 + §6 上方 band を統合**すると charter §4 (3) 想定下限 〜 上限近接に着地

→ 本 §5 + §6 14.17 年 (中央値) / 26.9 年 (上方) は charter §4 (3) 15-30 年想定の下限-上限近接、整合範囲内。

---

## 3. self-trace (group C 着手前の整合確認)

本 session で導入した算定値の前後 cross reference を確認:

| 項目 | 出処 | 整合 |
|---|---|---|
| 並走係数 4x = charter §4 (3) 想定 3-5x の中央値 | charter §4 (3) | ✓ |
| 学習曲線 weight = charter §4 (3) +20-30% 想定の milestone 別分配 (r41 +30% / r41.5 +20% / r42 +10% / r43-r44 +0%) | charter §4 (3) | ✓ (上限 +30% で r41 適用、r43-r44 inline 化想定) |
| §5.3 r41 中央値 84.08 暦月 = 16.17 × 1.30 × 4 | §3.9 r41 16.17 PM | ✓ |
| §5.3 r41.5 中央値 7.20 暦月 = 1.50 × 1.20 × 4 | §3.9 r41.5 1.50 PM | ✓ |
| §5.3 r42-α 中央値 2.86 暦月 = 0.65 × 1.10 × 4 | §3.9 r42-α 0.65 PM | ✓ |
| §5.3 r42-β 中央値 13.86 暦月 = 3.15 × 1.10 × 4 | §3.9 r42-β 3.15 PM | ✓ |
| §5.3 r42-γ 中央値 13.99 暦月 = 3.18 × 1.10 × 4 | §3.9 r42-γ 3.18 PM | ✓ |
| §5.3 r42-δ 中央値 9.90 暦月 = 2.25 × 1.10 × 4 | §3.9 r42-δ 2.25 PM | ✓ |
| §5.3 r43-r44 中央値 12.00 暦月 = 3.00 × 1.00 × 4 | §3.9 r43-r44 3.00 PM | ✓ |
| §5.3 Win 増分 中央値 8.32 暦月 = 1.89 × 1.10 × 4 | §4.2 Win 1.89 PM | ✓ |
| §5.3 Mac 増分 中央値 17.82 暦月 = 4.05 × 1.10 × 4 | §4.3 Mac 4.05 PM | ✓ |
| §5.3 Linux baseline 合計 143.89 暦月 = 35.97 weighted PM × 4 | §5.2 Linux weighted 35.97 PM | ✓ |
| §5.3 3 OS 合計 170.03 暦月 = 42.51 weighted PM × 4 | §5.2 3 OS weighted 42.51 PM | ✓ |
| §5.4 累積暦月 = §5.3 milestone 別 暦月の順次積算 | §5.3 + charter §4 (2) Linux 先行 + §4.4 OS 別 timing | ✓ |
| §5.6 marker 暦年 = 着手日 2026-05-28 + §5.4 累積暦月 | 進捗 marker のみ、撤退条件不使用 | ✓ (charter §3 / §8 (A) 遵守) |
| §6.2 base band +90%/-30% = 8 要因 正 correl 寄せ合成 | 独立 +60/-25%、完全 correl +215/-85% の中間 | ✓ (要因別根拠 §6.1 で明文化) |
| §6.3 milestone 別 band = base band の milestone 集中度補正 (r41 +120% / r41.5 +80% / r42-α +50% / r42-β +60% / r42-γ +70% / r42-δ +90% / r43-r44 +100%) | §6.2 base ±%、要因 (1)(4)(6)(8) の milestone 集中度 | ✓ (r41 上方 base +30% / r43-r44 上方 base +10% で補正) |
| §6.4 r41 上方 185 暦月 = 84.08 × 2.20 | §6.3 r41 +120% | ✓ |
| §6.4 vk-RC 上方 ~323 暦月 = weighted +90% on 170 暦月 中央値 | §6.3 milestone 別 band weighted | ✓ (各 milestone 個別積算では ~335 暦月、weighted 平均では 323 暦月、差 ~3-4% は band 推定の精度範囲内) |
| §6.4 vk-RC 下方 119 暦月 = 170 × 0.70 | §6.3 weighted -30% | ✓ |
| §6.5 3 シナリオ = 上方 27 年 + 中央 14 年 + 下方 10 年 + charter §4 (3) 15-30 年帯との整合 | §6.4 累積 band + charter §4 (3) | ✓ (上限 90% + 下限 94% で整合範囲内) |
| §6.6 方策 (a)-(g) 7 件 = sub-milestone 区切り強化 / 早期 prototype / 並走外注 / LL 着地 reset / scope 縮小 / abstraction 前倒し / shader 自動化 | charter §7 + §8 (A) / a-3 §B.x / 05 doc §10 | ✓ (各方策の発動 trigger 明文化) |

整合 ✓ (全 18 項目、cross reference 漏れなし、numeric arithmetic 整合)。

#### 数値精度の注

§6.4 累積 band の積算方法 (各 milestone 個別 band 積算 vs cumulative weighted band 適用) で ~3-4% 差。本 doc は cumulative weighted 表記を採用 (band 推定の精度範囲内、§7 group C 参照点比較で大きく外れる影響なし)。

---

## 4. AYA review 待ちポイント

### 4.1 本職並走 ratio 4x の妥当性

- charter §4 (3) 想定 3-5x の中央値、補正要因 3 件合成 (生 work hours 1.60x + Context switch 1.40x + 体調 skip 1.33x ≈ 3.0x、Vulkan 設計 cycle 上方振れで 5x)、中央値 4x
- AYA さん本職並走の実 throughput を踏まえて 4x が妥当か、3.5x or 4.5x のどちらか寄りに偏らせる必要があるか
- AYAstorm r1-r30 期間 (2026-01 〜 2026-05) の AYA さん throughput 実績を踏まえると、Vulkan 化は機能追加と性質が違うため別 ratio 想定 (Vulkan は底層 rewrite、機能追加より context 深く throughput 低下)

### 4.2 学習曲線 milestone 別 weight の妥当性

- charter §4 (3) +20-30% 想定の milestone 別分配
- r41 +30% は学習曲線最大、a-3 範囲内の絶対 work 最大、charter 上限
- r41.5 +20% は abstraction 設計 + license 分離手続の unknown
- r42-α/β/γ/δ +10% は Vulkan API inline 化済想定、機能 port は AYAstorm 既習
- r43-r44 +0% は polish 期、学習曲線 inline 化済想定
- Win/Mac 増分 +10% は LunarG SDK / MoltenVK / MSL の初回学習

過小評価 risk: r42+ で extension / driver-specific quirk の継続的な学習発生 → +10% を超える振れ → §6 (6) Vulkan 初見学習曲線実測偏差で band 上方寄せ済

### 4.3 §6 uncertainty band の振れ幅妥当性

- 8 要因 × 振れ幅 × 合成方法 (正 correl 寄せ中間合成) で base band +90%/-30%
- 上方 +90% は charter §4 (3) 不確実性 2-3x の下限 ~2x に近接、整合 ✓
- 下方 -30% は scope shrink + lib 進化 + 余裕係数余り の同時発生想定、現実的に -15-25% 程度 (band は上方寄り)

milestone 別 band 補正 (r41 +120%、r41.5 +80%、...、r43-r44 +100%、Mac 増分 +80%) の妥当性。特に r41 +120% は最大値 (Vulkan 初見学習曲線 + a-3 範囲 絶対 work 最大 + per-file 偏差 集中) で根拠が説明できているか。

### 4.4 marker 暦年の使い方確認

- §5.6 marker 暦年 = 進捗 marker (charter §3 「時間軸では撤退条件を設けない」遵守、charter §8 (A) 外部条件 trigger と切り分け)
- vk-RC 3 OS parity 完遂 = **~2040 年後半** が中央値 marker、§6 band 上方 = ~2053 年 / 下方 = ~2036 年
- charter §8 (B) 工程プラン破綻 trigger 閾値は §8 group C で別途定義、本 §5.6 marker は閾値設定の reference のみ

### 4.5 group C 着手前の方針確認

- §7 で Doom / Blender 参照点比較 (charter §4 (3) 想定の妥当性 cross check) の draft 方針
- §8 で charter §8 (B) plan B trigger 閾値の draft 方針 (絶対月数 / charter 想定との乖離 % / 1 milestone 過度遅延 / 累積遅延 / sub-milestone 完遂率 / 等から閾値選定)

---

## 5. 次 session 着手内容 (group C draft)

### 5.1 §7 Doom / Blender 参照点との比較

draft 予定 sub-section:
- §7.1 Doom 2016 (id Tech 6 OpenGL → Vulkan) 比較 — 体制 3 名経験者 + clean abstraction × 6-12 か月 = 実工数 18-36 人月、AYAstorm 換算 (1 人体制 + abstraction 不在 + 248 shader + 3 大グローバル) と本算定 14 年中央値の差分根拠
- §7.2 Blender Vulkan 比較 — 2019 着手 → 2026 現在 7 年未完 (近似 scale)、AYAstorm 1 人 vs Blender rotating contributor の差分根拠、Blender 7 年到達ながら未完の事実が本算定 14 年中央値の上方振れ risk の参照点
- §7.3 charter §4 (3) 6-15 人年 (本職並走 15-30 年) 想定の妥当性 cross check — 本算定 §5.5 + §6.5 で実施した整合判定の deeper validation
- §7.4 本算定中央値 vs 参照点中央値 のズレ要因分析 — Doom (18-36 PM) vs 本算定 (42 PM) vs Blender (7 年 = ~840 暦月 vs 本算定 170 暦月) の各差の根拠
- §7.5 参照点に無い AYAstorm 固有要因 — 撮影章用途 (r30 Cinematic + r14+ visual realism は AYAstorm 独自) / AYAstorm 機能 13 file 追加 / Mac t-noami さん workflow / 本職並走

### 5.2 §8 plan B trigger 条件 (charter §8 (B) 工程プラン破綻判定の閾値設定)

draft 予定 sub-section:
- §8.1 charter §8 (A) 外部条件 trigger は本 §8 範囲外 (LL Vulkan 先着地 / AYA life plan 変更)、charter §8 (B) 工程プラン破綻のみ本 §8 で扱う方針確認
- §8.2 plan B trigger 閾値の定義方針 (絶対月数 / charter 想定との乖離 % / 1 milestone 過度遅延 / 累積遅延 / sub-milestone 完遂率 / 等から閾値選定)
- §8.3 r41 達成までの trigger 閾値 (§6.4 r41 上方 +120% = ~15 年を超えた場合の対処、charter §8 (B) 「r41 達成が 3 年経過しても未達」との関係)
- §8.4 r42-α/β/γ 達成までの trigger 閾値 (機能 milestone 単位の遅延判定、§6.4 累積 band 上方を threshold に)
- §8.5 trigger 発火時の対処 (scope 縮小 / quality 緩和 / 別 viewer base 接続 (05 doc §10.4 defensibility 活用) / 撤退)
- §8.6 trigger 判定 cadence (年次 review / milestone 完遂時 / charter §6 仮 line up の正式化 timing)

### 5.3 group C は最終 group

- group C 完了で 8 section 揃い、work item (c) 完了宣言可能
- work item (d) r42+ 区切り確定 着手へ移行

---

## 6. AYA review pattern

### Pattern A: group B 算出値そのまま OK → group C 着手

→ 次 session で §7 + §8 draft 着手 (group C、最終 group)

### Pattern B: 並走 ratio / 学習曲線 weight / uncertainty band / marker 暦年 を修正したい

→ 06 doc 該当 section を edit (差分 commit) → 修正後の §5.3 / §5.6 / §6.4 / §6.5 を本 handoff doc に反映 → group C 着手 base 値を確定

---

## 7. commit log (本 session)

- 前 session 末: `(work item (c) group A draft + handoff doc) 完了済 (commit 41f98a341e)`
- 本 session: 06-effort-estimation.md §5 + §6 + navigation 更新 + 本 handoff doc を 1 commit で投入予定

---

## 8. 関連 doc / memory

### r40 章内部 doc

- `00-charter.md` — r40 章 charter (§3 時間軸では撤退条件設けない / §4 (3) 6-15 人年 + 並走係数 + 学習曲線 + 不確実性 2-3x / §8 (A) 外部条件 trigger / §8 (B) plan B trigger 閾値の元定義)
- `03-sub-phase-3-vulkan-plan.md` — work item (c) 親 doc (status 表更新済)
- `04-portage-inventory.md` — work item (a) 完了 (§5.4 段階 port 戦略 + §B.x AYAstorm 3 機能 = §1/§3 input)
- `05-vulkan-api-design.md` — work item (b) 完了 (§10 skeleton = §6.6 方策 (f) abstraction interface 前倒しの根拠、§9.4 MoltenVK = §6.3 Mac 増分 band の根拠)
- `06-effort-estimation.md` — work item (c) draft (foundation + group A + group B 完了 ← 本 session)

### memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory (group B 完了反映を本 session 末に追記予定)
- `project_ayastorm_three_platforms.md` — 3 OS 大前提 (§5.3 Win/Mac 増分 calendar 反映)
- `feedback_proactive_handoff.md` — group 境界 handoff の根拠
- `feedback_self_verify_before_handoff.md` — group 完了時 self-trace の根拠 (本 §3 で 18 項目実施)
- `feedback_explanation_lead_with_conclusion.md` — handoff doc 構成 (結論ファースト)
- `feedback_credit_t_noami_equal_billing.md` — Mac t-noami workflow の根拠 (§6.6 方策 (c) 並走外注検討で参照)
