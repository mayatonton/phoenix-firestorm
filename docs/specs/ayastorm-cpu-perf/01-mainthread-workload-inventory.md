# 01 — Main thread workload inventory (徹底棚卸し)

main thread の per-frame ループに乗っている処理を、**実コード trace で** 上から下まで全列挙する。
推論で埋めない。確認できた行 (file:line) を必ず添える。未確認は「未確認」と明示。

入力 entry point: `LLAppViewer::mainLoop()` → `LLAppViewer::doFrame()` (`indra/newview/llappviewer.cpp:1568`)

## 表の読み方

各セクションは以下のカラムで埋める。

| 系統 / 処理名 | entry (file:line) | 呼出頻度 | 同期前提 | 由来 (LL/FS/Aya) | main 占有理由 | offload 可能性 (一次判断) |

- **呼出頻度**: `per-frame` / `per-tick(N Hz)` / `event` / `idle-cycle` / `on-click`
- **同期前提**: `GL context 必須` / `order-dependent` / `非同期可` / `barrier 必要` / `sync` / `async`
- **由来**: `LL` (Linden 由来) / `FS` (Firestorm 由来) / `Aya` (AYAstorm 追加。release 番号併記)
- **offload 可能性 (一次判断)**: `thread pool` / `async job` / `batch` / `cull (削減)` / `main 残置必須` / `未判定` / `既に offload 済`
  - **ここはあくまで一次判断**。詳細評価は `02-offload-feasibility.md` で実コードと制約を見直して再評価する。

---

## §1. mainLoop() の per-frame 上層構造

`LLAppViewer::doFrame()` (`indra/newview/llappviewer.cpp:1568`) の per-frame 直下に並ぶ呼出。`idle()` / `display()` の中身は §2, §3 で別扱い。

| 系統 / 処理名 | entry (file:line) | 呼出頻度 | 同期前提 | 由来 | main 占有理由 | offload 可能性 |
|---|---|---|---|---|---|---|
| Frame Recording (Perf Stats 開始) | llappviewer.cpp:1597 | per-frame | sync | LL | profiling instrument | main 残置必須 |
| ETW Frame Logging | llappviewer.cpp:1631 | per-frame | sync | FS/Aya | ETW tracing (Windows only) | main 残置必須 |
| LLTrace BlockTimer Update | llappviewer.cpp:1639-1640 | per-frame | sync | LL | frame-level stats collection | main 残置必須 |
| Thread Recorder Pull | llappviewer.cpp:1643 | per-frame | sync | LL | inter-thread perf data sync | main 残置必須 |
| Callstack Clear | llappviewer.cpp:1646 | per-frame | sync | LL | debug profiling state | main 残置必須 |
| processMiscNativeEvents | llappviewer.cpp:1657 | per-frame | sync | LL | OS window event pump (Linux: SDL2) | main 残置必須 |
| gatherInput | llappviewer.cpp:1673 | per-frame | sync | LL | input aggregation (keyboard / mouse / joystick) | main 残置必須 |
| mainloop Event Post | llappviewer.cpp:1691 | per-frame | sync | LL | event-pump entry trigger | main 残置必須 |
| llcoro::suspend | llappviewer.cpp:1697 | per-frame | sync | LL | coroutine scheduler dispatch | main 残置必須 |
| scanJoystick | llappviewer.cpp:1722 | per-frame (条件付) | async | LL | joystick state sample | thread pool |
| scanKeyboard | llappviewer.cpp:1723 | per-frame (条件付) | async | LL | keyboard state sample | thread pool |
| scanMouse | llappviewer.cpp:1724 | per-frame (条件付) | async | LL | mouse state sample | thread pool |
| **idle()** | llappviewer.cpp:1748 | per-frame | sync | LL | message / network / simulation tick | main 残置必須 (詳細 §3) |
| **display()** | llappviewer.cpp:1780 | per-frame | GL context 必須 | LL | GPU rendering pipeline | main 残置必須 (詳細 §2) |
| Reflection Map Update | llappviewer.cpp:1787 | per-frame (条件付) | GL context | LL | post-render reflection capture | batch |
| FloaterSnapshot Update | llappviewer.cpp:1788 | per-frame (条件付) | GL context | LL | user-requested screenshot | async job |
| FloaterFlickr Update | llappviewer.cpp:1790 | per-frame (条件付) | GL context | FS | Flickr export | async job |
| PrimFeed Update | llappviewer.cpp:1791 | per-frame (条件付) | sync | Aya | AYAstorm 独自機能 | async job |
| ViewerStatsRecorder.idle | llappviewer.cpp:1797 | per-frame (条件付) | sync | LL | performance recording | async job |
| Yield Time Sleep | llappviewer.cpp:1820 | per-frame (条件付) | sync | LL | explicit OS yield | main 残置必須 |
| Non-Interactive Sleep | llappviewer.cpp:1827 | per-frame (条件付) | sync | LL | background mode idle | main 残置必須 |
| Background Yield Sleep | llappviewer.cpp:1847 | per-frame (条件付) | sync | FS | window-not-focused idle | main 残置必須 |
| updateTextureThreads | llappviewer.cpp:1874 | per-frame | sync | LL | texture cache / decode thread tick | async job |
| LLLFSThread::updateClass | llappviewer.cpp:1879 | per-frame | sync | LL | file I/O thread tick | async job |
| gMeshRepo.update | llappviewer.cpp:1894 | per-frame | sync | LL | mesh fetch queue tick | async job |
| FPS Limit Sleep | llappviewer.cpp:1921 | per-frame (条件付) | sync | FS | frame-rate cap enforcement | main 残置必須 |

**所見**

- mainLoop 直下は OS event pump、idle、display、worker queue tick が主要。
- AYAstorm 独自追加は **PrimFeed Update (line 1791)** と **ETW Frame Logging (line 1631, FS との共有)**。
- FS 独自追加は **FPS Limiter (line 1921)** と **Background Yield Sleep (line 1847)**。
- per-frame の主負荷は idle() と display() に集中している (詳細 §2, §3)。

---

## §2. display() — render submit / draw pass

入口: `display()` (`indra/newview/llviewerdisplay.cpp`) → `LLPipeline::*` (`indra/newview/pipeline.cpp`)

| 系統 / 処理名 | entry (file:line) | 呼出頻度 | 同期前提 | 由来 | main 占有理由 | offload 可能性 |
|---|---|---|---|---|---|---|
| Shadow Pass (RenderShadow) | pipeline.cpp:12837 | per-frame | GL context 必須 | LL | shadow frustum 計算 + 複数 pass RTT | frustum 計算は thread 化候補、RTT 描画は main 残置 |
| Deferred Geometry Pass | pipeline.cpp:5039 | per-frame | GL context 必須 | LL | Pool iteration + per-object draw call emit + matrix setup | draw batch 構築は pre-frame 化候補、現在 main で pool 走査 |
| **Occlusion Query Issue / Retrieval** | pipeline.cpp:3159 | per-frame | GL context 必須 | LL | query 発行と前 frame 結果回収の同期待ち | 実測 **3534 /s 投げて 3 /s しか cull** → 設計問題、async readback 候補 |
| State Sort | pipeline.cpp:3976 | per-frame | GL context 必須 | LL | visible object list → per-pool + z-sort | alpha z-sort は thread-safe でない → main 残置必須 |
| Atmospherics (Haze / Soften) | pipeline.cpp:11668 | per-frame | GL context 必須 | LL | post-deferred fullscreen pass | GPU-bound、CPU overhead 小 |
| **Godrays** | pipeline.cpp:11737 | per-frame (条件付) | GL context 必須 | Aya (r15) | screen-space shadow march + additive | GPU compute、main は dispatch + uniform push のみ |
| **Skin SSS** | pipeline.cpp:11785 | per-frame (条件付) | GL context 必須 | Aya (r20) | separable blur 2-pass + gbuffer3 skin mask | GPU compute、main は dispatch gate + uniform push のみ |
| Render Deferred Lighting | pipeline.cpp:11049 | per-frame | GL context 必須 | LL | sun shadow + local lights loop (デフォルト 256 灯) | local light frustum culling は thread 化候補 |
| RenderFinalize (tonemap / glow / DoF / AA) | pipeline.cpp:10035 | per-frame | GL context 必須 | LL + Aya 混在 | post-proc chain: luminance → exposure → tonemap → glow → motion blur → DoF → FXAA/SMAA | 各 pass GPU 側だが RTT bind / uniform push / draw submit が main で直列化 |
| **Volumetric Lighting** | pipeline.cpp:10156 | Cinematic only | GL context 必須 | Aya (r30 P3) | renderVolumetric() fullscreen pass | GPU compute、main は dispatch gate |
| **Motion Blur** | pipeline.cpp:10173 | Cinematic only | GL context 必須 | Aya (r30 P2) | velocity buffer + blur composite | velocity buffer 生成が main upfront cost |
| Depth-of-Field | pipeline.cpp:10191 | per-frame (条件付) | GL context 必須 | LL | adaptive CoC + separable blur multi-pass | GPU compute、main overhead 最小 |
| HUD Attachment Render | llviewerdisplay.cpp:1384 | per-frame | GL context 必須 | LL | HUD camera matrix setup + cull + stateSort + renderGeomPostDeferred | matrix setup serial、別 thread 不可 |
| UI Render (3D + 2D) | llviewerdisplay.cpp:1598 | per-frame | GL context 必須 | LL/FS | render_hud_elements + render_ui_3d + render_ui_2d | UI submit 段階 serial、UI batch 事前構築で一部 offload 可 |
| Occlusion Query Collection | llviewerdisplay.cpp:898,1014 | per-frame | GL context 必須 | LL | updateCull() + fetchQueryResult() (前 frame 結果 waitsync) | GPU query fence stall が main 病巣、async readback 未適用 |
| swapBuffers | llviewerdisplay.cpp:1703 | per-frame | GL context 必須 | LL | gViewerWindow→getWindow()→swapBuffers() | GPU 同期、main block |

**所見**

- GPU 19 % util に対し main 60 % は、以下の **CPU 側直列化** が原因:
  1. **Occlusion query を 3534/s 投げて Occluded 3/s** — fetchQueryResult が main で wait stall、かつ cull 効果ゼロに近い (設計問題)
  2. **Pool / per-light iteration の serialize** — pool 走査と light frustum test が main で逐次
  3. **Matrix setup + state change** in renderGeomDeferred — GL bind 必須で thread 化不可
  4. **Post-proc chain の直列化** — RTT bind / uniform push / draw submit が main 上でチェーン

- AYAstorm 独自 render pass (Godrays / Skin SSS / Volumetric / Motion Blur) は **C++ 側はほぼ dispatch gate + uniform push のみで軽い** 一次判断。重さの本体は GPU 側で完結している。

- offload 候補: ① Occlusion query を async readback で next frame に defer、② local light frustum culling を background thread で先行、③ post-proc chain の luminance を async に分離、④ draw batch を frame N-1 で事前構築。

---

## §3. idle() — object update / octree / message pump

入口: `LLAppViewer::idle()` (`indra/newview/llappviewer.cpp`)

| 系統 / 処理名 | entry (file:line) | 呼出頻度 | 同期前提 | 由来 | main 占有理由 | offload 可能性 |
|---|---|---|---|---|---|---|
| **object update** | llviewerobjectlist.cpp:1050 | per-frame (mActiveObjects 全体) | sync | LL | **21k object × idleUpdate() loop**、per-frame | thread pool 候補 |
| object LOD (pixel area + texture) | llviewerobjectlist.cpp:859-866 | per-frame (lazy 1/16) | async OK | LL | gObjectList.updateApparentAngles(): setPixelAreaAndAngle + updateTextures | lazy loop で既分散 |
| message pump | llappviewer.cpp:6559 | per-frame (loop = MESSAGE_MAX_PER_FRAME) | sync 必須 | LL | checkAllMessages + processAcks (frame budgeted) | main 残置必須 |
| region idleUpdate | llworld.cpp:1142, 1170 | per-frame × region 数 | sync 必須 | LL | LLViewerRegion::idleUpdate (max_time × 0.25) — land + visibility + cache culling | async job 候補 |
| region visibility update | llworld.cpp:1103 | per-region per-frame | async OK | LL | updatePatchVisibilities + frustum culling | worker thread 候補 |
| **octree balance** | pipeline.cpp:2675, 2683 | per-frame × region 数 | sync 必須 | LL | mOctree→balance() 全 partition + VO cache tree | async rebuild 候補 (frustum 変化時のみ trigger) |
| particle sim | llworld.cpp:1205 | per-frame | async OK | LL | LLViewerPartSim::updateSimulation | async particle sim 検討対象 |
| drawables updateMove | pipeline.cpp:2663 | per-frame | sync 必須 | LL | updateMovedList: spatial partition update | incremental batch 候補 |
| inventory observer | llinventorymodel.cpp:2339 | per-frame (dirty 時) | sync 必須 | LL | handleResponses + notifyObservers batch | observer batch 化検討 |
| avatar tracker | llcallingcard.cpp:513 | per-frame (dirty 時) | sync 必須 | LL | notifyObservers loop | observer batch 化検討 |
| idle callbacks fire | llcallbacklist.cpp:112 | per-frame | sync 必須 | LL | callFunctions list iterate | callback batch 化検討 |
| event notifier | lleventnotifier.cpp:124 | per-30s (~900 frame) | low freq | LL | mEventNotifications map iterate | 低コスト |
| cleanup deadObjects | llviewerobjectlist.cpp:1640 | per-frame (timer × 10 ms) | sync 必須 | FS | cleanDeadObjects reverse scan | incremental OK (既 timer 制) |
| cleanup drawables | lldrawable.cpp:300 | per-frame | sync 必須 | LL | LLDrawable::cleanupDeadDrawables | incremental 検討 |

**所見**

- **mActiveObjects (約 21k object) × idleUpdate()** の per-frame loop が main thread 最大の負荷源候補。
- **octree balance() が毎 frame 全 partition rebalance** されているのは O(N) worst-case、frustum 変化時のみ trigger に変えられれば大幅削減。
- Sim FPS 44.9 / Time Dilation 0.998 (サーバ余裕) なのに main 60 % = **viewer 側 per-object iteration が病巣** という所見と整合。
- offload 優先 (一次判断): ① idleUpdate を部位別 batch で thread pool、② octree balance を async + lazy trigger、③ region visibility を worker、④ particle sim を async。

---

## §4. HTTP completion callback path

入口: `LLCoreHttp` worker thread → `_HttpOperation::addAsReply` → main thread の `LLHttpRequest::update` → `visitNotifier` → `onCompleted`

| 系統 / 処理名 | entry (file:line) | 呼出頻度 | 同期前提 | 由来 | main 占有理由 | offload 可能性 |
|---|---|---|---|---|---|---|
| Texture Fetch (HTTP) | lltexturefetch.cpp:2153 (onCompleted) | 頻繁 | async→queue→main | LL | onCompleted 内の mutex + 状態管理 | CURL/body 処理は別 thread 可、handler は main |
| Mesh Fetch (Header / LOD) | llmeshrepository.cpp (onCompleted) | 多頻度 | async→queue→main | LL | JSON parse + 3D model state machine | parse 部のみ offload 可 |
| Inventory Fetch | llinventorymodel.cpp:5610 (onCompleted) | 中程度 | async→queue→main | LL | LLSD parse + inventory sync | parse 部のみ offload 可 |
| Material Manager | llmaterialmgr.cpp:67 (LLMaterialHttpHandler) | 低頻度 | async→queue→main | LL | cap request + LLSD parse | offload 可能 |
| XML-RPC | llxmlrpctransaction.cpp:367 (update) | 低頻度 | async→queue→main | LL | XML parse | offload 可能 |
| **Worker→Main Bridge** | _httpoperation.cpp:216 (addAsReply) | 全 callback | **強制 sync** | LLCore | queue 機構自体が設計仕様 | 不可 |
| Coroutine Handler | llcorehttputil.cpp:329 (mReplyPump.post) | 可変 | async→EventPump→main | LL | EventPump dispatch | 非同期 pump 一部可 |

**所見**

- **Worker thread**: HTTP I/O (`HttpService::threadRun` → `processTransport` → `completeRequest` → `stageAfterCompletion` → `addAsReply`) は完全に worker 完結。
- **Main thread**: `LLHttpRequest::update(0)` で reply queue を一括処理。`visitNotifier::285` で `mUserHandler→onCompleted` を呼ぶ。
- **timeout 連発の局所性**: `Easy_28` (CURLE_OPERATION_TIMEDOUT) が worker thread で発火 → status set → `addAsReply` で `mReplyQueue` に積層。複数 simhost 同時 timeout で queue に callback が大量積層 → main thread の `update(0)` で一括処理。callback 内の mutex 保持で main thread block。
- 既に offload 済: CURL I/O、response body 受信。
- offload 候補: body parse (JSON / LLSD)。現状は main thread (visitNotifier 内で実行)。
- AYAstorm 独自 HTTP caller は調査範囲で検出されず (Stream3D の URL HEAD は `llpositionalstreammgr` の curl 呼出で、CoreHttp 経由ではない。§6 で別扱い)。

---

## §5. Image / texture / asset / mesh dispatch

worker thread と main thread の **境界線**:

| 系統 / 処理名 | entry (file:line) | 呼出頻度 | 同期前提 | 由来 | main 占有理由 | offload 可能性 |
|---|---|---|---|---|---|---|
| **Texture Decode (Worker)** | llimageworker.cpp:91-118 (LLImageDecodeThread::decodeImage) | per-texture request | thread pool async | LL | image format parse + decompress (8-thread pool) | **既に offload 済** |
| Decode Output (Responder) | llimageworker.cpp:231-238 (ImageRequest::finishRequest) | per-decoded texture | callback async | LL | decoded bitmap → raw image 格納 | 既に offload 済 |
| **GL Upload (Main 同期点)** | llimagegl.cpp:1631-1743 (LLImageGL::createGLTexture) | per-texture first-upload | **main thread bind** | LL | glTexImage2D / glTexSubImage2D | 不可 (GL context bound) |
| **Worker→Main Sync (GPU fence)** | llimagegl.cpp:1745-1797 (LLImageGL::syncToMainThread) | per-worker-upload | GPU fence + WorkQueue | LL | **NVIDIA: glFenceSync + glClientWaitSync (line 1756)**、AMD: async notify (line 1769) | 一部可 (AMD のみ) |
| Texture Name Update | llimagegl.cpp:1800-1810 (syncTexName) | per-texture complete | main thread callback | LL | 旧 texture 削除 + mTexName 更新 | WorkQueue 内で統合可 |
| Texture Create List Process | llviewertexturelist.cpp:1114-1206 (updateImagesCreateTextures) | per-frame batch | main thread loop | LL | imagep→createTexture (1138), scaleDown (1151, 1189) | 部分可 (shared GL context) |
| Texture Fetch Queue Dispatch | llviewertexturelist.cpp:1253-1323 (updateImagesFetchTextures) | per-frame batch | main thread loop | LL | texture fetch worker pool enqueue | 既に offload 済 |
| Texture Stats Update (LOD calc) | llviewertexture.cpp:1803-1882 (processTextureStats) | per-texture per-frame | main thread loop | LL | mDesiredDiscardLevel 計算 + virtual size update | parallel safe (offload 可) |
| Texture Binding (Per-frame) | llgl.cpp:145, 208, 238, 315 (LLTexUnit::bind) | 19000+ object × per-prim | **main render loop** | LL | glBindTexture | 不可 (render pipeline) |
| Mesh Fetch Dispatch | llmeshrepository.cpp:1071-1150 | per-visible mesh | worker thread async | LL | HTTP download via thread pool | 既に offload 済 |
| Bake Texture Render | llvoavatar.cpp:966-1047 | per-avatar per-change | main thread render | LL | layer set composite → glTexImage2D | 部分可 (shared GL context) |
| **Shared GL Context (LLImageGLThread)** | llimagegl.cpp:2629-2662 (LLImageGLThread::run) | 持続 worker | dedicated thread | LL | mWindow→createSharedContext (2637) | **実装済 (GL 4.0+)** |
| PBO Streaming Upload | llimagegl.cpp:1461-1488 (setImage) + 2527-2549 | per-mip batch | main or worker context | LL | GL_PIXEL_UNPACK_BUFFER + glTexImage2D(nullptr) + glTexSubImage2D batched | 実装済 (stagger mode) |
| Default Texture Bind | llgl.cpp:206 (bindDefaultImage) | per-missing-texture | main render loop | LL | fallback white / normal texture bind | 不可 |

**所見**

- 既に offload 済: image decode (8-thread pool)、HTTP fetch、mesh fetch。
- **境界線**: decode 後の raw bitmap (`LLImageRaw*`) を main に引き継ぐ `createGLTexture()` (line 1631) が主同期点。
- **NVIDIA で shared GL upload が逆効果の疑い**: `syncToMainThread()` (line 1745-1797) で NVIDIA は `glClientWaitSync(GL_SYNC_GPU_COMMANDS_COMPLETE)` で完了待ちが強制 (line 1756)。AMD は async notify (line 1769)。
  - **今日の実測 (RTX 5090): 2 番目に重い thread (TID 181797, 30 % CPU)** がこの fence wait で blocking している有力候補。実測で確認したい (§7 未確認リスト)。
- offload 候補: processTextureStats (parallel safe)。

---

## §6. AYAstorm 独自追加処理

| 系統 / 処理名 | entry (file:line) | 呼出頻度 | 同期前提 | 由来 | main 占有理由 | offload 可能性 |
|---|---|---|---|---|---|---|
| **§6.1 occlusion raycast refresh** | llocclusiongeometrymgr.cpp:275 (refreshOccluders) | per-frame 1x | sync (gAudiop) | Aya (r13) | OBB pre-cull + triangle extraction (drain 6 prim/tick cap) | 部分可 (extract を tick 分散済み) |
| §6.1b occlusion channel apply | llocclusiongeometrymgr.cpp:530 (applyToChannel) | per-speaker per-frame | sync (FMOD channel) | Aya (r13) | segment-vs-OBB slab test × occluder count | 困難 (audio frame 同期) |
| **§6.2 Stream3D positional stream** | llpositionalstreammgr.cpp:2398 (update) | per-frame 1x | sync (gAudiop) | Aya (r5-r13) | ObjectPropertiesFamily poll + linkset eval (curl HEAD は **既に worker thread に分離済 = `llstream3durlresolve.cpp` LLStream3DUrlResolve、r13 C で完了**)。main 側は結果回収 + binding 更新のみ。 | **既に offload 済 (r13 C)** |
| §6.2b Stream3D distributed stereo | llpositionalstreammgr.cpp:2599 | per-frame 1x | sync | Aya (r8-r12) | per-binding: speaker position push + per-speaker volume idempotent + venue / wetgain / LFE apply | 低コスト (idempotent early-return) |
| §6.3 parcel music (FSParcelStreamQuality) | llstreamingaudio_fmodstudio.cpp (gAudiop 経由) | per-frame 1x | sync (FMOD) | Aya (r12.1) | LFE gain / upmix / stream start / restart | 困難 (FMOD stream state) |
| §6.4 r24 dullahan audio callback | llviewermedia.cpp → plugin ring buffer | async | async (plugin thread) | Aya (r24) | MOAP media decode → ring buffer write (plugin thread で実施) | 既に async |
| §6.5a picker (self rigged) | fsselfriggedpicker.cpp:42 (findClosestAttachment) | on-click | sync (mObjectIDBuffer) | Aya (r21) | GPU buffer readback (1 px RGBA8 read + coord xform) | 困難 (frame pixel-perfect readback) |
| §6.5b picker (other rigged) | fsselfriggedpicker.cpp:48 (findClosestAttachmentForAvatar) | on-click | sync (GPU buffer) | Aya (r28) | target avatar の object-ID buffer readback (1 px) | 困難 |
| §6.6 chat tab split (Human / Object) | fsfloaternearbychat.cpp:334 | per-message add | sync (UI) | Aya (r22) | message source 判定 + target_panel routing (O(1)) | 低コスト (if 分岐のみ) |
| §6.7 venue reverb | llvenuereverbdsp.cpp setVenue() | per-binding per-frame (idempotent) | sync (atomic slot swap) | Aya (r11) | IR file load (初回のみ、以降 atomic pointer swap O(ns)) | 困難 (IR catalog lock) |
| §6.7b binaural HRTF | llpositionalstreamstereo.cpp setHRTFMode() | per-binding (tag parse 時) | sync | Aya (r11) | HRTF DSP chain setup (per-binding 1x) | 困難 (audio device context) |
| §6.8a sun dazzle | pipeline.cpp rendering pass | per-frame 1x (shader) | sync | Aya (r14) | shader dispatch のみ (C++ は cvar read O(1)) | 既に GPU side |
| §6.8b volumetric atmosphere | pipeline.cpp volumetric rendering | per-frame 1x | sync | Aya (r14) | calcAtmosphericVars uniform 計算 + post-pass dispatch | 困難 (uniform buffer) |
| §6.8c godrays (cvar push) | pipeline.cpp renderVolumetric | per-frame 1x (Cinematic 時) | sync | Aya (r15) | RenderVolumetricLighting cvar read + upmix tuning push | 低コスト |
| §6.8d cloud volumetric | pipeline.cpp cloud render | per-frame 1x | sync | Aya (r18) | shader post-pass dispatch のみ | 既に GPU side |
| §6.8e translucency | pipeline.cpp alpha blend | per-frame 1x | sync | Aya (r19) | render pass dispatch (shader 側) | 既に GPU side |
| §6.8f avatar SSS flag propagate | llvovolume.cpp:5842 | per-drawable update | sync | Aya (r20) | draw_info flag set (O(1) per-drawable per-frame) | 低コスト |
| **§6.9 Cinematic Controls overlay apply** | llcinematicoverlay.cpp:54 (applyCinematicOverlay) | mode-switch 時 | sync | Aya (r30) | settings_cinematic_bd.xml load + gSavedSettings.setBOOL loop | 可能 (起動時 1x、非クリティカル) |

**所見**

- ログで 2 秒毎に出ている `[ayastorm:occlude] tick` の dt は 0.08〜0.32 s。frame 周期 (47 ms) より長いが、これは 2 秒の tick 内累積時間で per-frame ではない。
- AYAstorm 独自で **main thread に乗っている重さ疑い** 一次判断 top 2:
  1. **Occlusion triangle extraction drain** (llocclusiongeometrymgr.cpp:337-357) — per-prim 抽出を 6 drain/tick で実行、TP burst 時に queue 蓄積で顕在化の疑い
  2. **Volumetric atmosphere uniform calc** (pipeline.cpp calcAtmosphericVars) — Cinematic mode 時に毎 frame FP 計算
- いずれも **agent の推論ベースの一次判断**。実測 (§7 未確認リスト) で確認するまで数字を信用しない。
- なお Stream3D の curl HEAD resolve は **r13 C で worker thread 化済 (LLStream3DUrlResolve)**。02 §A6 deep-dive で確認、main blocking ではない (旧 `llpositionalstreammgr.cpp:2444` の同期 resolve コメントは過去版の遺物)。
- 視覚表現章 (r14-r20) の C++ 側は **大半が dispatch gate + uniform push のみで軽い** と一次判断。重さの本体は GPU 側で完結している。

---

## §7. 観測から残る謎 (未確認 / 実測必要リスト)

ここは **推論ベースの一次判断** や **実測しないと判断不能** の項目を集める。02 の評価に使う前に、これらは実機計測か追跡コード読みで埋める。

### A. 実測必要 (profile / instrumentation で取る)

- **NVIDIA shared GL upload worker (TID 181797, 30 % CPU) の正体**
  - llimagegl.cpp:1756 `glClientWaitSync` で block している時間が CPU 30 % の主因かを確認したい
  - 手段: `perf record -g -p 181797`、または LL_PROFILE_ZONE を syncToMainThread に挿入

- **idleUpdate() per-frame loop の総時間**
  - 21k object × idleUpdate を main thread でどれだけ食っているか
  - 手段: idleUpdate 外側に LL_PROFILE_ZONE、`Develop > Show Info > Frame Profile`

- **octree balance() の毎 frame コスト**
  - frustum 変化していない frame でも balance が走っているか確認
  - 手段: pipeline.cpp:2675 直前で frustum hash diff log

- **Occlusion query fetch の wait stall 時間**
  - llviewerdisplay.cpp:898/1014 の fetchQueryResult で何 ms main が止まっているか
  - 手段: 前後に glFinish/clock 挿入

- **HTTP timeout 連発時の main thread block 時間**
  - Easy_28 timeout 連鎖中に visitNotifier handler 群が main をどれだけ占有するか
  - 手段: visitNotifier loop 前後で clock 計測

### B. 推論ベース (実コードでさらに追跡したい)

- Occlusion triangle extraction の per-prim 実時間 (2 ms/prim は推論値)
- Volumetric atmosphere `calcAtmosphericVars` の per-frame 実 CPU 時間 (未測)
- AYAstorm の `PrimFeed Update` (llappviewer.cpp:1791) が実際に何をしているか (機能内容を spec で確認したい)

(Stream3D `evaluateLinkset()` の curl HEAD timeout は **02 §A6 で worker thread 化済を確認**、main blocking ではないため本リストから外した)

### C. 設計問題 (実測前に既に明らか)

- **Occlusion Queries Performed: 3534 /s に対して Objects Occluded: 3.169 /s** — 設計問題。query を 3534 投げて 3 個しか cull できないのは culling の効果がほぼゼロ。原因仮説:
  - open sky 環境で occluder 不在
  - occluder 候補のサイズ閾値が大きすぎる
  - query 結果回収の遅延が cull 決定に間に合っていない

---

## §8. 占有時間の見積もり (実測前 = 全部仮置き)

01 を埋め終わったあと、`Develop > Show Info > Frame Profile` と `perf record -g -p <pid>` で
実測値を埋める。**実測前にここに数字を入れない**。

(空欄維持)
