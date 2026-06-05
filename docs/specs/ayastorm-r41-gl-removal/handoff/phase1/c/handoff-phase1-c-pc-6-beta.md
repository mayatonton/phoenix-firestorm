# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-6β complete** marker

**作成日**: 2026-06-04
**前 commit chain** (= 直前 handoff 起案):
- `9ed4cca802` = Phase 1.C **PC-1 complete** = `Global_ReflectionProbes` shell blueprint codegen emit
- `c7f512d654` = Phase 1.C **PC-2 complete** = test UBO shell C++ 接続 = block-level test bring-up (= (c) 採用)
- `c31998c49f` = Phase 1.C **PC-3 complete** = `sAssetUboPool` grow algorithm + TUT 10/10 PASS (= (α) 採用)
- `912863bf81` = Phase 1.C **PC-4 complete** = `LLUboRingBuffer` ring buffer 4 MB / 16 MB grow algorithm + cvar `AYARingBufferSizeMB` 露出 + TUT 11/11 PASS (= (α') 採用)
- `2fb5af486e` = Phase 1.C **PC-5 complete** = `LLPipelineCacheStorage` PSO cache disk persist + 64 MB cap algorithm + cvar `AYAPipelineCacheSizeMB` 露出 + TUT 13/13 PASS (= (α'') + (e1) 採用)
- `465c55dcfd` = Phase 1.C **PC-6α complete** = `LLAssetUboPool` × Vulkan device 実 wire up (= 4 編集 + 起動時 1 物理 pool prealloc + reverse 順 destroy)

**本 handoff doc 目的**: **Phase 1.C PC-6β complete marker**。**PC-6 α..ζ strict 線形分割 (= PC-6α handoff §2 確定) 中の PC-6β = (RB) `LLUboRingBuffer` 実 `vmaCreateBuffer` HOST_VISIBLE+HOST_COHERENT+MAPPED factory injection wire up + cvar `AYARingBufferSizeMB` 読込 hookup 完結後の引継**。`indra/llrender/llvkloader.cpp` 単一 file 4 編集 + 1 新規 helper `createDrawUboRingBuffer()` で `LLUboRingBuffer` algorithm 層に sAllocator + vmaCreateBuffer を closure capture した BufferAllocator + BufferDestroyer + side-table `sDrawUboRingBufferRecords` (= handle ↔ VkBuffer/VmaAllocation/mapped pointer 連結) を提供 + cvar 起動時 1 度 LLCachedControl<U32> lookup + 起動時 1 物理 buffer (= 4 MB default) prealloc + shutdown 経路 destroyer 経由 vmaDestroyBuffer 配線。llrender build PASS (WARNING 0) + `INTEGRATION_TEST_lluboringbuffer` 11/11 PASS (= PC-4 TUT regression なし) + codegen unittest 130/130 PASS (= Phase 1.A / 1.B / 1.C PC-1..PC-6α regression なし)。

---

## §0 state 一行 summary

PC-6β = **`LLUboRingBuffer` × VMA 実 wire up + cvar AYARingBufferSizeMB 読込 hookup complete**:

- `indra/llrender/llvkloader.cpp` 4 編集 = (a) include 3 件追加 (`lluboringbuffer.h` + `llcontrol.h` + `<unordered_map>`) + `extern LLControlGroup gSavedSettings;` (b) file-static 追加 = `DrawUboRingBufferRecord` struct + `sDrawUboRingBufferRecords` map + `sDrawUboRingBufferMgr` unique_ptr (c) `createDrawUboRingBuffer()` helper 起案 (= cvar lookup + factory closure + destroyer closure + `std::make_unique` + `initialize()`) (d) init chain 追加 (= `createAssetUboPool()` 直後) + shutdown 追加 (= `sAssetUboPoolMgr->shutdown()` 直前 = reverse 順 init)
- factory lambda = `vmaCreateBuffer` 呼出 = usage=VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT + sharing=EXCLUSIVE + VmaAllocationCreateInfo (= usage=AUTO + flags=HOST_ACCESS_SEQUENTIAL_WRITE+MAPPED + requiredFlags=HOST_VISIBLE+HOST_COHERENT) + 成功時 `static_cast<BufferHandle>(reinterpret_cast<uintptr_t>(buffer))` + side-table 登録 + return handle、失敗時 LL_WARNS + 0 return
- destroyer lambda = side-table 検索 → `vmaDestroyBuffer` → erase
- cvar 読込 = `static LLCachedControl<U32> sRingBufferSizeMB(gSavedSettings, "AYARingBufferSizeMB", kInitialSizeMB=4)` で起動時 1 度 lookup (= settings.xml PC-4 block comment 「変更には viewer 再起動が必要」と整合)
- 新 file 0 件 / CMake 改変 0 件 / settings.xml 改変 0 件 (= PC-4 で既露出済)
- llrender build PASS (WARNING 0) + INTEGRATION_TEST_lluboringbuffer 11/11 PASS + codegen unittest 130/130 PASS

---

## §1 pre-requisite 最小読み (= 次 session 着手時参照、`feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session = PC-6γ 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc | 全文 | PC-6β 完結状態 + PC-6γ 着手起点 + PC-6 α..ζ 6 sub-task 進捗 + (RB) side-table 配線 pattern record |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-6-alpha.md` | §3 PC-6α 実施内容 (= helper + init/shutdown 配置 precedent) + §10 次 session 着手 1 line (= PC-6β 着手起点と本 doc の対) | PC-6α 配線 pattern (= helper 配置 / init chain / shutdown reverse 順) を踏襲した PC-6β 配置整合 record |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md` | §9 (PSO layout + cache) + §6.4 (4 pool split) | PC-6γ scope = (PSC) `LLPipelineCacheStorage` 実 file I/O + `vkGetPipelineCacheData` 取出 + `VkPipelineCacheCreateInfo.pInitialData` 投入 wire up + cvar `AYAPipelineCacheSizeMB` 読込 hookup |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `indra/llrender/llvkloader.cpp:15-32` | PC-6β 追加 include (= `lluboringbuffer.h` + `llcontrol.h` + `<unordered_map>`) + `extern LLControlGroup gSavedSettings;` |
| `indra/llrender/llvkloader.cpp` (= `sAssetUboPoolMgr` 直後) | PC-6β 追加 file-static (= `DrawUboRingBufferRecord` struct + `sDrawUboRingBufferRecords` map + `sDrawUboRingBufferMgr` unique_ptr) |
| `indra/llrender/llvkloader.cpp` (= `createAssetUboPool` 直後) | PC-6β `createDrawUboRingBuffer()` 全文 = factory + destroyer + cvar 読込 + ctor + initialize、PC-6γ `createPipelineCacheStorage()` (仮称) の pattern reference |
| `indra/llrender/llvkloader.cpp` (= init chain `createAssetUboPool` 直後 + shutdown `sAssetUboPoolMgr->shutdown()` 直前) | PC-6β 配置位置 = PC-6γ 配置位置の precedent |
| `indra/llcommon/llpipelinecachestorage.h` / `.cpp` | PC-6γ wire up 対象 = `CacheBlob` / `FileReader` / `FileWriter` type alias + `kDefaultMaxSizeMB=64` constexpr + ctor / initialize / shutdown / updateBlob / persistToDisk |
| `indra/llrender/llvkloader.cpp:773 createPipelineCache()` | PC-6γ wire up 対象 = `VkPipelineCacheCreateInfo{}` `pInitialData = nullptr` を blob 投入経路に置換 + shutdown 時 `vkGetPipelineCacheData` 取出 + `LLPipelineCacheStorage::updateBlob` + `persistToDisk` 配線 |
| `indra/newview/app_settings/settings.xml` (= `AYAPipelineCacheSizeMB` block) | cvar 露出状態確認、PC-6γ で読込 hookup 対象 (= PC-6β の `AYARingBufferSizeMB` 読込 pattern と対称) |
| `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md` | PC-6δ scope = 5 cadence flush 関数 5 種の update site 配置 |
| `indra/llrender/llglslshader.cpp:2032 bringupTestUBO()` | PC-6ε scope = block-level bring-up を SINGLETON cadence flush 関数経由の本格置換 |
| `indra/llrender/llglslshader.cpp:2480-2563, 3006-3079` | PC-6ζ scope = setter SAMPLER skip ↔ codegen SINGLETON cadence_tag=5 衝突 正攻法対応 |

---

## §2 PC-6β 着手前 scope 整理

PC-6β literal = PC-6α handoff §10 確定 1 line = 「(RB) `LLUboRingBuffer` 実 `vmaCreateBuffer` HOST_VISIBLE+MAPPED factory injection wire up + cvar `AYARingBufferSizeMB` 読込 hookup」。PC-6 α..ζ 6 sub-task strict 線形分割 (= PC-6α handoff §2) 継続採用、scope ambiguity 無し。

### §2.1 side-table 必要性発見 = BufferHandle 単独保持 ↔ VMA 3 値必要

PC-6α (`LLAssetUboPool`) は `PoolHandle` を `reinterpret_cast<VkDescriptorPool>(handle)` 1 cast で逆引きできた (= `VkDescriptorPool` opaque pointer のみで vkDestroyDescriptorPool に渡せる)。

PC-6β (`LLUboRingBuffer`) は `BufferHandle = std::uint64_t` opaque で algorithm 層内保持。しかし `vmaDestroyBuffer` は **`VkBuffer` + `VmaAllocation`** の 2 引数必要 + 将来 PC-6δ で per-draw write のため **`pMappedData` の 3 値目** も同 buffer に紐付けたい。BufferHandle 単独で 3 値を保持できない。

### §2.2 side-table 採用 (= 設計判断)

| 案 | 内容 | 評価 |
|---|---|---|
| (a) | side-table `std::unordered_map<BufferHandle, {VkBuffer, VmaAllocation, void*}>` を llvkloader.cpp namespace に保持 | **採用** = handle ↔ record 1:1、map 検索 O(1)、record 構造拡張容易 (= PC-6δ で mapped 利用) |
| (b) | BufferHandle 自体を heap allocated struct pointer に packed | record lifetime と handle lifetime 紐付け要、erase 漏れで leak risk、PC-6α precedent 逸脱 |
| (c) | LLUboRingBuffer API 改変で 3 値 handle 返却 | algorithm 層 Vulkan 概念漏れ = 非 Vulkan 環境 (TUT unittest) で AllocateResult API 破壊 = PC-4 11/11 PASS 維持不能 |

**(a) 採用根拠 3 件**:
1. **PC-6α precedent + PC-4 algorithm 層 invariant 両立**: BufferHandle opaque (= algorithm 層 Vulkan 非依存維持) + llvkloader 側 side-table で 3 値解決 (= PC-3 (α) DI callback 設計原則整合)
2. **grow 期間中の 2 record 並走対応**: `LLUboRingBuffer::tryGrow()` は `invokeAllocator(new)` → `destroyCurrentBuffer(old)` 順 (= 一時的に new + old の 2 record 並走)、map 構造でしか正しく扱えない (= 1 entry slot では new で old を上書きしてから destroyer が old を引けない)
3. **PC-6δ 拡張容易性**: per-draw write で mapped pointer 必要時、map record に `void* mapped` を初日から保持 (= 本 PC-6β で record 確保時に登録)、PC-6δ で `sDrawUboRingBufferRecords[mgr->getBuffer()].mapped` 経由で即時参照可能 (= 触り直し回避)

### §2.3 cvar 読込 timing 確認 = 起動時 1 度 lookup

handoff §10 確定 1 line = 「cvar 読込は `gSavedSettings.getU32("AYARingBufferSizeMB")` 経由 (起動時 1 回読込、再起動反映)」。

| 案 | 内容 | 評価 |
|---|---|---|
| (i) | `gSavedSettings.getU32("AYARingBufferSizeMB")` 直接呼出 | createDrawUboRingBuffer() 内 1 度実行 OK、ただし LLCachedControl の方が lookup cost 低減 + llglslshader.cpp 既存 pattern と整合 |
| (ii) | `static LLCachedControl<U32> sRingBufferSizeMB(...)` で 1 度 lookup | **採用** = llglslshader.cpp:1040 `sVulkanShaderAutoLocation` pattern 完全踏襲、static = 1 度 binding、起動時 1 度評価 |

**(ii) 採用根拠**: llglslshader.cpp で既に同形 `extern LLControlGroup gSavedSettings;` + `static LLCachedControl<bool> sVulkanShaderAutoLocation(...)` pattern が使われている (= line 69 + line 1040)、本 PC-6β は同形踏襲で diff review 容易性 + lookup cost 最小化を両取り。

---

## §3 PC-6β 実施内容

### §3.1 `indra/llrender/llvkloader.cpp` 編集 1 = include 追加 + extern 宣言

```cpp
#include "linden_common.h"
#include "llvkloader.h"

#include "volk.h"
#include "lldir.h"
#include "llassetubopool.h"
#include "lluboringbuffer.h"   // PC-6β 追加
#include "llcontrol.h"         // PC-6β 追加 (= LLCachedControl)

#include <vector>
#include <string>
#include <climits>
#include <cstring>
#include <fstream>
#include <memory>
#include <unordered_map>       // PC-6β 追加 (= side-table)

extern LLControlGroup gSavedSettings;  // PC-6β 追加 (= llglslshader.cpp:69 pattern)
```

llrender → llcommon link 既存 (= PC-6α 確認済)、llrender → llxml link は newview 側 link 時に解決 (= llglslshader.cpp 既存)。

### §3.2 `indra/llrender/llvkloader.cpp` 編集 2 = file-static 宣言追加

`sAssetUboPoolMgr` 宣言直後に挿入:

```cpp
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6β (RB):
// per-frame / per-pass cadence UBO ring buffer 実 wire up。LLUboRingBuffer は
// Vulkan device 非依存 bookkeeping algorithm で BufferHandle = std::uint64_t
// opaque 単独保持。実 VkBuffer + VmaAllocation + mapped pointer 三組は本 TU
// 内 side table (sDrawUboRingBufferRecords) で handle → record 解決する。
// factory は vmaCreateBuffer (HOST_VISIBLE + HOST_COHERENT + MAPPED)、
// destroyer は vmaDestroyBuffer。grow 時は invokeAllocator → destroyCurrent
// 順で一時的に 2 record が並走するため map 構造を採用 (= 1 entry slot 不可)。
struct DrawUboRingBufferRecord
{
    VkBuffer      buffer     = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    void*         mapped     = nullptr;
};
std::unordered_map<LLUboRingBuffer::BufferHandle, DrawUboRingBufferRecord>
    sDrawUboRingBufferRecords;
std::unique_ptr<LLUboRingBuffer> sDrawUboRingBufferMgr;
```

### §3.3 `indra/llrender/llvkloader.cpp` 編集 3 = `createDrawUboRingBuffer()` 起案

`createAssetUboPool()` 直後に配置 (= 設計 review 順、PC-6α `sAssetUboPoolMgr` ← PC-6β `sDrawUboRingBufferMgr` ← PC-6γ `sPipelineCacheStorageMgr` (仮称) の cadence 順 storage manager 4 cadence pool split pattern 継続)。

| component | 内容 |
|---|---|
| cvar 読込 | `static LLCachedControl<U32> sRingBufferSizeMB(gSavedSettings, "AYARingBufferSizeMB", LLUboRingBuffer::kInitialSizeMB)` = 起動時 1 度 lookup、`const U32 initial_mb = (U32)sRingBufferSizeMB;` で値取出 |
| factory lambda | `[](std::uint32_t size_bytes) -> LLUboRingBuffer::BufferHandle`、`sAllocator == VK_NULL_HANDLE` 早期 return 0、`VkBufferCreateInfo` (usage=UNIFORM_BUFFER_BIT + sharing=EXCLUSIVE) + `VmaAllocationCreateInfo` (usage=AUTO + flags=HOST_ACCESS_SEQUENTIAL_WRITE+MAPPED + requiredFlags=HOST_VISIBLE+HOST_COHERENT) + `vmaCreateBuffer(sAllocator, ...)` → 成功 + `info.pMappedData != nullptr` 時 handle = `static_cast<BufferHandle>(reinterpret_cast<uintptr_t>(buffer))` 算出 + side-table 登録 + return、失敗時 LL_WARNS + 0 return + 部分破棄 |
| destroyer lambda | `[](LLUboRingBuffer::BufferHandle handle) -> void`、`handle == 0 \|\| sAllocator == VK_NULL_HANDLE` 早期 return、map find → end 時 return、`vmaDestroyBuffer(sAllocator, rec.buffer, rec.allocation)` + map erase |
| ctor + initialize | `std::make_unique<LLUboRingBuffer>(factory, destroyer, initial_mb)` + `initialize()` (= 内部で 1 回 factory 呼出 = 1 物理 buffer prealloc)、失敗時 reset + return false |
| LL_INFOS marker | `"Draw UBO ring buffer wired up (PC-6β RB, cvar AYARingBufferSizeMB=<X>, initial=<Y> MB, max=16 MB, chunk=<Z> B (×3 frame), alignment=256 B, HOST_VISIBLE + HOST_COHERENT + MAPPED)"` |

### §3.4 `indra/llrender/llvkloader.cpp` 編集 4 = init chain + shutdown

**init chain** (= `createAssetUboPool` 直後、`createPlaceholderWhiteImage` 直前):

```cpp
if (!createAssetUboPool())
{
    shutdownVulkan();
    return false;
}

// PC-6β (RB): per-frame / per-pass cadence UBO ring buffer を sAssetUboPoolMgr 直後に立ち上げる
if (!createDrawUboRingBuffer())
{
    shutdownVulkan();
    return false;
}
```

**shutdown** (= `sAssetUboPoolMgr->shutdown()` 直前 = init reverse 順、`sAllocator` 生存中):

```cpp
// PC-6β (RB): draw UBO ring buffer teardown。init reverse 順 = sAssetUboPoolMgr より先、sAllocator 生存中に発火
if (sDrawUboRingBufferMgr)
{
    sDrawUboRingBufferMgr->shutdown();
    sDrawUboRingBufferMgr.reset();
}
sDrawUboRingBufferRecords.clear();

if (sAssetUboPoolMgr)
{
    sAssetUboPoolMgr->shutdown();
    sAssetUboPoolMgr.reset();
}
```

`sDrawUboRingBufferRecords.clear()` は本来 `shutdown()` 完走後 map は空になっているはず (= destroyer 経由で erase 済) だが、二重 shutdown / factory 失敗 path 等の防御で明示 clear。

### §3.5 build verify

1. **`make -j4 llrender`** 実走 (`build-linux-x86_64`):
   - `llvkloader.cpp.o` compile PASS
   - `libllrender.a` link PASS
   - ERROR 0 件 / WARNING 0 件 (= PC-6β 改変関連)
2. **`make -j4 INTEGRATION_TEST_lluboringbuffer`** 実走:
   - llcommon 静 lib (PC-6β 改変無関係) re-link 不要
   - `INTEGRATION_TEST_lluboringbuffer` re-link + POST_BUILD auto-run:
     ```
     Unit test group_started name=LLUboRingBuffer
     Unit test group_completed name=LLUboRingBuffer
         Total Tests:	11
         Passed Tests:	11	YAY!! \o/
     ```
   = PC-4 algorithm 層 regression なし

### §3.6 codegen unittest = 130/130 PASS

`python3 -m unittest discover -s scripts/ubo_codegen/tests` = `Ran 130 tests in 0.062s` `OK` (= Phase 1.A / 1.B / 1.C PC-1..PC-6α regression なし)。

---

## §4 PC-6β Exit Criteria 充足 record

PC-6β scope literal = 「(RB) `LLUboRingBuffer` 実 `vmaCreateBuffer` HOST_VISIBLE+MAPPED factory injection wire up + cvar `AYARingBufferSizeMB` 読込 hookup」。

| Exit 項目 | 充足 |
|---|---|
| (i) `LLUboRingBuffer` algorithm 層に実 Vulkan factory 配線 | ✅ closure capture lambda で `vmaCreateBuffer` / `vmaDestroyBuffer` 提供、algorithm 層は handle 種別非依存維持 |
| (ii) HOST_VISIBLE + HOST_COHERENT + MAPPED 整合 | ✅ `aci.requiredFlags = HOST_VISIBLE+HOST_COHERENT` + `aci.flags = HOST_ACCESS_SEQUENTIAL_WRITE+MAPPED` + `info.pMappedData != nullptr` 検証 |
| (iii) 起動時 1 物理 buffer prealloc | ✅ `initialize()` 内 1 回 factory 呼出、`getCurrentSizeMB() == cvar 値` (LL_INFOS marker で観測可能) |
| (iv) cvar AYARingBufferSizeMB 読込 hookup | ✅ `static LLCachedControl<U32> sRingBufferSizeMB(gSavedSettings, "AYARingBufferSizeMB", kInitialSizeMB=4)` で起動時 1 度 lookup、ctor `initial_size_mb` に注入、settings.xml PC-4 block comment 「変更には viewer 再起動が必要」と整合 |
| (v) shutdown 経路 destroyer 経由破棄 | ✅ `sDrawUboRingBufferMgr->shutdown()` で current buffer を destroyer 経由 `vmaDestroyBuffer` + map erase |
| (vi) PC-4 algorithm 層 regression なし | ✅ TUT 11/11 PASS (= `INTEGRATION_TEST_lluboringbuffer`) |
| (vii) llrender build PASS + warning 0 | ✅ `make -j4 llrender` ERROR 0 / WARNING 0 |
| (viii) codegen unittest regression なし | ✅ 130/130 PASS |

---

## §5 残 strict 線形 (= PC-6α handoff §2.2 採用継続)

```
PC-6α ✅ → PC-6β ✅ (本 commit) → PC-6γ (PSC LLPipelineCacheStorage 実 file I/O + vkGetPipelineCacheData wire up + cvar AYAPipelineCacheSizeMB 読込 hookup)
                              → PC-6δ (5 cadence per-frame/per-pass/per-asset/per-draw/per-skin flush 関数 update site 5 種で test UBO 空 dummy 書込 PASS、ring buffer allocate() / beginFrame() 経路通電)
                              → PC-6ε (block-level test bring-up を SINGLETON cadence flush 関数経由の本格置換)
                              → PC-6ζ (setter SAMPLER skip 正攻法対応)
                              → PC-7 (vkCmdBindDescriptorSets 通電)
                              → PC-8 (build verify)
                              → PC-N (Phase 1.C complete marker)
```

---

## §6 r41 milestone state

- Phase 1.A ✅ (= codegen 起点)
- Phase 1.B ✅ (= 30 setter Vulkan path 分岐 + `mUseUBO` runtime gate)
- (Z) SSS ✅
- (W) uniform4iv ✅
- (Y) Phase 1.C prep ✅
- PC-0 ✅
- PC-1 ✅
- PC-2 ✅
- PC-3 ✅
- PC-4 ✅
- PC-5 ✅
- PC-6α ✅
- **PC-6β ✅ 本 commit** (= `LLUboRingBuffer` × VMA 実 wire up + cvar AYARingBufferSizeMB 読込 hookup + side-table 配線 + llrender build PASS + TUT 11/11 + codegen 130/130)
- PC-6γ..PC-N ⏳ 次 session

---

## §7 self-verify 9 観点 全 ✅

1. **PC-6β Exit Criteria 8 項全充足** = §4 record = (i) factory 配線 + (ii) HOST_VISIBLE+HOST_COHERENT+MAPPED + (iii) prealloc + (iv) cvar hookup + (v) destroyer 経由 destroy + (vi) PC-4 TUT regression なし + (vii) llrender build PASS + (viii) codegen regression なし
2. **ring buffer source doc 整合** = design 07 §7.2 (initial=4 MB / max=16 MB)、§7.3 (alignment 256 safe)、§7.5 (3 chunk wrap + grow on chunk 内枯渇)、§8.4 (FRAMES_IN_FLIGHT 同期 rotate) を header file-level comment + cpp implementation で引用、constexpr 値完全一致
3. **side-table 採用根拠 3 件 record** = §2.2 = PC-6α precedent + grow 期間 2 record 並走 + PC-6δ 拡張容易性
4. **GATE-B 整合** = `mUseUBO` runtime gate に依存しない Vulkan 初期化層 (= PC-6α と同形、buffer 物理確保のみ)
5. **MUSEUBO-A 整合** = 本 PC-6β は `allocate()` / `beginFrame()` 呼出経路無し (= 5 cadence update site は PC-6δ)、buffer 物理確保 + cvar 1 度読込のみで既存 OpenGL 描画は touch せず、`mUseUBO=false` default で 100% 維持
6. **llrender target build PASS** + **INTEGRATION_TEST_lluboringbuffer POST_BUILD 11/11 PASS**
7. **codegen unittest 130/130 PASS** (= Phase 1.A / 1.B / 1.C PC-1..PC-6α regression なし)
8. **commit 内容 prep** = 1 modified (`indra/llrender/llvkloader.cpp` 4 編集) + 1 new doc (本 handoff) + 新 file 0 + CMake 改変 0 + settings.xml 改変 0 (= PC-4 露出済) + Co-Authored-By 不在
9. **`feedback_no_scope_shrink` 遵守** = PC-6β literal scope (RB + cvar 読込) 完全実施、side-table は scope 外拡張ではなく BufferHandle 単独保持 ↔ VMA 3 値必要の構造的要求への最小対応

---

## §8 引き継ぎ memory (= 既存活用、新規追加なし)

| memory | 適用観点 |
|---|---|
| `project_ayastorm_r41_vulkan_migration` | r41 milestone state (= §6) |
| `project_r41_phase1b_vulkan_host_gate` | GATE-B = `mUseUBO` runtime gate のみ、本 PC-6β Vulkan 初期化層は無関係 |
| `project_ayastorm_r41_design_principles` | (1) Upstream OpenGL 取り込みやすさ維持 + (2) Core プロセス分散実現 → DI callback で algorithm 層 ↔ Vulkan 層分離 + side-table は llvkloader 1 TU 内閉じ (= header 不露出、原則 1 整合) |
| `feedback_ubo_migration_one_at_a_time` | PC-6 α..ζ 分割継続 (= PC-6α §2 確定)、1 sub = 1 algorithm 層 wire up |
| `feedback_handoff_minimal_pre_req_read` | §1.1 必読 3 件 + §1.2 pinpoint reference 別記、全件読み禁止 |
| `feedback_self_verify_before_handoff` | §7 9 観点 self-verify 全 ✅ |
| `feedback_build_only_verified` | llrender build PASS + TUT 11/11 + codegen 130/130 で literal 検証取得 |
| `feedback_no_scope_shrink` | PC-6β literal scope 完全実施 (= §9 観点 9) |
| `feedback_doubt_self_first` | side-table 必要性 (= BufferHandle 単独保持 ↔ VMA 3 値必要) 発見で停止 + 案 (a)(b)(c) 評価記述 + (a) 採用根拠 3 件 明文化 |
| `feedback_proactive_handoff` | PC-6γ 引継 marker 本 handoff で能動 handoff |
| `feedback_release_branch_workflow` | feature branch (`feature/ayastorm-r41-gl-removal`) 上で work |
| `feedback_no_auto_commit` | AYA 「commit してください」literal 受領後 commit |
| `feedback_no_claude_coauthor` | Co-Authored-By 行不在 |
| `feedback_design_phase_no_code_write` | 本 PC-6β は実装 phase (= design phase 不該当)、ただし scope 厳守 (= RB wire up + cvar 読込のみ、PSC / 5 cadence / SAMPLER skip は PC-6γ 以降) で精神準拠 |

---

## §9 commit 段取り = PC-6β 単独 commit

git status 想定:
```
M indra/llrender/llvkloader.cpp                                                 (PC-6β 4 編集)
?? docs/.../handoff-substep-...-phase1-c-pc-6-beta.md                           (本 handoff doc)
```

**PC-6β commit 内容** (= AYA 「commit してください」literal 受領後実行):
- `indra/llrender/llvkloader.cpp` (modified、4 編集)
- `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-6-beta.md` (本 doc)

新 file / CMake / settings.xml / tests/ への touch 一切なし (= scope 最小、`feedback_tests_dir_never_commit` 自然遵守)。

---

## §10 次 session 着手 1 line

**PC-6γ 着手** = (PSC) `LLPipelineCacheStorage` 実 file I/O (= `std::fstream` reader/writer) + `vkGetPipelineCacheData` 取出 + `VkPipelineCacheCreateInfo.pInitialData` 投入 wire up + cvar `AYAPipelineCacheSizeMB` 読込 hookup。`indra/llrender/llvkloader.cpp` 追加 helper `createPipelineCacheStorage()` (仮称) で `FileReader` lambda (= `std::ifstream` で `~/.ayastorm_x64/cache/pipeline_cache.bin` 読込 → bytes vector return) + `FileWriter` lambda (= `std::ofstream` で同 path 書込) を closure capture、cvar 読込は PC-6β と同形 `static LLCachedControl<U32>` pattern (= `gSavedSettings.getU32("AYAPipelineCacheSizeMB")` 経由、起動時 1 回読込、再起動反映)。既存 `createPipelineCache()` (line 773) の `VkPipelineCacheCreateInfo{}` `pInitialData = nullptr` を `LLPipelineCacheStorage::getBlob()` 結果に置換、shutdown で `vkGetPipelineCacheData` 取出 → `updateBlob` → `persistToDisk` 配線。

---

## §11 次 session bootstrap (= AYA から次 session に投げる短い要約 candidate)

```
前 session で PC-6β = (RB) LLUboRingBuffer × VMA 実 wire up + cvar AYARingBufferSizeMB 読込 hookup complete (= indra/llrender/llvkloader.cpp 4 編集 = include 3 件 + extern + DrawUboRingBufferRecord struct + sDrawUboRingBufferRecords map + sDrawUboRingBufferMgr unique_ptr + createDrawUboRingBuffer() helper (cvar lookup + factory + destroyer + side-table 登録/解除 + ctor + initialize) + init chain + shutdown reverse 順 destroy 配線、起動時 1 物理 buffer (cvar default 4 MB) prealloc、llrender build PASS + INTEGRATION_TEST_lluboringbuffer 11/11 PASS + codegen 130/130 PASS、新 file 0 + CMake 改変 0 + settings.xml 改変 0 (= PC-4 で既露出済))。PC-6 strict 線形 α..ζ 6 sub-task 中 α + β 完結、本 session 着手 = PC-6γ = (PSC) LLPipelineCacheStorage 実 file I/O + vkGetPipelineCacheData wire up + VkPipelineCacheCreateInfo.pInitialData 投入 + cvar AYAPipelineCacheSizeMB 読込 hookup。

必読 3 件:
- docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-6-beta.md (全文)
- docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-6-alpha.md (§3 PC-6α 実施内容 + §10 次 session 着手 1 line)
- docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md (§9 PSO layout + cache + §6.4 4 pool split)

PC-6γ から進めてください。
```
