# RadianceGenFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `radiance_gen_f_param_ubo_legacy.glsl` は実 shader `class1/interface/radianceGenF.glsl ifdef LL_VULKAN_GLSL block` から literal extract 済。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加 + 実 shader binding 接続。**radiance environment map 生成用 shader** (= reflection probe IBL pipeline の一部)。

---

## §1. UBO identity

- **block_name**: `RadianceGenFParamUBO_Legacy`
- **block_hash**: `0xd2fd53efu` (= FNV-1a("RadianceGenFParamUBO_Legacy"))
- **block_size**: 256 B (= std140 32 B、device-padded 256 B)
- **member_count**: 8
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_radiancegenfparamubo_legacy.inl
struct RadianceGenFParamUBO_LegacyLayout {
    static constexpr std::uint32_t sourceIdx_OFFSET = 0u;  // size=4 align=4
    static constexpr std::uint32_t u_width_OFFSET = 4u;  // size=4 align=4
    static constexpr std::uint32_t mipLevel_OFFSET = 8u;  // size=4 align=4
    static constexpr std::uint32_t max_probe_lod_OFFSET = 12u;  // size=4 align=4
    static constexpr std::uint32_t probe_strength_OFFSET = 16u;  // size=4 align=4
    static constexpr std::uint32_t _pad_radiance_gen_f_legacy_0_OFFSET = 20u;  // size=4 align=4
    static constexpr std::uint32_t _pad_radiance_gen_f_legacy_1_OFFSET = 24u;  // size=4 align=4
    static constexpr std::uint32_t _pad_radiance_gen_f_legacy_2_OFFSET = 28u;  // size=4 align=4
};
inline constexpr std::uint32_t RadianceGenFParamUBO_Legacy_SIZE = 256u; // std140=32, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/radiance_gen_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 51) uniform RadianceGenFParamUBO_Legacy
{
    int   sourceIdx;
    int   u_width;
    float mipLevel;
    float max_probe_lod;
    float probe_strength;
    float _pad_radiance_gen_f_legacy_0;
    float _pad_radiance_gen_f_legacy_1;
    float _pad_radiance_gen_f_legacy_2;
};
```

= 実 data 5 member (sourceIdx / u_width / mipLevel / max_probe_lod / probe_strength) + std140 vec4 align 確保用 padding 3 member。

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 51
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、verify 要)
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:96` `{ "RadianceGenFParamUBO_Legacy", 0xd2fd53efu, 256u, 3u, 51u, 0u, 1u, 8u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush、ただし radiance gen は mipLevel 単位で複数 draw call し得る (= 各 mip level dirty かも、verify 要)
- **source**: ubo_metadata.inl:96 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `LLReflectionMapManager` 内 radiance environment map 生成経路 (= reflection probe IBL filtering、grep verify 要)
  - cube map source index / current mip level / probe LOD max / probe intensity 等の per-pass state
- **lifetime**: program 単位 (= radiance gen shader program bind 中のみ、ただし mip level 毎に更新)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/radiance_gen_f_param_ubo_legacy.glsl`
- **実 shader use site**:
  - **`class1/interface/radianceGenF.glsl:42 ifdef LL_VULKAN_GLSL block`** (= blueprint comment literal)
- **使用 uniform**: sourceIdx (int) / u_width (int) / mipLevel (float) / max_probe_lod (float) / probe_strength (float)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **本実装化後 setter** (= **不明 / verify 要**):
  - `LLReflectionMapManager` 内 radiance gen 経路の uniform 書込 site (= grep verify 要)
  - mip level 毎の per-pass uniform update (= verify 要)

---

## §7. 現状通電状態

- **状態**: **untouched**
- **通電 commit**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step 範囲、verify 要)

---

## §8. 本実装化に必要な作業

1. **shell 通電** + dummy buffer write + bind 経路通電
2. **実 member data 流入**: `LLReflectionMapManager` 内 radiance gen 経路 setter 経由 UBO 化
3. **dirty 判定 logic 追加**: PerProgram cadence + mip level 毎 update = PerProgram dirty が複数回連続発火する pattern (= verify 要、PerDraw cadence 相当の挙動になる可能性)
4. **flush logic 追加**: PerProgram cadence flush
5. **shader 接続**: 実 shader 既存 UBO declaration 使用 = 追加 shader 改変なし

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=3 binding=51 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ 32B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**mip level loop dirty risk**: radiance gen は mip chain 全段で同 program を bind し各段で `mipLevel` 値変化 = PerProgram cadence のままだと 1 frame 内に多数回 dirty/flush 発火、PerDraw cadence 化候補 (= 設計再検討要)。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)
- set=3 帯 Legacy 群

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)
- 73 件 (推定) cluster

### §11.3 同 shader consume UBO (= class1/interface/radianceGenF.glsl)
- **不明 / verify 要** = radianceGenF.glsl 内同時 consume UBO (= sampler + FrameViewProj 等可能性、verify 要)

### §11.4 同 data source UBO
- **可能性** (= verify 要): IrradianceGenFParamUBO_Legacy (set=3 binding=52、ubo_metadata.inl:50) = 同 LLReflectionMapManager 経路の sibling、相補的 IBL pipeline、verify 要

### §11.5 dirty 連動 UBO
- **可能性** (= verify 要): ReflectionProbeUBO_Legacy (= 同 reflection probe pipeline)、IrradianceGenFParamUBO_Legacy (= 同 IBL pipeline)

### §11.6 layout 共有関係
- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係
- PerProgram cadence ゆえ program bind 時に同時 bind

---

## §10. 不明事項

1. **既存 OpenGL 経路 setter call site** = `LLReflectionMapManager` 内 radiance gen 経路 (= grep verify 要)
2. **mipLevel update pattern** = mip chain ループで複数回 dirty するか単一値で全段共有か (= verify 要、cadence 設計影響)
3. **sourceIdx 意味** = cube map source index / face index / probe index 等の意味 (= verify 要)
4. **probe_strength owner** = cvar / LLReflectionMapManager state (= verify 要)
5. **IrradianceGenFParamUBO_Legacy との関係** = sibling pipeline か独立 pipeline か (= verify 要)
6. **shell 通電 commit** = 未来作業
7. **PerProgram cadence の妥当性** = mip loop dirty pattern と整合するか再設計検討要

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。
