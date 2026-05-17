# AYAstorm Deferred Shader Routing リファレンス

**作成日**: 2026-05-13 (r19 Translucency P0R3 で 2 時間の経路解析を完了 → 再解析回避のため永続記録)
**対象**: deferred 経路で「どのオブジェクトがどの fragment shader を踏むか」の確定マップ
**作成経緯**: r19 Translucency P0R3 で「葉/布/耳/肌の翻訳実装に必要な注入点」を確定するため、llvovolume.cpp / lldrawpool*.cpp / 各 GBuffer writer shader を上から下まで trace した結果

> **再利用方針**: shader / lit 計算系を触る前に **まず本書を参照**。経路が code 変更で動く可能性があるので、本書の年月日より新しい commit が llvovolume.cpp / lldrawpool*.cpp / GBuffer flag 系に入っていたら本書の更新要否を確認すること。

---

## 1. GBuffer flag → softenLightF 分岐の対応

`indra/llrender/llshadermgr.cpp:636-640` 定義:

```cpp
#define GBUFFER_FLAG_SKIP_ATMOS   0.0
#define GBUFFER_FLAG_HAS_ATMOS    0.34
#define GBUFFER_FLAG_HAS_PBR      0.67
#define GBUFFER_FLAG_HAS_HDRI     1.0
#define GET_GBUFFER_FLAG(data, flag)    (abs(data-flag)< 0.1)
```

`class3/deferred/softenLightF.glsl:167-209` 分岐:

| flag 値 | 条件 | 分岐 | 処理内容 |
|---|---|---|---|
| 0.67 | `HAS_PBR` | PBR 分岐 (line 167-190) | `pbrBaseLight()` で IBL + 太陽 lit |
| 1.0 | `HAS_HDRI` | HDRI 分岐 (line 191-196) | emissive をそのまま出力 (HDRI 空) |
| 0.0 | `SKIP_ATMOS` | SKIP_ATMOS 分岐 (line 197-208) | WL sky 用、emissive or baseColor を sRGB→linear + sky_hdr_scale 適用 |
| 0.34 | `HAS_ATMOS` | Legacy 分岐 (line 209-279) | irradiance + sun_contrib * baseColor、spec.a/envIntensity で gloss/legacy env 加算 |

---

## 2. GBuffer writer (= flag を書く shader) の確定リスト

`grep -n "GBUFFER_FLAG_HAS_" indra/newview/app_settings/shaders/` の結果から:

### HAS_PBR を書く writer (= softenLightF PBR 分岐に進む)

| Shader | 行 | 用途 |
|---|---|---|
| `class1/gltf/pbrmetallicroughnessF.glsl` | 246 | GLTF scene (gltfscenemanager) 経路 |
| `class1/deferred/pbropaqueF.glsl` | 117 | LLDrawPool::POOL_GLTF_PBR / POOL_GLTF_PBR_ALPHA_MASK (rigged + 非 rigged) |
| `class1/deferred/pbrterrainF.glsl` | 433 | PBR terrain |
| `class1/interface/occlusionF.glsl` | 33 | occlusion query (実画面に出ない) |

### HAS_ATMOS を書く writer (= softenLightF Legacy 分岐に進む)

| Shader | 行 | 用途 |
|---|---|---|
| `class3/deferred/materialF.glsl` | 432 | PASS_MATERIAL/SPECMAP/NORMMAP/NORMSPEC (gDeferredMaterialProgram[16]) |
| `class1/deferred/bumpF.glsl` | 67 | PASS_BUMP (legacy bumpmap preset、Spec/Normal map なし) |
| `class1/deferred/diffuseF.glsl` | 47 | PASS_SIMPLE 等 |
| `class1/deferred/diffuseIndexedF.glsl` | 51 | indexed simple |
| `class1/deferred/diffuseAlphaMaskF.glsl` | 58 | alpha mask |
| `class1/deferred/diffuseAlphaMaskNoColorF.glsl` | 50 | rigid avatar (gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram) |
| `class1/deferred/diffuseAlphaMaskIndexedF.glsl` | 55 | indexed alpha mask |
| `class1/deferred/avatarF.glsl` | 55 | Linden 標準 avatar body (gDeferredAvatarProgram) |
| `class1/deferred/impostorF.glsl` | 57 | avatar impostor |
| `class1/deferred/treeF.glsl` | 54 | Linden tree (gDeferredTreeProgram) |
| `class1/deferred/terrainF.glsl` | 64 | legacy terrain (gDeferredTerrainProgram) |

### HAS_HDRI を書く writer

| Shader | 行 | 用途 |
|---|---|---|
| `class1/deferred/skyF.glsl` | 97 | HDRI 空 (実 HDRI sky 使用時) |

### SKIP_ATMOS を書く writer

専用 writer 無し。WL sky 描画系で `gbufferFlag` が default 0.0 のまま gbuffer に残るケース。

---

## 3. LLVOVolume → Pool / PASS routing (llvovolume.cpp:6890-7240)

通常の prim / mesh は `LLVOVolume::rebuildFace` (line 6890 付近) で face を pool に登録する。優先度順:

```cpp
LLGLTFMaterial* gltf_mat = te->getGLTFRenderMaterial();   // line 6904
LLMaterial* mat = nullptr;
if (gltf_mat == nullptr) {
    mat = te->getMaterialParams().get();   // line 6921
}
bool use_legacy_bump = te->getBumpmap() && (te->getBumpmap() < 18)
                      && (!mat || mat->getNormalID().isNull());   // line 6933
```

### 分岐 A: `gltf_mat != nullptr` (line 6946-6960)

GLTF PBR material が貼られている face:
- `ALPHA_MODE_BLEND` → `PASS_ALPHA` (forward path, softenLightF バイパス)
- `ALPHA_MODE_MASK` → `PASS_GLTF_PBR_ALPHA_MASK`
- それ以外 → `PASS_GLTF_PBR`

`PASS_GLTF_PBR` / `PASS_GLTF_PBR_ALPHA_MASK` は `LLDrawPoolGLTFPBR::renderDeferred` (lldrawpoolpbropaque.cpp:53) で:
- `gDeferredPBROpaqueProgram.bind()` (非 rigged)
- `gDeferredPBROpaqueProgram.bind(true)` (rigged)

→ どちらも **`pbropaqueF.glsl`** → HAS_PBR → softenLightF **PBR 分岐**

### 分岐 B: `gltf_mat == nullptr && mat != nullptr && !hud_group` (line 6942)

旧 Spec/Normal map material が貼られている face:
- fullbright + 条件 → `PASS_FULLBRIGHT*`
- `blinn_phong_transparent` → `PASS_ALPHA` (forward)
- `use_legacy_bump` → `PASS_BUMP`
- それ以外 → `material_pass = true` → `PASS_MATERIAL` / `PASS_SPECMAP` / `PASS_NORMMAP` / `PASS_NORMSPEC` (+ MASK/EMISSIVE 変種) (line 7018-7062)

`PASS_MATERIAL*` 系は `LLDrawPoolMaterials::render` で `gDeferredMaterialProgram[idx].bind()` → **`materialF.glsl`** → HAS_ATMOS → softenLightF **Legacy 分岐**

`PASS_BUMP` は `LLDrawPoolBump` で **`bumpF.glsl`** → HAS_ATMOS → Legacy 分岐

### 分岐 C: `gltf_mat == nullptr && (mat == nullptr || hud_group)` (line 7065+)

`else if (mat)` (line 7065) は HUD group の場合のみ → `PASS_SHINY` / `PASS_FULLBRIGHT_SHINY` (実画面に影響少)

その他 (mat も null) は通常の simple/bump/alpha routing (line 7095-7211):
- shiny + deferred + fullbright → `PASS_FULLBRIGHT_SHINY`
- shiny + deferred + use_legacy_bump → `PASS_BUMP` → bumpF
- shiny + deferred → `PASS_SIMPLE` → diffuseF
- alpha + ... → `PASS_ALPHA*`
- 通常 → `PASS_SIMPLE` → diffuseF or `PASS_FULLBRIGHT` (fullbright path)

→ HAS_ATMOS → Legacy 分岐 (シンプルな lit)

---

## 4. オブジェクト種別 → 経路の早見表

| オブジェクト | 経路 | Pool | Bound Shader | gbuffer flag | softenLightF 分岐 |
|---|---|---|---|---|---|
| 普通プリム (texture のみ) | LLVOVolume 7201 | POOL_SIMPLE | diffuseF | HAS_ATMOS | Legacy |
| プリム + Spec map のみ | LLVOVolume 6942 → material_pass | POOL_MATERIALS | materialF | HAS_ATMOS | Legacy |
| プリム + Normal map のみ | LLVOVolume 6942 → material_pass | POOL_MATERIALS | materialF | HAS_ATMOS | Legacy |
| プリム + legacy bumpmap preset | LLVOVolume 7190 | POOL_BUMP | bumpF | HAS_ATMOS | Legacy |
| プリム + GLTF PBR material | LLVOVolume 6946-6960 | POOL_GLTF_PBR | pbropaqueF | HAS_PBR | PBR |
| Mesh 服 (Spec/Normal map) | LLVOVolume 6942 (rigged) | POOL_MATERIALS | materialF | HAS_ATMOS | Legacy |
| Mesh 服 (GLTF PBR) | LLVOVolume 6946-6960 (rigged) | POOL_GLTF_PBR | pbropaqueF (rigged 変種) | HAS_PBR | PBR |
| 耳アクセ等 rigged mesh attach (mat あり) | LLVOVolume 6942 | POOL_MATERIALS | materialF | HAS_ATMOS | Legacy |
| Linden 標準 avatar body | LLDrawPoolAvatar 647 | POOL_AVATAR | avatarF | HAS_ATMOS | Legacy |
| Modern mesh body avatar (mat あり) | LLVOVolume 6942 (rigged) | POOL_MATERIALS | materialF | HAS_ATMOS | Legacy |
| avatar impostor | LLDrawPoolAvatar 560 | POOL_AVATAR | impostorF | HAS_ATMOS | Legacy |
| Linden tree | LLDrawPoolTree 60 | POOL_TREE | treeF | HAS_ATMOS | Legacy |
| legacy terrain | LLDrawPoolTerrain 229 | POOL_TERRAIN | terrainF | HAS_ATMOS | Legacy |
| PBR terrain | LLDrawPoolTerrain 238 | POOL_TERRAIN | pbrterrainF | HAS_PBR | PBR |
| HDRI 空 | LLDrawPoolWLSky | POOL_WL_SKY | skyF | HAS_HDRI | HDRI |
| WL 空 (preset) | LLDrawPoolWLSky | POOL_WL_SKY | wlskyF 系 | 0.0 (= SKIP_ATMOS) | SKIP_ATMOS |
| alpha blend 物体 (葉の半透明含む) | LLVOVolume → POOL_ALPHA | POOL_ALPHA | alphaF / pbralphaF | **softenLightF バイパス** (forward path) | (該当無し) |

---

## 5. softenLightF バイパス経路 (alpha blend 系)

`PASS_ALPHA` 登録された face は `LLDrawPoolAlpha::render` で **forward rendering** される。softenLightF を踏まないので、本書 §1-4 の flag 分岐外。

forward shader:
- `class1/deferred/alphaF.glsl` — legacy alpha blend
- `class1/deferred/pbralphaF.glsl` — GLTF PBR alpha blend
- 各種 fullbright alpha

→ 葉が alpha blend で半透明な mesh の場合、softenLightF への注入だけでは効果が乗らない。alpha 系 forward shader にも別途注入が必要。

---

## 6. C++ shader bind 地点の主要ファイル

| ファイル | 関数 | 行 | 役割 |
|---|---|---|---|
| `pipeline.cpp` | `LLPipeline::renderDeferredLighting` | 9994-10026 | softenLightF (gDeferredSoftenProgram) bind + render |
| `pipeline.cpp` | `bindDeferredShader` | 9611 | slow path、全 uniform/texture bind |
| `pipeline.cpp` | `bindDeferredShaderFast` | 9595 | fast path (前回 bind 済を仮定) |
| `llviewershadermgr.cpp` | `loadShadersDeferred` | 2137-2166 | gDeferredSoftenProgram の shader file load (softenLightV.glsl + softenLightF.glsl) |
| `lldrawpoolpbropaque.cpp` | `LLDrawPoolGLTFPBR::renderDeferred` | 53-69 | gDeferredPBROpaqueProgram bind |
| `lldrawpoolavatar.cpp` | `beginDeferredSkinned` | 642 | gDeferredAvatarProgram bind |
| `lldrawpooltree.cpp` | `LLDrawPoolTree::render` | 60 | gDeferredTreeProgram bind |
| `lldrawpoolterrain.cpp` | `LLDrawPoolTerrain::render` | 229/238 | gDeferredTerrainProgram / gDeferredPBRTerrainProgram bind |
| `lldrawpoolmaterials.cpp` | (pool render) | 36-94 | gDeferredMaterialProgram[idx] bind (PASS 種別ごと) |

---

## 7. 診断 canary 設置パターン (再現用)

shader path を実機で確認したい時は、各 writer の `frag_data[0]` (= albedo) に固有色を上書きする canary が確実。

```glsl
// 例: materialF.glsl 432 行付近
frag_data[0] = vec4(0.0, 1.0, 0.0, emissive); // GREEN canary
//frag_data[0] = max(vec4(diffcol.rgb, emissive), vec4(0));
```

色割り当ての推奨 (memory `feedback_diag_canary_design.md` 準拠):
- materialF → 緑 (0, 1, 0)
- bumpF → シアン (0, 1, 1)
- diffuseF → 黄 (1, 1, 0)
- avatarF → 紫 (0.5, 0, 1)
- pbropaqueF → マゼンタ (1, 0, 1)
- pbrmetallicroughnessF → 白 (1, 1, 1)

deploy 先 (memory `project_shader_install_path.md`):
- `~/ayastorm/app_settings/shaders/<class>/<deferred or gltf>/<shader>.glsl`

cache clear (memory `project_ayastorm_shader_cache_path.md`):
- `rm -rf ~/.ayastorm_x64/cache/shader_cache/`

---

## 8. 解析時の落とし穴 (今回踏んだもの)

1. **softenLightF Legacy 分岐に赤 tint が無い** のに「avatar が赤い」と観測されたら、それは softenLightF PBR 由来ではなく WL の sunset 暖色光 / 別 post-process 由来。Legacy 分岐の lit 計算には `vec3(1, 0.3, 0.3)` 系の tint コードは存在しない (本書作成時点)
2. **Modern mesh body avatar は LLDrawPoolAvatar を踏まない**。rigged mesh 扱いで LLVOVolume 経由 → materialF 行き
3. **rigged GLTF PBR mesh は `gDeferredPBROpaqueProgram.bind(true)` 経由で pbropaqueF を踏む** (binding に `bool rigged` 引数あり、shader file は同一)
4. **`use_legacy_bump` 条件**: bumpmap ID < 18 = SL ビルトイン preset (brick/grass 等) のみ。custom bumpmap は ID 18 以上で別経路 (≒ materialF の normal map 経由)
5. **gbuffer flag は `abs(data - flag) < 0.1` でマッチ**。HAS_ATMOS = 0.34 と HAS_PBR = 0.67 の距離は 0.33 で十分離れている。precision で誤マッチは起きない (16-bit float でも安全)

---

## 9. 更新履歴

- 2026-05-13 初版: r19 Translucency P0R3 で確定した経路マップを記録。LLVOVolume::rebuildFace / LLDrawPoolAvatar / LLDrawPoolTree / LLDrawPoolTerrain / LLDrawPoolMaterials / LLDrawPoolGLTFPBR と各 GBuffer writer shader を上から下まで trace。canary 実機検証 (壁=緑+水色、耳=緑、avatar=緑、Mesh 服=ピンク=pbropaqueF) で結論と完全整合
