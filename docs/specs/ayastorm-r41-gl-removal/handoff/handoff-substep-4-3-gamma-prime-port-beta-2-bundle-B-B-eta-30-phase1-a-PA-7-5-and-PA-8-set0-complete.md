# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-7.5 + PA-8 set=0 完了

**作成日**: 2026-06-04
**前 session commit**: `794e466564` (= PA-7 完了 handoff doc 起案、本 session entry 時点)
**本 session 物理出力** (= 全て **未 commit**、AYA さん明示指示後 batch commit):
- `indra/newview/app_settings/shaders/aya_r41_exemplar/sky_placeholderV.glsl` MOD = `std140` qualifier 3 箇所補完
- `indra/cmake/AyaUboCodegen.cmake` MOD = `AYA_UBO_CODEGEN_BLUEPRINT_DIR` default 2 段変更 (= `…/shaders` → `…/aya_r41_exemplar` → `…/aya_r41_blueprints`、最終は `…/aya_r41_blueprints`)
- `indra/newview/app_settings/shaders/aya_r41_blueprints/set0/frame_view_proj.glsl` 新規
- `indra/newview/app_settings/shaders/aya_r41_blueprints/set0/frame_lights.glsl` 新規
- `indra/newview/app_settings/shaders/aya_r41_blueprints/set0/frame_atmosphere_lighting.glsl` 新規
- `indra/newview/app_settings/shaders/aya_r41_blueprints/set{1,2,3}/` 空 dir 3 件 (= 次 session 着手地点 placeholder)
- 本 handoff doc 新規

**次 session 着手**: **AYA 判断要** = §4.1 で問う parser binding=0 defect の修正タイミング (α / β / γ) を決めてから PA-8 set=1 (= MaterialUBO + MaterialUBO_Legacy 2 UBO 著作) 着手。

---

## §0 state 一行 summary

η-30 **Phase 1.A PA-7.5 完了 + PA-8 set=0 完了 state** (= PA-7.5 micro-fix = `sky_placeholderV.glsl` 3 UBO 宣言に `std140,` qualifier 補完 + cmake module `BLUEPRINT_DIR` default を `aya_r41_exemplar/` に縮小 = 実 indra/ tree CMake configure PASS + smoke 3 path PASS = miss/hit/--force + 6 file emit / PA-8 set=0 = 06a inventory §3.1 literal extract で `aya_r41_blueprints/set0/` 配下に 3 UBO file 著作 = `FrameViewProj` (binding=0, 9 members) + `FrameLights` (binding=1, 9 members) + `FrameAtmosphere_Lighting` (binding=2, 20 members) = source 全 4-5 サンプル site 間で member 完全一致確認 + cmake module `BLUEPRINT_DIR` default を `aya_r41_blueprints/` に最終確定 + smoke 3 path PASS = 8 file emit (= 5 aggregated + 3 per-block layout) / 38 member / 0 hash collision / C++17 standalone compile PASS = `lookup_block("FrameViewProj")` / `lookup_block("FrameLights")` / `lookup_block("FrameAtmosphere_Lighting")` 全 link OK)。

**self-verify 中に判明した defect** (= AYA 判断要、§4.1 参照): `scripts/ubo_codegen/main.py:189-194` `_ubo_to_block_spec()` が `ubo.layout_qual["set"]` / `["binding"]` を BlockSpec に forward しない = metadata に `descriptor_set=0, binding=0` が全 UBO 共通で書かれる (= FrameLights は本来 binding=1, FrameAtmosphere_Lighting は本来 binding=2)。**Phase 1.A Exit Criteria (= 09 §4.2 "codegen 実行 PASS + header include + bind 不変動作") は literal には充足** (= bind 不変 = 既存 GL setUniform call site 触らず) **だが、Phase 1.B 以降の host wiring (= UBO redirect 層 = dummy VkBuffer dispatch per binding point) は本 metadata の binding 値が正しい前提**。

**次 session 着手地点 = PA-8 set=1 (= MaterialUBO + MaterialUBO_Legacy 2 UBO 著作)**、ただし binding=0 defect の修正タイミング α/β/γ を AYA さんに先に確認。

PA-8 残 sub-step = strict 線形 = **set=1 (2 件) → set=2 (26 件) → set=3 (54 件) → Exit Criteria 検証 (= 09 §4.2 充足検証 + 既存 program 1 個 include + bind 不変動作)**。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-PA-7-5-and-PA-8-set0-complete.md`) | 全文 | PA-7.5 + PA-8 set=0 完了 state + parser binding=0 defect 詳細 + 残 sub-step 線形 + α/β/γ AYA 判断項 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-entry.md` | §3 PA-X 全体構成表 + §2 Phase 1.A scope + Exit Criteria | Phase 1.A 全体像 + PA-8 sub-task scope literal 参照 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md` | §3.2 set=1 (= MaterialUBO + MaterialUBO_Legacy)、§3.3 set=2 (= 26 件) §3.4 set=3 (= 54 件) | 残 82 UBO の binding 番号 + cadence + source 既存配置 (= literal extract source 候補) |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` | §3.1 (= std140 strict 順守判断 A) + §5.1 (= 4 file 分割契約) |
| `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §4.2 Phase 1.A Exit Criteria literal (= "85 UBO blueprint codegen 実行 PASS + 生成 header をテスト program で include + bind 不変動作確認") |
| `scripts/ubo_codegen/main.py:189-194` | `_ubo_to_block_spec()` defect 位置 (= α/γ で修正する場合の touch point) |
| `scripts/ubo_codegen/glsl_parser.py:282-315` | `_parse_layout_qual()` = `set`/`binding` を `quals` dict に既に extract 済 (= 修正側は main.py だけ、parser は無修正) |
| `indra/cmake/AyaUboCodegen.cmake:56-58` | 本 session 最終確定 BLUEPRINT_DIR = `…/aya_r41_blueprints` |
| `indra/newview/app_settings/shaders/aya_r41_blueprints/set0/*.glsl` | 本 session 著作 3 file = set=1/2/3 著作の format テンプレート (= `#version 450` + `layout(std140, set=N, binding=M) uniform <Name> { ... };` + `void main() {}`) |

---

## §2 本 session 成果

### §2.1 PA-7.5 真 scope 達成

| 出力契約 | 実装箇所 | 内容 |
|---|---|---|
| `sky_placeholderV.glsl` 3 UBO に `std140` 補完 | `indra/newview/app_settings/shaders/aya_r41_exemplar/sky_placeholderV.glsl:18,26,32` | `layout(set=0, binding=0) uniform PerFrameMatrixUBO` → `layout(std140, set=0, binding=0) uniform PerFrameMatrixUBO` (同様に TextureMatrixUBO + PushConstants) |
| cmake module `BLUEPRINT_DIR` 縮小 | `AyaUboCodegen.cmake:56-58` (1 段目) | `…/shaders` (= 250 .glsl) → `…/aya_r41_exemplar` (= 2 .glsl) = legacy LL shader を scan 対象外に (= parser が legacy shader pattern を fail させる問題回避) |
| 実 indra/ tree 経由 smoke 3 path PASS | `/tmp/aya_ubo_pa7_5_smoke/` 独立 cmake project | Run #1 cache miss = 6 file emit / Run #2 cache hit / Run #3 `--force` = 6 file 再 emit |
| C++17 standalone compile PASS | `/tmp/aya_ubo_pa7_5_smoke/build/sanity_check.cpp` | `#include "codegen/ubo/ubo_index.inl"` + `ubo::lookup_block("PerFrameMatrixUBO")` link OK |

### §2.2 PA-8 set=0 真 scope 達成

| 出力契約 | 実装箇所 | 内容 |
|---|---|---|
| `aya_r41_blueprints/set{0,1,2,3}/` dir 構造作成 | `indra/newview/app_settings/shaders/aya_r41_blueprints/` | 1 UBO 1 file 規約 = `set{N}/<lowercase_name>.glsl`、set=1/2/3 は本 session では空 dir 配置のみ (= 次 session 着手地点) |
| FrameViewProj literal extract | `aya_r41_blueprints/set0/frame_view_proj.glsl` 新規 | source = `class1/deferred/pbropaqueF.glsl:198` `#ifdef LL_VULKAN_GLSL` block、9 member、verified identical = 5 sample (pbropaqueF / simpleNoColorV / previewPhysicsV / bumpV / simpleNoAtmosV) |
| FrameLights literal extract | `aya_r41_blueprints/set0/frame_lights.glsl` 新規 | source = `class1/windlight/atmosphericsV.glsl:33` `#ifdef LL_VULKAN_GLSL` block、9 member、verified identical = 5 sample (atmosphericsV / atmosphericsFuncs / simpleColorF / cinematic_bd/shadowUtil / sumLightsV) |
| FrameAtmosphere_Lighting literal extract | `aya_r41_blueprints/set0/frame_atmosphere_lighting.glsl` 新規 | source = `class1/windlight/atmosphericsF.glsl:37` `#ifdef LL_VULKAN_GLSL` block、20 member、verified identical = 4 sample (atmosphericsF / atmosphericsHelpersV / atmosphericsFuncs / atmosphericsHelpersF) |
| cmake module `BLUEPRINT_DIR` 最終確定 | `AyaUboCodegen.cmake:56-58` (2 段目) | `…/aya_r41_exemplar` → `…/aya_r41_blueprints` = exemplar も scan 対象外 (= exemplar は sub-step 3.3-B-δ pre-flight 専用、binding clash 回避) |
| 実 indra/ tree 経由 smoke 3 path PASS | `/tmp/aya_ubo_pa8_set0_smoke/` 独立 cmake project | Run #1 cache miss = 8 file emit 183ms / Run #2 cache hit / Run #3 `--force` = 8 file 再 emit 174ms / 38 member (= 9+9+20) / 0 hash collision |
| C++17 standalone compile PASS | `/tmp/aya_ubo_pa8_set0_smoke/build/sanity_check.cpp` | `lookup_block("FrameViewProj")` / `lookup_block("FrameLights")` / `lookup_block("FrameAtmosphere_Lighting")` 全 link OK + `ubo::UniformLocation{}` zero-init OK |

### §2.3 blueprint file format 規約 (= set=1/2/3 著作テンプレート)

各 blueprint file は以下の固定構造 (= 本 session で proven、PA-8 残作業の標準):

```glsl
// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8
// <BlockName> UBO blueprint (= set=N binding=M)
// Source: literal extract from <canonical .glsl path>:<line> ifdef LL_VULKAN_GLSL block
//         (verified identical across <K> sample sites = <site1> / <site2> / ...)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.<X>
//         set=N 帯 binding=M / cadence=<Frame|Program|Draw|Asset|Skin|Global>

#version 450

layout(std140, set = N, binding = M) uniform <BlockName>
{
    <type> <member>;
    ...
};

void main() {}
```

注意点:
- `#version 450` 必須 (= glslang preprocess の version 要求)
- `std140` 必須 (= 04 §3.1 strict 順守判断 A、不在は parser が CodegenError raise)
- `void main() {}` stub 必須 (= 副次的に確認、preprocess 単独では不要かもしれないが exemplar / 本 session 3 file 全 main() 同梱で proven、無 main() は未検証)
- comment ヘッダの「verified identical」記述は source grep で複数 site の member 列が完全一致することを確認した上で記録 (= literal extract の正当性証跡)

### §2.4 PA-7.5 設計差分 (= 1 件、必ず引き継ぐ)

| # | spec 記述 | 実装変更 | 理由 |
|---|---|---|---|
| 1 | PA-7 entry handoff §3 PA-8 row = "85 UBO blueprint 全 .glsl 起案 ... 既存 GLSL shader 内の uniform を 85 UBO 単位に再構成 (= AYA 判断 + 既存 shader 改修なし、blueprint 配置のみ)" | 実装は `indra/newview/app_settings/shaders/aya_r41_blueprints/set{0,1,2,3}/<lowercase_name>.glsl` 配下に 1 UBO 1 file = literal extract from `#ifdef LL_VULKAN_GLSL` block (= 既存 production blueprint と内容完全一致を保証) | 既存 #ifdef 内宣言を **再著作せず literal extract** で再現 = Phase 1.B redirect 層の host C++ side が「UBO 名一致」で wire できる (= GLSL UBO 名と redirect 層キーの一意性を Phase 1.A 時点で確定)。AYA さん message 5-6 で確定: c1 path (= literal extract) > c2 path (= fresh 著作で微差出る可能性) |

### §2.5 副次 finding (= self-verify 中の発見、§4.1 で AYA 判断)

`scripts/ubo_codegen/main.py:189-194` `_ubo_to_block_spec()` 詳細:

```python
def _ubo_to_block_spec(ubo: UboBlockDecl, layout: BlockLayout) -> BlockSpec:
    return BlockSpec(
        name=ubo.block_name,
        layout=layout,
        cadence_tag=_derive_cadence(ubo.block_name),
    )
```

defect: `ubo.layout_qual` dict (= `_parse_layout_qual()` の出力) には `set=0, binding=1` 等が既に extract 済だが、BlockSpec への forward が漏れている。BlockSpec の `descriptor_set: int = 0` / `binding: int = 0` default のまま metadata に書かれる。

**影響範囲**:
- Phase 1.A Exit Criteria (= 09 §4.2 "codegen 実行 PASS + 生成 header include + bind 不変動作") → literal 充足 (= bind 不変 = 既存 GL setUniform call site 触らず、codegen metadata の binding field は host code でまだ参照されない)
- Phase 1.B host wiring (= UBO redirect 層 dummy VkBuffer per binding point dispatch) → **本 defect で blocker** (= binding 値が全 0 では dispatch 不可能)

**修正 effort**: main.py 3-5 line 追加 (= `descriptor_set=ubo.layout_qual.get("set", 0), binding=ubo.layout_qual.get("binding", 0)` を BlockSpec 引数に追加) + unittest 1-2 件追加 (= 既存 test に binding extraction assert 追加)。parser は無修正で OK。

### §2.6 副次 finding 2 (= push_constant の扱い、§4.1 と独立)

`PushConstants` UBO (= sky_placeholderV.glsl の 3 つ目) は `layout(std140, push_constant) uniform PushConstants`。`set=N, binding=M` 不在で `push_constant` qualifier のみ。本 session smoke で metadata は `descriptor_set=0, binding=0` で記録された (= §2.5 defect で全 UBO に共通する behavior の一例)。

**Phase 1.B 設計上、push_constant は VkPushConstantRange (= descriptor set とは別 binding mechanism) で扱う必要**。本 finding は §2.5 修正と合わせて `layout_qual.get("push_constant", False) is True` 時は別 path 走らせる (= BlockSpec に `is_push_constant: bool` field 追加) を検討対象に。**本 session ではただ記録のみ** (= 次 session で §4.1 判断後の修正範囲に含めるか分離か AYA さん判断)。

---

## §3 次 session 着手 (= PA-8 残 82 件)

### §3.1 着手 1 line

§4.1 で AYA さんに binding=0 defect 修正タイミング (α/β/γ) を確認 → 確定後 **PA-8 set=1 (= MaterialUBO + MaterialUBO_Legacy 2 件 著作)** を本 session と同じ「literal extract + smoke 3 path + C++17 compile」protocol で実行。

### §3.2 PA-8 残 sub-step scope (= strict 線形)

| sub-step | 件数 | 出力 dir | source ref | 完了条件 |
|---|---|---|---|---|
| PA-8 set=1 | 2 (= MaterialUBO + MaterialUBO_Legacy、両 binding=0、subset 0/1 で区別) | `aya_r41_blueprints/set1/` | 06a §3.2 + grep `uniform MaterialUBO\b` / `uniform MaterialUBO_Legacy\b` | smoke 3 path + 5 file 増 (= 10 file = 6 aggregated/index + per-block layout 4) → C++17 compile + metadata に MaterialUBO + MaterialUBO_Legacy 含む確認 |
| PA-8 set=2 | 26 (= PerDrawUBO_* / PerProgramUBO_*、binding=0..25) | `aya_r41_blueprints/set2/` | 06a §3.3 + grep `uniform PerDrawUBO_\|uniform PerProgramUBO_` | smoke 3 path + 5+2+26+per-block = 34 file emit → C++17 compile + 全 26 UBO metadata 含む確認 |
| PA-8 set=3 | 54 (= <Name>UBO_Legacy、binding 各種) | `aya_r41_blueprints/set3/` | 06a §3.4 + grep `uniform [A-Za-z]*UBO_Legacy\b` | smoke 3 path + 5 aggregated + 85 per-block layout = 90 file emit → C++17 compile + 全 85 UBO 名前 + binding 全件 unique 確認 |
| PA-8 Exit Criteria 検証 | - | - | 09 §4.2 | (i) codegen 実行 PASS (= cache miss/hit/--force 3 path) / (ii) 既存 program 1 個 (= 例: pbropaque) で `#include "codegen/ubo/ubo_index.inl"` 経由 metadata 解決確認 / (iii) bind 不変動作 = 既存 GL setUniform call site が runtime で fail せず viewer launch 可能 (= 実 build 必要、3 OS 統一は本 phase の scope 外、Linux のみで PASS で literal 充足) |

### §3.3 source 既存 site の grep 計画 (= 次 session 効率化)

set=1/2/3 著作で **literal extract source 確定** のために、grep `uniform <BlockName> \{` で複数 site が出る場合は **全 site member 一致確認** を行う。本 session set=0 で確立した protocol:

1. 06a inventory § から UBO 名 + binding を確認
2. `grep "uniform <Name> {" -r indra/newview/app_settings/shaders/` で site 数取得
3. 上位 3-5 site の member 列を `grep -A 20` で並列確認 → 完全一致なら literal 採用
4. 不一致あれば AYA 判断 (= source-of-truth site 選択 or 別 UBO に分割)

### §3.4 binding 一意性検証 (= PA-8 後半の必須 self-verify)

set=2 / set=3 著作完了後、**全 85 UBO 内で `(set, binding)` の組が unique** であることを smoke 後の metadata で機械的に確認:

```bash
grep '^    { "' /tmp/aya_ubo_pa8_*_smoke/build/codegen/ubo/ubo_metadata.inl \
  | awk '{print $4, $5}' | sort | uniq -c | sort -rn | head
```

最初の数字が 1 でない行があれば binding 衝突 = §4.1 の binding extraction fix が反映されていない or 06a inventory に矛盾あり。

### §3.5 Claude 自走可、ただし α/β/γ 判断 1 件のみ AYA に問う

set=1/2/3 著作は本 session set=0 と同じ protocol、Claude 単独で進行可能。**ただし §4.1 の α/β/γ 判断は AYA さん必須**。

---

## §4 AYA 判断必要項

### §4.1 parser binding=0 defect 修正タイミング (= 1 件、§2.5 で詳細)

| option | 内容 | trade-off |
|---|---|---|
| **(α)** PA-8 内で先に修正 | 次 session entry 直後 main.py 3-5 line fix + unittest 1-2 件追加 + smoke 再走で set=0 metadata の binding が 0/1/2 に正しく書かれることを確認 → その後 set=1 着手 | set=0 を **正しい metadata で確定** + 次 session で set=1/2/3 著作と同時に「binding 一意性」検証も走る (= §3.4) / scope は微増だが本筋 (= Phase 1.B-ready metadata) に沿う |
| **(β)** Phase 1.B 着手時に修正 | 本 session の set=0 metadata は binding=0 のまま、PA-8 残 set=1/2/3 も全 binding=0 で着作 → Phase 1.B entry sub-task として fix 着手 → 全 set 再 codegen で正しい binding に置換 | PA-8 strict scope (= 85 blueprint 起案 + Exit Criteria 充足) を厳格に保つ / ただし Phase 1.B 着手時に **過去の binding=0 metadata に依存するテスト program が混ざる risk** |
| **(γ)** PA-7.6 として独立 sub-step 化 | 本 handoff doc commit 後、PA-7.6 = main.py fix + unittest 追加 + 既存 PA-7.5 + PA-8 set=0 smoke 再 PASS の独立 commit → その後 PA-8 set=1 着手 | strict 線形を明示維持 + Phase 1.A の defect closure が明確 (= commit 履歴で trace 可能) / commit 数増 (= PA-7.5/PA-8 set=0 batch + PA-7.6 fix の 2 commit が必要、本 handoff doc は PA-7.6 着手用に書き直し) |

**Claude 推奨 = (α)**。理由:
1. Phase 1.A Exit Criteria は literal には binding=0 でも充足 (= bind 不変 = host code 無関与) だが、PA-8 で set=2 (= 26 件) / set=3 (= 54 件) を全部書いた後で defect 発覚 → 全 metadata 再生成は cache invalidate で済むが、**「真の binding 一意性」を PA-8 内で能動検証** (= §3.4) するには修正済が前提
2. 修正自体は 3-5 line で trivial、unittest 1-2 件追加で proven、scope 創造は最小
3. (β) は Phase 1.B 着手時の前置作業として残置 = 集中力分散
4. (γ) は形式は綺麗だが commit 1 増は実害

### §4.2 push_constant 扱い (= 1 件、§2.6 で詳細、§4.1 と独立)

`PushConstants` UBO のような `layout(push_constant)` qualifier 持ち UBO は Phase 1.B で別 dispatch path (= VkPushConstantRange) になる。本 session metadata は §2.5 defect の副作用で `descriptor_set=0, binding=0` 記録。

**(a)** §4.1 と合わせて fix (= `is_push_constant: bool` field を BlockSpec に追加、§4.1 α/γ 時に同時実施)
**(b)** Phase 1.B 着手時にまとめて (= §4.1 β と同期)
**(c)** 後送り = 06a inventory + 09 §4.2 + 04 §5.1 に push_constant 扱い記述が現在ほぼゼロ、PA-8 後の独立 sub-task で「設計 chapter 追加」phase が要りそう

**Claude 推奨 = (c)**。理由: push_constant は exemplar 1 件のみ (= sky_placeholderV) で本番 blueprint (06a §3.1-§3.4) にゼロ件、Phase 1.A scope ではほぼ影響なし。06a inventory に push_constant の章追加 + Phase 1.B redirect 層設計に組み込みを次の独立 design phase で扱う。

---

## §5 引き継ぎ済 memory 18 件 (= 次 session で active)

- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ
- `feedback_design_phase_no_code_write` — 解禁済 (Phase 1.A 実装 phase)
- `feedback_no_scope_shrink` — PA-8 残 82 件は全著作、part-of で済まさない
- `feedback_self_verify_before_handoff` — 本 session で発動 (= §2.5 defect 発見)
- `feedback_no_claude_coauthor` — 本 handoff doc 含め全 commit 共著行不在
- `feedback_one_step_at_a_time` — §4.1 / §4.2 を 1 件ずつ AYA に問う
- `feedback_doubt_self_first` — 本 session で発動 (= 全 UBO binding=0 を当然視せず疑った結果 §2.5 発見)
- `feedback_proactive_handoff` — 本 session で発動 (= 残作業 82 件は次 session に handoff)
- `feedback_no_auto_commit` — 本 handoff doc + 6 file 改変は AYA 明示指示後 batch
- `feedback_remove_verification_logs` — 本 session 追加 log/diagnostic 不在 (= 該当なし)
- `feedback_build_only_verified` — 本 session 全 verification PASS で記録
- `feedback_tests_dir_never_commit` — 本 session tests/ 改変なし
- `project_ayastorm_r41_vulkan_migration` — PA-7.5 + PA-8 set=0 完了 milestone
- `project_ayastorm_r41_design_principles` — 04 §3.1 strict (std140 only) を本 session で 3 箇所適用
- `feedback_ubo_migration_one_at_a_time` — set=0 3 件を本 session で完了、set=1/2/3 は次 session 以降に分割
- `project_build_procedure` — PA-8 Exit Criteria 検証で実 indra/ build 必要時に参照
- `feedback_use_agents_proactively` — set=1/2/3 大量 grep + member 一致確認は agent 並列推奨
- `project_ayastorm_three_platforms` — Exit Criteria 検証 (= viewer launch) は 3 OS 揃え別 phase

---

## §6 次 session 着手 1 line

**§4.1 (α/β/γ) + §4.2 (a/b/c) を AYA さんに確認 → (α) 確定なら main.py:189-194 に `descriptor_set=ubo.layout_qual.get("set", 0), binding=ubo.layout_qual.get("binding", 0)` 追加 + unittest 1-2 件追加 + smoke 再走 → PA-8 set=1 (= MaterialUBO + MaterialUBO_Legacy 2 件 著作、binding=0/subset で区別) 着手。**
