# WaterVParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.A PA-8 blueprint 起案済、Phase 1.C PC-2/PC-7δ per-program cadence 一括通電対象に含まれている可能性大 / verify 要)

**本実装化に必要な作業**: shell 6 member (= waveDir1 / waveDir2 / time / eyeVec / waterHeight / lightDir) を実 water V data (= LLDrawPoolWater / LLEnvironment water settings + LLViewerCamera 経由、verify 要) に置換 + dirty 判定 logic 追加 + 実 shader (= class1/environment/waterV.glsl + class3/environment/waterF.glsl 多 site verified identical) consume 接続検証

---

## §1. UBO identity

- **block_name**: `WaterVParamUBO_Legacy`
- **block_hash**: `0x4d192f45u`
- **block_size**: 256 B (= std140 64 B、device-padded 256 B)
- **member_count**: 6
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_watervparamubo_legacy.inl
struct WaterVParamUBO_LegacyLayout {
    static constexpr std::uint32_t waveDir1_OFFSET = 0u;  // size=8 align=8
    static constexpr std::uint32_t waveDir2_OFFSET = 8u;  // size=8 align=8
    static constexpr std::uint32_t time_OFFSET = 16u;  // size=4 align=4
    static constexpr std::uint32_t eyeVec_OFFSET = 32u;  // size=12 align=16
    static constexpr std::uint32_t waterHeight_OFFSET = 44u;  // size=4 align=4
    static constexpr std::uint32_t lightDir_OFFSET = 48u;  // size=12 align=16
};
inline constexpr std::uint32_t WaterVParamUBO_Legacy_SIZE = 256u; // std140=64, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/water_v_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 60) uniform WaterVParamUBO_Legacy
{
    vec2 waveDir1;
    vec2 waveDir2;
    float time;
    vec3 eyeVec;
    float waterHeight;
    vec3 lightDir;
};
```

= **blueprint literal extract from `class1/environment/waterV.glsl:61` `#ifdef LL_VULKAN_GLSL` block** (= blueprint header line 3 明示)
= **Multi-site: verified identical at `class3/environment/waterF.glsl:108`** (= blueprint header line 4 明示、V/F 両方で同一 UBO 宣言)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 60
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: `sAYAStandardLayout`
- **set 3 内訳**: 本 UBO binding=60 = Legacy 帯独立 binding
- **source**: `ubo_metadata.inl:119` `{ "WaterVParamUBO_Legacy", 0x4d192f45u, 256u, 3u, 60u, 0u, 1u, 6u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` `kCadencePerProgram = 1u`)
- **source**: ubo_metadata.inl:119 + llglslshader.cpp:95

---

## §4. 物理 owner

- **shell 段階 owner**: 未確認
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `waveDir1` = water wave direction 1 (vec2、LLEnvironment water settings)
  - `waveDir2` = water wave direction 2 (vec2、LLEnvironment water settings)
  - `time` = water animation time (float、per-frame time accumulator)
  - `eyeVec` = camera eye vector (vec3、LLViewerCamera 由来 camera direction)
  - `waterHeight` = water surface height (float、LLEnvironment water settings)
  - `lightDir` = sun/moon light direction (vec3、FrameLights.sun_dir / moon_dir 派生)
- **lifetime**: per-program (= water rendering shader instance)
- **bare uniform from waterV.glsl**: `lightDir` / `eyeVec` は元々 waterV.glsl で bare uniform として宣言されていた (= UnderWaterFParamUBO_Legacy §1 注記 `class3/environment/underWaterF.glsl:50-56` 参照)、本 UBO 化で rename なし (= waterV.glsl 内では nameless block member、同 shader 内では衝突なし、UnderWaterFParamUBO_Legacy 側で rename 対応)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/water_v_param_ubo_legacy.glsl` (= Phase 1.A PA-8 起案、Source `class1/environment/waterV.glsl:61`、Multi-site `class3/environment/waterF.glsl:108`)
- **実 shader use site**:
  - `class1/environment/waterV.glsl` (= V shader)
  - `class3/environment/waterF.glsl` (= F shader、同一 UBO 宣言 verified identical)
- **consume status**: blueprint literal extract source ゆえ既 consume、V/F 両方で UBO consume

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter** (= verify 要)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL setter = `uniform2fv("waveDir1"/"waveDir2", ...)` / `uniform1f("time"/"waterHeight", ...)` / `uniform3fv("eyeVec"/"lightDir", ...)` (= verify 要)
  - data source = LLDrawPoolWater / LLEnvironment LLSettingsWater + LLViewerCamera

---

## §7. 現状通電状態

- **状態**: **shell 通電有無 verify 要**
- **通電内容** (= 推定): zero dummy buffer write + per-program cadence 経路通電

---

## §8. 本実装化に必要な作業

1. **shell 6 member → 実 water V data 接続**:
   - LLEnvironment LLSettingsWater 経由 waveDir1/waveDir2/waterHeight 集約
   - LLViewerCamera 経由 eyeVec 集約
   - FrameLights.sun_dir / moon_dir 派生 lightDir 集約
   - per-frame time accumulator 接続
2. **dirty 判定 logic 追加**:
   - dirty 判定 trigger = camera move / water settings 切替 / day cycle 進行 (= per-frame 変化 member 多数、cadence 再評価要)
3. **flush logic 追加**:
   - per-program cadence ゆえ既経路活用
4. **shader 接続検証**:
   - 既存 `class1/environment/waterV.glsl` `#ifdef LL_VULKAN_GLSL` block (line 61 周辺) で UBO member access 動作確認
   - 既存 `class3/environment/waterF.glsl` `#ifdef LL_VULKAN_GLSL` block (line 108 周辺) で同一 UBO 宣言 + access 動作確認

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=3 内に収める | ✅ 維持 (binding=60 独立) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 64 B → 256 B padded (= vec2 + vec2 + float = 20 B → 16 B align で next vec3 OFFSET=32 (= 12 B align padding 投入) + vec3 + float = 48 B → 16 B align で next vec3 OFFSET=48 + vec3 = 60 B → 4 B implicit pad → 64 B std140) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint = 実 shader literal extract (V/F 両方で identical) |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**multi-site UBO 宣言**:
- waterV.glsl + waterF.glsl で同一 UBO 宣言 (= "Multi-site: verified identical")
- 同一 program (V/F pair) ゆえ shader 単位で UBO bind = 同 buffer 共有、descriptor 1 つで両 shader stage 参照
- Vulkan path で V shader と F shader が同一 binding access ゆえ二重 bind 不要 (= 通常 Vulkan stage 横断 UBO access パターン)

**cadence 再評価候補**:
- `time` / `eyeVec` / `lightDir` は per-frame で変化、per-program cadence stale risk
- per-frame 系への移動候補、Phase 2 で再評価

**bare uniform 元命名問題**:
- `lightDir` / `eyeVec` は元 waterV.glsl の bare uniform、UnderWaterFParamUBO_Legacy で rename 対応済 (= `lightDir_underwater_legacy` / `eyeVec_underwater_legacy`)
- 本 UBO は waterV.glsl 内ゆえ元命名維持、UnderWaterFParamUBO_Legacy 側で同名衝突回避

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- Asset/Skin 帯 + Legacy 帯各種、本 UBO binding=60 は独立

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- PerProgram cluster 全 88 件

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- `class1/environment/waterV.glsl` + `class3/environment/waterF.glsl` 内同時 consume UBO (= verify 要):
  - 推定候補: FrameViewProj (= view+proj matrix) + FrameAtmosphere_Lighting + FrameLights + WaterFogUBO_Legacy (set=3 binding=9) + PerProgramUBO_WaterF (set=2 binding=23)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **WaterFogUBO_Legacy** (= set=3 binding=9、PerProgram、5 member、ubo_metadata.inl:118) = water fog UBO、water rendering 共通 data source (= LLEnvironment LLSettingsWater)
- **UnderWaterFParamUBO_Legacy** (= set=3 binding=39、PerProgram、14 member、ubo_metadata.inl:115) = underwater F UBO、`lightDir_underwater_legacy` / `eyeVec_underwater_legacy` で本 UBO `lightDir` / `eyeVec` の rename 版を含む (= 同 host data source 確定)
- **PerProgramUBO_WaterF** (= set=2 binding=23、PerProgram、8 member、ubo_metadata.inl:92) = water F UBO、water rendering 共通 data source 可能性
- **PerProgramUBO_WaterHazeV** (= set=2 binding=15、PerProgram、4 member、ubo_metadata.inl:93) = water haze V UBO、water haze 系 data source 共有可能性

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **WaterFogUBO_Legacy** / **UnderWaterFParamUBO_Legacy** / **PerProgramUBO_WaterF** (= water rendering 系 同時 dirty 高確率、LLEnvironment water settings 切替 / camera move で連動)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- per-program cadence ゆえ waterV + waterF program bind 時に descriptor set 3 更新
- V/F 同一 UBO ゆえ 1 回 bind で V/F 両 stage 参照

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **shell 通電有無** = handoff doc chain 参照要
2. **6 member 各 data source** = LLEnvironment LLSettingsWater / LLViewerCamera / FrameLights 経路 (= verify 要)
3. **既存 OpenGL setter call site** = 6 member 各 uniform setter grep verify 要
4. **cadence 適正性** = per-frame 変化 member (time / eyeVec / lightDir) の per-program cadence stale risk (= verify 要)
5. **water rendering 系全 UBO 連動範囲** = WaterFogUBO_Legacy / UnderWaterFParamUBO_Legacy / PerProgramUBO_WaterF / PerProgramUBO_WaterHazeV の data source 共有 + dirty 連動経路 (= verify 要)
6. **bare uniform 経緯確認** = waterV.glsl 元 bare uniform `lightDir` / `eyeVec` の UBO 化前後の動作整合 (= verify 要、UnderWaterFParamUBO_Legacy 側 rename と本 UBO 維持の整合性)
7. **multi-site UBO 宣言 V/F 同期保証** = waterV.glsl + waterF.glsl の UBO 宣言 byte-for-byte 一致維持 (= verify 要、将来 blueprint 改変時の同期 gate 要)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。
