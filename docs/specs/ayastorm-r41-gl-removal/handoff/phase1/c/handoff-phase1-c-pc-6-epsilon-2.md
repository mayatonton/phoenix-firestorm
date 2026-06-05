# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6ε-2 complete handoff

**作成日**: 2026-06-04
**HEAD (PC-6ε-2 着手前)**: `2d1913357d` (= PC-6ε-1 = `flushSingletonUbos()` 新設 + `bringupTestUBO()` 経由本格置換 + 6 cadence 体系成立)
**完了 marker**: PC-6ε-2 = per-program / per-asset / per-skin cadence **dirty propagation 配線** = `UboInstance` 最小 struct 先行新設 (= chapter 07 拡充 placeholder) + 3 dirty map (`std::unordered_map<Key, UboInstance>` × 3) + `flushProgramUbos` / `flushAssetUbos` / `flushSkinUbos` の key 化 + `mUseUBO` runtime gate 配下
**次 session 着手 1 line**: PC-6ε-3 = per-draw cadence 残 pool 全配線 (= PC-6δ canary `LLDrawPoolSimple` 以外 15+ pool subclass、design 06b §2.3 + codebase trace)

---

## §0 必読 3 件 (= minimal pre-req per `feedback_handoff_minimal_pre_req_read`)

1. **本 handoff doc 全文** (= §1-§5)
2. **`docs/specs/ayastorm-r41-gl-removal/design/literal-cross-ref-audit.md`** §3.1 (PC-6ε scope canonical literals) + §3.2 (PC-6ζ scope)
3. **`docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md`** §2.3 (per-draw cadence update site) + §3.2.3 (`UboInstance::dirty` `std::atomic<bool>`) + §5.3 (`mUseUBO` gate ↔ dirty propagation) + §5.4 (per-cadence dirty propagation)

pinpoint reference (必要時のみ):
- `indra/llrender/llvkloader.cpp:400-449` = `UboInstance` 最小 struct + 3 dirty map 宣言 (anonymous ns、tag block 含む)
- `indra/llrender/llvkloader.cpp:3220-3300` = `flushProgramUbos` / `flushDrawUbos` / `flushAssetUbos` / `flushSkinUbos` PC-6ε-2 改修後本体
- `indra/llrender/llvkloader.cpp:2802-2814` = shutdown 経路 3 dirty map clear
- `indra/llrender/llvkloader.h:322-340` = 5 cadence flush 関数宣言 + PC-6ε-2 update tag block
- `indra/llrender/llglslshader.h:429` = `LLGLSLShader::mUseUBO` bool member 宣言 (= 06a §3.2 canonical)

---

## §1 PC-6ε-2 実施内容 (= 7 編集 / +99 / -7 行)

### §1.1 編集 1 = `indra/llrender/llvkloader.cpp` includes (+2 行)

- `#include "llglslshader.h"` 追加 (= `LLGLSLShader::mUseUBO` 完全型必要)
- `#include <atomic>` 追加 (= `std::atomic<bool>` 完全型必要)

llglslshader.h は llvkloader.h を include せず circular dep 無し (= grep 確認済)。

### §1.2 編集 2 = `indra/llrender/llvkloader.cpp` anonymous ns (+45 行)

`DrawUboRingBufferRecord` struct + `sDrawUboRingBufferRecords` 直後に PC-6ε-2 tag block + `UboInstance` 最小 struct + 3 dirty map 宣言:

```cpp
struct UboInstance
{
    std::atomic<bool> dirty{false};
    // PC-7 拡充 placeholder:
    //   VkBuffer      vk_buffer  = VK_NULL_HANDLE;
    //   void*         mapped_ptr = nullptr;
    //   uint32_t      size       = 0;
};
std::unordered_map<LLGLSLShader*, UboInstance>    sProgramUboDirty;
std::unordered_map<LL::GLTF::Asset*, UboInstance> sAssetUboDirty;
std::unordered_map<LL::GLTF::Skin*, UboInstance>  sSkinUboDirty;
```

tag block 内記述事項:
- 二段階 dedup 構造 (= design 06b §3.2、stage 1 = `mValue` cache / stage 3 = UBO dirty bit)
- stage 2 (= `forwardToUboUpload` routing) は Phase 1.B 既存 stub のまま、本格化は PC-7 で実施
- gate 配置 (= design 06b §5.3 + AYA 確認 2026-06-04):
  - per-program = entry gate (= shader 個別 mUseUBO 直接参照)
  - per-asset / per-skin = 構造的 gate (= setter 側 mUseUBO 分岐由来)
  - per-frame / per-draw / singleton = gate 無し (= GATE-B 整合)
- `std::atomic<bool>` の move 不可 → `operator[]` (C++17 piecewise default construct) または `try_emplace(key)` 経由のみ insert 可、本 sub では `find(key)` のみで insert path 無し (= dirty=true 経路は PC-7 完成時に有効化)

### §1.3 編集 3 = `indra/llrender/llvkloader.cpp` shutdown (+8 行)

`sDrawUboRingBufferRecords.clear();` 直後に PC-6ε-2 tag block + 3 dirty map `clear()` 追加:

```cpp
sProgramUboDirty.clear();
sAssetUboDirty.clear();
sSkinUboDirty.clear();
```

UboInstance は外部 resource 所有なし (= dirty atomic 単独) ゆえ `clear()` のみで安全 (= PC-7 で VkBuffer 追加時は destroy hook が必要、本 sub では placeholder)。

### §1.4 編集 4 = `indra/llrender/llvkloader.cpp` `flushProgramUbos` 改修 (+18 / -2 行)

旧 PC-6δ body:
```cpp
void flushProgramUbos(LLGLSLShader* shader)
{
    (void)shader; // PC-6ε で per-program dirty map key 化、本 sub では未参照
    flushDummyUboWrite("flushProgramUbos");
}
```

新 PC-6ε-2 body:
```cpp
void flushProgramUbos(LLGLSLShader* shader)
{
    if (!shader || !shader->mUseUBO)
    {
        return;
    }
    auto it = sProgramUboDirty.find(shader);
    if (it == sProgramUboDirty.end())
    {
        return;
    }
    if (!it->second.dirty.exchange(false, std::memory_order_acq_rel))
    {
        return;
    }
    flushDummyUboWrite("flushProgramUbos");
}
```

- entry gate `if (!shader || !shader->mUseUBO) return;` = design 06b §5.3 confirmed (= shader 個別 mUseUBO 直接参照、06a §3.2)
- `nullptr` 防御 = (void)shader cast 廃止に伴う安全側追加
- `find` + `exchange(false, acq_rel)` = design 06b §3.2.3 (= dirty bit set/check 規律) 整合

### §1.5 編集 5 = `indra/llrender/llvkloader.cpp` `flushDrawUbos` 改修 (= comment only +3 行)

```cpp
void flushDrawUbos()
{
    // r41 PC-6ε-2: per-draw は shader / owner key 無し = 構造的 gate のみ。
    // setter 側 mUseUBO 分岐で forwardToUboUpload 不呼出 → ring buffer に値入らず
    // → 空書込 path 維持 (= PC-6δ 既存形踏襲)。残 pool 全配線 (15+ subclass) は
    // PC-6ε-3 scope (= design 06b §2.3 + codebase trace)。
    flushDummyUboWrite("flushDrawUbos");
}
```

body 不変、PC-6ε-2 の文脈で per-draw が他 3 cadence と異なる構造的位置にあることを明示する comment 追加のみ。

### §1.6 編集 6 = `indra/llrender/llvkloader.cpp` `flushAssetUbos` 改修 (+18 / -2 行)

新 PC-6ε-2 body (= per-program と同形だが entry gate なし、構造的 gate のみ):

```cpp
void flushAssetUbos(LL::GLTF::Asset* asset)
{
    if (!asset)
    {
        return;
    }
    auto it = sAssetUboDirty.find(asset);
    if (it == sAssetUboDirty.end())
    {
        return;
    }
    if (!it->second.dirty.exchange(false, std::memory_order_acq_rel))
    {
        return;
    }
    flushDummyUboWrite("flushAssetUbos");
}
```

`asset` 引数文脈 mUseUBO 不在 (= shader 引数無し) のため entry gate 不可、代わりに setter 側 mUseUBO 分岐で `forwardToUboUpload` 不呼出 → `sAssetUboDirty` 空のまま → flush で no-op (= 構造的 gate)。

### §1.7 編集 7 = `indra/llrender/llvkloader.cpp` `flushSkinUbos` 改修 (+18 / -2 行)

per-asset と同形 (= 構造的 gate + key 化、key 型のみ `LL::GLTF::Skin*` に変更)。

### §1.8 編集 8 = `indra/llrender/llvkloader.h` header comment (+11 行)

`flushFrameUbos` / `flushProgramUbos` / `flushDrawUbos` / `flushAssetUbos` / `flushSkinUbos` 宣言直前の PC-6δ tag block 末尾に PC-6ε-2 update 段落追加:

- 「引数 key 化を本 PC-6ε-2 で実施」
- per-program entry gate + map find + exchange 経路
- per-asset / per-skin 構造的 gate + nullptr 防御
- per-draw は構造的 gate のみ (= PC-6δ 既存形踏襲、残 pool 配線は PC-6ε-3)

---

## §2 Exit Criteria 7 項全充足

| # | 項目 | 検証 record |
|---|---|---|
| (i) | `UboInstance` 最小 struct 新設 | ✅ llvkloader.cpp anonymous ns (line 400-449)、`std::atomic<bool> dirty{false}` member 単独、PC-7 拡充 placeholder comment 配置 |
| (ii) | per-program / per-asset / per-skin dirty map 3 件新設 | ✅ `sProgramUboDirty` (key=`LLGLSLShader*`) / `sAssetUboDirty` (key=`LL::GLTF::Asset*`) / `sSkinUboDirty` (key=`LL::GLTF::Skin*`)、std::hash<T*> defaults (forward decl のみで OK) |
| (iii) | `(void)arg` 解除 + key 化 dirty exchange | ✅ 3 関数全件改修済、`find(key)` + `dirty.exchange(false, std::memory_order_acq_rel)` 経由 |
| (iv) | `mUseUBO` runtime gate 配下 | ✅ per-program = entry gate `if (!shader \|\| !shader->mUseUBO) return;` + per-asset/skin/draw = 構造的 gate (= setter 側 mUseUBO 分岐由来、design 06a §5.4 整合) |
| (v) | llrender build PASS + warning 0 | ✅ `make -j4 llrender` = libllrender.a link PASS、ERROR 0 / WARNING 0 (= PC-6ε-2 改変関連) |
| (vi) | TUT 3 件 + codegen 130/130 PASS | ✅ `INTEGRATION_TEST_lluboringbuffer` 11/11 + `INTEGRATION_TEST_llassetubopool` 10/10 + `INTEGRATION_TEST_llpipelinecachestorage` 13/13 YAY 全 PASS + codegen `Ran 130 tests in 0.058s OK` = 130/130 PASS |
| (vii) | MUSEUBO-A + GATE-B 整合 | ✅ MUSEUBO-A = `sDrawUboRingBufferMgr` guard (= helper entry、PC-6δ 既存) + `shader->mUseUBO` entry gate (= per-program) + 構造的 gate (= per-asset/skin/draw) 多重保証。GATE-B = 5 cadence flush 関数本体は Vulkan init 層単独、`mUseUBO` runtime 参照は per-program のみで PC-6α..ε-1 既存形 GATE-B 整合維持 |

---

## §3 PC-6ε-3 entry conditions (= 次 session 着手内容)

### §3.1 scope = per-draw cadence 残 pool 全配線

PC-6δ canary 配置 = `LLDrawPoolSimple::renderDeferred()` 1 件のみ (= `LLVKLoader::flushDrawUbos()` 呼出)。PC-6ε-3 literal = 残 15+ pool subclass 全配線:

- 候補 pool family (= 既存 LLDrawPool* 派生 grep、PC-6ε-3 着手時に正確な count 取得):
  - `LLDrawPoolAlpha` / `LLDrawPoolAvatar` / `LLDrawPoolBump` / `LLDrawPoolGlow` / `LLDrawPoolGroundPlane` / `LLDrawPoolMaterials` / `LLDrawPoolMaterialsLOD` / `LLDrawPoolPBROpaque` / `LLDrawPoolPBRAlpha` / `LLDrawPoolSky` / `LLDrawPoolStars` / `LLDrawPoolTerrain` / `LLDrawPoolTree` / `LLDrawPoolWater` / `LLDrawPoolWLSky` (= 15 family、PC-6ε-3 着手時 trace)
  - `LLDrawPoolSimple` = PC-6δ canary 配置済

### §3.2 design source

- design 06b §2.3 = per-draw cadence update site
- design 06b §4.1 = `flushDrawUbos()` 駆動位置 = draw call 直前 (`LLDrawPool::renderItem()` 等の最深 dispatcher 入口)

### §3.3 PC-6ζ (= PC-6ε-3 後) scope

PC-6ζ literal = setter SAMPLER skip ↔ codegen SINGLETON cadence_tag=5 衝突 正攻法対応 (= design 06a §5.6 + codebase trace `llglslshader.cpp:2480-2563, 3006-3079` 17 setter family)。

---

## §4 self-verify (= 9 観点)

| # | 観点 | record |
|---|---|---|
| 1 | Exit Criteria 7 項全充足 | ✅ §2 表 |
| 2 | UboInstance source doc 整合 | ✅ design 06b §3.2.3 `struct UboInstance { ... std::atomic<bool> dirty{false}; }` 完全一致 (= chapter 07 拡充予定の `vk_buffer` / `mapped_ptr` / `size` member は placeholder comment) |
| 3 | gate 配置 (P1 / G1) 採用根拠 record | ✅ §1.2 + §1.4 で per-program entry gate / per-asset/skin/draw 構造的 gate を明文化 (= AYA 確認 2026-06-04 (P1) + (G1 refined) 採用) |
| 4 | GATE-B 整合 | ✅ §2 (vii)、per-program のみ shader->mUseUBO 参照、他 cadence は init 層単独で動作 (= PC-6α..ε-1 同形) |
| 5 | MUSEUBO-A 整合 | ✅ §2 (vii)、helper entry guard (= sDrawUboRingBufferMgr) + per-program entry gate + per-asset/skin/draw 構造的 gate の多重保証で既存 OpenGL 描画 100% 維持 |
| 6 | llrender build + TUT 3 件 + codegen 130/130 | ✅ §2 (v)(vi) |
| 7 | commit 内容 = 2 modified + 1 new doc + 新 file 0 + CMake 改変 0 + settings.xml 改変 0 | ✅ git diff --stat: llvkloader.cpp + llvkloader.h、本 handoff doc 1 件新規 |
| 8 | `feedback_no_scope_shrink` 遵守 | ✅ PC-6ε-2 literal scope (= per-program / per-asset / per-skin dirty map 配線 + UboInstance::dirty propagation + mUseUBO gate 配下) 完全実施。per-asset/skin の構造的 gate 採用は実装可能形への refinement (= mUseUBO 参照先不在ゆえ entry gate 物理不可、AYA 2026-06-04 確認済) で scope 縮小ではない |
| 9 | `feedback_design_phase_no_code_write` 整合 | ✅ 本 PC-6ε-2 は実装 phase (= 前 PC-6δ' audit interlude 完了 + PC-6ε-1 commit 後)、indra/ 改変 2 件 = 設計 phase ではない |

---

## §5 Phase 1.C 進行 state + 残線形

### §5.1 Phase 1.C marker 状態

- Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅
- PC-0 ✅ / PC-1 ✅ / PC-2 ✅ / PC-3 ✅ / PC-4 ✅ / PC-5 ✅ / PC-6α ✅ / PC-6β ✅ / PC-6γ ✅ / PC-6δ ✅ / PC-6δ' ✅ / PC-6ε-1 ✅ / **PC-6ε-2 ✅ 本 sub-step (= 実装 + verify + handoff doc 完了、commit 未実施 = §7 参照)**
- PC-6ε-3 / PC-6ζ / PC-7 / PC-8 / PC-N ⏳ 次 session 以降

### §5.2 残 strict 線形

PC-6ε-3 (= 本 handoff §3 = per-draw 残 pool 15+ subclass 全配線、design 06b §2.3 + codebase trace) → PC-6ζ (= setter SAMPLER skip 正攻法対応、cadence_tag=5 ↔ SINGLETON 衝突 site = design 06a §5.6) → PC-7 (= `vkCmdBindDescriptorSets` 通電 + dynamic offset 経路 ring buffer chunk hand-off + `UboInstance` member 拡充 (VkBuffer / mapped_ptr / size) + `forwardToUboUpload` 本格化 (= dirty=true 経路有効化)) → PC-8 (= 3 OS build verify、Linux primary + Win/Mac 後段) → PC-N (= Phase 1.C complete marker + Phase 1.D / Phase 2 着手起点)

---

## §6 feedback rule 遵守 record + Co-Authored-By 不在

- `feedback_proactive_handoff` 遵守 (= PC-6ε-3 引継 marker 本 doc)
- `feedback_handoff_minimal_pre_req_read` 遵守 (= §0 必読 3 件 + pinpoint reference 別記)
- `feedback_self_verify_before_handoff` 遵守 (= §4 9 観点 self-verify 全 ✅)
- `feedback_build_only_verified` 遵守 (= llrender build + 3 TUT + codegen で literal 検証取得)
- `feedback_no_scope_shrink` 遵守 (= §4 (8))
- `feedback_doubt_self_first` 遵守 (= UboInstance struct 不在発見 → 解釈 2 案 (P1 / P2) AYA 確認 + (G1) literal の per-asset/skin entry gate 物理不可発見 → refined 形 AYA 再確認 + 両件「OK」受領後実装)
- `feedback_confirm_referent_before_acting` 遵守 (= 同上、2 段階 AYA 確認)
- `feedback_ubo_migration_one_at_a_time` 遵守 (= PC-6ε-2 = per-program / per-asset / per-skin dirty map 配線単独実施、per-draw 残 pool は PC-6ε-3 へ分離、`UboInstance` member 拡充 + `forwardToUboUpload` 本格化は PC-7 へ分離)
- `feedback_design_phase_no_code_write` 整合 (= 本 PC-6ε-2 は実装 phase、§4 (9))
- `feedback_release_branch_workflow` 遵守 (= feature branch `feature/ayastorm-r41-gl-removal` 上 work)
- `feedback_no_auto_commit` 遵守 (= AYA 明示 commit 指示前は本 doc 含め uncommitted)
- `feedback_no_claude_coauthor` 遵守 (= Co-Authored-By 行不在予定)

---

**次 session 着手 1 line**: PC-6ε-3 = per-draw cadence 残 pool 15+ subclass 全配線 (= `LLDrawPool*` 派生から `LLVKLoader::flushDrawUbos()` 呼出、design 06b §2.3 + codebase trace で正確な pool subclass count + 駆動位置 (`LLDrawPool::renderItem()` 等の最深 dispatcher 入口) 確定)。Exit Criteria 詳細は次 session 着手前整理。

---

## §7 次 session 着手前 commit 手順 (= uncommitted state 引継)

### §7.1 本 PC-6ε-2 = uncommitted 状態

本 PC-6ε-2 は実装 + verify + handoff doc 起案まで完了したが、`feedback_no_auto_commit` 遵守で **commit 未実施** のまま session を締めた (AYA 明示指示「commit してください」未受領)。

`git status` (= 本 handoff 起案完了時点):

```
 M indra/llrender/llvkloader.cpp     (+99 / -7 = UboInstance struct + 3 dirty map + 3 flush 関数改修 + shutdown clear + include 追加)
 M indra/llrender/llvkloader.h       (+11 = PC-6ε-2 update 段落追加)
?? docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-6-epsilon-2.md
```

HEAD = `2d1913357d` (= PC-6ε-1 commit)。

### §7.2 次 session 着手手順

1. `git status` で本 handoff doc §7.1 と同じ uncommitted state を確認
2. AYA に PC-6ε-2 commit 指示を確認 (= 本 handoff 内容を commit message body に転記、`feedback_no_claude_coauthor` 遵守で Co-Authored-By 不在)
3. commit 完了 → `git log -1` で commit hash 取得 (= 次 PC-6ε-3 commit chain の前 entry)
4. その後 PC-6ε-3 着手 (= 本 handoff §3 sub-scope)

### §7.3 commit message draft 方針

PC-6α..δ + PC-6δ' + PC-6ε-1 precedent と同形 (= 1 行で long form / 内容 summary / Exit Criteria record / 次 sub 引継 marker / feedback rule 遵守 record / Co-Authored-By 不在)。本 handoff §0-§6 全内容を 1 行 long form に圧縮で良い。
