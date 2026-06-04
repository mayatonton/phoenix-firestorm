---
title: r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-7γ-2 design-lock + 段階分割 pin
date: 2026-06-05
status: design-locked (PC-7γ-2 = defensive 配線 only) / pending implementation
parent_handoff: handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-gamma-1-complete.md
---

# PC-7γ-2 design-lock + 段階分割 pin

## §0 本 handoff doc の位置付け

本 doc は **実装着手前** の design-lock handoff である (= 編集 0 件 / commit なし)。PC-7γ-1 design-lock 起案時と同形のフロー = 本 doc 起案 → AYA literal 確認 → 次 session で /clear → 本 doc + 必読 3 件 Read → コード編集着手、の順とする (= AYA 2026-06-05 literal「handoff 作成で段階実装してきませんか？　物量も見えませんし」整合)。

本 doc の主要目的 = (a) PC-7γ-2 着手前に発見した **構造的事実** (= GLTF subsystem bare OpenGL UBO 直叩き + codegen Asset_*/Skin_* block 未生成) を design-lock として pin、(b) **段階分割** (PC-7γ-2 defensive 配線 only / PC-7γ-3 本格化) の合理性を 5 件 ambiguity AYA 確認 record と紐付け、(c) 物量見積もりを実 trace 結果と共に提示し AYA さんに次 sub-step scope の妥当性判断材料を渡す。

## §1 必読 (= 次 session 実装着手前 minimum 3 件)

1. **本 doc 全文** (= PC-7γ-2 段階分割 + (D1)..(D5) AYA 確認 record + Exit Criteria 7 項 + 物量見積もり + 設計詳細)
2. **PC-7γ-1 complete handoff** = `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-gamma-1-complete.md` 全文 (= per-program 経路通電 + UboInstanceKey 構造 + sFrameUboInstances 配線 + 4 method 雛形)
3. **design/06b-cadence-update-site-and-dirty.md §2.4 + §2.5 + §5.2** (= per-asset / per-skin cadence 「既存関数継承」設計 + forwardToUboUpload routing 5 case 分岐 + getCurrentAssetUbo/SkinUbo signature) **+ design/06a-cache-structure-and-setter-redirect.md §5.4** (= setter mUseUBO 分岐 / 構造的 gate)

**pinpoint reference** (= 必要時 Read、context 節約のため初手では読まない):

- `indra/llrender/llglslshader.cpp`:
  - line 83-107 (= PC-7γ-1 anonymous ns kCadence* + lookup_block_size_by_hash helper)
  - line 96-97 (= `kCadencePerAsset` / `kCadencePerSkin` 定義 + 「現 codegen 0 件、PC-7γ-2 で本格化」comment、PC-7γ-2 で comment 更新対象)
  - line 2109-2188 (= PC-7γ-1 forwardToUboUpload 9 case switch、line 2152-2163 PER_ASSET/PER_SKIN case stub = PC-7γ-2 置換対象)
- `indra/llrender/llvkloader.h`:
  - line 339-340 (= flushAssetUbos / flushSkinUbos 宣言、PC-6ε-2 既存)
  - line 383-403 (= PC-7γ-1 4 method 宣言、PC-7γ-2 で 6 method (= asset 3 + skin 3) + sCurrent 4 method 追加対象)
- `indra/llrender/llvkloader.cpp`:
  - line 494-515 (= PC-6ε-2 dirty map 3 件 + comment、PC-7γ-2 で sAssetUboDirty/sSkinUboDirty key refactor)
  - line 524-530 (= PC-7γ-1 sFrameUboInstances + comment、PC-7γ-2 で sCurrentAsset/sCurrentSkin 追加位置参考)
  - line 3895-3927 (= PC-6ε-2 flushAssetUbos / flushSkinUbos、PC-7γ-2 で map key 拡張に伴う walk 経路追従)
  - line 3960-... (= PC-7γ-1 4 method 定義、PC-7γ-2 で 6 method (= 2 cadence × 3 method) + sCurrent setter/clearer 4 method 配置参考)
- `build-linux-x86_64/codegen/ubo/ubo_metadata.inl` (= 91 block 全件、cadence_tag 分布 = `0u`×3 / `1u`×80 / `2u`×7 / `5u`×1、**PER_ASSET (=3) / PER_SKIN (=4) entry 0 件**、`Asset_*` / `Skin_*` prefix の block name も 0 件 = D5 確定根拠)
- `indra/newview/gltf/asset.cpp`:
  - line 181-187 (= `Asset::updateNodeData()`、`glGenBuffers + glBufferData → mNodesUBO`、PC-7γ-3 で writeAssetUbo 経由置換)
  - line 230-236 (= `Asset::updateMaterialData()`、`glGenBuffers + glBufferData → mMaterialsUBO`、PC-7γ-3 で writeAssetUbo 経由置換)
- `indra/newview/gltf/animation.cpp`:
  - line 396-411 (= `Skin::~Skin()` 周辺 + UBO lifecycle、PC-7γ-3 unregister hook 候補 site)
  - line 455-456 (= `Skin::updateTransforms()`、`glBindBuffer + glBufferData → mUBO`、PC-7γ-3 で writeSkinUbo 経由置換)
- `indra/newview/gltfscenemanager.cpp`:
  - line 693-702 (= `flushAssetUbos(&asset)` + `glBindBufferBase(UB_GLTF_NODES, asset.mNodesUBO)` + `glBindBufferBase(UB_GLTF_MATERIALS, asset.mMaterialsUBO)`、PC-7γ-2 で `setCurrentAsset(&asset)` / `clearCurrentAsset()` 配線位置)
  - line 736 + 743-746 (= `flushSkinUbos(&skin)` + `glBindBufferBase(UB_GLTF_JOINTS, skin.mUBO)`、PC-7γ-2 で `setCurrentSkin(&skin)` / `clearCurrentSkin()` 配線位置)

## §2 起案契機 + AYA 確認 record

### §2.1 起案契機

PC-7γ-1 commit (c1f2e2d53a、2026-06-05) 後、AYA literal「PC-7γ-1 commit 済 (c1f2e2d53a)。次は PC-7γ-2 着手お願いします。…Exit Criteria は着手前に整理してから確認お願いします。」受領 → 必読 3 件 Read (= PC-7γ-1 complete handoff + design 06b §2.4-§2.5/§5.2 + design 06a §5.4) → 既存 codebase trace (= ubo_metadata.inl 全 91 block + gltf/asset.cpp + animation.cpp + gltfscenemanager.cpp + llvkloader.cpp PC-6ε-2 dirty map + llglslshader.cpp PC-7γ-1 forwardToUboUpload stub) → PC-7γ-2 着手前 ambiguity 5 件 (D1)..(D5) を AYA 1 batch 報告 → AYA literal「OK」(2026-06-05) で 5 件全件 lock。

その後、(D5) ubo_metadata.inl cadence_tag 分布 grep + (D4) GLTF host write site trace を実施 → **構造的事実** (= PER_ASSET=3 / PER_SKIN=4 entry 0 件 + Asset_*/Skin_* prefix block 0 件 + GLTF subsystem は bare OpenGL UBO 直叩き) を発見 → (D5) 再分析で **(D5-rev) PC-7γ-2 を defensive 配線 only に再定義 + PC-7γ-3 後段新設** 推奨案を AYA 報告 → AYA literal「分ける推奨です」 + 「handoff 作成で段階実装してきませんか？　物量も見えませんし」(2026-06-05) で段階分割 + 本 design-lock handoff 起案承認。

### §2.2 AYA 判断確定 record (= 全 6 件 = 5 件 ambiguity + 段階分割)

| ID | 確認内容 | AYA 採用 | literal record |
|---|---|---|---|
| (D1-A) | PER_ASSET/PER_SKIN write 経路 = 経路 A (forwardToUboUpload defensive) + 経路 B (GLTF host write 置換) 両方 in scope、本丸は経路 B (= 06b §2.4 「既存関数継承」) | 採用 | 「OK」(2026-06-05、1st batch) |
| (D2-A) | "current asset/skin" 追跡 = LLVKLoader anonymous ns に `sCurrentAsset` / `sCurrentSkin` static (main thread 専有 = 06b §5.4.1 整合) + `setCurrentAsset/Skin` / `clearCurrentAsset/Skin` 配線、gltfscenemanager で set/clear | 採用 | 「OK」(2026-06-05、1st batch) |
| (D3-A) | map key 拡張 = `UboAssetKey = std::pair<LL::GLTF::Asset*, U32>` / `UboSkinKey = std::pair<LL::GLTF::Skin*, U32>` + 各 Hash struct (PC-7γ-1 UboInstanceKey 同形)、map 型 refactor | 採用 | 「OK」(2026-06-05、1st batch) |
| (D4-A) | register/unregister hook = 既存 GL resource lifecycle 関数 (= 06b §2.4 「既存関数継承」整合) で hook、具体 site は実 codebase trace 後 design-lock doc に literal 記録 | 採用 (但し PC-7γ-3 持越に変更、D5-rev 整合) | 「OK」(2026-06-05、1st batch) |
| (D5-rev) | codegen Asset_*/Skin_* block 未生成 → PC-7γ-2 を **defensive 配線 only** に再定義 + PC-7γ-3 後段新設 (= codegen 追加 + bare OpenGL UBO 置換 + lifecycle hook) | 採用 | 「分ける推奨です」(2026-06-05、2nd) |
| 段階分割 + handoff 起案 | PC-7γ-2 を design-lock handoff 起案で段階実装の合理性 + 物量見積もり明示 | 採用 | 「handoff 作成で段階実装してきませんか？　物量も見えませんし」(2026-06-05、3rd) |

### §2.3 採用根拠 record

- **(D1-A) 両経路 in scope 根拠** = design 06b §5.2 literal の switch 5 case 構造完全網羅 + 経路 A (forwardToUboUpload) は現 codegen 0 件で defensive のみだが switch 構造の完全性 + PC-7δ 通電時に経路 A も含めた 5 case 検証可能、経路 B (GLTF host write 置換) は 06b §2.4 「既存関数継承」literal 整合の本丸 = 両方含めて PC-7γ scope 全体、PC-7γ-2 / PC-7γ-3 で実装時期分離
- **(D2-A) sCurrent static 根拠** = design 06b §5.2 `getCurrentAssetUbo(loc.block_hash)` literal で "current" owner 概念既存、§5.4.1 「現 phase main thread 専有」整合で static で thread-safe、setter/clearer pattern は gltfscenemanager.cpp:693-702 (= asset draw 直前) / 736 (= skin draw 直前) の **既存 flushAssetUbos/flushSkinUbos 配線点と同位置** で set/clear 可能 = call site 改変ゼロ (= 原則 1)、追加 setter 不要
- **(D3-A) map key 拡張根拠** = PC-7γ-1 で確立した UboInstanceKey 構造 (= `std::pair<Owner*, U32>` + Hash struct) を per-asset/per-skin に対称に展開、PC-6ε-2 の `<Owner*, UboInstance>` 単一 owner key は **1 owner 多 block 持つ owner (= 大規模 asset)** で dirty 共有 (= false sharing) 発生、`<Owner*, block_hash>` key で UBO 単位独立性確保、PC-7γ-1 で per-program で同根拠検証済
- **(D4-A) 既存 lifecycle hook 根拠** = design 06b §2.4 literal「**既存関数継承**: `gltf::Asset::updateNodeData()` (= `gltf/asset.cpp:183` 付近) / `Asset::updateMaterialData()` (= `gltf/asset.cpp:232` 付近) — inventory §1.1 で確認済の OpenGL path 関数を Vulkan path で `forwardToUboUpload` 経路に置換」整合、PC-7γ-1 の mapUniforms / unloadInternal hook と対称構造 (= shader lifecycle ↔ asset/skin lifecycle)、但し PC-7γ-3 で実装 (= 下記 D5-rev 整合)
- **(D5-rev) 段階分割根拠** = (1) ubo_metadata.inl 91 block 全件で cadence_tag 値 = `0u`×3 / `1u`×80 / `2u`×7 / `5u`×1 = **PER_ASSET (=3) / PER_SKIN (=4) entry 0 件** (実 grep 確認、2026-06-05)、(2) `Asset_*` / `Skin_*` prefix の block name も **0 件** (grep "Asset_GLTFNodes" / "Asset_GLTFMaterials" / "Skin_GLTFJoints" 全 0 hit)、(3) GLTF subsystem (gltf/asset.cpp + animation.cpp) は **bare OpenGL UBO API 直叩き** (= `glGenBuffers + glBufferData + glBindBufferBase`) で codegen perfect hash table 非経由、binding 番号 = `LLGLSLShader::UB_GLTF_NODES / UB_GLTF_MATERIALS / UB_GLTF_JOINTS` 直接 enum、(4) = PER_ASSET/PER_SKIN 経路 A (= forwardToUboUpload 経由) は現 codegen で実走不能、経路 B (= GLTF host write 置換) には **block_hash / block_size を何で識別するか** の設計問題が残る (= bare OpenGL UBO は size 動的 `std::vector::size() * sizeof(...)`、block_hash 概念不在 = synthetic ID scheme 設計が必要 = 06b §2.4 想定外で別 sub-step 設計が要る) = (5) PC-7γ-2 を **defensive 配線 only** (= 構造改修 + 将来 hook 準備) に再定義、PC-7γ-3 を後段新設 (= codegen Asset_*/Skin_* block 追加 or synthetic ID scheme + GLTF host write 置換 + lifecycle hook) として分離 = feedback_ubo_migration_one_at_a_time + feedback_no_scope_shrink 両整合 (= 「scope 縮小」ではなく「block_hash 未生成で本格化不能 = 構造的に分割せざるをえない」)
- **段階分割 + handoff 起案根拠** = PC-7γ-2 (defensive 配線、構造改修) と PC-7γ-3 (本格化、実走経路) は failure mode が独立、PC-7γ-2 で llrender + TUT 全 PASS 確認 → PC-7γ-3 で実走経路追加、の順番なら回帰隔離可能、handoff doc 起案で物量見積もり明示 + AYA さん次 sub-step scope 妥当性判断材料提供 (= PC-7γ-1 design-lock 起案 flow と同形)

## §3 PC-7γ-2 scope (= refinement 後 lock、defensive 配線 only)

### §3.1 PC-7γ-2 scope (実装対象 = 5 件 group)

**Title**: per-asset / per-skin cadence defensive 配線 = map key 拡張 + 6 method 新設 + sCurrentAsset/Skin tracking + forwardToUboUpload PER_ASSET/PER_SKIN case 置換 + gltfscenemanager set/clear 配線 (構造改修 only、現 codegen 0 件で実走しない)

**実装項目** (= 5 件):

1. **UboAssetKey / UboSkinKey + 各 Hash 新設** (anonymous ns、llvkloader.cpp):
   - `using UboAssetKey = std::pair<LL::GLTF::Asset*, U32 /*block_hash*/>;`
   - `using UboSkinKey  = std::pair<LL::GLTF::Skin*,  U32 /*block_hash*/>;`
   - `struct UboAssetKeyHash { size_t operator()(const UboAssetKey&) const noexcept; };` (= `std::hash<void*>(asset) ^ (std::hash<U32>(block_hash) << 1)`)
   - `struct UboSkinKeyHash  { size_t operator()(const UboSkinKey&)  const noexcept; };` (= 同 pattern)
   - PC-7γ-1 `UboInstanceKey` (= `<LLGLSLShader*, U32>`) と対称構造 = code shape 統一

2. **sAssetUboDirty / sSkinUboDirty 型 refactor**:
   - 旧: `std::unordered_map<LL::GLTF::Asset*, UboInstance> sAssetUboDirty;` (PC-6ε-2)
   - 新: `std::unordered_map<UboAssetKey, UboInstance, UboAssetKeyHash> sAssetUboDirty;`
   - 旧: `std::unordered_map<LL::GLTF::Skin*, UboInstance> sSkinUboDirty;` (PC-6ε-2)
   - 新: `std::unordered_map<UboSkinKey, UboInstance, UboSkinKeyHash> sSkinUboDirty;`
   - flushAssetUbos / flushSkinUbos の walk 経路追従 = `kv.first.first == asset` / `kv.first.first == skin` match check 追加 (= PC-7γ-1 flushProgramUbos 同形)

3. **LLVKLoader 6 method 新設** (llvkloader.h 宣言 + llvkloader.cpp 定義):
   - `bool registerAssetUbo  (LL::GLTF::Asset* asset, U32 block_hash, U32 block_size);`
   - `void unregisterAssetUbo(LL::GLTF::Asset* asset, U32 block_hash);`
   - `void writeAssetUbo     (LL::GLTF::Asset* asset, U32 block_hash, U32 offset, const void* data, size_t size);`
   - `bool registerSkinUbo  (LL::GLTF::Skin* skin, U32 block_hash, U32 block_size);`
   - `void unregisterSkinUbo(LL::GLTF::Skin* skin, U32 block_hash);`
   - `void writeSkinUbo     (LL::GLTF::Skin* skin, U32 block_hash, U32 offset, const void* data, size_t size);`
   - body = PC-7γ-1 4 method (= registerProgramUbo / unregisterProgramUbo / writeFrameUbo / writeProgramUbo) と完全対称 (= try_emplace / allocateUboInstanceBuffers / destroyUboInstanceBuffers / memcpy + dirty.store(release))
   - 注: 本 sub-step では **call site 不在** (= 構造的事実) = comment で「PC-7γ-3 で GLTF lifecycle hook + bare OpenGL UBO 置換時に呼出開始」明文化

4. **sCurrentAsset / sCurrentSkin static + setCurrent / clearCurrent 4 method 新設** (llvkloader.h 宣言 + llvkloader.cpp 定義):
   - anonymous ns に追加: `static LL::GLTF::Asset* sCurrentAsset = nullptr;` / `static LL::GLTF::Skin* sCurrentSkin = nullptr;`
   - 4 method 新設:
     - `void setCurrentAsset(LL::GLTF::Asset* asset);` (= `sCurrentAsset = asset;`)
     - `void clearCurrentAsset();` (= `sCurrentAsset = nullptr;`)
     - `void setCurrentSkin(LL::GLTF::Skin* skin);` (= `sCurrentSkin = skin;`)
     - `void clearCurrentSkin();` (= `sCurrentSkin = nullptr;`)
   - 注: main thread 専有 (= design 06b §5.4.1)、atomic 不要、locking 不要

5. **forwardToUboUpload PER_ASSET / PER_SKIN case 置換** (llglslshader.cpp、line 2152-2163 stub 置換):
   - `case kCadencePerAsset`: PC-7γ-1 `LL_WARNS_ONCE("PC-7γ-2 scope")` stub を以下に置換:
     ```cpp
     case kCadencePerAsset:
     {
         LL::GLTF::Asset* asset = LLVKLoader::getCurrentAsset();
         if (!asset)
         {
             LL_WARNS_ONCE("Vulkan") << "PC-7γ-2: PER_ASSET forwardToUboUpload — sCurrentAsset is nullptr, "
                                     << "host setCurrentAsset() not wired yet (PC-7γ-3 scope), block_hash=0x"
                                     << std::hex << loc.block_hash << std::dec << LL_ENDL;
             return;
         }
         LLVKLoader::writeAssetUbo(asset, loc.block_hash, loc.offset, data, size);
         break;
     }
     ```
   - `case kCadencePerSkin`: 同 pattern、`getCurrentSkin()` + `writeSkinUbo` 経由
   - 注: 現 codegen で kCadencePerAsset/Skin は 0 件で実走しない (= ubo_metadata.inl 確認済)、defensive 配線 + 将来 codegen 追加時 (= PC-7γ-3) に hot path 通電
   - 注 (重要): `LLVKLoader::getCurrentAsset()` / `getCurrentSkin()` accessor は本 sub-step で **追加宣言不要** = forwardToUboUpload 内側で `LLVKLoader` namespace 内 `sCurrentAsset` / `sCurrentSkin` を **直接参照**するのではなく、専用 accessor 経由 (= namespace 内 static の visibility 確保)。下記 §4.3 で実装雛形を明示

6. **gltfscenemanager.cpp set/clear 配線**:
   - line 693 `LLVKLoader::flushAssetUbos(&asset);` の直前に `LLVKLoader::setCurrentAsset(&asset);` 追加、glBindBufferBase 2 件の直後 (= asset draw 終了後)に `LLVKLoader::clearCurrentAsset();` 追加
   - line 736 `LLVKLoader::flushSkinUbos(&skin);` の直前に `LLVKLoader::setCurrentSkin(&skin);` 追加、glBindBufferBase の直後に `LLVKLoader::clearCurrentSkin();` 追加
   - 注: PC-7γ-3 で forwardToUboUpload PER_ASSET/PER_SKIN case が実走するときに `sCurrentAsset` / `sCurrentSkin` が non-null である必要、本 PC-7γ-2 で先回り配線

### §3.2 PC-7γ-2 scope **外** (= PC-7γ-3 持越、後段新設)

- **codegen Asset_GLTFNodes / Asset_GLTFMaterials / Skin_GLTFJoints block 追加** (= `scripts/ubo_codegen/` 更新 + `ubo_metadata.inl` 再生成 + cadence_tag = 3 / 4 emit)、もしくは bare OpenGL UBO 用 synthetic block_hash scheme 設計 (= 06b §2.4 想定外で別途設計議論必要)
- **bare OpenGL UBO 置換**: `gltf/asset.cpp:181-187` (updateNodeData) + `:230-236` (updateMaterialData) + `gltf/animation.cpp:455-456` (Skin::updateTransforms) を `LLVKLoader::registerAssetUbo / writeAssetUbo` (asset) / `registerSkinUbo / writeSkinUbo` (skin) 経由置換
- **LL::GLTF::Asset / Skin lifecycle hook 配線**: `Asset::~Asset()` / `Skin::~Skin()` で unregister 呼出、初回 update 時 lazy register、PC-7γ-1 mapUniforms / unloadInternal hook と対称構造
- **flush 関数本格化**: dirty.exchange(false) 後の vkCmdBindDescriptorSets 通電 + memcpy → GPU 経路は PC-7δ scope (= PC-7γ-1 PER_PROGRAM と同 scope 帯)
- **dynamic offset 経路**: per-draw ring buffer chunk hand-off + pDynamicOffsets[4] 配線は PC-7ε scope
- **codegen V1' update**: ubo_metadata.inl の set=1a/1b split 実装 + 130 件 unittest 回帰確認は PC-7α' scope (= Z2-C 持越、PC-7δ 通電前に必要)

### §3.3 PC-7γ-2 Exit Criteria (= 7 項、defensive 配線 only 版)

- **(i)** UboAssetKey + UboAssetKeyHash + UboSkinKey + UboSkinKeyHash 新設、sAssetUboDirty / sSkinUboDirty 型 refactor (= `std::unordered_map<UboAssetKey, UboInstance, UboAssetKeyHash>` / `std::unordered_map<UboSkinKey, UboInstance, UboSkinKeyHash>`)、flushAssetUbos / flushSkinUbos walk 経路 `kv.first.first == owner` match check 追加
- **(ii)** LLVKLoader 6 method 新設 = registerAssetUbo / unregisterAssetUbo / writeAssetUbo + registerSkinUbo / unregisterSkinUbo / writeSkinUbo、PC-7γ-1 4 method と対称 (= try_emplace / allocate / destroy / memcpy + dirty.store(release))、本 sub-step では call site 不在を comment 明文化
- **(iii)** sCurrentAsset / sCurrentSkin static + setCurrentAsset / clearCurrentAsset / setCurrentSkin / clearCurrentSkin 4 method 新設 + 内部 accessor (= getCurrentAsset / getCurrentSkin) 追加、main thread 専有 = atomic 不要
- **(iv)** forwardToUboUpload PER_ASSET / PER_SKIN case 置換 = sCurrentAsset / sCurrentSkin null check + writeAssetUbo / writeSkinUbo 経由 (defensive、現 codegen 0 件で実走しないことを comment 明文化)
- **(v)** gltfscenemanager.cpp set/clear 配線 = line 693 (flushAssetUbos 周辺) + line 736 (flushSkinUbos 周辺) で setCurrentAsset / clearCurrentAsset + setCurrentSkin / clearCurrentSkin 配線、将来 PC-7γ-3 通電時の owner 解決経路を先回り確立
- **(vi)** llrender + newview build PASS + warning 0 + INTEGRATION_TEST_lluboringbuffer 11/11 + INTEGRATION_TEST_llassetubopool 10/10 + INTEGRATION_TEST_llpipelinecachestorage 13/13 + codegen unittest 130/130 全 PASS
- **(vii)** MUSEUBO-A + GATE-B 整合 = MUSEUBO-A: forwardToUboUpload PER_ASSET/PER_SKIN case は呼出側 setter 31 site が `if (mUseUBO)` block 内側で呼出、mUseUBO=false default で本 entry 不到達 + sCurrent* null で defensive return 多重保証 = 既存 OpenGL 描画 100% 維持。GATE-B: 新 `#ifdef LL_VULKAN_GLSL` 追加 0 (= grep `git diff | grep "^\+" | grep LL_VULKAN_GLSL` で 0 件確認)

### §3.4 物量見積もり (= AYA 物量判断材料)

PC-7γ-1 実績 (= 461 行 +、3 file 改変、約 4 時間 session) を baseline に PC-7γ-2 推定:

| ファイル | 推定 +行 | 推定 -行 | 内訳 |
|---|---|---|---|
| `indra/llrender/llvkloader.h` | ~50 | 0 | 6 method 宣言 (= asset 3 + skin 3) + sCurrent 4 method 宣言 (= setCurrentAsset/clearCurrentAsset/setCurrentSkin/clearCurrentSkin) + getCurrentAsset/getCurrentSkin 2 method 宣言 + PC-7γ-2 tag block 6 行 |
| `indra/llrender/llvkloader.cpp` | ~280 | ~10 | UboAssetKey/UboSkinKey + 各 Hash 新設 (~25 行) + sAssetUboDirty/sSkinUboDirty 型 refactor (~5 行) + sCurrentAsset/Skin static (~3 行) + 6 method 定義 (~190 行、PC-7γ-1 4 method 173 行 baseline × 1.5 倍 = 6 method 推定) + setCurrent/clearCurrent/getCurrent 6 method 定義 (~40 行) + flushAssetUbos/flushSkinUbos walk 経路追従 (~15 行) + PC-7γ-2 tag block + comment (~10 行) |
| `indra/llrender/llglslshader.cpp` | ~30 | ~15 | forwardToUboUpload PER_ASSET/PER_SKIN case stub (= LL_WARNS_ONCE 6 行 × 2 = 12 行 stub 削除) → 通電 (= sCurrent null check + writeAssetUbo/Skin 経由 = ~15 行 × 2 = 30 行) + PC-7γ-2 comment 更新 + kCadencePerAsset/kCadencePerSkin comment 更新 (~5 行) |
| `indra/newview/gltfscenemanager.cpp` | ~10 | 0 | 2 site で set/clear 配線 (= asset 1 site + skin 1 site、各 set 1 行 + clear 1 行 + tag block 2 行) |
| **合計** | **~370** | **~25** | **4 file 改変**、PC-7γ-1 (461 行 +、3 file) と同等規模 (= 約 80% volume) |

doc 関連 (commit に含む):
- 本 design-lock handoff doc: 1 new file (= 本 doc、約 380 行)
- PC-7γ-2 complete handoff doc: 1 new file (= 実装完了後、約 350 行 推定)

CMake 改変 0 + settings.xml 改変 0 + 新規 cpp/h file 0 + 既存 file 改変 4 件、PC-7γ-1 (3 modified) より +1 file (= gltfscenemanager.cpp 新規追加)。

session 工数推定: PC-7γ-1 が約 4 時間 (= 必読 + trace + 5 段階 AYA 確認 + 実装 + build + verify + handoff)、PC-7γ-2 は (a) 物量同等 + (b) AYA 確認は本 doc で完了済 = 約 3 時間 (= 必読 + 実装 + build + verify + handoff)、PC-7γ-1 より 25% 短縮見込み。

### §3.5 着手 1 line (= 次 session)

PC-7γ-2 = (1) UboAssetKey + UboSkinKey + 各 Hash 新設 + sAssetUboDirty / sSkinUboDirty 型 refactor → (2) LLVKLoader 6 method 新設 (= asset 3 + skin 3、PC-7γ-1 4 method と対称) → (3) sCurrentAsset/Skin static + setCurrent/clearCurrent/getCurrent 6 method 新設 → (4) forwardToUboUpload PER_ASSET/PER_SKIN case 置換 (= sCurrent null check + writeAssetUbo/Skin 経由 defensive) → (5) gltfscenemanager.cpp set/clear 配線 (= asset 1 site + skin 1 site) → (6) flushAssetUbos/flushSkinUbos walk 経路追従 → build verify + TUT + codegen → self-verify 9 観点 → complete handoff doc 起案 → AYA commit 指示待ち。

## §4 設計詳細 (= 実装時参照)

### §4.1 UboAssetKey / UboSkinKey hash 実装

```cpp
// llvkloader.cpp anonymous ns 内、PC-7γ-1 UboInstanceKey 直後

// <AYAstorm r41 PC-7γ-2> per-asset / per-skin map key 拡張
// (D3-A) 採用: PC-7γ-1 UboInstanceKey と対称構造、UBO physical instance 単位独立性確保
using UboAssetKey = std::pair<LL::GLTF::Asset*, U32 /*block_hash*/>;
using UboSkinKey  = std::pair<LL::GLTF::Skin*,  U32 /*block_hash*/>;

struct UboAssetKeyHash {
    std::size_t operator()(const UboAssetKey& k) const noexcept {
        return std::hash<void*>{}(static_cast<void*>(k.first))
             ^ (std::hash<U32>{}(k.second) << 1);
    }
};

struct UboSkinKeyHash {
    std::size_t operator()(const UboSkinKey& k) const noexcept {
        return std::hash<void*>{}(static_cast<void*>(k.first))
             ^ (std::hash<U32>{}(k.second) << 1);
    }
};
// </AYAstorm r41 PC-7γ-2>
```

### §4.2 sAssetUboDirty / sSkinUboDirty 型 refactor

```cpp
// 旧 (PC-6ε-2)
std::unordered_map<LL::GLTF::Asset*, UboInstance> sAssetUboDirty;
std::unordered_map<LL::GLTF::Skin*, UboInstance>  sSkinUboDirty;

// 新 (PC-7γ-2)
std::unordered_map<UboAssetKey, UboInstance, UboAssetKeyHash> sAssetUboDirty;
std::unordered_map<UboSkinKey, UboInstance, UboSkinKeyHash>   sSkinUboDirty;
```

flushAssetUbos walk 経路追従 (= PC-7γ-1 flushProgramUbos 同形):
```cpp
// 旧 (PC-6ε-2)
auto it = sAssetUboDirty.find(asset);
if (it == sAssetUboDirty.end()) return;

// 新 (PC-7γ-2)
// asset 引数で multi-block 走査、block_hash 別 entry 全件 flush 候補
for (auto& kv : sAssetUboDirty) {
    if (kv.first.first != asset) continue;
    if (!kv.second.dirty.exchange(false, std::memory_order_acq_rel)) continue;
    // (PC-7δ で GPU upload 経路通電予定、本 sub-step では no-op 維持)
}
```

### §4.3 forwardToUboUpload PER_ASSET / PER_SKIN case 通電 (= sCurrent accessor 経由)

```cpp
// llvkloader.h に 2 method 追加 (= internal accessor)
namespace LLVKLoader {
    LL::GLTF::Asset* getCurrentAsset();
    LL::GLTF::Skin*  getCurrentSkin();
}

// llvkloader.cpp に accessor 定義
LL::GLTF::Asset* LLVKLoader::getCurrentAsset() { return sCurrentAsset; }
LL::GLTF::Skin*  LLVKLoader::getCurrentSkin()  { return sCurrentSkin; }

// llglslshader.cpp forwardToUboUpload (= line 2152-2163 置換)
case kCadencePerAsset:
{
    LL::GLTF::Asset* asset = LLVKLoader::getCurrentAsset();
    if (!asset)
    {
        // 現 codegen で kCadencePerAsset block 0 件 (= ubo_metadata.inl 確認済 2026-06-05)
        // ゆえ本 case は通常実走しない。実走時は sCurrentAsset 未設定 = PC-7γ-3 で
        // gltfscenemanager 側 setCurrentAsset 配線済 + GLTF host write 置換完了済の
        // 状態を想定。defensive 早期 return + LL_WARNS_ONCE で診断保留。
        LL_WARNS_ONCE("Vulkan") << "PC-7γ-2: PER_ASSET forwardToUboUpload — sCurrentAsset is nullptr, "
                                << "GLTF host write 置換 / codegen Asset_* block emit 未完 (PC-7γ-3 scope), "
                                << "block_hash=0x" << std::hex << loc.block_hash << std::dec << LL_ENDL;
        return;
    }
    LLVKLoader::writeAssetUbo(asset, loc.block_hash, loc.offset, data, size);
    break;
}
case kCadencePerSkin:
{
    LL::GLTF::Skin* skin = LLVKLoader::getCurrentSkin();
    if (!skin)
    {
        LL_WARNS_ONCE("Vulkan") << "PC-7γ-2: PER_SKIN forwardToUboUpload — sCurrentSkin is nullptr, "
                                << "GLTF host write 置換 / codegen Skin_* block emit 未完 (PC-7γ-3 scope), "
                                << "block_hash=0x" << std::hex << loc.block_hash << std::dec << LL_ENDL;
        return;
    }
    LLVKLoader::writeSkinUbo(skin, loc.block_hash, loc.offset, data, size);
    break;
}
```

### §4.4 6 method 雛形 (= PC-7γ-1 4 method 対称展開)

```cpp
// registerAssetUbo (= PC-7γ-1 registerProgramUbo 対称)
bool LLVKLoader::registerAssetUbo(LL::GLTF::Asset* asset, U32 block_hash, U32 block_size)
{
    if (!asset || block_size == 0) return false;
    UboAssetKey key{ asset, block_hash };
    auto [it, inserted] = sAssetUboDirty.try_emplace(key);
    if (!inserted) return true; // idempotent
    if (!allocateUboInstanceBuffers(it->second, block_size, "PER_ASSET"))
    {
        sAssetUboDirty.erase(it);
        return false;
    }
    return true;
}

void LLVKLoader::unregisterAssetUbo(LL::GLTF::Asset* asset, U32 block_hash)
{
    if (!asset) return;
    UboAssetKey key{ asset, block_hash };
    auto it = sAssetUboDirty.find(key);
    if (it == sAssetUboDirty.end()) return;
    destroyUboInstanceBuffers(it->second);
    sAssetUboDirty.erase(it);
}

void LLVKLoader::writeAssetUbo(LL::GLTF::Asset* asset, U32 block_hash,
                                U32 offset, const void* data, size_t size)
{
    if (!asset) return;
    UboAssetKey key{ asset, block_hash };
    auto it = sAssetUboDirty.find(key);
    if (it == sAssetUboDirty.end())
    {
        LL_WARNS_ONCE("Vulkan") << "PC-7γ-2 writeAssetUbo: not registered, "
                                << "block_hash=0x" << std::hex << block_hash << std::dec << LL_ENDL;
        return;
    }
    UboInstance& ubo = it->second;
    if (ubo.size == 0 || !ubo.mapped_ptr[sFrameIndex]) return;
    if (offset + size > ubo.size)
    {
        LL_WARNS_ONCE("Vulkan") << "PC-7γ-2 writeAssetUbo: out of range, "
                                << "offset=" << offset << " size=" << size
                                << " ubo.size=" << ubo.size << LL_ENDL;
        return;
    }
    std::memcpy(static_cast<U8*>(ubo.mapped_ptr[sFrameIndex]) + offset, data, size);
    ubo.dirty.store(true, std::memory_order_release);
}

// registerSkinUbo / unregisterSkinUbo / writeSkinUbo は asset 版と完全対称
// (= Asset → Skin / sAssetUboDirty → sSkinUboDirty / UboAssetKey → UboSkinKey)
```

### §4.5 sCurrentAsset / sCurrentSkin 配置

```cpp
// llvkloader.cpp anonymous ns 内 (= sFrameUboInstances 直後)

// <AYAstorm r41 PC-7γ-2> per-asset / per-skin current owner tracking
// (D2-A) 採用: design 06b §5.4.1 main thread 専有 ゆえ atomic 不要、static で thread-safe。
// forwardToUboUpload PER_ASSET / PER_SKIN case が "current owner" を解決する経路。
// gltfscenemanager.cpp:693 / 736 で set/clear 配線、forwardToUboUpload 内側で
// LLVKLoader::getCurrentAsset() / getCurrentSkin() 経由 accessor で読出。
LL::GLTF::Asset* sCurrentAsset = nullptr;
LL::GLTF::Skin*  sCurrentSkin  = nullptr;
// </AYAstorm r41 PC-7γ-2>
```

### §4.6 gltfscenemanager.cpp set/clear 配線位置

```cpp
// gltfscenemanager.cpp、line 693 周辺 (= flushAssetUbos 配線位置)
// <AYAstorm r41 PC-7γ-2> per-asset cadence current owner set
LLVKLoader::setCurrentAsset(&asset);
// </AYAstorm r41 PC-7γ-2>

LLVKLoader::flushAssetUbos(&asset);

if (asset.mNodesUBO != 0)
{
    glBindBufferBase(GL_UNIFORM_BUFFER, LLGLSLShader::UB_GLTF_NODES, asset.mNodesUBO);
}
glBindBufferBase(GL_UNIFORM_BUFFER, LLGLSLShader::UB_GLTF_MATERIALS, asset.mMaterialsUBO);

// (asset draw 群実行)
// ...

// <AYAstorm r41 PC-7γ-2> per-asset cadence current owner clear
LLVKLoader::clearCurrentAsset();
// </AYAstorm r41 PC-7γ-2>
```

skin 側も対称配線 (= line 736 / 743-746 周辺、setCurrentSkin → flushSkinUbos → glBindBufferBase → draw → clearCurrentSkin)。

注: 実 line 番号は実装着手時に再確認 (= context 圧迫で差異あり得る)、本 doc は schematic position として記録。

## §5 self-verify 観点 (= 実装完了後 9 観点 self-check)

1. **Exit Criteria 7 項全充足** (= §3.3 literal check)
2. **source doc 整合** = design 06b §2.4 (per-asset) + §2.5 (per-skin) + §5.2 (forwardToUboUpload routing) + §5.4 (thread 配線) + design 06a §5.4 (setter mUseUBO 分岐) literal 引用、PC-7γ-1 UboInstanceKey 対称構造
3. **AYA 採用根拠 record** = §2.2 で (D1-A)〜(D5-rev) + 段階分割 + handoff 起案、全件 literal「OK」/「分ける推奨」/「handoff 作成で段階実装」(2026-06-05) record 完了済
4. **GATE-B 整合** = 新 `#ifdef LL_VULKAN_GLSL` 追加 0 件 (= grep 確認)、本 sub-step も Vulkan init 層 + mUseUBO runtime gate 単独
5. **MUSEUBO-A 整合** = forwardToUboUpload PER_ASSET/PER_SKIN case は呼出側 setter 31 site が `if (mUseUBO)` block 内側、mUseUBO=false default で本 entry 不到達 + sCurrent* null で defensive return 多重保証
6. **build + TUT + codegen 全 PASS** = llrender + newview build + TUT 3 件 (11/11 + 10/10 + 13/13) + codegen 130/130 全 PASS literal 検証取得
7. **commit 内容** = 4 modified (= llvkloader.h + llvkloader.cpp + llglslshader.cpp + gltfscenemanager.cpp) + 2 new doc (= 本 design-lock + complete handoff) + 新 file 0 + CMake 改変 0 + settings.xml 改変 0 + Co-Authored-By 不在
8. **feedback_no_scope_shrink 遵守** = PC-7γ-2 literal scope (= 5 item) 完全実施、PC-7γ-3 分離は (D5-rev) AYA 確認済 = scope 縮小ではなく構造的事実 (= block_hash 未生成で本格化不能) による分割
9. **feedback_design_phase_no_code_write 整合** = 本 sub-step は実装 phase (= 本 design-lock doc 起案後の commit 直前で AYA 指示)、indra/ 改変 4 件 = 設計 phase ではない

## §6 残 strict 線形

PC-7γ-1 = ✅ commit c1f2e2d53a (per-program 専念)
→ **PC-7γ-2 = ⏳ 本 design-lock + 次 session 実装** (per-asset / per-skin defensive 配線)
→ PC-7γ-3 = ⏳ per-asset / per-skin 本格化 (= codegen Asset_*/Skin_* block 追加 or synthetic ID scheme + bare OpenGL UBO 置換 + GLTF lifecycle hook 配線、source doc = 06b §2.4 + §2.5、codegen update は PC-7α' と同類)
→ PC-7δ = vkCmdBindDescriptorSets 通電 + set=3 swap + sAYAStandardLayout 経由 bind + flush 側 dirty.exchange + GPU upload 経路 (= PER_PROGRAM + PER_ASSET + PER_SKIN 同 scope 帯)
→ PC-7ε = dynamic offset 経路 ring buffer chunk hand-off (= PER_DRAW、sDrawUboRingBufferMgr 経由)
→ PC-7α' = codegen ubo_metadata.inl V1' update (= set=1a/1b split、Z2-C 持越、PC-7δ 通電前に必要)
→ PC-8 = 3 OS build verify (= Linux primary + Win/Mac 後段)
→ PC-N = Phase 1.C complete marker + Phase 1.D / Phase 2 着手起点

## §7 r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅
+ PC-0 ✅ + PC-1 ✅ + PC-2 ✅ + PC-3 ✅ + PC-4 ✅ + PC-5 ✅ + PC-6α ✅ + PC-6β ✅
+ PC-6γ ✅ + PC-6δ ✅ + PC-6δ' ✅ + PC-6ε-1 ✅ + PC-6ε-2 ✅ + PC-6ε-3 ✅ + PC-6ζ ✅
+ PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅
+ **PC-7γ-2 ⏳ 本 design-lock (次 session 実装)**
+ PC-7γ-3..PC-N ⏳ 後段

## §8 引き継ぎ feedback rules 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 design-lock handoff 起案で PC-7γ-2 段階分割 pin
- **feedback_handoff_minimal_pre_req_read** 遵守 = 必読 3 件 (= 本 doc + PC-7γ-1 complete + design 06b §2.4/§2.5/§5.2 + 06a §5.4) + pinpoint reference 別記、全箇条書きリスト化なし
- **feedback_self_verify_before_handoff** 遵守 = 本 §3.3 Exit Criteria 7 項 + §5 self-verify 9 観点 で実装着手前に整理完了
- **feedback_build_only_verified** 遵守 = 本 sub-step は実装着手前 design-lock ゆえ build verify 持越、次 session 実装完了後に llrender + newview build + TUT + codegen で literal 検証取得予定
- **feedback_no_scope_shrink** 遵守 = §2.3 (D5-rev) で構造的事実 (= block_hash 未生成) による分割根拠を AYA 確認済、scope 縮小ではない
- **feedback_doubt_self_first** 遵守 = (D1)〜(D5) 5 件 ambiguity + (D5-rev) 再分析発見で停止 + grep 確認 + 推奨案提示 + AYA 確認 + literal「OK」「分ける推奨」「handoff 作成で段階実装」受領後、本 design-lock 起案
- **feedback_confirm_referent_before_acting** 遵守 = (D1)〜(D5) 1 batch AYA 確認 + (D5-rev) 段階分割 2nd 確認 + 段階分割 + handoff 起案 3rd 確認、推測実装なし
- **feedback_ubo_migration_one_at_a_time** 遵守 = PC-7γ-2 = per-asset / per-skin defensive 配線単独実施、本格化 (= GLTF host write 置換 + lifecycle hook + codegen 追加) は PC-7γ-3 へ分離、bind 通電は PC-7δ、dynamic offset は PC-7ε へ分離
- **feedback_design_phase_no_code_write** 整合 = 本 sub-step は **実装着手前** design-lock = indra/ 改変 0 件、本 doc 起案のみ
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上で次 session 実装予定
- **feedback_no_auto_commit** 遵守 = 本 doc 起案後 AYA literal commit 指示待ち
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在

## §9 着手 1 line (= 次 session)

次 session で /clear → 本 doc + PC-7γ-1 complete handoff + design 06b §2.4/§2.5/§5.2 + design 06a §5.4 を Read → PC-7γ-2 = (1) UboAssetKey + UboSkinKey + Hash 新設 + map refactor → (2) LLVKLoader 6 method 新設 → (3) sCurrent static + 6 method 新設 → (4) forwardToUboUpload PER_ASSET/PER_SKIN case 置換 → (5) gltfscenemanager set/clear 配線 → build verify + TUT + codegen → self-verify → complete handoff 起案 → AYA commit 指示待ち、の順で実装着手。
