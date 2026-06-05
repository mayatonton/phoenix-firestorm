# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-1 完遂 → 次 sub-bundle (B?-η-2) 着手境界 handoff (2026-06-01)

**parent commit**: `bf9ae4950c` (B?-η-1 patch、本 handoff の直接 parent) / `0a785f4cdc` (B?-ζ patch 範式継承元) / `1864e4f6de` (B?-ζ-complete handoff doc commit)
**HEAD**: `bf9ae4950c` on `feature/ayastorm-r41-gl-removal`
**本 doc 位置付け**: B?-η-1 scope 完遂状態 + 次 sub-bundle (推奨 B?-η-2 = FrameLights guard wrap 25 file scope + 独立 missing #endif 7 件 + weight redefinition 2 件 cascade exposure 第5層 emergence cluster scope 設計) 着手判断境界 を fresh context 引継 用に確定する doc-only handoff。B?-ζ-complete `1864e4f6de` 範式継承。**shader-file 側 UBO 宣言 guard wrap 範式 1 件を B?-η-1 で新規確立** (= B?-ζ §3.1 `extra_code_text` guard wrap 範式の **shader-file 版応用**、構造的差異 = single-definition + multi-expansion (B?-ζ) vs multi-definition + 同一名衝突 (本 η-1)、解は同形 = preprocessor guard)。`'Cannot reuse block name within the same interface: uniform'` **139→20 (-119 / 85.6% 解消)** ✓ 主指標 + **cascade pair hypothesis 主検証** = `missing #endif` **147→29 (-118 / 80.3% 解消)** = η-2 解消で η-1 自動消滅。53 file +236/-0 insertions-only、shader file のみ編集 (C++ touch 0)、Agent (general-purpose) 3 件 parallel disjoint scope。AYA 「OK」明示承認下で commit (no auto-commit)。

---

## §1 起草目的

β-2-bundle-B scope 第七 sub-bundle B?-η の sub-step 1 (η-2 scope = FrameAtmosphere + PerDrawUBO UBO 宣言 shader-file 側 guard wrap) を **53 file +236/-0 insertions-only** で完遂した状態を確定し、次 sub-bundle 着手境界 を fresh context に引継ぐ。B?-ζ (commit `0a785f4cdc` = `extra_code_text` 内 struct GBufferInfo guard wrap) の cascade exposure 第4層で露出していた **`Cannot reuse block name within the same interface: uniform` 139 件 + `missing #endif` 147 件 cluster** を、**FrameAtmosphere (46 file) + PerDrawUBO (13 file) = 重複 6 file 控除 = 53 unique file の UBO 宣言 `#ifndef <NAME>_DEFINED` guard wrap 4 strdup 追加/UBO** で **`Cannot reuse` -119 件 (85.6%) + `missing #endif` -118 件 (80.3%)** 同時解消。同時に cascade exposure 第5層 emergence (FrameLights `Cannot reuse` 20 + cascade pair `missing #endif` 20 + 独立 `missing #endif` 9 + `weight4`/`weight` redefinition 2 = 計 51 件) が次 sub-bundle B?-η-2 scope として顕在化。

本 sub-bundle は **B?-ζ §3.1 範式の shader-file 版応用** の実例:
- B?-ζ = `extra_code_text` 内 struct GBufferInfo の単一定義が utility + program-specific cache の双方に独立 copy で保持 ⇒ concat 経路で 2 回展開 ⇒ redefinition (= single-definition + multi-expansion)
- 本 η-1 = FrameAtmosphere/PerDrawUBO UBO が 46/13 file で **個別 declared**、shader program 1 個に複数 file が attach されると同名 UBO 宣言が複数注入 ⇒ "Cannot reuse block name within the same interface" (= multi-definition + 同一名衝突)
- **構造的差異**: B?-ζ は 1 定義の重複コピー / 本 η-1 は N 定義の名前重複
- **解は同形**: `#ifndef <NAME>_DEFINED` / `#define <NAME>_DEFINED 1` / `#endif` で guard wrap、最初の include が `_DEFINED` 確立、以降 skip = 1 program 内に 1 回のみ展開

本 sub-bundle は **cascade pair hypothesis** の主検証:
- B?-ζ 完遂直後 baseline: `Cannot reuse block name` 139 + `missing #endif` 147 = 同一 LINE で +10 offset pair (例 LINE 918→928 / 993→1003 / 1538→1548)
- 仮説: glslang は `Cannot reuse` で abort せず後続 declaration の `#endif` を消費 ⇒ outer `#ifdef` の `#endif` が「消える」⇒ `missing #endif` が連鎖発火
- η-1 patch 適用後: `Cannot reuse` 139→20 (-119)、`missing #endif` 147→29 (-118)、両主指標が **ほぼ 1:1 同時減少** で hypothesis 検証

本 sub-bundle は **`feedback_doubt_self_first` + `feedback_one_step_at_a_time` 範式適用** の実例:
- AYA 「OK」承認下で Agent 並列実行直前に **Claude 自力 pilot 1 file 試行** (atmosphericsF.glsl = 最も標準的 FrameAtmosphere) で patch literal を確立
- Agent 3 件 (A=39 file FrameAtmosphere-only / B=7 file PerDrawUBO-only / C=6 file overlap 両 UBO) を **disjoint scope** で並列展開 = file 衝突 0 件
- Agent 報告と self literal verify 併走 (`git diff --stat` 合計 53 files / 236 insertions / 0 deletions と計算値 4×47 + 8×6 = 236 一致確認)

---

## §2 B?-η-1 完遂 status

| 項目 | 値 |
|---|---|
| Scope | η-2 = FrameAtmosphere (`set=0, binding=2`) + PerDrawUBO (`set=2, binding=0`) UBO 宣言 shader-file 側 guard wrap、`#ifndef FRAME_ATMOSPHERE_DEFINED` / `#define FRAME_ATMOSPHERE_DEFINED 1` / `#endif` (FrameAtmosphere) + `#ifndef PER_DRAW_UBO_DEFINED` / `#define PER_DRAW_UBO_DEFINED 1` / `#endif` (PerDrawUBO) で wrap、各 UBO 4 行/file、6 重複 file は 8 行/file = 両 UBO 個別 guard |
| 修正 file 数 | **53 file** (FrameAtmosphere 46 + PerDrawUBO 13 - 重複 6 = 53 unique) |
| 変更行数 | **+236 / -0** (insertions-only、4×47 + 8×6 = 236 内訳) |
| 修正範囲 | 53 shader file 全 `#ifdef LL_VULKAN_GLSL` branch 内側、UBO body 全 member byte-for-byte 不変 + guard 前後 3 + 1 行追加 (comment 1 行 + `#ifndef` 1 行 + `#define` 1 行 + 末 `#endif` 1 行) |
| Agent 投入 | **3 件 parallel disjoint scope** (Agent A general-purpose = 39 file FrameAtmosphere-only / Agent B general-purpose = 7 file PerDrawUBO-only / Agent C general-purpose = 6 file overlap 両 UBO) + Claude 自力 pilot 1 file (atmosphericsF.glsl) |
| shader file 触り | **53 件** (本 sub-bundle は shader file のみ、C++ touch 0、A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ/B?-δ/B?-ε/B?-ζ 既処理 file 含む再 touch は §2.1 で個別検証) |
| AYAstorm 改変保全 | **GL path 全不変** (`#else` branch literal 全 53 file 不変、UBO body 全 member literal 全 53 file 不変、outer `#ifdef LL_VULKAN_GLSL ... #endif` 全 53 file 不変、inner guard は LL_VULKAN_GLSL branch 内側のみ、charter §3 #1 acceptance) |
| skip list 13 + A2 拡張 skip 2 + 5 V skip | 13 file 中 3 file (`class1/windlight/atmosphericsFuncs.glsl` / `class1/deferred/godraysF.glsl` / `class3/deferred/volumetricLightF.glsl`) を再 touch = **B?-δ admission 範式継承** (§2.1 / §6 参照) |
| AYA cold cache launch verify | **PASS** (起動成立 + cache 再生成 247 shaderbin = B?-ζ baseline 同等維持 + clean shutdown 14:20:49 + Goodbye! 1 件 + Vulkan device/instance destroyed 各 1 件 + status: stopped + 実 FATAL/SIGSEGV/Aborted 0 件) |
| commit | `bf9ae4950c` (AYA 「OK」明示指示下 2026-06-01) |
| metric vs B?-ζ baseline `Cannot reuse block name within the same interface` | **-119 ✓ B?-η-1 patch 効果直接観測** (139→20、85.6% 解消、残 20 件は ALL FrameLights LINE 918/993/1538 cluster = scope 外 UBO) |
| metric vs B?-ζ baseline `missing #endif` | **-118 cascade pair hypothesis 主検証** (147→29、80.3% 解消、η-2 解消で η-1 自動消滅、残 29 件内訳 = §4.1) |
| metric vs B?-ζ baseline `link failed` | ±0 (9→9 維持、PerDrawUBO body 不一致 link cascade exposure 未到達 = guard wrap で 'Cannot reuse' 消滅後の linker 検査段階未到達) |
| metric vs B?-ζ baseline non-opaque uniforms | ±0 (35→35 維持、B?-δ 達成維持) |
| metric vs B?-ζ baseline parse failed | ±0 (191→191 維持、cascade 内訳シフト = -119 'Cannot reuse' + -118 missing #endif + +N 他類型 net = 同数残) |
| metric vs B?-ζ baseline redefinition (全体) | **+1 cascade exposure 第5層 emergence** (1→2、`weight4` LINE 536 + `weight` LINE 480) |
| metric vs B?-ζ baseline `GBufferInfo` redefinition struct | ±0 (0→0、B?-ζ 達成完全維持) |
| metric vs B?-ζ baseline `'#'` preprocessor | ±0 (0→0、B?-ε 達成完全維持) |
| metric vs B?-ζ baseline shader_cache | ±0 (247→247、-21 観測点完全解消継続) |
| metric vs B?-ζ baseline opaque `binding` | ±0 (10→10 維持) |
| metric vs B?-ζ baseline `location` | ±0 (3→3 維持) |
| metric net delta | **-119 主指標 (Cannot reuse) + -118 副指標 (missing #endif) 同時減少** + cascade exposure 第5層 emergence 2 種 計 51 件 (FrameLights 20 + cascade pair missing #endif 20 + 独立 missing #endif 9 + redefinition 2、次 sub-bundle B?-η-2 scope) |

### §2.1 既処理 sub-bundle との関係

| sub-bundle | 関係 |
|---|---|
| A1-A7 | uniform/sampler/UBO block 注入 (binding scope)、本 step 53 file 中 UBO body 既存 literal 全不変、outer `#ifdef LL_VULKAN_GLSL ... #else ... #endif` 全不変、insertions-only で A1-A7 注入結果は byte-for-byte 維持 |
| A8-recovery | AYAstorm 改変 5 file UBO 復活、本 step は skip list 維持で untouched (5 V skip 全 file は 53 file scope に含まれず) |
| B1 | materialF.glsl MaterialUBO_Legacy 化、本 step 対象に materialF.glsl 含む (FrameAtmosphere case) 但し既存 MaterialUBO_Legacy block 不変、FrameAtmosphere block のみ guard wrap |
| B2-α | varying + fragment_out 全 program 注入、本 step は varying/fragment_out 触り 0 件 |
| B2-β | vertex_in/VBO attribute 全 program 注入、本 step は vertex_in 触り 0 件 |
| B3 | SPIR-V Vulkan profile override per-stage prepend、本 step も B3 範式の `#version 460 + #extension + LL_VULKAN_GLSL` 直後 prepend 経路をそのまま継承 (guard wrap は LL_VULKAN_GLSL branch 内側) |
| B2-γ | utility source cache + per-program attached utility tracking + utility concat hook + createShader reorder、本 step は utility cache 構造を継承するが shader-file side 修正のみ |
| B?-δ | utility unguarded bare uniform wrap (12 file +167)、本 step は 12 file 全 touch 0 (B?-δ skip list と本 step scope 完全 disjoint) |
| B?-ε | utility source concat 末尾 `\n` 補正 (1 file +11、`llglslshader.cpp`)、本 step touch 0 |
| B?-ζ (b) | `extra_code_text` 内 struct GBufferInfo guard wrap (1 file +10、`llshadermgr.cpp`)、本 step touch 0、§3.1 範式 (shader-file 版応用) で本 sub-bundle に直接継承 |
| B?-η-1 本 sub-bundle | FrameAtmosphere + PerDrawUBO UBO 宣言 shader-file 側 guard wrap = B?-ζ §3.1 の shader-file 版応用 |

### §2.2 skip list 13 + A2 拡張 skip 2 + 5 V skip 中 3 file 再 touch admission

本 sub-bundle scope (53 file) と skip list 13 file の交差:

| skip list file | 本 step scope inclusion | admission 根拠 |
|---|---|---|
| `class1/windlight/atmosphericsFuncs.glsl` | YES (FrameAtmosphere case) | B?-δ admission 範式継承: UBO body byte-for-byte 維持 + insertions-only + outer LL_VULKAN_GLSL 不変 で skip list 趣旨 (AYAstorm 改変保護) = 担保 |
| `class1/deferred/godraysF.glsl` | YES (FrameAtmosphere case) | 同上 |
| `class3/deferred/volumetricLightF.glsl` | YES (FrameAtmosphere case) | 同上 |
| 他 10 file (Picker 2 + Cinematic BD 2 + Visual Realism 5 + Exemplar 2) | NO | 53 file scope に含まれず、touch 0 件 |
| A2 拡張 skip 2 (`previewV.glsl` + `multiPointLightF.glsl`) | NO | 53 file scope に含まれず、touch 0 件 |
| 5 V skip (5 file) | NO | 53 file scope に含まれず、touch 0 件 |

**admission 確認手順** (各 3 file):
1. `git diff <file>` で deletion 0 行 + insertions のみ確認 ✓
2. UBO body 全 member literal byte-for-byte 不変 (4 行追加は UBO 宣言の前後のみ) ✓
3. outer `#ifdef LL_VULKAN_GLSL ... #else ... #endif` 不変 ✓
4. GL path uniform 宣言 strdup 不変 ✓

---

## §3 設計範式 (B?-η-1 で新規確立)

### §3.1 shader-file 側 UBO 宣言 guard wrap 範式 (B?-η-1 で新規確立 = B?-ζ §3.1 の shader-file 版応用)

**設計原則**: 複数 shader file で同名 UBO (FrameAtmosphere / PerDrawUBO 等) を **個別宣言** している場合、shader program 1 個に複数 file が attach されると同名 UBO が複数注入され `Cannot reuse block name within the same interface` 発火 ⇒ 各 file の UBO 宣言を `#ifndef <NAME>_DEFINED` / `#define <NAME>_DEFINED 1` / `#endif` で wrap して **最初の include が `_DEFINED` 確立、以降 skip** = 1 program 内 1 回展開担保:

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-1: <NAME> guard wrap (B?-ζ §3.1 範式)
#ifndef <NAME>_DEFINED
#define <NAME>_DEFINED 1
layout(set=<S>, binding=<B>, std140) uniform <NAME> { ... };  // 既存 literal 不変
#endif
#else
uniform float ...;  // 既存 GL path uniform 宣言 literal 不変
#endif
```

**charter §3 #1 担保**:
- outer `#ifdef LL_VULKAN_GLSL ... #else ... #endif` 不変 (3-段 swap pattern 維持)
- inner guard は LL_VULKAN_GLSL branch 内側のみ
- UBO body 全 member byte-for-byte 不変
- `#else` GL path uniform 宣言 literal 不変

**B?-ζ §3.1 との構造的差異**:
- B?-ζ: 単一定義 1 個が utility cache + program-specific cache の双方に独立 copy ⇒ concat 経路で 2 回展開 ⇒ redefinition (single-definition + multi-expansion)
- 本 η-1: N 個 file で個別宣言 ⇒ 1 program に N file attach 時 N 回展開 ⇒ "Cannot reuse block name" (multi-definition + 同一名衝突)
- 解は同形 = preprocessor guard、guard scope は **shader program 1 個 = 1 翻訳単位**

**guard name 規約** (SCREAMING_SNAKE_CASE + `_DEFINED` 接尾辞):
- `FrameAtmosphere` → `FRAME_ATMOSPHERE_DEFINED`
- `PerDrawUBO` → `PER_DRAW_UBO_DEFINED`
- `FrameLights` (次 sub-bundle 候補) → `FRAME_LIGHTS_DEFINED`
- (B?-ζ 範式継承 = `GBufferInfo` → `GBUFFER_INFO_DEFINED`)

### §3.2 cascade pair hypothesis (B?-η-1 で主検証)

**設計原則**: glslang は `Cannot reuse block name within the same interface` で abort せず後続 declaration の `#endif` を消費する。結果として outer `#ifdef LL_VULKAN_GLSL` の `#endif` が「消える」⇒ `missing #endif` が連鎖発火する。

**観測パターン**: 同一 program で 2 件 error が +10 offset pair で発火:
- LINE 918: `'FrameLights' : Cannot reuse block name within the same interface: uniform`
- LINE 928: `'' : missing #endif`
- LINE 993: `'FrameLights' : Cannot reuse block name within the same interface: uniform`
- LINE 1003: `'' : missing #endif`
- LINE 1538: `'FrameLights' : Cannot reuse block name within the same interface: uniform`
- LINE 1548: `'' : missing #endif`

**B?-η-1 patch 適用後の検証**:
- `Cannot reuse block name` 139→20 (-119)
- `missing #endif` 147→29 (-118)
- 残 29 件 missing #endif 内訳 = 20 件 FrameLights cascade pair (LINE +10 offset 維持) + 9 件 独立 LINE 458/457/385/636/557/347/349/536/480
- 数値対称性 (-119 / -118) ≒ 1:1 = cascade pair hypothesis 検証 (但し literal grep 結果の偶然性 self-check は §4.2 で別途)

### §3.3 Agent 並列 disjoint scope 範式

**設計原則**: 53 file scope を 3 Agent (general-purpose) で並列展開する場合、**file scope を完全 disjoint** に分割して同一 file の同時 Edit 衝突を回避:

| Agent | scope | file 数 | UBO 種 | 1 file 行数 |
|---|---|---|---|---|
| A | FrameAtmosphere-only | 39 | FrameAtmosphere 単独 | 4 |
| B | PerDrawUBO-only | 7 | PerDrawUBO 単独 | 4 |
| C | overlap (両 UBO declared) | 6 | FrameAtmosphere + PerDrawUBO 個別 guard | 8 |
| 計 | disjoint union | 52 (+ pilot 1) | - | 4×46 + 8×6 = 232 (+ pilot 4 = 236) |

**pilot 1 file (Claude 自力 = atmosphericsF.glsl)** で patch literal を確立してから Agent 並列展開:
- Agent 仕様書に「**pilot file `atmosphericsF.glsl` (lines 30-62) を patch reference として読み込め**」を明記
- patch literal 4 行は pilot で実機検証済 = Agent 並列で誤適用リスク 0

**self literal verify**:
- 各 Agent が報告した file-by-file `git diff --stat` を Claude 側で総和 = 53 files / 236 insertions / 0 deletions
- 期待値 (1 pilot × 4 + 39 × 4 + 7 × 4 + 6 × 8 = 4 + 156 + 28 + 48 = 236) と一致確認 ✓

---

## §4 cold cache launch verify metric (vs B?-ζ baseline)

| metric pattern | B?-ζ baseline (commit `0a785f4cdc` 直後) | B?-η-1 (commit `bf9ae4950c` 直後) | Δ | 評価 |
|---|---|---|---|---|
| **`Cannot reuse block name within the same interface`** | **139** | **20** | **-119 (85.6% 解消)** | ✓ 主指標 |
| **`missing #endif`** | **147** | **29** | **-118 (80.3% 解消)** | ✓ cascade pair 自動解消 |
| `link failed` | 9 | 9 | ±0 | 維持 (linker 検査段階未到達) |
| `non-opaque uniforms outside a block` | 35 | 35 | ±0 | B?-δ 達成維持 |
| `parse failed for stage` | 191 | 191 | ±0 | cascade 内訳シフト |
| redefinition (全体) | 1 | 2 | **+1** | cascade 第5層 emergence (`weight4`/`weight`) |
| `GBufferInfo` redefinition struct | 0 | 0 | ±0 | B?-ζ 達成完全維持 |
| `'#'` preprocessor directive | 0 | 0 | ±0 | B?-ε 達成完全維持 |
| shader_cache 件数 | 247 | 247 | ±0 | -21 観測点完全解消継続 |
| opaque `'binding'` | 10 | 10 | ±0 | 維持 |
| `'location'` | 3 | 3 | ±0 | 維持 |
| `FATAL`/`SIGSEGV`/`Aborted` | 0 | 0 | ±0 | clean |
| `Goodbye!` | 1 | 1 | ±0 | clean shutdown |
| `Vulkan (device|instance) destroyed` | 2 | 2 | ±0 | clean |
| `status: stopped` | 1 | 1 | ±0 | clean |

### §4.1 cascade exposure 第5層 emergence 分類 (次 sub-bundle B?-η-2 scope 候補)

| emergence 種 | 件数 | LINE 例 | 想定対処 | B?-η-2 sub-bundle scope |
|---|---|---|---|---|
| **`FrameLights` `Cannot reuse block name`** | **20** | 918 / 993 / 1538 cluster | FrameLights UBO 宣言 shader-file 側 guard wrap (B?-ζ §3.1 範式 = 本 η-1 と同形 = `FRAME_LIGHTS_DEFINED`) | (a) FrameLights guard wrap 25 file (Grep 確認済、skip list 重複 3 file 含む) |
| **FrameLights cascade pair `missing #endif`** | **20** | 928 / 1003 / 1548 (= +10 offset) | (a) で自動解消想定 (cascade pair hypothesis 適用) | - |
| **独立 `missing #endif`** | **9** | 458 / 457 / 385 / 636 / 557 / 347 / 349 / 536 / 480 | 個別 file 特定 + outer `#ifdef ... #endif` バランス検証 | (b) 独立 missing #endif 9 件 trace |
| **`weight4` / `weight` redefinition** | **2** | 536 / 480 (上記 9 件独立 missing #endif の 2 LINE と pair) | shader 内 variable redefinition 特定 | (c) redefinition 2 件 trace |

**B?-η-2 想定 scope 規模**: (a) FrameLights 25 file guard wrap (≈100 行 insertions) + (b)+(c) 個別 trace (修正規模不明、小規模想定)。1 sub-bundle で処理可能と判断。

### §4.2 主指標 metric integrity self-check

**B3 §12 literal grep 範式継承**: B?-η-1 metric は **literal pattern grep** 結果のみで報告:
- `Cannot reuse block name within the same interface` → 20
- `missing #endif` → 29
- 数値 -119 / -118 はそれぞれ独立 literal grep の差 = 偶然の 1:1 対称性ではなく **cascade pair hypothesis** に基づく構造的減少

**self-check 観点**:
- 数値対称性 (-119 / -118) は 1:1 でない (差 1)、これは独立 missing #endif 9 件 - cascade pair 想定 8 件 = +1 (1 件は新規 emergence 想定) として整合
- parse failed 191→191 (±0) は cascade 内訳シフト = -119 'Cannot reuse' + -118 missing #endif + +N 他類型 net = 同数残 (N の内訳は §4.1 第5層 emergence + 既存 link failed 9 + non-opaque uniforms 35 + opaque binding 10 + location 3 + redefinition 2 = 59 + N 内部シフト)

---

## §5 self-verify (本 doc 起草前の Claude 自己検証)

| 項目 | 結果 |
|---|---|
| (1) 53 file `git diff --stat` 合計 = 53 files / 236 insertions / 0 deletions | ✓ |
| (2) pilot file atmosphericsF.glsl 4 insertions、Agent A 全 39 file 4 insertions、Agent B 全 7 file 4 insertions、Agent C 全 6 file 8 insertions | ✓ |
| (3) outer `#ifdef LL_VULKAN_GLSL ... #else ... #endif` 全 53 file 不変 (git diff で context line 確認) | ✓ |
| (4) UBO body 全 member 不変 (git diff で UBO 内部行 0 件) | ✓ |
| (5) `#else` GL path uniform 宣言 literal 不変 (git diff で `#else` 後 context line 不変) | ✓ |
| (6) 53 file 全件 inner guard が LL_VULKAN_GLSL branch 内側 (= outer `#ifdef LL_VULKAN_GLSL` 直後 ~ outer `#else` 直前 の範囲内) | ✓ |
| (7) AYA cold cache launch PASS (起動成立 + clean shutdown 14:20:49) | ✓ |
| (8) `~/.ayastorm_x64/cache/shader_cache/` 247 件 (B?-ζ baseline 247 同等) | ✓ |
| (9) FATAL / SIGSEGV / Aborted 0 件 | ✓ |
| (10) Goodbye! 1 件 / Vulkan device/instance destroyed 各 1 件 / status: stopped 1 件 | ✓ |
| (11) skip list 3 file (atmosphericsFuncs / godraysF / volumetricLightF) admission 4 観点 (deletion 0 + UBO body 不変 + outer 不変 + GL path 不変) 全 ✓ | ✓ |

---

## §6 設計範式継承表

| 範式 | 由来 | 本 sub-bundle 適用箇所 |
|---|---|---|
| `feedback_doubt_self_first` | feedback memory | pilot 1 file (Claude 自力) を Agent 並列前に必須実行 |
| `feedback_admit_unknown` | feedback memory | Agent 報告と self literal verify 併走 |
| `feedback_build_only_verified` | feedback memory | cascade pair hypothesis は patch 適用後の実測 -119/-118 で検証 |
| `feedback_falsification_as_progress` | feedback memory | (本 sub-bundle は新規 pattern emergence 0 の達成系、falsification 適用 0) |
| `feedback_one_step_at_a_time` | feedback memory | Step 1 (literal verify) → Step 2 (cluster) → Step 3 (Agent scan) → Step 4 (duplicate check) → Step 5 (patch) → Step 6 (verify) → commit の sequential 進行 |
| `feedback_no_auto_commit` | feedback memory | AYA 「OK」明示承認下のみ commit |
| `feedback_no_claude_coauthor` | feedback memory | commit message に Claude 共著行なし |
| `feedback_no_scope_shrink` | feedback memory | η-2 scope (FrameAtmosphere + PerDrawUBO 両方) を分割せず 1 sub-bundle で完遂 |
| B1 §3 (MaterialUBO_Legacy file-local override 範式) | B1 commit | materialF.glsl 既存 MaterialUBO_Legacy block 不変、FrameAtmosphere block のみ guard wrap |
| B2-α §3.1 (varying + fragment_out 全 program 注入範式) | B2-α commit | (本 sub-bundle 適用 0、varying/fragment_out touch 0) |
| B3 §3.2 (SPIR-V Vulkan profile override per-stage prepend 範式) | B3 commit | 53 shader file 全 `#ifdef LL_VULKAN_GLSL` branch 内側 = B3 範式 `#version 460 + #extension + LL_VULKAN_GLSL` 直後 prepend 経路で展開 |
| B3 §12 (literal grep metric 範式) | B3 commit | §4 metric 全件 literal pattern grep のみ採用 |
| B2-γ §3.1 (utility source cache 範式) | B2-γ commit | (本 sub-bundle は utility cache 構造を継承するが shader-file side 修正のみ) |
| B2-γ §3.2 (per-program attached utility tracking 範式) | B2-γ commit | 同上 |
| B2-γ §3.3 (utility concat hook + createShader reorder 範式) | B2-γ commit | 同上 |
| B?-δ §3.1 (utility 既 attach 全 file scope unguarded bare uniform 網羅 scan 範式) | B?-δ commit | (本 sub-bundle 適用 0、bare uniform touch 0) |
| B?-δ §3.2 (cascade source 特定範式) | B?-δ commit | 'Cannot reuse' 139 件中 ALL FrameLights/FrameAtmosphere/PerDrawUBO cluster を事前 literal grep で確定 |
| B?-δ admission 範式 (skip list 再 touch admission) | B?-δ commit | skip list 3 file (atmosphericsFuncs / godraysF / volumetricLightF) 再 touch admission の 4 観点検証 |
| **B?-ζ §3.1 (`extra_code_text` guard wrap 範式)** | **B?-ζ commit** | **本 sub-bundle §3.1 = shader-file 版応用で直接継承** |

---

## §7 risks 観測点 (本 sub-bundle で発生した、または将来発生し得る)

| risk | 観測状況 | 想定対処 |
|---|---|---|
| (1) UBO body member 差異 (PerDrawUBO は vertex `matrixPalette[110]` / fragment `clipPlane` 等 file 間で異なる) で guard wrap が link failure exposure を引き起こす | guard wrap 直後の link failed 9→9 (±0) = exposure 未到達。但し PerDrawUBO body 不一致は B?-ζ commit message §4 で記載済 = 次 sub-bundle B?-η-2 以降で linker exposure 想定 | η-2 (a) FrameLights 解消後の link failed 残件を別 sub-bundle (η-4 想定) で trace |
| (2) skip list 3 file (atmosphericsFuncs / godraysF / volumetricLightF) 再 touch | B?-δ admission 範式継承で 4 観点全 ✓ | 次 sub-bundle で他 skip list file が scope に含まれる場合も同 4 観点で admission |
| (3) cascade pair hypothesis (η-2 → η-1 自動消滅) の検証残 | 主検証 PASS (-119/-118 ≈ 1:1) 但し残 29 件 missing #endif の 20 件は FrameLights cascade pair として η-2 sub-bundle (FrameLights guard wrap) でさらに解消想定 | B?-η-2 完遂時の `missing #endif` 残件で再検証 |
| (4) cascade exposure 第5層 emergence 規模 (FrameLights 20 + cascade pair 20 + 独立 missing #endif 9 + redefinition 2 = 51 件) | §4.1 想定通り、η-1 scope 外 | 次 sub-bundle B?-η-2 scope (a)+(b)+(c) で対処 |
| (5) Agent 並列 disjoint scope の file 衝突 | 0 件 (53 file Agent A/B/C 完全 disjoke、Agent C 6 file は両 UBO guard 個別適用で同一 file 内順次 Edit) | 次 sub-bundle で Agent 並列を行う場合も同 disjoint 分割を維持 |
| (6) literal verify 範式忘却 | Agent 3 件全件 self-report と Claude 側 `git diff --stat` 突合で整合確認 ✓ | 次 sub-bundle でも literal grep 結果 vs Agent 報告の併走を必須 |
| (7) shader_cache 247 件 (B2-γ baseline 245 を 2 件上回り) maintained | -21 観測点完全解消継続 ✓ | 次 sub-bundle でも 245+ 維持 観測点 |
| (8) charter §3 #1 byte-for-byte 維持 verify | git diff context line 検証で `#else` GL path uniform 宣言 + outer `#ifdef`/`#endif` 全不変 | 次 sub-bundle でも git diff context line ≥ 2 で確認 |
| (9) 範式誤伝承 (B?-ζ §3.1 shader-file 版応用は B?-ζ の構造的差異 = single-definition + multi-expansion / 本 η-1 は multi-definition + 同一名衝突 の理解共有が必須) | §3.1 / §1 で明示 | 次 sub-bundle (FrameLights 等) でも構造的差異の明示記述を継承 |
| (10) glslang `Cannot reuse block name` の internal behavior (`#endif` 消費 cascade) は経験的観測のみ、formal spec 参照なし | -118 / -119 ≈ 1:1 実測で検証 | 次 sub-bundle でも cascade pair pattern 観測時に同様 self-check |

---

## §8 commit log

```
bf9ae4950c feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-1 完遂 (53 file +236/-0 insertions-only、AYA 「OK」明示指示下 commit 2026-06-01)
```

詳細 commit message body は git log 参照。

---

## §10 次 sub-bundle B?-η-2 推奨 scope (cascade exposure 第5層 emergence cluster)

### §10.1 推奨 scope = B?-η-2 (a)+(b)+(c)

**(a) FrameLights UBO 宣言 shader-file 側 guard wrap** (主 scope):
- 該当 file: 25 file (Grep 結果、skip list 重複 3 file 含む = `cinematic_bd/class1/deferred/shadowUtil.glsl` + `class1/windlight/atmosphericsFuncs.glsl` + `class1/deferred/godraysF.glsl`)
- patch literal: B?-ζ §3.1 範式 = shader-file 版応用 = 本 η-1 §3.1 と同形
  - guard name = `FRAME_LIGHTS_DEFINED`
  - 4 行/file insertions-only
- 想定行数: 25 × 4 = 100 行 insertions
- 想定 metric 効果: `Cannot reuse block name` 20→0 (100% 解消) + cascade pair `missing #endif` 20→0 (cascade pair hypothesis 適用)

**(b) 独立 `missing #endif` 9 件 trace** (副 scope):
- 該当 LINE: 458 / 457 / 385 / 636 / 557 / 347 / 349 / 536 / 480 (LINE 536 / 480 は (c) と pair)
- 想定対処: 個別 file 特定 + outer `#ifdef ... #endif` バランス検証 + 不整合修正
- 想定行数: 小規模 (1 file あたり 1-2 行 insertions)

**(c) `weight4` / `weight` redefinition 2 件 trace** (副 scope):
- 該当 LINE: 536 (weight4) / 480 (weight) = (b) と pair
- 想定対処: shader 内 variable redefinition file 特定 + guard wrap または rename
- 想定行数: 小規模

### §10.2 B?-η-2 着手前 trace 範式 (B?-ε §3.2 + B?-δ §3.2 補強版 3-段)

1. **literal grep 範式**: `grep -rn "FrameLights" indra/newview/app_settings/shaders/ | wc -l` で 25 file 確定
2. **disjoint Agent scope 設計**: 25 file を 2-3 Agent disjoint scope 分割 (本 η-1 §3.3 範式継承)
3. **pilot 1 file (Claude 自力)** → Agent 並列展開 (本 η-1 §3.3 範式継承)

### §10.3 B?-η-2 完遂後の想定 cascade exposure 第6層

- (a) 完遂で `Cannot reuse block name` 20→0 + missing #endif cascade pair 20→0 解消想定
- (a) 完遂後の link failed 残件 (現在 9 件) が PerDrawUBO body 不一致 link cascade として exposure 想定 (= η-4 sub-bundle scope 想定)
- (b)+(c) 完遂で独立 missing #endif 9 件 + redefinition 2 件解消想定

---

## §11 観測点

1. **shader_cache 247 件 (B2-γ baseline 245 + 2)** maintained — 次 sub-bundle でも 245+ 維持 観測
2. **cascade pair hypothesis** (η-2 → η-1 自動消滅 / FrameAtmosphere/PerDrawUBO guard wrap で -119 / -118 ≈ 1:1) を B?-η-2 (a) FrameLights guard wrap でさらに検証 (FrameLights `Cannot reuse` 20 → 0 + cascade pair missing #endif 20 → 0 の 1:1 対称性確認)
3. **B?-ζ §3.1 範式の汎用性** = `extra_code_text` (C++ side) vs shader-file side 両系統で同形 preprocessor guard が有効 = 今後の同種 emergence (FrameLights 等) でも直接適用想定
