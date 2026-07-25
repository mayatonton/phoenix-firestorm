# Step 1 トレース報告 — L1(Producer/Consumer 分割)接地

> 設計者 2026-07-24 / 設計 = `docs/vknative_ui_scene_decouple_design.md`。readonly トレースのみ(コード変更なし)。
> 結論 = **L1 は既存の間接参照・既存フックに強く支えられており tractable**。OPEN 5 項中 3 項を解決、2 項を Step2 送り。

## 1. 決定的な足場(既存・positive)
### 1a. `LLPipelineFrameContext` + `getFrameRT()` 間接参照 = RT swap の中央集権点
- `getFrameRT()` = `LLPipelineFrameContext::getInstance().getActiveRT()`(`pipeline.cpp:479-482`)。`mActiveRT`(RenderTargetPack ポインタ)+ `setActiveRT` + `ScopedActiveRT`(`llpipelineframecontext.h:38-39,64-73`)。
- **scene RT 参照(`getFrameRT()->screen` 等 = 40 site)が全て間接参照経由** → **double-buffer = RenderTargetPack をもう 1 本足して active ポインタを swap するだけで 40 site 全てが active 側に解決する**(散在する raw ポインタなし)。
- `RenderTargetPack`(`pipeline.h:852-865`)= screen / deferredScreen / deferredLight / shadow[4]。既に mMainRT / mAuxillaryRT(probe)/ mHeroProbeRT の 3 本が存在し active を切替える運用 = **pack 差し替えは実績のある操作**。

### 1b. `mVkSnapshotRedirectTarget` = final 出力を swapchain→RT へ差し替える既存フック
- `renderFinalize` 終端(`pipeline.cpp` ~11021)= `mVkSnapshotRedirectTarget ? mVkSnapshotRedirectTarget->bindTarget() : LLVKLoader::beginSwapchainRendering()`。
- ∴ **Producer は final を swapchain でなく "presentable scene RT" へ書く配管が既にある**(snapshot 用の redirect を Producer が転用)。

### 1c. present は既に off-main / present engine は複数 target 対応
- `peExecute`(`llvkloader.cpp:765`)= PE スレッドで submit+present。`PEJob.presents` 複数 target loop(731,838)。

## 2. Producer / Consumer 境界(file:line 確定)
main `display()`(`llviewerdisplay.cpp:480`):
- **Producer(重い・scene 依存)**: shadow → occlusion(1047-1053)→ `renderGeomDeferred`(1059)→ `renderDeferredLighting`(1082)→ **`renderFinalize`(post 全連鎖)**。`renderFinalize`(`pipeline.cpp:10768`)= alpha plate composite / SSR(10842)/ luminance(10844)/ exposure / tonemap(10851)/ CAS / glow(10866)/ volumetric / DoF / FXAA / SMAA → final。**全て scene 依存 = Producer 側が正**。出力 = presentable RT(1b の redirect 経由)。
- **Consumer(安い・毎 present)**: **presentable RT を blit + UI overlay(`render_ui_3d/2d` `llviewerdisplay.cpp:1629-1653`)→ swapchain → `swap`(1102)**。
- **seam = 「完成した presentable scene 画像」**。Producer は自 CB/fence で async、Consumer は自 CB/fence で毎 iteration・**Producer fence を待たない**。

## 3. fence hard-wait 解体(核心)
- 現状 = `beginFrame`(`llvkloader.cpp:4828`)が単一 frame fence を hard-wait(4904)し display() 全体(producer+consumer)を 1 CB/1 submit で括る。
- L1 = **begin/submit を 2 系に分割**:
  - **Consumer begin**: 自 fence(fast)を待つ・swapchain acquire・blit+UI 記録・submit・enqueue present。**毎 iteration**。
  - **Producer begin**: 自 fence を poll(非ブロッキング)・前 scene 完了なら deferred+lighting+finalize を presentable RT(back)へ記録・submit。完了で back→front swap(`setActiveRT` 相当のポインタ入替)。**scene レート**。
- Consumer は Producer を待たない = **UI cadence が scene GPU 律速から外れる**(= 本質達成)。

## 4. OPEN 解決 / 残
| # | 項目 | 状態 |
|---|---|---|
| 1 | presentable RT の double-buffer 可否 | **解決(viable)**: 1a の間接参照 + 1b の redirect フック。追加 = presentable RT 2 本(LDR full-res ~2 画面 VRAM)。risk = consumer が blit で sample する image view を active 側で解決(2 本登録)。 |
| 2 | Producer→Consumer の GPU 順序保証 | **解決**: back→front swap を **CPU 側で Producer fence signal 観測後**に限れば、Consumer は常に GPU 完了済 front を読む → **handoff に GPU semaphore 不要**(layout 遷移は別途)。 |
| 3 | tonemap/post の producer/consumer 帰属 | **解決**: renderFinalize 全体を Producer 側(scene 依存)。Consumer = blit + UI のみ。renderFinalize は分割しない。 |
| 4 | fence 解体時の CPU 暴走防止(新 pacing) | **概ね解決**: Producer は 1-in-flight(poll 自己 pacing)、Consumer は自 fence + vsync 自己 pacing。上限は自然に bounded。詳細上限値は Step2。 |
| 5 | 3D HUD/attachments(`render_ui_3d`・`gGLLastModelView` snapshot 使用)の帰属 | **Step2 送り**: scene-locked UI。Consumer 側(snapshot 行列で毎 present 再実行)で成立するが、producer 側の方が安い可能性。実装時に確定。 |
| 6 | 入力 windowID routing 3OS(Step3 用) | Step3 送り(L1 に不要)。 |

## 5. Floater Render Unit 境界(Step3 用・予備トレース)
- floater draw = `mRootView->draw`(`llviewerwindow.cpp:3155`)→ 再帰。単一 floater は `gFloaterView`(`llfloater.cpp:364`)配下の LLView subtree。
- L1 では Render Unit は Consumer と同じく「presentable 画像 + UI」の枠組みで、**対象 floater の subtree のみを別 surface の Consumer が描く**。隔離 render context(専用 CB/UBO/matrix/descriptor)の詳細は Step3 で確定(L1 では不要)。

## 6. Step 2 に渡す L1 の具体形(実装 = AYA 承認後)
1. `RenderTargetPack`(または presentable 用の LDR RT)を **2 本 + active/front ポインタ**化(LLPipelineFrameContext に front/back or presentable selector を追加)。
2. `renderFinalize` の終端を **presentable RT(back)へ redirect**(`mVkSnapshotRedirectTarget` 機構を Producer 用に一般化)。
3. frame driver(`llvkloader` begin/end + `llappviewer`/`llviewerdisplay` の駆動)を **Producer submit(async・自 fence)/ Consumer submit(毎 iteration・自 fence・presentable front を blit + UI + present)** に分割。
4. Producer 完了 fence 観測で front swap。
5. **単一窓 gate**: scene を意図的に遅くして UI(toolbar/chat)が display レートで滑らかに回ることを計器 + 体感で実証(視覚同一 + validation 0 + オラクル沈黙)。

## 申告(縮小・省略・解釈)
- readonly トレースのみ。実測ベンチ無し(AYA 方針 = 体感価値で自明・Chat logic 計測は不作)。
- OPEN 5(3D HUD 帰属)/ 4 の pacing 上限値 / double-buffer の image view 登録詳細 = 実装(Step2)で確定。「書いてない」でなく「Step2 で決める」。
- presentable RT を「LDR 1 枚」と仮置き。HDR のまま持つ(consumer で tonemap)か LDR で持つかは Step2 の VRAM/画質判断(現状は renderFinalize が LDR final を作るので LDR 案)。
