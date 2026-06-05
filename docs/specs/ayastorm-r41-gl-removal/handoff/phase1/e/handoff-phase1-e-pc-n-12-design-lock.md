# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.E **PC-N-12 design-lock complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: Phase 1.E 内 **2nd sub-step = PC-N-12 = real node modelview** (= identity push constant modelview 卒業 + real `Asset::mNodes[node_index].mAssetMatrix` 経由 modelview matrix 通電 + `AYAGltfRealModelviewEnabled` cvar 新設) の design-lock phase 完了 marker = ambiguity (N12-1)..(N12-16) 16 件 全 AYA literal「全件推奨で OK」record (2026-06-05) + 実装計画 (a)-(h) 8 step 分解 + Exit Criteria 9+10 項明文化。`indra/` 改変 0 件 (= `feedback_design_phase_no_code_write` 整合)。

> **本 doc 位置付け**: PC-N-12 詳細 design-lock。Phase 1.E decomposition design-lock (= `handoff-...-phase1-e-decomposition-design-lock.md`) で確立した PC-N-11..PC-N-15 5 sub-step 分解 + 16 件 ambiguity (E-1)..(E-16) AYA 全件推奨採用 baseline 上に、PC-N-12 単独の **詳細実装計画** + **想定 code diff example** + **ambiguity 16 件 (N12-1)..(N12-16)** + **Exit Criteria 10 項** を確定。PC-N-11 design-lock doc と同形 pattern 踏襲 (= `handoff-...-phase1-e-pc-n-11-design-lock.md`)。実装は別 session で別途着手 (= `feedback_ubo_migration_one_at_a_time` 厳格遵守)。

---

## §0. 本 session 着手契機 + literal scope record

**契機**: AYA 指示「r41 Phase 1.E PC-N-12 design-lock 着手お願いします。直前 commit = `797332ee81` (PC-N-11 complete = Phase 1.E 内 1st sub-step 実装完了 = multi-skin real Skin path 通電)。必読 1 件: `handoff-...-phase1-e-pc-n-11-complete.md`。PC-N-12 scope (= Phase 1.E 内 2nd sub-step = real node modelview): identity push constant modelview → real `Asset::mNodes[node_index].mMatrix` 経由 (= `recordGltfAssetDraw` 内 push constant 64 B identity block 卒業) + `AYAGltfRealModelviewEnabled` cvar 新設 (Boolean default=0 Persist=1、`AYAGltfMultiSkinEnabled` 直後並列 = Phase 1.E cvar group) + ambiguity 出し + 推奨案提示 + AYA 確認 → design-lock doc 起案 (`indra/` 改変 0 件、`feedback_design_phase_no_code_write` 遵守)。GATE-B: `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (count llvkloader.cpp=6 維持)。MUSEUBO-A: `AYAGltfRealModelviewEnabled=false` default で既 identity modelview 維持 + OpenGL 描画 100% 維持。`feedback_ubo_migration_one_at_a_time` 厳格遵守 (PC-N-13..PC-N-15 は別 session)。`feedback_self_verify_before_handoff` 遵守。」literal 受領 (2026-06-05、PC-N-11 complete commit `797332ee81` 後の継続 session = 別 session の fresh context)。

**PC-N-12 literal scope** (= AYA task statement 直訳、4 項):

1. **identity push constant modelview 卒業** = `recordGltfAssetDraw` PC-N-8 (f) 内 line 6018-6031 の `real_asset_identity_modelview[16]` を `vkCmdPushConstants` VERTEX_BIT 64 B 投入する block を `<AYAstorm r41 PC-N-12 (a)>` tag block で wrap + cvar guard + sCurrentNodeIndex guard + bounds check + real Node.mAssetMatrix 投入分岐 ((N12-1) A + (N12-7) A + (N12-8) A + (N12-9) A 採用)
2. **real `Asset::mNodes[node_index].mAssetMatrix` 経由 modelview matrix 通電** = upstream `Asset::uploadTransforms` line 180 `t_mp[i] = node.mAssetMatrix` と同 source 流用 ((N12-1) A 採用 = mAssetMatrix = transform from local to asset space = parent chain 合成済、`mMatrix` は local-only ゆえ非 root node で誤)
3. **`AYAGltfRealModelviewEnabled` cvar 新設** = Boolean default=0 Persist=1 ((N12-3) A 採用)、settings.xml `AYAGltfMultiSkinEnabled` 直後並列 = Phase 1.E cvar group 連続配置 ((N12-4) A 採用)
4. **`setCurrentNodeIndex(S32) / clearCurrentNodeIndex() / getCurrentNodeIndex()` 新規 accessor 3 件 + sCurrentNodeIndex static + caller-side gltfscenemanager.cpp per-Primitive 配線** ((N12-2) A + (N12-14) A 採用 = `Primitive` 自体は `mNodeIndex` を持たず、`RenderBatch::PrimitiveData::mNodeIndex` (asset.h:332-336) のみ source ゆえ caller-side accessor 配線必須)

**Phase 境界**: PC-N-12 完了 = Phase 1.E 内 2nd sub-step 完了 = real Node modelview 経由 push constant 通電 baseline 確立。zero-buffer `PerDrawUBO_LightParams` 卒業 + multi-asset verify は PC-N-13 持越し、worker thread design + 実装は PC-N-14/15 持越し、sentinel storage 撤去 + Phase 1.E complete marker は PC-N-15 持越し ((E-11) A + (E-14) A 整合)。

---

## §1. 必読 1 件 + pinpoint reference

### §1.1 必読 1 件 (= 次 session = PC-N-12 実装 phase 着手前)

1. **本 PC-N-12 design-lock doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-e-pc-n-12-design-lock.md`

### §1.2 pinpoint reference 12 件 (= 実装 phase で必要分のみ Read)

1. **`recordGltfAssetDraw` PC-N-8 (f) push constant identity block**: `indra/llrender/llvkloader.cpp:6018-6031` = PC-N-12 改変対象本体 (= `real_asset_identity_modelview[16]` + `vkCmdPushConstants` 投入 site、現状 comment 行 6019 「PC-N-9 で real node modelview 置換予定」は旧記述 = PC-N-12 で正式 resolve)
2. **`recordGltfAssetDraw` PC-N-8 (f) real Asset path block 全体**: `indra/llrender/llvkloader.cpp:5906-6062` = PC-N-12 (a) tag block 配置場所 = PC-N-11 (a) inner block (line 5956-6015) 直後並列
3. **`LL::GLTF::Node` 構造**: `indra/newview/gltf/asset.h:180-230` = `mat4 mMatrix` (= local transform、identity 初期化) + `mat4 mAssetMatrix` (= transform from local to asset space、parent chain 合成済) + `bool mMatrixValid` フラグ + `mNeedsApplyMatrix` フラグ
4. **`Asset::uploadTransforms` 実装**: `indra/newview/gltf/asset.cpp:164-209` = upstream で `t_mp[i] = node.mAssetMatrix` 採用済 (= line 180)、`mat4` → `F32* m = glm::value_ptr(t_mp[i])` (= line 191) で column-major raw float pointer 取得 pattern
5. **`Asset::mNodes`**: `indra/newview/gltf/asset.h:359` = `std::vector<Node> mNodes` (= Asset 内 Node 配列、`mNodes[node_index]` で O(1) access)
6. **`RenderBatch::PrimitiveData::mNodeIndex`**: `indra/newview/gltf/asset.h:332-336` = `struct PrimitiveData { S32 mPrimitiveIndex; S32 mNodeIndex; }` = caller (gltfscenemanager.cpp) でのみ参照可能、`INVALID_INDEX` で未設定 sentinel
7. **`gltfscenemanager.cpp:741` `Node& node = asset.mNodes[pdata.mNodeIndex]`**: per-Primitive loop body 内 既存 access pattern = PC-N-12 caller-side でも同 source 流用、PC-N-12 で `setCurrentNodeIndex(pdata.mNodeIndex)` 配線時の pdata 参照位置
8. **`gltfscenemanager.cpp:752` `LLVKLoader::setCurrentPrimitive(&primitive)`**: per-Primitive loop body 冒頭 `if (rigged)` 外 unconditional 配線 = PC-N-12 `setCurrentNodeIndex(pdata.mNodeIndex)` も同形 並列配置位置 ((N12-14) A)
9. **`gltfscenemanager.cpp:792` `LLVKLoader::clearCurrentPrimitive()`**: per-Primitive loop body 末尾 unconditional clear = PC-N-12 `clearCurrentNodeIndex()` も同形 並列配置位置
10. **`llvkloader.h:502-516` 既 accessor 宣言群**: `setCurrentAsset/clearCurrentAsset/getCurrentAsset` + `setCurrentSkin/clearCurrentSkin/getCurrentSkin` + `setCurrentPrimitive/clearCurrentPrimitive/getCurrentPrimitive` = PC-N-12 `setCurrentNodeIndex/clearCurrentNodeIndex/getCurrentNodeIndex` 追加位置 (= line 516 直後並列、`<AYAstorm r41 PC-N-12 (b)>` tag block)
11. **`llvkloader.cpp:5746-5765` 既 accessor 実装群**: `setCurrentPrimitive/clearCurrentPrimitive/getCurrentPrimitive` 実装 = PC-N-12 accessor 実装は同 file 内同形配置
12. **settings.xml `AYAGltfMultiSkinEnabled` 配置**: `indra/newview/app_settings/settings.xml:10448-10458` = PC-N-11 で追加済 cvar、PC-N-12 `AYAGltfRealModelviewEnabled` cvar はこの直後並列追加 ((N12-4) A)

---

## §2. 現状調査結果 (= PC-N-11 complete baseline + 改変対象 site 確認)

### §2.1 PC-N-8 (f) 内 push constant identity block (= 改変対象本体)

| # | site | file:line | 現状 | PC-N-12 改変 |
|---|------|-----------|------|-------------|
| 1 | identity modelview 16 float 配列 | `llvkloader.cpp:6020-6025` | `const float real_asset_identity_modelview[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };` | cvar guard 内に移動 = cvar=true + sCurrentNodeIndex valid + bounds OK 時 real Node.mAssetMatrix 投入 / それ以外 identity 維持 |
| 2 | `vkCmdPushConstants` invocation | `llvkloader.cpp:6026-6031` | `vkCmdPushConstants(cmd_buf, sAvatarBoneLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, real_asset_identity_modelview)` | tag block で wrap + push 投入時 source pointer を identity / real のいずれかに分岐 ((N12-7) A 単一 push call、二重投入回避) |
| 3 | 旧 comment 行 6019「PC-N-9 で real node modelview 置換予定」 | `llvkloader.cpp:6019` | 旧記述 (= PC-N-9 は GLTFSceneManager::render 統合 phase ゆえ実際は scope 外) | PC-N-12 (a) tag block 内 new comment で置換 = 「PC-N-12 で real node modelview 配線本 sub-step」明示 |

### §2.2 関連 accessor (= 改変対象周辺)

| # | 項目 | 配置 | 状態 |
|---|------|------|------|
| 4 | `sCurrentAsset` static + accessor | `llvkloader.cpp:667` + `:5715-5728` | PC-7γ-2 既配線済 (= `gltfscenemanager.cpp` 既配線) |
| 5 | `sCurrentPrimitive` static + accessor | `llvkloader.cpp:678` + `:5746-5765` | PC-N-8 (e) 既配線済 (= `gltfscenemanager.cpp:752/792` 既配線) |
| 6 | `sCurrentNodeIndex` static + accessor | **未配置** = PC-N-12 新規配線対象 | new (= `<AYAstorm r41 PC-N-12 (b)>` tag block) |
| 7 | `LL::GLTF::Node::mAssetMatrix` | `asset.h:184` | upstream 既配線済 = `Node::updateTransforms` で `mAssetMatrix = parentMatrix * mMatrix` 計算 (`asset.cpp:119`) |
| 8 | `Asset::uploadTransforms` 経由 per-frame update | `asset.cpp:164` | per-frame `scene.updateTransforms(*this)` (= line 158) で全 Node の mAssetMatrix update + `uploadTransforms()` で UBO 投入、`recordGltfAssetDraw` 発火前に既実行 |

### §2.3 caller-side gltfscenemanager.cpp per-Primitive loop 構造

| # | site | line | 用途 |
|---|------|------|------|
| 9 | `Node& node = asset.mNodes[pdata.mNodeIndex]` access | `gltfscenemanager.cpp:741` | per-Primitive loop body 内既存 access (= GLSL `GLTF_NODE_ID` uniform 投入用、PC-N-12 同 source 流用) |
| 10 | `LLVKLoader::setCurrentPrimitive(&primitive)` | `gltfscenemanager.cpp:752` | PC-N-8 (e) 配線、unconditional (`if (rigged)` 外) = PC-N-12 `setCurrentNodeIndex(pdata.mNodeIndex)` 配線並列位置 |
| 11 | `LLVKLoader::setCurrentSkin(&skin)` | `gltfscenemanager.cpp:765` | PC-7γ-2 配線、`if (rigged)` 内 = PC-N-11 で消費済 (multi-skin path 通電) |
| 12 | `pdata.mNodeIndex` 既参照 | `gltfscenemanager.cpp:775` | GLSL `GLTF_NODE_ID` uniform 投入 (`uniform1i`) で既使用、PC-N-12 では同 source を accessor 経由で Vulkan path に渡す |
| 13 | `LLVKLoader::clearCurrentSkin()` | `gltfscenemanager.cpp:787` | drawRangeFast 直後 unconditional clear (= setCurrentSkin が `if (rigged)` 限定でも safe = nullptr→nullptr no-op) |
| 14 | `LLVKLoader::clearCurrentPrimitive()` | `gltfscenemanager.cpp:792` | 末尾 unconditional clear = PC-N-12 `clearCurrentNodeIndex()` 対称配置位置 |

### §2.4 重大 finding: `LL::GLTF::Primitive` は `mNodeIndex` を保持しない

**(原理)**: `LL::GLTF::Primitive` (= `gltf/primitive.h`) は Mesh の vertex/index 情報のみ保持。Mesh と Node の対応 (node → mesh は 1→1 だが、複数 Node が同 Mesh を参照可、同 Mesh 内の同 Primitive も共有) は `Asset::mScenes[].mNodes[]` の scene tree 経由でのみ resolve 可能。

**(host-side)**: `RenderBatch::PrimitiveData` (`asset.h:332-336`) が **batch 構築時に node_index と primitive_index を pair 保持**、render loop はこの pair を消費。すなわち `pdata.mNodeIndex` が唯一の host-side source。

**(PC-N-12 解決)**: `setCurrentNodeIndex(S32)` 新規 accessor 経由で caller (gltfscenemanager.cpp per-Primitive loop) から `pdata.mNodeIndex` を direct 投入 ((N12-2) A + (N12-14) A 採用)。`Primitive*` 経由 lookup は構造的に不可。

### §2.5 (E-13) commit msg + AYA task statement の正式 resolve ((N12-1) A 採用)

| source | modelview matrix 選択 |
|--------|---------------------|
| Phase 1.E decomposition design-lock commit `094546889b` commit message (E-13) | A: real `Asset::mNodes[node_index].mMatrix` 経由 |
| AYA PC-N-12 task statement (2026-06-05) | literal: real `Asset::mNodes[node_index].mMatrix` 経由 |
| upstream `Asset::uploadTransforms` 実装 (`asset.cpp:180`) | `t_mp[i] = node.mAssetMatrix` (= local→asset space 合成済) |
| **(N12-1) AYA 全件推奨採用結果** | **A**: `node.mAssetMatrix` (= asset 空間、parent chain 合成済) |

**採用根拠**: AYA task statement / commit msg literal は「mMatrix」表記だが、upstream `Asset::uploadTransforms` は `mAssetMatrix` を選択している (= local-only `mMatrix` は parent transform 未合成ゆえ非 root node で誤 modelview)。本 (N12-1) A で AYA task statement の literal「mMatrix」を upstream 実装と整合する `mAssetMatrix` に正式 resolve = AYA 全件推奨 OK record で確定 = 各 sub-step design-lock 時 implementation detail resolve 原則 (= PC-N-11 (N11-1) と同形 pattern)。

---

## §3. ambiguity (N12-1)..(N12-16) 16 件 AYA literal「全件推奨で OK」record (2026-06-05) + 採用根拠

### §3.1 matrix 選択系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N12-1) | modelview source matrix | **A**: `node.mAssetMatrix` (= asset 空間、parent chain 合成済) | OK (2026-06-05) | upstream `Asset::uploadTransforms` line 180 `t_mp[i] = node.mAssetMatrix` と整合、scene graph 根からの絶対 transform、`mMatrix` は local-only ゆえ非 root node で誤、AYA task statement literal「mMatrix」を upstream 実装整合の mAssetMatrix に正式 resolve = §2.5 |
| (N12-10) | mat4 → 64 B 変換 | **A**: `glm::value_ptr(node.mAssetMatrix)` を `vkCmdPushConstants` に raw float* で渡す (= column-major、upstream `Asset::uploadTransforms` line 191 同形 source 流用) | OK (2026-06-05) | upstream 既 source 流用、shader 側 GLSL column-major 慣習整合、`std::memcpy` 経由 64 B 投入は冗長ゆえ raw pointer 直接渡し |

### §3.2 accessor + caller 系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N12-2) | node_index 解決方法 | **A**: `setCurrentNodeIndex(S32) / clearCurrentNodeIndex() / getCurrentNodeIndex()` 新規 accessor 3 件 (= `sCurrentAsset/sCurrentSkin/sCurrentPrimitive` 同形 pattern、main thread 専有ゆえ atomic 不要) | OK (2026-06-05) | `Primitive` 自体は `mNodeIndex` を持たず caller-side `RenderBatch::PrimitiveData::mNodeIndex` のみ source (= §2.4 finding)、accessor 経由が唯一の path、`sCurrent*` static + setter/getter/clear 既 pattern 踏襲 |
| (N12-14) | caller-side gltfscenemanager.cpp 配線 | **A**: PC-N-12 scope 内に `setCurrentNodeIndex(pdata.mNodeIndex)` + `clearCurrentNodeIndex()` per-Primitive loop 配線も同梱 (= PC-N-12 単独で fire 可能化、`setCurrentPrimitive` 直後並列 + `clearCurrentPrimitive` 直前並列、`if (rigged)` 外側 unconditional) | OK (2026-06-05) | PC-N-11 では setCurrentSkin PC-7γ-2 既配線、PC-N-12 は新規 accessor ゆえ caller-side 配線必須、`setCurrentPrimitive` (PC-N-8 (e)) と同形 pattern 踏襲、`if (rigged)` 外配置で non-rigged primitive にも適用 (= real Node modelview は rigged/non-rigged 問わず必要) |

### §3.3 cvar 戦略系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N12-3) | cvar 新設 | **A**: `AYAGltfRealModelviewEnabled` Boolean default=0 Persist=1 (= PC-N-11 (N11-1)/(N11-2) 同形 pattern) | OK (2026-06-05) | 既 r41 cvar pattern 踏襲、MUSEUBO-A 整合 (= default OFF で既 identity modelview 維持 + OpenGL 描画 100% 維持) |
| (N12-4) | settings.xml 配置位置 | **A**: 既 `AYAGltfMultiSkinEnabled` cvar 直後並列 (= Phase 1.E cvar group 連続配置、(N11-10) A 整合) | OK (2026-06-05) | Phase 1.E cvar group 起点維持、PC-N-13/14/15 cvar もここに並べる想定 |
| (N12-5) | cvar Comment 内容 | **B**: 本 cvar 単独説明のみ (= 将来 cvar は当該 sub-step 着手時に追記、(N11-11) B 同形 pattern) | OK (2026-06-05) | 各 sub-step 別 session 別途 design-lock 原則、untouched 領域 silence、`feedback_no_scope_shrink` 違反ではなく現時点で PC-N-13/14/15 cvar literal 確定なし |

### §3.4 PC-N-8 (f) 改変系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N12-6) | tag block 命名 + 配置 | **A**: PC-N-8 (f) 内側 PC-N-11 (a) inner block 直後並列に新規 `<AYAstorm r41 PC-N-12 (a)>` inner tag block 配置 (= surgical insertion、outer PC-N-8 (f) 構造温存) | OK (2026-06-05) | PC-N-11 (a) 同形 pattern 踏襲、code archaeology 容易、各 sub-step 独立 tag block で git blame 解析整合 |
| (N12-7) | cvar guard 範囲 | **A**: `vkCmdPushConstants` 全体を guard 内、cvar=true 時 real modelview 投入 / cvar=false 時 identity 投入 (= 単一 push call、二重投入なし) | OK (2026-06-05) | identity 外置きだと dual push call 発生 + 後段 push が前段を上書きするだけで無駄、単一 push site 内 if-else 分岐で source pointer のみ切替が最小改変 |
| (N12-8) | fall-through path | **A**: cvar=false or `sCurrentNodeIndex == INVALID_INDEX` or `sCurrentAsset == nullptr` or bounds out 時 identity 投入 = MUSEUBO-A 整合、Phase 1.D baseline 不変 | OK (2026-06-05) | skip すると前 draw push value 残存 risk (= vkCmdPushConstants は draw call 跨ぎで保持される)、identity 強制で graceful degrade + GPU 状態決定性確保 |
| (N12-9) | bounds check 範囲 | **A**: 3 段 guard = `sCurrentNodeIndex != INVALID_INDEX` + `sCurrentAsset != nullptr` + `sCurrentNodeIndex < (S32)sCurrentAsset->mNodes.size()` | OK (2026-06-05) | graceful degrade、`mNodes.size()` 0 や node_index out-of-range で crash 防止、defensive programming 整合 |

### §3.5 副次系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N12-11) | first-fire `LL_INFOS` marker | **A**: `s_first_pcn12_real_modelview_fire` atomic flag (= PC-N-6/7/8/9/10/11 同形 pattern) | OK (2026-06-05) | 通電 literal 取得用、PC-N-12 (a) real modelview path first fire 時 1 回出力 + diagnostic 内容 = node_index + mAssetMatrix raw 16 float dump 等、AYA literal 受領用 |

### §3.6 検証 + Exit Criteria 系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N12-12) | build verify scope | **A**: llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 + GATE-B integrity `LL_VULKAN_GLSL count llvkloader.cpp=6` 不変 (= PC-N-11 同形) | OK (2026-06-05) | (E-8) commit msg / (E-16) doc body 整合、既 PC-N-6/7/8/9/10/11 同形 build verify pattern 踏襲 |
| (N12-13) | Exit Criteria 項目数 | **A**: 10 項 (= PC-N-11 同形、実装 phase 整合) | OK (2026-06-05) | PC-N-12 = 実装系 sub-step ゆえ実装 phase Exit Criteria 10 項 pattern 踏襲 |

### §3.7 design-lock phase 整合系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N12-15) | 想定改変 file 件数 (実装 phase) | **A**: 6 file = (1) `indra/llrender/llvkloader.cpp` (= PC-N-8 (f) 内 push constant block を `<AYAstorm r41 PC-N-12 (a)>` tag で wrap + cvar guard + fall-through + first-fire marker + sCurrentNodeIndex static + 3 accessor 実装) + (2) `indra/llrender/llvkloader.h` (= 3 accessor 宣言追加) + (3) `indra/newview/gltfscenemanager.cpp` (= per-Primitive loop に setCurrentNodeIndex/clearCurrentNodeIndex 配線) + (4) `indra/newview/app_settings/settings.xml` (= `AYAGltfRealModelviewEnabled` cvar 1 件追加) + (5) 本 cross-platform spec §6 PC-N-12 行 ✅ 反映 + §A 履歴 1 行追記 + (6) new handoff complete doc 起案 | OK (2026-06-05) | PC-N-11 (4 file) より +2 (header + caller) は構造的必然 (= `Primitive` が `mNodeIndex` を持たない = §2.4 finding)、`feedback_no_scope_shrink` 整合、既 setCurrentPrimitive PC-N-8 (e) 同形 caller-side wire pattern 踏襲 |
| (N12-16) | design-lock phase `indra/` 改変 0 件遵守 | **A**: 本 PC-N-12 design-lock doc 起案 + cross-platform spec §6 PC-N-12 行 design-lock 内容更新のみ、`indra/` 改変 0 件 = `feedback_design_phase_no_code_write` 厳格遵守 | OK (2026-06-05) | 全 design-lock phase 共通 pattern 踏襲 |

---

## §4. 実装計画 (a)-(h) 8 step (= 別 session で着手)

> **注**: 本 §4 は **実装 phase 用 step 分解 + 想定 code diff example** = 本 design-lock phase は `indra/` 改変 0 件、実装は別 session で別途着手 (= `feedback_design_phase_no_code_write` + `feedback_ubo_migration_one_at_a_time` 厳格遵守)。

### §4.1 step (a) — `AYAGltfRealModelviewEnabled` cvar 新設 (settings.xml)

`indra/newview/app_settings/settings.xml` の既 `AYAGltfMultiSkinEnabled` cvar 直後並列 ((N12-4) A) で `AYAGltfRealModelviewEnabled` cvar 1 件追加。Boolean default=0 Persist=1 ((N12-3) A)、PC-N-11 (N11-1)/(N11-2) 同形 r41 pattern。Comment は本 cvar 単独説明のみ ((N12-5) B = 将来 cvar 列挙しない)。

**想定 XML diff example**:

```xml
<key>AYAGltfRealModelviewEnabled</key>
<map>
    <key>Comment</key>
    <string>
        AYAstorm r41 Phase 1.E PC-N-12 = real node modelview push constant 配線 cvar
        (= recordGltfAssetDraw PC-N-8 (f) real Asset path 内 identity push constant
        modelview → real Asset::mNodes[node_index].mAssetMatrix 経由 = 第 2 sentinel-like
        placeholder 段階卒業)。
        OFF (default) = 既 PC-N-8 (f) identity 64 B push constant modelview path 維持
        (= Phase 1.D + PC-N-11 baseline 不変)。
        ON = sCurrentNodeIndex 有効 + bounds OK 時 real Node.mAssetMatrix 投入 =
        upstream Asset::uploadTransforms (per-frame、scene graph 根からの絶対 transform
        合成済) と同 source 流用 + glm::value_ptr で column-major 64 B raw float 投入。
        Prerequisite: AYAGltfRealDrawEnabled=1 (= recordGltfAssetDraw fire entry)。
    </string>
    <key>Persist</key>
    <integer>1</integer>
    <key>Type</key>
    <string>Boolean</string>
    <key>Value</key>
    <integer>0</integer>
</map>
```

### §4.2 step (b) — `setCurrentNodeIndex/clearCurrentNodeIndex/getCurrentNodeIndex` 宣言追加 (llvkloader.h)

`indra/llrender/llvkloader.h` の既 `setCurrentPrimitive/clearCurrentPrimitive/getCurrentPrimitive` 直後並列 (= line 516 直後) に `<AYAstorm r41 PC-N-12 (b)>` tag block で 3 accessor 宣言追加 ((N12-2) A)。

**想定 C++ diff example**:

```cpp
    // <AYAstorm r41 PC-N-12 (b)> sCurrentNodeIndex accessor 新規追加 ((N12-2) A)。
    //   PC-N-12 real node modelview 切替 = recordGltfAssetDraw PC-N-8 (f) 内
    //   push constant identity → real Asset::mNodes[node_index].mAssetMatrix 経由。
    //   GLTFSceneManager::render per-Primitive loop が PC-N-12 (c) で
    //   setCurrentNodeIndex(pdata.mNodeIndex) / clearCurrentNodeIndex() 配線。
    //   PC-N-12 単独発火なし = sCurrentNodeIndex == INVALID_INDEX natural guard。
    void setCurrentNodeIndex (S32 node_index);
    void clearCurrentNodeIndex();
    S32  getCurrentNodeIndex();
    // </AYAstorm r41 PC-N-12 (b)>
```

### §4.3 step (c) — `sCurrentNodeIndex` static + 3 accessor 実装 (llvkloader.cpp)

`indra/llrender/llvkloader.cpp` の anonymous namespace 内 `sCurrentPrimitive` 静的変数の直後並列に `S32 sCurrentNodeIndex = LL::GLTF::INVALID_INDEX;` 静的変数追加。3 accessor 実装は既 `setCurrentPrimitive/clearCurrentPrimitive/getCurrentPrimitive` (line 5751-5764) 直後並列に `<AYAstorm r41 PC-N-12 (c)>` tag block で配置 ((N12-2) A、main thread 専有ゆえ atomic 不要)。

**想定 C++ diff example** (static 変数 + accessor 実装):

```cpp
// static 変数追加 (= anonymous namespace 内 sCurrentPrimitive 直後並列)
S32 sCurrentNodeIndex = LL::GLTF::INVALID_INDEX;

// <AYAstorm r41 PC-N-12 (c)> sCurrentNodeIndex accessor 実装 ((N12-2) A、
//   AYA literal「全件推奨で OK」record 2026-06-05)。sCurrentPrimitive 同形 pattern。
//   GLTFSceneManager::render per-Primitive loop が PC-N-12 (e) で set/clear 配線。
void setCurrentNodeIndex(S32 node_index)
{
    sCurrentNodeIndex = node_index;
}

void clearCurrentNodeIndex()
{
    sCurrentNodeIndex = LL::GLTF::INVALID_INDEX;
}

S32 getCurrentNodeIndex()
{
    return sCurrentNodeIndex;
}
// </AYAstorm r41 PC-N-12 (c)>
```

### §4.4 step (d) — `LLCachedControl<bool> sAyastormGltfRealModelviewEnabled` 配置 (llvkloader.cpp)

`recordGltfAssetDraw` 内 PC-N-8 (f) 内 PC-N-11 (a) inner block 直後並列に `LLCachedControl<bool>` 配置 ((N12-3) A、PC-N-11 (b) と同形)。

**想定 C++ diff example**:

```cpp
static LLCachedControl<bool> sAyastormGltfRealModelviewEnabled(
    gSavedSettings, "AYAGltfRealModelviewEnabled", false);
```

### §4.5 step (e) — PC-N-8 (f) push constant identity block の `<AYAstorm r41 PC-N-12 (a)>` tag wrap + real modelview 分岐 (llvkloader.cpp)

`recordGltfAssetDraw` PC-N-8 (f) 内 push constant identity block (= line 6018-6031) を新規 `<AYAstorm r41 PC-N-12 (a)>` tag block で wrap ((N12-6) A、surgical insertion)。cvar guard + sCurrentNodeIndex valid guard + bounds check 3 段で real Node.mAssetMatrix path / identity fall-through path 分岐 ((N12-7)/(N12-8)/(N12-9) A)。

**想定 C++ diff example** (= line 6018-6031 改変):

```cpp
// <AYAstorm r41 PC-N-12 (a)> real node modelview push constant 配線 ((N12-1)..(N12-16) AYA
//   literal「全件推奨で OK」record 2026-06-05 採用)。AYAGltfRealModelviewEnabled cvar=true
//   かつ sCurrentNodeIndex != INVALID_INDEX かつ sCurrentAsset != nullptr かつ
//   sCurrentNodeIndex < (S32)sCurrentAsset->mNodes.size() 3 段 guard ((N12-9) A) 時
//   real Node.mAssetMatrix path = upstream Asset::uploadTransforms (asset.cpp:180) と同
//   source 流用 ((N12-1) A、mAssetMatrix = local→asset space 合成済 = parent chain 解決済)
//   + glm::value_ptr ((N12-10) A、column-major raw float*)。cvar=false or guard fail 時
//   identity 投入 ((N12-8) A = MUSEUBO-A 整合、前 draw push value 残存防止)。
//   単一 vkCmdPushConstants call ((N12-7) A、source pointer のみ if-else 切替で二重投入回避)。
static LLCachedControl<bool> sAyastormGltfRealModelviewEnabled(
    gSavedSettings, "AYAGltfRealModelviewEnabled", false);

const float identity_modelview[16] = {
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f,
};

const float* modelview_src = identity_modelview;
const S32     node_index   = getCurrentNodeIndex();
LL::GLTF::Asset* const current_asset = getCurrentAsset();
const bool real_path_eligible =
    (sAyastormGltfRealModelviewEnabled
     && node_index != LL::GLTF::INVALID_INDEX
     && current_asset != nullptr
     && node_index < (S32)current_asset->mNodes.size());

if (real_path_eligible)
{
    LL::GLTF::Node& node = current_asset->mNodes[node_index];
    modelview_src = glm::value_ptr(node.mAssetMatrix);

    // PC-N-12 (a) first-fire LL_INFOS marker ((N12-11) A)
    static std::atomic<bool> s_first_pcn12_real_modelview_fire{true};
    if (s_first_pcn12_real_modelview_fire.exchange(false, std::memory_order_acq_rel))
    {
        LL_INFOS("Vulkan") << "PC-N-12 (a) real node modelview path 通電 (first fire): "
                              "asset=" << current_asset
                           << ", node_index=" << node_index
                           << ", node_count=" << current_asset->mNodes.size()
                           << ", mAssetMatrix[0..3]={"
                           << modelview_src[0] << ", " << modelview_src[1] << ", "
                           << modelview_src[2] << ", " << modelview_src[3] << "}"
                           << "; upstream Asset::uploadTransforms (asset.cpp:180) "
                              "と同 source 流用 + glm::value_ptr column-major 64 B raw "
                              "float* 投入"
                           << LL_ENDL;
    }
}

vkCmdPushConstants(cmd_buf,
                   sAvatarBoneLayout,
                   VK_SHADER_STAGE_VERTEX_BIT,
                   /*offset=*/0,
                   /*size=*/64,
                   modelview_src);
// </AYAstorm r41 PC-N-12 (a)>
```

### §4.6 step (f) — gltfscenemanager.cpp per-Primitive loop に setCurrentNodeIndex/clearCurrentNodeIndex 配線

`indra/newview/gltfscenemanager.cpp` per-Primitive loop body 内 ((N12-14) A):
- 既 `LLVKLoader::setCurrentPrimitive(&primitive)` (line 752) **直後並列** に `<AYAstorm r41 PC-N-12 (e)>` tag block で `LLVKLoader::setCurrentNodeIndex(pdata.mNodeIndex)` 配線 (= `if (rigged)` 外、unconditional)
- 既 `LLVKLoader::clearCurrentPrimitive()` (line 792) **直前並列** に `LLVKLoader::clearCurrentNodeIndex()` 配線 (= unconditional clear)

**想定 C++ diff example**:

```cpp
// per-Primitive loop body 冒頭 (= line 752 直後)
LLVKLoader::setCurrentPrimitive(&primitive);
// <AYAstorm r41 PC-N-12 (e)> sCurrentNodeIndex per-Primitive 配線 ((N12-14) A、
//   AYA literal「全件推奨で OK」record 2026-06-05)。setCurrentPrimitive 並列、
//   `if (rigged)` 外で unconditional (= real Node modelview は rigged/non-rigged
//   問わず必要)。pdata.mNodeIndex は既 line 741/775 で参照済 = source 同一。
LLVKLoader::setCurrentNodeIndex(pdata.mNodeIndex);
// </AYAstorm r41 PC-N-12 (e)>

// per-Primitive loop body 末尾 (= line 792 直前)
// <AYAstorm r41 PC-N-12 (e)> sCurrentNodeIndex per-Primitive clear。
//   clearCurrentPrimitive 並列、unconditional clear (= setCurrentNodeIndex
//   必ず call 済 = nullptr→INVALID_INDEX no-op fallback ではなく明示 reset)。
LLVKLoader::clearCurrentNodeIndex();
// </AYAstorm r41 PC-N-12 (e)>
LLVKLoader::clearCurrentPrimitive();
```

### §4.7 step (g) — build verify literal 取得

PC-N-11 同形 ((N12-12) A):
1. `make -j4 llrender` → `[100%] Built target llrender` (= ERROR 0 / WARNING 0)
2. `INTEGRATION_TEST_lluboringbuffer` → 11/11 PASS
3. `INTEGRATION_TEST_llassetubopool` → 10/10 PASS
4. `INTEGRATION_TEST_llpipelinecachestorage` → 13/13 PASS
5. `python3 -m unittest discover tests` (codegen) → 131/131 OK
6. `grep -c LL_VULKAN_GLSL indra/llrender/llvkloader.cpp` → 6 (= PC-N-11 commit `797332ee81` 同数、GATE-B integrity 維持)

### §4.8 step (h) — handoff complete doc 起案

`handoff-substep-...-phase1-e-pc-n-12-complete.md` 起案 = step (a)-(h) 全実施 record + Exit Criteria 10 項全充足 + build verify literal 取得 + GATE-B integrity record + self-verify 9 観点 全 ✅ + 引き継ぎ memory + 次 session 着手 1 line (= PC-N-13 design-lock 着手 = real per-draw light params + multi-asset verify)。AYA literal 「commit してください」受領後 commit (`feedback_no_auto_commit` + `feedback_release_branch_workflow` + `feedback_no_claude_coauthor` 遵守)。

### §4.9 GATE-B 整合

- `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count `llvkloader.cpp=6` 不変、PC-N-11 commit `797332ee81` 同数)
- `AYAGltfRealModelviewEnabled` cvar runtime gate のみで Vulkan path / identity path 分岐
- shader 改変 0 件 (= 既 push constant `modelview_matrix` mat4 64 B / VERTEX_BIT layout 不変、shader 側は cvar 状態を知らない)
- memory `project_r41_phase1b_vulkan_host_gate` 整合

### §4.10 MUSEUBO-A 整合

- `AYAGltfRealModelviewEnabled=false` default で identity modelview 維持 = PC-N-11 baseline + Phase 1.D complete baseline 不変
- OpenGL 描画 100% 維持 (= 本 PC-N-12 改変は Vulkan path `recordGltfAssetDraw` 内のみ、OpenGL path 無関係)
- 5 段 graceful degrade 維持 (= cvar=false / sCurrentNodeIndex==INVALID_INDEX / sCurrentAsset==nullptr / bounds out / mNodes.size()==0 各 fall-through to identity)

### §4.11 設計原則整合

- **(1) Upstream OpenGL 取り込みやすさ維持**:
  - `recordGltfAssetDraw` signature 不変維持
  - `Asset::mNodes[].mAssetMatrix` upstream 既配線活用 = `Asset::uploadTransforms` per-frame 更新 + scene graph parent chain 解決済 = upstream 互換性最大化
  - shader 改変ゼロ = `modelview_matrix` push constant layout 不変
  - `GLTFSceneManager::render` 改変は host-side hook のみ ((N12-14) A、setCurrentPrimitive 並列 + clearCurrentPrimitive 並列、per-Primitive loop 構造温存)
- **(2) Core プロセス分散実現**:
  - per-Node modelview source は `sCurrentNodeIndex` static + thread-local accessor (= main thread 専有) で primitive-level granularity 維持
  - PC-N-14/15 worker thread 分散 design 整合 (= per-Primitive node_index resolution は immutable Asset 参照ゆえ thread-safe)

---

## §5. PC-N-12 design-lock Exit Criteria (= 9 項全充足、本 commit)

| # | criterion | status |
|---|-----------|--------|
| i | PC-N-12 literal scope §0 完全分解 4 件 (= AYA task statement literal 4 件直訳、(N12-1) A で mMatrix → mAssetMatrix 正式 resolve = §2.5) | ✅ |
| ii | 必読 1 件 §1.1 (本 PC-N-12 design-lock doc) + pinpoint reference 12 件 §1.2 別記 = full file dump なし (= `feedback_handoff_minimal_pre_req_read` 整合) | ✅ |
| iii | ambiguity (N12-1)..(N12-16) 16 件 + AYA literal「全件推奨で OK」record (2026-06-05) §3 | ✅ |
| iv | 採用根拠 16 件明文化 §3 (特に (N12-1) mAssetMatrix vs mMatrix 正式 resolve + (N12-2) accessor 経由必須の §2.4 finding + (N12-14) caller-side 配線必須 + (N12-7) 単一 push call の trade-off 明示) | ✅ |
| v | 実装計画 (a)-(h) 8 step 分解 §4 + 各 step 具体 code stub example 添付 | ✅ |
| vi | 実装 phase Exit Criteria 10 項明文化 (§6) | ✅ |
| vii | GATE-B 整合 §4.9 + MUSEUBO-A 整合 §4.10 + 設計原則整合 §4.11 | ✅ |
| viii | 想定改変 file 6 件明文化 (= PC-N-11 4 件より +2 (header + caller)、§3.7 (N12-15) A 採用根拠明示) | ✅ |
| ix | `indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合 | ✅ |

---

## §6. PC-N-12 実装 phase Exit Criteria (= 10 項、別 session)

| # | criterion |
|---|-----------|
| i | settings.xml `AYAGltfRealModelviewEnabled` Boolean cvar 1 件追加 (default=0 Persist=1、`AYAGltfMultiSkinEnabled` 直後並列、Comment 単独説明) ((N12-3)/(N12-4)/(N12-5) A/A/B) |
| ii | `llvkloader.h` `<AYAstorm r41 PC-N-12 (b)>` tag block で 3 accessor 宣言追加 ((N12-2) A) |
| iii | `llvkloader.cpp` anonymous namespace 内 `sCurrentNodeIndex` static + `<AYAstorm r41 PC-N-12 (c)>` tag block で 3 accessor 実装 ((N12-2) A) |
| iv | `recordGltfAssetDraw` PC-N-8 (f) 内 push constant identity block (line 6018-6031) を `<AYAstorm r41 PC-N-12 (a)>` tag block で wrap + `LLCachedControl<bool> sAyastormGltfRealModelviewEnabled` 配置 + cvar guard + sCurrentNodeIndex valid guard + bounds check + real Node.mAssetMatrix / identity fall-through 分岐 + 単一 push call ((N12-6)/(N12-7)/(N12-8)/(N12-9)/(N12-10) A) |
| v | `gltfscenemanager.cpp` per-Primitive loop に `<AYAstorm r41 PC-N-12 (e)>` tag block で `setCurrentNodeIndex(pdata.mNodeIndex)` (setCurrentPrimitive 直後並列、`if (rigged)` 外 unconditional) + `clearCurrentNodeIndex()` (clearCurrentPrimitive 直前並列、unconditional clear) 配線 ((N12-14) A) |
| vi | PC-N-12 (a) first-fire `LL_INFOS` marker (`s_first_pcn12_real_modelview_fire` atomic flag、PC-N-6/7/8/9/10/11 同形 pattern) ((N12-11) A) |
| vii | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp=6 不変、PC-N-11 commit `797332ee81` 同数) |
| viii | MUSEUBO-A 整合 = `AYAGltfRealModelviewEnabled=false` default で identity modelview 維持 + OpenGL 描画 100% 維持 + 5 段 graceful degrade 内部維持 |
| ix | build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 ((N12-12) A) + cross-platform spec §6 PC-N-12 行 ✅ 反映 + §A 履歴 1 行追記 + handoff complete doc 起案 |
| x | self-verify 9 観点 全 ✅ + AYA literal commit 指示受領後 commit (= `feedback_no_auto_commit` + `feedback_release_branch_workflow` + `feedback_no_claude_coauthor` 遵守) |

---

## §7. 次 session 着手手順 (= PC-N-12 実装 phase)

1. 必読 1 件 (本 PC-N-12 design-lock doc) を Read
2. §1.2 pinpoint reference 12 件のうち実装必要分のみ Read (= full file dump なし、`feedback_handoff_minimal_pre_req_read` 整合)
3. step (a) settings.xml `AYAGltfRealModelviewEnabled` cvar 追加
4. step (b) `llvkloader.h` 3 accessor 宣言追加
5. step (c) `llvkloader.cpp` `sCurrentNodeIndex` static + 3 accessor 実装
6. step (d) `recordGltfAssetDraw` 内 `LLCachedControl<bool>` 配置
7. step (e) PC-N-8 (f) push constant identity block を `<AYAstorm r41 PC-N-12 (a)>` tag wrap + real modelview 分岐
8. step (f) `gltfscenemanager.cpp` per-Primitive loop に setCurrentNodeIndex/clearCurrentNodeIndex 配線
9. step (g) build verify literal 取得 (= llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 + GATE-B integrity LL_VULKAN_GLSL count llvkloader.cpp=6 不変)
10. step (h) handoff complete doc 起案 + cross-platform spec §6 PC-N-12 行 ✅ 反映 + §A 履歴追記 + Exit Criteria 10 項 self-verify + AYA literal commit 指示受領後 commit
11. 以後 PC-N-13 design-lock 着手 (= real per-draw light params + multi-asset verify)

---

## §8. 残 strict 線形

- ✅ Phase 1.A / 1.B / (Z) SSS / (W) uniform4iv / (Y) Phase 1.C prep
- ✅ PC-0..PC-7ε / PC-N decomposition / PC-N-1..PC-N-4 (= Phase 1.C complete)
- ✅ PC-8 Linux primary marker (= Phase 1.C strict 線形終了)
- ✅ PC-N-5 (= Phase 1.D 着手起点) / Phase 1.D decomposition design-lock
- ✅ PC-N-6 / PC-N-7 / PC-N-8 / PC-N-9 / PC-N-10 (= Phase 1.D complete = 1 GLTF asset 完全 Vulkan draw 通電 達成)
- ✅ Phase 1.E decomposition design-lock (commit `094546889b`)
- ✅ PC-N-11 design-lock (commit `87560a4dc7`) + PC-N-11 実装 (commit `797332ee81` = Phase 1.E 内 1st sub-step 実装完了 = multi-skin real Skin path 通電)
- ✅ **PC-N-12 design-lock ✅ 本 commit = Phase 1.E 内 2nd sub-step design-lock 完了**
- ⏳ PC-N-12 実装 (= 別 session、step (a)-(h) 8 step 実施 + Exit Criteria 10 項 self-verify + handoff complete doc 起案)
- ⏳ PC-N-13 design-lock + 実装 (= real per-draw light params + multi-asset verify、`AYAGltfRealLightParamsEnabled` + `AYAGltfMultiAssetCanary` cvar 新設想定)
- ⏳ PC-N-14 design-lock (= worker thread design = per-Primitive UBO write + cmdbuf record 並列化 design)
- ⏳ PC-N-15 実装 + cleanup (= worker thread 実装 + `sGltfStubSkin` sentinel storage 撤去 + Phase 1.E complete marker)
- ⏳ Phase 1.E complete = 実 data 通電 + multi-asset / multi-skin / worker thread 分散達成 → Phase 1 全完了 → Mac/Win 開発者補完 phase

---

## §9. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α..PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1..PC-N-4 ✅ = Phase 1.C complete ✅ + PC-8 Linux primary marker ✅ + PC-N-5 ✅ + Phase 1.D decomposition design-lock ✅ + PC-N-6 ✅ + PC-N-7 ✅ + PC-N-8 ✅ + PC-N-9 ✅ + PC-N-10 ✅ = Phase 1.D complete ✅ (= 1 GLTF asset 完全 Vulkan draw 通電 達成) + Phase 1.E decomposition design-lock ✅ + PC-N-11 design-lock ✅ + PC-N-11 ✅ (= Phase 1.E 内 1st sub-step 実装完了 = multi-skin real Skin path 通電) + **PC-N-12 design-lock ✅ 本 commit** + PC-N-12 実装 ⏳ 次 session + PC-N-13 design-lock + 実装 ⏳ + PC-N-14 design-lock ⏳ + PC-N-15 設計 + 実装 + cleanup ⏳ + Phase 1.E complete ⏳ + Phase 1 全完了 ⏳ + Mac/Win 開発者補完 phase ⏳

---

## §10. self-verify 9 観点 全 ✅

1. ✅ PC-N-12 literal scope §0 完全分解 4 件 (= AYA task statement literal が source of truth、(N12-1) A で mMatrix → mAssetMatrix 正式 resolve = upstream `Asset::uploadTransforms` 整合)
2. ✅ 必読 1 件 §1.1 + pinpoint reference 12 件 §1.2 別記 = full file dump なし (= `feedback_handoff_minimal_pre_req_read` 整合)
3. ✅ 現状調査 §2 完全分解 (= push constant identity block + 関連 accessor + caller-side gltfscenemanager.cpp per-Primitive loop 構造 + §2.4 finding (`Primitive` が mNodeIndex を持たない構造的事実) + (E-13) commit msg vs AYA task statement vs upstream 整合 resolve)
4. ✅ ambiguity (N12-1)..(N12-16) 16 件 AYA literal「全件推奨で OK」record (2026-06-05) §3 + 採用根拠 16 件明文化 (特に (N12-1) mAssetMatrix vs mMatrix 正式 resolve + (N12-2) accessor 必須の §2.4 finding + (N12-14) caller-side 配線必須 + (N12-7) 単一 push call の trade-off 明示)
5. ✅ 実装計画 (a)-(h) 8 step §4 + 各 step 具体 code stub example 添付
6. ✅ GATE-B 整合 §4.9 (= `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、`AYAGltfRealModelviewEnabled` cvar runtime gate のみ、shader 改変ゼロ)
7. ✅ MUSEUBO-A 整合 §4.10 (= `AYAGltfRealModelviewEnabled=false` default で identity modelview 維持 + OpenGL 描画 100% 維持 + 5 段 graceful degrade 内部維持)
8. ✅ 設計原則整合 §4.11 (= (1) Upstream OpenGL 取り込みやすさ維持 = `recordGltfAssetDraw` signature 不変 + upstream `Asset::mNodes[].mAssetMatrix` 既配線活用 + `GLTFSceneManager::render` 改変は host-side hook のみ + shader 改変ゼロ + (2) Core プロセス分散実現 = `sCurrentNodeIndex` per-Primitive granularity 維持、PC-N-14/15 worker thread 分散 design 整合)
9. ✅ `indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合

---

## §11. 次 session 着手 1 line

PC-N-12 実装着手 = step (a)-(h) 8 step 実施 = (a) settings.xml `AYAGltfRealModelviewEnabled` cvar 1 件追加 (`AYAGltfMultiSkinEnabled` 直後並列、Boolean default=0 Persist=1、本 cvar 単独説明) + (b) `llvkloader.h` 3 accessor 宣言追加 (`<AYAstorm r41 PC-N-12 (b)>` tag block) + (c) `llvkloader.cpp` `sCurrentNodeIndex` static + 3 accessor 実装 (`<AYAstorm r41 PC-N-12 (c)>` tag block、`sCurrentPrimitive` 並列 pattern) + (d) `recordGltfAssetDraw` 内 `LLCachedControl<bool> sAyastormGltfRealModelviewEnabled` 配置 + (e) PC-N-8 (f) 内 push constant identity block (line 6018-6031) を `<AYAstorm r41 PC-N-12 (a)>` tag block で wrap + cvar + sCurrentNodeIndex + sCurrentAsset + bounds 3 段 guard + real Node.mAssetMatrix / identity fall-through 分岐 + 単一 vkCmdPushConstants call + first-fire `LL_INFOS` marker (`s_first_pcn12_real_modelview_fire` atomic flag) + (f) `gltfscenemanager.cpp` per-Primitive loop に `setCurrentNodeIndex(pdata.mNodeIndex)` (setCurrentPrimitive 並列、`if (rigged)` 外 unconditional) + `clearCurrentNodeIndex()` (clearCurrentPrimitive 並列、unconditional clear) 配線 + (g) build verify literal 取得 (= llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 + GATE-B integrity LL_VULKAN_GLSL count llvkloader.cpp=6 不変) + (h) handoff complete doc 起案 + cross-platform spec §6 PC-N-12 行 ✅ 反映 + §A 履歴 1 行追記 + Exit Criteria 10 項 self-verify + AYA literal commit 指示受領後 commit

---

## §A. feedback 遵守 record

- ✅ `feedback_proactive_handoff` (本 PC-N-12 design-lock doc 起案)
- ✅ `feedback_handoff_minimal_pre_req_read` (必読 1 件 + pinpoint reference 12 件別記、本 session も Read pinpoint のみ = PC-N-11 complete doc 全文 + `recordGltfAssetDraw` PC-N-8 (f) push constant block + `LL::GLTF::Node` 構造 + `Asset::uploadTransforms` 実装 + `RenderBatch::PrimitiveData` 定義 + `gltfscenemanager.cpp` per-Primitive loop 構造 + llvkloader.h accessor 宣言群 + settings.xml `AYAGltfMultiSkinEnabled` 配置、full file dump なし)
- ✅ `feedback_self_verify_before_handoff` (9 観点 self-verify 全 ✅)
- ✅ `feedback_build_only_verified` (design-lock phase は `indra/` 改変 0 件で build verify 対象外、実装 phase で literal 検証取得予定 (N12-12) A 採用)
- ✅ `feedback_no_scope_shrink` (PC-N-12 literal scope §0 完全分解 4 件 = AYA task statement literal が source of truth、(N12-1) A で mMatrix → mAssetMatrix 正式 resolve は upstream `Asset::uploadTransforms` line 180 整合の implementation detail resolve = scope 縮小ではなく正確化、(N12-15) A 6 file 改変は PC-N-11 4 file より +2 (header + caller) = 構造的必然 (`Primitive` が mNodeIndex を持たない = §2.4 finding)、`feedback_ubo_migration_one_at_a_time` 厳格遵守整合)
- ✅ `feedback_doubt_self_first` (design-lock phase で ambiguity 16 件発見 + 推奨案提示 + AYA literal「全件推奨で OK」record 後本 design-lock doc 起案、特に (N12-1) AYA task statement literal「mMatrix」と upstream `Asset::uploadTransforms` `mAssetMatrix` 不整合を Grep/Read で literal 確認後 mAssetMatrix を正式採用 + (N12-2) accessor 必須は `gltf/primitive.h` を Grep で `mNodeIndex` 検索後 unmatched 確認 + `asset.h:332-336` `RenderBatch::PrimitiveData::mNodeIndex` を Read で literal 確認後発見 = §2.4 finding、推測実装なし)
- ✅ `feedback_confirm_referent_before_acting` (16 件 batch AYA 確認 (2026-06-05)、各候補 + 推奨案 + 根拠明示後 AYA literal 一括「全件推奨で OK」record 受領で確定、推測実装なし、特に (N12-1) mAssetMatrix vs mMatrix 正式 resolve は 3 source (AYA task statement + commit msg + upstream 実装) の literal 全件提示後採用)
- ✅ `feedback_ubo_migration_one_at_a_time` 厳格遵守 (PC-N-12 = real node modelview 単独 sub-step = identity push constant 卒業 + real `Asset::mNodes[].mAssetMatrix` 投入 + `AYAGltfRealModelviewEnabled` cvar 新設 + `setCurrentNodeIndex` accessor 新設 + caller-side gltfscenemanager.cpp 配線、PC-N-13 (real per-draw light params + multi-asset verify) + PC-N-14 (worker thread design) + PC-N-15 (worker thread 実装 + cleanup) は分離 = 各 sub-step 別 session で別途 design-lock + 実装、本 doc 起案も PC-N-12 単独 design-lock のみ)
- ✅ `feedback_design_phase_no_code_write` 厳格遵守 (本 PC-N-12 design-lock phase は doc 起案 + cross-platform spec §6 行更新のみ、`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件)
- ✅ `feedback_release_branch_workflow` (feature branch `feature/ayastorm-r41-gl-removal` 上 commit 想定)
- ✅ `feedback_no_auto_commit` (AYA 明示 commit 指示受領後 commit 予定)
- ✅ `feedback_no_claude_coauthor` (Co-Authored-By 行不在予定)
- ✅ `feedback_no_bare_reference_ids` ((N12-1)..(N12-16) 各 ID に項目名 / 採用案内容併記 + (a)..(h) 各 step に作業内容併記)
- ✅ `feedback_tests_dir_never_commit` 整合 (tests/ 改変 0 件、git add 個別 file 指定予定)
- ✅ memory `project_ayastorm_r41_design_principles` 整合 ((1) Upstream OpenGL 取り込みやすさ維持 = `recordGltfAssetDraw` signature 不変 + upstream `Asset::mNodes[].mAssetMatrix` 既配線活用 (= `Asset::uploadTransforms` per-frame 更新 + scene graph parent chain 解決済) + `GLTFSceneManager::render` 改変は host-side hook のみ (setCurrentPrimitive 並列) + shader 改変ゼロ + (2) Core プロセス分散実現 = `sCurrentNodeIndex` per-Primitive granularity 維持、PC-N-14/15 worker thread 分散 design 整合)
- ✅ memory `project_r41_phase1b_vulkan_host_gate` 整合 (GATE-B = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、`AYAGltfRealModelviewEnabled` cvar runtime gate のみ、count=6 不変想定)
- ✅ memory `project_ayastorm_three_platforms` 整合 (cross-platform spec §6 PC-N-12 行更新で macOS / Windows 派生 fix 候補欄起案 = host-side push constant data source 切替は `vkCmdPushConstants` 呼出ゆえ MoltenVK 標準対応範囲 + upstream `Asset::uploadTransforms` per-frame 更新は OS 非依存 + descriptor set 数 5 維持 + `sGltfStubSkin` sentinel + `sGltfStubAssetPipeline` 維持で MoltenVK 影響増なし + `AYAGltfRealModelviewEnabled` cvar XML は OS 非依存 ゆえ macOS 派生 fix 候補なし想定 + Windows full Vulkan ゆえ派生 fix 候補なし想定、Linux primary 完成 → 他者補完 model 整合)
