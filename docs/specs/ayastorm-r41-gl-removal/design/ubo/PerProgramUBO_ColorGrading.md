# PerProgramUBO_ColorGrading — UBO design (= 実コードベース調査資料)

**通電状態**: **untouched** (= host C++ で `PerProgramUBO_ColorGrading` への setter 呼出ゼロ、grep 確認済 2026-06-06)

**本実装化に必要な作業**: 実 color grading 系定数 (= saturation / contrast / temperature / brightness / LUT intensity / LUT enabled) を host から PerProgram cadence setter 経由で書込 + class1/deferred/postDeferredTonemap.glsl 内 UBO consume へ切替

**章 thesis 関連**: AYAstorm r14+ 視覚表現章 + r30 Cinematic Control の color grading (= memory `project_ayastorm_visual_realism_chapter.md`) と直結 UBO

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_ColorGrading`
- **block_hash**: `0xbf80cb68u`
- **block_size**: 256 B (= std140=32 B, device-padded 256 B)
- **member_count**: 8 (= 実 6 + pad 2)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_colorgrading.inl:12-21
struct PerProgramUBO_ColorGradingLayout {
    static constexpr std::uint32_t color_saturation_OFFSET = 0u;
    static constexpr std::uint32_t color_contrast_OFFSET = 4u;
    static constexpr std::uint32_t color_temperature_OFFSET = 8u;
    static constexpr std::uint32_t color_brightness_OFFSET = 12u;
    static constexpr std::uint32_t color_grading_lut_intensity_OFFSET = 16u;
    static constexpr std::uint32_t color_grading_lut_enabled_OFFSET = 20u;
    static constexpr std::uint32_t _pad_cg0_OFFSET = 24u;
    static constexpr std::uint32_t _pad_cg1_OFFSET = 28u;
};
inline constexpr std::uint32_t PerProgramUBO_ColorGrading_SIZE = 256u; // std140=32, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_color_grading.glsl:9-19
layout(std140, set = 2, binding = 4) uniform PerProgramUBO_ColorGrading
{
    float color_saturation;
    float color_contrast;
    float color_temperature;
    float color_brightness;
    float color_grading_lut_intensity;
    int   color_grading_lut_enabled;
    float _pad_cg0;
    float _pad_cg1;
};
```

= **5 float + 1 int (LUT enable flag) + 2 pad、postDeferredTonemap per-program 定数**

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 4
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC`
- **pipeline layout**: `sAYAStandardLayout`
- **source**: ubo_metadata.inl:74 `{ "PerProgramUBO_ColorGrading", 0xbf80cb68u, 256u, 2u, 4u, 0u, 1u, 8u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram**
- **flush 経路**: PerProgram cadence setter

---

## §4. 物理 owner

- **shell 段階**: owner なし
- **本実装化後の data source** (= **verify 要**):
  - `LLPipeline` / `gSavedSettings` 経由 cvar (= AYAstorm Cinematic Control 系、r30 章で 13 cvar 配線済、memory `project_r30_cinematic_control_tuning_deferred.md`)
  - 既存 OpenGL 経路で `color_saturation` / `color_contrast` 等 uniform 書込 site (= postDeferredTonemap pass、grep verify 要)
- **lifetime**: program lifetime

---

## §5. use site (shader)

- **blueprint file**: `aya_r41_blueprints/set2/per_program_ubo_color_grading.glsl`
  - source extract from `class1/deferred/postDeferredTonemap.glsl:74` ifdef LL_VULKAN_GLSL block (single site)
- **実 shader use site** (= grep 結果):
  - `indra/newview/app_settings/shaders/class1/deferred/postDeferredTonemap.glsl` (= single site、fragment shader、tonemap + color grading pass)

---

## §6. 既存 setter call site (host C++)

- **現状**: **PerProgramUBO_ColorGrading 専用 setter 不在** (= grep 確認済)
- **本実装化後 setter** (= **不明 / verify 要**):
  - `postDeferredTonemap.glsl` 用 color grading parameter 書込 host site (= `LLPipeline::renderTonemap` / `applyColorGrading` 等候補、grep verify 要)
  - AYAstorm Cinematic Control 系 cvar (= r30 章) と直結

---

## §7. 現状通電状態

- **状態**: **untouched** (= Phase 1.E 終了時点)

---

## §8. 本実装化に必要な作業

1. **host C++ setter 配線** = postDeferredTonemap program bind 時に color grading parameter 書込
2. **data source 特定** = `gSavedSettings` cvar 経由 (= AYAstorm Cinematic Control 系、r30 章 13 cvar 流用可能性)
3. **dirty 判定** = cvar 変化時 dirty
4. **shader 側 #else block 撤去** (= Phase 2+ 時)

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | set=2 binding=4 独立 | ✅ 独立配置 |
| OS-3 | std140 padding | ✅ 32B → 256B padded、float/int scalar 連続で罠なし |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既存 GLSL #ifdef 既配置 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**int member 罠**: GLSL `int` is 32-bit signed = std140 align=4 size=4、host 側 int32_t (= boolean は 0/1 値で int 表現) で書込

---

## §10. 不明事項

1. **shell 通電 commit hash**
2. **既存 OpenGL setter call site** = color grading parameter 書込 host site
3. **AYAstorm Cinematic Control 13 cvar との対応関係** = r30 章 13 cvar (= memory `project_r30_cinematic_control_tuning_deferred.md`) と本 UBO 6 member の対応 verify 要
4. **color_grading_lut_enabled int 値域** = boolean 0/1 か他 enum か verify 要

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 4 binding、本 UBO は binding=4

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 本 batch PerProgram UBO 群 8 件

### §11.3 同 shader consume UBO

- `postDeferredTonemap.glsl` 内 = Frame_* + 他 PerProgram (= 同 shader 内 PerProgramUBO_GammaCorrect も consume 可能性、本 batch 並列 = postDeferredGammaCorrect.glsl も blueprint source listed、verify 要)

### §11.4 同 data source UBO

- color grading 系 = `TonemapUBO_Legacy` (set=3 binding=10 cadence=1) と data source 共有可能性 (= 推定)

### §11.5 dirty 連動 UBO

- Cinematic Control cvar 変化時 = TonemapUBO_Legacy + PerProgramUBO_GammaCorrect 等と連動可能性

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ postDeferredTonemap program bind 時 flush
