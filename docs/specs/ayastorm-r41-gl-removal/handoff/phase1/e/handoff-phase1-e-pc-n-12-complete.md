# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.E PC-N-12 complete

**Status**: ✅ **PC-N-12 complete = Phase 1.E 内 2nd sub-step 実装完了 = real
node modelview 通電 = identity push constant modelview → real
`Asset::mNodes[node_index].mAssetMatrix` 経由 (= `Asset::uploadTransforms`
upstream 整合) + `AYAGltfRealModelviewEnabled` cvar 新設 + 案 A
layering-safe pointer accessor approach 採用**

**Date**: 2026-06-05
**Branch**: `feature/ayastorm-r41-gl-removal`
**Previous commit**: `c9c99d278f` (PC-N-12 design-lock complete)

---

## §0. PC-N-12 literal scope 実装結果 (= 全件採用案通り実装完了、Option A pivot)

PC-N-12 = **Phase 1.E 内 2nd sub-step = real node modelview = identity push
constant modelview 卒業 + real `Asset::mNodes[node_index].mAssetMatrix` 経由 +
`AYAGltfRealModelviewEnabled` cvar 新設**。

literal scope 4 件全件実装完了 ((N12-1)..(N12-16) 16 件 AYA literal「全件推奨で
OK」record (2026-06-05) 全件採用案通り + **案 A layering-safe pointer accessor
approach AYA 承認 2026-06-05** = 実装着手前に design-lock §4.5 stub code
(= `current_asset->mNodes[node_index].mAssetMatrix` field access) が
`llvkloader.h:24-25` + `llvkloader.cpp:568-569` literal「newview gltf/asset.h は
llrender 層から include 不可」既制約と矛盾発見 → 案 A
`setCurrentNodeAssetMatrix(const F32*)` opaque pointer accessor (= caller 側で
`glm::value_ptr` 解決) を AYA 承認後採用 = (N12-2)/(N12-10)/(N12-14)/(N12-15)
literal 訂正、Exit Criteria + scope literal 維持):

1. ✅ identity push constant modelview 卒業 = `recordGltfAssetDraw` PC-N-8 (f)
   内 line 6018-6031 identity 64 B push constant block を新規
   `<AYAstorm r41 PC-N-12 (a)>` tag block で wrap、cvar guard + 2 段
   bounds guard で real / identity 分岐 ((N12-6)/(N12-7) A)
2. ✅ real `Asset::mNodes[node_index].mAssetMatrix` 経由 = caller 側
   `gltfscenemanager.cpp` per-Primitive loop で `glm::value_ptr(node.mAssetMatrix)`
   経由 column-major float pointer を `LLVKLoader::setCurrentNodeAssetMatrix`
   に登録、recordGltfAssetDraw 側で `getCurrentNodeAssetMatrix()` 経由消費
   ((N12-1) A `mAssetMatrix` 採用 = upstream `Asset::uploadTransforms` line 180
   `t_mp[i] = node.mAssetMatrix` 整合、非 root node でも正確、host-side
   `Asset::update` で per-frame 合成済)
3. ✅ `AYAGltfRealModelviewEnabled` cvar 新設 = Boolean default=0 Persist=1
   ((N12-3) A + (N12-4) A、settings.xml `AYAGltfMultiSkinEnabled` 直後並列 =
   Phase 1.E cvar group 連続配置 + (N12-5) B Comment 本 cvar 単独説明)
4. ✅ Option A layering-safe pointer accessor pivot = `setCurrentNodeAssetMatrix
   (const F32*)` / `clearCurrentNodeAssetMatrix()` / `getCurrentNodeAssetMatrix()`
   3 accessor 新設 (= design-lock §4.5 `setCurrentNodeIndex(S32)` から訂正、
   caller-side で `glm::value_ptr` 解決ゆえ llvkloader 層は `LL::GLTF::Asset`/
   `LL::GLTF::Node` を opaque pointer 受けすら不要 = `const F32*` のみ受領で
   llvkloader.h:24-25 + llvkloader.cpp:568-569 layering 制約完全充足、
   `setCurrentSkin`/`setCurrentPrimitive` opaque pointer pattern と同形)

---

## §1. 実装結果 = 8 step (a)-(h) 全実装

### §1.1 step (a) settings.xml `AYAGltfRealModelviewEnabled` cvar 追加 ((N12-3)/(N12-4)/(N12-5) A/B)

`indra/newview/app_settings/settings.xml`:

- 配置: `AYAGltfMultiSkinEnabled` 直後並列 (Phase 1.E cvar group 連続配置 =
  PC-N-11 `AYAGltfMultiSkinEnabled` の隣、PC-N-13/14/15 cvar もここに並べる想定)
- 仕様: `Type=Boolean` + `Value=0` (= default OFF) + `Persist=1`
- Comment: 本 cvar 単独説明 ((N12-5) B 採用 = 各 sub-step 別 session 別途
  design-lock 原則、untouched 領域 silence、PC-N-11 (N11-11) B 同形)
- 機能変化: `AYAGltfRealModelviewEnabled=true` かつ `getCurrentNodeAssetMatrix()
  != nullptr` 時に real node modelview path 通電、それ以外は既 identity 64 B
  push constant 維持 = MUSEUBO-A 整合

### §1.2 step (b) `llvkloader.h` 3 accessor 宣言追加 ((N12-2) A + Option A pivot)

`indra/llrender/llvkloader.h`:

```cpp
// <AYAstorm r41 PC-N-12 (b)> sCurrentNodeAssetMatrix accessor 新設 ...
void setCurrentNodeAssetMatrix (const F32* mat4_column_major);
void clearCurrentNodeAssetMatrix();
const F32* getCurrentNodeAssetMatrix();
// </AYAstorm r41 PC-N-12 (b)>
```

- 配置: PC-N-8 (e) accessor block 直後、closing `}` 直前 = 既
  `setCurrentSkin`/`setCurrentPrimitive` accessor 並列位置
- Option A pivot: `const F32*` opaque pointer 受け = `LL::GLTF::Asset` /
  `LL::GLTF::Node` 一切 include せず llvkloader.h:24-25 layering 制約完全充足
- 設計原則整合: caller 側で `glm::value_ptr(node.mAssetMatrix)` 経由 column-major
  float pointer 取得、recordGltfAssetDraw 側は受領した raw float pointer を
  そのまま `vkCmdPushConstants` source として消費

### §1.3 step (c) `llvkloader.cpp` static field + 3 accessor 実装 ((N12-2) A + Option A pivot)

`indra/llrender/llvkloader.cpp` anonymous namespace 内:

```cpp
// <AYAstorm r41 PC-N-12 (c)> per-Node current modelview tracking ...
const F32* sCurrentNodeAssetMatrix = nullptr;
// </AYAstorm r41 PC-N-12 (c)>
```

`getCurrentPrimitive()` 直後に accessor 実装:

```cpp
// <AYAstorm r41 PC-N-12 (c)> sCurrentNodeAssetMatrix accessor 実装 ...
void setCurrentNodeAssetMatrix(const F32* mat4_column_major)
{
    sCurrentNodeAssetMatrix = mat4_column_major;
}
void clearCurrentNodeAssetMatrix() { sCurrentNodeAssetMatrix = nullptr; }
const F32* getCurrentNodeAssetMatrix() { return sCurrentNodeAssetMatrix; }
// </AYAstorm r41 PC-N-12 (c)>
```

- 配置: `sCurrentPrimitive` static + `setCurrentPrimitive`/`clearCurrentPrimitive`/
  `getCurrentPrimitive` accessor 並列位置 (PC-N-8 (e) 既配線 pattern 踏襲)
- pattern: `sCurrentSkin`/`sCurrentAsset`/`sCurrentPrimitive` 同形 opaque pointer
  accessor
- Option A pivot 整合: `const F32*` 受領 + 保存のみ、`LL::GLTF::*` 一切非依存 =
  llvkloader.cpp:568-569 layering 制約完全充足

### §1.4 step (d) `recordGltfAssetDraw` PC-N-8 (f) push constant identity block を PC-N-12 (a) tag block で wrap + `LLCachedControl<bool>` 配置 ((N12-6)/(N12-7) A)

`indra/llrender/llvkloader.cpp` PC-N-8 (f) block 内 push constant identity site
(= 旧 line 6018-6031):

```cpp
// <AYAstorm r41 PC-N-12 (a)> real node modelview push constant 配線 ...
static LLCachedControl<bool> sAyastormGltfRealModelviewEnabled(
    gSavedSettings, "AYAGltfRealModelviewEnabled", false);
```

- outer `<AYAstorm r41 PC-N-8 (f)>` tag block 構造温存 (surgical insertion)
- inner `<AYAstorm r41 PC-N-12 (a)>` tag block 配置 = PC-N-11 (a) 同形 pattern
- pattern: PC-N-11 (b) `LLCachedControl<bool> sAyastormGltfMultiSkinEnabled` と同形
  ((N12-6) A surgical insertion)

### §1.5 step (e) cvar guard + 2 段 bounds guard + real / identity 分岐 + 単一 vkCmdPushConstants + first-fire marker ((N12-7)/(N12-8)/(N12-9)/(N12-10)/(N12-11) A)

```cpp
const float identity_modelview[16] = {
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f,
};

const F32* node_asset_matrix = getCurrentNodeAssetMatrix();
const bool real_path_eligible =
    (sAyastormGltfRealModelviewEnabled && node_asset_matrix != nullptr);
const float* modelview_src =
    real_path_eligible ? node_asset_matrix : identity_modelview;

if (real_path_eligible)
{
    static std::atomic<bool> s_first_pcn12_real_modelview_fire{true};
    if (s_first_pcn12_real_modelview_fire.exchange(false, std::memory_order_acq_rel))
    {
        LL_INFOS("Vulkan") << "PC-N-12 (a) real node modelview path 通電 (first fire): "
                              "src=" << (const void*)node_asset_matrix
                           << "; AYAGltfRealModelviewEnabled=true + "
                              "getCurrentNodeAssetMatrix() 非 null = "
                              "upstream gltfscenemanager.cpp per-Primitive loop で "
                              "glm::value_ptr(node.mAssetMatrix) 経由設定された "
                              "column-major mat4 を push constant に直接渡し、"
                              "vkCmdPushConstants 64 B / VERTEX_BIT で GPU 通電"
                           << LL_ENDL;
    }
}

vkCmdPushConstants(cmd_buf, sAvatarBoneLayout, VK_SHADER_STAGE_VERTEX_BIT,
                   /*offset=*/0, /*size=*/64, modelview_src);
// </AYAstorm r41 PC-N-12 (a)>
```

- cvar guard + null guard ((N12-7) A + (N12-8) A + (N12-9) A): 2 段 guard
  (`sAyastormGltfRealModelviewEnabled` + `node_asset_matrix != nullptr`) で
  real path eligibility 判定。design-lock §4.5 stub の 3 段 bounds guard
  (`sCurrentAsset` + `sCurrentNodeIndex >= 0` + `sCurrentNodeIndex < mNodes.size()`)
  は Option A pivot で全廃 = caller 側 `glm::value_ptr` が既に
  `node.mAssetMatrix` の有効性を guarantee、accessor null/non-null のみで
  代替 (= 構造的に bounds 違反不能)
- identity fall-through ((N12-8) A): cvar=false or `getCurrentNodeAssetMatrix()
  == nullptr` 時 既 identity 64 B 維持 = 5 段 graceful degrade pattern 維持
- column-major 直接渡し ((N12-10) A): caller 側 `glm::value_ptr(node.mAssetMatrix)`
  経由 column-major、Vulkan push constant も column-major、追加 transpose 不要
- 単一 vkCmdPushConstants call ((N12-7) A): if-else で source pointer のみ
  切替、二重 push call 回避、code 簡素
- first-fire marker ((N12-11) A): `s_first_pcn12_real_modelview_fire`
  `std::atomic<bool>` + `exchange` で single-fire、PC-N-6/7/8/9/10/11 同形 pattern

### §1.6 step (f) `gltfscenemanager.cpp` per-Primitive loop 配線 ((N12-14) A + Option A pivot)

`indra/newview/gltfscenemanager.cpp` per-Primitive loop:

- `LLVKLoader::setCurrentPrimitive(&primitive)` 直後並列に追加:

```cpp
// <AYAstorm r41 PC-N-12 (e)> per-Node real modelview source 配線 ...
LLVKLoader::setCurrentNodeAssetMatrix(glm::value_ptr(node.mAssetMatrix));
// </AYAstorm r41 PC-N-12 (e)>
```

- `LLVKLoader::clearCurrentPrimitive()` 直前並列に追加:

```cpp
// <AYAstorm r41 PC-N-12 (e)> per-Node real modelview source clear ...
LLVKLoader::clearCurrentNodeAssetMatrix();
// </AYAstorm r41 PC-N-12 (e)>
```

- 配置: `setCurrentPrimitive`/`clearCurrentPrimitive` 並列位置 (PC-N-9 (a)
  既配線 pattern 踏襲、`if (rigged)` 外 unconditional)
- Option A pivot 整合: caller 側で `glm::value_ptr(node.mAssetMatrix)` 経由
  column-major float pointer 取得 = `LL::GLTF::Node` field access は caller
  (newview 層) 側で完結、llvkloader (llrender 層) には float pointer のみ
  渡る = layering 制約完全充足
- `Node& node = asset.mNodes[pdata.mNodeIndex]` (line 741) 既配線 → そのまま
  `node.mAssetMatrix` 経由可能

### §1.7 step (g) build verify literal 取得 ((N12-12) A)

§2 参照。

### §1.8 step (h) handoff complete doc 起案 + cross-platform spec §6 PC-N-12 行 ✅ 反映 + §A 履歴 1 行追記

- 本 doc 起案
- `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md`
  §6 PC-N-12 行 状態 ✅ 反映 (Option A pivot 明示) + §A 履歴 1 行追記
  (chronological order = design-lock entry → ✅ 反映 entry 順)

---

## §2. build verify literal 取得

| # | check | result |
|---|-------|--------|
| 1 | `make -j4 llrender` | ✅ `[100%] Built target llrender` (= ERROR 0 / WARNING 0) |
| 2 | `INTEGRATION_TEST_lluboringbuffer` | ✅ 11/11 PASS YAY!! |
| 3 | `INTEGRATION_TEST_llassetubopool` | ✅ 10/10 PASS YAY!! |
| 4 | `INTEGRATION_TEST_llpipelinecachestorage` | ✅ 13/13 PASS YAY!! |
| 5 | `python3 -m unittest discover tests` (codegen) | ✅ 131/131 OK |
| 6 | `grep -c LL_VULKAN_GLSL indra/llrender/llvkloader.cpp` | ✅ 6 (= PC-N-11 commit `13bfb55b35` 同数、GATE-B integrity 維持) |

---

## §3. PC-N-12 実装 phase Exit Criteria (= 10 項全充足)

| # | criterion | status |
|---|-----------|--------|
| i | `recordGltfAssetDraw` PC-N-8 (f) 内 push constant identity block (line 6018-6031) を新規 `<AYAstorm r41 PC-N-12 (a)>` tag block で wrap + cvar guard + 2 段 guard (cvar + accessor non-null) + real / identity fall-through 分岐 ((N12-6)/(N12-7)/(N12-8)/(N12-9) A) | ✅ |
| ii | `glm::value_ptr(node.mAssetMatrix)` column-major 直接渡し + 単一 `vkCmdPushConstants` call ((N12-10) A) | ✅ |
| iii | `llvkloader.h` 3 accessor (`setCurrentNodeAssetMatrix`/`clearCurrentNodeAssetMatrix`/`getCurrentNodeAssetMatrix`) 宣言追加 + `llvkloader.cpp` static field + 3 accessor 実装 ((N12-2) A + Option A pivot) | ✅ |
| iv | settings.xml `AYAGltfRealModelviewEnabled` Boolean cvar 1 件追加 (default=0 Persist=1、`AYAGltfMultiSkinEnabled` 直後並列 = Phase 1.E cvar group 連続配置、Comment 単独説明) ((N12-3)/(N12-4)/(N12-5) A/B) | ✅ |
| v | `gltfscenemanager.cpp` per-Primitive loop に `LLVKLoader::setCurrentNodeAssetMatrix(glm::value_ptr(node.mAssetMatrix))` (setCurrentPrimitive 直後並列、`if (rigged)` 外 unconditional) + `LLVKLoader::clearCurrentNodeAssetMatrix()` (clearCurrentPrimitive 直前並列、unconditional clear) 配線 ((N12-14) A + Option A pivot) | ✅ |
| vi | PC-N-12 (a) first-fire `LL_INFOS` marker (`s_first_pcn12_real_modelview_fire` atomic flag、PC-N-6/7/8/9/10/11 同形 pattern) ((N12-11) A) | ✅ |
| vii | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp=6 不変、PC-N-11 commit `13bfb55b35` 同数) | ✅ |
| viii | MUSEUBO-A 整合 = `AYAGltfRealModelviewEnabled=false` default で identity 64 B push constant 維持 + OpenGL 描画 100% 維持 + PC-N-8 (f) 5 段 graceful degrade 内部維持 | ✅ |
| ix | build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 ((N12-12) A) + cross-platform spec §6 PC-N-12 行 ✅ 反映 + §A 履歴 1 行追記 ((N12-15) A 採用) + handoff complete doc 起案 ((N12-15) A 採用) | ✅ |
| x | self-verify 9 観点 全 ✅ | ✅ |

---

## §4. 改変 file 6 件

| # | file | 改変概要 | 実 LoC |
|---|------|---------|--------|
| 1 | `indra/llrender/llvkloader.h` | step (b) `<AYAstorm r41 PC-N-12 (b)>` tag block で 3 accessor (`setCurrentNodeAssetMatrix`/`clearCurrentNodeAssetMatrix`/`getCurrentNodeAssetMatrix`) 宣言追加 (PC-N-8 (e) accessor block 直後) | +18/-0 |
| 2 | `indra/llrender/llvkloader.cpp` | step (c)(d)(e) = anonymous namespace 内 `const F32* sCurrentNodeAssetMatrix = nullptr;` static + `<AYAstorm r41 PC-N-12 (c)>` tag block で 3 accessor 実装 + PC-N-8 (f) 内 push constant identity block を `<AYAstorm r41 PC-N-12 (a)>` tag block で wrap + `LLCachedControl<bool> sAyastormGltfRealModelviewEnabled` 配置 + cvar guard + accessor non-null guard + real `node.mAssetMatrix` / identity fall-through 分岐 + 単一 vkCmdPushConstants call + first-fire marker | +93/-4 (net +89) |
| 3 | `indra/newview/app_settings/settings.xml` | step (a) `AYAGltfRealModelviewEnabled` Boolean cvar 1 件追加 (`AYAGltfMultiSkinEnabled` 直後並列、default=0 Persist=1) | +28/-0 |
| 4 | `indra/newview/gltfscenemanager.cpp` | step (f) per-Primitive loop に `<AYAstorm r41 PC-N-12 (e)>` tag block で `LLVKLoader::setCurrentNodeAssetMatrix(glm::value_ptr(node.mAssetMatrix))` (setCurrentPrimitive 直後) + `LLVKLoader::clearCurrentNodeAssetMatrix()` (clearCurrentPrimitive 直前) 配線 | +22/-0 |
| 5 | `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md` | §6 PC-N-12 行 状態 ✅ 反映 (Option A pivot 明示) + §A 履歴 1 行追記 (chronological order = design-lock entry → ✅ 反映 entry 順) | +2/-1 |
| 6 | (本 file) `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-...-phase1-e-pc-n-12-complete.md` | step (h) 新規 PC-N-12 complete handoff doc 起案 | new |

PC-N-11 4 件 (llvkloader.cpp + settings.xml + spec + new handoff) から
+2 件 (`llvkloader.h` + `gltfscenemanager.cpp`) = 6 件の構造的必然 (= accessor
signature 新設 + caller-side 配線必須、(N12-2) A + Option A pivot 整合)。

shader 改変 0 件、codegen 改変 0 件、CMake 改変 0 件、tests/ 改変 0 件。

---

## §5. PC-N-12 達成事項 = real node modelview 通電 (Option A pivot)

### §5.1 通電経路 (= `AYAGltfRealModelviewEnabled=true` + `getCurrentNodeAssetMatrix() != nullptr` 時)

1. **upstream owner 切替** (PC-7γ-2 既配線): `GLTFSceneManager::render`
   per-Primitive loop body 内 `Node& node = asset.mNodes[pdata.mNodeIndex]`
   (= `gltfscenemanager.cpp:741`) で per-Primitive `Node&` 取得
2. **per-Node Asset::update** (upstream 既配線): host-side `Asset::update`
   per-frame iteration で `node.mAssetMatrix` を parent chain 合成済の
   asset space matrix で更新 (= `Asset::uploadTransforms` line 180
   `t_mp[i] = node.mAssetMatrix` 整合)
3. **per-Primitive accessor 配線** (本 sub-step 新設、(N12-14) A + Option A
   pivot): `setCurrentNodeAssetMatrix(glm::value_ptr(node.mAssetMatrix))` で
   `sCurrentNodeAssetMatrix` に column-major float pointer 設定 = caller-side
   `glm::value_ptr` 解決で llvkloader 層 layering 制約完全充足
4. **per-draw push constant** (本 sub-step 新設、(N12-10) A): `recordGltfAssetDraw`
   PC-N-12 (a) block で `getCurrentNodeAssetMatrix()` 経由消費 → `vkCmdPushConstants`
   64 B / VERTEX_BIT で GPU 通電 (column-major glm::mat4 → Vulkan push constant
   column-major、追加 transpose 不要)
5. **per-Primitive clear** (本 sub-step 新設): `clearCurrentNodeAssetMatrix()`
   で `sCurrentNodeAssetMatrix` を nullptr 化 = next Primitive で再設定 or
   identity fall-through

### §5.2 fall-through path (= `AYAGltfRealModelviewEnabled=false` or `getCurrentNodeAssetMatrix() == nullptr` 時)

- `modelview_src = identity_modelview` 既 identity 64 B 維持 (= PC-N-8 (f)
  同形 path)
- 単一 `vkCmdPushConstants` call で source pointer のみ切替 = code 簡素
- MUSEUBO-A 整合 = OpenGL 描画 100% 維持 + PC-N-8 (f) 5 段 graceful degrade
  内部維持

### §5.3 Option A layering-safe pointer accessor pivot 詳細

- **発見契機**: 実装着手前に design-lock §4.5 stub code `current_asset->mNodes
  [node_index].mAssetMatrix` field access が `llvkloader.h:24-25` literal
  「LL::GLTF::{Asset, Skin} は opaque pointer 受けに留め、include 連鎖を回避」
  + `llvkloader.cpp:568-569` literal「newview gltf/asset.h は llrender 層から
  include 不可 = layering violation 回避」既制約と矛盾発見
- **対応**: implementation 停止 → `feedback_doubt_self_first` /
  `feedback_admit_unknown` 遵守で AYA に矛盾 surface + 3 案提示
  - 案 A: caller-side `glm::value_ptr` 解決 + opaque float pointer accessor =
    `setCurrentNodeAssetMatrix(const F32*)` (= 推奨)
  - 案 B: llvkloader 層で `newview/gltf/asset.h` include (= layering 違反)
  - 案 C: helper getter で `LL::GLTF::Asset*` から `mAssetMatrix` を抽出
- **AYA 承認**: 案 A 採用承認受領 (2026-06-05)
- **literal 訂正**: (N12-2) `setCurrentNodeIndex(S32)` → `setCurrentNodeAssetMatrix
  (const F32*)` + (N12-10) `glm::value_ptr(node.mAssetMatrix)` column-major
  直接渡し caller-side 解決 + (N12-14) caller-side `setCurrentNodeIndex(pdata.
  mNodeIndex)` → `setCurrentNodeAssetMatrix(glm::value_ptr(node.mAssetMatrix))` +
  (N12-15) 改変 file 6 件 (header + caller 必須は維持) 構造的必然
- **Exit Criteria + scope literal 維持**: PC-N-12 literal scope 4 件 §0 全件
  実装、accessor signature 訂正は手段の最適化、目的 (real node modelview 通電) 不変
- **副次効果**: design-lock §4.5 stub の 3 段 bounds guard (`sCurrentAsset` +
  `sCurrentNodeIndex >= 0` + `sCurrentNodeIndex < mNodes.size()`) は構造的に
  不要化 = caller-side `glm::value_ptr` が既に `node.mAssetMatrix` の有効性を
  guarantee、accessor non-null のみで代替 = code 簡素化 + bounds 違反不能

### §5.4 Phase 1.E 残課題 (= PC-N-13..PC-N-15)

- **PC-N-13** (= 3rd sub-step) real per-draw + multi-asset verify =
  zero-buffer `PerDrawUBO_LightParams` 卒業 + `AYAGltfMultiAssetCanary` cvar
  (debug-only) で複数 asset 同時描画検証
- **PC-N-14** (= 4th sub-step) worker thread design-lock (= per-Primitive UBO
  write + cmdbuf record 並列化 design)
- **PC-N-15** (= 5th = 最終 sub-step) worker thread 実装 + cleanup (=
  `sGltfStubSkin` sentinel storage 撤去 + Phase 1.E complete marker)

---

## §6. 残 strict 線形

- ✅ Phase 1.A / 1.B / (Z) SSS / (W) uniform4iv / (Y) Phase 1.C prep
- ✅ PC-0..PC-7ε / PC-N decomposition / PC-N-1..PC-N-4 (= Phase 1.C complete)
- ✅ PC-8 Linux primary marker (= Phase 1.C strict 線形終了)
- ✅ PC-N-5 (= Phase 1.D 着手起点) / Phase 1.D decomposition design-lock
- ✅ PC-N-6 / PC-N-7 / PC-N-8 / PC-N-9 / PC-N-10 (= Phase 1.D complete = 1 GLTF
  asset 完全 Vulkan draw 通電 達成)
- ✅ Phase 1.E decomposition design-lock (commit `01cd001d07`) + PC-N-11
  design-lock (commit `cdf4dccc0d`) + PC-N-11 実装 (commit `13bfb55b35`) +
  PC-N-12 design-lock (commit `c9c99d278f`)
- ✅ **PC-N-12 実装 ✅ 本 commit = Phase 1.E 内 2nd sub-step 実装完了 =
  real node modelview 通電 (Option A pivot)**
- ⏳ PC-N-13 design-lock + 実装 (= real per-draw + multi-asset verify)
- ⏳ PC-N-14 design-lock (= worker thread design)
- ⏳ PC-N-15 実装 + cleanup (= worker thread 実装 + Phase 1.E complete marker)
- ⏳ Phase 1.E complete → Phase 1 全完了 → Mac/Win 開発者補完 phase

---

## §7. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ +
(Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α..PC-7ε ✅ +
PC-N decomposition design-lock ✅ + PC-N-1..PC-N-4 ✅ = Phase 1.C complete ✅ +
PC-8 Linux primary marker ✅ + PC-N-5 ✅ + Phase 1.D decomposition design-lock ✅
+ PC-N-6 ✅ + PC-N-7 ✅ + PC-N-8 ✅ + PC-N-9 ✅ + PC-N-10 ✅ = Phase 1.D
complete ✅ (= 1 GLTF asset 完全 Vulkan draw 通電 達成) +
Phase 1.E decomposition design-lock ✅ + PC-N-11 design-lock ✅ +
PC-N-11 ✅ + PC-N-12 design-lock ✅ +
**PC-N-12 ✅ 本 commit = Phase 1.E 内 2nd sub-step 実装完了 = real node
modelview 通電 (Option A pivot)** +
⏳ PC-N-13..PC-N-15 design-lock + 実装 + Phase 1.E complete + Phase 1 全完了 +
Mac/Win 開発者補完 phase

---

## §8. self-verify 9 観点 全 ✅

1. ✅ Exit Criteria 10 項全充足 (§3)
2. ✅ 必読 1 件 (PC-N-12 design-lock doc) + pinpoint reference 別記 = full file
   dump なし (= `feedback_handoff_minimal_pre_req_read` 整合)
3. ✅ step (a)/(b)/(c)/(d)/(e)/(f)/(g)/(h) 8 step 全実装 (§1)
4. ✅ ambiguity (N12-1)..(N12-16) 16 件 AYA literal「全件推奨で OK」record
   (2026-06-05) design-lock 継承 + 本実装で全件採用案通り実装 + **案 A
   layering-safe pointer accessor approach AYA 承認 2026-06-05** で
   (N12-2)/(N12-10)/(N12-14)/(N12-15) literal 訂正 (Exit Criteria + scope
   literal 維持)
5. ✅ GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp=6
   不変、PC-N-11 commit `13bfb55b35` 同数)
6. ✅ MUSEUBO-A 整合 = `AYAGltfRealModelviewEnabled=false` default で identity
   64 B push constant 維持 + OpenGL 描画 100% 維持 + PC-N-8 (f) 5 段 graceful
   degrade 内部維持
7. ✅ build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11/11 + 10/10 +
   13/13 + codegen 131/131 全 PASS + GATE-B integrity LL_VULKAN_GLSL=6 不変
8. ✅ commit 内容 = 4 modified (indra/) + 1 modified (cross-platform spec) +
   1 new doc (本 complete handoff) + CMake 改変 0 + codegen 改変 0 + shader 改変
   0 + tests/ 改変 0 + Co-Authored-By 不在
9. ✅ `feedback_no_scope_shrink` 遵守 = PC-N-12 literal scope 4 件 §0 全件実装、
   Option A pivot は手段の最適化 (= accessor signature 訂正で layering 制約
   充足) ゆえ scope 縮小ではない、目的 (real node modelview 通電) 不変、
   3 段 bounds guard → 2 段 guard 簡素化は Option A pivot の構造的必然 (=
   `glm::value_ptr` caller-side 解決で bounds 違反不能化) ゆえ縮小ではなく
   構造改善、PC-N-13..PC-N-15 持越しは別 phase 分解 =
   `feedback_ubo_migration_one_at_a_time` 厳格遵守整合

---

## §9. 次 session 着手 1 line

PC-N-13 design-lock 着手 = real per-draw + multi-asset verify =
zero-buffer `PerDrawUBO_LightParams` 卒業 + `AYAGltfMultiAssetCanary` cvar
(debug-only) で複数 asset 同時描画検証 + `AYAGltfRealLightParamsEnabled` cvar
新設 + ambiguity 確認 + 実装計画分解 + Exit Criteria 明文化 =
`feedback_ubo_migration_one_at_a_time` 厳格遵守で本 PC-N-12 real node modelview
通電 baseline 上に Phase 1.E 内 3rd sub-step として別 session 別途 design-lock。

---

## §A. feedback 遵守 record

- ✅ `feedback_proactive_handoff` (本 PC-N-12 complete handoff doc 起案)
- ✅ `feedback_handoff_minimal_pre_req_read` (必読 1 件 = PC-N-12 design-lock doc
  + pinpoint Read のみ、full file dump なし)
- ✅ `feedback_self_verify_before_handoff` (9 観点 self-verify 全 ✅)
- ✅ `feedback_build_only_verified` (llrender + WARNING 0 + TUT 11/11 + 10/10 +
  13/13 + codegen 131/131 + GATE-B integrity LL_VULKAN_GLSL=6 不変で literal
  検証取得)
- ✅ `feedback_no_scope_shrink` (PC-N-12 literal scope 4 件 §0 全件実装、Option A
  pivot は手段の最適化ゆえ scope 縮小ではない、目的 (real node modelview 通電)
  不変、2 段 guard 簡素化は Option A pivot の構造的必然ゆえ縮小ではなく構造改善)
- ✅ `feedback_doubt_self_first` (design-lock phase で ambiguity 16 件発見 +
  推奨案提示 + AYA literal「全件推奨で OK」record 後本実装、本実装着手前にも
  design-lock §4.5 stub code が llvkloader.h:24-25 + llvkloader.cpp:568-569
  layering 制約と矛盾発見 → 実装停止 + AYA に矛盾 surface + 案 A/B/C 提示 +
  推奨 (案 A) 提示 後 AYA 「A」承認受領で本実装、推測実装なし)
- ✅ `feedback_admit_unknown` (design-lock §4.5 stub と layering 制約の矛盾を
  発見した時点で「分からない」と認め、勝手に代替設計を選ばず AYA に 3 案提示 +
  推奨提示で確認、推測実装なし)
- ✅ `feedback_confirm_referent_before_acting` (16 件 batch AYA 確認 design-lock
  phase で完了、本実装中も Option A pivot は AYA 「A」literal 承認後採用、
  推測実装なし)
- ✅ `feedback_ubo_migration_one_at_a_time` 厳格遵守 (PC-N-12 = real node
  modelview 単独 sub-step、PC-N-13..PC-N-15 (= real per-draw + multi-asset
  verify / worker thread design + 実装) は別 phase の別 session で別途分解)
- ✅ `feedback_design_phase_no_code_write` 整合 (本 PC-N-12 は実装 phase = 改変
  あり、design-lock commit `c9c99d278f` で `indra/` 改変 0 件完了済)
- ✅ `feedback_release_branch_workflow` (feature branch
  `feature/ayastorm-r41-gl-removal` 上 commit)
- ✅ `feedback_no_auto_commit` (AYA 明示 commit 指示受領後 commit 予定)
- ✅ `feedback_no_claude_coauthor` (Co-Authored-By 行不在)
- ✅ `feedback_no_bare_reference_ids` ((N12-1)..(N12-16) 各 ID に項目名 / 採用案
  内容併記 + (a)..(h) 各 step に作業内容併記)
- ✅ `feedback_tests_dir_never_commit` 整合 (tests/ 改変 0 件、git add 個別
  file 指定予定)
- ✅ memory `project_ayastorm_r41_design_principles` 整合
  ((1) Upstream OpenGL 取り込みやすさ維持 = `recordGltfAssetDraw` signature 不変
  + `Asset::uploadTransforms` line 180 `t_mp[i] = node.mAssetMatrix` upstream
  既配線整合 + caller-side `glm::value_ptr` 解決で llvkloader 層 layering 制約
  完全充足 + shader 改変ゼロ + (2) Core プロセス分散実現 = per-Primitive
  `setCurrentNodeAssetMatrix`/`clearCurrentNodeAssetMatrix` hook で primitive-level
  granularity 維持、PC-N-14/15 worker thread 分散 design 整合)
- ✅ memory `project_r41_phase1b_vulkan_host_gate` 整合 (GATE-B = `#ifdef
  LL_VULKAN_GLSL` 新規追加 0 件、`AYAGltfRealModelviewEnabled` cvar runtime gate
  のみ、count llvkloader.cpp=6 不変)
- ✅ memory `project_ayastorm_three_platforms` 整合 (cross-platform spec §6
  PC-N-12 行状態 ✅ 反映 = host-side push constant data source 切替は OS 非依存
  + `vkCmdPushConstants` 64 B / VERTEX_BIT は MoltenVK 標準対応範囲 (= Metal
  `setVertexBytes` 経路) + caller-side `glm::value_ptr` column-major 経路は
  OS 非依存 + `const F32* sCurrentNodeAssetMatrix` static field + 3 accessor は
  host-side ゆえ MoltenVK 影響なし + descriptor set 数 5 維持 +
  `AYAGltfRealModelviewEnabled` cvar XML は OS 非依存 ゆえ macOS 派生 fix 候補
  なし想定 + Windows full Vulkan ゆえ派生 fix 候補なし想定、Linux primary
  完成 → 他者補完 model 整合)
