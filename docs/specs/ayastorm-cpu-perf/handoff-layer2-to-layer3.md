# Handoff: Layer 2 → Layer 3 (CPU perf 章)

**作成日**: 2026-05-26
**前セッション ID**: 491aa33c-55fe-48d9-9db2-6eb9785dd899 (本書作成時の継続元)
**目的**: 次セッション Claude が Layer 3 = 8 周目 BFS B drill (or 計測結果に応じた打ち手 phase 移行判断) を即着手できるよう、現在位置 + Layer 2 計測結果 + 次の手を 1 ファイルに集約。

---

## 1. 現在位置 one-liner

> **Layer 2 (7 周目 = renderGeomDeferred gap / sun_3 setup / viewerWindowDraw 内訳 を同時 drill) 完了。renderGeomDeferred は doOcclusion 6.17 ms 主犯で完全分解 ✓ — 打ち手議論移行可。残る Layer 3 候補は (1) renderShadow() body の内側 (sun_3 5.18 ms 突出の真犯人) と (2) LLViewerWindow::draw 関数全域 (13 ms gap、mRootView->draw 外側に逃げている)。**

詳細地図: `docs/specs/ayastorm-cpu-perf/04-observed-hot-path-map.md` §4.2.b (Layer 2 結果表) + §10 (one-liner)。

---

## 2. BFS 方針 (memory 必読)

`feedback_perf_map_bfs_drill.md` を **最初に読むこと**。要旨:
- A = 今の層を全部回る (横展開)
- B = そこから 1 段深く全部回る (深掘り)
- A → B → 更に深く ... を反復、全枝が同じ深さに揃いながら徐々に深くなる
- **層境界 = 安全な引き継ぎポイント** (本書は Layer 2 完成直後 = 境界)

加えて、Layer 2 で得た教訓:
- **`feedback_doubt_self_first.md` / `feedback_admit_unknown.md`**: handoff §3.1 は「mRootView->draw が viewerWindowDraw 18.6 ms の dominant」と仮定したが、実データで falsify (mRootView->draw は 0.70 ms = 5%)。**仮説でなく実測で進める** 。
- **active session filter 必須**: `display_startup()` 経由で LLViewerWindow::draw が login phase 中に親 zone を回避して fire するため、Layer 2 子 zone count は filter なしだと 25x 膨張する。**awk 集計は `frame >= <renderShadow first frame>` で active session 限定**。

---

## 3. Layer 2 計測結果 (7 周目、2026-05-26)

### 3.1 計測条件
- viewer: `feature/r31-bugfix-1-credits` + Group A-H 全配線 (Layer 1 + Layer 2)
- session: AYAPerfLogEnabled=1、AYA さん active session 録音
- CSV: `~/.ayastorm_x64/logs/AYAstorm-perf.csv` (20 MB、586,027 行)
- active session frame range: 32545 - 34251 (renderShadow 出現範囲、count 1709 active frames)
- 退避済: `~/.ayastorm_x64/logs/AYAstorm-perf-7pass-baseline.csv` (本作業前に退避必要 = §5 参照)

### 3.2 主要 zone (active frame 平均、frame >= 32545 filter)

| zone | total (us) | count | per-active-frame |
|---|---|---|---|
| doFrame_total | 92,836,490 | 1709 | 54.32 ms |
| display | 75,137,243 | 1708 | 43.99 ms |
| renderShadow | 28,638,714 | 3410 | 8.40 ms (× 2 call/frame = 16.8 ms/frame 相当) |
| renderShadow_sun (catalog Σ) | 27,370,011 | 9546 | 16.02 ms/frame (4 cascade × 2 main+cube) |
| **renderShadow_sun_call** | 26,400,376 | 9500 | **15.45 ms/frame** ← **renderShadow() body Σ** |
| uiRender_ui2d_viewerWindowDraw | 23,586,121 | 1707 | **13.82 ms/frame** |
| renderGeomDeferred | 12,406,017 | 3407 | 7.27 ms/frame (× 2 = main + HUD) |
| **renderGeomDeferred_doOcclusion** | 10,524,150 | 1704 | **6.17 ms/frame ← 主犯** |
| renderShadow_sun_3 | 8,844,751 | 1707 | 5.18 ms/frame (per-cascade 最重) |
| renderShadow_sun_0 | 10,780,099 | 3066 | 6.31 ms/frame (cube_snapshot で 2x 発火、main-only に正規化すると 3.5 ms/main-frame) |
| renderShadow_sun_2 | 4,414,046 | 1707 | 2.58 ms/frame |
| renderShadow_sun_1 | 3,328,066 | 3066 | 1.95 ms/frame (cube 2x、main-only ~1.0 ms) |
| renderShadow_stateSort | 15,178,274 | 12693 | 8.89 ms/frame |
| renderShadow_dispatch | 3,662,380 | 12693 | 2.14 ms/frame |
| uiRender_ui2d_vwDraw_rootView | 1,196,487 | 1711 | **0.70 ms/frame ← 5% only** |
| uiRender_ui2d_vwDraw_toolAndOverlays | 5,270 | 1711 | 3.1 us/frame (trivial) |
| renderGeomDeferred_setupHWLights | 20,518 | 3407 | 12 us/frame (trivial) |
| renderGeomDeferred_postLoop | 234 | 3407 | 0.14 us/frame (trivial) |
| renderGeomDeferred_prerender | 6,683 | 3407 | 3.9 us/frame (trivial) |

### 3.3 gap 分析 (3 つの drill 結果)

| 親 | parent | child Σ | 残 gap | 結論 |
|---|---|---|---|---|
| renderGeomDeferred | 7.27 ms | 0.012 + 0.97 + 0.0039 + 6.17 + 0.00014 = 7.15 ms | **0.11 ms (1.5%)** | **完全分解 ✓** doOcclusion が単独主犯 |
| renderShadow_sun (Σ) | 16.02 ms | sun_call 15.45 ms | **derived setup = 0.57 ms total** | setup は小さい、突出は body 側 |
| viewerWindowDraw | 13.82 ms | 0.70 + 0.003 = 0.70 ms | **13.12 ms (95%)** | **未分解** mRootView->draw 外側に大量の時間 |

---

## 4. Layer 3 = 8 周目 plan

### 4.1 二択 — Layer 3 続行 or 打ち手 phase 移行 (AYA 判断対象)

**option A: Layer 3 続行** (推奨)
- renderShadow() body 内側 + LLViewerWindow::draw 関数全域に Layer 3 zone 追加
- viewerWindowDraw 13 ms / sun_3 5.18 ms の真犯人を確定
- 打ち手 phase 移行前に「全体 80% 以上が説明済」状態にする

**option B: 部分 phase 移行**
- renderGeomDeferred は完全分解済 → 05 spec で doOcclusion の打ち手議論を **先行開始**
- 並行で残 2 領域 (sun_3 / viewerWindowDraw) は Layer 3 で drill
- メリット: 打ち手議論が早く始まる
- デメリット: 計測 + 打ち手の文脈切替コスト

**Claude 判断**: AYA 指示で決まる。default は A (BFS 厳守) を推奨。memory `feedback_perf_map_bfs_drill.md` の「層境界で揃える」原則に従う。

### 4.2 Layer 3 zone 追加候補 (option A の場合)

#### Group I-1. renderShadow() body 内訳 (sun_3 5.18 ms 真犯人特定)

`pipeline.cpp::LLPipeline::renderShadow(view, proj, shadow_cam, result, depth_clamp)` 関数 (line ~12450 周辺) の内側を drill。
構造調査 (Layer 3 着手前に Read 必須):
- frustum culling block
- pool dispatch loop (per-pool shadow rendering)
- per-draw call overhead (state binding 等)
- shadow_cam transform setup

**配線案 (調査後に確定)**:
| zone 名 (案) | 配線位置ヒント |
|---|---|
| `renderShadow_body_cull` | renderShadow() 関数頭の `pushRenderTypeMask`/`updateCull(shadow_cam, ...)` 周辺 |
| `renderShadow_body_poolLoop` | shadow pool dispatch for-loop wrap |
| `renderShadow_body_perPoolDispatch` | pool 個別の `p->renderShadow(i)` per-pool zone (table-driven、Group G の pool table 流用パターン) |

#### Group I-2. LLViewerWindow::draw 関数全域 (13 ms gap drill)

`indra/newview/llviewerwindow.cpp::LLViewerWindow::draw()` 関数 (line ~3005-3227) の **mRootView->draw 外側** を drill。

| zone 名 (案) | 配線位置ヒント |
|---|---|
| `uiRender_ui2d_vwDraw_setup` | 関数頭 (line 3007-3050、stop_glerror / matrixMode / loadIdentity / sDirtyRect / timecode / gUIProgram.bind / pushMatrix まで) |
| `uiRender_ui2d_vwDraw_topCtrl` | line 3193-3204 (`top_ctrl->draw()` 周辺) |
| `uiRender_ui2d_vwDraw_overlayTitle` | line 3207-3217 (`gShowOverlayTitle` block) |
| `uiRender_ui2d_vwDraw_teardown` | line 3219-3226 (setScaleFactor restore / popMatrix / gUIProgram.unbind / sIsDrawing=false) |

**仮説**: setup / teardown のどちらかに **GL state validation や shader bind overhead** が潜んでいる可能性。または `gUIProgram.bind()` / `gGL.pushMatrix()` が想像以上に重い (LLGLSL の internal state check)。

**注意**: Layer 2 同様、login phase 経路 (display_startup line 209) を親 zone が wrap していない問題は継続。frame >= 32545 filter 必須。または親 zone を line 209 にも追加することで子 count を正規化可能 (本作業の選択肢)。

#### Group I-3 (option): renderGeomDeferred doOcclusion 内訳

renderGeomDeferred は完全分解済だが、**doOcclusion 6.17 ms 自体の内訳**を取りたい場合は `pipeline.cpp::LLPipeline::doOcclusion(LLCamera& camera)` (line ~3165) の内側に Layer 3 zone を追加可能。

| zone 名 (案) | 配線位置ヒント |
|---|---|
| `doOcclusion_probeMgr` | mReflectionMapManager.doOcclusion() / mHeroProbeManager.doOcclusion() wrap (line ~3185-3206) |
| `doOcclusion_groupLoop` | per-group occlusion query result fetch loop wrap (line ~3238 周辺) |

ただし **05 spec 打ち手議論を先行する場合は不要**。doOcclusion の打ち手は「occlusion query を frame skip する / GPU side で遅延 fetch する / sub-sampling する」等 — 内訳取らなくても打ち手議論は可能。

### 4.3 配線時の確認手順 (汎用)

1. 必ず grep で line 番号再確認 (本書の line 番号は古くなる可能性):
   ```
   Grep: pattern="renderShadow|pushRenderTypeMask" path="indra/newview/pipeline.cpp" -n
   Grep: pattern="LLViewerWindow::draw|gUIProgram\.bind|popMatrix" path="indra/newview/llviewerwindow.cpp" -n
   ```
2. AYAPERF_ZONE は string literal 専用。動的 name は `LLAyastormPerfZone _aya_xxx_zone(const char*)` 直接 instantiate (lifetime は static const char* 必須)。
3. **新規 .cpp/.h 追加なし** → configure 不要、`--no-configure` のみ。
4. **scope の変数 lifetime に注意**: Layer 2 で renderShadow setup は変数 scope (`shadow_cam` 等) の都合で setup 直接 wrap を回避し `renderShadow_sun_call` (呼出側 wrap) で代替 = setup を **derived 量** とした。同様の判断が必要な箇所が出る可能性。

### 4.4 03 spec 更新義務

Layer 3 配線完了したら **`03-perf-log-infra.md` §6.7 Group I** として catalog 追記:
- §6.6 (Group H) と同じフォーマット
- §9 file 一覧の `pipeline.cpp` / `llviewerwindow.cpp` 行を更新

---

## 5. ビルド / 起動 / 計測フロー

memory `project_build_procedure.md` 完全準拠。要点だけ:

```bash
cd ~/work_firestorm/phoenix-firestorm
source .venv/bin/activate
export AUTOBUILD_VARIABLES_FILE=$HOME/work_firestorm/fs-build-variables/variables

# 1. configure (新規 .cpp/.h 追加なしならスキップ可)
# 2. build
autobuild build -A 64 -c ReleaseFS_open --no-configure

# 3. install
cd build-linux-x86_64/newview/packaged
rm -rf ~/ayastorm/
rm -f ~/.local/share/applications/ayastorm-viewer.desktop
./install.sh
rm -rf ~/.ayastorm_x64/cache/

# 4. 計測前: 7 周目 CSV を退避
mv ~/.ayastorm_x64/logs/AYAstorm-perf.csv \
   ~/.ayastorm_x64/logs/AYAstorm-perf-7pass-baseline.csv

# 5. AYA さんに起動依頼 → 同条件 active session 録音 → 終了報告
# 6. CSV 解析 (active session filter 必須):
awk -F, 'NR>1 && $1>=<renderShadow_first_frame> {c[$3]++; s[$3]+=$4} END {
  for (z in c) printf "%-50s %8d %14.0f %12.2f\n", z, c[z], s[z], s[z]/c[z]
}' ~/.ayastorm_x64/logs/AYAstorm-perf.csv | sort -k3 -n -r | head -90
```

ビルド / 起動 / 録音は AYA さん側 (memory `feedback_build.md` で configure / build / install / cache clear は Claude 連続実行 OK)。

---

## 6. CSV 退避ファイル一覧 (時系列比較用)

| 周 | ファイル | 状態 |
|---|---|---|
| 2 周目 | `~/.ayastorm_x64/logs/AYAstorm-perf-2pass-baseline.csv` | 59 MB |
| 3 周目 | `~/.ayastorm_x64/logs/AYAstorm-perf-3pass-baseline.csv` | 9.5 MB |
| 4 周目 | `~/.ayastorm_x64/logs/AYAstorm-perf-4pass-baseline.csv` | 8.2 MB (scope bug 含む) |
| 5 周目 | `~/.ayastorm_x64/logs/AYAstorm-perf-5pass-baseline.csv` | 7.8 MB (scope 修正済) |
| 6 周目 | `~/.ayastorm_x64/logs/AYAstorm-perf-6pass-baseline.csv` | 9.2 MB (Layer 1 完成) |
| **7 周目 (current, Layer 2 完成)** | `~/.ayastorm_x64/logs/AYAstorm-perf.csv` | **20 MB、本 handoff の根拠データ — 次セッションでは `-7pass-baseline.csv` に退避** |
| 8 周目 (これから取る) | 同上 path (上書き) → 計測前に 7 周目を `-7pass-baseline.csv` に退避すること | |

---

## 7. 必読 memory (Layer 3 着手前にロード推奨)

| memory | なぜ |
|---|---|
| `feedback_perf_map_bfs_drill.md` | A/B 用語定義、層境界引き継ぎ原則 (本作業の支柱) |
| `feedback_proactive_handoff.md` | **新規 (本セッション)** context 圧迫時は Claude 側から引き継ぎ提案 |
| `feedback_doubt_self_first.md` | Layer 2 で適用済 — 「mRootView->draw が dominant」仮説が falsify された、推論より実測 |
| `feedback_admit_unknown.md` | viewerWindowDraw の 13 ms gap の出所は現時点で不明 — 「分からない」と認めて Layer 3 で実データ取得 |
| `feedback_use_agents_proactively.md` | renderShadow() の構造把握は Explore agent (medium-thorough) 推奨 (関数が長い) |
| `feedback_no_auto_commit.md` | Layer 3 完成後も AYA 明示指示まで commit しない |
| `feedback_restore_debug_settings.md` | 05 spec 着手前に `AYAPerfLogEnabled=0` 戻し案内 |
| `feedback_build.md` | configure→build→install→cache clear 連続実行権限 |
| `feedback_one_step_at_a_time.md` | 計測フローは AYA さんに 1 step ずつ |
| `feedback_log_reading.md` | CSV 解析は Claude が直接 awk (active session filter 必須) |
| `project_build_procedure.md` | 完全フロー (venv activate + AUTOBUILD_VARIABLES_FILE 必須) |

---

## 8. 既知 risk / 注意

1. **active session filter の徹底**: login phase で display_startup() 経由の zone firing が混ざる。frame >= <renderShadow first frame> での filter を **必ず適用**。
2. **観測者効果**: 7 周目時点で 1709 active frame / 約 82 秒。Layer 3 で +6-10 zone 追加なら FPS 影響は小さい予想だが、もし 15 FPS 切ったら overhead 警告。
3. **renderShadow() 関数編集**: pipeline.cpp の中で巨大関数の一つ。Read offset 指定で必要箇所だけ読む、または Explore agent で構造把握。
4. **viewerWindowDraw 13 ms gap の真犯人不明**: 推論で結論せず、**setup / topCtrl / overlayTitle / teardown の 4 zone を全部追加**して実データで切り分ける (memory `feedback_perf_map_bfs_drill.md` — 「層境界で揃える」)。
5. **uncommitted diff の蓄積**: 本作業時点で perf log infra 関連で 14 file 触っている (llviewerwindow.cpp が 7 周目で追加)。Layer 3 で更に同じ 2 file (pipeline.cpp / llviewerwindow.cpp) 編集。AYA 明示指示で commit 段に入る時、まとめ方を相談 (memory `feedback_release_branch_workflow.md`)。

---

## 9. 引き継ぎ session 開始時の AYA さん発話例

最小:
```
docs/specs/ayastorm-cpu-perf/handoff-layer2-to-layer3.md 読んで Layer 3 進めて。
```

または phase 移行を指定する場合:
```
handoff-layer2-to-layer3.md 読んで、Layer 3 続行 (option A) か doOcclusion 打ち手先行 (option B) か判断材料くれ。
```

---

## 10. Open task list (引き継ぎ時に TaskCreate しなおすこと)

前セッション最終時点の未完 task:
- [ ] AYA さんに Layer 3 続行 (option A) or 打ち手 phase 先行 (option B) の判断仰ぐ
- [ ] (option A) Layer 3 zone 配線 (Group I-1 renderShadow body + Group I-2 LLViewerWindow::draw 関数全域)
- [ ] (option A) 8 周目 ビルド + install + cache clear
- [ ] (option A) AYA に 8 周目 active session 録音依頼 + CSV 解析 (active filter 必須)
- [ ] (option A) 04 spec を Layer 3 結果で更新
- [ ] (option A) 03 spec §6.7 Group I 追記
- [ ] (option B) 05 spec 起こし (doOcclusion 打ち手議論)
- [ ] (option B) `AYAPerfLogEnabled=0` 戻し案内 (05 spec 着手前に必須)
- [ ] commit 戦略 AYA と相談 (現時点 14 file uncommitted)
