# PerProgramUBO_WaterF — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `per_program_ubo_water_f.glsl` は実 shader `class3/environment/waterF.glsl ifdef LL_VULKAN_GLSL block` から literal extract 済 (= shader 側 UBO declaration は既に存在)。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加 + 実 shader binding 接続。

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_WaterF`
- **block_hash**: `0x7c76dc82u` (= FNV-1a("PerProgramUBO_WaterF"))
- **block_size**: 256 B (= std140 48 B、device-padded 256 B)
- **member_count**: 8
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_waterf.inl
struct PerProgramUBO_WaterFLayout {
    static constexpr std::uint32_t specular_OFFSET = 0u;  // size=12 align=16
    static constexpr std::uint32_t blend_factor_OFFSET = 12u;  // size=4 align=4
    static constexpr std::uint32_t normScale_OFFSET = 16u;  // size=12 align=16
    static constexpr std::uint32_t blurMultiplier_OFFSET = 28u;  // size=4 align=4
    static constexpr std::uint32_t refScale_OFFSET = 32u;  // size=4 align=4
    static constexpr std::uint32_t kd_OFFSET = 36u;  // size=4 align=4
    static constexpr std::uint32_t fresnelScale_OFFSET = 40u;  // size=4 align=4
    static constexpr std::uint32_t fresnelOffset_OFFSET = 44u;  // size=4 align=4
};
inline constexpr std::uint32_t PerProgramUBO_WaterF_SIZE = 256u; // std140=48, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_water_f.glsl
layout(std140, set = 2, binding = 23) uniform PerProgramUBO_WaterF
{
    vec3  specular;
    float blend_factor;
    vec3  normScale;
    float blurMultiplier;
    float refScale;
    float kd;
    float fresnelScale;
    float fresnelOffset;
};
```

= 全 8 member 実 data slot 確定 (= blueprint comment `Source: literal extract from class3/environment/waterF.glsl:119 ifdef LL_VULKAN_GLSL block (single site)`)。`kd` は **declared-but-unused** (= blueprint comment literal: `kd は declared-but-unused (host setter なし、shader 内参照なし)`)。

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 23
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通、verify 要)
- **pipeline layout**: `sAYAStandardLayout` (= Phase 1.A 確立 5-set V3a layout、`llvkloader.cpp:881` literal)
- **set 2 配置**: PerDraw+PerProgram 帯
- **source**: `ubo_metadata.inl:93` `{ "PerProgramUBO_WaterF", 0x7c76dc82u, 256u, 2u, 23u, 0u, 1u, 8u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal: `constexpr U32 kCadencePerProgram = 1u`)
- **意味詳細**: program 切替時に flush、program 単位で値を保持 (= `forwardToUboUpload` `kCadencePerProgram` case、`llglslshader.cpp:2147`)
- **source**: ubo_metadata.inl:93 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電 (= shell も未配置)
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - 水面描画 pipeline (= `LLDrawPoolWater` 内 / `LLPipeline::renderWater` 経路、grep verify 要)
  - water cvar 直接読出 (= RenderWaterRefResolution / WaterSpecular 等、grep verify 要)
  - water environment params (= 環境設定の water 系 settings、verify 要)
- **lifetime**: program 単位 (= water shader program bind 中のみ有効)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_water_f.glsl` (= Phase 1.A PA-8 起案、8 member 実 data slot)
- **実 shader use site**:
  - **`class3/environment/waterF.glsl:119 ifdef LL_VULKAN_GLSL block`** (= single site、blueprint comment literal)
  - shader 内 UBO declaration 既存 (= blueprint と同 layout)
- **使用 uniform**: specular (vec3) / blend_factor (float) / normScale (vec3) / blurMultiplier (float) / refScale (float) / kd (float、dead) / fresnelScale (float) / fresnelOffset (float)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし (= untouched)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL 経路 setter (= `pipeline.cpp` または `lldrawpoolwater.cpp` 内 `uniform3fv/uniform1f` 呼出、grep verify 要)
  - 31 setter 経路 (= mUseUBO 分岐) で UBO 化対応要

---

## §7. 現状通電状態

- **状態**: **untouched** (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)
- **通電 commit**: なし (= 未通電)
- **通電内容**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 範囲、verify 要)

---

## §8. 本実装化に必要な作業

1. **shell 通電** (= Phase 2 着手時):
   - blueprint ベース host 側 buffer 配置 + descriptor set 配線
   - dummy buffer write + bind 経路通電
2. **実 member data 流入**:
   - 既存 OpenGL 経路 setter (= waterF program の uniform 書込 site、grep verify 要) を UBO 化
3. **dirty 判定 logic 追加**:
   - PerProgram cadence ゆえ program 切替時に dirty
4. **flush logic 追加**:
   - PerProgram cadence flush (= `writePerProgramUbo` 経路)
5. **shader 接続**:
   - 実 shader `class3/environment/waterF.glsl ifdef LL_VULKAN_GLSL block` で UBO declaration 既存 = 追加 shader 改変なし (= OS-5 充足)
6. **dead member 整理**:
   - `kd` (declared-but-unused) は member 維持 (= layout 不変契約)、host setter は no-op で良い

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=2 binding=23 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 48B → 256B padded (codegen 出力) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**dead uniform 維持 risk**: `kd` は declared-but-unused、削除すると layout 不整合 → **維持必須**。

**std140 vec3 padding**: vec3 `specular` の後ろに `blend_factor` (float) が続き 16 B align、`normScale` 後ろに `blurMultiplier` (float) で 16 B align、std140 vec3+float 結合 pattern。codegen 出力で確定済。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 = PerDraw + PerProgram 混在
- 同 set=2 binding 範囲内 PerProgramUBO_* と同居

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 73 件 (推定) cluster の 1 個、`writePerProgramUbo` 経路で flush

### §11.3 同 shader consume UBO (= class3/environment/waterF.glsl)

- **不明 / verify 要** = waterF.glsl 内で他に consume される UBO (= Frame 系 + WaterFogUBO_Legacy / WaterVParamUBO_Legacy / UnderWaterFParamUBO_Legacy 等可能性、grep verify 要)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **可能性** (= verify 要): WaterFogUBO_Legacy (set=3 binding=9) / WaterVParamUBO_Legacy (set=3 binding=60) / UnderWaterFParamUBO_Legacy (set=3 binding=39) = 水面描画関連 UBO 群、同 owner class (= LLDrawPoolWater) 由来可能性大

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **不明 / verify 要** = water render program 切替時の同時 dirty UBO 群

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- PerProgram cadence ゆえ program bind 時に同時 bind

---

## §10. 不明事項

1. **既存 OpenGL 経路 setter call site 完全特定** = waterF program の uniform 書込 site (= grep verify 要)
2. **water data source の owner class** = LLDrawPoolWater / LLPipeline / water cvar 直接読出 (= verify 要)
3. **`kd` の本来意図** = declared-but-unused 状態の起源 (= blueprint extract source / 上流 OpenGL 経路 verify 要)
4. **同 shader file 内同時 consume UBO 一覧** = waterF.glsl 内全 UBO declaration grep 要
5. **shell 通電 commit** = 本 UBO 用 shell が今後配置される際の commit (= 未来作業)
6. **water 関連 UBO 群 (WaterFog/WaterV/UnderWater) との data source 共有関係** = 確認要
7. **PerProgram flush 経路の VkDescriptorBufferInfo bind 詳細** = `writePerProgramUbo` 経路実装詳細

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.6 同期)

**Layer**: L4-6 (= C 判定 water 系 5 UBO 連動 dirty group)
**status**: **起案済** (= 2026-06-06 C-6、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.6` (= single source of truth)
**要点**: water 5 UBO 連動 (= WaterFog + WaterV + UnderWaterF + WaterF + WaterHazeV)、本 UBO waterF program 専用 (set=2 binding=23)、8 member (specular/blend_factor/normScale/blurMultiplier/refScale/kd(dead)/fresnelScale/fresnelOffset)、kd declared-but-unused layout 維持必須、工数 L (group 全体)、AYA live verify (= waterF 描画、visual regression ゼロ §5.4)
**関連**: L0-1 dispatch (= waterF program 識別) / L0-4 cadence 再評価 / WaterFog/WaterV/UnderWaterF (= 同 LLEnvironment data source) / §5.4 visual regression policy
