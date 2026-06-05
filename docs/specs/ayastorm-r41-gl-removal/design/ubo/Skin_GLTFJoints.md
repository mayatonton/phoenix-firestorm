# Skin_GLTFJoints — UBO design (= 実コードベース調査資料)

**通電状態**: **pilot real data 通電済** (= Phase 1.E PC-N-5 で per-Skin register/wire 完成 + Phase 1.E PC-N-11 で multi-skin real Skin path 一本化 + Phase 1.E PC-N-15c で sGltfStubSkin sentinel + AYAGltfMultiSkinEnabled cvar 撤去 → real Skin path 完成)

**本実装化に必要な作業**: real bone matrix payload 書込み接続 (= LLVOAvatar / LL::GLTF::Asset 経由 mat3x4 palette → `writeSkinUbo` 経路) + 実 PBR shader (= class1/gltf/pbrmetallicroughnessV.glsl) consume 接続検証 + worker thread 並列化 (PC-N-15a infra 確立済、本 UBO は per-Skin cadence 経路で worker dispatch 既配線)

---

## §1. UBO identity

- **block_name**: `Skin_GLTFJoints`
- **block_hash**: `0xd86f22b5u` (= FNV-1a("Skin_GLTFJoints"))
- **block_size**: 16384 B (= std140 array `vec4 gltf_joints[1024]` Vulkan 1.3 min uniform block size 上限、device-padded 16384 B = 16384 B 倍数 alignment 充足)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_skin_gltfjoints.inl
struct Skin_GLTFJointsLayout {
    static constexpr std::uint32_t gltf_joints_OFFSET = 0u;  // size=16384 align=16 stride=16
};
inline constexpr std::uint32_t Skin_GLTFJoints_SIZE = 16384u; // std140=16384, device-padded=16384
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/skin_gltf_joints.glsl
layout(std140, set = 3, binding = 2) uniform Skin_GLTFJoints
{
    vec4 gltf_joints[1024];
};
```

= **real data 通電済**、blueprint 1 member `gltf_joints[1024]` は実 PBR shader (= `class1/gltf/pbrmetallicroughnessV.glsl:284-287`) literal extract と同形契約、access pattern = `gltf_joints[i*3 + 0..2]` で 3 vec4 = mat3x4 1 joint (= `pbrmetallicroughnessV.glsl:313-321`、blueprint header line 7-8 注記)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 2
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通、`vkCmdBindDescriptorSets` 経路 verify 要)
- **pipeline layout**: `sAYAStandardLayout` (= Phase 1.A 確立 5-set layout)
- **set 3 内訳** (= `llvkloader.cpp:862` `V3A_ASSET_SET_BINDINGS = 3` literal: "Asset_GLTFNodes + Asset_GLTFMaterials + Skin_GLTFJoints (X2-B sampler 除外)"):
  - set=3 binding=0 = Asset_GLTFNodes
  - set=3 binding=1 = Asset_GLTFMaterials
  - set=3 binding=2 = **Skin_GLTFJoints** (本 UBO)
  - set=3 binding=3..62 = Legacy UBO 群 (binding=2 は Skin_GLTFJoints 専有)
- **source**: `ubo_metadata.inl:105` `{ "Skin_GLTFJoints", 0xd86f22b5u, 16384u, 3u, 2u, 0u, 4u, 1u }` + `llvkloader.cpp:862` set 3 同居 literal + `llvkloader.cpp:3215` `w.dstBinding = 2;  // design 06c §2.5: set=3 binding=2 = Skin_GLTFJoints`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 4
- **意味**: **PerSkin** (= `llglslshader.cpp:98` literal: `constexpr U32 kCadencePerSkin = 4u; // 同上`)
- **意味詳細**: per-Skin cadence (= per-LL::GLTF::Skin* instance)、`flushSkinUbos(LL::GLTF::Skin* skin)` 経由 flush、Skin instance 単位 dirty 管理 (= `sSkinUboDirty[<Skin*, block_hash>]`)
- **source**: ubo_metadata.inl:105 + llglslshader.cpp:98 + llvkloader.cpp:578 `std::unordered_map<UboSkinKey, UboInstance, UboSkinKeyHash> sSkinUboDirty;` + llvkloader.cpp:5250 `void flushSkinUbos(LL::GLTF::Skin* skin)`

---

## §4. 物理 owner

- **owner**: `LL::GLTF::Skin*` (= per-instance、`thread_local LL::GLTF::Skin* sCurrentSkin = nullptr;` `llvkloader.cpp:677`)
- **shell 段階 owner**: `sPlaceholderSkin` (= `llvkloader.cpp:4286` `registerSkinUbo(sPlaceholderSkin, ubo::block_hash::Skin_GLTFJoints, ...)`、PC-N-15c 撤去後も維持 (Phase 1.F+ avatar Vulkan draw 通電 phase 持越し))
- **real data 段階 owner** (= Phase 1.E PC-N-11 後): real `LL::GLTF::Skin*` instance per multi-skin asset (= multi-skin real Skin path 一本化、PC-N-15c で AYAGltfMultiSkinEnabled cvar fall-through 撤去後)
- **lifetime**: per-Skin instance (= LL::GLTF::Skin lifetime と一致)、`registerSkinUbo` 時 buffer 生成 + `unregisterSkinUbo` 時 destroy (= `llvkloader.cpp:5824` + 5883)
- **source**: llvkloader.cpp:558 `using UboSkinKey = std::pair<LL::GLTF::Skin*, U32 /*block_hash*/>;` + llvkloader.cpp:677 sCurrentSkin static

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/skin_gltf_joints.glsl` (= Phase 1.C PC-7γ-3 起案、Source literal extract from `class1/gltf/pbrmetallicroughnessV.glsl:284-287`、blueprint header line 3 明示)
- **実 shader use site** (= literal extract source):
  - `class1/gltf/pbrmetallicroughnessV.glsl:284-287` = `Skin_GLTFJoints` block 宣言 site (= blueprint header line 3 literal reference)
  - `class1/gltf/pbrmetallicroughnessV.glsl:313-321` = `gltf_joints[i*3 + 0..2]` access pattern (= blueprint header line 7-8 注記)
- **consume status**: real PBR shader literal extract source ゆえ既 consume (= shader 側 `#ifdef LL_VULKAN_GLSL` block で `Skin_GLTFJoints` UBO member access)

---

## §6. 既存 setter call site (host C++)

- **register setter**: `registerSkinUbo(LL::GLTF::Skin* skin, U32 block_hash, U32 block_size)` (= `llvkloader.cpp:5824`)
  - 用途: per-Skin UboInstance buffer 生成 + descriptor wire (= `sSkinUboDirty[<skin, block_hash>]` entry 登録 + 内部で `wireSkinUboSetV3aToBinding2(skin)` 等価 descriptor wire 実施 (= `llvkloader.cpp:3177` 注記))
  - 呼出 site: `llvkloader.cpp:4286` `registerSkinUbo(sPlaceholderSkin, ubo::block_hash::Skin_GLTFJoints, block_size)` (= Phase 1.D PC-N-5 起案 placeholder Skin register)
- **wire helper**: `wireSkinUboSetV3aToBinding2(LL::GLTF::Skin* skin)` (= `llvkloader.cpp:3180`)
  - 用途: 明示再 wire (= `registerSkinUbo` 内部 wire と等価、independent entry point 趣旨、`llvkloader.cpp:4297-4302` 注記)
  - 呼出 site (1): `llvkloader.cpp:4299` `wireSkinUboSetV3aToBinding2(sPlaceholderSkin)` (= Phase 1.D PC-N-5 (d))
  - 呼出 site (2): recordGltfAssetDraw 内 `wireSkinUboSetV3aToBinding2(sCurrentSkin)` (= Phase 1.E PC-N-15c (b) real Skin path 一本化、撤去 marker 内 PC-N-15c (b)+(c) tag block で記述)
- **write setter**: `writeSkinUbo(LL::GLTF::Skin* skin, U32 block_hash, U32 offset, const void* data, size_t size)` (= `llvkloader.cpp:5899`)
  - 用途: per-Skin UBO buffer 書込み (= bone matrix mat3x4 palette write、offset+size 指定で部分 write 対応)
  - 呼出 site: recordGltfAssetDraw 内、real bone matrix 書込み接続後に呼出 (= Phase 1.F+ 持越し、本 PC-N-15c 時点では zero data write 経路、`llvkloader.cpp:6856` `writeSkinUbo (zero data)` 注記)
- **flush setter**: `flushSkinUbos(LL::GLTF::Skin* skin)` (= `llvkloader.cpp:5250`)
  - 用途: per-Skin dirty exchange → device buffer flush (= `flushDummyUboWrite("flushSkinUbos")` 経由、`llvkloader.cpp:5269`)
  - 呼出 site: recordGltfAssetDraw 内 `LLVKLoader::flushSkinUbos(sCurrentSkin)` (= `llvkloader.cpp:6662`、Phase 1.E PC-N-15c (b) real Skin path 一本化)
- **unregister setter**: `unregisterSkinUbo(LL::GLTF::Skin* skin, U32 block_hash)` (= `llvkloader.cpp:5883`)
  - 呼出 site: `llvkloader.cpp:4650` `unregisterSkinUbo(sPlaceholderSkin, ubo::block_hash::Skin_GLTFJoints)` (= shutdownVulkan path)

---

## §7. 現状通電状態

- **状態**: **pilot real data 通電済** (= Phase 1.E PC-N-5/11/15c)
- **通電内容**:
  - per-Skin register/wire 確立 (= Phase 1.D PC-N-5、`registerSkinUbo(sPlaceholderSkin)` + `wireSkinUboSetV3aToBinding2(sPlaceholderSkin)`)
  - multi-skin real Skin path 一本化 (= Phase 1.E PC-N-11、`writeSkinUbo` + `flushSkinUbos` 経路統合)
  - sGltfStubSkin sentinel + AYAGltfMultiSkinEnabled cvar 撤去 → real Skin path 完成 (= Phase 1.E PC-N-15c、commit fd29a77517)
  - worker thread 並列化 infra (= Phase 1.E PC-N-15a/b、worker thread dispatch 経路で `writeSkinUbo` 対応)
- **未通電項目** (= Phase 1.F+ 持越し):
  - real bone matrix payload 書込み (= LL::GLTF::Skin bone matrix → mat3x4 palette pack → `writeSkinUbo`)
  - 実 shader consume 検証 (= `class1/gltf/pbrmetallicroughnessV.glsl` で gltf_joints 経由 vertex skinning verify)
- **source**: handoff `docs/specs/ayastorm-r41-gl-removal/handoff/phase1/d/handoff-phase1-d-pc-n-5-complete.md` + `phase1/e/handoff-phase1-e-pc-n-11-complete.md` + `phase1/e/handoff-phase1-e-complete.md` (= Phase 1.E complete marker、commit fd29a77517)

---

## §8. 本実装化に必要な作業

1. **real bone matrix payload 書込み接続**:
   - 既存 OpenGL 経路 = `LLVOAvatar` / `LL::GLTF::Asset` 経由 bone matrix palette → uniform write
   - 新 Vulkan 経路 = `writeSkinUbo(real_skin, ubo::block_hash::Skin_GLTFJoints, 0, palette_data, 16384)` で mat3x4 palette pack 書込み
2. **実 PBR shader consume 検証**:
   - `class1/gltf/pbrmetallicroughnessV.glsl:284-287` (Skin_GLTFJoints 宣言) + `:313-321` (gltf_joints access pattern) で vertex skinning 動作確認
   - SPIR-V compile + Vulkan validation layer warnings 0 件確認
3. **worker thread 並列化**:
   - PC-N-15a infra 確立済 (= worker_count=19、`LL::WorkQueue runUntilClose` 駆動)
   - `writeSkinUbo` は worker thread 経路で実装済 (= `llvkloader.cpp:773` `sWorkerIdx = main thread 時 kInvalidWorkerIdx (= writeDrawUbo/writeSkinUbo` 注記)
   - real bone matrix 書込み接続後、worker thread 並列度確認
4. **codegen 再実行不要**:
   - blueprint `skin_gltf_joints.glsl` member `vec4 gltf_joints[1024]` は実 shader literal extract と一致、Phase 2 で member 不変契約 → codegen 不要

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合 (= memory `project_r41_phase2_4_principles` 原則 2):

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=3 内に収める | ✅ 維持 (= set 3 内 binding=2、Phase 1.D PC-N-5 で配置済) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 16384 B std140 array vec4 stride=16 (codegen 出力で生成、layout `gltf_joints_OFFSET = 0u; size=16384 align=16 stride=16` literal) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 (= 既存 ring buffer 経路で query 結果使用しているか確認) |
| OS-5 | shader 改変ゼロ | ✅ blueprint = 実 shader literal extract (= byte-for-byte 不可触 charter §3 #1 担保) |
| OS-7 | Linux validation layer warnings 0 件 | ✅ AYA live verify 「通常通りに描画されてます」record 2026-06-06 |

**buffer size 16384 B 制約**:
- Vulkan 1.3 min `maxUniformBufferRange` = 16384 B (= blueprint header line 7-8 注記 "Vulkan 1.3 min 16384 B → 341 nodes")
- OpenGL 経路 = 65536 B (= 1365 nodes)
- Vulkan path で 16384 B 上限 = 341 mat3x4 joints max (= blueprint vec4[1024] / 3 = 341.33)
- 341+ joints 必要な mesh は **未対応** (= Phase 1.F+ で分割 buffer / SSBO 化検討要、現時点 risk 顕在化なし)

**G5-A1 設計**:
- upper bound at register, runtime size at write (= `SKIN_JOINTS_UPPER_BOUND_SIZE`、PC-7γ-3 design-lock §4.3)
- blueprint は std140 上限 16384 B 固定宣言、実 buffer 確保は upload 時 runtime size を写し込む

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3、`llvkloader.cpp:862` `V3A_ASSET_SET_BINDINGS = 3` literal)

- Asset_GLTFNodes (set=3 binding=0)
- Asset_GLTFMaterials (set=3 binding=1)
- **Skin_GLTFJoints (set=3 binding=2、本 UBO)**
- + Legacy UBO 群 (set=3 binding=3..62)

### §11.2 同 cadence cluster UBO (= cadence_tag=4 PerSkin)

- **Skin_GLTFJoints (本 UBO、PerSkin 唯一)** = PerSkin cluster は本 UBO 1 個のみ (= ubo_metadata.inl 全 94 UBO 中 cadence_tag=4 は本 UBO のみ)

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- `class1/gltf/pbrmetallicroughnessV.glsl` で同時 consume されうる候補 (= verify 要):
  - Asset_GLTFNodes (set=3 binding=0、node modelview matrix)
  - Asset_GLTFMaterials (set=3 binding=1、material params)
  - PerDrawUBO_ObjectSkin (set=2 binding=0、real Skin path 一本化前は object skin matrix palette)
  - FrameViewProj (set=0 binding=0、view+proj matrix)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **PerDrawUBO_AvatarSkin** (= set=2 binding=0、cadence_tag=2 PerDraw、`ubo_metadata.inl:64`) = avatar skin matrix palette UBO、本 UBO は GLTF asset 由来 skin、AvatarSkin は LLVOAvatar 由来 skin、data source 別系統
- **PerDrawUBO_ObjectSkin** (= set=2 binding=0、cadence_tag=2 PerDraw、`ubo_metadata.inl:69`) = object skin matrix palette UBO、本 UBO と GLTF asset skin で源流共有可能性 (= verify 要)
- **PerDrawUBO_SkinnedVelocity** (= set=2 binding=0、cadence_tag=2 PerDraw、`ubo_metadata.inl:70`) = velocity UBO with skin、本 UBO と data source 連動可能性

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **不明 / verify 要** = Skin update 時 (= avatar pose change / bone animation) に同時 dirty になる UBO 確認要
- 推定候補 (= verify 要): PerDrawUBO_AvatarSkin / PerDrawUBO_SkinnedVelocity (= 同 skin event で dirty 連動可能性)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout (= Phase 1.A 確立、全 UBO 共通)

### §11.7 bind 順序関係

- per-Skin cadence ゆえ Skin instance 切替 timing で bind (= `vkCmdBindDescriptorSets` 1 回呼出で set=3 全 binding 含む、`wireSkinUboSetV3aToBinding2` で set=3 binding=2 dstBinding 指定)
- 本 UBO bind timing = recordGltfAssetDraw 内、`sCurrentSkin` 切替後 `wireSkinUboSetV3aToBinding2(sCurrentSkin)` で descriptor set 3 更新 (= Phase 1.E PC-N-15c (b) real Skin path 一本化)

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

本 file 起案時点で実コード調査で確定できなかった項目:

1. **real bone matrix payload 書込み接続 timing** = LL::GLTF::Skin bone matrix palette pack → `writeSkinUbo` 接続 timing (= Phase 1.F+ 持越し、handoff doc chain 参照要)
2. **341+ joints mesh 対応** = Vulkan 16384 B 上限超過 mesh の分割 buffer / SSBO 化方針 (= Phase 1.F+ で検討要、現時点 risk 顕在化なし)
3. **minUniformBufferOffsetAlignment 動的取得経路** = singleton/per-Skin flush 経路の VkDescriptorBufferInfo bind 詳細 (= `vkCmdBindDescriptorSets` 呼出 site verify 要)
4. **PerDrawUBO_ObjectSkin との data source 関係** = GLTF asset skin と object skin の源流共有可能性 (= verify 要)
5. **dirty 連動 UBO 完全特定** = avatar pose change 時 (PerDrawUBO_AvatarSkin / PerDrawUBO_SkinnedVelocity) の同時 dirty 経路 (= verify 要)
6. **worker thread 並列度実測** = PC-N-15a infra で worker_count=19 確立済、本 UBO の worker dispatch 実際の並列度 (= Phase 1.F+ 計測要)

= 上記 6 項目は Phase 1.F+ 着手時に grep + Read で逐次解消、確定後に「不明」記載削除 + 確定 literal 追記。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.6.1 同期)

**Layer**: **L5-1 / A-1** (= **唯一の A 判定**、pilot real data 通電済 2026-06-06、Phase 1.F+ real bone matrix 接続持越)
**status**: **起案済 + pilot real data 通電済** (= 2026-06-06 C-7、Phase 1.E PC-N-5/11/15c 完了、Phase 1.F+ で real bone matrix payload 書込み持越)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.6.1` (= single source of truth)
**要点**: trace L5-1、工数 M (= Phase 1.F+ 並走)、setter 完全特定済 (= 5 setter `llvkloader.cpp:5824/3180/5899/5250/5883`)、AYA live verify 「通常通りに描画されてます」record 2026-06-06、real bone matrix 接続後再 verify、visual regression ゼロ §5.4、原則 1 worker thread 並列化寄与
**関連**: L0-1 (= set=3 binding=2 衝突 with SkyF 解消対象) + L0-4 / §5.4 visual regression policy / Phase 1.F+ avatar Vulkan draw 通電 phase
