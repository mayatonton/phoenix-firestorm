# ShadowUtilParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `shadow_util_param_ubo_legacy.glsl` は実 shader `class1/deferred/shadowUtil.glsl ifdef LL_VULKAN_GLSL block` から literal extract 済 (= multi-site: cinematic_bd/class1/deferred/shadowUtil.glsl にも同 layout 確認、blueprint comment literal)。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加。**最大 size Legacy UBO (= 512 B = 448 B std140 → 512 B padded)** = shadow_matrix[6] mat4 array が支配。

---

## §1. UBO identity

- **block_name**: `ShadowUtilParamUBO_Legacy`
- **block_hash**: `0x1c7a416cu` (= FNV-1a("ShadowUtilParamUBO_Legacy"))
- **block_size**: 512 B (= std140 448 B、device-padded 512 B)
- **member_count**: 12
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_shadowutilparamubo_legacy.inl
struct ShadowUtilParamUBO_LegacyLayout {
    static constexpr std::uint32_t shadow_matrix_OFFSET = 0u;  // size=384 align=16 stride=64
    static constexpr std::uint32_t shadow_clip_OFFSET = 384u;  // size=16 align=16
    static constexpr std::uint32_t shadow_res_OFFSET = 400u;  // size=8 align=8
    static constexpr std::uint32_t proj_shadow_res_OFFSET = 408u;  // size=8 align=8
    static constexpr std::uint32_t shadow_bias_OFFSET = 416u;  // size=4 align=4
    static constexpr std::uint32_t shadow_offset_OFFSET = 420u;  // size=4 align=4
    static constexpr std::uint32_t shadow_softness_OFFSET = 424u;  // size=4 align=4
    static constexpr std::uint32_t spot_shadow_bias_OFFSET = 428u;  // size=4 align=4
    static constexpr std::uint32_t spot_shadow_offset_OFFSET = 432u;  // size=4 align=4
    static constexpr std::uint32_t _pad_shadow_util_legacy_0_OFFSET = 436u;  // size=4 align=4
    static constexpr std::uint32_t _pad_shadow_util_legacy_1_OFFSET = 440u;  // size=4 align=4
    static constexpr std::uint32_t _pad_shadow_util_legacy_2_OFFSET = 444u;  // size=4 align=4
};
inline constexpr std::uint32_t ShadowUtilParamUBO_Legacy_SIZE = 512u; // std140=448, device-padded=512
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/shadow_util_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 7) uniform ShadowUtilParamUBO_Legacy
{
    mat4  shadow_matrix[6];          // 0-383
    vec4  shadow_clip;               // 384-399
    vec2  shadow_res;                // 400-407
    vec2  proj_shadow_res;           // 408-415
    float shadow_bias;               // 416-419
    float shadow_offset;             // 420-423
    float shadow_softness;           // 424-427
    float spot_shadow_bias;          // 428-431
    float spot_shadow_offset;        // 432-435
    float _pad_shadow_util_legacy_0; // 436-439
    float _pad_shadow_util_legacy_1; // 440-443
    float _pad_shadow_util_legacy_2; // 444-447
};
```

= 実 data 9 member (shadow_matrix[6] + shadow_clip + shadow_res + proj_shadow_res + shadow_bias/offset/softness + spot_shadow_bias/offset) + std140 padding 3 member。**最重要 shadow rendering UBO** (= sun shadow 4 cascade + spot shadow 2 = mat4[6] palette + bias/offset/softness 全 shadow filtering parameter 集約)。

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 7
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、verify 要)
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:102` `{ "ShadowUtilParamUBO_Legacy", 0x1c7a416cu, 512u, 3u, 7u, 0u, 1u, 12u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush、shadow_matrix は frame 単位更新 + shadow_bias は cvar 変更時 update
- **source**: ubo_metadata.inl:102 + llglslshader.cpp:95 literal
- **注記**: cadence 妥当性 question = shadow_matrix は frame 単位で 1 値、全 shadow consume program 共通 → PerFrame cadence 候補だが現状 PerProgram、Phase 2 設計再検討余地有り得る

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `LLPipeline::generateSunShadow` 内 shadow_matrix 計算経路 (= grep verify 要)
  - shadow_bias / shadow_offset / shadow_softness 等は cvar 由来 (= RenderShadowBias / RenderShadowOffset / RenderShadowSoftness 等、grep verify 要)
  - shadow_res / proj_shadow_res = shadow map resolution (= cvar or pipeline state)
- **lifetime**: shadow rendering 全 program 共有 (= 同 frame 内 sun + spot shadow consume 多数 program で同値)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/shadow_util_param_ubo_legacy.glsl`
- **実 shader use site**:
  - **`class1/deferred/shadowUtil.glsl:80 ifdef LL_VULKAN_GLSL block`** (= primary、blueprint comment literal)
  - **`cinematic_bd/class1/deferred/shadowUtil.glsl:105`** (= multi-site identical、blueprint comment literal: `Multi-site: verified identical at cinematic_bd/class1/deferred/shadowUtil.glsl:105`)
  - shadowUtil.glsl は **shared include** = sun shadow / spot shadow consume する全 shader (= softenLightF / spotLightF / pointLightF / その他 deferred lighting 系) で include される共通 header
- **使用 uniform**: shadow_matrix[6] (mat4 × 6 = 4 sun cascade + 2 spot) / shadow_clip (vec4 = 4 cascade clip planes) / shadow_res (vec2) / proj_shadow_res (vec2) / shadow_bias / shadow_offset / shadow_softness / spot_shadow_bias / spot_shadow_offset

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **本実装化後 setter** (= **不明 / verify 要**):
  - `LLPipeline` 内 shadow setter (= `uniformMatrix4fv(shadow_matrix, 6, ...)` + `uniform4fv(shadow_clip)` + `uniform2fv(shadow_res)` + 各 float uniform、grep verify 要)
  - shadow consume program bind 直前に 9 値 setter 全 fire (= verify 要)

---

## §7. 現状通電状態

- **状態**: **untouched**
- **通電 commit**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step 範囲、verify 要)

---

## §8. 本実装化に必要な作業

1. **shell 通電** + dummy buffer write + bind 経路通電 (= 512 B = 256 B × 2 slot 消費、ring buffer align verify 要)
2. **実 member data 流入**: LLPipeline shadow setter (= 9 個 setter) を UBO 化、shadow_matrix[6] は連続 mat4 array
3. **dirty 判定 logic 追加**: PerProgram cadence + shadow rendering 開始時 dirty + cvar 変更時 dirty
4. **flush logic 追加**: PerProgram cadence flush
5. **shader 接続**: 実 shader 既存 UBO declaration 使用 = 追加 shader 改変なし
6. **cadence 設計再検討余地**: PerFrame cadence 化候補 (= 同 frame 内全 shadow consume program 共有値、program 単位 dirty 不要)、ただし layout 不変契約遵守

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=3 binding=7 配置 | ✅ 維持 |
| OS-3 | std140 padding + mat4[6] stride=64 厳守 | ✅ 448B → 512B padded、stride=64 codegen 確定 |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 (= 512B align 影響、256B align 境界跨ぐ) |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader (= 2 site 確認済) |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**大物 UBO (512B) ring buffer risk**: 256B 倍数 padded、UBO ring buffer 経路で 1 entry 2 slot 消費 (= 256B × 2 = 512B)、align verify 要。

**multi-site identical risk**: cinematic_bd/class1/deferred/shadowUtil.glsl と class1/deferred/shadowUtil.glsl 2 site で identical 確認済 (= blueprint comment literal)、本 UBO の layout 変更時は 2 site 同期更新必須。

**shadow_matrix 順序 risk**: mat4[6] の順序 = `sun cascade 0-3 + spot 0-1` か `spot 0-1 + sun 0-3` 等の順序、host 書込と shader access index が整合必要 = verify 要。

**shadow_clip 解釈**: vec4 = 4 cascade clip plane z 値 (= cascade 0-3) と推定、verify 要。

**memory `project_bd_biaserror_pitfall` 注記**: BD RenderShadowBiasError +0.1 は BD-X1 doctrine 単独例外。本 UBO の shadow_bias は BD parity 影響有り得る、Phase 2 で実機 verify 必須 (= cascade z≈30m で zero-crossing fringe 既知)。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)
- set=3 帯 Legacy 群

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)
- 73 件 (推定) cluster

### §11.3 同 shader consume UBO (= shadowUtil.glsl shared include)
- **不明 / verify 要** = shadowUtil.glsl を include する全 shader = softenLightF / spotLightF / pointLightF / deferred lighting 系多数 (= grep verify 要)
- 同 shader 内 同時 consume UBO (= FrameViewProj / FrameLights / 他 shadow sampler 等)

### §11.4 同 data source UBO
- **可能性** (= verify 要): FrameLights (= 同 lighting state) / 他 deferred lighting 系 UBO

### §11.5 dirty 連動 UBO
- **可能性** (= verify 要): frame 開始時 shadow update → 同時 dirty UBO (= FrameLights 等)

### §11.6 layout 共有関係
- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係
- PerProgram cadence ゆえ program bind 時に同時 bind
- shadow consume 全 program で同 UBO bind 共有 = 同 frame 内多回 bind (= 値不変ゆえ実質 1 値共有)

---

## §10. 不明事項

1. **既存 OpenGL 経路 setter call site** = LLPipeline shadow setter 9 個 (= grep verify 要)
2. **shadow_matrix[6] order** = sun 4 cascade + spot 2 の順序 (= verify 要)
3. **shadow_clip vec4 解釈** = 4 cascade clip z 値 (= verify 要)
4. **shadow_res / proj_shadow_res 区別** = shadow map resolution の 2 種、sun cascade と spot で別 (= verify 要)
5. **shadow_bias / spot_shadow_bias 区別** = sun と spot で別 bias、cvar 由来 verify 要
6. **cinematic_bd multi-site 同期更新 protocol** = blueprint comment 主張、独立 verify 要
7. **cadence 設計再検討** = PerProgram vs PerFrame、Phase 2 で再評価余地

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。
