# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2d-α prep handoff

**作成日**: 2026-06-03
**branch**: `feature/ayastorm-r41-gl-removal`
**前 handoff**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2c-complete.md` (commit `e5d3de6ecc` 起草、Phase 2c 末 ERROR 24 / parse fail event 7 / link 0 / Water+CoF+BlurLight+PbrTerrainV 4 program PASS、PbrTerrainV 2 permutation V-stage PASS)
**状態**: Phase 2d **prep 起草、sub-phase 分離決定 (本 prep は 2d-α scope のみ)**。AYA 承認後 feat 適用 → cold launch verify → complete handoff 起草。2d-β は次々 session で別 prep doc 起草。

---

## §0 Phase 2d sub-phase 分離 spec (本 prep の最重要事項)

Phase 2c complete handoff §4.2 で提案した 2 sub-phase 分離を本 prep で確定。Phase 2c 末残 7 root を **struct/alias scope 修正系 (binding 連番未消費)** と **UBO 化系 (binding 連番消費)** に分離し、本 prep は前者 = 2d-α のみ scope 化。

### §0.1 分離根拠 (`feedback_no_scope_shrink` と `feedback_one_step_at_a_time` の整合)

- η-28 sub-bundle の literal scope は **UBO 化作業** (binding 連番 1, 2, 3, … と消費)。本 phase 7 root のうち 4 root は struct redef guard / alias scope refinement / 既存 UBO 拡張で binding 連番未消費 = η-28 sub-bundle の literal scope 外。
- 2 sub-phase に分離することで **(a) sub-phase ごとに範式適用カテゴリが揃う** (2d-α = struct/alias scope、2d-β = UBO 化)、**(b) verify サイクルが 1 sub-phase 4-5 file に収まり AYA cold launch 浪費が抑えられる** (Phase 2a-2c の sub-phase あたり 5 file 規模を維持)。
- `feedback_no_scope_shrink` 違反ではない: 7 root 全消化は両 sub-phase 合算で達成、本 prep は 4 root 全消化を literal scope として固定。Phase 2c complete §4 表で **AYA が事前に sub-phase 分離提案を読み合意** している前提 (本 prep doc レビュー段階で再度合意取得)。

### §0.2 2d-α scope (本 prep): 4 root 5 file (binding 連番未消費)

| root id | program | stage | error 種別 | 修正カテゴリ | 編集 file |
|---|---|---|---|---|---|
| Issue B | Deferred Light Shader | F | `size` undeclared (L2439) | deferredUtil alias scope refinement | `class3/deferred/pointLightF.glsl` |
| Issue C ×2 | Deferred PBR Terrain Shader (heightmap + paintmap 2 permutation) | F | `TerrainMix` struct redefinition (L1750) | struct redef guard | `class1/deferred/pbrterrainF.glsl` + `class1/deferred/pbrterrainUtilF.glsl` + `class1/interface/pbrTerrainBakeF.glsl` (予防) |
| Issue E | Deferred SpotLight Shader | F | `color` undeclared (L2628) | deferredUtil alias scope refinement (Issue B 同型) | `class3/deferred/spotLightF.glsl` |
| Issue F | Deferred MultiSpotLight Shader (= spotLightF + MULTI_SPOTLIGHT permutation) | F | non-opaque uniforms outside a block (L2385) | 既存 PerProgramUBO_SpotLightF 拡張 (η-28-C type 3 同型) | `class3/deferred/spotLightF.glsl` (Issue E と同 file) |

**file 数合計**: 5 file (pointLightF + spotLightF + pbrterrainF + pbrterrainUtilF + pbrTerrainBakeF)
**binding 連番**: 未消費 (Issue F は既存 binding 10 拡張で完結)
**parse fail event 解消見込み**: 5 件 (Issue B 1 + Issue C×2 2 + Issue E 1 + Issue F 1)

### §0.3 2d-β scope (次々 session prep): 2 root (binding 連番消費)

| root id | program | stage | error 種別 | 修正カテゴリ | 編集 file |
|---|---|---|---|---|---|
| 2d-β-G | Skinned Deferred PBR Alpha Shader | F | non-opaque uniforms outside a block (L3692) | UBO 化 (新 PerProgramUBO_PbrAlphaF 系) | `class2/deferred/pbralphaF.glsl` (HAS_SKIN path) |
| 2d-β-H | Deferred PBR Alpha Shader | V | `modelview_projection_matrix` undeclared (L1252) | FrameViewProj attach 不足 (Vulkan path guard 漏れ) + 必要に応じ新 UBO | `class1/deferred/pbralphaV.glsl` (!HAS_SKIN path) |

**file 数見込み**: 2-3 file (class2/pbralphaF + class1/pbralphaV、HUD path で派生があれば +1)
**binding 連番**: 26+ 消費見込み (HAS_SKIN PerProgramUBO_PbrAlphaF 1 binding、!HAS_SKIN 補完で 0-1 binding)
**parse fail event 解消見込み**: 2 件

**本 prep では 2d-β を予告のみ**、`feedback_one_step_at_a_time` に従い prep を 2 つ並行起草せず、2d-α verify 完了後に 2d-β prep を別 doc で起草。

### §0.4 分離後の Phase 2d 全体達成 (両 sub-phase 合算)

| 観測項目 | Phase 2c 末 | Phase 2d-α 末 期待 | Phase 2d-β 末 期待 | Δ 合算 |
|---|---|---|---|---|
| ERROR 行数 | 24 | ~9 (cascade 含む root 4 件解消) | ~0 | -24 |
| parse failed event 数 | 7 | 2 (残 2d-β scope) | 0 | -7 |
| link failed | 0 | 0 維持 | 0 維持 | 0 維持 |
| SPIR-V 全 program 生成 | 5 program PASS + PbrTerrainV V-stage 2 permutation PASS | + PbrTerrainF 2 permutation + PointLight + SpotLight + MultiSpotLight PASS (5 program 追加) | + PbrAlpha 2 program PASS | 全 program PASS |

---

## §1 Scope 詳細 (Phase 2c complete §3 表ベース、Phase 2d-α 直接対象 4 root)

### §1.1 Issue B: pointLightF.glsl `size` undeclared

**観測 (Phase 2c 末 log L751-753)**:
```
ERROR: 0:2439: 'size' : undeclared identifier
ERROR: 0:2439: compilation terminated
ERROR: 0: 'compilation' : compilation errors
```

**root cause (source-tree trace 完了)**:
1. `class1/deferred/deferredUtil.glsl` L173-185: Vulkan path で `PerDrawUBO_LightParams { vec3 spot_light_color; float spot_light_size; }` (set=2 binding=0、guard `PER_DRAW_UBO_LIGHT_PARAMS_DEFINED`) 宣言 + L180-181 で `#define color spot_light_color` / `#define size spot_light_size` alias
2. `class1/deferred/deferredUtil.glsl` L780-783: file 末尾で `#ifdef LL_VULKAN_GLSL #undef color #undef size #endif` (η-20 範式記述「後段 attach 文書 shadowUtil/reflectionProbeF/main shader への漏れ防止」)
3. `class3/deferred/pointLightF.glsl` L66-78: pointLightF 自身でも **同 guard** `PER_DRAW_UBO_LIGHT_PARAMS_DEFINED` で `PerDrawUBO_LightParams { vec3 color; float size; }` を宣言 (member 名は `color`/`size`、deferredUtil 側と異なる)
4. attach 順 (deferredUtil 先行 → pointLightF 後段) で:
   - deferredUtil 側 UBO 宣言が guard set → pointLightF L66-78 は guard 競合で **skip**
   - deferredUtil L780-783 `#undef color` / `#undef size` で alias 切断
   - pointLightF L195 `if (lightDist >= size)` / L199 `float dist = lightDist / size;` で `size` を直接参照 = alias 切断後、self 宣言 skip 状態、**どこにも `size` 名 symbol が存在しない** → undeclared
5. Phase 2c で `sun_wash` UBO 化 (Phase 2c feat commit `c08080ded2`) により pointLightF parse の早期 abort が解消 → 後段の `size` 参照位置まで parse が到達 → 隠れていた undeclared が顕在化 = cascade reveal

**pointLightF 本体内 `color` 参照** (Issue B と同型 cascade、本 prep §2.1 で同時修正):
- L216 `vec3 intensity = dist_atten * color * 3.25;`
- L237 `final_color = color.rgb*lit*diffuse;`
- L251 `final_color += lit*scol*color.rgb*spec.rgb;`

→ `size` だけでなく `color` も同型問題、修正は両方 rename。

### §1.2 Issue C: PBR Terrain F `TerrainMix` struct redefinition (2 permutation)

**観測 (Phase 2c 末 log L712-714 + L722-724)**:
```
ERROR: 0:1750: 'struct' : 'TerrainMix' redefinition
ERROR: 0:1754: 'compilation' : compilation terminated
ERROR: 0: 'compilation' : compilation errors
```

**root cause (source-tree trace 完了)**:
1. `class1/deferred/pbrterrainF.glsl` L47-51:
```glsl
struct TerrainMix
{
    vec4 weight;
    int type;
};
```
2. `class1/deferred/pbrterrainUtilF.glsl` L183-187: **完全同 struct 宣言** (member 名 + 順序まで一致)
3. **attach 経路**: `llviewershadermgr.cpp` L966 で `pbrterrainUtilF.glsl` は `addCommonShader` 経路 (`shaders.push_back(make_pair("deferred/pbrterrainUtilF.glsl", 1))` の "1" = frag stage common shader) で **`pbrterrainF` を attach する全 program に自動同梱**。よって同一 program 内に `TerrainMix` struct が 2 回宣言 → glslang strict mode で redefinition error
4. heightmap permutation (L712) と paintmap permutation (L722) で **同 root が 2 permutation で 2 回観測** (元コード自体の問題、Phase 2c の編集とは独立、Phase 2c V-stage UBO 化により V-stage parse 通過 → F-stage parse 試行 → struct redef 露呈で reveal)

**`class1/interface/pbrTerrainBakeF.glsl` L34**: 同 struct 宣言が **別 program** (`pbrTerrainBakeV+pbrTerrainBakeF`、llviewershadermgr.cpp L3934-3935 で attach) にも存在。本 program では `pbrterrainUtilF` を attach しないため redefinition は発生しないが、将来的に共通 utility を bake program でも参照する変更が入ると衝突。**予防的に同 guard wrap を提案** (本 prep §2.2 で含める)。

### §1.3 Issue E: spotLightF.glsl `color` undeclared

**観測 (Phase 2c 末 log L840-843)**:
```
ERROR: 0:2628: 'color' : undeclared identifier
ERROR: 0:2628: 'rgb' : vector swizzle out of range
ERROR: 0:2628: compilation terminated
ERROR: 0: 'compilation' : compilation errors
```

**root cause (source-tree trace 完了、Issue B と同型)**:
1. `class3/deferred/spotLightF.glsl` L152-158: spotLightF 自身で同 guard `PER_DRAW_UBO_LIGHT_PARAMS_DEFINED` で `PerDrawUBO_LightParams { vec3 color; float size; }` 宣言 (Issue B の pointLightF L66-78 と同型)
2. attach 順 (deferredUtil 先行 → spotLightF 後段) で:
   - deferredUtil 側 guard set → spotLightF L152-158 skip
   - deferredUtil 末尾 `#undef color` / `#undef size` で alias 切断
3. spotLightF L384 `final_color += color.rgb * texture2DLodSpecular(stc.xy, (1 - spec.a) * (proj_lod * 0.6)).rgb * shadow * envIntensity;` で `color.rgb` 参照 → undeclared
4. spotLightF 本体は `size` を直接参照しない (deferredUtil の helper 関数 `clipProjectedLightVars` 等経由 = deferredUtil 内部での参照、alias 有効範囲内で compile される)

### §1.4 Issue F: Deferred MultiSpotLight Shader (= spotLightF + MULTI_SPOTLIGHT permutation) non-opaque uniforms outside a block

**観測 (Phase 2c 末 log L851-853)**:
```
ERROR: 0:2385: 'non-opaque uniforms outside a block' : ...
ERROR: 0:2387: 'missing #endif' : ...
ERROR: 0: 'compilation' : compilation errors
```

**root cause (source-tree trace 完了)**:
1. `class3/deferred/spotLightF.glsl` L139-149:
```glsl
#if defined(MULTI_SPOTLIGHT)
uniform vec3 center;                       // ← L140 plain uniform、Vulkan path 露出
#else
#ifdef LL_VULKAN_GLSL
layout(location=60) in vec3 trans_center;
#else
in vec3 trans_center;
#endif
#endif
```
2. **`gDeferredMultiSpotLightProgram`** (llviewershadermgr.cpp L1796-1797 で `multiPointLightV+spotLightF` を attach + MULTI_SPOTLIGHT define 適用) では L140 が plain `uniform vec3 center;` のまま Vulkan path に露出 → glslang strict mode `non-opaque uniforms outside a block` error
3. spotLightF L226 `c = center;` (MULTI_SPOTLIGHT 時 only) で本体参照

**修正方向**: `vec3 center` を spotLightF 既存 `PerProgramUBO_SpotLightF` (set=2 binding=10) に追加 field として吸収。`!MULTI_SPOTLIGHT` permutation では declared-but-unused (η-28-C type 3 「同 file 内 preprocessor permutation 共有」範式の派生形)、binding 連番未消費。

### §1.5 cross-stage / cross-program check 表

| 検査対象 | 結果 | 編集要否 |
|---|---|---|
| Issue B pointLightF cross-stage (V pair = `pointLightV.glsl`) | V は η-23 で `PerProgramUBO_PointLightV` set=2 binding=5 既 UBO 化、`size`/`color` 不所持 | 編集不要 |
| Issue B deferredUtil 側 alias scope (本 phase は alias 廃止しない) | deferredUtil L173-185 の rename + L780-783 `#undef` は他 file (shadowUtil/reflectionProbeF/main shader) の name pollution 防止に必要、本 phase は **pointLight/spotLight 側の本体参照を直接 UBO member 名に rename** で対応 (alias 経路を pointLight/spotLight から外す) | deferredUtil **編集なし** |
| Issue C pbrterrainF cross-stage (V pair = `pbrterrainV.glsl`) | V は Phase 2c で `PerProgramUBO_PbrTerrainV` (set=2 binding=24) 既 UBO 化、`TerrainMix` struct 不参照 | 編集不要 |
| Issue C pbrterrainUtilF other-program 露呈 | `pbrterrainUtilF.glsl` は `addCommonShader` 経路で frag stage common として全 frag program に attach される可能性、ただし `struct TerrainMix` 自体は **TERRAIN_PBR_DETAIL define が有効な program 限定**で実 compile される (Vulkan path の preprocessor scope)。pbrterrainF 以外の program で同 struct を参照する path 確認 → 現状無し。ただし guard wrap は副作用ゼロで予防価値あり | 予防 guard wrap |
| Issue C pbrTerrainBakeF program (`gPbrTerrainBakeProgram`) | `pbrTerrainBakeV+pbrTerrainBakeF` attach、pbrterrainUtilF を attach しないため現状 redef 発生せず | 予防 guard wrap |
| Issue E spotLightF cross-stage (V pair = `pointLightV.glsl` for gDeferredSpotLightProgram / `multiPointLightV.glsl` for gDeferredMultiSpotLightProgram) | 両 V は `color` 不参照 | 編集不要 |
| Issue F spotLightF `vec3 center` cross-stage (V pair = `multiPointLightV.glsl`) | multiPointLightV.glsl 側で `center` を out attribute として送信していないか要確認 (host C++ setter が direct UBO 書き込みでなく attribute 経路の可能性) | **本 prep §2.4 で trace、host C++ 側 setter も確認** |
| Issue F !MULTI_SPOTLIGHT permutation 影響 | spotLightF !MULTI_SPOTLIGHT permutation (`gDeferredSpotLightProgram`) では UBO に `vec3 center` field 追加されても declared-but-unused (本体 L228 で `c = trans_center;` を参照、`c = center;` は MULTI_SPOTLIGHT only) | η-28-C type 3 範式適用、parse 通過確認のみ |

---

## §2 命名規約と source 編集詳細 (reference doc §4-B + §6 命名規約に整合)

### §2.1 Issue B: pointLightF.glsl 修正

**編集箇所**:

**(1) L66-78 自前 UBO 宣言を削除** (deferredUtil 側を source of truth に統一):

修正前 (L66-78):
```glsl
// light params
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-3: PerDrawUBO per-group 固有化
#ifndef PER_DRAW_UBO_LIGHT_PARAMS_DEFINED
#define PER_DRAW_UBO_LIGHT_PARAMS_DEFINED 1
layout(set=2, binding=0, std140) uniform PerDrawUBO_LightParams {
    vec3  color;
    float size;
};
#endif
#else
uniform vec3 color;
uniform float size;
#endif
```

修正後 (L66-78):
```glsl
// light params
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2d-α: PerDrawUBO_LightParams の
//   宣言は deferredUtil.glsl L173-179 に 1 元化 (η-20 範式の rename `spot_light_color`/
//   `spot_light_size` を source of truth とする)。同 guard `PER_DRAW_UBO_LIGHT_PARAMS_DEFINED`
//   で deferredUtil 先行 attach 時 self UBO 宣言は skip され、deferredUtil L780-783 末尾の
//   `#undef color`/`#undef size` で alias 切断後、本 pointLightF 本体は alias 経路を通さず
//   UBO member 名 (`spot_light_color`/`spot_light_size`) を直接参照する形に変更
//   (本 §2.1 (2)/(3) で本体 rename)。
#else
uniform vec3 color;   // light_color (GL path のみ、Vulkan path は deferredUtil PerDrawUBO_LightParams 経由)
uniform float size;   // light_size  (同上)
#endif
```

**(2) L195 / L199 `size` 参照を `spot_light_size` に rename**:

修正前 (L195):
```glsl
    if (lightDist >= size)
    {
        discard;
    }
    float dist = lightDist / size;
```

修正後 (L195):
```glsl
#ifdef LL_VULKAN_GLSL
    if (lightDist >= spot_light_size)
    {
        discard;
    }
    float dist = lightDist / spot_light_size;
#else
    if (lightDist >= size)
    {
        discard;
    }
    float dist = lightDist / size;
#endif
```

**(3) L216 / L237 / L251 `color` 参照を `spot_light_color` に rename**:

L216 修正前:
```glsl
        vec3 intensity = dist_atten * color * 3.25;
```

L216 修正後:
```glsl
#ifdef LL_VULKAN_GLSL
        vec3 intensity = dist_atten * spot_light_color * 3.25;
#else
        vec3 intensity = dist_atten * color * 3.25;
#endif
```

L237 修正前:
```glsl
        final_color = color.rgb*lit*diffuse;
```

L237 修正後:
```glsl
#ifdef LL_VULKAN_GLSL
        final_color = spot_light_color.rgb*lit*diffuse;
#else
        final_color = color.rgb*lit*diffuse;
#endif
```

L251 修正前:
```glsl
                final_color += lit*scol*color.rgb*spec.rgb;
```

L251 修正後:
```glsl
#ifdef LL_VULKAN_GLSL
                final_color += lit*scol*spot_light_color.rgb*spec.rgb;
#else
                final_color += lit*scol*color.rgb*spec.rgb;
#endif
```

**host C++ 整合**: 既存の host C++ side (PerDrawUBO_LightParams setter) は η-3 整備段階で UBO buffer 経由書き込みに対応済 (`color`/`size` の reserved enum 経由 setter が PerDrawUBO_LightParams.spot_light_color / spot_light_size に bind されている前提)。本 prep では host C++ 編集なし、GLSL 側 reference の rename のみ。`feedback_self_verify_before_handoff` で deploy 前 grep 確認 (`PerDrawUBO_LightParams` の setter 経路を pipeline.cpp で確認、本 prep §6-A)。

**cross-program 影響**: 同 PerDrawUBO_LightParams を参照する program 群 (gDeferredLightProgram = pointLight、gDeferredSpotLightProgram = spotLight、gDeferredMultiSpotLightProgram = spotLight MULTI 系) は **全て同 UBO 経由**で、本 phase の rename は pointLightF/spotLightF 側の参照書換のみ、UBO descriptor 自体は不変。multiPointLightF (gDeferredMultiLightProgram) は本 file 本体で `color`/`size` を参照しないため (§1.5 で確認済)、影響なし。

### §2.2 Issue C: pbrterrainF + pbrterrainUtilF + pbrTerrainBakeF 修正

**編集箇所**:

**(1) `class1/deferred/pbrterrainF.glsl` L47-51 を guard wrap**:

修正前 (L47-51):
```glsl
struct TerrainMix
{
    vec4 weight;
    int type;
};
```

修正後 (L47-51):
```glsl
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2d-α: struct TerrainMix は
//   pbrterrainF.glsl + pbrterrainUtilF.glsl + interface/pbrTerrainBakeF.glsl の 3 箇所
//   で同 member 名・同順序で宣言され、pbrterrainUtilF は `addCommonShader` 経路で
//   pbrterrainF と同一 program に attach されるため struct redefinition となる。
//   guard wrap で 1 度だけ宣言 (η-28-F 範式、η-19 M_PI guard wrap 範式の struct 版派生)。
#ifndef TERRAIN_MIX_DEFINED
#define TERRAIN_MIX_DEFINED 1
struct TerrainMix
{
    vec4 weight;
    int type;
};
#endif
```

**(2) `class1/deferred/pbrterrainUtilF.glsl` L183-187 を guard wrap** (同 guard 名 `TERRAIN_MIX_DEFINED`):

修正前 (L183-187):
```glsl
struct TerrainMix
{
    vec4 weight;
    int type;
};
```

修正後 (L183-187):
```glsl
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2d-α: 同 guard wrap (pbrterrainF.glsl L47-51 と対)
#ifndef TERRAIN_MIX_DEFINED
#define TERRAIN_MIX_DEFINED 1
struct TerrainMix
{
    vec4 weight;
    int type;
};
#endif
```

**(3) `class1/interface/pbrTerrainBakeF.glsl` L34 を guard wrap** (予防、program 経路独立で現状 redef なし):

修正前 (L34):
```glsl
struct TerrainMix
```

修正後 (L34 周辺):
```glsl
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2d-α: 予防 guard wrap (pbrTerrainBake
//   program は pbrterrainUtilF を attach せず現状 redef は発生しないが、将来 common shader
//   経路で同 utility を bake program に加えた場合の衝突予防、η-28-F 範式適用)
#ifndef TERRAIN_MIX_DEFINED
#define TERRAIN_MIX_DEFINED 1
struct TerrainMix
{
    vec4 weight;
    int type;
};
#endif
```

注: 3 file の `TerrainMix` struct は **member 名・型・順序が完全一致** (Phase 2d-α 起草前 source-tree grep で確認済)。divergence 発見時は本 phase で同型化を先行 (member 名揃え)。

### §2.3 Issue E: spotLightF.glsl `color` rename (Issue B 同型修正)

**編集箇所**:

**(1) L152-158 自前 UBO 宣言を削除** (Issue B §2.1 (1) と同型):

修正前 (L152-158):
```glsl
#ifdef LL_VULKAN_GLSL
#ifndef PER_DRAW_UBO_LIGHT_PARAMS_DEFINED
#define PER_DRAW_UBO_LIGHT_PARAMS_DEFINED 1
layout(set=2, binding=0, std140) uniform PerDrawUBO_LightParams {
    vec3  color;
    float size;
};
#endif
#else
uniform vec3 color;
uniform float size;
#endif
```

修正後 (L152-158):
```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2d-α: PerDrawUBO_LightParams 宣言は
//   deferredUtil.glsl に 1 元化 (Issue E 修正、pointLightF と同型範式適用)。
//   本体 L384 の `color.rgb` 参照は `spot_light_color.rgb` に rename (本 §2.3 (2))。
#else
uniform vec3 color;
uniform float size;
#endif
```

**(2) L384 `color.rgb` を `spot_light_color.rgb` に rename**:

修正前 (L384):
```glsl
                        final_color += color.rgb * texture2DLodSpecular(stc.xy, (1 - spec.a) * (proj_lod * 0.6)).rgb * shadow * envIntensity;
```

修正後 (L384):
```glsl
#ifdef LL_VULKAN_GLSL
                        final_color += spot_light_color.rgb * texture2DLodSpecular(stc.xy, (1 - spec.a) * (proj_lod * 0.6)).rgb * shadow * envIntensity;
#else
                        final_color += color.rgb * texture2DLodSpecular(stc.xy, (1 - spec.a) * (proj_lod * 0.6)).rgb * shadow * envIntensity;
#endif
```

**spotLightF の `size` 参照**: 本体 main() 内に `size` を直接参照する箇所なし (Phase 2d-α prep grep で確認済、`if (lightDist >= size)` 系の参照は spotLightF にはなく、deferredUtil の `clipProjectedLightVars` 等内部関数経由 = deferredUtil 内 alias 有効範囲で compile 完結)。よって本 phase で spotLightF 本体の `size` rename は不要。

### §2.4 Issue F: spotLightF.glsl MULTI_SPOTLIGHT `vec3 center` を既存 UBO 拡張で吸収

**編集箇所**:

**(1) L74-88 PerProgramUBO_SpotLightF に `vec3 center` field 追加**:

修正前 (L74-88):
```glsl
#ifndef PER_PROGRAM_UBO_SPOT_LIGHT_F_DEFINED
#define PER_PROGRAM_UBO_SPOT_LIGHT_F_DEFINED 1
layout(set=2, binding=10, std140) uniform PerProgramUBO_SpotLightF {
    // chunk 0 (4 scalar)
    float proj_near;
    float proj_ambient_lod;
    float near_clip;
    float far_clip;
    // chunk 1 (vec3 + float)
    vec3  proj_origin;
    float sun_wash;
    // chunk 2 (4 scalar)
    int   proj_shadow_idx;
    float shadow_fade;
    float falloff;
    float global_light_strength;
};
#endif
```

修正後 (L74-88、`vec3 center` を chunk 3 として末尾追加):
```glsl
#ifndef PER_PROGRAM_UBO_SPOT_LIGHT_F_DEFINED
#define PER_PROGRAM_UBO_SPOT_LIGHT_F_DEFINED 1
layout(set=2, binding=10, std140) uniform PerProgramUBO_SpotLightF {
    // chunk 0 (4 scalar)
    float proj_near;
    float proj_ambient_lod;
    float near_clip;
    float far_clip;
    // chunk 1 (vec3 + float)
    vec3  proj_origin;
    float sun_wash;
    // chunk 2 (4 scalar)
    int   proj_shadow_idx;
    float shadow_fade;
    float falloff;
    float global_light_strength;
    // chunk 3 (vec3 + float) - r41 sub-step ... η-28 Phase 2d-α (Issue F):
    // MULTI_SPOTLIGHT permutation で参照される `vec3 center` を本 UBO に吸収。
    // !MULTI_SPOTLIGHT permutation では declared-but-unused (η-28-C type 3 範式)。
    vec3  center;
    float _pad_center;
};
#endif
```

**alignment 確認** (std140):
- chunk 0: offset 0-15 (4 floats)
- chunk 1: offset 16-31 (vec3 proj_origin + float sun_wash)
- chunk 2: offset 32-47 (4 scalars: int proj_shadow_idx + 3 floats)
- chunk 3: offset 48-63 (vec3 center + float _pad_center)
- total: 64 bytes ✓ (16-byte alignment)

**(2) L139-140 `#if defined(MULTI_SPOTLIGHT)` 内 plain uniform 削除 (Vulkan path UBO 経由へ統合)**:

修正前 (L139-149):
```glsl
// Light params
#if defined(MULTI_SPOTLIGHT)
uniform vec3 center;
#else
#ifdef LL_VULKAN_GLSL
layout(location=60) in vec3 trans_center;
#else
in vec3 trans_center;
#endif
#endif
```

修正後 (L139-149):
```glsl
// Light params
#if defined(MULTI_SPOTLIGHT)
#ifndef LL_VULKAN_GLSL
uniform vec3 center;   // GL path のみ (Vulkan path は PerProgramUBO_SpotLightF 経由、L74-88)
#endif
#else
#ifdef LL_VULKAN_GLSL
layout(location=60) in vec3 trans_center;
#else
in vec3 trans_center;
#endif
#endif
```

**(3) L226 `c = center;` の Vulkan path 確認**:

L226 `c = center;` は MULTI_SPOTLIGHT 時のみ評価される、Vulkan path では `center` を PerProgramUBO_SpotLightF.center 経由で参照する形に自動解決される (UBO member 名 `center` が global scope に anonymous で expose、η-28-C type 1 範式同型)。本体側 source 修正不要。

**host C++ 整合 (要 deploy 前 grep)**:
- `center` は host C++ で reserved uniform `CENTER` (要 enum 名 grep)、setter は `pipeline.cpp` の MULTI_SPOTLIGHT pass で書込み。本 phase で UBO 化したため、setter 側は **PerProgramUBO_SpotLightF buffer 経由書込み** に変更必要 (既存 PerProgramUBO_SpotLightF 経路に従う)。η-27 で UBO 化された他 field (proj_near 等) と同経路で setter 化、本 prep §6-A で確認。

### §2.5 本 phase 編集対象 file 集約

| # | file | 編集行 (要 deploy 前再 read 確認) | 修正カテゴリ |
|---|---|---|---|
| 1 | `class3/deferred/pointLightF.glsl` | L66-78 (UBO 削除) + L195/L199 (`size` rename) + L216/L237/L251 (`color` rename) | Issue B、deferredUtil alias scope refinement |
| 2 | `class3/deferred/spotLightF.glsl` | L74-88 (UBO 拡張 `vec3 center`) + L139-140 (plain uniform 削除) + L152-158 (UBO 削除) + L384 (`color` rename) | Issue E + Issue F |
| 3 | `class1/deferred/pbrterrainF.glsl` | L47-51 (struct guard wrap) | Issue C |
| 4 | `class1/deferred/pbrterrainUtilF.glsl` | L183-187 (struct guard wrap、同 guard 名) | Issue C |
| 5 | `class1/interface/pbrTerrainBakeF.glsl` | L34 周辺 (予防 struct guard wrap、同 guard 名) | Issue C 予防 |

合計 5 file、binding 連番未消費 (Issue F は既存 binding 10 拡張、新 binding なし)。

---

## §3 期待 ERROR Δ + cascade reveal 観察計画

### Phase 2c 末 → Phase 2d-α 末 期待値 (cold launch log 実測ベース)

| 観測項目 | Phase 2c 末 | Phase 2d-α 末 期待 | Δ |
|---|---|---|---|
| ERROR 行数 | 24 | **~8-10** | **-14 〜 -16** (4 root × cascade ratio 3-4) |
| parse failure event 数 | 7 | **2** (残 2d-β scope: pbralpha V + skin F) | **-5** |
| link failed | 0 維持 | 0 維持 | ZERO 継続 (12 sub-bundle 連続見込み) |
| Issue B PointLight Shader SPIR-V 生成 | parse fail | 成功 | +1 |
| Issue C ×2 PBR Terrain F (heightmap + paintmap) SPIR-V 生成 | parse fail ×2 | 成功 ×2 (V/F 両 stage 全 PASS) | +2 |
| Issue E SpotLight Shader SPIR-V 生成 | parse fail | 成功 | +1 |
| Issue F MultiSpotLight Shader SPIR-V 生成 | parse fail | 成功 | +1 |
| clean shutdown | 維持 | 維持 | - |

**Δ レンジ理由**: 4 root の root + cascade 合算で Phase 2c §3 表記載の **24 行から 16 行縮小** が下限 (cascade 0)、cascade reveal あれば差し引かれる。Phase 2c は cascade reveal +3 (Issue B + C×2) で variance あり、本 phase も struct/alias scope 修正による隠れ cascade reveal 0-2 を見込む。

### Phase 2d-α 直接 ERROR 消滅対象 (Phase 2c 末 log 行 → file 対応)

| Phase 2c 末 log L | program | stage | 該当 ERROR (root + cascade) | 該当修正 (本 prep §2.x) |
|---|---|---|---|---|
| L712-714 | Deferred PBR Terrain Shader heightmap | F | TerrainMix redefinition + cascade 2 行 | §2.2 (1)+(2) |
| L722-724 | Deferred PBR Terrain Shader paintmap | F | TerrainMix redefinition + cascade 2 行 | §2.2 (1)+(2) (同編集で 2 permutation 同時解消) |
| L751-753 | Deferred Light Shader | F | size undeclared + cascade 2 行 | §2.1 (1)+(2)+(3) |
| L840-843 | Deferred SpotLight Shader | F | color undeclared + cascade 3 行 | §2.3 (1)+(2) |
| L851-853 | Deferred MultiSpotLight Shader | F | non-opaque + cascade 2 行 | §2.4 (1)+(2) |

合計 root 4 件 (Issue C×2 を 1 修正で 2 件解消) + cascade 11 件 = 15 ERROR 行解消見込み、上振れ 16 + cascade reveal 0-2 (上振れ -16、下振れ -14)。

### Phase 2d-β 残予測 (Phase 2d-α 末で再評価、本 prep では scope 固定しない)

Phase 2d-α 末で残る parse fail event 2 件:
- L687-690 Skinned Deferred PBR Alpha Shader F (non-opaque uniforms outside a block @ 0:3692) = 2d-β scope
- L696-699 Deferred PBR Alpha Shader V (modelview_projection_matrix undeclared @ 0:1252) = 2d-β scope

Phase 2d-α verify 完了後に 2d-β prep 起草、scope は本 prep §0.3 表起点 (pbralpha class2 F + pbralphaV !HAS_SKIN path の guard 漏れ修正 + UBO 化)。

---

## §4 編集対象 file (literal 5 file、binding 連番未消費)

### 編集

1. `indra/newview/app_settings/shaders/class3/deferred/pointLightF.glsl`
   - L66-78 自前 UBO 宣言削除 + L195/L199 `size` → `spot_light_size` rename + L216/L237/L251 `color` → `spot_light_color` rename (本 prep §2.1)
2. `indra/newview/app_settings/shaders/class3/deferred/spotLightF.glsl`
   - L74-88 PerProgramUBO_SpotLightF に `vec3 center` + `float _pad_center` 追加 + L139-140 `uniform vec3 center;` を Vulkan path 削除 + L152-158 自前 UBO 削除 + L384 `color` → `spot_light_color` rename (本 prep §2.3 + §2.4)
3. `indra/newview/app_settings/shaders/class1/deferred/pbrterrainF.glsl`
   - L47-51 `struct TerrainMix` 宣言を `TERRAIN_MIX_DEFINED` guard wrap (本 prep §2.2 (1))
4. `indra/newview/app_settings/shaders/class1/deferred/pbrterrainUtilF.glsl`
   - L183-187 `struct TerrainMix` 宣言を同 guard wrap (本 prep §2.2 (2))
5. `indra/newview/app_settings/shaders/class1/interface/pbrTerrainBakeF.glsl`
   - L34 周辺 `struct TerrainMix` 宣言を同 guard wrap (予防、本 prep §2.2 (3))

### 非編集 (cross-stage / cross-program check で確認済)

- `class1/deferred/deferredUtil.glsl` (η-20 整備済、alias `#define color spot_light_color`/`#define size spot_light_size` + 末尾 `#undef` は他 file pollution 防止に必要、本 phase は pointLight/spotLight 側の本体 rename で対応)
- `class3/deferred/pointLightV.glsl` (η-23 既 UBO 化、`color`/`size` 不所持)
- `class3/deferred/multiPointLightV.glsl` (multiPointLightF / spotLight MULTI 系の V pair、`center` を out attribute として送信していないか要確認 = §1.5 cross-stage check で予防確認)
- `class3/deferred/multiPointLightF.glsl` (η-23 で `PerDrawUBO_MultiLight` 既集約、本体 `color`/`size` 不参照、Phase 2c で既 PASS)
- `class2/deferred/sunLightF.glsl` (本 phase scope 外)
- `class3/deferred/pbralphaF.glsl` および `class2/deferred/pbralphaF.glsl` (2d-β scope)
- `class1/deferred/pbralphaV.glsl` (2d-β scope)

---

## §5 reference-shader-location-map.md §6-A 更新計画 (本 commit 同梱)

### §5.1 §6-A 表更新 (binding 連番未消費だが note を 1 行追加)

Phase 2c で binding 21-25 まで埋まり、本 phase は **binding 26+ を消費しない**。ただし binding 10 (既存 PerProgramUBO_SpotLightF、η-27 起源) に `vec3 center + float _pad_center` を追加するため、表の note 列を更新:

修正前 (binding 10 行):
```markdown
| 2 | 10 | PerProgramUBO_SpotLightF | F | η-27 Phase 1d + 1e-A |
```

修正後:
```markdown
| 2 | 10 | PerProgramUBO_SpotLightF | F | η-27 Phase 1d + 1e-A (η-28 Phase 2d-α で center field 追加、Issue F 吸収) |
```

「(空き、η-28 Phase 2d+ 連番継続)」行 (現 binding 26+) は不変。

### §5.2 §6 命名規約セクションに新範式 2 件追記提案 (本 phase complete handoff で範式 doc 反映)

**η-28-E (新規範式、本 phase 確立)**: deferredUtil alias scope refinement
- 既存 alias 機構 (`#define X Y` + 末尾 `#undef X`) を維持しつつ、後段 attach file が本体で alias 経路を参照する場合、**後段側で alias 名でなく UBO member 名を直接参照**する書換で対応
- 本 phase で実証: pointLightF L195/L199 (`size` → `spot_light_size`)、L216/L237/L251 (`color` → `spot_light_color`)、spotLightF L384 (`color` → `spot_light_color`)
- 適用条件: deferredUtil 等の utility file 末尾で `#undef` した alias を後段 file が参照していることが cold launch log で露呈した場合
- 適用形態: utility 側を編集せず後段 file 本体の参照名のみ rename (`feedback_no_scope_shrink` 違反でないように、両方廃止せず utility-only / consumer-only の二分構造を維持)

**η-28-F (新規範式、本 phase 確立)**: struct redefinition guard wrap
- 同一 program 内に複数 file が同名 struct 宣言を持つ場合、**全宣言地点を `#ifndef <STRUCT>_DEFINED` / `#define <STRUCT>_DEFINED 1` / `#endif` で wrap**
- η-19 M_PI_DEFINED guard wrap 範式の struct 派生形
- 本 phase で実証: pbrterrainF + pbrterrainUtilF + pbrTerrainBakeF (予防) の `TerrainMix` struct を `TERRAIN_MIX_DEFINED` guard wrap
- 適用条件: `addCommonShader` 経路で同一 program に多重 attach される utility file と main file が同名 struct を宣言している場合 (`#define` macro 整合は前提、divergence あれば本 phase で同型化を先行)

---

## §6 Phase 2d-α 着手前 self-verify (Claude 側、AYA "OK" 後の deploy 前)

### §6-A reserved uniform 名 + UBO setter 整合 (host C++ vs GLSL)

deploy 前に Claude が再確認 (本 prep §2.x の host 整合記述の裏取り):

| GLSL 名 | LLShaderMgr enum (要 grep) | reserved push_back 場所 (要 grep) | host C++ setter 場所 (要 grep) | Phase 2d-α deploy 前確認 |
|---|---|---|---|---|
| `color` (PerDrawUBO_LightParams.spot_light_color) | LIGHT_COLOR 系 (η-3 起源、要 grep) | (要再 grep) | `pipeline.cpp` light volume pass の per-draw color setter (要再 grep、η-3 整備時に UBO buffer 経由化済の前提) | **deploy 前 grep** |
| `size` (PerDrawUBO_LightParams.spot_light_size) | LIGHT_SIZE 系 (η-3 起源、要 grep) | (要再 grep) | `pipeline.cpp` light volume pass の per-draw size setter (同上) | **deploy 前 grep** |
| `center` (PerProgramUBO_SpotLightF.center、Issue F 新規追加) | CENTER 系 (要 enum 名 grep) | (要 push_back 場所 grep) | `pipeline.cpp` MULTI_SPOTLIGHT pass の per-draw center setter (η-27 PerProgramUBO_SpotLightF 経由化済の他 field と同経路、本 phase で center も同経路化が必要) | **deploy 前 grep + setter 経路確認必須** |

**特記 (center)**: `vec3 center` を `PerProgramUBO_SpotLightF` に field 追加するため、**host C++ 側 setter も UBO buffer 経由書込みに変更**が必要。η-27 で proj_near 等が UBO 経由化された経路と同じパターンで対応:
- 既存 `gDeferredMultiSpotLightProgram.uniform3fv(CENTER, ...)` 経路を **PerProgramUBO_SpotLightF buffer 内 center offset (48-59 bytes) への書込み** に変更
- 該当箇所は llviewershadermgr.cpp / pipeline.cpp の MULTI_SPOTLIGHT pass、deploy 前 grep で特定

`feedback_self_verify_before_handoff` 適用、AYA "OK" 後 commit 直前で Claude が 10-15 分で消化 (本 prep §6-A の 3 件は host C++ 整合の核心、確認漏れは Issue A 同型 regression に直結)。

### §6-B 5 file edit 後の cross-check

deploy 前に Claude が再 read で以下確認:
- pointLightF L66-78 削除 + L195/L199/L216/L237/L251 rename: 4-layer nesting 不要 (UBO 宣言なし、本体 rename のみ)、`#ifdef LL_VULKAN_GLSL` / `#else` 分岐は本体行に挟む形 ✓
- spotLightF L74-88 UBO 拡張: `vec3 center` + `float _pad_center` 末尾追加 + std140 alignment 64 bytes 維持 ✓
- spotLightF L139-140 + L152-158 削除 + L384 rename: 各 `#ifdef LL_VULKAN_GLSL` / `#else` 分岐維持 ✓
- pbrterrainF + pbrterrainUtilF + pbrTerrainBakeF: 同 guard 名 `TERRAIN_MIX_DEFINED` で 3 file 同期 ✓、struct member 名・型・順序の divergence チェック (本 prep §2.2 注意書き) ✓
- 同 UBO 名 grep で唯一宣言:
  - `PerDrawUBO_LightParams`: deferredUtil.glsl L173-179 のみ (pointLightF/spotLightF 側削除後)
  - `PerProgramUBO_SpotLightF`: spotLightF.glsl L74-88 のみ (本 phase で center 追加後)
  - `struct TerrainMix`: 3 file で同型 + 同 guard

### §6-C cold launch verify checklist (reference doc §8 cookbook の Phase 2d-α 起点)

```bash
# 期待値: ERROR 24 → ~8-10, parse failure 7 → 2, link 0, clean shutdown
grep -cE "^ERROR: 0:" ~/.ayastorm_x64/logs/AYAstorm.log    # 期待 8-10
grep -c "glslang parse failed" ~/.ayastorm_x64/logs/AYAstorm.log  # 期待 2
grep -c "link failed\|link error" ~/.ayastorm_x64/logs/AYAstorm.log  # 期待 0
grep "Shutting down" ~/.ayastorm_x64/logs/AYAstorm.log   # 存在確認

# 5 target program 全 SPIR-V 生成成功 grep
grep -E "generatePerProgramSPIRV.*for program (Deferred Light Shader|Deferred SpotLight Shader|Deferred MultiSpotLight Shader|Deferred PBR Terrain Shader)" ~/.ayastorm_x64/logs/AYAstorm.log

# PBR Terrain F 2 permutation 確認 (heightmap + paintmap 両方 success)
grep -E "Deferred PBR Terrain Shader.*(HEIGHTMAP|PAINTMAP).*PASS|generatePerProgramSPIRV.*Deferred PBR Terrain" ~/.ayastorm_x64/logs/AYAstorm.log

# 残 parse fail 2 件確認 (2d-β scope)
grep -E "(Skinned )?Deferred PBR Alpha Shader" ~/.ayastorm_x64/logs/AYAstorm.log
```

### §6-D cascade reveal 観察 (Phase 2a +2 / 2b 0 / 2c +3、本 phase 推定 0-2)

Phase 2d-α は **struct/alias scope 修正のみ** で UBO 化 0、新 binding 追加 0。cascade reveal の見込みは:
- pointLightF / spotLightF の本体 rename で **他 helper 関数からの間接参照** (deferredUtil.glsl の helper が pointLight/spotLight UBO 値を関数引数で取得していないか) を要確認 (本 prep §6-B grep で潰す)
- pbrterrainF / pbrterrainUtilF の guard wrap で **他 file が `TERRAIN_MIX_DEFINED` を別意味で使っていないか** grep (副作用ゼロのはずだが念のため)

Δ event = -5 (cascade 0) → 期待通り Phase 2d-β scope は 2 root のみ
Δ event = -6 / -7 (cascade reveal -1 〜 -2、他隠れ root が表面化) → reference doc §7 cascade chain 同定 protocol で新規 chain を trace、2d-β scope 拡張

### §6-E host C++ setter 経路確認 (Issue F center UBO 化に伴う必須確認)

`PerProgramUBO_SpotLightF.center` 新 field の host C++ setter 経路:
1. `pipeline.cpp` の `gDeferredMultiSpotLightProgram` 関連箇所で `center` または `CENTER` reserved uniform を grep
2. 既存の同 UBO 内 field (proj_near, proj_ambient_lod 等) が UBO buffer 経由でどう書き込まれているか確認 (η-27 起源、推定: LLGLSLShader::uniformXXX 系から UBO buffer 経由 dispatch)
3. center 用に同経路を整備、または既存経路に center を追加

deploy 前に上記 3 段 trace を Claude が消化、setter 不整合があると Vulkan path で center 値が garbage となり描画 regression。AYA cold launch で MULTI_SPOTLIGHT pass の動作確認 (light 描画が破綻していないか) を含める。

---

## §7 範式継承 + 本 phase 適用範式

### 継承 (Phase 2c 以前から)

- `feedback_one_step_at_a_time` (本 prep は 2d-α scope 単独、2d-β は別 prep で次々 session)
- `feedback_no_scope_shrink` (4 root 全消化 = literal scope、Issue F の既存 UBO 拡張は「UBO 化作業」のように見えるが新 binding 消費なし = struct/alias scope category)
- `feedback_doubt_self_first` (本 prep §1 で Phase 2c complete §3 表記載の各 root を source-tree grep で再確認、特に Issue C の attach 経路 (addCommonShader L966) と Issue F の host C++ setter 経路を新規 trace)
- `feedback_render_full_trace_first` (4 root を deferredUtil/pointLightF/spotLightF/pbrterrainF/pbrterrainUtilF の 5 file + llviewershadermgr.cpp attach map で完全 trace)
- `feedback_no_auto_commit` (AYA "OK" 明示後に feat commit)
- `feedback_self_verify_before_handoff` (本 prep §6 self-verify で AYA cold launch 浪費を防ぐ、特に §6-A の host C++ setter 経路は Issue A 同型 regression 予防の核心)
- `feedback_proactive_handoff` (本 prep doc 自体)
- `feedback_falsification_as_progress` (Phase 2c で Issue A regression + Issue B/C cascade reveal を honest に記録した範式拡張を本 prep の trace 強化に継承)
- **η-28-A** (dump marker 信用せず source tree grep) — 本 prep §1 全 root の root cause trace で適用
- **η-28-B** (既存 transformed dump で再 cold launch 不要判定) — Phase 2c 末 transformed dump で current state 確認、本 prep 起草前に再 cold launch せず
- **η-28-C type 1** (cross-stage same UBO 共有、Phase 2a 由来 + Phase 2c Issue A 拡張) — 本 prep §2.4 で center UBO 経由 cross-stage 共有として再適用
- **η-28-C type 2** (cross-variant cvar-selected file 共有、Phase 2b 由来) — 本 phase 適用ケースなし
- **η-28-C type 3** (cross-permutation preprocessor 共有、Phase 2c 由来) — 本 prep §2.4 で MULTI_SPOTLIGHT / !MULTI_SPOTLIGHT 間で `center` field declared-but-unused 容認として適用
- **η-28-D** (prep doc literal を 3 段 trace で訂正、Phase 2c 由来) — 本 prep §1 で Phase 2c complete §3 表記載の root file/line を source-tree grep + attach map で再確認

### 本 phase 提案範式 (新規 2 件)

#### **η-28-E** (新規範式): deferredUtil alias scope refinement

詳細は §5.2 (range 記述) 参照。本 prep §2.1 + §2.3 で実証。

**範式定式化**:
> utility file (例: deferredUtil.glsl) で `#define X Y` alias を提供しつつ file 末尾 `#undef X` で alias scope を制限している場合、後段 attach file が本体で alias 名 `X` を参照すると `#undef` 後の領域では symbol 不在で undeclared error。**後段 file 本体側で alias 名でなく UBO member 名 `Y` を直接参照** (Vulkan path 限定の rename) することで対応。utility 側の alias 機構は維持 (他後段 file の name pollution 防止のため)。
>
> **適用条件**: utility file の末尾 `#undef` 範式が cold launch log で `<alias_name> undeclared identifier` を引き起こした場合
> **適用形態**: 後段 file 本体の参照名のみ書換、utility 編集なし
> **副作用**: 後段 file の GL path (`#else`) では alias 名のまま (二重 maintenance 増、GL path 廃止予定の r41 では一時的)

#### **η-28-F** (新規範式): struct redefinition guard wrap

詳細は §5.2 参照。本 prep §2.2 で実証。

**範式定式化**:
> 同一 program 内に多重 attach される utility file と main file が同名 struct を宣言している場合、**全宣言地点を `#ifndef <STRUCT>_DEFINED` / `#define <STRUCT>_DEFINED 1` guard で wrap**。η-19 M_PI guard wrap (const 値) の struct 派生形。
>
> **適用条件**: `addCommonShader` 等で同 program に多重 attach される file 群が同名 struct を宣言、glslang strict mode で `'struct' : '<name>' redefinition` を引き起こした場合
> **適用形態**: 全宣言地点に同 guard 名で wrap、member 名・型・順序の divergence あれば本 phase で同型化を先行
> **予防適用**: 別 program (本 phase 例: pbrTerrainBakeF) で同 struct 宣言があり現状 redef 発生しないが将来 utility を多重 attach する変更で衝突する可能性がある場合、予防 wrap

---

## §8 落穂拾い + Phase 2d-α 完了 checklist + Phase 2d-β 引き継ぎ事項

### 本 phase 完了 checklist

- [ ] AYA prep doc レビュー + "OK" 明示
- [ ] §6-A host C++ setter 経路 deploy 前 grep 消化 (Claude 10-15 分、特に `center` setter UBO 経由化が核心)
- [ ] §6-B 5 file edit 後 cross-check (Claude re-read)
- [ ] §4 の 5 file 修正 feat commit
- [ ] reference-shader-location-map.md §6-A binding 10 note 更新 + §6 命名規約 η-28-E / η-28-F 追記 (feat commit 同梱 or 直後 docs commit)
- [ ] deploy + shader cache clear (~/.ayastorm_x64/cache/shader_cache/ 全 clear)
- [ ] AYA cold launch + log 採取 (`~/.ayastorm_x64/logs/AYAstorm.log`)
- [ ] Claude self-verify: ERROR ~8-10 / parse failure 2 / link 0 / 5 program (PointLight + SpotLight + MultiSpotLight + PbrTerrain F heightmap/paintmap) SPIR-V 全成功 / clean shutdown / MULTI_SPOTLIGHT pass の center 値が garbage でないこと (描画 regression なし) を AYA 確認 (本 prep §6-C cookbook + §6-E setter 経路確認)
- [ ] Phase 2d-α complete handoff 起草 (§4 Phase 2d-β 表更新 + 残 2 root を 2d-β scope として明示、η-28-E / η-28-F 範式昇格を doc 本体反映)
- [ ] **次々 session**: Phase 2d-α complete handoff の §4 Phase 2d-β 表起点に Phase 2d-β prep 起草 (pbralpha class2 F + pbralphaV !HAS_SKIN path、binding 26+ 消費見込み)

### 本 phase 着手前の追加 trace (Claude 側、本 prep §6-A / §6-E 消化前に AYA "OK" 待ちで実施)

- `class2/deferred/pbralphaF.glsl` の L3692 付近 root cause file 内 trace (本 phase 直接対象ではないが、2d-β prep 起草時の助走として既に位置確認)
- `class3/deferred/multiPointLightV.glsl` の `center` out attribute 存在確認 (Issue F の cross-stage check、§1.5 で予防的に挙げた項目、本 prep §6-A deploy 前 grep で消化)
- `pipeline.cpp` の `gDeferredMultiSpotLightProgram` 関連 center setter 経路 (§6-E、本 prep §6-A の必須項目)

### 次々 session への引き継ぎ事項 (Phase 2d-β prep 起草起点)

1. **Phase 2d-β scope (本 prep §0.3 表)**:
   - **2d-β-G** (Skinned Deferred PBR Alpha Shader F、L687-690): `class2/deferred/pbralphaF.glsl` HAS_SKIN path で L3692 付近の non-opaque uniforms outside a block。新 `PerProgramUBO_PbrAlphaF` 系 UBO 化 (binding 26 推定)、host C++ setter 経路整備
   - **2d-β-H** (Deferred PBR Alpha Shader V、L696-699): `class1/deferred/pbralphaV.glsl` !HAS_SKIN path で `modelview_projection_matrix` undeclared = FrameViewProj attach 漏れ (L54-58 が `#ifndef LL_VULKAN_GLSL` で wrap されているが Vulkan path で attach されていない)。FrameViewProj guard wrap 追加 (η-1 範式継承)、binding 連番未消費の可能性も (UBO 不要なら 0 消費、新 UBO 必要なら 27 消費)
2. **2d-β feat 適用前の追加 trace**:
   - class2/pbralphaF.glsl 全 plain uniform リスト (L3692 だけでなく他にも guard 漏れがあれば一括対応)
   - pbralphaV.glsl !HAS_SKIN path で modelview_projection_matrix 以外の必要 uniform (host C++ setter 経路と対応確認)
   - HUD path (`#ifdef IS_HUD`) の派生 file があれば trace
3. **2d-β verify 観点**: Phase 2d-α 末 ERROR ~8-10 / parse fail 2 → 2d-β 末 ERROR 0 / parse fail 0 達成、SPIR-V 全 program PASS、link 0 維持
4. **Phase 2d 全体完了後**: Phase 2e は **Phase 2d-α + 2d-β verify 末で残 ERROR が真に 0 なら不要**。残 ERROR 出現時のみ 2e prep 起草、Phase 2d 末で評価。

### 落穂拾い

- **`feedback_no_scope_shrink` の境界線**: 本 prep の sub-phase 分離 (2d-α + 2d-β) は **AYA 事前合意済の分離** (Phase 2c complete §4.2 提案を AYA が前提として本 prep 起草指示) で、scope shrink ではない。AYA が「全 7 root 1 phase で」と指示した場合は本 prep を破棄し 1 phase に統合する形で対応。
- **deferredUtil 編集を避けた理由**: utility 側の alias 機構 (L173-185 + L780-783) を編集すると **shadowUtil / reflectionProbeF / main shader** の他後段 attach file で name pollution が再発する可能性。η-20 範式の rename 動機 (V stage MaterialUBO.color と衝突 36 件) を維持しつつ pointLight/spotLight の本体 rename のみで対応 = 影響範囲最小。
- **host C++ side 整合は本 phase 範式拡張で重要**: Issue F の `center` UBO 化に伴う setter 経路変更は **Issue A regression と同型リスク** (Phase 2c で waterF lightDir cross-stage import 漏れ)。`feedback_self_verify_before_handoff` の核心、本 prep §6-A / §6-E を AYA cold launch 前に消化必須。

---

## §9 reference link

- 前 handoff (η-28 Phase 2c complete): `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2c-complete.md` (commit `e5d3de6ecc` で起草、Phase 2c 末 ERROR 24 / parse fail 7 / link 0、§4 表 7 root + §10 引き継ぎ事項が本 prep 起点)
- Phase 2c feat commit (literal scope): `c08080ded2` (5 file UBO 化、binding 21-25)
- Phase 2c feat commit (Issue A fix): `5936ee7f2d` (waterF lightDir cross-stage import、η-28-C type 1 拡張範式の起源)
- location/UBO map: `docs/specs/ayastorm-r41-gl-removal/reference-shader-location-map.md` (§6-A binding 25 まで埋まり、binding 10 (PerProgramUBO_SpotLightF) は η-27 で起源、本 phase で center field 拡張予定)
- Phase 2c 末 cold launch log: `~/.ayastorm_x64/logs/AYAstorm.log` (ERROR 24、parse fail 7、link 0、本 prep §1 / §3 起点)
- Phase 2c 末 transformed dump 場所: `~/.ayastorm_x64/cache/shader_cache/transformed/` (η-28-B 範式で本 prep §1 root cause trace に活用済、特に pointLightF F / spotLightF F の dump で `size`/`color` 参照 line を確認可能)
- 関連 llviewershadermgr.cpp 行:
  - L1729-1730 (gDeferredLightProgram = pointLightV + pointLightF)
  - L1753-1754 (gDeferredMultiLightProgram = multiPointLightV + multiPointLightF)
  - L1775-1776 (gDeferredSpotLightProgram = pointLightV + spotLightF)
  - L1796-1797 (gDeferredMultiSpotLightProgram = multiPointLightV + spotLightF + MULTI_SPOTLIGHT define)
  - L966 (addCommonShader `deferred/pbrterrainUtilF.glsl` = 全 frag program 共有 attach)
  - L1654-1655 (gDeferredPBRTerrainProgram = pbrterrainV + pbrterrainF)
  - L3934-3935 (gPbrTerrainBakeProgram = pbrTerrainBakeV + pbrTerrainBakeF)
- 関連 deferredUtil.glsl 行:
  - L173-185 (PerDrawUBO_LightParams { vec3 spot_light_color; float spot_light_size; } 宣言 + alias `#define color spot_light_color` / `#define size spot_light_size`)
  - L780-783 (file 末尾 `#undef color` / `#undef size`、η-20 範式の name pollution 防止)
- 関連 reserved uniform push_back (host C++ 整合、deploy 前再 grep 対象):
  - LIGHT_COLOR / LIGHT_SIZE (η-3 起源、要再 grep)
  - CENTER (Issue F 用、要 enum 名 + push_back 行 grep)

---

**本 prep は η-28 Phase 2d-α scope の source of truth**。AYA レビュー + "OK" 明示後、Claude が §6-A / §6-E deploy 前 grep を消化 → 5 file 修正 feat commit + reference doc 更新 commit → AYA push + deploy + cold launch verify → Phase 2d-α complete handoff 起草 (本 prep §8 checklist 順)。
