# SimpleColorFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `simple_color_f_param_ubo_legacy.glsl` は実 shader `class1/objects/simpleColorF.glsl ifdef LL_VULKAN_GLSL block` から literal extract 済。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加。**simple color fragment 用最小 UBO** (= 1 member float `waterSign`、water surface 表/裏判定用 sign flag)。

---

## §1. UBO identity

- **block_name**: `SimpleColorFParamUBO_Legacy`
- **block_hash**: `0x06843627u` (= FNV-1a("SimpleColorFParamUBO_Legacy"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_simplecolorfparamubo_legacy.inl
struct SimpleColorFParamUBO_LegacyLayout {
    static constexpr std::uint32_t waterSign_OFFSET = 0u;  // size=4 align=4
};
inline constexpr std::uint32_t SimpleColorFParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/simple_color_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 41) uniform SimpleColorFParamUBO_Legacy
{
    float waterSign;
};
```

= 1 member float `waterSign` (= +1.0 or -1.0、water plane 上下判定 sign)。

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 41
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、verify 要)
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:103` `{ "SimpleColorFParamUBO_Legacy", 0x06843627u, 256u, 3u, 41u, 0u, 1u, 1u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush、water sign は camera 位置に依存 = frame 単位で 1 値
- **source**: ubo_metadata.inl:103 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - LLViewerCamera 位置 vs water plane z 判定 (= viewer 水中/水上判定、grep verify 要)
  - simple color 系 drawing pipeline (= LLDrawPool 内 simple 系、verify 要)
- **lifetime**: program 単位 (= simple color shader bind 中)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/simple_color_f_param_ubo_legacy.glsl`
- **実 shader use site**:
  - **`class1/objects/simpleColorF.glsl:64 ifdef LL_VULKAN_GLSL block`** (= blueprint comment literal)
- **使用 uniform**: waterSign (float)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **本実装化後 setter** (= **不明 / verify 要**):
  - simple color setter (= `uniform1f(waterSign, +1.0/-1.0)` 呼出 site、grep verify 要)

---

## §7. 現状通電状態

- **状態**: **untouched**
- **通電 commit**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step 範囲、verify 要)

---

## §8. 本実装化に必要な作業

1. **shell 通電** + dummy buffer write + bind 経路通電
2. **実 member data 流入**: viewer camera 位置 vs water plane 判定 → waterSign +1/-1 計算 + UBO write
3. **dirty 判定 logic 追加**: PerProgram cadence + camera 水中/水上 cross 時 dirty
4. **flush logic 追加**: PerProgram cadence flush
5. **shader 接続**: 実 shader 既存 UBO declaration 使用 = 追加 shader 改変なし

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=3 binding=41 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**1 member の存在意義 question**: float 1 個のみの UBO は冗長性高い、他 UBO に統合可能性検討 (= 同 PerProgramUBO_WaterHazeV の above_water int と相補、合体検討余地)。ただし shell layout 不変契約遵守。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)
- set=3 帯 Legacy 群

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)
- 73 件 (推定) cluster

### §11.3 同 shader consume UBO (= class1/objects/simpleColorF.glsl)
- **不明 / verify 要** = simpleColorF.glsl 内同時 consume UBO (= FrameViewProj 等可能性、verify 要)

### §11.4 同 data source UBO
- **可能性** (= verify 要): PerProgramUBO_WaterHazeV (set=2 binding=15) の above_water (int) = 同 camera 水面判定 data 由来、本 UBO の waterSign (float +1/-1) と意味的に同等、verify 要

### §11.5 dirty 連動 UBO
- **可能性** (= verify 要): PerProgramUBO_WaterHazeV + 他 water 系 UBO 群 = camera 水中/水上 cross 時に同時 dirty

### §11.6 layout 共有関係
- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係
- PerProgram cadence ゆえ program bind 時に同時 bind

---

## §10. 不明事項

1. **既存 OpenGL 経路 setter call site** = simple color の waterSign 書込 site (= grep verify 要)
2. **waterSign 計算 logic** = camera Z vs water plane Z 判定実装 (= grep verify 要)
3. **PerProgramUBO_WaterHazeV.above_water との関係** = 同 data 由来か独立か (= verify 要)
4. **simple color shader 適用範囲** = 何の object/draw call で使われるか (= grep verify 要)
5. **shell 通電 commit** = 未来作業
6. **camera 水中/水上 cross dirty trigger** = フレーム毎チェック or event-driven (= verify 要)
7. **`waterSign` 名 / float -1/+1 値選定理由** = 上流 OpenGL 由来 (= verify 要)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。
