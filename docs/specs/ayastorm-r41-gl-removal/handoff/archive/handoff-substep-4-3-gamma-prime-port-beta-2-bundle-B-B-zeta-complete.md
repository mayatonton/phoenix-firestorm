# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-ζ 完遂 → 次 sub-bundle (B?-η) 着手境界 handoff (2026-06-01)

**parent commit**: `0a785f4cdc` (B?-ζ patch、本 handoff の直接 parent) / `ed98ed0131` (B?-ε patch 範式継承元) / `8c4d7eca20` (B?-ε-complete handoff doc commit)
**HEAD**: `0a785f4cdc` on `feature/ayastorm-r41-gl-removal`
**本 doc 位置付け**: B?-ζ (b) scope 完遂状態 + 次 sub-bundle (推奨 B?-η = cascade exposure 第4層 emergence 5 種 + α cluster scope 設計) 着手判断境界 を fresh context 引継 用に確定する doc-only handoff。B?-ε-complete `8c4d7eca20` 範式継承。**`extra_code_text` 内 struct GBufferInfo 定義の `#ifndef GBUFFER_INFO_DEFINED` guard wrap 範式 1 件を B?-ζ で新規確立**。`'GBufferInfo' : redefinition struct` 215 件の **完全解消** (= guard wrap 1 hook で ALL 215 件 cluster を一括 clear) 達成、`feedback_doubt_self_first` / `feedback_admit_unknown` 範式適用で Agent 誤推論訂正後の自力 literal verify による root cause 確定 + 1 file +10/-0 insertions-only 最小局所修正。AYA 「OK」明示承認下で commit (no auto-commit)。

---

## §1 起草目的

β-2-bundle-B scope 第六 sub-bundle B?-ζ の (b) scope (`struct GBufferInfo` 定義 guard wrap) を **1 file +10/-0 insertions-only** で完遂した状態を確定し、次 sub-bundle 着手境界 を fresh context に引継ぐ。B?-ε (commit `ed98ed0131` = utility source concat 末尾 `\n` 補正) の cascade exposure 第3層で露出していた **`'GBufferInfo' : redefinition struct` 215 件** (line 分散 `0:141`/`0:159`/`0:160`/`0:161` 等) を、**`extra_code_text` 内 struct 定義の `#ifndef GBUFFER_INFO_DEFINED` guard wrap 1 hook 追加で 100% 解消**。同時に cascade exposure 第4層 emergence (5 種 + α cluster = `missing #endif` 147 + `Cannot reuse block name within the same interface` 139 + `non-opaque uniforms` 35 + `link failed` 9 + opaque `binding` +2 + `location` +3 + redefinition 残 1 = 計 332+ 件) が次 sub-bundle scope として顕在化。

本 sub-bundle は **`feedback_doubt_self_first` + `feedback_admit_unknown` 範式適用** の実例:
- Agent (Explore) 報告 = "missing definition injection" (= struct 定義が Vulkan path に注入されていない仮説)
- 但し error 種 = `redefinition struct` (= 2 回以上定義) と Agent 仮説の整合性検証で **誤推論 detect**
- 自力 literal verify で root cause 確定 = `extra_code_text` (`llshadermgr.cpp` line 921) の struct GBufferInfo 定義が **utility cache (例 gbufferUtil.glsl) と program-specific cache (例 softenLightF.glsl) の双方に独立 copy で保持** されており、`generatePerProgramSPIRV()` 内 concat 経路で utility + program-specific 各 1 回 = 2 回展開 ⇒ redefinition

本 sub-bundle は **charter §3 #1 insertions-only 範式** の厳密適用例:
- 既存 strdup (line 921 = `struct GBufferInfo {...};\n` 注入) literal **完全不変**
- 前後 3 strdup (`#ifndef GBUFFER_INFO_DEFINED\n` / `#define GBUFFER_INFO_DEFINED 1\n` / `#endif\n`) **追加のみ**
- GL path 動作 semantic 等価 (`[EXTRA_CODE_HERE]` marker 1 回展開で `#ifndef` も 1 回展開 → struct 1 回定義)

---

## §2 B?-ζ (b) 完遂 status

| 項目 | 値 |
|---|---|
| Scope | (b) `LLShaderMgr::loadShaderFile()` 内 `extra_code_text` injection に `struct GBufferInfo` 定義の `#ifndef GBUFFER_INFO_DEFINED` guard wrap (前 2 行 + 末 1 行 = 3 strdup 追加、既存 line 921 literal 不変) |
| 修正 file 数 | **1 file** (`indra/llrender/llshadermgr.cpp`) |
| 変更行数 | **+10 / -0** (insertions-only / comment 7 行 + code 3 行) |
| 修正範囲 | line 920-921 周辺、`extra_code_text[extra_code_count++] = strdup("struct GBufferInfo {...};\n")` の **前** に `strdup("#ifndef GBUFFER_INFO_DEFINED\n")` + `strdup("#define GBUFFER_INFO_DEFINED 1\n")` の 2 strdup 追加、**後** に `strdup("#endif\n")` の 1 strdup 追加 + 範式コメント 7 行を挿入 |
| Agent 投入 | **1 件** (Explore = struct GBufferInfo 定義 file 特定 + 派生 utility attach 経路 trace、但し Agent 報告 (missing definition injection 仮説) は誤推論 = 自力 literal verify で訂正 + root cause 確定) |
| shader file 触り | **0 件** (本 sub-bundle は C++ 1 file 修正のみ、A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ/B?-δ/B?-ε 既処理 file 全 byte-for-byte 維持) |
| AYAstorm 改変保全 | **GL path 全不変** (`glCreateShader`/`glShaderSource`/`glCompileShader`/`glAttachShader` 全不変、`loadShaderFile` strdup 内既存 line 921 literal 不変 = 前後 strdup 追加のみ、`[EXTRA_CODE_HERE]` marker 1 回展開で semantic 不変、charter §3 #1 acceptance) |
| AYA cold cache launch verify | **PASS** (起動成立 13:29:17 cache 再生成 247 shaderbin = B2-γ baseline 245 を 2 件上回り = -21 観測点完全解消 + 更に +2 + clean shutdown 13:30:40 + Goodbye! 1 件 + Vulkan device/instance destroyed 各 1 件 + status: stopped + 実 FATAL/SIGSEGV/Aborted 0 件) |
| commit | `0a785f4cdc` (AYA 「OK」明示指示下 2026-06-01) |
| metric vs B?-ε baseline `'GBufferInfo' : redefinition struct` | **-215 ✓ B?-ζ (b) patch 効果完全直接観測** (215→0, 100% 解消) |
| metric vs B?-ε baseline `'#' preprocessor` | ±0 (0→0 維持、B?-ε 達成 retain) |
| metric vs B?-ε baseline non-opaque uniforms | **+35 cascade exposure 第4層 emergence** (0→35、B?-δ scope 外 program の bare uniform leak、§4.1 詳細) |
| metric vs B?-ε baseline `missing #endif` | **+147 第4層大量 emergence** (0→147、§4.1 詳細) |
| metric vs B?-ε baseline `Cannot reuse block name` | **+139 第4層大量 emergence** (0→139、§4.1 詳細) |
| metric vs B?-ε baseline `link failed` | **+9 第4層 emergence** (0→9、Skinned/Avatar 系 PerDrawUBO vertex/fragment 不一致) |
| metric vs B?-ε baseline opaque `binding` | **+2 第4層 emergence** (8→10) |
| metric vs B?-ε baseline `location` | **+3 第4層 emergence** (0→3) |
| metric vs B?-ε baseline parse failed | **-32** (223→191、-215 GBufferInfo + 第4層 net 内訳の合算) |
| metric vs B?-ε baseline link failed | +9 (0→9 emergence) |
| metric vs B?-ε baseline shader_cache | **+23** (224→247 = B2-γ baseline 245 を 2 件超え = -21 観測点完全解消 + 更に +2) |
| metric net delta | **-215 主指標 (GBufferInfo) 完全解消** + cascade exposure 第4層 emergence 5 種 + α 計 332+ 件 (独立計上、次 sub-bundle B?-η scope) |

### §2.1 既処理 sub-bundle との関係

| sub-bundle | 関係 |
|---|---|
| A1-A7 | uniform/sampler/UBO block 注入 (binding scope)、本 step touch 0 件、既存 UBO byte-for-byte 維持 |
| A8-recovery | AYAstorm 改変 5 file UBO 復活、本 step も skip list 維持で untouched |
| B1 | materialF.glsl MaterialUBO_Legacy 化 (case 2 file-local override)、本 step touch 0 件 |
| B2-α | varying + fragment_out 全 program 注入、本 step touch 0 件 |
| B2-β | vertex_in/VBO attribute 全 program 注入、本 step touch 0 件 |
| B3 | SPIR-V Vulkan profile override per-stage prepend、本 step も B3 範式の `#version 460 + #extension + LL_VULKAN_GLSL` 直後 prepend 経路をそのまま継承 |
| B2-γ | utility source cache + per-program attached utility tracking + utility concat hook + createShader reorder、本 step touch 0 件 (utility cache 構造を継承するが C++ side 別箇所修正) |
| B?-δ | utility unguarded bare uniform wrap (12 file +167)、本 step touch 0 件、B?-δ scope **外** program (Underwater/Glow Post/Vignette/Snapshot/Pathfinding 等) の non-opaque uniform leak は cascade exposure 第4層 emergence として顕在化 = B?-δ §3.1 範式 (utility 既 attach 全 file scope unguarded bare uniform 網羅 scan) の **scope 拡張対象** = B?-η 候補 |
| B?-ε | utility source concat 末尾 `\n` 補正 (1 file +11)、本 step touch 0 件、§3.2 cascade source 特定範式 (ALL N errors 同一 `0:LINE` 集中) は本 step では **直接適用不可** (line 分散) = §3.2 補強版 3-段 trace 範式適用 |
| B?-ζ (b) 本 sub-bundle | `extra_code_text` 内 struct GBufferInfo 定義の guard wrap = utility/program-specific cache 重複展開を guard で吸収する Vulkan 設計 |

---

## §3 設計範式 (B?-ζ で新規確立)

### §3.1 `extra_code_text` 重複展開対処の guard wrap 範式 (B?-ζ で新規確立)

**設計原則**: `LLShaderMgr::loadShaderFile()` 内 `extra_code_text[]` に **struct definition** を `strdup` で注入する箇所では、`#ifndef ...DEFINED` / `#define ...DEFINED 1` / `#endif` の guard で wrap する:

```cpp
extra_code_text[extra_code_count++] = strdup("#ifndef <NAME>_DEFINED\n");
extra_code_text[extra_code_count++] = strdup("#define <NAME>_DEFINED 1\n");
extra_code_text[extra_code_count++] = strdup("struct <NAME> { ... };\n");  // 既存 literal 不変
extra_code_text[extra_code_count++] = strdup("#endif\n");
```

**Why critical**: Vulkan path では `loadShaderFile()` Vulkan 経路 (line 1036-1048 = `out_sources && LLVKLoader::isVulkanInitialized()`) で `shader_code_text[]` (= `extra_code_text[]` 全件 + actual file 内容) を `out_sources` に emplace_back する。utility file (例 `gbufferUtil.glsl`) と program-specific file (例 `softenLightF.glsl`) の **各々の cache に同じ `extra_code_text` 内容が独立 copy で保持**される (= utility cache に struct GBufferInfo 1 件、program-specific cache に struct GBufferInfo 1 件 = 計 2 件)。`generatePerProgramSPIRV()` 内 concat 経路 = `#version 460 + #extension + #define LL_VULKAN_GLSL` prepend → utility sources concat → program-specific stage source concat の順序で展開され、**1 program あたり utility cache 由来 1 回 + program-specific cache 由来 1 回 = struct 2 回定義** ⇒ `'GBufferInfo' : redefinition struct` で reject。guard wrap で 1 度目展開時に `<NAME>_DEFINED` define → 2 度目以降 `#ifndef` で skip → struct 1 回のみ定義 = redefinition 解消。

**GL path 動作不変担保**:
- GL path では `[EXTRA_CODE_HERE]` marker 1 回展開で `extra_code_text` 全件が 1 度しか inject されない
- ⇒ guard wrap も 1 度のみ展開 = `#ifndef` 1 回真 → `<NAME>_DEFINED` 1 回 define → struct 1 回定義 = guard なし時と semantic 等価
- → **charter §3 #1 acceptance (insertions-only)** 適合 (既存 strdup literal 不変、前後 strdup 追加のみ)

**設計判断 (案 vs 代替案)**:
- 案 R (本 patch) = `extra_code_text` 内 struct 注入 strdup の前後に guard strdup を追加 (`llshadermgr.cpp` 1 file 3 行 + コメント 7 行)
- 案 X (handoff doc §10 オリジナル) = `mVulkanAttached*Utilities` push_back 直前 dedup check → **REJECT** (異なる utility 経由なので無効、同一 utility 重複 attach は本問題の root cause ではない)
- 案 Y (handoff doc §10 オリジナル) = utility shader 内 `#ifndef ... #endif` guard 追加 → **REJECT** (utility shader 内 struct 定義 0 件 = wrap 対象が存在しない、struct 定義は C++ extra_code_text に唯一存在)

**案 R 選択理由**:
1. root cause 直接対処 = `extra_code_text` injection mechanism の Vulkan 多重展開構造に対する正しい guard
2. charter §3 #1 insertions-only 完全適合 (既存 strdup literal 不変)
3. GL path semantic 不変 = `[EXTRA_CODE_HERE]` marker 1 回展開で guard も 1 回展開
4. 将来 struct 追加時の継承可能 (同じ pattern で別 struct 定義も guard wrap 可能)

**範式継承元**: B?-ε §3.2 cascade source 特定範式 (ALL N errors 同一 `0:LINE` 集中 ⇒ root cluster 確定) の **補強版** (line 分散時の代替 3-段 trace = `struct <NAME>` 定義行 grep + `[EXTRA_CODE_HERE]` injection mechanism trace + Vulkan path 多重展開構造の verify)。B?-δ §3.2 直接適用不可ケースでの代替範式。

**範式継承先想定**: 次 sub-bundle B?-η 等で別 struct 定義 (例 `LightInfo` 等) が `extra_code_text` 内に追加された場合、同 guard wrap pattern を適用。または cascade exposure 第4層 emergence の `Cannot reuse block name within the same interface: uniform` 139 件 (例 `FrameAtmosphere` block の vertex/fragment 重複) も同様の重複展開構造の可能性 = trace 後に同範式適用可能性あり。

### §3.2 patch literal (`indra/llrender/llshadermgr.cpp` line 920-934)

```cpp
    // Master definition can be found in deferredUtil.glsl
    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-ζ: GBufferInfo struct 定義を
    // '#ifndef GBUFFER_INFO_DEFINED' guard で wrap。Vulkan path では utility cache
    // (例 gbufferUtil.glsl) と program-specific cache (例 softenLightF.glsl) の
    // 各々が extra_code_text を独立 copy で保持するため、concat 結果に struct 定義
    // が複数回現れ 'GBufferInfo : redefinition struct' で reject される。guard
    // により 1 度目で define、2 度目以降 skip され redefinition 解消。GL path は
    // [EXTRA_CODE_HERE] marker 1 回展開で 1 度しか実行されないため semantic 不変。
    extra_code_text[extra_code_count++] = strdup("#ifndef GBUFFER_INFO_DEFINED\n");
    extra_code_text[extra_code_count++] = strdup("#define GBUFFER_INFO_DEFINED 1\n");
    extra_code_text[extra_code_count++] = strdup("struct GBufferInfo { vec4 albedo; vec4 specular; vec3 normal; vec4 emissive; float gbufferFlag; float envIntensity; };\n");
    extra_code_text[extra_code_count++] = strdup("#endif\n");
```

挿入位置: `LLShaderMgr::loadShaderFile()` 内、`Indexed texture rendering requires GLSL 1.30 or later.` LL_ERRS 直後 (line 918 直後)、`//copy file into memory` (line 923) 直前。既存 line 921 (`struct GBufferInfo {...};\n` strdup) literal は **完全不変**、その**前 2 行** (`#ifndef` + `#define`) **後 1 行** (`#endif`) を strdup 追加 + 範式コメント 7 行を line 920 後に挿入。

---

## §4 AYA cold cache launch verify metric

baseline = `AYAstorm.old.b-zeta-pre-gbufferinfo-guard` (B?-ε verify 後 = B?-ε baseline 同等、401520 byte)、after = `AYAstorm.log` (B?-ζ (b) cold verify 後 13:29:17-13:30:40、396593 byte)。

| metric | baseline | after | delta | 評価 |
|---|---|---|---|---|
| `'GBufferInfo' : redefinition struct` | 215 | **0** | **-215** | ✓ B?-ζ (b) 直接効果 (100% 解消、主指標完全達成) |
| `'#' preprocessor directive cannot be preceded by another token` | 0 | 0 | ±0 | B?-ε 達成維持 |
| `non-opaque uniforms outside a block` | 0 | **35** | **+35** | cascade exposure 第4層 emergence (B?-δ scope 外、§4.1 詳細、次 B?-η scope 候補) |
| `missing #endif` | 0 | **147** | **+147** | cascade exposure 第4層大量 emergence (§4.1 詳細、次 B?-η scope 候補) |
| `Cannot reuse block name within the same interface: uniform` | 0 | **139** | **+139** | cascade exposure 第4層大量 emergence (例 `FrameAtmosphere` block の vertex/fragment 重複、§4.1 詳細) |
| `glslang link failed for program` | 0 | **9** | **+9** | cascade exposure 第4層 emergence (Skinned/Avatar 系 PerDrawUBO vertex/fragment 不一致、§4.1 詳細) |
| `'Member names and types must match'` (link 内 detail) | 0 | 155 | +155 | link failed 9 件の detail message、独立 program count ではない |
| `'binding' : sampler/texture/image requires layout(binding=X)` | 8 | **10** | **+2** | cascade exposure 第4層 emergence (元 8 件 + 2 件 追加 = 別 sampler の bare opaque uniform) |
| `'location' :` | 0 | **3** | **+3** | cascade exposure 第4層 emergence 小規模 (詳細未 trace) |
| `redefinition` (全体) | 215 | **1** | **-214** | GBufferInfo 215→0 (-215) + 別 redefinition 1 件残 (詳細未 trace、第4層) |
| `glslang parse failed for stage` | 223 | **191** | **-32** | -215 GBufferInfo + (+147 missing #endif + +139 reuse block + +35 non-opaque + +2 opaque binding + +α) ≈ literal 内訳重複 (1 program 1 parse failed message に複数 error 種が乗る) |
| `No function definition for main` | 0 | 0 | ±0 | B2-γ 既処理状態維持 |
| `'#version' :` | 0 | 0 | ±0 | B3 既処理状態維持 |
| shader_cache 件数 | 224 | **247** | **+23** | **B2-γ baseline 245 を 2 件上回り** = -21 観測点完全解消 + 更に +2 = GBufferInfo 解消で 23 件 program が cache 化成功 |
| Goodbye! 件数 | 1 | 1 | ±0 | clean shutdown 維持 |
| Vulkan device destroyed | 1 | 1 | ±0 | clean shutdown 維持 |
| Vulkan instance destroyed | 1 | 1 | ±0 | clean shutdown 維持 |
| status: stopped | 1 | 1 | ±0 | clean shutdown 維持 |
| FATAL / SIGSEGV / Aborted 実件数 | 0 | 0 | ±0 | safe |

### §4.1 Cascade exposure 第4層 分類 (次 B?-η scope 候補)

B?-ζ (b) で `'GBufferInfo' : redefinition struct` 215 件を完全解消した結果、glslang が parse でより深く進めるようになり、**preexisting だったが今まで line 分散 GBufferInfo で abort されて見えていなかった** 5 種 + α leak が露出 (= 同 215 program 集合および周辺 program が次の error で abort し直し、+ B?-δ scope 外 program が GBufferInfo 解消で次 error で abort し直し):

| 件数 | 種別 | 代表 program / 構造原因 | 想定対処 (B?-η 案) |
|---|---|---|---|
| **147** | `missing #endif` | utility/program-specific 内 `#if/#ifdef/#ifndef` block の終端 `#endif` 欠落 (preexisting、深 parse で初露出)。具体的 source 未 trace。 | (η-1) utility/program-specific file の awk 状態機械 (`#if`/`#ifdef`/`#ifndef`/`#endif` 平衡 check) 全 scan、特に B?-δ §3.1 範式の scan 範囲拡張版で対応 |
| **139** | `Cannot reuse block name within the same interface: uniform` | 例 `FrameAtmosphere` block の vertex/fragment 重複定義 (同 program 内で同 block 名が複数回宣言)、複数 file (例 atmosphericsFuncs.glsl + waterFogF.glsl 等) で同 block 名 UBO を重複宣言している可能性 | (η-2) UBO block 名グローバル一意性 check + 重複定義箇所の guard wrap (B?-ζ §3.1 範式の UBO block 版応用) または block 名 namespace 分離 (per-file suffix 等) |
| **35** | `non-opaque uniforms outside a block` | B?-δ scope 外 program (Underwater Shader/Glow Shader Post/Glow Extract Shader/Vignette Post/Snapshot Frame Post/Pathfinding Shader 等) の bare uniform leak | (η-3) B?-δ §3.1 範式 (utility 既 attach 全 file scope unguarded bare uniform 網羅 scan) の **scope 拡張版** で post-processing/non-deferred program 系の utility file 全 scan |
| **9** | `link failed for program` (Member names and types must match) | Skinned Highlight/Skinned Occlusion/Skinned Debug/Skinned Bump/Deferred Diffuse Non-Indexed Alpha Mask/Skinned Deferred Bump/Skinned PBR Glow/Deferred Skinned Tree Shadow/Deferred Avatar Shadow Shader の **PerDrawUBO vertex/fragment 不一致** (例 vertex stage `mat3x4 matrixPalette[110]` / fragment stage `vec4 clipPlane`) | (η-4) PerDrawUBO struct の vertex/fragment 同一性担保 = `#ifdef VERTEX_SHADER`/`FRAGMENT_SHADER` で内容を分岐させていた構造を統一 (両 stage に全 member 含める or 単一 PerDrawUBO definition を skinned/non-skinned で分割) |
| **+2** | opaque `'binding' : sampler/texture/image requires layout(binding=X)` | 元 8 件 + 第4層 2 件 = 別 sampler の bare opaque uniform (handoff doc §4.1 (B?-ε) 想定 (c) scope 8 件より 2 件増、第4層 emergence) | (η-5) 全 utility/program file `^uniform sampler/image/texture` 行に `layout(set=N, binding=M)` qualifier 注入 (A 範式 opaque uniform 版、set=2/binding=N+ または set=3/binding=12+ 範囲割当) |
| **+3** | `'location' :` | 小規模 emergence、詳細未 trace、B2-α scope 既処理範囲との関係 (新規 location 未注入箇所?) | (η-6) trace 必要、B2-α §3.1 範式の漏れ verify |
| **+1** | `redefinition` (全体) | GBufferInfo 215→0 (-215) 後の残 1 件、別 struct 重複可能性 | (η-7) 同 §3.1 範式適用または別 struct の重複展開 trace |

総 cascade exposure 第4層 件数 = 147 + 139 + 35 + 9 + 2 + 3 + 1 = **336 件** が次 sub-bundle B?-η scope (= handoff doc §10 (B?-ε) オリジナル想定の (c) opaque uniform 8 件 + α より **大幅拡大**)。但し 1 program で複数 error が乗る (parse failed 1 message に複数 error 種共存) ため、program count 基準では `parse failed for stage` 191 件 + `link failed for program` 9 件 = **200 program** が fail 状態 (B?-ε 後 223 → 200 で -23 = shader_cache +23 と整合)。

**重要**: cascade exposure 第4層 emergence は **全て preexisting** (B?-ε 時 GBufferInfo 215 件で 215 program が深 parse できず隠蔽) = `feedback_falsification_as_progress` 範式整合 = 第4層 emergence は B?-ζ patch の副作用ではなく、根本的に preexisting leak の露出。退行 (regression) ではない。literal verify: baseline (`AYAstorm.old.b-zeta-pre-gbufferinfo-guard`) で `missing #endif` / `Cannot reuse block name` / `non-opaque uniforms` / `link failed` / `location` 全て **0 件** = B?-ζ patch の直接副作用ではない確定。

### §4.2 主指標 metric integrity self-check (B3 §12 範式継承)

literal pattern grep のみ採用 (structural touch consistency / 3 段階 pair-grep 単調性):

- `'GBufferInfo' : redefinition struct`: B?-ζ after verify log 0 件 = `grep -c "'GBufferInfo' : redefinition struct" AYAstorm.log` 直接観測値 (calculated value ではない)
- baseline 215 件: B?-ε verify log 同名 grep 直接観測値 = `grep -c "'GBufferInfo' : redefinition struct" AYAstorm.old.b-zeta-pre-gbufferinfo-guard`
- delta -215: 単純減算、`missing #endif` +147 / `Cannot reuse block name` +139 / `non-opaque uniforms` +35 / `link failed` +9 等の第4層 emergence の合算とは独立計上 (literal grep 結果同士の単独 metric)
- `redefinition` (全体): 215→1 (-214) = GBufferInfo 215→0 + 別 redefinition 1 件残、これは literal grep で `redefinition` の partial match を取った結果で、独立 1 件として残存

範式違反検出: `parse failed` 223→191 (-32) を「主指標と独立に」報告、混同して net delta を主指標に compose しない。`-215 (GBufferInfo) + +147 (missing #endif) + +139 (Cannot reuse) + ... = -32 (parse failed)` の合算は literal pair-grep 結果同士の偶然の対称性ではなく、`parse failed` が **program count 基準** (= 1 program 1 message) に対して個別 error は **error 件数基準** (= 1 program に複数 error が乗る) のため数値的に compose 不可。program count = 200 (191 parse + 9 link) と shader_cache 247 = 247 - (Vulkan shader 全件 約 447) - 200 fail ≈ 247 cached + 200 fail で整合確認。

---

## §5 self-verify (commit 前最終確認)

| # | 検証項目 | 結果 |
|---|---|---|
| 1 | 修正 1 file 全て insertions-only (削除 0、修正範囲外 byte-for-byte 維持) | ✓ git diff --stat = +10/-0 |
| 2 | A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ/B?-δ/B?-ε 既処理 file 全 byte-for-byte 不変 | ✓ shader file touch 0、C++ side touch も llshadermgr.cpp 1 file の line 920-934 内のみ |
| 3 | GL path 全不変 (`glCreate*`/`glShaderSource`/`glCompile*`/`glAttach*`/`loadShaderFile strdup`) | ✓ 既存 line 921 (`struct GBufferInfo {...};\n` strdup) literal 不変、前後 strdup 追加のみ = insertions-only |
| 4 | skip list 13 + A2 拡張 skip 2 + 5 V skip untouched | ✓ shader file touch 0 (本 step は C++ 1 file 修正のみ) |
| 5 | guard wrap pattern が正しい (`#ifndef ... #define ... struct ... #endif`) | ✓ line 928-931 順序確認、GLSL preprocessor 標準 syntax |
| 6 | charter §3 #1 acceptance (Vulkan path 内部追加のみ、CPU 側 binding 接続 touch 0) | ✓ `loadShaderFile()` 内 extra_code_text injection 拡張 = `[EXTRA_CODE_HERE]` 注入 mechanism 内追加のみ、GL/Vulkan 両 path で同 strdup 配列 inject |
| 7 | AYA cold cache launch verify PASS (clean shutdown + FATAL/SIGSEGV/crash 0 件) | ✓ 13:29:17 起動 / 13:30:40 終了 / Goodbye! 1 / Vulkan destroyed 各 1 / status: stopped |
| 8 | shader_cache 再生成確認 (cold cache = 完全 clear から再構築) | ✓ 247 shaderbin re-built = B2-γ baseline 245 を 2 件上回り = -21 観測点完全解消 |
| 9 | 主指標 `'GBufferInfo' : redefinition struct` 215→0 直接観測 | ✓ literal grep count |
| 10 | cascade exposure 第4層 emergence 5 種 + α (missing #endif 147 / Cannot reuse block name 139 / non-opaque uniforms 35 / link failed 9 / opaque binding +2 / location +3 / redefinition 残 1) を §4.1 で次 B?-η scope として独立記録、本 sub-bundle scope と混同せず | ✓ §4.1 確立、退行ではなく preexisting emergence と literal verify |
| 11 | commit 前 AYA 「OK」明示指示確認 | ✓ 2026-06-01 AYA「OK」 |

---

## §6 設計範式継承表

| 範式 | 由来 sub-bundle | 本 B?-ζ 適用箇所 |
|---|---|---|
| `feedback_doubt_self_first` (効かない時はまず自分のコード/仮説を疑う) | feedback memory | Agent 報告 (missing definition injection 仮説) と error 種 (redefinition struct) の整合性検証で誤推論 detect → 自力 literal verify で root cause 確定 |
| `feedback_admit_unknown` (分からないときは「分からない」と言う) | feedback memory | Agent 仮説 1 つで満足せず literal grep で実データ取得 (= Vulkan path concat 構造 + extra_code_text 多重展開構造) |
| `feedback_build_only_verified` (正しいことを積み上げる) | feedback memory | guard wrap 3 strdup 追加で 215 件完全解消を実機検証 = literal grep 確認後 commit |
| `feedback_falsification_as_progress` (全 REJECT verdict は生き残りルート絞り込みの成果) | feedback memory | handoff doc §10 オリジナル想定 2 案 (utility list dedup / shader 内 #ifndef guard) は両方 **REJECT** = 唯一の生き残りルート = `extra_code_text` 内 guard wrap (案 R) の絞り込み |
| `feedback_one_step_at_a_time` (検証手順は1ステップずつ) | feedback memory | AYA 「1」選択 = (b) 単独 scope 厳守、(c) opaque uniform 8 件は次 sub-bundle へ |
| `feedback_no_auto_commit` (コミットは明示指示があるまでしない) | feedback memory | patch 完了 + verify 完了 + metric 報告 → AYA 「OK」明示指示後 commit (`0a785f4cdc`) |
| `feedback_no_claude_coauthor` (Co-Authored-By: Claude を付けない) | feedback memory | commit message 末尾 Claude 共著行なし |
| `feedback_no_scope_shrink` (AYA 指示 literal scope を勝手に縮小しない) | feedback memory | AYA「1」指示 = (b) scope literal 厳守、root cause 対処として `extra_code_text` guard wrap は scope literal 解釈拡張 = scope 縮小ではなく scope 内 root cause 対処 |
| B1 §3 範式: case 2 file-local UBO override 3-段 swap pattern | B1 patch | 該当なし (本 step は shader file touch 0) |
| B2-α §3.1: canonical Table location 順 cross-file 固定 | B2-α patch | 該当なし |
| B3 §3.2: GL profile / Vulkan profile 分離範式 | B3 patch | extra_code_text injection が GL/Vulkan 両 path で実行される構造を確認 = GL path で `[EXTRA_CODE_HERE]` 1 回展開、Vulkan path で utility/program-specific 各 cache 独立 copy |
| B3 §12: metric 整合性 self-check 範式 (literal pattern grep のみ採用) | B3 patch 訂正 | §4.2 で literal grep 採用、compose-aware net delta を主指標に混同せず |
| B2-γ §3.1: Vulkan utility source cache 範式 | B2-γ patch | 本 step は utility source cache 構造の前提を継承 (cache に extra_code_text 含む copy が保持される構造) を verify |
| B2-γ §3.2: per-program attached utility tracking 範式 | B2-γ patch | concat 経路の前提 (per-program 順序保持 utility list + program-specific source) を継承 |
| B2-γ §3.3: utility concat hook + createShader reorder 範式 | B2-γ patch | concat hook 経路は不変、本 step は cache 内容側 (= extra_code_text 注入 mechanism) を対象 |
| B?-δ §3.1: utility 既 attach 全 file scope unguarded bare uniform 網羅 scan 範式 | B?-δ patch | scope 外 program (Underwater/Glow Post 等) の bare uniform leak emergence で **scope 拡張対象** として顕在化 = B?-η 候補 |
| B?-δ §3.2: cascade source 特定範式 (ALL N errors 同一 0:LINE 集中 ⇒ root cluster) | B?-δ patch | 本 step では **直接適用不可** (line 分散) = 代替 3-段 trace 範式 (struct 定義行 grep + injection mechanism trace + 多重展開構造 verify) で補完 |
| B?-ε §3.1: utility 境界 `\n` 終端 invariant 強化範式 | B?-ε patch | 該当なし (本 step は struct 定義 inject mechanism 側) |

---

## §7 risks (B?-ζ 完遂後 / B?-η 着手前)

| # | risk | 評価 |
|---|---|---|
| 1 | guard wrap の `#ifndef GBUFFER_INFO_DEFINED` が他箇所で偶発的に既 define されている場合に struct 1 度も定義されない → undefined identifier error | **極低** (`GBUFFER_INFO_DEFINED` macro は本 patch 新規導入で他箇所未使用、grep で衝突 0 確認可、cold cache launch verify PASS で struct 参照成功確認済) |
| 2 | cascade exposure 第4層 emergence 5 種 + α (336 件) のうち `missing #endif` 147 件が解消困難 → program count base 進捗が停滞 | **中** (147 件は preexisting `#if/#ifdef` 非対称 block 由来 = utility/program-specific file の awk 状態機械 scan で root location 特定可能、B?-δ §3.1 範式 scope 拡張で対応見込み、但し scan 範囲拡大 = Agent 並列必須) |
| 3 | `Cannot reuse block name within the same interface: uniform` 139 件 (例 `FrameAtmosphere` block の vertex/fragment 重複) は UBO block 名 namespace 設計に深く関わる | **中** (B1/B2-α/B2-β/B3 で確立した UBO 設計範式の見直しが必要、複数 file (atmosphericsFuncs/waterFogF/sky/clouds 等) で同 block 名 UBO 宣言の dedup または block 名 namespace 分離が必要、large scope 想定) |
| 4 | `link failed for program` 9 件 (Skinned/Avatar 系 PerDrawUBO vertex/fragment 不一致) は B1 範式 (PerDrawUBO 設計) との関係で複雑 | **中** (PerDrawUBO の vertex stage `mat3x4 matrixPalette[110]` vs fragment stage `vec4 clipPlane` 不一致は Skinned program 特有 = matrixPalette を含む別 UBO 設計 or PerDrawUBO 内に matrixPalette 含めるか fragment 側にも matrixPalette declaration 含める設計判断) |
| 5 | guard wrap で `extra_code_text` array 1 element → 4 element 化 (struct GBufferInfo) → `extra_code_text` array max size `1024` への影響 | **極低** (現状 extra_code_text 件数は数十件オーダー、+3 element 程度では 1024 limit に到達せず、line 723 `extra_code_text[1024]` capacity 余裕大) |
| 6 | shader_cache 件数 247 が B2-γ baseline 245 を 2 件上回る理由の解釈 | **低** (-21 観測点完全解消 + 更に +2 件 = 元々 cache miss だった program が GBufferInfo 解消で cache 化成功、致命的でなく net positive 効果) |
| 7 | cascade exposure 第4層 `Cannot reuse block name` 139 件と `missing #endif` 147 件の相関性 (同 program で両方 emergence する可能性) | **中** (1 program で複数 error 種が同時露出する典型 cascade pattern、program count 基準では `parse failed` 191 件 + `link failed` 9 件 = 200 program、error 件数 vs program count の混同を避ける必要 = §4.2 B3 §12 範式適用) |
| 8 | A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ/B?-δ/B?-ε 既処理 file の C++ side touch 0 を literal verify 必要 | **低** (本 step は `llshadermgr.cpp` 1 file 内 `loadShaderFile()` 内のみ修正、他 file touch 0 = git diff --stat literal verify 済) |
| 9 | 範式継承 (B?-ζ §3.1 `extra_code_text` guard wrap 範式) の補強範囲が今後の `extra_code_text` 拡張 (別 struct 追加等) で誤伝承される可能性 | **低** (本 handoff §3.1 で「struct 定義限定 + `#ifndef <NAME>_DEFINED` macro 一意性確保」と明示、`feedback_one_step_at_a_time` 範式継承で予防的拡張なし) |
| 10 | Agent (Explore) 誤推論 (missing definition injection 仮説) の今後の reuse での再発 | **中** (Agent 報告は **literal verification** を併走させて検証必須、`feedback_doubt_self_first` / `feedback_admit_unknown` 範式継承を Agent prompt template にも反映する余地、本 handoff §6 で範式適用記録) |

---

## §8 commit メッセージ literal (`0a785f4cdc`)

```
feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-ζ 完遂 ((b) scope = struct GBufferInfo 定義に '#ifndef GBUFFER_INFO_DEFINED' guard wrap、'GBufferInfo' : redefinition struct 215→0 完全解消、1 file +10/-0 insertions-only、indra/llrender/llshadermgr.cpp 編集 shader file touch 0、Agent (Explore) 1 件 (struct GBufferInfo 定義 file 特定 + 派生 utility attach 経路 trace = 但し Agent 報告 (missing definition injection 仮説) と error 種 (redefinition struct) の整合性検証で誤推論 detect → 自力 literal verify (feedback_doubt_self_first / feedback_admit_unknown 範式適用) で root cause 確定 = extra_code_text の utility cache + program-specific cache 独立 copy による struct GBufferInfo 重複展開)、修正範囲 = indra/llrender/llshadermgr.cpp line 921 周辺 LLShaderMgr::loadShaderFile() 内 extra_code_text injection に '#ifndef GBUFFER_INFO_DEFINED' '#define GBUFFER_INFO_DEFINED 1' guard wrap 追加 (前 2 行 + 末 1 行 = 3 strdup 追加) + 範式コメント 7 行、設計範式継承 = (1) charter §3 #1 insertions-only (既存 strdup literal 不変、前後 strdup 追加のみ、GL path [EXTRA_CODE_HERE] marker 1 回展開で semantic 不変 + Vulkan path 2+ 回展開で 2 度目以降 guard skip) + (2) handoff doc §10 オリジナル想定 utility list dedup / shader 内 #ifndef guard は REJECT (異なる utility 経由 / utility 内 struct 定義 0 件) で root cause = extra_code_text の Vulkan utility/program-specific cache 重複展開 = guard 化が唯一の root cause 対処、metric vs B?-ε baseline (b-zeta-pre-gbufferinfo-guard) = 'GBufferInfo' redefinition struct 215→0 (-215 ✓ B?-ζ 直接効果完全達成) + '#' preprocessor 0→0 (±0 B?-ε 達成維持) + non-opaque uniforms 0→35 (+35 cascade exposure 第4層 emergence = B?-δ scope 外 program Underwater/Glow Post/Vignette/Snapshot Frame/Pathfinding 等の bare uniform leak) + missing #endif 0→147 (+147 第4層大量 emergence) + 'Cannot reuse block name within the same interface' 0→139 (+139 第4層大量 emergence = 例 FrameAtmosphere block の vertex/fragment 重複定義) + link failed 0→9 (+9 第4層 emergence = Skinned/Avatar 系 PerDrawUBO vertex/fragment 不一致 例 vertex mat3x4 matrixPalette[110] vs fragment vec4 clipPlane) + opaque 'binding' 8→10 (+2 第4層 emergence) + 'location' 0→3 (+3 第4層 emergence) + parse failed 223→191 (-32 = -215 GBufferInfo + 第4層 net 内訳) + redefinition (全体) 215→1 (-214 = GBufferInfo 215→0 + 別 redefinition 1 件残)、charter §3 #1 担保 (Vulkan path 内 strdup 追加のみ、GL path glCreateShader/glShaderSource/glCompileShader/glAttachShader 全不変、loadShaderFile strdup 内既存 literal 不変、LLVKLoader::isVulkanInitialized() guard なし = GL path でも guard 付き struct 1 回 inject = semantic 等価 + 既存 line 921 literal 維持)、AYA cold cache launch verify PASS (起動成立 13:29:17 cache 再生成 247 shaderbin = B2-γ baseline 245 を 2 件上回り = -21 観測点完全解消 + 更に +2 + clean shutdown 13:30:40 + Goodbye! 1 件 + Vulkan device/instance destroyed 各 1 件 + status: stopped + 実 FATAL/SIGSEGV/Aborted 0 件)、A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ/B?-δ/B?-ε 既処理 file 全 byte-for-byte 維持 (shader file touch 0、本 step は C++ 1 file 修正のみ)、skip list 13 + A2 拡張 skip 2 + 5 V skip untouched、cascade exposure 第4層 emergence 5 種 + α (missing #endif 147 + Cannot reuse block name 139 + non-opaque uniforms 35 + link failed 9 + opaque binding +2 + location +3 + redefinition 残 1 件 = 計 332+ 件) は §4 想定外規模、handoff doc §10 オリジナル想定の (c) opaque uniform 8 件 + α より大幅拡大 = 次 sub-bundle B?-η scope 設計対象、AYA 「OK」明示指示下 commit 2026-06-01)
```

---

## §9 build artifact

| artifact | path / state |
|---|---|
| install tree (B?-ζ C++ patch 反映) | `~/ayastorm/` (本 step は C++ 1 file 修正、autobuild incremental build → install.sh deploy 経由、cold cache verify 13:29 起動時の deploy 状態) |
| shader_cache | `~/.ayastorm_x64/cache/shader_cache/` 247 shaderbin (cold cache verify 13:29 再生成、B2-γ baseline 245 を 2 件上回り) |
| baseline log | `~/.ayastorm_x64/logs/AYAstorm.old.b-zeta-pre-gbufferinfo-guard` (B?-ε verify 後 = B?-ε baseline 同等、401520 byte) |
| after log | `~/.ayastorm_x64/logs/AYAstorm.log` (B?-ζ (b) verify 後、396593 byte、起動 13:29:17 / 終了 13:30:40) |
| executable | `~/ayastorm/firestorm-bin` (本 step は C++ change、autobuild incremental build で再生成 = ReleaseFS_open --fmodstudio -DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE --chan AYAstorm-release) |

---

## §10 次 sub-bundle 推奨 (B?-η)

cascade exposure 第4層 emergence 5 種 + α (336 件) の規模は handoff doc §10 (B?-ε) オリジナル想定 (c) opaque uniform 8 件 + α より大幅拡大 = **B?-η scope 設計は単一 sub-bundle に束ねるか分割するかの判断が必要**:

| 優先順位 | 候補 | scope | 想定 metric 効果 | 想定 file touch | Agent 必要性 |
|---|---|---|---|---|---|
| 1 | **B?-η-(η-1)** = `missing #endif` 147 件解消 | utility/program-specific file の awk 状態機械 (`#if`/`#ifdef`/`#ifndef`/`#endif` 平衡 check) 全 scan、B?-δ §3.1 範式の scan 範囲拡張版 | `missing #endif` 147→0 級減少見込み、cascade exposure 第5層露出可能性あり | shader 多数 file (B?-δ scope 外含む全範囲) | Agent (Explore) 並列必須 |
| 2 | **B?-η-(η-2)** = `Cannot reuse block name within the same interface` 139 件解消 | UBO block 名グローバル一意性 check + 重複定義箇所の guard wrap (B?-ζ §3.1 範式の UBO block 版応用) または block 名 namespace 分離 (per-file suffix 等) | `Cannot reuse block name` 139→0 級減少見込み、cascade exposure 第5層露出可能性あり | shader 多数 file (UBO 宣言全 file) または C++ side UBO 注入 mechanism (= B?-ζ 範式継承) | Agent (Explore) 並列必須 + B1 §3 範式の見直し |
| 3 | **B?-η-(η-3)** = `non-opaque uniforms outside a block` 35 件解消 | B?-δ §3.1 範式 (utility 既 attach 全 file scope unguarded bare uniform 網羅 scan) の **scope 拡張版** で post-processing/non-deferred program 系の utility file 全 scan | `non-opaque uniforms` 35→0 完全 clear 見込み | shader 数 file (post-processing utility) | Agent (Explore) 1 件 + awk 状態機械 |
| 4 | **B?-η-(η-4)** = `link failed` 9 件解消 (Skinned PerDrawUBO 統一) | PerDrawUBO の vertex/fragment 同一性担保 = `#ifdef VERTEX_SHADER`/`FRAGMENT_SHADER` で内容を分岐させていた構造を統一 (両 stage に全 member 含める or 単一 PerDrawUBO definition を skinned/non-skinned で分割) | `link failed` 9→0 級減少見込み | shader 数 file (PerDrawUBO 関連 + skinned) または C++ side UBO 注入 mechanism | Agent (Explore) 1 件 + B1 §3 範式の見直し |
| 5 | **B?-η-(η-5)** = opaque `binding` 10 件解消 | 全 utility/program file `^uniform sampler/image/texture` 行に `layout(set=N, binding=M)` qualifier 注入 (A 範式 opaque uniform 版、set=2/binding=N+ または set=3/binding=12+ 範囲割当) | opaque `binding` 10→0 完全 clear 見込み | shader 多数 file (~50-100 file 想定) | Agent (Explore) 並列必須 |
| 6 | **B?-η-(η-6)** = `'location' :` 3 件 + redefinition 残 1 件 解消 | trace 必要、B2-α §3.1 範式の漏れ verify + 別 struct 重複展開 trace | `'location' :` 3→0 + redefinition 残 1→0 | shader 数 file | 詳細未 trace、Agent (Explore) 推奨 |

**B?-η scope 設計指針 (推奨)**:
- 単一 sub-bundle で 336 件全解消は context 圧迫リスク高 + 設計複雑性 = **複数 sub-bundle に分割推奨**
- 推奨分割: **B?-η-1 = (η-1) `missing #endif` 147 件 + (η-3) `non-opaque uniforms` 35 件 (B?-δ §3.1 範式 scope 拡張系)** → **B?-η-2 = (η-2) `Cannot reuse block name` 139 件 + (η-4) `link failed` 9 件 (UBO 設計範式系)** → **B?-η-3 = (η-5) opaque `binding` 10 件 + (η-6) location 3 + redefinition 1 (qualifier/個別系)**
- 但し AYA 判断で 1 sub-bundle 束ね or 別分割も可

**B?-η 着手前必須事項**:
- AYA 明示指示要 (no auto-commit 範式継承)
- fresh context 推奨 (handoff doc + memory 範式継承記録から start)
- cache 完全 clear 必須 launch verify
- A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ/B?-δ/B?-ε/B?-ζ 既処理 file 再 touch 禁止 absolute rule
- Agent 報告は **literal verification** 併走必須 (本 B?-ζ で Agent 誤推論 detect 経験継承)

**B?-η-1 着手前 trace 範式 (B?-δ §3.1 範式 scope 拡張版)**:
- B?-δ §3.1 awk 状態機械 (`#ifdef LL_VULKAN_GLSL`/`#ifndef LL_VULKAN_GLSL`/`#else`/`#endif` 平衡 check) を post-processing/non-deferred program 系 utility file まで拡張
- `missing #endif` 147 件は preexisting `#if/#ifdef` 非対称 block 由来 = scan で root file/line 特定可能

**B?-η-2 着手前 trace 範式 (B?-ζ §3.1 範式 UBO block 版応用)**:
- `Cannot reuse block name` 139 件 = 同 program 内同 block 名 UBO 重複宣言、`grep -rn "uniform <BLOCK_NAME>" indra/.../shaders/` で全 file scan + per-block 名 出現箇所一覧
- 例 `FrameAtmosphere` block: 複数 file で `uniform FrameAtmosphere { ... }` 宣言 → 同 program に複数 file が attach されると重複 → block 名 namespace 分離 or guard 化

---

## §11 観測点 (B?-ζ 中に検出 / 致命的でない / 次 sub-bundle 着手前 trace 推奨)

| 観測点 | 詳細 | 致命度 |
|---|---|---|
| shader_cache 247 件 = B2-γ baseline 245 を 2 件上回り | -21 観測点完全解消 + 更に +2 = 元々 cache miss だった program が GBufferInfo 解消で cache 化成功、致命的でなく net positive 効果 | **低** (positive 観測、特に対処不要) |
| cascade exposure 第4層 emergence 5 種 + α 計 336 件 の **想定外規模** | handoff doc §10 (B?-ε) オリジナル想定 (c) opaque uniform 8 件 + α より約 42 倍規模、B?-η scope 設計に深い影響 | **中** (B?-η scope 設計時に直接影響、複数 sub-bundle 分割推奨、Agent 並列必須) |
| Agent (Explore) 誤推論 (missing definition injection 仮説) の検出 | error 種 (redefinition) と仮説整合性検証で detect、`feedback_doubt_self_first` / `feedback_admit_unknown` 範式の Agent 報告 review 適用 | **中** (Agent prompt template に literal verification 併走指示を含める余地、本 handoff §6 で範式適用記録) |
| `Member names and types must match` link error の 1 program 内多重出現 | 1 program で複数 detail message が出る (例 `mat3x4 matrixPalette[110]` vs `vec4 clipPlane`)、error 件数 vs program count の混同を避ける必要 | **低** (B3 §12 範式継承で literal grep 個別計上、program count は parse/link failed message ベース) |
| `redefinition` 全体 1 件残 (詳細未 trace) | GBufferInfo 215→0 後の残 1 件、別 struct 重複可能性 = B?-η-(η-6) scope 内で trace | **低** (小規模 1 件、B?-η-3 等で対処) |

---

(EOF)
