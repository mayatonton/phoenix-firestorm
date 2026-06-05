# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-2 complete** marker

**作成日**: 2026-06-04
**前 commit chain** (= 直前 handoff 起案):
- `e3f24f7a2a` = (Y) Phase 1.C prep handoff doc 起案
- `b8a37d078b` = (Y) Phase 1.C prep PC-0 (Q1) AYA 確定値 4 件 record
- `9ed4cca802` = Phase 1.C **PC-1 complete** = `Global_ReflectionProbes` shell blueprint codegen emit

**本 handoff doc 目的**: **Phase 1.C PC-2 complete marker**。test UBO shell C++ 接続 = block-level test bring-up (= AYA 判断 2026-06-04 (c) 採用)、新 `bringupTestUBO()` hook 起案 + llrender target build PASS + codegen unittest 130/130 PASS 完結後の引継。

---

## §0 state 一行 summary

PC-2 = **test UBO shell C++ 接続 (= block-level test bring-up) complete**:

- 新 private method `LLGLSLShader::bringupTestUBO()` 起案 (= llglslshader.h declaration + llglslshader.cpp definition)
- `ubo::lookup_block("Global_ReflectionProbes")` で識別子 + binding 取出経路成立 (= PC-1 確定 contract 5 項 assert 整合: `block_hash=0xabdfdb31u` / `block_size=256u` / `descriptor_set=0` / `binding=3` / `cadence_tag=5(SINGLETON)`)
- `forwardToUboUpload` 空 dummy buffer 書込 (= `static constexpr std::uint8_t s_dummy[Global_ReflectionProbes_SIZE] = {}`、forwardToUboUpload は Phase 1.B PB-6 空 stub のまま) PASS
- 空 dirty flag set (= `static bool s_test_ubo_dirty = false` shell 段階 marker、PC-6 で本格 dirty 機構と置換予定)
- `if (mUseUBO)` redirect 内 call site = `mapUniforms()` 末尾 PB-7 整合 check 直後追加
- llrender target incremental build PASS (= compile + link OK、warning 0 件 (volk.c -Wno-reorder 除く既知 1 件))
- codegen unittest 130/130 PASS (= Phase 1.A regression なし)
- include 追加 2 件: `ubo/ubo_metadata.inl` (= `BlockMetadata` + `lookup_block`) + `ubo/ubo_layout_global_reflectionprobes.inl` (= `Global_ReflectionProbes_SIZE` constant、codegen output linkage 強制)

---

## §1 pre-requisite 最小読み (= 次 session 着手時参照、`feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session = PC-3 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc | 全文 | PC-2 完結状態 + PC-3 着手起点 + (c) 案採用根拠 + setter SAMPLER skip ↔ codegen SINGLETON cadence_tag=5 衝突 record |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-prep.md` | §3.1 PC-3 row + §5.1 (W2) 持越項目 + §3.2 strict 線形 | PC-3 = (W2) `sAssetUboPool` 起動時 prealloc N=64 + grow chunk 64 実装 scope |
| 3 | `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` | §12 (W2) default 値 + 実装方針 | PC-3 (W2) 持越項目 default 値 (= N=64 / grow=64) + 配線位置 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `indra/llrender/llglslshader.h` | 463-475 行 (= `bringupTestUBO()` declaration 周辺、PC-2 コメント) |
| `indra/llrender/llglslshader.cpp` | 2000-2060 行付近 (= `bringupTestUBO()` 定義) + 1976 行付近 (= mapUniforms 末尾 call site) + 62-69 行 (= include 追加 2 件) |
| `build-linux-x86_64/codegen/ubo/ubo_metadata.inl` | `Global_ReflectionProbes` entry + `lookup_block()` 関数 |
| `build-linux-x86_64/codegen/ubo/ubo_layout_global_reflectionprobes.inl` | 全文 (= 17 line、`Global_ReflectionProbes_SIZE = 256u` constant) |
| `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md` | §4 cadence 別 flush 関数 5 種 (= PC-6 update site 設計、PC-2 hook 本格 wire-up 置換先) |
| `scripts/ubo_codegen/main.py:58-74` | `CADENCE_*` enum + `_PREFIX_TO_CADENCE` table (= SINGLETON=5 routing、setter SAMPLER skip 衝突原因) |
| `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-6-complete.md` | §1.2 06a §5.6 sampler OpenGL path 強制 (= setter `cadence_tag == 5` skip 根拠) |

---

## §2 PC-2 着手時発見 = setter SAMPLER skip ↔ codegen SINGLETON cadence_tag=5 衝突

### §2.1 衝突 detail

| layer | cadence_tag = 5 の意味 | 根拠 |
|---|---|---|
| **codegen (Phase 1.A)** | `CADENCE_SINGLETON` | `scripts/ubo_codegen/main.py:63` + `("Global_", CADENCE_SINGLETON)` routing (`main.py:71`) |
| **host setter (Phase 1.B)** | `CADENCE_SAMPLER` / OpenGL path 強制 skip | `llglslshader.cpp` 30 setter (= 2480-2563 / 3006-3079 等) `if (loc.cadence_tag == 5 /* CADENCE_SAMPLER */) return;` (= 06a §5.6 sampler OpenGL path 強制 根拠) |

実 emit 値: `Global_ReflectionProbes` metadata entry = `{ "Global_ReflectionProbes", 0xabdfdb31u, 256u, 0u, 3u, 0u, 5u, 1u }` (= cadence_tag=5)。setter 経路で見ると SAMPLER skip 扱い → `forwardToUboUpload` 不到達。

### §2.2 (a)(b)(c)(d) 4 案選択肢 (= AYA 判断 2026-06-04 提示)

| 案 | 内容 | scope | 副作用 |
|---|---|---|---|
| **(a)** | SAMPLER sentinel rebase (5 → 別値) + host 30 setter skip 修正 + codegen SAMPLER tag 明示 | 30 setter + codegen + 既存 metadata 再 emit | Phase 1.B 既 commit 触る |
| **(b)** | SINGLETON 別値 rebase (5 → 別値) + codegen 単独 + 全 emit 再生成 | codegen + emit 全 file | Phase 1.A 既 commit 触る |
| **(c)** ✅ | **block-level test bring-up** 再解釈 (= singleton は per-frame stable set 経路、setter 経路非依存) + 新 hook 1 件のみ追加 | 新 hook 1 件、既存 30 setter / codegen 不変 | PC-6 5 cadence update site で本格 wire-up |
| **(d)** | graph 接続のみ、forwardToUboUpload 実走不要解釈 | 0 行 | literal「書込 PASS」compile-time 解釈 |

### §2.3 AYA 確定 literal

「c」(= 2026-06-04 session 受領) → **(c) 採用**。

### §2.4 (c) 採用根拠 3 件

1. **設計整合**: singleton UBO (= 1 instance / 持続) は per-frame stable set で once bind / once upload semantics、per-uniform setter 経路を通る必要なし。`UB_REFLECTION_PROBES` Template A 起点根拠 (= 物理 instance 1 個 / pool 不要 / ring buffer 不要、prep §4.2-2) と semantic 整合。
2. **scope 最小**: Phase 1.A / 1.B 既 commit 不変、新 hook 1 件で PC-2 Exit 充足。`feedback_ubo_migration_one_at_a_time` 整合。
3. **PC-6 整合**: PC-6 = 5 cadence 全経路 update site で SINGLETON cadence の flush 関数を改めて起こす際、block-level bring-up は自然な拡張点。本 hook の dummy buffer 書込は実 reflection probe data 流入で置換予定。

### §2.5 残課題 = setter SAMPLER skip 正攻法対応

本 PC-2 は (c) で迂回、別 PC で正攻法対応予定 (= (a) 案相当)。candidate timing:
- **PC-6** 5 cadence update site で SINGLETON cadence flush 関数を起こす時、SAMPLER 概念を明示分離 (= 06a §5.6 sampler OpenGL path 強制を別 sentinel `0xFFFFFFFEu` 等に migrate)
- **Phase 2 第 1 UBO 本実装 migration** で setter 経路を Vulkan path に切替時、sampler 別途 handle (= 09 §5.2 Template A Phase 2 entry)

---

## §3 PC-2 実施内容

### §3.1 新 method `LLGLSLShader::bringupTestUBO()` 起案

**declaration** (= `indra/llrender/llglslshader.h:455-466` private section、`forwardToUboUpload` 直後):

```cpp
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-2: test UBO shell
// C++ 接続 = block-level test bring-up (= AYA 判断 2026-06-04 (c) 採用)。
// singleton (= cadence=5=SINGLETON、`Global_ReflectionProbes`) は per-uniform
// setter 経路 (= 06a §5.6 `cadence_tag == 5` SAMPLER skip path) を通らない
// = per-frame stable set で once bind / once upload の semantics ゆえ、
// mapUniforms() 末尾で 1 回限り `ubo::lookup_block()` で識別子 + binding 取出
// + `forwardToUboUpload` 空 dummy buffer 書込で経路通電のみ確認。
// mUseUBO=false default ゆえ実走しない (= MUSEUBO-A 整合)。実 data flush は
// PC-6 5 cadence update site で per-frame stable set 経路経由本格化、本 PC-2
// は graph 接続成立 + 空 dummy 書込 + 空 dirty flag set のみ。
void bringupTestUBO();
```

**definition** (= `indra/llrender/llglslshader.cpp` `forwardToUboUpload` body 直後):

主要 5 ブロック構成:
1. `ubo::lookup_block("Global_ReflectionProbes")` 引き → null 時 WARN + early return
2. **5 件 assert** で PC-1 確定 contract 整合 verify:
   - `block_hash == 0xabdfdb31u` (= FNV-1a("Global_ReflectionProbes"))
   - `block_size == ubo::Global_ReflectionProbes_SIZE` (= 256B padded)
   - `descriptor_set == 0u`
   - `binding == 3u`
   - `cadence_tag == 5u` (= SINGLETON、codegen main.py:63)
3. `UniformLocation` 組立 (= block_hash + offset=0 + size=256 + SINGLETON)
4. **空 dummy buffer 書込**: `static constexpr std::uint8_t s_dummy[ubo::Global_ReflectionProbes_SIZE] = {}` (= 256B zero) → `forwardToUboUpload(loc, s_dummy, ubo::Global_ReflectionProbes_SIZE)`
5. **空 dirty flag set**: `static bool s_test_ubo_dirty = false; s_test_ubo_dirty = false;` (= shell marker、PC-6 で本格機構置換)

### §3.2 call site = `mapUniforms()` 末尾 PB-7 整合 check 直後

```cpp
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-2:
// test UBO shell C++ 接続 hook 呼出 (= block-level test bring-up、...)
if (mUseUBO)
{
    bringupTestUBO();
}
```

= `mapUniforms()` 内で PB-2 (= integer index prefill) / PB-3 (= LLStaticHashedString prefill) / PB-7 (= 整合 check) 全完了後の最終 hook。`mUseUBO=false` default ゆえ実走しない (= MUSEUBO-A 整合)。

### §3.3 include 追加 2 件

`indra/llrender/llglslshader.cpp` の既存 `#include "llcontrol.h"` 直後に追加:

```cpp
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-2: test UBO shell
// C++ 接続 = block-level test bring-up = `ubo::lookup_block()` + `BlockMetadata`
// 取得 + PC-1 emit `Global_ReflectionProbes_SIZE` constant 参照。
// llglslshader.h は `ubo/ubo_perfect_hash.inl` (= `UniformLocation` + `lookup_runtime`)
// 既 include、本 .cpp 側で metadata + layout header を追加 include。
#include "ubo/ubo_metadata.inl"
#include "ubo/ubo_layout_global_reflectionprobes.inl"
```

- `ubo_metadata.inl` 入手: `BlockMetadata` struct + `g_block_count` + `g_block_metadata[91]` + `lookup_block()` 関数
- `ubo_layout_global_reflectionprobes.inl` 入手: `Global_ReflectionProbesLayout` struct + `Global_ReflectionProbes_SIZE = 256u` constant (= codegen output linkage 強制)

### §3.4 build verify

**command**: `cd build-linux-x86_64 && make -j4 llrender`

**結果**:
- configure 段 PASS (= cmake 2.7s + generate 0.1s、AyaUboCodegen 91 blueprint 反映)
- llrender 全 source 25 file compile PASS (= `llglslshader.cpp.o` 含む)
- llrender static library link PASS (= `libllrender.a` 生成)
- ERROR 0 件 / WARNING 0 件 (= volk.c -Wno-reorder warning 1 件は既知の C/C++ option mismatch、PC-2 改変無関係)

### §3.5 codegen unittest verify

**command**: `python3 -m unittest discover -s scripts/ubo_codegen/tests`

**結果**: `Ran 130 tests in 0.073s OK` = **130/130 PASS** ✅ (= Phase 1.A PC-1 後 base 維持、regression 0 件)

---

## §4 PC-2 Exit Criteria 充足検証

| Exit Criteria (= prep §3.1 PC-2 row) | 充足 |
|---|---|
| test UBO 識別子取出経路成立 | ✅ (= `ubo::lookup_block("Global_ReflectionProbes")` → `BlockMetadata*`、§3.1 step 1) |
| binding 取出経路成立 | ✅ (= `block->descriptor_set == 0u` + `block->binding == 3u` assert、§3.1 step 2) |
| 空 dirty flag set | ✅ (= `static bool s_test_ubo_dirty = false` shell marker、§3.1 step 5) |
| `if (mUseUBO)` redirect 内 forwardToUboUpload 空 dummy buffer 書込 PASS | ✅ (= `mapUniforms()` 末尾 `if (mUseUBO) { bringupTestUBO(); }` → `forwardToUboUpload(loc, s_dummy, 256)`、§3.2) |

**(c) 案採用ゆえ「setter から」literal は「block-level hook から」と読替** (= §2 record、AYA 確定 literal 2026-06-04)。setter 経路 SAMPLER skip 衝突は §2.5 残課題で別 PC 正攻法対応予定。

---

## §5 残 strict 線形 (= Phase 1.C 内、prep §3.2)

```
✅ PC-0 (Q1) AYA 確定 = UB_REFLECTION_PROBES + per-frame + Template A + 06c §3 接合表機械決定 (2026-06-04)
✅ PC-1 = shell blueprint codegen emit
✅ PC-2 = test UBO shell C++ 接続 (本 commit 完結、(c) 案採用)
  → ⏳ PC-3 = (W2) sAssetUboPool 起動時 prealloc N=64 + grow chunk 64
  → ⏳ PC-4 = (R1/RB) ring buffer 起動時 4 MB / 上限 16 MB + cvar AYARingBufferSizeMB
  → ⏳ PC-5 = (PSC) PSO cache ~/.ayastorm_x64/cache/pipeline_cache.bin 上限 64 MB
  → ⏳ PC-6 = 5 cadence 全経路 update site 実装 (= block-level bring-up と本格置換、SAMPLER skip 正攻法対応 候補 timing)
  → ⏳ PC-7 = vkCmdBindDescriptorSets 通電
  → ⏳ PC-8 = full viewer build + 起動 verify
  → ⏳ PC-N = Phase 1.C Exit Criteria 検証 + complete marker
→ ⏳ Phase 2 = Global_ReflectionProbes 本実装 migration (= 09 §5.2 Template A Phase 2)
```

---

## §6 r41 milestone state (= 本 PC-2 commit 完結時点)

| Phase | 状態 |
|---|---|
| Phase 1.A | ✅ 章クローズ (= `fe2f3a81c6`) |
| Phase 1.B (host-side) | ✅ complete (= `35c4be1046`) |
| (Z) AYAstorm r20 SSS verify | ✅ complete (= `7401feeb1f` + `4dde489ec4`、PR #130) |
| (W) 上流 uniform4iv bug fix | ✅ complete (= `5aadf174f2` + `2a06e12f44`、PR #131) |
| (Y) Phase 1.C prep | ✅ complete (= `e3f24f7a2a` + `b8a37d078b`) |
| Phase 1.C PC-0 | ✅ 確定 (= 2026-06-04) |
| Phase 1.C PC-1 | ✅ complete (= `9ed4cca802`) |
| **Phase 1.C PC-2** | ✅ **本 commit 完結** (= block-level test bring-up、build PASS、unittest 130/130 PASS) |
| Phase 1.C PC-3..PC-N | ⏳ 次 session 引継 |
| (W) (b) upstream LL PR | ⏳ ayastorm-release work 時判断 (= `project_uniform4iv_upstream_pr_deferred`) |

---

## §7 self-verify 9 観点 (= commit 前確認)

| # | 観点 | 確認方法 | 期待 |
|---|---|---|---|
| (1) PC-2 Exit Criteria 4 項全充足 | §4 table 全 ✅ | ✅ |
| (2) PC-1 確定 contract 5 項 (block_hash / size / set / binding / cadence) assert 整合 | §3.1 step 2 + ubo_metadata.inl Global_ReflectionProbes entry literal 値一致 | ✅ |
| (3) `mUseUBO` runtime gate 単独使用 (= GATE-B 整合、`#ifdef LL_VULKAN_GLSL` C++ 不使用) | mapUniforms call site + bringupTestUBO 全体 `mUseUBO` 単独 gate | ✅ |
| (4) MUSEUBO-A 整合 (= mUseUBO=false default で実走しない、既存 OpenGL 挙動 100% 維持) | call site `if (mUseUBO) { bringupTestUBO(); }`、default false | ✅ |
| (5) llrender target build PASS (= compile + link OK) | §3.4 make -j4 llrender EXIT 0 + ERROR 0 件 + warning 0 件 (volk.c 既知除く) | ✅ |
| (6) codegen unittest 130/130 PASS (= Phase 1.A regression なし) | §3.5 literal `Ran 130 tests in 0.073s OK` | ✅ |
| (7) (c) 案採用根拠 3 件 (= 設計整合 + scope 最小 + PC-6 整合) record | §2.4 | ✅ |
| (8) setter SAMPLER skip ↔ codegen SINGLETON cadence_tag=5 衝突 残課題 record | §2.5 = 別 PC 正攻法対応 候補 timing 明示 | ✅ |
| (9) commit 内容 = 2 modified (llglslshader.h +12 / llglslshader.cpp +81) + 1 new doc + Co-Authored-By 不在 | git status + git diff --stat | ✅ (本 commit 段) |

**改変対象** (= 本 commit):
- modified: `indra/llrender/llglslshader.h` (= +12 line、`bringupTestUBO()` declaration + PC-2 コメント)
- modified: `indra/llrender/llglslshader.cpp` (= +81 line、include 追加 2 件 + `bringupTestUBO()` definition + mapUniforms call site)
- new: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-2.md` (= 本 doc)
- `build-linux-x86_64/codegen/ubo/` 配下出力は **commit 対象外** (= build artifact、.gitignore 配下、PC-1 emit のまま不変)

---

## §8 引き継ぎ memory (= 次 session PC-3 着手時参照、既存 memory pointer)

- `project_ayastorm_r41_vulkan_migration` (= r41 milestone state pointer)
- `project_ayastorm_r41_design_principles` (= 2 大原則: 上流取込やすさ + Core 分散実現)
- `project_r41_phase1b_vulkan_host_gate` (= GATE-B = `mUseUBO` runtime flag 単独、`#ifdef LL_VULKAN_GLSL` C++ では使わない)
- `feedback_ubo_migration_one_at_a_time` (= UBO 化作業は 1 つずつ、大塊バッチ禁止、PC-2 (c) 案採用根拠の 1)
- `feedback_handoff_minimal_pre_req_read` (= 次 session pre-req は最小 3 件 + pinpoint 別記)
- `feedback_doubt_self_first` (= PC-2 着手時 setter SAMPLER skip 衝突発見で停止 + AYA 判断仰ぎ実施例)
- `feedback_self_bug_no_defer_option` (= 自作 bug 先送り禁止、setter SAMPLER skip 衝突は別 PC で正攻法対応明示)
- `feedback_no_scope_shrink` (= PC-2 literal scope (c) 案で「縮小」と取られないよう (a)(b)(c)(d) 案明示提示後 AYA 確定)
- `feedback_release_branch_workflow` (= feature branch 上 commit、release branch 直 commit せず)
- `feedback_no_auto_commit` (= AYA 明示指示後 commit)
- `feedback_no_claude_coauthor` (= Co-Authored-By 行不在)
- `feedback_self_verify_before_handoff` (= 9 観点 self-verify 全 ✅)
- `feedback_build_only_verified` (= PC-2 は実 llrender build + codegen unittest で literal 検証取得、机上推論せず)
- `feedback_proactive_handoff` (= context 残量監視で能動 handoff、PC-2 完結後 PC-3 引継 marker 提示)
- `feedback_design_phase_no_code_write` (= PC-2 は実装 phase ゆえ本 rule 非適用、indra/ 改変 OK)

---

## §9 次 session 着手 1 line

**「前 session で PC-2 = block-level test bring-up complete (= 新 `LLGLSLShader::bringupTestUBO()` hook 起案 + `ubo::lookup_block("Global_ReflectionProbes")` + 5 件 assert + `forwardToUboUpload` 空 dummy buffer 256B 書込 PASS、setter SAMPLER skip ↔ codegen SINGLETON cadence_tag=5 衝突は (c) 案で迂回、別 PC で正攻法対応予定)。本 session = **PC-3 着手** = (W2) `sAssetUboPool` 起動時 prealloc N=64 + grow chunk 64 実装 = per-asset cadence pool 起動時 N=64 alloc + dynamic grow chunk 64 動作 unittest PASS (= 07 §12 default 値、prep §3.1 PC-3 row + §5.1 (W2))。必読 3 件 = (1) 本 handoff doc 全文 + (2) prep doc §3.1 PC-3 row + §5.1 (W2) + §3.2 strict 線形 + (3) `07-descriptor-renderpass.md` §12 (W2) default 値 + 実装方針。Phase 1.C Exit = 5 cadence 全経路で `vkCmdBindDescriptorSets` 空 dummy buffer 成功 + render 出力は OpenGL path のまま。」**
