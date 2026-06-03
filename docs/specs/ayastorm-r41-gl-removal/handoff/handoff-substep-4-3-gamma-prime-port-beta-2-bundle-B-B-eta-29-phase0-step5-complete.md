# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-29 Phase 0 Step 5 complete (= Phase 0 全完走)

**作成**: 2026-06-03
**前 handoff**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-29-phase0-step4-complete.md` (= η-29 Phase 0 Step 4 = AYA 実機計測 log 解析 + 06a-prep §5.5 反映 + 副産物 2 件 spec 訂正 完了)
**branch**: `feature/ayastorm-r41-gl-removal`
**最新 commit (本 handoff 時点)**: `4e40fd2ab0` (= Phase 0 Step 5 = 計測 hook 除去 = `c27733ae79` 機械的 revert、`indra/` 2 file -80 line、flag OFF build PASS)

---

## §0 state 一行 summary

η-29 **Phase 0 Step 5 = 計測 hook 除去 phase** 完了 (= `4e40fd2ab0` = `c27733ae79` 機械的 revert = `indra/llrender/llglslshader.cpp` -71 + `indra/cmake/00-Common.cmake` -9 = 計 -80 line、grep `AYASTORM_UBO_CADENCE_HOOK|AYA_UBO_HOOK|UBO_CADENCE|g_aya_ubo_hook|ayaUboHookOn` の `indra/` 配下残存 0 件、flag OFF build PASS、共著行不在) → **Phase 0 計測 phase 5 step 全完走** (= Step 1 pre-hook static / Step 2 hook 配線 / Step 3 AYA 計測 / Step 4 解析+§5+副産物 / Step 5 hook 除去) → 次は **Phase 1.A entry** = (E') grep 実施 + (F) MaterialUBO 比較 + (Q1)(Q2)(Q4)(Q26-MUL)(Q27-CONFL) AYA 判断 + §5.5.5/§5.5.7 chapter 10 残課題追記候補 AYA 判断 + chapter 06b 起案。Claude 即着手対象 = Phase 1.A entry の **doc 系 sub-task 群** (= `indra/` 改変なし、chapter 06b 起案完了後の実装 entry は別 handoff)。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。最初の session 入りでは **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read (offset/limit) する。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-29-phase0-step5-complete.md` | 全文 | 本 handoff (= Phase 0 全完走 state + Phase 1.A entry 着手地点 + 次 session sub-task 分解) |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §14.3 (Stage 1 完了確認) + §14.4 (Stage 2 (Q1)-(Q5)) + §14.5 (Stage 3 着手 ready 14 項目) | Phase 0 → Phase 1.A 境界 + 残 AYA 判断 + Phase 1.A 着手 readiness |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/10-open-questions.md` | §0 + §1.0 (= 27 件 index) + §1.3 (Q1)-(Q5) + §1.5 (Q26-MUL / Q27-CONFL) | Phase 1.A entry で消化すべき open question 全件 + 新規追記候補 2 件 (§5.5.5 aya_* 3 件 / §5.5.7 matrix 系 4-5 件) の位置付け |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/design/06a-prep-phase0-measurement.md` | §3 (E') 同一 binding 複数 UBO 名 grep spec / §4 (F) MaterialUBO vs MaterialUBO_Legacy 比較 spec / §5.5.5 dead 121 件 (= `aya_*` 3 件 chapter 10 残課題追記候補) / §5.5.6 hashed-path 40 件 / §5.5.7 matrix 系 4-5 件 per-program → per-draw 補正 (= chapter 05 §7.3 補正、chapter 06b 起案直前で実施) / §6 chapter 05 / 06a / 06b への反映 flow |
| `docs/specs/ayastorm-r41-gl-removal/design/05-cadence-classification.md` | §7.3 (matrix 系 per-program → per-draw 補正、chapter 06b 起案直前で反映) |
| `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` | §3 (= mUniformUBOLoc cache 構造) + §5 (= 16 method setter 分岐) = Stage 3 self-check 3-7 で「起案済」確認対象 |
| `docs/specs/ayastorm-r41-gl-removal/design/06b-*.md` (未起案) | (= 起案 task 本体、Phase 1.A entry 後半の sub-task) |
| `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` | §6.4 / §4.3.1 / §5.6 = Stage 3 self-check 3-4 で「反映済」確認対象 |
| `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` | §5.4.1 / §5.2.1 / §11.5 / §12.5 / §13.5 = Stage 3 self-check 3-5 で「反映済」確認対象 |

---

## §2 Phase 0 Step 5 = 完了成果 (本 session で済んだこと)

### §2.1 commit log

```
4e40fd2ab0 revert(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-29 Phase 0 Step 5 計測 hook 除去 = c27733ae79 (Step 2 LL_INFOS hook 配線実装) を機械的 revert、indra/llrender/llglslshader.cpp + indra/cmake/00-Common.cmake から hook 痕跡全削除 / 06a-prep §2.8 検証完了後の除去 protocol 5 step 準拠 (= helper/macro/extern/setter body macro/CMake option 全削除、残存 grep 0 件、flag OFF build PASS) / Phase 0 計測 phase 完全完了 (= Step 1 pre-hook static / Step 2 hook 配線 / Step 3 AYA 計測 / Step 4 解析+§5 反映+副産物訂正 / Step 5 hook 除去 = 5 step 全完走) / 次 = Phase 1.A entry = (E')(F)(Q26-MUL)(Q27-CONFL) AYA 判断 + chapter 06b 起案
```

### §2.2 削除実施結果 (= literal scope、`indra/` 2 file revert)

| # | file | 削除内容 | line |
|---|---|---|---|
| 1 | `indra/llrender/llglslshader.cpp` | helper 2 個 (`ayaUboHookOnSetterByIndex` / `ByHashed`) + macro 2 個 + `extern U32 gFrameCount;` + anonymous namespace + `#ifdef AYASTORM_UBO_CADENCE_HOOK` block / 31 setter body 先頭 `AYA_UBO_HOOK_*` 行 × 31 全削除 | -71 |
| 2 | `indra/cmake/00-Common.cmake` | `option(AYASTORM_UBO_CADENCE_HOOK ...)` + `if(AYASTORM_UBO_CADENCE_HOOK) add_compile_definitions(...) endif()` + 説明 comment block 全行 | -9 |

合計 **-80 line** = `c27733ae79` (+80 line) と完全逆対称 = 機械的 revert PASS。

### §2.3 検証完了条件 (= 06a-prep §2.8 5 step 全件 PASS)

| step | 内容 | 結果 |
|---|---|---|
| 1 | log 解析完了 (= Step 4 で済) → cadence histogram 確定 → 06a §5.5 / chapter 05 反映待ち | ✅ Step 4 で §5.5 反映済、chapter 05 §7.3 matrix 補正は chapter 06b 起案直前 |
| 2 | `llglslshader.cpp` / `llglslshader.h` / `llappviewer.cpp` / CMake 関連の hook 行全削除 | ✅ `llglslshader.cpp` -71 + `00-Common.cmake` -9 (= 当初設計 `.h` / `llappviewer.cpp` 改変は実装時 skip 済、§2.3.3 frame counter 流用 + §2.3.1 extern 直接宣言で `.h` 公開不要だったため) |
| 3 | `git diff` で `AYASTORM_UBO_CADENCE_HOOK` / `AYA_UBO_HOOK` / `g_aya_ubo_hook` / `UBO_CADENCE` 残存 0 件 | ✅ Grep `AYASTORM_UBO_CADENCE_HOOK\|AYA_UBO_HOOK\|g_aya_ubo_hook\|UBO_CADENCE\|ayaUboHookOn` の `indra/` 配下 = 0 件 / `docs/specs/ayastorm-r41-gl-removal/` 内 = 70 件 (= 期待通り、§2.x spec 記述 + §5.5 観察記録 + handoff history) |
| 4 | flag OFF (= 通常) build PASS / flag ON build は不要 | ✅ autobuild `ReleaseFS_open` configure + build 100% built target、package 作成成功 (`Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261541010.tar.xz`)、EXIT=0 |
| 5 | migration commit へ進む | ✅ `4e40fd2ab0` commit 作成完了、共著行不在、working tree clean |

### §2.4 残った doc 系 spec 記述 (= 削除しない、Phase 1.A 入口資料として保存)

| file | 内容 | 残す理由 |
|---|---|---|
| `06a-prep-phase0-measurement.md` §2.x | hook 配線 spec 全部 (§2.1-§2.8) | Phase 0 計測手法の historical record、再計測が必要になった時の reproducer |
| `06a-prep-phase0-measurement.md` §5.5 | AYA 実機計測 観察結果 7 sub-section (§5.5.1-§5.5.7) | Phase 1.A 入口資料 (= chapter 06b 起案で cadence band inventory 358 件確定 + matrix 系補正 + aya_* 3 件追跡対象として参照) |
| handoff `*-step1/step2/step4/step5-complete.md` | 5 step の段階毎 handoff doc | session 履歴 = AYA 判断時の参照 |

---

## §3 次着手 = Phase 1.A entry (= Stage 2 + Stage 3 着手)

### §3.0 全体像 (= 6 sub-task 構成、本 handoff 範囲は doc 系 5 件 + 起案 1 件)

| sub-task | 内容 | scope | 担当 | 順序 |
|---|---|---|---|---|
| ST-1 | (E') 同一 binding 複数 UBO 名 grep 実施 (= 06a-prep §3 spec) | grep + chapter 10 反映 | Claude | 入口先頭 |
| ST-2 | (F) MaterialUBO vs MaterialUBO_Legacy 比較 (= 06a-prep §4 spec) | grep + read + chapter 10 反映 | Claude | ST-1 と並列可 |
| ST-3 | (Q26-MUL) / (Q27-CONFL) AYA 判断仰ぎ (= chapter 10 §1.5) | summary 提示 → AYA 判断 → chapter 10 反映 | Claude (summary) + AYA (判断) | ST-1/ST-2 結果反映後 |
| ST-4 | §5.5.5 `aya_*` 3 件 / §5.5.7 matrix 系 4-5 件 chapter 10 追記候補 AYA 判断 | summary 提示 → AYA 判断 → chapter 10 反映 | Claude (summary) + AYA (判断) | ST-3 と並列可 |
| ST-5 | (Q1) / (Q2) / (Q4) AYA 判断仰ぎ (= 09 §14.4 Stage 2 entry 必須) | summary 提示 → AYA 判断 → chapter 10 / 09 反映 | Claude (summary) + AYA (判断) | ST-3/ST-4 後 |
| ST-6 | chapter 06b 起案 (= cadence 5 種 update site 設計 + chapter 05 §7.3 matrix 補正反映) | 新 file 起案、`docs/specs/.../design/06b-*.md` | Claude (= 設計起案) | ST-1〜ST-5 結果集約後、最後 |

**indra/ 改変は ST-6 完了後の別 phase / 別 handoff** (= Phase 1.A 着手 = chapter 04 §6.4 NTTP 判定 + autobuild manifest pin 等の実装系 task に進む)。

### §3.1 ST-1 = (E') 同一 binding 複数 UBO 名 grep

06a-prep §3 spec に従い、`indra/newview/app_settings/shaders/` 配下で **同一 binding index に複数 UBO 名が紐付く可能性** を grep で全列挙、chapter 10 §1.5 の Q26-MUL の判断材料に変換する。

- input: 06a-prep §3 の grep pattern + 期待リスト 5 件
- output: 候補 N 件 × 該当 shader file 一覧 → chapter 10 §1.5 Q26-MUL 解消資料

### §3.2 ST-2 = (F) MaterialUBO vs MaterialUBO_Legacy 比較

06a-prep §4 spec に従い、両者の member 構造 diff を取り、Phase 1.A の対象 UBO 選定で MaterialUBO を残すか Legacy を残すかの判断材料に変換する。

- input: 06a-prep §4 の grep + read pattern
- output: 両者 member 差分表 → chapter 10 §1.4 / 第 1 UBO 選定 (Q1) 材料

### §3.3 ST-3 / ST-4 / ST-5 = AYA 判断仰ぎ集約

3 件の AYA 判断 batch を 1 session 内で消化する。判断結果は順次 chapter 10 / 09 に反映、Phase 1.A 着手 readiness (= 09 §14.5 Stage 3 14 項目) の 3-3 / 3-10 / 3-14 を消化。

| 判断仰ぎ単位 | 内容 | default 提案 |
|---|---|---|
| ST-3 | (Q26-MUL) (Q27-CONFL) (= chapter 10 §1.5 既登録 2 件) | (各 §1.5 内 default 欄を参照) |
| ST-4 | `aya_*` 3 件 + matrix 系 4-5 件 (= 新規追記候補) | 「追記する」が default、ST-6 起案前に chapter 10 反映 |
| ST-5 | (Q1) (Q2) (Q4) (= 09 §14.4 Stage 2 entry 必須 3 件) | (Q1) Template A 最小リスク UBO 優先 / (Q2) 1 UBO 厳守 / (Q4) Linux 完了後 Win/Mac 並走 |

### §3.4 ST-6 = chapter 06b 起案

chapter 06b は **cadence 5 種 update site 設計** (= per-draw / per-frame / per-program / per-asset / per-skin) の核となる chapter。06a-prep §6 反映 flow と §5.5 観察結果 + chapter 05 §7.3 matrix 補正を統合し、cadence 帯別の update site 配線設計を起案する。

- 起案単位: 新 file `docs/specs/ayastorm-r41-gl-removal/design/06b-<title>.md` (title は AYA と相談)
- 入力資料: 06a-prep §5.5 観察結果 + 05 §7.3 matrix 補正 + chapter 10 §1.5 ST-3/ST-4 確定結果 + ST-5 (Q1)(Q2) 確定結果
- 出力契約: 09 §14.5 Stage 3 self-check 3-8 (= chapter 06b cadence 5 種 update site + 06c descriptor set bind 起案済) を満たす最小 spec

### §3.5 規律 (= Phase 1.A entry 着手時 self-check)

1. **literal scope = doc 系のみ**: Phase 1.A entry session = grep (ST-1/ST-2) + AYA 判断仰ぎ (ST-3/ST-4/ST-5) + chapter 06b 起案 (ST-6) の **doc 系 6 sub-task のみ**、`indra/` 改変なし (= `feedback_design_phase_no_code_write` 継続)
2. **§5.5 / §2.x spec 記述は base material として参照**: 削除した hook 痕跡 (= `indra/` 側) は再現せず、`docs/` 側の §2.x / §5.5 記述だけで chapter 06b 起案する
3. **AYA 判断仰ぎは batch で**: ST-3 / ST-4 / ST-5 は 1 メッセージで全件 summary 提示、AYA 判断後にまとめて chapter 反映 (= `feedback_one_step_at_a_time` を厳守、AYA 判断 1 セッション内で全件消化)
4. **chapter 06b 起案は ST-1〜ST-5 結果集約後の最後**: 設計確定済の入力材料が揃ってから起案、起案途中で AYA 判断を割込ませない
5. **handoff 最小読み厳守**: 本 handoff §1.1 で 3 件、§1.2 pinpoint で必要時、それ以外の chapter は当該 sub-task 着手時のみ Read (= `feedback_handoff_minimal_pre_req_read`)
6. **`/tmp/aya_step4_analysis/` 産出物は再生成不要**: Step 4 の解析 script + tsv は AYA local 環境のみ、Phase 1.A entry では §5.5 反映済の結論のみ参照
7. **Q3 / Q5 は後ろ倒し可**: (Q3) OpenGL path 維持期間 / (Q5) Phase 番号化 は Stage 2 必須でない (= default 採用継続可)、Phase 1.A 中盤までに後追い OK、本 entry では仰がない

---

## §4 self-verify (= 本 handoff 起こした時点の整合性)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) 前 handoff (`...phase0-step4-complete.md`) からの遷移整合 | 前 §3 「次 = Step 5 hook 除去 phase」 → 本 handoff 「Step 5 完了 = Phase 0 全完走 → Phase 1.A entry」 起点と一致 | ✅ |
| (2) commit `4e40fd2ab0` 確認 | `git log --oneline -3` で HEAD と一致 (前 = `d6bd990882`)、`git show --stat` = `00-Common.cmake` -9 + `llglslshader.cpp` -71 = -80 line | ✅ |
| (3) `c27733ae79` 機械的逆対称 | Step 2 commit (+80 line) と完全逆対称 (-80 line)、grep 残存 `indra/` 配下 0 件 | ✅ |
| (4) flag OFF build PASS | autobuild `ReleaseFS_open` 100% built target、package 作成成功、EXIT=0 | ✅ |
| (5) `feedback_remove_verification_logs` 準拠 | 検証用 LL_INFOS hook を commit 前に出荷物から除去、Step 5 commit が出荷 state | ✅ |
| (6) `feedback_design_phase_no_code_write` 復帰 | Phase 0 計測 phase (Step 2/Step 5) のみ `indra/` 解禁、本 handoff 以降は doc 系 phase に戻る (= Phase 1.A entry doc sub-task 6 件) | ✅ |
| (7) `feedback_no_scope_shrink` 準拠 | Step 5 削除 scope は handoff §3.1 の literal 全件 (= helper + macro + extern + 31 setter body + CMake option + comment block) を機械的 revert、scope 縮小なし | ✅ |
| (8) `feedback_handoff_minimal_pre_req_read` 準拠 | 本 §1.1 = 3 件のみ列挙、§1.2 = pinpoint Read reference 分離 | ✅ |
| (9) `feedback_no_claude_coauthor` 準拠 | `4e40fd2ab0` commit message に `Co-Authored-By: Claude` 不在 | ✅ |
| (10) Phase 0 計測 phase 5 step 全完走 | Step 1 `d1099b4d64` / Step 2 `c27733ae79` / Step 3 (AYA 計測、commit 不在 phase) / Step 4 `6cbfbb28fa` + `c3e64095fe` / Step 5 `4e40fd2ab0` = 5 step 全 commit 確認 | ✅ |

---

## §5 引き継ぎ済の memory (= 次 session も活きる)

- `feedback_handoff_minimal_pre_req_read` (= 本 handoff §1 構造の根拠)
- `feedback_design_phase_no_code_write` (= Phase 1.A entry の doc 系 6 sub-task で `indra/` 改変なし継続、実装 entry は別 handoff で解禁)
- `feedback_no_scope_shrink` (= Step 5 機械的 revert で literal scope 全件削除、Phase 1.A entry の ST-1〜ST-6 でも同様)
- `feedback_self_verify_before_handoff` (= ST-3/ST-4/ST-5 AYA 判断仰ぎ前に Claude が summary 提示)
- `feedback_no_claude_coauthor` (= 全 commit 共著行禁止)
- `feedback_one_step_at_a_time` (= AYA 判断仰ぎ batch は 1 メッセージで全件提示、AYA 判断後にまとめて反映)
- `feedback_remove_verification_logs` (= Step 5 で実証済、今後の計測 phase でも同 protocol)
- `project_ayastorm_r41_vulkan_migration` (= r41 章 active pointer)
- `project_ayastorm_r41_design_principles` (= upstream 取込容易性 + core 並列化容易性の 2 大設計原則、chapter 06b 起案の前提)
- `feedback_ubo_migration_one_at_a_time` (= Phase 1.A 以降の UBO migration 規律、(Q2) 1 UBO 厳守 default の memory 根拠)

---

## §6 次 session 着手 1 line

「Step 5 commit `4e40fd2ab0` (= `c27733ae79` 機械的 revert / `indra/` -80 line / flag OFF build PASS) で Phase 0 計測 phase 5 step 全完走完了。次 = Phase 1.A entry = doc 系 6 sub-task = ST-1 (E') grep / ST-2 (F) MaterialUBO 比較 / ST-3 (Q26-MUL)(Q27-CONFL) AYA 判断 / ST-4 `aya_*` 3 件 + matrix 系 4-5 件 chapter 10 追記候補 AYA 判断 / ST-5 (Q1)(Q2)(Q4) AYA 判断仰ぎ / ST-6 chapter 06b 起案 (= cadence 5 種 update site 設計、ST-1〜ST-5 結果集約後最後)。`indra/` 改変なし継続 (= `feedback_design_phase_no_code_write` 復帰)、ST-3/ST-4/ST-5 AYA 判断仰ぎは batch 1 メッセージで全件 summary 提示、(Q3)(Q5) は後ろ倒し可で本 entry 範囲外」
