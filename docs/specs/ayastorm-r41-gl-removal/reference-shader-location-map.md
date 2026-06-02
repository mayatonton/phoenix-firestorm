# r41 Vulkan migration: shader interface location map (η-25 末時点)

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
| **21** | `vary_AtmosAttenuation` (atmospherics) / `vary_mat1` / `emissive_uv` / `metallic_roughness_texcoord` / `tangent_g[]` | **η-24 で pbrterrain vary_tangents 52 へ救出済 (case X)** |
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
| **40-49** | (空き) | **η-26+ の reassign 帯 candidate (49 まで 10 slot 空き)** |

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

**η-27 Phase 1e-B (39-49 空き帯への救出、§2 で予告済 candidate 帯使用例)**:

| location | type | varying | 救出元 | 救出 sub-step | V file | F file |
|---|---|---|---|---|---|---|
| **39** | vec2 | `normal_texcoord` | 旧 20 (atmospherics vary_AdditiveColor と衝突) | η-27 Phase 1e-B | class1/deferred/pbralphaV.glsl | class2/deferred/pbralphaF.glsl |

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

`layout(location=N)` (本 doc) と `layout(set=S, binding=B)` (UBO 用) は **完全独立 namespace**。同一番号でも衝突しない。UBO 側 binding map は handoff doc chain (η-3 §3 / η-23 §3 / η-24 §3 / η-25 §3.1) を参照。

η-27 末の UBO binding 占有 (set=2 namespace):

| set | binding | UBO 名 | 起源 sub-step |
|---|---|---|---|
| 2 | 0 | PerDrawUBO_LightParams | η-3 |
| 2 | 1 | PerDrawUBO_MultiLight | η-23 |
| 2 | 2 | PerProgramUBO_GammaCorrect | η-24 |
| 2 | 3 | PerProgramUBO_AlphaParams | η-25 Phase 1a |
| 2 | 4 | PerProgramUBO_ColorGrading | η-25 Phase 1b |
| 2 | 5 | PerProgramUBO_PointLightV | η-25 Phase 1c |
| 2 | 6 | PerProgramUBO_ShadowAlphaMaskV | η-26 Phase 1a |
| 2 | 7 | PerProgramUBO_PostDeferredV | η-26 Phase 1b |
| 2 | 8 | PerProgramUBO_FullbrightShinyV | η-26 Phase 1c |
| 2 | 9 | PerProgramUBO_FxaaF | η-27 Phase 1a |
| 2 | 10 | PerProgramUBO_SpotLightF | η-27 Phase 1d |
| 2 | 11 | PerProgramUBO_PbrAlphaV | η-27 Phase 1c |
| 2 | 12 | PerProgramUBO_PostDeferredNoDoFF | η-27 Phase 1b |
| 2 | 13+ | (空き、η-28+ 連番継続) | - |

---

**本 doc は r41 GL 移行完遂まで永続資料 (live document)**。η-26+ 各 sub-bundle 完遂時に §3 表と §6 UBO binding 表を更新する。
