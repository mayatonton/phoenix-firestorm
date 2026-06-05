# SunDiscFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.A PA-8 blueprint 起案済、Phase 1.C PC-2/PC-7δ per-program cadence 一括通電対象に含まれている可能性大 / verify 要)

**本実装化に必要な作業**: shell 1 member (= blend_factor) を実 sun disc blend data (= LLEnvironment day cycle 経由、verify 要) に置換 + dirty 判定 logic 追加 + 実 shader (= class1/deferred/sunDiscF.glsl) consume 接続検証

---

## §1. UBO identity

- **block_name**: `SunDiscFParamUBO_Legacy`
- **block_hash**: `0x03346425u`
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_sundiscfparamubo_legacy.inl
struct SunDiscFParamUBO_LegacyLayout {
    static constexpr std::uint32_t blend_factor_OFFSET = 0u;  // size=4 align=4
};
inline constexpr std::uint32_t SunDiscFParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/sun_disc_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 43) uniform SunDiscFParamUBO_Legacy
{
    float blend_factor;
};
```

= **blueprint literal extract from `class1/deferred/sunDiscF.glsl:48` `#ifdef LL_VULKAN_GLSL` block** (= blueprint header line 3 明示)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 43
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: `sAYAStandardLayout`
- **set 3 内訳**: 本 UBO binding=43 = Legacy 帯独立 binding
- **source**: `ubo_metadata.inl:112` `{ "SunDiscFParamUBO_Legacy", 0x03346425u, 256u, 3u, 43u, 0u, 1u, 1u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` `kCadencePerProgram = 1u`)
- **source**: ubo_metadata.inl:112 + llglslshader.cpp:95

---

## §4. 物理 owner

- **shell 段階 owner**: 未確認
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `blend_factor` = sun disc blend factor (= LLEnvironment day cycle / WindLight 由来、day/night blend、verify 要)
- **lifetime**: per-program (= sun disc rendering shader instance)
- **同名 member 共有 UBO**: StarsFParamUBO_Legacy (blend_factor 同名)、共通 day/night blend source の可能性 (= verify 要)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/sun_disc_f_param_ubo_legacy.glsl` (= Phase 1.A PA-8 起案、Source `class1/deferred/sunDiscF.glsl:48`)
- **実 shader use site**:
  - `class1/deferred/sunDiscF.glsl` (= blueprint header line 3 literal reference)
- **consume status**: blueprint literal extract source ゆえ既 consume

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter** (= verify 要)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL setter = `uniform1f("blend_factor", ...)` (= verify 要)
  - data source = LLEnvironment day cycle / WindLight blend 計算

---

## §7. 現状通電状態

- **状態**: **shell 通電有無 verify 要**
- **通電内容** (= 推定): zero dummy buffer write + per-program cadence 経路通電

---

## §8. 本実装化に必要な作業

1. **shell `blend_factor` → 実 sun disc blend data 接続**:
   - LLEnvironment day cycle 計算 host 側
2. **dirty 判定 logic 追加**:
   - dirty 判定 trigger = day cycle update (= per-frame の可能性、cadence 再評価要)
3. **flush logic 追加**:
   - per-program cadence ゆえ既経路活用
4. **shader 接続検証**:
   - 既存 `class1/deferred/sunDiscF.glsl` `#ifdef LL_VULKAN_GLSL` block (line 48 周辺) で UBO member access 動作確認

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=3 内に収める | ✅ 維持 (binding=43 独立) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 16 B → 256 B padded (1 × float = 4 B + 12 B padding = 16 B std140) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint = 実 shader literal extract |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**cadence 再評価候補**:
- `blend_factor` は per-frame で変化 (= day cycle 連続変化)、per-program cadence stale risk
- per-frame 系への移動候補、Phase 2 で再評価

**同名 member 衝突 risk**:
- StarsFParamUBO_Legacy.blend_factor と同名
- shader 側 nameless block member 衝突 risk (= 別 program で binding 異なるため衝突なしと推定、verify 要)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- Asset/Skin 帯 + Legacy 帯各種、本 UBO binding=43 は独立

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- PerProgram cluster 全 88 件

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- `class1/deferred/sunDiscF.glsl` 内同時 consume UBO (= verify 要):
  - 推定候補: FrameViewProj / FrameAtmosphere_Lighting / FrameLights (= sun direction)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **StarsFParamUBO_Legacy** (= set=3 binding=42、PerProgram、blend_factor member) = stars F UBO、blend_factor 共有可能性 (= day/night blend 同 source)
- **MoonFParamUBO_Legacy** (= set=3 binding=44、PerProgram) = moon F UBO、day/night blend 連動可能性
- **SkyFParamUBO_Legacy** / **SkyVParamUBO_Legacy** = sky 系 UBO、day cycle 連動可能性

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **StarsFParamUBO_Legacy** / **MoonFParamUBO_Legacy** / **SkyFParamUBO_Legacy** (= day cycle update 時の連動候補)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- per-program cadence ゆえ sunDiscF program bind 時に descriptor set 3 更新

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **shell 通電有無** = handoff doc chain 参照要
2. **blend_factor data source** = LLEnvironment day cycle / WindLight 経路 (= verify 要)
3. **既存 OpenGL setter call site** = uniform1f("blend_factor", ...) grep verify 要
4. **cadence 適正性** = day cycle 連続変化 blend_factor の per-program cadence stale risk (= verify 要)
5. **同名 member 衝突 status** = StarsFParamUBO_Legacy.blend_factor との shader 側衝突有無 (= 別 program/binding ゆえ衝突なしと推定、verify 要)
6. **同 data source UBO 連動範囲** = day/night blend 共通 source UBO 群 (= verify 要)

= 上記 6 項目は本 UBO file 完成時に grep + Read で逐次解消。
