# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6ε-1 complete handoff

**作成日**: 2026-06-04
**HEAD (PC-6ε-1 着手前)**: `189718bef4` (= PC-6δ' = audit interlude / doc only)
**完了 marker**: PC-6ε-1 = `flushSingletonUbos()` 新設 + `bringupTestUBO()` 経由 singleton cadence flush 経路本格置換 + 6 cadence 関数体系成立 (= 5 cadence + singleton)
**次 session 着手 1 line**: PC-6ε-2 = per-program / per-asset / per-skin dirty map 配線 (= `mUseUBO` runtime gate 配下で dirty propagation)

---

## §0 必読 3 件 (= minimal pre-req per `feedback_handoff_minimal_pre_req_read`)

1. **本 handoff doc 全文** (= §1-§5)
2. **`docs/specs/ayastorm-r41-gl-removal/design/literal-cross-ref-audit.md`** §3.1 + §3.2 (= PC-6ε-2 / PC-6ζ pre-cache literals、drift 予防)
3. **`docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md`** §3.2.3 (= `UboInstance::dirty` `std::atomic<bool>`) + §5.3 (= `mUseUBO` runtime gate + dirty propagation) + §5.4 (= per-cadence dirty propagation)

pinpoint reference (必要時のみ):
- `indra/llrender/llvkloader.h:326-353` = PC-6δ + PC-6ε-1 で確定した 6 flush 関数宣言
- `indra/llrender/llvkloader.cpp:3169-3220` = 6 flush 関数実装 + `flushDummyUboWrite` 共通 helper
- `indra/llrender/llglslshader.cpp:2041-2065` = `bringupTestUBO()` PC-6ε-1 改修後本体
- design 06a §5.6 = setter SAMPLER skip path (= cadence_tag=5 ↔ SINGLETON 衝突 site、PC-6ζ scope)

---

## §1 PC-6ε-1 実施内容 (= 3 編集 / +78 / -37 行)

### §1.1 編集 1 = `indra/llrender/llvkloader.h` (+25 行)

`flushSkinUbos(LL::GLTF::Skin*)` 宣言直後に PC-6ε-1 tag block + `void flushSingletonUbos();` 宣言追加 (= 既存 5 件と並列、第 6 cadence)。

設計根拠を file-level comment に明示:
- design 02 §3 = `Global_` prefix singleton cadence 明示分類
- design 06c §2.2 = `Global_ReflectionProbes` singleton 配置例
- design 06a §3.3 = `CadenceTag` enum 値域に singleton 含む

MUSEUBO-A / GATE-B 整合方針も comment 内明記。

### §1.2 編集 2 = `indra/llrender/llvkloader.cpp` (+19 行)

`flushSkinUbos(LL::GLTF::Skin*)` 実装直後に PC-6ε-1 tag block + `LLVKLoader::flushSingletonUbos()` 実装追加:

```cpp
void flushSingletonUbos()
{
    flushDummyUboWrite("flushSingletonUbos");
}
```

PC-6δ helper pattern 完全継承 (= MUSEUBO-A guard / 256 B `allocate` / side-table lookup / `memset 0` / first-fire LL_INFOS marker は helper entry に集約済)。`per-frame` cadence の `beginFrame()` 呼出は singleton では不要 (= per-frame 跨ぎ持続、design 06b §2.2 整合)。

### §1.3 編集 3 = `indra/llrender/llglslshader.cpp` (= bringupTestUBO 改修、+34 / -37 行)

(α) 採用 (AYA 2026-06-04 確定) = **PC-1 contract assertions は保持、`UniformLocation` + `forwardToUboUpload` + `s_test_ubo_dirty` shell flag のみ `LLVKLoader::flushSingletonUbos()` で置換**。

**保持** (= layered safety / codegen drift detection):
- `ubo::lookup_block("Global_ReflectionProbes")` + nullptr guard + LL_WARNS
- `llassert(block->block_hash == 0xabdfdb31u)` + 4 件の追加 contract assert (size / descriptor_set / binding / cadence_tag)

**置換**:
- 旧: `const ubo::UniformLocation loc{...}; forwardToUboUpload(loc, s_dummy, ubo::Global_ReflectionProbes_SIZE); static bool s_test_ubo_dirty = false; (void)s_test_ubo_dirty;`
- 新: `LLVKLoader::flushSingletonUbos();`

`<AYAstorm r41 PC-6ε-1>` tag block + design 02 §3 / 06c §2.2 引用 comment 配置。

関数 header doc は PC-2 単独表記 → PC-2 (起案) / PC-6ε-1 (本格化) の二段構え運用に改稿。

---

## §2 Exit Criteria 7 項全充足

| # | 項目 | 検証 record |
|---|---|---|
| (i) | 6 cadence 関数体系成立 | ✅ `flushFrameUbos` + `flushProgramUbos` + `flushDrawUbos` + `flushAssetUbos` + `flushSkinUbos` + **`flushSingletonUbos`** = 6 件 (llvkloader.h:326-353)、design 02 §3 6 cadence 体系整合 |
| (ii) | `bringupTestUBO` 経路 singleton flush PASS | ✅ 静的 verify: bringupTestUBO (llglslshader.cpp:2041) → LLVKLoader::flushSingletonUbos() (line 2061) → flushDummyUboWrite("flushSingletonUbos") delegate、first-fire LL_INFOS marker 既配置 (PC-6δ helper)。runtime LL_INFOS 確認は `mUseUBO=true` + Vulkan init 環境で AYA viewer run 時 (= PC-6δ と同レベル) |
| (iii) | llrender build PASS + warning 0 | ✅ `make -j4 llrender` = libllrender.a link PASS、ERROR 0 / WARNING 0 |
| (iv) | TUT 3 件 regression なし | ✅ `INTEGRATION_TEST_llassetubopool` 10/10 + `INTEGRATION_TEST_lluboringbuffer` 11/11 + `INTEGRATION_TEST_llpipelinecachestorage` 13/13 YAY 全 PASS |
| (v) | codegen unittest regression なし | ✅ 130 tests / 0.062s / OK = 130/130 PASS |
| (vi) | MUSEUBO-A 整合 | ✅ **二重 gate**: (1) bringupTestUBO 自体 `if (mUseUBO)` (llglslshader.cpp:1987) で gated、mUseUBO=false default で実走せず。(2) flushDummyUboWrite helper entry `if (!sDrawUboRingBufferMgr) return;` で Vulkan 未初期化時即 return。GL 単独動作 path 100% 維持 |
| (vii) | GATE-B 整合 | ✅ `flushSingletonUbos` / `flushDummyUboWrite` は mUseUBO runtime gate 未参照 (= PC-6α..δ 同形)、Vulkan initialization 層単独で flush 駆動。bringupTestUBO 側 mUseUBO gate は redirect 層側 (= 既存) で別途整備 |

---

## §3 PC-6ε-2 entry conditions (= 次 session 着手内容)

### §3.1 scope = per-program / per-asset / per-skin dirty map 配線

PC-6δ' audit doc §3.1 + PC-6δ handoff §10 由来。PC-6ε-2 literal:

- per-program dirty map = `flushProgramUbos(LLGLSLShader*)` の `shader` 引数を key 化して dirty propagation
- per-asset dirty map = `flushAssetUbos(LL::GLTF::Asset*)` の `asset` 引数を key 化
- per-skin dirty map = `flushSkinUbos(LL::GLTF::Skin*)` の `skin` 引数を key 化
- gate 配置 = `mUseUBO` runtime gate 配下 (= GATE-B 配置とは別、redirect 層 dirty propagation 層で gate)

### §3.2 design source

- design 06b §3.2.3 = `UboInstance` struct 内 `dirty` flag (`std::atomic<bool>`)
- design 06b §5.3 = `mUseUBO` runtime gate と dirty propagation の関係
- design 06b §5.4 = per-cadence dirty propagation 規律

### §3.3 PC-6ε-3 (= PC-6ε-2 後) scope

PC-6ε-3 literal = per-draw cadence 残 pool 全配線 (= PC-6δ canary `LLDrawPoolSimple` 以外 15+ pool subclass)。design 06b §2.3 + codebase trace 必要。

---

## §4 self-verify (= 9 観点)

| # | 観点 | record |
|---|---|---|
| 1 | Exit Criteria 7 項全充足 | ✅ §2 表 |
| 2 | flushSingletonUbos source doc 整合 | ✅ design 02 §3 + 06c §2.2 + 06a §3.3 引用 (llvkloader.h:329-352 + llvkloader.cpp 内 comment) |
| 3 | (α) 採用根拠 record | ✅ §1.3 で PC-1 contract assertions 保持 = layered safety 維持 |
| 4 | GATE-B 整合 | ✅ §2 (vii) = mUseUBO runtime gate 不依存 Vulkan init 層 (= PC-6α..δ 同形) |
| 5 | MUSEUBO-A 整合 | ✅ §2 (vi) = 二重 gate (mUseUBO + sDrawUboRingBufferMgr)、既存 OpenGL 描画 100% 維持 |
| 6 | llrender build + TUT 3 件 + codegen 130/130 | ✅ §2 (iii)(iv)(v) |
| 7 | commit 内容 = 3 modified + 1 new doc + 新 file 0 + CMake 改変 0 + settings.xml 改変 0 | ✅ git diff --stat: llglslshader.cpp / llvkloader.cpp / llvkloader.h、本 handoff doc 1 件新規 |
| 8 | `feedback_no_scope_shrink` 遵守 | ✅ audit doc §4.3 確定 sub-scope 完全実施 (= header decl / cpp 実装 / bringupTestUBO 置換 / 5 cadence touch せず) |
| 9 | `feedback_design_phase_no_code_write` 整合 | ✅ 本 PC-6ε-1 は実装 phase (= 前 PC-6δ' audit interlude 完了後)、`indra/` 改変 3 件 = 設計 phase ではない |

---

## §5 Phase 1.C 進行 state + 残線形

### §5.1 Phase 1.C marker 状態

- Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅
- PC-0 ✅ / PC-1 ✅ / PC-2 ✅ / PC-3 ✅ / PC-4 ✅ / PC-5 ✅ / PC-6α ✅ / PC-6β ✅ / PC-6γ ✅ / PC-6δ ✅ / PC-6δ' ✅ / **PC-6ε-1 ✅ 本 sub-step (= 実装 + verify + handoff doc 完了、commit 未実施 = §7 参照)**
- PC-6ε-2 / PC-6ε-3 / PC-6ζ / PC-7 / PC-8 / PC-N ⏳ 次 session 以降

### §5.2 残 strict 線形

PC-6ε-2 (= 本 handoff §3 = per-program/asset/skin dirty map 配線、mUseUBO gate 配下) → PC-6ε-3 (= per-draw 残 pool 全配線、LLDrawPoolSimple 以外 15+ pool subclass) → PC-6ζ (= SAMPLER skip 正攻法対応、cadence_tag=5 衝突 site) → PC-7 (= `vkCmdBindDescriptorSets` 通電 + dynamic offset 経路 ring buffer chunk hand-off) → PC-8 (= 3 OS build verify、Linux primary + Win/Mac 後段) → PC-N (= Phase 1.C complete marker + Phase 1.D / Phase 2 着手起点)

---

## §6 feedback rule 遵守 record + Co-Authored-By 不在

- `feedback_proactive_handoff` 遵守 (= PC-6ε-2 引継 marker 本 doc)
- `feedback_handoff_minimal_pre_req_read` 遵守 (= §0 必読 3 件 + pinpoint reference 別記)
- `feedback_self_verify_before_handoff` 遵守 (= §4 9 観点 self-verify 全 ✅)
- `feedback_build_only_verified` 遵守 (= llrender build + 3 TUT + codegen で literal 検証取得)
- `feedback_no_scope_shrink` 遵守 (= audit doc §4.3 確定 sub-scope 完全実施)
- `feedback_doubt_self_first` 遵守 (= bringupTestUBO 改修粒度 (α)/(β) で AYA 確認取得、layered safety 保持決定)
- `feedback_confirm_referent_before_acting` 遵守 (= 同上、literal 解釈ambiguity を AYA 確認で解消)
- `feedback_ubo_migration_one_at_a_time` 遵守 (= PC-6ε-1 = singleton cadence 単独実施、per-program / per-asset / per-skin dirty map は PC-6ε-2 へ分離)
- `feedback_design_phase_no_code_write` 整合 (= 本 PC-6ε-1 は実装 phase、§4 (9) 参照)
- `feedback_release_branch_workflow` 遵守 (= feature branch `feature/ayastorm-r41-gl-removal` 上 work)
- `feedback_no_auto_commit` 遵守 (= AYA 明示 commit 指示前は本 doc 含め uncommitted)
- `feedback_no_claude_coauthor` 遵守 (= Co-Authored-By 行不在予定)

---

**次 session 着手 1 line**: PC-6ε-2 = per-program / per-asset / per-skin dirty map 配線 (= `flushProgramUbos` / `flushAssetUbos` / `flushSkinUbos` 引数 key 化 + `UboInstance::dirty` `std::atomic<bool>` propagation + `mUseUBO` runtime gate 配下)。Exit Criteria 詳細は design 06b §3.2.3 / §5.3 / §5.4 由来で次 session 着手前整理。

---

## §7 次 session 着手前 commit 手順 (= uncommitted state 引継)

### §7.1 本 PC-6ε-1 = uncommitted 状態

本 PC-6ε-1 は実装 + verify + handoff doc 起案まで完了したが、`feedback_no_auto_commit` 遵守で **commit 未実施** のまま session を締めた (AYA 明示指示「commit してください」未受領)。

`git status` (= 本 handoff 起案完了時点):

```
 M indra/llrender/llglslshader.cpp     (+34 / -37 = bringupTestUBO 改修)
 M indra/llrender/llvkloader.cpp       (+19 = flushSingletonUbos 実装)
 M indra/llrender/llvkloader.h         (+25 = flushSingletonUbos 宣言)
?? docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-6-epsilon-1.md
```

HEAD = `189718bef4` (= PC-6δ' audit interlude commit)。

### §7.2 次 session 着手手順

1. `git status` で本 handoff doc §7.1 と同じ uncommitted state を確認
2. AYA に PC-6ε-1 commit 指示を確認 (= 本 handoff 内容を commit message body に転記、`feedback_no_claude_coauthor` 遵守で Co-Authored-By 不在)
3. commit 完了 → `git log -1` で commit hash 取得 (= 次 PC-6ε-2 commit chain の前 entry)
4. その後 PC-6ε-2 着手 (= 本 handoff §3 sub-scope)

### §7.3 commit message draft 方針

PC-6α..δ + PC-6δ' precedent と同形 (= 1 行で long form / 内容 summary / Exit Criteria record / 次 sub 引継 marker / feedback rule 遵守 record / Co-Authored-By 不在)。本 handoff §0-§6 全内容を 1 行 long form に圧縮で良い。
