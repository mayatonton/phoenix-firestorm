# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.D PC-N-8 complete

**Status**: ✅ **PC-N-8 implementation complete = Phase 1.D 内 3rd sub-step 実装完了**

**Date**: 2026-06-05
**Branch**: `feature/ayastorm-r41-gl-removal`
**Previous commit**: `8f315a7130` (PC-N-8 design-lock complete)

---

## §0. Literal scope (= design-lock §0 踏襲)

PC-N-8 = **Phase 1.D 内 3rd sub-step = 実 LL::GLTF::Asset 経由 vertex/index
buffer Vulkan infrastructure 新設 + register/write asset.cpp 側** = 14 件
ambiguity (N8-1)..(N8-14) AYA literal「全件推奨で進めてもらえますか?」record
(2026-06-05) 確定済の (a)-(g) 7 step (+(d') 防御 cleanup) 実装。

実装 site 7+1 件:

- (a) `PrimitiveVulkanBuffer` struct + `sPrimitiveVertexBuffers` /
  `sPrimitiveIndexBuffers` `unordered_map<Primitive*, PrimitiveVulkanBuffer>` 新設
- (b) 6 新 API 実装 = `registerPrimitiveVertexBuffer` + `writePrimitiveVertexBuffer`
  + `unregisterPrimitiveVertexBuffer` + 同形 index
- (c) `Primitive::uploadVulkanBuffers()` 新設 + `Asset::uploadTransforms` 末尾
  hook (= 全 Mesh 全 Primitive iterate、PC-7γ-3 lazy register pattern 踏襲)
- (d) `Primitive` dtor 内 `unregisterPrimitiveVertexBuffer` /
  `unregisterPrimitiveIndexBuffer` 対称配線
- (d') (= 防御追加) `shutdownVulkan` 内 `sPrimitiveVertexBuffers` /
  `sPrimitiveIndexBuffers` bulk teardown (= `sAssetUboDirty` / `sSkinUboDirty`
  同形 pattern、Vulkan 先停止 + Primitive 後解放 race 防御)
- (e) `sCurrentPrimitive` static + `setCurrentPrimitive` /
  `clearCurrentPrimitive` / `getCurrentPrimitive` accessor 新設
- (f) `recordGltfAssetDraw` 内 real Asset path 配線 (= PC-N-7 (e) 直前並列、
  `sCurrentAsset` / `sCurrentPrimitive` natural guard + `sPrimitiveVertexBuffers`
  / `sPrimitiveIndexBuffers` find guard + `sGltfStubAssetPipeline` 再利用 +
  UBO sequence + `bindVertexBufferVk` + `bindIndexBufferVk(UINT32)` +
  `vkCmdDrawIndexed(real_index_count, 1, 0, 0, 0)` + first-fire LL_INFOS marker
  + early return)
- (g) build verify (= llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131
  + GATE-B integrity = LL_VULKAN_GLSL count llvkloader.cpp=6 不変) + handoff
  complete doc 起案 + cross-platform spec §6 PC-N-8 行 ✅ + §A 履歴 1 行追記

---

## §1. 改変 file 一覧

| # | file | 改変概要 |
|---|------|----------|
| 1 | `indra/llrender/llvkloader.h` | (a) `class Primitive;` forward decl + (b) 6 新 API 宣言 + (e) `sCurrentPrimitive` accessor 3 宣言 |
| 2 | `indra/llrender/llvkloader.cpp` | (a) `PrimitiveVulkanBuffer` struct + 2 map 配置 + (b) 6 API 実装 + (d') `shutdownVulkan` 防御 cleanup + (e) `sCurrentPrimitive` storage + accessor 3 実装 + (f) `recordGltfAssetDraw` real Asset path 配線 |
| 3 | `indra/newview/gltf/primitive.h` | (c) `void uploadVulkanBuffers();` 宣言追加 |
| 4 | `indra/newview/gltf/primitive.cpp` | `#include "llvkloader.h"` + (c) `Primitive::uploadVulkanBuffers()` 実装 + (d) dtor 内 unregister hook |
| 5 | `indra/newview/gltf/asset.cpp` | (c) `Asset::uploadTransforms` 末尾 hook = 全 Mesh 全 Primitive iterate + `primitive.uploadVulkanBuffers()` |
| 6 | `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md` | §6 PC-N-8 行状態 ✅ 反映 + §A 履歴 1 行追記 |
| 7 | (本 file) `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-...-phase1-d-pc-n-8-complete.md` | 本 complete handoff doc 新規起案 |

settings.xml 改変 0 件 ((N8-7) A = cvar 新設 0 件)、codegen 改変 0 件、shader
改変 0 件、CMake 改変 0 件。

---

## §2. (a) Storage struct + 2 map

`indra/llrender/llvkloader.cpp` anonymous namespace 内 `sGltfStubIndexData` block
直後配置 (PC-N-7 stub 経路と並列 lifecycle):

```cpp
struct PrimitiveVulkanBuffer
{
    VkBuffer        buffer        = VK_NULL_HANDLE;
    VmaAllocation   allocation    = VK_NULL_HANDLE;
    void*           mapped        = nullptr;
    U32             size_bytes    = 0u;
    U32             element_count = 0u;
};
std::unordered_map<LL::GLTF::Primitive*, PrimitiveVulkanBuffer> sPrimitiveVertexBuffers;
std::unordered_map<LL::GLTF::Primitive*, PrimitiveVulkanBuffer> sPrimitiveIndexBuffers;
```

`llvkloader.h` 上部 namespace block 内に `class Primitive;` forward decl 追加
(= layering 制約遵守: `gltf/primitive.h` を `llrender/` から include 禁止、
unordered_map<Primitive*, ...> は pointer のみ参照ゆえ forward decl で十分)。

---

## §3. (b) 6 新 API

design-lock §4.2 採用案踏襲。API signature は `element_count` を 3rd 引数として
追加 (struct.element_count field 投入用、`PC-N-8 (f)` 内
`vkCmdDrawIndexed(real_index_count, ...)` で参照)。

```cpp
bool registerPrimitiveVertexBuffer  (LL::GLTF::Primitive* primitive, U32 size_bytes, U32 element_count);
void writePrimitiveVertexBuffer     (LL::GLTF::Primitive* primitive, U32 offset, const void* data, U32 size);
void unregisterPrimitiveVertexBuffer(LL::GLTF::Primitive* primitive);
bool registerPrimitiveIndexBuffer   (LL::GLTF::Primitive* primitive, U32 size_bytes, U32 element_count);
void writePrimitiveIndexBuffer      (LL::GLTF::Primitive* primitive, U32 offset, const void* data, U32 size);
void unregisterPrimitiveIndexBuffer (LL::GLTF::Primitive* primitive);
```

5 段 graceful degrade (register 系) = (1) `sAllocator` nullptr / (2) `primitive`
nullptr / (3) idempotent (= 既 register 時 early return true) / (4)
`vmaCreateBuffer` fail / (5) `alloc_info.pMappedData` nullptr。各段で
`LL_WARNS_ONCE` + return false。

VMA usage: `VMA_MEMORY_USAGE_AUTO` + `VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT`
+ `VMA_ALLOCATION_CREATE_MAPPED_BIT` ((N8-11) A、PC-N-6/PC-N-7 同形)。
usage flag: vertex は `VK_BUFFER_USAGE_VERTEX_BUFFER_BIT`、index は
`VK_BUFFER_USAGE_INDEX_BUFFER_BIT`。

---

## §4. (c) `Primitive::uploadVulkanBuffers()` + `Asset::uploadTransforms` hook

`indra/newview/gltf/primitive.cpp`:

```cpp
void Primitive::uploadVulkanBuffers()
{
    const U32 vertex_count = (U32) mPositions.size();
    const U32 index_count  = (U32) mIndexArray.size();

    if (vertex_count > 0)
    {
        const U32 stride       = 12u;  // R32G32B32_SFLOAT = 3 * 4 B (PC-N-6 同形)
        const U32 v_size_bytes = vertex_count * stride;

        std::vector<F32> packed(vertex_count * 3u);
        for (U32 i = 0; i < vertex_count; ++i)
        {
            const F32* src = mPositions[i].getF32ptr();
            packed[i * 3u + 0u] = src[0];
            packed[i * 3u + 1u] = src[1];
            packed[i * 3u + 2u] = src[2];
        }

        LLVKLoader::registerPrimitiveVertexBuffer(this, v_size_bytes, vertex_count);
        LLVKLoader::writePrimitiveVertexBuffer   (this, 0u, packed.data(), v_size_bytes);
    }

    if (index_count > 0)
    {
        const U32 i_size_bytes = index_count * 4u;  // UINT32 = 4 B/index (PC-N-7 同形)
        LLVKLoader::registerPrimitiveIndexBuffer(this, i_size_bytes, index_count);
        LLVKLoader::writePrimitiveIndexBuffer   (this, 0u, mIndexArray.data(), i_size_bytes);
    }
}
```

- `mPositions` (`std::vector<LLVector4a>`, 16 B/vertex) → packed vec3 (12 B/vertex)
  抽出。`getF32ptr()` で {x,y,z,w} の x/y/z 3 component を memcpy 並 (Vulkan
  vertex shader は xyz のみ消費、PC-N-6 sGltfStubVertexData 同形 layout)。
- `mIndexArray` (`std::vector<U32>`, 4 B/index) → 直接 `data()` 渡し
  (`VK_INDEX_TYPE_UINT32` source-of-truth 整合、PC-N-7 同形)。
- lazy register = 初回 register 成功、2nd+ call は内側 `.find()` guard で
  early return (idempotent)。
- Vulkan 未初期化時 = `sAllocator` nullptr → 5 段 graceful degrade で no-op
  (MUSEUBO-A 整合)。

`indra/newview/gltf/asset.cpp` `Asset::uploadTransforms` 末尾 hook
(PC-7γ-3 (h) `writeAssetUbo` 直後):

```cpp
for (Mesh& mesh : mMeshes)
{
    for (Primitive& primitive : mesh.mPrimitives)
    {
        primitive.uploadVulkanBuffers();
    }
}
```

---

## §5. (d) Primitive dtor unregister + (d') shutdownVulkan 防御 cleanup

`indra/newview/gltf/primitive.cpp` `Primitive::~Primitive()`:

```cpp
Primitive::~Primitive()
{
    mOctree = nullptr;
    LLVKLoader::unregisterPrimitiveVertexBuffer(this);
    LLVKLoader::unregisterPrimitiveIndexBuffer (this);
}
```

`unregister*Buffer` は `.find()` guard 済ゆえ未 register state でも no-op =
moved-from instance / Vulkan 未初期化 path 双方で安全 (`std::vector<Primitive>`
reallocation で moved-from dtor が fire しても map から no-op erase、moved-to は
次の `uploadVulkanBuffers()` 経由 lazy register で再 entry)。

`shutdownVulkan` 内 (PC-N-7 (d) block 直後):

```cpp
if (sAllocator != VK_NULL_HANDLE)
{
    for (auto& kv : sPrimitiveVertexBuffers) { /* vmaDestroyBuffer */ }
    for (auto& kv : sPrimitiveIndexBuffers)  { /* vmaDestroyBuffer */ }
}
sPrimitiveVertexBuffers.clear();
sPrimitiveIndexBuffers.clear();
```

`sAssetUboDirty` / `sSkinUboDirty` 同形 pattern。Vulkan 先停止 + Primitive
後解放 race 時の map 内残存 entry を防御的 cleanup。

---

## §6. (e) sCurrentPrimitive static + accessor

`indra/llrender/llvkloader.cpp` anonymous namespace 内 `sCurrentSkin` 直後:

```cpp
LL::GLTF::Primitive* sCurrentPrimitive = nullptr;
```

accessor 3 件 (= namespace LL 内 setCurrentSkin/clearCurrentSkin/getCurrentSkin
block 直後):

```cpp
void setCurrentPrimitive(LL::GLTF::Primitive* primitive) { sCurrentPrimitive = primitive; }
void clearCurrentPrimitive()                              { sCurrentPrimitive = nullptr;  }
LL::GLTF::Primitive* getCurrentPrimitive()                { return sCurrentPrimitive;     }
```

PC-N-8 単独では発火経路ゼロ = `GLTFSceneManager::render` set/clear は PC-N-9
scope。PC-N-8 では accessor declare + storage 配置のみ (= nullptr natural guard)。

---

## §7. (f) `recordGltfAssetDraw` real Asset path

PC-N-7 (e) 直前並列配置 = 3 cvar 経路優先順位
**PC-N-8 (real) > PC-N-7 (stub IB) > PC-N-6 (stub VB) > PC-N-5 (shader generate)**。

```cpp
{
    LL::GLTF::Primitive* primitive = sCurrentPrimitive;
    LL::GLTF::Asset*     asset     = sCurrentAsset;
    if (primitive != nullptr
        && asset != nullptr
        && sGltfStubAssetPipeline != VK_NULL_HANDLE)
    {
        auto vb_it = sPrimitiveVertexBuffers.find(primitive);
        auto ib_it = sPrimitiveIndexBuffers.find(primitive);
        if (vb_it != sPrimitiveVertexBuffers.end()
            && ib_it != sPrimitiveIndexBuffers.end()
            && vb_it->second.buffer != VK_NULL_HANDLE
            && ib_it->second.buffer != VK_NULL_HANDLE
            && ib_it->second.element_count > 0u)
        {
            vkCmdBindPipeline(cmd_buf, ..., sGltfStubAssetPipeline);
            // UBO sequence (writeDrawUbo + writeSkinUbo + flushSkinUbos + bindV3aRigged)
            // push constant 64 B identity
            const U32 real_index_count = ib_it->second.element_count;
            LLVKLoader::bindVertexBufferVk(cmd_buf, vb_it->second.buffer, 0);
            LLVKLoader::bindIndexBufferVk (cmd_buf, ib_it->second.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd_buf, real_index_count, 1, 0, 0, 0);
            // first-fire LL_INFOS marker
            return;
        }
    }
}
```

5 段 graceful degrade = (1) `sCurrentAsset` / (2) `sCurrentPrimitive` /
(3) `sGltfStubAssetPipeline` / (4) `sPrimitiveVertexBuffers` find /
(5) `sPrimitiveIndexBuffers` find + buffer handle + element_count。
guard fail 時は silent fall-through to PC-N-7 (e) (= 既存 stub 経路維持)。

cvar 新設 0 件 ((N8-7) A) = `sCurrentAsset` / `sCurrentPrimitive` の natural
guard が gate 役割。PC-N-9 で `GLTFSceneManager::render` が per-primitive loop
内 set/clear して自動発火。

`Skin_GLTFJoints` UBO は本 phase 時点でも `sGltfStubSkin` sentinel 共用 +
identity matrix bone data (= PC-N-9 で real Skin owner 切替予定)。

---

## §8. Build verify literal

GATE-B integrity check:

```
$ grep -c LL_VULKAN_GLSL indra/llrender/llvkloader.cpp
6
```

(= PC-N-7 commit `70e5faafff` と同数、`#ifdef LL_VULKAN_GLSL` 新規追加 0 件)

llrender:

```
$ make -j4 llrender
[100%] Built target llrender
```

WARNING 0 / ERROR 0。

INTEGRATION TUT:

```
$ ./sharedlibs/bin/INTEGRATION_TEST_lluboringbuffer       → 11/11 PASS YAY!!
$ ./sharedlibs/bin/INTEGRATION_TEST_llassetubopool         → 10/10 PASS YAY!!
$ ./sharedlibs/bin/INTEGRATION_TEST_llpipelinecachestorage → 13/13 PASS YAY!!
```

codegen:

```
$ cd scripts/ubo_codegen && python3 -m unittest discover tests
Ran 131 tests in 0.062s
OK
```

(= 131/131、PC-N-1..PC-N-7 同形維持)

---

## §9. Exit Criteria 10 項充足

| # | criterion | status |
|---|-----------|--------|
| i | `PrimitiveVulkanBuffer` struct + `sPrimitiveVertexBuffers`/`sPrimitiveIndexBuffers` 2 map 新設 ((N8-1) B + (N8-11) A) | ✅ |
| ii | 6 新 API (= register/write/unregister × vertex/index) 実装 + 5 段 graceful degrade ((N8-2) A + (N8-11) A) | ✅ |
| iii | `Primitive::uploadVulkanBuffers()` 新設 + `Asset::uploadTransforms` 末尾 hook (全 Primitive iterate、PC-7γ-3 lazy register pattern) ((N8-3) A + (N8-10) A) | ✅ |
| iv | `Primitive` dtor unregister 対称配線 ((N8-4) A) + (d') shutdownVulkan 防御 cleanup | ✅ |
| v | `sCurrentPrimitive` static + accessor 3 件 ((N8-5) B) | ✅ |
| vi | `recordGltfAssetDraw` 内 real Asset path 配線 (PC-N-7 (e) 直前並列、`signature 不変` 維持) ((N8-6) A + (N8-9) A) + `sGltfStubAssetPipeline` 再利用 ((N8-8) A) | ✅ |
| vii | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp = 6 不変、PC-N-7 同数) | ✅ |
| viii | MUSEUBO-A 整合 = `sCurrentAsset` / `sCurrentPrimitive` nullptr natural guard で PC-N-8 単独発火経路ゼロ + 5 段 graceful degrade ((N8-7) A) | ✅ |
| ix | build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 ((N8-12) A) | ✅ |
| x | tag block 統一 PC-N-8 (a)/(b)/(c)/(d)/(d')/(e)/(f) + first-fire LL_INFOS marker + handoff complete doc 起案 ((N8-13) A + (N8-14) A) | ✅ |

---

## §10. 残 strict 線形

- ✅ Phase 1.A
- ✅ Phase 1.B
- ✅ (Z) SSS / (W) uniform4iv / (Y) Phase 1.C prep
- ✅ PC-0..PC-6ζ / PC-7α..PC-7ε
- ✅ PC-N decomposition + PC-N-1..PC-N-4 (= Phase 1.C complete)
- ✅ PC-8 Linux primary marker (= Phase 1.C strict 線形終了)
- ✅ PC-N-5 design-lock + 実装 (= Phase 1.D 着手起点)
- ✅ Phase 1.D decomposition design-lock (= PC-N-6..PC-N-10 5 sub-step 分解)
- ✅ PC-N-6 design-lock + 実装 (= Phase 1.D 内 1st sub-step)
- ✅ PC-N-7 design-lock + 実装 (= Phase 1.D 内 2nd sub-step)
- ✅ PC-N-8 design-lock (commit `8f315a7130`) + **PC-N-8 実装 ✅ 本 commit
  (= Phase 1.D 内 3rd sub-step 実装完了)**
- ⏳ PC-N-9 design-lock + 実装 = `GLTFSceneManager::render` 統合 +
  `AYAGltfRealDrawEnabled` cvar gate + `setCurrentPrimitive` /
  `clearCurrentPrimitive` hook 配線 = 次 session 着手
- ⏳ PC-N-10 design-lock + 実装 = cleanup + 3 stub cvar deprecate
- ⏳ Phase 1.D complete → Phase 1 全完了 → Mac/Win 開発者補完 phase

---

## §11. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ +
(Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α..PC-7ε ✅ +
PC-N decomposition design-lock ✅ + PC-N-1..PC-N-4 ✅ = Phase 1.C complete ✅ +
PC-8 Linux primary marker ✅ = Phase 1.C strict 線形終了 ✅ +
PC-N-5 ✅ = Phase 1.D 着手起点 ✅ + Phase 1.D decomposition design-lock ✅ +
PC-N-6 ✅ = Phase 1.D 内 1st sub-step ✅ + PC-N-7 ✅ = 2nd sub-step ✅ +
**PC-N-8 ✅ 本 commit = 3rd sub-step ✅** + PC-N-9/10 ⏳ + Phase 1.D complete ⏳

---

## §12. self-verify 9 観点 全 ✅

1. ✅ Exit Criteria 10 項全充足 (§9)
2. ✅ 必読 1 件 (PC-N-8 design-lock doc) + pinpoint reference (PC-N-7 (e) 配置・
   Primitive dtor / mPositions・asset.cpp uploadTransforms 末尾) 別記 = full
   file dump なし (= feedback_handoff_minimal_pre_req_read 整合)
3. ✅ step (a)/(b)/(c)/(d)/(d')/(e)/(f)/(g) 8 site 全実装 ((d') は防御追加)
4. ✅ ambiguity (N8-1)..(N8-14) 14 件 AYA literal「全件推奨で進めてもらえますか?」
   record (2026-06-05) design-lock 継承 + 本実装で全件採用案通り実装
5. ✅ GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp = 6 不変)
6. ✅ MUSEUBO-A 整合 = `sCurrentAsset` / `sCurrentPrimitive` nullptr natural guard
   で PC-N-8 単独発火経路ゼロ + 5 段 graceful degrade で OpenGL 経路不変温存
7. ✅ build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11+10+13 +
   codegen 131/131 全 PASS
8. ✅ commit 内容 = 5 modified (.h/.cpp × 2 + primitive.h + primitive.cpp +
   asset.cpp) + 2 modified (cross-platform spec) + 1 new doc (本 handoff) +
   CMake 改変 0 + codegen 改変 0 + shader 改変 0 + settings.xml 改変 0 +
   Co-Authored-By 不在
9. ✅ feedback_no_scope_shrink 遵守 = PC-N-8 literal scope 7 件 §0 全件実装、
   `(N8-6) A signature 不変` は AYA literal「全件推奨」一括確認受領で確定 =
   `sCurrentAsset`/`sCurrentPrimitive` 経由 owner 解決ゆえ縮小ではない、
   per-Primitive ownership lifecycle で feedback_ubo_migration_one_at_a_time
   厳格遵守整合

---

## §13. 次 session 着手 1 line

PC-N-9 design-lock 着手 = `GLTFSceneManager::render` 統合 + per-Primitive loop
内 `LLVKLoader::setCurrentPrimitive` / `clearCurrentPrimitive` hook 配線 +
`AYAGltfRealDrawEnabled` cvar 新設 = ambiguity 確認 + 実装計画分解 + Exit
Criteria 明文化。`feedback_ubo_migration_one_at_a_time` 厳格遵守で本 PC-N-8
infrastructure を baseline に GLTFSceneManager 経由実 draw 通電。

---

## §A. feedback 遵守 record

- ✅ `feedback_proactive_handoff` (本 complete handoff doc 起案)
- ✅ `feedback_handoff_minimal_pre_req_read` (必読 1 件 + pinpoint Read のみ、
  full file dump なし)
- ✅ `feedback_self_verify_before_handoff` (9 観点 self-verify 全 ✅)
- ✅ `feedback_build_only_verified` (llrender + WARNING 0 + TUT 11+10+13 +
  codegen 131/131 + GATE-B integrity literal 検証取得)
- ✅ `feedback_no_scope_shrink` (PC-N-8 literal scope 7 件 §0 全件実装 + (d')
  防御追加、`(N8-6) A signature 不変` は AYA literal「全件推奨」一括確認で
  `sCurrentAsset`/`sCurrentPrimitive` 経由 owner 解決確定 = 縮小ではない)
- ✅ `feedback_doubt_self_first` (design-lock phase で ambiguity 14 件発見 +
  推奨案提示 + AYA literal「全件推奨で進めてもらえますか?」record 後本実装、
  本実装中も register API signature 拡張 (= `element_count` 3rd 引数) は
  PrimitiveVulkanBuffer.element_count field 投入経路の design gap 解決ゆえ
  literal 推奨案逸脱なしと判定 + Read で `Primitive` mPositions/mIndexArray
  type literal 確認後配線、推測実装なし)
- ✅ `feedback_confirm_referent_before_acting` (14 件 batch AYA 確認 design-lock
  phase で完了、本実装中も挿入順「PC-N-7 (e) 直前並列」は design-lock §4.9
  literal 確認後採用)
- ✅ `feedback_ubo_migration_one_at_a_time` 厳格遵守 (PC-N-8 = 実 LL::GLTF::Asset
  経由 vertex/index buffer Vulkan infrastructure 新設単独 sub-step =
  per-Primitive ownership + 6 新 API + Asset::uploadTransforms 末尾 hook +
  recordGltfAssetDraw real Asset path 配線、`GLTFSceneManager::render` 統合 +
  cvar gate は PC-N-9 で別 design-lock)
- ✅ `feedback_design_phase_no_code_write` 整合 (本 PC-N-8 は実装 phase =
  design-lock commit `8f315a7130` で indra/ 改変 0 件完了済、本 session で
  indra/ 改変は実装 phase ゆえ整合)
- ✅ `feedback_release_branch_workflow` (feature branch
  `feature/ayastorm-r41-gl-removal` 上 commit)
- ✅ `feedback_no_auto_commit` (AYA 明示 commit 指示「commit してください」
  literal 受領後 commit 予定)
- ✅ `feedback_no_claude_coauthor` (Co-Authored-By 行不在)
- ✅ `feedback_no_bare_reference_ids` ((N8-1)..(N8-14) 各 ID に項目名 / 採用案
  内容併記 + (a)..(g) 各 step に作業内容併記)
- ✅ `feedback_tests_dir_never_commit` 整合 (tests/ 改変 0 件、git add 個別
  file 指定予定)
- ✅ memory `project_ayastorm_r41_design_principles` 整合
  ((1) Upstream OpenGL 取り込みやすさ維持 = `recordGltfAssetDraw` signature 不変
  + `Primitive` 構造体 Vulkan field 直接追加なし (LLVKLoader 内 encapsulate) +
  `GLTFSceneManager::render` 改変 0 件 (PC-N-9 scope) + shader 改変ゼロ +
  (2) Core プロセス分散実現 = per-Primitive ownership で UBO/cmdbuf 並列化
  design 余地確保)
- ✅ memory `project_r41_phase1b_vulkan_host_gate` 整合 (GATE-B =
  `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、cvar 新設 0 件 ((N8-7) A natural guard)、
  count 不変)
- ✅ memory `project_ayastorm_three_platforms` 整合 (cross-platform spec §6
  PC-N-8 行状態 ✅ 反映 = `VERTEX_BUFFER_BIT` + `INDEX_BUFFER_BIT` + VMA
  `HOST_ACCESS_SEQUENTIAL_WRITE` + per-Primitive `unordered_map` storage は
  host-side container ゆえ MoltenVK 標準対応範囲、Windows full Vulkan ゆえ
  派生 fix 候補なし、Linux primary 完成 → 他者補完 model 整合)
