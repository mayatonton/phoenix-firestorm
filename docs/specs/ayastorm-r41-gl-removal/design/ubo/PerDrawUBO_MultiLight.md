# PerDrawUBO_MultiLight — UBO design (= 実コードベース調査資料)

**通電状態**: **untouched** (= host C++ で `PerDrawUBO_MultiLight` への `writeDrawUbo` / `register*` 呼出ゼロ、grep 確認済 2026-06-06、Phase 1.C bringupTestUBO 系 generic 経路の zero buffer 通電のみ想定 = verify 要)

**本実装化に必要な作業**: 実 light array (= LIGHT_COUNT 1..16 dynamic、`gDeferredMultiLightProgram[i]` 16 件 permutation) を host から writeDrawUbo 経由で書込 + class3/deferred/multiPointLightF.glsl 内 UBO consume へ切替

---

## §1. UBO identity

- **block_name**: `PerDrawUBO_MultiLight`
- **block_hash**: `0x77f0114cu` (= FNV-1a("PerDrawUBO_MultiLight"))
- **block_size**: 768 B (= std140=528 B, device-padded 768 B)
- **member_count**: 6
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perdrawubo_multilight.inl:12-20
struct PerDrawUBO_MultiLightLayout {
    static constexpr std::uint32_t light_OFFSET = 0u;                  // size=256 align=16 stride=16
    static constexpr std::uint32_t light_col_OFFSET = 256u;            // size=256 align=16 stride=16
    static constexpr std::uint32_t far_z_OFFSET = 512u;                // size=4 align=4
    static constexpr std::uint32_t global_light_strength_OFFSET = 516u; // size=4 align=4
    static constexpr std::uint32_t _pad_ml0_OFFSET = 520u;             // size=4 align=4
    static constexpr std::uint32_t _pad_ml1_OFFSET = 524u;             // size=4 align=4
};
inline constexpr std::uint32_t PerDrawUBO_MultiLight_SIZE = 768u; // std140=528, device-padded=768
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_draw_ubo_multi_light.glsl:15-23
layout(std140, set = 2, binding = 1) uniform PerDrawUBO_MultiLight
{
    vec4  light[16];
    vec4  light_col[16];
    float far_z;
    float global_light_strength;
    float _pad_ml0;
    float _pad_ml1;
};
```

= **6 member = vec4[16] (light position+radius) + vec4[16] (light color) + 2 float scalar + 2 pad、blueprint は最大 variant LIGHT_COUNT=16 を canonical 採用**

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 1 (= 他 PerDraw UBO の binding=0 共有とは異なる **独立 binding**)
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC` (= set=2 UBO_DYNAMIC 帯)
- **pipeline layout**: `sAYAStandardLayout`
- **source**: ubo_metadata.inl:68 `{ "PerDrawUBO_MultiLight", 0x77f0114cu, 768u, 2u, 1u, 0u, 2u, 6u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 2
- **意味**: **PerDraw** (= `llglslshader.cpp:96` literal)
- **flush 経路**: `LLVKLoader::writeDrawUbo`
- **source**: llglslshader.cpp:2151-2167

---

## §4. 物理 owner

- **shell 段階**: owner なし
- **本実装化後の data source** (= **verify 要**):
  - `LLPipeline::mNearbyLights` (= 推定、grep verify 要)
  - 既存 OpenGL 経路で `gDeferredMultiLightProgram[i]->uniform4fv(LIGHT_POSITION, ...)` + `uniform4fv(LIGHT_DIFFUSE, ...)` 等で書込む site grep verify 要
- **LIGHT_COUNT permutation**: viewer 側 addPermutation で 1..16 inject (= `llviewershadermgr.cpp:1756` で `gDeferredMultiLightProgram[i]` permutation 16 件、per_draw_ubo_multi_light.glsl:7-8 header literal)
- **host alloc 戦略** = full 16 entry alloc + smaller LIGHT_COUNT shader 派生は trailing entry 未参照 view (= Vulkan std140 layout-compat 慣用、shader 改修ゼロ、set=1 MaterialUBO option I precedent)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_draw_ubo_multi_light.glsl`
  - source extract from `class3/deferred/multiPointLightF.glsl:76` ifdef LL_VULKAN_GLSL block (single site) (= per_draw_ubo_multi_light.glsl:1-11 header literal)
- **実 shader use site** (= grep 結果):
  - `indra/newview/app_settings/shaders/class3/deferred/multiPointLightF.glsl` (= single site)
- = **single shader file consume** (= 他 PerDraw UBO と異なり 1 shader 専用)

---

## §6. 既存 setter call site (host C++)

- **現状**: **PerDrawUBO_MultiLight 専用 setter 不在** (= grep 確認済 2026-06-06、`llvkloader.cpp` + `llglslshader.cpp` 内 hit 0 件)
- **shell 段階通電経路**: 推定 = bringupTestUBO 経由 generic zero buffer 通電
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL 経路で `gDeferredMultiLightProgram[i]` 経由 LIGHT_POSITION + LIGHT_DIFFUSE array uniform を書込む call site grep verify 要

---

## §7. 現状通電状態

- **状態**: **untouched** (= Phase 1.E 終了時点、host C++ で本 UBO 個別配線ゼロ)
- **通電 commit**: なし
- **通電内容**:
  - host C++ 側 `writeDrawUbo(PerDrawUBO_MultiLight, ...)` 呼出 0 件
  - shader 側 `class3/deferred/multiPointLightF.glsl` で `#ifdef LL_VULKAN_GLSL` UBO consume block 既配置
- **shell 通電経路** (= 推定 / verify 要): bringupTestUBO 経路 generic 通電、768 B 大きめ buffer ゆえ ring buffer 経路 sDrawUboRingBufferMgr 経由

---

## §8. 本実装化に必要な作業

1. **host C++ writeDrawUbo 配線** = `gDeferredMultiLightProgram[i]` draw 経路で `LLVKLoader::writeDrawUbo(ubo::block_hash::PerDrawUBO_MultiLight, 0u, mlight_data, 528, dynamic_offset)` 配線追加 (= 528 B std140 size、768 B padded buffer 内 trailing 240 B 未使用)
2. **data source 特定** = 既存 OpenGL 経路 `LLPipeline::mNearbyLights` array 経由 `LIGHT_POSITION` + `LIGHT_DIFFUSE` uniform 書込 site grep verify
3. **dirty 判定** = light list 変化時に dirty (= camera move / light add/remove)、per-draw cadence ゆえ毎 draw 書込み許容
4. **LIGHT_COUNT permutation 対応** = full 16 entry alloc + LIGHT_COUNT < 16 時 trailing entry zero fill (= shader 改修ゼロ pattern)
5. **shader 側 #else block 撤去** (= Phase 2+ 時のみ)

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | set=2 binding=1 独立 binding | ✅ 独立配置、binding 共有なし |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 528B → 768B padded (codegen)、vec4[16] stride=16 で std140 自然 align |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既存 GLSL #ifdef LL_VULKAN_GLSL 既配置 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**size 中規模 risk**:
- 768 B per-draw buffer × per-draw cadence = ring buffer 圧迫可能性、scene 内 multiPoint draw 多発時 verify 要
- max 240 B trailing pad (= LIGHT_COUNT=1 時 light[0] + light_col[0] のみ書込)

**LIGHT_COUNT permutation risk**:
- 16 件 permutation × 同 UBO layout = SPIR-V compile 別、descriptor set layout 単一で OK
- LIGHT_COUNT < 16 時 trailing entry GPU read 未発火確認 (= shader uniform array access `light[i]` で `i < LIGHT_COUNT` guard 要 verify)

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **shell 通電 commit 特定** = bringupTestUBO 経由 generic 通電 commit hash
2. **既存 OpenGL setter call site** = `gDeferredMultiLightProgram[i]` draw 経路 LIGHT_POSITION + LIGHT_DIFFUSE array uniform 書込 host C++ site
3. **data source 上流** = `LLPipeline::mNearbyLights` 等の data source 構造
4. **LIGHT_COUNT permutation 詳細** = `llviewershadermgr.cpp:1756` literal verify 要 (= per_draw_ubo_multi_light.glsl header 既宣言)
5. **`far_z` / `global_light_strength` data source** = 既存 OpenGL 経路で書込まれる uniform 特定 (= grep verify 要)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 4 binding、本 UBO は binding=1 独立

### §11.2 同 cadence cluster UBO (= cadence_tag=2 PerDraw)

- PerDraw cluster 7 UBO (PerDrawUBO_ClipPlane §11.2 参照)
- **PerDrawUBO_MultiLight (本 UBO) = PerDraw cluster の唯一 binding=1 UBO**

### §11.3 同 shader consume UBO

- `class3/deferred/multiPointLightF.glsl` 内同時 consume = PerDrawUBO_LightParams (set=2 binding=0) + Frame_* set=0 帯 + PerProgram set=2 帯多 binding (= verify 要)

### §11.4 同 data source UBO

- Nearby light list 由来 = FrameLights (set=0 binding=1) + PerDrawUBO_LightParams (set=2 binding=0) + PerProgramUBO_PointLightF / SpotLightF と data source 共有可能性 (= 推定、verify 要)

### §11.5 dirty 連動 UBO

- light list 変化時 = FrameLights / PerDrawUBO_LightParams / PerProgramUBO_PointLightF / SpotLightF 全連動可能性

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- per-draw cadence ゆえ毎 draw call で `bindV3aStatic` / `bindV3aRigged` 経由 set=2 帯 4 binding 同時 bind

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.16 同期)

**Layer**: L4-16 (= PerDrawUBO_MultiLight 1 UBO、LIGHT_COUNT permutation)
**status**: **起案済** (= 2026-06-06 C-6-k、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.16` (= single source of truth)
**要点**: 6 member (light vec4[16] + light_col vec4[16] + far_z + global_light_strength + pad ×2)、**768B = PerDraw 中型 size**、setter 不明 [要追加調査]、状態 = **untouched** (writeDrawUbo 0 件)、**LIGHT_COUNT permutation 16 件** (= `gDeferredMultiLightProgram[i]`、`llviewershadermgr.cpp:1756`)、host alloc 戦略 = full 16 entry alloc + smaller LIGHT_COUNT trailing 未参照 view (= shader 改修ゼロ、set=1 MaterialUBO option I precedent)、multiPointLightF.glsl:76 singleton site、set=2 binding=1 独立 (= 他 PerDraw UBO binding=0 共有とは独立)、cross-UBO 連動 (= §3.5.15 LightParams + §3.5.2 sub-cluster (c) light cvar + FrameLights data source 共有候補) [要 verify D4 突合]、cadence PerDraw 維持、工数 group 全体 S-M 内
**関連**: L0-1 dispatch (= 衝突なし binding=1 独立) / §3.5.16 本 group / §3.5.15 LightParams (= 同 deferred lighting pipeline) / §3.5.2 sub-cluster (c) PointLightF/SpotLightF/PointLightV (= AYAstorm light cvar 連動) / FrameLights (= light array data source 共有候補)
