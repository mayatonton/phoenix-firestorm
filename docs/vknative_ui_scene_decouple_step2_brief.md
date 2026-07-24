# Step 2 実装 Brief — L1(Producer/Consumer 分割)

> 設計者発行 2026-07-24。設計 = `vknative_ui_scene_decouple_design.md` / 接地 = `vknative_ui_scene_decouple_step1_trace.md`。
> **座組 = 既定(設計者が Brief 発行・実装は Fresh 並走者・設計者が突合 gate)。** 実装は AYA 承認後。
> **serial 実装 = 1 slice ずつ(2a → gate → 2b → gate → 2c)。** 各 slice で build + gate を通してから次へ。

## 共通ルール(全 slice)
- kill switch = `AYASTORM_UISCENE`(0=旧経路そのまま=切り分け足場・gate 後撤去)。slice ごとに段階的に効かせる。
- read before edit / 機械置換禁止 / 新規コメント追加禁止(CLAUDE.md)。
- build = `cd build-linux-x86_64 && make -j20 ayastorm-bin`・判定 = `grep -c error:` + `[100%] Built target`。.h 改修ありゆえ **フルビルド**。
- gate = **視覚同一(scene)+ validation 0(`AYASTORM_VK_VALIDATION=1`)+ 診断起動(`AYASTORM_VKC=1 AYASTORM_PERF_LOG=5`)で全層オラクル沈黙**。agent は PASS 宣言不可(憲法)= 報告は VERIFIED(file:line)/CLAIMED/OPEN + 申告。

---
## Slice 2a — presentable RT 間接化(挙動同一・submit は 1 本のまま)
**狙い**: 「Producer 出力を presentable RT へ書き、Consumer が blit で swapchain へ」の配管を、**frame pacing を一切触らず**に通す。挙動完全同一を gate。

### 変更点
1. **presentable RT 新設**(`pipeline.h`/`pipeline.cpp`): `LLRenderTarget mScenePresentRT`(LDR・worldview full res)を追加。allocate/release は `mUIScreen`(`pipeline.cpp:1171,1664`)と同型・同解像度追従(`allocateScreenBuffer` 経路)。
   - ⚠️ **`mVkSnapshotRedirectTarget` を流用しない**(snapshot 意味と衝突)。**専用の `LLRenderTarget* mScenePresentRedirect = nullptr`** を新設(既存 redirect と同型・`pipeline.h:882` の隣)。
2. **renderFinalize 終端の redirect**(`pipeline.cpp:11018`): 現 `if (mVkSnapshotRedirectTarget) …bindTarget() else beginSwapchainRendering()` を、**`mScenePresentRedirect` も分岐に加える**(優先順 = snapshot > scenePresent > swapchain)。`AYASTORM_UISCENE` 有効時のみ `mScenePresentRedirect = &mScenePresentRT` を display 側で set。flush 側(11052)も対応。
3. **Consumer blit 挿入**(`llviewerdisplay.cpp` render_ui 直前 or renderFinalize 直後 = 1097 の前): `AYASTORM_UISCENE` 有効時、**`beginSwapchainRendering()` + fullscreen triangle で `mScenePresentRT` を swapchain へ blit**(既存 `mScreenTriangleVB` + 単純 copy shader・`gDeferredPostNoDoFNoiseProgram` 相当の pass-through)。その後は現行どおり UI overlay(render_ui_3d/2d)が swapchain 上に乗る。
   - 旧経路(switch off)= renderFinalize が直接 swapchain へ(現状)。

### 非スコープ(2a でやらない)
- frame 分割・async・double-buffer・fence 変更(2b/2c)。**submit は 1 本のまま**。

### gate(2a)
- `AYASTORM_UISCENE=1` と旧(=0)で**視覚同一**(blit は pass-through ゆえ画素同一が期待値)。validation 0・オラクル沈黙。
- 申告: blit 1 pass ぶんの GPU 増(full res 1 枚 copy = 微小)。mScenePresentRT の layout 遷移(color→sampled)を正しく張る。

---
## Slice 2b — submit を Producer/Consumer の 2 本に分割(同期・挙動同一)
**狙い**: 記録・submit を 2 系に割る。**まだ async でない**(同一 iteration 内で Consumer が Producer 完了後に走る)。

### 変更点
- `llvkloader` の frame 記録を、**Producer CB(deferred+lighting+renderFinalize→mScenePresentRT)** と **Consumer CB(blit+UI+present)** に分ける。当面は 1 iteration で Producer submit → Consumer submit を直列(Consumer は Producer を semaphore で待つ or 同 CB→2 submit)。
- `beginFrame/endFrame`(`llvkloader.cpp:4828/5085`)を「begin(role)/end(role)」に一般化 or Producer/Consumer 用の begin/submit を新設(既存 single-frame path は switch off で温存)。
- **Phase 2 非干渉**: Producer 側は既存 scene 記録(renderGeomDeferred 等 = Phase 2 が触る領域)を**そのまま内包**。変えるのは submit の括りのみ。

### gate(2b)
- 視覚同一・validation 0・オラクル沈黙。2 submit frame が devlost/VUID 無しで回る。fence 追跡(Producer/Consumer 各 fence)が reap と整合。

---
## Slice 2c — Producer async + double-buffer + swap(**本質達成の挙動変化**)
**狙い**: Consumer を Producer から解き放つ。UI cadence が scene GPU 律速から外れる。

### 変更点
- `mScenePresentRT` を **double-buffer(front/back)** 化 + `LLPipelineFrameContext` に front/back selector(`getActiveRT()` 間接参照が 40 site を自動解決 = Step1 §1a)。
- **Producer = async**: 自 fence を poll(非ブロッキング)。前 scene 完了時のみ back へ記録・submit。完了 fence 観測(CPU)で **back→front swap**(GPU semaphore 不要 = Step1 OPEN#2 解決)。busy なら skip(front 再利用)。
- **Consumer = 毎 iteration**: front を blit + UI + present。**Producer fence を待たない**。scene fence hard-wait(`llvkloader.cpp:4904`)を Consumer 経路から除去。
- pacing 上限 = Producer 1-in-flight(自己 pacing)/ Consumer 自 fence + vsync。CPU 暴走防止の上限確認(OPEN#4・実測で確定)。
- kill switch 追加 = `AYASTORM_UISCENE_ASYNC`(0=2b 同期に fallback)。

### gate(2c)= 本質達成
- **scene を意図的に遅く**(重シーン or 人工 stall)した状態で、**UI(toolbar/chat)の present が display レートで滑らかに回り続ける**ことを計器(VkPerf の consumer/producer 別 fps)+ 体感で実証。
- scene content は視覚同一・validation 0・全層オラクル沈黙・devlost 無し。

---
## OPEN(実装中に確定・申告必須)
- **#5 3D HUD/attachments 帰属**(`render_ui_3d`・`gGLLastModelView` snapshot 使用): 設計者裁定 = **Consumer 側**(snapshot 行列で毎 present 再実行)。2b で配置確定。実装者は実挙動を申告。
- **#4 Consumer→Producer 先行上限**: 2c で実測して固定。
- **double-buffer の image view 登録**: front/back 両 RT の view を bindless/descriptor 側で active 解決(2c)。
- **layout 遷移**: mScenePresentRT の color↔sampled 遷移(2a から)。

## 設計者の裁定(申告に記録済)
- presentable RT = **LDR**(現 renderFinalize final と同一 = 品質トレード無し)。
- renderFinalize 全体 = **Producer 側**(scene 依存 post を分割しない)。
- 3D HUD = **Consumer 側**。
- **やらないこと**: L2(別スレッド)/ Snapshot-camera / world snapshot 契約 / Chat logic-offload / multi-window(Step3)。本 Brief は単一窓の L1 のみ。
