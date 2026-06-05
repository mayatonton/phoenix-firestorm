# Asset_GLTFNodes — UBO design (= 実コードベース調査資料)

**通電状態**: pilot 段階通電済 (= Phase 1.C PC-7γ-3 で per-asset cadence 経路 + GLTF host write 置換配線、Phase 1.D/E で real PBR shader 連動)

**本実装化に必要な作業**: shader 側 GLTF node transform consume の Vulkan path 完成 (= 既存 `pbrmetallicroughnessV.glsl:335-338` LL_VULKAN_GLSL block の実効化) + asset upload pipeline (= node transforms array upload) と本 UBO 通電の整合性 verify

---

## §1. UBO identity

- **block_name**: `Asset_GLTFNodes`
- **block_hash**: `0x6e78dce2u` (= FNV-1a("Asset_GLTFNodes"))
- **block_size**: 16384 B (= std140 16384 B、device-padded 16384 B、Vulkan 1.3 min UBO size = 16384 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_asset_gltfnodes.inl:12-15
struct Asset_GLTFNodesLayout {
    static constexpr std::uint32_t gltf_nodes_OFFSET = 0u;  // size=16384 align=16 stride=16
};
inline constexpr std::uint32_t Asset_GLTFNodes_SIZE = 16384u;  // std140=16384, device-padded=16384
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/asset_gltf_nodes.glsl:14-17
layout(std140, set = 3, binding = 0) uniform Asset_GLTFNodes
{
    vec4 gltf_nodes[1024];
};
```

= **vec4[1024] array、GLTF node transform data (推定 mat4 単位 packing、48B align = 1 node)**、`MAX_NODES_PER_GLTF_OBJECT = gGLManager.mMaxUniformBlockSize/48` (= Vulkan 1.3 min 16384 B → 341 nodes、blueprint コメント記載)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 0
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **set 3 (Asset+Skin 帯)**: `llvkloader.cpp:862` literal: `V3A_ASSET_SET_BINDINGS = 3 // set=3: Asset_GLTFNodes + Asset_GLTFMaterials + Skin_GLTFJoints`
- **本 UBO の位置**: V3A_ASSET_SET_BINDINGS 内 1 番目 (= binding=0) (= blueprint コメント記載)
- **source**: `ubo_metadata.inl:28` literal: `{ "Asset_GLTFNodes", 0x6e78dce2u, 16384u, 3u, 0u, 0u, 3u, 1u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 3
- **意味**: **PerAsset** (= `llglslshader.cpp:97` literal: `constexpr U32 kCadencePerAsset = 3u`)
- **意味詳細**: GLTF asset 単位で update、`sCurrentAsset` accessor 経由 `writeAssetUbo` 経路で memcpy + dirty.store
- **source**: ubo_metadata.inl:28 + llglslshader.cpp:97 + llglslshader.cpp:2172-2195

---

## §4. 物理 owner

- **data source**: `LL::GLTF::Asset` (= GLTF asset の node transforms array、blueprint コメント記載「pbrmetallicroughnessV.glsl:335-338」)
- **owner class**: `LL::GLTF::Asset` (= `LLVKLoader::getCurrentAsset()` accessor 経由)
- **既存 OpenGL 経路 writer**: **不明 / verify 要** (= gltf/asset.cpp updateNodeData (= 推定) 等候補、grep verify 要)
- **lifetime**: GLTF asset load 〜 unload + node animation 更新時 (= per-asset cadence、frame 内複数回 write 可能性、verify 要)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/asset_gltf_nodes.glsl`
- **実 shader use site**:
  - `indra/newview/app_settings/shaders/class1/gltf/pbrmetallicroughnessV.glsl:335-338` (= blueprint source literal)
- **既存 OpenGL 経路では**: 同 file 内 `#else` block で uniform 個別宣言 (= verify 要、確認推奨)

---

## §6. 既存 setter call site (host C++)

- **shader manager 上の UB enum**:
  - `llglslshader.h:170` literal: `UB_GLTF_NODES, // "Asset_GLTFNodes"`
  - `llglslshader.cpp:1962` literal: `"Asset_GLTFNodes", // UB_GLTF_NODES`
- **set 配線 const**: `llvkloader.cpp:862` literal (= 同 V3A_ASSET_SET_BINDINGS 内)
- **forwardToUboUpload PerAsset case**: `llglslshader.cpp:2172-2195` literal (= Asset_GLTFMaterials と同経路、block_hash 経由 generic dispatch)
- **本 UBO 名指 setter**: 不明 / verify 要 (= host C++ には block 名 string `"Asset_GLTFNodes"` のみ存在)

---

## §7. 現状通電状態

- **状態**: **pilot 段階通電済** (= Phase 1.C PC-7γ-3)
- **PC-7γ-2**: defensive 配線 (= cadence_tag=3 block 0 件時)
- **PC-7γ-3**: GLTF host write 置換 + codegen Asset_* block 追加 + lifecycle hook 配線 (= `llglslshader.cpp:2178-2180` literal)
- **Phase 1.D / 1.E**: real PBR shader connection 進行中
- **通電 commit**: 不明 / verify 要

---

## §8. 本実装化に必要な作業

1. **GLTF node transform consume の Vulkan path 完成**:
   - `pbrmetallicroughnessV.glsl:335-338` LL_VULKAN_GLSL block 活性化
2. **asset upload pipeline 整合**:
   - `gltf/asset.cpp` updateNodeData (= 推定) の `writeAssetUbo` 配線
   - upper bound at register (16384 B) + runtime size at write (= G5-A1 規約)
3. **node animation update 経路**:
   - frame 内 node transform update → `writeAssetUbo` → dirty.store → flush triple-buffer
4. **per-asset cadence flush 経路 verify** (= Asset_GLTFMaterials と共通経路)
5. **set=3 swap 確認** (= rigged draw 経路で set=2 skip → set=3 swap、`llvkloader.cpp:2174`)

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 (= set=3 binding=0) |
| OS-3 | std140 padding 厳守 + vec4 array stride=16 | ✅ 厳守 |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ⚠️ pbrmetallicroughnessV.glsl に LL_VULKAN_GLSL block 追加済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**upper bound vs runtime size**:
- Vulkan min 16384 B / 16 = 1024 vec4
- Vulkan min 16384 B / 48 = 341 nodes (= blueprint コメント記載 MAX_NODES_PER_GLTF_OBJECT)
- OpenGL min 65536 B / 48 = 1365 nodes
- Vulkan path で node count > 341 の asset で truncate / split risk = verify 要

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守)

1. **既存 OpenGL 経路 writer 特定** = gltf/asset.cpp 内 updateNodeData の具体 call site (推定)
2. **node packing layout 詳細** = `pbrmetallicroughnessV.glsl:335-338` の packing 仕様 (= mat4 単位 packed transform / node_id 解決経路、48B align ゆえ vec3 = 3 vec4)
3. **PC-7γ-3 完了 commit hash**
4. **node count 上限超過 asset の処理方針** (= 341 nodes 超過時 truncate or split)
5. **node animation update cadence** (= per-frame か per-asset か、verify 要)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3 Asset 帯)

- **Asset_GLTFNodes (set=3 binding=0、本 UBO)**
- **Asset_GLTFMaterials** (set=3 binding=1)
- **Skin_GLTFJoints** (set=3 binding=2、cadence=PerSkin)

### §11.2 同 cadence cluster UBO (= cadence_tag=3 PerAsset)

ubo_metadata.inl 上 cadence_tag=3 は 2 件:
- **Asset_GLTFNodes (本 UBO)**
- **Asset_GLTFMaterials**

### §11.3 同 shader consume UBO

- `pbrmetallicroughnessV.glsl` 内同時 consume:
  - **Asset_GLTFMaterials** (set=3 binding=1)
  - 他 set=0 / set=2 UBO: 不明 / verify 要

### §11.4 同 data source UBO

- **Asset_GLTFMaterials** (= 同じ GLTF asset 由来、`LL::GLTF::Asset` から派生)

### §11.5 dirty 連動 UBO

- **Asset_GLTFMaterials** (= 同 asset 由来ゆえ asset 切替時に同時 dirty 推定、verify 要)
- **Skin_GLTFJoints** = skin 切替時のみ単独 dirty、本 UBO は不変 (= 推定、verify 要)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- set=3 帯 bind は asset draw 直前 `bindV3aStatic` / `bindV3aRigged` で全帯一括 (= `llvkloader.cpp:2168, 2172`)
- triple-buffer 経路 (= `sAssetUboSetV3a × FRAMES_IN_FLIGHT (=3)`、`llvkloader.cpp:905`)
- node update が frame 内 multi-pass で発生する場合 multi-update 経路要 (= verify 要)
