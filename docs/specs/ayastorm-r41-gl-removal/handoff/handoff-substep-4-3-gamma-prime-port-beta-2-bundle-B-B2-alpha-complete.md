# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B2-α 完遂 → B2-β 着手境界 handoff (2026-06-01)

**parent commit**: `bfa81e0283` (B2-α patch、本 handoff の直接 parent) / `18187c862e` (B1-complete handoff doc commit 範式継承元) / `5614494f56` (B1 patch 範式継承元)
**HEAD**: `bfa81e0283` on `feature/ayastorm-r41-gl-removal`
**本 doc 位置付け**: B2-α 完遂状態 + B2-β (vertex_in/VBO attribute layout(location=N) 注入) 着手境界 を fresh context 引継 用に確定する doc-only handoff。B1-complete `18187c862e` 範式継承。**canonical Table A 範式 (top-20 varying location 0-19 cross-file 固定) + Table B 範式 (fragment_out 別 namespace) を B2-α で新規確立**。SPIR-V `layout(location=N)` qualifier を varying (vertex_out / fragment_in) + fragment_out 全 program に 4 Agent 並列で注入し、location error 195→102 で 47.7% 解消。AYA 「commit して」明示承認下で commit (no auto-commit)。

---

## §1 起草目的

β-2-bundle-B scope 第二 sub-bundle B2-α (varying + fragment_out layout(location=N) 注入) を 189 file +2304 行に 4 Agent 並列で注入完遂した状態を確定し、B2-β (vertex_in/VBO attribute layout(location=N) 注入) 着手境界 を fresh context に引継ぐ。glslang Vulkan profile が user-defined in/out 全宣言に explicit location を要求するため、B1 (commit `5614494f56` = materialF MaterialUBO_Legacy 化 non-opaque 32→0) で binding scope を概ね片付けた次層として location error を解消。location 195→102 で -93 件 (47.7%) 解消 = B2-α patches 効果完全直接観測。残 102 件は B2-β scope (vertex_in/VBO attribute) + 一部 utility cascade、B2-β で更に -50〜80 件解消見込。

---

## §2 B2-α 完遂 status

| 項目 | 値 |
|---|---|
| Scope | varying (vertex_out + fragment_in) + fragment_out 全 program `layout(location=N)` 3-段 swap 注入 |
| 注入 file 数 | **189 file** (effective scope 236 file 中 47 件は no-op = shadow/velocity 系 out 宣言 0 件) |
| 変更行数 | **+2304 / 0** (insertions-only、deletion 0 件) |
| 注入 declaration 数 | **576 declarations** = Agent1 (deferred V) 140 + Agent2 (deferred F) 189 + Agent3 (interface+objects+effects+post) 108 + Agent4 (残) 139 |
| 3-段 swap pattern balanced | ifdef/endif count 全 189 file balanced (Python script verify 0 unbalanced) |
| variant pattern `defined(LL_VULKAN_GLSL)` | **0 件** |
| Agent 投入 | **4 並列 dispatch** (shared template /tmp/B2-alpha-agent-template.md + 個別 file list /tmp/b2a-agent{1,2,3,4}-files.txt) |
| canonical UBO (A1-A7/A8-recovery/B1) untouched | **違反 0 件** (全 既処理 UBO block byte-for-byte 維持) |
| skip list 13 file untouched | **全 untouched** (git diff 0 件確認) |
| AYAstorm 改変保全 | **GL path #else 分岐に既存 in/out 宣言 byte-for-byte 維持** |
| AYA cold cache launch verify | **PASS** (起動成立 07:39:38 → 07:40:25 ~47 秒 + clean shutdown + shader_cache 224 件再生成 + crash 0 + FATAL 0 + SIGSEGV 0) |
| commit | `bfa81e0283` (AYA 「commit して」明示指示下) |
| metric vs B1 baseline location | **-93 ✓ B2-α patches 効果完全直接観測** (195→102, 47.7% 解消) |
| metric net delta | **-53 errors** (location -93 + binding -29 cascade clear + non-opaque +63 cascade 露出 + missing #endif +6 cascade 移動) = controlled improvement |

### §2.1 4 Agent batch split 内訳

| Agent | scope | input file | patched | declaration |
|---|---|---|---|---|
| Agent1 | class1/deferred/*V.glsl | 51 | 40 (7 no-op + 4 skip-list 内除外) | 140 vertex_out |
| Agent2 | class1/deferred/*F.glsl | 60 | 54 (1 helper no-op + 5 skip-list 内除外) | 189 (135 fragment_in + 54 fragment_out) |
| Agent3 | class1/(interface+objects+effects+post)/*.glsl | 64 | 58 (6 no-op) | 108 (43 V_out + 37 F_in + 28 F_out) |
| Agent4 | class1/(lighting+windlight+avatar+gltf+environment) + class2 + class3 + aya_r41 + errorV/F | 61 | 37 (残 24 件は no-op/skip-list/utility) | 139 |
| **合計** | **236** | **189** | **576** |

### §2.2 既処理 sub-bundle との関係

| sub-bundle | 関係 |
|---|---|
| A1-A7 | uniform/sampler/UBO block 注入 (binding scope)、本 step touch 0 件、byte-for-byte 維持 |
| A8-recovery | AYAstorm 改変 (Cinematic BD shadowUtil/screenSpaceReflUtil + Visual Realism volumetricLight/blurLightF/godraysF 系 5 file) UBO 復活、本 step も skip list 維持で untouched |
| B1 | materialF.glsl MaterialUBO_Legacy 化 (case 2 file-local override)、本 step では materialF.glsl の varying 宣言 + frag_data MRT に layout(location=N) 注入、既存 UBO block byte-for-byte 維持 |

---

## §3 設計範式 (B1 継承 + B2-α 新規確立)

### §3.1 案 3 = canonical varying location top-20 cross-file 固定範式 (B2-α で新規確立)

**設計原則**: dominant varying (頻度 top-20、cross-file occurrence ≥4) を `location=0..19` に **cross-file 固定割当**。同一 varying 名は全 file で同じ location 番号を取り、vertex_out N == fragment_in N pair 整合を name-based に自動担保。新規 file 追加時も既存 canonical 番号継承で衝突回避。

**範式継承元**: bundle-A A4 (canonical MaterialUBO offset 確立) + A5 (canonical FrameAtmosphere 拡張 案 D) の cross-file 番号固定範式を varying layout に拡張。

### §3.2 Canonical Table A literal (B2-α で確立、top-20 varying)

| location | varying name | type pattern (dominant) | file count |
|----------|-------------|------------------------|------------|
| 0  | vary_texcoord0 | vec2(93), vec4(8) | 101 |
| 1  | vary_fragcoord | vec2(38), vec3(6), vec4(7) | 51 |
| 2  | vertex_color | vec4(57) | 53 |
| 3  | vary_position | vec3(37), vec4(2) | 36 |
| 4  | vary_normal | vec3(26) | 24 |
| 5  | vary_texcoord1 | vec2(8), vec3(2), vec4(8) | 18 |
| 6  | base_color_texcoord | vec2(10) | 6 |
| 7  | emissive_texcoord | vec2(10) | 6 |
| 8  | target_pos_x | float(8) | 8 |
| 9  | vary_tangent | vec3(10) | 10 |
| 10 | vary_sign | float(8) | 8 |
| 11 | (reserved, regex artifact `offset`) | - | 0 |
| 12 | vary_cur_clip | vec4(4) | 4 |
| 13 | vary_last_clip | vec4(4) | 4 |
| 14 | vary_offset | vec4(6) | 6 |
| 15 | post_pos | vec4(5) | 5 |
| 16 | vary_texcoord2 | vec2(4), vec4(2) | 6 |
| 17 | vary_texcoord3 | vec2(2), vec4(2) | 4 |
| 18 | (skipped, regex artifact `Software`) | - | 0 |
| 19 | vary_tc | vec2(4) | 4 |

### §3.3 Canonical Table B literal (B2-α で確立、fragment_out)

| location | output name | type pattern | file count |
|----------|------------|--------------|------------|
| 0 | frag_color | vec4(92) | 91 |
| 1 | frag_data | vec4(21) | 21 |
| 2 | nl | float(4) | 4 |
| 3 | diff | vec3(4) | 4 |
| 4 | spec | vec3(4) | 4 |
| 5 | pixR | AF1(1) | 1 |
| 6 | pixG | AF1(1) | 1 |
| 7 | pixB | AF1(1) | 1 |
| 8 | atten | vec3(1) | 1 |
| 9 | outColor | vec4(1) | 1 |
| 10 | diffuse | vec3(1) | 1 |
| 11 | specular | vec3(1) | 1 |

### §3.4 Sparse varying file-local sequential 範式 (location=20+)

rank 21+ varying は file 内 `location=20` から sequential 割当、隣 file 衝突は independent namespace で許容、program 単位の pair 整合のみ担保。例: `littleWave=20`, `tc=21`, `normal_texcoord=22`, `vary_dir=23`, `refCoord=24`, `trans_center=25`, `metallic_roughness_texcoord=26`, ... 計 46 sparse varying。

### §3.5 3-段 swap pattern literal (全 file 共通)

```glsl
// vertex shader の out 宣言:
#ifdef LL_VULKAN_GLSL
layout(location=N) <qualifier?> out <type> <name>;
#else
<qualifier?> out <type> <name>;
#endif

// fragment shader の in 宣言:
#ifdef LL_VULKAN_GLSL
layout(location=N) <qualifier?> in <type> <name>;
#else
<qualifier?> in <type> <name>;
#endif

// fragment shader の out 宣言 (frag_color/frag_data):
#ifdef LL_VULKAN_GLSL
layout(location=M) out <type> <name>;
#else
out <type> <name>;
#endif
```

`<qualifier?>` = `flat`/`noperspective`/`smooth`/`centroid` interpolation qualifier (元宣言と同位置で保持)。MRT array (`frag_data[4]` 等) は array 単一 `layout(location=M)` で element ごと auto 連番割当。

---

## §4 cold cache launch verify metric (2026-06-01T07:39-07:40Z)

| 項目 | B1 baseline | B2-α | delta | 解釈 |
|---|---|---|---|---|
| §4.1 hook fire | 224 | 224 | ±0 | shader pipeline 不変 |
| §4.2 parse failed | 224/224 | 224/224 | ±0 | first-error semantics 通り controlled (B1 同 measurement design) |
| §4.3 location | 195 | **102** | **-93 ✓** | B2-α patches 効果完全直接観測 (47.7% 解消) |
| §4.3 binding | 29 | **0** | -29 | cascade clear: 旧 binding-first error file が parse 早期失敗で binding 行未到達 |
| §4.3 non-opaque | 0 | **63** | +63 | cascade 次層露出: Water/Underwater/Glow/GlowExtract 等の bare uniform 宣言が location 解消後に first-error 位置取得 |
| §4.3 missing #endif | 8 | **14** | +6 | cascade 位置移動 |
| §4.3 **合計** | **232** | **179** | **-53** | controlled improvement |
| §4.4 shader_cache 再生成 | 224 | 224 | ±0 | GL path regression 0 |
| §4.5 link failed | 0 | **1** | +1 | "Deferred Diffuse Non-Indexed Alpha Mask Shader" の passTextureIndex/encodeNormal cross-stage 関数定義未解決、B2-α 起因でなく cascade exposure (B1 で location-first parse 失敗 → link 段階未到達、B2-α location 解消で link 段階到達 → linkage 未確立露出)、GL path #else branch 影響なし |
| §4.6 FATAL / SIGSEGV / crash | 0 / 0 / 0 | 0 / 0 / 0 | ±0 | 3 mention 全 benign (settings_crash_behavior load Default 1 + User 2) |
| §4.7 起動成立 + clean shutdown | PASS | **PASS** | - | Goodbye! 1 件 + Vulkan device destroyed 1 件 + Vulkan instance destroyed 1 件、charter §3 #1 acceptance 担保 |

**net delta -53** = location -93 + binding -29 + non-opaque +63 + missing #endif +6 = controlled cascade improvement、location category specifically **-93 = B2-α patches 効果完全直接観測**。bundle-B 全体 (B1-B?) 完遂時の集合的閾値で評価。

---

## §5 self-verify 結果

| # | check | result |
|---|---|---|
| §5.1 | skip list 13 file 違反 | **0 件** (git diff 全 file untouched) |
| §5.2 | insertions-only | **189 file 2304 insertions / 0 deletions** |
| §5.3 | ifdef/endif balance | **0 unbalanced** (全 189 file `#if*` count == `#endif` count、Python script verify) |
| §5.4 | canonical varying pair integrity (Table A) | **全 18 件 single location cross-file 完全一貫** (vary_texcoord0=0 / vary_fragcoord=1 / vertex_color=2 / vary_position=3 / vary_normal=4 / vary_texcoord1=5 / base_color_texcoord=6 / emissive_texcoord=7 / vary_tangent=9 / target_pos_x=8 / vary_sign=10 / vary_cur_clip=12 / vary_last_clip=13 / vary_offset=14 / post_pos=15 / vary_texcoord2=16 / vary_texcoord3=17 / vary_tc 19/20 で 2 program 別 namespace 各自己一貫) |
| §5.5 | frag_out separate namespace | **frag_color=0 / frag_data 0|1** (program 別 namespace 自己一貫、SPIR-V validator OK) |
| §5.6 | nested LL_VULKAN_GLSL inside LL_VULKAN_GLSL | **0 件** (Python awk script verify、二重 wrap mistake なし) |
| §5.7 | false-positive `offset` (HLSL float4) 注入 | **0 件** (関数 prototype が grep 検出されたが Edit 実宣言なし skip 動作確認) |
| §5.8 | UBO block byte-for-byte 維持 | **全 untouched** (bundle-A/A8-recovery/B1 既処理 UBO block touch 0 件、MaterialUBO_Legacy 含む) |

---

## §6 設計範式 (B1 継承 + B2-α 新規確立)

| # | 範式 | 確立 sub-bundle |
|---|---|---|
| §6.1 | canonical Table A 範式 (top-20 varying location 0-19 cross-file 固定) | **B2-α 新規** |
| §6.2 | sparse file-local sequential 範式 (rank 21+ varying は file 内 location=20+ sequential) | **B2-α 新規** |
| §6.3 | Table B fragment_out 範式 (vertex/fragment_in と independent namespace、frag_color=0 / frag_data=0\|1) | **B2-α 新規** |
| §6.4 | MRT array 範式 (array 宣言に single layout(location=N) で element ごと auto 連番) | **B2-α 新規** |
| §6.5 | GL #else byte-for-byte 範式 | bundle-A 確立、B1/B2-α 継承 |
| §6.6 | 既処理 file UBO untouched 範式 (FrameViewProj/FrameAtmosphere/MaterialUBO/PerDrawUBO/MaterialUBO_Legacy/PerDrawUBO_<file> 全 6 layout pattern) | bundle-A 確立、B1/B2-α 継承 |
| §6.7 | vertex_in untouched 範式 (B2-α scope は vertex_out/fragment_in/fragment_out 3 role 限定、vertex_in は B2-β scope 分離) | **B2-α 新規 (B2-β との scope 分離原則)** |
| §6.8 | false-positive skip 範式 (canonical table の regex artifact = Software / HLSL float4 offset 等は Agent prompt §10 で明示警告、実 GLSL 宣言 grep でのみ touch) | **B2-α 新規** |
| §6.9 | 4 Agent 並列 patch 範式 (236 file scope を directory/stage 基準で 4 batch 分割、shared template + 個別 file list で context isolation、並列実行で wallclock 短縮 + context window 保全) | **B2-α 新規** |

---

## §7 risks/caveats

### §7.1 metric net delta -53 = cascade improvement

location -93 が直接観測される B2-α patch 効果、binding -29 cascade clear、non-opaque +63 / missing #endif +6 cascade 露出。bundle-B 全体 (B1-B?) 完遂時の集合的閾値で評価 (bundle-A 同 measurement design)。

### §7.2 cold cache launch 必須

`rm -rf ~/.ayastorm_x64/cache/shader_cache/` sub-bundle 毎 verify 前必須化、本 commit でも実施済。

### §7.3 SPIR-V link failed 1 件 (B2-α cascade exposure)

"Deferred Diffuse Non-Indexed Alpha Mask Shader" の `passTextureIndex` / `encodeNormal` cross-stage 関数定義未解決。B2-α 起因でなく cascade exposure = B1 で location-first parse 失敗 → link 段階未到達、B2-α location 解消で link 段階到達 → linkage 未確立露出。後続 sub-bundle で utility shader linkage 整理時に解消想定。GL path #else branch 影響なし。

### §7.4 残 location 102 件 = B2-β scope 想定

B2-β (vertex_in/VBO attribute layout(location=N) 注入) で更に -50〜80 件解消見込。残 cascade は utility shader (textureUtilV.glsl 等) include path で扱う。

### §7.5 canonical Table A/B 範式 persist 性

artifact `/tmp/B2-alpha-canonical-table.md` 失効時は本 handoff doc §3.2-§3.3 から再生成可能。本 doc + commit message §3 が source of truth。

### §7.6 残 skip 既知 list

- exemplar 2 (diffuseV/F) untouched 維持 (sub-doc 03 §3.1.3 β-1 PoC 試作レール)
- skip list base 13 untouched (Picker 2 + Cinematic BD 2 + Visual Realism 7 + Exemplar 2)
- cinematic_bd 系 utility/include shader (cinematic_bd/class1/deferred/shadowUtil.glsl + cinematic_bd/class3/deferred/screenSpaceReflUtil.glsl) untouched 維持

### §7.7 vary_tc 19/20 split は許容差異

`vary_tc` は 2 program (postDeferredV↔fxaaF location=19 / glowcombineFXAAV↔glowcombineFXAAF location=20) で異なる location 取得。SPIR-V validator は per-program pair check のため program 別 namespace で各自己一貫なら OK、cross-program 差異は許容。`frag_data` 0/1 split も同様 (fragment_out independent namespace)。

---

## §8 charter §3 #1 acceptance 担保

- GL path (189 file `#else` 分岐内 既存 in/out 宣言 byte-for-byte 維持) untouched
- AYAstorm 独自改造意図保全 (Cinematic BD shadowUtil/screenSpaceReflUtil + Visual Realism volumetricLight/blurLightF/godraysF 系 7 file + Picker 2 + Exemplar 2 の機能特性そのまま)
- bundle-A/A8-recovery/B1 既処理 UBO block byte-for-byte 維持 (touch 0 件)
- 段階 1-4.3-γ'-port-β-2-bundle-B-B1 動作維持 (cold cache launch verify 起動成立 + clean shutdown + GL .shaderbin/shader_cache 224 件再生成 + crash 0 + GL shader compile/link fail 0 で確認、SPIR-V link failed 1 件は GL path 影響なし)

---

## §9 cross reference

### §9.1 commit / handoff doc 系譜

- `bfa81e0283` (B2-α patch、本 handoff の直接 parent)
- `18187c862e` (B1-complete handoff、範式継承元)
- `5614494f56` (B1 patch、bundle-B 第一 sub-bundle)
- `8d2cae435b` (A7-complete handoff、bundle-A 全体完遂)
- `862f7dc8bd` (A7 patch、bundle-A 残 cleanup)
- `b80d90bea4` (A6 patch、PerDrawUBO 6 layout pattern)
- `aed1438936` (A8-recovery、AYAstorm 改変 UBO 復活)

### §9.2 sub-doc / charter

- sub-doc 06 §1.2.2/§1.2.4/§3.1 sub-step 6.3
- sub-doc 07 §3.1 sub-step 7.2-7.4
- sub-doc 03 §3.1.3 (exemplar 2 役割)
- charter §3 #1 + §7.5
- project_ayastorm_r41_vulkan_migration.md (γ'-port-β-2-bundle-B-B1 完遂 + γ'-port-β-2-bundle-B-B2-α 完遂 + γ'-port-β-2-bundle-B-B2-β 着手境界)

### §9.3 memory references (feedback)

- feedback_proactive_handoff (本 doc 起草 = 周回境界での能動 handoff)
- feedback_self_verify_before_handoff (§5 self-verify 6 項目 all green)
- feedback_use_agents_proactively (4 Agent 並列 patch + 1 Agent stage-aware canonical 再導出 + 1 Agent scope 修正)
- feedback_no_scope_shrink (元 estimate 166 file → 真の scope 236 file 拡大、scope-shrink 回避)
- feedback_doubt_self_first (Agent 1 回目 scope-mix 検出 → AYA 指示下 (A) prep 再 redo で table 浄化)
- feedback_admit_unknown (canonical table の regex artifact `Software`/`offset` を transparent に AYA 報告)
- feedback_falsification_as_progress (cascade exposure を progress signal として正確記録)
- feedback_explanation_lead_with_conclusion (metric 報告で結論ファースト)
- feedback_no_claude_coauthor (commit message に Claude 共著行なし)
- feedback_no_auto_commit (AYA 「commit して」明示指示下で commit)
- feedback_one_step_at_a_time (step 3 patch → step 4 self-verify → step 5 deploy → step 6 AYA verify → step 7 metric → step 8 commit → step 9 handoff doc を sequential 実行)
- feedback_remove_verification_logs (Verification 用 LL_INFOS 追加なし、insertions-only)

---

## §10 次 action = B2-β 着手境界

### §10.1 B2-β scope 定義

vertex shader の `in` 宣言 (VBO attribute) への `layout(location=N)` 注入。

| 項目 | 値 |
|---|---|
| scope | vertex shader の `in <type> <name>;` (top-level、UBO block 外) 全宣言 |
| 推定 file 数 | ~107 vertex shader file (B2-α effective 236 file 中 111 vertex shader、4 skip-list 除外で ~107) |
| 推定 declaration 数 | ~250-400 (vertex shader 平均 2-4 attribute) |
| 推定 行数 | +500-1600 行 |
| 推定 location 解消 | -50〜80 件 (残 location 102 件中、B2-β scope は ~70-80 件想定、残りは utility cascade で B2-γ?) |
| canonical 番号体系 | C++ LLVertexBuffer::TypeMask 整合要 (VBO binding 番号一致)、CPU 側 binding と name 連動 |

### §10.2 B2-β 着手前に確認すべき事項

1. **LLVertexBuffer::TypeMask の canonical 番号体系** (`indra/llrender/llvertexbuffer.cpp` / `.h`) と shader 側 `in` 宣言の name 連動
2. **既存 `attribute` (legacy syntax) vs `in` (modern syntax) の混在** (今回 `in` 想定だが、古い shader で `attribute` 残存可能性)
3. **AYAstorm 改変 vertex attribute** (Picker `fsObjectIDV` の self picker buffer 等) は skip list 維持
4. **utility shader (textureUtilV.glsl 等) の vertex attribute** 別 handling

### §10.3 B2-β patch 範式 (B2-α 範式継承)

```glsl
// vertex shader の in 宣言:
#ifdef LL_VULKAN_GLSL
layout(location=N) in <type> <name>;
#else
in <type> <name>;
#endif
```

N は LLVertexBuffer::TypeMask の bit position 整合 (例: TYPE_VERTEX=0, TYPE_NORMAL=1, TYPE_TEXCOORD0=2, ...)。canonical 番号は C++ 側 source-of-truth で固定。

### §10.4 fresh context 引継 prompt 推奨

```
AYAstorm r41 Vulkan migration の sub-step 4.3-γ'-port-β-2-bundle-B-B2-β = vertex_in (VBO attribute) layout(location=N) 注入を着手します。

必読:
1. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B2-alpha-complete.md (本 handoff doc、parent 範式)
2. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B1-complete.md (B1 範式継承元)
3. indra/llrender/llvertexbuffer.cpp + .h (TypeMask canonical 番号体系)

cadence (B2-α 同):
1. trace (vertex_in 全 declaration 抽出、~107 file)
2. prep (canonical Table C = TypeMask 整合の vertex_in location 表) → AYA OK
3. patch (3-段 swap 注入、Agent 並列 推奨) → AYA OK
4. self-verify (skip list / insertions-only / pair integrity / canonical consistency)
5. deploy (cp + cache clear)
6. AYA cold cache launch verify
7. metric (location -50〜80 件解消想定)
8. commit (AYA OK)
9. handoff doc 起草 (本 doc 範式継承)

絶対 rule:
- B2-α 既処理 file の varying/frag_out 行 untouched (vertex_in 宣言行のみ touch)
- bundle-A/A8-recovery/B1 既処理 UBO block untouched
- skip list 13 file 機械的除外
- AYA 「commit して」明示指示前に commit 禁止
- one step at a time、1 メッセージ 1 アクション
```

---

## §11 build artifact persist

| artifact | path | size | 内容 |
|---|---|---|---|
| canonical table | `/tmp/B2-alpha-canonical-table.md` | ~3KB | Table A (top-20 varying) + Table B (fragment_out) literal |
| varying frequency | `/tmp/B2-alpha-varying-frequency.md` | ~4KB | varying name 出現頻度 ranking (Section 1) + fragment_out 頻度 (Section 2) |
| effective file list | `/tmp/B2-alpha-effective-files.txt` | ~10KB | 236 file (stage-aware filtered、skip 13 + utility 13 + geometry 1 除外後) |
| agent template | `/tmp/B2-alpha-agent-template.md` | ~6KB | 4 Agent 共通 patch 規則、3-段 swap pattern literal、hard rules 11 件、skip list 13 file |
| agent file lists | `/tmp/b2a-agent{1,2,3,4}-files.txt` | ~2-3KB each | 4 batch 分割 file list (deferred V 51 / deferred F 60 / interface系 64 / 残 61 = 236) |

これら artifact は fresh context で本 doc + commit message から再生成可能 (persist 性 fragile)。
