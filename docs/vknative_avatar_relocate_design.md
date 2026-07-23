# avatar 描画の main 退避(relocate)+ per-core 分散 — 本体回復 設計

- 状態: 設計者起草 2026-07-22 深夜(AYA 会話で設計思想を受領・approve 前)。真実源 = HEAD の実行コード。
- **🔴 2026-07-23 実測による決着(AYA)**: **avatar relocate(off-main)line は Phase 2 で終了**。crowd off-main が main から剥がす motion compute は実測で激安(worker 4%・pose/submit も激安)= 低配当と判明。CROWD_OFFMAIN スイッチ削除 = always-on 化(commit `28b9ca14de`・壁#1/#3 の Upstream 準拠を source 確認・master=MT_THREADS)。**本 doc の前提「CPU-main-bound=pose/compute が標的」は誤り**: main は bound だが標的は **per-draw 描画記録(15k draw・rigged が dynamic 経路)**。→ **次フェーズ = `docs/vknative_perdraw_record_recovery_design.md`(3戦略: vsync sleep / rigged indirect+per-core 記録 / 静的 sync skip)が本 line を supersede**。診断真実源 = memory `finding_crowd_bottleneck_vsync_busywait_not_cpu`。並列化(T系/PE)は +10fps 検証済で有効・per-core 記録(secondary cmd buffer)が「crowd をコアに分散」思想の正統標的。
- 位置づけ: crowd 本体回復(fps 11 → 使用可能・北極星 50-100av)の本線設計。**`docs/vknative_crowd_body_recovery_design.md` §4 の stepA(eliminate = 脱 texture batch key)を supersede**。実測値・帰属(measurements doc + 旧 §4 の gds 表)は資産として保持し、旧 §4 は「eliminate 案 = 不採用・履歴」に降格(§7)。
- 依拠する一次実測: `docs/vknative_crowd_step0_measurements.md`(再走行不要)。

---

## 0. 設計思想(AYA 会話 2026-07-22・確定)

crowd の重い塊(avatar 描画)に対し、**並列化(塊を割って撒く)= 死案**(前任 2 名の実帰結・不可分・join ~31µs/av が支配)、**部分 elimination(保持で消す)= 適格母集団ゼロ**(texture poke 0.1%・genuine な変化ゆえ消せない)。両路線が「塊は速くできず・撒けず・部分的に消せない genuine な仕事」という同じ壁に当たった。

残る唯一の路線 = **塊を as-is で丸ごと main の外へ退避(relocate)し、空いた更地で per-core に分散する**。

- **速さでなく位置の問題**: GL 6 倍差はトポロジー差([[project_cancer_single_main_thread_pipeline]])。塊が main の critical path に居る限り VK は GL を超えない。
- **通貨は latency**: 1-frame 遅延(pose 遅れ = 不可視)を払って main を人質から解放する。塊は速くしない。
- **doctrine の regime 改定(AYA 承認要)**: [[project_vk_doctrine_eliminate_not_parallelize]] は「main 上・frame 内 join」regime でのみ正しい。**off-main + join 無し regime では parallelize を解禁**(core を使う)。eliminate は破棄でなく off-main 後の任意最適化に降格。
- **eliminate(A-1a 脱 texture batch key)を選ばない理由**: batch identity(registerFace llvovolume.cpp:6848-6870)手術 = E2 白バグ族リスク + churn は genuine ゆえ配当 capped。しかも後述の global allocator concurrent 化コストは eliminate でも避けられない。**relocate-whole が構造的に安い。**

## 1. product 境界(AYA 明示)

- **小人数(3-5av)は GL の方が速くて可**。relocate は 1-frame latency を払う = 小人数では GL 劣後を仕様として受容。北極星は crowd regime(相互作用下で「たくさん」が固まらない)。
- **main を空ける価値は速度に限らない**: SDL2 導入・resource headroom・contention 回避。→ Phase 1 gate「main が空いたか」は fps と独立に価値(速度は非約束)。

## 2. 確立済みの構造事実(HEAD 実トレース・file:line)

### 2.1 塊は 3 相に分散・共有 spatial 構造で結ばれる(非連続)
- idle 相: `updateCharacter`(skeleton animation)= llvoavatar.cpp:2997(idl28)、per-object idleUpdate 内インライン(v1 revert 済で独立 dispatch 消滅)。
- geometry rebuild 相(gupd churn 12-30ms/f): attachment bridge の spatial group が TE/vol/sculpt 変更で DrawInfo 全量 rebuild。帰属 = measurements §4(88-92% が bridge)。
- display 相(47-71ms/f): render pass が bridge group の DrawInfo → bucket → GPU。

### 2.2 avatar render domain は avatar 粒度で disjoint(専有 = 設計不変条件として成立)
- `LLSpatialBridge : LLDrawable, LLSpatialPartition`(llspatialpartition.h:522)→ `LLAvatarBridge`(:805)/`LLAvatarPartition`(:835)。bridge = 親 partition 内の 1 drawable かつ自前 octree を持つ partition。
- 各 attachment が自分の bridge を持つ(llvoavatar.cpp:3198)。attachment は 1 avatar の mAttachmentPoints 所属 = **avatar 跨ぎで spatial group 共有なし**。
- avatar 内で共有される唯一の resource = matrix palette(`mMatrixPaletteCache`・per-avatar)。→ 同一 domain 内ゆえ per-core 分散を阻害しない。

### 2.3 塊は既に部分 relocate 済(T 系の頭金)
- geometry rebuild パイプライン(llvovolume.cpp): **fill(頂点データ生成)= worker/off-main**(`geoWorkerMain` thread "aya-geoup":5785 → `LLFace::runVkGeoFill`:5807)。**staging(genDrawInfo:7620)+ apply(applyGeoStaged:5886)= main**。
- 既存規律: generation guard(`mVkGeoGen`/`mVkGeoInflight` llvovolume.cpp:6599/6625)+ inflight byte cap 512MB(:5783)+ volume pin(:5841)= producer/consumer handoff の下地。
- **main に残る理由**: ①staging/apply が global VK allocator(下記)に触る ②apply が render 同 frame 読みの `group->mDrawMap` を書く。

### 2.4 消費チェーンと非対称 seam
- `group->mDrawMap` →(`patchGroup` llvkbucket.cpp:174)→ global bucket `range->mRecords`(materialize copy:200)→(`rebuildTemplateIfDirty`:340)→ MDI template / dynamic list → GPU。
- **生産側 = bridge-local**(group は bridge 専有)→ double-buffer 可。
- **消費側 = global・pass 単位で avatar と world を混ぜる**(bucket は描画 pass で全 group 集約)。avatar/rigged draw は static MDI から除外され dynamic 経路(`info->mAvatar.isNull()` が static 条件・llvkbucket.cpp:378)。
- → **join は bucket 層でなく生産境界に引く**: avatar 領域が group->mDrawMap を off-main で double-buffer 生産 → 薄い fold(patchGroup)が published front を global bucket に畳む(main 残置可)→ render は bucket を読む(不変)。

### 2.5 skinning 出力 → render
- skeleton animation(`updateCharacter` idl28・main)→ palette build(`updateSkinInfoMatrixPalette` llvoavatar.cpp:10725・**現状 render 中 lazy**・per-frame cache・per-avatar)→ GPU upload(`writeObjectSkinUBO` lldrawpool.cpp:1678 等・per-draw・render/main)。
- `mMatrixPaletteCache` は per-avatar、mFrame 版管理 + `mLastGLMp`(前 frame snapshot:10739)が既存 = **1-frame pipeline の下地あり**。
- seam = **mGLMp**: palette build を avatar 領域へ、render は cached mGLMp を読み UBO write(main 残置)。要 = 「render 中 lazy 計算」→「production 相で先行計算」のタイミング反転(1-frame offset の実体)。

## 3. 設計不変条件(relocate の背骨)

- **INV-1 disjoint render domain**: **domain = 1 animation root + それに変換従属する attachment 群**(本体 skeleton + attachment bridge 群 + palette cache)は他 domain と render state を共有しない。**animesh**(`PARTITION_CONTROL_AV`/`LLControlAVBridge`/自前 control avatar+skeleton+palette・lldrawable.cpp:1278-1281・llvoavatar.cpp:11620): **着用 animesh = 着用者 domain**(変換が着用者 joint 従属 = 別 domain 化は本体と 1-frame 泳ぎ)・**単独 animesh = 独立 domain**。**⚠️ domain サイズ可変 = 着用 animesh 1 個 ≈ アバター 1 体分メモリ** → growth-on-demand が吸収するが総予算(slot 950K・mega)は animesh 膨張 worst-case で見積(Phase-2 flag)。共有例外が見つかれば設計で disjoint 化(§6 OPEN)。
- **INV-2 per-domain 資源 shard**: off-main が触る global VK allocator は **per-render-domain の sub-pool に shard**(単一 global mutex は不可 = N core が lock を取り合い直列に退化 = 逃げた元に戻る)。fence で merge。
- **INV-3 1-frame 契約**: production は back-buffer に生産、render は front を読む。pose/skinning/幾何は 1-frame 遅延可。**着脱・topology 変化は例外**(遅延で穴)= join 契約で別扱い(§6 OPEN)。
- **INV-4 検出器 covered**: 越境は既存 race detector(C_DRAWDATA_RACE 等)が名指し(憲法の検出器は不変・盲目化しない)。
- **INV-5 production 窓中の avatar state 排他**(P1-c で確定): domain が avatar render state(TE/face/pose/skeleton)を staging で読む窓の間、main は同 state を mutate しない。main の network mutation は sync-in(Update-Geom 境界)で drain。機構 = 既存 volume-pin(sGeoVolumePins llvovolume.cpp:5841)を avatar production 窓へ拡張。per-domain race probe で検出。
- **INV-6 固定上限 worker pool**(Phase 2・AYA 制定 2026-07-23): off-main worker thread 数は **固定上限付き** = `clamp(cores - RESERVED, 1, HARD_MAX)`。**per-avatar thread(死案)でも unlimited でもない**。avatar 数が unlimited 設定でも thread は固定 → graceful degradation(thread 爆発不能)。off-main が処理する avatar 集合は既存 impostor 上限(sMaxNonImpostors ≤66)で bounded = 独自 eviction を作らない(既存 lifecycle 準拠)。
- **doctrine(AYA 制定 2026-07-23)= 「unlimited は誠実でない」**: core も memory も有限。Upstream の RenderAvatarMaxNonImpostors=0(無限)は「不可能な約束」。設計は現実的な Upper を honest に付ける(thread pool = INV-6・描画 avatar cap の強制可否は product 決裁)。

## 4. join 設計仕様(4 項)

1. **何が渡るか** = avatar bridge の published draw 出力(double-buffered `group->mDrawMap` + face VB)+ per-avatar `mGLMp`。
2. **fence footprint**（= relocate の全量）:
   | 対象 | 現状 | 処置 |
   |---|---|---|
   | skinning palette build(updateSkinInfoMatrixPalette) | render lazy/main | avatar 領域へ・mGLMp double-buffer(mLastGLMp 流用) |
   | genDrawInfo staging + applyGeoStaged | main | avatar 領域へ・bridge group を double-buffer(swap = mVkGeoGen 流用) |
   | `drawDataAcquireSlot`(llvkloader.cpp:10812)/ VB(mega-buffer)確保 | main 単独・race probe 保護(lock 無) | **per-domain sub-pool 化(INV-2)** |
   | patchGroup → global bucket fold | main | 薄いので main 残置可(published front を読むだけ) |
   | writeObjectSkinUBO(GPU skin UBO) | render/main | render 残置(GPU 資源) |
3. **1-frame 例外** = attach/detach・topology 変化(§6 OPEN)。
4. **所有権/回収** = 既済 reap + per-draw 所有権 + gen guard に接続。

## 5. 実装計画(分割・sequenced)

### Phase 1 = 1 avatar を off-main(gate = main が空いたか・速度非約束)
- P1-a ✅ **設計確定(2026-07-22・source 実トレース)**:
  - **INV-1 disjoint 確認**: bridge は root drawable が 1 個所有(lldrawable.cpp:1324)・attachment linkset は root の 1 bridge 共有(:1287)= 1 avatar 内。cross-region/HUD⇄animesh 遷移は markDead→新生成の **reparent**(:1253-1268)= 共有でない。**avatar 跨ぎ group 共有なし** = double-buffer 単位 = avatar domain で安全。
  - **double-buffer 器**: 二重化対象 = ①bridge group の `mDrawMap` ②per-avatar `mGLMp`。avatar 領域が back 生産・render は front・swap = 生産完了点(既存 `mVkGeoGen`/`mLastGLMp` 土台)。fold(patchGroup)は swap 後 main の薄い step。
  - **1-frame 契約(二層)**: **membership(attach/detach/despawn = どの draw record が存在するか)= 同期・即時**(front から force-evict/insert・pipeline を待たない = **detach ghost 封じ**。hook = detachObject llvoavatar.cpp:8399 / markDead)。**content(skin matrix・頂点・texture)= 1-frame pipeline**(不可視遅延)。lifetime = front の LLPointer(`mSrcDrawable`)で 1 frame 生存(reap 接続・既済)。
  - **申告(P1-c で詰める)**: ①membership 即時 evict は render 中 front を触る = swap と evict の順序に mini-fence(evict は render 外 window 限定)②despawn の domain teardown 順序(bridge markDead 経由)は未トレース = P1-b で domain lifetime と確認。
- P1-b ✅ **設計確定(2026-07-22・source 実トレース)= 二層 sub-pool**:
  - **両 allocator が同一パターン**: mega(`sMegaVertexPools[typemask]`/`sMegaIndexPool` = chunk リスト・chunk 内 free_ranges・llvkloader.cpp:7902-8117)+ DrawData slot(`sDrawDataSlotNext` bump + `sDrawDataSlotFreeList`・:306-308/10812)。両方 pool-of-chunks / bump+freelist の二層 + 単独所有 `VkcRaceProbe`(lock 無し)。確保は現状 main(llvertexbuffer.cpp:962/976)。
  - **common case(頻繁)= domain 所有の chunk/slab 内で lock なし sub-alloc**(他 domain 状態に不触 = 取り合いゼロ = Phase 2 ~N× 保持)。
  - **growth(稀)= chunk 新規生成(megaNewChunk = 実 VkBuffer)/ slab 割当**のみ global。mutex or main-fence で直列化(稀ゆえ per-alloc 直列化に退化しない = INV-2 満たす)。
  - **release = 既存 deferred free**(`sPendingMegaFrees`/`sPendingDrawDataSlotFrees`・frame タグ)を fence 点で batch。
  - **検出器 = per-domain 化(憲法 4・AYA 承認 2026-07-22)**: `VkcRaceProbe(owner&, cause)` は owner を参照で取る(llvkloader.cpp:232)= **各 domain が自分の `atomic<U32>` owner を持ち、`C_MEGA_RACE`/`C_DRAWDATA_RACE` を domain 粒度で assert**。growth 共有経路は global probe。**`llvkcontract.*` 本体 diff ゼロ = 検出器盲目化なし・粒度を pool shard に合わせて複製するだけ**。
  - **申告(P1-c で閉じる)**: domain 消滅(despawn)時に所有 chunk/slab を global へ返す順序 = P1-a 申告②の domain lifetime と同一問題。
- P1-c ✅ **設計確定(2026-07-22・source 実トレース)= 既存機構の再配置(新プリミティブ不要)**:
  - **既存の 1-frame パイプラインを延長**: display "Update Geom"(llviewerdisplay.cpp:817-825)= drainGeoPublishQueue(前 frame fill を apply)→ updateGeom(今 frame rebuild を stage + worker enqueue)。**stage(N)→worker fill→apply(N+1) が既存**。relocate は staging + skinning + apply も off-main 化し back-buffer 化するだけ。
  - **avatar 領域 thread の production 周期**(Phase 1 = 1 avatar/1 thread): ①skeleton anim(updateCharacter idl28 から移設)②palette build → back mGLMp(render-lazy から移設)③geometry staging(genDrawInfo・per-domain allocator P1-b)④fill(runVkGeoFill・既 off-main)⑤apply → **back** group->mDrawMap。
    - **進捗(2026-07-23 更新・HEAD `7a7358cc5e`)**: ①✅ skeleton anim off-main = M1/M2/M3 完遂(`a3021d8135`/`9c15d8c2e4`/`5bba495a06`・controller single-owner mutex で lifecycle race 根治・memory `handoff_avatar_skeleton_offmain`)/ ②**palette build = ✅ NON-TARGET 確定(実測で潰した・2026-07-23・AYA 裁定・memory `handoff_avatar_palette_offmain`)**/ ③capture=main(非atomic refcount 点ゆえ構造的に main = B.2)・build=off-main / ④✅既 off-main / ⑤✅B.2 完遂。⚠️ TSan Layer A(§Phase2 検出器)は boost.fiber 非互換で不可(memory `finding_tsan_layerA_blocked_boost_fiber`)= gate は jemalloc heap-corruption crash を race オラクルに使用。
    - **②palette 非標的の根拠(self 専用計器・revert 済)**: per-frame の CPU「skinning 計算」= bone palette build のみ = **self 28µs/f**(offload 可能な matMul+pack = 11.5µs/f = aChar 2.05ms/f の 0.6%)。**実 skinning(頂点変形)は GPU**(`writeObjectSkinUBO` llviewershadermgr.cpp:533 = UBO memcpy → vertex shader)。palette は pose 不変でも**毎 frame 無条件再 build = 半静的**(llvoavatar.cpp:10730)。∴ GPU がやる仕事の CPU 転送だけを thread に載せるのは無意味。avatar CPU 塊の重量は skeleton(off-main 済)に集中。→ **Phase 1 の 5-step 周期は skeleton+apply で実質達成・palette は no-op 標的として閉じる**。もしレバーがあるとしても parallelize でなく eliminate(pose 不変 skip)= crowd 用小レバー・E系領分。
  - **🔴 skeleton 相(①)の本質設計 = `docs/vknative_skeleton_offmain_motion_design.md`(2026-07-22 ソース導出・確定)**。要点(旧記述「skeleton anim を丸ごと移設」を精緻化): skeleton は **A/B に分解**して割る — **(A) 配置(mRoot 位置/回転 = drawable/agent/velocity 由来・pose 非依存)= live/main 残置**(視点=カメラが毎フレーム読む聖域)、**(B) articulation(updateMotions → child 局所変換)= off-main**、**融合(updateWorldMatrixChildren = live root × off-main 局所)= main で re-root**。**⛔ カメラは off-main に持っていかない**(配置 A の消費者・自前状態あり・RLV @setcam が足場)。**⛔「joint 出力を snapshot して readers を redirect」は誤り = 全面破棄**(入口 doc §2 SUPERSEDED)= 出力でなく入力(motion 計算)を off-main・readers は live joint を読むだけ。B 内部の壁 = motion controller lifecycle race(deferred-mutation で貫通)。
  - **fence 群 = 3 点・全て "Update Geom" 点(render は strictly 後)に置く**:
    1. **swap fence**: domain 完了 signal で back→front(group + palette)。placement = drainGeoPublishQueue の位置(render 前の安全窓)= 既存 drain を swap に置換/拡張。
    2. **membership 即時 evict(P1-a①)**: detach/despawn を "Update Geom" 点で front から force-evict。**現状も rebuild は updateGeom(render 前)= 視覚タイミング同一 = 無回帰**(Update-Geom 後着の detach は現行も次 frame 待ち)。
    3. **teardown/lifetime(P1-a②/P1-b②)**: despawn = evict from front(sync)→ production 停止 → 所有 chunk/slab + front records を **既存 deferred-free(sPendingMegaFrees 等)+ reap** で GPU fence 後に返却 → domain 削除。順序は deferred-reclaim が保証(既済 fence-safe)。
  - **⚠️ 本 Phase の唯一の真の新規リスク = avatar render STATE の共有**: domain は avatar 状態(TE/face/pose)を staging で読むが、main の network 処理も書く。→ **既存の volume-pin 機構(sGeoVolumePins llvovolume.cpp:5841 = worker fill 中の mutation 防止)を avatar production 窓へ拡張**。main の当該 avatar への mutation は sync-in(Update-Geom 境界)で drain・production 中は domain が排他 read。**設計不変条件 INV-5(追加)= production 窓中、main は domain の avatar render state を mutate しない**(per-domain race probe で検出)。
  - **申告**: ①INV-5 の pin 対象(どの avatar 状態を pin するか = face/TE/skeleton の最小集合)は実装時に genDrawInfo/updateCharacter の read 集合を実トレースして確定。②Phase 1 は 1 avatar ゆえ swap/pin は単純だが、Phase 2 で N domain の swap を同 "Update Geom" 点に集約する際の main 側 fold コスト(patchGroup ×N)は Phase 2 の funnel 事項。
- gate: **main-thread の avatar 費目(idl28 + pub_ms + palette)が ~0 へ**(直接計測・捏造不能)+ 視覚同一 + validation 0 + 装置全層沈黙 + C_PAR/C_DRAWDATA_RACE 発火 0。fps は非約束(動けば儲け)。
- L3 型 A/B(GEOAB)必須(幾何経路変更ゆえ)。

### Phase 2 = crowd avatar を off-main へ(段階的・measurement-driven・AYA 方針 2026-07-23)
**⚠️ 決め打ちしない**: 「単一 avatar-domain worker への相乗り(全 crowd avatar を 1 worker が処理)で main が空くか」を **まず実装して測る**。per-core 多スレッド分散(Step2)が要るかは **その時の switch A/B 差分でしか判らない**。

#### off-main は既存 impostor lifecycle に只乗り(独自 eviction を作らない・HEAD 実トレース)
- off-main が乗るのは **NORMAL_UPDATE(可視・非 impostor)avatar のみ**。`updateCharacter`(llvoavatar.cpp:5344)→ NORMAL → `LLMotionController::updateMotions(bool)` → `asyncActive()` → off-main。**impostor/非可視/muted/cloud = HIDDEN_UPDATE = `updateMotionsMinimal`(main・pose 計算せず前 pose 保持)= off-main 対象外**。
- **off-main 集合サイズは `sMaxNonImpostors`(llvoavatar.cpp:498・既定 12・GUI max 66)で上限固定**。総数 200 でも 1024m draw でも off-main worker が捌くのは常に **≤ 非 impostor 上限**。churn(通り抜け)は既存 lifecycle(region 離脱 → `killObjects` llviewerobjectlist.cpp:1579 → 破棄)が回す = **独自 eviction 不要**。
- ⚠️ **impostor は compute/render の盾であって memory の盾でない**(§7 memory-axis)。高 draw crowd の memory 枯渇は別軸・off-main では治らない。

#### 実装方針(AYA 決定 2026-07-23)= 既存 off-main 機構に乗る・新機構を作らない
- **壁 #1(drain-window)= 既存 async 状態機械の「窓を閉じる一塊」を minimal でも呼ぶだけ**(既存コード共有)。**壁 #3(pin)= 既存 geo apply job が既に使っている LLPointer pin パターンを motion task にも適用するだけ**(geo job は snapshot の `LLPointer<LLVOAvatar>` を main で ref/解放・worker は move のみ = 既に crowd lifetime-safe。motion task だけが self=不滅ゆえ生ポインタで取り残されている)。**どちらも発明でなく既存踏襲。**
- **破棄タイミング**は既存の deferred model に乗る: `markDead`(flag)→ `cleanDeadObjects`(llappviewer.cpp:6144・idle 1 点)で object-list ref 解放 → **他 ref(我々の pin)が残れば実 free は延期**(LLPointer refcount 0 まで)= pin は deferred-destruction に自然に噛む(実 free は worker done + main drain 後・≤1 frame 延期)。main の削除排他(cleanDeadObjects/sNoDelete)は worker を守れない(worker は main 直列の外)ゆえ pin が worker の lifetime 錨。
- **最初は Upstream 準拠**(既存 CVAR = RenderAvatarMaxNonImpostors 等そのまま・既存 impostor lifecycle 只乗り)。**深い変更(motion 全 snapshot 化・破棄タイミング精緻化・per-core 分散)は実装/計測で気づいたら仕様へ逆適用(後回し)。**
- **未完の sweep(逆適用対象・実装中に洗う)**: 描画対象切り替え点(TP / region・parcel 移動 / impostor↔full / visible↔hidden / spawn・despawn)が全て壁 #1/#3 で覆えるか。cleanDeadObjects の idle 内位置と worker dispatch/drain の順序関係。→ 実装で edge に当たったら spec に反映。

#### Step 1(相乗り)= self 限定ゲート 2 点を外す + 壁 3 点
- **ゲート除去(2)**: ①`setAsyncCompute(true)` を全 avatar へ(現 llvoavatarself.cpp:227 self 限定 → LLVOAvatar へ)②`drainGeoPublishQueue` の `isSelf()` を外す(llvovolume.cpp:6859・`group->mAvatarp=this` は全 avatar で設定 llvoavatar.cpp:3249)。工事 switch `AYASTORM_CROWD_OFFMAIN`(gate 後削除)。
- **🔴 壁 #1(correctness)= 可視↔非可視遷移 race**: self は「never hidden update」(llvoavatar.cpp:5401)ゆえ安全だったが crowd は頻繁に hidden 化。`updateMotionsMinimal`(llmotioncontroller.cpp:1037)が `purgeExcessMotions`/`deactivateStoppedMotions`/`resetJointSignatures` で **mComputeMutex 無しに container mutate** → async dispatch 済(window open)の avatar が hidden 化すると worker と race = heap corruption。**解 = drain-window 共有**: window を閉じる一塊(updateMotionsAsync:902-913 = worker done なら applyBackBuffer + deferred 適用 + flag clear / 未了なら early-return)を `updateMotionsMinimal` 冒頭でも呼ぶ(共通関数化)。main stall なし・self 挙動不変。
- **🔴 壁 #2(scaling・correctness でない)= 単一 worker 飽和**: 1 worker が ≤ 非 impostor 上限の motion+geo を直列。捌けなければ latency 増(graceful = async 状態機械が worker 未了で早期 return/geo job は queue)。→ **N は「200」でなく「非 impostor 上限(12-66)」に bounded** = 現実的 = switch A/B で測る。不足なら Step2。
- **🔴 壁 #3(lifecycle)= avatar runtime 破棄 UAF**: motion task queue が生ポインタ(`std::deque<LLMotionController*>` llvovolume.cpp:5859・pin なし)。self は不滅ゆえ無害だったが crowd は runtime 破棄(despawn/region)→ queued/in-compute controller が解放 → UAF。~LLMotionController(llmotioncontroller.cpp:154)は worker と非同期。**解 = motion task に avatar pin(B.2 型)**: task = `{LLPointer<LLVOAvatar> mPin; LLMotionController* mC;}`・push(main=ref)→ worker が done-queue へ *move*(ref/unref no-op)→ main が drain して解放(unref)。全 ref/unref main = B.2 不変条件。pin は transient(≤ 非 impostor 上限・≤1 frame)= 累積しない。geo apply 側は既に pin 済(job の LLPointer 群)ゆえ追加不要。

#### Step 2(要すれば per-core)= 固定上限 worker pool(honest Upper・AYA 制定 2026-07-23)
- Step1 で 1 worker が飽和 or main がまだ詰まるなら、crowd を disjoint avatar 集合に分割し **N thread(= N sub-pool)** へ。**前提 = Phase 1 の per-domain sub-pool(INV-2・P1-b `76470220aa` で下地済)**。単一 lock は天井を潰す。
- **⛔ N は固定上限付き**(INV-6): `N = clamp(cores - RESERVED, 1, HARD_MAX)`。RESERVED = main + render/PE lane + T1 texture + T2 geometry + slack(既存 thread と oversubscribe しない)。HARD_MAX = 控えめ(例 8・measure で tune)。**per-avatar thread(死案)でも unlimited thread でもない**。avatar が unlimited 設定でも thread は N 固定 → graceful(各 thread 多め)・**thread 爆発は構造的に不能**。
- 期待配当 = work は avatar 数に線形 + CPU-main-bound ゆえ N core で ~N×(帯域/funnel が律速でない限り)。安全 = D1-D4(`81b7b0aef4`)+ jemalloc crash オラクル(TSan Layer A は boost.fiber 非互換で不可)。

#### gate 計測法(恒久 A/B レバー)= `AYASTORM_MT_THREADS`
`=1`(全直列 = 並列化前)vs 通常(並列)を **同一 session で 2 連続起動** = 同一シーンで直列 vs 並列の clean 差分(シーン変動ゼロ)= 「並列化総量」を post-Phase2 で測る。before-baseline snapshot 不要(switch が「並列化前」を再現)。※`MT_THREADS=1` は T系も直列化 = 差分は「並列スタック全体」。Step2 では N-core knob(`MT_THREADS=N`)に拡張し scaling を測る(HARD_MAX で clamp)。

## 6. 縮小・省略・解釈申告(OPEN・approve 前に潰す/AYA 判断)

1. **bridge-group の avatar 跨ぎ共有例外**の有無は未 grep(cross-region linkset 等)。INV-1 の disjoint 化コストに直結。→ Phase 1 P1-a 前に確認。
2. **1-frame 例外(attach/detach・topology 変化)の join 契約**は未設計。pose 遅延可・着脱は穴。
3. **VB(mega-buffer)確保の worker-safety** は「fill が既に off-main で触る」から部分的にしか言えない。per-domain shard の実装コストは未見積。
4. **patchGroup main 残置**の判断は「薄い」推定依存。pub_ms 0.8-3.6ms/job の apply vs patchGroup 比は未分離。
5. **funnel(submission/bindless descriptor/upload queue)の N-way 安全性**は Phase 2 の gate 事項・未検証。
6. PASS は宣言しない。本 doc は設計地図であり動作証明ではない(正のオラクル = Phase 1 gate の実測)。

## 7. memory-axis(off-main の対象外・別 workstream・AYA 確認 2026-07-23)

**compute(本 relocate line)と memory は独立軸**:
- **compute**(skeleton/pose)= impostor 上限(sMaxNonImpostors ≤66)で堰き止め済 → off-main は draw distance に対して安全(worker 負荷 ≤ 上限・**memory を悪化させない**)。
- **memory** = **ほぼ何も bound しない**。1 SIM = 256×256m ゆえ draw distance 1024m = 半径 4 SIM ≈ 最大 8×8 SIM が視界 → load 済 region の全 avatar(impostor 含む)の mesh+texture が resident(impostor も billboard を焼くのに mesh 実体が要る)。**高 draw crowd(200+)は memory 枯渇で落ち得る**(Upstream 含む)。jelly-doll(RenderAvatarMaxComplexity)/ auto-mute(RenderAutoMuteSurfaceAreaLimit)は render を落とすが memory shed は限定的(未確認)。
- **∴ off-main は freeze(main stall)を治すが memory 枯渇は治さない = 別 workstream**(遠 region avatar の mesh/texture shed / avatar LOD)。**北極星「50-100 名で固まらない」は少 region ゆえ memory 圏内で、そこの敵は freeze(compute)= 本 line が正しい標的**。1024m×多 region の memory ceiling を狙うかは product 決裁(deferred)。

## 8. governance(旧設計との関係)

- `docs/vknative_crowd_body_recovery_design.md` §4 stepA(eliminate = A-1a 脱 texture batch key)= **不採用・履歴**。理由 = §0。
- 保持する資産 = 同 doc §1-3・§5(有界化 = 信頼性安全弁)+ `docs/vknative_crowd_step0_measurements.md` 全体(実測・帰属)。
- 本 § への approve と INV-2(doctrine regime 改定)・stepA supersede は **AYA 承認案件**。
