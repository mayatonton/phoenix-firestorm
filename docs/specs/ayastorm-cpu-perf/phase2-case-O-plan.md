# Phase 2 第 1 着手プラン — 案 O (doOcclusion async / Worker A)

**位置**: r40 章 Phase 2 第 1 着手 (05 §7.2 確定順序 1 番目)
**着手前提**: Phase 1.1 完成 (4 候補 POC + Day 7 統合判定) + Phase 1.2 (本 spec の親) AYA review PASS
**作成**: 2026-05-26

---

## §0. one-liner

`LLReflectionMapManager::doOcclusion()` (pipeline.cpp:3198) と `LLHeroProbeManager::doOcclusion()` (pipeline.cpp:3202) の **probe iteration loop 全体** を Worker A に移送する。**GL 呼出 (`glGen/Begin/End/GetQueryObjectuiv`) は main 残し**、worker は per-probe visibility 判定の **CPU 計算 + result buffer 書込** のみ。期待短縮 ceiling = 5.7-6.5 ms/frame (doOcclusion_reflectionProbes 8.1 ms × 70-80%、04 §4.2.i parent − Σ rmdo_* = 77.3% が剥がし対象本体)。

**ただし** rmdo_* 自体 (per-call 0.04-0.06 µs) は GL context tie で main 残し必須。本 plan の真の難所は **「probe iteration / branching / cube vertex setup の CPU 部分だけ worker、GL 呼出は main」の split を実コードで成立させる箇所** = `LLReflectionMap::doOcclusion(eye)` の **構造分解**。

---

## §1. 目的と本 plan のスコープ

### §1.1 目的

main thread から **5.7-6.5 ms/frame** を Worker A (専属 1 thread pool) に剥がす。per-candidate gate 50% 未満 (= 2.85 ms 未満) なら revert。

### §1.2 本 plan が答えること

1. **移送対象の確定** — `LLReflectionMap::doOcclusion(eye)` のどの部分を worker、どの部分を main に残すか (§2)
2. **Snapshot / Apply / 残置の確定** — 05 §5.1 をコード行レベルに具体化 (§3)
3. **Worker A pool infra の実装インタフェース** — 05 §4.2 を実コード class に展開 (§4)
4. **3-commit 工程** (baseline / 実装 / 再計測) と各 commit の touch 範囲 (§5)
5. **per-candidate gate 50% threshold の計測条件** (§6)
6. **着手前に潰す open questions** (§7)

### §1.3 本 plan が答えないこと

- 案 R-refined / 案 P-refined の plan (各別 plan で扱う、本 plan は案 O 専属)
- Worker B / Worker C の pool infra 設計 (各 plan で個別)
- HeroProbeManager の root cause (Hero probe 2 重発火) — 別 commit `82c32ea932` revert で対応済、本 plan は doOcclusion 並列化のみ

---

## §2. 移送対象 = `LLReflectionMap::doOcclusion(eye)` の構造分解

### §2.1 現状コードの境界

`LLReflectionMap::doOcclusion(eye)` (llreflectionmap.cpp:346-421) を **CPU 部分 (worker 移送可)** と **GL 部分 (main 残し)** に行単位で分解:

| 行 | 内容 | CPU/GL | 移送 |
|---|---|---|---|
| 358 | `dist = mRadius * F_SQRT3 + 1.f` | CPU | ✓ worker |
| 360-361 | `o.setSub(mOrigin, eye)` | CPU | ✓ worker |
| 363 | `do_query = false` | CPU | ✓ worker |
| 365-369 | eye inside radius check → `mOccluded = false; return` | CPU + state write | ✓ worker (write は result buffer 経由) |
| 371 | `mOcclusionQuery == 0` check | CPU | ✓ worker |
| 374 | `glGenQueries(1, &mOcclusionQuery)` | **GL** | ✗ main |
| 375 | `do_query = true` | CPU | ✓ worker |
| 381-385 | `glGetQueryObjectuiv(.., GL_QUERY_RESULT_AVAILABLE, ..)` | **GL** | ✗ main |
| 387-393 | `if (result > 0) { glGetQueryObjectuiv(.., GL_QUERY_RESULT, ..); }` | **GL** + state write | ✗ main (state write は worker apply 後) |
| 394-395 | `mOccluded = result == 0; mOcclusionPendingFrames = 0` | state write | ✗ main (apply phase) |
| 399 | `mOcclusionPendingFrames++` | state write | ✗ main (apply phase) |
| 403-419 | `if (do_query)` 内の `glBeginQuery + uniform + drawRange + glEndQuery` | **GL** | ✗ main |

### §2.2 結論: 2 pass モデル

実コード上は **per-probe で CPU 計算と GL 呼出が織り交ぜ** になっており、現状の関数を「前半 worker / 後半 main」と単純に切れない。Phase 2 実装では `LLReflectionMap::doOcclusion(eye)` を **2 pass 分解**:

- **Pass 1 (worker)**: per-probe で `dist / o / eye-inside-radius check / do_query 予測` を計算し、**per-probe action descriptor** を result buffer に書く
  - action: `SKIP_INSIDE_RADIUS` (eye inside → mOccluded=false 確定)
  - action: `NEED_GEN_QUERY` (mOcclusionQuery==0、main で glGenQueries → push)
  - action: `READ_RESULT_THEN_PUSH` (既存 query、main で result read → 結果次第で do_query)
  - action: `READ_RESULT_NO_PUSH` (既存 query、main で result read のみ)
- **Pass 2 (main)**: action descriptor を順に消化、GL 呼出 + `mOccluded` / `mOcclusionPendingFrames` write-back

per-probe action descriptor が worker → main の唯一の引き継ぎ単位。

### §2.3 移送 ceiling の再評価

04 §4.2.i parent per-call 258.4 µs / iteration × 8192 calls = 2.12 s 累積。このうち:
- rmdo_* (GL 部分) Σ = 480 ms (22.7%) → **main 残し必須**
- parent − Σ rmdo_* = 1.64 s (77.3%) = **Pass 1 worker 移送対象**

display frame 換算で `8.1 ms × 77.3% = 6.26 ms/frame` が理論上 worker に移送可能。Pass 2 main 残置 = 8.1 × 22.7% = 1.84 ms/frame は据え置き。

ただし Pass 1 と Pass 2 を直列実行 (worker → main wait → main GL 発行) では frame 内で sync 待ちが入り、実効短縮は **ceiling 5.7-6.5 ms に届かない可能性**。Phase 2 実装の選択肢:

- **同期式 (single frame)**: frame 内 worker → wait → main、待ち時間で実効短縮減
- **非同期式 (1 frame lag)**: worker は frame N の snapshot を計算、main は frame N+1 で apply。occlusion 自体が元々 1 frame lag (02 §A3) のため visual 影響は構造上吸収可能

**default 採用 = 非同期式 (1 frame lag)**。02 §A3 で既に確認された通り、occlusion 結果は元々 Frame N 発行 → Frame N+1 poll の async design。本 plan の worker offload はこの async window を CPU 側でも実現する。

---

## §3. Snapshot / Apply / 残置の確定 (05 §5.1 をコード行レベルへ)

### §3.1 Snapshot (main → worker、frame N 頭)

| 項目 | 内容 | 取得 site |
|---|---|---|
| `mProbes` shallow copy | `std::vector<LLPointer<LLReflectionMap>>` 全要素 (~32-128 個想定) を `LLPointer` の ref count 経由で shallow copy | pipeline.cpp:3198 の直前で `mReflectionMapManager.snapshotForOcclusion()` (新規 method) |
| Hero `mProbes` shallow copy | 同上 (Hero は通常 size==1) | pipeline.cpp:3202 直前 |
| `camera.getOrigin()` の `LLVector4a eye` | 現 llreflectionmapmanager.cpp:1618-1619 と同じ | snapshot 内に値コピー |
| frame seq | 何 frame 目の snapshot か (debug 用) | snapshot 内に gFrameCount |

**Snapshot lifecycle**:
- frame 頭 (pipeline.cpp:3198 直前) で main が dispatch
- worker は snapshot を read-only で消費、独自の result buffer へ action descriptor を書く
- frame 末 (apply phase) で main が swap 取得、worker は次 frame snapshot 待ち

**Race 検証 (本 plan §7.1)**: mProbes mutation は addProbe / deleteProbe / mProbes.clear で `mCreateList` / `mKillList` 経由の遅延処理 (llreflectionmapmanager.cpp:306/315/543/559/684/727/1599)。これらは `doProbeUpdate()` 内で apply され、`doOcclusion()` とは別 frame phase で実行されるため、frame 内で `mProbes` 自体は structural stable。Phase 2 着手 baseline commit で `doProbeUpdate` と `doOcclusion` の frame 内呼出順序を再 verify。

### §3.2 Apply (worker → main、frame N+1 頭、または N 末)

per-probe action descriptor を main が順に消化:

```cpp
// Pseudo-code (Phase 2 で確定)
for (auto& act : worker_result.actions) {
    LLReflectionMap* probe = act.probe; // LLPointer 保持
    switch (act.kind) {
    case SKIP_INSIDE_RADIUS:
        probe->mOccluded = false;
        break;
    case NEED_GEN_QUERY:
        glGenQueries(1, &probe->mOcclusionQuery);
        push_query(probe); // glBeginQuery + uniform + drawRange + glEndQuery
        break;
    case READ_RESULT_THEN_PUSH: {
        GLuint avail = 0;
        glGetQueryObjectuiv(probe->mOcclusionQuery, GL_QUERY_RESULT_AVAILABLE, &avail);
        if (avail) {
            GLuint result = 0;
            glGetQueryObjectuiv(probe->mOcclusionQuery, GL_QUERY_RESULT, &result);
            probe->mOccluded = (result == 0);
            probe->mOcclusionPendingFrames = 0;
            push_query(probe);
        } else {
            probe->mOcclusionPendingFrames++;
        }
        break;
    }
    case READ_RESULT_NO_PUSH:
        // 上の avail==0 path だけ流す版
        break;
    }
}
```

GL state は pipeline.cpp:3184-3194 で既に整っているため (ColorMask off / Depth test / `gOcclusionCubeProgram.bind()` / `mCubeVB->bind()`)、main 側 apply は **同じ context で続けて GL 発行可能**。

### §3.3 main 残置 (絶対)

| 残置項目 | 理由 |
|---|---|
| (a) `glGenQueries` / `glBeginQuery` / `glEndQuery` / `glGetQueryObjectuiv` | GL context tie、driver thread affinity 制約 (06 §2.9 R3 (b)) |
| (b) `mCubeVB->drawRange()` の発行 | GL draw call |
| (c) `gOcclusionCubeProgram` bind + uniform 設定 | GL shader state |
| (d) `mOccluded` / `mOcclusionPendingFrames` の write | worker → main の唯一の write-back path、apply phase で main が単独書込 |
| (e) probe rebuild dispatch (LLDrawable mark dirty) | frame 内 race 回避、`doProbeUpdate()` 別経路で main 限定 |

### §3.4 Lock 戦略

- **mProbes 内容**: snapshot 取得時に LLPointer shallow copy → worker は ref count 経由で probe 本体を read のみ。`mProbes` vector 自体への mutation は別 frame phase (mCreateList/mKillList 経由) のため frame 内 race なし
- **`mOccluded` / `mOcclusionPendingFrames`**: main thread からのみ write (apply phase)、worker は read しない。同 frame 内で reader (updateUniforms @427/598、HeroProbeManager pre-check @279) が apply 後に走ることを frame phase ordering で保証
- **result buffer**: per-frame fresh `std::vector<ProbeOcclusionAction>`、worker は append only、apply 時に move

---

## §4. Worker A pool infra の実装インタフェース (05 §4.2 をコード化)

### §4.1 class 設計

```cpp
// indra/newview/llreflectionocclusionworker.h (新規)
class LLReflectionOcclusionWorker {
public:
    LLReflectionOcclusionWorker();
    ~LLReflectionOcclusionWorker();

    void enqueue(const ProbeOcclusionSnapshot& snap);
    void wait_apply(ProbeOcclusionResult& out);
    void shutdown();

private:
    void thread_main();

    std::thread mThread;
    std::mutex mMutex;
    std::condition_variable mCv;
    bool mTaskReady = false;
    bool mResultReady = false;
    bool mStop = false;
    ProbeOcclusionSnapshot mSnap;
    ProbeOcclusionResult mResult;
};

struct ProbeOcclusionSnapshot {
    std::vector<LLPointer<LLReflectionMap>> mReflectionProbes; // shallow copy
    std::vector<LLPointer<LLReflectionMap>> mHeroProbes;
    LLVector4a mEye;
    U32 mFrameSeq;
};

enum class ProbeOcclusionActionKind {
    SKIP_INSIDE_RADIUS,
    NEED_GEN_QUERY,
    READ_RESULT_THEN_PUSH,
    READ_RESULT_NO_PUSH,
};

struct ProbeOcclusionAction {
    LLPointer<LLReflectionMap> mProbe;
    ProbeOcclusionActionKind mKind;
};

struct ProbeOcclusionResult {
    std::vector<ProbeOcclusionAction> mActions;
    U32 mFrameSeq;
};
```

### §4.2 lifecycle

- viewer 起動: `LLAppViewer::initWindow` 後 + Pipeline 初期化前後で `gReflectionOcclusionWorker = new LLReflectionOcclusionWorker()`
- frame 頭: `Pipeline::doOcclusion()` 入口で snapshot 作成 + enqueue
- frame 末: `Pipeline::doOcclusion()` 内、reflection probe iteration の代替で `wait_apply` 呼出 → apply
- viewer 終了: `LLAppViewer::cleanup` 内で `gReflectionOcclusionWorker->shutdown()` + delete

### §4.3 failure fallback

worker thread が exception 投げたら `wait_apply` は no-op + 当該 frame の Hero probe は前 frame state 継続 + `LL_WARNS("ReflectionProbes")` log。memory `feedback_root_cause_not_dump.md` に従い silent fallback はしない。

### §4.4 計測 zone

- `worker_enqueue`: main thread で snapshot 作成 + dispatch 時間
- `worker_wait`: main thread で wait_apply の wait 時間 (worker 計算がまだ終わってない時)
- `worker_apply`: main thread で action descriptor 消化 + GL 発行時間
- `worker_compute`: worker thread 側、Pass 1 計算時間
- 既存 `doOcclusion_reflectionProbes` 親 zone は維持、上記 4 zone を child として配線

---

## §5. 3-commit 工程 (05 §8.1)

### Commit 1: baseline (worker 移送なし、zone 追加 + 計測)

**touch**:
- `indra/newview/llreflectionmap.cpp` Pass 1 / Pass 2 境界の zone 追加 (Pass 1 候補部分に `AYAPERF_ZONE("rmdo_pass1_cpu")`、Pass 2 候補に `AYAPERF_ZONE("rmdo_pass2_gl")`)

**目的**: 19 周目 CSV (`tests/aya-gui/artifacts/.../AYAstorm-perf-18pass-baseline.csv`) を 04 §4.2.i baseline として再確認 + Pass 1/Pass 2 別計測で worker 移送可能上限を再確定。

**期待結果**: Pass 1 (CPU 部分) ≈ 6.0-6.5 ms/frame、Pass 2 (GL 部分) ≈ 1.6-2.1 ms/frame。両者比が 04 §4.2.i parent − Σ rmdo_* = 77.3% / Σ rmdo_* = 22.7% と一致するか確認。

### Commit 2: 実装 (worker 移送 + apply 配線)

**touch (新規)**:
- `indra/newview/llreflectionocclusionworker.h` / `.cpp` 新規
- `indra/newview/CMakeLists.txt` に新規 .cpp 追加

**touch (修正)**:
- `indra/newview/llreflectionmap.cpp` — `doOcclusion(eye)` を `doOcclusion_pass1_cpu(eye)` + `doOcclusion_pass2_gl(actions)` に分解、旧 single-pass entry は wrapper として残し initial cutover に備える
- `indra/newview/llreflectionmapmanager.cpp` — `doOcclusion()` を worker dispatch pattern に書き換え、`snapshotForOcclusion()` / `applyOcclusionResult()` 新規 method
- `indra/newview/llheroprobemanager.cpp` — 同様の dispatch pattern (同 Worker A pool 共有)
- `indra/newview/pipeline.cpp:3198 / 3202 / 3225` — Hero probe doOcclusion 呼出を `wait_apply` パスに置換
- `indra/newview/llappviewer.cpp` — pool lifecycle 配線 (init / cleanup)

**lifecycle gate**: cvar `AYARenderOcclusionWorkerEnabled` (default true、no-restart 切替可) で worker パスと旧 single-pass パスを A/B 切替可能にする。問題発生時の即時 disable + 視覚回帰の live 比較用。

### Commit 3: 再計測 (同 scene / 同条件 CSV)

**touch**:
- (commit message にのみ実機計測結果を記録、コード変更なし)

**計測条件** (05 §8.4 統一):
- scene: Aditi Bonifacio `secondlife://util.aditi.lindenlab.com/secondlife/Bonifacio/179/69/26`
- duration: 90s
- cvar: AYAPerfLogEnabled=1、AYARenderOcclusionWorkerEnabled=1
- harness: `tests/aya-gui/run_perf.py` LEAP 自動 login
- CSV 命名: `day1-case-O-postimpl.csv` (handoff §2 命名規約準拠)

**期待結果**:
- `doOcclusion_reflectionProbes` 親 zone main thread 占有: 8.1 ms → **1.6-2.4 ms** (Pass 2 のみ残置)
- 短縮: **5.7-6.5 ms/frame** (ceiling、worker compute は別 thread)
- 50% gate (2.85-3.25 ms 短縮) を超えれば PASS

---

## §6. per-candidate gate (05 §8)

| 判定 | 条件 | アクション |
|---|---|---|
| 🟢 PASS | Commit 3 計測 − Commit 1 baseline ≥ **2.85 ms/frame** | merge、案 R-refined 着手へ |
| 🟡 BORDERLINE | 1.5-2.85 ms 短縮、構造修正で 50% 到達可能 | PR 内 revise (snapshot コスト見直し / wait_apply 並列化等)、再計測 1 回まで |
| 🔴 FAIL | 1.5 ms 未満、構造修正できず | revert + 本 plan §7 unknown の再評価 → 案 O は r41+ 章送り |

---

## §7. Phase 2 着手前に潰す open questions

### §7.1 mProbes structural stability (05 §5.1 unknown (a))

**Q**: doOcclusion 実行 frame 内で `mProbes` が mutate するか?

**事前調査結果 (本 plan 着手前 grep finding)**:
- mutation 経由は `mCreateList` / `mKillList` 遅延処理のみ (llreflectionmapmanager.cpp:306/315/543/559/684/727/1599)
- これらは `doProbeUpdate()` 内で apply され `doOcclusion()` とは別 frame phase
- → **frame 内 structural stable** が gate 前提

**Phase 2 baseline commit で再 verify**: pipeline.cpp 内 `doProbeUpdate` と `doOcclusion` の呼出順を再 grep、frame 内で mProbes mutation が doOcclusion 前後で完結することを確認。

### §7.2 snapshot allocation cost (05 §5.1 unknown (b))

**Q**: `std::vector<LLPointer<LLReflectionMap>>` の shallow copy + atomic ref count inc が dispatch overhead を膨張させないか?

**baseline 取得方法**: Commit 1 で snapshot 部分のみ zone 配線 (`worker_enqueue` zone)、per-frame cost を測る。0.05 ms/frame 超なら snapshot 方式を見直し (raw pointer + lifetime gate に切替候補)。

### §7.3 HeroProbeManager との pool 共有

**Q**: HeroProbeManager の doOcclusion (pipeline.cpp:3202、3225) を同 Worker A pool で扱うか別 pool か?

**事前調査結果**: HeroProbeManager::doOcclusion() (llheroprobemanager.cpp:650-662) は LLReflectionMapManager::doOcclusion() と構造同一、mProbes は通常 1 要素のみ。**同 Worker A pool で snapshot 1 つに統合**、dispatch overhead 増を避ける。

### §7.4 1 frame lag の visual 影響

**Q**: 案 O 非同期式 (frame N snapshot → frame N+1 apply) で probe occlusion 結果が 1 frame 遅延することの visual 影響?

**事前推定**: 02 §A3 で確認済の通り、occlusion 自体が既に Frame N issue → Frame N+1 poll の async design。本 plan の lag はそれと **重ねて** さらに 1 frame ではなく、**同等の lag window 内** で worker apply に切替えるため visual 差分は **構造上発生しない見込み**。Phase 2 Commit 2 で side-by-side cvar A/B (`AYARenderOcclusionWorkerEnabled` toggle) で確認。

### §7.5 worker thread の lifecycle と再入

**Q**: viewer の teleport / log out → log in / window minimize で worker thread の lifecycle が破綻しないか?

**設計**: viewer 起動時 1 回 spawn、shutdown 時 1 回 join。teleport / login flow では worker は active のまま、snapshot enqueue のみが停止 (Pipeline::doOcclusion 呼出が止まる)。worker thread は condvar wait で idle、リソース消費は wait のみ。

---

## §8. 検証手順

### §8.1 機能 verify (Commit 2 完了時)

- AYAstorm 起動 + Aditi Bonifacio teleport
- 視覚チェック: reflection probe が正常に occlude / unocclude する (open sky / 屋内 / interior crossing で probe が disappear / reappear)
- `AYARenderOcclusionWorkerEnabled=0` (旧 single-pass) と `=1` (worker) で screenshot 比較、差分が **probe debug visualization 以外で発生しないこと** を verify
- log: `LL_WARNS("ReflectionProbes")` で worker exception が発火していないこと

### §8.2 perf verify (Commit 3 計測)

- run_perf.py で 90s capture × 3 回 (variance 測定)
- `doOcclusion_reflectionProbes` parent zone の per-frame 平均 ms を Commit 1 baseline と比較
- 50% gate 判定 (§6)

### §8.3 verify ログ除去 (memory `feedback_remove_verification_logs.md`)

- Commit 2 / 3 で追加した `LL_INFOS("ReflectionProbes")` debug log は Commit 3 PASS 後に除去してから merge commit

---

## §9. commit 単位と branch 運用

- **branch**: 本 plan の実装は `feature/ayastorm-r40-case-O` (新規切出し、`feature/ayastorm-r40-cpu-perf` ベース)
- memory `feedback_experiment_branch_single_scope.md` 通り 1 branch 1 scope、案 R-refined / 案 P-refined は別 branch
- 3 commit + 必要に応じて revise commit を追加、AYA push 待ちで release branch (ayastorm-release) には触れない (memory `feedback_release_branch_workflow.md`)

---

## §12. gate 判定結果 (2026-05-26、Commit 3 完了)

### §12.1 verdict: 🔴 **REJECT**

50% gate (2.85 ms 短縮要求) に対し、実測短縮分 **~0.006 ms** (= 6us)、3 桁不足。

### §12.2 計測条件

- AYAstorm `feature/ayastorm-r40-cpu-perf` 上、Commit 3 build (raw ptr fix + worker default ON)
- run_perf_case_O.py 25s active window × baseline (worker=0) + worker (worker=1) 各 1 回
- session: Aditi Bonifacio 179/69/26 idle (settling 10s 後)
- mProbes ≈ 460-470 (steady state)
- CSV: `tests/aya-gui/artifacts/20260526-215512-596286/rawptrfix-w1.csv` (worker 161 MB) / `tests/aya-gui/artifacts/20260526-215716-598159/rawptrfix-baseline.csv` (baseline)

### §12.3 zone 比較表 (per-frame avg)

| zone | baseline (worker=0) | worker (worker=1) | diff |
|---|---:|---:|---:|
| `doOcclusion_reflectionProbes` (parent) | 569 us | 625 us | **+56 us (regression)** |
| `worker_apply` (前 frame GL pass2) | n/a* | 510 us | — |
| `worker_compute` (worker thread CPU) | — | 0.80 us | — |
| `worker_enqueue` (main snapshot) | n/a* | 5.17 us | — |
| `worker_apply_hero` | n/a* | 1.13 us | — |
| `worker_enqueue_hero` | n/a* | 0.07 us | — |

\* baseline は cvar transition lag で序盤 877 frame だけ worker=1 状態 (LLCachedControl signal 反映前)、後期は 0。比較は steady state worker run と公平。

### §12.4 根本原因: spec 期待値の baseline 想定が 14x 過大

| spec §X | 実測 | 比 |
|---|---:|---:|
| spec §3 期待 doOcclusion_reflectionProbes baseline | 8.1 ms | 30x |
| 04 §4.2.i baseline parent avg | 258 us | 1x |
| 本計測 baseline (25s idle session) | 569 us | 2.2x (短 session 全期 startup overhead 比率高) |

spec 期待短縮 ceiling 5.7-6.5 ms (= 70-80% offload) が、実 baseline では既に 0.57 ms 全体。
- 内訳: worker_apply (= GL pass2 = main thread 専有) = 510 us (99%)
- CPU 剥がせる部分 (worker_compute + worker_enqueue) = 6 us (1%)

→ **doOcclusion は CPU bound でなく GL bound**。Phase 1.2 で 04 §4.2.i「parent − Σ rmdo_* = 77.3%」を「main 内 CPU の隠れ overhead」と解釈したが、実は GL 呼出のドライバー側コスト (`glGetQueryObjectuiv` など) が main thread 上に乗っていただけで、worker に剥がせる種類のものではなかった。

### §12.5 アクション

- ✅ worker path はクラッシュ修正済 (raw ptr non-owning、§13 参照) のため code path 自体は保持
- ✅ `AYARenderOcclusionWorkerEnabled` default **1 → 0** に変更 (settings.xml)
- ✅ experimental cvar として残置、live A/B 検証用、release default は旧 single-pass
- 案 O は **r40 章対象外** に確定、次 candidate に移送

### §12.6 r40 章への学び

- 04 spec の zone 計測表「parent − Σ children = X%」を **CPU 剥がし可能量 X%** と解釈してはいけない
- parent zone は main thread 滞在 wall time であり、その中の **GL 同期コスト** は剥がせない
- 案抽出時は「children に分解した時点で個別 child zone の us が GL 呼出 (glGet*, glBegin*) を含むか」を必ず分離記述する必要あり
- 04 spec の §4.2 表記再整理 task → §4.2.j 以降の周回で対応

### §12.7 結論 (Phase 2 案 O closed)

Phase 2 案 O は per-candidate gate 🔴 REJECT で **closed**。次 candidate は別 plan で起票。

---

## §13. 案外: worker path SIGSEGV 真因と修正 (2026-05-26)

### §13.1 観測

worker=1 で AYAstorm を 155 frame 程度走らせると main thread が `LLReflectionMap::autoAdjustOrigin` (llreflectionmap.cpp:89) で SIGSEGV。doOcclusion / pass2_gl / worker_compute ではない、想定外の path。

gdb backtrace:
```
#0 LLReflectionMap::autoAdjustOrigin at llreflectionmap.cpp:89
#1 LLReflectionMapManager::update at llreflectionmapmanager.cpp:520
#2 LLAppViewer::doFrame at llappviewer.cpp:1789
```

line 89 は `if (part && part->mPartitionType == ...)` — `part` が dangling ptr で deref segfault。

### §13.2 root cause

worker snapshot/result が `LLPointer<LLReflectionMap>` で probe を保持していたため、probe lifetime が 1 frame 延長。その 1 frame 間に probe 内 raw `mGroup` (LLSpatialGroup*) が別経路で freed → `autoAdjustOrigin` の `mGroup->getSpatialPartition()` が dangling ptr 返却 → deref segfault。

vanilla viewer では probe destroy と mGroup destroy が同 frame 内に揃うため latent、worker async lag で露見。

### §13.3 fix

`ProbeOcclusionSnapshotEntry::mProbe` / `ProbeOcclusionAction::mProbe` を `LLPointer<LLReflectionMap>` → `LLReflectionMap*` (raw、non-owning) に降格。apply phase で current `mProbes` 在籍 validation 経由でのみ deref:

```cpp
std::unordered_set<LLReflectionMap*> alive;
for (auto& p : mProbes) { alive.insert(p.get()); }
for (auto& a : result.mActions) {
    if (a.mProbe && alive.count(a.mProbe)) {
        a.mProbe->doOcclusion_pass2_gl(a.mKind);
    }
}
```

raw ptr 不要 deref を完全排除。probe lifetime は main thread `mProbes` の LLPointer のみが管理。

### §13.4 副次効果

vanilla viewer にも同種の race 罠 (mGroup raw deref) が latent で残っているが、本修正は worker path 専用 trigger 解除のみ、vanilla path の race fix は別 ticket (本 plan scope 外)。

---

## §14. 関連 spec / commit

| 参照先 | 用途 |
|---|---|
| `00-overview.md` §Phase 1.2 章 scope | r40 章 thesis、案 O 位置付け |
| `02-offload-feasibility.md` §A3 / §F.1 / §F.4 | 既 async design 確認 + ceiling 算定 |
| `04-observed-hot-path-map.md` §4.2.h / §4.2.i | Layer 6 doOcclusion 8.1 ms + Layer 8 Group N rmdo_* 22.7% / 77.3% 分解 |
| `05-core-assignment-plan.md` §3.1 / §4.2 / §5.1 / §7.2 / §8 | 案 O 採用判定 + Worker A pool 詳細 + 剥がし境界 + 着手順序 + per-candidate gate |
| `06-offload-feasibility-deep.md` §2.9 | L6-occ GL 制約 (CPU-side rollup のみ worker 可) |
| commit `171d6b90d1` | Layer 6 doOcclusion 3-split (Group L 配線) — Pass 1 / Pass 2 zone のベース |
| commit `aed520c761` | r31 → r40 採番きりなおし |
| commit `a4ea3d770b` | OCCLUDED grep finding 反映 (案 P-refined 着手前 clearing 工程) |

---

## §11. 着手 OK 条件 (本 plan AYA review gate)

1. 本 plan の AYA review PASS
2. §7.1 (mProbes structural stability) の verify が baseline commit で取れる準備
3. §7.4 (1 frame lag visual 影響) の A/B 切替 cvar 設計が AYA 同意
4. branch `feature/ayastorm-r40-case-O` 切出 (本 plan AYA review PASS 後)

5 を満たした時点で Commit 1 (baseline) 着手。
