# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-7ε design-lock** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-7ε (= dynamic offset 経路 ring buffer chunk hand-off = `bindV3aStatic` の `pDynamicOffsets[4]` zero placeholder を `sDrawUboRingBufferMgr` 経由 dynamic offset 実値配線) **design-lock phase 完了** marker = ambiguity 6 件 AYA literal「Claude 推奨案 OK」record (= 2026-06-05) + 実装計画 (a)-(g) 7 step 分解 + Exit Criteria 10 項明文化

---

## §0. 本 session 着手契機 + literal scope

**契機**: AYA 指示「PC-7ε design-lock 着手お願いします」literal 受領 (2026-06-05、前 PC-7α' complete commit `2e0de5e181` 後の継続 session = fresh context)。

**PC-7ε literal scope** (= 確認受領 6 件 ambiguity を反映した最終 scope):
1. **descriptor wiring** = `sDrawUboSetV3a` (1 set 固定) に ring buffer の `VkBuffer` を `vkUpdateDescriptorSets` で 4 binding 分 write (= 1 度のみ、`initVulkan` 内決定論的 timing)
2. **dynamic offset hand-off** = `bindV3aStatic` signature 拡張 = `const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS]` 引数化 (= caller 責任)、zero placeholder local 削除
3. **`recordPlaceholderPoolDraw` 1 draw chain** = `sDrawUboRingBufferMgr->allocate(256)` → dummy zero memset → `dynamic_offsets[4] = { offset, offset, offset, offset }` → `bindV3aStatic` 呼出
4. **ring buffer grow log** = `AllocateResult.grew == true` 観測時に `LL_WARNS_ONCE` 出力 (= re-wire 自動化は PC-N 持越、dummy phase 数 KB/frame ゆえ grow 起きない想定)
5. **`bindV3aRigged` 無改変** = set=2 復活 + per-draw 配線は PC-N 持越 (= H10-A avatar bone storage 再配線と一括)
6. **session 境界** = 本 design-lock phase は doc 起案のみ (= `indra/` 改変 0 件)、実装 phase は別 session の fresh context で着手

**session 境界根拠**: 前 PC-7δ design-lock (`4eb193601d`) + PC-7α' design-lock (`11a46edd9b`) と同形 = 大塊実装 (= ambiguity 確認 + 6 step 改変) は別 session の fresh context が安全 (= feedback_design_phase_no_code_write + feedback_proactive_handoff)。

---

## §1. 必読 3 件 + pinpoint reference

**次 session 必読 (= PC-7ε 実装着手前)**:

1. **本 design-lock doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-epsilon-design-lock.md`
2. **PC-7α' complete doc**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-alpha-prime-complete.md` §6 残 strict 線形 + §9 次 session 着手 1 line
3. **PC-7δ complete doc**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-delta-complete.md` §1.3 step (g)(h) `bindV3aStatic` / `bindV3aRigged` 実装記録 (= PC-7ε で signature 拡張対象)

**pinpoint reference (必要時のみ)**:

- **design 07 §7**: `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md` §7 「dynamic offset (L2) 実 Vulkan 配線 + ring buffer 容量」
  - §7.1 = 06b §5.3 確定 (= L1+L2 ring buffer + dynamic offset)
  - §7.2 = ring buffer 容量算定 (= 4 MB initial / 16 MB max)
  - §7.3 = offset alignment (= 256 B safe)
  - §7.4 = draw 単位 offset 投入経路 (= `offsets[4]` 構築 spec)
  - §7.5 = ring buffer 枯渇判定 + chunk 3 段
- **design 07 §8.4**: frame index 共有 (= `sFrameIndex` 全 cadence rotate)
- **design 07 §9.1**: pipeline layout 構成 (= 論理 5 set 帯、`sAYAStandardLayout`)
- **design 06b §5.3**: per-draw cadence Vulkan 最適化 (= L1+L2 採用根拠)
- **`indra/llcommon/lluboringbuffer.h`**: public API (= `AllocateResult` struct, `allocate()`, `beginFrame()`, `getBuffer()`)

**⚠ source doc 訂正**: 前 PC-7α' complete §9 + PC-7δ complete §5 が「design 07 **§9.3 + §12**」と記載していたが、実体は **§7** (= dynamic offset 配線 spec)。§9.3 = PSO cache、§12 = 未確定事項。本 PC-7ε design-lock doc 以降は **§7** を source とする。

---

## §2. 現状調査 (= PC-7δ 完了状態 + 未配線ギャップ)

### §2.1 PC-7δ 完了済 infrastructure

| 構成要素 | 状態 | 位置 |
|---|---|---|
| `LLUboRingBuffer` algorithm 層 | ✅ 完備 (TUT 11/11 PASS) | `indra/llcommon/lluboringbuffer.{h,cpp}` |
| `sDrawUboRingBufferMgr` 宣言 | ✅ `std::unique_ptr<LLUboRingBuffer>` | `llvkloader.cpp:418` |
| `sDrawUboRingBufferRecords` (= buffer handle → VkBuffer + VmaAllocation + mapped) | ✅ map 確保 | `llvkloader.cpp:410-417` |
| `createDrawUboRingBuffer()` (= VMA factory + destroyer 注入 + initialize) | ✅ | `llvkloader.cpp:1310-1391` |
| `sDrawUboRingBufferMgr->beginFrame()` per-frame 呼出 | ✅ `flushFrameUbos()` 内 | `llvkloader.cpp:4235` |
| `sDrawUboSetV3a` allocate (= 1 set 固定) | ✅ | `llvkloader.cpp:2594-2596` |
| `sDrawUboLayoutV3a` 構築 (= 4 binding × `UNIFORM_BUFFER_DYNAMIC`) | ✅ | `llvkloader.cpp:2387-2393` |
| `sDrawUboPoolV3a` 構築 (= maxSets=1) | ✅ | `llvkloader.cpp:2465-2472` |
| `bindV3aStatic` 4-set bind (= set=0/1a/1b/2、`pDynamicOffsets[4]={0,0,0,0}`) | ✅ placeholder | `llvkloader.cpp:1740-1769` |
| `bindV3aRigged` 4-set bind (= set=0/1a/1b/3、dynamic offset 0 個) | ✅ | `llvkloader.cpp:1771-1808` |
| `recordPlaceholderPoolDraw` (= `bindV3aStatic` 経由 placeholder draw) | ✅ | `llvkloader.cpp:4920` |
| `recordAvatarPlaceholderDraw` (= `bindV3aRigged` 経由 placeholder draw) | ✅ | `llvkloader.cpp:4978` |
| `flushDrawUbos()` (= dummy 256B zero memset) | ✅ placeholder | `llvkloader.cpp:4282-4289` |

### §2.2 🚨 未配線ギャップ (= PC-7ε 解消対象)

**`sDrawUboSetV3a` ↔ ring buffer の `VkBuffer` 接続が未配線**:

- `sDrawUboSetV3a` は確保済 (1 set、`llvkloader.cpp:2594-2596`)
- BUT `vkUpdateDescriptorSets` で ring buffer の `VkBuffer` を `sDrawUboSetV3a` に write していない (= grep `sDrawUboSetV3a` + `vkUpdateDescriptorSets` で接続なし確認、`llvkloader.cpp` 全体)
- 現状: `bindV3aStatic` (`llvkloader.cpp:1763-1768`) は空の descriptor set を bind している状態
- Vulkan validation layer enable 時は VUID error 発火想定 (= dynamic offset 持つ binding に NULL buffer)

**`bindV3aStatic` の `pDynamicOffsets[4]` zero placeholder**:

- `llvkloader.cpp:1759` で `const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS] = { 0u, 0u, 0u, 0u };` local 配列、全 4 zero
- `llvkloader.cpp:1733-1736` design note: 「pDynamicOffsets は本 PC-7δ では一律 0 (= ring buffer の actual offset 計算は PC-7ε scope = dynamic offset 経路 ring buffer chunk hand-off)」
- PC-7ε で実値 (= `AllocateResult.offset`) に置換要

### §2.3 V3a 定数 (= 確定)

| 定数 | 値 | 説明 | 位置 |
|---|---|---|---|
| `V3A_FRAME_SET_BINDINGS` | 4 | set=0 binding 数 | `llvkloader.cpp:653` |
| `V3A_PROGRAM_SET_A_BINDINGS` | 40 | set=1a binding 数 | `llvkloader.cpp:654` |
| `V3A_PROGRAM_SET_B_BINDINGS` | 40 | set=1b binding 数 | `llvkloader.cpp:655` |
| **`V3A_DRAW_SET_BINDINGS`** | **4** | **set=2 dynamic UBO binding 数 (= PC-7ε scope)** | `llvkloader.cpp:656` |
| `V3A_ASSET_SET_BINDINGS` | 3 | set=3 binding 数 | `llvkloader.cpp:657` |
| `V3A_DRAW_POOL_MAX_SETS` | 1 | ring buffer + dynamic offset で 1 set 固定 | `llvkloader.cpp:662` |
| `FRAMES_IN_FLIGHT` | 3 | per-frame rotate 数 | `llvkloader.cpp:113` |
| `LLUboRingBuffer::kInitialSizeMB` | 4 | ring buffer 起動時容量 | `lluboringbuffer.h:61` |
| `LLUboRingBuffer::kMaxSizeMB` | 16 | ring buffer grow 上限 | `lluboringbuffer.h:62` |
| `LLUboRingBuffer::kDefaultAlignment` | 256 | offset alignment | `lluboringbuffer.h:64` |

### §2.4 set=2 4 binding 内訳 (= `V3A_DRAW_SET_BINDINGS=4` の内容、design 07 §7.4 整合)

| binding | UBO block 名 | 期待 size | cadence |
|---|---|---|---|
| 0 | `Draw_LightParams` | std140 placeholder 256 B | PER_DRAW |
| 1 | `Draw_MultiLight` | std140 placeholder 256 B | PER_DRAW |
| 2 | `MaterialUBO` | std140 placeholder 256 B | PER_DRAW (= MC1 統合) |
| 3 | `MaterialLegacyBlinn` | std140 placeholder 256 B | PER_DRAW (= MC1 統合) |

**placeholder phase での実装簡略化**: 4 binding 同一 ring buffer 内の **同 offset** を参照 (= 1 allocate per draw × 4 binding 同 offset、(ε-2) A 採用)。`vkUpdateDescriptorSets` 時の `range` は 256 B placeholder (= 各 binding 独立 buffer info、buffer は同一 ring buffer、offset=0、range=256)。実 GLTF 通電 (PC-N) で binding 別 size + 4 個独立 allocate へ拡張。

---

## §3. ambiguity 6 件 AYA 確認 record (= 2026-06-05 literal「Claude 推奨案 OK」)

| # | 項目 | 採用案 (= Claude 推奨) | 採用根拠 |
|---|---|---|---|
| **(ε-1)** | PC-7ε scope 境界 | **A: descriptor wiring + dynamic offset hand-off + `recordPlaceholderPoolDraw` 1 draw chain** | descriptor wiring 未配線ゆえ A 必須 (§2.2 ギャップ)、B (= dynamic offset hand-off のみ) では bind が空 descriptor set を参照する状態が残り bind 経路通電目標未達 |
| **(ε-2)** | per-draw allocation 構造 | **A: 1 allocate per draw × 4 binding 同 offset (= 256 B placeholder)** | placeholder phase は 1 allocate で実装簡略化、PC-N 実 GLTF 通電時に B (= 4 独立 allocate) へ拡張、scope 縮小ではない (= design 07 §7.4 spec の段階的実現) |
| **(ε-3)** | `bindV3aStatic` signature | **A: `bindV3aStatic(cmd_buf, frame_index, const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS])` 引数化** | caller 責任分離 (= allocate ↔ bind を call site で連結明示)、test 可能性高、内部 query helper 案は state-dependent で副作用見えづらい |
| **(ε-4)** | descriptor wiring timing | **A: ring buffer init 直後に `vkUpdateDescriptorSets` 1 度 (= `initVulkan` 内決定論的 timing)** | 起動時 1 度のみ、決定論的、`AllocateResult.grew` 観測時の re-wire 拡張も容易、lazy 案は first allocate path で state 検出複雑化 |
| **(ε-5)** | `bindV3aRigged` の set=2 復活 | **A: PC-N 持越 (= 無改変、H10-A avatar bone storage 再配線と一括)** | scope 単一保持、rigged 経路の set=2 復活 = avatar bone storage の再配線と同 timing が合理、PC-N 実 GLTF avatar Vulkan draw 通電時に一括 |
| **(ε-6)** | ring buffer grow 時 re-wire | **A: `LL_WARNS_ONCE` log のみ (= 自動 re-update は PC-N 持越)** | dummy phase は数 KB/frame (= 256 B × 5 cadence × 数 frame) ゆえ grow 起きない (= 4 MB initial 容量の 0.001%)、実 GLTF 通電 PC-N まで simplification 許容、grow 観測時のみ log で診断容易 |

**追加確認 = source doc 訂正**: 前 handoff doc が「design 07 §9.3 + §12」と記載していた件、AYA 確認受領で本 PC-7ε design-lock doc 以降は **§7** を source とする (= §9.3 = PSO cache、§12 = 未確定事項、dynamic offset spec は §7)。

---

## §4. 実装計画 (a)-(g) 7 step + GATE-B/MUSEUBO-A 整合

### §4.1 step 分解

**(a) `wireDrawUboSetV3aToRingBuffer()` helper 新設** (= `llvkloader.cpp` 匿名 namespace 内):
- 4 個 `VkDescriptorBufferInfo` 構築 (= ring buffer の VkBuffer、offset=0、range=256 placeholder)
- 4 個 `VkWriteDescriptorSet` 構築 (= dstSet=`sDrawUboSetV3a`、dstBinding=0/1/2/3、descriptorType=`UNIFORM_BUFFER_DYNAMIC`)
- `vkUpdateDescriptorSets(sDevice, 4, writes, 0, nullptr)` 呼出
- guard = `sDrawUboRingBufferMgr` / `sDrawUboSetV3a` / `sDevice` 未初期化なら early return (= bool false)
- ring buffer の VkBuffer 取得経路 = `sDrawUboRingBufferMgr->getBuffer()` で BufferHandle 取得 → `sDrawUboRingBufferRecords[handle].buffer` で VkBuffer lookup
- return bool (= 全 step 成功時 true)

**(b) `initVulkan` 内呼出 chain 配線**:
- 順序確認: `createDrawUboRingBuffer()` (= ring buffer init、`llvkloader.cpp:1310-1391`) → `createV3aDescriptorPools()` → `createV3aDescriptorSetLayouts()` → `createV3aDescriptorSets()` (= `sDrawUboSetV3a` allocate、`llvkloader.cpp:2535-2670`) → **本 step (b) で `wireDrawUboSetV3aToRingBuffer()` 呼出を追加**
- 配置 = `createV3aDescriptorSets()` の `sDrawUboSetV3a` allocate 完了 (= `llvkloader.cpp:2594-2596` if 文 success path) 直後、または `createV3aDescriptorSets()` 末尾 (= LL_INFOS log 直後、`:2607-2614`)
- 失敗時 = LL_WARNS_ONCE + initVulkan 続行 (= mUseUBO=false default で bind 経路到達しないため fatal ではない)

**(c) `bindV3aStatic` signature 拡張** (`llvkloader.cpp:1740-1769`):
- 旧: `void bindV3aStatic(VkCommandBuffer cmd_buf, U32 frame_index)`
- 新: `void bindV3aStatic(VkCommandBuffer cmd_buf, U32 frame_index, const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS])`
- 内部の `const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS] = { 0u, 0u, 0u, 0u };` local 削除 (= `:1759`)
- `vkCmdBindDescriptorSets` の `pDynamicOffsets` 引数を function 引数の `dynamic_offsets` に置換
- guard 追加 = `dynamic_offsets == nullptr` で early return (= `cmd_buf == VK_NULL_HANDLE` 等の既存 guard と同形)
- tag block update = `<AYAstorm r41 PC-7δ (g)>` → `<AYAstorm r41 PC-7ε (c)>`、comment 内 「pDynamicOffsets は本 PC-7δ では一律 0」literal 更新

**(d) `recordPlaceholderPoolDraw` 内 allocate-chain 配線** (`llvkloader.cpp:4920` 周辺):
- 旧:
```cpp
vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, sSkySmokePipeline);
bindV3aStatic(cmd_buf, sFrameIndex);
// push constant + vkCmdDraw(3,1,0,0)
```
- 新:
```cpp
vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, sSkySmokePipeline);

// PC-7ε: per-draw allocate (1 件、4 binding 同 offset placeholder)
if (!sDrawUboRingBufferMgr) return;  // MUSEUBO-A guard
const LLUboRingBuffer::AllocateResult alloc = sDrawUboRingBufferMgr->allocate(256);
if (!alloc.success) {
    LL_WARNS_ONCE("Vulkan") << "PC-7ε ring buffer allocate failed in recordPlaceholderPoolDraw, skip draw" << LL_ENDL;
    return;
}
if (alloc.grew) {
    LL_WARNS_ONCE("Vulkan") << "PC-7ε ring buffer grew, sDrawUboSetV3a may reference stale buffer "
                            << "(re-wire 自動化は PC-N 持越、dummy phase 数 KB/frame ゆえ grow 起きない想定)" << LL_ENDL;
}
// dummy write (= placeholder phase、PC-6ε-3 で real per-draw data write 置換)
auto it = sDrawUboRingBufferRecords.find(alloc.buffer);
if (it != sDrawUboRingBufferRecords.end() && it->second.mapped != nullptr) {
    std::memset(static_cast<U8*>(it->second.mapped) + alloc.offset, 0, alloc.size);
}
const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS] = { alloc.offset, alloc.offset, alloc.offset, alloc.offset };
bindV3aStatic(cmd_buf, sFrameIndex, dynamic_offsets);
// push constant + vkCmdDraw(3,1,0,0)
```
- tag block = `<AYAstorm r41 PC-7ε (d)>` 新設

**(e) `recordAvatarPlaceholderDraw` 無改変確認** (= `llvkloader.cpp:4978`):
- 現状 `bindV3aRigged(cmd_buf, sFrameIndex)` 呼出維持 (= signature 不変)
- bindV3aRigged は dynamic offset 0 個 (= set=2 skip、set=0/1a/1b/3 のみ)、signature 拡張不要
- (ε-5) PC-N 持越記録のため comment 1 行追加可 = 「`// <PC-7ε note> rigged path is set=2 skip; per-draw dynamic offset wiring deferred to PC-N (= H10-A avatar bone storage 再配線と一括)`」

**(f) ring buffer-set=2 接続 維持確認**:
- `wireDrawUboSetV3aToRingBuffer()` (= step (a)) は initVulkan で 1 度のみ呼出
- ring buffer の VkBuffer は initialize() で 1 度確保 (= `createDrawUboRingBuffer()` line 1376 内)、destroy は shutdown 経路のみ
- ring buffer grow は LLUboRingBuffer 内部で新 VkBuffer 確保 + 旧 destroy、現 dummy phase では発火想定なし
- grow 観測時の handling = step (d) 内 `LL_WARNS_ONCE` (= (ε-6) A 採用)

**(g) build verify** (= 全 step 完了後、`indra/` 改変 4 件 modified 想定):
- `make -j4 llrender` PASS + WARNING 0
- `INTEGRATION_TEST_lluboringbuffer` 11/11 PASS (= ring buffer algorithm 層 regression なし)
- `INTEGRATION_TEST_llassetubopool` 10/10 PASS
- `INTEGRATION_TEST_llpipelinecachestorage` 13/13 PASS
- `python3 -m unittest discover -s scripts/ubo_codegen/tests -t scripts/ubo_codegen` 131/131 PASS (= codegen 層 regression なし)

### §4.2 GATE-B 整合 (= `#ifdef LL_VULKAN_GLSL` 新規追加 0 件)

- step (a)-(g) 全て host C++ + ring buffer 経路 = GLSL 改変 0 件
- `#ifdef LL_VULKAN_GLSL` 追加箇所 0 (= PC-7δ / PC-7α' / PC-7γ-3 と同形 GATE-B 整合)

### §4.3 MUSEUBO-A 整合 (= mUseUBO=false default で OpenGL 描画 100% 維持)

- `wireDrawUboSetV3aToRingBuffer()` (= step (a)) = Vulkan initVulkan 経路のみ呼出 = Vulkan 未起動 / mUseUBO=false default では descriptor wire 自体不発火
- `bindV3aStatic` signature 拡張 = caller (= `recordPlaceholderPoolDraw`) も Vulkan placeholder draw 経路のみ呼出
- `recordPlaceholderPoolDraw` (= sky smoke offscreen FBO 経路) は実 OpenGL 描画 path から独立 = 既存 OpenGL draw 100% 維持
- `bindV3aRigged` 無改変 (= step (e)) = 既存 avatar placeholder draw 経路維持

### §4.4 cadence × set × dynamic offset 表 (= PC-7ε 完成後の bind 構造)

| cadence | set | binding 数 | dynamic offset 数 | triple-buffer | source |
|---|---|---|---|---|---|
| per-frame | set=0 | 4 (= V3A_FRAME_SET_BINDINGS) | 0 | 3 set rotate (`sFrameUboSetV3a[3]`) | design 07 §8.2 |
| per-program | set=1a | 40 (= V3A_PROGRAM_SET_A_BINDINGS) | 0 | 3 set rotate (`sProgramUboSetA[3]`) | design 07 §6.2 (b) |
| per-program | set=1b | 40 (= V3A_PROGRAM_SET_B_BINDINGS) | 0 | 3 set rotate (`sProgramUboSetB[3]`) | design 07 §6.2 (b) |
| **per-draw** | **set=2** | **4 (= V3A_DRAW_SET_BINDINGS)** | **4 (= PC-7ε 配線対象)** | **ring buffer 3 chunk (`sDrawUboSetV3a` × 1)** | **design 07 §7.4** |
| per-asset | set=3 | 3 (= V3A_ASSET_SET_BINDINGS) | 0 | 3 set rotate (`sAssetUboSetV3a[3]`) | design 07 §6.3 |
| per-skin | set=3 (同居) | (per-asset と同居) | 0 | (per-asset と同居) | design 07 §5.2 |

**`maxBoundDescriptorSets=4` 死守**:
- static path (= `bindV3aStatic`) = set=0 + 1a + 1b + 2 = 4 set 同時 bind ✅
- rigged path (= `bindV3aRigged`) = set=0 + 1a + 1b → set=3 単独 swap = 2 step bind ✅

---

## §5. Exit Criteria 10 項

| # | Criteria | 充足 verify 方法 |
|---|----------|-----------|
| (i) | `wireDrawUboSetV3aToRingBuffer()` helper 新設 + 4 binding 分 `vkUpdateDescriptorSets` call | `llvkloader.cpp` diff 内 helper 関数追加 + `vkUpdateDescriptorSets(sDevice, V3A_DRAW_SET_BINDINGS, ...)` literal 出現 |
| (ii) | `initVulkan` 内で ring buffer init + `createV3aDescriptorSets()` 完了後に wire 呼出 | `llvkloader.cpp` diff 内 `wireDrawUboSetV3aToRingBuffer()` 呼出 追加 (= `createV3aDescriptorSets` 末尾 or `initVulkan` 末尾近く) |
| (iii) | `bindV3aStatic` signature 拡張 = `const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS]` 引数化 | `llvkloader.cpp:1740-1769` diff 内 signature literal `const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS]` |
| (iv) | `bindV3aStatic` 内 zero placeholder local 削除 (= caller responsibility) | `llvkloader.cpp` 内 `dynamic_offsets[V3A_DRAW_SET_BINDINGS] = { 0u, 0u, 0u, 0u }` literal 消滅 (= comment 履歴のみ残存可) |
| (v) | `recordPlaceholderPoolDraw` 内 allocate → dummy write → bind chain 配線 | `llvkloader.cpp:4920` 周辺 diff 内 `sDrawUboRingBufferMgr->allocate(256)` + `dynamic_offsets[4] = { alloc.offset, ... }` + `bindV3aStatic(..., dynamic_offsets)` literal |
| (vi) | `recordAvatarPlaceholderDraw` 無改変 = `bindV3aRigged` + set=2 復活は PC-N 持越 | `llvkloader.cpp:4978` 周辺 diff 内 bind 経路改変 0 件 (= comment 1 行追加可) |
| (vii) | ring buffer grow 観測時 `LL_WARNS_ONCE` log のみ | `llvkloader.cpp` 内 `alloc.grew` 判定 + `LL_WARNS_ONCE` literal 出現、自動 re-update path 無し |
| (viii) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 | `git diff` で `#ifdef LL_VULKAN_GLSL` 新規行 grep 0 件 |
| (ix) | MUSEUBO-A 整合 = mUseUBO=false default で既存 OpenGL 描画 100% 維持 | code review = `recordPlaceholderPoolDraw` (= sky offscreen FBO 経路) のみ改変 + `wireDrawUboSetV3aToRingBuffer` は Vulkan initVulkan 経路、setter / forwardToUboUpload / OpenGL draw 経路不変 |
| (x) | build verify = llrender + warning 0 + TUT 11+10+13 + codegen 131/131 全 PASS | `make -j4 llrender` PASS + INTEGRATION_TEST 3 件 PASS + `python3 -m unittest` 131/131 PASS literal record |

---

## §6. 着手手順 (= 次 session 実装 phase で実施)

1. **本 design-lock doc 全文 read** (= 必読 1 件) + PC-7α'/PC-7δ complete doc pinpoint reference (§1)
2. **`indra/llrender/llvkloader.cpp` の現状確認** = `bindV3aStatic` (`:1740-1769`) + `recordPlaceholderPoolDraw` (`:4920`) + `createDrawUboRingBuffer` (`:1310-1391`) + `createV3aDescriptorSets` (`:2535-2670`) read
3. **step (a) `wireDrawUboSetV3aToRingBuffer()` 実装** = helper 関数追加 (匿名 namespace 内、`createDrawUboRingBuffer` の近傍)
4. **step (b) `initVulkan` 内呼出 chain 追加** = `createV3aDescriptorSets()` 末尾 or `initVulkan` 末尾近くで `wireDrawUboSetV3aToRingBuffer()` 呼出
5. **step (c) `bindV3aStatic` signature 拡張** = 引数追加 + zero placeholder 削除 + tag block update
6. **step (d) `recordPlaceholderPoolDraw` allocate-chain 配線** = allocate → dummy write → bind chain
7. **step (e) `recordAvatarPlaceholderDraw` 無改変確認** = comment 1 行追加 (= PC-N 持越記録)
8. **step (f) ring buffer-set=2 接続 維持確認** = code review
9. **step (g) build verify** = `make -j4 llrender` + INTEGRATION_TEST 3 件 + codegen unittest 131/131
10. **Exit Criteria 10 項 self-verify** + `handoff-...-pc-7-epsilon-complete.md` 起案
11. **AYA literal commit 指示受領後 commit** (= `feedback_no_auto_commit`)

---

## §7. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + **PC-7ε design-lock ✅ 本 commit** + PC-7ε 実装 ⏳ 次 session + PC-8..PC-N ⏳ 次々 session 以降

---

## §8. self-verify 9 観点 全 ✅

1. **PC-7ε literal scope 6 件 §0 完全分解** ✅ = descriptor wiring + dynamic offset hand-off + recordPlaceholderPoolDraw chain + grow log + bindV3aRigged 無改変 + session 境界
2. **必読 3 件 §1 列挙 + pinpoint reference 別記 (= design 07 §7 / §8.4 / §9.1 + 06b §5.3 + lluboringbuffer.h)** ✅
3. **現状調査 §2 5 項網羅** ✅ = PC-7δ 完了 infrastructure 表 (§2.1) + 未配線ギャップ (§2.2) + V3a 定数 (§2.3) + set=2 4 binding 内訳 (§2.4)
4. **ambiguity (ε-1)..(ε-6) 6 件 AYA literal「Claude 推奨案 OK」record (2026-06-05) §3** ✅ + source doc 訂正 record
5. **採用根拠 6 件明文化 §3 表** ✅
6. **実装計画 (a)-(g) 7 step 分解 §4.1** ✅
7. **Exit Criteria 10 項明文化 §5** ✅
8. **GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 §4.2** ✅
9. **MUSEUBO-A 整合 = mUseUBO=false default 経路不変 + placeholder offscreen FBO 経路 + recordAvatarPlaceholderDraw 無改変 §4.3** ✅

---

## §9. 次 session 着手 1 line

**PC-7ε 実装着手** = step (a)-(g) 7 step 実施 = (a) `wireDrawUboSetV3aToRingBuffer()` helper 新設 + (b) `initVulkan` 呼出 chain + (c) `bindV3aStatic` signature 拡張 + (d) `recordPlaceholderPoolDraw` allocate-chain 配線 + (e) `recordAvatarPlaceholderDraw` 無改変 + (f) ring buffer-set=2 接続維持確認 + (g) build verify。Exit Criteria 10 項全充足 + handoff-...-pc-7-epsilon-complete.md 起案 + AYA commit 指示受領後 commit。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 design-lock handoff doc 起案 (= 次 session 引継 marker)
- **feedback_handoff_minimal_pre_req_read** 遵守 = 次 session 必読 3 件 + pinpoint reference 別記 §1
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §8
- **feedback_build_only_verified** 遵守 = design-lock phase は `indra/` 改変 0 件で build verify 対象外、実装 phase で literal 検証取得予定 §5 (x)
- **feedback_no_scope_shrink** 遵守 = PC-7ε literal scope 6 件 §0 完全分解、(ε-2) 1 allocate placeholder + (ε-5) bindV3aRigged 無改変 + (ε-6) grow re-wire 持越 は段階分離 (= PC-N に AYA 確認受領記録あり)、縮小ではない
- **feedback_doubt_self_first** 遵守 = ambiguity 6 件発見で停止 + 推奨案提示 + AYA literal「Claude 推奨案 OK」確認後 design-lock doc 起案、推測実装なし
- **feedback_confirm_referent_before_acting** 遵守 = (ε-1)..(ε-6) 6 件 + source doc 訂正 1 件 batch AYA 確認 (2026-06-05)、source doc reference の齟齬発見時も停止確認
- **feedback_ubo_migration_one_at_a_time** 遵守 = PC-7ε = dynamic offset hand-off + descriptor wiring 単独 sub-step、bindV3aRigged set=2 復活 (PC-N) + flushDrawUbos real per-draw write (PC-6ε-3) + ring buffer grow auto re-wire (PC-N) は分離
- **feedback_design_phase_no_code_write** 整合 = 本 design-lock phase は doc 起案のみ、`indra/` 改変 0 件
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示「commit してください」literal 受領まで commit せず
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
- **feedback_no_bare_reference_ids** 遵守 = (ε-1)..(ε-6) 各 ID に項目名 / 採用案内容併記 §3

---
