# Handoff: Phase 1.1 完成 → Phase 2 実装着手

**作成**: 2026-05-26 / **branch**: `feature/ayastorm-r40-cpu-perf` (origin から 6 ahead、AYA 側 push 待ち)

## 直前 commit 2 件 (本 session 末尾)

| sha | type | 内容 |
|---|---|---|
| `aed520c761` | `chore` | r31 → r40 採番きりなおし全 ref 更新 (16 files、xui × 12 lang + login.xml + 2 handoff + llviewerwindow.cpp comment) |
| `d9fbecccb6` | `perf(zone)` | Layer 6 全周 + Layer 7-8 配線 (Group L+M+N+O) + Phase 1.2 Y-refined 確定 (12 files、1949+/104−、05/06 spec 新規 + 04 結果整理) |

その前 4 commits は本 session 起点 (`99c771bbb6 docs(perf): r31 doOcclusion drill 13-17 周目 handoff doc 追加` から `171d6b90d1 perf(zone): Layer 6 LLPipeline::doOcclusion 3 分割 zone 配線` まで)。

## AYA 側 push 待ち (本 session 末尾で提示済)

```bash
cd /home/ishikawa/work_firestorm/phoenix-firestorm
git push -u origin feature/ayastorm-r40-cpu-perf
git push origin --delete feature/ayastorm-r31-cpu-perf
git branch -vv | grep cpu-perf
git ls-remote --heads origin | grep cpu-perf
```

push 完了後、新 session で `git fetch && git status` で同期確認推奨。

## Phase 1.1 完成判定 (handoff-week1-deepdive-to-phase12.md より)

3 条件すべて充足:
- 02-offload-feasibility §A1-A14 deep-dive ✓
- Layer 6 全周回 (Group M 配線 + 18 周目 CSV) ✓
- Layer 8 全周回 (Group N + O 配線 + 19 周目 CSV) ✓

**Phase 1.2 (05-core-assignment-plan.md Y-refined 改訂版) 着手可。**

剥がし 3 候補で go 判定 (合計 ceiling 11.7-14.5 ms/frame):

| 候補 | 想定 worker | 想定 ceiling | 着手順序 |
|---|---|---|---|
| 案 O (doOcclusion async) | Worker A | 5.7-6.5 ms/frame | **1 番目** |
| 案 R-refined (renderShadow pre-cull) | Worker B | 2-3 ms/frame | 2 番目 |
| 案 P-refined (parallel cull) | Worker C | 4-5 ms/frame | 3 番目 |

詳細順序と依存関係は `05-core-assignment-plan.md` §Phase 2 参照。

## Phase 2 着手第一手 (Worker A occlusion offload)

**target**: `LLReflectionMapManager::doOcclusion(camera)` 関数全体を main thread から剥がす (`rmdo_*` を worker 側に切出すだけでは 22.70% しか取れない、04 §4.2.i Phase 1.2 input bullet 1 で確定)。

**worker 移送時の同期境界**:
- camera frustum (read-only snapshot)
- probe occlusion state 更新 (write-back queue)
- GL context 制約 (GL 呼び出しは main thread 専有、worker からの query 発行は不可)

**現実解**: probe visibility 判定 / occlusion 結果消費を worker 側、`glBeginQuery`/`glEndQuery` 発行のみ main thread に残す split。

設計詳細は 06 spec §Worker pool infra と 05 spec §Phase 2 で詰める。

## 未着手の小タスク (Phase 2 開始前 / 並行可能)

1. ~~**OCCLUDED grep finding の spec 反映**~~ — **完了 (2026-05-26、本 commit)**
   - 確定事項: OCCLUDED は既に `LLOcclusionCullingGroup::mOcclusionState[NUM_CAMERAS]` で per-camera 化済 (llvieweroctree.h:332)
   - `setOcclusionState(OCCLUDED, ...)` は llvieweroctree.cpp:1158 で `STATE_MODE_DIFF` (per-camera)、`STATE_MODE_ALL_CAMERAS` は llspatialpartition.cpp:310/964 の `DISCARD_QUERY` のみ
   - 含意: §6 OCCLUDED 分離 refactor 不要、案 P-refined prerequisite cost ≈ zero
   - 反映先 (確定):
     - 02-offload-feasibility.md §E.2 (RED → GREEN row、結論 / Phase 1.2 scope 更新)
     - 02-offload-feasibility.md §F.1 (P-refined YELLOW → GREEN)
     - 02-offload-feasibility.md §F.3 (旧 OCCLUDED risk 取り消し線 + `mState` 他 bit を新 risk として残し)
     - 02-offload-feasibility.md §F.2 / §F.4 (P-refined 着手順序の条件付 → 純 GO)
     - 05-core-assignment-plan.md §0 (Y-refined 改訂後の 2026-05-26 update note 追加)
     - 05-core-assignment-plan.md §1.2 (旧第 4 項目 strikethrough)
     - 05-core-assignment-plan.md §3.1 / §4.1 / §5.3 (P-refined GREEN 化、prerequisite 不要)
     - 05-core-assignment-plan.md §6 全体 (削除ノート + grep finding 出典表に置換)
     - 05-core-assignment-plan.md §7.1 / §7.2 / §7.3 (Phase 2 着手順序から OCCLUDED step 消去、3 件順序のみ)
     - 05-core-assignment-plan.md §9.2 (案 P-Q1 削除、案 P-Q1' `mState` 他 bit 再 grep を新規)
     - 05-core-assignment-plan.md §9.3 (削除ノート)
     - 05-core-assignment-plan.md §10.1 (項目 3 strikethrough、項目 5 `mState` cross-camera write site grep を追加)
     - 05-core-assignment-plan.md §10.2 (旧 OCCLUDED NG 取り消し線 + `mState` atomic コスト NG 条件に置換)
     - 05-core-assignment-plan.md §12 改訂履歴に "OCCLUDED grep finding 反映" 追記
     - 06-offload-feasibility-deep.md §2.2 結論末尾に 2026-05-26 補足追加

2. **次回 perf capture 命名規約変更**
   - 現状: `--csv-name 18pass-baseline` のような自由名 → 同名衝突 (45 MB M-only / 406 MB M+N) が発生済
   - 改: `--csv-name day12-group-N.csv` のような **計測対象 zone 明示** 形式
   - 反映: AYA 側 capture 運用、コード変更不要 (run_perf.py の `--csv-name` 引数で対応済)
   - 04 §4.2.i に「次回から day12-group-N.csv 形式へ移行」を記載済

## 制約 (新 session で必ず守る)

| 項目 | 制約 |
|---|---|
| **tests/aya-gui/** | **commit 禁止** (login info 混入リスクで AYA 判断、local only)。`tests/` は untracked のまま維持 |
| **.claude/** | **commit 禁止** (Claude config、local) |
| **r40 章 scope** | 微小チューニング / hot path 短縮 / query 削減 / pool 化など「main 内で速くする」打ち手は提案も禁止 (memory: `feedback_r40_no_micro_tuning.md`)。**剥がし (別 thread 移送) のみ** |
| **commit** | AYA 明示指示まで commit しない (memory: `feedback_no_auto_commit.md`)。verify ログは commit 前に除去 (memory: `feedback_remove_verification_logs.md`) |
| **push** | AYA 側で実行 (memory: `feedback_release_flow.md`)、Claude は local commit まで |
| **検証用 LL_INFOS hook** | verify PASS 後に出荷物から除去してから commit |

## 関連 memory (本 session で参照したもの)

| memory | 用途 |
|---|---|
| `project_ayastorm_r40_cpu_parallel.md` | 章 thesis (剥がし候補抽出) |
| `feedback_r40_no_micro_tuning.md` | 短縮系打ち手禁止 (絶対条件) |
| `feedback_perf_map_bfs_drill.md` | 計測地図 BFS 層 drill |
| `feedback_admit_unknown.md` | 仮説 2 連続外れたら実データ取得に切替 |
| `feedback_build_only_verified.md` | 効果未確認 commit を積まない |
| `feedback_proactive_handoff.md` | 周回境界で handoff 作成 (本 doc) |
| `feedback_one_step_at_a_time.md` | 1 メッセージ 1 アクション |
| `feedback_explanation_lead_with_conclusion.md` | 結論ファースト |
| `feedback_full_commands.md` | コマンドは copy-paste 可能形 |

## /loop 状態 (本 session 末尾)

dynamic mode で fallback heartbeat 1800s pending (verify perf CSV captured for Day 1-2 doOcclusion Group N and integrate into 04 spec)。**verify は完了**、artifacts/ 新 capture 監視用 idle tick として残存。新 session 移行時に **自動的に解除される** (ScheduleWakeup は session-local)、明示的な TaskStop 不要。

## 新 session 起点 prompt 案

```
docs/specs/ayastorm-cpu-perf/handoff-phase11-to-phase2.md を読んで、Phase 2 着手第一手 (案 O: Worker A occlusion offload) の実装計画を 05 spec § と 06 spec § を参照しながら立ててください。実装着手前に着手プランを AYA に提示し、合意を取ってから着手。
```

または OCCLUDED finding spec 反映を先に片付けたい場合:

```
docs/specs/ayastorm-cpu-perf/handoff-phase11-to-phase2.md の「未着手の小タスク」1 (OCCLUDED grep finding の spec 反映) を実行してください。02 §F.3 / 05 §6 / 06 の該当節を grep で特定し、§6 refactor 不要 / 案 P-refined prereq ≈ zero を反映。
```
