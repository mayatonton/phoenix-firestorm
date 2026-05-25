# Handoff: Layer 5 → doOcclusion phase (CPU perf 章)

**作成日**: 2026-05-26
**目的**: Layer 5 (10 周目 matrixInit drill + 11 周目 打ち手 A 検証) 完了状態を引き継ぐ。**vwDraw_setup ライン (13-17 ms gap) は GPU sync stall と確定 → CPU 射程外として打ち切り**。次 phase は **真の CPU work bottleneck である `doOcclusion` 6.17 ms** (Layer 2 で確定済) への drill + 打ち手議論。

---

## 1. 現在位置 one-liner

> **viewerWindowDraw 13-17 ms gap = `LLUI::setLineWidth(1.f)` 経由 `glLineWidth()` が trigger していた GPU pipeline sync stall と確定。打ち手 A (line 3020 削除) で setup zone は 0.003 ms に縮減、UI 回帰なし、ただし stall は `vwDraw_rootView` に移動 (frame 内 sync point の位置移動)。正味 frame time 削減 ~3 ms。13 ms 本体は CPU 視点では削減不可 (GPU 側 frame 完了待ち)。残る真の CPU bottleneck は doOcclusion 6.17 ms/frame (Layer 2 確定済) で、次 session の本丸。**

詳細: `04-observed-hot-path-map.md` §4.2.d (Layer 4) + §4.2.e (Layer 5 plan) + §4.2.f (Layer 5 結果 / 打ち手 A 設計) + §4.2.g (打ち手 A 検証 = stall 移動確定)、`03-perf-log-infra.md` §6.7 Group I + §6.8 Group J + §6.9 Group K。

---

## 2. BFS 方針 / 必読 memory

`feedback_perf_map_bfs_drill.md` を最初に。加えて Layer 5 で適用済みかつ次 phase でも load-bearing:

- **`feedback_admit_unknown.md`**: 本章で setup variance に対し **仮説 3 連続 falsify** (sun position / night-heavy / scene-light-place)。打ち切り判断の根拠。次 phase の doOcclusion drill でも、推論で結論せず実測で進める。
- **`feedback_doubt_self_first.md`**: 私が前 handoff の「同 SLurl AYA 確認済」記述を確認なしに引いて §4.2.d に書き、本 session 中 AYA に訂正を依頼するハメに。次 session ではAYA 確認済表現は 1 次引用元を疎ましてから再記する。
- **`feedback_proactive_handoff.md`**: 本書がその実践 (本 session で memory 強化済、再三指示済 absolute rule)。

---

## 3. Layer 5 確定 finding (10 + 11 周目、本 session で fix)

### 3.1 matrixInit 4 分解 (10 周目、CSV: `AYAstorm-perf-10pass-baseline.csv`)

active frame 2810、frame >= 9223 filter。

| zone | per-call (us) | 占有率 |
|---|---|---|
| matrixInit (parent) | 16649.33 | 100% |
| **matrixInit_setLineWidth** | **16643.74** | **99.97% ← 主犯確定** |
| matrixInit_loadIdentity | 0.26 | 0.0016% |
| matrixInit_dirtyRect | 0.15 | 0.0009% |
| matrixInit_matrixMode | 0.11 | 0.0007% |

完全分解 ✓ (4 zone Σ ≈ parent、差 5us)。

### 3.2 真犯 call chain (静的解析で特定)

```
LLUI::setLineWidth(1.f)                             // llviewerwindow.cpp:3020
  → LLRender2D::setLineWidth(1.f)                   // llui.h:330
    → gGL.setLineWidth(1.f * UIGLScaleFactor)       // llrender2dutils.cpp:1812
      → LLRender::setLineWidth(line_width)          // llrender.cpp:1509
        → if (mLineWidth != line_width || mDirty)   // guard 通過
          → glLineWidth(line_width)                 // 16.6 ms ← 主犯
```

guard が毎フレーム通過する構造的理由:
- `LLUI::setLineWidth(1.f)` は `1.f × UIGLScaleFactor` を渡す
- 他多数 caller (`gGL.setLineWidth(1.f)` raw、llspatialpartition / llmanip / llviewerwindow:4754 等 30+ 箇所) は **scale 倍率なしの素の 1.f** を渡す
- → `mLineWidth` は frame 中で `1.f` (raw) と `1.f × UIscale` (LLUI 経由) を行き来し、guard 常に miss

### 3.3 8/9/10 周目 variance triangulate 結果

| 周 | vwDraw_setup | matrixInit | setLineWidth | 結論 |
|---|---|---|---|---|
| 8 | 13.15 ms | 未計測 | 未計測 | 通常 regime |
| 9 | 3.57 ms | 3.56 ms | 未計測 | **outlier (低)** |
| 10 | 16.70 ms | 16.65 ms | 16.64 ms | 通常 regime |

通常 13-17 ms、9 周目だけ 3.5 ms。**原因: 同 SLurl / 同時刻帯 (夜) でも生じる確率的揺らぎ** (AYA 確認: 全周回同条件)。

仮説 3 連続 falsify (sun position / night-heavy / scene-light) → **打ち切り宣言**、`feedback_admit_unknown.md` 適用。

### 3.4 打ち手 A 検証 (11 周目、CSV: `AYAstorm-perf-11pass-baseline.csv`)

active frame 2166、frame >= 25478 filter。**時刻条件: 昼** (打ち手判定は zone 値で行うので時刻差は問題なし) / 同 SLurl。

| zone | 10 周目 (前) | 11 周目 (後) | 変化 |
|---|---|---|---|
| setup_matrixInit_setLineWidth | 16643.74 us | **0.03 us** | -99.9997% ✓ |
| vwDraw_setup parent | 16702 us | **3.27 us** | -99.98% ✓ |
| **vwDraw_rootView** | **730 us** | **13652 us** | **+1769% ← stall 移動** |
| viewerWindowDraw parent | 16702 us | 13684 us | -3018 us / -18% |
| display (frame) | 48.90 ms | 39.57 ms | -9.3 ms (時刻差込) |
| doFrame_total | 58.04 ms | 49.52 ms | -8.5 ms (時刻差込) |

UI 回帰チェック (AYA 確認): focus border / world map / 選択枠 / manip 軸 全て異常なし → **打ち手 A は維持**。

### 3.5 GPU pipeline sync stall 仮説の決定的確定

判定 4 (display 下がらず stall が rootView に移動) で確定:

```
旧 frame: [render...] → [LLUI::setLineWidth が prior GPU 完了待ち = 16ms] → [UI...]
新 frame: [render...] → [setLineWidth 無し] → [rootView->draw 最初の GL state change が prior GPU 完了待ち = 13ms]
```

→ `glLineWidth(1.0)` は **CPU work でなく GPU sync point**。call を消すと sync point は後段の次の GL state change に移行 (driver-level の物理現象、削減不可)。

正味効果: -3 ms (`glLineWidth` call そのものの CPU overhead 分のみ) + zone clean。

---

## 4. 次 phase = doOcclusion 6.17 ms 攻撃 (本丸)

### 4.1 Layer 2 で確定済の前提

| 親 zone | per-active-frame | doOcclusion 寄与 |
|---|---|---|
| renderGeomDeferred | 7.27 ms | doOcclusion 6.17 ms (**85%**) |

詳細: 04 spec §4.2.b。

renderGeomDeferred 内の `doOcclusion(camera)` は **真の CPU work** (`vwDraw_rootView` の GPU stall と異なり、CPU 側で削減余地あり)。

### 4.2 doOcclusion の中身 (静的把握、未実測)

`doOcclusion()` は `LLViewerObject::doOcclusion(LLCamera&)` ではなく、**`LLPipeline::doOcclusion(LLCamera& camera)`** の呼出系統。本体は:
- octree traverse + frustum cull
- per-spatial-group occlusion query state machine (NEW_OCCLUSION_REQUEST / WAITING / FINISHED)
- `glGetQueryObjectuiv(id, GL_QUERY_RESULT_AVAILABLE, ...)` で GPU 側 query 完了 polling
- 取得後 `glGetQueryObjectuiv(id, GL_QUERY_RESULT, ...)` で結果取得 → visible/occluded 判定

**CPU 側 hot path 候補** (Layer 2 終了時点で 6.17 ms を消費している):
1. octree traverse + frustum cull (毎 frame 全 spatial group)
2. `glGetQueryObjectuiv(GL_QUERY_RESULT_AVAILABLE)` polling (GPU 同期コスト含む可能性、 vwDraw_setup と同類の罠)
3. query result fetch
4. visible/occluded 状態遷移 + child group propagation

### 4.3 Layer 6 配線案 (次 session の最初の作業)

`indra/newview/pipeline.cpp::LLPipeline::doOcclusion()` の本体 (要確定: pipeline.cpp 内 `LLPipeline::doOcclusion` 定義行を Grep で特定 → body 内側を 3-4 zone で wrap):

| zone 名 (案) | 配線対象 |
|---|---|
| `doOcclusion_traverse` | octree traverse / frustum cull ループ全体 |
| `doOcclusion_queryAvailable` | `glGetQueryObjectuiv(GL_QUERY_RESULT_AVAILABLE)` polling ループ |
| `doOcclusion_queryResult` | `glGetQueryObjectuiv(GL_QUERY_RESULT)` 取得 + 判定 |
| `doOcclusion_groupUpdate` | spatial group state 更新 + child propagation |

判定:
- もし `queryAvailable` が 主犯 → これも GPU sync stall 系 (vwDraw_setup と同じ罠) → CPU 視点削減困難、別軸 (frame skip / lazy fetch)
- もし `traverse` または `groupUpdate` が 主犯 → 真の CPU work、データ構造 / アルゴリズム最適化で削減可能
- もし均等分散 → 各々の打ち手を考える

### 4.4 打ち手候補 (Layer 6 結果次第で確定)

- **A. frame skip**: occlusion query を毎 frame 走らせず N 周期に 1 回 (visibility は前 result を引き継ぐ)
- **B. lazy fetch**: query 結果取得を次 frame に遅らせる (1 frame の visibility 遅延を許容)
- **C. sub-sampling**: 全 spatial group でなく一部のみ (camera 周辺優先)
- **D. occlusion 完全停止**: AYAstorm の用途で occlusion が本当に必要か検討 (撮影シーンでは draw call 上限が問題にならない可能性)

打ち手 A-C は実装複雑、D は最も単純で効果大だが回帰リスク (重 region で draw call 爆発) あり。**D を最初に試して frame time 評価する のが最短**。

---

## 5. ビルド / 計測フロー (前 handoff §6 と同じ、差分のみ)

```bash
# 1. 11 周目 CSV を退避 (本 session 末で実施済)
# (済) mv ~/.ayastorm_x64/logs/AYAstorm-perf.csv \
#         ~/.ayastorm_x64/logs/AYAstorm-perf-11pass-baseline.csv

# 2. Layer 6 zone 配線 (pipeline.cpp::LLPipeline::doOcclusion 4 分割)
# 3. build (--no-configure)
# 4. install + cache clear
# 5. AYA さんに 12 周目 active session 録音依頼 (8-11 周目と同 SLurl、時刻条件は不問)
# 6. CSV 解析 (active session filter 必須)
```

---

## 6. CSV 退避ファイル一覧 (Layer 5 完了時点)

| 周 | ファイル | 用途 |
|---|---|---|
| 2-6 周目 | `~/.ayastorm_x64/logs/AYAstorm-perf-{2,3,4,5,6}pass-baseline.csv` | Layer 0-1 |
| 7 周目 (Layer 2) | `AYAstorm-perf-7pass-baseline.csv` | doOcclusion 6.17 ms 主犯特定 |
| 8 周目 (Layer 3) | `AYAstorm-perf-8pass-baseline.csv` | vwDraw_setup 13.15 ms 主犯特定 |
| 9 周目 (Layer 4) | `AYAstorm-perf-9pass-baseline.csv` | matrixInit 99.7% 占有 (setup 3.57 ms outlier) |
| **10 周目 (Layer 5)** | `AYAstorm-perf-10pass-baseline.csv` | **setLineWidth 16.64 ms 主犯確定** (setup 16.70 ms) |
| **11 周目 (Layer 5 打ち手 A 検証)** | `AYAstorm-perf-11pass-baseline.csv` | **打ち手 A 適用後、stall 移動確定** (setup 0.003 ms / rootView 13.65 ms) |
| 12 周目 (Layer 6、次 session) | 同 path 上書き | doOcclusion 4 分割 (traverse / queryAvail / queryResult / groupUpdate) |

---

## 7. 配線済 zone 全リスト (Layer 3 + 4 + 5)

### `indra/newview/pipeline.cpp::LLPipeline::renderShadow()` (line 12459-12667)

Layer 3 (8 周目):
- `renderShadow_body_cull` (line 12496 周辺)
- `renderShadow_body_geom` (line 12570 周辺)
- `renderShadow_body_alpha` (line 12577 周辺)

Layer 4 (9 周目):
- `renderShadow_body_matrixSetup` (line 12506)
- `renderShadow_body_innerOcclusion` (line 12567)
- `renderShadow_body_cubeTeardown` (line 12665)

### `indra/newview/llviewerwindow.cpp::LLViewerWindow::draw()` (line 3005-3245)

Layer 3 (8 周目):
- `uiRender_ui2d_vwDraw_setup` (外側 wrap)
- `uiRender_ui2d_vwDraw_toolAndOverlays` (旧名 topCtrl 系)
- `uiRender_ui2d_vwDraw_overlayTitle` (不発、gShowOverlayTitle=false)
- `uiRender_ui2d_vwDraw_teardown`
- `uiRender_ui2d_vwDraw_rootView` (mRootView->draw wrap)

Layer 4 (9 周目、setup 内側 5 分割):
- `uiRender_ui2d_vwDraw_setup_stopGlerror`
- `uiRender_ui2d_vwDraw_setup_matrixInit`
- `uiRender_ui2d_vwDraw_setup_displayTimecode`
- `uiRender_ui2d_vwDraw_setup_uiProgramBind`
- `uiRender_ui2d_vwDraw_setup_pushMatrices`

**Layer 5 (10 周目、matrixInit 内側 4 分割)**:
- `uiRender_ui2d_vwDraw_setup_matrixInit_setLineWidth` (line 3022、**本体 call は打ち手 A で削除済**、zone 包囲のみ残置)
- `uiRender_ui2d_vwDraw_setup_matrixInit_matrixMode` (line 3027)
- `uiRender_ui2d_vwDraw_setup_matrixInit_loadIdentity` (line 3032)
- `uiRender_ui2d_vwDraw_setup_matrixInit_dirtyRect` (line 3038-3044)

### 次 session で配線予定 (Layer 6、doOcclusion 4 分割)

`indra/newview/pipeline.cpp::LLPipeline::doOcclusion()` 内部に 4 zone (詳細 §4.3)。

---

## 8. 打ち手 A 適用済の実コード変更 (uncommitted、要 AYA 相談)

`indra/newview/llviewerwindow.cpp` line 3020 周辺:

```cpp
{ // <FS:AYAstorm> Group K-1 (Layer 5 で 16.6 ms/frame の主犯と特定)
    // <FS:AYAstorm> CPU perf 章 r31 P0: LLUI::setLineWidth(1.f) を削除。
    // LLUI::setLineWidth は LLRender2D::setLineWidth 経由で UIGLScaleFactor を乗じるため、
    // 他多数の raw gGL.setLineWidth(1.f) caller と mLineWidth guard 値が一致せず、毎フレーム
    // glLineWidth() を発火させていた。default 1.0 で reset したい意図なら他 caller が責任を
    // 持つべき (各 caller は既に末尾で `gGL.setLineWidth(1.f)` または LLUI 経由で 1.f に戻している)。
    AYAPERF_ZONE("uiRender_ui2d_vwDraw_setup_matrixInit_setLineWidth");
    // LLUI::setLineWidth(1.f);  // 削除: r31 P0 / CPU perf 章 §4.2.f
} // </FS:AYAstorm>
```

UI 回帰なし AYA 確認済 (focus border / world map / 選択枠 / manip 軸)。

---

## 9. 必読 memory (前 handoff §9 と同じ、+ 追加)

前 handoff §9 全項目 + 以下:

| memory | 追加理由 |
|---|---|
| `feedback_admit_unknown.md` | **本 session で 3 連続 falsify、variance 原因特定を打ち切る判断の根拠。doOcclusion phase でも同原則** |
| `feedback_proactive_handoff.md` (更新済) | **再三指示済 absolute rule、本 session で memory 強化。次 session 開始時も context 残量監視** |
| `feedback_render_full_trace_first.md` | doOcclusion 4 分割は GLSL/shader でなく CPU 側 query polling + traversal の trace、grep でなく完全 trace |

---

## 10. 既知 risk

1. **uncommitted diff の蓄積継続**: 本 session 終了時点で 17+ file uncommitted (zone 配線 + spec 大量追記 + memory 更新 + 打ち手 A 適用)。**次 session 開始直後に AYA と commit 戦略相談** (粒度: Layer ごと / 打ち手ごと / spec ごと の分割案を提示する)。
2. **AYAPerfLogEnabled=1 が persist 状態**: 次 session でも計測継続するなら問題なし、05 spec 打ち手議論で off に戻す案内を必ず入れる。
3. **観測者効果の累積**: Layer 6 で更に 4 zone 追加 = 累積 9 (setup 内) + 7 (renderShadow body) + 4 (doOcclusion) = 20 zone。12 周目 で既存 zone (renderShadow 等) が 8-11 周目値と整合しているか必ず sanity check。
4. **stall が rootView に移動した状態のまま**: 13ms gap は frame 内のどこかに必ず現れる (GPU 物理制約)。CPU perf 章では「これは射程外」と書いて他章送りで OK。誤って drill しようとしない。

---

## 11. 引き継ぎ session 開始時の AYA さん発話例

```
docs/specs/ayastorm-cpu-perf/handoff-layer5-to-doocclusion.md 読んで進めて。
```

または:
```
handoff-layer5-to-doocclusion.md 読んで、Layer 6 (doOcclusion 4 分割) 配線お願いします。
```

または commit 戦略を先に相談したい場合:
```
handoff-layer5-to-doocclusion.md 読んで、まず uncommitted diff の commit 戦略を相談したい。
```

---

## 12. Open task list (引き継ぎ時に TaskCreate)

- [ ] **uncommitted diff の commit 戦略 AYA 相談** (現時点 17+ file、最優先)
- [ ] Layer 6 zone 配線 (pipeline.cpp::LLPipeline::doOcclusion 4 分割: traverse / queryAvail / queryResult / groupUpdate)
- [ ] ビルド + install + cache clear (Claude 連続実行 OK)
- [ ] AYA に 12 周目 active session 録音依頼 (同 SLurl、時刻不問)
- [ ] 12 周目 CSV 解析 + doOcclusion 内訳特定
- [ ] 04 spec §4.2.h に Layer 6 結果追記
- [ ] 03 spec §6.10 Group L (Layer 6 catalog) 追記
- [ ] doOcclusion 打ち手議論 (案 D = 完全停止を最初に試す or 案 A-C の段階削減)
- [ ] 打ち手検証用 build + 13 周目計測
- [ ] (Layer 6 完了後) 05 spec 起こし (打ち手議論全体まとめ + 出荷判断)
- [ ] `AYAPerfLogEnabled=0` 戻し案内 (05 spec 着手前に必須)
