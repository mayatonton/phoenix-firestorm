# r41 UBO Readiness — 全 94 UBO 実装判定資料 (= A/B/C 判定 + B/C 理由詳細)

**着手契機**: 2026-06-06 AYA literal「各 UBO の実装がすでにすぐ可能である場合には A 判定、情報がまだ不明確でわからないが B 判定、他の UBO が出来上がらないと完成判断がつかないものを C 判定として、B と C に関してはその理由の詳細も記述して表にした資料を作ってください」record

**位置付け**: Phase 2 = 全 94 UBO 一括本実装化の **着手順序 + 設計入力資料**。各 UBO の A/B/C 判定 + 不明事項詳細を表にまとめ、Phase 2 工程再設計の入力に供する。

**起案規律**:
- 判定根拠は各 UBO file §10 (不明事項) + §11 (他 UBO 関係) + §6 (既存 setter call site) + §7 (現状通電状態) を集約
- B/C 判定の理由詳細 = 推論禁止、実コード調査ベース (= memory `feedback_admit_unknown` 遵守)
- 1 UBO 1 read で全 94 file 順次精査済 (= sequential、agent 並列禁止)

---

## §1. 判定基準

| 判定 | 基準 |
|---|---|
| **A 判定** | (1) pilot real data 通電済 OR setter site 完全特定済、(2) data source 確定、(3) cadence 妥当性確定 (= 矛盾なし)、(4) 他 UBO 依存なし (= cross-UBO 同期 protocol 不要)、(5) 不明事項 minor (= cold launch validation のみ) |
| **B 判定** | 上記 A 条件のうち (1)(2)(3) のいずれかが情報不明確 (= setter call site 不明 / data source 不明 / cadence 妥当性 verify 要) |
| **C 判定** | 上記 A 条件 (4) で **他 UBO が出来上がらないと完成判断がつかない** (= 同 host data source 共有で cross-UBO 同期 protocol 要 / pair UBO で双方 cadence 連動設計要 / sequential pipeline で上流 UBO 完成後判断 / cadence 矛盾で他 UBO cadence 決定後評価) |

---

## §2. A 判定 UBO (= 実装すぐ可能)

| # | UBO 名 | set/binding | cadence | 通電状態 | 理由 |
|---|---|---|---|---|---|
| 1 | Skin_GLTFJoints | 3/2 | 4 PerSkin | **pilot real data 通電済** (Phase 1.E PC-N-5/11/15c) | register/wire/write/flush/unregister 全 5 setter site 特定済 (`llvkloader.cpp:5824/3180/5899/5250/5883`) + real bone matrix 接続待ち (Phase 1.F+ 持越) + AYA live verify「通常通りに描画されてます」record 2026-06-06、Vulkan validation 0 件確認済 |

= **A 判定 1 件のみ**。全 93 残 UBO は B/C 判定 (= setter 不明 or 他 UBO 依存)。

---

## §3. C 判定 UBO (= 他 UBO 依存、cross-UBO 同期 / pair / sequential 必須)

### §3.1 cross-UBO same data dirty 同期必須 (= 3 UBO triple-write)

| # | UBO 名 | set/binding | 依存先 UBO | 理由詳細 |
|---|---|---|---|---|
| 2 | MaterialUBO_Legacy | 1/0 | PBROpaqueExtraUBO_Legacy + AvatarFParamUBO_Legacy | `aya_sss_skin_flag` 同 r20 SSS skin flag を 3 UBO で重複格納、host 側で 3 UBO 同時 write 設計判断要 (= 3 UBO 同時 write か program 識別で 1 UBO のみ write か、 unblocking = SSS skin flag dispatch protocol 確定) |
| 3 | PBROpaqueExtraUBO_Legacy | 3/13 | MaterialUBO_Legacy + AvatarFParamUBO_Legacy | 同上 (= aya_sss_skin_flag 3 UBO 重複格納) |
| 4 | AvatarFParamUBO_Legacy | 3/54 | MaterialUBO_Legacy + PBROpaqueExtraUBO_Legacy + SkinSSSPrototypeFParamUBO_Legacy | aya_sss_skin_flag 3 UBO triple-write + r20 SSS pipeline 連動、unblocking = SSS skin flag dispatch protocol + r20 SSS phase shader consume site 全特定 |

### §3.2 cross-UBO same data dirty 同期必須 (= 2 UBO cross-write)

| # | UBO 名 | set/binding | 依存先 UBO | 理由詳細 |
|---|---|---|---|---|
| 5 | AtmoExtraUBO_Legacy | 3/0 | SkinSSSPrototypeFParamUBO_Legacy | `aya_visual_realism_enabled` 2 UBO 重複格納 (= AtmoExtra 側 vs SkinSSS 側 `aya_visual_realism_enabled_skinsss_legacy` rename 版、η-6 phase 2-A 範式)、cvar 変更時 2 UBO 同時 dirty 必須、unblocking = visual realism enable cvar dispatch protocol 確定 |
| 6 | SkinSSSPrototypeFParamUBO_Legacy | 3/30 | AtmoExtraUBO_Legacy | 同上 + r20 SSS multi-pass aya_blur_dir per-pass 値設定 logic 未確定、unblocking = AtmoExtraUBO_Legacy aya_visual_realism_enabled dispatch + multi-pass dirty pattern 確定 |
| 7 | PerProgramUBO_PostDeferredF | 2/20 | PerProgramUBO_PostDeferredNoDoFF | `chroma_str` 同 `RenderChromaStrength` cvar 由来 double-write 必須 (HAS_DOF_CHROMA permutation 切替で本 UBO ↔ NoDoFF 切替)、unblocking = chroma_str dispatch + HAS_DOF_CHROMA 切替 logic 確定 |
| 8 | PerProgramUBO_PostDeferredNoDoFF | 2/12 | PerProgramUBO_PostDeferredF | 同上 (= chroma_str cvar 由来 double-write) |
| 9 | PerProgramUBO_PointLightF | 2/25 | PerProgramUBO_SpotLightF + PerProgramUBO_PointLightV | `sun_wash` / `falloff` / `global_light_strength` 同 cvar 由来 Point/Spot 両 UBO double-write 必須、PerProgramUBO_PointLightV (binding=5) と V/F pair、unblocking = light cvar dispatch + light V/F pair 整合 |
| 10 | PerProgramUBO_SpotLightF | 2/10 | PerProgramUBO_PointLightF + PerProgramUBO_PointLightV (cross-stage declared-but-unused) | 10 member 統合、PointLightF と同 5 member 共有 + PerProgramUBO_PointLightV を spotLightF で declared-but-unused (= η-28-C type 3)、unblocking = light cvar dispatch + cross-stage 共有 SPIR-V validation |
| 11 | PerProgramUBO_PointLightV | 2/5 | PerProgramUBO_PointLightF + PerProgramUBO_SpotLightF | V/F pair + cross-stage 共有 (= spotLightF declared-but-unused)、unblocking = pointLightV/F + spotLightF 3 program 同時設計 |

### §3.3 cross-UBO same data dirty 同期必須 (= 3 UBO triple-write、shadow target width)

| # | UBO 名 | set/binding | 依存先 UBO | 理由詳細 |
|---|---|---|---|---|
| 12 | PerProgramUBO_ShadowAlphaMaskV | 2/6 | PbrShadowAlphaMaskVParamUBO_Legacy + AvatarAlphaShadowVParamUBO_Legacy | `shadow_target_width` 同 `LLShaderMgr::DEFERRED_SHADOW_TARGET_WIDTH` 由来 3 UBO triple-write 必須、`pipeline.cpp:8562/8570/8584/8592/12596/12611/12642` 7 setter site で 3 UBO 同時 update、unblocking = shadow_target_width dispatch + shadow render pass dirty 連動 |
| 13 | PbrShadowAlphaMaskVParamUBO_Legacy | 3/21 | PerProgramUBO_ShadowAlphaMaskV + AvatarAlphaShadowVParamUBO_Legacy | 同上 |
| 14 | AvatarAlphaShadowVParamUBO_Legacy | 3/22 | PerProgramUBO_ShadowAlphaMaskV + PbrShadowAlphaMaskVParamUBO_Legacy | 同上 + avatar alpha shadow program 専用、writer call site (= `DEFERRED_SHADOW_TARGET_WIDTH` setter) は pipeline.cpp 集約済確認 |

### §3.4 cross-UBO same data dirty 同期必須 (= 2 UBO、box_center/box_size)

| # | UBO 名 | set/binding | 依存先 UBO | 理由詳細 |
|---|---|---|---|---|
| 15 | OcclusionCubeVParamUBO_Legacy | 3/50 | PerProgramUBO_ShadowCubeV | `box_center` / `box_size` 同 `BOX_CENTER` / `BOX_SIZE` enum 由来、別 program 別 UBO で host 識別 dispatch 必須、setter call site 不明 (= grep 拡大要)、unblocking = box data source owner 特定 + program 識別 dispatch |
| 16 | PerProgramUBO_ShadowCubeV | 2/14 | OcclusionCubeVParamUBO_Legacy | 同上、host 側 program 識別で誤 bind 防止必須 |

### §3.5 GLTF texture transform 整理必須 (= 4 UBO/path、texture_normal_transform/texture_metallic_roughness_transform)

| # | UBO 名 | set/binding | 依存先 UBO | 理由詳細 |
|---|---|---|---|---|
| 17 | PbrOpaqueVParamUBO_Legacy | 3/53 | PerProgramUBO_PbrAlphaV + MaterialUBO + pbrmetallicroughnessV (bare local) | `texture_normal_transform` / `texture_metallic_roughness_transform` 同 LLFetchedGLTFMaterial 由来、`llfetchedgltfmaterial.cpp:136/140` setter で本 UBO に書込、別 program 別 UBO 整理要 + cadence 矛盾 (= material per-draw 切替 vs PerProgram cadence)、unblocking = GLTF transform dispatch + cadence 再評価 |
| 18 | PerProgramUBO_PbrAlphaV | 2/11 | PbrOpaqueVParamUBO_Legacy + MaterialUBO + pbrmetallicroughnessV | 同上 + pbralphaV 用 (= alpha pass) |
| 19 | MaterialUBO | 1/0 | MaterialUBO_Legacy (= 排他選択) + PbrOpaqueVParamUBO_Legacy + PerProgramUBO_PbrAlphaV + Asset_GLTFMaterials | set=1 binding=0 排他運用 logic 未確立 + 10-member PBR full canonical (= layout-compat 慣用で 6-member view) + texture transform 共有、unblocking = MaterialUBO/Legacy 排他 dispatch + layout-compat validation + GLTF transform 共有整理 |

### §3.6 water 系 4-5 UBO 連動 dirty 必須 (= LLEnvironment LLSettingsWater 由来)

| # | UBO 名 | set/binding | 依存先 UBO | 理由詳細 |
|---|---|---|---|---|
| 20 | WaterFogUBO_Legacy | 3/9 | UnderWaterFParamUBO_Legacy + WaterVParamUBO_Legacy + PerProgramUBO_WaterF | `waterFogColor` / `waterFogKS` を UnderWaterFParamUBO_Legacy `_underwater_legacy` rename 版で重複格納、water settings 切替で 2+ UBO 同時 dirty 必須、unblocking = water cvar dispatch + duplicate write 最適化 |
| 21 | WaterVParamUBO_Legacy | 3/60 | UnderWaterFParamUBO_Legacy + WaterFogUBO_Legacy + PerProgramUBO_WaterF + PerProgramUBO_WaterHazeV | V/F multi-site (waterV/waterF) + `lightDir` / `eyeVec` を UnderWater 側 rename で重複格納、unblocking = water settings dispatch + V/F multi-site 同期 + cadence 再評価 (= per-frame 変化 member の per-program stale risk) |
| 22 | UnderWaterFParamUBO_Legacy | 3/39 | WaterFogUBO_Legacy + WaterVParamUBO_Legacy + PerProgramUBO_WaterF + FrameLights | 14 member、4 rename member (= waterFogColor / waterFogKS / lightDir / eyeVec) 重複格納、FrameLights consume guard wrap (= `#ifndef FRAME_LIGHTS_DEFINED`)、unblocking = water 系全 4-5 UBO 同時設計 |
| 23 | PerProgramUBO_WaterF | 2/23 | WaterFogUBO_Legacy + WaterVParamUBO_Legacy + UnderWaterFParamUBO_Legacy + PerProgramUBO_WaterHazeV | water rendering 共通 8 member、`kd` declared-but-unused、unblocking = water 共通 data source 共有経路確定 |
| 24 | PerProgramUBO_WaterHazeV | 2/15 | WaterFogUBO_Legacy + WaterVParamUBO_Legacy + UnderWaterFParamUBO_Legacy | water haze V+F shared (host 1 bind)、`above_water` int = SimpleColorFParamUBO_Legacy.waterSign と data 共有候補、unblocking = water 共通 + VkShaderStageFlags V/F 両指定 verify |

### §3.7 sky/cloud/atmospheric 群連動 dirty 必須 (= LLSettingsSky 由来、9 UBO)

| # | UBO 名 | set/binding | 依存先 UBO | 理由詳細 |
|---|---|---|---|---|
| 25 | FrameAtmosphere_Lighting | 0/2 | FrameLights + AtmoExtraUBO_Legacy + Sky 系 + Clouds 系 | sky preset 切替 + day cycle 連動で多 UBO 同時 dirty、20 member 大気色 + `minimum_alpha`/`max_cof` post pass 同居、blue_horizon setter 確認済 (= `llsettingsvo.cpp:1057`)、他 19 member setter 不明、unblocking = sky preset dispatch + Frame UBO 同期 |
| 26 | AtmoExtraUBO_Legacy | 3/0 | SkinSSSPrototypeFParamUBO_Legacy + SkyVParamUBO_Legacy + SkyFParamUBO_Legacy + CloudsVParamUBO_Legacy + CloudsFParamUBO_Legacy + FrameAtmosphere_Lighting | aya_visual_realism_enabled cross-UBO + sky preset 由来 10 member (= lightnorm/haze_horizon/cloud_shadow/sun_moon_glow_factor + AYA r14/r16 cvar 5 件)、setter 部分特定 (= 4 site llsettingsvo.cpp) + AYA r14/r16 cvar writer 不明、binding=0 衝突候補 (Asset_GLTFNodes と同 binding)、unblocking = sky preset + AYA r14/r16 cvar dispatch + binding 衝突解決 |
| 27 | SkyVParamUBO_Legacy | 3/1 | SkyFParamUBO_Legacy + CloudsVParamUBO_Legacy + AtmoExtraUBO_Legacy + FrameAtmosphere_Lighting | `camPosLocal` per-frame 変化の per-program cadence stale risk + binding=1 衝突候補 (Asset_GLTFMaterials と同 binding)、unblocking = camPosLocal dispatch + cadence 再評価 |
| 28 | SkyFParamUBO_Legacy | 3/2 | SkyVParamUBO_Legacy + CloudsFParamUBO_Legacy + AtmoExtraUBO_Legacy + FrameAtmosphere_Lighting | 4 member (hdri_split_screen / moisture_level / droplet_radius / ice_level) data source 不明、binding=2 衝突候補 (Skin_GLTFJoints と同 binding)、unblocking = sky/cloud microphysics dispatch + binding 衝突 |
| 29 | CloudsVParamUBO_Legacy | 3/3 | CloudsFParamUBO_Legacy + AtmoExtraUBO_Legacy + SkyVParamUBO_Legacy | V/F cross-stage 共有 (cloudsF.glsl:79 複製併存、η-14 path G-β) + `camPosLocal` 重複、unblocking = sky preset + V/F 共有 SPIR-V validation |
| 30 | CloudsFParamUBO_Legacy | 3/4 | CloudsVParamUBO_Legacy + AtmoExtraUBO_Legacy + SkyFParamUBO_Legacy | WL cloud uniform writer 不明 + AYA r18 cvar writer 不明 + V 側複製受け、unblocking = WL cloud dispatch + AYA r18 cvar |
| 31 | StarsFParamUBO_Legacy | 3/42 | StarsVParamUBO_Legacy + SunDiscFParamUBO_Legacy + MoonFParamUBO_Legacy + Sky 系 | day cycle 連動、`blend_factor` / `time` 同名 member 別 UBO 共有、time per-frame stale risk、unblocking = day cycle dispatch + stars V/F pair |
| 32 | StarsVParamUBO_Legacy | 3/45 | StarsFParamUBO_Legacy + Sky 系 | F 側 `time` の rename 版 `stars_v_time` (= shader 衝突回避)、setter 不明、unblocking = stars V/F pair dispatch |
| 33 | SunDiscFParamUBO_Legacy | 3/43 | StarsFParamUBO_Legacy + MoonFParamUBO_Legacy + Sky 系 | `blend_factor` per-frame day cycle 連動、setter 不明、unblocking = day cycle dispatch |
| 34 | MoonFParamUBO_Legacy | 3/44 | StarsFParamUBO_Legacy + SunDiscFParamUBO_Legacy + Sky 系 | setter 特定済 (= `lldrawpoolwlsky.cpp:453-455` + `llsettingsvo.cpp:853`)、day cycle 連動、unblocking = sky preset 連動 dispatch |

### §3.8 velocity / motion blur pair UBO (= curr/prev cadence pair、5 UBO)

| # | UBO 名 | set/binding | 依存先 UBO | 理由詳細 |
|---|---|---|---|---|
| 35 | PerDrawUBO_AvatarSkin | 2/0 | PerDrawUBO_AvatarVelocity | curr frame palette、setter 特定済 (= `lldrawpool.cpp:701/737/775` 3 site)、PerDraw cadence + ring buffer 経路、AvatarVelocity と curr/prev 対称設計、dedup logic (= avatar+mHash) ring buffer 整合要、unblocking = AvatarVelocity 同時設計 + frame swap protocol + set=2 binding=0 共有 6 UBO dispatch |
| 36 | PerDrawUBO_AvatarVelocity | 2/0 | PerDrawUBO_AvatarSkin | prev frame palette、setter 特定済 (= `lldrawpool.cpp:1021`)、first-frame fallback (= `mLastGLMp.empty() ? mGLMp : mLastGLMp`、`lldrawpool.cpp:1015`) 維持必須、unblocking = AvatarSkin 同時設計 + lightning-streak velocity 回避 logic 移植 |
| 37 | PerDrawUBO_ObjectSkin | 2/0 | PerDrawUBO_SkinnedVelocity + Skin_GLTFJoints (= GLTF skin との data source 別系統) | 10752 B = 全 UBO 中最大、`matrixPalette[110]` + `lastMatrixPalette[110]` 2 array 統合 (mat3x4 stride=48)、ring buffer 圧迫 risk、setter 不明、unblocking = bone matrix palette dispatch + ring buffer 容量 verify + cadence 再分類検討 |
| 38 | PerDrawUBO_SkinnedVelocity | 2/0 | PerDrawUBO_ObjectSkin | ObjectSkin の `lastMatrixPalette` のみ抽出版 (5376 B)、data duplication risk (= ObjectSkin と同 data)、velocity-enabled draw 判定 logic 不明、unblocking = ObjectSkin 完成 + duplication 解消 strategy 確定 |
| 39 | VelocityVParamUBO_Legacy | 3/55 | PerProgramUBO_VelocityAlphaV + PerDrawUBO_AvatarVelocity + PerDrawUBO_SkinnedVelocity + MotionBlurFParamUBO_Legacy | `last_object_matrix` mat4 + PerProgram cadence で per-object per-frame 変化 = **cadence mismatch 重大**、4 setter site (lldrawpool/tree/terrain)、unblocking = cadence 再評価 (PerDraw 移行候補) + 関連 velocity UBO 全 5 件設計 |
| 40 | PerProgramUBO_VelocityAlphaV | 2/19 | VelocityVParamUBO_Legacy + PerDrawUBO_SkinnedVelocity + MotionBlurFParamUBO_Legacy | 同 `last_object_matrix` mat4 + PerProgram cadence で同 cadence mismatch、velocityAlphaV.glsl 単独 attach (skinnedVelocityAlphaV は skin matrix 経由 = 別 UBO)、unblocking = cadence 再評価 + VelocityV と整合 |

### §3.9 reflection probe / IBL pipeline 連動 (= LLReflectionMapManager 由来、5 UBO)

| # | UBO 名 | set/binding | 依存先 UBO | 理由詳細 |
|---|---|---|---|---|
| 41 | Global_ReflectionProbes | 0/3 | ReflectionProbeUBO_Legacy + RadianceGenFParamUBO_Legacy + IrradianceGenFParamUBO_Legacy + GaussianFParamUBO_Legacy | SINGLETON cadence 唯一、shell 通電済 (Phase 1.C PC-2)、real data source 不明 (= LLReflectionMapManager 内 reflection probe state 構造)、unblocking = real reflection probe data → SINGLETON 経路本実装化 + IBL pipeline 全 5 UBO 整合 |
| 42 | ReflectionProbeUBO_Legacy | 3/17 | Global_ReflectionProbes + RadianceGenFParamUBO_Legacy + IrradianceGenFParamUBO_Legacy | Global と同 LLReflectionMapManager 由来候補 + cadence/set 異なる (PerProgram vs SINGLETON)、統合可能性 + 別 UBO 維持理由 verify 要、setter 不明、unblocking = Global と data source 関係確定 + setter site 特定 |
| 43 | RadianceGenFParamUBO_Legacy | 3/51 | IrradianceGenFParamUBO_Legacy + Global_ReflectionProbes + GaussianFParamUBO_Legacy | mip chain loop で複数回 dirty pattern + PerProgram cadence では stale risk、setter site (= LLReflectionMapManager 内、`llreflectionmapmanager.cpp:930` sSourceIdx 共有)、unblocking = IBL pipeline cadence 再評価 + mip loop dirty 解消 |
| 44 | IrradianceGenFParamUBO_Legacy | 3/52 | RadianceGenFParamUBO_Legacy + Global_ReflectionProbes + GaussianFParamUBO_Legacy | sibling pipeline、setter 部分特定 (= `llreflectionmapmanager.cpp:977` sSourceIdx)、`max_probe_lod` setter 不明、unblocking = IBL pipeline 連動 |
| 45 | GaussianFParamUBO_Legacy | 3/58 | RadianceGenFParamUBO_Legacy + IrradianceGenFParamUBO_Legacy + Global_ReflectionProbes | reflection mip blur (= `gGaussianProgram`)、horizontal/vertical 2 pass、setter 不明、unblocking = reflection mip generation dispatch + 2 pass cadence 解消 |

### §3.10 post-process pipeline 連動 (= exposure / tonemap / gamma / color grading chain)

| # | UBO 名 | set/binding | 依存先 UBO | 理由詳細 |
|---|---|---|---|---|
| 46 | ExposureFParamUBO_Legacy | 3/25 | LuminanceFParamUBO_Legacy + TonemapUBO_Legacy + FrameAtmosphere_Lighting | shell 通電済 + write 経路通電済、setter 特定済 (= `pipeline.cpp:8815/8863-8865` 4 setter)、auto-exposure pipeline 連動、unblocking = post-process chain 全 UBO 整合 |
| 47 | LuminanceFParamUBO_Legacy | 3/26 | ExposureFParamUBO_Legacy + TonemapUBO_Legacy | shell 通電済、setter 特定済 (= `pipeline.cpp:8754`、LLStaticHashedString 経由)、LLStaticHashedString UBO redirect 経路 verify 要、unblocking = LLStaticHashedString redirect + Exposure 連動 |
| 48 | TonemapUBO_Legacy | 3/10 | ExposureFParamUBO_Legacy + LuminanceFParamUBO_Legacy + PerProgramUBO_ColorGrading + PerProgramUBO_GammaCorrect | exposure / tonemap / gamma chain 後段、setter 不明、AYAstorm Cinematic mode tonemap cvar 関連、unblocking = post-process chain dispatch + AYAstorm r30 Cinematic 13 cvar との関係 |
| 49 | PerProgramUBO_GammaCorrect | 2/2 | PerProgramUBO_ColorGrading + TonemapUBO_Legacy | 2 shader file 共有 consume (= postDeferredGammaCorrect + postDeferredTonemap)、setter 不明、unblocking = 2 program 共有 dispatch + gamma cvar |
| 50 | PerProgramUBO_ColorGrading | 2/4 | PerProgramUBO_GammaCorrect + TonemapUBO_Legacy + VignetteParamUBO_Legacy | AYAstorm Cinematic Control 13 cvar 直結 (memory `project_r30_cinematic_control_tuning_deferred`)、setter 不明、unblocking = Cinematic Control cvar dispatch + Tonemap 連動 |
| 51 | VignetteParamUBO_Legacy | 3/46 | PerProgramUBO_ColorGrading + TonemapUBO_Legacy | exoVignette (= Exodus / BD derivative) 由来、`vignette` vec3 3 component 内訳不明、setter 不明、unblocking = vignette dispatch + Cinematic mode 連動 |

### §3.11 glow chain sequential pipeline (= 4 UBO)

| # | UBO 名 | set/binding | 依存先 UBO | 理由詳細 |
|---|---|---|---|---|
| 52 | GlowExtractFParamUBO_Legacy | 3/20 | GlowVParamUBO_Legacy + GlowFParamUBO_Legacy + GlowCombineFParamUBO_Legacy | shell 通電済、setter 特定済 (= `pipeline.cpp:9061-9068` 5 setter)、glow chain 上流 (extract → blur → combine)、unblocking = glow chain 全 4 UBO 整合 |
| 53 | GlowVParamUBO_Legacy | 3/19 | GlowFParamUBO_Legacy + GlowExtractFParamUBO_Legacy + GlowCombineFParamUBO_Legacy | shell 通電済、setter 特定済 (= `pipeline.cpp:9134/9138` horizontal/vertical 2 setter)、blur V pair、`delta` derive 元不明、unblocking = `delta` source + 2 pass dispatch + GlowF pair 整合 |
| 54 | GlowFParamUBO_Legacy | 3/18 | GlowVParamUBO_Legacy + GlowExtractFParamUBO_Legacy + GlowCombineFParamUBO_Legacy | shell 通電済、setter 特定済 (= `pipeline.cpp:9116`)、`strength` derive 元不明、blur F pair、unblocking = `strength` source + GlowV pair |
| 55 | GlowCombineFParamUBO_Legacy | 3/49 | GlowExtractFParamUBO_Legacy + GlowVParamUBO_Legacy + GlowFParamUBO_Legacy | shell 通電済、setter 特定済 (= `pipeline.cpp:9589-9597` + `:10706-10715` 8 site)、color grading 統合 (greyscale_str / sepia_str / num_colors)、unblocking = glow chain 上流完成 |

### §3.12 SMAA pass chain pipeline (= 2 UBO)

| # | UBO 名 | set/binding | 依存先 UBO | 理由詳細 |
|---|---|---|---|---|
| 56 | SMAAParamUBO_Legacy | 3/14 | SMAABlendWeightsFParamUBO_Legacy | shared include (= SMAA.glsl 全 pass consume)、`SMAA_RT_METRICS` viewport resize 連動、cadence=PerFrame/SINGLETON 候補 (= 全 SMAA program 共有値)、setter 不明、unblocking = SMAA pipeline 全 pass dispatch + cadence 再評価 |
| 57 | SMAABlendWeightsFParamUBO_Legacy | 3/62 | SMAAParamUBO_Legacy | blend weights pass 専用、`subsampleIndices` temporal SMAA で frame 毎更新可能性、setter 不明、unblocking = SMAA pipeline + temporal SMAA enable cvar |

### §3.13 pathfinding debug pair (= 2 UBO)

| # | UBO 名 | set/binding | 依存先 UBO | 理由詳細 |
|---|---|---|---|---|
| 58 | PathfindingVParamUBO_Legacy | 3/47 | PathfindingNoNormalVParamUBO_Legacy | 兄弟 UBO (= `tint` / `alpha_scale` 共有 + `ambiance` 追加版)、debug 用途、setter 不明 (= `LLPathfindingPathTool` / `LLFloaterPathfindingConsole` 候補)、unblocking = pathfinding debug dispatcher + NoNormal pair 整合 |
| 59 | PathfindingNoNormalVParamUBO_Legacy | 3/48 | PathfindingVParamUBO_Legacy | 同上 (= ambiance 削除版)、program 識別で正しい UBO 選択必須、unblocking = pathfinding debug dispatcher 同期 |

### §3.14 GLTF asset pair (= 2 UBO、pilot infra stub 通電済)

| # | UBO 名 | set/binding | 依存先 UBO | 理由詳細 |
|---|---|---|---|---|
| 60 | Asset_GLTFMaterials | 3/1 | Asset_GLTFNodes + Skin_GLTFJoints (= set=3 同居) + MaterialUBO + PbrOpaqueVParamUBO_Legacy + PerProgramUBO_PbrAlphaV | pilot infra stub 通電済 (Phase 1.C PC-7γ-3)、PBR shader Vulkan path 完成待ち、341 material 上限 (Vulkan min 16384B)、unblocking = real PBR shader consume + Asset_GLTFNodes 同時 + GLTF transform 整理 |
| 61 | Asset_GLTFNodes | 3/0 | Asset_GLTFMaterials + Skin_GLTFJoints + MaterialUBO 系 | pilot infra stub 通電済、341 nodes 上限 (Vulkan 16384B/48B mat3x4)、node animation update cadence 不明、binding=0 衝突候補、unblocking = node packing layout + node count 超過対応 + binding 衝突解決 |

### §3.15 PerDrawUBO set=2 binding=0 共有 6 UBO の残 (= ClipPlane / LightParams)

| # | UBO 名 | set/binding | 依存先 UBO | 理由詳細 |
|---|---|---|---|---|
| 62 | PerDrawUBO_ClipPlane | 2/0 | PerDrawUBO_AvatarSkin/AvatarVelocity/LightParams/ObjectSkin/SkinnedVelocity (= 同 binding 6 UBO 共有) | name-based dispatch logic 未確立 (= host wiring で program 識別 + 該当 UBO 名 dispatch)、setter site 不明 (= `LLShaderMgr::CLIP_PLANE` 経由候補)、5 shader file 内 identical UBO 宣言 verify 要、unblocking = name-based dispatch + clip plane data source 特定 |
| 63 | PerDrawUBO_LightParams | 2/0 | PerDrawUBO_ClipPlane/AvatarSkin/AvatarVelocity/ObjectSkin/SkinnedVelocity + PerDrawUBO_MultiLight + PerProgramUBO_SpotLightF/PointLightF/PointLightV + FrameLights | **pilot zero IS real data 通電済** (Phase 1.E PC-N-13)、host write 256 B zero buffer architectural truth (= sky_smoke shader 非 consume)、Phase 1.F+ で real PBR shader 接続時に `spot_light_color` / `spot_light_size` 置換、unblocking = Phase 1.F+ 実 PBR shader 接続 + name-based dispatch |

### §3.16 PerDrawUBO_MultiLight 独立 (= set=2 binding=1)

| # | UBO 名 | set/binding | 依存先 UBO | 理由詳細 |
|---|---|---|---|---|
| 64 | PerDrawUBO_MultiLight | 2/1 | PerDrawUBO_LightParams + PerProgramUBO_PointLightF/SpotLightF/PointLightV + FrameLights | single shader file (= multiPointLightF.glsl) consume、LIGHT_COUNT 1..16 permutation、setter 不明 (= `gDeferredMultiLightProgram[i]` 経路)、unblocking = light list dispatch + LIGHT_COUNT permutation 整合 |

---

## §4. B 判定 UBO (= 情報不明確、setter / data source / cadence 妥当性)

### §4.1 setter call site 不明 + data source 不明 (= 主要不明)

| # | UBO 名 | set/binding | 主要不明事項 |
|---|---|---|---|
| 65 | AvatarClothVParamUBO_Legacy | 3/57 | `AVATAR_WIND` / `AVATAR_SINWAVE` / `AVATAR_GRAVITY` setter site 不明 (推定 lldrawpoolavatar.cpp)、gSinWaveParams 4 component 意味不明、cloth simulation tick cadence 不明 |
| 66 | DofCombineFParamUBO_Legacy | 3/24 | `DOF_RES_SCALE` / `DOF_WIDTH` setter 不明 (推定 pipeline.cpp DoF combine pass)、`dof_height` reserved list 漏れ確認要、CASParamUBO_Legacy / PerProgramUBO_CofF と data source 共有可能性 |
| 67 | NormaldebugVParamUBO_Legacy | 3/37 | debug 用途、`debug_normal_draw_length` setter 不明、debug menu trigger 不明、検証優先度低 |
| 68 | NormgenFParamUBO_Legacy | 3/33 | bump-to-normal 生成 dispatcher 特定要 (= `LLBumpImageList::onSourceLoaded` 候補)、`stepX/stepY` setter 不明、`bump_code` enum 値域 |
| 69 | PreviewVParamUBO_Legacy | 3/40 | preview render pipeline (= LLImageGL / LLViewerObject preview) setter 不明、8-light fixed array 上限 verify 要、768B 大物 UBO ring buffer 配置 verify 要 |
| 70 | PerProgramUBO_AlphaParams | 2/3 | `near_clip` (`LLShaderMgr::NEAR_CLIP` 候補) setter 不明、`LLViewerCamera::getNear()` data source verify 要 |
| 71 | PerProgramUBO_BlurLightF | 2/22 | SSAO blur kernel + delta + dist_factor + blur_size + kern_scale setter 不明 (推定 `gPipeline.mSSAOParams`)、AOUtilParamUBO_Legacy / SoftenLightParamUBO_Legacy と data source 共有可能性 |
| 72 | PerProgramUBO_CofF | 2/21 | DoF CoF parameter setter 不明 (推定 `LLPipeline::generateExposure` / `renderDoF`)、focus / fov 変化 dirty trigger、透過 DoF 構造制約注記 (memory `project_transparent_dof_design_constraint`) |
| 73 | PerProgramUBO_FsObjectIdF | 2/13 | **cadence 矛盾 重大**: cadence_tag=1 PerProgram vs r21 self rigged picker per-draw object ID write semantics、`object_id_packed` setter site 不明 (= `LLDrawInfo::mFSPickerLocalID` 経路)、vec4 pack 内訳不明 (16 B 内 object ID 32-bit + 12 B 何か) |
| 74 | PerProgramUBO_FullbrightShinyV | 2/8 | `texture_matrix1` mat4 setter 不明 (`LLShaderMgr::TEXTURE_MATRIX1` 経由候補)、shiny cubemap 6 face 個別 transform か全 face 共通 verify 要 |
| 75 | PerProgramUBO_FxaaF | 2/9 | NVIDIA FXAA constant (rcp_screen_res / rcp_frame_opt / rcp_frame_opt2) setter 不明 (推定 `LLPipeline::renderFXAA`)、viewport resize trigger |
| 76 | PerProgramUBO_GodraysF | 2/17 | godray cvar (= aya_r15_godrays_enabled / phase_exponent / strength) setter 不明 (推定 `pipeline.cpp doRenderGodrays`)、shader `// offset 0/16/32 + 12 pad` vs codegen packed 整合 verify 要 |
| 77 | PerProgramUBO_PbrTerrainV | 2/24 | setter 特定済 (= `lldrawpoolterrain.cpp:558` `terrain_texture_transforms` + `:586` `region_scale`)、terrain region 切替 dirty trigger 不明、`region_scale` heightmap declared-but-unused 確認 |
| 78 | PerProgramUBO_PostDeferredV | 2/7 | `tc_scale` 直接 setter 不明 (= `FXAA_TC_SCALE` 共有か別 enum か)、postDeferredV/F pair 関係、`_pad_pdv0` vec2 将来 member 追加意図 |
| 79 | PerProgramUBO_VisualizeBuffersF | 2/16 | debug-only feature (= buffer visualize)、setter 特定済 (= `pipeline.cpp:8709/8711` LLStaticHashedString)、LLStaticHashedString UBO redirect 経路 verify 要、検証優先度低 |
| 80 | PerProgramUBO_VolumetricLightF | 2/18 | godray pipeline (= `pipeline.cpp doRenderGodrays`) setter 不明、`seconds60` BD legacy dead uniform、godray cvar (RenderGodraysRes / Multiplier / FalloffMultiplier) verify 要 |
| 81 | RlvFParamUBO_Legacy | 3/56 | RLVa Sphere effect、`RlvHandler` / `RlvActions` 内 sphere effect uniform 書込 site 不明、ESphereMode enum 定義 verify、`rlvEffectParam3_uvec` bvec2 → uvec2 promote (η-7 phase 1 範式) cast 整合、memory `project_ayastorm_rlv_user_base` 機能維持必須 |
| 82 | ScreenSpaceReflPostFParamUBO_Legacy | 3/28 | SSR pass の zNear/zFar setter 不明、`LLViewerCamera::getNear()/getFar()` 由来明示、FrameViewProj から derive 可能性 (= 重複 owner risk)、memory `project_transparent_ssao_ssr_no_work` 注記 (= no scheduled work) |
| 83 | SnapshotFrameFParamUBO_Legacy | 3/38 | snapshot UI (= LLSnapshotFloater / llsnapshotlivepreview.cpp) setter 不明、frame_rect / border_color / border_thickness data source、snapshot UI 起動 timing |
| 84 | ShadowUtilParamUBO_Legacy | 3/7 | 512 B 最大 size Legacy UBO、shadow_matrix[6] mat4 array、setter 9 個不明 (= LLPipeline shadow 経路)、shadow_matrix order (sun cascade + spot 順序) 不明、cinematic_bd multi-site identical 同期、memory `project_bd_biaserror_pitfall` 注記 (BD shadow_bias) |
| 85 | SimpleColorFParamUBO_Legacy | 3/41 | `waterSign` setter 不明、camera Z vs water plane Z 判定実装、PerProgramUBO_WaterHazeV.above_water との data 共有候補、1 member の存在意義 question |
| 86 | TerrainVParamUBO_Legacy | 3/61 | `object_plane_s/t` setter 不明 (= LLDrawPoolTerrain texgen 経路)、OpenGL 古典 glTexGen 移植経路、PerProgramUBO_PbrTerrainV との data source 共有 |

### §4.2 cadence mismatch B 判定 (= cadence 矛盾あるが他 UBO 依存なし single member)

| # | UBO 名 | set/binding | 主要不明事項 |
|---|---|---|---|
| 87 | ClipFParamUBO_Legacy | 3/32 | setter 特定済 (= `llmaniptranslate.cpp:1715-1716` LLStaticHashedString)、LLStaticHashedString UBO redirect 経路 verify 要、independent dirty、unblocking = LLStaticHashedString redirect 確定 |
| 88 | MotionBlurFParamUBO_Legacy | 3/27 | `RenderMotionBlurStrength` cvar 由来 (= `pipeline.cpp:10240`)、独立 dirty、setter 行番号特定要 (推定 :10240 周辺) |
| 89 | AOUtilParamUBO_Legacy | 3/8 | setter 特定済 (= `pipeline.cpp:10636-10643` 4 setter)、consumer program 列挙不明 (= SSAO consume program 群)、screen_to_target_scale_factor Vulkan path 整合 |
| 90 | CASParamUBO_Legacy | 3/12 | setter 特定済 (= `pipeline.cpp:9182-9200` LLStaticHashedString uniform4uiv/uniform2f)、LLStaticHashedString UBO redirect + uvec4 packing 整合 verify 要、CAS pass bind 単位 (multi-bind risk) |
| 91 | GlobalFParamUBO_Legacy | 3/11 | shell 通電済 + write 経路通電済、`MIRROR_FLAG` / `CLIP_SIGN` setter 不明 (推定 LLHeroProbeManager 経路)、mirror pass setup 経路、clipSign vs ClipFParamUBO_Legacy 機能重複整理 |
| 92 | DeferredUtilParamUBO_Legacy | 3/6 | setter 部分特定 (= `pipeline.cpp:12180-12184/12255-12257` projector params + `lldrawpoolwaterexclusion.cpp:73-74`/`lldrawpoolalpha.cpp:101/118/123` waterSign)、`PROJECTOR_NEAR` 所属 UBO 不明 (= proj_near member 不在)、`PROJECTOR_AMBIENT_LOD` write 先 member、waterSign per-program vs per-draw cadence 不一致 risk、PerProgramUBO_SpotLightF 重複宣言禁止整合 |

### §4.3 SoftenLightParamUBO_Legacy 単独 B

| # | UBO 名 | set/binding | 主要不明事項 |
|---|---|---|---|
| 93 | SoftenLightParamUBO_Legacy | 3/5 | 8 member = AYAstorm 独自 translucency 系 (aya_translucency_params/tint) + SSAO 系 (ssao_irradiance_scale/max/effect_mat) + blur 系 (blur_size/fidelity)、AYAstorm 独自 host setter 経路不明、AOUtilParamUBO_Legacy / GaussianFParamUBO_Legacy / DeferredUtilParamUBO_Legacy と data source 共有可能性、memory `project_aya_visual_realism_alpha_protect` 整合 (= frag_color.a=0 必須)、member_count 表記揺れ (metadata=8 vs blueprint=9) |

### §4.4 FrameLights / FrameViewProj (= 通電済だが setter 拡大未完)

| # | UBO 名 | set/binding | 通電 + 主要不明事項 |
|---|---|---|---|
| 94 | FrameViewProj | 0/0 | **shell + write 経路通電済** (Phase 1.A PA-8 + 1.C PC-7γ-1)、setter 主特定済 (= `LLRender::syncMatrices` 5 setter)、`screen_res` / `env_mat` / `last_modelview_matrix` setter site 不明、per-shader UBO block 拡大対象 50+ file (= upstream merge conflict risk)、`FRAMES_IN_FLIGHT` triple-buffer 経路 verify 要 |
| - | FrameLights | 0/1 | **shell + write 経路通電済**、setter 不明 (= `pipeline.cpp::setupHWLights` array 4 element setter)、`sun_dir` / `moon_dir` / `waterPlane` / `sun_up_factor` 個別 setter site 不明、8 array stride=16 host data layout 整合、per-shader UBO block 拡大対象 (現 8 件確認、残 lighting shader) |

= FrameViewProj / FrameLights は **shell 通電済**だが setter / per-shader 拡大に多数不明事項あり = B 判定。Frame 3 UBO 全件 (FrameAtmosphere_Lighting は §3.7 C 判定) は独立 dirty トリガなし。

---

## §5. 集計

| 判定 | 件数 | 内訳 |
|---|---|---|
| **A 判定** | 1 | Skin_GLTFJoints (= pilot real data 通電済) |
| **B 判定** | 31 | setter / data source / cadence 妥当性 不明、独立 dirty UBO |
| **C 判定** | 62 | 他 UBO 依存 (= cross-UBO 同期 / pair / sequential pipeline / cadence 矛盾解消後判断) |
| **合計** | **94** ✅ | INDEX §2 一致 |

### §5.1 B 判定 31 件 リスト (= 独立 dirty + setter/data source 不明)

AvatarClothVParamUBO_Legacy / DofCombineFParamUBO_Legacy / NormaldebugVParamUBO_Legacy / NormgenFParamUBO_Legacy / PreviewVParamUBO_Legacy / PerProgramUBO_AlphaParams / PerProgramUBO_BlurLightF / PerProgramUBO_CofF / PerProgramUBO_FsObjectIdF / PerProgramUBO_FullbrightShinyV / PerProgramUBO_FxaaF / PerProgramUBO_GodraysF / PerProgramUBO_PbrTerrainV / PerProgramUBO_PostDeferredV / PerProgramUBO_VisualizeBuffersF / PerProgramUBO_VolumetricLightF / RlvFParamUBO_Legacy / ScreenSpaceReflPostFParamUBO_Legacy / SnapshotFrameFParamUBO_Legacy / ShadowUtilParamUBO_Legacy / SimpleColorFParamUBO_Legacy / TerrainVParamUBO_Legacy / ClipFParamUBO_Legacy / MotionBlurFParamUBO_Legacy / AOUtilParamUBO_Legacy / CASParamUBO_Legacy / GlobalFParamUBO_Legacy / DeferredUtilParamUBO_Legacy / SoftenLightParamUBO_Legacy / FrameViewProj / FrameLights

### §5.2 C 判定 62 件 主要 group

| group | 件数 | 主要 UBO |
|---|---|---|
| §3.1 aya_sss_skin_flag 3 UBO | 3 | MaterialUBO_Legacy + PBROpaqueExtraUBO_Legacy + AvatarFParamUBO_Legacy |
| §3.2 aya_visual_realism + chroma_str + light cvar | 7 | AtmoExtraUBO_Legacy + SkinSSSPrototypeFParamUBO_Legacy + PostDeferredF/NoDoFF + PointLightF/SpotLightF/PointLightV |
| §3.3 shadow_target_width 3 UBO | 3 | ShadowAlphaMaskV + PbrShadowAlphaMask + AvatarAlphaShadow |
| §3.4 box_center/box_size 2 UBO | 2 | OcclusionCube + ShadowCubeV |
| §3.5 GLTF texture transform 3 UBO | 3 | PbrOpaqueV + PbrAlphaV + MaterialUBO |
| §3.6 water 系 5 UBO | 5 | WaterFog + WaterV + UnderWater + WaterF + WaterHazeV |
| §3.7 sky/cloud/atmospheric 10 UBO | 10 | FrameAtmosphere_Lighting + AtmoExtra + Sky V/F + Clouds V/F + Stars F/V + SunDisc + Moon |
| §3.8 velocity 5 UBO | 5 | AvatarSkin + AvatarVelocity + ObjectSkin + SkinnedVelocity + VelocityVParam + VelocityAlphaV |
| §3.9 reflection probe / IBL 5 UBO | 5 | Global_ReflectionProbes + ReflectionProbe + RadianceGen + IrradianceGen + Gaussian |
| §3.10 post-process chain 6 UBO | 6 | Exposure + Luminance + Tonemap + GammaCorrect + ColorGrading + Vignette |
| §3.11 glow chain 4 UBO | 4 | GlowExtract + GlowV + GlowF + GlowCombine |
| §3.12 SMAA 2 UBO | 2 | SMAAParam + SMAABlendWeights |
| §3.13 pathfinding 2 UBO | 2 | Pathfinding V + NoNormal |
| §3.14 GLTF asset 2 UBO | 2 | Asset_GLTFMaterials + Asset_GLTFNodes |
| §3.15 set=2 binding=0 共有 (残) 2 UBO | 2 | ClipPlane + LightParams |
| §3.16 MultiLight 1 UBO | 1 | MultiLight |
| **合計** | **62** | (重複あり、cross group UBO は AtmoExtra/SkinSSS/PerDrawUBO_LightParams 等) |

= 重複勘案後合計 62 件で集計合致

---

## §6. Phase 2 工程入力 (= 着手順序 ヒント)

**Phase 2 工程再設計時の着手順序候補** (= AYA judgement 入力、本 doc は判断材料のみ):

### §6.1 即着手可能 = A 判定 (1 件)

- Skin_GLTFJoints (= Phase 1.F+ で real bone matrix 接続) = Phase 2 内最短経路

### §6.2 独立着手可能 = B 判定 (31 件)

- setter site 特定 + data source 特定の grep 調査完了後着手
- 並列性: 互いに dirty 連動なし独立 UBO ゆえ複数並列着手可能
- ただし cadence mismatch 重大 UBO (= PerProgramUBO_FsObjectIdF) は cadence 再設計優先

### §6.3 group 単位着手 = C 判定 (62 件、主要 16 group)

- group 内 UBO は同時設計必須 (= cross-UBO dirty 同期 protocol を group 単位で確立)
- group 間は独立、並列可能 (= 例: glow chain と SMAA chain は独立)
- 大型 group (= sky/cloud/atmospheric 10 UBO、post-process chain 6 UBO) は中期着手
- pair UBO (= velocity 5 UBO、SMAA 2 UBO 等) は短期着手可能

### §6.4 横断 protocol 確立必須 (= 全 group 共通の設計入力)

- name-based dispatch logic (= set=1 binding=0 排他 + set=2 binding=0 共有 6 UBO + set=3 binding 衝突 3 site)
- LLStaticHashedString UBO redirect 経路 (= CAS / Clip / Luminance / VisualizeBuffersF / Exposure 5+ UBO 影響)
- per-shader UBO block 拡大 (= Frame UBO 多 file 拡大、upstream merge conflict risk)
- cadence 再評価 (= velocity / GLTF material / per-frame 変化 member の per-program stale risk 多数 UBO)

---

## §A. メタ情報

### §A.1 起案 source

- 各 UBO file §10 (不明事項) + §11 (他 UBO 関係) + §6 (既存 setter call site) + §7 (現状通電状態) 全 94 file (= sequential 1 file 1 Read 順次精査)
- RELATIONS.md §4 (data source 17 系列) + §5 (dirty 連動 group) + §8 (不明事項 15 dimension)

### §A.2 完成 verify

- 全 94 UBO の A/B/C 判定 ✅
- B/C 判定の理由詳細 全件記載 ✅
- 集計 = 1 + 31 + 62 = 94 ✅ (INDEX §2 一致)
- Phase 2 着手順序ヒント記載 ✅

### §A.3 関連 doc

- INDEX.md (= 全 94 UBO summary + cadence_tag mapping)
- 各 UBO file (`<UBO名>.md`) = 詳細個別資料
- RELATIONS.md = 関係図 7 dimension (= 同 dir)

---

## §B. Phase 2.α 案 X 確定 record (= blueprint dir 位置付け + 二重 source 同期 protocol)

### §B.1 blueprint dir の位置付け = codegen 入力 source of truth (= 案 X 確定 2026-06-06)

全 94 UBO の **codegen 入力 source of truth** = `indra/newview/app_settings/shaders/aya_r41_blueprints/<set>/<ubo_lower>.glsl` (= blueprint dir 内 .glsl file 群)。`indra/cmake/AyaUboCodegen.cmake` の `AYA_UBO_CODEGEN_BLUEPRINT_DIR` を経由して `scripts/ubo_codegen/main.py` の入力に渡され、`ubo_metadata.inl` + `ubo_layout_<ubo>.inl` を生成する。

**AYAstorm shader runtime compile target は別 GLSL 系統** (= `class*/` + `cinematic_bd/` 配下の実 shader use site) で並列 build process (= design/04-codegen-ubo.md §2.2 literal「別 GLSL 並列 build process」)。二系統は二重 source として共存し、**`scripts/ubo_codegen/main.py` の二重 source 同期 protocol で整合 verify** される (= §B.2)。

A/B/C 判定の根拠となる各 UBO の §5 use site (= 実 shader use site path) + §1 struct definition (= blueprint file path) は両方 valid な記載で、案 X 確定後も整合性は保たれる (= blueprint dir = codegen 入力、actual = runtime compile target、両方 source of truth として正しい)。

### §B.2 二重 source 同期 protocol = main.py で formal化

- **`_verify_block_match`** (= α-2 commit `b66ec99f72`) = 同名 UBO 複数 file (= blueprint + actual の cross-source pair、または cinematic_bd 上書き path) の set/binding + subset/cadence + member 全件 layout 一致を構造的 verify。不一致時 `CodegenError` で abort。
- **`_verify_blueprint_actual_consistency`** + **`--verify-target-paths`** option (= phase F commit `868bc38cc9`) = blueprint と actual の二重 source 整合 verify を formal化、blueprint と actual を区別して対称的 cross-verify、5 test 同梱。
- §3 C 判定 UBO 群 (= cross-UBO same data dirty 同期) は **本 §B.2 protocol で blueprint ↔ actual の layout 一致 verify が pre-requisite**、host 側 cross-UBO dispatch protocol 設計の入力となる。

### §B.3 cross-ref

- 案 X 確定 source of truth = `docs/specs/ayastorm-r41-gl-removal/handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D.9
- blueprint dir README = `indra/newview/app_settings/shaders/aya_r41_blueprints/README.md` (= phase B commit `f95182ded5`)
- 設計 doc = `design/04-codegen-ubo.md` §2.2 / §4.4
- 二重 source 同期 protocol = `scripts/ubo_codegen/main.py` `_verify_block_match` + `_verify_blueprint_actual_consistency`
- phase E (= blueprint dir 内 7 UBO 同期書換) = commit `09ee5e8a8e` (= sub-session 5 step 2-batch-0-a actual 改修と同期、二重 source 同期断裂解消)
- handoff `phase2-prep/handoff-phase2-prep-ubo-files-complete.md` = 残作業 + 起案規律
- **Phase 2.α 案 X cross-ref** (= 2026-06-06、UBO codegen 入力 source 整合修復 sub-phase):
  - `handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D.9 (= 案 X 確定 source of truth)
  - `handoff/phase2/alpha/handoff-phase2-alpha-3-cmake-blueprint-readme-propagation.md` §3.1-§3.7 (= α-3 7 phase 構造)
  - `indra/newview/app_settings/shaders/aya_r41_blueprints/README.md` (= blueprint dir = **codegen 入力 source of truth** literal)
  - **案 X 整合**: A/B/C 判定の前提となる codegen 入力 source は **blueprint dir 単独**、`class*/` + `cinematic_bd/` は runtime compile target、両者は別 GLSL 並列 build process + 二重 source 同期 protocol (= main.py phase F commit `868bc38cc9` `_verify_block_match` + `_verify_blueprint_actual_consistency` + `--verify-target-paths` option) で整合 verify。本 doc 内 blueprint 言及 (= `SoftenLightParamUBO_Legacy` member_count 表記揺れ / `CASParamUBO_Legacy` 6 member comment) は全件 GLSL declaration / header literal 引用、案 X 整合済。
