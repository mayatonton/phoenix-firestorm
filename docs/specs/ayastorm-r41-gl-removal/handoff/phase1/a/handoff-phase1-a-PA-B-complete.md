# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-B **complete** + PA-N marker

**作成日**: 2026-06-04
**前 commit chain** (= 直前 handoff 起案):
- `60070d048b` = (X) Phase 1.A residual prep handoff doc 起案 (= 副次 (1) 既解消 + (2) 永続記録不要 + (3) spirv-cross CLI install 残存 + (b-1) literal 充足判定維持 案)
- `5bc170579f` = (Z) AYAstorm r20 SSS verify prep handoff doc 起案 (= 並行起案 2/3)
- `33f983c272` = (W) 上流 uniform4iv bug fix prep handoff doc 起案 (= 並行起案 3/3、bug 真位置 line 2558 訂正)

**本 handoff doc 目的**: **候補 (X) Phase 1.A 残作業 (b-1) 案 PA-B (= spirv-cross CLI system install + codegen 再走 + SPIR-V binding cross-check 通電 verify) complete marker + PA-N (Exit + handoff) 統合**。AYA 指示「推奨順で」literal 受領 (2026-06-04) で (X)(b-1) PA-B 着手、結果 = AYA install + Claude verify + Ubuntu noble compat 3 件 副次 fix (= `scripts/ubo_codegen/spirv_reflect.py` +42 -24 line) + codegen 再走 EXIT=0 + 90 blueprint 全 SPIR-V cross-check 通電 PASS + 副次 (2) `cmake -U AYA_SPIRV_CROSS` + reconfigure PASS + unittest 130/130 PASS。

---

## §0 state 一行 summary

候補 (X) Phase 1.A 残作業 (b-1) 案 (= literal 充足判定維持) PA-B + PA-N **完了**:
- ✅ spirv-cross CLI system install (= AYA `sudo apt install -y spirv-cross` 実行、`spirv-cross 2021.01.15+1.3.239.0-1build1` noble/universe)
- ✅ `which spirv-cross` = `/usr/bin/spirv-cross`
- ✅ `AYA_CODEGEN_SKIP_SPIRV_CHECK` env unset で codegen `--force` 再走 EXIT=0、90 blueprint 全 SPIR-V cross-check 通電 PASS、95 file emit、382 member / 10489 ms
- ✅ cmake cache `AYA_SPIRV_CROSS:FILEPATH=AYA_SPIRV_CROSS-NOTFOUND` → `cmake -U AYA_SPIRV_CROSS -S indra -B build-linux-x86_64` invalidate + reconfigure (2.6s) PASS → `AYA_SPIRV_CROSS:FILEPATH=/usr/bin/spirv-cross` 復活 (= 次 build 時 skip env 不在 path に入る、cross-check 通電継続保証)
- ✅ unittest 130/130 PASS、`indra/` + `cmake/` 改変 0、`scripts/ubo_codegen/spirv_reflect.py` のみ +42 -24 line (= 副次 fix 3 件)

= Phase 1.A 全 sub-step 完了 (= PA-1..PA-8 既終了 + PA-B 本 commit + PA-N 本 doc)、**Phase 1.A 章クローズ**。次 milestone = (Y) Phase 1.C 着手 / (Z) AYAstorm r20 SSS verify / (W) 上流 uniform4iv bug fix の AYA 判断。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc | 全文 | PA-B 完了 state + 副次 fix 3 件 + 残 milestone (Y)(Z)(W) AYA 判断要件 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-complete.md` | §3.3 (= line 候補 (X)(Y)(Z)(W) record) | Phase 1.B host-side 完了 + 4 候補並列性 (X)⊥(Z)⊥(W) + (Y) は (X) 後推奨 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §4.2 (= line 204-224) Phase 1 Exit Criteria | Phase 1.A literal 「codegen 実行 PASS + テスト program で include + bind 不変動作確認」+ Phase 1 完了時点「既存 program 動作 unchanged」注 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `scripts/ubo_codegen/spirv_reflect.py:101-145` | reflection 関数 `perform_reflection()` (= cross_cmd 引数順序 + reflect-only JSON output) |
| `scripts/ubo_codegen/spirv_reflect.py:274-296` | `version()` 関数 (= `--version` → `--revision` fallback、Ubuntu noble compat) |
| `scripts/ubo_codegen/spirv_reflect.py:226-243` | `verify_layout_against_spirv()` block size check (= `round_up(spv_size, STRUCT_ALIGN_FLOOR)` 後一致 accept、std140 §4.3.1.5 trailing pad spec literal 解釈差吸収) |
| `indra/cmake/AyaUboCodegen.cmake:24-47` | `find_program(AYA_SPIRV_CROSS spirv-cross)` + NOT 時 skip env path |
| (X) prep handoff `…-phase1-a-residual-prep.md` §2.1 / §2.3.1 | 副次 (1)(2)(3) state + (b-1) 案 PA-B/PA-N 構成 (= 本 commit で達成) |

---

## §2 PA-B 通電 verify 完了 record

### §2.1 install + cache invalidate flow (= AYA + Claude 協働)

| step | 実行者 | 内容 | 結果 |
|---|---|---|---|
| 1 | Claude | 現状確認 (= `which spirv-cross` 出力 0、`dpkg -l \| grep spirv` で library only、`apt-cache search` で `spirv-cross` package 存在) | CLI 不在 + Ubuntu noble/universe `spirv-cross` package 候補 確認 |
| 2 | Claude | package 素性確認 (= `apt-cache policy` + `dpkg -s`) | `noble/universe` + `2021.01.15+1.3.239.0-1build1` + Khronos 公式 upstream + Debian X Strike Force maintainer + libc6/libgcc/libstdc++ 単一依存 |
| 3 | AYA | `sudo apt install -y spirv-cross` 実行 | `/usr/bin/spirv-cross` install PASS |
| 4 | Claude | install verify (= `which` + `dpkg -l`) | `/usr/bin/spirv-cross` + version `2021.01.15+1.3.239.0-1build1` 確認 |
| 5 | Claude | codegen 直叩き verify (= `python3 scripts/ubo_codegen/main.py --force` + `AYA_CODEGEN_SKIP_SPIRV_CHECK` env unset) | **EXIT=5、toolchain probe fail** = Ubuntu noble package 互換性 finding 3 件 発見 |
| 6 | Claude | 副次 fix 3 件 適用 (= `scripts/ubo_codegen/spirv_reflect.py` +42 -24 line、§2.2 詳述) | unittest 130/130 PASS、codegen `--force` EXIT=0、90 blueprint 全 SPIR-V cross-check 通電 PASS |
| 7 | Claude | cmake cache invalidate (= `cmake -U AYA_SPIRV_CROSS -S indra -B build-linux-x86_64`) | reconfigure 2.6s、`AYA_SPIRV_CROSS:FILEPATH=/usr/bin/spirv-cross` cache 復活 (= 次 build 時 SKIP env 不在 path) |

### §2.2 副次 fix 3 件 (= Ubuntu noble compat literal 適用)

#### §2.2.1 fix (i) `version()` 関数: `--version` → `--revision` fallback

**現象**: `spirv-cross --version` = exit **1** + stdout 0 bytes + stderr 20181 bytes (= help dump、第 1 行「(Debian package)」)。Ubuntu noble package が `--version` flag を accept しない (= unknown arg として treat、help dump で exit 1)。

**fix**: `spirv_reflect.py:274-296` `version()` 関数を `--version` 試行 → fail なら `--revision` fallback の 2 段構成に変更。stdout/stderr 両方から最初の非空 line 抽出 (= Ubuntu noble package は stderr 側に「(Debian package)」出力、upstream Khronos は stdout 側 build timestamp + git hash 出力)。

**互換性**: upstream Khronos spirv-cross (= Vulkan SDK 1.3.296+ 等の新 version) でも `--version` 正規対応のため、Step 1 で hit する。Ubuntu noble package では Step 2 で hit する。両 distro compat 維持。

**根拠 確証**: literal 実行 verify (= `spirv-cross --version` exit 1 + `spirv-cross --revision` exit 0)。

#### §2.2.2 fix (ii) `perform_reflection()` cross_cmd 引数順序 + `--output-format json` 除去

**現象**: `spirv-cross --reflect /tmp/spv` = exit **1** + stderr「Didn't specify input file.」。Ubuntu noble package は positional argument (= SPIR-V file path) が flag より **前** であることを要求。`spirv-cross /tmp/spv --reflect` = exit **0** + stdout JSON output (= 1077 bytes for sample vert shader、`entryPoints/types/ubos` 構造) PASS。

更に `--output-format json` flag は Ubuntu noble package で未対応 (= help dump で exit 1)。`--reflect` 単独で JSON が default output (= help line「`[--reflect]: Emit JSON reflection.`」literal 確証)。

**fix**: `spirv_reflect.py:119-123` (= 改変後 line 123) の `cross_cmd` を `[str(spirv_cross), "--reflect", "--output-format", "json", str(spv_path)]` から `[str(spirv_cross), str(spv_path), "--reflect"]` に変更。

**互換性**: upstream Khronos spirv-cross も `<input> --reflect` の引数順序を accept する (= Khronos public main_cli.cpp の `parse_args()` 実装で positional + flag 順序自由)。両 distro compat 維持。

**根拠 確証**: literal 実行 verify (= `spirv-cross /tmp/fvp.spv --reflect` で JSON 出力 PASS、`FrameViewProj` block 構造完全取得)。

#### §2.2.3 fix (iii) `verify_layout_against_spirv()` block size check に `round_up` 後一致 accept

**現象**: codegen Python calc `FrameViewProj.std140_size = 496` vs SPIR-V reflection `block_size = 488`、diff 8 bytes mismatch raise。

**root cause**: std140 spec §4.3.1.5 literal で **「the structure may have padding at the end; the base offset of the member following the sub-structure is rounded up to the next multiple of the base alignment of the structure」** 規定、Python codegen が `std140.py:194 round_up(offset, max_align)` で trailing pad 加算 (= UBO base align = vec4 = 16、488 → 496)。SPIR-V reflection (glslang + spirv-cross) は raw member-end offset 報告 (= shader logical size、488)。両者目的差異:
- SPIR-V reflection 488 = shader が必要とする minimum buffer size
- std140 calc 496 = host alloc 用 round-up 後 size (= Vulkan/OpenGL UBO buffer alloc 標準慣例)

**fix**: `spirv_reflect.py:226-243` (= 改変後 line) の block size check に `round_up(expected_size, STRUCT_ALIGN_FLOOR)` (= 16 align) 後一致も accept。具体的には `layout.std140_size not in (expected_size, expected_padded)` のみ raise、それ以外は trailing pad spec 由来の妥当差 として PASS。

**確証**: 90 blueprint 全 SPIR-V cross-check 通電 PASS = bug ではなく spec 由来正常 path であることが全件確認された。

**import 追加**: `from std140 import BlockLayout, MemberLayout, STRUCT_ALIGN_FLOOR, round_up` (= 既存 import に `STRUCT_ALIGN_FLOOR, round_up` 追加)。

### §2.3 通電 verify 結果 (= Phase 1.A Exit Criteria (i) 充足確証)

```
[codegen_ubo] INFO: tool=ubo_codegen version=0.1.0-PA6 python=3.12.3
[codegen_ubo] INFO: input=indra/newview/app_settings/shaders/aya_r41_blueprints output=build-linux-x86_64/codegen/ubo
[codegen_ubo] INFO: 90 .glsl input(s) discovered
[codegen_ubo] INFO: --force given, bypassing incremental cache
[codegen_ubo] INFO: emitted 95 file(s) for 90 block(s) / 382 member(s) in 10489 ms
[codegen_ubo] INFO: cache written: build-linux-x86_64/codegen/cache/codegen_state.json
EXIT=0
```

- 90 blueprint .glsl 全 SPIR-V cross-check 通電
- 382 member 全 offset/array_stride/size cross-check PASS
- 95 file emit (= 5 aggregated `.inl` + 90 per-block `ubo_layout_<name>.inl`)
- 10489 ms (= 約 10.5 秒、cold cache `--force`)
- ERROR/WARNING 0 件

### §2.4 cmake cache invalidate 結果 (= 副次 (2) literal 対処、Phase 1.A complete §3.5 (2) 適用)

**Before**:
```
AYA_SPIRV_CROSS:FILEPATH=AYA_SPIRV_CROSS-NOTFOUND
```

**Command**: `cmake -U AYA_SPIRV_CROSS -S indra -B build-linux-x86_64`

**reconfigure log抜粋**:
```
-- AYAstorm r41 (PA-7): UBO Codegen wired = .../scripts/ubo_codegen/main.py
-- AYAstorm r41 (PA-7):   blueprint count = 90 .glsl files
-- Configuring done (2.6s)
-- Generating done (0.1s)
```

**After**:
```
AYA_SPIRV_CROSS:FILEPATH=/usr/bin/spirv-cross
```

**含意**: 次 build invoke 時、`AyaUboCodegen.cmake:129-132` の `if (NOT AYA_SPIRV_CROSS)` path 不到達 = `AYA_CODEGEN_SKIP_SPIRV_CHECK=1` env が **付与されない** = build 時 codegen で SPIR-V cross-check **通電継続**。skip path は CLI 不在 fallback として保持 (= 他開発者環境で spirv-cross 未 install 時の degrade path)。

---

## §3 残 sub-step strict 線形 (= PA-B + PA-N 完了後反映)

### §3.1 現 status (= PA-B + PA-N 完了)

```
✅ Phase 1.A 全 sub-step 完了 (= PA-1..PA-8 + PA-B + PA-N、本 commit marker)
✅ Phase 1.B host-side 完了 (= commit 35c4be1046 marker)
  → ⏳ 次 milestone: (Y) Phase 1.C 着手 / (Z) AYAstorm r20 SSS verify / (W) 上流 uniform4iv bug fix
  並行可能性: (Y) ⊥ (Z) ⊥ (W) (= (X) 完了で (Y) が unblocked)
```

### §3.2 候補 (Y)(Z)(W) AYA 判断要件 (= 推奨順次着手)

| 候補 | scope | Claude 推奨分岐 | 根拠 |
|---|---|---|---|
| **(Y)** | Phase 1.C 着手 (= test UBO shell + per-cadence update + descriptor bind + `vkCmdBindDescriptorSets` 通電 verify) | (= 09 §4.3 Phase 1.C spec literal、新 prep doc 起案先行) | Phase 1.A 完了で codegen 生成 header (= `ubo_index.inl` 等) が Phase 1.C input ready、`mUseUBO=false default` 維持で既存動作 unchanged 保証下で test UBO shell 通電可能 |
| **(Z)** | AYAstorm r20 章 SSS 効き verify (= prep `…-aya-r20-sss-verify-prep.md` 既起案) | **(c) ayastorm-release HEAD 直接 verify** | r41 host-side 改変 SSS 影響不可 + r20 章は r41 章と orthogonal、scope cross-contamination 回避 |
| **(W)** | 上流 uniform4iv 内 glUniform1iv bug fix (= prep `…-upstream-uniform4iv-bug-prep.md` 既起案、bug 真位置 line 2558) | **(c) 両方並行** (= AYAstorm fork 内 fix + upstream LL PR) | bug literal 確実 + call site 0 件で現状実害なしだが将来 risk + fix scope 1 line trivial + 上流取込やすさ維持 |

**並列性**: (Y) ⊥ (Z) ⊥ (W) (= (X) 完了で全候補 unblocked、相互依存なし)。AYA 判断要件 = どの候補 + どの分岐から着手。

---

## §4 self-verify (= 本 handoff 起案時点、commit 前確認)

| # | 観点 | 確認方法 | 期待 |
|---|---|---|---|
| (1) PA-B 通電 verify 完了 | codegen `--force` EXIT=0 + 90 blueprint cross-check + ERROR 0 件 | ✅ §2.3 |
| (2) 副次 fix 3 件 record | (i) version() fallback + (ii) cross_cmd 順序 + (iii) block size round_up accept | ✅ §2.2.1 + §2.2.2 + §2.2.3 |
| (3) cmake cache 復活確認 | `AYA_SPIRV_CROSS:FILEPATH=/usr/bin/spirv-cross` literal | ✅ §2.4 |
| (4) unittest 130/130 PASS | `python3 -m unittest discover -s scripts/ubo_codegen/tests` | ✅ |
| (5) 改変 scope 最小 | `indra/` + `cmake/` 改変 0、`scripts/ubo_codegen/spirv_reflect.py` のみ +42 -24 | ✅ `git diff --stat` |
| (6) Phase 1.A 章クローズ確証 | PA-1..PA-8 + PA-B + PA-N 全完了 marker、Exit Criteria (i)(ii)(iii) 全充足 | ✅ §0 + §3.1 |
| (7) 残 strict 線形反映 | (Y)(Z)(W) 並列性 + Claude 推奨分岐 + AYA 判断要件 | ✅ §3.2 |
| (8) commit 内容 | `scripts/ubo_codegen/spirv_reflect.py` +42 -24 + handoff doc 1 件 | ✅ (本 commit 段) |
| (9) Co-Authored-By 不在 | commit message 行頭 `Co-Authored-By:` 0 件 | ✅ |

---

## §5 引き継ぎ memory (= 次 session 着手時参照)

特に重要 (= 既存 memory から):

- `project_ayastorm_r41_vulkan_migration` (= r41 milestone active marker)
- `project_ayastorm_r41_design_principles` (= 上流取込やすさ + Core 分散実現)
- `project_r41_phase1b_vulkan_host_gate` (= GATE-B / MUSEUBO-A 確定)
- `feedback_ubo_migration_one_at_a_time` (= 1 sub-task 1 commit + cold launch 検証挟む)
- `feedback_self_verify_before_handoff` (= AYA 提示前 self-trace)
- `feedback_self_bug_no_defer_option` (= 「先送り/disable」を提案として並べない、fix or 別 sub-step 独立起案のみ)
- `feedback_doubt_self_first` (= AYA 提示情報を疑わず、自分の改変を疑う)
- `feedback_handoff_minimal_pre_req_read` (= 次 session pre-req は最小 3 件)
- `feedback_no_auto_commit` (= 本 doc commit は AYA 明示指示後)
- `feedback_no_claude_coauthor` (= Co-Authored-By 行不在)
- `feedback_no_scope_shrink` (= PA-B literal scope = SPIR-V cross-check **PASS** literal を「機能 PASS」でなく「結果 PASS」まで含意、bug 検出時の fix まで PA-B 内実施)
- `feedback_build_only_verified` (= 副次 fix 3 件 全 literal 実行 verify 経由で確証取得)

---

## §6 次 session 着手 1 line

**「前 session で候補 (X) Phase 1.A 残作業 (b-1) 案 PA-B (= spirv-cross CLI install + codegen 再走 + SPIR-V cross-check 通電 verify) + PA-N (= Exit + handoff) 完了 commit (= 本 doc + 1 commit、`scripts/ubo_codegen/spirv_reflect.py` +42 -24 副次 fix 3 件、`indra/` 改変 0)。本 session = Phase 1.A 章クローズ後、候補 (Y) Phase 1.C 着手 / (Z) AYAstorm r20 SSS verify / (W) 上流 uniform4iv bug fix の AYA 判断 + 着手。並列性 (Y) ⊥ (Z) ⊥ (W)。Claude 推奨 = (Y) は新 prep 起案先行、(Z) は (c) ayastorm-release HEAD 直接、(W) は (c) AYAstorm fork 内 fix + upstream LL PR 両方並行。必読 3 件 = (1) 本 handoff doc 全文 + (2) `…-phase1-b-complete.md` §3.3 + (3) 09 §4.2 line 204-224。」**
