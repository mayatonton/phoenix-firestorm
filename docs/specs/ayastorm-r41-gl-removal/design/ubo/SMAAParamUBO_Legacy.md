# SMAAParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `smaa_param_ubo_legacy.glsl` は実 shader `class1/deferred/SMAA.glsl ifdef LL_VULKAN_GLSL block` から literal extract 済。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加。**SMAA pipeline 全 pass 共通 RT metrics UBO** (= SMAA pass chain で SMAA.glsl shared include 経由共通使用)。

---

## §1. UBO identity

- **block_name**: `SMAAParamUBO_Legacy`
- **block_hash**: `0xaa2ed51au` (= FNV-1a("SMAAParamUBO_Legacy"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_smaaparamubo_legacy.inl
struct SMAAParamUBO_LegacyLayout {
    static constexpr std::uint32_t SMAA_RT_METRICS_OFFSET = 0u;  // size=16 align=16
};
inline constexpr std::uint32_t SMAAParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/smaa_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 14) uniform SMAAParamUBO_Legacy
{
    vec4 SMAA_RT_METRICS;
};
```

= 1 member vec4 `SMAA_RT_METRICS` (= SMAA standard RT metrics: `(1/screen_w, 1/screen_h, screen_w, screen_h)`)。

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 14
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、verify 要)
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:100` `{ "SMAAParamUBO_Legacy", 0xaa2ed51au, 256u, 3u, 14u, 0u, 1u, 1u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush、SMAA pass の全 program で共通値 = screen resize 時のみ実 update
- **source**: ubo_metadata.inl:100 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - screen resolution (= `gViewerWindow` size or render target size、grep verify 要)
  - SMAA post-process pipeline (= LLPipeline::renderPostProcess 経路)
- **lifetime**: process-wide (resize 時のみ更新) だが cadence=PerProgram ゆえ各 SMAA program bind 時に bind

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/smaa_param_ubo_legacy.glsl`
- **実 shader use site**:
  - **`class1/deferred/SMAA.glsl:41 ifdef LL_VULKAN_GLSL block`** (= blueprint comment literal)
  - SMAA.glsl は **shared include** = SMAA edge detection / blend weights / neighborhood blending 全 pass で include される共通 header (= verify 要)
- **使用 uniform**: SMAA_RT_METRICS (vec4)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **本実装化後 setter** (= **不明 / verify 要**):
  - SMAA pass setter (= `uniform4fv(SMAA_RT_METRICS, vec4(1/w, 1/h, w, h))` 呼出 site、grep verify 要)
  - viewer resize callback で update (= verify 要)

---

## §7. 現状通電状態

- **状態**: **untouched**
- **通電 commit**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step 範囲、verify 要)

---

## §8. 本実装化に必要な作業

1. **shell 通電** + dummy buffer write + bind 経路通電
2. **実 member data 流入**: screen resolution → SMAA_RT_METRICS 計算 + UBO write
3. **dirty 判定 logic 追加**: PerProgram cadence + viewer resize 時 dirty
4. **flush logic 追加**: PerProgram cadence flush
5. **shader 接続**: 実 shader 既存 UBO declaration 使用 = 追加 shader 改変なし
6. **SMAA pass chain 全 program で共通使用 verify**: SMAA edge / blend weights / neighborhood blending 全 program で同 UBO bind 確認要

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=3 binding=14 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**resize 連動 risk**: viewer resize 時 screen resolution 変化 → SMAA_RT_METRICS 全 component 更新必要、frame 切替時 staleness check 必須。

**cadence 妥当性 question**: 全 SMAA program で同値共有 + resize 時のみ update = cadence=PerFrame or SINGLETON 候補だが現状 PerProgram (= ubo_metadata.inl literal)。Phase 2 設計で cadence 再検討余地有り得る、ただし shell layout 不変契約遵守。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)
- set=3 帯 Legacy 群

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)
- 73 件 (推定) cluster

### §11.3 同 shader consume UBO (= SMAA.glsl shared include)
- **不明 / verify 要** = SMAA.glsl を include する全 pass で同 UBO consume = edge detection / blend weights / neighborhood blending 3 pass 想定 (= verify 要)
- **SMAABlendWeightsFParamUBO_Legacy** = 同 SMAA pipeline の blend weights pass 専用 UBO、本 UBO と同時 bind 可能性

### §11.4 同 data source UBO
- **可能性** (= verify 要): **SMAABlendWeightsFParamUBO_Legacy** (set=3 binding=62) = 同 SMAA pipeline sibling
- screen resolution 由来 = 他 post-process UBO 群も同 source 由来可能性大 (= ScreenSpaceReflPostFParamUBO_Legacy / CASParamUBO_Legacy 等、verify 要)

### §11.5 dirty 連動 UBO
- **可能性** (= verify 要): viewer resize 時に screen res 由来全 UBO 同時 dirty

### §11.6 layout 共有関係
- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係
- PerProgram cadence ゆえ program bind 時に同時 bind
- SMAA pass chain (= edge → blend weights → neighborhood) で連続 program bind = 全 pass で同 UBO bind 共有

---

## §10. 不明事項

1. **既存 OpenGL 経路 setter call site** = SMAA_RT_METRICS 書込 site (= grep verify 要)
2. **resize callback での update 経路** = viewer resize → SMAA_RT_METRICS recompute (= verify 要)
3. **SMAA pass chain 全 pass 一覧** = SMAA.glsl include する全 shader file 列挙 (= grep verify 要)
4. **SMAABlendWeightsFParamUBO_Legacy との data source 共有** = 同 SMAA pipeline ゆえ共有 verify 要
5. **cadence 設計妥当性** = PerProgram vs PerFrame/SINGLETON 再検討余地 (= Phase 2 設計入力)
6. **screen res 由来 UBO 群一覧** = 同 source 由来全 UBO の同時 dirty 連動設計 (= verify 要)
7. **shell 通電 commit** = 未来作業

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。
