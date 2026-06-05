# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.E **PC-N-15a design-lock complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: Phase 1.E 内 **5th sub-step = PC-N-15a = worker thread infra design-lock** (= per-thread `LLUboRingBuffer` instance + per-thread VkCommandPool + secondary VkCommandBuffer + thread_local accessor + 2 cvar の **storage 配線のみ**、worker thread launch + 並列実行は PC-N-15b 持越し) の design-lock phase 完了 marker = ambiguity (N15a-1)..(N15a-13) 13 件 全 AYA literal「OK」record (2026-06-05) + 実装計画 (a)-(e) 5 step 分解 + Exit Criteria 9+10 項明文化。`indra/` 改変 0 件 (= `feedback_design_phase_no_code_write` 整合)。実装は PC-N-15a 別 session、PC-N-15b/PC-N-15c は更に別 sub-step。

> **本 doc 位置付け**: PC-N-15a 詳細 design-lock。PC-N-14 design-lock (= `handoff-...-phase1-e-pc-n-14-design-lock.md`、commit `1a83070f31`) baseline 上に、**PC-N-14 段階で「(N14-7) B per-thread sub-ring 化」approve 済の path を実装着手前 investigation で blocker 発見 + (E-14) B literal 維持の唯一 path として α 案 (`LLUboRingBuffer` per-thread instance 化) に pivot、その implementation を更に 3 sub-step (PC-N-15a / PC-N-15b / PC-N-15c) に分解した第 1 段 = infra 配線のみ**。`feedback_ubo_migration_one_at_a_time` 厳格遵守で各 sub-phase build verify + 段階通電 + issue 局所化。
>
> **⭐ 重大 design pivot (= PC-N-15 着手前 investigation 経由 (N14-7) re-decide)**: PC-N-14 design-lock (N14-7) B literal「per-thread sub-ring buffer + main 集約 phase で merge」を investigation で具体化したところ、`LLUboRingBuffer` (`indra/llcommon/lluboringbuffer.h`) は **per-instance 独立 storage で thread-safe 動作可** (= chunk state は instance 内完結、copy/assignment delete で unique_ptr 経由のみ、constructor が factory+destroyer 注入型ゆえ N instance 独立生成可) と判明、ただし consumer-side `sDrawUboRingBufferRecords` map (`llvkloader.cpp:422`) は共有ゆえ per-thread sub-map 化必要。これが (N15a-2) ⭐ 確定 + AYA literal「OK」record (2026-06-05) で α 案 path 確定。β 案 (= mutex 保護 single instance) は AYA 指摘 (2026-06-05)「短い寿命な処理だけプロセス化できると読めるんだけどそれではあまり意味がない」literal で reject = mutex 直列化で worker thread 並列度が allocate 部分でゼロ化、(E-14) B literal「UBO write + cmdbuf 両方並列化」を見かけ上充足するが実質 cmdbuf 並列のみ = scope shrink 抵触。

---

## §0. 本 session 着手契機 + literal scope record

**契機**: AYA 指示「r41 Phase 1.E PC-N-15 実装着手お願いします。直前 commit = `1a83070f31` (PC-N-14 design-lock complete = worker thread design = per-Primitive UBO write + cmdbuf record 並列化 design = 20 件 ambiguity AYA literal「OK」record + 実装計画 (a)-(k) 11 step 分解)。task = Phase 1.E 内 5th = 最終 sub-step = worker thread 実装 + cleanup + Phase 1.E complete marker 起案。実装 phase ゆえ feedback_design_phase_no_code_write 解除、indra/ 改変 OK。」literal 受領 (2026-06-05)。

**着手前 investigation 経由 pivot**: Claude が PC-N-14 design-lock §A「`sDrawUboRingBufferMgr` 内部 thread-safety **未確認**」flag を受けて explore agent 4 件並列調査 → `LLUboRingBuffer` は per-instance 化可能だが **`LLUboRingBuffer::allocate` 自体は mutex/lock-free 機構なし、複数 thread 同時 allocate 不可、per-thread instance 化が唯一の lock-free path** と判明。これを AYA に surface (= 3 案 α/β/γ 提示)、AYA literal「B でいきましょう」record (= 「B 案 = PC-N-15 を 3 sub-phase 分解」採用 + α 案 = per-thread instance refactor 選択) 受領 (2026-06-05)。続いて PC-N-15a 単独で確定すべき ambiguity 13 件 batch 提示 → AYA literal「OK」record 一括受領 (2026-06-05) で本 PC-N-15a design-lock doc 起案。

**PC-N-15a literal scope** (= PC-N-14 step (a)-(k) 11 step のうち本 sub-step 担当 = (N15a-1) A 採用、5 項):

1. **2 cvar 新設 (= step (a))** = `AYAGltfWorkerThreadEnabled` (Boolean default=0 Persist=1) + `AYAGltfWorkerThreadCount` (U32 default=0 Persist=1、0=auto)。配置は `AYAGltfMultiAssetCanary` 直後並列 ((N14-11) A 既確定)。
2. **include + storage 宣言 (= step (b))** = `llvkloader.cpp` 冒頭 include 追加 (`<thread>`/`<atomic>`/`<vector>`/`<mutex>`) + anonymous namespace 内に `PcN14WorkerContext` struct + `std::vector<PcN14WorkerContext> sPcn14WorkerCtx` + `std::atomic<U32> sPcn14WorkerCount` storage 配線 ((N14-4) A LL::WorkQueue header include は PC-N-15b で `LL::WorkQueue::post` 実 use 時に追加、PC-N-15a infra のみゆえ最小限)。
3. **`thread_local` 化 4 件 + mutex 保護 1 件 (= step (c))** = `sCurrentAsset` (line 668) / `sCurrentSkin` (line 669) / `sCurrentPrimitive` (line 679) / `sCurrentNodeAssetMatrix` (line 696) の 4 件に `thread_local` 修飾子追加 ((N14-8) A 修正版 = (N15a-10) A 確定) + `sPcn13MultiAssetSeen` (line 707) は **main thread 単一維持 + `std::mutex sPcn13MultiAssetSeenMutex` 新設で保護** ((N15a-6) B = (N14-8) revisit、PC-N-13 (b) canary semantics 維持)。
4. **per-thread `LLUboRingBuffer` N instance + per-thread sub-map storage (= step (d))** = `PcN14WorkerContext` 内に `std::unique_ptr<LLUboRingBuffer> mDrawUboRingBuffer` + `std::unordered_map<LLUboRingBuffer::BufferHandle, DrawUboRingBufferRecord> mDrawUboSubRecords` + `std::unordered_map<UboSkinKey, UboInstance, UboSkinKeyHash> mSkinUboSubDirty` 配線 ((N15a-2) ⭐ A + (N15a-3) A + (N15a-4) A + (N15a-5) A)。main 集約 phase の merge 経路は PC-N-15b 持越し。
5. **initVulkan / shutdownVulkan lifecycle (= step (e))** = `initVulkan()` 内に新 helper `createWorkerThreadInfra()` 呼出 ((N15a-7) B always at init、`AYAGltfWorkerThreadCount` cvar 読込 + auto 決定 + N 件 storage allocate + 各 worker の VkCommandPool create + secondary VkCommandBuffer alloc + LLUboRingBuffer instance create + initialize)、`shutdownVulkan()` 内に対称 destroy ((N15a-8) A standard reverse-init order = LLUboRingBuffer destroy → vkFreeCommandBuffers → vkDestroyCommandPool → sPcn14WorkerCtx clear)。

**Phase 境界**: PC-N-15a 完了 = Phase 1.E 内 5th sub-step **infra 配線** 完了 = worker thread storage 確保 + thread_local 化 + cvar 配線。**worker thread launch + secondary cmd_buf record + `vkCmdExecuteCommands` 集約 + drain** は PC-N-15b 持越し。**`sGltfStubSkin` sentinel cleanup + Phase 1.E complete marker** は PC-N-15c 持越し。`AYAGltfWorkerThreadEnabled=true` ON 切替時も PC-N-15a 完了直後段階では **infra 起動のみで描画経路は変化しない** (= main thread linear sequential record 維持) ((N15a-1) A literal)。

---

## §1. 必読 1 件 + pinpoint reference + background reference

### §1.1 必読 1 件 (= 次 session = PC-N-15a 実装 phase 着手前)

1. **本 PC-N-15a design-lock doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-e-pc-n-15a-design-lock.md`

### §1.2 pinpoint reference 12 件 (= 実装 phase で必要分のみ Read、`feedback_handoff_minimal_pre_req_read` 整合)

1. **`LLUboRingBuffer` class 全体**: `indra/llcommon/lluboringbuffer.h` (= 132 行 / Vulkan device 非依存 / copy delete / factory+destroyer std::function 注入 / `kInitialSizeMB=4`/`kMaxSizeMB=16`/`kFramesInFlight=3`/`kDefaultAlignment=256` / `allocate()` thread-unsafe = per-instance 独立 chunk state mutate) — PC-N-15a (d) per-thread N instance 化対象、constructor signature = `LLUboRingBuffer(BufferAllocator, BufferDestroyer, initial_size_mb=4, max_size_mb=16, alignment=256)`
2. **`sDrawUboRingBufferRecords` + `sDrawUboRingBufferMgr` storage**: `indra/llrender/llvkloader.cpp:422-424` = `std::unordered_map<LLUboRingBuffer::BufferHandle, DrawUboRingBufferRecord> sDrawUboRingBufferRecords` + `std::unique_ptr<LLUboRingBuffer> sDrawUboRingBufferMgr` (= main thread instance 既配線) — PC-N-15a (d) `PcN14WorkerContext` 内に sub-map + per-thread instance として並列配置
3. **`createDrawUboRingBuffer` 関数**: `llvkloader.cpp:1438-1519` = main thread instance 既配線 example (= cvar 読込 + factory closure + destroyer closure + make_unique + initialize) — PC-N-15a (e) `createWorkerThreadInfra` 内で N instance 同 pattern 流用、ただし factory/destroyer は per-instance closure で per-thread sub-map に書込
4. **`createCommandPool` 関数**: `llvkloader.cpp:1083-1113` = main thread `sCommandPool` + primary `sCommandBuffer` 既配線 example (= `VkCommandPoolCreateInfo` flags=`VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT` / `vkCreateCommandPool` / `VkCommandBufferAllocateInfo` level=`VK_COMMAND_BUFFER_LEVEL_PRIMARY` / `vkAllocateCommandBuffers`) — PC-N-15a (e) `createWorkerThreadInfra` 内で per-thread `VkCommandPool` + secondary `VkCommandBuffer` を N 件作成、level は `VK_COMMAND_BUFFER_LEVEL_SECONDARY` ((N14-1) ⭐ A 整合)
5. **`shutdownVulkan` 内 sCommandPool destroy**: `llvkloader.cpp:4047` (関数 entry) + `:4303-4307` (destroy block) = `vkDestroyCommandPool(sDevice, sCommandPool, nullptr)` + `sCommandPool = VK_NULL_HANDLE` + `sCommandBuffer = VK_NULL_HANDLE` — PC-N-15a (e) 直後並列に `destroyWorkerThreadInfra()` 呼出 + 内部で N worker の LLUboRingBuffer destroy → vkFreeCommandBuffers → vkDestroyCommandPool → sPcn14WorkerCtx clear (reverse-init order)
6. **`sSkinUboDirty` storage**: `llvkloader.cpp:562` = `std::unordered_map<UboSkinKey, UboInstance, UboSkinKeyHash> sSkinUboDirty` + `shutdownVulkan` 内 `:4398-4401` で `destroyUboInstanceBuffers` 経由 destroy + `clear()` — PC-N-15a (d) `PcN14WorkerContext::mSkinUboSubDirty` で per-thread sub-map として並列配置、main 集約 phase merge は PC-N-15b 持越し
7. **4 file-static accessor (`sCurrentAsset`/`sCurrentSkin`/`sCurrentPrimitive`/`sCurrentNodeAssetMatrix`)**: `llvkloader.cpp:668`/`:669`/`:679`/`:696` = `LL::GLTF::Asset*`/`LL::GLTF::Skin*`/`LL::GLTF::Primitive*`/`const F32*` 各 nullptr 初期化 main thread 専有 — PC-N-15a (c) `thread_local` 修飾子追加 + comment 「main thread 専有」→「worker thread 内 thread_local 整合」更新、signature 不変
8. **`sPcn13MultiAssetSeen` storage + access site**: `llvkloader.cpp:698-708` (declaration block + `std::unordered_set<const void*>`) + `:6174-6205` (recordGltfAssetDraw 内 PC-N-13 (b) block で `insert` + `size() > 1u` 判定 + first-fire `LL_INFOS` marker) — PC-N-15a (c) **`thread_local` 化 reject**、`std::mutex sPcn13MultiAssetSeenMutex` 新設で保護 ((N15a-6) B = (N14-8) revisit)、PC-N-13 (b) canary semantics (= 「2 asset 以上同時描画通電検出」) 維持。実 use site の lock_guard 配置は PC-N-15b で worker thread context から fire 経路を実装する際に追加 (= PC-N-15a 段階では worker thread launch なしゆえ既 main thread context での access のみ、mutex declaration を追加するが lock 未取得状態でも behavioral regression なし)
9. **`AYAGltfMultiAssetCanary` cvar 配置**: `settings.xml:10532-10542` = Phase 1.E cvar group 末尾 — PC-N-15a (a) `AYAGltfWorkerThreadEnabled` + `AYAGltfWorkerThreadCount` を直後並列に配置 ((N14-11) A 既確定)
10. **U32 type cvar XML 書式**: `settings.xml:10553-10556` (`PluginInstancesLow` 例) = `<key>Type</key><string>U32</string><key>Value</key><integer>4</integer>` — PC-N-15a (a) `AYAGltfWorkerThreadCount` U32 cvar の書式参考
11. **`initVulkan` 関数 entry + sCommandPool create 呼出 site**: `llvkloader.cpp:3598` (`initVulkan` entry) + `createCommandPool` 呼出 site (= initVulkan 内、grep 確認予定) — PC-N-15a (e) `createCommandPool()` 呼出直後並列に `createWorkerThreadInfra()` 呼出 hook 配置
12. **anonymous namespace boundary**: `llvkloader.cpp:67` (namespace { 開始) + `:3596` (} 終了) = file-static storage 配置領域 — PC-N-15a (b) 内側に `PcN14WorkerContext` struct + storage + 2 cvar `LLCachedControl` 宣言を `<AYAstorm r41 PC-N-15a (a)>` tag block で配置

### §1.3 background reference 4 件 (= Phase 1.E 全体 + 設計原則)

- **PC-N-14 design-lock**: `handoff-...-phase1-e-pc-n-14-design-lock.md` (commit `1a83070f31`) = (N14-1)..(N14-20) 20 件 ambiguity AYA OK record + (N14-7) B literal が本 PC-N-15a の implementation source
- **Phase 1.E decomposition design-lock**: `handoff-...-phase1-e-decomposition-design-lock.md` §4.4 = PC-N-14/15 想定 ambiguity 5 件 + (E-9) B + (E-14) B 整合確認
- **PC-N-13 complete handoff**: `handoff-...-phase1-e-pc-n-13-complete.md` (commit `faae1544d6`) = baseline (= PC-N-8 (f) real Asset path + PC-N-11 (a) / PC-N-12 (a) / PC-N-13 (a) / (b) 既配線 tag block + `sPcn13MultiAssetSeen` 既配線確認)
- **design 09 phase roadmap**: `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` = Phase 1 全体 roadmap + worker thread 並列化方針 source ((E-9) B literal「design 09 参照」)

---

## §2. 現状調査結果 (= PC-N-14 design-lock baseline + α 案 path 確認)

### §2.1 `LLUboRingBuffer` per-instance 独立化可能性確認 (= (N15a-2) ⭐ 核心)

| # | 確認項目 | 結論 |
|---|---------|-----|
| 1 | header 配置 | `indra/llcommon/lluboringbuffer.h` (PC-N-14 design-lock §1.2 reference 13 で「`llrender/`」と記載していた箇所は `llcommon/` の誤り = `feedback_admit_unknown` で本 PC-N-15a design-lock §2 で訂正) |
| 2 | Vulkan device 依存 | **なし** (= header `:36-40` literal「Vulkan device 非依存の bookkeeping algorithm = 実 VkBuffer 生成/破棄は caller injected callback (BufferAllocator / BufferDestroyer) 経由」) — per-instance 化に追加 device 経路不要 |
| 3 | copy/assignment | `delete` (header `:76-77`) — `unique_ptr` 経由のみ、N instance 化は `std::vector<std::unique_ptr<LLUboRingBuffer>>` 経路で対応 |
| 4 | constructor signature | `LLUboRingBuffer(BufferAllocator, BufferDestroyer, initial_size_mb=4, max_size_mb=16, alignment=256)` (header `:69-73`) — factory+destroyer std::function 注入のみ、追加引数なし |
| 5 | internal state mutate | `mBuffer`/`mCurrentSizeBytes`/`mFrameIndex`/`mActiveChunk`/`mChunkBytesUsed`/`mChunksConsumedThisFrame`/`mInitialized` (header `:122-128`) = per-instance member ゆえ instance 独立性で mutate 隔離 — **mutex 不要で thread-safe 達成 (= 各 worker thread が自 instance のみ touch する前提)** |
| 6 | factory closure pattern | 既 `createDrawUboRingBuffer` (`llvkloader.cpp:1438-1519`) で `sDrawUboRingBufferRecords` 単一 map に書込む実装あり — PC-N-15a (d) では `PcN14WorkerContext::mDrawUboSubRecords` per-thread sub-map に書込む factory closure を per-instance 生成 |
| 7 | memory overhead 試算 | N=4 worker で initial=4 MB × 4 = 16 MB、max=16 MB × 4 = 64 MB (= main thread 既 instance 含めて 5 instance) — acceptable ((N15a-7) B always at init 採用根拠) |

**結論**: per-thread N instance 化は **`LLUboRingBuffer` class 改変 0 件で実現可** = constructor を N 回呼ぶだけで chunk state は完全独立 = lock-free worker thread allocation 達成。

### §2.2 既存 main thread storage 配置 (= PC-N-15a 並列追加対象 site)

| # | storage | line | 改変方針 |
|---|--------|------|---------|
| 8 | `sDrawUboRingBufferRecords` | `llvkloader.cpp:422-423` | PC-N-15a (b) で **maintain** (= main thread 既 instance 用)、worker 用 sub-map は `PcN14WorkerContext::mDrawUboSubRecords` として並列追加 |
| 9 | `sDrawUboRingBufferMgr` | `llvkloader.cpp:424` | PC-N-15a (b) で **maintain** (= main thread 既 instance 用)、worker 用 instance は `PcN14WorkerContext::mDrawUboRingBuffer` として並列追加 |
| 10 | `sSkinUboDirty` | `llvkloader.cpp:562` | PC-N-15a (b) で **maintain** (= main thread 既配線)、worker 用 sub-map は `PcN14WorkerContext::mSkinUboSubDirty` として並列追加 |
| 11 | `sCommandPool` / `sCommandBuffer` | `llvkloader.cpp:80-81` | PC-N-15a (b) で **maintain** (= main thread primary 既配線)、worker 用 pool/secondary cmd_buf は `PcN14WorkerContext::mCommandPool` + `::mSecondaryCmdBuf` として並列追加 |
| 12 | `sCurrentAsset` (PC-7γ-2 tag) | `llvkloader.cpp:668` | PC-N-15a (c) で `thread_local` 修飾子追加、comment 更新 |
| 13 | `sCurrentSkin` (PC-7γ-2 tag) | `llvkloader.cpp:669` | 同上 |
| 14 | `sCurrentPrimitive` (PC-N-8 (e) tag) | `llvkloader.cpp:679` | 同上 |
| 15 | `sCurrentNodeAssetMatrix` (PC-N-12 (c) tag) | `llvkloader.cpp:696` | 同上 |
| 16 | `sPcn13MultiAssetSeen` (PC-N-13 (b) tag) | `llvkloader.cpp:707` | PC-N-15a (c) で `thread_local` **化 reject** + `std::mutex sPcn13MultiAssetSeenMutex` 新設で保護 ((N15a-6) B、(N14-8) revisit、PC-N-13 (b) canary semantics 維持) |

### §2.3 `initVulkan` / `shutdownVulkan` 配置確認

| # | 関数 | line | PC-N-15a 改変 |
|---|------|------|-------------|
| 17 | `initVulkan()` entry | `llvkloader.cpp:3598` | PC-N-15a (e) で `createCommandPool()` 呼出 site 直後並列に `createWorkerThreadInfra()` 呼出 hook 追加 |
| 18 | `createCommandPool()` | `llvkloader.cpp:1083-1113` | PC-N-15a (e) 新 helper `createWorkerThreadInfra()` を直後 sibling として配置 (= per-thread VkCommandPool + secondary VkCommandBuffer + LLUboRingBuffer instance lifecycle) |
| 19 | `shutdownVulkan()` entry | `llvkloader.cpp:4047` | PC-N-15a (e) で `vkDestroyCommandPool(sDevice, sCommandPool, ...)` 直後並列に `destroyWorkerThreadInfra()` 呼出 hook 追加 |
| 20 | sCommandPool destroy block | `llvkloader.cpp:4303-4307` | 同上、reverse-init order = LLUboRingBuffer destroy → vkFreeCommandBuffers → vkDestroyCommandPool → sPcn14WorkerCtx clear |

### §2.4 既配線 tag block 整合 (= PC-N-15a 影響なし、PC-N-15b で active 化)

| # | tag block | line | PC-N-15a 段階での状態 |
|---|-----------|------|---------------------|
| 21 | PC-N-8 (f) outer block | `llvkloader.cpp:5906-6147` | **不変** (= worker thread launch 配線は PC-N-15b 持越し) |
| 22 | PC-N-11 (a) / PC-N-12 (a) / PC-N-13 (a) / PC-N-13 (b) inner block | 既配線位置 | **不変** (= worker thread context での実行は PC-N-15b 段階、PC-N-15a 段階では main thread 経路のみ) |

### §2.5 設計原則 (2) Core プロセス分散実現整合 (= PC-N-15a 単独段階)

| # | 設計原則項目 | PC-N-15a 段階確認 |
|---|------------|----------------|
| 23 | per-thread `LLUboRingBuffer` instance | (d) で storage 配線、PC-N-15b で actual allocate 経路通電 |
| 24 | per-thread VkCommandPool + secondary cmd_buf | (e) で lifecycle 配線、PC-N-15b で actual record 経路通電 |
| 25 | thread_local accessor 4 件 | (c) で 修飾子追加、PC-N-15b で worker thread context から fire 経路通電 |
| 26 | `recordGltfAssetDraw` signature 不変 | PC-N-15a (a)-(e) 全 step で recordGltfAssetDraw 内 改変 0 件、設計原則 (1) Upstream OpenGL 取り込みやすさ完全充足 |

---

## §3. ambiguity (N15a-1)..(N15a-13) 13 件 AYA literal「OK」record (2026-06-05) + 採用根拠

### §3.1 scope 分配系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N15a-1) | PC-N-15a literal scope 分配 | **A**: PC-N-14 11 step のうち (a)+(b)+(c)+(d)+(g) infra のみ、worker launch 経路なし | OK (2026-06-05) | (E-14) B「両方並列化」literal は PC-N-15a+b で達成、PC-N-15a 単独では infra のみで build verify + runtime regression 0 確認可能、PC-N-15b で worker launch 配線 + first-fire marker 3 件 fire 経路通電 |

### §3.2 ⭐ critical: per-thread `LLUboRingBuffer` 構造系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N15a-2) ⭐ | per-thread `LLUboRingBuffer` instance 化方針 | **A**: 完全独立 N instance (= constructor callback 既経路で N 件 instantiate、各 instance の chunk が完全独立 = lock-free 達成) | OK (2026-06-05) | mutex/atomic 不要 = worker thread allocate 並列度 100%、`LLUboRingBuffer` class 改変 0 件 = scope 最小、memory overhead = N 件 ring buffer (推定 N=4 で initial 16 MB / max 64 MB) acceptable、β案 (mutex 保護 single instance) は AYA literal「短い寿命な処理だけ並列化で意味なし」 reject 経由本 path 確定 |
| (N15a-3) | per-thread `sDrawUboRingBufferRecords` sub-map 構造 | **A**: `std::vector<std::unordered_map<...>>` (= `PcN14WorkerContext::mDrawUboSubRecords` 内に格納、index = worker_id) | OK (2026-06-05) | main 集約 phase で iterate 容易 (= `for (auto& ctx : sPcn14WorkerCtx) { ... }`)、N 変動対応、`thread_local std::unordered_map<...>` は accessor 経由 per-thread storage 取得困難、N 件名前付き個別 map は N 動的決定時に困難 |
| (N15a-4) | `LLUboRingBuffer` constructor callback per-instance 共有性 | **A**: 同一 Vulkan device callback 同型を全 instance で別 closure 生成 (= BufferAllocator / BufferDestroyer 実装は同一 vmaCreateBuffer / vmaDestroyBuffer 経路だが、closure capture する sub-map は per-instance) | OK (2026-06-05) | Vulkan device + VMA allocator は単一 instance 共有 = (N14-6) A VMA default thread-safe locking 信任、records sub-map は per-instance capture で thread-safe |
| (N15a-5) | `sSkinUboDirty` per-thread 分散方針 | **A**: per-thread sub-map (= `PcN14WorkerContext::mSkinUboSubDirty` `std::unordered_map<UboSkinKey, UboInstance, UboSkinKeyHash>`) + main 集約 phase merge は PC-N-15b 持越し | OK (2026-06-05) | UBO write 並列度確保、同一 Skin が複数 worker から書込まれる場合の dirty range union は PC-N-15b 内 ambiguity (= (N15b-?) 新規) として再投出予定、PC-N-15a 段階では storage 配線のみゆえ semantic ambiguity 未解決でも build verify 完走可能 |

### §3.3 既配線 storage 取扱系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N15a-6) ⭐ | `sPcn13MultiAssetSeen` 取扱 ((N14-8) revisit、(N14-21) 新規 ambiguity 解消) | **B**: **main thread 単一維持 + `std::mutex sPcn13MultiAssetSeenMutex` 新設で保護** | OK (2026-06-05) | `thread_local` 化すると同一 2 asset が別 worker thread に分散時に **canary 失火** (= PC-N-13 (b) 「2 asset 以上同時描画通電検出」semantics 破壊)、debug-only ゆえ contention 影響なし、main 単一 set で全 asset を 1 視点から観測 = canary 設計目的維持。PC-N-15a (c) 段階では mutex 宣言追加のみ、lock_guard 取得は PC-N-15b で worker thread context から PC-N-13 (b) block fire 経路を実装する際に追加 |

### §3.4 lifecycle 系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N15a-7) | initVulkan 内 per-thread storage 確保 timing | **B**: **always at init** (= `AYAGltfWorkerThreadEnabled` 状態に関わらず init 時 N 件 storage 確保) | OK (2026-06-05) | code 単純、disable 時 overhead は memory 消費のみ (= N=4 で initial 16 MB / max 64 MB、acceptable)、lazy は cvar 変更時の thread pool restart 経路複雑、initVulkan 1 度きり呼出ゆえ ON/OFF 切替時の dynamic re-init は viewer 再起動経由が UX 妥当 |
| (N15a-8) | shutdownVulkan cleanup 順序 | **A**: `LLUboRingBuffer` destroy → `VkCommandBuffer` free → `VkCommandPool` destroy → `sPcn14WorkerCtx` clear | OK (2026-06-05) | Vulkan dependency 順序標準 reverse-init order、LLUboRingBuffer 内部の BufferDestroyer callback が vmaDestroyBuffer 呼ぶゆえ VMA allocator 有効性確保で LLUboRingBuffer destroy 先行必須 |

### §3.5 Phase 1.E sub-step 拡張系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N15a-9) | Phase 1.E sub-step 拡張表記 | **A**: PC-N-15 → PC-N-15a / PC-N-15b / PC-N-15c 3 件に分割、cross-platform spec §6 にも 3 行追加 | OK (2026-06-05) | (E-10) B literal「最終 sub-step = 実装 + cleanup + Phase 1.E complete marker 統合」は PC-N-15c に集約、PC-N-15a/b は前段、Phase 1.E 総 sub-step は 5 (PC-N-11..15) → 7 (PC-N-11/12/13/14/15a/15b/15c) に拡張だが各 sub-step scope 小型化で `feedback_ubo_migration_one_at_a_time` 厳格遵守 |

### §3.6 改変規模 + Exit Criteria + build verify 系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N15a-10) | PC-N-15a 想定改変 file 件数 | **A**: **5 件** = (1) `indra/llrender/llvkloader.cpp` + (2) `indra/newview/app_settings/settings.xml` + (3) `cross-platform spec §6 PC-N-15a 行追加 + §A 履歴 1 行` + (4) **本 PC-N-15a design-lock doc 起案** + (5) **PC-N-15a complete handoff doc 起案** (= 別 session 実装後) | OK (2026-06-05) | PC-N-11/12/13/14 同形 5 件 pattern |
| (N15a-11) | Exit Criteria 項目数 | **A**: design-lock phase **9 項** + 実装 phase **10 項** (PC-N-6..14 同形 pattern) | OK (2026-06-05) | 既配線 pattern 踏襲 |
| (N15a-12) | build verify scope | **A**: PC-N-6..14 同形 = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 + GATE-B integrity `LL_VULKAN_GLSL count llvkloader.cpp=6` 不変 (= PC-N-14 design-lock commit `1a83070f31` 同数想定) | OK (2026-06-05) | 既配線 build verify pattern 踏襲、PC-N-15a 実装 phase で literal 検証取得予定 |
| (N15a-13) | PC-N-15a 失敗時 escalation 経路 | **A**: PC-N-15a 内 fix or 別 sub-step (= PC-N-15a.1) 起案 = AYA 判断 | OK (2026-06-05) | PC-N-11/12/13/14 同形 escalation pattern、infra 配線のみゆえ失敗想定低 (= LLUboRingBuffer 既実装の N instance 化 + VkCommandPool 既 pattern 複製) |

---

## §4. 実装計画 (a)-(e) 5 step (= PC-N-15a 別 session で着手)

> **注**: 本 §4 は **PC-N-15a 実装 phase 用 step 分解 + 想定 code diff example** = 本 PC-N-15a design-lock phase は `indra/` 改変 0 件、実装は PC-N-15a 別 session 別途着手 (= `feedback_design_phase_no_code_write` + `feedback_ubo_migration_one_at_a_time` + (N15a-1) A 厳格遵守)。

### §4.1 step (a) — `AYAGltfWorkerThreadEnabled` + `AYAGltfWorkerThreadCount` cvar 新設 (settings.xml)

`indra/newview/app_settings/settings.xml` の既 `AYAGltfMultiAssetCanary` cvar 直後並列 ((N14-11) A) で `AYAGltfWorkerThreadEnabled` cvar 追加、その直後並列で `AYAGltfWorkerThreadCount` cvar 追加。XML 書式は (N15a-10) A の (2) で改変対象。

**想定 XML diff** (= 既 PC-N-13 (b) `AYAGltfMultiAssetCanary` block 直後並列):

```xml
<!-- <FS:AYAstorm> AYAstorm r41 PC-N-15a (a)/<AYAstorm r41 PC-N-15a (a)> -->
<key>AYAGltfWorkerThreadEnabled</key>
<map>
    <key>Comment</key>
    <string>
        AYAstorm r41 Phase 1.E PC-N-14/PC-N-15a/b/c = worker thread enable gate
        (= recordGltfAssetDraw per-Primitive UBO write + cmdbuf record を
        secondary command buffer 経由 worker thread 並列化、設計原則 (2)
        Core プロセス分散実現の本丸、α 案 = per-thread LLUboRingBuffer
        instance 化で lock-free allocation 達成)。
        OFF (default) = 既 single-thread linear sequential record 維持
        (= Phase 1.D + PC-N-11 + PC-N-12 + PC-N-13 baseline 不変、MUSEUBO-A 整合)。
        ON = worker thread pool launch + per-Primitive secondary cmd_buf record +
        vkCmdExecuteCommands 集約 + first-fire marker 3 件起動 (PC-N-15b 通電予定)。
        PC-N-15a 段階では infra 配線のみ、ON 切替でも描画経路は変化しない
        (= worker storage 確保のみ、launch 経路は PC-N-15b 持越し)。
        Prerequisite: AYAGltfRealDrawEnabled=1 (= recordGltfAssetDraw fire entry)。
    </string>
    <key>Persist</key>
    <integer>1</integer>
    <key>Type</key>
    <string>Boolean</string>
    <key>Value</key>
    <integer>0</integer>
</map>
<key>AYAGltfWorkerThreadCount</key>
<map>
    <key>Comment</key>
    <string>
        AYAstorm r41 Phase 1.E PC-N-14/PC-N-15a = worker thread count
        (= AYAGltfWorkerThreadEnabled=1 時 worker storage 数決定、
        AYAGltfWorkerThreadEnabled=0 時も storage は確保される
        ((N15a-7) B always at init))。
        0 (default) = std::thread::hardware_concurrency() - 1 自動決定
        (= main thread 1 件除外、HW 並列度上限活用)。
        1-N = fixed worker thread count (= 開発時 A/B testing 用、上限は HW 並列度)。
        変更は viewer 再起動で反映 (= initVulkan 1 度きり呼出ゆえ runtime 動的
        re-init なし、((N15a-7) B 採用))。
    </string>
    <key>Persist</key>
    <integer>1</integer>
    <key>Type</key>
    <string>U32</string>
    <key>Value</key>
    <integer>0</integer>
</map>
<!-- </FS:AYAstorm> -->
```

### §4.2 step (b) — 冒頭 include 追加 + worker thread infrastructure storage 追加 (llvkloader.cpp)

`indra/llrender/llvkloader.cpp` 冒頭 include block に `<thread>` / `<atomic>` / `<vector>` / `<mutex>` 追加 (= 既追加なら skip)。anonymous namespace 内 (= line 67-3596 範囲) に `PcN14WorkerContext` struct + storage を `<AYAstorm r41 PC-N-15a (a)>` tag block で配置。

**想定 C++ diff** (= storage 配線のみ、worker launch 経路なし):

```cpp
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
// LL::WorkQueue header (= worker thread post / drain) は PC-N-15b で追加、
// PC-N-15a infra のみゆえ未追加。
// #include "llcommon/workqueue.h"

namespace {
    // <AYAstorm r41 PC-N-15a (a)> worker thread infrastructure storage
    //   ((N15a-2) ⭐ A per-thread LLUboRingBuffer instance + (N15a-3) A
    //   per-thread sDrawUboRingBufferRecords sub-map + (N15a-4) A per-instance
    //   factory closure + (N15a-5) A per-thread sSkinUboDirty sub-map +
    //   (N15a-7) B always at init + (N15a-8) A reverse-init cleanup、
    //   AYA literal「OK」record 2026-06-05)。
    //
    //   worker thread 数 N = AYAGltfWorkerThreadCount cvar=0 時
    //   std::thread::hardware_concurrency() - 1 (= main thread 除外)、>0 時
    //   fixed N ((N14-3) C hybrid + (N15a-7) B always at init)。
    //
    //   PC-N-15a 段階 = storage 配線のみ、worker launch + actual allocate /
    //   record 経路通電は PC-N-15b 持越し。
    struct PcN14WorkerContext
    {
        VkCommandPool       mCommandPool        = VK_NULL_HANDLE;
        VkCommandBuffer     mSecondaryCmdBuf    = VK_NULL_HANDLE;
        std::unique_ptr<LLUboRingBuffer> mDrawUboRingBuffer;
        std::unordered_map<LLUboRingBuffer::BufferHandle, DrawUboRingBufferRecord>
            mDrawUboSubRecords;
        std::unordered_map<UboSkinKey, UboInstance, UboSkinKeyHash>
            mSkinUboSubDirty;
    };
    std::vector<PcN14WorkerContext> sPcn14WorkerCtx;  // size = N worker threads
    std::atomic<U32>                sPcn14WorkerCount{0u};

    // 2 cvar LLCachedControl 宣言 ((N14-10) A + (N14-11) A、settings.xml step (a) 整合)
    static LLCachedControl<bool> sAyastormGltfWorkerThreadEnabled(
        gSavedSettings, "AYAGltfWorkerThreadEnabled", false);
    static LLCachedControl<U32>  sAyastormGltfWorkerThreadCount(
        gSavedSettings, "AYAGltfWorkerThreadCount", 0u);
    // </AYAstorm r41 PC-N-15a (a)>
}
```

### §4.3 step (c) — file-static accessor 4 件 `thread_local` 化 + `sPcn13MultiAssetSeen` mutex 保護 (llvkloader.cpp)

`sCurrentAsset` (`:668`) / `sCurrentSkin` (`:669`) / `sCurrentPrimitive` (`:679`) / `sCurrentNodeAssetMatrix` (`:696`) の 4 file-static 宣言に `thread_local` 修飾子追加 ((N14-8) A 修正版 = (N15a-10) A の 4 件版)。signature 不変、comment 「main thread 専有」→「worker thread 内 thread_local 整合」更新。

`sPcn13MultiAssetSeen` (`:707`) は **`thread_local` 化 reject** ((N15a-6) B = (N14-8) revisit)、代わりに直後並列に `std::mutex sPcn13MultiAssetSeenMutex` 新設。lock_guard 取得は PC-N-15b で worker thread context から PC-N-13 (b) block fire 経路を実装する際に追加 (= PC-N-15a 段階では mutex declaration を追加するが lock 未取得状態でも behavioral regression なし)。

**想定 C++ diff** (4 accessor のうち sCurrentNodeAssetMatrix 例):

```cpp
// <AYAstorm r41 PC-N-12 (c)> ... + PC-N-15a (b) thread_local 化追加
//   ((N14-8) A 修正版 = (N15a-10) A、worker thread 内 thread_local で
//   per-thread context 維持、layering 制約完全充足、accessor signature 不変)。
thread_local const F32* sCurrentNodeAssetMatrix = nullptr;
// </AYAstorm r41 PC-N-12 (c)>
```

`sPcn13MultiAssetSeen` 直後並列 (= `:708` 直後) に:

```cpp
// <AYAstorm r41 PC-N-15a (c)> sPcn13MultiAssetSeen mutex 保護
//   ((N15a-6) B = (N14-8) revisit、PC-N-13 (b) canary semantics 維持 =
//   thread_local 化すると同一 2 asset が別 worker thread に分散時に canary
//   失火、main 単一 set を mutex 経由 access で 2 asset 以上同時描画通電
//   検出を維持。PC-N-15a 段階では declaration のみ、lock_guard 取得は
//   PC-N-15b で worker thread context fire 経路実装時に追加)。
std::mutex sPcn13MultiAssetSeenMutex;
// </AYAstorm r41 PC-N-15a (c)>
```

### §4.4 step (d) — per-thread sub-map storage 配線確認 (= step (b) `PcN14WorkerContext` 内に既配線)

step (b) で `PcN14WorkerContext` struct 内に `mDrawUboRingBuffer` / `mDrawUboSubRecords` / `mSkinUboSubDirty` 3 件 storage を配線済。step (d) は step (b) の struct definition + storage 配線の **論理的分離** で、実装上は step (b) と統合される (= 単一 commit で 1 struct 内に 3 field を配置)。

設計上の分離理由: PC-N-14 design-lock §4 step (d) literal「per-thread sub-ring buffer 化」を独立 step として明示 ((N15a-1) A 整合)。

### §4.5 step (e) — initVulkan + shutdownVulkan で worker thread infra lifecycle 配線 (llvkloader.cpp)

`initVulkan()` (line 3598 entry) 内の `createCommandPool()` 呼出 site (= grep で確認、PC-N-15a 実装 phase で line 番号 fix) 直後並列に new helper `createWorkerThreadInfra()` 呼出 hook 追加。`shutdownVulkan()` (line 4047 entry) 内の `vkDestroyCommandPool(sDevice, sCommandPool, ...)` (`:4305-4307`) 直後並列に `destroyWorkerThreadInfra()` 呼出 hook 追加。

**想定 C++ diff** (= helper 新設 + caller hook):

```cpp
// createDrawUboRingBuffer 関数 (line 1438-1519) 直後並列に new helper 配置:

// <AYAstorm r41 PC-N-15a (d)> worker thread infra 確保 helper
//   ((N15a-2) ⭐ A per-thread LLUboRingBuffer N instance + (N15a-7) B
//   always at init + Vulkan spec per-thread VkCommandPool external sync
//   ((N14-5) A) + secondary cmd_buf alloc ((N14-1) ⭐ A)、AYA literal「OK」
//   record 2026-06-05)。
bool createWorkerThreadInfra()
{
    // worker thread count 決定 ((N14-3) C hybrid):
    const U32 cvar_count = (U32)sAyastormGltfWorkerThreadCount;
    U32 worker_count;
    if (cvar_count == 0u)
    {
        const U32 hw = (U32)std::max(1, (int)std::thread::hardware_concurrency() - 1);
        worker_count = hw;
    }
    else
    {
        worker_count = std::min(cvar_count,
                                (U32)std::max(1u, std::thread::hardware_concurrency()));
    }

    sPcn14WorkerCtx.resize(worker_count);
    sPcn14WorkerCount.store(worker_count, std::memory_order_release);

    // settings.xml AYARingBufferSizeMB 共有 (= main thread instance と同 cvar 経路)
    static LLCachedControl<U32> sRingBufferSizeMB(
        gSavedSettings, "AYARingBufferSizeMB", LLUboRingBuffer::kInitialSizeMB);
    const U32 initial_mb = (U32)sRingBufferSizeMB;

    for (U32 i = 0; i < worker_count; ++i)
    {
        PcN14WorkerContext& ctx = sPcn14WorkerCtx[i];

        // (1) per-thread VkCommandPool ((N14-5) A、Vulkan spec external sync)
        VkCommandPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                          VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = sGraphicsQueueFamily;
        if (vkCreateCommandPool(sDevice, &pool_info, nullptr, &ctx.mCommandPool) != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "PC-N-15a worker[" << i << "] vkCreateCommandPool failed" << LL_ENDL;
            return false;
        }

        // (2) secondary VkCommandBuffer alloc ((N14-1) ⭐ A)
        VkCommandBufferAllocateInfo cb_alloc = {};
        cb_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cb_alloc.commandPool = ctx.mCommandPool;
        cb_alloc.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        cb_alloc.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(sDevice, &cb_alloc, &ctx.mSecondaryCmdBuf) != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "PC-N-15a worker[" << i << "] vkAllocateCommandBuffers (SECONDARY) failed" << LL_ENDL;
            return false;
        }

        // (3) per-thread LLUboRingBuffer instance ((N15a-2) ⭐ A + (N15a-4) A
        //     per-instance factory closure + (N15a-3) A per-thread sub-map)
        auto factory = [i](std::uint32_t size_bytes) -> LLUboRingBuffer::BufferHandle {
            if (sAllocator == VK_NULL_HANDLE) { return 0; }
            VkBufferCreateInfo bci = {};
            bci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bci.size        = size_bytes;
            bci.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            VmaAllocationCreateInfo aci = {};
            aci.usage = VMA_MEMORY_USAGE_AUTO;
            aci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                      | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            aci.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            VkBuffer buffer = VK_NULL_HANDLE;
            VmaAllocation allocation = VK_NULL_HANDLE;
            VmaAllocationInfo info = {};
            VkResult r = vmaCreateBuffer(sAllocator, &bci, &aci, &buffer, &allocation, &info);
            if (r != VK_SUCCESS || info.pMappedData == nullptr)
            {
                if (buffer != VK_NULL_HANDLE) { vmaDestroyBuffer(sAllocator, buffer, allocation); }
                return 0;
            }
            const LLUboRingBuffer::BufferHandle handle =
                static_cast<LLUboRingBuffer::BufferHandle>(reinterpret_cast<std::uintptr_t>(buffer));
            DrawUboRingBufferRecord rec;
            rec.buffer = buffer;
            rec.allocation = allocation;
            rec.mapped = info.pMappedData;
            sPcn14WorkerCtx[i].mDrawUboSubRecords[handle] = rec;  // per-thread sub-map
            return handle;
        };
        auto destroyer = [i](LLUboRingBuffer::BufferHandle handle) {
            if (handle == 0 || sAllocator == VK_NULL_HANDLE) { return; }
            auto it = sPcn14WorkerCtx[i].mDrawUboSubRecords.find(handle);
            if (it == sPcn14WorkerCtx[i].mDrawUboSubRecords.end()) { return; }
            vmaDestroyBuffer(sAllocator, it->second.buffer, it->second.allocation);
            sPcn14WorkerCtx[i].mDrawUboSubRecords.erase(it);
        };
        ctx.mDrawUboRingBuffer = std::make_unique<LLUboRingBuffer>(factory, destroyer, initial_mb);
        if (!ctx.mDrawUboRingBuffer->initialize())
        {
            LL_WARNS("Vulkan") << "PC-N-15a worker[" << i << "] LLUboRingBuffer::initialize() failed" << LL_ENDL;
            return false;
        }
    }
    LL_INFOS("Vulkan") << "PC-N-15a worker thread infra created: worker_count=" << worker_count
                       << ", per-thread VkCommandPool + secondary VkCommandBuffer + "
                          "LLUboRingBuffer instance (initial=" << initial_mb << " MB)" << LL_ENDL;
    return true;
}

void destroyWorkerThreadInfra()
{
    for (auto& ctx : sPcn14WorkerCtx)
    {
        // reverse-init order ((N15a-8) A):
        if (ctx.mDrawUboRingBuffer) { ctx.mDrawUboRingBuffer.reset(); }  // (3) LLUboRingBuffer destroy → BufferDestroyer callback → vmaDestroyBuffer
        if (ctx.mSecondaryCmdBuf != VK_NULL_HANDLE && ctx.mCommandPool != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(sDevice, ctx.mCommandPool, 1, &ctx.mSecondaryCmdBuf);  // (2) secondary cmd_buf free
            ctx.mSecondaryCmdBuf = VK_NULL_HANDLE;
        }
        if (ctx.mCommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(sDevice, ctx.mCommandPool, nullptr);  // (1) VkCommandPool destroy
            ctx.mCommandPool = VK_NULL_HANDLE;
        }
        ctx.mDrawUboSubRecords.clear();
        ctx.mSkinUboSubDirty.clear();
    }
    sPcn14WorkerCtx.clear();  // sPcn14WorkerCtx clear
    sPcn14WorkerCount.store(0u, std::memory_order_release);
}
// </AYAstorm r41 PC-N-15a (d)>
```

caller hook (= initVulkan + shutdownVulkan):

```cpp
// initVulkan() 内、createCommandPool() 呼出直後並列:
//   if (!createCommandPool()) { return false; }
//   <AYAstorm r41 PC-N-15a (e)>
//   if (!createWorkerThreadInfra()) { return false; }
//   </AYAstorm r41 PC-N-15a (e)>

// shutdownVulkan() 内、vkDestroyCommandPool(sDevice, sCommandPool, ...) 直後並列:
//   if (sCommandPool != VK_NULL_HANDLE) { ... vkDestroyCommandPool(...); }
//   <AYAstorm r41 PC-N-15a (e)>
//   destroyWorkerThreadInfra();
//   </AYAstorm r41 PC-N-15a (e)>
```

### §4.6 GATE-B 整合 (= `#ifdef LL_VULKAN_GLSL` 新規追加 0 件)

PC-N-15a 改変は全て host-side C++ (= 2 cvar + storage 配線 + thread_local 化 + mutex 宣言 + initVulkan/shutdownVulkan hook)、shader/GLSL 改変なしゆえ `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 = `count llvkloader.cpp=6` 不変 (= PC-N-14 design-lock commit `1a83070f31` 同数想定)。

### §4.7 MUSEUBO-A 整合 (= OpenGL 描画 100% 維持 + runtime regression 0)

- `AYAGltfWorkerThreadEnabled=false` (default) で worker thread launch 経路 (= PC-N-15b 持越し) skip、PC-N-15a 段階では launch 経路自体未配線ゆえ false 時も true 時も描画経路は不変
- `AYAGltfWorkerThreadCount=0` (default) で auto 決定、PC-N-15a 段階では storage 確保のみゆえ machine HW 差異の影響なし
- OpenGL 描画 100% 維持 (= Vulkan 経路全体が PC-N-9 (a)/(b)/(c) で gate 済、PC-N-15a は Vulkan 経路内部 storage 追加のみ)
- initVulkan 内 `createWorkerThreadInfra()` 失敗時は `return false` で initVulkan 全体が失敗 → Vulkan disable → OpenGL fallback、graceful degrade

### §4.8 設計原則整合

- **(1) Upstream OpenGL 取り込みやすさ維持** = `recordGltfAssetDraw` signature 不変 + `GLTFSceneManager::render` caller-side 改変 0 件 + `LLUboRingBuffer` class 改変 0 件 (= per-instance 化のみ) + per-Primitive UBO write API signature 不変 + `thread_local` 修飾子追加のみで accessor signature 不変 ((N14-8) A) + shader 改変ゼロ
- **(2) Core プロセス分散実現** = per-thread N instance storage 配線 = PC-N-15b で worker thread + secondary cmd_buf record 通電時に per-thread sub-ring + sub-map が lock-free allocate を支える foundation 完成

---

## §5. PC-N-15a design-lock Exit Criteria (= 9 項全充足)

| # | criterion | status |
|---|-----------|--------|
| i | PC-N-15a literal scope §0 完全分解 5 項 (= 2 cvar + include/storage + thread_local 4 件 + mutex 1 件 + per-thread sub-map + initVulkan/shutdownVulkan lifecycle) | ✅ §0 |
| ii | 必読 1 件 (= 本 PC-N-15a design-lock doc) + pinpoint reference 12 件 + background reference 4 件 §1 別記 = full file dump なし | ✅ §1 |
| iii | 現状調査 §2 5 sub-section (= §2.1 LLUboRingBuffer per-instance 化確認 + §2.2 既 main storage 配置 + §2.3 initVulkan/shutdownVulkan 配置 + §2.4 既配線 tag block + §2.5 設計原則整合) | ✅ §2 |
| iv | ambiguity (N15a-1)..(N15a-13) 13 件 AYA literal「OK」record (2026-06-05) §3 + 採用根拠 13 件明文化 (特に (N15a-2) ⭐ A per-thread LLUboRingBuffer 完全独立 N instance + (N15a-6) B sPcn13MultiAssetSeen mutex 保護 = (N14-8) revisit + (N15a-9) A Phase 1.E sub-step 拡張 PC-N-15a/b/c) | ✅ §3 |
| v | 実装計画 (a)-(e) 5 step §4 + 各 step 想定 code diff example 添付 | ✅ §4 |
| vi | PC-N-15a 実装 phase Exit Criteria 10 項明文化 (§6) | ✅ §6 |
| vii | GATE-B 整合 + MUSEUBO-A 整合 + 設計原則 (1)(2) 整合明文化 §4.6-§4.8 | ✅ §4.6-§4.8 |
| viii | 想定改変 file 5 件明文化 ((N15a-10) A) | ✅ §3 (N15a-10) |
| ix | `indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合 | ✅ 本 commit |

---

## §6. PC-N-15a 実装 phase Exit Criteria (= 10 項全充足想定)

| # | criterion | 想定 status |
|---|-----------|-----------|
| i | settings.xml `AYAGltfWorkerThreadEnabled` Boolean + `AYAGltfWorkerThreadCount` U32 2 cvar 追加 (default=0 Persist=1、`AYAGltfMultiAssetCanary` 直後並列 = Phase 1.E cvar group 末尾連続) ((N14-10) A + (N14-11) A) | ⏳ PC-N-15a |
| ii | llvkloader.cpp 冒頭 `<thread>`/`<atomic>`/`<vector>`/`<mutex>` include 追加 (LL::WorkQueue header は PC-N-15b 持越し) | ⏳ PC-N-15a |
| iii | anonymous namespace 内 `<AYAstorm r41 PC-N-15a (a)>` tag block で `PcN14WorkerContext` struct + `std::vector<PcN14WorkerContext> sPcn14WorkerCtx` + `std::atomic<U32> sPcn14WorkerCount` storage + 2 cvar `LLCachedControl` 宣言 | ⏳ PC-N-15a |
| iv | 4 file-static accessor (`sCurrentAsset`/`sCurrentSkin`/`sCurrentPrimitive`/`sCurrentNodeAssetMatrix`) `thread_local` 修飾子追加 + comment 更新 ((N14-8) A 修正版 = (N15a-10) A) | ⏳ PC-N-15a |
| v | `sPcn13MultiAssetSeen` `thread_local` 化 reject + `<AYAstorm r41 PC-N-15a (c)>` tag block で `std::mutex sPcn13MultiAssetSeenMutex` 新設 (lock_guard 取得は PC-N-15b で追加) ((N15a-6) B = (N14-8) revisit) | ⏳ PC-N-15a |
| vi | `createWorkerThreadInfra()` + `destroyWorkerThreadInfra()` helper 新設 + initVulkan / shutdownVulkan caller hook 配線 ((N15a-7) B always at init + (N15a-8) A reverse-init cleanup + (N14-5) A per-thread VkCommandPool + (N14-1) ⭐ A secondary VkCommandBuffer + (N15a-2) ⭐ A per-thread LLUboRingBuffer instance + (N15a-3)/(N15a-4) per-instance factory closure + per-thread sub-map) | ⏳ PC-N-15a |
| vii | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp=6 不変、PC-N-14 design-lock commit `1a83070f31` 同数想定) + MUSEUBO-A 整合 = `AYAGltfWorkerThreadEnabled=false`/`true` 両方で描画経路不変 (= PC-N-15a 段階 launch 経路未配線) + OpenGL 描画 100% 維持 ((N15a-12) A) | ⏳ PC-N-15a |
| viii | build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 + GATE-B integrity ((N15a-12) A) | ⏳ PC-N-15a |
| ix | cross-platform spec §6 PC-N-15a 行 ✅ 反映 + §A 履歴 1 行追記 ((N15a-10) A の (3)) + handoff PC-N-15a complete doc 起案 ((N15a-10) A の (5)) | ⏳ PC-N-15a |
| x | self-verify 9 観点 全 ✅ | ⏳ PC-N-15a |

---

## §7. PC-N-15a 着手手順 8 step

1. 本 PC-N-15a design-lock doc 全文 Read (= 必読 1 件)
2. pinpoint reference 12 件 (§1.2) を必要分のみ Read (= full file dump 禁止、`feedback_handoff_minimal_pre_req_read` 整合)
3. step (a) settings.xml 2 cvar 追加 (= `AYAGltfMultiAssetCanary` 直後並列)
4. step (b) llvkloader.cpp 冒頭 include 追加 + anonymous namespace 内 `PcN14WorkerContext` struct + storage + 2 cvar LLCachedControl 宣言
5. step (c) 4 accessor `thread_local` 化 + `sPcn13MultiAssetSeen` mutex declaration 追加
6. step (e) `createWorkerThreadInfra()` + `destroyWorkerThreadInfra()` helper 新設 + initVulkan/shutdownVulkan caller hook 配線 (= step (d) per-thread sub-map storage は step (b) `PcN14WorkerContext` 内に統合済)
7. build verify literal 取得 (= llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 + GATE-B integrity LL_VULKAN_GLSL count llvkloader.cpp=6 不変) + GATE-B integrity 確認
8. cross-platform spec §6 PC-N-15a 行 ✅ 反映 + §A 履歴 1 行追記 + handoff PC-N-15a complete doc 起案 → AYA 明示 commit 指示受領後 commit

---

## §8. 残 strict 線形

- ✅ Phase 1.A / 1.B / (Z) SSS / (W) uniform4iv / (Y) Phase 1.C prep
- ✅ PC-0..PC-7ε / PC-N decomposition / PC-N-1..PC-N-4 (= Phase 1.C complete)
- ✅ PC-8 Linux primary marker (= Phase 1.C strict 線形終了)
- ✅ PC-N-5 (= Phase 1.D 着手起点) / Phase 1.D decomposition design-lock
- ✅ PC-N-6 / PC-N-7 / PC-N-8 / PC-N-9 / PC-N-10 (= Phase 1.D complete = 1 GLTF asset 完全 Vulkan draw 通電 達成)
- ✅ Phase 1.E decomposition design-lock (commit `01cd001d07`) + PC-N-11 design-lock (commit `cdf4dccc0d`) + PC-N-11 実装 (commit `13bfb55b35`) + PC-N-12 design-lock (commit `c9c99d278f`) + PC-N-12 実装 (commit `4373c302d7`) + PC-N-13 design-lock (commit `e13d00d4a6`) + PC-N-13 実装 (commit `faae1544d6`) + PC-N-14 design-lock (commit `1a83070f31`)
- ✅ **PC-N-15a design-lock ✅ 本 commit = Phase 1.E 内 5th sub-step design-lock complete = worker thread infra design (= per-thread LLUboRingBuffer N instance + per-thread VkCommandPool + secondary VkCommandBuffer + thread_local 4 件 + sPcn13MultiAssetSeen mutex 保護 + 2 cvar 配線 + initVulkan/shutdownVulkan lifecycle、ambiguity 13 件 resolve + 実装計画 5 step + Exit Criteria 9+10 項)**
- ⏳ PC-N-15a 実装 (= 別 session、infra 配線通電 + build verify)
- ⏳ PC-N-15b design-lock + 実装 (= worker thread launch + secondary cmd_buf record + vkCmdExecuteCommands 集約 + drain + first-fire marker 3 件 + sPcn13MultiAssetSeen lock_guard 取得 + sSkinUboDirty merge 経路)
- ⏳ PC-N-15c 実装 (= `sGltfStubSkin` sentinel storage 撤去 + 残余 placeholder 撤去 + Phase 1.E complete marker 起案)
- ⏳ Phase 1.E complete → Phase 1 全完了 → Mac/Win 開発者補完 phase

---

## §9. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ +
(Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α..PC-7ε ✅ +
PC-N decomposition design-lock ✅ + PC-N-1..PC-N-4 ✅ = Phase 1.C complete ✅ +
PC-8 Linux primary marker ✅ + PC-N-5 ✅ + Phase 1.D decomposition design-lock ✅
+ PC-N-6 ✅ + PC-N-7 ✅ + PC-N-8 ✅ + PC-N-9 ✅ + PC-N-10 ✅ = Phase 1.D
complete ✅ (= 1 GLTF asset 完全 Vulkan draw 通電 達成) +
Phase 1.E decomposition design-lock ✅ + PC-N-11 design-lock ✅ +
PC-N-11 ✅ + PC-N-12 design-lock ✅ + PC-N-12 ✅ + PC-N-13 design-lock ✅ +
PC-N-13 ✅ + PC-N-14 design-lock ✅ +
**PC-N-15a design-lock ✅ 本 commit = Phase 1.E 内 5th sub-step design-lock
complete = worker thread infra design (= per-thread LLUboRingBuffer N instance
+ per-thread VkCommandPool + secondary VkCommandBuffer + thread_local 4 件 +
sPcn13MultiAssetSeen mutex 保護 + 2 cvar 配線 + initVulkan/shutdownVulkan
lifecycle、ambiguity 13 件 resolve + 実装計画 5 step + Exit Criteria 9+10 項)** +
⏳ PC-N-15a 実装 + PC-N-15b design-lock + 実装 + PC-N-15c 実装 +
Phase 1.E complete + Phase 1 全完了 + Mac/Win 開発者補完 phase

---

## §10. self-verify 9 観点 全 ✅

1. ✅ PC-N-15a literal scope §0 完全分解 5 項 (= AYA task statement literal + PC-N-14 11 step 分解 + (N15a-1) A 採用案が source of truth = 2 cvar + include/storage + thread_local 4 件 + mutex 1 件 + per-thread sub-map + initVulkan/shutdownVulkan lifecycle)
2. ✅ 必読 1 件 §1.1 + pinpoint reference 12 件 §1.2 + background reference 4 件 §1.3 別記 = full file dump なし (= `feedback_handoff_minimal_pre_req_read` 整合)
3. ✅ 現状調査 §2 5 sub-section 網羅 (特に §2.1 = LLUboRingBuffer per-instance 化可能性確認 7 項目 = constructor 注入型 + copy delete + state per-instance member + factory closure pattern + memory overhead 試算で **class 改変 0 件で N instance 化実現可** 発見が本 PC-N-15a design-lock の最大 knowledge contribution、PC-N-14 design-lock §1.2 reference 13 「llrender/」誤記を「llcommon/」訂正も明示)
4. ✅ ambiguity (N15a-1)..(N15a-13) 13 件 AYA literal「OK」record (2026-06-05) §3 + 採用根拠 13 件明文化 (特に (N15a-2) ⭐ A per-thread LLUboRingBuffer 完全独立 N instance = β案 reject 経由 α案 path 確定 = (E-14) B literal「UBO write + cmdbuf 両方並列化」完全整合 + (N15a-6) B sPcn13MultiAssetSeen mutex 保護 = (N14-8) revisit で canary semantics 維持 + (N15a-9) A Phase 1.E sub-step 拡張 PC-N-15a/b/c = `feedback_ubo_migration_one_at_a_time` 厳格遵守)
5. ✅ 実装計画 (a)-(e) 5 step §4 + 各 step 想定 code diff example 添付 (特に step (e) `createWorkerThreadInfra()` helper の per-instance factory closure pattern = lambda capture `i` 経由 per-thread sub-map に書込 + `destroyWorkerThreadInfra()` reverse-init order = LLUboRingBuffer destroy → vkFreeCommandBuffers → vkDestroyCommandPool → sPcn14WorkerCtx clear)
6. ✅ GATE-B 整合 §4.6 (= `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、shader 改変ゼロ、host-side C++ 配線のみ)
7. ✅ MUSEUBO-A 整合 §4.7 (= `AYAGltfWorkerThreadEnabled=false`/`true` 両方で描画経路不変 = PC-N-15a 段階 launch 経路未配線、storage 確保のみ、OpenGL 描画 100% 維持 + initVulkan 失敗時 graceful degrade)
8. ✅ 設計原則整合 §4.8 (= (1) Upstream OpenGL 取り込みやすさ維持 = `recordGltfAssetDraw` signature 不変 + `GLTFSceneManager::render` caller-side 改変 0 件 + `LLUboRingBuffer` class 改変 0 件 + accessor signature 不変 + shader 改変ゼロ + (2) Core プロセス分散実現 = per-thread N instance storage 配線で PC-N-15b lock-free allocate 経路の foundation 完成)
9. ✅ `indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合 (= 本 commit は本 PC-N-15a design-lock doc 起案 + cross-platform spec §6 行更新 + §A 履歴追記のみ)

---

## §11. 次 session 着手 1 line

PC-N-15a 実装着手 = step (a)-(e) 5 step 実施 = (a) settings.xml `AYAGltfWorkerThreadEnabled` + `AYAGltfWorkerThreadCount` 2 cvar 追加 (`AYAGltfMultiAssetCanary` 直後並列) + (b) llvkloader.cpp 冒頭 include 追加 + anonymous namespace 内 `PcN14WorkerContext` struct + storage + 2 cvar LLCachedControl 宣言 + (c) 4 accessor `thread_local` 化 + `sPcn13MultiAssetSeen` mutex declaration 追加 + (d) per-thread sub-map storage は step (b) 統合済 + (e) `createWorkerThreadInfra()` + `destroyWorkerThreadInfra()` helper 新設 + initVulkan/shutdownVulkan caller hook 配線 + build verify literal 取得 + cross-platform spec §6 PC-N-15a 行 ✅ 反映 + §A 履歴 1 行追記 + handoff PC-N-15a complete doc 起案 = `feedback_ubo_migration_one_at_a_time` 厳格遵守で本 PC-N-15a design-lock baseline 上に Phase 1.E 内 5th = infra 配線 sub-step として別 session 実装。

---

## §A. feedback 遵守 record

- ✅ `feedback_proactive_handoff` (本 PC-N-15a design-lock handoff doc 起案)
- ✅ `feedback_handoff_minimal_pre_req_read` (必読 1 件 = 本 PC-N-15a design-lock doc + pinpoint reference 12 件 + background reference 4 件別記、full file dump なし)
- ✅ `feedback_self_verify_before_handoff` (9 観点 self-verify 全 ✅)
- ✅ `feedback_build_only_verified` (design-lock phase は `indra/` 改変 0 件で build verify 対象外、PC-N-15a 実装 phase で literal 検証取得予定 (N15a-12) A 採用)
- ✅ `feedback_no_scope_shrink` (PC-N-15a literal scope §0 完全分解 5 項 = PC-N-14 11 step のうち (a)+(b)+(c)+(d)+(g) 担当 (N15a-1) A 整合が source of truth、(N15a-9) A Phase 1.E sub-step 拡張 PC-N-15a/b/c は **`feedback_ubo_migration_one_at_a_time` 厳格遵守による build verify + 段階通電 + issue 局所化** ゆえ scope 縮小ではなく品質向上、(N15a-2) ⭐ A α案 path は PC-N-14 (E-14) B literal「UBO write + cmdbuf 両方並列化」完全整合 path で β案 reject 経由本 path 確定)
- ✅ `feedback_doubt_self_first` (PC-N-15 実装着手前 investigation 経由 (N14-7) re-decide、β案推奨を AYA literal「短い寿命な処理だけ並列化で意味なし」指摘で撤回 + α案 path 採用 + (N15a-6) B `sPcn13MultiAssetSeen` mutex 保護で (N14-8) revisit + 13 件 ambiguity 推奨案提示 + AYA literal「OK」record 後本 design-lock doc 起案、推測実装なし、特に (N15a-2) ⭐ critical = LLUboRingBuffer header `:36-40` literal「Vulkan device 非依存」+ `:69-73` constructor signature + `:76-77` copy delete + `:122-128` per-instance member を Read で確認後採用)
- ✅ `feedback_admit_unknown` (PC-N-14 design-lock §1.2 reference 13 「`llrender/` 内 LLUboRingBuffer」記述を investigation で「`llcommon/` 配置」と発見訂正、本 PC-N-15a design-lock §1.2/§2.1 で literal 訂正、勝手に推測せず Read で確認、sSkinUboDirty merge semantic ambiguity (= 同一 Skin が複数 worker から書込まれる場合) は PC-N-15a 段階では未解決ゆえ PC-N-15b 持越し明示 (= (N15a-5) A literal))
- ✅ `feedback_confirm_referent_before_acting` (13 件 batch AYA 確認 design-lock phase で完了 = AYA literal「OK」record 2026-06-05、推測実装なし、特に (N15a-2) ⭐ critical は LLUboRingBuffer header Read 後 per-instance 化可能性 7 項目明示 + 推奨 A 根拠明示後 AYA literal「OK」record で α 案 path 確定)
- ✅ `feedback_ubo_migration_one_at_a_time` 厳格遵守 (PC-N-15a = worker thread infra 配線単独 sub-step、PC-N-15b (= worker launch + secondary cmd_buf record + 並列実行通電) + PC-N-15c (= sentinel cleanup + Phase 1.E complete marker) は別 phase の別 session で別途 (N15a-9) A 整合)
- ✅ `feedback_design_phase_no_code_write` 厳格遵守 (本 PC-N-15a design-lock phase は doc 起案 + cross-platform spec §6 PC-N-15a 行追加 + §A 履歴追記のみ、`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件)
- ✅ `feedback_release_branch_workflow` (feature branch `feature/ayastorm-r41-gl-removal` 上 commit 予定)
- ✅ `feedback_no_auto_commit` (AYA 明示 commit 指示受領後 commit 予定)
- ✅ `feedback_no_claude_coauthor` / Co-Authored-By 行不在予定
- ✅ `feedback_no_bare_reference_ids` ((N15a-1)..(N15a-13) 各 ID に項目名 / 採用案内容併記 + (a)..(e) 各 step に作業内容併記 + (N14-1)/(N14-3)/(N14-5)/(N14-8)/(N14-10)/(N14-11)/(N14-21) 各 reference に PC-N-14 design-lock 内項目名併記)
- ✅ `feedback_tests_dir_never_commit` 整合 (tests/ 改変 0 件、git add 個別 file 指定予定)
- ✅ memory `project_ayastorm_r41_design_principles` 整合
  ((1) Upstream OpenGL 取り込みやすさ維持 = `recordGltfAssetDraw` signature 不変 + `GLTFSceneManager::render` caller-side 改変 0 件 + `LLUboRingBuffer` class 改変 0 件 ((N15a-2) ⭐ A) + accessor signature 不変 ((N14-8) A 修正版 = (N15a-10) A 4 件) + shader 改変ゼロ + (2) Core プロセス分散実現 = per-thread N instance storage 配線で PC-N-15b lock-free allocate 経路の foundation 完成 ((N15a-2) ⭐ A + (N15a-3)/(N15a-4)/(N15a-5) per-thread sub-map))
- ✅ memory `project_r41_phase1b_vulkan_host_gate` 整合 (GATE-B = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 ((N15a-12) A)、`AYAGltfWorkerThreadEnabled` + `AYAGltfWorkerThreadCount` cvar runtime gate のみ ((N14-10) A)、count llvkloader.cpp=6 不変想定 (= PC-N-14 design-lock commit `1a83070f31` 同数))
- ✅ memory `project_ayastorm_three_platforms` 整合 (cross-platform spec §6 PC-N-15a 行追加で macOS / Windows 派生 fix 候補なし想定 = host-side `std::thread::hardware_concurrency()` + per-thread `VkCommandPool` + secondary `VkCommandBuffer` + per-thread `LLUboRingBuffer` instance + `thread_local` storage は OS 非依存 + VMA default thread-safe locking は OS 非依存 + 2 cvar XML は OS 非依存 + descriptor set 数 5 維持 ゆえ macOS 派生 fix 候補なし + Windows full Vulkan ゆえ派生 fix 候補なし、Linux primary 完成 → 他者補完 model 整合)
