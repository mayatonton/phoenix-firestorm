# PreviewVParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `preview_v_param_ubo_legacy.glsl` は実 shader `class1/objects/previewV.glsl ifdef LL_VULKAN_GLSL block` から literal extract 済。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加 + 実 shader binding 接続。**本 UBO は size=768 B (大物)** = 4 個の vec4[8] array (light 配列) を含む。

---

## §1. UBO identity

- **block_name**: `PreviewVParamUBO_Legacy`
- **block_hash**: `0x52d1ac9eu` (= FNV-1a("PreviewVParamUBO_Legacy"))
- **block_size**: 768 B (= std140 608 B、device-padded 768 B)
- **member_count**: 7
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_previewvparamubo_legacy.inl
struct PreviewVParamUBO_LegacyLayout {
    static constexpr std::uint32_t texture_matrix0_OFFSET = 0u;  // size=64 align=16
    static constexpr std::uint32_t ambient_color_OFFSET = 64u;  // size=16 align=16
    static constexpr std::uint32_t color_OFFSET = 80u;  // size=16 align=16
    static constexpr std::uint32_t light_position_OFFSET = 96u;  // size=128 align=16 stride=16
    static constexpr std::uint32_t light_direction_OFFSET = 224u;  // size=128 align=16 stride=16
    static constexpr std::uint32_t light_attenuation_OFFSET = 352u;  // size=128 align=16 stride=16
    static constexpr std::uint32_t light_diffuse_OFFSET = 480u;  // size=128 align=16 stride=16
};
inline constexpr std::uint32_t PreviewVParamUBO_Legacy_SIZE = 768u; // std140=608, device-padded=768
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/preview_v_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 40) uniform PreviewVParamUBO_Legacy
{
    mat4 texture_matrix0;
    vec4 ambient_color;
    vec4 color;
    vec4 light_position[8];
    vec4 light_direction[8];
    vec4 light_attenuation[8];
    vec4 light_diffuse[8];
};
```

= 全 7 member 実 data slot 確定 (= blueprint comment `Source: literal extract from class1/objects/previewV.glsl:47 ifdef LL_VULKAN_GLSL block`)。preview 系 (= inventory item preview / texture preview 等) で使用される vertex shader 用 light 配列 UBO。

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 40
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、verify 要)
- **pipeline layout**: `sAYAStandardLayout`
- **set 3 配置**: Asset+Skin+Legacy 帯 (= INDEX.md §B.4 set mapping)
- **source**: `ubo_metadata.inl:95` `{ "PreviewVParamUBO_Legacy", 0x52d1ac9eu, 768u, 3u, 40u, 0u, 1u, 7u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush、program 単位で値を保持
- **source**: ubo_metadata.inl:95 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - preview render pipeline (= `LLImageGL` preview / `LLViewerObject` preview / texture preview UI、grep verify 要)
  - light state (= 固定 preview light、verify 要)
- **lifetime**: program 単位 (= preview shader program bind 中のみ有効)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/preview_v_param_ubo_legacy.glsl`
- **実 shader use site**:
  - **`class1/objects/previewV.glsl:47 ifdef LL_VULKAN_GLSL block`** (= blueprint comment literal)
- **使用 uniform**: texture_matrix0 (mat4) / ambient_color (vec4) / color (vec4) / light_position[8] / light_direction[8] / light_attenuation[8] / light_diffuse[8]

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL 経路 setter (= preview program の `uniformMatrix4fv(texture_matrix0)` + `uniform4fv(ambient_color/color)` + `uniform4fv(light_position/...)` 呼出 site、grep verify 要)
  - 31 setter 経路で UBO 化対応要

---

## §7. 現状通電状態

- **状態**: **untouched**
- **通電 commit**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step 範囲、verify 要)

---

## §8. 本実装化に必要な作業

1. **shell 通電** + dummy buffer write + bind 経路通電
2. **実 member data 流入**: 既存 OpenGL 経路 setter (= preview render の uniform 書込 site、grep verify 要) を UBO 化
3. **dirty 判定 logic 追加**: PerProgram cadence ゆえ program 切替時に dirty
4. **flush logic 追加**: PerProgram cadence flush
5. **shader 接続**: 実 shader 既存 UBO declaration 使用 = 追加 shader 改変なし
6. **light 配列 size 確認**: vec4[8] 4 array (8 light × 4 attribute) は固定 size、preview light 数 8 で十分か verify 要

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=3 binding=40 配置 | ✅ 維持 |
| OS-3 | std140 padding + array stride 16 厳守 | ✅ 608B → 768B padded、vec4[8] stride=16 codegen 確定 |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 (= 768B align 影響、256B align 境界跨ぐ可能性) |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**大物 UBO (768B) 配置 risk**: 256B 倍数 padded、UBO ring buffer 経路で 1 entry 3 slot 消費可能性 (= 256B × 3 = 768B)、verify 要。

**8-light fixed array**: 既存 OpenGL preview shader の light count 想定 8 と一致するか (= shader literal verify 要)。8 < 実 light 数なら shader extend 必要 = OS-5 違反 risk。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)
- set=3 帯 = Legacy UBO の主要設置帯 (= 76 件中の Legacy 群、INDEX.md 参照)
- 同 set=3 binding 範囲内多数の *ParamUBO_Legacy と同居

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)
- 73 件 (推定) cluster の 1 個

### §11.3 同 shader consume UBO (= class1/objects/previewV.glsl)
- **不明 / verify 要** = previewV.glsl 内で他に consume される UBO (= FrameViewProj / FrameLights 等可能性、grep verify 要)

### §11.4 同 data source UBO
- **不明 / verify 要** = preview light data 由来の他 UBO 確認要

### §11.5 dirty 連動 UBO
- **不明 / verify 要**

### §11.6 layout 共有関係
- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係
- PerProgram cadence ゆえ program bind 時に同時 bind

---

## §10. 不明事項

1. **既存 OpenGL 経路 setter call site** = preview program の uniform 書込 site (= grep verify 要)
2. **preview data source owner class** = LLImageGL / LLViewerObject preview path / texture preview UI 等 (= verify 要)
3. **8-light fixed array の上限** = 既存 OpenGL shader での light count 想定 verify 要
4. **`texture_matrix0` の owner** = preview UV transform 由来 (= verify 要)
5. **shell 通電 commit** = 未来作業
6. **768B UBO ring buffer 配置** = 256B ×3 slot 消費 pattern verify 要
7. **previewV.glsl `_param_ubo_legacy` suffix の意味** = Legacy 標記の意味 (= 既存 OpenGL bare uniform pattern からの UBO 化、verify 要)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.4.16 同期)

**Layer**: L3-16 (= B Tier β setter 推定済、PerProgram cadence、preview render light array 768 B 大物)
**status**: **起案済** (= 2026-06-06 C-5、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.4.16` (= single source of truth)

**sub-work 7 dim 要点**:
- **(1) 前提条件**: L0-1 dispatch + L0-4 cadence
- **(2) 不明事項**: preview render pipeline setter (= `LLImageGL` preview / `LLViewerObject` preview / texture preview UI) [要追加調査] / 8-light fixed array 上限 [要 verify] / 768 B 大物 UBO の ring buffer 配置 [要 verify]
- **(3) 調査手法**: D1 (preview program setter grep) + D2 (`LLFloater*Preview` 構造) + D4 (8 array stride 整合)
- **(4) 設計 task**: L3 全件共通 (= PerProgram triple-buffer / `forwardToUboUpload` PER_PROGRAM / program bind 単位 flush / `previewV.glsl` LL_VULKAN_GLSL 活性化)
- **(5) 工程**: trace L3-16、工数 **M** (= 大物 UBO + 4 array setter 多数)、L3-17/18/19/20 並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= preview render (= inventory item / texture preview) 既存と同一、visual regression ゼロ §5.4)
- **(7) 4 原則 gate**: 全 ✅、原則 4 = `previewV.glsl #else` block uniform 個別宣言維持

**関連**: L0-1 + L0-4 / §5.4 / `LLFloater*Preview` (= inventory item / texture preview 経路)
