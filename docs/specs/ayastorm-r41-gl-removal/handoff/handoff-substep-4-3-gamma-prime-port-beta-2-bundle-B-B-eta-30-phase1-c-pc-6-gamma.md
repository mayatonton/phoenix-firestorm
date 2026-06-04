# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-6γ complete** marker

**作成日**: 2026-06-04
**前 commit chain** (= 直前 handoff 起案):
- `9ed4cca802` = Phase 1.C **PC-1 complete** = `Global_ReflectionProbes` shell blueprint codegen emit
- `c7f512d654` = Phase 1.C **PC-2 complete** = test UBO shell C++ 接続 = block-level test bring-up (= (c) 採用)
- `c31998c49f` = Phase 1.C **PC-3 complete** = `sAssetUboPool` grow algorithm + TUT 10/10 PASS (= (α) 採用)
- `912863bf81` = Phase 1.C **PC-4 complete** = `LLUboRingBuffer` ring buffer 4 MB / 16 MB grow algorithm + cvar `AYARingBufferSizeMB` 露出 + TUT 11/11 PASS (= (α') 採用)
- `2fb5af486e` = Phase 1.C **PC-5 complete** = `LLPipelineCacheStorage` PSO cache disk persist + 64 MB cap algorithm + cvar `AYAPipelineCacheSizeMB` 露出 + TUT 13/13 PASS (= (α'') + (e1) 採用)
- `465c55dcfd` = Phase 1.C **PC-6α complete** = `LLAssetUboPool` × Vulkan device 実 wire up (= 4 編集 + 起動時 1 物理 pool prealloc + reverse 順 destroy)
- `e1d23a767a` = Phase 1.C **PC-6β complete** = `LLUboRingBuffer` × VMA 実 wire up + cvar `AYARingBufferSizeMB` 読込 hookup (= side-table 配線 + 起動時 1 物理 buffer prealloc)

**本 handoff doc 目的**: **Phase 1.C PC-6γ complete marker**。**PC-6 α..ζ strict 線形分割 (= PC-6α handoff §2 確定) 中の PC-6γ = (PSC) `LLPipelineCacheStorage` 実 file I/O (= `std::ifstream` reader / `std::ofstream` writer) + `vkGetPipelineCacheData` 取出 + `VkPipelineCacheCreateInfo.pInitialData` 投入 wire up + cvar `AYAPipelineCacheSizeMB` 読込 hookup 完結後の引継**。`indra/llrender/llvkloader.cpp` 単一 file 5 編集 + 1 新規 helper `createPipelineCacheStorage()` で `LLPipelineCacheStorage` algorithm 層に FileReader / FileWriter lambda + cvar 起動時 1 度 LLCachedControl<U32> lookup + 起動時 file load (e1 cap 超過 blob 破棄) + 既存 `createPipelineCache()` の `pInitialData` を blob 投入経路化 + shutdown 時 `vkGetPipelineCacheData` → `updateBlob` → `persistToDisk` → `vkDestroyPipelineCache` 順配線。llrender build PASS (WARNING 0) + `INTEGRATION_TEST_llpipelinecachestorage` 13/13 PASS (= PC-5 TUT regression なし) + `INTEGRATION_TEST_llassetubopool` 10/10 PASS + `INTEGRATION_TEST_lluboringbuffer` 11/11 PASS + codegen unittest 130/130 PASS (= Phase 1.A / 1.B / 1.C PC-1..PC-6β regression なし)。

---

## §0 state 一行 summary

PC-6γ = **`LLPipelineCacheStorage` × Vulkan device 実 wire up + cvar AYAPipelineCacheSizeMB 読込 hookup complete**:

- `indra/llrender/llvkloader.cpp` 5 編集 = (a) include `llpipelinecachestorage.h` 追加 (b) file-static `sPipelineCacheStorageMgr` unique_ptr + 配線意図 comment 追加 (c) `createPipelineCacheStorage()` helper 起案 (= cvar lookup + path 解決 + FileReader/FileWriter lambda + `std::make_unique` + `initialize()`) (d) `createPipelineCache()` の `VkPipelineCacheCreateInfo.pInitialData` を blob 投入経路化 + LL_INFOS marker 拡張 (e) init chain で `createPipelineCacheStorage()` を `createPipelineCache()` より前に呼出 + shutdown 経路で `vkGetPipelineCacheData` → `updateBlob` → `persistToDisk` → `vkDestroyPipelineCache` → storage manager reset 配線
- file path = `gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "pipeline_cache.bin")` (= design 07 §9.3 確定値 `~/.ayastorm_x64/cache/pipeline_cache.bin` と整合、AYAstorm 3 OS で gDirUtilp の `LL_PATH_CACHE` 経由 path 解決)
- cvar 読込 = `static LLCachedControl<U32> sPipelineCacheSizeMB(gSavedSettings, "AYAPipelineCacheSizeMB", LLPipelineCacheStorage::kDefaultMaxSizeMB=64)` で起動時 1 度 lookup (= settings.xml PC-5 block comment 「変更には viewer 再起動が必要」と整合)
- 新 file 0 件 / CMake 改変 0 件 / settings.xml 改変 0 件 (= PC-5 で既露出済)
- llrender build PASS (WARNING 0) + INTEGRATION_TEST_llpipelinecachestorage 13/13 PASS + INTEGRATION_TEST_llassetubopool 10/10 PASS + INTEGRATION_TEST_lluboringbuffer 11/11 PASS + codegen unittest 130/130 PASS

---

## §1 pre-requisite 最小読み (= 次 session 着手時参照、`feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session = PC-6δ 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc | 全文 | PC-6γ 完結状態 + PC-6δ 着手起点 + PC-6 α..ζ 6 sub-task 進捗 + (PSC) blob 投入経路 + 起動 load / shutdown persist record |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-6-beta.md` | §3 PC-6β 実施内容 (= helper + init/shutdown 配置 precedent) + §10 次 session 着手 1 line (= PC-6γ 着手起点と本 doc の対) | PC-6β 配線 pattern (= cvar 読込 / helper 配置 / init chain / shutdown reverse 順) を踏襲した PC-6γ 配置整合 record |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md` | §4 (= flush 関数 5 種) + §4.3 (= triple-buffering U1=3) + §5.3 (= L1+L2 default) | PC-6δ scope = 5 cadence (per-frame / per-pass / per-asset / per-draw / per-skin) flush 関数 update site 5 種で test UBO 空 dummy 書込 PASS → ring buffer `allocate()` / `beginFrame()` 経路通電 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `indra/llrender/llvkloader.cpp:15-33` | PC-6γ 追加 include (= `llpipelinecachestorage.h`) + PC-6α/β 既存 include 整合 |
| `indra/llrender/llvkloader.cpp:399-411` | PC-6γ 追加 file-static (= `sPipelineCacheStorageMgr` unique_ptr) + 配線意図 comment 全文 |
| `indra/llrender/llvkloader.cpp` (= `createPipelineCache()` body) | PC-6γ blob 投入経路化 (= `info.pInitialData = blob.data()` + 初回起動 empty=miss / hit 経路) |
| `indra/llrender/llvkloader.cpp` (= `createDrawUboRingBuffer()` 直後) | PC-6γ `createPipelineCacheStorage()` 全文 = cvar lookup + path 解決 + reader lambda + writer lambda + ctor + initialize、PC-6δ で同 helper 経由参照は無し (= storage 層独立、cadence update site とは無関係) |
| `indra/llrender/llvkloader.cpp` (= shutdown `sPerMaterialDescriptorSet = VK_NULL_HANDLE;` 直後 + `vkDestroyPipelineCache` 直前) | PC-6γ shutdown 経路 (= `vkGetPipelineCacheData` 2-pass + `updateBlob` + `persistToDisk` + LL_INFOS) record |
| `indra/llrender/llvkloader.cpp` (= init chain `createPipelineCache()` 直前) | PC-6γ init chain 配置位置 (= `createPipelineCacheStorage()` は `createPipelineCache()` より先) precedent |
| `indra/llcommon/lluboringbuffer.h` | PC-6δ wire up 対象 = `AllocateResult` (= buffer/offset/size/success/grew) + `allocate(size_bytes)` + `beginFrame()` API |
| `indra/llrender/llglslshader.cpp` (= 5 flush 関数 + cadence dispatch) | PC-6δ wire up 対象 = `flushFrameUbos()` / `flushPassUbos()` / `flushAssetUbos()` / `flushDrawUbos()` / `flushSkinUbos()` 5 種 update site 配置 |
| `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md` (§4 全文) | PC-6δ scope = 5 cadence flush 関数 5 種の update site 配置 + L1 dirty bit + L2 dynamic offset 経路 |
| `indra/llrender/llglslshader.cpp:2032 bringupTestUBO()` | PC-6ε scope = block-level bring-up を SINGLETON cadence flush 関数経由の本格置換 |
| `indra/llrender/llglslshader.cpp:2480-2563, 3006-3079` | PC-6ζ scope = setter SAMPLER skip ↔ codegen SINGLETON cadence_tag=5 衝突 正攻法対応 |

---

## §2 PC-6γ 着手前 scope 整理

PC-6γ literal = PC-6β handoff §10 確定 1 line = 「(PSC) `LLPipelineCacheStorage` 実 file I/O (= `std::fstream` reader/writer) + `vkGetPipelineCacheData` 取出 + `VkPipelineCacheCreateInfo.pInitialData` 投入 wire up + cvar `AYAPipelineCacheSizeMB` 読込 hookup」。PC-6 α..ζ 6 sub-task strict 線形分割 (= PC-6α handoff §2) 継続採用、scope ambiguity 無し。

### §2.1 init chain ordering 発見 = createPipelineCacheStorage は createPipelineCache の前

`createPipelineCache()` (= line 798) は `VkPipelineCacheCreateInfo.pInitialData` を渡す呼出。本 PC-6γ で blob 投入経路化するためには **storage 側が先に initialize() で file load を完了している必要** がある。一方 PC-6α / PC-6β の cadence storage manager (= sAssetUboPoolMgr / sDrawUboRingBufferMgr) は `createPipelineCache()` の後 (= sSharedDescriptorPool 直後) で init される pattern だった。

= 本 PC-6γ では cadence 順 (= PC-6α sAssetUboPoolMgr ← PC-6β sDrawUboRingBufferMgr ← PC-6γ sPipelineCacheStorageMgr) を init 順序として保ったまま **storage 化 wire up を `createPipelineCache()` 前置** に修正。

### §2.2 init chain 配置位置の 3 案検討

| 案 | 内容 | 評価 |
|---|---|---|
| (1) | `createPipelineCache()` の前に `createPipelineCacheStorage()` 独立呼出 | **採用** = ordering 明示 + diff 局所化、PC-6α/β cadence 順を init 経路で破らず手前で 1 行追加 |
| (2) | `createPipelineCache()` 内で `createPipelineCacheStorage()` 呼出 | helper 純度低下 (= 1 helper = 1 責務 precedent 逸脱)、cadence 順序が見えない |
| (3) | sSharedDescriptorPool 後の cadence 順位置で initialize、blob lookup は `vkCreatePipelineCache` を後段で呼直し | 既存 `createPipelineCache()` を分割改変必要、現 init chain の order 大幅変更で diff review コスト増 |

**(1) 採用根拠 3 件**:
1. **PC-6α/β precedent との位置整合**: cadence 順 (storage cadence < flush cadence < draw cadence) で storage は最上流。本 PC-6γ の PSC は **PSO compile pre-condition** であり cadence 順より更に上流 = init chain の `createCommandPool` etc. 群直前が自然
2. **diff 局所化**: 既存 `createPipelineCache()` の血脈 (= line 798 helper) を温存し blob 投入のみ patch、init chain は手前で 1 if 分追加のみで完結
3. **shutdownVulkan() reverse 順整合**: shutdown は `vkDestroyPipelineCache` 直前で `vkGetPipelineCacheData` → persist 実行、storage manager destroy はその直後 = init reverse 順を破らない

### §2.3 cvar 読込 timing 確認 = 起動時 1 度 lookup

PC-6β handoff §2.3 確定 1 line と同形 = `static LLCachedControl<U32> sPipelineCacheSizeMB(gSavedSettings, "AYAPipelineCacheSizeMB", LLPipelineCacheStorage::kDefaultMaxSizeMB)` で起動時 1 度 lookup、settings.xml PC-5 block comment 「変更には viewer 再起動が必要」と整合。

### §2.4 cache path 解決方式 = gDirUtilp 経由

design 07 §9.3 + §12 (PSC) 確定値 = `~/.ayastorm_x64/cache/pipeline_cache.bin` = AYAstorm 3 OS 全部で `gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "pipeline_cache.bin")` で path 解決。`LL_PATH_CACHE` は viewer init で directory 創出済 = pipeline cache write 時 parent dir 存在保証あり。

---

## §3 PC-6γ 実施内容

### §3.1 `indra/llrender/llvkloader.cpp` 編集 1 = include 追加

```cpp
#include "llassetubopool.h"
#include "lluboringbuffer.h"
#include "llpipelinecachestorage.h"   // PC-6γ 追加
#include "llcontrol.h"
```

llrender → llcommon link 既存 (= PC-6α / PC-6β 確認済)、include path 解決済。

### §3.2 `indra/llrender/llvkloader.cpp` 編集 2 = file-static 宣言追加

`sDrawUboRingBufferMgr` 宣言直後 + `sSharedDescriptorPool` 宣言直前に挿入:

```cpp
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6γ (PSC):
// VkPipelineCache blob の disk persist 機構 (= LLPipelineCacheStorage)。
// 起動時に file (= gDirUtilp LL_PATH_CACHE + "pipeline_cache.bin") から
// blob を load し、createPipelineCache() が VkPipelineCacheCreateInfo.
// pInitialData に投入することで PSO compile hit を確保。shutdownVulkan()
// で vkGetPipelineCacheData → updateBlob → persistToDisk により次回起動
// 向け blob を上書き保存。64 MB 上限は LLPipelineCacheStorage 側で
// enforce ((e1) = load 時 size > cap で blob 破棄 + persist 時 size > cap
// で writer 不呼出 + false return)、cvar AYAPipelineCacheSizeMB で配信。
// design 07 §9.3 (PSO cache 戦略) + §12 (PSC) 整合。
std::unique_ptr<LLPipelineCacheStorage> sPipelineCacheStorageMgr;
```

### §3.3 `indra/llrender/llvkloader.cpp` 編集 3 = `createPipelineCacheStorage()` 起案

`createDrawUboRingBuffer()` 直後に配置 (= cadence 順 storage manager 配置 pattern 継続、ただし init chain では `createPipelineCache()` 前置で呼出)。

| component | 内容 |
|---|---|
| cvar 読込 | `static LLCachedControl<U32> sPipelineCacheSizeMB(gSavedSettings, "AYAPipelineCacheSizeMB", LLPipelineCacheStorage::kDefaultMaxSizeMB=64)` = 起動時 1 度 lookup、`const U32 cap_mb = (U32)sPipelineCacheSizeMB;` で値取出 |
| path 解決 | `std::string file_path = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "pipeline_cache.bin")` |
| reader lambda | `[](const std::string& path, LLPipelineCacheStorage::CacheBlob& out) -> bool`、`std::ifstream f(path, std::ios::binary \| std::ios::ate)` + `is_open()` false 時 false return (= file 不在許容)、`tellg()` size <= 0 false return、`seekg(0)` + `out.resize(size)` + `f.read(...)` 失敗時 `out.clear()` + `shrink_to_fit()` + false return、成功時 true return |
| writer lambda | `[](const std::string& path, const LLPipelineCacheStorage::CacheBlob& data) -> bool`、`std::ofstream f(path, std::ios::binary \| std::ios::trunc)` + `is_open()` false 時 LL_WARNS + false return、`data.empty()` 時 write skip + `f.good()` return、非空時 `f.write(...)` 後 `f.good()` return |
| ctor + initialize | `std::make_unique<LLPipelineCacheStorage>(reader, writer, file_path, cap_mb)` + `initialize()` (= file load + (e1) cap 超過時 blob 破棄)、失敗時 LL_WARNS + reset + false return |
| LL_INFOS marker | `"Pipeline cache storage wired up (PC-6γ PSC, cvar AYAPipelineCacheSizeMB=<X> MB, path=<path>, initial blob=<Y> bytes (<Z> MB), within limit=<yes/no>)"` |

### §3.4 `indra/llrender/llvkloader.cpp` 編集 4 = `createPipelineCache()` blob 投入経路化

```cpp
bool createPipelineCache()
{
    // (略 PC-6γ comment)
    VkPipelineCacheCreateInfo info = {};
    info.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    info.initialDataSize = 0;
    info.pInitialData    = nullptr;
    if (sPipelineCacheStorageMgr)
    {
        const auto& blob = sPipelineCacheStorageMgr->getBlob();
        if (!blob.empty())
        {
            info.initialDataSize = blob.size();
            info.pInitialData    = blob.data();
        }
    }
    // (中略 vkCreatePipelineCache 呼出 + 失敗時 LL_WARNS)
    LL_INFOS("Vulkan") << "VkPipelineCache created (PC-6γ PSC initial blob="
                       << (S32)info.initialDataSize << " bytes)" << LL_ENDL;
    return true;
}
```

`sPipelineCacheStorageMgr` 不在時 / blob 空時は従来通り `pInitialData = nullptr` (= empty cache start、初回起動 miss 想定 + (e1) cap 超過 blob 破棄後の strap)。

### §3.5 `indra/llrender/llvkloader.cpp` 編集 5 = init chain + shutdown

**init chain** (= `createCommandPool()` チェーン直前に挿入):

```cpp
// PC-6γ (PSC): VkPipelineCache 用 disk-persist storage を createPipelineCache() より前に立ち上げる
if (!createPipelineCacheStorage())
{
    shutdownVulkan();
    return false;
}

if (!createCommandPool() || !createOffscreenImage() || !createRenderPass() || !createFramebuffer() || !createPipelineCache())
{
    shutdownVulkan();
    return false;
}
```

**shutdown** (= `sPerMaterialDescriptorSet = VK_NULL_HANDLE;` 直後 + `vkDestroyPipelineCache` 直前):

```cpp
// PC-6γ (PSC): pipeline cache の disk persist。vkDestroyPipelineCache 前に
// vkGetPipelineCacheData → updateBlob → persistToDisk で次回起動向 cache を保存。
if (sPipelineCache != VK_NULL_HANDLE && sPipelineCacheStorageMgr)
{
    std::size_t blob_size = 0;
    VkResult sz_res = vkGetPipelineCacheData(sDevice, sPipelineCache, &blob_size, nullptr);
    if (sz_res == VK_SUCCESS && blob_size > 0)
    {
        LLPipelineCacheStorage::CacheBlob blob(blob_size);
        VkResult get_res = vkGetPipelineCacheData(sDevice, sPipelineCache,
                                                  &blob_size, blob.data());
        if (get_res == VK_SUCCESS || get_res == VK_INCOMPLETE)
        {
            blob.resize(blob_size);
            sPipelineCacheStorageMgr->updateBlob(std::move(blob));
            const bool persisted = sPipelineCacheStorageMgr->persistToDisk();
            LL_INFOS("Vulkan") << "Pipeline cache shutdown persist (PC-6γ PSC, blob="
                               << sPipelineCacheStorageMgr->getBlobSize()
                               << " bytes, persisted="
                               << (persisted ? "yes" : "no (cap exceeded or write failed)")
                               << ")" << LL_ENDL;
        }
        // (else: LL_WARNS get/sz path)
    }
}
if (sPipelineCache != VK_NULL_HANDLE)
{
    vkDestroyPipelineCache(sDevice, sPipelineCache, nullptr);
    sPipelineCache = VK_NULL_HANDLE;
}
// PC-6γ (PSC): storage manager teardown。disk persist 実施済、shutdown() は blob clear のみ
if (sPipelineCacheStorageMgr)
{
    sPipelineCacheStorageMgr->shutdown();
    sPipelineCacheStorageMgr.reset();
}
```

`vkGetPipelineCacheData` Vulkan spec 2-pass pattern (= 1st call で size query + 2nd call で data fill) を厳守。`VK_INCOMPLETE` も `blob.resize(blob_size)` で actual written bytes に短縮可、`blob` 自体は事前確保サイズ ≥ written のため safe。

### §3.6 build verify

1. **`make -j4 llrender`** 実走 (`build-linux-x86_64`):
   - `llvkloader.cpp.o` compile PASS
   - `libllrender.a` link PASS
   - ERROR 0 件 / WARNING 0 件 (= PC-6γ 改変関連)
2. **`make -j4 INTEGRATION_TEST_llpipelinecachestorage`** = 再 build 不要 (= llcommon 静 lib 改変無関係、test 実行 file 直接実走で確認):
   ```
   Unit test group_started name=LLPipelineCacheStorage
   Unit test group_completed name=LLPipelineCacheStorage
       Total Tests:	13
       Passed Tests:	13	YAY!! \o/
   ```
   = PC-5 algorithm 層 regression なし
3. **`INTEGRATION_TEST_llassetubopool` 10/10 PASS** + **`INTEGRATION_TEST_lluboringbuffer` 11/11 PASS** (= PC-3 / PC-4 algorithm 層 regression なし)

### §3.7 codegen unittest = 130/130 PASS

`python3 -m unittest discover -s scripts/ubo_codegen/tests` = `Ran 130 tests in 0.062s` `OK` (= Phase 1.A / 1.B / 1.C PC-1..PC-6β regression なし)。

---

## §4 PC-6γ Exit Criteria 充足 record

PC-6γ scope literal = 「(PSC) `LLPipelineCacheStorage` 実 file I/O + `vkGetPipelineCacheData` 取出 + `VkPipelineCacheCreateInfo.pInitialData` 投入 wire up + cvar `AYAPipelineCacheSizeMB` 読込 hookup」。

| Exit 項目 | 充足 |
|---|---|
| (i) `LLPipelineCacheStorage` algorithm 層に実 file I/O 配線 | ✅ closure capture lambda で `std::ifstream` reader + `std::ofstream` writer 提供、algorithm 層は file system 非依存維持 |
| (ii) 起動時 file load + (e1) cap 超過 blob 破棄機構 | ✅ `createPipelineCacheStorage()` 内 `initialize()` 1 回呼出、内部で reader 起動 + size > cap 時 blob 破棄 (= LLPipelineCacheStorage::initialize() 実装) |
| (iii) `VkPipelineCacheCreateInfo.pInitialData` 投入 | ✅ `createPipelineCache()` で `sPipelineCacheStorageMgr->getBlob().data()` 投入、blob 空時は `nullptr` 維持 (= 初回 miss path) |
| (iv) shutdown 時 `vkGetPipelineCacheData` 2-pass + `updateBlob` + `persistToDisk` | ✅ 1st call で size query + 2nd call で data fill + `VK_INCOMPLETE` 許容 + `blob.resize(written)` + `persistToDisk` (= (e1) cap 超過時 writer 不呼出 + false return) |
| (v) cvar AYAPipelineCacheSizeMB 読込 hookup | ✅ `static LLCachedControl<U32> sPipelineCacheSizeMB(gSavedSettings, "AYAPipelineCacheSizeMB", kDefaultMaxSizeMB=64)` で起動時 1 度 lookup、ctor `max_size_mb` に注入、settings.xml PC-5 block comment 「変更には viewer 再起動が必要」と整合 |
| (vi) cache file path 解決 = AYAstorm 3 OS 整合 | ✅ `gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "pipeline_cache.bin")` (= design 07 §9.3 `~/.ayastorm_x64/cache/pipeline_cache.bin` Linux 確定値 + macOS/Windows は LL_PATH_CACHE 各 platform 解決に委譲) |
| (vii) PC-5 algorithm 層 regression なし | ✅ INTEGRATION_TEST_llpipelinecachestorage 13/13 PASS |
| (viii) PC-3 / PC-4 algorithm 層 regression なし | ✅ INTEGRATION_TEST_llassetubopool 10/10 + INTEGRATION_TEST_lluboringbuffer 11/11 PASS |
| (ix) llrender build PASS + warning 0 | ✅ `make -j4 llrender` ERROR 0 / WARNING 0 |
| (x) codegen unittest regression なし | ✅ 130/130 PASS |

---

## §5 残 strict 線形 (= PC-6α handoff §2.2 採用継続)

```
PC-6α ✅ → PC-6β ✅ → PC-6γ ✅ (本 commit) → PC-6δ (5 cadence per-frame/per-pass/per-asset/per-draw/per-skin flush 関数 update site 5 種で test UBO 空 dummy 書込 PASS、ring buffer allocate() / beginFrame() 経路通電)
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
- PC-6β ✅
- **PC-6γ ✅ 本 commit** (= `LLPipelineCacheStorage` × Vulkan device 実 wire up + cvar AYAPipelineCacheSizeMB 読込 hookup + blob 投入 / persist 経路配線 + llrender build PASS + TUT 13/13 + 10/10 + 11/11 + codegen 130/130)
- PC-6δ..PC-N ⏳ 次 session

---

## §7 self-verify 9 観点 全 ✅

1. **PC-6γ Exit Criteria 10 項全充足** = §4 record = (i) file I/O 配線 + (ii) load + (e1) cap 破棄 + (iii) pInitialData 投入 + (iv) shutdown 2-pass persist + (v) cvar hookup + (vi) path 3 OS 整合 + (vii) PC-5 TUT regression なし + (viii) PC-3/4 TUT regression なし + (ix) llrender build PASS + (x) codegen regression なし
2. **PSC source doc 整合** = design 07 §9.3 (PSO cache 戦略 = pInitialData 起動 disk load + 終了 save) + §12 (PSC) (= disk persist path 確定値 `~/.ayastorm_x64/cache/pipeline_cache.bin` + 上限 64 MB) を helper comment + LL_INFOS marker で引用、kDefaultMaxSizeMB=64 値完全一致
3. **init ordering 採用根拠 3 件 record** = §2.2 = PC-6α/β cadence 順整合 + diff 局所化 + shutdown reverse 順整合
4. **GATE-B 整合** = `mUseUBO` runtime gate 不依存 Vulkan 初期化層 (= PC-6α / PC-6β と同形、storage 物理確保 + file load + persist のみ、UBO redirect path とは独立)
5. **MUSEUBO-A 整合** = 本 PC-6γ は `allocate()` / `beginFrame()` / setter redirect 呼出経路無し (= 5 cadence update site は PC-6δ)、PSO cache 物理確保 + blob load/persist + cvar 1 度読込のみで既存 OpenGL 描画は touch せず、`mUseUBO=false` default で 100% 維持
6. **llrender target build PASS** + **INTEGRATION_TEST_llpipelinecachestorage 13/13 PASS** + **INTEGRATION_TEST_llassetubopool 10/10 PASS** + **INTEGRATION_TEST_lluboringbuffer 11/11 PASS**
7. **codegen unittest 130/130 PASS** (= Phase 1.A / 1.B / 1.C PC-1..PC-6β regression なし)
8. **commit 内容 prep** = 1 modified (`indra/llrender/llvkloader.cpp` 5 編集) + 1 new doc (本 handoff) + 新 file 0 + CMake 改変 0 + settings.xml 改変 0 (= PC-5 露出済) + Co-Authored-By 不在
9. **`feedback_no_scope_shrink` 遵守** = PC-6γ literal scope (PSC + cvar 読込) 完全実施、`createPipelineCache()` 改変 + init chain 1 行追加 + shutdown 経路追加は scope 外拡張ではなく PSC blob 投入 / persist の literal 要求への最小対応

---

## §8 引き継ぎ memory (= 既存活用、新規追加なし)

| memory | 適用観点 |
|---|---|
| `project_ayastorm_r41_vulkan_migration` | r41 milestone state (= §6) |
| `project_r41_phase1b_vulkan_host_gate` | GATE-B = `mUseUBO` runtime gate のみ、本 PC-6γ Vulkan 初期化層は無関係 |
| `project_ayastorm_r41_design_principles` | (1) Upstream OpenGL 取り込みやすさ維持 + (2) Core プロセス分散実現 → DI callback で algorithm 層 ↔ Vulkan 層分離 + storage 層は llvkloader 1 TU 内閉じ (= 原則 1 整合) |
| `feedback_ubo_migration_one_at_a_time` | PC-6 α..ζ 分割継続 (= PC-6α §2 確定)、1 sub = 1 algorithm 層 wire up |
| `feedback_handoff_minimal_pre_req_read` | §1.1 必読 3 件 + §1.2 pinpoint reference 別記、全件読み禁止 |
| `feedback_self_verify_before_handoff` | §7 9 観点 self-verify 全 ✅ |
| `feedback_build_only_verified` | llrender build PASS + TUT 13/13 + 10/10 + 11/11 + codegen 130/130 で literal 検証取得 |
| `feedback_no_scope_shrink` | PC-6γ literal scope 完全実施 (= §9 観点 9) |
| `feedback_doubt_self_first` | init chain ordering ambiguity (= storage は createPipelineCache() より先) 発見で停止 + 3 案評価 + (1) 採用根拠 3 件 明文化 |
| `feedback_proactive_handoff` | PC-6δ 引継 marker 本 handoff で能動 handoff |
| `feedback_release_branch_workflow` | feature branch (`feature/ayastorm-r41-gl-removal`) 上で work |
| `feedback_no_auto_commit` | AYA 「commit してください」literal 受領後 commit |
| `feedback_no_claude_coauthor` | Co-Authored-By 行不在 |
| `feedback_design_phase_no_code_write` | 本 PC-6γ は実装 phase (= design phase 不該当)、ただし scope 厳守 (= PSC wire up + cvar 読込のみ、5 cadence / SAMPLER skip は PC-6δ 以降) で精神準拠 |

---

## §9 commit 段取り = PC-6γ 単独 commit

git status 想定:
```
M indra/llrender/llvkloader.cpp                                                 (PC-6γ 5 編集)
?? docs/.../handoff-substep-...-phase1-c-pc-6-gamma.md                          (本 handoff doc)
```

**PC-6γ commit 内容** (= AYA 「commit してください」literal 受領後実行):
- `indra/llrender/llvkloader.cpp` (modified、5 編集)
- `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-6-gamma.md` (本 doc)

新 file / CMake / settings.xml / tests/ への touch 一切なし (= scope 最小、`feedback_tests_dir_never_commit` 自然遵守)。

---

## §10 次 session 着手 1 line

**PC-6δ 着手** = 5 cadence (per-frame / per-pass / per-asset / per-draw / per-skin) flush 関数 update site 5 種で test UBO 空 dummy 書込 PASS、ring buffer `allocate()` / `beginFrame()` 経路通電。具体的には `indra/llrender/llglslshader.cpp` の `flushFrameUbos()` / `flushPassUbos()` / `flushAssetUbos()` / `flushDrawUbos()` / `flushSkinUbos()` 5 種 (= design 06b §4 該当) で sDrawUboRingBufferMgr->`beginFrame()` + `allocate(size_bytes)` 呼出経路を立ち上げ、AllocateResult.offset を `vkCmdBindDescriptorSets(..., pDynamicOffsets=...)` 投入経路に渡す配線 (= design 07 §7.4)。本 PC-6δ では空 dummy 書込で経路通電のみ、実 PerFrame_/PerPass_/PerAsset_/PerDraw_/PerSkin_ UBO content 書込は PC-6ε (= block-level test bring-up 本格置換) 以降。

---

## §11 次 session bootstrap (= AYA から次 session に投げる短い要約 candidate)

```
前 session で PC-6γ = (PSC) LLPipelineCacheStorage × Vulkan device 実 wire up + cvar AYAPipelineCacheSizeMB 読込 hookup complete (= indra/llrender/llvkloader.cpp 5 編集 = include 追加 + sPipelineCacheStorageMgr unique_ptr + createPipelineCacheStorage() helper (cvar lookup + path 解決 + ifstream/ofstream lambda + ctor + initialize) + createPipelineCache() の pInitialData blob 投入経路化 + init chain で createPipelineCache() より前に呼出 + shutdown で vkGetPipelineCacheData 2-pass → updateBlob → persistToDisk → vkDestroyPipelineCache 配線、起動時 file load + (e1) 64 MB cap 超過 blob 破棄、llrender build PASS + INTEGRATION_TEST_llpipelinecachestorage 13/13 PASS + 10/10 + 11/11 + codegen 130/130 PASS、新 file 0 + CMake 改変 0 + settings.xml 改変 0 (= PC-5 で既露出済))。PC-6 strict 線形 α..ζ 6 sub-task 中 α + β + γ 完結、本 session 着手 = PC-6δ = 5 cadence (per-frame / per-pass / per-asset / per-draw / per-skin) flush 関数 update site 5 種で test UBO 空 dummy 書込 PASS、ring buffer allocate() / beginFrame() 経路通電。

必読 3 件:
- docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-6-gamma.md (全文)
- docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-6-beta.md (§3 PC-6β 実施内容 + §10 次 session 着手 1 line)
- docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md (§4 flush 関数 5 種 + §4.3 triple-buffering U1=3 + §5.3 L1+L2 default)

PC-6δ から進めてください。
```
