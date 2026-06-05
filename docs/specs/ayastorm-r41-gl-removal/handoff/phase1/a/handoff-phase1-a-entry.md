# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A entry (= Codegen pipeline 実装 entry handoff、η-29 設計 phase 完了 = Stage 3 14 項目 self-check 13/14 ✅ + 残 1 = 3-14 (Q-NTTP) AYA 判断本体)

**作成**: 2026-06-03
**前 handoff**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-29-stage3-sub-task1-4-complete.md` (= η-29 Stage 3 sub-task 1+2+3+4 完了 = 8/14 ✅ + (Q-NTTP) gap remediation、sub-task 5/6/7 は本 session で消化)
**branch**: `feature/ayastorm-r41-gl-removal`
**最新 commit (本 handoff 時点)**: 本 handoff doc + §14.5 row 3-12/3-13 inline ✅ mark + §14.4 ST-7 sub-task 7 paragraph 同 batch commit 予定
**本 handoff の position**: 09 §14.5 row 3-13 (Phase 1.A handoff doc 起案) の物理出力本体 = **本 file の存在 = row 3-13 ✅ verdict の根拠**

---

## §0 state 一行 summary

η-30 **Phase 1.A 実装 entry state** (= 09 §14.5 Stage 3 14 項目 self-check **14/14 ✅ + AYA (Q-NTTP)=A 確定** = 設計 phase 完了 = `feedback_design_phase_no_code_write` 完全解禁 = `indra/` 改変自由化)。**本 session 着手地点 = PA-1 (= spirv-cross 取込)**。

**🔴 重要 update (= 2026-06-03 PA-1 entry 直前 self-verify gap remediation)**: ST-7 sub-task 6 verdict (= row 3-11) は `autobuild.xml` grep のみで「3 dependency 0 件 = 全件追加必要」と判定したが、PA-1 entry 直前で `indra/cmake/Glslang.cmake` (= `find_package(glslang CONFIG REQUIRED)` + `glslang-15.1.0/` vendored + Ubuntu 24.04 `apt install glslang-dev` (15.1.0-2) 経路) + `indra/cmake/Python.cmake` (= `find_package(Python3 COMPONENTS Interpreter)`) 既存取込確認 = autobuild.xml grep のみの片側検証 gap 検出 = `feedback_doubt_self_first` 適用、AYA「A」応答で **PA-1 真 scope = spirv-cross のみ取込** (= `indra/cmake/SpirvCross.cmake` 起案 + system install + `find_package(spirv_cross_c_shared CONFIG REQUIRED)` = Glslang.cmake と同 pattern = Linux first-class baseline (r41 charter §1)、Win/Mac 3 OS bundle は r42-α/β 時に判断 (charter §7.5)) に scope 訂正済。本 PA-1 entry doc は本 update 内容で chapter 08 §5.4.1.5 + chapter 09 §14.4 / §14.5 + chapter 10 §1.0 / §1.2 (B2) の連動 update も実施 (= 4 message 分割 sequential 進行)。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。η-30 Phase 1.A 実装 session 入りでは **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read (offset/limit) する。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-entry.md` | 全文 | 本 handoff (= Phase 1.A 実装 phase entry 全体像 + sub-task PA-1〜PA-8 構成 + Exit Criteria + 紐付け持越項目) |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §4 (= Phase 1: codegen + redirect 層整備 全体 = §4.1 sub-Phase 構成 + §4.2 Phase 1 Exit Criteria + §4.3 紐付け持越項目) + §14.5 (= Stage 3 14 項目 self-check 全件 ✅ verdict 確認 = AYA 承認後の状態) | Phase 1.A scope + Exit Criteria + Phase 1.B/1.C 後続関係把握 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` | §0-§17 全章 (= Codegen pipeline 設計本体、Phase 1.A 実装 source of truth) | Codegen Python tool 起草の設計書、各 sub-task PA-1〜PA-8 の実装規模見積 + algorithm 詳細 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` | §4.3.1 std140 calculator algorithm 8 sub-subsection (= PA-4 実装) + §5.6 perfect hash CHD algorithm 7 sub-subsection (= PA-5 実装) + §6.4 name-based dispatch 7 sub-subsection (= PA-2/PA-3/PA-5 連動) |
| `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` | §5.4.1 SPIR-V reflection 二重保証 8 sub-subsection (= PA-4 実装) + §5.2.1 mini-parser 8 sub-subsection (= PA-3 実装) + §11.5 増分 build cache (= PA-6 実装) + §12.5 CMake DEPENDS + 手動 target (= PA-7 実装) + §13.5 3 OS binary identical 保証 |
| `docs/specs/ayastorm-r41-gl-removal/design/02-naming-convention.md` | §2.4 Codegen-UBO 生成識別子 4 種 (= `<Block>_<Member>_OFFSET` / `<Block>Layout` / `<Block>_SIZE` / `ubo_layout_<blockname>.inl`) = PA-4/PA-5 出力契約 |
| `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md` | §3.1 maxUniformBufferOffsetAlignment 256 + §7.3 ring buffer offset alignment + §11 「padding alignment 256 B 出力契約」 = PA-4 std140 calculator padding 規律 |
| `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md` | §3.1-§3.4 = 85 UBO blueprint inventory (= set=0 3 + set=1 2 + set=2 26 + set=3 54)、PA-8 入力 (= codegen 実行対象) |
| `autobuild.xml` | 既存 dependency entry (= `SDL2` / `gstreamer10` 等) と同形式で glslang / spirv-cross / Python entry 追加箇所 = PA-1 編集 target |
| `indra/cmake/` | CMake DEPENDS + `codegen_ubo_force` 手動 target 追加箇所 (= PA-7 編集 target) |
| `docs/specs/ayastorm-r41-gl-removal/design/10-open-questions.md` | §1.3 (Q-NTTP) 行 = AYA 判断 verdict 確認 (= default A R1 不採用 継続なら C++17 維持で Phase 1.A 着手、B/C 採用なら C++20 切替 cost 先行 task として PA-0 化) |

---

## §2 Phase 1.A scope + Exit Criteria (= 09 §4.1 / §4.2 inheritance)

### §2.1 Phase 1.A scope (= 09 §4.1 から literal 継承)

**Codegen pipeline 実装** = Python script 起草 + glslang 統合 + std140 calculator + SPIR-V reflection 二重保証 + perfect hash + cache + CMake DEPENDS。

該当 chapter = **08 全章** (= source of truth)。

### §2.2 Phase 1.A Exit Criteria (= 09 §4.2 から literal 継承)

**codegen script が既存 85 UBO blueprint を入力に取り、`ubo_metadata.inl` + `ubo_host_loader.inl` を生成、build error 0、生成 header の名前解決 lookup が compile-time 衝突 0**。

補足:
- 既存 program 1 個で生成 header を include + bind 不変動作確認 (= 09 §4.2 Phase 1.A Exit 補足)
- 既存 program 動作 unchanged = Vulkan path 分岐 ON でも OpenGL path 経路を選ぶ default 動作 (= 09 §4.2 注 = Phase 1.A 完了時点では実 Vulkan 描画は始まらない、第 1 UBO migration は Phase 2 へ)

---

## §3 Phase 1.A sub-task 構成 (= PA-1 〜 PA-8、Phase 1.A 実装 session が消化)

| sub-task | scope | 依存 chapter | 出力 / 完了条件 |
|---|---|---|---|
| **PA-1** | **spirv-cross 取込** (= **真 scope** = 2026-06-03 PA-1 entry 直前 gap remediation 結果) = `indra/cmake/SpirvCross.cmake` 起案 + system install + `find_package(spirv_cross_c_shared CONFIG REQUIRED)` (= Glslang.cmake と同 pattern = Linux first-class baseline、Win/Mac 3 OS bundle は r42-α/β 着手時に判断)。**glslang / Python は既存取込済 = PA-1 scope 外**: glslang = `indra/cmake/Glslang.cmake` で `find_package(glslang CONFIG REQUIRED)` + Ubuntu 24.04 `apt install glslang-dev` (15.1.0-2) + `glslang-15.1.0/` vendored、Python = `indra/cmake/Python.cmake` で `find_package(Python3 COMPONENTS Interpreter)` = autobuild bundle 経路使わず | 08 §5.4.1.5 format version pin paragraph (= post-completion correction 後 = glslang B2b 確定 + spirv-cross 同 pattern 拡張) + `indra/cmake/Glslang.cmake` 既存 pattern 参照 + chapter 10 §1.2 (B2) verdict ✅ B2b system pkg | `SpirvCross.cmake` 物理存在 + `find_package(spirv_cross_c_shared CONFIG REQUIRED)` で Linux build PASS + Codegen tool 経路で spirv-cross CLI / C API 経路選定 (= 後段 PA-4 で確定、本 PA-1 では取込のみ)。**autobuild.xml 編集は本 PA-1 では不要** (= glslang B2b system pkg 確定 + Python find_package(Python3) 既存取込済 + spirv-cross も同 system pkg 経路で取込、Win/Mac 3 OS bundle は r42-α/β 時に再検討) |
| **PA-2** | Codegen Python script base 構造 (= entry point + arg parse + I/O 契約 + log 出力 + 失敗 exit code) | 08 §5 + §6 + §7 + 04 §10 (B1) Python 3.8+ | `scripts/ubo_codegen/main.py` 起草、`--input <blueprint_path> --output <header_dir>` で起動可能、空入力で正常終了 |
| **PA-3** | GLSL mini-parser + glslang -E 前処理 (= UBO block 抽出 + member 列挙 + 型解決) | 08 §5.2.1 mini-parser 8 sub-subsection + 08 §17 (P) | mini-parser module 起草、85 UBO blueprint 入力に対し全 block / member 列挙 PASS、type token grammar EBNF 準拠 |
| **PA-4** | std140 offset calculator + SPIR-V reflection 二重保証 mechanism | 04 §4.3.1 calculator 8 sub-subsection + 08 §5.4.1 reflection 8 sub-subsection + 08 §17 (A1) | std140 calculator module 起草、各 UBO の offset / SIZE / 256B padding 出力、glslang 経由 SPIR-V reflection 結果と全 member 照合 = mismatch 時 build error 出力 |
| **PA-5** | perfect hash CHD Python frozen-table generator (= name → offset dispatch table) | 04 §5.6 CHD algorithm 7 sub-subsection + 08 §17 (G/B3) | perfect hash module 起草、UBO 全 member name に対し 2 段 hash + displacement table 構築、衝突 0 invariant + C++ 形式出力 |
| **PA-6** | 増分 build cache hash + mtime 併用 mechanism | 08 §11.5 cache spec + 08 §17 (B4) | cache module 起草、`<cache_dir>/codegen_state.json` で hash + mtime 記録、入力変更時のみ regenerate、glslang / spirv-cross / Python version も cache key 構成材料 |
| **PA-7** | CMake DEPENDS 自動 + 手動 `codegen_ubo_force` target 併設 | 08 §12.5 CMake spec + 08 §17 (B5) | `indra/cmake/00-Common.cmake` 等に CMake function 追加、UBO blueprint 変更で codegen 自動再実行、手動 target で全 force regenerate |
| **PA-8** | 85 UBO blueprint 入力 → 出力 header 生成 + Exit Criteria 充足検証 (= test program 1 個で include + bind 不変確認) | 08 §13.5 binary identical + inventory §3.1-§3.4 | `ubo_metadata.inl` + `ubo_host_loader.inl` + `ubo_perfect_hash.inl` + `ubo_dummy_init.inl` 4 file 生成、既存 program 1 個 (= 後の Phase 1.B/1.C/2 で本格使用予定の同 program) で include + build error 0 + compile-time 名前解決衝突 0 確認 |

**注**: sub-task PA-1 〜 PA-8 の順序は **strict 線形** (= 各 sub-task は前の sub-task の出力に依存)。例 = PA-2 が無いと PA-3 〜 PA-7 は import 不能、PA-4 が無いと PA-5 の出力先 (= name → offset 表) が undefined、PA-7 が無いと PA-8 の cmake build 統合不能。並列実施は **PA-3 と PA-4 のみ可** (= 両者 mini-parser 出力と glslang reflection 出力で独立に進行可、最終照合 phase で合流)。

**✅ (Q-NTTP) = A 確定済 (2026-06-03 ST-7 sub-task 8 batch)**: AYA「A」応答で **R1 不採用 / C++17 維持** = default A 採用継続 = R3 name-based dispatch + perfect hash CHD + frozen-table で十分高速、C++20 切替 cost (= 3 OS toolchain 確認 + dependent module re-validation + autobuild manifest 変更) 回避、R1 は Phase K+4 以降 polish 候補保留可。**PA-0 不要 = PA-1 (= spirv-cross 取込) から即着手**。

---

## §4 紐付け持越項目 (= 09 §4.3 + 08 §17 + autobuild pin)

| ID | 内容 | 解消 sub-task |
|---|---|---|
| 08 §17 (A1) | std140 offset Codegen 独自 calculator + SPIR-V reflection 二重保証 | PA-4 |
| 08 §17 (P) | GLSL parse 独自 mini-parser + glslang -E 前処理 | PA-3 |
| 08 §17 (G/B3) | perfect hash 独自 Python frozen-table | PA-5 |
| 08 §17 (B1) | Codegen Python 3.8+ | PA-2 |
| 08 §17 (B2) | glslang 統合 = **✅ B2b system pkg 確定** (= 実装で先行 commit 済 + `indra/cmake/Glslang.cmake` + `glslang-15.1.0/` vendored + system `apt install glslang-dev` 経路、2026-06-03 PA-1 entry 直前 gap remediation で B2 verdict 確定) | (取込済、scope 外) |
| 08 §17 (B2 spirv-cross extension) | spirv-cross 取込 = Glslang.cmake と同 pattern (= `SpirvCross.cmake` + system install + `find_package(spirv_cross_c_shared CONFIG REQUIRED)`) | PA-1 |
| 08 §17 (B4) | 増分 build cache hash + mtime 併用 | PA-6 |
| 08 §17 (B5) | CMake DEPENDS 自動 + 手動 `codegen_ubo_force` target 併設 | PA-7 |
| autobuild pin | autobuild.xml で glslang / spirv-cross / Python version pin entry 追加 (= 設計 review 2026-06-03 ST-7 sub-task 6 で「Phase 1.A 入口同タイミング実施」確定) | PA-1 |
| Phase 1.A Exit | 85 UBO blueprint で codegen 実行 + build error 0 + compile-time 衝突 0 | PA-8 |

**Phase 1.B / 1.C / 2 への送り出し** (= Phase 1.A scope 外、後続 Phase で消化):
- 06a §3 / §4 / §5 = 30 setter 内 Vulkan path 分岐 + mUniformUBOLoc cache → **Phase 1.B**
- 06b cadence 5 種 update site + dirty flag + 06c descriptor set bind 配線 → **Phase 1.C**
- 07 §12 (W2) `sAssetUboPool` prealloc + 07 §12 (R1) ring buffer + 07 §12 (PSC) PSO cache → **Phase 1.C**

---

## §5 規律 (= Phase 1.A 実装 session 入り時 self-check)

1. **`indra/` 改変解禁状態**: 本 file 作成時点では未解禁 (= `feedback_design_phase_no_code_write` 厳守継続)、Phase 1.A 実装 session 入り = AYA (Q-NTTP) 承認到達後 = **完全解禁**。以降の `indra/cmake/` / `scripts/ubo_codegen/` / `autobuild.xml` 編集は正常範囲
2. **`feedback_build_only_verified` 適用**: PA-1 〜 PA-8 各 sub-task で実機 build + 動作確認後にのみ commit (= 効果未確認 commit を積まない)
3. **`feedback_ubo_migration_one_at_a_time` 適用前準備**: Phase 1.A は migration 前提整備のため UBO 個別 migration は無、ただし PA-2 〜 PA-7 各 module も **1 module 1 commit** を default (= 大塊バッチ禁止と同精神)
4. **`feedback_no_claude_coauthor` 厳守**: Phase 1.A 全 commit で `Co-Authored-By: Claude` 行禁止
5. **`feedback_no_auto_commit` 厳守**: AYA「ok」「commit して」等明示指示後にのみ commit
6. **`feedback_one_step_at_a_time` 厳守**: 1 メッセージ 1 action、複数 sub-task 並列着手しない (= PA-3 + PA-4 並列可能性は AYA 判断後にのみ着手)
7. **`feedback_handoff_minimal_pre_req_read` 厳守**: 本 §1.1 3 件必読 + §1.2 pinpoint Read 切替、全件読み禁止
8. **`feedback_proactive_handoff` 適用**: Phase 1.A 実装中も context 残量 Claude 側で能動監視、PA-X 単位境界で handoff doc (= `...phase1-a-PA-X-complete.md`) 起案候補
9. **`feedback_remove_verification_logs` 適用**: PA-X 実装中の検証用 LL_INFOS hook は commit 前に必ず除去
10. **✅ (Q-NTTP) = A 確定済** = R1 不採用 / C++17 維持 = PA-0 不要、PA-1 (= spirv-cross 取込) から着手済
11. **`feedback_doubt_self_first` 強化適用 (= 2026-06-03 PA-1 entry 直前 gap remediation 教訓)**: PA-1 entry 直前で autobuild.xml grep のみの片側検証 gap 検出 → `indra/cmake/` + `scripts/` 横断 verify (Agent Explore) で実態確認後 AYA 判断仰ぎ = handoff doc pre-req 確認 phase でも本 feedback 適用 literal 教訓 (= ST-7 sub-task 6 verdict 起草時に `indra/cmake/Glslang.cmake` + `indra/cmake/Python.cmake` 確認漏れ = 片側 grep 検証 risk)
12. **`feedback_self_verify_before_handoff` 強化適用**: 設計 phase 内 self-check では「verify pass」「✅」mark を付ける前に **両側検証** (= 対象 file 単独 grep + 関連 file 横断 grep) 必須、片側のみで verdict しない

---

## §6 self-verify (= 本 handoff 起案時点の整合性、9 観点)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) 前 handoff (`...stage3-sub-task1-4-complete.md`) からの遷移整合 | 前 §3.1 「sub-task 5 候補 = 3-1+3-2 batch」 → 本 session で sub-task 5 (3-1+3-2 ✅) + sub-task 6 (3-11 ✅) + sub-task 7 (3-12 + 3-13 = 本 handoff doc 起案) 順次消化 = 13/14 ✅ 到達 | ✅ |
| (2) Phase 1.A scope + Exit Criteria が 09 §4.1 / §4.2 と literal 一致 | 本 §2.1 scope = 09 §4.1 row "1.A" cell 引用 / 本 §2.2 Exit = 09 §4.2 Phase 1.A bullet 引用 = 加工なし | ✅ |
| (3) 紐付け持越項目 7 件 (= 08 §17 (A1)(P)(G/B3)(B1)(B2)(B4)(B5)) + autobuild pin 全件 sub-task 紐付け | 本 §4 表で 7 件 + 1 件 全 PA-X 紐付け確認 | ✅ |
| (4) sub-task PA-1 〜 PA-8 が Phase 1.A Exit Criteria 全件覆う | PA-1 (autobuild) → PA-2 (Python base) → PA-3 (parser) → PA-4 (calculator + reflection) → PA-5 (perfect hash) → PA-6 (cache) → PA-7 (CMake) → PA-8 (85 UBO 実行 + 検証) = Exit Criteria の 4 element (= codegen 実行 / 4 file 生成 / build error 0 / 名前解決衝突 0) 全充足経路明示 | ✅ |
| (5) `feedback_design_phase_no_code_write` 解除 timing 整合 | 本 handoff 作成時点 = 未解禁 (= doc 配下追加のみ) / Phase 1.A 実装 session 入り = AYA (Q-NTTP) 承認後 = 完全解禁、本 §5-1 で明示 | ✅ |
| (6) (Q-NTTP) AYA 判断結果による分岐明示 | 本 §3 末 「(Q-NTTP) B/C 採用時 PA-0 = C++20 切替」paragraph で default A 継続 / B-C 採用の 2 経路を明示 | ✅ |
| (7) pre-requisite 最小読み 3 件 + pinpoint reference 構成 | 本 §1.1 3 件 (= 本 handoff + 09 §4/§14.5 + 08 §0-§17) + §1.2 pinpoint Read 8 file = `feedback_handoff_minimal_pre_req_read` 準拠 | ✅ |
| (8) `feedback_no_claude_coauthor` 明示継続 | 本 §5-4 で Phase 1.A 全 commit 共著行禁止明示 | ✅ |
| (9) Phase 1.B / 1.C / 2 への送り出し項目明示 | 本 §4 末 paragraph で 06a §3-§5 → Phase 1.B / 06b/06c → Phase 1.C / 07 §12 (W2)(R1)(PSC) → Phase 1.C を明示、本 Phase 1.A scope 外 | ✅ |
| (10) PA-1 真 scope correction reflect (= 2026-06-03 PA-1 entry 直前 gap remediation) | §0 + §3 PA-1 cell + §3 (Q-NTTP) paragraph + §4 (B2) 行 + §5 規律 11/12 + §6 row 10 + §7 memory + §8 着手 1 line 全件 update 済、chapter 08 §5.4.1.5 + chapter 09 §14.4 / §14.5 + chapter 10 §1.0 / §1.2 (B2) も連動 update 済 | ✅ |

---

## §7 引き継ぎ済 memory (= Phase 1.A 実装 session でも活きる)

- `feedback_design_phase_no_code_write` (= 解禁 timing は本 handoff §5-1 + 09 §14.5 row 3-12 verification cell に確定済、Phase 1.A 実装 session 入りで解除)
- `feedback_build_only_verified` (= PA-X 各 sub-task 実機 build + 動作確認後 commit)
- `feedback_ubo_migration_one_at_a_time` (= Phase 1.A は前準備、Phase 2 以降の UBO 個別 migration 時に load-bearing)
- `feedback_no_claude_coauthor` (= 全 commit 共著行禁止継続)
- `feedback_no_auto_commit` (= AYA 明示指示後 commit)
- `feedback_one_step_at_a_time` (= 1 メッセージ 1 action、PA-3 + PA-4 並列可能性も AYA 判断後)
- `feedback_handoff_minimal_pre_req_read` (= 本 §1 構造の根拠)
- `feedback_proactive_handoff` (= PA-X 単位境界で handoff doc 起案候補、context 残量 Claude 側監視)
- `feedback_self_verify_before_handoff` (= AYA 確認前に Claude 全 sub-task self-verify、本 handoff も 9 観点 self-verify PASS)
- `feedback_remove_verification_logs` (= 検証用 LL_INFOS hook は commit 前除去)
- `feedback_no_scope_shrink` (= PA-X scope を AYA 確認なしで縮小しない)
- `feedback_doubt_self_first` (= Phase 1.A 実装中 + **handoff pre-req 確認 phase 中** の verify gap 検出時は即時 remediation、本 PA-1 entry 直前 gap remediation で literal 適用済 = autobuild.xml grep のみの片側検証 → `indra/cmake/` + `scripts/` 横断 verify で実態確認 → AYA「A」応答で真 scope = spirv-cross のみ取込 に scope 訂正)
- `project_ayastorm_r41_vulkan_migration` (= r41 章 active pointer)
- `project_ayastorm_r41_design_principles` (= upstream 取込容易性 + core 並列化容易性の 2 大設計原則、Phase 1.A 実装の前提)
- `project_build_procedure` (= AYAstorm Linux build flow、PA-X 実装後の build 検証で使用)
- `feedback_use_agents_proactively` (= Phase 1.A 実装中の複数 grep 連鎖 / 3 file 以上の確認は Agent 使用)
- `feedback_admit_unknown` (= Phase 1.A 実装中の仮説 2 連続外れたら推論止めて log / canary / bisect で実データ取得に切替)

---

## §8 次 session 着手 1 line (= Phase 1.A 実装 session 入り時)

「前 session で η-29 設計 phase 全完走 = 09 §14.5 Stage 3 14 項目 self-check **全 14 ✅ + AYA (Q-NTTP)=A 確定** = `feedback_design_phase_no_code_write` 完全解禁 = `indra/` 改変自由化。本 session = **η-30 Phase 1.A 実装 entry = Codegen pipeline 実装** (= 09 §4.1 sub-Phase 1.A scope literal 継承)。**🔴 重要 update (= 2026-06-03 PA-1 entry 直前 gap remediation)**: ST-7 sub-task 6 verdict は `autobuild.xml` grep のみで「3 dependency 全件追加必要」と判定したが、PA-1 entry 直前で `indra/cmake/Glslang.cmake` + `indra/cmake/Python.cmake` 既存取込確認 = `feedback_doubt_self_first` 適用、AYA「A」応答で **PA-1 真 scope = spirv-cross のみ取込** (= `indra/cmake/SpirvCross.cmake` 起案 + system install + `find_package(spirv_cross_c_shared CONFIG REQUIRED)`) に訂正済。即着手対象 = 本 handoff §3 sub-task 構成表に従い、**PA-1 (= spirv-cross 取込、autobuild.xml 編集不要、Glslang.cmake と同 pattern)** → PA-2 (= Codegen Python script base) → PA-3 (= GLSL mini-parser + glslang -E) → PA-4 (= std140 calculator + SPIR-V reflection 二重保証) → PA-5 (= perfect hash CHD frozen-table) → PA-6 (= 増分 build cache) → PA-7 (= CMake DEPENDS + 手動 target) → PA-8 (= 85 UBO blueprint 実行 + Exit Criteria 充足検証) の strict 線形順序 (PA-3 + PA-4 のみ並列可)。各 PA-X 単位で `feedback_build_only_verified` + `feedback_no_claude_coauthor` + `feedback_no_auto_commit` 準拠、handoff doc は PA-X 単位境界で起案候補 (= `...phase1-a-PA-X-complete.md` 形式)。Phase 1.A Exit Criteria = `ubo_metadata.inl` + `ubo_host_loader.inl` + `ubo_perfect_hash.inl` + `ubo_dummy_init.inl` 4 file 生成 + 85 UBO blueprint で build error 0 + 名前解決 compile-time 衝突 0 + 既存 program 1 個 include + bind 不変動作確認 = 達成で Phase 1.B (= 30 setter redirect 層) entry へ移行。Phase 1.A 完了時点でも実 Vulkan 描画は始まらない (= 第 1 UBO migration は Phase 2 で AYA (Q1) 判断後)」
