# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-7.6 + PA-8 set=1 完了

**作成日**: 2026-06-04
**前 session commit**: `74618b2c2e` (= PA-7.5 + PA-8 set=0 完了、本 session entry 時点)
**本 session 物理出力** (= 全て **未 commit**、AYA さん明示指示後 batch commit):
- `scripts/ubo_codegen/main.py` MOD = `_ubo_to_block_spec()` に `descriptor_set` / `binding` を `layout_qual` から forward (3 line 追加)
- `scripts/ubo_codegen/tests/test_main.py` 新規 (= local-only、`.gitignore` 配下、`feedback_tests_dir_never_commit` 準拠で commit 対象外)
- `indra/newview/app_settings/shaders/aya_r41_blueprints/set1/material_ubo.glsl` 新規 (= 10-member PBR-extended canonical = AYA option I 確定)
- `indra/newview/app_settings/shaders/aya_r41_blueprints/set1/material_ubo_legacy.glsl` 新規 (= 8-member、materialF.glsl singleton)
- 本 handoff doc 新規

**次 session 着手**: **AYA 判断要なし** (= PA-7.6 defect closure 済 + set=1 確定済) → **PA-8 set=2 (= 26 UBO 著作)** 着手。前 session entry §3 PA-8 row literal scope 通り strict 線形継続。

---

## §0 state 一行 summary

η-30 **Phase 1.A PA-7.6 完了 + PA-8 set=1 完了 state**:
- **PA-7.6** = 前 session 発見 defect closure = `main.py:189-194` `_ubo_to_block_spec()` に `descriptor_set=int(ubo.layout_qual.get("set", 0))` + `binding=int(ubo.layout_qual.get("binding", 0))` 2 line 追加 = set=0 set=1 全 5 UBO metadata に正しい binding が書かれる (= FrameViewProj 0/0, FrameLights 0/**1**, FrameAtmosphere_Lighting 0/**2**, MaterialUBO 1/0, MaterialUBO_Legacy 1/0) / unittest 125 → **127/127 PASS** (= 新規 `test_main.py` UboToBlockSpecTests 2 test 追加 = `test_set_and_binding_forwarded_when_present` + `test_set_and_binding_default_to_zero_when_absent`)
- **PA-8 set=1** = `aya_r41_blueprints/set1/material_ubo.glsl` (= 10-member PBR-extended、source `class1/deferred/pbropaqueF.glsl:44` literal extract、verified identical 4 PBR sites = pbropaqueF/V + pbralphaF/V) + `material_ubo_legacy.glsl` (= 8-member、source `class3/deferred/materialF.glsl:38` singleton 1 site) = blueprint 2 file = 06a §3.2 inventory entry count 一致
- **smoke 3 path PASS** (= /tmp/aya_ubo_pa8_set1_smoke/) = miss 298ms / hit 47ms / --force 293ms / 10 file emit (= 5 aggregated + 5 per-block layout) / 5 block / 56 member / 0 hash collision (= g_chd_values[61])
- **C++17 standalone compile + 5/5 UBO lookup + cadence_tag assert PASS**

**設計判断 1 件 (= AYA option I 確定、必ず引き継ぐ)**: 49 file 中 4 PBR site が **10-member MaterialUBO** を、残り 45 site が **6-member baseline MaterialUBO** を同 `(set=1, binding=0, std140)` で宣言 = 06a §3.2 の 2 entry 記載とは別の divergence。AYA 判断 = **option I = 10-member full を canonical blueprint** = host C++ 側 alloc は full size、45 baseline shader は trailing 4 member 未参照 view (= Vulkan std140 layout-compat 慣用 = larger buffer に smaller block view 合法、既存 OpenGL で proven、shader 改修ゼロ堅持)。詳細 = §2.4。

**残 1 件 未確定 (= 次 session 自走可、AYA 判断不要)**: MaterialUBO と MaterialUBO_Legacy が metadata 上 `(set=1, binding=0, subset=0)` で完全同位 = GLSL qualifier に subset 概念がない → Phase 1.B host wiring で name 経由 dispatch する想定 (= 06a §3.2 所見「program ごとに片方のみ宣言」運用)、Phase 1.A scope では blocker でなし、set=2/3 著作後の Exit Criteria 検証時に再判定可。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-PA-7-6-and-PA-8-set1-complete.md`) | 全文 | PA-7.6 + PA-8 set=1 完了 state + AYA option I 判断 + 残 sub-step 線形 + subset 同位 finding |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-entry.md` | §3 PA-X 構成表 + §2 Phase 1.A scope + Exit Criteria | Phase 1.A 全体像 + PA-8 sub-task scope literal 参照 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md` | §3.3 set=2 (= 26 件 PerDrawUBO_* / PerProgramUBO_*) + §3.4 set=3 (= 54 件 *_Legacy) | 残 80 UBO の binding 番号 + cadence + source 既存配置 (= literal extract source 候補) |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §4.2 Phase 1.A Exit Criteria literal (= "85 UBO blueprint codegen 実行 PASS + 生成 header をテスト program で include + bind 不変動作確認") |
| `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` | §3.1 std140 strict 順守判断 A + §5.1 4 file 分割契約 |
| `scripts/ubo_codegen/main.py:189-196` | `_ubo_to_block_spec()` PA-7.6 fix 後の現状 (= `descriptor_set` / `binding` forward 済) |
| `indra/cmake/AyaUboCodegen.cmake:56-58` | BLUEPRINT_DIR = `…/aya_r41_blueprints` (= 本 session 不変、PA-8 set=2/3 でも同) |
| `indra/newview/app_settings/shaders/aya_r41_blueprints/set0/` + `set1/` | 既存 5 file (= set=0 3 + set=1 2) = set=2/3 著作テンプレ |

---

## §2 本 session 成果

### §2.1 PA-7.6 真 scope 達成

| 出力契約 | 実装箇所 | 内容 |
|---|---|---|
| `main.py:189-196` set/binding forward fix | `scripts/ubo_codegen/main.py:189-196` | `_ubo_to_block_spec()` に `descriptor_set=int(ubo.layout_qual.get("set", 0))` + `binding=int(ubo.layout_qual.get("binding", 0))` 2 line 追加 |
| unittest 1-2 件追加 | `scripts/ubo_codegen/tests/test_main.py` 新規 | `UboToBlockSpecTests` class = `test_set_and_binding_forwarded_when_present` (= layout_qual に set/binding 持つ場合の forward 確認) + `test_set_and_binding_default_to_zero_when_absent` (= push_constant 等で set/binding 不在時の 0 default 確認) |
| unittest 全件 PASS | `python3 -m unittest discover -s tests -t .` | **127/127 PASS** (= 既存 125 + 新規 2) / py_compile 8/8 PASS |
| smoke 3 path PASS | `/tmp/aya_ubo_pa7_6_smoke/` 独立 cmake project | Run #1 miss 171ms + 8 file emit (= 5 aggregated + 3 per-block layout) / Run #2 hit 49ms / Run #3 --force 187ms + 8 file 再 emit |
| set=0 metadata binding 検証 | `/tmp/aya_ubo_pa7_6_smoke/build/codegen/ubo/ubo_metadata.inl` | FrameViewProj 0/0 + FrameLights 0/**1** + FrameAtmosphere_Lighting 0/**2** = α fix で binding 値正しく forward (前 session は全 0/0) |
| C++17 standalone compile + runtime assert | `/tmp/aya_ubo_pa7_6_smoke/sanity_check.cpp` | 3 UBO lookup + binding/member_count assert 全 PASS |

### §2.2 PA-8 set=1 真 scope 達成

| 出力契約 | 実装箇所 | 内容 |
|---|---|---|
| `aya_r41_blueprints/set1/material_ubo.glsl` 新規 | 同 path | 10-member PBR-extended (= base 6 + metallicFactor + roughnessFactor + 2 pad)、source = `class1/deferred/pbropaqueF.glsl:44`、verified identical 4 PBR sites = pbropaqueF / pbropaqueV / pbralphaV / class2/pbralphaF (= 4/4 site member 列完全一致確認) |
| `aya_r41_blueprints/set1/material_ubo_legacy.glsl` 新規 | 同 path | 8-member、source = `class3/deferred/materialF.glsl:38` (= singleton 1 file のみ宣言) |
| smoke 3 path PASS | `/tmp/aya_ubo_pa8_set1_smoke/` 独立 cmake project | Run #1 miss 298ms + 10 file emit (= 5 aggregated + 5 per-block layout) + 56 member / Run #2 hit 47ms / Run #3 --force 293ms + 10 file 再 emit |
| metadata 全 5 UBO 正しい | `/tmp/aya_ubo_pa8_set1_smoke/build/codegen/ubo/ubo_metadata.inl` | `MaterialUBO` set=1/binding=0/cadence=1(PerProgram)/size=256B/10 member + `MaterialUBO_Legacy` set=1/binding=0/cadence=1(PerProgram)/size=256B/8 member (= block name `Material*` prefix で cadence=1 自動推定 PASS) |
| 0 hash collision | `ubo_perfect_hash.inl` | g_chd_values[61] (= 56 member + perfect hash 余裕 slot)、衝突なし |
| C++17 standalone compile + 5/5 lookup assert PASS | `/tmp/aya_ubo_pa8_set1_smoke/sanity_check.cpp` | 全 5 UBO の descriptor_set / binding / member_count / cadence_tag assert PASS |

### §2.3 blueprint 著作 protocol confirm (= 本 session で set=0 protocol を set=1 で再現)

set=1 著作で実施した step (= set=2/3 でも同 protocol):

1. **06a inventory §3.X で UBO 名 + binding 確認** (= 本 session = §3.2 で MaterialUBO @ 1/0 + MaterialUBO_Legacy @ 1/0)
2. **`grep "uniform <Name>\s*{"` で site 数取得 + literal 確認** (= MaterialUBO 49 site / MaterialUBO_Legacy 1 site)
3. **Agent 並列 Read で全 site member 列一致確認** (= MaterialUBO は 45 base + 4 PBR で **divergence 発見** → AYA 判断 = option I)
4. **canonical source 確定 + literal extract** (= MaterialUBO は `pbropaqueF.glsl:44` 10-member、MaterialUBO_Legacy は `materialF.glsl:38` 8-member)
5. **blueprint file format 規約適用** (= `#version 450` + `layout(std140, set=N, binding=M) uniform <Name> {...};` + `void main() {}` + comment header に source + sample sites + spec ref)
6. **smoke 3 path + C++17 compile + assert** (= /tmp/aya_ubo_pa8_setN_smoke/ で再現)

### §2.4 設計判断 1 件 (= AYA option I 確定、必ず引き継ぐ)

**finding**: 06a §3.2 inventory は set=1 に MaterialUBO + MaterialUBO_Legacy の **2 entry** と記載されているが、実際の shader source では `(set=1, binding=0, std140) uniform MaterialUBO` が **2 variant** で宣言されている:

| variant | member 数 | site 数 | 採用 file 例 |
|---|---|---|---|
| baseline (6-member) | 6 = mat4 + vec4[2]×2 + vec4 + vec3 + float | 45 | shadowAlphaMaskV / simpleNoColorV / pbrShadowAlphaMaskV / 他 42 |
| PBR-extended (10-member) | 10 = base 6 + metallicFactor + roughnessFactor + 2 pad | 4 | pbropaqueF / pbropaqueV / pbralphaV / class2/pbralphaF |

**AYA 判断項 (= 本 session §4.1 で問うた)**: 3 option 提示:
- **(I) blueprint = 10-member full canonical** = host C++ alloc full、45 baseline は trailing 4 未参照 view (= Vulkan std140 layout-compat 合法、既存 OpenGL で proven、shader 改修ゼロ)
- **(II) blueprint 2 entry 化** = base + PBR 別 UBO、4 PBR shader を `MaterialUBO_PBR` rename (= shader 改修発生、entry handoff §3 PA-8 "既存 shader 改修なし" 縛り違反)
- **(III) blueprint 保留** = 06a inventory update + 設計 phase 行き

**AYA 確定 = (I)**。理由 = 06a §3.2 が「2 件」と書いてある以上 blueprint 枚数は 2 で一致、Vulkan/GLSL std140 慣用で larger buffer に smaller view 合法、shader 改修ゼロ堅持、host redirect 層 (Phase 1.B) は full member 列知るべき。

**実装**: `material_ubo.glsl` は **10-member PBR-extended literal extract from `pbropaqueF.glsl:44`**、comment header に「46 file 中、4 PBR site が 10-member、残り 45 site が 6-member base のみを宣言。AYA option (I) 採用 = 10-member full を canonical blueprint」と明記。

### §2.5 残 1 件 未確定 (= 次 session 自走可、AYA 判断不要)

`MaterialUBO` と `MaterialUBO_Legacy` が metadata 上 **`(set=1, binding=0, subset=0)` で完全同位**:

| name | descriptor_set | binding | subset | cadence_tag | member_count |
|---|---|---|---|---|---|
| MaterialUBO | 1 | 0 | **0** | 1 (PerProgram) | 10 |
| MaterialUBO_Legacy | 1 | 0 | **0** | 1 (PerProgram) | 8 |

**原因**: GLSL `layout(...)` qualifier に `subset` 概念が存在しない (= `set=N, binding=M, std140` 等のみ規定) ため `glsl_parser._parse_layout_qual()` は subset 抽出できず、`main.py:194` も forward しない (= `descriptor_set` / `binding` 同様の forward path がない)。

**影響範囲**: Phase 1.A Exit Criteria literal 充足 (= bind 不変 = host code 無関与、metadata の subset field は Phase 1.B redirect 層の dispatch key として使う想定だが、Phase 1.A では unused)。Phase 1.B host wiring 時に解消する path 候補:
- **a)** name-based dispatch (= `MaterialUBO` / `MaterialUBO_Legacy` 名を直接 key に使う) = 06a §3.2 所見「program ごとに片方のみ宣言」運用前提で subset 不要 = 最 minimal
- **b)** block name prefix-based subset 推定 (= `_Legacy` suffix を subset=1 推定する main.py side rule) = 09 §4.3 拡張、Phase 1.B entry sub-task
- **c)** 06a inventory に subset 明示記述 + AYAstorm 独自 GLSL annotation pragma 導入 (= GLSL に subset = N コメント or `//[[subset=N]]` syntax 拡張) = 設計 phase 行き

**本 handoff 時点判定 = 次 session 自走可 + AYA 判断不要**。理由 = (1) Phase 1.A scope (= 85 blueprint 著作 + Exit Criteria 充足) では subset 同位は blocker でない (= existing program 1 個 include + bind 不変動作確認は subset 不要)、(2) Phase 1.B 着手時に redirect 層設計の一環として上 a/b/c 判断、(3) set=2/3 著作中に同様 finding (= 別 UBO 同 binding 同 subset) があれば handoff doc に随時記録。

### §2.6 引き継ぐべき protocol (= set=2/3 著作で literal 再利用)

**(P-1)** divergence 検出 protocol: `grep "uniform <Name>\s*{"` で site 列挙 → Agent 並列 Read で member 列比較 → divergence 発覚なら AYA 判断仰ぐ (= 本 session で set=1 で実施、option I 確定)

**(P-2)** binding 一意性 self-verify: smoke 後 metadata で `(set, binding, subset)` 組の重複を確認 = 06a inventory entry と数一致 (= set=2 26 件 / set=3 54 件 が emit metadata の entry 数と一致するか)

**(P-3)** cadence_tag 自動推定確認: block name prefix で `_PREFIX_TO_CADENCE` (= `main.py:_PREFIX_TO_CADENCE`) が機能してるか smoke 後確認 = `Frame*` → 0 / `PerProgramUBO_*` → 1 / `PerDrawUBO_*` → 2 / `<Name>UBO_Legacy` → 1 (= Material prefix で PerProgram 推定が現在の挙動、Legacy も同 = §2.5 と関連)

**(P-4)** hash collision check: `ubo_perfect_hash.inl` の `g_chd_values[N]` 配列が member total に対し perfect (= 0 collision) であること

---

## §3 次 session 着手 (= PA-8 set=2 + set=3 + Exit Criteria 検証)

### §3.1 着手 1 line

「前 session で PA-7.6 (= main.py:189-196 set/binding forward fix + unittest 2 件追加 = 127/127 PASS) + PA-8 set=1 (= MaterialUBO 10-member + MaterialUBO_Legacy 8-member literal extract + AYA option I 確定) 完了。本 session = **PA-8 set=2 着手** = `aya_r41_blueprints/set2/<lowercase_name>.glsl` で **26 UBO** (= 06a §3.3 PerDrawUBO_* + PerProgramUBO_* binding 0..25) 著作 = §2.6 protocol P-1〜P-4 に従い literal extract + smoke 3 path + C++17 compile。完了後 PA-8 set=3 (= 54 UBO `<Name>UBO_Legacy`) → Exit Criteria 検証 (= 09 §4.2 codegen 実行 PASS + 既存 program 1 個 include + bind 不変動作確認)」

### §3.2 PA-8 残 sub-step scope (= strict 線形)

| sub-step | 件数 | 出力 dir | source ref | 完了条件 |
|---|---|---|---|---|
| **PA-8 set=2** | 26 (= PerDrawUBO_* / PerProgramUBO_*、binding=0..25) | `aya_r41_blueprints/set2/` | 06a §3.3 + grep `uniform PerDrawUBO_\|uniform PerProgramUBO_` | smoke 3 path + 31 file emit (= 5 aggregated + 26 per-block layout) → C++17 compile + 全 26 UBO metadata 含む確認 + binding 一意性 (= set=2 内 binding 0..25 unique) |
| **PA-8 set=3** | 54 (= <Name>UBO_Legacy、binding 各種、§3.4 表の binding 番号) | `aya_r41_blueprints/set3/` | 06a §3.4 + grep `uniform [A-Za-z]*UBO_Legacy\b` | smoke 3 path + 90 file emit (= 5 aggregated + 85 per-block layout) → C++17 compile + 全 85 UBO 名前 + (set, binding, subset) 全件 unique 確認 |
| **PA-8 Exit Criteria 検証** | - | - | 09 §4.2 | (i) codegen 実行 PASS (= miss/hit/--force 3 path) / (ii) 既存 program 1 個 (= 例: pbropaque) で `#include "codegen/ubo/ubo_index.inl"` 経由 metadata 解決確認 / (iii) bind 不変動作 = 既存 GL setUniform call site が runtime で fail せず viewer launch 可能 (= 実 build 必要、3 OS 統一は本 phase の scope 外、Linux のみで PASS で literal 充足) |

### §3.3 PA-8 set=2 着手 protocol (= 本 session P-1〜P-4 再利用)

set=1 で確立した protocol で順次:
1. 06a §3.3 で 26 UBO の binding + canonical source 確認
2. 各 UBO で `grep "uniform <Name>\s*{"` site 数取得
3. 多 site UBO は Agent 並列 member 列確認 (= divergence 検出 protocol P-1)
4. canonical source 確定 + `aya_r41_blueprints/set2/<lowercase_name>.glsl` 著作 = `#version 450` + `layout(std140, set=2, binding=M) uniform <Name> {...};` + `void main() {}`
5. 全 26 著作後 smoke 3 path + C++17 standalone compile (= P-2 binding 一意性 + P-4 hash collision)

### §3.4 PA-8 set=3 着手 protocol (= set=2 と同、ただし binding 番号 spec)

06a §3.4 の binding 番号表に正確に従う (= binding=0/1/2/.../62 で **連続でなく抜けあり** = `AtmoExtraUBO_Legacy` 0 → `SkyVParamUBO_Legacy` 1 → ... → 15-16/23 等抜け)。binding 一意性 verify (P-2) で set=3 内 unique 確認 + 06a 表との 1:1 一致確認。

### §3.5 Exit Criteria 検証時の注意

- **既存 program 1 個 include**: `pbropaque` 等を Phase 1.B/1.C/2 で本格使用予定の同 program で include 試行 = `#include "codegen/ubo/ubo_index.inl"` 経由 = `aya_attach_ubo_codegen(<target>)` で `target_include_directories` 配線
- **bind 不変動作**: 既存 GL `glUniform*` / `glBindBufferBase` call site が runtime で fail せず viewer launch 可能 (= **実 build 必要 + Linux のみ実施、3 OS 統一は別 phase**)
- **build_only_verified 適用**: viewer 起動確認まで PASS 後にのみ PA-8 完了 commit (= Phase 1.A 全終了 marker = Phase 1.B entry)

---

## §4 self-verify (= 9 観点、本 handoff 起案時点)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) `main.py:189-196` fix が物理的に存在 | `_ubo_to_block_spec()` 関数本体に `descriptor_set=int(ubo.layout_qual.get("set", 0))` + `binding=int(ubo.layout_qual.get("binding", 0))` 2 line 追加済 | ✅ |
| (2) `tests/test_main.py` 新規 + unittest 全件 PASS | `UboToBlockSpecTests` class 2 test 追加、`python3 -m unittest discover` で 127/127 PASS | ✅ |
| (3) `aya_r41_blueprints/set1/material_ubo.glsl` 物理存在 | 10-member PBR-extended、`#version 450` + `layout(std140, set=1, binding=0) uniform MaterialUBO {...};` + `void main() {}` の固定 format | ✅ |
| (4) `aya_r41_blueprints/set1/material_ubo_legacy.glsl` 物理存在 | 8-member、source = materialF.glsl:38 literal extract | ✅ |
| (5) PA-7.6 smoke 3 path PASS | `/tmp/aya_ubo_pa7_6_smoke/` で 8 file emit + miss/hit/--force 3 path + set=0 metadata binding 0/1/2 正しい確認 | ✅ |
| (6) PA-8 set=1 smoke 3 path PASS | `/tmp/aya_ubo_pa8_set1_smoke/` で 10 file emit + miss/hit/--force 3 path + 5 block / 56 member / 0 hash collision | ✅ |
| (7) C++17 standalone compile + 5/5 UBO lookup assert PASS | `sanity_check.cpp` で descriptor_set / binding / member_count / cadence_tag 全件 assert PASS | ✅ |
| (8) git working tree 状態 = scripts/ubo_codegen/main.py modified + aya_r41_blueprints/set1/ untracked + handoff doc 新規、tests/ 不在 (= `feedback_tests_dir_never_commit`) | `git status` で確認、Co-Authored-By: Claude 行不在 | ✅ |
| (9) handoff doc 命名対称 | `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-PA-7-6-and-PA-8-set1-complete.md` = 前 handoff 命名規約 (`-PA-7-5-and-PA-8-set0-complete.md`) と対称 | ✅ |

---

## §5 引き継ぎ済 memory 18 件 (= 次 session で active)

- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ
- `feedback_design_phase_no_code_write` — 解禁済 (Phase 1.A 実装 phase)
- `feedback_no_scope_shrink` — set=2 26 件 / set=3 54 件は全著作、part-of で済まさない
- `feedback_self_verify_before_handoff` — 本 session で発動 (= §2.4 + §2.5 divergence + subset 同位の能動 surface)
- `feedback_no_claude_coauthor` — 本 handoff doc 含め全 commit 共著行不在
- `feedback_one_step_at_a_time` — set=1 著作中 §4.1 (= MaterialUBO option I/II/III) を AYA に問うた = 推測実装回避
- `feedback_doubt_self_first` — 本 session で発動 (= Agent 49 site member 一致確認 → 直接 grep で divergence 4 site 特定 = Agent の "49/49 identical" 結論を疑った)
- `feedback_proactive_handoff` — 本 session で発動 (= PA-7.6 + set=1 batch 完了で次 session に handoff)
- `feedback_no_auto_commit` — 本 handoff doc + main.py mod + set1/ 2 file は AYA 明示指示後 batch commit
- `feedback_remove_verification_logs` — 本 session 追加 log/diagnostic 不在 (= 該当なし)
- `feedback_build_only_verified` — 本 session 全 verification PASS で記録
- `feedback_tests_dir_never_commit` — `tests/test_main.py` 新規だが commit 対象外 (= 既存 5 test file と同 local-only pattern)
- `project_ayastorm_r41_vulkan_migration` — PA-7.6 + PA-8 set=1 完了 milestone
- `project_ayastorm_r41_design_principles` — Vulkan std140 layout-compat 慣用 (= larger buffer に smaller view) を本 session option I で活用
- `feedback_ubo_migration_one_at_a_time` — set=1 2 件を本 session で完了、set=2 26 件 / set=3 54 件は次 session 以降に分割
- `project_build_procedure` — PA-8 Exit Criteria 検証で実 indra/ build 必要時に参照
- `feedback_use_agents_proactively` — set=1 で agent 並列 member 確認実施 (= Agent 結果を直接 grep で 2 重 verify、`feedback_doubt_self_first` 適用 PASS)
- `project_ayastorm_three_platforms` — Exit Criteria 検証 (= viewer launch) は 3 OS 揃え別 phase、Phase 1.A は Linux first-class baseline

---

## §6 次 session 着手 1 line

**「前 session で PA-7.6 (= main.py:189-196 set/binding forward fix + tests/test_main.py 新規 = 127/127 PASS) + PA-8 set=1 (= material_ubo.glsl 10-member PBR-extended canonical + material_ubo_legacy.glsl 8-member literal extract、AYA option I 確定) 完了 + smoke 3 path + C++17 compile + 5/5 UBO lookup assert PASS。本 session = **PA-8 set=2 着手** = `aya_r41_blueprints/set2/<lowercase_name>.glsl` で **26 UBO** (= 06a §3.3 PerDrawUBO_* + PerProgramUBO_* binding 0..25) を本 handoff §2.6 protocol P-1〜P-4 (= divergence 検出 + binding 一意性 verify + cadence_tag 自動推定確認 + hash collision check) に従い literal extract + smoke 3 path + C++17 compile。完了後 PA-8 set=3 (= 54 UBO `<Name>UBO_Legacy`) → Exit Criteria 検証 (= 09 §4.2)。AYA 判断不要、Claude 自走可。」**
