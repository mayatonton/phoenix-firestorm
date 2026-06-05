# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.D PC-N-9 design-lock

**Status**: ⏳ **PC-N-9 design-lock complete = Phase 1.D 内 4th sub-step design-lock**

**Date**: 2026-06-05
**Branch**: `feature/ayastorm-r41-gl-removal`
**Previous commit**: `f68fd15e15` (PC-N-8 complete = Phase 1.D 内 3rd sub-step 実装完了)

---

## §0. PC-N-9 literal scope (= AYA 起案文 literal 4 件)

PC-N-9 = **Phase 1.D 内 4th sub-step = `GLTFSceneManager::render` 統合 +
`AYAGltfRealDrawEnabled` cvar gate 配線**。

literal scope 4 件 (AYA 起案文 2026-06-05):

1. `GLTFSceneManager::render` 統合 = per-Primitive loop 内
   `LLVKLoader::setCurrentPrimitive(&primitive)` / `clearCurrentPrimitive()`
   hook 配線
2. `setCurrentAsset(asset)` / `clearCurrentAsset()` の per-Asset hook 配線
   (= 既存 PC-7γ-2 配線が無ければ新設) **→ Read 結果: PC-7γ-2 で既配線済
   (`gltfscenemanager.cpp:697` + `:783`)、PC-N-9 改変 0 件、確認のみ**
3. `AYAGltfRealDrawEnabled` cvar 新設 (Boolean default false Persist=1、
   PC-N-6/7 stub cvar 同形 pattern、PC-N-10 で 3 stub cvar deprecate 予定)
4. PC-N-8 (f) で配線済 `recordGltfAssetDraw` real Asset path を本 cvar gate
   に従属 (= cvar=false 時は PC-N-8 経路 skip + 既存 stub 経路へ fall-through)

実装 site 3 件 (= 別 session):

- **(a)** GLTFSceneManager::render per-Primitive loop body 冒頭 (= line 743 直後、
  `if (rigged)` 外) で `LLVKLoader::setCurrentPrimitive(&primitive)` + line 777
  clearCurrentSkin 並列 (drawRangeFast 直後 unconditional) で
  `LLVKLoader::clearCurrentPrimitive()`
- **(b)** llvkloader.cpp PC-N-8 (f) block (line 6092-6207) 全体を
  `static LLCachedControl<bool> sAyastormGltfRealDrawEnabled` guard で wrap
  (= cvar=false 時 silent fall-through to PC-N-7 (e) block)
- **(c)** settings.xml に `AYAGltfRealDrawEnabled` Boolean cvar 1 件追加
  (Persist=1, default 0)

---

## §1. 必読 (= 1 件)

- **PC-N-8 complete handoff**:
  `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-...-phase1-d-pc-n-8-complete.md`
  = PC-N-8 commit `f68fd15e15` 結果 = `PrimitiveVulkanBuffer` struct + 2 map +
  6 新 API + `Primitive::uploadVulkanBuffers()` + `Asset::uploadTransforms`
  末尾 hook + `Primitive` dtor unregister + shutdownVulkan 防御 cleanup +
  `sCurrentPrimitive` static + accessor + recordGltfAssetDraw real Asset path
  配線 (= sCurrentAsset/sCurrentPrimitive natural guard)

### §1.1 pinpoint reference (= 別記、full file dump なし)

| # | site | location | 用途 |
|---|------|----------|------|
| 1 | `GLTFSceneManager::render` per-Asset shader bind + setCurrentAsset 配線 | `gltfscenemanager.cpp:681-704` | PC-7γ-2 既配線確認 = setCurrentAsset(&asset) on line 697 |
| 2 | `GLTFSceneManager::render` per-Primitive loop body | `gltfscenemanager.cpp:738-779` | (a) setCurrentPrimitive/clearCurrentPrimitive 配線 site = line 743 直後 + line 777 並列 |
| 3 | `GLTFSceneManager::render` ds loop 終了 + clearCurrentAsset | `gltfscenemanager.cpp:782-786` | PC-7γ-2 既配線確認 = clearCurrentAsset() on line 783 |
| 4 | PC-N-7 (e) block (stub IB) cvar guard pattern | `llvkloader.cpp:6229-6307` | (b) PC-N-9 cvar guard 配置 pattern source = `static LLCachedControl<bool> sAyastormGltfStubIbEnabled(gSavedSettings, "AYAGltfStubIndexBufferEnabled", false)` 同形 |
| 5 | PC-N-8 (f) block real Asset path | `llvkloader.cpp:6092-6207` | (b) AYAGltfRealDrawEnabled cvar guard で wrap 対象 block |
| 6 | sCurrentPrimitive accessor 宣言 | `llvkloader.h:502-507` 周辺 | (a) setCurrentPrimitive/clearCurrentPrimitive 既宣言確認 (PC-N-8 (e) で配線済) |
| 7 | sCurrentPrimitive storage + 実装 | `llvkloader.cpp:739` + `:5932-5951` | (a) accessor 既実装確認 (PC-N-8 (e) で配線済) |
| 8 | AYAGltfStubVertexBufferEnabled cvar XML | `settings.xml:10432-10443` | (c) AYAGltfRealDrawEnabled cvar XML pattern source (= PC-N-6 同形 Comment + Persist=1 + default 0) |
| 9 | AYAGltfStubIndexBufferEnabled cvar XML | `settings.xml:10455-10466` | (c) AYAGltfRealDrawEnabled cvar XML pattern source (= PC-N-7 同形) |
| 10 | recordAvatarPlaceholderDraw 末尾 recordGltfAssetDraw fire hook | `llvkloader.cpp:6642` | (N9-6) A AYAGltfStubDrawEnabled prerequisite 経路確認 = recordGltfAssetDraw 発火入口 |
| 11 | cross-platform spec §6 PC-N-9 行 | `ayastorm-r41-cross-platform-port-spec.md:110` | (= 後述) stub → design-lock 内容更新対象 |
| 12 | Phase 1.D decomposition §4.4 PC-N-9 scope | `handoff-substep-...-phase1-d-decomposition-design-lock.md:136-142` | PC-N-9 scope literal 確認 = (D-5) C + (D-12) A 並走 Vulkan dispatcher + AYAGltfRealDrawEnabled cvar gate |

---

## §2. 現状調査 (= 7 項)

### §2.1 setCurrentAsset/clearCurrentAsset 既配線済 (PC-7γ-2)

```
gltfscenemanager.cpp:697  LLVKLoader::setCurrentAsset(&asset);   // if (!shader_bound) 内、flushAssetUbos 直前
gltfscenemanager.cpp:783  LLVKLoader::clearCurrentAsset();        // ds loop 終了直前
```

= PC-N-9 改変 0 件、AYA 起案文「既存 PC-7γ-2 配線が無ければ新設」条件節は不発動。

### §2.2 setCurrentSkin/clearCurrentSkin 既配線済 (PC-7γ-2)

```
gltfscenemanager.cpp:755  LLVKLoader::setCurrentSkin(&skin);     // if (rigged) 内、flushSkinUbos 直前
gltfscenemanager.cpp:777  LLVKLoader::clearCurrentSkin();         // drawRangeFast 直後 unconditional
```

PC-N-9 で setCurrentPrimitive/clearCurrentPrimitive を **対称配置**: setCurrentPrimitive
は per-Primitive loop body 冒頭 (`if (rigged)` 外、line 743 直後)、clearCurrentPrimitive
は line 777 clearCurrentSkin 並列 (drawRangeFast 直後 unconditional)。

### §2.3 sCurrentPrimitive storage + accessor 既配線済 (PC-N-8 (e))

```
llvkloader.cpp:739    LL::GLTF::Primitive* sCurrentPrimitive = nullptr;
llvkloader.cpp:5938-5950  setCurrentPrimitive / clearCurrentPrimitive / getCurrentPrimitive 実装
llvkloader.h          accessor 3 件宣言 (PC-N-8 で追加済)
```

= PC-N-9 で LLVKLoader 側 storage / accessor 改変 0 件、GLTFSceneManager 側 hook
配線のみ。

### §2.4 PC-N-8 (f) block 現状 = cvar gate なし

`llvkloader.cpp:6092-6207` (PC-N-8 (f) block):

```cpp
{
    LL::GLTF::Primitive* primitive = sCurrentPrimitive;
    LL::GLTF::Asset*     asset     = sCurrentAsset;
    if (primitive != nullptr
        && asset != nullptr
        && sGltfStubAssetPipeline != VK_NULL_HANDLE)
    {
        ...
    }
}
```

= 現状は `sCurrentAsset/sCurrentPrimitive` natural guard のみ。PC-N-9 で外側に
`AYAGltfRealDrawEnabled` cvar guard 追加 = (N9-7) A 採用、最外側 cvar guard で
natural guard 評価 skip。

### §2.5 recordGltfAssetDraw 発火経路 = recordAvatarPlaceholderDraw 末尾 hook

```
llvkloader.cpp:6642  recordGltfAssetDraw(cmd_buf);
```

= `recordAvatarPlaceholderDraw` 内 `AYAGltfStubDrawEnabled` cvar gate 内で fire。
PC-N-9 では本 fire 入口は不変温存 = `AYAGltfStubDrawEnabled=true` が PC-N-8 (f)
real path 発火の prerequisite。PC-N-10 で `AYAGltfStubDrawEnabled` deprecate 時
に統合予定。((N9-6) A 採用)

### §2.6 per-Primitive loop ds loop 二重実行

```
gltfscenemanager.cpp:?  for (U32 ds = 0; ds < 2; ++ds)   // ds loop = OpenGL 2 pass
gltfscenemanager.cpp:738-779  per-Primitive loop body  // ds loop 内
```

= ds loop は OpenGL 2 pass (= alpha/opaque 双方)、per-Primitive loop は 2 回通る。
setCurrentPrimitive/clearCurrentPrimitive も 2 回 set/clear 実行されるが natural
guard で no-op、recordGltfAssetDraw も 2 回 fire (= AYAGltfStubDrawEnabled +
AYAGltfRealDrawEnabled 双方 ON 時)。`first-fire LL_INFOS marker` は 1 回のみ
出力 (= `std::atomic<bool> s_first_pcn9_fire{true}; .exchange(false)` pattern)。
((N9-13) A 採用)

### §2.7 PC-N-6/7/8 既存 cvar gate pattern (= (b) source)

```cpp
static LLCachedControl<bool> sAyastormGltfStubVbEnabled(
    gSavedSettings, "AYAGltfStubVertexBufferEnabled", false);  // PC-N-6
static LLCachedControl<bool> sAyastormGltfStubIbEnabled(
    gSavedSettings, "AYAGltfStubIndexBufferEnabled", false);   // PC-N-7
```

PC-N-9 では同形:

```cpp
static LLCachedControl<bool> sAyastormGltfRealDrawEnabled(
    gSavedSettings, "AYAGltfRealDrawEnabled", false);          // PC-N-9
```

---

## §3. Ambiguity (N9-1)..(N9-14) 14 件 + AYA literal record

AYA literal「全件推奨で OK」一括確認 record (2026-06-05) 全件採用:

| # | 項目 | 採用案 | AYA literal | 採用根拠 |
|---|------|--------|-------------|---------|
| (N9-1) | `AYAGltfRealDrawEnabled` cvar gate 配置 | **A**: PC-N-8 (f) block 全体を recordGltfAssetDraw 内 `LLCachedControl<bool>` guard で wrap (false 時 silent fall-through to PC-N-7 (e)) | OK 2026-06-05 | AYA 起案文 literal「PC-N-8 (f) で配線済 recordGltfAssetDraw real Asset path を本 cvar gate に従属」直訳整合、PC-N-6/PC-N-7 同形 pattern (cvar guard 分岐冒頭) 踏襲、`signature 不変` ((N8-6) A) 維持 |
| (N9-2) | `setCurrentPrimitive` 配線位置 | **A**: per-Primitive loop body 冒頭 (= line 743 `Primitive& primitive = ...` 直後、`if (rigged)` 外) で `setCurrentPrimitive(&primitive)` + line 777 clearCurrentSkin 並列 `clearCurrentPrimitive()` (unconditional) | OK 2026-06-05 | primitive は rigged/non-rigged 問わず常に存在、`setCurrentSkin` は `if (rigged)` 内限定だが primitive は全 path 必要、clearCurrentSkin (unconditional pattern) 同形対称配置 |
| (N9-3) | `setCurrentAsset` / `clearCurrentAsset` 配線 | **A**: PC-7γ-2 で既配線済 (`:697`, `:783`) 確認のみ、PC-N-9 改変 0 件 | OK 2026-06-05 | Read 結果で確認済、AYA 起案文「既存 PC-7γ-2 配線が無ければ新設」条件節は不発動 (= 既存配線あり) |
| (N9-4) | `AYAGltfRealDrawEnabled` cvar 命名 | **literal 確定**: AYA 起案文 literal 採用 | (literal) | AYA 起案文「`AYAGltfRealDrawEnabled` cvar 新設」literal 直訳 |
| (N9-5) | cvar Type / default / Persist | **A**: Boolean default 0 Persist 1 (= PC-N-6/7 同形) | OK 2026-06-05 | AYA 起案文 literal「Boolean default false Persist=1」直訳、PC-N-6/7 stub cvar 同形 pattern 踏襲、PC-N-10 deprecate 予定明示 |
| (N9-6) | `AYAGltfStubDrawEnabled` (PC-N-5) prerequisite 関係 | **A**: AYAGltfStubDrawEnabled=true 継続要 (= recordGltfAssetDraw fire 入口は recordAvatarPlaceholderDraw 末尾 hook = PC-N-5 cvar gate 内)、Comment に明記、PC-N-10 で統合 | OK 2026-06-05 | 最小改変、PC-N-10 で AYAGltfStubDrawEnabled deprecate 予定ゆえ「prerequisite 2 cvar 同時 ON」期間は短期、live A/B 検証で許容、PC-N-5 hook path 不変 = MUSEUBO-A 整合 |
| (N9-7) | recordGltfAssetDraw 内 cvar guard 構造 | **A**: PC-N-8 (f) block の `if (primitive != nullptr && ...)` 外側に `LLCachedControl<bool>` guard 追加 (= `if (sAyastormGltfRealDrawEnabled && primitive != nullptr && ...)`) | OK 2026-06-05 | PC-N-7 (e) pattern (cvar guard 最外側) 踏襲、cvar=false 時 natural guard 評価 skip = 軽微 perf 改善 |
| (N9-8) | 改変 tag block 命名 | **A**: `<AYAstorm r41 PC-N-9 (a)>` per-Primitive set/clear + `(b)` cvar gate wrap on PC-N-8 (f) + `(c)` settings.xml cvar | OK 2026-06-05 | PC-N-6/7/8 同形 tag pattern 踏襲、(a)/(b)/(c) 命名統一 |
| (N9-9) | design-lock Exit Criteria 項目数 + 実装 phase Exit Criteria 項目数 | **A**: 9 + 10 項 (PC-N-6/7/8 同形 template) | OK 2026-06-05 | PC-N-6/7/8 同形 template 踏襲、consistency |
| (N9-10) | build verify scope | **A**: llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 + GATE-B integrity (= `LL_VULKAN_GLSL` count `llvkloader.cpp` = 6 不変) (= PC-N-6/7/8 同形) | OK 2026-06-05 | PC-8 Linux primary marker 採用後 Phase 1.D 標準 scope |
| (N9-11) | cross-platform spec §6 PC-N-9 行更新内容 | **A**: design-lock 内容更新 = (N9-1) A cvar gate on PC-N-8 (f) block + (N9-2) A per-Primitive loop body 冒頭/末尾 + (N9-6) A AYAGltfStubDrawEnabled prerequisite + macOS/Windows 派生 fix 候補なし想定 (= settings.xml/gltfscenemanager.cpp は OS 非依存) | OK 2026-06-05 | PC-N-6/7/8 同形 doc 更新 pattern 踏襲 |
| (N9-12) | 想定改変 file | **A**: `gltfscenemanager.cpp` + `llvkloader.cpp` + `settings.xml` + `cross-platform spec` + handoff doc = 5 file (= indra 3 + doc 2) | OK 2026-06-05 | (N9-1)/(N9-2) A 採用根拠、`llvkloader.h` 改変 0 件 (= PC-N-8 (e) で accessor 既宣言済)、shader/CMake/codegen 改変 0 件 |
| (N9-13) | per-Primitive loop ds loop 二重実行影響 | **A**: ds loop は OpenGL 2 pass、setCurrentPrimitive も 2 回 set/clear するが natural guard で no-op、recordGltfAssetDraw も 2 回 fire (= AYAGltfStubDrawEnabled + AYAGltfRealDrawEnabled 双方 ON 時)、first-fire LL_INFOS marker は 1 回のみ (= `std::atomic` exchange pattern) | OK 2026-06-05 | OpenGL ds loop に追従が natural、recordGltfAssetDraw 二重 fire は Vulkan 描画 stub 段階ゆえ visual impact なし (= PC-N-10 で recordAvatarPlaceholderDraw 統合時に整理) |
| (N9-14) | indra/ 改変 vs doc-only (= design-lock phase) | **A**: 本 PC-N-9 design-lock phase は `indra/` 改変 0 件、doc 起案のみ、実装 phase は別 session | OK 2026-06-05 | `feedback_design_phase_no_code_write` 厳格遵守 = PC-N-6/7/8 同 pattern |

---

## §4. 実装計画 (= (a)-(c) 3 step、実装 phase = 別 session)

### §4.1 (a) GLTFSceneManager::render 内 per-Primitive set/clear 配線

`indra/newview/gltfscenemanager.cpp:738-779` per-Primitive loop body 冒頭:

```cpp
for (auto& pdata : batches[i].mPrimitives)
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_GLTF("GLTF draw call");
    Node& node = asset.mNodes[pdata.mNodeIndex];
    Mesh& mesh = asset.mMeshes[node.mMesh];
    Primitive& primitive = mesh.mPrimitives[pdata.mPrimitiveIndex];

    // <AYAstorm r41 PC-N-9 (a)> per-Primitive current owner set
    //   ((N9-2) A、AYA literal「全件推奨で OK」record 2026-06-05) =
    //   recordGltfAssetDraw PC-N-8 (f) real Asset path 配線が
    //   sCurrentPrimitive を解決する経路を per-Primitive loop body 冒頭で確立
    //   (= if (rigged) 外、primitive は rigged/non-rigged 問わず常に存在)。
    //   clearCurrentPrimitive は drawRangeFast 直後の clearCurrentSkin 並列で
    //   unconditional clear (= setCurrentSkin if (rigged) 限定パターン同形対称)。
    LLVKLoader::setCurrentPrimitive(&primitive);
    // </AYAstorm r41 PC-N-9 (a)>

    if (rigged)
    {
        ...
    }
    ...
    {
        ...
        primitive.mVertexBuffer->drawRangeFast(...);
    }

    // <AYAstorm r41 PC-N-9 (a)> per-Primitive current owner clear (per-primitive
    //   対称配置、unconditional clear で safe = setCurrentPrimitive 必ず call 済)。
    LLVKLoader::clearCurrentPrimitive();
    // </AYAstorm r41 PC-N-9 (a)>

    LLVKLoader::clearCurrentSkin();   // PC-7γ-2 既配線
}
```

### §4.2 (b) llvkloader.cpp PC-N-8 (f) block AYAGltfRealDrawEnabled cvar guard wrap

`indra/llrender/llvkloader.cpp:6092-6207` PC-N-8 (f) block 全体を guard で wrap:

```cpp
// <AYAstorm r41 PC-N-9 (b)> AYAGltfRealDrawEnabled cvar gate on PC-N-8 (f) block
//   ((N9-1) A + (N9-7) A、AYA literal「全件推奨で OK」record 2026-06-05) =
//   PC-N-8 (f) block 全体を `LLCachedControl<bool>` guard で wrap、
//   cvar=false 時 silent fall-through to PC-N-7 (e) stub IB 経路 (= 既存 stub
//   経路温存、MUSEUBO-A 整合)。PC-N-6/PC-N-7 同形 pattern (cvar guard 最外側)。
//   Cvar 優先順位 in recordGltfAssetDraw (PC-N-9 適用後):
//   PC-N-9 (AYAGltfRealDrawEnabled, real Asset) > PC-N-7 (AYAGltfStubIndexBufferEnabled, stub IB) >
//   PC-N-6 (AYAGltfStubVertexBufferEnabled, stub VB) > PC-N-5 (shader generate 3 vertex)。
//   AYAGltfStubDrawEnabled=true は依然 prerequisite (= recordGltfAssetDraw fire
//   入口は recordAvatarPlaceholderDraw 末尾 hook = PC-N-5 cvar gate 内)、
//   PC-N-10 で AYAGltfStubDrawEnabled deprecate 時に統合予定。
static LLCachedControl<bool> sAyastormGltfRealDrawEnabled(
    gSavedSettings, "AYAGltfRealDrawEnabled", false);
if (sAyastormGltfRealDrawEnabled)
{
    // <AYAstorm r41 PC-N-8 (f)> real LL::GLTF::Asset 経由 vertex/index buffer
    //   ... (PC-N-8 既配線 block、無変更)
    {
        LL::GLTF::Primitive* primitive = sCurrentPrimitive;
        LL::GLTF::Asset*     asset     = sCurrentAsset;
        if (primitive != nullptr
            && asset != nullptr
            && sGltfStubAssetPipeline != VK_NULL_HANDLE)
        {
            ...
            // first-fire LL_INFOS marker PC-N-8 (f) 既配線、PC-N-9 で別 marker 不要
            //   (= PC-N-8 (f) marker が PC-N-9 cvar=true 経由発火 = "PC-N-8 (f)
            //   経路 PC-N-9 経由初発火" の意味で第一報)
            return;
        }
    }
    // </AYAstorm r41 PC-N-8 (f)>
}
// </AYAstorm r41 PC-N-9 (b)>

// <AYAstorm r41 PC-N-7 (e)> stub IB cvar guard (= 既配線、無変更)
...
```

注: PC-N-8 (f) 既配線 first-fire LL_INFOS marker は無変更で温存 (= PC-N-9 経由
発火時にも literal「PC-N-8 (f) GLTF real Asset draw 通電 (first fire)」が
log 出力)。PC-N-9 独自の追加 marker は不要 ((N9-13) A 整合)。

### §4.3 (c) settings.xml AYAGltfRealDrawEnabled cvar 追加

`indra/newview/app_settings/settings.xml:10466` 周辺 (= PC-N-7 cvar 直後並列):

```xml
<!-- <FS:AYAstorm r41 Phase 1.D PC-N-9> 実 LL::GLTF::Asset 経由 Vulkan real draw 経路
     通電の live A/B 切替 cvar。design-lock handoff-substep-...-phase1-d-pc-n-9-design-lock.md
     (= (N9-4) literal + (N9-5) A、AYA literal「全件推奨で OK」record 2026-06-05)。
     OFF (default) = recordGltfAssetDraw 内 PC-N-7/PC-N-6/PC-N-5 stub 経路 (= cvar=true 時)
     を継続 = PC-N-8 完了状態と機能等価 (MUSEUBO-A 整合)。前提として
     AYAGltfStubDrawEnabled=true が必要 (= recordGltfAssetDraw 発火 hook、PC-N-10 で
     統合予定)。ON = PC-N-8 (f) で配線済 real LL::GLTF::Asset 経由 vertex/index buffer
     bind + UBO sequence + bindVertexBufferVk + bindIndexBufferVk(UINT32) +
     vkCmdDrawIndexed(real_index_count, 1, 0, 0, 0) 経路発火 = Phase 1.D 4th sub-step
     通電 (= GLTFSceneManager::render 経由 per-Primitive owner 解決 + 実 Asset draw
     通電の事実確立)。Cvar 優先順位 in recordGltfAssetDraw:
     PC-N-9 (this) > PC-N-7 AYAGltfStubIndexBufferEnabled > PC-N-6 AYAGltfStubVertexBufferEnabled >
     PC-N-5 AYAGltfStubDrawEnabled。GATE-B 整合 = #ifdef LL_VULKAN_GLSL 新規追加 0 件
     (= runtime cvar gate のみ)。Persist=1 で起動間保持。PC-N-10 で 3 stub cvar deprecate 予定。 -->
<key>AYAGltfRealDrawEnabled</key>
<map>
  <key>Comment</key>
  <string>(r41 Phase 1.D PC-N-9) Enable GLTF real LL::GLTF::Asset Vulkan draw 経路 (= PC-N-8 (f) で配線済 real Asset path = sCurrentAsset/sCurrentPrimitive 経由 owner 解決 + sPrimitiveVertexBuffers/sPrimitiveIndexBuffers find + sGltfStubAssetPipeline 再利用 + UBO sequence + bindVertexBufferVk + bindIndexBufferVk(UINT32) + vkCmdDrawIndexed(real_index_count, 1, 0, 0, 0) for Phase 1.D 4th sub-step 通電). Default OFF, debug live A/B 用. 前提として AYAGltfStubDrawEnabled=true が必要 (= recordGltfAssetDraw 発火 hook, PC-N-10 で統合予定). Cvar 優先順位 in recordGltfAssetDraw: PC-N-9 (this) > PC-N-7 AYAGltfStubIndexBufferEnabled > PC-N-6 AYAGltfStubVertexBufferEnabled > PC-N-5 AYAGltfStubDrawEnabled. ON 時 PC-N-8 (f) block 発火 (= real Asset draw) + GLTFSceneManager::render per-Primitive loop 内 setCurrentPrimitive/clearCurrentPrimitive hook (PC-N-9 (a) 配線) で owner 解決. OFF 時 PC-N-7/6/5 stub 経路へ fall-through (= PC-N-8 完了状態と機能等価). PC-N-10 で 3 stub cvar (AYAGltfStubDrawEnabled, AYAGltfStubVertexBufferEnabled, AYAGltfStubIndexBufferEnabled) deprecate 予定.</string>
  <key>Persist</key>
  <integer>1</integer>
  <key>Type</key>
  <string>Boolean</string>
  <key>Value</key>
  <integer>0</integer>
</map>
<!-- </FS:AYAstorm> -->
```

### §4.4 想定改変 file 5 件

| # | file | 改変概要 | 推定 LoC |
|---|------|---------|---------|
| 1 | `indra/newview/gltfscenemanager.cpp` | (a) per-Primitive loop body 冒頭 `setCurrentPrimitive` + 末尾 `clearCurrentPrimitive` 配線 | +6/-0 |
| 2 | `indra/llrender/llvkloader.cpp` | (b) PC-N-8 (f) block 全体を `AYAGltfRealDrawEnabled` cvar guard で wrap | +4/-0 |
| 3 | `indra/newview/app_settings/settings.xml` | (c) `AYAGltfRealDrawEnabled` Boolean cvar 1 件追加 | +20/-0 |
| 4 | `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md` | §6 PC-N-9 行 design-lock 内容更新 + §A 履歴 1 行追記 | +2/-1 |
| 5 | (本 file) `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-...-phase1-d-pc-n-9-design-lock.md` | 新規 PC-N-9 design-lock handoff doc 起案 | new |

`llvkloader.h` 改変 0 件 (= PC-N-8 (e) で accessor 既宣言済)、shader 改変 0 件、
codegen 改変 0 件、CMake 改変 0 件、tests/ 改変 0 件。

### §4.5 GATE-B 整合

`#ifdef LL_VULKAN_GLSL` 新規追加 0 件 = memory `project_r41_phase1b_vulkan_host_gate`
遵守。`AYAGltfRealDrawEnabled` cvar gate は `LLCachedControl<bool>` runtime cvar
経由ゆえ `#ifdef` 非依存 = GATE-B 違反なし。

build verify literal target: `grep -c LL_VULKAN_GLSL indra/llrender/llvkloader.cpp = 6`
(= PC-N-8 commit `f68fd15e15` 同数、PC-N-7/PC-N-6 同数)。

### §4.6 MUSEUBO-A 整合 (= 5 段 graceful degrade)

`AYAGltfRealDrawEnabled=false` default で本 PC-N-9 経路発火なし、PC-N-8 完了状態と
機能等価。5 段 graceful degrade:

1. `sAllocator` nullptr → PC-N-8 (f) 内 vmaCreateBuffer 経路発火不可
2. `AYAGltfRealDrawEnabled` cvar false → PC-N-9 (b) cvar guard で silent skip
3. `sCurrentAsset` nullptr → PC-N-8 (f) natural guard で silent skip
4. `sCurrentPrimitive` nullptr → PC-N-8 (f) natural guard で silent skip
5. `sPrimitiveVertexBuffers` / `sPrimitiveIndexBuffers` `.find()` end → PC-N-8 (f)
   natural guard で silent fall-through to PC-N-7 (e)

加えて prerequisite cvar = `AYAGltfStubDrawEnabled=false` 時 `recordGltfAssetDraw`
自体が `recordAvatarPlaceholderDraw` 末尾 hook 経路で fire しないゆえ全 PC-N
経路発火なし = MUSEUBO-A 整合維持。

### §4.7 設計原則整合

memory `project_ayastorm_r41_design_principles` 整合:

- **(1) Upstream OpenGL 取り込みやすさ維持**:
  - GLTFSceneManager::render OpenGL path **完全温存** = drawRangeFast 経路無変更
  - PC-N-9 (a) は setCurrentPrimitive/clearCurrentPrimitive hook 2 行追加のみ =
    OpenGL behavior 不変
  - `recordGltfAssetDraw` signature 不変 ((N8-6) A 維持) = call site 温存
  - shader 改変ゼロ + sGltfStubAssetPipeline 再利用 ((N8-8) A) で新 pipeline 追加なし
- **(2) Core プロセス分散実現**:
  - per-Primitive setCurrentPrimitive/clearCurrentPrimitive hook は per-Primitive
    cadence で配置済 = Phase 1.E worker thread 分散時に primitive-level
    granularity で並列化可能な hook point

### §4.8 cvar 優先順位確定

PC-N-9 適用後、`recordGltfAssetDraw` 内の 3 cvar gate 評価順:

```
1. AYAGltfRealDrawEnabled (PC-N-9, this)  → real Asset path (PC-N-8 (f))
2. AYAGltfStubIndexBufferEnabled (PC-N-7) → stub IB path
3. AYAGltfStubVertexBufferEnabled (PC-N-6) → stub VB path
4. (else) shader generate 3 vertex (PC-N-5)
```

全 4 cvar の prerequisite = `AYAGltfStubDrawEnabled=true` (= `recordGltfAssetDraw`
発火入口 = `recordAvatarPlaceholderDraw` 末尾 hook、PC-N-10 で deprecate 予定)。

---

## §5. PC-N-9 design-lock Exit Criteria (= 9 項全充足)

| # | criterion | status |
|---|-----------|--------|
| i | PC-N-9 literal scope §0 明文化 ((N9-4) literal + (N9-5) A + 残 (N9-*) A 採用後) | ✅ |
| ii | 必読 1 件 §1 + pinpoint reference 12 件別記 | ✅ |
| iii | ambiguity 14 件 + AYA literal「全件推奨で OK」record (2026-06-05) | ✅ |
| iv | 採用根拠 14 件明文化 | ✅ |
| v | 実装計画 (a)-(c) 3 step 分解 + 各 step code stub example 添付 | ✅ |
| vi | 実装 phase Exit Criteria 10 項明文化 (§6) | ✅ |
| vii | GATE-B 整合 + MUSEUBO-A 整合 + 設計原則整合 | ✅ |
| viii | 想定改変 file 5 件明文化 | ✅ |
| ix | `indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合 | ✅ |

---

## §6. PC-N-9 実装 phase Exit Criteria (= 10 項、次 session 着手予定)

| # | criterion |
|---|-----------|
| i | GLTFSceneManager::render per-Primitive loop body 冒頭 `setCurrentPrimitive(&primitive)` + 末尾 `clearCurrentPrimitive()` 配線 ((N9-2) A) |
| ii | llvkloader.cpp PC-N-8 (f) block 全体を `static LLCachedControl<bool> sAyastormGltfRealDrawEnabled(gSavedSettings, "AYAGltfRealDrawEnabled", false)` guard で wrap ((N9-1) A + (N9-7) A) |
| iii | settings.xml `AYAGltfRealDrawEnabled` Boolean cvar 1 件追加 (Persist=1, default 0、PC-N-6/7 同形 Comment + Cvar 優先順位 + PC-N-10 deprecate 予定明示) ((N9-4) literal + (N9-5) A) |
| iv | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= `grep -c LL_VULKAN_GLSL indra/llrender/llvkloader.cpp` = 6 不変、PC-N-8 commit `f68fd15e15` 同数) |
| v | MUSEUBO-A 整合 = `AYAGltfRealDrawEnabled=false` default で PC-N-9 経路発火なし、PC-N-8 完了状態と機能等価 + 5 段 graceful degrade |
| vi | build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 ((N9-10) A) |
| vii | tag block 統一 PC-N-9 (a)/(b)/(c) + first-fire LL_INFOS marker (= PC-N-8 (f) 既配線温存) ((N9-8) A) |
| viii | handoff complete doc 起案 + cross-platform spec §6 PC-N-9 行状態 ✅ 反映 + §A 履歴 1 行追記 |
| ix | commit 内容 = 3 modified (indra/) + 1 modified (cross-platform spec) + 1 new doc (complete handoff) + Co-Authored-By 不在 ((N9-12) A) |
| x | self-verify 9 観点 全 ✅ (= PC-N-8 同形) |

### §6.1 integrity check

実装 phase で取得予定 literal:

```
$ grep -c LL_VULKAN_GLSL indra/llrender/llvkloader.cpp
6
$ make -j4 llrender
[100%] Built target llrender
$ ./sharedlibs/bin/INTEGRATION_TEST_lluboringbuffer       → 11/11 PASS YAY!!
$ ./sharedlibs/bin/INTEGRATION_TEST_llassetubopool         → 10/10 PASS YAY!!
$ ./sharedlibs/bin/INTEGRATION_TEST_llpipelinecachestorage → 13/13 PASS YAY!!
$ cd scripts/ubo_codegen && python3 -m unittest discover tests
Ran 131 tests in 0.062s
OK
```

### §6.2 cross-platform spec §6 PC-N-9 行更新内容 (= 別途編集対象)

design-lock 内容更新:

```
| **PC-N-9** | (= **Phase 1.D 内 4th sub-step = GLTFSceneManager::render 統合 + AYAGltfRealDrawEnabled cvar gate**、(N9-1) A 採用 = PC-N-8 (f) block 全体を recordGltfAssetDraw 内 LLCachedControl<bool> guard で wrap + (N9-2) A per-Primitive loop body 冒頭 setCurrentPrimitive + 末尾 clearCurrentPrimitive 配線 (if (rigged) 外) + (N9-3) A setCurrentAsset/clearCurrentAsset は PC-7γ-2 で既配線済確認のみ + (N9-4) literal cvar 命名 AYAGltfRealDrawEnabled + (N9-5) A Boolean default 0 Persist 1 + (N9-6) A AYAGltfStubDrawEnabled prerequisite 継続要 (= PC-N-10 で統合) + (N9-7) A cvar guard 最外側配置 + (N9-8) A 3 tag block (a)/(b)/(c) + (N9-9) A 9+10 項 Exit Criteria + (N9-10) A build verify PC-N-6/7/8 同形 + (N9-11) A cross-platform spec 更新 + (N9-12) A 5 file (indra 3 + doc 2) + (N9-13) A ds loop 二重実行は natural guard 経由 no-op + first-fire marker は PC-N-8 (f) 既配線温存 + (N9-14) A design-lock phase = indra/ 改変 0 件 + 全件推奨 OK record 2026-06-05) | descriptor set 数は PC-N-8 と同 5 set 維持 + setCurrentPrimitive/clearCurrentPrimitive accessor は PC-N-8 (e) で既配線済 + GLTFSceneManager::render 改変は host-side hook 配置ゆえ OS 非依存 + AYAGltfRealDrawEnabled cvar XML は OS 非依存 + cvar guard 構造は OS 非依存 ゆえ macOS 派生 fix 候補なし | full Vulkan ゆえ派生 fix 候補なし想定 | ⏳ design-lock complete 本 commit / 実装 ⏳ |
```

---

## §7. 着手手順 (= 5 step、次 session 実装 phase)

1. 必読 1 件 (本 PC-N-9 design-lock doc) Read + pinpoint reference (= PC-N-8 (f)
   block 既配線 + per-Primitive loop body + settings.xml PC-N-7 cvar pattern) Read
2. (a) gltfscenemanager.cpp per-Primitive loop 内 setCurrentPrimitive/
   clearCurrentPrimitive 配線
3. (b) llvkloader.cpp PC-N-8 (f) block 全体を AYAGltfRealDrawEnabled cvar guard
   で wrap
4. (c) settings.xml AYAGltfRealDrawEnabled cvar 1 件追加
5. build verify literal 取得 (llrender + WARNING 0 + TUT 11+10+13 + codegen
   131/131 + GATE-B integrity) + handoff complete doc 起案 + cross-platform spec
   §6 PC-N-9 状態 ✅ 反映 + §A 履歴 1 行追記 → AYA commit 指示後 commit

---

## §8. 残 strict 線形

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
- ✅ PC-N-8 design-lock + 実装 (commit `f68fd15e15`) (= Phase 1.D 内 3rd sub-step)
- ⏳ **PC-N-9 design-lock ✅ 本 commit / PC-N-9 実装 ⏳ 次 session** (= Phase
  1.D 内 4th sub-step)
- ⏳ PC-N-10 design-lock + 実装 = cleanup + 3 stub cvar deprecate
- ⏳ Phase 1.D complete → Phase 1 全完了 → Mac/Win 開発者補完 phase

---

## §9. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ +
(Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α..PC-7ε ✅ +
PC-N decomposition design-lock ✅ + PC-N-1..PC-N-4 ✅ = Phase 1.C complete ✅ +
PC-8 Linux primary marker ✅ = Phase 1.C strict 線形終了 ✅ +
PC-N-5 ✅ = Phase 1.D 着手起点 ✅ + Phase 1.D decomposition design-lock ✅ +
PC-N-6 ✅ = Phase 1.D 内 1st sub-step ✅ + PC-N-7 ✅ = 2nd sub-step ✅ +
PC-N-8 ✅ = 3rd sub-step ✅ + **PC-N-9 design-lock ✅ 本 commit** + PC-N-9 実装
⏳ + PC-N-10 ⏳ + Phase 1.D complete ⏳

---

## §10. self-verify 9 観点 全 ✅

1. ✅ PC-N-9 literal scope §0 完全分解 4 件 (= AYA 起案文 literal 4 件 直訳)
2. ✅ 必読 1 件 §1 + pinpoint reference 12 件別記 = full file dump なし
   (= `feedback_handoff_minimal_pre_req_read` 整合)
3. ✅ 現状調査 §2 7 項網羅 (= PC-7γ-2 既配線確認 + PC-N-8 (e)/(f) 既配線確認 +
   recordGltfAssetDraw 発火経路 + ds loop 二重実行 + PC-N-6/7 cvar pattern)
4. ✅ ambiguity (N9-1)..(N9-14) 14 件 + AYA literal「全件推奨で OK」record
   (2026-06-05) + 採用根拠 14 件明文化
5. ✅ 実装計画 (a)-(c) 3 step §4 + 各 step code stub example 添付 (=
   setCurrentPrimitive/clearCurrentPrimitive 配線 + AYAGltfRealDrawEnabled
   cvar guard wrap + settings.xml XML stub)
6. ✅ GATE-B 整合 §4.5 (= `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、cvar runtime
   gate のみ、shader 改変ゼロ)
7. ✅ MUSEUBO-A 整合 §4.6 (= AYAGltfRealDrawEnabled=false default + 5 段
   graceful degrade、PC-N-8 完了状態と機能等価)
8. ✅ 設計原則整合 §4.7 (= (1) GLTFSceneManager::render OpenGL path 完全温存 +
   signature 不変 + (2) per-Primitive granularity hook で worker thread 分散
   余地確保)
9. ✅ `indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml
   改変 0 件 = `feedback_design_phase_no_code_write` 整合

---

## §11. 次 session 着手 1 line

PC-N-9 実装着手 = step (a) gltfscenemanager.cpp per-Primitive setCurrentPrimitive/
clearCurrentPrimitive 配線 + (b) llvkloader.cpp PC-N-8 (f) block AYAGltfRealDrawEnabled
cvar guard wrap + (c) settings.xml cvar 1 件追加 + build verify literal 取得 +
handoff complete doc 起案 + AYA commit 指示受領後 commit。
`feedback_ubo_migration_one_at_a_time` 厳格遵守で本 PC-N-8 infrastructure を
baseline に GLTFSceneManager 経由実 draw 通電。

---

## §A. feedback 遵守 record

- ✅ `feedback_proactive_handoff` (本 PC-N-9 design-lock doc 起案)
- ✅ `feedback_handoff_minimal_pre_req_read` (必読 1 件 + pinpoint reference 12 件
  別記、本 session も Read pinpoint のみ = PC-N-8 complete doc 全文 + per-Primitive
  loop body + PC-N-8 (f) block + PC-N-7 (e) cvar pattern + settings.xml AYAGltfStub*
  cvar 配置 + cross-platform spec §6 PC-N-9 行、full file dump なし)
- ✅ `feedback_self_verify_before_handoff` (9 観点 self-verify 全 ✅)
- ✅ `feedback_build_only_verified` (design-lock phase は `indra/` 改変 0 件で
  build verify 対象外、実装 phase で literal 検証取得予定 (N9-10) A 採用)
- ✅ `feedback_no_scope_shrink` (PC-N-9 literal scope §0 完全分解 4 件 = AYA 起案文
  literal が source of truth、(N9-3) A 採用は PC-7γ-2 で既配線済確認結果ゆえ
  「無ければ新設」条件節は不発動 = 縮小ではない、`feedback_ubo_migration_one_at_a_time`
  厳格遵守整合)
- ✅ `feedback_doubt_self_first` (design-lock phase で ambiguity 14 件発見 + 推奨案
  提示 + AYA literal「全件推奨で OK」record 後本 design-lock doc 起案、特に
  (N9-1) cvar gate 配置は 3 候補全列挙 + AYA 起案文 literal「PC-N-8 (f) で配線済
  recordGltfAssetDraw real Asset path を本 cvar gate に従属」直訳整合根拠で A 採用、
  (N9-6) AYAGltfStubDrawEnabled prerequisite 関係は 2 候補列挙 + PC-N-10 統合予定
  根拠で A 採用、推測実装なし)
- ✅ `feedback_confirm_referent_before_acting` (14 件 batch AYA 確認 (2026-06-05)、
  各候補 + 推奨案 + 根拠明示後 AYA literal 一括「全件推奨で OK」record 受領で確定、
  推測実装なし、特に (N9-2) setCurrentPrimitive 配線位置は per-Primitive loop body
  冒頭 vs if (rigged) 内 候補列挙 + setCurrentSkin pattern 同形対称根拠で A 採用)
- ✅ `feedback_ubo_migration_one_at_a_time` 厳格遵守 (PC-N-9 = GLTFSceneManager::render
  統合 + AYAGltfRealDrawEnabled cvar gate 配線単独 sub-step = setCurrentPrimitive
  hook + cvar guard wrap + settings.xml cvar 1 件、PC-N-10 残 1 sub-step は分離
  (= 3 stub cvar deprecate + cleanup は PC-N-10 で別 design-lock)、本 doc 起案も
  PC-N-9 単独 design-lock のみ)
- ✅ `feedback_design_phase_no_code_write` 厳格遵守 (本 PC-N-9 design-lock phase は
  doc 起案のみ、`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 +
  settings.xml 改変 0 件)
- ✅ `feedback_release_branch_workflow` (feature branch `feature/ayastorm-r41-gl-removal`
  上 commit)
- ✅ `feedback_no_auto_commit` (AYA 明示 commit 指示受領後 commit 予定)
- ✅ `feedback_no_claude_coauthor` (Co-Authored-By 行不在)
- ✅ `feedback_no_bare_reference_ids` ((N9-1)..(N9-14) 各 ID に項目名 / 採用案
  内容併記 + (a)..(c) 各 step に作業内容併記)
- ✅ `feedback_tests_dir_never_commit` 整合 (tests/ 改変 0 件、git add 個別
  file 指定予定)
- ✅ memory `project_ayastorm_r41_design_principles` 整合
  ((1) Upstream OpenGL 取り込みやすさ維持 = GLTFSceneManager::render OpenGL path
  完全温存 + drawRangeFast 経路無変更 + `recordGltfAssetDraw` signature 不変
  ((N8-6) A 維持) + sGltfStubAssetPipeline 再利用 + shader 改変ゼロ +
  (2) Core プロセス分散実現 = per-Primitive setCurrentPrimitive/clearCurrentPrimitive
  hook で primitive-level granularity の worker thread 分散余地確保)
- ✅ memory `project_r41_phase1b_vulkan_host_gate` 整合 (GATE-B = `#ifdef LL_VULKAN_GLSL`
  新規追加 0 件、cvar runtime gate のみ ((N9-1) A + (N9-7) A)、count 不変想定)
- ✅ memory `project_ayastorm_three_platforms` 整合 (cross-platform spec §6
  PC-N-9 行 design-lock 内容更新 = `setCurrentPrimitive/clearCurrentPrimitive`
  accessor は PC-N-8 (e) で既配線済 + GLTFSceneManager::render hook 配置は OS 非依存
  + AYAGltfRealDrawEnabled cvar XML は OS 非依存 ゆえ macOS 派生 fix 候補なし、
  Windows full Vulkan ゆえ派生 fix 候補なし、Linux primary 完成 → 他者補完 model 整合)
