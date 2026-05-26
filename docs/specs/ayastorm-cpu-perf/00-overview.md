# AYAstorm CPU Perf 改善 — 章 overview

## この章の目的

AYAstorm viewer の重さは GPU でも、メモリでも、ネットでもなく **main thread 1 本** に集中している。
ここを資料ベースで棚卸しし、分散可能な処理を切り出すための長期テーマ folder。

> 「main thread 1 本」が律速。GPU は 5 割以上余っているのに FPS が 11〜20 まで落ちる。
> 改善は GPU 側 (shader / DoF / shadow) でなく **CPU drawcall submit / per-frame update を捌く側** を直すこと。

## 出発点となる実測値 (2026-05-25)

下に挙げる値はすべて **同一セッション 1 リージョン滞在中** の同時刻スナップショット。
将来 baseline と比較するため、ここに固定して残す。

### CPU / GPU バランス

| 項目 | 値 | 判定 |
|---|---|---|
| GPU (RTX 5090) utilization | **19 %** | 大幅余裕 |
| GPU memory used | 10.7 GB / 32 GB (2 %) | 余裕 |
| GPU power | 110 W | 余裕 |
| CPU 全体 (20 core) load average | 1.93 | 1.5 core 相当 |
| **main render thread (TID 181823) CPU** | **60 %** | 律速 |
| 2 番目 thread (TID 181797) CPU | **30 %** | おそらく worker / image decode |
| その他 39 thread | ほぼ 0 % | sleeping |

### Frame / Render

| 項目 | 値 |
|---|---|
| FPS | 20.8 |
| frame (mean) | 47.8 ms |
| frametime 99th pct | 54.8 ms |
| jitter | 2.2 ms (= 安定だが地力で重い) |
| KTris per Frame | **1,608** (= 1.6 M tri/frame) |
| KTris per Sec | 34,163 |
| Total Objects | **21,761** |
| Material Count | 192 |
| **Occlusion Queries Performed** | **3,534 /s** |
| **Objects Occluded** | **3.169 /s** ← ほぼゼロ |
| Object Unoccluded | 0 /s |

### Simulator / Network

| 項目 | 値 |
|---|---|
| Sim FPS | 44.9 (full 45 にほぼ届く) |
| Physics FPS | 44.9 |
| Time Dilation | 0.998 (sim 側余裕) |
| Ping Sim | 188 ms |
| Packet Loss | 0.9 % |
| UDP Data Received | 134 Kb/s |
| Actual In | 16.8 KB/s |
| Actual Out | 1.2 KB/s |

### 補助所見

- `Easy_28` (CURLE_OPERATION_TIMEDOUT) が複数 simhost の `/cap/...` POST で同時多発
- `[ayastorm:occlude] tick` は 2 秒毎、`occluders=0 hit=0`、dt=0.08〜0.32 s (AYAstorm 独自負荷は軽い側)
- `WARNING Non Finite mOrigin` が region 追加付近で時々

## Phase 構造 (2026-05-26 AYA 確定)

| Phase | スコープ | 完了条件 |
|---|---|---|
| **Phase 1: 並列化設計** | (a) 地図完成 + (b) Core 振り分け設計案 | 各剥がし候補が「どの worker thread/Core に乗せるか」 spec 化済 |
| Phase 2: 実装 | 03 以降で Phase 1.2 設計に基づいて剥がし実装 | 1 改善 1 PR、累積で main 60% → 30%台 |
| Phase 3: 再計測 | 各実装の効果を CSV 比較で確認 | – |

**Phase 1 完了までは Phase 2 (実装) に着手しない。** AYA の章 thesis (main 集中の構造的剥がし) を守るための強制ゲート。

## ロードマップ

```
00-overview.md                       ← このファイル
01-mainthread-workload-inventory.md  ← Phase 1.1: 徹底 CPU 棚卸し (実コード trace ベース)
02-offload-feasibility.md            ← Phase 1.1: 01 の各処理を分散可能性で評価 (§A1-A14 deep-dive 済 2026-05-26)
03-perf-log-infra.md                 ← zone 計測 CSV infra
04-observed-hot-path-map.md          ← Phase 1.1: Layer 1-8 zone 配線実測 (Layer 6 doOcclusion + Layer 8 rmdo_* / vwDraw_*)
05-core-assignment-plan.md           ← Phase 1.2: Y-refined 3 候補 (案 O / R-refined / P-refined) の worker pool + 同期境界設計
06+ 並列化実装 (Phase 2)             ← 1 改善 1 ファイル 1 PR
```

順序は「Phase 1.1 地図完成 → Phase 1.2 設計案 → Phase 2 実装 → Phase 3 再計測」。
1 改善 = 1 ファイル = 1 PR を原則にする (release note per feature の方針と整合)。

## Phase 1.1 進捗 (2026-05-26 完成)

- ✓ §A1-A6 deep-dive 済 (idleUpdate / octree balance / occlusion query / NVIDIA fence / HTTP parse / Stream3D)
- ✓ §A8-A14 deep-dive 済 (light culling / luminance / draw batch / region visibility / particle sim / observer batch / processTextureStats)
- ✓ Layer 1-6 zone 配線 (Group A-M、commit `171d6b90d1` + 18 周目分既配線)
- ✓ Layer 7-8 zone 配線 (Group N rmdo_* / Group O vwDraw_* 動的、19 周目 1 週間 deep-dive で完成)
- ✓ 4 候補 POC spike (案 O / R / P / Q、Day 4-6) + Day 7 統合判定 (02 §F)

## Phase 1.2 章 scope (2026-05-26 Day 7 確定 — 案 Y-refined)

Phase 1.1 で **剥がし候補 4 件中 3 件 GO / 1 件 DROP** を確定。Phase 1.2 で取り扱う剥がし対象 (= r40 章の実装 scope):

| 候補 | 内容 | 期待 ceiling | 着手順 |
|---|---|---|---|
| **案 O (doOcclusion async)** | Hero probe iteration loop 全体を Worker A に移送、main は GL Begin/End のみ残し | 5.7-6.5 ms/frame | 1 |
| **案 R-refined (renderShadow pre-cull worker)** | shadow cascade 4 つの updateCull + stateSort を Worker B に移送、GL dispatch は main 残し | 2-3 ms/frame | 2 |
| **案 P-refined (mState atomic + 並列 cull)** | `mState` atomic 化 + per-task LLCullResult buffer + main_cam / shadow_cam 並列 cull (2026-05-26 grep finding で旧 OCCLUDED 分離 prerequisite は不要確定、02 §E.2 / 05 §6 削除ノート参照) | 4-5 ms/frame | 3 |
| 案 Q (vwDraw text cache) | DROP — 19 周目 Group O 実測 0.145 ms/frame、frame budget 0.5% (詳細 04 §4.2.j) | — | — |

剥がし候補 3 件合計の理論上限 = **11.7-14.5 ms/frame**。設計は 05-core-assignment-plan.md (Y-refined 改訂版)、判定根拠は 02 §F、計測根拠は 04 §4.2.h/i/j。

**02 §A1-A14 のうち Y-refined に含まれない候補 (A1/A5/A8/A11/A12/A14)** は本章 (r40) 射程外。Phase 1.2/2 では扱わず、後続章 (r41+) で再評価対象として記録のみ残す。

## 13-17 周目の脱線 (2026-05-25/26、履歴記録)

Layer 6 (LLPipeline::doOcclusion 3 分割 zone 配線、commit `171d6b90d1`) で `doOcclusion` が hot 上位と判明した直後、**02 §D 優先順位を無視して微小チューニング 5 周** に脱線した:

| 周 | 打ち手 | 結果 |
|---|---|---|
| 13 | (B) Hero probe 2 重 occlusion query 除去 (`114d3dd532`) | 効果不在、scene 違い疑い |
| 14 | Layer 7 hero/map 分割 (hero=0 確定) | 打ち手 B 効果なし確定 → `82c32ea932` で revert |
| 15 | Layer 8 4-zone 配線 (`queryGen` = 4940 us/frame 主犯) | 真因特定 |
| 16 | (C) angular size culling (r/d < 0.01 skip) | -7.4% のみ、構造的に弱い |
| 17 | (F) query pool BATCH=256 lazy gen | `queryGen` 99% 削減成功も `queryPush` +4700us 爆増、net +7650us 逆効果 |

AYA 指示 (2026-05-26):
- 打ち手 B は `82c32ea932` で revert 済
- 打ち手 C/F は uncommitted のまま破棄 (commit せず)
- **「短縮系打ち手は r40 章スコープ外」を再確認** → 本 chapter は並列化候補抽出に戻る

### 学び
- doOcclusion 内 query 削減のような **短縮系打ち手は剥がす可否判断に影響しない** ため章スコープ外
- glGenQueries pool 化は GPU driver state 移行 cost が glBeginQuery に shift するだけで net 勝つとは限らない (計測してから確定すべき)
- octree node 自動 probe register は spatial design 仕様、reflection probe 数を絞るには別レイヤー (空間設計 or update 周期間引き) からの介入要

### 残す zone 配線
- `171d6b90d1` Layer 6 LLPipeline::doOcclusion 3 分割 zone — 計測 infra として keep (Layer 6 全周回着手時に再利用)

### 棄てる試行
- Layer 7 hero/map 分割 (uncommitted)
- Layer 8 4-zone 配線 (uncommitted)
- 打ち手 C angular cull / 打ち手 F query pool (uncommitted)

## 進め方の原則

- **推測でなく実コード trace と実測**: grep 棚卸しで満足せず、entry point から呼出を辿って表化する
- **未確認は「未確認」と明示**: 推論で埋めない (memory: 分からないときは分からないと言う)
- **AYAstorm 由来 / LL/FS 由来 を分けて記す**: 改善方針が違う (前者は自分で直す、後者は upstream 互換を保ちつつ別 channel で再実装するか維持)
- **GL context 必須 / order 制約 のあるものは main 残置**: 移せないものは「なぜ移せないか」を 1 行で残す
- **設計倒れ spec を積まない**: 実装される予定が立っていないものは書かない (memory: 正しいことを積み上げる)

## 章末ゴール

- 「main thread 60 % のうち、AYAstorm 由来 X %、LL/FS 由来 Y %、移動可能 Z %」が言える状態
- それを根拠に 03 以降の改善が ROI 順に並ぶ
