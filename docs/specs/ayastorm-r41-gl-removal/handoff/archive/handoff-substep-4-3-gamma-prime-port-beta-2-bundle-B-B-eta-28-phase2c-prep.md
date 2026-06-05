# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2c prep handoff

**作成日**: 2026-06-03
**branch**: `feature/ayastorm-r41-gl-removal`
**前 handoff**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2b-complete.md` (commit `36f1b79304` 起草、Phase 2b 末 ERROR 18 / parse fail event 10 / link 0 / clean shutdown / ΔERROR -4 確定)
**状態**: Phase 2c **prep 起草**。AYA 承認後 feat 適用 → cold launch verify → complete handoff 起草。

---

## §0 Phase 2b complete handoff からの scope 訂正 (本 prep の最重要事項)

Phase 2b complete handoff §3 (cold launch ERROR 18 内訳) と §4.1 (Phase 2c scope 予測) には **prep 起草段階の file 特定誤り 2 件** が含まれている。本 Phase 2c prep で source-tree grep + stage_type 実検証 + llviewershadermgr.cpp attach map 確認により訂正済。

### §0.1 訂正 1: L197 root file = **`waterV` ではなく `waterF`** (V→F stage 訂正)

| 項目 | Phase 2b complete §3 の記述 | 本 prep の確定値 |
|---|---|---|
| log L197 root file | `class1/environment/waterV.glsl` (V-stage 想定) | **`class3/environment/waterF.glsl`** (F-stage) |
| stage_type 確認 | (未確認) | log L197 stage_type field = `0x8b30` = `GL_FRAGMENT_SHADER` |
| transformed dump 検証 | (未実施) | dump line 4079 = `uniform float blend_factor;` ← class3/environment/waterF.glsl L94 と一致 |
| 既存 UBO 化状況 | (誤想定) | `class1/environment/waterV.glsl` は η-6 で **`WaterVParamUBO_Legacy` set=3 binding=60** に UBO 化済、本 Phase 2c 編集対象でない |

### §0.2 訂正 2: binding 25 root file = **`sunLightF` ではなく `pointLightF`** (program attach 訂正)

| 項目 | Phase 2b complete §4.1 の記述 | 本 prep の確定値 |
|---|---|---|
| log L746 program | "Deferred Light Shader" | "Deferred Light Shader" (program 名一致) |
| 想定 attach .glsl | `class2/deferred/sunLightF.glsl` (推定) | **`class3/deferred/pointLightF.glsl`** (実 attach) |
| 確認方法 | (program 名のみで推定) | `llviewershadermgr.cpp` L1722-1730 grep で `gDeferredLightProgram` の `mShaderFiles` = `pointLightV.glsl` + `pointLightF.glsl` 確定 |
| `sun_wash` 所在 | sunLightF と想定 | **`class3/deferred/pointLightF.glsl` L43** (root)。`class2/deferred/sunLightF.glsl` は `sun_dir` + `shadow_bias` のみで `sun_wash` 不所持 |

### §0.3 訂正の波及範囲

両訂正とも **Phase 2c scope の binding 番号 (21-25) と総 file 数 (5) は不変**、訂正は file path / UBO 名 / stage / wrap 対象 uniform 集合の 4 項目に限定。Phase 2b complete §3 / §4.1 を読む後続 session 向けに本 §0 を最初に置き、誤読防止。

### §0.4 訂正発見の経緯 (範式記録)

`feedback_doubt_self_first` 適用: Phase 2b complete prep doc の literal 記述 (waterV / sunLightF) を信用せず、本 prep 起草初手で以下 3 段 trace を実施:
1. cold launch log L197 / L746 の **stage_type field** (`0x8b30` vs `0x8b31`) で stage 判別
2. transformed dump (η-28-B 範式で既存) の対応 line 内容 grep で **uniform 名 + file 推定**
3. `llviewershadermgr.cpp` の `mShaderFiles.push_back` 実 attach grep で **program → .glsl 対応**を確定

→ 両件とも prep doc 起草段階 (cold launch verify 前) の **file 想定誤り** と判明。Phase 2c 着手前に訂正できたため Δ への影響なし。本 trace 3 段は **η-28-D 範式** として §7 に追記提案 (Phase 2c complete handoff で範式 doc 反映予定)。

---

## §1 Scope (Phase 2b complete handoff §4 + 本 prep §0 訂正の整合)

### 直接対象 5 program (cold launch log 実測、Phase 2b 末 ERROR 18 → Phase 2c 末 ERROR ~10-12 期待)

| binding | UBO 名 | source .glsl | uniform | stage | program 名 (log) | 観測 ERROR 行 |
|---|---|---|---|---|---|---|
| 21 | `PerProgramUBO_CofF` | `class1/deferred/cofF.glsl` | `float depth_cutoff` + `norm_cutoff` + `focal_distance` + `blur_constant` + `tan_pixel_angle` + `magnification` (6 floats) | F | Deferred CoF Shader | L1294 (root) |
| 22 | `PerProgramUBO_BlurLightF` | `class1/deferred/blurLightF.glsl` | `float dist_factor` + `blur_size` + `vec2 delta` + `vec3 kern[4]` + `float kern_scale` | F | Deferred Blur Light Shader | L864 (root) |
| 23 | `PerProgramUBO_WaterF` (**訂正: ≠ WaterV**) | `class3/environment/waterF.glsl` | `float blend_factor` + `vec3 lightDir` + `vec3 specular` + `float blurMultiplier` + `float refScale` + `float kd` + `vec3 normScale` + `float fresnelScale` + `float fresnelOffset` | F | Water Shader | L197 (root) |
| 24 | `PerProgramUBO_PbrTerrainV` | `class1/deferred/pbrterrainV.glsl` | `vec4 terrain_texture_transforms[5]` + `float region_scale` | V | Deferred PBR Terrain Shader (heightmap + paintmap 2 permutation) | L710 + L717 (2 root) |
| 25 | `PerProgramUBO_PointLightF` (**訂正: ≠ SunLightF**) | `class3/deferred/pointLightF.glsl` | `float sun_wash` + `float falloff` + `vec4 viewport` + `float global_light_strength` | F | Deferred Light Shader | L746 (root) |

### η-28-C 範式適用 status (本 phase: 適用なし)

| 検査対象 | 結果 |
|---|---|
| binding 21 CofF cross-stage (V pair = `postDeferredNoTCV.glsl`) | V は `position` + FrameViewProj のみで CofF uniform 群不使用 → 適用不要 |
| binding 22 BlurLightF cross-stage (V pair = `blurLightV.glsl`) | V は `screen_res` + FrameViewProj 系のみで blur uniform 群不使用 → 適用不要 |
| binding 22 BlurLightF cross-variant (AYASTORM_CINEMATIC macro 分岐) | 同 .glsl 内 `#if AYASTORM_CINEMATIC` で 2 つの `main()` 実装、uniform 群は **共通 block 外** → 同 file 内変種は UBO 設計影響なし |
| binding 23 WaterF cross-variant (`class1/environment/waterF.glsl`) | class1 版は **error fallback stub** (`frag_color = vec4(1,0,1,1)` magenta、plain uniform 無し) → 適用不要 |
| binding 23 WaterF cross-stage (V pair = `class1/environment/waterV.glsl`) | V は η-6 で `WaterVParamUBO_Legacy` set=3 binding=60 に UBO 化済 → 別 descriptor、本 binding 23 と独立 |
| binding 23 underwater 変種 (`class3/environment/underWaterF.glsl`) | plain uniform 群は全て `#ifndef LL_VULKAN_GLSL` guard 内 (Vulkan path 露出なし) → 適用不要 |
| binding 24 PbrTerrainV cross-permutation (HEIGHTMAP_WITH_NOISE / PBR_PAINTMAP) | 同 .glsl 2 permutation、`region_scale` は `#if PBR_PAINTMAP` で gated、`terrain_texture_transforms` は無条件 → UBO に **両 field 常時含める** (η-28-C 範式の同 file permutation 版、Phase 2b postDeferredHQDoFF と同型) |
| binding 24 PbrTerrainV cross-stage (F pair = `pbrterrainF.glsl`) | F は η-21+ で `TerrainDetailUBO` set=1 binding=17 に UBO 化済 → 別 descriptor |
| binding 25 PointLightF cross-stage (V pair = `pointLightV.glsl`) | V は η-23 で `PerProgramUBO_PointLightV` set=2 binding=5 に UBO 化済、本 PointLightF 群 (`sun_wash` 等) 不使用 → 適用不要 |

**本 Phase 2c の η-28-C 適用範囲**: 唯一 binding 24 (PbrTerrainV) の **同 file permutation 共有** が η-28-C 同型 (Phase 2b postDeferredF ↔ postDeferredHQDoFF cross-variant の拡張)。

### 合計実装 file 数: 5 file (literal 5、preventive 0)

Phase 2b と異なり cross-variant preventive 追加なし (waterF の class1 stub と sunLightF は本 Phase scope 外、§0 訂正で確定)。

---

## §2 命名規約と alignment 設計 (reference doc §4-B + §6 命名規約に整合)

### §2.1 binding 21: `PerProgramUBO_CofF` (cofF.glsl)

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2c:
//   DoF cofF.glsl の 6 plain float uniform を UBO 化。
//   host = pipeline.cpp (DoF gather pass)、reserved uniform = DOF_FOCAL_DISTANCE 群。
//   max_cof は既に FrameAtmosphere_Lighting 内、screen_res / inv_proj は FrameViewProj 内 → 二重宣言禁止。
//   V pair = postDeferredNoTCV.glsl は CofF uniform 群不使用 → F 単独 attach。
#ifndef PER_PROGRAM_UBO_COF_F_DEFINED
#define PER_PROGRAM_UBO_COF_F_DEFINED 1
layout(set=2, binding=21, std140) uniform PerProgramUBO_CofF {
    float depth_cutoff;     // offset 0,  size 4
    float norm_cutoff;      // offset 4,  size 4
    float focal_distance;   // offset 8,  size 4
    float blur_constant;    // offset 12, size 4
    float tan_pixel_angle;  // offset 16, size 4
    float magnification;    // offset 20, size 4
    float _pad0;            // offset 24, size 4 (vec4 boundary 揃え)
    float _pad1;            // offset 28, size 4
};  // total 32
#endif
#else
uniform float depth_cutoff;
uniform float norm_cutoff;
uniform float focal_distance;
uniform float blur_constant;
uniform float tan_pixel_angle;
uniform float magnification;
#endif
```

**host C++ 整合**:
- reserved uniform 登録: llshadermgr.cpp:1675-1676 (`depth_cutoff` / `norm_cutoff`)、L1699-1702 (`focal_distance` / `blur_constant` / `tan_pixel_angle` / `magnification`)
- enum: `LLShaderMgr::DEFERRED_DEPTH_CUTOFF` / `DEFERRED_NORM_CUTOFF` / `DOF_FOCAL_DISTANCE` / `DOF_BLUR_CONSTANT` / `DOF_TAN_PIXEL_ANGLE` / `DOF_MAGNIFICATION` (要 deploy 前 §6-A 再確認)
- setter: `pipeline.cpp` DoF gather pass で `gDeferredCoFProgram.uniform1f(LLShaderMgr::DOF_FOCAL_DISTANCE, ...)` 等

**cross-stage check**: V pair = `postDeferredNoTCV.glsl` は CofF uniform 群不使用 (`position` + FrameViewProj のみ) → F 単独 attach。

### §2.2 binding 22: `PerProgramUBO_BlurLightF` (blurLightF.glsl)

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2c:
//   SSAO/shadow blur kernel uniform を UBO 化。
//   host = pipeline.cpp (renderDeferredLighting 系、reserved = DEFERRED_BLUR_SIZE + sDelta/sKern/sDistFactor)。
//   screen_res は FrameViewProj 既参照、二重宣言禁止。
//   AYASTORM_CINEMATIC マクロ 2 main() 実装は UBO 共有 (uniform 宣言は #if の外)。
#ifndef PER_PROGRAM_UBO_BLUR_LIGHT_F_DEFINED
#define PER_PROGRAM_UBO_BLUR_LIGHT_F_DEFINED 1
layout(set=2, binding=22, std140) uniform PerProgramUBO_BlurLightF {
    vec2  delta;            // offset 0,  size 8
    float dist_factor;      // offset 8,  size 4
    float blur_size;        // offset 12, size 4
    vec3  kern[4];          // offset 16, size 64 (vec3 array stride 16 × 4 = 16/32/48/64 開始)
    float kern_scale;       // offset 80, size 4
    float _pad0;            // offset 84, size 4 (vec4 boundary 揃え)
    float _pad1;            // offset 88, size 4
    float _pad2;            // offset 92, size 4
};  // total 96
#endif
#else
uniform float dist_factor;
uniform float blur_size;
uniform vec2 delta;
uniform vec3 kern[4];
uniform float kern_scale;
#endif
```

**host C++ 整合**:
- reserved uniform 登録: llshadermgr.cpp:1659 (`blur_size`)
- `delta` / `kern` / `dist_factor` / `kern_scale` は pipeline.cpp で `LLStaticHashedString sDelta` / `sKern` / `sDistFactor` / `sKernScale` 経由 (reserved enum 未登録、cached hash name lookup)
- setter: `pipeline.cpp::renderDeferredLighting` 系、SSAO blur pass で per-frame 配信

**cross-stage check**: V pair = `blurLightV.glsl` は `screen_res` + FrameViewProj のみで blur uniform 群不使用 → F 単独 attach。

**AYASTORM_CINEMATIC variant**: 同 .glsl 内 `#if AYASTORM_CINEMATIC` で 2 つの `main()` 実装、uniform 群は **`#if` の外 (L56-60) で宣言** → UBO 化後も両 variant が同 UBO を参照、追加対応不要。

### §2.3 binding 23: `PerProgramUBO_WaterF` (**訂正: ≠ WaterV**、class3/environment/waterF.glsl)

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2c:
//   Water surface PBR uniform を UBO 化 (blend_factor + lightDir + specular + 反射/屈折 param)。
//   host = LLSettingsVOWater::applyToShader 系 (WATER_FRESNEL_SCALE 他 reserved 群)。
//   FrameAtmosphere_Lighting 内 classic_mode 既参照、二重宣言禁止。
//   class1/environment/waterF.glsl は error fallback stub (plain uniform 無し) → cross-variant 適用不要。
//   waterV (V pair) は η-6 で WaterVParamUBO_Legacy set=3 binding=60 に UBO 化済 → 別 descriptor、本 binding 23 と独立。
#ifndef PER_PROGRAM_UBO_WATER_F_DEFINED
#define PER_PROGRAM_UBO_WATER_F_DEFINED 1
layout(set=2, binding=23, std140) uniform PerProgramUBO_WaterF {
    vec3  lightDir;         // offset 0,  size 12
    float blend_factor;     // offset 12, size 4  (vec3 直後 4byte に float 詰め)
    vec3  specular;         // offset 16, size 12
    float blurMultiplier;   // offset 28, size 4
    vec3  normScale;        // offset 32, size 12
    float refScale;         // offset 44, size 4
    float kd;               // offset 48, size 4
    float fresnelScale;     // offset 52, size 4
    float fresnelOffset;    // offset 56, size 4
    float _pad0;            // offset 60, size 4 (vec4 boundary 揃え)
};  // total 64
#endif
#else
uniform float blend_factor;
uniform vec3 lightDir;
uniform vec3 specular;
uniform float blurMultiplier;
uniform float refScale;
uniform float kd;
uniform vec3 normScale;
uniform float fresnelScale;
uniform float fresnelOffset;
#endif
```

**host C++ 整合**:
- reserved uniform 登録: llshadermgr.cpp:1749 (`lightDir`)、L1756-1762 (`refScale` / `normScale` / `fresnelScale` / `fresnelOffset` / `blurMultiplier`)、L1825 (`blend_factor`)
- enum: `LLShaderMgr::LIGHT_DIRECTION` 系 / `WATER_REF_SCALE` / `WATER_NORM_SCALE` / `WATER_FRESNEL_SCALE` / `WATER_FRESNEL_OFFSET` / `WATER_BLUR_MULTIPLIER` / `BLEND_FACTOR` (要 deploy 前 §6-A 再確認)
- `specular` / `kd` は llsettingsvo.cpp 経由 (`LLStaticHashedString` または reserved enum、要 §6-A 再確認)
- setter: `LLSettingsVOWater::applyToShader` で per-frame water settings push

**cross-stage check**: V pair = `class1/environment/waterV.glsl` は η-6 で `WaterVParamUBO_Legacy` set=3 binding=60 に既 UBO 化、本 binding 23 とは別 descriptor + 別 uniform 集合 → 編集不要。

**class1 vs class3 (cross-variant check)**: `class1/environment/waterF.glsl` は **error fallback stub** (`frag_color = vec4(1,0,1,1)` magenta debug output、plain uniform 無し)、本 cold launch では `mShaderLevel[SHADER_WATER] >= 3` で class3 が選ばれ class1 は parse 対象外 → cross-variant 適用不要。

**underwater 変種 (cross-variant check)**: `class3/environment/underWaterF.glsl` は plain uniform 群 L80-114 が全て `#ifndef LL_VULKAN_GLSL` guard 内 (Vulkan path 露出なし) → 編集不要。

### §2.4 binding 24: `PerProgramUBO_PbrTerrainV` (pbrterrainV.glsl、permutation 共有)

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2c:
//   PBR terrain V-stage の terrain_texture_transforms + region_scale を UBO 化。
//   host = pipeline.cpp PBR terrain pass (TERRAIN_TEXTURE_TRANSFORMS + REGION_SCALE reserved)。
//   2 permutation (HEIGHTMAP_WITH_NOISE / PBR_PAINTMAP) で同 UBO 共有、両 field 常時含める
//   (region_scale は paintmap で gated 参照、heightmap 側は declared-but-unused 容認)。
//   F pair = pbrterrainF.glsl は η-21+ で TerrainDetailUBO set=1 binding=17 に UBO 化済 → 別 descriptor。
#ifndef PER_PROGRAM_UBO_PBR_TERRAIN_V_DEFINED
#define PER_PROGRAM_UBO_PBR_TERRAIN_V_DEFINED 1
layout(set=2, binding=24, std140) uniform PerProgramUBO_PbrTerrainV {
    vec4  terrain_texture_transforms[5];  // offset 0,  size 80 (vec4 array stride 16 × 5)
    float region_scale;                   // offset 80, size 4
    float _pad0;                          // offset 84, size 4 (vec4 boundary 揃え)
    float _pad1;                          // offset 88, size 4
    float _pad2;                          // offset 92, size 4
};  // total 96
#endif
#else
uniform vec4 terrain_texture_transforms[5];
#if TERRAIN_PAINT_TYPE == TERRAIN_PAINT_TYPE_PBR_PAINTMAP
uniform float region_scale;
#endif
#endif
```

**host C++ 整合**:
- reserved uniform 登録: llshadermgr.cpp:1533 (`terrain_texture_transforms`)、L1812 (`region_scale`)
- enum: `LLShaderMgr::TERRAIN_TEXTURE_TRANSFORMS` / `REGION_SCALE` (要 deploy 前 §6-A 再確認)
- setter: pipeline.cpp PBR terrain pass で per-region 更新

**cross-permutation check** (η-28-C 同型):
- HEIGHTMAP_WITH_NOISE permutation: `terrain_texture_transforms` のみ使用、`region_scale` は **declared (UBO 内) だが本体未参照** (declared-but-unused 容認、glslang strict mode で warning レベル、parse 通過)
- PBR_PAINTMAP permutation: 両 field 使用
- **両 permutation で同 UBO + 同 binding + 同 guard 名** = η-28-C 範式の同 file permutation 適用

**cross-stage check**: F pair = `pbrterrainF.glsl` は η-21+ で `TerrainDetailUBO` set=1 binding=17 に UBO 化済 + 同 file 内に plain uniform 無し → 編集不要。

### §2.5 binding 25: `PerProgramUBO_PointLightF` (**訂正: ≠ SunLightF**、class3/deferred/pointLightF.glsl)

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2c:
//   pointLightF.glsl (= "Deferred Light Shader" の F-stage 実 attach) の plain uniform を UBO 化。
//   sun_wash は dead-uniform (main() 未参照) だが parse 通過のため UBO 含める。
//   FrameViewProj 内 inv_proj/screen_res、FrameAtmosphere_Lighting 内 classic_mode、
//   PerDrawUBO_LightParams 内 color/size 既参照、二重宣言禁止。
//   V pair = pointLightV.glsl は η-23 で PerProgramUBO_PointLightV set=2 binding=5 に UBO 化済 → 別 descriptor。
#ifndef PER_PROGRAM_UBO_POINT_LIGHT_F_DEFINED
#define PER_PROGRAM_UBO_POINT_LIGHT_F_DEFINED 1
layout(set=2, binding=25, std140) uniform PerProgramUBO_PointLightF {
    vec4  viewport;                // offset 0,  size 16
    float sun_wash;                // offset 16, size 4 (dead uniform、host setter 経由するが本体未参照)
    float falloff;                 // offset 20, size 4
    float global_light_strength;   // offset 24, size 4
    float _pad0;                   // offset 28, size 4 (vec4 boundary 揃え)
};  // total 32
#endif
#else
uniform float sun_wash;
uniform float falloff;
uniform vec4 viewport;
uniform float global_light_strength;
#endif
```

**host C++ 整合**:
- reserved uniform 登録: llshadermgr.cpp:1537 (`viewport`)、L1657 (`sun_wash`)、L1895 (`global_light_strength`)
- `falloff` は llshadermgr.cpp 内 `LIGHT_FALLOFF` reserved enum 登録済 (要 deploy 前 §6-A 再確認)
- enum: `LLShaderMgr::VIEWPORT` / `DEFERRED_SUN_WASH` / `LIGHT_FALLOFF` / `GLOBAL_LIGHT_STRENGTH`
- setter: `pipeline.cpp::renderDeferredLighting` + light volume render pass

**sun_wash ステータス**: GLSL L43 で宣言、`main()` 本体未参照 (grep 確認、cofF の `max_cof` と異なり完全 dead)。host C++ は reserved enum 経由 setter あり (sky settings 連動)。UBO 含めるが GLSL 側未参照 (declared-but-unused) で glslang strict mode は warning レベル、parse 通過。

**cross-stage check**: V pair = `class3/deferred/pointLightV.glsl` は η-23 で `PerProgramUBO_PointLightV` (set=2 binding=5、center/size 用) に UBO 化済、本 binding 25 PointLightF 群 (`sun_wash` 等) 不使用 → 別 descriptor + 別 uniform 集合、編集不要。

**class2 sunLightF との関係** (§0.2 訂正の補足): `gDeferredLightProgram` は `pointLightV+pointLightF` を attach する program で **sunLightF.glsl は attach しない**。class2 sunLightF.glsl は別 program (`gDeferredSunProgram`) で使われ、`sun_dir` + `shadow_bias` のみ持ち本 phase scope 外。Phase 2c 末 cold launch で sunLightF 系 ERROR cascade が出たら Phase 2d/2e で対応 (本 phase では予防修正しない、scope shrink でなく scope 明示固定)。

---

## §3 期待 ERROR Δ + cascade reveal 観察計画

### Phase 2b 末 → Phase 2c 末 期待値 (cold launch log 実観測ベース)

| 観測項目 | Phase 2b 末 | Phase 2c 末 期待 | Δ |
|---|---|---|---|
| ERROR 行数 | 18 | **~10-12** | **-6 〜 -8** (5 root × 1.2-1.6 cascade ratio 程度の不確実性) |
| parse failure event 数 | 10 | ~4-6 | -4 〜 -6 |
| link failed | 0 維持 | 0 維持 | ZERO 継続 (11 sub-bundle 連続) |
| 5 target program SPIR-V 生成 | (parse fail) | 全成功 | +5 (pbrterrainV は permutation 2 と数えると +6) |
| pbrterrainV cross-permutation parse | 両 permutation parse fail | 両 permutation parse 成功 | +2 |
| clean shutdown | 維持 | 維持 | - |

**Δ レンジ幅 (-6 〜 -8) の理由**: Phase 2a で cascade reveal +2 観測 (PBR Alpha V + SpotLight F MULTI)、Phase 2b で cascade reveal 0 観測 (Δ clean -4)。Phase 2c は 5 root × cross-permutation 1 件 = 6 parse fail 解消が下限、上振れ 2 で cascade reveal 0 + Δ -6、Phase 2a 同型の cascade chain reveal で Δ -8 〜 -10 もあり得る (waterF / pointLightF / pbrterrainV は派生 cascade 候補)。

### Phase 2c 直接 ERROR 消滅対象 (cold launch log 行 → file 対応)

| log L | program | stage | 該当 ERROR (Phase 2b 末) | 該当 UBO (Phase 2c 末) |
|---|---|---|---|---|
| L197 | Water Shader | F (0x8b30) | `non-opaque uniforms outside a block` (blend_factor @ transformed dump 4079) | PerProgramUBO_WaterF (binding 23) |
| L710 | Deferred PBR Terrain Shader (HEIGHTMAP) | V (0x8b31) | `non-opaque uniforms outside a block` (terrain_texture_transforms @ transformed dump 1223) | PerProgramUBO_PbrTerrainV (binding 24) |
| L717 | Deferred PBR Terrain Shader (PAINTMAP) | V (0x8b31) | `non-opaque uniforms outside a block` (region_scale or terrain_texture_transforms @ transformed dump 1115) | PerProgramUBO_PbrTerrainV (binding 24、同 UBO 共有) |
| L746 | Deferred Light Shader | F (0x8b30) | `non-opaque uniforms outside a block` (sun_wash or viewport @ transformed dump 2287) | PerProgramUBO_PointLightF (binding 25) |
| L864 | Deferred Blur Light Shader | F (0x8b30) | `non-opaque uniforms outside a block` (dist_factor or blur_size @ transformed dump 1730) | PerProgramUBO_BlurLightF (binding 22) |
| L1294 | Deferred CoF Shader | F (0x8b30) | `non-opaque uniforms outside a block` (depth_cutoff etc @ transformed dump 1721) | PerProgramUBO_CofF (binding 21) |

### Phase 2d/2e 残予測 (Phase 2c 末で再評価、本 prep では scope 固定しない)

η-28 Phase 2b complete handoff §4 で予測した Phase 2c (cofF + blurLightF + waterF + pbrterrainV) を本 Phase 2c で消化 + 訂正版 pointLightF 含む = 5 file。Phase 2d (pbralpha cascade / spotLight MULTI cascade / 残 sunLightF) は Phase 2c 末 cold launch log の cascade reveal 状況を見て scope 再評価。本 prep ではまだ Phase 2d scope 固定せず (Phase 2c verify 後の handoff で詳細化)。

---

## §4 編集対象 file (literal 5 file、η-28-C preventive 0)

### 編集

1. `indra/newview/app_settings/shaders/class1/deferred/cofF.glsl`
   - L48-L53 (plain `uniform float depth_cutoff;` 他 6 行) を `#ifdef LL_VULKAN_GLSL` UBO block + `#else` plain で wrap
2. `indra/newview/app_settings/shaders/class1/deferred/blurLightF.glsl`
   - L56-L60 (plain `uniform float dist_factor;` 他 5 行) を `#ifdef LL_VULKAN_GLSL` UBO block + `#else` plain で wrap
3. `indra/newview/app_settings/shaders/class3/environment/waterF.glsl`
   - L94 (plain `uniform float blend_factor;`) + L147-L154 (plain `uniform vec3 lightDir;` 他 8 行) を `#ifdef LL_VULKAN_GLSL` UBO block + `#else` plain で wrap。2 ブロック分離 (L94 と L147-L154) を 1 UBO に統合、UBO block 配置位置は L147 群直上 (L94 の元位置は `#else` plain のみ残し)。または UBO block を L94 直上に置いて L147-L154 を `#ifdef LL_VULKAN_GLSL` 内で空にする (推奨: 後者、可読性のため。実装は §6-B self-verify で最終決定)。
4. `indra/newview/app_settings/shaders/class1/deferred/pbrterrainV.glsl`
   - L69-L71 (`#if TERRAIN_PAINT_TYPE == TERRAIN_PAINT_TYPE_PBR_PAINTMAP` + `uniform float region_scale;`) + L178 (`uniform vec4[5] terrain_texture_transforms;`) を **1 UBO に統合**、`#ifdef LL_VULKAN_GLSL` 側は両 field 常時含める (permutation 共有)、`#else` 側は permutation 条件保持 (`region_scale` は元の `#if TERRAIN_PAINT_TYPE` 内に残す、`terrain_texture_transforms` は無条件で残す)
5. `indra/newview/app_settings/shaders/class3/deferred/pointLightF.glsl`
   - L43 (`uniform float sun_wash;`) + L59 (`uniform float falloff;`) + L95 (`uniform vec4 viewport;`) + L128 (`uniform float global_light_strength;`) を `#ifdef LL_VULKAN_GLSL` 内 1 UBO に統合、`#else` 側は 4 plain 維持

### 非編集 (cross-stage / cross-variant check で確認済)

- `class1/deferred/postDeferredNoTCV.glsl` (cofF / blurLightF / pointLightF / pbrterrainV 全て V 側 pair としては不使用)
- `class1/deferred/blurLightV.glsl` (blur uniform 群不使用)
- `class3/deferred/pointLightV.glsl` (η-23 で `PerProgramUBO_PointLightV` set=2 binding=5 既 UBO 化、本 phase uniform 群不所持)
- `class1/environment/waterV.glsl` (η-6 で `WaterVParamUBO_Legacy` set=3 binding=60 既 UBO 化)
- `class1/environment/waterF.glsl` (error fallback stub、plain uniform 無し)
- `class3/environment/underWaterF.glsl` (plain uniform 群全て `#ifndef LL_VULKAN_GLSL` guard 内)
- `class1/deferred/pbrterrainF.glsl` (η-21+ で `TerrainDetailUBO` set=1 binding=17 既 UBO 化、plain uniform 無し)
- `class2/deferred/sunLightF.glsl` (本 phase scope 外、§0.2 訂正の補足、Phase 2d 以降判断)

---

## §5 reference-shader-location-map.md §6-A 更新計画 (本 commit 同梱)

Phase 2c feat commit と同 commit (または直後 docs commit) で `reference-shader-location-map.md` §6-A 表に以下 5 行追加:

| set | binding | UBO 名 | stage | 起源 sub-step |
|---|---|---|---|---|
| 2 | 21 | PerProgramUBO_CofF | F | η-28 Phase 2c |
| 2 | 22 | PerProgramUBO_BlurLightF | F | η-28 Phase 2c |
| 2 | 23 | PerProgramUBO_WaterF | F | η-28 Phase 2c |
| 2 | 24 | PerProgramUBO_PbrTerrainV | V (heightmap + paintmap permutation 共有) | η-28 Phase 2c |
| 2 | 25 | PerProgramUBO_PointLightF | F | η-28 Phase 2c |

「(空き、η-28 Phase 2c+ 連番継続)」行 (現 binding 21+) を 26+ へ繰り下げ。

stage 列の binding 24 は `V (permutation 共有)` 注釈付き、reference doc §6 命名規約セクションに **η-28-C 範式の同 file permutation 適用例** として追記提案 (Phase 2c complete handoff §7 で範式 doc 本体反映)。

---

## §6 Phase 2c 着手前 self-verify (Claude 側、AYA "OK" 後の deploy 前)

### §6-A reserved uniform 名整合 (host C++ vs GLSL)

deploy 前に Claude が再確認 (本 prep §2.x の host 整合記述の裏取り):

| GLSL 名 | LLShaderMgr enum (要 grep) | reserved push_back 場所 ✓ | host C++ setter 場所 (要 grep) | Phase 2c deploy 前確認 |
|---|---|---|---|---|
| `depth_cutoff` | DEFERRED_DEPTH_CUTOFF | llshadermgr.cpp:1675 ✓ | pipeline.cpp DoF gather (要再 grep) | **deploy 前 grep** |
| `norm_cutoff` | DEFERRED_NORM_CUTOFF | llshadermgr.cpp:1676 ✓ | pipeline.cpp DoF gather (要再 grep) | **deploy 前 grep** |
| `focal_distance` | DOF_FOCAL_DISTANCE | llshadermgr.cpp:1699 ✓ | pipeline.cpp DoF gather (要再 grep) | **deploy 前 grep** |
| `blur_constant` | DOF_BLUR_CONSTANT | llshadermgr.cpp:1700 ✓ | pipeline.cpp DoF gather (要再 grep) | **deploy 前 grep** |
| `tan_pixel_angle` | DOF_TAN_PIXEL_ANGLE | llshadermgr.cpp:1701 ✓ | pipeline.cpp DoF gather (要再 grep) | **deploy 前 grep** |
| `magnification` | DOF_MAGNIFICATION | llshadermgr.cpp:1702 ✓ | pipeline.cpp DoF gather (要再 grep) | **deploy 前 grep** |
| `blur_size` | DEFERRED_BLUR_SIZE | llshadermgr.cpp:1659 ✓ | pipeline.cpp renderDeferredLighting (要再 grep) | **deploy 前 grep** |
| `dist_factor` | (LLStaticHashedString sDistFactor) | pipeline.cpp:387 (推定、要再 grep) | pipeline.cpp renderDeferredLighting (要再 grep) | **deploy 前 grep** |
| `delta` | (LLStaticHashedString sDelta) | pipeline.cpp:387 (推定、要再 grep) | pipeline.cpp renderDeferredLighting (要再 grep) | **deploy 前 grep** |
| `kern` | (LLStaticHashedString sKern) | pipeline.cpp:388 (推定、要再 grep) | pipeline.cpp renderDeferredLighting (要再 grep) | **deploy 前 grep** |
| `kern_scale` | (LLStaticHashedString sKernScale 推定) | pipeline.cpp (要再 grep) | pipeline.cpp renderDeferredLighting (要再 grep) | **deploy 前 grep** |
| `blend_factor` | BLEND_FACTOR | llshadermgr.cpp:1825 ✓ | LLSettingsVOWater::applyToShader (要再 grep) | **deploy 前 grep** |
| `lightDir` | LIGHT_DIRECTION 系 | llshadermgr.cpp:1749 ✓ | LLSettingsVOWater 系 (要再 grep) | **deploy 前 grep** |
| `specular` | (要 enum 名 grep) | (要 push_back 場所 grep) | (要 setter grep) | **deploy 前 grep** |
| `blurMultiplier` | WATER_BLUR_MULTIPLIER | llshadermgr.cpp:1762 ✓ | LLSettingsVOWater 系 (要再 grep) | **deploy 前 grep** |
| `refScale` | WATER_REF_SCALE | llshadermgr.cpp:1756 ✓ | LLSettingsVOWater 系 (要再 grep) | **deploy 前 grep** |
| `kd` | (要 enum 名 grep) | (要 push_back 場所 grep) | (要 setter grep) | **deploy 前 grep** |
| `normScale` | WATER_NORM_SCALE | llshadermgr.cpp:1759 ✓ | LLSettingsVOWater 系 (要再 grep) | **deploy 前 grep** |
| `fresnelScale` | WATER_FRESNEL_SCALE | llshadermgr.cpp:1760 ✓ | LLSettingsVOWater 系 (要再 grep) | **deploy 前 grep** |
| `fresnelOffset` | WATER_FRESNEL_OFFSET | llshadermgr.cpp:1761 ✓ | LLSettingsVOWater 系 (要再 grep) | **deploy 前 grep** |
| `terrain_texture_transforms` | TERRAIN_TEXTURE_TRANSFORMS | llshadermgr.cpp:1533 ✓ | pipeline.cpp PBR terrain pass (要再 grep) | **deploy 前 grep** |
| `region_scale` | REGION_SCALE | llshadermgr.cpp:1812 ✓ | pipeline.cpp PBR terrain pass (要再 grep) | **deploy 前 grep** |
| `sun_wash` | DEFERRED_SUN_WASH | llshadermgr.cpp:1657 ✓ | sky settings 連動 setter (要再 grep) | **deploy 前 grep** (GLSL 本体未参照 dead-uniform 確認済) |
| `falloff` | LIGHT_FALLOFF | (要 push_back 場所 grep) | light volume render pass (要再 grep) | **deploy 前 grep** |
| `viewport` | VIEWPORT | llshadermgr.cpp:1537 ✓ | pipeline.cpp render volume pass (要再 grep) | **deploy 前 grep** |
| `global_light_strength` | GLOBAL_LIGHT_STRENGTH | llshadermgr.cpp:1895 ✓ | sky settings 連動 setter (要再 grep) | **deploy 前 grep** |

deploy 前 grep 項目は AYA "OK" 後の commit 直前で Claude が 5-10 分で消化、`feedback_self_verify_before_handoff` 適用。

### §6-B 5 file edit 後の cross-check

deploy 前に Claude が再 read で以下確認:
- UBO ブロック宣言の closing `};` 直後の `#endif` (=`#ifndef PER_PROGRAM_UBO_*_DEFINED` の対) ✓
- 4 階層 nesting (`LL_VULKAN_GLSL` / `_DEFINED` guard / `layout` / closing) reference doc §6 命名規約セクションの shadowCubeV 範式と完全同型 ✓
- plain uniform 削除漏れなし (`#else` 側に moved 確認)
- file 全体での同名 uniform 再宣言なし (`PerProgramUBO_*` 名 grep で本 file のみ唯一宣言)
- **binding 23 waterF**: L94 `blend_factor` と L147-L154 群 を **1 UBO に統合**配置確認 (推奨配置: UBO block を L94 元位置直上、L147-L154 元位置は `#else` 側のみ残す)
- **binding 24 pbrterrainV**: `#ifdef LL_VULKAN_GLSL` 側で `region_scale` を **permutation 条件外** で UBO 含める、`#else` 側は元の `#if TERRAIN_PAINT_TYPE` 条件保持 (heightmap で declared-but-unused は許容)
- **binding 25 pointLightF**: L43 / L59 / L95 / L128 の 4 uniform を **1 UBO に統合**配置 (推奨: 既存 L43 sun_wash 直上に UBO block、L59 / L95 / L128 元位置は `#else` 側のみ残す)

### §6-C cold launch verify checklist (reference doc §8 cookbook の Phase 2c 起点)

```bash
# 期待値: ERROR 18 → ~10-12, link 0, clean shutdown
grep -cE "^ERROR: 0:" ~/.ayastorm_x64/logs/AYAstorm.log    # 期待 10-12
grep -c "glslang parse failed" ~/.ayastorm_x64/logs/AYAstorm.log  # 期待 4-6
grep -c "link failed\|link error" ~/.ayastorm_x64/logs/AYAstorm.log  # 期待 0
grep "Shutting down" ~/.ayastorm_x64/logs/AYAstorm.log   # 存在確認

# 5 target program 全 SPIR-V 生成成功 grep
grep -E "generatePerProgramSPIRV.*for program (Deferred CoF Shader|Deferred Blur Light Shader|Water Shader|Deferred PBR Terrain Shader|Deferred Light Shader)" ~/.ayastorm_x64/logs/AYAstorm.log

# pbrterrainV 2 permutation 確認 (HEIGHTMAP / PAINTMAP 両方 success)
grep -E "Deferred PBR Terrain Shader.*(HEIGHTMAP|PAINTMAP)" ~/.ayastorm_x64/logs/AYAstorm.log
```

### §6-D cascade reveal 観察 (Phase 2a 教訓: cascade event 数 +2 reveal あり、Phase 2b は 0)

Phase 2a は cascade reveal +2 / Phase 2b は 0 観測。Phase 2c 直接対象 5 file は **root ERROR 1 件ずつ / pbrterrainV のみ permutation 2 件**で総 root parse fail 6 件、Δ event 数の理論下限 -6 (cascade 0)。上振れで Phase 2a 同型の cascade chain reveal (waterF → underwater? pointLightF → spotLight? pbrterrainV → pbrterrainF?) が出る可能性は中程度。

Δ event = -6 (cascade 0) → 期待通り Phase 2d scope を waterHazeF cascade / spotLight MULTI 残 / pbralpha 系で再評価
Δ event = -8 / -10 (cascade +2 / +4 reveal) → reference doc §7 cascade chain 同定 protocol で新規 chain を trace、Phase 2d/2e scope 拡張

---

## §7 範式継承 + 本 phase 適用範式

### 継承 (Phase 2b 以前から)

- `feedback_one_step_at_a_time` (verify 1 ステップずつ)
- `feedback_no_scope_shrink` (literal 5 file scope = AYA 承認 scope、shrink 禁止)
- `feedback_doubt_self_first` (本 prep §0 で Phase 2b complete prep doc の literal 「waterV」「sunLightF」を疑い、stage_type 0x8b30/0x8b31 + transformed dump + llviewershadermgr.cpp attach map の 3 段 trace で訂正)
- `feedback_render_full_trace_first` (uniform を shader tree 全体 + host C++ + reserved uniform 表 で trace、推論禁止)
- `feedback_no_auto_commit` (AYA "OK" 明示後に feat commit)
- `feedback_self_verify_before_handoff` (本 prep §6 self-verify で AYA cold launch 浪費を防ぐ)
- `feedback_proactive_handoff` (本 prep doc 自体)
- **η-28-A** (dump marker 信用せず source tree grep) — 本 prep §2 全 file の uniform grep で適用
- **η-28-B** (既存 transformed dump で再 cold launch 不要判定) — Phase 2b 末 transformed dump で current state 確認、本 prep 起草前に再 cold launch せず
- **η-28-C** (program 内 cross-stage / cross-variant 共有 UBO) — 本 prep §1 cross-stage/variant 検査表で 1 件適用 (binding 24 PbrTerrainV 同 file permutation 共有)

### 本 phase 提案範式: **η-28-D** (prep doc literal を 3 段 trace で訂正)

**η-28-D 範式定式化** (本 prep §0 で実践、Phase 2c complete handoff で範式 doc 反映予定):

> 先 phase complete handoff の literal 記述 (file path / UBO 名 / stage / program attach 想定) を **本 phase prep 起草の初手で必ず 3 段 trace** で訂正検査:
> 1. **stage_type field 確認**: cold launch log の該当 root ERROR 行で `Shader compile error.*stage type=0x8b30/0x8b31` から F/V 判別
> 2. **transformed dump 対応 line 確認**: η-28-B 範式の既存 dump (cache/shader_cache/transformed/) で error position の line を grep、uniform 名 + 構造から source file 推定
> 3. **llviewershadermgr.cpp 実 attach map 確認**: `mShaderFiles.push_back` grep で program 名 → .glsl file 対応を確定 (program 名と file 名は一致しない場合多数、推定禁止)
>
> **適用条件**: 先 phase complete handoff で「次 phase scope = ...」と literal file path を列挙していて、cold launch verify 直前でない場合 (prep 起草段階で本 trace 必須)
>
> **背景**: Phase 2c で 2 件の訂正発見 (waterV→waterF + sunLightF→pointLightF) は cold launch verify 後だと 1 サイクル ~20 分の浪費。prep 起草段階で消化することで AYA 検証回数増を防ぐ (`feedback_self_verify_before_handoff` の具体化版)

### 本 phase 適用既存 η-28-C 範式

binding 24 PbrTerrainV の **同 file 内 permutation 共有** (HEIGHTMAP_WITH_NOISE / PBR_PAINTMAP) は η-28-C の 3 つ目の適用形 (η-28-C の type taxonomy):
- **type 1**: 同 program V/F pair 共有 (Phase 2a waterHazeF V+F)
- **type 2**: 同 program cvar-selected file variant 共有 (Phase 2b postDeferredF ↔ postDeferredHQDoFF)
- **type 3** (NEW): 同 file 内 preprocessor permutation 共有 (Phase 2c pbrterrainV HEIGHTMAP / PAINTMAP)

3 型とも本質は同じ ("同 UBO + 同 binding + 同 guard 名で複数地点を揃える")、適用対象が type 1=stage、type 2=cvar、type 3=preprocessor macro。範式 doc 本体 (reference-shader-location-map.md §6 末 + handoff Phase 2a complete §7) には Phase 2c complete handoff で 3 型 taxonomy を追記。

---

## §8 落穂拾い + Phase 2c 完了 checklist

- [ ] AYA prep doc レビュー + "OK" 明示
- [ ] §6-A reserved uniform setter 場所 deploy 前 grep 消化 (Claude 5-10 分)
- [ ] §4 の 5 file UBO 化 feat commit
- [ ] reference-shader-location-map.md §6-A 表に 5 行追加 (binding 21-25) — feat commit 同梱 or 直後 docs commit
- [ ] deploy + shader cache clear (~/.ayastorm_x64/cache/shader_cache/ 全 clear)
- [ ] AYA cold launch + log 採取 (`~/.ayastorm_x64/logs/AYAstorm.log`)
- [ ] Claude self-verify: ERROR ~10-12 / parse failure ~4-6 / link 0 / SPIR-V 5 program (+ pbrterrainV 2 permutation) 全成功 / clean shutdown (本 prep §6-C cookbook で確認)
- [ ] Phase 2c complete handoff 起草 (§4 Phase 2d 表 + 残予測表 を本 phase 末 log で更新、η-28-D 範式定式化 + η-28-C type taxonomy 3 型追記)
- [ ] **次々々 session**: Phase 2c complete handoff の §4 Phase 2d 表 (pbralpha cascade / spotLight MULTI 残 / sunLightF / waterHazeF cascade 残) を起点に Phase 2d prep 起草

---

## §9 reference link

- 前 handoff (η-28 Phase 2b complete): `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2b-complete.md` (commit `36f1b79304` で起草、Phase 2b 末 ERROR 18 / Δ -4)
- Phase 2b feat commit: `1e30d59c4e`
- Phase 2b prep commit: `c037f6f696`
- location/UBO map: `docs/specs/ayastorm-r41-gl-removal/reference-shader-location-map.md` (η-28 整備済、Phase 2c feat commit で §6-A 5 行追加予定)
- Phase 2b 末 cold launch log: `~/.ayastorm_x64/logs/AYAstorm.log` (3786 行、ERROR 18、本 prep §3 / §6 起点)
- Phase 2b 末 transformed dump 場所: `~/.ayastorm_x64/cache/shader_cache/transformed/` (η-28-B 範式で本 prep §0 / §2 起草に活用済)
- 関連 llviewershadermgr.cpp 行: L1009-1031 (Water Shader)、L1640-1655 (gDeferredPBRTerrainProgram)、L1722-1730 (gDeferredLightProgram = pointLightV+pointLightF)、L1854-1859 (Deferred Blur Light Shader)、L3078-3082 (Deferred CoF Shader)
- 関連 llshadermgr.cpp reserved uniform push_back 行: L1533 (terrain_texture_transforms)、L1537 (viewport)、L1657 (sun_wash)、L1659 (blur_size)、L1675-1676 (depth_cutoff / norm_cutoff)、L1699-1702 (DoF 4 件)、L1749 (lightDir)、L1756-1762 (water 5 件)、L1812 (region_scale)、L1825 (blend_factor)、L1895 (global_light_strength)

---

**本 prep doc は η-28 Phase 2c 着手前の source of truth**。§0 訂正 2 件を最優先に読み、Phase 2b complete handoff §3 / §4.1 literal は本 prep §0 で上書き済。AYA 承認 → §6-A deploy 前 grep → feat commit → cold launch verify → complete handoff 起草 の 1-session 構成。
