# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.E PC-N-13 complete

**Status**: ✅ **PC-N-13 complete = Phase 1.E 内 3rd sub-step 実装完了 = real
per-draw light params cvar gate 通電 +「zero IS real data」semantic 確立 +
multi-asset GLTF draw canary 配線 + `AYAGltfRealLightParamsEnabled` +
`AYAGltfMultiAssetCanary` 2 cvar 新設**

**Date**: 2026-06-05
**Branch**: `feature/ayastorm-r41-gl-removal`
**Previous commit**: `e13d00d4a6` (PC-N-13 design-lock complete)

---

## §0. PC-N-13 literal scope 実装結果 (= 全件採用案通り実装完了)

PC-N-13 = **Phase 1.E 内 3rd sub-step = real per-draw + multi-asset verify =
zero-buffer `PerDrawUBO_LightParams` 卒業 ((N13-1) ⭐ C 採用 =「zero IS real
data」semantic 確立) + `AYAGltfMultiAssetCanary` cvar (debug-only) で複数 asset
同時描画検証 + `AYAGltfRealLightParamsEnabled` cvar 新設**。

literal scope 4 件全件実装完了 ((N13-1)..(N13-16) 16 件 AYA literal「全件推奨で
OK」record (2026-06-05) 全件採用案通り):

1. ✅ zero-buffer `PerDrawUBO_LightParams` 卒業 = PC-N-8 (f) 内 writeDrawUbo
   site (= line 5984-5995) を新規 `<AYAstorm r41 PC-N-13 (a)>` tag block で
   wrap + `LLCachedControl<bool> sAyastormGltfRealLightParamsEnabled` 配置 +
   cvar guard 内「zero IS real data」first-fire `LL_INFOS` marker 起動
   ((N13-1) ⭐ C 採用 = sky_smoke shader `sGltfStubAssetPipeline` 流用
   architectural truth 尊重 + Phase 1.F+ 実 PBR shader 接続時に PC-N-13.1
   等で data 内容置換着手予定)
2. ✅ `AYAGltfMultiAssetCanary` cvar (debug-only) 新設 = PC-N-12 (a) closing
   tag 直後に `<AYAstorm r41 PC-N-13 (b)>` multi-asset canary marker block +
   file-static `std::unordered_set<const void*> sPcn13MultiAssetSeen` で seen
   asset address tracking + size>1 検出 first-fire `LL_INFOS` marker
   ((N13-11) A + (N13-3) A debug-only 整合)
3. ✅ `AYAGltfRealLightParamsEnabled` cvar 新設 = settings.xml `AYAGltfRealModelviewEnabled`
   直後並列に Boolean default=0 Persist=1 追加 ((N13-2) A + (N13-4) A +
   (N13-5) A + (N13-7) A Comment 単独説明)
4. ✅ Phase 1.E cvar group 連続配置 = `AYAGltfMultiAssetCanary` を
   `AYAGltfRealLightParamsEnabled` 直後並列に配置 ((N13-6) A 末尾追加)

---

## §1. 実装結果 = 7 step (a)-(g) 全実装

### §1.1 step (a) settings.xml `AYAGltfRealLightParamsEnabled` + `AYAGltfMultiAssetCanary` 2 cvar 追加 ((N13-2)/(N13-3)/(N13-4)/(N13-5)/(N13-6)/(N13-7) A)

`indra/newview/app_settings/settings.xml`:

- 配置: `AYAGltfRealModelviewEnabled` 直後並列 = Phase 1.E cvar group 連続配置
  (PC-N-11 `AYAGltfMultiSkinEnabled` → PC-N-12 `AYAGltfRealModelviewEnabled` →
  PC-N-13 `AYAGltfRealLightParamsEnabled` + `AYAGltfMultiAssetCanary` の順)
- 仕様: 各 cvar `Type=Boolean` + `Value=0` (= default OFF) + `Persist=1`
- Comment: 各 cvar 単独説明 ((N13-7) A 採用 = PC-N-11 (N11-11) B + PC-N-12
  (N12-5) B 同形、untouched 領域 silence)
- 機能変化: `AYAGltfRealLightParamsEnabled=true` 時に「zero IS real data」
  semantic 通電 first-fire log 出力、data path は unchanged (zero buffer
  write 維持 = sky_smoke shader 非 consume architectural truth 整合) =
  MUSEUBO-A 整合
- `AYAGltfMultiAssetCanary=true` 時に PC-N-8 (f) real Asset path で seen
  asset address tracking + 2nd 以降 asset 検出時 first-fire log 出力 =
  debug-only 機能 ((N13-3) A 整合)

### §1.2 step (b) `llvkloader.cpp` `#include <unordered_set>` + file-static `sPcn13MultiAssetSeen` ((N13-16) A 改変 1 件目)

`indra/llrender/llvkloader.cpp`:

- 冒頭 include block に `#include <unordered_set>` 追加 (= `std::unordered_set
  <const void*>` 使用のため)
- anonymous namespace 内 `sCurrentNodeAssetMatrix` static 直後に新規
  `<AYAstorm r41 PC-N-13 (b)>` tag block 配置:

```cpp
// <AYAstorm r41 PC-N-13 (b)> multi-asset canary 用 seen asset address tracker
//   ((N13-11) A、AYA literal「全件推奨で OK」record 2026-06-05)。
//   main thread 専有 (recordGltfAssetDraw は GLTFSceneManager::render から呼出)
//   ゆえ mutex 不要。AYAGltfMultiAssetCanary cvar=ON 時のみ insert/size 走査、
//   debug-only ゆえ steady state cost 0 (= cvar=OFF default で全 access skip)。
//   address のみ追跡で Asset content 一切 deref せず llvkloader 層 layering
//   制約完全充足 (= newview/gltf/asset.h include 不要、const void* opaque
//   pointer 比較のみ、deref せず layering 制約完全充足)。
std::unordered_set<const void*> sPcn13MultiAssetSeen;
// </AYAstorm r41 PC-N-13 (b)>
```

- 配置: `sCurrentNodeAssetMatrix` 直後並列 = PC-N-12 (c) accessor 並列位置
- pattern: file-static + anonymous namespace + main thread 専有 (=
  GLTFSceneManager::render → recordGltfAssetDraw single thread 呼出)
- layering 制約: `const void*` opaque pointer 追跡のみ、Asset 内容 deref せず
  = llvkloader.cpp:568-569 layering 制約完全充足

### §1.3 step (c) PC-N-8 (f) writeDrawUbo site を PC-N-13 (a) tag block で wrap + cvar guard + first-fire marker ((N13-8)/(N13-9)/(N13-10) A、(N13-1) ⭐ C 採用)

`indra/llrender/llvkloader.cpp` PC-N-8 (f) block 内 writeDrawUbo zero buffer
site (= 旧 line 5984-5995):

```cpp
// PC-N-8 (f) 同形 per-draw UBO 配線 (= PC-N-7 (e) 同形 sequence)。
// <AYAstorm r41 PC-N-13 (a)> real per-draw light params cvar gate
//   ((N13-1) C 採用 = sky_smoke shader 非 consume architectural truth
//   尊重 +「zero IS real data」semantic 確立、AYA literal「全件推奨で
//   OK」record 2026-06-05)。
//   sGltfStubAssetPipeline (= sky_smoke shader 流用) は PerDrawUBO_LightParams
//   を非 consume = 現 phase で 256 B host write zero buffer は descriptor set
//   layout 充足のための architectural truth (= 設計通り、bug ではない)。
//   AYAGltfRealLightParamsEnabled cvar=ON 時 first-fire LL_INFOS marker 起動
//   + log で「現 phase は zero IS real data = sky_smoke shader 非
//   consume」明示。Phase 1.F+ 実 PBR shader 接続時に data 内容置換
//   (PC-N-13.1)。writeDrawUbo 自体は unconditional 呼出 (=
//   dynamic_offset 構築は両 path 必須、ring buffer allocate 経路必須、
//   cvar gate は marker 起動のみ作用、data path は unchanged)。
static LLCachedControl<bool> sAyastormGltfRealLightParamsEnabled(
    gSavedSettings, "AYAGltfRealLightParamsEnabled", false);

static const U8 real_asset_draw_zero_buf[256] = {};
U32 real_asset_dynamic_offset = 0u;
LLVKLoader::writeDrawUbo(
    ubo::block_hash::PerDrawUBO_LightParams,
    /*offset=*/0u, real_asset_draw_zero_buf,
    sizeof(real_asset_draw_zero_buf),
    real_asset_dynamic_offset);

// PC-N-13 (a) first-fire LL_INFOS marker ((N13-10) A、PC-N-6/7/8/9/
//   10/11/12 同形 pattern)。zero IS real data semantic 通電 literal
//   取得用 (= sky_smoke shader 非 consume architectural truth 記録)。
if (sAyastormGltfRealLightParamsEnabled)
{
    static std::atomic<bool> s_first_pcn13_real_light_params_fire{true};
    if (s_first_pcn13_real_light_params_fire.exchange(false, std::memory_order_acq_rel))
    {
        LL_INFOS("Vulkan") << "PC-N-13 (a) real per-draw light params cvar gate 通電 (first fire): "
                              "AYAGltfRealLightParamsEnabled=true; "
                              "現 phase は zero IS real data 解釈 ((N13-1) C 採用) = "
                              "sGltfStubAssetPipeline (sky_smoke shader 流用) は "
                              "PerDrawUBO_LightParams を非 consume = "
                              "host write 256 B zero buffer が descriptor set layout 充足 "
                              "architectural truth (llvkloader.cpp:5843-5845 既明示)。"
                              "Phase 1.F+ 実 PBR shader 接続時に PC-N-13.1 等で data 内容置換着手予定。"
                           << LL_ENDL;
    }
}
// </AYAstorm r41 PC-N-13 (a)>
```

- outer `<AYAstorm r41 PC-N-8 (f)>` tag block 構造温存 (surgical insertion)
- inner `<AYAstorm r41 PC-N-13 (a)>` tag block 配置 = PC-N-11 (a) / PC-N-12 (a)
  同形 pattern
- `LLCachedControl<bool>` 配置 ((N13-9) A): PC-N-13 (a) tag block 内に
  `sAyastormGltfRealLightParamsEnabled` 配置 = real path 発火時のみ first-fire
  marker 起動、data 内容引続 zero (案 C 採用)
- writeDrawUbo unconditional 呼出: cvar gate は marker 起動のみ作用、data path
  unchanged = MUSEUBO-A 整合 + GATE-B 整合
- first-fire marker ((N13-10) A): `s_first_pcn13_real_light_params_fire`
  `std::atomic<bool>` + `exchange` で single-fire、PC-N-6/7/8/9/10/11/12 同形
  pattern、log 内容で「現 phase は zero IS real data = sky_smoke shader 非
  consume」明示

### §1.4 step (d) PC-N-13 (b) multi-asset canary marker block 配置 ((N13-11) A + (N13-3) A)

`indra/llrender/llvkloader.cpp` PC-N-12 (a) closing tag 直後 (= PC-N-8 (f)
real Asset path 内、PC-N-13 (a) 隣接):

```cpp
// </AYAstorm r41 PC-N-12 (a)>

// <AYAstorm r41 PC-N-13 (b)> multi-asset GLTF draw canary marker
//   ((N13-11) A、AYA literal「全件推奨で OK」record 2026-06-05)。
//   AYAGltfMultiAssetCanary cvar=ON 時 PC-N-8 (f) real Asset path で
//   seen asset address tracking + 2nd 以降 asset 検出時 first-fire
//   LL_INFOS marker 1 回 (= debug-only 機能、(N13-3) A 整合)。
//   address のみ追跡で Asset content 一切 deref せず llvkloader 層
//   layering 制約完全充足。
static LLCachedControl<bool> sAyastormGltfMultiAssetCanary(
    gSavedSettings, "AYAGltfMultiAssetCanary", false);
if (sAyastormGltfMultiAssetCanary)
{
    sPcn13MultiAssetSeen.insert(static_cast<const void*>(asset));
    if (sPcn13MultiAssetSeen.size() > 1u)
    {
        static std::atomic<bool> s_first_pcn13_multi_asset_canary_fire{true};
        if (s_first_pcn13_multi_asset_canary_fire.exchange(false, std::memory_order_acq_rel))
        {
            LL_INFOS("Vulkan") << "PC-N-13 (b) multi-asset GLTF draw canary 発火 (first fire): "
                                  "seen_asset_count=" << sPcn13MultiAssetSeen.size()
                               << ", current_asset=" << static_cast<const void*>(asset)
                               << "; AYAGltfMultiAssetCanary=true で recordGltfAssetDraw real Asset path に "
                                  "2 件目以降の asset 到達を確認 = multi-asset GLTF draw 通電 literal 取得 "
                                  "((N13-12) A integration approach 整合、real SL sample 信任、"
                                  "synthetic 不要、既存 SL inv 複数 GLTF asset 同時 rez で発火)。"
                               << LL_ENDL;
        }
    }
}
// </AYAstorm r41 PC-N-13 (b)>
```

- 配置: PC-N-12 (a) closing tag 直後 = PC-N-13 (a) と並列、PC-N-8 (f) real
  Asset path 内 ((N13-11) A 隣接配置)
- `LLCachedControl<bool> sAyastormGltfMultiAssetCanary` 配置 = debug-only 機能、
  default OFF
- `sPcn13MultiAssetSeen.insert(static_cast<const void*>(asset))` = address
  のみ追跡、Asset content 一切 deref せず
- `size() > 1u` 検出時 first-fire marker = 1 件目 asset では発火せず、2 件目
  以降 asset 到達で 1 回のみ発火
- first-fire marker: `s_first_pcn13_multi_asset_canary_fire` `std::atomic<bool>`
  + `exchange` で single-fire、PC-N-6/7/8/9/10/11/12 同形 pattern
- log 内容: `seen_asset_count` + `current_asset` address で multi-asset 通電
  literal 取得 ((N13-12) A integration approach 整合)

### §1.5 step (e) build verify literal 取得 ((N13-14) A)

§2 参照。

### §1.6 step (f) cross-platform spec §6 PC-N-13 行 ✅ 反映 + §A 履歴 1 行追記

- `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md`
  §6 PC-N-13 行 状態 ✅ 反映 (= 実装 complete = llrender PASS + WARNING 0 +
  TUT 11+10+13 + codegen 131/131 + GATE-B integrity `LL_VULKAN_GLSL` count
  llvkloader.cpp=6 不変)
- §A 履歴 1 行追記 (chronological order = design-lock entry → ✅ 反映 entry 順)

### §1.7 step (g) handoff complete doc 起案

- 本 doc 起案

---

## §2. build verify literal 取得

| # | check | result |
|---|-------|--------|
| 1 | `make -j4 llrender` | ✅ `[100%] Built target llrender` (= ERROR 0 / WARNING 0) |
| 2 | `INTEGRATION_TEST_lluboringbuffer` | ✅ 11/11 PASS YAY!! |
| 3 | `INTEGRATION_TEST_llassetubopool` | ✅ 10/10 PASS YAY!! |
| 4 | `INTEGRATION_TEST_llpipelinecachestorage` | ✅ 13/13 PASS YAY!! |
| 5 | `python3 -m unittest discover tests` (codegen at `scripts/ubo_codegen/`) | ✅ 131/131 OK |
| 6 | `grep -c LL_VULKAN_GLSL indra/llrender/llvkloader.cpp` | ✅ 6 (= PC-N-12 commit `4373c302d7` 同数、GATE-B integrity 維持) |

---

## §3. PC-N-13 実装 phase Exit Criteria (= 10 項全充足)

| # | criterion | status |
|---|-----------|--------|
| i | `recordGltfAssetDraw` PC-N-8 (f) 内 writeDrawUbo zero buffer site (line 5984-5995) を新規 `<AYAstorm r41 PC-N-13 (a)>` tag block で wrap + `LLCachedControl<bool> sAyastormGltfRealLightParamsEnabled` 配置 + cvar guard 内「zero IS real data」first-fire `LL_INFOS` marker 起動 ((N13-1) ⭐ C 採用 + (N13-8)/(N13-9)/(N13-10) A) | ✅ |
| ii | PC-N-12 (a) closing tag 直後に新規 `<AYAstorm r41 PC-N-13 (b)>` multi-asset canary marker block + `LLCachedControl<bool> sAyastormGltfMultiAssetCanary` 配置 + `sPcn13MultiAssetSeen.insert(static_cast<const void*>(asset))` + `size() > 1u` 検出 first-fire `LL_INFOS` marker ((N13-11) A + (N13-3) A debug-only) | ✅ |
| iii | `llvkloader.cpp` 冒頭 `#include <unordered_set>` 追加 + anonymous namespace 内 `sCurrentNodeAssetMatrix` 直後に `<AYAstorm r41 PC-N-13 (b)>` tag block で `std::unordered_set<const void*> sPcn13MultiAssetSeen` file-static 配置 (= main thread 専有ゆえ mutex 不要、`const void*` opaque pointer 追跡のみで layering 制約完全充足) | ✅ |
| iv | settings.xml `AYAGltfRealLightParamsEnabled` + `AYAGltfMultiAssetCanary` 2 Boolean cvar 追加 (default=0 Persist=1、`AYAGltfRealModelviewEnabled` 直後並列 = Phase 1.E cvar group 連続配置、各 cvar 単独説明) ((N13-2)/(N13-3)/(N13-4)/(N13-5)/(N13-6)/(N13-7) A) | ✅ |
| v | PC-N-13 (a) + PC-N-13 (b) first-fire `LL_INFOS` marker 2 件 (`s_first_pcn13_real_light_params_fire` + `s_first_pcn13_multi_asset_canary_fire` atomic flag、PC-N-6/7/8/9/10/11/12 同形 pattern) ((N13-10) A) | ✅ |
| vi | writeDrawUbo unconditional 呼出維持 = cvar gate は marker 起動のみ作用、data path unchanged ((N13-1) ⭐ C 採用 = sky_smoke shader 非 consume architectural truth 尊重 +「zero IS real data」semantic 確立、Phase 1.F+ 実 PBR shader 接続時に PC-N-13.1 で data 内容置換予定) | ✅ |
| vii | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp=6 不変、PC-N-12 commit `4373c302d7` 同数) | ✅ |
| viii | MUSEUBO-A 整合 = `AYAGltfRealLightParamsEnabled=false` + `AYAGltfMultiAssetCanary=false` default で writeDrawUbo zero buffer write 維持 + canary skip + OpenGL 描画 100% 維持 + PC-N-8 (f) 5 段 graceful degrade 内部維持 | ✅ |
| ix | build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 ((N13-14) A) + cross-platform spec §6 PC-N-13 行 ✅ 反映 + §A 履歴 1 行追記 + handoff complete doc 起案 ((N13-16) A 採用) | ✅ |
| x | self-verify 9 観点 全 ✅ | ✅ |

---

## §4. 改変 file 4 件

| # | file | 改変概要 | 実 LoC |
|---|------|---------|--------|
| 1 | `indra/llrender/llvkloader.cpp` | step (b)(c)(d) = 冒頭 `#include <unordered_set>` + anonymous namespace 内 `sCurrentNodeAssetMatrix` 直後に `<AYAstorm r41 PC-N-13 (b)>` tag block で `std::unordered_set<const void*> sPcn13MultiAssetSeen` file-static 配置 + PC-N-8 (f) 内 writeDrawUbo zero buffer site を `<AYAstorm r41 PC-N-13 (a)>` tag block で wrap + `LLCachedControl<bool>` 2 件宣言 (`sAyastormGltfRealLightParamsEnabled` + `sAyastormGltfMultiAssetCanary`) + cvar guard + PC-N-13 (a) first-fire marker + PC-N-12 (a) closing tag 直後に PC-N-13 (b) multi-asset canary block + `sPcn13MultiAssetSeen.insert` + size>1 検出 + PC-N-13 (b) first-fire marker | net 大幅追加 |
| 2 | `indra/newview/app_settings/settings.xml` | step (a) `AYAGltfRealLightParamsEnabled` + `AYAGltfMultiAssetCanary` 2 Boolean cvar 追加 (`AYAGltfRealModelviewEnabled` 直後並列、default=0 Persist=1、各 cvar 単独説明) | +52/-0 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md` | §6 PC-N-13 行 状態 ✅ 反映 + §A 履歴 1 行追記 (chronological order = design-lock entry → ✅ 反映 entry 順) | +2/-1 |
| 4 | (本 file) `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-...-phase1-e-pc-n-13-complete.md` | step (g) 新規 PC-N-13 complete handoff doc 起案 | new |

PC-N-12 6 件 (llvkloader.h + llvkloader.cpp + settings.xml + gltfscenemanager.cpp
+ spec + new handoff) から -2 件 (`llvkloader.h` 不要 + `gltfscenemanager.cpp`
不要) = 4 件 (= accessor signature 新設なし + caller-side 配線なし、PC-N-13 は
recordGltfAssetDraw 内部完結 + settings.xml + spec + handoff のみの構造的必然)。

shader 改変 0 件、codegen 改変 0 件、CMake 改変 0 件、tests/ 改変 0 件、
`llvkloader.h` 改変 0 件、`gltfscenemanager.cpp` 改変 0 件。

---

## §5. PC-N-13 達成事項 = real per-draw light params cvar gate 通電 +「zero IS real data」semantic 確立 + multi-asset canary 配線

### §5.1 「zero IS real data」semantic 通電経路 (= `AYAGltfRealLightParamsEnabled=true` 時)

1. **upstream owner 既配線**: `GLTFSceneManager::render` → `recordGltfAssetDraw`
   real Asset path 到達 (= PC-N-9 commit `1b60381d67` 既配線)
2. **per-draw UBO write** (PC-N-8 (f) 既配線、本 sub-step は cvar gate 付加のみ):
   `writeDrawUbo(PerDrawUBO_LightParams, ...)` で 256 B host write zero buffer
   を set=2 binding=0 dynamic offset ring buffer に配置
3. **cvar gate first-fire marker** (本 sub-step 新設、(N13-10) A):
   `AYAGltfRealLightParamsEnabled=true` 時 `s_first_pcn13_real_light_params_fire`
   atomic flag 経由 first-fire `LL_INFOS` marker 1 回起動 = log で「現 phase
   は zero IS real data = sky_smoke shader 非 consume」literal 記録
4. **descriptor set layout 充足** (PC-N-8 (f) 既配線): set=2 binding=0 dynamic
   offset binding を vkCmdBindDescriptorSets で bind = sky_smoke shader 非
   consume でも descriptor set layout 制約充足 architectural truth
5. **shader 非 consume** ((N13-1) ⭐ C 採用 architectural truth): `sGltfStubAssetPipeline`
   流用 sky_smoke shader は `PerDrawUBO_LightParams` を一切 read せず =
   `pointLightF.glsl` / `spotLightF.glsl` / `multiPointLightF.glsl` は
   post-deferred lighting で GLTF asset draw とは無関係、host write 256 B
   zero buffer は descriptor set layout 充足 architectural truth (現 phase
   設計通り、bug ではない)

### §5.2 multi-asset canary 経路 (= `AYAGltfMultiAssetCanary=true` 時)

1. **per-draw asset address tracking** (本 sub-step 新設、(N13-11) A):
   `sPcn13MultiAssetSeen.insert(static_cast<const void*>(asset))` で seen
   asset address 蓄積 = `std::unordered_set<const void*>` で O(1) insert
2. **2 件目以降検出** ((N13-11) A): `sPcn13MultiAssetSeen.size() > 1u` 判定で
   2 件目以降 asset 到達検出
3. **first-fire marker** ((N13-10) A): `s_first_pcn13_multi_asset_canary_fire`
   atomic flag 経由 first-fire `LL_INFOS` marker 1 回起動 = log で
   `seen_asset_count` + `current_asset` address literal 取得 = multi-asset
   GLTF draw 通電 literal 確認
4. **layering 制約完全充足**: `const void*` opaque pointer 追跡のみ、Asset
   content 一切 deref せず = llvkloader 層から `newview/gltf/asset.h` include
   不要 = llvkloader.cpp:568-569 layering 制約完全充足
5. **main thread 専有 = mutex 不要**: `recordGltfAssetDraw` は
   `GLTFSceneManager::render` から single thread 呼出 = `sPcn13MultiAssetSeen`
   は main thread 専有ゆえ atomic / mutex 不要 (= PC-N-14/PC-N-15 worker
   thread design 時に再評価予定)

### §5.3 fall-through path (= 両 cvar OFF default 時 = MUSEUBO-A 整合)

- `AYAGltfRealLightParamsEnabled=false` (default): cvar gate 無発火、writeDrawUbo
  zero buffer write 維持、first-fire marker 起動なし、log 静音
- `AYAGltfMultiAssetCanary=false` (default): canary block 全 skip、
  `sPcn13MultiAssetSeen` 一切 insert せず (= steady state cost 0)、log 静音
- OpenGL 描画 100% 維持 + PC-N-8 (f) 5 段 graceful degrade 内部維持 +
  MUSEUBO-A 整合

### §5.4 (N13-1) ⭐ critical C 採用 =「zero IS real data」semantic 確立 詳細

- **検討経緯**: PC-N-13 task statement literal「zero-buffer
  `PerDrawUBO_LightParams` 卒業」を design-lock phase で 3 案検討
  - 案 A: 実 PBR light data 注入 = `sGltfStubAssetPipeline` 流用 sky_smoke
    shader は `PerDrawUBO_LightParams` 非 consume ゆえ無意味 + scope 拡張で
    `feedback_ubo_migration_one_at_a_time` 違反 risk
  - 案 B: 別 UBO binding に切替 = `LightParams_GLTF` 新設 + shader 接続 =
    scope 過大 + `feedback_ubo_migration_one_at_a_time` 違反
  - 案 C ⭐: zero IS real data 解釈 = sky_smoke shader 非 consume architectural
    truth 尊重 + cvar gate + first-fire marker で「現 phase で zero が real」
    を log 記録、Phase 1.F+ 実 PBR shader 接続時に PC-N-13.1 で data 内容置換
    着手予定
- **AYA 承認**: (N13-1) C 採用「全件推奨で OK」record (2026-06-05)
- **sky_smoke shader 非 consume architectural truth literal 確認**:
  `llvkloader.cpp:5843-5845` 既存 comment「shader 未参照でも GPU error なし」
  literal で確認、`pointLightF.glsl` / `spotLightF.glsl` /
  `multiPointLightF.glsl` は post-deferred lighting で GLTF asset draw とは
  無関係、`sGltfStubAssetPipeline` 流用 sky_smoke shader は
  `PerDrawUBO_LightParams` を一切 read せず
- **Phase 1.F+ 着手予定**: 実 PBR shader 接続時 (= sky_smoke shader 流用卒業時)
  に PC-N-13.1 等で data 内容置換 = PerDrawUBO_LightParams set=2 binding=0
  256 B host write を実 light data に置換、shader 側で consume = real per-draw
  PBR lighting 通電 = `feedback_ubo_migration_one_at_a_time` 厳格遵守で
  Phase 1.E 範囲外

### §5.5 Phase 1.E 残課題 (= PC-N-14/PC-N-15)

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
  PC-N-12 design-lock (commit `c9c99d278f`) + PC-N-12 実装 (commit `4373c302d7`)
  + PC-N-13 design-lock (commit `e13d00d4a6`)
- ✅ **PC-N-13 実装 ✅ 本 commit = Phase 1.E 内 3rd sub-step 実装完了 =
  real per-draw light params cvar gate 通電 +「zero IS real data」semantic
  確立 + multi-asset GLTF draw canary 配線**
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
PC-N-11 ✅ + PC-N-12 design-lock ✅ + PC-N-12 ✅ + PC-N-13 design-lock ✅ +
**PC-N-13 ✅ 本 commit = Phase 1.E 内 3rd sub-step 実装完了 = real per-draw
light params cvar gate 通電 +「zero IS real data」semantic 確立 + multi-asset
canary 配線** +
⏳ PC-N-14..PC-N-15 design-lock + 実装 + Phase 1.E complete + Phase 1 全完了 +
Mac/Win 開発者補完 phase

---

## §8. self-verify 9 観点 全 ✅

1. ✅ Exit Criteria 10 項全充足 (§3)
2. ✅ 必読 1 件 (PC-N-13 design-lock doc) + pinpoint reference 別記 = full file
   dump なし (= `feedback_handoff_minimal_pre_req_read` 整合)
3. ✅ step (a)/(b)/(c)/(d)/(e)/(f)/(g) 7 step 全実装 (§1)
4. ✅ ambiguity (N13-1)..(N13-16) 16 件 AYA literal「全件推奨で OK」record
   (2026-06-05) design-lock 継承 + 本実装で全件採用案通り実装 (特に (N13-1) ⭐
   C 採用 =「zero IS real data」semantic 確立、sky_smoke shader 非 consume
   architectural truth 尊重で Phase 1.F+ 実 PBR shader 接続時に PC-N-13.1 で
   data 内容置換予定)
5. ✅ GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp=6
   不変、PC-N-12 commit `4373c302d7` 同数)
6. ✅ MUSEUBO-A 整合 = `AYAGltfRealLightParamsEnabled=false` +
   `AYAGltfMultiAssetCanary=false` default で writeDrawUbo zero buffer write
   維持 + canary skip + OpenGL 描画 100% 維持 + PC-N-8 (f) 5 段 graceful
   degrade 内部維持
7. ✅ build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11/11 + 10/10 +
   13/13 + codegen 131/131 全 PASS + GATE-B integrity `LL_VULKAN_GLSL`=6 不変
8. ✅ commit 内容 = 2 modified (indra/) + 1 modified (cross-platform spec) +
   1 new doc (本 complete handoff) + CMake 改変 0 + codegen 改変 0 + shader 改変
   0 + `llvkloader.h` 改変 0 + `gltfscenemanager.cpp` 改変 0 + tests/ 改変 0 +
   Co-Authored-By 不在
9. ✅ `feedback_no_scope_shrink` 遵守 = PC-N-13 literal scope 4 件 §0 全件実装、
   (N13-1) ⭐ C 採用「zero IS real data」semantic は sky_smoke shader 非 consume
   architectural truth 尊重 + cvar gate + first-fire marker で「現 phase で zero
   が real」を log 記録ゆえ scope 縮小ではなく architectural truth 尊重、
   Phase 1.F+ 実 PBR shader 接続時に PC-N-13.1 で data 内容置換予定は
   `feedback_ubo_migration_one_at_a_time` 厳格遵守整合、PC-N-14..PC-N-15
   持越しは別 phase 分解 = `feedback_ubo_migration_one_at_a_time` 厳格遵守整合

---

## §9. 次 session 着手 1 line

PC-N-14 design-lock 着手 = worker thread design (= per-Primitive UBO write +
cmdbuf record 並列化 design) + ambiguity 確認 + 実装計画分解 + Exit Criteria
明文化 = `feedback_ubo_migration_one_at_a_time` 厳格遵守で本 PC-N-13 real
per-draw light params cvar gate 通電 +「zero IS real data」semantic 確立 +
multi-asset canary 配線 baseline 上に Phase 1.E 内 4th sub-step として別 session
別途 design-lock。

---

## §A. feedback 遵守 record

- ✅ `feedback_proactive_handoff` (本 PC-N-13 complete handoff doc 起案)
- ✅ `feedback_handoff_minimal_pre_req_read` (必読 1 件 = PC-N-13 design-lock doc
  + pinpoint Read のみ、full file dump なし)
- ✅ `feedback_self_verify_before_handoff` (9 観点 self-verify 全 ✅)
- ✅ `feedback_build_only_verified` (llrender + WARNING 0 + TUT 11/11 + 10/10 +
  13/13 + codegen 131/131 + GATE-B integrity `LL_VULKAN_GLSL`=6 不変で literal
  検証取得)
- ✅ `feedback_no_scope_shrink` (PC-N-13 literal scope 4 件 §0 全件実装、(N13-1)
  ⭐ C 採用「zero IS real data」semantic は sky_smoke shader 非 consume
  architectural truth 尊重 + cvar gate + first-fire marker で「現 phase で zero
  が real」を log 記録ゆえ scope 縮小ではなく architectural truth 尊重、
  Phase 1.F+ 実 PBR shader 接続時に PC-N-13.1 で data 内容置換予定明示で
  `feedback_ubo_migration_one_at_a_time` 厳格遵守整合)
- ✅ `feedback_doubt_self_first` (design-lock phase で ambiguity 16 件発見 +
  推奨案提示 + AYA literal「全件推奨で OK」record 後本実装、特に (N13-1) ⭐
  C 採用「zero IS real data」semantic = sky_smoke shader 非 consume architectural
  truth literal `llvkloader.cpp:5843-5845` 既存 comment で確認後採用、推測実装
  なし)
- ✅ `feedback_admit_unknown` (design-lock phase で zero-buffer
  `PerDrawUBO_LightParams` 卒業 task statement literal を 3 案検討 + 案 C 採用
  根拠 = sky_smoke shader 非 consume architectural truth、Phase 1.F+ 実 PBR
  shader 接続時に再着手予定明示で「分からない」を明示、勝手に scope 拡張せず)
- ✅ `feedback_confirm_referent_before_acting` (16 件 batch AYA 確認 design-lock
  phase で完了、本実装中も PC-N-13 (a) + PC-N-13 (b) tag block 配置位置は AYA
  design-lock 承認後採用、推測実装なし)
- ✅ `feedback_ubo_migration_one_at_a_time` 厳格遵守 (PC-N-13 = real per-draw
  light params cvar gate + multi-asset canary 単独 sub-step、PC-N-14..PC-N-15
  (= worker thread design + 実装) + PC-N-13.1 (= Phase 1.F+ 実 PBR shader
  接続時の data 内容置換) は別 phase の別 session で別途分解)
- ✅ `feedback_design_phase_no_code_write` 整合 (本 PC-N-13 は実装 phase = 改変
  あり、design-lock commit `e13d00d4a6` で `indra/` 改変 0 件完了済)
- ✅ `feedback_release_branch_workflow` (feature branch
  `feature/ayastorm-r41-gl-removal` 上 commit)
- ✅ `feedback_no_auto_commit` (AYA 明示 commit 指示受領後 commit 予定)
- ✅ `feedback_no_claude_coauthor` (Co-Authored-By 行不在)
- ✅ `feedback_no_bare_reference_ids` ((N13-1)..(N13-16) 各 ID に項目名 / 採用案
  内容併記 + (a)..(g) 各 step に作業内容併記)
- ✅ `feedback_tests_dir_never_commit` 整合 (tests/ 改変 0 件、git add 個別
  file 指定予定)
- ✅ memory `project_ayastorm_r41_design_principles` 整合
  ((1) Upstream OpenGL 取り込みやすさ維持 = `recordGltfAssetDraw` signature 不変
  + writeDrawUbo unconditional 呼出維持 + sky_smoke shader 非 consume
  architectural truth 尊重 + shader 改変ゼロ + (2) Core プロセス分散実現 =
  per-Primitive writeDrawUbo + per-Asset canary hook で primitive-level /
  asset-level granularity 維持、PC-N-14/15 worker thread 分散 design 整合)
- ✅ memory `project_r41_phase1b_vulkan_host_gate` 整合 (GATE-B = `#ifdef
  LL_VULKAN_GLSL` 新規追加 0 件、`AYAGltfRealLightParamsEnabled` +
  `AYAGltfMultiAssetCanary` cvar runtime gate のみ、count llvkloader.cpp=6
  不変)
- ✅ memory `project_ayastorm_three_platforms` 整合 (cross-platform spec §6
  PC-N-13 行状態 ✅ 反映 = host-side `writeDrawUbo` zero buffer write + cvar
  gate first-fire marker + `std::unordered_set<const void*>` address tracking
  は OS 非依存 + PerDrawUBO_LightParams UBO bind 経路 PC-N-1/2 既配線済 (=
  256 B set=2 binding=0 ring buffer + dynamic offset、MoltenVK 標準対応範囲) +
  descriptor set 数 5 維持 + `AYAGltfRealLightParamsEnabled` +
  `AYAGltfMultiAssetCanary` cvar XML は OS 非依存 ゆえ macOS 派生 fix 候補
  なし想定 + Windows full Vulkan ゆえ派生 fix 候補なし想定、Linux primary
  完成 → 他者補完 model 整合)
