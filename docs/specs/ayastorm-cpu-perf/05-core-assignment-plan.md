# 05 — Core Assignment Plan (Worker 振り分け設計 / Y-refined)

**位置**: r40 章 Phase 1.2 (Phase 1.1 = 地図完成 + 4 候補 POC → 本 spec = 設計 → Phase 2 = 剥がし実装)

**ゲート前条件**: Phase 1.1 完成判定 ✓ (1 週間 deep-dive Day 1-7 完走、02 §F.4 で go/no-go 確定)

**作成**: 2026-05-26 (Y-refined 改訂版、初版 2026-05-26 09:41 の 9 候補版を Day 7 統合判定後に 3 候補へ集約)

---

## §0. one-liner (結論ファースト)

本 spec で **剥がし対象を Y-refined の 3 候補に確定** — 案 O (doOcclusion async) / 案 R-refined (renderShadow pre-cull worker) / 案 P-refined (mState atomic + 並列 cull)。**Worker pool は A / B / C の 3 構成** で、A = doOcclusion 専属、B = shadow pre-cull 専属、C = P-refined 用 (OCCLUDED 分離 prerequisite 後に立ち上げ)。**Phase 2 着手順序は O → R-refined → P-refined**。各候補の go/no-go は **02 §F.1 ceiling の 50% 未満なら revert** の per-candidate gate で運用 (§8)。02 §A1-A14 のうち本章 Y-refined に含まれない A1/A5/A8/A11/A12/A14 は r41+ 章で再評価。

---

## §1. 目的とスコープ

### §1.1 単一目的

**main thread を専有している処理を別 thread に「移送」する設計案を作る。**

- 採用判定軸は **剥がし可能性のみ**。
- **短縮 / 削減 / lazy trigger / polling sleep / cvar 値弄り / batch 化で main 内で速くする** 打ち手は本 spec の射程外 (memory `feedback_r40_no_micro_tuning`、13-17 周目 5 周空転実例あり)。

### §1.2 本 spec が答えること

1. r40 章で剥がす対象は何か — Day 7 統合判定の Y-refined 3 候補 (§3)
2. 各候補を **どの Worker pool に置くか** (§4)
3. **main 残置部分の境界線** (snapshot 入力 / apply 出力 / lock 戦略) — 候補ごと (§5)
4. **案 P-refined の prerequisite** (OCCLUDED 分離 refactor) — 単独工程として明示 (§6)
5. **Phase 2 着手順序** (§7)
6. **per-candidate go/no-go gate** — 実装後の ceiling 達成率による revert 条件 (§8)
7. **未解決 unknown** と Phase 2 着手前に潰すべき open question (§9)
8. **Phase 2 着手 gate** (§10)

### §1.3 本 spec が答えないこと

- 各 worker の **実装コード**: Phase 2 で書く。本 spec は契約 (input/output/lock) まで。
- **削減系打ち手の評価**: 別章 (r41+) があれば扱うが、r40 では一切評価しない。
- **GPU 側施策**: GPU pass 最適化は GPU 章 (r42+ 候補) で扱う。
- **02 §A1-A14 のうち Y-refined 非採用候補**: A1/A5-mat/A5-inv/A8/A11/A12/A14 は本 spec で扱わない。02 §A の deep-dive 自体は保持され、r41+ 章で再評価対象として記録 (00 overview §Phase 1.2 章 scope 参照)。

---

## §2. r40 thesis 再確認 (本 spec の制約)

memory `project_ayastorm_r40_cpu_parallel.md` + `feedback_r40_no_micro_tuning.md` から本 spec が守る条件:

| 条件 | 内容 | 違反例 |
|---|---|---|
| C1 | 採用 = 剥がしのみ | 「lazy trigger で 0.5 ms 削れる」を採用案にしない |
| C2 | 段階 1 件ずつ | O+R+P まとめ実装計画にしない、独立 PR 単位で並べる |
| C3 | apply 同期点を明示 | 「worker でやって main は気にしない」と書かない、必ず apply 関数 + 同期方式を書く |
| C4 | main 残置を黙らない | snapshot 取得 / apply / lock の 3 つを必ず candidate ごとに書く |
| C5 | 効果未確認の commit を積まない | Phase 2 各実装は per-candidate baseline 計測 → 実装 → 再計測の 3 step ゲートを義務付け (memory `feedback_build_only_verified`) — 詳細 §8 |

---

## §3. 剥がし対象 = Y-refined 3 候補 (Day 7 統合判定確定)

### §3.1 採用 3 候補

| 候補 | 担当 worker | 期待短縮 ceiling | 構造判定 (02 §E) | 着手 phase |
|---|---|---|---|---|
| **案 O — doOcclusion async** | Worker A 専属 | **5.7-6.5 ms/frame** (doOcclusion_reflectionProbes 8.1 ms × 70-80%) | 🟢 GREEN — rmdo_* state machine 22.7%、parent − Σ = 77.3% iteration loop が剥がし対象本体 | Phase 2 第 1 (§7) |
| **案 R-refined — renderShadow pre-cull worker** | Worker B 専属 | **2-3 ms/frame** (renderShadow body 内 cull + stateSort の ~25%) | 🟢 GREEN — sCull/mNumVisibleFaces scope 小、GL blocker は body 側のみ。pre-cull (updateCull + stateSort) を frame N-1 で worker 先行構築可能 | Phase 2 第 2 |
| **案 P-refined — mState atomic + 並列 cull** | Worker C (OCCLUDED 分離後立ち上げ) | **4-5 ms/frame** (main_cam + shadow_cam 並列) | 🟡 YELLOW (条件付 GO) — atomic mState + per-task buffer は GREEN、**OCCLUDED bit を mState から mOcclusionState[per-camera] へ分離する prerequisite refactor 必要** (§6) | Phase 2 第 3 (prerequisite spec 経由) |

合計理論上限 = **11.7-14.5 ms/frame**。

### §3.2 DROP 候補 (Day 7 確定)

| 候補 | 判定根拠 |
|---|---|
| 案 Q (vwDraw text cache) | 19 周目 Group O 実測 = vwDraw_rootView 0.145 ms/frame。100% 削減でも frame budget 0.5% (0.08 ms)。詳細 04 §4.2.j |

### §3.3 本 spec の射程外 (r41+ 章で再評価)

02 §A1-A14 deep-dive 結果のうち本 spec で扱わない候補:

| 02 ID | 内容 | 02 での判定 | 本 spec の扱い |
|---|---|---|---|
| A1 | object `idleUpdate` (position/velocity 補間) | 02 §A1 deep-dive 済、剥がし候補として grounded | r41+ 章で再評価 — region crossing race の解析が重く Y-refined scope では外す |
| A5-mat / A5-inv | Material / Inventory asset callback defer | 02 §A5 deep-dive 済 | r41+ 章で再評価 — apply 順序保証の re-design 必要 |
| A8 | local light frustum culling 先行化 | 02 §A8 deep-dive 済 | r41+ 章で再評価 — A1 と worker A 共有時に競合分析必要 |
| A11 | region visibility / terrain patch | 02 §A11 deep-dive 済 | r41+ 章で再評価 |
| A12 | particle sim per-element update | 02 §A12 deep-dive 済 | r41+ 章で再評価 |
| A14 | processTextureStats | 02 §A14 deep-dive 済 | r41+ 章で再評価 (元 05 で reference impl 候補だったが、Y-refined では doOcclusion 直接効果優先) |

**A2 / A3 / A4 / A10 / A13 は短縮系のため r40 全体で除外** (02 §F.2 + 旧 05 §3.2 と同義)。

---

## §4. Worker pool 設計

### §4.1 pool 構成と命名 (Y-refined 改訂)

旧 05 の A/B/C taxonomy (cull/parse/compute) は **9 候補を抱える pre-thesis 設計** だった。Y-refined は 3 候補に絞られたため pool 構成を **candidate 専属モデル** に再定義:

| pool | 担当候補 | 専属理由 |
|---|---|---|
| **Worker A** | 案 O (doOcclusion async) | reflection probe visibility 計算は **frame 内に必ず完了する必要** (apply は同 frame の display zone)、他候補と pool 共有すると競合 |
| **Worker B** | 案 R-refined (renderShadow pre-cull) | shadow updateCull + stateSort は **frame N-1 で先行構築 → frame N の renderShadow body で消費**。frame 跨ぎ pool となり A/C と scheduling が独立 |
| **Worker C** | 案 P-refined (並列 cull) | OCCLUDED 分離後の main_cam + shadow_cam 並列 cull 用。**P-refined prerequisite (§6) 完了まで立ち上げ保留** |

pool 数 = 3 とした理由:
1. **frame budget の分離**: A = frame 内完結、B = frame 跨ぎ可、C = frame 内完結 (P-refined 着手時)。同 pool に混ぜると scheduling 設計が破綻
2. **lock domain の分離**: A は reflection probe visibility queue writer、B は LLCullResult buffer (4 cascade × 11 sub-buffer) writer、C は per-camera mOcclusionState writer。lock 競合が pool 越えに起きない
3. **段階着手**: C は §6 prerequisite が片付くまで物理的に動かせない、A/B と独立 lifecycle

4 pool 以上にしない: Y-refined 3 候補で十分、pool 増は context-switch overhead と設計負担増。

### §4.2 Worker A 詳細 (案 O 専属 — task #10 設計詳細)

#### §4.2.1 thread 構成

- **thread 数**: 1 thread 固定 (doOcclusion async の serial execution が前提、Hero probe iteration loop は inherently sequential)
- **lifecycle**: viewer 起動時に立ち上げ、idle = condvar wait、frame 頭で main が dispatch、frame 中 1 回 only
- **shutdown**: viewer 終了時に `pool.drain() + pool.stop()` で停止、in-flight task は完了まで待機

#### §4.2.2 dispatch interface

```cpp
// 概念 — Phase 2 で確定
class DoOcclusionWorker {
public:
    void enqueue(const ProbeOcclusionSnapshot& snap);  // main thread が frame 頭で呼ぶ
    void wait_apply(ProbeVisibilityResult* out);       // main thread が GL 発行直前で呼ぶ
private:
    std::thread mThread;
    std::atomic<bool> mTaskReady;
    std::condition_variable mCv;
    ProbeOcclusionSnapshot mSnap;
    ProbeVisibilityResult mResult;
};
```

#### §4.2.3 同期方式

- main → worker: `std::atomic<bool> mTaskReady` + `condition_variable::notify_one()`
- worker → main: 同様の done flag、main は `wait_apply` で wait
- snapshot は **immutable copy** (frame 頭で main が dump、worker は read-only access)
- result buffer は **per-frame fresh allocate** (vector swap by move)、lock 不要

#### §4.2.4 failure fallback

worker が exception 投げたら apply は **no-op + 当該 frame は probe visibility 既存値継続使用** + log。memory `feedback_root_cause_not_dump` に従い fallback 発動は log で可視化、silent fallback はしない。

### §4.3 Worker B 詳細 (案 R-refined 専属)

#### §4.3.1 thread 構成

- **thread 数**: 1 thread 固定 (4 cascade を sequential に処理、cascade 間に依存無いが pool 内並列化は次段最適化)
- **lifecycle**: 同 Worker A、frame N-1 dispatch / frame N apply の frame-skew 運用
- **frame skew**: shadow 用 cull は 1 frame 遅延しても視覚回帰最小 (shadow は元々 LOD 落としで 1 frame stale が許容範囲)。Phase 2 着手時 visual diff で確認

#### §4.3.2 dispatch interface

```cpp
class ShadowPreCullWorker {
public:
    void enqueue(const ShadowRenderContext& ctx);   // 4 cascade 分の camera + frustum snapshot
    void wait_apply(LLCullResult* out_4_cascade);    // 4 個分の cull result buffer
private:
    std::thread mThread;
    ShadowRenderContext mCtx;
    LLCullResult mResult[4];
};
```

#### §4.3.3 同期方式

- main → worker: `enqueue` 時に ShadowRenderContext copy (cascade 4 個 × camera + frustum + sShadowRender 等)
- worker → main: 4 個の LLCullResult を pre-allocated buffer に書き、apply 時に move
- frame skew: worker は frame N の dispatch を受けて frame N+1 で apply (option)。Phase 2 で frame N と frame N+1 双方の運用を試して visual diff 取る

### §4.4 Worker C 詳細 (案 P-refined 専属)

§6 prerequisite 完了まで立ち上げ保留のため、本 spec では **interface のみ示し thread 数 / lifecycle は §6 完了時に確定**。

```cpp
class ParallelCullWorker {
public:
    void enqueue_main_cull(const CullSnapshot& main);
    void enqueue_shadow_cull(const CullSnapshot& shadow);
    void wait_apply(LLCullResult* out_main, LLCullResult* out_shadow);
};
```

prerequisite (§6) で OCCLUDED bit が `mOcclusionState[per-cam]` に分離された後、main_cam と shadow_cam の cull traversal が独立 buffer で並列実行可能。

### §4.5 共通契約 (3 pool 共通)

- **dispatch**: main thread が `enqueue(snapshot)` で投入、worker は内部で wait → process → done flag
- **完了通知**: worker → main は `std::atomic<bool>` + `std::condition_variable`、main は apply 直前で wait
- **shutdown**: viewer 終了時 main が `pool.drain() + pool.stop()` を呼ぶ
- **失敗時 fallback**: worker が exception 投げたら apply は **no-op** + 既存値継続 + log (memory `feedback_root_cause_not_dump`)
- **計測 zone**: 各 pool の enqueue / wait_apply は AYAPERF_ZONE で挟み、dispatch overhead が effect を食ってないか per-PR で確認

---

## §5. 候補ごとの剥がし境界 (snapshot / apply / 残置)

### §5.1 案 O: doOcclusion async (Worker A)

| 項目 | 内容 |
|---|---|
| worker 移送 | Hero probe iteration loop 全体 (parent − Σ rmdo_* = 77.3%、04 §4.2.i 実測根拠) — per-probe visibility 判定、result roll-up |
| snapshot 入力 | `LLReflectionMapManager::mReflectionProbes` snapshot (`std::vector<LLPointer<LLReflectionMap>>` の **shallow copy**、probe 本体は immutable for 1 frame) + 前 frame で発行した query id list + camera frustum + agent pos |
| apply 出力 | per-probe visibility bit (`bool[N]`)、main thread の `LLReflectionMapManager::doOcclusion()` 末尾で `markOccluder()` 反映 |
| **main 残置 (絶対)** | (a) **GL query 発行 (`glBeginQuery` / `glEndQuery`、`drawCube`)** — GL context tie で剥がし不可、(b) probe rebuild dispatch (LLDrawable mark dirty) — frame 内 race 回避 |
| lock | snapshot は frame 頭で 1 回 immutable copy、worker は read-only access、result は per-frame fresh buffer |
| 想定 unknown | (a) probe rebuild 中の snapshot 整合性 (worker 動作中に main が probe を rebuild する race) — Phase 2 着手前に probe lifecycle 確認、(b) snapshot allocation cost が dispatch overhead を膨張させないか — A14 等 reference 計測ない状況のため Worker A 立ち上げ時に snapshot コスト単独計測必要 |
| 関連 commit | Group L (commit `171d6b90d1`) + Group M (uncommitted 18 周目) + Group N (uncommitted 19 周目) — Phase 2 第 1 着手時に keep zone |

### §5.2 案 R-refined: renderShadow pre-cull worker (Worker B)

| 項目 | 内容 |
|---|---|
| worker 移送 | shadow camera **4 cascade 分** の `updateCull(shadow_cam, &result)` (line pipeline.cpp:12536) + `stateSort(shadow_cam, &result)` (line 12541) — read-only octree traversal + drawable state classification |
| snapshot 入力 | `ShadowRenderContext` struct (新規) — sShadowRender flag、sUseOcclusion flag、4 cascade 分の shadow camera (`LLCamera`)、frustum 6 plane × 4、agent pos、shadow split params |
| apply 出力 | 4 個の `LLCullResult` buffer — main thread の renderShadow body 内で各 cascade dispatch 時に消費 |
| **main 残置 (絶対)** | (a) **GL dispatch** — matrix push/pop、shader bind、`renderGeomShadow` / `renderAlphaObjects` / `renderMaskedObjects` (line ~12547-12714)、(b) shadow FBO bind / blit、(c) cascade 内 GL state mutation |
| lock | octree は **reader 並列、writer 排他** (RW lock)、shadow cull 中の writer は main のみ (LLDrawable add/remove は main 限定)。Phase 2 着手前に RW lock の per-frame 取得頻度実測 |
| 想定 unknown | (a) frame N-1 で先行構築する場合の visual diff (shadow 1 frame stale 受容可否) — Phase 2 着手時 A/B 比較、(b) sShadowRender 参照箇所 12 file 以上 (02 §E.1) のうち、ShadowRenderContext 化に伴う波及範囲の正確な site list (Phase 2 着手前に grep 確定) |
| 関連 grep 結果 | **15 files / 74 sites** 確定 (§5.2.1 詳細)、05 §10.2 NG 条件「30 file 以上」**未満** → 案 R-refined scope refine 不要、着手可 |

#### §5.2.1 sShadowRender / sUseOcclusion site list (Phase 2 着手前 gate 4、2026-05-26 grep 確定)

`git grep -n 'sShadowRender\|sUseOcclusion' indra/` 結果: **15 files / 74 sites**。02 §E.1 §F.3 想定「12 file 以上」+25% 程度、05 §10.2 NG 条件「30 file 以上」未満。

##### カテゴリ別内訳

| カテゴリ | 内容 | site 数 | files |
|---|---|---|---|
| **A. 宣言 / 定義 / cvar binding** | `pipeline.h:751` (sUseOcclusion decl) / `:791` (sShadowRender decl) / `pipeline.cpp:420` (sUseOcclusion init) / `:438` (sShadowRender init) / `:1446` (init call) / `llviewercontrol.cpp:715` (RenderUseOcclusion cvar setter) | 6 | 2 |
| **B. save / restore wrapper** | `S32 saved = sUseOcclusion; sUseOcclusion = 0; ...; sUseOcclusion = saved;` の RAII pattern (renderShadow / cube snapshot / window snapshot / 360 capture / impostor) | 14 | 5 |
| **C. 読み取り分岐 (大半)** | `if (LLPipeline::sShadowRender)` / `if (sUseOcclusion > 1)` 等の branching、render / cull / shadow logic 内 | ~54 | 13 |

##### B カテゴリ詳細 (ShadowRenderContext::Scope RAII 化対象)

| file:line | コンテキスト |
|---|---|
| `pipeline.cpp:12503-12507 / 12718-12719` | **renderShadow body** (案 R-refined の本丸) — `sShadowRender = true` + `sUseOcclusion = 0` をスコープに入れる |
| `pipeline.cpp:12942-12948` | cube snapshot 用 sUseOcclusion 退避 |
| `pipeline.cpp:13961-13966 / 14254-14257` | generateImpostor 用 sUseOcclusion + sShadowRender 退避 |
| `llviewerwindow.cpp:6702-6705 / 6837` | window snapshot 用 sUseOcclusion 退避 |
| `llviewerdisplay.cpp:889-892 / 1034` | mini-map / cube snapshot 用 sUseOcclusion 退避 |
| `llviewerdisplay.cpp:1333-1334 / 1368` | cube snapshot 別ハンドラ用 sUseOcclusion 退避 |
| `llviewerdisplay.cpp:1459-1460 / 1514` | hud render 用 sUseOcclusion 退避 |
| `llfloater360capture.cpp:486-489 / 622` | 360 capture 用 sUseOcclusion 退避 |

##### C カテゴリ詳細 (本 PR では unchanged 維持)

| file | site 数 | 用途 |
|---|---|---|
| `pipeline.cpp` | ~21 | shadow 抑制 (4624/4639/4724/4729)、reflection 抑制 (3182/3209/4542)、cull predicate (3153/4041/4107/4491/4517/5105/13802/13817)、cube 等 (3087/3088/3233/4517/12605/12607) |
| `llspatialpartition.cpp` | 5 | cull traversal occlusion check (1067/1158/1236/1805) + shadow path check (1460) |
| `llvieweroctree.cpp` | 3 | octree occlusion enable / mode check (1109/1175/1329) |
| `llvoavatar.cpp` | 3 | shadow render 時に head 含める分岐 (6104/6189/6248) |
| `lldrawable.cpp` | 2 | drawable cull predicate (1579/1585) |
| `llfetchedgltfmaterial.cpp` | 2 | shadow path で alpha mode override (76/98) |
| `gltfscenemanager.cpp` | 1 | gltf scene shadow path skip (823) |
| `llviewerjoint.cpp` | 1 | joint shadow render 分岐 (91) |
| `lldrawpoolterrain.cpp` | 1 | terrain shadow shader 分岐 (106) |
| `llviewertexture.cpp` | 1 | shadow path assertion `llassert(!sShadowRender)` (1807) |

##### 波及範囲の結論

- **案 R-refined Phase 2 PR の touch 範囲**: B カテゴリ 14 sites + A カテゴリ 2 sites (init 経由 struct 化、cvar binding 変更) = **16 sites / 7 files**
- **C カテゴリ ~54 sites は本 PR では unchanged**。worker B は B カテゴリ RAII で構築された ShadowRenderContext の copy を受け取り read するため、C カテゴリの `if (sShadowRender)` は main thread での読みだけ続き、worker 移送に影響しない
- 長期で全 74 sites を thread-local accessor (`ShadowRenderContext::current().sShadowRender`) に切り替える option は別 spec (r32+ 候補)

##### 05 §10.2 NG 条件評価

| NG 条件 | 数値 | 判定 |
|---|---|---|
| sShadowRender 化が 30 file 以上に波及 (= 工数破綻、scope refine) | 7 files (B+A、worker 着手 PR) / 全体 15 files | 🟢 **PASS** (両基準で NG 条件未満) |

→ **案 R-refined 着手前 gate 4 (sShadowRender 12 file 以上の正確な site list) クリア**。

---

### §5.3 案 P-refined: mState atomic + 並列 cull (Worker C)

| 項目 | 内容 |
|---|---|
| worker 移送 | main_cam + shadow_cam の cull traversal 並列実行 (main_cam は Worker C-1、shadow_cam 4 cascade は Worker C-2..5 候補)、各々 per-task LLCullResult buffer 書込 |
| snapshot 入力 | camera frustum × 5 (main + shadow 4)、octree root pointer (read-only)、agent pos |
| apply 出力 | 5 個の LLCullResult buffer (main thread が end-of-cull で union) |
| **main 残置 (絶対)** | (a) LLCullResult buffer union (5 個を main の cull result に merge)、(b) post-cull state mark (LLSpatialGroup mark dirty 等)、(c) GL state mutation 全般 |
| lock | (a) mState は **atomic U32 化** (02 §E.2)、(b) per-task push buffer は worker 独立、(c) union 時のみ main 排他 (短時間) |
| **必須 prerequisite** | **§6 OCCLUDED 分離 refactor** — これ無しでは false-positive occlusion で shadow 描画破綻 |
| 想定 unknown | (a) mOcclusionState[per-cam] 分離後の cross-camera occlusion 共有効果損失 (02 §F.3 risk register、shared occlusion で 0.5 ms 未満なら drop して安全側)、(b) atomic mState の read overhead が cull traversal hot loop で問題化しないか — Phase 2 着手時 baseline 取得 |

---

## §6. 案 P-refined 前提 = OCCLUDED 分離 refactor (global refactor 単独工程)

### §6.1 工程の位置付け

案 P-refined の構造的 prerequisite として **mState の OCCLUDED bit (0x10000) を mOcclusionState[per-camera] に分離する refactor** が必要 (02 §E.2 RED blocker、02 §F.3 risk 1)。

**本工程は P-refined と切り離して単独 spec / 単独 PR とする判断を Phase 1.2 完了時 (AYA review 時点) に取る**:

- **option A (単独 spec)**: 06-occluded-bit-separation.md 等の独立 spec として工程を切り出し、P-refined 着手前にレビュー → merge → 視覚回帰確認 → 然る後 P-refined 着手
- **option B (P-refined spec の §0 として取り込み)**: P-refined 着手時に 1 PR の prerequisite step として実施 (small refactor かつ視覚回帰確認 frame profile で完結する場合)

判断軸: 視覚回帰確認の規模。occlusion 共有最適化が複数経路 (shadow / probe / impostor) で効いているなら option A、shadow 単独なら option B。Phase 1.2 完了時の AYA review で確定。

### §6.2 refactor 内容

| 段階 | 内容 |
|---|---|
| Step 1 | `LLSpatialGroup::mState` から OCCLUDED bit (0x10000) を削除、`SG_STATE_INHERIT_MASK` から除外 |
| Step 2 | `mOcclusionState[sCurCameraID]` (既存) に OCCLUDED 相当の bit を追加、setter/getter を `getOcclusionState(camera_id)` 経由に統一 |
| Step 3 | OCCLUDED 参照 site (現 mState 0x10000 を read している全箇所) を `mOcclusionState[camera_id]` 経由に書き換え |
| Step 4 | 視覚回帰確認 — shadow / probe / impostor の各経路で frame diff 撮影、occlusion shared 期待箇所で false negative が出ないか |

### §6.3 視覚回帰確認手順

1. baseline frame profile 撮影 (refactor 前、19 周目 baseline CSV を流用可)
2. refactor 後 frame profile 撮影
3. screenshot diff (memory `feedback_render_bug_canary_protocol` の canary 色塗りを応用、occlusion で消えるべき drawable に色 uniform 仕込み)
4. 共有最適化が損失する場合の ms 増加を測定 — **0.5 ms/frame 未満なら go**、それ以上なら refactor 設計やり直し (mOcclusionState の cross-camera sharing 機構を追加)

### §6.4 関連 grep

- mState の 0x10000 read/write site: `git grep -n '0x10000\|OCCLUDED\|mState' indra/newview/llspatialpartition.cpp indra/newview/llpipeline.cpp`
- mOcclusionState 既存 site: `git grep -n 'mOcclusionState' indra/`
- SG_STATE_INHERIT_MASK 参照: `git grep -n 'SG_STATE_INHERIT_MASK' indra/`

(具体 file:line は Phase 2 §6 着手時に grep 結果を本 spec に追記)

---

## §7. Phase 2 着手順序 (ROI × 独立性)

### §7.1 判定軸

- **ROI**: 期待 ms 削減 (§3.1 ceiling) / 工数見積
- **独立性**: 他候補との依存 (snapshot 共有 / apply 順序)
- **prerequisite**: §6 OCCLUDED 分離が後続候補を blocking しているか

### §7.2 確定順序

| 順 | 候補 | 理由 | 工数感 |
|---|---|---|---|
| 1 | **案 O (doOcclusion async)** | ceiling 最大 (5.7-6.5 ms)、構造シンプル、Worker A 1 thread 単独 pool で立ち上げ、他候補と依存無し | 1-1.5 週 (Worker A pool infra + snapshot 配線 + apply 結線) |
| 2 | **案 R-refined (renderShadow pre-cull worker)** | 案 O と independent、Worker B 立ち上げ、ShadowRenderContext 化と sShadowRender 12 file 波及修正が主 | 1 週 (Worker B + 4 cascade buffer + sShadowRender param 化) |
| 3 | **§6 OCCLUDED 分離 refactor** | P-refined の prerequisite、視覚回帰確認手順込み | 2-3 日 (refactor 本体) + 視覚回帰確認 (frame diff 撮影 × 数 scene) |
| 4 | **案 P-refined (mState atomic + 並列 cull)** | §6 完了後着手、Worker C 立ち上げ、main_cam + shadow_cam 並列 | 1-1.5 週 |

### §7.3 順序の根拠

- **案 O を最初に置く**: ceiling 最大 + 構造シンプル + 他候補 prerequisite なし、Worker pool infra (§4.5 共通契約) の reference impl も兼ねる
- **案 R-refined を 2 番目**: 案 O と独立 (worker / lock / snapshot 全て別)、Worker B は frame-skew 運用で frame 跨ぎ pool の reference
- **§6 を 3 番目**: P-refined 着手の物理的 blocker。option A (単独 spec) を取る場合は本順序、option B (P-refined 取り込み) なら 4 と統合
- **案 P-refined を最後**: prerequisite (§6) 完成 + 案 O/R で worker pool infra 運用知見が溜まってから着手

---

## §8. 各候補の per-candidate go/no-go gate (Phase 2 内運用)

### §8.1 gate 運用

各候補は **独立 PR** (memory `feedback_experiment_branch_single_scope`)、PR 内で以下 3 commit に分ける:

1. **baseline commit**: zone 追加 + 計測 CSV 取得 + 期待 ceiling 確認
2. **実装 commit**: worker 移送 + snapshot/apply 配線
3. **再計測 commit**: 同 scene / 同条件で CSV 取得 + 効果確認

### §8.2 採否判定 (memory `feedback_build_only_verified`)

| 判定 | 条件 | アクション |
|---|---|---|
| 🟢 PASS | 再計測 - baseline ≥ **02 §F.1 ceiling の 50%** | merge、次候補へ |
| 🟡 BORDERLINE | 50% 未満 だが構造修正で達成可能 | PR 内で revise、再計測 1 回まで |
| 🔴 FAIL | 50% 未満 + 構造修正できず | **revert + 本 spec を見直し**、当該候補は r32+ 章へ送り |

### §8.3 候補別 ceiling 50% threshold

| 候補 | ceiling (02 §F.1) | 50% threshold |
|---|---|---|
| 案 O | 5.7-6.5 ms/frame | **2.85-3.25 ms/frame** |
| 案 R-refined | 2-3 ms/frame | **1.0-1.5 ms/frame** |
| 案 P-refined | 4-5 ms/frame | **2.0-2.5 ms/frame** |

### §8.4 計測条件統一

- scene: Aditi Bonifacio 固定 SLurl (run_perf.py 既存 harness)
- duration: 90s (baseline と再計測 同条件)
- cvar: AYAPerfLogEnabled=1 で当該候補関連 zone を全 capture
- 計測実施者: Claude (memory `feedback_log_reading` + `feedback_proactive_diagnostic`)、AYA に raw CSV 貼り付けさせない

---

## §9. Open questions / unknown (Phase 2 着手前に潰す)

### §9.1 全体

- **Q1**: Worker A/B の thread 数は 1 で十分か (現 §4.2.1 / §4.3.1 で各 1 固定)、それとも 2 以上で並列度が必要か — A14 reference impl が無いため Worker A 立ち上げ時に直接計測
- **Q2**: pool dispatch 同期コスト (`enqueue` + `wait_apply`) の overhead が 0.05 ms 切るか — 案 O 着手時の per-frame zone で確認、超えるなら設計やり直し

### §9.2 候補別

- **案 O**:
  - 案 O-Q1: probe rebuild 中の snapshot 整合性 (worker 動作中に main が probe rebuild する race) — Phase 2 着手前に probe lifecycle 確認
  - 案 O-Q2: snapshot allocation cost (`std::vector<LLPointer<LLReflectionMap>>` shallow copy が dispatch overhead 膨張させないか) — Worker A 立ち上げ時に snapshot コスト単独計測

- **案 R-refined**:
  - 案 R-Q1: frame N-1 先行構築の visual diff (shadow 1 frame stale 受容可否) — Phase 2 着手時 frame skew あり/なし A/B
  - 案 R-Q2: sShadowRender 参照 12 file 以上 (02 §E.1 §F.3) の正確な site list と ShadowRenderContext 化の波及範囲 — Phase 2 着手前に grep 確定

- **案 P-refined**:
  - 案 P-Q1: §6 OCCLUDED 分離後の cross-camera occlusion 共有効果損失 (shared occlusion で 0.5 ms 未満なら go) — §6.3 視覚回帰確認手順で確定
  - 案 P-Q2: atomic mState read overhead が cull traversal hot loop で問題化しないか — Phase 2 着手時 baseline

### §9.3 §6 OCCLUDED 分離 単独 spec 化判断 (option A vs B、§6.1)

- 視覚回帰確認の規模が大きい (複数経路で occlusion 共有が効いている) → option A、独立 06 spec 化
- 規模が小さい (shadow 単独) → option B、P-refined PR の §0 として取り込み

Phase 1.2 完成時 (本 spec AYA review 時点) に occlusion 共有経路を grep で列挙し判断を確定。

---

## §10. Phase 2 着手 gate

### §10.1 着手 OK 条件

以下すべて満たした時点で Phase 2 (実装) 着手可:

1. 本 spec の AYA review PASS
2. §9.1 Q1 / Q2 が Worker A reference impl で数値確認できる準備整備済 (案 O 第 1 着手の baseline commit で確認)
3. §9.3 OCCLUDED 分離 option A/B 判断確定
4. §5.2 sShadowRender 12 file 以上の正確な site list grep 済 (案 R-refined 着手前) — **✓ 2026-05-26 確定 (§5.2.1、15 files / 74 sites、NG 条件未満)**

### §10.2 着手 NG 条件

- Worker A の dispatch overhead が 0.5 ms を超える (= worker pool 構成そのものが r40 では成立しない、設計やり直し)
- 案 R-refined の sShadowRender 化が 30 file 以上に波及 (= 工数破綻、scope refine)
- §6 OCCLUDED 分離で cross-camera occlusion 共有損失が 0.5 ms 超 (= P-refined 設計やり直し、shared occlusion 機構を別途維持)

### §10.3 着手後の運用

- 各候補は **独立 PR** (memory `feedback_experiment_branch_single_scope` 通り 1 branch 1 scope)
- 各 PR は §8.1 の 3 commit に分け、計測 commit は AYA review 対象
- per-candidate baseline は `~/.ayastorm_x64/logs/AYAstorm-perf-Npass-baseline.csv` に保存 (run_perf.py harness が自動 copy)
- 計測実施は Claude 主体 (memory `feedback_log_reading` + `feedback_proactive_diagnostic`)、AYA は build / 起動 / 動作確認のみ

---

## §11. 関連 spec / commit

| 参照先 | 用途 |
|---|---|
| `00-overview.md` §Phase 1.2 章 scope | r40 章 thesis、Y-refined scope 確定 |
| `02-offload-feasibility.md` §E / §F | 4 候補 POC + Day 7 統合判定、ceiling 算定根拠 |
| `03-perf-log-infra.md` §6.11-§6.13 | 計測 infra (Group L/M/N/O zone catalog) |
| `04-observed-hot-path-map.md` §4.2.h/i/j | Layer 6/8 実測 (案 O / Q の判断根拠) |
| `handoff-week1-deepdive-to-phase12.md` | 19 周目 → Phase 1.2 引き継ぎ |
| commit `171d6b90d1` | Layer 6 doOcclusion 3-split (Group L) — 案 O reference zone |
| (uncommitted) `indra/newview/llreflectionmap.cpp` | Group N rmdo_* zone (案 O 内部分解、Phase 2 着手時 keep) |
| (uncommitted) `indra/llui/llview.cpp` | Group O vwDraw_* zone (案 Q DROP 判定済、Phase 2 着手前に keep/外す判断) |

---

## §12. 改訂履歴

| 版 | 日付 | 内容 |
|---|---|---|
| 初版 | 2026-05-26 09:41 | 9 候補 / 3 pool (cull/parse/compute) の pre-thesis 版、A14 reference impl 想定 |
| Y-refined 改訂 | 2026-05-26 (本版) | Day 7 統合判定後、3 候補 (O / R-refined / P-refined) に集約、pool 構成を candidate 専属モデルに再定義、§6 OCCLUDED 分離単独工程化、per-candidate ceiling 50% gate 明示 |
