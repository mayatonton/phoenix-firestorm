# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2d-β pivot to codegen design deepening handoff

**作成日**: 2026-06-03
**branch**: `feature/ayastorm-r41-gl-removal`
**前 handoff (= REJECT 履歴対象)**: `handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-beta-prep.md` (commit `95e690b426`)
**設計 source of truth**: `design/04-codegen-ubo.md` + `design/08-build-codegen-pipeline.md` (= 新 scope の中核 spec)
**状態**: **方向性 pivot 確定**。旧 Phase 2d-β prep (= Phase 0 計測 docs polish 方向) を REJECT 履歴として保存、新 Phase 2d-β scope を **chapter 04 + 08 codegen pipeline prototype 深化** に置換。本 handoff で pivot 起源 + 新 scope + 着手前提を確定し、次 session 着手地点に渡す。

---

## §0 本 handoff の目的 (= 認識転換 + scope 置換)

### §0.1 認識転換 (= AYA 指摘 2026-06-03 由来)

η-28 sub-bundle で進めてきた一連の作業 (Phase 2a-2c / Phase 2d-α / Phase 2d-β prep) は **「設計に合わせて作る → 大量エラー → エラー取り」の後半 (= エラー取り) を、前半 (= 設計に合わせて作る) なしに実施し続けた状態** だったと確定:

| phase | 作業 | 「設計に合わせて作る」前半の有無 |
|---|---|---|
| Phase 2a-2c | 既存 vulkanize 出力の parse error 解消 (= η-28-A〜η-28-F 範式適用) | × (= 既存 vulkanize C++ + 既存 GLSL を温存したまま error 潰し) |
| Phase 2d-α | TerrainMix / SpotLight / PBRMix struct guard + cross-stage UBO 共有 | × (= 同上、parse pass を目標とする error 潰し) |
| Phase 2d-β prep (旧、commit `95e690b426`) | Phase 0 計測 docs 化 (06a-prep §2/§3/§5.5 polish) | × (= 計測準備の docs polish、設計本丸の codegen + redirect 層に距離) |

AYA 言: 「設計に合わせて作って大量にエラーが出てそこからエラー取りをしてくならわかるが、今あるエラーを適当に潰そうとしても何も仕事になってない」

→ **エラー取りは設計に合わせて作って初めて意味ある仕事になる**、設計に合わせて作れていない状態で error 潰しを続けても「何も仕事になっていない」。

### §0.2 旧 Phase 2d-β prep の REJECT 履歴記録 (= 削除でなく archive)

旧 prep (commit `95e690b426`) は **REJECT 履歴として保存** (= memory `feedback_falsification_as_progress` + `feedback_build_only_verified` 準拠、削除 / revert しない):

| 旧 prep 内 deliverable | REJECT 理由 |
|---|---|
| A: setter 30 entry point cookbook 反映 (= 06a-prep §2 polish) | docs polish に scope 縮退、設計本丸 (codegen + redirect 層) を進めない |
| B: 既存 GL state cookbook 反映 (= 06a-prep §1.1 入力 source 拡張) | 同上 |
| C: 既存 vulkanize C++ bug 2 件を (G') 計測対象として 06a-prep §5.5 追加 | bug の根本修正は Phase 1.B (redirect 層実装) で処理、計測対象化で deferred するのは「適当に潰す」trap の同型 |
| bridge phase 範式新設 (= design-phase ↔ implementation-phase 境界) | 概念自体は有効、ただし bridge phase の literal scope を Phase 0 計測 docs polish に絞ったのが trap |

**bridge phase 範式の扱い**: 概念として有効、適用は将来 phase (= 例えば η-29 Phase 0 計測完了後の chapter 06b 起案前の bridge 等) で温存。本 handoff 起案時点では棚上げ。

### §0.3 新 Phase 2d-β scope (= chapter 04 + 08 codegen pipeline prototype 深化)

設計 chapter 04 (Codegen-UBO) + 08 (codegen pipeline) を **Phase 1.A 入口直前まで深化** = 「設計通り作れば動く」状態まで仕様密度を上げる:

| 深化対象 | source chapter | 現状 | 深化目標 |
|---|---|---|---|
| 84 UBO blueprint → host accessible UBO struct 変換 spec | 04 §6 (name-based 解決 dispatch) + 06a §4 (perfect hash table) | dispatch 機構 spec はあるが、変換 generator の **実 algorithm レベル詳細** 未起案 | input GLSL ファイル群 → 出力 C++ header (ubo_metadata.inl + ubo_host_loader.inl) の完全 generator spec |
| std140 offset calculator | 04 §7 + 08 §17 (A1) | 二重保証 protocol あり、ただし計算 algorithm 詳細未起案 | std140 rule 6 条 (= base alignment / array stride / matrix col-major / struct alignment / vec3 padding / bool size) を全列挙、各 rule の generator 実装 pseudo-code |
| perfect hash table 生成 algorithm | 04 §6 + 08 §17 (G/B3) | CHD/FCH algorithm 採用方針あり、ただし具体 algorithm 詳細未起案 | CHD algorithm の bucket 配置 / displacement table 計算 / lookup function 生成 の pseudo-code レベル spec |
| SPIR-V reflection 二重保証 protocol | 08 §17 (A1) | 二重保証方針あり、ただし照合 algorithm 未起案 | spirv-cross 出力 JSON parse → 独自 calculator 出力 diff → 不一致時 build fail flow の pseudo-code |
| GLSL parse 独自 mini-parser + glslang -E 前処理 | 08 §17 (P) | mini-parser 採用方針あり、ただし parse rule 未起案 | UBO block 宣言の **正規表現 / token rule** 完全列挙、edge case (= nested struct / array / matrix) handling |
| 増分 build cache hash + mtime 併用 | 08 §17 (B4) | hash + mtime 方針あり、ただし invalidation rule 未起案 | hash 対象 file 列挙 / mtime 比較順序 / cache invalidation trigger 完全列挙 |
| CMake DEPENDS 自動 + 手動 force target | 08 §17 (B5) | DEPENDS + force 方針あり、ただし CMake patch 配置 file 未確定 | `indra/cmake/<file>.cmake` 配置先候補 + 既存 entry との干渉 grep 計画 |

### §0.4 「設計に合わせて作る → エラー取り」の正しい flow (= 本 handoff 起源の新範式)

```
[現状の trap]
  error 露呈 → error 潰し → 次の error 露呈 → error 潰し → ... (= 設計本丸進展ゼロ)

[正しい flow]
  設計深化 → 設計通り実装 → 大量 error 露呈 → error 取りで仕上げ → 意味ある成果
                ↑                                        ↑
                Phase 1.A〜1.C で実施                    Phase 1.X-vN 等で再 phase 化
```

**本 handoff 確定**: η-28 sub-bundle 内で error 潰しを継続せず、Phase 1.A 入口前の設計深化に振り替え。Phase 1.A で「設計通り作る → 露呈する大量 error の取り」が **意味ある仕事** になる state 到達を本 Phase 2d-β-revise scope に確定。

---

## §1 pre-requisite 必読 file (= 最低限 3 件、memory `feedback_handoff_minimal_pre_req_read` 準拠)

### §1.1 必読 1: 本 handoff (= 本 file)
- 認識転換 + 旧 prep REJECT 履歴 + 新 scope + 着手前提

### §1.2 必読 2: 設計 chapter 04 (Codegen-UBO)
- `design/04-codegen-ubo.md` 全体 (= 新 scope の中核 spec、Phase 1.A 入口前深化の主対象)

### §1.3 必読 3: 設計 chapter 08 (codegen pipeline)
- `design/08-build-codegen-pipeline.md` 全体 (= chapter 04 と並列の深化対象、§17 持越 7 件 (A1/P/G/B3/B1-B5) が深化スコープ)

### §1.4 pinpoint Read (= 深化作業中に必要時のみ)
- `design/06a-cache-structure-and-setter-redirect.md` §3-§5 (= redirect 層 spec、chapter 04 generator 出力 header の consumer 側)
- `design/09-phase-roadmap.md` §4 (= Phase 1.A/B/C Exit Criteria、深化目標の達成基準)
- `ayastorm-r41-ubo-current-state-inventory.md` §3 (= 既存 84 UBO blueprint の generator 入力 source)
- `design-review-2026-06-03-second-pass.md` §7 / §8 (= AYA 判断 4 + 修正 20、着手前提条件)
- 旧 Phase 2d-β prep (commit `95e690b426` 参照、= REJECT 履歴の参照、再採用しない)

---

## §2 新 Phase 2d-β-revise literal scope (= chapter 04 + 08 prototype 深化、`indra/` 改変ゼロ)

### §2.1 Deliverable A: chapter 04 generator algorithm 深化

| sub-task | 反映先 § | 内容 |
|---|---|---|
| A-1 | 04 §6 拡張 or 新規 §6a | name-based 解決 dispatch の **実 generator algorithm** = 入力 GLSL block 宣言 → 出力 `<UBO>_offset_table.inl` + `<UBO>_setter_dispatch.inl` の生成 pseudo-code、edge case (nested struct / array / matrix col-major / vec3 padding) を全列挙 |
| A-2 | 04 §7 拡張 | std140 offset calculator の **rule 6 条全列挙** + 各 rule の pseudo-code (= base alignment / array stride / matrix col-major / struct alignment / vec3 padding / bool size)、SPIR-V reflection との照合 protocol を pseudo-code レベル |
| A-3 | 04 §6 拡張 | perfect hash table 生成 CHD algorithm の pseudo-code = (i) bucket 分割 (ii) displacement table 計算 (iii) lookup function 生成、生成 header の compile-time 衝突保証論証 |

### §2.2 Deliverable B: chapter 08 codegen pipeline prototype 深化

| sub-task | 反映先 § | 内容 |
|---|---|---|
| B-1 | 08 §17 (A1) 拡張 | std140 offset 二重保証 protocol = spirv-cross JSON parse → 独自 calculator 出力 diff → 不一致時 build fail flow の pseudo-code、不一致 fix flow (= どちらが正か判定 protocol) を明示 |
| B-2 | 08 §17 (P) 拡張 | GLSL parse 独自 mini-parser の **token rule + 正規表現完全列挙**、edge case handling (nested struct / array / matrix / preprocessor gate 内 block)、glslang -E 前処理の入出力 contract |
| B-3 | 08 §17 (B4) 拡張 | 増分 build cache hash + mtime 併用 protocol = hash 対象 file 列挙 / mtime 比較順序 / cache invalidation trigger 完全列挙、CMake DEPENDS 配線との関係 |
| B-4 | 08 §17 (B5) 拡張 | CMake DEPENDS + 手動 force target の `indra/cmake/<file>.cmake` **配置先候補 listing** + 既存 entry との干渉 grep 計画 (= Phase 1.A 入口で実 grep 実施できる 1 step pinned command) |
| B-5 | 08 §13 拡張 or 新規 §13a | 3 OS 確証 X-α/β/γ の codegen 出力 binary identical 保証 (= reproducible build 要求) の 達成 protocol = file ordering / line ending / encoding 制御の pseudo-code |

### §2.3 Deliverable C: Phase 1.A 入口 1 step 着手可能 state 確定 protocol

| sub-task | 反映先 § | 内容 |
|---|---|---|
| C-1 | 04 / 08 末尾に新規 §「Phase 1.A 入口 1 step checklist」 追加 | 深化完了状態の確認 checklist (= Deliverable A/B 全 sub-task 反映済 + spec 密度が pseudo-code レベル + Phase 1.A 着手時に AYA + Claude が同一 doc 駆動可能)、Phase 1.A Exit Criteria (= 09 §4.2) と整合確認 |
| C-2 | 04 / 08 cross-ref 表追加 | 04 / 08 / 06a / 09 間の dependency 図 = 04 generator 出力 → 06a redirect 層消費 / 08 pipeline 駆動 / 09 Phase 1.A Exit 判定、本 phase で **04/08 深化 = 06a/09 の consumer 契約満たし** であることの可視化 |

---

## §3 Phase 2d-β-revise 着手前提条件 (= 旧 prep §5 から継承、依然 valid)

### §3.1 前提 1: AYA 判断 4 件完了 (= 第二次査読 §7)

| ID | 内容 | 本 phase との関係 |
|---|---|---|
| **Q22-NUM** | set=2 帯 25 vs 26 (= 総数 84 vs 85) | chapter 04 generator 入力 listing 数値整合に影響 |
| **Q23-K** | chapter 09 §2.1 K placeholder と §5.2 template 矛盾 | Deliverable C-1 Exit Criteria 整合に影響 |
| **Q24-S1** | chapter 06a §9 (S1) 持越 vs 06a-prep §2.2.2「(S1) 解消」 | chapter 04 generator 出力 header の consumer 側 spec 整合に影響 (= 06a §4 redirect 層) |
| **Q25-21CNT** | chapter 10 §1 21 件 double-count 検証 | 本 phase 着手判定の前提 review 完了確認に影響 |

### §3.2 前提 2: 設計 chapter 群 修正推奨 20 件 反映完了

- 第二次査読 §8.1 18 件 + audit §3.1 2 件 = 20 件 (main session 並行処理)
- 04 / 08 の修正 (= 第二次査読 §8.1 chapter 04 1 件 + 09 5 件) は **本 phase 直接対象**、main session で先行反映後本 phase 着手

### §3.3 前提 3 (新規追加): 旧 Phase 2d-β prep REJECT 履歴の handoff 反映完了

- 本 handoff (= 本 doc) を読込済 = 旧 prep の方向性 REJECT を認識
- 新 scope (= chapter 04 + 08 prototype 深化) を内面化、旧 prep の Deliverable A/B/C (= Phase 0 計測 docs polish) を本 phase で再採用しない

---

## §4 着手前提未充足時の handling

### §4.1 AYA 判断 4 件のうち Q22-NUM / Q24-S1 未受領

→ 本 phase 着手不可、main session 待ち (= 旧 prep §6.1 と同型 handling、memory `feedback_doubt_self_first` 準拠の待機選択)。

### §4.2 修正推奨 20 件のうち 04 / 08 / 09 関連未反映

→ 本 phase 着手不可、main session 反映完了確認後着手。

### §4.3 部分着手 (= Deliverable A のみ実施 等) の扱い

- **禁止**: memory `feedback_no_scope_shrink` 準拠、本 phase literal scope = A + B + C 全 3 deliverable 一括、scope shrink 違反となる。

---

## §5 Phase 2d-β-revise 完了条件 + 次 step (= Phase 1.A 入口)

### §5.1 完了条件 (= 4 件全達成)

- [ ] Deliverable A (= chapter 04 深化): A-1 / A-2 / A-3 全 sub-task の chapter 04 反映 + diff 検証
- [ ] Deliverable B (= chapter 08 深化): B-1〜B-5 全 sub-task の chapter 08 反映 + diff 検証
- [ ] Deliverable C (= Phase 1.A 入口 1 step state): C-1 / C-2 全 sub-task の 04 / 08 反映 + 09 §4 cross-ref 整合
- [ ] self-verify: 04 / 08 全文を Read で再読 → pseudo-code レベル密度確認 + Phase 1.A 着手時 AYA + Claude が同一 doc 駆動可能 verify (= memory `feedback_self_verify_before_handoff`)
- [ ] complete handoff doc 起草 (= 本 prep を source of truth として完了状態記述)
- [ ] commit (= memory `feedback_no_auto_commit` 準拠、AYA 明示指示後)

### §5.2 完了後の次 step = Phase 1.A (η-30、`indra/` 改変解禁)

- `design/09-phase-roadmap.md` §4.1 Phase 1.A sub-Phase scope (= codegen pipeline 実装) 着手
- 本 phase で確定の chapter 04 + 08 spec を **そのまま実装** に落とす flow:
  1. Python script 起案 (= chapter 04 / 08 spec の pseudo-code を実 code 化)
  2. 既存 84 UBO blueprint を入力に generator 実行 → header 出力
  3. **大量 build error / 不整合露呈** = 本格的「error 取り」phase へ移行
  4. error 取りで chapter 04 / 08 spec の不足を露呈・補完 → Phase 1.A-v2 として再 iteration
- 「設計通り作る → 露呈する大量 error の取り」が **意味ある仕事** になる state、本 phase 完了で到達。

### §5.3 Phase 0 (η-29) 計測 phase との関係

- 旧 prep が想定していた Phase 0 (= 計測 phase、η-29) は **Phase 1.A と並行 or 後段で実施**:
  - Phase 1.A は Phase 0 結果非依存 (= 84 UBO blueprint は既存、cadence 情報は Phase 1.C update site で必要)
  - Phase 0 計測着手の docs polish (= 旧 prep scope) は **Phase 1.A 進行中の sub-task** もしくは **Phase 1.C 入口前 bridge** で再起案候補
- chapter 09 §2.1 Phase マップの Phase 0 → Phase 1 順序は **設計 source of truth として尊重**、ただし design-phase 内の **どの phase で深化するか** は本 handoff で「chapter 04 + 08 先行深化」と確定。

---

## §6 範式継承 + 本 phase 適用範式

### §6.1 継承

- `feedback_design_phase_no_code_write` (= 本 phase は docs 化のみ、`indra/` 改変ゼロ、Phase 1.A 入口で解禁)
- `feedback_handoff_minimal_pre_req_read` (= 必読 3 件)
- `feedback_no_scope_shrink` (= 3 deliverable 一括 literal scope §4.3)
- `feedback_one_step_at_a_time` (= main session 完了待ち §3 / §4)
- `feedback_self_verify_before_handoff` (= 完了条件 §5.1)
- `feedback_no_auto_commit` (= commit は AYA 明示指示後)
- `feedback_doubt_self_first` (= 前提未充足時 §4 は待機選択)
- `feedback_falsification_as_progress` (= 旧 Phase 2d-β prep を REJECT 履歴として保存 §0.2)
- `feedback_build_only_verified` (= 旧 prep の方向性 REJECT を「正しいことを積み上げる」原則で確定 §0.2)

### §6.2 本 handoff 新規確立範式

- **「設計に合わせて作る前半なくしてエラー取り後半をしない」範式 (= 本 handoff §0.4 起源)**:
  - 「設計に合わせて作る → 大量 error → error 取りで仕上げ」の flow を **前半 (= 設計に合わせて作る)** から始める
  - 既存実装の error を「適当に潰す」phase は設計本丸進展なしの場合、**phase として成立しない** (= 「何も仕事になっていない」)
  - 適用判定: 「この phase で設計本丸 (= chapter NN spec or `indra/` 内 design conformant 実装) が進むか?」が yes なら起案可、no なら別 phase に振り替え
  - 範式新規追加判断: 本 handoff の認識転換 (= η-28 sub-bundle 全体が前半なしの後半連発だったとの自己 audit) で確定、将来 phase 起案時の最終 check として参照

---

## §7 関連 commit / handoff cross-ref

### §7.1 残置 commit (= Phase 2d-α 系、parse pass 自体は維持)

| hash | 種別 | 扱い |
|---|---|---|
| `0587c574da` | feat | Phase 2d-α 初版、残置 (= parse pass は維持価値あり) |
| `9a576884c0` | docs | reference doc 更新、残置 |
| `e886fa92c7` | feat | Phase 2d-α Fix、残置 |
| `c7cdaa279a` | docs | Phase 2d-α complete handoff、残置 (= Phase 2d-α 自己完結記録) |

### §7.2 REJECT 履歴 commit (= 旧 Phase 2d-β prep)

| hash | 種別 | 扱い |
|---|---|---|
| `95e690b426` | docs | 旧 Phase 2d-β prep、**REJECT 履歴として archive** (削除 / revert しない、§0.2 参照) |

### §7.3 関連 handoff

- `handoff-substep-...-eta-28-phase2d-alpha-complete.md` (= Phase 2d-α 自己完結、本 pivot に影響しない)
- `handoff-substep-...-eta-28-phase2d-beta-prep.md` (= 旧 Phase 2d-β prep、本 handoff で REJECT 認定)
- `handoff-substep-...-eta-28-pivot-to-ubo-design.md` (= η-28 UBO 設計 pivot、本 handoff の **同型先行例** = 方向性 pivot を handoff で記録)

### §7.4 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` (= r41 milestone active)
- `project_ayastorm_r41_design_principles.md` (= 2 大設計原則、本 pivot は原則 1+2 への直接寄与優先)
- `feedback_design_phase_no_code_write` (= 本 phase の規律根幹)
- `feedback_falsification_as_progress` (= 旧 prep REJECT 履歴保存の規律)
- `feedback_build_only_verified` (= 「正しいことを積み上げる」原則、本 pivot の根本動機)
- 新規追加候補 (= 本 handoff §6.2 由来) = `feedback_design_first_then_error_fix` (仮称、別 memory として保存予定)

---

## §8 本 handoff の制約 (= 次 session 開始時に Claude が忘れがちな点)

- **旧 Phase 2d-β prep (commit `95e690b426`) は REJECT 履歴**: 削除 / revert しない、新 phase で再採用もしない、archive として保存のみ
- **新 Phase 2d-β-revise は docs 深化のみ**: `indra/` 改変ゼロ、Phase 1.A (= η-30) で解禁、本 phase で codegen 実 code は書かない
- **着手前提は 2 件 + 1 件 = 3 件**: AYA 判断 4 件 + 修正推奨 20 件 + 本 handoff REJECT 認識、いずれか未充足は §4 で着手不可規定
- **literal scope は 3 deliverable 一括**: 部分着手禁止 (§4.3)、memory `feedback_no_scope_shrink` 違反
- **「適当 error 潰し」の誘惑を抑止**: 深化作業中に既存 error の話題が出ても本 phase scope に取り込まない (= 設計本丸進展なしの作業を混入させない、§6.2 新範式厳守)
- **本 handoff は次 session 着手前に必読 1 件として必ず読む**: 旧 prep の REJECT を認識せず旧 scope で着手すると pivot 効果が消える
- **新範式 (= §6.2) は memory 追加候補**: 本 handoff 完了 + AYA 確認後、memory `feedback_design_first_then_error_fix` (仮称) として永続化候補、本 handoff 起案時点では handoff 内のみ記述

---

**本 handoff は Phase 2d-β-revise (= chapter 04 + 08 codegen pipeline prototype 深化) 完遂までの永続参照**。完了後は `handoff-substep-...-eta-28-phase2d-beta-revise-complete.md` (起案予定) に役割移管。次 phase は Phase 1.A (= η-30、`indra/` 改変解禁 + codegen pipeline 実装) として implementation-phase 入口へ遷移、Phase 0 (η-29) 計測 phase は Phase 1.A 進行中 or Phase 1.C 入口前 bridge で並行 / 後段実施を chapter 09 §2.2 dependency 図と整合させて確定。
