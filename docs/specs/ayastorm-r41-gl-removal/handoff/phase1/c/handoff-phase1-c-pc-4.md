# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-4 complete** marker

**作成日**: 2026-06-04
**前 commit chain** (= 直前 handoff 起案):
- `e3f24f7a2a` = (Y) Phase 1.C prep handoff doc 起案
- `b8a37d078b` = (Y) Phase 1.C prep PC-0 (Q1) AYA 確定値 4 件 record
- `9ed4cca802` = Phase 1.C **PC-1 complete** = `Global_ReflectionProbes` shell blueprint codegen emit
- `c7f512d654` = Phase 1.C **PC-2 complete** = test UBO shell C++ 接続 = block-level test bring-up (= (c) 採用)
- **PC-3 commit pending** = `sAssetUboPool` grow algorithm + TUT 10/10 PASS (= 前 session で実装 + handoff doc 起案、未 commit、本 handoff §11 で同時 commit 段取り提示)

**本 handoff doc 目的**: **Phase 1.C PC-4 complete marker**。(R1)/(RB) ring buffer 4 MB 起動 + 16 MB grow 上限 algorithm 実装 + cvar `AYARingBufferSizeMB` 露出 + TUT unittest 11/11 PASS + codegen unittest 130/130 PASS 完結後の引継。AYA 採用 案 (α') = `llcommon` 単体配置 + `LL_ADD_INTEGRATION_TEST` framework 経由 unittest + `settings.xml` cvar 露出 only (= 実 Vulkan device 配線は PC-6 持越)、(α) PC-3 precedent pattern 踏襲。

---

## §0 state 一行 summary

PC-4 = **`LLUboRingBuffer` ring buffer algorithm + cvar 露出 + unittest complete**:

- 新 class `LLUboRingBuffer` 起案 (= `indra/llcommon/lluboringbuffer.h` 131 line + `lluboringbuffer.cpp` 220 line、Vulkan device 非依存 bookkeeping algorithm)
- 起動時 prealloc = 4 MB 単一 ring buffer、chunk 数 = `kFramesInFlight = 3` で 1.33 MB / chunk
- chunk overflow detect 時 = N-1 (= GPU 読込中) hazard 判定で grow trigger or safe wrap (= `kFramesInFlight - 1 = 2` chunk まで safe)
- grow chain = double-and-cap (= 4 → 8 → 16 MB cap)、上限到達後の alloc fail は `success=false` return で caller 側 handle
- `kInitialSizeMB = 4` / `kMaxSizeMB = 16` / `kFramesInFlight = 3` / `kDefaultAlignment = 256` (= `design/07-vulkan-api-state.md` §7.2/§7.3/§7.5/§8.4 確定値)
- 実 `VkBuffer` factory injection (= VMA `vmaCreateBuffer` HOST_VISIBLE+MAPPED) は PC-6 (= llrender / llvkloader 側 wire up) に持越、本 PC-4 段は dependency injection (`BufferAllocator` / `BufferDestroyer` callback) で Vulkan-decoupled algorithm 単体検証
- TUT unittest 11/11 PASS = `INTEGRATION_TEST_lluboringbuffer` (= `indra/llcommon/tests/lluboringbuffer_test.cpp` 11 ケース、`LL_ADD_INTEGRATION_TEST` framework 経由)
- codegen unittest 130/130 PASS (= Phase 1.A / 1.B / 1.C PC-1..PC-3 regression なし)
- `indra/llcommon/CMakeLists.txt` 3 line 編集 (= source + header + `LL_ADD_INTEGRATION_TEST` 行)
- `indra/newview/app_settings/settings.xml` 1 cvar 追加 = `AYARingBufferSizeMB` (default 4 / Persist 1 / U32、debug settings 露出のみ、cvar 読込は PC-6 持越)

---

## §1 pre-requisite 最小読み (= 次 session 着手時参照、`feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session = PC-5 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc | 全文 | PC-4 完結状態 + PC-5 着手起点 + (α') 案採用根拠 + ring buffer 4 MB / 16 MB source doc 確定 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-prep.md` | §3.1 PC-5 row + §5.1 (PSC) 持越項目 + §3.2 strict 線形 | PC-5 = (PSC) PSO cache 64 MB 実装 scope |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md` | §11 PSO cache 仕様 (= PC-5 起点) | PC-5 (PSC) default 値 + cache 構造 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `indra/llcommon/lluboringbuffer.h` | 全文 (= 131 line、class declaration + DI callback type alias + design 07 §7.2/§7.3/§7.5/§8.4 引用 comment) |
| `indra/llcommon/lluboringbuffer.cpp` | 全文 (= 220 line、`allocate` chunk wrap + grow / `tryGrow` / `beginFrame` 実装) |
| `indra/llcommon/tests/lluboringbuffer_test.cpp` | 全文 (= 306 line、11 TUT case = default 値 / init / 小 alloc + align / 非 default align / chunk overflow wrap / 3 chunk hazard grow / 多重 grow / max cap / factory failure / shutdown 再 init / `beginFrame` 進行) |
| `indra/llcommon/CMakeLists.txt` | line 103 `lluboringbuffer.cpp` / line 245 `lluboringbuffer.h` / line 343 `LL_ADD_INTEGRATION_TEST(lluboringbuffer ...)` |
| `indra/newview/app_settings/settings.xml:10364-10381` | `AYARingBufferSizeMB` cvar block (= 17 行、`<FS:AYAstorm r41 Phase 1.C PC-4>` 区切り内) |
| `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md:§7.2` | ring buffer 容量算定 (= 5000 draw 通常 sim 3.84 MB → 4 MB / 20000 draw 過密 sim 15.36 MB → 16 MB) + cvar `AYARingBufferSizeMB` |
| `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md:§7.3` | offset alignment = `minUniformBufferOffsetAlignment` (Vulkan 1.3 spec 最大 256 / AMD/NVIDIA 典型 64) |
| `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md:§7.5` | chunk 構造 (= 3 段 chunk / wrap / grow trigger) |
| `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md:§8.4` | `FRAMES_IN_FLIGHT = 3` triple-buffering + `beginFrame()` 単一進行点 |
| `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md:§12` | (R1) → (RB) ring buffer prefix rename 経緯 |
| `indra/llrender/llvkloader.h` / `.cpp` | (R1)/(RB) wire up 起点 (= PC-6 timing、`vmaCreateBuffer` HOST_VISIBLE+MAPPED 配線予定) |

---

## §2 PC-4 着手前発見 = scope ambiguity 解消経緯

### §2.1 prep doc 「cvar 反映」literal scope = 3 解釈分岐

prep doc §3.1 PC-4 row Exit literal = 「per-frame cadence ring buffer 4 MB alloc + cvar 反映 + 16 MB 上限 enforcement」。`cvar 反映` literal に 3 解釈分岐:

| 解釈 | 内容 |
|---|---|
| (α') | `settings.xml` cvar 露出 only + algorithm 内部 default 値で動作確認、cvar 読込配線は PC-6 持越 |
| (β') | `settings.xml` cvar 露出 + `gSavedSettings.getU32("AYARingBufferSizeMB")` 読込 + ring buffer ctor 引数経路まで配線 (= llcommon 単体では `gSavedSettings` 無し、上層 hookup 要) |
| (γ') | `settings.xml` cvar 露出 + 読込 + Vulkan device 配線 (= 実 `vmaCreateBuffer` 経路まで) |

### §2.2 AYA 確定 literal

「Claude 推奨: (α')」(= 2026-06-04 session 受領) → **(α') 採用**。

### §2.3 (α') 採用根拠 3 件

1. **PC-3 precedent 整合**: PC-3 (α) と同形 = `llcommon` 単体 algorithm + DI callback + TUT framework、Vulkan device wire up は PC-6 一括で集約。Phase 1.C 全 PC 同じ pattern で scope 一貫性確保。
2. **`feedback_ubo_migration_one_at_a_time` 整合**: 1 PC = 1 algorithm class、Vulkan device 配線は別 PC = 並走 risk 排除。
3. **`feedback_design_phase_no_code_write` 準拠**: 実 Vulkan device wire up は llrender/llvkloader touch を伴い、PC-4 単独 scope を逸脱。PC-6 で 4 cadence pool + ring buffer + PSO cache を統合 wire up = touch 範囲集約で diff review 容易。

---

## §3 PC-4 実施内容

### §3.1 新 class `LLUboRingBuffer` 起案

**header** (= `indra/llcommon/lluboringbuffer.h`、131 line):

```cpp
class LLUboRingBuffer
{
public:
    using BufferHandle    = std::uint64_t;
    using BufferAllocator = std::function<BufferHandle(std::uint32_t /*size_bytes*/)>;
    using BufferDestroyer = std::function<void(BufferHandle)>;

    static constexpr std::uint32_t kInitialSizeMB    = 4;   // design 07 §7.2
    static constexpr std::uint32_t kMaxSizeMB        = 16;  // design 07 §7.2
    static constexpr std::uint32_t kFramesInFlight   = 3;   // design 07 §7.5 + §8.4
    static constexpr std::uint32_t kDefaultAlignment = 256; // design 07 §7.3

    LLUboRingBuffer(BufferAllocator allocator, BufferDestroyer destroyer,
                    std::uint32_t initial_size_mb = kInitialSizeMB,
                    std::uint32_t max_size_mb     = kMaxSizeMB,
                    std::uint32_t alignment       = kDefaultAlignment);
    ~LLUboRingBuffer();

    bool initialize();
    void shutdown();

    struct AllocateResult {
        BufferHandle  buffer  = 0;
        std::uint32_t offset  = 0;
        std::uint32_t size    = 0;
        bool          success = false;
        bool          grew    = false;
    };
    AllocateResult allocate(std::uint32_t size_bytes);
    void beginFrame();

    // 14 getters: getInitialSizeMB / getMaxSizeMB / getCurrentSizeMB /
    //   getCurrentSizeBytes / getChunkSizeBytes / getAlignment /
    //   getFrameIndex / getActiveChunk / getChunkBytesUsed /
    //   getChunksUsedThisFrame / getBuffer / isInitialized / isAtMaxSize
};
```

- copy-disabled (`= delete`)
- header file-level doc comment で `design/07-vulkan-api-state.md` §7.2/§7.3/§7.5/§8.4 確定値根拠 + (α') 採用根拠 + PC-3 (α) pattern 踏襲を明示
- `AllocateResult.offset` = `vkCmdBindDescriptorSets` `pDynamicOffsets[]` 投入経路 (= design 07 §7.4)

**implementation** (= `indra/llcommon/lluboringbuffer.cpp`、220 line):

| method | 動作 |
|---|---|
| ctor | `initial_size_mb == 0` ? `kInitialSizeMB` : initial_size_mb 等、0 渡された時 default fallback + `mInitialSizeMB > mMaxSizeMB` 時は cap |
| `initialize()` | `invokeAllocator(mInitialSizeMB * 1MB)` 呼出、null 時 false、成功時 state 初期化 + `mInitialized = true` |
| `shutdown()` | `destroyCurrentBuffer()` + state reset + `mInitialized = false` |
| `allocate(size_bytes)` | (1) align up to `mAlignment` (2) `aligned > chunk_size` 時 `tryGrow` (= 1 alloc が 1 chunk 越え special case) (3) `mChunkBytesUsed + aligned > chunk_size` 時: chunks_used+1 >= 3 (= N-1 hazard) なら `tryGrow`、それ以外なら wrap to next chunk (4) offset = `mActiveChunk * chunk_size + mChunkBytesUsed` (5) `mChunkBytesUsed += aligned` |
| `beginFrame()` | `++mFrameIndex`、`mActiveChunk = mFrameIndex % 3`、`mChunkBytesUsed = 0`、`mChunksConsumedThisFrame = 1` |
| `tryGrow()` | already at max 時 false、`new_size = cur × 2` capped at `mMaxSizeMB`、`invokeAllocator` 失敗時 false、成功時 `destroyCurrentBuffer` + 新 buffer 採用 |
| `invokeAllocator(size_bytes)` | `mAllocator` 無効時 0、有効時 callback 呼出 |
| `destroyCurrentBuffer()` | `mBuffer != 0 && mDestroyer` 時 `mDestroyer(mBuffer)`、`mBuffer = 0` |

### §3.2 TUT unittest 11 ケース起案

**file**: `indra/llcommon/tests/lluboringbuffer_test.cpp` (= 306 line、`tut::test_group<ubo_ring_buffer_data>` 使用)

| # | 検証内容 | ensure 条件 |
|---|---|---|
| 1 | default 値 + 未 init state | `kInitialSizeMB=4 / kMaxSizeMB=16 / kFramesInFlight=3 / kDefaultAlignment=256` + 0 buffer + !initialized |
| 2 | `initialize()` 後 | `getBuffer() != 0` + `getCurrentSizeMB() == 4` + `getCurrentSizeBytes() == 4 MB` + `getChunkSizeBytes() ≈ 1.33 MB` + initialized flag |
| 3 | 小 alloc + 256 alignment | 1B alloc → `result.size == 256`、`result.offset == 0`、`mChunkBytesUsed == 256` |
| 4 | 非 default alignment (= 64) | 1B alloc → `result.size == 64`、`mChunkBytesUsed == 64` |
| 5 | chunk overflow → safe wrap (no grow) | chunk 内一杯 alloc 後 + 1B alloc → `result.grew == false` + `mActiveChunk` advance + `mChunksConsumedThisFrame == 2` |
| 6 | 3 chunk hazard → grow trigger | 2 chunk 連続消費後 + 1B alloc → `result.grew == true` + buffer doubled (= 3 → 6 MB) |
| 7 | 多重 grow chain (= 3 → 6 → 12 MB cap) | grow chain 動作 + `getCurrentSizeMB() <= mMaxSizeMB` |
| 8 | max cap enforcement | 16 MB cap 到達後の alloc fail = `result.success == false` |
| 9 | allocator failure | factory 0 return → `initialize() == false` |
| 10 | shutdown 後再 init | shutdown → 0 buffer → 再 `initialize()` で新 buffer |
| 11 | `beginFrame()` frame index 進行 + chunk reset | `++mFrameIndex` + `mActiveChunk = mFrameIndex % 3` + `mChunkBytesUsed == 0` + `mChunksConsumedThisFrame == 1` |

`ubo_ring_buffer_data` struct = test fixture (= `next_handle` 連番 + `destroyed` log + `makeIncrementingAllocator` / `makeRecordingDestroyer`)、PC-3 (α) pattern 踏襲。

### §3.3 `CMakeLists.txt` 3 line 編集

**file**: `indra/llcommon/CMakeLists.txt`

| 編集 | 位置 | 内容 |
|---|---|---|
| 追加 1 | line 103 | `lluboringbuffer.cpp` (= `lltracethreadrecorder.cpp` ↔ `lluri.cpp` 間、alphabetical) |
| 追加 2 | line 245 | `lluboringbuffer.h` (= `lltreeiterators.h` ↔ `llunits.h` 間、alphabetical) |
| 追加 3 | line 343 | `LL_ADD_INTEGRATION_TEST(lluboringbuffer "" "${test_libs}")` (= `LL_ADD_INTEGRATION_TEST(llassetubopool ...)` 直後) |

### §3.4 `settings.xml` cvar 1 件追加

**file**: `indra/newview/app_settings/settings.xml`、line 10364-10381

```xml
<!-- <FS:AYAstorm r41 Phase 1.C PC-4> per-frame / per-pass cadence UBO ring buffer の起動時 prealloc 容量 (MB)。
     design/07-vulkan-api-state.md §7.2 確定値 = default 4 MB / grow 上限 16 MB。
     本 cvar は debug settings 露出のみ (= PC-4 (α') = llcommon LLUboRingBuffer algorithm 単体 +
     settings.xml cvar 露出)。実 cvar 読込 + Vulkan device 配線 (vmaCreateBuffer + HOST_VISIBLE+MAPPED) は
     PC-6 (= 5 cadence update site 着手 timing) で llrender / llvkloader 側 wire up 予定。
     変更には viewer 再起動が必要 (= ring buffer は起動時 1 回 prealloc、frame 中の動的変更非対応)。 -->
<key>AYARingBufferSizeMB</key>
<map>
  <key>Comment</key>
  <string>(r41 Phase 1.C) per-frame / per-pass cadence UBO ring buffer の起動時 prealloc 容量 (MB)。default=4 ... grow 上限 16 MB ... 実 wire up は PC-6 持越、本 cvar は debug settings 露出のみ。変更には viewer 再起動が必要</string>
  <key>Persist</key><integer>1</integer>
  <key>Type</key><string>U32</string>
  <key>Value</key><integer>4</integer>
</map>
<!-- </FS:AYAstorm> -->
```

挿入位置: `AYAViewModeMigrationVersion` block 直後 + `PluginInstancesLow` block 直前。`<FS:AYAstorm r41 Phase 1.C PC-4>` ... `</FS:AYAstorm>` で区切り = r41 PC-4 由来明示。

### §3.5 build verify = `LL_TESTS=ON` 再 configure + INTEGRATION_TEST 実走

1. **CMake reconfigure** = `cmake -DLL_TESTS=ON /home/ishikawa/work_firestorm/phoenix-firestorm/indra` → Configuring done (2.7s) + Generating done (0.2s) PASS + AyaUboCodegen 91 blueprint 反映
2. **`make -j4 INTEGRATION_TEST_lluboringbuffer`** 実走:
   - Building `lluboringbuffer.cpp.o` PASS
   - Linking llcommon static library PASS
   - Building `tests/lluboringbuffer_test.cpp.o` + `__/test/test.cpp.o` + `__/test/lltut.cpp.o` PASS
   - Linking `INTEGRATION_TEST_lluboringbuffer` executable PASS
   - **POST_BUILD auto-run**:
     ```
     Unit test group_started name=LLUboRingBuffer
     Unit test group_completed name=LLUboRingBuffer
         Total Tests:	11
         Passed Tests:	11	YAY!! \o/
     ```
3. ERROR 0 件 / WARNING 0 件 (= lluboringbuffer 関連、既知 volk.c `-Wno-reorder` 除く)

### §3.6 codegen unittest = 130/130 PASS

`python3 -m unittest discover -s scripts/ubo_codegen/tests` = `Ran 130 tests in 0.062s` `OK` (= Phase 1.A / 1.B / 1.C PC-1..PC-3 regression なし)。

---

## §4 PC-4 Exit Criteria 充足 record

prep doc §3.1 PC-4 row Exit literal = 「per-frame cadence ring buffer 4 MB alloc + cvar 反映 + 16 MB 上限 enforcement」。

| Exit 項目 | 充足 |
|---|---|
| (i) per-frame cadence ring buffer 4 MB alloc | ✅ TUT test<2> = `getCurrentSizeMB() == 4` + `getCurrentSizeBytes() == 4194304` (= 4 MB) |
| (ii) cvar 反映 | ✅ `settings.xml:10370` `AYARingBufferSizeMB` cvar 追加 (= (α') 解釈 = debug settings 露出 only、AYA 確定 2026-06-04) |
| (iii) 16 MB 上限 enforcement | ✅ TUT test<7> = grow chain 上限到達 + TUT test<8> = `isAtMaxSize() == true` + alloc fail (= `result.success == false`) |
| (iv) unittest PASS (= 暗黙、PC-3 precedent) | ✅ TUT 11/11 PASS (= `INTEGRATION_TEST_lluboringbuffer` POST_BUILD auto-run) |

---

## §5 残 strict 線形 (= prep doc §3.2 整合)

```
PC-4 ✅ (本 commit) → PC-5 (PSC PSO cache 64 MB)
                  → PC-6 (5 cadence update site = block-level bring-up と本格置換、SAMPLER skip 正攻法対応 候補 timing、(W2) 実 VkDescriptorPool factory injection wire up + (RB) 実 vmaCreateBuffer factory injection wire up + cvar 読込 hookup timing)
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
- PC-3 ✅ pending commit (= `sAssetUboPool` grow algorithm + unittest 10/10、前 session 完結未 commit)
- **PC-4 ✅ 本 commit** (= `LLUboRingBuffer` ring buffer algorithm + cvar 露出 + unittest 11/11)
- PC-5..PC-N ⏳ 次 session

---

## §7 self-verify 9 観点 全 ✅

1. **PC-4 Exit Criteria 4 項全充足** = §4 record
2. **ring buffer 4 MB / 16 MB / chunk 構造 / alignment source doc 整合** = `design/07-vulkan-api-state.md` §7.2/§7.3/§7.5/§8.4 全引用、code constexpr 値と完全一致
3. **(α') 採用根拠 3 件 record** = §2.3 = PC-3 precedent 整合 + `feedback_ubo_migration_one_at_a_time` 整合 + `feedback_design_phase_no_code_write` 準拠
4. **GATE-B 整合** = `mUseUBO` runtime gate に依存しない algorithm 層 (= 本 PC-4 は pure bookkeeping、`#ifdef LL_VULKAN_GLSL` C++ 不使用、`mUseUBO` も touch せず)
5. **MUSEUBO-A 整合** = 本 PC-4 は callsite が無く ((RB) wire up は PC-6)、既存 OpenGL 挙動 100% 維持
6. **llcommon target build PASS** + **INTEGRATION_TEST_lluboringbuffer build + POST_BUILD auto-run 11/11 PASS**
7. **codegen unittest 130/130 PASS** (= Phase 1.A / 1.B / 1.C PC-1..PC-3 regression なし)
8. **commit 内容 prep** = 2 new file (lluboringbuffer.h + .cpp) + 1 new test file (tests/lluboringbuffer_test.cpp) + 2 modified (CMakeLists.txt + settings.xml) + 1 new doc (本 handoff) + Co-Authored-By 不在
9. **`feedback_no_scope_shrink` 遵守** = literal scope 「per-frame cadence ring buffer 4 MB alloc + cvar 反映 + 16 MB 上限 enforcement」を縮小せず、(α') 採用で algorithm + 11 unittest case + cvar 露出 全実装

---

## §8 引き継ぎ memory (= 既存活用、新規追加なし、`feedback_*` 系のみ参照)

| memory | 適用観点 |
|---|---|
| `project_ayastorm_r41_vulkan_migration` | r41 milestone state (= 本 handoff §6) |
| `project_r41_phase1b_vulkan_host_gate` | GATE-B = `mUseUBO` runtime gate のみ、本 PC-4 algorithm 層は無関係 |
| `project_ayastorm_r41_design_principles` | (1) Upstream OpenGL 取り込みやすさ維持 + (2) Core プロセス分散実現 → dependency injection で algorithm 層分離 (= 原則 2 整合) |
| `feedback_ubo_migration_one_at_a_time` | 本 PC-4 は (RB) 持越項目 1 件のみ、cold launch 検証は PC-6 wire up 時 |
| `feedback_handoff_minimal_pre_req_read` | §1.1 必読 3 件 + §1.2 pinpoint reference 別記、全件読み禁止 |
| `feedback_self_verify_before_handoff` | §7 9 観点 self-verify 全 ✅ |
| `feedback_build_only_verified` | TUT 11/11 + codegen 130/130 で literal 検証取得、机上推論せず |
| `feedback_no_scope_shrink` | (α') 採用は scope 縮小ではない (= §2.1 3 解釈明示後 AYA 確定で literal scope 充足) |
| `feedback_doubt_self_first` | (R1)/(RB) 名前 drift (= 旧 R1 → 新 RB rename) 発見で `design/07-vulkan-api-state.md` §12 grep 確認 + cvar scope ambiguity 解消で AYA 確認実施 |
| `feedback_proactive_handoff` | PC-5 引継 marker 本 handoff で能動 handoff |
| `feedback_release_branch_workflow` | feature branch (`feature/ayastorm-r41-gl-removal`) 上で work、release branch 直 commit せず |
| `feedback_no_auto_commit` | AYA 「commit してください」literal 受領後 commit |
| `feedback_no_claude_coauthor` | Co-Authored-By 行不在 |
| `feedback_design_phase_no_code_write` | (α') 採用で Vulkan device wire up は PC-6 まで含めず、本 PC-4 は algorithm + cvar 露出のみで scope 厳守 |

---

## §9 commit 段取り = PC-3 + PC-4 連続 commit 提案

git status 現状:
```
M indra/llcommon/CMakeLists.txt          (PC-3 + PC-4 両方の 3+3 line 編集が混在)
M indra/newview/app_settings/settings.xml (PC-4 cvar 1 件)
?? docs/.../handoff-substep-...-phase1-c-pc-3.md  (PC-3 handoff doc、前 session 起案未 commit)
?? docs/.../handoff-substep-...-phase1-c-pc-4.md  (本 handoff doc、本 session 起案)
?? indra/llcommon/llassetubopool.cpp + .h          (PC-3 source、前 session 起案未 commit)
?? indra/llcommon/llassetubopool_test.cpp          (PC-3 test、前 session 起案未 commit、`tests/` は indra/llcommon 配下で OK)
?? indra/llcommon/lluboringbuffer.cpp + .h         (PC-4 source、本 session 起案)
?? indra/llcommon/lluboringbuffer_test.cpp         (PC-4 test、本 session 起案)
```

**推奨 commit 順** (= AYA 「commit してください」literal 受領後実行):

1. **PC-3 commit**: llassetubopool.h/.cpp + tests/llassetubopool_test.cpp + CMakeLists.txt (3 line のみ part-stage) + PC-3 handoff doc
2. **PC-4 commit** (本 commit): lluboringbuffer.h/.cpp + tests/lluboringbuffer_test.cpp + CMakeLists.txt (残 3 line) + settings.xml + 本 PC-4 handoff doc

CMakeLists.txt は `git add -p` で PC-3 / PC-4 分割 stage 推奨 (= 2 commit で diff review 容易性確保)。

`feedback_tests_dir_never_commit` 整合: top-level `tests/` (= AYA security info dir) には触れない、`indra/llcommon/tests/` は viewer source dir 配下で別物 (= 既存 tracked `*_test.cpp` 多数存在で確認済)。

---

## §10 次 session 着手 1 line

**PC-5 着手** = (PSC) PSO cache 64 MB 実装 = `design/07-vulkan-api-state.md` §11 PSO cache 仕様 record (= cache 構造 + default 値 + grow / eviction policy)。

---

## §11 次 session bootstrap (= AYA から次 session に投げる短い要約 candidate)

```
前 session で PC-4 = (R1)/(RB) ring buffer 4 MB / 16 MB grow algorithm + cvar AYARingBufferSizeMB 実装 complete (= llcommon 配置 + LL_ADD_INTEGRATION_TEST framework + TUT 11/11 PASS + settings.xml cvar 露出 (α') = debug settings 露出のみ、実 wire up は PC-6 持越)。本 session 着手 = PC-5 = (PSC) PSO cache 64 MB 実装。

必読 3 件:
- docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-4.md (全文)
- docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-prep.md (§3.1 PC-5 row + §5.1 (PSC) + §3.2 strict 線形)
- docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md (§11 PSO cache 仕様)

PC-5 から進めてください。
```
