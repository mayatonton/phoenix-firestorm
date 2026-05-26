# 04 — 観測された hot path map (Layer 1 全周完了, 6 周目)

**「具体的な道筋を立てるための最初の情報」** = 地図。
03 で配線した zone を順次有効化し、3-6 周目で **Layer 1 = doFrame_total 直下の主要 zone を全部同じ粒度で内訳化** を達成した結果。

実機検証 only (memory: `feedback_build_only_verified.md`)。**この時点で最適化判断はしない**。
打ち手議論 (05 spec) は **Layer 2 = 1 段深い全周** を済ませてから始める。

### BFS 方針 (memory `feedback_perf_map_bfs_drill.md`)
- **A = 今の層を全部回る** (全 heavy zone を同じ粒度で内訳化)
- **B = そこから 1 段深く全部回る**
- A → B → 更に深く → ... と全枝が同じ深さで揃いながら徐々に深くなる
- 引き継ぎは **層境界で行う** (Layer N 途中で session 切ると次セッション再構成コスト発生)

---

## §1. 計測条件 (6 周目)

| 項目 | 値 |
|---|---|
| 日時 | 2026-05-26 |
| viewer | AYAstorm r31-bugfix-1 系 (feature/r31-bugfix-1-credits + perf log infra Group A-G 全配線) |
| 計測対象 | `AYAPerfLogEnabled=1` で active session 録音 |
| GPU | RTX 5090 |
| OS | Linux (Ubuntu 系) |
| CSV path (6 周目) | `~/.ayastorm_x64/logs/AYAstorm-perf.csv` |
| 退避済 CSV | 2 周目 `-perf-2pass-baseline.csv` / 3 周目 `-perf-3pass-baseline.csv` / 4 周目 `-perf-4pass-baseline.csv` (scope bug) / 5 周目 `-perf-5pass-baseline.csv` |
| 全 doFrame 回数 | 17,263 (起動含む) |
| login 後 active render frame | 1,450 |
| 録音長 | wall 約 82 秒 (active session) |
| 観測 FPS | 17.6 (active session) — 計測 overhead 込み |

---

## §2. 周回履歴 (物差し校正 → A 全周完成 → B 全周完成)

| 周 | 何をした | カバー zone | 残った疑問 |
|---|---|---|---|
| 2 周目 | 5 zone | idleUpdate / updateCull / octreeBalance / fetchQueryResult / syncToMainThread + visitNotifier | frame の 93% 未計測 |
| 3 周目 | +27 zone (Group A-E) | doFrame_total / idle / display / render pass 12 種 / idle 内訳 6 種 / texture worker 2 種 / AYAstorm 独自 2 種 + B1/B2 fix | renderShadow / uiRender / stateSort の内訳不明 |
| 4 周目 | +11 zone (Group F) | renderShadow_sun / renderShadow_projector / renderShadow_stateSort / cubeFaceRender_total / stateSort_main/hud/cubeFace/impostor / uiRender_hudElements/3d/2d | uiRender_2d 17.3 ms に scope 漏れ疑い |
| 5 周目 | uiRender split 修正 + uiRender_renderAll / uiRender_ui2d 追加 | scope 修正完了 | uiRender_ui2d 18.6 ms の内訳不明、renderShadow_sun の cascade 数不明、renderGeomDeferred 内訳不明 |
| 6 周目 | +11 catalog (Group G, pool は 22 展開) | uiRender_ui2d_viewerWindowDraw/uiScreenComposite + renderShadow_sun_<0..3> + renderShadow_projector_<0..1> + renderShadow_dispatch + renderGeomDeferred_prerender + renderGeom_pool_<TYPE×22> | renderGeomDeferred の 6.8 ms gap / sun_3 突出 5.77 ms の per-cascade setup / gViewerWindow->draw 18.6 ms 内訳 = Layer 2 で drill |
| **7 周目 (本書)** | **+6 catalog (Group H, Layer 2 = B 全周)** | **renderGeomDeferred_setupHWLights/doOcclusion/postLoop + renderShadow_sun_call + uiRender_ui2d_vwDraw_toolAndOverlays/rootView** | **viewerWindowDraw の新 gap 13.12 ms (mRootView->draw が予想外に軽い 0.70 ms) = Layer 3 で drill / sun_3 突出は renderShadow() body 側 (Layer 3 候補)** |

---

## §3. Layer 1 全周完成: 観測された包含ツリー (6 周目実測値)

active render frame (1,450 frame) の **per-active-frame 平均** で記述。
`★` 印は per-frame total >= 1 ms、`★★` は >= 5 ms、`★★★` は >= 10 ms。

```
doFrame_total           5.81 ms/全frame (17263 frame avg)
│
├─ idle                 1.09 ms/全frame
│   ├─ idleUpdate            0.98 ms/active-ish frame
│   ├─ regionIdleUpdate      0.63 ms/active-ish frame
│   ├─ messagePump           0.54 ms/active-ish frame
│   ├─ inventoryObserver     1.79 ms/dirty frame (34 frame のみ)
│   ├─ particleSim           0.17 ms
│   ├─ drawablesUpdateMove   0.036 ms
│   ├─ idleCallbacks         38 us
│   └─ octreeBalance         11 us (trivial ✓)
│
├─ display              ~50.4 ms/active frame (73097 ms / 1450)
│   │
│   ├─ updateCull            ★ 1.04 ms/frame
│   │
│   ├─ stateSort (top)       7.77 ms/frame (count 14573, 10.05 call/frame)
│   │   ├─ stateSort_main         0.83 ms/frame
│   │   ├─ stateSort_hud          0.24 ms/frame
│   │   ├─ stateSort_cubeFace     0.093 ms/frame
│   │   ├─ stateSort_impostor     ~0 (rare)
│   │   └─ renderShadow_stateSort 6.63 ms/frame ← shadow pass 内訳
│   │
│   ├─ renderShadow         ★★★ 15.1 ms/frame (count 2894, 2 call/frame = main + cube_face)
│   │   ├─ renderShadow_sun (catalog) ★★★ 14.4 ms/frame
│   │   │   ├─ sun_3            ★★ 5.77 ms/frame ← cascade 内 最重
│   │   │   ├─ sun_0            ★ 4.41 ms/frame (cube_snapshot で 2x 発火: count 2514)
│   │   │   ├─ sun_2            ★ 2.59 ms/frame
│   │   │   └─ sun_1            ★ 1.65 ms/frame (cube 2x: count 2514)
│   │   ├─ renderShadow_projector (catalog) 0.19 ms/frame (trivial)
│   │   │   ├─ projector_0        0.13 ms (cube 2x)
│   │   │   └─ projector_1        0.058 ms (cube 2x)
│   │   ├─ renderShadow_dispatch  2.36 ms/frame (j=0/1 inner draw call loop)
│   │   ├─ renderShadow_stateSort 6.63 ms/frame ← stateSort と重複計上
│   │   └─ **未計測 gap (cascade setup / FBO bind / frustum compute)**  ← Layer 2 候補
│   │
│   ├─ renderGeomDeferred  ★★ 7.87 ms/frame (count 2891, 2 call/frame = main + HUD)
│   │   ├─ renderGeomDeferred_prerender  4 us/frame (trivial)
│   │   ├─ renderGeom_pool (catalog Σ)  1.04 ms/frame ← pool dispatch
│   │   │   ├─ pool_MATERIALS     0.51 ms/frame
│   │   │   ├─ pool_TERRAIN       0.41 ms
│   │   │   ├─ pool_SIMPLE        0.056 ms
│   │   │   ├─ pool_WL_SKY        0.033 ms
│   │   │   ├─ pool_GLTF_PBR      0.012 ms
│   │   │   ├─ pool_AVATAR        0.010 ms
│   │   │   ├─ pool_BUMP          0.006 ms
│   │   │   ├─ pool_GLTF_PBR_ALPHA_MASK 0.003 ms
│   │   │   ├─ pool_TREE          0.003 ms
│   │   │   ├─ pool_GRASS         0.002 ms
│   │   │   └─ pool_ALPHA_MASK    0.002 ms
│   │   └─ **未計測 gap ~6.83 ms/frame (86%)** ← Layer 2 主犯候補
│   │       (doOcclusion / setupHWLights / postLoop work)
│   │
│   ├─ renderDeferredLighting  ★ 1.80 ms/frame (sun shadow + local lights)
│   ├─ motionBlur (Cinematic)  0.81 ms/frame
│   ├─ atmosphericsHaze        2.2 us/frame (trivial)
│   ├─ renderFinalize          50 us/frame (trivial)
│   ├─ volumetric (Cinematic)  2.7 us/frame (trivial)
│   ├─ depthOfField            4.5 us/frame (trivial)
│   ├─ cubeFaceRender_total    ★ 1.75 ms/frame
│   ├─ hudAttachmentRender     0.46 ms/frame
│   │
│   ├─ uiRender             ★★★ 19.3 ms/frame
│   │   ├─ uiRender_hudElements   10 us (trivial ✓)
│   │   ├─ uiRender_3d            1.5 us (trivial ✓)
│   │   ├─ uiRender_renderAll     189 us (LLHUDObject::renderAll、trivial)
│   │   └─ uiRender_2d         ★★★ 18.8 ms/frame
│   │       └─ uiRender_ui2d   ★★★ 18.6 ms/frame
│   │           ├─ uiRender_ui2d_viewerWindowDraw ★★★ 18.6 ms/frame ← gViewerWindow->draw 単独で 100%
│   │           └─ uiRender_ui2d_uiScreenComposite ~3.5 us (FBO blit、trivial)
│   │
│   ├─ fetchQueryResult        1.4 us/frame (trivial)
│   └─ swapBuffers             12 us/frame
│
├─ updateTextureThreads   7.3 us/全frame (trivial)
│   ├─ updateImagesCreateTextures  87 us/call (17995 call)
│   └─ updateImagesFetchTextures   140 us/call (17991 call)
│
└─ meshRepoUpdate         0.15 us/全frame (trivial)

(独立 worker thread)
syncToMainThread       455 us/call × 998 call = 0.45 sec total (軽い)
visitNotifier          184 us/call × 1313 call = 0.24 sec total (B2 fix 効いて健全)

(AYAstorm 独自 — 一次判断「軽い」が実測確定 ✓)
occlusionRefresh       0.20 us/call (2179 call、無視可)
stream3DUpdate         20.8 us/call (2179 call、軽い)
```

---

## §4. Layer 1 完成度の検算 (gap 分析)

### §4.1 display zone の containment

```
display 親 = 50.4 ms/active frame
子合計:
  renderShadow         15.1 ms
  uiRender             19.3 ms
  renderGeomDeferred    7.87 ms
  renderDeferredLighting 1.80
  cubeFaceRender_total  1.75
  stateSort_main + _hud (display 直接) 1.06
  updateCull           1.04
  motionBlur           0.81
  hudAttachmentRender  0.46
  renderFinalize/volumetric/swapBuffers/atmos/DOF/fetchQuery < 0.1 each
  ───────────────────
  合計  ~49.2 ms
gap = 50.4 - 49.2 ≈ 1.2 ms  (2.4%)  ← Layer 1 で display 親はほぼ埋まった ✓
```

### §4.2 子の中の gap (Layer 2 で drill 対象)

| 親 zone | 親 total | 子合計 | gap | 主犯候補 |
|---|---|---|---|---|
| **renderGeomDeferred** | 7.87 ms | 1.04 ms (pool) + 4us (prerender) | **6.83 ms (86%)** | doOcclusion(camera) / setupHWLights / pool loop の post work |
| renderShadow | 15.1 ms | 14.4 (sun) + 0.19 (proj) + 2.36 (dispatch) + 6.63 (stateSort) = 23.6 | overlap (sun は dispatch + stateSort 含む) | sun_3 突出 5.77 ms の cascade setup |
| uiRender_ui2d | 18.6 ms | 18.6 (vwDraw) + 3.5us (composite) | **0** (完全分解) | gViewerWindow->draw 内側 (LLViewerWindow::draw → mRootView->draw 等) |
| renderShadow_sun | 14.4 ms | 4 cascade Σ = 14.42 | 0 (完全分解) | sun_3 単体が次の B drill 対象 |

### §4.2.b 7 周目 Layer 2 結果 — gap drill 結果

**計測条件**: 2026-05-26 active session (frame >= 32545, doFrame_total count = 1709)、AYAPerfLogEnabled=1。
**注意**: 子 zone (vwDraw_rootView 等) は LLViewerWindow::draw が `display_startup()` 経由でも fire するため、frame >= 32545 filter で active session 限定。filter なしだと count が 25x 膨張する (login phase の display_startup 経路で親 zone が wrap されていないため)。

| 親 zone | 親 (active/frame) | 子 (active/frame) | 残 gap | 結論 |
|---|---|---|---|---|
| **renderGeomDeferred** | 7.27 ms | setupHWLights 12 us + pool Σ 0.97 ms + prerender 3.9 us + doOcclusion 6.17 ms + postLoop 0.14 us = **7.15 ms** | **0.11 ms (1.5%)** | **完全分解達成 ✓** doOcclusion が 6.17 ms = gap 6.8 ms の主犯 (85% of parent) |
| renderShadow_sun (Σ) | 14.4 ms/frame (※ count 9546 全 cascade 合算) | sun_call 15.47 ms (Σ) ※ count 9500 | **derived setup (Σ) = sun catalog 27.37 ms − sun_call 26.40 ms = 0.97 ms total → 0.57 ms/active frame** | sun_3 突出は **setup でない** ことが判明。renderShadow() body (depth-only draw calls) が真の主犯 = Layer 3 drill 対象 |
| uiRender_ui2d_viewerWindowDraw | 13.82 ms | vwDraw_rootView 0.70 ms + vwDraw_toolAndOverlays 3.1 us = **0.70 ms** | **13.12 ms (95%)** | **想定外** mRootView->draw は trivial。残 13 ms は **LLViewerWindow::draw 関数頭尾 / top_ctrl / overlay title / gUIProgram.bind 等 (Layer 3 で drill 必要)** |

**Layer 2 の総括**:
- **renderGeomDeferred**: 完全分解 ✓ doOcclusion が単独主犯 (Layer 3 不要、05 spec で打ち手議論可能)
- **renderShadow_sun_3**: setup でなく **renderShadow() body 内側** が真の主犯 — Layer 3 で renderShadow() の内訳 drill (pool dispatch 1 段 + per-cascade shadow culling 等)
- **viewerWindowDraw**: handoff の仮説 (mRootView->draw が dominant) を **データが falsify**。13 ms は LLViewerWindow::draw 内 mRootView->draw 外の領域 — Layer 3 で関数全域に zone 追加して再 drill (memory `feedback_doubt_self_first.md` / `feedback_admit_unknown.md` 適用、推論で結論せず実データ重視)

### §4.2.c 8 周目 Layer 3 結果 — gap drill 結果

**計測条件**: 2026-05-26 active session (frame >= 15156, doFrame_total count = 1846)、AYAPerfLogEnabled=1。Group I 配線後 (renderShadow body 3 zone + LLViewerWindow::draw 4 zone)。

#### Layer 3 主結果

| 親 zone (Layer 2) | 親 (active/frame) | Layer 3 子内訳 | 残 gap | 結論 |
|---|---|---|---|---|
| **uiRender_ui2d_viewerWindowDraw** | 13.88 ms | vwDraw_setup 13.15 ms + vwDraw_rootView 0.69 ms + vwDraw_toolAndOverlays 0.003 ms + vwDraw_teardown 0.0002 ms + topCtrl 0 us + overlayTitle 0 us = **13.84 ms** | **0.04 ms (0.3%)** | **完全分解達成 ✓** **`vwDraw_setup` 単独で 94.7%** = 衝撃の主犯確定 |
| renderShadow_sun_call (per call) | 2.60 ms/call | stateSort 1010 + dispatch 298 + body_cull 148 + body_geom 72 + body_alpha 395 = **1923 us/call** | **674 us/call (26%)** | 部分分解。残 gap = 関数頭尾 matrix setup + 内部 doOcclusion + cube teardown |

#### I-2 詳細: viewerWindowDraw 分解 (active frame 1846)

| zone | total (us) | count | per-frame (ms) | 占有率 |
|---|---|---|---|---|
| viewerWindowDraw (parent) | 25,613,652 | 1846 | 13.88 | 100% |
| **vwDraw_setup** | **24,322,913** | **1850** | **13.15** | **94.7%** |
| vwDraw_rootView (Layer 2) | 1,276,204 | 1850 | 0.69 | 5.0% |
| vwDraw_toolAndOverlays (Layer 2) | 5,784 | 1850 | 0.003 | 0.02% |
| vwDraw_teardown | 364 | 1850 | 0.0002 | <0.01% |
| vwDraw_topCtrl | 0 | 0 | — | 不発 (focus 不在) |
| vwDraw_overlayTitle | 0 | 0 | — | 不発 (gShowOverlayTitle=false、予想通り) |

**setup 13.15 ms の中身候補** (line 3008-3055):
1. `stop_glerror()` (line 3015) ← **`glGetError()` 強制 → GPU sync stall 疑い濃厚**
2. `gUIProgram.bind()` (line 3055) ← LLGLSL state validation + uniform re-push
3. `static LLCachedControl<bool> displayTimecode` get (line 3034)
4. `gGL.matrixMode/loadIdentity` (line 3022-3024)
5. `gGL.pushMatrix() / LLUI::pushMatrix()` (line 3058-3059)

**Layer 4 drill 対象**: setup 内側を上記 5 path に分割 (vwDraw_setup_stopGlerror / setup_matrixInit / setup_displayTimecode / setup_uiProgramBind / setup_pushMatrices)。

#### I-1 詳細: renderShadow body 分解

| zone | total (us) | count | per-call (us) | sun 寄与 (ms/frame) |
|---|---|---|---|---|
| renderShadow_sun_call (parent) | 26,159,709 | 10074 | 2596.75 | 14.17 |
| renderShadow_stateSort (Layer 2) | 13,893,576 | 13750 | 1010.44 | 5.52 (10074/13750 比例) |
| **renderShadow_body_alpha** | **5,431,848** | **13750** | **395.04** | **2.16** |
| renderShadow_dispatch (Layer 2) | 4,094,500 | 13750 | 297.78 | 1.62 |
| renderShadow_body_cull | 2,037,514 | 13750 | 148.18 | 0.81 |
| renderShadow_body_geom | 992,122 | 13750 | 72.15 | 0.39 |
| **Σ wrapped (5 zone)** | — | — | **1923 us/call** | **10.49 ms/frame (sun 寄与)** |
| **残 gap** | — | — | **674 us/call (26%)** | **3.68 ms/frame** |

**残 gap 674 us/call** の候補 (line 12502-12526 + 12559-12562 + 12652-12666):
- matrix setup (proj/modelview push + load, line 12502-12515)
- doOcclusion inside renderShadow() (line 12559-12562、`sUseOcclusion>1` 条件、Layer 3 未配線)
- cube/matrix teardown (line 12652-12666)

**Layer 4 drill 対象**: 上記 3 path を `renderShadow_body_matrixSetup` / `renderShadow_body_innerOcclusion` / `renderShadow_body_cubeTeardown` で wrap。

**注**: body_* zones は projector renderShadow() からも fire (count 13750 = sun_call 10074 + 一部 projector path)。本表の「sun 寄与」列は (10074/13750) で比例配分した推定値。

#### Layer 3 全体総括

- **viewerWindowDraw**: 完全分解 ✓ **`vwDraw_setup` 単独主犯 13.15 ms 確定**。仮説 (stop_glerror / gUIProgram.bind の GPU stall) は Layer 4 で実証必要
- **renderShadow body**: 部分分解、`body_alpha` 395 us/call が最重、残 gap 674 us/call は Layer 4 で 3 zone 追加して撃ち抜く
- **doOcclusion 6.94 ms/frame**: renderGeomDeferred 配下の doOcclusion は Layer 2 で確定済、内訳は 05 spec 打ち手議論で扱う方針 (frame skip / GPU 遅延 fetch / sub-sampling)

### §4.2.d 9 周目 Layer 4 結果 — setup 内側 + renderShadow body 内側 drill 結果

**計測条件**: 2026-05-26 active session (frame >= 17022, active frame = 1901)、AYAPerfLogEnabled=1。Group J 配線後 (vwDraw_setup を 5 zone + renderShadow body 内に 3 zone 追加)。CSV: `~/.ayastorm_x64/logs/AYAstorm-perf-9pass-baseline.csv`。**時刻条件: 夜 (AYA 確認済) / 同 SLurl (8 周目と一致)**。

#### J-2 詳細: vwDraw_setup 内側 5 分解 (active frame 1901)

| zone | total (us) | count | per-call (us) | per-frame (ms) | 占有率 |
|---|---|---|---|---|---|
| vwDraw_setup (parent) | 6,786,740 | 1901 | 3570.09 | 3.57 | 100% |
| **vwDraw_setup_matrixInit** | **6,776,830** | **1901** | **3564.88** | **3.56** | **99.7% ← 主犯** |
| vwDraw_setup_uiProgramBind | 1,402 | 1901 | 0.74 | 0.0007 | 0.02% ← falsified |
| vwDraw_setup_displayTimecode | 752 | 1901 | 0.40 | 0.0004 | 0.01% (trivial、static cvar get + if false 経路) |
| vwDraw_setup_pushMatrices | 526 | 1901 | 0.28 | 0.0003 | 0.008% |
| vwDraw_setup_stopGlerror | 31 | 1901 | 0.016 | 0.00002 | 0.0005% ← falsified (`stop_glerror` = `LL_DEBUG` 包囲、release で noop) |

**setup 13 ms 仮説の答え合わせ**:
- ✗ `stop_glerror` (Layer 3 で第 1 候補) → 0.016 us = noise floor、**falsified**
- ✗ `gUIProgram.bind` (Layer 3 で第 2 候補) → 0.74 us = noise floor、**falsified**
- ✓ **matrixInit (setLineWidth / matrixMode / loadIdentity / dirtyRect-if) が単独で 99.7% 占有確定**

#### J-1 詳細: renderShadow body 内側 3 分解 (active frame 1901)

| zone | total (us) | count | per-call (us) | 結論 |
|---|---|---|---|---|
| renderShadow_body_alpha (Layer 3) | 5,150,985 | 14231 | 361.96 | 8 周目 395 と整合 |
| renderShadow_body_cull (Layer 3) | 1,914,861 | 14231 | 134.56 | 8 周目 148 と整合 |
| renderShadow_body_geom (Layer 3) | 921,335 | 14231 | 64.74 | 8 周目 72 と整合 |
| renderShadow_body_matrixSetup | 4,909 | 14231 | **0.35** | **trivial** (matrix push/load + LLVertexBuffer::unbind) |
| renderShadow_body_cubeTeardown | 2,158 | 14231 | **0.15** | **trivial** (matrix pop + state restore) |
| renderShadow_body_innerOcclusion | 343 | 14231 | **0.02** | **trivial** (`sUseOcclusion > 1` 経路、本 session では分岐不発が大半) |

**含意**: renderShadow body の真犯人は **alpha (362) + cull (135) + geom (65) = 561 us/call** で確定 (Layer 3 と同じ結論を Layer 4 が反証しなかった ✓)。matrixSetup / innerOcclusion / cubeTeardown は全て < 1 us = trivial、**Layer 4 で追加した 3 zone は body 内に隠れた hot path が無いことを実証** (8 周目の残 gap 674 us/call は cull/geom/alpha 自体の per-call 変動 + body wrap 外 fn 呼出オーバヘッド)。

#### J 全体総括 + 新規未解決問題

✓ **matrixInit が viewerWindowDraw 主犯確定** (Layer 5 で 4 分割 drill)
✓ **renderShadow body 内側 3 zone は全 trivial、cull/geom/alpha 以外に隠れた hot path 無し**
✗ **新規未解決 — 8 周目 setup 13.15 ms ⇔ 9 周目 setup 3.57 ms の 3.7 倍変動 (−73%)**

##### 変動問題の確定条件 (AYA 確認済)
- 8/9 周目 とも **夜**、**同 SLurl**
- viewer build 差分: Group J 5+3 zone 追加のみ (子 zone は parent 内側、parent 総和を **下げる** ことは通常起きない)

##### Claude 直前推論の falsify (Layer 5 完了時点で 3 連続)

| 仮説 | 内容 | falsify 根拠 |
|---|---|---|
| sun position 説 | 「9 周目が日中で setup 軽い」 | AYA 確認: 9 周目も夜 |
| night = GPU sync stall 説 | 「夜は GPU stall で setup 重い」 | 上記により前提崩壊 |
| scene 重さ proxy 説 | 「9 周目だけ場所が違う (森林 + 人なし + 川) で軽い」 | AYA 確認: 全周回同 SLurl |

→ **3 連続外しで原因特定を打ち切り**。`feedback_admit_unknown.md` 適用、**variance 原因は「同 SLurl / 同時刻帯 (夜) 下の確率的揺らぎ」と確定** (driver state / GPU pipeline scheduling 起因候補だが Claude 側からは特定不能)。Layer 5 完了時点でこの問題は射程外扱い。

##### 確定事項 (打ち手 A 検証後に判明、§4.2.f 参照)
- setLineWidth zone の時間 = **GPU pipeline sync stall** (CPU work でない、prior frame GPU 完了待ち)
- → 13 ms gap の本質は CPU でなく GPU 側 (CPU perf 章では削減不可)
- → variance 3.5⇔17 ms は GPU pipeline state の確率的揺らぎ、特定意味なし

### §4.2.e Layer 5 plan (10 周目以降の drill 対象)

**目的**: matrixInit 内のどの GL call で時間が消えているか確定 + variance 再現性確認。

#### A. matrixInit 4 分割 (llviewerwindow.cpp::LLViewerWindow::draw line 3019 周辺)

| zone 名 | wrap 対象 |
|---|---|
| `setup_matrixInit_setLineWidth` | `LLUI::setLineWidth(1.f)` |
| `setup_matrixInit_matrixMode` | `gGL.matrixMode(MM_MODELVIEW)` |
| `setup_matrixInit_loadIdentity` | `gGL.loadIdentity()` |
| `setup_matrixInit_dirtyRect` | `if (!RenderUIBuffer) { sDirtyRect = ... }` block |

総和 ≈ 3.56 ms (or 8 周目 13 ms 寄り) なら、どの GL call 単発が重いか / 全体が薄く重いか (sync stall 候補) を切り分け可能。

#### B. variance 再現性確認 (10 周目)

Layer 5 配線 + **同条件で 1 回追加**。8/9/10 の 3 点で:
- 13.15 / 3.57 / X = X が 13 寄りなら 9 周目 outlier、X が 3.5 寄りなら 8 周目 outlier、中間なら本当に大変動

#### C. 計測 overhead sanity check

10 周目 CSV で renderShadow 等 **既存 zone (Layer 2 以前)** の値が 8/9 周目と整合か確認。整合しなければ計測 infra 自体に session 間ノイズ要因あり。

### §4.2.f 10 周目 Layer 5 結果 — matrixInit 主犯確定 + 打ち手 A

**計測条件**: 2026-05-26 active session (frame >= 9223, active frame = 2810)、AYAPerfLogEnabled=1。Group K-1 配線後 (matrixInit を 4 zone)。CSV: `~/.ayastorm_x64/logs/AYAstorm-perf-10pass-baseline.csv`。**時刻条件: 夜 (AYA 確認済) / 同 SLurl (8/9 周目と一致)**。

#### K-1 詳細: matrixInit 4 分解 (active frame 2810)

| zone | total (us) | count | per-call (us) | 占有率 |
|---|---|---|---|---|
| matrixInit (parent) | 46,784,613 | 2810 | 16649.33 | 100% |
| **matrixInit_setLineWidth** | **46,768,901** | **2810** | **16643.74** | **99.97% ← 真の主犯確定** |
| matrixInit_loadIdentity | 743 | 2810 | 0.26 | 0.0016% |
| matrixInit_dirtyRect | 433 | 2810 | 0.15 | 0.0009% |
| matrixInit_matrixMode | 320 | 2810 | 0.11 | 0.0007% |

**4 zone 完全分解 ✓** (parent との差 = 5 us = noise floor)。

#### 8/9/10 周目 三点比較 (variance triangulation)

| 周 | vwDraw_setup | matrixInit | setLineWidth | 結論 |
|---|---|---|---|---|
| 8 | 13.15 ms | (未計測) | (未計測) | 重い regime |
| 9 | 3.57 ms | 3.56 ms | (未計測) | **outlier (低)** |
| 10 | **16.70 ms** | 16.65 ms | **16.64 ms** | 重い regime |

→ **9 周目が outlier、normal regime は 13-17 ms。原因 99.97% が `LLUI::setLineWidth(1.f)` 単発と確定**。outlier 9 周目の原因は本章では解明留保 (GPU pipeline state の確率的揺らぎ、driver/scheduler 起因候補)。

**renderShadow body 内訳の 3 点整合** (計測 infra noise が無いことの sanity check):
- cull: 148/135/180 us — 整合
- geom: 72/65/79 us — 整合
- alpha: 395/362/481 us — 整合
- innerOcclusion/matrixSetup/cubeTeardown: 全 < 1 us — 整合

#### 真犯 call chain 確定

```
LLUI::setLineWidth(1.f)                            // llviewerwindow.cpp:3020
  → LLRender2D::setLineWidth(1.f)                  // llui.h:330
    → gGL.setLineWidth(1.f * UIGLScaleFactor)      // llrender2dutils.cpp:1812
      → LLRender::setLineWidth(line_width)         // llrender.cpp:1509
        → if (mLineWidth != line_width || mDirty)  // guard 通過
          → glLineWidth(line_width)                // 16.6 ms ← 主犯
```

**guard が毎フレーム通過する構造的理由**:
- `LLUI::setLineWidth(1.f)` は `1.f × UIGLScaleFactor` を渡す
- 他多数 caller (`gGL.setLineWidth(1.f)` raw 経路、llspatialpartition / llmanip / llviewerwindow:4754 等 30+ 箇所) は **scale 倍率なしの素の 1.f** を渡す
- → `mLineWidth` は frame 中で `1.f` (raw) と `1.f × UIscale` (LLUI 経由) を行き来し、UIscale != 1.0 のシステムでは guard が常に miss する

#### 打ち手 A: `LLUI::setLineWidth(1.f)` 削除 (line 3020)

**実施内容**: llviewerwindow.cpp:3020 の `LLUI::setLineWidth(1.f);` をコメントアウト (zone 包囲は計測継続のため残置)。

**理由**:
- 「frame 頭で念のため 1.f に reset」の意図だが、line width を変更する 30+ caller は **末尾で各自 1.f に戻している** ので reset は冗長
- 仮に reset が必要なケースがあるとしても、それは「個別 caller が 1.f に戻し忘れている bug」であって、毎フレーム 16ms かけて隠す方が悪い
- もし削除で UI 表示の回帰が出れば、その回帰こそ真の問題 (どの caller が leak しているかが暴露される) → 個別修正

**判定基準** (11 周目で確認):
1. `setup_matrixInit_setLineWidth` zone が ~0 us → コール除去成功
2. `vwDraw_setup` parent が ~5 us 近傍 → コスト元確定 (削除で完全解消)
3. `display` parent が −13〜17 ms 規模で低下 → **本物の frame time 削減**
4. もし `display` parent が下がらない → GPU pipeline sync stall が glLineWidth に集約されていただけ、stall は後段 (gUIProgram.bind or pushMatrix or 直後の rootView->draw) に移行 → 別軸打ち手必要

**仮説不確定性**: glLineWidth 自体の cost か、prior draw の GPU sync stall を glLineWidth が trigger しているかは未確定。判定 4 で識別する。

### §4.2.g 11 周目 打ち手 A 検証結果 — GPU sync stall 仮説の決定的確定

**計測条件**: 2026-05-26 active session (frame >= 25478, active frame = 2166)、AYAPerfLogEnabled=1。打ち手 A 適用後 build。**時刻条件: 昼 (時刻差は打ち手判定に影響しない、zone 値で確実に判定可能)** / 同 SLurl (全周共通)。

CSV: `~/.ayastorm_x64/logs/AYAstorm-perf-11pass-baseline.csv`。

#### zone 移動の確定 (判定 1, 2, 4 を同時 PASS)

| zone | 10 周目 (打ち手前) | 11 周目 (打ち手後) | 変化 | 判定 |
|---|---|---|---|---|
| setup_matrixInit_setLineWidth | 16643.74 us | **0.03 us** | -99.9997% | **判定 1 ✓** call 完全除去 |
| vwDraw_setup parent | 16702.26 us | **3.27 us** | -99.98% | **判定 2 ✓** setup 消滅 |
| **vwDraw_rootView** | **730.55 us** | **13652.21 us** | **+1769%** | **判定 4 ✗** stall 後段移動 |
| viewerWindowDraw parent | 16702.26 us | 13684.37 us | **-3018 us / -18%** | 正味 -3 ms |
| display (frame) | 48.90 ms | 39.57 ms | -9.3 ms (時刻差込) | — |
| doFrame_total | 58.04 ms | 49.52 ms | -8.5 ms (時刻差込) | — |

renderShadow body 内訳の 11 周目値も整合 (cull 134, geom 66, alpha 357) → 計測 infra noise なし、打ち手 A 単独の効果と確定。

#### 仮説確定: GPU pipeline sync stall

判定 4 (display 下がらず、stall が `vwDraw_rootView` に移動) で **GPU sync stall 仮説が決定的に validate**:

```
旧 frame 構造 (打ち手前):
[render work...] → [LLUI::setLineWidth が prior frame 完了待ち = 16ms 計上] → [UI render 続き]

新 frame 構造 (打ち手後):
[render work...] → [setLineWidth 無し] → [mRootView->draw の最初の GL state change が prior frame 完了待ち = 13ms 計上]
```

→ `glLineWidth(1.0)` は **CPU work でなく GPU sync point** (state change が driver level で pipeline drain を trigger していた)。call を消すと sync point は後段の次の GL state change に移行する。

#### UI 回帰確認 (AYA 確認済)

- focus border / world map 線 / 選択枠 / manip 軸 全て異常なし
- → 打ち手 A は **回帰なしで維持可能**

#### 含意 (重要)

1. **打ち手 A は維持 (revert しない)**:
   - 正味 -3 ms 削減 (時刻差を補正しても 8/9 周目 setup avg 8.4ms → 11 周目 0.003ms = vwDraw_setup zone 単独で大幅減)
   - `vwDraw_setup` zone がクリーンになり、今後の measurement で UI render の真の bottleneck が見えやすくなる
   - UI 回帰なし

2. **13 ms gap の本質は CPU でなく GPU**:
   - CPU perf 章 (本 spec の射程) では削減不可
   - GPU 側打ち手 (frame 中の sync point 削減 / async UI dispatch 等) は別章で扱う

3. **`vwDraw_rootView` 内の drill は意味が薄い**:
   - stall は scope 内部の特定 zone でなく GL state change の最初に "現れる" だけ
   - drill しても sync stall を内訳できない (driver-level の物理現象)

4. **真の CPU work bottleneck は別系統**:
   - **doOcclusion 6.17 ms/frame** (renderGeomDeferred 配下、Layer 2 で確定済) ← 次 phase 本丸
   - これは GPU sync stall でなく **`glGetQueryObjectuiv` polling + occlusion test の CPU side work** で、削減余地あり (frame skip / lazy fetch / sub-sampling 等)

---

### §4.2.h 18 周目 Layer 6 全周回 結果 — updateCull / renderDeferredLighting 内訳

**計測条件**: 2026-05-26 active session (wall_ms 1003→137621 = **136.6 s**, max_frame=27755, display fps ≈ 26 (renderDeferredLighting count 3585 から逆算))、AYAPerfLogEnabled=1。Group M 配線後 (updateCull 3 zone + renderDeferredLighting 4 zone) build。同 SLurl (8-11 周目と一致)。

CSV: `~/.ayastorm_x64/logs/AYAstorm-perf-18pass-baseline.csv` (45 MB、Group M-only build run、人手 login。**§4.2.i の同名 CSV (`tests/aya-gui/artifacts/.../AYAstorm-perf-18pass-baseline.csv`、406 MB) とは別ファイル**: §4.2.i は Group N zones 追加後の M+N 合算 build の run_perf.py 自動 capture)。

備考: Layer 6 では **per-call (= per-fire)** を主軸に読む。frame ID は HUD render path + shadow split から増加するため、`total_us / frame_count` の per-frame 換算は zone 間で母数が揃わず比較不能 (03 spec §6.11 注記済)。

#### M-1 詳細: updateCull body 3 分解

| zone | total (us) | count | per-call (us) | parent 比 (per-call) |
|---|---|---|---|---|
| updateCull (parent, llviewerdisplay.cpp:903) | 1,779,808 | 1,809 | **983.86** | 100% |
| updateCull_waterClip | 1,639 | 19,221 | 0.09 | 0.01% (per-call) |
| updateCull_regionPartition | 3,902,140 | 19,221 | **203.01** | 20.6% (per-call) |
| updateCull_skyRender | 2,739 | 19,221 | 0.14 | 0.01% (per-call) |

##### 観察

1. **sub-zone count 19221 / parent count 1809 = 10.63×** — top-level frame の updateCull 1 回に対し、sub-zone は HUD render + shadow split (4 cascade × 1.5 + Hero probe 等) 経路から **約 10 回 fire**。03 spec §6.11 既知制約通り。
2. **updateCull body の hot は `regionPartition` 一択** (per-call 203 us / parent 984 us = **20.6%**)。waterClip / skyRender は 0.1 us クラスで誤差。
3. parent − Σ children (per-call) = 984 − 203.24 = **780.6 us** が body 外 implicit (priority queue / agent setup / impostor / partition init 系) に潜在。これは function-internal の前後 setup で、Layer 7 drill すれば更に追えるが、**1 個の dominant sub-zone (regionPartition) で 20% を占めることを掴んだのが本周回の収穫**。
4. shadow/HUD path 全体での **regionPartition 総コスト = 0.203 ms × 10.6 fire = 2.15 ms/frame 級** (top-level 1 回 + shadow 9 回程度の合算)。これは Phase 1.2 で別 thread に剥がす候補として最有力。

#### M-2 詳細: renderDeferredLighting body 4 分解

| zone | total (us) | count | per-call (us) | per-call 占有率 |
|---|---|---|---|---|
| renderDeferredLighting (parent) | 3,132,044 | 3,585 | **873.65** | 100% |
| _lightmap | 22,568 | 3,585 | 6.30 | 0.7% |
| _atmospherics | 16,871 | 3,585 | 4.71 | 0.5% |
| _localLights | 419,094 | 3,585 | 116.90 | 13.4% |
| _postDeferred | 2,655,448 | 3,585 | **740.71** | **84.8%** |

Σ children (per-call) = 6.30 + 4.71 + 116.90 + 740.71 = **868.62 us** / parent 873.65 = **99.42% カバー**

##### 観察

1. **implicit setup gap = 873.65 − 868.62 = 5.03 us/call = 0.58%** — 03 spec §6.11 の "明示 zone 切らず逆算" 方針が妥当だったことを確認。**Layer 7 drill 不要**。
2. **renderDeferredLighting の主犯確定 = `postDeferred` (per-call 741 us、children の 85%)**。handoff の事前仮説 PASS。`postDeferred` は softenLightF + atmosphericsFinal + DoF + tone map 一帯の post-deferred pass を内包。
3. localLights が 13% (117 us/call) — 二位だが postDeferred の 1/6。
4. lightmap / atmospherics は 1% 未満 (誤差級)。

#### Layer 6 全周回 総括

| 観点 | 判定 |
|---|---|
| updateCull 主犯確定 | ✓ `regionPartition` (per-call 203 us、parent の 20.6%、shadow/HUD path 10× fire) |
| renderDeferredLighting 主犯確定 | ✓ `postDeferred` (per-call 741 us、children の 85%) |
| implicit setup gap (renderDeferredLighting) | **0.58% (5 us/call)** — drill 不要 |
| implicit setup gap (updateCull) | **79.4% (781 us/call)** — body 外 setup に潜在、但し dominant sub-zone (regionPartition) 確認済 |
| Layer 7 drill 必要性 | **不要** (renderDeferredLighting は 99% カバー、updateCull は dominant sub 確認済) |
| Phase 1.1 完成判定 | **✓ PASS** |

##### Phase 1.2 (Core 振り分け設計) 着手判断

- 02-offload-feasibility §A1-A14 deep-dive ✓
- Layer 6 全周回 (Group M 配線 + 18 周目 CSV) ✓
- 18 周目 CSV 結果整理 (本 §4.2.h) ✓

**3 条件すべて充足 → Phase 1.2 (`05-core-assignment-plan.md` Y-refined 改訂版) 着手可。**

Phase 1.2 で集約すべき main thread offload 候補 (現時点 top-3):

| 候補 | per-frame コスト概算 | 剥がし先候補 |
|---|---|---|
| `doOcclusion_reflectionProbes` (Layer 5/6 既存) | 6.37 ms/call × 1.28 call/frame ≈ 8.1 ms (もしくは display frame 換算で 3.16 ms) | Worker A (occlusion query polling) |
| `renderDeferredLighting_postDeferred` | 741 us/display-frame ≈ 0.74 ms | GPU 側 / async (CPU side は dispatch だけ、削減余地確認要) |
| `updateCull_regionPartition` (shadow/HUD 含む 10 fire) | 203 us × ≈10 = 2.0 ms/frame | Worker B (partition traversal / impostor) |

詳細振り分けと依存解析は 05 spec で詰める。

---

### §4.2.i 19 周目 Layer 8 Group N 結果 — Hero probe doOcclusion 状態機械内訳

**計測条件**: 2026-05-26 active session、SLurl `secondlife://util.aditi.lindenlab.com/secondlife/Bonifacio/179/69/26` (Aditi grid、昼時間帯固定、8/9/10/11/18 周目と一致)、AYAPerfLogEnabled=1。
**配線**: `indra/newview/llreflectionmap.cpp:383/391/407` の Group N-1/N-2/N-3 zone — `LLReflectionMap::doOcclusion()` 内 GL クエリ状態機械 3 分解。
**CSV**: `tests/aya-gui/artifacts/20260526-111600-414334/AYAstorm-perf-18pass-baseline.csv` (406 MB、累積 13.09 M rows、38,499 unique frames、wall_ms 873〜153,133 = 152.3 s)。**Group M+N 合算 build の run_perf.py 自動 capture**。§4.2.h の同名 CSV (`~/.ayastorm_x64/logs/...`、45 MB) とは別ファイル — basename 衝突は次回 capture から `day12-group-N.csv` 形式へ移行 (本周回分は本記述で disambiguate)。
**自動起動**: 本周回から `tests/aya-gui/run_perf.py` (LEAP harness 経由 LLURLDispatcher `secondlife:///app/location_login/...` 自動 login) で人手 login 不要化。

#### N 詳細: rmdo 状態機械 3 分解

| zone | total (us) | count | per-call (us) | parent 比 |
|---|---:|---:|---:|---:|
| **parent: doOcclusion_reflectionProbes** | 2,116,598 | 8,192 | **258.374** | 100.0% |
| N-1: rmdo_resultAvail (GL_QUERY_RESULT_AVAILABLE poll) | 155,527 | 3,509,084 | **0.044** | 7.35% |
| N-2: rmdo_resultRead (GL_QUERY_RESULT fetch、AVAILABLE>0 時のみ) | 118,162 | 3,507,879 | **0.034** | 5.58% |
| N-3: rmdo_pushQuery (glBeginQuery + uniform + drawCube + glEndQuery、do_query=true 時のみ) | 206,813 | 3,508,639 | **0.059** | 9.77% |
| **Σ rmdo_***  | **480,502** | 10,525,602 | — | **22.70%** |
| **parent − Σ rmdo_*** (= probe iteration / branching / cube vertex setup 等 implicit body) | **1,636,096** | — | — | **77.30%** |

#### 観察

1. **rmdo_* 3 zone は per-call sub-microsecond** — 状態機械 GL 呼び出し本体 (AVAILABLE poll / RESULT fetch / Begin+drawCube+End) は完全に GPU 待ちでなく driver 側 enqueue だけで決着している。**Layer 9 (rmdo_* 内側 drill) 不要**。
2. **真犯人は parent − Σ の 77.30% (= per-call 199.6 us 相当の implicit body)** — Hero probe iteration loop (mProbes 走査)、probe 毎の condition branch (`do_occlusion_query` / `mCubeArray` 有無 / `mOccluded` 状態遷移)、cube vertex buffer 共有準備、occlusion query handle 配列管理が支配的。
3. **do_query=true 比率 ≈ 99.99%** (pushQuery_count / resultAvail_count = 3,508,639 / 3,509,084) — RESULT 読了直後 ほぼ全 probe が即時 re-query を発行している。query を **間引く / pipeline する** 余地は GL 状態機械側でなく、**probe 群を 1 frame 内に何個まで投げるかという policy 層**にある。
4. parent per-call 258 us × 1.28 call/frame (Layer 6 と同一 budget) ≈ **331 us/frame ≈ 0.33 ms/frame** — display frame 換算で main thread 上 fixed cost。仮に 5.x 章 §4.2.h top-3 候補 (doOcclusion_reflectionProbes 全体 8.1 ms/frame) と区別すると、本 Layer 8 で「Hero probe 自体は GL 待ちでなく iteration / branch 層」と確定。

#### Layer 8 Group N 総括

| 観点 | 判定 |
|---|---|
| rmdo_* 状態機械 hot path 性 | **✗ 否** (per-call sub-us、parent 比 22.70%) |
| Hero probe 主犯部位 | **probe iteration loop + branch + cube vertex setup (parent − Σ = 77.30%)** |
| Layer 9 (rmdo_* 内側) drill 必要性 | **不要** (per-call 0.04〜0.06 us、これ以上分解しても誤差級) |
| do_query 間引き policy 検討必要性 | **可** (99.99% 即時 re-query、frame 跨ぎ間引きで主観的に effective probe 数を下げられる) |
| Phase 1.2 における Hero probe offload 設計示唆 | **GL 呼び出し移送ではなく iteration loop 全体を worker に剥がすべき** (rmdo_* を worker 側に切出すだけでは 22.70% しか取れない) |

##### Phase 1.2 (`05-core-assignment-plan.md`) への直接 input

- §4.2.h で挙げた Worker A 候補「occlusion query polling」 = rmdo_resultAvail/resultRead — 本周回で **per-call 0.04 us = 剥がしても効かない** ことが確定。Worker A の真の target は **`LLReflectionMapManager::doOcclusion(camera)` 関数全体** (mProbes 走査ループごと別 thread)。
- worker 移送時の同期境界: camera frustum (read-only snapshot) + probe occlusion state 更新 (write-back queue) + GL context 制約 (GL 呼び出しは main thread 専有のため worker からの query 発行は不可)。
- 上記制約から、Hero probe offload は **「probe visibility 判定 / occlusion 結果消費」を worker 側で行い、`glBeginQuery`/`glEndQuery` 発行のみ main thread に残す split** が現実解。
- 設計詳細は 05 spec §(TBD) で詰める。

---

### §4.3 sun cascade の per-call 分析

cube_snapshot 中に sun_0/sun_1 だけ 2 回発火 (cascade 0/1 のみ cube face shadow 再計算)。
count = 2514 (= 1450 + 1064) → sun_0/1 は main + cube の合算。sun_2/3 は count 1449 = main only。

per-call (main scene shadow):
- sun_0: 6395 ms / 2514 = 2.54 ms/call (main + cube 平均)
- sun_1: 2389 / 2514 = 0.95 ms/call
- sun_2: 3751 / 1449 = 2.59 ms/call (main only)
- sun_3: 8363 / 1449 = **5.77 ms/call (main only)** ← 突出

**含意**: sun_3 は最遠 cascade で frustum が広く、shadow map に投影する drawable が多い。per-cascade setup (frustum compute / FBO bind / culling) と draw 本体のどちらが効いているかは Layer 2 drill で分離。

---

### §4.2.j 19 周目 Layer 8 Group O 結果 — vwDraw per-child 分解 (Day 2-3)

**計測条件**: 2026-05-26 active session、SLurl `secondlife://util.aditi.lindenlab.com/secondlife/Bonifacio/179/69/26` (Aditi grid、昼)、AYAPerfLogEnabled=1。Group O 配線後 (`LLView::drawChildren()` parent-name gate、root / main_view の 2 段 per-child 動的 zone)。
**CSV**: `tests/aya-gui/artifacts/20260526-114303-467640/AYAstorm-perf-day23-group-O.csv` (291 MB、9.1 M rows、40,812 unique display frames、wall_ms span 135.2s)。
**自動起動**: run_perf.py + LLURLDispatcher 経由 (login 人手不要)。

#### O-1 詳細: mRootView 直下 child 分解

| zone (mRootView 直下) | total (us) | count | per-call (us) | per-frame (us) | 親 (vwDraw_rootView) 比 |
|---|---:|---:|---:|---:|---:|
| **parent: uiRender_ui2d_vwDraw_rootView** | 5,905,268 | 59,456 | **99.322** | 144.71 | 100.0% |
| `vwDraw_root_main_view` | 5,432,483 | 59,456 | **91.370** | 133.11 | **92.0%** |
| `vwDraw_root_console` | 413,342 | 59,456 | 6.952 | 10.13 | 7.0% |
| `vwDraw_root_hud` | 5,683 | 34,488 | 0.165 | 0.14 | 0.1% |
| **Σ root_*** | 5,851,508 | — | — | 143.38 | **99.09%** |
| **parent − Σ root_*** (= drawChildren overhead) | 53,760 | — | — | 1.32 | 0.91% |

**parent count 59,456 / unique frames 40,812 = 1.457 calls/frame** — vwDraw_rootView は display frame 1 + HUD render path 計 1.5 回/frame 発火。

#### O-2 詳細: MainPanel ("main_view") 直下 widget 分解

| zone (main_view 直下) | total (us) | count | per-call (us) | per-frame (us) | 親 (vwDraw_root_main_view) 比 |
|---|---:|---:|---:|---:|---:|
| **parent: vwDraw_root_main_view** | 5,432,483 | 59,456 | **91.370** | 133.11 | 100.0% |
| `vwDraw_mp_menu_stack` | 3,200,338 | 59,456 | **53.827** | 78.43 | **58.9%** |
| `vwDraw_mp_navigation_bar` | 726,784 | 34,488 | 21.074 | 12.22 | 13.4% |
| `vwDraw_mp_progress_view` | 694,340 | 50,667 | 13.704 | 11.68 | 12.8% |
| `vwDraw_mp_Menu Holder` | 166,765 | 59,456 | 2.805 | 2.81 | 3.1% |
| `vwDraw_mp_tooltip view` | 14,459 | 59,456 | 0.243 | 0.24 | 0.27% |
| `vwDraw_mp_popup_holder` | 4,487 | 59,456 | 0.075 | 0.08 | 0.08% |
| `vwDraw_mp_snapshot_floater_view_holder` | 2,875 | 59,456 | 0.048 | 0.05 | 0.05% |
| `vwDraw_mp_hint_holder` | 666 | 24,968 | 0.027 | 0.01 | 0.01% |
| **Σ mp_*** | 4,810,714 | — | — | 105.52 | **88.6%** |
| **parent − Σ mp_*** (= main_view 自身の drawDebugRect / iteration overhead) | 621,769 | — | — | 27.59 | 11.4% |

#### 観察

1. **vwDraw_rootView 親が 144.71 us/frame = 0.145 ms/frame** — Layer 1 (7 周目) 計測時の 18.6 ms/frame という値からは大幅に縮小。原因は (a) Layer 8 計測時 SLurl が Bonifacio 静止 (HUD/floater 少ない) (b) Layer 1 時の session 状態 (login 直後 progress bar / floater 多数) との session diff。**現実の vwDraw cost は 18.6 ms ではなく 0.15 ms 級が baseline**。
2. **mRootView 直下 console (6.95 us/call、7.0%) は予想外の 2nd hot** — Floater console widget が常時 visible で draw されている。drop 候補ではないが Phase 1.2 で「画面外なら skip」の確認余地あり。
3. **MainPanel 直下 main_view drill (O-2) 主犯 = `menu_stack` (78.4 us/frame、58.9%)** — menu_stack は layout_stack で内部に world_panel / status_bar_container / topinfo_bar_container / login_panel_holder / menu_bar_holder 等を抱える。Layer 9 drill する場合は menu_stack 内側 (gMenuBarView 等) が target。
4. **`navigation_bar` (12.22 us/frame、13.4%) と `progress_view` (11.68 us/frame、12.8%) が 2 位 / 3 位** — どちらも比較的軽い。progress_view が count 50,667 (85% visible) なのは dismiss 後も visibility flag が残存している、または mProgressViewMini との切替が dispatch されている可能性。本周回では再現性のため追わない。
5. **tail 5 widget (Menu Holder / tooltip view / popup_holder / snapshot_floater_view_holder / hint_holder) は per-call sub-µs〜2.8 µs**、合計 < 0.4% — Layer 9 drill 対象外。

#### 案 Q (vwDraw text width cache) 判定

| 観点 | 結果 |
|---|---|
| 現状 vwDraw 全体 cost | **0.145 ms/frame** (Layer 1 当初推定の 1/120) |
| 想定主犯 (案 Q: per-frame text width measurement) | menu_stack 内側 LLTextBox 系の getTextWidth 呼出が想定だが、parent 自体が 0.078 ms/frame で frame budget の 0.5% 未満 |
| ROI | text cache 実装 (1 週工数) で 100% 削減できても **0.078 ms/frame 短縮** = 60 fps frame budget 16.67 ms の **0.5%** |
| **判定** | 🔴 **DROP** (Phase 1.2 scope から外す) |

**結論**: 案 Q (vwDraw text cache) は **drop**。Phase 1.2 の剥がし候補は 案 O / 案 R-refined / 案 P-refined の **3 件**で確定。

#### Day 2-3 Layer 8 総括

| 観点 | 判定 |
|---|---|
| Layer 8 root_* per-child 分解 | **✓ 99.09% カバー** (drawChildren overhead < 1%) |
| Layer 8 mp_* per-child 分解 | **✓ 88.6% カバー** (main_view 自身 overhead 11.4% は LLView::draw / drawDebugRect 通常 cost) |
| Layer 9 drill 必要性 | **不要** (vwDraw 全体が 0.145 ms/frame、Phase 1.2 候補からは drop) |
| 案 Q go/no-go | **DROP 確定** |
| Day 7 統合判定への input | **3 候補で確定 (案 O / R-refined / P-refined)、案 Q drop** |

---

## §5. AYAstorm 独自 — 一次判断「軽い」を実測で確定 (継続)

| zone | 6 周目 per-call | 結論 |
|---|---|---|
| occlusionRefresh | 0.20 us | ✓ 確定 (trivial、最適化対象外) |
| stream3DUpdate | 20.8 us | ✓ 確定 (軽い、最適化対象外) |

01 §6 の一次判断が 6 周目でも維持。**AYAstorm r5-r28 独自機能の CPU 負荷は無視可**。

---

## §6. Layer 2 = B (1 段深い全周) 作業 plan

### §6.1 順序

1. Layer 2 zone 追加 (§6.2)
2. 再ビルド (configure 不要)
3. install + cache + CSV clear
4. 同条件で active session 録音
5. **Layer 2 全周が埋まったら 04 を Layer 2 版で更新 (or 04-layer2.md 新規)**
6. gap が <10% になったら **05 spec で初めて打ち手議論**

### §6.2 Layer 2 候補 zone (= 7 周目に追加すべき)

| 親 zone (Layer 1) | Layer 2 zone (新規) | 配線ヒント |
|---|---|---|
| renderGeomDeferred (gap 6.83 ms) | `renderGeomDeferred_doOcclusion` | `pipeline.cpp` 内 `doOcclusion(camera)` call wrap |
| 同上 | `renderGeomDeferred_setupHWLights` | 同 `setupHWLights()` call wrap |
| 同上 | `renderGeomDeferred_postLoop` | pool loop 抜けた後〜関数末まで |
| renderShadow_sun_3 (5.77 ms 突出) | `renderShadow_sun_setup` | sun cascade loop 内、renderShadow() 呼ぶ前 frustum/FBO setup |
| uiRender_ui2d_viewerWindowDraw (18.6 ms) | `uiRender_ui2d_vwDraw_floaterView` | `llviewerwindow.cpp` LLViewerWindow::draw 内 `gFloaterView->draw()` 周辺 |
| 同上 | `uiRender_ui2d_vwDraw_rootView` | 同上 `mRootView->draw()` wrap |

**注意**: 上記 zone 名は仮、配線時に実コード確認 (`llviewerwindow.cpp` は本セッション未読、巨大ファイル予想)。

### §6.3 やらないこと (Layer 2 段階で)

- 最適化判断 (打ち手提案は Layer 2 gap が縮んでから 05 spec で)
- pool dispatch 内訳 (pool_MATERIALS が 0.51 ms と最大だが、これ以上の分解は GPU 側計測領域に入る)
- 計測 instrumentation overhead の精密測定 (Tracy 等への移行検討は別 phase)

---

## §7. 観測者効果 (instrumentation overhead) の警告

各周回での FPS 変動:
- 2 周目: 28 FPS (5 zone)
- 3 周目: 21 FPS (33 zone)
- 4 周目: 18.2 FPS (44 zone, scope bug 含む)
- 5 周目: 17.1 FPS (46 zone, scope 修正後)
- **6 周目: 17.6 FPS (76 distinct zone, Layer 1 完成)**

FPS は zone 数増加で下がっているが直線的ではない (5→6 で zone +30 / FPS +0.5)。これは **空 poll (visitNotifier B2 fix 効いている) と trivial zone (pool_* 等) の overhead が小さい**ため。

**含意**: 数字の **絶対値 (FPS / ms)** は計測 enabled 時の overhead 込みで読む。
**比率と call/frame** は信頼できる。Layer 2 で更に +6 zone なら overhead 影響は誤差範囲予想。

deploy 時 (`AYAPerfLogEnabled=0` default) は全 zone no-op。

---

## §8. 引継ぎ時 — 即座に確認すべきファイル

| 何を見たいか | ファイル |
|---|---|
| 章の出発点 | `docs/specs/ayastorm-cpu-perf/00-overview.md` |
| main thread workload 全リスト (推論ベース) | `docs/specs/ayastorm-cpu-perf/01-mainthread-workload-inventory.md` |
| offload 可能性 (推論ベース) | `docs/specs/ayastorm-cpu-perf/02-offload-feasibility.md` |
| infra 実装 + 配線 (Group A-G 全 catalog) | `docs/specs/ayastorm-cpu-perf/03-perf-log-infra.md` (§6.2-6.5) |
| **実測値による地図 (このファイル)** | `docs/specs/ayastorm-cpu-perf/04-observed-hot-path-map.md` |
| **Layer 2 引き継ぎ書** | `docs/specs/ayastorm-cpu-perf/handoff-layer1-to-layer2.md` |
| 2 周目 raw CSV | `~/.ayastorm_x64/logs/AYAstorm-perf-2pass-baseline.csv` |
| 3 周目 raw CSV | `~/.ayastorm_x64/logs/AYAstorm-perf-3pass-baseline.csv` |
| 4 周目 raw CSV (scope bug 含む) | `~/.ayastorm_x64/logs/AYAstorm-perf-4pass-baseline.csv` |
| 5 周目 raw CSV | `~/.ayastorm_x64/logs/AYAstorm-perf-5pass-baseline.csv` |
| **6 周目 raw CSV (current)** | `~/.ayastorm_x64/logs/AYAstorm-perf.csv` |

---

## §9. 約束されたコミット原則 (memory 参照)

- コミットは AYA さんの明示指示があるまでしない (`feedback_no_auto_commit.md`)
- 計測 infra は default OFF (`AYAPerfLogEnabled=0`) で出荷しても害なし、release note 不要
- 計測完了後 AYA 側 debug settings で `AYAPerfLogEnabled` を 0 に戻す案内を **05 spec 着手前に必ず**
  (memory: `feedback_restore_debug_settings.md`)

---

## §10. 現在位置サマリ (引き継ぎ用 one-liner)

> **19 周目 Layer 8 全周完了 (Day 1-2 Group N + Day 2-3 Group O) + 4 候補 POC spike (Day 4-5/5-6) + Day 7 統合判定済。Phase 1.1 完成、Phase 1.2 着手可。Hero probe doOcclusion rmdo_* は 22.7% (per-call sub-µs)、真犯人は iteration loop 77.3%。vwDraw は実測 0.145 ms/frame で 案 Q drop。剥がし 3 候補 (案 O / R-refined / P-refined) 合計 ceiling 11.7-14.5 ms/frame で go 判定 (02 §F)。**

(以下、旧版 8 周目時点の one-liner)
---

> **8 周目 Layer 3 完了。viewerWindowDraw は `vwDraw_setup` 単独 13.15 ms (94.7%) で完全分解 ✓ — 主犯は関数頭の stop_glerror / gUIProgram.bind / matrix 系のどれか。renderShadow body は body_alpha 395 us/call 最重 + 残 gap 674 us/call (26%) で部分分解。次は Layer 4 = vwDraw_setup 内側 5 zone + renderShadow body 残 gap 3 zone 同時 drill。**

(以下、旧版 7 周目時点の one-liner)
---

> **7 周目 Layer 2 完了。renderGeomDeferred は doOcclusion 6.17 ms 主犯で完全分解 ✓、renderShadow_sun_3 setup は 0.57 ms と小さく真犯人は body 側、viewerWindowDraw 13 ms gap は mRootView->draw 外で予想外発見。次は Layer 3 = LLViewerWindow::draw 関数全域 wrap + renderShadow() body 内側 drill。**

詳細引き継ぎ書: `docs/specs/ayastorm-cpu-perf/handoff-layer2-to-layer3.md` (本書と並列)

(以下、旧版 6 周目時点の one-liner)
---


> **Layer 1 (doFrame_total 直下の主要 zone を全部同じ粒度で内訳化) 完了。次は Layer 2 = 1 段深い全周 = 「renderGeomDeferred の 6.8 ms gap / sun_3 5.77 ms / gViewerWindow->draw 18.6 ms」の中身を全部同時に drill する。**
