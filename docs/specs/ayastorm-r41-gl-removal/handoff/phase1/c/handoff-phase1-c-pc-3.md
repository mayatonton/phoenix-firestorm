# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-3 complete** marker

**作成日**: 2026-06-04
**前 commit chain** (= 直前 handoff 起案):
- `e3f24f7a2a` = (Y) Phase 1.C prep handoff doc 起案
- `b8a37d078b` = (Y) Phase 1.C prep PC-0 (Q1) AYA 確定値 4 件 record
- `9ed4cca802` = Phase 1.C **PC-1 complete** = `Global_ReflectionProbes` shell blueprint codegen emit
- `c7f512d654` = Phase 1.C **PC-2 complete** = test UBO shell C++ 接続 = block-level test bring-up (= (c) 採用)

**本 handoff doc 目的**: **Phase 1.C PC-3 complete marker**。(W2) `sAssetUboPool` 起動時 prealloc N=64 + grow chunk 64 algorithm 実装 + TUT unittest 10/10 PASS + codegen unittest 130/130 PASS 完結後の引継。AYA 採用 案 (α) = `llcommon` 単体配置 + `LL_ADD_INTEGRATION_TEST` framework 経由 unittest。

---

## §0 state 一行 summary

PC-3 = **`sAssetUboPool` grow algorithm + unittest complete**:

- 新 class `LLAssetUboPool` 起案 (= `indra/llcommon/llassetubopool.h` + `llassetubopool.cpp`、Vulkan device 非依存 bookkeeping algorithm)
- 起動時 prealloc N=64 = 1 物理 pool 確保、全 pool 枯渇 detect 時 = grow chunk 64 で新 1 物理 pool 追加 (= `sAssetUboPools[]` 配列化 grow)
- `kPreallocAssetCount = 64` / `kGrowChunkAssetCount = 64` (= `design/07-vulkan-api-state.md` §6.1/§6.3/§7.2 確定値)
- 実 `VkDescriptorPool` factory injection は PC-6 (= llrender / llvkloader 側 wire up) に持越、本 PC-3 段は dependency injection (`PoolFactory` / `PoolDestroyer` callback) で Vulkan-decoupled algorithm 単体検証
- TUT unittest 10/10 PASS = `INTEGRATION_TEST_llassetubopool` (= `indra/llcommon/tests/llassetubopool_test.cpp` 10 ケース、`LL_ADD_INTEGRATION_TEST` framework 経由)
- codegen unittest 130/130 PASS (= Phase 1.A / 1.B / 1.C PC-1..PC-2 regression なし)
- `indra/llcommon/CMakeLists.txt` 3 line 編集 (= source + header + `LL_ADD_INTEGRATION_TEST` 行)

---

## §1 pre-requisite 最小読み (= 次 session 着手時参照、`feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session = PC-4 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc | 全文 | PC-3 完結状態 + PC-4 着手起点 + (α) 案採用根拠 + W2 default 値 source doc 特定経緯 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-prep.md` | §3.1 PC-4 row + §5.1 (R1)/(RB) 持越項目 + §3.2 strict 線形 | PC-4 = (R1) ring buffer 4 MB 起動 / (RB) 16 MB grow 上限 + cvar `AYARingBufferSizeMB` 実装 scope |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md` | §7.1/§7.2 ring buffer 仕様 | PC-4 (R1)/(RB) default 値 + grow 上限 + cvar 名 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `indra/llcommon/llassetubopool.h` | 全文 (= 95 line、class declaration + DI callback type alias + Exit literal compliance comment) |
| `indra/llcommon/llassetubopool.cpp` | 全文 (= 137 line、`growOnce` / `acquire` / `resetAllSlots` / `shutdown` 実装) |
| `indra/llcommon/tests/llassetubopool_test.cpp` | 全文 (= 203 line、10 TUT case = default 値 / prealloc / 64 capacity / 65th grow / 多重 grow / reset / shutdown / factory failure / custom params / handle 単調増加) |
| `indra/llcommon/CMakeLists.txt` | `llcommon_SOURCE_FILES` 内 `llassetubopool.cpp` 行 + `llcommon_HEADER_FILES` 内 `llassetubopool.h` 行 + `LL_ADD_INTEGRATION_TEST(llassetubopool ...)` 行 |
| `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md:§6.1` | `sAssetUboPool` (set=3) `maxSets = N × 3 = 192 (N=64)` |
| `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md:§6.3` | 起動時 prealloc N=64 + grow chunk 64 + `sAssetUboPools[]` 配列化方針 |
| `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md:§7.2` | ring buffer 4 MB 起動 / 16 MB grow 上限 (= PC-4 next 起点) |
| `indra/llrender/llvkloader.h` / `.cpp` | `sSharedDescriptorPool` のみ実装済 (= 4 cadence pool 未実装、PC-6 で wire up 起点) |

---

## §2 PC-3 着手前発見 = scope ambiguity 解消経緯

### §2.1 prep doc 「07 §12」literal 参照 = stale reference

prep doc §5.1 (W2) 行で `07-descriptor-renderpass.md §12` を参照していたが、実 file は §1-§6 までしか存在せず (W2) default 値 (N=64 / grow 64) を含む §12 は drift していた。

### §2.2 真 source doc 特定 = `design/07-vulkan-api-state.md`

`grep -rn "N=64\|prealloc" docs/specs/ayastorm-r41-gl-removal/design/` で確定:

| location | literal |
|---|---|
| `design/07-vulkan-api-state.md:§6.1` | `sAssetUboPool` (set=3) `maxSets = N × 3 = 192 (N=64)` |
| `design/07-vulkan-api-state.md:§6.3` | 「起動時 prealloc 1 物理 pool (capacity = N=64 asset) + 全 pool 枯渇 detect 時 = 新 1 物理 pool 追加」+ 「配列化 `sAssetUboPools[]` で grow 履歴保持、cleanup は cadence 単位 (= shutdown 時 reverse 順 destroy)」 |
| `design/07-vulkan-api-state.md:§7.2` | ring buffer 4 MB 起動 / 16 MB grow 上限 (= PC-4 起点) |

本 handoff doc では `design/07-vulkan-api-state.md` を真 source として明示記録。

### §2.3 「unittest PASS」literal scope 確認 = AYA 判断仰ぎ

llrender には test infrastructure 不在、4 cadence pool (set=3 layout 含む) は llvkloader にも未実装。「per-asset cadence pool 起動時 N=64 alloc + dynamic grow chunk 64 動作 unittest PASS」literal を満たす配置候補 3 案を提示:

| 案 | 配置 | unittest framework | 副作用 |
|---|---|---|---|
| **(α)** ✅ | `llcommon` 単体配置 = Vulkan device 非依存 bookkeeping algorithm class + dependency injection (factory/destroyer callback) | `LL_ADD_INTEGRATION_TEST` (= llcommon 既設 TUT framework) | 純 algorithm のみ、実 `VkDescriptorPool` factory は PC-6 で wire up |
| (β) | `llrender` 配置 + Vulkan device mock 起案 | 新規 mock 起案要 | Vulkan symbol 依存、unittest 起動 boot path 長い |
| (γ) | spec doc 章だけ追加 + 実装は PC-6 統合 | unittest 不要 | literal「unittest PASS」充足せず |

### §2.4 AYA 確定 literal

「Claude 推奨: (α)」(= 2026-06-04 session 受領) → **(α) 採用**。

### §2.5 (α) 採用根拠 3 件

1. **literal scope 完全充足**: 「unittest PASS」literal を最小 scope で実現 (= 純 algorithm 単体検証、Vulkan symbol 依存ゼロ)。
2. **責務分離**: pool grow 機構は Vulkan device API と独立した bookkeeping problem、algorithm 層と device 層を切り分けると PC-6 wire up 時に algorithm 部分は touch 不要 = regression risk 最小化。
3. **dependency injection で testability**: `PoolFactory` / `PoolDestroyer` callback で Vulkan device 無し環境で grow algorithm 単独検証可、edge case (factory failure 等) も再現容易。

---

## §3 PC-3 実施内容

### §3.1 新 class `LLAssetUboPool` 起案

**header** (= `indra/llcommon/llassetubopool.h`、95 line):

```cpp
class LLAssetUboPool
{
public:
    using PoolHandle    = std::uint64_t;
    using PoolFactory   = std::function<PoolHandle()>;
    using PoolDestroyer = std::function<void(PoolHandle)>;

    static constexpr std::uint32_t kPreallocAssetCount  = 64;
    static constexpr std::uint32_t kGrowChunkAssetCount = 64;

    LLAssetUboPool(PoolFactory factory, PoolDestroyer destroyer,
                   std::uint32_t prealloc_count = kPreallocAssetCount,
                   std::uint32_t grow_chunk     = kGrowChunkAssetCount);
    ~LLAssetUboPool();

    bool initialize();
    void shutdown();

    struct AcquireResult { PoolHandle pool=0; std::uint32_t slot_in_pool=0; bool success=false; };
    AcquireResult acquire();

    void resetAllSlots();

    std::uint32_t getPoolCount()          const noexcept;
    std::uint32_t getTotalAssetCapacity() const noexcept;
    std::uint32_t getUsedAssetCount()     const noexcept;
    std::uint32_t getPreallocCount()      const noexcept;
    std::uint32_t getGrowChunkCount()     const noexcept;
    bool          isInitialized()         const noexcept;
```

- copy-disabled (`= delete`)
- 内部 state: `mPools` / `mPoolCapacities` / `mUsedPerPool` の 3 並列 `std::vector` + `mInitialized` flag
- header file-level doc comment で `design/07-vulkan-api-state.md` §6.1/§6.3/§7.2 確定値根拠 + (α) 採用根拠を明示

**implementation** (= `indra/llcommon/llassetubopool.cpp`、137 line):

| method | 動作 |
|---|---|
| ctor | `prealloc_count == 0` ? `kPreallocAssetCount` : prealloc_count 等、0 渡された時 default fallback |
| `initialize()` | `growOnce(mPreallocCount)` 1 回呼出 → `mInitialized = true`、既 init 済時は no-op |
| `shutdown()` | `mPools.rbegin()` → `rend()` で `mDestroyer()` reverse 順呼出 + 3 vector clear + flag reset |
| `growOnce(capacity)` | `mFactory()` 呼出、null/0 handle 時 false、成功時 3 vector に `push_back` |
| `acquire()` | 線形 search で `mUsedPerPool[i] < mPoolCapacities[i]` 探す、見つからない時 `growOnce(mGrowChunkCount)` で新 pool 追加、新 pool slot 0 を割当 |
| `resetAllSlots()` | `mUsedPerPool` 全要素 0 set (= pool 配列 / capacity 配列は維持) |

### §3.2 TUT unittest 10 ケース起案

**file**: `indra/llcommon/tests/llassetubopool_test.cpp` (= 203 line、`tut::test_group<asset_ubo_pool_data>` 使用)

| # | 検証内容 | ensure 条件 |
|---|---|---|
| 1 | default 値 (= constructor) | `kPreallocAssetCount = 64u` + `kGrowChunkAssetCount = 64u` + 0 pool + !initialized |
| 2 | `initialize()` 後 | 1 pool + capacity 64 + 0 used + initialized flag |
| 3 | 64 連続 acquire | 1 pool 維持 + slot 昇順 + 64 used |
| 4 | 65th acquire = grow | 2 pools + capacity 128 + 新 pool slot 0 + 65 used |
| 5 | 200 acquire = 多重 grow | 4 pools + capacity 256 + 200 used |
| 6 | `resetAllSlots()` | pool 数維持 + 0 used + 再 acquire OK |
| 7 | `shutdown()` | 0 pool + destroyer 2 回呼出 + !initialized + 再 init OK |
| 8 | factory failure (= 0 handle return) | initialize fail + acquire fail |
| 9 | custom params (= 16 / 8) | 16 prealloc + 8 grow + 17th で 2 pools capacity 24 |
| 10 | handle 単調増加 | 3 pool handle = 1 / 2 / 3 (= factory `next_handle++`) |

`asset_ubo_pool_data` struct = test fixture (= `next_handle` 連番 + `destroyed` log)、`makeIncrementingFactory` / `makeRecordingDestroyer` で callback 注入。

### §3.3 `CMakeLists.txt` 3 line 編集

**file**: `indra/llcommon/CMakeLists.txt`

| 編集 | 位置 | 内容 |
|---|---|---|
| 追加 1 | `llcommon_SOURCE_FILES` `llassettype.cpp` 直後 | `llassetubopool.cpp` |
| 追加 2 | `llcommon_HEADER_FILES` `llassettype.h` 直後 | `llassetubopool.h` |
| 追加 3 | `LL_ADD_INTEGRATION_TEST(bitpack ...)` 直後 | `LL_ADD_INTEGRATION_TEST(llassetubopool "" "${test_libs}")` |

### §3.4 build verify = `LL_TESTS=ON` 再 configure + INTEGRATION_TEST 実走

1. **CMake reconfigure** = `cmake -DLL_TESTS=ON ...` → Configuring done (2.7s) + Generating done (0.2s) PASS
2. **`make -j4 INTEGRATION_TEST_llassetubopool`** 実走:
   - Building llcommon_tests / llcommon target PASS
   - Building `tests/llassetubopool_test.cpp.o` PASS
   - Building `__/test/test.cpp.o` + `__/test/lltut.cpp.o` PASS
   - Linking `INTEGRATION_TEST_llassetubopool` executable PASS
   - **POST_BUILD auto-run**:
     ```
     Unit test group_started name=LLAssetUboPool
     Unit test group_completed name=LLAssetUboPool
         Total Tests:	10
         Passed Tests:	10	YAY!! \o/
     ```
3. ERROR 0 件 / WARNING 0 件 (= llassetubopool 関連、既知 volk.c `-Wno-reorder` 除く)

### §3.5 codegen unittest = 130/130 PASS

`python3 -m unittest discover -s scripts/ubo_codegen/tests` = `Ran 130 tests in 0.062s` `OK` (= Phase 1.A / 1.B / 1.C PC-1..PC-2 regression なし)。

---

## §4 PC-3 Exit Criteria 充足 record

prep doc §3.1 PC-3 row Exit literal = 「per-asset cadence pool 起動時 N=64 alloc + dynamic grow chunk 64 動作 unittest PASS」。

| Exit 項目 | 充足 |
|---|---|
| (i) per-asset cadence pool 起動時 N=64 alloc | ✅ TUT test<2> = `getPoolCount() == 1u` + `getTotalAssetCapacity() == 64u` |
| (ii) dynamic grow chunk 64 動作 | ✅ TUT test<4> = 65th acquire で `getPoolCount() == 2u` + `getTotalAssetCapacity() == 128u` |
| (iii) unittest PASS | ✅ TUT 10/10 PASS (= `INTEGRATION_TEST_llassetubopool` POST_BUILD auto-run) |

---

## §5 残 strict 線形 (= prep doc §3.2 整合)

```
PC-3 ✅ (本 commit) → PC-4 (R1/RB ring buffer 4 MB/16 MB + cvar AYARingBufferSizeMB)
                  → PC-5 (PSC PSO cache 64 MB)
                  → PC-6 (5 cadence update site = block-level bring-up と本格置換、SAMPLER skip 正攻法対応 候補 timing、(W2) 実 VkDescriptorPool factory injection wire up timing)
                  → PC-7 (vkCmdBindDescriptorSets 通電)
                  → PC-8 (build verify)
                  → PC-N (complete marker)
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
- **PC-3 ✅ 本 commit** (= `sAssetUboPool` grow algorithm + unittest 10/10)
- PC-4..PC-N ⏳ 次 session

---

## §7 self-verify 9 観点 全 ✅

1. **PC-3 Exit Criteria 3 項全充足** = §4 record
2. **W2 default 値 source doc 特定** = `design/07-vulkan-api-state.md` §6.1/§6.3/§7.2 (prep doc 「07 §12」literal は stale、本 handoff §2.1 record)
3. **(α) 採用根拠 3 件 record** = §2.5 = literal scope 完全充足 + 責務分離 + dependency injection で testability
4. **GATE-B 整合** = `mUseUBO` runtime gate に依存しない algorithm 層 (= 本 PC-3 は pure bookkeeping、`#ifdef LL_VULKAN_GLSL` C++ 不使用、`mUseUBO` も touch せず)
5. **MUSEUBO-A 整合** = 本 PC-3 は callsite が無く ((W2) wire up は PC-6)、既存 OpenGL 挙動 100% 維持
6. **llcommon target build PASS** + **INTEGRATION_TEST_llassetubopool build + POST_BUILD auto-run 10/10 PASS**
7. **codegen unittest 130/130 PASS** (= Phase 1.A / 1.B / 1.C PC-1..PC-2 regression なし)
8. **commit 内容 prep** = 2 new file (llassetubopool.h + .cpp) + 1 new test file (tests/llassetubopool_test.cpp、`git add -f` 要 = `.gitignore` で `tests/` ignored、`indra/llcommon/tests/` の既存 tracked test と同様の扱い) + 1 modified (CMakeLists.txt) + 1 new doc (本 handoff)
9. **`feedback_no_scope_shrink` 遵守** = literal scope 「per-asset cadence pool 起動時 N=64 alloc + dynamic grow chunk 64 動作 unittest PASS」を縮小せず、(α) 採用で algorithm + 10 unittest case 全実装

---

## §8 引き継ぎ memory (= 既存活用、新規追加なし、`feedback_*` 系のみ参照)

| memory | 適用観点 |
|---|---|
| `project_ayastorm_r41_vulkan_migration` | r41 milestone state (= 本 handoff §6) |
| `project_r41_phase1b_vulkan_host_gate` | GATE-B = `mUseUBO` runtime gate のみ、本 PC-3 algorithm 層は無関係 |
| `project_ayastorm_r41_design_principles` | (1) Upstream OpenGL 取り込みやすさ維持 + (2) Core プロセス分散実現 → dependency injection で algorithm 層分離 (= 原則 2 整合) |
| `feedback_ubo_migration_one_at_a_time` | 本 PC-3 は (W2) 持越項目 1 件のみ、cold launch 検証は PC-6 wire up 時 |
| `feedback_handoff_minimal_pre_req_read` | §1.1 必読 3 件 + §1.2 pinpoint reference 別記、全件読み禁止 |
| `feedback_self_verify_before_handoff` | §7 9 観点 self-verify 全 ✅ |
| `feedback_build_only_verified` | TUT 10/10 + codegen 130/130 で literal 検証取得、机上推論せず |
| `feedback_no_scope_shrink` | (α) 採用は scope 縮小ではない (= §2.3 3 案明示後 AYA 確定で literal scope 充足) |
| `feedback_doubt_self_first` | (W2) default 値 source doc drift 発見で AYA 確認 + grep 検証実施 |
| `feedback_proactive_handoff` | PC-4 引継 marker 本 handoff で能動 handoff |
| `feedback_release_branch_workflow` | feature branch (`feature/ayastorm-r41-gl-removal`) 上で work、release branch 直 commit せず |
| `feedback_no_auto_commit` | AYA 「commit してください」literal 受領後 commit |
| `feedback_no_claude_coauthor` | Co-Authored-By 行不在 |
| `feedback_tests_dir_never_commit` | top-level `tests/` (= AYA security info) には触れない、`indra/llcommon/tests/` は viewer source dir で別物 (= 既存 tracked `*_test.cpp` 存在で確認済) |

---

## §9 次 session 着手 1 line

**PC-4 着手** = (R1) ring buffer 4 MB 起動 + (RB) 16 MB grow 上限 + cvar `AYARingBufferSizeMB` 実装 = `design/07-vulkan-api-state.md` §7.2 default 値 record (= ring buffer 4 MB start / 16 MB grow cap)。

---

## §10 次 session bootstrap (= AYA から次 session に投げる短い要約 candidate)

```
前 session で PC-3 = (W2) sAssetUboPool grow algorithm 実装 complete (= llcommon 配置 + LL_ADD_INTEGRATION_TEST framework + TUT 10/10 PASS)。本 session 着手 = PC-4 = (R1) ring buffer 4 MB 起動 / (RB) 16 MB grow 上限 + cvar AYARingBufferSizeMB 実装。

必読 3 件:
- docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-3.md (全文)
- docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-prep.md (§3.1 PC-4 row + §5.1 (R1)/(RB) + §3.2 strict 線形)
- docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md (§7.1/§7.2 ring buffer 仕様)

PC-4 から進めてください。
```
