# WaterFogUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.A PA-8 blueprint 起案済、Phase 1.C PC-2/PC-7δ per-program cadence 一括通電対象に含まれている可能性大 / verify 要)

**本実装化に必要な作業**: shell 5 member (= waterFogColor / waterFogDensity / waterFogKS / _pad_waterfog_0 / _pad_waterfog_1) を実 water fog data (= LLDrawPoolWater / LLEnvironment water settings 経由、verify 要) に置換 + dirty 判定 logic 追加 + 実 shader (= class1/environment/waterFogF.glsl) consume 接続検証

---

## §1. UBO identity

- **block_name**: `WaterFogUBO_Legacy`
- **block_hash**: `0xb1b533deu`
- **block_size**: 256 B (= std140 32 B、device-padded 256 B)
- **member_count**: 5
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_waterfogubo_legacy.inl
struct WaterFogUBO_LegacyLayout {
    static constexpr std::uint32_t waterFogColor_OFFSET = 0u;  // size=16 align=16
    static constexpr std::uint32_t waterFogDensity_OFFSET = 16u;  // size=4 align=4
    static constexpr std::uint32_t waterFogKS_OFFSET = 20u;  // size=4 align=4
    static constexpr std::uint32_t _pad_waterfog_0_OFFSET = 24u;  // size=4 align=4
    static constexpr std::uint32_t _pad_waterfog_1_OFFSET = 28u;  // size=4 align=4
};
inline constexpr std::uint32_t WaterFogUBO_Legacy_SIZE = 256u; // std140=32, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/water_fog_ubo_legacy.glsl
layout(std140, set = 3, binding = 9) uniform WaterFogUBO_Legacy
{
    vec4  waterFogColor;
    float waterFogDensity;
    float waterFogKS;
    float _pad_waterfog_0;
    float _pad_waterfog_1;
};
```

= **blueprint literal extract from `class1/environment/waterFogF.glsl:48` `#ifdef LL_VULKAN_GLSL` block** (= blueprint header line 3 明示)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 9
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: `sAYAStandardLayout`
- **set 3 内訳**: 本 UBO binding=9 = Legacy 帯独立 binding
- **source**: `ubo_metadata.inl:118` `{ "WaterFogUBO_Legacy", 0xb1b533deu, 256u, 3u, 9u, 0u, 1u, 5u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` `kCadencePerProgram = 1u`)
- **source**: ubo_metadata.inl:118 + llglslshader.cpp:95

---

## §4. 物理 owner

- **shell 段階 owner**: 未確認
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `waterFogColor` = water fog color (vec4 RGBA、LLEnvironment water settings 由来)
  - `waterFogDensity` = water fog density (float、LLEnvironment water settings 由来)
  - `waterFogKS` = water fog Ks scattering coefficient (float、LLEnvironment water settings 由来)
  - `_pad_waterfog_0` / `_pad_waterfog_1` = std140 vec4 align padding (= 2 × float pad、unused)
- **lifetime**: per-program (= water fog rendering shader instance)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/water_fog_ubo_legacy.glsl` (= Phase 1.A PA-8 起案、Source `class1/environment/waterFogF.glsl:48`)
- **実 shader use site**:
  - `class1/environment/waterFogF.glsl` (= blueprint header line 3 literal reference)
- **同名 member 共有 shader** (= grep `waterFogColor|waterFogDensity|waterFogKS` 結果):
  - `class3/environment/underWaterF.glsl` (= UnderWaterFParamUBO_Legacy 内 `waterFogColor_underwater_legacy` / `waterFogKS_underwater_legacy` で rename 済、同 host data source 由来、UnderWaterFParamUBO_Legacy §11.4 で記述)
- **consume status**: blueprint literal extract source ゆえ既 consume

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter** (= verify 要)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL setter = `uniform4fv("waterFogColor", ...)` / `uniform1f("waterFogDensity", ...)` / `uniform1f("waterFogKS", ...)` (= verify 要)
  - data source = LLEnvironment water settings (= LLSettingsWater)

---

## §7. 現状通電状態

- **状態**: **shell 通電有無 verify 要**
- **通電内容** (= 推定): zero dummy buffer write + per-program cadence 経路通電

---

## §8. 本実装化に必要な作業

1. **shell 3 active member → 実 water fog data 接続**:
   - LLEnvironment LLSettingsWater 経由 waterFogColor / waterFogDensity / waterFogKS 集約
2. **dirty 判定 logic 追加**:
   - dirty 判定 trigger = LLEnvironment water settings 切替 / day cycle 進行
3. **flush logic 追加**:
   - per-program cadence ゆえ既経路活用
4. **shader 接続検証**:
   - 既存 `class1/environment/waterFogF.glsl` `#ifdef LL_VULKAN_GLSL` block (line 48 周辺) で UBO member access 動作確認

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=3 内に収める | ✅ 維持 (binding=9 独立) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 32 B → 256 B padded (vec4 16 B + 4 × float 16 B = 32 B std140) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint = 実 shader literal extract |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**UnderWaterFParamUBO_Legacy との data source 重複**:
- UnderWaterFParamUBO_Legacy 内 `waterFogColor_underwater_legacy` / `waterFogKS_underwater_legacy` は本 UBO `waterFogColor` / `waterFogKS` の rename 版 (= 衝突回避 rename、UnderWaterFParamUBO_Legacy §1 注記)
- 同 host data source から派生 → 2 UBO に重複書込み発生 (= duplicate write 性能 risk)
- 集約 host setter で両 UBO 同時 update + dirty 連動経路要 (= Phase 2 で重複書込み最適化検討)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- Asset/Skin 帯 + Legacy 帯各種、本 UBO binding=9 は独立

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- PerProgram cluster 全 88 件

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- `class1/environment/waterFogF.glsl` 内同時 consume UBO (= verify 要):
  - 推定候補: FrameViewProj (= view+proj matrix) + FrameAtmosphere_Lighting

### §11.4 同 data source UBO (= 同 host data source から派生)

- **UnderWaterFParamUBO_Legacy** (= set=3 binding=39、PerProgram、14 member、ubo_metadata.inl:115) = underwater F UBO、本 UBO `waterFogColor` / `waterFogKS` を `_underwater_legacy` suffix rename で含む (= 同 host data source 確定)
- **PerProgramUBO_WaterF** (= set=2 binding=23、PerProgram、8 member、ubo_metadata.inl:92) = water F UBO、water rendering 共通 data source 可能性 (= verify 要、PerProgramUBO_WaterF blueprint Read 要)
- **WaterVParamUBO_Legacy** (= set=3 binding=60、PerProgram、6 member、ubo_metadata.inl:119) = water V UBO、water rendering 共通 data source 可能性

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **UnderWaterFParamUBO_Legacy** (= 同 host data source、LLEnvironment water settings 切替で同時 dirty 確定)
- **PerProgramUBO_WaterF** / **WaterVParamUBO_Legacy** (= water rendering 系 同時 dirty 可能性大)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- per-program cadence ゆえ waterFogF program bind 時に descriptor set 3 更新

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **shell 通電有無** = handoff doc chain 参照要
2. **waterFogColor/Density/KS data source** = LLEnvironment LLSettingsWater 内 state 構造 (= verify 要)
3. **既存 OpenGL setter call site** = uniform4fv("waterFogColor", ...) / uniform1f("waterFogDensity"/"waterFogKS", ...) grep verify 要
4. **UnderWaterFParamUBO_Legacy との重複書込み最適化** = 同 host data source から 2 UBO 同時 update 経路設計 (= Phase 2 で重複書込み回避策検討要)
5. **dirty 連動経路** = LLEnvironment water settings 切替時の 2+ UBO 同時 dirty 配線 (= verify 要)
6. **PerProgramUBO_WaterF / WaterVParamUBO_Legacy との data source 共有** = water rendering 全体の共通 data source 棚卸し (= verify 要)
7. **同 shader consume UBO 完全特定** = class1/environment/waterFogF.glsl 内同時 consume UBO 群 (= grep verify 要)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。
