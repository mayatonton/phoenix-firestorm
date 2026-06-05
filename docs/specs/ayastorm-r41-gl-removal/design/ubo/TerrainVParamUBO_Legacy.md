# TerrainVParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.A PA-8 blueprint 起案済、Phase 1.C PC-2/PC-7δ per-program cadence 一括通電対象に含まれている可能性大 / verify 要)

**本実装化に必要な作業**: shell 2 member (= object_plane_s + object_plane_t) を実 terrain texgen plane data (= LLDrawPoolTerrain / texgen 経由、verify 要) に置換 + dirty 判定 logic 追加 + 実 shader (= class1/deferred/terrainV.glsl) consume 接続検証

---

## §1. UBO identity

- **block_name**: `TerrainVParamUBO_Legacy`
- **block_hash**: `0x4af073b7u`
- **block_size**: 256 B (= std140 32 B、device-padded 256 B)
- **member_count**: 2
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_terrainvparamubo_legacy.inl
struct TerrainVParamUBO_LegacyLayout {
    static constexpr std::uint32_t object_plane_s_OFFSET = 0u;  // size=16 align=16
    static constexpr std::uint32_t object_plane_t_OFFSET = 16u;  // size=16 align=16
};
inline constexpr std::uint32_t TerrainVParamUBO_Legacy_SIZE = 256u; // std140=32, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/terrain_v_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 61) uniform TerrainVParamUBO_Legacy
{
    vec4 object_plane_s;
    vec4 object_plane_t;
};
```

= **blueprint literal extract from `class1/deferred/terrainV.glsl:108` `#ifdef LL_VULKAN_GLSL` block** (= blueprint header line 3 明示)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 61
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: `sAYAStandardLayout`
- **set 3 内訳**: 本 UBO binding=61 = Legacy 帯独立 binding
- **source**: `ubo_metadata.inl:113` `{ "TerrainVParamUBO_Legacy", 0x4af073b7u, 256u, 3u, 61u, 0u, 1u, 2u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` `kCadencePerProgram = 1u`)
- **source**: ubo_metadata.inl:113 + llglslshader.cpp:95

---

## §4. 物理 owner

- **shell 段階 owner**: 未確認
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `object_plane_s` = terrain texgen object plane S (= vec4 plane equation Ax+By+Cz+D=0、LLDrawPoolTerrain texgen 経由、verify 要)
  - `object_plane_t` = terrain texgen object plane T (= 同様 vec4 plane equation)
- **lifetime**: per-program (= terrain rendering shader instance)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/terrain_v_param_ubo_legacy.glsl` (= Phase 1.A PA-8 起案、Source `class1/deferred/terrainV.glsl:108`)
- **実 shader use site**:
  - `class1/deferred/terrainV.glsl` (= blueprint header line 3 literal reference)
- **consume status**: blueprint literal extract source ゆえ既 consume

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter** (= verify 要)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL setter = `uniform4fv("object_plane_s", ...)` / `uniform4fv("object_plane_t", ...)` (= verify 要)
  - data source = LLDrawPoolTerrain texgen object-linear plane equations 計算

---

## §7. 現状通電状態

- **状態**: **shell 通電有無 verify 要**
- **通電内容** (= 推定): zero dummy buffer write + per-program cadence 経路通電

---

## §8. 本実装化に必要な作業

1. **shell 2 member → 実 terrain texgen plane data 接続**:
   - LLDrawPoolTerrain 内 texgen object-linear plane equations (= 古典 OpenGL `glTexGen(GL_OBJECT_PLANE)`) を host 計算
2. **dirty 判定 logic 追加**:
   - dirty 判定 trigger = terrain texture binding 切替 / texgen settings 変更
3. **flush logic 追加**:
   - per-program cadence ゆえ既経路活用
4. **shader 接続検証**:
   - 既存 `class1/deferred/terrainV.glsl` `#ifdef LL_VULKAN_GLSL` block (line 108 周辺) で UBO member access 動作確認

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=3 内に収める | ✅ 維持 (binding=61 独立) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 32 B → 256 B padded (2 × vec4 = 32 B std140) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint = 実 shader literal extract |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**terrain texgen 古典 GL 機能**:
- `object_plane_s` / `object_plane_t` = OpenGL 古典 `glTexGen(GL_OBJECT_PLANE, GL_S/T, plane)` 機能の host 側計算移植
- Vulkan 化で host 側計算明示化、shader 側 UBO member 経由 access
- 既存 OpenGL 経路でも `uniform4fv` 経由で texgen plane 渡している可能性大 (= LL OpenGL は GL3.x core profile ベース、`glTexGen` deprecated 後 host 計算 + uniform 経路、verify 要)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- Asset/Skin 帯 + Legacy 帯各種、本 UBO binding=61 は独立

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- PerProgram cluster 全 88 件

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- `class1/deferred/terrainV.glsl` 内同時 consume UBO (= verify 要):
  - 推定候補: FrameViewProj (= view+proj matrix) + PerProgramUBO_PbrTerrainV (set=2 binding=24、PerProgram、PBR terrain V 系 UBO)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **PerProgramUBO_PbrTerrainV** (= set=2 binding=24、PerProgram、5 member、ubo_metadata.inl:81) = PBR terrain V UBO、本 UBO の PBR 対応版 (= terrain rendering 共通 data source 可能性)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **PerProgramUBO_PbrTerrainV** (= terrain rendering 同時 dirty 高確率、verify 要)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- per-program cadence ゆえ terrainV program bind 時に descriptor set 3 更新

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **shell 通電有無** = handoff doc chain 参照要
2. **object_plane_s/t data source** = LLDrawPoolTerrain texgen 計算経路 (= verify 要)
3. **既存 OpenGL setter call site** = uniform4fv("object_plane_s", ...) grep verify 要
4. **texgen 計算方式** = host 側計算 (= OpenGL 古典 glTexGen 移植) 経路 verify 要
5. **PerProgramUBO_PbrTerrainV との data source 共有** = terrain rendering 共通 data source 確認要
6. **dirty 判定 trigger** = terrain texture binding 切替 / texgen settings 変更 event hook (= verify 要)

= 上記 6 項目は本 UBO file 完成時に grep + Read で逐次解消。
