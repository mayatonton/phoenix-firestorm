# StarsVParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.A PA-8 blueprint 起案済、Phase 1.C PC-2/PC-7δ per-program cadence 一括通電対象に含まれている可能性大 / verify 要)

**本実装化に必要な作業**: shell 1 member (= stars_v_time) を実 stars V time data に置換 + dirty 判定 logic 追加 + 実 shader (= class1/deferred/starsV.glsl) consume 接続検証

---

## §1. UBO identity

- **block_name**: `StarsVParamUBO_Legacy`
- **block_hash**: `0x6a17ccbfu`
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_starsvparamubo_legacy.inl
struct StarsVParamUBO_LegacyLayout {
    static constexpr std::uint32_t stars_v_time_OFFSET = 0u;  // size=4 align=4
};
inline constexpr std::uint32_t StarsVParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/stars_v_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 45) uniform StarsVParamUBO_Legacy
{
    float stars_v_time;
};
```

= **blueprint literal extract from `class1/deferred/starsV.glsl:67` `#ifdef LL_VULKAN_GLSL` block** (= blueprint header line 3 明示)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 45
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: `sAYAStandardLayout`
- **set 3 内訳**: 本 UBO binding=45 = Legacy 帯独立 binding
- **source**: `ubo_metadata.inl:111` `{ "StarsVParamUBO_Legacy", 0x6a17ccbfu, 256u, 3u, 45u, 0u, 1u, 1u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` `kCadencePerProgram = 1u`)
- **source**: ubo_metadata.inl:111 + llglslshader.cpp:95

---

## §4. 物理 owner

- **shell 段階 owner**: 未確認
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `stars_v_time` = stars V shader time (= per-frame time accumulator、stars position animation 等、LLEnvironment 経由、verify 要)
- **lifetime**: per-program (= stars V rendering shader instance)
- **命名 `stars_v_time`**: V (vertex) shader 専用 time、F shader の `time` (= StarsFParamUBO_Legacy.time member) と命名分離 (= shader 側 nameless block member 衝突回避 rename と推定、verify 要)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/stars_v_param_ubo_legacy.glsl` (= Phase 1.A PA-8 起案、Source `class1/deferred/starsV.glsl:67`)
- **実 shader use site**:
  - `class1/deferred/starsV.glsl` (= blueprint header line 3 literal reference)
- **consume status**: blueprint literal extract source ゆえ既 consume

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter** (= verify 要)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL setter = `uniform1f("time", ...)` (= shader 側 nameless block member 命名は `stars_v_time` だが #define alias の可能性、verify 要)
  - data source = LLEnvironment day cycle time / per-frame time accumulator

---

## §7. 現状通電状態

- **状態**: **shell 通電有無 verify 要**
- **通電内容** (= 推定): zero dummy buffer write + per-program cadence 経路通電

---

## §8. 本実装化に必要な作業

1. **shell `stars_v_time` → 実 stars V time data 接続**:
   - per-frame time accumulator 接続
2. **dirty 判定 logic 追加**:
   - dirty 判定 trigger = per-frame time update
3. **flush logic 追加**:
   - per-program cadence ゆえ既経路活用
4. **shader 接続検証**:
   - 既存 `class1/deferred/starsV.glsl` `#ifdef LL_VULKAN_GLSL` block (line 67 周辺) で UBO member access 動作確認

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=3 内に収める | ✅ 維持 (binding=45 独立) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 16 B → 256 B padded (1 × float = 4 B + 12 B padding = 16 B std140) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint = 実 shader literal extract |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**cadence 再評価候補**:
- `stars_v_time` は per-frame で変化、per-program cadence では stale data risk
- per-frame 系への移動候補、Phase 2 で再評価

**命名 rename 状態 verify**:
- `stars_v_time` 命名は F shader `time` (= StarsFParamUBO_Legacy.time member) と分離
- shader 側 nameless block member 衝突回避の rename pattern と推定 (= memory `feedback_handoff_minimal_pre_req_read` 参照、underWaterF/waterFog で対処済 pattern と同形、verify 要)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- Asset/Skin 帯 + Legacy 帯各種、本 UBO binding=45 は独立

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- PerProgram cluster 全 88 件

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- `class1/deferred/starsV.glsl` 内同時 consume UBO (= verify 要):
  - 推定候補: FrameViewProj (set=0 binding=0、view+proj matrix)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **StarsFParamUBO_Legacy** (= set=3 binding=42、PerProgram、time member) = stars V/F pair (= 同 stars rendering)
  - `stars_v_time` (本 UBO) と `time` (StarsFParamUBO_Legacy) は同 source の可能性大、shader 別 (V/F) ゆえ UBO 別宣言

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **StarsFParamUBO_Legacy** (= V/F pair、同 stars rendering 同時 dirty 高確率)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- per-program cadence ゆえ starsV program bind 時に descriptor set 3 更新

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **shell 通電有無** = handoff doc chain 参照要
2. **stars_v_time data source** = LLEnvironment day cycle time / per-frame time accumulator 経路 (= verify 要)
3. **既存 OpenGL setter call site** = uniform1f("time", ...) または "stars_v_time" 名前 grep verify 要
4. **命名 rename 経緯** = shader 側 nameless block member 衝突回避の rename pattern verify 要
5. **cadence 適正性** = per-frame 変化 stars_v_time の per-program cadence stale risk (= verify 要)
6. **StarsFParamUBO_Legacy.time との data source 共有** = 同 stars rendering V/F pair の time member 共有経路 (= verify 要)

= 上記 6 項目は本 UBO file 完成時に grep + Read で逐次解消。
