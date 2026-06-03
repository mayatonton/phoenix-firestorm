# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-5 完了

**作成日**: 2026-06-03
**前 session commit**: `74d7a90d30` (PA-3 + PA-4 並列完了 = 5 module 1130 line)
**本 session 物理出力**: `scripts/ubo_codegen/perfect_hash.py` 568 line (= 未 commit、本 handoff doc 起案 commit と同 batch で AYA 判断)
**次 session 着手**: PA-6 (= 増分 build cache hash + mtime 配備 + .inl 実 file 書出経路 wire = 08 §11.5)

---

## §0 state 一行 summary

η-30 **Phase 1.A PA-5 完了 state** (= CHD perfect hash + frozen-table 構造体 codegen + `<BlockName>Layout` / `<BlockName>_<MemberName>_OFFSET` / `<BlockName>_SIZE` 識別子 emit + block_name → BlockMetadata dispatch + 880-key stress PASS + unittest 75/75 + py_compile 6/6 + `indra/` working tree clean)。**次 session 着手地点 = PA-6 (= `scripts/ubo_codegen/build_cache.py` 起草 = chapter 08 §11.5 増分 build cache hash + mtime 併用 + 4 file 分割 emit + .inl 実 file 書出経路 wire)**。

PA-5 完了で残 PA は **strict 線形** = PA-6 → PA-7 → PA-8。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。PA-6 着手 session では **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read (offset/limit) する。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-PA-5-complete.md` | 全文 | 本 handoff (= PA-5 完了 state + PA-6 着手地点 + 4 file 分割 emit 判断保留点) |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-entry.md` | §3 PA-X 全体構成表 + §2 Phase 1.A scope + Exit Criteria | Phase 1.A 全体像 + PA-6 sub-task scope literal 参照 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` | §11 (= 増分 build cache strategy) + §11.5 (= 増分 build cache 詳細化 Phase 2d-β-revise Deliverable B-3) + §6 (= `ubo_metadata.inl` 出力契約) | PA-6 cache key 構成 + JSON cache 構造 + 4 file 分割 emit の source of truth |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` | §5.1 生成ファイル群 4 種 (= `ubo_layout_<blockname>.inl` / `ubo_perfect_hash.inl` / `ubo_metadata.inl` / `ubo_index.inl`) + §5.5 配置 path |
| `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` | §11.5.1 cache key environment block (= Python / glslang / spirv-cross version) + §11.5.2 mtime + hash 併用 algorithm + §12 (PA-7 で使う) CMake DEPENDS |
| `scripts/ubo_codegen/main.py` | PA-2 起案物 = entry point / `run()` / log / exit code、PA-6 で cache miss → emit 経路を wire する着地点 |
| `scripts/ubo_codegen/perfect_hash.py` | PA-5 起案物 = `emit_perfect_hash_inl(blocks)` API、PA-6 で 4 file 分割 emit に切替えるか統合維持か判断点 |
| `scripts/ubo_codegen/glslang_preproc.py` | PA-3 起案物 = `version()` for cache material (= 08 §11.5.1 codegen 環境 cache key) |
| `scripts/ubo_codegen/spirv_reflect.py` | PA-4 起案物 = `version()` for cache material + `should_skip_spirv_check()` (= 08 §11.5.1 codegen 環境 cache key) |
| `scripts/ubo_codegen/std140.py` | PA-4 起案物 = `BlockLayout` / `MemberLayout` schema、PA-6 で metadata.inl 分割 emit の input shape |

---

## §2 本 session 成果 (= PA-5 完了 内訳)

**物理出力**: `scripts/ubo_codegen/perfect_hash.py` 568 line 新規 + `scripts/ubo_codegen/tests/test_perfect_hash.py` 32 test 新規 (= local-only repo-wide `.gitignore` 配下)。
**commit 状態**: **未 commit** (= `feedback_no_auto_commit` 準拠、AYA さん明示指示後に handoff doc + perfect_hash.py + 設計差分 2 件記録を batch commit)。

### §2.1 PA-5 真 scope 達成 (= 04 §5.6 + 08 §6 + 02 §2.4 全実装)

| sub-section / 出力契約 | 実装箇所 (`scripts/ubo_codegen/perfect_hash.py`) | 内容 |
|---|---|---|
| 04 §5.6.1 (CHD 採用根拠) | module docstring + comment | default CHD (Belazzougui/Botelho/Dietzfelbinger 2009) 採用、FCH/BDZ/naive seed search 不採用 |
| 04 §5.6.2 (2 段 hash + displacement table 概念) | `build_chd()` body | bucket g(key) % M → displacement d_i → final (h_inner ⊕ d) % N |
| 04 §5.6.3 (構築 step) | `build_chd()` step 1-4 | (1) duplicate detection + table_size = next_prime(max(int(n*1.1), n+1)) + bucket_count = max(1, n // λ) / (2) FNV-1a で bucket 振分 / (3) bucket size 降順 sort / (4) per-bucket seed reroll (= MAX_DISPLACEMENT_SEEDS=2^20 cap) |
| 04 §5.6.4 (algorithm 性能特性) | end-to-end smoke で実測 | 880-key emit 24.66 ms / table_size=971 / bucket_count=220 (= 設計 N=880 → ~5 ms 同 order、Python overhead 込みで誤差 level) |
| 04 §5.6.5 (衝突 0 invariant build-time 保証) | `verify_chd()` | 全 input key を lookup 経路で再 resolve、slot None / key mismatch で CodegenError (= 08 §9.4 E4 format) |
| 04 §5.6.6 (出力 C++ data 形式) | `emit_perfect_hash_inl()` + 5 sub-emit | (1) `UniformLocation` struct / (2) `<BlockName>Layout` + `<BlockName>_SIZE` / (3) `BlockMetadata` table + `lookup_block()` / (4) `g_chd_displacement` + `g_chd_values` + `g_chd_key_strings` / (5) `fnv1a_32` constexpr + `lookup_runtime` inline (= false-positive 排除 = parallel key-string table + strcmp) |
| 04 §5.6.7 (実装規模見積) | 実測 568 line | 設計 ~310 line vs 実装 568 line = 差 ~250 line は (b) frozen-table 拡張 (~60) + (c) per-block Layout struct emit (~30) + docstring / comments / data types / lookup_python (~120) (= handoff §3.2 scope (b)+(c) 込みの真 scope を吸収) |
| 02 §2.4 識別子 4 種 | `_emit_block_layouts()` + emit | (a) `<BlockName>Layout` struct ✅ / (b) `<BlockName>_<MemberName>_OFFSET` static constexpr ✅ / (c) `<BlockName>_SIZE` inline constexpr ✅ / (d) `ubo_layout_<blockname>.inl` 別 file 分割は **PA-6 で wire** (= 本 PA-5 では `emit_perfect_hash_inl()` 単一 string output) |
| 08 §6 BlockMetadata schema | `_emit_block_metadata_table()` | `block_name` / `block_hash` / `block_size` / `descriptor_set` / `binding` / `subset` / `cadence_tag` / `member_count` (= 設計 §6.1 schema literal、§7.1 sort 規則準拠 block_name 昇順) |
| 09 §11.6 確定 R3 = name-based dispatch | `lookup_runtime()` inline emit | R1 (template <auto>) 不採用 / C++17 維持、`std::strcmp` non-constexpr 制約に合わせ `inline` (= compile-time 呼出不要、R3 = runtime mUniform[index] pre-cache fill path) |

### §2.2 設計との微差分 (= 実装上の制約による小修正、必ず引き継ぐ)

| # | spec 記述 | 実装変更 | 理由 |
|---|---|---|---|
| 1 | 04 §5.6.6 `inline constexpr uint16_t g_chd_displacement[M]` | `uint32_t` に変更 | `MAX_DISPLACEMENT_SEEDS = 1 << 20 = 1048576` > uint16_t max 65535、安全側。実 880-key 計測では seed < 100 で収まるが overflow 防御 |
| 2 | 04 §5.6.6 `constexpr lookup_runtime(...)` | `inline` (constexpr 除去) | `std::strcmp` は C++17 で非 constexpr、Q-NTTP=A 確定 (= R1 不採用) で compile-time 呼出は不要、runtime-only path で R3 = `mUniform[index]` pre-cache fill 用途 = `inline` で十分 |

両件は 09 §11.6 / 08 §5.4.1.5 / 04 §5.6.6 の **思想は維持**、実装制約上の物理選択 (= 設計違反ではない)。PA-7 cmake build 検証時に必ず確認、必要なら設計 doc に追記 cross-ref。

### §2.3 unittest 32 test 内訳 (= `scripts/ubo_codegen/tests/test_perfect_hash.py` local-only)

| class | test 数 | 内容 |
|---|---|---|
| `HashHelperTests` | 7 | fnv1a_32 deterministic + 空入力 = offset_basis + 1 byte 既知値 + seed 変化 + 32 bit mask + `next_prime` 7 case + `_is_prime` 16 case |
| `CHDConstructionTests` | 11 | empty / single / 3-key / 100-key / **880-key stress** (= 88 block × 10 member) / unknown key → None / duplicate key 拒絶 / length mismatch / λ=0 / capacity 0.5 / **seed exhaustion** (= 50 key 1 seed budget で強制失敗) / **deterministic output** (= 同入力 2 回で同 result) |
| `VerifyCHDTests` | 3 | 正常 build で PASS / slot key 改竄で raise / empty table + non-empty keys で raise |
| `CollectUniformKeysTests` | 1 | 2 block で keys = `block::member` 形式 + block_hash 共有 + cadence 個別 + offset/size passthrough |
| `EmitTests` | 10 | `_escape_cstring` 3 case (basic / quote+backslash / non-ascii) + `_wrap_uint_array` per-line 区切り + 5 section 全部入り smoke + empty blocks framework / sorted by name (§7.1) / metadata literal field (binding/size/count) / **end-to-end keys resolve** (= emit + lookup_python 整合) |

**実行 command**: `python3 -m unittest discover -s scripts/ubo_codegen/tests -t scripts/ubo_codegen` = 0.023s / **75/75 PASS** (43 prior + 32 new) / 0 failure / 0 error

### §2.4 end-to-end smoke (= 88 block × 10 member = 880-key)

| 観点 | 結果 |
|---|---|
| emit time | 24.66 ms |
| inl 出力 size | 156164 byte / 3371 line |
| CHD table_size | 971 (= prime ≥ next_prime(int(880*1.1)) + 余裕) |
| CHD bucket_count | 220 (= 880 // λ=4) |
| lookup miss (= 全 880 key を lookup_python で再解決) | **0 / 880** |
| 未登録 key の lookup | `None` (= false-positive 排除 OK) |

### §2.5 module 行数

| file | line | 状態 |
|---|---|---|
| `scripts/ubo_codegen/perfect_hash.py` | 568 | 新規、未 commit |
| `scripts/ubo_codegen/tests/test_perfect_hash.py` | 366 | 新規、local-only (= repo `.gitignore` 配下、commit 対象外) |

既存 5 module (PA-3 + PA-4) は 不変動。

---

## §3 次着手地点 = PA-6 (増分 build cache + 4 file 分割 emit + 実 file 書出経路)

### §3.1 着手 1 line

「**PA-6 = `scripts/ubo_codegen/build_cache.py` 起草 + `main.py` 配線 = chapter 08 §11.5 増分 build cache hash + mtime 併用 (= B4a) + cache key environment block (= Python / glslang / spirv-cross version 連動) + 4 file 分割 emit (= `ubo_layout_*.inl` / `ubo_perfect_hash.inl` / `ubo_metadata.inl` / `ubo_index.inl`) + cache miss 時のみ Codegen 経路実行 + cache hit 時は touch のみで CMake DEPENDS 解消**」(= entry handoff §3 PA-6 row literal 継承)

### §3.2 PA-6 scope (= entry handoff §3 PA-6 row literal + 04 §5.1 file 分割契約)

| 項目 | 内容 |
|---|---|
| 出力 | (1) `scripts/ubo_codegen/build_cache.py` 新規 = cache key 構成 + JSON load/save + skip 判定 / (2) `scripts/ubo_codegen/main.py` 配線 = parse 結果 + std140 layout + SPIR-V reflection + perfect_hash emit を実 file 書出 / (3) `perfect_hash.py` 内 emit を 4 file 分割 emit に拡張 (= 既存 `emit_perfect_hash_inl` を維持しつつ `emit_layout_inl` / `emit_metadata_inl` / `emit_index_inl` 追加) |
| 依存 chapter | 08 §11.5 cache strategy + 08 §11.5.1 cache key environment + 08 §11.5.2 algorithm + 04 §5.1 生成 file 群 + 04 §5.5 配置 path (= `indra/newview/generated/ubo/`) + 08 §17 (B4) cache 解消 |
| 入力 | GLSL blueprint dir + 出力 header dir + 既存 cache JSON (= `<cache_dir>/codegen_state.json`) |
| 出力 | (a) 4 file 物理書出 (= cache miss 時のみ) / (b) cache JSON 更新 (= mtime + sha256 + global_output_hash) / (c) cache hit 時 .inl touch のみ (= CMake DEPENDS 解消) |
| 完了条件 | 同一 GLSL 入力 2 回実行で 2 回目 skip + GLSL 1 file 変更で当該 .inl のみ regenerate + Python / glslang / spirv-cross version 変更で全 invalidate + cache JSON schema version=1 |
| unit test | `tests/test_build_cache.py` (local-only) で cache miss / cache hit / per-file invalidation / global invalidation / cache JSON schema |

### §3.3 PA-6 完了後の次 task (= strict 線形)

| PA | scope |
|---|---|
| **PA-7** | CMake DEPENDS + 手動 target (= 08 §12) = `indra/cmake/` に codegen target 配線 + DEPENDS で blueprint 変更検知 + 手動 `codegen_ubo_force` target 配備 |
| **PA-8** | 85 UBO blueprint 実行 + Exit Criteria 充足検証 (= 09 §4.2) = `--input <85 blueprint dir> --output <indra/newview/generated/ubo/>` で 4 file 生成 + build error 0 + 名前解決衝突 0 + 既存 program 1 個 include + bind 不変動作確認 |

### §3.4 PA-6 は Claude 自走可 (AYA 判断不要)

(= cache strategy B4a は 08 §11.2 で確定済、cache key environment は 08 §11.5.1 で確定済、4 file 分割契約は 04 §5.1 で確定済、AYA 判断仰ぎ batch 不要)。

### §3.5 PA-6 着手前 self-verify 必須項目 (= `feedback_doubt_self_first` + `feedback_self_verify_before_handoff` 適用)

1. **PA-5 設計差分 2 件 (= §2.2) を 04 §5.6.6 + 08 §5.4.1.5 + 09 §11.6 に cross-ref 追加するか PA-7/PA-8 着手前に判断** (= 設計 doc を後追い update する場合は本 PA-6 session で対象 paragraph 候補を pin する)
2. **`emit_perfect_hash_inl` 既存 API の後方互換維持** (= PA-6 で 4 file 分割 emit 追加時、既存 `emit_perfect_hash_inl` 単一 string output は test code でも参照されている、break しない)
3. **`indra/newview/generated/ubo/` 配置 dir は git ignore 対象** (= 04 §5.5 + 08 §11.4、PA-6 で `.gitignore` entry 追加要確認 = 既存 entry あれば skip、無ければ追加)
4. **cache JSON file 配置 = `build/codegen/cache/codegen_state.json`** (= 08 §11.3 schema、`build/` 配下なら既存 git ignore 経路で commit 対象外)

---

## §4 self-verify (= PA-5 完了時点 9 観点 PASS)

| # | 観点 | 結果 |
|---|---|---|
| 1 | `scripts/ubo_codegen/perfect_hash.py` 物理存在 568 line | ✅ |
| 2 | py_compile 6/6 PASS (Python 3.12.3) | ✅ codegen_error 51 + glslang_preproc 99 + glsl_parser 483 + std140 210 + spirv_reflect 287 + perfect_hash 568 |
| 3 | unittest **75/75** PASS (= 43 prior + 32 new) | ✅ 0.023s / 0 failure / 0 error |
| 4 | 880-key end-to-end stress = emit 24.66 ms / lookup miss 0 / unknown key → None | ✅ |
| 5 | 04 §5.6.1〜§5.6.7 全 7 sub-section 達成 | ✅ (= §2.1 表 7 row) |
| 6 | 02 §2.4 識別子 4 種 達成 (= `<BlockName>Layout` / `<MemberName>_OFFSET` / `<BlockName>_SIZE` / `ubo_layout_<blockname>.inl` (file 分割は PA-6 wire)) | ✅ (= 識別子 3 種 emit 済 + file 分割は PA-6 真 scope) |
| 7 | 08 §6 BlockMetadata schema (= 8 field + sort by name §7.1) emit 達成 | ✅ |
| 8 | `indra/` working tree clean (= `feedback_design_phase_no_code_write` 解禁後でも `scripts/` 配下のみ改変) | ✅ `git status indra/` clean |
| 9 | 共著行不在 (= `Co-Authored-By: Claude` 不在、本 doc + 本 commit 両方) | ✅ (本 commit 未実行、commit 時に再確認) |

---

## §5 引き継ぎ済 memory (= PA-6 entry session で重要度 ↑)

- `feedback_handoff_minimal_pre_req_read` (= 全件読み禁止、本 §1.1 3 件のみ)
- `feedback_design_phase_no_code_write` (= 完全解禁済、PA-6 で `scripts/ubo_codegen/build_cache.py` 新規可)
- `feedback_no_scope_shrink` (= PA-6 scope = entry handoff §3 PA-6 row literal + 4 file 分割契約 04 §5.1、勝手な縮小禁止)
- `feedback_self_verify_before_handoff` (= AYA 確認前に Claude 全 sub-task self-verify、本 PA-5 で end-to-end 880-key stress + 75/75 unittest で適用済)
- `feedback_no_claude_coauthor` (= 全 commit 共著行禁止)
- `feedback_one_step_at_a_time` (= 1 メッセージ 1 アクション、AYA 質問時も 1 件ずつ)
- `feedback_doubt_self_first` (= test 失敗時は推測せず即 trace、本 PA-5 で 3 件適用 = next_prime 早期 return / seed exhaustion 確率不足 / sort 方向誤読)
- `feedback_proactive_handoff` (= PA-X 単位境界で handoff doc 起案候補、本 doc 起案 = 適用例)
- `feedback_no_auto_commit` (= AYA 明示指示後のみ commit、本 PA-5 session も同様)
- `feedback_remove_verification_logs` (= PA-6 中の検証 log は commit 前除去)
- `feedback_build_only_verified` (= 効果未確認の commit 積まない、PA-6 完了は cache miss/hit/invalidation unit test + 同一入力 2 回実行 skip 確認で判定)
- `feedback_tests_dir_never_commit` (= `scripts/ubo_codegen/tests/` 配下 test file は repo 全体 `.gitignore` で local-only、commit 対象外)
- `project_ayastorm_r41_vulkan_migration` (= r41 章 active pointer)
- `project_ayastorm_r41_design_principles` (= upstream 取込容易性 + core 並列化容易性の 2 大設計原則)
- `feedback_ubo_migration_one_at_a_time` (= UBO 化作業は 1 つずつ、Phase 2 以降で重要度 ↑)
- `project_build_procedure` (= AYAstorm Linux build flow、PA-7/PA-8 で実 build 検証使用)
- `feedback_use_agents_proactively` (= 複数 grep 連鎖 + 3 file 以上の確認は Agent、PA-6 cache 機構 unit test 起草で候補)

---

## §6 次 session 着手 1 line (= PA-6 実装 session 入り時)

「前 session で η-30 Phase 1.A PA-5 完了 = perfect hash CHD + frozen-table 構造体 codegen = `scripts/ubo_codegen/perfect_hash.py` 568 line + 32 unit test、04 §5.6.1〜§5.6.7 全 7 sub-section + 02 §2.4 識別子 3 種 (= `<BlockName>Layout` / `<MemberName>_OFFSET` / `<BlockName>_SIZE`) + 08 §6 BlockMetadata schema emit + 09 §11.6 確定 R3 = name-based dispatch (= C++17 維持、`std::strcmp` 非 constexpr 制約に合わせ `lookup_runtime` を `inline` 化)、880-key end-to-end stress = emit 24.66 ms / lookup miss 0 / table_size 971 / bucket 220、unittest 75/75 PASS + py_compile 6/6 PASS + `indra/` working tree clean。設計差分 2 件 (= `g_chd_displacement` uint16 → uint32 / `lookup_runtime` constexpr → inline) は思想維持の物理選択、設計 doc cross-ref 追加要否は PA-6 session で判断。本 session = **η-30 Phase 1.A PA-6 = 増分 build cache hash + mtime + 4 file 分割 emit + 実 file 書出経路 wire** = `scripts/ubo_codegen/build_cache.py` 新規 + `main.py` 配線 + `perfect_hash.py` 内 emit を 4 file 分割に拡張 (= `ubo_layout_<blockname>.inl` / `ubo_perfect_hash.inl` / `ubo_metadata.inl` / `ubo_index.inl`)、chapter 08 §11.5 cache strategy B4a (= hash + mtime 併用) + §11.5.1 cache key environment (= Python / glslang / spirv-cross version) + 04 §5.1 file 分割契約を source of truth、cache JSON schema version=1、同一 GLSL 入力 2 回実行で 2 回目 skip + per-file invalidation + global invalidation 完了条件。PA-6 は Claude 自走可 (= cache strategy B4a は 08 §11.2 で確定済、4 file 分割契約は 04 §5.1 で確定済、AYA 判断不要)。PA-6 完了後は PA-7 (= CMake DEPENDS + 手動 target = 08 §12) 単独着手、PA-7 → PA-8 は strict 線形。各 PA-X 単位で `feedback_build_only_verified` + `feedback_no_claude_coauthor` + `feedback_no_auto_commit` 準拠、handoff doc は PA-X 単位境界で起案候補」
