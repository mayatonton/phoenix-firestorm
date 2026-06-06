# r41 UBO Relations — 全 94 UBO 関係図 (= 7 dimension 一元集約)

**着手契機**: 2026-06-06 AYA literal「1 つは UBO の関係図を別資料として UBO ファイルと同じディレクトリに書いてください」record

**位置付け**: Phase 2 = 全 94 UBO 一括本実装化の **設計入力資料 (関係図)**。各 UBO file §11 (他 UBO との関係) を 7 dimension 別に一元集約。

**起案規律**:
- 各 dimension で実コード source verify 済の関係のみ確定形記載
- 推定/不明関係は §8 不明事項集約に分離 (= memory `feedback_admit_unknown` 遵守)
- 1 UBO 1 read で全 94 file 順次精査済 (= sequential、agent 並列禁止)

---

## §1. descriptor set 同居関係 (= set 別 cluster)

### §1.1 set 0 (= Frame 帯、`llvkloader.cpp:858` `V3A_FRAME_SET_BINDINGS = 4` literal)

| binding | UBO 名 | size | cadence |
|---|---|---|---|
| 0 | FrameViewProj | 512 | 0 PerFrame |
| 1 | FrameLights | 768 | 0 PerFrame |
| 2 | FrameAtmosphere_Lighting | 256 | 0 PerFrame |
| 3 | Global_ReflectionProbes | 256 | 5 SINGLETON |

**bind pattern**: frame start で set=0 全 4 UBO 同時 bind (= `vkCmdBindDescriptorSets` 1 回呼出、`llvkloader.cpp:2839` literal)

### §1.2 set 1 (= Material 帯、Material 専用)

| binding | UBO 名 | size | cadence |
|---|---|---|---|
| 0 | MaterialUBO | 256 | 1 PerProgram (PBR 10-member full canonical) |
| 0 | MaterialUBO_Legacy | 256 | 1 PerProgram (legacy 8-member、shader 単位排他選択) |

**bind pattern**: program 切替時に set=1 bind。**排他運用** = 同 program 内で両者 attach 不可、host C++ name-based dispatch で program 識別後どちらか 1 UBO 配線 (= blueprint header literal)

### §1.3 set 2 (= PerDraw + PerProgram 混在帯、UBO_DYNAMIC、`llvkloader.cpp:861` `V3A_DRAW_SET_BINDINGS = 4` literal)

**binding 0 共有 6 UBO** (= name-based dispatch、blueprint header literal `set=2 binding=0 を共有する 6 UBO の 1 つ`):

| UBO 名 | size | cadence |
|---|---|---|
| PerDrawUBO_AvatarSkin | 768 | 2 PerDraw |
| PerDrawUBO_AvatarVelocity | 768 | 2 PerDraw |
| PerDrawUBO_ClipPlane | 256 | 2 PerDraw |
| PerDrawUBO_LightParams | 256 | 2 PerDraw |
| PerDrawUBO_ObjectSkin | 10752 | 2 PerDraw (全 UBO 中最大) |
| PerDrawUBO_SkinnedVelocity | 5376 | 2 PerDraw |

**他 binding (= 独立配置 PerDraw + PerProgram 系)**:

| binding | UBO 名 | size | cadence |
|---|---|---|---|
| 1 | PerDrawUBO_MultiLight | 768 | 2 PerDraw (独立 binding) |
| 2 | PerProgramUBO_GammaCorrect | 256 | 1 PerProgram |
| 3 | PerProgramUBO_AlphaParams | 256 | 1 PerProgram |
| 4 | PerProgramUBO_ColorGrading | 256 | 1 PerProgram |
| 5 | PerProgramUBO_PointLightV | 256 | 1 PerProgram |
| 6 | PerProgramUBO_ShadowAlphaMaskV | 256 | 1 PerProgram |
| 7 | PerProgramUBO_PostDeferredV | 256 | 1 PerProgram |
| 8 | PerProgramUBO_FullbrightShinyV | 256 | 1 PerProgram |
| 9 | PerProgramUBO_FxaaF | 256 | 1 PerProgram |
| 10 | PerProgramUBO_SpotLightF | 256 | 1 PerProgram (本 set 内 10-member 最大) |
| 11 | PerProgramUBO_PbrAlphaV | 256 | 1 PerProgram |
| 12 | PerProgramUBO_PostDeferredNoDoFF | 256 | 1 PerProgram |
| 13 | PerProgramUBO_FsObjectIdF | 256 | 1 PerProgram |
| 14 | PerProgramUBO_ShadowCubeV | 256 | 1 PerProgram |
| 15 | PerProgramUBO_WaterHazeV | 256 | 1 PerProgram |
| 16 | PerProgramUBO_VisualizeBuffersF | 256 | 1 PerProgram |
| 17 | PerProgramUBO_GodraysF | 256 | 1 PerProgram |
| 18 | PerProgramUBO_VolumetricLightF | 256 | 1 PerProgram |
| 19 | PerProgramUBO_VelocityAlphaV | 256 | 1 PerProgram |
| 20 | PerProgramUBO_PostDeferredF | 256 | 1 PerProgram |
| 21 | PerProgramUBO_CofF | 256 | 1 PerProgram |
| 22 | PerProgramUBO_BlurLightF | 256 | 1 PerProgram |
| 23 | PerProgramUBO_WaterF | 256 | 1 PerProgram |
| 24 | PerProgramUBO_PbrTerrainV | 256 | 1 PerProgram |
| 25 | PerProgramUBO_PointLightF | 256 | 1 PerProgram |

**bind pattern**: per-draw cadence は ring buffer 経路 (= `sDrawUboRingBufferMgr`) で dynamic offset rotation。per-program cadence は program bind 時 set=2 帯一括更新。

### §1.4 set 3 (= Asset + Skin + Legacy 大同居帯、`llvkloader.cpp:862` `V3A_ASSET_SET_BINDINGS = 3` literal)

**Asset/Skin 帯 (binding=0..2)**:

| binding | UBO 名 | size | cadence |
|---|---|---|---|
| 0 | Asset_GLTFNodes | 16384 | 3 PerAsset |
| 1 | Asset_GLTFMaterials | 16384 | 3 PerAsset |
| 2 | Skin_GLTFJoints | 16384 | 4 PerSkin |

**Legacy 帯 (binding=0..62、cadence=1 PerProgram)**:

| binding | UBO 名 | size | 備考 |
|---|---|---|---|
| 0 | AtmoExtraUBO_Legacy | 256 | **binding=0 衝突候補** (Asset_GLTFNodes と同 binding、cadence 別) |
| 1 | SkyVParamUBO_Legacy | 256 | **binding=1 衝突候補** (Asset_GLTFMaterials と同 binding、cadence 別) |
| 2 | SkyFParamUBO_Legacy | 256 | **binding=2 衝突候補** (Skin_GLTFJoints と同 binding、cadence 別) |
| 3 | CloudsVParamUBO_Legacy | 256 | |
| 4 | CloudsFParamUBO_Legacy | 256 | |
| 5 | SoftenLightParamUBO_Legacy | 256 | |
| 6 | DeferredUtilParamUBO_Legacy | 256 | |
| 7 | ShadowUtilParamUBO_Legacy | 512 | 大物 |
| 8 | AOUtilParamUBO_Legacy | 256 | |
| 9 | WaterFogUBO_Legacy | 256 | |
| 10 | TonemapUBO_Legacy | 256 | |
| 11 | GlobalFParamUBO_Legacy | 256 | |
| 12 | CASParamUBO_Legacy | 256 | |
| 13 | PBROpaqueExtraUBO_Legacy | 256 | |
| 14 | SMAAParamUBO_Legacy | 256 | |
| 17 | ReflectionProbeUBO_Legacy | 256 | |
| 18 | GlowFParamUBO_Legacy | 256 | |
| 19 | GlowVParamUBO_Legacy | 256 | |
| 20 | GlowExtractFParamUBO_Legacy | 256 | |
| 21 | PbrShadowAlphaMaskVParamUBO_Legacy | 256 | |
| 22 | AvatarAlphaShadowVParamUBO_Legacy | 256 | |
| 24 | DofCombineFParamUBO_Legacy | 256 | |
| 25 | ExposureFParamUBO_Legacy | 256 | |
| 26 | LuminanceFParamUBO_Legacy | 256 | |
| 27 | MotionBlurFParamUBO_Legacy | 256 | |
| 28 | ScreenSpaceReflPostFParamUBO_Legacy | 256 | |
| 30 | SkinSSSPrototypeFParamUBO_Legacy | 256 | |
| 32 | ClipFParamUBO_Legacy | 256 | |
| 33 | NormgenFParamUBO_Legacy | 256 | |
| 37 | NormaldebugVParamUBO_Legacy | 256 | debug |
| 38 | SnapshotFrameFParamUBO_Legacy | 256 | |
| 39 | UnderWaterFParamUBO_Legacy | 256 | 14 member |
| 40 | PreviewVParamUBO_Legacy | 768 | 大物 |
| 41 | SimpleColorFParamUBO_Legacy | 256 | |
| 42 | StarsFParamUBO_Legacy | 256 | |
| 43 | SunDiscFParamUBO_Legacy | 256 | |
| 44 | MoonFParamUBO_Legacy | 256 | |
| 45 | StarsVParamUBO_Legacy | 256 | |
| 46 | VignetteParamUBO_Legacy | 256 | |
| 47 | PathfindingVParamUBO_Legacy | 256 | debug |
| 48 | PathfindingNoNormalVParamUBO_Legacy | 256 | debug |
| 49 | GlowCombineFParamUBO_Legacy | 256 | |
| 50 | OcclusionCubeVParamUBO_Legacy | 256 | |
| 51 | RadianceGenFParamUBO_Legacy | 256 | |
| 52 | IrradianceGenFParamUBO_Legacy | 256 | |
| 53 | PbrOpaqueVParamUBO_Legacy | 256 | |
| 54 | AvatarFParamUBO_Legacy | 256 | |
| 55 | VelocityVParamUBO_Legacy | 256 | |
| 56 | RlvFParamUBO_Legacy | 256 | RLVa |
| 57 | AvatarClothVParamUBO_Legacy | 256 | |
| 58 | GaussianFParamUBO_Legacy | 256 | |
| 60 | WaterVParamUBO_Legacy | 256 | |
| 61 | TerrainVParamUBO_Legacy | 256 | |
| 62 | SMAABlendWeightsFParamUBO_Legacy | 256 | |

**bind pattern**: program 切替時に set=3 帯一括 bind (= `bindV3aStatic` / `bindV3aRigged` 経路、`llvkloader.cpp:2168/2172/2192/2233` literal)、cadence 別経路で 衝突 binding は subset 分離想定 (= verify 要、§8.1 不明事項)

---

## §2. cadence cluster 関係 (= flush timing 別 cluster)

cadence_tag mapping (= `llglslshader.cpp:95-99` literal、INDEX §1):

### §2.1 cadence_tag=0 PerFrame (= 3 UBO、`flushFrameUbos` 共通経路、`llvkloader.cpp:5117`)

- FrameViewProj
- FrameLights
- FrameAtmosphere_Lighting

= frame start で全 3 UBO 一括 write + bind、`sFrameUboInstances` map<block_hash, UboInstance> 経路 (= `llvkloader.cpp:653`)

### §2.2 cadence_tag=1 PerProgram (= 80 UBO、`flushProgramUbos` 共通経路、`llvkloader.cpp:5142`、cadence cluster 最大)

= 全 Legacy UBO (53 件) + PerProgramUBO_* (25 件) + MaterialUBO (10-member) + MaterialUBO_Legacy (8-member) = 80 件、`sProgramUboDirty` map<UboInstanceKey, UboInstance> 経路 (= shader × block_hash key、`llvkloader.cpp:576`)

program bind 時に triple-buffer write + flush。

### §2.3 cadence_tag=2 PerDraw (= 7 UBO、`writeDrawUbo` + ring buffer 経路、`llglslshader.cpp:2151-2167`)

- PerDrawUBO_AvatarSkin
- PerDrawUBO_AvatarVelocity
- PerDrawUBO_ClipPlane
- PerDrawUBO_LightParams (= pilot 通電済「zero IS real data」semantic、Phase 1.E PC-N-13)
- PerDrawUBO_MultiLight
- PerDrawUBO_ObjectSkin
- PerDrawUBO_SkinnedVelocity

= ring buffer (= `sDrawUboRingBufferMgr`、Phase 1.B PC-6β + PC-N-15a per-thread 配線済) 経路で per-draw allocate + memcpy + dynamic_offset 返却。

### §2.4 cadence_tag=3 PerAsset (= 2 UBO、`writeAssetUbo` 共通経路、`llglslshader.cpp:2172-2195`)

- Asset_GLTFMaterials (= pilot 通電済 Phase 1.C PC-7γ-3)
- Asset_GLTFNodes (= 同)

= `sCurrentAsset` (= `LLVKLoader::getCurrentAsset()`) thread_local accessor 経由、`sAssetUboSetV3a × FRAMES_IN_FLIGHT (=3)` triple-buffer 経路 (= `llvkloader.cpp:905/5729-5840`)

### §2.5 cadence_tag=4 PerSkin (= 1 UBO、`flushSkinUbos` 専用経路、`llvkloader.cpp:5250`)

- Skin_GLTFJoints (= pilot real data 通電済 Phase 1.E PC-N-5/11/15c) = **PerSkin cluster 唯一**

= `sCurrentSkin` (= `thread_local LL::GLTF::Skin* sCurrentSkin`、`llvkloader.cpp:677`) 経由、`sSkinUboDirty[<Skin*, block_hash>]` map (= `llvkloader.cpp:578`) で per-instance dirty 管理。

### §2.6 cadence_tag=5 SINGLETON (= 1 UBO、`flushSingletonUbos` 別経路)

- Global_ReflectionProbes (= shell 通電済 Phase 1.C PC-2) = **SINGLETON cluster 唯一**

= process-wide 1 instance、frame 内多 update 許容 (= reflection probe regenerate 等)

---

## §3. shader consume 関係 (= 同 shader file 内同時 consume)

### §3.1 確定済 consume pair (= 各 UBO file §11.3 で確認済)

| shader file | 同時 consume UBO 群 |
|---|---|
| `class1/deferred/aoUtil.glsl` | AOUtilParamUBO_Legacy + FrameViewProj |
| `class1/gltf/pbrmetallicroughnessV.glsl` | Asset_GLTFMaterials + Asset_GLTFNodes + Skin_GLTFJoints + FrameViewProj (推定) |
| `class1/lighting/sumLightsV.glsl` | FrameLights + (推定) FrameAtmosphere_Lighting + FrameViewProj |
| `class1/lighting/sumLightsSpecularV.glsl` | FrameLights + (同) |
| `class1/environment/waterFogF.glsl` | FrameLights + WaterFogUBO_Legacy + (推定) FrameViewProj |
| `class1/deferred/exposureF.glsl` | ExposureFParamUBO_Legacy + (推定) Frame 系 |
| `class3/deferred/spotLightF.glsl` | PerProgramUBO_SpotLightF + PerProgramUBO_PointLightV (declared-but-unused、η-28-C type 3) + DeferredUtilParamUBO_Legacy 経由 (重複宣言禁止) |
| `class3/deferred/reflectionProbeF.glsl` | PerDrawUBO_ClipPlane + PerDrawUBO_LightParams + (推定) Global_ReflectionProbes + ReflectionProbeUBO_Legacy |
| `class3/deferred/softenLightF.glsl` | PerDrawUBO_ClipPlane + PerDrawUBO_LightParams + SoftenLightParamUBO_Legacy + ShadowUtilParamUBO_Legacy (shared include) |
| `class1/deferred/deferredUtil.glsl` (shared include) | PerDrawUBO_LightParams + DeferredUtilParamUBO_Legacy |
| `class1/deferred/shadowUtil.glsl` (shared include) | ShadowUtilParamUBO_Legacy + FrameLights (= shadow consume 全 program 共通) |
| `class3/environment/underWaterF.glsl` | UnderWaterFParamUBO_Legacy + FrameLights (`#ifndef FRAME_LIGHTS_DEFINED` guard wrap、underWaterF.glsl:94-100 literal 確認) |
| `class1/environment/waterV.glsl` + `class3/environment/waterF.glsl` | WaterVParamUBO_Legacy (V/F multi-site verified identical) + (推定) Frame 系 + WaterFogUBO_Legacy |
| `class3/deferred/waterHazeV.glsl` + `class3/deferred/waterHazeF.glsl` | PerProgramUBO_WaterHazeV (V/F shared、host 1 bind) |
| `class1/deferred/cloudsF.glsl` | CloudsFParamUBO_Legacy + CloudsVParamUBO_Legacy (fragment 側複製、cloudsF.glsl:79 literal、η-14 path G-β) + (推定) AtmoExtraUBO_Legacy + FrameAtmosphere_Lighting |
| `class1/windlight/atmosphericsFuncs.glsl` (shared) | AtmoExtraUBO_Legacy + FrameAtmosphere_Lighting |
| `class1/deferred/postDeferredTonemap.glsl` | PerProgramUBO_GammaCorrect + PerProgramUBO_ColorGrading (本 batch 同 shader 内共起) + TonemapUBO_Legacy (推定) |
| `class1/deferred/postDeferredGammaCorrect.glsl` | PerProgramUBO_GammaCorrect (2 program 共通 consume) |
| `class1/deferred/pbropaqueF.glsl` | PBROpaqueExtraUBO_Legacy + MaterialUBO + PerDrawUBO_ClipPlane + PerDrawUBO_LightParams + GlobalFParamUBO_Legacy + (推定) Frame 系 + Skin_GLTFJoints |
| `class3/deferred/materialF.glsl` | MaterialUBO_Legacy + (推定) Frame 系 |
| `class3/deferred/multiPointLightF.glsl` | PerDrawUBO_MultiLight + PerDrawUBO_LightParams |
| `class1/avatar/avatarSkinV.glsl` | PerDrawUBO_AvatarSkin |
| `class1/avatar/objectSkinV.glsl` | PerDrawUBO_ObjectSkin (lastMatrixPalette member access) |
| `class1/deferred/avatarVelocityV.glsl` | PerDrawUBO_AvatarVelocity (lastMatrixPalette[45]) |
| `class1/deferred/skinnedVelocityV.glsl` + `skinnedVelocityAlphaV.glsl` | PerDrawUBO_SkinnedVelocity |
| `class1/deferred/avatarV.glsl` | AvatarClothVParamUBO_Legacy + (推定) Frame 系 + PerDrawUBO_AvatarSkin |
| `class1/deferred/avatarF.glsl` | AvatarFParamUBO_Legacy + (推定) Frame 系 + Material 系 |
| `class1/deferred/avatarAlphaShadowV.glsl` | AvatarAlphaShadowVParamUBO_Legacy + (推定) FrameViewProj |
| `class1/deferred/pbrShadowAlphaMaskV.glsl` | PbrShadowAlphaMaskVParamUBO_Legacy + (推定) FrameViewProj |
| `class1/deferred/shadowAlphaMaskV.glsl` | PerProgramUBO_ShadowAlphaMaskV + (推定) FrameViewProj |
| `class1/deferred/shadowCubeV.glsl` | PerProgramUBO_ShadowCubeV + (推定) FrameViewProj |
| `class1/interface/occlusionCubeV.glsl` | OcclusionCubeVParamUBO_Legacy + (推定) FrameViewProj |

### §3.2 多 program 共有 shared include UBO

- **deferredUtil.glsl** = DeferredUtilParamUBO_Legacy + PerDrawUBO_LightParams (= 全 deferred lighting program で link)
- **shadowUtil.glsl** = ShadowUtilParamUBO_Legacy (= sun shadow / spot shadow consume 全 shader、2 site identical = class1 + cinematic_bd)
- **SMAA.glsl** = SMAAParamUBO_Legacy (= edge detection / blend weights / neighborhood blending 全 SMAA pass)
- **atmosphericsFuncs.glsl** = AtmoExtraUBO_Legacy (= windlight consumer 全 program で link)

### §3.3 V/F cross-stage 共有 (= 同 binding を V+F 両 stage で参照)

| UBO | V shader | F shader |
|---|---|---|
| CloudsVParamUBO_Legacy (set=3 binding=3) | cloudsV.glsl (起源) | cloudsF.glsl:79 (複製併存、η-14 path G-β) |
| WaterVParamUBO_Legacy (set=3 binding=60) | waterV.glsl | waterF.glsl:108 (multi-site verified identical) |
| PerProgramUBO_WaterHazeV (set=2 binding=15) | waterHazeV.glsl | waterHazeF.glsl (V+F shared、host 1 bind) |
| PerProgramUBO_PointLightV (set=2 binding=5) | pointLightV.glsl (起源) | spotLightF.glsl:151 (declared-but-unused、η-28-C type 3) |

= 同 pipeline 内で V/F 両 stage が同 binding 参照、host bind 1 回で両 stage に対応

---

## §4. データ依存関係 (= 同 host data source 由来)

### §4.1 SSS skin flag 共有 (= AYAstorm r20 Phase C `aya_sss_skin_flag` member、`llshadermgr.cpp:1611` reserved literal)

3 UBO で重複格納:
- MaterialUBO_Legacy (set=1 binding=0)
- PBROpaqueExtraUBO_Legacy (set=3 binding=13)
- AvatarFParamUBO_Legacy (set=3 binding=54)

= 同 r20 SSS skin flag を 3 UBO で持つ、host 側で 3 UBO 同時 write 必要 or program 識別で 1 UBO のみ write 設計判断要

### §4.2 visual realism enable 共有 (= AYAstorm r14+ `aya_visual_realism_enabled` member)

2 UBO で重複格納:
- AtmoExtraUBO_Legacy (set=3 binding=0、`aya_visual_realism_enabled`)
- SkinSSSPrototypeFParamUBO_Legacy (set=3 binding=30、`aya_visual_realism_enabled_skinsss_legacy` = rename 版、η-6 phase 2-A 範式)

= 同 r14+ visual realism cvar 由来、cross-UBO 同値書込み要

### §4.3 shadow target width 共有 (= `LLShaderMgr::DEFERRED_SHADOW_TARGET_WIDTH` 経由、`pipeline.cpp:8562/8570/8584/8592/12596/12611/12642` 7 site setter)

3 UBO で重複格納:
- PerProgramUBO_ShadowAlphaMaskV (set=2 binding=6)
- PbrShadowAlphaMaskVParamUBO_Legacy (set=3 binding=21)
- AvatarAlphaShadowVParamUBO_Legacy (set=3 binding=22)

= 同 shadow target width を 3 program で持つ、shadow render pass で連動 dirty 必須

### §4.4 box_center / box_size 共有 (= shadow cube + occlusion cube)

2 UBO で重複格納:
- OcclusionCubeVParamUBO_Legacy (set=3 binding=50、occlusion query 用 bounding box)
- PerProgramUBO_ShadowCubeV (set=2 binding=14、shadow cube map 用)

= 同 `BOX_CENTER` / `BOX_SIZE` enum (`llshadermgr.h:157-158`) で異なる用途、host 側 program 識別 + dispatch logic 要

### §4.5 GLTF texture transform 共有 (= `LLShaderMgr::TEXTURE_NORMAL_TRANSFORM` / `TEXTURE_METALLIC_ROUGHNESS_TRANSFORM` 経由、`llfetchedgltfmaterial.cpp:136-140` setter)

3 UBO + 1 bare local 経路:
- PbrOpaqueVParamUBO_Legacy (set=3 binding=53、pbropaqueV.glsl)
- PerProgramUBO_PbrAlphaV (set=2 binding=11、pbralphaV.glsl)
- MaterialUBO (set=1 binding=0、`texture_base_color_transform` + `texture_emissive_transform` でも GLTF transform 系列)
- pbrmetallicroughnessV.glsl (= bare local 計算、`gltf_material_data` UBO 経由 derive)

= 同 GLTF material 由来、material 切替で全 GLTF transform UBO 同時 dirty 候補

### §4.6 water 系 data source 共有 (= LLEnvironment LLSettingsWater + LLDrawPoolWater)

水面描画関連 4 UBO 群:
- WaterFogUBO_Legacy (set=3 binding=9、`waterFogColor` / `waterFogDensity` / `waterFogKS`)
- WaterVParamUBO_Legacy (set=3 binding=60、`waveDir1` / `waveDir2` / `time` / `eyeVec` / `waterHeight` / `lightDir`)
- UnderWaterFParamUBO_Legacy (set=3 binding=39、14 member、`waterFogColor_underwater_legacy` / `waterFogKS_underwater_legacy` / `lightDir_underwater_legacy` / `eyeVec_underwater_legacy` = rename 4 member 含、η-6 §3.3 範式)
- PerProgramUBO_WaterF (set=2 binding=23、`specular` / `blend_factor` / `normScale` / `refScale` / `fresnelScale` / `fresnelOffset` 等)

= 同 LLEnvironment water settings 切替で 4 UBO 連動 dirty 必須、UnderWaterFParamUBO_Legacy の rename 4 member は WaterFog/WaterV と同 host data source からの duplicate

### §4.7 light 系 data source 共有 (= LLPipeline::setupHWLights / mNearbyLights)

light 関連 UBO 群:
- FrameLights (set=0 binding=1、PerFrame、`light_position[8]` / `light_direction[8]` / `light_attenuation[8]` / `light_diffuse[8]` / `light_deferred_attenuation[8]`)
- PerDrawUBO_LightParams (set=2 binding=0、PerDraw、`spot_light_color` / `spot_light_size`)
- PerDrawUBO_MultiLight (set=2 binding=1、PerDraw、`light[16]` / `light_col[16]` + scalar)
- PerProgramUBO_PointLightF (set=2 binding=25、PerProgram、`sun_wash` / `falloff` / `global_light_strength`)
- PerProgramUBO_PointLightV (set=2 binding=5、PerProgram、`center` / `size`)
- PerProgramUBO_SpotLightF (set=2 binding=10、PerProgram、10 member 統合)

= light list 変化時 / cvar 変化時に複数 UBO 連動 dirty、`sun_wash` / `falloff` / `global_light_strength` 等は Point/Spot 両方で持つ double-write 必須

### §4.8 sky preset 共有 (= LLSettingsSky)

sky/cloud/atmospheric 関連 UBO 群:
- AtmoExtraUBO_Legacy (set=3 binding=0、`lightnorm` / `haze_horizon` / `cloud_shadow` / `sun_moon_glow_factor` + AYA r14/r16)
- SkyVParamUBO_Legacy (set=3 binding=1)
- SkyFParamUBO_Legacy (set=3 binding=2、`hdri_split_screen` / `moisture_level` / `droplet_radius` / `ice_level`)
- CloudsVParamUBO_Legacy (set=3 binding=3、`camPosLocal` / `cloud_scale` / `cloud_color`)
- CloudsFParamUBO_Legacy (set=3 binding=4、`cloud_pos_density1/2` / `blend_factor` / `cloud_variance` + AYA r18)
- FrameAtmosphere_Lighting (set=0 binding=2、PerFrame、20 member 大気色)

= sky environment 切替で全 sky 系 UBO 連動 dirty

### §4.9 stars 系 V/F pair

- StarsFParamUBO_Legacy (set=3 binding=42、`blend_factor` / `custom_alpha` / `time`)
- StarsVParamUBO_Legacy (set=3 binding=45、`stars_v_time` = F 側 `time` の rename 版、shader 衝突回避 pattern)

### §4.10 day/night blend 共有 (= `blend_factor` 同名 member)

day cycle 連動 UBO 群:
- StarsFParamUBO_Legacy (set=3 binding=42、`blend_factor`)
- SunDiscFParamUBO_Legacy (set=3 binding=43、`blend_factor`)
- CloudsFParamUBO_Legacy (set=3 binding=4、`blend_factor`)
- CloudsVParamUBO_Legacy (set=3 binding=3、(推定 LLSettingsSky 由来))
- MoonFParamUBO_Legacy (set=3 binding=44、`moon_brightness` = LLSettingsSky day cycle 連動)

### §4.11 reflection probe 系 (= LLReflectionMapManager 由来)

- Global_ReflectionProbes (set=0 binding=3、SINGLETON、process-wide)
- ReflectionProbeUBO_Legacy (set=3 binding=17、PerProgram、`max_probe_lod` / `transparent_surface`)
- RadianceGenFParamUBO_Legacy (set=3 binding=51、`sourceIdx` / `u_width` / `mipLevel` / `max_probe_lod` / `probe_strength`)
- IrradianceGenFParamUBO_Legacy (set=3 binding=52、`sourceIdx` / `max_probe_lod`、PC-N-5 register 設計同類)
- GaussianFParamUBO_Legacy (set=3 binding=58、reflection mip blur、`gGaussianProgram` 経由)

= reflection probe regenerate 時に 5 UBO 連動 dirty 候補

### §4.12 velocity 系 (= last frame matrix 由来)

velocity / motion blur 関連 UBO 群:
- PerDrawUBO_AvatarVelocity (set=2 binding=0、`lastMatrixPalette[45]`、`mGLMp` / `mLastGLMp` 由来、`lldrawpool.cpp:1021` setter)
- PerDrawUBO_SkinnedVelocity (set=2 binding=0、`lastMatrixPalette_skinned_velocity[110]`、object-side、ObjectSkin と同 data 由来)
- PerDrawUBO_ObjectSkin (set=2 binding=0、`matrixPalette[110]` + `lastMatrixPalette[110]` = 2 mat3x4 配列)
- PerDrawUBO_AvatarSkin (set=2 binding=0、`matrixPalette[45]` = current、AvatarVelocity と pair)
- VelocityVParamUBO_Legacy (set=3 binding=55、`last_object_matrix` mat4、`lldrawpool.cpp:845/934`、`lldrawpooltree.cpp:202`、`lldrawpoolterrain.cpp:248` 4 setter)
- PerProgramUBO_VelocityAlphaV (set=2 binding=19、`last_object_matrix` mat4、velocityAlphaV.glsl)
- MotionBlurFParamUBO_Legacy (set=3 binding=27、`motion_blur_strength`、cvar 由来、post-pass)

= avatar frame 切替時 (= bone pose 更新) で curr/prev 対称 UBO 同時 dirty、object draw 時に VelocityVParamUBO_Legacy + PerProgramUBO_VelocityAlphaV 同時 dirty 候補

### §4.13 SSAO 系 (= LLPipeline SSAO)

- AOUtilParamUBO_Legacy (set=3 binding=8、`RenderSSAO*` cvar 由来)
- PerProgramUBO_BlurLightF (set=2 binding=22、SSAO blur kernel)
- SoftenLightParamUBO_Legacy (set=3 binding=5、`ssao_irradiance_scale` / `ssao_irradiance_max` / `ssao_effect_mat`)

### §4.14 DoF / post-pass screen res 系 (= screen resolution 由来)

window resize 時連動 UBO 群:
- DofCombineFParamUBO_Legacy (set=3 binding=24、`dof_width` / `dof_height`)
- CASParamUBO_Legacy (set=3 binding=12、`out_screen_res`)
- PerProgramUBO_FxaaF (set=2 binding=9、`rcp_screen_res` 等)
- PerProgramUBO_CofF (set=2 binding=21、DoF CoF params)
- PerProgramUBO_PostDeferredF (set=2 binding=20、`res_scale` / `chroma_str`)
- PerProgramUBO_PostDeferredNoDoFF (set=2 binding=12、`chroma_str`)
- PerProgramUBO_PostDeferredV (set=2 binding=7、`tc_scale`)
- SMAAParamUBO_Legacy (set=3 binding=14、`SMAA_RT_METRICS`)
- ScreenSpaceReflPostFParamUBO_Legacy (set=3 binding=28、`zNear` / `zFar`)
- FrameViewProj (set=0 binding=0、`screen_res`)

### §4.15 exposure / tonemap 系 (= LLPipeline post-process)

- ExposureFParamUBO_Legacy (set=3 binding=25、`dt` / `noiseVec` / `dynamic_exposure_params` / `dynamic_exposure_params2`)
- LuminanceFParamUBO_Legacy (set=3 binding=26、`diffuse_luminance_scale`)
- TonemapUBO_Legacy (set=3 binding=10、`exposure` / `tonemap_mix` / `tonemap_type`)
- PerProgramUBO_GammaCorrect (set=2 binding=4、display gamma)
- PerProgramUBO_ColorGrading (set=2 binding=4、color saturation / contrast / temperature / brightness / LUT)
- VignetteParamUBO_Legacy (set=3 binding=46、`vignette` vec3)
- PerProgramUBO_PostDeferredF / NoDoFF (= 同 `chroma_str` cvar 由来)
- GlowCombineFParamUBO_Legacy (set=3 binding=49、`greyscale_str` / `sepia_str` / `num_colors` = color grading)

### §4.16 glow chain (= LLPipeline::renderPostProcess glow pipeline)

- GlowExtractFParamUBO_Legacy (set=3 binding=20、glow extract、`pipeline.cpp:9049-9099`)
- GlowVParamUBO_Legacy (set=3 binding=19、glow blur V、`glowDelta`、horizontal/vertical 2 pass)
- GlowFParamUBO_Legacy (set=3 binding=18、glow blur F、`glowStrength`)
- GlowCombineFParamUBO_Legacy (set=3 binding=49、glow combine、color grading)

= sequential dispatch: extract → blur (V + F pair × 2 pass) → combine

### §4.17 camPosLocal 共有 (= camera local position)

- CloudsVParamUBO_Legacy (set=3 binding=3)
- SkyVParamUBO_Legacy (set=3 binding=1)
- MaterialUBO_Legacy (set=1 binding=0、`camPosLocal`)
- (他 grep 結果) class3/deferred/materialF.glsl / class1/deferred/cloudsF.glsl 等で同名 access

= 同 `LLViewerCamera::getOrigin()` 経由 local space 由来、複数 UBO で持つ可能性 (= verify 要)

---

## §5. dirty 連動関係 (= 1 UBO dirty 時に同時 dirty)

### §5.1 確定 dirty 連動 group

| trigger event | 同時 dirty UBO 群 |
|---|---|
| frame 開始 (= camera + viewport state 確定) | FrameViewProj + FrameLights + FrameAtmosphere_Lighting (= per-frame 3 UBO 全件) |
| environment / sky preset 切替 | AtmoExtraUBO_Legacy + SkyVParamUBO_Legacy + SkyFParamUBO_Legacy + CloudsVParamUBO_Legacy + CloudsFParamUBO_Legacy + MoonFParamUBO_Legacy + StarsFParamUBO_Legacy + StarsVParamUBO_Legacy + SunDiscFParamUBO_Legacy + FrameAtmosphere_Lighting + FrameLights (= sun_dir 連動) |
| LLEnvironment water settings 切替 | WaterFogUBO_Legacy + WaterVParamUBO_Legacy + UnderWaterFParamUBO_Legacy + PerProgramUBO_WaterF + PerProgramUBO_WaterHazeV (推定) |
| avatar pose 更新 (= bone animation) | PerDrawUBO_AvatarSkin + PerDrawUBO_AvatarVelocity (= curr/prev 対称、frame swap で current → last shift) |
| attachment object draw | PerDrawUBO_ObjectSkin + PerDrawUBO_SkinnedVelocity (= 同 mesh の curr/prev) |
| per-draw object 切替 (= velocity track) | VelocityVParamUBO_Legacy + PerProgramUBO_VelocityAlphaV (推定、cadence 矛盾あり) |
| material 切替 (= GLTF) | MaterialUBO + Asset_GLTFMaterials + PbrOpaqueVParamUBO_Legacy + PerProgramUBO_PbrAlphaV (= GLTF texture transform 共有) |
| asset 切替 (= GLTF asset) | Asset_GLTFMaterials + Asset_GLTFNodes (= 同 asset 由来) |
| reflection probe regenerate | Global_ReflectionProbes + ReflectionProbeUBO_Legacy + RadianceGenFParamUBO_Legacy + IrradianceGenFParamUBO_Legacy + GaussianFParamUBO_Legacy (推定) |
| shadow target resize | PerProgramUBO_ShadowAlphaMaskV + PbrShadowAlphaMaskVParamUBO_Legacy + AvatarAlphaShadowVParamUBO_Legacy (= 同 shadow_target_width) + ShadowUtilParamUBO_Legacy |
| window resize (= screen res 変化) | DofCombineFParamUBO_Legacy + CASParamUBO_Legacy + PerProgramUBO_FxaaF + SMAAParamUBO_Legacy + ScreenSpaceReflPostFParamUBO_Legacy + FrameViewProj.screen_res (= 全 post-process target サイズ依存) |
| mirror pass 突入 | GlobalFParamUBO_Legacy (`mirror_flag` / `clipSign`) + Global_ReflectionProbes + ReflectionProbeUBO_Legacy (推定) |
| `RenderChromaStrength` cvar 変化 | PerProgramUBO_PostDeferredF + PerProgramUBO_PostDeferredNoDoFF (= 同 cvar 由来 double-write) |
| `RenderGlobalLightStrength` / `RenderDeferredSunWash` cvar 変化 | PerProgramUBO_PointLightF + PerProgramUBO_SpotLightF (= 同 cvar 由来 double-write) |
| AYA r20 SSS skin flag cvar 変化 | MaterialUBO_Legacy + PBROpaqueExtraUBO_Legacy + AvatarFParamUBO_Legacy (= 3 UBO triple-write) |
| AYA r14+ visual realism enable cvar 変化 | AtmoExtraUBO_Legacy + SkinSSSPrototypeFParamUBO_Legacy (= cross-UBO 同値書込み) |
| glow chain dispatch (= extract → blur → combine sequential) | GlowExtractFParamUBO_Legacy → GlowVParamUBO_Legacy + GlowFParamUBO_Legacy (同 pass) → GlowCombineFParamUBO_Legacy |
| SMAA pass chain dispatch | SMAAParamUBO_Legacy (shared) + SMAABlendWeightsFParamUBO_Legacy (= blend weights pass) |
| exposure 計算更新 | ExposureFParamUBO_Legacy + LuminanceFParamUBO_Legacy + TonemapUBO_Legacy (= post-process chain 連動) |
| pathfinding debug mode 切替 | PathfindingVParamUBO_Legacy + PathfindingNoNormalVParamUBO_Legacy (= 兄弟 UBO、`tint` / `alpha_scale` 共有) |
| RLVa sphere effect 切替 | RlvFParamUBO_Legacy 単独 (= 独立 dirty) |
| snapshot UI 切替 | SnapshotFrameFParamUBO_Legacy 単独 (= 独立 dirty) |

### §5.2 cvar 変更 trigger による dirty 連動 (= settings.xml cvar listener)

- `RenderSSAO*` cvar → AOUtilParamUBO_Legacy + PerProgramUBO_BlurLightF + SoftenLightParamUBO_Legacy
- `RenderGlow*` cvar → glow chain 4 UBO
- `RenderTonemap*` cvar → TonemapUBO_Legacy + PerProgramUBO_GammaCorrect + PerProgramUBO_ColorGrading
- `RenderMotionBlurStrength` cvar → MotionBlurFParamUBO_Legacy 単独
- `RenderShadowBias` / `RenderShadowOffset` / `RenderShadowSoftness` cvar → ShadowUtilParamUBO_Legacy 単独

### §5.3 独立 dirty (= 他 UBO と連動なし)

- ClipFParamUBO_Legacy (= manip translate visible 切替時のみ)
- NormaldebugVParamUBO_Legacy (= debug setting 切替)
- NormgenFParamUBO_Legacy (= bump texture 生成時のみ)
- AvatarClothVParamUBO_Legacy (= cloth simulation tick 単独)
- PreviewVParamUBO_Legacy (= preview UI 単独)
- TerrainVParamUBO_Legacy (= terrain region 切替単独)
- SimpleColorFParamUBO_Legacy (= camera 水中/水上 cross 時単独、PerProgramUBO_WaterHazeV.above_water と data source 共有候補)

---

## §6. layout 共有関係 (= sAYAStandardLayout 5-set V3a layout)

**全 94 UBO 共通**:

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout (= Phase 1.A 確立、`llvkloader.cpp:881` literal)
- set 0 = Frame 帯 (4 UBO)
- set 1 = Material 帯 (2 UBO 排他)
- set 2 = PerDraw + PerProgram 帯 (26 UBO、binding=0 6 UBO 共有)
- set 3 = Asset + Skin + Legacy 大同居帯 (60+ UBO)
- set 4 = (Phase 1.A 設計済、Phase 2 着手前確認要)

= 全 UBO で同一 pipeline layout 共有、shader 改変ゼロ前提で OS-2 gate 充足

---

## §7. bind 順序関係 (= frame 内 timing)

### §7.1 frame 開始 (= set=0 一括 bind)

- frame start で `vkCmdBindDescriptorSets` 1 回呼出で set=0 全 4 UBO 同時 bind (= `llvkloader.cpp:2839` literal)
- frame 内 stable (= matrix stack 変化は memcpy + dirty.store のみ、bind 自体は再発火しない)

### §7.2 program 切替 (= set=1 + set=2 + set=3 帯 更新)

- program 切替時 (= LLGLSLShader::bind) に各 set 帯の binding 更新
- per-program cadence は triple-buffer flush 経路
- per-draw cadence は ring buffer dynamic_offset rotation

### §7.3 per-draw cadence (= ring buffer slot + dynamic_offset)

- `bindV3aStatic` / `bindV3aRigged` 経路 (= `llvkloader.cpp:2168/2172/2192/2233`) で set=2 帯 4 binding 同時 bind
- 各 draw call で `V3A_DRAW_SET_BINDINGS=4` 個の dynamic_offset 渡し

### §7.4 per-asset cadence (= GLTF asset 切替)

- recordGltfAssetDraw 内 `sCurrentAsset` 切替後に set=3 binding=0/1 (Asset_GLTFNodes / Asset_GLTFMaterials) 更新
- triple-buffer (= `sAssetUboSetV3a × FRAMES_IN_FLIGHT (=3)`、`llvkloader.cpp:905`)

### §7.5 per-skin cadence (= Skin instance 切替)

- recordGltfAssetDraw 内 `sCurrentSkin` 切替後 `wireSkinUboSetV3aToBinding2(sCurrentSkin)` (= `llvkloader.cpp:3180`) で set=3 binding=2 更新
- Phase 1.E PC-N-15c real Skin path 一本化

### §7.6 SINGLETON cadence (= Global_ReflectionProbes)

- frame 内多 update OK (= reflection probe regenerate 時)、`flushSingletonUbos` 別経路
- set=0 binding=3 として frame start 時 set=0 一括 bind に同梱

### §7.7 V/F cross-stage 共有 bind

- 同 binding を V+F 両 stage で参照する pattern:
  - WaterVParamUBO_Legacy (set=3 binding=60、waterV/waterF)
  - PerProgramUBO_WaterHazeV (set=2 binding=15、waterHazeV/waterHazeF、host 1 bind で V+F 両 stage 対応)
  - CloudsVParamUBO_Legacy (set=3 binding=3、cloudsV 起源 + cloudsF 複製併存)
  - PerProgramUBO_PointLightV (set=2 binding=5、pointLightV 起源 + spotLightF declared-but-unused、η-28-C type 3)

= Vulkan 仕様上 descriptor set bind は pipeline 単位、stage visibility は VkShaderStageFlags (VERTEX | FRAGMENT) で制御

### §7.8 glow chain sequential dispatch

- pass 単位 program bind、connected pass で連続 program bind:
  - Pass 1: GlowExtractFParamUBO_Legacy (set=3 binding=20)
  - Pass 2: GlowVParamUBO_Legacy (binding=19) + GlowFParamUBO_Legacy (binding=18) 同 pass、horizontal/vertical 2 dispatch
  - Pass 3: GlowCombineFParamUBO_Legacy (binding=49)

### §7.9 SMAA pass chain sequential dispatch

- SMAA edge detection → blend weights → neighborhood blending 3 pass:
  - SMAAParamUBO_Legacy (= shared、全 pass)
  - SMAABlendWeightsFParamUBO_Legacy (= blend weights pass のみ)

---

## §8. 不明事項集約 (= 各 UBO file §10 + §11 不明引用、Phase 2 設計入力)

### §8.1 set/binding 衝突候補 verify 要 (= INDEX §4.1 集約)

- **set=3 binding=0 衝突**: AtmoExtraUBO_Legacy + Asset_GLTFNodes (= cadence 別 = PerProgram vs PerAsset、subset 分離経路 verify 要)
- **set=3 binding=1 衝突**: SkyVParamUBO_Legacy + Asset_GLTFMaterials (= cadence 別)
- **set=3 binding=2 衝突**: SkyFParamUBO_Legacy + Skin_GLTFJoints (= cadence 別 = PerProgram vs PerSkin、subset 分離経路 verify 要)
- **set=2 binding=0 共有 6 UBO**: PerDrawUBO_AvatarSkin / AvatarVelocity / ClipPlane / LightParams / ObjectSkin / SkinnedVelocity (= 全 PerDraw cadence、program 識別 dispatch logic 未確立、verify 要)
- **set=2 binding=4 衝突**: PerProgramUBO_GammaCorrect (binding=4 か別 binding か誤記 verify 要、blueprint 確認 PerProgramUBO_ColorGrading は binding=4 確定、PerProgramUBO_GammaCorrect は binding=2 が正)
- **set=1 binding=0 排他運用**: MaterialUBO (10-member PBR canonical) + MaterialUBO_Legacy (8-member) (= shader 単位排他選択 logic 未確立、verify 要)

### §8.2 cadence 再評価候補 (= INDEX §4.2 集約)

- **PerProgramUBO_VelocityAlphaV** (cadence_tag=1 PerProgram) vs `last_object_matrix` per-draw 変化 = mismatch 重大、Phase 2 で PerDraw cadence 移行事実上必須
- **VelocityVParamUBO_Legacy** (cadence=1 PerProgram) vs `last_object_matrix` per-object per-frame 変化 = 同 mismatch
- **PerProgramUBO_PbrAlphaV** (cadence_tag=1) vs GLTF material per-draw 切替 = 同種 mismatch
- **PerProgramUBO_FsObjectIdF** (cadence_tag=1) vs r21 self rigged picker per-draw object ID write = semantic 矛盾
- **PerProgramUBO_FullbrightShinyV** / **PostDeferredV** / **PostDeferredF** / **PostDeferredNoDoFF** / **PointLightF** / **SpotLightF** / **PointLightV** (= per-light / per-draw 性質を PerProgram で運ぶ stale risk あり)
- **PerDrawUBO_ObjectSkin** (cadence=2 + size=10752 = 全 UBO 中最大) = ring buffer 圧迫 risk、PerSkin cadence 移行検討要 + PerDrawUBO_SkinnedVelocity と data duplication
- **TonemapUBO_Legacy** / **ShadowUtilParamUBO_Legacy** / **SMAAParamUBO_Legacy** / **ScreenSpaceReflPostFParamUBO_Legacy** / **SoftenLightParamUBO_Legacy** = PerProgram cadence 妥当性 question (= PerFrame / SINGLETON 化候補)
- **SkyVParamUBO_Legacy** / **CloudsVParamUBO_Legacy** / **UnderWaterFParamUBO_Legacy** / **WaterVParamUBO_Legacy** = per-frame 変化 member (`camPosLocal` / `eyeVec` / `screenRes` 等) の per-program cadence stale risk
- **StarsFParamUBO_Legacy** (= `time` per-frame) / **StarsVParamUBO_Legacy** (= `stars_v_time` per-frame) / **SunDiscFParamUBO_Legacy** (= `blend_factor` per-frame) = per-frame 変化の per-program cadence stale risk
- **OcclusionCubeVParamUBO_Legacy** (= per-cube 性質、per-draw 寄り) / **NormgenFParamUBO_Legacy** (= 個別 bump 生成毎)
- **PerProgramUBO_FsObjectIdF** (= cadence_tag=1 PerProgram、r21 per-draw object ID write semantics と矛盾、重大)
- **PerDrawUBO_ClipPlane** (= per-draw 性質、現 PerDraw cadence で適切)
- **RadianceGenFParamUBO_Legacy** (= mip chain ループで複数回 dirty、PerDraw cadence 化候補)

### §8.3 同名 member 別 UBO 重複格納 (= INDEX §4.3 集約)

- `aya_sss_skin_flag` = MaterialUBO_Legacy + PBROpaqueExtraUBO_Legacy + AvatarFParamUBO_Legacy (3 UBO 重複)
- `aya_visual_realism_enabled` (rename 含) = AtmoExtraUBO_Legacy + SkinSSSPrototypeFParamUBO_Legacy
- `box_center` / `box_size` = OcclusionCubeVParamUBO_Legacy + PerProgramUBO_ShadowCubeV
- `shadow_target_width` = PerProgramUBO_ShadowAlphaMaskV + PbrShadowAlphaMaskVParamUBO_Legacy + AvatarAlphaShadowVParamUBO_Legacy (3 UBO 重複)
- `texture_normal_transform` / `texture_metallic_roughness_transform` = PbrOpaqueVParamUBO_Legacy + PerProgramUBO_PbrAlphaV + MaterialUBO + pbrmetallicroughnessV.glsl (bare local)
- `tc_scale` = PerProgramUBO_FxaaF (`rcp_screen_res`?) + PerProgramUBO_PostDeferredV (= 別 component / verify 要)
- `camPosLocal` = CloudsVParamUBO_Legacy + SkyVParamUBO_Legacy + MaterialUBO_Legacy + cloudsF.glsl / materialF.glsl (grep 結果)
- `clipPlane` (vec4) = PerDrawUBO_ClipPlane (per-draw) + ClipFParamUBO_Legacy (`clip_plane`、per-program、別経路) (= 別用途 vs 同 data 不明、verify 要)
- `waterFogColor` / `waterFogKS` = WaterFogUBO_Legacy + UnderWaterFParamUBO_Legacy (rename 版)
- `lightDir` / `eyeVec` = WaterVParamUBO_Legacy + UnderWaterFParamUBO_Legacy (rename 版)
- `lastMatrixPalette` = PerDrawUBO_AvatarVelocity + PerDrawUBO_ObjectSkin + PerDrawUBO_SkinnedVelocity (= 同 mesh の prev frame palette、3 UBO で異 size)
- `matrixPalette` = PerDrawUBO_AvatarSkin + PerDrawUBO_ObjectSkin (= 同 mesh の current frame palette)
- `last_object_matrix` = VelocityVParamUBO_Legacy + PerProgramUBO_VelocityAlphaV
- `chroma_str` = PerProgramUBO_PostDeferredF + PerProgramUBO_PostDeferredNoDoFF
- `sun_wash` / `falloff` / `global_light_strength` = PerProgramUBO_PointLightF + PerProgramUBO_SpotLightF
- `mipLevel` = PerProgramUBO_VisualizeBuffersF + RadianceGenFParamUBO_Legacy + screenSpaceReflUtil.glsl (= 別 program 別 UBO 共通)
- `blend_factor` = StarsFParamUBO_Legacy + SunDiscFParamUBO_Legacy + CloudsFParamUBO_Legacy + PerProgramUBO_WaterF
- `time` = StarsFParamUBO_Legacy + StarsVParamUBO_Legacy (rename `stars_v_time`) + WaterVParamUBO_Legacy
- `max_probe_lod` = ReflectionProbeUBO_Legacy + RadianceGenFParamUBO_Legacy + IrradianceGenFParamUBO_Legacy
- `sourceIdx` = RadianceGenFParamUBO_Legacy + IrradianceGenFParamUBO_Legacy
- `waterSign` = DeferredUtilParamUBO_Legacy + SimpleColorFParamUBO_Legacy
- `tint` / `alpha_scale` = PathfindingVParamUBO_Legacy + PathfindingNoNormalVParamUBO_Legacy
- `proj_*` 系 (= proj_n / proj_p / proj_focus / proj_lod / proj_range / proj_ambiance) = DeferredUtilParamUBO_Legacy + PerProgramUBO_SpotLightF (= shared include deferredUtil.glsl 経由、spotLightF 側で重複宣言禁止)

### §8.4 SPIR-V 実 offset vs codegen literal 整合性 (= INDEX §4.4 集約)

- **PerProgramUBO_PostDeferredV** (= shader `// offset 0 / 16` コメント vs codegen `OFFSET = 0u/4u/8u`)
- **PerProgramUBO_GodraysF** (= shader `// offset 0/16/32 + 12 pad` vs codegen packed)
- **PerProgramUBO_PostDeferredF** (= shader `// offset 16 + 12 pad` vs codegen `OFFSET = 4u`)

= shader 側 comment は documentation のみ、SPIR-V compile 後の実 offset は codegen 通り想定、Phase 2 cold launch validation で確認要

### §8.5 member_count 表記揺れ (= INDEX §4.5 集約)

- **SoftenLightParamUBO_Legacy** = ubo_metadata.inl `member_count=8` vs blueprint header note=9 検出、INDEX §2 + RELATIONS は metadata 一致で 8 採用
- **CASParamUBO_Legacy** = ubo_metadata.inl `member_count=5` vs blueprint comment 「6 member」表記 = blueprint comment 誤記 or 旧 design 残骸

### §8.6 同 host data source 重複書込み (= INDEX §4.6 集約)

- **WaterFog / UnderWater / WaterV / WaterF / WaterHazeV** = LLEnvironment LLSettingsWater 由来、複数 UBO 同時 update、Phase 2 で flush 最適化候補
- **PerProgramUBO_PointLightF / SpotLightF** = `sun_wash` / `falloff` / `global_light_strength` 同 cvar 由来 double-write
- **PerProgramUBO_PostDeferredF / NoDoFF** = `RenderChromaStrength` cvar 同 double-write
- **shadow_target_width 3 UBO** = `pipeline.cpp:8562+` 7 site setter から 3 UBO 同時 write
- **aya_sss_skin_flag 3 UBO** = SSS skin flag triple-write
- **GLTF texture transform** = pbropaqueV / pbralphaV / pbrmetallicroughnessV 経由で複数 UBO 同 data

### §8.7 set=3 帯 bind 単位 verify (= INDEX §4.7)

- set=3 帯 ~58 UBO の `vkCmdBindDescriptorSets` bind 単位 = set 全体 rebind vs binding 個別 rebind = `llvkloader.cpp` 詳細 verify 要 (= 全 set=3 UBO 共通)
- program 切替時に set=3 帯全 binding rebind か個別 rebind か (= 全 Legacy UBO 共通不明)

### §8.8 cinematic_bd 系 r30+ chapter member の host writer (= INDEX §4.8)

- `cinematic_bd_*` member 含む UBO (= ShadowUtilParamUBO_Legacy multi-site 確認、PerProgramUBO_ColorGrading r30 Cinematic Control 13 cvar 関連) の host C++ writer 経路 = r30+ chapter 実装で個別 grep verify 要

### §8.9 V3a 5-set 設計と Legacy UBO 実 binding 配置の整合性 (= INDEX §4.9)

- `llvkloader.cpp:862` literal `V3A_ASSET_SET_BINDINGS = 3` だが metadata 上 set=3 Legacy UBO の binding 値は 1..62 まで分布 = V3a 設計と codegen 実態の乖離 verify 要 (= 全 Legacy UBO 共通)

### §8.10 shell 通電有無 verify (= INDEX §4.10)

- 全 untouched 確認済多数 UBO で **Phase 1.C PC-2/PC-7δ で shell 通電されたか否か** = handoff doc chain (`handoff/phase1/c/`) 参照要 (= 本 file 起案では design-phase 規律で Read 未実施、verify 要)

### §8.11 各 UBO の host owner class (= INDEX §4.11)

- 全 94 UBO の owner class (= LLPipeline / LLReflectionMapManager / LLViewerCamera / LLEnvironment / RlvHandler / LLWaterParamManager / LLDrawPool* 等) = 各 file §4 で記載されているが、未確定多数、Phase 2 設計再開時に grep verify 要

### §8.12 AYAstorm r14-r28 章追加 cvar 経由 writer 経路 (= INDEX §4.12)

- AYAstorm r14/r16/r18/r20/r24/r28 章追加 cvar (= `aya_visual_realism_enabled` / `aya_r14_*` / `aya_r16_*` / `aya_r18_*` / `aya_r20_*` / `aya_sss_skin_flag` / `aya_translucency_*` / `aya_glow_*` 等) の host C++ writer 経路 = 本調査で特定できず、各章実装で個別 grep verify 要

### §8.13 LLStaticHashedString uniform path verify

- CASParamUBO_Legacy (= `pipeline.cpp:9182-9200` literal `LLStaticHashedString` 経由 setter)
- ClipFParamUBO_Legacy (= `llmaniptranslate.cpp:1715-1716` 同経路)
- LuminanceFParamUBO_Legacy (= `pipeline.cpp:8753-8754` 同経路)
- PerProgramUBO_VisualizeBuffersF (= `pipeline.cpp:8707-8711` 同経路)
- ExposureFParamUBO_Legacy (= `pipeline.cpp:8815` `LLStaticHashedString dt` 同経路)

= LLStaticHashedString 引数版 uniform setter (= name-based) が UBO redirect path (= `mUniformUBOLoc` index-based) に正しく到達するか verify 必須 (= memory `feedback_render_full_trace_first` 遵守要)

### §8.14 cross-UBO same data dirty 同期 protocol (= 設計入力)

`aya_visual_realism_enabled` / `aya_sss_skin_flag` / `shadow_target_width` / `chroma_str` / `sun_wash` 等の同 host data 由来 cross-UBO member の dirty 連動 update protocol が未設計 = host C++ 側 cvar listener / event hook 単一経路で複数 UBO 同時 dirty 配線必要

### §8.15 declared-but-unused / dead uniform 整理

- PerProgramUBO_PointLightF (= `sun_wash` dead、shader 本体未参照だが host setter あり)
- PerProgramUBO_VolumetricLightF (= `seconds60` BD legacy dead)
- PerProgramUBO_WaterF (= `kd` declared-but-unused)
- DofCombineFParamUBO_Legacy (= `dof_height` reserved list 漏れ確認要)
- DeferredUtilParamUBO_Legacy (= `PROJECTOR_NEAR` 所属 UBO 不明)

= 全 dead/unused member は layout 不変契約のため Phase 2 でも維持必須、host setter は no-op 化可能

---

## §A. メタ情報

### §A.1 起案 source

- 各 UBO file §11.1〜§11.7 + §10 全 94 file (= sequential 1 file 1 Read 順次精査)
- INDEX.md §4 横断不明事項 12 件
- 各 file 内 reserved literal source (= `llshadermgr.cpp/.h` / `llglslshader.cpp` / `pipeline.cpp` / `llvkloader.cpp` 等)

### §A.2 完成 verify

- 全 94 UBO の set/binding/cadence 一元集約 ✅ (§1)
- cadence cluster 全 6 種集約 ✅ (§2)
- shader consume 確定済 + 推定明示 ✅ (§3)
- 17 data source 系列集約 ✅ (§4.1-§4.17)
- 確定済 dirty 連動 group 20+ ✅ (§5)
- layout 共有 ✅ (§6)
- bind 順序 9 種類 ✅ (§7)
- 不明事項 15 dimension 集約 ✅ (§8)

### §A.3 関連 doc

- INDEX.md (= 全 94 UBO summary + cadence_tag mapping)
- 各 UBO file (`<UBO名>.md`) = 詳細個別資料
- READINESS.md = A/B/C 実装判定資料 (= 同 dir)
- handoff `phase2-prep/handoff-phase2-prep-ubo-files-complete.md` = 残作業 + 起案規律
- **Phase 2.α 案 X cross-ref** (= 2026-06-06、UBO codegen 入力 source 整合修復 sub-phase):
  - `handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D.9 (= 案 X 確定 source of truth)
  - `handoff/phase2/alpha/handoff-phase2-alpha-3-cmake-blueprint-readme-propagation.md` §3.1-§3.7 (= α-3 7 phase 構造)
  - `indra/newview/app_settings/shaders/aya_r41_blueprints/README.md` (= blueprint dir = **codegen 入力 source of truth** literal)
  - **案 X 整合**: §3.3 V/F cross-stage 共有 (= CloudsV/F + WaterV/F 等同 binding 多 stage 参照) と §8 不明事項集約は **blueprint dir 単独 source** + actual との二重 source 同期 protocol で formal化 (= main.py phase F commit `868bc38cc9` `_verify_block_match` + `_verify_blueprint_actual_consistency` + `--verify-target-paths` option)。本 doc 内 blueprint 言及 (= §1.2 / §3.2 / §3.3 等の `blueprint header literal` / `blueprint コメント verified identical` 引用) は全件 GLSL declaration source 引用、案 X 整合済。

---

## §B. Phase 2.α 案 X 確定 record (= blueprint dir 位置付け + 二重 source 同期 protocol 詳細)

### §B.1 blueprint dir = codegen 入力 source of truth (= 案 X 確定 2026-06-06)

§A.3 末尾に概要記載済、本 §B で詳細補足:

- **codegen 入力 source of truth path** = `indra/newview/app_settings/shaders/aya_r41_blueprints/<set>/<ubo_lower>.glsl` (= 94 file = ubo_metadata.inl g_block_count=94 整合)
- **本 doc 内 blueprint 言及 (= §1.2 / §3.2 / §3.3 / §3.5 / §5 等)** = 全件 GLSL declaration source 引用 (= blueprint header literal `verified identical across N sample sites` / blueprint コメント / blueprint origin / blueprint comment `member_count` 不一致 等) であり、案 X 確定 (= blueprint dir = codegen 入力 source of truth) 整合済。
- **AYAstorm shader runtime compile target は別 GLSL 系統** (= `class*/` + `cinematic_bd/` 配下の実 shader use site) で並列 build process (= design/04-codegen-ubo.md §2.2 literal「別 GLSL 並列 build process」)。本 doc §3 全 shader consume mapping の path = actual GLSL 系統、blueprint dir 内 file path は §1.2 等で別途 source として参照。

### §B.2 二重 source 同期 protocol = main.py で formal化

- **`_verify_block_match`** (= α-2 commit `b66ec99f72`) = 同名 UBO 複数 file (= blueprint + actual の cross-source pair、cinematic_bd 上書き path、CloudsV/F / WaterV/F の V/F cross-stage 共有等) の set/binding + subset/cadence + member 全件 layout 一致を構造的 verify。
- **`_verify_blueprint_actual_consistency`** + **`--verify-target-paths`** option (= phase F commit `868bc38cc9`) = blueprint と actual の二重 source 整合 verify を formal化。
- 本 doc §3.3 V/F cross-stage 共有 (= cloudsV/cloudsF.glsl + waterV/waterF.glsl + skinnedVelocityV/AlphaV.glsl + postDeferredGammaCorrect/Tonemap.glsl 等 verified identical pair) は本 §B.2 protocol で blueprint ↔ actual の cross-source verify が pre-requisite。
- §5 dirty 連動 group (= 同 data source 複数 UBO triple-write / pair UBO 等) の各 UBO 改修時、blueprint dir 側と actual GLSL 系統側を **本 §B.2 protocol で同期 verify** する必要あり。

### §B.3 §8 不明事項との関係

- §8.1〜§8.15 dim 15 件不明事項のうち、blueprint comment vs metadata の表記揺れ (= §8 内 SoftenLight `member_count=8 vs blueprint header note=9` / CASParamUBO_Legacy `metadata=5 vs blueprint comment 「6 member」` 等) は本 §B.2 `_verify_block_match` で member_count 検証経路に formal化、blueprint comment 訂正 path が Phase 2.α 案 X 確定で明確化。

### §B.4 cross-ref

- 案 X 確定 source of truth = `handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D.9
- blueprint dir README = `indra/newview/app_settings/shaders/aya_r41_blueprints/README.md` (= phase B commit `f95182ded5`)
- 設計 doc = `design/04-codegen-ubo.md` §2.2 / §4.4
- 二重 source 同期 protocol = `scripts/ubo_codegen/main.py` `_verify_block_match` + `_verify_blueprint_actual_consistency`
- phase E (= blueprint dir 内 7 UBO 同期書換) = commit `09ee5e8a8e`
- 関連 doc = INDEX.md §B / READINESS.md §B / WORK_ORDER.md §B (= 同 dir)
