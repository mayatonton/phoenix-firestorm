# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-7γ-1 complete

**起案日**: 2026-06-05
**起案者**: Claude (本 session)
**前 commit**: 4ef7ab2aab (PC-7β = UboInstance triple-buffer skeleton)
**本 commit 対象**: PC-7γ-1 complete (per-program 専念、per-asset/skin は PC-7γ-2 持越)
**次 sub-step**: PC-7γ-2 (per-asset / per-skin UBO register/write 配線、GLTF path 確定後)

---

## §0 Scope

PC-7γ-1 = per-program UBO 経路本格化 (per-asset / per-skin 持越し)。design-lock doc
(handoff-substep-...-pc-7-gamma-1-design-lock.md) §3 で AYA 確認済の 6 item literal
scope を完全実装:

1. **UboInstanceKey 新設 + sProgramUboDirty refactor** (= shader × block_hash key 化、K2 採用)
2. **sFrameUboInstances 新設 + init/shutdown 配線** (= PER_FRAME UBO 先回り allocate)
3. **forwardToUboUpload 本格化** (= switch 5 case + PER_FRAME/PER_PROGRAM 通電 + 他 stub)
4. **LLVKLoader::registerProgramUbo + unregisterProgramUbo + writeFrameUbo + writeProgramUbo 新設**
5. **LLGLSLShader::mapUniforms() / unloadInternal() で hook 呼出**
6. **`[[maybe_unused]]` 撤去** (= allocateUboInstanceBuffers が register 経路から呼出開始)

scope literal 完全実施、scope 縮小なし (= feedback_no_scope_shrink 遵守)。

---

## §1 編集詳細

### §1.0 全体 stat (= git diff --stat 抜粋)

```
 indra/llrender/llglslshader.cpp | 192 ++++++++++++++++++++++++++++--
 indra/llrender/llvkloader.cpp   | 250 ++++++++++++++++++++++++++++++++++++++--
 indra/llrender/llvkloader.h     |  38 ++++++
 3 files changed, 461 insertions(+), 19 deletions(-)
```

新 file 0 件 + CMake 改変 0 + settings.xml 改変 0。

### §1.1 llvkloader.h 編集 (+38 行)

flushSingletonUbos() 直後に PC-7γ-1 tag block + 4 method 宣言追加:

```cpp
bool registerProgramUbo  (LLGLSLShader* shader, U32 block_hash, U32 block_size);
void unregisterProgramUbo(LLGLSLShader* shader, U32 block_hash);
void writeFrameUbo       (U32 block_hash, U32 offset, const void* data, size_t size);
void writeProgramUbo     (LLGLSLShader* shader, U32 block_hash, U32 offset, const void* data, size_t size);
```

`block_hash` 型は `U32` 採用 (= design-lock doc 当初記載 `uint64_t` を実 codebase の
`ubo::UniformLocation::block_hash std::uint32_t` (= ubo_perfect_hash.inl line 14) に
合わせ、AYA 採択 2026-06-05 (C1) 受領済)。

### §1.2 llvkloader.cpp 編集 (+250 / -9)

#### §1.2.1 includes 拡充 (= ubo_metadata.inl + <utility>)
`#include "ubo/ubo_metadata.inl"` 新規追加 (= g_block_metadata[] walk 用、initVulkan
で PER_FRAME 全 block enumerate 必要)、`#include <utility>` 追加 (= std::pair)。

#### §1.2.2 UboInstanceKey + UboInstanceKeyHash 新設 (anonymous ns)
```cpp
using UboInstanceKey = std::pair<LLGLSLShader*, U32 /*block_hash*/>;
struct UboInstanceKeyHash {
    std::size_t operator()(const UboInstanceKey& k) const noexcept {
        return std::hash<void*>{}(static_cast<void*>(k.first))
             ^ (std::hash<U32>{}(k.second) << 1);
    }
};
```

PC-6ε-2 既存 `sProgramUboDirty` 型を `std::unordered_map<LLGLSLShader*, UboInstance>`
→ `std::unordered_map<UboInstanceKey, UboInstance, UboInstanceKeyHash>` に refactor。
sAssetUboDirty / sSkinUboDirty は現形維持 (= PC-7γ-2 持越、W7-C 整合)。

#### §1.2.3 sFrameUboInstances 新設 (anonymous ns)
```cpp
std::unordered_map<U32 /*block_hash*/, UboInstance> sFrameUboInstances;
```
key=block_hash 単独 (= owner 概念無し、全 shader 共有 = design 06b §2.1 + 07 §8.2)。

#### §1.2.4 `[[maybe_unused]]` 撤去 (= allocateUboInstanceBuffers)
PC-7β tag comment を「PC-7γ-1 で属性撤去 = registerProgramUbo / initVulkan から
呼出開始」に更新、属性削除 (= AYA (W8-A) 整合)。

#### §1.2.5 flushProgramUbos key 経路追従
PC-6ε-2 既存 `sProgramUboDirty.find(shader)` を walk + `kv.first.first == shader`
match に refactor (= 1 shader N block 個別 dedup)。PC-7γ-1 commit 時点では writeProgramUbo
経由 entry 生成済だが GPU 経路 (= PC-7δ scope = W3-B「memcpy までで stop」) は未通電
ゆえ flushDummyUboWrite 既存形維持。

#### §1.2.6 initVulkan sFrameUboInstances allocate 配線
PC-7α V3a scaffolding 直後 (createV3aDescriptorSetLayouts/Pools/StandardPipelineLayout
後) に挿入。`g_block_metadata[91]` を walk、cadence_tag=0 (= PER_FRAME) block を全件
try_emplace + allocateUboInstanceBuffers (= 個別 block_size、3 buffer × FRAMES_IN_FLIGHT=3
= 9 buffer)。failure 時 shutdownVulkan() 経由 cleanup。

現 codegen 出力 (= ubo_metadata.inl) で cadence_tag=0 block は 3 件:
- `FrameAtmosphere_Lighting` (block_hash=0x14974e57u, size=256u)
- `FrameLights` (block_hash=0xed61ac9bu, size=768u)
- `FrameViewProj` (block_hash=0x06aff62cu, size=512u)

#### §1.2.7 shutdownVulkan sFrameUboInstances teardown 追加
PC-7β 既存 3 dirty map teardown 直後に 4 map 目として sFrameUboInstances 走査 destroy +
clear。sAllocator 生存中に発火 (= init reverse 順)。

#### §1.2.8 4 LLVKLoader namespace method 定義 (flushSingletonUbos 直後 +約140 行)
- `registerProgramUbo`: try_emplace + allocateUboInstanceBuffers (= 失敗時 erase + false return / 既存 entry idempotent true return)
- `unregisterProgramUbo`: find → destroyUboInstanceBuffers + erase (= 未 register safe)
- `writeFrameUbo`: sFrameUboInstances.find + offset check + std::memcpy + dirty.store(true)
- `writeProgramUbo`: sProgramUboDirty.find (UboInstanceKey) + offset check + memcpy + dirty.store(true)

全 4 method で:
- nullptr / size==0 / out-of-range は LL_WARNS_ONCE + return (= silent skip)
- mapped_ptr[sFrameIndex] への direct memcpy (= persistent map、PC-7β 確保形 (Y4-A) 整合)
- std::memory_order_release で dirty.store (= PC-7δ flush 側 acq_rel と pair)

### §1.3 llglslshader.cpp 編集 (+192 / -10)

#### §1.3.1 includes 拡充
`#include <unordered_set>` 新規追加 (= mapUniforms / unloadInternal で
seen_program_hashes に使用)。`#include "llvkloader.h"` は既存 (PC-6δ から)。

#### §1.3.2 anonymous ns kCadence* constexpr + lookup_block_size_by_hash helper
`using` 直後に新設:
```cpp
namespace {
    constexpr U32 kCadencePerFrame   = 0u;
    constexpr U32 kCadencePerProgram = 1u;
    constexpr U32 kCadencePerDraw    = 2u;
    constexpr U32 kCadencePerAsset   = 3u;
    constexpr U32 kCadencePerSkin    = 4u;
    constexpr U32 kCadenceSingleton  = 5u;
    constexpr U32 kCadenceSampler    = 6u;
    constexpr U32 kCadenceUnknown    = 7u;
    constexpr U32 kCadenceInvalid    = 0xFFFFFFFFu;

    U32 lookup_block_size_by_hash(U32 block_hash) {
        for (U32 i = 0; i < ubo::g_block_count; ++i) {
            if (ubo::g_block_metadata[i].block_hash == block_hash) {
                return ubo::g_block_metadata[i].block_size;
            }
        }
        return 0u;
    }
}
```

C++ 側に CadenceTag enum 定義不在 (= codegen 出力 inl は数値 literal、design 06a §3.3
は spec 上のみの enum) ゆえ anonymous ns localize (AYA (C2) 採択 2026-06-05)。

#### §1.3.3 forwardToUboUpload 本格化 (line 2009-2011 stub 置換)
switch (loc.cadence_tag) で 9 case 分岐:
- PER_FRAME (0)   → LLVKLoader::writeFrameUbo (経路通電)
- PER_PROGRAM (1) → LLVKLoader::writeProgramUbo (経路通電)
- PER_DRAW (2)    → LL_WARNS_ONCE (PC-7ε scope)
- PER_ASSET (3)   → LL_WARNS_ONCE (PC-7γ-2 scope)
- PER_SKIN (4)    → LL_WARNS_ONCE (PC-7γ-2 scope)
- SINGLETON (5)   → llassert_always(false) (= flushSingletonUbos 別経路で本来発生せず)
- SAMPLER (6)     → return (= 呼出側 setter 31 site で既 skip 済の防御)
- UNKNOWN (7)     → return (同上)
- INVALID (0xFFFF)→ return (同上)
- default         → llassert_always(false)

#### §1.3.4 mapUniforms() per-program register hook 配線 (bringupTestUBO 直後)
`if (mUseUBO)` block 内側で:
1. seen_program_hashes (unordered_set<U32>) に PER_PROGRAM block_hash を集約 (重複排除)
2. lookup_block_size_by_hash で block_size 解決
3. LLVKLoader::registerProgramUbo(this, block_hash, block_size) 呼出
4. 失敗時 LL_WARNS (= continue で次 block 試行)

#### §1.3.5 unloadInternal() per-program unregister hook 配線
function 冒頭 (`sInstances.erase(this);` 直後、`mUniform.clear();` 前) に挿入。
mUseUBO=true ならば PER_PROGRAM block_hash unique 集約 + unregisterProgramUbo 呼出。
直後に mUniformUBOLoc.clear() + mUniformUBOLocByHash.clear() を hook 後に追加
(= shader 再 link 整合保証、unregister 後 cache 解放 = 二重 register 防止)。

---

## §2 build verify (= literal 検証)

### §2.1 llrender build
```
make -j4 llrender
[ 90%] Building CXX object llrender/CMakeFiles/llrender.dir/llglslshader.cpp.o
[ 90%] Building CXX object llrender/CMakeFiles/llrender.dir/llvkloader.cpp.o
[ 90%] Linking CXX static library libllrender.a
[100%] Built target llrender
```
ERROR 0 / WARNING 0 (= PC-7γ-1 改変関連 zero、touch + rebuild で再確認済)。

### §2.2 TUT integration tests
```
INTEGRATION_TEST_lluboringbuffer        : 11/11 YAY
INTEGRATION_TEST_llassetubopool         : 10/10 YAY
INTEGRATION_TEST_llpipelinecachestorage : 13/13 YAY
```
PC-3 / PC-4 / PC-5 algorithm 層 regression なし。

### §2.3 codegen unittest
```
python3 -m unittest discover -s scripts/ubo_codegen/tests
Ran 130 tests in 0.061s
OK
```
130/130 PASS (= Phase 1.A / 1.B / 1.C PC-1..PC-7β regression なし)。

---

## §3 PC-7γ-1 Exit Criteria 9 項全充足

- **(i)** ✅ UboInstanceKey + UboInstanceKeyHash 新設、sProgramUboDirty 型 refactor
  (= `std::unordered_map<UboInstanceKey, UboInstance, UboInstanceKeyHash>`)、sAssetUboDirty
  / sSkinUboDirty 現形維持
- **(ii)** ✅ sFrameUboInstances 新設 + initVulkan で cadence_tag=PER_FRAME 全 block
  (= 3 件) allocate + shutdownVulkan 対称 teardown
- **(iii)** ✅ forwardToUboUpload 本格化 = switch 9 case (5 case + sentinel 3 + default) +
  PER_FRAME/PER_PROGRAM 通電 + PER_DRAW/ASSET/SKIN stub LL_WARNS_ONCE + SINGLETON
  llassert_always + SAMPLER/UNKNOWN/INVALID defensive return
- **(iv)** ✅ mapUniforms() per-program register hook = bringupTestUBO 直後、
  mUniformUBOLoc walk + unique block_hash 集約 + lookup_block_size_by_hash + registerProgramUbo
- **(v)** ✅ unloadInternal() per-program unregister hook = mUniformUBOLoc.clear() 前、
  unique block_hash 集約 + unregisterProgramUbo + cache clear
- **(vi)** ✅ `[[maybe_unused]]` 撤去 = allocateUboInstanceBuffers から属性削除、
  registerProgramUbo / initVulkan PER_FRAME allocate path で呼出開始
- **(vii)** ✅ make -j4 llrender PASS + warning 0
- **(viii)** ✅ TUT 3 件 (11/11 + 10/10 + 13/13) + codegen 130/130 全 PASS
- **(ix)** ✅ MUSEUBO-A + GATE-B 整合 = MUSEUBO-A: mapUniforms/unloadInternal で
  `if (mUseUBO)` block 内 register/unregister、mUseUBO=false default で sProgramUboDirty
  / sFrameUboInstances 双方 entry 入らず既存 OpenGL 描画 100% 維持。GATE-B: 新 `#ifdef
  LL_VULKAN_GLSL` 追加 0 (= grep 確認、git diff 内 `LL_VULKAN_GLSL` 出現 6 件全件
  comment 内 GATE-B 整合宣言のみ)

---

## §4 残 strict 線形

PC-7γ-1 = ✅ 本 commit (per-program 専念)
→ PC-7γ-2 = per-asset / per-skin UBO register/write 配線 (= GLTF path 確定後、
  sAssetUboDirty / sSkinUboDirty key 拡張 + forwardToUboUpload PER_ASSET/PER_SKIN case
  stub 置換 + LL::GLTF::Asset/Skin lifecycle hook 配線、source doc = 06b §2.4/§2.5)
→ PC-7δ = vkCmdBindDescriptorSets 通電 + set=3 swap + sAYAStandardLayout 経由 bind +
  flush 側 dirty.exchange + GPU upload 経路 (= 本 PC-7γ-1 dirty.store(release) と pair)
→ PC-7ε = dynamic offset 経路 ring buffer chunk hand-off (= PER_DRAW、
  sDrawUboRingBufferMgr 経由)
→ PC-7α' = codegen ubo_metadata.inl V1' update (= set=1a/1b split、Z2-C 持越、PC-7δ
  通電前に必要)
→ PC-8 = 3 OS build verify (= Linux primary + Win/Mac 後段)
→ PC-N = Phase 1.C complete marker + Phase 1.D / Phase 2 着手起点

---

## §5 r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅
+ PC-0 ✅ + PC-1 ✅ + PC-2 ✅ + PC-3 ✅ + PC-4 ✅ + PC-5 ✅ + PC-6α ✅ + PC-6β ✅
+ PC-6γ ✅ + PC-6δ ✅ + PC-6δ' ✅ + PC-6ε-1 ✅ + PC-6ε-2 ✅ + PC-6ε-3 ✅ + PC-6ζ ✅
+ PC-7α ✅ + PC-7β ✅ + **PC-7γ-1 ✅ 本 commit**
+ PC-7γ-2..PC-N ⏳ 次 session

---

## §6 self-verify 9 観点 全 ✅

1. **Exit Criteria 9 項全充足** = §3 で literal 確認済
2. **source doc 整合** = design 06b §3.2 / §3.2.3 / §5.2 / §5.4 + design 06a §3.3 +
   design 07 §8.2 / §8.3 / §8.4 + design 09 §4.1、UniformLocation 型は
   ubo_perfect_hash.inl 実装と整合 (= U32 block_hash 採用、AYA (C1) 確認)
3. **AYA 採用根拠 record** = design-lock doc §2 で W1-A/W2-A/W3-B/W4-A/W6-A/W7-C/W8-A
   + Exit Criteria 9 項目 + (C1)〜(C4) 全件 literal「OK」(2026-06-05) record 完了済
4. **GATE-B 整合** = 新 `#ifdef LL_VULKAN_GLSL` 追加 0 件 (= grep 確認)、
   diff 内 6 件全件 comment 内 GATE-B 整合宣言のみ、Vulkan init 層単独
5. **MUSEUBO-A 整合** = mapUniforms/unloadInternal で `if (mUseUBO)` block 内 hook、
   forwardToUboUpload 呼出側 setter 31 site も既存 `if (mUseUBO)` block 内側、
   mUseUBO=false default で全 entry 不到達 = 既存 OpenGL 描画 100% 維持
6. **build + TUT + codegen 全 PASS** = §2 で literal 検証取得
7. **commit 内容** = 3 modified + 1 new doc (本 handoff) + 新 file 0 + CMake 改変 0
   + settings.xml 改変 0 + Co-Authored-By 不在
8. **feedback_no_scope_shrink 遵守** = PC-7γ-1 literal scope (= 6 item) 完全実施、
   per-asset/per-skin の PC-7γ-2 分離は AYA (W7-C) 確認済 refinement で scope 縮小ではない、
   PER_DRAW stub は AYA (W3-B) 確認済 = 「memcpy までで stop」整合
9. **feedback_design_phase_no_code_write 整合** = 本 PC-7γ-1 は実装 phase
   (= design-lock doc commit 後)、indra/ 改変 3 件 = 設計 phase ではない

---

## §7 引き継ぎ memory (feedback rules 遵守 record)

- **feedback_proactive_handoff** 遵守 = 本 handoff 起案で PC-7γ-2 引継 marker 確立
- **feedback_handoff_minimal_pre_req_read** 遵守 = 必読 3 件 (= design-lock doc + PC-7β
  handoff + 06b §3.2 + §5.2) + pinpoint Read で context 圧迫回避
- **feedback_self_verify_before_handoff** 遵守 = 本 §3 + §6 で 9 観点 self-verify 全 ✅
- **feedback_build_only_verified** 遵守 = llrender build + TUT 3 件 + codegen 130/130
  で literal 検証取得
- **feedback_no_scope_shrink** 遵守 = §6 (8) 記載
- **feedback_doubt_self_first** 遵守 = 設計 lock doc の `uint64_t` 仮定を実 codebase
  trace で `std::uint32_t` 確認、AYA (C1) で型修正承認取得後実装
- **feedback_confirm_referent_before_acting** 遵守 = (C1)/(C2)/(C3)/(C4) 4 件確認、
  推測実装なし
- **feedback_ubo_migration_one_at_a_time** 遵守 = PC-7γ-1 = per-program 専念単独実施、
  per-asset/skin は PC-7γ-2 へ分離、bind + dynamic offset は PC-7δ/ε へ分離
- **feedback_design_phase_no_code_write** 整合 = 本 PC-7γ-1 は実装 phase、indra/ 改変 3 件
- **feedback_release_branch_workflow** 遵守 = feature branch
  `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示待ち (= 本 handoff 起案後)
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在

---

## §8 次 session 着手 1 line

PC-7γ-2 = per-asset / per-skin UBO 経路本格化 = GLTF path 確定 (= LL::GLTF::Asset /
Skin lifecycle hook 確定) → sAssetUboDirty / sSkinUboDirty key 拡張 (= asset×block_hash
/ skin×block_hash) → forwardToUboUpload PER_ASSET/PER_SKIN case stub 置換 (=
writeAssetUbo / writeSkinUbo 経由) → register/unregister hook 配線、Exit Criteria は
次 session 着手前整理。
