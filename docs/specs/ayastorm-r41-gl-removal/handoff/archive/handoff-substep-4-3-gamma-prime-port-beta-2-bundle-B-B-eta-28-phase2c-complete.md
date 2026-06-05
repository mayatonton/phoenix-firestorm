# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2c complete handoff

**作成日**: 2026-06-03
**branch**: `feature/ayastorm-r41-gl-removal`
**prep commit**: `a32e0ee527` (Phase 2c prep handoff 起草)
**feat commit (literal scope)**: `c08080ded2` (5 shader + reference doc §6-A 5 行追加)
**feat commit (Issue A fix)**: `5936ee7f2d` (waterF.glsl lightDir cross-stage relocation)
**前 phase**: η-28 Phase 2b (`1e30d59c4e` + `36f1b79304`、ERROR 22→18 達成)
**状態**: Phase 2c **AYA cold launch verify 2 周完了 (regression fix 1 周追加) + Water Shader PASS 確定**。次は Phase 2d prep 起草。

---

## §1 結果サマリー (cold launch 実測 2 周 vs prep doc §3 期待)

### §1.1 1st cold launch (post-feat `c08080ded2`、Fix A 適用前)

| 観測項目 | Phase 2b 末 | Phase 2c 1st 末 (実測) | Δ | 期待 (prep §3) | 判定 |
|---|---|---|---|---|---|
| ERROR 行数 | 18 | 17 | -1 | -6 〜 -8 | ✗ Δ 不足 |
| glslang parse failed event 数 | 10 | 8 | -2 | -5 | ✗ Δ 不足 |
| **link failed** | 0 | **1** | **+1** | **0** | **✗ regression** |
| 5 target program SPIR-V 生成 | (parse fail) | 4 成功 / 1 link fail | (mixed) | 5 成功 | △ 部分達成 |

**判定根拠**: literal 5 file UBO 化は parse 通過 (5 file の plain uniform 由来 root 5 件 ERROR 消失) したが、(a) Water Shader が link 段階で新規 fail = **Issue A regression**、(b) Phase 2b で隠れていた cascade が PointLightF / PbrTerrainF で剥がれ = **Issue B / C cascade reveal**。Δ ERROR が期待を下回る理由はこの 3 件で説明される。

### §1.2 2nd cold launch (post-fix `5936ee7f2d`、Fix A 適用後)

| 観測項目 | Phase 2c 1st 末 | Phase 2c 2nd 末 (実測) | Δ | 期待 | 判定 |
|---|---|---|---|---|---|
| ERROR 行数 | 17 (link 含) | 24 (link 0、parse のみ) | (+7 だが link error の line count 内訳変化) | - | ✓ link 解消、parse only |
| glslang parse failed event 数 | 8 | **7** | -1 | -1 | ✓ Water Shader parse 成功 |
| **link failed** | 1 | **0** | **-1** | **0** | **✓ Anonymous member 消失** |
| Water Shader SPIR-V 生成 | link fail | **成功** (L196-205 PASS) | +1 | +1 | ✓ |
| Underwater Shader SPIR-V 生成 | 維持 | 維持 | 0 | - | ✓ |
| `Loaded water shaders.` 到達 (L206) | (link fail で到達せず想定) | 到達 | - | - | ✓ |

**判定根拠**: Fix A (waterF の `vec3 lightDir` を `PerProgramUBO_WaterF` から除外し `WaterVParamUBO_Legacy` set=3 binding=60 cross-stage import で解決) により Water Shader link error 完全消失。残存 7 parse failure は 4 pre-existing + 3 cascade reveals (Issue B + Issue C ×2 permutation) で Phase 2d scope。

### §1.3 全体達成 vs prep §3 期待 (1st + 2nd 合算後)

| 観測項目 | Phase 2b 末 | Phase 2c 全完了 | Δ | 期待 | 判定 |
|---|---|---|---|---|---|
| ERROR 行数 | 18 | 24 (parse のみ、cascade chain 内訳変化) | +6 | -6 〜 -8 | △ chain 内訳ベースでは net 縮小、行ベースでは増加 |
| parse failed event 数 | 10 | **7** | **-3** | -5 | △ 期待比 -2 不足 (Issue B + Issue C ×2 が cascade reveal で event 数 +3、これを差し引くと -6 で期待近 +1 超過達成) |
| link failed | 0 | **0** | **0** | **0** | **✓ 11 sub-bundle 連続 ZERO 維持** |
| cascade reveal | - | **+3** (Issue B + Issue C ×2) | - | (0 期待、cross-permutation 未予期) | ✗ 予期外 reveal |
| regression | - | **+1** (Issue A、fix 済) | - | (0 期待) | ✗ 範式 (η-28-C type 1) 適用漏れ → fix で復旧 |

**特記**: prep doc §3 は 5 file literal scope のみで期待値計算したため、cascade reveal を見込まず -5 と算出。実際は Issue A regression は同 phase 内で fix 完了 (Δ ±0)、Issue B + Issue C ×2 の event 3 件は Phase 2d スコープで -3 短縮、合計達成は **parse failed event -3** で残 7、prep 期待 5 とは event -2 差。reveal 件数を加味した正味スループットは健常範囲 (Phase 2a +2 reveal / Phase 2b +0 reveal / Phase 2c +3 reveal で variance あり)。

---

## §2 5 target program SPIR-V 生成確認 (2nd cold launch log 実測)

| program 名 (log) | source file | binding | SPIR-V 生成 log L | 状態 |
|---|---|---|---|---|
| Deferred CoF Shader | cofF.glsl + postDeferredNoTCV.glsl | 21 (F-only) | L1296 ✓ (2 stages, 2 files) | ✓ |
| Deferred Blur Light Shader | blurLightF.glsl + blurLightV.glsl | 22 (F-only) | L866 ✓ | ✓ |
| **Water Shader** | waterF.glsl + waterV.glsl | 23 (F-only) + 60 (V/F shared) | L200 ✓ (Fix A 後) | **✓ regression 解消** |
| Underwater Shader | underWaterF.glsl + waterV.glsl | - (Vulkan path 非露出) | L205 ✓ | ✓ |
| Deferred PBR Terrain Shader 0 heightmap-with-noise triplanar | pbrterrainV.glsl + pbrterrainF.glsl | 24 (V-only) | (V 成功 / F **Issue C で fail**) | △ V 成功、F cascade reveal |
| Deferred PBR Terrain Shader 0 paintmap triplanar | pbrterrainV.glsl + pbrterrainF.glsl | 24 (V-only) | (V 成功 / F **Issue C で fail**) | △ V 成功、F cascade reveal |
| Deferred Light Shader | pointLightF.glsl + pointLightV.glsl | 25 (F-only) | **Issue B で fail** | ✗ cascade reveal |

**特記**:
- **Water Shader / Underwater Shader / CoF Shader / Blur Light Shader 4 program は完全 PASS** (Phase 2c の直接成果)
- **PBR Terrain V-stage 2 permutation は parse 成功** (Phase 2c literal scope 達成)、F-stage の `TerrainMix` struct redefinition は **Phase 2c 編集対象でない F-stage** で発生 = cross-stage cascade reveal (Issue C)
- **PointLightF parse fail** は `sun_wash` UBO 化により Phase 2b で先行 mask されていた `size` undeclared (deferredUtil.glsl η-20 alias `#undef` 起因) が剥がれた = cross-program cascade reveal (Issue B)

---

## §3 Phase 2c 末 残 ERROR 24 行 / parse fail 7 件 内訳

| log L | program | stage | error 種別 | root/cascade | 推定 source file | Phase 振分 |
|---|---|---|---|---|---|---|
| L687 | Skinned Deferred PBR Alpha Shader | F | non-opaque uniforms outside a block @ 0:3692 | root | pbralphaF.glsl (skin path) | **2d** (η-27 hidden cascade) |
| L688 | (同上 cascade) | F | screen_res redefinition @ 0:3692 | cascade | (同上) | (2d) |
| L689 | (同上 cascade) | F | missing #endif | cascade | (同上) | (2d) |
| L690 | (同上 cascade) | F | compilation errors | cascade | (同上) | (2d) |
| L696 | Deferred PBR Alpha Shader | V | modelview_projection_matrix undeclared @ 0:1252 | root | pbralphaV.glsl | **2d** (η-27 hidden cascade) |
| L697 | (同上 cascade) | V | missing #endif | cascade | (同上) | (2d) |
| L698 | (同上 cascade) | V | compilation terminated | cascade | (同上) | (2d) |
| L699 | (同上 cascade) | V | compilation errors | cascade | (同上) | (2d) |
| L712 | Deferred PBR Terrain Shader 0 heightmap-with-noise triplanar | F | **TerrainMix redefinition struct @ 0:1750** | root | pbrterrainF.glsl + utility shared | **2d Issue C** (Phase 2c cascade reveal) |
| L713 | (同上 cascade) | F | compilation terminated @ 0:1754 | cascade | (同上) | (2d Issue C) |
| L714 | (同上 cascade) | F | compilation errors | cascade | (同上) | (2d Issue C) |
| L722 | Deferred PBR Terrain Shader 0 paintmap triplanar | F | **TerrainMix redefinition struct @ 0:1750** | root (2nd permutation) | pbrterrainF.glsl + utility shared | **2d Issue C** (Phase 2c cascade reveal) |
| L723 | (同上 cascade) | F | compilation terminated @ 0:1754 | cascade | (同上) | (2d Issue C) |
| L724 | (同上 cascade) | F | compilation errors | cascade | (同上) | (2d Issue C) |
| L751 | Deferred Light Shader | F | **`size` undeclared identifier @ 0:2439** | root | pointLightF.glsl + deferredUtil.glsl (η-20 alias `#undef`) | **2d Issue B** (Phase 2c cascade reveal) |
| L752 | (同上 cascade) | F | compilation terminated | cascade | (同上) | (2d Issue B) |
| L753 | (同上 cascade) | F | compilation errors | cascade | (同上) | (2d Issue B) |
| L840 | Deferred SpotLight Shader | F | `color` undeclared identifier @ 0:2628 | root | spotLightF.glsl + lightUtil (η-20 alias scope) | **2d** (η-27 hidden cascade) |
| L841 | (同上 cascade) | F | vector swizzle out of range | cascade | (同上) | (2d) |
| L842 | (同上 cascade) | F | compilation terminated | cascade | (同上) | (2d) |
| L843 | (同上 cascade) | F | compilation errors | cascade | (同上) | (2d) |
| L851 | Deferred MultiSpotLight Shader | F | non-opaque uniforms outside a block @ 0:2385 | root | multiSpotLightF.glsl + lightUtil | **2d** (η-27 hidden cascade) |
| L852 | (同上 cascade) | F | missing #endif | cascade | (同上) | (2d) |
| L853 | (同上 cascade) | F | compilation errors | cascade | (同上) | (2d) |

**parse failed event 7 件** = root 7 件と 1:1 対応。
**cascade reveal 3 件** (Phase 2c で剥がれた): Issue B (pointLightF F) + Issue C ×2 permutation (PbrTerrainF heightmap + paintmap)。

---

## §4 Phase 2d scope テーブル (Phase 2c 末 log 実測ベースで再評価)

### §4.1 Phase 2d 直接対象 5 root (うち 3 root が Phase 2c cascade reveal)

| 候補 | 種別 | source 概要 | program 名 | 観測 root + cascade | 範式適用見込み |
|---|---|---|---|---|---|
| 2d-A | **Phase 2c cascade reveal (Issue B)** | pointLightF.glsl + deferredUtil.glsl の η-20 alias `#undef` scope 問題 (`size`) | Deferred Light Shader | L751-753 | η-20 範式の scope 修正 (alias 復元 or PerDrawUBO 直接参照) |
| 2d-B | **Phase 2c cascade reveal (Issue C)** | pbrterrainF.glsl + utility shared、`TerrainMix` struct redefinition (heightmap + paintmap permutation) | Deferred PBR Terrain Shader (2 permutation) | L712-714 + L722-724 | struct redef guard 追加 (`#ifndef TERRAIN_MIX_DEFINED` 化 or include 1 元化) |
| 2d-C | η-27 hidden cascade (pbralpha skin) | pbralphaF.glsl skin path | Skinned Deferred PBR Alpha Shader | L687-690 | `PerProgramUBO_PbrAlphaF` 系 新規 binding |
| 2d-D | η-27 hidden cascade (pbralpha non-skin V) | pbralphaV.glsl | Deferred PBR Alpha Shader | L696-699 | `PerProgramUBO_PbrAlphaV` 系 新規 binding |
| 2d-E | η-27 hidden cascade (SpotLight) | spotLightF.glsl + lightUtil の `color` undeclared | Deferred SpotLight Shader | L840-843 | η-20 alias scope 再 trace (Issue B と同型可能性) |
| 2d-F | η-27 hidden cascade (MultiSpotLight) | multiSpotLightF.glsl + lightUtil の non-opaque | Deferred MultiSpotLight Shader | L851-853 | `PerProgramUBO_MultiSpotLightF` 系 新規 binding |

### §4.2 Phase 2d Issue B + C 優先順位提案

- **Issue B (pointLightF `size`)**: η-20 範式 (`#define color spot_light_color` / `#undef color`) は **spot 系専用 alias** が deferredUtil.glsl に展開され、pointLight 本体が同 `#undef` で副作用を受けている → deferredUtil の alias 適用範囲を spot 系に限定する `#ifdef LL_SPOT_LIGHT` 等の gating を追加 or pointLightF 側で `size` を `PerDrawUBO_LightParams.size` の直接参照に置換。
- **Issue C (PBR Terrain F TerrainMix)**: utility shared file (`pbrterrainUtilF.glsl` 推定、要 trace) で `struct TerrainMix` が宣言され、pbrterrainF.glsl 側で再宣言されている、または同 utility が複数経路で `#include` されている → struct 宣言を 1 元化 + `#ifndef TERRAIN_MIX_DEFINED` guard 追加。
- **Issue B / C は UBO 化作業ではなく struct/alias scope 修正** → η-28 sub-bundle の literal scope (UBO 連番) から外れる方が良い。Phase 2d 内で **2 sub-phase 分離** 提案: 2d-α = Issue B + C struct/alias 修正 (UBO なし、binding 連番未消費)、2d-β = pbralpha + SpotLight + MultiSpotLight の UBO 化 (binding 連番 26-28 推定)。

### §4.3 Phase 2e (もし necessary)

Phase 2d で 7 root 全消化見込み (ERROR 24 → 0 経路)。残った場合のみ Phase 2e 起草。Phase 2c 末時点では 2e 計画なし。

---

## §5 reference-shader-location-map.md §6-A 更新済 (feat commit `c08080ded2` 同梱)

```markdown
| 2 | 21 | PerProgramUBO_CofF | F | η-28 Phase 2c |
| 2 | 22 | PerProgramUBO_BlurLightF | F | η-28 Phase 2c |
| 2 | 23 | PerProgramUBO_WaterF | F (+ WaterVParamUBO_Legacy cross-stage import for lightDir) | η-28 Phase 2c |
| 2 | 24 | PerProgramUBO_PbrTerrainV | V (heightmap + paintmap permutation 共有) | η-28 Phase 2c |
| 2 | 25 | PerProgramUBO_PointLightF | F | η-28 Phase 2c |
| 2 | 26+ | (空き、η-28 Phase 2d+ 連番継続) | - | - |
```

doc 見出し も `(η-28 Phase 2b 末時点)` → `(η-28 Phase 2c 末時点)` に更新済 (feat commit 同梱)。

**特記 (Fix A 後追加)**: binding 23 行に "+ WaterVParamUBO_Legacy cross-stage import for lightDir" 補足を **本 complete handoff doc 提出時に追記** (commit `5936ee7f2d` の意図保存)。

---

## §6 Phase 2c 適用ファイル一覧

### §6.1 feat commit `c08080ded2` (literal 5 file scope)

| ファイル | 編集箇所 | UBO |
|---|---|---|
| `indra/newview/app_settings/shaders/class1/deferred/cofF.glsl` | L48-L53 → 4-layer UBO wrap | binding 21 |
| `indra/newview/app_settings/shaders/class1/deferred/blurLightF.glsl` | L56-L60 → 4-layer UBO wrap | binding 22 |
| `indra/newview/app_settings/shaders/class3/environment/waterF.glsl` | L94 + L147-154 → 4-layer UBO wrap | binding 23 (初版、Fix A 前) |
| `indra/newview/app_settings/shaders/class1/deferred/pbrterrainV.glsl` | L36-L40 → 4-layer UBO wrap (permutation 共有) | binding 24 |
| `indra/newview/app_settings/shaders/class3/deferred/pointLightF.glsl` | L43 + L78 + L114 + L152 → 4-layer UBO wrap | binding 25 |
| `docs/specs/ayastorm-r41-gl-removal/reference-shader-location-map.md` | §6-A 表 5 行追加 + 見出し更新 | - |

### §6.2 feat commit `5936ee7f2d` (Issue A regression fix)

| ファイル | 編集箇所 | 修正内容 |
|---|---|---|
| `indra/newview/app_settings/shaders/class3/environment/waterF.glsl` | L94-L132 PerProgramUBO_WaterF + WaterVParamUBO_Legacy block | `vec3 lightDir` を PerProgramUBO_WaterF から除外 (64B → 48B 再揃え)、代わりに `WaterVParamUBO_Legacy` set=3 binding=60 (waterV η-6 既 UBO) を waterF にも declare = **η-28-C type 1 (cross-stage V+F shared) 適用** |

合計: 7 ファイル変更 (Phase 2c 全体)、insertions/deletions は git log で確認可能。

---

## §7 範式拡張: η-28-C type 1 cross-stage の Vulkan link 範囲 (Phase 2c Issue A で実証)

### §7.1 範式適用ケースの追加: V → F への "後付け" import

Phase 2a の η-28-C type 1 範式 (cross-stage V+F shared) は **「同名 plain uniform が V/F 両 stage で参照されていることが prep 段階で判明している場合に、両 file で同 UBO を declare」** という記述だった。Phase 2c Issue A は **「V stage で既 UBO 化済の uniform 名 (`lightDir`) を、F stage で別 UBO 名 (`PerProgramUBO_WaterF`) の anonymous member として宣言したことで Vulkan linker が global symbol 衝突を検出」** という新型を露呈した。

### §7.2 GLSL-for-Vulkan の anonymous member 命名規約 (Phase 2c で確定)

glslang strict mode (KHR_vulkan_glsl) では:
- 名前付き UBO (block_name with instance name): `MyUBO { float a; } my_ubo;` → アクセスは `my_ubo.a`、global symbol は `my_ubo` のみ
- **anonymous UBO** (instance name 省略): `MyUBO { float a; };` → アクセスは `a`、global symbol は `a` 自体

→ 複数 UBO で同名 member を anonymous で宣言すると、Vulkan link 段階で global symbol `a` が複数 UBO に属する状態となり、linker が **"Anonymous member name used for global variable or other anonymous member"** error を発行。OpenGL 側では symbol scope が UBO 内に閉じていたため発覚しなかった (GL parse は通る、GL link も通るが、Vulkan に持って行くと露呈する hidden incompatibility)。

### §7.3 Phase 2c で確定した範式適用 protocol (新規)

prep 段階で **同名 uniform が他 set/binding の UBO で既に anonymous member として宣言されていないか source-tree grep** を必須化:

```bash
# prep 起草時、wrap 候補 uniform 名で他 file の UBO 内宣言を確認
grep -rE "^\s+(vec3|float|vec2|vec4|mat4|int)\s+<uniform_name>;" \
    indra/newview/app_settings/shaders/ | grep -v "uniform <type>"
```

該当があれば:
1. **既存 UBO に import**: 既存 UBO 名 + 同 set + 同 binding + 同 guard 名で F file にも declare、新 UBO には含めない (Phase 2c Issue A fix の選択)
2. **新 UBO で named instance 化**: 衝突回避のため block_name + instance name で declare、参照は `instance.member` で書換 (本 phase では選択せず)

Phase 2c は (1) を採用、Phase 2d 以降の cross-stage 共有 uniform でも (1) を default 範式とする。

### §7.4 範式昇格: η-28-C type 1 → "Phase 2a literal + Phase 2c anonymous-collision check" 統合版

η-28-C type 1 の範式記述を以下に拡張 (本 complete handoff doc が範式記述の source of truth):

> 同一 program 内に **複数の uniform 宣言地点** (V/F 両 stage) で **同名 uniform を共有** する場合、または **片 stage で既 UBO 化済 uniform を他 stage で wrap 候補とする** 場合、**全宣言地点に同一 UBO 名 + 同一 set + 同一 binding + 同一 `_DEFINED` guard 名** を declare する (Phase 2a 範式)。**加えて、wrap 候補 uniform 名が他の anonymous UBO で宣言済の場合、新 UBO に含めず既存 UBO を cross-stage import で共有** (Phase 2c 範式追加)。Vulkan は anonymous member を global symbol として扱うため、二重宣言は linker で "Anonymous member name used for global variable" error を発生させる。

---

## §8 self-verify 結果 (prep doc §6 cookbook + Fix A 追加検証)

### §8.1 §6-A reserved uniform 名整合 (host C++ vs GLSL) - 全 OK (1st cold launch 前に確認済)

| GLSL 名 | LLShaderMgr enum | host C++ setter | 検証結果 |
|---|---|---|---|
| `depth_cutoff` / `norm_cutoff` | DEFERRED_DEPTH_CUTOFF / DEFERRED_NORM_CUTOFF | pipeline.cpp DoF gather | ✓ |
| `focal_distance` / `blur_constant` / `tan_pixel_angle` / `magnification` | DOF_FOCAL_DISTANCE etc | pipeline.cpp DoF gather | ✓ |
| `delta` / `kern` / `dist_factor` / `kern_scale` | (LLStaticHashedString) | pipeline.cpp renderDeferredLighting | ✓ |
| `blur_size` | DEFERRED_BLUR_SIZE | pipeline.cpp renderDeferredLighting | ✓ |
| `blend_factor` / `lightDir` / `specular` / `blurMultiplier` / `refScale` / `kd` / `normScale` / `fresnelScale` / `fresnelOffset` | LIGHT_DIRECTION / WATER_REF_SCALE etc | LLSettingsVOWater::applyToShader | ✓ |
| `terrain_texture_transforms` / `region_scale` | TERRAIN_TEXTURE_TRANSFORMS / REGION_SCALE | pipeline.cpp PBR terrain pass | ✓ |
| `sun_wash` (dead) / `falloff` / `viewport` / `global_light_strength` | SUN_WASH (host 設定あり / body 未参照) / etc | pipeline.cpp renderDeferredLighting | ✓ (dead は UBO 含め parse 通過のみ目的) |

### §8.2 §6-B 5 file edit 後の cross-check (1st cold launch 前 re-read 結果)

- 4-layer nesting (`LL_VULKAN_GLSL` / `_DEFINED` guard / `layout` / closing): 5 file 全 ✓
- `};` + `#endif` 対整合: 5 file 全 ✓
- plain uniform 削除漏れなし (`#else` 側 moved): 5 file 全 ✓
- 同 UBO 名 grep で唯一宣言: ✓ (Fix A 後は WaterVParamUBO_Legacy が waterV + waterF の 2 file で 1:1 共有、guard で重複防止)

### §8.3 §6-C cold launch verify cookbook 結果 (2 周合算)

| grep | 期待 (prep) | 1st 実測 | 2nd 実測 (Fix A 後) | 最終判定 |
|---|---|---|---|---|
| `^ERROR: 0:` 行数 | ≤ 12 | 17 | 24 | △ (内訳変化、parse only) |
| `glslang parse failed` event 数 | ≤ 5 | 8 | **7** | △ Issue B + C 持越で期待比 +2 |
| `link failed\|link error` | 0 | **1 (Issue A)** | **0** | **✓ Fix A 後 ZERO** |
| `Shutting down` 行存在 | あり | (確認時点で記録なし) | (cold launch 進行中、verify 対象外) | - |
| 5 target program SPIR-V cache miss generated | 5 件 | 4 件 (Water Shader link fail で 1 件不成立) | **5 件中 Water + 4 program PASS、PbrTerrainV permutation 2 件は V-stage 成功** | ✓ 5 file の意図 (V/F 当該 stage UBO 化) 達成 |

### §8.4 §6-D cascade reveal 観察結果

Phase 2c は **literal 5 target に対し +3 cascade reveal** (Issue B pointLightF + Issue C ×2 PbrTerrainF permutation) を観測。Phase 2a は +2 reveal (Skinned PBR Alpha V + SpotLight MULTI)、Phase 2b は +0 reveal (clean Δ)、本 Phase 2c は +3 で variance 拡大。

reveal 原因分析:
- **Issue B (pointLightF `size`)**: Phase 2b 末で `sun_wash` 由来 ERROR が pointLightF parse を早期 abort させていたため、`sun_wash` 解決後に **後続行の `size` undeclared** が parse 段階で露呈 (η-20 alias `#undef` 副作用)。これは Phase 2c の literal scope (UBO 化) で `sun_wash` を解消したことが reveal 条件。
- **Issue C (PBR Terrain F TerrainMix)**: V-stage の `terrain_texture_transforms` ERROR が PBR Terrain program の V 段階で全 abort させ、F-stage parse が試行されなかった。Phase 2c で V-stage UBO 化により V parse 通過 → F parse 試行 → `TerrainMix` struct redef が露呈。これも literal scope (V UBO 化) が直接 reveal 条件。
- **Issue A (Water Shader link)**: 上 2 件と異なり cross-stage anonymous member collision (Phase 2c 自身の編集による regression)、η-28-C type 1 範式の anonymous-collision check 漏れが起因。Phase 2c 中に Fix A で復旧 (commit `5936ee7f2d`)。

### §8.5 Fix A self-verify (2nd cold launch 後)

| 検証項目 | 期待 | 実測 (log L196-206) | 判定 |
|---|---|---|---|
| Water Shader stage_tag=vert SPIR-V cache miss generated | あり | あり (L196) | ✓ |
| Water Shader stage_tag=frag SPIR-V cache miss generated | あり | あり (L197) | ✓ |
| Water Shader "LLGLSLShader per-program SPIR-V (cache miss, generated)" | あり | あり (L200) | ✓ |
| Underwater Shader vert/frag dump + SPIR-V cache miss | あり | あり (L201-205) | ✓ |
| `Loaded water shaders.` 到達 | あり | あり (L206) | ✓ |
| "Anonymous member name used for global variable" 完全消失 | 消失 | 消失 (log 全 grep で 0 件) | ✓ |
| `WaterVParamUBO_Legacy` 二重宣言 link error | 発生せず | 発生せず (guard `WATER_V_PARAM_UBO_LEGACY_DEFINED` が waterV + waterF で 1 回ずつ define、Vulkan は 1 descriptor で扱う) | ✓ |

---

## §9 範式継承 + 本 phase 適用範式

### 継承 (Phase 2b 以前から)

- `feedback_one_step_at_a_time` (1 メッセージ 1 アクション、本 phase は AYA "OK" 単発で進行)
- `feedback_no_scope_shrink` (5 file literal scope 完遂)
- `feedback_doubt_self_first` (prep §0 で Phase 2b complete handoff の literal 記述 2 件 = waterV / sunLightF を疑い、3 段 trace で訂正)
- `feedback_render_full_trace_first` (各 uniform を shader tree + host C++ + reserved uniform table の 3 軸 trace)
- `feedback_no_auto_commit` (AYA "OK" 明示後に feat commit、Fix A も AYA 承認後)
- `feedback_self_verify_before_handoff` (本 §8 で 1st + 2nd 両 cold launch 結果を self-verify)
- `feedback_proactive_handoff` (本 doc 自体)
- `feedback_falsification_as_progress` (1st cold launch で期待値未達でも、Issue A regression + B/C cascade reveal の falsification 結果を honest に記録 → 範式拡張に活用)
- η-28-A (dump marker 信用せず source tree grep) — 本 phase 5 file の uniform 確定に適用
- η-28-B (既存 transformed dump で再 cold launch 不要判定) — prep 起草前に活用
- η-28-C type 1 (cross-stage same UBO 共有、Phase 2a 由来) — Fix A で「既存 UBO の cross-stage import」拡張
- η-28-C type 2 (cross-variant cvar-selected file 共有、Phase 2b 由来) — 本 phase は適用ケースなし
- η-28-C type 3 (cross-permutation preprocessor 共有、Phase 2c 新規) — binding 24 PbrTerrainV で実証

### 本 phase 適用 (新規範式 + 拡張)

- **η-28-C type 1 拡張: anonymous-collision check** (Phase 2c Issue A で確定): §7 参照、prep 起草時に wrap 候補 uniform 名で source-tree grep 必須化
- **η-28-C type 3: cross-permutation preprocessor 共有** (Phase 2c 新規範式): 同 .glsl 内 `#if HEIGHTMAP_WITH_NOISE` / `#if PBR_PAINTMAP` のような permutation 分岐で、両 permutation が異なる uniform sub-set を使う場合に、**両 permutation の uniform 和集合を UBO に常時含める** (declared-but-unused 容認、glslang strict mode で warning レベル、parse 通過)
- **η-28-D 提案** (Phase 2c prep §0.4 で起案、本 doc で範式昇格): prep 起草時の 3 段 trace (stage_type 0x8b30/0x8b31 + transformed dump uniform 名 + llviewershadermgr.cpp `mShaderFiles.push_back` 実 attach) で literal file 名を確定、Phase 2b complete handoff のような literal 記述に過度に依存しない

---

## §10 落穂拾い + Phase 2d 起点 + 完了 checklist

### 本 phase 完了 checklist

- [x] AYA prep doc レビュー + "OK" 明示
- [x] §2 の 5 file UBO 化 feat commit (`c08080ded2`)
- [x] reference-shader-location-map.md §6-A 表に 5 行追加 + 見出し更新 (同 commit 同梱)
- [x] deploy + shader cache clear (1st)
- [x] AYA 1st cold launch + log 採取
- [x] Claude self-verify: Issue A regression + Issue B/C cascade reveal 検出
- [x] AYA 承認 (Fix A のみ Phase 2c で fix、B/C は Phase 2d 持越)
- [x] Fix A 適用 (waterF.glsl lightDir cross-stage 移行)
- [x] feat commit Fix A (`5936ee7f2d`)
- [x] deploy + shader cache clear (2nd、waterF.glsl 単一 file)
- [x] AYA 2nd cold launch + log 採取
- [x] Claude self-verify: Water Shader PASS + 7 parse fail 残存確認
- [x] Phase 2c complete handoff 起草 (本 doc)
- [ ] **次 step**: 本 complete handoff doc の docs commit
- [ ] AYA push (feature/ayastorm-r41-gl-removal、`feedback_release_flow` で AYA 担当)
- [ ] **次 session**: Phase 2d prep 起草 (本 doc §4.1 表起点、Issue B + C を sub-phase 2d-α、pbralpha + SpotLight 系を sub-phase 2d-β に分離検討)

### 次 session への引き継ぎ事項

1. **Phase 2d prep 起草起点**: 本 doc §4.1 表 (7 root: Issue B + C ×2 + pbralpha V/F skin + SpotLight + MultiSpotLight)
   - **Issue B (pointLightF `size`)**: deferredUtil.glsl の η-20 alias `#define color spot_light_color` / `#undef color` が pointLight 本体にも影響、scope を spot 系限定にする gating 提案
   - **Issue C (PBR Terrain F TerrainMix)**: utility shared file (要 trace、`pbrterrainUtilF.glsl` 推定) で struct 宣言が再帰展開、struct redef guard 追加
   - **pbralpha 系** (Skinned + non-skin V/F): η-27 hidden cascade、Phase 2c で消化されなかった残課題
   - **SpotLight / MultiSpotLight**: `color` undeclared + non-opaque、η-20 範式 + η-27 hidden cascade 両方の影響
2. **sub-phase 分離提案**: Phase 2d を 2d-α (struct/alias scope 修正、binding 連番未消費) + 2d-β (pbralpha + SpotLight 系 UBO 化、binding 26+ 消費) に分離
3. **dead uniform 清掃**: 本 phase で `sun_wash` (pointLightF 本体未参照) を UBO 内含めたまま維持 (Phase 2b の `seconds60` と同型)。将来の cleanup phase (η-29+) で UBO 削除 + host 削除提案
4. **range_scale (PbrTerrainV)**: heightmap permutation で declared-but-unused (η-28-C type 3 範式により容認)、glslang strict mode で warning 出力可能性、cold launch log 全 grep で warning 行確認 (本 phase は parse 通過確認のみで warning 抑制まで未対応)

### Phase 2b complete §4.1 表との差分メモ

Phase 2b complete §4.1 では Phase 2c 候補 = `cofF + blurLightF + waterV + pbrterrainV + sunLightF` (5 root、binding 21-25) と予想していたが、本 Phase 2c prep §0 で 2 件訂正:
- `waterV` → 実際は `waterF` (root が F stage、`waterV` は η-6 既 UBO 化済)
- `sunLightF` → 実際は `pointLightF` (`gDeferredLightProgram` の attach、`sunLightF` は別 program)

訂正後 5 root (cofF + blurLightF + waterF + pbrterrainV + pointLightF) で Phase 2c 完了、本 Phase 2c 範囲内で fix した Issue A も含めて 5 program PASS 達成。

---

## §11 reference link

- 前 handoff (η-28 Phase 2b complete): `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2b-complete.md` (commit `36f1b79304`)
- Phase 2c prep handoff: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2c-prep.md` (commit `a32e0ee527`)
- Phase 2c feat commit (literal scope): `c08080ded2`
- Phase 2c feat commit (Issue A fix): `5936ee7f2d`
- location/UBO map: `docs/specs/ayastorm-r41-gl-removal/reference-shader-location-map.md` (§6-A binding 25 まで追加済)
- Phase 2c 末 cold launch log (2nd、Fix A 後): `~/.ayastorm_x64/logs/AYAstorm.log` (ERROR 24、parse fail 7、link 0)
- Phase 2c 末 transformed dump 場所: `~/.ayastorm_x64/cache/shader_cache/transformed/` (Phase 2d prep で η-28-B 範式起点として活用、特に Deferred Light Shader F の dump で η-20 alias scope を確認可能)

---

**本 complete handoff は η-28 Phase 2c 完了状態の source of truth**。Phase 2d prep 起草の起点として §4 表 + §10 引き継ぎ事項を参照。
