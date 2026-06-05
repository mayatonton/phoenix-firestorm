# ReflectionProbeUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `reflection_probe_ubo_legacy.glsl` は実 shader `class3/deferred/reflectionProbeF.glsl ifdef LL_VULKAN_GLSL block` から literal extract 済。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加。**Global_ReflectionProbes (set=0 binding=3 SINGLETON) との関係要整理** (= 同 data source 候補だが cadence/set 異なる、§11.4 参照)。

---

## §1. UBO identity

- **block_name**: `ReflectionProbeUBO_Legacy`
- **block_hash**: `0x37e365f8u` (= FNV-1a("ReflectionProbeUBO_Legacy"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 2
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_reflectionprobeubo_legacy.inl
struct ReflectionProbeUBO_LegacyLayout {
    static constexpr std::uint32_t max_probe_lod_OFFSET = 0u;  // size=4 align=4
    static constexpr std::uint32_t transparent_surface_OFFSET = 4u;  // size=4 align=4
};
inline constexpr std::uint32_t ReflectionProbeUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/reflection_probe_ubo_legacy.glsl
layout(std140, set = 3, binding = 17) uniform ReflectionProbeUBO_Legacy
{
    float max_probe_lod;
    bool transparent_surface;
};
```

= 実 data 2 member (= max_probe_lod / transparent_surface)。**最小 size UBO** (= 8 B std140、256 B padded)。

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 17
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、verify 要)
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:97` `{ "ReflectionProbeUBO_Legacy", 0x37e365f8u, 256u, 3u, 17u, 0u, 1u, 2u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush、program 単位で値を保持
- **source**: ubo_metadata.inl:97 + llglslshader.cpp:95 literal
- **注記**: **Global_ReflectionProbes (cadence=5 SINGLETON) と異なる cadence** = 同 reflection probe data でも consume pattern が違う = Global_ReflectionProbes は process-wide singleton (frame 内多 update)、本 UBO は program 単位 (= reflection probe consume shader bind 時 1 回 update)

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `LLReflectionMapManager` 経由の現在 reflection probe state (= max probe LOD + 透過面判定)
  - `LLPipeline::renderDeferredLighting` 経路の per-pass parameter
- **lifetime**: program 単位 (= reflection probe consume shader program bind 中)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/reflection_probe_ubo_legacy.glsl`
- **実 shader use site**:
  - **`class3/deferred/reflectionProbeF.glsl:80 ifdef LL_VULKAN_GLSL block`** (= blueprint comment literal)
- **使用 uniform**: max_probe_lod (float) / transparent_surface (bool)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **本実装化後 setter** (= **不明 / verify 要**):
  - `LLReflectionMapManager` 内 reflection probe consume 経路 setter (= `uniform1f(max_probe_lod)` + `uniform1i(transparent_surface)`、grep verify 要)
  - 31 setter 経路で UBO 化対応要

---

## §7. 現状通電状態

- **状態**: **untouched**
- **通電 commit**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step 範囲、verify 要)

---

## §8. 本実装化に必要な作業

1. **shell 通電** + dummy buffer write + bind 経路通電
2. **実 member data 流入**: 既存 OpenGL 経路 setter (= reflection probe consume 経路、grep verify 要) を UBO 化
3. **dirty 判定 logic 追加**: PerProgram cadence
4. **flush logic 追加**: PerProgram cadence flush
5. **shader 接続**: 実 shader 既存 UBO declaration 使用 = 追加 shader 改変なし
6. **Global_ReflectionProbes との関係整理** (= §11.4 参照): 同 data source なら 1 UBO に統合可能性検討、ただし cadence 違い (PerProgram vs SINGLETON) で別 UBO 設計理由が有り得る = verify 要

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=3 binding=17 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**bool member std140 risk**: `bool transparent_surface` は std140 で 4 B (uint 相当) ゆえ codegen layout offset=4 size=4 と整合、shader 側 bool 解釈は VkBool32 (= 4 B 0/1) 想定要、verify 要。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)
- set=3 帯 Legacy 群

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)
- 73 件 (推定) cluster

### §11.3 同 shader consume UBO (= class3/deferred/reflectionProbeF.glsl)
- **不明 / verify 要** = reflectionProbeF.glsl 内同時 consume UBO (= FrameViewProj / FrameLights / FrameAtmosphere_Lighting / Global_ReflectionProbes 等可能性、verify 要)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **Global_ReflectionProbes** (set=0 binding=3 cadence=5 SINGLETON、ubo_metadata.inl:45) = 同 LLReflectionMapManager 由来候補、cadence/set 異なる:
  - Global_ReflectionProbes = process-wide singleton (= 全 program 共有、frame 内多 update OK)
  - 本 UBO ReflectionProbeUBO_Legacy = program 単位 (= reflectionProbeF program bind 時のみ)
  - 統合可能性 + 別 UBO 維持理由 = **verify 要** (= Phase 2 設計で再評価)
- **RadianceGenFParamUBO_Legacy** (set=3 binding=51) + **IrradianceGenFParamUBO_Legacy** (set=3 binding=52) = 同 IBL pipeline の sibling、`max_probe_lod` 名共有 (= RadianceGen にも同名 member)、verify 要

### §11.5 dirty 連動 UBO
- **可能性** (= verify 要): Global_ReflectionProbes (= 同 reflection probe data 由来ゆえ連動可能性大)

### §11.6 layout 共有関係
- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係
- PerProgram cadence ゆえ program bind 時に同時 bind

---

## §10. 不明事項

1. **既存 OpenGL 経路 setter call site** = `LLReflectionMapManager` 内 reflection probe consume 経路 (= grep verify 要)
2. **`max_probe_lod` の owner** = LLReflectionMapManager state / cvar 直接読出 (= verify 要)
3. **`transparent_surface` 判定 logic** = pass 種類 (opaque/blend) 判定 (= verify 要)
4. **Global_ReflectionProbes との data source 共有** = 同一 manager 由来か独立か (= verify 要、Phase 2 設計影響)
5. **RadianceGen / IrradianceGen との `max_probe_lod` 共有** = 同値か別値か (= verify 要)
6. **shell 通電 commit** = 未来作業
7. **bool transparent_surface の std140 4 B 解釈** = VkBool32 整合 verify 要

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。
