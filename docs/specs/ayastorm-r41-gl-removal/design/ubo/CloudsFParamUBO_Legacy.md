# CloudsFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= shell 通電なし、host C++ writer / register 配線無し、shader 側 LL_VULKAN_GLSL block でのみ宣言済)

**本実装化に必要な作業**: per-program cadence register / write 配線追加 + 既存 OpenGL 経路 setter (= cloud_pos_density1/2 + blend_factor + cloud_variance + AYA r18 cvar) の mUseUBO 分岐経路から `forwardToUboUpload` → `writeProgramUbo` 経由で本 UBO に書込み開始 + `cloudsF.glsl` の LL_VULKAN_GLSL block 活性化

---

## §1. UBO identity

- **block_name**: `CloudsFParamUBO_Legacy`
- **block_hash**: `0xe60e18aeu`
- **block_size**: 256 B (= std140 48 B、device-padded 256 B)
- **member_count**: 8
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_cloudsfparamubo_legacy.inl:12-21
struct CloudsFParamUBO_LegacyLayout {
    static constexpr std::uint32_t cloud_pos_density1_OFFSET = 0u;                  // size=12 align=16
    static constexpr std::uint32_t blend_factor_OFFSET = 12u;                       // size=4 align=4
    static constexpr std::uint32_t cloud_pos_density2_OFFSET = 16u;                 // size=12 align=16
    static constexpr std::uint32_t cloud_variance_OFFSET = 28u;                     // size=4 align=4
    static constexpr std::uint32_t aya_r18_cloud_volumetric_enabled_OFFSET = 32u;   // size=4 align=4
    static constexpr std::uint32_t aya_r18_strength_OFFSET = 36u;                   // size=4 align=4
    static constexpr std::uint32_t _pad_clouds_f_legacy_0_OFFSET = 40u;             // size=4 align=4
    static constexpr std::uint32_t _pad_clouds_f_legacy_1_OFFSET = 44u;             // size=4 align=4
};
inline constexpr std::uint32_t CloudsFParamUBO_Legacy_SIZE = 256u; // std140=48, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/clouds_f_param_ubo_legacy.glsl:9-19
layout(std140, set = 3, binding = 4) uniform CloudsFParamUBO_Legacy
{
    vec3  cloud_pos_density1;
    float blend_factor;
    vec3  cloud_pos_density2;
    float cloud_variance;
    int   aya_r18_cloud_volumetric_enabled;
    float aya_r18_strength;
    float _pad_clouds_f_legacy_0;
    float _pad_clouds_f_legacy_1;
};
```

= **WL cloud density params + blend + variance + AYAstorm r18 章 cvar 2 件 (= volumetric clouds enabled flag + strength)**

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 4
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:35` literal: `{ "CloudsFParamUBO_Legacy", 0xe60e18aeu, 256u, 3u, 4u, 0u, 1u, 8u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: cloud fragment shader program bind 単位で update
- **source**: ubo_metadata.inl:35 + llglslshader.cpp:95

---

## §4. 物理 owner

- **data source**:
  - `cloud_pos_density1` (vec3): WL cloud density 1 (= `llinventory/llsettingssky.cpp:84` literal: `SETTING_CLOUD_POS_DENSITY1("cloud_pos_density1")`)
  - `blend_factor` (float): WL cloud blend factor
  - `cloud_pos_density2` (vec3): WL cloud density 2
  - `cloud_variance` (float): WL cloud variance
  - `aya_r18_cloud_volumetric_enabled` (int): AYAstorm r18 章 cvar (= 推定 = settings.xml AYAR18CloudVolumetric* cvar、verify 要)
  - `aya_r18_strength` (float): AYAstorm r18 strength cvar
- **uniform 名 reserved**: `llshadermgr.cpp:1622` literal: `mReservedUniforms.push_back("cloud_pos_density1")`
- **uniform 名 ↔ enum**: `llshadermgr.h:149` literal: `CLOUD_POS_DENSITY1, // "cloud_pos_density1"`
- **既存 OpenGL 経路 writer site**: **不明 / verify 要** (= grep `CLOUD_POS_DENSITY1` / `blend_factor` / `cloud_variance` writer 特定要、推定 `llsettingsvo.cpp` 経路)
- **lifetime**: sky preset 切替時 / AYA r18 cvar 変更時

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/clouds_f_param_ubo_legacy.glsl`
- **実 shader use site** (= 1 file confirmed):
  - `indra/newview/app_settings/shaders/class1/deferred/cloudsF.glsl:61` (= blueprint source literal、`#ifdef LL_VULKAN_GLSL` block)
- **CloudsVParamUBO_Legacy 複製併存**: `cloudsF.glsl:79` literal で `CloudsVParamUBO_Legacy` を本 file 内に guard 付き複製 (= `CLOUDS_V_PARAM_UBO_LEGACY_DEFINED` guard、η-14 path G-β で cloud_scale 解決経路、`cloudsF.glsl:71-83`)

---

## §6. 既存 setter call site (host C++)

- **uniform 名 reserved**: `llshadermgr.cpp:1622` literal
- **uniform 名 ↔ enum**: `llshadermgr.h:149` literal
- **writer call site**: **不明 / verify 要** (= grep `CLOUD_POS_DENSITY1` で writer 特定要、推定 llsettingsvo.cpp 経路)
- **AYA r18 cvar writer**: 不明 / verify 要 (= AYAstorm r18 章実装)
- **本 UBO 名指 setter**: なし

---

## §7. 現状通電状態

- **状態**: **untouched** (= shell 通電もされていない)
- **PC-7γ-1 PerProgram register**: 条件付き
- **PC-7γ-1 PerProgram write**: 条件付き

---

## §8. 本実装化に必要な作業

1. **mUseUBO ON 化**
2. **shader 側 LL_VULKAN_GLSL block 活性化** (= cloudsF.glsl:61、起案済確認要)
3. **writer call site 特定** (= cloud_pos_density1/2 + blend_factor + cloud_variance + AYA r18 cvar)
4. **CloudsVParamUBO_Legacy 同 file 内複製整合**: cloudsF.glsl:79 で binding=3 (CloudsVParamUBO_Legacy) を fragment 側にも guard 付き複製、両 UBO 同時 bind の整合性確認
5. **setter 経路 verify**
6. **codegen 再実行不要**

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 (= set=3 binding=4) |
| OS-3 | std140 padding 厳守 | ✅ 48B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ⚠️ cloudsF.glsl に LL_VULKAN_GLSL block 追加 + CloudsVParamUBO_Legacy 複製追加 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**fragment 側 V param 複製の Vulkan path 整合**:
- `cloudsF.glsl:79` literal: 本来 vertex shader 側で参照される CloudsVParamUBO_Legacy (set=3 binding=3) を fragment 側にも複製、C++ side では descriptor set bind 単位で両 stage から参照可能 (= cloudsF.glsl コメント記載「pipeline 単位で両 stage から参照可能」)
- shader-only fix (= charter §3 #1)、C++ 改修不要

**設計原則 (1) Upstream 取り込みやすさ**:
- cloudsF.glsl は upstream LL 由来 + LL_VULKAN_GLSL block で AYAstorm 追加 + r18 章 cvar 追加 = upstream merge conflict 高 risk

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守)

1. **WL cloud uniform writer 特定** = cloud_pos_density1/2 + blend_factor + cloud_variance setter
2. **AYAstorm r18 章 cvar 詳細** = aya_r18_cloud_volumetric_enabled / aya_r18_strength の setter site
3. **CloudsVParamUBO_Legacy fragment 側複製の binding 衝突確認** = 同 pipeline 内 vertex/fragment 両 stage で同 binding 参照時の Vulkan 仕様整合

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- **CloudsFParamUBO_Legacy (binding=4、本 UBO)**
- **CloudsVParamUBO_Legacy (binding=3、密関連、本 UBO file 内に複製併存)**
- SkyFParamUBO_Legacy (binding=2)
- SkyVParamUBO_Legacy (binding=1)
- AtmoExtraUBO_Legacy (binding=0)

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- CloudsVParamUBO_Legacy / SkyFParamUBO_Legacy / SkyVParamUBO_Legacy / AtmoExtraUBO_Legacy (= 同 windlight 系)

### §11.3 同 shader consume UBO

- cloudsF.glsl 内同時宣言/consume:
  - **CloudsVParamUBO_Legacy** (set=3 binding=3、fragment 側複製、cloudsF.glsl:79 literal)
  - **AtmoExtraUBO_Legacy** (= 推定、verify 要、windlight 系)
  - **FrameAtmosphere_Lighting** (= 推定、verify 要)

### §11.4 同 data source UBO (= LLSettingsSky 由来)

- **CloudsVParamUBO_Legacy** (= 同 sky preset 由来、cloud_color / cloud_scale 含む)
- **AtmoExtraUBO_Legacy** (= 同 sky preset 由来、haze_horizon / cloud_shadow / sun_moon_glow_factor)
- **SkyFParamUBO_Legacy / SkyVParamUBO_Legacy** (= 同 sky preset 由来候補、verify 要)

### §11.5 dirty 連動 UBO

- sky preset 切替時に同時 dirty 推定: CloudsVParamUBO_Legacy / SkyFParamUBO_Legacy / SkyVParamUBO_Legacy / AtmoExtraUBO_Legacy / 本 UBO

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- set=3 帯 bind は `bindV3aStatic` 経路で全帯一括
- cloud program bind 時 PerProgram cadence triple-buffer flush
- vertex/fragment 両 stage で同 set=3 binding=3 参照 (= 複製併存パターン)

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.7 同期)

**Layer**: L4-7 sub-cluster (d) (= Clouds V/F pair 2 UBO + cloudsF.glsl:79 複製併存)
**status**: **起案済** (= 2026-06-06 C-6-b、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.7` (= single source of truth)
**要点**: 8 member (cloud_pos_density1 vec3 + blend_factor float + cloud_pos_density2 vec3 + cloud_variance float + AYA r18 cvar 2 件 + pad ×2)、sky preset 切替 trigger で WL cloud density 4 member dirty + AYA r18 volumetric clouds cvar 変化 trigger で 2 member dirty、cloud_pos_density1 reserved 登録 `llshadermgr.cpp:1622` + `llshadermgr.h:149` literal 確認、writer 全件不明 [要追加調査]、AYA r18 cvar setter 不明 [要追加調査]、本 UBO 内で CloudsVParamUBO_Legacy 複製併存 (= cloudsF.glsl:79、η-14 path G-β、両 stage 参照 SPIR-V validation 要)、blend_factor 名 sub-cluster (e) Stars/SunDisc と同名異 data source 候補 [要 verify D4 突合]、cadence PerProgram 維持、工数 group 全体 L 内
**関連**: L0-1 dispatch (= 衝突なし binding=4) / §3.5.7 sub-cluster (d) CloudsV (= V/F pair 複製併存元) / sub-cluster (e) StarsF/SunDiscF (= blend_factor 同名 cross verify) / sub-cluster (a) FrameAtmosphere_Lighting (= 同 LLSettingsSky 連動) / AYAstorm r18 視覚表現章 (= memory `project_ayastorm_visual_realism_chapter`)
