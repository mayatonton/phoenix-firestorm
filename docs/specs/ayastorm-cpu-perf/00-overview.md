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

## ロードマップ

```
00-overview.md                       ← このファイル
01-mainthread-workload-inventory.md  ← 徹底 CPU 棚卸し (実コード trace ベース)
02-offload-feasibility.md            ← 01 の各処理を分散可能性で評価
```

以後、`03-*` 以降で実際の改善 1 件 1 ファイルを足していく。
順序は「計測 → 1 hot path bisect → 1 改善 → 再計測」のループ。
1 改善 = 1 ファイル = 1 PR を原則にする (release note per feature の方針と整合)。

## 進め方の原則

- **推測でなく実コード trace と実測**: grep 棚卸しで満足せず、entry point から呼出を辿って表化する
- **未確認は「未確認」と明示**: 推論で埋めない (memory: 分からないときは分からないと言う)
- **AYAstorm 由来 / LL/FS 由来 を分けて記す**: 改善方針が違う (前者は自分で直す、後者は upstream 互換を保ちつつ別 channel で再実装するか維持)
- **GL context 必須 / order 制約 のあるものは main 残置**: 移せないものは「なぜ移せないか」を 1 行で残す
- **設計倒れ spec を積まない**: 実装される予定が立っていないものは書かない (memory: 正しいことを積み上げる)

## 章末ゴール

- 「main thread 60 % のうち、AYAstorm 由来 X %、LL/FS 由来 Y %、移動可能 Z %」が言える状態
- それを根拠に 03 以降の改善が ROI 順に並ぶ
