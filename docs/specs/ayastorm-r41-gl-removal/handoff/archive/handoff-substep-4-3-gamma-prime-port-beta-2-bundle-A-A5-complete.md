# r41 sub-step 4.3-γ'-port-β-2-bundle-A-A5 完遂 → A6 着手境界 handoff (2026-06-01)

**parent commit**: `eddcc7ac40` (A5 patch、本 handoff の直接 parent) / `26a383f03f` (A4-complete handoff doc commit、範式継承元 source-of-truth)
**HEAD**: `eddcc7ac40` on `feature/ayastorm-r41-gl-removal`
**本 doc 位置付け**: A5 完遂状態 + A6 着手境界 を fresh context 引継 用に確定する doc-only handoff。A4-complete `26a383f03f` 範式継承。AYA 「OK」明示承認下で commit (no auto-commit)。

---

## §1 起草目的

β-2-bundle-A scope 中 A5 (per-material extension sampler binding (F) + Probe + Post/utility + pbrterrainF 専用 TerrainDetailUBO + PBR terrain detail sampler) を 50 file に注入完遂した状態を確定し、A6 (per-draw (G) PerDrawUBO + (H) per-draw sampler、skinning matrixPalette/lastMatrixPalette + per-light color/proj_mat + clipPlane + size、推定 ~15 file) 着手境界を fresh context に引継ぐ。binding 番号 / 注入範式 / 既処理 file untouched 履行 / cold cache launch verify 結果 / A4 で確立した 6 hard rule の継承を全て literal canonical で固定する。

---

## §2 A5 完遂 status

| 項目 | 値 |
|---|---|
| Scope | per-material extension sampler (F) `set=1/binding=4-16` + Probe `set=1/binding=40-42` + Post/utility `set=1/binding=50-66` + pbrterrainF 専用 TerrainDetailUBO `set=1/binding=17/std140` + PBR terrain detail sampler `set=1/binding=20-39` (5×4 layout) |
| 注入 file 数 | **50 file** (Agent1 class1/deferred 30 file + Agent2 class1/interface + class1/objects + class1/post + class2 + class3 + class1/gltf 20 file) |
| 変更行数 | **+312 / -0** (insertions-only) |
| 3-段 swap block 総数 | **68 block** (Agent1 43 + Agent2 25)、全 50 file balanced (`#ifdef` = `#else` = `#endif` counts 一致) |
| TerrainDetailUBO 注入 declaration | **1 件** (pbrterrainF 専用、artifact §2 (E) Note 「Handle in sub-bundle A5」明示委任に基づく新規 UBO、binding=17) |
| extension sampler 注入 declaration (binding=4-16 狭義) | **45 件** (=4 diffuseRect 20 / =5 emissiveMap 4 / =6 bumpMap 5 / =7 emissiveRect 4 / =8 metallicRoughnessMap 1 / =9 occlusionMap 1 / =10 bumpMap2 1 / =11 altDiffuseMap 1 / =12 specularRect 2 / =13 tex0 3 / =14 tex1 1 / =15 texture0 1 / =16 texture1 1) |
| Probe / cube sampler 注入 declaration (binding=40-42) | **3 件** (irradianceProbes / heroProbes / color_grading_lut) |
| Post / SMAA / utility sampler 注入 declaration (binding=50-66) | **22 件** (詳細 §5.4 参照) |
| terrain detail sampler 注入 declaration (binding=20-39) | **20 件** (pbrterrainF 16 PBR detail + terrainF 4 legacy detail) |
| sampler/UBO 注入 declaration 総数 | **91 件** (1 + 45 + 3 + 22 + 20) |
| A1/A2/A3/A4/A8-recovery 既処理 file untouched (既存 block) | **違反 0 件** (9 file 追加注入対象でも既存 UBO/sampler 行 diff 全 0、extension sampler のみ独立 3-段 swap 追加) |
| skip list 4 file untouched | **全 untouched** (exemplar 2 = diffuseV/F + A2 拡張 2 = previewV/multiPointLightF) |
| AYA cold cache launch verify | **PASS** (起動成立 + clean shutdown + GL `.shaderbin/shader_cache` 224 件再生成 + crash 0 + GL shader compile/link fail 0) |
| commit | `eddcc7ac40` (AYA 「OK commit して」明示指示下) |
| 2 Agent 並列 patch | Agent1 (class1/deferred 30) + Agent2 (interface/objects/post/class2/class3/gltf 20) |

### §2.1 既処理 9 file への追加注入 内訳 (A4 範式継承で既存 block untouched)

extension sampler を 1 件以上参照する file は既処理 sub-bundle 完了済でも追加注入対象 (conditional injection 範式)。9 file 列挙:

| file | 既処理 sub-bundle | A5 追加注入 |
|---|---|---|
| `class1/deferred/deferredUtil.glsl` | A3 (depthMap等 sampler) + A4 (normalMap sampler) | extension sampler 1 件 |
| `class1/deferred/pbrglowF.glsl` | A4 (MaterialUBO) | extension sampler 1 件 |
| `class1/deferred/pbropaqueF.glsl` | A4 (dual IS_HUD MaterialUBO 2 block) | extension sampler 数件 |
| `class1/deferred/skyF.glsl` | A3 (environmentMap sampler2D) | extension sampler 1 件 |
| `class1/deferred/volumetricLightF.glsl` | A8-recovery (untouched 復活 file、UBO) | extension sampler 1 件 |
| `class1/gltf/pbrmetallicroughnessF.glsl` | A4 (SSBO exclusion 範式、sampler のみ) | extension sampler 数件 |
| `class2/deferred/pbralphaF.glsl` | A4 (dual IS_HUD MaterialUBO 2 block) | extension sampler 1 件 |
| `class3/deferred/materialF.glsl` | A3 (environmentMap samplerCube) + A4 (MaterialUBO) | extension sampler 1 件 |
| `class3/deferred/reflectionProbeF.glsl` | A3 (sampler cluster) | extension sampler 数件 |

全 9 file で既存 UBO block (FrameViewProj/FrameLights/FrameAtmosphere/MaterialUBO) + 既存 sampler binding qualifier は **absolute untouched**、extension sampler のみ独立 3-段 swap 追加。

---

## §3 binding rule §2 (F) + TerrainDetailUBO literal canonical (A5 で確定)

### §3.1 extension sampler binding 表 (set=1, binding=4-16)

```
| sampler              | type      | binding             | file 参照数 (A5 注入) |
|----------------------|-----------|---------------------|----------------------|
| diffuseRect          | sampler2D | set=1, binding=4    | 20 |
| emissiveMap          | sampler2D | set=1, binding=5    |  4 |
| bumpMap              | sampler2D | set=1, binding=6    |  5 |
| emissiveRect         | sampler2D | set=1, binding=7    |  4 |
| metallicRoughnessMap | sampler2D | set=1, binding=8    |  1 |
| occlusionMap         | sampler2D | set=1, binding=9    |  1 |
| bumpMap2             | sampler2D | set=1, binding=10   |  1 |
| altDiffuseMap        | sampler2D | set=1, binding=11   |  1 |
| specularRect         | sampler2D | set=1, binding=12   |  2 |
| tex0                 | sampler2D | set=1, binding=13   |  3 |
| tex1                 | sampler2D | set=1, binding=14   |  1 |
| texture0             | sampler2D | set=1, binding=15   |  1 |
| texture1             | sampler2D | set=1, binding=16   |  1 |
```

### §3.2 pbrterrainF 専用 TerrainDetailUBO literal (set=1, binding=17/std140)

```glsl
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=17, std140) uniform TerrainDetailUBO {
    vec4 baseColorFactors[4];    // 4 detail layer
    vec3 emissiveColors[4];      // 4 detail layer (vec3 array → std140 で各要素 16-byte stride)
    vec4 metallicFactors;        // 4 detail layer 1 vec4 packed
    vec4 roughnessFactors;       // 4 detail layer 1 vec4 packed
    vec4 minimum_alphas;         // 4 detail layer 1 vec4 packed
};
#endif
```

- **割当根拠**: artifact §2 (E) Note 「Handle in sub-bundle A5」明示委任 + binding=17 は (F) extension sampler 表の gap (16 と 20-39 の間 unallocated 17-19) を利用、post/utility binding=50-66 と衝突なし。
- **size**: std140 で 176 byte (vec4[4]=64 + vec3[4]=64 std140 stride + vec4*3=48)。
- **range**: artifact §2 では未記載の新規設計。本 doc + commit message §2 + A4-complete §6.2 から再生成可能。

### §3.3 PBR terrain detail sampler 表 (set=1, binding=20-39、5×4 layout)

artifact §2 (F) literal を踏襲:

```
detail_0          binding=20    detail_0_base_color         binding=21
detail_0_normal   binding=22    detail_0_metallic_roughness binding=23
detail_0_emissive binding=24
detail_1          binding=25    detail_1_base_color         binding=26
detail_1_normal   binding=27    detail_1_metallic_roughness binding=28
detail_1_emissive binding=29
detail_2          binding=30    detail_2_base_color         binding=31
detail_2_normal   binding=32    detail_2_metallic_roughness binding=33
detail_2_emissive binding=34
detail_3          binding=35    detail_3_base_color         binding=36
detail_3_normal   binding=37    detail_3_metallic_roughness binding=38
detail_3_emissive binding=39
```

A5 注入実績: pbrterrainF が 16 PBR detail sampler + terrainF が 4 legacy detail (`detail_0/1/2/3`) = 計 20 declaration。

### §3.4 Probe / cube sampler 表 (set=1, binding=40-42)

```
| sampler            | type             | binding             | 注入数 |
|--------------------|------------------|---------------------|--------|
| irradianceProbes   | samplerCubeArray | set=1, binding=40   | 1 |
| heroProbes         | samplerCubeArray | set=1, binding=41   | 1 |
| color_grading_lut  | sampler3D        | set=1, binding=42   | 1 |
```

### §3.5 Post / SMAA / utility sampler 表 (set=1, binding=50-66)

artifact §2 (F) literal:

```
| sampler          | binding             | 注入数 |
|------------------|---------------------|--------|
| exclusionTex     | set=1, binding=50   | 3 |
| alpha_ramp       | set=1, binding=51   | 2 |
| paint_map        | set=1, binding=52   | 1 |
| projectionMap    | set=1, binding=53   | 1 |
| screenTex        | set=1, binding=54   | 2 |
| velocityTex      | set=1, binding=55   | 2 |
| velocityMap      | set=1, binding=56   | 1 |
| srcMap           | set=1, binding=57   | 1 |
| halo_map         | set=1, binding=58   | 1 |
| rainbow_map      | set=1, binding=59   | 1 |
| dither_tex       | set=1, binding=60   | 1 |
| predicationTex   | set=1, binding=61   | 1 |
| previousColorTex | set=1, binding=62   | 1 |
| searchTex        | set=1, binding=63   | 1 |
| edgesTex         | set=1, binding=64   | 1 |
| areaTex          | set=1, binding=65   | 1 |
| blendTex         | set=1, binding=66   | 1 |
```

### §3.6 3-段 swap pattern 範式 (A3/A4 継承)

extension sampler:

```glsl
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=N) uniform <type> <name>;
#else
uniform <type> <name>;
#endif
```

TerrainDetailUBO (新規 UBO):

```glsl
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=17, std140) uniform TerrainDetailUBO { ... };
#else
uniform vec4 baseColorFactors[4];
uniform vec3 emissiveColors[4];
uniform vec4 metallicFactors;
uniform vec4 roughnessFactors;
uniform vec4 minimum_alphas;
#endif
```

pbrterrainF は 3 block 個別 swap:
- block 1 = `alpha_ramp` / `paint_map` (TERRAIN_PAINT_TYPE 分岐内)
- block 2 = `detail_N_*` (TERRAIN_PBR_DETAIL 分岐内 4 sub-block)
- block 3 = TerrainDetailUBO (+ GL `#else` 内 TERRAIN_PBR_DETAIL 条件 declaration 順序 byte-for-byte 保全)

---

## §4 cold cache launch verify metric (2026-06-01) vs A4 baseline

| metric | A4 baseline | A5 current | 差分 | 解釈 |
|---|---|---|---|---|
| hook fire (`generatePerProgramSPIRV`) | 224 | 224 | 0 | cold cache 経路 OK |
| parse fail | 224/224 | 224/224 | 0 | A5 単独 SPIR-V 生成 0% controlled、A2/A3/A4 同 measurement design、glslang per-file first-error fail semantics 通り |
| `ERROR ... 'location'` | 155 | 158 | **+3** | A5 patches 解消後の次 error 露出 cascade |
| `ERROR ... 'binding'` | 35 | 30 | **-5** | **A5 patches 効果直接観測** |
| `ERROR ... 'non-opaque uniforms outside block'` | 34 | 36 | **+2** | small cascade |
| `ERROR ... missing #endif` | 8 | 8 | 0 | glslang error cascade artifact、bundle-A 全体完遂で根本 error 解消時に自然消滅 |
| GL `.shaderbin/shader_cache` 再生成 | 223 | 224 | **+1** | GL path regression 0、shader 1 件追加 |
| link failed | 0 | 0 | 0 | - |
| FATAL / SIGSEGV / crash | 0 | 0 | 0 | 7 件 mention 全 benign (CrashSettings group load 3 + settings_crash_behavior load/save 4) |
| 起動成立 + clean shutdown | OK | OK | - | Goodbye! + Vulkan device destroyed + Vulkan instance destroyed 確認、charter §3 #1 acceptance 担保 |

### §4.1 net delta 解釈

`binding -5 + location +3 + non-opaque +2 = 0` (net delta ±0)。ただし **binding category specifically -5** が A5 patches 効果直接観測。location +3 / non-opaque +2 は glslang per-file first-error fail semantics による次 error 露出 cascade で A5 patch defect ではない。controlled measurement design 内 (A2/A3/A4 同範式)、bundle-A 全体完遂時の集合的閾値で評価する設計 (bundle-A-prep §6.1)。charter §3 #1 acceptance 担保。

### §4.2 PBR shader 残 error

Deferred PBR Opaque/Alpha + HUD PBR + Skinned PBR Shader 群は依然 binding/non-opaque で fail。これは A5 scope 外の SSBO (gltf_material_data) + PerDrawUBO 由来で、**A6 (PerDrawUBO + per-light + skinning) 範式で解消予定**。A5 patch 自体は期待通り動作。

---

## §5 self-verify 結果

### §5.1 (a) skip list 違反 = 0 件

4 file × git diff = 全 0 件:
- `class1/deferred/diffuseV.glsl` (exemplar)
- `class1/deferred/diffuseF.glsl` (exemplar)
- `class1/deferred/previewV.glsl` (A2 拡張 skip)
- `class3/deferred/multiPointLightF.glsl` (A2 拡張 skip)

### §5.2 (b) UBO/sampler declaration canonical byte-for-byte = 50/50 file 完全一致

binding 番号 + type は §3 表 (artifact §2 (F) + TerrainDetailUBO §3.2) 遵守、独自割当 0 件。

### §5.3 (c) 3-段 swap pattern 整合

全 50 file balanced (`#ifdef LL_VULKAN_GLSL` = `#else` = `#endif` counts 一致)、`#if defined(LL_VULKAN_GLSL)` 等変形 0 件、Agent1 43 block + Agent2 25 block = 68 block。

### §5.4 (d) binding 番号別 注入 count

```
=4  (diffuseRect)              20
=5  (emissiveMap)               4
=6  (bumpMap)                   5
=7  (emissiveRect)              4
=8  (metallicRoughnessMap)      1
=9  (occlusionMap)              1
=10 (bumpMap2)                  1
=11 (altDiffuseMap)             1
=12 (specularRect)              2
=13 (tex0)                      3
=14 (tex1)                      1
=15 (texture0)                  1
=16 (texture1)                  1
=17 (TerrainDetailUBO)          1
=20-39 (terrain sampler)       20   (pbrterrainF 16 PBR detail + terrainF 4 legacy detail)
=40 (irradianceProbes)          1
=41 (heroProbes)                1
=42 (color_grading_lut)         1
=50 (exclusionTex)              3
=51 (alpha_ramp)                2
=52 (paint_map)                 1
=53 (projectionMap)             1
=54 (screenTex)                 2
=55 (velocityTex)               2
=56 (velocityMap)               1
=57 (srcMap)                    1
=58 (halo_map)                  1
=59 (rainbow_map)               1
=60 (dither_tex)                1
=61 (predicationTex)            1
=62 (previousColorTex)          1
=63 (searchTex)                 1
=64 (edgesTex)                  1
=65 (areaTex)                   1
=66 (blendTex)                  1
```

binding 番号 4-16 / 20-39 / 40-42 / 50-66 は全て artifact §2 (F) canonical 範囲内、独自割当 0 件。binding=17 (TerrainDetailUBO) のみ artifact §2 (F) 未記載の新規割当 (artifact §2 (E) Note 「Handle in sub-bundle A5」明示委任 + unallocated gap 17-19 を利用、本 doc §3.2 / §6.2 / §7.2 で literal canonical 確定、後 maintenance window で artifact 本体へ back-port 予定)。

### §5.5 (e) A1/A2/A3/A4/A8-recovery 既処理 9 file の既存 block untouched

9 file × UBO 行 / 既存 sampler 行 diff = 全 0 件:
- FrameViewProj / FrameLights / FrameAtmosphere / MaterialUBO 不変
- A3 既存 sampler binding qualifier 不変
- extension sampler のみ独立 3-段 swap 追加

### §5.6 (f) insertions-only = 312 insertions / 0 deletions

deletion 例外 0 件 (A1/A2 のような non-canonical uniform move 例外なし)。

---

## §6 設計差分 (A4 vs A5)

| sub-bundle | UBO/sampler 構造 | 注入形式 |
|---|---|---|
| A4 | (E) MaterialUBO `set=1/binding=0/std140` + (F) per-material core sampler `set=1/binding=1-3` (diffuseMap/normalMap/specularMap) | UBO block + individual sampler 混在 |
| **A5** | **(F) extension sampler `set=1/binding=4-16` + Probe `40-42` + Post/utility `50-66` + 新規 TerrainDetailUBO `set=1/binding=17/std140` + PBR terrain detail sampler `20-39`** | **individual sampler (UBO block 不作 = A4 個別 sampler 範式継承) + pbrterrainF のみ A4-範式 TerrainDetailUBO 新規 UBO 注入** |
| A6 (予定) | (G) PerDrawUBO `set=2/binding=0` + (H) per-draw sampler `set=2/binding=1+` (skinning matrixPalette/lastMatrixPalette + per-light color/proj_mat + clipPlane + size) | UBO block + dual-context routing |

### §6.1 A4 個別 sampler 範式継承

extension sampler は MaterialUBO 不作の individual declaration (`layout(set=1, binding=M) uniform sampler2D <name>;` 直接注入)。A4 で確立した dual IS_HUD branch 範式 + SSBO exclusion 範式 (pbrmetallicroughnessF) + 既処理 file 追加注入範式を全て継承。

### §6.2 pbrterrainF 専用 TerrainDetailUBO

artifact §2 (E) Note 「Handle in sub-bundle A5」明示委任に基づく新規 UBO:
- binding=17 (unallocated gap 17-19 内、post utility binding=50-66 と衝突なし)
- std140 alignment 順序 (vec4 array → vec3 array → vec4 scalar)
- 176 byte
- artifact §2 (F) 未記載の新規設計、本 doc §3.2 が source-of-truth

### §6.3 pbrterrainF GL #else 内 TERRAIN_PBR_DETAIL 条件 byte-for-byte 保全

legacy GL path の以下条件 declaration 順序を維持:
- `#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_METALLIC_ROUGHNESS)`
- `#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)`

### §6.4 conditional injection 範式 (A2/A3/A4 共通)

sampler 1 件でも参照する file のみ注入、参照 0 件 file は touch 無し。既処理 9 file への追加注入もこの範式に従う。

---

## §7 A5 で確定した範式 (A6 継承必須)

### §7.1 hard rule 6 件 (A6 Agent prompt 必須注入)

1. **byte-for-byte canonical**: §3 binding 表 + TerrainDetailUBO literal + 3-段 swap pattern を文字通り copy。member 順序変更 / 型変更 / 追加削除一切禁止。
2. **3-段 swap pattern 厳守**: `#ifdef LL_VULKAN_GLSL ... #else ... #endif`。`#if defined(LL_VULKAN_GLSL)` 等変形禁止。
3. **binding 番号は binding rule artifact §2 表通り**: 独自割当禁止。新規 UBO 設計が必要な場合 (A5 の TerrainDetailUBO 同様) は artifact gap を利用し handoff doc に literal 確定。
4. **1 file 1 patch**: 同 file 内で複数 swap block 必要時 (UBO + sampler 分離、または pbrterrainF 3 block 分離) は OK だが、UBO は意味単位で集約 (dual IS_HUD branch / TERRAIN_PBR_DETAIL 分岐除く)。
5. **A1/A2/A3/A4/A5/A8-recovery 既処理 file の UBO block + sampler binding qualifier 再 touch 禁止**: 違反は即 abort、A6 以降 absolute untouched。既処理 file への追加注入は extension/per-draw のみ独立 3-段 swap 追加。
6. **dual-type / dual-branch / SSBO exclusion / dual-context routing は file 毎 actual 判定**: 単純パターンマッチでなく、実 grep + binding rule artifact + 既処理 handoff doc 整合確認後に注入。

### §7.2 新規 UBO 設計範式 (A5 で確立)

pbrterrainF TerrainDetailUBO のように artifact §2 (E)/(F) の Note 明示委任に基づく新規 UBO は:
- binding 番号は artifact gap (unallocated 範囲) を利用
- std140 alignment 順序を文書化 (vec4 array → vec3 array → vec4 scalar 範式)
- size を byte 単位で記録
- handoff doc §3 literal canonical として source-of-truth 化
- artifact 自体への back-port は次 maintenance window 時に handoff から逆流

### §7.3 既処理 file 追加注入 範式 (A5 で確立)

既処理 sub-bundle 完遂 file でも extension sampler / per-draw uniform を参照する場合、既存 block を **absolute untouched** に保ち extension/per-draw のみ独立 3-段 swap 追加。A5 では 9 file 該当、全 file で既存 UBO + 既存 sampler binding qualifier の diff 0 件確認。

### §7.4 pbrterrainF 3 block 個別 swap 範式 (A5 で確立)

preprocessor 分岐 (TERRAIN_PAINT_TYPE / TERRAIN_PBR_DETAIL) 内に複数 sampler/uniform cluster がある場合、各 cluster を個別 swap block に分離:
- block 1 = `alpha_ramp` / `paint_map` (TERRAIN_PAINT_TYPE 分岐内)
- block 2 = `detail_N_*` (TERRAIN_PBR_DETAIL 分岐内 4 sub-block)
- block 3 = TerrainDetailUBO (+ GL `#else` 内 TERRAIN_PBR_DETAIL 条件 declaration 順序 byte-for-byte 保全)

A6 で skinning shader (objectSkinV.glsl 等) も同様に preprocessor 分岐 (HAS_SKIN 等) 内 cluster 出現の可能性あり、本範式継承。

---

## §8 A6 着手境界

### §8.1 A6 scope

| 軸 | 内容 |
|---|---|
| (G) PerDrawUBO | `set=2/binding=0/std140` block、A4-complete §3.6 + A5 §7.2 範式で新規 UBO 設計、member = `mat3x4 matrixPalette[MAX_JOINTS_PER_MESH_OBJECT]` + `mat3x4 lastMatrixPalette[MAX_JOINTS_PER_MESH_OBJECT]` + `vec4 clipPlane` + `float size` + per-light context member (`vec3 color` + `float size` + `mat4 proj_mat` 等、pointLightF/spotLightF/deferredUtil 限定 file-local override) |
| (H) per-draw sampler | `set=2/binding=1+`、artifact §2 (H) に「No declarations found in current scan」と記載、bundle-A scope では 0 件想定 (`set=2/binding=1-15` 予約のみ) |
| 推定 file 数 | **~15 file** (skinning ~4 + per-light pointLightF/spotLightF/deferredUtil 3 + clipPlane consumer ~5 + size consumer ~4 = ~15) |
| 推奨 Agent 並列度 | **1 Agent** (file 数少なめ + dual-context routing 厳密判定要件で 1 Agent serial が安全) |
| dual-context routing 範式 | A4 で確立: `pointLightF` / `spotLightF` / `deferredUtil` の `color` は per-light = A6 PerDrawUBO 行き、deferredUtil の `normalMap` sampler は A4 で per-material 注入済 (touch 禁止) |

### §8.2 A6 着手前チェックリスト 10 件

1. `git fetch origin` で origin 最新と同期
2. HEAD = `eddcc7ac40` 確認 (本 A5 patch commit)
3. AYAstorm 独自改造 11 file は A8-recovery で完遂済 (A4/A5 範式継承通常 file 扱い)
4. skip list 4 file (exemplar 2 + A2 拡張 2) 機械的 exclusion 継続
5. binding rule artifact `/tmp/bundle-A-binding-rules.md` §2 (G)+(H) セクション再読み込み (失効時は本 doc + A4-complete + A1-A3/A8-recovery 完遂 handoff doc から再生成可能)
6. 7 件 prior handoff doc 読了 (本 doc + A4-complete `26a383f03f` + A3-complete `524391d78d` + A2-complete `8e69841a53` + A1-complete `99afb8f1bc` + A8-recovery-complete `ac3f294643` + bundle-A-prep `1434341904`)
7. 既処理 file 再 touch 禁止 rule (A1/A2/A3/A4/A5/A8-recovery 全 sub-bundle) 絶対 enforce
8. project memory `project_ayastorm_r41_vulkan_migration.md` 確認
9. cold cache launch verify 準備 (`rm -rf ~/.ayastorm_x64/cache/` + shader cp)
10. AYA 「OK」承認待ち (A6-trace 着手前 + A6-patch 起動前 + A6 commit)

### §8.3 推奨 cadence 8 step (A1-A5 範式継承)

1. **A6-trace**: Claude bare entity grep + union + skip list 機械的除外 + dual-context routing 厳密判定 (per-light vs per-material `color`) + MAX_JOINTS_PER_MESH_OBJECT 値確認 (legacy 45 vs PBR 110)
2. **A6-prep**: 1 Agent prompt 構築 (担当 file list + PerDrawUBO canonical literal + dual-context routing 表 + hard rule 6 件)
3. **A6-patch**: 1 Agent 起動 (AYA 「OK」明示指示下、general-purpose subagent_type)
4. **A6-verify**: Claude self-verify (skip list / canonical / 3-段 swap / 既処理 untouched / insertions-only)
5. **A6-handoff**: shader cp `~/ayastorm/app_settings/shaders/...` + `rm -rf ~/.ayastorm_x64/cache/shader_cache/` + AYA launch verify + clean shutdown 確認
6. **A6-measurement**: Claude log 解析 (期待: binding error -N + non-opaque -M + location 連動 cascade、bundle-A 全体閾値で評価、PBR shader 残 error 解消観測可能性)
7. **A6-commit**: AYA 「OK commit して」明示指示下で commit
8. **A6-complete handoff doc 起草**: A6 完遂状態 + A7 (no-op pass-through 検証) または bundle-A 全体集合的閾値 measurement 着手境界を fresh context 引継 用に確定

---

## §9 risks/caveats 8 件

### §9.1 measurement plan 是正済継承

sub-bundle 単独 metric は controlled で baseline 同等 (location 増減は cascade、bundle-A 全体集合的閾値で評価)。A6 でも binding/non-opaque 大幅減少観測可能性あり、location 連動 +N も予期。A5 で binding -5 直接観測、A6 で per-draw binding 由来 error 解消で類似の category-specific 減少を期待。

### §9.2 cold cache launch verify 必須

`rm -rf ~/.ayastorm_x64/cache/` を sub-bundle 毎 verify 前必須化。A5 でも実施済。

### §9.3 既処理 file (A1/A2/A3/A4/A5/A8-recovery) の UBO block + sampler binding qualifier 一体不可分

A6 以降 absolute untouched。Agent prompt 必須注入で違反 0 件 enforce。A5 で 9 file 追加注入時の untouched 履行範式 (§7.3) を A6 でも継承。

### §9.4 PerDrawUBO 新規 UBO 設計

A5 の TerrainDetailUBO 範式 (§7.2) 継承。MAX_JOINTS_PER_MESH_OBJECT 値次第で size 計算が変動 (legacy 45 joint = ~4.4 KiB / PBR 110 joint = ~10.6 KiB)、Vulkan minimum 16 KiB maxUniformBufferRange 内だが PBR 110 で tight。A6-trace で per-program 確認要 (bundle-A-prep §7.5 既出 risk)。

### §9.5 dual-context routing 厳密判定

A4 §3.6 で確立した `color` per-material (12 file) vs per-light (`pointLightF`/`spotLightF`/`deferredUtil` 3 file) 分離を A6 でも厳密 enforce。誤注入は per-program SPIR-V parse error 発生で即発覚するが、prevention で Agent prompt に file-path 判定 rule literal 注入。

### §9.6 binding rule artifact `/tmp` persist 性 fragile

失効時は本 doc §3 + A4-complete §3.6 + bundle-A-prep §2 表から再生成可能。

### §9.7 残 skip 既知 list

exemplar 2 (diffuseV/F) untouched 維持 (sub-doc 03 §3.1.3 β-1 PoC 試作レール) + A2 拡張 skip 2 (previewV/multiPointLightF) untouched 維持。

### §9.8 context budget concern

A6 session fresh context 推奨。1 Agent 並列なので Agent 起動時の context overhead 軽微だが、A6-trace + A6-verify + A6-measurement の Claude 直接処理で context 消費する点に注意。

---

## §10 AYA 承認境界 6 件

1. 本 A5-complete handoff doc commit (doc-only)
2. A6 着手指示
3. A6-patch 1 Agent 起動
4. A6 commit
5. A1/A2/A3/A4/A5/A8-recovery 既処理 file 再 touch 禁止境界
6. skip list 4 file untouched 継続

---

## §11 次 session 投入 prompt (fresh context 推奨)

```
AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-A-A6 着手前 trace を進めて。

読了必須 (7 件):
- docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A5-complete.md (本 handoff、A6 着手境界 source-of-truth)
- handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A4-complete.md
- handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A3-complete.md
- handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A2-complete.md
- handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A1-complete.md
- handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A8-recovery-complete.md
- handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-prep.md

A6 scope: (G) PerDrawUBO set=2/binding=0/std140 + (H) per-draw sampler set=2/binding=1+
  member = mat3x4 matrixPalette[MAX_JOINTS_PER_MESH_OBJECT] + mat3x4 lastMatrixPalette[...]
        + vec4 clipPlane + float size
        + per-light context (pointLightF/spotLightF/deferredUtil 限定 file-local override
          vec3 color + float size + mat4 proj_mat)
  推定 ~15 file、1 Agent serial

cadence 8 step (A6-trace → A6-prep → A6-patch [AYA 「OK」明示要] → A6-verify [self]
              → A6-handoff [shader cp + rm -rf cache + AYA launch verify] → A6-measurement
              → A6-commit [AYA 「OK commit して」明示要] → A6-complete handoff doc 起草)

hard rule 6 件: A5-complete §7.1 参照
新規 UBO 設計範式: A5-complete §7.2 (TerrainDetailUBO 範式) 継承
dual-context routing 厳密判定: A4-complete §3.6 + A5-complete §9.5 参照

feedback rule 12 件: doubt_self_first / no_scope_shrink / self_verify_before_handoff /
                    use_agents_proactively / admit_unknown / falsification_as_progress /
                    explanation_lead_with_conclusion / no_claude_coauthor / no_auto_commit /
                    one_step_at_a_time / proactive_handoff / remove_verification_logs

1st action: A6-trace 着手前に AYA 「OK」明示確認 + 読了済 1 行 status 報告
```

---

## §12 cross reference

### §12.1 handoff doc 系譜

- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A1-complete.md` (A1 完遂、commit `99afb8f1bc`)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A2-complete.md` (A2 完遂、commit `8e69841a53`)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A8-recovery-complete.md` (A8-recovery 完遂、commit `ac3f294643`)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A3-complete.md` (A3 完遂、commit `524391d78d`)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A4-complete.md` (A4 完遂、commit `26a383f03f`)
- **本 doc** (A5 完遂、commit 予定)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-prep.md` (bundle-A 全体 prep、commit `1434341904`)
- `handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md` (β-2-hook 完遂、commit `cecb9e6467`)

### §12.2 patch commit 系譜

- `6f941c0a48` (A1 patch)
- `9f77f875db` (A2 patch)
- `aed1438936` (A8-recovery patch)
- `ebd5e2b16d` (A3 patch)
- `f703710f8d` (A4 patch)
- **`eddcc7ac40` (A5 patch、本 handoff の直接 parent)**

### §12.3 spec sub-doc

- sub-doc 06 §1.2.2 / §1.2.4 / §3.1 sub-step 6.3
- sub-doc 07 §3.1 sub-step 7.2-7.4
- sub-doc 03 §3.1.3 (exemplar 2 役割)
- charter §3 #1 + §7.5

### §12.4 artifact

- binding rule artifact `/tmp/bundle-A-binding-rules.md` §2 (F) (A5 で extension sampler / Probe / Post-utility 範囲確定) + §2 (G)+(H) (A6 source-of-truth)
- 新規 UBO `TerrainDetailUBO` literal (本 doc §3.2 source-of-truth、artifact §2 (E) Note 「Handle in sub-bundle A5」明示委任に基づく)
- skip list `/tmp/skip-list-4.txt` (base 4 file: exemplar 2 + A2 拡張 2)
- project memory `project_ayastorm_r41_vulkan_migration.md` (γ'-port-β-2-bundle-A-A5 完遂 + γ'-port-β-2-bundle-A-A6 着手境界 active)

### §12.5 feedback rules 12 件

`doubt_self_first` (A5 commit message で extension sampler binding 番号 summary 記述誤記を artifact §2 (F) canonical に補正) / `no_scope_shrink` / `self_verify_before_handoff` / `use_agents_proactively` (2 Agent 並列) / `admit_unknown` / `falsification_as_progress` / `explanation_lead_with_conclusion` / `no_claude_coauthor` / `no_auto_commit` (AYA 「OK commit して」明示指示下で commit) / `one_step_at_a_time` 遵守 / `proactive_handoff` / `remove_verification_logs`

---

**End of A5-complete handoff doc.** 次 action = fresh context で sub-step 4.3-γ'-port-β-2-bundle-A-A6 着手、本 doc §11 prompt を次 Claude session 投入用に使用可能。
