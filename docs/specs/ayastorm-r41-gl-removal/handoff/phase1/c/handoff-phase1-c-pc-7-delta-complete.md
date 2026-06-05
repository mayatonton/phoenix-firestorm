# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-7δ complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-7δ (= vkCmdBindDescriptorSets **通電** + V3a 5-set 構成完成 + set=3 swap + sAYAStandardLayout 経由 bind + SINGLETON case 本格化) 完了 marker + PC-7ε (= dynamic offset 経路 ring buffer chunk hand-off) 引継

---

## §0. 必読 3 件 (次 session 着手前)

1. **本 handoff doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-delta-complete.md`
2. **PC-7δ design-lock handoff doc**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-delta-design-lock.md`
   - (H1-A)..(H10-A) 9+1 件 ambiguity 採用根拠 + §4.1 step (a)..(p) 16 step 実装計画 + §5 Exit Criteria 11 項
3. **design 06c §2 全体 + design 07 §9.3 + §12 + design 06b §3.2.3**
   - pinpoint reference 別記:
     - `docs/specs/ayastorm-r41-gl-removal/design/06c-buffer-layout-and-binding.md` §2 (V3a 5-set scheme = PC-7δ で bind 通電完了、PC-7ε で dynamic offset 通電予定)
     - `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md` §9.3 (dynamic offset 経路 = PC-7ε scope) + §12 (frame loop)
     - `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md` §3.2.3 (UboInstance struct 完成形、本 PC-7δ で sSingletonUboInstances 追加)

---

## §1. PC-7δ 完了内容 = vkCmdBindDescriptorSets **通電** + V3a 5-set **構成完成** + SINGLETON **本格化**

### §1.0 scope = AYA design-lock 確認 9+1 件実装 (= design-lock doc §5 Exit Criteria 11 項全充足)

PC-7δ **literal scope 5 件** (= AYA「PC-7δ 実装着手お願いします」literal 受領 2026-06-05、design-lock §3 で 9+1 件 ambiguity 全 AYA literal「OK」record 完了):

1. **vkCmdBindDescriptorSets 通電** = V3a 5-set bind 経路の実 firing
2. **set=3 swap** = rigged GLTF draw 切替時の set=2 ↔ set=3 swap helper 整備
3. **sAYAStandardLayout 経由 bind** = placeholder draw 2 件 (= sSkySmoke / sAvatarBone) を sAYAStandardLayout 統一
4. **V3a 5-set 構成完成** = VkDescriptorSet 実体 allocate + vkUpdateDescriptorSets + vkCmdBindDescriptorSets 通電
5. **SINGLETON case 本格化** = `LLVKLoader::writeSingletonUbo(block_hash, ...)` 新設 + forwardToUboUpload SINGLETON case `llassert_always(false)` → 通電

**本 PC-7δ scope 外 (= PC-7ε 以降 scope)**:
- dynamic offset 経路 ring buffer chunk hand-off (= PC-7ε)
- codegen ubo_metadata.inl V1' set=1a/1b split (= PC-7α'、Z2-C 持越、PC-7ε 通電前に必要)
- 3 OS build verify (= PC-8)
- avatar bone storage buffer 経路再配線 (= PC-N 実 GLTF avatar Vulkan draw 通電時、H10-A 整合)

### §1.1 採用根拠 9+1 件 (= AYA literal「OK」受領 record 2026-06-05)

(H1-A) bind path target = placeholder draw 2 件を sAYAStandardLayout + V3a 5-set bind に migrate:
- 根拠 = literal「通電」整合 (= 実 bind firing 経路を本 PC-7δ で確立)、placeholder offscreen FBO ゆえ MUSEUBO-A 完全維持 (= mUseUBO=false default で OpenGL 描画 100% 維持)、avatar 経路 (= H10-A 統合) で set=3 swap literal も同時達成

(H2-A) VkDescriptorSet allocation timing = initVulkan で V3a pool から eager allocate:
- 根拠 = FRAMES_IN_FLIGHT=3 × cadence 固定数 計 13 set 一括 (= sFrameUboSetV3a × 3 + sProgramUboSetA × 3 + sProgramUboSetB × 3 + sDrawUboSetV3a × 1 + sAssetUboSetV3a × 3)、design 07 §6.4 grow only 整合、frame 内 allocate churn 排除

(H3-A) vkUpdateDescriptorSets timing = register*Ubo 内 = UboInstance 確保直後に update:
- 根拠 = per-instance pair (= shader + block_hash) で hot path 除外、register-once + bind-many 設計、PC-7γ-1..γ-3 register hook と統合的 (= 1 sub-step で確立)

(H4-B) set=3 swap 実走 timing = bind helper 整備のみ、実 swap は実 GLTF Vulkan draw 通電 sub-step (PC-N) で発火:
- 根拠 = 実 GLTF Vulkan draw 不在で実 swap は意味なし、bind helper API (bindV3aStatic / bindV3aRigged) 整備で set=2 ↔ set=3 swap 経路完成、本 PC-7δ で avatar placeholder 経由 bindV3aRigged 経路の actual firing 確立

(H5-A) SINGLETON case 本格化 = `LLVKLoader::writeSingletonUbo(block_hash, ...)` 新設 + 呼出:
- 根拠 = writeFrameUbo / writeProgramUbo 等と pattern 統一 (= setter ↔ flush 2 経路統合)、sSingletonUboInstances 別 map で cadence 隔離 (= key 単純化 = block_hash 単独、shader-agnostic)、flushSingletonUbos 本格化と連携 (= PC-6ε-1 placeholder → register-once + bind-many 経路完成)

(H6) §4.4.1 literal 採用 = static は set=0/1a/1b/2、rigged は set=0/1a/1b/3:
- 根拠 = maxBoundDescriptorSets=4 死守 (= Vulkan spec minimum)、set=2 ↔ set=3 swap 経路で rigged GLTF draw 時に set=2 を unbind + set=3 を bind = bind 状態は常に 4 set 以下、bindV3aRigged で set=0/1a/1b 連続 bind + set=3 単独 bind の 2 vkCmdBindDescriptorSets call (= set=2 skip ゆえ非連続)

(H7) GATE-B 追加なし = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件:
- 根拠 = GATE-B 規律 (= GLSL 改変 0 件、host C++ + LLGLSLShader::forwardToUboUpload SINGLETON case のみ)、mUseUBO runtime gate 単独 + LLVKLoader internal defensive guard 並走で十分

(H8) MUSEUBO-A 整合 = (H1-A) で成立:
- 根拠 = placeholder offscreen FBO 経路ゆえ実 OpenGL 描画影響ゼロ、bind 実発火は不可視 transition (= sOffscreenFramebuffer 経由)、mUseUBO=false default 経路 (= setter 不呼出) と独立

(H9-A) V3a 5-set 全 cadence bind 配線:
- 根拠 = literal「V3a 5-set 構成完成」整合、UBO migration one-at-a-time は PC-7 全体 sub-step 区切りで成立 (= PC-7γ 系 = register 経路、PC-7δ = bind 経路、PC-7ε = dynamic offset)、cadence 内分割不要

(H10-A) avatar placeholder pipeline layout compatibility = push descriptor 経路 (STORAGE_BUFFER) を PC-7δ で disable + sAvatarBonePipeline & sSkySmokePipeline の pipeline layout を sAYAStandardLayout で再構築:
- 根拠 = scope 拡大せず set=3 swap literal 達成、placeholder ゆえ視覚 no-op 等価維持、avatar bone storage 経路は PC-N 実 GLTF avatar Vulkan draw 通電時に再配線 (= storage buffer は別 layout 路で再導入)、(H10-B) sAYAStandardLayout に STORAGE_BUFFER 追加 = V3a 5-set 設計逸脱、(H10-C) bind 別 frame phase = literal 違反 で排除

### §1.2 編集 1 (indra/llrender/llvkloader.h +6 / -0) = step (l) writeSingletonUbo 宣言

PC-7δ (l) tag block で `void writeSingletonUbo(U32 block_hash, U32 offset, const void* data, size_t size);` 宣言追加。`writeFrameUbo` 直後配置で signature 同形 (= block_hash 単独 key) + sSingletonUboInstances target。

### §1.3 編集 2 (indra/llrender/llvkloader.cpp、合計 +689 / -101) = step (a)-(p) 全実装

#### §1.3.1 step (a)(k): V3a 5-set static array + sSingletonUboInstances map 新設 (line 680-723)

PC-7δ tag block (= V3a 5-set 実体 + per-singleton UboInstance map):
- `sFrameUboSetV3a[FRAMES_IN_FLIGHT]`、`sProgramUboSetA[FRAMES_IN_FLIGHT]`、`sProgramUboSetB[FRAMES_IN_FLIGHT]` = 各 3 件 (per-frame + per-program A/B、frame 単位 set 切替)
- `sDrawUboSetV3a` = 1 件 (= ring buffer + dynamic offset で 1 set 固定、PC-7ε で dynamic offset 経路完成予定)
- `sAssetUboSetV3a[FRAMES_IN_FLIGHT]` = 3 件 (per-asset + per-skin 同居、set=3 swap target)
- `sSingletonUboInstances` = `std::unordered_map<U32, UboInstance>` (= block_hash 単独 key、shader-agnostic)

#### §1.3.2 step (b)(c): createV3aDescriptorSets helper 新設 + initVulkan 配線 (line 2535-2670, 3329-3337)

- `createV3aDescriptorSets()` helper 新設 = 5 set × FRAMES_IN_FLIGHT/cadence で計 13 set を V3a pool (sV3aDescriptorPoolFrame / Program / Draw / Asset) から `vkAllocateDescriptorSets` 一括 allocate (= H2-A eager)
- initVulkan 配線 = createV3aDescriptorPools() 後 `!createV3aDescriptorSets()` chain に追加 (= line 3329-3334)

#### §1.3.3 step (d)(m): PER_FRAME + SINGLETON allocate ループ追加 + vkUpdateDescriptorSets (line 3400-3500 周辺)

- PER_FRAME 3 block (FrameViewProj / FrameLights / FrameAtmosphere_Lighting) allocate ループ末尾に vkUpdateDescriptorSets 呼出 = sFrameUboSetV3a[3] へ binding=0/1/2 write
- SINGLETON cadence allocate ループ追加 = `ubo_metadata.inl` walk で `cadence_tag == kCadenceSingleton`(=5) 拾い、sSingletonUboInstances[block_hash] = UboInstance 確保 + sFrameUboSetV3a[3] binding=3 (= Global_ReflectionProbes) へ vkUpdateDescriptorSets

#### §1.3.4 step (e): register*Ubo 末尾 vkUpdateDescriptorSets 追加 (3 method)

- **registerProgramUbo**: binding<40 → sProgramUboSetA / binding>=40 → sProgramUboSetB (binding-40) 暫定 heuristic (= PC-7α' codegen V1' split 完了まで)、FRAMES_IN_FLIGHT=3 triple-buffer vkUpdateDescriptorSets、binding>=80 で LL_WARNS_ONCE
- **registerAssetUbo**: sAssetUboSetV3a[3] へ set=3 binding=0/1 (= Asset_GLTFNodes / Asset_GLTFMaterials) triple-buffer vkUpdateDescriptorSets
- **registerSkinUbo**: sAssetUboSetV3a[3] へ set=3 binding=2 (= Skin_GLTFJoints) triple-buffer vkUpdateDescriptorSets

#### §1.3.5 step (f)(n): shutdownVulkan teardown 配線 (line ~3680 周辺)

- 13 set 個別 vkFreeDescriptorSets 不要 (= pool destroy で implicit free)、PC-7δ (f) tag block で 13 set handle defensive nulling (= 再 init safe、createV3aDescriptorSets 再呼出時)
- sSingletonUboInstances 全件 destroyUboInstanceBuffers 呼出 + .clear() (= sFrameUboInstances teardown 直後配置、init reverse 順遵守)

#### §1.3.6 step (g)(h): bindV3aStatic / bindV3aRigged helper 新設 (line 1740-1808)

- **bindV3aStatic(cmd_buf, frame_index)**: set=0/1a/1b/2 連続 4 set bind + V3A_DRAW_SET_BINDINGS=4 zero dynamic offsets (= PC-7ε で実 dynamic offset 計算予定)
- **bindV3aRigged(cmd_buf, frame_index)**: set=0/1a/1b 3 set 連続 bind + set=3 単独 bind の 2 vkCmdBindDescriptorSets call (= set=2 ↔ set=3 swap 経路、非連続 set skip)

#### §1.3.7 step (i): recordPlaceholderPoolDraw bind path migrate + sSkySmokePipeline 再構築

- createSkySmokePipeline (line ~2928-2942): 旧 `createStandardPipelineLayout(set_layouts, 2, push_constants, 1)` 12 行 block → `sSkySmokeLayout = sAYAStandardLayout` alias (= sAYAStandardLayout statically guard + alias 共用)
- shutdownVulkan: sSkySmokeLayout destroy alias-detection guard (= `sSkySmokeLayout != sAYAStandardLayout` 時のみ vkDestroyPipelineLayout)
- recordPlaceholderPoolDraw: 旧 `sPerFrameDescriptorSet[sFrameIndex]` + `bindPerMaterialDescriptorSet` 2-step → `bindV3aStatic(cmd_buf, sFrameIndex)` 1 call、push constant 64 B identity + vkCmdDraw(3,1,0,0) 不変

#### §1.3.8 step (j): recordAvatarPlaceholderDraw bind path migrate + sAvatarBonePipeline 再構築 + push descriptor disable

- createAvatarBonePipeline (line 3124-3145): 旧 `createStandardPipelineLayout(set_layouts[3]={PerFrame, PerMaterial, AvatarBone}, push_constants, 1)` 13 行 block → `sAvatarBoneLayout = sAYAStandardLayout` alias (= H10-A 採用)
- shutdownVulkan: sAvatarBoneLayout destroy alias-detection guard (= sSkySmokeLayout と symmetric)
- recordAvatarPlaceholderDraw: 旧 push descriptor (STORAGE_BUFFER for sAvatarBoneStorageBuffer) + 2 step bind 経路 → `bindV3aRigged(cmd_buf, sFrameIndex)` 1 call、push constant 64 B identity + vkCmdDraw(3,1,0,0) 不変、push descriptor 経路 (= vkCmdPushDescriptorSetKHR) disable (= PC-N で再配線、H10-A 整合)

#### §1.3.9 step (l): writeSingletonUbo 定義新設 (line 4552-4595)

- signature = `void writeSingletonUbo(U32 block_hash, U32 offset, const void* data, size_t size)`、writeFrameUbo pattern 同形 + sSingletonUboInstances target
- defensive guard = sAllocator 未初期化 / sSingletonUboInstances.find miss / out of range write で LL_WARNS_ONCE + no-op return (= MUSEUBO-A integrity 維持)
- success path = memcpy(ubo.mapped_ptr[sFrameIndex], data, size) + ubo.dirty.store(true, release)

#### §1.3.10 step (o): flushSingletonUbos 本格化

- 旧 placeholder (= bringupTestUBO 経由 flushDummyUboWrite 単発呼出) → 新 = sSingletonUboInstances walk + 各 entry の dirty.exchange(false, acq_rel) で dirty dedup、register-once + bind-many 経路完成 (= bind は initVulkan vkUpdateDescriptorSets で済、毎 flush は write 確定のみ)

### §1.4 編集 3 (indra/llrender/llglslshader.cpp +16 / -2) = step (p) SINGLETON case 本格化

PC-7δ (p) tag block で forwardToUboUpload SINGLETON case (= line 2204-2210) を `llassert_always(false)` placeholder → `LLVKLoader::writeSingletonUbo(loc.block_hash, loc.offset, data, size)` 通電。setter 経由 SINGLETON write 経路確立 = sSingletonUboInstances register-once + bind-many と整合 (= H5-A pattern 完成)。

---

## §2. build verify (= AYA `feedback_build_only_verified` 整合)

### §2.1 llrender build

```
make -j4 llrender
[100%] Building CXX object llrender/CMakeFiles/llrender.dir/llglslshader.cpp.o
[100%] Building CXX object llrender/CMakeFiles/llrender.dir/llvkloader.cpp.o
[100%] Linking CXX static library libllrender.a
[100%] Built target llrender
```

= ERROR 0 / WARNING 0 (= PC-7δ 改変関連、llvkloader.h / llvkloader.cpp / llglslshader.cpp)

### §2.2 TUT integration tests

```
sharedlibs/bin/INTEGRATION_TEST_lluboringbuffer
  Total Tests: 11 / Passed Tests: 11 YAY!! \o/

sharedlibs/bin/INTEGRATION_TEST_llassetubopool
  Total Tests: 10 / Passed Tests: 10 YAY!! \o/

sharedlibs/bin/INTEGRATION_TEST_llpipelinecachestorage
  Total Tests: 13 / Passed Tests: 13 YAY!! \o/
```

= PC-3 / PC-4 / PC-5 algorithm 層 regression なし

### §2.3 codegen unittest

```
python3 -m unittest discover -s scripts/ubo_codegen/tests
Ran 130 tests in 0.061s
OK
```

= 130/130 PASS = Phase 1.A / 1.B / 1.C PC-1..PC-7γ-3 regression なし

### §2.4 Vulkan validation

本 PC-7δ では実 Vulkan device 上の runtime validation (= vkCreate/Update/Bind ライフタイム検証) は build verify 対象外 (= AYAstorm cold launch で AYA さん側 verify)。host-side static check (= layout compatibility / set index 範囲 / descriptor type 一致) は code review + symmetric pattern (= sSkySmokeLayout / sAvatarBoneLayout の alias 検出 + 13 set defensive nulling + register-once 内 update timing) で担保。

---

## §3. PC-7δ Exit Criteria 11 項全充足

| # | Exit Criteria | 充足状況 |
|---|--------------|---------|
| (i) | sFrameUboSetV3a / sProgramUboSetA / sProgramUboSetB / sDrawUboSetV3a / sAssetUboSetV3a 5 件 static array + `createV3aDescriptorSets()` helper + initVulkan 配線 | ✅ §1.3.1 / §1.3.2 |
| (ii) | vkUpdateDescriptorSets 呼出 = PER_FRAME initVulkan 内 3 件 + registerProgramUbo / registerAssetUbo / registerSkinUbo 末尾 各 method 内 | ✅ §1.3.3 / §1.3.4 |
| (iii) | sSingletonUboInstances map + writeSingletonUbo helper + initVulkan SINGLETON allocate ループ + shutdownVulkan teardown | ✅ §1.3.1 / §1.3.3 / §1.3.5 / §1.3.9 |
| (iv) | forwardToUboUpload SINGLETON case 本格化 = `llassert_always(false)` → `writeSingletonUbo(...)` | ✅ §1.4 |
| (v) | bindV3aStatic / bindV3aRigged helper 新設 = vkCmdBindDescriptorSets で 4 set 構成 bind (set=2 ↔ set=3 swap) | ✅ §1.3.6 |
| (vi) | recordPlaceholderPoolDraw bind path migrate = bindV3aStatic 呼出 + sSkySmokePipeline pipeline layout 再構築 | ✅ §1.3.7 |
| (vii) | recordAvatarPlaceholderDraw bind path migrate = bindV3aRigged 呼出 + sAvatarBonePipeline pipeline layout 再構築 + push descriptor 経路 disable | ✅ §1.3.8 |
| (viii) | flushSingletonUbos 本格化 = sSingletonUboInstances walk + dirty exchange | ✅ §1.3.10 |
| (ix) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 | ✅ §6 (4) |
| (x) | MUSEUBO-A 整合 = mUseUBO=false default で既存 OpenGL 描画 100% 維持、bind 実発火は placeholder offscreen FBO 経路 | ✅ §6 (5) |
| (xi) | build verify = llrender build PASS + warning 0 + TUT 3 件 (11+10+13) + codegen 130/130 PASS | ✅ §2 |

---

## §4. 残 strict 線形

- **PC-7ε** = dynamic offset 経路 ring buffer chunk hand-off = set=2 per-draw を sDrawUboRingBufferMgr 経由 dynamic offset 計算 + pDynamicOffsets[4] 実値配線 (= 本 PC-7δ は zero 埋め placeholder)、source doc = 07 §9.3
- **PC-7α'** = codegen ubo_metadata.inl V1' update = scripts/ubo_codegen/main.py で set=1a/1b split 実装 + ubo_metadata.inl 再生成 + 130 件 unittest 回帰確認、Z2-C 持越、PC-7ε / PC-N 通電前に必要 (= 暫定 binding<40 → 1a / binding>=40 → 1b heuristic 撤去)
- **PC-8** = 3 OS build verify、Linux primary + Win/Mac 後段
- **PC-N** = Phase 1.C complete marker + 実 GLTF Vulkan draw 通電 + avatar bone storage 経路再配線 (= H10-A 持越分) + Phase 1.D / Phase 2 着手起点

---

## §5. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + **PC-7δ ✅ 本 commit** + PC-7ε..PC-N ⏳ 次 session 以降

---

## §6. self-verify 9 観点 全 ✅

1. **Exit Criteria 11 項全充足** = §3 確認
2. **必読 3 件 Read 完了** = design-lock doc + design 06c §2 + 07 §9.3/§12 + 06b §3.2.3
3. **(H1-A)..(H10-A) 9+1 件採用根拠 record** = §1.1 で 10 件採用根拠明文化、AYA literal「OK」(2026-06-05) record
4. **GATE-B 整合** = `git diff indra/ | grep "^+" | grep LL_VULKAN_GLSL` = 1 hit (= comment 1 行「新規追加 0 件」宣言)、実 `#ifdef LL_VULKAN_GLSL` 追加 0 件
5. **MUSEUBO-A 整合** = mUseUBO=false default 経路 (= forwardToUboUpload 不呼出) 不変 + placeholder offscreen FBO 経路 (= recordPlaceholderPoolDraw / recordAvatarPlaceholderDraw) で実 OpenGL 描画影響ゼロ、bind 実発火は不可視 transition、avatar bone storage 経路 disable で実 OpenGL avatar 描画影響なし (= placeholder ゆえ視覚 no-op 等価維持)
6. **build + TUT 11+10+13 + codegen 130/130 全 PASS** = §2 確認
7. **commit 内容** = 3 modified (= llglslshader.cpp + llvkloader.cpp + llvkloader.h) + 2 new doc (= design-lock + complete handoff) + 新 file 0 + CMake 改変 0 + settings.xml 改変 0 + Co-Authored-By 不在 + diff stat = +616/-101
8. **feedback_no_scope_shrink 遵守** = PC-7δ literal scope 5 件 (= vkCmdBindDescriptorSets 通電 + set=3 swap + sAYAStandardLayout 経由 bind + V3a 5-set 構成完成 + SINGLETON 本格化) 全件実施、avatar bone storage 経路 disable は H10-A AYA 確認済の scope 内段階措置で縮小ではない
9. **feedback_design_phase_no_code_write 整合** = 本 PC-7δ は実装 phase (= 別 session で design-lock 完了済 commit 4eb193601d 上に実装、AYA「実装は別 session で行ったほうが安全では？」literal 賛成判断による分離)、indra/ 改変 3 件 = 設計 phase ではない

---

## §7. 引き継ぎ memory 14 件

(既存 memory pointer 列挙、変更なし)

1. project_ayastorm_r41_vulkan_migration
2. project_r41_phase1b_vulkan_host_gate (GATE-B 確定)
3. project_ayastorm_r41_design_principles (2 大原則)
4. feedback_ubo_migration_one_at_a_time (1 つずつ実施)
5. feedback_proactive_handoff (引継 marker 起案)
6. feedback_handoff_minimal_pre_req_read (必読 3 件)
7. feedback_self_verify_before_handoff (9 観点)
8. feedback_build_only_verified (literal 検証取得)
9. feedback_no_scope_shrink (literal scope 完遂)
10. feedback_doubt_self_first (ambiguity 停止 + 確認)
11. feedback_confirm_referent_before_acting (batch AYA 確認)
12. feedback_design_phase_no_code_write (実装 phase 整合)
13. feedback_release_branch_workflow (feature branch 上 commit)
14. feedback_no_auto_commit (AYA 明示指示後 commit) + feedback_no_claude_coauthor

---

## §8. 次 session 着手 1 line

**PC-7ε** = dynamic offset 経路 ring buffer chunk hand-off = set=2 per-draw bind を sDrawUboRingBufferMgr 経由 dynamic offset 計算 + bindV3aStatic の pDynamicOffsets[4] zero placeholder を実値配線 (= 本 PC-7δ で bindV3aStatic は dynamic offset 4 件 zero 埋め完了済、PC-7ε で実値計算経路通電)、source doc = 07 §9.3 + 12、Exit Criteria は次 session 着手前整理。**注: PC-7α' (codegen V1' set=1a/1b split) を PC-7ε 前に着手する選択もあり、AYA さん判断待ち**。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = PC-7ε 引継 marker 本 handoff
- **feedback_handoff_minimal_pre_req_read** 遵守 = 必読 3 件 + pinpoint reference 別記
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅
- **feedback_build_only_verified** 遵守 = llrender build + TUT 11/11 + 10/10 + 13/13 + codegen 130/130 で literal 検証取得
- **feedback_no_scope_shrink** 遵守 = literal scope 5 件全件実施、avatar bone storage 経路 disable は H10-A scope 内段階措置
- **feedback_doubt_self_first** 遵守 = (H1)..(H10) 9+1 件 ambiguity design-lock phase で発見 + 推奨案提示 + AYA 確認 + literal「OK」受領、本実装 phase は確認後実施
- **feedback_confirm_referent_before_acting** 遵守 = design-lock phase で 9 件 batch AYA 確認 + (H10) 単発確認、推測実装なし
- **feedback_ubo_migration_one_at_a_time** 遵守 = PC-7δ = vkCmdBindDescriptorSets 通電 + set=3 swap + V3a 5-set 完成 + SINGLETON 本格化 単一 sub-step (= bind 経路 1 単位)、dynamic offset (PC-7ε) + codegen V1' (PC-7α') は分離
- **feedback_design_phase_no_code_write** 整合 = 本 PC-7δ は実装 phase (= design-lock phase 別 session で完了済 commit 4eb193601d 上で実施)、indra/ 改変 3 件
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = 「commit してください」literal 受領まで commit せず
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
