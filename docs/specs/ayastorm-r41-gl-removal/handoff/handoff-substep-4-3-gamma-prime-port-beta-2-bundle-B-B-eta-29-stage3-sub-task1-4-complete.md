# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-29 Stage 3 sub-task 1+2+3+4 complete (= 09 §14.5 14 項目 self-check 8/14 ✅ / 残 6 項目、(Q-NTTP) gap remediation 実施済)

**作成**: 2026-06-03
**前 handoff**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-29-phase1-a-entry-st6-complete.md` (= Phase 1.A entry doc 系 6 sub-task 全完走 = ST-6 完了 = 09 §14.5 self-check 3-3 + 3-8 = 2 件 ✅ / 残 12 項目、次 = Stage 3 残 12 項目消化 phase + (Q3)(Q5) AYA 判断 batch 最優先)
**branch**: `feature/ayastorm-r41-gl-removal`
**最新 commit (本 handoff 時点)**: `698fb9cbe7` (= 本 session 最終 commit = sub-task 3+4 完了 + (Q-NTTP) gap remediation)
**本 session 未 commit**: なし (= 本 handoff doc 自身を別 commit で出す予定、AYA 指示後)

---

## §0 state 一行 summary

η-29 **Phase 1.A entry Stage 3 残 12 項目消化 phase = sub-task 1+2+3+4 完走** (= 本 session 2 commit = `e41389fad1` sub-task 1+2 / `698fb9cbe7` sub-task 3+4 + (Q-NTTP) gap remediation)。**09 §14.5 Stage 3 self-check 14 項目のうち 8 件 ✅ 完了 / 残 6 項目** (= 3-3 + 3-4 + 3-5 + 3-6 + 3-7 + 3-8 + 3-9 + 3-10 ✅)。本 session 主要成果 = (1) ST-7 batch (Q3)(Q5) = A 確定 = Stage 2 5/5 ✅ 完全達成 / (2) sub-task 2 = chapter 04 §6.4/§4.3.1/§5.6 + chapter 08 §5.4.1/§5.2.1/§11.5/§12.5/§13.5 反映済 verify (= Deliverable A-1/A-2/A-3 + B-1/B-2/B-3/B-4/B-5) / (3) sub-task 3 = chapter 09 §14 全 8 subsection + chapter 04 §6.4.7 NTTP 判定材料 + chapter 06a §3 cache + §5 16 method setter 起案済 verify / (4) sub-task 4 = chapter 02 §2.4 naming + chapter 07 §3-§6/§11 set 帯 + 256B 出力契約 + chapter 10 持越項目 14 件中 13 件 既登録 verify + (NTTP) gap 検出 → 即時 remediation (= chapter 09 §11.6 (Q-NTTP) 新設 + chapter 10 §1.0 row 29 + §1.3 表 (Q-NTTP) 行)。**indra/ 改変なし継続** (= `feedback_design_phase_no_code_write` 厳守、本 session も doc edit only)。Claude 即着手対象 = **sub-task 5 候補 = §14.5 row 3-1 (Stage 0 全 12 項目確認) + row 3-2 (Stage 1 全 9 項目確認) batch** (= Claude 自走 verify、AYA 判断不要、各 item grep / read 必要で scope 中程度)。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。最初の session 入りでは **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read (offset/limit) する。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-29-stage3-sub-task1-4-complete.md` | 全文 | 本 handoff (= Stage 3 sub-task 1-4 完了 state + 残 6 項目消化 phase 着手地点 + (Q-NTTP) gap remediation 経緯) |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §14.4 (= ST-7 batch + sub-task 2/3/4 paragraph 全件、8/14 ✅ / 残 6 項目 progress) + §14.5 (= 14 項目 self-check、3-3/3-4/3-5/3-6/3-7/3-8/3-9/3-10 inline ✅ mark 確認 + 残 6 項目 = 3-1/3-2/3-11/3-12/3-13/3-14 詳細) | 残 6 項目消化 phase 全体把握 + 次 sub-task 5 着手 priority 判断 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/10-open-questions.md` | §0 + §1.0 (= 29 件 index、11 件判断済 / 18 件未判断 = (Q-NTTP) row 29 + §1.3 5→6 件 count update) + §1.3 ((Q-NTTP) 行 + ST-7 sub-task 4 batch verdict paragraph) | 次 sub-task 5 (= Stage 0/1 verify) + Phase 1.A 入口 (Q-NTTP) AYA 判断連動の前提 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §11.6 (= (Q-NTTP) 新設 section = A/B/C 3 案 + default A R1 不採用 + 確定タイミング Phase 1.A 入口判定可) + §14.2 (= Stage 0 全 12 項目、row 0-9 遡及 retroactive 充足 mark 含む) + §14.3 (= Stage 1 全 9 項目、次 sub-task 5 row 3-2 verify 対象) |
| `docs/specs/ayastorm-r41-gl-removal/design/10-open-questions.md` | §2 + §5 + §6 (= sub-task 5 row 3-1 Stage 0 + row 3-2 Stage 1 verify 時に持越項目 cross-ref 確認用) |
| `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` | §6.4.7 (= NTTP 判定材料 7 line = R1 採用必要性 vs C++20 切替 cost trade-off + R1 不採用 default、(Q-NTTP) AYA 判断本体 = §14.5 row 3-14 で必要) |
| `docs/specs/ayastorm-r41-gl-removal/design/02-naming-convention.md` | §2.4 (= Codegen-UBO 生成識別子 4 種 = `<Block>_<Member>_OFFSET` / `<Block>Layout` / `<Block>_SIZE` / `ubo_layout_<blockname>.inl`、sub-task 4 verify 済) |
| `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md` | §3 (V1) device limit query / §4 (V3) set=1 layout / §5 (S3) sampler 49 / §6 (W) pool 容量 / §11 「padding alignment 256 B 出力契約」chapter 08 提供 / §3.1 + §7.3 = 256 alignment、sub-task 4 verify 済 |
| `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` | §3 mUniformUBOLoc cache 4 subsection + §5 16 method setter 6 subsection、sub-task 3 verify 済 |
| `docs/specs/ayastorm-r41-gl-removal/design/06a-prep-phase0-measurement.md` | §2-§6 (= Stage 1 (= η-29 Phase 0 計測) 反映 flow、sub-task 5 row 3-2 verify 対象) |
| `autobuild.xml` | Python / glslang / spirv-cross version pin 行 (= sub-task 6 候補 = row 3-11 autobuild manifest pin 確認対象) |

---

## §2 本 session 成果 (= Stage 3 sub-task 1+2+3+4 完了)

### §2.1 commit 状態

```
e41389fad1 docs(r41): ... Stage 3 sub-task 1+2 完了 = ST-7 batch (Q3)(Q5) 確定 + chapter 04/08 反映済 verify
                                                                                            +24/-6 line
698fb9cbe7 docs(r41): ... Stage 3 sub-task 3+4 完了 + (Q-NTTP) gap remediation
                                                                                            +33/-9 line
                                                                                       計 +57/-15 line
```

**working tree clean** (本 handoff doc 自身は別 commit 予定)。

### §2.2 sub-task 1 = ST-7 batch (Q3)(Q5) = A 確定 = Stage 2 完全達成

| Q ID | verdict | 根拠 |
|---|---|---|
| (Q3) | **A 確定** = OpenGL path 並走期間 = 全 UBO 移行完了まで並走 (= Phase K+4 で初撤廃) | chapter 10 §1.3 「推奨で」AYA 応答 = ST-5 batch 「全 default 採用」継承、REJECT 時 baseline 確保最大、memory `feedback_build_only_verified` 整合、B/C 案 Phase K+3 進行中再評価可 = 後ろ倒し option 保持 |
| (Q5) | **A 確定** = Phase 0 計測 phase 番号化 = 独立 Phase η-29 として明示 | sub-step 命名 `4.3-γ'-port-β-2-bundle-B-B?-η-29` で物理現実が既 active = 既物理確定の形式 ✅ 化、09 §14.4 で既「default 確定 = 既反映済」と記載、B/C は sub-step rename cost / 時系列矛盾で技術的不成立 |

反映先 = chapter 09 §11.3 / §11.5 default → 確定形書換 + §14.4 ST-7 verdict マーク (= **Stage 2 5/5 ✅ 完全達成**) + chapter 10 §1.0 状態 column (Q3) ✅ + (Q5) ✅ + count 内訳 9 → 11 件判断済 / 19 → 17 件未判断 + §1.3 verdict マーク。

### §2.3 sub-task 2 = chapter 04 + 08 反映済 verify (= Claude 自走、AYA 判断不要)

| chapter | section | Deliverable | sub-subsection count |
|---|---|---|---|
| 04 §4.3.1 | std140 calculator algorithm 詳細化 | A-2 | 8 sub-subsection (`§4.3.1.1`-`.8`) |
| 04 §5.6 | perfect hash CHD algorithm 詳細化 | A-3 | 7 sub-subsection (`§5.6.1`-`.7`) |
| 04 §6.4 | name-based dispatch algorithm 詳細化 | A-1 | 7 sub-subsection (`§6.4.1`-`.7`) |
| 08 §5.4.1 | SPIR-V reflection 二重保証 mechanism | B-1 | 8 sub-subsection (`§5.4.1.1`-`.8`) |
| 08 §5.2.1 | mini-parser 詳細化 | B-2 | 8 sub-subsection (`§5.2.1.1`-`.8`) |
| 08 §11.5 | 増分 build cache 詳細化 | B-3 | 1 section |
| 08 §12.5 | CMake DEPENDS + 手動 target 詳細化 | B-4 | 1 section |
| 08 §13.5 | 3 OS binary identical 保証 mechanism 詳細化 | B-5 | 1 section |

反映先 = 09 §14.5 row 3-4 + row 3-5 inline `✅ 2026-06-03 ST-7 sub-task 2` mark + §14.4 sub-task 2 完了 paragraph (= count 4 → 6 件 ✅ 完了 / 残 10 → 8 項目)。

### §2.4 sub-task 3 = chapter 09 §14 + 04 §6.4.7 + chapter 06a verify (= Claude 自走)

| chapter | section | 内容 |
|---|---|---|
| 09 §14 全 8 subsection | §14.1 readiness 4 階層 + §14.2 Stage 0 + §14.3 Stage 1 + §14.4 Stage 2 + §14.5 Stage 3 + §14.6 達成順序図 + §14.7 chapter 整合 pointer + §14.8 self-evaluation | row 3-6 対象、本 ST-7 batch で §14.4 Stage 2 完全達成 paragraph + sub-task 2/3/4 完了 paragraph 追記済 = live update 継続 |
| 04 §6.4.7 | C++20 NTTP 採否判定材料 7 line | R1 採用必要性 vs C++20 切替 cost trade-off 明示 + R1 不採用 default + (Q-NTTP) chapter 09 §11 AYA 判断仰ぎ候補登録 intent (= sub-task 4 で実 entry 追加) |
| 06a §3 | mUniformUBOLoc cache 構造 4 subsection (`§3.1` struct UniformLocation / `§3.2` LLGLSLShader 配置 / `§3.3` cadence_tag enum / `§3.4` 解放規律) | row 3-7 対象 |
| 06a §5 | 16 method setter Vulkan path 分岐 6 subsection (`§5.1` 17 method 一覧 / `§5.2` path 分岐 code shape / `§5.3` 共通 pattern / `§5.4` mUseUBO flag / `§5.5` integer index vs LLStaticHashedString / `§5.6` sampler 49 個 OpenGL path 強制) | 06a doc 495 line で要件物理充足 |

反映先 = 09 §14.5 row 3-6 + row 3-7 inline `✅ 2026-06-03 ST-7 sub-task 3` mark + §14.4 sub-task 3 完了 paragraph (= count 6 → 8 件 ✅ 完了 不変 / 残 8 項目 = sub-task 4 で再 update) + 「handoff §3.1 sub-task 番号と §14.5 row 番号は別体系で対応が一意ではない」明示 paragraph。

### §2.5 sub-task 4 = chapter 02/07/10 verify + (Q-NTTP) gap remediation (= `feedback_doubt_self_first` 適用)

**row 3-9 verify** (= chapter 02 §2.4 + chapter 07 set 帯 5 化 / 256B padding):

| chapter | section | verify 結果 |
|---|---|---|
| 02 §2.4 | Codegen-UBO 生成識別子 (= 4 種 `<Block>_<Member>_OFFSET` / `<Block>Layout` / `<Block>_SIZE` / `ubo_layout_<blockname>.inl` + machine-derivable 規律) | 起案済 ✅ |
| 07 §3 | (V1) device limit query | 起案済 ✅ |
| 07 §4 | (V3) set=1 layout 方式確定 | 起案済 ✅ |
| 07 §5 | (S3) sampler 49 個 配置確定 | 起案済 ✅ |
| 07 §6 | (W) pool 容量算定 | 起案済 ✅ |
| 07 §11 line 547 | 「padding alignment 256 B 出力契約」chapter 08 提供 | 起案済 ✅ |
| 07 §3.1 | maxUniformBufferOffsetAlignment 256 limit | 起案済 ✅ |
| 07 §7.3 | ring buffer offset alignment 256 | 起案済 ✅ |
| 07 §3.2 line 385 | 「Codegen 側で UBO struct size を alignment 倍数 (256 B safe) で padding 出力」 | 起案済 ✅ |

**row 3-10 verify** (= inventory + chapter 10 持越項目):

| 期待登録 ID | 場所 | verify 結果 |
|---|---|---|
| (V1')(V3')(S3') | chapter 10 §1.0 row 1-3 + §1.1 詳細 row 70-72 | 既登録 ✅ |
| (W) | chapter 10 §1.0 row 4 | 既登録 ✅ |
| (A1)(P)(G/B3)(B1)(B2)(B4)(B5) | chapter 10 §1.0 row 5-11 + §1.2 詳細 row 83-89 | 既登録 ✅ |
| (P-future)(cache-grow) | chapter 10 §2.3 line 183-184 | 既登録 ✅ |
| inventory §7 残課題接続 | chapter 10 §5 + §6.1 + §6.2 | 既登録 ✅ |
| **(NTTP)** | chapter 04 §6.4.7 で「09 §11 (Q-NTTP) として登録」と intent 記述あるが、09 §11 / 10 §1.0 / §1.3 への実 entry 未追加 | **GAP 検出 → 本 sub-task 4 batch で remediation 実施** |

**(Q-NTTP) gap remediation 実施** (= `feedback_doubt_self_first` 適用、verify 中の gap 検出 → 即時解消):
- (a) chapter 09 §11.6 (Q-NTTP) 新設 (= R1 compile-time literal path 採否 = C++20 NTTP 採用 vs C++17 維持、A/B/C 3 案 + default A R1 不採用 = R3 name-based dispatch + perfect hash CHD で十分高速 + C++17 維持 cost 回避、確定タイミング Phase 1.A 入口判定可 = default A 採用継続で着手可)
- (b) chapter 10 §1.0 row 29 (Q-NTTP) 追加 = 28 → 29 件 / count 内訳 §1.3 5 → 6 件 / 判断済 11 件不変 / 未判断 17 → 18 件 = §1.1 4 + §1.2 7 + §1.3 (Q-NTTP) 1 + §1.4 4 + §1.5 (F)(Q28-FFDUP) 2
- (c) chapter 10 §1.3 表に (Q-NTTP) 行追加 = default A R1 不採用 / C++17 維持、判断ポイント A/B/C trade-off 明示、出典 09 §11.6 + 04 §6.4.7
- (d) chapter 10 §1.0 「未判断 1 件新規登録 cross-ref」paragraph 追加
- (e) chapter 10 §1.3 末尾「ST-7 sub-task 4 batch (Q-NTTP) 新規登録」paragraph 追加
- (f) chapter 09 §14.2 row 0-9 遡及 retroactive 充足 mark 追加 (= 起案時点 (NTTP) 漏れ retro 解消、grep verify 25 → 29 件 update、Stage 0 verdict 12 項目 ✅ は不変 = 実体化)

**反映先** = 09 §14.5 row 3-9 + row 3-10 inline `✅ 2026-06-03 ST-7 sub-task 4` mark + §14.4 sub-task 4 完了 paragraph (= count 6 → 8 件 ✅ 完了 / 残 8 → 6 項目)。

### §2.6 sub-task 3 paragraph 訂正 (= sub-task 4 で実施)

sub-task 3 paragraph 当初記述「(Q-NTTP) chapter 09 §11 AYA 判断仰ぎ候補登録」は **chapter 04 §6.4.7 文末の登録 intent 記述** であって、**chapter 09 §11 / chapter 10 §1.0 / §1.3 への実 entry 追加は未実施**だった。sub-task 4 paragraph で「§6.4.7 文末『chapter 09 §11 (Q-NTTP) として AYA 判断仰ぎ候補に登録』は **登録 intent の記述**、実 entry 追加は sub-task 4 batch で実施 (= 09 §11.6 新設 + 10 §1.0 row 29 + §1.3 表)」と訂正明示。

---

## §3 次着手 = Stage 3 残 6 項目消化 phase = sub-task 5 候補

### §3.1 全体像 (= 09 §14.5 残 6 項目)

09 §14.5 Stage 3 self-check 14 項目のうち、本 session で **3-3 + 3-4 + 3-5 + 3-6 + 3-7 + 3-8 + 3-9 + 3-10 = 8 件 ✅ 完了 / 残 6 項目**。

| 項目 | 内容 | 着手 priority | 担当 | 出力 |
|---|---|---|---|---|
| **3-1** | Stage 0 全 12 項目 ✅ 確認 | **次 session sub-task 5 候補** | Claude 自走 verify | 09 §14.5 3-1 行 ✅ mark (= §14.2 全 12 項目 cross-ref 確認、row 0-9 遡及充足は本 session 反映済) |
| **3-2** | Stage 1 全 9 項目 ✅ 確認 (= Phase 0 計測完了 + chapter 05/06a/06b/06c 反映済) | **次 session sub-task 5 候補 (= 3-1 と batch 可)** | Claude 自走 verify | 09 §14.5 3-2 行 ✅ mark (= §14.3 全 9 項目 cross-ref 確認、06a-prep §6 反映 flow 全行「反映済」確認) |
| 3-11 | autobuild manifest pin 状態確認 (= Python / glslang / spirv-cross version pin) | sub-task 6 候補 | Claude 自走 verify (= `autobuild.xml` grep) | 09 §14.5 3-11 行 ✅ mark or 「(Phase 1.A 内 task)」維持判断 |
| 3-12 | `indra/` 改変解禁 = `feedback_design_phase_no_code_write` 解除点 | Phase 1.A 入口で達成 | (常時継続) | 09 §14.5 3-12 行 ✅ mark (= Phase 1.A handoff doc 起案後) |
| 3-13 | Phase 1.A handoff doc 起案 (= 本 chapter §4 Phase 1.A scope + Exit Criteria 反映) | sub-task 7 候補 | Claude 起案 | `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B?-eta-30-phase1-a-entry.md` 起案 |
| **3-14** | C++ standard NTTP 採否 AYA 判断仰ぎ (= (Q-NTTP) AYA 判断本体) | **最後 (= Stage 3 ✅ 直前)** | AYA 判断 (= chapter 10 §1.3 (Q-NTTP) default A 採用継続 or B/C 採用判定) | chapter 10 §1.3 (Q-NTTP) 状態 ✅ + chapter 09 §11.6 default → 確定形書換 + 09 §14.5 3-14 行 ✅ mark |

**全 14 項目 ✅ + AYA 承認 = `feedback_design_phase_no_code_write` 解除条件** (= `indra/` 改変解禁 = Phase 1.A 実装 entry へ移行)。

### §3.2 sub-task 5 候補 = 3-1 + 3-2 batch (= 次 session 着手 1)

| row | verify 対象 | 方法 |
|---|---|---|
| 3-1 | §14.2 Stage 0 全 12 項目 (= 0-1 chapter 04 prototype + 0-2 chapter 08 + 0-3 chapter 09 + 0-4 chapter 06a-prep + 0-5 chapter 02 §2.4 + 0-6 chapter 05 + 0-7 chapter 06b/06c + 0-8 chapter 07 + 0-9 chapter 10 (= (NTTP) 遡及充足済) + 0-10 chapter 01 + 0-11 inventory §3.3.1 + 0-12 `feedback_design_phase_no_code_write` 継続) | 各 row の確認方法欄に従い grep / read で全件 ✅ 確認、Stage 0 verdict 「全 12 項目 ✅」が本 session sub-task 4 (NTTP) 遡及充足で実体化済 |
| 3-2 | §14.3 Stage 1 全 9 項目 (= 1-1 AYAstorm build 動作 + 1-2 LL_INFOS hook 配線 + 1-3 3 scenario cold launch + 1-4 log 解析 + 1-5 (E') 5 件 grep + 1-6 (F) MaterialUBO diff + 1-7 (H1b) 16 件 cadence 確定 + 1-8 検証 hook 除去 + 1-9 (RF) fence throttle 頻度 log) | 06a-prep §6 反映 flow 全行「反映済」確認、Phase 0 step 5 完了 (= `4e40fd2ab0` revert + flag OFF build PASS) で全件 ✅ 達成済 |

Claude は次 session 入りで本 handoff §1.1 3 件読み → §3.2 から sub-task 5 着手、§14.2 / §14.3 各 row を pinpoint Read で確認 → 09 §14.5 3-1 + 3-2 inline ✅ mark + §14.4 sub-task 5 完了 paragraph (= count 8 → 10 件 ✅ 完了 / 残 6 → 4 項目)。

### §3.3 規律 (= 次 session 着手時 self-check)

1. **literal scope = doc 系のみ継続**: 残 6 項目消化 phase 全体で `indra/` 改変なし (= `feedback_design_phase_no_code_write` 厳守)、3-12 解除点到達は Phase 1.A handoff doc (= 3-13) 起案後
2. **AYA 判断 batch は 1 メッセージで全件 summary**: (Q-NTTP) AYA 判断本体 (= 3-14) は 1 batch で提示 (= `feedback_one_step_at_a_time` 準拠、(Q1)-(Q5) batch と同手法)
3. **handoff 最小読み厳守**: 本 handoff §1.1 で 3 件、§1.2 pinpoint で必要時、それ以外の chapter は当該 sub-task 着手時のみ Read (= `feedback_handoff_minimal_pre_req_read`)
4. **3-14 (Q-NTTP) AYA 判断本体は最後**: 残 5 項目 (= 3-1 + 3-2 + 3-11 + 3-12 + 3-13) 消化後の最後の AYA 判断、default A 採用継続で Phase 1.A 着手可
5. **`feedback_no_scope_shrink` 継続**: AYA verdict full set に literal 適用、scope 縮小しない
6. **commit 戦略 = AYA 判断**: 本 handoff doc は本 session 末尾で別 commit (= AYA 指示後)、sub-task 5 以降は batch commit 単位を AYA 判断で
7. **共著行不在厳守**: 全 commit で `Co-Authored-By: Claude` 行禁止 (= `feedback_no_claude_coauthor`)
8. **`feedback_doubt_self_first` 継続**: 本 session sub-task 4 で適用済 (= (NTTP) gap 検出 → 即時 remediation)、残 6 項目でも 同 protocol 継続
9. **`feedback_proactive_handoff` 継続**: 周回境界で context 残量監視、handoff 作成判断は能動的に
10. **sub-task 5 = Claude 自走 verify、AYA 判断不要**: 3-1 + 3-2 batch は §14.2 / §14.3 全件 ✅ cross-ref 確認、AYA 中断不要

---

## §4 self-verify (= 本 handoff 起こした時点の整合性)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) 前 handoff (`...phase1-a-entry-st6-complete.md`) からの遷移整合 | 前 §3.1 「残 12 項目消化 phase + (Q3)(Q5) 最優先」 → 本 handoff 「sub-task 1-4 完了 = 8/14 ✅ / 残 6 項目」起点と一致 | ✅ |
| (2) commit log `698fb9cbe7` 確認 | `git log --oneline -1` で HEAD と一致 (= 本 session 最終 commit) | ✅ |
| (3) 本 session 編集 working tree clean 確認 | `git status --short` = clean、本 handoff doc 自身は本 §作成中で未 commit | ✅ |
| (4) `feedback_design_phase_no_code_write` 準拠 | 本 session = doc edit only、`indra/` 改変ゼロ厳守継続 | ✅ |
| (5) `feedback_handoff_minimal_pre_req_read` 準拠 | 本 §1.1 = 3 件のみ列挙、§1.2 = pinpoint Read reference 分離 | ✅ |
| (6) `feedback_one_step_at_a_time` 準拠 | sub-task 1+2 / 3+4 各 batch で 1 メッセージ 1 action、AYA 確認 (= 「推奨で」「B で進めて」「ok」「A で」「推奨で」「ok」) を毎 batch | ✅ |
| (7) `feedback_no_claude_coauthor` 準拠 | 本 session 2 commit (`e41389fad1` + `698fb9cbe7`) で共著行不在確認、本 handoff 内・次 session commit でも共著行禁止明示 | ✅ |
| (8) `feedback_no_scope_shrink` 準拠 | AYA「推奨で」literal 受領 + sub-task 4 で「13/14 部分 ✅」降格案 B 不採用 = gap 解消 A 案 (= scope 拡大方向) を AYA 「推奨で」承認、scope 縮小なし | ✅ |
| (9) `feedback_doubt_self_first` 適用記録 | sub-task 4 で (NTTP) gap 検出 (= chapter 04 §6.4.7 「09 §11 (Q-NTTP) として登録」は intent 記述、実 entry 未追加) → 即時 remediation (= 09 §11.6 + 10 §1.0 + §1.3 + 遡及 §14.2 row 0-9)、sub-task 3 paragraph 記述不正確を訂正 | ✅ |
| (10) Stage 3 14 項目 self-check 進捗整合 | 09 §14.4 + §14.5 で 3-3 + 3-4 + 3-5 + 3-6 + 3-7 + 3-8 + 3-9 + 3-10 = 8 件 ✅ 完了 / 残 6 項目 update 反映済 | ✅ |
| (11) Stage 2 完全達成 確認 | 09 §14.4 5/5 ✅ = (Q1) A / (Q2) A / (Q3) A / (Q4) C / (Q5) A 全件確定 = K 値計算可能 + Phase 順序確定 + OpenGL path 維持期間確定 + Phase 0 番号化形式 ✅ | ✅ |
| (12) (Q-NTTP) gap remediation 完了確認 | chapter 09 §11.6 新設 + chapter 10 §1.0 row 29 + §1.3 表 (Q-NTTP) 行 + 遡及 §14.2 row 0-9 充足 = 6 箇所 反映、grep verify pass | ✅ |
| (13) Stage 3 残 6 項目消化 phase 着手 priority 明示 | §3.1 table で 5 sub-task 単位 batch 化 + 3-1+3-2 最優先 / 3-14 最後 priority 明示 | ✅ |

---

## §5 引き継ぎ済の memory (= 次 session も活きる)

- `feedback_handoff_minimal_pre_req_read` (= 本 handoff §1 構造の根拠)
- `feedback_design_phase_no_code_write` (= 残 6 項目消化 phase も `indra/` 改変なし継続、解除は 3-13 Phase 1.A handoff doc 起案後)
- `feedback_no_scope_shrink` (= AYA「推奨で」を literal 全件に適用、(Q-NTTP) gap remediation でも scope 拡大方向の A 案採用)
- `feedback_self_verify_before_handoff` (= 残 6 項目消化も AYA 確認前に Claude 全 cross-ref self-verify、本 handoff 自体も 13 観点 self-verify PASS)
- `feedback_no_claude_coauthor` (= 全 commit 共著行禁止、本 session 2 commit 共著行不在確認済)
- `feedback_one_step_at_a_time` (= AYA 判断仰ぎ batch は 1 メッセージで全件提示、(Q-NTTP) AYA 判断本体 (3-14) も同様)
- `feedback_doubt_self_first` (= 本 session sub-task 4 で適用 = (NTTP) gap 検出 + 即時 remediation、残 6 項目でも grep / verify 先行で前提自己検証)
- `feedback_proactive_handoff` (= 本 handoff 作成自体の根拠、周回境界 = sub-task 1-4 完了 + 次 sub-task 5 scope 中程度で context 残量懸念)
- `feedback_no_auto_commit` (= 本 session 2 commit は AYA 「ok」確認後実施、本 handoff doc 自身も AYA 指示後 commit)
- `project_ayastorm_r41_vulkan_migration` (= r41 章 active pointer)
- `project_ayastorm_r41_design_principles` (= upstream 取込容易性 + core 並列化容易性の 2 大設計原則、残 6 項目消化の前提)
- `feedback_ubo_migration_one_at_a_time` (= 3-14 (Q-NTTP) AYA 判断本体時の規律 default、Phase 1.A 実装 entry 直前で再確認)

---

## §6 次 session 着手 1 line

「前 session で η-29 Phase 1.A entry Stage 3 sub-task 1+2+3+4 完走 = 2 commit (`e41389fad1` + `698fb9cbe7`) = 09 §14.5 14 項目 self-check **8/14 ✅ / 残 6 項目** + (Q-NTTP) gap remediation 実施 (= chapter 04 §6.4.7 NTTP 判定材料の 09 §11 (Q-NTTP) 登録 intent 記述に対し実 entry 未追加 gap を `feedback_doubt_self_first` 適用で sub-task 4 verify 中に検出 → 即時解消 = chapter 09 §11.6 (Q-NTTP) 新設 + chapter 10 §1.0 row 29 + §1.3 表 (Q-NTTP) 行 + 遡及 §14.2 row 0-9 retroactive 充足)。本 session = Stage 3 残 6 項目消化 phase の **sub-task 5 候補 = §14.5 row 3-1 (Stage 0 全 12 項目確認) + row 3-2 (Stage 1 全 9 項目確認) batch** (= Claude 自走 verify、AYA 判断不要、各 row §14.2 / §14.3 cross-ref で全件 ✅ 確認、出力 = 3-1 + 3-2 inline ✅ mark + §14.4 sub-task 5 完了 paragraph = count 8 → 10 件 ✅ / 残 6 → 4 項目)。`indra/` 改変なし継続 (= `feedback_design_phase_no_code_write` 厳守、解除は 3-13 Phase 1.A handoff doc 起案後)、commit 戦略 (= sub-task 5 単独 vs sub-task 6 候補 = 3-11 autobuild manifest pin と batch) は AYA 判断。残 sub-task 候補 = 3-1+3-2 (sub-task 5) / 3-11 autobuild manifest pin (sub-task 6) / 3-12 `indra/` 改変解禁 = Phase 1.A 入口で達成 / 3-13 Phase 1.A handoff doc 起案 (sub-task 7) / 3-14 (Q-NTTP) AYA 判断本体 = 最後。全 14 項目 ✅ + AYA 承認で indra/ 改変解禁 = Phase 1.A 実装 entry 別 handoff へ移行」
