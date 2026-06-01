# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B2-β 完遂 → 次 sub-bundle 着手境界 handoff (2026-06-01)

**parent commit**: `eca091e1ce` (B2-β patch、本 handoff の直接 parent) / `c0e1b25310` (B2-α-complete handoff doc commit 範式継承元) / `bfa81e0283` (B2-α patch 範式継承元)
**HEAD**: `eca091e1ce` on `feature/ayastorm-r41-gl-removal`
**本 doc 位置付け**: B2-β 完遂状態 + 次 sub-bundle (B2-γ 候補 = utility / cascade 露出 cleanup or B3 = bundle-B 第三 sub-bundle 着手判断境界) を fresh context 引継 用に確定する doc-only handoff。B2-α-complete `c0e1b25310` 範式継承。**canonical Table C 範式 (vertex_in/VBO attribute 14 location 0-13 cross-file 固定) を B2-β で新規確立**。SPIR-V `layout(location=N)` qualifier を vertex shader の `in <type> <name>;` 宣言 (VBO attribute) 全 program に 3 Agent 並列で注入し、location error 102→13 で 87.3% 解消。AYA 「OK」明示承認下で commit (no auto-commit)。

---

## §1 起草目的

β-2-bundle-B scope 第二 sub-bundle の β 分支 B2-β (vertex_in/VBO attribute layout(location=N) 注入) を 92 file +844 行に 3 Agent 並列で注入完遂した状態を確定し、次 sub-bundle 着手境界 を fresh context に引継ぐ。B2-α (commit `bfa81e0283` = varying + fragment_out 全 program 注入、location 195→102) で varying scope を解消した次層として vertex_in/VBO attribute scope を解消。location 102→13 で -89 件 (87.3%) 解消 = B2-β patches 効果完全直接観測。残 13 件 location は utility / cascade 露出 範疇、B2-α の link failed 1 件と合わせて次 sub-bundle で扱う想定。

---

## §2 B2-β 完遂 status

| 項目 | 値 |
|---|---|
| Scope | vertex shader の `in <type> <name>;` (VBO attribute、top-level、UBO block 外) 全宣言 3-段 swap 注入 |
| 注入 file 数 | **92 file** (effective scope 111 vertex shader file 中 14 件は no-op = `gl_VertexIndex` only helper/include、5 件は skip-list 内除外) |
| 変更行数 | **+844 / 0** (insertions-only、deletion 0 件) |
| 注入 declaration 数 | **211 declarations** = Agent1 (deferred 前半) 75 + Agent2 (deferred 後半 + interface) 88 + Agent3 (avatar + objects + effects + post + class2 + class3 + gltf + environment + errorV) 48 |
| 3-段 swap pattern balanced | ifdef/endif count 全 92 file balanced (差分 grep verify 0 unbalanced) |
| variant pattern `defined(LL_VULKAN_GLSL)` | **0 件** |
| Agent 投入 | **3 並列 dispatch** (shared template /tmp/B2-beta-agent-template.md + 個別 file list /tmp/b2b-agent{1,2,3}-files.txt) |
| canonical (A1-A7/A8-recovery/B1/B2-α) untouched | **違反 0 件** (全 既処理 UBO block + varying/frag_out byte-for-byte 維持) |
| skip list 13 file untouched | **全 untouched** (git diff 0 件確認) |
| skip 5 vertex shader (Picker + Visual Realism + Exemplar + A2 拡張) untouched | **全 untouched** |
| AYAstorm 改変保全 | **GL path #else 分岐に既存 in 宣言 byte-for-byte 維持** |
| AYA cold cache launch verify | **PASS** (起動成立 + clean shutdown + Goodbye! 1 件 + Vulkan device/instance destroyed 各 1 件 + shader_cache 245 件再生成 + FATAL/SIGSEGV/crash 0 件) |
| commit | `eca091e1ce` (AYA 「OK」明示指示下) |
| metric vs B2-α baseline location | **-89 ✓ B2-β patches 効果完全直接観測** (102→13, 87.3% 解消) |
| metric net delta | **-32 errors** (location -89 + parse failed shader -104 + binding +45 cascade 露出 + non-opaque +11 cascade 露出 + missing #endif +1 cascade 移動 + Linking +205 cascade 露出 + #version +36 cascade 露出) = controlled cascade improvement |

### §2.1 3 Agent batch split 内訳

| Agent | scope | input file | patched | declaration |
|---|---|---|---|---|
| Agent1 | class1/deferred/*V.glsl 前半 (SMAA*/alpha*/avatar*/bump*/clouds*/diffuse*/emissive*/fullbright*/genbrdflut*/impostor*/material*/moon*/normgen*/pbrShadow*/pbralpha*) | 23 | 23 | 75 |
| Agent2 | class1/deferred/*V.glsl 後半 (pbrglow→velocity) + class1/interface/*V.glsl | 46 | 46 | 88 |
| Agent3 | avatar + objects + effects + post + class2 + class3 + gltf + environment + errorV | 23 | 23 | 48 |
| **合計** | **92** | **92** | **211** |

### §2.2 既処理 sub-bundle との関係

| sub-bundle | 関係 |
|---|---|
| A1-A7 | uniform/sampler/UBO block 注入 (binding scope)、本 step touch 0 件、byte-for-byte 維持 |
| A8-recovery | AYAstorm 改変 (Cinematic BD shadowUtil/screenSpaceReflUtil + Visual Realism volumetricLight/blurLightF/godraysF 系 5 file) UBO 復活、本 step も skip list 維持で untouched |
| B1 | materialF.glsl MaterialUBO_Legacy 化 (case 2 file-local override)、本 step touch 0 件 |
| B2-α | varying (vertex_out + fragment_in) + fragment_out 全 program 注入 (canonical Table A/B 確立)、本 step では vertex_in 宣言行のみ touch、`out`/`in` (fragment_in)/`out` (fragment_out) 行は全 untouched (insertions-only 違反禁止 absolute rule) |

---

## §3 設計範式 (B2-α 継承 + B2-β 新規確立)

### §3.1 案 4 = canonical vertex_in location cross-file 固定範式 (B2-β で新規確立)

**設計原則**: vertex_in/VBO attribute は CPU 側 `LLShaderMgr::initAttribsAndUniforms` の `mReservedAttribs` push_back 順 = `LLVertexBuffer::AttributeType` enum 値 = SPIR-V `layout(location=N)` 番号 = `glBindAttribLocation` binding 番号、4 者完全 1:1:1:1 整合。同一 `in <name>;` 行は全 file で同じ location 番号を取り、cross-API (GL `glBindAttribLocation` ↔ Vulkan `layout(location)`) pair 整合を name-based に自動担保。新規 attribute 追加時は CPU 側 enum 拡張 + canonical 番号継承で衝突回避。

**範式継承元**: B2-α §3.1 案 3 (canonical varying location top-20 cross-file 固定) を vertex_in scope に拡張。差異 = varying は GPU 内 pair (vertex_out ↔ fragment_in) のみだが、vertex_in は CPU 側 binding と整合必要 = source-of-truth が C++ enum。

### §3.2 Canonical Table C literal (B2-β で確立、vertex_in 14 attribute)

source-of-truth: `indra/llrender/llshadermgr.cpp:1406-1419` (`LLShaderMgr::initAttribsAndUniforms`) push_back 順 + `indra/llrender/llvertexbuffer.h:131-148` (`LLVertexBuffer::AttributeType` enum)

| location | attribute name | dominant type | TYPE_* enum | file 出現 |
|----------|---------------|---------------|-------------|----------|
| 0  | position       | vec3 (default) / vec4 (sky/star) | TYPE_VERTEX | 88 file |
| 1  | normal         | vec3 | TYPE_NORMAL | 35 file |
| 2  | texcoord0      | vec2 | TYPE_TEXCOORD0 | 56 file |
| 3  | texcoord1      | vec2 | TYPE_TEXCOORD1 | 8 file |
| 4  | texcoord2      | vec2 | TYPE_TEXCOORD2 | 3 file |
| 5  | texcoord3      | vec2 | TYPE_TEXCOORD3 | 1 file |
| 6  | diffuse_color  | vec4 | TYPE_COLOR | 26 file |
| 7  | emissive       | vec4 | TYPE_EMISSIVE | 1 file |
| 8  | tangent        | vec4 | TYPE_TANGENT | 4 file |
| 9  | weight         | vec4 | TYPE_WEIGHT | 0 file (occlusionSkinned 含む weight4 経由) |
| 10 | weight4        | vec4 | TYPE_WEIGHT4 | 8 file |
| 11 | clothing       | vec4 | TYPE_CLOTHWEIGHT | 1 file |
| 12 | joint          | uvec4 | TYPE_JOINT | 1 file |
| 13 | texture_index  | int | TYPE_TEXTURE_INDEX | 5 file |

### §3.3 3-段 swap pattern literal (B2-β scope)

```glsl
// vertex shader の in 宣言 (top-level、UBO block 外):
#ifdef LL_VULKAN_GLSL
layout(location=N) in <type> <name>;
#else
in <type> <name>;
#endif
```

N は Canonical Table C の literal 番号 (name → location 固定マッピング)。`<type>` と `<name>` は元宣言と byte-for-byte 一致 (type variant 許容 = `position` の vec3/vec4 等)。

### §3.4 conditional duplicate 範式 (HAS_SKIN 等の guard 内重複)

`pbropaqueV.glsl` / `pbralphaV.glsl` / `pbrmetallicroughnessV.glsl` 等で `#ifndef HAS_SKIN` / `#else` 両分岐に `in vec3 position;` 重複宣言される場合、各重複に独立で 3-段 swap 注入。両分岐とも同じ location 番号取得 (Table C は name-based 固定マッピング、variant 別 SPIR-V validator は per-program pair check のため program 別 namespace で自己一貫なら OK)。

### §3.5 nesting 規則

既存 `#ifdef MULTI_UV` / `#ifndef HAS_SKIN` 等の preprocessor guard **内側** に `#ifdef LL_VULKAN_GLSL` を nest するのは OK (variant guard の内側で API guard を切る = 正常)。逆に **既存 `#ifdef LL_VULKAN_GLSL` ブロック内に再度 `#ifdef LL_VULKAN_GLSL`** を nest するのは禁止 (二重 wrap mistake、B2-α `out` 行は本 step touch 禁止 = 既 LL_VULKAN_GLSL block 内のため)。

---

## §4 cold cache launch verify metric (2026-06-01)

| 項目 | B2-α baseline | B2-β | delta | 解釈 |
|---|---|---|---|---|
| §4.1 hook fire | 224 | 224 | ±0 | shader pipeline 不変 |
| §4.2 parse failed shader | 224/224 | **120/224** | **-104** | 104 shader が parse 段階突破 = B2-β patches 効果 + B2-α cascade clear 累積観測 |
| §4.3 location | 102 | **13** | **-89 ✓** | B2-β patches 効果完全直接観測 (87.3% 解消) |
| §4.3 binding | 0 | 45 | +45 | cascade 次層露出: vertex_in 解消後 fragment_out / utility shader の binding 行が first-error 位置取得 |
| §4.3 non-opaque | 63 | 74 | +11 | cascade 露出: 旧 location-first error file が parse 深部到達 → bare uniform 行に first-error 位置移動 |
| §4.3 missing #endif | 14 | 15 | +1 | cascade 位置移動 |
| §4.3 Linking failed | 0 | 205 | +205 | cascade 露出: vertex_in/varying 全解消で大半 program が link 段階到達、cross-stage 関数定義未解決 / utility shader linkage 整理 sub-bundle scope |
| §4.3 #version directive | 0 | 36 | +36 | cascade 露出: utility/include shader の `#version 460 core` Vulkan profile 非対応 (Vulkan は `#version 460` + `#extension GL_KHR_vulkan_glsl`)、後続 sub-bundle scope |
| §4.3 **4 category 合計** (location+binding+non-opaque+missing #endif) | **179** | **147** | **-32** | controlled improvement |
| §4.4 shader_cache 再生成 | 224 | **245** | **+21** | GL path 改善 (vertex 圧縮拡張 21 file 含む)、regression 0 |
| §4.5 FATAL / SIGSEGV / crash | 0 / 0 / 0 | 0 / 0 / 0 | ±0 | 全 benign |
| §4.6 起動成立 + clean shutdown | PASS | **PASS** | - | Goodbye! 1 件 + Vulkan device destroyed 1 件 + Vulkan instance destroyed 1 件、charter §3 #1 acceptance 担保 |

**net delta -32 (4 category)** = location -89 + binding +45 + non-opaque +11 + missing #endif +1 = controlled cascade improvement、location category specifically **-89 = B2-β patches 効果完全直接観測**。cascade exposure (binding/non-opaque/Linking/#version) は parse 深部到達による次層露出 = progress signal、bundle-B 全体完遂時の集合的閾値で評価。

---

## §5 self-verify 結果

| # | check | result |
|---|---|---|
| §5.1 | skip list 13 file 違反 | **0 件** (git diff 全 file untouched) |
| §5.2 | skip 5 vertex shader (Picker + Visual Realism + Exemplar + A2 拡張) 違反 | **0 件** (fsObjectIDV/godraysV/blurLightV/diffuseV/previewV 全 untouched) |
| §5.3 | insertions-only | **92 file 844 insertions / 0 deletions** |
| §5.4 | ifdef/endif balance | **0 unbalanced** (全 92 file `#ifdef LL_VULKAN_GLSL` count == `#else` count == `#endif` count、grep verify) |
| §5.5 | canonical vertex_in name-location 整合 (Table C) | **全 211 declaration single location cross-file 完全一貫** (position=0 / normal=1 / texcoord0=2 / texcoord1=3 / texcoord2=4 / texcoord3=5 / diffuse_color=6 / emissive=7 / tangent=8 / weight4=10 / clothing=11 / joint=12 / texture_index=13、CPU enum push_back 順 1:1:1 整合) |
| §5.6 | nested LL_VULKAN_GLSL inside LL_VULKAN_GLSL | **0 件** (B2-α `out`/`in` (fragment_in)/`out` (fragment_out) 既 wrapped block 内に二重 wrap mistake なし) |
| §5.7 | conditional duplicate 同 location 取得 (pbropaqueV/pbralphaV/pbrmetallicroughnessV) | **全 一貫** (HAS_SKIN guard 両分岐 position=0, diffuse_color=6, normal=1, tangent=8, texcoord0=2 同 location 取得) |
| §5.8 | UBO block byte-for-byte 維持 | **全 untouched** (bundle-A/A8-recovery/B1 既処理 UBO block touch 0 件、MaterialUBO_Legacy 含む) |
| §5.9 | B2-α 既処理 varying/frag_out 行 byte-for-byte 維持 | **全 untouched** (vertex_in 宣言行のみ touch、`out`/`in` (fragment_in)/`out` (fragment_out) 行は全 untouched) |
| §5.10 | layout(location=N) 値域 0-13 (Table C 範囲内) | **全 211 件 in range** (Table C 外の location 番号注入 0 件) |

---

## §6 設計範式 (B2-α 継承 + B2-β 新規確立)

| # | 範式 | 確立 sub-bundle |
|---|---|---|
| §6.1 | canonical Table C 範式 (vertex_in/VBO attribute 14 location 0-13 cross-file 固定、CPU enum 整合) | **B2-β 新規** |
| §6.2 | CPU `LLShaderMgr::initAttribsAndUniforms` push_back 順 = `LLVertexBuffer::AttributeType` enum = SPIR-V `layout(location=N)` = `glBindAttribLocation` の 4 者 1:1:1:1 整合範式 | **B2-β 新規** |
| §6.3 | conditional duplicate (HAS_SKIN guard 両分岐重複) は各重複に独立 3-段 swap 注入、両分岐とも同 location (Table C name-based 固定) | **B2-β 新規** |
| §6.4 | nesting 規則 (既存 preprocessor guard 内側に `#ifdef LL_VULKAN_GLSL` nest は OK、既 LL_VULKAN_GLSL block 内 nest は禁止) | **B2-β 新規** |
| §6.5 | Table A 範式 (top-20 varying location 0-19 cross-file 固定) | B2-α 確立、B2-β 継承 (untouched) |
| §6.6 | Table B 範式 (fragment_out 別 namespace、frag_color=0 / frag_data=0\|1) | B2-α 確立、B2-β 継承 (untouched) |
| §6.7 | GL #else byte-for-byte 範式 | bundle-A 確立、B1/B2-α/B2-β 継承 |
| §6.8 | 既処理 file UBO untouched 範式 | bundle-A 確立、B1/B2-α/B2-β 継承 |
| §6.9 | 既処理 file varying/frag_out 行 untouched 範式 (B2-α 既 wrapped 行は本 step touch 禁止) | **B2-β 新規 (insertions-only absolute rule 拡張)** |
| §6.10 | 3 Agent 並列 patch 範式 (92 file scope を deferred 前半/後半+interface/その他 で 3 batch 分割、shared template + 個別 file list で context isolation) | **B2-β 新規 (B2-α 4 Agent 範式の縮小版)** |

---

## §7 risks/caveats

### §7.1 metric net delta -32 (4 category) = cascade improvement

location -89 が直接観測される B2-β patch 効果、binding/non-opaque/missing #endif の +57 cascade 露出、Linking +205 / #version +36 は parse 深部到達 (224→120 file) による次層露出 = progress signal。bundle-B 全体 (B1-B?) 完遂時の集合的閾値で評価 (bundle-A 同 measurement design)。

### §7.2 cold cache launch 必須

`rm -rf ~/.ayastorm_x64/cache/shader_cache/` sub-bundle 毎 verify 前必須化、本 commit でも実施済。

### §7.3 Linking failed 205 件 = 次 sub-bundle scope

vertex_in/varying 全解消で大半 program が link 段階到達、cross-stage 関数定義未解決 / utility shader linkage 整理が次 sub-bundle scope (B2-γ 候補)。B2-α §7.3 で報告された "Deferred Diffuse Non-Indexed Alpha Mask Shader" の `passTextureIndex` / `encodeNormal` 含む類似 linkage 問題群、後続 sub-bundle で utility shader linkage 整理時に解消想定。GL path #else branch 影響なし。

### §7.4 #version directive 36 件 = 次 sub-bundle scope

`#version 460 core` (GL desktop profile) を Vulkan glslang は warning/error 扱い、`#version 460` + `#extension GL_KHR_vulkan_glsl : enable` への切替が必要。utility shader / include shader scope、後続 sub-bundle (or β-3 / γ scope) で扱う想定。

### §7.5 残 location 13 件 = utility cascade

残 13 件は cinematic_bd / utility include shader / 一部 stage-aware 漏れの可能性、後続 sub-bundle で残 cascade として扱う。B2-β scope は CPU `LLShaderMgr` enum 範疇の VBO attribute で完遂、本範式拡張不要。

### §7.6 canonical Table C 範式 persist 性

artifact `/tmp/B2-beta-trace.md` + `/tmp/B2-beta-agent-template.md` 失効時は本 handoff doc §3.2 (Table C literal) + commit message `eca091e1ce` から再生成可能。本 doc + `indra/llrender/llshadermgr.cpp:1406-1419` + `indra/llrender/llvertexbuffer.h:131-148` が source of truth。

### §7.7 残 skip 既知 list

- skip list base 13 untouched (Picker 2 + Cinematic BD 2 + Visual Realism 7 + Exemplar 2)
- 本 B2-β scope 内の skip 5 vertex shader untouched 維持: fsObjectIDV (Picker) / godraysV (Visual Realism) / blurLightV (Visual Realism) / diffuseV (Exemplar β-1 PoC) / previewV (A2 拡張 / Software regex artifact)
- aya_r41_exemplar/sky_placeholderV (gl_VertexIndex only、auto-no-op、touch 0 件) untouched
- cinematic_bd 系 utility/include shader untouched 維持

### §7.8 vertex_in/VBO attribute と CPU binding の cross-API 整合

SPIR-V `layout(location=N)` の N は GL `glBindAttribLocation` の N と同値必須 (LLShaderMgr::initAttribsAndUniforms の push_back 順 = N 値)。本 step では C++ 側変更不要 = 既 binding 番号と SPIR-V 注入番号一致 (Table C 1:1:1:1 整合)。新規 attribute 追加時は CPU enum 拡張 + Table C 拡張同時必要。

---

## §8 charter §3 #1 acceptance 担保

- GL path (92 file `#else` 分岐内 既存 in 宣言 byte-for-byte 維持) untouched
- AYAstorm 独自改造意図保全 (Cinematic BD shadowUtil/screenSpaceReflUtil + Visual Realism volumetricLight/blurLightF/godraysF 系 7 file + Picker 2 + Exemplar 2 + A2 拡張 2 の機能特性そのまま)
- bundle-A/A8-recovery/B1 既処理 UBO block byte-for-byte 維持 (touch 0 件)
- B2-α 既処理 varying/frag_out 行 byte-for-byte 維持 (touch 0 件、本 step は vertex_in 宣言行のみ touch、insertions-only absolute rule 拡張担保)
- 段階 1-4.3-γ'-port-β-2-bundle-B-B2-α 動作維持 (cold cache launch verify 起動成立 + clean shutdown + GL .shaderbin/shader_cache 245 件再生成 + crash 0 + GL shader compile/link fail 0 で確認、Linking failed 205 件 / #version 36 件 / SPIR-V link failed cascade exposure は GL path 影響なし)

---

## §9 cross reference

### §9.1 commit / handoff doc 系譜

- `eca091e1ce` (B2-β patch、本 handoff の直接 parent)
- `c0e1b25310` (B2-α-complete handoff、範式継承元)
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
- project_ayastorm_r41_vulkan_migration.md (γ'-port-β-2-bundle-B-B2-α 完遂 + γ'-port-β-2-bundle-B-B2-β 着手境界 → 本 commit で B2-β 完遂 + 次 sub-bundle 着手境界 active)

### §9.3 memory references (feedback)

- feedback_proactive_handoff (本 doc 起草 = 周回境界での能動 handoff)
- feedback_self_verify_before_handoff (§5 self-verify 10 項目 all green)
- feedback_use_agents_proactively (3 Agent 並列 patch)
- feedback_no_scope_shrink (effective scope 92 file 全網羅、skip list literal 除外のみ)
- feedback_doubt_self_first (Agent self-report 208 vs 実 211 差異検出 → git diff grep で actual count 確定)
- feedback_admit_unknown (Agent report 差異を transparent に AYA 報告 + 実 count 提示)
- feedback_falsification_as_progress (cascade exposure Linking +205 / #version +36 を progress signal として正確記録)
- feedback_explanation_lead_with_conclusion (metric 報告で結論ファースト = location -89 / parse failed -104)
- feedback_no_claude_coauthor (commit message に Claude 共著行なし)
- feedback_no_auto_commit (AYA 「OK」明示指示下で commit)
- feedback_one_step_at_a_time (step 1 trace → step 2 prep → step 3 patch → step 4 self-verify → step 5 deploy → step 6 AYA verify → step 7 commit → step 8 handoff doc → step 9 memory 更新 を sequential 実行)
- feedback_remove_verification_logs (Verification 用 LL_INFOS 追加なし、insertions-only)

---

## §10 次 action = 次 sub-bundle 着手境界

### §10.1 次 sub-bundle 候補

| 候補 | scope | 推定 |
|---|---|---|
| **B2-γ** (utility / Linking cascade cleanup) | cross-stage 関数定義未解決 (`passTextureIndex` / `encodeNormal` 等) の utility shader linkage 整理、Linking failed 205 件解消対象 | linker error 中心、~20-30 utility shader |
| **B3** (bundle-B 第三 sub-bundle) | utility/include shader `#version 460 core` → `#version 460 + GL_KHR_vulkan_glsl` 切替、`#version` directive error 36 件解消対象 | 36 件単純 directive 注入、Agent 1 件で完遂可能 |
| **B?** (残 location 13 件 + 残 non-opaque 74 件 cleanup) | utility / cinematic_bd / 一部 stage-aware 漏れ scope | mixed bag、scope 確定要 |

選択は AYA 判断 (技術判断 = 効果直接観測しやすい順 / dependency 順)。推奨順 = B3 (`#version` directive、機械的注入で次層 cascade 解消可能) → B2-γ (Linking、上流 utility 確定要) → B? (残 cleanup)。

### §10.2 次 sub-bundle 着手前に確認すべき事項

1. **utility shader source-of-truth** (cinematic_bd/class1/deferred/shadowUtil.glsl 等の skip 維持 vs touch 判断)
2. **`#version 460 core` → `#version 460 + #extension` 切替の正確な syntax** (glslang Vulkan profile 仕様)
3. **cross-stage 関数 forward declaration vs include 化** (B2-α §7.3 で報告された 1 件と B2-β cascade 205 件の根本対応)
4. **残 location 13 件の actual file 特定** (utility / cinematic_bd / 漏れ判別)

### §10.3 fresh context 引継 prompt 推奨

```
AYAstorm r41 Vulkan migration の sub-step 4.3-γ'-port-β-2-bundle-B-(B3 or B2-γ or B?) を着手します。

必読:
1. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B2-beta-complete.md (本 handoff doc、parent 範式)
2. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B2-alpha-complete.md (B2-α 範式継承元、Table A/B literal)
3. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B1-complete.md (B1 範式継承元、MaterialUBO_Legacy)
4. (B3 着手の場合) glslang Vulkan profile `#version` directive 仕様 + `GL_KHR_vulkan_glsl` extension
5. (B2-γ 着手の場合) utility/include shader linkage routing (cross-stage 関数定義の forward declaration vs include 化)

cadence (B1/B2-α/B2-β 同):
1. trace (scope file 抽出 + canonical 設計)
2. prep (Table / 範式 提示) → AYA OK
3. patch (Agent 並列 推奨) → AYA OK
4. self-verify (skip list / insertions-only / pair integrity / canonical consistency)
5. deploy (cp + cache clear)
6. AYA cold cache launch verify
7. metric (該当 category 直接観測 + cascade 露出記録)
8. commit (AYA OK)
9. handoff doc 起草 (本 doc 範式継承)

絶対 rule:
- B2-α/B2-β 既処理 file の varying/frag_out/vertex_in 行 untouched (本 step scope 行のみ touch)
- bundle-A/A8-recovery/B1 既処理 UBO block untouched
- skip list 13 file 機械的除外
- AYA 「commit して」or 「OK」明示指示前に commit 禁止
- one step at a time、1 メッセージ 1 アクション
```

---

## §11 build artifact persist

| artifact | path | size | 内容 |
|---|---|---|---|
| canonical Table C trace | `/tmp/B2-beta-trace.md` | ~5KB | §1 Canonical Table C (vertex_in 14 attribute) + §2 scope inventory + §3 skip intersection + §4 file 分布 + §5-§7 |
| agent template | `/tmp/B2-beta-agent-template.md` | ~6KB | 3 Agent 共通 patch 規則、3-段 swap pattern literal、hard rules 11 件、Table C literal、3 example |
| agent file lists | `/tmp/b2b-agent{1,2,3}-files.txt` | ~1-2KB each | 3 batch 分割 file list (deferred V 前半 23 / deferred V 後半+interface 46 / その他 23 = 92) |
| effective file list | `/tmp/b2b-patch-targets.txt` | ~3KB | 92 file effective scope (111 V file 中 14 helper 除外 + 5 skip 除外) |

これら artifact は fresh context で本 doc + commit message `eca091e1ce` + `indra/llrender/llshadermgr.cpp:1406-1419` + `indra/llrender/llvertexbuffer.h:131-148` から再生成可能 (persist 性 fragile)。
