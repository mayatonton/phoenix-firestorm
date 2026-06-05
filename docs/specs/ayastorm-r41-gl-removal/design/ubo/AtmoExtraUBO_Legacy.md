# AtmoExtraUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= shell 通電なし、host C++ writer / register 配線無し、shader 側 LL_VULKAN_GLSL block でのみ宣言済)

**本実装化に必要な作業**: per-program cadence register / write 配線追加 + 既存 OpenGL 経路 setter (= llsettingsvo.cpp 由来 atmospheric uniform 群 + AYA r14/r16 cvar 値) の mUseUBO 分岐経路から `forwardToUboUpload` → `writeProgramUbo` 経由で本 UBO に書込み開始 + 4 shader (atmosphericsFuncs / skyV / skinSSSF / cloudsV) の LL_VULKAN_GLSL block 活性化

---

## §1. UBO identity

- **block_name**: `AtmoExtraUBO_Legacy`
- **block_hash**: `0x8f8b9c68u` (= FNV-1a("AtmoExtraUBO_Legacy"))
- **block_size**: 256 B (= std140 48 B、device-padded 256 B)
- **member_count**: 10
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_atmoextraubo_legacy.inl:12-23
struct AtmoExtraUBO_LegacyLayout {
    static constexpr std::uint32_t lightnorm_OFFSET = 0u;                                // size=12 align=16
    static constexpr std::uint32_t haze_horizon_OFFSET = 12u;                            // size=4 align=4
    static constexpr std::uint32_t cloud_shadow_OFFSET = 16u;                            // size=4 align=4
    static constexpr std::uint32_t sun_moon_glow_factor_OFFSET = 20u;                    // size=4 align=4
    static constexpr std::uint32_t aya_visual_realism_enabled_OFFSET = 24u;              // size=4 align=4
    static constexpr std::uint32_t aya_r14_volumetric_atmosphere_enabled_OFFSET = 28u;   // size=4 align=4
    static constexpr std::uint32_t aya_r14_strength_OFFSET = 32u;                        // size=4 align=4
    static constexpr std::uint32_t aya_r16_aerial_perspective_enabled_OFFSET = 36u;      // size=4 align=4
    static constexpr std::uint32_t aya_r16_strength_OFFSET = 40u;                        // size=4 align=4
    static constexpr std::uint32_t _pad_atmo_extra_legacy_0_OFFSET = 44u;                // size=4 align=4
};
inline constexpr std::uint32_t AtmoExtraUBO_Legacy_SIZE = 256u; // std140=48, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/atmo_extra_ubo_legacy.glsl:9-21
layout(std140, set = 3, binding = 0) uniform AtmoExtraUBO_Legacy
{
    vec3  lightnorm;
    float haze_horizon;
    float cloud_shadow;
    float sun_moon_glow_factor;
    int   aya_visual_realism_enabled;
    int   aya_r14_volumetric_atmosphere_enabled;
    float aya_r14_strength;
    int   aya_r16_aerial_perspective_enabled;
    float aya_r16_strength;
    float _pad_atmo_extra_legacy_0;
};
```

= **AYA r14/r16 視覚表現章 cvar 含む atmospheric extra params**、`lightnorm` (vec3 sun direction) + LL atmospheric uniform 3 件 + AYAstorm 視覚表現章 cvar 5 件

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 0
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **set=3 内 Legacy 帯**: binding=0 (= Asset_GLTFNodes と同 binding 番号だが set 同居の中で Legacy cadence_tag=1 別経路、`llvkloader.cpp` 内設計上は set=3 binding 番号空間共有)
- ⚠️ **binding=0 衝突注意**: Asset_GLTFNodes (cadence=3 PerAsset) と本 UBO (cadence=1 PerProgram) が同 set=3 binding=0 で binding 番号衝突 (= ubo_metadata.inl literal 確認)。Vulkan 仕様上 binding 番号は set 内で unique 必須ゆえ pipeline layout 上では分離必要 = verify 要
- **source**: `ubo_metadata.inl:29` literal: `{ "AtmoExtraUBO_Legacy", 0x8f8b9c68u, 256u, 3u, 0u, 0u, 1u, 10u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: shader program bind 単位で update、`sProgramUboDirty` triple-buffer 経路で flush
- **source**: ubo_metadata.inl:29 + llglslshader.cpp:95 + llglslshader.cpp:2147-2149

---

## §4. 物理 owner

- **data source** (= 10 member 別):
  - `lightnorm` (vec3): `LLSettingsSky` (= `llsettingsvo.cpp:869, 875, 1349` literal: `shader->uniform3fv(LLViewerShaderMgr::LIGHTNORM, light_direction)` / `LLViewerShaderMgr::LIGHTNORM`)
  - `haze_horizon` (float): `LLSettingsSky::getHazeHorizon()` (= `llsettingsvo.cpp:844` literal: `draw_real(shader, getHazeHorizon(), LLShaderMgr::HAZE_HORIZON)` + `llinventory/llsettingssky.cpp:78` literal: `SETTING_HAZE_HORIZON("haze_horizon")`)
  - `cloud_shadow` (float): `LLSettingsSky::getCloudShadow()` (= `llsettingsvo.cpp:849` literal: `draw_real(shader, getCloudShadow(), LLShaderMgr::CLOUD_SHADOW)` + `llinventory/llsettingssky.cpp:88` literal)
  - `sun_moon_glow_factor` (float): `LLSettingsSky::getSunMoonGlowFactor()` (= `llsettingsvo.cpp:1071` literal)
  - `aya_visual_realism_enabled` (int): AYA cvar (= 推定、verify 要)
  - `aya_r14_volumetric_atmosphere_enabled` (int): AYAstorm r14 章 cvar (= memory `project_ayastorm_r14_pivot_to_light` 章)
  - `aya_r14_strength` (float): AYAstorm r14 strength cvar
  - `aya_r16_aerial_perspective_enabled` (int): AYAstorm r16 章 cvar
  - `aya_r16_strength` (float): AYAstorm r16 strength cvar
- **uniform 名 reserved**: `llshadermgr.cpp:1614 / 1616 / 1841` literal
- **uniform 名 ↔ enum**: `llshadermgr.h:141 (HAZE_HORIZON) / 143 (CLOUD_SHADOW) / 358 (SUN_MOON_GLOW_FACTOR)` literal
- **lifetime**: sky preset 切替時 (haze_horizon / cloud_shadow / sun_moon_glow_factor / lightnorm) + AYA cvar 変更時 (aya_*) update

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/atmo_extra_ubo_legacy.glsl`
- **実 shader use site** (= 4 file confirmed):
  - `indra/newview/app_settings/shaders/class1/windlight/atmosphericsFuncs.glsl:85-101` literal (= 起源、η-1 §3.1 範式継承、`ATMO_EXTRA_UBO_LEGACY_DEFINED` guard wrap)
  - `indra/newview/app_settings/shaders/class1/deferred/skyV.glsl:112` literal (= η-14 path G 経由参照、Skybox 削除 trade-off で AtmoExtra 経由解決)
  - `indra/newview/app_settings/shaders/class1/deferred/skinSSSF.glsl` (= grep 確認)
  - `indra/newview/app_settings/shaders/class1/deferred/cloudsV.glsl` (= grep 確認)
- **shader registration**: `llviewershadermgr.cpp:854 / 957` literal: `shaders.push_back( make_pair( "windlight/atmosphericsFuncs.glsl", mShaderLevel[SHADER_WINDLIGHT] ) )` (= snippet shader、windlight consumer 全 program に link)

---

## §6. 既存 setter call site (host C++)

- **lightnorm**:
  - `llsettingsvo.cpp:869` literal: `shader->uniform3fv(LLViewerShaderMgr::LIGHTNORM, light_direction)`
  - `llsettingsvo.cpp:875` literal: 同 (= 別 path)
  - `llsettingsvo.cpp:1349` literal: 同
  - `lldrawpoolwater.cpp:298` literal: `shader->uniform3fv(LLViewerShaderMgr::LIGHTNORM, 1, rotated_light_direction.mV)`
- **haze_horizon**: `llsettingsvo.cpp:844` literal: `draw_real(shader, getHazeHorizon(), LLShaderMgr::HAZE_HORIZON)` + `llsettingsvo.cpp:1092` literal (= DefaultParam map)
- **cloud_shadow**: `llsettingsvo.cpp:849` literal: `draw_real(shader, getCloudShadow(), LLShaderMgr::CLOUD_SHADOW)` + `llsettingsvo.cpp:1101` literal
- **sun_moon_glow_factor**: `llsettingsvo.cpp:1071` literal: `shader->uniform1f(LLShaderMgr::SUN_MOON_GLOW_FACTOR, getSunMoonGlowFactor())`
- **aya_visual_realism_enabled / aya_r14_* / aya_r16_***: **不明 / verify 要** (= 推定 = settings.xml AYAVisualRealism* cvar + LLCachedControl 経由 shader uniform setter、grep verify 要)
- **本 UBO 名指 setter**: なし (= `Grep "AtmoExtraUBO_Legacy" indra/llrender` 結果 0 件、generic write 経路)

---

## §7. 現状通電状態

- **状態**: **untouched** (= shell 通電もされていない)
- **PC-7γ-1 PerProgram register**: 条件付き = mUseUBO=true 時 atmosphericsFuncs.glsl link 時に走る
- **PC-7γ-1 PerProgram write**: 条件付き = mUseUBO=true で `llsettingsvo.cpp` 内 uniform setter が走ると `writeProgramUbo` 経路に流れる (default OFF)

---

## §8. 本実装化に必要な作業

1. **mUseUBO ON 化** (= PerProgram cluster 共通 gate)
2. **shader 側 LL_VULKAN_GLSL block 活性化** (= 4 shader、起案済確認要)
3. **AYAstorm r14/r16 cvar の setter 経路特定** = settings.xml AYAVisualRealism* / AYARealism* cvar 等 (= 推定、grep verify 要)
4. **lightnorm 整合** (= vec3 sun direction が複数 path で write される、mUseUBO 経路で重複 write の安全性確認)
5. **既存 OpenGL 経路 verify**:
   - `llsettingsvo.cpp:844, 849, 869, 875, 1071, 1349` の uniform setter が `mUseUBO=true` 時に正しく forwardToUboUpload → writeProgramUbo へ dispatch
6. **codegen 再実行不要** (= layout / member 不変)

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 (= set=3 binding=0) |
| OS-3 | std140 padding 厳守 | ✅ 48B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ⚠️ 4 shader に LL_VULKAN_GLSL block 追加済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**binding=0 衝突 critical**:
- Asset_GLTFNodes (cadence=3 set=3 binding=0) と本 UBO (cadence=1 set=3 binding=0) が同 binding = pipeline layout 上 unique 必要
- 推定設計 = cadence_tag 別経路で binding 番号空間分離 (= AtmoExtraUBO_Legacy は Legacy 帯空間 binding=0、Asset_GLTFNodes は Asset 帯空間 binding=0、Vulkan descriptor set 設計上は別 subset)
- **verify 要** = pipeline layout 内の実 binding 番号割当 + descriptor set layout 内の subset 分離経路

**設計原則 (1) Upstream 取り込みやすさ**:
- atmosphericsFuncs.glsl / skyV.glsl / cloudsV.glsl は upstream LL 由来 + LL_VULKAN_GLSL block で AYAstorm 追加 = upstream merge conflict 高 risk

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守)

1. **AYAstorm r14/r16 cvar の uniform setter** = aya_r14_volumetric_atmosphere_enabled / aya_r14_strength / aya_r16_aerial_perspective_enabled / aya_r16_strength / aya_visual_realism_enabled の writer site (= AYAstorm 章別追加、grep verify 要)
2. **binding=0 衝突解決経路** = Asset_GLTFNodes との pipeline layout 上の分離経路 (= subset 値違いで分離か、別経路か)
3. **lightnorm の重複 writer** = `llsettingsvo.cpp:869 + 875 + 1349` + `lldrawpoolwater.cpp:298` の 4 writer の意味分離 (= which path is currently active for which shader)
4. **skinSSSF.glsl 内本 UBO 参照詳細** = 確認要
5. **`_pad_atmo_extra_legacy_0` の役割** = std140 16 B align 整合のための padding

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

本 UBO (binding=0) 周辺:
- **Asset_GLTFNodes (set=3 binding=0、cadence=3 PerAsset、binding 衝突候補、verify 要)**
- SkyVParamUBO_Legacy (binding=1)
- SkyFParamUBO_Legacy (binding=2)
- CloudsVParamUBO_Legacy (binding=3)
- CloudsFParamUBO_Legacy (binding=4)
- SoftenLightParamUBO_Legacy (binding=5)
- DeferredUtilParamUBO_Legacy (binding=6)

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram、88 件最大)

主要関連 (= 同じく atmospheric / windlight 系):
- SkyVParamUBO_Legacy (set=3 binding=1)
- SkyFParamUBO_Legacy (set=3 binding=2)
- CloudsVParamUBO_Legacy (set=3 binding=3)
- CloudsFParamUBO_Legacy (set=3 binding=4)
- SoftenLightParamUBO_Legacy (set=3 binding=5)

### §11.3 同 shader consume UBO

- atmosphericsFuncs.glsl 内同時宣言:
  - **FrameAtmosphere_Lighting** (= set=0 binding=2、verify 要 = blueprint コメント記載で η-14 path G の Lighting (set=0 binding=2 from atmosphericsHelpersV/atmosphericsFuncs) 整合)
- skyV.glsl 内同時 consume (= η-14 path G コメント記載):
  - FrameAtmosphere_Lighting + 本 UBO
- cloudsV.glsl 内同時 consume:
  - CloudsVParamUBO_Legacy (binding=3) と同居

### §11.4 同 data source UBO (= LLSettingsSky 由来)

- **SkyVParamUBO_Legacy** (= SkyV system uniform 由来候補、verify 要)
- **SkyFParamUBO_Legacy** (= SkyF system uniform 由来候補)
- **FrameAtmosphere_Lighting** (= per-frame atmospheric lighting 由来、verify 要)

### §11.5 dirty 連動 UBO

- **SkyVParamUBO_Legacy / SkyFParamUBO_Legacy** (= sky preset 切替時に同時 dirty 推定、verify 要)
- **FrameAtmosphere_Lighting** (= per-frame atmospheric 由来ゆえ毎 frame dirty)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- set=3 帯 bind は `bindV3aStatic` / `bindV3aRigged` で全帯一括 (= `llvkloader.cpp:2168, 2172`)
- shader program switch 時に PerProgram cadence の triple-buffer 経路で update

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.2 同期)

**Layer**: L4-2 sub-cluster (a) (= visual_realism 2 UBO cross-write)
**status**: **起案済** (= 2026-06-06 C-6、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.2` (= single source of truth)
**要点**: aya_visual_realism 2 UBO cross-write (= AtmoExtra + SkinSSS)、本 UBO `aya_visual_realism_enabled` offset=24 部分 write、他 9 member (lightnorm/haze_horizon/cloud_shadow/sun_moon_glow_factor/aya_r14_*/aya_r16_*) は §3.5.7 sky/cloud group trigger、binding=0 衝突 (Asset_GLTFNodes と) L0-1 dispatch で解決、4 shader (atmosphericsFuncs/skyV/skinSSSF/cloudsV) 改変ゼロ、工数 L (group 全体)、AYA r14/r16 cvar setter 未取得 [要追加調査]
**関連**: L0-1 dispatch (= binding=0 衝突解決) / L0-3 per-shader 拡大 (= atmosphericsFuncs snippet shader) / §3.5.7 sky/cloud group (= 9 member 共有) / §3.5.1 r20 SSS (= SkinSSS 経由交差) / §5.4 visual regression policy
