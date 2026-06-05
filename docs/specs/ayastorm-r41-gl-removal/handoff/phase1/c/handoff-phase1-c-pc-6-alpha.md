# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-6α complete** marker

**作成日**: 2026-06-04
**前 commit chain** (= 直前 handoff 起案):
- `9ed4cca802` = Phase 1.C **PC-1 complete** = `Global_ReflectionProbes` shell blueprint codegen emit
- `c7f512d654` = Phase 1.C **PC-2 complete** = test UBO shell C++ 接続 = block-level test bring-up (= (c) 採用)
- `c31998c49f` = Phase 1.C **PC-3 complete** = `sAssetUboPool` grow algorithm + TUT 10/10 PASS (= (α) 採用)
- `912863bf81` = Phase 1.C **PC-4 complete** = `LLUboRingBuffer` ring buffer 4 MB / 16 MB grow algorithm + cvar `AYARingBufferSizeMB` 露出 + TUT 11/11 PASS (= (α') 採用)
- `2fb5af486e` = Phase 1.C **PC-5 complete** = `LLPipelineCacheStorage` PSO cache disk persist + 64 MB cap algorithm + cvar `AYAPipelineCacheSizeMB` 露出 + TUT 13/13 PASS (= (α'') + (e1) 採用)

**本 handoff doc 目的**: **Phase 1.C PC-6α complete marker**。**PC-6 全体 (= 5 cadence update site 統合 wire up) を α..ζ 6 sub-task に strict 線形分割した上で PC-6α = (W2) `sAssetUboPool` 実 `VkDescriptorPool` factory injection wire up 完結後の引継**。`indra/llrender/llvkloader.cpp` 単一 file 4 編集で `LLAssetUboPool` algorithm 層に sDevice / vkCreateDescriptorPool を closure capture した DI callback を提供 + 起動時 1 物理 pool (= 192 set / 576 UBO / 9408 SAMPLER) prealloc + shutdown 経路 reverse 順 destroy 配線。llrender build PASS + `INTEGRATION_TEST_llassetubopool` 10/10 PASS (= PC-3 TUT regression なし) + codegen unittest 130/130 PASS (= Phase 1.A / 1.B / 1.C PC-1..PC-5 regression なし)。

---

## §0 state 一行 summary

PC-6α = **`LLAssetUboPool` × Vulkan device 実 wire up complete**:

- `indra/llrender/llvkloader.cpp` 4 編集 = (a) include `llassetubopool.h` + `<memory>` (b) namespace globals = `ASSET_POOL_UBO_BINDINGS_PER_ASSET=3` + `ASSET_POOL_SAMPLER_BINDINGS_PER_ASSET=49` constexpr + `std::unique_ptr<LLAssetUboPool> sAssetUboPoolMgr` (c) `createAssetUboPool()` helper 起案 (= factory + destroyer closure + `std::make_unique` + `initialize()`) (d) init chain 追加 (= `createSharedDescriptorPool` 直後) + shutdown 追加 (= `sSharedDescriptorPool` 破棄前)
- factory lambda = `vkCreateDescriptorPool` 呼出 = maxSets=192 (= 64 asset × 3 frame) + UBO descriptorCount=576 (= 3 binding × 64 × 3 frame) + SAMPLER descriptorCount=9408 (= 49 binding × 64 × 3 frame) + `flags=0` (= design 07 §6.4 `FREE_DESCRIPTOR_SET_BIT` 不要)
- destroyer lambda = `vkDestroyDescriptorPool`
- 新 file 0 件 / CMake 改変 0 件 (= llrender → llcommon link 既存)
- llrender build PASS + INTEGRATION_TEST_llassetubopool 10/10 PASS + codegen unittest 130/130 PASS

---

## §1 pre-requisite 最小読み (= 次 session 着手時参照、`feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session = PC-6β 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc | 全文 | PC-6α 完結状態 + PC-6β 着手起点 + PC-6 α..ζ 分割理由 record + (W2) factory injection 配線 pattern |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-5.md` | §1.1 必読 3 件 + §3.4 settings.xml cvar block + §10 次 session 着手 1 line | PC-5 で起案済 `LLPipelineCacheStorage` API + cvar `AYAPipelineCacheSizeMB` 露出状態 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md` | §7 (= ring buffer + dynamic offset + chunk 構造) + §6.4 (= 4 pool split 採用根拠) | PC-6β scope = `LLUboRingBuffer` 実 `vmaCreateBuffer` HOST_VISIBLE+MAPPED 配線 + cvar `AYARingBufferSizeMB` 読込 hookup |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `indra/llrender/llvkloader.cpp:15-26` | PC-6α 追加 include (= `llassetubopool.h` + `<memory>`) |
| `indra/llrender/llvkloader.cpp:358-373` | PC-6α 追加 file-static (= constexpr 2 + `sAssetUboPoolMgr` unique_ptr) |
| `indra/llrender/llvkloader.cpp` (= `createSharedDescriptorPool` 直後) | PC-6α `createAssetUboPool()` 全文 = factory + destroyer + ctor + initialize、PC-6β `createDrawUboRingBuffer()` (仮称) の pattern reference |
| `indra/llrender/llvkloader.cpp` (= init chain `createSharedDescriptorPool` 直後 + shutdown `sSharedDescriptorPool` 破棄前) | PC-6α 配置位置 = PC-6β / PC-6γ 配置位置の precedent |
| `indra/llcommon/lluboringbuffer.h` / `.cpp` | PC-6β wire up 対象 = `BufferHandle` / `BufferAllocator` / `BufferDestroyer` type alias + `kInitialSizeMB=4` / `kMaxSizeMB=16` / `kFramesInFlight=3` / `kDefaultAlignment=256` constexpr + ctor / initialize / shutdown / `AllocateResult` / allocate / beginFrame |
| `indra/llcommon/llpipelinecachestorage.h` / `.cpp` | PC-6γ wire up 対象 = `CacheBlob` / `FileReader` / `FileWriter` type alias + `kDefaultMaxSizeMB=64` constexpr + ctor / initialize / shutdown / updateBlob / persistToDisk |
| `indra/newview/app_settings/settings.xml` (= `AYARingBufferSizeMB` block + `AYAPipelineCacheSizeMB` block) | cvar 露出状態確認、PC-6β / PC-6γ で読込 hookup 対象 |
| `indra/llrender/llvkloader.cpp:756 createPipelineCache()` | PC-6γ wire up 対象 = `VkPipelineCacheCreateInfo{}` `pInitialData = nullptr` を blob 投入経路に置換 + shutdown 時 `vkGetPipelineCacheData` 取出 + `LLPipelineCacheStorage::updateBlob` + `persistToDisk` 配線 |
| `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md` | PC-6δ scope = 5 cadence flush 関数 5 種の update site 配置 |
| `indra/llrender/llglslshader.cpp:2032 bringupTestUBO()` | PC-6ε scope = block-level bring-up を SINGLETON cadence flush 関数経由の本格置換 |
| `indra/llrender/llglslshader.cpp:2480-2563, 3006-3079` | PC-6ζ scope = setter SAMPLER skip (`cadence_tag == 5 /* CADENCE_SAMPLER */ return`) ↔ codegen SINGLETON cadence_tag=5 衝突 正攻法対応 |

---

## §2 PC-6 着手前 scope 整理 = α..ζ 6 sub-task 分割

### §2.1 PC-6 scope ambiguity 発見

PC-5 handoff §10 が示す PC-6 scope は 6 sub-task 包含:

| sub | 内容 |
|---|---|
| α | (W2) `sAssetUboPool` 実 `VkDescriptorPool` factory injection wire up |
| β | (RB) `LLUboRingBuffer` 実 `vmaCreateBuffer` HOST_VISIBLE+MAPPED factory injection wire up + cvar `AYARingBufferSizeMB` 読込 hookup |
| γ | (PSC) `LLPipelineCacheStorage` 実 file I/O (= `std::fstream` reader/writer) + `vkGetPipelineCacheData` 取出 + `VkPipelineCacheCreateInfo.pInitialData` 投入 wire up + cvar `AYAPipelineCacheSizeMB` 読込 hookup |
| δ | 5 cadence (per-frame/per-pass/per-asset/per-draw/per-skin) flush 関数 update site (= 06b §4 該当 5 種) で test UBO 空 dummy 書込 PASS |
| ε | block-level test bring-up を SINGLETON cadence flush 関数経由の本格置換 (= PC-2 (c) 採用後の正攻法 graduation) |
| ζ | SAMPLER skip 正攻法対応 (= PC-2 §2.5 残課題 = `cadence_tag == 5 /* CADENCE_SAMPLER */` force skip ↔ codegen SINGLETON cadence_tag=5 衝突解消) |

これは `feedback_ubo_migration_one_at_a_time` 「1 PC = 1 algorithm 層」precedent (PC-3 (α) / PC-4 (α') / PC-5 (α''))を大きく逸脱。

### §2.2 Claude 推奨 = PC-6 strict 線形分割 + AYA 確定

「Claude 推奨 = PC-6 を strict 線形 PC-6α..ζ に分割」を AYA literal「Claude 推奨」(= 2026-06-04 session) で受領 → **PC-6 = α..ζ 6 sub-task 分割採用**。

### §2.3 分割採用根拠 3 件

1. **PC-3 (α) / PC-4 (α') / PC-5 (α'') precedent 整合** = 同形 = 1 PC = 1 algorithm 層 wire up + 1 build/test verify cycle + diff review 容易 + Phase 1.C 全 PC 同じ pattern で scope 一貫性確保
2. **`feedback_ubo_migration_one_at_a_time` 整合** = 6 sub-task 同時着手は cold launch 検証単位が交錯し regression 切分け困難、1 sub = 1 build verify 単位で hazard 局所化
3. **`feedback_no_scope_shrink` 整合** = 分割は scope 縮小ではない (= 6 sub-task 全て後続実施)、literal scope 全部完遂

---

## §3 PC-6α 実施内容

### §3.1 `indra/llrender/llvkloader.cpp` 編集 1 = include 追加

```cpp
#include "linden_common.h"
#include "llvkloader.h"

#include "volk.h"
#include "lldir.h"
#include "llassetubopool.h"   // PC-6α 追加

#include <vector>
#include <string>
#include <climits>
#include <cstring>
#include <fstream>
#include <memory>             // PC-6α 追加
```

llrender → llcommon link は既存 (`indra/llrender/CMakeLists.txt:126-127`)、include path 解決済。

### §3.2 `indra/llrender/llvkloader.cpp` 編集 2 = file-static 宣言追加

`sSharedDescriptorPool` 宣言直前に挿入:

```cpp
// PC-6α (W2): per-asset (set=3) descriptor pool grow 機構実 wire up。
// design/07-vulkan-api-state.md §6.1 / §6.3 / §6.4:
//   per-pool sizing (= 1 物理 pool = 64 asset × FRAMES_IN_FLIGHT=3 frame):
//     maxSets               = 64 × 3 = 192
//     UBO  descriptorCount  = 3 binding × 64 × 3 = 576
//     SAMPLER descriptorCount = 49 binding × 64 × 3 = 9408
//   FREE_DESCRIPTOR_SET_BIT は付けない (= grow only)。
constexpr U32 ASSET_POOL_UBO_BINDINGS_PER_ASSET     = 3;
constexpr U32 ASSET_POOL_SAMPLER_BINDINGS_PER_ASSET = 49;
std::unique_ptr<LLAssetUboPool> sAssetUboPoolMgr;
```

### §3.3 `indra/llrender/llvkloader.cpp` 編集 3 = `createAssetUboPool()` 起案

`createSharedDescriptorPool()` 直後に配置 (= 設計 review 順、sSharedDescriptorPool ← sAssetUboPool ← sFrameUboPool/sProgramUboPool/sDrawUboPool の 4 cadence pool split pattern 起点)。

| component | 内容 |
|---|---|
| factory lambda | `[frames]() -> PoolHandle` (= `FRAMES_IN_FLIGHT` を closure capture)、`sDevice == VK_NULL_HANDLE` 早期 return 0、`VkDescriptorPoolSize[2]` (UBO + SAMPLER) + `VkDescriptorPoolCreateInfo{maxSets=192, poolSizeCount=2, flags=0}` + `vkCreateDescriptorPool(sDevice, ...)` → 成功時 `reinterpret_cast<PoolHandle>(pool)` return、失敗時 LL_WARNS + 0 return |
| destroyer lambda | `[](PoolHandle h) -> void`、`h == 0 \|\| sDevice == VK_NULL_HANDLE` 早期 return、`vkDestroyDescriptorPool(sDevice, reinterpret_cast<VkDescriptorPool>(h), nullptr)` |
| ctor + initialize | `std::make_unique<LLAssetUboPool>(factory, destroyer, prealloc=64, grow=64)` + `initialize()` (= 内部で 1 回 factory 呼出 = 1 物理 pool prealloc)、失敗時 reset + return false |
| LL_INFOS marker | `"Asset UBO pool wired up (PC-6α W2, prealloc=64 asset × 3 frame = 192 set / pool, UBO=576, SAMPLER=9408, grow chunk=64 asset, pool count=1)"` |

### §3.4 `indra/llrender/llvkloader.cpp` 編集 4 = init chain + shutdown

**init chain** (= `createSharedDescriptorPool` 直後、`createPlaceholderWhiteImage` 直前):

```cpp
if (!createVmaAllocator() || !createSharedDescriptorPool())
{
    shutdownVulkan();
    return false;
}

// PC-6α (W2): per-asset descriptor pool grow 機構を sSharedDescriptorPool 直後に立ち上げる
if (!createAssetUboPool())
{
    shutdownVulkan();
    return false;
}
```

**shutdown** (= `sSharedDescriptorPool` 破棄前、sDevice 生存中):

```cpp
// PC-6α (W2): LLAssetUboPool::shutdown() が全 grow pool を destroyer 経由で逆順 destroy
if (sAssetUboPoolMgr)
{
    sAssetUboPoolMgr->shutdown();
    sAssetUboPoolMgr.reset();
}

if (sSharedDescriptorPool != VK_NULL_HANDLE)
{
    vkDestroyDescriptorPool(sDevice, sSharedDescriptorPool, nullptr);
    sSharedDescriptorPool = VK_NULL_HANDLE;
}
```

### §3.5 build verify

1. **`make -j4 llrender`** 実走:
   - `llvkloader.cpp.o` compile PASS
   - `libllrender.a` link PASS
   - ERROR 0 件 / WARNING 0 件 (= PC-6α 改変関連)
2. **`make -j4 INTEGRATION_TEST_llassetubopool`** 実走:
   - llcommon 静 lib build PASS
   - `INTEGRATION_TEST_llassetubopool` POST_BUILD auto-run:
     ```
     Unit test group_started name=LLAssetUboPool
     Unit test group_completed name=LLAssetUboPool
         Total Tests:	10
         Passed Tests:	10	YAY!! \o/
     ```
   = PC-3 algorithm 層 regression なし

### §3.6 codegen unittest = 130/130 PASS

`python3 -m unittest discover -s scripts/ubo_codegen/tests` = `Ran 130 tests in 0.066s` `OK` (= Phase 1.A / 1.B / 1.C PC-1..PC-5 regression なし)。

---

## §4 PC-6α Exit Criteria 充足 record

PC-6α scope literal = 「(W2) `sAssetUboPool` 実 `VkDescriptorPool` factory injection wire up」。

| Exit 項目 | 充足 |
|---|---|
| (i) `LLAssetUboPool` algorithm 層に実 Vulkan factory 配線 | ✅ closure capture lambda で `vkCreateDescriptorPool` / `vkDestroyDescriptorPool` 提供、algorithm 層は handle 種別非依存維持 |
| (ii) 起動時 1 物理 pool prealloc | ✅ `initialize()` 内 1 回 factory 呼出、`getPoolCount() == 1` (LL_INFOS marker で観測可能) |
| (iii) shutdown 経路 reverse 順 destroy | ✅ `sAssetUboPoolMgr->shutdown()` で `LLAssetUboPool::shutdown()` が全 pool を逆順 destroy |
| (iv) PC-3 algorithm 層 regression なし | ✅ TUT 10/10 PASS (= `INTEGRATION_TEST_llassetubopool`) |
| (v) llrender build PASS + warning 0 | ✅ `make -j4 llrender` ERROR 0 / WARNING 0 |
| (vi) codegen unittest regression なし | ✅ 130/130 PASS |

---

## §5 残 strict 線形 (= §2.2 採用後)

```
PC-6α ✅ (本 commit) → PC-6β (RB LLUboRingBuffer 実 vmaCreateBuffer HOST_VISIBLE+MAPPED wire up + cvar AYARingBufferSizeMB 読込 hookup)
                  → PC-6γ (PSC LLPipelineCacheStorage 実 file I/O + vkGetPipelineCacheData wire up + cvar AYAPipelineCacheSizeMB 読込 hookup)
                  → PC-6δ (5 cadence per-frame/per-pass/per-asset/per-draw/per-skin flush 関数 update site 5 種で test UBO 空 dummy 書込 PASS)
                  → PC-6ε (block-level test bring-up を SINGLETON cadence flush 関数経由の本格置換)
                  → PC-6ζ (setter SAMPLER skip 正攻法対応 = cadence_tag == 5 force skip ↔ codegen SINGLETON cadence_tag=5 衝突解消)
                  → PC-7 (vkCmdBindDescriptorSets 通電)
                  → PC-8 (build verify)
                  → PC-N (Phase 1.C complete marker)
```

---

## §6 r41 milestone state

- Phase 1.A ✅ (= codegen 起点)
- Phase 1.B ✅ (= 30 setter Vulkan path 分岐 + `mUseUBO` runtime gate)
- (Z) SSS ✅
- (W) uniform4iv ✅ (= (a) fix)
- (Y) Phase 1.C prep ✅
- PC-0 ✅ (= AYA 確定値 4 件 record)
- PC-1 ✅ (= `Global_ReflectionProbes` shell blueprint codegen emit + 256B padding)
- PC-2 ✅ (= test UBO shell C++ 接続 = block-level bring-up、(c) 採用)
- PC-3 ✅ (= `sAssetUboPool` grow algorithm + unittest 10/10、(α) 採用)
- PC-4 ✅ (= `LLUboRingBuffer` ring buffer algorithm + cvar 露出 + unittest 11/11、(α') 採用)
- PC-5 ✅ (= `LLPipelineCacheStorage` disk persist + 64 MB cap algorithm + cvar 露出 + unittest 13/13、(α'') + (e1) 採用)
- **PC-6α ✅ 本 commit** (= `LLAssetUboPool` × Vulkan device 実 wire up = factory injection + 起動時 1 物理 pool prealloc + shutdown reverse 順 destroy + llrender build PASS + TUT 10/10 + codegen 130/130)
- PC-6β..PC-N ⏳ 次 session

---

## §7 self-verify 9 観点 全 ✅

1. **PC-6α Exit Criteria 6 項全充足** = §4 record = (i) factory 配線 + (ii) prealloc + (iii) reverse 順 destroy + (iv) PC-3 TUT regression なし + (v) llrender build PASS + (vi) codegen regression なし
2. **per-pool sizing 値 source doc 整合** = design 07 §6.3 (= maxSets=192 / UBO=576 / SAMPLER=9408)、code constexpr 値完全一致
3. **PC-6 α..ζ 分割採用根拠 3 件 record** = §2.3 = precedent 整合 + `feedback_ubo_migration_one_at_a_time` 整合 + `feedback_no_scope_shrink` 整合
4. **GATE-B 整合** = `mUseUBO` runtime gate に依存しない Vulkan 初期化層 (= 本 PC-6α は Vulkan 初期化時に無条件 prealloc、PC-3 (α) algorithm 層は mUseUBO 不依存と同形)
5. **MUSEUBO-A 整合** = 本 PC-6α は acquire 呼出経路無し (= 5 cadence update site は PC-6δ)、pool 物理確保のみで既存 OpenGL 描画は touch せず、`mUseUBO=false` default で 100% 維持
6. **llrender target build PASS** + **INTEGRATION_TEST_llassetubopool POST_BUILD 10/10 PASS**
7. **codegen unittest 130/130 PASS** (= Phase 1.A / 1.B / 1.C PC-1..PC-5 regression なし)
8. **commit 内容 prep** = 1 modified (`indra/llrender/llvkloader.cpp` 4 編集) + 1 new doc (本 handoff) + 新 file 0 + CMake 改変 0 + settings.xml 改変 0 + Co-Authored-By 不在
9. **`feedback_no_scope_shrink` 遵守** = PC-6 α..ζ 分割は scope 縮小ではない (= §2.3 根拠 3 件)、PC-6α literal scope (W2) 完全実施

---

## §8 引き継ぎ memory (= 既存活用、新規追加なし)

| memory | 適用観点 |
|---|---|
| `project_ayastorm_r41_vulkan_migration` | r41 milestone state (= §6) |
| `project_r41_phase1b_vulkan_host_gate` | GATE-B = `mUseUBO` runtime gate のみ、本 PC-6α Vulkan 初期化層は無関係 |
| `project_ayastorm_r41_design_principles` | (1) Upstream OpenGL 取り込みやすさ維持 + (2) Core プロセス分散実現 → DI callback で algorithm 層 ↔ Vulkan 層分離 (= 原則 2 整合) |
| `feedback_ubo_migration_one_at_a_time` | PC-6 α..ζ 分割採用根拠 (= §2.3 根拠 2)、1 sub = 1 algorithm 層 wire up |
| `feedback_handoff_minimal_pre_req_read` | §1.1 必読 3 件 + §1.2 pinpoint reference 別記、全件読み禁止 |
| `feedback_self_verify_before_handoff` | §7 9 観点 self-verify 全 ✅ |
| `feedback_build_only_verified` | llrender build PASS + TUT 10/10 + codegen 130/130 で literal 検証取得 |
| `feedback_no_scope_shrink` | PC-6 α..ζ 分割は scope 縮小ではない (= §2.3 根拠 3 件 + §5 残 strict 線形で 6 sub-task 全実施明示) |
| `feedback_doubt_self_first` | PC-6 scope ambiguity 発見で停止 + AYA 判断仰ぎ実施 |
| `feedback_proactive_handoff` | PC-6β 引継 marker 本 handoff で能動 handoff |
| `feedback_release_branch_workflow` | feature branch (`feature/ayastorm-r41-gl-removal`) 上で work |
| `feedback_no_auto_commit` | AYA 「commit してください」literal 受領後 commit |
| `feedback_no_claude_coauthor` | Co-Authored-By 行不在 |
| `feedback_design_phase_no_code_write` | 本 PC-6α は実装 phase (= design phase 不該当)、ただし scope 厳守 (= W2 wire up のみ、ring buffer / PSC / 5 cadence / SAMPLER skip は PC-6β 以降) で精神準拠 |

---

## §9 commit 段取り = PC-6α 単独 commit (= PC-3 / PC-4 / PC-5 既 commit 済 + PC-6 α..ζ 分割採用)

git status 想定:
```
M indra/llrender/llvkloader.cpp                                                 (PC-6α 4 編集)
?? docs/.../handoff-substep-...-phase1-c-pc-6-alpha.md                          (本 handoff doc)
```

**PC-6α commit 内容** (= AYA 「commit してください」literal 受領後実行):
- `indra/llrender/llvkloader.cpp` (modified、4 編集)
- `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-6-alpha.md` (本 doc)

新 file / CMake / settings.xml / tests/ への touch 一切なし (= scope 最小、`feedback_tests_dir_never_commit` 自然遵守)。

---

## §10 次 session 着手 1 line

**PC-6β 着手** = (RB) `LLUboRingBuffer` 実 `vmaCreateBuffer` HOST_VISIBLE+MAPPED factory injection wire up + cvar `AYARingBufferSizeMB` 読込 hookup。`indra/llrender/llvkloader.cpp` 追加 helper `createDrawUboRingBuffer()` (仮称) で `BufferAllocator` lambda (= `vmaCreateBuffer` + HOST_VISIBLE + HOST_COHERENT + MAPPED) + `BufferDestroyer` lambda (= `vmaDestroyBuffer`) を closure capture して `std::make_unique<LLUboRingBuffer>(factory, destroyer, mb=cvar 読込値 default=4)` 配線、PC-6α と同 pattern + cvar 読込は `gSavedSettings.getU32("AYARingBufferSizeMB")` 経由 (起動時 1 回読込、再起動反映)。

---

## §11 次 session bootstrap (= AYA から次 session に投げる短い要約 candidate)

```
前 session で PC-6α = (W2) LLAssetUboPool × Vulkan device 実 wire up complete (= indra/llrender/llvkloader.cpp 4 編集 = include + 2 constexpr + sAssetUboPoolMgr unique_ptr + createAssetUboPool() helper (factory + destroyer closure + ctor + initialize) + init chain + shutdown reverse 順 destroy 配線、起動時 1 物理 pool (192 set / 576 UBO / 9408 SAMPLER) prealloc、llrender build PASS + INTEGRATION_TEST_llassetubopool 10/10 PASS + codegen 130/130 PASS、新 file 0 + CMake 改変 0 + settings.xml 改変 0)。PC-6 全体 (= 5 cadence update site 統合 wire up) を α..ζ 6 sub-task strict 線形分割採用 (AYA literal「Claude 推奨」2026-06-04)、PC-6α 完結。本 session 着手 = PC-6β = (RB) LLUboRingBuffer 実 vmaCreateBuffer HOST_VISIBLE+MAPPED factory injection wire up + cvar AYARingBufferSizeMB 読込 hookup。

必読 3 件:
- docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-6-alpha.md (全文)
- docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-5.md (§1.1 必読 3 件 + §3.4 settings.xml cvar block + §10 次 session 着手 1 line)
- docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md (§7 ring buffer + dynamic offset + chunk 構造 + §6.4 4 pool split 採用根拠)

PC-6β から進めてください。
```
