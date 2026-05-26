# Handoff: 案 O Worker A path SIGSEGV 真因特定 — DIAG 計装後の再仕切り

**作成**: 2026-05-26 / **branch**: `feature/ayastorm-r40-cpu-perf` / **状態**: Commit 2 (Worker A 実装) 着地済、Commit 3 (再計測 + 50% gate 判定) を出すための smoke が再現性 SIGSEGV で止まっている。

## TL;DR

- `/loop verify perf CSV captured for Day 1-2 doOcclusion Group N and integrate into 04 spec` は **PASS 安定** (`04-observed-hot-path-map.md` §4.2.i に CSV / 数値 / 判定すべて固定済、再現可能)。本周回ではこちらに作業は不要。
- 仮説 2 連続外し済 → 推論を続けず、**DIAG 計装の coverage を広げて crash 窓を取りに行く** または **coredump 取得経路を確保する** に切替。
- 既に DIAG `LL_WARNS` を 4 ファイルに展開済、Commit 3 を出す前に **必ず除去** (`feedback_remove_verification_logs.md`)。

## /loop タスク (PASS)

- CSV: `tests/aya-gui/artifacts/20260526-111600-414334/AYAstorm-perf-18pass-baseline.csv` (406 MB、13.09 M rows、38,499 frames、wall_ms 873–153,133 = 152.3 s)
- spec §4.2.i: parent doOcclusion_reflectionProbes 2,116,598 us / 8,192 count / 258.374 us per-call、Σ rmdo_* = 22.70%、parent−Σ = 77.30%。Layer 9 不要判定、Phase 1.2 input まで反映済。
- 次 session でも /loop が来たら再 verify は spec 単独で完結 (CSV を再 grep する必要なし)、ただし **04 spec の §4.2.i を編集する別作業が走ったら整合再確認**。

## 直前まで確定している事実

1. **alignment 仮説 dead** (前 session)
2. **LLPointer 経由 refcount race 仮説 dead** (本 session 前段)
   - `llpointer.h:87` move ctor は raw ptr 強奪、`unref()` 未呼出
   - move-assignment は copy-and-swap、これも refcount touch なし
   - `LLRefCount::mRef` は非 atomic だが、worker thread はそもそも触らない経路
3. **worker thread 本体は健全** — DIAG marker で iter 0〜29 + 200 走破確認 (`tests/aya-gui/artifacts/20260526-212214-585237/viewer.log`)
4. **pass2_gl も健全** — iter 0〜59 全て `kind=1 (NEED_GEN_QUERY) query=0` で shader/cubeVB ポインタ valid
5. **MgrApply ai=2+ で probe pointer が 2 値 alternation** している (iter 17 → 0x733919510d40, iter 18 → 0x73391bf98700, iter 19 → 0x733919510d40, …) — 1-frame-lag で snap 出所が前 frame の状態を反映している model としては想定範囲、UAF を示唆する material ではない。**ただし要再確認候補**。

## crash の現観測

- Smoke: `AYA_TEST_SLURL="secondlife://util.aditi.lindenlab.com/secondlife/Bonifacio/179/69/26" ./launch_session.sh run_perf_case_O.py --worker-enabled 1 --duration 25 --csv-name diag-w1.csv` (cwd `tests/aya-gui/`)
- artifact: `tests/aya-gui/artifacts/20260526-212214-585237/`
- 最後の DIAG 行: `12:22:38Z worker iter=200 frame=29177 entries=0` (Hero empty snap)
- 1 秒 silence → `12:22:39Z status: error` → process 死
- `FATAL`/`SIGSEGV`/`abort` などの fatal log line は **viewer.log に 出ていない**
- coredump 未取得 (`ulimit -c = 0`、`kernel.core_pattern` = apport)

## 改修済ファイル (DIAG 込)

| file | 状態 | DIAG 除去要 |
|---|---|---|
| `indra/newview/llreflectionocclusionworker.cpp` | new | yes (`LL_WARNS("AYAOccWorker") [DIAG]` 2 箇所) |
| `indra/newview/llreflectionocclusionworker.h` | new | (header に DIAG なし) |
| `indra/newview/llreflectionmapmanager.cpp` | modify | yes (`s_diag_mgr_iter` ブロック、行 1635 付近) |
| `indra/newview/llreflectionmapmanager.h` | modify | (DIAG 無、interface 変更のみ) |
| `indra/newview/llheroprobemanager.cpp` | modify | yes (`s_diag_hero_iter` 相当、行 660 付近) |
| `indra/newview/llheroprobemanager.h` | modify | (DIAG 無) |
| `indra/newview/llreflectionmap.cpp` | modify | yes (`s_diag_pass2_iter`、行 449 付近) |
| `indra/newview/llreflectionmap.h` | modify | (DIAG 無) |
| `indra/newview/CMakeLists.txt` | modify | (DIAG 無、worker .cpp/.h 追加のみ) |
| `indra/newview/app_settings/settings.xml` | modify | `AYARenderOcclusionWorkerEnabled` U32 追加 |

## 次 session 推奨 (3 候補、AYA 選択)

### (a) DIAG 拡張 + 再 smoke (低リスク、build 25 分)

- `iter % 200 == 0` を `iter % 5 == 0` に変更し連続 sample
- pass2_gl の 60 件 cap を撤廃、ただし **kind=2 (POLL_AND_MAYBE_PUSH) 専用 log** にして volume 抑制
- `LLPipeline::doOcclusion()` の `mReflectionMapManager.doOcclusion()` 直後に「Mgr frame done」marker 追加
- 再 smoke で crash 直前数 frame の trace を確実に拾う

### (b) coredump 取得 + gdb bt (中リスク、設定変更要)

- `ulimit -c unlimited` + `sudo sysctl -w kernel.core_pattern=/tmp/core-%e-%p` を session 内一時設定
- launcher (`run_perf_case_O.py` / `launch_session.sh`) に同 export 仕込み
- 再 smoke で coredump 取得、`gdb /home/ishikawa/ayastorm/do-not-directly-run-ayastorm-bin /tmp/core-*` で bt
- 利点: 1 撃で crash frame & spot 確定。欠点: apport 解除の system 変更が必要

### (c) Worker disable で baseline 計測のみ先行 (回避策)

- 案 O 実装は保持、`AYARenderOcclusionWorkerEnabled=0` で **legacy path baseline CSV** だけ先に取る
- worker=1 経路の crash 解析は別チケットに分離、Phase 2 進行は legacy CSV を基準値として固定
- 利点: Commit 3 の半分 (baseline 側) は前に進む。欠点: 50% gate 判定は完全に保留 (本来の Commit 3 目的を満たさない)

## AYA さんへの相談ポイント

- (a)/(b)/(c) いずれを取るか
- (b) を取る場合、system sysctl 変更を本 PC で許容するか (本来 user dir 内で完結したいが core_pattern は host setting)
- いずれにせよ Commit 3 提出前に DIAG `LL_WARNS` 除去 + settings.xml の `AYAPerfLogEnabled` / `AYARenderOcclusionWorkerEnabled` 既定値を確認

## 参照済 reading list (次 session 着手前に読み返さなくて OK の確定知識)

- `indra/llcommon/llpointer.h:87` (noexcept move ctor 確定)
- `indra/llcommon/llrefcount.h:81` (`mutable S32 mRef` 非 atomic 確定)
- `indra/newview/pipeline.cpp:3174-3220` (caller GL state setup 確定)
- 本 spec ディレクトリ `phase2-case-O-plan.md` §2 / §3 / §4 (Snapshot/Apply 設計確定)
