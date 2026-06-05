# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.D **PC-N-5 complete**

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-N-5 (= Phase 1.D 着手起点 = 実 GLTF Vulkan draw 通電 1 stub) **実装 phase 完了 marker**。design-lock (commit `efad5f0200`) で確定した step (a)-(g) 7 step 実装 + Exit Criteria 10 項全充足 + build verify literal 取得 (llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131)。

> **本 doc 位置付け**: PC-N-5 design-lock (= `efad5f0200`) 後の **PC-N-5 実装 phase 完了 marker**。`recordGltfAssetDraw` 新設 + `sGltfStubSkin` 第 2 address-only sentinel 新設 + initVulkan register / shutdownVulkan unregister + `AYAGltfStubDrawEnabled` cvar 切替 hook 配線。次は PC-N-5 complete 後の Phase 1.D 内後続 sub-step (= PC-N-6 仮定以降) design-lock phase。

---

## §0. 本 session 着手契機 + PC-N-5 完了 scope record

**契機**: AYA 指示「r41 Phase 1.D PC-N-5 実装着手お願いします」literal 受領 (2026-06-05、PC-N-5 design-lock commit `efad5f0200` 後の継続 session = 別 session の fresh context) + 必読 1 件 = `handoff-substep-...-phase1-d-pc-n-5-design-lock.md` (= 10 件 ambiguity 全 AYA literal「OK」record 済) Read + pinpoint reference 12 件 Read → step (a)-(g) 7 step 実装 → build verify literal 取得 → 本 complete doc 起案。

**PC-N-5 完了 scope** (= design-lock §0 literal 5 件 + step (a)-(g) 7 step 全実装):

1. **`sGltfStubSkin` sentinel storage 新設** = 第 2 の address-only sentinel pattern (= `sPlaceholderSkin` 同形、`alignas(void*) char sGltfStubSkinStorage[1]` + `reinterpret_cast<LL::GLTF::Skin*>`、`anonymous namespace` 内、PC-N-3 §2.7 address-only sentinel pattern 踏襲) → llvkloader.cpp:582-601 (PC-N-5 (a) tag block)
2. **`recordGltfAssetDraw` 関数新設** = identity matrix bone matrix data 構築 + `writeDrawUbo` (PerDrawUBO_LightParams zero 256B) + `writeSkinUbo` (identity 256B) + `flushSkinUbos` + `bindV3aRigged` set=3 swap + push constant identity modelview + `vkCmdDraw(3,1,0,0)` + first-fire LL_INFOS marker → llvkloader.cpp:5479-5591 (PC-N-5 (b) tag block)
3. **`initVulkan` 内 stub Skin register + 初回 wire** = `ubo::g_block_metadata` walk で `Skin_GLTFJoints` block_size lookup + `registerSkinUbo(sGltfStubSkin, Skin_GLTFJoints, block_size)` + `wireSkinUboSetV3aToBinding2(sGltfStubSkin)` + 3 段 graceful degrade (block lookup miss / register fail / wire fail で LL_WARNS_ONCE) → llvkloader.cpp:3770-3823 (PC-N-5 (c) tag block)
4. **`shutdownVulkan` 内 stub Skin unregister** = `unregisterSkinUbo(sGltfStubSkin, ubo::block_hash::Skin_GLTFJoints)` 対称 lifecycle → llvkloader.cpp:4134-4140 (PC-N-5 (d) tag block)
5. **`AYAGltfStubDrawEnabled` cvar 新設** (= Boolean default `false` Persist=1) + `recordAvatarPlaceholderDraw` 末尾 cvar gate hook 経由 `recordGltfAssetDraw(cmd_buf)` 並走発火 → llvkloader.cpp:5714-5734 (PC-N-5 (e) tag block) + settings.xml:10405-10420 (cvar 1 件追加)

---

## §1. 必読 + pinpoint reference (= 次 session = Phase 1.D 内後続 sub-step design-lock 向け)

**次 session 必読**:

1. **本 PC-N-5 complete doc 全文**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-d-pc-n-5-complete.md`

**pinpoint reference** (= 後続 sub-step design-lock phase で必要分のみ Read):

- **PC-N-5 design-lock doc**: `handoff-...-phase1-d-pc-n-5-design-lock.md` (= ambiguity 10 件 AYA literal「OK」record + 実装計画 7 step + Exit Criteria 10 項)
- **本 PC-N-5 complete §2**: 実装内容 5 site (PC-N-5 (a)/(b)/(c)/(d)/(e) tag block) + line ref
- **本 PC-N-5 complete §3**: build verify literal (llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131)
- **`sGltfStubSkin` sentinel + `recordGltfAssetDraw`**: llvkloader.cpp:582-601 / 5479-5591
- **PC-N decomposition §5 PC-N-5 概略**: PC-N-5 後続 sub-step (= 実 LL::GLTF::Asset 経由 vertex buffer + GLTFSceneManager::render 統合) 設計参考
- **design 06b §2.5 + 06c §2.5**: `flushSkinUbos(skin)` を GLTFSceneManager::render 直前で配置 pattern (= 後続 sub-step 想定統合 site)
- **cross-platform spec §6 PC-N-5 行**: macOS / Windows 派生 fix 候補欄 (= Phase 1 全完了時の Mac/Win 開発者補完 phase 用)
- **GATE-B literal**: memory `project_r41_phase1b_vulkan_host_gate` (= 後続 sub-step も `#ifdef LL_VULKAN_GLSL` 新規追加 0 件維持)

---

## §2. 実装内容

### §2.1 編集 1 = `indra/llrender/llvkloader.cpp` (+220 行 / -0)

**5 編集 site** (= step (a)/(b)/(c)/(d)/(e)):

| # | site | line | tag block | 内容 |
|---|------|------|-----------|------|
| (a) | sentinel storage | 582-601 | `<AYAstorm r41 PC-N-5 (a)>` | `alignas(void*) char sGltfStubSkinStorage[1] = {}` + `LL::GLTF::Skin* const sGltfStubSkin = reinterpret_cast<LL::GLTF::Skin*>(&sGltfStubSkinStorage[0])` (= `sPlaceholderSkin` 直後、anonymous namespace 内、address-only pattern 踏襲) |
| (b) | recordGltfAssetDraw 関数 | 5479-5591 | `<AYAstorm r41 PC-N-5 (b)>` | nested anonymous namespace 内 file-static helper、cmd_buf / sAvatarBonePipeline / sAvatarBoneLayout / sAYAStandardLayout / sDrawUboRingBufferMgr nullptr guard + `vkCmdBindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, sAvatarBonePipeline)` + zero PerDrawUBO_LightParams 256B writeDrawUbo + identity matrix mat4 padded 256B writeSkinUbo + flushSkinUbos + bindV3aRigged set=3 swap + identity push constant 64B + `vkCmdDraw(3,1,0,0)` + first-fire LL_INFOS marker |
| (c) | initVulkan register + initial wire | 3770-3823 | `<AYAstorm r41 PC-N-5 (c)>` | `ubo::g_block_metadata` walk で `Skin_GLTFJoints` block_size lookup + `registerSkinUbo(sGltfStubSkin, ubo::block_hash::Skin_GLTFJoints, stub_skin_block_size)` + `wireSkinUboSetV3aToBinding2(sGltfStubSkin)` + 3 段 graceful degrade (block lookup miss / register fail / wire fail で LL_WARNS_ONCE)、PC-N-3 (c)+(d) tag block 直後配置 |
| (d) | shutdownVulkan unregister | 4134-4140 | `<AYAstorm r41 PC-N-5 (d)>` | `unregisterSkinUbo(sGltfStubSkin, ubo::block_hash::Skin_GLTFJoints)` (= PC-N-3 sPlaceholderSkin unregister と対称、bulk teardown loop 前) |
| (e) | cvar 発火 hook | 5714-5734 | `<AYAstorm r41 PC-N-5 (e)>` | `recordAvatarPlaceholderDraw` 末尾、`static LLCachedControl<bool> sAyastormGltfStubDrawEnabled(gSavedSettings, "AYAGltfStubDrawEnabled", false)` + `if (sAyastormGltfStubDrawEnabled) recordGltfAssetDraw(cmd_buf)` 並走発火 |

### §2.2 `indra/llrender/llvkloader.h` 改変 0 件

- `sGltfStubSkin` + `recordGltfAssetDraw` は anonymous namespace 内 file-static + helper、`LLVKLoader::` 公開 API 不変
- `LL::GLTF::Skin` forward declaration は llvkloader.h:31-32 既存維持
- design-lock §6.2 entry 3「.h 改変 0 件想定」literal 通り

### §2.3 codegen 改変 0 件

- `Skin_GLTFJoints` block (= block_hash + block_size + ubo_layout_skin_gltfjoints.inl) は PC-7γ-3 で配置済 + PC-N-3 で sPlaceholderSkin 経由通電済を再利用
- `scripts/ubo_codegen` 改変 0 件、`build-linux-x86_64/codegen/ubo/*.inl` 改変 0 件
- 結果 = codegen unittest 131/131 維持 (= (N5-9) A 整合)

### §2.4 shader 改変 0 件

- GLSL UBO layout は既存 `Skin_GLTFJoints` 再利用 (= PC-7γ-3 blueprint + PC-7γ-2 host 配線が baseline)
- `*.glsl` + `aya_r41_blueprints/` 改変 0 件

### §2.5 GATE-B 整合 (= memory `project_r41_phase1b_vulkan_host_gate` 遵守)

- `#ifdef LL_VULKAN_GLSL` **新規追加 0 件**
- PC-N-5 (e) hook の cvar gate は `LLCachedControl<bool>` runtime cvar 経由ゆえ `#ifdef` 非依存 = GATE-B 違反なし
- step (a)/(b)/(c)/(d) 全 site も `#ifdef LL_VULKAN_GLSL` 新規追加 0 件

### §2.6 MUSEUBO-A 整合 (= memory `feedback_ubo_migration_one_at_a_time` 整合)

- `AYAGltfStubDrawEnabled=false` default = `recordGltfAssetDraw` 発火なし = PC-N-3 完了状態と機能等価
- `AYAGltfStubDrawEnabled=true` 時のみ `recordAvatarPlaceholderDraw` 直後並走発火 = debug live A/B 経路
- `mUseUBO=false` default で既存 OpenGL 描画 100% 維持 = `recordGltfAssetDraw` は Vulkan placeholder offscreen FBO 経路、`mUseUBO` 不問
- 3 段 graceful degrade (block lookup miss / register fail / wire fail) で fallback、`recordGltfAssetDraw` 内 5 段 nullptr guard (cmd_buf / sAvatarBonePipeline / sAvatarBoneLayout / sAYAStandardLayout / sDrawUboRingBufferMgr) で安全運転

### §2.7 編集 2 = `indra/newview/app_settings/settings.xml` (+19 行 / -0)

`AYAGltfStubDrawEnabled` Boolean cvar 1 件追加 (= settings.xml:10405-10420):

```xml
<!-- <FS:AYAstorm r41 Phase 1.D PC-N-5> 実 GLTF Vulkan draw 通電 1 stub の live A/B 切替 cvar。
     default false で発火なし (PC-N-3 完了状態と機能等価)、true で recordGltfAssetDraw 並走発火。 -->
<key>AYAGltfStubDrawEnabled</key>
<map>
  <key>Comment</key>
  <string>(r41 Phase 1.D PC-N-5) Enable GLTF stub draw (recordGltfAssetDraw) parallel to recordAvatarPlaceholderDraw for live A/B testing. Default OFF.</string>
  <key>Persist</key><integer>1</integer>
  <key>Type</key><string>Boolean</string>
  <key>Value</key><integer>0</integer>
</map>
```

- `AYARingBufferSizeMB` / `AYAPipelineCacheSizeMB` 同形 r41 cvar pattern 踏襲
- Persist=1 で起動間設定保持 ((N5-4) A 採用根拠)

---

## §3. build verify literal (= (N5-9) A scope)

1. **llrender build**: `make -j4 llrender` → **PASS** / ERROR 0 / **WARNING 0**
2. **INTEGRATION_TEST_lluboringbuffer**: **11/11 PASS YAY**
3. **INTEGRATION_TEST_llassetubopool**: **10/10 PASS YAY**
4. **INTEGRATION_TEST_llpipelinecachestorage**: **13/13 PASS YAY**
5. **codegen unittest**: `python3 -m unittest discover tests` from `scripts/ubo_codegen/` → **131/131 OK** (= PC-N-1..PC-N-4 同形維持、(N5-9) A 整合)

= PC-3 / PC-4 / PC-5 algorithm 層 regression なし + Phase 1.A / 1.B / 1.C PC-1..PC-N-4 regression なし。

---

## §4. PC-N-5 Exit Criteria 10 項 全充足

| # | Criteria | 充足 |
|---|----------|------|
| (i) | `sGltfStubSkin` sentinel storage 新設 (anonymous namespace, `sPlaceholderSkin` 直後) ((N5-3) A) | ✅ llvkloader.cpp:582-601 |
| (ii) | `recordGltfAssetDraw` 関数新設 (= identity matrix + writeSkinUbo + flushSkinUbos + bindV3aRigged + vkCmdDraw + first-fire LL_INFOS marker) ((N5-1) A + (N5-5) A + (N5-6) A) | ✅ llvkloader.cpp:5479-5591 |
| (iii) | `initVulkan` 内 stub Skin register + 初回 wire (3 段 graceful degrade) ((N5-3) A + (N5-7) A) | ✅ llvkloader.cpp:3770-3823 |
| (iv) | `shutdownVulkan` 内 stub Skin unregister (= 対称 lifecycle) | ✅ llvkloader.cpp:4134-4140 |
| (v) | `AYAGltfStubDrawEnabled` cvar 新設 (Boolean default `false` Persist=1) + `recordGltfAssetDraw` 発火 hook ((N5-4) A) | ✅ llvkloader.cpp:5714-5734 + settings.xml:10405-10420 |
| (vi) | identity matrix bone data writeSkinUbo + flushSkinUbos + bindV3aRigged 正規 sequence (= design 06b §2.5 GLTFSceneManager::render 直前 pattern 踏襲) ((N5-6) A + (N5-7) A) | ✅ llvkloader.cpp:5479-5591 内 |
| (vii) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 | ✅ §2.5 |
| (viii) | MUSEUBO-A 整合 = `mUseUBO=false` default + `AYAGltfStubDrawEnabled=false` default で stub draw 発火なし、PC-N-3 完了状態と機能等価 | ✅ §2.6 |
| (ix) | build verify llrender + WARNING 0 + TUT 11+10+13 + codegen 131/131 全 PASS ((N5-9) A) | ✅ §3 |
| (x) | tag block 統一 PC-N-5 (a)/(b)/(c)/(d)/(e) + first-fire LL_INFOS marker + handoff complete doc 起案 | ✅ 本 commit |

---

## §5. 残 strict 線形

**Phase 1.C complete** ✅ (= PC-N-3 commit `71f7bb2a89`)
**PC-8 Linux primary marker** ✅ (= commit `3c0c72d34f`、Phase 1.C strict 線形終了)
**PC-N-5 design-lock** ✅ (= commit `efad5f0200`、Phase 1.D 着手起点 design-lock)
**PC-N-5 実装** ✅ 本 commit (= Phase 1.D 着手起点 実装完了)

次:

- **Phase 1.D 内後続 sub-step (= PC-N-6 仮定以降) design-lock** ⏳ 次 session (= PC-N-5 後続 sub-step 群を別 design-lock phase で分解、(N5-1) B (= 実 LL::GLTF::Asset 経由 vertex buffer upload) + (N5-1) C (= GLTFSceneManager::render 内 mUseUBO gate 配線) 等)
- **Phase 1 全完了** → **Mac/Win 開発者補完 phase** (= `ayastorm-r41-cross-platform-port-spec.md` 確定形提供)

---

## §6. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1 design-lock ✅ + PC-N-1 ✅ + PC-N-2 design-lock ✅ + PC-N-2 ✅ + PC-N-4 design-lock ✅ + PC-N-4 ✅ + PC-N-3 design-lock ✅ + PC-N-3 ✅ = **Phase 1.C complete ✅** + PC-8 Linux primary marker ✅ = **Phase 1.C strict 線形終了 ✅** + PC-N-5 design-lock ✅ + **PC-N-5 ✅ 本 commit = Phase 1.D 着手起点 実装完了** + Phase 1.D 内後続 sub-step ⏳ 次 session

---

## §7. self-verify 9 観点 全 ✅

1. **Exit Criteria 10 項全充足** §4 ✅
2. **必読 1 件 (本 complete handoff doc)** §1 + pinpoint reference 8 件別記 ✅
3. **step (a)..(g) 7 step 全実装** (= (a) sentinel + (b) record 関数 + (c) register/wire + (d) unregister + (e) cvar 発火 hook + (f) build verify + (g) handoff complete doc 起案) ✅
4. **ambiguity (N5-1)..(N5-10) 10 件 AYA literal「OK」record (2026-06-05)** design-lock 継承、本実装で全件採用案通り実装 ✅
5. **GATE-B 整合** = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 §2.5 ✅
6. **MUSEUBO-A 整合** = `AYAGltfStubDrawEnabled=false` default で発火なし、PC-N-3 完了状態と機能等価 + `mUseUBO=false` default で OpenGL 描画 100% 維持 + 5 段 nullptr guard + 3 段 graceful degrade §2.6 ✅
7. **build verify literal 取得** = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 全 PASS §3 ✅
8. **commit 内容** = 1 modified (llvkloader.cpp +220) + 1 modified (settings.xml +19) + 1 new doc (本 complete handoff) + 新 file 0 (除 doc) + CMake 改変 0 + codegen 改変 0 + shader 改変 0 + .h 改変 0 + Co-Authored-By 不在 ✅
9. **feedback_no_scope_shrink 遵守** = PC-N-5 literal scope 5 件 §0 全件実装、(N5-1) A 採用は AYA literal「OK」record 済段階分離 (= 「実 GLTF Vulkan draw」literal 解釈確定、(N5-1) B/C は Phase 1.D 内後続 sub-step 持越)、縮小ではなく `feedback_ubo_migration_one_at_a_time` 厳格遵守整合 ✅

---

## §8. 引き継ぎ memory (= 次 session 向け要点)

1. **PC-N-5 = Phase 1.D 着手起点 完了** = `recordGltfAssetDraw` 新設 + `sGltfStubSkin` 第 2 sentinel + cvar 切替 hook 配線
2. **`AYAGltfStubDrawEnabled` cvar** default `false` Persist=1 = live A/B 経路、true 時のみ `recordAvatarPlaceholderDraw` 直後並走発火
3. **address-only sentinel pattern** = `sPlaceholderSkin` (PC-N-3) + `sGltfStubSkin` (PC-N-5) 同形、`UboSkinKey<Skin*, block_hash>` で discrimination
4. **identity matrix bone data 256B** = `Skin_GLTFJoints` UBO layout 通電確認、zero (sentinel) と区別可能 ((N5-6) A)
5. **3 段 graceful degrade** initVulkan (block lookup miss / register fail / wire fail) + 5 段 nullptr guard recordGltfAssetDraw (cmd_buf / pipeline / layout / standardlayout / ringbuffer mgr)
6. **bindV3aRigged signature 不変** ((N5-7) A) = `UboSkinKey` map 第 1 要素 Skin* pointer compare で sentinel と stub Skin* 同居 discrimination
7. **GLTFSceneManager::render 改変 0 件** ((N5-8) A) = PC-N-5 = llrender 層完結、`feedback_ubo_migration_one_at_a_time` 厳格遵守
8. **後続 sub-step (= PC-N-6 仮定以降)** = (N5-1) B (= 実 LL::GLTF::Asset 経由 vertex buffer upload) + (N5-1) C (= GLTFSceneManager::render 内 mUseUBO gate 配線) 想定、design-lock phase で別途分解
9. **PC-N-3 §2.7 address-only sentinel pattern** が Phase 1.D 全 sub-step の baseline (= `LL::GLTF::Skin` forward-decl only 制約遵守 + 後段 per-Skin parallelization 基盤)
10. **cross-platform spec §6 PC-N-5 行** = macOS / Windows 派生 fix 候補欄 = Phase 1 全完了時 Mac/Win 開発者補完 phase 用 reference
11. **build verify scope** = llrender + WARNING 0 + TUT 11+10+13 + codegen 131/131 (= PC-8 Linux primary marker 採用後の Phase 1.D 内 sub-step 標準 scope)
12. **PC-N-3 sPlaceholderSkin + PC-N-5 sGltfStubSkin** = 2 sentinel 並走 = Phase 1.D 内後続 sub-step で 3rd sentinel 不要、実 LL::GLTF::Asset 経由経路に移行想定
13. **`AYAGltfStubDrawEnabled=true` 時の動作確認** は cold launch + Vulkan path 有効化 (= 別 cvar 経由) 時のみ可、本 PC-N-5 では設定枠配線 + build verify literal のみで完了
14. **feedback_no_scope_shrink 遵守** = (N5-1) A 採用は AYA literal「OK」record 済の literal 解釈確定、後続 sub-step 持越は段階分離

---

## §9. 次 session 着手 1 line

**Phase 1.D 内後続 sub-step (= PC-N-6 仮定以降) design-lock 着手** = PC-N-5 後続 sub-step 群を別 design-lock phase で分解 (= ambiguity 確認 + 実装計画分解 + Exit Criteria 明文化)、想定 scope = (N5-1) B (= 実 LL::GLTF::Asset 経由 vertex buffer upload) + (N5-1) C (= GLTFSceneManager::render 内 mUseUBO gate 配線) + 他 Phase 1.D 内 sub-step 群、`feedback_ubo_migration_one_at_a_time` 厳格遵守で 1 sub-step ずつ分解。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 PC-N-5 complete handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 必読 1 件 + pinpoint reference 8 件別記、本 session も Read pinpoint のみ (= design-lock doc + recordAvatarPlaceholderDraw + sPlaceholderSkin + writeSkinUbo + flushSkinUbos + registerSkinUbo + wireSkinUboSetV3aToBinding2 + ubo_metadata.inl + AYARingBufferSizeMB cvar pattern)、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §7
- **feedback_build_only_verified** 遵守 = llrender build + WARNING 0 + TUT 11/11 + 10/10 + 13/13 + codegen 131/131 で literal 検証取得 §3
- **feedback_no_scope_shrink** 遵守 = PC-N-5 literal scope 5 件 §0 全件実装、(N5-1) A 採用は AYA literal「OK」record 済段階分離 (= 「実 GLTF Vulkan draw」literal 解釈確定、(N5-1) B/C は Phase 1.D 内後続 sub-step 持越) = 縮小ではない、`feedback_ubo_migration_one_at_a_time` 厳格遵守整合
- **feedback_doubt_self_first** 遵守 = design-lock phase で ambiguity 10 件発見 + 推奨案提示 + AYA literal「OK」受領後本実装、本実装中も recordAvatarPlaceholderDraw signature + bindV3aRigged calling convention + ubo_metadata.inl literal を Read で literal 確認後配線、推測実装なし
- **feedback_confirm_referent_before_acting** 遵守 = 10 件 batch AYA 確認 design-lock phase で完了、本実装中も namespace 配置 (= nested anonymous namespace) 判断は既存 sPlaceholderSkin 配置 pattern を literal 確認後採用
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = PC-N-5 = 実 GLTF Vulkan draw 通電 1 stub 単独 sub-step (= 第 2 sentinel-like skin + recordGltfAssetDraw 新設 + cvar 切替)、(N5-1) B (= 実 LL::GLTF::Asset 経由 vertex buffer) + (N5-1) C (= GLTFSceneManager::render 統合) は Phase 1.D 内後続 sub-step に分離
- **feedback_design_phase_no_code_write** 整合 = 本 PC-N-5 は実装 phase = design-lock commit `efad5f0200` で `indra/` 改変 0 件完了済、本 session で `indra/llrender/llvkloader.cpp` + `indra/newview/app_settings/settings.xml` 改変は実装 phase ゆえ整合
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「commit してください」literal 受領後 commit 予定
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在予定
- **feedback_no_bare_reference_ids** 遵守 = (N5-1)..(N5-10) 各 ID に項目名 / 採用案内容併記 + (a)..(g) 各 step に作業内容併記 §2
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、`git add` 個別 file 指定 + `git add -A` 不使用予定
- **memory `project_ayastorm_r41_design_principles`** 整合 = (1) Upstream OpenGL 取り込みやすさ維持 = `GLTFSceneManager::render` 改変 0 件 ((N5-8) A 採用) + (2) Core プロセス分散実現 = sentinel + stub Skin* 並走 baseline (= 後段 per-Skin parallelization 基盤)
- **memory `project_r41_phase1b_vulkan_host_gate`** 整合 = GATE-B = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、cvar runtime gate のみ
- **memory `project_ayastorm_three_platforms`** 整合 = PC-8 Linux primary marker 採用 + cross-platform spec §6 PC-N-5 行追記済 (= design-lock commit `efad5f0200` 内)、Linux primary 完成 → 他者補完 model と整合
