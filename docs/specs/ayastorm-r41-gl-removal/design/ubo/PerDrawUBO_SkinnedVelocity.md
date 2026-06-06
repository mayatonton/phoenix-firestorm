# PerDrawUBO_SkinnedVelocity — UBO design (= 実コードベース調査資料)

**通電状態**: **untouched** (= host C++ で `PerDrawUBO_SkinnedVelocity` への `writeDrawUbo` / `register*` 呼出ゼロ、grep 確認済 2026-06-06)

**本実装化に必要な作業**: 実 `lastMatrixPalette_skinned_velocity[110]` (= mat3x4 stride=48 × 110 joint、前 frame bone palette) を host から writeDrawUbo 経由で書込 + class1/deferred/skinnedVelocityV.glsl + skinnedVelocityAlphaV.glsl 内 UBO consume へ切替

**異常 size 注記**: 5376 B (= std140 5280 B + device pad 96 B) = PerDrawUBO_ObjectSkin 半分 (= lastMatrixPalette のみ抽出版、現 frame palette 不要 = velocity computation 専用)

---

## §1. UBO identity

- **block_name**: `PerDrawUBO_SkinnedVelocity`
- **block_hash**: `0x2531887eu` (= FNV-1a("PerDrawUBO_SkinnedVelocity"))
- **block_size**: 5376 B (= std140=5280 B, device-padded 5376 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perdrawubo_skinnedvelocity.inl:12-15
struct PerDrawUBO_SkinnedVelocityLayout {
    static constexpr std::uint32_t lastMatrixPalette_skinned_velocity_OFFSET = 0u;  // size=5280 align=16 stride=48
};
inline constexpr std::uint32_t PerDrawUBO_SkinnedVelocity_SIZE = 5376u; // std140=5280, device-padded=5376
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_draw_ubo_skinned_velocity.glsl:14-17
layout(std140, set = 2, binding = 0) uniform PerDrawUBO_SkinnedVelocity
{
    mat3x4 lastMatrixPalette_skinned_velocity[110];
};
```

= **1 member = mat3x4[110] (= 前 frame bone palette のみ、velocity computation 専用)**

**MAX_JOINTS_PER_MESH_OBJECT = 110** (= viewer 側 addPermutation で 110 inject、`llviewershadermgr.cpp:3353/3383` `getMaxJointCount()=LL_MAX_JOINTS_PER_MESH_OBJECT=110`、per_draw_ubo_skinned_velocity.glsl:7-9 header literal)

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 0
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC` (= set=2 UBO_DYNAMIC 帯)
- **pipeline layout**: `sAYAStandardLayout`
- **set=2 binding=0 共有 6 UBO の 1 名** (= name-based dispatch)
- **source**: ubo_metadata.inl:70 `{ "PerDrawUBO_SkinnedVelocity", 0x2531887eu, 5376u, 2u, 0u, 0u, 2u, 1u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 2
- **意味**: **PerDraw** (= `llglslshader.cpp:96`)
- **flush 経路**: `LLVKLoader::writeDrawUbo`
- **source**: llglslshader.cpp:2151-2167

---

## §4. 物理 owner

- **shell 段階**: owner なし
- **本実装化後の data source** (= **verify 要**):
  - `LLVOAvatar::mPrevFrameRiggedJointMatrix` (= 推定、grep verify 要)
  - PerDrawUBO_ObjectSkin の `lastMatrixPalette` member と同 cache (= 同 data source、別 UBO export 形式)
- **lifetime**: per-draw call (= velocity-enabled rigged draw 毎)
- **trigger**: velocity computation 必要 phase (= deferred motion blur / TAA velocity buffer 等)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_draw_ubo_skinned_velocity.glsl`
  - source extract from `class1/deferred/skinnedVelocityV.glsl:81` ifdef LL_VULKAN_GLSL block (verified identical across 2 sample sites = skinnedVelocityV / skinnedVelocityAlphaV) (= header:1-9 literal)
- **実 shader use site** (= grep 結果):
  - `indra/newview/app_settings/shaders/class1/deferred/skinnedVelocityV.glsl`
  - `indra/newview/app_settings/shaders/class1/deferred/skinnedVelocityAlphaV.glsl`
- = **2 shader file consume (= velocity buffer 用 opaque + alpha 2 variant)**

---

## §6. 既存 setter call site (host C++)

- **現状**: **PerDrawUBO_SkinnedVelocity 専用 setter 不在** (= grep 確認済 2026-06-06)
- **shell 段階通電経路**: 推定 = bringupTestUBO 経由 generic zero buffer 通電 (= 5376 B 中型、ring buffer 容量 verify 要)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL 経路で `lastMatrixPalette_skinned_velocity` uniform を書込む host C++ site (= velocity shader 用 setter、grep verify 要)
  - PerDrawUBO_ObjectSkin の `lastMatrixPalette` と同 data 投入 = 同一 host 経路で 2 UBO 同時書込み可能性 (= 推定、verify 要)

---

## §7. 現状通電状態

- **状態**: **untouched** (= Phase 1.E 終了時点)
- **通電 commit**: なし
- **通電内容**:
  - host C++ 側 `writeDrawUbo(PerDrawUBO_SkinnedVelocity, ...)` 呼出 0 件
  - shader 側 `class1/deferred/skinnedVelocityV.glsl` + `skinnedVelocityAlphaV.glsl` で UBO consume block 既配置
- **shell 通電経路** (= 推定 / verify 要): 5376 B ゆえ bringupTestUBO 経路で通電可否 verify 要

---

## §8. 本実装化に必要な作業

1. **host C++ writeDrawUbo 配線** = velocity draw 経路で `LLVKLoader::writeDrawUbo(ubo::block_hash::PerDrawUBO_SkinnedVelocity, 0u, last_palette_data, 5280, dynamic_offset)` 配線追加
2. **data source 統合** = PerDrawUBO_ObjectSkin の `lastMatrixPalette` cache と同 data (= host 側 1 cache から 2 UBO 同時書込み戦略 verify 要)
3. **mat3x4 packing** = stride=48、ObjectSkin と同形
4. **dirty 判定** = bone animation 毎 frame 更新 + frame swap で current → last shift
5. **velocity-enabled draw 限定** = 全 rigged draw でなく velocity buffer 生成必要時のみ書込 (= cadence per-draw だが draw 種別で skip 判定要、verify 要)

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | set=2 binding=0 共有 6 UBO | ✅ name-based dispatch 既配置 |
| OS-3 | std140 padding 厳守 + mat3x4 stride=48 | ✅ 110 × 48 = 5280 B std140、5376 B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既存 GLSL #ifdef LL_VULKAN_GLSL 既配置 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**異常 size risk**:
- 5376 B = PerDraw cluster 2 番目大型 (ObjectSkin 10752 B 次点)
- per-draw cadence × 5376 B = ring buffer 消費 risk、velocity-enabled scene で verify 要
- 解決策候補 = PerDrawUBO_ObjectSkin と統合検討 (= 同 data lastMatrixPalette を 1 UBO から再利用) = Phase 1.B 設計 verify 要

**data duplication 罠**:
- PerDrawUBO_ObjectSkin.lastMatrixPalette と PerDrawUBO_SkinnedVelocity.lastMatrixPalette_skinned_velocity = 同 data
- 2 UBO 別 binding 配置で host 側 2 回書込 = 5280 B × 2 = 10560 B/draw ring buffer 消費
- 統合 candidate = ObjectSkin pass で SkinnedVelocity shader が ObjectSkin UBO 直接 consume (= shader 改修必要、別案)

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **shell 通電可否** = 5376 B が bringupTestUBO 経路で通電可能か verify 要
2. **既存 OpenGL setter call site** = `lastMatrixPalette_skinned_velocity` uniform 書込 host C++ site (= velocity shader 用、grep verify 要)
3. **data source 上流** = `LLVOAvatar` の prev frame palette cache 機構 (= grep verify 要)
4. **velocity-enabled draw 判定 logic** = どの rigged draw が velocity buffer 生成必要か判定 (= deferred motion blur / TAA / DoF 等の trigger 条件、verify 要)
5. **ObjectSkin との統合検討** = data duplication 回避 strategy (= Phase 1.B 設計再検討要)
6. **2 shader file consume 統一確認** = skinnedVelocityV / skinnedVelocityAlphaV 内 UBO block 内容完全一致確認 (= blueprint header「verified identical」literal 既宣言)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 4 binding、本 UBO は binding=0 (= 6 共有 UBO のうち 1)

### §11.2 同 cadence cluster UBO (= cadence_tag=2 PerDraw)

- PerDraw cluster 7 UBO (PerDrawUBO_ClipPlane §11.2 参照)

### §11.3 同 shader consume UBO

- `class1/deferred/skinnedVelocityV.glsl` + `skinnedVelocityAlphaV.glsl` 内同時 consume = Frame_* set=0 帯 + PerProgramUBO_VelocityAlphaV (= velocity alpha 用 program data) (= verify 要)

### §11.4 同 data source UBO

- **PerDrawUBO_ObjectSkin (本 UBO と同 data 由来)** = `lastMatrixPalette` member と同 data、別 UBO export
- **Skin_GLTFJoints (set=3 binding=2 cadence=4)** = GLTF 専用 (= 本 UBO と別 owner、別 cadence)
- **PerDrawUBO_AvatarVelocity (set=2 binding=0 cadence=2 size=768)** = avatar 用 velocity 版 (= 本 UBO の avatar variant、verify 要)

### §11.5 dirty 連動 UBO

- 同 rigged mesh velocity draw で PerDrawUBO_ObjectSkin と連動 dirty

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- per-draw cadence ゆえ毎 velocity draw call で `bindV3aRigged` 経由 set=2 帯 4 binding 同時 bind

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.8 同期)

**Layer**: L4-8 sub-cluster (b) (= object curr/prev pair + 抽出版 2 UBO、data duplication 解消候補)
**status**: **起案済** (= 2026-06-06 C-6-c、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.8` (= single source of truth)
**要点**: 1 member (lastMatrixPalette_skinned_velocity[110] mat3x4 stride=48)、5376 B = PerDraw cluster 2 番目大型、ObjectSkin.lastMatrixPalette と同 data 別 UBO export、bone animation frame swap trigger で ObjectSkin と同時 dirty、**data duplication 解消候補** (= ObjectSkin UBO 直接 consume への shader 改修 vs 維持) [要 AYA 判断]、cadence PerDraw → PerSkin 昇格検討候補 [要 AYA 判断]、setter 不明 [要追加調査]、velocity-enabled draw 限定判定 logic 不明 [要追加調査]、2 file consume (= skinnedVelocityV.glsl + skinnedVelocityAlphaV.glsl、blueprint `verified identical`)、工数 group 全体 L 内
**関連**: L0-1 dispatch (= set=2 binding=0 共有 6 UBO) / §3.5.8 sub-cluster (b) ObjectSkin (= data duplication 統合候補) / sub-cluster (a) AvatarVelocity (= avatar 版 lastMatrixPalette 同形パターン) / sub-cluster (c) VelocityAlphaV (= 同 velocity pipeline 経路)
