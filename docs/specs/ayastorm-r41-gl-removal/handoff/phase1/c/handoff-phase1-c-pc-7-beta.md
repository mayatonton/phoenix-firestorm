---
title: r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-7β complete + PC-7γ 引継 marker
date: 2026-06-05
status: complete (PC-7β) / pending (PC-7γ..ε)
parent_handoff: handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-alpha.md
---

# PC-7β complete + PC-7γ 引継 marker

## §0 必読 (= 次 session 着手前 minimum 3 件)

1. **本 doc 全文** (= PC-7β 実施内容 + PC-7γ scope + (Y1-A)(Y2-A)(Y3-A)(Y4-A) AYA 確認 record + PC-7γ 着手前確認候補)
2. **design/06b-cadence-update-site-and-dirty.md §3.2 + §3.2.3 + §5.2 + §5.3 + §5.4** (= UBO physical instance dirty bit + forwardToUboUpload routing 5 case 分岐 + per-owner register hook 配線形)
3. **design/06a-cache-structure-and-setter-redirect.md §5.4** (= setter 側 mUseUBO 分岐 + forwardToUboUpload 呼出 site source of truth)

**pinpoint reference** (= 必要時 Read):
- `indra/llrender/llvkloader.cpp` 編集 3 箇所 (= UboInstance struct line 444-481 = member 拡充 / allocateUboInstanceBuffers + destroyUboInstanceBuffers helper line 1248-1346 / shutdownVulkan teardown 配線 line 3345-3361)
- `indra/llrender/llglslshader.cpp` forwardToUboUpload stub 既存位置 (= Phase 1.B 既存 stub、PC-7γ で本格化)
- `build-linux-x86_64/codegen/ubo/ubo_metadata.inl` (= ubo block_size field、PC-7γ allocate call site で per-block size 投入時参照)

## §1 起案契機 + AYA 確認 record

### §1.1 起案契機

PC-7α commit (e9779a256f、2026-06-05) 後、AYA literal「PC-7β 着手お願いします」受領 → 必読 3 件 Read (= PC-7α handoff doc 全文 + design 07 §6 / §8 + design 06b §3.2.3 + llvkloader.cpp 現 UboInstance struct line 444-451) → PC-7β scope 整理で 4 件 ambiguity (Y1)(Y2)(Y3)(Y4) 浮上 → 一括 AYA 確認取得後実装。

### §1.2 AYA 判断確定 record (= 全 4 件)

| ID | 確認内容 | AYA 採用 | literal record |
|---|---|---|---|
| (Y1-A) | UboInstance 内 VkBuffer 配列 layout = `VkBuffer vk_buffer[FRAMES_IN_FLIGHT]` 3 別 buffer (= sPerFrameUboBuffer 同形、map race 回避明示) | 採用 | 「OK」(2026-06-05) |
| (Y2-A) | UboInstance buffer size 決定方式 = per-block size (= ubo_metadata.inl `block_size` field、UBO 毎個別 allocate) | 採用 | 「OK」(2026-06-05) |
| (Y3-A) | allocate/destroy lifecycle PC-7β scope = helper 新設 + shutdown 全件 destroy のみ wired (= PC-7γ で per-owner register hook 配線時に allocate call site 追加) | 採用 | 「OK」(2026-06-05) |
| (Y4-A) | VMA 使用 = `vmaCreateBuffer` (HOST_VISIBLE + HOST_COHERENT + MAPPED + SEQUENTIAL_WRITE) = sDrawUboRingBufferRecords factory / LLAssetUboPool 同形 | 採用 | 「OK」(2026-06-05) |

### §1.3 採用根拠 record

- **(Y1-A) 3 別 buffer 根拠** = 既存 `sPerFrameUboBuffer[FRAMES_IN_FLIGHT]` (`llvkloader.cpp:132`) 同形、design 07 §8.4 FRAMES_IN_FLIGHT 同期 rotate 整合、frame 単位の write race 回避を明示 (= per-frame map 切替で write head と GPU 読込中 buffer が分離)、(Y1-B) 1 buffer + offset 案より dynamic offset 経路 (= PC-7ε scope) と独立設計可能
- **(Y2-A) per-block size 根拠** = ubo_metadata.inl `block_size` field を source of truth として個別 allocate、memory 節約 (= 91 block 平均 ~256 B vs 固定 1024 B = 4 倍節約)、design 06b §3.2.3 `uint32_t size; // ubo_metadata.inl から` literal 整合
- **(Y3-A) helper skeleton 根拠** = PC-7α handoff §3.2 (ix) literal「allocate / destroy のみ wired、map insert / write 未配線」整合、PC-7γ で per-owner register hook (= shader link / asset 構築 / skin 構築) と allocate call site を同 commit で配線、UBO migration one-at-a-time 規律 (= feedback_ubo_migration_one_at_a_time) 遵守
- **(Y4-A) VMA 採用根拠** = PC-6α (LLAssetUboPool) + PC-6β (sDrawUboRingBufferRecords) と同 VMA pattern、persistent map 経由で memcpy 直書き化準備 (= PC-7γ forwardToUboUpload で `memcpy(ubo.mapped_ptr[sFrameIndex], src, ubo.size)` 直結)、VMA budget tracking 自動継承、handoff §3.2 (v) literal「HOST_VISIBLE + HOST_COHERENT + MAPPED」整合

## §2 PC-7β 実施内容

### §2.1 編集 1 (llvkloader.cpp UboInstance struct +33 行 / -6 行、line 444-481)

PC-7α PC-7β tag block で member 拡充根拠を明文化 (= (Y1-A)(Y2-A)(Y3-A)(Y4-A) 引用 + design 06b §3.2.3 完成形 + design 07 §8.3 / §8.4 整合 + member layout 注: dirty std::atomic<bool> move/copy 不可 / VkBuffer 等 trivially destructible / size==0 sentinel)。

旧 struct (PC-6ε-2、std::atomic<bool> dirty 単独 + PC-7 placeholder comment) → 新 struct = dirty + `VkBuffer vk_buffer[FRAMES_IN_FLIGHT]` + `VmaAllocation allocation[FRAMES_IN_FLIGHT]` + `void* mapped_ptr[FRAMES_IN_FLIGHT]` + `uint32_t size`。default 初期化値 = `{ VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE }` / `{ nullptr, nullptr, nullptr }` / `0`。

### §2.2 編集 2 (llvkloader.cpp allocate / destroy helper 新設 +99 行、line 1248-1346)

`createDrawUboRingBuffer` 直後の anonymous ns 内 helper 関数群に PC-7β tag block + 2 関数追加。

#### `allocateUboInstanceBuffers(UboInstance& ubo, uint32_t size, const char* owner_tag)`

- signature: `bool`、`[[maybe_unused]]` 属性付与 (= 本 PC-7β scope で call site 未配線、PC-7γ で配線時に attribute 撤去予定、`-Werror=unused-function` 抑止)
- 前条件 check = `sAllocator == VK_NULL_HANDLE || size == 0` で LL_WARNS + false return
- FRAMES_IN_FLIGHT=3 loop で `vmaCreateBuffer` 一括確保:
  - `VkBufferCreateInfo` usage = `VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT`、sharingMode = `EXCLUSIVE`
  - `VmaAllocationCreateInfo` usage = `VMA_MEMORY_USAGE_AUTO`、flags = `HOST_ACCESS_SEQUENTIAL_WRITE_BIT | MAPPED_BIT`、requiredFlags = `HOST_VISIBLE | HOST_COHERENT` (= sDrawUboRingBufferRecords factory 同形)
- failure path: 失敗 frame の VK_NULL_HANDLE check + 確保済 prev frame 全件 vmaDestroyBuffer で巻き戻し + false return (= 部分 allocate state を残さない)
- success 時 `ubo.size = size` で sentinel 解除

#### `destroyUboInstanceBuffers(UboInstance& ubo)`

- signature: `void`、属性なし (= shutdownVulkan で entry 走査呼出済)
- 早期 return = `sAllocator == VK_NULL_HANDLE || ubo.size == 0` (= unallocated sentinel safe)
- FRAMES_IN_FLIGHT=3 loop で `vmaDestroyBuffer` 全件呼出 + member field を VK_NULL_HANDLE / nullptr reset
- `ubo.size = 0` で sentinel 復帰

### §2.3 編集 3 (llvkloader.cpp shutdownVulkan 配線 +15 行 / -4 行、line 3345-3361)

旧 (PC-6ε-2、`sProgramUboDirty.clear() + sAssetUboDirty.clear() + sSkinUboDirty.clear()` のみ) → 新:

- PC-6ε-2 + PC-7β tag block で「UboInstance::vk_buffer[FRAMES_IN_FLIGHT] / allocation[] / mapped_ptr[] / size を triple-buffer 一括確保 (= AYA (Y1-A)(Y3-A)(Y4-A) 確認)、本 PC-7β は helper skeleton + teardown のみ wired = call site は PC-7γ で配線」明文化
- entry 走査 3 行追加 = `for (auto& kv : sProgramUboDirty) { destroyUboInstanceBuffers(kv.second); }` + 同 sAssetUboDirty + 同 sSkinUboDirty、その後 `.clear()`
- init reverse 順 = sAssetUboPoolMgr / sAllocator destroy より前 (= sAllocator 生存中に発火 = vmaDestroyBuffer safe) 維持
- 本 PC-7β scope では allocate call site 未配線で entry は空のまま (= ubo.size==0 sentinel で no-op safe)、PC-7γ 通電後の forward-safe な teardown 形を本 sub で確立

## §3 PC-7γ entry conditions

### §3.1 PC-7γ scope

**Title**: forwardToUboUpload 本格化 + dirty map insert + per-owner register hook + allocate call site 配線

**Source doc**:
- design 06b §3.2 (二段階 dedup 構造) + §3.2.2 (mValue cache miss → forwardToUboUpload) + §5.2 (forwardToUboUpload routing 5 case 分岐) + §5.3 (cadence 別 flush gate) + §5.4 (thread-safe 要件)
- design 06a §5.4 (setter 内 mUseUBO 分岐 + forwardToUboUpload 呼出 site source of truth)
- PC-7α handoff §3.1 (Z1-B / Z3-A 並存方針 = 旧 PC-6α/β/γ 配線 vs V3a 5-set scaffolding)

**Touchpoint** (予測):
- `indra/llrender/llglslshader.cpp` forwardToUboUpload stub 本格化 (= 現 Phase 1.B stub) = dirty=true 経路有効化 + per-cadence dirty map insert (= try_emplace key 経由) + per-owner allocate hook 呼出
- per-owner register hook 配線:
  - shader: LLGLSLShader 構築完了 / link 完了時 (= `mUseUBO=true` 確定 site) で sProgramUboDirty[shader] を try_emplace + allocateUboInstanceBuffers 呼出
  - asset: LL::GLTF::Asset 構築 / scene 投入時 (= 既存 LLAssetUboPool acquire site 同等) で sAssetUboDirty[asset] を try_emplace + allocateUboInstanceBuffers 呼出
  - skin: LL::GLTF::Skin 構築 / avatar attach 時 で sSkinUboDirty[skin] を try_emplace + allocateUboInstanceBuffers 呼出
- per-owner unregister hook = shader destruct / asset destruct / skin destruct site で destroyUboInstanceBuffers + map erase
- ubo_metadata.inl `block_size` field 参照経路 = forwardToUboUpload 内で `loc.block_hash` から block lookup → block_size を allocateUboInstanceBuffers の size 引数に投入
- `[[maybe_unused]]` attribute 撤去 (= allocateUboInstanceBuffers call site 配線後)

### §3.2 PC-7γ Exit Criteria (= 次 session 着手前整理、本 §3.2 は草案)

- (i) forwardToUboUpload stub → 本格化 (= 5 cadence case 分岐 + per-cadence dirty map insert + memcpy(mapped_ptr[sFrameIndex], src, size) + dirty.store(true))、design 06b §3.2.2 + §5.2 整合
- (ii) per-owner register hook 3 件配線 = shader link / asset 構築 / skin 構築 site で try_emplace + allocateUboInstanceBuffers 呼出、size = ubo_metadata.inl block_size 投入
- (iii) per-owner unregister hook 3 件配線 = shader / asset / skin destruct site で destroyUboInstanceBuffers + map erase
- (iv) `[[maybe_unused]]` 属性撤去 (= allocateUboInstanceBuffers call site 通電)
- (v) flush 関数 (= flushProgramUbos / flushAssetUbos / flushSkinUbos) dirty.exchange(false) 後の本格 upload path は本 PC-7γ scope 外 (= PC-7δ で vkCmdBindDescriptorSets 通電と一体)、本 PC-7γ では memcpy までで stop
- (vi) MUSEUBO-A 整合 = per-program entry gate (= shader->mUseUBO 明示参照、PC-6ε-2 確定) + per-asset/skin 構造的 gate (= setter 側 mUseUBO 分岐由来) で既存 OpenGL 描画 100% 維持
- (vii) GATE-B 整合 = #ifdef LL_VULKAN_GLSL 新規追加 0
- (viii) llrender + newview build PASS + warning 0 + TUT 11+10+13 + codegen 130/130 全 PASS
- (ix) vkCmdBindDescriptorSets 通電 + dynamic offset 経路 (= sAYAStandardLayout 経由 bind + pDynamicOffsets[4]) は PC-7δ / PC-7ε scope = 本 PC-7γ では memcpy 完了で stop

### §3.3 PC-7γ 着手前確認候補 (= 次 session で再 confirm)

- **(W1)** per-owner register hook 配線 site 確定 = shader link 完了時 (`LLGLSLShader::link()` or `mUseUBO=true` 確定 site) vs 初回 forwardToUboUpload 内 lazy allocate vs 既存 PC-6α LLAssetUboPool acquire と一体化 → Claude 推奨 default = (W1-A) shader = link 完了 site / asset = LLAssetUboPool acquire 直後 / skin = avatar attach site で個別配線 (= 3 owner 種別ごとに最適化、PC-6α/β 既存 hook 流用可能なところは流用)
- **(W2)** forwardToUboUpload 内 cadence 判定経路 = `loc.cadence` field 直参照 (= design 06b §5.2 5 case 分岐 switch) vs cadence 別 forward 関数 split → Claude 推奨 default = (W2-A) 単一 forwardToUboUpload + switch 内 5 case 分岐 (= 06b §5.2 literal 整合、call site が setter 31 site で 5 case 分岐は callee 側に閉じ込め)
- **(W3)** PC-7γ scope に「flush 関数本格化 (= dirty.exchange + memcpy → GPU 経路)」を含める or PC-7δ へ分離 → Claude 推奨 default = (W3-B) PC-7γ は forwardToUboUpload 本格化 + memcpy までで stop、flush 側 dirty.exchange 後の upload path は PC-7δ で vkCmdBindDescriptorSets 通電と一体実装 (= UBO migration one-at-a-time、handoff 境界での検証可能性維持)

## §4 self-verify 9 観点 (= AYA 確認依頼前)

1. **Exit Criteria 9 項全充足** ✅ §2 で全件記録、handoff §3.2 (i)-(ix) と実装対応 = (i) UboInstance struct member 拡充 + (ii) helper 2 件新設 + (iii) shutdownVulkan 3 map destroy 配線 + (iv) X3-A 整合 FRAMES_IN_FLIGHT=3 一括 + failure 巻き戻し + (v) HOST_VISIBLE + HOST_COHERENT + MAPPED + SEQUENTIAL_WRITE + (vi) MUSEUBO-A 整合 call site 未配線 + (vii) GATE-B 整合 #ifdef LL_VULKAN_GLSL 0 + (viii) llrender build + TUT + codegen 全 PASS + (ix) forwardToUboUpload 本格化 PC-7γ scope
2. **UboInstance struct source doc 整合** ✅ design 06b §3.2.3 完成形 (`VkBuffer vk_buffer` + `void* mapped_ptr` + `uint32_t size` + `std::atomic<bool> dirty`) を triple-buffer 拡張形 (= (Y1-A) FRAMES_IN_FLIGHT=3 配列化) で実装、design 07 §8.3 (per-program/asset/skin triple-buffering 必須) + §8.4 (frame index 共有) 整合
3. **(Y1-A)(Y2-A)(Y3-A)(Y4-A) AYA 確認 record** ✅ §1.2 + §1.3 で 4 件確認 + 採用根拠 4 件明文化、AYA literal「OK」(2026-06-05)
4. **GATE-B 整合** ✅ #ifdef LL_VULKAN_GLSL 新規追加 0 (= `git diff | grep "^\+" | grep LL_VULKAN_GLSL` で 0 件確認、scaffolding は Vulkan init 層単独、mUseUBO runtime gate 不参照、PC-6α..ζ + PC-7α 同形)
5. **MUSEUBO-A 整合** ✅ allocateUboInstanceBuffers call site 未配線 (= [[maybe_unused]] 属性付与) で 3 dirty map 空のまま、forwardToUboUpload 経路 PC-7γ scope = mUseUBO=false default で既存 OpenGL 描画 100% 維持、destroy 側は no-op safe (= ubo.size==0 sentinel)
6. **llrender build + TUT 11+10+13 + codegen 130/130 全 PASS** ✅ 本 session 内で literal 検証取得 (= make -j4 llrender PASS + warning 0 / INTEGRATION_TEST_lluboringbuffer 11/11 YAY / INTEGRATION_TEST_llassetubopool 10/10 YAY / INTEGRATION_TEST_llpipelinecachestorage 13/13 YAY / python3 -m unittest discover 130/130 OK)
7. **commit 内容** = 1 modified (llvkloader.cpp +147 / -8 行) + 1 new doc (本 handoff) + 新 file 0 + CMake 改変 0 + settings.xml 改変 0 + Co-Authored-By 不在
8. **feedback_no_scope_shrink 遵守** ✅ PC-7β literal scope (= UboInstance member 拡充 + triple-buffer 一括確保 helper + shutdown 配線) 完全実施、allocate call site の PC-7γ への分離は AYA (Y3-A) 確認済 + PC-7α handoff §3.2 (ix) literal「allocate / destroy のみ wired」整合で scope 縮小ではない
9. **feedback_design_phase_no_code_write 整合** ✅ 本 PC-7β は実装 phase (= PC-7α commit 後)、indra/ 改変 1 件 = 設計 phase ではない

## §5 Phase 1.C 進行 state + 残線形

### §5.1 milestone state

- Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅
- PC-0 ✅ + PC-1 ✅ + PC-2 ✅ + PC-3 ✅ + PC-4 ✅ + PC-5 ✅
- PC-6α ✅ + PC-6β ✅ + PC-6γ ✅ + PC-6δ ✅ + PC-6δ' ✅ + PC-6ε-1 ✅ + PC-6ε-2 ✅ + PC-6ε-3 ✅ + PC-6ζ ✅
- PC-7α ✅ + **PC-7β ✅ 本 commit**
- PC-7γ..PC-7ε ⏳ 次 session 以降
- PC-7α' (= codegen V1' update、Z2-C 持越) ⏳ 別 sub-task
- PC-8 (= 3 OS build verify) ⏳
- PC-N (= Phase 1.C complete marker + Phase 1.D / Phase 2 着手起点) ⏳

### §5.2 残 strict 線形

- **PC-7γ** (= forwardToUboUpload 本格化 + dirty map insert + per-owner register hook + allocate call site 配線) → 本 handoff §3 scope
- **PC-7δ** (= vkCmdBindDescriptorSets 通電 + set=3 swap + sAYAStandardLayout 経由 bind = 既存 placeholder bind path から V3a layout へ移行 + flush 側 dirty.exchange 後の GPU 経路)
- **PC-7ε** (= dynamic offset 経路 ring buffer chunk hand-off = set=2 per-draw を sDrawUboRingBufferMgr 経由 dynamic offset 計算 + pDynamicOffsets[4] 配線)
- **PC-7α'** (= codegen ubo_metadata.inl V1' update、scripts/ubo_codegen/main.py で set=1a/1b split 実装 + ubo_metadata.inl 再生成 + 130 件 unittest 回帰確認) — PC-7δ 通電前に必要
- **PC-8** (= 3 OS build verify、Linux primary + Win/Mac 後段)
- **PC-N** (= Phase 1.C complete marker + Phase 1.D / Phase 2 着手起点)

## §6 feedback rule 遵守 record

- **feedback_proactive_handoff 遵守** = PC-7γ 引継 marker 本 handoff、AYA literal「handoff 起案して commit してください」(2026-06-05) 受領で起案
- **feedback_handoff_minimal_pre_req_read 遵守** = §0 必読 3 件 + pinpoint reference 別記、全箇条書きリスト不採用
- **feedback_self_verify_before_handoff 遵守** = §4 で 9 観点 self-verify 全 ✅
- **feedback_build_only_verified 遵守** = llrender build + TUT 11/11 + 10/10 + 13/13 + codegen 130/130 で literal 検証取得
- **feedback_no_scope_shrink 遵守** = PC-7β literal scope = UboInstance member 拡充 + helper skeleton + teardown 配線 (= AYA (Y3-A) 確認済の skeleton 性質)、scope 縮小ではない
- **feedback_doubt_self_first 遵守** = (Y1)(Y2)(Y3)(Y4) 4 件 ambiguity 発見で停止 + 推奨案提示 + AYA 確認 + literal「OK」受領後実装
- **feedback_confirm_referent_before_acting 遵守** = 同上、4 件 batch AYA 確認、推測実装なし
- **feedback_ubo_migration_one_at_a_time 遵守** = PC-7β = UboInstance member 拡充 + helper skeleton + teardown 単独実施、forwardToUboUpload 本格化 + per-owner register hook + allocate call site は PC-7γ へ分離、bind 通電 + dynamic offset は PC-7δ..ε へ分離
- **feedback_design_phase_no_code_write 整合** = 本 PC-7β は実装 phase (= PC-7α commit 後)、indra/ 改変 1 件 = 設計 phase ではない
- **feedback_release_branch_workflow 遵守** = feature branch feature/ayastorm-r41-gl-removal 上 commit
- **feedback_no_auto_commit 遵守** = AYA 明示 commit 指示「handoff 起案して commit してください」literal 受領 (2026-06-05) 後 commit
- **feedback_no_claude_coauthor 遵守** = Co-Authored-By 行不在
