# PerDrawUBO_ObjectSkin — UBO design (= 実コードベース調査資料)

**通電状態**: **untouched** (= host C++ で `PerDrawUBO_ObjectSkin` への `writeDrawUbo` / `register*` 呼出ゼロ、grep 確認済 2026-06-06)

**本実装化に必要な作業**: 実 `matrixPalette[110]` + `lastMatrixPalette[110]` (= mat3x4 stride=48 × 110 joint、`LLMeshSkinInfo` 経由 bone matrix) を host から writeDrawUbo 経由で書込 + class1/avatar/objectSkinV.glsl 内 UBO consume へ切替

**異常 size 注記**: 10752 B (= std140 10560 B + device pad 192 B) = **全 UBO 中最大 size**、Skin_GLTFJoints (16384 B set=3 cadence=4) と並ぶ skin matrix 大型 UBO

---

## §1. UBO identity

- **block_name**: `PerDrawUBO_ObjectSkin`
- **block_hash**: `0x12c7004du` (= FNV-1a("PerDrawUBO_ObjectSkin"))
- **block_size**: 10752 B (= std140=10560 B, device-padded 10752 B)
- **member_count**: 2
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perdrawubo_objectskin.inl:12-16
struct PerDrawUBO_ObjectSkinLayout {
    static constexpr std::uint32_t matrixPalette_OFFSET = 0u;      // size=5280 align=16 stride=48
    static constexpr std::uint32_t lastMatrixPalette_OFFSET = 5280u; // size=5280 align=16 stride=48
};
inline constexpr std::uint32_t PerDrawUBO_ObjectSkin_SIZE = 10752u; // std140=10560, device-padded=10752
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_draw_ubo_object_skin.glsl:13-17
layout(std140, set = 2, binding = 0) uniform PerDrawUBO_ObjectSkin
{
    mat3x4 matrixPalette[110];
    mat3x4 lastMatrixPalette[110];
};
```

= **2 member = mat3x4[110] × 2 (= current + previous frame bone palette、velocity computation 用)**

**MAX_JOINTS_PER_MESH_OBJECT = 110** (= `indra/llcharacter/lljoint.h:48` `LL_MAX_JOINTS_PER_MESH_OBJECT`、`llviewershadermgr.cpp:870` で全 shader inject、blueprint は literal int 110 = per_draw_ubo_object_skin.glsl:7-9 header literal)

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 0
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC` (= set=2 UBO_DYNAMIC 帯)
- **pipeline layout**: `sAYAStandardLayout`
- **set=2 binding=0 共有 6 UBO の 1 名** (= name-based dispatch)
- **source**: ubo_metadata.inl:69 `{ "PerDrawUBO_ObjectSkin", 0x12c7004du, 10752u, 2u, 0u, 0u, 2u, 2u }`

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
  - `LLMeshSkinInfo` (= rigged mesh の bone binding 情報、memory `project_skin_hash_collision_bom_body.md` 参照)
  - `LLVOAvatar::mRiggedJointMatrix` (= 推定、grep verify 要)
  - 既存 OpenGL 経路で `objectSkinV.glsl` 用 `matrixPalette` uniform を書込む host site grep verify 要
- **lifetime**: per-draw call (= rigged mesh draw 毎)

**Skin_GLTFJoints との関係**:
- Skin_GLTFJoints (set=3 binding=2 cadence=4 size=16384) = GLTF asset 専用 per-Skin cadence
- PerDrawUBO_ObjectSkin (set=2 binding=0 cadence=2 size=10752) = legacy object 専用 per-draw cadence
- = **両 UBO は別 data source 別 cadence、host wiring 完全独立**

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_draw_ubo_object_skin.glsl`
  - source extract from `class1/avatar/objectSkinV.glsl:43` ifdef LL_VULKAN_GLSL block (single site)
- **実 shader use site** (= grep 結果):
  - `indra/newview/app_settings/shaders/class1/avatar/objectSkinV.glsl` (= single site、vertex shader)
- = **single shader file consume**

---

## §6. 既存 setter call site (host C++)

- **現状**: **PerDrawUBO_ObjectSkin 専用 setter 不在** (= grep 確認済 2026-06-06)
- **shell 段階通電経路**: 推定 = bringupTestUBO 経由 generic zero buffer 通電 (= 10752 B 大型ゆえ ring buffer 圧迫 risk verify 要)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL 経路で `matrixPalette` uniform を書込む host C++ site (= `LLVOAvatar::computeJointMatrices` 等候補、grep verify 要)
  - `lastMatrixPalette` (= previous frame palette) は別 cache 経由 (= velocity computation 用、verify 要)

---

## §7. 現状通電状態

- **状態**: **untouched** (= Phase 1.E 終了時点、host C++ で本 UBO 個別配線ゼロ)
- **通電 commit**: なし
- **通電内容**:
  - host C++ 側 `writeDrawUbo(PerDrawUBO_ObjectSkin, ...)` 呼出 0 件
  - shader 側 `class1/avatar/objectSkinV.glsl` で UBO consume block 既配置
- **shell 通電経路** (= 推定 / verify 要): 10752 B 大型 ゆえ bringupTestUBO 経路で通電可否 verify 要 (= ring buffer 容量制約)

---

## §8. 本実装化に必要な作業

1. **host C++ writeDrawUbo 配線** = rigged object draw 経路で `LLVKLoader::writeDrawUbo(ubo::block_hash::PerDrawUBO_ObjectSkin, 0u, palette_data, 10560, dynamic_offset)` 配線追加
2. **data source 特定** = `LLMeshSkinInfo` 経由 bone matrix + `LLVOAvatar` の現/前 frame palette cache
3. **mat3x4 packing** = mat3x4 std140 stride=48 (= 3 vec4 内 各 vec4 上 3 component) ゆえ host 側 mat4 → mat3x4 投影変換要 (= GLTF skin と同 packing)
4. **dirty 判定** = bone animation 毎 frame 更新 = per-frame dirty + per-rigged-draw 書込み
5. **lastMatrixPalette 管理** = 前 frame palette cache (= velocity computation 用、frame swap で current → last shift)
6. **shader 側 #else block 撤去** (= Phase 2+ 時のみ)

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | set=2 binding=0 共有 6 UBO | ✅ name-based dispatch 既配置 |
| OS-3 | std140 padding 厳守 + mat3x4 stride=48 | ✅ codegen 出力で stride=48 明示、110 joint × 48 B × 2 = 10560 B std140 |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既存 GLSL #ifdef LL_VULKAN_GLSL 既配置 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**異常 size risk**:
- 10752 B = **全 UBO 中最大** (= cadence=2 PerDraw 内最大、Skin_GLTFJoints 16384 B cadence=4 PerSkin に次ぐ)
- per-draw cadence × 10752 B = ring buffer 大量消費 risk (= rigged draw 多発 scene で sDrawUboRingBufferMgr 容量 verify 要)
- 解決策候補 = cadence 再分類 (= PerSkin cadence へ移行検討、Skin_GLTFJoints 同等 cadence 採用) = Phase 1.B 設計再検討 / verify 要

**maxUniformBufferRange 制約**:
- Vulkan spec で minimum guarantee = 16384 B (= 16 KiB)、本 UBO 10752 B は範囲内
- 但し dynamic offset alignment 要件で 256 B padding 必須 (= device-padded 10752 = 10496 + 256 align)

**mat3x4 packing 罠**:
- mat3x4 = column_major で 3 vec4 (= 各 vec4 上 3 component + 1 pad)
- host 側 mat4 → mat3x4 変換 = 平行移動 + 回転 + scale 投影、bottom row (0,0,0,1) drop

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **shell 通電可否** = 10752 B 大型 buffer が bringupTestUBO 経路で通電可能か verify 要 (= ring buffer 容量制約)
2. **既存 OpenGL setter call site** = `matrixPalette` uniform 書込 host C++ site (= `LLVOAvatar::computeJointMatrices` 等候補、grep verify 要)
3. **lastMatrixPalette cache 機構** = 前 frame palette cache の host 実装 (= verify 要、velocity computation 用)
4. **cadence 再分類検討** = PerDraw → PerSkin / PerProgram 移行検討 (= Phase 1.B 設計 verify 要、Skin_GLTFJoints 同等戦略)
5. **set=2 binding=0 共有 dispatch 詳細** = ObjectSkin vs ClipPlane / LightParams / AvatarSkin / SkinnedVelocity / AvatarVelocity の program 識別 logic
6. **LL_MAX_JOINTS_PER_MESH_OBJECT 110 verify** = `indra/llcharacter/lljoint.h:48` literal 再確認要 (= blueprint header 既宣言)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 4 binding、本 UBO は binding=0 (= 6 共有 UBO のうち 1)

### §11.2 同 cadence cluster UBO (= cadence_tag=2 PerDraw)

- PerDraw cluster 7 UBO (PerDrawUBO_ClipPlane §11.2 参照)
- **PerDrawUBO_ObjectSkin (本 UBO) = PerDraw cluster の最大 size UBO**

### §11.3 同 shader consume UBO

- `class1/avatar/objectSkinV.glsl` 内同時 consume = Frame_* set=0 帯 + set=1 Material + set=2 PerDraw 他 binding (= verify 要)

### §11.4 同 data source UBO

- **Skin_GLTFJoints (set=3 binding=2 cadence=4)** = 同 bone matrix 系だが別 cadence 別 set、GLTF 専用 vs Legacy 専用で完全分離
- **PerDrawUBO_SkinnedVelocity (set=2 binding=0 cadence=2 size=5376)** = lastMatrixPalette のみ抽出版 (= velocity computation 専用 shader 用)
- **PerDrawUBO_AvatarSkin (set=2 binding=0 cadence=2 size=768)** = avatar 専用 skin (= LL_MAX_JOINTS_PER_AVATAR 別値、verify 要)

### §11.5 dirty 連動 UBO

- 同 rigged mesh draw で PerDrawUBO_SkinnedVelocity (= 同 mesh の前 frame palette) と連動 dirty

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- per-draw cadence ゆえ毎 rigged draw call で `bindV3aRigged` 経由 set=2 帯 4 binding 同時 bind

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.8 同期)

**Layer**: L4-8 sub-cluster (b) (= object curr/prev pair + 抽出版 2 UBO、data duplication 解消候補)
**status**: **起案済** (= 2026-06-06 C-6-c、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.8` (= single source of truth)
**要点**: 2 member (matrixPalette[110] mat3x4 + lastMatrixPalette[110] mat3x4)、**10752 B = 全 UBO 中最大 size** (= cadence=2 PerDraw 内最大、Skin_GLTFJoints 16384 B PerSkin に次ぐ)、bone animation frame swap trigger で SkinnedVelocity と同期 dirty、**ring buffer 容量境界 risk** (= maxUniformBufferRange Vulkan minimum 16384B 範囲内だが境界近接) [要 verify]、**data duplication 罠** (= lastMatrixPalette ↔ SkinnedVelocity.lastMatrixPalette_skinned_velocity 同 data) [要 AYA 判断 = 統合 vs 維持]、cadence PerDraw → PerSkin 昇格検討候補 (= Skin_GLTFJoints 同等戦略) [要 AYA 判断]、setter 不明 [要追加調査]、MAX_JOINTS_PER_MESH_OBJECT=110 (= `lljoint.h:48`)、mat3x4 stride=48 packing 注意、工数 group 全体 L 内
**関連**: L0-1 dispatch (= set=2 binding=0 共有 6 UBO) / §3.5.8 sub-cluster (b) SkinnedVelocity (= data duplication 統合候補) / Skin_GLTFJoints (= 別 cadence 別 set GLTF 専用、本 UBO は Legacy 専用、完全独立) / memory `project_skin_hash_collision_bom_body` (= LLMeshSkinInfo mHash bone binding)
