# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-N-2 design-lock complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-N-2 (= `bindV3aRigged` set=2 復活 = signature 拡張 (`const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS]` 引数化、`bindV3aStatic` 同形、PC-7ε パターン) + `recordAvatarPlaceholderDraw` 内 allocate-chain 配線 (= PC-N-1 `recordPlaceholderPoolDraw` パターン同形)) の **design-lock phase 完了** marker = ambiguity (N2-1)..(N2-9) 9 件 + 重要 gap G-1 全 AYA literal「Claude 推奨案 OK」record (2026-06-05) + 実装計画 (a)-(g) 7 step 分解 + Exit Criteria 10 項明文化。`indra/` 改変 0 件 (= `feedback_design_phase_no_code_write` 整合)。

> **本 doc 位置付け**: PC-N decomposition design-lock (= commit `cf7b0b99b0`) §4.2 で確定した PC-N-2 sub-step (= 2 番目着手) の詳細 design-lock。PC-N-1 complete (= 直前 commit `02ab8a45a7` で `writeDrawUbo` helper 新設 + `forwardToUboUpload` PER_DRAW case 通電 + `recordPlaceholderPoolDraw` allocate-chain 通電) を前提として、rigged avatar 経路の set=2 復活 + dynamic offset 配線を固定。別 session で実装 phase 着手。

---

## §0. 本 session 着手契機 + literal scope record

**契機**: AYA 指示「PC-N-2 design-lock 着手お願いします」literal 受領 (2026-06-05) → 直前 commit `02ab8a45a7` (PC-N-1 complete) §8 次 session 着手 1 line に従い、必読 1 件 (PC-N-1 complete doc) Read + pinpoint reference (PC-N decomposition §4.2 + PC-N-1 design-lock + design 07 §7/§8.4) Read → Explore agent 10 項現状調査 (= `bindV3aRigged` 全文 dump + `bindV3aStatic` PC-7ε 拡張形 + `recordAvatarPlaceholderDraw` 全文 dump + `recordPlaceholderPoolDraw` PC-N-1 新形 + `V3A_DRAW_SET_BINDINGS` 定数 + `sDrawUboSetV3a` wire 状態 + codegen set=2 binding 配置 + avatar bone storage 現状 (= PC-N-3 境界) + `forwardToUboUpload` PER_DRAW PC-N-1 状態 + design 07 §7/§8.4 pinpoint) → ambiguity 9 件 + 重要 gap 1 件 (= G-1 = PC-7ε (e) comment 内 「PC-N 実 GLTF avatar Vulkan draw 通電時」literal vs PC-N decomposition §4.2「PC-N-2 = set=2 復活」記述の差異) 発見 → AYA literal「Claude 推奨案 OK」一括確認受領 (2026-06-05) で本 design-lock doc 起案。

**PC-N-2 literal scope** (= PC-N decomposition §4.2 継承):

1. **`bindV3aRigged` signature 拡張** = 現 `void bindV3aRigged(VkCommandBuffer cmd_buf, U32 frame_index)` を `void bindV3aRigged(VkCommandBuffer cmd_buf, U32 frame_index, const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS])` へ拡張 (= `bindV3aStatic` 同形、(N2-4) A) + guard 拡張 (= `sDrawUboSetV3a` nullptr + `dynamic_offsets` nullptr 追加、(N2-5) A)
2. **`bindV3aRigged` body 内 set=2 bind 復活** = 現状第 1 `vkCmdBindDescriptorSets` (= set=0/1a/1b 3 set) を 4 set 拡張 (= set=0/1a/1b/2)、`pDynamicOffsets` に引数 wire、第 2 `vkCmdBindDescriptorSets` (= set=3 swap) は構造維持 ((N2-1) A + (N2-4) A 採用)
3. **`recordAvatarPlaceholderDraw` allocate-chain 配線** = `sAvatarBonePipeline` guard と並行で `sDrawUboRingBufferMgr` nullptr guard 追加 (= 失敗時 `recordPlaceholderPoolDraw` fallback、(N2-6) A) + `PerDrawUBO_LightParams` 経由 zero write (= `recordPlaceholderPoolDraw` と同 block_hash、(N2-2) A) + `dynamic_offsets[V3A_DRAW_SET_BINDINGS]` 配列構築 (= 4 個同一 offset、(N2-3) A) + `bindV3aRigged(cmd_buf, sFrameIndex, dynamic_offsets)` 呼出
4. **PC-7ε (e) comment 整合更新** = 現 `llvkloader.cpp:5244-5247` の「PC-N 実 GLTF avatar Vulkan draw 通電時に H10-A avatar bone storage 再配線と一括」literal を G-1 解消文言 (= 「set=2 復活 = PC-N-2 で実施、H10-A bone storage 再配線 = PC-N-3」) に更新 (= G-1 解消、(N2-7) A)
5. **log message 更新** = `recordAvatarPlaceholderDraw` 内 LL_INFOS first-fire marker に「PC-N-2 set=2 復活通電済」literal 追加 (= PC-7ε (c) `recordPlaceholderPoolDraw` log 更新パターン同形、(N2-7) A)

**`indra/` 改変想定 2 file** (= 実装 phase で別 session):
- `indra/llrender/llvkloader.cpp` (= `bindV3aRigged` signature + body 拡張 + `recordAvatarPlaceholderDraw` allocate-chain 配線 + PC-7ε (e) comment 更新 + log message 更新)
- `indra/llrender/llvkloader.h` (= `bindV3aRigged` 公開宣言 signature 更新)

**`indra/` 改変想定 0 file** (= PC-N-2 scope 外):
- codegen 出力 (= `tests/codegen/main.py` 等) = (N1-7) A pattern 踏襲、binding=2/3 配置 + 4 binding 再分配は別 sub-step
- shader (= `.glsl` files) = set=2 binding 0/1 既存参照のみ、改変なし
- avatar bone storage 経路 = PC-N-3 持越

---

## §1. 必読 1 件 + pinpoint reference

**次 session 必読 (= PC-N-2 実装 phase 着手前)**:

1. **本 PC-N-2 design-lock doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-n-2-design-lock.md`

**pinpoint reference (実装 phase で必要分のみ)**:

- **PC-N-1 complete doc**: `handoff-...-pc-n-1-complete.md` = `writeDrawUbo` helper 実装 + `recordPlaceholderPoolDraw` allocate-chain 配線 (= step (c) `<AYAstorm r41 PC-N-1 (c)>` tag block) の参考実装、PC-N-2 同形パターン作成のベース
- **PC-N decomposition design-lock doc**: `handoff-...-pc-n-decomposition-design-lock.md` §4.2 = PC-N-2 scope 規定 + 依存関係 + Phase 境界
- **`bindV3aStatic` PC-7ε 拡張形**: `indra/llrender/llvkloader.cpp:1747-1779` (= `<AYAstorm r41 PC-7ε (c)>` tag block) = `bindV3aRigged` signature 拡張の同形雛形
- **`recordPlaceholderPoolDraw` PC-N-1 新形**: `indra/llrender/llvkloader.cpp:5129-5213` (= `<AYAstorm r41 PC-N-1 (c)>` tag block) = `recordAvatarPlaceholderDraw` allocate-chain 配線の同形雛形
- **design 07 §7.4 + §8.4**: `design/07-vulkan-api-state.md:387-410, 444-453` = dynamic offset 4 個構成 + FRAMES_IN_FLIGHT 同期 rotate (= `sFrameIndex` 共有)

---

## §2. 現状調査結果 (= Explore agent 10 項要約)

### §2.1 現 code 状態

| # | 項目 | file:line | 現状要約 |
|---|------|-----------|---------|
| 1 | `bindV3aRigged` 現 signature + body | `llvkloader.cpp:1781-1819` | `void bindV3aRigged(VkCommandBuffer cmd_buf, U32 frame_index)` = set=0/1a/1b 3 set bind (= 第 1 call、`dynamicOffsetCount=0`、`pDynamicOffsets=nullptr`) + set=3 単独 bind (= 第 2 call、`firstSet=3`、`descriptorSetCount=1`、`dynamicOffsetCount=0`)。**set=2 skip 状態** (= PC-7ε で deferred、本 PC-N-2 で復活対象)、tag block `<AYAstorm r41 PC-7δ>` (PC-7ε 後も未更新)。guard: `cmd_buf` / `sAYAStandardLayout` / `frame_index` 範囲 / `sFrameUboSetV3a[frame_index]` / `sProgramUboSetA[frame_index]` / `sProgramUboSetB[frame_index]` / `sAssetUboSetV3a[frame_index]` nullptr check |
| 2 | `bindV3aStatic` PC-7ε 拡張形 (= 参考実装) | `llvkloader.cpp:1747-1779` | `void bindV3aStatic(VkCommandBuffer cmd_buf, U32 frame_index, const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS])` = set=0/1a/1b/2 4 set 同時 bind (= 1 call、`dynamicOffsetCount=V3A_DRAW_SET_BINDINGS=4`、`pDynamicOffsets=dynamic_offsets`)。guard: 上記 + `sDrawUboSetV3a` nullptr + `dynamic_offsets` nullptr。tag block `<AYAstorm r41 PC-7ε (c)>` |
| 3 | `recordAvatarPlaceholderDraw` 現 body | `llvkloader.cpp:5223-5276` | `void recordAvatarPlaceholderDraw(VkCommandBuffer cmd_buf)` = guard (`sAvatarBonePipeline` / `sAvatarBoneLayout` / `sAYAStandardLayout` nullptr → `recordPlaceholderPoolDraw` fallback、`cmd_buf` nullptr → return) + `vkCmdBindPipeline(sAvatarBonePipeline)` + `bindV3aRigged(cmd_buf, sFrameIndex)` (= set=2 skip 引数列) + `vkCmdPushConstants(modelview_identity_64B)` + `vkCmdDraw(3,1,0,0)` + first-fire LL_INFOS。tag block `<AYAstorm r41 PC-7δ (j)>` + `<AYAstorm r41 PC-7ε (e)>` (= G-1 該当 comment) |
| 4 | `recordPlaceholderPoolDraw` PC-N-1 新形 (= 参考実装) | `llvkloader.cpp:5129-5213` | guard (`cmd_buf` / `sSkySmokePipeline` / `sSkySmokeLayout` nullptr → return) + `vkCmdBindPipeline(sSkySmokePipeline)` + `<PC-N-1 (c)>` tag block 内: `sDrawUboRingBufferMgr` nullptr guard → `static const U8 zero_buf[256] = {}` → `LLVKLoader::writeDrawUbo(PerDrawUBO_LightParams, 0, zero_buf, 256, dynamic_offset)` → `dynamic_offsets[V3A_DRAW_SET_BINDINGS] = {dynamic_offset × 4}` → `bindV3aStatic(cmd_buf, sFrameIndex, dynamic_offsets)` + `vkCmdPushConstants` + `vkCmdDraw(3,1,0,0)` + first-fire LL_INFOS (= PC-N-1 marker 更新済) |
| 5 | `V3A_DRAW_SET_BINDINGS` 定数 | `llvkloader.cpp:656` | `constexpr U32 V3A_DRAW_SET_BINDINGS = 4` (= 固定値、comment 「set=2: Draw_LightParams + Draw_MultiLight + MaterialUBO + MaterialLegacyBlinn (UBO_DYNAMIC)」 = design 07 §7.4 spec の 4 binding 独立構成想定) |
| 6 | `sDrawUboSetV3a` wire 状態 | `llvkloader.cpp:714` (decl) + `:2605-2611` (allocate) + `:2629-2691` (wire) | `VkDescriptorSet sDrawUboSetV3a = VK_NULL_HANDLE` 宣言 → `createV3aDescriptorSets` 内 `vkAllocateDescriptorSets` 1 set 確保 → `wireDrawUboSetV3aToRingBuffer()` (= PC-7ε (a) tag) で 4 binding × `VkWriteDescriptorSet` を `vkUpdateDescriptorSets` 一括投入 → ring buffer VkBuffer 接続済。**PC-N-2 では既存 wire 経路維持** (= 改変なし) |
| 7 | codegen set=2 binding 配置現状 | `build-linux-x86_64/codegen/ubo/ubo_metadata.inl:64-75` | binding=0 に 6 block 集中 (= AvatarSkin 768 / AvatarVelocity 768 / ClipPlane 256 / **LightParams 256** / ObjectSkin 10752 / SkinnedVelocity 5376) + binding=1 に 1 block (= MultiLight 768) + binding=2/3 0 block。**PC-N-2 scope 外**: (N1-7) A pattern 踏襲、binding=2/3 配置 + 4 binding 再分配は別 sub-step |
| 8 | avatar bone storage 現状 (= PC-N-3 境界) | `llvkloader.cpp:107` (decl) + `:3170-3183` (allocate) | `VkBuffer sAvatarBoneStorageBuffer = VK_NULL_HANDLE` 宣言 → `createBufferVk(7040, STORAGE_BUFFER \| TRANSFER_DST, HOST_VISIBLE \| HOST_COHERENT)` で 110 × mat4 (= identity) 事前確保 + `sAvatarBoneStorageBufferMapped` mapped pointer。**push descriptor 経路 disable** (= PC-7δ H10-A 採用、`sAvatarBoneLayout` は `sAYAStandardLayout` alias 共用)。**PC-N-2 scope 外**: bone storage write 経路 (= `writeAvatarBoneStorage` helper 新設) + set=3 経由再 wire は **PC-N-3 持越** |
| 9 | `forwardToUboUpload` PER_DRAW case (= PC-N-1 通電済) | `llglslshader.cpp:2151-2169` | `case kCadencePerDraw: { U32 dynamic_offset = 0u; LLVKLoader::writeDrawUbo(loc.block_hash, loc.offset, data, size, dynamic_offset); (void)dynamic_offset; return; }` = `<PC-N-1 (b)>` tag block。**dynamic_offset は caller (= bind 経路) に伝達せず discard** (= setter 経路と bind 経路の連結は PC-N-5 持越、PC-N-2 では placeholder pool 経路 = `recordAvatarPlaceholderDraw` 側で別途 chunk 確保 + bind)。**PC-N-2 scope 外** (= PC-N-1 完結部分) |
| 10 | design 07 §7.4 (dynamic offset 4 個構成) + §8.4 (FRAMES_IN_FLIGHT 同期) | `design/07-vulkan-api-state.md:387-410, 444-453` | §7.4 = `U32 offsets[4] = {DrawTransform, DrawLights, MaterialPBR, MaterialDithering}; vkCmdBindDescriptorSets(..., set=2, 1, &sDrawDescriptorSet, 4, offsets)` (= 4 dynamic offset 投入)。§8.4 = `sFrameIndex` 全 cadence 共有 (= set=0/1a/1b/2/3 rotate を `beginFrame()` 1 回で同期) |

### §2.2 🚨 重要 gap G-1 (= PC-7ε (e) comment vs PC-N decomposition §4.2 文言差異)

**現 `llvkloader.cpp:5244-5247` literal**:
```
// <AYAstorm r41 PC-7ε (e)> rigged path は set=2 skip (= dynamic offset 0 個) ゆえ
//   bindV3aRigged signature 不変。per-draw dynamic offset 配線 + set=2 復活は PC-N
//   実 GLTF avatar Vulkan draw 通電時に H10-A avatar bone storage 再配線と一括
//   ((ε-5) A 採用、AYA 確認 2026-06-05)。
```

**PC-N decomposition §4.2 (= 2026-06-05 AYA literal「Claude 推奨案 OK」record 済) literal**:
> **PC-N-2** = `bindV3aRigged` set=2 復活 = signature 拡張 (`const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS]` 引数化、PC-7ε パターン) + `recordAvatarPlaceholderDraw` 内 allocate-chain 配線

**差異**: PC-7ε (e) comment は文脈上「PC-N 実 GLTF avatar Vulkan draw 通電時 (= PC-N-5)」に set=2 復活と H10-A bone storage 再配線を「一括」する読みになる。一方 PC-N decomposition §4.2 では PC-N-2 (= set=2 復活) と PC-N-3 (= bone storage 再配線) が独立 sub-step と分解されている。

**採用** (= AYA literal「OK」確認 2026-06-05): **PC-N decomposition を正準採用**。PC-7ε (e) comment は PC-N decomposition phase 確定前 (= PC-7ε 実装 phase 時点 2026-06-05 早朝、PC-N decomposition 確定は同日後) の暫定文言であり、PC-N-2 実装 step (d) で該当 comment を更新 (= 「set=2 復活 = PC-N-2 で実施、H10-A bone storage 再配線 = PC-N-3」literal に置換) で解消。

### §2.3 design doc 章

| # | 章 | 出典 | 該当 |
|---|----|------|------|
| 11 | dynamic offset (L2) 実 Vulkan 配線 + ring buffer 容量 | `design/07 §7` (line 361-410) | (N2-1) A + (N2-3) A 根拠 (= 4 dynamic offset 構成 spec 出典) |
| 12 | triple-buffering (U1=3) frame index 共有 | `design/07 §8.4` (line 444-453) | (N2-1) A 根拠 (= `sFrameIndex` 全 cadence 同期) |
| 13 | set=1a/1b split + bind 順 | `design/06c §2.3` | (N2-1) A 根拠 (= set=0/1a/1b/2 連続 bind の妥当性) |

---

## §3. ambiguity (N2-1)..(N2-9) 9 件 + 重要 gap G-1 AYA literal「Claude 推奨案 OK」record (2026-06-05) + 採用根拠

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| **(N2-1)** | `bindV3aRigged` body 内 `vkCmdBindDescriptorSets` 構造 | **A**: 1 call で set=0/1a/1b/2 4 set bind (= `bindV3aStatic` 同形、`firstSet=0, descriptorSetCount=4, dynamicOffsetCount=V3A_DRAW_SET_BINDINGS, pDynamicOffsets=dynamic_offsets`) + 別 call で set=3 swap (= 現 第 2 call 構造維持) | OK (2026-06-05) | `bindV3aStatic` 同形 → 対称性 + test 容易性、現状の「set=0/1a/1b と set=3 が別 call 分離」構造を踏襲 (= H10-A push descriptor disable 維持下で set=3 を独立 bind 必要)、5 set 統合 (C) は H10-A `sAvatarBoneLayout = sAYAStandardLayout` alias 共用整合性を崩す risk |
| **(N2-2)** | `recordAvatarPlaceholderDraw` で使う block_hash | **A**: `PerDrawUBO_LightParams` (= `0x9ebc071fu`, 256 B, set=2 binding=0、`recordPlaceholderPoolDraw` と共有) | OK (2026-06-05) | (N1-5) B pattern 踏襲 = placeholder phase は API path 通電が本質、real avatar data (= `PerDrawUBO_AvatarSkin` 768 B 等) 構築は PC-N-5 (実 GLTF) 持越、最小 size (= 256 B) で grow リスク回避、`recordPlaceholderPoolDraw` と同 block 共有で test 整合性 |
| **(N2-3)** | `dynamic_offsets[V3A_DRAW_SET_BINDINGS]` 4 個の値構造 | **A**: 4 個同一 offset (= `writeDrawUbo` 1 回呼出、`alloc.offset` を 4 binding に reuse) | OK (2026-06-05) | (ε-2) A + (N1-5) B pattern 踏襲、placeholder phase は 1 allocate で簡略化、PC-N-5 で B (= 4 独立 allocate per binding) へ拡張可、現 codegen で binding=2/3 未配置 (= shader 未参照) ゆえ 4 個同一 offset で GPU error なし |
| **(N2-4)** | set=3 swap 挙動 | **A**: 同形維持 (= 第 2 call で `firstSet=3, descriptorSetCount=1, &sAssetUboSetV3a[frame_index]` bind、現状踏襲) | OK (2026-06-05) | H10-A push descriptor disable (= PC-7δ) + `sAvatarBoneLayout` が `sAYAStandardLayout` alias 共用ゆえ layout 整合維持、bone storage 再配線 (= PC-N-3) 時に再評価、本 PC-N-2 で 1 call 統合 (C) すると PC-N-3 で再分離必要 (= scope 拡大) |
| **(N2-5)** | `bindV3aRigged` guard 条件拡張 | **A**: `sDrawUboSetV3a` nullptr + `dynamic_offsets` nullptr 追加 (= `bindV3aStatic` 同形) | OK (2026-06-05) | `bindV3aStatic` 同形 → defensive coding 一貫性、set=2 復活で `sDrawUboSetV3a` が必須参照になる、caller (= `recordAvatarPlaceholderDraw`) 側でも nullptr guard あるが多重保証 |
| **(N2-6)** | `recordAvatarPlaceholderDraw` 内 nullptr guard 位置 | **A**: `sDrawUboRingBufferMgr` 失敗時 `recordPlaceholderPoolDraw` fallback (= 現 `sAvatarBonePipeline` guard と並行) | OK (2026-06-05) | 現 `sAvatarBonePipeline` / `sAvatarBoneLayout` / `sAYAStandardLayout` guard と同形 fallback パターン、caller fail-fast、`recordPlaceholderPoolDraw` 側も同 nullptr guard 持つ (= ring buffer mgr null 時 return) ゆえ無限再帰なし、graceful degrade で validation 違反 0 件維持 |
| **(N2-7)** | log message + PC-7ε (e) comment 更新 (G-1 解消) | **A**: (i) `recordAvatarPlaceholderDraw` 内 LL_INFOS first-fire marker に「PC-N-2 set=2 復活通電済」literal 追加 + (ii) PC-7ε (e) comment (= `llvkloader.cpp:5244-5247`) を「set=2 復活 = PC-N-2 で実施、H10-A bone storage 再配線 = PC-N-3」literal に更新 | OK (2026-06-05) | PC-7ε (c) `recordPlaceholderPoolDraw` log 更新パターン同形、verification log marker、G-1 (= PC-7ε comment vs PC-N decomposition 文言差異) を実装 step (d) で解消 |
| **(N2-8)** | build verify scope | **A**: `llrender` + warning 0 + TUT 11+10+13 + codegen 131/131 (= Linux primary、PC-N-1 同形) | OK (2026-06-05) | cold launch verify は PC-8 集約 ((N-9) A 確認済)、(N1-9) A pattern 踏襲、PC-N-2 単独 sub-step では build verify のみ |
| **(N2-9)** | PC-N-2 Exit Criteria 項目数 | **A**: 10 項 (= PC-N-1 同形 template) | OK (2026-06-05) | 一貫性 + template 流用、項目内訳 = (i) signature 拡張 + (ii) set=2 bind 復活 + (iii) recordAvatarPlaceholderDraw 配線 + (iv) PC-7ε comment 更新 + (v) log marker 更新 + (vi) GATE-B 整合 + (vii) MUSEUBO-A 整合 + (viii) build verify + (ix) tag block 統一 + (x) handoff complete doc 起案 |
| **(G-1)** | 🚨 PC-7ε (e) comment vs PC-N decomposition §4.2 文言差異 | **PC-N decomposition 正準採用** = PC-7ε (e) comment は PC-N decomposition phase 確定前の暫定文言、PC-N-2 実装 step (d) で更新 | OK (2026-06-05) | PC-N decomposition §4.2 は 2026-06-05 AYA literal「OK」record 済、PC-7ε (e) comment は同日早朝の暫定記述ゆえ後発確定の PC-N decomposition が正準、実装 step (d) で comment 更新で解消 |

---

## §4. PC-N-2 実装計画 (a)-(g) 7 step 分解

### §4.1 step (a) = `bindV3aRigged` signature 拡張 (`llvkloader.cpp` + `.h`)

**file**: `indra/llrender/llvkloader.cpp:1781` (= function 定義) + `indra/llrender/llvkloader.h` 内 `bindV3aRigged` 公開宣言

**改変** (= (N2-1) A + (N2-5) A 採用):

```cpp
// 旧 signature (= PC-7ε まで未更新)
void bindV3aRigged(VkCommandBuffer cmd_buf, U32 frame_index);

// 新 signature (= PC-N-2 (a) 拡張、bindV3aStatic 同形)
void bindV3aRigged(VkCommandBuffer cmd_buf,
                   U32             frame_index,
                   const U32       dynamic_offsets[V3A_DRAW_SET_BINDINGS]);
```

**guard 拡張** (= (N2-5) A 採用):

```cpp
// 旧 guard (= line 1783-1790)
if (cmd_buf == VK_NULL_HANDLE ||
    sAYAStandardLayout == VK_NULL_HANDLE ||
    frame_index >= FRAMES_IN_FLIGHT ||
    sFrameUboSetV3a[frame_index]    == VK_NULL_HANDLE ||
    sProgramUboSetA[frame_index]    == VK_NULL_HANDLE ||
    sProgramUboSetB[frame_index]    == VK_NULL_HANDLE ||
    sAssetUboSetV3a[frame_index]    == VK_NULL_HANDLE)
{
    return;
}

// 新 guard (= PC-N-2 (a)、sDrawUboSetV3a + dynamic_offsets 追加)
if (cmd_buf == VK_NULL_HANDLE ||
    sAYAStandardLayout == VK_NULL_HANDLE ||
    frame_index >= FRAMES_IN_FLIGHT ||
    sFrameUboSetV3a[frame_index]    == VK_NULL_HANDLE ||
    sProgramUboSetA[frame_index]    == VK_NULL_HANDLE ||
    sProgramUboSetB[frame_index]    == VK_NULL_HANDLE ||
    sDrawUboSetV3a                  == VK_NULL_HANDLE ||  // (N2-5) A 追加
    sAssetUboSetV3a[frame_index]    == VK_NULL_HANDLE ||
    dynamic_offsets                 == nullptr)           // (N2-5) A 追加
{
    return;
}
```

### §4.2 step (b) = `bindV3aRigged` body 内 set=2 bind 復活 (`llvkloader.cpp:1791-1818`)

**改変** (= (N2-1) A + (N2-3) A + (N2-4) A 採用、tag block `<AYAstorm r41 PC-7δ>` → `<AYAstorm r41 PC-N-2 (b)>`):

```cpp
// 旧 (= PC-7δ 実装、set=2 skip)
const VkDescriptorSet sets_0_to_1b[3] = {
    sFrameUboSetV3a[frame_index],
    sProgramUboSetA[frame_index],
    sProgramUboSetB[frame_index],
};
vkCmdBindDescriptorSets(cmd_buf,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        sAYAStandardLayout,
                        /*firstSet=*/0,
                        /*descriptorSetCount=*/3,
                        sets_0_to_1b,
                        /*dynamicOffsetCount=*/0,
                        /*pDynamicOffsets=*/nullptr);

// 新 (= PC-N-2 (b) 実装、set=2 復活、bindV3aStatic 同形)
const VkDescriptorSet sets_0_to_2[V3A_DRAW_SET_BINDINGS] = {
    sFrameUboSetV3a[frame_index],
    sProgramUboSetA[frame_index],
    sProgramUboSetB[frame_index],
    sDrawUboSetV3a,                  // (N2-1) A 追加: set=2 復活
};
vkCmdBindDescriptorSets(cmd_buf,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        sAYAStandardLayout,
                        /*firstSet=*/0,
                        /*descriptorSetCount=*/V3A_DRAW_SET_BINDINGS,
                        sets_0_to_2,
                        /*dynamicOffsetCount=*/V3A_DRAW_SET_BINDINGS,
                        dynamic_offsets);                    // (N2-3) A: 4 個同一 offset (caller 構築)
```

**第 2 call (= set=3 swap)** = 現状維持 ((N2-4) A 採用):

```cpp
// 既存 (= PC-7δ、set=2 ↔ set=3 swap 実走、現状維持)
vkCmdBindDescriptorSets(cmd_buf,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        sAYAStandardLayout,
                        /*firstSet=*/3,
                        /*descriptorSetCount=*/1,
                        &sAssetUboSetV3a[frame_index],
                        /*dynamicOffsetCount=*/0,
                        /*pDynamicOffsets=*/nullptr);
```

### §4.3 step (c) = `recordAvatarPlaceholderDraw` allocate-chain 配線 (`llvkloader.cpp:5223-5276`)

**改変** (= (N2-2) A + (N2-3) A + (N2-6) A 採用、tag block `<AYAstorm r41 PC-7δ (j)>` 維持 + 内部 `<AYAstorm r41 PC-N-2 (c)>` 追加):

```cpp
// 旧 (= PC-7δ 実装、set=2 skip 経由)
void recordAvatarPlaceholderDraw(VkCommandBuffer cmd_buf)
{
    // <AYAstorm r41 PC-7δ (j)> H10-A 採用: push descriptor 経路 disable + sAvatarBoneLayout alias 共用
    if (sAvatarBonePipeline == VK_NULL_HANDLE ||
        sAvatarBoneLayout == VK_NULL_HANDLE ||
        sAYAStandardLayout == VK_NULL_HANDLE)
    {
        recordPlaceholderPoolDraw(cmd_buf);
        return;
    }

    if (cmd_buf == VK_NULL_HANDLE) { return; }

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, sAvatarBonePipeline);

    // <AYAstorm r41 PC-7ε (e)> rigged path は set=2 skip ... (G-1 該当 comment、本 PC-N-2 (d) で更新)
    bindV3aRigged(cmd_buf, sFrameIndex);

    // push constant identity + vkCmdDraw + first-fire LL_INFOS (= 既存維持)
    ...
}

// 新 (= PC-N-2 (c) 実装、allocate-chain 配線 + bindV3aRigged 拡張呼出)
void recordAvatarPlaceholderDraw(VkCommandBuffer cmd_buf)
{
    // <AYAstorm r41 PC-7δ (j)> H10-A 採用 (= 維持)
    if (sAvatarBonePipeline == VK_NULL_HANDLE ||
        sAvatarBoneLayout == VK_NULL_HANDLE ||
        sAYAStandardLayout == VK_NULL_HANDLE)
    {
        recordPlaceholderPoolDraw(cmd_buf);
        return;
    }

    if (cmd_buf == VK_NULL_HANDLE) { return; }

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, sAvatarBonePipeline);

    // <AYAstorm r41 PC-N-2 (c)> per-draw ring buffer allocate-chain を writeDrawUbo helper 経由 API path で配線
    //   (= AYA literal「Claude 推奨案 OK」確認 2026-06-05、ambiguity (N2-2) A PerDrawUBO_LightParams 共有
    //   + (N2-3) A 4 個同一 offset + (N2-6) A sDrawUboRingBufferMgr nullptr 時 recordPlaceholderPoolDraw fallback)。
    //   placeholder phase ゆえ zero data 維持、real avatar data 構築は PC-N-5 (実 GLTF) 持越。
    if (!sDrawUboRingBufferMgr)
    {
        recordPlaceholderPoolDraw(cmd_buf);    // (N2-6) A fallback
        return;
    }
    static const U8 zero_buf[256] = {};
    U32 dynamic_offset = 0u;
    LLVKLoader::writeDrawUbo(
        ubo::block_hash::PerDrawUBO_LightParams,   // (N2-2) A: recordPlaceholderPoolDraw と共有
        /*offset=*/0u,
        zero_buf,
        sizeof(zero_buf),
        dynamic_offset);
    const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS] = {
        dynamic_offset, dynamic_offset, dynamic_offset, dynamic_offset,   // (N2-3) A: 4 個同一
    };
    bindV3aRigged(cmd_buf, sFrameIndex, dynamic_offsets);   // (N2-1) A: 拡張 signature 呼出
    // </AYAstorm r41 PC-N-2 (c)>

    // push constant identity + vkCmdDraw (= 既存維持)
    const float identity_modelview[16] = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };
    vkCmdPushConstants(cmd_buf, sAvatarBoneLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, identity_modelview);
    vkCmdDraw(cmd_buf, 3, 1, 0, 0);

    // first-fire LL_INFOS (= (N2-7) A で「PC-N-2 set=2 復活通電済」literal 追加、§4.5 で詳述)
    ...
}
```

### §4.4 step (d) = PC-7ε (e) comment 整合更新 (G-1 解消) (`llvkloader.cpp:5244-5247`)

**改変** (= (N2-7) A + (G-1) 採用):

```cpp
// 旧 (= PC-7ε (e) tag、G-1 該当)
// <AYAstorm r41 PC-7ε (e)> rigged path は set=2 skip (= dynamic offset 0 個) ゆえ
//   bindV3aRigged signature 不変。per-draw dynamic offset 配線 + set=2 復活は PC-N
//   実 GLTF avatar Vulkan draw 通電時に H10-A avatar bone storage 再配線と一括
//   ((ε-5) A 採用、AYA 確認 2026-06-05)。

// 新 (= PC-N-2 (d) で更新、G-1 解消)
// <AYAstorm r41 PC-N-2 (d)> set=2 復活 = 本 PC-N-2 で実施 (= bindV3aRigged signature 拡張 +
//   recordAvatarPlaceholderDraw allocate-chain 配線、(N2-1)..(N2-9) AYA literal「Claude 推奨案 OK」
//   確認 2026-06-05)。H10-A avatar bone storage 再配線 (= writeAvatarBoneStorage helper 新設 +
//   set=3 経由再 wire) は PC-N-3 持越し、ring buffer grow 自動 re-wire は PC-N-4 持越し
//   (= PC-N decomposition design-lock commit cf7b0b99b0、AYA literal「OK」確認 2026-06-05)。
//   placeholder phase ゆえ zero data 維持、real avatar data 構築 (= PerDrawUBO_AvatarSkin 等) は
//   PC-N-5 実 GLTF avatar Vulkan draw 通電持越し。
```

> **注**: 旧 PC-7ε (e) tag block は完全削除し、PC-N-2 (d) tag block に置換。コメント以外のコード行 (= `bindV3aRigged(cmd_buf, sFrameIndex)` 呼出) は step (c) で `bindV3aRigged(cmd_buf, sFrameIndex, dynamic_offsets)` に拡張済ゆえ重複処理なし。

### §4.5 step (e) = log message 更新 (`llvkloader.cpp:5263-5275`)

**改変** (= (N2-7) A 採用):

```cpp
// 旧 (= PC-7δ 実装)
static bool s_first_avatar_call = true;
if (s_first_avatar_call)
{
    s_first_avatar_call = false;
    LL_INFOS("Vulkan") << "Avatar placeholder pool draw fired (PSO bind sAvatarBonePipeline + "
                          "bindV3aRigged (set=0 Frame V3a + set=1a/1b ProgramUbo + set=3 AssetUbo, "
                          "push descriptor 経路 disable) + push constant 64 B / "
                          "VERTEX_BIT + vkCmdDraw(3,1,0,0))"
                       << LL_ENDL;
}

// 新 (= PC-N-2 (e) 実装、set=2 復活 marker 追加)
static bool s_first_avatar_call = true;
if (s_first_avatar_call)
{
    s_first_avatar_call = false;
    LL_INFOS("Vulkan") << "Avatar placeholder pool draw fired (PC-N-2 set=2 復活通電済: "
                          "writeDrawUbo(PerDrawUBO_LightParams, zero 256B) → "
                          "dynamic_offsets[4] (= 4 個同一 offset) → "
                          "bindV3aRigged (set=0 Frame V3a + set=1a/1b ProgramUbo + "
                          "set=2 DrawUbo V3a + set=3 AssetUbo, "
                          "push descriptor 経路 disable 維持) + push constant 64 B / "
                          "VERTEX_BIT + vkCmdDraw(3,1,0,0))"
                       << LL_ENDL;
}
```

### §4.6 step (f) = build verify

**実行内容** (= (N2-8) A 採用、PC-N-1 同形パターン):

```bash
cd /home/ishikawa/work_firestorm/phoenix-firestorm/build-linux-x86_64
make -j4 llrender
# 期待: PASS + ERROR 0 + WARNING 0
make -j4 INTEGRATION_TEST_lluboringbuffer
ctest -R lluboringbuffer
# 期待: 11/11 PASS
make -j4 INTEGRATION_TEST_llassetubopool
ctest -R llassetubopool
# 期待: 10/10 PASS
make -j4 INTEGRATION_TEST_llpipelinecachestorage
ctest -R llpipelinecachestorage
# 期待: 13/13 PASS
cd /home/ishikawa/work_firestorm/phoenix-firestorm/scripts/ubo_codegen
python -m unittest discover -s tests -v
# 期待: 131/131 PASS
```

### §4.7 step (g) = handoff complete doc 起案 + AYA commit 指示後 commit

**作業**:

1. `handoff-...-pc-n-2-complete.md` 起案 = §0 着手契機 + §1 必読 + pinpoint reference + §2 実装内容 (= step (a)-(g) 全実装記録 + 改変サマリ表) + §3 build verify 結果 + §4 Exit Criteria 10 項充足 + §5 残 strict 線形 + §6 milestone state + §7 self-verify 9 観点 + §8 次 session 着手 1 line + §A feedback 遵守 record
2. AYA literal 「commit してください」受領後 git add 個別 file (= `indra/llrender/llvkloader.cpp` + `indra/llrender/llvkloader.h` + 新 doc) + commit (= Co-Authored-By 不在 + 個別 file 指定)

### §4.8 GATE-B 整合 (全 step 共通)

`#ifdef LL_VULKAN_GLSL` 新規追加 0 件 = `project_r41_phase1b_vulkan_host_gate` 遵守。host 側 redirect 層は `mUseUBO` runtime flag のみで gate (= GATE-B 確定 2026-06-04)。

PC-N-2 改変箇所:
- `bindV3aRigged` signature + body = Vulkan-only code (= `#ifdef` 不要、Vulkan unit 自体が gate)
- `recordAvatarPlaceholderDraw` = Vulkan placeholder 経路、`mUseUBO` 不問、起動時 1 度のみ first-fire log (= `#ifdef` 不要)
- PC-7ε (e) comment 更新 = コメントのみ、code 行影響なし (= `#ifdef` 不要)
- log message 更新 = LL_INFOS 文字列のみ (= `#ifdef` 不要)

### §4.9 MUSEUBO-A 整合 (全 step 共通)

`mUseUBO=false` default で既存 OpenGL 描画 100% 維持:
- `bindV3aRigged` = Vulkan 描画 path 内のみ呼ばれる (= `recordAvatarPlaceholderDraw` + 将来の real avatar draw 経路)、OpenGL 描画影響ゼロ
- `recordAvatarPlaceholderDraw` = Vulkan placeholder offscreen FBO 経路、`sAvatarBonePipeline` / `sAvatarBoneLayout` / `sAYAStandardLayout` 全 nullptr 時 `recordPlaceholderPoolDraw` fallback + `sDrawUboRingBufferMgr` nullptr 時も同 fallback ((N2-6) A) = 多重 graceful degrade
- 視覚 no-op 等価維持 (= avatar bone storage は identity matrix 維持、描画結果 0 差異)

---

## §5. PC-N-2 Exit Criteria 10 項

| # | Criteria |
|---|----------|
| (i) | `bindV3aRigged` signature 拡張 = `const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS]` 引数追加 + guard 拡張 (= `sDrawUboSetV3a` + `dynamic_offsets` nullptr 追加、`bindV3aStatic` 同形、(N2-1) A + (N2-5) A) |
| (ii) | `bindV3aRigged` body 内 set=2 bind 復活 = 第 1 `vkCmdBindDescriptorSets` を 3 set → 4 set 拡張 (= set=0/1a/1b/2) + `pDynamicOffsets` に引数 wire、第 2 call (= set=3 swap) 構造維持 ((N2-1) A + (N2-4) A) |
| (iii) | `recordAvatarPlaceholderDraw` allocate-chain 配線 = `sDrawUboRingBufferMgr` nullptr → `recordPlaceholderPoolDraw` fallback + `writeDrawUbo(PerDrawUBO_LightParams, 0, zero_buf, 256, dynamic_offset)` + `dynamic_offsets[V3A_DRAW_SET_BINDINGS]={dynamic_offset×4}` + `bindV3aRigged(cmd_buf, sFrameIndex, dynamic_offsets)` 呼出 ((N2-2) A + (N2-3) A + (N2-6) A) |
| (iv) | PC-7ε (e) comment 整合更新 = `llvkloader.cpp:5244-5247` の G-1 該当 comment を「set=2 復活 = PC-N-2、bone storage 再配線 = PC-N-3」literal に更新 ((N2-7) A + (G-1)) |
| (v) | log message 更新 = `recordAvatarPlaceholderDraw` first-fire LL_INFOS に「PC-N-2 set=2 復活通電済」literal 追加 ((N2-7) A) |
| (vi) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 |
| (vii) | MUSEUBO-A 整合 = `mUseUBO=false` default で既存 OpenGL 描画 100% 維持 + `sDrawUboRingBufferMgr` nullptr 多重 fallback |
| (viii) | build verify = `llrender` build + ERROR 0 / WARNING 0 + INTEGRATION_TEST 11+10+13 全 PASS + codegen unittest 131/131 PASS ((N2-8) A) |
| (ix) | tag block 統一 = `<AYAstorm r41 PC-N-2 (a)>` (signature) + `<AYAstorm r41 PC-N-2 (b)>` (set=2 bind 復活) + `<AYAstorm r41 PC-N-2 (c)>` (recordAvatarPlaceholderDraw 配線) + `<AYAstorm r41 PC-N-2 (d)>` (PC-7ε (e) comment 更新) + log marker `PC-N-2 set=2 復活通電済` |
| (x) | handoff complete doc 起案 + AYA commit 指示後 commit (= Co-Authored-By 不在 + 個別 file 指定 + 新 file 0 除 doc + CMake 改変 0 + settings.xml 改変 0 + codegen 改変 0) |

---

## §6. 着手手順 (= 次 session で PC-N-2 実装 phase 着手)

1. AYA 指示「PC-N-2 実装着手お願いします」literal 受領待ち
2. 本 PC-N-2 design-lock doc 全文 Read (= 必読 1 件)
3. `bindV3aStatic` PC-7ε 拡張形 (= `llvkloader.cpp:1747-1779`) pinpoint Read = signature + guard 同形雛形参考
4. `recordPlaceholderPoolDraw` PC-N-1 新形 (= `llvkloader.cpp:5129-5213`、`<PC-N-1 (c)>` tag block) pinpoint Read = allocate-chain 雛形参考
5. `bindV3aRigged` 現 body (= `llvkloader.cpp:1781-1819`) + `recordAvatarPlaceholderDraw` 現 body (= `llvkloader.cpp:5223-5276`) 全文 Read
6. step (a) `bindV3aRigged` signature 拡張 (`.h` + `.cpp`) → step (b) body 内 set=2 bind 復活 → step (c) `recordAvatarPlaceholderDraw` allocate-chain 配線 → step (d) PC-7ε (e) comment 更新 (G-1 解消) → step (e) log message 更新 → step (f) build verify → step (g) handoff complete doc 起案
7. Exit Criteria 10 項 self-verify 全 ✅
8. AYA literal「commit してください」受領後 git add 個別 file + commit

---

## §7. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1 design-lock ✅ + PC-N-1 ✅ + **PC-N-2 design-lock ✅ 本 commit** + PC-N-2 実装 ⏳ 次 session + PC-N-4 design-lock ⏳ + PC-N-4 実装 ⏳ + PC-N-3 design-lock ⏳ + PC-N-3 実装 ⏳ = Phase 1.C complete ⏳ + PC-8 (3 OS build verify) ⏳ + PC-N-5 = Phase 1.D 着手起点 ⏳

---

## §8. self-verify 9 観点 全 ✅

1. **PC-N-2 literal scope 5 件 §0 完全分解** = `bindV3aRigged` signature 拡張 + body 内 set=2 bind 復活 + `recordAvatarPlaceholderDraw` allocate-chain 配線 + PC-7ε (e) comment 更新 (G-1 解消) + log message 更新 ✅
2. **必読 1 件 §1 + pinpoint reference 5 件 別記** = PC-N-1 complete + PC-N decomposition §4.2 + `bindV3aStatic` PC-7ε 拡張形 + `recordPlaceholderPoolDraw` PC-N-1 新形 + design 07 §7.4/§8.4 ✅
3. **現状調査 §2 10 項網羅** = code 9 項 (`bindV3aRigged` + `bindV3aStatic` + `recordAvatarPlaceholderDraw` + `recordPlaceholderPoolDraw` + `V3A_DRAW_SET_BINDINGS` + `sDrawUboSetV3a` + codegen set=2 + bone storage + `forwardToUboUpload` PER_DRAW) + design doc 1 項 (= §7.4/§8.4) + G-1 重要 gap 別記 ✅
4. **ambiguity (N2-1)..(N2-9) 9 件 + 重要 gap G-1 AYA literal「Claude 推奨案 OK」record (2026-06-05) §3** ✅
5. **採用根拠 9 件 + G-1 解消方針明文化 §3 表** ✅
6. **実装計画 (a)-(g) 7 step 分解 §4** ✅
7. **GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 §4.8** ✅
8. **MUSEUBO-A 整合 = `mUseUBO=false` default 経路不変 + 多重 nullptr fallback §4.9** ✅
9. **Exit Criteria 10 項明文化 §5 + `indra/` 改変 0 件 = `feedback_design_phase_no_code_write` 整合** ✅

---

## §9. 次 session 着手 1 line

**PC-N-2 実装着手** = step (a)-(g) 7 step 実施 = (a) `bindV3aRigged` signature 拡張 (= `const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS]` 引数追加 + guard 拡張、`.h` + `.cpp` 同時更新) + (b) body 内 set=2 bind 復活 (= 第 1 `vkCmdBindDescriptorSets` を 3 set → 4 set + `pDynamicOffsets` wire、第 2 call set=3 swap 構造維持) + (c) `recordAvatarPlaceholderDraw` allocate-chain 配線 (= `sDrawUboRingBufferMgr` nullptr → `recordPlaceholderPoolDraw` fallback + `writeDrawUbo(PerDrawUBO_LightParams, zero 256B)` + `dynamic_offsets[4]={dynamic_offset×4}` + `bindV3aRigged(cmd_buf, sFrameIndex, dynamic_offsets)`) + (d) PC-7ε (e) comment 整合更新 (= G-1 解消、「set=2 復活 = PC-N-2、bone storage 再配線 = PC-N-3」literal 置換) + (e) log message 更新 (= first-fire LL_INFOS に PC-N-2 marker 追加) + (f) build verify (= llrender + warning 0 + TUT 11+10+13 + codegen 131/131) + (g) handoff complete doc 起案、Exit Criteria 10 項全充足、AYA literal「commit してください」受領後 commit。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 design-lock handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 次 session 必読 1 件 + pinpoint reference 5 件 別記、本 session も Explore agent 経由 pinpoint 取得のみ、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §8
- **feedback_build_only_verified** 遵守 = design-lock phase は `indra/` 改変 0 件で build verify 対象外、実装 phase で literal 検証取得予定
- **feedback_no_scope_shrink** 遵守 = PC-N-2 literal scope 5 件 §0 完全分解 (= signature 拡張 + set=2 bind 復活 + allocate-chain 配線 + PC-7ε comment 更新 + log marker 更新)、(N2-2) A (= `PerDrawUBO_LightParams` 共有) + (N2-3) A (= 1 allocate 4 binding 同 offset) は AYA literal「OK」record 済段階分離 = 縮小ではない (= PC-N-1 (N1-5) B + PC-7ε (ε-2) A パターン同形、real avatar data + 4 独立 allocate は PC-N-5 持越し record 済)
- **feedback_doubt_self_first** 遵守 = ambiguity 9 件発見 + 重要 gap 1 件 (G-1 = PC-7ε (e) comment vs PC-N decomposition §4.2 文言差異) 発見で停止 + 推奨案提示 + AYA literal「Claude 推奨案 OK」確認後 design-lock doc 起案、推測実装なし
- **feedback_confirm_referent_before_acting** 遵守 = 9 件 + G-1 batch AYA 確認 (2026-06-05)、PC-7ε (e) comment と PC-N decomposition §4.2 の literal 差異も AYA に明示提示 + literal「OK」受領で PC-N decomposition 正準採用確定、推測実装なし
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = PC-N-2 = `bindV3aRigged` set=2 復活 + `recordAvatarPlaceholderDraw` allocate-chain 配線単独 sub-step、PC-N-3 (bone storage 再配線 = `writeAvatarBoneStorage` helper 新設 + set=3 経由再 wire) + PC-N-4 (grow auto re-wire) + PC-N-5 (実 GLTF avatar draw) は分離、本 doc 起案も PC-N-2 単独 design-lock のみ
- **feedback_design_phase_no_code_write** 整合 = 本 PC-N-2 design-lock phase は doc 起案のみ、`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「commit してください」literal 受領まで commit せず
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
- **feedback_no_bare_reference_ids** 遵守 = (N2-1)..(N2-9) 各 ID に項目名 / 採用案内容併記 §3、G-1 に literal 差異内容併記、(a)..(g) 各 step に作業内容併記 §4
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、git add 個別 file 指定予定 + `git add -A` 不使用

---
