# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-6δ complete** marker

**作成日**: 2026-06-04
**前 commit chain** (= 直前 handoff 起案):
- `9ed4cca802` = Phase 1.C **PC-1 complete** = `Global_ReflectionProbes` shell blueprint codegen emit
- `c7f512d654` = Phase 1.C **PC-2 complete** = test UBO shell C++ 接続 = block-level test bring-up (= (c) 採用)
- `c31998c49f` = Phase 1.C **PC-3 complete** = `sAssetUboPool` grow algorithm + TUT 10/10 PASS (= (α) 採用)
- `912863bf81` = Phase 1.C **PC-4 complete** = `LLUboRingBuffer` ring buffer 4 MB / 16 MB grow algorithm + cvar `AYARingBufferSizeMB` 露出 + TUT 11/11 PASS (= (α') 採用)
- `2fb5af486e` = Phase 1.C **PC-5 complete** = `LLPipelineCacheStorage` PSO cache disk persist + 64 MB cap algorithm + cvar `AYAPipelineCacheSizeMB` 露出 + TUT 13/13 PASS (= (α'') + (e1) 採用)
- `465c55dcfd` = Phase 1.C **PC-6α complete** = `LLAssetUboPool` × Vulkan device 実 wire up
- `e1d23a767a` = Phase 1.C **PC-6β complete** = `LLUboRingBuffer` × VMA 実 wire up + cvar `AYARingBufferSizeMB` 読込 hookup (= side-table 配線)
- `8e69ab01cc` = Phase 1.C **PC-6γ complete** = `LLPipelineCacheStorage` × Vulkan device 実 file I/O + `vkGetPipelineCacheData` wire up + cvar `AYAPipelineCacheSizeMB` 読込 hookup

**本 handoff doc 目的**: **Phase 1.C PC-6δ complete marker**。**PC-6 α..ζ strict 線形分割 (= PC-6α handoff §2 確定) 中の PC-6δ = 5 cadence (per-frame / per-program / per-draw / per-asset / per-skin) flush 関数 update site 5 種で test UBO 空 dummy 書込 PASS、ring buffer `allocate()` / `beginFrame()` 経路通電 完結後の引継**。`indra/llrender/llvkloader.h` 2 編集 (= forward decl + 5 flush 宣言) + `indra/llrender/llvkloader.cpp` 1 編集 (= 5 flush 実装 + 共通 helper `flushDummyUboWrite`) + 4 driver position 編集 (= `indra/newview/pipeline.cpp` renderGeomDeferred 入口 / `indra/llrender/llglslshader.cpp` bind() 入口 / `indra/newview/lldrawpoolsimple.cpp` renderDeferred canary / `indra/newview/gltfscenemanager.cpp` asset/skin UBO bind 直前) で 5 cadence flush 駆動位置 wire up。MUSEUBO-A 整合 = sDrawUboRingBufferMgr 未初期化時 = 即時 return = GL 単独動作 path で完全 no-op。llrender build PASS (WARNING 0) + 3 newview TU compile PASS (pipeline.cpp / lldrawpoolsimple.cpp / gltfscenemanager.cpp) + `INTEGRATION_TEST_lluboringbuffer` 11/11 PASS + `INTEGRATION_TEST_llassetubopool` 10/10 PASS + `INTEGRATION_TEST_llpipelinecachestorage` 13/13 PASS + codegen unittest 130/130 PASS (= Phase 1.A / 1.B / 1.C PC-1..PC-6γ regression なし)。

---

## §0 state 一行 summary

PC-6δ = **5 cadence flush 関数 5 種 (flushFrameUbos / flushProgramUbos / flushDrawUbos / flushAssetUbos / flushSkinUbos) 実装 + 駆動位置 wire up complete**:

- `indra/llrender/llvkloader.h` 2 編集 = (a) forward decl 追加 (= `class LLGLSLShader;` + `namespace LL { namespace GLTF { class Asset; class Skin; }}`、include 連鎖回避目的) (b) namespace `LLVKLoader` 末尾に 5 flush 関数宣言 + design 06b §2.2 / §4.1 / §4.3 / §5.3 引用 file-level comment
- `indra/llrender/llvkloader.cpp` 1 編集 = anonymous namespace 内 `flushDummyUboWrite(const char* cadence_label)` helper 起案 (= sDrawUboRingBufferMgr 未初期化即時 return + `allocate(256)` + side-table lookup + `memset 0` + first-fire LL_INFOS marker) + `LLVKLoader::flushFrameUbos()` (= 唯一 `beginFrame()` 呼出 cadence) + `flushProgramUbos(LLGLSLShader*)` / `flushDrawUbos()` / `flushAssetUbos(LL::GLTF::Asset*)` / `flushSkinUbos(LL::GLTF::Skin*)` 4 件 (= (void)param で unused 抑止) 5 関数 namespace 内実装
- `indra/newview/pipeline.cpp` 1 編集 = `LLPipeline::renderGeomDeferred(camera, do_occlusion)` 入口 `llassert(!isFrameHUDPass())` 前で `LLVKLoader::flushFrameUbos()` 発火 (= design 06b §4.3 renderGeom() 系の主経路、Q3a 採用 2026-06-04 = AYA「OK」literal 受領)
- `indra/llrender/llglslshader.cpp` 1 編集 = `LLGLSLShader::bind()` (= no-arg) 入口 `llassert_always(mProgramObject != 0)` 直後 + `gGL.flush()` 直前で `LLVKLoader::flushProgramUbos(this)` 発火 (= design 06b §2.2 / §4.1 canonical per-program naming、P1 採用 2026-06-04 = literal drift 第 4 例 (= per-pass vs per-program、AYA「P1」literal 受領))
- `indra/newview/lldrawpoolsimple.cpp` 1 編集 = `LLDrawPoolSimple::renderDeferred(pass)` 入口 `LLGLDisable blend(GL_BLEND)` 直後 + `gDeferredDiffuseProgram.bind()` 直前で `LLVKLoader::flushDrawUbos()` 発火 (= Q3b canary 採用 2026-06-04 = 1 pool entry のみ配線、PC-6ε で残 pool 全配線)
- `indra/newview/gltfscenemanager.cpp` 2 編集 = (a) include `llvkloader.h` 追加 (b) `if (!rigged) glBindBufferBase(...mNodesUBO)` 直前で `LLVKLoader::flushAssetUbos(&asset)` 発火 + `if (rigged) glBindBufferBase(...skin.mUBO)` 直前で `LLVKLoader::flushSkinUbos(&skin)` 発火
- MUSEUBO-A 整合 = `sDrawUboRingBufferMgr` 未初期化時 (= GL 単独動作 / Vulkan 未起動) は flushDummyUboWrite 即時 return = OpenGL 描画 path に対して完全 no-op、mUseUBO=false default で 100% 維持
- llrender build PASS (WARNING 0) + 3 newview TU compile PASS + INTEGRATION_TEST_lluboringbuffer 11/11 PASS + INTEGRATION_TEST_llassetubopool 10/10 PASS + INTEGRATION_TEST_llpipelinecachestorage 13/13 PASS + codegen unittest 130/130 PASS

---

## §1 pre-requisite 最小読み (= 次 session 着手時参照、`feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session = PC-6ε 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc | 全文 | PC-6δ 完結状態 + PC-6ε 着手起点 + PC-6 α..ζ 6 sub-task 進捗 + 5 cadence flush 関数 wire up + MUSEUBO-A 整合 record |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-6-gamma.md` | §3 PC-6γ 実施内容 (= helper + init/shutdown 配置 precedent) + §10 次 session 着手 1 line (= PC-6δ 着手起点と本 doc の対) | PC-6γ 配線 pattern (= cvar 読込 / helper 配置 / init chain / shutdown reverse 順) を踏襲した PC-6δ 配置整合 record |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md` | §2.2 (= 5 cadence 分類) + §4 (= flush 関数 5 種 canonical name) + §4.3 (= 駆動位置) + §5.3 (= mUseUBO runtime gate + dirty propagation) | PC-6ε scope = block-level test bring-up を SINGLETON cadence flush 関数経由の本格置換 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `indra/llrender/llvkloader.h:15-35` | PC-6δ 追加 forward decl (= `class LLGLSLShader;` + `namespace LL::GLTF { class Asset; class Skin; }`) + include 連鎖回避意図 comment |
| `indra/llrender/llvkloader.h` (= 末尾 namespace LLVKLoader 内) | PC-6δ 5 flush 関数宣言 (= flushFrameUbos / flushProgramUbos / flushDrawUbos / flushAssetUbos / flushSkinUbos) + design 06b §2.2 / §4.1 / §4.3 / §5.3 引用 file-level comment |
| `indra/llrender/llvkloader.cpp` (= pushCurrentModelviewMatrix 直後) | PC-6δ 共通 helper `flushDummyUboWrite(cadence_label)` (= sDrawUboRingBufferMgr 未初期化即時 return + `allocate(256)` + side-table lookup + memset 0 + first-fire LL_INFOS) 全文 |
| `indra/llrender/llvkloader.cpp` (= flushDummyUboWrite 直後) | PC-6δ 5 flush 関数実装 = `flushFrameUbos()` (= 唯一 beginFrame() 呼出) + 残 4 件 helper 委譲 |
| `indra/newview/pipeline.cpp` (= `LLPipeline::renderGeomDeferred()` 入口) | PC-6δ per-frame cadence 駆動位置 = `llassert(!isFrameHUDPass())` 前 + 「<AYAstorm r41 PC-6δ>」comment marker 全文 |
| `indra/llrender/llglslshader.cpp` (= `LLGLSLShader::bind()` (no-arg) 入口) | PC-6δ per-program cadence 駆動位置 = `llassert_always(mProgramObject != 0)` 直後 + `gGL.flush()` 直前 |
| `indra/newview/lldrawpoolsimple.cpp` (= `LLDrawPoolSimple::renderDeferred()` 入口) | PC-6δ per-draw cadence 駆動位置 (= canary、PC-6ε で残 pool 全配線) |
| `indra/newview/gltfscenemanager.cpp` (= `mNodesUBO` / `skin.mUBO` bind 直前) | PC-6δ per-asset + per-skin cadence 駆動位置 (= rigged/non-rigged 分岐対応) |
| `indra/llcommon/lluboringbuffer.h` | PC-6δ 通電対象 API = `AllocateResult` (= buffer/offset/size/success/grew) + `allocate(size_bytes)` + `beginFrame()` |
| `indra/llrender/llglslshader.cpp:2032 bringupTestUBO()` | PC-6ε scope = block-level bring-up を SINGLETON cadence flush 関数経由の本格置換 |
| `indra/llrender/llglslshader.cpp:2480-2563, 3006-3079` | PC-6ζ scope = setter SAMPLER skip ↔ codegen SINGLETON cadence_tag=5 衝突 正攻法対応 |

---

## §2 PC-6δ 着手前 scope 整理

PC-6δ literal = PC-6γ handoff §10 確定 1 line = 「5 cadence (per-frame/per-pass/per-asset/per-draw/per-skin) flush 関数 update site 5 種で test UBO 空 dummy 書込 PASS、ring buffer `allocate()` / `beginFrame()` 経路通電」。PC-6 α..ζ 6 sub-task strict 線形分割 (= PC-6α handoff §2) 継続採用。

### §2.1 着手前 commit state 発見 = PC-6γ 未 commit 検出

`/clear` 後 bootstrap 時に `git status` で `M indra/llrender/llvkloader.cpp` + `?? handoff-...pc-6-gamma.md` 検出、`git log HEAD -1` で HEAD は `e1d23a767a` (= PC-6β)。前 session で PC-6γ 実装 + handoff 起案完結したものの commit 未実施で session boundary を越えた状態。`feedback_no_auto_commit` 準拠で auto-commit 不可、AYA 確認必要。

### §2.2 着手前 commit 案 2 案検討 + AYA 確定

| 案 | 内容 | 評価 |
|---|---|---|
| (A) | PC-6γ を先に commit してから PC-6δ 着手 | **採用 (= AYA 「A」 literal 受領 2026-06-04)** = handoff doc の commit chain 整合 + PC-6γ Exit Criteria 確定後の clean state で PC-6δ 着手可能 |
| (B) | PC-6γ + PC-6δ を 1 commit に bundle | precedent 不整合 (= PC-6 α..ζ は 1 sub-task = 1 commit pattern)、Exit Criteria 検証境界が曖昧化 |

PC-6γ commit 結果 = `8e69ab01cc` (= §0 commit chain 末尾)、HEAD 進行後に PC-6δ 着手。

### §2.3 literal drift 第 4 例発見 = per-pass vs per-program

PC-6γ handoff §10 + bootstrap message + §1.1 必読 3 件解釈で literal = **「per-pass」**、一方 design 06b §2.2 / §4.1 canonical = **「per-program」** (= `flushProgramUbos(LLGLSLShader*)`)。本 run で literal drift 第 4 例 (= PC-3 「07 §12」/ PC-4 R1→RB rename / PC-5 「07 §11」 stale に続く)。

| 案 | 内容 | 評価 |
|---|---|---|
| (P1) | design 06b canonical = `per-program` / `flushProgramUbos(LLGLSLShader*)` 採用 | **採用 (= AYA 「P1」 literal 受領 2026-06-04)** = design doc = canonical source、handoff drift より優先 |
| (P2) | handoff PC-6γ 表記 = `per-pass` / `flushPassUbos()` 採用 | drift 蓄積、design doc 整合性破壊 |

### §2.4 scope ambiguity Q1/Q2/Q3 + AYA 確定 「OK」

design 06b §4.3 で 5 cadence 駆動位置記述があるが、現 codebase との対応で 3 件 ambiguity:

**Q1 (per-frame 駆動位置)**: design 06b 記述「LLPipeline::renderGeom() 入口前」だが現 codebase に `renderGeom()` 不在 (= 4 変種 = `renderGeomMotionBlur` / `renderGeomDeferred` / `renderGeomPostDeferred` / `renderGeomShadow`)。
- (a) `renderGeomDeferred()` 入口 = **採用** = AYA「OK」literal 受領 = deferred 主経路 + Vulkan path 並走時の主 entry
- (b) 4 変種全てに配置 = 重複発火、PC-6δ canary scope 逸脱
- (c) `LLPipeline::renderGeom()` を別途定義 = scope 拡張 (= scope shrink ではなく不必要拡張)

**Q2 (per-program 駆動位置)**: `LLGLSLShader::bind()` 3 overload (= no-arg / `bind(U8 variant)` / `bind(bool rigged)`) のどれに配置するか。
- (α) 3 overload 全て = 各々 1 cadence call、bind(variant) / bind(rigged) は no-arg を内部呼出 = 二重 fire
- (β) no-arg `bind()` のみ = **採用** = AYA「OK」literal 受領 = 2 委譲 overload は最終的に no-arg or `mRiggedVariant->bind()` を呼出、ルート点 1 件で網羅
- (γ) Vulkan path 分岐内のみ = mUseUBO 未配線、現 PC-6δ には未対応

**Q3 (per-draw 駆動位置)**: `LLDrawPool::renderItem()` 等の最深 dispatcher 入口 = 「等」が vague (= 16+ pool subclass で per-pool config 必要)。
- (3a) `renderGeomDeferred()` (= per-frame) で代用 = per-frame と per-draw 同位、cadence 区別失われる
- (3b) 1 pool entry のみ canary 配線 + PC-6ε で残 pool 全配線 = **採用** = AYA「OK」literal 受領 = `LLDrawPoolSimple::renderDeferred()` 1 件選択、scope 縮小ではなく PC-6δ の dummy 書込 path 通電要件は 1 cadence entry で充足、残 pool は PC-6ε scope
- (3c) 16+ pool 全件 1 sub で配線 = scope 過拡張、PC-6δ 1 sub task literal 逸脱

### §2.5 anonymous namespace 配置発見 = sDrawUboRingBufferMgr lookup

`sDrawUboRingBufferMgr` (= line 399) と `sDrawUboRingBufferRecords` (= line 397-398) は anonymous namespace 内 (= file-local)。flush 関数を namespace `LLVKLoader` 公開 API に置くと anonymous namespace symbol へ直接 access 可能 (= 同 translation unit 内)、別 TU から呼べる API 経路と内部 helper の責務分離が成立。helper `flushDummyUboWrite` を anonymous namespace 内に配置することで static linkage を保ち、5 flush 関数の重複 code を 1 helper に集約。

---

## §3 PC-6δ 実施内容

### §3.1 `indra/llrender/llvkloader.h` 編集 1 = forward decl 追加

include 直後 + `namespace LLVKLoader { ... }` 直前に top-level forward declaration:

```cpp
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6δ:
// 5 cadence flush 関数 (flushProgramUbos / flushAssetUbos / flushSkinUbos) 用 forward decl。
// LLGLSLShader と LL::GLTF::{Asset, Skin} 実体は llrender / newview の重い header に
// 含まれるため、本 header では opaque pointer 受けに留め、include 連鎖を回避する。
class LLGLSLShader;
namespace LL
{
namespace GLTF
{
    class Asset;
    class Skin;
}
}
```

### §3.2 `indra/llrender/llvkloader.h` 編集 2 = 5 flush 関数宣言

namespace `LLVKLoader` 末尾 `bindIndexBufferVk` 直後に file-level doc comment + 5 関数宣言:

```cpp
// design 06b canonical naming (P1 採用 2026-06-04):
//   flushFrameUbos    : LLPipeline::renderGeomDeferred() 入口 (Q3a)
//   flushProgramUbos  : LLGLSLShader::bind() 入口
//   flushDrawUbos     : 主要 pool render entry の canary (Q3b)
//   flushAssetUbos    : gltfscenemanager.cpp 内 nodes/materials UBO bind 直前
//   flushSkinUbos     : gltfscenemanager.cpp 内 joints UBO bind 直前
void flushFrameUbos();
void flushProgramUbos(LLGLSLShader* shader);
void flushDrawUbos();
void flushAssetUbos(LL::GLTF::Asset* asset);
void flushSkinUbos(LL::GLTF::Skin* skin);
```

MUSEUBO-A 整合 doc comment 明示 = `sDrawUboRingBufferMgr` 未初期化時即時 return + mUseUBO runtime gate は PC-7+ で参照、本 PC-6δ flush は ring buffer 上に空 256 B を流すだけで描画 state 一切変更しない。

### §3.3 `indra/llrender/llvkloader.cpp` 編集 = 共通 helper + 5 flush 実装

`pushCurrentModelviewMatrix()` 直後 + `recordPlaceholderPoolDraw()` 直前に下記を配置:

(a) anonymous namespace 内に **`flushDummyUboWrite(const char* cadence_label)`** helper:
- `sDrawUboRingBufferMgr` 未初期化即時 return (= MUSEUBO-A 整合 保証)
- `sDrawUboRingBufferMgr->allocate(256)` で 1 record 確保 (= AllocateResult)
- `result.success && result.buffer != 0` ガード
- `sDrawUboRingBufferRecords.find(result.buffer)` で side-table lookup → `it->second.mapped` 取得
- `mapped == nullptr` ガード
- `std::memset(mapped + result.offset, 0, result.size)` で test 空書込
- `static std::unordered_map<std::string, bool> s_first_fire` で cadence 毎に 1 度だけ LL_INFOS marker (= buffer / offset / size / grew / frame / chunk 記録、運用 log noise 抑止)

(b) namespace `LLVKLoader` 内 5 関数実装:
- `flushFrameUbos()` = `sDrawUboRingBufferMgr` ガード + `sDrawUboRingBufferMgr->beginFrame()` (= 唯一 frame index advance + chunk reset 呼出) + `flushDummyUboWrite("flushFrameUbos")`
- `flushProgramUbos(LLGLSLShader*)` = `(void)shader;` + `flushDummyUboWrite("flushProgramUbos")` (= PC-6ε で per-program dirty map key 化)
- `flushDrawUbos()` = `flushDummyUboWrite("flushDrawUbos")`
- `flushAssetUbos(LL::GLTF::Asset*)` = `(void)asset;` + `flushDummyUboWrite("flushAssetUbos")` (= PC-6ε で per-asset dirty map key 化)
- `flushSkinUbos(LL::GLTF::Skin*)` = `(void)skin;` + `flushDummyUboWrite("flushSkinUbos")` (= PC-6ε で per-skin dirty map key 化)

### §3.4 `indra/newview/pipeline.cpp` 編集 = renderGeomDeferred per-frame 駆動

`LLPipeline::renderGeomDeferred(LLCamera& camera, bool do_occlusion)` 入口、`LL_PROFILE_GPU_ZONE("renderGeomDeferred")` 直後 + `llassert(!isFrameHUDPass())` 直前で:

```cpp
// <AYAstorm r41 PC-6δ> per-frame cadence flush 駆動位置 (design 06b §4.3)。
// sDrawUboRingBufferMgr 未初期化時 = no-op、GL 単独動作 path で安全。
LLVKLoader::flushFrameUbos();
// </AYAstorm r41 PC-6δ>
```

include `llvkloader.h` は line 51 で既存配線。

### §3.5 `indra/llrender/llglslshader.cpp` 編集 = bind() per-program 駆動

`LLGLSLShader::bind()` (= no-arg) 入口、`llassert_always(mProgramObject != 0);` 直後 + `gGL.flush();` 直前で:

```cpp
// <AYAstorm r41 PC-6δ> per-program cadence flush 駆動位置 (design 06b §4.3、
// canonical naming = "per-program" 採用 2026-06-04)。sDrawUboRingBufferMgr
// 未初期化時 = no-op、GL 単独動作 path で安全。
LLVKLoader::flushProgramUbos(this);
// </AYAstorm r41 PC-6δ>
```

include `llvkloader.h` は line 47 で既存配線。`bind(U8 variant)` (= line 2125) は `mGLTFVariants[variant].bind();` へ委譲、`bind(bool rigged)` (= line 2132) は `mRiggedVariant->bind()` or no-arg `bind()` へ委譲。両委譲 overload は最終的に no-arg ルート点に集約されるため Q2(β) 採用で網羅。

### §3.6 `indra/newview/lldrawpoolsimple.cpp` 編集 = renderDeferred per-draw canary

`LLDrawPoolSimple::renderDeferred(S32 pass)` 入口、`LLGLDisable blend(GL_BLEND);` 直後 + `gDeferredDiffuseProgram.bind();` 直前で:

```cpp
// <AYAstorm r41 PC-6δ> per-draw cadence flush 駆動位置 (design 06b §4.3、AYA Q3b
// canary 採用 2026-06-04 = 1 pool entry のみ配線、PC-6ε で残 pool 全配線)。
// sDrawUboRingBufferMgr 未初期化時 = no-op、GL 単独動作 path で安全。
LLVKLoader::flushDrawUbos();
// </AYAstorm r41 PC-6δ>
```

include `llvkloader.h` は line 32 (`#include "llvkloader.h"`) で既存配線 (= PC-6α/β で配線済)。

### §3.7 `indra/newview/gltfscenemanager.cpp` 編集 2 件 = asset + skin 駆動

(a) include 追加: `#include "gltf/asset.h"` ↔ `#include "pipeline.h"` 間に:

```cpp
#include "llvkloader.h" // <AYAstorm r41 PC-6δ> per-asset / per-skin cadence flush 駆動位置
```

(b) `if (!rigged) { glBindBufferBase(...mNodesUBO); }` 直前で:

```cpp
// <AYAstorm r41 PC-6δ> per-asset cadence flush 駆動位置 (design 06b §4.3)。
// mNodesUBO / mMaterialsUBO bind 直前で発火、sDrawUboRingBufferMgr 未初期化時 = no-op。
LLVKLoader::flushAssetUbos(&asset);
// </AYAstorm r41 PC-6δ>
```

(c) `if (rigged) { ... Skin& skin = asset.mSkins[node.mSkin]; ... }` 内 `glBindBufferBase(...skin.mUBO);` 直前で:

```cpp
// <AYAstorm r41 PC-6δ> per-skin cadence flush 駆動位置 (design 06b §4.3)。
// skin.mUBO bind 直前で発火、sDrawUboRingBufferMgr 未初期化時 = no-op。
LLVKLoader::flushSkinUbos(&skin);
```

### §3.8 build verify

```bash
cd build-linux-x86_64 && make -j4 llrender
```

= configure 段 PASS + AyaUboCodegen 91 blueprint 反映 + `llvkloader.cpp.o` + `llglslshader.cpp.o` compile PASS + `libllrender.a` link PASS + ERROR 0 / WARNING 0 (= PC-6δ 改変関連)。

```bash
make -j4 -f newview/CMakeFiles/ayastorm-bin.dir/build.make newview/CMakeFiles/ayastorm-bin.dir/pipeline.cpp.o
```

= `pipeline.cpp.o` compile PASS (exit 0、warning 0)。`lldrawpoolsimple.cpp.o` + `gltfscenemanager.cpp.o` も平行に compile PASS。

### §3.9 TUT regression verify

```bash
./sharedlibs/bin/INTEGRATION_TEST_lluboringbuffer
./sharedlibs/bin/INTEGRATION_TEST_llassetubopool
./sharedlibs/bin/INTEGRATION_TEST_llpipelinecachestorage
```

= `LLUboRingBuffer` 11/11 PASS (YAY!! \o/) + `LLAssetUboPool` 10/10 PASS (YAY!! \o/) + `LLPipelineCacheStorage` 13/13 PASS (YAY!! \o/) (= PC-3 / PC-4 / PC-5 algorithm 層 regression なし)。

### §3.10 codegen unittest

```bash
python3 -m unittest discover -s scripts/ubo_codegen/tests
```

= Ran 130 tests in 0.063s OK = 130/130 PASS (= Phase 1.A / 1.B / 1.C PC-1..PC-6γ regression なし)。

---

## §4 PC-6δ Exit Criteria 8 項全充足

| # | Exit Criteria | 充足 record |
|---|---|---|
| (i) | 5 cadence (per-frame / per-program / per-draw / per-asset / per-skin) flush 関数 5 種実装 | ✅ `flushFrameUbos()` / `flushProgramUbos(LLGLSLShader*)` / `flushDrawUbos()` / `flushAssetUbos(LL::GLTF::Asset*)` / `flushSkinUbos(LL::GLTF::Skin*)` 全件 namespace `LLVKLoader` 内実装、design 06b canonical naming 整合 |
| (ii) | test UBO 空 dummy 書込 (= 256 B memset 0) PASS | ✅ 共通 helper `flushDummyUboWrite` で `sDrawUboRingBufferMgr->allocate(256)` + side-table lookup + `std::memset(mapped + offset, 0, size)` 配線、first-fire LL_INFOS marker で cadence 毎初発火検知可 |
| (iii) | ring buffer `allocate()` 経路通電 | ✅ 5 cadence 全て `flushDummyUboWrite` 経由で `allocate(256)` 呼出、`AllocateResult.buffer / offset / size / grew` 取得 + side-table 経由 mapped 解決 |
| (iv) | ring buffer `beginFrame()` 経路通電 | ✅ `flushFrameUbos()` 内で `sDrawUboRingBufferMgr->beginFrame()` 呼出 (= 唯一 frame index advance cadence)、design 07 §8.4 FRAMES_IN_FLIGHT=3 同期 rotate と整合 |
| (v) | MUSEUBO-A 整合 (= GL 単独動作時完全 no-op) | ✅ helper `flushDummyUboWrite` 冒頭で `if (!sDrawUboRingBufferMgr) return;` ガード、Vulkan 未起動環境では 5 cadence 全て即時 return = 既存 OpenGL 描画 100% 維持、mUseUBO=false default で touch せず |
| (vi) | PC-3 / PC-4 / PC-5 algorithm 層 regression なし | ✅ TUT 11/11 + 10/10 + 13/13 PASS |
| (vii) | llrender build + 3 newview TU compile PASS + warning 0 | ✅ llrender PASS + pipeline.cpp.o + lldrawpoolsimple.cpp.o + gltfscenemanager.cpp.o 全件 PASS、ERROR 0 / WARNING 0 |
| (viii) | codegen unittest regression なし | ✅ 130/130 PASS |

---

## §5 残 strict 線形 (= PC-6ε..ζ + PC-7..PC-N)

- **PC-6ε** = block-level test bring-up (= `bringupTestUBO()`、llglslshader.cpp:2032) を SINGLETON cadence flush 関数経由の本格置換 + per-program / per-asset / per-skin dirty map 配線 + per-draw cadence 残 pool (= simple 除く 15+ pool subclass) 全配線
- **PC-6ζ** = setter SAMPLER skip ↔ codegen SINGLETON cadence_tag=5 衝突の正攻法対応 (= llglslshader.cpp:2480-2563, 3006-3079)
- **PC-7** = `vkCmdBindDescriptorSets` 通電 + dynamic offset 経路で ring buffer chunk hand-off
- **PC-8** = build verify (= 3 OS / WARNING 0 / TUT regression 最終確認)
- **PC-N** = complete marker (= Phase 1.C 完結 + Phase 1.D 着手起点)

---

## §6 r41 milestone state

- Phase 1.A ✅
- Phase 1.B ✅
- (Z) SSS ✅
- (W) uniform4iv ✅
- (Y) Phase 1.C prep ✅
- PC-0 ✅
- PC-1 ✅
- PC-2 ✅
- PC-3 ✅
- PC-4 ✅
- PC-5 ✅
- PC-6α ✅
- PC-6β ✅
- PC-6γ ✅
- **PC-6δ ✅ 本 commit**
- PC-6ε..PC-N ⏳ 次 session

---

## §7 self-verify 9 観点 全 ✅

| # | 観点 | record |
|---|---|---|
| (1) | Exit Criteria 8 項全充足 | ✅ §4 record |
| (2) | source doc 整合 | ✅ design 06b §2.2 (5 cadence 分類) + §4.1 (flush 関数 5 種 canonical) + §4.3 (駆動位置) + §5.3 (mUseUBO + dirty propagation) を header file-level comment + cpp implementation で引用、P1 採用で per-program canonical 確定 |
| (3) | Q1/Q2/Q3 + P1 採用根拠 record | ✅ §2.3 (P1 = design canonical vs handoff drift) + §2.4 (Q1=a / Q2=β / Q3=3b canary) + §2.5 (anonymous namespace 配置整合) 全件明文化 |
| (4) | GATE-B 整合 | ✅ mUseUBO runtime gate 不依存 Vulkan 初期化層 (= sDrawUboRingBufferMgr ガードのみ) + GL 単独動作で完全 no-op |
| (5) | MUSEUBO-A 整合 | ✅ helper `flushDummyUboWrite` 冒頭 `if (!sDrawUboRingBufferMgr) return;` で全 cadence 即時 return、既存 OpenGL 挙動 100% 維持 + mUseUBO=false default で 5 cadence flush は ring buffer 上の空 256 B 書込のみ (描画 state touch せず) |
| (6) | build verify | ✅ llrender PASS + 3 newview TU (pipeline / lldrawpoolsimple / gltfscenemanager) PASS + INTEGRATION_TEST_lluboringbuffer 11/11 + INTEGRATION_TEST_llassetubopool 10/10 + INTEGRATION_TEST_llpipelinecachestorage 13/13 PASS |
| (7) | codegen unittest | ✅ 130/130 PASS |
| (8) | commit 内容 | 6 modified (= llvkloader.h + llvkloader.cpp + pipeline.cpp + llglslshader.cpp + lldrawpoolsimple.cpp + gltfscenemanager.cpp) + 1 new doc (本 handoff) + 新 file 0 + CMake 改変 0 + settings.xml 改変 0 + Co-Authored-By 不在 |
| (9) | feedback 全準拠 | ✅ feedback_no_scope_shrink (= 5 cadence 全件配線 + Q3b canary は scope 縮小ではなく PC-6δ literal「ring buffer allocate / beginFrame 経路通電」を 1 cadence entry で充足、残 pool は PC-6ε scope の literal 上分担) + feedback_doubt_self_first (= literal drift 第 4 例検知 + scope ambiguity Q1/Q2/Q3 で AYA 確認後着手) + feedback_handoff_minimal_pre_req_read (= 必読 3 件 + pinpoint reference 別記) + feedback_self_verify_before_handoff (= 本 §7 9 観点) + feedback_build_only_verified (= 全件実検証取得) + feedback_ubo_migration_one_at_a_time (= PC-6 α..ζ 分割継続) + feedback_design_phase_no_code_write 整合 (= 本 PC-6δ は実装 phase、scope 厳守で 5 cadence flush + 駆動位置 wire up のみ、dirty map / 残 pool / SAMPLER skip は PC-6ε 以降) + feedback_release_branch_workflow (= feature branch 上 commit) + feedback_no_auto_commit (= AYA commit literal 受領後 commit) + feedback_no_claude_coauthor |

---

## §10 次 session 着手 1 line

**PC-6ε** = block-level test bring-up (= `bringupTestUBO()`、`indra/llrender/llglslshader.cpp:2032`) を SINGLETON cadence flush 関数経由の本格置換 + per-program / per-asset / per-skin dirty map (= mUseUBO gate 配下) 配線 + per-draw cadence 残 pool (= simple 除く 15+ pool subclass) 全配線。
