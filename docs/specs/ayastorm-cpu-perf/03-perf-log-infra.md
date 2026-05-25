# 03 — Perf log infrastructure & baseline 計測 results

`docs/specs/ayastorm-cpu-perf/` の 1 番目の実装。
§7-A の数字を取るための **zone CSV writer infra**。

このファイルは 2026-05-26 時点の **handoff 状態**。
ここを起点に B1/B2 fix と wide zones 追加に進む。

---

## §1. 実装した infra

### §1.1 モジュール本体 (llcommon 配置)

| ファイル | 役割 |
|---|---|
| `indra/llcommon/llayastormperflog.h` | public API + `AYAPERF_ZONE(name)` macro |
| `indra/llcommon/llayastormperflog.cpp` | CSV writer + RAII zone 実装 |

**公開 API**:

```cpp
namespace LLAyastormPerfLog {
    void init(bool enable, const std::string& output_path);
    void shutdown();
    void onFrameEnd();
    void recordSample(const char* zone, U64 dt_us);
    bool isEnabled();
}

class LLAyastormPerfZone { /* RAII */ };
#define AYAPERF_ZONE(name) LLAyastormPerfZone _aya_perf_zone_(name)
```

**配置理由**: llrender (syncToMainThread) / llcorehttp (visitNotifier) からも include できるよう **llcommon に置いた**。lower layer は newview に依存できないため。
- 当初 newview に置いて build したが、syncToMainThread 配線時に層を上げた (2026-05-26)
- llcommon は `lldir.h` に依存しないため、`init()` が path を引数で受ける形にした (caller が gDirUtilp で解決)

**thread-safety**: `recordSample` は internal mutex で保護。main + worker (LLImageGLThread、HttpService 等) 双方から呼んで良い。

### §1.2 cvar gate

`indra/newview/app_settings/settings.xml` に `AYAPerfLogEnabled` 登録 (U32, default 0, Persist=1)。

- **0 = 完全 no-op** (起動時に `init()` で即 return、file open / disk I/O 一切なし)
- **1 = 起動時に `<userdir>/logs/AYAstorm-perf.csv` を open**、`AYAPERF_ZONE` 各 scope の dt を append
- **再起動で適用** (起動時のみ判定、live 切替不可)

### §1.3 起動 / シャットダウン

`indra/newview/llappviewer.cpp`:

- **起動側** (settings load 後、cinematic overlay の隣):
  ```cpp
  LLAyastormPerfLog::init(
      gSavedSettings.getU32("AYAPerfLogEnabled") != 0,
      gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "AYAstorm-perf.csv"));
  ```
- **frame end 側** (doFrame 末尾、`pingMainloopTimeout("Main:End")` の直後):
  ```cpp
  LLAyastormPerfLog::onFrameEnd();   // global frame counter ++
  ```
- **shutdown 側** (cleanup() 末尾、`LL_INFOS() << "Goodbye!"` の手前):
  ```cpp
  LLAyastormPerfLog::shutdown();     // flush + close
  ```

### §1.4 出力形式

```
frame,wall_ms,zone,dt_us
12345,1763456,idleUpdate,4823
12345,1763456,octreeBalance,12
...
```

- `frame` = doFrame 累計 (起動からの全 frame、login 前含む)
- `wall_ms` = init() からの経過 ms
- `zone` = `AYAPERF_ZONE("...")` で渡した文字列
- `dt_us` = RAII scope dt (us)

post-process は awk / pandas / SQL いずれも自由。

---

## §2. 現在配線されている zone (5 個 → 6 zone name)

| zone 名 | 配線ファイル | コード位置 | 計測対象 |
|---|---|---|---|
| `idleUpdate` | `indra/newview/llviewerobjectlist.cpp` | `update()` 内、idle iteration block 全体 (~L1031-1068) | `mActiveObjects (~21k) × idleUpdate()` + flexible + texture-anim |
| `octreeBalance` | `indra/newview/pipeline.cpp` | `updateMove()` 内 (~L2666-2686) | 全 region × NUM_PARTITIONS の `mOctree->balance()` |
| `updateCull` | `indra/newview/llviewerdisplay.cpp` | display() 内 (line ~898 周辺) | `gPipeline.updateCull(...)` (前 frame OQ 結果回収 + frustum cull) |
| `fetchQueryResult` | `indra/newview/llviewerdisplay.cpp` | display() 内 (line ~1014 周辺) | `LLSceneMonitor::fetchQueryResult()` (GL query wait) |
| `syncToMainThread` | `indra/llrender/llimagegl.cpp` | `LLImageGL::syncToMainThread()` 先頭 (line ~1748) | NVIDIA `glClientWaitSync` / AMD WorkQueue post (worker thread) |
| `visitNotifier` | `indra/llcorehttp/httprequest.cpp` | `HttpRequest::update()` 先頭 (line ~415) | reply queue drain + visitNotifier 呼出 (**B2: 修正必要**) |

各 grep キーワード: `AYAPERF_ZONE`、`<FS:AYAstorm> CPU perf 章 §7-A`

---

## §3. ビルド状態

- 1 周目: idleUpdate のみ配線 → 2026-05-25 build PASS → AYA 検証 PASS (仕組み正しさ)
- **2 周目: 5 zone (idleUpdate + 残り 4 + visitNotifier) 配線 → 2026-05-26 build PASS → AYA 60 秒録音完了**
- 出力 CSV: `/home/ishikawa/.ayastorm_x64/logs/AYAstorm-perf.csv` (~70MB、2.2M 行、大半が visitNotifier の空 poll)

ビルドは標準フロー (`project_build_procedure.md` 参照):
```
autobuild configure -A 64 -c ReleaseFS_open -- --fmodstudio -DLL_TESTS:BOOL=FALSE -DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE --package --chan AYAstorm-release
autobuild build -A 64 -c ReleaseFS_open --no-configure
```
新 .cpp/.h 追加なしなら configure 不要。

---

## §4. 2 周目 baseline 計測 results (2026-05-26, 60 秒 session)

| zone | count | total | avg | 一次判定 |
|---|---|---|---|---|
| **updateCull** | 1,664 | **1,910.7 ms** | **1,148 us/frame** | **重い** (前 frame OQ wait + frustum cull) |
| **idleUpdate** | 3,771 | **1,921.0 ms** | 509 us/frame | 重い (21k object loop) |
| **syncToMainThread** | 1,384 | 445.3 ms | 322 us/call | 中。NVIDIA fence wait は 30% CPU 主因では**ない** |
| **octreeBalance** | 3,775 | 26.8 ms | 7 us | trivial |
| **fetchQueryResult** | 1,664 | 2.2 ms | 1.3 us | trivial |
| visitNotifier | 2,185,187 | **(overflow)** | — | **B1/B2 修正待ち** |

### §4.1 重要な観察

- 60 秒 × ~28 FPS (display 1664 frame) ≒ **frame budget 合計 ~60 sec**
- 計測 5 zone (visitNotifier 除く) 合計 ~4.3 sec = **frame 全体の 7% しかカバーできていない**
- **残り 93% (50+ sec) は計測 zone の外** = display() 内側の deferred geom / state sort / shadow / local lights / post-proc / UI render あたりに潜む

### §4.2 frame counter 不整合

- idleUpdate count = 3,771 / octreeBalance count = 3,775 → doFrame 全 frame で発火
- updateCull / fetchQueryResult count = 1,664 → display() に到達した frame のみ

→ 起動〜ログイン中に doFrame は呼ばれるが display() に行かない frame が約 2,100 ある (login 進行 / window not shown 等)。login 後は 28 FPS = 60 秒 × 28 = 1,680 ≒ 1,664 で整合。

---

## §5. 既知 bug (3 周目で fix)

### §5.1 (B1) U64 underflow in zone destructor

`llcommon/llayastormperflog.cpp` の `LLAyastormPerfZone::~LLAyastormPerfZone()`:

```cpp
LLAyastormPerfZone::~LLAyastormPerfZone()
{
    if (mStartUs != 0)
    {
        const U64 dt_us = totalTime() - mStartUs;  // ← underflow!
        LLAyastormPerfLog::recordSample(mZoneName, dt_us);
    }
}
```

**症状**: 49 行で `dt_us = 18446744073709551615` (= 2^64-1) → sum オーバーフロー → avg がゴミ

**原因**: zone scope が短すぎる (~0 us) + `totalTime()` の clock 粒度・コア間スキューで end < start を観測

**fix**:
```cpp
const U64 now = totalTime();
const U64 dt_us = (now >= mStartUs) ? (now - mStartUs) : 0;
```

### §5.2 (B2) visitNotifier 過剰計測

`indra/llcorehttp/httprequest.cpp` の `HttpRequest::update()` 先頭に zone を置いたため、queue が空でも 1 行 (dt=0) 記録される。

**症状**: 2.2M 行のうち 95% (210万行) が dt=0 の空 poll。CSV が 70 MB に膨らむ、avg を見ても意味なし、disk I/O が計測自体を歪める懸念。

**fix**: zone を inner loop に絞る (`replies.empty()` 判定後の処理側):

```cpp
HttpStatus HttpRequest::update(long usecs)
{
    HttpOperation::ptr_t op;

    if (usecs)
    {
        const HttpTime limit(totalTime() + HttpTime(usecs));
        while (limit >= totalTime() && (op = mReplyQueue->fetchOp()))
        {
            AYAPERF_ZONE("visitNotifier");   // ← 各 callback 単位
            op->visitNotifier(this);
            op.reset();
        }
    }
    else
    {
        HttpReplyQueue::OpContainer replies;
        mReplyQueue->fetchAll(replies);
        if (!replies.empty())
        {
            AYAPERF_ZONE("visitNotifier");   // ← 空でない時だけ
            for (HttpReplyQueue::OpContainer::iterator iter(replies.begin());
                 replies.end() != iter; ++iter)
            {
                op.reset();
                op.swap(*iter);
                op->visitNotifier(this);
            }
        }
    }
    return HttpStatus();
}
```

これで dt=0 の空行が消える。

---

## §6. 3 周目作業 plan — 「地図優先」方針 (2026-05-26 AYA 確認)

### §6.0 章の目的 (再定義)

2 周目で「updateCull 重い (1148us)」「idleUpdate 重い」等の所見が出たが、
**5 zone (frame の 7%) しかカバーしていない時点での所見は早計**。
残り 93% の missing time に隠れた hot path が解像度低いままだと「目に見えたバグ」を
潰しても章の本丸を外す。

**3 周目の目的は最適化ではなく「解像度の高い大きな地図」を作ること**。
具体的な道筋を立てるための最初の情報を厚く取る。

ただし **地図が書けないバグ (B1/B2) は先に潰す** — 物差しが歪んだ状態で地図は描けない。
B1/B2 は「機能バグ」ではなく「計測 infra の校正」として位置づける。

### §6.1 順序

1. **B1 fix** (llcommon/llayastormperflog.cpp、~3 行) ← 物差し校正
2. **B2 fix** (llcorehttp/httprequest.cpp、上記 §5.2 の通り) ← 物差し校正
3. **wide zones 27 個追加** (§6.2 参照) ← 地図を厚くする本体
4. **再ビルド** (configure 不要、--no-configure のみで可)
5. **install + cache clear** (CSV も削除)
6. **AYA に 60 秒 baseline 環境で再録音依頼**
7. **CSV 解析 + 04-observed-hot-path-map.md で地図 1 枚絵化**
8. **04 を見て初めて「打ち手」の議論 (= 05 以降)** — ここまで最適化判断はしない

### §6.2 追加する wide zones カタログ (27 個 / 5 group)

既存 6 zone (idleUpdate, octreeBalance, updateCull, fetchQueryResult, syncToMainThread, visitNotifier)
に以下 27 を加えて **計 33 zone** とする。

#### Group A. doFrame 上層 (5 個) — frame budget の天井
| zone 名 | 配線位置 | 根拠 (01 §1) |
|---|---|---|
| `doFrame_total` | llappviewer.cpp:1568 doFrame() 全体 | frame 全体の budget |
| `idle` | llappviewer.cpp:1748 idle() call site 包む | idle() 全体 |
| `display` | llappviewer.cpp:1780 display() call site 包む | display() 全体 |
| `updateTextureThreads` | llappviewer.cpp:1874 | texture cache / decode tick |
| `meshRepoUpdate` | llappviewer.cpp:1894 gMeshRepo.update | mesh fetch queue tick |

#### Group B. idle() 内側 (6 個) — idleUpdate の周辺
| zone 名 | 配線位置 | 根拠 (01 §3) |
|---|---|---|
| `messagePump` | llappviewer.cpp:6559 checkAllMessages + processAcks | LL message tick |
| `regionIdleUpdate` | llworld.cpp:1142 / 1170 | LLViewerRegion::idleUpdate × region 数 |
| `drawablesUpdateMove` | pipeline.cpp:2663 updateMovedList | spatial partition update |
| `particleSim` | llworld.cpp:1205 LLViewerPartSim::updateSimulation | per-frame |
| `inventoryObserver` | llinventorymodel.cpp:2339 | dirty 時のみ |
| `idleCallbacks` | llcallbacklist.cpp:112 callFunctions | per-frame |

#### Group C. display() 内側 (12 個) — render pass 解像度
| zone 名 | 配線位置 | 根拠 (01 §2) |
|---|---|---|
| `stateSort` | pipeline.cpp:3976 | visible → per-pool + z-sort |
| `renderShadow` | pipeline.cpp:12837 | shadow frustum + RTT |
| `renderGeomDeferred` | pipeline.cpp:5039 | Pool iteration + draw call |
| `renderDeferredLighting` | pipeline.cpp:11049 | sun shadow + local lights |
| `atmosphericsHaze` | pipeline.cpp:11668 | post-deferred fullscreen |
| `renderFinalize` | pipeline.cpp:10035 | tonemap / glow / DoF / AA |
| `volumetric` | pipeline.cpp:10156 (条件付 / Cinematic) | renderVolumetric pass |
| `motionBlur` | pipeline.cpp:10173 (条件付 / Cinematic) | velocity buffer + composite |
| `depthOfField` | pipeline.cpp:10191 (条件付) | CoC + blur |
| `hudAttachmentRender` | llviewerdisplay.cpp:1384 | HUD camera cull + render |
| `uiRender` | llviewerdisplay.cpp:1598 | hud_elements + ui_3d + ui_2d |
| `swapBuffers` | llviewerdisplay.cpp:1703 | GPU 同期 / present |

#### Group D. texture upload 境界 (2 個) — main / worker bridge の表側
| zone 名 | 配線位置 | 根拠 (01 §5) |
|---|---|---|
| `updateImagesCreateTextures` | llviewertexturelist.cpp:1114 | per-frame batch GL upload |
| `updateImagesFetchTextures` | llviewertexturelist.cpp:1253 | per-frame fetch enqueue |

#### Group E. AYAstorm 独自 (2 個) — 一次判断「軽い」を実測で確認
| zone 名 | 配線位置 | 根拠 (01 §6) |
|---|---|---|
| `occlusionRefresh` | llocclusiongeometrymgr.cpp:275 refreshOccluders | OBB pre-cull + extract drain |
| `stream3DUpdate` | llpositionalstreammgr.cpp:2398 update | linkset eval + binding |

**注意**: 上記 line 番号は 01 inventory 作成時 (2026-05-25) のもの。grep で再確認してから配線すること。
条件付 zone (volumetric / motionBlur / depthOfField / particleSim / inventoryObserver) は
無効時 scope に入らない位置に置く (CSV 行数の節約 + B2 と同じ罠回避)。

### §6.3 分析の見方 (4 周目で使う差分式)

地図は **包含関係** で書く。各 zone は親の内側にある:

```
doFrame_total
├─ idle
│   ├─ idleUpdate     (object × 21k)
│   ├─ octreeBalance
│   ├─ messagePump
│   ├─ regionIdleUpdate
│   ├─ drawablesUpdateMove
│   ├─ particleSim
│   ├─ inventoryObserver
│   └─ idleCallbacks
├─ display
│   ├─ updateCull
│   ├─ stateSort
│   ├─ renderShadow
│   ├─ renderGeomDeferred
│   ├─ renderDeferredLighting
│   ├─ atmosphericsHaze
│   ├─ renderFinalize
│   ├─ volumetric          (Cinematic)
│   ├─ motionBlur          (Cinematic)
│   ├─ depthOfField        (条件付)
│   ├─ hudAttachmentRender
│   ├─ uiRender
│   ├─ fetchQueryResult
│   └─ swapBuffers
├─ updateTextureThreads
│   └─ updateImagesCreateTextures / updateImagesFetchTextures
└─ meshRepoUpdate

(独立 worker thread)
syncToMainThread          (LLImageGLThread)
visitNotifier             (HttpService → main reply drain)

(AYAstorm 独自 — idle/display どこから呼ばれるかは grep で再確認)
occlusionRefresh
stream3DUpdate
```

差分式の例:
- `display - (updateCull + stateSort + renderShadow + renderGeomDeferred + renderDeferredLighting + atmosphericsHaze + renderFinalize + volumetric + motionBlur + depthOfField + hudAttachmentRender + uiRender + fetchQueryResult + swapBuffers)` = **display 内の未計測隙間**
- `idle - (idleUpdate + octreeBalance + messagePump + regionIdleUpdate + drawablesUpdateMove + particleSim + inventoryObserver + idleCallbacks)` = **idle 内の未計測隙間**
- `doFrame_total - (idle + display + updateTextureThreads + meshRepoUpdate)` = **doFrame 上層の未計測隙間**

隙間が大きい階層が次の解像度向上 (= 4 周目以降の追加 zone) 候補。隙間が小さくなった段階で
04 spec に「観測された hot path map」を 1 枚絵で確定させる。

### §6.4 4 周目 sub-zone カタログ (11 個 / Group F)

04 spec §4 の 3 大発見 (uiRender 14.7 ms / renderShadow 20.9 ms / stateSort 10.5 call/frame) を
**内訳化** するため 11 zone を追加。計 **44 zone**。

#### Group F-1. uiRender 内訳 (3 個)
| zone 名 | 配線位置 | 目的 |
|---|---|---|
| `uiRender_hudElements` | llviewerdisplay.cpp render_ui() 内 LL_PROFILE_ZONE_NAMED_CATEGORY_UI("HUD") 直後 | render_hud_elements() 寄与 |
| `uiRender_3d` | llviewerdisplay.cpp render_ui() 内 LL_PROFILE_ZONE_NAMED_CATEGORY_UI("UI 3D") 直後 | render_ui_3d() 寄与 |
| `uiRender_2d` | llviewerdisplay.cpp render_ui() 内 LL_PROFILE_ZONE_NAMED_CATEGORY_UI("UI 2D") 直後 | LLHUDObject::renderAll + render_ui_2d() 寄与 |

#### Group F-2. renderShadow 内訳 (3 個)
| zone 名 | 配線位置 | 目的 |
|---|---|---|
| `renderShadow_sun` | pipeline.cpp:13121 sun cascade for loop body 先頭 (per-iter sample) | 4 cascade ごとの個別 dt |
| `renderShadow_projector` | pipeline.cpp:13540 projector for loop body 先頭 (per-iter sample) | spot light 2 個ごとの個別 dt |
| `renderShadow_stateSort` | pipeline.cpp:12456 stateSort 呼出 wrap | shadow pass 内 stateSort 寄与 (= sun + projector + cube_face の合計回数) |

#### Group F-3. stateSort caller 別 (4 個)
| zone 名 | 配線位置 | 目的 |
|---|---|---|
| `stateSort_main` | llviewerdisplay.cpp:1007 wrap | display() main path |
| `stateSort_cubeFace` | llviewerdisplay.cpp:1347 wrap | display_cube_face() 内 |
| `stateSort_hud` | llviewerdisplay.cpp:1488 wrap | render_hud_attachments() 内 HUD scene |
| `stateSort_impostor` | pipeline.cpp:13935 wrap | generateImpostor() (rare) |

#### Group F-4. cube face wrap (1 個)
| zone 名 | 配線位置 | 目的 |
|---|---|---|
| `cubeFaceRender_total` | llviewerdisplay.cpp:1287 display_cube_face() 関数定義先頭 | reflection map per-frame コスト直視 (上記 renderShadow_sun × cube_face 部分 + stateSort_cubeFace + その他を包含) |

**包含関係 (4 周目)**:
- `renderShadow` ⊃ {`renderShadow_sun` × cascade 数, `renderShadow_projector` × 2, `renderShadow_stateSort`}
- `cubeFaceRender_total` ⊃ {`stateSort_cubeFace`, `renderShadow` (cube face 側), 他 cube face 内描画}
- `stateSort` 計 = `stateSort_main` + `stateSort_cubeFace` + `stateSort_hud` + `stateSort_impostor` + `renderShadow_stateSort` (検算可能)

### §6.5 6 周目 sub-zone カタログ (Group G — Layer 1 全周 / 11 catalog → 32 distinct zones)

5 周目で見えた **uiRender_ui2d 18.6 ms / renderShadow_sun の cascade 数不明 / renderGeomDeferred 内訳不明 / renderShadow_dispatch 寄与不明** を埋めるための **Layer 1 全周** (= 「A: 今の階層を全部見て回る」、memory `feedback_perf_map_bfs_drill.md`)。

#### Group G-1. uiRender_ui2d 内訳 (2 個)
| zone 名 | 配線位置 | 目的 |
|---|---|---|
| `uiRender_ui2d_viewerWindowDraw` | llviewerdisplay.cpp:2018 (RenderUIBuffer=true) + 2045 (else) | `gViewerWindow->draw()` = LLView::draw recursion 全体 |
| `uiRender_ui2d_uiScreenComposite` | llviewerdisplay.cpp:2029 (FBO bind + composite triangle strip) | UI screen blit |

#### Group G-2. renderShadow 内訳 (Group F-2 拡張) (7 個 = 4 cascade + 2 projector + 1 dispatch)
| zone 名 | 配線位置 | 目的 |
|---|---|---|
| `renderShadow_sun_<0..3>` | pipeline.cpp:13169 (4 entry static table at L13159) | sun cascade 個別 dt (per-frame は cube_snapshot で 0/1 は 2x 発火) |
| `renderShadow_projector_<0..1>` | pipeline.cpp:13595 (2 entry static table at L13587) | spot light 個別 dt |
| `renderShadow_dispatch` | pipeline.cpp:12519-12548 (j=0/1 inner dispatch loop wrap) | shadow pass の per-pool draw call |

#### Group G-3. renderGeomDeferred 内訳 (2 個 + 22 pool 展開)
| zone 名 | 配線位置 | 目的 |
|---|---|---|
| `renderGeomDeferred_prerender` | pipeline.cpp:5088 (pool prerender loop wrap) | prerender pass 全体 |
| `renderGeom_pool_<TYPE>` | pipeline.cpp:5162 (22 entry static table at L5136) | pool 種別ごとの dt (MATERIALS/TERRAIN/SIMPLE/AVATAR/GLTF_PBR/BUMP/WL_SKY/TREE/GRASS/ALPHA_MASK/GLTF_PBR_ALPHA_MASK 等) |

**実装ノート (dynamic zone name pattern)**:
`AYAPERF_ZONE(name)` macro は string literal 専用 (`__LINE__` concat 都合)。動的 (table-driven) zone 名を取りたい場合は `LLAyastormPerfZone _aya_xxx_zone(const char* name);` を直接 instantiate する。`name` の lifetime は caller 責任 = 必ず `static const char*` table から渡す (heap/stack の char* はダメ)。

```cpp
// pipeline.cpp:5136 (renderGeom_pool 例)
static const char* k_pool_zone_names[LLDrawPool::NUM_POOL_TYPES] = {
    "renderGeom_pool_NONE", "renderGeom_pool_SIMPLE", "renderGeom_pool_GROUND",
    ... (22 entries, 順序は ePoolType enum と完全一致) ...
};
LLAyastormPerfZone _aya_pool_zone(
    (cur_type < (U32)LLDrawPool::NUM_POOL_TYPES)
        ? k_pool_zone_names[cur_type]
        : "renderGeom_pool_OOB");
```

**包含関係 (6 周目 Layer 1 完成形)**:
- `renderShadow` ⊃ {`renderShadow_sun_0/1/2/3` (Σ), `renderShadow_projector_0/1` (Σ), `renderShadow_dispatch` (Σ), `renderShadow_stateSort` (Σ)} + cube face 側 renderShadow も含む
- `renderShadow_sun` (catalog total) ≡ Σ `renderShadow_sun_<0..3>`
- `renderShadow_projector` (catalog total) ≡ Σ `renderShadow_projector_<0..1>`
- `renderGeomDeferred` ⊃ {`renderGeomDeferred_prerender`, Σ `renderGeom_pool_*`} + **未計測 gap** (doOcclusion / setupHWLights / postLoop 候補 = Layer 2 で drill)
- `uiRender_ui2d` ⊃ {`uiRender_ui2d_viewerWindowDraw`, `uiRender_ui2d_uiScreenComposite`}
- `uiRender_ui2d_viewerWindowDraw` = LLViewerWindow::draw 全体 (Layer 2 で floaterView / rootView 等に drill 候補)

---

### §6.6 7 周目 sub-zone カタログ (Group H — Layer 2 全周 / 6 catalog)

6 周目 Layer 1 で明らかになった **renderGeomDeferred gap 6.8 ms / renderShadow_sun_3 5.77 ms 突出 / uiRender_ui2d_viewerWindowDraw 18.6 ms** の 3 箇所を 1 段深く同時 drill (BFS B-drill、memory `feedback_perf_map_bfs_drill.md`)。

#### Group H-1. renderGeomDeferred gap 内訳 (3 個)
| zone 名 | 配線位置 | 目的 |
|---|---|---|
| `renderGeomDeferred_setupHWLights` | pipeline.cpp:5081 (関数冒頭 `setupHWLights()` wrap) | HW light state setup 単独 dt — gap 副犯候補 |
| `renderGeomDeferred_doOcclusion` | pipeline.cpp:5132 (pool while-loop 内 `doOcclusion(camera)` wrap、per-frame 1 回発火) | GPU occlusion query result 集計 — gap 主犯候補 |
| `renderGeomDeferred_postLoop` | pipeline.cpp:5211 (while-loop 抜け後の `gGLLastMatrix / matrixMode / loadMatrix / setColorMask` wrap) | pool dispatch 完了後の GL state restore — gap 残り吸収 |

**包含**: `renderGeomDeferred` ⊃ {`renderGeomDeferred_setupHWLights`, `renderGeomDeferred_prerender`, Σ `renderGeom_pool_*`, `renderGeomDeferred_doOcclusion`, `renderGeomDeferred_postLoop`} + while-loop dispatch overhead (cur_type 判定 / pool skip etc.) — 残 gap が大きい場合 Layer 3 で while-loop body 内側 wrap 候補。

#### Group H-2. renderShadow_sun cascade 内訳 (1 個 + derived 量)
| zone 名 | 配線位置 | 目的 |
|---|---|---|
| `renderShadow_sun_call` | pipeline.cpp:13537 (sun cascade for-loop 内 `renderShadow(view[j], proj[j], shadow_cam, ...)` 呼出 wrap) | renderShadow 関数呼出 単独 dt (4 cascade 分の合計) |

**実装ノート (なぜ setup を直接 wrap しないか)**:
sun cascade for-loop 内で `shadow_cam` / `view[j]` / `proj[j]` 等が **renderShadow 呼出より前** で declared、呼出後で使用される。setup を nested scope で wrap すると `shadow_cam` が scope 外に出てしまい再宣言が必要 = 侵襲的。**呼出側 wrap で代替し、setup phase は分析時に `sun_<j>` 合計 − `sun_call` で導出**。

**包含 + derived**:
- `renderShadow_sun_call` ≡ Σ over j=0..3 of (per-iter renderShadow() call dt)
- **derived**: `renderShadow_sun_setup_total` (4 cascade 合算) = (`renderShadow_sun_0` + `renderShadow_sun_1` + `renderShadow_sun_2` + `renderShadow_sun_3`) − `renderShadow_sun_call`
- per-cascade setup は `renderShadow_sun_<j>` − (per-cascade renderShadow call dt) だが、`sun_call` は cascade 区別なし — 6.8 ms gap 解析では合計値で十分。Layer 3 で必要なら `sun_call_<0..3>` の 4 entry static table 化が次の手。

#### Group H-3. uiRender_ui2d_viewerWindowDraw 内訳 (2 個)
| zone 名 | 配線位置 | 目的 |
|---|---|---|
| `uiRender_ui2d_vwDraw_toolAndOverlays` | llviewerwindow.cpp:3079 (tool draw + mouselook crosshair + drawMouselookInstructions 全体 wrap) | tool / mouselook overlays 描画 dt |
| `uiRender_ui2d_vwDraw_rootView` | llviewerwindow.cpp:3189 (`mRootView->draw()` 呼出 wrap) | 全 UI widget tree 再帰描画 dt — 18.6 ms の主犯候補 |

**実装ノート (gFloaterView->draw は不在)**:
handoff §3.1 では `gFloaterView->draw()` を wrap 候補としていたが、コード調査の結果 **`LLViewerWindow::draw()` 内で `gFloaterView->draw()` は直接呼出されない**。`gFloaterView` は `mRootView` の子で、`mRootView->draw()` の LLView::draw 再帰内で間接的に走る。したがって Layer 2 では root レベルの 2 分割 (toolAndOverlays / rootView) のみで止め、Layer 3 で `mRootView->draw()` 内側の widget tree を drill する場合は `LLView::draw()` か特定 named child (gFloaterView/gToolTipView 等) への配線が必要 = 別軸の侵襲性高い作業。

**包含**: `uiRender_ui2d_viewerWindowDraw` ⊃ {`uiRender_ui2d_vwDraw_toolAndOverlays`, `uiRender_ui2d_vwDraw_rootView`} + 残 = setup / top_ctrl / overlay title / popMatrix 等 (合計数百 us 以下を予想)。

### §6.7 8 周目 sub-zone カタログ (Group I — Layer 3 / 7 catalog)

7 周目 Layer 2 で残った 2 軸の gap を drill:
- `renderShadow_sun_call` 15.45 ms (body Σ) − `stateSort` 8.89 ms − `dispatch` 2.14 ms = **gap 4.42 ms** (renderShadow() body 内に未計上の path がある)
- `uiRender_ui2d_viewerWindowDraw` 13.82 ms − `rootView` 0.70 ms − `toolAndOverlays` 0.003 ms = **gap 13.12 ms** (mRootView->draw 外側 95% が未分解)

3 軸目 `renderGeomDeferred` は Layer 2 で完全分解済 (gap 1.5%) のため Layer 3 対象外。doOcclusion 内訳は 05 spec 打ち手議論で扱う方針。

#### Group I-1. renderShadow body 内訳 (3 個)

renderShadow() 本体は pool dispatch loop を持たない (固定 `types[]` 配列で `renderObjects()` 直接呼出)。代わりに gap は (a) updateCull (b) renderGeomShadow (c) shadow alpha block の 3 path に分かれる。

| zone 名 | 配線位置 | 目的 |
|---|---|---|
| `renderShadow_body_cull` | pipeline.cpp:12496 (RAII GL state 設定後の `updateCull(shadow_cam, result)` wrap) | shadow cull 単独 dt — gap 副犯候補 |
| `renderShadow_body_geom` | pipeline.cpp:12570 (`renderGeomShadow(shadow_cam)` 既存 LL_PROFILE block に AYAPERF_ZONE 追加) | opaque shadow geom 描画 dt — gap 中犯候補 |
| `renderShadow_body_alpha` | pipeline.cpp:12577 (shadow alpha block 全体 = masked + blend + grass + material + GLTF alpha mask 6 sub-block) | alpha/masked shadow 描画 dt — gap 主犯候補 |

**包含**:
- `renderShadow_sun_call` ⊃ {`renderShadow_body_cull`, `renderShadow_stateSort`, `renderShadow_dispatch`, `renderShadow_body_geom`, `renderShadow_body_alpha`} + matrix setup / cube teardown 等 小寄与
- 4 path 合計で `sun_call` 15.45 ms をどれだけ説明できるかが Layer 3 判定基準

#### Group I-2. LLViewerWindow::draw 関数全域 (4 個)

mRootView->draw の **外側** 13 ms gap を 4 path で切り分け。

| zone 名 | 配線位置 | 目的 |
|---|---|---|
| `uiRender_ui2d_vwDraw_setup` | llviewerwindow.cpp:3008 (関数頭 `LLView::sIsDrawing=true` から `gGL.pushMatrix() / LLUI::pushMatrix()` 直後まで) | stop_glerror / matrix init / timecode / gUIProgram.bind / pushMatrix の総和 — 13 ms gap 主犯候補 |
| `uiRender_ui2d_vwDraw_topCtrl` | llviewerwindow.cpp:3207 (`if (top_ctrl && top_ctrl->getVisible())` block 内) | focus 中 top control 描画 dt (focus 不在で 0 us) |
| `uiRender_ui2d_vwDraw_overlayTitle` | llviewerwindow.cpp:3221 (`if (gShowOverlayTitle && !mOverlayTitle.empty())` block 内) | 特殊 overlay title 描画 dt (default OFF で 0 us 想定) |
| `uiRender_ui2d_vwDraw_teardown` | llviewerwindow.cpp:3235 (外側 `}` 後の `LLUI::popMatrix() / gGL.popMatrix() / gUIProgram.unbind() / sIsDrawing=false`) | teardown 総和 — popMatrix / unbind の GL state restore 寄与 |

**包含**: `uiRender_ui2d_viewerWindowDraw` ⊃ {`uiRender_ui2d_vwDraw_setup`, `uiRender_ui2d_vwDraw_toolAndOverlays`, `uiRender_ui2d_vwDraw_rootView`, `uiRender_ui2d_vwDraw_topCtrl`, `uiRender_ui2d_vwDraw_overlayTitle`, `uiRender_ui2d_vwDraw_teardown`} + 小さな未計上 path (`sDebugRects` block 等、通常 false)

**実装ノート (setup zone の push/pop 非対称)**:
`gGL.pushMatrix()` / `LLUI::pushMatrix()` は line 3052-3053 で実行 → 対の pop は line 3232-3233 (teardown 内)。setup zone は push 直後で閉じる (3056 行で `}`) ため、zone scope と GL matrix stack scope は **故意に非対称**。AYAPERF_ZONE は RAII の dt 計測のみで GL state を触らないので問題なし。

**仮説 (setup 13 ms 主犯候補)**:
- `gUIProgram.bind()` (line 3055) ← LLGLSL の internal state 検証 + uniform 再 push が想像以上に重い可能性
- `stop_glerror()` (line 3015) ← `glGetError()` 強制 flush で stall
- `static LLCachedControl<bool> displayTimecode` の get (line 3034)

Layer 3 で setup が 10 ms 以上を占めるなら、上記 3 候補を更に細分化 (Layer 4)。

### §6.8 9 周目 sub-zone カタログ (Group J — Layer 4 / 8 catalog)

8 周目 Layer 3 で確定した 2 軸を更に 1 段深く drill:
- `vwDraw_setup` 13.15 ms (viewerWindowDraw の 94.7%) → **真犯人を 5 path に分割**
- `renderShadow_sun_call` 残 gap 674 us/call (26%) → 3 path に分割 (matrix setup + 内部 doOcclusion + cube teardown)

#### Group J-1. renderShadow body 残 gap 内訳 (3 個)

| zone 名 | 配線位置 | 目的 |
|---|---|---|
| `renderShadow_body_matrixSetup` | pipeline.cpp:12506 (line 12505-12530 = stateSort 直後の `gGL.matrixMode/pushMatrix/loadMatrix × 2 + texunit unbind + LLVertexBuffer::unbind` wrap) | shadow proj/modelview matrix push + GL state setup |
| `renderShadow_body_innerOcclusion` | pipeline.cpp:12567 (dispatch 直後の `if (sUseOcclusion > 1) doOcclusion(shadow_cam)` wrap) | renderShadow() 内部 doOcclusion (sUseOcclusion>1 時のみ発火) |
| `renderShadow_body_cubeTeardown` | pipeline.cpp:12665 (関数末尾の `gDeferredShadowCubeProgram.bind + setColorMask + matrix pop × 2 + sUseOcclusion/sShadowRender 復元`) | cube program bind + matrix teardown + 状態復元 |

**包含 (Layer 4 完成時の sun_call 内訳)**:
- `renderShadow_sun_call` ⊃ {`body_cull`, `body_matrixSetup`, `stateSort`, `dispatch`, `body_innerOcclusion`, `body_geom`, `body_alpha`, `body_cubeTeardown`} = 8 sub-zone 完全分解
- Layer 3 残 674 us/call (26%) のうちどれだけ Layer 4 で説明できるか = Layer 4 判定基準

**dead code 警告**: line 12520-12526 の `struct CompareVertexBuffer` は定義のみで未使用 (`Grep CompareVertexBuffer pipeline.cpp` で確認済)。Layer 4 配線では `body_matrixSetup` zone scope 内に内包 (runtime cost ゼロ、整理は別作業)。

#### Group J-2. vwDraw_setup 内側 5 個 (主犯特定)

13.15 ms gap の真犯人を確定するため、setup block を 5 path に物理分割。

| zone 名 | 配線位置 | 目的 (仮説) |
|---|---|---|
| `uiRender_ui2d_vwDraw_setup_stopGlerror` | llviewerwindow.cpp:3011 (`LLView::sIsDrawing=true + stop_glerror()` wrap) | **GPU sync stall 主犯候補** — `glGetError()` の pipeline flush |
| `uiRender_ui2d_vwDraw_setup_matrixInit` | llviewerwindow.cpp:3019 (`LLUI::setLineWidth + gGL.matrixMode/loadIdentity + sDirtyRect set` wrap) | CPU 側 matrix init、軽い予想 |
| `uiRender_ui2d_vwDraw_setup_displayTimecode` | llviewerwindow.cpp:3036 (`static LLCachedControl<bool> displayTimecode + if 内 renderUTF8` wrap) | displayTimecode は default OFF、static get が初回 1 回のみコスト発生 |
| `uiRender_ui2d_vwDraw_setup_uiProgramBind` | llviewerwindow.cpp:3058 (`gUIProgram.bind() + color4f` wrap) | **LLGLSL state validation 主犯候補** — uniform 再 push + GL state diff 検証 |
| `uiRender_ui2d_vwDraw_setup_pushMatrices` | llviewerwindow.cpp:3067 (`gGL.pushMatrix() + LLUI::pushMatrix()` wrap) | matrix stack push、軽い予想 |

**包含**: `vwDraw_setup` ⊃ {`setup_stopGlerror`, `setup_matrixInit`, `setup_displayTimecode`, `setup_uiProgramBind`, `setup_pushMatrices`} + 5 zone 間の AYAPERF_ZONE overhead (合計 数 us 以下)。

**仮説検証ロジック (Layer 4 計測後)**:
- もし `setup_stopGlerror` が 10 ms 以上を占める → GPU sync stall 確定、打ち手は「`stop_glerror()` を debug only にする」or「frame 中 1 回まで」等
- もし `setup_uiProgramBind` が 10 ms 以上を占める → LLGLSL state validation 重い、打ち手は「shader bind 結果キャッシュ」or「LLGLSL::bind 内部分析」
- 両方とも数 ms 未満なら、5 path 合計 ≠ 13 ms = 仮説外、`AYAPERF_ZONE` の overhead 自体が膨らんでいる可能性 → 観測者効果として別軸検討

**Layer 4 計測結果 (9 周目 = 2026-05-26 active)**: 04 spec §4.2.d 参照。`setup_matrixInit` が **99.7%** (3.56 ms/frame) で単独主犯確定。stopGlerror / uiProgramBind / displayTimecode / pushMatrices は全 < 1 us = 仮説 falsified。**ただし 8 周目 setup 13.15 ms ⇔ 9 周目 setup 3.57 ms の 3.7 倍変動が新規未解決**、原因は Layer 5 で triangulate。

### §6.9 10 周目 sub-zone カタログ (Group K — Layer 5 / 4 catalog)

9 周目 Layer 4 で確定した `setup_matrixInit` 3.56 ms (setup の 99.7%) を **どの GL call が実際に時間を食っているか** 確定するため、4 path に物理分割。同時に 10 周目 計測で 8/9 周目 setup 変動 (13.15 ⇔ 3.57 ms) の triangulate を兼ねる。

#### Group K-1. vwDraw_setup_matrixInit 内側 4 個

| zone 名 | 配線位置 | 目的 (仮説) |
|---|---|---|
| `uiRender_ui2d_vwDraw_setup_matrixInit_setLineWidth` | llviewerwindow.cpp:3020 (`LLUI::setLineWidth(1.f)` wrap) | LL UI line width set、薄い予想 |
| `uiRender_ui2d_vwDraw_setup_matrixInit_matrixMode` | llviewerwindow.cpp:3023 (`gGL.matrixMode(MM_MODELVIEW)` wrap) | LLRender matrix mode 切替、薄い予想 |
| `uiRender_ui2d_vwDraw_setup_matrixInit_loadIdentity` | llviewerwindow.cpp:3025 (`gGL.loadIdentity()` wrap) | matrix stack identity 再 push 候補、薄い予想だが GL state diff 経路あり |
| `uiRender_ui2d_vwDraw_setup_matrixInit_dirtyRect` | llviewerwindow.cpp:3029-3032 (`if (!RenderUIBuffer) { sDirtyRect = ... }` block wrap) | window rect クエリ + LLView 全体 dirty 化、**OS Window size 取得経路の sync stall 候補** |

**包含**: `setup_matrixInit` ⊃ {`setup_matrixInit_setLineWidth`, `setup_matrixInit_matrixMode`, `setup_matrixInit_loadIdentity`, `setup_matrixInit_dirtyRect`} + 4 zone 間の AYAPERF_ZONE overhead。

**仮説検証ロジック (Layer 5 計測後)**:
- 4 zone 中 1 つに集中 → 「GL call 単発が重い」(driver bind 等の固有コスト)、打ち手は「該当 call の頻度削減 / 廃止」
- 4 zone に薄く分散 → 「全体が薄く重い」(sync stall / scheduling)、打ち手は「setup block 全体を describe しない / batch」候補
- 4 zone 合計 ≠ parent matrixInit → `AYAPERF_ZONE` の overhead 自体が膨らんでいる (観測者効果) → Layer 5 で sanity check

**8 ⇔ 9 周目 setup 変動 triangulate** (Group K の副次目的):
- 10 周目 setup parent が 13 寄り / 3.5 寄り / 中間 のどれか → variance 性質 (再現性 / outlier 位置) を判定
- 既存 zone (renderShadow body cull/geom/alpha 等) の 10 周目値が 8/9 周目と整合しているか → 計測 infra session 間ノイズの sanity check

---

## §7. 引継ぎ時 — 即座に確認すべきファイル

| 何を見たいか | ファイル |
|---|---|
| 章の出発点 | `docs/specs/ayastorm-cpu-perf/00-overview.md` |
| main thread workload 全リスト | `docs/specs/ayastorm-cpu-perf/01-mainthread-workload-inventory.md` |
| offload 可能性評価 | `docs/specs/ayastorm-cpu-perf/02-offload-feasibility.md` |
| **infra 実装 + 現状** (このファイル) | `docs/specs/ayastorm-cpu-perf/03-perf-log-infra.md` |
| zone 実装本体 | `indra/llcommon/llayastormperflog.{h,cpp}` |
| cvar gate | `indra/newview/app_settings/settings.xml` (`AYAPerfLogEnabled`) |
| init/shutdown/onFrameEnd 呼出 | `indra/newview/llappviewer.cpp` |
| 配線済 5 zone | §2 表参照 |

---

## §8. 約束されたコミット原則 (引継ぎ Claude 向け)

- **コミットは AYA さんの明示指示があるまでしない** (memory: `feedback_no_auto_commit.md`)
- **検証用ログ / hook は commit 前に外す** (memory: `feedback_remove_verification_logs.md`)
- **push / PR / Release は AYA 側** (memory: `feedback_release_flow.md`)
- 本 perf log infra は **計測用 infra でユーザー機能ではない**。default OFF (`AYAPerfLogEnabled=0`) のため出荷しても害なし。**release note 不要、commit するときは AYA 確認後**。

---

## §9. 既に変更した実コードの一覧 (まだ uncommitted)

```
# infra 本体 (2 周目までで既存)
indra/llcommon/CMakeLists.txt              # llayastormperflog 追加
indra/llcommon/llayastormperflog.h         # 新規
indra/llcommon/llayastormperflog.cpp       # 3 周目: B1 (U64 underflow) fix
indra/newview/app_settings/settings.xml    # AYAPerfLogEnabled 追加

# zone 配線済ファイル (6 周目で合計 13 file / Layer 1 完成 = 44 catalog + Group G 11 catalog = 55 catalog → distinct zone は pool 22 展開で 76)
indra/newview/llappviewer.cpp              # init/shutdown + doFrame_total/idle/updateTextureThreads/meshRepoUpdate/messagePump
indra/newview/llviewerdisplay.cpp          # display/updateCull/fetchQueryResult/hudAttachmentRender/uiRender/swapBuffers + 4 周目: cubeFaceRender_total/stateSort_main/stateSort_cubeFace/stateSort_hud/uiRender_hudElements/uiRender_3d/uiRender_2d + 5 周目: uiRender_renderAll/uiRender_ui2d (split) + 6 周目: uiRender_ui2d_viewerWindowDraw/uiRender_ui2d_uiScreenComposite
indra/newview/pipeline.cpp                 # octreeBalance/drawablesUpdateMove/stateSort/renderShadow/renderGeomDeferred/renderDeferredLighting/atmosphericsHaze/renderFinalize/volumetric/motionBlur/depthOfField + 4 周目: renderShadow_sun/renderShadow_projector/renderShadow_stateSort/stateSort_impostor + 6 周目: renderShadow_dispatch/renderShadow_sun_<0..3>/renderShadow_projector_<0..1>/renderGeomDeferred_prerender/renderGeom_pool_<TYPE×22> + 7 周目 Layer 2: renderGeomDeferred_setupHWLights/renderGeomDeferred_doOcclusion/renderGeomDeferred_postLoop/renderShadow_sun_call + 8 周目 Layer 3: renderShadow_body_cull/renderShadow_body_geom/renderShadow_body_alpha + 9 周目 Layer 4: renderShadow_body_matrixSetup/renderShadow_body_innerOcclusion/renderShadow_body_cubeTeardown
indra/newview/llviewerwindow.cpp           # 7 周目 Layer 2: uiRender_ui2d_vwDraw_toolAndOverlays/uiRender_ui2d_vwDraw_rootView (#include llayastormperflog.h 追加) + 8 周目 Layer 3: uiRender_ui2d_vwDraw_setup/uiRender_ui2d_vwDraw_topCtrl/uiRender_ui2d_vwDraw_overlayTitle/uiRender_ui2d_vwDraw_teardown + 9 周目 Layer 4: uiRender_ui2d_vwDraw_setup_stopGlerror/setup_matrixInit/setup_displayTimecode/setup_uiProgramBind/setup_pushMatrices
indra/newview/llviewerobjectlist.cpp       # idleUpdate
indra/newview/llworld.cpp                  # 3 周目新規: regionIdleUpdate/particleSim
indra/newview/llinventorymodel.cpp         # 3 周目新規: inventoryObserver
indra/newview/llviewertexturelist.cpp      # 3 周目新規: updateImagesCreateTextures/updateImagesFetchTextures
indra/newview/llocclusiongeometrymgr.cpp   # 3 周目新規: occlusionRefresh
indra/newview/llpositionalstreammgr.cpp    # 3 周目新規: stream3DUpdate
indra/llcommon/llcallbacklist.cpp          # 3 周目新規: idleCallbacks
indra/llrender/llimagegl.cpp               # syncToMainThread
indra/llcorehttp/httprequest.cpp           # visitNotifier (3 周目で B2 fix: inner loop に scope 移動)

# 仕様書
docs/specs/ayastorm-cpu-perf/00-overview.md                  # 既存
docs/specs/ayastorm-cpu-perf/01-mainthread-workload-inventory.md  # 既存
docs/specs/ayastorm-cpu-perf/02-offload-feasibility.md       # 既存
docs/specs/ayastorm-cpu-perf/03-perf-log-infra.md            # 3 周目で §6 を地図優先版に書き換え
```

`git status` / `git diff` で実状確認可能。branch は `feature/r31-bugfix-1-credits` で作業中だが、CPU perf 章用に別 branch 切り出しは未実施 (AYA 判断待ち、memory: `feedback_experiment_branch_single_scope.md` 参照)。

---

## §10. キャッシュ系の調査済所見 (重複作業防止用)

AYA さん 2026-05-26 質問「キャッシュ捨てるのも重要、もう分散されてる気もする」に対する調査結果:

| 系統 | クラス | スレッド | per-frame 発火 |
|---|---|---|---|
| **texture cache** | `LLTextureCache : LLWorkerThread` | 専用 worker | purge も worker、自スレッド完結 ✓ |
| **mesh cache** | `LLMeshRepoThread : LLThread` | 専用 thread | fetch + cleanup 同 thread ✓ |
| **VOCache** | 単 singleton | main | per-frame eviction なし、init 時のみ ✓ |
| **asset cache** | `LLAssetStorage` | main + worker | per-frame eviction なし ✓ |

→ **キャッシュ eviction は per-frame hot path に乗っていない**。zone 追加対象から外して良い。
ただし LLTextureCache worker 自体の CPU 占有 (TID 181797 30% 候補) は syncToMainThread zone で間接的に見える + `perf record -g -p <TID>` で symbol-level に取れる。
