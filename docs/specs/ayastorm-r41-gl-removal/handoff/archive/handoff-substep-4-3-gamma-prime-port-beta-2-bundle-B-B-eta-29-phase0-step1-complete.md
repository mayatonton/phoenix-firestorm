# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-29 Phase 0 Step 1 complete

**作成**: 2026-06-03
**前 handoff**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-beta-revise-complete.md` (= η-28 完了 = 設計 chapter 04+08+09 深化 + bridge phase 範式完遂)
**branch**: `feature/ayastorm-r41-gl-removal`
**最新 commit (本 handoff 時点)**: `d1099b4d64` (= Phase 0 Step 1 doc 反映 + `.gitignore tests/` 追加)

---

## §0 state 一行 summary

η-29 **Phase 0 Step 1 = Pre-hook Static Analysis** 完了 (5 件 / chapter 10 27 件化 / `indra/` 改変ゼロ) → 次は **Step 2 = LL_INFOS hook 配線実装** (= `indra/` 初解禁、`llglslshader.cpp` + CMake 3 ファイル編集)。AYA 確認待ち事項なし、Step 2 着手即可。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。最初の session 入りでは **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read (offset/limit) する。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-29-phase0-step1-complete.md` | 全文 | 本 handoff (= 現 state + 次着手 spec) |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/06a-prep-phase0-measurement.md` | §2.3.1 / §2.3.2 / §2.4 | Step 2 実装する hook code shape + 30 setter 挿入位置 + CMake gate |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §14.3 (Stage 1 self-check 9 項目) | Phase 0 計測 entry condition 確認 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `indra/llrender/llglslshader.cpp` | §2.2.1 整数 index setter 17 method (line 2141-2538) / §2.2.2 LLStaticHashedString setter 13 method (line 2617-2844) — 行番号は HEAD 時点の参考、実装時 grep で再確認 |
| `indra/newview/llappviewer.h` | `extern U32 gFrameCount;` (line 422) — §2.3.1 helper `#include` |
| `indra/cmake/00-Common.cmake` | §2.4 CMake `option()` 追加先候補 — 実装入口で配置先 grep 再確認 |
| `docs/specs/ayastorm-r41-gl-removal/design/10-open-questions.md` | §1.5 (Q26-MUL) (Q27-CONFL) — Step 1 で新規登録した sub-question (Phase 1.A 入口での AYA 判断対象) |

---

## §2 Phase 0 Step 1 = 完了成果 (本 session で済んだこと)

### §2.1 commit log

```
d1099b4d64 docs(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-29 Phase 0 Step 1 Pre-hook Static Analysis 結果反映 = (E') 5 UBO 同 binding=0 全件 set=2 確定 + C++ name attach 0 件 + V/F 同 program 共存 risk dormant 検出 / (F) MaterialUBO 52 vs Legacy 1 member 完全別物 + class3 link conflict dormant 検出 → F2 narrowing 確定 / (P1)(P2)(P3)(P4) 全 4 件解消 + gFrameCount 流用で §2.3.3 配線不要 / chapter 10 §1 27 件化 (Q26-MUL + Q27-CONFL 新規 2 件) + §1.5 (F) default F1→F2 narrowing + (Q26-MUL) MaterialUBO_Legacy rename + (Q27-CONFL) V/F 同 binding 共存 解消方針 sub-question 切出 / .gitignore tests/ 追加 (AYA local test artifacts 誤 commit 防止) / indra/ 改変ゼロ厳守継続 (Phase 0 Step 1 = pre-hook static 部、Step 2 hook 配線で初解禁予定)
```

### §2.2 成果 (= 設計 doc 内 5 件解消)

| # | 解消対象 | 結論 | 反映先 |
|---|---|---|---|
| 1 | (E') 同 binding=0 5 UBO grep | 全件 `set=2 binding=0 std140`、C++ name attach 0 件、同 program 内 V+F 共存 risk **dormant** (= `LL_VULKAN_GLSL` 現 OFF) 検出 | `06a-prep §3.5.1-§3.5.7` |
| 2 | (F) MaterialUBO vs MaterialUBO_Legacy 比較 | 名前重複だが member 完全別物 (52 vs 1 member, 160B vs 64B、`set=1 binding=0` 同位置)、`mShaderLevel=class3` 時 link conflict **dormant** 検出、**F2 narrowing 確定** (= `_Legacy` rename / set ずらし) | `06a-prep §4.6.1-§4.6.6` |
| 3 | (P1) LL_INFOS class 名 `"UBO_CADENCE"` 衝突確認 | 既存 grep 0 件 = 衝突なし | `06a-prep §2.3.1 規律` + §7 P1 |
| 4 | (P2) 30 setter line 番号 HEAD 時確認 | §2.2.1 / §2.2.2 表に line 反映、実装時 grep 再確認規律明記 | §7 P2 |
| 5 | (P3)(P4) frame counter 設計 | 既存 `gFrameCount` (`llappviewer.cpp:369`, `llappviewer.h:422`) 流用 = **§2.3.3 frame counter 配線 不要マーク**、helper は `#include "llappviewer.h"` 追加のみ | `06a-prep §2.3.1` update + §2.3.3 不要マーク + §7 P3/P4 |

### §2.3 sub-question 新規登録 2 件 (= Phase 1.A 入口 AYA 判断対象)

| ID | 内容 | default 提案 | 登録先 |
|---|---|---|---|
| (Q26-MUL) | MaterialUBO_Legacy rename + class3 link conflict 解消方針 | MUL-A1 (= rename `MaterialUBO_Legacy` → 別名) + MUL-B1 (= 解消は Phase 1.A 入口) | `10-open-questions.md §1 表 row 26` + §1.5 |
| (Q27-CONFL) | (E') V/F 同 program 内 set=2 binding=0 共存 risk 解消方針 | CONFL-A1 (= 4 V-stage + 1 F-stage UBO 全件 inventory 化) + CONFL-B2 (= shader 分割でなく binding ずらし or set 帯分離) | `10-open-questions.md §1 表 row 27` + §1.5 |

### §2.4 chapter 10 数値 update

- §1.0 header: 25 件 → **27 件** (Q26-MUL + Q27-CONFL 追加)
- 未判断: 21 → **23 件**
- §1.5 件数: 1 → **3 件**
- §5 #1 / #5 / §7.3 cross-ref 全反映済

### §2.5 副産物 (今 session 中で発生、commit 済)

- `.gitignore` に `tests/` を追加 (AYA local test artifacts = security info 含む、誤 commit 防止)

---

## §3 次着手 = Phase 0 **Step 2** = LL_INFOS hook 配線実装

### §3.1 scope (= literal)

**指示語省略主語禁止 (= `feedback_no_bare_reference_ids` + `feedback_confirm_referent_before_acting`)**。以下 3 ファイル + 1 CMake editing が Step 2 全件:

| # | 編集対象 | 内容 | 参照 spec |
|---|---|---|---|
| 1 | `indra/llrender/llglslshader.cpp` 上部 (anonymous namespace 内) | helper 2 個 (`ayaUboHookOnSetterByIndex` / `ByHashed`) + macro 2 個 (`AYA_UBO_HOOK_IDX` / `_HASH`) + `#include "llappviewer.h"`、全て `#ifdef AYASTORM_UBO_CADENCE_HOOK` gate | `06a-prep §2.3.1` |
| 2 | 同 `llglslshader.cpp` 30 setter body 先頭 | macro 1 行挿入 × 30 method (= integer index 17 method §2.2.1 + LLStaticHashedString 13 method §2.2.2)、`mProgramObject` check より **前**、既存 body 1 文字も改変しない | `06a-prep §2.3.2` |
| 3 | `indra/cmake/00-Common.cmake` (= 配置先は実装入口で grep 確認、変更影響最小箇所選択) | `option(AYASTORM_UBO_CADENCE_HOOK "..." OFF)` + `if(...) add_definitions(-DAYASTORM_UBO_CADENCE_HOOK=1) endif()` | `06a-prep §2.4` |
| **不要** | ~~`indra/newview/llappviewer.cpp` frame counter 配線~~ | **Phase 0 Step 1 で gFrameCount 流用に簡素化済 (= §2.3.3 不要マーク)** | `06a-prep §2.3.3` |

### §3.2 規律 (= 実装 phase 入口 self-check)

1. **release build 混入禁止**: default OFF、`#ifdef` gate 外に出さない、release tag build には混入しない (= flag 未指定で diff が `#ifdef` 内に閉じる確認)
2. **30 setter 挿入は 1 文字も既存 body 改変しない**: 検証完了後 §2.8 除去 protocol で diff が hook 行のみに閉じる = 除去 commit が機械的に作れる
3. **frame counter は `gFrameCount` 直接参照**: 専用 atomic counter / §2.3.3 配線追加は **やらない** (Step 1 解消結果)
4. **計測専用 build flag は `-DAYASTORM_UBO_CADENCE_HOOK=ON`**: AYA Linux で build → §2.6 3 scenario 90 frame 計測
5. **build 失敗時の commit 禁止**: Step 2 完了 = AYA Linux で `-DAYASTORM_UBO_CADENCE_HOOK=ON` build PASS + flag OFF build PASS の 2 軸確認後に commit
6. **`indra/` 改変 = Step 2 で 初解禁**: ただし範囲は本 §3.1 表 3 ファイル literal のみ、scope 拡張禁止 (= `feedback_no_scope_shrink` 逆 = scope 拡張も禁止)

### §3.3 Step 2 完了 = Step 3 移行条件

Step 2 完了 = 以下 3 件 PASS で確認:
- (i) flag OFF build PASS (= 通常 build 影響なし)
- (ii) flag ON build PASS (= 計測専用 build がリンク通る)
- (iii) `git diff` で hook gate 内に全変更が閉じている (= `#ifdef AYASTORM_UBO_CADENCE_HOOK` の外側に AYA 由来 line ゼロ)

3 件 PASS 後、commit → **Step 3 = AYA Linux 実機計測 (= §2.6 3 scenario 90 frame)** へ移行。Step 3 は AYA 側 build + 起動 + log 採取 session、Claude は Step 4 (= log 解析 → §5) で復帰。

---

## §4 self-verify (= 本 handoff 起こした時点の整合性)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) 前 handoff (`...phase2d-beta-revise-complete.md`) からの遷移整合 | 「次 phase = η-29 Phase 0 実機計測 phase」着手 ready state が本 handoff 起点と一致 | ✅ |
| (2) commit `d1099b4d64` 確認 | `git log --oneline -1` で本 handoff 起こす前の HEAD と一致 | ✅ |
| (3) `indra/` 改変ゼロ厳守 | 本 session 中 `indra/` 配下 0 file 編集 (3 doc + `.gitignore` のみ) | ✅ |
| (4) `feedback_design_phase_no_code_write` 準拠 | Phase 0 Step 1 = pre-hook static = doc 化のみ = `indra/` 解禁前 phase 該当 | ✅ |
| (5) chapter 10 数値整合 (= 27 件 + 内訳) | §1.0 header / 表 / count / 未判断 / §1.5 件数 / §5 cross-ref / §7.3 全件 update 済 | ✅ |
| (6) `feedback_handoff_minimal_pre_req_read` 準拠 | 本 §1.1 = 3 件のみ列挙、§1.2 = pinpoint Read 用 reference 分離、全件列挙廃止 | ✅ |

---

## §5 引き継ぎ済の memory (= 次 session も活きる)

- `feedback_handoff_minimal_pre_req_read` (= 本 handoff §1 構造の根拠)
- `feedback_design_phase_no_code_write` (= Step 2 で `indra/` 初解禁、それまで設計 phase 規律継続)
- `feedback_no_scope_shrink` (= Step 2 literal 3 ファイル 範囲 厳守、AYA 指示なく拡縮しない)
- `feedback_no_claude_coauthor` (= 全 commit 共著行禁止)
- `project_ayastorm_r41_vulkan_migration` (= r41 章 active pointer)
- `project_ayastorm_r41_design_principles` (= upstream 取込容易性 + core 並列化容易性の 2 大設計原則)

---

## §6 次 session 着手 1 line

「Phase 0 Step 2 着手 = `06a-prep §2.3.1` / §2.3.2 / §2.4 の literal に従って `indra/llrender/llglslshader.cpp` (hook helper + 30 setter macro) + `indra/cmake/00-Common.cmake` (option) を編集、flag OFF/ON 双方 build PASS 確認後 commit、`indra/` 改変は本 3 ファイル literal の外に出さない」
