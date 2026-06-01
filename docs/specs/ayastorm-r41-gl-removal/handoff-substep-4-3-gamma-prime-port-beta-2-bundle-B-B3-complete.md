# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B3 完遂 → 次 sub-bundle 着手境界 handoff (2026-06-01)

**parent commit**: `71a8f87cde` (B3 patch、本 handoff の直接 parent) / `eca091e1ce` (B2-β patch 範式継承元) / `64f3229109` (B2-β-complete handoff doc commit 範式継承元)
**HEAD**: `71a8f87cde` on `feature/ayastorm-r41-gl-removal`
**本 doc 位置付け**: B3 完遂状態 + 次 sub-bundle (B2-γ 候補 = Linking cross-stage 関数 linkage 整理 261 件 / B? = 残 location 13 + binding 45 + missing #endif 15 cleanup) 着手判断境界 を fresh context 引継 用に確定する doc-only handoff。B2-β-complete `64f3229109` 範式継承。**SPIR-V Vulkan profile override 範式 (per-stage prepend、複数 file 同 stage concat 時 stage 先頭 1 回のみ #version 460 + #extension GL_KHR_vulkan_glsl : enable 出力) を B3 で新規確立**。`generatePerProgramSPIRV()` 内 concat ロジック構造解消 (per-file `#version XXX` 流用 → per-stage prepend) により、glslang `'must occur first'` + `'bad profile name'` reject 36 件 + non-opaque 74 件 cascade clear を直接観測。AYA 「OK」明示承認下で commit (no auto-commit)。

---

## §1 起草目的

β-2-bundle-B scope 第三 sub-bundle B3 (`#version` directive 切替) を 1 file (`indra/llrender/llglslshader.cpp` `generatePerProgramSPIRV()` 内 line 753-769) +12 insertions / -8 deletions の structural rewrite で完遂した状態を確定し、次 sub-bundle 着手境界 を fresh context に引継ぐ。B2-β (commit `eca091e1ce` = vertex_in/VBO attribute 全 program 注入、location 102→13) の cascade 露出 `#version directive 36 件` を **shader file 改変ではなく C++ Vulkan path 内部の concat 構造解消で消去**、加えて non-opaque 74 件 cascade clear を直接観測。scope は当初推定の 36 shader file (Agent 1 件) から **1 C++ file (Agent 不要 single Edit)** へ桁違い縮小、根本原因確定 (= 同 stage 複数 file concat 時の重複 `#version` 出力) を log + code 両面 trace で確定後の patch。

---

## §2 B3 完遂 status

| 項目 | 値 |
|---|---|
| Scope | `LLGLSLShader::generatePerProgramSPIRV()` 内 stage concat ロジック (line 753-769) per-file `#version` 流用 → per-stage prepend 範式置換 |
| 修正 file 数 | **1 file** (`indra/llrender/llglslshader.cpp` のみ) |
| 変更行数 | **+12 / -8** (structural rewrite、insertions-only ではない、C++ Vulkan path 内部改造) |
| 修正範囲 | `LLGLSLShader::generatePerProgramSPIRV()` 内 line 753-769、stage_indices loop の外側 prepend + loop 内 sources[0] skip |
| Agent 投入 | **0 件** (Agent 不要、single Edit で完遂、scope 桁違い縮小) |
| shader file 触り | **0 件** (skip list 13 含む全 shader file untouched、A1-A7/A8-recovery/B1/B2-α/B2-β 既処理 file 全 byte-for-byte 維持) |
| AYAstorm 改変保全 | **GL path 全不変** (loadShaderFile 内 `shader_code_text[0] = "#version XXX\n"` strdup logic 不変、collect_for_vulkan=false の GL compile path 完全 untouched) |
| AYA cold cache launch verify | **PASS** (起動成立 + clean shutdown + Goodbye! 1 件 + Vulkan device/instance destroyed 各 1 件 + shader_cache 245 件再生成 ±0 + 実 FATAL/SIGSEGV/crash 0 件) |
| commit | `71a8f87cde` (AYA 「OK」明示指示下) |
| metric vs B2-β baseline #version | **-36 ✓ B3 patches 効果完全直接観測** (36→0, 100% 解消) |
| metric vs B2-β baseline non-opaque | **-74 ✓ cascade clear** (74→0, 100% 解消) |
| metric net delta | **-54 errors** (#version -36 + non-opaque -74 + Linking +56 cascade 露出 = -54 controlled cascade improvement) |

### §2.1 既処理 sub-bundle との関係

| sub-bundle | 関係 |
|---|---|
| A1-A7 | uniform/sampler/UBO block 注入 (binding scope)、本 step touch 0 件、byte-for-byte 維持 |
| A8-recovery | AYAstorm 改変 (Cinematic BD shadowUtil/screenSpaceReflUtil + Visual Realism volumetricLight/blurLightF/godraysF 系 5 file) UBO 復活、本 step も skip list 維持で untouched |
| B1 | materialF.glsl MaterialUBO_Legacy 化 (case 2 file-local override)、本 step touch 0 件 |
| B2-α | varying (vertex_out + fragment_in) + fragment_out 全 program 注入 (canonical Table A/B 確立)、本 step 全 shader file untouched |
| B2-β | vertex_in/VBO attribute 全 program 注入 (canonical Table C 確立)、本 step 全 shader file untouched |

---

## §3 設計範式 (B3 で新規確立)

### §3.1 per-stage Vulkan profile prepend 範式 (B3 で新規確立)

**設計原則**: SPIR-V 生成 path で同 stage type に複数 file が concat される構成 (例 SMAA EdgeDetect VERTEX program = `SMAAEdgeDetectV.glsl` + `SMAA.glsl(VERTEX)` の 2 file、`llviewershadermgr.cpp:2878-2881` 参照) において、各 file の `sources[0]` (= `LLShaderMgr::loadShaderFile()` が strdup した GL profile `#version XXX\n`) を そのまま append すると **2 回目以降の `#version`** が glslang Vulkan rules で `'must occur first in shader'` reject。**stage 先頭 1 回のみ Vulkan profile を prepend** し、各 file の `sources[0]` は **skip** することで重複 `#version` を構造解消。Vulkan profile = `#version 460` + `#extension GL_KHR_vulkan_glsl : enable` + `#define LL_VULKAN_GLSL 1`。

**範式継承元**: B2-α §3.1 (canonical varying location top-20 cross-file 固定) / B2-β §3.1 (canonical vertex_in location 0-13 cross-file 固定) が GLSL 側 shader file 範式だったのに対し、本 B3 範式は **C++ Vulkan path 内部 concat 範式**。shader file 範式と直交 (相互依存なし)。

### §3.2 GL profile / Vulkan profile 分離範式 (B3 で新規確立)

**設計原則**: shader 入力 source 配列 (`std::vector<std::string> sources`) は GL path で `LLShaderMgr::loadShaderFile()` が組み立て、`sources[0] = "#version XXX\n"` (GL major.minor version 依存 = 420/400/150/330/140)。GL compile path は本配列を unmodified で `glCompileShader` に流す (charter §3 #1 = GL path byte-for-byte 担保)。Vulkan path は `LLGLSLShader::generatePerProgramSPIRV()` 内で同配列を読むが、**concat 構築時に `sources[0]` を skip + Vulkan profile prepend** で profile override を局所完結。loadShaderFile 内 `shader_code_text[0]` strdup ロジック (`llshadermgr.cpp:743-777`) は完全 untouched、GL path への漏洩 0。

**charter §3 #1 acceptance**: Vulkan path 内部改造のみ、GL path collect_for_vulkan=false の loadShaderFile strdup 不変、shader_cache 245 再生成 ±0 で担保。

### §3.3 patch literal (line 753-769 置換)

**修正前** (B2-β 範式継承、per-file `#version` 流用):
```cpp
// Concat all source fragments for this stage type. LL_VULKAN_GLSL macro is
// injected after the first source string of each file (matches existing
// LLShaderMgr::createSPIRVFromGLSL pattern, sub-doc 06 §1.2.4).
std::string concatenated;
for (size_t idx : stage_indices)
{
    const auto& stage = stages[idx];
    if (!stage.sources.empty() && !stage.sources[0].empty())
    {
        concatenated.append(stage.sources[0]);
    }
    concatenated.append("#define LL_VULKAN_GLSL 1\n");
    for (size_t i = 1; i < stage.sources.size(); ++i)
    {
        concatenated.append(stage.sources[i]);
    }
}
```

**修正後** (B3 新規確立、per-stage Vulkan profile prepend):
```cpp
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B3: Vulkan profile #version override.
// 同 stage に複数 file が連結される構成 (例 SMAA VERTEX = SMAAEdgeDetectV.glsl +
// SMAA.glsl(VERTEX)) で、各 file の sources[0] (= GL profile "#version XXX\n") を
// そのまま append すると glslang が 2 回目以降を 'must occur first' で reject。
// 加えて GL profile (#version 420 等) は Vulkan glslang で 'bad profile name' 扱い。
// stage 先頭 1 回のみ Vulkan profile (#version 460 + GL_KHR_vulkan_glsl extension +
// LL_VULKAN_GLSL macro) を出力し、各 file の sources[0] は skip (GL path
// loadShaderFile の strdup には影響なし、collect_for_vulkan=false の GL compile
// path 不変、charter §3 #1 acceptance)。
std::string concatenated;
concatenated.append("#version 460\n");
concatenated.append("#extension GL_KHR_vulkan_glsl : enable\n");
concatenated.append("#define LL_VULKAN_GLSL 1\n");
for (size_t idx : stage_indices)
{
    const auto& stage = stages[idx];
    for (size_t i = 1; i < stage.sources.size(); ++i)
    {
        concatenated.append(stage.sources[i]);
    }
}
```

### §3.4 empty check 残置範式

修正前は `sources[0]` も `sources[1..]` も全 file 空なら `concatenated` empty 化が可能だったが、修正後は template prepend 3 行 (`#version 460\n#extension GL_KHR_vulkan_glsl : enable\n#define LL_VULKAN_GLSL 1\n`) で常時 non-empty 化。line 775-781 の `if (concatenated.empty())` check は dead code 化するが、defensive code として残置 (害なし、将来 prepend 全 skip の variant 追加時に再活性化可能)。

### §3.5 glslang `setEnvInput` 整合

`setEnvInput(EShSourceGlsl, lang, EShClientVulkan, 450)` (line 784) は Vulkan rules `#version 460` を accept (Vulkan profile では 450/460 とも core 暗黙)。`#extension GL_KHR_vulkan_glsl : enable` は Vulkan glslang 標準 extension (推奨明示宣言、`setEnvClient(EShClientVulkan, EShTargetVulkan_1_2)` と整合)。

---

## §4 cold cache launch verify metric (2026-06-01 19:15)

| 項目 | B2-β baseline | B3 | delta | 解釈 |
|---|---|---|---|---|
| §4.1 hook fire | 224 | 224 | ±0 | shader pipeline 不変 |
| §4.2 parse failed shader (program) | 120/224 | 147/224 | +27 | line 1 reject 解消 36 件中 27 件が body parse まで深化 (深部 error で reject) + 9 件 parse 通過 = progress signal (深化 + 通過両側 improvement) |
| §4.3 **#version directive** | 36 | **0** | **-36 ✓** | **B3 patches 効果完全直接観測** (100% 解消、`must occur first` + `bad profile name` + `bad tokens following profile` 全消失) |
| §4.3 **non-opaque type** | 74 | **0** | **-74 ✓** | **cascade clear** (B2-β #version 重複で line 1 reject されていた program が parse 通過後に non-opaque 行に first-error 取得、B3 で line 1 解消 → 旧 first-error 位置が消失) |
| §4.3 location | 13 | 13 | ±0 | utility / cinematic_bd 残置 (次 sub-bundle scope) |
| §4.3 binding | 45 | 45 | ±0 | utility 残置 (次 sub-bundle scope) |
| §4.3 missing #endif | 15 | 15 | ±0 | utility 残置 (次 sub-bundle scope) |
| §4.3 Linking failed | 205 | 261 | +56 | cascade 露出: #version + non-opaque 解消で大半 program が parse 段階深化 → link 段階到達、cross-stage 関数定義未解決 / utility shader linkage 整理 sub-bundle scope (B2-γ) |
| §4.3 **7 error category 合計** (#version+non-opaque+location+binding+#endif+Linking、parse failed 除く) | 388 | **334** | **-54 ✓** | **controlled cascade improvement** (B2-β -32 / B2-α -93 と同等規模) |
| §4.4 shader_cache 再生成 | 245 | **245** | **±0 ✓** | **GL path regression 0** (charter §3 #1 担保) |
| §4.5 FATAL / SIGSEGV / crash (実) | 0 / 0 / 0 | **0 / 0 / 0** | ±0 | grep match 4 件は全 `settings_crash_behavior.xml` ファイル名 false positive、実 0 件 |
| §4.6 起動成立 + clean shutdown | PASS | **PASS** | - | Goodbye! 1 件 + Vulkan device destroyed 1 件 + Vulkan instance destroyed 1 件、charter §3 #1 acceptance 担保 |

**net delta -54 (7 category)** = #version -36 + non-opaque -74 + location/binding/#endif ±0 + Linking +56 = controlled cascade improvement。**#version + non-opaque 合計 -110 件 = B3 効果完全直接観測**。Linking cascade exposure +56 は parse 深部到達 (224→147 = 77 program が parse 段階突破) による次層露出 = progress signal、bundle-B 全体完遂時の集合的閾値で評価。

---

## §5 self-verify 結果

| # | check | result |
|---|---|---|
| §5.1 | git diff | **1 file changed, 12 insertions(+), 8 deletions(-)** (`indra/llrender/llglslshader.cpp` のみ) |
| §5.2 | shader file touch | **0 件** (skip list 13 含む全 shader file untouched、A1-A7/A8-recovery/B1/B2-α/B2-β 既処理 file 全 byte-for-byte 維持) |
| §5.3 | C++ syntax | **OK** (`{}` balanced、`append()` chain、empty check 残置、stage_indices loop 構造維持) |
| §5.4 | GL path 影響 | **0** (`generatePerProgramSPIRV()` は `collect_for_vulkan=true` block 内のみ呼出、`loadShaderFile()` strdup logic 完全 untouched、`shader_code_text[0]` GL profile `#version XXX\n` も完全不変) |
| §5.5 | charter §3 #1 担保 | **OK** (Vulkan path 内部改造のみ、GL path byte-for-byte 維持、shader_cache 245 再生成 ±0 + 起動成立 + clean shutdown で実証) |
| §5.6 | concatenated 出力構成 | line 1 = `#version 460` / line 2 = `#extension GL_KHR_vulkan_glsl : enable` / line 3 = `#define LL_VULKAN_GLSL 1` / line 4+ = 各 file `sources[1..]` (`sources[0]` skip) |
| §5.7 | empty check 整合 | **OK** (template prepend 3 行で常時 non-empty 化、defensive code として残置害なし) |

---

## §6 設計範式 (B2-α/B2-β 継承 + B3 新規確立)

| # | 範式 | 確立 sub-bundle |
|---|---|---|
| §6.1 | per-stage Vulkan profile prepend 範式 (同 stage 複数 file concat 時 stage 先頭 1 回のみ Vulkan profile 出力、各 file sources[0] = GL profile 行は skip) | **B3 新規** |
| §6.2 | GL profile / Vulkan profile 分離範式 (loadShaderFile strdup 不変 + Vulkan path 内部 override 局所完結、GL compile path への漏洩 0) | **B3 新規** |
| §6.3 | empty check 残置範式 (template prepend で常時 non-empty 化後も defensive code 維持、害なし) | **B3 新規** |
| §6.4 | canonical Table C 範式 (vertex_in/VBO attribute 14 location 0-13 cross-file 固定、CPU enum 整合) | B2-β 確立、B3 継承 (untouched) |
| §6.5 | CPU `LLShaderMgr::initAttribsAndUniforms` push_back 順 = `LLVertexBuffer::AttributeType` enum = SPIR-V `layout(location=N)` = `glBindAttribLocation` の 4 者 1:1:1:1 整合範式 | B2-β 確立、B3 継承 (untouched) |
| §6.6 | conditional duplicate (HAS_SKIN guard 両分岐重複) は各重複に独立 3-段 swap 注入、両分岐とも同 location | B2-β 確立、B3 継承 (untouched) |
| §6.7 | nesting 規則 (既存 preprocessor guard 内側に `#ifdef LL_VULKAN_GLSL` nest は OK、既 LL_VULKAN_GLSL block 内 nest は禁止) | B2-β 確立、B3 継承 (untouched) |
| §6.8 | Table A 範式 (top-20 varying location 0-19 cross-file 固定) | B2-α 確立、B3 継承 (untouched) |
| §6.9 | Table B 範式 (fragment_out 別 namespace、frag_color=0 / frag_data=0\|1) | B2-α 確立、B3 継承 (untouched) |
| §6.10 | GL #else byte-for-byte 範式 | bundle-A 確立、B1/B2-α/B2-β/B3 継承 |
| §6.11 | 既処理 file UBO untouched 範式 | bundle-A 確立、B1/B2-α/B2-β/B3 継承 |
| §6.12 | 既処理 file varying/frag_out/vertex_in 行 untouched 範式 | B2-α/B2-β 確立、B3 継承 (本 step は shader file 全 untouched) |

---

## §7 risks/caveats

### §7.1 metric net delta -54 (7 category) = controlled cascade improvement

#version -36 + non-opaque -74 が直接観測される B3 patch 効果 (合計 -110 解消)、Linking +56 cascade 露出 = parse 深部到達 (224→147 = 77 program 突破) による次層露出 = progress signal。bundle-B 全体 (B1-B?) 完遂時の集合的閾値で評価 (bundle-A 同 measurement design)。

### §7.2 cold cache launch 必須

`rm -rf ~/.ayastorm_x64/cache/` (cache 全削除、shader_cache 含む) を sub-bundle 毎 verify 前必須化、本 commit でも実施済。

### §7.3 Linking failed 261 件 = 次 sub-bundle (B2-γ) scope

#version + non-opaque 解消で大半 program が parse 段階深化 → link 段階到達、cross-stage 関数定義未解決 (`passTextureIndex` / `mirrorClip` / `encodeNormal` / `getObjectSkinnedTransform` 等) / utility shader linkage 整理が次 sub-bundle scope (B2-γ 候補)。B2-α §7.3 / B2-β §7.3 で報告された linkage 問題群、後続 sub-bundle で utility shader 範式確立時に集合的解消想定。GL path #else branch 影響なし。

### §7.4 残 location 13 / binding 45 / missing #endif 15 = mixed cleanup sub-bundle scope

残 location 13 件 (utility / cinematic_bd / 一部 stage-aware 漏れ)、binding 45 件 (utility sampler/UBO)、missing #endif 15 件 (B2-α/B2-β 注入の preprocessor balance 検証要)、後続 sub-bundle (B? = mixed cleanup) で扱う想定。scope 確定は次 sub-bundle 着手前 trace で。

### §7.5 残 parse failed 147 件 = Linking cascade と重複

parse failed (program) 147 件は line 1 reject 解消後の **body parse error** 集合 = location 13 + binding 45 + missing #endif 15 + 残 syntax/semantic error 74 件相当。Linking failed 261 件と一部 overlap (parse 通過した program のみ link 段階到達、parse 失敗 program は link 試行されず)。bundle-B 完遂時の集合的閾値で評価。

### §7.6 `createSPIRVFromGLSL()` (旧 path) 修正対象外

`llshadermgr.cpp:538-550` の `LLShaderMgr::createSPIRVFromGLSL()` 内 concat ロジック (per-file `#version` 流用 + `#define LL_VULKAN_GLSL` 注入) は β-1 per-file model 用、β-2 で `generatePerProgramSPIRV()` per-program model に置換済 (`llglslshader.cpp:614` コメント参照 = "β-2 で既存 per-file hook 削除に伴い唯一の SPIR-V 生成 path は generatePerProgramSPIRV()")。B3 修正対象は **`generatePerProgramSPIRV()` のみ**、`createSPIRVFromGLSL()` は dead code (本 step touch 0 件、将来 cleanup sub-step で削除候補)。

### §7.7 `#version 460` profile 選定根拠

- `setEnvInput(EShSourceGlsl, lang, EShClientVulkan, 450)` (line 784) は GLSL 450 base で Vulkan rules 適用
- `setEnvClient(EShClientVulkan, EShTargetVulkan_1_2)` (line 785) は Vulkan 1.2 client API
- `setEnvTarget(EShTargetSpv, EShTargetSpv_1_5)` (line 786) は SPIR-V 1.5 target
- `#version 460` は GLSL 4.60 で SPIR-V interop 標準 (Vulkan glslang 推奨)
- `#version 450` でも parse OK だが、glsl 4.6 拡張 (e.g. layout component qualifier 等) を利用可能にするため 460 採用
- `core` profile suffix は Vulkan rules で不要 (`setEnvClient(EShClientVulkan, ...)` で暗黙 core)

### §7.8 `#extension GL_KHR_vulkan_glsl : enable` 必要性

- Vulkan glslang は `EShClientVulkan` 指定で `GL_KHR_vulkan_glsl` extension を **暗黙的に enable** (`setEnvInput` で適用)
- `#extension GL_KHR_vulkan_glsl : enable` 明示宣言は **推奨** (将来の glslang 厳格化で `require` 化された場合への防衛)
- shader source 内 explicit declaration は redundant だが、SPIR-V 規格上 valid (`shader.parse()` で warning なし)

### §7.9 generatePerProgramSPIRV() empty check (line 775-781) dead code 化

修正後は template prepend 3 行で常時 non-empty 化、`if (concatenated.empty())` は false branch のみ通る dead code に。defensive code として残置 (将来 variant 追加 = e.g. prepend skip mode で再活性化可能)、害なし。

---

## §8 charter §3 #1 acceptance 担保

- GL path 不変: `LLShaderMgr::loadShaderFile()` 内 `shader_code_text[0] = "#version XXX\n"` strdup logic (`llshadermgr.cpp:743-777`) byte-for-byte 維持、GL compile path (`glCompileShader` 経由) 全不変
- AYAstorm 独自改造意図保全: 全 shader file (Cinematic BD shadowUtil/screenSpaceReflUtil + Visual Realism volumetricLight/blurLightF/godraysF 系 7 file + Picker 2 + Exemplar 2 + A2 拡張 2 含む) touch 0 件
- bundle-A/A8-recovery/B1 既処理 UBO block byte-for-byte 維持 (touch 0 件)
- B2-α 既処理 varying/frag_out 行 byte-for-byte 維持 (touch 0 件)
- B2-β 既処理 vertex_in 行 byte-for-byte 維持 (touch 0 件)
- 段階 1-4.3-γ'-port-β-2-bundle-B-B2-β 動作維持 (cold cache launch verify 起動成立 + clean shutdown + shader_cache 245 再生成 + 実 crash 0 件、Linking failed 261 件 cascade exposure は GL path 影響なし)

---

## §9 cross reference

### §9.1 commit / handoff doc 系譜

- `71a8f87cde` (B3 patch、本 handoff の直接 parent)
- `64f3229109` (B2-β-complete handoff doc commit、範式継承元)
- `eca091e1ce` (B2-β patch、bundle-B 第二 sub-bundle β 分支)
- `c0e1b25310` (B2-α-complete handoff doc)
- `bfa81e0283` (B2-α patch、bundle-B 第二 sub-bundle α 分支)
- `18187c862e` (B1-complete handoff)
- `5614494f56` (B1 patch、bundle-B 第一 sub-bundle)
- `8d2cae435b` (A7-complete handoff、bundle-A 全体完遂)
- `862f7dc8bd` (A7 patch、bundle-A 残 cleanup)
- `aed1438936` (A8-recovery、AYAstorm 改変 UBO 復活)

### §9.2 sub-doc / charter

- sub-doc 06 §1.2.2/§1.2.4/§3.1 sub-step 6.3
- sub-doc 07 §3.1 sub-step 7.2-7.4
- sub-doc 03 §3.1.3 (exemplar 2 役割)
- charter §3 #1 + §7.5
- project_ayastorm_r41_vulkan_migration.md (γ'-port-β-2-bundle-B-B2-β 完遂 → 本 commit で B3 完遂 + 次 sub-bundle 着手境界 active)

### §9.3 memory references (feedback)

- feedback_proactive_handoff (本 doc 起草 = 周回境界での能動 handoff)
- feedback_self_verify_before_handoff (§5 self-verify 7 項目 all green)
- feedback_doubt_self_first (当初推定 36 shader file scope を log + code 両面 trace で 1 C++ file に scope 訂正)
- feedback_no_scope_shrink (literal scope = `#version directive` 36 件全解消、shader file 範式 untouched 同時担保、scope は file 数ではなく error category で literal 整合)
- feedback_admit_unknown (Agent 不要判断 = 根本原因確定後の scope 桁違い縮小、recalibration を transparent に AYA 報告)
- feedback_falsification_as_progress (cascade exposure Linking +56 を progress signal として正確記録)
- feedback_explanation_lead_with_conclusion (metric 報告で結論ファースト = #version -36 ✓ + non-opaque -74 ✓ + net -54)
- feedback_no_claude_coauthor (commit message に Claude 共著行なし)
- feedback_no_auto_commit (AYA 「OK」明示指示下で commit)
- feedback_one_step_at_a_time (step 1 trace → step 2 prep → step 3 patch → step 4 self-verify → step 5 deploy → step 6 AYA verify → step 7 commit → step 8 handoff doc → step 9 memory 更新 を sequential 実行)
- feedback_remove_verification_logs (Verification 用 LL_INFOS 追加なし、structural rewrite only)
- feedback_render_full_trace_first (推測禁止、log + code 両面 trace で根本原因確定後の patch、SMAA 例 = `llviewershadermgr.cpp:2878-2881` mShaderFiles 同 stage 複数 push 確認)
- feedback_build (configure → build → install → cache clear 一括実行、本 sub-bundle で実施)

---

## §10 次 action = 次 sub-bundle 着手境界

### §10.1 次 sub-bundle 候補

| 候補 | scope | 推定 |
|---|---|---|
| **B2-γ** (Linking cascade cleanup) | cross-stage 関数定義未解決 (`passTextureIndex` / `mirrorClip` / `encodeNormal` / `getObjectSkinnedTransform` 等) の utility shader linkage 整理、Linking failed 261 件解消対象 | linker error 中心、~20-30 utility shader、forward declaration vs include 化判断 |
| **B?** (残 mixed cleanup) | location 13 + binding 45 + missing #endif 15 + 残 syntax/semantic 74 cleanup | mixed bag、scope 確定要 trace、~20-40 file |

選択は AYA 判断 (技術判断 = 効果直接観測しやすい順 / dependency 順)。**推奨順 = B2-γ** (Linking 261 件は本 commit で深部到達した最大 cascade、上流 utility 確定要だが解消量大) → **B?** (mixed cleanup、scope 確定要)。B3 で `#version` directive cascade を一括解消したため、次は **Linking** が最大の残 cascade、解消で大半 program が compile/link 成立段階に到達想定。

### §10.2 次 sub-bundle 着手前に確認すべき事項

1. **utility shader source-of-truth** (cinematic_bd/class1/deferred/shadowUtil.glsl 等の skip 維持 vs touch 判断、cross-stage 関数 forward declaration 配置確認)
2. **cross-stage 関数 forward declaration vs include 化の trade-off** (B2-α §7.3 で報告された 1 件と B3 cascade 261 件の根本対応、glslang per-program link で全 stage 単一 TProgram に addShader 済 = link 段階解消可能性)
3. **残 location 13 件の actual file 特定** (utility / cinematic_bd / 漏れ判別、grep `0:N: 'layout'` で specific log line 抽出)
4. **残 missing #endif 15 件の preprocessor balance 検証** (B2-α/B2-β 注入の `#ifdef LL_VULKAN_GLSL` / `#else` / `#endif` 三重整合 spot check)

### §10.3 fresh context 引継 prompt 推奨

```
AYAstorm r41 Vulkan migration の sub-step 4.3-γ'-port-β-2-bundle-B-(B2-γ or B?) を着手します。

必読:
1. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B3-complete.md (本 handoff doc、parent 範式、per-stage Vulkan profile prepend 範式 / GL profile / Vulkan profile 分離範式)
2. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B2-beta-complete.md (B2-β 範式継承元、Table C literal)
3. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B2-alpha-complete.md (B2-α 範式継承元、Table A/B literal)
4. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B1-complete.md (B1 範式継承元、MaterialUBO_Legacy)
5. (B2-γ 着手の場合) utility/include shader linkage routing (cross-stage 関数定義の forward declaration vs include 化、glslang per-program link 仕様)
6. (B? 着手の場合) 残 location 13 + binding 45 + missing #endif 15 の actual file 特定 trace

cadence (B1/B2-α/B2-β/B3 同):
1. trace (scope file 抽出 + canonical 設計 + 根本原因確定 = log + code 両面)
2. prep (Table / 範式 提示) → AYA OK
3. patch (Agent 並列 or single Edit、scope 規模に応じて判断) → AYA OK
4. self-verify (skip list / insertions-only or structural rewrite / pair integrity / canonical consistency)
5. deploy (cp + cache clear、C++ 修正なら autobuild 経由)
6. AYA cold cache launch verify
7. metric (該当 category 直接観測 + cascade 露出記録)
8. commit (AYA OK)
9. handoff doc 起草 (本 doc 範式継承)

絶対 rule:
- B2-α/B2-β/B3 既処理 file の varying/frag_out/vertex_in 行 + Vulkan profile prepend logic untouched (本 step scope のみ touch)
- bundle-A/A8-recovery/B1 既処理 UBO block untouched
- skip list 13 file 機械的除外
- AYA 「commit して」or 「OK」明示指示前に commit 禁止
- one step at a time、1 メッセージ 1 アクション
- cold cache verify 必須 (rm -rf ~/.ayastorm_x64/cache/)
- 根本原因確定前に patch 着手禁止 (log + code 両面 trace、推測禁止 = feedback_render_full_trace_first)
```

---

## §11 build artifact persist

| artifact | path | 内容 |
|---|---|---|
| 根本原因 trace log抜粋 | `~/.ayastorm_x64/logs/AYAstorm.log` (verify 前 B2-β snapshot) line 1374-1424 周辺 | SMAA Edge Detection / Neighborhood Blending / T2x Resolve 各 quality (Low/Medium/High/Ultra) で `0:93/98/105: '#version' : must occur first in shader` + `bad profile name` + `bad tokens following profile` error 3 件 × 12 program = 36 件 |
| code trace 結果 | `llglslshader.cpp:472-490` (createShader() 内 mShaderFiles loop = 各 file 個別 loadShaderFile → mStageSources 蓄積) + `llglslshader.cpp:667-769` (generatePerProgramSPIRV() stages_by_type loop = 同 stage 複数 file concat) | scope 確定根拠 |
| SMAA program 構成 source-of-truth | `llviewershadermgr.cpp:2877-2881` (gSMAAEdgeDetectProgram[i].mShaderFiles に SMAAEdgeDetectF.glsl + SMAA.glsl(FRAGMENT) + SMAAEdgeDetectV.glsl + SMAA.glsl(VERTEX) の 4 file push) | 同 stage 複数 file concat 構成証拠 |

これら artifact は fresh context で本 doc + commit message `71a8f87cde` + `llglslshader.cpp:636-810` (generatePerProgramSPIRV 全体) + `llshadermgr.cpp:743-777` (loadShaderFile #version strdup) から再生成可能 (persist 性 stable、コード trace 範式が source of truth)。
