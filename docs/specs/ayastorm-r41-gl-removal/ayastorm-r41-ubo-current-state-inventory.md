# AYAstorm r41 UBO 現状棚卸し (live document)

**作成日**: 2026-06-03
**起源 handoff**: `handoff/archive/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-pivot-to-ubo-design.md`
**位置付け**: r41 Vulkan UBO 全体設計 (= `ayastorm-r41-ubo-overall-design.md`、未起案) の **前提資料**。設計議論を始める前に、**OpenGL path で現在実際に動いている UBO 構造** と、これまで Vulkan 用に前倒しで仕込んだ **GLSL blueprint** とを完全に切り分けて把握するための live doc。

---

## §0 本 doc を起こすに至った経緯 (= 失敗反省)

η-24 から η-28 Phase 2c までの 5 sub-step 期間中、AYAstorm の r41 Vulkan 移行作業は **「Vulkan parse error が出た program に対してその program 単独の新規 UBO を切る」** を 26 回反復する形で進行した。結果として set=2 binding 0-25 の 26 個が積み上がったが、**「OpenGL 現状で UBO を何個使い、host 側で何回 bind しているか」は 1 度も棚卸ししていなかった**。

2026-06-03 AYA 指示で本欠陥が露見:

> 「現在の OpenGL での描画で UBO 何個作って Bind 数がどれぐらいになるかを考慮しないと仕事になってない」

本 doc は **その指示への直接の対応**。Vulkan 側の設計は本 doc が完成した後に始める。

---

## §1 OpenGL path で実働している UBO (= 論理 binding 4 種 / 物理 instance 1+2N+M 個)

OpenGL path で host C++ が `glBindBufferBase(GL_UNIFORM_BUFFER, ...)` を呼んで bind している UBO は、**論理 binding point = 4 種** (= `LLGLSLShader::UB_*` enum の 4 値、`llglslshader.h:157-160`) に対して **物理 GL buffer instance = scene 規模に依存して 1+2N+M 個** が動的に生成される (= 同じ binding point に対して owner 毎に別 instance を順次 bind)。

| # | binding 名 (`LLGLSLShader::UB_*`) | binding 値 | 物理 instance owner | instance 数 | 主 caller (bind 箇所) | 寿命 | 1 frame の bind 回数推定 |
|---|---|---|---|---|---|---|---|
| 1 | `UB_REFLECTION_PROBES` | 0 | `LLReflectionMapManager::mUBO` (**singleton**、`llreflectionmapmanager.cpp:1295-1297` で `glGenBuffers`) | **1 個** | `LLReflectionMapManager::setUniforms()` (`llreflectionmapmanager.cpp:1334`) | per-frame (reflection cube update 時) | 1 回 / frame (cube update は 6 frame 周期で更に間引き) |
| 2 | `UB_GLTF_NODES` | 1 | `gltf::Asset::mNodesUBO` (**per-Asset** メンバ、`asset.cpp:181-184` で `glGenBuffers`) | **N 個** (rezzed GLTF asset 数) | `GLTFSceneManager::render(variant)` (`gltfscenemanager.cpp:693`) | per-asset | N × 描画 variant 数 (典型 1-3 / frame、上限は rezzed GLTF 数) |
| 3 | `UB_GLTF_MATERIALS` | 2 | `gltf::Asset::mMaterialsUBO` (**per-Asset** メンバ、`asset.cpp:230-233` で `glGenBuffers`) | **N 個** (rezzed GLTF asset 数) | `GLTFSceneManager::render(variant)` (`gltfscenemanager.cpp:696`) | per-asset | 同上 (典型 1-3 / frame) |
| 4 | `UB_GLTF_JOINTS` | 3 | `gltf::Skin::mUBO` (**per-Skin** メンバ、`animation.cpp:409-412` で `glGenBuffers`、`animation.cpp:394-400` の `~Skin()` で `glDeleteBuffers`) | **M 個** (rigged GLTF primitive の Skin 数) | `GLTFSceneManager::render(variant)` (`gltfscenemanager.cpp:736`) | per-rigged-asset | M × draw call 数 (典型 1-10 / frame、rigged GLTF avatar 数次第) |

**所有パターンの整理**:
- 論理 binding 種類は **静的に 4 確保** (= `glUniformBlockBinding` で program 内 block index と紐付ける binding point 数)
- 物理 GL buffer instance は **動的に owner class が個別所有** (Manager singleton / Asset per-instance / Skin per-instance)、dtor で `glDeleteBuffers` 解放 (`Skin::~Skin()` で確認、Asset / Manager も同等想定)
- frame 内では同じ `binding=N` に対して **owner A の `mXxxUBO` → owner B の `mXxxUBO` → ...** と順次 bind 切替で描画
- = OpenGL UBO の典型「**論理 binding 静的・物理 buffer per-owner**」パターン
- scene 規模での scaling 例: 5 GLTF asset (うち rigged 2、各 Skin 1) なら `1 + 2×5 + 2 = 13 instance` / 大規模 GLTF avatar 描画 sim では数十 instance

**含意**:
- 「C++ 側 UBO は 4 個」は **binding 種類数の意味のみ正確**、物理 buffer instance 数は scene 規模で変動
- 設計 doc 議論で「UBO 数」を扱う時は **「binding 種類 (= 寿命分類 / shader 内宣言数)」** と **「物理 buffer instance (= memory footprint / upload 回数 / Core 分散単位)」** を **必ず区別** すること

### §1.1 UBO 生成 / upload site

| UBO | 生成 site | upload 頻度 |
|---|---|---|
| `UB_REFLECTION_PROBES` | `llreflectionmapmanager.cpp:1297` (`updateUniforms()`) | reflection update 時のみ |
| `UB_GLTF_NODES` | `gltf/asset.cpp:183` (`Asset::updateNodeData()`) | asset transform 変化時 |
| `UB_GLTF_MATERIALS` | `gltf/asset.cpp:232` (`Asset::updateMaterialData()`) | material 変化時 |
| `UB_GLTF_JOINTS` | `gltf/animation.cpp:411` (`Skin::updateTransforms()`) | rigged animation 毎 frame |

### §1.2 host C++ で program 側 binding を確立する箇所

| file:line | 内容 |
|---|---|
| `llglslshader.cpp:1869` (`LLGLSLShader::addPermutation()`) | program link 後に `glUniformBlockBinding(prog, blockIndex, bindingPoint)` を呼び、上記 4 UBO を該当 program の block index に紐付け |

それ以外の UBO 名 (e.g., `FrameViewProj` / `PerProgramUBO_*` / `<...>UBO_Legacy`) は **`glUniformBlockBinding` を 1 度も呼んでいない** = OpenGL path で実体が無い blueprint。

### §1.3 `glBindBufferRange` / non-GL_UNIFORM_BUFFER target

- `glBindBufferRange` 呼出: **0 件** (range 指定での partial bind は OpenGL path で未使用)
- `glBindBufferBase` の target が `GL_UNIFORM_BUFFER` 以外の呼出は本表外 (e.g., SSBO は別軸調査)

---

## §2 OpenGL path bare uniform 更新メカニズム (= UBO ではない値の流れ)

OpenGL path で **frame ごとに値を流している主要 mechanism は UBO ではなく bare uniform** (= `glUniform1i` / `glUniform1f` / `glUniform3fv` / `glUniform4fv` / `glUniformMatrix4fv` 系の個別 setter)。

### §2.1 主要 dispatcher

| dispatcher | 役割 | 呼出 frequency |
|---|---|---|
| `LLEnvironment::updateShaderUniforms(shader)` (`llenvironment.cpp`) | Sky / Water / Atmosphere の per-program uniform 反映 | shader bind 毎 (per-program × per-frame) |
| `LLViewerShaderMgr::updateShaderUniforms()` (`llviewershadermgr.cpp`) | dispatcher 上位 | per-frame |
| `LLGLSLShader::uniform*fv(name, ...)` (`llglslshader.cpp:2166-2554`) | 任意の値の setter | 描画 loop の各箇所で頻繁 |

### §2.2 推定 update 回数 / frame

| 種別 | 推定回数 | 備考 |
|---|---|---|
| 個別 `glUniform*` 呼出 (per-program bind 直後 + per-draw 直前) | **数百 〜 数千 / frame** | scene 規模、shader 数、avatar 数で変動 |
| その内 `LLEnvironment::updateShaderUniforms` 経由 | 数十 〜 数百 / frame | sky / water 系を含む全 shader |
| 上記とは別軸の per-draw setter (mTransform 等) | 残り | 描画 call 数依存 |

### §2.3 構造的含意

- **OpenGL path は scalar/vector 単位で逐次 upload する設計**。UBO の "塊で 1 回 upload して shader 内 layout で参照" という抽象は **採用していない**
- = **本 OpenGL の uniform 更新コストは 1 frame で数百回の API call**。Vulkan 化で UBO 単位の batch upload に乗せ替えれば、API call 数を桁で減らせる **本質的なゲインがある**
- ただし Vulkan 化前に **OpenGL path で UBO を増やす介入はしない** (= upstream Firestorm との divergence を避ける、原則 1 抵触)

---

## §3 GLSL 側に宣言済の UBO blueprint (= Vulkan path 用前倒し設計)

`indra/newview/app_settings/shaders/` 配下の `.glsl` で `uniform <Name> { ... }` ブロックを宣言している UBO の全件。これらは **全て `#ifdef LL_VULKAN_GLSL` の内側** に置かれており、**OpenGL path には到達しない** (= 宣言だけ存在し、実体 bind は §1 の 4 個のみ)。

### §3.1 set=0 帯 (per-frame backbone、3 個)

| binding | UBO 名 | 宣言例 | member 概要 |
|---|---|---|---|
| 0 | `FrameViewProj` | `class1/deferred/pbropaqueF.glsl:198` 他多数 | mat4 × 3 (view / proj / view_proj), mat3 × 2 (normal 系), vec2 |
| 1 | `FrameLights` | `class1/windlight/atmosphericsV.glsl:33` 他 | int, vec3 × 4, vec4 × 8, vec3 × 8, vec4 × 8, vec3 × 8, vec2 × 8 (sun/moon + light array) |
| 2 | `FrameAtmosphere_Lighting` | `class1/windlight/atmosphericsF.glsl:37` 他 | vec3 × 5, float × 9, int × 2 (atmosphere lighting params) |

**所見**: 3 個とも `Frame*` prefix で per-frame 寿命を明示。host C++ 側に upload 経路 **未整備** (`glBindBufferBase` で bind されていない)。

### §3.2 set=1 帯 (per-program material、2 個 = `MaterialUBO` + `MaterialUBO_Legacy`)

| binding | UBO 名 | 宣言例 | member 概要 |
|---|---|---|---|
| 0 | `MaterialUBO` | `class1/objects/simpleNoColorV.glsl:46` 他 | mat4, vec4 × 2, vec4, vec3, float × 4 (per-program material params) |
| 0 | `MaterialUBO_Legacy` | `class3/deferred/materialF.glsl:38` | per-program material params (legacy 派生形) |

**所見**: 
- **同 set/binding (1/0) に 2 個別 UBO 名** が共存。同一 program 内に両者を attach すると competition が出るはずなので、**program ごとに片方のみ宣言** されているはず (要 program 単位確認)。
- `set=1` は他に sampler (`diffuseMap` / `normalMap` / `specularMap` 等) が混在 (reference doc §6-C 参照)
- host C++ 側 upload 経路 **未整備** (両者とも `glBindBufferBase` 未呼出)

### §3.3 set=2 帯 (PerDrawUBO / PerProgramUBO、26 個)

η-24 から η-28 Phase 2c まで積み上げた 26 個 (Phase 2d-α 適用後で `vec3 center` field が binding=10 に追加されたのみ、binding 数は不変):

| binding | UBO 名 | 宣言例 | 寿命接頭辞 | 起源 |
|---|---|---|---|---|
| 0 | `PerDrawUBO_LightParams` | `class1/deferred/deferredUtil.glsl:175` | per-draw | η-3 |
| 1 | `PerDrawUBO_MultiLight` | `class3/deferred/multiPointLightF.glsl:76` | per-draw | η-23 |
| 2 | `PerProgramUBO_GammaCorrect` | `class1/deferred/postDeferredGammaCorrect.glsl:45` | per-program | η-24 |
| 3 | `PerProgramUBO_AlphaParams` | `class1/deferred/alphaV.glsl:145` | per-program | η-25 Phase 1a |
| 4 | `PerProgramUBO_ColorGrading` | `class1/deferred/postDeferredTonemap.glsl:74` | per-program | η-25 Phase 1b |
| 5 | `PerProgramUBO_PointLightV` | `class3/deferred/pointLightV.glsl:63` | per-program | η-25 Phase 1c |
| 6 | `PerProgramUBO_ShadowAlphaMaskV` | `class1/deferred/shadowAlphaMaskV.glsl:85` | per-program | η-26 Phase 1a |
| 7 | `PerProgramUBO_PostDeferredV` | `class1/deferred/postDeferredV.glsl:46` | per-program | η-26 Phase 1b |
| 8 | `PerProgramUBO_FullbrightShinyV` | `class1/deferred/fullbrightShinyV.glsl:60` | per-program | η-26 Phase 1c |
| 9 | `PerProgramUBO_FxaaF` | `class1/deferred/fxaaF.glsl:2126` | per-program | η-27 Phase 1a |
| 10 | `PerProgramUBO_SpotLightF` | `class3/deferred/spotLightF.glsl:74` | per-program | η-27 1d/1e-A + η-28 Phase 2d-α |
| 11 | `PerProgramUBO_PbrAlphaV` | `class1/deferred/pbralphaV.glsl:98` | per-program | η-27 Phase 1c |
| 12 | `PerProgramUBO_PostDeferredNoDoFF` | `class1/deferred/postDeferredNoDoFF.glsl:81` | per-program | η-27 Phase 1b |
| 13 | `PerProgramUBO_FsObjectIdF` | `class1/deferred/fsObjectIDF.glsl:33` | per-program | η-28 Phase 2a |
| 14 | `PerProgramUBO_ShadowCubeV` | `class1/deferred/shadowCubeV.glsl:57` | per-program | η-28 Phase 2a |
| 15 | `PerProgramUBO_WaterHazeV` (V+F 共有) | `class3/deferred/waterHazeV.glsl:86` (+F) | per-program | η-28 Phase 2a |
| 16 | `PerProgramUBO_VisualizeBuffersF` | `class1/deferred/postDeferredVisualizeBuffers.glsl:40` | per-program | η-28 Phase 2a |
| 17 | `PerProgramUBO_GodraysF` | `class1/deferred/godraysF.glsl:124` | per-program | η-28 Phase 2b |
| 18 | `PerProgramUBO_VolumetricLightF` | `class3/deferred/volumetricLightF.glsl:129` | per-program | η-28 Phase 2b |
| 19 | `PerProgramUBO_VelocityAlphaV` | `class1/deferred/velocityAlphaV.glsl:60` | per-program | η-28 Phase 2b |
| 20 | `PerProgramUBO_PostDeferredF` (+ HQDoFF 共有) | `class1/deferred/postDeferredF.glsl:108` | per-program | η-28 Phase 2b |
| 21 | `PerProgramUBO_CofF` | `class1/deferred/cofF.glsl:56` | per-program | η-28 Phase 2c |
| 22 | `PerProgramUBO_BlurLightF` | `class1/deferred/blurLightF.glsl:64` | per-program | η-28 Phase 2c |
| 23 | `PerProgramUBO_WaterF` | `class3/environment/waterF.glsl:119` | per-program | η-28 Phase 2c |
| 24 | `PerProgramUBO_PbrTerrainV` (permutation 共有) | `class1/deferred/pbrterrainV.glsl:79` | per-program | η-28 Phase 2c |
| 25 | `PerProgramUBO_PointLightF` | `class3/deferred/pointLightF.glsl:53` | per-program | η-28 Phase 2c |

#### §3.3.1 set=2 内で binding 重複と見える UBO 名群 (要追加調査)

Agent 棚卸し結果には **同一 binding に複数の別 UBO 名** が並んでいた例がある:

- binding=0 候補: `PerDrawUBO_ClipPlane` / `PerDrawUBO_LightParams` / `PerDrawUBO_SkinnedVelocity` / `PerDrawUBO_AvatarVelocity` / `PerDrawUBO_AvatarSkin` / `PerDrawUBO_ObjectSkin`

これは:
- **A 案**: per-program で binding=0 が別 UBO 名に再割当されている (= GL/Vulkan ではこれが許される、program ごとに binding namespace が独立)
- **B 案**: 起源 sub-step (η-3 / η-23) で複数 UBO に同一 binding を割当ていて後段で reorganize 漏れた dead code
- **C 案**: Agent 棚卸しの grep が `PerDrawUBO_*` を一律 binding=0 として誤抽出

の 3 通りが考えられる。**本棚卸し doc 時点では確定せず、追加調査 (§7) の課題として明示**。

handoff doc §2.1 の確定情報は binding=0 = `PerDrawUBO_LightParams` / binding=1 = `PerDrawUBO_MultiLight` の 2 個のみで、ClipPlane / SkinnedVelocity / AvatarVelocity / AvatarSkin / ObjectSkin は **過去 handoff doc 群で言及されていない**。η-3 期以前 (Stage 1-2 期) の初期投入の可能性が高い。

### §3.4 set=3 帯 (`<Name>UBO_Legacy` 群、**54 個確定** = 2026-06-03 grep 集計)

η-6 / η-13 期 program 単位の旧 grouping。**全件 grep で 54 unique UBO 名を確認** (handoff doc §6-D の 32 個より 22 個多い = handoff doc が古い、本 doc が正)。binding 番号:

| binding | UBO 名 | 宣言例 |
|---|---|---|
| 0 | `AtmoExtraUBO_Legacy` | `class1/windlight/atmosphericsFuncs.glsl:89` |
| 1 | `SkyVParamUBO_Legacy` | `class1/deferred/skyV.glsl:83` |
| 2 | `SkyFParamUBO_Legacy` | `class1/deferred/skyF.glsl:117` |
| 3 | `CloudsVParamUBO_Legacy` | `class1/deferred/cloudsV.glsl:106` |
| 4 | `CloudsFParamUBO_Legacy` | `class1/deferred/cloudsF.glsl:61` |
| 5 | `SoftenLightParamUBO_Legacy` | `class3/deferred/softenLightF.glsl:57` |
| 6 | `DeferredUtilParamUBO_Legacy` | `class1/deferred/deferredUtil.glsl:99` |
| 7 | `ShadowUtilParamUBO_Legacy` | `class1/deferred/shadowUtil.glsl:80` |
| 8 | `AOUtilParamUBO_Legacy` | `class1/deferred/aoUtil.glsl:41` |
| 9 | `WaterFogUBO_Legacy` | `class1/environment/waterFogF.glsl:48` |
| 10 | `TonemapUBO_Legacy` | `class1/deferred/tonemapUtilF.glsl:146` |
| 11 | `GlobalFParamUBO_Legacy` | `class1/deferred/globalF.glsl:32` |
| 12 | `CASParamUBO_Legacy` | `class1/deferred/CASF.glsl:52` |
| 13 | `PBROpaqueExtraUBO_Legacy` | `class1/deferred/pbropaqueF.glsl:162` |
| 14 | `SMAAParamUBO_Legacy` | `class1/deferred/SMAA.glsl:41` |
| 17 | `ReflectionProbeUBO_Legacy` | `class3/deferred/reflectionProbeF.glsl:80` |
| 18 | `GlowFParamUBO_Legacy` | `class1/effects/glowF.glsl:39` |
| 19 | `GlowVParamUBO_Legacy` | `class1/effects/glowV.glsl:54` |
| 20 | `GlowExtractFParamUBO_Legacy` | `class1/effects/glowExtractF.glsl:71` |
| 21 | `PbrShadowAlphaMaskVParamUBO_Legacy` | `class1/deferred/pbrShadowAlphaMaskV.glsl:89` |
| 22 | `AvatarAlphaShadowVParamUBO_Legacy` | `class1/deferred/avatarAlphaShadowV.glsl:59` |
| 24 | `DofCombineFParamUBO_Legacy` | `class1/deferred/dofCombineF.glsl:98` |
| 25 | `ExposureFParamUBO_Legacy` | `class1/deferred/exposureF.glsl:49` |
| 26 | `LuminanceFParamUBO_Legacy` | `class1/deferred/luminanceF.glsl:60` |
| 27 | `MotionBlurFParamUBO_Legacy` | `class1/deferred/motionBlurF.glsl:66` |
| 28 | `ScreenSpaceReflPostFParamUBO_Legacy` | `class3/deferred/screenSpaceReflPostF.glsl:57` |
| 30 | `SkinSSSPrototypeFParamUBO_Legacy` | `class1/deferred/skinSSSF.glsl:80` |
| 32 | `ClipFParamUBO_Legacy` | `class1/interface/clipF.glsl:46` |
| 33 | `NormgenFParamUBO_Legacy` | `class1/deferred/normgenF.glsl:53` |
| 37 | `NormaldebugVParamUBO_Legacy` | `class1/interface/normaldebugV.glsl:56` |
| 38 | `SnapshotFrameFParamUBO_Legacy` | `class1/post/snapshotFrameF.glsl:33` |
| 39 | `UnderWaterFParamUBO_Legacy` | `class3/environment/underWaterF.glsl:63` |
| 40 | `PreviewVParamUBO_Legacy` | `class1/objects/previewV.glsl:47` |
| 41 | `SimpleColorFParamUBO_Legacy` | `class1/objects/simpleColorF.glsl:64` |
| 42 | `StarsFParamUBO_Legacy` | `class1/deferred/starsF.glsl:57` |
| 43 | `SunDiscFParamUBO_Legacy` | `class1/deferred/sunDiscF.glsl:48` |
| 44 | `MoonFParamUBO_Legacy` | `class1/deferred/moonF.glsl:67` |
| 45 | `StarsVParamUBO_Legacy` | `class1/deferred/starsV.glsl:67` |
| 46 | `VignetteParamUBO_Legacy` | `class1/post/exoVignetteF.glsl:42` |
| 47 | `PathfindingVParamUBO_Legacy` | `class1/interface/pathfindingV.glsl:70` |
| 48 | `PathfindingNoNormalVParamUBO_Legacy` | `class1/interface/pathfindingNoNormalV.glsl:65` |
| 49 | `GlowCombineFParamUBO_Legacy` | `class1/interface/glowcombineF.glsl:44` |
| 50 | `OcclusionCubeVParamUBO_Legacy` | `class1/interface/occlusionCubeV.glsl:54` |
| 51 | `RadianceGenFParamUBO_Legacy` | `class1/interface/radianceGenF.glsl:42` |
| 52 | `IrradianceGenFParamUBO_Legacy` | `class2/interface/irradianceGenF.glsl:42` |
| 53 | `PbrOpaqueVParamUBO_Legacy` | `class1/deferred/pbropaqueV.glsl:88` |
| 54 | `AvatarFParamUBO_Legacy` | `class1/deferred/avatarF.glsl:76` |
| 55 | `VelocityVParamUBO_Legacy` | `class1/deferred/velocityV.glsl:54` |
| 56 | `RlvFParamUBO_Legacy` | `class1/deferred/rlvF.glsl:62` |
| 57 | `AvatarClothVParamUBO_Legacy` | `class1/deferred/avatarV.glsl:109` |
| 58 | `GaussianFParamUBO_Legacy` | `class1/interface/gaussianF.glsl:46` |
| 60 | `WaterVParamUBO_Legacy` | `class1/environment/waterV.glsl:61` (+ waterF.glsl:108) |
| 61 | `TerrainVParamUBO_Legacy` | `class1/deferred/terrainV.glsl:108` |
| 62 | `SMAABlendWeightsFParamUBO_Legacy` | `class1/deferred/SMAABlendWeightsF.glsl:114` |

**所見**:
- **2026-06-03 grep 集計確定 = 54 unique UBO 名** (handoff §6-D の 32 個より 22 個多い、handoff doc 古い)
- 同名 UBO が複数 file で再宣言されている例あり:
  - `CloudsVParamUBO_Legacy` (binding=3): `cloudsV.glsl` + `cloudsF.glsl` の 2 file
  - `WaterVParamUBO_Legacy` (binding=60): `waterV.glsl` + `waterF.glsl` の 2 file (V+F 共有)
  - `ShadowUtilParamUBO_Legacy` (binding=7): `class1/deferred/shadowUtil.glsl` + `cinematic_bd/class1/deferred/shadowUtil.glsl` の 2 file (cinematic_bd は AYAstorm 独自 BD overlay 由来、同 binding 共有)
- set=3 は program 単位の grouping だが、**`set=2 PerProgramUBO_*` と同じ寿命** (per-program)。命名規則だけ違って役割重複
- host C++ 側 upload 経路 **未整備** (どれも `glBindBufferBase` で bind されていない)

---

## §4 host C++ 側 UBO 管理 API 棚卸し

### §4.1 `LLGLSLShader::UB_*` enum (= host から見た UBO 論理 binding 識別子)

`llglslshader.h:157-160` で定義される UBO enum (= **2026-06-03 確定**、全件):

| enum 値 | shader 内 block 名 | 物理 instance owner | instance 数 |
|---|---|---|---|
| `UB_REFLECTION_PROBES` | `ReflectionProbes` | `LLReflectionMapManager::mUBO` (singleton) | 1 個 |
| `UB_GLTF_JOINTS` | `GLTFJoints` | `gltf::Skin::mUBO` (per-Skin) | M 個 (rigged GLTF Skin 数) |
| `UB_GLTF_NODES` | `GLTFNodes` | `gltf::Asset::mNodesUBO` (per-Asset) | N 個 (rezzed GLTF asset 数) |
| `UB_GLTF_MATERIALS` | `GLTFMaterials` | `gltf::Asset::mMaterialsUBO` (per-Asset) | N 個 (rezzed GLTF asset 数) |

**= 論理 binding 種類 4 のみ**。物理 GL buffer instance は §1 / §1.1 で詳述した通り **1 + 2N + M 個** が scene 規模に応じて動的生成。これ以外の UBO 名 (= §3 で列挙した 80+ 個の Vulkan blueprint) は **enum 未登録 = host 側で bind する識別子そのものが無い**。

### §4.2 既存 bind / upload mechanism

| operation | API | site (代表) |
|---|---|---|
| UBO buffer 生成 | `glGenBuffers` + `glBufferData(GL_UNIFORM_BUFFER, ...)` | `llreflectionmapmanager.cpp:1297` / `gltf/asset.cpp:183` 等 |
| UBO upload | `glBufferData` / `glBufferSubData` | upload site と同じ |
| UBO bind | `glBindBufferBase(GL_UNIFORM_BUFFER, binding, bufId)` | `llreflectionmapmanager.cpp:1334` / `gltfscenemanager.cpp:693/696/736` |
| program 内 block 紐付け | `glUniformBlockBinding(prog, blockIdx, binding)` | `llglslshader.cpp:1869` |

### §4.3 redirect 層の **完全欠落** (= **2026-06-03 確定**)

`LLGLSLShader::uniform4fv("color", ...)` のような **既存 call site** が呼ばれた時、UBO 化済 shader への redirect 経路が **無い**。`llglslshader.cpp:2166-2557` の uniform setter family 全件を直 read で確認:

| method | line | 実装 |
|---|---|---|
| `uniform1f` | 2166 | `glUniform1f(mUniform[index], x)` 直呼び |
| `fastUniform1f` | 2192 | 同上 |
| `uniform2f` | 2202 | `glUniform2f(...)` 直呼び |
| `uniform3f` | 2229 | `glUniform3f(...)` 直呼び |
| `uniform4f` | 2256 | `glUniform4f(...)` 直呼び |
| `uniform1iv` | 2283 | `glUniform1iv(...)` 直呼び |
| `uniform4iv` | 2310 | `glUniform1iv(...)` 直呼び (`4iv` 実装が `1iv` 呼出に bug の可能性、要別軸) |
| `uniform1fv` | 2338 | `glUniform1fv(...)` 直呼び |
| `uniform2fv` | 2365 | `glUniform2fv(...)` 直呼び |
| `uniform3fv` | 2392 | `glUniform3fv(...)` 直呼び |
| `uniform4fv` | 2419 | `glUniform4fv(...)` 直呼び |
| `uniform4uiv` | 2447 | `glUniform4uiv(...)` 直呼び |
| `uniformMatrix2fv` | 2475 | `glUniformMatrix2fv(...)` 直呼び |
| `uniformMatrix3fv` | 2496 | `glUniformMatrix3fv(...)` 直呼び |
| `uniformMatrix3x4fv` | 2517 | `glUniformMatrix3x4fv(...)` 直呼び |
| `uniformMatrix4fv` | 2538 | `glUniformMatrix4fv(...)` 直呼び |

**観測**:
- 全 16 method **`#ifdef LL_VULKAN_GLSL` 分岐ゼロ** / **`if (mUseUBO)` 分岐ゼロ** / **Vulkan path redirect の痕跡ゼロ**
- `mValue` cache (= 値 dedup で同値時 GL call 省略) は存在するが **これは GL 側の最適化** で Vulkan UBO redirect とは無関係
- `glUniform4iv` setter が内部で `glUniform1iv` を呼んでいる箇所 (line 2330) は **別軸の bug 候補**、本棚卸し scope 外

**含意**:
- Vulkan path に切替えても、shader 側で UBO 化された `uniform vec4 color` には **値が来ない**
- = §3 の 80+ 個 UBO は **「宣言されているが値が来ない」** 完全 dead 状態
- = **本棚卸しで最大の発見事項**。Vulkan 化を完成させるには **host C++ 側 redirect 層の新規実装が必須**

---

## §5 1 frame の bind / update 仕事量見積

### §5.1 OpenGL path 現状 (実測の代理推定)

| 種別 | 推定回数 / frame | source |
|---|---|---|
| UBO bind (`glBindBufferBase`) | **~(1) + (2N) + (M × draw)** = scene 規模次第で **10〜100 回** | §1 の owner pattern (singleton 1 + per-Asset 2N + per-Skin M × draw call 数) |
| UBO upload (`glBufferData` / `glBufferSubData`) | **owner state 変化時のみ** (reflection cube 更新 / GLTF asset transform 変化 / rigged animation 毎 frame) | upload は dirty 時、bind とは独立 |
| bare uniform set (`glUniform*`) | **~数百〜数千回** | §2.2 |
| **合計 GL API call** | **~数百〜数千 / frame** | scene 規模次第 (bare uniform が支配的、UBO bind は副次) |

**注**: 旧版で「UBO bind ~5-20 回」と推定していたが、これは `binding` 種類数 (4) ベースの誤算。実際は **per-Asset / per-Skin instance を順次 bind 切替** するため、GLTF asset 数 × draw call 数で 10〜100 回オーダーになりうる (大規模 GLTF avatar sim では更に多い)。

### §5.2 Vulkan 化後の理想形 (= 設計目標)

| 種別 | 目標回数 / frame | 根拠 |
|---|---|---|
| UBO upload (memcpy + batch) | **数十回** | per-frame 3 + per-program ~25 + per-draw N |
| descriptor set bind (= UBO bind 相当) | **数十回** | per-program 単位で 1 回 |
| individual uniform set | **0 回** | UBO 統合で消滅 |

**= 数千 API call → 数十回 buffer write** の構造改善が Vulkan 化の本来ゲイン。

### §5.3 ゲイン実現の前提

- §4.3 で指摘した **host C++ redirect 層**が完成していること
- §3 の 80+ 個 UBO のうち **本当に必要なものだけが残っていること** (= 設計 doc の主題)

---

## §6 構造的所見

### §6.1 OpenGL path と Vulkan blueprint は **完全分離している**

GLSL の `#ifdef LL_VULKAN_GLSL` gate により、OpenGL path は §3 の UBO blueprint に **一切到達しない**。逆も真。

**含意**:
- 設計 pivot で UBO blueprint を大幅再構成しても、**OpenGL path の動作には影響しない**
- = pivot 期間中の検証 risk は Vulkan side に限定、AYAstorm リリース sched に影響無し

### §6.2 Vulkan blueprint は **「宣言だけある」状態** で実体上 dead

- §3 の 80+ 個 UBO は `glBindBufferBase` も `glUniformBlockBinding` も呼ばれていない
- = Vulkan path を build して shader compile が通っても、**実 frame 描画では値が UBO に入らない**

**含意**:
- Vulkan の単体 SPIR-V parse 成功 = 「Vulkan 化が進んでいる」と誤読していた
- 実態は **「parse error が消えただけで、Vulkan 描画は値無しで成立しない」**
- = η-24 から η-28 Phase 2c の 26 sub-step は **「Vulkan parse は通るが描画は無効」** な状態を積んでいた

### §6.3 set=2 (新規 26 個) と set=3 Legacy (~53 個) は **役割重複**

- 両者とも per-program 寿命
- 命名規約は違うが grouping 単位は同じ "program param"
- = **どちらかに統合すべき** (= 設計 doc 主題の 1 つ)

### §6.4 host C++ redirect 層は **未着手**

§4.3 で詳述。これが無いと Vulkan 描画は値無しで dead。

### §6.5 bare uniform → UBO migration の **粒度設計が未定義**

- OpenGL の bare uniform は per-program 単位で `LLEnvironment::updateShaderUniforms` 等の dispatcher から個別 setter で投入
- Vulkan の UBO は 1 ブロック単位で memcpy upload
- = どの bare uniform を **どの UBO に集約するか** の対応表が未整備
- 現状 §3 の 80+ 個 UBO は **「parse error が出た uniform を 1 個ずつ UBO 化」** で発生しており、bare uniform → UBO 集約の対応表として完全ではない

### §6.6 「論理 binding 種類」と「物理 buffer instance」は **必ず区別** する (= 2026-06-03 AYA 指摘で追加)

§1 / §4.1 で確定した通り、UBO の数え方には 2 軸ある:

- **論理 binding 種類**: `LLGLSLShader::UB_*` enum 値の数 (= shader 内 `layout(binding=N)` で参照される binding point 数 = `glUniformBlockBinding` で program 内 block index と紐付ける論理 slot 数)
  - 現状 OpenGL path = **4 種** (REFLECTION_PROBES / GLTF_NODES / GLTF_MATERIALS / GLTF_JOINTS)
  - GLSL Vulkan blueprint = **85 個** (set=0:3 / set=1:2 / set=2:26 / set=3:54) だが host enum 未登録のため OpenGL path には現れない
- **物理 GL buffer instance**: 実際に `glGenBuffers` で生成された GL buffer object の数 (= memory footprint / upload 対象 / Core 分散単位)
  - 現状 OpenGL path = **1 (singleton) + 2N (per-Asset) + M (per-Skin) 個**、scene 規模で動的変動
  - 大規模 GLTF avatar sim では数十〜100 個オーダー

**設計議論時の含意**:
- 「UBO を統合/分割するか」議論時は **論理 binding 軸** (= 寿命分類 / shader 宣言数 / call site refactor)
- 「memory 削減 / upload cost 削減 / worker thread 分散」議論時は **物理 instance 軸** (= owner class 数 / per-instance scaling)
- 両軸を混同すると「24 個の per-program UBO」 ↔ 「数千個の per-draw light instance」のような不当な比較が発生する
- 旧 §5.1 「UBO bind ~5-20 回 / frame」推定は **論理軸のみ** で算出した誤算、物理 instance ベースで再評価済

---

## §7 不確実点 / 追加調査必要事項 (= 設計 doc 起案前に解消)

| # | 項目 | 状態 | 解消方法 |
|---|---|---|---|
| 1 | set=2 内で同一 binding に複数 UBO 名と見える件 (§3.3.1) | 未解消 | `git log -S` + grep で各 UBO 宣言行の preprocessor gate 詳細確認、program 単位 binding namespace 検証 |
| 2 | `LLGLSLShader::UB_*` enum の正確な全件 | **✓ 解消 (2026-06-03)**: 4 個確定、§4.1 update 済 | - |
| 3 | set=3 Legacy の正確な個数 | **✓ 解消 (2026-06-03)**: 54 個確定、§3.4 update 済 | - |
| 4 | `FrameViewProj` / `FrameLights` / `FrameAtmosphere_Lighting` の正確な member 一覧と他 UBO/bare uniform との重複 | 未解消 | 各 GLSL 宣言行直 read + bare uniform 名との突合 |
| 5 | `MaterialUBO` と `MaterialUBO_Legacy` の関係 (両者 set=1 binding=0 共有) | 部分解消 (2 個別 UBO 名で同 binding 共有を §3.2 で記載、program 単位の attach 排他確認は未) | program 単位での attach 確認 |
| 6 | `LLGLSLShader::uniform*fv()` (`llglslshader.cpp:2166-2557`) の現状 GL path のみか / Vulkan path redirect の痕跡 | **✓ 解消 (2026-06-03)**: redirect 痕跡 0 件確定、§4.3 update 済 | - |
| 7 | upstream Firestorm との UBO blueprint 差分 | 未解消 | upstream HEAD との diff |
| 8 | OpenGL path に存在するが棚卸し外の SSBO / image binding 有無 | 未解消 | `glBindBufferBase` の non-GL_UNIFORM_BUFFER target 全件 |
| 9 | 典型 scene での実 instance 数 (N = rezzed GLTF asset 数 / M = rigged GLTF Skin 数) | 未解消 (2026-06-03 §6.6 追加に伴う) | AYA 機 typical 撮影 scene 起動時 log で `glGenBuffers` 由来 owner 数を `LL_INFOS` hook 一時挿入で計測、または GLTF asset/Skin 生成 site にカウンタ |

---

## §8 設計議論への含意 (= 次 step への橋渡し)

本棚卸しを踏まえた設計議論の起点 (= `ayastorm-r41-ubo-overall-design.md` で詰める内容):

### §8.1 第一 議題: redirect 層の枠組み (= 描画を成立させるための必須条件)

§4.3 で指摘した host C++ redirect 層が無ければ Vulkan path は描画として成立しない。**設計 doc の冒頭で枠組みを決める**。

### §8.2 第二 議題: bare uniform → UBO の集約対応表

§6.5 で指摘した粒度設計。**OpenGL の bare uniform 群を寿命別に分類**し、それぞれを **どの UBO に集約するか** を表として確定。

### §8.3 第三 議題: set=2 (26 個) + set=3 (53 個) の再構成

§6.3 で指摘した役割重複。**§8.2 の集約表に従って統合 / 削除 / 保持を決定**。

### §8.4 第四 議題: 寿命分類体系の最終確定

per-frame / per-view / per-program / per-material / per-draw の 5 分類で過不足ないか。**§8.2 の集約表が実際に当てはまるかで判定**。

### §8.5 第五 議題: Phase 2d-α 適用済 commit 群の処遇

η-28 Phase 2d-α で適用済 5 file feat (`0587c574da`) + 3 docs commit は **本棚卸しの含意 (§6.2: Vulkan blueprint は dead) からして実害ゼロ・実効果ゼロ**。設計 doc 確定後に push / 保留 / revert を決定。

### §8.6 第六 議題 (2026-06-03 追加): instance 数 axis での設計評価

§6.6 で確定した「論理 binding 種類」と「物理 buffer instance」の二軸を、設計 doc の各議題に **両軸で評価する** ルールとして組み込む:

- **redirect 層 (§8.1)**: dirty flag 粒度は「論理 binding 単位 = 84 bit」と「物理 instance 単位 = scene 規模で動的」のどちらを基準にするか (現状の OpenGL 4 種 per-owner instance パターンを Vulkan で再現するか、あるいは redirect 層では論理 binding 単位に閉じて物理 instance は別レイヤー)
- **bare uniform → UBO 集約 (§8.2)**: per-program bare uniform は **論理 binding 1 個** に集約しても、frame 内で同一 binding に対し program × N draw call 分の **物理 upload + bind** が発生する点を見落とさない
- **set=2 + set=3 再構成 (§8.3)**: 「統合 / 削除 / 保持」判断は論理軸 (= 寿命分類整合) で行うが、Core 分散時の **物理 upload 並列度** にも影響するため、両軸チェック
- **寿命分類 (§8.4)**: per-frame / per-program / per-draw は **論理軸**、per-asset / per-skin / per-light など **owner 単位** は物理軸の細分。両軸を分けて分類体系を定義する必要あり

---

## §9 本 doc の更新サイクル

- §7 課題が解消されたら本 doc を update
- 設計 doc (`ayastorm-r41-ubo-overall-design.md`) 起案時、本 doc を引用 source として扱う
- η-29+ で UBO 構成が変化したら本 doc にも反映 (live doc)

**= 本 doc は r41 Vulkan 完成まで permanent reference**。設計 pivot 後の作業は本 doc を起点に計画する。

---

## Appendix A: 関連 file 群

### A.1 host C++ UBO 管理
- `indra/llrender/llglslshader.h` (LLGLSLShader / UB_* enum)
- `indra/llrender/llglslshader.cpp` (`addPermutation()` / `uniform*fv()` family)
- `indra/newview/llreflectionmapmanager.cpp` (UB_REFLECTION_PROBES)
- `indra/newview/gltfscenemanager.cpp` (UB_GLTF_*)
- `indra/newview/gltf/asset.cpp` (GLTF node/material UBO)
- `indra/newview/gltf/animation.cpp` (GLTF joint UBO)
- `indra/newview/llenvironment.cpp` (`updateShaderUniforms`)
- `indra/newview/llviewershadermgr.cpp` (shader 登録 + dispatcher 上位)

### A.2 GLSL UBO blueprint 集中地点
- `indra/newview/app_settings/shaders/class1/deferred/` (Frame*/PerProgram*/PerDraw*/Legacy 群多数)
- `indra/newview/app_settings/shaders/class3/deferred/` (Light 系 PerProgram*)
- `indra/newview/app_settings/shaders/class1/windlight/` (atmospherics 系 Frame*)
- `indra/newview/app_settings/shaders/class1/effects/` (Glow 系 Legacy)
- `indra/newview/app_settings/shaders/class1/interface/` (Path/Occlusion 系 Legacy)
- `indra/newview/app_settings/shaders/class1/post/` (Snapshot/Vignette 系 Legacy)
- `indra/newview/app_settings/shaders/class1/environment/` (Water 系 Legacy)
- `indra/newview/app_settings/shaders/class1/objects/` (simple 系 Legacy + MaterialUBO 観測)

### A.3 参照 handoff doc
- `handoff/archive/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-pivot-to-ubo-design.md` (本 pivot の経緯資料)
- `handoff/archive/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-alpha-prep.md` (Phase 2d-α 前 prep doc、本 pivot で前提変更)
- `reference-shader-location-map.md` §6-A〜E (既存 UBO binding マップ、本 doc で update が必要な箇所多数)

---

**本 doc 完成時点で `ayastorm-r41-ubo-overall-design.md` の起案条件を満たす**。§7 課題を順に潰した後、設計 doc に進む。
