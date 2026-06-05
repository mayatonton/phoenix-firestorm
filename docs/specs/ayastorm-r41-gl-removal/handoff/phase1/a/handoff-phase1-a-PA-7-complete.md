# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-7 完了

**作成日**: 2026-06-04
**前 session commit**: `eaa6848700` (= PA-6 完了 handoff doc 起案、本 session entry 時点)
**本 session 物理出力**: `indra/cmake/AyaUboCodegen.cmake` 新規 + `indra/llrender/CMakeLists.txt` MOD + `scripts/ubo_codegen/glsl_parser.py` MOD + `scripts/ubo_codegen/tests/test_glsl_parser.py` MOD + 本 handoff doc 新規 (= 全て **未 commit**、AYA さん明示指示後 batch commit)
**次 session 着手**: PA-7.5 (= `aya_r41_exemplar/sky_placeholder{V,F}.glsl` への `std140` 補完 + 実 indra/ tree end-to-end PASS 化) または PA-8 (= 85 UBO blueprint 起案 + Exit Criteria 09 §4.2 充足検証)

---

## §0 state 一行 summary

η-30 **Phase 1.A PA-7 完了 state** (= `indra/cmake/AyaUboCodegen.cmake` 新規 = §12.3 CMake DEPENDS 自動 + §12.5.2 CONFIGURE_DEPENDS で新規 GLSL 自動検出 + §12.5.4 `codegen_ubo_force` 手動 target + spirv-cross 不在時 `AYA_CODEGEN_SKIP_SPIRV_CHECK=1` 自動 env / `indra/llrender/CMakeLists.txt` に `include(AyaUboCodegen)` + `aya_attach_ubo_codegen(llrender)` 追加 = llvkloader.cpp は llrender 内 TU ゆえ 1 attach で §12.5.3 llrender + llvkloader 両 satisfy / `glsl_parser.py` の `layout(...)` を UBO 専用前提から `uniform IDENT LBRACE` pattern check に修正 = `layout(location=N) out/in/sampler` を skip / unittest 125/125 + py_compile 8/8 + 独立 cmake smoke 3 path PASS = miss 58ms / hit touch only / --force 62ms / 6 file emit)。**次 session 着手地点 = PA-7.5 (= `aya_r41_exemplar/*.glsl` の UBO に `std140` qualifier 補完、実 indra/ tree で end-to-end PASS 確認)** または直接 **PA-8** (= 85 UBO blueprint authoring + Exit Criteria 充足検証 = 09 §4.2)。

PA-7 完了で残 PA は **strict 線形** = (PA-7.5 micro-fix) → PA-8 最終。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。PA-7.5 or PA-8 着手 session では **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read (offset/limit) する。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-PA-7-complete.md` | 全文 | 本 handoff (= PA-7 完了 state + PA-7.5 / PA-8 着手地点 + cmake module 仕様 + parser 修正範囲) |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-entry.md` | §3 PA-X 全体構成表 + §2 Phase 1.A scope + Exit Criteria | Phase 1.A 全体像 + PA-8 sub-task scope literal 参照 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/09-misc-decisions.md` | §4.2 Phase 1.A Exit Criteria 全文 | PA-8 = 85 UBO blueprint 実行 + Exit Criteria 充足検証 の判定基準 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` | §3.1 (= std140 strict 順守判断 A / PA-7.5 で exemplar 改修するか parser を緩めるかの根拠) + §5.1 (= 4 file 分割契約) |
| `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` | §12.3 (= snippet 例、本 PA-7 実装と CLI flag 差 = `--input`/`--output` vs `--input-glsl-dir`/`--output-dir` を意識) + §12.5.2 / §12.5.3 / §12.5.4 (= CONFIGURE_DEPENDS / add_dependencies / force target 実装根拠) + §13.5 (= 3 OS binary identical) |
| `indra/cmake/AyaUboCodegen.cmake` | 本 PA-7 起案物 = `aya_attach_ubo_codegen(<target>)` helper + `AYA_UBO_CODEGEN_*` cache var 列 |
| `indra/llrender/CMakeLists.txt` | 本 PA-7 改変 = `include(AyaUboCodegen)` 位置 (line 10) + `aya_attach_ubo_codegen(llrender)` 位置 (`AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK` block 直後) |
| `scripts/ubo_codegen/glsl_parser.py` | 本 PA-7 改変 = `parse_glsl()` 内 `layout` token 分岐 (= peek で `uniform IDENT LBRACE` pattern 検査、非 UBO は semi まで skip) |
| `scripts/ubo_codegen/tests/test_glsl_parser.py` | 本 PA-7 追加 3 test (= `test_non_ubo_layout_skipped_out` / `_in` / `_sampler`) |
| `indra/newview/app_settings/shaders/aya_r41_exemplar/sky_placeholder{V,F}.glsl` | PA-7.5 改修対象 = `layout(set=0, binding=N) uniform <Name> { ... };` の `set=` 前に `std140,` 挿入 (= 04 §3.1 strict) |

---

## §2 本 session 成果 (= PA-7 完了 内訳)

**物理出力**:
- `indra/cmake/AyaUboCodegen.cmake` 新規 (= 約 160 line、§12.3 + §12.5.2/3/4 + spirv-cross fallback env)
- `indra/llrender/CMakeLists.txt` MOD (= `include(AyaUboCodegen)` 1 行 + `aya_attach_ubo_codegen(llrender)` 1 行 + コメント 4 行)
- `scripts/ubo_codegen/glsl_parser.py` MOD (= `parse_glsl()` 内 `layout` 分岐に peek pattern check 24 行追加)
- `scripts/ubo_codegen/tests/test_glsl_parser.py` MOD (= 3 test = 26 行追加)
- 本 handoff doc 新規

**commit 状態**: **未 commit** (= `feedback_no_auto_commit` 準拠、AYA さん明示指示後に handoff doc + 4 file 改変を batch commit)。

### §2.1 PA-7 真 scope 達成 (= 08 §12 + entry handoff §3 PA-7 row literal)

| sub-section / 出力契約 | 実装箇所 | 内容 |
|---|---|---|
| 08 §12.3 add_custom_command 起動 | `AyaUboCodegen.cmake` `add_custom_command(OUTPUT ${AYA_UBO_CODEGEN_OUTPUTS} ...)` | 5 aggregated file (= `ubo_index.inl` / `ubo_perfect_hash.inl` / `ubo_metadata.inl` / `ubo_dummy_init.inl` / `ubo_host_loader.inl`) を OUTPUT に列挙、DEPENDS = GLSL files + Python module files + `main.py` |
| 08 §12.3 codegen_ubo target | `AyaUboCodegen.cmake` `add_custom_target(codegen_ubo DEPENDS ...)` | leaf target = 利用 target から `add_dependencies(<target> codegen_ubo)` で order 保証 |
| 08 §12.5.2 CONFIGURE_DEPENDS | `AyaUboCodegen.cmake` `file(GLOB_RECURSE ... CONFIGURE_DEPENDS ...)` | 新規 GLSL ファイル追加時 build 開始時 cmake が自動 reconfigure (= 開発者運用ルール不要) |
| 08 §12.5.3 add_dependencies + target_include_directories | `AyaUboCodegen.cmake` `aya_attach_ubo_codegen(<target>)` 関数 | helper 関数化 = `add_dependencies(<target> codegen_ubo)` + `target_include_directories(<target> PUBLIC ${AYA_UBO_CODEGEN_INCLUDE_DIR})` を一括設定。`indra/llrender/CMakeLists.txt` で `aya_attach_ubo_codegen(llrender)` で配線 (= llvkloader.cpp は llrender 内 TU で同時 satisfy、§12.5.3 llrender + llvkloader 列記を 1 attach で履行) |
| 08 §12.5.4 手動 force target | `AyaUboCodegen.cmake` `add_custom_target(codegen_ubo_force COMMAND ... --force ...)` | `cmake --build build/ --target codegen_ubo_force` で cache 完全 invalidate + 全 .inl 再生成 |
| 08 §12.5.7 cache hit 時 touch | `main.py` 既存 `touch_outputs()` (= PA-6 起案) + `add_custom_command` OUTPUT mtime 更新 | cache hit path で全 OUTPUT mtime bump → CMake 次回 DEPENDS 解消で codegen tool 起動 skip |
| 08 §13.5 / 09 cmake module 整合 | `AyaUboCodegen.cmake` `include(Python)` 経由の `${PYTHON_EXECUTABLE}` | 3 OS 統一 Python 解決 (= Linux find_program `python3` / Win `Python3` package / Mac autobuild bundle) |
| **PA-7 副次** glsl_parser robustness | `glsl_parser.py` `parse_glsl()` `layout` 分岐 | `layout(location=N) out/in/sampler` 等 非 UBO declaration を semi まで skip (= 250 .glsl recursive walk で UBO 外宣言が parse_glsl を fail させない) |

### §2.2 設計との微差分 (= 実装上の制約による小修正、必ず引き継ぐ)

| # | spec 記述 | 実装変更 | 理由 |
|---|---|---|---|
| 1 | 08 §12.3 snippet の CLI flag = `--input-glsl-dir` / `--output-dir` | 実装は `--input` / `--output` (= PA-6 main.py 既存) | PA-6 起案時 main.py CLI を short 名で揃えた歴史を継承、spec snippet は例示と解釈、機能は完全一致 |
| 2 | 08 §12.3 snippet は `find_package(Python3 3.8 REQUIRED)` 直書き | 実装は `include(Python)` で `${PYTHON_EXECUTABLE}` 経由 | AYAstorm 既存 cmake module の Python.cmake (= autobuild python bundle 含む) を再利用、3 OS 統一解決 |
| 3 | 08 §12.5 spirv-cross 必須前提 | 実装は spirv-cross 不在時 `AYA_CODEGEN_SKIP_SPIRV_CHECK=1` 環境変数を `cmake -E env` で自動付与 | dev 環境で spirv-cross install されていなくても codegen が走る (= Linux/Win/Mac install instruction 未整備 phase での暫定許容、Phase 1.A の Codegen tool 既存 fallback path を活用) |
| 4 | 08 §12.3 snippet は `add_dependencies(llrender codegen_ubo)` を直接記述 | 実装は `aya_attach_ubo_codegen(<target>)` 関数経由 | call site 1 行で完結 + 将来他 target 追加 (= llui 等) が容易、include path 露出も同時 |

4 件とも 08 §12 の **思想は維持**、実装上の利便性 + 既存 module 再利用での物理選択。spec doc cross-ref 追加判断は PA-8 完了後の総まとめで AYA さんに伺う。

### §2.3 副次 glsl_parser 修正 (= PA-7 verification 中に判明した parser bug)

**症状**: 実 indra/ tree (= 250 .glsl recursive) で codegen_ubo 起動時、`layout(location = 0) out vec4 outColor;` (= 通常 fragment shader 宣言) を UBO block と誤認 → `unexpected token (expected IDENT 'uniform', got IDENT:'out')` で fail。

**原因**: `glsl_parser.py:parse_glsl()` が `layout` token を見たら無条件で `_parse_ubo_block()` を呼ぶ実装、UBO 外の `layout(location/binding=N) in/out/uniform sampler...` を許容しない。

**修正**: `layout` token を見たら **peek** で括弧バランス追って `layout(...)` の直後を確認、`uniform IDENT LBRACE` 3 token pattern にマッチした時のみ `_parse_ubo_block()` 起動、それ以外は `_skip_to_semi()` で declaration 1 文を読み捨て。

**回帰確認**: 既存 122 test + 新規 3 test = 125 test PASS、`MIXED` GLSL 内の `uniform sampler2D diffuseMap;` (= bare `uniform` プレフィックス無し) は従来通り `_parse_uniform_or_sampler()` 経由で `sampler_decls` に記録。

**未対応 (= 既知制約)**: `layout(binding=N) uniform sampler2D tex;` (= layout 修飾付き sampler) は今回 skip 経路で **silently 無視** = `sampler_decls` に記録されない。PA-7 scope では UBO 優先で許容、Phase 1.B+ で sampler tracking が必要になった時点で skip path に sampler 検出を追加。

### §2.4 unittest 125 test 内訳 (= 既存 122 + 新規 3)

- 既存 122 test = HashingTests 3 + NormalizePathTests 2 + EnvBlockTests 2 + StateIOTests 4 + CheckCacheTests 17 + TouchOutputsTests 2 + HashHelperTests 7 + CHDConstructionTests 11 + VerifyCHDTests 3 + CollectUniformKeysTests 1 + EmitTests 10 + EmitDummyInitTests 4 + EmitHostLoaderTests 5 + CountPerSetBindingsTests 2 + EmitIndexAggregateTests 2 + GLSL parser tests 10 + std140 tests 14 + SPIR-V reflect tests 19 + canonical_output_files updated 2 = 累計 122 → 125 (= 全件継続 PASS)
- 新規 3 test = `GlslParserTests`:
  - `test_non_ubo_layout_skipped_out`: `layout(location = 0) out vec4 outColor;` + UBO `U` の 2 件で UBO 1 のみ抽出
  - `test_non_ubo_layout_skipped_in`: `layout(location = 1) in vec3 inNormal;` + UBO `V` の 2 件で UBO 1 のみ抽出
  - `test_non_ubo_layout_skipped_sampler`: `layout(binding = 5) uniform sampler2D tex;` + UBO `W` の 2 件で UBO 1 のみ抽出

### §2.5 end-to-end smoke 3 path (= 独立 cmake project /tmp/aya_ubo_pa7_smoke/)

| Run | command | 結果 |
|---|---|---|
| 1 (cache miss) | `cmake --build build --target codegen_ubo` | `[codegen_ubo] INFO: cache miss: cache state missing or corrupt` + 6 file emit (= layout/index/metadata/perfect_hash/dummy_init/host_loader) + 58ms |
| 2 (cache hit) | 同上 (連続実行) | `[100%] Built target codegen_ubo` (= make/cmake 上で再 build skip、Python tool 起動なし) |
| 3 (--force) | `cmake --build build --target codegen_ubo_force` | `[codegen_ubo] INFO: --force given, bypassing incremental cache` + 6 file 再 emit + 62ms |

独立 project = 最小有効 UBO 1 件 (= `frame_viewproj.glsl` に `layout(std140, set=0, binding=0) uniform FrameViewProj { mat4 view; mat4 proj; };`) で wiring 単体の end-to-end が PASS することを確認。

### §2.6 実 indra/ tree (= 250 .glsl recursive) 現状

| 段階 | 結果 |
|---|---|
| CMake configure | ✅ PASS (= 250 .glsl 検出 + codegen_ubo + codegen_ubo_force 両 target 作成) |
| codegen_ubo 起動 | ✅ Python tool 起動 + 250 file discover ログ確認 |
| 全 .glsl parse | ❌ `aya_r41_exemplar/sky_placeholderV.glsl:18` の `PerFrameMatrixUBO` が `layout(set=0, binding=0) uniform PerFrameMatrixUBO { ... };` (= `std140` qualifier 不在) で 04 §3.1 strict 判定 A により reject |

**判定**: PA-7 = CMake wiring 自体は ✅ PASS。実 indra/ tree blocker は **PA-7.5 (= exemplar 改修) または PA-8 (= 85 UBO blueprint 起案で全 UBO `std140` 順守)** の scope。CMake wiring 部分の verification は独立 smoke で確証済み。

---

## §3 次着手 PA-7.5 or PA-8

### §3.1 着手 1 line

**PA-7.5** (= `aya_r41_exemplar/sky_placeholder{V,F}.glsl` 内 UBO 宣言 4 件 に `std140` 挿入 + 実 indra/ tree で end-to-end 3 path PASS 化 + 4 file 検証 出力 sanity check) **または** **PA-8** (= 85 UBO blueprint 起案 = chapter 09 §4.2 + Exit Criteria 充足検証 + 全 path end-to-end PASS 化)。

### §3.2 PA-7.5 scope (= micro-fix、推奨)

| 出力 | 内容 |
|---|---|
| 改修 file | `indra/newview/app_settings/shaders/aya_r41_exemplar/sky_placeholderV.glsl` (= 3 UBO 宣言) + `sky_placeholderF.glsl` (= 改修不要、UBO 宣言なし、確認のみ) |
| 改修内容 | `layout(set = 0, binding = N) uniform <Name>` → `layout(std140, set = 0, binding = N) uniform <Name>` (= 既存 `set` qualifier の前に `std140,` を挿入、3 件) |
| 完了条件 | 実 indra/ tree (= 250 .glsl) で `cmake --build build --target codegen_ubo` PASS + 6 file emit + cache hit/miss/force 3 path PASS + 出力 sanity (= `ubo_index.inl` に exemplar 3 block 含まれる) |
| 工数見積 | 約 15 min (= sed 一発 + cmake build 確認 + git diff 確認) |

**自走可 / AYA 判断不要**: 04 §3.1 判断 A literal 「std140 layout のみサポート」に従う直訳改修。

### §3.3 PA-8 scope (= strict 線形最終 PA)

| 出力 | 内容 |
|---|---|
| 出力 | 85 UBO blueprint 全 `.glsl` 起案 (= chapter 09 §4.2 Exit Criteria 列挙) + 実 indra/ tree で end-to-end PASS + 4 file 検証 sanity (= `ubo_metadata.inl` 85 entry / `ubo_perfect_hash.inl` 衝突 0 / `ubo_host_loader.inl` mUniformUBOLoc prefill / `ubo_dummy_init.inl` stub g_program_count=0) |
| 依存 chapter | 04 §3 (= UBO 仕様) + 09 §4.2 (= Exit Criteria) + 02 §2.1 (= cadence_tag prefix 命名) + 06a inventory (= 85 UBO 一覧 = blueprint authoring の master source) |
| 入力 | 既存 GLSL shader 内の uniform を 85 UBO 単位に再構成 (= AYA 判断 + 既存 shader 改修なし、blueprint 配置のみ) |
| 完了条件 | (1) 85 UBO blueprint 全件 `.glsl` 配置 / (2) `codegen_ubo` PASS = 6 file emit / (3) cache 3 path PASS / (4) 出力 4 file 検証 (= block count = 85 / hash 衝突 0 / set 振分 (= set=0/1a/1b/2/3) 整合 / dummy_init stub) / (5) 09 §4.2 Exit Criteria 全項 ✅ |
| 工数見積 | blueprint 起案 + 検証で約 2-3 session 想定 (= 85 UBO 単位の authoring が支配) |

### §3.4 strict 線形

PA-7.5 micro-fix → PA-8 最終。**PA-8 完了で Phase 1.A 全体終了**、次は Phase 1.B 着手 (= chapter 09 phase roadmap 後続)。

### §3.5 PA-7.5 / PA-8 共通 self-verify 必須 4 項目

1. `cmake --build <build> --target codegen_ubo` 実 indra/ tree (= 250 .glsl) で PASS = 6 file emit
2. cache hit 経路 PASS = 連続 build で codegen tool 起動なし (= `[100%] Built target codegen_ubo` のみ)
3. `cmake --build <build> --target codegen_ubo_force` PASS = --force で cache bypass + 6 file 再 emit
4. 出力 4 file (= `ubo_metadata.inl` / `ubo_perfect_hash.inl` / `ubo_dummy_init.inl` / `ubo_host_loader.inl`) の C++17 standalone compile sanity (= `g++ -std=c++17 -Wall -Wextra -c` で warning なし)

---

## §4 self-verify 9 観点 PASS

| # | 観点 | 結果 |
|---|---|---|
| 1 | `indra/cmake/AyaUboCodegen.cmake` 物理存在 + 約 160 line | ✅ |
| 2 | `indra/llrender/CMakeLists.txt` MOD 2 箇所 (= include + attach) | ✅ |
| 3 | `scripts/ubo_codegen/glsl_parser.py` MOD = layout peek pattern 追加 | ✅ |
| 4 | py_compile 8/8 PASS (= Python 3.12.3 syntax check) | ✅ |
| 5 | unittest 125/125 PASS (= 122 + 3 new) | ✅ |
| 6 | 独立 cmake smoke 3 path PASS (= miss 58ms / hit / --force 62ms / 6 file emit) | ✅ |
| 7 | 実 indra/ tree CMake configure PASS (= 250 .glsl 検出) + codegen tool 起動確認 (= PA-7.5 blocker 判明) | ✅ (= wiring scope PASS、blueprint scope は PA-8) |
| 8 | git working tree = scripts + indra (cmake/llrender) のみ改変、`Co-Authored-By: Claude` 共著行不在 (= `feedback_no_claude_coauthor`) | ✅ |
| 9 | handoff doc 命名対称 (= PA-6-complete に対し PA-7-complete) | ✅ |

---

## §5 引き継ぎ済 memory 18 件 (= PA-7.5 / PA-8 entry session で重要度 ↑)

`feedback_handoff_minimal_pre_req_read` + `feedback_design_phase_no_code_write` (= 解禁後、`indra/cmake/` + `indra/llrender/` cmake 配下のみ改変、shader source は PA-8 scope で慎重に) + `feedback_no_scope_shrink` + `feedback_self_verify_before_handoff` + `feedback_no_claude_coauthor` + `feedback_one_step_at_a_time` + `feedback_doubt_self_first` + `feedback_proactive_handoff` + `feedback_no_auto_commit` + `feedback_remove_verification_logs` + `feedback_build_only_verified` + `feedback_tests_dir_never_commit` + `project_ayastorm_r41_vulkan_migration` + `project_ayastorm_r41_design_principles` + `feedback_ubo_migration_one_at_a_time` + `project_build_procedure` + `feedback_use_agents_proactively` + `project_ayastorm_three_platforms` (= PA-7.5 / PA-8 は Linux 先行 + Win/Mac は r42-α/β 検証信任)

---

## §6 次 session 着手 1 line

**PA-7.5 (= `aya_r41_exemplar/sky_placeholder{V,F}.glsl` に `std140` 補完 + 実 indra/ tree end-to-end 3 path PASS 化) または PA-8 (= 85 UBO blueprint 起案 + Exit Criteria 09 §4.2 充足検証) entry。AYA さんに micro-fix を挟むか直接 PA-8 か確認推奨。**
