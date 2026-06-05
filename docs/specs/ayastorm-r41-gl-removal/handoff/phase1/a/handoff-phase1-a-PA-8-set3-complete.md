# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3 完了

**作成日**: 2026-06-04
**前 session commit**: `1d456eb04f` (= PA-8 set=2 完了、本 session entry 時点)
**本 session 物理出力** (= 全て **未 commit**、AYA さん明示指示後 batch commit):
- `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/` 配下 **54 .glsl file** 新規 (= 06a §3.4 `<Name>UBO_Legacy` literal extract、binding 0..62 with 9 gaps = 15/16/23/29/31/34/35/36/59)
- 本 handoff doc 新規

**次 session 着手**: **AYA 判断要なし** (= PA-8 set=3 完了で 85 UBO 全 blueprint 揃った) → **Phase 1.A Exit Criteria (iii) 実 indra/ build + viewer launch 確認** (= 09 §4.2 `bind 不変動作` literal、Linux first-class baseline で実施) → 完了で **Phase 1.A 全終了 marker** → Phase 1.B entry (= host C++ redirect 層着手)。前 entry handoff §3 PA-X 構成表通り strict 線形最終。

---

## §0 state 一行 summary

η-30 **Phase 1.A PA-8 set=3 完了 state**:
- **PA-8 set=3** = `aya_r41_blueprints/set3/` 配下 **54 file** literal extract from `class*/...` 既存 `#ifdef LL_VULKAN_GLSL` block。06a §3.4 表通り 54 distinct `<Name>UBO_Legacy` 名 + binding 番号 0..62 with 9 gaps (= 15/16/23/29/31/34/35/36/59) を 1 UBO 1 file 規約で配置。
- **smoke 3 path PASS** (= /tmp/aya_ubo_pa8_set3_smoke/) = Run #1 miss 5125ms + 95 file emit (= 5 aggregated + 90 per-block layout = set=0 3 + set=1 2 + set=2 31 + set=3 54) / Run #2 hit 0.04s (= Python 起動なし、`[100%] Built target codegen_ubo` 再 build skip) / Run #3 --force 5159ms + 95 file 再 emit
- **metadata 全 90 UBO 正しい** (= `g_block_count=90u` / 0 hash collision / 382 member 合計 / binding 分布 = 前 phase 36 + set=3 54 @ binding 0..62 unique with 9 gaps)
- **C++17 standalone compile + 90/90 UBO lookup PASS** (= sanity_check.cpp で prior[5] + set2[31] + set3[54] 全 Expected entry の descriptor_set + binding 一致 assert + set=2/set=3 cadence_tag=1 (PerProgram default fallback) assert + UniformLocation{} link OK = `./sanity_check` 出力 = `"PASS: 90/90 UBO lookup OK (set=0 3 + set=1 2 + set=2 31 + set=3 54)"`)
- **multi-site UBO 3 件全 identical 確認** (= P-1 protocol 適用、Agent 並列 + 直接 Read で二重 verify、§2.2 詳細)
- **unittest 127/127 + py_compile 8/8 PASS** (= 既存 PA-8 set=2 baseline 維持、本 session 新規 test 追加なし)
- **macro literal 置換 0 件** (= P-5 protocol 確認、set=3 `<Name>UBO_Legacy` 群は compile-time macro 不使用、literal 値のみで構成)

**設計判断 0 件** (= AYA 判断項なし、§2.7 protocol P-1〜P-5 で 54 件全件処理 + 各 UBO は single-name = 共存問題なし + multi-site 3 件全 identical、判断分岐発生せず)。

**残 1 件 未確定 (= 副次 finding、Phase 1.B 行き)**: set=3 54 UBO 全 cadence_tag が **1 (PerProgram default fallback)** = `main.py:65-72` `_PREFIX_TO_CADENCE` 表が `Frame*` / `Program_*` / `Draw_*` / `Asset_*` / `Skin_*` / `Global_*` の 6 prefix のみ列挙、AYAstorm naming `<Name>UBO_Legacy` は **match せず default 1=PerProgram に fallback**。前 phase (set=2) 既知 finding と同性質、blocker でない (= Phase 1.A Exit Criteria literal 充足、bind 不変 = host code 無関与)。Phase 1.B host wiring で `_PREFIX_TO_CADENCE` 拡張 (= main.py 3-5 line + unittest 2-3 件追加) で解消、詳細 = §2.6。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-PA-8-set3-complete.md`) | 全文 | PA-8 set=3 完了 state + Exit Criteria (iii) viewer launch protocol + Phase 1.B entry 直前 副次 task 候補 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-entry.md` | §3 PA-X 構成表 + §2 Phase 1.A scope + Exit Criteria | Phase 1.A 全体像 + Exit Criteria (iii) scope literal 参照 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §4.2 Phase 1.A Exit Criteria literal + §5 Phase 1.B entry scope | Phase 1.A 全終了判定 + Phase 1.B host C++ redirect 層 entry scope |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` | §3.1 std140 strict 順守判断 A + §5.1 4 file 分割契約 |
| `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md` | §3.4 set=3 54 UBO 表 (= 本 session 著作 source-of-truth、Phase 1.B host wiring でも参照) |
| `scripts/ubo_codegen/main.py:65-72` | `_PREFIX_TO_CADENCE` 表 (= Phase 1.B entry 直前 副次 task 候補、§2.6 詳細) |
| `indra/cmake/AyaUboCodegen.cmake:56-58` | BLUEPRINT_DIR = `…/aya_r41_blueprints` (= 本 session 不変、Phase 1.B でも同) |
| `indra/newview/app_settings/shaders/aya_r41_blueprints/set{0,1,2,3}/` | 全 90 .glsl (= set=0 3 + set=1 2 + set=2 31 + set=3 54) = Phase 1.B host wiring の literal source |
| `indra/newview/CMakeLists.txt` + `indra/llrender/CMakeLists.txt` | Exit Criteria (iii) 実 build で `aya_attach_ubo_codegen(<target>)` 動作確認の target |

---

## §2 本 session 成果

### §2.1 PA-8 set=3 真 scope 達成

| 出力契約 | 実装箇所 | 内容 |
|---|---|---|
| `aya_r41_blueprints/set3/<lowercase_name>.glsl` 54 file | 同 path | 06a §3.4 表通り 54 distinct `<Name>UBO_Legacy` literal extract from `class*/...` `#ifdef LL_VULKAN_GLSL` block、binding 0..62 with 9 gaps |
| smoke 3 path PASS | `/tmp/aya_ubo_pa8_set3_smoke/` 独立 cmake project | Run #1 miss 5125ms + 95 file emit (= 5 aggregated + 90 per-block layout) / Run #2 hit 0.04s (= Python 起動なし) / Run #3 --force 5159ms + 95 file 再 emit |
| metadata 全 90 UBO 正しい | `/tmp/aya_ubo_pa8_set3_smoke/build/codegen/ubo/ubo_metadata.inl` | `g_block_count=90u` / 0 hash collision / 382 member 合計 / binding 分布 = set=0 3 (0/1/2) + set=1 2@0 共存 + set=2 31 (binding=0 6 共存 + 1..25 単一) + set=3 54 @ binding 0..62 unique with 9 gaps (= 15/16/23/29/31/34/35/36/59) |
| C++17 standalone compile + 90/90 lookup assert PASS | `/tmp/aya_ubo_pa8_set3_smoke/sanity_check.cpp` | 全 90 UBO の descriptor_set + binding 一致 + set=2 31 + set=3 54 件 cadence_tag=1 (PerProgram default fallback) assert PASS + UniformLocation{} link OK = `./sanity_check` 出力 = `"PASS: 90/90 UBO lookup OK (set=0 3 + set=1 2 + set=2 31 + set=3 54)"` |
| unittest 全件 PASS | `python3 -m unittest discover -s scripts/ubo_codegen/tests -v` | **127/127 PASS** (= PA-8 set=2 baseline 維持、本 session 新規 test なし) / py_compile 8/8 PASS |

### §2.2 multi-site UBO verify (= 3 件 = §2.7 P-1 protocol 適用後 identical 確認)

set=3 UBO で複数 site 宣言ある 3 件は Agent 並列 Read + 直接 Read で完全一致確認 (= `feedback_doubt_self_first` 適用、PA-8 set=1/set=2 と同流二重 verify):

| UBO 名 | site 数 | site list | 結果 |
|---|---|---|---|
| `ShadowUtilParamUBO_Legacy` | 2 | `class1/deferred/shadowUtil.glsl:80` + `cinematic_bd/class1/deferred/shadowUtil.glsl:105` | identical (= 12 member) |
| `WaterVParamUBO_Legacy` | 2 | `class1/environment/waterV.glsl:61` + `class3/environment/waterF.glsl:108` | identical (= 6 member) |
| `CloudsVParamUBO_Legacy` | 2 | `class1/deferred/cloudsV.glsl:106` + `class1/deferred/cloudsF.glsl:79` | identical (= 4 member) |

残 51 UBO は single site で divergence 発生せず。

### §2.3 blueprint 著作 protocol confirm (= §2.7 P-1〜P-5 を 54 UBO に scale)

PA-8 set=1/set=2 で確立した protocol を 54 UBO に適用:

1. **06a inventory §3.4 で 54 UBO 名 + binding 確認** (= 0..62 with 9 gaps = 15/16/23/29/31/34/35/36/59)
2. **`grep "uniform [A-Za-z]*UBO_Legacy\s*{"` で site 数取得 + literal 確認** (= 54 distinct 名 + 3 件 multi-site = 計 57 site)
3. **Agent 並列 Read で全 multi-site member 列一致確認** (= 3/3 identical、直接 Read で 6 file 二重 verify = `feedback_doubt_self_first` 適用)
4. **canonical source 確定 + literal extract** (= 全 54 件)
5. **blueprint file format 規約適用** (= `#version 450` + `layout(std140, set=3, binding=M) uniform <Name>_Legacy {...};` + `void main() {}` + comment header に source + sample sites + spec ref)
6. **smoke 3 path + C++17 compile + assert** (= /tmp/aya_ubo_pa8_set3_smoke/ で再現)

### §2.4 設計判断 0 件 (= AYA 判断項なし、自走完了)

PA-8 set=2 では 6 UBO @ binding=0 共存に対し option I/II/III の AYA 判断項が発生したが、set=3 は:
- **共存問題なし** = 54 UBO 全 single-name + binding 0..62 with 9 gaps = 1 binding 1 UBO の純粋構造
- **divergence なし** = 3 件 multi-site 全 identical (= §2.2)
- **macro literal 不使用** = 全 54 UBO は固定 array dim + literal vec/mat のみで構成 (= §2.5)

ゆえに本 session は AYA 判断分岐なしで完走。

### §2.5 設計差分 0 件 (= macro literal 置換なし、技術判断項なし)

PA-8 set=2 では `MAX_JOINTS_PER_MESH_OBJECT` × 2 + `LIGHT_COUNT` × 1 の 3 件 macro literal 置換が発生したが、set=3 `<Name>UBO_Legacy` 群は **compile-time macro 不使用** = 全 member 列が literal 値のみ (= `mat4 shadow_matrix[6]` の 6 等は literal int constant) で構成、置換 protocol P-5 未発動。

### §2.6 副次 finding (= 次 session 自走可、AYA 判断不要、Phase 1.B 行き = PA-8 set=2 §2.6 と同性質、累計記録)

set=3 54 UBO 全 cadence_tag が **1 (PerProgram default fallback)** = `main.py:65-72` `_PREFIX_TO_CADENCE` 表が `Frame*` / `Program_*` / `Draw_*` / `Asset_*` / `Skin_*` / `Global_*` の 6 prefix のみ列挙、`<Name>UBO_Legacy` (= AYAstorm naming) は **match せず default 1=PerProgram に fallback**。set=2 で既知の同性質 finding (= `PerDrawUBO_*` / `PerProgramUBO_*` も match せず default fallback) の延長。

**累計**: set=2 31 件 + set=3 54 件 = **85 UBO 全 default fallback** (= 動作上は cadence=PerProgram で扱われる、Phase 1.A bind 不変動作には無関係)。

**影響範囲**: Phase 1.A Exit Criteria literal 充足 (= bind 不変 = host code 無関与、metadata の cadence_tag field は Phase 1.B redirect 層の dispatch hint として使う想定だが、Phase 1.A では unused)。

**Phase 1.B 行き fix path (= AYA 判断不要、技術判断のみ)**:
- **a) (推奨)** `_PREFIX_TO_CADENCE` 表に `PerDrawUBO_` → 2 (= Draw) / `PerProgramUBO_` → 1 (= Program) / `UBO_Legacy` suffix-match → 1 (= Program、PA-8 set=2 設計判断通り Legacy は per-program 切替) を追加 = main.py 3-5 line fix + unittest 2-3 件追加 (= PA-7.6 set/binding forward fix + PA-8 set=2 §2.6 提示と同流の trivial fix)
- **b)** block name suffix-based cadence 推定 (= `_F`/`_V` suffix で stage 推定) = 不採用、cadence と stage は直交
- **c)** 06a inventory に cadence 明示記述 + AYAstorm 独自 GLSL annotation pragma 導入 (= 設計 phase 行き) = 不採用、a) で十分

**本 handoff 時点判定 = 次 session Phase 1.B entry 直前 (or 直後) に a) で 1 commit、PA-7.6 fix と同 weight の small task**。

### §2.7 引き継ぐべき protocol (= Phase 1.B/1.C/2 でも literal 再利用可能)

PA-8 set=1/set=2/set=3 で確立した protocol、本 session で 54 UBO scale で再確認、blueprint 著作系作業で literal 再利用:

**(P-1)** divergence 検出 protocol: `grep "uniform <Name>\s*{"` で site 列挙 → Agent 並列 Read で member 列比較 → 直接 Read で二重 verify → divergence 発覚なら AYA 判断仰ぐ (= 本 session で 3 multi-site UBO 全 identical 確認、divergence なし)

**(P-2)** binding 一意性 self-verify: smoke 後 metadata で `(set, binding, subset)` 組の重複を確認 = 06a inventory entry と数一致 (= 本 session = set=3 54 件が emit metadata の entry 数と一致、set=3 内 binding 0..62 unique with 9 gaps 確認)

**(P-3)** cadence_tag 自動推定確認: block name prefix で `_PREFIX_TO_CADENCE` (= `main.py:65-72`) が機能してるか smoke 後確認 = §2.6 累計 set=2+set=3 85 件 全 default 1 fallback、Phase 1.B 行き (= blocker でない)

**(P-4)** hash collision check: `ubo_perfect_hash.inl` の `g_chd_values[N]` 配列が member total に対し perfect (= 0 collision) であること = 本 session 90 UBO で確認

**(P-5)** macro literal 置換 protocol (= PA-8 set=2 追加): blueprint に compile-time macro 出現時は (a) source 出典の `#define` / `constexpr` 確定 → (b) permutation なし固定値ならその値、permutation ありなら max 値採用 → (c) comment header に macro 名 + 出典 + 採用値 + 理由明記 (= 本 session 未発動)

---

## §3 次 session 着手 (= Phase 1.A Exit Criteria (iii) viewer launch 確認 + Phase 1.B entry)

### §3.1 着手 1 line

「前 session で PA-8 set=3 (= aya_r41_blueprints/set3/ 配下 54 file literal extract = 06a §3.4 全 `<Name>UBO_Legacy`、binding 0..62 with 9 gaps = 15/16/23/29/31/34/35/36/59) 完了 + smoke 3 path + C++17 compile + 90/90 lookup assert PASS + unittest 127/127 + py_compile 8/8 PASS + multi-site 3 件 identical 確認。本 session = **Phase 1.A Exit Criteria (iii) 実 indra/ build + viewer launch + bind 不変動作確認** (= 09 §4.2 literal、Linux first-class baseline) → 完了で **Phase 1.A 全終了 marker** → batch commit (= set3/ 54 file + 本 handoff doc + Phase 1.A 終了 commit) → **Phase 1.B entry** = host C++ redirect 層着手 (= entry handoff §4 + 09 §5 参照、副次 task `_PREFIX_TO_CADENCE` 拡張は entry 直前 or 直後)。」

### §3.2 残 sub-step scope (= strict 線形最終)

| sub-step | 件数 | 出力 | source ref | 完了条件 |
|---|---|---|---|---|
| **Exit Criteria (i)** | - | codegen 実行 PASS | 09 §4.2 | 本 session で smoke 3 path PASS = **✅ 既達成** (= §2.1) |
| **Exit Criteria (ii)** | - | 既存 program 1 個 include | 09 §4.2 | 本 session で sanity_check.cpp の `#include "codegen/ubo/ubo_index.inl"` 経由 90 UBO metadata lookup PASS = **✅ 既達成** (= §2.1) |
| **Exit Criteria (iii)** | - | bind 不変動作確認 (実 build + viewer launch) | 09 §4.2 | 実 indra/ build + viewer launch + 既存 GL setUniform 動作不変 (= shader runtime fail せず launch 可能) = **次 session 着手 scope** |
| **Phase 1.A 全終了 marker** | - | batch commit + memory update | - | (iii) PASS 後 set3/ 54 file + handoff + 終了 commit 一括 = **AYA 明示指示後** |
| **Phase 1.B entry** | - | host C++ redirect 層着手 | 09 §5 + entry handoff §4 | Phase 1.B PB-1 (= UBO redirect 層 dummy VkBuffer per binding 配線) entry handoff 起案 = **次次 session 以降** |

### §3.3 Exit Criteria (iii) viewer launch 確認 protocol

1. **build 前 self-verify**: `git status` で本 session 出力 (= set3/ 54 file + handoff) のみ未 commit + indra/ 他改変なし確認
2. **`project_build_procedure` 参照** = configure → build → install → cache clear のフルフロー (= memory 既登録、`feedback_build` で全権実行可)
3. **build phase**: `indra/cmake/AyaUboCodegen.cmake` で `codegen_ubo` target が build 中に起動 + `aya_attach_ubo_codegen(llrender)` 経由で llrender target が depend = 90 UBO metadata header 生成 (= `${CMAKE_BINARY_DIR}/codegen/ubo/ubo_*.inl`)
4. **build PASS 条件**: build 全 PASS (= shader compile error 不在 + linker error 不在 + codegen header include による既存 .cpp C++ compile error 不在)
5. **viewer launch + bind 不変動作確認**: `~/ayastorm/` に install + cache clear + 起動 = 既存 GL `glUniform*` / `glBindBufferBase` call site が runtime で fail せず viewer 通常動作 (= Phase 1.A literal: codegen header は include されるが既存 bind は不変)
6. **AYA 立ち合い必要** = 実 viewer 起動は AYA さん操作 + 動作目視 (= `feedback_build` で Claude 実行可だが、起動後の動作目視は AYA 必須)
7. **PASS 後 batch commit**: AYA 明示指示後、set3/ 54 file + handoff + Phase 1.A 全終了 marker (= 適切 commit message で literal) 一括

### §3.4 Phase 1.A 全終了後 直 = Phase 1.B entry

Phase 1.B = host C++ redirect 層 (= 既存 `glUniform*` call site を維持しつつ、内部で codegen metadata に基づき UBO buffer を per-binding point 用意し、Vulkan side で descriptor set 更新する shim 層) 着手。詳細 = 09 §5 + entry handoff §4。

**Phase 1.B entry 直前 or 直後 副次 task 候補**: §2.6 `_PREFIX_TO_CADENCE` 拡張 = main.py 3-5 line + unittest 2-3 件 = PA-7.6 fix と同 weight (= 30 min 程度)、Phase 1.B 着手前に独立 commit 1 件で済む。

### §3.5 Exit Criteria (iii) 時の注意

- **3 OS 統一 scope 外**: Phase 1.A Exit Criteria literal 充足は **Linux first-class baseline 1 platform** で十分 (= 09 §4.2、3 OS 統一は別 phase = Phase 1.B/1.C/2 完了後の release 直前 phase)
- **`build_only_verified` 適用**: viewer 起動目視 PASS 前に Phase 1.A 完了 commit しない、PASS 後にのみ AYA 明示指示で commit
- **`feedback_remove_verification_logs` 適用**: build/viewer launch 検証中に追加した LL_INFOS hook あれば commit 前に除去 (= 本 session 追加 hook なし、Phase 1.A 完了 commit は blueprint + handoff のみで literal)
- **`feedback_restore_debug_settings` 適用**: viewer launch 検証で debug settings 一時変更があれば AYA に「戻す値表」提示

---

## §4 self-verify (= 9 観点、本 handoff 起案時点)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) `aya_r41_blueprints/set3/` 配下 54 file 物理存在 | `ls .../set3/ \| wc -l` = 54 | ✅ |
| (2) 全 54 file は `#version 450` + `layout(std140, set=3, binding=N) uniform <Name>_Legacy {...}` + `void main() {}` の固定 format | 各 file 物理 Read で確認 (= 3 sample: water_v / shadow_util / clouds_v、binding 値は 06a §3.4 表通り) | ✅ |
| (3) PA-8 set=3 smoke 3 path PASS | `/tmp/aya_ubo_pa8_set3_smoke/` で miss 5125ms + hit 0.04s + --force 5159ms、95 file emit (= 5 aggregated + 90 per-block layout) | ✅ |
| (4) metadata 全 90 UBO 正しい + 0 hash collision + 382 member | `ubo_metadata.inl` で `g_block_count=90u`、binding 分布 = set=0 3 + set=1 2@0 + set=2 31 (6@0 + 25@1..25) + set=3 54 @ 0..62 with 9 gaps、`g_chd_values[N]` で 0 collision | ✅ |
| (5) C++17 standalone compile + 90/90 lookup assert PASS | `sanity_check.cpp` で全 UBO descriptor_set + binding 一致 + set=2/set=3 cadence_tag=1 assert PASS、`./sanity_check` 出力 = `"PASS: 90/90 UBO lookup OK (set=0 3 + set=1 2 + set=2 31 + set=3 54)"` | ✅ |
| (6) multi-site UBO 3 件全 identical 確認 | Agent 並列 + 直接 Read 6 file で二重 verify、divergence なし (= `feedback_doubt_self_first` 適用) | ✅ |
| (7) macro literal 置換不要確認 (= P-5 未発動) | 54 UBO 全 member 列 literal 値構成、compile-time macro 不使用 | ✅ |
| (8) unittest 127/127 + py_compile 8/8 PASS | `python3 -m unittest discover` + `python3 -m py_compile` 全 PASS、PA-8 set=2 baseline 維持 | ✅ |
| (9) git working tree 状態 = `aya_r41_blueprints/set3/` untracked + handoff doc 新規、indra/ 他改変なし + scripts/ 改変なし + Co-Authored-By: Claude 行不在 + handoff doc 命名対称 (= 前 handoff `…-PA-8-set2-complete.md` と対称で `…-PA-8-set3-complete.md`) | `git status` で確認 (= `?? indra/newview/app_settings/shaders/aya_r41_blueprints/set3/` のみ + 本 handoff doc) | ✅ |

---

## §5 引き継ぎ済 memory (= 次 session で active)

- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ
- `feedback_design_phase_no_code_write` — 解禁済 (Phase 1.A 実装 phase 継続、Phase 1.B 着手後も実装 phase)
- `feedback_no_scope_shrink` — set=3 54 件全著作、part-of で済まさない (= 本 session 厳守済)
- `feedback_self_verify_before_handoff` — 本 session 発動 (= §2.6 cadence default fallback 累計記録 + Phase 1.B 副次 task 候補 surface)
- `feedback_no_claude_coauthor` — 本 handoff doc 含め全 commit 共著行不在
- `feedback_one_step_at_a_time` — set=3 著作中 AYA 判断項発生せず (= §2.4)、推測実装回避は P-1〜P-5 protocol 自走で literal 充足
- `feedback_doubt_self_first` — 本 session 発動 (= Agent multi-site member 一致確認結果を直接 Read 6 file で二重 verify、PA-8 set=1/set=2 と同流)
- `feedback_proactive_handoff` — 本 session 発動 (= PA-8 set=3 完了で次 session に handoff)
- `feedback_no_auto_commit` — 本 handoff doc + set3/ 54 file は AYA 明示指示後 batch commit
- `feedback_remove_verification_logs` — 本 session 追加 log/diagnostic 不在 (= 該当なし、blueprint + handoff のみ)
- `feedback_build_only_verified` — Exit Criteria (iii) viewer launch PASS 前に Phase 1.A 完了 commit しない
- `feedback_tests_dir_never_commit` — 本 session test 新規追加なし (= PA-8 set=2 baseline 維持)
- `feedback_restore_debug_settings` — Exit Criteria (iii) 時の debug settings 注意点 (§3.5)
- `feedback_build` — Exit Criteria (iii) full build フローは Claude 全権実行 (= `project_build_procedure` 参照)
- `project_build_procedure` — Exit Criteria (iii) で参照 (= configure → build → install → cache clear)
- `project_ayastorm_r41_vulkan_migration` — PA-8 set=3 完了 milestone = Phase 1.A 全終了直前
- `project_ayastorm_r41_design_principles` — Vulkan std140 layout-compat 慣用 (= larger buffer に smaller view) は本 session 未発動 (= macro literal 置換なしゆえ)
- `feedback_ubo_migration_one_at_a_time` — set=3 54 件を本 session で完了、Phase 1.B host wiring は別 session
- `project_ayastorm_three_platforms` — Exit Criteria (iii) は Linux first-class baseline 1 platform で literal 充足 (= 3 OS 統一は Phase 1.B/1.C/2 完了後の release 直前 phase)
- `feedback_use_agents_proactively` — set=3 で Agent 並列 member 確認実施 (= Agent 結果を直接 Read で 2 重 verify、`feedback_doubt_self_first` 適用 PASS)

---

## §6 次 session 着手 1 line

**「前 session で PA-8 set=3 (= aya_r41_blueprints/set3/ 配下 54 file literal extract from `class*/...` ifdef LL_VULKAN_GLSL block = 06a §3.4 全 `<Name>UBO_Legacy`、binding 0..62 with 9 gaps = 15/16/23/29/31/34/35/36/59) 完了 + smoke 3 path + C++17 compile + 90/90 lookup assert PASS + unittest 127/127 + py_compile 8/8 PASS + multi-site 3 件 identical 確認。本 session = **Phase 1.A Exit Criteria (iii) 実 indra/ build + viewer launch + bind 不変動作確認** (= 09 §4.2 literal、Linux first-class baseline、`project_build_procedure` フル実行) → 完了で **Phase 1.A 全終了 marker** → AYA 明示指示後 batch commit (= set3/ 54 file + 本 handoff doc + Phase 1.A 終了 commit message) → **Phase 1.B entry** = host C++ redirect 層着手 (= 09 §5 + entry handoff §4)。副次 task = §2.6 `_PREFIX_TO_CADENCE` 拡張 = main.py 3-5 line + unittest 2-3 件、Phase 1.B entry 直前 or 直後 独立 commit (= PA-7.6 fix と同 weight)。AYA 判断不要、Claude 自走可 (= 起動目視のみ AYA 立ち合い)。」**
