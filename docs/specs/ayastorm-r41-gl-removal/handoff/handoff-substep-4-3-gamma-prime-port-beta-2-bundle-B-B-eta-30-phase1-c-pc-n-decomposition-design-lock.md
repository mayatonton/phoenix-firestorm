# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-N decomposition design-lock complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-N (= Phase 1.C complete marker = 5 要素集約 sub-step) を **PC-N-1..PC-N-5 5 sub-step に分解** する design-lock phase 完了 marker = ambiguity (N-1)..(N-9) 9 件 全 AYA literal「Claude 推奨案 OK」record (2026-06-05) + 各 sub-step 概略 + 依存関係 + 着手順序 + 全体 Exit Criteria 明文化。`indra/` 改変 0 件 (= `feedback_design_phase_no_code_write` 整合)。

> **本 doc 位置付け**: PC-N **全体分解** の overview。各 PC-N-? sub-step の詳細実装 design-lock は当該 sub-step 着手時に **別 session で個別起案** (= `feedback_ubo_migration_one_at_a_time` 厳格遵守、5 sub-step を一括設計しない)。本 doc は分解粒度 + 依存関係 + 着手順序の固定のみ。

---

## §0. 本 session 着手契機 + literal scope record

**契機**: AYA 指示「PC-7ε complete 後、3 候補 (PC-7α'' / PC-8 / PC-N) から推奨で」literal 受領 (2026-06-05) → Claude 推奨案 **PC-N 分解 design-lock phase** 提示 → AYA literal「OK」受領 (2026-06-05) で本 session 着手 → 現状調査 (Explore agent 9 項) → ambiguity 9 件発見 → AYA literal「Claude 推奨案 OK」一括確認受領 (2026-06-05) で本 design-lock doc 起案。

**PC-N 分解 literal scope** (= 5 sub-step):

1. **PC-N-1** = `flushDrawUbos` real per-draw data write 通電 (= PC-6ε-3 持越) = `forwardToUboUpload` PER_DRAW case 通電 + `writeDrawUbo` helper 新設 + `recordPlaceholderPoolDraw` dummy zero memset → real write 経由置換
2. **PC-N-2** = `bindV3aRigged` set=2 復活 = signature 拡張 (`const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS]` 引数化、PC-7ε パターン) + `recordAvatarPlaceholderDraw` 内 allocate-chain 配線
3. **PC-N-3** = avatar bone storage 経路再配線 (= H10-A 持越) = `writeAvatarBoneStorage` helper 新設 + setter ↔ flush 2 経路統合 + push descriptor 経路 disable → set=3 経由再配線
4. **PC-N-4** = ring buffer grow 自動 re-wire = `AllocateResult.grew` 観測 frame 末尾で `vkUpdateDescriptorSets` 再発火 (= deferred、`flushDrawUbos` 後)
5. **PC-N-5** = 実 GLTF Vulkan draw 通電 (= 1 GLTF asset draw stub、最小通電、Phase 1.D 着手起点)

**Phase 境界**: PC-N-1..PC-N-4 完了 = **Phase 1.C complete** marker、PC-N-5 = Phase 1.D 着手起点 (= (N-8) B 採用)。

---

## §1. 必読 1 件 + pinpoint reference

**次 session 必読 (= PC-N-1 design-lock phase 着手前)**:

1. **本 PC-N decomposition design-lock doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-n-decomposition-design-lock.md`

**pinpoint reference (各 PC-N-? design-lock phase 着手時に必要分のみ)**:

- **PC-7ε complete doc**: `handoff-...-pc-7-epsilon-complete.md` = dynamic offset hand-off 通電 + PC-7ε パターン (= signature 拡張 + allocate-chain) の参考実装
- **design 07 §6 (ring buffer growth)**: `design/07-vulkan-api-state.md:309-360` = PC-N-4 source
- **design 07 §7 (dynamic offset)**: `design/07-vulkan-api-state.md:361-400` = PC-N-1/2 source
- **design 07 §8 (per-draw write path)**: `design/07-vulkan-api-state.md:§8` = PC-N-1 source
- **design 06b §5.3 (L1+L2 dynamic offset)**: `design/06b-cadence-update-site-and-dirty.md:§5.3` = PC-N-1 + PC-N-2 path 整合
- **design 06c §2.3 (set=1a/1b split)**: `design/06c-descriptor-set-bind-wiring.md:§2.3` = bind 経路整合確認
- **lluboringbuffer.h public API**: `indra/llcommon/lluboringbuffer.h:82-130` = `AllocateResult` struct + `tryGrow()` + `getBuffer()` + `beginFrame()`

---

## §2. 現状調査結果 (= Explore agent 9 項要約)

### §2.1 現 code 状態 (indra/)

| # | 項目 | file:line | 現状要約 |
|---|------|-----------|---------|
| 1 | `bindV3aRigged` | `llvkloader.cpp:1781-1810` | rigged path で set=0/1a/1b/3 bind (= set=2 skip)、dynamic_offsets 引数なし、PC-7ε (e) comment で PC-N 持越記録あり |
| 2 | `flushDrawUbos` | `llvkloader.cpp:4376-4383` | `flushDummyUboWrite("flushDrawUbos")` dummy 実装、real per-draw write 未通電 |
| 3 | `recordAvatarPlaceholderDraw` | `llvkloader.cpp:5093-5146` | `bindV3aRigged` 呼出 (set=2 skip + set=3 swap)、push descriptor 経路 disable 済、bone storage 再配線は PC-N 持越 |
| 4 | ring buffer grow path | `lluboringbuffer.cpp:87-130` + `.h:82-89` | `tryGrow()` 動作中、`AllocateResult.grew` flag 返却済、grow 観測時 `LL_WARNS_ONCE` のみ (= PC-7ε (ε-6) A 採用) |
| 5 | `forwardToUboUpload` PER_DRAW | `llglslshader.cpp:2151-2157` | `LL_WARNS_ONCE` diagnostic only、SINGLETON case (PC-7δ で通電) と対比、未通電 |
| 6 | `sAvatarBoneStorageBuffer` | `llvkloader.cpp:107-109 + 3112-3180 + 3677-3683` | VkBuffer + 7040 B prealloc 済 (mat4×110)、bone data write path 0 件 = placeholder identity matrix のみ |
| 7 | 実 GLTF Vulkan draw 経路 | grep 結果 0 件 | 完全未実装、placeholder offscreen FBO 経路のみ、PC-N 新規構成要素 |

### §2.2 design doc 章

| # | 章 | 出典 | 該当 PC-N-? |
|---|----|------|------------|
| 8 | ring buffer growth + dynamic offset + per-draw write | `design/07 §6 + §7 + §8` (line 309-406) | PC-N-1 / PC-N-2 / PC-N-4 |
| 9 | set=1a/1b split + avatar bone storage 帯 | `design/06c §2.3` + `design/06b §5.3` | PC-N-2 / PC-N-3 整合確認 |

### §2.3 PC-N 5 要素の依存関係

```
PC-N-1 (flushDrawUbos real write)
    ↓ (real write が ring buffer に入って初めて set=2 dynamic offset bind の意味発生)
PC-N-2 (bindV3aRigged set=2 復活)
    ↓ (set=2 通電中の grow 障害回避のため safety net 早期構築)
PC-N-4 (ring buffer grow auto re-wire)
    ↓ (per-draw write + bind 経路安定後、bone storage 通電)
PC-N-3 (avatar bone storage 再配線)
    ↓ (全 infrastructure 完成後、最終通電)
PC-N-5 (実 GLTF Vulkan draw 通電)  ← Phase 1.D 着手起点
```

---

## §3. ambiguity (N-1)..(N-9) 9 件 AYA literal「Claude 推奨案 OK」record (2026-06-05) + 採用根拠

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N-1) | PC-N 分解粒度 | **A**: 5 sub-step (PC-N-1..PC-N-5) 各独立 | OK (2026-06-05) | `feedback_ubo_migration_one_at_a_time` 厳格遵守、各 sub-step 単独 testable + build verify 独立 |
| (N-2) | 着手順序 | **A**: PC-N-1 → -2 → -4 → -3 → -5 | OK (2026-06-05) | 依存順、PC-N-4 を PC-N-2 直後に置くことで set=2 通電後すぐ grow safety net 構築 (= bind 経路通電中の grow 障害回避) |
| (N-3) | PC-N-1 scope | **B**: `forwardToUboUpload` PER_DRAW case 通電 + `writeDrawUbo` helper 新設 + 既存 dummy zero memset → real write 置換 | OK (2026-06-05) | PC-7ε dummy memset 残存は検証経路不明、real write 通電と同 sub-step で置換が論理整合 |
| (N-4) | PC-N-2 signature 戦略 | **A**: `bindV3aRigged` signature 拡張 = `const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS]` 引数化 (= bindV3aStatic 同形、PC-7ε パターン) | OK (2026-06-05) | caller 責任分離一貫性、bindV3aStatic との対称、test 容易性 |
| (N-5) | PC-N-3 write 経路 | **A**: `writeAvatarBoneStorage` helper 新設 + setter ↔ flush 2 経路統合 (= writeFrameUbo / writeSingletonUbo パターン) | OK (2026-06-05) | 既存 `write*Ubo` パターン統一、register-once + bind-many 整合 |
| (N-6) | PC-N-4 timing | **A**: `AllocateResult.grew` 観測 frame 末尾で `vkUpdateDescriptorSets` 再発火 (= deferred、`flushDrawUbos` 後) | OK (2026-06-05) | frame 中 `vkUpdateDescriptorSets` は同期 hazard、deferred で beginFrame 境界 timing 安全 |
| (N-7) | PC-N-5 scope 単位 | **A**: 1 GLTF asset draw stub (= 最小通電 + 既存 placeholder 並走) | OK (2026-06-05) | `feedback_ubo_migration_one_at_a_time`、最小 stub で経路通電確認後、Phase 1.D で本格拡張 |
| (N-8) | Phase 1.C complete marker timing | **B**: PC-N-1..PC-N-4 完了 = Phase 1.C complete、PC-N-5 は Phase 1.D 着手起点 | OK (2026-06-05) | 実 GLTF draw = 描画通電 = Phase 1.D 本論、Phase 1.C = UBO/descriptor infrastructure 完成までが論理境界 |
| (N-9) | PC-8 (3 OS build verify) と PC-N の順序 | **A**: PC-N 全 sub-step (PC-N-1..-4) 完了後に PC-8 = 最終 gate | OK (2026-06-05) | Linux primary は各 sub-step で build verify 取得、Win/Mac AYA 環境依頼は Phase 1.C complete marker で集約効率的 |

---

## §4. PC-N-1..PC-N-5 各 sub-step 概略

> **注**: 各 sub-step の **詳細 step (a)-(g)** + **ambiguity 確認** + **Exit Criteria** は当該 sub-step 着手時に **別 session で個別 design-lock 起案** (= `feedback_ubo_migration_one_at_a_time` + `feedback_design_phase_no_code_write` 厳格遵守)。本 §4 は overview のみ。

### §4.1 PC-N-1 = `flushDrawUbos` real per-draw data write 通電 (= PC-6ε-3 持越)

- **scope**: `forwardToUboUpload` PER_DRAW case 通電 + `writeDrawUbo(U32 block_hash, U32 offset, const void* data, size_t size)` helper 新設 + `recordPlaceholderPoolDraw` 内 dummy zero memset → real write 経由置換
- **想定改変 file**: `llvkloader.cpp` + `llvkloader.h` + `llglslshader.cpp`
- **想定 design-lock ambiguity** (= 着手時に確認): `writeDrawUbo` 内部 ring buffer allocate timing (= setter 内 or flush 内)、PER_DRAW key (= block_hash 単独 or block_hash + draw_id)、dirty 管理粒度
- **依存**: PC-7ε 完了 (= ring buffer chunk hand-off 通電済) ✅
- **後続**: PC-N-2 (set=2 復活) が PC-N-1 の real write を受け取る

### §4.2 PC-N-2 = `bindV3aRigged` set=2 復活

- **scope**: `bindV3aRigged` signature 拡張 = `const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS]` 引数化 (= bindV3aStatic 同形) + set=2 skip 削除 + `recordAvatarPlaceholderDraw` 内 allocate-chain 配線 (= PC-7ε `recordPlaceholderPoolDraw` パターン同形)
- **想定改変 file**: `llvkloader.cpp` (= `bindV3aRigged` + `recordAvatarPlaceholderDraw`)
- **想定 design-lock ambiguity**: set=2 binding count (= 4 with rigged path?)、avatar 用 dummy data 内容 (= zero memset or identity matrix)、set=3 swap timing との連動
- **依存**: PC-N-1 (real write 通電) → PC-N-2
- **後続**: PC-N-4 (grow safety net) を直後

### §4.3 PC-N-4 = ring buffer grow 自動 re-wire

- **scope**: `AllocateResult.grew` 観測 frame 末尾で `vkUpdateDescriptorSets` 再発火 (= deferred、`flushDrawUbos` 後)。PC-7ε の `LL_WARNS_ONCE` のみ → 自動 re-wire path 通電
- **想定改変 file**: `llvkloader.cpp` (= `flushDrawUbos` 末尾 or `endFrame` 周辺 + `wireDrawUboSetV3aToRingBuffer` 再呼出 path)
- **想定 design-lock ambiguity**: deferred timing precise location (= `flushDrawUbos` 末尾 or `vkCmdEndRenderPass` 後 or `endFrame()`)、re-wire 失敗時 fallback (= LL_WARNS_ONCE + 続行 or fatal)
- **依存**: PC-N-2 (set=2 復活) → PC-N-4 (= set=2 経路通電中の grow safety net)
- **後続**: PC-N-3 (bone storage) に進行

### §4.4 PC-N-3 = avatar bone storage 経路再配線 (= H10-A 持越)

- **scope**: `writeAvatarBoneStorage(U32 skin_hash, const mat4* matrices, size_t count)` helper 新設 + setter ↔ flush 2 経路統合 (= `writeFrameUbo` / `writeSingletonUbo` パターン同形) + push descriptor 経路完全 disable → set=3 経由再配線
- **想定改変 file**: `llvkloader.cpp` + `llvkloader.h` + (avatar bone matrix 計算経路)
- **想定 design-lock ambiguity**: bone storage ring buffer (= 専用 or sDrawUboRingBuffer 共用)、key (= skin_hash or per-avatar id)、mat4 layout (= row-major or column-major)
- **依存**: PC-N-1 + PC-N-2 + PC-N-4 (= write/bind/grow infrastructure) → PC-N-3
- **後続**: PC-N-5 (= 実 GLTF avatar draw 通電) に進行

### §4.5 PC-N-5 = 実 GLTF Vulkan draw 通電 (1 stub) = Phase 1.D 着手起点

- **scope**: 1 GLTF asset draw stub 経路新設 + 既存 placeholder offscreen FBO 経路と並走 + 最小 1 mesh / 1 material / 1 draw call で経路通電確認
- **想定改変 file**: `LLDrawPoolMaterials` (or 類似 GLTF render path) + `llvkloader.cpp` 経由 Vulkan dispatch
- **想定 design-lock ambiguity**: GLTF asset 選定 (= 起動時固定 stub asset or 既存 inventory 1 件)、placeholder 経路との切替条件 (= debug cvar or build-time flag)、Phase 1.D scope 境界
- **依存**: PC-N-1 + PC-N-2 + PC-N-3 + PC-N-4 (= 全 infrastructure 完成) → PC-N-5
- **後続**: Phase 1.D 着手起点 = 本 sub-step は Phase 1.C 外、Phase 1.C complete marker は PC-N-4 完了時点

### §4.6 GATE-B 整合 (全 sub-step 共通)

`#ifdef LL_VULKAN_GLSL` 新規追加 0 件 = `project_r41_phase1b_vulkan_host_gate` 遵守。host 側 redirect 層は `mUseUBO` runtime flag のみで gate (= GATE-B 確定 2026-06-04)。

### §4.7 MUSEUBO-A 整合 (全 sub-step 共通)

`mUseUBO=false` default で既存 OpenGL 描画 100% 維持。本 PC-N 5 sub-step は全て:
- `recordPlaceholderPoolDraw` / `recordAvatarPlaceholderDraw` (= placeholder offscreen FBO 経路、視覚 no-op 等価維持)
- `llvkloader.cpp` 内 Vulkan initVulkan 経路 (= mUseUBO 不問、起動時 1 度のみ)
- `forwardToUboUpload` (= mUseUBO=true 時のみ発火、default false 経路不変)

の範囲で完結 = 実 OpenGL 描画影響ゼロ。

---

## §5. PC-N 分解 Exit Criteria 10 項

| # | Criteria |
|---|----------|
| (i) | PC-N 分解 5 sub-step (PC-N-1..PC-N-5) literal scope §0 明文化 |
| (ii) | 必読 1 件 (本 doc) + pinpoint reference 7 件 §1 列挙 |
| (iii) | 現状調査 9 項 §2 網羅 (= code 7 項 + design doc 2 項) |
| (iv) | ambiguity (N-1)..(N-9) 9 件 AYA literal「Claude 推奨案 OK」record (2026-06-05) §3 |
| (v) | 採用根拠 9 件 §3 明文化 |
| (vi) | PC-N 5 sub-step 概略 + 依存関係 §4 明文化 |
| (vii) | Phase 境界明文化 = PC-N-1..PC-N-4 完了 = Phase 1.C complete、PC-N-5 = Phase 1.D 着手起点 §0 + §4.5 |
| (viii) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 §4.6 |
| (ix) | MUSEUBO-A 整合 = mUseUBO=false default 描画 100% 維持 §4.7 |
| (x) | `indra/` 改変 0 件 = `feedback_design_phase_no_code_write` 整合 |

---

## §6. 着手手順 (= 次 session で PC-N-1 design-lock 着手)

1. AYA 指示「PC-N-1 design-lock 着手お願いします」literal 受領待ち
2. 本 PC-N decomposition design-lock doc 全文 Read (= 必読 1 件)
3. PC-7ε complete doc + design 07 §7/§8 + design 06b §5.3 pinpoint Read (= PC-N-1 source)
4. Explore agent で PC-N-1 詳細現状調査 = `forwardToUboUpload` PER_DRAW case + `writeFrameUbo`/`writeSingletonUbo` pattern + dummy zero memset 周辺
5. PC-N-1 ambiguity (= 想定 §4.1) を列挙 → 推奨案併記 → AYA literal 確認
6. PC-N-1 design-lock doc 起案 (= step (a)-(g) + Exit Criteria) → `indra/` 改変 0 件 → AYA commit 指示後 commit
7. 別 session で PC-N-1 実装着手 → complete handoff doc 起案 → AYA commit 指示後 commit
8. 以後 PC-N-2 → PC-N-4 → PC-N-3 → PC-N-5 を同パターンで進行

---

## §7. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + PC-7ε ✅ + **PC-N decomposition design-lock ✅ 本 commit** + PC-N-1 design-lock ⏳ 次 session + PC-N-1 ⏳ + PC-N-2 ⏳ + PC-N-4 ⏳ + PC-N-3 ⏳ = Phase 1.C complete ⏳ + PC-8 (3 OS build verify) ⏳ + PC-N-5 = Phase 1.D 着手起点 ⏳

---

## §8. self-verify 9 観点 全 ✅

1. **PC-N 分解 literal scope 5 sub-step §0 完全分解** ✅
2. **必読 1 件 §1 + pinpoint reference 7 件 別記** ✅
3. **現状調査 §2 9 項網羅** = code 7 項 + design doc 2 項 + 依存関係図 ✅
4. **ambiguity (N-1)..(N-9) 9 件 AYA literal「Claude 推奨案 OK」record (2026-06-05) §3** ✅
5. **採用根拠 9 件明文化 §3** ✅
6. **PC-N 5 sub-step 概略 §4 + 依存関係 §2.3** ✅
7. **Phase 境界明文化 = (N-8) B 採用 = PC-N-1..PC-N-4 完了 = Phase 1.C complete** ✅
8. **GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 §4.6** ✅
9. **MUSEUBO-A 整合 = mUseUBO=false default 経路不変 §4.7 + `indra/` 改変 0 件 = `feedback_design_phase_no_code_write` 整合** ✅

---

## §9. 次 session 着手 1 line

**PC-N-1 design-lock 着手** = `flushDrawUbos` real per-draw data write 通電 (= PC-6ε-3 持越) の詳細 step 分解 + ambiguity 確認 + Exit Criteria 明文化 (= `forwardToUboUpload` PER_DRAW case 通電 + `writeDrawUbo` helper 新設 + `recordPlaceholderPoolDraw` dummy zero memset → real write 置換)。`indra/` 改変 0 件、別 session で実装 phase 着手。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 design-lock handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 次 session 必読 1 件 + pinpoint reference 7 件 別記
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §8
- **feedback_build_only_verified** 遵守 = design-lock phase は `indra/` 改変 0 件で build verify 対象外、各 sub-step 実装 phase で literal 検証取得予定
- **feedback_no_scope_shrink** 遵守 = PC-N literal scope 5 要素 §0 完全分解、各 sub-step は段階分離 = 縮小ではない (= PC-7ε パターン同形)
- **feedback_doubt_self_first** 遵守 = ambiguity 9 件発見で停止 + 推奨案提示 + AYA literal「Claude 推奨案 OK」確認後 design-lock doc 起案、推測実装なし
- **feedback_confirm_referent_before_acting** 遵守 = 9 件 batch AYA 確認 (2026-06-05)、推測実装なし
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = PC-N を 5 sub-step に分解、各 sub-step は別 session で個別 design-lock + 実装、本 doc は overview のみ
- **feedback_design_phase_no_code_write** 整合 = 本 PC-N decomposition design-lock は doc 起案のみ、`indra/` 改変 0 件
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「commit してください」literal 受領まで commit せず
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
- **feedback_no_bare_reference_ids** 遵守 = (N-1)..(N-9) 各 ID に項目名 / 採用案内容併記 §3 + (PC-N-1)..(PC-N-5) 各 ID に scope 内容併記 §0 + §4
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、git add 個別 file 指定予定

---
