# 記録相並列化の分解可能性検証(台帳・設計正本)

制定 2026-08-10。担当 = 設計者。status = **検証中(裁定未)**。
上位フレーム = 「ガン = 単一 main thread 直列」/「直列記録は一時状態」(memory 恒久)。前回崩壊の教訓 = 順序不可分の鎖(cull→幾何→記録→submit)を縦に割った category error。今回の仮説形 = **鎖は割らず、updateGeom 完了で入力を凍結した後の「pass 記録」だけを per-core で fork し、順序どおり join して単一 submitter に渡す相並列**。

## §0 裁定基準(先に固定・変更は AYA 承認)
- **fail-closed**: 記録相から到達可能な全共有可変状態は、証拠が出るまで **BLOCKER**。「たぶん大丈夫」「観測されなかった」は無罪の根拠にならない(憲法 6)。
- 無罪の形は 4 種のみ(各 file:line 必須):
  - **RO** = 記録相中は構造的に読み取り専用(書き手が相の外にしか居ない証明)
  - **TL** = 既に thread_local
  - **PART** = per-thread 分割可能(cursor/pool 等・分割設計を併記)
  - **FRZ** = 相開始時に snapshot 凍結可能(凍結点と凍結コストを併記)
- 1 つでも **BLOCKER が残れば「不可」裁定**(= 改修案を付して AYA に返す)。分岐は全枝トレース(憲法 7)。
- **scoping 裁定(AYA 2026-08-10)**: 第 1 スライス = **影 cascade fork のみ**(独立視錐台 = 自然分解・Phase A 無罪実績)。閉包対象 = 影 record 相の到達面のみ。camera pool 群(alpha 本体・materials・PBR・water/hero)の walk は「camera 側 = main 直列残置」の設計事実による不到達証明で camera スライス着手時まで先送り(sampling でなく scoping)。
- **閉包条件**: §2 の入口から到達可能な関数集合が (a) 台帳収載 or (b) 不到達証明 のどちらかで尽きた時のみ「閉包」。サンプリング不可。
- 検証完了後に**独立監査**(Fresh)→ AYA 裁定。実装はその後。

## §1 方法論
1. 入口列挙(§2)→ 呼び出し面の幅優先で、触る static/global/メンバ共有状態を台帳(§3)へ 1 行ずつ。hop ごとに本 file を更新(context 揮発対策)。
2. 分類は保守側に倒す。迷ったら BLOCKER のまま残す。
3. 「main-thread 前提」の検出は静的トレースを正とし、動的検出器(R1-R4 型)は**実装フェーズの gate 用**として設計だけ §5 に置く(検証段階でコードは触らない)。

## §2 記録相の定義と入口(トレース対象)
記録相 = 「cull/updateGeom/DrawData 供給が完了し、CB へ vkCmd* を積む区間」。fork 想定単位 = pass(影 cascade 0-3・camera 各 pass 群)。
- 入口(確定・pipeline.cpp): camera = `renderGeomDeferred`(:5422・pool 駆動 :5537)/`renderGeomPostDeferred`(:5679・:5826)。影 = `generateSunShadow`(:14492)→ `renderShadow`(:14079)→ `renderShadowOpaqueBucketizedMultiview`(:13762)/`renderShadowAlphaMultiview`(:13827)/`renderShadowOpaqueBucketized`(:14044)/`renderGeomShadow`(:5872・pool :5921)。他 = occlusion 発行・probe(gCubeSnapshot 再入)・water/hero・UI(consumer = main 残置)・aux。ここから各 LLDrawPool::render* → push 系 → LLVKLoader 記録 API へ幅優先【継続】。
- **walk 済セグメント(2026-08-10 第 3 波)**: `renderObjects`/`renderGLTFObjects`(pipeline.cpp:8976-9026)→ `pushBatches`/`pushUntexturedBatches`(lldrawpool.cpp:1323-1390)→ `pushIndirectBucket`(:1422-)→ `pushIndirectSpans`(:1400-)。接触列挙: gGLModelView/gGLLastMatrix = TL(#18 確定)/ sCurBoundShaderPtr 読み(#3)/ LLVKBucket 状態 = bucket 共有変異(#25 新)+ currentVisBits→singleton(#23 増強)/ shamdi delta idiom(#26 新)/ indirectRingAlloc(#6)/ ensureVkDrawDataSlot per-fire(#9 増強)/ VKC DrawScope・checkPerDrawIDFreshnessAtFire(#8)/ applyModelMatrix+gGL.syncMatrices(#2)/ gSnapshot 読み(FRZ 相当・書き手相外)/ s_fire_spans・sShadowBatchCullRadius = TL 無罪。bucket registry 変異(onGroupDestroyed 等 llvkbucket.cpp:308-355)は scene 側 lifecycle = 凍結契約(相中 scene 変異なし)に従属。**残 walk =** `renderAlphaObjects` 残余・`pushRiggedBatches`・GLTFSceneManager・camera 側 pool 群(alpha/materials/PBR)・`beginShaderDrawOrSkip` 内部【未】。
- **walk 済セグメント(2026-08-10 第 3 波続)**: `pushBatch`(:2017-)→ `drawInfoBindless`(:1283-)= 接触: gGL texunit bindFast/matrixMode/loadMatrix 群(#2)/ `gPipeline.mTextureMatrixOps++`(#28 新)/ `uploadMatrixPalette` → `avatar->updateSkinInfoMatrixPalette` = per-avatar cache 相中 lazy build(#27 新)/ `buildAndOverrideScenePerDrawSet`・`establishPerDrawId` → desc lane[0](#11)+ DrawData author(#5/#9)+ **texture bindless heap slot の相中 ensure(LLImageGL 族変異 = #9 同族・相前吸い上げ対象)**/ `setBuffer`/`drawRange` → 記録 API(memo TL 済)/ `vkcVerifyDrawModelview`(#8)。非 MDI per-draw 本流の接触面は全て既収載項に合流 = 新カテゴリなし。
- **walk 済セグメント(2026-08-10 第 2 セッション)**: `renderShadowOpaqueBucketizedMultiview`(:13762-13826)= 接触列挙: gGL 6 点(diffuseColor4f/setColorMask/getTexUnit/LLGLEnable×2/LLGLDepthTest)→ #2 / `gDeferredShadowMultiviewProgram.bind` → #3 / `LLPipelineFrameContext::setShadowPass` → #23(新)/ `sUseOcclusion` save-restore・`LLVertexBuffer::unbind` → #24(新)/ `ScopedIdentityModelView` → #18 / in-path `updateCull`+`stateSort`(:13780-13781)= §2 構造発見(前倒し工事対象)の再確認 / VkPerf shadow scope 群 = 全 TL(無罪)。次セグメント = `renderObjects` → `LLRenderPass::renderGroups` → pool walk【未】。
- **構造発見(2026-08-10)**: `generateSunShadow` は cascade ごとに **cull→stateSort→postSort(= rebuildGeom 変異!)→record を交互**に行う。postSort 帯の in-pass rebuild(今日の vb hostwrite で実証済)はこの交互構造の産物。∴ 相並列の前提工事 = **cull/stateSort/rebuild 群を全 pass ぶん前倒しして凍結相に集約**し、record だけの純粋区間を作ること。これは fork 以前に直列のまま行える準備改修(それ自体が正しさを固める)。
- fork 境界の外に置くもの(main 残置): cull・updateGeom・texture publish(INV-1)・VB upload CB(vbStageCopyVk = on_main guard 済)・occlusion query 発行?(要判定)・submit(PE)。

## §3 状態台帳(symbol / 所在 / 記録相中の書き手 / 分類 / 証拠・処方)
| # | symbol | 所在 | 分類(暫定) | 根拠・処方(file:line) |
|---|---|---|---|---|
| 1 | `tRecordCmdOverride`・`tMemoCmd`・memo sLast* 群 | llvkloader.cpp:205-235 | **TL(確定・設計条項 1 件)** | s 接頭含め全 memo が thread_local(:205-217 sLast* 群・:219-235 tMemo/tVB/tIB 群)。⚠️ ただし epoch 無効化(`tCmdRecordEpoch`)の bump 点は main の 3 箇所のみ(`beginCommandRecording` :5647・aux :13622・:13740)= worker の TL epoch は永久に不変 → CB handle が frame 跨ぎで再利用されると stale memo 命中の構造穴。**処方 = fork 入口で worker 自身の tCmdRecordEpoch を bump する設計条項必須**。同経路の `LLGLSLShader::sCurPerCallVkOffsetsDirty` は TL(llglslshader.h:402)で無罪 |
| 2 | `gGL`(LLRender)+ LLGLState 追跡 static 群 | llrender.h:490/llrender.cpp:47/llgl.cpp:357-406 | **TL(確定・seed 分類の誤りを訂正)+ lifecycle 条項(重)** | `gGL` instance 自体が thread_local(llrender.h:490)・gGLModelView/gGLLastModelView/LLGLState::sStateMap・cull/stencil 追跡群も全 TL。race は構造的に無い。**残る設計課題 = worker TL instance のライフサイクル**: ①fresh instance は未初期化(main の gGL.init 相当が worker で必要か = LLRender ctor の資源確保要件【未】)②親 state 非継承 → fork prologue seed(#18 条項)。ただし各 pass 関数が入口で自 state を確立する構造(LLGLEnable 等 in-fork)なので seed 対象は行列+既定値中心 |
| 3 | **LLGLSLShader object の record 相中可変 member 群**(current ptr は TL 訂正) | llglslshader.h:209(TL)/llglslshader.cpp:2753-2757/:2801-2805 | **BLOCKER(実体確定・範囲訂正)** | `sCurBoundShaderPtr` は既に TL(h:209/cpp:75)= seed の「per-thread current 化」は不要と訂正。**真の有罪 = shader object 単一実体の member を record 相中に書く 3 系統**: ①PC shadow `mVkFragPC[]`+`mVkFragPCMask`(:2753-2757・vkReassertFragPC が誤値再生)②per-program UBO `mVkActivePerProgramUBOMapped` への直接 memcpy + 非 atomic `mVkPerProgramUBOGeneration++`(:2801-2805 = 同一 shader を 2 fork が別値で使うと last-writer-wins)③pipeline map(#10)。処方 = lane 化(`mVkPerDrawLane[MAX_RECORD_LANES]` の既設前例に倣い PC/UBO 状態を lane 配列化)or pass 文脈へ外出し |
| 4 | `sInDynamicRendering`(pass scope flag) | llvkloader.cpp:147 | **TL(確定・seed 分類の誤りを訂正)** | 既に thread_local(:147)。fork thread ごとに自 CB の pass scope を持つ = 相並列意味論と整合。周辺の pass 状態(sSavedHasDepth 等 :7289 帯)の TL/共有は §2 幅優先で個別確認【未】 |
| 5 | DrawData scratch cursor(`sDrawDataScratchCursor`) | llvkloader.cpp:402/:13018 | **PART(確定)** | atomic fetch_add(:13018)・overflow は `C_DRAWDATA_SCRATCH_WRAP` fail-closed 発火 + modulo(:13020-13029)・重複排除 memo は TL(:13004-13006)。reset は beginFrame main(:5911)= 相前 |
| 6 | indirect ring cursor(`indirectRingAlloc`) | llvkloader.cpp:408/:12735-12787 | **BLOCKER(確定)** | cursor = **素の U32 非 atomic**(:408 宣言・:12784 非 atomic bump)。さらに record 相中の lazy buffer 生成(:12742-)と lazy frame reset(:12771-12775 非 atomic check-and-reset)。処方 = cursor atomic 化 + 生成/frame reset を相前 main へ前倒し(or pass ごと事前予約) |
| 7 | occlusion query pool(acquire/release) | llvkloader.cpp:6563- | **不到達(影スライス・確定)** | 二重に閉: ①`generateSunShadow` 全域 `LLDisableOcclusionCulling` RAII(pipeline.cpp:14504・:14475-14491)②`renderShadow` 自身も入口で `sUseOcclusion=0`(:14094)・restore は末尾 :14259・span 内の書き手は pipeline.cpp 全書き手列挙(:1557/:13774/:13823/:15732/:16016)+ 外部(llviewercontrol.cpp:719/llfloater360capture/llviewerdisplay)のいずれも不達・query 発行側 guard = llvieweroctree.cpp:1047(`<2 return`)。∴ :14139 `doOcclusion` は影スライスで機械的 dead。**camera スライス時に再裁定** |
| 8 | VKC 検証器(mdi shadow 表・fbslot 台帳 等) | llvkcontract.cpp:479 | **BLOCKER** | 「記録スレッド(main)専用アクセス前提」とコメント明記。検出器改修 = 憲法 4 |
| 9 | `LLDrawInfo::ensureVkDrawDataSlot`(記録中の LLDrawInfo 変異 + slot allocator) | llspatialpartition.cpp:4182 | **BLOCKER(範囲縮小・確定)** | allocator 側は無罪と判明: `drawDataAcquireSlot`(:12871-)は **TL `tAllocDomain` の per-thread domain 専有**(+`C_DRAWDATA_RACE` probe :12878)・`drawDataReleaseSlotDeferred`(:12908-)は per-domain `mPendMutex` 下 enqueue・2 thread self-test 実在(`allocDomainSelfTest` :12925-)。**残る有罪 = LLDrawInfo::mVkDrawDataSlot への record 相中変異**: 同一 DrawInfo が複数 fork(camera pass と影 pass)から同時 ensure されると二重 acquire + torn write。処方 = ensure を相前(凍結相)に前倒し(§2 構造発見の前倒し工事と同一枠)+ worker への tAllocDomain 割当(createRenderDomain 既存 :10040 帯) |
| 10 | pipeline cache(miss 時 `vkCreateGraphicsPipelines`) | llglslshader.cpp:4172/:88/:355 | **無罪(確定・Stage 0 実装時に訂正)** | `VkPipelineCache` object = driver 内部同期(:3611-3621)・memo TL(:85-87)。**map access は既に `sVkPipelineCacheMutex`(:88)で lock 済**: `getOrCreateVkPipelineForBoundRT` の関数 scope lock(:4172)が find→compile→insert 全体を被覆・unload の iterate+clear も lock 済(:355)。第 2 セッションの「無 lock insert」判定は同関数上流の lock を見落とした誤り = 訂正。改修不要 |
| 11 | descriptor set 供給(populate/pool) | llvkloader.cpp:497-506/:6996/:7390 | **PART(既設 lane 基盤・休眠)** | `sPerDrawDescLanes[MAX_RECORD_LANES]`(:506)+ per-shader `mVkPerDrawLane[MAX_RECORD_LANES]`・accessor list lanes(llglslshader.h:388-389/:426)= 前並列化の lane 分割構造が現存。ただし現在 `MAX_RECORD_LANES = 1`(llvkloader.h:73)で record 経路は `sPerDrawDescLanes[0]` 直書き(:6996)。処方 = lane 数復元 + record lane id 配線(新規設計でなく復配線)。lane 内 deferred_free reap は main frame tick(:7390)= 相外。matrix ring set 供給は #12 で mutex 済 |
| 12 | matrix ring / skin palette cursor | llvkloader.cpp:7461/:8692 | **PART(確定)** | 両者 atomic fetch_add。matrix ring growth は `sMatrixRingGrowthMutex`(:3907)+ 初回 reserve で vector realloc なし(:3909-3912)・per-thread current は TL(:200-203)。skin palette overflow は INVALID slot 退避 + `skin_bl_of` 計上(:8693-8697)= 落ちない |
| 13 | `gVkPerf` 計器群 | llvkloader.h | **RO 相当(atomic)** | 全 atomic。ただし `gVkPerfSetPath` 等の非 atomic gl obal 併設分を個別確認【未】 |
| 14 | mega/staging/upload CB(vbStageCopyVk) | llvkloader.cpp:9770 | **RO(相中)確定(条件+改修 1)** | `vbStageCopyVk` は on_main guard(:9770)で worker 不達。条件 = 凍結相前倒し(record 相中の VB 書換禁止 = 既定設計不変)。⚠️ guard が **silent false**(worker から呼ぶと copy が黙って落ちる = 隠れ fail-open)→ 実装フェーズで R 型 assert に昇格必須。`reclaimDeferredOnAllocFailure`(:5700)も main-only silent skip = worker の確保失敗時 reclaim 不発(安全側だが縮退・設計で注記) |
| 15 | `sVbStaging`・one-shot staging | llvkloader.cpp | **RO(相中)確定** | `vbStagingAlloc` の呼び手は vbStageCopyVk 経由のみ = main 限定(#14)。record fork worker は one-shot を submit しない(#20 条件と同一の設計不変条件)|
| 16 | フレーム CB 自体(`sCommandBuffers[slot]`) | llvkloader.cpp | **PART** | fork 単位ごとに secondary CB or per-pass primary + 順序 join(設計本体) |
| 17 | `gVkPerfShadowCtx`/`gVkPerfShadowSection`/`gVkPerfSetPath` 等の非 atomic 計器 global | llvkloader.h:1683-1685 | **TL(確定)** | 3 本とも既に thread_local(.cpp:6021/:6032-6033)。scope guard も TL 前提で正(h:1765-1773) |
| 18 | LLViewerCamera / gGLModelView 等の行列 global | llrender.cpp:49 / pipeline.cpp:351 | **TL 確定 + fork-seed 条項(FRZ)** | `gGLModelView`(llrender.cpp:49)・`gGLLastMatrix`(pipeline.cpp:351)・`LLRenderTarget::sCurResX`/`sBoundTarget`(llrendertarget.h:63/:239)・`sShadowBatchCullRadius`(lldrawpool.h:420)= 全て既に thread_local。⚠️ **横断設計条項: TL は「分離」を与えるが「継承」を与えない** — worker TL は初期値(identity/null)で始まるため、fork prologue で pass 文脈(行列・RT・viewport・cull radius 等)を親から**明示 seed(FRZ 値渡し)**する工事が必須。LLViewerCamera 側は【未】 |
| 19 | **解放系一式**(enqueue 側) | llvkloader.cpp | **分割裁定(確定)** | main-only assert(`noteDeferredEnqueueThread` :6727-6734)の被覆は 6 API のみ = `releaseOcclusionQueryVk`(:6738)/`destroyBufferVk`(:8977)/`destroyPipelineVk`・`destroyShaderModuleVk`・`destroyPipelineLayoutVk`・`destroyDescriptorSetLayoutVk`(:10483-10522)。**(a) mutex 済で並列 enqueue 無罪**: `drawDataReleaseSlotDeferred`(per-domain mPendMutex :12920)・`bindlessReleaseSlotDeferred`(sBindlessSlotMutex :12849)・`megaEnqueueFree`(sAllocGrowthMutex + per-domain mPendMutex :9612-9623)。**(b) main-only 残留 = BLOCKER**: 上記 6 API。記録相からの到達 = `destroyBufferVk` ← indirectRingAlloc 失敗経路(:12763・#6 で相前化すれば消滅)/ ensurePerDrawUBOArenaCurrent frame 切替(:7896・tick :5663 相前で実質不達)。occlusion release は #7 の main 残置裁定に従属。pipeline/layout destroy 系は record 相から不達(生成のみ・eviction は main tick)【残確認 = shader teardown 経路が相中に走らないこと = 自明(teardown は相外)】 |
| 20 | 各 free queue の **reap 側**(`reapReady` 系・tickOneShotFreeQueue 等) | llvkloader.cpp | **RO(相中)確定(条件 1)** | 駆動点全列挙: `tickMegaFreeQueue` = :5657(beginCommandRecording 帯 main = 相前)のみ。`tickOneShotFreeQueue` = :5659(相前)+ one-shot submit 経路(:9145/:9149/:9196)と upload worker unregister(:9340/:9349)= upload worker 自 pool bucket のみ(R1 構造)。`drawDataReclaimDomain` = main reap tick(:10478)+ 明示 `renderDomainReclaim`(self-test のみ)。**条件 = 記録 fork worker は one-shot submit を呼ばない**(純 record には upload なし = 設計不変条件として明文化) |
| 23 | `LLPipelineFrameContext` singleton(mShadowPass・cull result 等 pass 文脈) | llpipelineframecontext.h:30/:42 | **BLOCKER(新規・§2 walk 発見)** | 素の bool を singleton に set/reset(pipeline.cpp:13768/:13825)。さらに `currentVisBits()` が `getCullResult()` を同 singleton から引く(llvkbucket.cpp:498-502)= **cull result ポインタも pass 文脈として共有 singleton 在住**。fork 間で pass 文脈が混線。処方 = fork 引数化(pass context 一式 = shadow flag + cull result + 行列を値渡し)or TL 化 + fork-seed(#18 条項と同枠)|
| 24 | `LLPipeline` static 群の record 経路内 save/restore(`sUseOcclusion` 等)+ `LLVertexBuffer::unbind` の static bind 状態 | pipeline.cpp:13773-13774/:13824・llvertexbuffer | **BLOCKER(新規・§2 walk 発見)** | 影 MDI 経路が `sUseOcclusion` を save/0/restore = 共有 static の相中書換。`LLVertexBuffer::unbind()`(:13785)の bind 追跡 static も同族。処方 = pass ローカル化(fork 単位の値)。LLPipeline static の全数列挙は §2 walk 継続で【未】 |
| 29 | **LLDrawPool object の record 相中 member 状態**(`prerender`/`beginShadowPass`/`renderShadow(i)`/`endShadowPass`) | pipeline.cpp:5905-5925(renderGeomShadow) | **BLOCKER(族・#3 同型)** | terrain/tree/avatar 等 pool 単一実体の member を cascade fork が並行に触る(begin/end が pool/shader/gGL state を書く)。mPools 集合自体は凍結契約下 RO。処方 = #3 と同枠(pool state の pass 文脈化 or lane 化)。**各 pool renderShadow 内部の個別 walk は改修設計フェーズへ【未】**(裁定には本族の存在で足りる)|
| 27 | **per-avatar palette cache の record 相中遅延構築**(`updateSkinInfoMatrixPalette`) | lldrawpool.cpp:2112(uploadMatrixPalette 経由) | **BLOCKER(新規)** | record 相中に avatar object の MatrixPaletteCache を lazy build = 同一 avatar が camera fork と影 fork に同時出現で並行変異。処方 = 凍結相で可視 avatar 全数の palette を先行確定(#25 の相前化と同枠)。付随の objectSkinTryAdopt/StoreCache は #21(TL+mutex)へ合流 |
| 28 | LLPipeline/gPipeline member 計器の record 相中非 atomic 増分(`mTextureMatrixOps++` 等) | lldrawpool.cpp:2054 | **BLOCKER(計器・軽)** | singleton member への素 ++。#26 と同処方(ローカル計数 → 集約)。同族の全数列挙は【未】 |
| 25 | **bucket テンプレの record 相中変異**(rebuild + 全数 content-refresh) | lldrawpool.cpp:1424/:1492-1498 | **BLOCKER(新規・主要)** | `pushIndirectBucket` が record 相中に `rebuildTemplateIfDirty`(:1424)+ per-fire で `ensureVkDrawDataSlot`+`mdiAuthorAndCheck`(:1493-1497 = content-refresh 暫定形の全数 refresh)を実行。**bucket は pass type 単位で fork 間共有**(camera MDI と影 MDI・影 4 cascade 同士が同一 bucket を歩く)= 並列 fork で rebuild/author が衝突。処方 = memory `perf_mdi_template_content_dirty_mark` の根治(dirty-mark = 相前 rebuild + fire 時 read-only 化)と**同一工事** — 並列化前提工事と per-draw コスト根治が合流する点 |
| 26 | shamdi/matcen 等の **delta 差分 idiom**(atomic global の span 差分を自 cell に計上) | lldrawpool.cpp:1343-1352 | **BLOCKER(計器帰属・軽)** | `gVkPerf.shamdi` は atomic 配列(llvkloader.h:1551)= race は無いが、`r0 = mdi_rec.load() → 後で差分加算` は並行時に他 thread の増分を自 cell へ誤帰属 = 検証器/gate 計器として壊れる。処方 = ローカル計数 → atomic 加算(memory `project_serial_recording_is_temporary` 名指しの先行工事そのもの)|
| 21 | ObjectSkin 動的 UBO(shadow 本体・up frame/gen 群・frame cache) | llvkloader.cpp:8634-8640/:8665-8667 | **TL+PART(確定)** | shadow・WriteGen・UpFrame/Gen/Offset/Buf/Hash 全て thread_local(:8634-8640)。`sObjectSkinFrameCache` は mutex 保護 + 相内 lazy clear も mutex 下(:8667/:8671-8675/:8897)。※TL 化の意味論 = worker ごとに独立 upload(重複 upload は許容・正しさは保つ)|
| 22 | `allocPerDrawUBOSlice` + per-draw UBO arena | llvkloader.cpp:7932-/:7877-7916 | **PART(確定・条件 1+未確認 1)** | cursor = atomic CAS loop(:7958-)・overflow 追加は `sPerDrawArenaGrowthMutex` 下。条件 = frame 切替 reset(`ensurePerDrawUBOArenaCurrent` :7877 = destroyBufferVk を含む)は `tickPerDrawUBOArena`(:5663 = beginCommandRecording 帯 main)で相前に済む構造が既にあり、これを fork 前提の不変条件として明文化。overflow 経路の worker からの `createBufferVkImpl` は **VMA 内部同期で無罪確定**(`createVmaAllocator` :3649- に EXTERNALLY_SYNCHRONIZED flag なし = VMA 既定の内部 mutex 有効)|

## §4 既知の設計資産(再掲・前提)
- ⚠️ **休眠基盤への警戒(AYA 2026-08-10)**: 「基盤が在る」≠「基盤が正しい」。TL/lane/domain 群は前回崩壊期(category error)に書かれたものを含む = 半実装・bit-rot の疑いを持って扱う。台帳で PART/TL と裁定した項も、**実装フェーズでは §5 の 3 本柱(R 検出器・TSan・A/B hash)で個別に有罪推定から検証**する。「TL だから安全」を根拠にした設計省略は不可(憲法 6)。
- per-thread cmd pool(texture worker 工事)/ tRecordCmdOverride / timeline 単一 submitter / publish-on-main INV-1 / VB 専用 upload CB / fence 全廃。
- **per-avatar 並列は死案**(join 支配 ~31µs/av)= 粒度は per-pass(per-core 粗粒度)のみ検討。

## §5 確認手法の確立(検証段階では設計のみ・実装は裁定後)
実装フェーズの gate は次の 3 本柱で、**視覚や「落ちなかった」に依存しない**(憲法 6):
1. **thread-assert 検出器の全面展開(R 型)**: `noteDeferredEnqueueThread` の既設パターンを一般化し、台帳で「main 残置」と裁定した全 API(解放 enqueue・reap・publish・cursor 等)に fail-closed の thread 検査を付ける。worker が 1 回でも触れば VKC チャネルで名指し = 「たまたま壊れなかった」を排除。検出器新設 = 憲法 4 で AYA 承認をこの設計に含める。
2. **TSan 診断 build**: `-fsanitize=thread` の別 build を用意し、短時間走行で data race を機械検出(常用 build とは別・診断起動専用)。静的トレースの見落とし(lock なし共有 write)への保険。
3. **直列/並列 A/B 恒等オラクル(L3 型)**: 記録 API 層に(診断時のみ)pass ごとの記録列 hash(cmd 種別+主要引数の連結 hash)を取り、`AYASTORM_MT_THREADS=1`(既存 kill switch = 全直列退化)と並列走行で **pass 単位の hash 一致**を機械照合。join 順の正しさと記録内容の恒等を視覚でなく byte で判定。
- 受入順序: TSan clean + R 検出器沈黙 + A/B hash 恒等 → はじめて視覚 gate(AYA)。

## §6 改修案(裁定「不可(現状)」に付す再設計・7 系統 3 工程)

### §6.0 目標不変条件と工程原則
- **INV-P1(最上位)**: 「record 相は資源を**読むだけ**。作らない・解放しない・単一実体の member を書かない・pass 文脈は引数で受ける」。以下の全系統はこの不変条件の破れを 1 つずつ閉じる汎用機構(per-symptom flag 禁止)。
- **工程原則**: Stage 0/1 は**直列のまま**実施し各々単独で gate(警報ゼロ+既存検出器沈黙)。fork を導入するのは Stage 2 のみ。∴ 途中段で崩壊しても帰属が自明・Stage 0 は単体で perf 配当も出る(テンプレ author 前倒し = per-draw コスト削減)。
- **fork 単位(影スライス・現構造より)**: MV 経路 = union cull(:15214 帯)→ MV 一括 record(:15229 renderShadowOpaqueBucketizedMultiview)→ **per-layer 補完 record ×4**(:15261)。fork 候補 = per-layer 4 record(+ MV 一括は main 残置可)。非 MV fallback(:15322)= per-cascade 4 record。

### §6.1 系統①: 凍結相前倒し(Stage 0 本丸・= dirty-mark 根治と同一工事)
> **実装 Brief = `docs/vknative_parallel_stage0_brief.md`(2026-08-10 起草・自己監査済)**。恒久制約(Brief B9 より昇格): **DrawData author は可視性・pass 非依存が前提**(computeDrawDataSlots の入力 = DrawInfo member + texture slot のみ)。将来 DrawData に pass 依存値を足す変更はこの前提を破る = freeze walk 設計の見直しを伴う。
- 破れている不変条件: 「record 相中に描画資源を構築している」(lazy build 族)。
- 改修形(4 点・全て「相前 = updateGeom 後の凍結点で実施」に統一):
  1. **cull/stateSort の record 関数からの摘出**: `renderShadowOpaqueBucketized(Multiview)` 内の `updateCull`+`stateSort`(pipeline.cpp:13780-13781/:14049-14050)を呼び手(generateSunShadow)の凍結帯へ移動。record 関数は CullResult を引数で受けるだけにする。
  2. **bucket テンプレの相前確定**(#25): `rebuildTemplateIfDirty`(lldrawpool.cpp:1424)と per-fire content author(:1493-1497 の `ensureVkDrawDataSlot`+`mdiAuthorAndCheck`)を凍結帯の「全 pass 分 bucket 確定 pass」へ前倒し。fire 時は**テンプレ+DrawData を読むだけ**(= memory `perf_mdi_template_content_dirty_mark` の根治形そのもの)。dirty 判定は既存 `mTplDirty` + DrawData content の dirty-mark 化。
  3. **DrawInfo slot / texture heap slot の相前 ensure**(#9 残余): 凍結帯で可視 DrawInfo を走査し `ensureVkDrawDataSlot`・heap slot ensure を済ませる(2. と同じ走査に相乗り)。record 中の ensure 呼びは残すが**「既確定を返すだけ」**が正常経路になり、R 型 assert(§6.9)で「相中の新規確保」を fail-closed 検出。
  4. **avatar palette の相前確定**(#27): 凍結帯で可視 avatar 集合の `updateSkinInfoMatrixPalette` を先行実行。record 中は cache hit のみ。
- **INV-P2(自己監査で追加)**: texture/heap の publish・descriptor write は **update 段のみ**(凍結点〜frame 終端は禁止・assert 化)。現行でも publish は `updateImages` 帯(llviewertexturelist.cpp:1296 = render 外)= 構造は既に適合・不変条件化と assert が新規。これにより「凍結点で author → fire まで stale なし」が成立し、§6.1.2 の前倒しは slice B stale 教訓(content-refresh の導入理由)と両立する(refresh の代替 = INV-P2 + dirty-mark)。
- gate: 直列で視覚同一 + 警報ゼロ + (2.) は mdi_rec/mdi_dyn/shamdi 恒等 + per-draw 記録費の実測減(副配当)。

### §6.2 系統⑥: 計器正規化(Stage 0)
- 破れている不変条件: 「帰属計測が global の時間差分に依存」(delta idiom)。
- 改修形: `shamdi` 帯の `r0=load()→差分加算`(lldrawpool.cpp:1343-1352/:13946 帯)を**ローカル計数 → atomic 加算**へ(pushIndirectBucket が (rec,dyn) を戻り値/参照で返す形)。`gPipeline.mTextureMatrixOps++`(:2054)・`s_rt_resume_count`(llvkloader.cpp:13889 帯)等の素 ++ は atomic 化 or TL 化。対象の全数列挙は実装 Brief で(grep 網: `\+\+.*gVkPerf` 非 atomic / `\+\+` on singleton member in record path)。
- gate: 直列で計器値の恒等(改修前後の同一走行比較)。

### §6.3 系統③: indirect ring の並列安全化(#6・Stage 0)
- 破れている不変条件: 「ring cursor が非 atomic + 生成/リセットが record 相中 lazy」。
- 改修形: `sIndirectRingCursor`(llvkloader.cpp:408)を atomic fetch_add 化(#5 と同形・overflow は既存 fail 経路維持)。buffer 生成(:12742-)と frame reset(:12771-)は `beginCommandRecording` 帯の相前 tick へ移動(#22 の tick と同居)。
- gate: 直列で挙動恒等(mdi_call/mdi_rec 恒等)。

### §6.4 #10: per-shader pipeline map の同期(Stage 0)
- 改修形: `mVkPipelineCache` の lookup/insert(llglslshader.cpp:4466-4472)を per-shader mutex で包む(miss は稀 = 競合実費ゼロ)。memo(TL)は無変更。
- gate: 直列で挙動恒等。

### §6.5 系統⑤: pass 文脈の fork 引数化(Stage 1)
- 破れている不変条件: 「pass 文脈(shadow flag・cull result・行列・target 幅)が singleton/global 在住」。
- 改修形: `RecordPassContext` 構造体(値渡し)を導入 = { shadow_pass, LLCullResult*, view/proj, target_width, cascade index, …}。record 系関数(renderShadow* / renderObjects / push 系)の読み先を `LLPipelineFrameContext::getInstance()`(#23: llpipelineframecontext.h:42・llvkbucket.cpp:498)と `sUseOcclusion` save/restore(#24)から ctx 参照へ置換。直列では ctx を main が 1 個持つだけ = 挙動恒等。
- gate: 直列で挙動恒等 + `LLPipelineFrameContext` の record 経路参照ゼロ(grep 機械確認)。

### §6.6 系統②: object member の lane 化(#3/#29・Stage 1)
- 破れている不変条件: 「shader/pool の単一実体 member を record 相中に書く」。
- 改修形(既設 `mVkPerDrawLane[MAX_RECORD_LANES]`(llglslshader.h:426)の前例に統一):
  - shader PC shadow: `mVkFragPC`/`mVkFragPCMask`(llglslshader.cpp:2753-2757)→ lane 配列化(書き/再生とも lane id 経由)。
  - per-program UBO: `mVkActivePerProgramUBOMapped` 直書き + 非 atomic `mVkPerProgramUBOGeneration`(:2801-2805)→ 書き込み先を per-lane slice(#22 の allocPerDrawUBOSlice は並列安全済)へ・generation は lane 別。
  - pool 状態(#29): `begin/endShadowPass` が書く pool member を列挙し(実装 Brief で pool 別 walk)、pass ローカル/lane へ移す。
- gate: 直列(lane=1)で挙動恒等 = 純リファクタ。

### §6.7 #11: record lane 復配線(Stage 1)
- 改修形: `MAX_RECORD_LANES`(llvkloader.h:73)を worker 数へ・`sPerDrawDescLanes[0]` 直書き(llvkloader.cpp:6996)と shader 側 lane 配列を lane id 配線に戻す。lane id は fork prologue が TL に設定(直列時は常に 0 = 挙動恒等)。
- gate: 直列 lane=1 恒等。

### §6.8 系統⑦: fork 本体(Stage 2)
- fork prologue(worker 側・一様形 = main 特例なし・R1): ①TL seed = RecordPassContext から行列/viewport/target を設定 ②`tCmdRecordEpoch` bump(#1 条項)③`tAllocDomain` 割当(`createRenderDomain` 既存)④lane id 設定 ⑤gGL worker lifecycle = LLRender TL instance の初期化要件を実装 Brief で確定(【未】明記済)。
- 記録面: per-layer record を per-thread CB(secondary or per-pass primary・#16)へ。**順序 join = 生成順に main が集約し単一 submitter(PE)へ**。worker は one-shot submit・vbStageCopyVk・解放 main-only 6 API に不達(構造 + assert)。
- **pass 所有(自己監査で訂正)**: 各 fork が**自 CB 内で自 layer の pass begin/end を所有**する(per-layer の LLRTScope 相当を worker prologue/epilogue が記録・TL `sInDynamicRendering` #4 と整合・F2 契約に接続)。「fork 前に pass 確立」ではない。fallback 枝(`beginShaderDrawOrSkip` の swapchain fallback / RT resume・llvkloader.cpp:13870-13902)は worker では即 FAIL(fail-closed)= 正常経路で不達が契約。
- kill switch: 既存 `AYASTORM_MT_THREADS=1` で全直列退化(A/B hash オラクルの比較基準)。

### §6.9 系統④: 検出器・gate(Stage 2 と並走・憲法 4 = AYA 承認対象)
- VKC lane 対応(#8): 「記録スレッド main 専用」前提(llvkcontract.cpp:479)の台帳群を lane 別 or mutex 化。**検出器の盲目化ではなく多重化**(fail-closed 維持)。
- R 型 thread-assert 全面展開: main-only API 6 本 + vbStageCopyVk 系の silent-false を**発火型**へ昇格・「相中の新規確保」(§6.1-3)検出。
- §5 の 3 本柱: TSan 診断 build / 直列↔並列 pass 単位 hash 恒等 / R 検出器沈黙 → その後にのみ視覚 gate(AYA)。

### §6.10 自己監査(correctness / gap / hidden・2026-08-10 敵対的自己監査 実施済)
- **監査で実コード検証した最弱主張 2 点**: ①§6.6 の per-program UBO = per-shader **ring**(`mVkPerProgramUBORing[f][idx]`・llglslshader.cpp:3290-3305)で ring idx/active ptr が共有 member と実証 = lane 化は「ring idx・active ptr の lane 別化」で成立(ring 本体は共有可)。②texture publish は `updateImages` 帯(llviewertexturelist.cpp:1296 = record 外)と実証 → INV-P2 を §6.1 に追加(前倒し × slice B stale 教訓の両立条件)。
- **監査が正した設計欠陥 2 件**: §6.8 の pass 所有の記述を訂正(「fork 前確立」→「各 fork が自 CB 内で begin/end 所有」)/ §6.1 に INV-P2 欠落を追加。
- correctness: 各 Stage は直列で挙動恒等を gate に持つ = 崩壊時の帰属が段単位で確定。fork 前に全 BLOCKER 系統が閉じる依存順(①⑥③⑩→⑤②⑪→⑦④)。
- gap(既知・実装 Brief で閉じる義務): pool 別 `renderShadow` 内部の member 全列挙(#29)/ LLRender ctor の worker 初期化要件(#2)/ §6.2 計器の全数列挙 / per-program UBO の write/commit 全周期 trace(§6.6)/ `union_result` の並行 read 純粋性(LLCullResult iteration が lazy 変異を持たないこと)/ 非 MV fallback(:15322)の 4-result 前倒し(憲法 7 = MV/非 MV 両枝とも工事対象)/ MV 一括 record は第 1 段では fork **しない**(main 残置)= 範囲最小化。
- hidden(安い方へ寄せた点の申告): §6.1-3 で「record 中 ensure は残し assert で監視」= 完全撤去でなく検出併用(全 call site 撤去は工数過大・fail-closed 検出で等価安全)。§6.6 pool 状態は列挙未了のまま設計形だけ確定(裁定は不変)。
- 未強制点: 本設計自体はまだ機構強制されない = Stage 順の遵守は運用依存(各段 gate が実質の強制)。**独立監査は本設計段では実施しない(AYA 裁定 2026-08-10)= 実装時に実コード全読トレースで実施**。

## 進捗ログ
- 2026-08-10: 台帳骨格 + 容疑者 20 項 seed(#2,3,4,7,8,9,19 が主要 BLOCKER)+ §5 確認手法 3 本柱 + §2 入口確定 + 構造発見(cull/record 交互 → 前倒し工事が fork の前提)。次 = 入口からの幅優先で【未】潰し(#1 memo 変数の s/t 判定 → #5,6,10,11,12 の同期方式 → #19 解放到達経路の全列挙)。
- 2026-08-10(第 2 セッション)= **llvkloader 資源層の閉包が大きく前進・「休眠並列基盤」の実在を確認**:
  - **無罪確定(TL/PART/RO)**: #1(全 memo TL・ただし fork 入口 epoch bump 条項)・#5・#12・#13 相当・#17(既 TL)・#20・#21 ObjectSkin(新設・全 TL + mutex cache)・#22 per-draw UBO arena(新設・atomic CAS + mutex growth)。
  - **BLOCKER 確定(処方付き)**: #6 indirect ring cursor(素 U32 + lazy 生成/reset)・#10 per-shader pipeline map 無 lock insert(軽・mutex で閉)。
  - **範囲縮小**: #9 = allocator は per-thread domain 既設(tAllocDomain/self-test 実在)で無罪 → 有罪は DrawInfo 変異のみ(相前吸い上げで閉)。#19 = mutex 済 3 系統は無罪・main-only assert 6 API のうち record 到達は #6 経由 1 本(相前化で消滅)。#11 = lane 基盤既設(MAX_RECORD_LANES=1 で休眠)= 復配線工事。
  - **残る主要 BLOCKER = #2 gGL・#3 sCurBoundShaderPtr + uniform cache・#7 occlusion・#8 VKC 検証器**(いずれも記録相の中核状態 = 相並列設計の本丸)。#4 は seed 誤分類で実は TL(:147)= 無罪に訂正。#22 buffer 生成は VMA 内部同期で無罪確定。
  - 残【未】= #14/#15 flush 経路・#18 行列 FRZ 設計・#4 周辺 pass 状態(sSavedHasDepth 等)・§2 幅優先(LLDrawPool::render* → push 系の newview 側 walk = 最大の未走査面。gGL/#3 の接触点列挙もこの walk で同時に出る)。
- 2026-08-10(第 3 波)= **§2 walk を MDI fire / 非 MDI per-draw 本流まで貫通・台帳 28 項**:
  - **AYA 警戒指示を §4 に明文化**: 休眠基盤は「在る≠正しい」= PART/TL 裁定済みの項も実装フェーズは §5 3 本柱で有罪推定から検証。
  - **横断設計条項の発見(#18)**: TL は分離を与えるが継承を与えない = fork prologue での pass 文脈 seed(FRZ 値渡し)が全 TL 資産に共通で必須。
  - **新規 BLOCKER**: #25 bucket テンプレ相中変異(rebuild+全数 refresh・fork 間 bucket 共有 = **dirty-mark 根治と同一工事**)/ #26 shamdi delta idiom(計器帰属)/ #27 per-avatar palette 相中 lazy build / #28 gPipeline member 計器。
  - **無罪追加確認**: gGLModelView・gGLLastMatrix・sCurResX・sBoundTarget・sShadowBatchCullRadius・s_fire_spans = 全 TL。gSnapshot = FRZ 相当。
  - **構造結論(中間)**: 非 MDI per-draw 本流の接触は全て既収載項に合流 = 未知カテゴリの湧きは止まりつつある。閉包を阻む本丸は変わらず #2 gGL(texunit/行列 state machine)・#3 shader current・#8 VKC・#25 系「相中 lazy build 族」(テンプレ・palette・heap slot・DrawData slot = **全て「凍結相への前倒し」という単一処方に収斂**)。
  - 次 = 残 walk(alpha 残余・rigged・GLTF・camera pool 群・beginShaderDrawOrSkip)+ #2/#3 の接触全列挙 + #14/#15。
- 2026-08-10(第 4 波・影スライス scoping 承認後)= **影スライス到達面の walk 完了・裁定準備完了**:
  - **walk 済(第 4 波)**: `renderAlphaObjects` 全体・`renderShadowAlphaMultiview` 全体・`renderAlphaObjectsMultiview`・`renderMaskedObjects`/`renderFullbrightMaskedObjects` → `pushMaskBatches`・`pushGLTFBatch`/`pushUntexturedGLTFBatch`・`setMinimumAlpha`/`setObjectAlpha`/`vkPushFragPC` 内部・`beginShaderDrawOrSkip` 内部・`renderShadow` 本体・`renderShadowOpaqueBucketized`・`generateSunShadow` 冒頭(RAII)・`renderGeomShadow` pool loop。全接触は既収載項に合流(新カテゴリ = #29 pool object 状態のみ)。
  - **重大訂正 2 件(seed の過剰 BLOCKER)**: #2 `gGL` は instance ごと thread_local(llrender.h:490)+ LLGLState 追跡群も全 TL = race 構造なし → 課題は worker instance lifecycle へ変質。#3 `sCurBoundShaderPtr` も TL(h:209)→ 真の有罪 = shader object member 3 系統(PC shadow・per-program UBO 直書き+非 atomic gen・pipeline map)に実体確定。
  - **無罪確定追加**: #7 影スライス不到達(二重 guard・書き手全列挙)・#14/#15(凍結契約下 RO・silent-false の assert 昇格を実装条件に)・共有 UBO 動的 family 全 TL(AvatarSkin/PBRMaterial/DrawColor/ShadowParams/ShadowViewProj)。
  - **beginShaderDrawOrSkip の設計注意**: pass 逸脱時の fallback 枝(beginSwapchainRendering / resumeVkDynamicRendering :13870-13902)は worker で構造排除必須(F2 契約領域)+ `s_rt_resume_count` 非 atomic(#28 族)。
  - **影スライスの BLOCKER 総括(裁定入力)**: ①相中 lazy build 族 = #25 bucket テンプレ・#27 avatar palette・#9 DrawInfo slot・texture heap slot → **単一処方 = 凍結相前倒し** ②単一実体 member 書き = #3 shader object・#29 pool object → lane 化 or pass 文脈化 ③#6 indirect ring cursor atomic 化+相前化 ④#8 VKC 検証器 lane 対応(憲法 4 = AYA 承認要)⑤#23/#24 pass 文脈の fork 引数化 ⑥計器 = #26/#28 ローカル計数化 ⑦横断 = fork prologue(TL seed・epoch bump #1・tAllocDomain 割当)+ gGL worker lifecycle(#2)。
  - 残【未】(裁定を変えない・改修設計フェーズへ): 各 pool renderShadow 内部(#29 詳細)・LLRender ctor の worker 初期化要件(#2)・LLViewerCamera(#18)。
