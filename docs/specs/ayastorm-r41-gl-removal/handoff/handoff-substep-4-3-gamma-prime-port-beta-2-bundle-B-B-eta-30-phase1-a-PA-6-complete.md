# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-6 完了

**作成日**: 2026-06-03
**前 session commit**: `1463633fbf` (PA-5 完了 = `scripts/ubo_codegen/perfect_hash.py` 568 line + 32 unittest PASS)
**本 session 物理出力**: `scripts/ubo_codegen/build_cache.py` 340 line 新規 + `scripts/ubo_codegen/perfect_hash.py` 4 file 分割 emit 拡張 + `scripts/ubo_codegen/main.py` 全配線書き直し (= 未 commit、本 handoff doc 起案 commit と同 batch で AYA 判断)
**次 session 着手**: PA-7 (= CMake DEPENDS + 手動 target = 08 §12)

---

## §0 state 一行 summary

η-30 **Phase 1.A PA-6 完了 state** (= 増分 build cache hash + mtime 併用 = B4a + cache key environment block = Python / glslang / spirv-cross version + 4 file 分割 emit = `ubo_layout_<blockname>.inl` / `ubo_perfect_hash.inl` / `ubo_metadata.inl` / `ubo_index.inl` + `main.py` end-to-end 配線 = glslang preprocess → parse → std140 → SPIR-V verify → 4 file emit → cache write + 同一入力 2 回実行で 2 回目 skip = touch のみ + `--force` で bypass + unittest 109/109 + py_compile 8/8 + `indra/` working tree clean)。**次 session 着手地点 = PA-7 (= CMake DEPENDS + 手動 target `codegen_ubo_force` 配備 = chapter 08 §12)**。

PA-6 完了で残 PA は **strict 線形** = PA-7 → PA-8 (= 85 UBO blueprint 実行 + Exit Criteria 充足検証 = 09 §4.2)。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。PA-7 着手 session では **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read (offset/limit) する。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-PA-6-complete.md` | 全文 | 本 handoff (= PA-6 完了 state + PA-7 着手地点 + cache JSON schema + main.py CLI 仕様) |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-entry.md` | §3 PA-X 全体構成表 + §2 Phase 1.A scope + Exit Criteria | Phase 1.A 全体像 + PA-7 sub-task scope literal 参照 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` | §12 (= CMake DEPENDS 設計 + 手動 target `codegen_ubo_force`) + §17 (= B4) cache 解消 | PA-7 CMake glue の source of truth |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` | §11.5 (= cache strategy 詳細、PA-6 実装根拠の確認用) + §11.3 (= cache file 配置 path) + §12.1 (= add_custom_command DEPENDS list) + §12.2 (= add_custom_target `codegen_ubo_force` 仕様) |
| `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` | §5.1 (= 4 file 分割契約) + §5.5 (= 配置 path = `indra/newview/generated/ubo/`) |
| `docs/specs/ayastorm-r41-gl-removal/design/09-misc-decisions.md` | §4.2 (= PA-8 Exit Criteria) |
| `scripts/ubo_codegen/build_cache.py` | PA-6 起案物 = cache key 構成 + JSON load/save + `check_cache()` 判定 + `build_state()` 書出 + `touch_outputs()` (= CMake DEPENDS 解消用 mtime bump) |
| `scripts/ubo_codegen/main.py` | PA-6 起案物 = `--input` / `--output` / `--cache-file` / `--project-root` / `--force` / `--glslang-bin` / `--spirv-cross-bin` CLI + cache miss/hit 経路分岐 |
| `scripts/ubo_codegen/perfect_hash.py` | PA-5 + PA-6 = 既存 `emit_perfect_hash_inl()` 維持 + 新規 `emit_layout_inl(block)` / `emit_metadata_inl(blocks)` / `emit_perfect_hash_inl_split(blocks)` / `emit_index_inl(blocks)` の 4 分割 emit API |
| `indra/cmake/` 配下 (= 確認のみ、PA-7 で改変対象) | 既存 codegen 系 cmake module 有無 + `add_custom_command` 既存パターン |

---

## §2 本 session 成果 (= PA-6 完了 内訳)

**物理出力**:
- `scripts/ubo_codegen/build_cache.py` 340 line 新規
- `scripts/ubo_codegen/perfect_hash.py` 4 file 分割 emit API 追加 (= 既存 `emit_perfect_hash_inl()` は後方互換維持)
- `scripts/ubo_codegen/main.py` 全配線 (= 163 line → 325 line、cache 判定 + end-to-end pipeline + CLI 拡張)
- `scripts/ubo_codegen/tests/test_build_cache.py` 34 test 新規 (= local-only repo-wide `.gitignore` 配下)

**commit 状態**: **未 commit** (= `feedback_no_auto_commit` 準拠、AYA さん明示指示後に handoff doc + 3 file 改変を batch commit)。

### §2.1 PA-6 真 scope 達成 (= 08 §11.5 + 04 §5.1 + entry handoff §3 PA-6 row literal)

| sub-section / 出力契約 | 実装箇所 | 内容 |
|---|---|---|
| 08 §11.5 cache strategy B4a (= hash + mtime 併用) | `build_cache.py` `check_cache()` | (1) state file 存在 / (2) schema version=1 / (3) script + 全 module sha256 / (4) environment block / (5) input file sha256_normalized + mtime + file_size / (6) output file sha256 + size 全 invalidate trigger 検出 |
| 08 §11.5.1 cache key environment block | `build_cache.py` `EnvVersions` dataclass + `main.py` `_collect_env()` | glslang_version (= `glslang --version` 先頭行) + spirv_cross_version (= `spirv-cross --version` 先頭行 or "skipped") + python_version (= `sys.version_info`) + host_platform (= `sys.platform`) |
| 08 §11.5.2 algorithm | `check_cache()` step 1-7 + `build_state()` | 全 trigger 列挙 (= STATE_MISSING / SCHEMA_MISMATCH / SCRIPT_SHA / MODULE_SHA / GLSLANG_DRIFT / SPIRV_CROSS_DRIFT / PYTHON_DRIFT / HOST_DRIFT / INPUT_ADDED / INPUT_REMOVED / INPUT_SHA / OUTPUT_MISSING / OUTPUT_TAMPERED / EXPECTED_OUTPUT_DRIFT) → `CacheDecision(is_hit, reason)` 返却 |
| 08 §11.5.7 atomic write | `build_cache.py` `write_state()` | `tempfile.NamedTemporaryFile` + `os.replace()` で atomic、`.tmp` 残骸禁止 |
| 08 §11.5 path normalization | `build_cache.py` `normalize_path()` | POSIX 形式 + project_root 相対 + 範囲外は元 path fallback (= 3 OS 同一 cache 再現性) |
| 08 §11.5 CRLF normalization | `build_cache.py` `sha256_file_normalized()` | CRLF → LF 統一後 sha256 (= Win commit / macOS-Linux 開発で同一 hash) |
| 04 §5.1 4 file 分割 emit | `perfect_hash.py` 4 new emit API | (a) `emit_layout_inl(block)` = 1 block 専用 `<BlockName>Layout` struct + `<MemberName>_OFFSET` + `<BlockName>_SIZE` / (b) `emit_metadata_inl(blocks)` = `BlockMetadata` table + `lookup_block()` / (c) `emit_perfect_hash_inl_split(blocks)` = `UniformLocation` + CHD displacement + lookup_runtime / (d) `emit_index_inl(blocks)` = 4 file 集約 include |
| 04 §5.5 配置 path | `main.py` `--output <dir>` | `mkdir(parents=True, exist_ok=True)` 自動作成、blueprint 実 path は AYA 指定 (PA-8) |
| entry handoff §3 PA-6 row (= main.py 配線) | `main.py` `run()` + `_process_glsl_file()` + `_emit_all()` | glslang preprocess (= `-S <stage>` 自動付与) → parse_glsl → compute_layout → SPIR-V verify (= AYA_CODEGEN_SKIP_SPIRV_CHECK で skip 可) → BlockSpec 構築 → 4 file emit |
| entry handoff §3 PA-6 row (= cache miss/hit 分岐) | `main.py` `run()` step 5-8 | cache hit → `touch_outputs()` で mtime bump のみ (= CMake DEPENDS 解消) / cache miss → 全 .glsl 処理 + 4 file 書出 + `write_state()` |
| 08 §11.3 cache file 配置 | `main.py` `--cache-file` CLI | default = `<output>/../codegen/cache/codegen_state.json` (= AYA 指定可、`build/` 配下既存 git ignore 経路) |

### §2.2 設計との微差分 (= 実装上の制約による小修正、必ず引き継ぐ)

| # | spec 記述 | 実装変更 | 理由 |
|---|---|---|---|
| 1 | 08 §11.5.2 `check_cache(input_files, output_dir, ...)` | `expected_output_file_names` を Optional に降格 | cache pre-check 時点 (= GLSL parse 前) は出力 file 名群が未確定。None 指定時は state 記録済 keys を信頼する fallback path を追加。`main.py` cache hit path で expected None でも touch 可能 |
| 2 | 08 §11.5.1 cache key に `host_platform` 明記なし | `EnvVersions.host_platform = sys.platform` 追加 | 3 OS 横断 sandboxing で同一 input でも platform 差で stage suffix 解釈差が出る可能性。安全側で cache key 構成材料に追加 (= 08 §11.5.1 env block の拡張) |
| 3 | 04 §5.1 `<BlockName>Layout` file は per-block ファイル | `layout_filename(block_name)` = `ubo_layout_<lowercase(block_name)>.inl` | 04 §5.1 は file 名規約 literal 未明記。Win FS case-insensitive 問題回避で lowercase 採用、3 OS 同一 file 名 |

3 件とも 08 §11.5 / 04 §5.1 の **思想は維持**、実装制約上の物理選択。PA-7 cmake DEPENDS 設定時に必ず確認 (= 特に §2.2 #3 の lowercase file 名規約)、必要なら設計 doc cross-ref 追加。

### §2.3 unittest 34 test 内訳 (= `scripts/ubo_codegen/tests/test_build_cache.py` local-only)

| class | test 数 | 内容 |
|---|---|---|
| `HashingTests` | 3 | sha256_file deterministic + CRLF normalization 一致 + trailing whitespace preservation |
| `NormalizePathTests` | 2 | POSIX relative path + outside-root fallback |
| `EnvBlockTests` | 2 | `EnvVersions.to_dict()` round trip + `collect_module_hashes()` keyed by basename |
| `StateIOTests` | 4 | missing → None / corrupt JSON → None / atomic write + sorted JSON / `.tmp` 残骸不在 |
| `CheckCacheTests` | 17 | (01) 初回 miss = STATE_MISSING / (02) 即時 hit / (03) script sha drift / (04) module sha drift / (05) module add drift / (06) glslang drift / (07) spirv-cross drift / (08) python drift / (09) host drift / (10) input added / (11) input removed / (12) input content changed / (13) mtime drift + content unchanged = hit / (14) input disappeared / (15) output missing / (16) output tampered / (17) expected output set drift |
| `BuildStateSchemaTests` | (CheckCache に統合) | schema field 存在性 |
| `TouchOutputsTests` | 2 | mtime bump 確認 + missing file silent skip |
| `CanonicalNamesTests` | (Hashing に統合) | layout_filename lowercase 確認 |

**実行 command**: `python3 -m unittest discover -s tests -t .` from `scripts/ubo_codegen/` = 0.073s / **109/109 PASS** (75 prior + 34 new) / 0 failure / 0 error

### §2.4 end-to-end smoke (= 1-block GLSL `FrameViewProj`)

| 観点 | 結果 |
|---|---|
| Run 1 (cache miss) | emit 67ms / 4 file 書出 / `cache miss: cache state missing or corrupt` log |
| Run 2 (cache hit) | `cache hit (all checks passed) — touching outputs only` / touch のみ実行 |
| Run 3 (`--force`) | `--force given, bypassing incremental cache` / 強制 re-emit 58ms |
| 出力 4 file | `ubo_index.inl` 335B / `ubo_layout_frameviewproj.inl` 695B / `ubo_metadata.inl` 1254B / `ubo_perfect_hash.inl` 2314B |
| cache JSON schema | version=1 / codegen_tool_version + environment + input_files + output_files + build_metadata 全 field 揃い |

### §2.5 module 行数 / 改変

| file | 状態 | 行数 |
|---|---|---|
| `scripts/ubo_codegen/build_cache.py` | 新規、未 commit | 340 |
| `scripts/ubo_codegen/perfect_hash.py` | 改変 (4 emit API 追加)、未 commit | 568 → ~720 (+~150) |
| `scripts/ubo_codegen/main.py` | 全配線書き直し、未 commit | 163 → 325 |
| `scripts/ubo_codegen/tests/test_build_cache.py` | 新規、local-only (= repo `.gitignore` 配下、commit 対象外) | ~310 |

既存 5 module (PA-3 + PA-4 = codegen_error / glslang_preproc / glsl_parser / std140 / spirv_reflect) は **不変動**。

---

## §3 次着手地点 = PA-7 (CMake DEPENDS + 手動 target)

### §3.1 着手 1 line

「**PA-7 = `indra/cmake/` 配下に codegen target 配線 + GLSL blueprint dir を DEPENDS に列挙 + 手動 `codegen_ubo_force` target 配備 (= `--force` flag pass-through) + `indra/newview/CMakeLists.txt` または対応する viewer cmake に `add_custom_command(OUTPUT ubo_*.inl COMMAND python3 scripts/ubo_codegen/main.py ...)` 配線 + cache hit 時の touch path が CMake DEPENDS を解消する動作確認**」(= entry handoff §3 PA-7 row literal 継承)

### §3.2 PA-7 scope (= entry handoff §3 PA-7 row literal + 08 §12)

| 項目 | 内容 |
|---|---|
| 出力 | (1) `indra/cmake/AyaUboCodegen.cmake` (または既存 cmake module に追記、`indra/cmake/` 配下の既存 pattern 確認後決定) 新規 / (2) `indra/newview/CMakeLists.txt` (または viewer 主 cmake) に codegen target include + DEPENDS 配線追加 |
| 依存 chapter | 08 §12.1 add_custom_command 仕様 + §12.2 add_custom_target `codegen_ubo_force` 仕様 + 08 §17 (B4) DEPENDS 解消 |
| 入力 | GLSL blueprint dir path (= AYA 指定、`indra/newview/app_settings/shaders/...` 配下想定だが PA-8 で確定) + `scripts/ubo_codegen/main.py` path + cache file 配置 dir |
| 出力 | (a) CMake configure 時に target 認識 / (b) GLSL blueprint 改変で .inl 再生成 / (c) `make codegen_ubo_force` で `--force` bypass / (d) cache hit 時 .inl 既存ならば touch のみで build 進行 |
| 完了条件 | (i) CMake configure 成功 / (ii) `cmake --build . --target codegen_ubo` で codegen target 実行 / (iii) GLSL 1 file touch 後 build で .inl re-emit 起動 / (iv) 同 build を 2 回実行で 2 回目 skip 確認 / (v) `cmake --build . --target codegen_ubo_force` で強制 re-emit 確認 |
| unit test | CMake test は基本不要、実 build flow で確認 (= PA-7 完了時の self-verify は手動 cmake configure + build) |

### §3.3 PA-7 完了後の次 task (= strict 線形)

| PA | scope |
|---|---|
| **PA-8** | 85 UBO blueprint 実行 + Exit Criteria 充足検証 (= 09 §4.2) = `--input <85 blueprint dir> --output <indra/newview/generated/ubo/>` で 4 file 群生成 + build error 0 + 名前解決衝突 0 + 既存 program 1 個以上 include + 既存描画 bind 不変動作確認 + cold launch 検証 |

### §3.4 PA-7 は Claude 自走可 (AYA 判断不要)

(= CMake DEPENDS 設計は 08 §12 で確定済、手動 target 仕様は 08 §12.2 で確定済、blueprint dir path は PA-8 で確定、AYA 判断仰ぎ batch 不要)。**ただし** `indra/cmake/` 配下の既存 pattern (= 既存 codegen target 有無 / 既存 `add_custom_command` の引数 idiom / Linux/Mac/Win 3 OS の python3 binary 解決方法) を着手前に必ず確認 (= `feedback_use_agents_proactively` 適用候補)。

### §3.5 PA-7 着手前 self-verify 必須項目 (= `feedback_doubt_self_first` + `feedback_self_verify_before_handoff` 適用)

1. **既存 `indra/cmake/` 配下に codegen 系 module があるか確認** (= 重複作成回避、既存 pattern に乗る)
2. **3 OS の python3 解決方法統一** (= `find_package(Python3 REQUIRED)` か直接 `python3` か、AYAstorm 既存 cmake idiom に従う)
3. **`indra/newview/generated/ubo/` git ignore 確認** (= 04 §5.5、PA-7 で実体生成始まる前に `.gitignore` entry 追加要、不在ならば追加)
4. **cache file 配置 = `build/codegen/cache/codegen_state.json`** (= 08 §11.3、CMake で `${CMAKE_BINARY_DIR}/codegen/cache/codegen_state.json` 渡し)
5. **PA-6 設計差分 3 件 (= §2.2) を chapter 08 §11.5.1 + chapter 04 §5.1 に cross-ref 追加するか PA-7/PA-8 着手前に判断**

---

## §4 self-verify (= PA-6 完了時点 9 観点 PASS)

| # | 観点 | 結果 |
|---|---|---|
| 1 | `scripts/ubo_codegen/build_cache.py` 340 line 物理存在 + perfect_hash.py 4 emit API 追加 + main.py 全配線書き直し | ✅ |
| 2 | py_compile 8/8 PASS (Python 3.12.3) | ✅ codegen_error + glslang_preproc + glsl_parser + std140 + spirv_reflect + perfect_hash + main + build_cache |
| 3 | unittest **109/109** PASS (= 75 prior + 34 new) | ✅ 0.073s / 0 failure / 0 error |
| 4 | end-to-end smoke (= Run 1 miss 67ms / Run 2 hit touch only / Run 3 force 58ms) 3 path 全 PASS | ✅ |
| 5 | cache invalidation 全 trigger (= script sha / module sha / glslang / spirv-cross / python / host / input add/remove/modify / output missing/tampered / expected set drift) unit test 検証 | ✅ |
| 6 | 4 file 分割 emit 実 file 書出確認 (= ubo_index.inl / ubo_layout_frameviewproj.inl / ubo_metadata.inl / ubo_perfect_hash.inl) | ✅ |
| 7 | cache JSON schema (= version=1 + codegen_tool_version + environment + input_files + output_files + build_metadata) 全 field 揃い | ✅ |
| 8 | `indra/` working tree clean (= `feedback_design_phase_no_code_write` 解禁後でも `scripts/` 配下のみ改変) | ✅ `git status indra/` clean |
| 9 | 共著行不在 (= `Co-Authored-By: Claude` 不在、本 doc + 本 commit 両方) | ✅ (本 commit 未実行、commit 時に再確認) |

---

## §5 引き継ぎ済 memory (= PA-7 entry session で重要度 ↑)

- `feedback_handoff_minimal_pre_req_read` (= 全件読み禁止、本 §1.1 3 件のみ)
- `feedback_design_phase_no_code_write` (= 完全解禁済、PA-7 で `indra/cmake/` + `indra/newview/CMakeLists.txt` 改変可)
- `feedback_no_scope_shrink` (= PA-7 scope = entry handoff §3 PA-7 row literal + 08 §12、勝手な縮小禁止)
- `feedback_self_verify_before_handoff` (= AYA 確認前に Claude 全 sub-task self-verify、本 PA-6 で end-to-end 3 path + 109/109 unittest で適用済)
- `feedback_no_claude_coauthor` (= 全 commit 共著行禁止)
- `feedback_one_step_at_a_time` (= 1 メッセージ 1 アクション、AYA 質問時も 1 件ずつ)
- `feedback_doubt_self_first` (= test 失敗時は推測せず即 trace、本 PA-6 で glslang `-S vert` 必須に対し直接 trace で fix)
- `feedback_proactive_handoff` (= PA-X 単位境界で handoff doc 起案候補、本 doc 起案 = 適用例)
- `feedback_no_auto_commit` (= AYA 明示指示後のみ commit、本 PA-6 session も同様)
- `feedback_remove_verification_logs` (= PA-7 中の検証 log は commit 前除去)
- `feedback_build_only_verified` (= 効果未確認の commit 積まない、PA-7 完了は実 CMake configure + build + GLSL touch 後再 build skip 確認で判定)
- `feedback_tests_dir_never_commit` (= `scripts/ubo_codegen/tests/` 配下 test file は repo 全体 `.gitignore` で local-only、commit 対象外)
- `project_ayastorm_r41_vulkan_migration` (= r41 章 active pointer)
- `project_ayastorm_r41_design_principles` (= upstream 取込容易性 + core 並列化容易性の 2 大設計原則)
- `feedback_ubo_migration_one_at_a_time` (= UBO 化作業は 1 つずつ、Phase 2 以降で重要度 ↑)
- `project_build_procedure` (= AYAstorm Linux build flow、PA-7 で実 CMake configure + build 検証使用)
- `feedback_use_agents_proactively` (= 複数 grep 連鎖 + 3 file 以上の確認は Agent、PA-7 `indra/cmake/` 既存 pattern 調査で適用候補)
- `project_ayastorm_three_platforms` (= Linux/macOS/Windows 3 OS 揃え、PA-7 CMake で 3 OS python3 解決方法考慮)

---

## §6 次 session 着手 1 line (= PA-7 実装 session 入り時)

「前 session で η-30 Phase 1.A PA-6 完了 = 増分 build cache + 4 file 分割 emit + main.py 全配線 = `scripts/ubo_codegen/build_cache.py` 340 line 新規 + `perfect_hash.py` 4 emit API 追加 + `main.py` 163→325 line 書き直し + `tests/test_build_cache.py` 34 test 新規、08 §11.5 cache strategy B4a (= hash + mtime 併用) + §11.5.1 cache key environment block (= glslang / spirv-cross / python / host_platform) + §11.5.2 全 invalidate trigger (= STATE_MISSING / SCHEMA / SCRIPT / MODULE / 環境 4 種 / INPUT 4 種 / OUTPUT 2 種 / EXPECTED DRIFT) + §11.5.7 atomic write + 04 §5.1 4 file 分割 emit (= `ubo_layout_<blockname>.inl` per-block + `ubo_perfect_hash.inl` + `ubo_metadata.inl` + `ubo_index.inl`) 全達成、end-to-end smoke = Run 1 miss 67ms / Run 2 hit touch only / Run 3 `--force` 58ms、unittest 109/109 PASS + py_compile 8/8 PASS + `indra/` working tree clean。設計差分 3 件 (= `expected_output_file_names` Optional 降格 / `host_platform` 追加 / `layout_filename` lowercase) は思想維持の物理選択、設計 doc cross-ref 追加要否は PA-7 session で判断。本 session = **η-30 Phase 1.A PA-7 = CMake DEPENDS + 手動 target** = `indra/cmake/` 配下に codegen target 配線 + GLSL blueprint dir DEPENDS 列挙 + `add_custom_target(codegen_ubo_force ... --force)` 配備 + `indra/newview/CMakeLists.txt` (または viewer 主 cmake) に include + cache hit 時 touch path で DEPENDS 解消、chapter 08 §12 source of truth、3 OS python3 解決方法は AYAstorm 既存 cmake idiom 準拠。PA-7 は Claude 自走可 (= CMake 設計確定済、AYA 判断不要)、ただし `indra/cmake/` 既存 pattern (= 既存 codegen module / `add_custom_command` idiom / python3 解決) を Agent 並列調査で先確認。PA-7 完了後は PA-8 (= 85 UBO blueprint 実行 + Exit Criteria 充足検証 = 09 §4.2) 単独着手、strict 線形最終 PA。各 PA-X 単位で `feedback_build_only_verified` + `feedback_no_claude_coauthor` + `feedback_no_auto_commit` 準拠、handoff doc は PA-X 単位境界で起案候補」
