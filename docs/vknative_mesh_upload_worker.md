# Stage 3: mesh(buffer)off-main upload 詳細設計

制定 2026-08-10。統一土台 = [[docs/vknative_dl_resource_foundation.md]](§3.5 lifecycle / §5.3 worker API / §3.6 correctness)。image 側 = [[docs/vknative_texture_upload_worker.md]]。**本 doc は Stage 3 の設計正本。実装 GO は §4 の open を全て潰してから**(施主厳命 = 全把握してから・over-claim 禁止)。
**状態 2026-08-10**: §5 の (A)/(B) 判定 = 施主決裁待ちのまま。Stage 1(texture)は初回実装 revert → 再設計済(texture doc PART J)= 本 doc の O1-O4 トレース・D1/D2 dead 判定に影響なし。

## 0. 位置づけ
mesh は buffer 機構(mega-buffer 永続 map)= per-thread substrate(`tAllocDomain`/`MegaDomain`)は**既存**(§3 表)。新 primitive 不要。統一 3 相 lifecycle(§3.5)の buffer インスタンス。**image(Stage 1)と同一 worker lifecycle**(`registerGpuUploadWorker(GPU_SUB_BUFFER, domain)`)。

## 1. 3 相の割り当て(トレース済の骨格)
- **①capture(main)** = `genDrawInfo`(`llvovolume.cpp:7272`)+ `staged.mDefer`(`:7278`)。DrawInfo を即構築せず `LLDrawInfoSnapshot` を `LLGeoStagedRebuild.mFaces[].mSnaps` に捕捉(`:7516`)。VB alloc は capture 内(`:7483`)。
- **②fill(off-main worker・buffer substrate)** = VB 頂点データ充填(`getGeometryVolume`→`flush_vbo` memcpy・§1.5)。**⚠️ 現状の deferred-fill 実行位置が未確定**(§4-O1)。
- **③fold(main)** = `applyGeoStaged`(`:5927`)の後半 = face 状態確定(`setVertexBuffer:6044`)+ `buildDrawInfoFromSnapshot`(`:6054`)+ `foldBuiltDrawInfo`(`:6077`→`group->mDrawMap`)+ `group->mBufferMap` 更新(`:6079`)。**main 単独必須**(render が読む構造)。

## 2. worker 配線(統一 lifecycle)
- worker run() 冒頭: `registerGpuUploadWorker(GPU_SUB_BUFFER, createRenderDomain())`(§5.3.1)= `setThreadAllocDomain`。
- worker iteration 末尾: `megaReclaimDomain(自 domain)`(§3.6.2 修正 = 所有スレッドが自 domain 回収・main の `tickMegaFreeQueue` は触らない)。
- worker run() 末尾: `unregisterGpuUploadWorker()` + `renderDomainReclaim(domain)`。
- finalize: 相③を mainloop WorkQueue へ postTo(§5.3.3)。

## 3. correctness(§3.6 の buffer 面)
- lifetime = `megabufRelease`→`megaEnqueueFree`(`:9348`)→`reapReady`(GPU 完了 gate `:773`)= 健全(§3.6.1)。
- **§3.6.2 修正必須**: worker domain の free を per-frame reclaim(§2 の worker iteration reclaim)。無ければセッション中メモリ膨張。
- race 検出器 `C_MEGA_RACE`(`:9402`)が worker domain の alloc/reclaim を監視 = gate オラクル。

## 4. O1-O4 トレース結果(潰した・重大結論あり)
### O1 = deferred-fill の実行位置【解決】
- `staged.mInline = true` を rebuildGeom が**常に設定**(`:7062`)→ `fill_inline`(`:7505`)は常に真 → **VB fill(`getGeometryVolume`)は常に main で inline**。`getGeometryVolume` の呼び出しは `:7181`/`:7599` の 2 箇所のみ・**両方 genDrawInfo 内・applyGeoStaged には無い**。
- ∴ **deferred-fill モード(mInline=false)はコードは在るが fill 実行器が存在しない**(dead な半実装)。`mDefer`(`:7594`)は tangents 未 ready 時の**中断→次フレーム再試行**用(`:7086` で return)であって「fill を後で走らせる」機構ではない。
- **結論**: mesh off-main = 「capture を mInline=false にして fill を抜き、**worker fill 実行器を新規に書く**」= 既存 wiring では済まない(新規コード必須)。

### O2/O4 = getGeometryVolume の off-main 安全性【★重大 = category error 直撃】
- fill の本体 `getGeometryVolume`(`:7599`)は周囲(`:7576-7609`)で **`vobj->updateRelativeXform(true/false)` で vobj 状態を mutate** し、`vobj->getVolume()`/`getRelativeXform()`/`getTextureEntry()`/LOD を**ライブ読み**する。
- これらは **main が毎フレーム更新する per-object 状態**。fill を off-main に出すと、移動中オブジェクトの xform/volume を main 更新と同時に read/mutate = **race**。
- = **過去に geo worker を撤退させた category error そのもの**(順序不可分の描画鎖を割った・avatar+SimRez+影 大崩壊の根)。buffer *substrate* は per-thread で安全だが、**fill の入力ソース(ライブ object 状態)が非分離**。
- **∴ mesh off-main fill は「そのまま worker に出す」と category error 再発**。回避には **geometry-input snapshot**(volume 頂点 + xform 行列 + TE を capture 時に main で不変コピーし、worker はその snapshot から fill)が必須 = fill を live object 状態から切り離す新設計 + snapshot コスト。

### O3 = trigger/queue【解決 = 存在しない】
- `applyGeoStaged` は `rebuildGeom` 末尾 inline(`:7122`)。**staged package を worker に渡す queue は無い**。off-main には capture→queue→worker fill→main fold post の queue 機構を新規に要する。

## 5. ★Stage 3 の最終判定(施主に上げる設計判断)
O1-O4 を追い込んだ結論 = **mesh off-main fill は texture と違い「dormant を wiring」では済まず、category error(geo worker 撤退の根)を回避する geometry-input snapshot の新設計が要る**。選択肢:
- **(A) mesh fill は main に残す**(image=texture のみ off-main)。mesh の main コスト(gupd churn)は別手(rebuild 有界化・dirty-mark 等)で個別対処。**blast radius 最小・category error 無し**。
- **(B) geometry-input snapshot を設計して mesh fill を off-main**。snapshot コスト(頂点+xform コピー)が fill off-main の利得を上回らないか要実測。**category error 回避が設計の主眼**。
- ⚠️ どちらも product/方針判断 = **施主決裁事項**。設計者は現時点 **(A) を推奨**(施主厳命「category error を繰り返すな」+ blast radius 最小 + texture off-main だけでも FPS 結合の主因 `fb_heap_default` を叩ける)。
- **本 doc の O1-O4 は全て機械的事実(file:line)まで解けた = 壁でない**。残るは (A)/(B) の product 決裁のみ。

## 6. スレッド化→撤退 残骸の監査(施主要望 2026-08-10「戻したから変な処理・バグがあれば一緒に直す」)
mesh geometry rebuild 領域を残骸/バグ観点で棚卸し。**active bug は未発見・dead code(スレッド時代の残骸)2 件 + latent 不完全 infra 1 件を確認**。
### DEAD(到達不能・確定)= 失敗したスレッド設計の残骸
- **D1: `applyGeoStaged` の `prebuilt` 引数 + 分岐**(`llvovolume.cpp:5938/6015/6047`)= 呼び出しは :7122 の 1 箇所のみ `prebuilt=nullptr` 既定 → **到達不能**。旧設計「worker が built_map を prebuild → main が fold」の残骸。
- **D2: deferred-fill モード(`mInline=false`/`fill_inline=false` 分岐)**= `staged.mInline=true` を :7062 で常に設定 → `fill_inline` 常に真 → capture-without-fill 分岐は**到達不能**。旧設計「fill を worker に defer」の残骸(fill 実行器も無し)。
- ⚠️ **重要 = これらは「保持すべき休眠 MT infra」ではなく「category error だった失敗設計の残骸」**。将来の正しい再スレッド化は snapshot 方式(§4・live 状態から切離)であって prebuilt/deferred-fill 方式ではない。∴ **将来の足場としての価値は無い**(むしろ動くように見えて動かない誤誘導コード)。
### LATENT(現状無害・off-main で顕在化)
- **L1: `tickMegaFreeQueue` が `sMegaMain` のみ reclaim**(`:9617`)= 直列の今は無害・worker domain 使用で膨張(foundation §3.6.2)。
### active bug
- live 経路(`mInline=true` 同期)に明確なバグは未発見。`mDefer` 再試行(:7086)も group が GEOM_DIRTY のまま retry = 正しい。snapshot 間接化(capture→`buildDrawInfoFromSnapshot`)は live・動作正で wasteful 気味だが bug でない。
### 判断(施主決裁)
- D1/D2 は dead ゆえ削除で**機能喪失なし**。ただし **shadow/MDI 隣接の高リスク領域**(flicker バグ履歴)= **texture Stage 1 に混ぜない**・独立の careful gate で。憲法 = agent は「無害」判定不可 → 削除は AYA 承認事項。
- **推奨 = D1/D2 は別チケットで撤去**(失敗設計の残骸ゆえ・保持は誤誘導)/ L1 は off-main mesh を採らない限り放置可(採るなら §3.6.2 で修正)。

## 申告(縮小・省略・解釈)
- mesh の CPU 基盤(`LLMeshRepoThread` raw LLThread)は現状のまま(§2.5)。
- 相② に出すのは VB fill のみ。cull/record/submit は main(§4-O4)。
- particle/GLTF 等 mesh 系の内部重複(sampler 解決相当)は Stage 3 着手時に棚卸し(未精査)。
- **本 doc は architecture-ready であって implementation-ready ではない**(O1-O4 が残る)。texture(Stage 1)は implementation-ready。
