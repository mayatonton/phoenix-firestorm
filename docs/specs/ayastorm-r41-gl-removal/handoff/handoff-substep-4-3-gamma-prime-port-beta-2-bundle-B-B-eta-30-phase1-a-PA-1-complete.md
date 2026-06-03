# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-1 完了

**作成日**: 2026-06-03
**前 session commit**: `3a4b38991a` (PA-1 完了)
**次 session 着手**: PA-2 (Codegen Python script base 構造)

---

## §0 state 一行 summary

η-30 **Phase 1.A PA-1 完了 state** (= spirv-cross 取込 + (B2) B2b system pkg 確定 + PA-1 真 scope 訂正 + version drift doc 更新)。**本 session 着手地点 = PA-2 (= Codegen Python script base 構造 = `scripts/ubo_codegen/main.py` 起草)**。

設計 phase 完了 (= Stage 3 14/14 ✅) 後の初実装 commit が `3a4b38991a` で着地済。`feedback_design_phase_no_code_write` 完全解禁 state、`indra/` + `scripts/` 配下改変自由化済。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。PA-2 着手 session では **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read (offset/limit) する。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-PA-1-complete.md` | 全文 | 本 handoff (= PA-1 完了 state + PA-2 着手地点) |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-entry.md` | §3 PA-X 全体構成表 + §2 Phase 1.A scope + Exit Criteria | Phase 1.A 全体像 + PA-2 sub-task scope literal 参照 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` | §5 (= I/O 契約 + script 構造) + §6 (= log 出力) + §7 (= 失敗 exit code) + §11.5 (= 増分 build cache) | PA-2 Codegen Python script base 設計 source of truth |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` | §10 (B1) Python 3.8+ 要件 + §4.3.1 std140 calculator (= PA-4 連動) + §5.6 perfect hash CHD (= PA-5 連動) |
| `docs/specs/ayastorm-r41-gl-removal/design/02-naming-convention.md` | §2.4 Codegen-UBO 生成識別子 4 種 = PA-2 出力 file 名 + 生成 symbol 命名規約 |
| `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md` | §3.1-§3.4 = 85 UBO blueprint inventory (= PA-2 入力契約定義 = PA-8 実行対象) |
| `indra/cmake/SpirvCross.cmake` | PA-1 起案物 (= 28 line、Glslang.cmake と完全 symmetric)、PA-3/PA-4 で実 link 発火予定 |
| `indra/cmake/Glslang.cmake` | PA-3 glslang 統合の参照 pattern (= find_package(glslang CONFIG REQUIRED) + ll::glslang INTERFACE IMPORTED) |
| `indra/cmake/Python.cmake` | PA-7 CMake DEPENDS 配線時の Python interpreter 参照 (= find_package(Python3 COMPONENTS Interpreter)) |

---

## §2 本 session 成果 (= PA-1 完了 内訳)

**commit**: `3a4b38991a` (5 files changed, +63/-19, 1 new file)

### §2.1 PA-1 真 scope 達成

| 項目 | 状態 |
|------|------|
| `indra/cmake/SpirvCross.cmake` 起案 (28 line) | ✅ Glslang.cmake と完全 symmetric pattern |
| system install (`sudo apt install libspirv-cross-c-shared-dev` = Ubuntu 24.04 = 1.3.239.0) | ✅ AYA 実行 |
| find_package verify | ✅ cmake config (= `/usr/share/spirv_cross_c_shared/cmake/spirv_cross_c_sharedConfig.cmake` Debian 慣例 path) + target 名 `spirv-cross-c-shared` + include dir `/usr/include/spirv_cross` 全件整合 |
| version drift doc 更新 | ✅ cache key env block 実 install 値反映 = Python 3.12.3 / glslang 15.1.0 / spirv-cross 1.3.239.0 |

### §2.2 PA-1 entry 直前 gap remediation (= `feedback_doubt_self_first` 適用)

ST-7 sub-task 6 verdict は `autobuild.xml` **片側 grep のみ**で「3 dependency 0 件 = 全件追加必要」と判定したが、PA-1 entry 直前で `indra/cmake/` + `scripts/` 横断 verify (= Agent Explore medium thoroughness) 実行 → finding 3 件:

- **glslang**: `indra/cmake/Glslang.cmake` で `find_package(glslang CONFIG REQUIRED)` + `glslang-15.1.0/` vendored + Ubuntu 24.04 `apt install glslang-dev` (15.1.0-2) 既存取込済 = chapter 10 §1.2 (B2) **B2b system pkg 確定**
- **Python**: `indra/cmake/Python.cmake` で `find_package(Python3 COMPONENTS Interpreter)` 既存取込済 (= autobuild manifest 経路使わず host 探索)
- **spirv-cross**: 真に未取込 = PA-1 真 scope

AYA「A」応答で PA-1 scope = spirv-cross のみ取込 に訂正。

### §2.3 連動 update 範囲 (= 5 file 計 22 edit + 1 new file)

| file | edit 数 | 内容 |
|---|---|---|
| `indra/cmake/SpirvCross.cmake` (new) | 1 (file 新規) | Glslang.cmake と完全 symmetric pattern |
| `docs/.../handoff/...phase1-a-entry.md` | 8 | §0 state + §3 PA-1 cell + §3 (Q-NTTP) + §4 (B2) + §5 規律 11/12 + §6 row 10 + §7 memory + §8 着手 1 line |
| `docs/.../design/08-build-codegen-pipeline.md` | 3 | §5.4.1.5 format pin paragraph 全書換 + §5.4.1.1 line 538 autobuild bundle → B2b 記述 + §11.5.1 cache key env 実値反映 |
| `docs/.../design/09-phase-roadmap.md` | 5 | §14.4 sub-task 6 post-completion note + §14.4 末尾 gap remediation paragraph 新設 + §14.4 sub-task 6 (c) example 値 inline 訂正 + §14.5 row 3-11 verdict 訂正 + version drift example 値書換 |
| `docs/.../design/10-open-questions.md` | 6 | §1.0 row 9 (B2) ✅ + §1.0 header count 12→13 + §1.0 count 内訳 13 判断済 / 16 未判断 + §1.0 (B2) cross-ref paragraph 新設 + §1.0 未判断 17→16 件 解消順序 update + §1.2 (B2) row B2a→B2b 確定 |

### §2.4 Stage 3 14/14 ✅ verdict 自体は維持

(= 設計 phase 完了状態は変わらず、本 correction は scope 縮小 1 件 (= spirv-cross のみ) + (B2) verdict 確定 + verdict 訂正 mechanism の補強)。

---

## §3 次着手地点 = PA-2 (Codegen Python script base 構造)

### §3.1 着手 1 line

「**PA-2 = `scripts/ubo_codegen/main.py` 起草 = entry point + arg parse + I/O 契約 + log 出力 + 失敗 exit code 配備 = `--input <blueprint_path> --output <header_dir>` で起動可能、空入力で正常終了**」(= handoff entry doc §3 PA-2 row literal 継承)

### §3.2 PA-2 scope (= entry handoff §3 PA-2 row literal)

| 項目 | 内容 |
|---|---|
| 出力 | `scripts/ubo_codegen/main.py` 新規起草 (= AYAstorm r41 新設 directory) |
| 依存 chapter | 08 §5 + §6 + §7 + 04 §10 (B1) Python 3.8+ 要件 |
| 起動契約 | `python3 main.py --input <blueprint_path> --output <header_dir>` |
| 完了条件 | 空入力で正常終了 (= 0 件処理 + exit 0)、log 出力 + 失敗時 exit code 配備 |

### §3.3 PA-2 完了後の次 task

PA-3 (= GLSL mini-parser + glslang -E 前処理) と **PA-4 (= std140 calculator + SPIR-V reflection 二重保証)** が並列実施可 (= entry handoff §3 注「PA-3 + PA-4 のみ並列」)。PA-5 〜 PA-8 は strict 線形。

### §3.4 PA-2 は Claude 自走可 (AYA 判断不要)

(= 入力契約 + 起動 syntax + Python 3.8+ 要件は entry handoff + chapter 08 で確定済、AYA 判断仰ぎ batch 不要)。

---

## §4 self-verify (= 9 観点 PASS)

| # | 観点 | 結果 |
|---|---|---|
| 1 | commit `3a4b38991a` 履歴に PA-1 完了記載 | ✅ |
| 2 | `indra/cmake/SpirvCross.cmake` 物理存在 (28 line) | ✅ |
| 3 | 5 file 計 22 edit 反映済 | ✅ |
| 4 | chapter 10 §1.2 (B2) ✅ B2b system pkg 確定 | ✅ |
| 5 | chapter 10 §1.0 count 13 判断済 / 16 未判断 | ✅ |
| 6 | chapter 08 §11.5.1 cache key env 実値反映 (3.12.3/15.1.0/1.3.239.0) | ✅ |
| 7 | Stage 3 14/14 ✅ verdict 維持 (= 設計 phase 完了状態 変わらず) | ✅ |
| 8 | 共著行不在 (`Co-Authored-By: Claude` 不在 commit) | ✅ |
| 9 | handoff doc 命名対称 (`...eta-30-phase1-a-PA-1-complete.md`) | ✅ |

---

## §5 引き継ぎ済 memory (= PA-2 entry session で重要度 ↑)

- `feedback_handoff_minimal_pre_req_read` (= 全件読み禁止、本 §1.1 3 件のみ)
- `feedback_design_phase_no_code_write` (= 完全解禁済、PA-2 で `scripts/` 配下新規 directory 作成可)
- `feedback_no_scope_shrink` (= PA-2 scope = entry handoff §3 PA-2 row literal、勝手な縮小禁止)
- `feedback_self_verify_before_handoff` (= AYA 確認前に Claude 全 sub-task self-verify)
- `feedback_no_claude_coauthor` (= 全 commit 共著行禁止)
- `feedback_one_step_at_a_time` (= 1 メッセージ 1 アクション、AYA に質問する場合も 1 件ずつ)
- `feedback_doubt_self_first` (= PA-2 実装中の verify gap 検出時は即時 remediation、本 PA-1 entry 直前で literal 適用済)
- `feedback_proactive_handoff` (= Phase 1.A 実装中も context 残量 Claude 側で能動監視、PA-X 単位境界で handoff doc 起案候補)
- `feedback_no_auto_commit` (= AYA 明示指示後のみ commit、PA-2 完了で確認待ち)
- `feedback_remove_verification_logs` (= PA-2 中の検証 log は commit 前除去)
- `feedback_build_only_verified` (= 効果未確認の commit 積まない、PA-2 起動契約は空入力で exit 0 を必ず確認)
- `project_ayastorm_r41_vulkan_migration` (= r41 章 active pointer)
- `project_ayastorm_r41_design_principles` (= upstream 取込容易性 + core 並列化容易性の 2 大設計原則、Phase 1.A 実装の前提)
- `feedback_ubo_migration_one_at_a_time` (= UBO 化作業は 1 つずつ、Phase 2 以降で重要度 ↑)
- `project_build_procedure` (= AYAstorm Linux build flow、PA-X 実装後の build 検証で使用)
- `feedback_use_agents_proactively` (= 重い trace + 複数 grep 連鎖 + 3 ファイル以上の確認は Agent、本 PA-1 で Agent Explore 適用で gap 検出に成功)

---

## §6 次 session 着手 1 line (= PA-2 実装 session 入り時)

「前 session で η-30 Phase 1.A PA-1 完了 = spirv-cross 取込 (= `indra/cmake/SpirvCross.cmake` 起案 + system install verify + version drift doc 更新) + (B2) B2b system pkg 確定 + PA-1 真 scope 訂正 (= ST-7 sub-task 6 verdict 訂正、glslang/Python は既存取込済 scope 外)、commit `3a4b38991a` で着地。本 session = **η-30 Phase 1.A PA-2 = Codegen Python script base 構造起草** = `scripts/ubo_codegen/main.py` 新規 directory + 新規 file 作成、entry point + arg parse + I/O 契約 + log 出力 + 失敗 exit code 配備、`--input <blueprint_path> --output <header_dir>` 起動可 + 空入力で正常終了 (= exit 0)。依存 = 08 §5 + §6 + §7 + 04 §10 (B1) Python 3.8+。PA-2 は Claude 自走可 (= AYA 判断不要、入力契約 + 起動 syntax + Python 3.8+ 要件は確定済)。PA-2 完了後は PA-3 + PA-4 並列実施可 (= entry handoff §3 注)。各 PA-X 単位で `feedback_build_only_verified` + `feedback_no_claude_coauthor` + `feedback_no_auto_commit` 準拠、handoff doc は PA-X 単位境界で起案候補」
