# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.D **PC-N-8 design-lock** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: Phase 1.D 内 **3rd sub-step (PC-N-8)** = 実 `LL::GLTF::Asset` 経由 vertex/index buffer Vulkan infrastructure 新設 + register/write asset.cpp 側 + `recordGltfAssetDraw` 実 Asset path 配線 の **design-lock 完了 marker**。ambiguity (N8-1)..(N8-14) 14 件 AYA literal「全件推奨で進めてもらえますか?」record (2026-06-05) + 実装計画 (a)-(g) 7 step 分解 + Exit Criteria 9 (design-lock) + 10 (実装 phase) 項明文化。`indra/` 改変 0 件 (= `feedback_design_phase_no_code_write` 厳格遵守)。

> **本 doc 位置付け**: PC-N-7 complete (= commit `70e5faafff`) の継続 design-lock phase。Phase 1.D decomposition design-lock (= commit `44c81ea228`) §4.3 で PC-N-8 = "material/transform UBO 実 bind 配線" と概略起案されていたが、AYA 起案文「実 `LL::GLTF::Asset` 経由 vertex/index buffer Vulkan infrastructure 新設 + register/write asset.cpp 側」が **literal scope** 確定。decomposition §4.3 概略は古い、本 doc literal scope が source of truth。

---

## §0. 本 session 着手契機 + literal scope record

**契機**: AYA 指示「r41 Phase 1.D PC-N-8 design-lock 着手お願いします」literal 受領 (2026-06-05、PC-N-7 complete commit `70e5faafff` 後の継続 session = 別 session の fresh context) + 必読 1 件 (PC-N-7 complete doc) 全文 Read + pinpoint reference 4 件 Read (= Phase 1.D decomposition §4.2-4.3 + cross-platform spec §6 PC-N-8 行 + `recordGltfAssetDraw` 現状 + `asset.cpp` PC-7γ-3 dual-write 配線) + Explore agent 経由 10 項現状調査 → ambiguity (N8-1)..(N8-14) 14 件 + 推奨案 + 採用根拠提示 → AYA literal「全件推奨で進めてもらえますか?」一括確認受領 (2026-06-05) で本 design-lock doc 起案。

**PC-N-8 literal scope** (= AYA 起案文 literal、本 design-lock で確定):

1. **実 `LL::GLTF::Asset` 経由 vertex buffer Vulkan infrastructure 新設** ((N8-2) A per-Primitive ownership、(N8-3) A position vec3 only)
2. **実 `LL::GLTF::Asset` 経由 index buffer Vulkan infrastructure 新設** ((N8-8) A UINT32)
3. **register/write asset.cpp 側 = per-Primitive vertex/index buffer ownership lifecycle** ((N8-5) B per-Primitive API、(N8-10) A lazy register on first upload)
4. **register/unregister API 新設 6 件** = `registerPrimitiveVertexBuffer` + `writePrimitiveVertexBuffer` + `unregisterPrimitiveVertexBuffer` + 同形 index = 6 件 ((N8-5) B)
5. **`Primitive::uploadVulkanBuffers()` 新設関数** = `Asset::uploadTransforms` 同形 dual-write pattern で raw `mPositions` (position vec3 only) + `mIndexArray` upload ((N8-4) A direct copy)
6. **`recordGltfAssetDraw` 内 real Asset path 配線** = signature 不変 ((N8-6) A) + `sCurrentAsset` + `sCurrentPrimitive` natural guard ((N8-7) A cvar 新設 0 件、(N8-12) A static 経路) + `sGltfStubAssetPipeline` 再利用 ((N7-8) A 継承) + 実 vertex/index buffer bind + `vkCmdDrawIndexed`
7. **`sCurrentPrimitive` static + setter/getter 新設** = PC-N-9 で `GLTFSceneManager::render` が set/clear する事前準備 ((N8-12) A)

**Phase 境界**: PC-N-8 単独で実発火なし (= `sCurrentAsset == nullptr` + `sCurrentPrimitive == nullptr` natural guard、PC-N-8 build verify は llrender PASS + WARNING 0 + integrity 検証のみ)、PC-N-9 で `GLTFSceneManager::render` 統合 + `AYAGltfRealDrawEnabled` cvar gate 配線 → real Asset draw 自動発火 → PC-N-10 cleanup。

---

## §1. 必読 1 件 + pinpoint reference

**次 session 必読 (= PC-N-8 実装 phase 着手前)**:

1. **本 PC-N-8 design-lock doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-d-pc-n-8-design-lock.md`

**pinpoint reference (PC-N-8 実装 phase 着手時に必要分のみ Read)**:

- **PC-N-7 complete doc**: `handoff-...-phase1-d-pc-n-7-complete.md` = 直前 sub-step pattern source (= step (a)/(c)/(d)/(e)/(e') 5 site 構造 + `sGltfStubAssetPipeline` 再利用 (N7-8) A + bindIndexBufferVk wrap signature)
- **`Asset::uploadTransforms` 内 PC-7γ-3 dual-write 配線**: `indra/newview/gltf/asset.cpp:213-231` = lazy register on first upload pattern (= `LLVKLoader::registerAssetUbo(this, ...)` + `LLVKLoader::writeAssetUbo(this, ...)`) source-of-truth
- **`Asset` dtor 内 unregister 対称配線**: `indra/newview/gltf/asset.cpp:138-150` = `unregisterAssetUbo` 対称 lifecycle pattern source
- **`LLVKLoader::registerAssetUbo` / `writeAssetUbo` 実装**: `indra/llrender/llvkloader.cpp:5463-5571` = key type `UboAssetKey<Asset*, block_hash>` + storage `sAssetUboDirty` + `allocateUboInstanceBuffers` + `sAssetUboSetV3a` descriptor wire pattern (= PC-N-8 で per-Primitive 同形に新設する際の reference)
- **`recordGltfAssetDraw` 現状 PC-N-7 (e) cvar 分岐**: `indra/llrender/llvkloader.cpp:5840-6146` = 4 段 cvar 分岐 (PC-N-7 / PC-N-6 / PC-N-5 fallback) + UBO + Skin 配線 + `bindV3aRigged` + push const + bindVertexBufferVk + bindIndexBufferVk + vkCmdDrawIndexed pattern
- **`Primitive` クラス定義**: `indra/newview/gltf/primitive.h:41-116` = `mPositions` / `mNormals` / `mTangents` / `mIndexArray` field 配置 + `mVertexBuffer` (OpenGL LLVertexBuffer*) + `mVertexOffset` / `mIndexOffset` pattern
- **`Primitive::upload` 実装**: `indra/newview/gltf/primitive.cpp:540-597` = OpenGL setData 経路 (= position/normal/tangent/uv0/uv1/weight/joint sequential setData)、Vulkan upload は別関数として並走配置 source
- **`sCurrentAsset` / `sCurrentSkin` static + accessor**: `indra/llrender/llvkloader.cpp:700-701` + `gltfscenemanager.cpp:697/755/777/783` = setter/clear pattern (= PC-N-8 で `sCurrentPrimitive` 同形 static 新設の reference)
- **`bindVertexBufferVk` / `bindIndexBufferVk` wrap signature**: `indra/llrender/llvkloader.h:287-294` + `llvkloader.cpp:6559-6584` = 引数 + guard pattern
- **`PrimitiveVulkanBuffer` 想定 struct 比較対象 = `UboInstance` struct**: `indra/llrender/llvkloader.cpp` 内 UboInstance struct = per-instance buffer + alloc + mapped + size pattern (= PC-N-8 で per-Primitive 同形 struct 新設の reference)
- **cross-platform spec §6**: `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md` = PC-N-8 行 design-lock 内容更新 (本 commit 内)
- **GATE-B literal**: memory `project_r41_phase1b_vulkan_host_gate` = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件維持
- **設計原則**: memory `project_ayastorm_r41_design_principles` = (1) Upstream OpenGL 取り込みやすさ維持 + (2) Core プロセス分散実現

---

## §2. 現状調査結果 (= Explore agent 経由 10 項網羅)

### §2.1 code 8 項

| # | 項目 | file:line | 要約 |
|---|------|-----------|------|
| 1 | `Asset` / `Mesh` / `Primitive` 構造 | `asset.h:353-468` + `primitive.h:41-116` | Asset 内 `mNodesUBO` (U32) / `mMaterialsUBO` (U32) + `mRenderData[2]` per-variant batch、Primitive 内 `mPositions/mNormals/mTangents` (CPU std::vector<LLVector4a>) + `mIndexArray` (std::vector<U32>) + `LLPointer<LLVertexBuffer> mVertexBuffer` (OpenGL shared) + `mVertexOffset/mIndexOffset` |
| 2 | `Primitive::upload(LLVertexBuffer*)` 実装 | `primitive.cpp:540-597` | OpenGL LLVertexBuffer mapped region へ sequential setData 列 (position/normal/tangent/uv0/uv1/weight/joint)、Y-flip texcoord on-the-fly、`setIndexData(index_array.data(), mIndexOffset, getIndexCount())` で index 書き込み |
| 3 | `Mesh::prep()` / upload 起点 | `asset.cpp:1455-1466` | 各 primitive を iterate して `.prep(asset)` 委譲のみ、実 vertex buffer 構築は GLTFSceneManager / Asset prep 経路 |
| 4 | `LLVKLoader::registerAssetUbo` / `writeAssetUbo` | `llvkloader.h:476-478` + `llvkloader.cpp:5463-5571` | `(Asset*, block_hash, block_size)` lazy register、key=UboAssetKey、storage=sAssetUboDirty unordered_map、allocateUboInstanceBuffers + sAssetUboSetV3a descriptor wire、write は memcpy + dirty.store |
| 5 | `LLVKLoader::registerSkinUbo` / `writeSkinUbo` | `llvkloader.h:479-481` + `llvkloader.cpp:5573-5670` | Asset 対称形 (Skin*, block_hash=Skin_GLTFJoints)、Skin::uploadMatrixPalette lazy register + dtor unregister 対称 lifecycle |
| 6 | `recordGltfAssetDraw` 現状 (PC-N-7 complete) | `llvkloader.cpp:5840-6146` | signature `(VkCommandBuffer cmd_buf)` 不変、Asset/Skin 識別は `sCurrentAsset` / `sCurrentSkin` static 経由、4 段 cvar 分岐 (PC-N-7 / PC-N-6 / PC-N-5 fallback)、3 cvar 優先順位 PC-N-7 > PC-N-6 > PC-N-5 + PC-N-5 fallback (`sAvatarBonePipeline` shader-generated) |
| 7 | `bindVertexBufferVk` / `bindIndexBufferVk` wrap | `llvkloader.h:287-294` + `llvkloader.cpp:6559-6584` | `(cmd_buf, buffer, offset[, index_type])`、cmd_buf/buffer VK_NULL_HANDLE 時 no-op graceful degrade |
| 8 | `Asset` dtor 内 unregister 対称配線 | `asset.cpp:138-150` | `glDeleteBuffers(mNodesUBO)` + `LLVKLoader::unregisterAssetUbo(this, Asset_GLTFNodes)` + 同形 Materials、dual-unregister 対称破棄 |

### §2.2 asset.cpp + GLTFSceneManager 2 項

| # | 項目 | file:line | 要約 |
|---|------|-----------|------|
| 9 | `Asset::uploadTransforms` PC-7γ-3 dual-write | `asset.cpp:164-231` | (k) lazy register on first upload (`if (mNodesUBO == 0)`) + (h) dual-write defensive (`LLVKLoader::writeAssetUbo(this, ...)`) unconditional、Vulkan 未初期化時 sAllocator guard で no-op |
| 10 | `GLTFSceneManager::render(asset, variant)` 構造 | `gltfscenemanager.cpp:640-786` | shader bind → `setCurrentAsset(&asset)` (line 697) → `flushAssetUbos(&asset)` (line 702) → OpenGL UBO bind → per-Primitive loop (line 722-779) で `setCurrentSkin(&skin)` (line 755) → `flushSkinUbos(&skin)` (line 759) → `primitive.mVertexBuffer->drawRangeFast()` → loop 末尾 `clearCurrentSkin()` → loop 終了後 `clearCurrentAsset()`、PC-N-9 で並走 Vulkan dispatcher 追加余地 |

### §2.3 Asset インスタンス管理 + sCurrentPrimitive 新設根拠

- Asset registry 不在 (= GLTFSceneManager / caller 所有)、`Asset*` pointer 識別 = `UboAssetKey<Asset*, block_hash>` unique
- `sCurrentAsset` / `sCurrentSkin` static は `llvkloader.cpp:700-701` 既存、GLTFSceneManager::render が set/clear
- PC-N-8 で `sCurrentPrimitive` 同形 static 新設 = `recordGltfAssetDraw` 内 real Asset path で参照、PC-N-9 で `GLTFSceneManager::render` が per-Primitive loop 内 set/clear する事前準備

---

## §3. ambiguity (N8-1)..(N8-14) 14 件 AYA literal「全件推奨で進めてもらえますか?」record (2026-06-05) + 採用根拠

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N8-1) PC-N-8 literal scope 確定境界 | **B**: infrastructure 新設 + asset.cpp 側 register/write + `recordGltfAssetDraw` 内 real Asset path 配線 (= sCurrentAsset != nullptr natural guard、cvar 新設 0 件、PC-N-9 で GLTFSceneManager::render 統合時に自動発火) | 全件推奨 (2026-06-05) | AYA literal「実 vertex/index buffer bind + drawIndexed」整合、`AYAGltfRealDrawEnabled` cvar 新設は PC-N-9 scope ゆえ PC-N-8 では新設 0 件、`feedback_ubo_migration_one_at_a_time` 厳格遵守、PC-N-9 食い込み回避 |
| (N8-2) ownership 粒度 | **A**: per-Primitive ownership (= Primitive* identity、storage は LLVKLoader 内 unordered_map<Primitive*, PrimitiveVulkanBuffer>) | 全件推奨 (2026-06-05) | AYA literal scope「per-Primitive ownership lifecycle」明記、Primitive 粒度が PC-N-9 以降 multi-Primitive 並走時の最小単位、Primitive lifecycle と対称、設計原則 (2) Core プロセス分散実現整合、`Primitive` 構造体への Vulkan field 直接追加は不要 (= layering 維持、`gltf/primitive.h` から `vulkan/vulkan.h` include 回避) = storage は LLVKLoader 内 encapsulate |
| (N8-3) vertex data layout 最小スコープ | **A**: position vec3 only (= stride 12 B、PC-N-7 stub layout 同形維持) | 全件推奨 (2026-06-05) | `feedback_ubo_migration_one_at_a_time` 厳格遵守、PC-N-8 = 実 Asset データ取込最小単位 sub-step、PC-N-7 stub baseline 拡張、`sGltfStubAssetPipeline` vertex input state (binding=0 stride=12 location=0 R32G32B32_SFLOAT) 再利用ゆえ shader 改変ゼロ + PSO 改変なし、normal/tangent/uv/joint は PC-N-9 以降段階拡張 |
| (N8-4) raw vertex data source | **A**: `Primitive::mPositions` (std::vector<LLVector4a>) 直接 copy | 全件推奨 (2026-06-05) | 各 attribute 個別 std::vector が source-of-truth、LLVertexBuffer は OpenGL 専用 path、Vulkan も同形 CPU data 直接構築、layer 違反なし、(N8-3) A 整合で position vec3 のみ抽出 |
| (N8-5) register/write API signature 新設方式 | **B**: per-Primitive 形 API = `registerPrimitiveVertexBuffer(Primitive*, size_t)` + `writePrimitiveVertexBuffer(Primitive*, offset, data, size)` + `unregisterPrimitiveVertexBuffer(Primitive*)` + 同形 index = 6 件 | 全件推奨 (2026-06-05) | (N8-2) A per-Primitive ownership 整合、Primitive* 識別 key、PC-7γ-3 同形 pattern、API consistency、ownership lifecycle 対称、unregister 対称形、block_hash 概念は vertex/index buffer に不要 (= UBO の std140 layout 由来ゆえ生 buffer に不該当) |
| (N8-6) `recordGltfAssetDraw` signature 拡張形 | **A**: signature 不変 = `(VkCommandBuffer cmd_buf)`、Asset/Primitive 識別は `sCurrentAsset` + `sCurrentPrimitive` static 経由 | 全件推奨 (2026-06-05) | AYA literal「signature 拡張 = Asset* / Primitive* 受領」と緊張ありだが、design 06b §5.2 sCurrentAsset/sCurrentSkin pattern 既存踏襲、call site 単純、stub 経路と signature 統一温存、設計原則 (1) Upstream OpenGL 取り込みやすさ維持、PC-N-9 で GLTFSceneManager::render 統合時 `setCurrentAsset` + `setCurrentPrimitive` 経路で自動受け渡し、AYA literal「全件推奨」一括確認受領で本 A 案確定 |
| (N8-7) real Asset draw cvar gate | **A**: cvar 新設 0 件 + sCurrentAsset != nullptr + sCurrentPrimitive != nullptr natural guard (= PC-N-8 単独発火なし、PC-N-9 で `AYAGltfRealDrawEnabled` 正式新設) | 全件推奨 (2026-06-05) | Phase 1.D decomposition §4.4 で `AYAGltfRealDrawEnabled` は PC-N-9 scope 明記、PC-N-8 で cvar 新設 0 件 + settings.xml 改変 0 件、natural guard で発火経路分離、PC-N-9 で GLTFSceneManager::render 統合時 set 経路で自動発火 |
| (N8-8) index format | **A**: `VK_INDEX_TYPE_UINT32` | 全件推奨 (2026-06-05) | PC-N-7 (N7-7) B precedent 継承、`mIndexArray = std::vector<U32>` source-of-truth 整合、forward compat |
| (N8-9) VMA usage flag | **A**: PC-N-6/7 stub 同形 = `VMA_MEMORY_USAGE_AUTO` + `HOST_ACCESS_SEQUENTIAL_WRITE_BIT` + `MAPPED_BIT` (host-visible 永続 mapped) | 全件推奨 (2026-06-05) | PC-N-6/7 stub 同形 pattern、1 Primitive prototype scope では staging overkill、`feedback_ubo_migration_one_at_a_time` 厳格遵守で device-local + staging 移行は Phase 1.E 以降 worker thread phase で対応 |
| (N8-10) register/upload cadence + site | **A**: lazy register on first upload (= PC-7γ-3 (k)/(h) 同形 pattern)、新 site = `Primitive::uploadVulkanBuffers()` 新設関数 = `Asset::uploadTransforms` 末尾並走 hook で Asset 内全 Primitive iterate + uploadVulkanBuffers() call、各 Primitive で lazy register + write (Vulkan 未初期化時 sAllocator guard で no-op) | 全件推奨 (2026-06-05) | PC-7γ-3 (k)/(h) lazy register pattern 踏襲、ownership 明確、layer 違反なし、MUSEUBO-A 整合 (= mUseUBO=false default で OpenGL 描画 100% 維持、Vulkan 未初期化時 no-op)、cadence は per-Asset (= uploadTransforms と同形) |
| (N8-11) Primitive 粒度 (1 個 vs 全個) | **A**: 最小 1 Primitive 発火スコープ (= PC-N-8 単独では 1 Primitive のみ draw 発火想定だが、upload は全 Primitive iterate で host-visible buffer 構築 = upload は cheap memcpy、draw 発火は PC-N-9 で sCurrentPrimitive set 経路で開始) | 全件推奨 (2026-06-05) | `feedback_ubo_migration_one_at_a_time` 厳格遵守、PC-N-7 stub draw 1 triangle baseline 拡張、multi-Primitive ループは PC-N-9 GLTFSceneManager::render 統合時 per-Primitive loop で自然拡張、register/write API は per-Primitive ゆえ multi-Primitive 拡張は同 API 経路で自動対応 |
| (N8-12) Asset* / Primitive* source 経路 | **A**: `sCurrentAsset` + `sCurrentPrimitive` static 経路 (= PC-N-8 で `sCurrentPrimitive` static 新設 + setter/getter/clear 配線、GLTFSceneManager::render は PC-N-9 で set) | 全件推奨 (2026-06-05) | (N8-6) A 整合、design 06b §5.2 sCurrentAsset/sCurrentSkin pattern 拡張、`sCurrentPrimitive` を `llvkloader.cpp` 内 anonymous namespace に新設 + `setCurrentPrimitive(Primitive*)` / `clearCurrentPrimitive()` / `getCurrentPrimitive()` 配線 (= PC-N-9 で GLTFSceneManager::render が set) |
| (N8-13) build verify scope | **A**: llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 (= PC-N-7 同形、Linux primary marker 採用後標準) | 全件推奨 (2026-06-05) | PC-N-6/7 同形 scope、(N7-12) A precedent 継承、Linux primary marker 採用後標準 |
| (N8-14) Exit Criteria 数 + step 分解粒度 | **A**: Exit Criteria 10 項 + step (a)-(g) 7 step (= PC-N-6/7 同形 template) | 全件推奨 (2026-06-05) | PC-N-6/7 pattern 踏襲、consistency |

---

## §4. 実装計画 (a)-(g) 7 step ((N8-14) A 採用)

### §4.1 (a) LLVKLoader 内 storage struct + map 新設

**配置**: `indra/llrender/llvkloader.cpp` anonymous namespace 内、PC-N-7 (a) `sGltfStubIndexBuffer` storage block 直後並列。

**code stub example**:

```cpp
// <AYAstorm r41 PC-N-8 (a)> per-Primitive Vulkan vertex/index buffer storage 新設 (= UboInstance 同形 struct)。
struct PrimitiveVulkanBuffer
{
    VkBuffer        buffer        = VK_NULL_HANDLE;
    VmaAllocation   allocation    = VK_NULL_HANDLE;
    void*           mapped        = nullptr;
    U32             size_bytes    = 0u;
    U32             element_count = 0u;  // vertex_count or index_count
};

static std::unordered_map<LL::GLTF::Primitive*, PrimitiveVulkanBuffer> sPrimitiveVertexBuffers;
static std::unordered_map<LL::GLTF::Primitive*, PrimitiveVulkanBuffer> sPrimitiveIndexBuffers;
// </AYAstorm r41 PC-N-8 (a)>
```

### §4.2 (b) LLVKLoader API 新設 6 件 (= llvkloader.h + .cpp) ((N8-5) B)

**配置**: `indra/llrender/llvkloader.h` `registerAssetUbo` / `writeAssetUbo` / `unregisterAssetUbo` declare 直後並列。

**code stub example (.h)**:

```cpp
// <AYAstorm r41 PC-N-8 (b)> per-Primitive vertex/index buffer API 新設 ((N8-5) B、PC-7γ-3 同形 pattern)。
static bool registerPrimitiveVertexBuffer(LL::GLTF::Primitive* primitive, U32 size_bytes);
static void writePrimitiveVertexBuffer(LL::GLTF::Primitive* primitive, U32 offset, const void* data, U32 size);
static void unregisterPrimitiveVertexBuffer(LL::GLTF::Primitive* primitive);

static bool registerPrimitiveIndexBuffer(LL::GLTF::Primitive* primitive, U32 size_bytes);
static void writePrimitiveIndexBuffer(LL::GLTF::Primitive* primitive, U32 offset, const void* data, U32 size);
static void unregisterPrimitiveIndexBuffer(LL::GLTF::Primitive* primitive);
// </AYAstorm r41 PC-N-8 (b)>
```

**code stub example (.cpp registerPrimitiveVertexBuffer)**:

```cpp
// <AYAstorm r41 PC-N-8 (b)> per-Primitive vertex buffer register (lazy on first call)。
bool LLVKLoader::registerPrimitiveVertexBuffer(LL::GLTF::Primitive* primitive, U32 size_bytes)
{
    if (sAllocator == VK_NULL_HANDLE) { return false; }  // (1/5) graceful degrade
    if (primitive == nullptr || size_bytes == 0u)         { return false; }  // (2/5)
    if (sPrimitiveVertexBuffers.find(primitive) != sPrimitiveVertexBuffers.end()) { return true; }  // (3/5) already registered

    PrimitiveVulkanBuffer entry;
    entry.size_bytes = size_bytes;

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size        = size_bytes;
    buffer_info.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_create_info{};
    alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                            | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo alloc_info{};
    VkResult res = vmaCreateBuffer(sAllocator, &buffer_info, &alloc_create_info,
                                   &entry.buffer, &entry.allocation, &alloc_info);
    if (res != VK_SUCCESS) { LL_WARNS_ONCE("AYAStormR41PCN8") << "..." << LL_ENDL; return false; }  // (4/5)
    if (alloc_info.pMappedData == nullptr) { vmaDestroyBuffer(sAllocator, entry.buffer, entry.allocation); return false; }  // (5/5)

    entry.mapped = alloc_info.pMappedData;
    sPrimitiveVertexBuffers.emplace(primitive, entry);
    return true;
}
// </AYAstorm r41 PC-N-8 (b)>
```

`registerPrimitiveIndexBuffer` は usage flag `VK_BUFFER_USAGE_INDEX_BUFFER_BIT` のみ差分。`writePrimitiveVertexBuffer` / `writePrimitiveIndexBuffer` は mapped pointer に memcpy。`unregisterPrimitiveVertexBuffer` / `unregisterPrimitiveIndexBuffer` は `vmaDestroyBuffer` + map erase。

### §4.3 (c) `Primitive::uploadVulkanBuffers()` 新設関数 ((N8-10) A)

**配置**: `indra/newview/gltf/primitive.h` declare + `primitive.cpp` 実装、`Primitive::upload(LLVertexBuffer*)` (line 540-597) 直後並列。

**code stub example (.h)**:

```cpp
// <AYAstorm r41 PC-N-8 (c)> per-Primitive Vulkan vertex/index buffer upload (= LLVKLoader register/write 経路)。
void uploadVulkanBuffers();
// </AYAstorm r41 PC-N-8 (c)>
```

**code stub example (.cpp)**:

```cpp
// <AYAstorm r41 PC-N-8 (c)> per-Primitive Vulkan vertex/index buffer upload ((N8-10) A lazy)。
void Primitive::uploadVulkanBuffers()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_GLTF;

    // vertex buffer = position vec3 only ((N8-3) A、(N8-4) A direct copy)
    if (!mPositions.empty())
    {
        // pack to vec3 array (= 12 B per vertex、LLVector4a 16 B → 12 B 抽出)
        std::vector<F32> packed_positions;
        packed_positions.reserve(mPositions.size() * 3u);
        for (const LLVector4a& p : mPositions)
        {
            packed_positions.push_back(p.getF32ptr()[0]);
            packed_positions.push_back(p.getF32ptr()[1]);
            packed_positions.push_back(p.getF32ptr()[2]);
        }
        const U32 v_size = (U32)(packed_positions.size() * sizeof(F32));
        if (LLVKLoader::registerPrimitiveVertexBuffer(this, v_size))
        {
            LLVKLoader::writePrimitiveVertexBuffer(this, 0, packed_positions.data(), v_size);
        }
    }

    // index buffer = UINT32 ((N8-8) A、mIndexArray std::vector<U32> direct copy)
    if (!mIndexArray.empty())
    {
        const U32 i_size = (U32)(mIndexArray.size() * sizeof(U32));
        if (LLVKLoader::registerPrimitiveIndexBuffer(this, i_size))
        {
            LLVKLoader::writePrimitiveIndexBuffer(this, 0, mIndexArray.data(), i_size);
        }
    }
}
// </AYAstorm r41 PC-N-8 (c)>
```

**呼出 hook 配置**: `Asset::uploadTransforms` 末尾 (= asset.cpp:231 直後)、PC-7γ-3 (h) `writeAssetUbo` 配線直後並列に追加。

**code stub example (asset.cpp:231 直後)**:

```cpp
// <AYAstorm r41 PC-N-8 (c-hook)> Asset 内全 Primitive の Vulkan buffer lazy upload ((N8-10) A、PC-7γ-3 同形 cadence)。
for (Mesh& mesh : mMeshes)
{
    for (Primitive& primitive : mesh.mPrimitives)
    {
        primitive.uploadVulkanBuffers();
    }
}
// </AYAstorm r41 PC-N-8 (c-hook)>
```

### §4.4 (d) `Primitive` dtor 内 unregister 対称配線

**配置**: `indra/newview/gltf/primitive.cpp` `Primitive::~Primitive()` (dtor) 内、`Asset::~Asset()` (asset.cpp:138-150) と同形 pattern。

**code stub example**:

```cpp
// <AYAstorm r41 PC-N-8 (d)> per-Primitive Vulkan buffer 対称 unregister ((N8-2) A lifecycle 対称)。
LLVKLoader::unregisterPrimitiveVertexBuffer(this);
LLVKLoader::unregisterPrimitiveIndexBuffer(this);
// </AYAstorm r41 PC-N-8 (d)>
```

**注意**: `Primitive` に既存 dtor がない場合は新設要、その際 default move/copy semantics に注意 (= unordered_map storage key は Primitive* 自身ゆえ、move/copy 時に key が変わる)。**design ambiguity 残**: dtor の copy/move semantics 取扱は実装 phase で literal 確認要 (= 追加 ambiguity 候補 marker)。

### §4.5 (e) `sCurrentPrimitive` static + setter/getter/clear 新設 ((N8-12) A)

**配置**: `indra/llrender/llvkloader.cpp` `sCurrentAsset` / `sCurrentSkin` static (line 700-701) 直後並列 + `.h` accessor declare。

**code stub example (.cpp)**:

```cpp
// <AYAstorm r41 PC-N-8 (e)> sCurrentPrimitive static 新設 (= PC-N-9 で GLTFSceneManager::render が set/clear)。
static LL::GLTF::Primitive* sCurrentPrimitive = nullptr;
// </AYAstorm r41 PC-N-8 (e)>
```

**code stub example (.h)**:

```cpp
// <AYAstorm r41 PC-N-8 (e)> sCurrentPrimitive accessor ((N8-12) A)。
static void setCurrentPrimitive(LL::GLTF::Primitive* primitive);
static void clearCurrentPrimitive();
static LL::GLTF::Primitive* getCurrentPrimitive();
// </AYAstorm r41 PC-N-8 (e)>
```

**accessor 実装**: setter/clear は static assignment、getter は static return。**GLTFSceneManager::render での set/clear は PC-N-9 scope、PC-N-8 では accessor declare + storage 配置のみ**。

### §4.6 (f) `recordGltfAssetDraw` 内 real Asset path 配線

**配置**: `indra/llrender/llvkloader.cpp` `recordGltfAssetDraw` 内、**PC-N-7 (e) tag block 直前並列**配置 (= cvar 優先順位 PC-N-8 real Asset > PC-N-7 > PC-N-6 > PC-N-5)。

**code stub example**:

```cpp
// <AYAstorm r41 PC-N-8 (f)> real Asset draw path (= sCurrentAsset + sCurrentPrimitive natural guard、cvar 新設 0 件 (N8-7) A)。
{
    LL::GLTF::Asset*     current_asset     = sCurrentAsset;
    LL::GLTF::Primitive* current_primitive = sCurrentPrimitive;

    if (current_asset != nullptr && current_primitive != nullptr
        && sGltfStubAssetPipeline != VK_NULL_HANDLE)  // pipeline 再利用 ((N7-8) A 継承)
    {
        auto vb_it = sPrimitiveVertexBuffers.find(current_primitive);
        auto ib_it = sPrimitiveIndexBuffers.find(current_primitive);
        if (vb_it != sPrimitiveVertexBuffers.end()
            && ib_it != sPrimitiveIndexBuffers.end()
            && vb_it->second.buffer != VK_NULL_HANDLE
            && ib_it->second.buffer != VK_NULL_HANDLE)
        {
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, sGltfStubAssetPipeline);

            // PC-N-6/7 同形 UBO 配線 (writeDrawUbo + writeSkinUbo + flushSkinUbos + bindV3aRigged + push const identity)
            // ... (PC-N-7 (e) tag block 同形)

            bindVertexBufferVk(cmd_buf, vb_it->second.buffer, 0);
            bindIndexBufferVk(cmd_buf, ib_it->second.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd_buf, ib_it->second.element_count, 1u, 0u, 0, 0u);

            static bool sFirstFire = true;
            if (sFirstFire)
            {
                sFirstFire = false;
                LL_INFOS("AYAStormR41PCN8") << "PC-N-8 (f) GLTF real Asset path 通電 (first fire): asset=" << (void*)current_asset
                                            << " primitive=" << (void*)current_primitive
                                            << " vertex_count=" << vb_it->second.element_count
                                            << " index_count=" << ib_it->second.element_count
                                            << LL_ENDL;
            }
            return;  // PC-N-7 / PC-N-6 / PC-N-5 経路 skip (= 4 cvar gate で並走分離)
        }
    }
}
// </AYAstorm r41 PC-N-8 (f)>
```

**5 段 graceful degrade**: sAllocator nullptr / sCurrentAsset nullptr / sCurrentPrimitive nullptr / map entry 不在 / VkBuffer VK_NULL_HANDLE = 各段で fallthrough (= PC-N-7 経路 → PC-N-6 → PC-N-5 fallback)。

### §4.7 (g) build verify + handoff complete doc 起案

**build verify literal 取得対象** ((N8-13) A):

- `make -j4 llrender` → PASS / ERROR 0 / WARNING 0
- `INTEGRATION_TEST_lluboringbuffer` → 11/11 PASS
- `INTEGRATION_TEST_llassetubopool` → 10/10 PASS
- `INTEGRATION_TEST_llpipelinecachestorage` → 13/13 PASS
- `cd scripts/ubo_codegen && python3 -m unittest discover tests` → 131/131 OK
- `grep -c "LL_VULKAN_GLSL" indra/llrender/llvkloader.cpp` → 6 (= PC-N-7 commit `70e5faafff` 時点と同数、GATE-B integrity)

### §4.8 GATE-B 整合

`#ifdef LL_VULKAN_GLSL` 新規追加 0 件 = memory `project_r41_phase1b_vulkan_host_gate` 遵守。新 API 6 件 + storage struct + map + sCurrentPrimitive accessor は全て runtime LLVKLoader 内 encapsulate、cvar 新設 0 件、settings.xml 改変 0 件。

### §4.9 MUSEUBO-A 整合

`mUseUBO=false` default で OpenGL 描画 100% 維持 (= 既存 PC-7γ-3 dual-write pattern と同形)。Vulkan 未初期化時 `sAllocator == VK_NULL_HANDLE` guard で全 register/write 関数が no-op。`sCurrentAsset` / `sCurrentPrimitive` は GLTFSceneManager が PC-N-9 で set するまで nullptr ゆえ recordGltfAssetDraw 内 real Asset path 発火なし = **PC-N-8 単独で発火経路ゼロ**。5 段 graceful degrade (sAllocator / sCurrentAsset / sCurrentPrimitive / map entry / VkBuffer) 全段で fallthrough。

### §4.10 設計原則整合

- **(1) Upstream OpenGL 取り込みやすさ維持**: `recordGltfAssetDraw` signature 不変 ((N8-6) A)、`Primitive` 構造体への Vulkan field 直接追加なし ((N8-2) A storage は LLVKLoader 内 encapsulate)、`GLTFSceneManager::render` 改変 0 件、shader 改変ゼロ、PSO 改変なし (`sGltfStubAssetPipeline` 再利用 (N7-8) A 継承)。
- **(2) Core プロセス分散実現**: per-Primitive ownership ((N8-2) A) で multi-Primitive 並走時の最小単位確立、PC-N-9 以降の per-Primitive worker thread 分離余地確保、register/write API は Primitive* 単位ゆえ thread-safe 化は Phase 1.E で同 API 経路で対応可。

### §4.11 想定改変 file 5 件 (= 実装 phase = 別 session)

| # | file | 改変概要 | 想定 +/- |
|---|------|---------|---------|
| 1 | `indra/llrender/llvkloader.cpp` | (a) storage struct + map 新設 + (b) 6 新 API 実装 + (e) sCurrentPrimitive static + accessor + (f) recordGltfAssetDraw real Asset path | +180〜220 / -0 |
| 2 | `indra/llrender/llvkloader.h` | (b) 6 新 API declare + (e) sCurrentPrimitive accessor declare | +12〜18 / -0 |
| 3 | `indra/newview/gltf/primitive.h` | (c) `uploadVulkanBuffers()` declare + (d) dtor 新設 or 拡張 | +3〜6 / -0 |
| 4 | `indra/newview/gltf/primitive.cpp` | (c) `uploadVulkanBuffers()` 実装 + (d) dtor 内 unregister 対称配線 | +35〜45 / -0 |
| 5 | `indra/newview/gltf/asset.cpp` | (c-hook) `Asset::uploadTransforms` 末尾に全 Primitive iterate + `uploadVulkanBuffers()` call | +7〜10 / -0 |

**改変 0 件**: `indra/newview/gltfscenemanager.cpp` (= PC-N-9 scope) + `indra/newview/app_settings/settings.xml` (= cvar 新設 0 件 (N8-7) A) + CMake + codegen + shader + tests/

---

## §5. PC-N-8 design-lock Exit Criteria 9 項全充足

| # | Criteria | 充足 |
|---|----------|------|
| (i) | PC-N-8 literal scope §0 明文化 ((N8-1) B + (N8-2) A + (N8-3) A + (N8-5) B + (N8-6) A + (N8-7) A 採用後の 7 件) | ✅ §0 |
| (ii) | 必読 1 件 §1 + pinpoint reference 13 件別記 | ✅ §1 |
| (iii) | ambiguity (N8-1)..(N8-14) 14 件 + AYA literal「全件推奨で進めてもらえますか?」record (2026-06-05) | ✅ §3 |
| (iv) | 採用根拠 14 件明文化 | ✅ §3 |
| (v) | 実装計画 (a)-(g) 7 step 分解 + 各 step 具体 code stub example 添付 | ✅ §4.1-4.7 |
| (vi) | 実装 phase Exit Criteria 10 項明文化 | ✅ §6 |
| (vii) | GATE-B 整合 + MUSEUBO-A 整合 + 設計原則整合 | ✅ §4.8-4.10 |
| (viii) | 想定改変 file 5 件明文化 | ✅ §4.11 |
| (ix) | `indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合 | ✅ 本 commit |

---

## §6. 実装 phase Exit Criteria 10 項 ((N8-14) A 採用)

| # | Criteria |
|---|----------|
| (i) | `sPrimitiveVertexBuffers` / `sPrimitiveIndexBuffers` storage map + `PrimitiveVulkanBuffer` struct 新設 (PC-N-8 (a) tag block、llvkloader.cpp anonymous namespace 内) |
| (ii) | LLVKLoader 6 新 API 実装 = `registerPrimitiveVertexBuffer` + `writePrimitiveVertexBuffer` + `unregisterPrimitiveVertexBuffer` + 同形 index = 6 件 (PC-N-8 (b) tag block、(N8-5) B per-Primitive 形) |
| (iii) | `Primitive::uploadVulkanBuffers()` 新設関数実装 = position vec3 抽出 + index UINT32 direct copy + lazy register on first call (PC-N-8 (c) tag block、(N8-3) A + (N8-4) A) |
| (iv) | `Asset::uploadTransforms` 末尾に全 Primitive iterate + `uploadVulkanBuffers()` call hook 追加 (PC-N-8 (c-hook) tag block、PC-7γ-3 同形 cadence) |
| (v) | `Primitive` dtor 内 `unregisterPrimitiveVertexBuffer` + `unregisterPrimitiveIndexBuffer` 対称配線 (PC-N-8 (d) tag block) |
| (vi) | `sCurrentPrimitive` static + setter/getter/clear accessor 新設 (PC-N-8 (e) tag block、(N8-12) A、PC-N-9 で GLTFSceneManager::render が set) |
| (vii) | `recordGltfAssetDraw` 内 real Asset path 配線 (PC-N-8 (f) tag block、PC-N-7 (e) 直前並列、sCurrentAsset + sCurrentPrimitive + map entry + VkBuffer 4 guard、`sGltfStubAssetPipeline` 再利用、UBO + Skin 配線 + bindVertexBufferVk + bindIndexBufferVk(UINT32) + vkCmdDrawIndexed + first-fire LL_INFOS marker) |
| (viii) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count=6 不変、PC-N-7 commit `70e5faafff` と同数) |
| (ix) | MUSEUBO-A 整合 = sCurrentAsset/sCurrentPrimitive == nullptr natural guard で PC-N-8 単独発火なし + 5 段 graceful degrade |
| (x) | build verify literal = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 全 PASS + handoff complete doc 起案 |

### §6.1 integrity check 想定

- `grep -c "LL_VULKAN_GLSL" indra/llrender/llvkloader.cpp` → **6** (= PC-N-7 と同数、新規 `#ifdef` 追加 0 件)
- `grep -c "registerPrimitiveVertexBuffer" indra/llrender/llvkloader.cpp` → 想定 ≥ 1 (= 実装、宣言、map.find 等で複数)
- `grep -c "uploadVulkanBuffers" indra/newview/gltf/primitive.cpp` → 想定 ≥ 1 (= 関数定義)

### §6.2 想定 build verify command

```bash
make -j4 llrender
INTEGRATION_TEST_lluboringbuffer
INTEGRATION_TEST_llassetubopool
INTEGRATION_TEST_llpipelinecachestorage
cd scripts/ubo_codegen && python3 -m unittest discover tests
grep -c "LL_VULKAN_GLSL" indra/llrender/llvkloader.cpp
```

---

## §7. 着手手順 (= 次 session で PC-N-8 実装 phase 着手)

1. AYA 指示「PC-N-8 実装着手お願いします」literal 受領待ち
2. 本 PC-N-8 design-lock doc 全文 Read (= 必読 1 件) + pinpoint reference (§1) を実装着手 step ごとに必要分のみ Read (= `feedback_handoff_minimal_pre_req_read` 遵守、full file dump 禁止)
3. step (a) storage struct + map 新設 → step (b) 6 新 API 実装 → step (c) Primitive::uploadVulkanBuffers + asset.cpp (c-hook) → step (d) Primitive dtor → step (e) sCurrentPrimitive accessor → step (f) recordGltfAssetDraw real Asset path → step (g) build verify literal 取得 → handoff complete doc 起案 + Exit Criteria 10 項 self-verify
4. (d) Primitive dtor 内 unregister 配線時、`Primitive` の copy/move semantics 取扱を literal 確認 (= unordered_map storage key は Primitive* 自身ゆえ move/copy 後の key 整合性確認要)
5. AYA 明示 commit 指示後 commit (= `feedback_no_auto_commit` 遵守)

---

## §8. 残 strict 線形

PC-N-8 design-lock ✅ 本 commit → **PC-N-8 実装** ⏳ 次 session (= step (a)-(g) 7 step 実施 + Exit Criteria 10 項 self-verify + handoff complete doc 起案) → PC-N-9 design-lock + 実装 (= `GLTFSceneManager::render` 統合 + `AYAGltfRealDrawEnabled` cvar gate 配線 + `setCurrentPrimitive` / `clearCurrentPrimitive` per-Primitive loop 配線 → PC-N-8 real Asset path 自動発火) ⏳ → PC-N-10 design-lock + 実装 (= cleanup + 3 stub cvar deprecate + `sGltfStubSkin` sentinel deprecate + Phase 1.D complete marker 起案) ⏳ → **Phase 1.D complete** ⏳ → Phase 1 全完了 → Mac/Win 開発者補完 phase。

---

## §9. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1 design-lock ✅ + PC-N-1 ✅ + PC-N-2 design-lock ✅ + PC-N-2 ✅ + PC-N-4 design-lock ✅ + PC-N-4 ✅ + PC-N-3 design-lock ✅ + PC-N-3 ✅ = Phase 1.C complete ✅ + PC-8 Linux primary marker ✅ = Phase 1.C strict 線形終了 ✅ + PC-N-5 design-lock ✅ + PC-N-5 ✅ = Phase 1.D 着手起点 実装完了 ✅ + Phase 1.D decomposition design-lock ✅ + PC-N-6 design-lock ✅ + PC-N-6 ✅ = Phase 1.D 内 1st sub-step 実装完了 ✅ + PC-N-7 design-lock ✅ + PC-N-7 ✅ = Phase 1.D 内 2nd sub-step 実装完了 ✅ + **PC-N-8 design-lock ✅ 本 commit = Phase 1.D 内 3rd sub-step design-lock 完了** + PC-N-8 実装 ⏳ 次 session + PC-N-9/10 各 design-lock + 実装 ⏳ + Phase 1.D complete ⏳ + Phase 1.E (multi-asset / multi-skin / worker thread) ⏳

---

## §10. self-verify 9 観点 全 ✅

1. **PC-N-8 literal scope §0 完全分解 7 件** = (1) vertex buffer infrastructure + (2) index buffer infrastructure + (3) register/write asset.cpp 側 lifecycle + (4) API 新設 6 件 + (5) `Primitive::uploadVulkanBuffers()` 新設 + (6) `recordGltfAssetDraw` real Asset path 配線 + (7) `sCurrentPrimitive` static + accessor 新設 ✅
2. **必読 1 件 §1 + pinpoint reference 13 件別記** = PC-N-7 complete + asset.cpp uploadTransforms PC-7γ-3 + Asset dtor unregister + registerAssetUbo/writeAssetUbo + recordGltfAssetDraw + Primitive 構造 + Primitive::upload + sCurrentAsset/sCurrentSkin + bindVertexBufferVk/bindIndexBufferVk + UboInstance struct + cross-platform spec §6 + GATE-B + 設計原則 ✅
3. **現状調査 §2 10 項網羅** = code 8 項 + asset.cpp + GLTFSceneManager 2 項 + Asset インスタンス管理 + sCurrentPrimitive 新設根拠 ✅
4. **ambiguity (N8-1)..(N8-14) 14 件 AYA literal「全件推奨で進めてもらえますか?」record (2026-06-05) §3** ✅
5. **採用根拠 14 件明文化 §3** ✅
6. **実装計画 (a)-(g) 7 step §4 + 各 step 具体 code stub example 添付** = (a) storage struct + map + (b) 6 新 API + (c) Primitive::uploadVulkanBuffers + (c-hook) Asset::uploadTransforms 末尾 hook + (d) Primitive dtor + (e) sCurrentPrimitive accessor + (f) recordGltfAssetDraw real Asset path + (g) build verify ✅
7. **GATE-B 整合 §4.8 (= `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、cvar 新設 0 件、settings.xml 改変 0 件) + MUSEUBO-A 整合 §4.9 (= sCurrentAsset/sCurrentPrimitive natural guard で PC-N-8 単独発火なし + 5 段 graceful degrade) + 設計原則整合 §4.10 ((1) call site 不変 + Primitive Vulkan field 直接追加なし + shader 改変ゼロ + (2) per-Primitive ownership で worker thread 分離余地確保)** ✅
8. **想定改変 file 5 件明文化 §4.11** = llvkloader.cpp / .h + primitive.h / .cpp + asset.cpp、`gltfscenemanager.cpp` (= PC-N-9 scope) + settings.xml + CMake + codegen + shader + tests/ は改変 0 件 ✅
9. **`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合** = cross-platform spec §6 PC-N-8 行追記 + 本 design-lock doc 1 件起案のみ ✅

---

## §11. 次 session 着手 1 line

**PC-N-8 実装着手** = step (a) storage struct + map 新設 + (b) 6 新 API 実装 + (c) `Primitive::uploadVulkanBuffers()` 新設 + asset.cpp 末尾 hook + (d) `Primitive` dtor 内 unregister + (e) `sCurrentPrimitive` static + accessor + (f) `recordGltfAssetDraw` real Asset path 配線 + (g) build verify literal 取得 + handoff complete doc 起案 + Exit Criteria 10 項 self-verify + AYA commit 指示受領後 commit。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 PC-N-8 design-lock doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 必読 1 件 + pinpoint reference 13 件別記、本 session も Read pinpoint のみ (= PC-N-7 complete doc 全文 + Phase 1.D decomposition §4.2-4.3 + cross-platform spec §6 PC-N-8 行 + asset.cpp:200-291 + Explore agent 経由 10 項 pinpoint 報告)、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §10
- **feedback_build_only_verified** 遵守 = design-lock phase は `indra/` 改変 0 件で build verify 対象外、実装 phase で literal 検証取得予定 (N8-13) A 採用
- **feedback_no_scope_shrink** 遵守 = PC-N-8 literal scope §0 完全分解 7 件、(N8-1) B 採用は「PC-N-8 scope = infrastructure + register/write + recordGltfAssetDraw real Asset path 配線」の境界確定ゆえ縮小ではない (= Phase 1.D decomposition §4.4 で `AYAGltfRealDrawEnabled` cvar gate + GLTFSceneManager::render 統合は PC-N-9 scope 明記)、stub 経路は並走維持で PC-N-10 deprecate、PC-N-8 単独 sub-step 範囲は完全保持
- **feedback_doubt_self_first** 遵守 = ambiguity 14 件発見で停止 + 推奨案提示 + AYA literal「全件推奨で進めてもらえますか?」一括確認後本 design-lock doc 起案、推測実装なし、特に (N8-6) signature 拡張要否 (AYA literal「signature 拡張 = Asset* / Primitive* 受領」と緊張ありの A 採用) は AYA 一括確認受領で確定
- **feedback_confirm_referent_before_acting** 遵守 = 14 件 batch AYA 確認 (2026-06-05)、AYA literal「全件推奨で進めてもらえますか?」一括record 受領で確定、推測実装なし
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = PC-N-8 = 実 Asset 経由 vertex/index buffer infrastructure + register/write + recordGltfAssetDraw 配線単独 sub-step、PC-N-9 (GLTFSceneManager::render 統合 + cvar gate) + PC-N-10 (cleanup) 残 2 sub-step は分離、本 doc 起案も PC-N-8 単独 design-lock のみ
- **feedback_design_phase_no_code_write** 厳格遵守 = 本 PC-N-8 design-lock phase は doc 起案のみ、`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 + CMake 改変 0 件
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit 予定
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「commit してください」literal 受領待ち
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在予定
- **feedback_no_bare_reference_ids** 遵守 = (N8-1)..(N8-14) 各 ID に項目名 / 採用案内容併記 + (a)..(g) 各 step に作業内容併記
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、git add 個別 file 指定 + `git add -A` 不使用予定
- **memory `project_ayastorm_r41_design_principles`** 整合 = (1) Upstream OpenGL 取り込みやすさ維持 = (N8-6) A signature 不変 + (N8-2) A storage は LLVKLoader 内 encapsulate (= Primitive 構造体への Vulkan field 直接追加なし) + GLTFSceneManager::render 改変 0 件 + shader 改変ゼロ + PSO 改変なし ((N7-8) A 継承) + (2) Core プロセス分散実現 = (N8-2) A per-Primitive ownership で multi-Primitive 並走時の最小単位確立、PC-N-9 以降の per-Primitive worker thread 分離余地確保
- **memory `project_r41_phase1b_vulkan_host_gate`** 整合 = GATE-B = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、cvar runtime gate のみ (PC-N-8 では cvar 新設 0 件、natural guard) §4.8
- **memory `project_ayastorm_three_platforms`** 整合 = cross-platform spec §6 PC-N-8 行追記で macOS / Windows 派生 fix 候補欄起案 = `VK_INDEX_TYPE_UINT32` + INDEX_BUFFER_BIT / VERTEX_BUFFER_BIT usage + VMA HOST_ACCESS_SEQUENTIAL_WRITE は MoltenVK 標準対応範囲ゆえ派生 fix 候補なし + Windows full Vulkan ゆえ派生 fix 候補なし、Linux primary 完成 → 他者補完 model 整合

---
