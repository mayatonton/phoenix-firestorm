# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.D PC-N-10 complete = **Phase 1.D complete marker**

**Status**: ✅ **PC-N-10 complete = Phase 1.D 内 5th = 最終 sub-step 実装完了 = Phase 1.D complete marker = 1 GLTF asset 完全 Vulkan draw 通電 ((N10-11) A 採用)**

**Date**: 2026-06-05
**Branch**: `feature/ayastorm-r41-gl-removal`
**Previous commit**: `dcdb5f4799` (PC-N-10 design-lock complete)

---

## §0. PC-N-10 literal scope 実装結果 (= 全件採用案通り実装完了)

PC-N-10 = **Phase 1.D 内 5th = 最終 sub-step = cleanup + 3 stub cvar deprecate +
Phase 1.D complete marker 起案**。

literal scope 7 件全件実装完了 ((N10-1)..(N10-16) 16 件 AYA literal「全件推奨で OK」
record (2026-06-05) 全件採用案通り):

1. ✅ 3 stub cvar 全 deprecate = `AYAGltfStubDrawEnabled` +
   `AYAGltfStubVertexBufferEnabled` + `AYAGltfStubIndexBufferEnabled` ((N10-1) A)
2. ✅ settings.xml 3 stub cvar 行完全削除 + llvkloader.cpp LLCachedControl 宣言削除
   ((N10-2) A)
3. ✅ `sGltfStubVertexBuffer` / `sGltfStubIndexBuffer` storage + initVulkan VMA
   allocate + shutdownVulkan vmaDestroyBuffer 全撤去 ((N10-4) A)
4. ✅ `recordAvatarPlaceholderDraw` 末尾 entry hook の cvar 名のみ
   `AYAGltfStubDrawEnabled` → `AYAGltfRealDrawEnabled` 切替 ((N10-6) A)
5. ✅ PC-N-6 (e) + PC-N-7 (e) stub 経路 + PC-N-5 base shader generate 3 vertex 経路
   全撤去 + PC-N-9 (b) cvar guard wrap 撤去 (二重 gate 冗長解消) ((N10-7) A +
   (N10-8) A + (N10-9) A)
6. ✅ `sGltfStubSkin` sentinel 維持 (= PC-N-8 (f) line 6170 sentinel 共用、real
   Skin owner 切替は Phase 1.E 持越し) ((N10-3) B)
7. ✅ `sGltfStubAssetPipeline` 維持 (= PC-N-8 (f) 再利用、名称 "Stub" は legacy
   命名残るが機能は real 共用) ((N10-5) B)

---

## §1. 実装結果 = 10 step (a)-(j) 全実装

### §1.1 step (a) entry hook cvar 切替 + tag rename ((N10-6) A + (N10-16) A)

`indra/llrender/llvkloader.cpp` `recordAvatarPlaceholderDraw` 末尾 hook:

- cvar 名: `AYAGltfStubDrawEnabled` → `AYAGltfRealDrawEnabled` 切替
- tag block 名称: `PC-N-5 (e)` → `PC-N-10 (a)` rename
- 機能変化: `AYAGltfRealDrawEnabled=true` 時のみ `recordGltfAssetDraw` fire =
  PC-N-8 (f) real LL::GLTF::Asset 経由経路に直結
- entry hook 配置温存 = `recordGltfAssetDraw` dead code 化回避

### §1.2 step (b) PC-N-6 (e) + PC-N-7 (e) + PC-N-5 base 経路撤去 + (c) PC-N-9 (b) cvar guard wrap 撤去

`indra/llrender/llvkloader.cpp` `recordGltfAssetDraw` 内 cvar 分岐:

- **PC-N-9 (b) cvar guard wrap 撤去** ((N10-8) A): entry hook 自体が
  `AYAGltfRealDrawEnabled` cvar gate に切替わるゆえ二重 gate 冗長
- **PC-N-7 (e) stub IB 経路撤去** ((N10-7) A): `AYAGltfStubIndexBufferEnabled`
  分岐 block 全削除
- **PC-N-6 (e) stub VB 経路撤去** ((N10-7) A): `AYAGltfStubVertexBufferEnabled`
  分岐 block 全削除
- **PC-N-5 base shader generate 3 vertex 経路撤去** ((N10-9) A): base shader
  generate 経路 (line 6438-6519 付近) 削除
- **PC-N-8 (f) real LL::GLTF::Asset 経由経路のみ残存** + 5 段 graceful degrade
  内部維持

PC-N-10 (b) removal marker tag block 配置済。

### §1.3 step (d) stub VB/IB storage + VMA allocate/destroy 撤去 ((N10-4) A)

`indra/llrender/llvkloader.cpp` 3 site cleanup (PC-N-10 (d-1)/(d-2)/(d-3) marker):

- **(d-1) storage 撤去** (line 601-607): PC-N-6 (a) + PC-N-7 (a) `sGltfStubVertexBuffer`
  + `sGltfStubIndexBuffer` storage 撤去 = constexpr `sGltfStubVertexCount`/
  `Stride`/`BufferSize` + `sGltfStubVertexData[9]` + constexpr `sGltfStubIndexCount`/
  `Stride`/`BufferSize` + `sGltfStubIndexData[3]` + `VkBuffer`/`VmaAllocation`/
  mapped fields 全削除
- **(d-2) initVulkan VMA allocate 撤去** (line 3993-4007): PC-N-6 (c) + PC-N-7 (c)
  `vmaCreateBuffer` block 全撤去、`createGltfStubAssetPipeline()` call は (N10-5) B
  per 維持
- **(d-3) shutdownVulkan vmaDestroyBuffer 撤去** (line 4325-4335): PC-N-6 (d)
  `sGltfStubVertexBuffer` destroy + PC-N-7 (d) `sGltfStubIndexBuffer` destroy 撤去、
  `sGltfStubAssetPipeline` destroy は (N10-5) B per 維持
- **inline literal fix** (line 3482): `vbd.stride = sGltfStubVertexStride;` →
  `vbd.stride = 12u;  // vec3 position = 3 × sizeof(F32)` (= constexpr 撤去で
  `createGltfStubAssetPipeline()` vertex input binding 用 stride を inline 化)

`sGltfStubSkin` sentinel storage ((N10-3) B) + `sGltfStubAssetPipeline` ((N10-5) B)
維持。

### §1.4 step (e) + (f) settings.xml 3 stub cvar 削除 + AYAGltfRealDrawEnabled Comment 更新

`indra/newview/app_settings/settings.xml`:

- **(e) 3 stub cvar 行完全削除** ((N10-2) A): `AYAGltfStubDrawEnabled`
  (line 10405-10422) + `AYAGltfStubVertexBufferEnabled` (10424-10443) +
  `AYAGltfStubIndexBufferEnabled` (10445-10466) 全 XML cvar block 削除
- **(f) AYAGltfRealDrawEnabled Comment 更新** ((N10-10) A):
  - 「PC-N-10 で 3 stub cvar deprecate 予定」記述削除
  - 「3 stub cvar 統合済 = AYAGltfRealDrawEnabled 単独 cvar に一本化」記述追加
  - 「Phase 1.D complete marker」記述追加
  - 「OFF (default) = recordGltfAssetDraw 発火経路ゼロ = OpenGL 描画 100% 維持」
  - 「ON = PC-N-8 (f) で配線済 real LL::GLTF::Asset 経由 = Phase 1.D complete 通電」
  - `sGltfStubSkin` sentinel 維持 ((N10-3) B) + `sGltfStubAssetPipeline` 維持
    ((N10-5) B) 明示

### §1.5 step (g) cross-platform spec §6 PC-N-10 行 ✅ 反映 + §A 履歴 1 行追記 ((N10-12) A)

`docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md`:

- §6 PC-N-10 行 = ⏳ → ✅ design-lock + 実装 complete = **Phase 1.D complete
  marker**
- §A 履歴 1 行追記 = 2026-06-05 PC-N-10 状態 ✅ 反映 = Phase 1.D complete marker

### §1.6 step (h)+(j) handoff complete doc 起案 + Phase 1.D complete marker 統合明示 ((N10-11) A)

本 file が PC-N-10 complete handoff doc = **Phase 1.D complete marker 統合明示**
(Phase 1.C complete = PC-N-3 complete doc 内統合と同形 pattern 踏襲)。
§5 で Phase 1.D summary 別 § 記載 (1 GLTF asset 完全 Vulkan draw 通電 達成)。

### §1.7 step (i) build verify literal 取得 ((N10-13) A)

§2 参照。

---

## §2. build verify literal 取得

| # | check | result |
|---|-------|--------|
| 1 | `make -j4 llrender` | ✅ `[100%] Built target llrender` (= ERROR 0 / WARNING 0) |
| 2 | `INTEGRATION_TEST_lluboringbuffer` | ✅ 11/11 PASS YAY!! |
| 3 | `INTEGRATION_TEST_llassetubopool` | ✅ 10/10 PASS YAY!! |
| 4 | `INTEGRATION_TEST_llpipelinecachestorage` | ✅ 13/13 PASS YAY!! |
| 5 | `python3 -m unittest discover tests` (codegen) | ✅ 131/131 OK |
| 6 | `grep -c LL_VULKAN_GLSL indra/llrender/llvkloader.cpp` | ✅ 6 (= PC-N-9 commit `1b60381d67` 同数、GATE-B integrity 維持) |

---

## §3. PC-N-10 実装 phase Exit Criteria (= 10 項全充足)

| # | criterion | status |
|---|-----------|--------|
| i | `recordAvatarPlaceholderDraw` 末尾 entry hook cvar 名 `AYAGltfStubDrawEnabled` → `AYAGltfRealDrawEnabled` 切替 + tag block 名称 PC-N-5 (e) → PC-N-10 (a) rename ((N10-6) A + (N10-16) A) | ✅ |
| ii | `recordGltfAssetDraw` 内 PC-N-6 (e) + PC-N-7 (e) + PC-N-5 base + PC-N-9 (b) cvar guard wrap 全撤去 + PC-N-8 (f) のみ残存 ((N10-7) A + (N10-8) A + (N10-9) A) | ✅ |
| iii | `sGltfStubVertexBuffer` + `sGltfStubIndexBuffer` storage + initVulkan VMA allocate + shutdownVulkan vmaDestroyBuffer 全撤去 + `sGltfStubSkin` sentinel + `sGltfStubAssetPipeline` 維持 ((N10-3) B + (N10-4) A + (N10-5) B) | ✅ |
| iv | settings.xml 3 stub cvar 行完全削除 + `AYAGltfRealDrawEnabled` Comment 更新 (3 stub cvar 統合済 + Phase 1.D complete marker 記載) ((N10-2) A + (N10-10) A) | ✅ |
| v | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp=6 不変、PC-N-9 commit `1b60381d67` 同数) | ✅ |
| vi | MUSEUBO-A 整合 = `AYAGltfRealDrawEnabled=false` default で `recordGltfAssetDraw` 発火経路ゼロ + OpenGL 描画 100% 維持 + 5 段 graceful degrade 内部維持 | ✅ |
| vii | build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 ((N10-13) A) | ✅ |
| viii | cross-platform spec §6 PC-N-10 行 ✅ 反映 + §A 履歴 1 行追記 ((N10-12) A) | ✅ |
| ix | handoff complete doc 起案 + Phase 1.D complete marker 統合明示 ((N10-11) A) | ✅ |
| x | self-verify 9 観点 全 ✅ | ✅ |

---

## §4. 改変 file 4 件

| # | file | 改変概要 | 実 LoC |
|---|------|---------|--------|
| 1 | `indra/llrender/llvkloader.cpp` | step (a) entry hook cvar 名切替 + tag rename / step (b) PC-N-7/6 (e) + PC-N-5 base 撤去 / step (c) PC-N-9 (b) cvar guard wrap 撤去 / step (d) stub VB/IB storage + VMA allocate + vmaDestroyBuffer 撤去 + inline `vbd.stride = 12u;` literal fix | +41/-540 (net -499) |
| 2 | `indra/newview/app_settings/settings.xml` | step (e) 3 stub cvar 行完全削除 / step (f) `AYAGltfRealDrawEnabled` Comment 更新 | +9/-76 (net -67) |
| 3 | `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md` | step (g) §6 PC-N-10 行状態 ✅ 反映 = Phase 1.D complete marker + §A 履歴 1 行追記 | +2/-1 |
| 4 | (本 file) `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-...-phase1-d-pc-n-10-complete.md` | step (h)+(j) 新規 PC-N-10 complete handoff doc 起案 + Phase 1.D complete marker 統合明示 | new |

`llvkloader.h` 改変 0 件 (= PC-N-8 (e) accessor 既宣言済、新規 API 追加 0 件 ゆえ
header 改変不要)。shader 改変 0 件、codegen 改変 0 件、CMake 改変 0 件、tests/
改変 0 件。

---

## §5. **Phase 1.D complete = 1 GLTF asset 完全 Vulkan draw 通電 marker** ((N10-11) A 採用)

### §5.1 Phase 1.D summary (PC-N-5..PC-N-10 5 sub-step 完了)

Phase 1.D = **実 GLTF Vulkan draw 通電** 章 5 sub-step:

- ✅ **PC-N-5** (= 着手起点) `recordGltfAssetDraw` 新設 + `AYAGltfStubDrawEnabled`
  cvar gate + Skin_GLTFJoints UBO bind 経由 rigged stub draw 通電
  (commit `675529a891`)
- ✅ **PC-N-6** (= 1st sub-step) stub vertex buffer Vulkan 経路通電 = file-static
  `sGltfStubVertexBuffer` (= position vec3 × 3 hardcoded triangle) VMA buffer
  + `vkCmdBindVertexBuffers` + `sGltfStubAssetPipeline` 新設 +
  `AYAGltfStubVertexBufferEnabled` 段階 cvar (commit `7e90245d10`)
- ✅ **PC-N-7** (= 2nd sub-step) stub index buffer Vulkan 経路通電 = file-static
  `sGltfStubIndexBuffer` (= `{0,1,2}` CCW UINT32) + `vkCmdBindIndexBuffer` +
  `vkCmdDrawIndexed` + `AYAGltfStubIndexBufferEnabled` 段階 cvar
  (commit `01473751e9`)
- ✅ **PC-N-8** (= 3rd sub-step) **実 LL::GLTF::Asset 経由 vertex/index buffer
  Vulkan infrastructure 新設** = per-Primitive ownership lifecycle +
  `sPrimitiveVertexBuffers`/`sPrimitiveIndexBuffers` `unordered_map` + 6 新 API
  (`registerPrimitiveVertexBuffer`/`writePrimitiveVertexBuffer`/
  `unregisterPrimitiveVertexBuffer` + 同形 index) + `Primitive::uploadVulkanBuffers()`
  + `Asset::uploadTransforms` 末尾 hook + `Primitive` dtor 対称配線 +
  `sCurrentPrimitive` static + accessor + `recordGltfAssetDraw` 内 real Asset
  path 配線 (commit `51b550585e`)
- ✅ **PC-N-9** (= 4th sub-step) **`GLTFSceneManager::render` 統合 +
  `AYAGltfRealDrawEnabled` cvar gate** = per-Primitive loop body 冒頭
  `setCurrentPrimitive` + 末尾 `clearCurrentPrimitive` 配線 + PC-N-8 (f) block
  全体を cvar guard で wrap + `AYAGltfRealDrawEnabled` Boolean cvar 1 件追加
  (commit `1b60381d67`)
- ✅ **PC-N-10** (= 5th = 最終 sub-step) **cleanup + 3 stub cvar deprecate +
  Phase 1.D complete marker 起案** = 本 commit

### §5.2 Phase 1.D 達成事項

**1 GLTF asset 完全 Vulkan draw 通電** 達成:

- `AYAGltfRealDrawEnabled=true` 時、entry hook (`recordAvatarPlaceholderDraw`
  末尾) → `recordGltfAssetDraw` → PC-N-8 (f) real LL::GLTF::Asset 経由経路 fire
- per-Primitive vertex/index buffer ownership で `Asset::uploadTransforms` 経由
  lazy register + memcpy + `vkCmdBindVertexBuffers` + `vkCmdBindIndexBuffer`
  (UINT32) + `vkCmdDrawIndexed(real_index_count, 1, 0, 0, 0)` 実行
- `sGltfStubAssetPipeline` 再利用 (Vulkan 仕様 dynamic state ゆえ real Asset draw
  にも適用可)
- `sGltfStubSkin` sentinel address-only pattern で rigged binding 通電
  (real Skin owner 切替は Phase 1.E 持越し)
- 5 段 graceful degrade 維持 (sAllocator + sCurrentAsset + sCurrentPrimitive +
  sGltfStubAssetPipeline + sPrimitiveVertexBuffers find = 各 nullptr guard)

### §5.3 Phase 1.D 残課題 (= Phase 1.E scope)

- multi-asset (= 複数 GLTF asset 同時描画)
- multi-skin (= real Skin owner 切替 = `sGltfStubSkin` sentinel 卒業)
- worker thread (= per-Primitive granularity の並列化、設計原則 (2) 実現)

---

## §6. 残 strict 線形

- ✅ Phase 1.A / 1.B / (Z) SSS / (W) uniform4iv / (Y) Phase 1.C prep
- ✅ PC-0..PC-7ε / PC-N decomposition / PC-N-1..PC-N-4 (= Phase 1.C complete)
- ✅ PC-8 Linux primary marker (= Phase 1.C strict 線形終了)
- ✅ PC-N-5 (= Phase 1.D 着手起点) / Phase 1.D decomposition design-lock
- ✅ PC-N-6 / PC-N-7 / PC-N-8 / PC-N-9 (= Phase 1.D 1st-4th sub-step)
- ✅ PC-N-10 design-lock (commit `dcdb5f4799`) + **PC-N-10 実装 ✅ 本 commit** =
  **Phase 1.D 内 5th = 最終 sub-step 実装完了 = Phase 1.D complete marker**
- ⏳ Phase 1.E (= multi-asset / multi-skin / worker thread) = 別 phase の別
  session で別途分解 (= `feedback_ubo_migration_one_at_a_time` 厳格遵守)
- ⏳ Phase 1 全完了 → Mac/Win 開発者補完 phase

---

## §7. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ +
(Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α..PC-7ε ✅ +
PC-N decomposition design-lock ✅ + PC-N-1..PC-N-4 ✅ = Phase 1.C complete ✅ +
PC-8 Linux primary marker ✅ = Phase 1.C strict 線形終了 ✅ +
PC-N-5 ✅ = Phase 1.D 着手起点 ✅ + Phase 1.D decomposition design-lock ✅ +
PC-N-6 ✅ + PC-N-7 ✅ + PC-N-8 ✅ + PC-N-9 ✅ +
PC-N-10 design-lock ✅ + **PC-N-10 ✅ 本 commit = Phase 1.D 内 5th = 最終
sub-step 実装完了 = Phase 1.D complete marker = 1 GLTF asset 完全 Vulkan
draw 通電 達成** +
⏳ Phase 1.E (= multi-asset / multi-skin / worker thread)
⏳ Phase 1 全完了 ⏳ Mac/Win 開発者補完 phase

---

## §8. self-verify 9 観点 全 ✅

1. ✅ Exit Criteria 10 項全充足 (§3)
2. ✅ 必読 1 件 (PC-N-10 design-lock doc) + pinpoint reference 別記、full file
   dump なし (= `feedback_handoff_minimal_pre_req_read` 整合)
3. ✅ step (a)/(b)/(c)/(d)/(e)/(f)/(g)/(h)/(i)/(j) 10 step 全実装 (§1)
4. ✅ ambiguity (N10-1)..(N10-16) 16 件 AYA literal「全件推奨で OK」record
   (2026-06-05) design-lock 継承 + 本実装で全件採用案通り実装
5. ✅ GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp=6
   不変、PC-N-9 commit `1b60381d67` 同数)
6. ✅ MUSEUBO-A 整合 = `AYAGltfRealDrawEnabled=false` default で
   `recordGltfAssetDraw` 発火経路ゼロ + OpenGL 描画 100% 維持 + 5 段 graceful
   degrade 内部維持
7. ✅ build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11/11 + 10/10 +
   13/13 + codegen 131/131 全 PASS
8. ✅ commit 内容 = 2 modified (indra/) + 1 modified (cross-platform spec) +
   1 new doc (本 complete handoff = Phase 1.D complete marker 統合明示) + CMake
   改変 0 + codegen 改変 0 + shader 改変 0 + .h 改変 0 + tests/ 改変 0 +
   Co-Authored-By 不在
9. ✅ `feedback_no_scope_shrink` 遵守 = PC-N-10 literal scope 7 件 §0 全件実装、
   (N10-3) B `sGltfStubSkin` 維持 + (N10-5) B `sGltfStubAssetPipeline` 維持 は
   design-lock phase で AYA 確認済の意図的維持 = 縮小ではない、Phase 1.E
   (multi-skin / worker thread) 持越しは別 phase 分解 = `feedback_ubo_migration_one_at_a_time`
   厳格遵守整合

---

## §9. 次 session 着手 1 line

Phase 1.E design-lock 着手 = multi-asset / multi-skin (= `sGltfStubSkin` sentinel
卒業) / worker thread 分散 (= 設計原則 (2) Core プロセス分散実現) 着手 decomposition
= `feedback_ubo_migration_one_at_a_time` 厳格遵守で本 PC-N-10 = Phase 1.D
complete = 1 GLTF asset 完全 Vulkan draw 通電 baseline 上に Phase 1.E 章として
別 phase decomposition。

---

## §A. feedback 遵守 record

- ✅ `feedback_proactive_handoff` (本 PC-N-10 complete handoff doc 起案 = Phase
  1.D complete marker 統合明示)
- ✅ `feedback_handoff_minimal_pre_req_read` (必読 1 件 + pinpoint Read のみ、
  full file dump なし = PC-N-10 design-lock doc + pinpoint reference 11 件)
- ✅ `feedback_self_verify_before_handoff` (9 観点 self-verify 全 ✅)
- ✅ `feedback_build_only_verified` (llrender + WARNING 0 + TUT 11/11 + 10/10 +
  13/13 + codegen 131/131 + GATE-B integrity LL_VULKAN_GLSL=6 不変で literal
  検証取得)
- ✅ `feedback_no_scope_shrink` (PC-N-10 literal scope 7 件 §0 全件実装、(N10-3)
  B + (N10-5) B 維持は design-lock phase AYA 確認済意図的維持 = 縮小ではない、
  Phase 1.E 持越しは別 phase 分解 = `feedback_ubo_migration_one_at_a_time`
  厳格遵守整合)
- ✅ `feedback_doubt_self_first` (design-lock phase で ambiguity 16 件発見 +
  推奨案提示 + AYA literal「全件推奨で OK」record 後本実装、本実装中も PC-N-6 (a)
  + PC-N-7 (a) storage 撤去範囲 + PC-N-6 (c) + PC-N-7 (c) VMA allocate 撤去範囲
  + PC-N-6 (d) + PC-N-7 (d) destroy 撤去範囲を Read で literal 確認後 surgical
  preservation = (N10-5) B `createGltfStubAssetPipeline()` call + vmaDestroyBuffer
  pipeline destroy のみ温存、`sGltfStubVertexStride` inline literal fix も Read
  で参照箇所 literal 確認後実施、推測実装なし)
- ✅ `feedback_confirm_referent_before_acting` (16 件 batch AYA 確認 design-lock
  phase で完了、本実装中も `sGltfStubAssetPipeline` 維持 vs 撤去 + sentinel storage
  維持 vs 撤去は (N10-3) B + (N10-5) B literal 確認後採用、orphaned function/
  namespace structure 復旧時も 2 件の `// </AYAstorm r41 PC-N-5 (b)>` marker を
  anchor として literal 確認後 collapse)
- ✅ `feedback_ubo_migration_one_at_a_time` 厳格遵守 (PC-N-10 = cleanup + 3 stub
  cvar deprecate + Phase 1.D complete marker 起案単独 sub-step、Phase 1.E
  (multi-asset / multi-skin / worker thread) は別 phase の別 session で別途分解)
- ✅ `feedback_design_phase_no_code_write` 整合 (本 PC-N-10 は実装 phase = 改変
  あり、design-lock commit `dcdb5f4799` で `indra/` 改変 0 件完了済)
- ✅ `feedback_release_branch_workflow` (feature branch
  `feature/ayastorm-r41-gl-removal` 上 commit)
- ✅ `feedback_no_auto_commit` (AYA 明示 commit 指示受領後 commit 予定)
- ✅ `feedback_no_claude_coauthor` (Co-Authored-By 行不在)
- ✅ `feedback_no_bare_reference_ids` ((N10-1)..(N10-16) 各 ID に項目名 / 採用案
  内容併記 + (a)..(j) 各 step に作業内容併記)
- ✅ `feedback_tests_dir_never_commit` 整合 (tests/ 改変 0 件、git add 個別
  file 指定予定)
- ✅ memory `project_ayastorm_r41_design_principles` 整合
  ((1) Upstream OpenGL 取り込みやすさ維持 = entry hook 配置温存 (= cvar 名のみ
  切替、(N10-6) A) + `sGltfStubSkin` sentinel 維持 ((N10-3) B) +
  `sGltfStubAssetPipeline` 維持 ((N10-5) B) + `recordGltfAssetDraw` signature
  不変 + `GLTFSceneManager::render` 改変 0 件 + shader 改変ゼロ +
  (2) Core プロセス分散実現 = per-Primitive `setCurrentPrimitive`/
  `clearCurrentPrimitive` hook + per-Primitive vertex/index buffer ownership
  維持 = Phase 1.E (multi-asset / worker thread) 着手起点確保)
- ✅ memory `project_r41_phase1b_vulkan_host_gate` 整合 (GATE-B = `#ifdef
  LL_VULKAN_GLSL` 新規追加 0 件、cvar 新設 0 件 + 既存 4 cvar から 3 件削除 =
  削減 phase、count=6 不変)
- ✅ memory `project_ayastorm_three_platforms` 整合 (cross-platform spec §6
  PC-N-10 行状態 ✅ 反映 = host-side cleanup ゆえ OS 非依存 + descriptor set 数
  5 維持 + `sGltfStubSkin` sentinel + `sGltfStubAssetPipeline` 維持で MoltenVK
  影響増なし + 3 stub cvar 削除 + stub VB/IB storage 撤去は host-side container/
  XML cleanup ゆえ MoltenVK 標準対応範囲 + Windows full Vulkan ゆえ派生 fix
  候補なし、Linux primary 完成 → 他者補完 model 整合 + **Phase 1.D complete =
  1 GLTF asset 完全 Vulkan draw 通電 = Linux primary 達成**)
