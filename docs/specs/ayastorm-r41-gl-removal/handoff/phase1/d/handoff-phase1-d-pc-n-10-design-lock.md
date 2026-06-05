# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.D PC-N-10 design-lock

**Status**: ✅ **PC-N-10 design-lock complete = Phase 1.D 内 5th = 最終 sub-step design-lock**

**Date**: 2026-06-05
**Branch**: `feature/ayastorm-r41-gl-removal`
**Previous commit**: `50b1171cff` (PC-N-9 complete = Phase 1.D 内 4th sub-step 実装完了)

---

## §0. PC-N-10 literal scope (= AYA 起案文 + Phase 1.D decomposition §4.5 + PC-N-9 complete §1.3 record 整合)

PC-N-10 = **Phase 1.D 内 5th = 最終 sub-step = cleanup + 3 stub cvar deprecate + Phase 1.D complete marker 起案**。

**literal scope 7 件** ((N10-1) A 採用 = AYA 起案文 + PC-N-9 complete §1.3 + settings.xml AYAGltfRealDrawEnabled Comment 「PC-N-10 で 3 stub cvar deprecate 予定」record 整合):

1. **3 stub cvar deprecate** = `AYAGltfStubDrawEnabled` (PC-N-5) + `AYAGltfStubVertexBufferEnabled` (PC-N-6) + `AYAGltfStubIndexBufferEnabled` (PC-N-7) を settings.xml から完全削除 + llvkloader.cpp 内 LLCachedControl 宣言削除
2. **`AYAGltfRealDrawEnabled` 一本化** = recordGltfAssetDraw fire entry hook を AYAGltfRealDrawEnabled cvar gate に切替 (= recordAvatarPlaceholderDraw 末尾 hook の cvar 名のみ変更)
3. **PC-N-5 base shader generate 経路撤去** = recordGltfAssetDraw 内 line 6438-6519 付近 sAvatarBonePipeline 経路 + writeDrawUbo + writeSkinUbo + vkCmdDraw(3,1,0,0) 全削除
4. **PC-N-6 (e) stub VB 経路撤去** + **PC-N-7 (e) stub IB 経路撤去** = recordGltfAssetDraw 内 cvar 分岐ブロック全削除、PC-N-8 (f) real Asset path のみ残す
5. **PC-N-9 (b) cvar guard wrap 撤去** = entry hook 自体が AYAGltfRealDrawEnabled cvar gate に切替わるゆえ二重 gate 冗長
6. **stub VB/IB storage 撤去** = `sGltfStubVertexBuffer` / `sGltfStubIndexBuffer` storage + initVulkan 内 VMA allocate + shutdownVulkan 内 vmaDestroyBuffer 全削除
7. **`sGltfStubSkin` sentinel + `sGltfStubAssetPipeline` 維持** = PC-N-8 (f) real Asset path で再利用 ((N10-3) B + (N10-5) B 採用)、real Skin owner 切替は Phase 1.E (multi-skin) scope 持越し

**Phase 1.D complete marker**: PC-N-10 完了 = **Phase 1.D complete** = 1 GLTF asset 完全 Vulkan draw 通電 ((D-2) A + (D-11) A 整合)。Phase 1.E = multi-asset / multi-skin / worker thread (= memory `project_ayastorm_r41_design_principles` (2) Core プロセス分散実現) は本 Phase 1.D scope 外。

---

## §1. 必読 1 件 + pinpoint reference 11 件

### §1.1 必読 1 件 (= 全文 Read 推奨)

- `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-d-pc-n-9-complete.md` (= PC-N-9 complete handoff doc = HEAD `50b1171cff` 直前 baseline)

### §1.2 pinpoint reference 11 件 (= full file dump せず literal 確認のみ)

| # | reference | 目的 |
|---|-----------|------|
| 1 | `indra/llrender/llvkloader.cpp:582-601` (PC-N-5 (a) sGltfStubSkin sentinel storage) | (N10-3) B 維持確認 |
| 2 | `indra/llrender/llvkloader.cpp:608-650` 付近 (PC-N-6 (a) sGltfStubVertexBuffer storage + pipeline + sGltfStubAssetPipeline 関連) | (N10-4) A 撤去 + (N10-5) B 維持 site 確認 |
| 3 | `indra/llrender/llvkloader.cpp:3999-4052` (PC-N-5 (c) initVulkan 内 registerSkinUbo(sGltfStubSkin)) | (N10-3) B 維持確認、unregister timing 同形維持 |
| 4 | `indra/llrender/llvkloader.cpp:4485-4491` (PC-N-5 (d) shutdownVulkan 内 unregisterSkinUbo(sGltfStubSkin)) | (N10-3) B 維持確認 |
| 5 | `indra/llrender/llvkloader.cpp:6063-6519` (PC-N-5 (b) recordGltfAssetDraw 本体 + PC-N-6/PC-N-7/PC-N-8/PC-N-9 各 cvar 分岐 + PC-N-5 base shader generate 経路) | (N10-7) A + (N10-8) A + (N10-9) A 撤去 site 確認 |
| 6 | `indra/llrender/llvkloader.cpp:6642-6662` (PC-N-5 (e) recordAvatarPlaceholderDraw 末尾 AYAGltfStubDrawEnabled cvar gate hook) | (N10-6) A cvar 切替 site 確認 (= cvar 名のみ AYAGltfRealDrawEnabled 切替、tag block 名称 PC-N-10 (a) に rename = (N10-16) A) |
| 7 | `indra/newview/app_settings/settings.xml:10411-10477` 付近 (AYAGltfStubDrawEnabled + AYAGltfStubVertexBufferEnabled + AYAGltfStubIndexBufferEnabled + AYAGltfRealDrawEnabled cvar 4 件配置) | (N10-2) A 削除 + (N10-10) A Comment 更新 site 確認 |
| 8 | `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md:111` (§6 PC-N-10 行 stub = ⏳) | (N10-12) A 更新 site 確認 |
| 9 | `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md:129-142` (§A 履歴) | §A 履歴追記 pattern 確認 |
| 10 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-d-decomposition-design-lock.md:144-150` (§4.5 PC-N-10 overview) | scope 整合確認 |
| 11 | memory `project_ayastorm_r41_design_principles` + `project_r41_phase1b_vulkan_host_gate` + `project_ayastorm_three_platforms` | GATE-B + 設計原則 + 3 platform 整合確認 |

---

## §2. 現状調査 (= 8 項網羅)

### §2.1 PC-N-5 (a) sGltfStubSkin sentinel storage (llvkloader.cpp:582-601)

```cpp
// <AYAstorm r41 PC-N-5 (a)> GLTF stub skin sentinel = 第 2 の address-only sentinel
alignas(void*) char sGltfStubSkinStorage[1] = {};
LL::GLTF::Skin* const sGltfStubSkin =
    reinterpret_cast<LL::GLTF::Skin*>(&sGltfStubSkinStorage[0]);
// </AYAstorm r41 PC-N-5 (a)>
```

→ **(N10-3) B 採用 = 維持**。PC-N-8 (f) real Asset path で flushSkinUbos(sGltfStubSkin) sentinel 共用、real Skin owner 切替は Phase 1.E scope 持越し。tag block 名称も維持 (= legacy 名残るが機能は real 共用)。

### §2.2 PC-N-6/PC-N-7 stub VB/IB storage (llvkloader.cpp:608-650 付近)

```cpp
// <AYAstorm r41 PC-N-6 (a)> ... sGltfStubVertexBuffer + sGltfStubVertexAllocation + sGltfStubVertexMapped + sGltfStubVertexData[3] + sGltfStubAssetPipeline
// <AYAstorm r41 PC-N-7 (a)> ... sGltfStubIndexBuffer + sGltfStubIndexAllocation + sGltfStubIndexMapped + sGltfStubIndexData[3] + sGltfStubIndexCount
```

→ **(N10-4) A 採用 = stub VB/IB storage 完全撤去** (PC-N-6 (a)/(c)/(d) + PC-N-7 (a)/(c)/(d) tag block 全削除)。
→ **(N10-5) B 採用 = sGltfStubAssetPipeline 維持** (PC-N-8 (f) line 6175-6180 付近で再利用、名称 "Stub" は legacy 命名残るが機能は real 共用)。

### §2.3 PC-N-5 (e) recordAvatarPlaceholderDraw 末尾 hook (llvkloader.cpp:6642-6662)

```cpp
// <AYAstorm r41 PC-N-5 (e)> AYAGltfStubDrawEnabled cvar=true 時、
//   recordAvatarPlaceholderDraw 末尾から並走 fire ((N5-4) A)
static LLCachedControl<bool> sAyastormGltfStubDrawEnabled(
    gSavedSettings, "AYAGltfStubDrawEnabled", false);
if (sAyastormGltfStubDrawEnabled)
{
    recordGltfAssetDraw(cmd_buf);
}
// </AYAstorm r41 PC-N-5 (e)>
```

→ **(N10-6) A 採用 = cvar 名のみ切替** = `sAyastormGltfStubDrawEnabled` → `sAyastormGltfRealDrawEnabled` + cvar key `"AYAGltfStubDrawEnabled"` → `"AYAGltfRealDrawEnabled"`。**(N10-16) A 採用 = tag block 名称 PC-N-10 (a) に rename**。

### §2.4 recordGltfAssetDraw 内 cvar 優先順位 (llvkloader.cpp:6063-6519)

PC-N-9 (b) wrapping 後の構造:
```
recordGltfAssetDraw {
    if (PC-N-9 (b) AYAGltfRealDrawEnabled) {
        PC-N-8 (f) real Asset path (line 6092-6207)
        if (success) early return;
    }
    if (PC-N-7 AYAGltfStubIndexBufferEnabled) {
        PC-N-7 (e) stub IB path
        early return;
    }
    if (PC-N-6 AYAGltfStubVertexBufferEnabled) {
        PC-N-6 (e) stub VB path
        early return;
    }
    PC-N-5 base shader generate 3 vertex path (line 6438-6519)
}
```

→ **(N10-7) A 採用 = PC-N-6 (e) + PC-N-7 (e) cvar 分岐ブロック全撤去**。
→ **(N10-8) A 採用 = PC-N-9 (b) cvar guard wrap 撤去** (= entry hook 側が AYAGltfRealDrawEnabled cvar gate に切替わるゆえ冗長)。
→ **(N10-9) A 採用 = PC-N-5 base shader generate 経路撤去**。

撤去後の構造:
```
recordGltfAssetDraw {
    PC-N-8 (f) real Asset path のみ (5 段 graceful degrade で sCurrentAsset/sCurrentPrimitive nullptr 時 silent return)
}
```

### §2.5 settings.xml 4 cvar 配置 (settings.xml:10411-10477 付近)

- `AYAGltfStubDrawEnabled` (PC-N-5、line 10411 付近)
- `AYAGltfStubVertexBufferEnabled` (PC-N-6、line 10432 付近)
- `AYAGltfStubIndexBufferEnabled` (PC-N-7、line 10455 付近)
- `AYAGltfRealDrawEnabled` (PC-N-9、line 10477 付近)

→ **(N10-2) A 採用 = 3 stub cvar 行完全削除** (= 各 cvar XML block <key>/<map>/<Comment>/<Persist>/<Type>/<Value> 全削除)。
→ **(N10-10) A 採用 = AYAGltfRealDrawEnabled Comment 更新** = 「PC-N-10 で 3 stub cvar deprecate 予定」記述を「Phase 1.D complete marker = 3 stub cvar 統合済 = `AYAGltfRealDrawEnabled` 単独で実 GLTF Vulkan draw 経路切替」に更新、cvar 優先順位記述も「PC-N-9 単独」に簡素化。

### §2.6 PC-N-8 (f) real Asset path 内 sGltfStubAssetPipeline 再利用 (llvkloader.cpp:6175-6180 付近)

PC-N-8 design-lock (N8-8) A 採用結果。**(N10-5) B 採用根拠**。

### §2.7 Phase 1.D decomposition §4.5 vs AYA 起案文 (今 session) 緊張点

§4.5 literal = 「AYAGltfStubDrawEnabled deprecate」のみ明示、(D-12) literal も同形。
AYA 起案文 (今 session) + PC-N-9 complete §1.3 + settings.xml AYAGltfRealDrawEnabled Comment = 「3 stub cvar deprecate」明示。

→ **(N10-1) A 採用** = AYA 起案文 (今 session) を source of truth として 3 stub cvar 全 deprecate に拡張 (= §4.5 は overview 時点の最小 scope、本 PC-N-10 design-lock で AYA 確認後拡張)。

### §2.8 GLTFSceneManager::render fire entry 確認

PC-N-9 complete doc + gltfscenemanager.cpp 確認結果:
- PC-N-9 setCurrentPrimitive/clearCurrentPrimitive hook は owner 解決のみ
- recordGltfAssetDraw fire 新経路は新設なし
- 現状の唯一の fire entry = recordAvatarPlaceholderDraw 末尾 hook (PC-N-5 (e))

→ **(N10-6) A 採用根拠** = entry hook 完全撤去 (B 案) は recordGltfAssetDraw dead code 化、cvar 名のみ切替 (A 案) で fire 経路維持。

---

## §3. ambiguity (N10-1)..(N10-16) 16 件 + AYA literal「全件推奨で OK」record (2026-06-05) + 採用根拠

| # | 項目 | 採用案 | AYA record | 採用根拠 |
|---|------|--------|------------|----------|
| (N10-1) | PC-N-10 literal scope 範囲 | **A**: 3 stub cvar 全 deprecate (= AYAGltfStubDrawEnabled + AYAGltfStubVertexBufferEnabled + AYAGltfStubIndexBufferEnabled) + 関連 stub VB/IB storage + PC-N-5 base shader generate 経路撤去 + AYAGltfRealDrawEnabled 一本化 | OK (2026-06-05) | AYA 起案文 (今 session) + PC-N-9 complete §1.3 + settings.xml AYAGltfRealDrawEnabled Comment 「3 stub cvar deprecate 予定」record 整合、§4.5 literal は overview 時点の最小 scope、本 PC-N-10 design-lock で 3 cvar 統合に拡張 |
| (N10-2) | 3 stub cvar 撤去方法 | **A**: settings.xml 行完全削除 + llvkloader.cpp 内 LLCachedControl 宣言削除 | OK (2026-06-05) | PC-N-9 で AYAGltfRealDrawEnabled 代替配線済、deprecation period 不要、code-clean state 達成、Persist=1 legacy 値も新 cvar に影響しない |
| (N10-3) | `sGltfStubSkin` sentinel storage 撤去 vs 維持 | **B**: 維持 (PC-N-5 (a)/(c)/(d) tag block 維持) | OK (2026-06-05) | PC-N-8 (f) line 6170 + 関連 wrapped block 内で sGltfStubSkin sentinel 経由 flushSkinUbos、real Skin owner 切替は Phase 1.E (multi-skin) scope 持越し、本 PC-N-10 で撤去すると PC-N-8 (f) 経路破壊 |
| (N10-4) | `sGltfStubVertexBuffer` / `sGltfStubIndexBuffer` storage 撤去 | **A**: 完全撤去 (PC-N-6/7 (a)/(c)/(d) tag block 全削除、initVulkan VMA allocate + shutdownVulkan vmaDestroyBuffer 全削除) | OK (2026-06-05) | (N10-1) A + (N10-7) A 整合、stub buffer 使用経路なしになる、code-clean、live A/B は PC-N-9 で確立済ゆえ stub VB/IB 退避不要 |
| (N10-5) | `sGltfStubAssetPipeline` 撤去 vs 維持 | **B**: 維持 | OK (2026-06-05) | PC-N-8 (f) line 6175-6180 付近で再利用 ((N8-8) A 採用)、real Asset draw 継続使用、撤去すると PC-N-8 (f) 経路破壊。名称 "Stub" は legacy 命名残るが機能は real 共用 |
| (N10-6) | recordGltfAssetDraw fire entry hook 処理 | **A**: PC-N-5 (e) hook の cvar 名のみ切替 (`AYAGltfStubDrawEnabled` → `AYAGltfRealDrawEnabled`) | OK (2026-06-05) | PC-N-9 setCurrentPrimitive hook は owner 解決のみ、recordGltfAssetDraw fire 新経路は新設なし、B 採用すると recordGltfAssetDraw dead code 化、A = 同 caller site で cvar 名のみ切替で fire 経路維持 + AYAGltfRealDrawEnabled 一本化 |
| (N10-7) | PC-N-6 (e) + PC-N-7 (e) stub 経路撤去 vs 維持 | **A**: 完全撤去 (cvar 分岐ブロック削除、recordGltfAssetDraw 内 PC-N-8 (f) のみ残す) | OK (2026-06-05) | (N10-1) A + (N10-2) A + (N10-4) A 整合、cvar 削除で stub 経路 dead code 化、削除が code-clean |
| (N10-8) | PC-N-9 (b) cvar guard wrap 撤去 vs 維持 | **A**: 撤去 (= entry hook 自体が AYAGltfRealDrawEnabled cvar gate に切替わるゆえ二重 gate 冗長) | OK (2026-06-05) | (N10-6) A 採用整合、entry side のみで gate、冗長 gate 撤去 = code-clean、PC-N-8 (f) 内部 5 段 graceful degrade は維持 |
| (N10-9) | PC-N-5 base shader generate 3 vertex 経路 (recordGltfAssetDraw 内 line 6438-6519 付近) 撤去 vs 維持 | **A**: 撤去 | OK (2026-06-05) | (N10-6) A 後 recordGltfAssetDraw は AYAGltfRealDrawEnabled=true 時のみ fire = PC-N-8 (f) のみ fire、PC-N-5 base path は dead code 化、撤去が code-clean |
| (N10-10) | settings.xml AYAGltfRealDrawEnabled Comment 更新 | **A**: 「PC-N-10 で 3 stub cvar deprecate 予定」記述を「3 stub cvar 統合済」「Phase 1.D complete marker」に更新 | OK (2026-06-05) | record の整合性、Comment 内 cvar 優先順位記述も「PC-N-9 単独」に簡素化 |
| (N10-11) | Phase 1.D complete marker doc 起案位置 | **A**: PC-N-10 complete handoff doc 内に Phase 1.D complete marker 統合明示 + Phase 1.D summary 別記 | OK (2026-06-05) | Phase 1.C complete = PC-N-3 complete doc 内統合同形 pattern 踏襲、doc 数最小化、§4.5 「Phase 1.D complete marker 起案」literal 整合 |
| (N10-12) | cross-platform spec §6 PC-N-10 行起案内容 | **A**: ⏳ → ✅ + Phase 1.D complete marker + 「stub 経路撤去 + cvar 統合は host-side cleanup ゆえ macOS 派生 fix なし + Windows full Vulkan ゆえ派生 fix なし」明示 | OK (2026-06-05) | PC-N-6/7/8/9 同形 pattern、Linux primary 完成 marker 整合 |
| (N10-13) | build verify scope (PC-N-10 実装 phase) | **A**: llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 + GATE-B integrity LL_VULKAN_GLSL count llvkloader.cpp=6 不変 (PC-N-6/7/8/9 同形) | OK (2026-06-05) | 一貫性、PC-N-9 commit `50b1171cff` と同数維持で regression なし確認 |
| (N10-14) | Exit Criteria 項目数 | **A**: 10 項 (PC-N-6/7/8/9 同形) | OK (2026-06-05) | 一貫性 |
| (N10-15) | 実装計画 step 分解粒度 | **A**: 10 step = (a) entry hook cvar 切替 + (b) PC-N-6 (e) + PC-N-7 (e) + PC-N-5 base 経路撤去 + (c) PC-N-9 (b) cvar guard 撤去 + (d) stub VB/IB storage + initVulkan + shutdownVulkan 撤去 + (e) settings.xml 3 stub cvar 削除 + (f) AYAGltfRealDrawEnabled Comment 更新 + (g) cross-platform spec §6 PC-N-10 行更新 + (h) Phase 1.D complete marker 起案 + (i) build verify literal 取得 + (j) handoff complete doc 起案 | OK (2026-06-05) | PC-N-6/7/8/9 同形粒度、各 site 独立 testable + build verify 独立 |
| (N10-16) | `recordAvatarPlaceholderDraw` 末尾 PC-N-5 (e) tag block 名称 | **A**: PC-N-10 (a) に rename (cvar 切替で意味変わる) | OK (2026-06-05) | 機能 = AYAGltfRealDrawEnabled cvar gate に変化 = tag 名称も整合的に PC-N-10 (a) に更新、code archaeology 容易 |

---

## §4. 実装計画 step (a)-(j) 10 step 分解

> **注**: 本 §4 は **実装 phase = 別 session = `feedback_ubo_migration_one_at_a_time` 厳格遵守** で着手する。本 design-lock phase は `indra/` 改変 0 件。

### §4.1 step (a) `recordAvatarPlaceholderDraw` 末尾 entry hook cvar 切替

`indra/llrender/llvkloader.cpp:6642-6662` PC-N-5 (e) tag block:

```diff
- // <AYAstorm r41 PC-N-5 (e)> AYAGltfStubDrawEnabled cvar=true 時、
- //   recordAvatarPlaceholderDraw 末尾から並走 fire ((N5-4) A)
- static LLCachedControl<bool> sAyastormGltfStubDrawEnabled(
-     gSavedSettings, "AYAGltfStubDrawEnabled", false);
- if (sAyastormGltfStubDrawEnabled)
- {
-     recordGltfAssetDraw(cmd_buf);
- }
- // </AYAstorm r41 PC-N-5 (e)>
+ // <AYAstorm r41 PC-N-10 (a)> AYAGltfRealDrawEnabled cvar=true 時、
+ //   recordAvatarPlaceholderDraw 末尾から real GLTF Vulkan draw fire ((N10-6) A + (N10-16) A)
+ //   PC-N-5 (e) AYAGltfStubDrawEnabled cvar から PC-N-9 AYAGltfRealDrawEnabled cvar に切替
+ //   = 3 stub cvar 統合 + Phase 1.D complete marker 起案
+ static LLCachedControl<bool> sAyastormGltfRealDrawEnabled(
+     gSavedSettings, "AYAGltfRealDrawEnabled", false);
+ if (sAyastormGltfRealDrawEnabled)
+ {
+     recordGltfAssetDraw(cmd_buf);
+ }
+ // </AYAstorm r41 PC-N-10 (a)>
```

### §4.2 step (b) `recordGltfAssetDraw` 内 PC-N-6 (e) + PC-N-7 (e) + PC-N-5 base 経路撤去

`indra/llrender/llvkloader.cpp:6063-6519` recordGltfAssetDraw 本体内:

撤去対象:
- PC-N-6 (e) AYAGltfStubVertexBufferEnabled cvar 分岐ブロック (line 6230-6331 付近)
- PC-N-7 (e) AYAGltfStubIndexBufferEnabled cvar 分岐ブロック (line 6340-6431 付近)
- PC-N-5 base shader generate 3 vertex 経路 (line 6438-6519 付近、sAvatarBonePipeline + writeDrawUbo + writeSkinUbo(sGltfStubSkin, identity 256B) + flushSkinUbos + bindV3aRigged + push constant identity + vkCmdDraw(3,1,0,0) + first-fire LL_INFOS marker)

維持:
- PC-N-8 (f) real Asset path (line 6092-6207 付近、PC-N-9 (b) wrap 内側)

### §4.3 step (c) PC-N-9 (b) cvar guard wrap 撤去

`indra/llrender/llvkloader.cpp:6092 付近` PC-N-9 (b) tag block:

```diff
- // <AYAstorm r41 PC-N-9 (b)> AYAGltfRealDrawEnabled cvar gate
- static LLCachedControl<bool> sAyastormGltfRealDrawEnabled(
-     gSavedSettings, "AYAGltfRealDrawEnabled", false);
- if (sAyastormGltfRealDrawEnabled)
- {
-     // <AYAstorm r41 PC-N-8 (f)> ... (既配線、無変更)
- }
- // </AYAstorm r41 PC-N-9 (b)>
+ // <AYAstorm r41 PC-N-8 (f)> ... (既配線、無変更、PC-N-9 (b) cvar guard wrap は
+ //   PC-N-10 (c) で撤去 = entry hook 側が AYAGltfRealDrawEnabled cvar gate に切替わるゆえ二重 gate 冗長)
```

PC-N-8 (f) block 内部 first-fire LL_INFOS marker + 5 段 graceful degrade は無変更で温存。

### §4.4 step (d) stub VB/IB storage + initVulkan VMA allocate + shutdownVulkan vmaDestroyBuffer 撤去

`indra/llrender/llvkloader.cpp` 全 site:

撤去対象:
- PC-N-6 (a) sGltfStubVertexBuffer + sGltfStubVertexAllocation + sGltfStubVertexMapped + sGltfStubVertexData[3] + 関連 constexpr storage (line 608-650 付近)
- PC-N-6 (c) initVulkan 内 vmaCreateBuffer + memcpy(mapped, sGltfStubVertexData, ...) + 3 段 graceful degrade
- PC-N-6 (d) shutdownVulkan 内 vmaDestroyBuffer(sAllocator, sGltfStubVertexBuffer, ...) + nullify
- PC-N-7 (a) sGltfStubIndexBuffer + sGltfStubIndexAllocation + sGltfStubIndexMapped + sGltfStubIndexData[3] + sGltfStubIndexCount/Stride/BufferSize 関連 constexpr storage
- PC-N-7 (c) initVulkan 内 vmaCreateBuffer + memcpy(mapped, sGltfStubIndexData, ...) + 3 段 graceful degrade
- PC-N-7 (d) shutdownVulkan 内 vmaDestroyBuffer(sAllocator, sGltfStubIndexBuffer, ...) + nullify

維持:
- PC-N-5 (a) sGltfStubSkin sentinel storage ((N10-3) B)
- PC-N-5 (c)/(d) initVulkan registerSkinUbo(sGltfStubSkin) + shutdownVulkan unregisterSkinUbo(sGltfStubSkin) ((N10-3) B)
- sGltfStubAssetPipeline 全 lifecycle (= create + destroy) ((N10-5) B、PC-N-8 (f) 再利用)

### §4.5 step (e) settings.xml 3 stub cvar 削除

`indra/newview/app_settings/settings.xml`:

撤去対象 3 cvar XML block 全 (= `<key>...</key><map><key>Comment</key><string>...</string><key>Persist</key><integer>1</integer><key>Type</key><string>Boolean</string><key>Value</key><integer>0</integer></map>` + 直前 Comment):
- `AYAGltfStubDrawEnabled` (line 10408-10422 付近)
- `AYAGltfStubVertexBufferEnabled` (line 10425-10443 付近)
- `AYAGltfStubIndexBufferEnabled` (line 10446-10466 付近)

維持:
- `AYAGltfRealDrawEnabled` (line 10468-10487 付近、Comment 内容のみ (f) で更新)

### §4.6 step (f) AYAGltfRealDrawEnabled Comment 更新

`indra/newview/app_settings/settings.xml` AYAGltfRealDrawEnabled XML block Comment 内:

更新内容:
- 「PC-N-10 で 3 stub cvar deprecate 予定」記述削除
- 「3 stub cvar 統合済 (= AYAGltfStubDrawEnabled / AYAGltfStubVertexBufferEnabled / AYAGltfStubIndexBufferEnabled は Phase 1.D PC-N-10 で完全削除)」追記
- 「Phase 1.D complete marker = 1 GLTF asset 完全 Vulkan draw 通電」追記
- cvar 優先順位記述「PC-N-9 > PC-N-7 > PC-N-6 > PC-N-5」を「PC-N-10 単独 = AYAGltfRealDrawEnabled のみ」に簡素化
- 「ON = recordAvatarPlaceholderDraw 末尾 hook 経由 recordGltfAssetDraw fire + PC-N-8 (f) real LL::GLTF::Asset 経由 vertex/index buffer bind + UBO sequence + bindVertexBufferVk + bindIndexBufferVk(UINT32) + vkCmdDrawIndexed(real_index_count, 1, 0, 0, 0) 経路発火」明示

### §4.7 step (g) cross-platform spec §6 PC-N-10 行更新

`docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md`:

- §6 PC-N-10 行 = ⏳ → ✅ + Phase 1.D complete marker 反映
- 内容: 「PC-N-10 cleanup = 3 stub cvar deprecate + stub VB/IB storage 撤去 + PC-N-6 (e) + PC-N-7 (e) + PC-N-5 base 経路撤去 + PC-N-9 (b) cvar guard 撤去 + AYAGltfRealDrawEnabled 一本化、(N10-1)..(N10-16) 16 件 ambiguity 全件推奨案採用 OK 2026-06-05」
- macOS 派生 fix 候補: なし (= host-side cleanup ゆえ OS 非依存、descriptor set 数は PC-N-9 と同 5 set 維持、sGltfStubSkin sentinel + sGltfStubAssetPipeline 維持で MoltenVK 影響増なし)
- Windows 派生 fix 候補: なし (= full Vulkan)
- §A 更新履歴 1 行追記

design-lock phase 時点では §6 行は ⏳ design-lock complete + 実装 ⏳ 状態。

### §4.8 step (h) Phase 1.D complete marker 起案

PC-N-10 complete handoff doc 内に **Phase 1.D complete marker** 統合明示:
- §0 PC-N-10 完了 = Phase 1.D 全完了 marker
- §5 残 strict 線形に「Phase 1.D complete ✅ 本 commit + Phase 1.E ⏳ 次 phase」明記
- §6 r41 milestone state に「Phase 1.D complete ✅」反映
- Phase 1.D summary 別 §記載 (= PC-N-5 着手起点 + PC-N-6/7/8/9 各 sub-step + PC-N-10 cleanup の歴史 1 表)

### §4.9 step (i) build verify literal 取得

PC-N-6/7/8/9 同形 scope ((N10-13) A):

| # | check | 期待 result |
|---|-------|-------------|
| 1 | `make -j4 llrender` | `[100%] Built target llrender` + ERROR 0 + WARNING 0 |
| 2 | `INTEGRATION_TEST_lluboringbuffer` | 11/11 PASS YAY!! |
| 3 | `INTEGRATION_TEST_llassetubopool` | 10/10 PASS YAY!! |
| 4 | `INTEGRATION_TEST_llpipelinecachestorage` | 13/13 PASS YAY!! |
| 5 | `python3 -m unittest discover tests` (codegen) | 131/131 OK |
| 6 | `grep -c LL_VULKAN_GLSL indra/llrender/llvkloader.cpp` | 6 (= PC-N-9 commit `50b1171cff` 同数、GATE-B integrity 維持) |

### §4.10 step (j) handoff complete doc 起案

PC-N-6/7/8/9 同形 pattern:
- `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-...-phase1-d-pc-n-10-complete.md`
- §0 literal scope 7 件全件実装結果 + Phase 1.D complete marker 統合明示
- §1 実装結果 = 10 site 全実施
- §2 build verify literal 取得結果
- §3 Exit Criteria 10 項全充足
- §4 改変 file 一覧
- §5 残 strict 線形 (= Phase 1.D complete ✅ + Phase 1.E ⏳)
- §6 r41 milestone state (= Phase 1.D complete 反映)
- §7 self-verify 9 観点 全 ✅
- §8 次 session 着手 1 line = Phase 1.E design-lock 着手 (= multi-asset / multi-skin / worker thread)
- §A feedback 遵守 record

---

## §4.11 GATE-B 整合 (= memory `project_r41_phase1b_vulkan_host_gate`)

- `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 = `grep -c LL_VULKAN_GLSL indra/llrender/llvkloader.cpp` 結果は PC-N-9 commit `50b1171cff` と同数 (= 6) 維持
- 撤去 phase ゆえ `LL_VULKAN_GLSL` 既存 6 件は無変更維持 (= count 不変)
- cvar 新設 0 件 + 既存 4 cvar から 3 件削除 = 削減 phase

## §4.12 MUSEUBO-A 整合

- PC-N-10 完了後 default 状態 = `AYAGltfRealDrawEnabled=false`
- `AYAGltfRealDrawEnabled=false` 時 recordGltfAssetDraw fire entry hook 自体 skip = recordGltfAssetDraw 全経路発火なし
- → OpenGL 描画 100% 維持 + Vulkan host-side 新規 redirect 経路発火ゼロ = `feedback_visual_decisions_need_live_ab` 整合
- `AYAGltfRealDrawEnabled=true` 時のみ PC-N-8 (f) real Asset path fire = 5 段 graceful degrade (sCurrentAsset / sCurrentPrimitive / sGltfStubAssetPipeline / sPrimitiveVertexBuffers find / sPrimitiveIndexBuffers find 各 nullptr guard)

## §4.13 設計原則整合 (= memory `project_ayastorm_r41_design_principles`)

- **(1) Upstream OpenGL 取り込みやすさ維持**:
  - `recordAvatarPlaceholderDraw` 末尾 hook 配置温存 (= PC-N-5 (e) → PC-N-10 (a) cvar 名のみ切替) + GLTFSceneManager::render OpenGL path 完全温存 + drawRangeFast 経路無変更 + `recordGltfAssetDraw` signature 不変 ((N8-6) A 維持) + sGltfStubAssetPipeline 維持 + shader 改変ゼロ
- **(2) Core プロセス分散実現**:
  - per-Primitive setCurrentPrimitive/clearCurrentPrimitive hook + per-Primitive vertex/index buffer ownership ((N8-1) B) 維持 = primitive-level granularity の worker thread 分散余地確保 + Phase 1.E (multi-asset / worker thread) 着手起点

## §4.14 想定改変 file 4 件 (= 実装 phase = 別 session)

| # | file | 改変概要 | 想定 LoC |
|---|------|---------|---------|
| 1 | `indra/llrender/llvkloader.cpp` | (a)+(b)+(c)+(d) = entry hook cvar 切替 + PC-N-6 (e) + PC-N-7 (e) + PC-N-5 base 経路撤去 + PC-N-9 (b) cvar guard 撤去 + stub VB/IB storage + initVulkan VMA allocate + shutdownVulkan vmaDestroyBuffer 撤去 (sGltfStubSkin sentinel + sGltfStubAssetPipeline 維持) | +20/-300 推定 (= 大量撤去) |
| 2 | `indra/newview/app_settings/settings.xml` | (e)+(f) = 3 stub cvar 行削除 + AYAGltfRealDrawEnabled Comment 更新 | +10/-50 推定 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md` | (g) §6 PC-N-10 行 ✅ 反映 + Phase 1.D complete marker + §A 履歴 1 行追記 | +3/-2 推定 |
| 4 | new = `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-...-phase1-d-pc-n-10-complete.md` | (h)+(j) PC-N-10 complete handoff doc + Phase 1.D complete marker 統合明示 | new |

`llvkloader.h` 改変 0 件 (= PC-N-8 (e) accessor 既宣言済、PC-N-10 で削除する API 0 件)。
shader 改変 0 件、codegen 改変 0 件、CMake 改変 0 件、tests/ 改変 0 件、settings/storage 物理 削除 (cvar 行削除) のみ。

---

## §5. PC-N-10 design-lock Exit Criteria 9 項全充足

| # | Criteria | status |
|---|----------|--------|
| (i) | PC-N-10 literal scope §0 明文化 (= AYA 起案文 + Phase 1.D decomposition §4.5 + PC-N-9 complete §1.3 整合の 7 件 literal scope) | ✅ |
| (ii) | 必読 1 件 §1.1 (PC-N-9 complete doc) + pinpoint reference 11 件 §1.2 別記 = full file dump なし | ✅ |
| (iii) | ambiguity (N10-1)..(N10-16) 16 件 + AYA literal「全件推奨で OK」record (2026-06-05) §3 | ✅ |
| (iv) | 採用根拠 16 件明文化 §3 | ✅ |
| (v) | 実装計画 (a)-(j) 10 step 分解 + 各 step 具体 code diff example 添付 §4.1-§4.10 | ✅ |
| (vi) | 実装 phase Exit Criteria 10 項明文化 §6 | ✅ |
| (vii) | GATE-B 整合 §4.11 + MUSEUBO-A 整合 §4.12 + 設計原則整合 §4.13 | ✅ |
| (viii) | 想定改変 file 4 件明文化 §4.14 | ✅ |
| (ix) | `indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合 | ✅ |

---

## §6. PC-N-10 実装 phase Exit Criteria (= 10 項、(N10-14) A)

| # | criterion |
|---|-----------|
| (i) | `recordAvatarPlaceholderDraw` 末尾 PC-N-5 (e) hook の cvar 名のみ `AYAGltfStubDrawEnabled` → `AYAGltfRealDrawEnabled` 切替 + tag block 名称 PC-N-10 (a) に rename ((N10-6) A + (N10-16) A) |
| (ii) | `recordGltfAssetDraw` 内 PC-N-6 (e) + PC-N-7 (e) + PC-N-5 base shader generate 経路全撤去 ((N10-7) A + (N10-9) A) |
| (iii) | PC-N-9 (b) cvar guard wrap 撤去 ((N10-8) A) |
| (iv) | `sGltfStubVertexBuffer` / `sGltfStubIndexBuffer` storage + initVulkan VMA allocate + shutdownVulkan vmaDestroyBuffer 全撤去 ((N10-4) A) |
| (v) | `sGltfStubSkin` sentinel + `sGltfStubAssetPipeline` 維持 ((N10-3) B + (N10-5) B) |
| (vi) | settings.xml 3 stub cvar 行完全削除 + AYAGltfRealDrawEnabled Comment 更新 ((N10-2) A + (N10-10) A) |
| (vii) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp=6 不変、PC-N-9 commit `50b1171cff` 同数) |
| (viii) | MUSEUBO-A 整合 = `AYAGltfRealDrawEnabled=false` default で recordGltfAssetDraw 全経路発火なし + PC-N-8 (f) 5 段 graceful degrade 維持 |
| (ix) | build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 ((N10-13) A) |
| (x) | Phase 1.D complete marker 統合明示 + PC-N-10 complete handoff doc 起案 ((N10-11) A) + cross-platform spec §6 PC-N-10 行 ✅ 反映 + §A 履歴 1 行追記 ((N10-12) A) |

### §6.1 想定 build verify command (= 実装 phase = 別 session)

```bash
cd /home/ishikawa/work_firestorm/phoenix-firestorm
cd build-linux-x86_64 && make -j4 llrender 2>&1 | tail -20
./indra/llrender/INTEGRATION_TEST_lluboringbuffer
./indra/llrender/INTEGRATION_TEST_llassetubopool
./indra/llrender/INTEGRATION_TEST_llpipelinecachestorage
cd ../scripts/ubo_codegen && python3 -m unittest discover tests
grep -c LL_VULKAN_GLSL ../../indra/llrender/llvkloader.cpp  # = 6 期待
```

### §6.2 cross-platform spec §6 PC-N-10 行更新内容 draft (= 実装 phase で本記述に置換)

```
| **PC-N-10** | (= **Phase 1.D 内 5th = 最終 sub-step = cleanup + 3 stub cvar deprecate**、
  (N10-1) A 採用 = 3 stub cvar 全 deprecate + (N10-2) A 完全削除 + (N10-3) B sGltfStubSkin 維持 +
  (N10-4) A stub VB/IB storage 撤去 + (N10-5) B sGltfStubAssetPipeline 維持 +
  (N10-6) A entry hook cvar 名のみ切替 + (N10-7) A PC-N-6/7 stub 経路撤去 + (N10-8) A PC-N-9 (b) wrap 撤去 +
  (N10-9) A PC-N-5 base 経路撤去 + (N10-10) A Comment 更新 + (N10-11) A Phase 1.D complete marker 統合 +
  (N10-12) A 本 §6 行更新 + (N10-13) A build verify scope + (N10-14) A Exit Criteria 10 項 +
  (N10-15) A 10 step (a)-(j) + (N10-16) A tag rename + 全件推奨 OK record 2026-06-05) |
  host-side cleanup ゆえ OS 非依存 + descriptor set 数は PC-N-9 と同 5 set 維持 +
  sGltfStubSkin sentinel + sGltfStubAssetPipeline 維持で MoltenVK 影響増なし | full Vulkan ゆえ派生 fix 候補なし想定 |
  ✅ design-lock + 実装 complete 本 commit (= llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 +
  GATE-B integrity = LL_VULKAN_GLSL count llvkloader.cpp=6 不変 = Phase 1.D complete marker) |
```

---

## §7. 着手手順 5 step (= 別 session 実装 phase)

1. 必読 1 件 (本 doc) Read + pinpoint reference 11 件 Read (= full file dump 回避、`feedback_handoff_minimal_pre_req_read` 整合)
2. step (a)-(j) 10 step 順次実施 (= `indra/llrender/llvkloader.cpp` + `indra/newview/app_settings/settings.xml` 改変)
3. build verify literal 取得 (= §6.1 command 順次実行)
4. cross-platform spec §6 PC-N-10 行 ✅ 反映 + §A 履歴 1 行追記 + handoff complete doc 起案 (= Phase 1.D complete marker 統合明示)
5. self-verify 9 観点 確認 → AYA 明示 commit 指示受領後 commit (= `feedback_no_auto_commit` 整合)

---

## §8. 残 strict 線形

- ✅ Phase 1.A / 1.B / (Z) SSS / (W) uniform4iv / (Y) Phase 1.C prep
- ✅ PC-0..PC-7ε / PC-N decomposition / PC-N-1..PC-N-4 (= Phase 1.C complete)
- ✅ PC-8 Linux primary marker (= Phase 1.C strict 線形終了)
- ✅ PC-N-5 (= Phase 1.D 着手起点) / Phase 1.D decomposition design-lock
- ✅ PC-N-6 / PC-N-7 / PC-N-8 / PC-N-9 (= Phase 1.D 1st-4th sub-step)
- ✅ **PC-N-10 design-lock ✅ 本 commit** (= Phase 1.D 内 5th = 最終 sub-step design-lock)
- ⏳ PC-N-10 実装 = 次 session 着手 (= cleanup + 3 stub cvar deprecate + Phase 1.D complete marker 起案)
- ⏳ **Phase 1.D complete** = PC-N-10 実装完了 marker = 1 GLTF asset 完全 Vulkan draw 通電
- ⏳ Phase 1.E (= multi-asset / multi-skin / worker thread) = 別 phase の別 session で別途分解

---

## §9. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ +
(Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α..PC-7ε ✅ +
PC-N decomposition design-lock ✅ + PC-N-1..PC-N-4 ✅ = Phase 1.C complete ✅ +
PC-8 Linux primary marker ✅ = Phase 1.C strict 線形終了 ✅ +
PC-N-5 ✅ = Phase 1.D 着手起点 ✅ + Phase 1.D decomposition design-lock ✅ +
PC-N-6 ✅ + PC-N-7 ✅ + PC-N-8 ✅ + PC-N-9 ✅ +
**PC-N-10 design-lock ✅ 本 commit** + PC-N-10 実装 ⏳ + Phase 1.D complete ⏳

---

## §10. self-verify 9 観点 全 ✅

1. ✅ **PC-N-10 literal scope §0 完全分解 7 件** = 3 stub cvar deprecate + AYAGltfRealDrawEnabled 一本化 + PC-N-5 base 経路撤去 + PC-N-6 (e) + PC-N-7 (e) 撤去 + PC-N-9 (b) wrap 撤去 + stub VB/IB storage 撤去 + sGltfStubSkin + sGltfStubAssetPipeline 維持 (= AYA 起案文 (今 session) + Phase 1.D decomposition §4.5 + PC-N-9 complete §1.3 record 整合)
2. ✅ **必読 1 件 §1.1 + pinpoint reference 11 件 §1.2 別記** = PC-N-9 complete doc + 各 site pinpoint Read のみ、full file dump なし (= `feedback_handoff_minimal_pre_req_read` 整合)
3. ✅ **現状調査 §2 8 項網羅** = sGltfStubSkin sentinel storage + stub VB/IB storage + PC-N-5 (e) entry hook + recordGltfAssetDraw 内 cvar 優先順位 + settings.xml 4 cvar 配置 + sGltfStubAssetPipeline 再利用 + §4.5 vs AYA 起案文緊張点 + GLTFSceneManager::render fire entry
4. ✅ **ambiguity (N10-1)..(N10-16) 16 件 AYA literal「全件推奨で OK」record (2026-06-05) §3** + 採用根拠 16 件明文化
5. ✅ **実装計画 (a)-(j) 10 step §4 + 各 step 具体 code diff example 添付** (= entry hook cvar 切替 + PC-N-6/7/8/9 cvar 分岐ブロック撤去 + stub VB/IB storage 撤去 + settings.xml 3 cvar 行削除 + Comment 更新 + cross-platform spec §6 PC-N-10 行更新 + Phase 1.D complete marker 起案 + build verify + handoff complete doc 起案)
6. ✅ **GATE-B 整合 §4.11** (= `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、cvar 新設 0 件 + 既存 4 cvar から 3 件削除 = 削減 phase、shader 改変ゼロ)
7. ✅ **MUSEUBO-A 整合 §4.12** (= `AYAGltfRealDrawEnabled=false` default で recordGltfAssetDraw 全経路発火なし + PC-N-8 (f) 5 段 graceful degrade 維持)
8. ✅ **設計原則整合 §4.13** (= (1) Upstream OpenGL 取り込みやすさ維持 = entry hook 配置温存 + signature 不変 + sGltfStubAssetPipeline 維持 + shader 改変ゼロ + (2) Core プロセス分散実現 = per-Primitive hook + per-Primitive vertex/index buffer ownership 維持 = Phase 1.E 着手起点)
9. ✅ **`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合** (= 本 design-lock phase は doc 起案のみ、実装は別 session)

---

## §11. 次 session 着手 1 line

PC-N-10 実装着手 = step (a)-(j) 10 step 実施 = (a) entry hook cvar 切替 + (b) PC-N-6 (e) + PC-N-7 (e) + PC-N-5 base 経路撤去 + (c) PC-N-9 (b) cvar guard 撤去 + (d) stub VB/IB storage + initVulkan + shutdownVulkan 撤去 + (e) settings.xml 3 stub cvar 削除 + (f) AYAGltfRealDrawEnabled Comment 更新 + (g) cross-platform spec §6 PC-N-10 行更新 + (h) Phase 1.D complete marker 起案 + (i) build verify literal 取得 (= llrender + WARNING 0 + TUT 11+10+13 + codegen 131/131 + GATE-B integrity LL_VULKAN_GLSL count llvkloader.cpp=6 不変) + (j) handoff complete doc 起案 (= Phase 1.D complete marker 統合明示) + Exit Criteria 10 項 self-verify + AYA commit 指示受領後 commit。

---

## §A. feedback 遵守 record

- ✅ `feedback_proactive_handoff` (本 PC-N-10 design-lock doc 起案)
- ✅ `feedback_handoff_minimal_pre_req_read` (必読 1 件 + pinpoint reference 11 件別記、本 session も Read pinpoint のみ = PC-N-9 complete doc 全文 + Phase 1.D decomposition §4.5 + cross-platform spec §6 PC-N-10 行 + llvkloader.cpp PC-N-5 (a)/(c)/(d)/(e) site + recordGltfAssetDraw 内 cvar 優先順位 + settings.xml 4 cvar 配置、full file dump なし)
- ✅ `feedback_self_verify_before_handoff` (9 観点 self-verify 全 ✅)
- ✅ `feedback_build_only_verified` (design-lock phase は `indra/` 改変 0 件で build verify 対象外、実装 phase で literal 検証取得予定 (N10-13) A 採用)
- ✅ `feedback_no_scope_shrink` (PC-N-10 literal scope §0 完全分解 7 件 = AYA 起案文 (今 session) が source of truth、Phase 1.D decomposition §4.5 literal「AYAGltfStubDrawEnabled 1 件のみ」と緊張ありだが AYA literal「全件推奨で OK」一括確認受領 (2026-06-05) で 3 stub cvar 全 deprecate 確定 = 拡張ではあるが縮小ではない、`feedback_ubo_migration_one_at_a_time` 厳格遵守整合)
- ✅ `feedback_doubt_self_first` (design-lock phase で ambiguity 16 件発見 + 推奨案提示 + AYA literal「全件推奨で OK」record 後本 design-lock doc 起案、特に (N10-1) §4.5 vs AYA 起案文緊張点 + (N10-3) sGltfStubSkin 維持 + (N10-5) sGltfStubAssetPipeline 維持 + (N10-6) entry hook 完全撤去 vs cvar 名切替の trade-off を Grep/Read で literal 確認後判断、推測実装なし)
- ✅ `feedback_confirm_referent_before_acting` (16 件 batch AYA 確認 (2026-06-05)、各候補 + 推奨案 + 根拠明示後 AYA literal 一括「全件推奨で OK」record 受領で確定、推測実装なし、特に (N10-6) entry hook 処理は 3 候補全列挙後採用、(N10-3) sGltfStubSkin 維持は PC-N-8 (f) line 6170 sentinel 共用根拠提示後採用)
- ✅ `feedback_ubo_migration_one_at_a_time` 厳格遵守 (PC-N-10 = cleanup + 3 stub cvar deprecate 単独 sub-step = entry hook cvar 切替 + 3 cvar 撤去 + PC-N-5/6/7 stub 経路撤去 + stub VB/IB storage 撤去、本 doc 起案も PC-N-10 単独 design-lock のみ、Phase 1.E = multi-asset / multi-skin / worker thread は別 phase の別 session で別途分解)
- ✅ `feedback_design_phase_no_code_write` 厳格遵守 (本 PC-N-10 design-lock phase は doc 起案のみ、`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件)
- ✅ `feedback_release_branch_workflow` 遵守 (feature branch `feature/ayastorm-r41-gl-removal` 上 commit 予定)
- ✅ `feedback_no_auto_commit` 遵守 (AYA 明示 commit 指示「commit してください」literal 受領後 commit)
- ✅ `feedback_no_claude_coauthor` 遵守 / Co-Authored-By 行不在
- ✅ `feedback_no_bare_reference_ids` 遵守 ((N10-1)..(N10-16) 各 ID に項目名 / 採用案内容併記 + (a)..(j) 各 step に作業内容併記)
- ✅ `feedback_tests_dir_never_commit` 整合 (tests/ 改変 0 件、git add 個別 file 指定予定)
- ✅ memory `project_ayastorm_r41_design_principles` 整合 ((1) Upstream OpenGL 取り込みやすさ維持 = entry hook 配置温存 + signature 不変 + sGltfStubAssetPipeline 維持 + GLTFSceneManager::render 改変 0 件 + shader 改変ゼロ + (2) Core プロセス分散実現 = per-Primitive hook + per-Primitive vertex/index buffer ownership 維持 = Phase 1.E (multi-asset / worker thread) 着手起点)
- ✅ memory `project_r41_phase1b_vulkan_host_gate` 整合 (GATE-B = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、cvar 新設 0 件 + 既存 4 cvar から 3 件削除 = 削減 phase、count=6 不変)
- ✅ memory `project_ayastorm_three_platforms` 整合 (cross-platform spec §6 PC-N-10 行更新 = host-side cleanup ゆえ OS 非依存 + descriptor set 数 5 維持 + sGltfStubSkin sentinel + sGltfStubAssetPipeline 維持で MoltenVK 影響増なし、Windows full Vulkan ゆえ派生 fix 候補なし、Linux primary 完成 → 他者補完 model 整合)
