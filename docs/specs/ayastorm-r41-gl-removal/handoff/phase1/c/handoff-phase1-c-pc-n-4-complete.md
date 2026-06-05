# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-N-4 complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-N-4 (= ring buffer grow 自動 re-wire = `writeDrawUbo` 内 `AllocateResult.grew` 観測時 file-static `std::atomic<bool>` flag set + `endFrame()` 末尾 (= `sInFrame=false` 直後) で `exchange(false)` + `wireDrawUboSetV3aToRingBuffer()` 再呼出 hook、`flushDrawUbos` PC-N-1 placeholder comment 撤去) の **実装 phase 完了** marker = step (a)-(g) 7 step 全実装 + Exit Criteria 10 項全充足。

> **本 doc 位置付け**: PC-N-4 design-lock (= `d1c68e077e`) §4 で確定の実装計画 (a)-(g) 7 step を本 session の fresh context で実装した実装 phase 完了 marker。次 strict 線形 = PC-N-3 design-lock (= `writeAvatarBoneStorage` helper 新設 + bone storage 経路再配線 H10-A 持越) → PC-N-3 実装 = **Phase 1.C complete**。

---

## §0. 本 session 着手契機 + 完了 scope record

**契機**: AYA 指示「PC-N-4 実装着手お願いします」literal 受領 (2026-06-05、PC-N-4 design-lock commit `d1c68e077e` 後の継続 session = 別 session の fresh context) + 必読 1 件 = `handoff-...-pc-n-4-design-lock.md` (= ambiguity (N4-1)..(N4-10) 10 件 AYA literal「推奨案採用 OK」record + 実装計画 (a)-(g) 7 step 分解 + Exit Criteria 10 項) Read + pinpoint reference 4 件 (`writeDrawUbo` grow 観測経路 `llvkloader.cpp:4824-4835` + `endFrame()` 構造 `llvkloader.cpp:4094-4113` + `wireDrawUboSetV3aToRingBuffer()` helper `llvkloader.cpp:2658-2710` + `flushDrawUbos` PC-N-1 placeholder `llvkloader.cpp:4420-4424`) pinpoint Read → step (a)-(g) 7 step 順次実装 → build verify (= llrender + warning 0 + TUT 11+10+13 + codegen 131/131 全 PASS) → 本 complete doc 起案。

**PC-N-4 literal scope 5 件 完了** (= design-lock §0 継承):

1. ✅ **grow flag 新設**: anonymous namespace 内 `std::atomic<bool> sDrawUboRingBufferGrewThisFrame{false}` 追加 ((N4-1) A) = `llvkloader.cpp:420-427` tag block
2. ✅ **`writeDrawUbo` 内 grow 観測時 flag set**: `if (alloc.grew)` block 内 `LL_WARNS_ONCE` 直後に `sDrawUboRingBufferGrewThisFrame.store(true, std::memory_order_release)` 追加 ((N4-5) A) = `llvkloader.cpp:4862-4876` tag block
3. ✅ **`endFrame()` 末尾 hook 配線**: `sInFrame=false` 直後に `exchange(false, std::memory_order_acq_rel)` + `wireDrawUboSetV3aToRingBuffer()` 再呼出 + first-rewire LL_INFOS marker ((N4-2) D + (N4-4) A + (N4-6) A) = `llvkloader.cpp:4122-4148` tag block
4. ✅ **`flushDrawUbos` PC-N-1 placeholder comment 撤去**: 旧「PC-N-4 持越 hook: if (sDrawUboRingBufferGrewThisFrame) { ... }」comment block を「PC-N-4 で `endFrame()` 経由実装済」literal 置換 ((N4-2) D 整合) = `llvkloader.cpp:4442-4448` tag block
5. ✅ **`writeDrawUbo` 内 LL_WARNS_ONCE comment 整合更新**: 旧「自動 re-update は PC-N-4 持越」literal を「PC-N-4 で実装済 (= `endFrame()` 末尾 hook、`sDrawUboRingBufferGrewThisFrame` flag 経由)」literal 更新 + 関数 header comment (`llvkloader.cpp:4806-4808`) も整合 update

---

## §1. 必読 1 件 + pinpoint reference (次 session = PC-N-3 design-lock 向け)

**次 session 必読**:

1. **本 PC-N-4 complete doc 全文**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-n-4-complete.md`

**pinpoint reference** (PC-N-3 design-lock で必要分のみ):

- **PC-N decomposition design-lock**: `handoff-...-pc-n-decomposition-design-lock.md` §4.4 = PC-N-3 scope 定義 (= `writeAvatarBoneStorage` helper 新設 + bone storage 経路再配線 H10-A 持越)
- **PC-N-2 complete doc**: `handoff-...-pc-n-2-complete.md` = `bindV3aRigged` set=2 復活完了状態 + `recordAvatarPlaceholderDraw` allocate-chain 配線 (= PC-N-3 着手 baseline)
- **本 PC-N-4 complete doc §2 / §3**: grow re-wire hook 実装済状態 (= PC-N-3 で bone storage 同形 path 設計時の参考)
- **`wireDrawUboSetV3aToRingBuffer()` helper**: `indra/llrender/llvkloader.cpp:2658-2710` (= PC-7ε helper) + 本 PC-N-4 で grow 後再呼出経路通電済
- **avatar bone storage 現状**: `indra/llrender/llvkloader.cpp` 内 H10-A push descriptor disable + `sAvatarBoneLayout` = `sAYAStandardLayout` alias 共用構造 (= PC-N-3 で `writeAvatarBoneStorage` helper 新設 + set=3 経由再 wire 持越)

---

## §2. 実装内容 (step (a)-(g) 7 step 全実装)

### §2.1 編集 1 (llvkloader.cpp +60 / -17)

| step | 内容 | 場所 | 行数増減 |
|------|------|------|---------|
| (a) | grow flag 新設 = `std::atomic<bool> sDrawUboRingBufferGrewThisFrame{false}` anonymous namespace 内 | line 420-427 | +9 |
| (b) | `writeDrawUbo` 内 `if (alloc.grew)` block で `store(true, memory_order_release)` + comment 整合更新 | line 4862-4876 | +13 / -5 |
| (c) | `endFrame()` 末尾 (= `sInFrame=false` 直後) `exchange(false, memory_order_acq_rel)` + `wireDrawUboSetV3aToRingBuffer()` 再呼出 + first-rewire LL_INFOS marker + Doxygen-style 14 行 comment | line 4122-4148 | +29 |
| (d) | `flushDrawUbos` PC-N-1 placeholder comment 撤去 + 整合更新 = 旧「PC-N-4 持越 hook」placeholder comment block を「PC-N-4 で `endFrame()` 経由実装済」literal 置換 + first-fire LL_INFOS marker 更新 | line 4442-4458 | +6 / -7 |
| (e) | `writeDrawUbo` LL_WARNS_ONCE comment 整合更新 = (b) と同 site で実施 + 関数 header comment (`llvkloader.cpp:4806-4808`) も整合 update (= integrity fix) | line 4806-4808, 4862-4876 | +3 / -5 |

**累計**: +60 insertions / -17 deletions (= `git diff --stat indra/llrender/llvkloader.cpp` 検証取得)

### §2.2 .h 改変 0 件

- 全改変は `llvkloader.cpp` 内匿名 namespace + `endFrame()` (= `LLVKLoader` namespace 直下) + `writeDrawUbo` (= `LLVKLoader` namespace 直下) 内
- `wireDrawUboSetV3aToRingBuffer()` は匿名 namespace 内 helper、`LLVKLoader::endFrame()` から unqualified lookup で到達可 (= 既存 `initVulkan` line 3447 で同形動作実証済)
- `sDrawUboRingBufferGrewThisFrame` は匿名 namespace 内 file-static、`writeDrawUbo` + `endFrame()` 両 site から unqualified lookup で参照可
- `llvkloader.h` 公開 API 不変

### §2.3 GATE-B 整合

`#ifdef LL_VULKAN_GLSL` 新規追加 0 件:

- (a) grow flag は anonymous namespace 内 C++ 変数 (= GLSL 関係なし)
- (b) `writeDrawUbo` 内 store call は host C++ Vulkan path 専有
- (c) `endFrame()` 末尾 hook も host C++ Vulkan path 専有
- (d) `flushDrawUbos` comment 更新は no-code 改変

= `project_r41_phase1b_vulkan_host_gate` GATE-B (= host C++ は `mUseUBO` runtime flag のみで gate) 整合

### §2.4 MUSEUBO-A 整合

`mUseUBO=false` default 描画 100% 維持:

- grow flag は `writeDrawUbo` 内 `if (alloc.grew)` block 内で set、`writeDrawUbo` は `forwardToUboUpload` PER_DRAW case (`mUseUBO=true` 時のみ) からのみ呼出
  → `mUseUBO=false` default で flag set されない → `endFrame()` hook 内 `exchange(false)` は false 返却 → `wireDrawUboSetV3aToRingBuffer()` 未呼出 = no-op
- `wireDrawUboSetV3aToRingBuffer()` 失敗時 graceful degrade (= `LL_WARNS_ONCE` + `return false`、`endFrame()` 続行) ((N4-3) A)
- stale 描画は 1 frame のみ許容 ((N4-7) A) = grow は initial 4 MB → 8 MB + 8 MB → 16 MB の起動初期数 frame のみ発火想定 (= design 07 §7.5)、次 frame 以降は新 VkBuffer 参照で正常描画

---

## §3. build verify (= step (f))

| # | check | 結果 |
|---|-------|------|
| 1 | `cd build-linux-x86_64 && make -j4 llrender` | ✅ PASS (= `[100%] Built target llrender`) |
| 2 | ERROR 0 + WARNING 0 | ✅ (= `grep -iE "warning:|error:"` 0 件) |
| 3 | `INTEGRATION_TEST_lluboringbuffer` | ✅ 11/11 PASS YAY (= PC-3 algorithm 層 regression なし) |
| 4 | `INTEGRATION_TEST_llassetubopool` | ✅ 10/10 PASS YAY (= PC-4 algorithm 層 regression なし) |
| 5 | `INTEGRATION_TEST_llpipelinecachestorage` | ✅ 13/13 PASS YAY (= PC-5 algorithm 層 regression なし) |
| 6 | `scripts/ubo_codegen` unittest | ✅ 131/131 PASS (= Phase 1.A / 1.B / 1.C PC-1..PC-N-2 regression なし) |

= (N4-9) A 採用 build verify scope literal 取得済。

---

## §4. PC-N-4 実装 phase Exit Criteria 10 項全充足

| # | Criteria | 充足 |
|---|----------|------|
| (i) | grow flag 新設 = anonymous namespace 内 `std::atomic<bool> sDrawUboRingBufferGrewThisFrame{false}` ((N4-1) A) | ✅ line 420-427 |
| (ii) | `writeDrawUbo` grow 観測時 flag set = `if (alloc.grew)` block 内 `store(true, memory_order_release)` 追加 ((N4-5) A) | ✅ line 4862-4876 |
| (iii) | `endFrame()` 末尾 hook 配線 = `sInFrame=false` 直後 `exchange(false, memory_order_acq_rel)` + `wireDrawUboSetV3aToRingBuffer()` 再呼出 ((N4-2) D + (N4-4) A + (N4-6) A) | ✅ line 4122-4148 |
| (iv) | `wireDrawUboSetV3aToRingBuffer()` re-wire 経路通電 = 既存 helper (= PC-7ε line 2658-2710) 4 binding × UNIFORM_BUFFER_DYNAMIC 再 wire 動作 ((N4-4) A) | ✅ 既存 helper unmodified 再利用、tag block PC-N-4 (c) で呼出 |
| (v) | flag reset = `exchange(false)` で atomic read+reset (= 失敗時無限再試行回避、failure は `LL_WARNS_ONCE` で重複抑制) ((N4-6) A + (N4-3) A) | ✅ line 4136 `exchange(false, std::memory_order_acq_rel)` |
| (vi) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 | ✅ §2.3 |
| (vii) | MUSEUBO-A 整合 = `mUseUBO=false` default で flag set されず hook no-op、grow stale 1 frame 描画歪み許容 ((N4-7) A) | ✅ §2.4 |
| (viii) | build verify = `llrender` build + ERROR 0 / WARNING 0 + INTEGRATION_TEST 11+10+13 全 PASS + codegen unittest 131/131 PASS ((N4-9) A) | ✅ §3 |
| (ix) | tag block 統一 = `<AYAstorm r41 PC-N-4 (a)>` + `<AYAstorm r41 PC-N-4 (b)>` + `<AYAstorm r41 PC-N-4 (c)>` + `<AYAstorm r41 PC-N-4 (d)>` + first-rewire LL_INFOS marker `PC-N-4 (c) endFrame: ring buffer grow detected` | ✅ tag 4 ペア全配置 + LL_INFOS marker line 4142 |
| (x) | handoff complete doc 起案 + AYA commit 指示後 commit (= Co-Authored-By 不在 + 個別 file 指定 + 新 file 0 除 doc + CMake 改変 0 + settings.xml 改変 0 + codegen 改変 0) | ✅ 本 doc + commit pending AYA 指示 |

---

## §5. 残 strict 線形

PC-N-3 design-lock (= 次 session、`writeAvatarBoneStorage` helper 新設 + bone storage 経路再配線 H10-A 持越) → PC-N-3 実装 = **Phase 1.C complete** → PC-8 (= 3 OS build verify、Linux primary + Win/Mac AYA 環境依頼) → PC-N-5 = **Phase 1.D 着手起点** (= 実 GLTF Vulkan draw 通電 1 stub)。

---

## §6. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1 design-lock ✅ + PC-N-1 ✅ + PC-N-2 design-lock ✅ + PC-N-2 ✅ + PC-N-4 design-lock ✅ + **PC-N-4 ✅ 本 commit** + PC-N-3 design-lock ⏳ 次 session + PC-N-3 実装 ⏳ = Phase 1.C complete ⏳ + PC-8 ⏳ + PC-N-5 = Phase 1.D 着手起点 ⏳

---

## §7. self-verify 9 観点 全 ✅

1. **Exit Criteria 10 項全充足** §4 ✅
2. **必読 1 件 (本 complete handoff doc) §1 + pinpoint reference 5 件別記** (= PC-N decomposition design-lock §4.4 + PC-N-2 complete doc + 本 PC-N-4 complete doc §2/§3 + `wireDrawUboSetV3aToRingBuffer()` helper + avatar bone storage 現状) ✅
3. **step (a)..(g) 7 step 全実装** = (a) grow flag 新設 line 420-427 + (b) `writeDrawUbo` flag set line 4862-4876 + (c) `endFrame()` 末尾 hook line 4122-4148 + (d) `flushDrawUbos` placeholder 撤去 line 4442-4458 + (e) `writeDrawUbo` comment 整合更新 (b) と同 site + integrity fix line 4806-4808 + (f) build verify §3 + (g) 本 doc 起案 ✅
4. **ambiguity (N4-1)..(N4-10) 9 件 AYA literal「推奨案採用 OK」record (2026-06-05) §3 design-lock 継承** ✅ (= design-lock commit `d1c68e077e` §3 全 9 件 OK record 済)
5. **GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件** §2.3 ✅
6. **MUSEUBO-A 整合 = `mUseUBO=false` default 経路不変** (= `writeDrawUbo` は `forwardToUboUpload` PER_DRAW case `mUseUBO=true` 時のみ呼出 → `mUseUBO=false` で flag 未 set → `endFrame()` hook no-op、`wireDrawUboSetV3aToRingBuffer()` 失敗時 graceful degrade、stale 描画 1 frame 許容) §2.4 ✅
7. **llrender build + WARNING 0 + TUT 11+10+13 + codegen 131/131 全 PASS** §3 ✅
8. **commit 内容** = 1 modified (`llvkloader.cpp` +60/-17) + 1 new doc (handoff complete) + 新 file 0 (除 doc) + CMake 改変 0 + settings.xml 改変 0 + codegen 改変 0 + shader 改変 0 + Co-Authored-By 不在 ✅
9. **feedback_no_scope_shrink 遵守** = PC-N-4 literal scope 5 件 §0 全件実施、(N4-7) A (= 1 frame stale 描画歪み許容) + (N4-8) A (= bone storage は PC-N-3 持越) は AYA literal「OK」record 済段階分離 = 縮小ではない、line 4806-4808 関数 header comment integrity update は (e) 整合の minor 拡張 (= 縮小でも禁止 scope 拡張でもない) ✅

---

## §8. 引き継ぎ memory 14 件

- (本 session で memory 改変 0 件、既存 14 件参照のみ)
- 関連 memory: `project_ayastorm_r41_vulkan_migration` (= r41 章 milestone pointer) + `project_r41_phase1b_vulkan_host_gate` (= GATE-B literal) + `feedback_ubo_migration_one_at_a_time` (= UBO 化 1 つずつ厳格) + `feedback_no_scope_shrink` (= literal scope 完全実施) + `feedback_self_verify_before_handoff` (= 9 観点 self-verify) + `feedback_build_only_verified` (= literal 検証取得) + `feedback_doubt_self_first` (= design-lock phase で ambiguity 10 件発見済) + `feedback_confirm_referent_before_acting` (= design-lock phase で 10 件 batch AYA 確認済) + `feedback_design_phase_no_code_write` (= 本 session は実装 phase ゆえ `indra/` 改変は正規) + `feedback_release_branch_workflow` (= feature branch 上 commit) + `feedback_no_auto_commit` (= AYA commit 指示後 commit) + `feedback_no_claude_coauthor` (= Co-Authored-By 不在) + `feedback_no_bare_reference_ids` (= (N4-1)..(N4-10) 各 ID に項目名併記) + `feedback_tests_dir_never_commit` (= `tests/` 改変 0 件)

---

## §9. 次 session 着手 1 line

**PC-N-3 design-lock 着手** = `writeAvatarBoneStorage` helper 新設 + bone storage 経路再配線 H10-A 持越 (= push descriptor disable 状態下で set=3 経由再 wire + bone storage write helper 新設)、design-lock phase 着手要 (= 本 commit 完了後、別 session の fresh context で ambiguity 確認 + 実装計画分解後の実装 phase)。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 complete handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 必読 1 件 + pinpoint reference 5 件別記、本 session も Read 4 file pinpoint のみ (= design-lock doc + `writeDrawUbo` + `endFrame()` + `wireDrawUboSetV3aToRingBuffer()` + `flushDrawUbos` placeholder)、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §7
- **feedback_build_only_verified** 遵守 = llrender build + WARNING 0 + TUT 11/11 + 10/10 + 13/13 + codegen 131/131 で literal 検証取得
- **feedback_no_scope_shrink** 遵守 = PC-N-4 literal scope 5 件 §0 全件実施、line 4806-4808 関数 header comment integrity update は (e) 整合の minor 拡張 (= 縮小ではない)
- **feedback_doubt_self_first** 遵守 = design-lock phase で ambiguity 10 件発見 + 推奨案提示 + AYA literal「推奨案採用 OK」確認後本実装、推測実装なし
- **feedback_confirm_referent_before_acting** 遵守 = 10 件 batch AYA 確認 design-lock phase で完了
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = PC-N-4 = ring buffer grow 自動 re-wire 単独 sub-step、PC-N-3 (bone storage 再配線) + PC-N-5 (実 GLTF avatar draw) は分離
- **feedback_design_phase_no_code_write** 整合 = 本 PC-N-4 は実装 phase = design-lock commit `d1c68e077e` で `indra/` 改変 0 件 完了済、本 session で `indra/` 改変は実装 phase ゆえ整合
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「commit してください」literal 受領後 commit
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
- **feedback_no_bare_reference_ids** 遵守 = (N4-1)..(N4-10) 各 ID に項目名 / 採用案内容併記 + (a)..(g) 各 step に作業内容併記
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、`git add` 個別 file 指定 + `git add -A` 不使用予定

---
