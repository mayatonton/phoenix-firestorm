---
title: r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-7γ-1 design-lock + 実装着手前 pin
date: 2026-06-05
status: design-locked (PC-7γ-1) / pending implementation
parent_handoff: handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-beta.md
---

# PC-7γ-1 design-lock + 実装着手前 pin

## §0 本 handoff doc の位置付け

本 doc は **実装着手前** の design-lock handoff である (= 編集 0 件 / commit なし)。前 session 末で context 圧迫 → /clear → summary 復元の経緯があり、本 session で再確定した PC-7γ scope refinement + (W1)..(W8) 8 件 AYA 確認 record + Exit Criteria 9 項 (refinement 後の per-program 1 件版) を、次 session で /clear 直後に参照可能な形で pin することが目的。

実装着手は本 doc 起案後の **次 session** で /clear → 本 doc + 必読 3 件 Read → コード編集着手、の順とする (= AYA 2026-06-05 指示「handoff doc 起案 → commit せず (= まだ実装 0 件) → 次 session で /clear → handoff doc 経由実装着手」literal 整合)。

## §1 必読 (= 次 session 実装着手前 minimum 3 件)

1. **本 doc 全文** (= PC-7γ-1 scope refinement + (W1)..(W8) AYA 確認 record + Exit Criteria 9 項 + 設計詳細)
2. **PC-7β complete handoff** = `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-beta.md` 全文 (= PC-7β 実装内容 + UboInstance struct + allocate/destroy helper 配置 line + shutdownVulkan 配線)
3. **design/06b-cadence-update-site-and-dirty.md §3.2 + §3.2.3 + §5.2 + §5.4** (= 二段階 dedup 構造 + UboInstance struct 完成形 + forwardToUboUpload routing 5 case 分岐 + thread-safe 要件)

**pinpoint reference** (= 必要時 Read、context 節約のため初手では読まない):
- `indra/llrender/llglslshader.cpp`:
  - line 370-430 (`~LLGLSLShader()` / `unload()` / `unloadInternal()` = per-program unregister hook 候補)
  - line 1734-1910+ (`mapUniforms()` = per-program register hook 候補、`mUniformUBOLoc` 構築 site)
  - line 2009-2011 (forwardToUboUpload empty stub = PC-7γ-1 で本格化対象)
  - line 2061 (PC-1 contract `llassert(block->cadence_tag == 5u)` = SINGLETON 不変)
  - line 2363-3534 (setter 31 site、mUseUBO 分岐内 forwardToUboUpload 呼出予定)
- `indra/llrender/llvkloader.cpp`:
  - line 113 (FRAMES_IN_FLIGHT=3)
  - line 123-135 (sPerFrameUboBuffer = Phase 1.A 既存、2 binding 旧 layout)
  - line 444-481 (PC-7β UboInstance struct)
  - line 479-481 (PC-6ε-2 sProgramUboDirty / sAssetUboDirty / sSkinUboDirty、PC-7γ-1 で sProgramUboDirty のみ key refactor)
  - line 1248-1346 (allocateUboInstanceBuffers + destroyUboInstanceBuffers helper、`[[maybe_unused]]` 付与)
  - line 3345-3361 (shutdownVulkan PC-7β teardown 配線)
  - line 3761-3828 (PC-6ε-2 flushProgramUbos / flushAssetUbos / flushSkinUbos)
- `build-linux-x86_64/codegen/ubo/ubo_metadata.inl` (= 91 block、`block_hash` / `block_size` / `cadence_tag` field 参照、per-asset (cadence_tag=3) / per-skin (cadence_tag=4) 0 件であることに注意)
- `scripts/ubo_codegen/main.py` line 58-63 (= CADENCE_* enum 定義、PER_FRAME=0 / PER_PROGRAM=1 / PER_DRAW=2 / PER_ASSET=3 / PER_SKIN=4 / SINGLETON=5)

## §2 起案契機 + AYA 確認 record

### §2.1 起案契機

PC-7β commit (4ef7ab2aab、2026-06-05) 後、AYA literal「PC-7γ 着手お願いします」受領 → 必読 3 件 Read (= PC-7β handoff 全文 + design 06b §3.2 + design 06a §5.4) + Agent 経由 codebase trace 10 件並列取得 → PC-7γ 着手前 ambiguity 8 件浮上 → 5 段階 AYA 確認 (literal「OK」5 回) で全件 lock。

前 session が context 圧迫で /clear → summary 復元の経緯あり、決定情報が現 conversation context のみに存在する状態 = 実装着手前に本 design-lock handoff を起案して pin、次 session で /clear 後の参照可能性を確保する。

### §2.2 AYA 判断確定 record (= 全 8 件)

| ID | 確認内容 | AYA 採用 | literal record |
|---|---|---|---|
| (W1-A) | per-owner register hook 配線 site = shader 別 / asset 別 / skin 別の最適 site で個別配線 | 採用 | 「OK」(2026-06-05、2nd) |
| (W2-A) | forwardToUboUpload 内 cadence 判定 = 単一 forwardToUboUpload + switch 内 5 case 分岐 (= design 06b §5.2 literal 整合) | 採用 | 「OK」(2026-06-05、2nd) |
| (W3-B) | PC-7γ scope = forwardToUboUpload 本格化 + memcpy までで stop、flush 側 GPU 経路は PC-7δ | 採用 | 「OK」(2026-06-05、2nd) |
| (W4-A) | UboInstance dirty 粒度 = UBO physical instance 単位 (= `std::pair<Owner*, uint64_t block_hash>` key)、PC-6ε-2 の `<Owner*, UboInstance>` から refactor | 採用 | 「OK」(2026-06-05、2nd) |
| (W6-A) | per-frame UBO 配線 = `sFrameUboInstances` 新設 + initVulkan 時 allocate + shutdownVulkan teardown / per-draw は stub (PC-7ε で本格化) | 採用 | 「OK」(2026-06-05、4th) |
| (W7-C) | PC-7γ scope refinement = PC-7γ-1 (per-program 専念) + PC-7γ-2 (per-asset / per-skin、別 sub-step) に分割 | 採用 | 「OK」(2026-06-05、5th) |
| (W8-A) | per-program register hook = `LLGLSLShader::mapUniforms()` 完了 site で `mUniformUBOLoc` 経由 unique block_hash 集約 + try_emplace + allocateUboInstanceBuffers | 採用 | 「OK」(2026-06-05、5th) |
| Exit Criteria 9 項 (refinement 後) | per-program 1 件版で Exit Criteria 確定 | 採用 | 「OK」(2026-06-05、3rd) |

注: (W5) は中間案検討で AYA 確認に至らず番号スキップ (= (W4) → (W6) 直行)、本 handoff record では追跡対象外。

### §2.3 採用根拠 record

- **(W1-A) 個別配線根拠** = shader / asset / skin の lifecycle 差異 (= shader link 1 回 / asset acquire 多回 / skin attach 多回) で hook site が構造的に異なる、PC-6α LLAssetUboPool / PC-6β LLDrawUboRingBuffer 既存 hook 流用余地あり (asset side)、design 06b §2.4 (per-asset) + §2.5 (per-skin) literal 整合
- **(W2-A) switch 分岐根拠** = call site が setter 31 site で広範、5 case 分岐は callee 側 (= forwardToUboUpload 内) に閉じ込めて caller 側の影響面積最小化、design 06b §5.2 literal「switch (loc.cadence) { case PER_FRAME: ... case PER_PROGRAM: ... }」完全一致
- **(W3-B) PC-7γ memcpy までで stop 根拠** = UBO migration one-at-a-time (= feedback_ubo_migration_one_at_a_time)、forwardToUboUpload 本格化 + memcpy で host 側 write path 確立まで → flush 側 dirty.exchange + GPU 経路 (= vkCmdBindDescriptorSets) は PC-7δ で一体実装、handoff 境界での検証可能性維持
- **(W4-A) UBO physical instance 単位 dirty 根拠** = design 06b §3.2.3 + §3.4 canonical (= K2 UBO 単位 dirty flag)、PC-6ε-2 の `<Owner*, UboInstance>` は 1 owner 1 UboInstance で多 block 持つ owner (= shader) で dirty 共有 (false sharing) 発生、`<Owner*, block_hash>` key で UBO 単位独立性を確保
- **(W6-A) sFrameUboInstances 新設根拠** = per-frame cadence は owner 概念無し (= shader 共有)、global static array で 3 block (FrameViewProj / FrameLights / FrameAtmosphere) × FRAMES_IN_FLIGHT=3 = 9 buffer を init 時 allocate、design 07 §8.2 (per-frame UBO V3a 4 binding) + §8.4 (sFrameIndex rotate) 整合 / per-draw stub = ring buffer chunk hand-off (= PC-7ε scope) と独立、stub で switch case 構造を確定して PC-7ε で本格化
- **(W7-C) PC-7γ-1/-2 分割根拠** = per-asset / per-skin block は ubo_metadata.inl で **0 件登録** (= cadence_tag=3/4 不在、codegen 未生成) = setter 経由 forwardToUboUpload が呼ばれない、design 06b §2.4/§2.5 で別 path (GLTF update 関数経由) 必要 = PC-7γ-1 scope に含めると register hook 配線先が不明瞭、PC-7γ-2 で GLTF path 確定後に分離実装が安全 (= feedback_no_scope_shrink 整合 = AYA 明示承認取得済、scope 縮小ではなく refinement)
- **(W8-A) mUniformUBOLoc 経由根拠** = LLGLSLShader が参照する UBO block 集合は mapUniforms() 内で構築される mUniformUBOLoc にすでに集約されている (= block_hash unique)、mapUniforms() 完了直後の hook 1 site で全 block の try_emplace + allocateUboInstanceBuffers 一括完了、追加の grep / trace 不要

## §3 PC-7γ-1 scope (= refinement 後 lock)

### §3.1 PC-7γ-1 scope (実装対象)

**Title**: forwardToUboUpload 本格化 + sFrameUboInstances 新設 + per-program register/unregister hook + sProgramUboDirty key refactor (per-program 専念)

**実装項目** (= 6 件):

1. **UboInstanceKey + hash helper 新設** (anonymous ns、llvkloader.cpp):
   - `using UboInstanceKey = std::pair<LLGLSLShader*, uint64_t>;` (= owner + block_hash)
   - `struct UboInstanceKeyHash { size_t operator()(const UboInstanceKey&) const; };` (= std::hash<void*>(owner) ^ std::hash<uint64_t>(block_hash))
   - `sProgramUboDirty` 型を `std::unordered_map<LLGLSLShader*, UboInstance>` → `std::unordered_map<UboInstanceKey, UboInstance, UboInstanceKeyHash>` に refactor
   - `sAssetUboDirty` / `sSkinUboDirty` は **現形維持** (= PC-7γ-2 持越、本 PC-7γ-1 では触らない)

2. **sFrameUboInstances 新設** (anonymous ns、llvkloader.cpp):
   - `static std::unordered_map<uint64_t, UboInstance> sFrameUboInstances;` (= key = block_hash、owner 概念無し global static)
   - **initVulkan 時 allocate**: ubo_metadata.inl から cadence_tag=PER_FRAME (= 0) の block を全件 enumerate → 各 block について sFrameUboInstances[block_hash] を try_emplace + allocateUboInstanceBuffers(ubo, block_size, "PER_FRAME") 呼出 (= 3 block × 3 frame = 9 buffer)
   - **shutdownVulkan teardown**: PC-7β 既存の sProgramUboDirty / sAssetUboDirty / sSkinUboDirty entry 走査 destroy に sFrameUboInstances 追加 (= 4 map 全件 destroyUboInstanceBuffers)

3. **forwardToUboUpload 本格化** (llglslshader.cpp、line 2009-2011 既存 stub 置換):
   - Signature 確認: `void forwardToUboUpload(const UniformLocation& loc, const void* data, size_t size)` (= design 06b §5.1 literal)
   - 内部 switch (loc.cadence) 5 case 分岐:
     - **case PER_FRAME** (= 0): sFrameUboInstances[loc.block_hash] を find → `memcpy(ubo.mapped_ptr[sFrameIndex] + loc.offset, data, size)` + `ubo.dirty.store(true, std::memory_order_release)`
     - **case PER_PROGRAM** (= 1): `UboInstanceKey key{ this, loc.block_hash };` → sProgramUboDirty[key] を find → memcpy + dirty.store(true)
     - **case PER_DRAW** (= 2): **stub に LL_WARNS_ONCE("PC-7γ-1 stub, PC-7ε で本格化")** (= ring buffer chunk hand-off は PC-7ε scope)
     - **case PER_ASSET** (= 3): **stub に LL_WARNS_ONCE("PC-7γ-1 stub, PC-7γ-2 で配線")** (= ubo_metadata.inl で cadence_tag=3 は 0 件、setter 経由到達は本来発生しないが defensive stub)
     - **case PER_SKIN** (= 4): **stub に LL_WARNS_ONCE("PC-7γ-1 stub, PC-7γ-2 で配線")** (= 同上)
     - **case SINGLETON** (= 5): PC-6ε-1 で flushSingletonUbos 経路確定済 = forwardToUboUpload 経由は本来発生しない (= setter 内 dispatch で SINGLETON は別経路)、defensive `llassert(false && "SINGLETON forwarded via forwardToUboUpload unexpected")`
     - **case SAMPLER** (= 6) / **case UNKNOWN** (= 7) / **INVALID** (= 0xFFFFFFFF): `llassert(false && "invalid cadence")` (= PC-6ζ で sentinel 再配置済)
   - `loc.offset` 参照経路は ubo_metadata.inl `MemberMetadata` の `offset` field 経由、forwardToUboUpload caller 側 (= setter 内) で既に解決済の前提 (= UniformLocation struct に offset 含む)

4. **per-program register hook 配線** (llglslshader.cpp、mapUniforms() 完了 site = (W8-A)):
   - mapUniforms() 内 mUniformUBOLoc 構築完了直後に loop `for (auto& kv : mUniformUBOLoc) { uint64_t block_hash = kv.second.block_hash; ... }` → unique block_hash 集約 (= `std::unordered_set<uint64_t> seen_hashes;` で重複排除) → LLVKLoader::registerProgramUbo(this, block_hash, block_size) 呼出
   - LLVKLoader::registerProgramUbo (llvkloader.h / .cpp 新設、PC-7γ-1 で追加):
     - signature: `static bool registerProgramUbo(LLGLSLShader* shader, uint64_t block_hash, uint32_t block_size)`
     - body: `UboInstanceKey key{ shader, block_hash };` → `auto [it, inserted] = sProgramUboDirty.try_emplace(key);` → inserted 時のみ `allocateUboInstanceBuffers(it->second, block_size, "PER_PROGRAM")` 呼出 + 失敗時 erase + false return / 既存 entry 時 true return (= idempotent)
   - block_size 投入経路 = ubo_metadata.inl `BlockMetadata::block_size` field を mapUniforms() 内 lookup (= 既存 mUniformUBOLoc 構築時に block 解決済の前提、block_size accessor 追加が必要なら同 commit 内で実装)

5. **per-program unregister hook 配線** (llglslshader.cpp、unloadInternal() = line 385-410 候補):
   - unloadInternal() 内で `for (auto& kv : mUniformUBOLoc) { uint64_t block_hash = kv.second.block_hash; LLVKLoader::unregisterProgramUbo(this, block_hash); }` 呼出 (= mUniformUBOLoc clear() 前に発火)
   - LLVKLoader::unregisterProgramUbo (llvkloader.h / .cpp 新設):
     - signature: `static void unregisterProgramUbo(LLGLSLShader* shader, uint64_t block_hash)`
     - body: `UboInstanceKey key{ shader, block_hash };` → sProgramUboDirty.find(key) → 見つかれば `destroyUboInstanceBuffers(it->second)` + `sProgramUboDirty.erase(it)` / 見つからなければ no-op (= shader が mUseUBO=false で register されていない case safe)

6. **`[[maybe_unused]]` 属性撤去** (llvkloader.cpp、line 1271 allocateUboInstanceBuffers 関数宣言):
   - 上記 (4)(5) で call site 配線完了 = `-Werror=unused-function` 抑止不要、属性削除
   - destroyUboInstanceBuffers は属性不在 (PC-7β shutdownVulkan で既に call site 持つ) ゆえ変更なし

### §3.2 PC-7γ-1 scope **外** (= 持越)

- **per-asset / per-skin** (= cadence_tag=3/4): ubo_metadata.inl 未登録、GLTF update 関数経由の別 path 必要、PC-7γ-2 で別 sub-step として実装
- **flush 関数本格化**: dirty.exchange(false) 後の vkCmdBindDescriptorSets 通電 + memcpy → GPU 経路は PC-7δ scope
- **dynamic offset 経路**: per-draw ring buffer chunk hand-off + pDynamicOffsets[4] 配線は PC-7ε scope
- **codegen V1' update**: ubo_metadata.inl の set=1a/1b split 実装 + 130 件 unittest 回帰確認は PC-7α' scope (= Z2-C 持越、PC-7δ 通電前に必要)

### §3.3 PC-7γ-1 Exit Criteria (= 9 項、refinement 後の per-program 1 件版)

- **(i)** UboInstanceKey + hash helper 新設 + sProgramUboDirty 型 refactor (= `std::unordered_map<UboInstanceKey, UboInstance, UboInstanceKeyHash>`)、sAssetUboDirty / sSkinUboDirty 現形維持
- **(ii)** sFrameUboInstances 新設 + initVulkan 時 ubo_metadata.inl 経由 cadence_tag=PER_FRAME 全 block allocate (= 3 block × 3 frame = 9 buffer) + shutdownVulkan teardown 配線
- **(iii)** forwardToUboUpload 本格化 = switch 5 case 分岐 + PER_FRAME / PER_PROGRAM case で memcpy + dirty.store(true) / PER_DRAW / PER_ASSET / PER_SKIN case は stub (LL_WARNS_ONCE) / SINGLETON / SAMPLER / UNKNOWN / INVALID case は llassert(false)
- **(iv)** per-program register hook = LLGLSLShader::mapUniforms() 完了 site で mUniformUBOLoc 経由 unique block_hash 集約 + LLVKLoader::registerProgramUbo 呼出 (per-asset / per-skin register は PC-7γ-2 持越)
- **(v)** per-program unregister hook = LLGLSLShader::unloadInternal() で mUniformUBOLoc loop + LLVKLoader::unregisterProgramUbo 呼出 (per-asset / per-skin unregister は PC-7γ-2 持越)
- **(vi)** `[[maybe_unused]]` 属性撤去 (= allocateUboInstanceBuffers call site 通電)
- **(vii)** MUSEUBO-A 整合 = per-program entry gate (= shader->mUseUBO 明示参照、PC-6ε-2 + 本 PC-7γ-1 register hook 内側で参照) + mUseUBO=false default で既存 OpenGL 描画 100% 維持
- **(viii)** GATE-B 整合 = #ifdef LL_VULKAN_GLSL 新規追加 0 (= grep `git diff | grep "^\+" | grep LL_VULKAN_GLSL` で 0 件確認)
- **(ix)** llrender + newview build PASS + warning 0 + INTEGRATION_TEST_lluboringbuffer 11/11 + INTEGRATION_TEST_llassetubopool 10/10 + INTEGRATION_TEST_llpipelinecachestorage 13/13 + codegen unittest 130/130 全 PASS

### §3.4 着手 1 line (= 次 session)

PC-7γ-1 = (1) UboInstanceKey 新設 + sProgramUboDirty refactor → (2) sFrameUboInstances 新設 + init/shutdown 配線 → (3) forwardToUboUpload 本格化 (switch 5 case) → (4) LLVKLoader::registerProgramUbo / unregisterProgramUbo 新設 → (5) LLGLSLShader::mapUniforms() / unloadInternal() で hook 呼出 → (6) `[[maybe_unused]]` 撤去 → build verify + TUT + codegen → self-verify 9 観点 → handoff doc 起案 → AYA commit 指示待ち。

## §4 設計詳細 (= 実装時参照、context 圧迫で消失したくない部分)

### §4.1 UboInstanceKey hash 実装

```cpp
struct UboInstanceKeyHash {
    size_t operator()(const UboInstanceKey& k) const noexcept {
        // owner pointer と block_hash の XOR mix、衝突懸念低 (owner は ASLR で散る + block_hash は perfect_hash 由来)
        return std::hash<LLGLSLShader*>{}(k.first) ^ (std::hash<uint64_t>{}(k.second) << 1);
    }
};
```

### §4.2 forwardToUboUpload switch 雛形

```cpp
void LLGLSLShader::forwardToUboUpload(const UniformLocation& loc, const void* data, size_t size)
{
    if (!mUseUBO) return; // MUSEUBO-A 整合、defensive entry gate

    switch (loc.cadence)
    {
    case CADENCE_PER_FRAME:
    {
        auto it = sFrameUboInstances.find(loc.block_hash);
        if (it == sFrameUboInstances.end()) { LL_WARNS_ONCE() << "PER_FRAME UBO not registered: " << loc.block_hash << LL_ENDL; return; }
        UboInstance& ubo = it->second;
        if (ubo.size == 0 || !ubo.mapped_ptr[sFrameIndex]) return;
        memcpy(static_cast<uint8_t*>(ubo.mapped_ptr[sFrameIndex]) + loc.offset, data, size);
        ubo.dirty.store(true, std::memory_order_release);
        break;
    }
    case CADENCE_PER_PROGRAM:
    {
        UboInstanceKey key{ this, loc.block_hash };
        auto it = sProgramUboDirty.find(key);
        if (it == sProgramUboDirty.end()) { LL_WARNS_ONCE() << "PER_PROGRAM UBO not registered: " << loc.block_hash << LL_ENDL; return; }
        UboInstance& ubo = it->second;
        if (ubo.size == 0 || !ubo.mapped_ptr[sFrameIndex]) return;
        memcpy(static_cast<uint8_t*>(ubo.mapped_ptr[sFrameIndex]) + loc.offset, data, size);
        ubo.dirty.store(true, std::memory_order_release);
        break;
    }
    case CADENCE_PER_DRAW:
        LL_WARNS_ONCE() << "PER_DRAW stub: PC-7ε で本格化 (ring buffer chunk hand-off)" << LL_ENDL;
        break;
    case CADENCE_PER_ASSET:
        LL_WARNS_ONCE() << "PER_ASSET stub: PC-7γ-2 で配線 (GLTF Asset update path)" << LL_ENDL;
        break;
    case CADENCE_PER_SKIN:
        LL_WARNS_ONCE() << "PER_SKIN stub: PC-7γ-2 で配線 (GLTF Skin update path)" << LL_ENDL;
        break;
    case CADENCE_SINGLETON:
        llassert_always(false && "SINGLETON forwarded via forwardToUboUpload unexpected (PC-6ε-1 flushSingletonUbos 別経路)");
        break;
    case CADENCE_SAMPLER:
    case CADENCE_UNKNOWN:
    default:
        llassert_always(false && "invalid cadence in forwardToUboUpload");
        break;
    }
}
```

注: `UniformLocation::cadence` / `block_hash` / `offset` field の existence は llglslshader.h / llrender 既存 struct を実装着手前に確認すること。Phase 1.B / PC-6ε-2 で構築済の前提だが、field 名差異あれば適宜 rename。

### §4.3 registerProgramUbo 雛形

```cpp
// llvkloader.h
class LLVKLoader {
public:
    static bool registerProgramUbo(LLGLSLShader* shader, uint64_t block_hash, uint32_t block_size);
    static void unregisterProgramUbo(LLGLSLShader* shader, uint64_t block_hash);
    // ... 既存 API
};

// llvkloader.cpp
bool LLVKLoader::registerProgramUbo(LLGLSLShader* shader, uint64_t block_hash, uint32_t block_size)
{
    if (!shader || block_size == 0) return false;
    UboInstanceKey key{ shader, block_hash };
    auto [it, inserted] = sProgramUboDirty.try_emplace(key);
    if (!inserted) return true; // idempotent、再 register 無視
    if (!allocateUboInstanceBuffers(it->second, block_size, "PER_PROGRAM"))
    {
        sProgramUboDirty.erase(it);
        return false;
    }
    return true;
}

void LLVKLoader::unregisterProgramUbo(LLGLSLShader* shader, uint64_t block_hash)
{
    if (!shader) return;
    UboInstanceKey key{ shader, block_hash };
    auto it = sProgramUboDirty.find(key);
    if (it == sProgramUboDirty.end()) return; // shader が mUseUBO=false で未 register 等 safe
    destroyUboInstanceBuffers(it->second);
    sProgramUboDirty.erase(it);
}
```

### §4.4 mapUniforms() register hook 配置形

```cpp
// llglslshader.cpp、mapUniforms() 末尾近く (= mUniformUBOLoc 構築完了直後)
if (mUseUBO)
{
    std::unordered_set<uint64_t> seen_hashes;
    for (const auto& kv : mUniformUBOLoc)
    {
        uint64_t block_hash = kv.second.block_hash;
        if (!seen_hashes.insert(block_hash).second) continue; // 重複 skip
        uint32_t block_size = /* ubo_metadata.inl から lookup */;
        if (!LLVKLoader::registerProgramUbo(this, block_hash, block_size))
        {
            LL_WARNS() << "Failed to register PER_PROGRAM UBO: hash=" << block_hash << LL_ENDL;
        }
    }
}
```

注: ubo_metadata.inl から block_size lookup する API (= `getUboBlockSize(uint64_t block_hash)` 等) が既存になければ同 commit で追加が必要。codegen 既存 output (= `g_ubo_block_metadata[]` static array) を線形 scan するヘルパで足りる (= 91 block で O(N) 許容)。

### §4.5 unloadInternal() unregister hook 配置形

```cpp
// llglslshader.cpp、unloadInternal() 内 mUniformUBOLoc.clear() 前
if (mUseUBO)
{
    std::unordered_set<uint64_t> seen_hashes;
    for (const auto& kv : mUniformUBOLoc)
    {
        uint64_t block_hash = kv.second.block_hash;
        if (!seen_hashes.insert(block_hash).second) continue;
        LLVKLoader::unregisterProgramUbo(this, block_hash);
    }
}
```

### §4.6 sFrameUboInstances initVulkan 配置形

```cpp
// llvkloader.cpp、createV3aDescriptorSetLayouts() 直後の initVulkan 内
extern const BlockMetadata g_ubo_block_metadata[]; // codegen output
extern const size_t g_ubo_block_count;
for (size_t i = 0; i < g_ubo_block_count; ++i)
{
    const auto& meta = g_ubo_block_metadata[i];
    if (meta.cadence_tag != CADENCE_PER_FRAME) continue;
    UboInstance& ubo = sFrameUboInstances[meta.block_hash]; // default construct
    if (!allocateUboInstanceBuffers(ubo, meta.block_size, "PER_FRAME"))
    {
        LL_WARNS() << "Failed to allocate PER_FRAME UBO: " << meta.block_name << LL_ENDL;
        // shutdown 経路で部分 cleanup される、ここでは false return せず continue (= per-frame は global で起動 abort より degraded 起動を選好)
    }
}
```

注: g_ubo_block_metadata の symbol 名 / extern 宣言は codegen output (= ubo_metadata.inl) の実形に追従して調整。

### §4.7 shutdownVulkan teardown 追加

```cpp
// llvkloader.cpp、shutdownVulkan 内 PC-7β tag block 拡張
for (auto& kv : sFrameUboInstances) { destroyUboInstanceBuffers(kv.second); }
sFrameUboInstances.clear();
// 既存 PC-7β = sProgramUboDirty / sAssetUboDirty / sSkinUboDirty entry 走査 destroy + clear
```

## §5 self-verify 9 観点 (= 実装後、AYA commit 指示前に再確認)

実装完了後、handoff doc 起案前に以下 9 観点で self-verify する (= feedback_self_verify_before_handoff)。

1. **Exit Criteria 9 項全充足** = §3.3 (i)-(ix) 全件 ✅
2. **UboInstance + sFrameUboInstances source doc 整合** = design 06b §3.2.3 完成形 + design 07 §8.2 / §8.4 整合
3. **(W1-A)(W2-A)(W3-B)(W4-A)(W6-A)(W7-C)(W8-A) AYA 確認 record** = 本 doc §2.2 で 7 件 + Exit Criteria 1 件 = 8 件、AYA literal「OK」(2026-06-05、5 回)
4. **GATE-B 整合** = #ifdef LL_VULKAN_GLSL 新規追加 0 (grep 確認)
5. **MUSEUBO-A 整合** = forwardToUboUpload entry gate (`if (!mUseUBO) return;`) + register hook 内側 `if (mUseUBO)` gate で mUseUBO=false default で既存 OpenGL 描画 100% 維持
6. **build verify** = llrender + newview build PASS + warning 0 + TUT 11+10+13 + codegen 130/130 全 PASS
7. **commit 内容** = llvkloader.cpp + llvkloader.h + llglslshader.cpp + (必要時) llglslshader.h の 3-4 file modified + 1 new doc + 新 file 0 + CMake 改変 0 + settings.xml 改変 0 + Co-Authored-By 不在
8. **feedback_no_scope_shrink 遵守** = PC-7γ-1 literal scope (= 本 doc §3.1 6 件) 完全実施、per-asset/per-skin の PC-7γ-2 分離は AYA (W7-C) 確認済の refinement で scope 縮小ではない
9. **feedback_design_phase_no_code_write 整合** = 本 PC-7γ-1 は実装 phase (= PC-7β commit 後)、indra/ 改変 = 設計 phase ではない

## §6 Phase 1.C 進行 state + 残線形

### §6.1 milestone state

- Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅
- PC-0 ✅ + PC-1 ✅ + PC-2 ✅ + PC-3 ✅ + PC-4 ✅ + PC-5 ✅
- PC-6α ✅ + PC-6β ✅ + PC-6γ ✅ + PC-6δ ✅ + PC-6δ' ✅ + PC-6ε-1 ✅ + PC-6ε-2 ✅ + PC-6ε-3 ✅ + PC-6ζ ✅
- PC-7α ✅ + PC-7β ✅
- **PC-7γ-1 design-locked (本 doc) / 実装 pending**
- PC-7γ-2 ⏳ (= per-asset / per-skin、PC-7γ-1 commit 後)
- PC-7δ ⏳ (= vkCmdBindDescriptorSets 通電 + flush 側 GPU 経路)
- PC-7ε ⏳ (= dynamic offset + per-draw ring buffer chunk hand-off)
- PC-7α' ⏳ (= codegen V1' update、Z2-C 持越、PC-7δ 通電前に必要)
- PC-8 ⏳ (= 3 OS build verify)
- PC-N ⏳ (= Phase 1.C complete marker)

### §6.2 残 strict 線形

- **PC-7γ-1** (= forwardToUboUpload 本格化 + per-program 専念) → 本 doc scope
- **PC-7γ-2** (= per-asset / per-skin register hook + GLTF update path)
- **PC-7δ** (= vkCmdBindDescriptorSets 通電 + sAYAStandardLayout 経由 bind + flush 側 dirty.exchange 後 GPU 経路)
- **PC-7ε** (= dynamic offset ring buffer chunk hand-off + pDynamicOffsets[4])
- **PC-7α'** (= codegen ubo_metadata.inl V1' update、PC-7δ 通電前)
- **PC-8** (= 3 OS build verify)
- **PC-N** (= Phase 1.C complete marker)

## §7 feedback rule 遵守 record (= 本 design-lock 起案時)

- **feedback_proactive_handoff 遵守** = 前 session 末で context 圧迫 → /clear → summary 復元の経緯あり、本 session で再確定した PC-7γ scope refinement + 8 件 AYA 確認 record を実装着手前に pin、AYA literal「OK」(2026-06-05) で本 doc 起案承認受領
- **feedback_handoff_minimal_pre_req_read 遵守** = §1 必読 3 件 + pinpoint reference 別記、全箇条書きリスト不採用
- **feedback_self_verify_before_handoff 整合** = 実装着手前ゆえ §5 は scaffold のみ、実装後の self-verify は次 session で実施
- **feedback_no_scope_shrink 遵守** = PC-7γ-1 への refinement は AYA (W7-C) literal「OK」(2026-06-05、5th) で明示承認取得済、scope 縮小ではなく per-asset/per-skin の path 不明瞭 (= ubo_metadata.inl 未登録) を別 sub に分離
- **feedback_doubt_self_first 遵守** = 8 件 ambiguity 発見で停止 + 推奨案提示 + AYA 確認 + literal「OK」5 回受領後 design lock
- **feedback_confirm_referent_before_acting 遵守** = 同上、5 段階 AYA 確認、推測実装なし
- **feedback_ubo_migration_one_at_a_time 遵守** = PC-7γ-1 = forwardToUboUpload 本格化 + per-program 専念単独、per-asset/per-skin は PC-7γ-2 へ分離、bind 通電 + dynamic offset は PC-7δ..ε へ分離
- **feedback_design_phase_no_code_write 遵守** = 本 doc は実装着手前 design-lock、indra/ 改変 0 件 (= 本 handoff doc 1 new file のみ)
- **feedback_no_auto_commit 遵守** = AYA 明示「handoff 必要ないですか？」→ Claude 提案 → AYA literal「OK」(2026-06-05) で起案、commit 指示は別途
- **feedback_no_claude_coauthor 遵守** = Co-Authored-By 行不在
