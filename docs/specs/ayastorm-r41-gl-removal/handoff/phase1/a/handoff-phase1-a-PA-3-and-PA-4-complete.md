# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-3 + PA-4 並列完了

**作成日**: 2026-06-03
**前 session commit**: `74d7a90d30` (PA-3 + PA-4 並列完了 = 5 module 新規 1130 line)
**次 session 着手**: PA-5 (= perfect hash CHD + frozen-table 名前 → metadata dispatch 構造体 codegen)

---

## §0 state 一行 summary

η-30 **Phase 1.A PA-3 + PA-4 並列完了 state** (= GLSL mini-parser + glslang -E 前処理 + std140 calculator + SPIR-V reflection 二重保証 全 5 module 起案 + unittest 43/43 PASS + py_compile 5/5 PASS + `indra/` working tree clean)。**次 session 着手地点 = PA-5 (= `scripts/ubo_codegen/perfect_hash.py` 起草 = chapter 09 §11.6 確定 R3 = perfect hash CHD + frozen-table 構造体 codegen = 08 §7)**。

PA-3 + PA-4 完了で並列実施可枠は使い切り。PA-5 以降は **strict 線形**: PA-5 → PA-6 → PA-7 → PA-8。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。PA-5 着手 session では **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read (offset/limit) する。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-PA-3-and-PA-4-complete.md` | 全文 | 本 handoff (= PA-3 + PA-4 完了 state + PA-5 着手地点) |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-entry.md` | §3 PA-X 全体構成表 + §2 Phase 1.A scope + Exit Criteria | Phase 1.A 全体像 + PA-5 sub-task scope literal 参照 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` | §7 (= 名前 → metadata dispatch table 出力契約) + §11.5 (= 増分 build cache hash + mtime) | PA-5 出力 file 構造 + PA-6 連動 cache 設計 source of truth |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` | §5.6 perfect hash CHD アルゴリズム本体 + §6.4 device 256B padding + §10 (B1) Python 3.8+ 要件 |
| `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §11.6 (Q-NTTP) 確定 = R3 name-based dispatch (R1 不採用 / C++17 維持) + §4 Phase 1.A scope + §14.4 末尾 ST-7 sub-task 8 完走 paragraph |
| `docs/specs/ayastorm-r41-gl-removal/design/02-naming-convention.md` | §2.4 Codegen-UBO 生成識別子 4 種 (= `<BlockName>Layout` / `<BlockName>_<MemberName>_OFFSET` / `<BlockName>_SIZE` / `ubo_layout_<blockname>.inl`) |
| `scripts/ubo_codegen/main.py` | PA-2 起案物 = entry point / `run()` / log / exit code、PA-5 module を wire するときの着地点 |
| `scripts/ubo_codegen/std140.py` | PA-4 起案物 = `BlockLayout` / `MemberLayout` dataclass、PA-5 で metadata table 出力するときの input shape |
| `scripts/ubo_codegen/glsl_parser.py` | PA-3 起案物 = `ParseResult` / `UboBlockDecl` / `SamplerDecl` dataclass、PA-5 で block_name 全集合を取り出すときの参照 |
| `scripts/ubo_codegen/spirv_reflect.py` | PA-4 起案物 = `_normalise_reflection` + `verify_layout_against_spirv`、PA-5 では直接呼び出さず main.py 経由連携 |
| `scripts/ubo_codegen/codegen_error.py` | PA-3/4 共通 `CodegenError` + `format_error()`、PA-5 でも同じ exception 基盤を継承 |

---

## §2 本 session 成果 (= PA-3 + PA-4 並列完了 内訳)

**commit**: `74d7a90d30` (5 files changed, +1130/-0, 5 new file)

### §2.1 PA-3 真 scope 達成 (= 08 §5.2 / §5.2.1.* 全実装)

| sub-section | 実装箇所 (`scripts/ubo_codegen/`) | 内容 |
|---|---|---|
| §5.2 (glslang -E wrapper) | `glslang_preproc.py` (99 line) | `find_glslang()` + `preprocess()` + `version()` + DEFAULT 候補名 fallback (`glslangValidator` / `glslang`) |
| §5.2.1.1 token 表 | `glsl_parser.py` (483 line) | LINE_DIR / HASH_DIR / WS / NL / LBRACE / RBRACE / LPAREN / RPAREN / LBRACK / RBRACK / COMMA / SEMI / EQ / INT / IDENT / SKIP |
| §5.2.1.2 EBNF | `glsl_parser.py` `parse_glsl()` | program → ubo_block \| struct_def \| bare_uniform \| sampler_decl \| other_skip |
| §5.2.1.3 ubo_block | `_parse_ubo_block()` | `layout(...)  uniform <Name> { <members> } [<instance>];` + std140 必須 check |
| §5.2.1.4 struct_def | `_parse_struct()` | `struct <Name> { <members> };` + duplicate name 拒絶 |
| §5.2.1.5 member_decl | `_parse_member_decl()` | `type ident [literal_size];` + 動的 array (IDENT) 拒絶 |
| §5.2.1.6 unsupported | `UNSUPPORTED_TYPE_IDENTS` frozenset | double / dvec2-4 / dmat NxM / uint64_t / int64_t (= chapter 04 §4.3.1.6 build error 対象) |
| §5.2.1.7 #line tracking | `tokenize()` LINE_DIR handler | `cur_line = N - 1` 補正 (= 直後 NL で N に lift = C/GLSL 規格準拠) |
| §5.2.1.8 nested struct | `parse_glsl()` pass 2 | struct_lookup hit で `Member.nested_struct` 参照 set |
| sampler 切分 | `SAMPLER_TYPES` frozenset | sampler2D / sampler3D / samplerCube / sampler{2D,Cube}Array / samplerCubeShadow / isampler2D / usampler2D / samplerBuffer / sampler2DMS 等 |
| 多重 instance reject | `_parse_ubo_block()` | `} a, b;` 検出で CodegenError (LL AYAstorm GLSL 慣用外) |
| 共通 exception | `codegen_error.py` (51 line) | `CodegenError(Exception)` + `format_error()` (= chapter 08 §9.4 build error format) |

### §2.2 PA-4 真 scope 達成 (= 04 §4.3.1 + 08 §5.4.1 全実装)

| sub-section | 実装箇所 (`scripts/ubo_codegen/`) | 内容 |
|---|---|---|
| §4.3.1.1 base align/size 表 | `std140.py` `PRIMITIVE_TYPES` | scalar 4 種 + vec2/3/4 family 各 4 種 + 正方 mat2/3/4 + 矩形 matNxM 全 9 種 = 27 型 |
| §4.3.1.2 state machine | `compute_layout()` | `round_up(offset, align)` + `offset += base_size` + `max_align` track |
| §4.3.1.3 nested recursion | `compute_struct_type_info()` | layout 再帰 + `base_align = max(16, member 最大 align)` |
| §4.3.1.4 array stride | `arrayify()` | `stride = round_up(base_size, 16)` (= vec3[N] / float[N] が要 vec4 stride) |
| §4.3.1.5 trailing padding | `compute_layout()` 末尾 | `std140_size = round_up(offset, max_align)` |
| §6.4 device padding | `pad_to_device_align()` | `round_up(size, 256)` = `DEVICE_OFFSET_ALIGN` |
| §4.3.1.6 unsupported | `_resolve_type_info()` | dict miss + nested None で CodegenError |
| §5.4.1.1 pipeline | `spirv_reflect.py` `extract_reflection()` | glslang -V -S → .spv → spirv-cross --reflect → JSON |
| §5.4.1.3 per-member verify | `verify_layout_against_spirv()` | block 存在 + block_size + 全 member offset + array_stride 比較 |
| §5.4.1.5 normalisation 境界 | `_normalise_reflection()` | the *only* shape callers see = `{<BlockName>: {_block_size, _set, _binding, <member>: {offset, array_stride}}}` |
| §5.4.1.6 escape hatch | `should_skip_spirv_check()` + `verify_layout_against_spirv()` 先頭 | `AYA_CODEGEN_SKIP_SPIRV_CHECK=1` で WARN log + verify skip |
| `version()` for cache | `spirv_reflect.py` + `glslang_preproc.py` | chapter 08 §11.5.1 cache material |

### §2.3 entry 直前 self-verify gap remediation (= `feedback_doubt_self_first` 適用 2 件)

| # | gap | finding | fix |
|---|---|---|---|
| 1 | `#line N "path"` directive で `cur_line = N` set すると次の NL で `N+1` に bump = off-by-one | `test_line_directive_tracked` で `43 != 42` 失敗 | `cur_line = N - 1` に補正 (= C/GLSL 規格 = #line N applies to NEXT source line) |
| 2 | `_normalise_reflection` で `types = refl.get("types", {}) or {}` が **`or {}` short-circuit で非 dict (= 空 list `[]`) を mask** | `test_types_not_object_raises` で raise されず失敗 | `if types is None: types = {}` + `isinstance(types, dict)` check に分離 |

両件 fix 後 43/43 PASS。

### §2.4 unittest 43 test 内訳 (= `scripts/ubo_codegen/tests/` local-only repo-wide gitignore 配下)

| test file | test 数 | 内容 |
|---|---|---|
| `test_glsl_parser.py` | 10 | simple UBO / nested struct + LightCone / sampler + bare uniform + UBO 混在 / std140 必須 / unsupported type double / 動的 array N / duplicate struct / instance name / multi-instance reject / `#line N "path"` tracking |
| `test_std140.py` | 14 | Util 6 (round_up / pad_to_device_align / arrayify_float stride 16 + total 128 / arrayify_vec3 stride 16 + total 64 / zero align reject / zero count reject) + Layout 9 (3 mat4 = 192 → block 256 / vec3 trailing hole + float fill / float → vec3 align 16 / float[8] stride 16 / mat3 size 48 / mat4[3] stride 64 + total 192 / nested struct = round_up(28, 16) = 32 / unsupported type / max align trailing padding) |
| `test_spirv_reflect.py` | 19 | Normalise 9 (basic block / array_stride passthrough / missing array_stride default 0 / root 非 object reject / types 非 object reject / ubo missing name reject / ubo unknown type reject / empty ubos / ubos 欠落 = 空 dict) + Verify 10 (match passes / block missing / block size mismatch / member missing / member offset mismatch / array stride mismatch / array stride match passes / offset field None reject / escape hatch skip) |

**実行 command**: `python3 -m unittest discover -s scripts/ubo_codegen/tests -t scripts/ubo_codegen` = 0.001s / 0 failure / 0 error

### §2.5 module 行数 1130 line 内訳

| file | line | role |
|---|---|---|
| `scripts/ubo_codegen/codegen_error.py` | 51 | 共通 exception + format helper |
| `scripts/ubo_codegen/glslang_preproc.py` | 99 | `glslangValidator -E` subprocess wrapper |
| `scripts/ubo_codegen/glsl_parser.py` | 483 | GLSL mini-parser 本体 (token → grammar → ParseResult) |
| `scripts/ubo_codegen/std140.py` | 210 | std140 layout calculator (PRIMITIVE_TYPES + compute_layout + arrayify + nested 再帰) |
| `scripts/ubo_codegen/spirv_reflect.py` | 287 | SPIR-V reflection 二重保証 (extract + normalise + verify + escape hatch + version) |
| **計** | **1130** | |

---

## §3 次着手地点 = PA-5 (perfect hash CHD + frozen-table 構造体 codegen)

### §3.1 着手 1 line

「**PA-5 = `scripts/ubo_codegen/perfect_hash.py` 起草 = chapter 09 §11.6 確定 R3 = perfect hash CHD + frozen-table 構造体 codegen + 08 §7 = 名前 → metadata dispatch table 出力契約 (= `<BlockName>Layout` / `<BlockName>_<MemberName>_OFFSET` / `<BlockName>_SIZE` の dispatch table を `.inl` ヘッダで出力)**」(= entry handoff §3 PA-5 row literal 継承)

### §3.2 PA-5 scope (= entry handoff §3 PA-5 row literal)

| 項目 | 内容 |
|---|---|
| 出力 | `scripts/ubo_codegen/perfect_hash.py` 新規起草 |
| 依存 chapter | 04 §5.6 (CHD アルゴリズム) + 08 §7 (出力契約) + 09 §11.6 確定 R3 (= R1 不採用 / C++17 維持) |
| 入力 | parse_glsl + compute_layout 結果 = `List[BlockLayout]` |
| 出力 | (a) CHD perfect hash table + (b) frozen-table = `<BlockName>` → `BlockLayout` の constexpr dispatch + (c) `<BlockName>Layout` / `<BlockName>_<MemberName>_OFFSET` / `<BlockName>_SIZE` 識別子 |
| 完了条件 | 85 UBO 想定で名前衝突なし + dispatch O(1) + 出力 `.inl` ヘッダ 1 件 (PA-6 で実 file 書出は wire) |
| unit test | `tests/test_perfect_hash.py` (local-only) で衝突なし + dispatch 整合性 + 80+ block の stress test |

### §3.3 PA-5 完了後の次 task (= strict 線形)

| PA | scope |
|---|---|
| **PA-6** | 増分 build cache hash + mtime (= 08 §11.5) = blueprint + 環境 (glslang/spirv-cross/Python version) を cache key 化 + 不要 codegen skip + `.inl` 書出経路 |
| **PA-7** | CMake DEPENDS + 手動 target (= 08 §12) = `indra/cmake/` に codegen target 配線 + DEPENDS で blueprint 変更検知 |
| **PA-8** | 85 UBO blueprint 実行 + Exit Criteria 充足検証 (= 09 §4.2) = `--input <85 blueprint dir> --output <indra/...>` で 4 file 生成 + build error 0 + 名前解決衝突 0 |

### §3.4 PA-5 は Claude 自走可 (AYA 判断不要)

(= R3 採用は ST-7 sub-task 8 batch で確定済、CHD アルゴリズム + 出力契約は chapter 04 §5.6 + 08 §7 で確定済、AYA 判断仰ぎ batch 不要)。

---

## §4 self-verify (= 9 観点 PASS)

| # | 観点 | 結果 |
|---|---|---|
| 1 | commit `74d7a90d30` 履歴に PA-3 + PA-4 完了記載 | ✅ |
| 2 | 5 module 物理存在 1130 line | ✅ codegen_error.py 51 + glslang_preproc.py 99 + glsl_parser.py 483 + std140.py 210 + spirv_reflect.py 287 |
| 3 | unittest 43/43 PASS (= 0.001s / 0 failure / 0 error) | ✅ `python3 -m unittest discover -s scripts/ubo_codegen/tests -t scripts/ubo_codegen` |
| 4 | py_compile 5/5 PASS (Python 3.12.3) | ✅ |
| 5 | PA-3 = 08 §5.2 / §5.2.1.* 全 sub-section 達成 | ✅ §5.2.1.1〜§5.2.1.8 全 8 sub-section + glslang -E wrapper + sampler/bare 切分 + 多重 instance reject |
| 6 | PA-4 = 04 §4.3.1 + 08 §5.4.1 全 sub-section 達成 | ✅ §4.3.1.1〜§4.3.1.6 全 6 sub-section + §5.4.1.1/§5.4.1.3/§5.4.1.5/§5.4.1.6 + §6.4 device padding |
| 7 | `indra/` working tree clean (= `feedback_design_phase_no_code_write` 解禁後でも scripts/ 配下のみ) | ✅ `git status indra/` clean |
| 8 | 共著行不在 (`Co-Authored-By: Claude` 不在 commit) | ✅ `git log -1 --format="%(trailers:key=Co-Authored-By)"` empty |
| 9 | handoff doc 命名対称 (`...eta-30-phase1-a-PA-3-and-PA-4-complete.md`) | ✅ |

---

## §5 引き継ぎ済 memory (= PA-5 entry session で重要度 ↑)

- `feedback_handoff_minimal_pre_req_read` (= 全件読み禁止、本 §1.1 3 件のみ)
- `feedback_design_phase_no_code_write` (= 完全解禁済、PA-5 で `scripts/ubo_codegen/perfect_hash.py` 新規可)
- `feedback_no_scope_shrink` (= PA-5 scope = entry handoff §3 PA-5 row literal、勝手な縮小禁止)
- `feedback_self_verify_before_handoff` (= AYA 確認前に Claude 全 sub-task self-verify、本 PA-3+4 session で gap 2 件検出 + remediation で適用済)
- `feedback_no_claude_coauthor` (= 全 commit 共著行禁止)
- `feedback_one_step_at_a_time` (= 1 メッセージ 1 アクション、AYA 質問時も 1 件ずつ)
- `feedback_doubt_self_first` (= test 失敗時は推測せず即 trace、本 PA-3+4 で 2 件適用 = #line N off-by-one + types `or {}` short-circuit)
- `feedback_proactive_handoff` (= PA-X 単位境界で handoff doc 起案候補)
- `feedback_no_auto_commit` (= AYA 明示指示後のみ commit)
- `feedback_remove_verification_logs` (= PA-5 中の検証 log は commit 前除去)
- `feedback_build_only_verified` (= 効果未確認の commit 積まない、PA-5 完了は CHD 衝突なし + dispatch 整合性 unit test PASS で判定)
- `feedback_tests_dir_never_commit` (= `scripts/ubo_codegen/tests/` 配下 test file は repo 全体 `.gitignore` で local-only、commit 対象外)
- `project_ayastorm_r41_vulkan_migration` (= r41 章 active pointer)
- `project_ayastorm_r41_design_principles` (= upstream 取込容易性 + core 並列化容易性の 2 大設計原則)
- `feedback_ubo_migration_one_at_a_time` (= UBO 化作業は 1 つずつ、Phase 2 以降で重要度 ↑)
- `project_build_procedure` (= AYAstorm Linux build flow、PA-7/PA-8 で実 build 検証使用)
- `feedback_use_agents_proactively` (= 重い trace + 複数 grep 連鎖は Agent、PA-5 CHD 衝突 stress test で候補)

---

## §6 次 session 着手 1 line (= PA-5 実装 session 入り時)

「前 session で η-30 Phase 1.A PA-3 + PA-4 並列完了 = GLSL mini-parser (= `glsl_parser.py` 483 line、08 §5.2.1.* 全 8 sub-section) + glslang -E 前処理 wrapper (= `glslang_preproc.py` 99 line) + std140 calculator (= `std140.py` 210 line、04 §4.3.1.* 全 6 sub-section + 27 型 PRIMITIVE_TYPES) + SPIR-V reflection 二重保証 (= `spirv_reflect.py` 287 line、08 §5.4.1.* 全 4 sub-section + escape hatch + `_normalise_reflection` 単一 JSON schema 境界) + 共通 exception (= `codegen_error.py` 51 line)、計 5 module 新規 1130 line、unittest 43/43 PASS + py_compile 5/5 PASS + `indra/` working tree clean、commit `74d7a90d30` で着地。本 session = **η-30 Phase 1.A PA-5 = perfect hash CHD + frozen-table 構造体 codegen 起草** = `scripts/ubo_codegen/perfect_hash.py` 新規 file 作成、chapter 04 §5.6 (CHD アルゴリズム) + 08 §7 (= 名前 → metadata dispatch table 出力契約) + 09 §11.6 確定 R3 (= R1 不採用 / C++17 維持) を source of truth として、85 UBO 想定で名前衝突なし + dispatch O(1) + `.inl` ヘッダ 1 件出力 (= 実 file 書出は PA-6 で wire)。PA-5 は Claude 自走可 (= R3 採用は ST-7 sub-task 8 batch で確定済、AYA 判断不要)。PA-5 完了後は PA-6 (= 増分 build cache hash + mtime = 08 §11.5) 単独着手、PA-6 → PA-7 → PA-8 は strict 線形 (= 並列実施可枠は PA-3+4 で使い切り)。各 PA-X 単位で `feedback_build_only_verified` + `feedback_no_claude_coauthor` + `feedback_no_auto_commit` 準拠、handoff doc は PA-X 単位境界で起案候補」
