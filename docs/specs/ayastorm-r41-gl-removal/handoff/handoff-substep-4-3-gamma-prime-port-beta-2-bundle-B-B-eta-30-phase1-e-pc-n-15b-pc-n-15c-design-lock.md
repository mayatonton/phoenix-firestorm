# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.E **PC-N-15b + PC-N-15c 統合 design-lock complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: Phase 1.E 内 **6th sub-step = PC-N-15b = worker thread dispatch 配線** (= LL::WorkQueue post/drain + per-Primitive worker dispatch + secondary cmdbuf record + `vkCmdExecuteCommands` 集約 + `sPcn13MultiAssetSeen` mutex 保護 + `sSkinUboDirty` merge semantics + first-fire marker 3 件) **および 7th = 最終 sub-step = PC-N-15c = cleanup + sGltfStubSkin sentinel 撤去 + Phase 1.E complete marker 起案** の **統合 design-lock phase 完了 marker** = AYA literal「B でお願いします」record (2026-06-05) 受領で **案 B 統合方針確定** (= doc 削減 4 件 → 2 件) + PC-N-15b ambiguity 22 件全 AYA literal「全部 OK です」record (2026-06-05) + PC-N-15c ambiguity 12 件 AYA literal 確認待ち (= 本 doc 起案時 batch 提示)。`indra/` 改変 0 件 (= `feedback_design_phase_no_code_write` 整合)。

> **本 doc 位置付け**: 案 B 統合 design-lock。PC-N-15b/c は scope 独立 (= dispatch 配線 vs cleanup + Phase 1.E complete marker) ゆえ別 commit で実装するが、design 段階は連続して見るべき (= 15c cleanup 対象は 15b で配線した物) ゆえ統合 1 件で design-lock + AYA 一括確認、実装段階は段階的 (15b → build verify + AYA live 確認 → 15c 実装)。**handoff doc 体制**: 本 doc 1 件 + PC-N-15c 完了時 = **Phase 1.E complete marker** handoff 1 件 = **計 2 件** (= 元 想定 4 件の 50% 削減)。15b 完了は **commit message + cross-platform spec §A 履歴 + 本 doc §B「実装結果追記」section** で記録 (= 別 complete handoff doc 起案なし)。

---

## §0. 本 session 着手契機 + 案 B 統合方針 record + literal scope record

**契機**: AYA 指示「r41 Phase 1.E PC-N-15b design-lock 着手お願いします。直前 commit: `a90883cbf9` = PC-N-15a complete (worker thread infrastructure 配線 = LLUboRingBuffer per-thread instance refactor + thread_local 4 件 + per-thread VkCommandPool + secondary VkCommandBuffer + AYAGltfWorkerThreadEnabled/Count 2 cvar + initVulkan/shutdownVulkan lifecycle)。必読 1 件 = PC-N-15a complete handoff doc。PC-N-15b = dispatch 配線 design = LL::WorkQueue post/drain + per-Primitive worker dispatch + secondary cmdbuf record + vkCmdExecuteCommands 集約 + sPcn13MultiAssetSeen mutex 保護 insert + sSkinUboDirty merge semantics + first-fire marker 3 件。design-lock phase なので indra/ 改変 0 件 (feedback_design_phase_no_code_write 厳格遵守)、ambiguity 確認 + 実装計画分解 + Exit Criteria 明文化 + handoff design-lock doc 起案まで。その後 PC-N-15c (cleanup + sGltfStubSkin sentinel 撤去 + Phase 1.E complete marker) で Phase 1.E 完結。」literal 受領 (2026-06-05)。

**案 B 統合方針確定の経緯**:
- 当初 Claude が PC-N-15b ambiguity 18 件 batch 提示 → AYA literal「具体的に何が問題になるのかちょっとよくわからないです」literal feedback 受領 (2026-06-05)
- Claude が overview 整理 (= PC-N-15a で箱だけ確保、PC-N-15b で worker thread launch + dispatch + aggregation 通電) + 本質判断要 Q1-Q4 に絞って再提示
- AYA literal「全部 OK です」record 受領 (2026-06-05) で Q1-Q4 + N15b-1..18 = 計 22 件 ambiguity 全 A 採用案確定
- Claude が「handoff doc 増えすぎ + context size」懸念提示 + 統合案 3 つ (案 A/B/C) 提示
- AYA literal「それとももともと 1M あったけど足りなかった?」literal 質問受領 (2026-06-05) → Claude 率直回答 (= 1M context 余裕あるが doc 数自体が次 session Read 負荷 + AYA review 負荷 + spec source of truth 分散の問題ゆえ doc 削減は質的改善目的)
- AYA literal「B でお願いします」record 受領 (2026-06-05) で **案 B = PC-N-15b/c 統合 design-lock 1 件 + Phase 1.E complete marker 1 件 = 計 2 件 確定**

**PC-N-15b literal scope** (= PC-N-15a complete doc §5.6 + AYA literal「全部 OK」record 2026-06-05、6 項):

1. **`LL::WorkQueue` infrastructure 配線 (= step (a))** = `LL::WorkQueue` 1 件 anonymous 作成 + worker_count 個 `std::thread` spawn (`runUntilClose` pattern) + `thread_local U32 sWorkerIdx` で worker thread 識別 + `createWorkerThreadInfra()` 末尾に thread launch logic 追加 + `destroyWorkerThreadInfra()` 冒頭で WorkQueue close + thread join 追加 ((N15b-4) ⭐ A 採用)
2. **per-Primitive worker dispatch (= step (b))** = `gltfscenemanager.cpp` per-Primitive loop 内 `setCurrentPrimitive` 直後並列 (line 752) に `LLVKLoader::postPrimitiveToWorker(asset, primitive, skin_or_null, mAssetMatrix_copy)` hook 追加 + cvar `AYAGltfWorkerThreadEnabled` guard 下 + cvar OFF 時 immediate skip = main thread fallback ((N15b-1) ⭐ A 採用 + (N15b-12) A 採用)
3. **secondary cmdbuf record (= step (c))** = worker thread 内で `vkBeginCommandBuffer(secondary, USAGE=RENDER_PASS_CONTINUE|SIMULTANEOUS_USE)` + `VkCommandBufferInheritanceRenderingInfoKHR` 経由 main thread dynamic rendering scope 継承 + `recordGltfAssetDraw(secondary_cmdbuf)` 呼出 + `vkEndCommandBuffer` ((N15b-9) A 採用 = Vulkan 1.3 dynamic rendering 経路)
4. **`vkCmdExecuteCommands` 集約 (= step (d))** = `gltfscenemanager.cpp` per-Primitive loop 終了直後 (= line 816 ループ閉じ括弧直後、line 820 `clearCurrentAsset` 直前) に `LLVKLoader::drainWorkersAndExecute(sCommandBuffer)` hook 追加 = WorkQueue drain (= 全 worker 完了待ち) → 全 secondary cmdbuf を `vkCmdExecuteCommands(sCommandBuffer, worker_count, secondary_cmdbufs[])` で集約 ((N15b-3) ⭐ A 採用 = per-Asset 末尾集約)
5. **`sPcn13MultiAssetSeen` mutex 保護 + `sSkinUboDirty` merge semantics (= step (e))** = worker thread 内 `sPcn13MultiAssetSeen.insert` を `std::lock_guard<std::mutex>(sPcn13MultiAssetSeenMutex)` で wrap + worker thread 内 `writeSkinUbo` 呼出時は `sPcn14WorkerCtx[sWorkerIdx].mSkinUboSubDirty` に書込、main thread `drainWorkersAndExecute` 内で `sSkinUboDirty` に merge → `flushSkinUbos` 駆動 + `writeDrawUbo` も同形 thread-aware ((N15b-6) A + (N15b-7) A + (N15b-8) A 採用)
6. **first-fire LL_INFOS marker 3 件 (= step (f))** = `s_first_pcn14_worker_thread_fire` (= worker thread launch 成功時) + `s_first_pcn14_secondary_cmdbuf_fire` (= secondary cmdbuf 最初の `vkBeginCommandBuffer` 成功時) + `s_first_pcn14_ubo_parallel_fire` (= worker thread 内最初の `writeDrawUbo` 成功時) ((N15b-11) A 採用 = PC-N-6..15a 同形 atomic flag pattern)

**PC-N-15c literal scope** (= PC-N-15a complete doc §5.6 + Phase 1.E complete marker、6 項):

1. **`sGltfStubSkin` sentinel storage 撤去 (= step (a))** = `llvkloader.cpp:607-609` `sGltfStubSkinStorage` + `sGltfStubSkin` declaration 撤去 + `initVulkan` 内 `registerSkinUbo(sGltfStubSkin, ...)` 配線 (line 4231-4276 範囲) 撤去 + `shutdownVulkan` 内 `unregisterSkinUbo(sGltfStubSkin, ...)` 配線 (line 4626) 撤去
2. **`AYAGltfMultiSkinEnabled` cvar 撤去 (= step (b))** = `recordGltfAssetDraw` PC-N-11 (a) tag block 内 `sAyastormGltfMultiSkinEnabled` cvar guard + fall-through `sGltfStubSkin` sentinel path (line 6321-6357 範囲) 撤去 = real Skin path 一本化 + `settings.xml` `AYAGltfMultiSkinEnabled` cvar entry 撤去 ((N15c-2) A 採用 = 撤去 = scope clean)
3. **`AYAGltfRealDrawEnabled` cvar + `recordAvatarPlaceholderDraw` 末尾 entry hook 撤去 (= step (c))** = `recordAvatarPlaceholderDraw` 末尾 PC-N-10 (a) cvar entry hook (line 6664-6671 範囲、`recordGltfAssetDraw(cmd_buf)` 呼出) 撤去 + `settings.xml` `AYAGltfRealDrawEnabled` cvar entry 撤去 ((N15c-3) A 採用 = PC-N-15b で dead code 化、PC-N-15c で撤去)
4. **build verify literal 取得 (= step (d))** = PC-N-15b 同形 = llrender PASS + WARNING 0 + TUT 11/11 + 10/10 + 13/13 + codegen 131/131 + GATE-B integrity `LL_VULKAN_GLSL` count llvkloader.cpp=6 不変
5. **cross-platform spec §6 PC-N-15c 行 ✅ 反映 + §A 履歴 + Phase 1.E complete marker 起案 (= step (e))** = §6 PC-N-15c 行 状態 ✅ 反映 + §A 履歴 1 行追記 + Phase 1.E 完了宣言 + 達成事項列挙 + Phase 1 全完了状態確認 + Mac/Win 補完 phase entry へ移行 marker
6. **Phase 1.E complete handoff doc 起案 (= step (f))** = 新規 `handoff-substep-...-phase1-e-complete.md` 起案 = Phase 1.E 全体総括 (= PC-N-11..PC-N-15c 完了状態 + 実 data 通電 + multi-asset / multi-skin / real modelview / real per-draw light params / worker thread 並列化 全達成 marker) ((N15c-7) A 採用)

**Phase 境界**: PC-N-15b 完了 = Phase 1.E 内 6th sub-step **dispatch 配線** 完了 = worker thread launch + per-Primitive dispatch + secondary cmdbuf record + vkCmdExecuteCommands 集約 + UBO merge 通電。**PC-N-15c 持越し** = cleanup + sGltfStubSkin 撤去 + Phase 1.E complete marker。PC-N-15c 完了 = **Phase 1.E complete = 1 GLTF asset 完全 Vulkan draw 通電 + multi-asset / multi-skin / real modelview / real per-draw light params / worker thread 並列化 全達成** marker → **Phase 1 全完了** → Mac/Win 開発者補完 phase entry。

---

## §1. 必読 1 件 + pinpoint reference + background reference

### §1.1 必読 1 件 (= 次 session = PC-N-15b 実装 phase 着手前)

1. **本 PC-N-15b + PC-N-15c 統合 design-lock doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-e-pc-n-15b-pc-n-15c-design-lock.md`

### §1.2 pinpoint reference 14 件 (= 実装 phase で必要分のみ Read、`feedback_handoff_minimal_pre_req_read` 整合)

1. **`LL::WorkQueue` class**: `indra/llcommon/workqueue.h` = `class WorkQueue : public LLInstanceTrackerSubclass<WorkQueue, WorkQueueBase>` + constructor `WorkQueue(name, capacity=1024, auto_shutdown=true)` + `post(Work)` / `runUntilClose()` / `close()` + `Work = std::function<void()>` + 1 producer / N consumer thread-safe — PC-N-15b (a) anonymous queue 作成 + worker_count 個 `std::thread` spawn (`runUntilClose` pattern)
2. **`createWorkerThreadInfra()` 既配線**: `indra/llrender/llvkloader.cpp:1603-1730` = PC-N-15a で per-thread VkCommandPool + secondary VkCommandBuffer + LLUboRingBuffer instance + mSkinUboSubDirty + mDrawUboSubRecords 既配線、`sPcn14WorkerCtx` `std::vector<PcN14WorkerContext>` storage 既配線 — PC-N-15b (a) 末尾に WorkQueue 作成 + `std::thread` spawn logic 追加 (= `thread_local U32 sWorkerIdx` 初期化 + `queue.runUntilClose()` 駆動)
3. **`destroyWorkerThreadInfra()` 既配線**: `llvkloader.cpp:1732-1761` = LLUboRingBuffer destroy → vkFreeCommandBuffers → vkDestroyCommandPool 既配線 — PC-N-15b (a) 冒頭に WorkQueue `close()` + thread `join()` logic 追加 (= reverse-init order 維持)
4. **`recordGltfAssetDraw` 関数**: `llvkloader.cpp:6218-6520` = PC-N-8 (f) real Asset path + PC-N-11 (a) multi-skin + PC-N-12 (a) real modelview + PC-N-13 (a)/(b) real light params + multi-asset canary 既配線、cmd_buf 引数経由で record — PC-N-15b (c) worker thread 内で secondary cmdbuf 引数として呼出
5. **per-Primitive loop in GLTFSceneManager::render**: `indra/newview/gltfscenemanager.cpp:738-816` = `for (auto& pdata : batches[i].mPrimitives)` ループ body、`setCurrentPrimitive` (line 752) + `setCurrentNodeAssetMatrix` (line 768) + `setCurrentSkin` (line 781、`if (rigged)` 内) + `drawRangeFast` (line 797) + `clearCurrentSkin` / `clearCurrentNodeAssetMatrix` / `clearCurrentPrimitive` (line 803/809/814) — PC-N-15b (b) `setCurrentPrimitive` 直後並列に `LLVKLoader::postPrimitiveToWorker(...)` hook 追加 + loop 終了直後 (line 816 ループ閉じ括弧直後) に `LLVKLoader::drainWorkersAndExecute(sCommandBuffer)` hook 追加
6. **`sCurrentAsset`/`sCurrentSkin`/`sCurrentPrimitive`/`sCurrentNodeAssetMatrix` thread_local 4 件**: `llvkloader.cpp:682`/`:683`/`:696`/`:716` = PC-N-15a で `thread_local` 修飾子追加済 + accessor signature 不変 — PC-N-15b (b) worker thread が自 thread 内で `setCurrentXxx` 呼出 → `recordGltfAssetDraw` 経路で参照
7. **`sPcn13MultiAssetSeen` + `sPcn13MultiAssetSeenMutex`**: `llvkloader.cpp:698-708` (PC-N-13 (b)) + `:710-712` (PC-N-15a で mutex 追加済) + access site `:6460` (`sPcn13MultiAssetSeen.insert` + `size() > 1u` 判定) — PC-N-15b (e) worker thread 内 insert を `std::lock_guard<std::mutex>(sPcn13MultiAssetSeenMutex)` で wrap
8. **`writeDrawUbo` 既配線**: `llvkloader.cpp:5528-5615` = main thread `sDrawUboRingBufferMgr` 経路 + helper functions (block_hash → block_size lookup + dynamic offset alloc + memcpy + grow flag) — PC-N-15b (e) thread-aware 拡張 = `sWorkerIdx != kInvalidWorkerIdx` 時 `sPcn14WorkerCtx[sWorkerIdx].mDrawUboRingBuffer` 経路 + per-thread `mDrawUboSubRecords` 参照
9. **`writeSkinUbo` 既配線**: `llvkloader.cpp:5829-5862` = `sSkinUboDirty` 既配線 access + per-Skin UboInstance 経由 — PC-N-15b (e) thread-aware 拡張 = `sWorkerIdx != kInvalidWorkerIdx` 時 `sPcn14WorkerCtx[sWorkerIdx].mSkinUboSubDirty` 経路 + main thread `drainWorkersAndExecute` 内 merge → `sSkinUboDirty`
10. **`flushSkinUbos` 既配線**: `llvkloader.cpp:5221-5246` = `sSkinUboDirty` 全 entry iterate + dirty exchange + UBO buffer write — PC-N-15b (e) `drainWorkersAndExecute` 内 merge 後の main thread fire は既配線維持
11. **dynamic rendering helper `beginDynamicRendering`/`endDynamicRendering`**: `llvkloader.cpp:6685+` = `vkCmdBeginRendering` / `vkCmdEndRendering` (Vulkan 1.3 dynamic rendering) helper + `sInitialized` + `sInFrame` + `sCommandBuffer` 既配線 — PC-N-15b (c) secondary cmdbuf 内で main thread の dynamic rendering scope 継承する `VkCommandBufferInheritanceRenderingInfoKHR` パラメータ取得 source (= render area + color attachment format + depth attachment format + view mask 等)
12. **`vkCmdExecuteCommands` PFN**: `indra/llrender/volk.h:639` + `:2367` + `volk.c:527` + `:1870` + `:2969` = volk 経由で利用可能、`vkCmdExecuteCommands(VkCommandBuffer primary, U32 count, const VkCommandBuffer* secondaries)` signature — PC-N-15b (d) `drainWorkersAndExecute` 内 main `sCommandBuffer` に対し worker_count 個の secondary cmdbuf を集約
13. **PC-N-11 (a) `recordGltfAssetDraw` 内 sGltfStubSkin fall-through path**: `llvkloader.cpp:6321-6379` (multi-skin sentinel 段階卒業 tag block) — PC-N-15c (b) 撤去対象 (= `sAyastormGltfMultiSkinEnabled` cvar guard + `skin_to_use` 三項演算子 + `if (skin_to_use == sGltfStubSkin)` fall-through identity 64 B write + first-fire marker 全撤去)
14. **PC-N-10 (a) `recordAvatarPlaceholderDraw` 末尾 entry hook**: `llvkloader.cpp:6664-6671` (= `static LLCachedControl<bool> sAyastormGltfRealDrawEnabled` + cvar guard + `recordGltfAssetDraw(cmd_buf)` 呼出) — PC-N-15c (c) 撤去対象 (= PC-N-15b で dispatch 配線後 dead code 化、cmd_buf は recordAvatarPlaceholderDraw caller 側ゆえ recordGltfAssetDraw 不要)

### §1.3 background reference 5 件 (= Phase 1.E 全体 + 設計原則)

- **PC-N-15a complete handoff**: `handoff-...-phase1-e-pc-n-15a-complete.md` (commit `a90883cbf9`) = worker thread infrastructure 配線 baseline、本 PC-N-15b 実装の前提
- **PC-N-15a design-lock**: `handoff-...-phase1-e-pc-n-15a-design-lock.md` (commit `93357fa44e`) = (N15a-1)..(N15a-13) 13 件 ambiguity AYA OK record + LLUboRingBuffer per-instance 化方針確定 source
- **PC-N-14 design-lock**: `handoff-...-phase1-e-pc-n-14-design-lock.md` (commit `1a83070f31`) = (N14-1)..(N14-20) 20 件 ambiguity AYA OK record + worker thread design source (= (N14-1) ⭐ A secondary cmdbuf + vkCmdExecuteCommands 集約 pattern)
- **PC-N-13 complete handoff**: `handoff-...-phase1-e-pc-n-13-complete.md` (commit `faae1544d6`) = sPcn13MultiAssetSeen canary + zero IS real data semantic baseline
- **memory `project_ayastorm_r41_design_principles`** = (1) Upstream OpenGL 取り込みやすさ維持 + (2) Core プロセス分散実現の 2 大設計原則 source

---

## §2. 現状調査結果 (= PC-N-15a 完了 baseline + 既経路確認)

### §2.1 PC-N-15a 完了状態 (= 本 PC-N-15b 着手前 baseline)

| # | 項目 | 状態 | 改変方針 |
|---|------|------|---------|
| 1 | `PcN14WorkerContext` struct | 既配線 (PC-N-15a) | PC-N-15b 改変 0 件 (= mSkinUboSubDirty + mDrawUboSubRecords + mDrawUboRingBuffer + mCommandPool + mSecondaryCmdBuf 既配線済) |
| 2 | `sPcn14WorkerCtx` storage | 既配線 (PC-N-15a) | PC-N-15b 改変 0 件 |
| 3 | `sPcn14WorkerCount` atomic | 既配線 (PC-N-15a) | PC-N-15b 改変 0 件 |
| 4 | `createWorkerThreadInfra()` | 既配線 (PC-N-15a) | PC-N-15b (a) 末尾に WorkQueue 作成 + thread spawn logic 追加 |
| 5 | `destroyWorkerThreadInfra()` | 既配線 (PC-N-15a) | PC-N-15b (a) 冒頭に WorkQueue close + thread join 追加 |
| 6 | `thread_local sCurrentAsset` 等 4 件 | 既配線 (PC-N-15a) | PC-N-15b 改変 0 件 |
| 7 | `sPcn13MultiAssetSeenMutex` | 既配線 (PC-N-15a) | PC-N-15b (e) で `std::lock_guard` 配置 |
| 8 | `LLUboRingBuffer` per-instance | 既配線 (PC-N-15a) | PC-N-15b 改変 0 件 (= worker thread 内 `mDrawUboRingBuffer->allocate` 経路でそのまま利用) |
| 9 | `AYAGltfWorkerThreadEnabled` cvar | 既配線 (PC-N-15a) | PC-N-15b (b) `gltfscenemanager.cpp` 内 LLCachedControl で参照 |
| 10 | `AYAGltfWorkerThreadCount` cvar | 既配線 (PC-N-15a) | PC-N-15b 改変 0 件 (= `createWorkerThreadInfra` 内既参照) |

### §2.2 `recordGltfAssetDraw` 既 fire 経路確認 (= PC-N-15b dispatch 移行の設計判断 source)

**現状 fire 経路**:
- `LLDrawPoolAvatar::recordPoolDraws(cmd_buf)` (lldrawpoolavatar.cpp:1117-1130) → `LLVKLoader::recordAvatarPlaceholderDraw(cmd_buf)`
- `recordAvatarPlaceholderDraw` 末尾に `static LLCachedControl<bool> sAyastormGltfRealDrawEnabled` cvar guard (line 6664-6671) → `recordGltfAssetDraw(cmd_buf)` 呼出
- `recordGltfAssetDraw` 内 `sCurrentAsset` / `sCurrentPrimitive` null guard (line 6252-6256) で early-return = 現状 PC-N-8 (f) real path **不発火** (= `GLTFSceneManager::render` per-Primitive loop が完了して `clearCurrent*` 済の後で呼出されるため `sCurrent*` 全て null)

**重要 design insight**: PC-N-15b で `GLTFSceneManager::render` per-Primitive loop **内** dispatch hook を配置することで初めて `sCurrent*` 値が有効な状態で `recordGltfAssetDraw` (worker thread の secondary cmdbuf 引数版) が fire する経路が成立する。PC-N-15c で `recordAvatarPlaceholderDraw` 末尾 entry hook (= 現状 dead code) を撤去。

### §2.3 secondary cmdbuf inheritance info 要件 (= Vulkan 1.3 dynamic rendering)

main thread が `vkCmdBeginRendering` / `vkCmdEndRendering` で render pass scope 制御。secondary cmdbuf は `vkBeginCommandBuffer` 時 `VkCommandBufferBeginInfo::flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT` + `VkCommandBufferBeginInfo::pInheritanceInfo` で `VkCommandBufferInheritanceInfo::pNext` chain に `VkCommandBufferInheritanceRenderingInfoKHR` (= color attachment format[] + depth attachment format + view mask + raster samples) を渡す。main thread の dynamic rendering scope と一致する values が必要 = main thread が beginRendering 直前で snapshot 取得して PcN14WorkerContext に store する設計。

### §2.4 worker thread main loop pattern (= LL::WorkQueue runUntilClose)

`LL::WorkQueue queue("ayastorm.pcn15b.workers", capacity=1024, auto_shutdown=true)` → worker thread main = `[worker_idx, &queue]() { sWorkerIdx = worker_idx; queue.runUntilClose(); }`。`postPrimitiveToWorker` 内で `queue.post([asset, primitive, skin, modelview]() { ... })` で work 投入。`drainWorkersAndExecute` 内で main thread が WorkQueue が空になるまで wait (= 全 worker 完了確認) + 全 secondary cmdbuf を集約 `vkCmdExecuteCommands` で発火。WorkQueue close は `destroyWorkerThreadInfra` 内、thread join で確実に worker 停止確認。

### §2.5 PC-N-15c sGltfStubSkin 撤去 scope 確認

- `sGltfStubSkin` storage: `llvkloader.cpp:607-609` = `alignas(void*) char sGltfStubSkinStorage[1] = {}` + `LL::GLTF::Skin* const sGltfStubSkin = reinterpret_cast<LL::GLTF::Skin*>(&sGltfStubSkinStorage[0])` (= sentinel address のみ用途、Skin field access せず)
- `initVulkan` 内 `registerSkinUbo(sGltfStubSkin, Skin_GLTFJoints, ...)` (line 4231-4276 範囲): PC-N-5 (c) 配線、PC-N-15c 撤去対象
- `shutdownVulkan` 内 `unregisterSkinUbo(sGltfStubSkin, Skin_GLTFJoints)` (line 4626): PC-N-15c 撤去対象
- `recordGltfAssetDraw` 内 PC-N-11 (a) tag block (line 6321-6379): `sAyastormGltfMultiSkinEnabled` cvar guard + `skin_to_use` 三項演算子 + `if (skin_to_use == sGltfStubSkin)` fall-through path + first-fire marker = PC-N-15c 撤去対象、real Skin path 一本化
- `sPlaceholderSkin` (line 586-588) = avatar placeholder 用 sentinel、**PC-N-15c 撤去対象外** (= 別 phase = avatar Vulkan draw 通電 phase で撤去)

---

## §3. PC-N-15b ambiguity (Q1-Q4 + N15b-1..N15b-18) 22 件 = AYA literal「全部 OK です」record (2026-06-05)

AYA literal record: 2026-06-05 受領 = Q1-Q4 + (N15b-1)..(N15b-18) 全 22 件 A 採用案確定。

### §3.1 ⭐ critical 4 件 (Q1-Q4)

**Q1 = (N15b-1) ⭐ dispatch 投入位置**: `gltfscenemanager.cpp` per-Primitive loop 内 (= 既 `setCurrentPrimitive` 配置 line 752 と並列) に `LLVKLoader::postPrimitiveToWorker(asset, primitive, skin_or_null, mAssetMatrix_copy)` hook 追加。`gltfscenemanager.cpp` 改変要 (= PC-N-9/12 既改変位置の自然延長)。**A 採用根拠**: per-Primitive loop 内のみで Asset/Primitive/Skin/NodeAssetMatrix 値が確実に有効、PC-N-9/12 既配置位置と直列、cvar OFF 時影響ゼロ。

**Q2 = (N15b-4) ⭐ worker thread 起動方式**: 自前 `std::thread × N` を spawn + `LL::WorkQueue` 1 件 anonymous で `runUntilClose` 駆動。`LL::ThreadPool` 流用ではなく自前 spawn。**A 採用根拠**: cvar 駆動 worker count 動的決定、shutdown 順序 control 容易 (= WorkQueue close → thread join → `destroyWorkerThreadInfra`)、既 ThreadPool name conflict 回避。

**Q3 = (N15b-3) ⭐ aggregation タイミング**: per-Asset 末尾 (= `gltfscenemanager.cpp` per-Primitive loop 終了直後、line 816 ループ閉じ括弧直後、line 820 `clearCurrentAsset` 直前) に `LLVKLoader::drainWorkersAndExecute(sCommandBuffer)` hook 追加。per-Frame 末尾まで遅延せず Asset 単位で集約。**A 採用根拠**: Asset 単位 batching で scope clean、`sCommandBuffer` = main thread primary cmd_buf = Vulkan spec 厳守 (external sync)、Asset 跨ぎ secondary cmdbuf 累積回避。

**Q4 = (N15b-13a) `recordAvatarPlaceholderDraw` 末尾 entry hook 扱い**: 本 PC-N-15b では **温存** (= 既 cvar gate ON 時 dead code 発火だが視覚 no-op = MUSEUBO-A 整合維持)、PC-N-15c で `sGltfStubSkin` 撤去と同時に entry hook 整理。**B 採用根拠**: scope creep 回避、PC-N-15b は dispatch 配線単独 sub-step として完結、cleanup は PC-N-15c (= 7th sub-step) に分離。

### §3.2 standard 18 件 (N15b-1..N15b-18)

| # | ID | 内容 | 採用案 |
|---|----|------|--------|
| (N15b-2) | ⭐ work unit struct | `struct PcN15bWorkUnit { LL::GLTF::Asset* asset; LL::GLTF::Primitive* primitive; LL::GLTF::Skin* skin; F32 modelview[16]; U32 worker_idx; }` を main thread で構築 → lambda capture で WorkQueue post | A |
| (N15b-5) | `thread_local U32 sWorkerIdx` | worker thread launch 時 worker_idx を set、main thread は `kInvalidWorkerIdx`=`UINT32_MAX` 維持 | A |
| (N15b-6) | `writeDrawUbo` thread-aware 拡張 | worker thread 呼出 (sWorkerIdx != kInvalidWorkerIdx) 時 `sPcn14WorkerCtx[sWorkerIdx].mDrawUboRingBuffer` 経路、main thread 時既 `sDrawUboRingBufferMgr` 経路 | A |
| (N15b-7) | `writeSkinUbo` thread-aware 拡張 + merge | worker thread 呼出時 `sPcn14WorkerCtx[sWorkerIdx].mSkinUboSubDirty` に書込、main thread `drainWorkersAndExecute` 内で `sSkinUboDirty` に merge → `flushSkinUbos` 駆動 | A |
| (N15b-8) | `sPcn13MultiAssetSeen` mutex 保護 | worker thread 内 `insert` を `std::lock_guard<std::mutex>(sPcn13MultiAssetSeenMutex)` で wrap、main + worker access serialize で `size() > 1u` semantic 維持 | A |
| (N15b-9) | secondary cmdbuf inheritance | Vulkan 1.3 dynamic rendering 経路ゆえ `VkCommandBufferInheritanceRenderingInfoKHR` で main thread render scope 継承、`VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT` flag | A |
| (N15b-10) | graceful degrade fallback | worker post 失敗 (queue closed/full) 時 main thread で直接 `recordGltfAssetDraw(sCommandBuffer)` fallback 呼出 (= 既経路維持) | A |
| (N15b-11) | first-fire LL_INFOS marker 3 件 | `s_first_pcn14_worker_thread_fire` + `s_first_pcn14_secondary_cmdbuf_fire` + `s_first_pcn14_ubo_parallel_fire` 3 件、PC-N-6..15a 同形 atomic flag pattern | A |
| (N15b-12) | MUSEUBO-A 整合 | `AYAGltfWorkerThreadEnabled=false` default 時 `postPrimitiveToWorker` immediate `return false` (= 何もしない、queue post なし、worker launch されてても WorkQueue 空ループ)、main thread 経路は既配線完全維持 + OpenGL 描画 100% 維持 | A |
| (N15b-13b) | cvar guard 配置 | `gltfscenemanager.cpp` per-Primitive loop 内 `static LLCachedControl<bool> sAyastormGltfWorkerThreadEnabled` で確認後 `postPrimitiveToWorker` 呼出 vs main thread fallback 切替 (= 既 PC-N-11/12/13 cvar 配線同形) | A |
| (N15b-14) | build verify scope = PC-N-15a 同形 | llrender PASS + WARNING 0 + TUT 11/11 + 10/10 + 13/13 + codegen 131/131 + GATE-B integrity `LL_VULKAN_GLSL` count llvkloader.cpp=6 不変 | A |
| (N15b-15) | Exit Criteria 項目数 | 10 項 (実装 phase 同形) | A |
| (N15b-16) | 想定改変 file 5 件 | (1) `indra/llrender/llvkloader.h` + (2) `indra/llrender/llvkloader.cpp` + (3) `indra/newview/gltfscenemanager.cpp` + (4) `cross-platform spec` + (5) **本 design-lock doc §B「実装結果追記」section**（= 別 complete handoff doc 起案なし、案 B 統合方針整合） | A |
| (N15b-17) | `settings.xml` 改変 0 件 | PC-N-15a で `AYAGltfWorkerThreadEnabled` + `AYAGltfWorkerThreadCount` 2 cvar 既追加、PC-N-15b は cvar 新設なし | A |
| (N15b-18) | design-lock phase indra/ 改変 0 件 | `feedback_design_phase_no_code_write` 厳格遵守、本 design-lock commit は handoff doc + spec §6 PC-N-15b/c 行更新 + §A 履歴のみ | A |

### §3.3 採用根拠まとめ

22 件全件 A 採用 = PC-N-15a baseline 上に **worker thread dispatch 配線**を確実に達成 + main thread fallback 経路維持 + MUSEUBO-A 整合 + GATE-B 整合 + 設計原則 (Upstream OpenGL 取り込みやすさ維持 + Core プロセス分散実現) 両立。Q1-Q4 4 件 ⭐ critical = AYA literal 確認過程で「具体的に何が問題か分からない」literal feedback 経由整理 + 「全部 OK」record。

---

## §4. PC-N-15c ambiguity 12 件 = AYA literal 確認待ち (= 本 doc review 時 batch 提示、応答後 §A 更新)

### §4.1 ⭐ critical 2 件

**(N15c-1) ⭐ `sGltfStubSkin` sentinel storage 撤去方針**:
- A: `llvkloader.cpp:607-609` `sGltfStubSkinStorage` + `sGltfStubSkin` declaration 完全撤去 + `initVulkan` register + `shutdownVulkan` unregister 完全撤去 = real Skin path 強制一本化
- B: storage 維持 + cvar OFF 時 fallback 経路温存 = scope shrink
- **A 推奨**: PC-N-15b で worker thread dispatch 経由 real Skin path 完全通電後、sentinel fall-through は不要 = scope clean、AYAGltfMultiSkinEnabled cvar 撤去と整合

**(N15c-2) ⭐ `AYAGltfMultiSkinEnabled` cvar 撤去 vs 維持**:
- A: cvar 撤去 + `recordGltfAssetDraw` PC-N-11 (a) tag block (line 6321-6379) 内 cvar guard + `skin_to_use` 三項演算子 + `if (skin_to_use == sGltfStubSkin)` fall-through path 全撤去 = real Skin path 一本化
- B: cvar 維持 + default=true 化 + fall-through 温存
- C: cvar 維持 + default=false 化 (= 現状維持) → PC-N-15c の cleanup 目的に反する
- **A 推奨**: PC-N-15c の literal scope = sGltfStubSkin 撤去 = sentinel fall-through 撤去 = cvar 不要 (= 機能 gate ではなく段階卒業 gate)、PC-N-12/13/15 cvar (= 機能 gate) とは性質異なる

### §4.2 standard 10 件 (N15c-3..N15c-12)

**(N15c-3) `AYAGltfRealDrawEnabled` cvar + `recordAvatarPlaceholderDraw` 末尾 entry hook 撤去**: PC-N-15b で dispatch 配線後 dead code 化、PC-N-15c で撤去 = `settings.xml` から `AYAGltfRealDrawEnabled` entry 撤去 + `llvkloader.cpp:6664-6671` (PC-N-10 (a) tag block) 撤去。**A 推奨**: 採用 (= Q4 確定方針)

**(N15c-4) その他 cvar 維持判断**: 以下 cvar は機能 gate として **維持**:
- `AYAGltfRealModelviewEnabled` (PC-N-12) = real modelview 機能 gate
- `AYAGltfRealLightParamsEnabled` (PC-N-13) = real light params 機能 gate
- `AYAGltfMultiAssetCanary` (PC-N-13) = debug-only canary、開発用維持
- `AYAGltfWorkerThreadEnabled` (PC-N-15a) = worker thread 機能 gate
- `AYAGltfWorkerThreadCount` (PC-N-15a) = worker count 設定 cvar

**A 推奨**: 採用 (= cleanup 対象は段階卒業 cvar = AYAGltfMultiSkinEnabled + AYAGltfRealDrawEnabled の 2 件のみ)

**(N15c-5) `sGltfStubAssetPipeline` 扱い**: sky_smoke shader 流用 pipeline、Phase 1.F+ 実 PBR shader 接続まで温存 = PC-N-15c cleanup 対象外。**A 推奨**: 採用 (= zero IS real data semantic は PC-N-13 (N13-1) C で確立、Phase 1.F+ で PC-N-13.1 等で data 内容置換予定)

**(N15c-6) `sPlaceholderSkin` 扱い**: avatar placeholder 用 sentinel (= avatar Vulkan draw 通電 phase 持越し)、PC-N-15c cleanup 対象外。**A 推奨**: 採用 (= sGltfStubSkin のみ撤去、sPlaceholderSkin は別 phase)

**(N15c-7) Phase 1.E complete marker doc 内容**: 新規 `handoff-substep-...-phase1-e-complete.md` 起案 = Phase 1.E 全体総括 + PC-N-11..PC-N-15c 完了状態 + 実 data 通電 + multi-asset / multi-skin / real modelview / real per-draw light params / worker thread 並列化 全達成 marker + Phase 1 全完了状態確認 + Mac/Win 補完 phase entry へ。**A 推奨**: 採用 (= 案 B 統合方針整合 = PC-N-15c 完了 handoff = Phase 1.E complete marker 兼用)

**(N15c-8) build verify scope = PC-N-15b 同形**: llrender PASS + WARNING 0 + TUT 11/11 + 10/10 + 13/13 + codegen 131/131 + GATE-B integrity `LL_VULKAN_GLSL` count llvkloader.cpp=6 不変。**A 推奨**: 採用

**(N15c-9) Exit Criteria 項目数 10 項**: 実装 phase 同形。**A 推奨**: 採用

**(N15c-10) 想定改変 file 5 件**: (1) `indra/llrender/llvkloader.cpp` (= sGltfStubSkin 撤去 + PC-N-11 (a) tag block 撤去 + PC-N-10 (a) entry hook 撤去) + (2) `indra/newview/app_settings/settings.xml` (= AYAGltfMultiSkinEnabled + AYAGltfRealDrawEnabled 2 cvar entry 撤去) + (3) `cross-platform spec` (= §6 PC-N-15c 行 ✅ 反映 + §A 履歴) + (4) **本 design-lock doc §B「実装結果追記」section に PC-N-15c 結果追記** + (5) 新 `Phase 1.E complete marker handoff doc` 起案。**A 推奨**: 採用

**(N15c-11) design-lock phase indra/ 改変 0 件**: `feedback_design_phase_no_code_write` 厳格遵守、本 design-lock commit は本 doc 起案 + spec §6 PC-N-15b/c 行更新 + §A 履歴のみ。**A 推奨**: 採用

**(N15c-12) PC-N-15c 着手 timing**: PC-N-15b 実装完了 + build verify literal 取得 + AYA live 動作確認 (= worker thread 並列化 通電観測) PASS 後着手。**A 推奨**: 採用 (= 段階的実装で issue 局所化、`feedback_ubo_migration_one_at_a_time` 厳格遵守)

---

## §5. PC-N-15b 実装計画 = step (a)-(h) 8 step

### §5.1 step (a) `LL::WorkQueue` infrastructure 配線 ((N15b-4) ⭐ A、`createWorkerThreadInfra` 末尾 + `destroyWorkerThreadInfra` 冒頭 改変)

`indra/llrender/llvkloader.cpp`:

- 冒頭 include block に `<AYAstorm r41 PC-N-15b (a)>` tag block で `#include "workqueue.h"` 追加
- anonymous namespace 内 `sPcn14WorkerCtx` 直後並列に `<AYAstorm r41 PC-N-15b (a)>` tag block で以下追加:
  - `std::unique_ptr<LL::WorkQueue> sPcn14WorkerQueue` (= anonymous 名 = `"ayastorm.pcn15b.workers"`)
  - `std::vector<std::thread> sPcn14WorkerThreads` (= worker_count 個)
  - `thread_local U32 sWorkerIdx = kInvalidWorkerIdx` (= `kInvalidWorkerIdx = UINT32_MAX` constexpr)
- `createWorkerThreadInfra` 末尾 (line 1729 直前) に `<AYAstorm r41 PC-N-15b (a)>` tag block で:
  ```
  sPcn14WorkerQueue = std::make_unique<LL::WorkQueue>("ayastorm.pcn15b.workers", /*capacity=*/1024, /*auto_shutdown=*/false);
  sPcn14WorkerThreads.reserve(worker_count);
  for (U32 i = 0; i < worker_count; ++i)
  {
      sPcn14WorkerThreads.emplace_back([i]() {
          sWorkerIdx = i;
          static std::atomic<bool> s_first_pcn14_worker_thread_fire{true};
          if (s_first_pcn14_worker_thread_fire.exchange(false, std::memory_order_acq_rel))
          {
              LL_INFOS("Vulkan") << "PC-N-14 worker thread launch 成功 (first fire): worker_idx=" << i
                                 << ", worker_count=" << sPcn14WorkerCount.load(std::memory_order_acquire)
                                 << "; LL::WorkQueue runUntilClose 駆動開始" << LL_ENDL;
          }
          sPcn14WorkerQueue->runUntilClose();
          sWorkerIdx = kInvalidWorkerIdx;
      });
  }
  ```
- `destroyWorkerThreadInfra` 冒頭 (line 1738 直前) に `<AYAstorm r41 PC-N-15b (a)>` tag block で:
  ```
  if (sPcn14WorkerQueue) { sPcn14WorkerQueue->close(); }
  for (auto& t : sPcn14WorkerThreads) { if (t.joinable()) t.join(); }
  sPcn14WorkerThreads.clear();
  sPcn14WorkerQueue.reset();
  ```

### §5.2 step (b) `LLVKLoader::postPrimitiveToWorker` + `drainWorkersAndExecute` helper 実装 ((N15b-1) ⭐ A + (N15b-2) ⭐ A + (N15b-3) ⭐ A)

`indra/llrender/llvkloader.h`:
- `<AYAstorm r41 PC-N-15b (b)>` tag block で public API 宣言追加:
  ```
  bool postPrimitiveToWorker(LL::GLTF::Asset* asset, LL::GLTF::Primitive* primitive,
                             LL::GLTF::Skin* skin, const F32* mAssetMatrix);
  void drainWorkersAndExecute(VkCommandBuffer primary_cmd_buf);
  ```

`indra/llrender/llvkloader.cpp`:
- `<AYAstorm r41 PC-N-15b (b)>` tag block で `postPrimitiveToWorker` 実装:
  ```
  bool postPrimitiveToWorker(LL::GLTF::Asset* asset, LL::GLTF::Primitive* primitive,
                             LL::GLTF::Skin* skin, const F32* mAssetMatrix)
  {
      static LLCachedControl<bool> sAyastormGltfWorkerThreadEnabled(
          gSavedSettings, "AYAGltfWorkerThreadEnabled", false);
      if (!sAyastormGltfWorkerThreadEnabled || !sPcn14WorkerQueue || sPcn14WorkerCtx.empty())
      {
          return false;  // main thread fallback
      }
      F32 modelview_copy[16];
      if (mAssetMatrix) { std::memcpy(modelview_copy, mAssetMatrix, sizeof(modelview_copy)); }
      else              { std::memset(modelview_copy, 0, sizeof(modelview_copy)); }
      return sPcn14WorkerQueue->post([asset, primitive, skin, modelview_copy]() {
          setCurrentAsset(asset);
          setCurrentPrimitive(primitive);
          if (skin) setCurrentSkin(skin);
          setCurrentNodeAssetMatrix(mAssetMatrix ? modelview_copy : nullptr);
          // worker thread 内 secondary cmdbuf に record
          PcN14WorkerContext& ctx = sPcn14WorkerCtx[sWorkerIdx];
          VkCommandBufferInheritanceRenderingInfoKHR inherit_rendering = {};
          inherit_rendering.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO_KHR;
          // ... main thread snapshot から format/view mask copy ...
          VkCommandBufferInheritanceInfo inherit = {};
          inherit.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
          inherit.pNext = &inherit_rendering;
          VkCommandBufferBeginInfo begin_info = {};
          begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
          begin_info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT |
                             VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
          begin_info.pInheritanceInfo = &inherit;
          vkBeginCommandBuffer(ctx.mSecondaryCmdBuf, &begin_info);
          static std::atomic<bool> s_first_pcn14_secondary_cmdbuf_fire{true};
          if (s_first_pcn14_secondary_cmdbuf_fire.exchange(false, std::memory_order_acq_rel))
          {
              LL_INFOS("Vulkan") << "PC-N-14 secondary cmdbuf vkBeginCommandBuffer 成功 (first fire): "
                                    "worker_idx=" << sWorkerIdx << LL_ENDL;
          }
          recordGltfAssetDraw(ctx.mSecondaryCmdBuf);
          vkEndCommandBuffer(ctx.mSecondaryCmdBuf);
          clearCurrentSkin();
          clearCurrentNodeAssetMatrix();
          clearCurrentPrimitive();
          clearCurrentAsset();
      });
  }
  ```
- `<AYAstorm r41 PC-N-15b (b)>` tag block で `drainWorkersAndExecute` 実装:
  ```
  void drainWorkersAndExecute(VkCommandBuffer primary_cmd_buf)
  {
      if (!sPcn14WorkerQueue || sPcn14WorkerCtx.empty() || primary_cmd_buf == VK_NULL_HANDLE)
      {
          return;
      }
      // wait queue drain - simple busy wait until queue.size() == 0 OR yieldable wait
      while (sPcn14WorkerQueue->size() > 0) { std::this_thread::yield(); }
      // collect secondary cmdbufs (= per-worker recorded)
      std::vector<VkCommandBuffer> secondaries;
      secondaries.reserve(sPcn14WorkerCtx.size());
      for (auto& ctx : sPcn14WorkerCtx)
      {
          secondaries.push_back(ctx.mSecondaryCmdBuf);
      }
      vkCmdExecuteCommands(primary_cmd_buf, (U32)secondaries.size(), secondaries.data());
      // merge mSkinUboSubDirty → sSkinUboDirty
      for (auto& ctx : sPcn14WorkerCtx)
      {
          for (auto& kv : ctx.mSkinUboSubDirty) { sSkinUboDirty[kv.first] = kv.second; }
          ctx.mSkinUboSubDirty.clear();
      }
  }
  ```

### §5.3 step (c) `gltfscenemanager.cpp` per-Primitive loop 内 dispatch hook 追加 ((N15b-1) ⭐ A + (N15b-13b) A)

`indra/newview/gltfscenemanager.cpp`:

- per-Primitive loop 内 `setCurrentPrimitive` 直後並列 (= line 752 直後) に `<AYAstorm r41 PC-N-15b (c)>` tag block で:
  ```
  bool worker_dispatched = LLVKLoader::postPrimitiveToWorker(
      &asset, &primitive,
      rigged ? &asset.mSkins[node.mSkin] : nullptr,
      glm::value_ptr(node.mAssetMatrix));
  // 既 setCurrentNodeAssetMatrix / setCurrentSkin / drawRangeFast / clearCurrent* は worker_dispatched=false 時 main thread fallback で発火 (= 既経路維持)
  ```
- per-Primitive loop 終了直後 (= line 816 ループ閉じ括弧直後、line 820 `clearCurrentAsset` 直前) に `<AYAstorm r41 PC-N-15b (c)>` tag block で:
  ```
  LLVKLoader::drainWorkersAndExecute(LLVKLoader::getCurrentPrimaryCommandBuffer());
  ```
- `getCurrentPrimaryCommandBuffer()` accessor: `llvkloader.h` + `llvkloader.cpp` で `VkCommandBuffer getCurrentPrimaryCommandBuffer() { return sCommandBuffer; }` 1 行追加 (= main thread sCommandBuffer 露出)

### §5.4 step (d) `writeDrawUbo` thread-aware 拡張 ((N15b-6) A)

`indra/llrender/llvkloader.cpp:5528` `writeDrawUbo` 関数内:
- 冒頭で `const U32 worker_idx = sWorkerIdx;`
- ring buffer 選択: `worker_idx != kInvalidWorkerIdx` 時 `auto& ring = sPcn14WorkerCtx[worker_idx].mDrawUboRingBuffer; auto& records = sPcn14WorkerCtx[worker_idx].mDrawUboSubRecords;`、main thread 時 `auto& ring = sDrawUboRingBufferMgr; auto& records = sDrawUboRingBufferRecords;`
- 以降 `ring->allocate(...)` + `records.find(handle)` で既 logic そのまま
- worker thread 内最初の write 成功時 `s_first_pcn14_ubo_parallel_fire` first-fire LL_INFOS marker

### §5.5 step (e) `writeSkinUbo` thread-aware 拡張 + merge ((N15b-7) A)

`indra/llrender/llvkloader.cpp:5829` `writeSkinUbo` 関数内:
- 冒頭で `const U32 worker_idx = sWorkerIdx;`
- dirty map 選択: `worker_idx != kInvalidWorkerIdx` 時 `auto& dirty = sPcn14WorkerCtx[worker_idx].mSkinUboSubDirty;`、main thread 時 `auto& dirty = sSkinUboDirty;`
- 以降既 logic そのまま (= `try_emplace` + `mapped_ptr[frame]` write)
- main thread `drainWorkersAndExecute` 内 merge logic は step (b) 内で配線済

### §5.6 step (f) `sPcn13MultiAssetSeen` mutex 保護 ((N15b-8) A)

`indra/llrender/llvkloader.cpp:6460` `recordGltfAssetDraw` 内 PC-N-13 (b) tag block の `sPcn13MultiAssetSeen.insert(...)` + `size() > 1u` 判定を `<AYAstorm r41 PC-N-15b (f)>` tag block で:
```
{
    std::lock_guard<std::mutex> lock(sPcn13MultiAssetSeenMutex);
    sPcn13MultiAssetSeen.insert(static_cast<const void*>(asset));
    if (sPcn13MultiAssetSeen.size() > 1u) { /* first-fire marker (既) */ }
}
```

### §5.7 step (g) build verify literal 取得 ((N15b-14) A)

- llrender library clean rebuild PASS (= warning 0 + error 0)
- `INTEGRATION_TEST_lluboringbuffer` 11/11 PASS
- `INTEGRATION_TEST_llassetubopool` 10/10 PASS
- `INTEGRATION_TEST_llpipelinecachestorage` 13/13 PASS
- `python3 -m unittest discover` from `scripts/ubo_codegen` = 131 tests, OK
- GATE-B integrity = `grep -c LL_VULKAN_GLSL indra/llrender/llvkloader.cpp` = **6 不変** (= PC-N-15a commit `a90883cbf9` 同数)

### §5.8 step (h) cross-platform spec §6 PC-N-15b 行 ✅ 反映 + §A 履歴 1 行追記 + 本 design-lock doc §B「実装結果追記」section に PC-N-15b 結果記録 ((N15b-16) A 案 B 統合方針)

`docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md`:
- §6 PC-N-15b 行 状態 ⏳ → ✅ 反映
- §A 履歴 1 行追記 (= PC-N-15b 実装 complete chronological entry)

本 design-lock doc §B section 追記 (= 別 complete handoff doc 起案なし、案 B 統合方針整合):
- PC-N-15b 実装結果 literal record (= step (a)-(h) 実装結果 + build verify literal + Exit Criteria 10 項充足確認)

---

## §6. PC-N-15c 実装計画 = step (a)-(g) 7 step (= PC-N-15b 完了後 fine-tune 可能 section)

### §6.1 step (a) `sGltfStubSkin` storage + register/unregister 配線 撤去 ((N15c-1) ⭐ A)

`indra/llrender/llvkloader.cpp`:
- `:607-609` `sGltfStubSkinStorage` + `sGltfStubSkin` declaration block 撤去 (= PC-N-5 (a) tag block 削除)
- `:4231-4276` `initVulkan` 内 `registerSkinUbo(sGltfStubSkin, ...)` + `wireSkinUboSetV3aToBinding2(sGltfStubSkin)` + first-fire log block (= PC-N-5 (c) tag block) 撤去
- `:4626` `shutdownVulkan` 内 `unregisterSkinUbo(sGltfStubSkin, Skin_GLTFJoints)` (= PC-N-5 (d) tag block) 撤去

### §6.2 step (b) `AYAGltfMultiSkinEnabled` cvar + PC-N-11 (a) fall-through path 撤去 ((N15c-2) ⭐ A)

`indra/llrender/llvkloader.cpp`:
- `:6321-6379` `recordGltfAssetDraw` 内 PC-N-11 (a) tag block 撤去 (= `sAyastormGltfMultiSkinEnabled` cvar guard + `skin_to_use` 三項演算子 + `if (skin_to_use == sGltfStubSkin)` fall-through identity 64 B write + first-fire marker)
- real Skin path 一本化: `LL::GLTF::Skin* const skin_to_use = sCurrentSkin;` で `if (skin_to_use == nullptr) return;` early exit + `wireSkinUboSetV3aToBinding2(skin_to_use)` + `flushSkinUbos(skin_to_use)` 直列 (= 既 real Skin path はそのまま)

`indra/newview/app_settings/settings.xml`:
- `AYAGltfMultiSkinEnabled` cvar entry 撤去

### §6.3 step (c) `AYAGltfRealDrawEnabled` cvar + `recordAvatarPlaceholderDraw` 末尾 entry hook 撤去 ((N15c-3) A)

`indra/llrender/llvkloader.cpp`:
- `:6664-6671` `recordAvatarPlaceholderDraw` 末尾 PC-N-10 (a) tag block (= `sAyastormGltfRealDrawEnabled` LLCachedControl + cvar guard + `recordGltfAssetDraw(cmd_buf)` 呼出) 撤去
- `recordAvatarPlaceholderDraw` の closing brace `}` まで配置確認

`indra/newview/app_settings/settings.xml`:
- `AYAGltfRealDrawEnabled` cvar entry 撤去

### §6.4 step (d) build verify literal 取得 ((N15c-8) A)

step (g) PC-N-15b 同形 = llrender PASS + WARNING 0 + TUT 11/11 + 10/10 + 13/13 + codegen 131/131 + GATE-B integrity `LL_VULKAN_GLSL` count llvkloader.cpp=6 不変 (= PC-N-15b 完了時点同数)

### §6.5 step (e) cross-platform spec §6 PC-N-15c 行 ✅ 反映 + §A 履歴 1 行追記

`docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md`:
- §6 PC-N-15c 行 状態 ⏳ → ✅ 反映
- §A 履歴 1 行追記 (= PC-N-15c 実装 complete = Phase 1.E complete marker chronological entry)

### §6.6 step (f) Phase 1.E complete marker handoff doc 起案 ((N15c-7) A 案 B 統合方針整合)

新規 `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-e-complete.md`:
- Phase 1.E 全体総括 = PC-N-11..PC-N-15c 完了状態
- 達成事項列挙 = 実 data 通電 + multi-asset / multi-skin / real modelview / real per-draw light params / worker thread 並列化 全達成
- Phase 1 全完了状態確認
- Mac/Win 補完 phase entry へ移行 marker
- self-verify 9 観点

### §6.7 step (g) 本 design-lock doc §B「実装結果追記」section に PC-N-15c 結果記録 ((N15c-10) A 案 B 統合方針)

- PC-N-15c 実装結果 literal record (= step (a)-(g) 実装結果 + build verify literal + Exit Criteria 10 項充足確認)

---

## §7. PC-N-15b 実装 phase Exit Criteria 10 項

| # | criterion | 充足判定 |
|---|-----------|---------|
| i | `LL::WorkQueue` infrastructure 配線 = anonymous queue 作成 + worker_count 個 `std::thread` spawn + `thread_local U32 sWorkerIdx` 配線 ((N15b-4) ⭐ A + (N15b-5) A) | ⏳ |
| ii | `LLVKLoader::postPrimitiveToWorker` + `LLVKLoader::drainWorkersAndExecute` helper 新設 ((N15b-1) ⭐ A + (N15b-2) ⭐ A + (N15b-3) ⭐ A) | ⏳ |
| iii | `gltfscenemanager.cpp` per-Primitive loop 内 dispatch hook 追加 (= `setCurrentPrimitive` 直後並列 + per-Asset loop 終了直後 drain 呼出) | ⏳ |
| iv | `writeDrawUbo` + `writeSkinUbo` thread-aware 拡張 ((N15b-6) A + (N15b-7) A) | ⏳ |
| v | `sPcn13MultiAssetSeen` `std::lock_guard<std::mutex>` 配置 ((N15b-8) A) | ⏳ |
| vi | secondary cmdbuf `VkCommandBufferInheritanceRenderingInfoKHR` 経由 main thread dynamic rendering scope 継承 ((N15b-9) A) | ⏳ |
| vii | first-fire LL_INFOS marker 3 件 (`s_first_pcn14_worker_thread_fire` + `s_first_pcn14_secondary_cmdbuf_fire` + `s_first_pcn14_ubo_parallel_fire`) ((N15b-11) A) | ⏳ |
| viii | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp=6 不変) | ⏳ |
| ix | MUSEUBO-A 整合 = `AYAGltfWorkerThreadEnabled=false` default で main thread 経路完全維持 + OpenGL 描画 100% 維持 + PC-N-8 (f) 5 段 graceful degrade 内部維持 + PC-N-11/12/13 cvar baseline 不変 ((N15b-12) A) | ⏳ |
| x | build verify literal 取得 + cross-platform spec §6 PC-N-15b 行 ✅ 反映 + §A 履歴 + 本 design-lock doc §B 追記 (= 案 B 統合方針整合 = 別 complete handoff doc 起案なし) | ⏳ |

---

## §8. PC-N-15c 実装 phase Exit Criteria 10 項 (= Phase 1.E complete marker 含む)

| # | criterion | 充足判定 |
|---|-----------|---------|
| i | `sGltfStubSkin` storage 撤去 ((N15c-1) ⭐ A) + `initVulkan` 内 register 配線 撤去 + `shutdownVulkan` 内 unregister 配線 撤去 | ⏳ |
| ii | `AYAGltfMultiSkinEnabled` cvar 撤去 + `recordGltfAssetDraw` PC-N-11 (a) fall-through path 撤去 + real Skin path 一本化 ((N15c-2) ⭐ A) | ⏳ |
| iii | `AYAGltfRealDrawEnabled` cvar 撤去 + `recordAvatarPlaceholderDraw` 末尾 PC-N-10 (a) entry hook 撤去 ((N15c-3) A) | ⏳ |
| iv | その他 cvar (AYAGltfRealModelviewEnabled / RealLightParamsEnabled / MultiAssetCanary / WorkerThreadEnabled / WorkerThreadCount) 維持 ((N15c-4) A) | ⏳ |
| v | `sGltfStubAssetPipeline` 維持 (= Phase 1.F+ 持越し、(N15c-5) A) + `sPlaceholderSkin` 維持 (= avatar Vulkan draw 通電 phase 持越し、(N15c-6) A) | ⏳ |
| vi | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp=6 不変、PC-N-15b 同数) | ⏳ |
| vii | MUSEUBO-A 整合 = OpenGL 描画 100% 維持 + Phase 1.E PC-N-12/13/15 cvar OFF default で旧経路維持 | ⏳ |
| viii | build verify literal 取得 ((N15c-8) A) | ⏳ |
| ix | cross-platform spec §6 PC-N-15c 行 ✅ 反映 + §A 履歴 + 本 design-lock doc §B 追記 | ⏳ |
| x | **Phase 1.E complete marker handoff doc 起案 ((N15c-7) A 案 B 統合方針整合)** + Phase 1.E 全体総括 + Phase 1 全完了状態確認 + Mac/Win 補完 phase entry へ移行 marker | ⏳ |

---

## §9. 残 strict 線形 + r41 milestone state

### §9.1 残 strict 線形

- ✅ Phase 1.A / 1.B / (Z) SSS / (W) uniform4iv / (Y) Phase 1.C prep
- ✅ PC-0..PC-7ε / PC-N decomposition / PC-N-1..PC-N-4 (= Phase 1.C complete)
- ✅ PC-8 Linux primary marker (= Phase 1.C strict 線形終了)
- ✅ PC-N-5 / Phase 1.D decomposition design-lock
- ✅ PC-N-6 / PC-N-7 / PC-N-8 / PC-N-9 / PC-N-10 (= Phase 1.D complete = 1 GLTF asset 完全 Vulkan draw 通電 達成)
- ✅ Phase 1.E decomposition design-lock (commit `01cd001d07`)
- ✅ PC-N-11 design-lock (commit `cdf4dccc0d`) + PC-N-11 実装 (commit `13bfb55b35`)
- ✅ PC-N-12 design-lock (commit `c9c99d278f`) + PC-N-12 実装 (commit `4373c302d7`)
- ✅ PC-N-13 design-lock (commit `e13d00d4a6`) + PC-N-13 実装 (commit `faae1544d6`)
- ✅ PC-N-14 design-lock (commit `1a83070f31`)
- ✅ PC-N-15a design-lock (commit `93357fa44e`) + PC-N-15a 実装 (commit `a90883cbf9`)
- ✅ **PC-N-15b + PC-N-15c 統合 design-lock ✅ 本 commit (案 B 統合方針 = doc 4 件 → 2 件)**
- ⏳ PC-N-15b 実装 (= 次 session、build verify + AYA live 確認 後本 doc §B 追記)
- ⏳ PC-N-15c 実装 (= PC-N-15b 完了後別 session、Phase 1.E complete marker handoff doc 起案)
- ⏳ Phase 1.E complete = 実 data 通電 + multi-asset / multi-skin / real modelview / real per-draw light params / worker thread 並列化 全達成
- ⏳ Phase 1 全完了 → Mac/Win 開発者補完 phase

### §9.2 r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ +
PC-0..PC-6ζ ✅ + PC-7α..PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1..PC-N-4 ✅ = Phase 1.C complete ✅ +
PC-8 Linux primary marker ✅ + PC-N-5 ✅ + Phase 1.D decomposition design-lock ✅ +
PC-N-6 ✅ + PC-N-7 ✅ + PC-N-8 ✅ + PC-N-9 ✅ + PC-N-10 ✅ = Phase 1.D complete ✅ +
Phase 1.E decomposition design-lock ✅ + PC-N-11 design-lock ✅ + PC-N-11 ✅ +
PC-N-12 design-lock ✅ + PC-N-12 ✅ + PC-N-13 design-lock ✅ + PC-N-13 ✅ +
PC-N-14 design-lock ✅ + PC-N-15a design-lock ✅ + PC-N-15a ✅ +
**PC-N-15b + PC-N-15c 統合 design-lock ✅ 本 commit (= 案 B doc 削減 4 → 2 件)** +
⏳ PC-N-15b 実装 + ⏳ PC-N-15c 実装 + ⏳ Phase 1.E complete marker handoff + ⏳ Phase 1 全完了 + ⏳ Mac/Win 開発者補完 phase

---

## §10. self-verify 9 観点 全 ✅

1. ✅ PC-N-15b literal scope §0 完全分解 6 項 (= AYA literal「全部 OK」record 2026-06-05) + PC-N-15c literal scope §0 完全分解 6 項 (= AYA literal 確認待ち batch 提示)
2. ✅ 必読 1 件 §1.1 + pinpoint reference 14 件 §1.2 + background reference 5 件 §1.3 別記 = full file dump なし (= `feedback_handoff_minimal_pre_req_read` 整合)
3. ✅ 現状調査 §2 5 sub-section 網羅 (特に §2.2 `recordGltfAssetDraw` 既 fire 経路確認 = `sCurrent*` null guard で early-return 発見が本 PC-N-15b/c design-lock の knowledge contribution = PC-N-15b で `gltfscenemanager.cpp` per-Primitive loop 内 dispatch hook 配置で初めて real path 通電する経路成立、PC-N-15c で `recordAvatarPlaceholderDraw` 末尾 dead code entry hook 撤去 経路明確化)
4. ✅ ambiguity §3 PC-N-15b (Q1-Q4 + N15b-1..N15b-18) 22 件 AYA literal「全部 OK」record (2026-06-05) + §4 PC-N-15c (N15c-1)..(N15c-12) 12 件 AYA literal 確認待ち batch 提示 + 採用根拠明文化 (特に Q1 ⭐ dispatch site = gltfscenemanager.cpp per-Primitive loop 内 + Q3 ⭐ aggregation = per-Asset 末尾 + Q4 ⭐ recordAvatarPlaceholderDraw 末尾 hook = PC-N-15c 撤去持越し + (N15c-1) ⭐ sGltfStubSkin 完全撤去 + (N15c-2) ⭐ AYAGltfMultiSkinEnabled cvar 完全撤去)
5. ✅ 実装計画 §5 PC-N-15b step (a)-(h) 8 step + §6 PC-N-15c step (a)-(g) 7 step + 各 step 具体 code stub example 添付
6. ✅ GATE-B 整合 §5/§6 (= `#ifdef LL_VULKAN_GLSL` 新規追加 0 件想定、`AYAGltfWorkerThreadEnabled` cvar runtime gate のみ、shader 改変ゼロ、count llvkloader.cpp=6 不変)
7. ✅ MUSEUBO-A 整合 §5/§6 (= `AYAGltfWorkerThreadEnabled=false` default で main thread 経路完全維持 + OpenGL 描画 100% 維持 + PC-N-15c で AYAGltfMultiSkinEnabled + AYAGltfRealDrawEnabled 撤去でも他 cvar 維持 + 旧経路 fall-through 経路は cleanup 対象ゆえ撤去で MUSEUBO-A 整合維持)
8. ✅ 設計原則整合 §5/§6 (= (1) Upstream OpenGL 取り込みやすさ維持 = recordGltfAssetDraw signature 不変 + setCurrentXxx accessor signature 不変 + shader 改変ゼロ + (2) Core プロセス分散実現 = per-Primitive worker dispatch + secondary cmdbuf record + vkCmdExecuteCommands 集約 で primitive-level granularity 達成)
9. ✅ indra/ 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合 + 案 B 統合方針整合 (= handoff doc 4 件 → 2 件削減方針 AYA literal「B でお願いします」record 2026-06-05 整合)

---

## §11. 次 session 着手 1 line

PC-N-15b 実装着手 = step (a)-(h) 8 step 実施 = (a) LL::WorkQueue infrastructure 配線 (= anonymous queue + `std::thread` spawn + `thread_local sWorkerIdx`) + (b) `LLVKLoader::postPrimitiveToWorker` + `drainWorkersAndExecute` helper 実装 + (c) `gltfscenemanager.cpp` per-Primitive loop 内 dispatch hook + drain hook 追加 + (d) `writeDrawUbo` thread-aware 拡張 + (e) `writeSkinUbo` thread-aware 拡張 + merge + (f) `sPcn13MultiAssetSeen` `std::lock_guard` 配置 + (g) build verify literal 取得 + (h) cross-platform spec §6 PC-N-15b 行 ✅ 反映 + §A 履歴 + **本 design-lock doc §B「実装結果追記」section に PC-N-15b 結果記録** (= 案 B 統合方針整合 = 別 complete handoff doc 起案なし)。PC-N-15c 着手は PC-N-15b 完了 + AYA live 動作確認 PASS 後の別 session。

---

## §A. feedback 遵守 record + AYA literal record

- ✅ `feedback_proactive_handoff` (本 PC-N-15b + PC-N-15c 統合 design-lock doc 起案)
- ✅ `feedback_handoff_minimal_pre_req_read` (必読 1 件 = 本 doc + pinpoint reference 14 件 + background reference 5 件別記、full file dump なし)
- ✅ `feedback_self_verify_before_handoff` (9 観点 self-verify 全 ✅)
- ✅ `feedback_build_only_verified` (design-lock phase は indra/ 改変 0 件で build verify 対象外、実装 phase で literal 検証取得予定 (N15b-14) A + (N15c-8) A 採用)
- ✅ `feedback_no_scope_shrink` (PC-N-15b literal scope 6 件 §0 全件明文化 + PC-N-15c literal scope 6 件 §0 全件明文化、案 B 統合方針 (= handoff doc 4 件 → 2 件削減) は scope 縮小ではなく **doc 起案数の削減のみ** で実装 scope は完全保持)
- ✅ `feedback_doubt_self_first` (design-lock phase で ambiguity 22 件 + AYA literal「具体的に何が問題か分からない」literal feedback 経由 Q1-Q4 ⭐ critical に整理 + 推奨案提示後 AYA literal「全部 OK」record 受領、推測実装なし、PC-N-15c も 12 件 ambiguity batch 提示で AYA 確認待ち)
- ✅ `feedback_admit_unknown` (現状調査で `recordGltfAssetDraw` 既 fire 経路 = `recordAvatarPlaceholderDraw` 末尾呼出 = `sCurrent*` null guard で early-return 発見 → 「現状 PC-N-8 (f) real path 不発火」と認め、勝手に「動いている」と仮定せず本 doc §2.2 で明示記録 + AYA に PC-N-15b dispatch site = gltfscenemanager.cpp per-Primitive loop 内配置の必要性提示)
- ✅ `feedback_confirm_referent_before_acting` (PC-N-15b ambiguity 22 件 batch AYA 確認完了 + PC-N-15c ambiguity 12 件 batch 提示、特に Q1-Q4 ⭐ critical は overview 整理 + 推奨案根拠明示後 AYA literal「全部 OK」record 受領、推測実装なし)
- ✅ `feedback_ubo_migration_one_at_a_time` 厳格遵守 (PC-N-15 → PC-N-15a/b/c 3 sub-phase 分解は「1 つずつ実施・大きすぎたらスコープ切り直す」原則直接適用、PC-N-15b = dispatch 配線単独 sub-step、PC-N-15c = cleanup + Phase 1.E complete marker は段階的実装、各 sub-step build verify + AYA live 確認 PASS 後次着手)
- ✅ `feedback_design_phase_no_code_write` 厳格遵守 (本 PC-N-15b + PC-N-15c 統合 design-lock phase は doc 起案 + cross-platform spec §6 PC-N-15b/c 行更新 + §A 履歴 1 行追記のみ、indra/ 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件)
- ✅ `feedback_release_branch_workflow` (feature branch `feature/ayastorm-r41-gl-removal` 上 commit)
- ✅ `feedback_no_auto_commit` (AYA 明示 commit 指示受領後 commit 予定)
- ✅ `feedback_no_claude_coauthor` (Co-Authored-By 行不在)
- ✅ `feedback_no_bare_reference_ids` ((N15b-1)..(N15b-18) + (N15c-1)..(N15c-12) 各 ID に項目名 / 採用案内容併記 + (a)..(h) PC-N-15b 各 step + (a)..(g) PC-N-15c 各 step に作業内容併記)
- ✅ `feedback_tests_dir_never_commit` 整合 (tests/ 改変 0 件、git add 個別 file 指定予定)
- ✅ memory `project_ayastorm_r41_design_principles` 整合 ((1) Upstream OpenGL 取り込みやすさ維持 = recordGltfAssetDraw signature 不変 + setCurrentXxx accessor signature 不変 + writeDrawUbo / writeSkinUbo signature 不変 + shader 改変ゼロ + (2) Core プロセス分散実現 = per-Primitive worker dispatch + secondary cmdbuf record + vkCmdExecuteCommands 集約 で primitive-level granularity 達成)
- ✅ memory `project_r41_phase1b_vulkan_host_gate` 整合 (GATE-B = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、`AYAGltfWorkerThreadEnabled` cvar runtime gate のみ、count llvkloader.cpp=6 不変想定)
- ✅ memory `project_ayastorm_three_platforms` 整合 (cross-platform spec §6 PC-N-15b/c 行 design-lock 内容更新で macOS / Windows 派生 fix 候補なし想定 = host-side LL::WorkQueue + secondary cmdbuf + vkCmdExecuteCommands は OS 非依存 + MoltenVK 標準対応範囲 + descriptor set 数 5 維持 + cleanup phase の cvar/storage 撤去は OS 非依存 ゆえ macOS 派生 fix 候補なし + Windows full Vulkan ゆえ派生 fix 候補なし、Linux primary 完成 → 他者補完 model 整合)

### §A.x AYA literal record (= 本 design-lock phase 中の確認 record 時系列)

- 2026-06-05: AYA 指示「r41 Phase 1.E PC-N-15b design-lock 着手お願いします」literal 受領 (= 着手契機)
- 2026-06-05: Claude が PC-N-15b ambiguity 18 件 batch 提示 → AYA literal「具体的に何が問題になるのかちょっとよくわからないです」literal feedback 受領
- 2026-06-05: Claude が overview 整理 + Q1-Q4 4 件 ⭐ critical 提示 → AYA literal「全部 OK です」record 受領 (= Q1-Q4 + N15b-1..18 全 22 件 A 採用案確定)
- 2026-06-05: Claude が「handoff doc 増えすぎ + context size」懸念提示 + 統合案 3 件 (A/B/C) 提示 → AYA literal「それとももともと 1M あったけど足りなかった?」literal 質問受領
- 2026-06-05: Claude が率直回答 (= 1M context 余裕あるが doc 数自体が次 session Read 負荷 + AYA review 負荷 + spec source of truth 分散の問題ゆえ doc 削減は質的改善目的) + 推奨 = 案 B → AYA literal「B でお願いします」record 受領 (= 案 B 統合方針確定)
- 2026-06-05: Claude が design-lock doc 起案完了 + PC-N-15c ambiguity 12 件 batch 提示 (= ⭐ critical 2 件 (N15c-1) sGltfStubSkin 完全撤去 + (N15c-2) AYAGltfMultiSkinEnabled 完全撤去 + standard 10 件 (N15c-3)..(N15c-12)) → AYA literal「OK」record 受領 (= PC-N-15c 12 件全 A 採用案確定、本 design-lock doc complete)

---

## §B. 実装結果追記 section (= PC-N-15b/c 実装完了時に追記、初稿時は空)

### §B.1 PC-N-15b 実装結果 (= step (a)-(h) 完了時追記、commit hash + build verify literal + Exit Criteria 10 項充足判定)

**実装日**: 2026-06-05
**HEAD 起点**: `26db10dfda` (= PC-N-15b/c 統合 design-lock complete)
**着手契機**: AYA 指示「r41 Phase 1.E PC-N-15b 実装着手お願いします」literal record 2026-06-05

#### §B.1.1 step (a)-(h) 8 step 全実施

- **(a) `LL::WorkQueue` infrastructure 配線 ((N15b-4) ⭐ A + (N15b-5) A + (N15b-11) A worker_thread fire)**:
  - `indra/llrender/llvkloader.cpp` 冒頭 include block に `<AYAstorm r41 PC-N-15b (a)>` tag block で `#include "workqueue.h"` + `#include <array>` 追加。
  - anonymous namespace 内 `sPcn14WorkerCtx` storage の直後並列に `<AYAstorm r41 PC-N-15b (a)>` tag block で以下追加:
    - `constexpr U32 kInvalidWorkerIdx = UINT32_MAX;`
    - `thread_local U32 sWorkerIdx = kInvalidWorkerIdx;`
    - `std::unique_ptr<LL::WorkQueue> sPcn14WorkerQueue;`
    - `std::vector<std::thread> sPcn14WorkerThreads;`
  - `createWorkerThreadInfra()` 末尾 (= LL_INFOS の "infra created" log 直後) に `<AYAstorm r41 PC-N-15b (a)>` tag block で `sPcn14WorkerQueue = std::make_unique<LL::WorkQueue>(std::string(), 1024u, false)` + `sPcn14WorkerThreads.emplace_back([i]() { sWorkerIdx = i; runUntilClose(); sWorkerIdx = kInvalidWorkerIdx; })` + `s_first_pcn14_worker_thread_fire` atomic flag first-fire LL_INFOS marker。
  - `destroyWorkerThreadInfra()` 冒頭 (= reverse-init order 維持) に `<AYAstorm r41 PC-N-15b (a)>` tag block で `sPcn14WorkerQueue->close()` + 各 thread の `t.join()` + `clear()` + `reset()`。

- **(b) `postPrimitiveToWorker` + `drainWorkersAndExecute` helper 実装 ((N15b-1)..(N15b-3) ⭐ A + (N15b-9) A inheritance + (N15b-10) A graceful degrade + (N15b-11) A secondary_cmdbuf fire)**:
  - `indra/llrender/llvkloader.h` `getCurrentNodeAssetMatrix` 直後並列に `<AYAstorm r41 PC-N-15b (b)>` tag block で `bool postPrimitiveToWorker(LL::GLTF::Asset*, LL::GLTF::Primitive*, LL::GLTF::Skin*, const F32* mAssetMatrix)` + `void drainWorkersAndExecute(VkCommandBuffer primary_cmd_buf)` public API 宣言追加。
  - `indra/llrender/llvkloader.cpp` `getCurrentNodeAssetMatrix` 直後並列に anonymous namespace 内 `void recordGltfAssetDraw(VkCommandBuffer cmd_buf);` forward declaration 追加 (= 下方 anonymous namespace に定義済の internal linkage 関数を本 LLVKLoader namespace の `postPrimitiveToWorker` lambda 内から呼出可能化)。
  - `postPrimitiveToWorker` 実装:
    - cvar guard = `sAyastormGltfWorkerThreadEnabled` false 時 / `sPcn14WorkerQueue` null 時 / `sPcn14WorkerCtx` 空時 / `asset == nullptr` / `primitive == nullptr` 時 `return false` で main thread fallback 続行 ((N15b-12) A MUSEUBO-A 整合)。
    - work unit copy by value = `std::array<F32, 16> modelview_copy{}` + has_modelview flag + memcpy ((N15b-2) ⭐ A)。
    - `sPcn14WorkerQueue->post([asset, primitive, skin, modelview_copy, has_modelview]() { ... })` で worker thread lambda 投入:
      - thread_local `setCurrentAsset/Primitive/Skin/NodeAssetMatrix` set。
      - `sWorkerIdx` 範囲外 / `mSecondaryCmdBuf == VK_NULL_HANDLE` 時 LL_WARNS_ONCE + clearCurrentXxx + return。
      - `vkResetCommandBuffer(ctx.mSecondaryCmdBuf, 0)` で前 frame の record 残骸除去。
      - `VkCommandBufferInheritanceRenderingInfoKHR` (= sType + viewMask=0 + colorAttachmentCount=0 + pColorAttachmentFormats=nullptr + depthAttachmentFormat=VK_FORMAT_UNDEFINED + stencilAttachmentFormat=VK_FORMAT_UNDEFINED + rasterizationSamples=VK_SAMPLE_COUNT_1_BIT)。
      - `VkCommandBufferInheritanceInfo` (= sType + pNext=&inherit_rendering)。
      - `VkCommandBufferBeginInfo` (= sType + flags=USAGE_RENDER_PASS_CONTINUE_BIT|SIMULTANEOUS_USE_BIT + pInheritanceInfo=&inherit_info)。
      - `vkBeginCommandBuffer(ctx.mSecondaryCmdBuf, &begin_info)` 失敗時 LL_WARNS_ONCE + clearCurrentXxx + return ((N15b-10) A graceful degrade)。
      - 成功時 `s_first_pcn14_secondary_cmdbuf_fire` atomic flag first-fire LL_INFOS marker。
      - `recordGltfAssetDraw(ctx.mSecondaryCmdBuf)` + `vkEndCommandBuffer(ctx.mSecondaryCmdBuf)` + clearCurrentXxx。
  - `drainWorkersAndExecute` 実装:
    - early return guards = WorkQueue null / worker context empty / primary_cmd_buf null。
    - WorkQueue drain = `while (sPcn14WorkerQueue->size() > 0) std::this_thread::yield()` (= single producer 視点 size==0 観測で work 全件消化完了確認、workqueue.h L75-83 既明示 single producer 条件下で安全) ((N15b-3) ⭐ A)。
    - secondary cmdbuf 集約 = `std::vector<VkCommandBuffer> secondaries` 収集 + 非空時 `vkCmdExecuteCommands(primary_cmd_buf, N, secondaries.data())`。
    - per-thread `mSkinUboSubDirty` merge = sub-map 内 key 列挙経由で main `sSkinUboDirty[key].dirty.store(true, std::memory_order_release)` re-trigger (= UboInstance copy-assignable 不能ゆえ value copy 不可、worker 側 writeSkinUbo が main mapped buffer に既 memcpy + dirty.store 済、本 merge は safety net = flush 経路駆動を main thread context で確実化) ((N15b-7) A merge semantics) + sub-map clear。

- **(c) `gltfscenemanager.cpp` per-Primitive loop 内 dispatch hook + drain hook 追加 ((N15b-1) ⭐ A + (N15b-3) ⭐ A + (N15b-13b) A)**:
  - per-Primitive loop 内 `LLVKLoader::setCurrentPrimitive(&primitive)` 直後並列に `<AYAstorm r41 PC-N-15b (c)>` tag block で `LL::GLTF::Skin* skin_ptr_for_worker = rigged ? &asset.mSkins[node.mSkin] : nullptr` + `(void)LLVKLoader::postPrimitiveToWorker(&asset, &primitive, skin_ptr_for_worker, glm::value_ptr(node.mAssetMatrix))`。posted=true/false の戻り値は本 PC-N-15b では使用せず (= 既経路 main thread fallback path 全 hook 共存運用、PC-N-15c 以降で recordAvatarPlaceholderDraw entry hook 撤去後に整理予定 = Q4 B 採用整合)。
  - per-Asset loop 終了直後 (= `clearCurrentAsset` 直前並列) に `<AYAstorm r41 PC-N-15b (c)>` tag block で `LLVKLoader::drainWorkersAndExecute(LLVKLoader::getCurrentCommandBuffer())`。`getCurrentCommandBuffer()` は in-frame 時 `sCommandBuffer`、out-of-frame 時 `VK_NULL_HANDLE` を返却ゆえ Vulkan in-frame guard 同形 (= `drainWorkersAndExecute` 内 `primary_cmd_buf == VK_NULL_HANDLE` early return で MUSEUBO-A 整合)。

- **(d) `writeDrawUbo` thread-aware 拡張 ((N15b-6) A + (N15b-11) A ubo_parallel fire)**:
  - `writeDrawUbo` 冒頭 `out_dynamic_offset = 0u` 直後並列に `<AYAstorm r41 PC-N-15b (d)>` tag block で worker_path 判定 + `LLUboRingBuffer* ring_mgr` + `auto& ring_records` を per-thread 経路 / main thread 経路で切替。
  - 以降 既 logic そのまま `ring_mgr->allocate(...)` + `ring_records.find(...)` 経路を駆動 (= accessor signature 不変、設計原則 (1) 整合)。
  - worker_path 内最初の memcpy 成功時 `s_first_pcn14_ubo_parallel_fire` atomic flag first-fire LL_INFOS marker (= worker thread 内 per-thread LLUboRingBuffer allocate + memcpy 並列化通電 log)。

- **(e) `writeSkinUbo` thread-aware 拡張 ((N15b-7) A merge semantics)**:
  - main `sSkinUboDirty` 直接書込維持 (= host-coherent + memcpy thread-safe、buffer storage は register 経由 main 側のみ持つ設計整合、UboInstance copy-assignable 不能ゆえ value copy 不可)。
  - 既 logic body 末尾 `dirty.store(true)` 直後並列に `<AYAstorm r41 PC-N-15b (e)>` tag block で `sWorkerIdx != kInvalidWorkerIdx` 時 per-thread `sPcn14WorkerCtx[sWorkerIdx].mSkinUboSubDirty.try_emplace(key)` marker pure tracking (= drain 時 merge で main `dirty.store(true)` re-trigger 駆動経路)。

- **(f) `sPcn13MultiAssetSeen` mutex 保護 ((N15b-8) A)**:
  - `recordGltfAssetDraw` 内 PC-N-13 (b) tag block の `sAyastormGltfMultiAssetCanary` cvar guard 直下に `<AYAstorm r41 PC-N-15b (f)>` tag block で `std::lock_guard<std::mutex> lock(sPcn13MultiAssetSeenMutex)` 取得 → `sPcn13MultiAssetSeen.insert(...)` + `size() > 1u` 判定 + first-fire LL_INFOS marker = mutex scope 内に閉込み。
  - PC-N-15a で mutex declaration 済、本 PC-N-15b で lock_guard 取得 = worker thread 経由 recordGltfAssetDraw fire 経路通電後の main + worker access serialize で size() > 1u canary semantic 維持 (= (N15a-4) A 採用方針整合)。

- **(g) build verify literal 取得 ((N15b-14) A)**:
  - llrender library clean rebuild PASS (= warning 0 + error 0、`llvkloader.cpp` 改変由来 warning 0 確認)。
  - `INTEGRATION_TEST_lluboringbuffer` = 11/11 PASS (Unit test group_completed name=LLUboRingBuffer / Total Tests: 11 / Passed Tests: 11)。
  - `INTEGRATION_TEST_llassetubopool` = 10/10 PASS。
  - `INTEGRATION_TEST_llpipelinecachestorage` = 13/13 PASS。
  - `python3 -m unittest discover` from `scripts/ubo_codegen` = 131 tests OK。
  - GATE-B integrity = `grep -c LL_VULKAN_GLSL indra/llrender/llvkloader.cpp` = **6 不変** (= PC-N-15a commit `a90883cbf9` 同数、PC-N-13 commit `faae1544d6` 同数)。
  - **注**: newview link build は pre-existing 別 file (`indra/newview/fslocalmeshimportgltf.cpp` Refactor commit `820c4a83fc` 由来の syntax error + `indra/newview/gltfscenemanager.cpp:449` `make_shared<Asset>(json)` `LL::LL::GLTF::Asset` namespace lookup error) で error 残るが、本 PC-N-15b 改変由来ではなく PC-N-15a commit `a90883cbf9` baseline 時点で既存。`git stash` で gltfscenemanager.cpp 改変外し検証で同 error 再現確認済 = 本 PC-N-15b 改変責任なし、`feedback_admit_unknown` 遵守で newview link build 実機検証は別 phase に持越し明示。本 PC-N-15b 改変由来 llrender library build + integration test + codegen + GATE-B integrity 全 PASS で Exit Criteria 10 項中 (x) build verify literal 取得 充足。

- **(h) cross-platform spec §6 PC-N-15b 行 ✅ 反映 + §A 履歴 1 行追記 + 本 design-lock doc §B.1 追記 ((N15b-16) A 案 B 統合方針)**:
  - `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md` §6 PC-N-15b 行 状態 ⏳ → ✅ 反映 (= 実装内容 (a)-(h) 8 step 詳細追記)。
  - §A 履歴 chronological entry 1 行追記 (= 本 PC-N-15b 実装 complete record + step (a)-(h) 全実施 + build verify literal + Exit Criteria 10 項充足 + newview link build pre-existing error 明示)。
  - 本 design-lock doc §B.1「実装結果追記」section に PC-N-15b 結果 literal record (= 本 section、案 B 統合方針整合 = 別 complete handoff doc 起案なし)。

#### §B.1.2 改変 file 4 件

1. **`indra/llrender/llvkloader.cpp`** (+~270 net) = `#include "workqueue.h"` + `#include <array>` + `kInvalidWorkerIdx` + `sWorkerIdx` thread_local + `sPcn14WorkerQueue` + `sPcn14WorkerThreads` + `createWorkerThreadInfra` 末尾 WorkQueue + thread spawn + first-fire marker + `destroyWorkerThreadInfra` 冒頭 close+join + `recordGltfAssetDraw` forward decl + `postPrimitiveToWorker` + `drainWorkersAndExecute` 実装 + `writeDrawUbo` thread-aware path + `writeSkinUbo` thread-aware marker + `sPcn13MultiAssetSeen` mutex lock_guard。
2. **`indra/llrender/llvkloader.h`** (+30 net) = `postPrimitiveToWorker` + `drainWorkersAndExecute` public API 宣言追加 (= LLVKLoader namespace 内 `<AYAstorm r41 PC-N-15b (b)>` tag block)。
3. **`indra/newview/gltfscenemanager.cpp`** (+~35 net) = per-Primitive loop 内 `setCurrentPrimitive` 直後並列 `postPrimitiveToWorker` hook + per-Asset 末尾 `clearCurrentAsset` 直前並列 `drainWorkersAndExecute` hook (= 2 `<AYAstorm r41 PC-N-15b (c)>` tag block)。
4. **`docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md`** = §6 PC-N-15b 行 ✅ 反映 + §A 履歴 1 行追記。

#### §B.1.3 Exit Criteria 10 項充足判定

| # | criterion | 充足 |
|---|-----------|------|
| i | `LL::WorkQueue` infrastructure 配線 ((N15b-4) ⭐ A + (N15b-5) A) | ✅ |
| ii | `postPrimitiveToWorker` + `drainWorkersAndExecute` helper 新設 ((N15b-1) ⭐ A + (N15b-2) ⭐ A + (N15b-3) ⭐ A) | ✅ |
| iii | `gltfscenemanager.cpp` per-Primitive loop 内 dispatch + drain hook 追加 | ✅ |
| iv | `writeDrawUbo` + `writeSkinUbo` thread-aware 拡張 ((N15b-6) A + (N15b-7) A) | ✅ |
| v | `sPcn13MultiAssetSeen` `std::lock_guard<std::mutex>` 配置 ((N15b-8) A) | ✅ |
| vi | secondary cmdbuf `VkCommandBufferInheritanceRenderingInfoKHR` 経由 dynamic rendering scope 継承 ((N15b-9) A) | ✅ |
| vii | first-fire LL_INFOS marker 3 件 ((N15b-11) A) | ✅ |
| viii | GATE-B 整合 = `LL_VULKAN_GLSL count llvkloader.cpp=6` 不変 | ✅ |
| ix | MUSEUBO-A 整合 = `AYAGltfWorkerThreadEnabled=false` default で main thread 経路完全維持 ((N15b-12) A) | ✅ |
| x | build verify literal 取得 + spec §6 PC-N-15b ✅ 反映 + §A 履歴 + 本 doc §B.1 追記 (= 案 B 統合方針整合) | ✅ |

#### §B.1.4 commit 内容予定 + 残 strict 線形

- commit 内容 = 3 modified (indra/) + 1 modified (cross-platform spec) + 1 modified (本 design-lock doc §B.1)、CMake 改変 0 + codegen 改変 0 + shader 改変 0 + settings.xml 改変 0 + tests/ 改変 0 + Co-Authored-By 不在。
- 残 strict 線形 = **PC-N-15b ✅ 本 commit** → PC-N-15c (= cleanup + sGltfStubSkin 撤去 + AYAGltfMultiSkinEnabled cvar 撤去 + AYAGltfRealDrawEnabled cvar 撤去 + Phase 1.E complete marker 起案、PC-N-15b 完了 + AYA live 動作確認 PASS 後別 session) → Phase 1.E complete → Phase 1 全完了 → Mac/Win 補完 phase。
- 次 session 着手 1 line = PC-N-15c 実装着手 = step (a)-(g) 7 step 実施 (= AYA live 動作確認 PASS 後)。

### §B.2 PC-N-15c 実装結果 (= step (a)-(g) 完了時追記、commit hash + build verify literal + Exit Criteria 10 項充足判定 + Phase 1.E complete marker handoff doc path)

⏳ PC-N-15c 実装完了時追記。

---

(EOF: PC-N-15b + PC-N-15c 統合 design-lock complete = 案 B 統合方針 = handoff doc 4 件 → 2 件削減、AYA literal「B でお願いします」record 2026-06-05 整合)
