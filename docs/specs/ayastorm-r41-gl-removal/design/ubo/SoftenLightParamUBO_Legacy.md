# SoftenLightParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.A PA-8 blueprint 起案済、Phase 1.C PC-2/PC-7δ per-program cadence 一括通電対象に含まれている可能性大 / verify 要)

**本実装化に必要な作業**: shell 8 member (= aya_translucency_params / aya_translucency_tint / blur_size / blur_fidelity / ssao_irradiance_scale / ssao_irradiance_max / _pad / ssao_effect_mat) を実 deferred lighting data (= LLEnvironment / debug settings 経由、verify 要) に置換 + dirty 判定 logic 追加 + 実 shader (= class3/deferred/softenLightF.glsl) consume 接続検証

---

## §1. UBO identity

- **block_name**: `SoftenLightParamUBO_Legacy`
- **block_hash**: `0x608809f1u`
- **block_size**: 256 B (= std140 96 B、device-padded 256 B)
- **member_count**: 8
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_softenlightparamubo_legacy.inl
struct SoftenLightParamUBO_LegacyLayout {
    static constexpr std::uint32_t aya_translucency_params_OFFSET = 0u;  // size=16 align=16
    static constexpr std::uint32_t aya_translucency_tint_OFFSET = 16u;  // size=12 align=16
    static constexpr std::uint32_t blur_size_OFFSET = 28u;  // size=4 align=4
    static constexpr std::uint32_t blur_fidelity_OFFSET = 32u;  // size=4 align=4
    static constexpr std::uint32_t ssao_irradiance_scale_OFFSET = 36u;  // size=4 align=4
    static constexpr std::uint32_t ssao_irradiance_max_OFFSET = 40u;  // size=4 align=4
    static constexpr std::uint32_t _pad_soften_light_legacy_0_OFFSET = 44u;  // size=4 align=4
    static constexpr std::uint32_t ssao_effect_mat_OFFSET = 48u;  // size=48 align=16
};
inline constexpr std::uint32_t SoftenLightParamUBO_Legacy_SIZE = 256u; // std140=96, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/soften_light_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 5) uniform SoftenLightParamUBO_Legacy
{
    vec4  aya_translucency_params;
    vec3  aya_translucency_tint;
    float blur_size;
    float blur_fidelity;
    float ssao_irradiance_scale;
    float ssao_irradiance_max;
    float _pad_soften_light_legacy_0;
    mat3  ssao_effect_mat;
};
```

= **blueprint literal extract from `class3/deferred/softenLightF.glsl:57` `#ifdef LL_VULKAN_GLSL` block** (= blueprint header line 3 明示、metadata header note は "9 member" 表記だが codegen layout は 8 member (= mat3 を 1 member counted))

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 5
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: `sAYAStandardLayout`
- **set 3 内訳**: 本 UBO binding=5 = Legacy 帯独立 binding
- **source**: `ubo_metadata.inl:109` `{ "SoftenLightParamUBO_Legacy", 0x608809f1u, 256u, 3u, 5u, 0u, 1u, 8u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` `kCadencePerProgram = 1u`)
- **source**: ubo_metadata.inl:109 + llglslshader.cpp:95

---

## §4. 物理 owner

- **shell 段階 owner**: 未確認
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `aya_translucency_params` = AYAstorm 独自 translucency params (vec4、debug settings 経由)
  - `aya_translucency_tint` = AYAstorm 独自 translucency tint (vec3 RGB)
  - `blur_size` / `blur_fidelity` = SSAO blur params (= LLPipeline / debug settings 由来、verify 要)
  - `ssao_irradiance_scale` / `ssao_irradiance_max` = SSAO irradiance params (= LLEnvironment / SSAO settings 由来)
  - `ssao_effect_mat` = SSAO effect matrix (mat3、48 B std140 padding)
- **lifetime**: per-program (= deferred lighting shader instance)
- **AYAstorm 独自 member 注意**: `aya_translucency_params` / `aya_translucency_tint` は AYAstorm r14+ 視覚表現章独自実装 (= memory `project_ayastorm_visual_realism_chapter` 関連) ゆえ Vulkan port 時に既存 OpenGL 実装と一致確認必須

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/soften_light_param_ubo_legacy.glsl` (= Phase 1.A PA-8 起案、Source `class3/deferred/softenLightF.glsl:57`)
- **実 shader use site**:
  - `class3/deferred/softenLightF.glsl` (= blueprint header line 3 literal reference)
- **consume status**: blueprint literal extract source ゆえ既 consume

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter** (= verify 要)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL setter = `uniform4fv("aya_translucency_params", ...)` / `uniform3fv("aya_translucency_tint", ...)` / `uniform1f("blur_size", ...)` 等 (= verify 要)
  - data source = LLPipeline / LLEnvironment / AYAstorm 独自 cvar (= verify 要)
- **AYAstorm 独自 member ゆえ memory `reference_deferred_shader_routing` + `project_aya_visual_realism_alpha_protect` 参照要**

---

## §7. 現状通電状態

- **状態**: **shell 通電有無 verify 要**
- **通電内容** (= 推定): zero dummy buffer write + per-program cadence 経路通電

---

## §8. 本実装化に必要な作業

1. **shell 8 member → 実 deferred lighting data 接続**:
   - AYAstorm 独自 translucency 系 = AYAstorm 視覚表現章実装と整合確認
   - SSAO 系 = LLPipeline SSAO 計算 host 側で集約
   - blur 系 = post-pass blur params
   - mat3 ssao_effect_mat = SSAO effect 行列計算
2. **dirty 判定 logic 追加**:
   - dirty 判定 trigger = LLEnvironment update / debug settings 変更
3. **flush logic 追加**:
   - per-program cadence ゆえ既経路活用
4. **shader 接続検証**:
   - 既存 `class3/deferred/softenLightF.glsl` `#ifdef LL_VULKAN_GLSL` block (line 57 周辺) で UBO member access 動作確認
   - AYAstorm 独自 translucency 系の AYAstorm View / Cinematic mode 動作 verify

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=3 内に収める | ✅ 維持 (binding=5 独立) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 96 B → 256 B padded (vec4 + vec3+float pack + 4×float + mat3 48 B = 96 B std140) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint = 実 shader literal extract |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**mat3 padding 注意**:
- std140 mat3 = 3 × vec4 = 48 B (= 各 column が vec3 だが vec4 stride で配置)
- layout `ssao_effect_mat_OFFSET = 48u; size=48 align=16` literal 一致

**AYAstorm 独自 member の upstream 取り込み risk**:
- `aya_translucency_params` / `aya_translucency_tint` は AYAstorm r14+ 独自実装
- 設計原則 (1) Upstream OpenGL 取り込みやすさ維持 (= memory `project_ayastorm_r41_design_principles`) との緊張
- Vulkan path 改変は AYAstorm 独自 patch として upstream PR 別管理推奨

**ssao_irradiance_max member_count 表記揺れ**:
- ubo_metadata.inl は member_count=8 (= mat3 を 1 member counted)
- blueprint header note は "9 member" 表記 (= mat3 を 3 vec3 = 3 column counted?、verify 要)
- 実 codegen layout は 8 member、本資料は ubo_metadata.inl 一致で 8 member 採用

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- Asset/Skin 帯 + Legacy 帯各種、本 UBO binding=5 は独立

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- PerProgram cluster 全 88 件

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- `class3/deferred/softenLightF.glsl` 内同時 consume UBO (= verify 要):
  - 推定候補: FrameViewProj / FrameLights / FrameAtmosphere_Lighting / Global_ReflectionProbes / 他 deferred lighting 系 UBO
- memory `reference_deferred_shader_routing` 参照要

### §11.4 同 data source UBO (= 同 host data source から派生)

- **AOUtilParamUBO_Legacy** (= set=3 binding=8、PerProgram) = SSAO util UBO、本 UBO の SSAO 系 member と data source 共有可能性 (= verify 要)
- **GaussianFParamUBO_Legacy** (= set=3 binding=58、PerProgram) = Gaussian blur UBO、blur 系 member と data source 共有可能性 (= verify 要)
- **DeferredUtilParamUBO_Legacy** (= set=3 binding=6、PerProgram) = deferred util UBO、deferred lighting 共通 params 共有可能性

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **不明 / verify 要** = SSAO / deferred lighting params 切替時の連動 UBO 確認要

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- per-program cadence ゆえ softenLightF program bind 時に descriptor set 3 更新

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **shell 通電有無** = handoff doc chain 参照要
2. **AYAstorm 独自 translucency 系 data source** = aya_translucency_params / aya_translucency_tint の host setter 経路 (= verify 要、AYAstorm r14+ 独自実装)
3. **SSAO 系 data source** = LLPipeline SSAO 計算 host 側経路 (= verify 要)
4. **ssao_effect_mat 計算経路** = SSAO effect 行列 host 側計算 (= verify 要)
5. **既存 OpenGL setter call site** = 8 member 各 uniform setter grep verify 要
6. **member_count 表記揺れ** = ubo_metadata.inl (8) vs blueprint header note (9) (= 確定要、本資料は 8 採用)
7. **同 shader consume UBO 完全特定** = class3/deferred/softenLightF.glsl 内同時 consume UBO 群 (= grep verify 要、memory `reference_deferred_shader_routing` 参照)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。
