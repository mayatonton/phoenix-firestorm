# StarsFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.A PA-8 blueprint 起案済、Phase 1.C PC-2/PC-7δ per-program cadence 一括通電対象に含まれている可能性大 / verify 要)

**本実装化に必要な作業**: shell 3 member (= blend_factor / custom_alpha / time) を実 stars rendering data (= LLVOWLSky / LLEnvironment 経由、verify 要) に置換 + dirty 判定 logic 追加 + 実 shader (= class1/deferred/starsF.glsl) consume 接続検証

---

## §1. UBO identity

- **block_name**: `StarsFParamUBO_Legacy`
- **block_hash**: `0x1654604fu`
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 3
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_starsfparamubo_legacy.inl
struct StarsFParamUBO_LegacyLayout {
    static constexpr std::uint32_t blend_factor_OFFSET = 0u;  // size=4 align=4
    static constexpr std::uint32_t custom_alpha_OFFSET = 4u;  // size=4 align=4
    static constexpr std::uint32_t time_OFFSET = 8u;  // size=4 align=4
};
inline constexpr std::uint32_t StarsFParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/stars_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 42) uniform StarsFParamUBO_Legacy
{
    float blend_factor;
    float custom_alpha;
    float time;
};
```

= **blueprint literal extract from `class1/deferred/starsF.glsl:57` `#ifdef LL_VULKAN_GLSL` block** (= blueprint header line 3 明示)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 42
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: `sAYAStandardLayout`
- **set 3 内訳**: 本 UBO binding=42 = Legacy 帯独立 binding
- **source**: `ubo_metadata.inl:110` `{ "StarsFParamUBO_Legacy", 0x1654604fu, 256u, 3u, 42u, 0u, 1u, 3u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` `kCadencePerProgram = 1u`)
- **source**: ubo_metadata.inl:110 + llglslshader.cpp:95

---

## §4. 物理 owner

- **shell 段階 owner**: 未確認
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `blend_factor` = stars blend factor (= LLVOWLSky / LLEnvironment 由来、day/night blend、verify 要)
  - `custom_alpha` = stars custom alpha (= debug settings / LLEnvironment 由来)
  - `time` = stars time (= per-frame time accumulator、stars twinkle animation 等)
- **lifetime**: per-program (= stars rendering shader instance)
- **同名 member 共有他 shader** (= grep `blend_factor|custom_alpha` 結果): waterF / sunDiscF / cloudsF / cloudsV 等 (= verify 要)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/stars_f_param_ubo_legacy.glsl` (= Phase 1.A PA-8 起案、Source `class1/deferred/starsF.glsl:57`)
- **実 shader use site**:
  - `class1/deferred/starsF.glsl` (= blueprint header line 3 literal reference)
- **consume status**: blueprint literal extract source ゆえ既 consume

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter** (= verify 要)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL setter = `uniform1f("blend_factor", ...)` / `uniform1f("custom_alpha", ...)` / `uniform1f("time", ...)` (= verify 要)
  - data source = LLVOWLSky stars rendering / LLEnvironment day cycle (= verify 要)

---

## §7. 現状通電状態

- **状態**: **shell 通電有無 verify 要**
- **通電内容** (= 推定): zero dummy buffer write + per-program cadence 経路通電

---

## §8. 本実装化に必要な作業

1. **shell 3 member → 実 stars rendering data 接続**:
   - blend_factor = day/night blend 計算 host 側
   - custom_alpha = stars alpha override
   - time = per-frame time accumulator
2. **dirty 判定 logic 追加**:
   - dirty 判定 trigger = day cycle update (= per-frame の可能性、cadence 再評価要)
3. **flush logic 追加**:
   - per-program cadence ゆえ既経路活用
4. **shader 接続検証**:
   - 既存 `class1/deferred/starsF.glsl` `#ifdef LL_VULKAN_GLSL` block (line 57 周辺) で UBO member access 動作確認

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=3 内に収める | ✅ 維持 (binding=42 独立) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 16 B → 256 B padded (3 × float = 12 B + 4 B padding = 16 B std140) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint = 実 shader literal extract |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**cadence 再評価候補**:
- `time` member は per-frame で変化 (= twinkle animation)、per-program cadence では同 program 内 frame 間 stale data risk
- per-frame 系 (= FrameViewProj 帯) への移動候補、現 per-program は Phase 1 初期段階 shell 設計、Phase 2 で再評価

**同名 member 衝突 risk**:
- `blend_factor` / `custom_alpha` 同名 member を他 shader (waterF / sunDiscF / cloudsF / cloudsV) で持つ
- shader 側 nameless block member の global scope export 衝突 risk (= 既存 underWaterF/waterFog で対処された rename pattern、verify 要)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- Asset/Skin 帯 + Legacy 帯各種、本 UBO binding=42 は独立

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- PerProgram cluster 全 88 件

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- `class1/deferred/starsF.glsl` 内同時 consume UBO (= verify 要):
  - 推定候補: FrameViewProj / FrameAtmosphere_Lighting / StarsVParamUBO_Legacy (= V/F pair)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **StarsVParamUBO_Legacy** (= set=3 binding=45、PerProgram、stars_v_time member) = stars V UBO、本 UBO の V/F pair (= 同 stars rendering)
- **SunDiscFParamUBO_Legacy** (= set=3 binding=43、PerProgram、blend_factor member) = sun disc UBO、blend_factor 共有可能性 (= day/night blend 同 source)
- **CloudsFParamUBO_Legacy** / **CloudsVParamUBO_Legacy** = clouds 系 UBO、custom_alpha 共有可能性

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **不明 / verify 要** = day cycle update 時の連動 UBO 確認要
- 推定候補: StarsVParamUBO_Legacy / SunDiscFParamUBO_Legacy / CloudsFParamUBO_Legacy / MoonFParamUBO_Legacy

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- per-program cadence ゆえ starsF program bind 時に descriptor set 3 更新

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **shell 通電有無** = handoff doc chain 参照要
2. **stars rendering data source** = LLVOWLSky / LLEnvironment 内 blend_factor / custom_alpha / time state (= verify 要)
3. **既存 OpenGL setter call site** = 3 member 各 uniform1f setter grep verify 要
4. **cadence 適正性** = time member の per-frame 変化 vs per-program cadence (= verify 要、stale risk)
5. **同名 member 衝突 status** = waterF / sunDiscF / cloudsF / cloudsV の blend_factor / custom_alpha との shader 側 rename 状態 (= verify 要)
6. **同 data source UBO 連動範囲** = day/night blend 共通 source UBO 群 (= verify 要)

= 上記 6 項目は本 UBO file 完成時に grep + Read で逐次解消。
