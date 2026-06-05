# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2 完了

**作成日**: 2026-06-04
**前 session commit**: `0dda2e8198` (= PA-7.6 + PA-8 set=1 完了、本 session entry 時点)
**本 session 物理出力** (= 全て **未 commit**、AYA さん明示指示後 batch commit):
- `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/` 新規 dir + 31 .glsl file (= 06a §3.3 inventory 26 entry に対し AYA option I 確定で **31 件** 著作 = binding=0 で 6 UBO 共存 (PerDraw 帯)、binding=1..25 で 25 UBO)
- 本 handoff doc 新規

**次 session 着手**: **AYA 判断要なし** (= set=2 確定済) → **PA-8 set=3 (= 54 UBO `<Name>UBO_Legacy` 著作)** → **Phase 1.A Exit Criteria 検証** (= 09 §4.2)。前 entry handoff §3 PA-8 row literal scope 通り strict 線形継続で Phase 1.A 完了。

---

## §0 state 一行 summary

η-30 **Phase 1.A PA-8 set=2 完了 state**:
- **PA-8 set=2** = `aya_r41_blueprints/set2/` 配下 **31 file** (= `per_draw_ubo_*.glsl` 7 + `per_program_ubo_*.glsl` 24) literal extract from `class*/...` 既存 `#ifdef LL_VULKAN_GLSL` block。06a §3.3 表は 26 entry だが、`binding=0` 行に 6 UBO 名 (= ClipPlane / AvatarSkin / ObjectSkin / SkinnedVelocity / AvatarVelocity / LightParams) が同居しており、AYA 判断 = **option I = 6 UBO は別 blueprint で per-program 切替** (= set=1 MaterialUBO/MaterialUBO_Legacy 同 binding 共存と同 precedent)。
- **smoke 3 path PASS** (= /tmp/aya_ubo_pa8_set2_smoke/) = Run #1 miss 2069ms + 41 file emit (= 5 aggregated + 36 per-block layout = set=0 3 + set=1 2 + set=2 31) + 173 member / Run #2 hit 0.05s (= Python 起動なし) / Run #3 --force 2086ms + 41 file 再 emit
- **metadata 全 36 UBO 正しい** (= `g_block_count=36u` / 0 hash collision / binding 分布: set=0 3 binding 全 unique + set=1 2 UBO @ binding=0 共存 + set=2 31 UBO @ 26 distinct binding = binding=0 で 6 共存 + binding=1..25 単一)
- **C++17 standalone compile + 36/36 UBO lookup PASS** (= sanity_check.cpp で 全 UBO descriptor_set + binding 一致 assert + cadence_tag=1 (PerProgram default fallback) assert)
- **unittest 127/127 + py_compile 8/8 PASS** (= 既存 PA-7.6 baseline 維持、新規 test 追加なし)

**設計判断 1 件 (= AYA option I 確定、必ず引き継ぐ)**: 06a §3.3 inventory 表の `binding=0` 行に 6 UBO 名同居 = 各 program は片方のみ宣言する per-program namespace 想定 (= GL/Vulkan ともに合法、program 内で UBO 名重複しない限り別 program 間で binding 重複 OK)。同じ pattern は set=1 で既出 (MaterialUBO / MaterialUBO_Legacy 同 binding=0)。Phase 1.B host wiring で name-based dispatch (= §2.5 `feedback_design_phase_no_code_write` 適用、Phase 1.B 行き)。詳細 = §2.4。

**設計差分 1 件 (= macro literal 置換)**: 2 UBO の member に compile-time macro `MAX_JOINTS_PER_MESH_OBJECT` (= `lljoint.h:48` `LL_MAX_JOINTS_PER_MESH_OBJECT = 110`、`llviewershadermgr.cpp:870` で global #define 注入) + `LIGHT_COUNT` (= `llviewershadermgr.h:33` `LL_DEFERRED_MULTI_LIGHT_COUNT = 16`、`llviewershadermgr.cpp:1756` で 1..16 permutation) が使われている。blueprint は literal int 必要ゆえ **MAX_JOINTS=110** + **LIGHT_COUNT=16 (max permutation = canonical)** 採用、comment header で macro 出典明記。詳細 = §2.5。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-PA-8-set2-complete.md`) | 全文 | PA-8 set=2 完了 state + option I 確定 (= 6 UBO @ binding=0 共存) + macro literal 置換 + 残 sub-step 線形 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-entry.md` | §3 PA-X 構成表 + §2 Phase 1.A scope + Exit Criteria | Phase 1.A 全体像 + PA-8 set=3 sub-task scope literal 参照 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md` | §3.4 set=3 (= 54 件 `<Name>UBO_Legacy`) | 残 54 UBO の binding 番号 + cadence + source 既存配置 (= literal extract source 候補) |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §4.2 Phase 1.A Exit Criteria literal (= "85 UBO blueprint codegen 実行 PASS + 生成 header をテスト program で include + bind 不変動作確認") |
| `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` | §3.1 std140 strict 順守判断 A + §5.1 4 file 分割契約 |
| `scripts/ubo_codegen/main.py:65-72` | `_PREFIX_TO_CADENCE` 表 (= `Frame*` → 0 / `Program_*` → 1 / `Draw_*` → 2 / `Asset_*` → 3 / `Skin_*` → 4 / `Global_*` → 5、prefix に underscore 必須 = `PerDrawUBO_*` / `PerProgramUBO_*` は match せず default 1=PerProgram に fallback、§2.6 で詳細) |
| `indra/cmake/AyaUboCodegen.cmake:56-58` | BLUEPRINT_DIR = `…/aya_r41_blueprints` (= 本 session 不変、PA-8 set=3 でも同) |
| `indra/newview/app_settings/shaders/aya_r41_blueprints/set0/` + `set1/` + `set2/` | 既存 36 file (= set=0 3 + set=1 2 + set=2 31) = set=3 著作テンプレ |

---

## §2 本 session 成果

### §2.1 PA-8 set=2 真 scope 達成

| 出力契約 | 実装箇所 | 内容 |
|---|---|---|
| `aya_r41_blueprints/set2/per_draw_ubo_*.glsl` 7 file (= binding=0 6 共存 + binding=1 MultiLight) | 同 path | ClipPlane / AvatarSkin / ObjectSkin / SkinnedVelocity / AvatarVelocity / LightParams (= binding=0、option I) + MultiLight (= binding=1) literal extract from `class*/...` `#ifdef LL_VULKAN_GLSL` block |
| `aya_r41_blueprints/set2/per_program_ubo_*.glsl` 24 file (= binding=2..25) | 同 path | GammaCorrect 2 / AlphaParams 3 / ColorGrading 4 / PointLightV 5 / ShadowAlphaMaskV 6 / PostDeferredV 7 / FullbrightShinyV 8 / FxaaF 9 / SpotLightF 10 / PbrAlphaV 11 / PostDeferredNoDoFF 12 / FsObjectIdF 13 / ShadowCubeV 14 / WaterHazeV 15 / VisualizeBuffersF 16 / GodraysF 17 / VolumetricLightF 18 / VelocityAlphaV 19 / PostDeferredF 20 / CofF 21 / BlurLightF 22 / WaterF 23 / PbrTerrainV 24 / PointLightF 25 |
| smoke 3 path PASS | `/tmp/aya_ubo_pa8_set2_smoke/` 独立 cmake project | Run #1 miss 2069ms + 41 file emit (= 5 aggregated + 36 per-block layout) + 173 member / Run #2 hit 0.05s (= Python 起動なし) / Run #3 --force 2086ms + 41 file 再 emit |
| metadata 全 36 UBO 正しい | `/tmp/aya_ubo_pa8_set2_smoke/build/codegen/ubo/ubo_metadata.inl` | `g_block_count=36u` / 0 hash collision / set=0 3 binding 全 unique (0/1/2) + set=1 2 UBO @ binding=0 共存 + set=2 31 UBO @ 26 distinct binding (= binding=0 6 共存 + binding=1..25 単一) |
| C++17 standalone compile + 36/36 lookup assert PASS | `/tmp/aya_ubo_pa8_set2_smoke/sanity_check.cpp` | 全 36 UBO の descriptor_set + binding 一致 + set=2 31 件 cadence_tag=1 (PerProgram default fallback) assert PASS |
| unittest 全件 PASS | `python3 -m unittest discover -s scripts/ubo_codegen/tests -v` | **127/127 PASS** (= PA-7.6 baseline 維持、本 session 新規 test なし) / py_compile 8/8 PASS |

### §2.2 multi-site UBO verify (= 6 件 = §2.6 P-1 適用後 identical 確認)

set=2 UBO で複数 site 宣言ある 6 件は Agent 並列 Read + 直接 Read で完全一致確認 (= `feedback_doubt_self_first` 適用、PA-8 set=1 で Agent "49/49 identical" 結論が 4 件 divergence を見逃した教訓を踏まえ二重 verify):

| UBO 名 | site 数 | site list | 結果 |
|---|---|---|---|
| `PerDrawUBO_ClipPlane` | 2 | `class1/deferred/pbropaqueF.glsl:180` + `class1/deferred/softenLightF.glsl:90` | identical |
| `PerProgramUBO_PostDeferredF` | 2 | `class1/deferred/postDeferredF.glsl:108` + `class3/deferred/postDeferredHQDoFF.glsl:121` | identical |
| `PerProgramUBO_WaterHazeV` | 2 | `class1/deferred/waterHazeV.glsl:86` + `class3/environment/waterHazeF.glsl:50` | identical |
| `PerProgramUBO_PointLightV` | 2 | `class1/deferred/pointLightV.glsl:63` + `class3/deferred/spotLightF.glsl:151` | identical |
| `PerProgramUBO_GammaCorrect` | 2 | `class3/deferred/postDeferredGammaCorrect.glsl:45` + `class3/deferred/postDeferredTonemap.glsl:52` | identical |
| `PerDrawUBO_SkinnedVelocity` | 2 | `class3/avatar/skinnedVelocityAlphaV.glsl:110` + `class3/avatar/skinnedVelocityV.glsl:81` | identical |

残 25 UBO は single site で divergence 発生せず。

### §2.3 blueprint 著作 protocol confirm (= 本 session で set=1 protocol を 31 UBO に scale)

set=1 で確立した step を 31 UBO に適用 (= set=3 でも同 protocol):

1. **06a inventory §3.3 で UBO 名 + binding 確認** (= 本 session = 26 entry + binding=0 行に 6 UBO 同居 finding)
2. **`grep "uniform <Name>\s*{"` で site 数取得 + literal 確認** (= 31 distinct 名 + 6 件 multi-site)
3. **Agent 並列 Read で全 multi-site member 列一致確認** (= 6/6 identical、直接 Read で 12 file 二重 verify = `feedback_doubt_self_first` 適用)
4. **canonical source 確定 + literal extract** (= 全 31 件)
5. **blueprint file format 規約適用** (= `#version 450` + `layout(std140, set=2, binding=M) uniform <Name> {...};` + `void main() {}` + comment header に source + sample sites + spec ref + macro 出典 (該当 2 件))
6. **smoke 3 path + C++17 compile + assert** (= /tmp/aya_ubo_pa8_set2_smoke/ で再現)

### §2.4 設計判断 1 件 (= AYA option I 確定、必ず引き継ぐ)

**finding**: 06a §3.3 inventory 表に `set=2 binding=0` 行が **1 entry** で記載されているが、grep 結果は **6 distinct UBO 名** = `PerDrawUBO_ClipPlane` + `PerDrawUBO_AvatarSkin` + `PerDrawUBO_ObjectSkin` + `PerDrawUBO_SkinnedVelocity` + `PerDrawUBO_AvatarVelocity` + `PerDrawUBO_LightParams` が同 binding を共有。

**AYA 判断項 (= 本 session §4.1 で問うた)**: 3 option 提示:
- **(I) 6 UBO を別 blueprint で per-program 切替** = 各 program は片方のみ宣言する per-program namespace 想定 (= GL/Vulkan ともに合法、別 program 間で binding 重複 OK)、shader 改修ゼロ
- **(II) 6 UBO を merge して single canonical UBO 化** = 全 member 列 union、各 program は subset 参照 view (= MaterialUBO 10-member full と同流の Vulkan std140 layout-compat)
- **(III) blueprint 保留** = 06a inventory update + 設計 phase 行き

**AYA 確定 = (I)** (= 1 文字回答 `I`)。理由 = set=1 で確定した MaterialUBO/MaterialUBO_Legacy 同 binding 共存 precedent と同形、6 件全て独立 UBO で per-program 排他選択ゆえ merge する正当性なし、shader 改修ゼロ堅持。

**実装**: 6 file 全て **set=2 binding=0 literal extract**、comment header に「set=2 binding=0 は 6 UBO 共存 (= set=1 MaterialUBO/MaterialUBO_Legacy 同 binding 共存と同 precedent)。各 program は 6 件中 1 つのみ宣言する想定で per-program namespace 独立、Phase 1.B host wiring で name-based dispatch (= 06a §3.2 所見再利用)」と明記。

### §2.5 設計差分 1 件 (= macro literal 置換、AYA 判断要なし、技術判断のみ)

2 UBO の member に compile-time macro 使用:

| UBO 名 | macro | source 出典 | literal 採用値 | 理由 |
|---|---|---|---|---|
| `PerDrawUBO_ObjectSkin` | `MAX_JOINTS_PER_MESH_OBJECT` (mat3x4 array dim) | `indra/llcharacter/lljoint.h:48` `LL_MAX_JOINTS_PER_MESH_OBJECT = 110` + `llviewershadermgr.cpp:870` global #define 注入 (= `LLSkinningUtil::getMaxJointCount()`) | **110** | runtime 固定値 (= `constexpr U32`)、permutation なし、global 注入で全 program 共通 |
| `PerDrawUBO_SkinnedVelocity` | `MAX_JOINTS_PER_MESH_OBJECT` (mat3x4 array dim) | 上同 | **110** | 上同 |
| `PerDrawUBO_MultiLight` | `LIGHT_COUNT` (vec4 array dim) | `indra/newview/llviewershadermgr.h:33` `LL_DEFERRED_MULTI_LIGHT_COUNT = 16` + `llviewershadermgr.cpp:1756` per-program permutation (`gDeferredMultiLightProgram[i].addPermutation("LIGHT_COUNT", llformat("%d", i+1));` for i=0..15) | **16 (= max permutation)** | per-program permutation で 1..16 値変化、host C++ alloc は max size 一括が安全 (= MaterialUBO option I と同流 Vulkan std140 layout-compat)、small LIGHT_COUNT shader は trailing 未参照 view |

comment header に上 3 件全て macro 出典 + 採用値 + 理由 明記済。

### §2.6 残 1 件 未確定 (= 次 session 自走可、AYA 判断不要、Phase 1.B 行き)

set=2 31 UBO 全 cadence_tag が **1 (PerProgram default fallback)** = `main.py:65-72` `_PREFIX_TO_CADENCE` 表が `Frame*` / `Program_*` / `Draw_*` / `Asset_*` / `Skin_*` / `Global_*` の 6 prefix のみ列挙、`PerDrawUBO_*` / `PerProgramUBO_*` (= AYAstorm naming) は **match せず default 1=PerProgram に fallback**。

**影響範囲**: Phase 1.A Exit Criteria literal 充足 (= bind 不変 = host code 無関与、metadata の cadence_tag field は Phase 1.B redirect 層の dispatch hint として使う想定だが、Phase 1.A では unused)。Phase 1.B host wiring 時に解消する path 候補:
- **a)** `_PREFIX_TO_CADENCE` 表に `PerDrawUBO_` → 2 / `PerProgramUBO_` → 1 / `<Name>UBO_Legacy` → 1 を追加 = main.py 3-5 line fix (= PA-7.6 set/binding forward fix と同流の trivial fix)
- **b)** block name suffix-based cadence 推定 (= 例: `_F` / `_V` suffix で stage 推定、ただし cadence と直交ゆえ採用しない)
- **c)** 06a inventory に cadence 明示記述 + AYAstorm 独自 GLSL annotation pragma 導入 (= 設計 phase 行き)

**本 handoff 時点判定 = 次 session 自走可 + AYA 判断不要 + Phase 1.B 行き**。理由 = (1) Phase 1.A scope (= 85 blueprint 著作 + Exit Criteria 充足) では cadence_tag default fallback は blocker でない (= existing program 1 個 include + bind 不変動作確認は cadence 不要)、(2) Phase 1.B host wiring 着手時に redirect 層設計の一環として a/b/c 判断、(3) set=3 著作中に同様 finding (= `_Legacy` suffix 全件 cadence=1 fallback) があれば随時記録、(4) **重要**: 本 handoff §1.1 必読 3 件には含めない (= 自走判断項のため AYA 判断仰がず)。

### §2.7 引き継ぐべき protocol (= set=3 著作で literal 再利用)

PA-8 set=1 で起案した protocol を本 session で 31 UBO scale で再確認、set=3 でも literal 再利用:

**(P-1)** divergence 検出 protocol: `grep "uniform <Name>\s*{"` で site 列挙 → Agent 並列 Read で member 列比較 → divergence 発覚なら AYA 判断仰ぐ (= 本 session で 6 multi-site UBO 全 identical 確認、divergence なし)

**(P-2)** binding 一意性 self-verify: smoke 後 metadata で `(set, binding, subset)` 組の重複を確認 = 06a inventory entry と数一致 (= set=3 54 件が emit metadata の entry 数と一致するか、set=3 内 binding 0..62 unique で抜けあり = 06a §3.4 表通り)

**(P-3)** cadence_tag 自動推定確認: block name prefix で `_PREFIX_TO_CADENCE` (= `main.py:65-72`) が機能してるか smoke 後確認 = §2.6 で確認したように **set=2 31 件 + set=3 54 件 全 default 1 fallback** の見込み (= Phase 1.B 行き、blocker でない)

**(P-4)** hash collision check: `ubo_perfect_hash.inl` の `g_chd_values[N]` 配列が member total に対し perfect (= 0 collision) であること

**(P-5、本 session 追加)** macro literal 置換 protocol: blueprint に compile-time macro 出現時は (a) source 出典の `#define` / `constexpr` 確定 → (b) permutation なし固定値ならその値、permutation ありなら max 値採用 (= Vulkan std140 layout-compat 慣用、small permutation shader は trailing 未参照 view) → (c) comment header に macro 名 + 出典 + 採用値 + 理由明記

---

## §3 次 session 着手 (= PA-8 set=3 + Exit Criteria 検証)

### §3.1 着手 1 line

「前 session で PA-8 set=2 (= aya_r41_blueprints/set2/ 配下 31 file literal extract + AYA option I 確定 = 6 UBO @ binding=0 共存 + 25 UBO @ binding=1..25 + macro literal 置換 = MAX_JOINTS=110 + LIGHT_COUNT=16) 完了 + smoke 3 path + C++17 compile + 36/36 lookup assert PASS。本 session = **PA-8 set=3 着手** = `aya_r41_blueprints/set3/<lowercase_name>.glsl` で **54 UBO** (= 06a §3.4 `<Name>UBO_Legacy` binding 各種、表通りで連続でなく抜けあり) 著作 = §2.7 protocol P-1〜P-5 に従い literal extract + smoke 3 path + C++17 compile。完了後 Phase 1.A Exit Criteria 検証 (= 09 §4.2 codegen 実行 PASS + 既存 program 1 個 include + bind 不変動作確認) で Phase 1.A 全終了。」

### §3.2 PA-8 残 sub-step scope (= strict 線形)

| sub-step | 件数 | 出力 dir | source ref | 完了条件 |
|---|---|---|---|---|
| **PA-8 set=3** | 54 (= `<Name>UBO_Legacy`、binding 各種、§3.4 表の binding 番号) | `aya_r41_blueprints/set3/` | 06a §3.4 + grep `uniform [A-Za-z]*UBO_Legacy\b` | smoke 3 path + 95 file emit (= 5 aggregated + 90 per-block layout = set=0 3 + set=1 2 + set=2 31 + set=3 54) → C++17 compile + 全 90 UBO 名前 + (set, binding, subset) 全件 unique 確認 (= set=3 内 binding は 06a §3.4 表通り、連続でなく抜けあり、重複可否は 06a 確認後判定) |
| **PA-8 Exit Criteria 検証** | - | - | 09 §4.2 | (i) codegen 実行 PASS (= miss/hit/--force 3 path) / (ii) 既存 program 1 個 (= 例: pbropaque) で `#include "codegen/ubo/ubo_index.inl"` 経由 metadata 解決確認 / (iii) bind 不変動作 = 既存 GL setUniform call site が runtime で fail せず viewer launch 可能 (= 実 build 必要、3 OS 統一は本 phase の scope 外、Linux のみで PASS で literal 充足) |

### §3.3 PA-8 set=3 着手 protocol (= §2.7 P-1〜P-5 再利用)

1. 06a §3.4 で 54 UBO の binding + canonical source 確認 (= binding 0..62 抜けあり、重複可否含む)
2. 各 UBO で `grep "uniform <Name>\s*{"` site 数取得
3. 多 site UBO は Agent 並列 + 直接 Read で member 列確認 (P-1 divergence 検出、`feedback_doubt_self_first` 二重 verify)
4. canonical source 確定 + `aya_r41_blueprints/set3/<lowercase_name>.glsl` 著作 = `#version 450` + `layout(std140, set=3, binding=M) uniform <Name>_Legacy {...};` + `void main() {}`
5. 全 54 著作後 smoke 3 path + C++17 standalone compile (= P-2 binding unique + P-4 hash collision)
6. macro literal 置換あれば P-5 適用

### §3.4 Exit Criteria 検証時の注意

- **既存 program 1 個 include**: `pbropaque` 等を Phase 1.B/1.C/2 で本格使用予定の同 program で include 試行 = `#include "codegen/ubo/ubo_index.inl"` 経由 = `aya_attach_ubo_codegen(<target>)` で `target_include_directories` 配線
- **bind 不変動作**: 既存 GL `glUniform*` / `glBindBufferBase` call site が runtime で fail せず viewer launch 可能 (= **実 build 必要 + Linux のみ実施、3 OS 統一は別 phase**)
- **build_only_verified 適用**: viewer 起動確認まで PASS 後にのみ PA-8 完了 commit (= Phase 1.A 全終了 marker = Phase 1.B entry)
- **副次 task 候補**: §2.6 cadence_tag default fallback fix を Phase 1.B entry 直前 or 直後の small task として `main.py:65-72` `_PREFIX_TO_CADENCE` に `PerDrawUBO_` / `PerProgramUBO_` / `_Legacy` を追加 = trivial 3-5 line + unittest 2-3 件追加 (= PA-7.6 fix と同流)

---

## §4 self-verify (= 9 観点、本 handoff 起案時点)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) `aya_r41_blueprints/set2/` 配下 31 file 物理存在 | `ls /home/ishikawa/work_firestorm/phoenix-firestorm/indra/newview/app_settings/shaders/aya_r41_blueprints/set2/ \| wc -l` = 31 | ✅ |
| (2) 全 31 file は `#version 450` + `layout(std140, set=2, binding=N) uniform <Name> {...}` + `void main() {}` の固定 format | 各 file 物理 Read で確認、binding 値は metadata 表通り | ✅ |
| (3) PA-8 set=2 smoke 3 path PASS | `/tmp/aya_ubo_pa8_set2_smoke/` で miss 2069ms + hit 0.05s + --force 2086ms、41 file emit (= 5 aggregated + 36 per-block layout) | ✅ |
| (4) metadata 全 36 UBO 正しい + 0 hash collision | `ubo_metadata.inl` で `g_block_count=36u`、binding 分布 = set=0 3 unique + set=1 2@0 + set=2 31 with 6@0 + 25@1..25 (= option I 通り) | ✅ |
| (5) C++17 standalone compile + 36/36 lookup assert PASS | `sanity_check.cpp` で全 UBO descriptor_set + binding 一致 + set=2 cadence_tag=1 assert PASS、`./sanity_check` 出力 = "PASS: 36/36 UBO lookup OK" | ✅ |
| (6) multi-site UBO 6 件全 identical 確認 | Agent 並列 + 直接 Read 12 file で二重 verify、divergence なし (= `feedback_doubt_self_first` 適用) | ✅ |
| (7) macro literal 置換 3 件 (= MAX_JOINTS×2 + LIGHT_COUNT×1) source 出典明記 | comment header で `lljoint.h:48` + `llviewershadermgr.h:33` + 採用値 + 理由明記 | ✅ |
| (8) unittest 127/127 + py_compile 8/8 PASS | `python3 -m unittest discover` + `python3 -m py_compile` 全 PASS、PA-7.6 baseline 維持 | ✅ |
| (9) git working tree 状態 = aya_r41_blueprints/set2/ untracked + handoff doc 新規、indra/ 他改変なし + scripts/ 改変なし + Co-Authored-By: Claude 行不在 + handoff doc 命名対称 (= 前 handoff `…-PA-7-6-and-PA-8-set1-complete.md` と対称で `…-PA-8-set2-complete.md`) | `git status` で確認 | ✅ |

---

## §5 引き継ぎ済 memory 18 件 (= 次 session で active)

- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ
- `feedback_design_phase_no_code_write` — 解禁済 (Phase 1.A 実装 phase)
- `feedback_no_scope_shrink` — set=3 54 件は全著作、part-of で済まさない (= 本 session で set=2 26 → 31 拡張も option I 受領、scope 拡大も同様に厳守)
- `feedback_self_verify_before_handoff` — 本 session で発動 (= §2.4 6 UBO @ binding=0 共存 finding + §2.5 macro literal 置換 + §2.6 cadence default fallback finding の 3 件を能動 surface)
- `feedback_no_claude_coauthor` — 本 handoff doc 含め全 commit 共著行不在
- `feedback_one_step_at_a_time` — set=2 著作中 §4.1 (= 6 UBO @ binding=0 共存 option I/II/III) を AYA に問うた = 推測実装回避
- `feedback_doubt_self_first` — 本 session で発動 (= Agent multi-site member 一致確認結果を直接 Read 12 file で二重 verify、PA-8 set=1 で Agent "49/49 identical" 結論が 4 件 divergence を見逃した教訓踏襲)
- `feedback_proactive_handoff` — 本 session で発動 (= PA-8 set=2 batch 完了で次 session に handoff)
- `feedback_no_auto_commit` — 本 handoff doc + set2/ 31 file は AYA 明示指示後 batch commit
- `feedback_remove_verification_logs` — 本 session 追加 log/diagnostic 不在 (= 該当なし)
- `feedback_build_only_verified` — 本 session 全 verification PASS で記録
- `feedback_tests_dir_never_commit` — 本 session test 新規追加なし (= PA-7.6 baseline 維持)
- `project_ayastorm_r41_vulkan_migration` — PA-8 set=2 完了 milestone
- `project_ayastorm_r41_design_principles` — Vulkan std140 layout-compat 慣用 (= larger buffer に smaller view) を §2.5 LIGHT_COUNT=16 採用で活用
- `feedback_ubo_migration_one_at_a_time` — set=2 31 件を本 session で完了、set=3 54 件は次 session 以降に分割
- `project_build_procedure` — PA-8 Exit Criteria 検証で実 indra/ build 必要時に参照
- `feedback_use_agents_proactively` — set=2 で agent 並列 member 確認実施 (= Agent 結果を直接 Read で 2 重 verify、`feedback_doubt_self_first` 適用 PASS)
- `project_ayastorm_three_platforms` — Exit Criteria 検証 (= viewer launch) は 3 OS 揃え別 phase、Phase 1.A は Linux first-class baseline

---

## §6 次 session 着手 1 line

**「前 session で PA-8 set=2 (= aya_r41_blueprints/set2/ 配下 31 file literal extract + AYA option I 確定 = 6 UBO @ binding=0 共存 + 25 UBO @ binding=1..25 + macro literal 置換 3 件 (= MAX_JOINTS_PER_MESH_OBJECT=110 × 2 + LIGHT_COUNT=16 × 1)) 完了 + smoke 3 path + C++17 compile + 36/36 lookup assert PASS + unittest 127/127 + py_compile 8/8 PASS。本 session = **PA-8 set=3 着手** = `aya_r41_blueprints/set3/<lowercase_name>.glsl` で **54 UBO** (= 06a §3.4 `<Name>UBO_Legacy`、binding 0..62 抜けあり、表通り) を本 handoff §2.7 protocol P-1〜P-5 (= divergence 検出 + binding 一意性 verify + cadence_tag 自動推定確認 + hash collision check + macro literal 置換) に従い literal extract + smoke 3 path + C++17 compile。完了後 Phase 1.A Exit Criteria 検証 (= 09 §4.2 codegen 実行 PASS + 既存 program 1 個 include + bind 不変動作確認) で Phase 1.A 全終了。AYA 判断不要、Claude 自走可。」**
