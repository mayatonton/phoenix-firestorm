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

- **INV-1 disjoint render domain**: 各 avatar の描画ドメイン(本体 skeleton + attachment bridge 群 + palette cache)は他 avatar と render state を共有しない。共有例外が見つかれば設計で disjoint 化する(§6 OPEN)。
- **INV-2 per-domain 資源 shard**: off-main が触る global VK allocator は **per-render-domain の sub-pool に shard**(単一 global mutex は不可 = N core が lock を取り合い直列に退化 = 逃げた元に戻る)。fence で merge。
- **INV-3 1-frame 契約**: production は back-buffer に生産、render は front を読む。pose/skinning/幾何は 1-frame 遅延可。**着脱・topology 変化は例外**(遅延で穴)= join 契約で別扱い(§6 OPEN)。
- **INV-4 検出器 covered**: 越境は既存 race detector(C_DRAWDATA_RACE 等)が名指し(憲法の検出器は不変・盲目化しない)。

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
- P1-a: avatar render domain の境界確定 + double-buffer 器(bridge group / mGLMp の front/back + swap fence)。既存 mVkGeoGen/mLastGLMp を土台に。
- P1-b: global allocator 2 本(drawDataAcquireSlot・VB)を **per-domain sub-pool 化**(INV-2)。C_DRAWDATA_RACE で越境検証。
- P1-c: 1 avatar 分の production(skinning palette build + genDrawInfo staging + apply)を avatar 領域 thread へ移し、main は published front を fold/render。
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
