# Asset_GLTFMaterials — UBO design (= 実コードベース調査資料)

**通電状態**: pilot 段階通電済 (= Phase 1.C PC-7γ-3 で per-asset cadence 経路 + GLTF host write 置換配線、Phase 1.D/E で real PBR shader 連動)

**本実装化に必要な作業**: shader 側 PBR material consume の Vulkan path 完成 (= 既存 `pbrmetallicroughnessV.glsl:66-82` + `pbrmetallicroughnessF.glsl:38-42` LL_VULKAN_GLSL block の実効化) + asset upload pipeline と本 UBO 通電の整合性 verify

---

## §1. UBO identity

- **block_name**: `Asset_GLTFMaterials`
- **block_hash**: `0xd0494ee1u` (= FNV-1a("Asset_GLTFMaterials"))
- **block_size**: 16384 B (= std140 16384 B、device-padded 16384 B、Vulkan 1.3 min UBO size = 16384 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_asset_gltfmaterials.inl:12-15
struct Asset_GLTFMaterialsLayout {
    static constexpr std::uint32_t gltf_material_data_OFFSET = 0u;  // size=16384 align=16 stride=16
};
inline constexpr std::uint32_t Asset_GLTFMaterials_SIZE = 16384u;   // std140=16384, device-padded=16384
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/asset_gltf_materials.glsl:15-19
layout(std140, set = 3, binding = 1) uniform Asset_GLTFMaterials
{
    // see class1/gltf/pbrmetallicroughnessV.glsl:66-82 for packing layout
    vec4 gltf_material_data[1024];
};
```

= **vec4[1024] array、PBR material data packing**、packing layout 詳細は `class1/gltf/pbrmetallicroughnessV.glsl:66-82` literal 参照 (blueprint コメント記載)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 1
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout` (= Phase 1.A 確立 5-set layout)
- **set 3 (Asset+Skin 帯)**: `llvkloader.cpp:862` literal: `V3A_ASSET_SET_BINDINGS = 3 // set=3: Asset_GLTFNodes + Asset_GLTFMaterials + Skin_GLTFJoints (X2-B sampler 除外)` = set=3 内 Asset 帯 binding=0/1 + Skin 帯 binding=2
- **本 UBO の位置**: V3A_ASSET_SET_BINDINGS 内 2 番目 (= binding=1) (= blueprint コメント記載)
- **source**: `ubo_metadata.inl:27` literal: `{ "Asset_GLTFMaterials", 0xd0494ee1u, 16384u, 3u, 1u, 0u, 3u, 1u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 3
- **意味**: **PerAsset** (= `llglslshader.cpp:97` literal: `constexpr U32 kCadencePerAsset = 3u; // 現 codegen 0 件、PC-7γ-2 defensive 通電 / PC-7γ-3 本格化`)
- **意味詳細**: GLTF asset 単位で update、`sCurrentAsset` (= `LLVKLoader::getCurrentAsset()`) 経由で current asset 解決 + `writeAssetUbo(asset, block_hash, offset, data, size)` で memcpy + dirty.store (= `llglslshader.cpp:2172-2195`)
- **source**: ubo_metadata.inl:27 + llglslshader.cpp:97 + llglslshader.cpp:2172-2195

---

## §4. 物理 owner

- **data source**: `LL::GLTF::Asset` (= GLTF asset の materials array、blueprint コメント記載「class1/gltf/pbrmetallicroughnessV.glsl:66-82 for packing layout」)
- **owner class**: `LL::GLTF::Asset` (= `LLVKLoader::getCurrentAsset()` accessor 経由)
- **既存 OpenGL 経路 writer**: **不明 / verify 要** (= gltf/asset.cpp updateMaterialData 等候補、grep verify 要)
- **lifetime**: GLTF asset load 〜 unload (= upload 時 runtime size を写し込む = blueprint コメント `runtime size at write` = PC-7γ-3 design-lock §4.3 `ASSET_MATERIALS_UPPER_BOUND_SIZE`)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/asset_gltf_materials.glsl`
- **実 shader use site** (= 実 PBR material consume):
  - `indra/newview/app_settings/shaders/class1/gltf/pbrmetallicroughnessV.glsl:66-82` (= packing layout 起源、blueprint source literal)
  - `indra/newview/app_settings/shaders/class1/gltf/pbrmetallicroughnessF.glsl:38-42` (= 同 block 定義 V/F 共有、blueprint コメント記載)

---

## §6. 既存 setter call site (host C++)

- **shader manager 上の UB enum**:
  - `llglslshader.h:171` literal: `UB_GLTF_MATERIALS, // "Asset_GLTFMaterials"`
  - `llglslshader.cpp:1963` literal: `"Asset_GLTFMaterials", // UB_GLTF_MATERIALS`
- **set 配線 const**: `llvkloader.cpp:862` literal: `V3A_ASSET_SET_BINDINGS = 3 // set=3: Asset_GLTFNodes + Asset_GLTFMaterials + Skin_GLTFJoints`
- **forwardToUboUpload PerAsset case**: `llglslshader.cpp:2172-2195` literal で `LL::GLTF::Asset* asset = LLVKLoader::getCurrentAsset()` → `LLVKLoader::writeAssetUbo(asset, loc.block_hash, loc.offset, data, size)` (= block_hash 経由 generic 経路)
- **本 UBO 名指 setter**: 不明 / verify 要 (= host C++ には block 名 string `"Asset_GLTFMaterials"` のみ存在、個別 setter 名は generic write 経路に統合)

---

## §7. 現状通電状態

- **状態**: **pilot 段階通電済** (= Phase 1.C PC-7γ-3 で per-asset cadence 通電)
- **PC-7γ-2**: defensive 配線 (= cadence_tag=3 block 0 件で実走無し)
- **PC-7γ-3**: GLTF host write 置換 + codegen Asset_* block 追加 + lifecycle hook 配線で hot path 通電 (= `llglslshader.cpp:2178-2180` literal)
- **Phase 1.D / 1.E**: real PBR shader connection 進行中 (= memory `project_ayastorm_r41_vulkan_migration` Phase 1.E complete marker handoff doc 参照)
- **通電 commit**: 不明 / verify 要 (= PC-7γ-3 完了 commit chain 参照要)

---

## §8. 本実装化に必要な作業

1. **PBR shader consume の Vulkan path 完成**:
   - `pbrmetallicroughnessV.glsl:66-82` + `pbrmetallicroughnessF.glsl:38-42` LL_VULKAN_GLSL block 活性化
2. **asset upload pipeline 整合**:
   - `gltf/asset.cpp` updateMaterialData (= 推定、verify 要) の `writeAssetUbo` 配線確認
   - upper bound at register (16384 B) + runtime size at write (= G5-A1 規約、blueprint コメント記載) の動作確認
3. **per-asset cadence flush 経路 verify**:
   - `sCurrentAsset` accessor 経路が PBR draw 直前で set されること
   - flush は `sAssetUboSetV3a` triple-buffer 経路 (= `llvkloader.cpp:5729-5840` literal)
4. **set=3 swap 確認**:
   - rigged draw 経路で set=2 skip → set=3 swap (= `llvkloader.cpp:2174` literal) と非衝突

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=3 Asset 帯収納 | ✅ 維持 |
| OS-3 | std140 padding 厳守 + vec4 array stride=16 | ✅ 厳守 (codegen 出力) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ⚠️ pbrmetallicroughness V/F に LL_VULKAN_GLSL block 追加済 = AYAstorm 改変、upstream merge conflict risk |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**upper bound at register / runtime size at write**:
- blueprint 宣言は std140 array 上限 1024 vec4 (= 16384 B) で固定、実 buffer 確保は runtime size を写し込む (= G5-A1: blueprint コメント記載)
- viewer `#define MAX_UBO_VEC4S` は `gGLManager.mMaxUniformBlockSize/16` runtime 上限 (= Vulkan 1.3 min 16384 B → 1024 vec4、OpenGL 65536 B → 4096 vec4)

**Vulkan vs OpenGL upper bound 差分**:
- Vulkan min 16384 B / 16 = 1024 vec4
- OpenGL min 65536 B / 16 = 4096 vec4
- Vulkan path では asset の material count が 1024 vec4 packed entry 上限を超える asset で truncate / split risk = verify 要

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守)

1. **既存 OpenGL 経路 writer 特定** = gltf/asset.cpp 内 updateMaterialData (= 推定) の具体 call site
2. **packing layout 詳細** = `pbrmetallicroughnessV.glsl:66-82` の packing 仕様 (= vec4 単位 packed material 構造、material_id index 解決経路)
3. **runtime size at write 経路** = `writeAssetUbo` 内で runtime size の伝達経路
4. **PC-7γ-3 完了 commit hash** = pilot 通電 commit
5. **PBR shader Vulkan path 実効化 phase** = Phase 1.E 完了後の persistence 確認要
6. **asset 切替 timing と sCurrentAsset accessor** の race condition 有無

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3 Asset 帯)

`llvkloader.cpp:862` literal `V3A_ASSET_SET_BINDINGS = 3` 内訳:
- **Asset_GLTFNodes** (set=3 binding=0)
- **Asset_GLTFMaterials (set=3 binding=1、本 UBO)**
- **Skin_GLTFJoints** (set=3 binding=2、cadence=PerSkin、Skin 帯)

### §11.2 同 cadence cluster UBO (= cadence_tag=3 PerAsset)

ubo_metadata.inl 上 cadence_tag=3 は 2 件 (= `llglslshader.cpp:97` literal「現 codegen 0 件、PC-7γ-3 で本格化」):
- **Asset_GLTFNodes** (set=3 binding=0)
- **Asset_GLTFMaterials (本 UBO)**

= 同 cadence cluster は 2 UBO のみ、`writeAssetUbo` generic 経路で flush

### §11.3 同 shader consume UBO

- `pbrmetallicroughnessV.glsl` 内同時 consume:
  - **Asset_GLTFNodes** (= 同 file 内 set=3 binding=0)
  - 他 set=0 (FrameViewProj 等) + set=2 PerDraw 系: 不明 / verify 要
- `pbrmetallicroughnessF.glsl` 内同時 consume: 不明 / verify 要

### §11.4 同 data source UBO

- **Asset_GLTFNodes** (= 同じ GLTF asset 由来、`LL::GLTF::Asset` から派生)

### §11.5 dirty 連動 UBO

- **Asset_GLTFNodes** (= 同 asset 由来ゆえ asset 切替時に同時 dirty 推定、verify 要)
- **Skin_GLTFJoints** = skin と material は別 lifecycle (= verify 要、推定では asset hierarchical structure ゆえ skin 単独切替時は本 UBO 不変)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- set=3 帯 bind は asset draw 直前に `bindV3aStatic` / `bindV3aRigged` 経路 (= `llvkloader.cpp:2168, 2172` literal)
- set=3 全帯一括 bind (= Asset binding=0/1 + Skin binding=2 同時 bind)
- triple-buffer 経路で frame in flight 単位 update (= `sAssetUboSetV3a × FRAMES_IN_FLIGHT (=3)`、`llvkloader.cpp:905` literal)

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.14 同期)

**Layer**: L4-14 (= GLTF asset 2 UBO pair、Phase 3 R4 メインターゲット)
**status**: **起案済** (= 2026-06-06 C-6-i、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.14` (= single source of truth)
**要点**: 1 member (gltf_material_data vec4[1024])、**16384B = Vulkan 1.3 min UBO size**、**PerAsset cadence cluster 唯一 2 件の 1 つ** (= ubo_metadata.inl cadence_tag=3、`llglslshader.cpp:97` literal)、**pilot 段階通電済** (= Phase 1.C PC-7γ-3、`llglslshader.cpp:2172-2195` literal writeAssetUbo generic 経路)、UB_GLTF_MATERIALS enum + `"Asset_GLTFMaterials"` block 名 string 登録、binding=1 衝突 = SkyV (PerProgram) [要 verify L0-1]、**Phase 3 R4 メインターゲット = per-asset 本実装 + 実 PBR shader 接続**、MAX_UBO_VEC4S = 1024 vec4 (= material count > 1024 で truncate/split risk) [要 verify + 要 AYA 判断]、packing `pbrmetallicroughnessV.glsl:66-82` + F:38-42 2 file consume、updateMaterialData setter 不明 [要追加調査]、upper bound at register / runtime size at write G5-A1 規約、triple-buffer (= sAssetUboSetV3a × FRAMES_IN_FLIGHT=3)、工数 group 全体 M 内
**関連**: L0-1 dispatch (= binding=1 衝突 SkyV (PerProgram) と PerAsset cadence 別経路) / §3.5.14 sibling Asset_GLTFNodes (= 同 LL::GLTF::Asset 由来、asset 切替時同時 dirty) / §3.5.7 sub-cluster (c) SkyV (= binding=1 衝突解消) / Phase 3 R4 = per-asset 本実装 + 実 PBR shader 接続
