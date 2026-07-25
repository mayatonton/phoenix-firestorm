# UI/Scene cadence 分離(単一直列パイプライン是正)= 本質設計 / 副次成果 = フローター別窓

> 設計者起票 2026-07-24(指揮官との本質設計討議の収束)。branch `feature/ayastorm-phase3-multiwindow`。
> **統治文書 `docs/vknative_recovery_plan.md` の「本体回復」に位置づく本質設計。** 旧 `docs/vknative_phase3_multiwindow_design.md`(feature-first 草案)を supersede。
> 全 claim は HEAD の file:line 根拠。推測禁止・不変条件ファースト。

## 0. 設計目的(framing = defect-first, not feature-first)
**目的 = 本質欠陥「単一直列パイプラインが UI の present/描画 cadence を scene GPU 律速に溶接している」の是正。** フローター別窓・別ディスプレイ最大化は、欠陥を治した結果として**生成される副次ケース**であって設計の駆動因ではない。
- **価値 = 速度(throughput)ではなく体感速度・滑らかさ**(AYA 確定)。scene が遅くても UI が display レートで滑らかに応答する状態を作る。throughput は増えない。
- 参照 = [[project_cancer_single_main_thread_pipeline]] / [[project_northstar_crowd_ui_never_freezes]] / [[finding_lockstep_dvfs_ceiling]]。

## 1. 核心欠陥(不変条件違反)+ HEAD 事実(file:line)
**破れている不変条件**: 「UI の present/描画 cadence は scene 描画(GPU)cadence から独立でなければならない」。
- 現状 = 1 iteration に 1 つの paced frame: `beginFrame`(`llvkloader.cpp:4828`)が **scene fence を hard-wait**(`vkWaitForFences` 4904)→ loop 全体が scene GPU 律速。`display()`(`llviewerdisplay.cpp:174`)で scene→UI を同一 CB に記録し `endFrame`(209)で 1 submit。
- ∴ scene が 15fps なら UI(chat のスクロール・カーソル・入力反応)も 15fps に引きずられる。

## 2. 分離の正体(honest)= present 分離 vs content 分離・MainThread 依存 2 本
render は MainThread に 2 つ依存する:
1. **データ snapshot 依存**: 描く元データ(Chat = 自 LLView tree 状態)は MainThread 所有。
2. **GPU worker 依存**: 描画結果の submit/受取。

- **present 分離**(本設計 L1 で達成) = 最後に合成した像を display レートで再提示。窓が生存・滑らか。
- **content 分離**(camera 再投影等) = stale snapshot から新フレーム生成。**Chat には不要**(§3)。
- Chat の data 依存は event-driven で安く、MainThread が固まる時はそもそも誰も中身を作れない = 許容。
- HEAD の既存 micro-snapshot 証跡 = `render_ui` が live camera でなく `gGLLastModelView`(スナップ行列)を load(`llviewerdisplay.cpp:1593-1594`)/ torn-read 病 = [[handoff_skeleton_offmain_race_class1_class2_resolved]] Class2。

## 3. スコープ(剪定確定)
- **対象 = Chat-class(純 2D・scene/camera データ無し)**: nearby chat(第一検体)/ IM session / phototools コントロール / preferences / aya_cinematic。
- **camera に触りたいのは Snapshot floater だけ**、かつ Snapshot 使用時は **GPU フル回転 regime** = Chat と別世界 → **別トラックへ park**。**world/camera snapshot 契約はスコープ外**。
- **Chat の logic 負荷計測は不要**(scene 比で微小・判断が変わらない・AYA 確定)。

## 4. アーキテクチャ
### 4.1 seam(接地・file:line)
重い scene は**オフスクリーン HDR RT** へ: `getFrameRT()->screen`(deferred)。tonemap/post/UI は安い。
- `renderFinalize`(`pipeline.cpp:10768`)= `getFrameRT()->screen` を読み(10795)tonemap(10851)/gamma(10861)→ swapchain。UI overlay(`render_ui_3d/2d` `llviewerdisplay.cpp:1629-1653`)がその上。
- **`getFrameRT()` = frame 単位 RT selector が既存**(`allocateScreenBuffer` `pipeline.cpp:1021`)= **double-buffer の素地**(寿命/fence 追跡は要トレース = §10 OPEN)。
- ∴ seam = **「deferred → HDR scene RT(重い・GPU 律速)= Producer」** と **「tonemap(最新 scene RT)+ UI → swapchain → present(安い)= Consumer」**。

### 4.2 L1(単一スレッド・present 分離)= 本質達成の中核
frame driver を Producer/Consumer に割る(**別スレッド不要**):
```
毎 iteration(安い・display cadence):
  Consumer:
    acquire swapchain
    record: tonemap(最新完成 scene RT) + fresh UI overlay → swapchain
    submit(Consumer CB + Consumer fence) ← scene fence を待たない
    enqueue present(既存 PE スレッド)
  Producer(async・自 fence・毎回でなくてよい):
    if 前 scene 完了(producer fence を poll):
       renderGeomDeferred → scene RT(N-buffer: back へ書き完了で front へ swap)
       submit(Producer CB + Producer fence)
    else: skip(現 front を再利用)
```
- **肝** = Consumer は Producer fence を hard-wait しない。現 front(最後に完成した scene RT)を tonemap して UI を重ね present。scene は非同期。
- **主要 benefit は main 窓 UI にも即出る**(scene 遅くても toolbar/chat が滑らか)= **単一窓で L1 gate 可**(multi-window 前でも実証できる)。
- tonemap/post は Consumer 側(producer = 生 deferred → HDR RT / consumer = tonemap+UI+present)。

### 4.3 L2(別スレッド・後続 phase・park)
migrated floater を UI Thread へ。**gGL 全体の thread-safe 化はしない** → **floater 単位 render 隔離**(§5)。ここで初めて指揮官の「GPU worker 受渡」難関 = **semaphore + N-buffer RT の GPU 側受渡**(CPU readback 無し・queue 集約点 = 既存 PE スレッド + `sSwapchainAccessMutex`)が中心になる。

## 5. Floater Render Unit(load-bearing 定義)
**境界 = 「LLView subtree + 自己完結 render context(専用 command buffer・専用 per-frame UBO・専用 matrix 状態・専用 descriptor)+ surface + present cadence」**。
- **1 枚ずつ migrate**: main UI tree(`mRootView->draw` `llviewerwindow.cpp:3155`・floater は `gFloaterView->addChild` `llfloater.cpp:364`)から抜き、Consumer 経由で自 surface に present。
- **nearby chat = 第一検体**(最小・scene/camera 無し)。以降 phototools→IM→preferences→aya_cinematic を同型で。
- L1 では unit は main スレッド上(隔離 context だが同スレッド)。L2 で**同じ unit をそのまま別スレッドへ載せ替え**(境界を隔離で切ってあるので作り直さない)。
- **概念は "Logic 同居可能な自己完結 unit" として広めに取る**(将来 logic-offload で境界を引き直さないため)。ただし **logic-offload は build しない**(§3・微小)。

## 6. VK present 層(per-window の最小 context)
Consumer が複数 surface に present するための per-window 状態。**物理強制ぶんのみ** context 化:
- per-window = surface(`sSurface` 110)/ swapchain 一式(112-116)/ depth(577-580)/ acquire 状態(571-572)/ imageAvailable・renderFinished semaphore(565/568)/ recreatePending。
- 共有据え置き = frameIndex(556)/ per-frame ring(179)・UBO(166)/ device / pipeline / bindless / mega-buffer / skin SSBO / **PE スレッド(present は既に off-main)**。
- present engine は既に `PEJob.presents` 複数 target loop(`llvkloader.cpp:731,838`)。submit を multi-signal 化(rf 複数)+ `sSwapchainRecreatePending` を per-window 化。
- window 層 = `LLWindowManager::createWindow`(`llwindow.h:318`)は instance factory・`getNativeWindowHandles`(`llwindowsdl2.cpp:2304`)per-instance。**入力は windowID routing 新設**(`gatherInput` `llwindowsdl2.cpp:1531` は現状 windowID 無視)。`initSurface(LLWindow*)`(11773)は既に window 引数化(XLIB/Win32/Metal 分岐)。

## 7. 検証 / gate
- **L1 gate(単一窓)**: scene を意図的に遅くした状態で、**UI(toolbar/chat)の present が display レートで回り続ける**ことを体感 + 計器で実証。加えて **視覚同一(scene)+ validation 0(`AYASTORM_VK_VALIDATION=1`)+ 診断起動で全層オラクル沈黙**(CLAUDE.md gate 基準)。
- **Render Unit gate**: nearby chat を自 surface に抽出し、main scene を覆わず・別ディスプレイ最大化・**親最小化中も生存**(main loop は最小化中も回る `llappviewer.cpp:1762`)。
- ⚠️ 視覚は最終 gate のみ・判定オラクルにしない(AYA 厳命)。

## 8. 実装計画(段階・各段独立検証)
- **Step 1(トレース・非ベンチ)**: L1 設計接地 = `getFrameRT()` double-buffer 可否(RT 寿命・fence 追跡)/ scene fence hard-wait(4904)の解体点 / Producer-Consumer 分割の CB・fence 割り / Floater Render Unit 境界の詳細確定。**Chat logic 計測は含めない。**
- **Step 2(L1 本体)**: frame driver を Producer(async scene→N-buffer HDR RT)/ Consumer(tonemap 最新 + UI → present)に分割。**単一窓で UI present が scene と別 cadence で回ることを実証**(= 本質達成)。
- **Step 3(Render Unit + multi-window)**: nearby chat を自 surface に抽出 = Consumer を N-surface 対応化 + windowID 入力 routing。**別窓・別ディスプレイ最大化・親最小化耐性**。
- **Step 4+**: 同型で floater を 1 枚ずつ / L2(UI Thread・GPU worker 受渡)/ Snapshot-camera = 各々別 phase。

## 9. Phase 2(per-draw 記録回復)との衝突面
- Phase 2 = draw 記録(`llvkbucket.cpp` shadow/scene cmd)。本設計 = frame driver 構造 + present。**触る面は概ね分離**。
- 重なり = `llvkloader.cpp` 同一ファイル別領域 + `beginFrame/endFrame` の構造変更。**緩和 = Producer/Consumer 分割は既存 scene 記録経路(Phase 2 が触る)を Producer 側にそのまま内包**し、変えるのは「fence 待ちと present の括り」= Phase 2 の記録内部に手を入れない設計にする。

## 10. OPEN / 申告欄
### OPEN(要トレース・Step 1 で潰す)
1. `getFrameRT()` の N-buffer 化 = RT 寿命 / consumer 読み中に producer が再利用しない fence 追跡。
2. Consumer→Producer 間の GPU 順序保証(front swap を CPU で fence-signal 後に限れば semaphore 不要か、要る場合の張り方)。
3. tonemap/post を Consumer 側へ移す時の依存(SSR `pipeline.cpp:10842`・luminance 10844 等が producer/consumer どちらに属すか)。
4. scene fence hard-wait 解体時の CPU 暴走防止(producer が consumer に何 frame 先行してよいかの上限 = 新 pacing)。
5. per-window 入力 routing の 3 OS 契約(SDL2 先行・win32/macosx 後追い)。

### 申告(縮小・省略・解釈)
- **L1 単一スレッド先行**: 本質(present 分離)を単一スレッドで達成し、L2(別スレッド・GPU worker 受渡)は後続 phase に park。
- **content 分離(camera 再投影)= やらない**(Snapshot 専用・別トラック)。**world/camera snapshot 契約 = 作らない**。
- **Chat logic-offload = build しない**(微小・境界は将来同居可能に取るのみ)。
- **解釈**: 「分離」= present/描画 cadence を scene から外すこと(データ生成の MainThread 所有は残す)。「Floater Render Unit」= 同一 device・隔離 render context の自己完結単位(別プロセス/別 device でない)。
- **やらなかった**: 実ビルド/起動なし(設計段階)。Step 1 のトレース詳細・CB/fence 割りの確定は Step 1 成果物。実装・計器は AYA 承認後。
