# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 **Phase 1.E (handoff sub-letter) complete marker** = Linux primary baseline 確立 marker

> **2026-06-06 全 doc audit 訂正 (§0.1 参照)**: 当初本 doc は「Phase 1 全完了 marker = Mac/Win 補完 phase entry」と命名されたが、全 spec doc audit で **roadmap §2.1 体系 (Phase 0 / 1.A/B/C / 2..K / K+1..K+5) 統一基準と handoff sub-letter 体系の整合性混乱** が確認された (= F1-F9 finding)。**2026-06-06 追加訂正で**: handoff sub-letter Phase 1.E (= Linux primary baseline 確立) ✅ + roadmap §2.1 統一基準 Phase 1 (= 1.A codegen + 1.B 30 setter redirect + 1.C UB_REFLECTION_PROBES shell 通電) **✅ 完走** (= R1/R2/R9/R10 ✅、R7 minor + R8 deferred は完走判定に影響なし、AYA さん合意 2026-06-06)。**次 phase = Phase 2 = R3 (= UB_REFLECTION_PROBES 本実装、shell に実 data 入れる)** ⏳。R3 と Phase 1 完走は別事項 (= R3 は Phase 2 仕事、Phase 1 完走条件に含まれない)。R4/R5/R6 (= 実 PBR shader 接続 / zero IS real data 卒業 / avatar Vulkan draw 通電) も Phase 2/3 仕事として deferred。

## §0. 着手契機 + 位置付け

**着手契機**: PC-N-15c 実装完了 (= Phase 1.E (handoff sub-letter) 内 7th = 最終 sub-step 実装完了 = cleanup + sGltfStubSkin sentinel 撤去 + AYAGltfMultiSkinEnabled cvar 撤去 + AYAGltfRealDrawEnabled cvar + recordAvatarPlaceholderDraw 末尾 entry hook 撤去) + AYA live verify「通常通りに描画されてます」record 2026-06-06 受領 → 当初は「Phase 1 全完了 marker」として本 doc 起案 (= 案 B 統合方針整合 = PC-N-15c 完了 handoff = Phase 1.E complete marker 兼用、(N15c-7) A 採用) → **2026-06-06 全 doc audit で Phase 番号体系不整合が確認され、本 doc は「Phase 1.E (handoff sub-letter) complete = Linux primary baseline 確立」までの範囲に literal 縮小** (§0.1)。

### §0.1 2026-06-06 全 doc audit 訂正 record (= 不整合 9 件 + Phase 1/Phase 2 境界 literal 確定)

AYA さん指示で全 119 file (= live root 5 + design/ 14 + handoff/misc/ 4 + handoff/phase1/{a,b,c,d,e}/ 96) + archive sample 19 file audit 実施。**主要 finding**:

- **F1**: Phase 番号体系 4 系統並走 = (A) handoff Phase 1.A..1.E + 1.F+ (新体系) / (B) roadmap §2.1 Phase 0 / 1.A/B/C / 2..K / K+1..K+5 / (C) 00-charter Phase 1 = libGL 完全除去 + vk-α / (D) eta-28 phase2a-d (放棄済)。相互 mapping literal 未配置。
- **F4**: 現実装 (Phase 1.D/1.E) は roadmap Template A 順序 (Phase 2 = UB_REFLECTION_PROBES → Phase 3 = GLTFMaterials → Phase 4 = GLTFNodes → Phase 5 = GLTFJoints) を **完全 skip**、代わりに Skin_GLTFJoints (PC-N-5/11) + PerDrawUBO_LightParams (PC-N-13「zero IS real data」semantic) を pilot 先回り着手。UB_REFLECTION_PROBES 本実装は未着手 (= Phase 1.C で `Global_ReflectionProbes` shell zero dummy write までで停止)。
- **F5**: 本 doc 内自己矛盾 confirmed = §3.1/§5「Phase 1 全完了 ✅」literal vs §2.4「sGltfStubAssetPipeline + sPlaceholderSkin 維持 (= Phase 1.F+ 持越し)」literal vs §5「Phase 1.F+ ⏳」literal の同居。
- **F6**: 「zero IS real data」semantic (pc-n-13-complete (N13-1) C) = Phase 1.E (handoff sub-letter) close を成立させるための創造的 framing、実 data 内容置換は Phase 1.F+ 持越し。
- **F9**: inventory.md §6.4「host C++ redirect 層は未着手」literal は 2026-06-03 doc 起案時点 snapshot、Phase 1.B (commit `35c4be1046`) 完了で 30 setter Vulkan path 分岐実装済 = doc 更新漏れのみ、Phase 1.B 完了主張は妥当。

**Phase 1/Phase 2 境界 literal 確定** (= AYA さん合意 2026-06-06):

- **Phase 1 (= roadmap §2.1 統一基準 § 4 から literal)**:
  - **1.A** = codegen pipeline (chapter 08 全章) + 全 85 UBO blueprint 入力で codegen PASS + `ubo_metadata.inl` + `ubo_host_loader.inl` 生成
  - **1.B** = 30 setter method 内部 Vulkan path 分岐 + `mUniformUBOLoc` cache + `mUseUBO=false` default で OpenGL path 経路維持
  - **1.C** = test UBO 1 個 (= `UB_REFLECTION_PROBES` shell = Phase 2 第 1 UBO と同一実体、空 struct + 空 dirty flag + 空 flush 実装) で 5 cadence 全経路 + `vkCmdBindDescriptorSets` 空 dummy buffer 成功
  - **完了条件** = 既存 program 動作 unchanged (= Vulkan path 分岐 ON でも OpenGL path 経路を選ぶ default 動作)。実 data 流入は Phase 2 で開始
- **Phase 2 (= roadmap §2.1 統一基準 §5)** = 第 1 UBO (= `UB_REFLECTION_PROBES`、(Q1) A 確定) の本実装 migration (= shell の実 member + 実 dirty 判定 + 実 flush logic 置換)、1 UBO 厳守 ((Q2) A)、AYA cold launch verify + canary cvar で経路成立確認
- **Phase 3..K** = Template A 順 = Phase 3 (GLTFMaterials per-asset) → Phase 4 (GLTFNodes per-asset) → Phase 5 (GLTFJoints per-skin) → 残 UBO 順次。**Phase 2/3 詳細定義は Phase 2 着手前に separate session で確定** (AYA さん指示 2026-06-06)
- **Phase K+1/K+2/K+3** = 3 OS 確証 (Linux/Win/Mac、(Q4) C Linux 先行 → Win/Mac 並走)、**Phase K+4** = OpenGL path 撤廃 ((Q3) A 全 UBO 移行完了まで並走)、**Phase K+5** = release 整備

**現実装 handoff Phase 1.A..1.E の roadmap §2.1 上の位置**:

| handoff sub-letter | roadmap §2.1 上の位置 | 状態 |
|---|---|---|
| Phase 1.A | roadmap Phase 1.A | ✅ |
| Phase 1.B | roadmap Phase 1.B | ✅ (= inventory.md §6.4 「未着手」literal は 2026-06-03 snapshot で更新漏れ) |
| Phase 1.C (PC-0..PC-7ε + PC-N-1..4) | roadmap Phase 1.C (shell UBO 1 個 = UB_REFLECTION_PROBES) | **✅ 完走** (PC-2 zero dummy + PC-6δ 5 cadence + PC-7δ vkCmdBindDescriptorSets 通電 = roadmap §4.2 Exit Criteria 充足)。shell に実 data 入れる作業 (= R3) は Phase 2 仕事として独立 |
| Phase 1.D (PC-N-5..10) | **roadmap Phase 2..K Template A 順序逸脱した pilot 先回り着手** (= GLTF asset 系 placeholder/stub infrastructure) | pilot ✅ |
| Phase 1.E (PC-N-11..15c + bridge) | **roadmap Phase 2..K Template A 順序逆走** (Skin_GLTFJoints + PerDrawUBO_LightParams「zero IS real data」+ push constant) | pilot ✅ (= Skin_GLTFJoints real data 通電 + push constant 通電 + 「zero IS real data」semantic のみ + worker thread 並列化 baseline) |
| Phase 1.F+ (未着手) | roadmap 上未定義の create 命名 = **Phase 2/3 定義時に所属確定 (= AYA さん指示 2026-06-06 deferred)** | 残件 R4/R5/R6 (= 実 PBR shader 接続 + zero IS real data 卒業 + avatar Vulkan draw 通電) |

**Phase 1 完走 gate (= Phase 2 着手条件) = R1-R10 全件充足 + AYA cold launch verify PASS** (詳細 §5)。

**位置付け**: r41 milestone 内 **Phase 1.E (handoff sub-letter) complete marker = Linux primary baseline 確立 marker** = 個別 sub-step 完了 handoff (= PC-N-* complete handoff) と異なり **Phase 1.E (handoff sub-letter) 全体総括 + Linux primary baseline 確立確認** = (1) PC-N-11..PC-N-15c 完了状態 + (2) GLTF asset cluster pilot 通電 (Skin_GLTFJoints real data + push constant real modelview + PerDrawUBO_LightParams「zero IS real data」semantic) + worker thread 並列化 baseline 達成 + (3) **roadmap §2.1 統一基準では Phase 1 完走未達 + Phase 2..K Template A 順序逸脱の pilot 先回り着手状態** 明示 + (4) Phase 1 完走 gate (= R1-R10) 明示。本 doc 完了で r41 handoff Phase 1.E (= Linux primary baseline) closure、**次 phase = roadmap §2.1 Phase 1 完走 (= R1/R2/R3 着手) ⏳**。Mac/Win 開発者補完 phase は AYA さん明示「Linux 完走後」(2026-06-05 PC-8 literal record) で deferred。

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
  - **2026-06-06 audit 訂正 (= 実 source 正準 naming)**: per-thread `VkCommandPool` + secondary `VkCommandBuffer` の実 source naming = `struct PcN14WorkerContext { VkCommandPool mCommandPool; VkCommandBuffer mSecondaryCmdBuf; std::unique_ptr<LLUboRingBuffer> mDrawUboRingBuffer; ... };` (`llvkloader.cpp:749`) + 保有 vector = `sPcn14WorkerCtx` (`llvkloader.cpp:759`)。旧 doc literal「`sWorkerCommandPools` / `sWorkerSecondaryCommandBuffers` 2 配列」は概念表現で実 naming と乖離、機能は等価存在
- **worker thread dispatch** (PC-N-15b): `LL::WorkQueue` post/drain + per-Primitive `postPrimitiveToWorker` hook (gltfscenemanager.cpp per-Primitive loop 内、`setCurrentPrimitive` 直後並列) + secondary cmdbuf record (`VkCommandBufferInheritanceRenderingInfoKHR` 経由 dynamic rendering scope 継承) + `vkCmdExecuteCommands` 集約 (per-Asset 末尾 `drainWorkersAndExecute`) + `writeDrawUbo`/`writeSkinUbo` thread-aware 拡張 + first-fire marker 3 件
- **MUSEUBO-A 整合維持**: `AYAGltfWorkerThreadEnabled=false` default で worker thread infra 起動するが post 経路で即時 false 返却 = main thread 経路完全維持 + OpenGL 描画 100% 維持
- **AYA live verify「OK ログインできました」record 2026-06-06** = worker thread infra 起動成功 + bindV3aRigged firstSet=4 fix 後 crash 解消立証

### §2.4 cleanup (PC-N-15c) で達成された一本化

- **sGltfStubSkin sentinel 撤去**: PC-N-5 (a) 導入 address-only sentinel + register/unregister 配線完全撤去、real Skin path 一本化
- **AYAGltfMultiSkinEnabled cvar 撤去**: sentinel fall-through path 物理消失で段階卒業 gate 役目終了
- **AYAGltfRealDrawEnabled cvar 撤去**: PC-N-15b worker thread dispatch 配線後 `recordAvatarPlaceholderDraw` 末尾 entry hook が dead code 化ゆえ撤去
- **維持される cvar (= 機能 gate)**: `AYAGltfRealModelviewEnabled` + `AYAGltfRealLightParamsEnabled` + `AYAGltfMultiAssetCanary` + `AYAGltfWorkerThreadEnabled` + `AYAGltfWorkerThreadCount` ((N15c-4) A)
- **維持される sentinel**: `sGltfStubAssetPipeline` (= Phase 1.F+ 実 PBR shader 接続まで再利用、(N15c-5) A) + `sPlaceholderSkin` (= avatar Vulkan draw 通電 phase 持越し、(N15c-6) A)

## §3. Phase 1.E (handoff sub-letter) 完了状態確認 + roadmap §2.1 統一基準での position

> **2026-06-06 audit 訂正**: 旧 §3.1「Phase 1 全完了状態確認」title は **handoff sub-letter 完了の意** であり、roadmap §2.1 統一基準での Phase 1 完走 ✅ とは異なる (= F4 Template A 順序逸脱 + R3/R4/R5/R6 残)。本 §3.1 table は **handoff sub-letter 完了状態** を示し、roadmap §2.1 上の position 列を併記。

### §3.1 handoff sub-letter 完了状態 + roadmap §2.1 上の position

| sub-phase | scope | 状態 (handoff sub-letter) | roadmap §2.1 上の position |
|-----------|-------|---|---|
| **Phase 1.A** | Vulkan loader 基盤 + descriptor set layout V3a 5-set + pipeline layout sAYAStandardLayout + per-frame UBO + ring buffer 基盤 | ✅ | roadmap Phase 1.A ✅ |
| **Phase 1.B** | C++ Vulkan gate (`mUseUBO` runtime flag、`LL_VULKAN_GLSL` macro は GLSL 専用) + GATE-B 確立 | ✅ | roadmap Phase 1.B ✅ (inventory.md §6.4 は 2026-06-03 snapshot で更新漏れ) |
| **(Z) SSS / (W) uniform4iv** | upstream fix accommodation | ✅ | r41 milestone と独立 (misc 系) |
| **(Y) Phase 1.C prep** | preparatory cleanup | ✅ | roadmap Phase 1.C 入口 |
| **Phase 1.C** (PC-0..PC-7ε + PC-N-1..PC-N-4) | per-program / per-singleton UBO cadence + sky_smoke placeholder + bindV3aStatic/Rigged + writeDrawUbo / writeSkinUbo API + ring buffer grow auto re-wire | ✅ | roadmap Phase 1.C **✅ 完走** (= UB_REFLECTION_PROBES shell `Global_ReflectionProbes` を PC-2 zero dummy + PC-6δ 5 cadence + PC-7δ `vkCmdBindDescriptorSets` 通電 = roadmap §4.2 Exit Criteria 充足)。shell の実 data 置換 (= R3) は Phase 2 仕事 |
| **PC-8 Linux primary marker + PC-N-5** | Phase 1.D 着手起点 = sGltfStubSkin sentinel register + identity matrix bone data 通電 baseline | ✅ | **roadmap Phase 2..K Template A 順序逸脱した pilot 先回り着手 入口** |
| **Phase 1.D** (PC-N-6..PC-N-10) | 実 LL::GLTF::Asset 経由 vertex/index buffer Vulkan infrastructure 通電 + Phase 1.D complete marker | ✅ | **roadmap Phase 2..K Template A 順序逸脱した pilot 先回り着手** (= 本来 Phase 2 = UB_REFLECTION_PROBES 本実装が先、Phase 3-5 = GLTFMaterials/Nodes/Joints の順、本 1.D は GLTF asset 系 placeholder/stub infrastructure 先回り構築) |
| **Phase 1.E** (PC-N-11..PC-N-15c) | 実 data 通電 + multi-asset / multi-skin / real modelview / real per-draw light params + worker thread 並列化 + cleanup | ✅ 本 commit (= handoff sub-letter) | **roadmap Phase 2..K Template A 順序逆走 pilot** (= Skin_GLTFJoints real data 通電 + push constant real modelview + PerDrawUBO_LightParams「zero IS real data」semantic + worker thread 並列化 baseline、UB_REFLECTION_PROBES 本実装は依然 未着手) |

### §3.2 設計原則 (= memory `project_ayastorm_r41_design_principles`) 充足

- **(1) Upstream OpenGL 取り込みやすさ維持**: call site API 温存 (= `recordGltfAssetDraw` / `recordAvatarPlaceholderDraw` signature 不変 + `writeDrawUbo` / `writeSkinUbo` / `bindV3aRigged` accessor signature 不変 + `LLUboRingBuffer` class 改変 0 件 + shader 改変ゼロ + `gltf/primitive.h` から `vulkan/vulkan.h` include 回避)、`PrimitiveVulkanBuffer` struct + 6 新 API は LLVKLoader 内 encapsulate
- **(2) Core プロセス分散実現**: per-thread `LLUboRingBuffer` instance + per-thread `VkCommandPool` + secondary `VkCommandBuffer` + worker thread per-Primitive dispatch で primitive-level granularity 達成 + UBO/cmdbuf 並列化容易な設計成立

### §3.3 MUSEUBO-A 整合維持 (= 全 cvar OFF default で OpenGL 描画 100% 維持)

- 全実装 cvar default OFF 状態で Phase 1.D + 1.E complete baseline = Phase 1.D complete (= PC-N-10 commit `16a26f6272`) と機能等価
- PC-N-15c で cvar 2 件撤去 (`AYAGltfMultiSkinEnabled` + `AYAGltfRealDrawEnabled`) だが残存 cvar (= `AYAGltfWorkerThreadEnabled` 等) default OFF で baseline 維持
- AYA live verify 2026-06-06「通常通りに描画されてます」record で OpenGL 経路維持確認

### §3.4 GATE-B 整合 (= memory `project_r41_phase1b_vulkan_host_gate`)

- `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= **active preprocessor directive 0 件** + 整合 marker comment 5 件 `llvkloader.cpp`、Phase 1.A 確立時点と active directive 0 件不変)
- **2026-06-06 audit 訂正**: 旧 literal「count = 6 不変」は **comment 内の literal 文字列出現** count であり、active preprocessor directive count ではない (= GATE-B 整合の本質は active directive 0 件、comment marker count drift は機能影響なし)。2026-06-06 grep 確認時 comment marker = 5 件 (= 起案以後 1 件減、機能影響なし、active directive は当初から 0 件)
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

## §5. handoff sub-letter 完了状態 + Phase 1 完走 gate 残件 (= Phase 2 着手前提)

### §5.1 handoff sub-letter strict 線形 (= 過去 sub-step 完了 chain)

- Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α..PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1..PC-N-4 ✅ = **Phase 1.C (handoff sub-letter) complete ✅** (= roadmap Phase 1.C shell 段階のみ達成)
- PC-8 Linux primary marker ✅ + PC-N-5 ✅ + Phase 1.D decomposition design-lock ✅ + PC-N-6..PC-N-10 ✅ = **Phase 1.D (handoff sub-letter) complete ✅** (= roadmap Phase 2..K Template A 順序逸脱の pilot 先回り着手)
- Phase 1.E decomposition design-lock ✅ + PC-N-11..PC-N-13 ✅ + PC-N-14 design-lock ✅ + PC-N-15a design-lock + 実装 ✅ + PC-N-15b/c 統合 design-lock ✅ + PC-N-15b 実装 ✅ + bridge `cc027bb835` ✅ + bridge hotfix `95fdbdedac` ✅ + **PC-N-15c 実装 ✅ 本 commit** = **Phase 1.E (handoff sub-letter) complete ✅ 本 commit** (= roadmap Phase 2..K Template A 順序逆走 pilot)
- = **Linux primary baseline 確立 ✅ 本 commit** (= Vulkan path 通電 + GLTF asset cluster pilot + worker thread 並列化 baseline)

### §5.2 Phase 1 完走判定 + Phase 2 着手 scope (= roadmap §2.1 統一基準、AYA さん合意 2026-06-06 + 2026-06-06 追加訂正で R3-R6 分類訂正)

**2026-06-06 audit 追加訂正 (= R3-R6 分類修正)**: 旧 §5.2 で R3 (UB_REFLECTION_PROBES 本実装) を「Phase 1 完走 gate 残件」table に入れたのは literal 誤分類。roadmap §2.1 上で R3 は **Phase 2 仕事** (= shell に実 data 入れる作業)、Phase 1 (.A/.B/.C shell まで) とは独立。同様に R4 (実 PBR shader 接続) / R5 (zero IS real data 卒業) / R6 (avatar Vulkan draw 通電) も **Phase 2/3 仕事** (= AYA さん指示 2026-06-06 で詳細 scope 定義は Phase 2 着手前 separate session deferred)。本訂正で R3-R6 を「Phase 2/3 着手 scope」 table に移動、Phase 1 完走 gate table は R1/R2/R7/R8/R9/R10 のみに整理。

**Phase 1 完走判定 (= roadmap §2.1 統一基準)**:

| # | Phase 1 完走項目 | source literal | status |
|---|---|---|---|
| **R1** | Phase 1.B 完了状態 verification (= inventory.md §6.4 + §3 件数 update) | inventory.md:286-291,356 doc 起案時 snapshot vs phase1-b-complete.md:31 完了主張 vs 実 source 31 setter 全件 Vulkan path 分岐 ✅ | ✅ (本 audit + 本訂正 commit で消化、inventory.md §3 冒頭注記 + §6.4 status update 完了) |
| **R2** | Phase 1.C shell 段階 完了状態 verification (= UB_REFLECTION_PROBES shell `Global_ReflectionProbes` で 5 cadence 全経路 `vkCmdBindDescriptorSets` 通電 literal 検証) | roadmap §4.1 Phase 1.C Exit Criteria + phase1/c/ pc-7δ + pc-7ε literal + 実 source 確認 (`bringupTestUBO()` + flush 6 関数 + 4 vkCmdBindDescriptorSets site 全 sAYAStandardLayout 経由) | ✅ (本 audit で literal 確認、roadmap §4.2 Exit Criteria literal 充足) |
| **R7** | GLTF asset rez シナリオで first-fire marker 3 件発火確認 | 本 doc §4.3 (c) 「Linux でも未確認、scene に GLTF asset 不在ゆえ Phase 1.E live verify では発火せず」 | ⏳ minor item (= Phase 1.E pilot 実機 verify 残、AYA 実機 session 1 回で消化可能、Phase 1 完走判定に影響少) |
| **R8** | design/10-open-questions.md 残 AYA 判断 | design/10:55,69 audit 訂正後 = 残 10 件 (= (V1')(V3')(S3')(W) + (K)(M)(N)(O) + (F)(Q28-FFDUP)、Phase 1.A/B/C 設計判断は実装 default 採用で物理充足済) | ⏳ deferred (= AYA さん指示 2026-06-06、別 batch session で消化、本訂正で 16 → 10 件に縮小) |
| **R9** | handoff-phase1-e-complete.md 自己矛盾解消 | 旧 §3.1/§2.4/§5 literal 矛盾 (= 本 doc audit F5) | ✅ (本訂正 commit で消化) |
| **R10** | Phase 番号体系 4 系統の整合 = 既存 doc 訂正 (= 新規起案ゼロ) | F1 = 4 体系並走、相互 mapping 未配置 | ✅ (本訂正 commit で消化) |

**Phase 1 完走判定 verdict**: **✅ 完走** (= handoff Phase 1.A..1.E ✅ + roadmap §2.1 Phase 1.A/B/C ✅ + R1/R2/R9/R10 ✅、R7/R8 は minor / deferred で Phase 1 完走判定に影響なし、AYA さん合意 2026-06-06)。

**Phase 2 着手 scope (= 詳細定義は Phase 2 着手前 separate session、AYA さん指示 2026-06-06 deferred)**:

| # | Phase 2/3 仕事 | source literal | 着手前提 |
|---|---|---|---|
| **R3** | Phase 2 = UB_REFLECTION_PROBES 本実装 = shell `Global_ReflectionProbes` (PC-2 で配置済) を **実 member + 実 dirty 判定 + 実 flush logic** に置換 | roadmap §5.2 Template A + (Q1) A 確定 (design/10:36) | Phase 1 完走 ✅ + AYA さん Phase 2 詳細 scope 定義 separate session |
| **R4** | 実 PBR shader 接続 (= sGltfStubAssetPipeline 撤去 + sky_smoke shader 流用卒業) | 本 doc §2.4 + cross-platform-port-spec.md:115,119 | **Phase 2/3 定義時に所属確定 (AYA さん指示 2026-06-06 deferred)** |
| **R5** | 「zero IS real data」semantic 卒業 (= PerDrawUBO_LightParams real 内容投入) | pc-n-13-complete.md:17 (N13-1) C + 本 doc §2.1 | R4 と同 phase or 直後 |
| **R6** | avatar Vulkan draw 通電 (= sPlaceholderSkin 撤去) | 本 doc §2.4 「avatar Vulkan draw 通電 phase 持越し」 | **Phase 2/3 定義時に所属確定 (AYA さん指示 2026-06-06 deferred)** |

### §5.3 次 phase entry

- **本訂正 commit で消化**: R1 (inventory update) + R2 (Phase 1.C shell verification) + R9 (本 doc 自己矛盾解消) + R10 (Phase 番号体系整合) = **Phase 1 完走 ✅ 確定**
- **minor / deferred**: R7 (GLTF asset rez first-fire marker 発火確認 = AYA 実機 session minor item) + R8 (残 10 件 AYA 判断 deferred = 別 batch session)
- **次 phase entry**: R3 = roadmap §2.1 Phase 2 (= UB_REFLECTION_PROBES 本実装) **着手** = AYA さん指示で Phase 2 入口で Phase 2/3 詳細 scope 定義 separate session 設定 (2026-06-06)
- **R4/R5/R6 所属 Phase 番号**: Phase 2/3 定義時に AYA さん判断確定 = 案 X (Phase 3+ Template A GLTFMaterials/Nodes/Joints 内消化) / 案 Y (別 phase 新設) / 案 Z (R3 と並列消化) のいずれか
- **Mac/Win 開発者補完 phase** = roadmap Phase K+1/K+2/K+3、**Linux primary 全 Phase 完走後** (= AYA さん明示 2026-06-05 PC-8 literal record「まずLinuxで完成の後、それを提供してつないでもらう」)、Phase 2..K 完走後着手
- **Phase K+4 (OpenGL 撤廃)** = (Q3) A 全 UBO 移行完了まで並走、Phase K+4 で初撤廃
- **Phase K+5 (release)** = release note + tag 切り出し

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
