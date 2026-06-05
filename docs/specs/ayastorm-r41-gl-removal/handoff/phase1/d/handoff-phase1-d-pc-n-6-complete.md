# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.D **PC-N-6 complete**

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-N-6 (= Phase 1.D 内 1st sub-step = stub vertex buffer Vulkan 経路通電) **実装 phase 完了 marker**。design-lock (commit `146c3002e5`) で確定した step (a)-(g) 7 step 実装 + Exit Criteria 10 項全充足 + build verify literal 取得 (llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131)。

> **本 doc 位置付け**: PC-N-6 design-lock (= `146c3002e5`) 後の **PC-N-6 実装 phase 完了 marker**。`sGltfStubVertexBuffer` file-static VMA buffer 新設 + `sGltfStubAssetPipeline` 新設 (= sAvatarBonePipeline 同形 + vertex input state 拡張) + initVulkan VMA allocate + initial upload + shutdownVulkan 対称破棄 + `recordGltfAssetDraw` 内 `AYAGltfStubVertexBufferEnabled` cvar 分岐 + bindVertexBufferVk + vkCmdDraw(N) 通電。次は PC-N-7 design-lock phase (= 実 LL::GLTF::Asset 経由 index buffer upload + vkCmdDrawIndexed)。

---

## §0. 本 session 着手契機 + PC-N-6 完了 scope record

**契機**: AYA 指示「r41 Phase 1.D PC-N-6 実装着手お願いします」literal 受領 (2026-06-05、PC-N-6 design-lock commit `146c3002e5` 後の継続 session = 別 session の fresh context) + 必読 1 件 = `handoff-substep-...-phase1-d-pc-n-6-design-lock.md` (= 13 件 ambiguity 全 AYA literal「すべて推奨でお願いします」record 済) Read + pinpoint reference 13 件 Read → step (a)-(g) 7 step 実装 → build verify literal 取得 → 本 complete doc 起案。

**PC-N-6 完了 scope** (= design-lock §0 literal 7 件 + step (a)-(g) 7 step 全実装):

1. **`sGltfStubVertexBuffer` file-static + VMA storage 新設** = anonymous namespace 内 file-static `VkBuffer` + `VmaAllocation` + `void* mapped` + `sGltfStubVertexData[9]` const CCW triangle (= NDC vec3 × 3 = 36 B) + static_assert で size 一致確認 → llvkloader.cpp:603-634 (PC-N-6 (a) tag block)
2. **`sGltfStubAssetPipeline` storage + `createGltfStubAssetPipeline` 関数新設** = `sAvatarBonePipeline` 同形 + `VkVertexInputBindingDescription` (binding=0, stride=12, INPUT_RATE_VERTEX) + `VkVertexInputAttributeDescription` (location=0, R32G32B32_SFLOAT, offset=0) で vertex input state 拡張、sAYAStandardLayout + sky smoke shader 流用 ((N6-7) B 別 pipeline 新設) → llvkloader.cpp:3430-3552 (PC-N-6 (b) tag block)
3. **`initVulkan` 内 VMA allocate + initial upload + pipeline create** = `vmaCreateBuffer` (VMA_MEMORY_USAGE_AUTO + HOST_ACCESS_SEQUENTIAL_WRITE_BIT + MAPPED_BIT) + `memcpy(mapped, sGltfStubVertexData, 36B)` + `createGltfStubAssetPipeline()` call + 4 段 graceful degrade (sAllocator nullptr / vmaCreateBuffer fail / mapped nullptr / createPipeline fail で LL_WARNS_ONCE) → llvkloader.cpp:3982-4047 (PC-N-6 (c) tag block)
4. **`shutdownVulkan` 内 対称破棄** = `vkDestroyPipeline(sGltfStubAssetPipeline)` + `vmaDestroyBuffer(sGltfStubVertexBuffer, sGltfStubVertexAllocation)` + storage nullify (= PC-N-5 sGltfStubSkin unregister 同形対称 lifecycle) → llvkloader.cpp:4366-4382 (PC-N-6 (d) tag block)
5. **`recordGltfAssetDraw` 内 cvar 分岐 + 別 pipeline bind + 実 vertex buffer bind** = `static LLCachedControl<bool> sAyastormGltfStubVertexBufferEnabled(gSavedSettings, "AYAGltfStubVertexBufferEnabled", false)` + true 時に `vkCmdBindPipeline(sGltfStubAssetPipeline)` + UBO sequence (writeDrawUbo + writeSkinUbo + flushSkinUbos + bindV3aRigged + push constant identity) + `bindVertexBufferVk(sGltfStubVertexBuffer, 0)` + `vkCmdDraw(sGltfStubVertexCount, 1, 0, 0)` + first-fire LL_INFOS marker + early return、PC-N-5 経路 (= sAvatarBonePipeline + vkCmdDraw(3,1,0,0) shader generate) は cvar false 時に従来通り維持 → llvkloader.cpp:5751-5849 (PC-N-6 (e) tag block)
6. **`AYAGltfStubVertexBufferEnabled` cvar 新設** = Boolean default `false` Persist=1 (= `AYAGltfStubDrawEnabled` 同形 r41 cvar pattern 踏襲) → settings.xml:10432 周辺 (cvar 1 件追加)
7. **build verify literal 取得** = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 全 PASS

---

## §1. 必読 + pinpoint reference (= 次 session = PC-N-7 design-lock 向け)

**次 session 必読**:

1. **本 PC-N-6 complete doc 全文**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-d-pc-n-6-complete.md`

**pinpoint reference** (= PC-N-7 design-lock phase で必要分のみ Read):

- **PC-N-6 design-lock doc**: `handoff-...-phase1-d-pc-n-6-design-lock.md` (= ambiguity 13 件 AYA literal「すべて推奨でお願いします」record + 実装計画 7 step + Exit Criteria 10 項)
- **Phase 1.D decomposition design-lock**: `handoff-...-phase1-d-decomposition-design-lock.md` (= PC-N-6..PC-N-10 5 sub-step 分解 + PC-N-7 = 実 LL::GLTF::Asset 経由 index buffer upload + vkCmdDrawIndexed)
- **本 PC-N-6 complete §2**: 実装内容 5 site (PC-N-6 (a)/(b)/(c)/(d)/(e) tag block) + line ref
- **本 PC-N-6 complete §3**: build verify literal (llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131)
- **`sGltfStubVertexBuffer` + `sGltfStubAssetPipeline` + `recordGltfAssetDraw`**: llvkloader.cpp:603-634 / 3430-3552 / 5751-5849
- **PC-N-5 complete §2.1**: `recordGltfAssetDraw` 関数 PC-N-5 (b) 経路 (= cvar false 時従来 path) との並走関係
- **LL::GLTF::Asset PC-7γ-3 uploadTransforms dual-write infrastructure**: PC-N-7 で実 vertex buffer 経路統合時 reference
- **LL::GLTF::Primitive 構造体**: per-attribute raw vertex data layout (= PC-N-7 で実 vertex buffer 構築時 reference)
- **cross-platform spec §6 PC-N-6 行**: macOS / Windows 派生 fix 候補欄 (= Phase 1 全完了時の Mac/Win 開発者補完 phase 用)
- **GATE-B literal**: memory `project_r41_phase1b_vulkan_host_gate` (= PC-N-7 以降も `#ifdef LL_VULKAN_GLSL` 新規追加 0 件維持)

---

## §2. 実装内容

### §2.1 編集 1 = `indra/llrender/llvkloader.cpp` (+343 行 / -0)

**5 編集 site** (= step (a)/(b)/(c)/(d)/(e)):

| # | site | line | tag block | 内容 |
|---|------|------|-----------|------|
| (a) | sGltfStubVertexBuffer file-static + VMA storage | 603-634 | `<AYAstorm r41 PC-N-6 (a)>` | `VkBuffer sGltfStubVertexBuffer` + `VmaAllocation sGltfStubVertexAllocation` + `void* sGltfStubVertexMapped` + `constexpr U32 sGltfStubVertexCount=3u` + `sGltfStubVertexStride=12u` + `sGltfStubVertexBufferSize=36u` + `const F32 sGltfStubVertexData[9]` (= CCW triangle NDC, vec3 × 3) + `static_assert(sizeof(data) == BufferSize)` (= PC-N-5 sGltfStubSkin sentinel 直後配置、anonymous namespace 内) |
| (b) | sGltfStubAssetPipeline storage + createGltfStubAssetPipeline 関数 | 3430-3552 | `<AYAstorm r41 PC-N-6 (b)>` | `VkPipeline sGltfStubAssetPipeline = VK_NULL_HANDLE` storage + `bool createGltfStubAssetPipeline()` 関数 (= `createAvatarBonePipeline` body clone + vertex input state 拡張 = `VkVertexInputBindingDescription{binding=0, stride=sGltfStubVertexStride, INPUT_RATE_VERTEX}` + `VkVertexInputAttributeDescription{location=0, binding=0, format=R32G32B32_SFLOAT, offset=0}`、sAYAStandardLayout + sky smoke shader modules 流用、shader 改変ゼロ) |
| (c) | initVulkan VMA allocate + initial upload + pipeline create | 3982-4047 | `<AYAstorm r41 PC-N-6 (c)>` | `vmaCreateBuffer` (VMA_MEMORY_USAGE_AUTO + VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT + VMA_ALLOCATION_CREATE_MAPPED_BIT) + `alloc_info.pMappedData` 取得 + `memcpy(mapped, sGltfStubVertexData, sGltfStubVertexBufferSize)` + `createGltfStubAssetPipeline()` call + 4 段 graceful degrade (sAllocator nullptr / vmaCreateBuffer fail / mapped nullptr / createPipeline fail で LL_WARNS_ONCE)、PC-N-5 (c) tag block 直後配置 |
| (d) | shutdownVulkan 対称破棄 | 4366-4382 | `<AYAstorm r41 PC-N-6 (d)>` | `vkDestroyPipeline(sDevice, sGltfStubAssetPipeline, nullptr)` + storage nullify + `vmaDestroyBuffer(sAllocator, sGltfStubVertexBuffer, sGltfStubVertexAllocation)` + 3 storage (buffer/allocation/mapped) nullify (= PC-N-5 (d) sGltfStubSkin unregister 同形対称 lifecycle、bulk teardown loop 前) |
| (e) | recordGltfAssetDraw cvar 分岐 + 別 pipeline + bindVertexBufferVk | 5751-5849 | `<AYAstorm r41 PC-N-6 (e)>` | `static LLCachedControl<bool> sAyastormGltfStubVertexBufferEnabled(gSavedSettings, "AYAGltfStubVertexBufferEnabled", false)` + true 時に nullptr guard (sGltfStubVertexBuffer / sGltfStubAssetPipeline) + `vkCmdBindPipeline(sGltfStubAssetPipeline)` + UBO sequence (writeDrawUbo + writeSkinUbo + flushSkinUbos + bindV3aRigged + push constant identity) + `bindVertexBufferVk(sGltfStubVertexBuffer, 0)` + `vkCmdDraw(sGltfStubVertexCount, 1, 0, 0)` + first-fire LL_INFOS marker + early return、PC-N-5 経路 (= sAvatarBonePipeline + shader generate vkCmdDraw(3,1,0,0)) は cvar false 時に従来通り維持 |

### §2.2 `indra/llrender/llvkloader.h` 改変 0 件

- `sGltfStubVertexBuffer` / `sGltfStubAssetPipeline` / `createGltfStubAssetPipeline` は anonymous namespace 内 file-static + helper、`LLVKLoader::` 公開 API 不変
- design-lock §6.2 entry 3「.h 改変 0 件想定」literal 通り

### §2.3 codegen 改変 0 件

- `Skin_GLTFJoints` block + `PerDrawUBO_LightParams` block は PC-7γ-3 + PC-N-3 + PC-N-5 で配置済を再利用 (= recordGltfAssetDraw 内 PC-N-5 (b) writeSkinUbo / writeDrawUbo sequence をそのまま PC-N-6 (e) cvar branch でも流用)
- `scripts/ubo_codegen` 改変 0 件、`build-linux-x86_64/codegen/ubo/*.inl` 改変 0 件
- 結果 = codegen unittest 131/131 維持 (= (N6-11) A 整合)

### §2.4 shader 改変 0 件

- sky smoke shader 流用 (= PC-N-5 createAvatarBonePipeline と同じ shader module を sGltfStubAssetPipeline でも使用)
- vertex input state 拡張は host-side pipeline 配線のみ、shader 側 `gl_VertexIndex` 経路は不変 = vertex attribute 実 consumption なし (= (N6-3) A 採用根拠整合)
- `*.glsl` + `aya_r41_blueprints/` 改変 0 件

### §2.5 GATE-B 整合 (= memory `project_r41_phase1b_vulkan_host_gate` 遵守)

- `#ifdef LL_VULKAN_GLSL` **新規追加 0 件**
- PC-N-6 (e) hook の cvar gate は `LLCachedControl<bool>` runtime cvar 経由ゆえ `#ifdef` 非依存 = GATE-B 違反なし
- step (a)/(b)/(c)/(d) 全 site も `#ifdef LL_VULKAN_GLSL` 新規追加 0 件
- (= settings.xml の Comment 文字列内 `LL_VULKAN_GLSL` literal 言及はあるが code 改変ではない、record purpose のみ)

### §2.6 MUSEUBO-A 整合 (= memory `feedback_ubo_migration_one_at_a_time` 整合)

- `AYAGltfStubVertexBufferEnabled=false` default = PC-N-6 (e) cvar 分岐 発火なし = PC-N-5 完了状態と機能等価
- `AYAGltfStubVertexBufferEnabled=true` 時のみ別 pipeline bind + 実 vertex buffer bind 経路発火 = debug live A/B 経路
- `mUseUBO=false` default で既存 OpenGL 描画 100% 維持 = recordGltfAssetDraw は Vulkan placeholder offscreen FBO 経路、`mUseUBO` 不問
- 4 段 graceful degrade (sAllocator / sGltfStubVertexBuffer / sGltfStubAssetPipeline / sGltfStubVertexMapped 各 nullptr guard) で安全運転
- PC-N-5 経路 (cvar false 時の sAvatarBonePipeline + shader generate path) は破壊しない、live A/B 経路独立 ((N6-7) B + (N6-8) A 採用根拠整合)

### §2.7 編集 2 = `indra/newview/app_settings/settings.xml` (+21 行 / -0)

`AYAGltfStubVertexBufferEnabled` Boolean cvar 1 件追加 (= settings.xml:10432 周辺):

```xml
<!-- <FS:AYAstorm r41 Phase 1.D PC-N-6> stub vertex buffer Vulkan 経路通電の live A/B 切替 cvar。
     default false で PC-N-5 経路 (shader generate vkCmdDraw) 維持、true で別 pipeline + bindVertexBufferVk 経路発火。 -->
<key>AYAGltfStubVertexBufferEnabled</key>
<map>
  <key>Comment</key>
  <string>(r41 Phase 1.D PC-N-6) Enable GLTF stub vertex buffer Vulkan path (sGltfStubAssetPipeline + bindVertexBufferVk + vkCmdDraw(N)) parallel to PC-N-5 stub draw path for live A/B testing. Default OFF.</string>
  <key>Persist</key><integer>1</integer>
  <key>Type</key><string>Boolean</string>
  <key>Value</key><integer>0</integer>
</map>
```

- `AYAGltfStubDrawEnabled` (PC-N-5) 同形 r41 cvar pattern 踏襲
- Persist=1 で起動間設定保持 ((N6-9) C 採用根拠)

### §2.8 編集 3 = `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md` (+2 / -1)

- §6 PC-N-6 行 状態を ✅ design-lock + 実装 complete に更新 (= 本 commit literal 反映)
- §A 更新履歴 1 行追記 (2026-06-05 PC-N-6 完了 marker、llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131)

---

## §3. build verify literal (= (N6-11) A scope)

1. **llrender build**: `make -j4 llrender` → **PASS** / ERROR 0 / **WARNING 0**
2. **INTEGRATION_TEST_lluboringbuffer**: **11/11 PASS YAY**
3. **INTEGRATION_TEST_llassetubopool**: **10/10 PASS YAY**
4. **INTEGRATION_TEST_llpipelinecachestorage**: **13/13 PASS YAY**
5. **codegen unittest**: `python3 -m unittest discover tests` from `scripts/ubo_codegen/` → **131/131 OK** (= PC-N-1..PC-N-5 同形維持、(N6-11) A 整合)

= PC-3 / PC-4 / PC-5 algorithm 層 regression なし + Phase 1.A / 1.B / 1.C / 1.D 着手起点 PC-1..PC-N-5 regression なし。

---

## §4. PC-N-6 Exit Criteria 10 項 全充足

| # | Criteria | 充足 |
|---|----------|------|
| (i) | `sGltfStubVertexBuffer` file-static + VMA storage + `sGltfStubVertexData[9]` CCW triangle 新設 ((N6-3) A + (N6-4) B) | ✅ llvkloader.cpp:603-634 |
| (ii) | `sGltfStubAssetPipeline` storage + `createGltfStubAssetPipeline` 関数新設 (= 別 pipeline + vertex input state 拡張) ((N6-7) B) | ✅ llvkloader.cpp:3430-3552 |
| (iii) | `initVulkan` 内 VMA allocate (host-visible mapped) + initial upload + pipeline create + 4 段 graceful degrade ((N6-5) B + (N6-6) A) | ✅ llvkloader.cpp:3982-4047 |
| (iv) | `shutdownVulkan` 内 対称破棄 (= vkDestroyPipeline + vmaDestroyBuffer + storage nullify) | ✅ llvkloader.cpp:4366-4382 |
| (v) | `recordGltfAssetDraw` 内 cvar 分岐 + 別 pipeline bind + UBO sequence + bindVertexBufferVk + vkCmdDraw(N) + first-fire LL_INFOS marker ((N6-1) B + (N6-2) A + (N6-8) A + (N6-10) B) | ✅ llvkloader.cpp:5751-5849 |
| (vi) | `AYAGltfStubVertexBufferEnabled` cvar 新設 (Boolean default `false` Persist=1) ((N6-9) C) | ✅ settings.xml:10432 周辺 |
| (vii) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 | ✅ §2.5 |
| (viii) | MUSEUBO-A 整合 = `mUseUBO=false` default + `AYAGltfStubVertexBufferEnabled=false` default で PC-N-6 経路発火なし、PC-N-5 完了状態と機能等価 + 4 段 graceful degrade | ✅ §2.6 |
| (ix) | build verify llrender + WARNING 0 + TUT 11+10+13 + codegen 131/131 全 PASS ((N6-11) A) | ✅ §3 |
| (x) | tag block 統一 PC-N-6 (a)/(b)/(c)/(d)/(e) + first-fire LL_INFOS marker + handoff complete doc 起案 | ✅ 本 commit |

---

## §5. 残 strict 線形

**Phase 1.C complete** ✅ (= PC-N-3 commit `71f7bb2a89`)
**PC-8 Linux primary marker** ✅ (= commit `8ba7296caf`、Phase 1.C strict 線形終了)
**PC-N-5 design-lock** ✅ (= commit `d1fa626fad`、Phase 1.D 着手起点 design-lock)
**PC-N-5 実装** ✅ (= commit `b7a67ce659`、Phase 1.D 着手起点 実装完了)
**Phase 1.D decomposition design-lock** ✅ (= commit `44c81ea228`、PC-N-6..PC-N-10 5 sub-step 分解)
**PC-N-6 design-lock** ✅ (= commit `146c3002e5`、Phase 1.D 内 1st sub-step design-lock)
**PC-N-6 実装** ✅ 本 commit (= Phase 1.D 内 1st sub-step 実装完了)

次:

- **PC-N-7 design-lock** ⏳ 次 session (= Phase 1.D 内 2nd sub-step = 実 LL::GLTF::Asset 経由 index buffer upload + `vkCmdBindIndexBuffer` + `vkCmdDrawIndexed`、design-lock phase 着手要)
- **PC-N-7 実装** ⏳ + **PC-N-8/9/10 design-lock + 実装** ⏳
- **Phase 1.D complete** ⏳ → **Phase 1 全完了** → **Mac/Win 開発者補完 phase** (= `ayastorm-r41-cross-platform-port-spec.md` 確定形提供)

---

## §6. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1 design-lock ✅ + PC-N-1 ✅ + PC-N-2 design-lock ✅ + PC-N-2 ✅ + PC-N-4 design-lock ✅ + PC-N-4 ✅ + PC-N-3 design-lock ✅ + PC-N-3 ✅ = **Phase 1.C complete ✅** + PC-8 Linux primary marker ✅ = **Phase 1.C strict 線形終了 ✅** + PC-N-5 design-lock ✅ + PC-N-5 ✅ = **Phase 1.D 着手起点 実装完了 ✅** + Phase 1.D decomposition design-lock ✅ + PC-N-6 design-lock ✅ + **PC-N-6 ✅ 本 commit = Phase 1.D 内 1st sub-step 実装完了** + PC-N-7 design-lock ⏳ 次 session + PC-N-7..PC-N-10 各 design-lock + 実装 ⏳ + Phase 1.D complete ⏳

---

## §7. self-verify 9 観点 全 ✅

1. **Exit Criteria 10 項全充足** §4 ✅
2. **必読 1 件 (本 complete handoff doc)** §1 + pinpoint reference 10 件別記 ✅
3. **step (a)..(g) 7 step 全実装** (= (a) sGltfStubVertexBuffer + (b) sGltfStubAssetPipeline 関数 + (c) initVulkan VMA allocate + (d) shutdownVulkan 対称破棄 + (e) recordGltfAssetDraw cvar 分岐 + (f) build verify + (g) handoff complete doc 起案) ✅
4. **ambiguity (N6-1)..(N6-13) 13 件 AYA literal「すべて推奨でお願いします」record (2026-06-05)** design-lock 継承、本実装で全件採用案通り実装 ✅
5. **GATE-B 整合** = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 §2.5 ✅
6. **MUSEUBO-A 整合** = `AYAGltfStubVertexBufferEnabled=false` default で発火なし、PC-N-5 完了状態と機能等価 + `mUseUBO=false` default で OpenGL 描画 100% 維持 + 4 段 graceful degrade §2.6 ✅
7. **build verify literal 取得** = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 全 PASS §3 ✅
8. **commit 内容** = 1 modified (llvkloader.cpp +343) + 1 modified (settings.xml +21) + 1 modified (cross-platform spec +2/-1) + 1 new doc (本 complete handoff) + 新 file 0 (除 doc) + CMake 改変 0 + codegen 改変 0 + shader 改変 0 + .h 改変 0 + Co-Authored-By 不在 ✅
9. **feedback_no_scope_shrink 遵守** = PC-N-6 literal scope 7 件 §0 全件実装、(N6-1) B 採用は AYA literal「すべて推奨でお願いします」record 済の段階分離 (= 「PC-N-6 literal scope 最小単位」確定、実 LL::GLTF::Asset 経路は PC-N-7..PC-N-10 持越)、縮小ではなく `feedback_ubo_migration_one_at_a_time` 厳格遵守整合 ✅

---

## §8. 引き継ぎ memory (= 次 session 向け要点)

1. **PC-N-6 = Phase 1.D 内 1st sub-step 完了** = stub vertex buffer Vulkan 経路通電 = file-static VMA buffer + 別 pipeline + bindVertexBufferVk + vkCmdDraw(N)
2. **`AYAGltfStubVertexBufferEnabled` cvar** default `false` Persist=1 = live A/B 経路、true 時のみ別 pipeline + 実 vertex buffer 経路発火、PC-N-5 stub draw 経路 (= cvar false 時の従来 path) は破壊しない
3. **2 stub draw 経路 並走 baseline** = PC-N-5 (= sAvatarBonePipeline + shader generate vkCmdDraw(3,1,0,0)) + PC-N-6 (= sGltfStubAssetPipeline + bindVertexBufferVk + vkCmdDraw(N))、PC-N-10 で両 stub 経路 cleanup
4. **`sGltfStubAssetPipeline` 別 pipeline** ((N6-7) B 採用) = `sAvatarBonePipeline` 同形 + vertex input state 拡張 (binding=0 stride=12 + location=0 R32G32B32_SFLOAT)、sAvatarBonePipeline は H10-A placeholder 専用温存、shader 改変ゼロ
5. **`sGltfStubVertexData` file-static CCW triangle** ((N6-3) A 採用) = NDC vec3 × 3 = 36 B、shader 側 `gl_VertexIndex` 経路 (sky smoke shader) ゆえ vertex attribute 実 consumption なし
6. **VMA host-visible mapped buffer pattern** ((N6-5) B 採用) = VMA_MEMORY_USAGE_AUTO + HOST_ACCESS_SEQUENTIAL_WRITE_BIT + MAPPED_BIT、stub 36 B 極小ゆえ staging overkill、PC-7β UBO ring buffer 同形 pattern
7. **VMA file-static cadence** ((N6-6) A 採用) = stub 固定 literal、ring buffer overkill、PC-N-5 sentinel lifecycle 整合、1 回 initVulkan 配置 + shutdownVulkan 破棄
8. **4 段 graceful degrade** = sAllocator / sGltfStubVertexBuffer / sGltfStubAssetPipeline / sGltfStubVertexMapped 各 nullptr guard、initVulkan で 4 件すべて check + LL_WARNS_ONCE で fallback
9. **PC-N-7 = 次 sub-step** = 実 LL::GLTF::Asset 経由 index buffer upload + `vkCmdBindIndexBuffer` + `vkCmdDrawIndexed`、本 PC-N-6 stub vertex buffer 経路を baseline に実 index data 取込で拡張
10. **GLTFSceneManager::render 改変 0 件** = PC-N-6 = llvkloader 層完結、`feedback_ubo_migration_one_at_a_time` 厳格遵守、GLTFSceneManager 統合は PC-N-9 持越
11. **recordGltfAssetDraw signature 不変** ((N6-2) A 採用) = file-static stub vertex data ゆえ Asset* 引数不要、PC-N-8 で拡張持越
12. **cross-platform spec §6 PC-N-6 行** = macOS / Windows 派生 fix 候補欄 = Phase 1 全完了時 Mac/Win 開発者補完 phase 用 reference、MoltenVK 標準対応範囲ゆえ派生 fix 候補なし
13. **build verify scope** = llrender + WARNING 0 + TUT 11+10+13 + codegen 131/131 (= PC-8 Linux primary marker 採用後の Phase 1.D 内 sub-step 標準 scope)
14. **`AYAGltfStubVertexBufferEnabled=true` 時の動作確認** は cold launch + Vulkan path 有効化 (= 別 cvar 経由) 時のみ可、本 PC-N-6 では設定枠配線 + build verify literal のみで完了
15. **feedback_no_scope_shrink 遵守** = (N6-1) B 採用は AYA literal「すべて推奨でお願いします」record 済の literal scope 最小単位確定、PC-N-7..PC-N-10 持越は段階分離

---

## §9. 次 session 着手 1 line

**PC-N-7 design-lock 着手** = 実 LL::GLTF::Asset 経由 index buffer upload + `vkCmdBindIndexBuffer` + `vkCmdDrawIndexed` (= Phase 1.D 内 2nd sub-step) = ambiguity 確認 + 実装計画分解 + Exit Criteria 明文化、`feedback_ubo_migration_one_at_a_time` 厳格遵守で本 PC-N-6 stub vertex buffer 経路を baseline に実 index data 取込で拡張、想定改変 file 3〜4 件 (llvkloader.cpp + settings.xml + LL::GLTF::Asset/Primitive 拡張可能性 + cross-platform spec §6 PC-N-7 行追記)。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 PC-N-6 complete handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 必読 1 件 + pinpoint reference 10 件別記、本 session も Read pinpoint のみ (= PC-N-6 design-lock doc + recordGltfAssetDraw 現状 + createAvatarBonePipeline body + initVulkan PC-N-5 (c) tag + shutdownVulkan PC-N-5 (d) tag + bindVertexBufferVk wrap + settings.xml AYAGltfStubDrawEnabled cvar pattern)、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §7
- **feedback_build_only_verified** 遵守 = llrender build + WARNING 0 + TUT 11/11 + 10/10 + 13/13 + codegen 131/131 で literal 検証取得 §3
- **feedback_no_scope_shrink** 遵守 = PC-N-6 literal scope 7 件 §0 全件実装、(N6-1) B 採用は AYA literal「すべて推奨でお願いします」record 済段階分離 (= 「PC-N-6 literal scope 最小単位」確定、実 LL::GLTF::Asset 経路は PC-N-7..PC-N-10 持越) = 縮小ではない、`feedback_ubo_migration_one_at_a_time` 厳格遵守整合
- **feedback_doubt_self_first** 遵守 = design-lock phase で ambiguity 13 件発見 + 推奨案提示 + AYA literal「すべて推奨でお願いします」受領後本実装、本実装中も createAvatarBonePipeline literal + VMA usage flag + bindVertexBufferVk signature を Read で literal 確認後配線、推測実装なし
- **feedback_confirm_referent_before_acting** 遵守 = 13 件 batch AYA 確認 design-lock phase で完了、本実装中も namespace 配置 (= nested anonymous namespace 内 file-static) 判断は既存 sGltfStubSkin 配置 pattern を literal 確認後採用
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = PC-N-6 = stub vertex buffer Vulkan 経路通電単独 sub-step (= file-static + 別 pipeline + cvar 切替)、PC-N-7..PC-N-10 残 4 sub-step は分離
- **feedback_design_phase_no_code_write** 整合 = 本 PC-N-6 は実装 phase = design-lock commit `146c3002e5` で `indra/` 改変 0 件完了済、本 session で `indra/llrender/llvkloader.cpp` + `indra/newview/app_settings/settings.xml` 改変は実装 phase ゆえ整合
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「commit してください」literal 受領後 commit 予定 (= 本 doc 起案完了時点では commit 未実施)
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在予定
- **feedback_no_bare_reference_ids** 遵守 = (N6-1)..(N6-13) 各 ID に項目名 / 採用案内容併記 + (a)..(g) 各 step に作業内容併記 §2
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、`git add` 個別 file 指定 + `git add -A` 不使用予定
- **memory `project_ayastorm_r41_design_principles`** 整合 = (1) Upstream OpenGL 取り込みやすさ維持 = `GLTFSceneManager::render` 改変 0 件 + sAvatarBonePipeline 不変温存 ((N6-7) B 別 pipeline 新設) + shader 改変ゼロ + (2) Core プロセス分散実現 = PC-N-7 以降の per-Primitive vertex buffer ownership design で実現
- **memory `project_r41_phase1b_vulkan_host_gate`** 整合 = GATE-B = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、cvar runtime gate のみ
- **memory `project_ayastorm_three_platforms`** 整合 = PC-8 Linux primary marker 採用 + cross-platform spec §6 PC-N-6 行 ✅ 状態更新済 (= 本 commit 内)、MoltenVK 標準対応範囲ゆえ macOS 派生 fix 候補なし + Windows full Vulkan ゆえ派生 fix 候補なし、Linux primary 完成 → 他者補完 model と整合
