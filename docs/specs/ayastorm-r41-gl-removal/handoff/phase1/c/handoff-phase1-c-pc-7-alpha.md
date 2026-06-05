---
title: r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-7α complete + PC-7β 引継 marker
date: 2026-06-05
status: complete (PC-7α) / pending (PC-7β..ε)
parent_handoff: handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-6-zeta.md
---

# PC-7α complete + PC-7β 引継 marker

## §0 必読 (= 次 session 着手前 minimum 3 件)

1. **本 doc 全文** (= PC-7α 実施内容 + PC-7β scope + (X1-B/X2-B/X3-A/Z1-B/Z2-C/Z3-A) AYA 確認 record)
2. **design/07-vulkan-api-state.md §3.2 (V1') + §6.2 / §6.3 (pool 容量) + §9.1 (sAYAStandardLayout)** (= V3a 5-set scaffolding source of truth)
3. **design/06c-descriptor-set-bind-wiring.md §2 全体** (= descriptor set 4 帯 cadence 別配置 + V1' split 後の論理 5 帯)

**pinpoint reference** (= 必要時 Read):
- `indra/llrender/llvkloader.cpp` 編集 6 箇所 (= DeviceLimits struct line 349-378 / queryAndLogDeviceLimits 532-540 + 575-594 / anonymous ns V3a 宣言 line 506-571 / createV3aDescriptorSetLayouts + createV3aDescriptorPools + createAYAStandardPipelineLayout helpers / initVulkan 配線 / shutdownVulkan 配線)
- `build-linux-x86_64/codegen/ubo/ubo_metadata.inl` (= 現状 set 帯 drift = Z2-C 持越、PC-7β とは独立な codegen V1' 更新 task)

## §1 起案契機 + AYA 確認 record

### §1.1 起案契機

PC-6ζ commit (8950c452f0、2026-06-05) 後、AYA literal「PC-7 着手お願いします」受領 → 必読 3 件 Read (= PC-6ζ handoff doc 全文 + 06c §2 全体 + 07 §9.1 + §9.3 + §12) → PC-7 scope 整理で 3 件 ambiguity (X1)(X2)(X3) + codebase 調査で 3 件 state mismatch (Z1)(Z2)(Z3) 浮上 → 順次 AYA 確認取得後実装。

### §1.2 AYA 判断確定 record (= 全 6 件)

| ID | 確認内容 | AYA 採用 | literal record |
|---|---|---|---|
| (X1-B) | PC-7 5 分割 (PC-7α scaffolding / PC-7β UboInstance buffers / PC-7γ forwardToUboUpload 本格化 / PC-7δ vkCmdBindDescriptorSets 通電 / PC-7ε dynamic offset 経路) | 採用 | 「Claude 推奨 = (X1-B) PC-7α..ε 5 分割」(2026-06-05) |
| (X2-B) | sampler descriptor は PC-7 scope 外 (= S3' design 07 §5.2 持越) | 採用 | 「OK」(2026-06-05) |
| (X3-A) | UboInstance VkBuffer triple-buffer 一括確保 (= PC-7β 内、FRAMES_IN_FLIGHT=3 同時 allocate) | 採用 | 「OK」(2026-06-05) |
| (Z1-B) | 既存 sPerFrameDescriptorSetLayout (2 binding) 不変、別名 sFrameUboLayoutV3a (4 binding) 新設で並存 | 採用 | 「OK」(2026-06-05) |
| (Z2-C) | codegen ubo_metadata.inl V1' update (set=1a/1b split 反映) は独立 PC-7α' sub-task として後段、PC-7α は VkObject scaffolding 純化 | 採用 | 「OK」(2026-06-05) |
| (Z3-A) | 既存 sPerMaterialDescriptorSetLayout (7 binding COMBINED_IMAGE_SAMPLER) 不変、別名 sDrawUboLayoutV3a (4 binding UBO_DYNAMIC) 新設で並存 | 採用 | 「OK」(2026-06-05) |

### §1.3 採用根拠 record

- **(X1-B) 5 分割根拠** = PC-7 scope 全体 (= vkCmdBindDescriptorSets 通電 + dynamic offset ring buffer chunk hand-off + UboInstance member 拡充 + forwardToUboUpload 本格化) は 1 commit に積めない大塊、UBO migration one-at-a-time 規律 (= feedback_ubo_migration_one_at_a_time) + handoff between commit boundary で逐次検証可能化
- **(X2-B) sampler 除外根拠** = sampler 49 binding は set=3 同居 (S3') で design 07 §5.2 / §10 持越、本 PC-7 では純 UBO bind 経路の通電 priority、sampler は別 sub-step (= PC-7+ / Phase 1.D 検討)
- **(X3-A) triple-buffer 一括確保根拠** = FRAMES_IN_FLIGHT=3 同期 rotate (= design 07 §8.4) で 3 buffer parallel 必須、PC-7β UboInstance member 拡充と同 commit で同形に確保 = 後段 PC-7γ で per-frame map 切替の write race 回避
- **(Z1-B / Z3-A) 並存根拠** = 既存 PC-6α/β/γ 配線 (= Phase 1.B PerFrameMatrixUBO + per-material 7 PBR slot + LLAssetUboPool + LLUboRingBuffer) は Phase 1.B での動作不変が **MUSEUBO-A 整合根本** (= mUseUBO=false default で既存 OpenGL 描画 100% 維持)、新 V3a layout を独立 object として並設 → PC-7δ で bind path 切替時に旧 layout deprecate 判断可能 (= 段階移行 + rollback path 保全)
- **(Z2-C) 独立 sub-task 根拠** = codegen ubo_metadata.inl 更新は 130 件 unittest 影響 + 91 block 全件 set/binding 番号変更 = 影響大 task、PC-7α VkObject scaffolding と分離して回帰隔離 + AYA literal「PC-7α' codegen V1' update は別 commit」整合

## §2 PC-7α 実施内容

### §2.1 編集 1 (llvkloader.cpp DeviceLimits struct +4 field、line 349-378)

旧 7 field (= maxBoundDescriptorSets / maxPushConstantsSize / maxPushDescriptors / maxPerStageDescriptorSampledImages / maxColorAttachments / maxDescriptorSetSamplers / pushDescriptorSupported + memoryBudgetSupported) → 新 11 field = `maxDescriptorSetUniformBuffers` + `maxDescriptorSetUniformBuffersDynamic` + `maxPerStageDescriptorUniformBuffers` + `minUniformBufferOffsetAlignment` 追加。design 07 §3.1 / §3.2 (V1') 整合。

PC-7α tag block で「V3a 5-set 適合判定」明文化。fail-safe (= set=1 不適合 device で起動 abort or 追加 split) は PC-7δ で実装、本 PC-7α は query + log のみ。

### §2.2 編集 2 (queryAndLogDeviceLimits +4 populate + 4 LL_INFOS、line 532-540 + 575-594)

`VkPhysicalDeviceLimits` from `vkGetPhysicalDeviceProperties2` から 4 field populate + LL_INFOS 出力 (= startup smoke で device 値観察可能化)。本 PC-7α では分岐 logic 追加なし (= Vulkan 1.3 spec 最小 72 ≥ 40 で全 device 必ず 1 set 適合、V1' default 採用済)。

### §2.3 編集 3 (anonymous ns V3a 宣言 +66 行、line 506-571)

PC-7α tag block で V3a 5-set 設計準拠を明文化 + 既存 PC-6α-β-γ object 群との並存方針 (Z1-B + Z3-A) + X2-B sampler 除外 + GATE-B / MUSEUBO-A 整合根拠を comment 記録。

新規宣言:
- 5 binding count constexpr (= `V3A_FRAME_SET_BINDINGS=4` / `V3A_PROGRAM_SET_A_BINDINGS=40` / `V3A_PROGRAM_SET_B_BINDINGS=40` / `V3A_DRAW_SET_BINDINGS=4` / `V3A_ASSET_SET_BINDINGS=3`)
- 4 pool maxSets constexpr (= `V3A_FRAME_POOL_MAX_SETS=FRAMES_IN_FLIGHT=3` / `V3A_PROGRAM_POOL_MAX_SETS=6` / `V3A_DRAW_POOL_MAX_SETS=1` / `V3A_ASSET_POOL_MAX_SETS_INI=64*FRAMES_IN_FLIGHT=192`、design 07 §6.3)
- 5 VkDescriptorSetLayout = `sFrameUboLayoutV3a` / `sProgramUboLayoutA` / `sProgramUboLayoutB` / `sDrawUboLayoutV3a` / `sAssetUboLayoutV3a`
- 4 VkDescriptorPool = `sFrameUboPoolV3a` / `sProgramUboPoolV3a` / `sDrawUboPoolV3a` / `sAssetUboPoolV3a`
- 1 VkPipelineLayout = `sAYAStandardLayout`

### §2.4 編集 4 (3 create helper 関数 +190 行、anonymous ns 内)

#### `createV3aDescriptorSetLayouts()`

lambda `build_ubo_layout()` で共通 layout 生成 (= descriptor_type + binding_count 引数化)、5 件呼出で 5 layout 作成。stage flag = `VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT` (= 既存 `sPerFrameDescriptorSetLayout` と整合)。X2-B 整合 = 全 binding `UBO` or `UBO_DYNAMIC`、`COMBINED_IMAGE_SAMPLER` 不使用。

#### `createV3aDescriptorPools()`

lambda `build_pool()` で共通 pool 生成、4 件呼出で 4 pool 作成。`FREE_DESCRIPTOR_SET_BIT` 不付与 (= grow only、shutdown 時 reverse 順 destroy で十分、design 07 §6.4 整合)。

| pool | descriptor_type | maxSets | descriptorCount |
|---|---|---|---|
| sFrameUboPool | UBO | 3 | 4 × 3 = 12 |
| sProgramUboPool | UBO | 6 | (40 + 40) × 3 = 240 |
| sDrawUboPool | UBO_DYNAMIC | 1 | 4 × 1 = 4 |
| sAssetUboPool | UBO | 192 | 3 × 192 = 576 |

#### `createAYAStandardPipelineLayout()`

`v3a_set_layouts[5]` array (= set=0/1a/1b/2/3) + push constant range (= VERTEX|FRAGMENT, offset=0, size=64 B = modelview matrix 1 件) を既存 `createStandardPipelineLayout` helper に渡して `sAYAStandardLayout` 生成。

注: design 07 §9.1 + §2.3 + §4.4 + §9.2 = vkCmdBindDescriptorSets 時の同時 bind 数は **4 set 上限** (= maxBoundDescriptorSets=4 死守)、pipeline layout 自体は **5 set 全て** を包含する logical layout、実 bind 時 set=3 swap で 4 set 構成 (= PC-7δ scope)。

### §2.5 編集 5 (initVulkan 配線 +12 行、createPerFrameDescriptorSets 後)

`createV3aDescriptorSetLayouts() || createV3aDescriptorPools() || createAYAStandardPipelineLayout()` short-circuit OR で 3 helper 連続呼出、failure 時 shutdownVulkan() + return false。

### §2.6 編集 6 (shutdownVulkan 配線 +50 行、sPerMaterialDescriptorSet 後)

V3a 5-set scaffolding teardown = pipeline layout → 4 pool → 5 layout の reverse 順 destroy。各 object は VK_NULL_HANDLE check で skip safe。

## §3 PC-7β entry conditions

### §3.1 PC-7β scope

**Title**: UboInstance buffer materialization (= VkBuffer / mapped_ptr / size triple-buffer 一括確保)

**Source doc**:
- design 06b §3.2.3 (UboInstance struct 完成形 = std::atomic<bool> dirty + VkBuffer + mapped_ptr + size)
- design 07 §7 (ring buffer 4 MB initial / 16 MB max) — 但し PC-7β は per-program / per-asset / per-skin の **個別 buffer** (= ring buffer は per-draw set=2 で別経路、PC-7ε scope)
- design 07 §8.4 (FRAMES_IN_FLIGHT=3 同期 rotate)
- AYA (X3-A) literal「OK」(2026-06-05) = triple-buffer 一括確保採用

**Touchpoint** (予測):
- `indra/llrender/llvkloader.cpp` UboInstance struct (line 434-441) member 拡充 = std::atomic<bool> dirty + **VkBuffer vk_buffer[FRAMES_IN_FLIGHT]** + **void* mapped_ptr[FRAMES_IN_FLIGHT]** + **VmaAllocation allocation[FRAMES_IN_FLIGHT]** + **uint32_t size** 追加
- sProgramUboDirty / sAssetUboDirty / sSkinUboDirty 3 map の値型変更整合 (= 3 map insert site すべて triple-buffer allocate 経由化)
- 既存 LLAssetUboPool / LLUboRingBuffer (= PC-6α/β 配線) との重複回避 (= 別経路 dirty propagation 用 individual buffer、ring buffer 統合は PC-7ε で別途)
- UboInstance allocate/destroy lifecycle = shader / asset / skin owner register 時 allocate + unregister 時 destroy、PC-7β 内では shutdown 全件 destroy のみ (= per-owner lifecycle hook は PC-7γ で配線)

### §3.2 PC-7β Exit Criteria (= 次 session 着手前整理、本 §3.2 は草案)

- (i) UboInstance struct member 拡充 = std::atomic<bool> dirty + VkBuffer vk_buffer[FRAMES_IN_FLIGHT] + void* mapped_ptr[FRAMES_IN_FLIGHT] + VmaAllocation allocation[FRAMES_IN_FLIGHT] + uint32_t size + Vulkan device コンテキスト reference 不要 (= LLVKLoader 関数経由 alloc)
- (ii) UboInstance 内 triple-buffer allocate/destroy helper = `bool allocateUboInstanceBuffers(UboInstance&, uint32_t size)` + `void destroyUboInstanceBuffers(UboInstance&)` 新設、vmaCreateBuffer + vmaMapMemory + vmaUnmapMemory + vmaDestroyBuffer 3 件 × N owner
- (iii) shutdownVulkan で 3 dirty map 全件 destroyUboInstanceBuffers 呼出 (= sProgramUboDirty + sAssetUboDirty + sSkinUboDirty)
- (iv) X3-A 整合: FRAMES_IN_FLIGHT=3 一括確保、map insert 時 3 buffer 同時 vmaCreateBuffer (= 部分 allocate / late allocate 不採用)
- (v) HOST_VISIBLE + HOST_COHERENT + MAPPED (= 既存 sPerFrameUboBuffer と同 VMA 設定、persistent map 保持) で memcpy 直書き化準備
- (vi) MUSEUBO-A 整合 = allocate path は init / per-owner register hook 経由のみ、bind 未通電 = 既存 OpenGL 描画 100% 維持
- (vii) GATE-B 整合 = #ifdef LL_VULKAN_GLSL 新規追加 0
- (viii) llrender build PASS + warning 0 + TUT 11+10+13 + codegen 130/130 全 PASS
- (ix) forwardToUboUpload 本格化 (= dirty=true 経路 + memcpy) は PC-7γ scope = 本 PC-7β では allocate / destroy のみ wired、map insert / write 未配線

### §3.3 PC-7β 着手前確認候補 (= 次 session で再 confirm)

- **(Y1)** UboInstance 内 VkBuffer 配列 layout = `VkBuffer vk_buffer[FRAMES_IN_FLIGHT]` (3 別 buffer) vs `VkBuffer vk_buffer + uint32_t per_frame_offset[FRAMES_IN_FLIGHT]` (1 buffer + offset) → Claude 推奨 default = (Y1-A) 3 別 buffer = sPerFrameUboBuffer 配線同形、map race 回避明示
- **(Y2)** UboInstance buffer size 決定 = 各 UBO block size (= ubo_metadata.inl `block_size` field) を初期 size として individual allocate vs 固定 max size (例 1024 B) 統一 → Claude 推奨 default = (Y2-A) per-block size (= memory 節約 + ubo_metadata.inl source of truth 整合)
- **(Y3)** per-owner allocate timing = init 時 全件先回り vs 初回 forwardToUboUpload 時 lazy allocate → Claude 推奨 default = (Y3-A) shader / asset / skin owner register 時 (= PC-7γ で per-owner hook 配線、本 PC-7β では shutdown teardown のみ + register hook の skeleton)

## §4 self-verify 9 観点 (= AYA 確認依頼前)

1. **Exit Criteria 9 項全充足** ✅ §2 で全件記録 (i)..(ix)
2. **5 layout source doc 整合** ✅ design 06c §2.2-§2.5 binding count (= 4/40/40/4/3) + descriptor_type (= UBO × 4 + UBO_DYNAMIC × 1) + design 07 §3.2 (V1') 整合
3. **(X1-B / X2-B / X3-A / Z1-B / Z2-C / Z3-A) 採用根拠 record** ✅ §1.2 + §1.3 で 6 件確認 + 採用根拠 5 件 明文化
4. **GATE-B 整合** ✅ #ifdef LL_VULKAN_GLSL 新規追加 0 (= grep 確認、comment 1 件のみ = 既存 GATE-B 整合宣言、code path 追加なし)
5. **MUSEUBO-A 整合** ✅ scaffolding は Vulkan init 層単独、bind 未通電 (= vkCmdBindDescriptorSets 未呼出)、mUseUBO=false default で既存 OpenGL 描画 100% 維持
6. **llrender build + TUT 11+10+13 + codegen 130/130 全 PASS** ✅ 本 session 内で literal 検証取得
7. **commit 内容** = 1 modified (llvkloader.cpp +375 行) + 1 new doc (本 handoff) + 新 file 0 + CMake 改変 0 + settings.xml 改変 0 + Co-Authored-By 不在
8. **feedback_no_scope_shrink 遵守** ✅ PC-7α literal scope (= Vulkan object scaffolding) 完全実施、bind 通電 + UboInstance 拡充 + forwardToUboUpload 本格化 + dynamic offset の sub-step 分離は AYA (X1-B) 確認済の 5 分割で scope 縮小ではない
9. **feedback_design_phase_no_code_write 整合** ✅ 本 PC-7α は実装 phase (= PC-6ζ commit 後)、indra/ 改変 1 件 = 設計 phase ではない

## §5 Phase 1.C 進行 state + 残線形

### §5.1 milestone state

- Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅
- PC-0 ✅ + PC-1 ✅ + PC-2 ✅ + PC-3 ✅ + PC-4 ✅ + PC-5 ✅
- PC-6α ✅ + PC-6β ✅ + PC-6γ ✅ + PC-6δ ✅ + PC-6δ' ✅ + PC-6ε-1 ✅ + PC-6ε-2 ✅ + PC-6ε-3 ✅ + PC-6ζ ✅
- **PC-7α ✅ 本 commit**
- PC-7β..PC-7ε ⏳ 次 session 以降
- PC-7α' (= codegen V1' update、Z2-C 持越) ⏳ 別 sub-task
- PC-8 (= 3 OS build verify) ⏳
- PC-N (= Phase 1.C complete marker + Phase 1.D / Phase 2 着手起点) ⏳

### §5.2 残 strict 線形

- **PC-7β** (= UboInstance member 拡充 + triple-buffer 一括確保) → 本 handoff §3 scope
- **PC-7γ** (= forwardToUboUpload 本格化 = dirty=true 経路有効化 + memcpy + dirty map insert + per-owner register hook)
- **PC-7δ** (= vkCmdBindDescriptorSets 通電 + set=3 swap + sAYAStandardLayout 経由 bind = 既存 placeholder bind path から V3a layout へ移行)
- **PC-7ε** (= dynamic offset 経路 ring buffer chunk hand-off = set=2 per-draw を sDrawUboRingBufferMgr 経由 dynamic offset 計算 + pDynamicOffsets[4] 配線)
- **PC-7α'** (= codegen ubo_metadata.inl V1' update、scripts/ubo_codegen/main.py で set=1a/1b split 実装 + ubo_metadata.inl 再生成 + 130 件 unittest 回帰確認) — PC-7δ 通電前に必要
- **PC-8** (= 3 OS build verify、Linux primary + Win/Mac 後段)
- **PC-N** (= Phase 1.C complete marker + Phase 1.D / Phase 2 着手起点)

## §6 feedback rule 遵守 record

- **feedback_proactive_handoff 遵守** = PC-7β 引継 marker 本 handoff、AYA literal「handoff 起案して commit してください」(2026-06-05) 受領で起案
- **feedback_handoff_minimal_pre_req_read 遵守** = §0 必読 3 件 + pinpoint reference 別記、全箇条書きリスト不採用
- **feedback_self_verify_before_handoff 遵守** = §4 で 9 観点 self-verify 全 ✅
- **feedback_build_only_verified 遵守** = llrender build + TUT 11/11 + 10/10 + 13/13 + codegen 130/130 で literal 検証取得
- **feedback_no_scope_shrink 遵守** = PC-7α literal scope = scaffolding 純化 (= AYA (X1-B) 5 分割確認済)、scope 縮小ではない
- **feedback_doubt_self_first 遵守** = (X1)(X2)(X3) 3 件 ambiguity + (Z1)(Z2)(Z3) 3 件 state mismatch 発見で停止 + 推奨案提示 + AYA 確認 + literal「OK」受領後実装
- **feedback_confirm_referent_before_acting 遵守** = 同上、6 件確認、推測実装なし
- **feedback_ubo_migration_one_at_a_time 遵守** = PC-7α = VkObject scaffolding 単独実施、UboInstance 拡充 / forwardToUboUpload 本格化 / bind 通電 / dynamic offset は PC-7β..ε へ分離
- **feedback_design_phase_no_code_write 整合** = 本 PC-7α は実装 phase (= PC-6ζ commit 後)、indra/ 改変 1 件 = 設計 phase ではない
- **feedback_release_branch_workflow 遵守** = feature branch feature/ayastorm-r41-gl-removal 上 commit
- **feedback_no_auto_commit 遵守** = AYA 明示 commit 指示「handoff 起案して commit してください」literal 受領 (2026-06-05) 後 commit
- **feedback_no_claude_coauthor 遵守** = Co-Authored-By 行不在
