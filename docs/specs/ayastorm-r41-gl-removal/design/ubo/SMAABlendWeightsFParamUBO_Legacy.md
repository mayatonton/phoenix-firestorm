# SMAABlendWeightsFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `smaa_blend_weights_f_param_ubo_legacy.glsl` は実 shader `class1/deferred/SMAABlendWeightsF.glsl ifdef LL_VULKAN_GLSL block` から literal extract 済。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加。**SMAA (Subpixel Morphological Antialiasing) blend weights pass** 専用 UBO。

---

## §1. UBO identity

- **block_name**: `SMAABlendWeightsFParamUBO_Legacy`
- **block_hash**: `0x4506bc9eu` (= FNV-1a("SMAABlendWeightsFParamUBO_Legacy"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_smaablendweightsfparamubo_legacy.inl
struct SMAABlendWeightsFParamUBO_LegacyLayout {
    static constexpr std::uint32_t subsampleIndices_OFFSET = 0u;  // size=16 align=16
};
inline constexpr std::uint32_t SMAABlendWeightsFParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/smaa_blend_weights_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 62) uniform SMAABlendWeightsFParamUBO_Legacy
{
    vec4 subsampleIndices;
};
```

= 1 member vec4 `subsampleIndices` (= SMAA 2x / 4x subsample blending 用 index 4 component)。

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 62
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、verify 要)
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:99` `{ "SMAABlendWeightsFParamUBO_Legacy", 0x4506bc9eu, 256u, 3u, 62u, 0u, 1u, 1u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush、SMAA pass 単位で固定値 (= temporal SMAA で subsample 値変化想定)
- **source**: ubo_metadata.inl:99 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - SMAA post-process pipeline (= `LLPipeline::renderPostProcess` 内 SMAA 経路、grep verify 要)
  - SMAA temporal 用 subsample index (= 2x SMAA で 2 frame jitter、4x で 4 frame、verify 要)
- **lifetime**: program 単位 (= SMAA blend weights shader bind 中)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/smaa_blend_weights_f_param_ubo_legacy.glsl`
- **実 shader use site**:
  - **`class1/deferred/SMAABlendWeightsF.glsl:67 ifdef LL_VULKAN_GLSL block`** (= blueprint comment literal)
- **使用 uniform**: subsampleIndices (vec4)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **本実装化後 setter** (= **不明 / verify 要**):
  - SMAA pass setter (= `uniform4fv(subsampleIndices, ...)` 呼出 site、grep verify 要)

---

## §7. 現状通電状態

- **状態**: **untouched**
- **通電 commit**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step 範囲、verify 要)

---

## §8. 本実装化に必要な作業

1. **shell 通電** + dummy buffer write + bind 経路通電
2. **実 member data 流入**: 既存 OpenGL 経路 setter (= grep verify 要) を UBO 化
3. **dirty 判定 logic 追加**: PerProgram cadence + temporal SMAA 有効時は frame 毎 dirty 可能性
4. **flush logic 追加**: PerProgram cadence flush
5. **shader 接続**: 実 shader 既存 UBO declaration 使用 = 追加 shader 改変なし

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=3 binding=62 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**temporal SMAA dirty pattern risk**: SMAA 2x/4x で subsample index が frame 毎更新 = PerProgram cadence のまま 1 frame 内 dirty 1 回 (= same program 内では 1 値) で対応可能だが、temporal 経路の verify 必要。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)
- set=3 帯 Legacy 群

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)
- 73 件 (推定) cluster

### §11.3 同 shader consume UBO (= class1/deferred/SMAABlendWeightsF.glsl)
- **不明 / verify 要** = SMAABlendWeightsF.glsl 内同時 consume UBO (= SMAAParamUBO_Legacy 等可能性、verify 要)

### §11.4 同 data source UBO
- **可能性** (= verify 要): **SMAAParamUBO_Legacy** (set=3 binding=14、ubo_metadata.inl:100) = 同 SMAA pipeline の sibling UBO、SMAA pass chain (edge detection → blend weights → neighborhood blending) の 1 pass 用

### §11.5 dirty 連動 UBO
- **可能性** (= verify 要): SMAAParamUBO_Legacy (= 同 SMAA pipeline 由来)

### §11.6 layout 共有関係
- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係
- PerProgram cadence ゆえ program bind 時に同時 bind

---

## §10. 不明事項

1. **既存 OpenGL 経路 setter call site** = SMAA pass setter (= grep verify 要)
2. **subsampleIndices の owner** = SMAA mode (1x/2x/4x) + temporal frame index (= verify 要)
3. **temporal SMAA enable 経路** = AYAstorm 側で temporal SMAA on/off cvar 等 (= verify 要)
4. **SMAAParamUBO_Legacy との data source 共有** = 同 pipeline 由来 verify 要
5. **shell 通電 commit** = 未来作業
6. **SMAA pass chain 全 UBO 一覧** = edge detect / blend weights / neighborhood blending 各 pass の UBO 配置 verify 要
7. **PerProgram flush 経路の VkDescriptorBufferInfo bind 詳細** = verify 要

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.12 同期)

**Layer**: L4-12 sub-cluster (b) (= blend weights 1 UBO、temporal SMAA subsample index)
**status**: **起案済** (= 2026-06-06 C-6-g、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.12` (= single source of truth)
**要点**: 1 member (subsampleIndices vec4)、SMAA 2x/4x subsample blending 用 index、SMAABlendWeightsF.glsl:67 singleton site、setter 不明 [要追加調査]、temporal SMAA frame swap trigger で per-frame dirty pattern (= PerProgram cadence で frame 内 1 値、stale risk なし想定) [要 verify D3]、temporal SMAA enable 経路 (= AYAstorm temporal SMAA cvar 候補) [要追加調査]、subsampleIndices owner = SMAA mode (1x/2x/4x) + temporal frame index [要 verify]、cadence PerProgram 維持、工数 group 全体 S-M 内
**関連**: L0-1 dispatch (= 衝突なし binding=62) / §3.5.12 sub-cluster (a) SMAAParam (= SMAA pipeline sibling、shared include 経由共通 consume)
