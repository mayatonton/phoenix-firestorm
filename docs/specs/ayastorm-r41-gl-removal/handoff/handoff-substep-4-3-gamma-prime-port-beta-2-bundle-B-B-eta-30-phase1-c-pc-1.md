# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-1 complete** marker

**作成日**: 2026-06-04
**前 commit chain** (= 直前 handoff 起案):
- `fe2f3a81c6` = (X) Phase 1.A PA-B + PA-N **complete** = Phase 1.A 章クローズ
- `7401feeb1f` = (Z) AYAstorm r20 SSS verify **complete**
- `5aadf174f2` = (W) 上流 uniform4iv bug fix **complete**
- `e3f24f7a2a` = (Y) Phase 1.C prep handoff doc 起案
- `b8a37d078b` = (Y) Phase 1.C prep PC-0 (Q1) AYA 確定値 4 件 record

**本 handoff doc 目的**: **Phase 1.C PC-1 complete marker**。`Global_ReflectionProbes` shell blueprint .glsl 起案 + codegen 走行 + emit 出力検証 + 256B 倍数 padding 確認完結後の引継。

---

## §0 state 一行 summary

PC-1 = **`Global_ReflectionProbes` shell blueprint codegen emit complete**:

- `indra/newview/app_settings/shaders/aya_r41_blueprints/set0/global_reflection_probes.glsl` 新規起案 (1 file)
- `scripts/ubo_codegen/main.py` 走行 EXIT 0 = 91 input / 91 block / 96 file emit / 383 member / 10634ms / ERROR・WARNING 0 件
- `build-linux-x86_64/codegen/ubo/ubo_layout_global_reflectionprobes.inl` 生成、`Global_ReflectionProbes_SIZE = 256u // std140=16, device-padded=256` = **256B 倍数 padding 確認** ✅
- 全 aggregate file (= `ubo_metadata.inl` / `ubo_perfect_hash.inl` / `ubo_index.inl` / `ubo_host_loader.inl` / `ubo_dummy_init.inl`) 反映、`g_set0_binding_count = 4u` (= 3 → 4 で binding=3 反映)
- unittest 130/130 PASS (= `python3 -m unittest discover -s scripts/ubo_codegen/tests`)

---

## §1 pre-requisite 最小読み (= 次 session 着手時参照、`feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session = PC-2 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc | 全文 | PC-1 完結状態 + PC-2 着手起点 + shell blueprint 識別子 (= block name `Global_ReflectionProbes` / set=0 binding=3 / size=256B / cadence=SINGLETON) |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-prep.md` | §3.1 PC-1..PC-N sub-task table + §3.2 strict 線形 + §4.1 PC-0 確定値 4 件 | Phase 1.C 全体 scope + PC-2 input 契約 (= PC-1 で確定した shell の binding / set / size / PSO layout) |
| 3 | `indra/llrender/llglslshader.cpp:2480-2563, 3006-3079` (= Phase 1.B 完了済 30 setter Vulkan path 分岐) | 該当行 + その周辺 `if (mUseUBO)` runtime gate | PC-2 C++ shell 接続点 (= setter から test UBO 識別子 + binding 取出経路、空 dummy buffer 書込) |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `indra/newview/app_settings/shaders/aya_r41_blueprints/set0/global_reflection_probes.glsl` | 全文 (= 25 line)、shell blueprint 識別子確認 |
| `build-linux-x86_64/codegen/ubo/ubo_layout_global_reflectionprobes.inl` | 全文 (= `Global_ReflectionProbesLayout` struct + `Global_ReflectionProbes_SIZE = 256u`) |
| `build-linux-x86_64/codegen/ubo/ubo_metadata.inl` | `Global_ReflectionProbes` entry (= size/set/binding/cadence/member_count) |
| `build-linux-x86_64/codegen/ubo/ubo_host_loader.inl` | `g_set0_binding_count = 4u` (= PC-1 で 3 → 4 増 reflection) |
| `docs/specs/ayastorm-r41-gl-removal/design/06c-descriptor-set-bind-wiring.md` | §3 接合表 set=0 binding=3 row + §3.1 §3.2 配置根拠 |
| `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md` | §5.1 `forwardToUboUpload` 経路 + §4 cadence 別 flush 関数 (= PC-2 dirty flag set 経路 + PC-6 update site) |
| `scripts/ubo_codegen/main.py:65-80` | `_PREFIX_TO_CADENCE` table (= `Global_` → CADENCE_SINGLETON=5 routing 根拠) |

---

## §2 PC-1 実施内容

### §2.1 shell blueprint .glsl 起案

**新規 file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set0/global_reflection_probes.glsl` (25 line)

**配置先 set0/ 根拠**: 06c §3 接合表 で set=0 binding=3 確定 (= Frame* 3 UBO + Global_ReflectionProbes を per-frame stable set に同居、§3.1 配置根拠)

**block name 確定**: `Global_ReflectionProbes`
- PC-0 (Q1-a) AYA 確定値 = `UB_REFLECTION_PROBES` (= 概念識別子)
- 06c §3 接合表 で `UB_GLOBAL_REFLECTION_PROBES` (= 旧 `UB_REFLECTION_PROBES` rename) に詳細化、対応 GLSL block name = `Global_ReflectionProbes`
- PC-0 literal の `UB_REFLECTION_PROBES` ≡ host-side enum `UB_GLOBAL_REFLECTION_PROBES` ≡ GLSL block name `Global_ReflectionProbes` (= 同一 UBO の 3 階層識別子)

**descriptor set / binding 確定**: `layout(std140, set = 0, binding = 3)`
- 06c §3 接合表 機械決定 (= PC-0 (Q1-c) literal 充足)
- set=0 = per-frame stable set 帯、binding=3 = Frame* 3 UBO (binding=0/1/2) の次

**cadence 確定**: `Global_` prefix → CADENCE_SINGLETON (= main.py:71 routing 確定)
- PC-0 (Q1-b) literal = "per-frame" は **binding cadence** (= stable set frame-start bind) を指す、codegen routing cadence は **owner cadence** = SINGLETON
- 09 §5.2 Template A 整合 (= "singleton 系最小 UBO、per-frame cadence" は owner=singleton + bind=per-frame stable set の意)

**shell member**: `vec4 _shell_placeholder;` (= std140 16B + device-padded 256B)
- 09 §4.2 shell ↔ 本実装 layout 互換性 = shell の **set/binding/size/PSO layout** は Phase 2 本実装と完全一致、**member 定義 / dirty 判定 / flush logic** は Phase 2 で全面書換可
- size=256B は chapter 08 §6.4 で 256B 倍数 padding 確定の最小単位、Phase 2 で実 member 群追加時 size 増えても 256B 倍数を維持する契約

### §2.2 codegen 走行 + emit 検証

**command**: `python3 scripts/ubo_codegen/main.py --input indra/newview/app_settings/shaders/aya_r41_blueprints --output build-linux-x86_64/codegen/ubo --force`

**log 出力 literal**:
```
[codegen_ubo] INFO: tool=ubo_codegen version=0.1.0-PA6 python=3.12.3
[codegen_ubo] INFO: input=indra/newview/app_settings/shaders/aya_r41_blueprints output=build-linux-x86_64/codegen/ubo
[codegen_ubo] INFO: 91 .glsl input(s) discovered
[codegen_ubo] INFO: --force given, bypassing incremental cache
[codegen_ubo] INFO: emitted 96 file(s) for 91 block(s) / 383 member(s) in 10634 ms
[codegen_ubo] INFO: cache written: build-linux-x86_64/codegen/ubo/codegen_state.json
```

**EXIT = 0** / **ERROR・WARNING 0 件**

**入出力 delta** (= Phase 1.A PA-B 後 base 90 blueprint / 382 member との差分):
- input: 90 → **91** .glsl (= +1 `global_reflection_probes.glsl`)
- block: 90 → **91** (= +1 `Global_ReflectionProbes`)
- emit file: 95 → **96** (= +1 `ubo_layout_global_reflectionprobes.inl`)
- member: 382 → **383** (= +1 `_shell_placeholder`)
- 走行時間: 10489ms → 10634ms (= +145ms、SPIR-V cross-check 1 個分追加負荷)

### §2.3 emit 出力検証 (= per-layout / aggregate 5 file)

**per-layout** = `build-linux-x86_64/codegen/ubo/ubo_layout_global_reflectionprobes.inl`:
```cpp
struct Global_ReflectionProbesLayout {
    static constexpr std::uint32_t _shell_placeholder_OFFSET = 0u;  // size=16 align=16
};
inline constexpr std::uint32_t Global_ReflectionProbes_SIZE = 256u; // std140=16, device-padded=256
```

→ **256B 倍数 padding 確認 ✅** (= PC-1 Exit Criteria literal 充足)

**aggregate** (= 5 file 全 reflect):

| file | 反映内容 |
|---|---|
| `ubo_metadata.inl` | `{ "Global_ReflectionProbes", 0xabdfdb31u, 256u, 0u, 3u, 0u, 5u, 1u }` = size=256 / set=0 / binding=3 / cadence=5(SINGLETON) / member_count=1 |
| `ubo_perfect_hash.inl` | `"Global_ReflectionProbes::_shell_placeholder"` entry 登録 |
| `ubo_index.inl` | `#include "ubo_layout_global_reflectionprobes.inl"` 追加 |
| `ubo_host_loader.inl` | `g_set0_binding_count = 4u` (= 3 → 4 で binding=3 反映)、`g_set1a/1b/2/3` 不変 |
| `ubo_dummy_init.inl` | Phase 1.A stub のまま (= `g_program_count = 0u`、Phase 1.B host-side で program enumeration 後反映予定) |

### §2.4 unittest 検証

**command**: `python3 -m unittest discover -s scripts/ubo_codegen/tests`
**結果**: `Ran 130 tests in 0.067s OK` = **130/130 PASS** ✅ (= Phase 1.A PA-B 後 base 維持)

### §2.5 codegen output 実 path 確認 (= prep doc 引用 path drift 反映)

prep doc §9 + §3.1 PC-1 row literal の **`indra/llrender/codegen_ubo/<UBO 識別子>.h`** は **概念表記**。実 emit 先は `indra/cmake/AyaUboCodegen.cmake` 確立済の `${CMAKE_BINARY_DIR}/codegen/ubo/` = `build-linux-x86_64/codegen/ubo/`:

- 根拠: `indra/cmake/AyaUboCodegen.cmake:AYA_UBO_CODEGEN_OUTPUT_DIR = ${CMAKE_BINARY_DIR}/codegen/ubo`
- include path: `AYA_UBO_CODEGEN_INCLUDE_DIR = ${CMAKE_BINARY_DIR}/codegen` (= host-side `#include "ubo/ubo_index.inl"` 経由で参照)
- 生成 file naming: `<BlockName>.h` ではなく `ubo_layout_<lowercased_blockname>.inl` (= `perfect_hash.emit_layout_inl()` 確立済の Phase 1.A 命名規約)

PC-1 Exit Criteria literal 「indra/llrender/codegen_ubo/<UBO 識別子>.h 生成」は **意図充足** = build dir 配下生成 + cmake include path 経由 host-side 参照可能 (= source tree に generated file を撒かない方針整合)。

---

## §3 PC-1 Exit Criteria 充足検証

| Exit Criteria (= prep §3.1 PC-1 row) | 充足 |
|---|---|
| (a) `indra/llrender/codegen_ubo/<UBO 識別子>.h` 生成 | ✅ (= `build-linux-x86_64/codegen/ubo/ubo_layout_global_reflectionprobes.inl` 生成、§2.5 path 説明) |
| (b) std140 size 256B 倍数 padding 確認 | ✅ (= `Global_ReflectionProbes_SIZE = 256u`、§2.3) |

**Phase 2 本実装 layout 互換性 契約** (= 09 §4.2、PC-1 で確定する不可触構造):
- **set/binding**: set=0 / binding=3 (= 06c §3 接合表機械決定)
- **buffer size**: 256B (= 現状 placeholder 1 vec4 std140=16B padded、Phase 2 で member 群追加時 256B 倍数維持契約)
- **PSO layout**: VkPipelineLayoutCreateInfo の descriptor set layout 列 (= chapter 07 §11 既存設計依存、PC-7 で配線時確定)
- 上記 3 項は **Phase 2 で再利用される契約済構造**、shell の member 定義 / dirty 判定 / flush logic のみ Phase 2 で全面書換可

---

## §4 残 strict 線形 (= Phase 1.C 内、prep §3.2)

```
✅ PC-0 (Q1) AYA 確定 = UB_REFLECTION_PROBES + per-frame + Template A + 06c §3 接合表機械決定 (2026-06-04)
✅ PC-1 = shell blueprint codegen emit (本 commit 完結)
  → ⏳ PC-2 = test UBO shell C++ 接続 (= Phase 1.B 完了済 setter から test UBO 識別子 + binding 取出経路成立、空 dirty flag set)
  → ⏳ PC-3 = (W2) sAssetUboPool 起動時 prealloc N=64 + grow chunk 64
  → ⏳ PC-4 = (R1/RB) ring buffer 起動時 4 MB / 上限 16 MB + cvar AYARingBufferSizeMB
  → ⏳ PC-5 = (PSC) PSO cache ~/.ayastorm_x64/cache/pipeline_cache.bin 上限 64 MB
  → ⏳ PC-6 = 5 cadence 全経路 update site 実装
  → ⏳ PC-7 = vkCmdBindDescriptorSets 通電
  → ⏳ PC-8 = full viewer build + 起動 verify
  → ⏳ PC-N = Phase 1.C Exit Criteria 検証 + complete marker
→ ⏳ Phase 2 = Global_ReflectionProbes 本実装 migration (= 09 §5.2 Template A Phase 2)
```

---

## §5 r41 milestone state (= 本 PC-1 commit 完結時点)

| Phase | 状態 |
|---|---|
| Phase 1.A | ✅ 章クローズ (= `fe2f3a81c6`) |
| Phase 1.B (host-side) | ✅ complete (= `35c4be1046`) |
| (Z) AYAstorm r20 SSS verify | ✅ complete (= `7401feeb1f` + `4dde489ec4`、PR #130) |
| (W) 上流 uniform4iv bug fix | ✅ complete (= `5aadf174f2` + `2a06e12f44`、PR #131) |
| (Y) Phase 1.C prep | ✅ complete (= `e3f24f7a2a` + `b8a37d078b` PC-0 確定値 4 件 record) |
| **Phase 1.C 実装 PC-0** | ✅ 確定 (= 2026-06-04 AYA literal「Claude 推奨で OK」) |
| **Phase 1.C 実装 PC-1** | ✅ **本 commit 完結** (= shell blueprint emit + 256B padding 確認 + unittest 130/130 PASS) |
| Phase 1.C 実装 PC-2..PC-N | ⏳ 次 session 引継 |
| (W) (b) upstream LL PR | ⏳ ayastorm-release work 時判断 (= `project_uniform4iv_upstream_pr_deferred`) |

---

## §6 self-verify 9 観点 (= commit 前確認)

| # | 観点 | 確認方法 | 期待 |
|---|---|---|---|
| (1) PC-1 Exit Criteria 2 項全充足 | (a) emit ✅ + (b) 256B padding ✅ (§3) | ✅ |
| (2) shell block name + set/binding + size + cadence routing 全 spec 整合 | §2.1 + §2.3 + 06c §3 接合表 + main.py:71 | ✅ |
| (3) codegen 走行 EXIT 0 + ERROR・WARNING 0 件 | §2.2 log literal | ✅ |
| (4) aggregate 5 file 全 reflect | §2.3 table | ✅ |
| (5) unittest 130/130 PASS | §2.4 literal | ✅ |
| (6) codegen output path drift 反映 (= prep §9 / §3.1 PC-1 row literal vs 実 path) | §2.5 説明 | ✅ |
| (7) Phase 2 本実装 layout 互換性 契約明示 | §3 契約 3 項 (set/binding + size + PSO layout) | ✅ |
| (8) commit 内容 = 1 new blueprint .glsl + 1 new handoff doc + Co-Authored-By 不在 | git status + 本 commit 段 | ✅ (本 commit 段) |
| (9) `feedback_no_scope_shrink` 遵守 (= PC-1 literal scope 完全実施、build dir 出力 / unittest verify 含む) | §2.1 + §2.2 + §2.3 + §2.4 全実施 | ✅ |

**改変対象** (= 本 commit):
- new: `indra/newview/app_settings/shaders/aya_r41_blueprints/set0/global_reflection_probes.glsl` (= 25 line shell blueprint)
- new: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-1.md` (= 本 doc)
- `build-linux-x86_64/codegen/ubo/` 配下出力は **commit 対象外** (= build artifact、`.gitignore` 配下)

---

## §7 引き継ぎ memory (= 次 session PC-2 着手時参照、既存 memory pointer)

- `project_ayastorm_r41_vulkan_migration` (= r41 milestone state pointer)
- `project_ayastorm_r41_design_principles` (= 2 大原則: 上流取込やすさ + Core 分散実現)
- `project_r41_phase1b_vulkan_host_gate` (= GATE-B = `mUseUBO` runtime flag 単独、`#ifdef LL_VULKAN_GLSL` C++ では使わない)
- `feedback_ubo_migration_one_at_a_time` (= UBO 化作業は 1 つずつ、大塊バッチ禁止、cold launch 検証挟む)
- `feedback_handoff_minimal_pre_req_read` (= 次 session pre-req は最小 3 件 + pinpoint 別記)
- `feedback_doubt_self_first` (= AYA 報告を尊重、自分の改変を疑う)
- `feedback_self_bug_no_defer_option` (= 自作 bug 先送り禁止、ただし上流 bug 別)
- `feedback_no_scope_shrink` (= PC-* literal scope 全扱う、shell だから縮小許可されない)
- `feedback_release_branch_workflow` (= release branch 直 commit せず、feature branch 経由)
- `feedback_no_auto_commit` (= AYA 明示指示後 commit)
- `feedback_no_claude_coauthor` (= Co-Authored-By 行不在)
- `feedback_self_verify_before_handoff` (= 9 観点 self-verify 全 ✅)
- `feedback_build_only_verified` (= PC-1 は実 codegen 走行 + unittest 130/130 PASS で literal 検証取得、机上推論せず)
- `feedback_proactive_handoff` (= context 残量監視で能動 handoff、AYA 指示待たず周回境界で起案)

---

## §8 次 session 着手 1 line

**「前 session で PC-1 = `Global_ReflectionProbes` shell blueprint emit complete (= `set=0 binding=3 size=256B cadence=SINGLETON`、unittest 130/130 PASS)。本 session = **PC-2 着手** = test UBO shell C++ 接続 = Phase 1.B 完了済 30 setter Vulkan path 分岐 (`llglslshader.cpp:2480-2563, 3006-3079`) から `Global_ReflectionProbes` 識別子 + binding 取出経路成立 + `if (mUseUBO)` redirect 内で `forwardToUboUpload` 空 dummy buffer 書込 PASS (= 06b §5.1 経路通電のみ確認、render 出力は OpenGL path のまま)。必読 3 件 = (1) 本 handoff doc 全文 + (2) prep doc §3.1 PC sub-task table + §4.1 PC-0 確定値 + (3) `llglslshader.cpp:2480-2563, 3006-3079`。Phase 1.C Exit = 5 cadence 全経路で `vkCmdBindDescriptorSets` 空 dummy buffer 成功 + render 出力は OpenGL path のまま。」**
