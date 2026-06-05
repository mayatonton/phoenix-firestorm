# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.E **PC-N-14 design-lock complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: Phase 1.E 内 **4th sub-step = PC-N-14 = worker thread design-lock** (= per-Primitive UBO write + cmdbuf record 並列化 design = 設計原則 (2) Core プロセス分散実現の本丸) の design-lock phase 完了 marker = ambiguity (N14-1)..(N14-20) 20 件 全 AYA literal「OK」record (2026-06-05) + 実装計画 (a)-(k) 11 step 分解 + Exit Criteria 9+10 項明文化。`indra/` 改変 0 件 (= `feedback_design_phase_no_code_write` 整合)。実装は PC-N-15 で別 session ((E-10) B + `feedback_ubo_migration_one_at_a_time` 厳格遵守)。

> **本 doc 位置付け**: PC-N-14 詳細 design-lock。Phase 1.E decomposition design-lock (= `handoff-...-phase1-e-decomposition-design-lock.md`) + PC-N-11 design-lock + PC-N-11 実装 + PC-N-12 design-lock + PC-N-12 実装 + PC-N-13 design-lock + PC-N-13 実装 (commit `289d44b536`) baseline 上に、PC-N-14 単独の **詳細実装計画 (= PC-N-15 で別 session 実施)** + **想定 code diff example** + **ambiguity 20 件 (N14-1)..(N14-20)** + **Exit Criteria 10 項 (PC-N-15 実装 phase 用)** を確定。PC-N-11/PC-N-12/PC-N-13 design-lock doc と同形 pattern 踏襲、ただし PC-N-14 は **design-lock + 実装が PC-N-14 / PC-N-15 で別 sub-step 分割** ((E-9) B + (E-10) B 採用) ゆえ本 doc は PC-N-15 実装 phase の source of truth (= (N14-18) B 採用「別 design doc 新設不要」)。
>
> **⭐ 重大 design decision (= (N14-1) ⭐ critical A 採用根拠)**: cmdbuf 分散方式は **secondary command buffer + `vkCmdExecuteCommands` 集約 pattern** を採用。Vulkan 標準 multi-thread cmdbuf record pattern (= worker thread が `VK_COMMAND_BUFFER_LEVEL_SECONDARY` を独立記録、main thread が 1 回の `vkCmdExecuteCommands` で集約) ゆえ submit は main thread 単一維持で synchronization 簡素 + (E-14) B「UBO write + cmdbuf 両方並列化」literal 完全整合。case B (primary cmdbuf per-thread + multi-submit) は submit 順序 + GPU side fence/semaphore 必要で複雑度過大、case C (UBO write のみ並列、cmdbuf main) は (E-14) B 違反 (= scope 縮小)。

---

## §0. 本 session 着手契機 + literal scope record

**契機**: AYA 指示「r41 Phase 1.E PC-N-14 design-lock 着手お願いします。直前 commit = `289d44b536` (PC-N-13 complete = real per-draw light params cvar gate 通電 +「zero IS real data」semantic 確立 + multi-asset canary 配線、Phase 1.E 内 3rd sub-step 実装完了)。task = Phase 1.E 内 4th sub-step = worker thread design-lock = per-Primitive UBO write + cmdbuf record 並列化 design + ambiguity 確認 + 実装計画分解 + Exit Criteria 明文化。持越し (`feedback_ubo_migration_one_at_a_time` 厳格遵守): PC-N-15 (worker thread 実装 + Phase 1.E complete marker) は別 session。design-lock phase: `indra/` 改変 0 件 (`feedback_design_phase_no_code_write` 厳格遵守)、doc 起案 + cross-platform spec §6 PC-N-14 行更新のみ。」literal 受領 (2026-06-05、PC-N-13 complete commit `289d44b536` 後の継続 session = 別 session の fresh context)。

**PC-N-14 literal scope** (= AYA task statement 直訳 + Phase 1.E decomposition (E-9) B + (E-14) B + (E-3) A 整合、3 項):

1. **per-Primitive UBO write 並列化 design** = `recordGltfAssetDraw` PC-N-8 (f) real Asset path 内の per-Primitive UBO write 経路 (= `writeDrawUbo`/`writeSkinUbo`/`flushSkinUbos` + 既配線 PC-N-11 (a)/PC-N-12 (a)/PC-N-13 (a)/(b) tag block) を worker thread に分散 = sub-ring buffer pattern + main thread 集約 ((N14-7) B 採用)。
2. **per-Primitive cmdbuf record 並列化 design** = `vkCmdBindDescriptorSets`/`vkCmdPushConstants`/`vkCmdBindVertexBuffers`/`vkCmdBindIndexBuffer`/`vkCmdDrawIndexed` の per-Primitive 記録を **secondary command buffer (= `VK_COMMAND_BUFFER_LEVEL_SECONDARY`)** に worker thread が独立記録 + main thread が **1 回の `vkCmdExecuteCommands`** で primary cmdbuf に集約 ((N14-1) ⭐ A 採用)。
3. **2 cvar 新設** = `AYAGltfWorkerThreadEnabled` (Boolean default=0 Persist=1、enable/disable gate) + `AYAGltfWorkerThreadCount` (U32 default=0、0=`hardware_concurrency()-1` auto、>0=fixed thread count) ((N14-10) A + (N14-3) C hybrid 採用)。

**Phase 境界**: PC-N-14 完了 = Phase 1.E 内 4th sub-step **設計** 完了 = worker thread 設計確定 (= 全 ambiguity 20 件 resolve + 実装計画 (a)-(k) 11 step + Exit Criteria 10 項明文化)。**実装** は PC-N-15 持越し ((E-10) B 整合)。`sGltfStubSkin` sentinel storage 撤去 + 残余 placeholder 撤去 + Phase 1.E complete marker 起案も PC-N-15 持越し ((E-11) A + (E-10) B 統合)。

---

## §1. 必読 1 件 + pinpoint reference

### §1.1 必読 1 件 (= 次 session = PC-N-15 実装 phase 着手前)

1. **本 PC-N-14 design-lock doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-e-pc-n-14-design-lock.md`

### §1.2 pinpoint reference 14 件 (= 実装 phase で必要分のみ Read)

1. **`recordGltfAssetDraw` 関数定義**: `indra/llrender/llvkloader.cpp:5946` = `static void recordGltfAssetDraw(VkCommandBuffer cmd_buf)` (= 単一 cmd_buf 経由 linear sequential record、PC-N-14 (a) で cmd_buf 引数を worker thread context per-Primitive 化または primary cmd_buf 維持 + secondary cmd_buf 配給)
2. **PC-N-8 (f) real Asset path block 全体**: `indra/llrender/llvkloader.cpp:5906-6147` = PC-N-14 (a) cvar gate wrap + worker thread launch + per-Primitive secondary cmd_buf 配給対象、内側に既 PC-N-11 (a) / PC-N-12 (a) / PC-N-13 (a) / PC-N-13 (b) tag block 並存
3. **`sCommandBuffer` + `sCommandPool` storage**: `indra/llrender/llvkloader.cpp:80` (storage) / `:1085` (CreateInfo) / `:1104` (alloc) = 単一 instance 現状、PC-N-14 (b) で per-worker thread `sWorkerCommandPool[N]` + `sWorkerSecondaryCmdBuf[N]` storage 追加
4. **file-static accessor 5 件 (`sCurrentAsset`/`sCurrentSkin`/`sCurrentPrimitive`/`sCurrentNodeAssetMatrix`/`sPcn13MultiAssetSeen`)**: `llvkloader.cpp:668`/`:669`/`:679`/`:696`/`:707` = PC-N-14 (c) で `thread_local` 修飾子追加 ((N14-8) A 採用)、signature 不変
5. **accessor 実装 5 件 (`setCurrentAsset`/`setCurrentSkin`/`setCurrentPrimitive`/`setCurrentNodeAssetMatrix`/`get*` + `clear*`)**: `llvkloader.cpp:5745-5818` = PC-N-14 (c) 改変対象 (= `thread_local` 化整合 + comment 「main thread 専有」→「worker thread 内 thread_local」更新のみ)
6. **`writeDrawUbo` signature**: `llvkloader.cpp:5256` = `void writeDrawUbo(U32 block_hash, U32 offset, const void* data, size_t size, U32& out_dynamic_offset)` = PC-N-14 (d) で `sDrawUboRingBufferMgr` per-thread sub-ring 分散 ((N14-7) B 採用)、signature 不変
7. **`writeSkinUbo` signature**: `llvkloader.cpp:5557` = `void writeSkinUbo(LL::GLTF::Skin* skin, U32 block_hash, U32 offset, const void* data, size_t size)` = PC-N-14 (d) で `sSkinUboDirty` per-thread sub-map 分散候補、Skin key で衝突可能性検討 (= 同一 Skin が複数 worker thread から書込まれた時の dirty range merge)
8. **`flushDrawUbos` + `flushSkinUbos`**: `llvkloader.cpp:4873` (`flushDrawUbos`) + `flushSkinUbos` (file 内呼出 site から推定) = PC-N-14 (e) で main 集約 phase 後の unified flush call 配置
9. **`wireSkinUboSetV3aToBinding2`**: `llvkloader.cpp:2858` = PC-N-11 (a) per-draw rewire 既配線、PC-N-14 (a) で secondary cmd_buf 記録経路下に移行 (= secondary cmd_buf は descriptor set inheritance 仕様確認、`VkCommandBufferInheritanceInfo` で render pass + framebuffer + subpass index 引継ぎ)
10. **`vkCmdBindDescriptorSets` 4 site**: `llvkloader.cpp:1837/1898/1940/1950` = PC-N-2 (b) body 内 set=0/1a/1b/2 4-set bind + set=3 単独 bind = PC-N-14 (a) で secondary cmd_buf に移行対象 (= per-Primitive recording の所属判定)
11. **`vkCmdPushConstants` / `vkCmdBindIndexBuffer` / `vkCmdBindVertexBuffers` / `vkCmdDrawIndexed`**: `llvkloader.cpp:6166` (push constant、PC-N-12 (a) 内) / `:6693` (`bindIndexBufferVk` wrapper 内) / `:6677` (`bindVertexBufferVk` wrapper 内) / `:6214` (drawIndexed) = PC-N-14 (a) で全 site secondary cmd_buf 記録に移行
12. **`GLTFSceneManager::render(U8 variant)`**: `indra/newview/gltfscenemanager.cpp:599` + per-Primitive loop `:738-816` (= `setCurrentAsset:697` / `flushAssetUbos:702` / `setCurrentPrimitive:752` / `setCurrentNodeAssetMatrix:768` / `setCurrentSkin:781` / `flushSkinUbos:785` / `drawRangeFast:797` / `clear*:803-820`) = PC-N-14 (f) で worker thread launch / drain hook 配置検討、ただし (N14-9) A 採用で `recordGltfAssetDraw` 内 launch + drain ゆえ caller-side 改変 0 件想定
13. **`sDrawUboRingBufferMgr`**: `llvkloader.cpp:424` (storage) / `:1503` (initialize) = `LLUboRingBuffer` unique_ptr、PC-N-14 (d) で per-thread sub-ring 化 ((N14-7) B 採用)、thread-safety 現状未確認ゆえ実装時 LL::UboRingBufferMgr 内部実装確認必須
14. **`Skin::uploadMatrixPalette`**: `indra/newview/gltf/skin.cpp` (PC-7γ-3 (j) dual-write 既配線) = main thread context 想定、PC-N-14 (d) で worker thread context 移行可否確認 (= `Skin` object lifecycle + dual-write 内 GL/Vulkan 経路の thread-safety 確認、最悪 main thread 維持 + worker は consume side のみ並列化)

### §1.3 background reference 3 件 (= Phase 1.E 全体 + 設計原則)

- **Phase 1.E decomposition design-lock**: `handoff-...-phase1-e-decomposition-design-lock.md` §4.4 = PC-N-14 想定 ambiguity 5 件 (worker thread 数 / command pool / VMA / sync / cvar) + (E-9) B + (E-14) B 整合確認
- **PC-N-13 complete handoff**: `handoff-...-phase1-e-pc-n-13-complete.md` = baseline (= PC-N-8 (f) real Asset path + PC-N-11 (a) / PC-N-12 (a) / PC-N-13 (a) / (b) 既配線 tag block 配置確認)
- **design 09 phase roadmap**: `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` = Phase 1 全体 roadmap + worker thread 並列化方針 source ((E-9) B literal「design 09 参照」)

---

## §2. 現状調査結果 (= PC-N-13 complete baseline + worker thread 設計対象 site 確認)

### §2.1 `recordGltfAssetDraw` 現状構造 (= 単一 thread baseline)

| # | site | file:line | 現状 | PC-N-14 改変 |
|---|------|-----------|------|-------------|
| 1 | 関数 signature | `llvkloader.cpp:5946` | `static void recordGltfAssetDraw(VkCommandBuffer cmd_buf)` | unchanged ((N14-2) A per-Primitive granularity 採用、関数 signature 不変、worker thread 配給は内部 detail) |
| 2 | PC-N-8 (f) real Asset path block | `llvkloader.cpp:5906-6147` | 単一 cmd_buf 経由 linear sequential record (PC-N-11/12/13 tag block 既配線) | PC-N-14 (a) で cvar gate wrap + worker thread launch + per-Primitive secondary cmd_buf 配給 |
| 3 | `writeDrawUbo` site | `llvkloader.cpp:5984-5995` + PC-N-13 (a) | sDrawUboRingBufferMgr allocate + dynamic_offset (= host write 256 B zero buf、(N13-1) C zero IS real data) | PC-N-14 (d) で per-thread sub-ring 化 ((N14-7) B、main 集約 phase で merge) |
| 4 | `writeSkinUbo` + `flushSkinUbos` + `wireSkinUboSetV3aToBinding2` (PC-N-11 (a) 内) | `llvkloader.cpp:5997-6056` | per-draw rewire + dirty exchange + sentinel/real path 切替 | PC-N-14 (d) で per-thread sub-map dirty + main 集約後 unified flush ((N14-7) B) |
| 5 | `vkCmdPushConstants` (PC-N-12 (a) 内) | `llvkloader.cpp:6059-6120` (PC-N-12 (a) inner block) + `:6166` (push constant call) | single push constant 64 B / VERTEX_BIT call | PC-N-14 (a) で secondary cmd_buf に移行 |
| 6 | `vkCmdBindVertexBuffers` / `vkCmdBindIndexBuffer` / `vkCmdDrawIndexed` | `llvkloader.cpp:6204` (`bindVertexBufferVk`) / `:6213` (`bindIndexBufferVk`) / `:6214` (`vkCmdDrawIndexed`) | primary cmd_buf 直接記録 | PC-N-14 (a) で secondary cmd_buf に移行 |

### §2.2 file-static accessor 群 (= thread-safety 観点 + PC-N-14 改変方針)

| # | 変数 | line | 現状 thread-safety | PC-N-14 改変 ((N14-8) A) |
|---|------|------|-------------------|------------------------|
| 7 | `sCurrentAsset` (PC-7γ-2 tag) | `llvkloader.cpp:668` | main thread 専有、mutex 不要 | `thread_local LL::GLTF::Asset*` 化 + comment「main thread 専有」→「worker thread 内 thread_local」更新 |
| 8 | `sCurrentSkin` (PC-7γ-2 tag) | `llvkloader.cpp:669` | main thread 専有、mutex 不要 | `thread_local LL::GLTF::Skin*` 化 (同上) |
| 9 | `sCurrentPrimitive` (PC-N-8 (e) tag) | `llvkloader.cpp:679` | main thread 専有、mutex 不要 | `thread_local LL::GLTF::Primitive*` 化 (同上) |
| 10 | `sCurrentNodeAssetMatrix` (PC-N-12 (c) tag) | `llvkloader.cpp:696` | main thread 専有、mutex 不要 | `thread_local const F32*` 化 (同上) |
| 11 | `sPcn13MultiAssetSeen` (PC-N-13 (b) tag) | `llvkloader.cpp:707` | main thread 専有、mutex 不要 | **`thread_local std::unordered_set<const void*>`** 化または main thread 単一維持 (= debug-only canary ゆえ worker thread context で fire しても fire しなくても機能影響なし、(N14-8) A 採用で thread_local 統一推奨、ただし size 監視は per-thread 視点で十分) |

### §2.3 既存 Vulkan / threading インフラ (= 全 0 件 = PC-N-14 新設対象)

| # | infra | 現状 | PC-N-14 (b) 新設 |
|---|------|------|-------------|
| 12 | `LL::WorkQueue` / `LLThreadPool` / `std::thread` (`llvkloader.cpp` 内) | **未使用** (grep 結果 0 件) | PC-N-14 (b) で `LL::WorkQueue` 経由 worker thread pool 設営 ((N14-4) A 採用) |
| 13 | `VK_COMMAND_BUFFER_LEVEL_SECONDARY` / `vkCmdExecuteCommands` | **未使用** (grep 結果 0 件) | PC-N-14 (b) で per-thread secondary cmd_buf 設営 + (a) で `vkCmdExecuteCommands` 集約 ((N14-1) ⭐ A 採用) |
| 14 | `VkCommandPool` storage | 単一 `sCommandPool` (`:80`) | PC-N-14 (b) で `sWorkerCommandPool[N]` array (N = worker thread count) 追加 ((N14-5) A、Vulkan spec 必須) |
| 15 | `sDrawUboRingBufferMgr` thread-safety | `LLUboRingBuffer` unique_ptr、内部実装 thread-safety **未確認** | PC-N-14 (d) で per-thread sub-ring 化 + main 集約 phase で merge ((N14-7) B) |
| 16 | `Skin::uploadMatrixPalette` thread context | main thread (PC-7γ-3 (j) dual-write 既配線) | PC-N-14 (d) で worker thread context 移行可否確認、最悪 main thread 維持 + consume only 並列 |

### §2.4 既配線 tag block (= 既配線、PC-N-14 (a) で内側に配置移行)

| # | tag block | line | sub-step | PC-N-14 (a) 改変 |
|---|-----------|------|----------|----|
| 17 | PC-N-8 (f) outer block | `llvkloader.cpp:5906-6147` | Phase 1.D 5th = real Asset path 確立 | PC-N-14 (a) で全体を cvar gate wrap + worker thread launch + per-Primitive secondary cmd_buf 配給 |
| 18 | PC-N-11 (a) inner block | `llvkloader.cpp:5997-6056` | Phase 1.E 1st = multi-skin real Skin path | worker thread context 内で実行 (= `sCurrentSkin` thread_local 整合) |
| 19 | PC-N-12 (a) inner block | `llvkloader.cpp:6059-6120` | Phase 1.E 2nd = real node modelview | worker thread context 内で実行 (= `sCurrentNodeAssetMatrix` thread_local 整合) |
| 20 | PC-N-13 (a) inner block | `llvkloader.cpp:5984-5995` 内 | Phase 1.E 3rd = writeDrawUbo zero buffer cvar wrap | worker thread context 内で実行 (= per-thread sub-ring allocate) |
| 21 | PC-N-13 (b) inner block | PC-N-12 (a) closing tag 直後 | Phase 1.E 3rd = multi-asset canary marker | worker thread context 内で実行 (= `sPcn13MultiAssetSeen` thread_local 整合) |

### §2.5 設計原則 (2) Core プロセス分散実現整合確認

| # | 設計原則項目 | 確認 |
|---|------------|------|
| 22 | per-Primitive granularity 既配線 | PC-N-8 (a)/(b)/(c)/(d) で `sPrimitiveVertexBuffers`/`sPrimitiveIndexBuffers` `unordered_map<Primitive*, PrimitiveVulkanBuffer>` 既配線 = worker thread 分散余地確保済 |
| 23 | per-Asset cadence + per-Primitive ownership | PC-7γ-3 lazy register + PC-N-8 (e) `sCurrentPrimitive` 既配線 = worker thread context での per-Primitive iteration 整合 |
| 24 | Upstream OpenGL 取り込みやすさ | `recordGltfAssetDraw` signature 不変、`GLTFSceneManager::render` 改変 0 件 (= (N14-9) A `recordGltfAssetDraw` 内 launch + drain) |

---

## §3. ambiguity (N14-1)..(N14-20) 20 件 AYA literal「OK」record (2026-06-05) + 採用根拠

### §3.1 ⭐ critical: cmdbuf 分散方式 + granularity 系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N14-1) ⭐ | cmdbuf 分散方式 | **A**: **secondary command buffer (`VK_COMMAND_BUFFER_LEVEL_SECONDARY`) + `vkCmdExecuteCommands` 集約 pattern** = worker thread が secondary cmd_buf 独立記録、main thread が 1 回の `vkCmdExecuteCommands` で primary cmd_buf に集約 | OK (2026-06-05) | (E-14) B「UBO write + cmdbuf 両方並列化」literal 完全整合 + Vulkan 標準 multi-thread cmdbuf record pattern + main thread 単一 submit 維持で sync 簡素、case B (primary cmd_buf per-thread + multi-submit) は submit 順序 + GPU fence/semaphore 必要で複雑度過大、case C (UBO write のみ並列、cmdbuf main) は (E-14) B 違反 (= scope 縮小) |
| (N14-2) | worker thread granularity | **A**: **per-Primitive** | OK (2026-06-05) | PC-N-8 per-Primitive ownership 既配線、design 09 既述、設計原則 (2) Core プロセス分散実現の本丸、case B per-Asset は asset 数 = 2-10 想定で並列度限定、case C per-Skin batch は descriptor binding stale 化対策には有用だが granularity 中途半端 |

### §3.2 worker thread pool 設計系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N14-3) | thread count 決定方針 | **C**: **hybrid** = `AYAGltfWorkerThreadCount` cvar 経由制御、default=0 (= `std::thread::hardware_concurrency() - 1` auto, main thread 除外)、>0 = fixed thread count | OK (2026-06-05) | 開発時調整可 + default 自動 sizing、case A 固定は CPU 環境差異対応不可、case B fixed default 4 は HW 上限考慮なし、case C hybrid が最も flexible |
| (N14-4) | WorkQueue 種別 | **A**: **`LL::WorkQueue`** (LL 既存 work queue infra 流用) | OK (2026-06-05) | viewer 内整合性 (= LL 既存 thread pool infra 経路)、case B std::thread + std::queue 自前は LL framework 二重化、case C tbb は依存導入未確認 |
| (N14-5) | per-thread VkCommandPool | **A**: **per-worker thread 独立 `VkCommandPool` 必須** | OK (2026-06-05) | Vulkan spec 制約 (= `VkCommandPool` は thread external sync、同一 pool を複数 thread から触れない、選択肢なし) |
| (N14-6) | VMA allocator thread-safety | **A**: **VMA default thread-safe locking** 採用 (= 既 VMA allocator 単一 instance + 内部 mutex protection) | OK (2026-06-05) | VMA 標準対応で簡素、worker thread 数 (= 4-8 想定) で locking contention 限定的、case B per-thread VMA allocator は複雑度大で benefit 限定 |

### §3.3 既配線 storage thread-safety 系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N14-7) | `sDrawUboRingBufferMgr` / `sSkinUboDirty` thread-safety | **B**: **per-thread sub-ring buffer + main 集約 phase で merge** | OK (2026-06-05) | conflict 完全回避、worker thread 並列度維持、main 集約 phase で merge ゆえ flush API signature 不変、case A mutex protect allocate は contention bottleneck、case C atomic fetch_add offset は ring buffer wrap 周辺の lock-free 設計が複雑、case B が最も safe + simple |
| (N14-8) | file-static accessor 5 件 (`sCurrentAsset`/`sCurrentSkin`/`sCurrentPrimitive`/`sCurrentNodeAssetMatrix`/`sPcn13MultiAssetSeen`) thread context | **A**: **`thread_local` 修飾子追加のみ** (signature 不変、comment 更新のみ) | OK (2026-06-05) | layering 制約完全充足、accessor signature 不変、PC-N-8 (e) / PC-7γ-2 / PC-N-12 (c) / PC-N-13 (b) 既配線をそのまま thread_local 化のみで足る、case B per-Primitive parameter pack 化は signature 大変更で `recordGltfAssetDraw` (N8-6) A「signature 不変」破壊、case C 現状 file-static 維持は worker thread 並列化不能 (= (E-14) B 違反) |

### §3.4 launch / drain / sync 系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N14-9) | worker thread launch / drain 配置 | **A**: **`recordGltfAssetDraw` 内 per-Asset loop entry で launch + per-Asset loop exit で drain** | OK (2026-06-05) | Phase 1.E scope 内、Phase 1.F+ で expansion 余地、`GLTFSceneManager::render` caller-side 改変 0 件 (= 設計原則 (1) Upstream OpenGL 取り込みやすさ整合)、case B render 外別 phase launch は frame boundary 跨ぎで scope 拡張 |
| (N14-13) | main vs worker thread 同期 mechanism | **A**: **WaitForAll (worker drain) + `vkCmdExecuteCommands` で順序保持** | OK (2026-06-05) | Vulkan secondary cmd_buf 標準 pattern (= GPU 側 fence 不要、CPU 側 join で順序保持)、case B GPU fence/semaphore は per-Primitive 数だけ fence 増加で複雑、case C CPU barrier 簡素だが順序保持 explicit でない、case A が最も Vulkan 標準整合 |

### §3.5 cvar gate + first-fire marker 系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N14-10) | cvar gate 命名 | **A**: **`AYAGltfWorkerThreadEnabled`** Boolean default=0 Persist=1 + **`AYAGltfWorkerThreadCount`** U32 default=0 (= 0=auto, >0=fixed) | OK (2026-06-05) | (N14-3) C hybrid 整合、PC-N-11/12/13 per-sub-step cvar pattern 継続 (= (E-12) B literal は decomposition 段階の話、PC-N-11/12/13 で per-sub-step cvar 実装済の流れ整合)、worker thread infrastructure change は individual A/B testable 重要 |
| (N14-11) | settings.xml 配置 | **A**: **`AYAGltfMultiAssetCanary` 直後並列** (= Phase 1.E cvar group 末尾連続) | OK (2026-06-05) | PC-N-11/12/13 同形 pattern (= Phase 1.E cvar group 連続配置原則) |
| (N14-12) | first-fire `LL_INFOS` marker | **A**: **3 marker** = PC-N-14 (a) worker thread launch first-fire + PC-N-14 (b) secondary cmd_buf record first-fire + PC-N-14 (c) `vkCmdExecuteCommands` merge first-fire | OK (2026-06-05) | worker thread 経路 3 stage literal 取得、PC-N-6/7/8/9/10/11/12/13 同形 atomic flag pattern 拡張 (= `s_first_pcn14_worker_launch_fire` / `s_first_pcn14_secondary_record_fire` / `s_first_pcn14_execute_commands_fire`)、case B 1 marker のみは worker thread 経路の中間 stage literal 取得不可 |

### §3.6 検証 + 失敗時 escalation 系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N14-14) | integration 検証手順 | **A**: **`AYAGltfMultiAssetCanary` 既配線併用** + 専用 marker log + AYA 実機 multi-asset 同時 rez | OK (2026-06-05) | PC-N-13 (b) 既配線流用、scope 拡張回避、worker thread 並列度 literal は PC-N-13 (b) canary 経由で multi-asset 並列発火確認 + (N14-12) 3 marker log で worker thread 経路 literal 取得 |
| (N14-15) | 失敗時 escalation 経路 | **A**: PC-N-14 内 fix or 別 sub-step (= PC-N-14.1) 起案 = AYA 判断、設計段階では成功想定 | OK (2026-06-05) | PC-N-11/12/13 同形 escalation pattern、worker thread 設計は Vulkan spec + LL::WorkQueue 既配線 + secondary cmd_buf 標準 pattern ゆえ成功想定が design-lock default、失敗時の具体 fix path は実機現象見て初めて判断可能 |
| (N14-16) | build verify scope | **A**: **PC-N-6..13 同形** = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 + GATE-B integrity `LL_VULKAN_GLSL count llvkloader.cpp=6` 不変 | OK (2026-06-05) | 既配線 build verify pattern 踏襲、PC-N-13 commit `289d44b536` 同数想定 |

### §3.7 Exit + 改変規模 + design-lock 整合 系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N14-17) | Exit Criteria 項目数 | **A**: **10 項** (= PC-N-11/12/13 同形、PC-N-15 実装 phase 用) | OK (2026-06-05) | PC-N-14 = design-lock = Exit Criteria 9 項 / PC-N-15 = 実装 phase = Exit Criteria 10 項、PC-N-6..13 pattern 踏襲 |
| (N14-18) | PC-N-15 想定改変 file 件数 | **B**: **4 件** = (1) `indra/llrender/llvkloader.cpp` + (2) `indra/newview/app_settings/settings.xml` + (3) 本 cross-platform spec §6 PC-N-14/PC-N-15 行更新 + §A 履歴追記 + (4) new handoff PC-N-15 complete doc 起案。**別 design doc 新設不要** (= 本 PC-N-14 design-lock doc が PC-N-15 実装 source of truth) | OK (2026-06-05) | (E-9) B literal「必要時新設」= 不要なら新設しない、scope 拡張回避、本 PC-N-14 design-lock doc が ambiguity 20 件 + 実装計画 (a)-(k) 11 step + Exit Criteria 10 項を完全網羅ゆえ別 design doc 新設不要、PC-N-15 実装 phase は本 doc を必読 1 件として進む、case A 5 件 (= 新設 design doc) は冗長 |
| (N14-19) | design-lock phase `indra/` 改変 0 件 | **A**: 必須 (`feedback_design_phase_no_code_write` 厳格遵守) | OK (2026-06-05) | 全 sub-step 共通 design-lock 規則 |
| (N14-20) | PC-N-15 持越し範囲 | **A**: 実装 (worker thread 実装 + 検証) + cleanup (`sGltfStubSkin` sentinel storage 撤去 + 残余 placeholder 撤去 + `sGltfStubAssetPipeline` rename 候補保留 = (E-11) B Phase 1.F 候補) + Phase 1.E complete marker 起案 = 全 PC-N-15 持越し | OK (2026-06-05) | (E-10) B 整合「最終 sub-step = 実装 + cleanup + Phase 1.E complete marker 統合 1 sub-step」、`feedback_ubo_migration_one_at_a_time` 厳格遵守 |

---

## §4. 実装計画 (a)-(k) 11 step (= PC-N-15 別 session で着手)

> **注**: 本 §4 は **PC-N-15 実装 phase 用 step 分解 + 想定 code diff example** = 本 PC-N-14 design-lock phase は `indra/` 改変 0 件、実装は PC-N-15 で別 session 別途着手 (= `feedback_design_phase_no_code_write` + `feedback_ubo_migration_one_at_a_time` + (E-10) B 厳格遵守)。

### §4.1 step (a) — `AYAGltfWorkerThreadEnabled` + `AYAGltfWorkerThreadCount` cvar 新設 (settings.xml)

`indra/newview/app_settings/settings.xml` の既 `AYAGltfMultiAssetCanary` cvar 直後並列 ((N14-11) A) で `AYAGltfWorkerThreadEnabled` cvar 追加、その直後並列で `AYAGltfWorkerThreadCount` cvar 追加。

**想定 XML diff example**:

```xml
<key>AYAGltfWorkerThreadEnabled</key>
<map>
    <key>Comment</key>
    <string>
        AYAstorm r41 Phase 1.E PC-N-14/PC-N-15 = worker thread enable gate
        (= recordGltfAssetDraw per-Primitive UBO write + cmdbuf record を
        secondary command buffer 経由 worker thread 並列化、設計原則 (2)
        Core プロセス分散実現の本丸)。
        OFF (default) = 既 single-thread linear sequential record 維持
        (= Phase 1.D + PC-N-11 + PC-N-12 + PC-N-13 baseline 不変、MUSEUBO-A 整合)。
        ON = worker thread pool launch + per-Primitive secondary cmd_buf record +
        vkCmdExecuteCommands 集約 + first-fire marker 3 件起動。
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
        AYAstorm r41 Phase 1.E PC-N-14/PC-N-15 = worker thread count (= AYAGltfWorkerThreadEnabled=1 時のみ参照)。
        0 (default) = std::thread::hardware_concurrency() - 1 自動決定
        (= main thread 1 件除外、HW 並列度上限活用)。
        1-N = fixed worker thread count (= 開発時の A/B testing 用、上限は HW 並列度)。
        変更は AYAGltfWorkerThreadEnabled OFF → ON 切替時に反映 (= initVulkan 再走 or
        viewer 再起動で worker thread pool 再構築)。
    </string>
    <key>Persist</key>
    <integer>1</integer>
    <key>Type</key>
    <string>U32</string>
    <key>Value</key>
    <integer>0</integer>
</map>
```

### §4.2 step (b) — 冒頭 include 追加 + worker thread infrastructure storage 追加 (llvkloader.cpp)

`indra/llrender/llvkloader.cpp` 冒頭 include block に `LL::WorkQueue` ヘッダ追加 (= 既 LL framework header 経路、具体ヘッダ名は実装 phase で確認、想定 `#include "workqueue.h"` または `#include "llcoros.h"` 等)。anonymous namespace 内に per-worker thread storage 追加:

**想定 C++ diff example**:

```cpp
#include <thread>
#include <atomic>
#include <vector>
// LL::WorkQueue header (実装 phase で正確なパス確認)
// #include "workqueue.h"

namespace {
    // <AYAstorm r41 PC-N-14 (b)> worker thread infrastructure storage
    //   ((N14-4) A LL::WorkQueue + (N14-5) A per-thread VkCommandPool +
    //   (N14-10) A 2 cvar gate、AYA literal「OK」record 2026-06-05)。
    //   worker thread 数 N = AYAGltfWorkerThreadCount cvar=0 時
    //   std::thread::hardware_concurrency() - 1 (= main thread 除外)、
    //   >0 時 fixed N ((N14-3) C hybrid)。
    //   initVulkan 内で N 決定 + per-thread VkCommandPool + secondary
    //   VkCommandBuffer alloc、shutdownVulkan 内で対称 destroy ((N14-20) A
    //   PC-N-15 cleanup phase)。
    struct PcN14WorkerContext {
        VkCommandPool       mCommandPool        = VK_NULL_HANDLE;
        VkCommandBuffer     mSecondaryCmdBuf    = VK_NULL_HANDLE;
        // per-thread sub-ring buffer storage ((N14-7) B):
        // std::unique_ptr<LLUboRingBuffer> mDrawUboSubRing;
        // std::unordered_map<LL::GLTF::Skin*, /*dirty range*/> mSkinUboSubDirty;
    };
    std::vector<PcN14WorkerContext> sPcn14WorkerCtx;  // size = N worker threads
    std::atomic<U32>                sPcn14WorkerCount{0u};
    // </AYAstorm r41 PC-N-14 (b)>
}
```

### §4.3 step (c) — file-static accessor 5 件を `thread_local` 化 (llvkloader.cpp)

`sCurrentAsset` (`:668`) / `sCurrentSkin` (`:669`) / `sCurrentPrimitive` (`:679`) / `sCurrentNodeAssetMatrix` (`:696`) / `sPcn13MultiAssetSeen` (`:707`) の 5 file-static 宣言に `thread_local` 修飾子追加 ((N14-8) A)。signature 不変、comment 「main thread 専有」→「worker thread 内 thread_local 整合」更新。

**想定 C++ diff example**:

```cpp
// <AYAstorm r41 PC-N-12 (c)> ... thread_local 修飾子追加 ((N14-8) A、
//   worker thread 内 thread_local で per-thread context 維持、layering
//   制約完全充足、accessor signature 不変)。
thread_local const F32* sCurrentNodeAssetMatrix = nullptr;
// </AYAstorm r41 PC-N-12 (c)>
```

(他 4 件同形、`thread_local` 修飾子追加のみ + comment 更新)

### §4.4 step (d) — per-thread sub-ring buffer 化 (`sDrawUboRingBufferMgr` / `sSkinUboDirty`) ((N14-7) B)

`sDrawUboRingBufferMgr` (`:424`) を per-thread sub-ring buffer 化:
- worker thread context 内で `writeDrawUbo` 呼出は当該 thread の sub-ring に allocate
- main 集約 phase (= `vkCmdExecuteCommands` 直前) で sub-ring を main ring に merge

`sSkinUboDirty` 同形 = per-thread sub-map dirty + main 集約 phase で merge。

**設計詳細** (= PC-N-15 実装時に具体化):

```cpp
// worker thread 内 writeDrawUbo:
//   sPcn14WorkerCtx[tid].mDrawUboSubRing->allocate(...) を呼出
//   ((N14-7) B = per-thread sub-ring buffer pattern)
// main 集約 phase 内 flushDrawUbos:
//   sPcn14WorkerCtx[*].mDrawUboSubRing から sDrawUboRingBufferMgr に merge
//   + 既 flush API call (= dirty range vkFlushMappedMemoryRanges)
```

### §4.5 step (e) — PC-N-8 (f) real Asset path を worker thread 並列化 (llvkloader.cpp)

`recordGltfAssetDraw` PC-N-8 (f) real Asset path 全体を新規 `<AYAstorm r41 PC-N-14 (a)>` tag block で wrap ((N14-1) ⭐ A 採用):
- cvar gate (= `AYAGltfWorkerThreadEnabled` cvar=true 時 worker thread 経路 / false 時既 single-thread 経路)
- per-Primitive 単位で worker thread に task 投入
- 各 worker thread が secondary cmd_buf に `vkCmdBindDescriptorSets` / `vkCmdPushConstants` / `vkCmdBindVertexBuffers` / `vkCmdBindIndexBuffer` / `vkCmdDrawIndexed` を独立記録
- main thread が worker drain + 1 回の `vkCmdExecuteCommands` で primary cmd_buf に集約

**想定 C++ diff example** (= high-level structure):

```cpp
// <AYAstorm r41 PC-N-14 (a)> worker thread per-Primitive UBO write + cmdbuf
//   record 並列化 ((N14-1) ⭐ A secondary cmd_buf + vkCmdExecuteCommands 集約、
//   (N14-2) A per-Primitive granularity、(N14-9) A recordGltfAssetDraw 内
//   launch + drain、(N14-13) A WaitForAll + vkCmdExecuteCommands、AYA literal
//   「OK」record 2026-06-05)。
static LLCachedControl<bool> sAyastormGltfWorkerThreadEnabled(
    gSavedSettings, "AYAGltfWorkerThreadEnabled", false);

if (sAyastormGltfWorkerThreadEnabled)
{
    // PC-N-14 (a) first-fire marker ((N14-12) A 3 marker の第 1):
    static std::atomic<bool> s_first_pcn14_worker_launch_fire{true};
    if (s_first_pcn14_worker_launch_fire.exchange(false, std::memory_order_acq_rel))
    {
        LL_INFOS("Vulkan") << "PC-N-14 (a) worker thread launch first-fire: "
                              "AYAGltfWorkerThreadEnabled=true, "
                              "worker_count=" << sPcn14WorkerCount.load()
                           << ", granularity=per-Primitive, "
                              "cmdbuf=secondary + vkCmdExecuteCommands 集約 ((N14-1) A)。"
                           << LL_ENDL;
    }

    // per-Primitive task 投入 (= LL::WorkQueue 経由 (N14-4) A):
    //   each task: secondary cmd_buf に PC-N-11 (a) + PC-N-12 (a) + PC-N-13 (a) + (b) 既配線 inner block を実行
    //              + vkCmdBindDescriptorSets + vkCmdPushConstants + vkCmdBindVertexBuffers + vkCmdBindIndexBuffer + vkCmdDrawIndexed
    //   thread_local sCurrent* 5 件は worker thread launch 時 caller 値 propagate ((N14-8) A)

    // WaitForAll = worker drain ((N14-13) A):
    //   workqueue.drain() 等

    // main 集約 phase = sub-ring buffer merge + secondary cmd_buf 集約:
    //   for each worker_ctx: vkCmdExecuteCommands(cmd_buf, 1, &worker_ctx.mSecondaryCmdBuf);

    // PC-N-14 (c) first-fire marker ((N14-12) A 第 3):
    static std::atomic<bool> s_first_pcn14_execute_commands_fire{true};
    if (s_first_pcn14_execute_commands_fire.exchange(false, std::memory_order_acq_rel))
    {
        LL_INFOS("Vulkan") << "PC-N-14 (c) vkCmdExecuteCommands merge first-fire: "
                              "secondary_count=" << sPcn14WorkerCount.load()
                           << " (= worker_count)。"
                           << LL_ENDL;
    }
}
else
{
    // 既 single-thread linear sequential record path 維持 (MUSEUBO-A 整合)
    // ... 既 PC-N-8 (f) real Asset path body unchanged
}
// </AYAstorm r41 PC-N-14 (a)>
```

### §4.6 step (f) — secondary cmd_buf record 経路の inheritance info 設定 (llvkloader.cpp)

secondary cmd_buf の `vkBeginCommandBuffer` 呼出時に `VkCommandBufferInheritanceInfo` で primary cmd_buf の render pass + framebuffer + subpass index 引継ぎ:

**想定 C++ diff example**:

```cpp
// <AYAstorm r41 PC-N-14 (d)> secondary cmd_buf inheritance info 設定
//   (= Vulkan spec 必須、render pass + framebuffer + subpass index 引継ぎ)。
VkCommandBufferInheritanceInfo inheritance_info = {};
inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
inheritance_info.renderPass  = /* primary cmd_buf 現在の render pass */;
inheritance_info.framebuffer = /* primary cmd_buf 現在の framebuffer */;
inheritance_info.subpass     = /* primary cmd_buf 現在の subpass index */;

VkCommandBufferBeginInfo begin_info = {};
begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
begin_info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
begin_info.pInheritanceInfo = &inheritance_info;

vkBeginCommandBuffer(worker_ctx.mSecondaryCmdBuf, &begin_info);

// PC-N-14 (b) first-fire marker ((N14-12) A 第 2):
static std::atomic<bool> s_first_pcn14_secondary_record_fire{true};
if (s_first_pcn14_secondary_record_fire.exchange(false, std::memory_order_acq_rel))
{
    LL_INFOS("Vulkan") << "PC-N-14 (b) secondary cmd_buf record first-fire: "
                          "VK_COMMAND_BUFFER_LEVEL_SECONDARY + "
                          "VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT + "
                          "inheritance render_pass/framebuffer/subpass 設定済。"
                       << LL_ENDL;
}
// </AYAstorm r41 PC-N-14 (d)>
```

### §4.7 step (g) — initVulkan + shutdownVulkan で worker thread pool lifecycle (llvkloader.cpp)

initVulkan 内で:
- `AYAGltfWorkerThreadCount` cvar 読込 + `std::thread::hardware_concurrency() - 1` 自動決定 ((N14-3) C)
- `sPcn14WorkerCtx` array allocate (N 件)
- 各 worker thread 用 `VkCommandPool` create ((N14-5) A) + secondary `VkCommandBuffer` alloc

shutdownVulkan 内で対称 destroy ((N14-20) A PC-N-15 cleanup phase 整合):
- `vkFreeCommandBuffers` + `vkDestroyCommandPool`
- `sPcn14WorkerCtx` clear

### §4.8 step (h) — `sGltfStubSkin` sentinel storage 撤去 ((E-11) A + (N14-20) A、PC-N-15 cleanup phase)

PC-N-11 で (E-11) A literal「`sGltfStubSkin` sentinel 撤去は PC-N-15 cleanup phase 持越し」を採用済。PC-N-15 で撤去:
- `sGltfStubSkin` storage 削除
- PC-N-8 (f) 内 sentinel fall-through 経路撤去 (= PC-N-11 (a) 内 `skin_to_use` の null fallback を直接 `nullptr` または assertion に置換)
- `sGltfStubAssetPipeline` rename 候補 (= (E-11) B Phase 1.F 候補) は保留

### §4.9 step (i) — build verify literal 取得 ((N14-16) A)

PC-N-6..13 同形:
- `make -j4 llrender` → `[100%] Built target llrender` (= ERROR 0 / WARNING 0)
- `INTEGRATION_TEST_lluboringbuffer` → 11/11 PASS YAY!!
- `INTEGRATION_TEST_llassetubopool` → 10/10 PASS YAY!!
- `INTEGRATION_TEST_llpipelinecachestorage` → 13/13 PASS YAY!!
- `python3 -m unittest discover tests` (codegen at `scripts/ubo_codegen/`) → 131/131 OK
- `grep -c LL_VULKAN_GLSL indra/llrender/llvkloader.cpp` → 6 (= PC-N-13 commit `289d44b536` 同数、GATE-B integrity 維持)

### §4.10 step (j) — cross-platform spec §6 PC-N-14 + PC-N-15 行 ✅ 反映 + §A 履歴追記

PC-N-15 実装 complete 時:
- `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md` §6 PC-N-14 行 状態 ✅ 反映 (= design-lock complete) + PC-N-15 行 状態 ✅ 反映 (= 実装 complete = Phase 1.E complete marker)
- §A 履歴に PC-N-14 design-lock entry + PC-N-15 実装 complete entry の 2 行追記 (chronological order = design-lock entry → ✅ 反映 entry 順)

### §4.11 step (k) — PC-N-15 handoff complete doc 起案 + Phase 1.E complete marker 統合明示

PC-N-15 complete handoff doc 起案 = 本 PC-N-14 design-lock doc を必読 1 件として進む + Phase 1.E complete marker 統合明示 ((E-10) B literal「最終 sub-step = 実装 + cleanup + Phase 1.E complete marker 統合」)。

### §4.12 GATE-B 整合 (= `#ifdef LL_VULKAN_GLSL` 新規追加 0 件)

PC-N-14 改変は全て host-side C++ (= worker thread infra + thread_local 化 + cvar 追加 + secondary cmd_buf record)、shader/GLSL 改変なしゆえ `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 = `count llvkloader.cpp=6` 不変 (= PC-N-13 commit `289d44b536` 同数)。

### §4.13 MUSEUBO-A 整合 (= OpenGL 描画 100% 維持 + default OFF)

- `AYAGltfWorkerThreadEnabled=false` (default) で worker thread 経路 skip = 既 single-thread linear sequential record 維持 (= Phase 1.D + PC-N-11 + PC-N-12 + PC-N-13 baseline 不変)
- `AYAGltfWorkerThreadCount` は `AYAGltfWorkerThreadEnabled=true` 時のみ参照、default=0 で auto 決定ゆえ機能影響なし
- OpenGL 描画 100% 維持 (= Vulkan 経路全体が cvar gate 外側 PC-N-9 (a)/(b)/(c) で gate 済、PC-N-14 は Vulkan 経路内部 worker thread 分散のみ)

### §4.14 設計原則整合

- **(1) Upstream OpenGL 取り込みやすさ維持** = `recordGltfAssetDraw` signature 不変 + `GLTFSceneManager::render` caller-side 改変 0 件 (= (N14-9) A) + per-Primitive UBO write API signature 不変 (= (N14-7) B sub-ring 化は host-side detail) + shader 改変ゼロ
- **(2) Core プロセス分散実現** = worker thread + secondary cmd_buf + per-thread sub-ring buffer = per-Primitive granularity の UBO write + cmdbuf record 完全並列化 = (E-14) B literal 完全整合

---

## §5. PC-N-14 design-lock Exit Criteria (= 9 項全充足)

| # | criterion | status |
|---|-----------|--------|
| i | PC-N-14 literal scope §0 完全分解 3 項 (= per-Primitive UBO write 並列化 + per-Primitive cmdbuf record 並列化 + 2 cvar 新設) | ✅ §0 |
| ii | 必読 1 件 (= 本 PC-N-14 design-lock doc) + pinpoint reference 14 件 §1 別記 = full file dump なし | ✅ §1 |
| iii | 現状調査 §2 5 sub-section (= §2.1 recordGltfAssetDraw 構造 + §2.2 file-static accessor 5 件 + §2.3 既存 threading infra 0 件 + §2.4 既配線 tag block 5 件 + §2.5 設計原則整合) | ✅ §2 |
| iv | ambiguity (N14-1)..(N14-20) 20 件 AYA literal「OK」record (2026-06-05) §3 + 採用根拠 20 件明文化 (特に (N14-1) ⭐ critical A secondary cmd_buf + (N14-8) A `thread_local` 化 + (N14-10) A 2 cvar 新設 + (N14-18) B 別 design doc 新設不要) | ✅ §3 |
| v | 実装計画 (a)-(k) 11 step §4 + 各 step 想定 code diff example 添付 | ✅ §4 |
| vi | PC-N-15 実装 phase Exit Criteria 10 項明文化 (§6) | ✅ §6 |
| vii | GATE-B 整合 + MUSEUBO-A 整合 + 設計原則 (1)(2) 整合明文化 §4.12-§4.14 | ✅ §4.12-§4.14 |
| viii | 想定改変 file 4 件明文化 ((N14-18) B = 別 design doc 新設不要) | ✅ §3 (N14-18) |
| ix | `indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合 | ✅ 本 commit |

---

## §6. PC-N-15 実装 phase Exit Criteria (= 10 項全充足想定)

| # | criterion | 想定 status |
|---|-----------|-----------|
| i | settings.xml `AYAGltfWorkerThreadEnabled` + `AYAGltfWorkerThreadCount` 2 cvar 追加 (default=0 Persist=1、`AYAGltfMultiAssetCanary` 直後並列 = Phase 1.E cvar group 末尾連続) ((N14-10) A + (N14-11) A) | ⏳ PC-N-15 |
| ii | `recordGltfAssetDraw` PC-N-8 (f) real Asset path 全体を `<AYAstorm r41 PC-N-14 (a)>` tag block で wrap + cvar gate + worker thread launch + per-Primitive secondary cmd_buf 配給 + WaitForAll drain + `vkCmdExecuteCommands` 集約 ((N14-1) ⭐ A + (N14-2) A + (N14-9) A + (N14-13) A) | ⏳ PC-N-15 |
| iii | file-static accessor 5 件 (`sCurrentAsset`/`sCurrentSkin`/`sCurrentPrimitive`/`sCurrentNodeAssetMatrix`/`sPcn13MultiAssetSeen`) を `thread_local` 化 ((N14-8) A、signature 不変、comment 更新) | ⏳ PC-N-15 |
| iv | `sDrawUboRingBufferMgr` / `sSkinUboDirty` per-thread sub-ring/sub-map 化 + main 集約 phase で merge ((N14-7) B) | ⏳ PC-N-15 |
| v | per-worker thread `VkCommandPool` + secondary `VkCommandBuffer` storage `sPcn14WorkerCtx` array 新設 + initVulkan/shutdownVulkan lifecycle 配線 ((N14-5) A + (N14-6) A + (N14-20) A) | ⏳ PC-N-15 |
| vi | PC-N-14 (a) worker thread launch + (b) secondary cmd_buf record + (c) `vkCmdExecuteCommands` merge の 3 first-fire `LL_INFOS` marker ((N14-12) A、`s_first_pcn14_*_fire` atomic flag 3 件、PC-N-6..13 同形 pattern) | ⏳ PC-N-15 |
| vii | `sGltfStubSkin` sentinel storage 撤去 + PC-N-8 (f) sentinel fall-through 経路撤去 ((E-11) A + (N14-20) A、PC-N-15 cleanup phase) | ⏳ PC-N-15 |
| viii | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp=6 不変、PC-N-13 commit `289d44b536` 同数) + MUSEUBO-A 整合 = `AYAGltfWorkerThreadEnabled=false` default で既 single-thread linear sequential record 維持 + OpenGL 描画 100% 維持 ((N14-16) A) | ⏳ PC-N-15 |
| ix | build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 + GATE-B integrity ((N14-16) A) + cross-platform spec §6 PC-N-14 + PC-N-15 行 ✅ 反映 + §A 履歴 2 行追記 ((N14-18) B) + handoff PC-N-15 complete doc 起案 + Phase 1.E complete marker 統合明示 ((E-10) B) | ⏳ PC-N-15 |
| x | self-verify 9 観点 全 ✅ | ⏳ PC-N-15 |

---

## §7. PC-N-15 着手手順 11 step

1. 本 PC-N-14 design-lock doc 全文 Read (= 必読 1 件)
2. pinpoint reference 14 件 (§1.2) を必要分のみ Read (= full file dump 禁止、`feedback_handoff_minimal_pre_req_read` 整合)
3. step (a) settings.xml 2 cvar 追加
4. step (b) include 追加 + `sPcn14WorkerCtx` array storage + 2 cvar `LLCachedControl` 宣言
5. step (c) file-static accessor 5 件 `thread_local` 化
6. step (d) per-thread sub-ring buffer 化
7. step (e)+(f) PC-N-8 (f) を `<AYAstorm r41 PC-N-14 (a)>` tag block で wrap + secondary cmd_buf record + inheritance info + 3 first-fire marker
8. step (g) initVulkan + shutdownVulkan lifecycle 配線
9. step (h) `sGltfStubSkin` sentinel storage 撤去 (cleanup phase)
10. step (i) build verify literal 取得 + GATE-B integrity 確認
11. step (j)+(k) cross-platform spec §6 PC-N-14 + PC-N-15 行 ✅ 反映 + §A 履歴 2 行追記 + handoff PC-N-15 complete doc 起案 + Phase 1.E complete marker 統合明示 → AYA 明示 commit 指示受領後 commit

---

## §8. 残 strict 線形

- ✅ Phase 1.A / 1.B / (Z) SSS / (W) uniform4iv / (Y) Phase 1.C prep
- ✅ PC-0..PC-7ε / PC-N decomposition / PC-N-1..PC-N-4 (= Phase 1.C complete)
- ✅ PC-8 Linux primary marker (= Phase 1.C strict 線形終了)
- ✅ PC-N-5 (= Phase 1.D 着手起点) / Phase 1.D decomposition design-lock
- ✅ PC-N-6 / PC-N-7 / PC-N-8 / PC-N-9 / PC-N-10 (= Phase 1.D complete = 1 GLTF asset 完全 Vulkan draw 通電 達成)
- ✅ Phase 1.E decomposition design-lock (commit `094546889b`) + PC-N-11 design-lock (commit `87560a4dc7`) + PC-N-11 実装 (commit `797332ee81`) + PC-N-12 design-lock (commit `9a62f11416`) + PC-N-12 実装 (commit `b6b39bfd9f`) + PC-N-13 design-lock (commit `cd253cb754`) + PC-N-13 実装 (commit `289d44b536`)
- ✅ **PC-N-14 design-lock ✅ 本 commit = Phase 1.E 内 4th sub-step design-lock complete = worker thread design (= per-Primitive UBO write + cmdbuf record 並列化 design + ambiguity 20 件 resolve + 実装計画 11 step + Exit Criteria 10 項)**
- ⏳ PC-N-15 実装 + cleanup (= worker thread 実装 + `sGltfStubSkin` sentinel storage 撤去 + Phase 1.E complete marker 起案)
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
PC-N-13 ✅ + **PC-N-14 design-lock ✅ 本 commit = Phase 1.E 内 4th sub-step
design-lock complete = worker thread design (= per-Primitive UBO write +
cmdbuf record 並列化 design + ambiguity 20 件 resolve + 実装計画 11 step +
Exit Criteria 10 項)** +
⏳ PC-N-15 実装 + cleanup + Phase 1.E complete + Phase 1 全完了 +
Mac/Win 開発者補完 phase

---

## §10. self-verify 9 観点 全 ✅

1. ✅ PC-N-14 literal scope §0 完全分解 3 項 (= AYA task statement literal が source of truth = per-Primitive UBO write 並列化 + per-Primitive cmdbuf record 並列化 + 2 cvar 新設)
2. ✅ 必読 1 件 §1.1 + pinpoint reference 14 件 §1.2 別記 = full file dump なし (= `feedback_handoff_minimal_pre_req_read` 整合)
3. ✅ 現状調査 §2 5 sub-section 網羅 (= §2.1 `recordGltfAssetDraw` 構造 + §2.2 file-static accessor 5 件 thread-safety + §2.3 既存 threading infra 0 件 = 全 PC-N-14 新設対象 + §2.4 既配線 tag block 5 件 + §2.5 設計原則整合)
4. ✅ ambiguity (N14-1)..(N14-20) 20 件 AYA literal「OK」record (2026-06-05) §3 + 採用根拠 20 件明文化 (特に (N14-1) ⭐ critical A secondary cmd_buf + vkCmdExecuteCommands 集約は (E-14) B literal 完全整合 + (N14-8) A `thread_local` 化は layering 制約完全充足 + accessor signature 不変 + (N14-10) A 2 cvar 新設は PC-N-11/12/13 per-sub-step cvar pattern 継続 + (N14-18) B 別 design doc 新設不要は (E-9) B literal「必要時新設」整合)
5. ✅ 実装計画 (a)-(k) 11 step §4 + 各 step 想定 code diff example 添付 (特に step (e) PC-N-14 (a) tag block + cvar gate + worker thread launch + secondary cmd_buf 配給 + WaitForAll drain + `vkCmdExecuteCommands` 集約 + 3 first-fire marker)
6. ✅ GATE-B 整合 §4.12 (= `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、AYAGltfWorkerThreadEnabled + AYAGltfWorkerThreadCount cvar runtime gate のみ、shader 改変ゼロ)
7. ✅ MUSEUBO-A 整合 §4.13 (= `AYAGltfWorkerThreadEnabled=false` default で既 single-thread linear sequential record 維持 + `AYAGltfWorkerThreadCount=0` default で auto 決定ゆえ機能影響なし + OpenGL 描画 100% 維持 + Vulkan 経路全体は PC-N-9 (a)/(b)/(c) で gate 済)
8. ✅ 設計原則整合 §4.14 (= (1) Upstream OpenGL 取り込みやすさ維持 = `recordGltfAssetDraw` signature 不変 + `GLTFSceneManager::render` caller-side 改変 0 件 + per-Primitive UBO write API signature 不変 + shader 改変ゼロ + (2) Core プロセス分散実現 = worker thread + secondary cmd_buf + per-thread sub-ring buffer = per-Primitive granularity の UBO write + cmdbuf record 完全並列化 = (E-14) B literal 完全整合)
9. ✅ `indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合

---

## §11. 次 session 着手 1 line

PC-N-15 実装着手 = step (a)-(k) 11 step 実施 = (a) settings.xml `AYAGltfWorkerThreadEnabled` + `AYAGltfWorkerThreadCount` 2 cvar 追加 + (b) include + `sPcn14WorkerCtx` storage + (c) file-static accessor 5 件 `thread_local` 化 + (d) per-thread sub-ring buffer 化 + (e)+(f) PC-N-8 (f) を `<AYAstorm r41 PC-N-14 (a)>` tag block で wrap + secondary cmd_buf record + inheritance info + 3 first-fire marker + (g) initVulkan + shutdownVulkan lifecycle + (h) `sGltfStubSkin` sentinel storage 撤去 (cleanup) + (i) build verify literal 取得 + (j) cross-platform spec §6 PC-N-14 + PC-N-15 行 ✅ 反映 + §A 履歴 2 行追記 + (k) handoff PC-N-15 complete doc 起案 + Phase 1.E complete marker 統合明示 = `feedback_ubo_migration_one_at_a_time` 厳格遵守で本 PC-N-14 design-lock baseline 上に Phase 1.E 内 5th = 最終 sub-step として別 session 実装。

---

## §A. feedback 遵守 record

- ✅ `feedback_proactive_handoff` (本 PC-N-14 design-lock handoff doc 起案)
- ✅ `feedback_handoff_minimal_pre_req_read` (必読 1 件 = 本 PC-N-14 design-lock doc + pinpoint reference 14 件別記、full file dump なし)
- ✅ `feedback_self_verify_before_handoff` (9 観点 self-verify 全 ✅)
- ✅ `feedback_build_only_verified` (design-lock phase は `indra/` 改変 0 件で build verify 対象外、PC-N-15 実装 phase で literal 検証取得予定 (N14-16) A 採用)
- ✅ `feedback_no_scope_shrink` (PC-N-14 literal scope §0 完全分解 3 項 = AYA task statement literal + (E-9) B + (E-14) B + (E-3) A 整合が source of truth、(N14-18) B 別 design doc 新設不要は scope 縮小ではなく重複回避、PC-N-15 持越し ((N14-20) A) は (E-10) B literal「最終 sub-step = 実装 + cleanup + Phase 1.E complete marker 統合」整合)
- ✅ `feedback_doubt_self_first` (design-lock phase で ambiguity 20 件発見 + 推奨案提示 + AYA literal「OK」record 後本 design-lock doc 起案、特に (N14-1) ⭐ critical = (E-14) B literal「UBO write + cmdbuf 両方並列化」整合の唯一案として secondary cmd_buf + `vkCmdExecuteCommands` 集約 pattern を 3 案検討後採用、推測実装なし)
- ✅ `feedback_admit_unknown` (現状調査 §2.3 で `sDrawUboRingBufferMgr` 内部 thread-safety **未確認** + `Skin::uploadMatrixPalette` worker thread context 移行可否 **要確認** を明示、PC-N-15 実装時に具体化、勝手に推測しない)
- ✅ `feedback_confirm_referent_before_acting` (20 件 batch AYA 確認 design-lock phase で完了 = AYA literal「OK」record 2026-06-05、推測実装なし)
- ✅ `feedback_ubo_migration_one_at_a_time` 厳格遵守 (PC-N-14 = worker thread design 単独 sub-step、PC-N-15 (= 実装 + cleanup + Phase 1.E complete marker 統合) は別 phase の別 session で別途 (E-10) B 整合)
- ✅ `feedback_design_phase_no_code_write` 厳格遵守 (本 PC-N-14 design-lock phase は doc 起案 + cross-platform spec §6 PC-N-14 行更新のみ、`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件)
- ✅ `feedback_release_branch_workflow` (feature branch `feature/ayastorm-r41-gl-removal` 上 commit 予定)
- ✅ `feedback_no_auto_commit` (AYA 明示 commit 指示受領後 commit 予定)
- ✅ `feedback_no_claude_coauthor` / Co-Authored-By 行不在予定
- ✅ `feedback_no_bare_reference_ids` ((N14-1)..(N14-20) 各 ID に項目名 / 採用案内容併記 + (a)..(k) 各 step に作業内容併記 + (E-3)/(E-9)/(E-10)/(E-11)/(E-12)/(E-14) 各 reference に Phase 1.E decomposition 内項目名併記)
- ✅ `feedback_tests_dir_never_commit` 整合 (tests/ 改変 0 件、git add 個別 file 指定予定)
- ✅ memory `project_ayastorm_r41_design_principles` 整合
  ((1) Upstream OpenGL 取り込みやすさ維持 = `recordGltfAssetDraw` signature 不変 + `GLTFSceneManager::render` caller-side 改変 0 件 ((N14-9) A) + per-Primitive UBO write API signature 不変 ((N14-7) B sub-ring 化は host-side detail) + `thread_local` 修飾子追加のみで accessor signature 不変 ((N14-8) A) + shader 改変ゼロ + (2) Core プロセス分散実現 = worker thread + secondary cmd_buf + per-thread sub-ring buffer = per-Primitive granularity の UBO write + cmdbuf record 完全並列化 = (E-14) B literal 完全整合)
- ✅ memory `project_r41_phase1b_vulkan_host_gate` 整合 (GATE-B = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 ((N14-16) A)、`AYAGltfWorkerThreadEnabled` + `AYAGltfWorkerThreadCount` cvar runtime gate のみ ((N14-10) A)、count llvkloader.cpp=6 不変想定 (= PC-N-13 commit `289d44b536` 同数))
- ✅ memory `project_ayastorm_three_platforms` 整合 (cross-platform spec §6 PC-N-14 行 design-lock 内容更新で macOS / Windows 派生 fix 候補なし想定 = host-side threading design ((N14-4) A LL::WorkQueue) は OS 非依存 + `VK_COMMAND_BUFFER_LEVEL_SECONDARY` + `vkCmdExecuteCommands` は MoltenVK 標準対応範囲 (= Metal parallel render encoder 経路) + per-thread `VkCommandPool` ((N14-5) A) は Vulkan spec 必須で MoltenVK 標準対応 + `thread_local` accessor ((N14-8) A) は C++ 標準で OS 非依存 + VMA default thread-safe locking ((N14-6) A) は MoltenVK 標準対応 + 2 cvar XML は OS 非依存 ゆえ macOS 派生 fix 候補なし想定 + Windows full Vulkan ゆえ派生 fix 候補なし想定、Linux primary 完成 → 他者補完 model 整合)
