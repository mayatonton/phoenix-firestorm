# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-29 Phase 1.A entry ST-6 complete (= doc 系 6 sub-task 全完走、Stage 3 14 項目 self-check 残 12 項目 → 次 = (Q3)(Q5) AYA 判断 + 残 self-check 消化)

**作成**: 2026-06-03
**前 handoff**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-29-phase1-a-entry-st1-st5-complete.md` (= η-29 Phase 1.A entry ST-1〜ST-5 完了 = 11 件 「全 default 採用」literal 全件適用 / 4 file +94/-14 / Stage 3 入口達成宣言 / 14.5 14 項目 self-check 3-3 ✅ 完了 残 13 項目)
**branch**: `feature/ayastorm-r41-gl-removal`
**最新 commit (本 handoff 時点)**: `e649d983b2` (= 前 handoff doc commit、Phase 1.A entry ST-1〜ST-5 完了 handoff)
**本 session 未 commit**: 4 file (= `05-existing-inventory-link.md` / `06b-cadence-update-site-and-dirty.md` / `09-phase-roadmap.md` / `10-open-questions.md` = ST-6 前段 (a)(b) + B 案 delta integration +48/-16 line)

---

## §0 state 一行 summary

η-29 **Phase 1.A entry doc 系 6 sub-task 全完走** (= ST-1〜ST-5 前 session / **ST-6 本 session 完了** = (a) R-AYA1/2/3 grep verdict = dead / dead / alive 確定 (= chapter 10 §2.7 反映) + (b) R-MAT1-4 cadence 値 literal 転記 (= R-MAT4 = `normal_matrix` 確定、当初候補 `modelview_projection_inverse` 撤回 = grep 0 件、chapter 05 §7.3.4 + chapter 10 §2.7 反映) + 本体 = chapter 06b/06c **既起案済 verify** + delta integration 4 件 (= §2.3 注 R-MAT1-4 反映 + §2.3 注 R-AYA3 反映 + §3.3.4 Q26-MUL 確定名 + §8.1 確定 cross-ref table 8 行)) → 09 §14.5 Stage 3 self-check **3-8 ✅ 完了** = 残 12 項目。**indra/ 改変なし継続** (= `feedback_design_phase_no_code_write` 厳守、本 session も doc edit only)。Claude 即着手対象 = **(Q3)(Q5) AYA 判断 batch** (= chapter 10 §1.3 後ろ倒し可項目、Stage 3 14.5 残 12 項目消化 phase の最初の sub-task)。本 session 4 file edit は uncommitted、次 session 入りで本 handoff doc + 4 file を 2 commit に分けて (= ST-6 反映 commit + handoff commit) 出すか、handoff commit 先行 + ST-6 反映 commit 同梱は AYA 判断。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。最初の session 入りでは **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read (offset/limit) する。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-29-phase1-a-entry-st6-complete.md` | 全文 | 本 handoff (= Phase 1.A entry ST-6 完了 state + Stage 3 残 12 項目消化 phase 着手地点 + 本 session 4 file edit uncommitted state) |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §14.4 (= Stage 2 完了 + Stage 3 入口達成宣言 + **ST-6 3-8 ✅ 完了 paragraph**) + §14.5 (= Stage 3 14 項目 self-check、**3-3 + 3-8 = 2 件 ✅ 完了 / 残 12 項目** 内訳) | 残 12 項目消化 phase 全体把握 + 次 sub-task 着手 priority 判断 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/10-open-questions.md` | §0 + §1.0 (= 28 件 index、9 件判断済 / 19 件未判断) + §1.3 ((Q3)(Q5) 後ろ倒し可項目) + §2.7 (= ST-6 前段 (a)(b) 反映済 R-AYA1-3 / R-MAT1-4 / R-TERR table、状態 column 確定 verdict 含む) | 次 sub-task = (Q3)(Q5) AYA 判断 batch 着手の前提 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md` | §2.2 (= Q27-CONFL 注追記済) / §2.3 (= R-MAT1-4 + R-AYA3 注追記済) / §3.3.4 (= Q26-MUL 確定名追記済) / §8.1 (= 確定 cross-ref table 8 行追記済) = 本 session B 案 delta integration 反映箇所 |
| `docs/specs/ayastorm-r41-gl-removal/design/06c-descriptor-set-bind-wiring.md` | §0-§11 structure (= 既起案済 513 line、ST-6 既起案 verify 対象、本 session は無編集) |
| `docs/specs/ayastorm-r41-gl-removal/design/05-existing-inventory-link.md` | §7.3.4 (= 本 session 編集 = R-MAT4 訂正 `normal_matrix` 4 件目確定 + `modelview_projection_inverse` 撤回 paragraph) |
| `docs/specs/ayastorm-r41-gl-removal/design/06a-prep-phase0-measurement.md` | §5.5.7 matrix 系 cadence 補正 観察値 (= 437-688 cpf、本 session §7.3.4 / §2.7 へ literal 転記済) |
| `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` | §6.4 / §4.3.1 / §5.6 = 残 self-check 3-4 で「反映済確認」対象 |
| `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` | §5.4.1 / §5.2.1 / §11.5 / §12.5 / §13.5 = 残 self-check 3-5 で「反映済確認」対象 |
| `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` | §3 (mUniformUBOLoc cache) + §5 (16 method setter 分岐) = 既起案済確認 (= 残 self-check 3-7) |
| `indra/llrender/llshadermgr.cpp` | `:1505-1512` (= LLShaderMgr canonical "matrix state" reserved uniforms 7 件 = `modelview_matrix` / `projection_matrix` / `inv_proj` / `modelview_projection_matrix` / `inv_modelview` / `identity_matrix` / `normal_matrix`) = R-MAT 確定根拠 |
| `indra/newview/app_settings/shaders/class1/deferred/avatarF.glsl` | `:74-84` (= `aya_sss_skin_flag` dual declaration `#ifdef LL_VULKAN_GLSL` UBO `AvatarFParamUBO_Legacy` set=3 binding=54 vs `#else uniform float`) = R-AYA3 alive 確定根拠 |

---

## §2 本 session 成果 (= Phase 1.A entry ST-6 完了)

### §2.1 commit 状態

```
e649d983b2 docs(r41): ... Phase 1.A entry ST-1〜ST-5 完了 handoff 作成   (= 前 session 最終 commit)

[uncommitted, 本 session 編集分]
 M docs/specs/ayastorm-r41-gl-removal/design/05-existing-inventory-link.md     +5/-4
 M docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md  +25
 M docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md               +4/-2
 M docs/specs/ayastorm-r41-gl-removal/design/10-open-questions.md              +14/-10
                                                                          計  +48/-16
```

### §2.2 ST-6 前段 (a) = R-AYA1/2/3 grep verdict

| ID | uniform 名 | grep 結果 | verdict | 根拠 |
|---|---|---|---|---|
| R-AYA1 | `aya_alpha_plate` | 0 件 (`indra/` 全域) | **dead** | setter site / shader 使用とも不在、追記候補から除外 |
| R-AYA2 | `aya_alpha_plate_enabled` | 0 件 (`indra/` 全域) | **dead** | 同上 |
| R-AYA3 | `aya_sss_skin_flag` | reserved enum 登録 + setter 4 site + shader `avatarF.glsl:74-84` 既使用 + Vulkan UBO `AvatarFParamUBO_Legacy` set=3 binding=54 既移植済 dual-path (= `#ifdef LL_VULKAN_GLSL` ↔ `#else uniform float` 切替) | **alive (= 第 3 category)** | dead / hashed の 2 分類では捕まらず、AYA 「dead / dead / alive で進めて」確認済 |

反映先 = chapter 10 §2.7 = 状態 column (= 6th column) 新設 + R-AYA1/2/3 行に確定 verdict mark + ST-6 前段 (a) 完了 paragraph 追記。

### §2.3 ST-6 前段 (b) = R-MAT1-4 cadence column 反映

| ID | uniform 名 | grep occurrences | cadence 確定 | 備考 |
|---|---|---|---|---|
| R-MAT1 | `modelview_matrix` | 多数 (canonical matrix state reserved uniform) | **per-draw** | 06a-prep §5.5.7 観察値 437-688 cpf |
| R-MAT2 | `inv_modelview` | 多数 (同上) | **per-draw** | 同上 |
| R-MAT3 | `modelview_projection_matrix` | 多数 (同上) | **per-draw** | 同上 |
| R-MAT4 | `normal_matrix` | 172 件 | **per-draw** | 当初候補 `modelview_projection_inverse` は grep 0 件で撤回 (= AYA 「A 案で進めて」= `normal_matrix` 単独 4 件目確定)、§5.5.7 「matrix 系 4-5 件」の 4 件目として確定 |

反映先 = chapter 10 §2.7 = R-MAT1-4 行に per-draw cadence column + 確定 mark + ST-6 前段 (b) 完了 paragraph 追記 / chapter 05 §7.3.4 = group 4 件目 `normal_matrix` 確定 row + `modelview_projection_inverse` 撤回 paragraph (= 「2026-06-03 ST-6 前段 (b) literal 転記完了 + R-MAT4 訂正」)。

### §2.4 ST-6 本体 = chapter 06b/06c 既起案済 verify + B 案 delta integration

**重要 = 設計起案 framing の修正**: 前 handoff §3.2 では ST-6 を「**新 file 起案**」と framing していたが、本 session 着手時点で **`06b-cadence-update-site-and-dirty.md` (441 line) + `06c-descriptor-set-bind-wiring.md` (513 line) は既存**、両 chapter とも 09 §14.5 self-check 3-8 (= cadence 5 種 update site + descriptor set bind 起案済) の要件を comprehensive に満たす。

AYA 「B 案で進めて」確認済 → **B 案 = verify + delta integration** = 既起案 chapter を verify した上で、ST-6 前段 (a)(b) findings + 関連 batch verdict を delta として追記。

| chapter / 編集箇所 | 追記内容 | 反映 source |
|---|---|---|
| 06b §2.2 per-program 注追記 | Q27-CONFL `A1+B2+C1` 確定 反映 = V/F stage 同 binding=0 共存 5 UBO を `set=2, binding=1/2/3/4` 振り直し + F 側 ClipPlane `binding=0` 維持 + Phase 1.A 入口 (= C1) LL_VULKAN_GLSL 有効化前全件解消 | chapter 10 §1.5 ST-3 batch verdict |
| 06b §2.3 per-draw 注追記 (R-MAT1-4 反映) | per-draw cadence 確定 4 件 = `modelview_matrix` / `inv_modelview` / `modelview_projection_matrix` / `normal_matrix` 列挙 + 437-688 cpf 観察値 + `modelview_projection_inverse` 撤回 | chapter 10 §2.7 / chapter 05 §7.3.4 ST-6 前段 (b) |
| 06b §2.3 per-draw 注追記 (R-AYA3 反映) | R-AYA3 alive 確定 + cadence per-draw (= avatar 描画毎切替 flag、`AvatarFParamUBO_Legacy` set=3 binding=54 経由) + R-AYA1/R-AYA2 dead 除外 | chapter 10 §2.7 ST-6 前段 (a) |
| 06b §3.3.4 Q26-MUL 確定名 (新設) | `MaterialUBO_Legacy` → `MaterialUBO_Class3_Legacy` 改名確定 + per-draw cadence 統合は両 prefix 共通の前提 + 命名差は cadence / dirty 機構に影響しない | chapter 10 §1.0 Q26-MUL ✅ / chapter 02 §3.2 MUL-A1/B1 規律 |
| 06b §8.1 確定 cross-ref (新設) | 確定 cross-ref table 8 行 = Q1 / Q2 / Q4 / Q26-MUL / Q27-CONFL / Q28-FFDUP / R-AYA1-3 / R-MAT1-4 の確定 verdict + 本 chapter 反映点 列挙 (= §8 持越 (K)(L)(M)(U1)(U2)(U3)(U4) と独立に、本 chapter 設計の前提条件確認用) | chapter 10 §1.0/§1.5 + chapter 09 §11.1/§11.2/§11.4 |
| 09 §14.5 self-check 3-8 行 | **✅ 2026-06-03 ST-6** mark inline (= 項目 cell 内、column count 整合) + 詳細 evidence (= chapter 06b/06c 既起案済 + ST-6 前段 delta integration 完了) | 09 §14.5 既存 table |
| 09 §14.4 paragraph 末尾 | ST-6 完了 summary 追記 + count update = **3-3 + 3-8 = 2 件 ✅ 完了 / 残 12 項目** | 前 handoff §14.4 (= 3-3 ✅ / 残 13 項目) からの遷移 |

### §2.5 chapter 06c (= descriptor set bind 起案) 本 session 状態

- **無編集** (= verify only)
- 既起案 513 line で §0-§11 structure 完備 = §2 descriptor set 4 帯 cadence 別配置 + §4 flush 直後 bind 配線で 09 §14.5 3-8 後半要件を物理的に満たす
- = 本 entry 範囲外、別途 chapter 06b verify と並行で structure 確認済として §2.4 表に「既起案済」記載
- = 09 §14.5 3-8 ✅ は 06b + 06c 両 chapter 既起案済の合成判定

### §2.6 chapter 反映 summary (= 本 session 編集 4 file)

| chapter | 編集箇所 | 内容 |
|---|---|---|
| chapter 05 §7.3.4 | group 4 件目 row 訂正 + 末尾 paragraph 追記 | R-MAT4 = `normal_matrix` 4 件目確定 / row 5 削除 (= `modelview_projection_inverse` 撤回) / 「2026-06-03 ST-6 前段 (b) literal 転記完了 + R-MAT4 訂正」paragraph |
| chapter 06b §2.2 | per-program table に注 row 1 件追加 | Q27-CONFL A1+B2+C1 確定反映 |
| chapter 06b §2.3 | per-draw table に注 row 2 件追加 | R-MAT1-4 反映 + R-AYA3 反映 |
| chapter 06b §3.3 | §3.3.4 新設 subsection | Q26-MUL `MaterialUBO_Class3_Legacy` 確定名反映 |
| chapter 06b §8 | §8.1 新設 subsection | 確定 cross-ref table 8 行 (Q1/Q2/Q4/Q26-MUL/Q27-CONFL/Q28-FFDUP/R-AYA1-3/R-MAT1-4) |
| chapter 09 §14.4 | 末尾 paragraph 追記 | ST-6 完了 summary + 「3-3 + 3-8 = 2 件 ✅ 完了 / 残 12 項目」count update |
| chapter 09 §14.5 | 3-8 行 項目 cell | inline `✅ 2026-06-03 ST-6` mark + detailed evidence |
| chapter 10 §2.7 | 8 件 table の状態 column (新設、6th column) + 末尾 paragraph 2 件追記 | R-AYA1/2/3 + R-MAT1-4 + R-TERR 行に確定 verdict mark / ST-6 前段 (a) 完了 paragraph (alive 第 3 category 正当化含む) / ST-6 前段 (b) 完了 paragraph (= `normal_matrix` 4 件目確定 + `modelview_projection_inverse` 撤回) |
| chapter 10 §1.0 | 状態 column R-MAT4 row | `modelview_projection_inverse` → `normal_matrix` 訂正 |

---

## §3 次着手 = Stage 3 残 12 項目消化 phase = (Q3)(Q5) AYA 判断 batch 先行

### §3.1 全体像 (= 09 §14.5 残 12 項目)

09 §14.5 Stage 3 self-check 14 項目のうち、本 session で **3-3 + 3-8 = 2 件 ✅ 完了 / 残 12 項目**。

| 項目 | 内容 | 着手 priority | 担当 | 出力 |
|---|---|---|---|---|
| **3-1** | (Q3) AYA 判断 | **最優先 (= 次 session sub-task 1)** | AYA 判断 | chapter 10 §1.3 (Q3) 状態 ✅ + chapter 09 関連 §反映 |
| **3-2** | (Q5) AYA 判断 | **最優先 (= 次 session sub-task 1 同梱)** | AYA 判断 | chapter 10 §1.3 (Q5) 状態 ✅ + chapter 09 関連 §反映 |
| 3-4 | chapter 04 §6.4 / §4.3.1 / §5.6 反映済確認 | sub-task 2 | Claude 自走 verify | 09 §14.5 3-4 行 ✅ mark |
| 3-5 | chapter 08 §5.4.1 / §5.2.1 / §11.5 / §12.5 / §13.5 反映済確認 | sub-task 2 (3-4 と batch 可) | Claude 自走 verify | 09 §14.5 3-5 行 ✅ mark |
| 3-6 | chapter 04 NTTP 判定 | sub-task 3 | Claude 判定 | chapter 04 NTTP section 確認 + 09 §14.5 3-6 行 ✅ mark |
| 3-7 | chapter 06a 起案済確認 | sub-task 3 (3-6 と batch 可) | Claude 自走 verify | 06a structure verify + 09 §14.5 3-7 行 ✅ mark |
| 3-9 | autobuild manifest pin 反映済確認 | sub-task 4 | Claude 自走 verify (= grep `autobuild.xml` 関連) | 09 §14.5 3-9 行 ✅ mark |
| 3-10 | chapter 06c descriptor set bind 起案 (= verify-only) | sub-task 4 (3-9 と batch 可) | Claude 自走 verify | 06c structure verify + 09 §14.5 3-10 行 ✅ mark (= 既起案、本 session で並行 verify 済) |
| 3-11 | Vulkan validation layer 配線確認 | sub-task 5 | Claude 自走 verify | 関連 chapter (= 07 / 08 / 09) verify + 09 §14.5 3-11 行 ✅ mark |
| 3-12 | LL_VULKAN_GLSL 段階解禁 plan 確認 | sub-task 5 (3-11 と batch 可) | Claude 自走 verify | 09 §12 / §13 段階解禁 plan verify + 09 §14.5 3-12 行 ✅ mark |
| 3-13 | autobuild ReleaseFS_Vulkan target 起案確認 | sub-task 6 | Claude 自走 verify | autobuild target spec verify + 09 §14.5 3-13 行 ✅ mark |
| 3-14 | Phase 1.A 第 1 UBO target 確定 | **最後 (= Stage 3 ✅ 直前)** | AYA 判断 (= (Q1)(Q2) で確定済 default 採用前提、最終 target 名指定のみ) | chapter 09 §15 / §16 で第 1 UBO target 名 確定 + 09 §14.5 3-14 行 ✅ mark |

**全 14 項目 ✅ + AYA 承認 = `feedback_design_phase_no_code_write` 解除条件** (= `indra/` 改変解禁 = Phase 1.A 実装 entry へ移行)。

### §3.2 (Q3)(Q5) AYA 判断 batch = 次 sub-task 着手 1

| Q ID | 内容 | chapter 10 §1.3 default | 提案 batch 形式 |
|---|---|---|---|
| (Q3) | (前 handoff §3.3 / chapter 10 §1.3 参照、後ろ倒し可項目) | (= 既登録 default) | AYA 「全 default 採用」or 個別 verdict batch 提示 |
| (Q5) | (同上) | (= 既登録 default) | 同上 |

Claude は次 session 入りで chapter 10 §1.3 を pinpoint Read して (Q3)(Q5) 各 default verdict / 残 sub-option を抽出 + batch 提示 = `feedback_one_step_at_a_time` 準拠 (= 1 メッセージで全件 summary + AYA 判断後にまとめて反映)。

### §3.3 規律 (= 次 session 着手時 self-check)

1. **literal scope = doc 系のみ継続**: 残 12 項目消化 phase 全体で `indra/` 改変なし (= `feedback_design_phase_no_code_write` 厳守)、実装 entry は別 phase / 別 handoff
2. **AYA 判断 batch は 1 メッセージで全件 summary**: (Q3)(Q5) は 1 batch で提示 (= `feedback_one_step_at_a_time` 準拠、ST-3/ST-4/ST-5 の手法を踏襲)
3. **handoff 最小読み厳守**: 本 handoff §1.1 で 3 件、§1.2 pinpoint で必要時、それ以外の chapter は当該 sub-task 着手時のみ Read (= `feedback_handoff_minimal_pre_req_read`)
4. **3-14 第 1 UBO target 確定は最後**: 残 11 項目消化後の最後の AYA 判断、(Q1)(Q2) 確定 default 前提で具体 UBO 名指定のみ
5. **`feedback_no_scope_shrink` 継続**: AYA 「全 default 採用」を verdict full set に literal 適用、scope 縮小しない
6. **commit 戦略 = AYA 判断**: 本 session 4 file 編集 (uncommitted) は次 session で commit、(a) ST-6 反映 commit 先行 + handoff doc commit、(b) handoff doc commit 先行 + ST-6 反映を次 sub-task commit に同梱、のいずれを取るかは AYA 判断
7. **共著行不在厳守**: 全 commit で `Co-Authored-By: Claude` 行禁止 (= `feedback_no_claude_coauthor`)
8. **`feedback_doubt_self_first` 継続**: ST-6 で適用済 (= R-MAT4 候補 `modelview_projection_inverse` 当初前提を grep で検証 → 0 件で撤回 → AYA 確認 → A 案採用)、残 12 項目でも 同 protocol 継続

---

## §4 self-verify (= 本 handoff 起こした時点の整合性)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) 前 handoff (`...phase1-a-entry-st1-st5-complete.md`) からの遷移整合 | 前 §3 「次 = ST-6 = chapter 06b 起案 + 起案前 grep 2 件」 → 本 handoff 「ST-6 完了 = (a) grep + (b) cadence 転記 + 本体 B 案 delta integration」起点と一致 | ✅ |
| (2) commit log `e649d983b2` 確認 | `git log --oneline -1` で HEAD と一致 (= 前 handoff doc commit) | ✅ |
| (3) 本 session 編集 4 file uncommitted 確認 | `git status --short` = `M` 4 file (05 / 06b / 09 / 10) + `git diff --stat` = +48/-16 | ✅ |
| (4) `feedback_design_phase_no_code_write` 準拠 | 本 session = doc edit only、`indra/` 改変ゼロ厳守継続 | ✅ |
| (5) `feedback_handoff_minimal_pre_req_read` 準拠 | 本 §1.1 = 3 件のみ列挙、§1.2 = pinpoint Read reference 分離 | ✅ |
| (6) `feedback_one_step_at_a_time` 準拠 | ST-6 前段 (a)(b) は 1 sub-task ずつ AYA 確認、本体 B 案 delta は 1 編集セッション内で順次 | ✅ |
| (7) `feedback_no_claude_coauthor` 準拠 | 本 handoff 内、次 session commit でも共著行禁止明示 | ✅ |
| (8) `feedback_no_scope_shrink` 準拠 | ST-6 で AYA 「全 default 採用」前 session verdict full set を literal 維持、scope 縮小なし | ✅ |
| (9) `feedback_doubt_self_first` 適用記録 | R-MAT4 当初候補 `modelview_projection_inverse` grep 0 件 → 撤回 → AYA 確認 → A 案 (`normal_matrix` 単独) 採用 + R-AYA3 「dead か hashed か」分類で捕まらず → alive 第 3 category 確認 → AYA「dead/dead/alive」承認 | ✅ |
| (10) Stage 3 14 項目 self-check 進捗整合 | 09 §14.4 + §14.5 で 3-3 + 3-8 = 2 件 ✅ 完了 / 残 12 項目 update 反映済 | ✅ |
| (11) chapter 06b/06c 既起案済 verify 明示 | §2.4 で B 案 (= verify + delta integration) 採用経緯 + 06c 無編集理由 (= verify only) 明示 | ✅ |
| (12) Stage 3 残 12 項目消化 phase 着手 priority 明示 | §3.1 table で 6 sub-task 単位 batch 化 + (Q3)(Q5) 最優先 / 3-14 最後 priority 明示 | ✅ |

---

## §5 引き継ぎ済の memory (= 次 session も活きる)

- `feedback_handoff_minimal_pre_req_read` (= 本 handoff §1 構造の根拠)
- `feedback_design_phase_no_code_write` (= 残 12 項目消化 phase も `indra/` 改変なし継続、実装 entry は別 handoff で解禁)
- `feedback_no_scope_shrink` (= AYA「全 default 採用」を literal 全件に適用、(Q3)(Q5) でも同様)
- `feedback_self_verify_before_handoff` (= 残 12 項目消化も AYA 確認前に Claude 全 cross-ref self-verify)
- `feedback_no_claude_coauthor` (= 全 commit 共著行禁止)
- `feedback_one_step_at_a_time` (= AYA 判断仰ぎ batch は 1 メッセージで全件提示、(Q3)(Q5) も同様)
- `feedback_doubt_self_first` (= ST-6 R-MAT4 撤回で実証、残 12 項目でも grep / verify 先行で前提自己検証)
- `feedback_proactive_handoff` (= 本 handoff 作成自体の根拠、context 圧迫前に能動引き継ぎ)
- `feedback_no_auto_commit` (= 本 session 4 file 編集を commit せず handoff で次 session に明示引き継ぎ、commit 戦略は AYA 判断)
- `project_ayastorm_r41_vulkan_migration` (= r41 章 active pointer)
- `project_ayastorm_r41_design_principles` (= upstream 取込容易性 + core 並列化容易性の 2 大設計原則、残 12 項目消化の前提)
- `feedback_ubo_migration_one_at_a_time` (= 3-14 第 1 UBO target 確定の規律 default、Phase 1.A 実装 entry 直前で再確認)

---

## §6 次 session 着手 1 line

「前 session で η-29 Phase 1.A entry doc 系 6 sub-task 全完走 = ST-6 完了 (= 4 file uncommitted: chapter 05 §7.3.4 訂正 + chapter 06b §2.2/§2.3/§3.3.4/§8.1 delta integration 4 件 + chapter 09 §14.4/§14.5 3-8 ✅ mark + chapter 10 §2.7 状態 column 確定 + §1.0 R-MAT4 訂正、ST-6 前段 (a) R-AYA1/2/3 verdict = dead/dead/alive 確定 / 前段 (b) R-MAT1-4 cadence per-draw 4 件 literal 転記 = R-MAT4 `normal_matrix` 確定 + `modelview_projection_inverse` 撤回 / 本体 B 案 = chapter 06b/06c 既起案済 verify + delta integration)。本 session = Stage 3 14 項目 self-check 残 12 項目消化 phase の最初の sub-task = **(Q3)(Q5) AYA 判断 batch** (= chapter 10 §1.3 後ろ倒し可項目、1 メッセージで default + 残 sub-option summary 提示 → AYA 判断後にまとめて反映)。`indra/` 改変なし継続 (= `feedback_design_phase_no_code_write` 厳守)、commit 戦略 (= 4 file 反映 commit 先行 vs (Q3)(Q5) commit 同梱) は AYA 判断。残 sub-task = 3-1/3-2 (Q3)(Q5) batch / 3-4+3-5 chapter 04+08 反映済確認 batch / 3-6+3-7 NTTP 判定 + 06a verify batch / 3-9+3-10 autobuild manifest pin + 06c verify batch / 3-11+3-12 validation layer + LL_VULKAN_GLSL 解禁 plan batch / 3-13 autobuild target / 3-14 第 1 UBO target 確定 (= 最後 / AYA 判断)。全 14 項目 ✅ + AYA 承認で indra/ 改変解禁 = Phase 1.A 実装 entry 別 handoff へ移行」
