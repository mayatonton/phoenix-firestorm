# avatar 描画の main 退避(relocate)+ per-core 分散 — 本体回復 設計

- 状態: 設計者起草 2026-07-22 深夜(AYA 会話で設計思想を受領・approve 前)。真実源 = HEAD の実行コード。
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
    - **進捗(2026-07-22・HEAD `42d5fb9531`)**: ①✅ skeleton anim off-main = M1/M2/M3 完遂(`a3021d8135`/`9c15d8c2e4`/`5bba495a06`・controller single-owner mutex で lifecycle race 根治・memory `handoff_avatar_skeleton_offmain`)/ ③capture=main(非atomic refcount 点ゆえ構造的に main = B.2)・build=off-main / ④✅既 off-main / ⑤✅B.2 完遂。**🔴 残 = ② palette build off-main(唯一の本体未着手)= memory `handoff_avatar_palette_offmain`**。⚠️ TSan Layer A(§Phase2 検出器)は boost.fiber 非互換で不可(memory `finding_tsan_layerA_blocked_boost_fiber`)= gate は jemalloc heap-corruption crash を race オラクルに使用。
  - **🔴 skeleton 相(①)の本質設計 = `docs/vknative_skeleton_offmain_motion_design.md`(2026-07-22 ソース導出・確定)**。要点(旧記述「skeleton anim を丸ごと移設」を精緻化): skeleton は **A/B に分解**して割る — **(A) 配置(mRoot 位置/回転 = drawable/agent/velocity 由来・pose 非依存)= live/main 残置**(視点=カメラが毎フレーム読む聖域)、**(B) articulation(updateMotions → child 局所変換)= off-main**、**融合(updateWorldMatrixChildren = live root × off-main 局所)= main で re-root**。**⛔ カメラは off-main に持っていかない**(配置 A の消費者・自前状態あり・RLV @setcam が足場)。**⛔「joint 出力を snapshot して readers を redirect」は誤り = 全面破棄**(入口 doc §2 SUPERSEDED)= 出力でなく入力(motion 計算)を off-main・readers は live joint を読むだけ。B 内部の壁 = motion controller lifecycle race(deferred-mutation で貫通)。
  - **fence 群 = 3 点・全て "Update Geom" 点(render は strictly 後)に置く**:
    1. **swap fence**: domain 完了 signal で back→front(group + palette)。placement = drainGeoPublishQueue の位置(render 前の安全窓)= 既存 drain を swap に置換/拡張。
    2. **membership 即時 evict(P1-a①)**: detach/despawn を "Update Geom" 点で front から force-evict。**現状も rebuild は updateGeom(render 前)= 視覚タイミング同一 = 無回帰**(Update-Geom 後着の detach は現行も次 frame 待ち)。
    3. **teardown/lifetime(P1-a②/P1-b②)**: despawn = evict from front(sync)→ production 停止 → 所有 chunk/slab + front records を **既存 deferred-free(sPendingMegaFrees 等)+ reap** で GPU fence 後に返却 → domain 削除。順序は deferred-reclaim が保証(既済 fence-safe)。
  - **⚠️ 本 Phase の唯一の真の新規リスク = avatar render STATE の共有**: domain は avatar 状態(TE/face/pose)を staging で読むが、main の network 処理も書く。→ **既存の volume-pin 機構(sGeoVolumePins llvovolume.cpp:5841 = worker fill 中の mutation 防止)を avatar production 窓へ拡張**。main の当該 avatar への mutation は sync-in(Update-Geom 境界)で drain・production 中は domain が排他 read。**設計不変条件 INV-5(追加)= production 窓中、main は domain の avatar render state を mutate しない**(per-domain race probe で検出)。
  - **申告**: ①INV-5 の pin 対象(どの avatar 状態を pin するか = face/TE/skeleton の最小集合)は実装時に genDrawInfo/updateCharacter の read 集合を実トレースして確定。②Phase 1 は 1 avatar ゆえ swap/pin は単純だが、Phase 2 で N domain の swap を同 "Update Geom" 点に集約する際の main 側 fold コスト(patchGroup ×N)は Phase 2 の funnel 事項。
- gate: **main-thread の avatar 費目(idl28 + pub_ms + palette)が ~0 へ**(直接計測・捏造不能)+ 視覚同一 + validation 0 + 装置全層沈黙 + C_PAR/C_DRAWDATA_RACE 発火 0。fps は非約束(動けば儲け)。
- L3 型 A/B(GEOAB)必須(幾何経路変更ゆえ)。

### Phase 2 = per-core 分散(gate = 帯域と funnel が N-way に耐えるか)
- crowd を disjoint avatar 集合に分割し N thread(= N sub-pool)へ(§0 の「使っちまえ」)。同じ join に流す(1 avatar でも N avatar でも同一機構)。
- 前提 = Phase 1 の per-domain sub-pool(INV-2)。単一 lock で作ると Phase 2 天井が潰れる。
- 期待配当 = work は avatar 数に線形 + CPU-main-bound(step0)ゆえ N core で ~N×(帯域/funnel が律速でない限り)。
- 安全 = D1-D4(`81b7b0aef4`)+ TSan(Layer A 未走行)が入場ゲート。

## 6. 縮小・省略・解釈申告(OPEN・approve 前に潰す/AYA 判断)

1. **bridge-group の avatar 跨ぎ共有例外**の有無は未 grep(cross-region linkset 等)。INV-1 の disjoint 化コストに直結。→ Phase 1 P1-a 前に確認。
2. **1-frame 例外(attach/detach・topology 変化)の join 契約**は未設計。pose 遅延可・着脱は穴。
3. **VB(mega-buffer)確保の worker-safety** は「fill が既に off-main で触る」から部分的にしか言えない。per-domain shard の実装コストは未見積。
4. **patchGroup main 残置**の判断は「薄い」推定依存。pub_ms 0.8-3.6ms/job の apply vs patchGroup 比は未分離。
5. **funnel(submission/bindless descriptor/upload queue)の N-way 安全性**は Phase 2 の gate 事項・未検証。
6. PASS は宣言しない。本 doc は設計地図であり動作証明ではない(正のオラクル = Phase 1 gate の実測)。

## 7. governance(旧設計との関係)

- `docs/vknative_crowd_body_recovery_design.md` §4 stepA(eliminate = A-1a 脱 texture batch key)= **不採用・履歴**。理由 = §0。
- 保持する資産 = 同 doc §1-3・§5(有界化 = 信頼性安全弁)+ `docs/vknative_crowd_step0_measurements.md` 全体(実測・帰属)。
- 本 § への approve と INV-2(doctrine regime 改定)・stepA supersede は **AYA 承認案件**。
