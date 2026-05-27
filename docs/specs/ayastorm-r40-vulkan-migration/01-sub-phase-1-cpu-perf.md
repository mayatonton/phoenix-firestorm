# r40 sub-phase 1: CPU perf 章 (closed 2026-05-27)

**status**: closed 2026-05-27 (Phase 2 全候補 REJECT)
**親 charter**: `00-charter.md`
**位置付け**: r40 章の sub-phase 1 (独立 chapter ではない)
**後継**: sub-phase 2 (`02-sub-phase-2-extended-falsify.md`)

---

## 1. sub-phase thesis (CPU perf 着手時)

「**AYAstorm の CPU side perf 余白を、OpenGL main thread から重い処理を剥がして worker thread に移すことで獲得する。Vulkan 化なしで現行 OpenGL stack のままマルチプロセス並列化を達成する。**」

着手時の前提:
- OpenGL の main thread 拘束は API 制約として認識済み (GL context は main thread bind)
- 剥がし対象は GL call そのものではなく、GL 呼出周辺の純 CPU 処理 (cull / occlusion 判定 / pre-cull 等)
- worker thread への剥がしで main thread 余白を作り、その余白に別 work (描画拡張 / 機能追加) を乗せる

## 2. Phase 1.1: 地図完成

### 計測 infra 構築

- **AYAPerfLog**: Tracy 風 zone CSV writer + cvar 制御の計測 harness
  - zone enter/exit 時に CSV 行 emit、frame 単位で集約
  - cvar `AYAPerfLogEnable` で ON/OFF、出荷 default OFF
- **run_perf.py harness**: bench 実行 / CSV 解析 / 統計算出の python script
  - backend agnostic (sub-phase 3 = Vulkan migration でも流用可能)
- **baseline CSV**: 1670 frame 実測 (2026-05-27)
  - 後続 phase の比較 ground truth として保存

### Layer 1-8 zone 配線

CPU frame の hot path を 8 layer に分解、各 layer に AYAPerfLog zone を配線:

| layer | 範囲 | hot helper 例 |
|---|---|---|
| 1 | frame top | `LLAppViewer::idle()` 入口 |
| 2 | idle network | network poll / message queue |
| 3 | idle update | object update / animation tick |
| 4 | render prepare | `LLPipeline::updateRender()` |
| 5 | cull | `LLPipeline::stateSort()` + `LLSpatialPartition::cull()` |
| 6 | occlusion | `LLSpatialGroup::checkOcclusion()` |
| 7 | render execute | shadow / deferred / forward dispatch |
| 8 | swap / present | GL buffer swap |

### deep-dive 報告

`docs/specs/ayastorm-render-perf-survey.md` 内の §A1-A14 として各 layer の hot helper を deep-dive:

- A1-A4: cull / occlusion 周辺 (layer 5-6)
- A5-A8: render execute 周辺 (layer 7)
- A9-A12: pipeline.cpp 3 大グローバル (sCull / sShadowRender / sCurCameraID) の影響範囲
- A13-A14: geometry mutation (rebuildMesh / markOccluder) の main thread 拘束理由

### 剥がし候補絞込

deep-dive 結果から剥がし候補 4 件に絞込:

- **案 O**: doOcclusion async (occlusion 判定の async 化)
- **案 Q**: vwDraw text cache (text rendering の cache 化)
- **案 R-refined**: renderShadow pre-cull (shadow 用 pre-cull の worker 化)
- **案 P-refined**: parallel cull (cull 全体の worker 化)

各案ごとに worker-safe な剥がし上限 (= main thread 余白獲得 ms/frame) を理論算定、Phase 2 で実装/pre-impl 検証。

## 3. Phase 2: 全候補 REJECT

### 案 O (doOcclusion async): commit `f695722a07` 実装後 gate REJECT

- 実装: `LLSpatialGroup::doOcclusion()` を worker thread に async dispatch、結果を次 frame で main thread が回収
- gate 検証: 実測 main thread 余白獲得 = gate (1.33 ms/frame) に対し有意な不足
- verdict: REJECT、default OFF で出荷
- 残存: `RenderOcclusionAsync` cvar で opt-in 可能、ただし default 利用前提なし

### 案 Q (vwDraw text cache): 実測 0.5% で DROP

- 実装前段で hot path 寄与率を実測
- vwDraw (= viewer-draw text rendering) の frame 寄与 = 0.5% (= 0.06-0.1 ms/frame at 60-100 ms baseline)
- gate に到底届かない、実装する value なし
- verdict: DROP (実装せず)

### 案 R-refined (renderShadow pre-cull): pre-impl REJECT

- 実装前段で worker-safe な剥がし上限を理論算定
- renderShadow pre-cull は frustum cull の subset、main thread 拘束部 (geometry mutation / GL state lookup) を除いた純 CPU 処理が worker-safe 候補
- 算定結果: worker-safe 上限 = 0.25 ms/frame
- gate 1.33 ms に対し **84% 不足**
- verdict: pre-impl REJECT (実装せず)

### 案 P-refined (parallel cull): pre-impl REJECT

- 実装前段で worker-safe な剥がし上限を理論算定
- parallel cull は cull 全体を worker 並列化、ただし pipeline.cpp 3 大グローバル参照 + cull 内 GL 呼出 (`checkOcclusion`) で main thread 拘束部が残る
- 算定結果: worker-safe 上限 = 0.29 ms/frame
- gate 1.33 ms に対し **78% 不足**
- verdict: pre-impl REJECT (実装せず)

## 4. Root cause: OpenGL main 剥がしの構造的不能

Phase 2 全 REJECT の root cause を特定:

### 4.1 pipeline.cpp の 3 大グローバル

| グローバル | 役割 | 並列化阻止理由 |
|---|---|---|
| `sCull` | 現 frame の cull 結果 (octree / spatial group cache) | mutable global、worker 並列で race window |
| `sShadowRender` | shadow render 中の state (camera / matrix / pass index) | main thread 専用、worker からの参照不可 |
| `sCurCameraID` | 現在描画中の camera ID (cube map face / shadow cascade idx) | main thread loop で逐次更新、worker 並列で stale read |

これら 3 つは pipeline.cpp の全 hot helper から参照される、global state が main thread 専用前提で設計されている。

### 4.2 cull/stateSort 内の GL 呼出

- `LLSpatialGroup::checkOcclusion()` は GL query object 経由で occlusion 判定
- GL context は main thread bind、worker thread から GL call 不可
- → cull 全体を worker 化しても checkOcclusion で main thread 戻り (= async 待ち)、worker 並列の利点を消す

### 4.3 geometry mutation の main thread 拘束

- `rebuildMesh()` / `markOccluder()` は LLDrawable / LLSpatialGroup の mutable state を変更
- GL VBO upload を伴う場合がある (GL context bind 制約)
- worker 並列で mutation すると次 frame の main thread cull / render と race

### 4.4 帰結: 構造的に不可能

剥がし上限 = 理論の 4% (worker-safe 0.25-0.29 ms / gate 1.33 ms)、つまり OpenGL stack のままでは main thread 拘束を維持しつつ剥がせる余白がほぼゼロ。

**OpenGL viewer の main 剥がしは構造的に不可能** と確定。pipeline.cpp 3 大グローバル + cull/stateSort 内 GL 呼出 + geometry mutation を全部 worker-safe に再設計する work は、それ自体が pipeline.cpp 全面書換に等しく、Vulkan 化と同等の規模になる。

## 5. sub-phase close 判断 + 後継選択経緯

### close 判断 (2026-05-27)

- Phase 2 全 4 案 REJECT、root cause = OpenGL stack の構造的制約
- 剥がし路線 (現 OpenGL のまま CPU 並列化) は完全に塞がった
- AYA 確定: 「**根本想定を再考します。現状の OpenGL と Main を剥がすこと自体が不可能と認めます**」

### 後継候補 (2026-05-27)

3 案を検討:

- **(α) GPU compute migration**: 重い CPU 処理を GPU compute shader (OpenGL Compute Shader 4.3+) に移す
  - 検討結果: 部分対応のみ可能 (cull の一部、occlusion query の代替)、3 大グローバル問題は解決せず
  - dual maintenance cost (GL + Compute Shader) > 一気完遂、却下
- **(β) GL 非依存 CPU 処理の純粋分離**: GL 呼出を含まない CPU helper だけ純粋分離して worker 化
  - 検討結果: 剥がせる helper の合計 = gate 1.33 ms に到底不足、案 R-refined / P-refined 同様
  - 却下
- **(γ) abstraction scaffold (Vulkan / GL dual backend)**: render API abstraction 層を入れて dual backend で進める
  - 検討結果: dual maintenance cost が一気完遂より高い (両 backend 永続 maintain)、却下

→ 全 alternative も却下、**Vulkan 一気移植 (GL 完全削除 / dual backend 禁止) が最低 cost path** と確定。

「**Vulkan 移植なくして未来の SL Viewer の生き残る道はゼロ。世界が消滅しているのと変わらない。Main に処理を追加するだけの余白はすでになく、現在の Viewer をのままメンテすることの意義は薄い**」

### 同日中の再 pivot (2026-05-27 夕): 自前 Vulkan も drop

同日中の追加検討で自前 Vulkan も drop、後継 = sub-phase 2 (LL Vulkan 着地待ち + 鉱脈発掘) に置換:

- 自前 Vulkan は「LL 着地時 reset で乗換」前提 = **中継ぎ性質**
- 中継ぎが LL reset で消えるなら、中継ぎ自体作らない方が経済合理
- viewer fork による Vulkan 完遂事例ゼロ
- AYA 確定: 「**君に同意してリンデンが Vulkan を成功させるのを祈るが正解に見えてきた。やれることは Main 処理の無駄を削って 5% の鉱脈発掘**」

→ sub-phase 2 (LL 待ち + 鉱脈発掘) で 2026-05-27 起動。

ただし sub-phase 2 は翌 2026-05-28 に falsify (1 日鉱脈調査ゼロ)、sub-phase 3 = Vulkan 化選択に再 pivot。詳細 → `02-sub-phase-2-extended-falsify.md`

## 6. sub-phase 1 で生き残る成果物

sub-phase 3 (Vulkan migration) で再利用可能な成果物:

- **AYAPerfLog infra** — API agnostic、Vulkan 移植中の per-stage 計測に流用可
- **run_perf.py harness** — backend agnostic
- **baseline CSV (1670 frame, 2026-05-27)** — vk-α 後の比較 ground truth
- **02 §A1-A14 deep-dive** — Vulkan 移植時の CPU side 動作の根拠資料
- **04 hot path map** — vk-RC feature parity 確認時の比較対象
- **pipeline.cpp 3 大グローバル analysis** — sub-phase 3 の Vulkan 設計時に「触ってはいけない構造的 blocker」の根拠、LL Vulkan 着地後の機能 port 設計時にも再利用

## 7. 関連 doc / memory

### doc

- `00-charter.md` — r40 章 charter
- `02-sub-phase-2-extended-falsify.md` — sub-phase 2 詳細
- `docs/specs/ayastorm-render-perf-survey.md` — Phase 1.1 で参照した perf 棚卸し doc (§A1-A14 deep-dive 本体)

### memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active
- `feedback_falsification_as_progress.md` — Phase 2 全 REJECT が sub-phase 2/3 への絞り込み成果
