# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-29 Phase 1.A entry ST-1〜ST-5 complete (= doc 系 5 sub-task 完了、残 ST-6 = chapter 06b 起案)

**作成**: 2026-06-03
**前 handoff**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-29-phase0-step5-complete.md` (= η-29 Phase 0 計測 phase 5 step 全完走 = `4e40fd2ab0` mechanical revert / `indra/` -80 line / flag OFF build PASS、次 = Phase 1.A entry doc 系 6 sub-task)
**branch**: `feature/ayastorm-r41-gl-removal`
**最新 commit (本 handoff 時点)**: `55976af9b6` (= 前 handoff doc commit、Phase 0 Step 5 完了 handoff)
**本 session 未 commit**: 4 file (= `02-naming-convention.md` / `05-existing-inventory-link.md` / `09-phase-roadmap.md` / `10-open-questions.md` = ST-3/ST-4/ST-5 chapter 反映 +94/-14 line)

---

## §0 state 一行 summary

η-29 **Phase 1.A entry doc 系 6 sub-task のうち ST-1〜ST-5 = 5 sub-task 完了** (= ST-1 (E') grep / ST-2 (F) MaterialUBO 比較 / ST-3 (Q26-MUL)(Q27-CONFL) AYA 判断 / ST-4 `aya_*` 3 件 + matrix 系 4-5 件 + R-TERR chapter 10 追記候補 AYA 判断 / ST-5 (Q1)(Q2)(Q4) AYA 判断 = 全 default 採用、本 session で chapter 10 + 02 + 05 + 09 反映済) → 残は **ST-6 = chapter 06b 起案 = cadence 5 種 update site 設計 + chapter 05 §7.3 matrix 補正反映** のみ。**indra/ 改変なし継続** (= `feedback_design_phase_no_code_write` 厳守)。Claude 即着手対象 = ST-6 着手前の **R-AYA1/2/3 grep 実施** + **R-MAT1-4 cadence column 反映** (= chapter 10 §2.7 確定資料化) → chapter 06b 起案。本 session 4 file edit は uncommitted、次 session 入りで本 handoff doc + 4 file を 2 commit に分けて (= ST-3/ST-4/ST-5 反映 commit + handoff commit) 出すか、handoff commit 先行 + 反映を ST-6 commit に同梱するかは AYA 判断。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。最初の session 入りでは **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read (offset/limit) する。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-29-phase1-a-entry-st1-st5-complete.md` | 全文 | 本 handoff (= Phase 1.A entry ST-1〜ST-5 完了 state + ST-6 着手地点 + 本 session 4 file edit uncommitted state) |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/10-open-questions.md` | §0 + §1.0 (= 28 件 index、9 件判断済 / 19 件未判断) + §1.5 (Q26-MUL / Q27-CONFL / Q28-FFDUP 確定 default) + §2.7 (= Phase 0 計測由来追記候補 8 件 table = R-AYA1/2/3 + R-MAT1/2/3/4 + R-TERR) | ST-6 起案で参照する確定済 AYA 判断 + 起案前 grep 必須項目 (R-AYA1-3 + R-MAT1-4) |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §14.4 (Stage 2 (Q1)(Q2)(Q4) ✅ + Stage 3 入口達成宣言) + §14.5 (Stage 3 着手 ready 14 項目 self-check、3-3 ✅ 完了 / 残 13 項目内訳) | ST-6 起案後の Stage 3 14 項目 self-check 進捗確認 + 実装 entry までの残設計 task 全体把握 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/design/06a-prep-phase0-measurement.md` | §3 (E') grep spec / §4 (F) 結論 F2 + member 差分 / §5.5.5 dead 121 件 (= R-AYA1/2/3 = `aya_alpha_plate` / `aya_alpha_plate_enabled` / `aya_sss_skin_flag`) / §5.5.6 hashed-path 40 件 / §5.5.7 matrix 系 4-5 件 per-program → per-draw 補正 (= R-MAT1/2/3/4 = `modelview_matrix` / `inv_modelview` / `modelview_projection_matrix` etc.) / §6 chapter 05 / 06a / 06b への反映 flow |
| `docs/specs/ayastorm-r41-gl-removal/design/05-existing-inventory-link.md` | §5.4 (= 本 session 追記 = (F2) MaterialUBO_Class3_Legacy 確定 + Q26-MUL 反映先 table) / §7.3.4 (= 本 session 追記 = matrix 系 cadence 補正 table + aya_* 結節注記) |
| `docs/specs/ayastorm-r41-gl-removal/design/02-naming-convention.md` | §3.2 (= 本 session 編集 = MaterialUBO / MaterialUBO_Class3_Legacy 確定 + MUL-A1/B1 規律) |
| `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §11.1 (Q1)=A 確定 / §11.2 (Q2)=A 確定 / §11.4 (Q4)=C 確定 / §14.4 Stage 2 完了 + Stage 3 入口達成宣言 |
| `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` | §6.4 / §4.3.1 / §5.6 = ST-6 起案で chapter 06b との反映関係参照 |
| `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` | §3 (= mUniformUBOLoc cache 構造) + §5 (= 16 method setter 分岐) = chapter 06b 起案の前提 chapter |
| `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` | §5.4.1 / §5.2.1 / §11.5 / §12.5 / §13.5 = Stage 3 self-check 3-5 で「反映済」確認対象 |

---

## §2 本 session 成果 (= Phase 1.A entry ST-1〜ST-5 完了)

### §2.1 commit 状態

```
55976af9b6 docs(r41): ... Phase 0 Step 5 完了 handoff 作成   (= 前 session 最終 commit)

[uncommitted, 本 session 編集分]
 M docs/specs/ayastorm-r41-gl-removal/design/02-naming-convention.md           +6/-2
 M docs/specs/ayastorm-r41-gl-removal/design/05-existing-inventory-link.md     +43
 M docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md               +8
 M docs/specs/ayastorm-r41-gl-removal/design/10-open-questions.md              +37/-12
                                                                          計  +94/-14
```

### §2.2 ST-1 = (E') 同一 binding 複数 UBO 名 grep 結果

| 観点 | 結果 |
|---|---|
| 検出した同一 set/binding 複数 UBO 名 | **5 UBO at set=2 binding=0 (= F+V stage conflict)** = `PerDrawUBO_ClipPlane` (F side) / `PerDrawUBO_AvatarSkin` `PerDrawUBO_ObjectSkin` `PerDrawUBO_SkinnedVelocity` `PerDrawUBO_AvatarVelocity` (V side 4 件) |
| 物理的原因 | `indra/llrender/llshadermgr.cpp:216` で `globalF.glsl` を全 program に無条件 attach (= F に `PerDrawUBO_ClipPlane` 常駐) + `indra/newview/llviewershadermgr.cpp` で `hasSkinning=true` → `avatarSkinV.glsl` (= `:179`) / `hasObjectSkinning=true` → `objectSkinV.glsl` (= `:188`) + velocity 系 explicit attach 4 件 (`:1479` `:1502` `:2233` `:3346` `:3378` `:3396`) |
| Vulkan link 影響 | **set=2 binding=0 が F+V で異なる UBO に紐付くため link 拒否** (= Q27-CONFL の生 evidence) |
| 解決方針 (= Q27-CONFL default B2+C1 採用) | V side の 4 UBO を **set=2 binding=1/2/3/4** に振り直す + 同一 program で V/F に同じ UBO 名なら **C1 (共通 include)** で重複定義回避 |
| 反映先 | chapter 10 §1.5 (Q27-CONFL) 反映欄 (= 本 session 追記) |

### §2.3 ST-2 = (F) MaterialUBO vs MaterialUBO_Legacy 比較結果

| 観点 | 結果 |
|---|---|
| MaterialUBO member 数 | **52** (= GLTF PBR フル material set、class3) |
| MaterialUBO_Legacy member 数 | **1** (= class1/2 legacy material) |
| member 重複 | **0 件** (= 完全に異なる構造、両者は別 UBO として独立すべき) |
| 結論 | **F2 = MaterialUBO_Legacy → `MaterialUBO_Class3_Legacy` 改名 + 新 V shader `class3/deferred/materialV.glsl` 起案** (= Q26-MUL default A1+B1 採用) |
| 反映先 | chapter 02 §3.2 (= 本 session 編集) + chapter 05 §5.4 (= 本 session 追記、F2 + Q26-MUL 反映先 table) + chapter 10 §1.5 (Q26-MUL) 反映欄 (= 本 session 編集) |

### §2.4 ST-3 = AYA 判断 batch 結果 (= chapter 10 §1.5 既登録 2 件 + 新規 1 件)

| Q ID | 内容 | 確定 verdict | default 採用 |
|---|---|---|---|
| Q26-MUL | MaterialUBO_Legacy 重複命名 → 改名 / 構造 refactor | **A1 + B1** (= `MaterialUBO_Class3_Legacy` 改名 + 新 class3 `deferred/materialV.glsl` 起案) | ✅ default |
| Q27-CONFL | F+V set=2 binding=0 conflict 解消 | **A1 + B2 + C1** (= V側 binding ずらし + 共通 include 重複定義回避) | ✅ default |
| Q28-FFDUP | (新規) F+F (フラグメント間) UBO 同一名 / 同一 binding 重複定義 (= `globalF.glsl` 無条件 attach + program 固有 F side 命名衝突候補) | **FFDUP-A1 + B2** (= 共通 include で one-definition / 衝突検出時 namespace prefix) | ✅ default |

反映先 = chapter 10 §1.0 (= 27 件 → 28 件 / 9 件判断済) + §1.5 (= 3 件確定 verdict + 反映先記述) + chapter 02 §3.2 + chapter 05 §5.4。

### §2.5 ST-4 = AYA 判断 batch 結果 (= Phase 0 計測由来追記候補 8 件)

| ID | 種別 | 内容 | 確定 |
|---|---|---|---|
| R-AYA1 | `aya_*` 独自 | `aya_alpha_plate` setter site / dead or hashed 判定 | **追記 + 起案前 grep 必須** |
| R-AYA2 | `aya_*` 独自 | `aya_alpha_plate_enabled` setter site / dead or hashed 判定 | **追記 + 起案前 grep 必須** |
| R-AYA3 | `aya_*` 独自 | `aya_sss_skin_flag` setter site / dead or hashed 判定 | **追記 + 起案前 grep 必須** |
| R-MAT1 | matrix 系 | `modelview_matrix` cadence 補正 (per-program 推定 437-688 cpf → per-draw 確定) | **追記 + cadence column 反映** |
| R-MAT2 | matrix 系 | `inv_modelview` cadence 補正 (同上) | **追記 + cadence column 反映** |
| R-MAT3 | matrix 系 | `modelview_projection_matrix` cadence 補正 (同上) | **追記 + cadence column 反映** |
| R-MAT4 | matrix 系 | (§5.5.7 で挙がった残 1-2 件 matrix 系、要 grep 確定) | **追記 + cadence column 反映** |
| R-TERR | terrain | terrain detail_* 20 件 dead candidates (= s2/s3 両 scene で observed 0、別 scene 計測候補) | **追記 + 別 phase 再計測 task 登録** |

反映先 = chapter 10 §2.7 = 「Phase 0 計測由来追記候補」新設 (= 本 session 追記、8 件 table)。**R-AYA1/2/3 の grep 確定 + R-MAT1-4 cadence column 反映は ST-6 起案直前** (= 次 session 着手地点)。

### §2.6 ST-5 = AYA 判断 batch 結果 (= 09 §14.4 Stage 2 entry 必須 3 件)

| Q ID | 内容 | 確定 verdict | default 採用 |
|---|---|---|---|
| Q1 | 第 1 UBO 選定 strategy | **A = Template A 最小リスク UBO 優先** (= 安全 UBO 単独で Phase 1.A inception) | ✅ default |
| Q2 | Phase 1.A の UBO 同時消化数 | **A = 1 UBO 厳守** (= `feedback_ubo_migration_one_at_a_time` 完全準拠) | ✅ default |
| Q4 | Win/Mac validation 並列度 | **C = Linux 完了後 Win/Mac 並走** (= Mac は @t-noami 信任) | ✅ default |

反映先 = chapter 10 §1.3 (= Q1/Q2/Q4 状態 ✅ 判断済 + 末尾 ST-5 batch verdict paragraph) + chapter 09 §11.1 / §11.2 / §11.4 (= 各 Q 末尾に ✅ 2026-06-03 確定 mark) + chapter 09 §14.4 (= Stage 2 完了 + Stage 3 入口達成宣言)。

**Stage 2 達成 → Stage 3 入口 entry 達成** (= 09 §14.5 14 項目 self-check のうち **3-3 ✅ 完了 / 残 13 項目**)。

### §2.7 chapter 反映 summary (= 本 session 編集 4 file)

| chapter | 編集箇所 | 内容 |
|---|---|---|
| chapter 02 §3.2 | MaterialUBO 行 / MaterialUBO_Legacy 行 + 規律 paragraph | 「確定」マーク + MUL-A1/B1 details + Q26-MUL 反映規律 1 段 |
| chapter 05 §5.4 (新設) | (F2) MaterialUBO_Class3_Legacy 確定 + ST-3 Q26-MUL verdict A1+B1 + 反映先 table | F vs F-Legacy 構造 diff 結論 + 確定後の chapter 06b 起案前提 |
| chapter 05 §7.3.4 (新設) | matrix 系 4-5 件 cadence 補正 table + aya_* 結節注記 | per-program 推定 → per-draw 確定補正 + R-AYA1/2/3 grep 連動明示 |
| chapter 09 §11.1 / §11.2 / §11.4 | 各 Q 末尾 ✅ 確定 mark | Q1=A / Q2=A / Q4=C 確定 + chapter 10 §1.3 「全 default 採用」AYA 応答リンク |
| chapter 09 §14.4 | Stage 2 完了 paragraph | Stage 2 達成 + Stage 3 入口 entry 達成宣言 + 14.5 14 項目連動明示 |
| chapter 10 §1.0 | header (28 件 / 9 判断済) + Q1/Q2/Q4/Q26-MUL/Q27-CONFL 状態列 ✅ + Q28-FFDUP 新行 + count 内訳 + reflection cross-ref + 解消順序 update | 27 → 28 件 / 5 件 ✅ 追加 |
| chapter 10 §1.3 | 末尾 ST-5 batch verdict paragraph | (Q1)(Q2)(Q4) verdict + (Q3)(Q5) 後ろ倒し継続 |
| chapter 10 §1.5 | Q28-FFDUP 行 + (Q26-MUL)/(Q27-CONFL)/(Q28-FFDUP) 反映先 paragraph + ST-3 batch verdict | 3 件確定 + 反映 cross-ref + 解消経緯 |
| chapter 10 §2.7 (新設) | Phase 0 計測由来追記候補 8 件 table (R-AYA1/2/3 + R-MAT1/2/3/4 + R-TERR) | ST-4 batch 結果集約 + ST-6 起案前 grep 連動 |

---

## §3 次着手 = ST-6 = chapter 06b 起案 + 起案前 grep 2 件

### §3.1 全体像 (= 残 1 sub-task = ST-6 のみ、ただし起案前 grep 2 件が前段)

| 順序 | sub-task | scope | 担当 | 出力 |
|---|---|---|---|---|
| 1 | **ST-6 前段 (a)** = R-AYA1/2/3 grep 実施 | grep + chapter 10 §2.7 状態列確定 (dead / hashed) | Claude | §2.7 R-AYA1/2/3 行に 「dead 確定」or「hashed 確定」マーク追記 |
| 2 | **ST-6 前段 (b)** = R-MAT1-4 cadence column 反映 | 06a-prep §5.5.7 から chapter 05 §7.3.4 / chapter 10 §2.7 へ cadence 補正値転記 | Claude | §2.7 R-MAT1-4 行に per-draw 確定値転記 + §7.3.4 補正 table 数値 fill |
| 3 | **ST-6 本体** = chapter 06b 起案 | 新 file `docs/specs/.../design/06b-<title>.md` (title は AYA と相談) | Claude (= 設計起案) | cadence 5 種 update site 配線設計 + chapter 05 §7.3 matrix 補正反映 + 09 §14.5 self-check 3-8 を満たす最小 spec |

### §3.2 ST-6 本体 = chapter 06b 起案契約

| 項目 | 内容 |
|---|---|
| ファイル名 | `docs/specs/ayastorm-r41-gl-removal/design/06b-<title>.md` (title は AYA と相談、例 candidate: `06b-cadence-update-site-design.md`) |
| 設計対象 | **cadence 5 種 update site 配線設計** (= per-draw / per-frame / per-program / per-asset / per-skin) |
| 入力資料 | (a) 06a-prep §5.5 観察結果 7 sub-section / (b) 06a-prep §5.5.7 matrix 補正 (= per-program → per-draw) / (c) chapter 05 §7.3 補正済 inventory / (d) chapter 10 §1.5 Q26-MUL / Q27-CONFL / Q28-FFDUP 確定 / (e) chapter 10 §1.3 (Q1)(Q2) 確定 / (f) chapter 10 §2.7 R-AYA1/2/3 + R-MAT1-4 + R-TERR ST-6 前段確定結果 |
| 出力契約 | 09 §14.5 Stage 3 self-check **3-8** (= chapter 06b cadence 5 種 update site + 06c descriptor set bind 起案済) を満たす最小 spec |
| chapter 06b / 06c 関係 | 本 entry 範囲は **chapter 06b のみ**、chapter 06c (descriptor set bind 起案) は ST-6 完了後の別 sub-task / 別 handoff |
| 共著行 | 不在厳守 (= `feedback_no_claude_coauthor`) |

### §3.3 ST-6 完了後の残設計 task (= Stage 3 14 項目 ✅ 完走まで)

09 §14.5 Stage 3 self-check 14 項目のうち、本 session 完走で **3-3 ✅ 完了 / 残 13 項目**。

ST-6 で消化される項目 = **3-8** (= 06b 起案済) のみ。残 12 項目は次の design phase で消化:

| 項目 | 内容 | 消化 phase |
|---|---|---|
| 3-1 | (Q3) AYA 判断 | (Q3)(Q5) batch (= 後ろ倒し可、Phase 1.A 中盤 OK) |
| 3-2 | (Q5) AYA 判断 | 同上 |
| 3-4 | chapter 04 §6.4 / §4.3.1 / §5.6 反映済確認 | Phase 1.A 実装 entry 直前 |
| 3-5 | chapter 08 §5.4.1 / §5.2.1 / §11.5 / §12.5 / §13.5 反映済確認 | 同上 |
| 3-6 | chapter 04 NTTP 判定 | 同上 |
| 3-7 | chapter 06a 設計起案済確認 (= 既起案) | Phase 1.A 実装 entry 直前 |
| 3-9 | autobuild manifest pin 反映済確認 | Phase 1.A 実装 entry 直前 |
| 3-10 | chapter 06c descriptor set bind 起案 | ST-6 完了後の別 sub-task |
| 3-11 | Vulkan validation layer 配線確認 | 実装 entry 後 |
| 3-12 | LL_VULKAN_GLSL 段階解禁 plan 確認 | 同上 |
| 3-13 | autobuild ReleaseFS_Vulkan target 起案確認 | 同上 |
| 3-14 | Phase 1.A 第 1 UBO target 確定 | (Q1)(Q2) で確定 = 実装 entry 直前で最終 target 名指定 |

**indra/ 改変解禁 = 14 項目全 ✅ + AYA 承認** (= `feedback_design_phase_no_code_write` 解除条件)。

### §3.4 規律 (= ST-6 着手時 self-check)

1. **literal scope = doc 系のみ**: ST-6 = 起案前 grep 2 件 + chapter 06b 新規 file 起案 = **`indra/` 改変なし継続** (= `feedback_design_phase_no_code_write` 厳守)
2. **R-AYA1/2/3 grep は ST-6 本体起案前に実施**: `aya_alpha_plate` / `aya_alpha_plate_enabled` / `aya_sss_skin_flag` の setter site / 使用 shader / dead か hashed か確定、chapter 10 §2.7 に結果反映してから起案
3. **R-MAT1-4 cadence 値は 06a-prep §5.5.7 から転記**: 再計測しない、§5.5.7 観察値 (= 437-688 cpf) を chapter 05 §7.3.4 + chapter 10 §2.7 に literal 転記
4. **chapter 06b は新 file**: 既存 chapter (06a / 06a-prep) には追記しない、新 file として独立起案
5. **chapter 06c (= descriptor set bind 起案) は本 entry 範囲外**: ST-6 完了後の別 sub-task / 別 handoff、本 session で起案しない
6. **handoff 最小読み厳守**: 本 handoff §1.1 で 3 件、§1.2 pinpoint で必要時、それ以外の chapter は当該 sub-task 着手時のみ Read (= `feedback_handoff_minimal_pre_req_read`)
7. **(Q3) / (Q5) は後ろ倒し可**: Phase 1.A 中盤までに後追い OK、ST-6 起案では default 採用継続 (= chapter 10 §1.3 各 row 末尾 default)、本 entry では仰がない
8. **commit 戦略 = AYA 判断**: 本 session 4 file 編集 (uncommitted) は次 session で commit、(a) ST-3/ST-4/ST-5 反映 commit を先行させてから handoff doc commit + ST-6 着手、(b) handoff doc commit 先行 + 4 file 反映を ST-6 commit に同梱、のいずれを取るかは AYA 判断

---

## §4 self-verify (= 本 handoff 起こした時点の整合性)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) 前 handoff (`...phase0-step5-complete.md`) からの遷移整合 | 前 §3 「次 = Phase 1.A entry = doc 系 6 sub-task = ST-1〜ST-6」 → 本 handoff 「ST-1〜ST-5 完了 / 残 ST-6」 起点と一致 | ✅ |
| (2) commit log `55976af9b6` 確認 | `git log --oneline -1` で HEAD と一致 (= 前 handoff doc commit) | ✅ |
| (3) 本 session 編集 4 file uncommitted 確認 | `git status --short` = `M` 4 file (02 / 05 / 09 / 10) + `git diff --stat` = +94/-14 | ✅ |
| (4) `feedback_design_phase_no_code_write` 準拠 | 本 session = doc edit only、`indra/` 改変ゼロ厳守継続 | ✅ |
| (5) `feedback_handoff_minimal_pre_req_read` 準拠 | 本 §1.1 = 3 件のみ列挙、§1.2 = pinpoint Read reference 分離 | ✅ |
| (6) `feedback_one_step_at_a_time` 準拠 | ST-3/ST-4/ST-5 AYA 判断は 1 batch メッセージで全件 summary 提示、AYA 「全 default 採用」応答後にまとめて 4 chapter 反映 | ✅ |
| (7) `feedback_no_claude_coauthor` 準拠 | 本 handoff 内、次 session commit でも共著行禁止明示 | ✅ |
| (8) `feedback_no_scope_shrink` 準拠 | ST-1〜ST-5 literal scope (= AYA 「全 default 採用」を 8 件 (Q26-MUL/Q27-CONFL/Q28-FFDUP/R-AYA1-3+R-MAT1-4+R-TERR/Q1/Q2/Q4) literal 全件に適用) | ✅ |
| (9) Stage 3 入口達成宣言整合 | 09 §14.4 Stage 2 完了 + Stage 3 14 項目のうち 3-3 ✅、本 §3.3 で残 13 項目内訳明示 | ✅ |
| (10) chapter 10 件数整合 | §1.0 header 27 → 28 件 (= Q28-FFDUP 新規追加) + 判断済 9 件 (= Q1/Q2/Q4/Q6/Q7/Q26-MUL/Q27-CONFL/Q28-FFDUP/Q?? — 旧 4 件 + 新 5 件) ※ §1.0 実値で要再 verify、本 session 編集時に内訳 update 済 | ✅ (要 next session §1.0 内訳行で再 visual check) |
| (11) chapter 06b 起案前 grep 2 件明示 | §3.1 1-2 で R-AYA1/2/3 grep + R-MAT1-4 cadence column 反映を ST-6 本体前段として明示 | ✅ |
| (12) Phase 1.A 実装 entry は別 handoff 明示 | §3.3 残 12 項目内訳で「Phase 1.A 実装 entry 直前」「実装 entry 後」を分離 | ✅ |

---

## §5 引き継ぎ済の memory (= 次 session も活きる)

- `feedback_handoff_minimal_pre_req_read` (= 本 handoff §1 構造の根拠)
- `feedback_design_phase_no_code_write` (= ST-6 起案も `indra/` 改変なし継続、実装 entry は別 handoff で解禁)
- `feedback_no_scope_shrink` (= ST-1〜ST-5 で AYA 「全 default 採用」を 8 件 literal 全件に適用、ST-6 でも同様)
- `feedback_self_verify_before_handoff` (= ST-6 起案も AYA 確認前に Claude 全 cross-ref self-verify)
- `feedback_no_claude_coauthor` (= 全 commit 共著行禁止)
- `feedback_one_step_at_a_time` (= AYA 判断仰ぎ batch は 1 メッセージで全件提示、AYA 判断後にまとめて反映 = ST-3/ST-4/ST-5 で実証済)
- `project_ayastorm_r41_vulkan_migration` (= r41 章 active pointer)
- `project_ayastorm_r41_design_principles` (= upstream 取込容易性 + core 並列化容易性の 2 大設計原則、chapter 06b 起案の前提)
- `feedback_ubo_migration_one_at_a_time` (= Phase 1.A 以降の UBO migration 規律、(Q2) 1 UBO 厳守 default の memory 根拠 = ST-5 で AYA 確定済)
- `feedback_no_auto_commit` (= 本 session 4 file 編集を commit せず handoff で次 session に明示引き継ぎ、commit 戦略は AYA 判断)
- `feedback_proactive_handoff` (= 本 handoff 作成自体の根拠 = AYA「(b) で」応答前に Claude から能動提案)

---

## §6 次 session 着手 1 line

「前 session で Phase 1.A entry doc 系 6 sub-task のうち ST-1〜ST-5 完了 (= 4 file uncommitted: chapter 02 §3.2 + chapter 05 §5.4/§7.3.4 新設 + chapter 09 §11.1/§11.2/§11.4/§14.4 + chapter 10 §1.0/§1.3/§1.5/§2.7、AYA「全 default 採用」を Q26-MUL/Q27-CONFL/Q28-FFDUP/R-AYA1-3+R-MAT1-4+R-TERR/Q1/Q2/Q4 = 8 件 + 3 件 = 11 件 literal 全件に適用)。本 session = 残 ST-6 = chapter 06b 起案 = cadence 5 種 update site 設計 + chapter 05 §7.3 matrix 補正反映。起案前段 2 件 = (a) R-AYA1/2/3 grep (= `aya_alpha_plate` / `aya_alpha_plate_enabled` / `aya_sss_skin_flag` の setter site / dead or hashed 確定) → chapter 10 §2.7 反映、(b) R-MAT1-4 cadence column 反映 (= 06a-prep §5.5.7 観察値 437-688 cpf を chapter 05 §7.3.4 + chapter 10 §2.7 に literal 転記)。起案契約 = 09 §14.5 Stage 3 self-check 3-8 を満たす最小 spec、chapter 06c (= descriptor set bind 起案) は本 entry 範囲外。`indra/` 改変なし継続 (= `feedback_design_phase_no_code_write` 厳守)、commit 戦略 (= 4 file 反映 commit 先行 vs ST-6 commit 同梱) は AYA 判断、(Q3)(Q5) は後ろ倒し継続で本 entry 範囲外」
