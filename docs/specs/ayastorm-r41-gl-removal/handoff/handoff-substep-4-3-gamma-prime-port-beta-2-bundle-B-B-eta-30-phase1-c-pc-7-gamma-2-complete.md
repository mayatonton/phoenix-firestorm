# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-7γ-2 complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-7γ-2 (per-asset / per-skin cadence **defensive 配線**) 完了 marker + PC-7γ-3 (本格化) 引継

---

## §0. 必読 3 件 (次 session 着手前)

1. **本 handoff doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-gamma-2-complete.md`
2. **PC-7γ-2 design-lock handoff doc**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-gamma-2-design-lock.md`
   - PC-7γ-3 scope (= codegen Asset_*/Skin_* block 追加 + bare OpenGL UBO 置換 + GLTF lifecycle hook) の設計記録は本 doc 含む
3. **design 06b §2.4 + §2.5 + §5.2 + §5.3 + §5.4** + **design 06a §5.4 (mUseUBO flag placement)**
   - pinpoint reference 別記:
     - `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md` §2.4 (per-asset cadence update site) + §2.5 (per-skin) + §5.2 (forwardToUboUpload routing) + §5.3 (per-program/asset/skin register hook) + §5.4 (thread wiring)
     - `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` §5.4 (mUseUBO flag placement)

---

## §1. PC-7γ-2 完了内容 = per-asset / per-skin **defensive 配線**

### §1.0 scope = AYA 確認 5 件実装 (= design-lock doc §3.3 Exit Criteria 7 項全充足)

PC-7γ-2 **literal scope** (= AYA 「PC-7γ-2 着手お願いします」literal 受領 2026-06-05):

1. **UboAssetKey / UboSkinKey + Hash 新設** + sAssetUboDirty / sSkinUboDirty map refactor (= UboInstanceKey と同形)
2. **LLVKLoader 6 method 新設** = registerAssetUbo / unregisterAssetUbo / writeAssetUbo + Skin 3 件
3. **sCurrentAsset / sCurrentSkin static + 6 method** = setCurrentAsset / clearCurrentAsset / getCurrentAsset + Skin 3 件
4. **forwardToUboUpload PER_ASSET / PER_SKIN case stub → defensive 通電** (= sCurrent* lookup + write* method 呼出)
5. **gltfscenemanager.cpp set/clear 配線** (= setCurrentAsset/Skin で current owner 確立 + 対称 clearCurrentAsset/Skin)

**本 PC-7γ-2 scope 外 (= PC-7γ-3 scope)**:
- codegen ubo_metadata.inl Asset_* / Skin_* block 追加 (= 現 codegen PER_ASSET=0 / PER_SKIN=0 件)
- bare OpenGL UBO bind (mNodesUBO / mMaterialsUBO / skin.mUBO) → cadence UBO 置換
- GLTF asset/skin lifecycle hook (= 構築時 registerAssetUbo / 破棄時 unregisterAssetUbo)
- mUseUBO=true 時の hot path 通電 (= 現 mUseUBO=false default で本 PC-7γ-2 defensive 配線は inactive)

### §1.1 採用根拠 5 件 (= AYA 確認 record 2026-06-05)

(D1-A) UboAssetKey / UboSkinKey 型 = std::pair<GLTF::Asset*, U32> / std::pair<GLTF::Skin*, U32>:
- 根拠 = UboInstanceKey (PC-7γ-1 で std::pair<LLGLSLShader*, U32> 採用) と完全同形、Hash struct も同形 (hash<void*>(first) ^ hash<U32>(second) << 1)、AYA literal「OK」(2026-06-05)

(D2-A) sCurrentAsset / sCurrentSkin static placement = LLVKLoader anonymous ns 内 (= sFrameUboInstances 後):
- 根拠 = main thread 専有 (= GLTFSceneManager::render() は main thread 単独呼出) → atomic 不要、std::atomic 不採用で簡素化、AYA literal「OK」(2026-06-05)

(D3-A) writeAssetUbo / writeSkinUbo 内部実装 = writeFrameUbo / writeProgramUbo と同形:
- 根拠 = bounds check (offset + size <= ubo.size) + std::memcpy(ubo.mapped_ptr[sFrameIndex], data, size) + ubo.dirty.store(true, release)、PC-7γ-1 4 method と一貫した実装、AYA literal「OK」(2026-06-05)

(D4-A) forwardToUboUpload PER_ASSET / PER_SKIN case = sCurrent* lookup + write* method 呼出:
- 根拠 = getCurrentAsset / getCurrentSkin == nullptr 時 LL_WARNS_ONCE + defensive return、非 nullptr 時 writeAssetUbo / writeSkinUbo 呼出、PC-7γ-1 PER_PROGRAM case と一貫した routing、AYA literal「OK」(2026-06-05)

(D5-rev) gltfscenemanager.cpp set/clear 配線位置 = 非対称 (asset = ds loop scope、skin = primitive loop scope):
- 根拠 = asset は `if (!shader_bound)` 内で 1 度 set (= ds loop body 内全 primitive で共有)、ds loop 終了直前で clear / skin は `if (rigged)` 内で per-primitive set、drawRangeFast 直後で unconditional clear (= setCurrentSkin 未呼出時は nullptr → nullptr の no-op)、AYA literal「OK」(2026-06-05)

### §1.2 編集 1 (llvkloader.h +53 / -0)

PC-7γ-1 tag block 直後に PC-7γ-2 tag block 追加。LLVKLoader namespace 内 12 method 宣言:

- `bool registerAssetUbo(LL::GLTF::Asset*, U32 block_hash, U32 block_size);`
- `void unregisterAssetUbo(LL::GLTF::Asset*, U32 block_hash);`
- `void writeAssetUbo(LL::GLTF::Asset*, U32 block_hash, U32 offset, const void* data, size_t size);`
- `bool registerSkinUbo(LL::GLTF::Skin*, U32 block_hash, U32 block_size);`
- `void unregisterSkinUbo(LL::GLTF::Skin*, U32 block_hash);`
- `void writeSkinUbo(LL::GLTF::Skin*, U32 block_hash, U32 offset, const void* data, size_t size);`
- `void setCurrentAsset(LL::GLTF::Asset*);`
- `void clearCurrentAsset();`
- `LL::GLTF::Asset* getCurrentAsset();`
- `void setCurrentSkin(LL::GLTF::Skin*);`
- `void clearCurrentSkin();`
- `LL::GLTF::Skin* getCurrentSkin();`

### §1.3 編集 2 (llvkloader.cpp +295 / -X)

- **§1.3.1 forward decl** = `namespace LL { namespace GLTF { class Asset; class Skin; } }` (= include header bloat 回避)
- **§1.3.2 PC-7γ-2 anonymous ns block (= PC-7γ-1 UboInstanceKey 直後)** = UboAssetKey / UboSkinKey type alias + UboAssetKeyHash / UboSkinKeyHash struct + sAssetUboDirty / sSkinUboDirty 型変更 (= PC-6ε-2 旧 map → PC-7γ-2 新 map) + sCurrentAsset / sCurrentSkin static (= main thread 専有、atomic 不要)
- **§1.3.3 flushAssetUbos / flushSkinUbos walk refactor** = PC-6ε-2 旧 (find(asset) 単一 key 探索) → PC-7γ-2 新 (walk pattern = `for (auto& kv : sAssetUboDirty) { if (kv.first.first != asset) continue; if (kv.second.dirty.exchange(false, acq_rel)) any_dirty = true; }`) で multi-block 対応
- **§1.3.4 6 cadence method 定義** (= PC-7γ-1 4 method 直後):
  - registerAssetUbo (= try_emplace(UboAssetKey, UboInstance{}) + allocateUboInstanceBuffers + failure 時 erase + return false)
  - unregisterAssetUbo (= find + destroyUboInstanceBuffers + erase)
  - writeAssetUbo (= find + bounds check + memcpy + dirty.store)
  - registerSkinUbo / unregisterSkinUbo / writeSkinUbo 同形 (Skin 用)
- **§1.3.5 6 sCurrent accessor 定義** = setCurrentAsset / clearCurrentAsset / getCurrentAsset + Skin 3 件 (= 単純 sCurrent* = arg / nullptr / return sCurrent*)
- **§1.3.6 shutdownVulkan teardown 追記** = sAssetUboDirty walk + destroyUboInstanceBuffers 呼出 + sCurrentAsset / sCurrentSkin nullptr リセット (= PC-7γ-1 sFrameUboInstances teardown 同形)

### §1.4 編集 3 (llglslshader.cpp +58 / -X)

PC-7γ-1 forwardToUboUpload 9-case switch 内 PER_ASSET / PER_SKIN case 置換:

旧 (PC-7γ-1、LL_WARNS_ONCE "PC-7γ-2 scope" stub):
```cpp
case kCadencePerAsset:
case kCadencePerSkin:
    LL_WARNS_ONCE("Vulkan") << "PC-7γ-1: PER_ASSET / PER_SKIN forward (PC-7γ-2 scope)" << LL_ENDL;
    return;
```

新 (PC-7γ-2、defensive 通電):
```cpp
case kCadencePerAsset:
{
    LL::GLTF::Asset* asset = LLVKLoader::getCurrentAsset();
    if (!asset) {
        LL_WARNS_ONCE("Vulkan") << "PC-7γ-2: PER_ASSET forwardToUboUpload — sCurrentAsset is nullptr ..." << LL_ENDL;
        return;
    }
    LLVKLoader::writeAssetUbo(asset, loc.block_hash, loc.offset, data, size);
    return;
}
case kCadencePerSkin: { /* same pattern */ }
```

+ forwardToUboUpload docstring 更新 (= "PC-7γ-2: PER_ASSET / PER_SKIN 通電完了 (defensive 配線)")
+ kCadencePerAsset / kCadencePerSkin constexpr 上 comment 更新

### §1.5 編集 4 (gltfscenemanager.cpp +25 / -0)

`GLTFSceneManager::render(Asset&, U8 variant)` (line 640-) 内 4 site 配線:

- **(a) setCurrentAsset**: `if (!shader_bound)` block 内、bindDeferredShader 直後 + flushAssetUbos 直前 (= line ~697)
- **(b) clearCurrentAsset**: ds loop (`for (U32 ds = 0; ds < 2; ++ds)`) 終了直前 (= line ~771)
- **(c) setCurrentSkin**: `if (rigged)` block 内、flushSkinUbos 直前 (= line ~752、per-primitive)
- **(d) clearCurrentSkin**: drawRangeFast 直後 unconditional (= line ~770、per-primitive、setCurrentSkin 未呼出時は nullptr → nullptr no-op)

各 site 統一 tag block (= `// <AYAstorm r41 PC-7γ-2>` ... `// </AYAstorm r41 PC-7γ-2>`) + 設計意図 comment (= D2-A 確認 record + defensive 配線 only + PC-7γ-3 hot path 通電予定)

---

## §3. build verify

### §3.1 llrender build

```
make -j4 llrender
```

- libllrender.a link PASS
- ERROR 0 / WARNING 0 (= PC-7γ-2 改変関連)
- llvkloader.cpp.o + llglslshader.cpp.o compile PASS

### §3.2 newview gltfscenemanager.cpp build

```
touch ../indra/newview/gltfscenemanager.cpp
make -j4 -f newview/CMakeFiles/ayastorm-bin.dir/build.make \
  newview/CMakeFiles/ayastorm-bin.dir/gltfscenemanager.cpp.o
```

- gltfscenemanager.cpp.o compile PASS
- ERROR 0 / WARNING 0

注 = ayastorm-bin 全リンクは llui/tests/llurlmatch_test.cpp の pre-existing build error (= 本 PC-7γ-2 と無関係、git log で 92283a5133 以前から存在) で失敗するが、本 PC-7γ-2 改変 file (= llrender 3 件 + newview gltfscenemanager.cpp) は全件 .o compile PASS で確認済。PC-6ε-3 と同形の per-TU rebuild verification precedent 整合。

### §3.3 TUT 3 件

```
INTEGRATION_TEST_lluboringbuffer:    11 / 11 PASS YAY \o/
INTEGRATION_TEST_llassetubopool:     10 / 10 PASS YAY \o/
INTEGRATION_TEST_llpipelinecachestorage: 13 / 13 PASS YAY \o/
```

= PC-3 / PC-4 / PC-5 algorithm 層 regression なし

### §3.4 codegen unittest

```
python3 -m unittest discover -s scripts/ubo_codegen/tests
Ran 130 tests in 0.073s OK
```

= 130/130 PASS = Phase 1.A / 1.B / 1.C PC-1..PC-7γ-1 regression なし

---

## §4. PC-7γ-2 Exit Criteria 7 項全充足

| # | Exit Criteria | 充足状況 |
|---|--------------|---------|
| (i) | UboAssetKey / UboSkinKey + Hash 新設 + map refactor | ✅ §1.3.2 |
| (ii) | LLVKLoader 6 method 新設 (Asset 3 + Skin 3) | ✅ §1.3.4 (llvkloader.h §1.2 / llvkloader.cpp §1.3.4) |
| (iii) | sCurrentAsset / sCurrentSkin static + 6 accessor 新設 | ✅ §1.3.5 (llvkloader.h §1.2 / llvkloader.cpp §1.3.2 + §1.3.5) |
| (iv) | forwardToUboUpload PER_ASSET / PER_SKIN case 置換 | ✅ §1.4 |
| (v) | gltfscenemanager.cpp 4 site 配線 (set/clear 対称) | ✅ §1.5 |
| (vi) | GATE-B 整合 (= #ifdef LL_VULKAN_GLSL 新規追加 0) | ✅ §7 (5) |
| (vii) | MUSEUBO-A 整合 + llrender + gltfscenemanager build + TUT 3 件 + codegen 130/130 全 PASS | ✅ §3 |

---

## §5. 残 strict 線形

- **PC-7γ-3** = per-asset / per-skin **本格化** (= codegen Asset_* / Skin_* block 追加 + bare OpenGL UBO (mNodesUBO / mMaterialsUBO / skin.mUBO) → cadence UBO 置換 + GLTF asset/skin lifecycle hook 配線 = 構築時 registerAssetUbo + 破棄時 unregisterAssetUbo) + (= source doc = 06b §2.4 + §2.5 + §5.3 + §5.4 + 06a §5.4 + codegen main.py + ubo_metadata.inl)
- **PC-7δ** = vkCmdBindDescriptorSets 通電 + set=3 swap + sAYAStandardLayout 経由 bind = 既存 placeholder bind path から V3a layout へ移行 + SINGLETON case llassert_always → flushSingletonUbos 経由 bind 通電
- **PC-7ε** = dynamic offset 経路 ring buffer chunk hand-off = set=2 per-draw を sDrawUboRingBufferMgr 経由 dynamic offset 計算 + pDynamicOffsets[4] 配線
- **PC-7α'** = codegen ubo_metadata.inl V1' update = scripts/ubo_codegen/main.py で set=1a/1b split 実装 + ubo_metadata.inl 再生成 + 130 件 unittest 回帰確認、Z2-C 持越、PC-7δ 通電前に必要
- **PC-8** = 3 OS build verify、Linux primary + Win/Mac 後段
- **PC-N** = Phase 1.C complete marker + Phase 1.D / Phase 2 着手起点

---

## §6. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + **PC-7γ-2 ✅ 本 commit** + PC-7γ-3..PC-N ⏳ 次 session

---

## §7. self-verify 9 観点 全 ✅

1. **Exit Criteria 7 項全充足** = §4 確認
2. **UboAssetKey / UboSkinKey source doc 整合** = UboInstanceKey (PC-7γ-1) と完全同形、design 06b §3.2.3 (UboInstance struct 拡張形) 整合
3. **(D1-A)(D2-A)(D3-A)(D4-A)(D5-rev) 採用根拠 record** = §1.1 で 5 件確認 + 採用根拠 5 件明文化、AYA literal「OK」(2026-06-05) record
4. **GATE-B 整合** = #ifdef LL_VULKAN_GLSL 新規追加 0、本 PC-7γ-2 改変は全て mUseUBO runtime gate (= setter 内 mUseUBO 分岐 + forwardToUboUpload 経路) 配下、scaffolding (= sAssetUboDirty / sSkinUboDirty map + sCurrent* static) は Vulkan init 層単独動作
5. **MUSEUBO-A 整合** = mUseUBO=false default で setter 内 forwardToUboUpload 不呼出 → PER_ASSET / PER_SKIN case 未到達 + sCurrentAsset / sCurrentSkin 設定経路 (= GLTFSceneManager::render() 内 setCurrentAsset / setCurrentSkin) は mUseUBO 値に依存せず動作するが、forwardToUboUpload が不呼出ゆえ writeAssetUbo / writeSkinUbo も不呼出 → sAssetUboDirty / sSkinUboDirty map insert 0 件 → flushAssetUbos / flushSkinUbos walk 空、既存 OpenGL 描画 100% 維持
6. **llrender + gltfscenemanager.cpp build + TUT 11+10+13 + codegen 130/130 全 PASS** = §3 確認
7. **commit 内容** = 4 modified (llglslshader.cpp + llvkloader.cpp + llvkloader.h + gltfscenemanager.cpp) + 2 new doc (handoff design-lock + complete) + 新 file 0 + CMake 改変 0 + settings.xml 改変 0 + Co-Authored-By 不在
8. **feedback_no_scope_shrink 遵守** = PC-7γ-2 literal scope (= per-asset / per-skin defensive 配線 = 5 件実装事項) 完全実施、本格化 (= codegen 追加 + bare OpenGL UBO 置換 + lifecycle hook) の PC-7γ-3 分離は AYA design-lock 確認時に明文化済 (= scope 縮小ではない、UBO migration one-at-a-time 規律遵守)
9. **feedback_design_phase_no_code_write 整合** = 本 PC-7γ-2 は実装 phase (= design-lock doc commit 後の実装 phase)、indra/ 改変 4 件 = 設計 phase ではない

---

## §8. 引き継ぎ memory 14 件

(既存 memory pointer 列挙、変更なし)

1. project_ayastorm_r41_vulkan_migration
2. project_r41_phase1b_vulkan_host_gate (GATE-B 確定)
3. project_ayastorm_r41_design_principles (2 大原則)
4. feedback_ubo_migration_one_at_a_time (1 つずつ実施)
5. feedback_proactive_handoff (引継 marker 起案)
6. feedback_handoff_minimal_pre_req_read (必読 3 件)
7. feedback_self_verify_before_handoff (9 観点)
8. feedback_build_only_verified (literal 検証取得)
9. feedback_no_scope_shrink (literal scope 完遂)
10. feedback_doubt_self_first (ambiguity 停止 + 確認)
11. feedback_confirm_referent_before_acting (batch AYA 確認)
12. feedback_design_phase_no_code_write (実装 phase 整合)
13. feedback_release_branch_workflow (feature branch 上 commit)
14. feedback_no_auto_commit (AYA 明示指示後 commit) + feedback_no_claude_coauthor

---

## §9. 次 session 着手 1 line

**PC-7γ-3** = per-asset / per-skin **本格化** = codegen Asset_* / Skin_* block 追加 (= scripts/ubo_codegen/main.py 拡張 + ubo_metadata.inl 再生成 + 130 件 unittest 回帰確認) + bare OpenGL UBO (= asset.mNodesUBO / asset.mMaterialsUBO / skin.mUBO の glBindBufferBase 経路) → cadence UBO 置換 + GLTF asset/skin lifecycle hook 配線 (= GLTFSceneManager 構築時 registerAssetUbo / 破棄時 unregisterAssetUbo)、Exit Criteria は次 session 着手前整理

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = PC-7γ-3 引継 marker 本 handoff
- **feedback_handoff_minimal_pre_req_read** 遵守 = 必読 3 件 + pinpoint reference 別記
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅
- **feedback_build_only_verified** 遵守 = llrender build + gltfscenemanager TU rebuild + TUT 11/11 + 10/10 + 13/13 + codegen 130/130 で literal 検証取得
- **feedback_no_scope_shrink** 遵守 = literal scope 完遂、PC-7γ-3 分離は design-lock 時確認済
- **feedback_doubt_self_first** 遵守 = (D1)(D2)(D3)(D4)(D5) 5 件 ambiguity 発見で停止 + 推奨案提示 + AYA 確認 + literal「OK」受領後実装 (= design-lock phase)
- **feedback_confirm_referent_before_acting** 遵守 = 5 件 batch AYA 確認、推測実装なし
- **feedback_ubo_migration_one_at_a_time** 遵守 = PC-7γ-2 = per-asset / per-skin defensive 配線単独実施、本格化 (codegen 追加 + bare OpenGL UBO 置換 + lifecycle hook) は PC-7γ-3 へ分離
- **feedback_design_phase_no_code_write** 整合 = 本 PC-7γ-2 は実装 phase、indra/ 改変 4 件
- **feedback_release_branch_workflow** 遵守 = feature branch feature/ayastorm-r41-gl-removal 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「commit してください」literal 受領後 commit (= 本 handoff 起案後待ち)
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
