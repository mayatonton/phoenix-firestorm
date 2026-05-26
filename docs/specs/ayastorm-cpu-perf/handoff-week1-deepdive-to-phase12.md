# Handoff — Phase 1.1 完成 (1 週間 deep dive Day 1-7 完走) → Phase 1.2 着手待ち

**作成**: 2026-05-26 / **branch**: `feature/ayastorm-r40-cpu-perf` (旧 `feature/ayastorm-r31-cpu-perf`、2026-05-26 採番きりなおしで rename)

## 1. 現在位置 (one-liner)

> 1 週間 deep dive Day 1-7 完走 (Layer 8 計測地図埋め + 4 候補 POC + 統合判定)。Phase 1.1 完成、剥がし候補 3 件確定 (案 O / R-refined / P-refined、ceiling 合計 11.7-14.5 ms/frame)、案 Q drop 確定。**uncommitted、AYA 指示待ち**。次 session = `05-core-assignment-plan.md` 新規 (Worker A/B/C 割当 + 同期境界設計)。

## 2. 何が起きていたか (経緯 60 秒読み)

- 13-17 周目 (短縮系) 脱線 → AYA 強制 reset → r40 thesis = 剥がし候補抽出のみ
- 18 周目 (Layer 6 全周 / Group L+M) で Layer 1-6 地図完成
- AYA 自動化指示「君が自動でViewer立ち上げるすべを構築」→ `tests/aya-gui/run_perf.py` + LLURLDispatcher 経由 auto-login 実装、Aditi Bonifacio 固定 SLurl で自動 CSV 取得 infra 確立
- 19 周目 = 1 週間 deep dive 開始
  - Day 1-2 (#13): doOcclusion Layer 8 = rmdo_* 状態機械 3 分解 (Group N) → 04 §4.2.i
  - Day 2-3 (#12): vwDraw Layer 8 = `LLView::drawChildren()` parent-name gate 動的 zone (Group O) → 04 §4.2.j
  - Day 4-5 (#15): renderShadow refactor POC (案 R) → GL blocker で partial scope に refine
  - Day 5-6 (#16): L6-cull atomic POC (案 P) → OCCLUDED 分離 prerequisite に refine
  - Day 7 (#17): 4 候補統合判定 → 02 §E/§F に集約、案 Q drop

## 3. 次 session が即やること

### 3.1 commit 判断 (AYA 確認待ち)

uncommitted changes:
```
indra/llui/llview.cpp                                   # Group O 動的 zone 配線
indra/newview/llreflectionmap.cpp                       # Group N rmdo_* 配線 (本 session 前から)
indra/newview/pipeline.cpp                              # Group L+M (18 周目分、既配線)
docs/specs/ayastorm-cpu-perf/00-overview.md             # 既編集 (内容詳細は git diff)
docs/specs/ayastorm-cpu-perf/02-offload-feasibility.md  # §E POC + §F Day 7 判定 追記
docs/specs/ayastorm-cpu-perf/03-perf-log-infra.md       # §6.12 Group N + §6.13 Group O 追記
docs/specs/ayastorm-cpu-perf/04-observed-hot-path-map.md # §4.2.i + §4.2.j + §10 one-liner 更新
docs/specs/ayastorm-cpu-perf/handoff-layer6-bfs-to-18pass.md  # (前 session 作成、本 handoff で上書き相当の役目)
docs/specs/ayastorm-cpu-perf/handoff-week1-deepdive-to-phase12.md  # 本 handoff (新規)
tests/aya-gui/                                          # auto-launch harness 一式 (新規 dir)
```

`feedback_no_auto_commit.md` により AYA 明示指示まで commit しない。AYA に「commit する? 章分けは?」を確認。
推奨章分け案 (次 session で AYA に提示):
1. **infra**: `tests/aya-gui/` (auto-launch harness 一式) — 単独 commit、章スコープ独立 (perf 章 deep-dive 用 infra でなく将来 GUI test infra として再利用可能)
2. **計測地図 19 周目**: Group N (llreflectionmap.cpp) + Group O (llui/llview.cpp) + 03/04 spec 追記 — 単独 commit
3. **POC + 判定**: 02 spec §E/§F 追記 + 00 spec scope 明記 (#8) — 単独 commit

### 3.2 Phase 1.2 着手 = `05-core-assignment-plan.md` 新規

task #7 (Phase 2 順序を Y-refined で再構成) と #10 (Worker A pool infra 設計詳細) の前段。05 spec で以下を確定:

| 章 | 内容 |
|---|---|
| §1 Worker pool 設計 | A: doOcclusion 全体 / B: shadow pre-cull / C: (P-refined 用、OCCLUDED 分離後着手) |
| §2 同期境界 (案 O) | camera frustum snapshot (read-only) + probe visibility queue (write-back) + GL Begin/End は main 残し |
| §2.1 同期境界 (案 R-refined) | shadow_cam 4 cascade snapshot + 4 cascade `LLCullResult` buffer + sShadowRender → ShadowRenderContext struct 経由 |
| §3 案 P-refined OCCLUDED 分離 prerequisite | mState から mOcclusionState[per-cam] への移送、視覚回帰確認手順 |
| §4 着手順序 | 1. 案 O → 2. 案 R-refined → 3. P-refined (OCCLUDED 単独 spec 化判断含む) |
| §5 各候補の go/no-go gate | 実装後 frame profile で実測 vs 02 §F.1 ceiling 比較、ceiling の 50% 未満なら revert/再設計 |

### 3.3 camera drift 軽微 fix (run_perf.py) — **適用済**

AYA 第 1 報「カメラの向きがおかしくなってたので直しておいた」+ 第 2 報「またカメラのいちが変なのでなおしておきました」。

**root cause** (AYA 解説): `h.camera_orbit(±15)` 内部の `backend.drag(ctrl+alt)` SL カメラ orbit gesture は **drag 操作が完全対称復元できず**、orbit pair (+15→-15) 後も焦点ずれが残り iteration 毎に累積 drift。

**fix** (run_perf.py:90-101): AYA 指摘の「Esc で元位置に戻せる」を機械化。各 iteration 末で `h.backend.key('Escape', win_id=wid)` を送信、累積を iteration 単位で断ち切る。orbit motion 自体は active-session 代表負荷として維持。commit は AYA 明示指示後。

## 4. 触らない・覚えておくこと

### 4.1 絶対条件 (memory 既読)

- **r40 章で短縮系打ち手を提案しない** (`feedback_r40_no_micro_tuning.md`) — Day 7 統合判定で 4 候補すべて「剥がし」観点で評価、短縮系は完全 0
- **BFS 原則** (`feedback_perf_map_bfs_drill.md`) — Layer 8 Group N + O は同深度で BFS 完成、Layer 9 は両者とも drill 不要判定
- **commit は AYA 明示指示まで禁止** (`feedback_no_auto_commit.md`)
- **検証用ログは commit 前に外す** (`feedback_remove_verification_logs.md`) — 該当無し (今回追加した zone は出荷物として default OFF gate 済)

### 4.2 確定結果 (Day 7 統合)

| 候補 | 判定 | ceiling | Phase 1.2 着手順 |
|---|---|---|---|
| 案 O (doOcclusion async) | 🟢 GO | 5.7-6.5 ms/frame | 1 番目 |
| 案 R-refined (renderShadow pre-cull worker) | 🟢 GO | 2-3 ms/frame | 2 番目 |
| 案 P-refined (mState atomic + 並列 cull) | 🟢 GO (条件付) | 4-5 ms/frame | 3 番目 (OCCLUDED 分離 prerequisite) |
| 案 Q (vwDraw text cache) | 🔴 DROP | 0.08 ms/frame (frame budget 0.5%) | — |

### 4.3 配線済 zone 一覧 (19 周目時点 / Layer 1-8 全周)

既存 (Group A-M、Layer 1-6 全周) + 19 周目追加:
- Group N (Day 1-2 Layer 8): `rmdo_resultAvail` / `rmdo_resultRead` / `rmdo_pushQuery` (llreflectionmap.cpp:383/391/407)
- Group O (Day 2-3 Layer 8): `vwDraw_root_<child>` / `vwDraw_mp_<child>` (llui/llview.cpp::drawChildren 内 parent-name gate、動的)

詳細は 03-perf-log-infra.md §6.12/§6.13 + §9 file list。

### 4.4 既知の制約

- Group O 動的 zone は `LLAyastormPerfLog::isEnabled() + mName == "root"/"main_view"` 2 段 gate。AYAPerfLogEnabled=0 default で zero overhead だが、ON 時は drawChildren 1 呼出当たり string compare 2 件 + std::string concat 1 件 (allocation あり) — 実測 overhead は今回 CSV から判断不能 (比較対象無し)。Phase 2 着手前に再計測する場合は zone 文字列を pre-cache する微小最適化検討余地あり。
- 19 周目 CSV `tests/aya-gui/artifacts/20260526-114303-467640/AYAstorm-perf-day23-group-O.csv` (291 MB) は session 単独取得 (clear 済)、累積でなく純粋な 90s 計測。
- run_perf.py camera_orbit drift 既知 (§3.3 参照)。

## 5. ファイル一覧 (本周回で変更 / 新規、uncommitted)

```
indra/llui/llview.cpp                                   # Group O 動的 zone (新規変更)
docs/specs/ayastorm-cpu-perf/02-offload-feasibility.md  # §E + §F 追記
docs/specs/ayastorm-cpu-perf/03-perf-log-infra.md       # §6.12 + §6.13 追記、§9 file list
docs/specs/ayastorm-cpu-perf/04-observed-hot-path-map.md # §4.2.i + §4.2.j + §10 one-liner
docs/specs/ayastorm-cpu-perf/handoff-week1-deepdive-to-phase12.md  # 本ファイル (新規)
tests/aya-gui/artifacts/20260526-114303-467640/AYAstorm-perf-day23-group-O.csv  # 19 周目 CSV (291 MB、artifact)
```

`git status` / `git diff` で確認可能。

## 6. 関連 spec / commit

| 参照先 | 用途 |
|---|---|
| `docs/specs/ayastorm-cpu-perf/00-overview.md` | r40 章 thesis、Phase 構造、13-17 周目脱線記録 |
| `docs/specs/ayastorm-cpu-perf/02-offload-feasibility.md` | §A1-A14 deep-dive + §E POC spike + §F Day 7 判定 |
| `docs/specs/ayastorm-cpu-perf/03-perf-log-infra.md` | 計測 infra + §6.11 Group M + §6.12 Group N + §6.13 Group O |
| `docs/specs/ayastorm-cpu-perf/04-observed-hot-path-map.md` | Layer 1-8 全結果、§4.2.h/i/j で Layer 6/8 統合 |
| `docs/specs/ayastorm-cpu-perf/handoff-layer6-bfs-to-18pass.md` | 18 周目 handoff (本 handoff の前史) |
| commit `171d6b90d1` | Layer 6 doOcclusion 3-split (Group L) |
| commit `99c771bbb6` | 13-17 周目 handoff doc 追加 |

## 7. Phase 1.2 → Phase 2 への移行条件

Phase 1.1 完成 = 本 handoff §2 経緯すべて完了 ✓
Phase 1.2 完成 = `05-core-assignment-plan.md` の §1-§5 すべて埋まる + AYA approve
Phase 2 (実装) 着手 = 05 spec approve 後、案 O から順次着手 (task #11)

---

**handoff 作成者** Claude opus-4-7 (session `491aa33c-55fe-48d9-9db2-6eb9785dd899`)、autonomous loop tick 経由
