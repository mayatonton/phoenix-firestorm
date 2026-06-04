# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-5 complete** marker

**作成日**: 2026-06-04
**前 commit chain** (= 直前 handoff 起案):
- `9ed4cca802` = Phase 1.C **PC-1 complete** = `Global_ReflectionProbes` shell blueprint codegen emit
- `c7f512d654` = Phase 1.C **PC-2 complete** = test UBO shell C++ 接続 = block-level test bring-up (= (c) 採用)
- `c31998c49f` = Phase 1.C **PC-3 complete** = `sAssetUboPool` grow algorithm + TUT 10/10 PASS (= (α) 採用)
- `912863bf81` = Phase 1.C **PC-4 complete** = `LLUboRingBuffer` ring buffer 4 MB / 16 MB grow algorithm + cvar `AYARingBufferSizeMB` 露出 + TUT 11/11 PASS (= (α') 採用)

**本 handoff doc 目的**: **Phase 1.C PC-5 complete marker**。(PSC) PSO cache 64 MB disk persist 上限 enforcement algorithm 実装 + cvar `AYAPipelineCacheSizeMB` 露出 + TUT unittest 13/13 PASS + codegen unittest 130/130 PASS 完結後の引継。AYA 採用 案 (α'') = `llcommon` 単体配置 + DI callback (`FileReader` / `FileWriter`) + `LL_ADD_INTEGRATION_TEST` framework 経由 unittest + `settings.xml` cvar 露出 only (= 実 Vulkan device 配線 + `vkGetPipelineCacheData` / `VkPipelineCacheCreateInfo.pInitialData` 投入 は PC-6 持越)、PC-3 (α) / PC-4 (α') precedent pattern 踏襲。enforcement 戦略 = (e1) = load 時 size > max なら blob 破棄 + persist 時 size > max なら writer 不呼出 + false return で caller 判断、algorithm 層は bool query 提供のみ。

---

## §0 state 一行 summary

PC-5 = **`LLPipelineCacheStorage` PSO cache disk persist algorithm + cvar 露出 + unittest complete**:

- 新 class `LLPipelineCacheStorage` 起案 (= `indra/llcommon/llpipelinecachestorage.h` 117 line + `llpipelinecachestorage.cpp` 111 line、Vulkan device 非依存 bookkeeping algorithm)
- disk I/O は caller injected callback (= `FileReader` / `FileWriter`) 経由で Vulkan / std::filesystem decoupled (= PC-3 (α) / PC-4 (α') precedent DI pattern 踏襲)
- 上限 = `kDefaultMaxSizeMB = 64` (= `design/07-vulkan-api-state.md` §9.3 + §12 (PSC) 確定値)、disk path = `~/.ayastorm_x64/cache/pipeline_cache.bin` (= 同上)
- (e1) enforcement 戦略 = load 時 file size > 64 MB なら blob 破棄 = 0 cache から再生成 (= stale 大 blob 排除) + persist 時 blob size > 64 MB なら writer 不呼出 + `false` return で caller (PC-6 wire up timing) が skip / delete / truncate 戦略を決定
- algorithm 層は size 関連 bool query 提供のみ (= `isWithinLimit` / `getBlobSize` / `getBlobSizeMB` / `getMaxSizeBytes` / `getMaxSizeMB`)
- 実 `vkGetPipelineCacheData` (= `sPipelineCache` → blob 取出) + 起動時 file load → `VkPipelineCacheCreateInfo.pInitialData` 投入 / shutdown 時 save の wire up は PC-6 (= 5 cadence update site 着手 timing) で llrender / llvkloader 側で実装予定
- TUT unittest 13/13 PASS = `INTEGRATION_TEST_llpipelinecachestorage` (= `indra/llcommon/tests/llpipelinecachestorage_test.cpp` 13 ケース、`LL_ADD_INTEGRATION_TEST` framework 経由)
- codegen unittest 130/130 PASS (= Phase 1.A / 1.B / 1.C PC-1..PC-4 regression なし)
- `indra/llcommon/CMakeLists.txt` 3 line 編集 (= source + header + `LL_ADD_INTEGRATION_TEST` 行)
- `indra/newview/app_settings/settings.xml` 1 cvar 追加 = `AYAPipelineCacheSizeMB` (default 64 / Persist 1 / U32、debug settings 露出のみ、cvar 読込は PC-6 持越)

---

## §1 pre-requisite 最小読み (= 次 session 着手時参照、`feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session = PC-6 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc | 全文 | PC-5 完結状態 + PC-6 着手起点 + (α'') 案採用根拠 + (e1) enforcement 戦略 record + design 07 §9.3 / §12 PSC source 確定 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-prep.md` | §3.2 strict 線形 + §3.1 PC-6 row + §6 5 cadence update site 統合 wire up | PC-6 = 5 cadence 全経路 update site + 実 VkDescriptorPool / vmaCreateBuffer / pipeline cache factory injection wire up scope |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md` | §6 (descriptor pool) + §7 (ring buffer + dynamic offset) + §9 (PSO layout + cache) + §10 (reflection update sync) | PC-6 wire up 整合 + 5 cadence 全経路 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `indra/llcommon/llpipelinecachestorage.h` | 全文 (= 117 line、class declaration + DI callback type alias + design 07 §9.3 / §12 引用 comment + (e1) 戦略明示 + (α'') 採用根拠) |
| `indra/llcommon/llpipelinecachestorage.cpp` | 全文 (= 111 line、`initialize` (e1) load reject + `updateBlob` + `persistToDisk` (e1) cap skip 実装) |
| `indra/llcommon/tests/llpipelinecachestorage_test.cpp` | 全文 (= 250 line、13 TUT case = default 値 / init missing / init load OK / init size>max reject / updateBlob / isWithinLimit true / isWithinLimit false / persist OK / persist cap skip / writer fail / shutdown 再 init / reader nullptr / ctor max=0 fallback) |
| `indra/llcommon/CMakeLists.txt` | `llpipelinecachestorage.cpp` (= `llmutex.cpp` ↔ `llpointer.cpp` 間) / `llpipelinecachestorage.h` (= `llnametable.h` ↔ `llpointer.h` 間) / `LL_ADD_INTEGRATION_TEST(llpipelinecachestorage ...)` (= `lluboringbuffer` 直後) |
| `indra/newview/app_settings/settings.xml:10383-10405` | `AYAPipelineCacheSizeMB` cvar block (= 22 行、`<FS:AYAstorm r41 Phase 1.C PC-5>` 区切り内) |
| `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md:§9.3` | PSO cache 戦略 (= pipeline 1 layout 共有 = cache hit 率最大 + `pCachedData` 起動時 disk load + 終了時 save) |
| `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md:§12` | (PSC) 確定値 record (= disk path + 64 MB cap) + (RB) → (R1) rename 経緯 |
| `indra/llrender/llvkloader.h` / `.cpp` | (PSC) wire up 起点 (= PC-6 timing、`sPipelineCache` line 64 + `createPipelineCache()` line 756 が現在 `VkPipelineCacheCreateInfo{}` `pInitialData = nullptr` で in-memory only = 起動時 file load → `pInitialData` 投入 + shutdown 時 `vkGetPipelineCacheData` 取出 → `LLPipelineCacheStorage::updateBlob` → `persistToDisk` 配線予定) |
| `indra/llcommon/llassetubopool.h` / `.cpp` + `lluboringbuffer.h` / `.cpp` | PC-3 (α) / PC-4 (α') precedent (= 同 pattern algorithm 層 + DI callback、PC-6 wire up 時に 3 件揃って 5 cadence factory injection) |

---

## §2 PC-5 着手前発見 = scope ambiguity 解消経緯

### §2.1 prep doc + handoff §1.1 「design 07 §11 PSO cache 仕様」literal stale 発見

PC-4 handoff §1.1 必読 3 件目 = 「`design/07-vulkan-api-state.md` §11 PSO cache 仕様 (= PC-5 起点)」literal、prep doc §3.1 PC-5 row Exit literal = 「Vulkan pipeline 1 件作成時 PSC hit miss + 起動初回 cache 生成 + 64 MB 上限 enforcement」。

PC-5 着手時に `design/07-vulkan-api-state.md` を `^##` で section 列挙したところ、**§11 = chapter 04/06a/06b/06c/08/09 分担境界** で PSO cache 内容は持たず、**真 source は §9.3 PSO cache 戦略 + §12 (PSC) 確定値 record** と判明 (= PC-3 着手時の「07 §12」literal stale 発見、PC-4 着手時の (R1) → (RB) rename drift 発見、と同類のリテラル drift 第 3 例)。

### §2.2 「64 MB 上限 enforcement」literal scope = 4 解釈分岐

prep doc §3.1 PC-5 row + §5.3 (PSC) literal = 「上限 64 MB」「64 MB 上限 enforcement」。

| 解釈 | 内容 |
|---|---|
| (e1) | load 時 (`initialize`) file size > 64 MB なら blob 破棄 (= 0 cache 再生成) + persist 時 blob size > 64 MB なら writer 不呼出 + `false` return = caller (PC-6 wire up timing) が skip / delete / truncate 戦略決定。algorithm 層は bool query 提供のみ |
| (e2) | persist 時 blob size > 64 MB なら algorithm 層内で blob を 64 MB に truncate して書込 |
| (e3) | persist 時 blob size > 64 MB なら algorithm 層内で disk file を `unlink` (= 削除して 0 から再生成) |
| (e4) | persist 時 blob size > 64 MB なら algorithm 層内で writer skip + log のみ + file 不変 |

### §2.3 AYA 確定 literal

「OK」(= 2026-06-04 session 受領、(α'') = `llcommon` 単体配置 + DI callback + cvar 露出 only + (e1) enforcement) → **(α'') + (e1) 採用**。

### §2.4 (α'') + (e1) 採用根拠 3 件

1. **PC-3 (α) / PC-4 (α') precedent 整合**: 同形 = `llcommon` 単体 algorithm + DI callback + TUT framework、Vulkan device wire up は PC-6 一括で集約。Phase 1.C 全 PC 同じ pattern で scope 一貫性確保 (= `feedback_ubo_migration_one_at_a_time` 整合)。
2. **責務分離 = (e1) algorithm 層は bool query のみ、戦略決定は caller**: PSC 上限超過時の skip / delete / truncate 判断は viewer policy 領域 (= 起動時 = 既存 disk cache 信頼で破棄、shutdown 時 = blob を保持して次回 retry、等)。algorithm 層に戦略を埋め込むと PC-6 wire up 時の policy 変更が algorithm 層改変を要求 = touch 範囲拡大 = `feedback_design_phase_no_code_write` の精神に反する。bool query で caller 判断委譲が最小 surface。
3. **`feedback_no_scope_shrink` 整合**: prep literal 「64 MB 上限 enforcement」を「algorithm 層は何もしない」にせず、cap query + load reject + persist 拒否 で literal scope 充足。caller 判断分は PC-6 で対称的に wire up = 全体として「64 MB enforcement」literal 実現。

---

## §3 PC-5 実施内容

### §3.1 新 class `LLPipelineCacheStorage` 起案

**header** (= `indra/llcommon/llpipelinecachestorage.h`、117 line):

```cpp
class LLPipelineCacheStorage
{
public:
    using CacheBlob  = std::vector<std::uint8_t>;
    using FileReader = std::function<bool(const std::string& /*path*/, CacheBlob& /*out*/)>;
    using FileWriter = std::function<bool(const std::string& /*path*/, const CacheBlob& /*data*/)>;

    static constexpr std::uint32_t kDefaultMaxSizeMB = 64;  // design 07 §9.3 + §12 (PSC)

    LLPipelineCacheStorage(FileReader    reader,
                           FileWriter    writer,
                           std::string   file_path,
                           std::uint32_t max_size_mb = kDefaultMaxSizeMB);
    ~LLPipelineCacheStorage();

    bool initialize();        // file load + (e1) load reject
    void shutdown();          // state reset, no auto-persist
    bool updateBlob(CacheBlob new_blob);  // replace, always true
    bool persistToDisk();     // (e1) size > max → writer 不呼出 + false

    const CacheBlob&   getBlob()         const noexcept;
    std::size_t        getBlobSize()     const noexcept;
    std::uint32_t      getBlobSizeMB()   const noexcept;
    std::size_t        getMaxSizeBytes() const noexcept;
    std::uint32_t      getMaxSizeMB()    const noexcept;
    const std::string& getFilePath()     const noexcept;
    bool               isWithinLimit()   const noexcept;
    bool               isInitialized()   const noexcept;

private:
    FileReader    mReader;
    FileWriter    mWriter;
    std::string   mFilePath;
    std::uint32_t mMaxSizeMB;
    CacheBlob     mCacheBlob;
    bool          mInitialized = false;
};
```

- copy-disabled (`= delete`)
- header file-level doc comment で `design/07-vulkan-api-state.md` §9.3 + §12 (PSC) 確定値根拠 + 既存配線 (= `llvkloader.cpp` `sPipelineCache` line 64 + `createPipelineCache()` line 756 で `VkPipelineCacheCreateInfo{}` `pInitialData = nullptr` = in-memory only = 本 PC-5 で disk persist + 64 MB 上限 enforcement の algorithm 層を追加) + (α'') 採用根拠 + (e1) enforcement 戦略を明示
- DI callback (`FileReader` / `FileWriter`) で file system 未準備な unittest 環境でも単独検証可能

**implementation** (= `indra/llcommon/llpipelinecachestorage.cpp`、111 line):

| method | 動作 |
|---|---|
| ctor | `max_size_mb == 0` ? `kDefaultMaxSizeMB` : `max_size_mb` (= 0 渡された時 default fallback)、`mReader` / `mWriter` / `mFilePath` を `std::move` で初期化 |
| `initialize()` | (1) `mInitialized` 時 true return (= idempotent) (2) `!mReader` 時 false return (3) `mReader(mFilePath, loaded)` 呼出 (4) `ok && !loaded.empty()` 時 = `loaded.size() > getMaxSizeBytes()` なら `loaded.clear() + shrink_to_fit()` (= (e1) load reject)、それ以外 `mCacheBlob = std::move(loaded)` (5) `mInitialized = true` + true return (= file 不在 / read 失敗は許容 = 起動初回想定で empty blob start) |
| `shutdown()` | `mCacheBlob.clear()` + `shrink_to_fit()` + `mInitialized = false`、auto-persist しない (= caller 責任) |
| `updateBlob(new_blob)` | `mCacheBlob = std::move(new_blob)` + true return (= 無条件置換、size cap check は別途) |
| `persistToDisk()` | (1) `!mInitialized \|\| !mWriter` 時 false return (2) `mCacheBlob.size() > getMaxSizeBytes()` 時 false return (= (e1) cap skip、writer 不呼出) (3) `mWriter(mFilePath, mCacheBlob)` 呼出 + 結果 return |
| `getBlobSizeMB()` | `mCacheBlob.size() / (1024 * 1024)` を `std::uint32_t` cast |
| `getMaxSizeBytes()` | `mMaxSizeMB * 1024 * 1024` を `std::size_t` cast |
| `isWithinLimit()` | `mCacheBlob.size() <= getMaxSizeBytes()` |

### §3.2 TUT unittest 13 ケース起案

**file**: `indra/llcommon/tests/llpipelinecachestorage_test.cpp` (= 250 line、`tut::test_group<pipeline_cache_data>` 使用)

| # | 検証内容 | ensure 条件 |
|---|---|---|
| 1 | default 値 + 未 init state | `kDefaultMaxSizeMB == 64u` + `getMaxSizeMB() == 64u` + `getMaxSizeBytes() == 64 MB` + `getBlobSize() == 0` + `!isInitialized()` + `isWithinLimit()` (empty) + 0 reader/writer call |
| 2 | `initialize()` で file 不在 | `initialize() == true` (file 不在許容) + `isInitialized()` + `getBlobSize() == 0` + 1 reader call |
| 3 | `initialize()` で file 内容 load 成功 | files map に 1024B prefill → `getBlobSize() == 1024` + `getBlob()[0] == 0xAB` + `isWithinLimit()` |
| 4 | (e1) load reject = file size > max | max = 1 MB + 2 MB prefill → `initialize() == true` + `getBlobSize() == 0` (= 破棄) + `isWithinLimit()` |
| 5 | `updateBlob()` で blob 置換 | 512B blob update → `getBlobSize() == 512` + `getBlob()[0] == 0xCD` |
| 6 | `isWithinLimit() == true` (= ぴったり max) | max = 1 MB + 1 MB blob update → `isWithinLimit()` + `getBlobSizeMB() == 1u` |
| 7 | `isWithinLimit() == false` (= max + 1) | max = 1 MB + (1 MB + 1B) blob → `!isWithinLimit()` |
| 8 | `persistToDisk()` size OK → writer 呼出 + true | 256B blob persist → `persistToDisk() == true` + 1 writer call + writer path/size 整合 + `files[path].size() == 256` |
| 9 | (e1) `persistToDisk()` size > max → writer 不呼出 + false | max = 1 MB + (1 MB + 1B) blob → `persistToDisk() == false` + 0 writer call + `files` 不変 |
| 10 | `persistToDisk()` writer fail → false | writer 強制失敗 → `persistToDisk() == false` |
| 11 | `shutdown()` 後再 `initialize()` | initialize + updateBlob(128B) + shutdown → `!isInitialized()` + `getBlobSize() == 0` + 再 `initialize() == true` |
| 12 | reader nullptr → `initialize()` false | `initialize() == false` + `!isInitialized()` + `persistToDisk() == false` (uninit) |
| 13 | ctor `max_size_mb = 0` → 64 fallback | `getMaxSizeMB() == 64u` + `getMaxSizeBytes() == 64 MB` |

`pipeline_cache_data` struct = test fixture (= `files` map + `write_log` / `read_log` + `reader_force_fail` / `writer_force_fail` + `makeReader()` / `makeWriter()`)、PC-3 (α) / PC-4 (α') pattern 踏襲。

### §3.3 `CMakeLists.txt` 3 line 編集

**file**: `indra/llcommon/CMakeLists.txt`

| 編集 | 位置 | 内容 |
|---|---|---|
| 追加 1 | line 72 | `llpipelinecachestorage.cpp` (= `llmutex.cpp` ↔ `llpointer.cpp` 間、alphabetical) |
| 追加 2 | line 199 | `llpipelinecachestorage.h` (= `llnametable.h` ↔ `llpointer.h` 間、alphabetical) |
| 追加 3 | line 346 | `LL_ADD_INTEGRATION_TEST(llpipelinecachestorage "" "${test_libs}")` (= `LL_ADD_INTEGRATION_TEST(lluboringbuffer ...)` 直後) |

### §3.4 `settings.xml` cvar 1 件追加

**file**: `indra/newview/app_settings/settings.xml`、line 10383-10405

```xml
<!-- <FS:AYAstorm r41 Phase 1.C PC-5> VkPipelineCache disk persist 上限 (MB)。
     design/07-vulkan-api-state.md §9.3 + §12 (PSC) 確定値 = ~/.ayastorm_x64/cache/pipeline_cache.bin、
     上限 64 MB。本 cvar は debug settings 露出のみ (= PC-5 (α'') = llcommon
     LLPipelineCacheStorage algorithm 単体 + settings.xml cvar 露出)。実 cvar 読込 + 起動時 file load →
     VkPipelineCacheCreateInfo.pInitialData 投入 + shutdown 時 vkGetPipelineCacheData + 64 MB 上限超過
     時の戦略 (skip / delete / truncate) caller 決定は PC-6 (= 5 cadence update site 着手 timing) で
     llrender / llvkloader 側 wire up 予定。algorithm 層 (= 本 PC-5) は load 時 size > max なら blob
     破棄 (= (e1) load reject)、persistToDisk 時 size > max なら writer 不呼出 + false return で
     caller 判断、の bool query 提供のみ。変更には viewer 再起動が必要。 -->
<key>AYAPipelineCacheSizeMB</key>
<map>
  <key>Comment</key>
  <string>(r41 Phase 1.C) VkPipelineCache disk persist 上限 (MB)。default=64 (= design/07-vulkan-api-state.md §9.3 + §12 (PSC) 確定値、~/.ayastorm_x64/cache/pipeline_cache.bin)。load 時 file size > max なら blob 破棄 = 0 cache から再生成 ((e1) load reject)、persist 時 blob size > max なら writer 不呼出 + 戦略 (skip / delete / truncate) caller 判断 ((e1) cap skip)。実 wire up は PC-6 持越、本 cvar は debug settings 露出のみ。変更には viewer 再起動が必要</string>
  <key>Persist</key><integer>1</integer>
  <key>Type</key><string>U32</string>
  <key>Value</key><integer>64</integer>
</map>
<!-- </FS:AYAstorm> -->
```

挿入位置: `AYARingBufferSizeMB` block (PC-4) 直後 + `PluginInstancesLow` block 直前。`<FS:AYAstorm r41 Phase 1.C PC-5>` ... `</FS:AYAstorm>` で区切り = r41 PC-5 由来明示。

### §3.5 build verify = `LL_TESTS=ON` 再 configure + INTEGRATION_TEST 実走

1. **CMake reconfigure** = `cmake -DLL_TESTS=ON /home/ishikawa/work_firestorm/phoenix-firestorm/indra` → Configuring done (2.7s) + Generating done (0.2s) PASS + AyaUboCodegen 91 blueprint 反映
2. **`make -j4 INTEGRATION_TEST_llpipelinecachestorage`** 実走:
   - Building `llpipelinecachestorage.cpp.o` PASS
   - Linking llcommon static library PASS
   - Building `tests/llpipelinecachestorage_test.cpp.o` + `__/test/test.cpp.o` + `__/test/lltut.cpp.o` PASS
   - Linking `INTEGRATION_TEST_llpipelinecachestorage` executable PASS
   - **POST_BUILD auto-run**:
     ```
     Unit test group_started name=LLPipelineCacheStorage
     Unit test group_completed name=LLPipelineCacheStorage
         Total Tests:	13
         Passed Tests:	13	YAY!! \o/
     ```
3. ERROR 0 件 / WARNING 0 件 (= llpipelinecachestorage 関連、既知 volk.c `-Wno-reorder` 除く)

### §3.6 codegen unittest = 130/130 PASS

`python3 -m unittest discover -s scripts/ubo_codegen/tests` = `Ran 130 tests in 0.063s` `OK` (= Phase 1.A / 1.B / 1.C PC-1..PC-4 regression なし)。

---

## §4 PC-5 Exit Criteria 充足 record

prep doc §3.1 PC-5 row Exit literal = 「Vulkan pipeline 1 件作成時 PSC hit miss + 起動初回 cache 生成 + 64 MB 上限 enforcement」。

| Exit 項目 | 充足 |
|---|---|
| (i) Vulkan pipeline 1 件作成時 PSC hit miss (= 機構) | ✅ `LLPipelineCacheStorage::initialize()` で file load → blob 取得 (= PC-6 wire up 時に `VkPipelineCacheCreateInfo.pInitialData` 投入経路成立)、初回 file 不在時は empty blob start (= miss 想定) |
| (ii) 起動初回 cache 生成 | ✅ `LLPipelineCacheStorage::persistToDisk()` で blob → file 書込 (= PC-6 wire up 時に `vkGetPipelineCacheData` 結果を `updateBlob` → `persistToDisk` 配線で初回生成成立) |
| (iii) 64 MB 上限 enforcement | ✅ (e1) load 時 size > 64 MB → blob 破棄 (TUT test<4>) + persist 時 size > 64 MB → writer 不呼出 + false return (TUT test<9>) + `isWithinLimit()` query (TUT test<6>/<7>) |
| (iv) unittest PASS (= 暗黙、PC-3 / PC-4 precedent) | ✅ TUT 13/13 PASS (= `INTEGRATION_TEST_llpipelinecachestorage` POST_BUILD auto-run) |

(i) (ii) は algorithm 層完結 (= 機構成立)、実 Vulkan pipeline 配線 + `vkGetPipelineCacheData` 経由の hit/miss 検証は PC-6 wire up 時 = (α'') = `feedback_design_phase_no_code_write` 整合の scope 切分け (= PC-3 (α) / PC-4 (α') precedent)。

---

## §5 残 strict 線形 (= prep doc §3.2 整合)

```
PC-5 ✅ (本 commit) → PC-6 (5 cadence update site = block-level bring-up と本格置換、SAMPLER skip 正攻法対応 候補 timing、(W2) sAssetUboPool 実 VkDescriptorPool factory injection wire up + (RB) LLUboRingBuffer 実 vmaCreateBuffer factory injection wire up + (PSC) LLPipelineCacheStorage 実 file I/O + vkGetPipelineCacheData factory injection wire up + 3 cvar (AYAAssetUboPoolSize 該当無し → 該当 cvar は PC-6 で確定 / AYARingBufferSizeMB / AYAPipelineCacheSizeMB) 読込 hookup timing)
                  → PC-7 (vkCmdBindDescriptorSets 通電)
                  → PC-8 (build verify)
                  → PC-N (complete marker)
```

PC-6 は 3 algorithm 層 (= sAssetUboPool / LLUboRingBuffer / LLPipelineCacheStorage) 揃った状態で 5 cadence (= UB_GLOBAL_REFLECTION_PROBES 等) 全経路 update site に factory injection 一括 wire up = touch 範囲集約で diff review 容易 (= (α) / (α') / (α'') 共通根拠)。

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
- **PC-5 ✅ 本 commit** (= `LLPipelineCacheStorage` disk persist + 64 MB cap algorithm + cvar 露出 + unittest 13/13、(α'') + (e1) 採用)
- PC-6..PC-N ⏳ 次 session

---

## §7 self-verify 9 観点 全 ✅

1. **PC-5 Exit Criteria 4 項全充足** = §4 record (= (i) hit/miss 機構 + (ii) 初回生成機構 + (iii) 64 MB enforcement + (iv) unittest)
2. **PSC 64 MB / disk path source doc 整合** = `design/07-vulkan-api-state.md` §9.3 + §12 (PSC) 全引用、code `kDefaultMaxSizeMB = 64` constexpr 値完全一致、prep doc 「07 §11」literal stale record (§2.1)
3. **(α'') + (e1) 採用根拠 3 件 record** = §2.4 = PC-3 (α) / PC-4 (α') precedent 整合 + 責務分離 (algorithm 層は bool query のみ) + `feedback_no_scope_shrink` 整合
4. **GATE-B 整合** = `mUseUBO` runtime gate に依存しない algorithm 層 (= 本 PC-5 は pure bookkeeping、`#ifdef LL_VULKAN_GLSL` C++ 不使用、`mUseUBO` も touch せず)
5. **MUSEUBO-A 整合** = 本 PC-5 は callsite が無く ((PSC) wire up は PC-6)、既存 OpenGL 挙動 100% 維持
6. **llcommon target build PASS** + **INTEGRATION_TEST_llpipelinecachestorage build + POST_BUILD auto-run 13/13 PASS**
7. **codegen unittest 130/130 PASS** (= Phase 1.A / 1.B / 1.C PC-1..PC-4 regression なし)
8. **commit 内容 prep** = 2 new file (`llpipelinecachestorage.h` + `.cpp`) + 1 new test file (`tests/llpipelinecachestorage_test.cpp`、`git add -f` = `.gitignore` で `tests/` ignored、PC-3 / PC-4 同等扱い) + 2 modified (`CMakeLists.txt` + `settings.xml`) + 1 new doc (本 handoff) + Co-Authored-By 不在
9. **`feedback_no_scope_shrink` 遵守** = literal scope 「Vulkan pipeline 1 件作成時 PSC hit miss + 起動初回 cache 生成 + 64 MB 上限 enforcement」を縮小せず、(α'') + (e1) 採用で algorithm + 13 unittest case + cvar 露出 全実装

---

## §8 引き継ぎ memory (= 既存活用、新規追加なし、`feedback_*` 系のみ参照)

| memory | 適用観点 |
|---|---|
| `project_ayastorm_r41_vulkan_migration` | r41 milestone state (= 本 handoff §6) |
| `project_r41_phase1b_vulkan_host_gate` | GATE-B = `mUseUBO` runtime gate のみ、本 PC-5 algorithm 層は無関係 |
| `project_ayastorm_r41_design_principles` | (1) Upstream OpenGL 取り込みやすさ維持 + (2) Core プロセス分散実現 → dependency injection で algorithm 層分離 (= 原則 2 整合) |
| `feedback_ubo_migration_one_at_a_time` | 本 PC-5 は (PSC) 持越項目 1 件のみ、cold launch 検証は PC-6 wire up 時 |
| `feedback_handoff_minimal_pre_req_read` | §1.1 必読 3 件 + §1.2 pinpoint reference 別記、全件読み禁止 |
| `feedback_self_verify_before_handoff` | §7 9 観点 self-verify 全 ✅ |
| `feedback_build_only_verified` | TUT 13/13 + codegen 130/130 で literal 検証取得、机上推論せず |
| `feedback_no_scope_shrink` | (α'') + (e1) 採用は scope 縮小ではない (= §2.2 4 解釈明示後 AYA 確定で literal scope 充足) |
| `feedback_doubt_self_first` | 「07 §11」literal stale 発見で `^##` section 列挙 + grep 確認 + AYA 確認実施 (= PC-3 / PC-4 着手時と同類のリテラル drift 第 3 例) |
| `feedback_proactive_handoff` | PC-6 引継 marker 本 handoff で能動 handoff |
| `feedback_release_branch_workflow` | feature branch (`feature/ayastorm-r41-gl-removal`) 上で work、release branch 直 commit せず |
| `feedback_no_auto_commit` | AYA 「commit してください」literal 受領後 commit |
| `feedback_no_claude_coauthor` | Co-Authored-By 行不在 |
| `feedback_design_phase_no_code_write` | (α'') 採用で Vulkan device wire up は PC-6 まで含めず、本 PC-5 は algorithm + cvar 露出のみで scope 厳守 |

---

## §9 commit 段取り = PC-5 単独 commit (= PC-3 / PC-4 既 commit 済)

git status 現状:
```
M indra/llcommon/CMakeLists.txt          (PC-5 3 line 編集)
M indra/newview/app_settings/settings.xml (PC-5 cvar 1 件)
?? indra/llcommon/llpipelinecachestorage.cpp + .h  (PC-5 source、本 session 起案)
?? docs/.../handoff-substep-...-phase1-c-pc-5.md   (本 handoff doc)
   (tests/llpipelinecachestorage_test.cpp は .gitignore で hidden、git add -f で stage)
```

**PC-5 commit 内容** (= AYA 「commit してください」literal 受領後実行):
- `indra/llcommon/llpipelinecachestorage.h` (new)
- `indra/llcommon/llpipelinecachestorage.cpp` (new)
- `indra/llcommon/tests/llpipelinecachestorage_test.cpp` (new、`git add -f` = `.gitignore` `tests/` ignored、PC-3 / PC-4 同等扱い)
- `indra/llcommon/CMakeLists.txt` (3 line)
- `indra/newview/app_settings/settings.xml` (1 cvar block)
- `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-5.md` (本 doc)

`feedback_tests_dir_never_commit` 整合: top-level `tests/` (= AYA security info dir) には触れない、`indra/llcommon/tests/` は viewer source dir 配下で別物 (= 既存 tracked `*_test.cpp` 多数存在で確認済、PC-3 / PC-4 で確認済)。

---

## §10 次 session 着手 1 line

**PC-6 着手** = 5 cadence update site = (W2) sAssetUboPool 実 `VkDescriptorPool` factory injection wire up + (RB) LLUboRingBuffer 実 `vmaCreateBuffer` HOST_VISIBLE+MAPPED factory injection wire up + (PSC) LLPipelineCacheStorage 実 file I/O + `vkGetPipelineCacheData` factory injection wire up + cvar (`AYARingBufferSizeMB` / `AYAPipelineCacheSizeMB`) 読込 hookup + block-level test bring-up (= PC-2 (c) 採用) と本格置換 + SAMPLER skip 正攻法対応 候補 timing。

---

## §11 次 session bootstrap (= AYA から次 session に投げる短い要約 candidate)

```
前 session で PC-5 = (PSC) PSO cache 64 MB disk persist algorithm + cvar AYAPipelineCacheSizeMB 実装 complete (= llcommon 配置 + DI callback (FileReader/FileWriter) + LL_ADD_INTEGRATION_TEST framework + TUT 13/13 PASS + settings.xml cvar 露出 (α'') = debug settings 露出のみ + (e1) enforcement = load 時 size>max blob 破棄 + persist 時 size>max writer 不呼出 + caller 判断、algorithm 層は bool query のみ。実 wire up は PC-6 持越)。本 session 着手 = PC-6 = 5 cadence update site = 3 algorithm 層 (sAssetUboPool / LLUboRingBuffer / LLPipelineCacheStorage) 揃った状態で実 Vulkan device 配線 + cvar 読込 hookup 一括統合。

必読 3 件:
- docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-5.md (全文)
- docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-prep.md (§3.2 strict 線形 + §3.1 PC-6 row + §6 5 cadence update site 統合 wire up)
- docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md (§6 descriptor pool + §7 ring buffer + dynamic offset + §9 PSO layout + cache + §10 reflection update sync)

PC-6 から進めてください。
```
