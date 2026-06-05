# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 **Phase 1.E complete marker** = Phase 1 全完了 marker = Mac/Win 開発者補完 phase entry

## §0. 着手契機 + 位置付け

**着手契機**: PC-N-15c 実装完了 (= Phase 1.E 内 7th = 最終 sub-step 実装完了 = cleanup + sGltfStubSkin sentinel 撤去 + AYAGltfMultiSkinEnabled cvar 撤去 + AYAGltfRealDrawEnabled cvar + recordAvatarPlaceholderDraw 末尾 entry hook 撤去) + AYA live verify「通常通りに描画されてます」record 2026-06-06 受領 → **Phase 1.E 全体総括 + Phase 1 全完了 marker + Mac/Win 補完 phase entry 移行 marker** として本 doc 起案 (= 案 B 統合方針整合 = PC-N-15c 完了 handoff = Phase 1.E complete marker 兼用、(N15c-7) A 採用)。

**位置付け**: r41 milestone 内 **Phase 1 (Linux primary 完成) complete marker** = 個別 sub-step 完了 handoff (= PC-N-* complete handoff) と異なり **Phase 1.E + Phase 1 全体総括** = (1) PC-N-11..PC-N-15c 完了状態 + (2) 実 data 通電 + multi-asset / multi-skin / real modelview / real per-draw light params / worker thread 並列化 全達成 + (3) Phase 1 全完了状態確認 + (4) Mac/Win 補完 phase entry 起点。本 doc 完了で r41 Phase 1 (Linux primary) closure + 次 phase (= Mac/Win 開発者補完) への bridge 確立。

## §1. Phase 1.E 全 sub-step 完了状態 (= 7 sub-step 累積)

| # | sub-step | scope | 完了 commit | 状態 |
|---|---------|-------|-------------|------|
| 1 | **PC-N-11** | multi-skin sentinel 段階卒業 cvar 配線 = `AYAGltfMultiSkinEnabled` cvar + `recordGltfAssetDraw` PC-N-11 (a) fall-through path + real Skin owner `sCurrentSkin` 経由切替 | `797332ee81` | ✅ |
| 2 | **PC-N-12** | real node modelview push constant 配線 = `AYAGltfRealModelviewEnabled` cvar + caller (gltfscenemanager.cpp per-Primitive loop) で `glm::value_ptr(node.mAssetMatrix)` 経由 raw column-major 64 B pointer 投入 + `sCurrentNodeAssetMatrix` accessor 経由 layering-safe pointer pattern | `b6b39bfd9f` | ✅ |
| 3 | **PC-N-13** | real per-draw light params cvar gate + zero IS real data semantic 確立 + multi-asset canary 配線 = `AYAGltfRealLightParamsEnabled` cvar + `AYAGltfMultiAssetCanary` cvar + `sPcn13MultiAssetSeen` `std::unordered_set<const void*>` + `size() > 1u` 判定 + first-fire marker | `289d44b536` | ✅ |
| 4 | **PC-N-14** | worker thread design-lock = `LL::WorkQueue` infrastructure 採用 + per-thread sub-ring 採用 + secondary cmdbuf + `vkCmdExecuteCommands` 集約 pattern + `VkCommandBufferInheritanceRenderingInfoKHR` 経由 dynamic rendering scope 継承 design | `e96f7e2a68` | ✅ |
| 5 | **PC-N-15a** | worker thread infrastructure 配線 = `LLUboRingBuffer` per-thread instance refactor (class 改変 0 件 = constructor `BufferAllocator`/`BufferDestroyer` `std::function` injection pattern + unique_ptr) + `thread_local` 4 件 (`sCurrentAsset`/`sCurrentSkin`/`sCurrentPrimitive`/`sCurrentNodeAssetMatrix`) + per-thread `VkCommandPool` + secondary `VkCommandBuffer` allocate + `AYAGltfWorkerThreadEnabled` + `AYAGltfWorkerThreadCount` 2 cvar + initVulkan/shutdownVulkan lifecycle | `708b8ff8a4` | ✅ |
| 6 | **PC-N-15b** | worker thread dispatch 配線 = `LL::WorkQueue` + per-Primitive `postPrimitiveToWorker` hook + secondary cmdbuf record + `vkCmdExecuteCommands` 集約 + `writeDrawUbo`/`writeSkinUbo` thread-aware 拡張 + `sPcn13MultiAssetSeen` mutex 保護 + first-fire marker 3 件 (`s_first_pcn14_worker_thread_fire` + `s_first_pcn14_secondary_cmdbuf_fire` + `s_first_pcn14_ubo_parallel_fire`) | `159a8e3271` | ✅ |
| 6.5 | **bridge** (newview link fix) | `primitive.h` class Primitive 閉じ `};` 補填 (PC-N-8 commit `f68fd15e15` brace bug 修正) + Firestorm fork patch 整合 test 群 build 復旧 + .gitignore root-anchor 訂正 | `cc027bb835` | ✅ |
| 6.7 | **bridge hotfix 2 件** | `bindV3aRigged` `firstSet=3` → `firstSet=4` (= Vulkan slot vs design naming 混同 fix、Asset descriptor set を Asset layout slot に正しく bind) + `drainWorkersAndExecute` 冒頭 `AYAGltfWorkerThreadEnabled` cvar guard 追加 (= `postPrimitiveToWorker` 対称 gate) | `95fdbdedac` | ✅ |
| 7 | **PC-N-15c** | cleanup + Phase 1.E complete marker = `sGltfStubSkin` storage + register/unregister 配線完全撤去 + `AYAGltfMultiSkinEnabled` cvar + `recordGltfAssetDraw` PC-N-11 (a) fall-through path 完全撤去 (= real Skin path 一本化) + `AYAGltfRealDrawEnabled` cvar + `recordAvatarPlaceholderDraw` 末尾 PC-N-10 (a) entry hook 撤去 + Phase 1.E complete marker 起案 (= 本 doc) | (本 commit) | ✅ |

**bridge commits 6.5/6.7 補足**: PC-N-15b 完了直後の AYA live verify 段階で発覚した 2 件の問題 = (1) newview link build pre-existing error (= PC-N-8 commit f68fd15e15 の `primitive.h` brace 削除回し忘れによる cascade error)、(2) `bindV3aRigged` 内 Vulkan slot 番号誤解釈による Avatar pool draw 時 SIGSEGV。両方 PC-N-15c 着手の N15c-12 timing gate「PC-N-15b live verify PASS 後着手」を物理的に充足するため bridge phase として独立 commit、PC-N-15c 着手前提整備として位置付け。

## §2. 達成事項列挙 = Phase 1.E core deliverables

### §2.1 実 data 通電 (= zero placeholder からの卒業)

- **real GLTF asset path** (PC-N-8 baseline → PC-N-15c 完了): `LL::GLTF::Asset` 経由 vertex/index buffer Vulkan infrastructure → real Primitive 由来 vertex/index buffer bind + `vkCmdDrawIndexed(real_index_count, 1, 0, 0, 0)` 通電 (= sGltfStubAssetPipeline 流用、shader 改変ゼロ)
- **real Skin matrix palette path** (PC-N-11 → PC-N-15c real Skin path 一本化): upstream `Skin::uploadMatrixPalette` PC-7γ-3 (j) dual-write 既書込 real bone matrix palette を消費 + `wireSkinUboSetV3aToBinding2(sCurrentSkin)` per-draw rewire で multi-skin descriptor binding 正確性確保
- **real node modelview matrix path** (PC-N-12): caller (gltfscenemanager.cpp per-Primitive loop) で `glm::value_ptr(node.mAssetMatrix)` 経由 raw column-major 64 B pointer を `sCurrentNodeAssetMatrix` accessor 経由投入 → `vkCmdPushConstants` で direct 送出 (= upstream `Asset::uploadTransforms` asset.cpp:180 と同 source 流用)
- **real per-draw light params path** (PC-N-13 + 「zero IS real data」semantic 確立): 現 phase は sGltfStubAssetPipeline 流用 sky_smoke shader が PerDrawUBO_LightParams を非 consume = host write 256 B zero buffer が descriptor set layout 充足の architectural truth、`AYAGltfRealLightParamsEnabled` cvar gate + first-fire marker で literal 取得、Phase 1.F+ 実 PBR shader 接続時に data 内容置換予定

### §2.2 multi-asset / multi-skin support

- **multi-asset canary** (PC-N-13 (b) + PC-N-15b (f) mutex 保護): `sPcn13MultiAssetSeen` `std::unordered_set<const void*>` + `std::mutex sPcn13MultiAssetSeenMutex` 保護 + `size() > 1u` 検出時 first-fire marker (= 複数 GLTF asset 同時 iterate 検証 PASS literal 取得用、debug-only canary)
- **multi-skin real Skin path 一本化** (PC-N-11 → PC-N-15c sentinel 撤去): `sCurrentSkin` 経由 owner 解決 + `wireSkinUboSetV3aToBinding2(sCurrentSkin)` per-draw rewire で frame 内 2nd Skin register 時 binding=2 上書き → 1st Skin binding stale 化 risk を per-draw rewire で吸収 ((N11-5) A)、PC-N-15c で sGltfStubSkin sentinel fall-through 撤去で multi-skin gate 完全卒業

### §2.3 worker thread 並列化 = Core プロセス分散実現 (= 設計原則 (2))

- **worker thread infrastructure** (PC-N-15a): `LLUboRingBuffer` per-thread instance refactor (class 改変 0 件) + `thread_local` 4 件 + per-thread `VkCommandPool` + secondary `VkCommandBuffer` + 2 cvar (`AYAGltfWorkerThreadEnabled` + `AYAGltfWorkerThreadCount`) + initVulkan/shutdownVulkan lifecycle
- **worker thread dispatch** (PC-N-15b): `LL::WorkQueue` post/drain + per-Primitive `postPrimitiveToWorker` hook (gltfscenemanager.cpp per-Primitive loop 内、`setCurrentPrimitive` 直後並列) + secondary cmdbuf record (`VkCommandBufferInheritanceRenderingInfoKHR` 経由 dynamic rendering scope 継承) + `vkCmdExecuteCommands` 集約 (per-Asset 末尾 `drainWorkersAndExecute`) + `writeDrawUbo`/`writeSkinUbo` thread-aware 拡張 + first-fire marker 3 件
- **MUSEUBO-A 整合維持**: `AYAGltfWorkerThreadEnabled=false` default で worker thread infra 起動するが post 経路で即時 false 返却 = main thread 経路完全維持 + OpenGL 描画 100% 維持
- **AYA live verify「OK ログインできました」record 2026-06-06** = worker thread infra 起動成功 + bindV3aRigged firstSet=4 fix 後 crash 解消立証

### §2.4 cleanup (PC-N-15c) で達成された一本化

- **sGltfStubSkin sentinel 撤去**: PC-N-5 (a) 導入 address-only sentinel + register/unregister 配線完全撤去、real Skin path 一本化
- **AYAGltfMultiSkinEnabled cvar 撤去**: sentinel fall-through path 物理消失で段階卒業 gate 役目終了
- **AYAGltfRealDrawEnabled cvar 撤去**: PC-N-15b worker thread dispatch 配線後 `recordAvatarPlaceholderDraw` 末尾 entry hook が dead code 化ゆえ撤去
- **維持される cvar (= 機能 gate)**: `AYAGltfRealModelviewEnabled` + `AYAGltfRealLightParamsEnabled` + `AYAGltfMultiAssetCanary` + `AYAGltfWorkerThreadEnabled` + `AYAGltfWorkerThreadCount` ((N15c-4) A)
- **維持される sentinel**: `sGltfStubAssetPipeline` (= Phase 1.F+ 実 PBR shader 接続まで再利用、(N15c-5) A) + `sPlaceholderSkin` (= avatar Vulkan draw 通電 phase 持越し、(N15c-6) A)

## §3. Phase 1 全完了状態確認

### §3.1 Phase 1 全 sub-phase 完了

| sub-phase | scope | 状態 |
|-----------|-------|------|
| **Phase 1.A** | Vulkan loader 基盤 + descriptor set layout V3a 5-set + pipeline layout sAYAStandardLayout + per-frame UBO + ring buffer 基盤 | ✅ |
| **Phase 1.B** | C++ Vulkan gate (`mUseUBO` runtime flag、`LL_VULKAN_GLSL` macro は GLSL 専用) + GATE-B 確立 | ✅ |
| **(Z) SSS / (W) uniform4iv** | upstream fix accommodation | ✅ |
| **(Y) Phase 1.C prep** | preparatory cleanup | ✅ |
| **Phase 1.C** (PC-0..PC-7ε + PC-N-1..PC-N-4) | per-program / per-singleton UBO cadence + sky_smoke placeholder + bindV3aStatic/Rigged + writeDrawUbo / writeSkinUbo API + ring buffer grow auto re-wire | ✅ |
| **PC-8 Linux primary marker + PC-N-5** | Phase 1.D 着手起点 = sGltfStubSkin sentinel register + identity matrix bone data 通電 baseline | ✅ |
| **Phase 1.D** (PC-N-6..PC-N-10) | 実 LL::GLTF::Asset 経由 vertex/index buffer Vulkan infrastructure 通電 + Phase 1.D complete marker | ✅ |
| **Phase 1.E** (PC-N-11..PC-N-15c) | 実 data 通電 + multi-asset / multi-skin / real modelview / real per-draw light params + worker thread 並列化 + cleanup | ✅ 本 commit |

### §3.2 設計原則 (= memory `project_ayastorm_r41_design_principles`) 充足

- **(1) Upstream OpenGL 取り込みやすさ維持**: call site API 温存 (= `recordGltfAssetDraw` / `recordAvatarPlaceholderDraw` signature 不変 + `writeDrawUbo` / `writeSkinUbo` / `bindV3aRigged` accessor signature 不変 + `LLUboRingBuffer` class 改変 0 件 + shader 改変ゼロ + `gltf/primitive.h` から `vulkan/vulkan.h` include 回避)、`PrimitiveVulkanBuffer` struct + 6 新 API は LLVKLoader 内 encapsulate
- **(2) Core プロセス分散実現**: per-thread `LLUboRingBuffer` instance + per-thread `VkCommandPool` + secondary `VkCommandBuffer` + worker thread per-Primitive dispatch で primitive-level granularity 達成 + UBO/cmdbuf 並列化容易な設計成立

### §3.3 MUSEUBO-A 整合維持 (= 全 cvar OFF default で OpenGL 描画 100% 維持)

- 全実装 cvar default OFF 状態で Phase 1.D + 1.E complete baseline = Phase 1.D complete (= PC-N-10 commit `16a26f6272`) と機能等価
- PC-N-15c で cvar 2 件撤去 (`AYAGltfMultiSkinEnabled` + `AYAGltfRealDrawEnabled`) だが残存 cvar (= `AYAGltfWorkerThreadEnabled` 等) default OFF で baseline 維持
- AYA live verify 2026-06-06「通常通りに描画されてます」record で OpenGL 経路維持確認

### §3.4 GATE-B 整合 (= memory `project_r41_phase1b_vulkan_host_gate`)

- `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count `llvkloader.cpp` = 6 不変、Phase 1.A 確立時点同数)
- `mUseUBO` runtime flag 経路のみで C++ Vulkan gate 制御、GLSL `LL_VULKAN_GLSL` macro は GLSL TU 専用

## §4. Mac/Win 開発者補完 phase entry

### §4.1 Linux primary 完成状態 = 他 OS 補完 model 起点

Phase 1 全完了 = **Linux primary 環境での r41 Phase 1 (Vulkan path 通電 baseline) 完成**。次 phase = **Mac/Win 開発者補完 phase** = 他 OS 開発者が Linux primary を base に MoltenVK (macOS) / Windows full Vulkan の派生 fix を投入する model。

### §4.2 cross-platform spec 補足 (= `ayastorm-r41-cross-platform-port-spec.md`)

- **macOS MoltenVK**: 本 Phase 1.E 実装は host-side worker thread infrastructure + per-thread VkCommandPool + secondary VkCommandBuffer + `VkCommandBufferInheritanceRenderingInfoKHR` + descriptor set 数 5 維持 = MoltenVK 標準対応範囲想定、Vulkan 1.3 dynamic rendering inheritance MoltenVK 対応確認要 (= 派生 fix 候補)、`LLUboRingBuffer` per-instance VMA HOST_VISIBLE+MAPPED は OS 非依存
- **Windows full Vulkan**: 全実装 host-side stdlib (`std::thread` + `std::mutex` + `std::vector` + `std::unique_ptr`) + Vulkan 標準 API のみ使用 = 派生 fix 候補なし想定、`AYAGltfWorkerThreadCount` cvar (= `std::thread::hardware_concurrency()` 経由 worker 数決定) は OS 非依存

### §4.3 Mac/Win 補完 phase での着手候補

- (a) macOS MoltenVK 環境で本 binary build + 起動確認 (= MoltenVK runtime での `vkCmdBindDescriptorSets` 5-slot pipeline layout + secondary cmdbuf inheritance 動作確認)
- (b) Windows full Vulkan 環境で本 binary build + 起動確認 (= Windows-specific `std::thread::hardware_concurrency()` 値検証 + worker_count 経路)
- (c) GLTF asset rez シナリオで `s_first_pcn14_secondary_cmdbuf_fire` + `s_first_pcn14_ubo_parallel_fire` first-fire marker 3 件全発火確認 (Linux でも未確認、scene に GLTF asset 不在ゆえ Phase 1.E live verify では発火せず、別 phase で rez verify)
- (d) Phase 1.F (実 PBR shader 接続 + zero placeholder data から real data 内容置換) 着手前提整備

## §5. 残 strict 線形 + 次 phase entry

- Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α..PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1..PC-N-4 ✅ = **Phase 1.C complete ✅**
- PC-8 Linux primary marker ✅ + PC-N-5 ✅ + Phase 1.D decomposition design-lock ✅ + PC-N-6..PC-N-10 ✅ = **Phase 1.D complete ✅**
- Phase 1.E decomposition design-lock ✅ + PC-N-11..PC-N-13 ✅ + PC-N-14 design-lock ✅ + PC-N-15a design-lock + 実装 ✅ + PC-N-15b/c 統合 design-lock ✅ + PC-N-15b 実装 ✅ + bridge `cc027bb835` ✅ + bridge hotfix `95fdbdedac` ✅ + **PC-N-15c 実装 ✅ 本 commit** = **Phase 1.E complete ✅ 本 commit**
- = **Phase 1 全完了 ✅ 本 commit**
- → **Mac/Win 開発者補完 phase** ⏳ (entry 移行)
- → **Phase 1.F+** (実 PBR shader 接続 + real data 内容置換) ⏳

## §6. self-verify 9 観点

1. **Phase 1.E 全 sub-step (PC-N-11..PC-N-15c) 完了状態** §1 table 全 ✅ + bridge commits 2 件 (= newview link fix + hotfix 2 件) §1 table に補足記載 ✅
2. **達成事項 §2** 4 sub-section 網羅 (実 data 通電 + multi-asset/multi-skin + worker thread 並列化 + cleanup 一本化) ✅
3. **Phase 1 全完了状態 §3** = 全 sub-phase ✅ table + 設計原則 (1)/(2) 充足 + MUSEUBO-A 整合 + GATE-B 整合 ✅
4. **Mac/Win 補完 phase entry §4** = MoltenVK/Windows 環境差分予想 + 補完 phase 着手候補 (a)-(d) ✅
5. **残 strict 線形 §5** = Phase 1.A..1.E 全 ✅ + Phase 1 全完了 ✅ + 次 phase ⏳ entry marker ✅
6. **build verify literal** = PC-N-15c `[100%] Built target ayastorm-bin` + binary 3.05 GB + error 0 / warning 0 + AYA live verify「通常通りに描画されてます」record 2026-06-06 ✅
7. **GATE-B 整合** = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count `llvkloader.cpp` = 6 不変、Phase 1.A 確立時点同数) ✅
8. **MUSEUBO-A 整合** = AYA live verify 2026-06-06「通常通りに描画されてます」record で OpenGL 描画 100% 維持確認 ✅
9. **`feedback_proactive_handoff` 遵守** = 本 Phase 1.E complete marker handoff doc 起案 ((N15c-7) A 案 B 統合方針整合 = PC-N-15c 完了 handoff = Phase 1.E complete marker 兼用) ✅

## §A. AYA literal record + feedback 遵守 record

### §A.1 AYA literal record 時系列

- **2026-06-05** AYA literal「OK」record = PC-N-15c ambiguity 12 件 (N15c-1)..(N15c-12) 全件推奨案承認
- **2026-06-05** AYA literal「B でお願いします」record = PC-N-15b/c 統合 design-lock 案 B (= handoff doc 4 件 → 2 件削減) 採用
- **2026-06-05** AYA literal「案 2 newview link fix phase 着手」+「案 X-B LL_TESTS=OFF」+「a commit」+「root /tests/ のみ excluded」record = bridge commit `cc027bb835` 確定
- **2026-06-06** AYA literal「a」record = bindV3aRigged firstSet=4 hotfix 適用承認
- **2026-06-06** AYA literal「LL_INFOS を入れてってトレースするしかない」+「視野狭くなってるけど、落ちる可能性は１箇所なくて」record = canary 多数挿入で root cause 特定 approach 採用 → bindV3aRigged 内 Vulkan slot 番号誤解釈 fix
- **2026-06-06** AYA literal「OK ログインできました」record = bridge hotfix `95fdbdedac` live verify PASS
- **2026-06-06** AYA literal「了解ひきつづき予定通り作業を進めてください」record = cvar 撤去理由再確認後 PC-N-15c plan 通り続行確定
- **2026-06-06** AYA literal「通常通りに描画されてます」record = PC-N-15c live verify PASS

### §A.2 feedback 遵守 record

- **feedback_proactive_handoff** = 本 Phase 1.E complete marker handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** = 本 doc は phase 完了 marker ゆえ pre-req は PC-N-15b/c 統合 design-lock doc + cross-platform spec のみ、full file dump なし
- **feedback_self_verify_before_handoff** = §6 9 観点 self-verify 全 ✅
- **feedback_build_only_verified** = `[100%] Built target ayastorm-bin` literal + AYA live verify「通常通りに描画されてます」record 2026-06-06 で literal 検証取得
- **feedback_doubt_self_first** = bridge hotfix phase で仮説 2 連続外れ後、推論止めて canary 多数挿入で実 data 取得に切替 (= AYA literal「LL_INFOS 入れてって」record で誘導確認後実施)
- **feedback_admit_unknown** = bridge hotfix phase で apport core ignored で stack trace 不取得 → 「分からない」明示後 LL_INFOS_ONCE canary 50 件 across 7 functions 挿入で root cause 特定
- **feedback_confirm_referent_before_acting** = PC-N-15c 各 step 着手前に AYA literal 確認 record (= cvar 撤去理由質問時の停止 + 再説明 + literal「予定通り作業を進めてください」record 後続行)
- **feedback_no_scope_shrink** = PC-N-15c literal scope 6 項 全件実装、cvar 撤去理由の質問対応中も plan 不変
- **feedback_no_auto_commit** = AYA literal「commit して」明示指示 待ち (= 本 doc は commit 候補、AYA literal 受領後 commit)
- **feedback_no_claude_coauthor** = Co-Authored-By 行不在
- **feedback_release_branch_workflow** = feature branch `feature/ayastorm-r41-gl-removal` 上での実装
- **feedback_no_bare_reference_ids** = 各 PC-N-* ID + bridge commit hash に内容明記 + Phase 1.A..1.E table に scope 明記
- **feedback_tests_dir_never_commit** = root `/tests/` 改変なし (= bridge commit `cc027bb835` で .gitignore root-anchor 訂正済)
- **feedback_perf_map_bfs_drill** = bridge hotfix phase で recordAvatarPlaceholderDraw 単一関数のみ canary でなく callee 6 関数全周回 canary 投入で「層単位で全周回」原則適用
- **feedback_remove_verification_logs** = canary は bridge hotfix commit `95fdbdedac` 直前で全撤去、解析履歴は永続コメント形式で保持
- **feedback_ubo_migration_one_at_a_time** = Phase 1.E PC-N-11..PC-N-15c 全 sub-step を 1 つずつ実施、PC-N-15 → PC-N-15a/b/c 3 sub-phase 分解 + bridge commits 2 件分離 で大塊バッチ回避
- **memory project_ayastorm_r41_design_principles** = §3.2 (1)/(2) 充足確認
- **memory project_r41_phase1b_vulkan_host_gate** = §3.4 GATE-B 整合確認
- **memory project_ayastorm_three_platforms** = §4 Mac/Win 補完 phase entry 整理
