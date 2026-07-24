# Phase 3 — フローター外出し(マルチウィンドウ)設計 + 実装計画

> ⚠️ **framing SUPERSEDED(2026-07-24)= `docs/vknative_ui_scene_decouple_design.md`。** 本書は feature-first(「別窓機能を作る」)草案。討議の結論は defect-first(「単一直列パイプラインの UI/scene cadence 溶接を是正 → 別窓は副次生成」)へ転回した。**設計方針・段階計画は新文書が正。** 本書は **Q1-Q6 feasibility 確定 + フローター render-surface 表(§5)+ 各層 file:line 事実**の参照として残置。
>
> 設計者起票 2026-07-24 / branch `feature/ayastorm-phase3-multiwindow`(phase2 `4c9535e9e07` 起点)。
> feasibility spike の後続。全 claim は HEAD の file:line 根拠。Linux 先行・3 OS を意識した抽象で設計。

## 0. 前提・スコープ
- product 目的 = 撮影/設定調整時に一部フローターを**別 OS ウィンドウ**に出し、main viewport を覆わせない。
- Linux(SDL2/XLIB)先行実装。Windows(win32)/ macOS(Metal/MoltenVK)は**同一契約**で後追いできる抽象を置く。
- 別プロセス/別 device は不採用。**同一 VkDevice・同一 UI tree の subtree を第2 swapchain に描く**。

## 1. 現状アーキテクチャの事実(file:line・spike で確定済)
### VK frame/present は単一 window に固定(`indra/llrender/llvkloader.cpp`)
- surface/swapchain = file-static 単数: `sSurface`(110)、`sSwapchain/sSwapchainImages/sSwapchainImageViews/sSwapchainFormat/sSwapchainExtent`(112-116)、depth 一式(577-580)、`sSwapchainClearedThisFrame`(575)、`sAcquiredImageIndex/sImageAcquired`(571-572)。
- frame 同期 = **frame-in-flight 添字のみ**(window 単位でない): `sFrameIndex`(556)、`sCommandBuffers[FRAMES_IN_FLIGHT]`(558)、`sInFlightFences`(561)、`sImageAvailableSemaphores`(565)、`sRenderFinishedSemaphores`(568)。
- `beginFrame`(4828)= `sSwapchain` を直接 acquire(4931)/ `endFrame`(5085)= `sCommandBuffers[sFrameIndex]` end → present target 組み(5460-5482)/ `beginSwapchainRendering`(6550)= 単一 swapchain view を dynamic rendering の color attachment に束縛(6568,6621-6623)。
### 引数化の素地(positive)
- `initSurface(LLWindow* window)`(11773)= **既に window 引数**から surface 生成(格納先が singleton なだけ・11788 `getNativeWindowHandles()`)。Win32/Metal 分岐も同関数内(11804-11832)。
- `PEJob.presents = std::vector<PEPresentTarget>`(731)、`PEPresentTarget` は自前 `swapchain`(711)= present engine は **1 submit 複数 swapchain 対応構造**(現状 5477 で 1 個 push のみ)。
### window 層(`indra/llwindow/`)
- `LLWindowManager::createWindow`(`llwindow.h:318`)= instance factory。`mWindow`(SDL_Window*)・`getNativeWindowHandles`(`llwindowsdl2.cpp:2304`・per-instance `mSDL_Display/mSDL_XWindowID`)は全て per-instance。
- viewer の createWindow 呼出は **1 箇所のみ**(`llviewerwindow.cpp:2069`)。
- ⚠️ `gatherInput`(`llwindowsdl2.cpp:1531`)= `SDL_PollEvent`(1545)でグローバル queue を吸い、`event.window.windowID` で分配せず全部 `this`→`mCallbacks`(1586,1717 等)。**単一 window 前提**。
### UI 描画(`indra/newview/`)
- 単一グローバル frame driver: `llviewerdisplay.cpp:174` `beginFrame()` → scene → `gViewerWindow->draw()`(198)→ `endFrame()`(209)。main loop 側 = `llappviewer.cpp:1772-1810`。
- UI tree = `LLViewerWindow::draw()`(`llviewerwindow.cpp:3036`)が `mRootView->draw()`(3155 付近)を呼ぶ単一 root。フローターは `gFloaterView->addChild(this)`(`llfloater.cpp:364`)。
- UI-RT 分離経路あり: `RenderUIBuffer` で UI を offscreen `gPipeline.mUIScreen` に描いてから swapchain へ blit(`llviewerdisplay.cpp:1917,1949,1952,1962-1969`)= **第2 UI パスの足場**。

## 2. 大枠設計 — per-window `VkWindowContext`
中核工事 = singleton の window 依存状態を **context 構造体**に括り、既存 main を「context #0」として再現。

### 2.1 `VkWindowContext`(新規・llvkloader.cpp 内)
括り出す members(現 singleton → context field):
- surface / swapchain / images / imageViews / format / extent / depth 一式 / clearedThisFrame / acquiredImageIndex / imageAcquired
- 同期: commandBuffers[FIF] / inFlightFences[FIF] / imageAvailableSemaphores[FIF] / renderFinishedSemaphores[FIF] / frameIndex / frameSubmittedMonotonic[FIF] / PE slot state[FIF]
- 対応 `LLWindow*`(surface 再生成・resize 用)

**据え置き(context 化しない)= device/instance/allocator/descriptor/pipeline/mega-buffer/skin SSBO 等の共有資源**(全 window 共通)。thread_local の memo 群(`sLast*` 191-202・`tCmdRecordEpoch` 208 等)は「記録中の cmd」に対する memo ゆえ **context 切替時に必ず invalidate**(§7 段階0 の不変条件)。

### 2.2 API の context 引数化(既定値で後方互換)
- `beginFrame(VkWindowContext&=ctx0, bool acquire=true)` / `endFrame(VkWindowContext&=ctx0)` / `beginSwapchainRendering(VkWindowContext&=ctx0)`。
- 既存呼出(scene 記録 13 site・`beginSwapchainRendering` 各所)は既定 ctx0 で不変 → **Phase 2 の記録経路と衝突しない**(§8)。
- `currentRecordCmd()`(651)は「現在アクティブな context の cmd」を返すよう、thread_local `tActiveWindowCtx` を導入(記録スレッドは 1 context を処理中と仮定=直列)。

### 2.3 present 経路
- 各 context の `endFrame` が自 context の `PEJob`(swapchain/semaphore/fence 自前)を `peEnqueue`。present engine(既に per-target swapchain 対応 711,731)はそのまま。**当面は独立 PEJob 2 本**(main frame + 第2 UI frame)。1-submit 相乗りは最適化として後回し(OPEN-1)。

## 3. 入力 routing 設計(Linux 先行・3 OS 契約)
問題 = `gatherInput` が windowID 無視で全 event を `this` に流す(単一 window 前提)。
### 設計
- **event pump を 1 本化**し windowID→LLWindow を引く: llwindow 層に `LLWindowManager` の `id→LLWindow*` registry を持たせ、`gatherInput` が `event.window.windowID`(既に resize で使用実績 `llwindowsdl2.cpp:887`)/ mouse・key event の `windowID` で対象 LLWindow を判定 → その instance の `mCallbacks` へ dispatch。
- main window 以外は **UI 専用 callbacks**(3D なし・LLView subtree にヒットテスト)。第2 window の LLWindowCallbacks 実装は「対象フローターの LLView に mouse/key を配送」する薄い新規クラス。
- 3 OS 契約: routing の判定境界を **llwindow 層(backend 非依存)**に置き、`llwindowsdl2` が Linux 実装。`llwindowwin32`(WM_ メッセージは hwnd で既に window 判別可)/ `llwindowmacosx`(NSWindow 単位)は同 registry 契約に後追い接続。

## 4. 第2 UI パス設計(floater subtree の孤立描画)
- 第2 context に **UI 専用 RT + swapchain**。3D scene・shadow・probe は**回さない**(pure 2D)。
- 対象フローターは gFloaterView から**論理的に外す**(main draw でスキップ)か、**専用 host view** に移す。第2 window frame で `floater->draw()` を、第2 window の viewport/UI scale/matrix を設定して直接呼ぶ(`LLView::draw` は子を相対座標で再帰=subtree 単独描画可能)。
- per-window UI scale(`mDisplayScale`・`llviewerwindow.cpp:3089`、`ui_scale_factor` `llviewerdisplay.cpp:1942`)は第2 window 用に別途保持(OPEN-3)。
- 駆動点 = main loop の main `endFrame` 後(`llappviewer.cpp:1810` 直後)に第2 context の begin/draw/end を**直列**で回す(Linux-first・main thread)。

## 5. 対象フローター一覧(render-surface 分類・HEAD 確定)
| フローター | 実体クラス / XUI | render-surface | 外出し |
|---|---|---|---|
| phototools | `FloaterQuickPrefs`(`llviewerfloaterreg.cpp:689` / `floater_phototools.xml`) | なし(純 2D コントロール) | ✅ 第一候補 |
| aya_cinematic | `FloaterQuickPrefs`(`llviewerfloaterreg.cpp:694` / `floater_aya_cinematic.xml`) | 純 2D(⚠️ XUI が media/web 語に一致=要 1 行確認 OPEN-4) | ✅ 候補 |
| nearby chat(FS) | `FSFloaterNearbyChat`(`llviewerfloaterreg.cpp:439` / `floater_fs_nearby_chat.xml`・`can_tear_off` 既存) | なし(0 ヒット) | ✅ 候補 |
| nearby chat(LL) | `LLFloaterIMNearbyChat`(`llviewerfloaterreg.cpp:437` / `floater_im_session.xml`) | なし | ✅ 候補 |
| IM session | `LLFloaterIMSession`(`llfloaterimsession.cpp`) | なし(0 ヒット) | ✅ 候補 |
| Preferences | `LLFloaterPreference`(`llviewerfloaterreg.cpp:560`) | なし(`LLSnapshotLivePreview/DynamicTexture` 0 ヒット) | ✅ 候補 |
| **snapshot** | `LLFloaterSnapshot`(`LLSnapshotLivePreview` 保持) | **ライブ 3D プレビュー面あり** | ⛔ 例外・後段(第2 context で preview RT が要る) |

判定法 = 各 .cpp / XUI に対する `LLSnapshotLivePreview / LLViewerDynamicTexture / LLMediaCtrl / LL3D` の出現数 grep(snapshot 以外は 0)。`LLTextureCtrl`(テクスチャ 2D 表示)は render-surface に非該当=外出し可。

## 6. 3 OS 配慮(Linux 先行・後追い契約)
- **surface**: `initSurface` は既に XLIB/Win32/Metal 分岐(11791-11832)= per-window でそのまま。
- **window backend**: Linux `llwindowsdl2` を実装、Win `llwindowwin32` / Mac `llwindowmacosx` は §3 registry 契約 + §2 context を後追い。event routing の判定境界を backend 非依存層に置くのが 3 OS の要。
- **Metal(MoltenVK)**: 第2 NSWindow に CAMetalLayer を貼る必要(macosx backend・後段)。

## 7. 実装計画(段階・各段独立検証)
### 段階 0 — VK singleton → `VkWindowContext`(描画結果不変のリファクタ)
- §2 の members を context 化、全 API に既定 ctx0 引数追加、`currentRecordCmd`/memo 群を context-aware 化。
- **破れてはいけない不変条件** = context 切替時に thread_local memo(pipeline/desc/mv/viewport/vb/ib memo・epoch)を invalidate(汚染防止)。
- gate = 視覚同一 + validation 0(`AYASTORM_VK_VALIDATION=1`)+ 診断起動(`AYASTORM_VKC=1 AYASTORM_PERF_LOG=5`)で全層オラクル沈黙。**A/B 不要な純リファクタだが load-bearing**。

### 段階 1 — 最小 spike: 第2 OS window に単色 present(feasibility 確定点)
- `createWindow` 2 個目 → `initSurface(window2)` → ctx1 で swapchain 生成 → 単色 clear present。入力不要(閉じる SDL 配線のみ)。
- **これが回れば Q1+Q2 の feasibility 確定**(2 surface/2 swapchain/2 present が同一 device で回る)。
- kill switch = `AYASTORM_MW=1`(spike 中の切り分け・gate 後撤去)。

### 段階 2 — 対象フローター 1 枚を第2 window に描く + 入力
- §4 の第2 UI パス + §3 の windowID routing(SDL2)。対象 = phototools 1 枚。
- gate = 第2 window で phototools が操作でき main scene を覆わない + validation 0 + オラクル沈黙。**product 目的の feasibility 確定点**。

### 段階 3 — 実運用配線(登録制 + tear-off トリガ)
- 「外出し可能フローター」を登録し、UI(既存 tear-off 導線)から OS-window 分離をトリガ。候補順 = phototools → nearby chat → IM → preferences → aya_cinematic。
- **snapshot は最後**(preview RT 別 spike)。

### 段階 4 — 3 OS 展開(win32 / macosx backend の routing + surface)+ 掃除(kill switch 撤去)

**最小 spike = 段階 1**(VK/OS-window feasibility 確定)。**product feasibility = 段階 2**。

## 8. Phase 2(per-draw 記録回復)との衝突面
- Phase 2 = draw 記録経路(`llvkbucket.cpp` shadow/scene cmd 積み)。Phase 3 段階0 = frame/present/swapchain の器。**触る面は概ね分離**。
- 重なり = `llvkloader.cpp` 同一ファイルの別領域(テキスト conflict は出るが意味衝突小)。
- **緩和策** = 段階0 の API 変更は**既定値付き引数**で既存呼出を壊さない(Phase 2 の `sCommandBuffers[sFrameIndex]` 前提を温存)。マージ時は llvkloader.cpp の手動 conflict 解決前提。

## 9. OPEN / 申告欄
### OPEN(未確定・要トレース or product 判断)
1. **present 同期モデル** = 第2 context を独立 PEJob で回すか 1-submit 相乗り(711,731)か。解像度/リフレッシュ差時の acquire タイミング分離が未トレース。当面独立 PEJob。
2. **thread_local memo の context 跨ぎ**(186-220)= frame-in-flight 添字のまま 2 context で成立するか、context 切替 invalidate が要るか(段階0 で実トレース)。
3. **per-window UI scale/DPI**(`mDisplayScale` 3089・`ui_scale_factor` 1942)= 第2 window の独立 scale 未設計。
4. **aya_cinematic XUI の media/web 語一致**(§5 ⚠️)= 実 render 依存か label 語か 1 行確認。
5. **snapshot preview**(段階3 例外)= 第2 context で 3D preview RT を回す設計は本 spike スコープ外。
### 申告(縮小・省略・解釈)
- **Linux 先行**: 段階1-3 は SDL2 のみ実装。win32/macosx は契約設計のみ・実装は段階4(product 判断で前倒し可)。
- **解釈**: 「外出し」= 同一 device・同一 UI tree の subtree を第2 swapchain に描く(別プロセス不採用)。別解釈なら再設計。
- **やらない**: 段階0 の具体 signature 詳細・context 構造の field 単位確定は着手 Brief で提示(本書は大枠)。実ビルド/起動なし(readonly 調査)。
