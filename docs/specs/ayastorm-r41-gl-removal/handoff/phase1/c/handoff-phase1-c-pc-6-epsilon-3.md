# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6ε-3 complete handoff

**作成日**: 2026-06-05
**HEAD (PC-6ε-3 着手前)**: `2ee383f898` (= PC-6ε-2 = per-program / per-asset / per-skin dirty propagation 配線 + `UboInstance` 最小 struct 先行新設 + 3 dirty map + mUseUBO runtime gate 配下)
**完了 marker**: PC-6ε-3 = **per-draw cadence 残 pool 全配線** = `LLDrawPool*` 派生 15 site から `LLVKLoader::flushDrawUbos()` 呼出、PC-6δ canary `LLDrawPoolSimple::renderDeferred()` 1 site と合わせて計 16 pool subclass wired (= deprecated/empty `LLDrawPoolSky` のみ除外)
**次 session 着手 1 line**: PC-6ζ = setter SAMPLER skip ↔ codegen SINGLETON `cadence_tag=5` 衝突 正攻法対応 (= design 06a §5.6、`llglslshader.cpp:2480-2563, 3006-3079` 17 setter family)

---

## §0 必読 3 件 (= minimal pre-req per `feedback_handoff_minimal_pre_req_read`)

1. **本 handoff doc 全文** (= §1-§5)
2. **`docs/specs/ayastorm-r41-gl-removal/design/literal-cross-ref-audit.md`** §3.2 (PC-6ζ scope canonical literals) + §5 (prevention rule 5 件、本 PC-6ε-3 で rule-1 case 1 件適用 = §1.0 参照)
3. **`docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure.md`** §5.6 (setter SAMPLER skip ↔ SINGLETON 衝突) + §3.3 (CadenceTag enum、`SINGLETON=5`)

pinpoint reference (必要時のみ):
- `indra/llrender/llglslshader.cpp:2480-2563` = setter family 第 1 群 (`uniform1i` 系)
- `indra/llrender/llglslshader.cpp:3006-3079` = setter family 第 2 群 (`uniformN[fi]v` 系、SAMPLER skip 衝突 site)
- `indra/newview/lldrawpoolsimple.cpp:33` = PC-6δ canary 配置 (`LLDrawPoolSimple::renderDeferred()`)、PC-6ε-3 追加 5 site は同 file 内 47/124/145/164/192 line
- `indra/newview/lldrawpoolsky.cpp:46-49` = DEPRECATED empty body (= PC-6ε-3 除外根拠、`recordPlaceholderPoolDraw` のみ既配置)

---

## §1 PC-6ε-3 実施内容 (= 11 編集 / +60 / -0 行)

### §1.0 着手前発見 = handoff doc literal drift (= literal-cross-ref-audit rule-1 case)

PC-6ε-2 handoff §3.1 列挙の候補 pool family 名に drift が混入していた (= PC-6 累積章で 5 件目の literal drift case)。grep 結果と整合:

| handoff 列挙 | 実 codebase | 状態 |
|---|---|---|
| `LLDrawPoolSimple` | `LLDrawPoolSimple` | ✅ PC-6δ canary 既配置 |
| `LLDrawPoolAlpha` | `LLDrawPoolAlpha` | ✅ 実在 |
| `LLDrawPoolAvatar` | `LLDrawPoolAvatar` | ✅ 実在 |
| `LLDrawPoolBump` | `LLDrawPoolBump` | ✅ 実在 |
| `LLDrawPoolGlow` | `LLDrawPoolGlow` (= lldrawpoolsimple.cpp 内) | ✅ 実在 |
| `LLDrawPoolGroundPlane` | grep 0 件 | ✗ 不在 = handoff drift |
| `LLDrawPoolMaterials` | `LLDrawPoolMaterials` | ✅ 実在 |
| `LLDrawPoolMaterialsLOD` | grep 0 件 | ✗ 不在 = handoff drift |
| `LLDrawPoolPBROpaque` | `LLDrawPoolGLTFPBR` (= 真名) | ⚠️ rename drift = handoff drift |
| `LLDrawPoolPBRAlpha` | grep 0 件 | ✗ 不在 = handoff drift |
| `LLDrawPoolSky` | `LLDrawPoolSky` (= empty body / DEPRECATED) | ⚠️ 除外 = 実装不能 |
| `LLDrawPoolStars` | grep 0 件 | ✗ 不在 = handoff drift |
| `LLDrawPoolTerrain` | `LLDrawPoolTerrain` | ✅ 実在 |
| `LLDrawPoolTree` | `LLDrawPoolTree` | ✅ 実在 |
| `LLDrawPoolWater` | `LLDrawPoolWater` | ✅ 実在 |
| `LLDrawPoolWLSky` | `LLDrawPoolWLSky` | ✅ 実在 |
| (handoff 列挙漏れ) | `LLDrawPoolAlphaMask` (= lldrawpoolsimple.cpp 内) | ✅ 実在 = handoff omission |
| (handoff 列挙漏れ) | `LLDrawPoolFullbright` (= lldrawpoolsimple.cpp 内) | ✅ 実在 = handoff omission |
| (handoff 列挙漏れ) | `LLDrawPoolFullbrightAlphaMask` (= lldrawpoolsimple.cpp 内) | ✅ 実在 = handoff omission |
| (handoff 列挙漏れ) | `LLDrawPoolGrass` (= lldrawpoolsimple.cpp 内) | ✅ 実在 = handoff omission |
| (handoff 列挙漏れ) | `LLDrawPoolWaterExclusion` | ✅ 実在 = handoff omission |

drift 修正報告 → AYA literal「OK」(2026-06-05) で 15 subclass scope 確定 (= 実在 16 - Sky deprecated 1 = 15 + PC-6δ canary Simple = 16 wired)。

### §1.1 配置 pattern (= AYA (B') 採用 2026-06-04)

3 案提示 = (A) 1 pool 1 site / (B') 1 pool 1 site (= renderDeferred / renderPostDeferred / render どれか 1 つ、Avatar の delegate 構造除外) / (B) 1 method 1 site (= renderDeferred + renderPostDeferred 両方) / (C) 最深 dispatcher (= renderItem) で再帰的全数。AYA literal「(B') OK」(2026-06-05) で確定。

配置 rule:
- `LL_PROFILE_ZONE_SCOPED_*` directive 直後 (= method 入口、design 06b §4.1「最深 dispatcher 入口」厳密解釈)
- `LLDrawPoolGLTFPBR` は `LL_PROFILE_ZONE` 不在のため `llassert(...)` 直後に配置
- `LLDrawPoolAvatar::renderDeferred()` は body が `render(pass);` 1 行 delegate なので `LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;` 直後に配置 (PC-6δ canary の Simple と同形)
- 早期 return guard 配置前で問題なし (= `flushDrawUbos()` は `dirty.exchange(false)` で no-op、cheap)

統一 tag block:

```cpp
    // <AYAstorm r41 PC-6ε-3> per-draw cadence flush (design 06b §4.1、AYA (B') 採用 2026-06-04 = 1 pool 1 site)
    LLVKLoader::flushDrawUbos();
    // </AYAstorm r41 PC-6ε-3>
```

### §1.2 編集対象 11 file / 15 site

| # | file | class::method | 配置 line (改修後) | 配置位置 |
|---|---|---|---|---|
| 1 | `indra/newview/lldrawpoolsimple.cpp` | `LLDrawPoolGlow::renderPostDeferred` | 47 直後 | `LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL` 直後 |
| 2 | `indra/newview/lldrawpoolsimple.cpp` | `LLDrawPoolAlphaMask::renderDeferred` | 124 直後 | `LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL` 直後 |
| 3 | `indra/newview/lldrawpoolsimple.cpp` | `LLDrawPoolGrass::renderDeferred` | 145 直後 | `LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL` 直後 |
| 4 | `indra/newview/lldrawpoolsimple.cpp` | `LLDrawPoolFullbright::renderPostDeferred` | 164 直後 | `LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL` 直後 |
| 5 | `indra/newview/lldrawpoolsimple.cpp` | `LLDrawPoolFullbrightAlphaMask::renderPostDeferred` | 192 直後 | `LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL` 直後 |
| 6 | `indra/newview/lldrawpoolbump.cpp` | `LLDrawPoolBump::renderDeferred` | 548 直後 | `LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL` 直後 |
| 7 | `indra/newview/lldrawpoolterrain.cpp` | `LLDrawPoolTerrain::renderDeferred` | 151 直後 | `LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL` 直後 |
| 8 | `indra/newview/lldrawpoolwlsky.cpp` | `LLDrawPoolWLSky::renderDeferred` | 475 直後 | `LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL` 直後 |
| 9 | `indra/newview/lldrawpooltree.cpp` | `LLDrawPoolTree::renderDeferred` | 68 直後 | `LL_PROFILE_ZONE_SCOPED` 直後 |
| 10 | `indra/newview/lldrawpoolavatar.cpp` | `LLDrawPoolAvatar::renderDeferred` | 228 直後 | `LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR` 直後、`render(pass);` 直前 |
| 11 | `indra/newview/lldrawpoolwater.cpp` | `LLDrawPoolWater::renderPostDeferred` | 146 直後 | `LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL` 直後 |
| 12 | `indra/newview/lldrawpoolwaterexclusion.cpp` | `LLDrawPoolWaterExclusion::render` | 46 直後 | `LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL` 直後 |
| 13 | `indra/newview/lldrawpoolpbropaque.cpp` | `LLDrawPoolGLTFPBR::renderDeferred` | 57 直後 | `llassert(!...isHUDPass())` 直後 (= `LL_PROFILE_ZONE_*` 不在) |
| 14 | `indra/newview/lldrawpoolmaterials.cpp` | `LLDrawPoolMaterials::renderDeferred` | 108 直後 | `LL_PROFILE_ZONE_SCOPED_CATEGORY_MATERIAL` 直後 |
| 15 | `indra/newview/lldrawpoolalpha.cpp` | `LLDrawPoolAlpha::renderPostDeferred` | 147 直後 | `LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL` 直後 |

各 site は同一 tag block (= §1.1) を挿入、tag block 内 `LLVKLoader::flushDrawUbos();` 呼出 1 行。各 file は `#include "llvkloader.h"` を既に含む (= PC-6δ で sub-step 3.4-δ-2 placeholder pool draw helper 経由で全 lldrawpool*.cpp に追加済、grep 確認済)。

### §1.3 LLDrawPoolSky 除外根拠 (= 実装不能、scope shrink ではない)

`indra/newview/lldrawpoolsky.cpp:33-49` = `LLDrawPoolSky::render(S32 pass)` 本体は完全 empty (= comment のみ「DEPRECATED」明示)。`recordPoolDraws` (= line 64) には PC-6δ canary `recordPlaceholderPoolDraw` 既配置。draw call site 不在ゆえ `flushDrawUbos()` 配置先無し = 実装不能。`feedback_no_scope_shrink` 整合 (= AYA 2026-06-05 確認受領「OK」)。

---

## §2 Exit Criteria 7 項全充足

| # | 項目 | 検証 record |
|---|---|---|
| (i) | 15 site 全件で `LLVKLoader::flushDrawUbos()` 配置 + 統一 tag block | ✅ `grep "AYAstorm r41 PC-6ε-3" indra/newview/` = 30 件 (15 site × 2 tag = open + close)、11 file へ分散、§1.2 表完一致 |
| (ii) | 配置位置 = design 06b §4.1「最深 dispatcher 入口」整合 | ✅ 全 site `LL_PROFILE_ZONE_SCOPED_*` directive 直後 (= method 入口、`render` / `renderDeferred` / `renderPostDeferred` のいずれか 1 つで AYA (B') 採用)、Avatar / GLTFPBR 特殊 case は §1.2 注記参照 |
| (iii) | `#include "llvkloader.h"` 11 file 全件確保 | ✅ 全 file PC-6δ で既追加済 (= sub-step 3.4-δ-2 placeholder pool draw helper 経由)、PC-6ε-3 新規 include 追加 0 件 |
| (iv) | llrender build PASS + warning 0 | ✅ `make -j4 llrender` exit 0、ERROR 0 / WARNING 0 (= PC-6ε-3 改変関連)、libllrender.a 既存 link 状態維持 |
| (v) | newview drawpool TU 11 件 compile PASS + warning 0 | ✅ `make -j4 -f build.make` 11 件 `.cpp.o` 全件 rebuild exit 0、ERROR 0 / WARNING 0 |
| (vi) | TUT 3 件 + codegen 130/130 PASS | ✅ `INTEGRATION_TEST_lluboringbuffer` 11/11 + `INTEGRATION_TEST_llassetubopool` 10/10 + `INTEGRATION_TEST_llpipelinecachestorage` 13/13 YAY 全 PASS + codegen `Ran 130 tests in 0.057s OK` = 130/130 PASS |
| (vii) | MUSEUBO-A + GATE-B 整合 | ✅ MUSEUBO-A = `flushDrawUbos` body は PC-6ε-2 で per-draw が「構造的 gate のみ (= setter 側 mUseUBO 分岐由来)」確定 + helper entry guard `if (!sDrawUboRingBufferMgr) return;` で Vulkan 未初期化時即 return、mUseUBO=false default で既存 OpenGL 描画 100% 維持。GATE-B = `flushDrawUbos` 本体は Vulkan init 層単独、`mUseUBO` runtime 参照無し (= PC-6α..ε-2 同形 GATE-B 整合維持) |

---

## §3 PC-6ζ entry conditions (= 次 session 着手内容)

### §3.1 scope = setter SAMPLER skip ↔ codegen SINGLETON 衝突 正攻法対応

PC-2 で `bringupTestUBO` の `Global_ReflectionProbes` を `cadence_tag=SINGLETON=5` 確定後、setter 群 (= `LLGLSLShader::uniform1i` 等 17 family) は SAMPLER bind 経路で UBO 経路を skip するが、`cadence_tag=5` SINGLETON UBO 経由の uniform は本来 SAMPLER bind 不要であり、現状 skip rule が衝突。

design source:
- design 06a §5.6 = setter SAMPLER skip 仕様 (= UBO 化対象 uniform でも SAMPLER bind 経路を例外的に skip)
- design 06a §3.3 = `CadenceTag` enum (= `SINGLETON=5`、codegen `main.py:63 CADENCE_SINGLETON=5`)
- codebase trace = `llglslshader.cpp:2480-2563` (= uniform1i 系第 1 群) + `llglslshader.cpp:3006-3079` (= uniformN[fi]v 系第 2 群、SAMPLER skip 衝突 site)

### §3.2 PC-7 (= PC-6ζ 後) scope

PC-7 literal = `vkCmdBindDescriptorSets` 通電 + dynamic offset 経路 ring buffer chunk hand-off + `UboInstance` member 拡充 (= `VkBuffer vk_buffer` / `void* mapped_ptr` / `uint32_t size`、PC-6ε-2 §1.2 placeholder comment) + `forwardToUboUpload` 本格化 (= dirty=true 経路有効化)。

---

## §4 self-verify (= 9 観点)

| # | 観点 | record |
|---|---|---|
| 1 | Exit Criteria 7 項全充足 | ✅ §2 表 |
| 2 | 15 site source doc 整合 | ✅ design 06b §2.3 (per-draw cadence update site = `LLDrawPool` 派生) + §4.1 (`flushDrawUbos()` 駆動位置 = draw call 直前 / 最深 dispatcher 入口) を全 tag block で literal 引用、(B') 採用根拠を AYA 確認 record で明文化 |
| 3 | (B') + Sky 除外 採用根拠 record | ✅ §1.1 で (A)/(B')/(B)/(C) 4 案比較 + AYA literal「(B') OK」(2026-06-05) record、§1.3 で Sky empty body / DEPRECATED + recordPlaceholderPoolDraw 既配置を実装不能根拠として明示 |
| 4 | GATE-B 整合 | ✅ §2 (vii)、`flushDrawUbos` 本体は Vulkan init 層単独、`mUseUBO` runtime 参照無し (= PC-6α..ε-2 同形) |
| 5 | MUSEUBO-A 整合 | ✅ §2 (vii)、helper entry guard (= `sDrawUboRingBufferMgr`) + 構造的 gate (= setter 側 mUseUBO 分岐由来、PC-6ε-2 確定) の多重保証で既存 OpenGL 描画 100% 維持 |
| 6 | llrender + newview build + TUT 3 件 + codegen 130/130 | ✅ §2 (iv)(v)(vi)、build verify 全件 PASS |
| 7 | commit 内容 = 11 modified + 1 new doc + 新 file 0 + CMake 改変 0 + settings.xml 改変 0 | ✅ git diff --stat: 11 lldrawpool*.cpp + 本 handoff doc 1 件新規、CMakeLists.txt 改変なし (= include 既存) |
| 8 | `feedback_no_scope_shrink` 遵守 | ✅ PC-6ε-3 literal scope (= per-draw 残 pool 全配線) 完全実施。Sky 除外は scope 縮小ではなく実装不能 (= empty body + DEPRECATED、§1.3)。handoff drift 5 件 (`GroundPlane` / `MaterialsLOD` / `PBROpaque rename` / `PBRAlpha` / `Stars`) は実 codebase に不在で配線不可能、handoff 列挙漏れ 5 件 (`AlphaMask` / `Fullbright` / `FullbrightAlphaMask` / `Grass` / `WaterExclusion`) は本 PC-6ε-3 で補完済 |
| 9 | `feedback_design_phase_no_code_write` 整合 | ✅ 本 PC-6ε-3 は実装 phase (= PC-6ε-2 commit 後)、indra/newview/ 改変 11 件 = 設計 phase ではない |

---

## §5 Phase 1.C 進行 state + 残線形

### §5.1 Phase 1.C marker 状態

- Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅
- PC-0 ✅ / PC-1 ✅ / PC-2 ✅ / PC-3 ✅ / PC-4 ✅ / PC-5 ✅ / PC-6α ✅ / PC-6β ✅ / PC-6γ ✅ / PC-6δ ✅ / PC-6δ' ✅ / PC-6ε-1 ✅ / PC-6ε-2 ✅ / **PC-6ε-3 ✅ 本 sub-step (= 実装 + verify + handoff doc 完了、commit 未実施 = §7 参照)**
- PC-6ζ / PC-7 / PC-8 / PC-N ⏳ 次 session 以降

### §5.2 残 strict 線形

PC-6ζ (= 本 handoff §3.1 = setter SAMPLER skip ↔ codegen SINGLETON `cadence_tag=5` 衝突 正攻法対応、design 06a §5.6 + codebase trace `llglslshader.cpp:2480-2563, 3006-3079` 17 setter family) → PC-7 (= `vkCmdBindDescriptorSets` 通電 + dynamic offset 経路 ring buffer chunk hand-off + `UboInstance` member 拡充 + `forwardToUboUpload` 本格化) → PC-8 (= 3 OS build verify、Linux primary + Win/Mac 後段) → PC-N (= Phase 1.C complete marker + Phase 1.D / Phase 2 着手起点)

---

## §6 feedback rule 遵守 record + Co-Authored-By 不在

- `feedback_proactive_handoff` 遵守 (= PC-6ζ 引継 marker 本 doc)
- `feedback_handoff_minimal_pre_req_read` 遵守 (= §0 必読 3 件 + pinpoint reference 別記)
- `feedback_self_verify_before_handoff` 遵守 (= §4 9 観点 self-verify 全 ✅)
- `feedback_build_only_verified` 遵守 (= llrender + 11 newview TU + 3 TUT + codegen 130/130 で literal 検証取得)
- `feedback_no_scope_shrink` 遵守 (= §4 (8)、handoff drift 5 件 + 列挙漏れ 5 件補完 + Sky 実装不能 record)
- `feedback_doubt_self_first` 遵守 (= handoff doc literal drift 5 件発見で停止 + grep 確認 + AYA 報告 + 確定後実装)
- `feedback_confirm_referent_before_acting` 遵守 (= scope drift / 配置 granularity / Exit Criteria 3 件 batch AYA 確認、literal「OK」受領後実装)
- `feedback_ubo_migration_one_at_a_time` 遵守 (= PC-6ε-3 = per-draw 残 pool 配線単独実施、setter SAMPLER skip は PC-6ζ へ分離、UboInstance member 拡充 + forwardToUboUpload 本格化は PC-7 へ分離)
- `feedback_design_phase_no_code_write` 整合 (= 本 PC-6ε-3 は実装 phase、§4 (9))
- `feedback_release_branch_workflow` 遵守 (= feature branch `feature/ayastorm-r41-gl-removal` 上 work)
- `feedback_no_auto_commit` 遵守 (= AYA 明示 commit 指示前は本 doc 含め uncommitted)
- `feedback_no_claude_coauthor` 遵守 (= Co-Authored-By 行不在予定)

---

**次 session 着手 1 line**: PC-6ζ = setter SAMPLER skip ↔ codegen SINGLETON `cadence_tag=5` 衝突 正攻法対応 (= design 06a §5.6、`llglslshader.cpp:2480-2563, 3006-3079` 17 setter family、codebase trace で正確な setter 数 + 衝突 site 確定)。Exit Criteria 詳細は次 session 着手前整理。

---

## §7 次 session 着手前 commit 手順 (= uncommitted state 引継)

### §7.1 本 PC-6ε-3 = uncommitted 状態

本 PC-6ε-3 は実装 + verify + handoff doc 起案まで完了したが、`feedback_no_auto_commit` 遵守で **commit 未実施** のまま session を締めた (AYA 明示指示「commit してください」未受領)。

`git status` (= 本 handoff 起案完了時点):

```
 M indra/newview/lldrawpoolalpha.cpp           (+4 = PC-6ε-3 tag block 1 site)
 M indra/newview/lldrawpoolavatar.cpp          (+4 = PC-6ε-3 tag block 1 site)
 M indra/newview/lldrawpoolbump.cpp            (+4 = PC-6ε-3 tag block 1 site)
 M indra/newview/lldrawpoolmaterials.cpp       (+4 = PC-6ε-3 tag block 1 site)
 M indra/newview/lldrawpoolpbropaque.cpp       (+4 = PC-6ε-3 tag block 1 site)
 M indra/newview/lldrawpoolsimple.cpp          (+20 = PC-6ε-3 tag block 5 site)
 M indra/newview/lldrawpoolterrain.cpp         (+4 = PC-6ε-3 tag block 1 site)
 M indra/newview/lldrawpooltree.cpp            (+4 = PC-6ε-3 tag block 1 site)
 M indra/newview/lldrawpoolwater.cpp           (+4 = PC-6ε-3 tag block 1 site)
 M indra/newview/lldrawpoolwaterexclusion.cpp  (+4 = PC-6ε-3 tag block 1 site)
 M indra/newview/lldrawpoolwlsky.cpp           (+4 = PC-6ε-3 tag block 1 site)
?? docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-6-epsilon-3.md
```

HEAD = `2ee383f898` (= PC-6ε-2 commit)。

### §7.2 次 session 着手手順

1. `git status` で本 handoff §7.1 と同じ uncommitted state を確認
2. AYA に PC-6ε-3 commit 指示を確認 (= 本 handoff 内容を commit message body に転記、`feedback_no_claude_coauthor` 遵守で Co-Authored-By 不在)
3. commit 完了 → `git log -1` で commit hash 取得 (= 次 PC-6ζ commit chain の前 entry)
4. その後 PC-6ζ 着手 (= 本 handoff §3.1 sub-scope、setter SAMPLER skip 正攻法対応)

### §7.3 commit message draft 方針

PC-6α..δ + PC-6δ' + PC-6ε-1 + PC-6ε-2 precedent と同形 (= 1 行で long form / 内容 summary / Exit Criteria record / 次 sub 引継 marker / feedback rule 遵守 record / Co-Authored-By 不在)。本 handoff §0-§6 全内容を 1 行 long form に圧縮で良い。
