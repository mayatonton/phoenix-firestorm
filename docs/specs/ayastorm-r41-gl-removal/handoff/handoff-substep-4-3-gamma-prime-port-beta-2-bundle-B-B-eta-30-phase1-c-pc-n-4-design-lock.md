# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-N-4 design-lock complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-N-4 (= ring buffer grow 自動 re-wire = `AllocateResult.grew` 観測 frame 末尾で `vkUpdateDescriptorSets` 再発火 deferred、`flushDrawUbos` PC-N-1 hook placeholder を実 trigger 化) の **design-lock phase 完了** marker = ambiguity (N4-1)..(N4-10) 10 件 AYA literal「推奨案採用 OK」record (= 2026-06-05) + 実装計画 (a)-(g) 7 step 分解 + Exit Criteria 10 項明文化。

> **本 doc 位置付け**: PC-N decomposition design-lock (= `cf7b0b99b0`) §4.3 で確定した PC-N-4 sub-step (= ring buffer grow 自動 re-wire) の design-lock phase doc。実装 phase は別 session の fresh context で着手。

---

## §0. 本 session 着手契機 + literal scope record

**契機**: AYA 指示「PC-N-4 design-lock 着手お願いします」literal 受領 (2026-06-05、PC-N-2 complete commit `2e0587bac7` 後の継続 session = 別 session の fresh context) + 必読 1 件 = `handoff-...-pc-n-2-complete.md` (= PC-N-2 実装 phase 完了 = `bindV3aRigged` set=2 復活 signature 拡張 + `recordAvatarPlaceholderDraw` allocate-chain 配線、§5 残 strict 線形 + §8 次 session 着手 1 line) 全文 Read + pinpoint reference 4 件 (PC-N decomposition design-lock §4.3 + `writeDrawUbo` grow 観測経路 `llvkloader.cpp:4711-4790` + `flushDrawUbos` PC-N-1 hook placeholder `llvkloader.cpp:4376-4413` + `AllocateResult.grew` 仕様 `lluboringbuffer.h`) pinpoint Read → Explore 経路で `wireDrawUboSetV3aToRingBuffer` (PC-7ε helper) + `beginFrame()` / `endFrame()` 構造 + `flushDrawUbos` 14 caller site 確認 → ambiguity (N4-1)..(N4-10) 10 件 + 推奨案 + 採用根拠提示 → AYA literal「推奨案採用 OK」一括確認受領 (2026-06-05) → 本 design-lock doc 起案。

**PC-N-4 literal scope 5 件** (= PC-N decomposition §4.3 継承):

1. **grow flag 新設**: anonymous namespace 内 `std::atomic<bool> sDrawUboRingBufferGrewThisFrame{false}` 追加 ((N4-1) A)
2. **`writeDrawUbo` 内 grow 観測時 flag set**: 既存 `if (alloc.grew)` block (`llvkloader.cpp:4824-4835`) の `LL_WARNS_ONCE` 直後に `sDrawUboRingBufferGrewThisFrame.store(true, memory_order_release)` 追加 ((N4-5) A)
3. **`endFrame()` 末尾 hook 配線**: `sInFrame=false` 直後 (`llvkloader.cpp:4111` 後) に `if (sDrawUboRingBufferGrewThisFrame.exchange(false, memory_order_acq_rel)) { wireDrawUboSetV3aToRingBuffer(); }` hook + first-fire LL_INFOS marker 追加 ((N4-2) D + (N4-4) A + (N4-6) A)
4. **`flushDrawUbos` PC-N-1 placeholder comment 撤去**: line 4420-4424 の「PC-N-4 持越 hook」comment block を「PC-N-4 で `endFrame()` 経由実装済」literal 置換 ((N4-2) D 整合)
5. **`writeDrawUbo` 内 LL_WARNS_ONCE comment 整合更新**: line 4826-4828 の「自動 re-update は PC-N-4 持越」literal を「PC-N-4 で実装済 (= `endFrame()` 末尾 hook)」literal 更新

---

## §1. 必読 1 件 + pinpoint reference (次 session = PC-N-4 実装着手向け)

**次 session 必読**:

1. **本 PC-N-4 design-lock doc 全文**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-n-4-design-lock.md`

**pinpoint reference** (実装 phase で必要分のみ):

- **PC-N-2 complete doc**: `handoff-...-pc-n-2-complete.md` = PC-N-2 完了状態 (= `bindV3aRigged` set=2 復活 + `recordAvatarPlaceholderDraw` allocate-chain) の参考、PC-N-4 着手 baseline
- **`writeDrawUbo` grow 観測経路**: `indra/llrender/llvkloader.cpp:4824-4835` (= `<AYAstorm r41 PC-N-1 (a)>` tag block 内 `if (alloc.grew)` block) = step (b) flag set 追加 site
- **`flushDrawUbos` PC-N-1 hook placeholder**: `indra/llrender/llvkloader.cpp:4388-4426` = step (d) comment 撤去 site
- **`endFrame()` 構造**: `indra/llrender/llvkloader.cpp:4094-4113` = step (c) hook 追加 site (= `sInFrame=false` 直後)
- **`wireDrawUboSetV3aToRingBuffer()` helper**: `indra/llrender/llvkloader.cpp:2658-2710` (= `<AYAstorm r41 PC-7ε (a)>` tag block) = step (c) 再呼出 helper、4 binding × UNIFORM_BUFFER_DYNAMIC + `vkUpdateDescriptorSets` 既存実装
- **`AllocateResult.grew` 仕様**: `indra/llcommon/lluboringbuffer.h:88` = `bool grew = false; // 本 alloc 内で ring buffer grow が発火した` (= caller 観測責任)
- **design 07 §7.5 grow 仕様**: `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md:404-440` = ring buffer 4 MB → 8 MB → 16 MB grow + chunk wrap 仕様、grow 頻度想定 (= 起動初期数 frame のみ)

---

## §2. 現状調査記録

### §2.1 code 現状 (8 項)

| # | 項目 | 現状 |
|---|------|------|
| 1 | `writeDrawUbo` grow 観測 site | `llvkloader.cpp:4824` `if (alloc.grew)` block + `LL_WARNS_ONCE` のみ、flag set なし |
| 2 | `flushDrawUbos` PC-N-1 hook placeholder | `llvkloader.cpp:4420-4424` comment-only placeholder = `if (sDrawUboRingBufferGrewThisFrame) { wireDrawUboSetV3aToRingBuffer(); sDrawUboRingBufferGrewThisFrame = false; }` 想定 |
| 3 | `wireDrawUboSetV3aToRingBuffer()` helper | `llvkloader.cpp:2658-2710` = 4 binding × UNIFORM_BUFFER_DYNAMIC + offset=0 + range=256 placeholder write、prerequisite missing 時 `LL_WARNS_ONCE` + `return false` (= graceful degrade) |
| 4 | 既存 wire 呼出 site | `llvkloader.cpp:3438` initVulkan 経路 1 度のみ |
| 5 | `beginFrame()` 構造 | `llvkloader.cpp:4022-4092` = `sFrameIndex` advance + `vkResetCommandBuffer` + `vkBeginCommandBuffer` + `vkCmdBeginRenderPass` |
| 6 | `endFrame()` 構造 | `llvkloader.cpp:4094-4113` = `vkCmdEndRenderPass` + `vkEndCommandBuffer` + `sInFrame=false` (line 4111) + `return true` (line 4112) |
| 7 | `flushDrawUbos` 呼出元 | `lldrawpool*.cpp` 14 site + `lldrawpoolmaterials.cpp:110` (= per-pool、per-frame 複数回呼出) |
| 8 | `sDrawUboRingBufferMgr->beginFrame()` | `flushFrameUbos` 内 line 4341 (= 1 frame 1 回 advance) |

### §2.2 `AllocateResult.grew` 仕様

`indra/llcommon/lluboringbuffer.h:82-89`:

```cpp
struct AllocateResult
{
    BufferHandle  buffer  = 0;
    std::uint32_t offset  = 0;
    std::uint32_t size    = 0;
    bool          success = false;
    bool          grew    = false; // 本 alloc 内で ring buffer grow が発火した
};
```

caller 観測責任 + 1 alloc 内 grow 発火確認可。

### §2.3 design 07 §7.5 grow 仕様

- 初期 4 MB → 8 MB → 16 MB grow (= max 16 MB)
- 起動初期数 frame のみ発火想定 (= dummy phase 数 KB/frame、grow 起きない)
- grow 後の VkBuffer は前 VkBuffer と別 instance、`sDrawUboSetV3a` は前 instance を参照する stale 状態 = `vkUpdateDescriptorSets` 再発火必要

---

## §3. ambiguity (N4-1)..(N4-10) 10 件 AYA literal「推奨案採用 OK」record (2026-06-05) + 採用根拠

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N4-1) | grow flag storage 位置 + 型 | **A**: anonymous namespace 内 file-static `std::atomic<bool> sDrawUboRingBufferGrewThisFrame{false}` (= `writeDrawUbo` で `store(true)`、hook で `exchange(false)`) | OK (2026-06-05) | 既存 LL_WARNS_ONCE site 流用最小、atomic で thread-safety、PC-N-1 hook placeholder comment (line 4420-4424) 同形、`LLUboRingBuffer` 改変回避 (= algorithm 層汚染なし) |
| (N4-2) | re-wire 発火 timing | **D**: `endFrame()` 末尾 (= `sInFrame=false` 直後、line 4111 後) | OK (2026-06-05) | `vkUpdateDescriptorSets` は command buffer recording 中の descriptor 更新は VUID 違反、`endFrame()` 末尾 = `vkEndCommandBuffer` 後 `sInFrame=false` 直後で recording 終了状態確定 + flushDrawUbos 全 14 site 終了後集約 + 次 frame submit/present に間に合う timing。A (= flushDrawUbos 末尾) は per-pool 14 回 redundant、C (= 次 beginFrame 先頭) は次 frame data の write 前という timing、B (= vkCmdEndRenderPass 後 vkEndCommandBuffer 前) は recording 中で VUID risk |
| (N4-3) | re-wire 失敗時 fallback | **A**: `LL_WARNS_ONCE` + 続行 (= 既存 `wireDrawUboSetV3aToRingBuffer()` pattern 同形) | OK (2026-06-05) | `wireDrawUboSetV3aToRingBuffer()` (line 2658-2710) 既に `LL_WARNS_ONCE` + `return false` pattern、graceful degrade、stale VkBuffer 参照は描画歪みのみで crash しない、MUSEUBO-A 整合 (= placeholder offscreen FBO 経路は別) |
| (N4-4) | wire helper 呼出 | **A**: 既存 `wireDrawUboSetV3aToRingBuffer()` 直接再呼出 (= PC-7ε helper 再利用) | OK (2026-06-05) | 同一 binding 構造 (= 4 binding × UNIFORM_BUFFER_DYNAMIC + offset=0 + range=256) を grow 後再適用するだけ、helper 重複は scope 拡大、PC-7ε helper の `LL_INFOS` marker が再発火確認 log 兼用可 |
| (N4-5) | grow 観測 site 範囲 | **A**: `writeDrawUbo` 内 1 箇所 (= 現状 LL_WARNS_ONCE site 流用) | OK (2026-06-05) | PC-N-1 で `recordPlaceholderPoolDraw` + PC-N-2 で `recordAvatarPlaceholderDraw` の allocate は両方 `writeDrawUbo` 経由化済、生 `sDrawUboRingBufferMgr->allocate` 呼出は 0 件 (= 後付け確認可)、B は重複、C は algorithm 層汚染 |
| (N4-6) | flag reset timing | **A**: re-wire 直後 `exchange(false)` (= hook 内 1 step、atomic read+reset) | OK (2026-06-05) | `exchange(false)` で atomic に read+reset、re-wire 成功失敗問わず reset (= 失敗時無限再試行回避)、failure は `LL_WARNS_ONCE` で 1 回のみ log で重複抑制、B (= 次 beginFrame 末尾 reset) は 2 hook 必要で複雑化 |
| (N4-7) | 同 frame 内 stale 描画許容 | **A**: 同 frame の grow 観測時 set=2 bind は stale VkBuffer 参照 → 1 frame 描画歪み許容 (= frame 末尾 re-wire、次 frame 以降正常) | OK (2026-06-05) | grow は initial 4 MB → 8 MB の 1 回 + 8 MB → 16 MB の 1 回 = 起動初期数 frame のみ発火想定 (= design 07 §7.5)、1 frame の歪み許容で構造単純化、C (= grow 観測時その frame の `bindV3aRigged` skip) は draw-time skip 判定複雑、B (= 同 frame 内即時 re-wire) は VUID 違反 |
| (N4-8) | PC-N-3 `sAvatarBoneStorage` re-wire との関係 | **A**: PC-N-4 scope は `sDrawUboSetV3a` のみ (= bone storage は PC-N-3 同形 path で別途実装) | OK (2026-06-05) | `feedback_ubo_migration_one_at_a_time` 厳格遵守、PC-N-3 未着手 (= bone storage ring buffer 未配線) で予測実装は推測コード、PC-N-3 design-lock 時に PC-N-4 hook の bone storage 拡張を別途設計 |
| (N4-9) | build verify scope | **A**: llrender + warning 0 + TUT 11+10+13 + codegen 131/131 (= PC-N-1 / PC-N-2 同形、Linux primary) | OK (2026-06-05) | (N1-9) A + (N2-8) A pattern 踏襲、cold launch verify は PC-8 集約、PC-N-4 単独 sub-step では build verify のみ |
| (N4-10) | PC-N-4 Exit Criteria 項目数 | **A**: 10 項 (= PC-N-1 / PC-N-2 同形 template) | OK (2026-06-05) | 一貫性 + template 流用 |

---

## §4. 実装計画 (a)-(g) 7 step

### (a) grow flag 新設

- 場所: anonymous namespace 内 (= `llvkloader.cpp:66` 内、既存 `sDrawUboRingBufferMgr` 近傍想定)
- 追加: `std::atomic<bool> sDrawUboRingBufferGrewThisFrame{false};`
- tag block: `<AYAstorm r41 PC-N-4 (a)>` literal「grow flag 新設 (= writeDrawUbo grow 観測 → endFrame() 末尾 hook 経由 re-wire)」comment 付記

### (b) `writeDrawUbo` 内 grow 観測時 flag set

- 場所: `llvkloader.cpp:4824-4835` (= `<AYAstorm r41 PC-N-1 (a)>` tag block 内 `if (alloc.grew)` block)
- 追加: `LL_WARNS_ONCE` 直後 (= 既存 message の後) に `sDrawUboRingBufferGrewThisFrame.store(true, std::memory_order_release);`
- tag block: `<AYAstorm r41 PC-N-4 (b)>` literal「grow flag set (= endFrame() 末尾 hook で re-wire 発火)」inline comment 付記

### (c) `endFrame()` 末尾 hook 配線

- 場所: `llvkloader.cpp:4111` (= `sInFrame=false` 直後)、`return true` 前
- 追加:
  ```cpp
  // <AYAstorm r41 PC-N-4 (c)> ring buffer grow 自動 re-wire hook。
  //   writeDrawUbo 内 alloc.grew 観測時に sDrawUboRingBufferGrewThisFrame set、
  //   本 hook で frame 末尾 (= vkEndCommandBuffer 後 sInFrame=false 直後) に
  //   wireDrawUboSetV3aToRingBuffer() 再呼出 = sDrawUboSetV3a を新 VkBuffer に再 wire。
  //   AYA literal「推奨案採用 OK」確認 2026-06-05、ambiguity (N4-2) D + (N4-4) A +
  //   (N4-6) A 採用:
  //     - (N4-2) D timing = endFrame 末尾 (= command buffer recording 終了状態
  //       確定 + flushDrawUbos 全 site 終了後集約)
  //     - (N4-4) A helper = 既存 wireDrawUboSetV3aToRingBuffer() 再呼出 (= PC-7ε
  //       helper 再利用、4 binding × UNIFORM_BUFFER_DYNAMIC + offset=0 + range=256
  //       同一 binding 構造を grow 後再適用)
  //     - (N4-6) A reset = exchange(false) で atomic に read+reset (= re-wire 成功
  //       失敗問わず reset、失敗は LL_WARNS_ONCE で重複抑制)
  if (sDrawUboRingBufferGrewThisFrame.exchange(false, std::memory_order_acq_rel))
  {
      static std::atomic<bool> s_first_rewire{true};
      if (s_first_rewire.exchange(false, std::memory_order_acq_rel))
      {
          LL_INFOS("Vulkan") << "PC-N-4 (c) endFrame: ring buffer grow detected, "
                                "re-wiring sDrawUboSetV3a to new VkBuffer (first fire)"
                             << LL_ENDL;
      }
      wireDrawUboSetV3aToRingBuffer();
  }
  // </AYAstorm r41 PC-N-4 (c)>
  ```

### (d) `flushDrawUbos` PC-N-1 placeholder comment 撤去 + 整合更新

- 場所: `llvkloader.cpp:4420-4424` (= PC-N-1 hook placeholder comment block)
- 改変: 旧「PC-N-4 持越 hook: if (sDrawUboRingBufferGrewThisFrame) { ... }」comment block を「PC-N-4 で `endFrame()` 経由実装済 (= ring buffer grow 自動 re-wire は frame 末尾集約、`flushDrawUbos` 経由 hook は不要)」literal 置換
- tag block: `<AYAstorm r41 PC-N-4 (d)>` literal で PC-N-1 (d) tag 範囲内に統合

### (e) `writeDrawUbo` LL_WARNS_ONCE comment 整合更新

- 場所: `llvkloader.cpp:4826-4828`
- 改変: 旧「sDrawUboSetV3a 再 wire は PC-N-4 持越し」+「自動 re-update は PC-N-4 持越」literal を「PC-N-4 で実装済 (= `endFrame()` 末尾 hook、`sDrawUboRingBufferGrewThisFrame` flag 経由)」literal 更新

### (f) build verify (= (N4-9) A)

- `cd build-linux-x86_64 && make -j4 llrender` PASS + ERROR 0 + WARNING 0
- `make -j4 INTEGRATION_TEST_lluboringbuffer INTEGRATION_TEST_llassetubopool INTEGRATION_TEST_llpipelinecachestorage` + 各 11/11 + 10/10 + 13/13 PASS
- `cd scripts/ubo_codegen && python3 -m unittest discover -s tests -v` 131/131 PASS

### (g) handoff complete doc 起案

- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-n-4-complete.md` 新規
- AYA commit 指示「commit してください」literal 受領後 commit (= `feedback_no_auto_commit` 遵守)

### §4.8 GATE-B 整合

`#ifdef LL_VULKAN_GLSL` 新規追加 0 件: 全改変は Vulkan-only code (= anonymous namespace + `endFrame()` + `writeDrawUbo`) 内、`#ifdef` 不要 (= `project_r41_phase1b_vulkan_host_gate` GATE-B 整合)。

### §4.9 MUSEUBO-A 整合

`mUseUBO=false` default 経路 100% 維持:

- grow flag は `writeDrawUbo` 内 set、`writeDrawUbo` は `forwardToUboUpload` PER_DRAW case `mUseUBO=true` 時のみ呼出 → `mUseUBO=false` で flag set されない → `endFrame()` hook は no-op (= flag false 維持)
- `wireDrawUboSetV3aToRingBuffer()` 失敗時 graceful degrade (= `LL_WARNS_ONCE` + 続行)
- stale 描画は 1 frame のみ許容 (= grow 起動初期数 frame、design 07 §7.5)

---

## §5. PC-N-4 design-lock Exit Criteria 9 項全充足

| # | Criteria | 充足 |
|---|----------|------|
| (i) | PC-N-4 literal scope 5 件 §0 明文化 | ✅ |
| (ii) | 必読 1 件 §1 + pinpoint reference 6 件別記 | ✅ |
| (iii) | ambiguity (N4-1)..(N4-10) 10 件 全 AYA literal「推奨案採用 OK」record (2026-06-05) §3 | ✅ |
| (iv) | 採用案根拠明文化 §3 | ✅ |
| (v) | 実装計画 (a)-(g) 7 step 分解 §4 | ✅ |
| (vi) | Exit Criteria 10 項明文化 §6 | ✅ |
| (vii) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 §4.8 | ✅ |
| (viii) | MUSEUBO-A 整合 = `mUseUBO=false` default 描画 100% 維持 §4.9 | ✅ |
| (ix) | `indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 = `feedback_design_phase_no_code_write` 整合 | ✅ |

---

## §6. PC-N-4 実装 phase Exit Criteria 10 項 (次 session 検証対象)

| # | Criteria |
|---|----------|
| (i) | grow flag 新設 = anonymous namespace 内 `std::atomic<bool> sDrawUboRingBufferGrewThisFrame{false}` ((N4-1) A) |
| (ii) | `writeDrawUbo` grow 観測時 flag set = `if (alloc.grew)` block 内 `store(true)` 追加 ((N4-5) A) |
| (iii) | `endFrame()` 末尾 hook 配線 = `sInFrame=false` 直後 `exchange(false)` + `wireDrawUboSetV3aToRingBuffer()` 再呼出 ((N4-2) D + (N4-4) A + (N4-6) A) |
| (iv) | `wireDrawUboSetV3aToRingBuffer()` re-wire 経路通電 = 既存 helper 4 binding × UNIFORM_BUFFER_DYNAMIC 再 wire 動作 ((N4-4) A) |
| (v) | flag reset = `exchange(false)` 直後 (= 失敗時無限再試行回避、failure は `LL_WARNS_ONCE`) ((N4-6) A + (N4-3) A) |
| (vi) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 |
| (vii) | MUSEUBO-A 整合 = `mUseUBO=false` default で flag set されず hook no-op、grow stale 1 frame 描画歪み許容 ((N4-7) A) |
| (viii) | build verify = `llrender` build + ERROR 0 / WARNING 0 + INTEGRATION_TEST 11+10+13 全 PASS + codegen unittest 131/131 PASS ((N4-9) A) |
| (ix) | tag block 統一 = `<AYAstorm r41 PC-N-4 (a)>` + `<AYAstorm r41 PC-N-4 (b)>` + `<AYAstorm r41 PC-N-4 (c)>` + `<AYAstorm r41 PC-N-4 (d)>` + first-rewire LL_INFOS marker `PC-N-4 (c) endFrame: ring buffer grow detected` |
| (x) | handoff complete doc 起案 + AYA commit 指示後 commit (= Co-Authored-By 不在 + 個別 file 指定 + 新 file 0 除 doc + CMake 改変 0 + settings.xml 改変 0 + codegen 改変 0) |

---

## §7. 着手手順 (次 session 実装 phase)

1. 本 PC-N-4 design-lock doc 全文 Read
2. pinpoint reference 6 件中必要分のみ Read (= 推奨は `writeDrawUbo` grow 観測 site + `endFrame()` 構造 + `wireDrawUboSetV3aToRingBuffer()` の 3 件)
3. step (a)-(g) 7 step 順次実装
4. build verify (= step (f)) 全 PASS 確認
5. complete handoff doc 起案 (= step (g))
6. self-verify 9 観点全 ✅ 確認
7. AYA commit 指示「commit してください」literal 受領後 `git add` 個別 file 指定 + commit (= `feedback_no_auto_commit` + `feedback_no_claude_coauthor` 遵守)
8. Phase 1.C 残 strict 線形: PC-N-3 design-lock (= `writeAvatarBoneStorage` helper 新設 + bone storage 経路再配線 H10-A 持越) → PC-N-3 実装 = **Phase 1.C complete**

---

## §8. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1 design-lock ✅ + PC-N-1 ✅ + PC-N-2 design-lock ✅ + PC-N-2 ✅ + **PC-N-4 design-lock ✅ 本 commit** + PC-N-4 実装 ⏳ 次 session + PC-N-3 design-lock ⏳ + PC-N-3 実装 ⏳ = Phase 1.C complete ⏳ + PC-8 (3 OS build verify) ⏳ + PC-N-5 = Phase 1.D 着手起点 ⏳

---

## §9. self-verify 9 観点 全 ✅

1. **PC-N-4 literal scope 5 件 §0 完全分解** (= grow flag 新設 + `writeDrawUbo` grow 観測時 flag set + `endFrame()` 末尾 hook 配線 + `flushDrawUbos` PC-N-1 placeholder 撤去 + `writeDrawUbo` LL_WARNS_ONCE comment 整合更新) ✅
2. **必読 1 件 §1 + pinpoint reference 6 件別記** (= PC-N-2 complete + `writeDrawUbo` grow 観測経路 + `flushDrawUbos` placeholder + `endFrame()` 構造 + `wireDrawUboSetV3aToRingBuffer()` helper + `AllocateResult.grew` 仕様 + design 07 §7.5) ✅
3. **現状調査 §2 8 項網羅** (= code 現状 8 項表 + `AllocateResult.grew` 仕様 + design 07 §7.5 grow 仕様) ✅
4. **ambiguity (N4-1)..(N4-10) 10 件 AYA literal「推奨案採用 OK」record (2026-06-05) §3** ✅
5. **採用根拠 10 件明文化 §3 表** ✅
6. **実装計画 (a)-(g) 7 step 分解 §4** ✅
7. **GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 §4.8** ✅
8. **MUSEUBO-A 整合 = `mUseUBO=false` default 経路不変 + grow stale 1 frame 描画歪み許容 §4.9** ✅
9. **Exit Criteria (i)-(ix) 9 項明文化 §5 + 実装 phase Exit Criteria 10 項 §6 + `indra/` 改変 0 件 = `feedback_design_phase_no_code_write` 整合** ✅

---

## §10. 次 session 着手 1 line

**PC-N-4 実装着手** = step (a)-(g) 7 step 実施 = (a) grow flag 新設 (= `std::atomic<bool> sDrawUboRingBufferGrewThisFrame{false}` anonymous namespace 内) + (b) `writeDrawUbo` 内 `if (alloc.grew)` block で `store(true)` + (c) `endFrame()` 末尾 (= `sInFrame=false` 直後) `exchange(false)` + `wireDrawUboSetV3aToRingBuffer()` 再呼出 + first-rewire LL_INFOS marker + (d) `flushDrawUbos` PC-N-1 placeholder comment 撤去 + 整合更新 + (e) `writeDrawUbo` LL_WARNS_ONCE comment 整合更新 + (f) build verify (= llrender + warning 0 + TUT 11+10+13 + codegen 131/131) + (g) handoff complete doc 起案 + Exit Criteria 10 項 self-verify + AYA commit 指示受領後 commit。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 design-lock handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 次 session 必読 1 件 + pinpoint reference 6 件別記、本 session も Read 4 file pinpoint のみ (= PC-N-2 complete doc + `writeDrawUbo` + `flushDrawUbos` + `lluboringbuffer.h` + `endFrame()` + `wireDrawUboSetV3aToRingBuffer()` + design 07 §7.5)、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §9
- **feedback_build_only_verified** 遵守 = design-lock phase は `indra/` 改変 0 件で build verify 対象外、実装 phase で literal 検証取得予定
- **feedback_no_scope_shrink** 遵守 = PC-N-4 literal scope 5 件 §0 完全分解、(N4-7) A (= 1 frame stale 描画歪み許容) + (N4-8) A (= bone storage は PC-N-3 持越) は AYA literal「推奨案採用 OK」record 済段階分離 = 縮小ではない (= `feedback_ubo_migration_one_at_a_time` 厳格遵守整合、PC-N-3 / PC-N-5 持越は design-lock phase 確定 record 済)
- **feedback_doubt_self_first** 遵守 = ambiguity 10 件発見で停止 + 推奨案提示 + AYA literal「推奨案採用 OK」確認後 design-lock doc 起案、推測実装なし
- **feedback_confirm_referent_before_acting** 遵守 = 10 件 batch AYA 確認 2026-06-05、(N4-2) timing 候補 4 件 (A/B/C/D) 全列挙 + 推奨案 D + 根拠明示 + AYA literal「OK」受領で確定、推測実装なし
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = PC-N-4 = ring buffer grow 自動 re-wire 単独 sub-step、PC-N-3 (bone storage 再配線) + PC-N-5 (実 GLTF avatar draw) は分離、本 doc 起案も PC-N-4 単独 design-lock のみ
- **feedback_design_phase_no_code_write** 整合 = 本 PC-N-4 design-lock phase は doc 起案のみ、`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit 予定
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「commit してください」literal 受領後 commit
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
- **feedback_no_bare_reference_ids** 遵守 = (N4-1)..(N4-10) 各 ID に項目名 / 採用案内容併記 §3 + (a)..(g) 各 step に作業内容併記 §4
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、`git add` 個別 file 指定 + `git add -A` 不使用予定

---
