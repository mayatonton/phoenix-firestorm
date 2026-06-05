# PerProgramUBO_PbrTerrainV — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `per_program_ubo_pbr_terrain_v.glsl` は実 shader `class1/deferred/pbrterrainV.glsl:79 ifdef LL_VULKAN_GLSL block` から literal extract 済。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加 + 実 shader binding 接続。

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_PbrTerrainV`
- **block_hash**: `0xa440d64au` (= FNV-1a("PerProgramUBO_PbrTerrainV"))
- **block_size**: 256 B (= std140 96 B、device-padded 256 B)
- **member_count**: 5 (= 1 active + 1 active + 3 pad)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_pbrterrainv.inl
struct PerProgramUBO_PbrTerrainVLayout {
    static constexpr std::uint32_t terrain_texture_transforms_OFFSET = 0u;  // size=80 align=16 stride=16
    static constexpr std::uint32_t region_scale_OFFSET = 80u;  // size=4 align=4
    static constexpr std::uint32_t _pad0_OFFSET = 84u;  // size=4 align=4
    static constexpr std::uint32_t _pad1_OFFSET = 88u;  // size=4 align=4
    static constexpr std::uint32_t _pad2_OFFSET = 92u;  // size=4 align=4
};
inline constexpr std::uint32_t PerProgramUBO_PbrTerrainV_SIZE = 256u; // std140=96, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_pbr_terrain_v.glsl
layout(std140, set = 2, binding = 24) uniform PerProgramUBO_PbrTerrainV
{
    vec4  terrain_texture_transforms[5];
    float region_scale;
    float _pad0;
    float _pad1;
    float _pad2;
};
```

= active member 2 個 (= `terrain_texture_transforms[5]` 80 B + `region_scale` 4 B) + tail pad 12 B。blueprint comment literal: `Source: literal extract from class1/deferred/pbrterrainV.glsl:79 ifdef LL_VULKAN_GLSL block (single site)` + `(region_scale は heightmap 側 declared-but-unused 容認)`。

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 24
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通)
- **pipeline layout**: `sAYAStandardLayout`
- **set 2 配置**: PerDraw + PerProgram 帯 (= INDEX.md §B.4 set mapping)
- **source**: `ubo_metadata.inl:81` `{ "PerProgramUBO_PbrTerrainV", 0xa440d64au, 256u, 2u, 24u, 0u, 1u, 5u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush、program 単位で値を保持。Terrain は region 単位で UV transform / region_scale が変化、program 単位で flush して描画
- **source**: ubo_metadata.inl:81 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source**:
  - `LLDrawPoolTerrain` (= `lldrawpoolterrain.cpp:558` `shader->uniform4fv(LLShaderMgr::TERRAIN_TEXTURE_TRANSFORMS, transform_vec4_count, (F32*)transforms_packed)`)
  - `LLViewerRegion` (= `lldrawpoolterrain.cpp:586` `shader->uniform1f(LLShaderMgr::REGION_SCALE, regionp->getWidth())`)
- **lifetime**: region bind 単位 (= terrain pool で region 切替時に dirty)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_pbr_terrain_v.glsl`
- **実 shader use site**: **`class1/deferred/pbrterrainV.glsl:79 ifdef LL_VULKAN_GLSL block`** (= single site)
  - 既存 UBO block (pbrterrainV.glsl:80-81):
    ```glsl
    vec4  terrain_texture_transforms[5];  // offset 0 (vec4 array stride 16 × 5 = 80 bytes)
    float region_scale;                   // offset 80
    ```
  - 既存 OpenGL `#else` block (pbrterrainV.glsl:89, 201):
    ```glsl
    uniform float region_scale;
    uniform vec4[5] terrain_texture_transforms;
    ```
  - 使用箇所多数: `pbrterrainV.glsl:224-317` (terrain_texture_transforms[0..4] 各成分の TTT 取出) + `:328` (`vary_texcoord = position.xy / region_scale`)
  - comment literal `pbrterrainV.glsl:200-201`: `Vulkan 側は PerProgramUBO_PbrTerrainV (binding 24、region_scale と統合) に移動済`

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **既存 OpenGL 経路 setter**:
  - `llshadermgr.cpp:1533` `mReservedUniforms.push_back("terrain_texture_transforms")` + `:1535` `llassert(mReservedUniforms.size() == LLShaderMgr::TERRAIN_TEXTURE_TRANSFORMS +1)`
  - `llshadermgr.cpp:1812` `mReservedUniforms.push_back("region_scale")`
  - `llshadermgr.h:71` `TERRAIN_TEXTURE_TRANSFORMS, //  "terrain_texture_transforms" (GLTF)`
  - `llshadermgr.h:325` `REGION_SCALE, //  "region_scale" (GLTF)`
  - 実 setter: **`lldrawpoolterrain.cpp:558`** `shader->uniform4fv(LLShaderMgr::TERRAIN_TEXTURE_TRANSFORMS, transform_vec4_count, (F32*)transforms_packed)` + **`:586`** `shader->uniform1f(LLShaderMgr::REGION_SCALE, regionp->getWidth())`
- **本実装化後 setter**:
  - 31 setter 経路 (= mUseUBO 分岐) で UBO 化対応要 (= `forwardToUboUpload` redirect)
  - `uniform4fv count=5` で 80 B memcpy + `uniform1f` で 4 B memcpy (offset 80)

---

## §7. 現状通電状態

- **状態**: **untouched**
- **通電 commit**: なし
- **通電内容**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step 範囲、verify 要)

---

## §8. 本実装化に必要な作業

1. **shell 通電** (= Phase 2 着手時):
   - blueprint ベースで host 側 buffer 配置 + descriptor set 配線
2. **実 member data 流入**:
   - `lldrawpoolterrain.cpp:558 / :586` の `uniform4fv` / `uniform1f` 呼出 site を UBO 化
3. **dirty 判定 logic 追加**:
   - PerProgram cadence + terrain region 切替時 dirty (= region change trigger)
4. **flush logic 追加**:
   - PerProgram cadence flush (= `writeProgramUbo` 経路、`llvkloader.cpp:5503`)
5. **shader 接続**:
   - 実 shader `class1/deferred/pbrterrainV.glsl:79 ifdef LL_VULKAN_GLSL block` で UBO declaration 既存 = 追加 shader 改変なし (= OS-5 充足)
   - 既存 OpenGL `#else` block (`:89, :201`) uniform 個別宣言は温存
6. **tail pad 維持**:
   - `_pad0/1/2` (12 B) は std140 padding (96 B std140 sum)、layout 不変契約で維持

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=2 binding=24 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 96B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**`region_scale` heightmap 側 declared-but-unused 容認**: blueprint comment literal `region_scale は heightmap 側 declared-but-unused 容認`。pbrterrainV.glsl は使用しているが、heightmap 系 shader 側で declared-but-unused 状況、UBO size 不変の制約上維持必須。

**terrain pool 切替 timing**: region 切替が描画中 (= program bind 中) に起こり得る場合、PerProgram cadence では捉えきれない可能性 (= verify 要、Phase 2 で per-draw cadence 移行検討)。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 = PerDraw + PerProgram 混在

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- ubo_metadata.inl で 73 件 (推定) の最大 cluster

### §11.3 同 shader consume UBO (= class1/deferred/pbrterrainV.glsl)

- **不明 / verify 要** = pbrterrainV.glsl 内で他に consume される UBO (= FrameViewProj / FrameAtmosphere_Lighting / TerrainVParamUBO_Legacy 等、grep verify 要)

### §11.4 同 data source UBO (= 同 host data source から派生)

- `TerrainVParamUBO_Legacy` (= `ubo_metadata.inl:113` set=3 binding=61) は同 terrain 系 (= 非 PBR terrain) の可能性、verify 要

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- region 切替時、関連 region 系 UBO (= AtmoExtra / Sky 系) と連動の可能性

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ program bind 時に同時 bind

---

## §10. 不明事項

1. **terrain pool dirty trigger 詳細** = region 切替時 + heightmap update 時の dirty flag set 経路 (= verify 要)
2. **`region_scale` heightmap 側 declared-but-unused 状況** = heightmap 系 shader で region_scale が宣言されながら未使用な理由 (= blueprint comment literal、verify 要)
3. **PerProgram cadence の region 切替対応** = region 切替が program rebind を伴うか、独立 dirty が必要か (= verify 要)
4. **`TerrainVParamUBO_Legacy` との関係** = 別 UBO で同 terrain data を duplicate 持つか、PBR/legacy 切替で別 program 使用か (= verify 要)
5. **shell 通電 commit** = 未来作業
6. **PerProgram flush 経路の VkDescriptorBufferInfo bind 詳細** = `writeProgramUbo` 経路実装詳細
7. **`transform_vec4_count` の上限** = `lldrawpoolterrain.cpp:558` 第 2 引数で渡される count が **必ず 5 か** (= layout 80 B 確定の前提)、それとも variable で UBO size 不整合になり得るか

= 上記 7 項目は本 UBO file 完成時に逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.3.1 同期)

**Layer**: L2-1 (= B Tier α setter 完全特定済、PerProgram cadence、terrain visual 容易判定)
**status**: **起案済** (= 2026-06-06 C-4、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.3.1` (= single source of truth)

**sub-work 7 dim 要点**:
- **(1) 前提条件**: L0-1 dispatch + L0-4 cadence
- **(2) 不明事項**: terrain pool dirty trigger [要追加調査] / region_scale heightmap declared-but-unused [要追加調査] / PerProgram cadence の region 切替対応 [要追加調査] / `TerrainVParamUBO_Legacy` 関係 [要追加調査] / `transform_vec4_count` 上限 = 5 固定か [要 verify]
- **(3) 調査手法**: D1 (`lldrawpoolterrain.cpp:558/586` redirect 後動作) + D3 (region 切替 trigger) + D4 (TerrainVParamUBO_Legacy data source 関係)
- **(4) 設計 task**: PerProgram cadence triple-buffer (= terrain pool active 時) / `lldrawpoolterrain.cpp:558/586` 2 setter call を `forwardToUboUpload` → `writeProgramUbo` (= 80 B + 4 B memcpy) / terrain pool bind 単位 flush (= region 切替時) / `pbrterrainV.glsl:79` 既存 LL_VULKAN_GLSL block 活性化
- **(5) 工程**: trace 順 L2 1 件目 (= setter 完全特定済で確実)、工数 **S-M** (= 半日)、L2-2 / L2-3 / L2-4 並列可
- **(6) A 確定**: mUseUBO ON + shader 活性化 + 2 setter 通電 + AYA live verify (= PBR terrain region texture transform / region_scale 描画既存と同一、**visual regression ゼロ §5.4**) + Vulkan validation 0 + transform_vec4_count = 5 固定 verify
- **(7) 4 原則 gate**: 全 ✅、原則 4 = `pbrterrainV.glsl #else` block uniform 個別宣言維持

**関連**: L0-1 + L0-4 (= WORK_ORDER §2) / §5.4 visual regression policy / `TerrainVParamUBO_Legacy` (= 同 terrain 系候補、L3 内)
