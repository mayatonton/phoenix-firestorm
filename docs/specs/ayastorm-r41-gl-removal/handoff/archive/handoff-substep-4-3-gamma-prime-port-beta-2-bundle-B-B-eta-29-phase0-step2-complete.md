# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-29 Phase 0 Step 2 complete

**作成**: 2026-06-03
**前 handoff**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-29-phase0-step1-complete.md` (= η-29 Phase 0 Step 1 = Pre-hook Static Analysis 完了)
**branch**: `feature/ayastorm-r41-gl-removal`
**最新 commit (本 handoff 時点)**: `c27733ae79` (= Phase 0 Step 2 = LL_INFOS hook 配線実装、`indra/` 初解禁 phase 完了)

---

## §0 state 一行 summary

η-29 **Phase 0 Step 2 = LL_INFOS hook 配線実装** 完了 (= `indra/llrender/llglslshader.cpp` + `indra/cmake/00-Common.cmake` の 2 file editing、flag OFF/ON 双方 build PASS、`#ifdef AYASTORM_UBO_CADENCE_HOOK` gate 内に全変更閉込) → 次は **Step 3 = AYA Linux 実機計測 session** (= AYA 側 build + 起動 + log 採取、Claude は Step 4 = log 解析 → §5 反映で復帰)。Claude 即着手対象なし、AYA 計測完了待ち state。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。最初の session 入りでは **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read (offset/limit) する。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-29-phase0-step2-complete.md` | 全文 | 本 handoff (= 現 state + Step 3 AYA session + Step 4 復帰地点 spec) |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/06a-prep-phase0-measurement.md` | §2.5 (build 手順) / §2.6 (3 scenario 90 frame) / §2.7 (log 採取 + 後処理) | Step 3 AYA 側手順 spec |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §14.3 (Stage 1 self-check 9 項目) | Phase 0 計測 entry/exit condition 確認 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `indra/llrender/llglslshader.cpp` | 上部 helper block (`#ifdef AYASTORM_UBO_CADENCE_HOOK` ~ `#endif` 直後の macro 定義) / 31 setter body 先頭 (= `AYA_UBO_HOOK_IDX("uniform*")` / `AYA_UBO_HOOK_HASH("uniform*")` 1 行) — 行番号は HEAD 時点で grep 再確認 |
| `indra/cmake/00-Common.cmake` | 末尾 `option(AYASTORM_UBO_CADENCE_HOOK ...)` block (= 9 line) |
| `docs/specs/ayastorm-r41-gl-removal/design/06a-prep-phase0-measurement.md` | §2.3.1 (= 次 session で `#include`→`extern` 修正対象、§7.4 副産物として下記 §2.3 に記録) / §2.2.2 (= 「13 method」typo を「14 method」に訂正、同 §7.4) / §5 (= log 解析 結果反映先、Step 4 復帰時) |
| `docs/specs/ayastorm-r41-gl-removal/design/10-open-questions.md` | §1.5 Q26-MUL / Q27-CONFL (= Phase 1.A 入口 AYA 判断対象、Phase 0 計測結果が判断材料の一部) |

---

## §2 Phase 0 Step 2 = 完了成果 (本 session で済んだこと)

### §2.1 commit log

```
c27733ae79 feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-29 Phase 0 Step 2 LL_INFOS hook 配線実装 = indra/llrender/llglslshader.cpp 上部に helper 2 個 + macro 2 個 + 31 setter body 先頭 macro 挿入 + indra/cmake/00-Common.cmake 末尾 option / 全 helper + macro 実体 #ifdef AYASTORM_UBO_CADENCE_HOOK gate 内 + #else 側 no-op `((void)0)` 定義 = flag OFF 時 setter body 挙動完全不変 / 副産物 1 = spec §2.3.1 #include layering 違反検出 → extern U32 gFrameCount; 直接宣言に置換 / 副産物 2 = 30 setter count は §2.2.2 prose typo 由来、grep で 17+14=31 件確定 / 3 完了条件 全件 PASS (i)(ii)(iii)
```

### §2.2 成果 (= literal scope 2 file editing)

| # | 編集対象 | 内容 | 結果 |
|---|---|---|---|
| 1 | `indra/llrender/llglslshader.cpp` 上部 (`using std::string;` 直後) | helper 2 個 (`ayaUboHookOnSetterByIndex` = index→reserved uniform name lookup → `LL_INFOS("UBO_CADENCE")` / `ByHashed` = LLStaticHashedString 直 emit) + macro 2 個 (`AYA_UBO_HOOK_IDX("setter_name")` / `_HASH`) + `extern U32 gFrameCount;` (= 後述 §2.3 副産物 1) / 全て `#ifdef AYASTORM_UBO_CADENCE_HOOK` gate 内、`#else` 側に no-op `((void)0)` macro 定義 | +40 line |
| 2 | 同 `llglslshader.cpp` 31 setter body 先頭 | macro 1 行挿入 × 31 method (= integer index 17 method §2.2.1 + LLStaticHashedString 14 method §2.2.2 = 「30」は §2.2.2 prose typo 由来、後述 §2.3 副産物 2)、`LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER` 行より前、既存 body 1 文字も改変なし | +31 line |
| 3 | `indra/cmake/00-Common.cmake` 末尾 (= `endif (LINUX OR DARWIN)` 直後) | `option(AYASTORM_UBO_CADENCE_HOOK "Enable r41 UBO cadence hook for Phase 0 measurement" OFF)` + `if(AYASTORM_UBO_CADENCE_HOOK) add_compile_definitions(AYASTORM_UBO_CADENCE_HOOK=1) endif()` + 説明 comment | +9 line |

### §2.3 副産物 2 件 (= 次 session で spec doc に反映予定)

| # | 副産物 | 検出経緯 | 対応 |
|---|---|---|---|
| 1 | spec `06a-prep §2.3.1` の `#include "llappviewer.h"` は layering 違反 | 初回 flag ON build (`bmht2adiw`) で `fatal error: llappviewer.h: No such file or directory` (= `llrender` library は dependency graph 上 `newview` の下層、`indra/newview/` は `llrender` の include path 外) | 実装側を `extern U32 gFrameCount;` 直接宣言に変更 (= `llappviewer.cpp:369` の `U32 gFrameCount = 0;` を最終 viewer link 時に resolve、設計意図同等) → flag ON build PASS (`bi02babo5`) / spec §2.3.1 本文の `#include` 例示を `extern` 例示に修正は **次 session で別 commit** (本 session は literal scope 2 file 厳守) |
| 2 | spec `06a-prep §2.2.2` prose の「13 method」は typo (= table 14 行) | 31 setter grep 全件確認 (= §2.2.1 17 行 + §2.2.2 14 行 = 合計 31 件) で発見 | 実装は **「全 setter cadence」原則** (= `feedback_no_scope_shrink` 拡張不可、ただし scope 縮小も不可) に従い 31 件全件挿入 / spec §2.2.2 本文「13 method」 → 「14 method」訂正は **次 session で別 commit** (副産物 1 と同 batch) |

### §2.4 検証 3 完了条件 全件 PASS

| # | 条件 | 確認方法 | 結果 |
|---|---|---|---|
| (i) | flag OFF build PASS | autobuild `ReleaseFS_open` 通常 build (= flag 未指定 = default OFF)、100% built target、package 作成成功 | ✅ (task `b1wg1hzvk` EXIT=0) |
| (ii) | flag ON build PASS | `-DAYASTORM_UBO_CADENCE_HOOK:BOOL=ON` 再 configure + build、100% built target、helper symbol link 通過 (= `gFrameCount` 最終 viewer link 解決確認)、package 作成成功 | ✅ (task `bi02babo5` EXIT=0) |
| (iii) | `git diff` で全変更が `#ifdef AYASTORM_UBO_CADENCE_HOOK` gate 内に閉じる | `git show c27733ae79 --stat` で 2 file modified、`llglslshader.cpp` の helper/macro/extern は全て gate 内、setter body macro 行は gate OFF 時 `((void)0)` 化、`00-Common.cmake` の `add_compile_definitions` は `if(AYASTORM_UBO_CADENCE_HOOK)` 内、AYA 由来 line で gate 外側に出ているもの **ゼロ** | ✅ |

---

## §3 次着手 = Phase 0 **Step 3** = AYA Linux 実機計測 (= AYA 側 session)

### §3.1 scope (= literal、Claude 出番なし)

**Claude 出番なし**。Step 3 は **AYA 側 build + 起動 + log 採取 session**。本 §3.1 は Step 3 entry condition + AYA 手順の **spec への pointer のみ**、Claude が代行しない。

| # | AYA action | 参照 spec |
|---|---|---|
| 1 | `-DAYASTORM_UBO_CADENCE_HOOK:BOOL=ON` で AYA Linux build (= 計測専用 build) | `06a-prep §2.5` |
| 2 | 3 scenario × 30 frame log 採取 (= cold launch / GLTF rez / shader-heavy) | `06a-prep §2.6` |
| 3 | log file (`~/.ayastorm_x64/logs/AYAstorm.log`) を Claude session に貼り付け or 抜粋共有 | `06a-prep §2.7` |
| 4 | 計測完了後、AYA 計測 build を default OFF build に戻す (= Persist=1 cvar 系は今回なし、cmake 再 configure のみで十分) | `06a-prep §2.5.4` (戻し手順) |

### §3.2 Step 4 (= Claude 復帰地点) 着手条件

AYA から以下 1 件が提示された時点で Step 4 着手:
- (a) `UBO_CADENCE` class line が含まれる log 抜粋 (= 3 scenario 分の生 log、min 90 frame 相当)

着手対象 (Step 4):
- (b) log 解析 (= per-frame / per-program / per-uniform setter call count 集計、§5 観察項目 7 件 fill)
- (c) `06a-prep §5` への結果反映 (= 観察 → Phase 1.A 入口判断材料化)
- (d) §2.3 副産物 2 件 (spec §2.3.1 `#include`→`extern` + §2.2.2 「13 method」→「14 method」) を **同 session 内別 commit** で spec doc 訂正

### §3.3 規律 (= Step 4 復帰時 self-check)

1. **literal scope 厳守**: Step 4 = log 解析 + §5 反映 + §2.3 副産物 2 件訂正、`indra/` 改変は **やらない** (= 計測用 hook 除去は §2.8 protocol、別 step)
2. **副産物 2 件 spec 訂正は本体解析 commit と分離**: AYA 認識合わせ容易化のため、log 解析結果反映 commit と spec typo 訂正 commit は別 commit
3. **§5 fill 後の判断は AYA に渡す**: Claude は計測結果の **客観整理まで**、Phase 1.A 入口判断 (= (E')(F)(Q26-MUL)(Q27-CONFL) 等) は AYA 判断対象
4. **計測 hook 除去 (= §2.8 protocol) は Step 5 で実施**: Step 4 完了後、別 session で `c27733ae79` を機械的 revert (= hook 行のみの diff) で計測 build 痕跡を main 系に残さない

---

## §4 self-verify (= 本 handoff 起こした時点の整合性)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) 前 handoff (`...phase0-step1-complete.md`) からの遷移整合 | 「次 = Step 2 着手即可」(前 §0) → 本 handoff 「Step 2 完了 → Step 3 AYA 移行」 起点と一致 | ✅ |
| (2) commit `c27733ae79` 確認 | `git log --oneline -1` で本 handoff 起こす前の HEAD と一致 (前 = `8579b09318`、本 = `c27733ae79`、間に Step 2 実装 commit 1 件のみ) | ✅ |
| (3) `indra/` 改変は literal 2 file 内に閉じる | `git show c27733ae79 --stat` = `indra/llrender/llglslshader.cpp` + `indra/cmake/00-Common.cmake` 2 file のみ、波及ゼロ | ✅ |
| (4) `feedback_no_scope_shrink` 準拠 | 30 setter spec を典拠に 30 件でなく、grep 実数 31 件 (= 17+14) を「全 setter cadence」原則優先で全件挿入、scope 縮小なし | ✅ |
| (5) `feedback_design_phase_no_code_write` 解禁範囲整合 | Phase 0 Step 2 = `indra/` 初解禁 phase 明示済 (前 handoff §3.1)、本 session 編集は本 phase 範囲内 | ✅ |
| (6) `feedback_handoff_minimal_pre_req_read` 準拠 | 本 §1.1 = 3 件のみ列挙、§1.2 = pinpoint Read 用 reference 分離、全件列挙廃止 | ✅ |
| (7) `feedback_no_claude_coauthor` 準拠 | `c27733ae79` commit message に `Co-Authored-By: Claude` 不在 | ✅ |

---

## §5 引き継ぎ済の memory (= 次 session も活きる)

- `feedback_handoff_minimal_pre_req_read` (= 本 handoff §1 構造の根拠)
- `feedback_design_phase_no_code_write` (= Step 2 で `indra/` 初解禁完了、以降 Step 4 = log 解析 doc 化のみ、Step 5 = 機械的 revert は別 phase)
- `feedback_no_scope_shrink` (= Step 2 31 setter 全件挿入の根拠、副産物 spec typo 訂正は別 commit に分離)
- `feedback_no_claude_coauthor` (= 全 commit 共著行禁止)
- `project_ayastorm_r41_vulkan_migration` (= r41 章 active pointer)
- `project_ayastorm_r41_design_principles` (= upstream 取込容易性 + core 並列化容易性の 2 大設計原則)

---

## §6 次 session 着手 1 line

「Step 2 commit `c27733ae79` 完了、AYA 側 Step 3 = Linux 実機計測 (`06a-prep §2.5/§2.6/§2.7` 3 scenario 90 frame log 採取) 待ち。AYA から `UBO_CADENCE` log 提示で Step 4 = log 解析 + §5 反映 + spec §2.3.1 `#include`→`extern` / §2.2.2 「13 method」→「14 method」副産物 2 件訂正 (本体解析と別 commit) 着手、`indra/` 改変なし」
