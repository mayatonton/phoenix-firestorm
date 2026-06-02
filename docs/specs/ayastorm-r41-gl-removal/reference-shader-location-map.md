# r41 Vulkan migration: shader interface location map (η-28 Phase 2c 末時点)

**目的**: GLSL `layout(location=N)` qualifier の **slot 識別子** としての確定割当を全 shader 横断で資料化。η-26+ の location reassign 着手時の **事前 trace 範式** (空き slot 即座確定) として参照する。

**前提知識** (η-25 末で AYA 質問に応えた整理):

- `location=N` は **stage 間で同名 varying / attribute を結びつける slot ID 番号**であり、**描画順序や優先度ではない**
- V→F pair で V `out` と F `in` が同 location を持てば接続成立、それだけ
- 制約: **同 stage 内で同番号 2 占有不可** = `overlapping use of location` parse error の正体
- 配列型は **連続 slot 占有**: vec3[4] @ 52 = 52-55 / float[4] @ 56 = 56-59 / vec4[10] @ 29 = 29-38
- Vulkan 最小保証 `maxVertexOutputComponents = 64` (= 16 slot × 4 component) → 60 番台まで全 GPU 安全使用域
- GL は driver auto-assign、Vulkan は shader 作者責任 (KHR_vulkan_glsl extension)

---

## §1 vertex attribute band (location=0〜18)

vertex stage `in` 側で **頂点バッファ binding と対応**。CPU side の `glVertexAttribPointer` / VBO layout と整合必要。R41 GL 移行ではここは GL 由来の固定 index を踏襲。

| location | type | varying / attribute | 適用 file 帯 | 備考 |
|---|---|---|---|---|
| 0 | vec3 | `position` | 全 V shader 共通 | 標準 vertex attribute |
| 0 | vec2 | `vary_texcoord0` (out) / `vary_uv` (in) | V/F 共通 | F stage `in` でも 0 を共有 (V stage attribute と F stage varying は **独立 namespace**) |
| 0 | vec4 | `frag_color` (out) | 全 F shader | F stage output (これも独立 namespace) |
| 1 | vec3 | `normal` | 全 V shader 共通 | 標準 vertex attribute |
| 2 | vec2 | `texcoord0` (V in) / `vertex_color` (V out / F in) | - | V attr 2 (texcoord0) vs varying 2 (vertex_color) は同 program 内で稀に共存、要注意 |
| 3 | vec2 | `texcoord1` | - | - |
| 4 | vec2 | `texcoord2` | - | - |
| 5 | vec2 | `texcoord3` | - | - |
| 6 | vec4 | `diffuse_color` (attribute) | alpha 系 V shader 多用 | |
| 7 | vec4 | `emissive` | - | - |
| 8 | vec4 | `tangent` | - | - |
| 9 | int | indices (skinning 系) | - | - |
| 10 | vec4 | weights (skinning 系) | - | - |
| 11-12 | - | (未使用) | - | |
| 13 | int | `texture_index` (indexed) | indexedTextureV.glsl | |
| 14-17 | - | (未使用 or 細部) | - | |
| 18 | int | `vary_texture_index` (flat out) | indexedTextureV.glsl | |
| 19 | - | (未使用、η-18 § buffer 確保) | - | |

---

## §2 varying band: legacy 占有帯 (location=20〜30) ⚠

**性質**: 多数の shader family が同番号を **互いに知らずに共有** している legacy 帯。同一 program 内で 2 つが衝突すると `overlapping use of location` error → η-18 / η-24 / η-25 で **50-60 帯への reassign** が継続中。

| location | varying (全 family 列挙) | 衝突 risk |
|---|---|---|
| **20** | `vary_AdditiveColor` (atmospherics) / `vertex_position` / `vertex_emissive` / `vary_mat0` / `vary_dir` / `tc` / `tc0` / `vary_tc` / `vary_uv` / `screenpos` / `base_color_uv` / `normal_texcoord` / `normal_g` / `pos_w` | **最重 14 種、η-24 で alpha 系 51 / pbrterrain 系 50 へ救出済、η-25 で trans_center 60 へ救出済** |
| **21** | `vary_AtmosAttenuation` (atmospherics) / `vary_mat1` / `emissive_uv` / `metallic_roughness_texcoord` / `tangent_g[]` | **η-24 で pbrterrain vary_tangents 52 へ救出済 (case X)、η-27 Phase 1e-C で pbralpha metallic_roughness_texcoord 40 へ救出済** |
| **22** | `vary_CloudDensity` / `vary_mat2` / `vary_rel_pos` / `normal_uv` / `view` | 5 種 |
| **23** | `vary_light_dir` / `altitude_blend_factor` / `metallic_roughness_uv` | 3 種 |
| **24** | `occlusion_uv` | 1 種 (旧 pbrterrain vary_signs、η-25 Phase 2 で 56 へ救出済) |
| **25** | (空き) | **η-25 Phase 2 で pbrterrain vary_signs 25 → 56 救出後、空き化** |
| **26** | `sun_fade` / `vary_pixcoord` / `camera_ray` / `pos` / `vary_CloudColorSun` / `vary_HazeColor` / `refCoord` | 7 種 (skybox/water 系密集) |
| **27** | `vary_LightNormPosDot` / `vary_CloudColorAmbient` / `littleWave` | 3 種 |
| **28** | `vary_texcoord` (F in、limited shader) | 1 種 |
| **29** | `vary_coords[2]` / `vary_coords[10]` (F in) | **vary_coords[10] = 29-38 slot 連続占有** |
| **30** | `vary_coords[2]` / `vary_coords[10]` (V out) | (29 の続き) |
| **31-38** | `vary_coords[10]` tail (該当 shader のみ) | 限定 occupation |
| **39** | `normal_texcoord` (η-27 Phase 1e-B で旧 20 から救出済、pbralphaV/F) | 1 種、§3 表参照 |
| **40** | `metallic_roughness_texcoord` (η-27 Phase 1e-C で旧 21 から救出済、pbralphaV/F) | 1 種、§3 表参照 |
| **41-49** | (空き) | **η-26+ の reassign 帯 candidate (49 まで 9 slot 空き)** |

**実運用 rule**: 20-30 帯は **新規 varying 追加禁止**、衝突発覚した既存 varying は **50-60 帯へ救出**。

---

## §3 varying band: r41 reassign 帯 (location=50〜60) = 救出域

**性質**: η-18 §3.1 範式類で確立した **legacy 帯 (20-30) からの救出先**。array slot 連続性に注意。

| location | type | varying | 救出元 | 救出 sub-step | V file | F file |
|---|---|---|---|---|---|---|
| **50** | vec3 | `vary_vertex_normal` | 旧 20 | η-24 Phase C | pbrterrainV.glsl | pbrterrainUtilF.glsl |
| **51** | vec3 | `vary_norm` | 旧 20 | η-24 Phase C | alphaV.glsl (class1/2) | alphaF.glsl (class1/2) |
| **52,53,54,55** | vec3[4] | `vary_tangents[4]` | 旧 21 (V) / 旧 20 (F) | η-24 case X (V) / η-25 Phase 2 (F) | pbrterrainV.glsl | pbrterrainF.glsl |
| **56,57,58,59** | float[4] | `vary_signs[4]` | 旧 25 (V) / 旧 24 (F) | η-25 Phase 2 | pbrterrainV.glsl | pbrterrainF.glsl |
| **60** | vec3 | `trans_center` | 旧 20 (V+F+F) | η-25 Phase 1d (cascade) | pointLightV.glsl | pointLightF.glsl / spotLightF.glsl |
| **61-63** | - | (空き) | - | - | - | - |

**η-27 Phase 1e-B / 1e-C (39-49 空き帯への救出、§2 で予告済 candidate 帯使用例)**:

| location | type | varying | 救出元 | 救出 sub-step | V file | F file |
|---|---|---|---|---|---|---|
| **39** | vec2 | `normal_texcoord` | 旧 20 (atmospherics vary_AdditiveColor と衝突) | η-27 Phase 1e-B | class1/deferred/pbralphaV.glsl | class2/deferred/pbralphaF.glsl |
| **40** | vec2 | `metallic_roughness_texcoord` | 旧 21 (atmospherics vary_AtmosAttenuation と衝突) | η-27 Phase 1e-C | class1/deferred/pbralphaV.glsl | class2/deferred/pbralphaF.glsl |

**実運用 rule**:
- **array varying 配置時は tail を確実に占有**: vec3[4] @ 52 = 52-55 全部消費 = 53/54/55 を別 varying に割り当てない
- **V↔F pair 同期 reassign 必須**: V 側のみ移動して F 側を旧 location に残すと SPIR-V link 失敗
- **新規 reassign 時は本 §3 表を先に更新してから edit**

---

## §4 Vulkan 制約と GPU 上限

| 制約 | 最小保証 | 実装現状 (η-25 末) | 余裕 |
|---|---|---|---|
| `maxVertexOutputComponents` | 64 (= 16 slot × vec4) | 最大 60 番台使用 | 4 slot |
| `maxFragmentInputComponents` | 64 | 最大 60 番台使用 | 4 slot |
| `maxVertexInputAttributes` | 16 | location 0-18 帯 | 余裕 |

**判定**: 現状 60 番台まで安全使用域。η-26+ で reassign 必要時は **§3 の 61-63 と §2 の 39-49 帯** から選定可能。

---

## §4-B std140 alignment 既知パターン集 (η-28 整備)

**目的**: Phase 2a で waterHazeV / shadowCubeV / visualizeBuffersF の UBO 設計時に都度確認した std140 規則を表化。Phase 2b 以降 (特に cofF の 6 uniform float 群 / pbrterrainV の vec4[5] / blurLightF の `kern[4]`) で boilerplate を即流用可能にする。

### 基本原則

- **base alignment** = 各 member の自然 alignment を 16 byte 倍数に切上げ (scalar / vec2 / vec3 / vec4 で挙動異なる)
- **offset** = 直前 member の offset + size、ただし次 member の base alignment 倍数に切上げ
- **block 全体 size** = 末尾 padding 込みで 16 byte 倍数

### scalar / vec 系

| GLSL 型 | size | base align | 占有 byte (単独配置時) | 備考 |
|---|---|---|---|---|
| `int` / `uint` / `float` | 4 | **16** (std140 で 4→16 切上げ) | 16 | size 4 + 12 pad、明示 pad 推奨 |
| `bool` | 4 | 16 | 16 | UBO で bool 直書きは非推奨、int で代替 |
| `vec2` | 8 | 8 | 16 (単独配置時) | vec2+vec2 連続なら 16 でちょうど消費 |
| `vec3` | 12 | **16** | 16 (size 12 + 4 pad) | **罠**: vec3 直後に float を置くと float が 16 align で 16 byte 占有、合計 32 |
| `vec4` | 16 | 16 | 16 | 最も素直 |
| `mat3` | 48 | 16 | 48 (3 × vec4) | **罠**: 内部は vec3[3] でなく vec4[3] 配置、host upload も pad 付き |
| `mat4` | 64 | 16 | 64 (4 × vec4) | 素直 |

### array 系 (std140 最大の罠)

| GLSL 型 | element size | array stride | 占有 byte | 備考 |
|---|---|---|---|---|
| `float[N]` / `int[N]` / `uint[N]` | 4 | **16** | 16N | **罠**: stride 4 でなく 16、N=5 で 80 byte 占有 |
| `vec2[N]` | 8 | **16** | 16N | stride 16 (各要素 vec4 配置、内部 8 data + 8 pad) |
| `vec3[N]` | 12 | **16** | 16N | stride 16 (各要素 vec4 配置、内部 12 data + 4 pad) |
| `vec4[N]` | 16 | 16 | 16N | 素直 |
| `mat4[N]` | 64 | 64 | 64N | 素直 |

### 実例 (Phase 2a 採用パターン、source: η-28 commit `c53f6e0782`)

**case 1: vec3 + vec3 (shadowCubeV、明示 pad)**

```glsl
#ifndef PER_PROGRAM_UBO_SHADOW_CUBE_V_DEFINED
#define PER_PROGRAM_UBO_SHADOW_CUBE_V_DEFINED 1
layout(set=2, binding=14, std140) uniform PerProgramUBO_ShadowCubeV {
    vec3  box_center;          // offset 0
    float _pad_shadowcube0;    // offset 12
    vec3  box_size;            // offset 16
    float _pad_shadowcube1;    // offset 28
};  // total 32
#endif
```

**case 2: int + pad×3 (waterHazeV)**

```glsl
layout(set=2, binding=15, std140) uniform PerProgramUBO_WaterHazeV {
    int above_water;           // offset 0, size 4 + 12 pad = 16
};  // total 16
```

### 危険な縮め書き vs 安全 explicit pad

**危険** (host C++ 側 layout 確認漏れで stride 誤認しやすい):

```glsl
uniform Block {
    vec3 a;
    float b;
    vec3 c;
};
```

**安全** (explicit pad、host 側 layout 疑念ゼロ):

```glsl
uniform Block {
    vec3  a;        // offset 0
    float _pad_a;   // offset 12
    float b;        // offset 16
    float _pad_b0, _pad_b1, _pad_b2;  // offset 20-31
    vec3  c;        // offset 32
    float _pad_c;   // offset 44
};  // total 48
```

### Phase 2b/c 直接必要な事前計算 (Phase 2b 着手時の起点)

| 想定 program | 構成 | 想定 size | 備考 |
|---|---|---|---|
| velocityAlphaV | `mat4 last_object_matrix` | 64 | mat4 単独、素直 |
| postDeferredHQDoFF | `float res_scale + float chroma_str` 隣接 | 32 (16+16) | float 単独は 16 占有 |
| cofF (6 uniform) | `float` × 6 (depth_cutoff / norm_cutoff / focal_distance / blur_constant / tan_pixel_angle / magnification) | 96 (6×16) | float[6] でなく個別宣言推奨 (host 側 uniform 名保持) |
| blurLightF | `float dist_factor + float blur_size + vec3 delta + float kern[4] + float kern_scale` | 16+16+16+64+16 = 128 | `kern[4]` は **stride 16** で 64 byte (罠) |
| pbrterrainV | `vec4 terrain_texture_transforms[5] + (region_scale 等)` | 80 + α | vec4[5] は素直 80 |

UBO 設計時は本 §4-B を参照 → host C++ 側 `glBufferData` size と member offset を整合確認。

---

## §5 η-26+ 着手前 trace チェックリスト

新規 location reassign 必要が発生したとき、本 doc を起点に以下を順次確認:

1. **衝突元 verify**: AYA cold launch log で `overlapping use of location N` をカテゴリ別 grep
2. **N の現使用 verify**: `Grep "layout\(location=N\)"` で全 file の `in`/`out` 確認 (本 doc §2 と整合確認)
3. **V↔F pair 全 file 列挙**: V side `out` と F side `in` の両方を file リストアップ (η-25 Phase 1d で pointLightV + pointLightF + spotLightF = 3 file の好例)
4. **救出先 slot 選定**: 本 doc §3 表で次の free slot を確認、array 型なら連続 N 個確保
5. **本 doc §3 表を edit してから shader edit**: 表更新先行で「事後 rationalization」を防ぐ (η-25 §3.4 / §3.5 教訓)
6. **3 file 以上の同期 reassign は単一 commit に閉じる**: V/F 間 mismatch 残置で第N+1層 cascade 発生
7. **cold launch verify**: ERROR Δ 表で `overlapping location` カテゴリ完封確認

---

## §6 既存 binding map (set/binding) との関係 (補足)

`layout(location=N)` (本 doc) と `layout(set=S, binding=B)` (UBO / sampler 用) は **完全独立 namespace**。同一番号でも衝突しない。UBO 側 binding map は本 §6 + handoff doc chain (η-3 §3 / η-23 §3 / η-24 §3 / η-25 §3.1 / η-28 Phase 2a §6-A) を参照。

### UBO 命名規約 + `#ifndef <NAME>_DEFINED` guard 範式 (η-28 整備)

#### 命名規約 (set 単位の prefix で寿命と所在を表現)

| prefix / suffix | set | 寿命 (= 更新頻度) | 例 | 備考 |
|---|---|---|---|---|
| `Frame<Name>` | 0 | **per-frame** (frame 開始時 1 回 upload) | `FrameViewProj` / `FrameLights` / `FrameAtmosphere_Lighting` | η-1〜η-5 期 backbone |
| (sampler は名前 free、UBO のみ `MaterialUBO`) | 1 | **per-program / per-draw material** | `MaterialUBO` (binding=0) / `diffuseMap` 等 sampler | η-6+ 期 material 抽象 |
| `PerDrawUBO_<Name>` | 2 | **per-draw** (描画 call 毎 upload) | `PerDrawUBO_LightParams` / `PerDrawUBO_MultiLight` / `PerDrawUBO_ClipPlane` | mTransform / 描画 call 単位の動的値 |
| `PerProgramUBO_<Name><Stage>` | 2 | **per-program** (program bind 時 1 回 upload) | `PerProgramUBO_FxaaF` / `PerProgramUBO_ShadowCubeV` / `PerProgramUBO_WaterHazeV` (V+F 共有) | `<Stage>` suffix (V/F) は **起源 stage** を示すだけで attach 範囲ではない (実 attach は §6-A stage 列で確認) |
| `<Name>UBO_Legacy` | 3 | **legacy set=3 帯** (η-6 / η-13 期由来、program 単位の旧 grouping) | `WaterFogUBO_Legacy` / `OcclusionCubeVParamUBO_Legacy` / `DeferredUtilParamUBO_Legacy` | set=2 PerProgramUBO への移行候補は個別判断 (§6-E 参照) |

#### `#ifndef <NAME>_DEFINED` / `#define ... 1` 多重定義防止 pattern

複数 shader file が同一 UBO ブロックを include 経由で重複宣言する状況 (e.g., atmosphericsFuncs.glsl が複数 file から include される) を防ぐため、全 UBO 宣言は guard で挟む。guard 名は **UBO 名を SHOUT_SNAKE_CASE に変換 + `_DEFINED` suffix** が範式 (η-3 PerDrawUBO_LightParams で確立)。

| UBO 名 | guard 名 (範式) |
|---|---|
| `FrameViewProj` | `FRAME_VIEW_PROJ_DEFINED` |
| `FrameLights` | `FRAME_LIGHTS_DEFINED` |
| `FrameAtmosphere_Lighting` | `FRAME_ATMOSPHERE_LIGHTING_DEFINED` |
| `PerDrawUBO_LightParams` | `PER_DRAW_UBO_LIGHT_PARAMS_DEFINED` |
| `PerProgramUBO_ShadowCubeV` | `PER_PROGRAM_UBO_SHADOW_CUBE_V_DEFINED` |
| `PerProgramUBO_WaterHazeV` (V+F 共有) | `PER_PROGRAM_UBO_WATER_HAZE_V_DEFINED` (V/F 両 file で同一 guard) |
| `WaterFogUBO_Legacy` | `WATER_FOG_UBO_LEGACY_DEFINED` |
| `AtmoExtraUBO_Legacy` | `ATMO_EXTRA_UBO_LEGACY_DEFINED` |

#### 新規 UBO 追加 boilerplate (η-28 Phase 2a shadowCubeV.glsl 実例)

```glsl
#ifdef LL_VULKAN_GLSL
// (任意の解説 comment、η-X 範式 reference)
#ifndef PER_PROGRAM_UBO_SHADOW_CUBE_V_DEFINED
#define PER_PROGRAM_UBO_SHADOW_CUBE_V_DEFINED 1
layout(set=2, binding=14, std140) uniform PerProgramUBO_ShadowCubeV {
    vec3  box_center;
    float _pad_shadowcube0;
    vec3  box_size;
    float _pad_shadowcube1;
};
#endif
#else
uniform vec3 box_center;
uniform vec3 box_size;
#endif
```

**4 階層 nesting** の意味:
1. `#ifdef LL_VULKAN_GLSL` = Vulkan binding 表記が使える環境のみ UBO 宣言、GL は plain uniform fallback (`#else` 側)
2. `#ifndef ..._DEFINED` = include 経由の重複宣言防止
3. `#define ..._DEFINED 1` = 自身を多重定義防止 marker として登録
4. `layout(set, binding) uniform` = UBO 本体

#### V+F 両 stage 共有宣言 (η-28-C 範式) との関係

waterHazeV / waterHazeF 両 file に **同一 UBO 名 + 同一 binding + 同一 guard 名** を宣言する場合、Vulkan は両 stage で **descriptor 1 個共有** で扱い、host 側は 1 回 bind で両 stage 参照可能。guard は stage 毎に独立 `#define` が立つので衝突なし。詳細は handoff doc §2 (発見経緯) + §7 (範式定式化) 参照、ここでは再定義しない。

---

### §6-A set=2 = `PerProgramUBO_*` / `PerDrawUBO_*` 帯 (η-28 Phase 2c 末時点)

**stage 列の読み方** (η-28 Phase 2a 新規追加、Phase 2b/2c で type 拡張):
- `V` / `F` = UBO ブロック宣言が **その stage 単独** に閉じている
- `V+F` = **同一 program の V/F 両 stage** に同名 UBO ブロックを宣言し共有 (η-28-C type 1: stage 跨ぎ)
- `F (+ <variant> cross-variant)` = **同一 program で cvar 切替される 2 file** に同名 UBO 宣言 (η-28-C type 2: cvar variant 跨ぎ)
- `V (permutation 共有)` = **同一 file 内 preprocessor permutation 2 種** に同 UBO 宣言 (η-28-C type 3: preprocessor 跨ぎ)
- `(per-draw)` = 描画 call 毎 host 側更新、stage は文脈依存
- UBO 名末尾の `V` / `F` suffix は **起源 stage** を示すだけ、実 attach 範囲は本列で確認

| set | binding | UBO 名 | stage | 起源 sub-step |
|---|---|---|---|---|
| 2 | 0 | PerDrawUBO_LightParams | (per-draw) | η-3 |
| 2 | 1 | PerDrawUBO_MultiLight | (per-draw) | η-23 |
| 2 | 2 | PerProgramUBO_GammaCorrect | F | η-24 |
| 2 | 3 | PerProgramUBO_AlphaParams | F | η-25 Phase 1a |
| 2 | 4 | PerProgramUBO_ColorGrading | F | η-25 Phase 1b |
| 2 | 5 | PerProgramUBO_PointLightV | V | η-25 Phase 1c |
| 2 | 6 | PerProgramUBO_ShadowAlphaMaskV | V | η-26 Phase 1a |
| 2 | 7 | PerProgramUBO_PostDeferredV | V | η-26 Phase 1b |
| 2 | 8 | PerProgramUBO_FullbrightShinyV | V | η-26 Phase 1c |
| 2 | 9 | PerProgramUBO_FxaaF | F | η-27 Phase 1a |
| 2 | 10 | PerProgramUBO_SpotLightF | F | η-27 Phase 1d |
| 2 | 11 | PerProgramUBO_PbrAlphaV | V | η-27 Phase 1c |
| 2 | 12 | PerProgramUBO_PostDeferredNoDoFF | F | η-27 Phase 1b |
| 2 | 13 | PerProgramUBO_FsObjectIdF | F | η-28 Phase 2a |
| 2 | 14 | PerProgramUBO_ShadowCubeV | V | η-28 Phase 2a |
| 2 | 15 | PerProgramUBO_WaterHazeV | **V+F** | η-28 Phase 2a |
| 2 | 16 | PerProgramUBO_VisualizeBuffersF | F | η-28 Phase 2a |
| 2 | 17 | PerProgramUBO_GodraysF | F | η-28 Phase 2b |
| 2 | 18 | PerProgramUBO_VolumetricLightF | F | η-28 Phase 2b |
| 2 | 19 | PerProgramUBO_VelocityAlphaV | V | η-28 Phase 2b |
| 2 | 20 | PerProgramUBO_PostDeferredF | F (+ HQDoFF cross-variant) | η-28 Phase 2b |
| 2 | 21 | PerProgramUBO_CofF | F | η-28 Phase 2c |
| 2 | 22 | PerProgramUBO_BlurLightF | F | η-28 Phase 2c |
| 2 | 23 | PerProgramUBO_WaterF | F | η-28 Phase 2c |
| 2 | 24 | PerProgramUBO_PbrTerrainV | V (permutation 共有) | η-28 Phase 2c |
| 2 | 25 | PerProgramUBO_PointLightF | F | η-28 Phase 2c |
| 2 | 26+ | (空き、η-28 Phase 2d+ 連番継続) | - | - |

**stage 列の retroactive 注意**: 既存 (binding 0-12) は UBO 名 suffix から `V` / `F` を推定記載。新規 cascade 表面化時に再確認推奨。`PerProgramUBO_WaterHazeV` (binding=15) は η-28 Phase 2a self-trace で V+F 両 stage attach が判明した実例 (waterHazeV / waterHazeF 両者に `above_water` 使用、waterHazeV 単独 UBO 化では F stage cascade error 浮上の見込みだったため両 stage 宣言で予防)。

### §6-B set=0 = `Frame*` 帯 + 一部 sampler (per-frame 寿命)

per-frame 更新の view/projection / lighting backbone + 一部 cubemap sampler。観測サンプル (η-1〜η-5 期実装 + 一部 sampler、source tree grep 由来、要全件棚卸し時は本表更新):

| binding | 名称 | 種別 | 主使用 | 備考 |
|---|---|---|---|---|
| 0 | `FrameViewProj` | UBO (std140) | 全 V shader (mat4 群) | η-1 backbone、最広域使用 |
| 1 | `FrameLights` | UBO (std140) | atmospherics 系 / pbralpha 系 / sunLight 等 | sun/moon/light 関連 |
| 2 | `FrameAtmosphere_Lighting` | UBO (std140) | atmospherics 系 / impostor / pbralpha 等 | atmosphere lighting params |
| 4 | `lightMap` | sampler2D | pbrmetallicroughnessF / pbralphaF | screen-space lighting |
| 6 | `environmentMap` | samplerCube | reflectionProbeF | env reflection |
| 7 | `reflectionProbes` | samplerCubeArray | reflectionProbeF / irradianceGenF | probe array |
| 17 | `glowNoiseMap` | sampler2D | glowExtractF | glow extract noise |

**注意**: set=0 は **per-frame UBO の専用帯ではない**。η-X 各期の判断で sampler も混在配置されている (binding 4/6/7/17 等)。新規 sampler 追加時は本表を update。Phase 2b 着手前準備として全件 sampler / UBO 棚卸しを推奨 (η-28 Phase 2a 末時点では本表が観測サンプル止まり)。

### §6-C set=1 = `MaterialUBO` (binding=0) + sampler 帯 (per-program material 寿命)

per-program material 系 + 主要 sampler。観測サンプル (η-6+ 期由来、要全件棚卸し):

| binding | 名称 | 種別 | 主使用 | 備考 |
|---|---|---|---|---|
| 0 | `MaterialUBO` / `MaterialUBO_Legacy` | UBO (std140) | bumpV / avatarV / pbralphaF / materialF / simpleNoColorV 等 | per-program material params (η-6 期、Legacy 派生あり) |
| 1 | `diffuseMap` | sampler2D | 全描画 path で広域使用 | base color |
| 2 | `normalMap` | sampler2D | pbrmetallicroughnessF | normal |
| 3 | `specularMap` | sampler2D | materialF / pbralphaF | PBR packed (occlusion/metal/roughness) |
| 4 | `diffuseRect` | sampler2D | exoVignetteF / screenSpaceReflPostF / snapshotFrameF | screen-space diffuse |
| 5 | `emissiveMap` | sampler2D | pbrmetallicroughnessF / pbralphaF | emissive |
| 6 | `bumpMap` | sampler2D | materialF / pbralphaF | bump |
| 8 | `metallicRoughnessMap` | sampler2D | pbrmetallicroughnessF | PBR metallic/roughness |
| 9 | `occlusionMap` | sampler2D | pbrmetallicroughnessF | PBR occlusion |
| 12 | `specularRect` | sampler2D | screenSpaceReflPostF | screen-space specular |
| 15 | `texture0` | sampler2D | bumpF | bump texture 0 |
| 16 | `texture1` | sampler2D | bumpF | bump texture 1 |
| 40 | `irradianceProbes` | samplerCubeArray | reflectionProbeF | irradiance probes |

**注意**: set=1 は **sampler 帯と UBO 帯の混在**。`MaterialUBO_Legacy` (materialF) は名前に `_Legacy` 付きだが **set=3 ではなく set=1 binding=0** に配置 (η-X 期の特殊判断、移行候補は個別)。

### §6-D set=3 = `<Name>UBO_Legacy` 帯 (η-6 / η-13 期由来、program 単位の旧 grouping)

η-6 / η-13 期 program 単位 grouping。set=2 PerProgramUBO への移行 (= 名前 + binding 体系統一) の **候補集** だが、`set=2` 帯 (binding 連番) と異なり **program 局所 binding を 0/1/5-7/9 等の散発的番号で確保**。観測 (η-28 Phase 2a 末 source tree grep):

| binding | UBO 名 | source .glsl | 起源 sub-step (推定) |
|---|---|---|---|
| 0 | AtmoExtraUBO_Legacy | atmosphericsFuncs.glsl | η-1〜η-4 |
| 1 | SkyVParamUBO_Legacy | class1/deferred/skyV.glsl | η-X |
| 5 | SoftenLightParamUBO_Legacy | class3/deferred/softenLightF.glsl | η-X |
| 6 | DeferredUtilParamUBO_Legacy | class1/deferred/deferredUtil.glsl | η-X |
| 7 | ShadowUtilParamUBO_Legacy | class1/deferred/shadowUtil.glsl | η-X |
| 9 | WaterFogUBO_Legacy | class1/environment/waterFogF.glsl | η-X |
| 17 | ReflectionProbeUBO_Legacy | class3/deferred/reflectionProbeF.glsl | η-X |
| 18 | GlowFParamUBO_Legacy | class1/effects/glowF.glsl | η-X |
| 19 | GlowVParamUBO_Legacy | class1/effects/glowV.glsl | η-X |
| 20 | GlowExtractFParamUBO_Legacy | class1/effects/glowExtractF.glsl | η-X |
| 25 | ExposureFParamUBO_Legacy | class1/deferred/exposureF.glsl | η-X |
| 26 | LuminanceFParamUBO_Legacy | class1/deferred/luminanceF.glsl | η-X |
| 28 | ScreenSpaceReflPostFParamUBO_Legacy | class3/deferred/screenSpaceReflPostF.glsl | η-X |
| 30 | SkinSSSPrototypeFParamUBO_Legacy | class1/deferred/skinSSSF.glsl | η-X |
| 32 | ClipFParamUBO_Legacy | class1/interface/clipF.glsl | η-X |
| 37 | NormaldebugVParamUBO_Legacy | class1/interface/normaldebugV.glsl | η-X |
| 38 | SnapshotFrameFParamUBO_Legacy | class1/post/snapshotFrameF.glsl | η-X |
| 40 | PreviewVParamUBO_Legacy | class1/objects/previewV.glsl | η-X |
| 41 | SimpleColorFParamUBO_Legacy | class1/objects/simpleColorF.glsl | η-X |
| 42 | StarsFParamUBO_Legacy | class1/deferred/starsF.glsl | η-X |
| 43 | SunDiscFParamUBO_Legacy | class1/deferred/sunDiscF.glsl | η-X |
| 44 | MoonFParamUBO_Legacy | class1/deferred/moonF.glsl | η-X |
| 46 | VignetteParamUBO_Legacy | class1/post/exoVignetteF.glsl | η-X |
| 47 | PathfindingVParamUBO_Legacy | class1/interface/pathfindingV.glsl | η-X |
| 48 | PathfindingNoNormalVParamUBO_Legacy | class1/interface/pathfindingNoNormalV.glsl | η-X |
| 49 | GlowCombineFParamUBO_Legacy | class1/interface/glowcombineF.glsl | η-X |
| 50 | OcclusionCubeVParamUBO_Legacy | class1/interface/occlusionCubeV.glsl | η-X |
| 51 | RadianceGenFParamUBO_Legacy | class1/interface/radianceGenF.glsl | η-X |
| 52 | IrradianceGenFParamUBO_Legacy | class2/interface/irradianceGenF.glsl | η-X |
| 58 | GaussianFParamUBO_Legacy | class1/interface/gaussianF.glsl | η-X |
| 60 | WaterVParamUBO_Legacy | class1/environment/waterV.glsl | η-15 (η-8 §3.4 範式継承) |
| 61 | TerrainVParamUBO_Legacy | class1/deferred/terrainV.glsl | η-15 (η-8 §3.4 範式継承) |

**注意**: 起源 sub-step (η-X) は Phase 2b 直前準備で `git log -S` 各 UBO 名で個別 trace 推奨 (本表は source tree 現状の grep 結果)。set=2 への移行判断は program 単位で個別 (= **必ずしも全 Legacy を移すべきとは限らない**、η-6/η-13 期の grouping が現代的に妥当な場合は as-is 維持)。

---

## §7 cascade ERROR 種別表 (log 解析 cookbook、η-28 整備)

**目的**: AYAstorm cold launch log の `ERROR: 0:<line>:` 系を読むときに、**根本 ERROR** (= UBO 化等の修正対象) と **cascade 派生 ERROR** (= 根本解消で連動消滅) を即判定。Phase 2b/c/d で頻繁に必要。

### 種別 → 意味 → 対処 → Phase 2a 観測例

| ERROR 文字列 | 意味 | 根本 vs cascade | 対処 | Phase 2a 観測例 |
|---|---|---|---|---|
| `non-opaque uniforms outside a block` | Vulkan で許容されない plain uniform 宣言 | **根本** | 該当 uniform を UBO 化 (本 doc §6-A / 4-B 参照) | L197 waterF blend_factor / L264 cofF depth_cutoff 等 |
| `missing #endif` | 直前の `#ifdef` 入子内で別 ERROR 発生 → parse 中断、結果として `#endif` 未到達 | **cascade** | 直前 ERROR が root cause、本 ERROR は表面化現象 | L689 (screen_res redefinition cascade で `#endif` 到達せず) / L694 PBR Alpha V |
| `undeclared identifier <name>` | include 経路の uniform / varying 宣言が parse 失敗で未到達 | **cascade** | 上流 file の parse error を解消、本 ERROR は連動消滅 | L696 PBR Alpha V `modelview_projection_matrix` / L834 SpotLight F `color` |
| `vector swizzle selection out of range` | `undeclared` で型不明変数を `.rgb` 等で swizzle した continuation | **cascade** | 直前 undeclared 解消で連動消滅 | L835 SpotLight F `color.rgb` |
| `compilation terminated` | parse 中断 sentinel (cascade chain の末端) | **cascade** (sentinel) | 直前 ERROR が root cause | L698 PBR Alpha V / L836 SpotLight F |
| `redefinition` | 同名 uniform / 変数が `#ifdef` gate 内外で 2 度宣言 | **根本** | gate logic 見直し、`#else` 側との二重宣言を排除 | L688 Skinned PBR Alpha F `screen_res` |
| `overlapping use of location N` | 同 stage 内で `layout(location=N)` を 2 占有 | **根本** | 本 doc §3 表で free slot 確認 → 救出 reassign | (η-25 Phase 2 等で頻発、η-28 では 0 件) |

### cascade chain 同定 protocol (log 行列読み)

1. **`grep -nE "ERROR: 0:[0-9]+:" log.txt`** で全 ERROR を行番号付きで列挙
2. **連続行 = 同一 program 内 cascade chain**: e.g., L688 (redefinition) → L689 (missing #endif) → L696 (undeclared) → L698 (terminated) は 1 chain
3. **行番号間が空く = 別 program (= 別 SPIR-V 生成試行)**: e.g., L264 と L694 は別 program
4. **chain 末端 = `compilation terminated`** が sentinel として現れる
5. **根本判定**: chain 先頭の `redefinition` / `non-opaque uniforms outside a block` / `overlapping use of location` が root cause

### Phase 2a で実観測した cascade chain (2 件、§3 で再掲)

| program | chain |
|---|---|
| Deferred PBR Alpha V | `redefinition` (η-27 で Skinned 変種のみ表面化していた) → `missing #endif` → `undeclared modelview_projection_matrix` → `compilation terminated` |
| Deferred SpotLight F | `redefinition` (η-27 で MULTI 変種のみ表面化していた) → `undeclared color` → `vector swizzle selection out of range (.rgb)` → `compilation terminated` |

両者とも Phase 2d (pbralphaF binding=25 / spotLightF MULTI 分岐) 実装で一括消滅見込み (cascade 派生 4+6 件 ≒ 計 10 件と handoff doc §3 で同定済)。

### Vulkan / fatal 系 (本 doc 範囲外、別軸)

| ERROR | 意味 | 対処 |
|---|---|---|
| `VK_ERROR_*` | Vulkan API 呼出エラー | host C++ 側調査 (validation layer / driver log) |
| `link failed` / `link error` | SPIR-V link 失敗 (descriptor / location mismatch 等) | 本 doc §3 (location) / §6-A-E (binding) で整合確認 |
| `fatal` / `panic` / `abort` / `signal SIG*` | viewer crash | gdb / core dump、shader parse 範囲外 |

---

## §8 log 観測 checklist (verify cookbook、η-28 整備)

**目的**: 各 Phase 完了時の **self-verify cookbook**。AYA に verify 依頼する前に Claude が確認すべき項目を 1 箇所集約 (`feedback_self_verify_before_handoff` の手順化)。

### 基本観測 (各 phase 完了後の self-verify、毎回必須)

```bash
# parse failure event 数 (Δ 期待値と比較)
grep -c "glslang parse failed" ~/.ayastorm_x64/logs/AYAstorm.log

# ERROR 行数 (Δ 期待値と比較、cascade reveal/消滅で event 数とずれる場合あり)
grep -cE "^ERROR: 0:" ~/.ayastorm_x64/logs/AYAstorm.log

# link 失敗確認 (η-3 以降 0 維持期待、η-28 Phase 2a 末で 9 sub-bundle 連続 ZERO)
grep -c "link failed\|link error" ~/.ayastorm_x64/logs/AYAstorm.log

# clean shutdown 確認 (異常終了時は出ない)
grep "Shutting down" ~/.ayastorm_x64/logs/AYAstorm.log
```

### target program SPIR-V 生成成功確認

```bash
# 各 Phase の target program 名を一覧して SPIR-V 生成 OK 確認
grep -E "generatePerProgramSPIRV.*for program <PROGRAM_NAME>" ~/.ayastorm_x64/logs/AYAstorm.log

# 例 (η-28 Phase 2a):
#   "FS Object ID Shader" / "Water Haze Shader" / "Deferred Shadow Cube Shader" / "Deferred Buffer Visualization Shader"
```

### cascade chain 解析

```bash
# 全 ERROR を行番号付きで列挙、連続行=同一 program cascade chain
grep -nE "ERROR: 0:[0-9]+:" ~/.ayastorm_x64/logs/AYAstorm.log

# 行番号付近の文脈確認 (前後 5 行で program 名 / chain 範囲特定)
grep -nE "ERROR: 0:[0-9]+:" ~/.ayastorm_x64/logs/AYAstorm.log | head -20
```

### transformed dump 同定 (UBO 化前後の実 GLSL 確認)

```bash
# program 名 → UUID + stage 取得
grep "dumpTransformedStageSource.*<PROGRAM_NAME>" ~/.ayastorm_x64/logs/AYAstorm.log

# dump file 直 read (UUID と stage が判明したら)
# ~/.ayastorm_x64/cache/shader_cache/transformed/<UUID>_<stage>.glsl
```

### fatal / Vulkan API 系除外確認

```bash
# 致命系が混入していないか確認 (混入なら本 phase の修正と関係ない別軸の問題)
grep -E "VK_ERROR|fatal|panic|abort|signal SIG" ~/.ayastorm_x64/logs/AYAstorm.log
```

### Phase 完了時 self-verify 6 step (handoff doc 起草前に毎回実施)

1. **parse failure event 数の Δ 確認**: prep doc 予想値と一致するか (一致 = 修正範囲適切 / 不一致 = cascade reveal or 上流影響)
2. **ERROR 行数の Δ 確認**: prep doc 予想値と一致するか (event 数とずれる場合 = cascade reveal/消滅の偶然相殺、§7 cascade chain 解析で同定)
3. **link failed = 0 維持確認**: η-3 以降の連続 ZERO sub-bundle 数を更新
4. **target program SPIR-V 生成成功 grep**: 各 Phase target 4 program 全てで SPIR-V OK 確認
5. **clean shutdown 維持確認**: `Shutting down` 行存在 (Vulkan validation panic 等で異常終了なら 0)
6. **cascade reveal の新規 event 同定**: §7 cascade chain protocol で新規表面化 program を特定、後続 Phase 配分に反映

### handoff doc §1 結果サマリ表のテンプレ (本 §8 checklist と対応)

| 観測項目 | 期待 | 実測 | 判定 |
|---|---|---|---|
| ERROR 行数 | (prep doc 値) | (cold launch log 実測) | ✓ / △ / ✗ |
| parse failure event 数 | (prep doc 値) | (cold launch log 実測) | ✓ / △ / ✗ |
| link failed | 0 維持 | (cold launch log 実測) | ✓ / △ |
| target program SPIR-V 生成 | 全成功 | (cold launch log 実測) | ✓ / △ |
| (予防修正対象、該当時) | cascade 出ない | (cold launch log 実測) | ✓ / △ |
| clean shutdown | 維持 | (cold launch log 実測) | ✓ / △ |

(η-28 Phase 2a handoff §1 が本テンプレの実例)

---

**本 doc は r41 GL 移行完遂まで永続資料 (live document)**。η-26+ 各 sub-bundle 完遂時に §3 表と §6-A〜E UBO binding 表を更新する。η-28 整備で §4-B (std140 alignment) / §6 命名規約 + guard / §6-B〜E (set=0/1/3) / §7 (cascade ERROR) / §8 (log checklist) を追加、Phase 2b/c/d の trace / verify は本 doc を起点に実施可能。
