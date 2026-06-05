# r41 UBO Design Index — 全 94 UBO 個別設計資料 (= 実コードベース調査)

**着手契機**: 2026-06-06 AYA literal record「全 UBO に対して全体設計からみてどのように実装する必要があるのか 94 UBO に対し現状状態で網羅した資料を作成する必要があります (実コードから)」「これを先にせずに Phase 2 の工程などと言うものはそもそも語れるわけがありません」

**位置付け**: Phase 2 = 全 UBO 一括本実装化 (= AYA literal 「Phase 2 で触る UBO はすべてにしてください」record 2026-06-06) の **設計入力資料**。各 UBO の現状状態 + 本実装化に必要な情報を実コード source 直接 reference で網羅。本資料完成までは Phase 2 工程議論不可。

**起案規律**:
- 各 UBO 1 file (= `<UBO 名>.md`)、本 INDEX.md は全 UBO list + summary table のみ
- 全項目 **実コード source 直接 reference** (= 推論禁止)
- 確定できない項目は **「不明」と明示記載** (= memory `feedback_admit_unknown` 遵守)
- 各 file 内 structure = §B 10 項目 (本 INDEX §3 参照)

## §1. 全 94 UBO list (= `ubo_metadata.inl` `g_block_count = 94u` literal source)

source: `build-linux-x86_64/codegen/ubo/ubo_metadata.inl:24` (= `inline constexpr std::uint32_t g_block_count = 94u;`)

cadence_tag mapping (= **確定**、`llglslshader.cpp:95-99` literal source):

| cadence_tag | 意味 | UBO 数 |
|---|---|---|
| 0 | per-frame | 3 |
| 1 | per-program | 80 |
| 2 | per-draw | 7 |
| 3 | per-asset | 2 |
| 4 | per-skin | 1 |
| 5 | **SINGLETON** (= `flushSingletonUbos` 別経路、process-wide 1 instance) | 1 (Global_ReflectionProbes) |
| 合計 | | **94** ✅ |

**source**: `indra/llrender/llglslshader.cpp:99` literal `constexpr U32 kCadenceSingleton = 5u; // Global_ReflectionProbes (flushSingletonUbos 別経路)` + 周辺 enum 定義 = cadence_tag enum 確定。

## §2. UBO list table (= block_name 順、ubo_metadata.inl literal copy)

| # | block_name | set | binding | size (B) | cadence_tag | member_count | 通電状態 | file |
|---|---|---|---|---|---|---|---|---|
| 1 | AOUtilParamUBO_Legacy | 3 | 8 | 256 | 1 | 4 | untouched | [AOUtilParamUBO_Legacy.md](AOUtilParamUBO_Legacy.md) |
| 2 | Asset_GLTFMaterials | 3 | 1 | 16384 | 3 | 1 | pilot infra stub | [Asset_GLTFMaterials.md](Asset_GLTFMaterials.md) |
| 3 | Asset_GLTFNodes | 3 | 0 | 16384 | 3 | 1 | pilot infra stub | [Asset_GLTFNodes.md](Asset_GLTFNodes.md) |
| 4 | AtmoExtraUBO_Legacy | 3 | 0 | 256 | 1 | 10 | untouched | [AtmoExtraUBO_Legacy.md](AtmoExtraUBO_Legacy.md) |
| 5 | AvatarAlphaShadowVParamUBO_Legacy | 3 | 22 | 256 | 1 | 1 | untouched | [AvatarAlphaShadowVParamUBO_Legacy.md](AvatarAlphaShadowVParamUBO_Legacy.md) |
| 6 | AvatarClothVParamUBO_Legacy | 3 | 57 | 256 | 1 | 3 | untouched | [AvatarClothVParamUBO_Legacy.md](AvatarClothVParamUBO_Legacy.md) |
| 7 | AvatarFParamUBO_Legacy | 3 | 54 | 256 | 1 | 4 | untouched | [AvatarFParamUBO_Legacy.md](AvatarFParamUBO_Legacy.md) |
| 8 | CASParamUBO_Legacy | 3 | 12 | 256 | 1 | 5 | untouched | [CASParamUBO_Legacy.md](CASParamUBO_Legacy.md) |
| 9 | ClipFParamUBO_Legacy | 3 | 32 | 256 | 1 | 1 | untouched | [ClipFParamUBO_Legacy.md](ClipFParamUBO_Legacy.md) |
| 10 | CloudsFParamUBO_Legacy | 3 | 4 | 256 | 1 | 8 | untouched | [CloudsFParamUBO_Legacy.md](CloudsFParamUBO_Legacy.md) |
| 11 | CloudsVParamUBO_Legacy | 3 | 3 | 256 | 1 | 4 | untouched | [CloudsVParamUBO_Legacy.md](CloudsVParamUBO_Legacy.md) |
| 12 | DeferredUtilParamUBO_Legacy | 3 | 6 | 256 | 1 | 8 | untouched | [DeferredUtilParamUBO_Legacy.md](DeferredUtilParamUBO_Legacy.md) |
| 13 | DofCombineFParamUBO_Legacy | 3 | 24 | 256 | 1 | 3 | untouched | [DofCombineFParamUBO_Legacy.md](DofCombineFParamUBO_Legacy.md) |
| 14 | ExposureFParamUBO_Legacy | 3 | 25 | 256 | 1 | 4 | untouched | [ExposureFParamUBO_Legacy.md](ExposureFParamUBO_Legacy.md) |
| 15 | FrameAtmosphere_Lighting | 0 | 2 | 256 | 0 | 20 | untouched | [FrameAtmosphere_Lighting.md](FrameAtmosphere_Lighting.md) |
| 16 | FrameLights | 0 | 1 | 768 | 0 | 9 | untouched | [FrameLights.md](FrameLights.md) |
| 17 | FrameViewProj | 0 | 0 | 512 | 0 | 9 | untouched | [FrameViewProj.md](FrameViewProj.md) |
| 18 | GaussianFParamUBO_Legacy | 3 | 58 | 256 | 1 | 2 | untouched | [GaussianFParamUBO_Legacy.md](GaussianFParamUBO_Legacy.md) |
| 19 | GlobalFParamUBO_Legacy | 3 | 11 | 256 | 1 | 4 | untouched | [GlobalFParamUBO_Legacy.md](GlobalFParamUBO_Legacy.md) |
| 20 | **Global_ReflectionProbes** | 0 | 3 | 256 | **5 (不明)** | 1 | **shell 通電済** (= Phase 1.C) | [Global_ReflectionProbes.md](Global_ReflectionProbes.md) |
| 21 | GlowCombineFParamUBO_Legacy | 3 | 49 | 256 | 1 | 4 | untouched | [GlowCombineFParamUBO_Legacy.md](GlowCombineFParamUBO_Legacy.md) |
| 22 | GlowExtractFParamUBO_Legacy | 3 | 20 | 256 | 1 | 5 | untouched | [GlowExtractFParamUBO_Legacy.md](GlowExtractFParamUBO_Legacy.md) |
| 23 | GlowFParamUBO_Legacy | 3 | 18 | 256 | 1 | 1 | untouched | [GlowFParamUBO_Legacy.md](GlowFParamUBO_Legacy.md) |
| 24 | GlowVParamUBO_Legacy | 3 | 19 | 256 | 1 | 1 | untouched | [GlowVParamUBO_Legacy.md](GlowVParamUBO_Legacy.md) |
| 25 | IrradianceGenFParamUBO_Legacy | 3 | 52 | 256 | 1 | 4 | untouched | [IrradianceGenFParamUBO_Legacy.md](IrradianceGenFParamUBO_Legacy.md) |
| 26 | LuminanceFParamUBO_Legacy | 3 | 26 | 256 | 1 | 1 | untouched | [LuminanceFParamUBO_Legacy.md](LuminanceFParamUBO_Legacy.md) |
| 27 | MaterialUBO | 1 | 0 | 256 | 1 | 10 | untouched | [MaterialUBO.md](MaterialUBO.md) |
| 28 | MaterialUBO_Legacy | 1 | 0 | 256 | 1 | 8 | untouched | [MaterialUBO_Legacy.md](MaterialUBO_Legacy.md) |
| 29 | MoonFParamUBO_Legacy | 3 | 44 | 256 | 1 | 1 | untouched | [MoonFParamUBO_Legacy.md](MoonFParamUBO_Legacy.md) |
| 30 | MotionBlurFParamUBO_Legacy | 3 | 27 | 256 | 1 | 1 | untouched | [MotionBlurFParamUBO_Legacy.md](MotionBlurFParamUBO_Legacy.md) |
| 31 | NormaldebugVParamUBO_Legacy | 3 | 37 | 256 | 1 | 1 | untouched | [NormaldebugVParamUBO_Legacy.md](NormaldebugVParamUBO_Legacy.md) |
| 32 | NormgenFParamUBO_Legacy | 3 | 33 | 256 | 1 | 4 | untouched | [NormgenFParamUBO_Legacy.md](NormgenFParamUBO_Legacy.md) |
| 33 | OcclusionCubeVParamUBO_Legacy | 3 | 50 | 256 | 1 | 4 | untouched | [OcclusionCubeVParamUBO_Legacy.md](OcclusionCubeVParamUBO_Legacy.md) |
| 34 | PBROpaqueExtraUBO_Legacy | 3 | 13 | 256 | 1 | 4 | untouched | [PBROpaqueExtraUBO_Legacy.md](PBROpaqueExtraUBO_Legacy.md) |
| 35 | PathfindingNoNormalVParamUBO_Legacy | 3 | 48 | 256 | 1 | 4 | untouched | [PathfindingNoNormalVParamUBO_Legacy.md](PathfindingNoNormalVParamUBO_Legacy.md) |
| 36 | PathfindingVParamUBO_Legacy | 3 | 47 | 256 | 1 | 4 | untouched | [PathfindingVParamUBO_Legacy.md](PathfindingVParamUBO_Legacy.md) |
| 37 | PbrOpaqueVParamUBO_Legacy | 3 | 53 | 256 | 1 | 2 | untouched | [PbrOpaqueVParamUBO_Legacy.md](PbrOpaqueVParamUBO_Legacy.md) |
| 38 | PbrShadowAlphaMaskVParamUBO_Legacy | 3 | 21 | 256 | 1 | 1 | untouched | [PbrShadowAlphaMaskVParamUBO_Legacy.md](PbrShadowAlphaMaskVParamUBO_Legacy.md) |
| 39 | PerDrawUBO_AvatarSkin | 2 | 0 | 768 | 2 | 1 | untouched | [PerDrawUBO_AvatarSkin.md](PerDrawUBO_AvatarSkin.md) |
| 40 | PerDrawUBO_AvatarVelocity | 2 | 0 | 768 | 2 | 1 | untouched | [PerDrawUBO_AvatarVelocity.md](PerDrawUBO_AvatarVelocity.md) |
| 41 | PerDrawUBO_ClipPlane | 2 | 0 | 256 | 2 | 1 | untouched | [PerDrawUBO_ClipPlane.md](PerDrawUBO_ClipPlane.md) |
| 42 | **PerDrawUBO_LightParams** | 2 | 0 | 256 | 2 | 2 | **pilot「zero IS real data」semantic shell** (= Phase 1.E PC-N-13) | [PerDrawUBO_LightParams.md](PerDrawUBO_LightParams.md) |
| 43 | PerDrawUBO_MultiLight | 2 | 1 | 768 | 2 | 6 | untouched | [PerDrawUBO_MultiLight.md](PerDrawUBO_MultiLight.md) |
| 44 | PerDrawUBO_ObjectSkin | 2 | 0 | 10752 | 2 | 2 | untouched | [PerDrawUBO_ObjectSkin.md](PerDrawUBO_ObjectSkin.md) |
| 45 | PerDrawUBO_SkinnedVelocity | 2 | 0 | 5376 | 2 | 1 | untouched | [PerDrawUBO_SkinnedVelocity.md](PerDrawUBO_SkinnedVelocity.md) |
| 46 | PerProgramUBO_AlphaParams | 2 | 3 | 256 | 1 | 4 | untouched | [PerProgramUBO_AlphaParams.md](PerProgramUBO_AlphaParams.md) |
| 47 | PerProgramUBO_BlurLightF | 2 | 22 | 256 | 1 | 8 | untouched | [PerProgramUBO_BlurLightF.md](PerProgramUBO_BlurLightF.md) |
| 48 | PerProgramUBO_CofF | 2 | 21 | 256 | 1 | 8 | untouched | [PerProgramUBO_CofF.md](PerProgramUBO_CofF.md) |
| 49 | PerProgramUBO_ColorGrading | 2 | 4 | 256 | 1 | 8 | untouched | [PerProgramUBO_ColorGrading.md](PerProgramUBO_ColorGrading.md) |
| 50 | PerProgramUBO_FsObjectIdF | 2 | 13 | 256 | 1 | 1 | untouched | [PerProgramUBO_FsObjectIdF.md](PerProgramUBO_FsObjectIdF.md) |
| 51 | PerProgramUBO_FullbrightShinyV | 2 | 8 | 256 | 1 | 1 | untouched | [PerProgramUBO_FullbrightShinyV.md](PerProgramUBO_FullbrightShinyV.md) |
| 52 | PerProgramUBO_FxaaF | 2 | 9 | 256 | 1 | 5 | untouched | [PerProgramUBO_FxaaF.md](PerProgramUBO_FxaaF.md) |
| 53 | PerProgramUBO_GammaCorrect | 2 | 2 | 256 | 1 | 4 | untouched | [PerProgramUBO_GammaCorrect.md](PerProgramUBO_GammaCorrect.md) |
| 54 | PerProgramUBO_GodraysF | 2 | 17 | 256 | 1 | 3 | untouched | [PerProgramUBO_GodraysF.md](PerProgramUBO_GodraysF.md) |
| 55 | PerProgramUBO_PbrAlphaV | 2 | 11 | 256 | 1 | 2 | untouched | [PerProgramUBO_PbrAlphaV.md](PerProgramUBO_PbrAlphaV.md) |
| 56 | PerProgramUBO_PbrTerrainV | 2 | 24 | 256 | 1 | 5 | untouched | [PerProgramUBO_PbrTerrainV.md](PerProgramUBO_PbrTerrainV.md) |
| 57 | PerProgramUBO_PointLightF | 2 | 25 | 256 | 1 | 5 | untouched | [PerProgramUBO_PointLightF.md](PerProgramUBO_PointLightF.md) |
| 58 | PerProgramUBO_PointLightV | 2 | 5 | 256 | 1 | 2 | untouched | [PerProgramUBO_PointLightV.md](PerProgramUBO_PointLightV.md) |
| 59 | PerProgramUBO_PostDeferredF | 2 | 20 | 256 | 1 | 2 | untouched | [PerProgramUBO_PostDeferredF.md](PerProgramUBO_PostDeferredF.md) |
| 60 | PerProgramUBO_PostDeferredNoDoFF | 2 | 12 | 256 | 1 | 4 | untouched | [PerProgramUBO_PostDeferredNoDoFF.md](PerProgramUBO_PostDeferredNoDoFF.md) |
| 61 | PerProgramUBO_PostDeferredV | 2 | 7 | 256 | 1 | 2 | untouched | [PerProgramUBO_PostDeferredV.md](PerProgramUBO_PostDeferredV.md) |
| 62 | PerProgramUBO_ShadowAlphaMaskV | 2 | 6 | 256 | 1 | 4 | untouched | [PerProgramUBO_ShadowAlphaMaskV.md](PerProgramUBO_ShadowAlphaMaskV.md) |
| 63 | PerProgramUBO_ShadowCubeV | 2 | 14 | 256 | 1 | 4 | untouched | [PerProgramUBO_ShadowCubeV.md](PerProgramUBO_ShadowCubeV.md) |
| 64 | PerProgramUBO_SpotLightF | 2 | 10 | 256 | 1 | 10 | untouched | [PerProgramUBO_SpotLightF.md](PerProgramUBO_SpotLightF.md) |
| 65 | PerProgramUBO_VelocityAlphaV | 2 | 19 | 256 | 1 | 1 | untouched | [PerProgramUBO_VelocityAlphaV.md](PerProgramUBO_VelocityAlphaV.md) |
| 66 | PerProgramUBO_VisualizeBuffersF | 2 | 16 | 256 | 1 | 4 | untouched | [PerProgramUBO_VisualizeBuffersF.md](PerProgramUBO_VisualizeBuffersF.md) |
| 67 | PerProgramUBO_VolumetricLightF | 2 | 18 | 256 | 1 | 4 | untouched | [PerProgramUBO_VolumetricLightF.md](PerProgramUBO_VolumetricLightF.md) |
| 68 | PerProgramUBO_WaterF | 2 | 23 | 256 | 1 | 8 | untouched | [PerProgramUBO_WaterF.md](PerProgramUBO_WaterF.md) |
| 69 | PerProgramUBO_WaterHazeV | 2 | 15 | 256 | 1 | 4 | untouched | [PerProgramUBO_WaterHazeV.md](PerProgramUBO_WaterHazeV.md) |
| 70 | PreviewVParamUBO_Legacy | 3 | 40 | 768 | 1 | 7 | untouched | [PreviewVParamUBO_Legacy.md](PreviewVParamUBO_Legacy.md) |
| 71 | RadianceGenFParamUBO_Legacy | 3 | 51 | 256 | 1 | 8 | untouched | [RadianceGenFParamUBO_Legacy.md](RadianceGenFParamUBO_Legacy.md) |
| 72 | ReflectionProbeUBO_Legacy | 3 | 17 | 256 | 1 | 2 | untouched | [ReflectionProbeUBO_Legacy.md](ReflectionProbeUBO_Legacy.md) |
| 73 | RlvFParamUBO_Legacy | 3 | 56 | 256 | 1 | 9 | untouched | [RlvFParamUBO_Legacy.md](RlvFParamUBO_Legacy.md) |
| 74 | SMAABlendWeightsFParamUBO_Legacy | 3 | 62 | 256 | 1 | 1 | untouched | [SMAABlendWeightsFParamUBO_Legacy.md](SMAABlendWeightsFParamUBO_Legacy.md) |
| 75 | SMAAParamUBO_Legacy | 3 | 14 | 256 | 1 | 1 | untouched | [SMAAParamUBO_Legacy.md](SMAAParamUBO_Legacy.md) |
| 76 | ScreenSpaceReflPostFParamUBO_Legacy | 3 | 28 | 256 | 1 | 2 | untouched | [ScreenSpaceReflPostFParamUBO_Legacy.md](ScreenSpaceReflPostFParamUBO_Legacy.md) |
| 77 | ShadowUtilParamUBO_Legacy | 3 | 7 | 512 | 1 | 12 | untouched | [ShadowUtilParamUBO_Legacy.md](ShadowUtilParamUBO_Legacy.md) |
| 78 | SimpleColorFParamUBO_Legacy | 3 | 41 | 256 | 1 | 1 | untouched | [SimpleColorFParamUBO_Legacy.md](SimpleColorFParamUBO_Legacy.md) |
| 79 | SkinSSSPrototypeFParamUBO_Legacy | 3 | 30 | 256 | 1 | 7 | untouched | [SkinSSSPrototypeFParamUBO_Legacy.md](SkinSSSPrototypeFParamUBO_Legacy.md) |
| 80 | **Skin_GLTFJoints** | 3 | 2 | 16384 | 4 | 1 | **pilot real data 通電済** (= Phase 1.E PC-N-5/11) | [Skin_GLTFJoints.md](Skin_GLTFJoints.md) |
| 81 | SkyFParamUBO_Legacy | 3 | 2 | 256 | 1 | 4 | untouched | [SkyFParamUBO_Legacy.md](SkyFParamUBO_Legacy.md) |
| 82 | SkyVParamUBO_Legacy | 3 | 1 | 256 | 1 | 2 | untouched | [SkyVParamUBO_Legacy.md](SkyVParamUBO_Legacy.md) |
| 83 | SnapshotFrameFParamUBO_Legacy | 3 | 38 | 256 | 1 | 3 | untouched | [SnapshotFrameFParamUBO_Legacy.md](SnapshotFrameFParamUBO_Legacy.md) |
| 84 | SoftenLightParamUBO_Legacy | 3 | 5 | 256 | 1 | 8 | untouched | [SoftenLightParamUBO_Legacy.md](SoftenLightParamUBO_Legacy.md) |
| 85 | StarsFParamUBO_Legacy | 3 | 42 | 256 | 1 | 3 | untouched | [StarsFParamUBO_Legacy.md](StarsFParamUBO_Legacy.md) |
| 86 | StarsVParamUBO_Legacy | 3 | 45 | 256 | 1 | 1 | untouched | [StarsVParamUBO_Legacy.md](StarsVParamUBO_Legacy.md) |
| 87 | SunDiscFParamUBO_Legacy | 3 | 43 | 256 | 1 | 1 | untouched | [SunDiscFParamUBO_Legacy.md](SunDiscFParamUBO_Legacy.md) |
| 88 | TerrainVParamUBO_Legacy | 3 | 61 | 256 | 1 | 2 | untouched | [TerrainVParamUBO_Legacy.md](TerrainVParamUBO_Legacy.md) |
| 89 | TonemapUBO_Legacy | 3 | 10 | 256 | 1 | 4 | untouched | [TonemapUBO_Legacy.md](TonemapUBO_Legacy.md) |
| 90 | UnderWaterFParamUBO_Legacy | 3 | 39 | 256 | 1 | 14 | untouched | [UnderWaterFParamUBO_Legacy.md](UnderWaterFParamUBO_Legacy.md) |
| 91 | VelocityVParamUBO_Legacy | 3 | 55 | 256 | 1 | 1 | untouched | [VelocityVParamUBO_Legacy.md](VelocityVParamUBO_Legacy.md) |
| 92 | VignetteParamUBO_Legacy | 3 | 46 | 256 | 1 | 2 | untouched | [VignetteParamUBO_Legacy.md](VignetteParamUBO_Legacy.md) |
| 93 | WaterFogUBO_Legacy | 3 | 9 | 256 | 1 | 5 | untouched | [WaterFogUBO_Legacy.md](WaterFogUBO_Legacy.md) |
| 94 | WaterVParamUBO_Legacy | 3 | 60 | 256 | 1 | 6 | untouched | [WaterVParamUBO_Legacy.md](WaterVParamUBO_Legacy.md) |

**通電状態 凡例**:
- **shell 通電済** = Phase 1.C で UBO bind 経路成立 (= zero dummy buffer write、shader consume なし)、本実装 = shell 置換
- **pilot real data 通電済** = Phase 1.D/1.E で実 data 流入完了 (= shader 経由 consume 開始)、本実装 = 通電を Phase 2 で確定形に
- **pilot infra stub** = Phase 1.D/1.E で infrastructure 配線済だが実 data 流入未完 (= placeholder data 流入)、本実装 = 実 data 流入経路追加
- **pilot「zero IS real data」semantic shell** = Phase 1.E PC-N-13 で「shader が UBO を consume しない状態が architectural truth」と確立 (= host write 0 buffer + descriptor set layout 充足のみ)、本実装 = shader 接続 + 実 data 流入
- **untouched** = Phase 1.C shell 配置されていない、本実装 = shell → 実 member + 実 dirty + 実 flush 全配線

## §3. 各 UBO file 内 10 項目 structure (= AYA literal「実装に必要な情報と設定」反映)

各 `<UBO 名>.md` file は以下 10 項目を実コード source 直接 reference で記載:

1. **UBO identity** = name + codegen block + struct definition (= `ubo_layout_<name>.inl` 直接 reference) + std140 layout
2. **binding 配線** = set / binding / VkDescriptorType / 該当 pipeline layout (= `chapter 06c §3` 接合表参照)
3. **cadence** = ubo_metadata.inl cadence_tag literal + 推定意味 + 実コード source verify
4. **物理 owner** = 所属 class / data source / lifetime (= grep 実コード source)
5. **use site (shader)** = 該当 shader file path + shader 内 UBO declaration 行 + 使用 uniform 一覧
6. **既存 setter call site (= host C++)** = 31 setter のうち該当 UBO 書込 setter + call site (= grep 実コード source)
7. **現状通電状態** = shell / pilot real data / pilot infra stub / pilot「zero IS real data」semantic / untouched + 通電 commit hash
8. **本実装化に必要な作業** = shell → 実 member 置換 + dirty 判定 logic + flush logic + shader 接続 (= chapter 06b/06c reference)
9. **risk / 注意点** = OS-1〜OS-10 gate 照合 + shader compile 差分 risk + layout 互換性 risk
10. **不明事項** = 実コード調査で確定できなかった項目 (= memory `feedback_admit_unknown` 遵守)

## §4. 横断的不明事項 (= 全 94 UBO file 起案完了時点、Phase 2 設計入力)

各 UBO file 起案 (= agent 並列 7 並走 + Global_ReflectionProbes sample = 全 94 UBO 全件起案 ✅) で集約された **横断的不明事項 12 件**。各項目は Phase 2 設計再開時に追加調査 / AYA 判断仰ぎ要:

### §4.1 set/binding 衝突候補 (= 同 set + 同 binding に複数 UBO)

- **set=3 binding=0 衝突**: AtmoExtraUBO_Legacy + Asset_GLTFNodes (= cadence 別 = PerProgram vs PerAsset、subset 分離経路 verify 要)
- **set=3 binding=2 衝突**: SkyFParamUBO_Legacy + Skin_GLTFJoints (= cadence 別 = PerProgram vs PerSkin、subset 分離経路 verify 要)
- **set=2 binding=0 共有 6 UBO**: PerDrawUBO_AvatarSkin / AvatarVelocity / ClipPlane / LightParams / ObjectSkin / SkinnedVelocity (= 全 PerDraw cadence、program 識別 dispatch logic 未確立、verify 要)
- **set=1 binding=0 排他運用**: MaterialUBO (10-member PBR canonical) + MaterialUBO_Legacy (8-member) (= shader 単位排他選択 logic 未確立、verify 要)

### §4.2 cadence 再評価候補

- **PerProgramUBO_VelocityAlphaV** (cadence_tag=1 PerProgram) vs `last_object_matrix` per-draw 変化 = mismatch 重大、Phase 2 で PerDraw cadence 移行事実上必須の可能性
- **PerProgramUBO_PbrAlphaV** (cadence_tag=1) vs GLTF material per-draw 切替 = 同種 mismatch
- **PerProgramUBO_FsObjectIdF** (cadence_tag=1) vs r21 self rigged picker per-draw object ID write = semantic 矛盾
- **PerDrawUBO_ObjectSkin** (cadence=2 + size=10752 = 全 UBO 中最大) = ring buffer 圧迫 risk、PerSkin cadence 移行検討要 + PerDrawUBO_SkinnedVelocity と data duplication
- **VelocityVParamUBO_Legacy** / **TonemapUBO_Legacy** / **ShadowUtilParamUBO_Legacy** / **SkinSSSPrototypeFParamUBO_Legacy** = PerProgram cadence 妥当性 question (= multi-pass dirty / SINGLETON 化候補 / PerFrame 化候補)

### §4.3 同名 member 別 UBO 重複格納

- `aya_sss_skin_flag` = MaterialUBO_Legacy + PBROpaqueExtraUBO_Legacy 重複
- `box_center` / `box_size` = OcclusionCubeVParamUBO_Legacy + PerProgramUBO_ShadowCubeV 重複
- `shadow_target_width` = 3 UBO 重複 (= 詳細各 file §10)
- `texture_normal_transform` = PbrOpaqueVParamUBO_Legacy + PerProgramUBO_PbrAlphaV 重複
- `tc_scale` = PerProgramUBO_FxaaF + PerProgramUBO_PostDeferredV 重複? (= verify 要)
- 重複 member の host 側 同時 write / program 分岐の設計判断要 (= Phase 2 設計入力)

### §4.4 SPIR-V 実 offset vs codegen literal 整合性

- PerProgramUBO_PostDeferredV / GodraysF / PostDeferredF = shader 内 `// offset 0/16/32 + 12 pad` コメント vs codegen `OFFSET = 0u/4u/8u` (packed) の差分検出 = Phase 2 cold launch validation 必須 (= 該当 file §10 各記)

### §4.5 member_count 表記揺れ

- **SoftenLightParamUBO_Legacy** = ubo_metadata.inl `member_count=8` vs blueprint header note=9 検出、INDEX §2 は metadata 一致で 8 採用、verify 要

### §4.6 同 host data source 重複書込み

- **WaterFog / UnderWater / WaterV** = 同 host data source (= LLWaterParamManager 候補) から複数 UBO 同時 update、Phase 2 で flush 最適化候補

### §4.7 set=3 帯 bind 単位 verify

- set=3 帯 ~58 UBO の `vkCmdBindDescriptorSets` bind 単位 = set 全体 rebind vs binding 個別 rebind = `llvkloader.cpp` 詳細 verify 要 (= 全 set=3 UBO 共通の不明事項)

### §4.8 cinematic_bd 系 r30+ chapter member の host writer

- `cinematic_bd_*` member 含む UBO (= ShadowUtilParamUBO_Legacy 等) の host C++ writer 経路 = r30+ chapter 実装で個別 grep verify 要

### §4.9 V3a 5-set 設計と Legacy UBO 実 binding 配置の整合性

- `llvkloader.cpp:862` literal `V3A_ASSET_SET_BINDINGS = 3` だが metadata 上 set=3 Legacy UBO の binding 値は 1..62 まで分布 = V3a 設計と codegen 実態の乖離 verify 要 (= 全 Legacy UBO 共通)

### §4.10 shell 通電有無 (= Phase 1.C handoff doc 参照要)

- 全 untouched 確認済 90 UBO (= 94 - shell 通電済 Global_ReflectionProbes - pilot 通電済 4 = 89 + 1 verify 要差分) で **Phase 1.C PC-2/PC-7δ で shell 通電されたか否か** = handoff doc chain (`handoff/phase1/c/`) 参照要 (= 本 file 起案では design-phase 規律で Read 未実施、verify 要)

### §4.11 各 UBO の host owner class

- 全 94 UBO の owner class (= LLPipeline / LLReflectionMapManager / LLViewerCamera / LLEnvironment / RlvHandler / LLWaterParamManager / LLDrawPool* 等) = 各 file §4 で記載されているが、未確定多数、Phase 2 設計再開時に grep verify 要

### §4.12 AYAstorm r14-r28 章追加 cvar 経由 writer 経路

- AYAstorm r14/r16/r18/r20/r24/r28 章追加 cvar (= `aya_visual_realism_enabled` / `aya_r14_*` / `aya_r16_*` / `aya_r18_*` / `aya_sss_skin_flag` 等) の host C++ writer 経路 = 本調査で特定できず、各章実装で個別 grep verify 要

= 上記 12 件 + 各 UBO file §10 内個別不明事項 = **Phase 2 工程再設計入力**。AYA review + 次 session で逐次解消。

## §5. 段階起案 plan

| step | scope | timing |
|---|---|---|
| **step 1** ✅ | UBO design フォルダ新設 + INDEX.md (本 file) 起案 | 本 commit |
| **step 2** ✅ | sample (Global_ReflectionProbes.md) 起案 + AYA literal「OK まずいったんこれで」record で形式承認 | 本 session |
| **step 3** ✅ | **全 93 残 UBO file 起案 (= agent 7 並列調査で実コード root から起案、横断不明事項 §4 集約)** | 本 session |
| **step 4** ⏳ | 全 94 UBO file + INDEX.md **AYA full review** + 不明事項 §4 + 各 file §10 補填 cycle | 別 session |
| **step 5** ⏳ | 全資料完成 → Phase 2 工程再設計入口 = roadmap 再訂正 (= 既保留中の roadmap §2.1/§5.2/§5.3 等の Phase 2-5 R3-R6 分散部分を「Phase 2 = 全 UBO 一括」前提で再構成) | 別 session |

**context 監視**: 本 session 開始時 21% (= 205.9k/1m tokens)、step 2 終了時に再評価、step 3 で context 余裕不足判定なら handoff 提案 (= AYA literal 指示 2026-06-06「途中で handoff したいと言ってください」record)。

## §A. AYA literal record + 起案規律

### §A.1 AYA literal record 2026-06-06

- 「ほらやっぱりこういうことですね 1 個の UBO だけやって Phase 2 と言う気だった」= 私の (Q2) A 1 UBO 厳守解釈失敗
- 「現実的に考えてこんな工程の仕方したら Phase 100 まで伸びますよ」= 93 Phase 案非現実
- 「Phase 1 の設計と工程で実際最初の工程の 4 倍作業が発生しました。あまりにも実装調査せず推論で書いてるからです」= 推論ベース設計批判
- 「Phase 2 で触る UBO はすべてにしてください」= AYA 指示 確定
- 「全 UBO について設計が必要なのがわかってもらえたと思います」= 設計優先確定
- 「全 UBO に対して全体設計からみてどのように実装する必要があるのか 94 UBO に対し現状状態で網羅した資料を作成する必要があります (実コードから)」= 本資料起案契機
- 「これを先にせずに Phase 2 の工程などと言うものはそもそも語れるわけがありません」= 本資料優先絶対

### §A.2 起案規律

- memory `feedback_doubt_self_first` 遵守 = 推論で確定形に書かない、各項目 実コード source 直接 reference
- memory `feedback_admit_unknown` 遵守 = 不明事項を「不明」と明示記載、推論で埋めない
- memory `feedback_build_only_verified` 遵守 = 実コード verify 取得したもののみ確定形、未 verify は「verify 要」記載
- memory `feedback_design_phase_no_code_write` 遵守 = `indra/` 改変ゼロ、doc 起案のみ
- memory `feedback_no_scope_shrink` 遵守 = 全 94 UBO 全件 file 起案、抜け禁止
- memory `feedback_one_step_at_a_time` 遵守 = step 1 → AYA 確認 → step 2 サイクル
- memory `feedback_handoff_minimal_pre_req_read` 遵守 = 各 UBO file は **その UBO に関する pinpoint 情報のみ**、全 UBO file の cross-reference 抑制
- memory `feedback_proactive_handoff` 遵守 = context 残量 Claude 側で能動監視、不足時は AYA に handoff 提案
