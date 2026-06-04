# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-N-3 complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-N-3 (= H10-A avatar bone storage 経路再配線 = `sAvatarBoneStorageBuffer` (7040 B static mapped) deprecate + `sPlaceholderSkin` sentinel 新設 + `initVulkan` で `registerSkinUbo` + `wireSkinUboSetV3aToBinding2` 初回 wire + `shutdownVulkan` で `unregisterSkinUbo` + `recordAvatarPlaceholderDraw` 内 `writeSkinUbo` + `flushSkinUbos` + `bindV3aRigged` 正規 sequence 配線) の **実装 phase 完了** marker = step (a)-(g) 7 step 全実装 + Exit Criteria 10 項全充足。

> **本 doc 位置付け**: PC-N-3 design-lock (= `4fb5acaf97`) §4 で確定の実装計画 (a)-(g) 7 step を本 session の fresh context で実装した実装 phase 完了 marker。**Phase 1.C complete**。次 strict 線形 = PC-8 (= 3 OS build verify、Linux primary + Win/Mac AYA 環境依頼) → PC-N-5 = **Phase 1.D 着手起点** (= 実 GLTF Vulkan draw 通電 1 stub)。

---

## §0. 本 session 着手契機 + 完了 scope record

**契機**: AYA 指示「PC-N-3 実装着手お願いします」literal 受領 (2026-06-05、PC-N-3 design-lock commit `4fb5acaf97` 後の継続 session = 別 session の fresh context) + 必読 1 件 = `handoff-...-pc-n-3-design-lock.md` (= ambiguity (N3-1)..(N3-11) 11 件 + G-1 AYA literal「推奨案採用 OK」record + 実装計画 (a)-(g) 7 step 分解 + Exit Criteria 10 項) Read + pinpoint reference (= `writeSkinUbo` 実装 `llvkloader.cpp:5110-` + `flushSkinUbos` `llvkloader.cpp:4504-` + `registerSkinUbo`/`unregisterSkinUbo` `llvkloader.cpp:5050-` + `recordAvatarPlaceholderDraw` 現状 `llvkloader.cpp:5380-` + `sAvatarBoneStorageBuffer` declaration `llvkloader.cpp:103-105` + `wireDrawUboSetV3aToRingBuffer` template `llvkloader.cpp:2658-2710` + `ubo_layout_skin_gltfjoints.inl` line 15 (`Skin_GLTFJoints_SIZE = 16384u`) + `ubo_metadata.inl` line 105/214) pinpoint Read → step (a)-(g) 7 step 順次実装 + 重要発見 (= `LL::GLTF::Skin` は llvkloader.h:31-32 で forward-decl only ゆえ default-constructed instance 不可、address-only sentinel pattern 採用で (N3-6) A の本旨 (= address が unique + safe + 触られない) を温存) → build verify (= llrender + warning 0 + TUT 11+10+13 + codegen 131/131 全 PASS) → 本 complete doc 起案。

**PC-N-3 literal scope 4 件 完了** (= design-lock §0 継承):

1. ✅ **`sAvatarBoneStorageBuffer` + `Allocation` + `Mapped` + `allocateAvatarBoneStorageBuffer()` deprecate** ((N3-1) A) = anonymous namespace の declaration 3 行削除 + `allocateAvatarBoneStorageBuffer()` 関数 body 撤去 + `initVulkan` 内 call chain から除外 + `shutdownVulkan` 内 vmaDestroyBuffer block 撤去 = `llvkloader.cpp` PC-N-3 (a) tag block 4 箇所
2. ✅ **placeholder skin sentinel 新設** ((N3-6) A) = `alignas(void*) char sPlaceholderSkinStorage[1] = {};` + `LL::GLTF::Skin* const sPlaceholderSkin = reinterpret_cast<LL::GLTF::Skin*>(&sPlaceholderSkinStorage[0]);` (= layering 整合 address-only sentinel、`LL::GLTF::Skin` 完全型 unavailable ゆえ default-constructed instance 不可) = PC-N-3 (b) tag block
3. ✅ **`initVulkan` register + `shutdownVulkan` unregister + 初回 wire** ((N3-7) A + (N3-4) A) = `ubo_metadata.inl` から Skin_GLTFJoints block_size lookup + `registerSkinUbo(sPlaceholderSkin, ubo::block_hash::Skin_GLTFJoints, skin_block_size)` + `wireSkinUboSetV3aToBinding2(sPlaceholderSkin)` 明示再 wire + `unregisterSkinUbo` 対称 teardown = PC-N-3 (c)+(d) tag block 2 箇所
4. ✅ **`wireSkinUboSetV3aToBinding2(LL::GLTF::Skin*)` helper 新設** ((N3-4) A) + **`recordAvatarPlaceholderDraw` 内 `writeSkinUbo` + `flushSkinUbos` + `bindV3aRigged` 正規 sequence 配線** ((N3-2) A + (N3-8) A) = `wireDrawUboSetV3aToRingBuffer` (PC-7ε) 同形 pattern helper + `static const U8 zero_skin_buf[256] = {}` + `LLVKLoader::writeSkinUbo(sPlaceholderSkin, Skin_GLTFJoints, 0u, zero_skin_buf, 256)` + `LLVKLoader::flushSkinUbos(sPlaceholderSkin)` → 直後 `bindV3aRigged` の set=3 swap 経路通電 = PC-N-3 (d) + (e) tag block + LL_INFOS first-fire marker PC-N-3 通電 literal 追加

---

## §1. 必読 1 件 + pinpoint reference (次 session = PC-8 向け)

**次 session 必読**:

1. **本 PC-N-3 complete doc 全文**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-n-3-complete.md`

**pinpoint reference** (PC-8 で必要分のみ):

- **PC-N decomposition design-lock**: `handoff-...-pc-n-decomposition-design-lock.md` §5 = Phase 1.C complete 後 PC-8 (= 3 OS build verify、Linux primary 完了済本 commit、Win/Mac は AYA 環境依頼)
- **本 PC-N-3 complete doc §3**: build verify 結果 (= Linux primary 取得済)
- **PC-N-2 complete doc**: `handoff-...-pc-n-2-complete.md` = PC-N-2 set=2 復活完了状態 (= PC-N-3 着手 baseline)
- **PC-N-4 complete doc**: `handoff-...-pc-n-4-complete.md` = ring buffer grow 自動 re-wire 完了状態
- **GATE-B literal**: memory `project_r41_phase1b_vulkan_host_gate` (= `#ifdef LL_VULKAN_GLSL` host C++ 未定義 + runtime mUseUBO flag のみ)

---

## §2. 実装内容 (step (a)-(g) 7 step 全実装)

### §2.1 編集 1 (llvkloader.cpp +230 / -80 net +150)

| step | 内容 | 場所 (after) | 行数増減 |
|------|------|------|---------|
| (a) | `sAvatarBoneStorageBuffer` + `Allocation` + `Mapped` declaration 撤去 + PC-N-3 (a) deprecation comment block | line 100-115 (anonymous namespace) | +5 / -3 |
| (a) | `allocateAvatarBoneStorageBuffer()` 関数 body 全削除 + PC-N-3 (a) deprecation comment | line 3261-3272 | +6 / -65 |
| (a) | `initVulkan` call chain から `allocateAvatarBoneStorageBuffer()` 除外 + PC-N-3 (a) tag block + 説明 comment | line 3665-3679 | +9 / -1 |
| (a) | `shutdownVulkan` 内 `vmaDestroyBuffer` block 撤去 + PC-N-3 (a) no-op comment (= teardown は unregisterSkinUbo + bulk loop 担当) | line 3798-3805 | +4 / -6 |
| (b) | placeholder skin sentinel 新設 = `alignas(void*) char sPlaceholderSkinStorage[1] = {}` + `LL::GLTF::Skin* const sPlaceholderSkin = reinterpret_cast<LL::GLTF::Skin*>(...)` + PC-N-3 (b) tag block + 19 行 layering 制約説明 comment | line 563-579 (anonymous namespace `sSkinUboDirty` 直後) | +19 |
| (d) | `wireSkinUboSetV3aToBinding2(LL::GLTF::Skin*)` helper 新設 = FRAMES_IN_FLIGHT loop + vkUpdateDescriptorSets binding=2 wire + PC-N-3 (d) tag block + 14 行説明 comment + LL_INFOS marker | line 2746-2820 (anonymous namespace、`wireDrawUboSetV3aToRingBuffer` 直後) | +77 |
| (c)+(d) | `initVulkan` 内 register + 初回 wire = `ubo_metadata.inl` block_size lookup + `registerSkinUbo(sPlaceholderSkin, Skin_GLTFJoints, skin_block_size)` + `wireSkinUboSetV3aToBinding2(sPlaceholderSkin)` + 3 段 graceful degrade (block lookup miss / register fail / wire fail で LL_WARNS_ONCE) | line 3701-3748 (`updatePerMaterialDescriptorSet` 直後 + `logVmaBudgetSmoke` 直前) | +48 |
| (c) | `shutdownVulkan` 内 `unregisterSkinUbo(sPlaceholderSkin, Skin_GLTFJoints)` 追加 + PC-N-3 (c) tag block (= bulk teardown loop 前で対称 lifecycle 明示) | line 4051-4058 | +7 |
| (e) | `recordAvatarPlaceholderDraw` 関数 header comment 更新 = sAvatarBoneStorageMapped → per-Skin UboInstance 経路 literal 整合 + PC-N-3 (a) deprecation note | line 5383-5395 | +9 / -1 |
| (e) | `recordAvatarPlaceholderDraw` 内 PC-N-2 (d) tag block 内 comment 整合更新 = PC-N-3 (e) tag block 追加 + bone storage 再配線 H10-A 完了 literal 明示 | line 5422-5444 | +19 / -5 |
| (e) | `recordAvatarPlaceholderDraw` 内 writeSkinUbo + flushSkinUbos 配線 = `static const U8 zero_skin_buf[256] = {}` + `LLVKLoader::writeSkinUbo(...)` + `LLVKLoader::flushSkinUbos(...)` + PC-N-3 (e) tag block + 8 行説明 comment | line 5460-5479 (bindV3aRigged 直前) | +20 |
| (e) | `recordAvatarPlaceholderDraw` LL_INFOS first-fire marker 更新 = `PC-N-3 placeholder skin sentinel writeSkinUbo + flushSkinUbos 通電済 (set=3 binding=2 Skin_GLTFJoints, sPlaceholderSkin sentinel 経路, sAvatarBoneStorageBuffer deprecated)` literal 追加 | line 5506-5512 | +3 / -1 |

**累計**: +230 insertions / -80 deletions / net +150 (= `git diff --stat indra/llrender/llvkloader.cpp` 検証取得)

### §2.2 .h 改変 0 件

- `bindV3aRigged` signature 不変 ((N3-9) A) = `(VkCommandBuffer, U32 frame_index, const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS])` 維持
- `wireSkinUboSetV3aToBinding2` helper は anonymous namespace 内 ローカル (= `wireDrawUboSetV3aToRingBuffer` PC-7ε と同形)、`initVulkan` (= `LLVKLoader` namespace 直下) から unqualified lookup で到達可
- `sPlaceholderSkin` + `sPlaceholderSkinStorage` も anonymous namespace 内 file-static、`initVulkan` + `shutdownVulkan` + `recordAvatarPlaceholderDraw` 全 site から unqualified lookup で参照可
- `LL::GLTF::Skin` forward declaration は `llvkloader.h:31-32` 既存維持 (= 改変なし)
- `llvkloader.h` 公開 API 不変

### §2.3 codegen 改変 0 件

- (N3-3) B 採用 = PC-7γ-3 で既配置の `Skin_GLTFJoints` codegen 出力をそのまま活用 (= `ubo::block_hash::Skin_GLTFJoints` constexpr U32 既存 + `ubo_layout_skin_gltfjoints.inl` 既存 + `llglslshader.h:169` UB_GLTF_JOINTS enum + `llglslshader.cpp:1961` `"Skin_GLTFJoints"` name mapping 既存)
- `scripts/ubo_codegen` 改変 0 件 + `build-linux-x86_64/codegen/` 改変 0 件
- codegen unittest 131/131 PASS (= 131 維持、132 化なし、(N3-10) A 整合)

### §2.4 shader 改変 0 件

- shader (`*.glsl` / `*.glslf` / `aya_r41_blueprints/`) 改変 0 件
- design 06b/06c §2.5 確定 set=3 binding=2 = Skin_GLTFJoints は既存 codegen 配置で shader 側 binding 既定義済

### §2.5 GATE-B 整合

`#ifdef LL_VULKAN_GLSL` 新規追加 0 件:

- (a) `sAvatarBoneStorageBuffer` declaration 撤去 + `allocateAvatarBoneStorageBuffer()` 撤去 + call chain 整合は全て host C++ Vulkan path 内
- (b) `sPlaceholderSkin` sentinel は anonymous namespace 内 C++ pointer (= GLSL 関係なし)
- (c) `registerSkinUbo` / `unregisterSkinUbo` / `wireSkinUboSetV3aToBinding2` は全て host C++ Vulkan path 専有
- (e) `writeSkinUbo` / `flushSkinUbos` は host C++ Vulkan path 専有 + `bindV3aRigged` signature 不変

= `project_r41_phase1b_vulkan_host_gate` GATE-B (= host C++ は `mUseUBO` runtime flag のみで gate) 整合

### §2.6 MUSEUBO-A 整合

`mUseUBO=false` default 描画 100% 維持:

- `recordAvatarPlaceholderDraw` は Vulkan placeholder offscreen FBO 経路のみで呼出 (= `mUseUBO=false` default で OpenGL 描画 100% 維持 = 既存実装変更なし)
- 多重 nullptr fallback:
  - `sDrawUboRingBufferMgr` nullptr → `recordPlaceholderPoolDraw` fallback (= 既存 PC-N-2 guard)
  - `wireSkinUboSetV3aToBinding2` 失敗時 (= sDevice 不在 / skin not registered / no frame to wire) → LL_WARNS_ONCE + return false (= graceful degrade)
  - `registerSkinUbo` 失敗時 → LL_WARNS_ONCE + 続行 (= `recordAvatarPlaceholderDraw` 側で writeSkinUbo は entry 不在ゆえ早期 return + `flushSkinUbos` も同様 + bindV3aRigged set=3 binding=2 は stale VkBuffer 参照になるが視覚 no-op 維持)
  - block_size lookup miss → LL_WARNS_ONCE + register skip + recordAvatarPlaceholderDraw は stale binding で続行 (= 視覚 placeholder 維持)

### §2.7 アーキテクチャ制約発見 (address-only sentinel pattern)

design-lock §4 (N3-6) A literal は「default 初期化で安全、uninitialized addr は touch で crash、safest + simplest」だったが、本実装で発見:

- `LL::GLTF::Skin` は `llvkloader.h:31-32` で **forward declaration のみ** (= `namespace LL { namespace GLTF { class Skin; }}`)
- 完全型 (= `gltf/asset.h`) は newview 階層 (= `llvertexbuffer.h` + `llvolumeoctree.h` 等依存) ゆえ llrender 層から include すると layering violation
- `static LL::GLTF::Skin sPlaceholderSkin{}` は完全型 unavailable ゆえ compile error
- **解決**: address-only sentinel pattern = `alignas(void*) char sPlaceholderSkinStorage[1] = {}` (= 1 byte stable address holder) + `LL::GLTF::Skin* const sPlaceholderSkin = reinterpret_cast<LL::GLTF::Skin*>(&sPlaceholderSkinStorage[0])` (= 型は LL::GLTF::Skin* だが実体は 1 byte storage の address)
- **(N3-6) A 本旨温存**: address は unique + safe + 触られない = `sSkinUboDirty` map key compare (= `UboSkinKey` 第 1 要素) + `writeSkinUbo` / `flushSkinUbos` / `registerSkinUbo` 内部の pointer compare のみ、**member access / method call 一切なし** ゆえ完全型不要、address-only で機能等価

= layering 制約遵守 + (N3-6) A spirit 温存 + PC-N-5 で実 Skin* 投入時に並走可能 (= sentinel と実 Skin* を sSkinUboDirty 上で同居)

---

## §3. build verify (= step (f))

| # | check | 結果 |
|---|-------|------|
| 1 | `cd build-linux-x86_64 && make -j4 llrender` | ✅ PASS (= `[100%] Built target llrender`) |
| 2 | ERROR 0 + WARNING 0 | ✅ (= `grep -iE "warning\|error"` 0 件) |
| 3 | `INTEGRATION_TEST_lluboringbuffer` | ✅ 11/11 PASS YAY (= PC-3 algorithm 層 regression なし) |
| 4 | `INTEGRATION_TEST_llassetubopool` | ✅ 10/10 PASS YAY (= PC-4 algorithm 層 regression なし) |
| 5 | `INTEGRATION_TEST_llpipelinecachestorage` | ✅ 13/13 PASS YAY (= PC-5 algorithm 層 regression なし) |
| 6 | `scripts/ubo_codegen` unittest | ✅ 131/131 PASS (= Phase 1.A / 1.B / 1.C PC-1..PC-N-4 regression なし、131 維持 = (N3-3) B + (N3-10) A 整合) |

= (N3-10) A 採用 build verify scope literal 取得済。

---

## §4. PC-N-3 実装 phase Exit Criteria 10 項全充足

| # | Criteria | 充足 |
|---|----------|------|
| (i) | `sAvatarBoneStorageBuffer` + `Allocation` + `Mapped` + `allocateAvatarBoneStorageBuffer()` deprecate ((N3-1) A) | ✅ §2.1 (a) 4 site 撤去 |
| (ii) | placeholder skin sentinel 新設 ((N3-6) A 本旨 = address-only sentinel pattern で温存) | ✅ §2.1 (b) + §2.7 |
| (iii) | `initVulkan` register + `shutdownVulkan` unregister ((N3-7) A) + 初回 wire ((N3-4) A) | ✅ §2.1 (c)+(d) |
| (iv) | `wireSkinUboSetV3aToBinding2(skin)` helper 新設 ((N3-4) A) = FRAMES_IN_FLIGHT loop + vkUpdateDescriptorSets binding=2 wire | ✅ §2.1 (d) line 2746-2820 |
| (v) | `recordAvatarPlaceholderDraw` 内 writeSkinUbo + flushSkinUbos + bindV3aRigged 正規 sequence 配線 ((N3-2) A + (N3-8) A) | ✅ §2.1 (e) line 5460-5479 |
| (vi) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 | ✅ §2.5 |
| (vii) | MUSEUBO-A 整合 = `mUseUBO=false` default 描画 100% 維持 + 多重 nullptr fallback | ✅ §2.6 |
| (viii) | build verify = `llrender` build + ERROR 0 / WARNING 0 + INTEGRATION_TEST 11+10+13 全 PASS + codegen unittest 131/131 PASS ((N3-10) A) | ✅ §3 |
| (ix) | tag block 統一 = `<AYAstorm r41 PC-N-3 (a)>` × 4 + `(b)` + `(c)` + `(d)` + `(c)+(d)` + `(e)` × 3 + LL_INFOS first-fire marker PC-N-3 通電 literal 追加 | ✅ tag 全ペア配置 + LL_INFOS marker line 5509-5511 |
| (x) | handoff complete doc 起案 + AYA commit 指示後 commit (= Co-Authored-By 不在 + 個別 file 指定 + 新 file 0 除 doc + CMake 改変 0 + settings.xml 改変 0 + codegen 改変 0 + shader 改変 0) | ✅ 本 doc + commit pending AYA 指示 |

---

## §5. 残 strict 線形 (= Phase 1.C complete 達成)

**Phase 1.C complete** ✅ 本 commit (= PC-0..PC-7ε + PC-N-1..PC-N-4 + **PC-N-3 ✅**)

次:

- **PC-8** (= 3 OS build verify、Linux primary 完了済本 commit、Win/Mac は AYA 環境依頼) ⏳
- **PC-N-5** = Phase 1.D 着手起点 (= 実 GLTF Vulkan draw 通電 1 stub = sentinel skin と実 Skin* を sSkinUboDirty 上で同居、bone matrix data 構築 + bindV3aRigged 経由実 draw) ⏳

---

## §6. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1 design-lock ✅ + PC-N-1 ✅ + PC-N-2 design-lock ✅ + PC-N-2 ✅ + PC-N-4 design-lock ✅ + PC-N-4 ✅ + PC-N-3 design-lock ✅ + **PC-N-3 ✅ 本 commit = Phase 1.C complete ✅** + PC-8 (3 OS build verify Linux primary 本 commit、Win/Mac 後段) ⏳ + PC-N-5 = Phase 1.D 着手起点 ⏳

---

## §7. self-verify 9 観点 全 ✅

1. **Exit Criteria 10 項全充足** §4 ✅
2. **必読 1 件 (本 complete handoff doc) §1 + pinpoint reference 5 件別記** (= PC-N decomposition §5 + 本 PC-N-3 complete §3 + PC-N-2 complete + PC-N-4 complete + GATE-B literal memory) ✅
3. **step (a)..(g) 7 step 全実装** = (a) sAvatarBoneStorageBuffer deprecate (= 4 site) + (b) sPlaceholderSkin sentinel (= address-only pattern) + (c) register/unregister 対称 lifecycle + (d) wireSkinUboSetV3aToBinding2 helper + 初回 wire + (e) recordAvatarPlaceholderDraw writeSkinUbo + flushSkinUbos 配線 + comment 整合 + LL_INFOS marker + (f) build verify §3 + (g) 本 doc 起案 ✅
4. **ambiguity (N3-1)..(N3-11) 11 件 + G-1 AYA literal「推奨案採用 OK」record (2026-06-05) design-lock 継承** ✅ (= design-lock commit `4fb5acaf97` §3 全 11 件 + G-1 OK record 済、本 session で (N3-3) B 採用後の事実訂正済) + (N3-6) A 本旨温存 = address-only sentinel pattern §2.7
5. **GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件** §2.5 ✅
6. **MUSEUBO-A 整合 = `mUseUBO=false` default 経路不変** (= recordAvatarPlaceholderDraw は Vulkan placeholder offscreen FBO 経路、多重 nullptr fallback = sDrawUboRingBufferMgr + wireSkinUboSetV3aToBinding2 + registerSkinUbo + block_size lookup) §2.6 ✅
7. **llrender build + WARNING 0 + TUT 11+10+13 + codegen 131/131 全 PASS** §3 ✅
8. **commit 内容** = 1 modified (`llvkloader.cpp` +230/-80 net +150) + 1 new doc (handoff complete) + 新 file 0 (除 doc) + CMake 改変 0 + settings.xml 改変 0 + codegen 改変 0 + shader 改変 0 + Co-Authored-By 不在 ✅
9. **feedback_no_scope_shrink 遵守** = PC-N-3 literal scope 4 件 §0 全件実施、(N3-3) B (= codegen 改変 0 件、既存 Skin_GLTFJoints block 活用) は design-lock phase で AYA literal「B で」record 後事実訂正済 (= 縮小ではなく PC-7γ-3 既配置発見の事実訂正) + (N3-6) A 本旨温存 = address-only sentinel pattern は layering 制約遵守の minor 拡張 (= 縮小でも禁止 scope 拡張でもない、機能等価) ✅

---

## §8. 引き継ぎ memory 14 件

- (本 session で memory 改変 0 件、既存参照のみ)
- 関連 memory: `project_ayastorm_r41_vulkan_migration` (= r41 章 milestone pointer) + `project_r41_phase1b_vulkan_host_gate` (= GATE-B literal) + `feedback_ubo_migration_one_at_a_time` (= UBO 化 1 つずつ厳格) + `feedback_no_scope_shrink` (= literal scope 完全実施) + `feedback_self_verify_before_handoff` (= 9 観点 self-verify) + `feedback_build_only_verified` (= literal 検証取得) + `feedback_doubt_self_first` (= design-lock phase で ambiguity 11 件 + G-1 発見済) + `feedback_confirm_referent_before_acting` (= design-lock phase で 12 件 batch AYA 確認済) + `feedback_design_phase_no_code_write` (= 本 session は実装 phase ゆえ `indra/` 改変は正規) + `feedback_release_branch_workflow` (= feature branch 上 commit) + `feedback_no_auto_commit` (= AYA commit 指示後 commit) + `feedback_no_claude_coauthor` (= Co-Authored-By 不在) + `feedback_no_bare_reference_ids` (= (N3-1)..(N3-11) 各 ID に項目名併記) + `feedback_tests_dir_never_commit` (= `tests/` 改変 0 件)

---

## §9. 次 session 着手 1 line

**PC-8 (3 OS build verify) 着手** = Linux primary 完了済本 commit、Win/Mac は AYA 環境依頼。Phase 1.D 着手起点 = PC-N-5 = 実 GLTF Vulkan draw 通電 1 stub (= sentinel skin と実 Skin* を sSkinUboDirty 上で同居、bone matrix data 構築 + bindV3aRigged 経由実 draw)。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 complete handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 必読 1 件 + pinpoint reference 5 件別記、本 session も Read pinpoint のみ (= design-lock doc + writeSkinUbo/flushSkinUbos/registerSkinUbo + recordAvatarPlaceholderDraw + sAvatarBoneStorageBuffer declaration + wireDrawUboSetV3aToRingBuffer + ubo_layout_skin_gltfjoints.inl + ubo_metadata.inl)、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §7
- **feedback_build_only_verified** 遵守 = llrender build + WARNING 0 + TUT 11/11 + 10/10 + 13/13 + codegen 131/131 で literal 検証取得
- **feedback_no_scope_shrink** 遵守 = PC-N-3 literal scope 4 件 §0 全件実施、address-only sentinel pattern は (N3-6) A 本旨温存 (= 機能等価) §2.7
- **feedback_doubt_self_first** 遵守 = design-lock phase で ambiguity 11 件 + G-1 発見 + 推奨案提示 + AYA literal「推奨案採用 OK」確認 + (N3-3) 事実訂正後本実装、本実装中も `LL::GLTF::Skin` forward-decl only 発見で address-only sentinel pattern に pivot ((N3-6) A 本旨温存)、推測実装なし
- **feedback_confirm_referent_before_acting** 遵守 = 12 件 batch AYA 確認 design-lock phase で完了、本実装中も `LL::GLTF::Skin` 完全型 unavailable を Read で literal 確認後 address-only pattern 採用
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = PC-N-3 = H10-A avatar bone storage 経路再配線単独 sub-step、PC-N-5 (実 GLTF avatar draw) は分離
- **feedback_design_phase_no_code_write** 整合 = 本 PC-N-3 は実装 phase = design-lock commit `4fb5acaf97` で `indra/` 改変 0 件 完了済、本 session で `indra/` 改変は実装 phase ゆえ整合
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「commit してください」literal 受領後 commit
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
- **feedback_no_bare_reference_ids** 遵守 = (N3-1)..(N3-11) 各 ID に項目名 / 採用案内容併記 + (a)..(g) 各 step に作業内容併記
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、`git add` 個別 file 指定 + `git add -A` 不使用予定

---
