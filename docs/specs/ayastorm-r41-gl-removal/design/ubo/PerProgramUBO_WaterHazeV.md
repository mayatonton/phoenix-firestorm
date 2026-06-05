# PerProgramUBO_WaterHazeV — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `per_program_ubo_water_haze_v.glsl` は実 shader `class3/deferred/waterHazeV.glsl ifdef LL_VULKAN_GLSL block` から literal extract 済 (= shader 側 UBO declaration 既存、V+F shared host 1 bind)。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加 + 実 shader binding 接続。

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_WaterHazeV`
- **block_hash**: `0x341ff24cu` (= FNV-1a("PerProgramUBO_WaterHazeV"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 4
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_waterhazev.inl
struct PerProgramUBO_WaterHazeVLayout {
    static constexpr std::uint32_t above_water_OFFSET = 0u;  // size=4 align=4
    static constexpr std::uint32_t _pad_waterhaze0_OFFSET = 4u;  // size=4 align=4
    static constexpr std::uint32_t _pad_waterhaze1_OFFSET = 8u;  // size=4 align=4
    static constexpr std::uint32_t _pad_waterhaze2_OFFSET = 12u;  // size=4 align=4
};
inline constexpr std::uint32_t PerProgramUBO_WaterHazeV_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_water_haze_v.glsl
layout(std140, set = 2, binding = 15) uniform PerProgramUBO_WaterHazeV
{
    int   above_water;
    float _pad_waterhaze0;
    float _pad_waterhaze1;
    float _pad_waterhaze2;
};
```

= 実 data member は `above_water` (int) 1 個のみ、残 3 個は std140 vec4 align 確保用 explicit padding member (= blueprint comment `set=2 帯 binding=15 / cadence=PerProgram / 4 member (int + 3 float pad)`)。

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 15
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、verify 要)
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:94` `{ "PerProgramUBO_WaterHazeV", 0x341ff24cu, 256u, 2u, 15u, 0u, 1u, 4u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush、program 単位で値を保持
- **source**: ubo_metadata.inl:94 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - water haze 描画 pipeline (= `LLPipeline::renderWaterHaze` / `LLDrawPoolWater` 経路、grep verify 要)
  - camera 位置の水面相対判定 (= viewer camera が water plane の上か下か bool、verify 要)
- **lifetime**: program 単位 (= water haze shader program bind 中のみ有効)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_water_haze_v.glsl`
- **実 shader use site**:
  - **`class3/deferred/waterHazeV.glsl:86 ifdef LL_VULKAN_GLSL block`** (= primary、blueprint comment literal)
  - **`class3/deferred/waterHazeF.glsl`** (= 同 layout、blueprint comment literal: `verified identical across 2 sample sites = waterHazeV / waterHazeF [V+F shared、host 1 bind]`)
- **使用 uniform**: above_water (int) / `_pad_waterhaze0/1/2` (float、padding only)
- **特記**: V+F shared (= vertex shader と fragment shader で同 UBO declaration、host 側 bind は 1 回のみ)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL 経路 setter (= water haze program の `uniform1i(above_water, ...)` 呼出 site、grep verify 要)
  - 31 setter 経路で UBO 化対応要

---

## §7. 現状通電状態

- **状態**: **untouched**
- **通電 commit**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step 範囲、verify 要)

---

## §8. 本実装化に必要な作業

1. **shell 通電** + dummy buffer write + bind 経路通電
2. **実 member data 流入**: 既存 OpenGL 経路 `uniform1i(above_water, ...)` 呼出 site (= grep verify 要) を UBO 化
3. **dirty 判定 logic 追加**: PerProgram cadence ゆえ program 切替時に dirty
4. **flush logic 追加**: PerProgram cadence flush (= `writePerProgramUbo` 経路)
5. **shader 接続**: 実 shader 既存 UBO declaration 使用 = 追加 shader 改変なし
6. **dead padding 維持**: `_pad_waterhaze0/1/2` は std140 vec4 align 確保用、layout 不変契約で維持必須

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=2 binding=15 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**V+F shared bind risk**: V と F で同 UBO declaration、host bind 1 回で V+F 両方に対応 = shader stage 両方含む VkShaderStageFlags 指定要 (= VERTEX | FRAGMENT)、verify 要。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)
- set=2 帯 PerDraw+PerProgram 混在

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)
- 73 件 (推定) cluster の 1 個

### §11.3 同 shader consume UBO (= waterHazeV.glsl / waterHazeF.glsl)
- **不明 / verify 要** = waterHaze V/F 内で他に consume される UBO (= Frame 系 + WaterFogUBO_Legacy / 他 water 系 UBO 可能性、grep verify 要)

### §11.4 同 data source UBO
- **可能性** (= verify 要): 他 water 系 UBO (WaterFog/WaterF/WaterV/UnderWaterF) = camera 水面相対状態を共有する可能性

### §11.5 dirty 連動 UBO
- **不明 / verify 要**

### §11.6 layout 共有関係
- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係
- PerProgram cadence ゆえ program bind 時に同時 bind、V+F 両 stage 共用

---

## §10. 不明事項

1. **既存 OpenGL 経路 setter call site** = `above_water` uniform 書込 site (= grep verify 要)
2. **above_water bool 判定 logic** = camera 位置 vs water plane Z 判定実装 (= grep verify 要、`LLPipeline` / `LLViewerCamera` 内)
3. **V+F shared 確認** = waterHazeV.glsl と waterHazeF.glsl の UBO declaration literal 一致 (= blueprint comment 主張、独立 verify 要)
4. **shell 通電 commit** = 未来作業、現時点不明
5. **water 関連 UBO 群との data source 共有関係** = 確認要
6. **PerProgram flush 経路の VkDescriptorBufferInfo bind 詳細** = verify 要
7. **VkShaderStageFlags 指定** = V+F shared ゆえ VERTEX | FRAGMENT 両指定要 (= verify 要)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。
