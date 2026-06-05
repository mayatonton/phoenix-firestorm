# FrameAtmosphere_Lighting — UBO design (= 実コードベース調査資料)

**通電状態**: shell 通電済 + write 経路本格化済 (= Phase 1.A PA-8 blueprint 起案 + 1.C PC-7γ-1、`sFrameUboInstances` allocate + `writeFrameUbo` PER_FRAME case 通電、実 shader UBO block は `#ifdef LL_VULKAN_GLSL` block 内 atmosphericsF / lightAlphaMaskF / lightAlphaMaskNonIndexedF 等で宣言済、`blue_horizon` setter は `llsettingsvo.cpp:1057` literal で BLUE_HORIZON LLShaderMgr 経由)

**本実装化に必要な作業**: 既存 atmospheric color setter 群 (= sunlight_color / moonlight_color / ambient_color / blue_horizon / blue_density / haze_density / density_multiplier / distance_multiplier 等) の forwardToUboUpload PER_FRAME 経由 UBO redirect 通電 + per-shader UBO block 拡大

---

## §1. UBO identity

- **block_name**: `FrameAtmosphere_Lighting`
- **block_hash**: `0x14974e57u` (= `ubo_metadata.inl:40` literal)
- **block_size**: 256 B (= std140=128, device-padded=256、`ubo_layout_frameatmosphere_lighting.inl:34` literal)
- **member_count**: 20
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_frameatmosphere_lighting.inl
struct FrameAtmosphere_LightingLayout {
    static constexpr std::uint32_t sunlight_color_OFFSET        = 0u;   // size=12 align=16
    static constexpr std::uint32_t scene_light_strength_OFFSET  = 12u;  // size=4  align=4
    static constexpr std::uint32_t moonlight_color_OFFSET       = 16u;  // size=12 align=16
    static constexpr std::uint32_t haze_density_OFFSET          = 28u;  // size=4  align=4
    static constexpr std::uint32_t ambient_color_OFFSET         = 32u;  // size=12 align=16
    static constexpr std::uint32_t density_multiplier_OFFSET    = 44u;  // size=4  align=4
    static constexpr std::uint32_t blue_horizon_OFFSET          = 48u;  // size=12 align=16
    static constexpr std::uint32_t distance_multiplier_OFFSET   = 60u;  // size=4  align=4
    static constexpr std::uint32_t blue_density_OFFSET          = 64u;  // size=12 align=16
    static constexpr std::uint32_t max_y_OFFSET                 = 76u;  // size=4  align=4
    static constexpr std::uint32_t glow_OFFSET                  = 80u;  // size=12 align=16
    static constexpr std::uint32_t sky_sunlight_scale_OFFSET    = 92u;  // size=4  align=4
    static constexpr std::uint32_t sky_ambient_scale_OFFSET     = 96u;  // size=4  align=4
    static constexpr std::uint32_t sky_hdr_scale_OFFSET         = 100u; // size=4  align=4
    static constexpr std::uint32_t classic_mode_OFFSET          = 104u; // size=4  align=4
    static constexpr std::uint32_t cube_snapshot_OFFSET         = 108u; // size=4  align=4
    static constexpr std::uint32_t minimum_alpha_OFFSET         = 112u; // size=4  align=4
    static constexpr std::uint32_t max_cof_OFFSET               = 116u; // size=4  align=4
    static constexpr std::uint32_t _pad_atm0_OFFSET             = 120u; // size=4  align=4
    static constexpr std::uint32_t _pad_atm1_OFFSET             = 124u; // size=4  align=4
};
inline constexpr std::uint32_t FrameAtmosphere_Lighting_SIZE = 256u; // std140=128, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set0/frame_atmosphere_lighting.glsl
layout(std140, set = 0, binding = 2) uniform FrameAtmosphere_Lighting
{
    vec3  sunlight_color;          float scene_light_strength;
    vec3  moonlight_color;         float haze_density;
    vec3  ambient_color;           float density_multiplier;
    vec3  blue_horizon;            float distance_multiplier;
    vec3  blue_density;            float max_y;
    vec3  glow;                    float sky_sunlight_scale;
    float sky_ambient_scale; float sky_hdr_scale;
    int   classic_mode;      int   cube_snapshot;
    float minimum_alpha;     float max_cof;
    float _pad_atm0;         float _pad_atm1;
};
```

= **20 member 全 std140 で vec3 + float tail packing**、Phase 1.A AYA option 採用 = atmospheric color 7 (sunlight / moonlight / ambient / blue_horizon / blue_density / glow + scene_light_strength) + density 関連 3 (haze_density / density_multiplier / distance_multiplier) + sky scale 3 (sky_sunlight_scale / sky_ambient_scale / sky_hdr_scale) + mode flag 2 (classic_mode / cube_snapshot) + post pass 2 (minimum_alpha / max_cof) + pad 2

---

## §2. binding 配線

- **descriptor_set**: 0
- **binding**: 2
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **set 0 内訳** (= `llvkloader.cpp:858` `V3A_FRAME_SET_BINDINGS = 4`):
  - set=0 binding=0 = FrameViewProj
  - set=0 binding=1 = FrameLights
  - set=0 binding=2 = **FrameAtmosphere_Lighting (本 UBO)**
  - set=0 binding=3 = Global_ReflectionProbes
- **source**: `ubo_metadata.inl:40` `{ "FrameAtmosphere_Lighting", 0x14974e57u, 256u, 0u, 2u, 0u, 0u, 20u }` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 0
- **意味**: **per-frame** (= `llglslshader.cpp:94`)
- **flush 経路**: `LLVKLoader::flushFrameUbos()` (= `llvkloader.cpp:5117`)
- **write 経路**: `LLVKLoader::writeFrameUbo` (= `llglslshader.cpp:2144`)
- **storage**: `sFrameUboInstances`

---

## §4. 物理 owner

- **owner**: `LLEnvironment` / `LLSettingsSky` (= environment system、sky settings 由来)
- **data source**:
  - `sunlight_color` / `moonlight_color` / `ambient_color` / `blue_horizon` / `blue_density` / `glow` = `LLSettingsSky::getSunlightColor` 等 getter (= `llsettingssky.cpp:74,77,105` SETTING_BLUE_HORIZON / SETTING_HAZE_DENSITY / SETTING_SUNLIGHT_COLOR literal)
  - `haze_density` / `density_multiplier` / `distance_multiplier` / `max_y` = 同 settings 由来
  - `scene_light_strength` = `LLSettingsSky::getSceneLightStrength` (= verify 要)
  - `sky_sunlight_scale` / `sky_ambient_scale` / `sky_hdr_scale` = sky shader 用 scale factor (= verify 要)
  - `classic_mode` / `cube_snapshot` = mode flag (= cube snapshot 中の特殊処理 gate)
  - `minimum_alpha` / `max_cof` = post-process 用 param (= post-process shader 由来)
- **lifetime**: per-frame (= environment transition で連続変化、frame 開始時にスナップ)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set0/frame_atmosphere_lighting.glsl` (= Phase 1.A PA-8 起案)
- **blueprint origin (literal extract source)**: `class1/windlight/atmosphericsF.glsl:37` (= `frame_atmosphere_lighting.glsl:4` literal comment、4 sample sites = atmosphericsF / atmosphericsHelpersV / atmosphericsFuncs / atmosphericsHelpersF)
- **実 shader use site** (= UBO block 宣言済 shader、grep verify):
  - `class1/lighting/lightAlphaMaskF.glsl:37-45` (= sunlight_color 等 8 member 部分宣言)
  - `class1/lighting/lightAlphaMaskNonIndexedF.glsl:37-45` (= 同)
  - blueprint origin sample 4 file (= atmosphericsF / atmosphericsHelpersV / atmosphericsFuncs / atmosphericsHelpersF)
- **raw uniform 並走 pattern** (= `#else` block):
  - `class1/deferred/skyV.glsl:117-126` literal: `uniform vec3 sunlight_color; uniform vec3 moonlight_color; uniform vec3 ambient_color; uniform vec3 blue_horizon; uniform vec3 blue_density; uniform float haze_density; uniform float density_multiplier; uniform float distance_multiplier;`
  - = OpenGL path 維持

---

## §6. 既存 setter call site (host C++)

- **`blue_horizon` setter**: `llsettingsvo.cpp:1057` literal: `shader->uniform3fv(LLShaderMgr::BLUE_HORIZON, blue_horizon.mV);` (= `llsettingsvo.cpp:1051` で `LLColor3 blue_horizon = getBlueHorizon() * auto_adjust_blue_horizon_scale;`)
- **他 atmospheric color setter**: 不明 / verify 要 (= `llsettingsvo.cpp` 内に同類 setter site あり推定、`LLShaderMgr::SUNLIGHT_COLOR` / `MOONLIGHT_COLOR` / `AMBIENT_COLOR` / `BLUE_DENSITY` / `HAZE_DENSITY` 等経由)
- **`scene_light_strength` setter**: 不明 / verify 要
- **`sky_*_scale` setter**: 不明 / verify 要 (= sky shader 用 scale)
- **`classic_mode` / `cube_snapshot` setter**: 不明 / verify 要 (= mode flag)
- **`minimum_alpha` / `max_cof` setter**: 不明 / verify 要 (= post-process pass 由来)
- **共通 redirect 経路**: `LLGLSLShader::uniform3fv` / `uniform1f` / `uniform1i` (= 既 PC-7γ-1 で UBO redirect 通電)
- **PER_FRAME case**: `forwardToUboUpload` PER_FRAME → `writeFrameUbo`

---

## §7. 現状通電状態

- **状態**: **shell 通電済 + write 経路本格化済** (= Phase 1.A PA-8 + 1.C PC-7γ-1)
- **bind 経路**: `llvkloader.cpp:2839` 経由 set=0 全 4 UBO 同時 bind
- **write 経路**: `forwardToUboUpload` PER_FRAME → `writeFrameUbo` → memcpy + dirty
- **MUSEUBO-A 整合**: `mUseUBO=false` default で本 entry 不到達

---

## §8. 本実装化に必要な作業

1. **`mUseUBO=true` cold launch 検証**:
   - `llsettingsvo.cpp:1057` BLUE_HORIZON setter から forwardToUboUpload PER_FRAME 経由 writeFrameUbo 実走確認
   - 20 member 全 offset memcpy 正常性 (= 特に vec3 + float tail packing の offset 整合)
2. **per-shader UBO consume 拡大**:
   - 現 blueprint origin 4 sample + grep ヒット 2 file (lightAlphaMaskF / lightAlphaMaskNonIndexedF) で UBO block 宣言済
   - 残 sky / atmospheric shader (= class1/deferred/skyV / skyF + windlight shader 群) に対し UBO block 宣言拡大要
3. **setter site 完全特定**:
   - `LLShaderMgr::SUNLIGHT_COLOR` / `MOONLIGHT_COLOR` / `AMBIENT_COLOR` / `BLUE_DENSITY` / `HAZE_DENSITY` / `DENSITY_MULTIPLIER` / `DISTANCE_MULTIPLIER` / `MAX_Y` / `GLOW` 等の setter call site grep 要
   - `sky_*_scale` / `classic_mode` / `cube_snapshot` / `minimum_alpha` / `max_cof` setter 経路特定要
4. **mat3 packing 不在の確認**:
   - 本 UBO は mat3/mat4 不在、std140 vec3 + float tail packing 80% (= 5 group ×16B)、Frame 系最小 size
5. **codegen 再実行不要**:
   - blueprint member 不変、layout 改変なし

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 (= set 0 binding=2) |
| OS-3 | std140 padding 厳守 | ✅ 128 B std140 → 256 B padded、vec3 + float tail packing 検証済 |
| OS-4 | minUniformBufferOffsetAlignment | ⚠️ verify 要 (= 256 B = alignment 倍数) |
| OS-5 | shader 改変ゼロ | ⚠️ per-shader UBO block 拡大必須、`#ifdef LL_VULKAN_GLSL` gate |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**vec3 + float tail packing risk**:
- `vec3 sunlight_color; float scene_light_strength;` 形式 ×5 group + scale ×3 + mode flag ×2 + post pass ×2 + pad ×2 = 20 member
- std140 で vec3 が 16 B align ゆえ trailing 4 B に float を入れることで 16 B 単位 packing
- host C++ data layout も同 packing 想定 (= raw uniform 経路と一致するか verify 要)

**post pass member (minimum_alpha / max_cof)**:
- atmosphere 本来の member でなく、post-process pass の共有 param が同居 (= Phase 1.A AYA option で集約)
- 別 cadence (= per-program) 候補だが、frame 内 stable ゆえ per-frame 配置採用 (= verify 要 = setter trigger 経路)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=0)

- FrameViewProj (set=0 binding=0)
- FrameLights (set=0 binding=1)
- **FrameAtmosphere_Lighting (set=0 binding=2、本 UBO)**
- Global_ReflectionProbes (set=0 binding=3)

### §11.2 同 cadence cluster UBO (= cadence_tag=0 per-frame)

- FrameViewProj
- FrameLights
- **FrameAtmosphere_Lighting (本 UBO)**

### §11.3 同 shader consume UBO

- 推定: atmospheric shader (atmosphericsV / atmosphericsF 等) で同時 consume = FrameLights (= 同 windlight 系 lighting shader、blueprint origin が atmosphericsV/F)
- sky shader (skyV / skyF) で sunlight_color / blue_horizon 等 consume → FrameViewProj 経由 modelview も同時 consume

### §11.4 同 data source UBO

- **同 owner**: `LLEnvironment` / `LLSettingsSky` (= environment system)
- 他 UBO で同 owner = `FrameLights` (= sun_dir / moon_dir 経由、environment system 由来連動)

### §11.5 dirty 連動 UBO

- 推定: `FrameLights` (= environment transition で sun_dir 等と sunlight_color 同時更新)
- verify 要 (= dirty trigger 経路 = `llsettingsvo.cpp` 内 batch update site)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- frame start で set=0 全 4 UBO 同時 bind

---

## §10. 不明事項

1. **`sunlight_color` / `moonlight_color` / `ambient_color` 等の setter call site** = `llsettingsvo.cpp` 内 `LLShaderMgr::SUNLIGHT_COLOR` 等経由想定、grep verify 要
2. **`scene_light_strength` / `sky_*_scale` setter call site** = 推定 environment system 由来、`llsettingsvo.cpp` 内 setter site 特定要
3. **`classic_mode` / `cube_snapshot` setter call site** = mode flag、reflection probe regenerate 時 set 等想定
4. **`minimum_alpha` / `max_cof` setter call site** = post-process pass 由来、`pipeline.cpp` 内 post-deferred 経路 setter site
5. **post pass member の per-frame 配置妥当性** = `minimum_alpha` / `max_cof` が atmospheric 系と同 cadence (per-frame) 整合か (= per-program 候補)、Phase 1.A option 採用根拠 verify 要
6. **dirty 連動 detail** = environment transition 時の `FrameLights` との同時 dirty trigger 経路
7. **per-shader UBO block 拡大対象** = grep verify (= 現確認 6 file、残 sky / atmospheric / lighting shader での宣言状況)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。
