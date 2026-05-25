# Handoff: Layer 4 → Layer 5 (CPU perf 章)

**作成日**: 2026-05-26
**目的**: Layer 3 (8 周目) + Layer 4 (9 周目) 完了状態を引き継ぐ。**未解決の session 間変動 (setup 13.15 ms ⇔ 3.57 ms、両方夜・同位置)** が次セッションの最優先 task。

---

## 1. 現在位置 one-liner

> **Layer 3 (8 周目: renderShadow body 3 分割 + viewerWindowDraw 関数全域 4 分割) + Layer 4 (9 周目: vwDraw_setup 5 分割 + renderShadow body 内側 3 zone 追加) 完了。viewerWindowDraw 13 ms gap の 95% は setup 内、setup 内の 99.7% は matrixInit (setLineWidth/matrixMode/loadIdentity/dirtyRect-if のいずれか) と判明。ただし 8 周目 setup 13.15 ms vs 9 周目 setup 3.57 ms の 3.7 倍変動が未解明 — Claude 仮説 (sun position / night-heavy) は 2 連続 falsify、推論停止 + 実測継続フェーズ。**

詳細: `04-observed-hot-path-map.md` §4.2.c (Layer 3 結果) + §4.2.d (Layer 4 結果、未記入なら次セッションで追記)、 `03-perf-log-infra.md` §6.7 Group I + §6.8 Group J。

---

## 2. BFS 方針 / 必読 memory

`feedback_perf_map_bfs_drill.md` を最初に。加えて Layer 2 → 3 → 4 で適用済の memory:
- `feedback_doubt_self_first.md`: Layer 2 で mRootView->draw 仮説、Layer 4 で sun position / night-heavy 仮説、計 3 件 falsify
- `feedback_admit_unknown.md`: **2 連続外したら推論停止して実測切替** — Layer 5 はこの原則で進める
- `feedback_proactive_handoff.md`: 本書がその実践

---

## 3. Layer 3 計測結果 (8 周目、2026-05-26 04:47)

### 3.1 計測条件
- viewer: `feature/r31-bugfix-1-credits` + Group A-I 全配線
- CSV: `~/.ayastorm_x64/logs/AYAstorm-perf-8pass-baseline.csv` (18.5 MB)
- 時刻条件: **夜** (Sansara/AYA さん常用 SLurl、推定)
- active session filter: `frame >= <renderShadow first frame>` で限定

### 3.2 主要 zone (active frame 平均)

| zone | per-active-frame | 占有 |
|---|---|---|
| viewerWindowDraw | 13.86 ms | (親) |
| **vwDraw_setup** | **13.15 ms** | **94.7% ← 主犯** |
| vwDraw_rootView | 0.70 ms | 5.0% |
| vwDraw_topCtrl | 0.0035 ms | 0.025% |
| vwDraw_overlayTitle | 未出現 (gShowOverlayTitle=false 想定) | — |
| vwDraw_teardown | 0.002 ms | 0.014% |

| renderShadow body 内 | per-call | 備考 |
|---|---|---|
| renderShadow_body_cull | (要追記) | frustum cull + pushRenderTypeMask |
| renderShadow_body_geom | (要追記) | pool dispatch (types[] fixed array) |
| renderShadow_body_alpha | (要追記) | alpha pass |

**8 周目 結論**: viewerWindowDraw 13 ms gap の真犯人は **setup block** (line 3007-3055)。topCtrl / overlayTitle / teardown は trivial。

---

## 4. Layer 4 計測結果 (9 周目、2026-05-26 05:05)

### 4.1 計測条件
- 同じ viewer + Group A-J 全配線
- CSV: `~/.ayastorm_x64/logs/AYAstorm-perf.csv` (28.7 MB、**次セッション開始前に `-9pass-baseline.csv` に退避**)
- 時刻条件: **夜** (AYA さん確認、§5 で重要)
- 同位置 (SLurl 完全一致、AYA さん確認)

### 4.2 主要 zone

| zone | per-active-frame |
|---|---|
| vwDraw_setup | **3.57 ms** (← 8 周目 13.15 ms から 3.7 倍低下) |
| vwDraw_setup_stopGlerror | 0.016 us (trivial) ← **falsified** |
| **vwDraw_setup_matrixInit** | **~3.56 ms (per-call 3565 us)** ← **setup の 99.7%** |
| vwDraw_setup_displayTimecode | 要追記 (trivial 想定) |
| vwDraw_setup_uiProgramBind | 0.74 us (trivial) ← **falsified** |
| vwDraw_setup_pushMatrices | 要追記 |

renderShadow body 追加 zone (innerOcclusion / matrixSetup / cubeTeardown) の数値は §4.2.d 04 spec で別途確定 (本書では省略)。

### 4.3 falsified hypothesis 一覧

| 仮説 | 出所 | falsify 根拠 |
|---|---|---|
| stop_glerror が setup の主犯 | Layer 3 設計時 Claude 推論 | 0.016 us = noise floor |
| gUIProgram.bind が setup の主犯 | 同上 | 0.74 us = noise floor |
| 「9 周目が日中、8 周目が夜だから setup 軽い」 | Claude session 間比較推論 | **AYA 確認: 9 周目も夜** |
| 「night = GPU sync stall で重い」 | Claude 直前提案 | 上記により前提崩壊 |

---

## 5. 未解決の最重要問題: setup 13.15 ms ⇔ 3.57 ms の 3.7 倍変動

### 5.1 確定条件 (AYA 確認済)
- 8 周目 / 9 周目 とも **夜**
- **同じ位置** (SLurl 完全一致)
- viewer build 差分: 9 周目 は 8 周目 + Layer 4 child zone 5 個追加のみ (子 zone は parent 内側に入るので parent 総和を増やしてもおかしくないが、3 倍以上低下するのは説明できない)

### 5.2 説明できないこと
- 8 周目 13.15 ms → 9 周目 3.57 ms = **−9.58 ms / −73%**
- 両方夜なら sun position 説は無効
- 両方同位置なら scene 内容差は基本ない (avatars/objects 変動は本来微差)
- AYAPERF_ZONE 計測 overhead が増えて parent が **下がる** 方向は通常起きない

### 5.3 残候補 (推論止めて実測で切り分けるべきリスト)
1. **計測ノイズ / GPU sync stall の確率的発生**: 同条件でも 3-4 倍変動するなら、計測 1 回では結論できない → **3 回目以降の再計測で triangulate 必須**
2. **session 内 frame 分布の偏り**: avg ではなく per-frame timeline 描いたら、8 周目に **一部 frame だけ突出** している可能性 (avg を引き上げる long tail frame)
3. **AYA さん active session 内容の差**: avatar 数 / UI 操作頻度 / chat window 開閉等
4. **driver state (GL context) の起動順依存**: 起動直後 vs しばらく走らせた後で sync 挙動が違う等
5. **時刻 internal (夜の何時か)**: SL の "夜" は cycle 内で多少変動、shadow cascade frustum 位置が変わる

### 5.4 Layer 5 plan (推奨)

**目的**: matrixInit 内のどの GL call で時間が消えているか確定 + variance 再現性確認

#### A. matrixInit 4 分割 (Layer 5 zone 配線)

`llviewerwindow.cpp::LLViewerWindow::draw()` line 3019 周辺の `vwDraw_setup_matrixInit` を 4 zone に分解:

| zone 名 (案) | 配線位置 |
|---|---|
| `setup_matrixInit_setLineWidth` | `gGL.setLineWidth(1.0f)` 1 行 wrap |
| `setup_matrixInit_matrixMode` | `gGL.matrixMode(LLRender::MM_MODELVIEW)` + `loadIdentity` 直前まで |
| `setup_matrixInit_loadIdentity` | `gGL.loadIdentity()` 1 行 wrap |
| `setup_matrixInit_dirtyRect` | `if (LLView::sDirtyRect) {...}` block 全体 wrap |

総和が 3.56 ms にほぼ一致するか、どれか 1 つに集中するか、で「GL call 単発が重い」vs 「全体が薄く重い (sync stall 候補)」を切り分け。

#### B. variance 再現性確認 (10 周目)

Layer 5 配線 + 計測を **同条件で 1 回追加** (10 周目)。8/9/10 の 3 点で:
- 13.15 / 3.57 / X = X が 13 寄りなら 9 周目が outlier、X が 3.5 寄りなら 8 周目が outlier、中間なら本当に大変動
- どちらにせよ「1 回計測で結論しない」原則を守る

#### C. 計測 overhead 影響の sanity check

10 周目 CSV で renderShadow 等 既存 zone の値が 8/9 周目と整合しているか確認。**もし 9→10 で他 zone も大幅低下していたら計測 infra 自体に session 間ノイズ要因がある**。

---

## 6. ビルド / 計測フロー (前 handoff §5 と同じ、差分のみ)

```bash
# 1. 9 周目 CSV を退避
mv ~/.ayastorm_x64/logs/AYAstorm-perf.csv \
   ~/.ayastorm_x64/logs/AYAstorm-perf-9pass-baseline.csv

# 2. Layer 5 zone 配線 (llviewerwindow.cpp の matrixInit 4 分割) — 新規 .cpp/.h なし
# 3. build (--no-configure)
# 4. install + cache clear
# 5. AYA さんに 10 周目 active session 録音依頼 (8/9 周目と同 SLurl / 同時刻帯)
# 6. CSV 解析 (active session filter 必須)
```

---

## 7. CSV 退避ファイル一覧 (現状)

| 周 | ファイル | 用途 |
|---|---|---|
| 2-6 周目 | `~/.ayastorm_x64/logs/AYAstorm-perf-{2,3,4,5,6}pass-baseline.csv` | Layer 0-1 |
| 7 周目 (Layer 2) | `~/.ayastorm_x64/logs/AYAstorm-perf-7pass-baseline.csv` | doOcclusion 6.17 ms 主犯特定 |
| **8 周目 (Layer 3)** | `~/.ayastorm_x64/logs/AYAstorm-perf-8pass-baseline.csv` | vwDraw_setup 13.15 ms 主犯特定 |
| **9 周目 (Layer 4、current)** | `~/.ayastorm_x64/logs/AYAstorm-perf.csv` | matrixInit 99.7% 占有 (setup 3.57 ms) — **次セッション開始時に退避** |
| 10 周目 (Layer 5、これから) | 同 path 上書き | matrixInit 4 分割 + variance triangulate |

---

## 8. 配線済 zone 全リスト (Layer 3 + Layer 4)

### `indra/newview/pipeline.cpp::LLPipeline::renderShadow()` (line 12459-12667)

Layer 3 (8 周目):
- `renderShadow_body_cull` (line 12496 周辺)
- `renderShadow_body_geom` (line 12570 周辺)
- `renderShadow_body_alpha` (line 12577 周辺)

Layer 4 (9 周目):
- `renderShadow_body_matrixSetup` (line 12506、matrix push/load + struct CompareVertexBuffer dead code + LLVertexBuffer::unbind)
- `renderShadow_body_innerOcclusion` (line 12567、`if (sUseOcclusion > 1) doOcclusion(shadow_cam)`)
- `renderShadow_body_cubeTeardown` (line 12665、gDeferredShadowCubeProgram.bind + matrix pop + sUseOcclusion/sShadowRender restore)

### `indra/newview/llviewerwindow.cpp::LLViewerWindow::draw()` (line 3005-3235)

Layer 3 (8 周目):
- `uiRender_ui2d_vwDraw_setup` (line 3008、外側 wrap)
- `uiRender_ui2d_vwDraw_topCtrl` (line 3207、if block 内)
- `uiRender_ui2d_vwDraw_overlayTitle` (line 3221、if block 内)
- `uiRender_ui2d_vwDraw_teardown` (line 3235、外側 closing brace 後)

Layer 4 (9 周目、setup 内側 5 分割):
- `uiRender_ui2d_vwDraw_setup_stopGlerror` (line 3011)
- `uiRender_ui2d_vwDraw_setup_matrixInit` (line 3019、setLineWidth/matrixMode/loadIdentity/dirtyRect-if)
- `uiRender_ui2d_vwDraw_setup_displayTimecode` (line 3036)
- `uiRender_ui2d_vwDraw_setup_uiProgramBind` (line 3058、gUIProgram.bind + color4f)
- `uiRender_ui2d_vwDraw_setup_pushMatrices` (line 3067、gGL.pushMatrix + LLUI::pushMatrix)

---

## 9. 必読 memory (前 handoff §7 と同じ、+ 追加)

前 handoff §7 全項目 + 以下:

| memory | 追加理由 |
|---|---|
| `feedback_admit_unknown.md` | **本セッションで 2 仮説 falsify、Layer 5 は推論せず実測で進める原則** |
| `feedback_render_full_trace_first.md` | matrixInit 内訳は GLSL/shader でなく GL state call の trace、grep でなく完全 trace |

---

## 10. 既知 risk

1. **変動の原因不明状態で 05 spec (打ち手議論) を始めると、打ち手対象が安定 quantity でない**: matrixInit 3.5 ms 削減と 13 ms 削減では打ち手規模が違う → **Layer 5 で triangulate 後でないと打ち手議論できない**
2. **observer effect の蓄積**: Layer 4 で setup 内に 5 zone 追加、Layer 5 で更に 4 zone 追加 = setup 内 9 zone 累積。total wall time が overhead で水増しされる可能性 → 10 周目で「他 zone (renderShadow 等) の数値が 8/9 周目と乖離していないか」を必ず sanity check
3. **uncommitted diff 14+ file 蓄積継続**: commit 戦略の AYA 相談、次 phase 境界 (打ち手議論開始時) で確実に切る

---

## 11. 引き継ぎ session 開始時の AYA さん発話例

```
docs/specs/ayastorm-cpu-perf/handoff-layer4-to-layer5.md 読んで進めて。
```

または:
```
handoff-layer4-to-layer5.md 読んで、setup variance の triangulate (10 周目) を Layer 5 と同 build で。
```

---

## 12. Open task list (引き継ぎ時に TaskCreate)

- [ ] 9 周目 CSV を `-9pass-baseline.csv` に退避
- [ ] Layer 5 zone 配線 (matrixInit 4 分割、llviewerwindow.cpp line 3019 周辺)
- [ ] 8 周目 + 9 周目 の renderShadow body 内訳 (cull/geom/alpha + matrixSetup/innerOcclusion/cubeTeardown) の数値を 04 spec §4.2.c / §4.2.d に確定追記 — **本セッションで未追記**
- [ ] ビルド + install + cache clear (Claude 連続実行 OK)
- [ ] AYA に 10 周目 active session 録音依頼 (同 SLurl / 同時刻帯)
- [ ] 10 周目 CSV 解析 + 8/9/10 三点比較で variance 原因切り分け
- [ ] 04 spec §4.2.e に Layer 5 結果追記
- [ ] 03 spec §6.9 Group K (Layer 5 catalog) 追記
- [ ] commit 戦略 AYA 相談 (現時点 14+ file uncommitted)
- [ ] `AYAPerfLogEnabled=0` 戻し案内 (05 spec 着手前に必須)
- [ ] (variance 解決後) 05 spec 起こし (matrixInit / doOcclusion 打ち手議論)
