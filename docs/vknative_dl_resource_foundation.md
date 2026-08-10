# DL リソース統一土台(設計正本・master)

制定 2026-08-10。施主命題 = **texture だけでなく全 DL 資産(texture/mesh/animation/audio/汎用 asset)は同じ課題を持つ。土台はそれを意識し、最終的にどう整理して完成とするかまでを設計する**(でないと整理が未完に終わる)。本 doc = 統一土台の end-state + 完成定義 + 工程(roadmap)の正本。texture 段の詳細 = [[docs/vknative_texture_upload_worker.md]](Stage 1)。

---

## 1. 棚卸し(全 DL パイプラインの現状・実測 file:line)
DL は資産種別ごとに層(fetch → cache → decode → GPU-create)を持つが、**各層で 2-4 種の別実装が併存し統一基盤がない**。

| 層 \ 種別 | Texture | Mesh | Audio | 汎用 asset(anim/clothing/gesture/notecard/script/material/settings) |
|---|---|---|---|---|
| **fetch(HTTP)** | `LLTextureFetch : LLWorkerThread`(`lltexturefetch.h:56`)+HttpRequest。sw=`TextureFetchConcurrency` | `LLMeshRepoThread : LLThread`(`llmeshrepository.h:438`)+HttpRequest。sw=`MeshMaxConcurrentRequests` | asset 経由 | `LLAssetStorage`/`LLTransferManager`/`LLXferManager`(`llmessage/`・legacy UDP+HTTP) |
| **cache** | `LLTextureCache : LLWorkerThread`(`lltexturecache.h:41`) | LLMeshRepoThread 内 disk cache | LLFileSystem | LLFileSystem/VFS |
| **decode** | `LLImageDecodeThread`(`llimageworker.h:34`・独自) | LLMeshRepoThread 内 | `LLVorbisDecodeState`(`llaudiodecodemgr.cpp:56`)on `ThreadPool "General"` | anim = main で `deserialize` |
| **GPU-create(層α)** | image: `LLImageGL`→`VkTexResidency`→**staging+`vkCmdCopyBufferToImage`(one-shot cmd)** | buffer: `flush_vbo`(`llvertexbuffer.cpp:1120-1153`)= **mega-buffer 永続 map へ `memcpy` 直書き(cmd buffer 不使用)** | なし | なし(CPU データ) |

> ⚠️ **訂正(2026-08-10・実測)**: GPU-create は資源クラスで upload 機構が**根本的に異なる** → 単一 primitive で統一できない:
> - **image 系**(texture / cubemap `createCubeArrayImageVk:11315` / RT)= staging buffer + `vkCmdCopyBufferToImage` を **one-shot command buffer** で。off-main = **per-thread command pool primitive** が必須。
> - **buffer 系**(mesh VB/IB)= mega-buffer(永続 host-visible map)へ `memcpy` 直書き。**command buffer を一切使わない**。off-main の substrate = **per-thread `tAllocDomain`(`:360`)+ per-domain `MegaDomain` + `C_MEGA_RACE` 検出器 = 既に built**(command pool と対称・§3 表)。
> ∴ per-thread command pool は **image 側の欠落ピース**。buffer 側 substrate は既存。両者は「per-thread GPU リソース分割」の 1 原理の 2 実装 = **一緒に設計する**(施主指摘)。

**threading 基盤の分裂(核心の mess)**:
- `LLQueuedThread`→`LLWorkerThread`(`llcommon/`・旧)= texture fetch/cache のみ。
- raw `LLThread` = mesh(逸脱)。
- `LL::WorkQueue`/`LL::ThreadPool`(現代・mainloop pump もこれ)= audio decode / image decode / geo worker / **LLImageGLThread**(`LL::ThreadPool("LLImageGL",1)`)。
- `LLSingleton`/main = 汎用 asset・animation。
- GPU submit = 単一 `sCommandPool`(per-thread pool 未存在 = 層α の N-thread 化の壁。詳細 = texture doc)。

**共通しているもの(土台の芽)**: ①HTTP = `LLCore::HttpRequest`(texture/mesh 共通)②現代 CPU substrate = `LL::WorkQueue`/`ThreadPool` ③GPU = llvkloader one-shot primitive(`beginOneShotCommandBufferVk`/`submitOneShotVk`)。**統一はこの 3 芽の上に寄せる。**

## 2. 全 DL 資産が共有する課題(施主命題の分解)
1. **CPU substrate 不統一**(LLWorkerThread / raw LLThread / WorkQueue / main が混在)= debug・再並列化・kill-switch がバラバラ。
2. **GPU-create が main 貼付け or 単一 pool**(texture=main 予算に落ちて FPS 結合 / mesh VB=main record)= 層α の N-thread 化不能。
3. **finalize の main 復帰口がバラバラ**(texture=mainloop WorkQueue pump / 他=各自)。
4. **resolution/upload の車輪重複**(texture の sampler 解決が 2 関数複製 = texture doc PART E。他種別も同型の疑い)。
5. **kill-switch 族が不統一**(`TextureFetchConcurrency`/`MeshMaxConcurrentRequests`/`AYASTORM_MT_*` が別体系)。

## 1.5 全 GPU-DL リソースの完全マップ(2 機構で尽きる・実測で確認)
「全部把握してから設計」(施主厳命)= GPU に載る全 DL リソースの upload 経路を全数トレースした結論 = **例外なく 2 機構に還元。第3の機構は無い**:

| リソース | upload 機構 | 根拠(file:line) |
|---|---|---|
| texture(streaming) | **image**(staging+one-shot) | `LLImageGL`→`uploadImageDataVk:10142` |
| cubemap / reflection probe | **image** | `createCubeArrayImageVk:11233`→one-shot |
| render target | **image**(確保のみ) | `createColorAttachmentImageVk:9724` |
| terrain texture / paintmap | **image**(= texture) | `llvlcomposition.cpp:176`=`LLViewerFetchedTexture` |
| material(AT_MATERIAL) | **image に還元**(独自 upload 無・texture 参照のみ) | `llfetchedgltfmaterial.h:66-69` |
| mesh(VB/IB) | **buffer**(mega persistent map) | `genBuffer:799`→`megabufAcquire`→`flush_vbo:1120` memcpy |
| GLTF geometry | **buffer**(= LLVertexBuffer) | `gltf/primitive.cpp:399` |

- **∴ off-main GPU リソース upload = image 機構 + buffer 機構の 2 つだけを off-main 化すれば全 DL GPU リソースが載る**。composite は分解済ゆえ特別扱い不要。
- **区別(重要)**: 上記は **DL リソースの永続 upload**。別物 = **フレーム記録アリーナ**(DrawData `AllocDomain` / matrix ring `:193` / per-program UBO `:8231` / skin `:8483`)= 毎フレーム transient・これも thread_local+per-domain で分割済だが**用途は MT draw 記録(撤退済 geo worker 系)であって DL upload ではない**。混同しない。
- **現状の loading は image も buffer も main 上**(texture create=`updateImagesCreateTextures` / mesh VB=geometry rebuild の gupd churn)= **両方 NOT OK**(施主「他リソースは今の読み方でいいのか」への回答 = 良くない・両方 main 貼付け)。統一土台で両方 off-main が正解。

## 2.5 スコープ確定(施主 2026-08-10)= GPU に載る資源だけ
**対象 = GPU-consumed DL = texture / mesh(VB・IB)/ cubemap・RT のみ。** 非 GPU DL(**audio** / 汎用 asset = animation データ・clothing・gesture・notecard・script・settings・landmark 等)は**現状のまま=触らない**。
- 理由: 非 GPU 資産は `LLAssetStorage`/`LLTransferManager`/legacy transfer 層 = **波及範囲が広すぎ**、かつ層α(GPU)の payoff が無い。
- 好都合: texture/mesh は既に専用スレッド(`LLTextureFetch`/`LLMeshRepoThread`)で legacy asset 層と分離済 = **対象を絞れば legacy 層に一切触れずに済む**(blast radius が閉じる)。
- ∴ 本土台の**必須統一 = GPU substrate(per-thread pool)のみ**。CPU substrate の rebase は「GPU pipeline に直接資する & blast radius が許容」な範囲に限定し、それ以外の既存スレッドは**現状の CPU 実装のまま GPU-create stage だけを per-thread pool に差す**。

## 3. 統一 end-state(完成形の設計)
全 DL 資産を**同一形状のステージ pipeline**に載せる:
```
[Fetch(HTTP)] → [Cache] → [Decode] → [GPU-Create(層α)] → [finalize on mainloop]
   WorkQueue      WorkQueue   WorkQueue    per-thread pool      mainloop WorkQueue
```
- **CPU substrate = `LL::WorkQueue`/`LL::ThreadPool` に一本化**(現代・LLImageGLThread が既に採用)。旧 `LLWorkerThread`(texture)と raw `LLThread`(mesh)を段階移行。各 stage = WorkQueue task。
- **GPU substrate = 「per-thread GPU リソース分割」という 1 つの原理の 2 実装**(施主指摘 2026-08-10「texture と mesh は同一オブジェクトの GPU リソースとして一緒に読まれる = 一緒に設計しろ」で判明)。両者は**完全対称**で、**buffer 側は既に built・image 側(command pool)だけ欠落**:

  | | **image 系**(texture/cubemap/RT・one-shot upload) | **buffer/DrawData 系**(mesh VB/IB・mega map) |
  |---|---|---|
  | thread_local ハンドル | `t_cmdPool`(**新設**・texture doc PART B) | `tAllocDomain`(`llvkloader.cpp:360`・**既存**) |
  | domain/pool 作成 | `registerThreadCmdPool()`(新設) | `createRenderDomain()`(`:9574`・既存) |
  | thread に束ねる | register が `t_cmdPool` 設定 | `setThreadAllocDomain(id)`(`:9595`・既存) |
  | 分割実体 | per-thread `VkCommandPool` | per-domain `MegaDomain`+`AllocDomain`(`:9177`) |
  | race 安全 | 外部同期(per-pool free・PART B.3) | `C_MEGA_RACE` 検出器(`:9402`・既存) |
  | 回収 | `unregisterThreadCmdPool()`(新設) | `renderDomainReclaim(id)`(`:9583`・既存) |
  | upload 実体 | staging+`vkCmdCopyBufferToImage`(one-shot) | `megabufAcquire`+`flush_vbo` memcpy(`:9393`/`llvertexbuffer.cpp:1120`) |

  - **統一原理 = worker は起動時に自分の GPU リソース substrate(image=command pool / buffer=render domain)を acquire し、off-main で upload、finalize は mainloop へ**。
  - ∴ **off-main GPU リソース upload 土台 = この 2 substrate の対**。image 側の欠落(command pool)を埋めれば、image と buffer の両方が同一パターンで off-main 化可能になる。**mesh は「既存 domain substrate を使う worker を足す」= 新 primitive 設計ではない**(§5 Stage 4 訂正)。
  - 便宜 API(任意)= 両 substrate を 1 呼びで acquire する `registerGpuUploadWorker(domainId)`(= `registerThreadCmdPool()` + `setThreadAllocDomain()`)。image だけの worker は command pool のみ、buffer だけの worker は domain のみ、両方 load する worker は両方 = composable。
- **finalize = mainloop WorkQueue へ postTo**(LLImageGLThread の既存 pattern `postTo(queue, work, mainCallback)` を全種別の標準に)。
- **kill-switch = `AYASTORM_MT_*` master 族に統合**(master = `AYASTORM_MT_THREADS`・per-stage sub)。既存 `TextureFetchConcurrency`/`MeshMaxConcurrentRequests` は GUI 到達性を保ちつつ master 従属に。
- **resolution/upload は共有 helper 1 本ずつ**(sampler 解決・staging upload 等の重複を種別横断で畳む)。

## 3.5 統一 worker lifecycle(texture と mesh を両方トレースして確定・施主「texture 先行は危険」への回答)
image/mesh を**両方追い込んだ結果**、off-main upload は資源種別に依らず**同一の 3 相 lifecycle**に収束する(= texture 専用形を作る危険が無いことの確認):

| 相 | texture(image) | mesh(buffer) |
|---|---|---|
| ①**capture / decode**(main) | fetch→decode(既存 thread)→create 要求 | `genDrawInfo` + `staged.mDefer`(`llvovolume.cpp:7278`)= DrawInfo/fill 作業を `LLGeoStagedRebuild.mFaces`(`:5877`)へ捕捉 |
| ②**fill / upload**(off-main worker・substrate 必要) | `createTexture`→one-shot(**image substrate = command pool**) | `applyGeoStaged`(`:5927`)の fill = VB alloc+`getGeometryVolume` memcpy(**buffer substrate = alloc domain**) |
| ③**finalize / fold**(main) | `postCreateTexture`(`llviewertexture.cpp:1787`)= bindless slot 反映 | `foldBuiltDrawInfo`(`:6437`)= `group->mDrawMap`(draw record)へ反映 |

- **worker lifecycle**(両者共通)= 起動時 `registerGpuUploadWorker(domainId)`(必要な substrate を acquire: image=pool / buffer=domain / 両方=両方)→ ②を off-main → ③を mainloop postTo で main finalize。
- **既存資産の確認(3 つとも built・dormant)**: buffer substrate(`tAllocDomain`/`MegaDomain`/`C_MEGA_RACE`)+ staged rebuild 分離(`LLGeoStagedRebuild`)+ finalize の mainloop pump。**欠落は image substrate(command pool)のみ**。
- **∴ texture(Stage 1)は「texture 専用形」ではなく、両資源から設計した統一 lifecycle の最初のインスタンス**。mesh(Stage 3)は同 lifecycle を buffer substrate + 既存 staged rebuild で埋める。**危険(texture 形が mesh に嵌らない)は、mesh を先にトレースして cut が同一と確認したことで解消**。

## 3.6 resource-access correctness 監査(施主「無駄な処理・やってはいけないアクセス・紛れたバグ」2026-08-10)
束ねる前に既存の resource-access を監査。**核心は健全・具体的な潜在バグ 1 件 + 無駄 2 件を発見**。

### 3.6.1 健全と確認(use-after-free の山ではない)
- **readiness**: `VkTexResidency::ensureSlot`(`llimagegl.cpp:1419`)= view が NULL なら **slot を作らず INVALID 返し**(:1427)→ 未 ready は白 default(`vkHeapSlotOrDefault:1468`)。**garbage を指す slot を作らない**(identity 安全)。
- **lifetime(image slot / buffer slice / DrawData slot 全て)**: 解放は全て **deferred + GPU 完了 gate**。`publish`(:1391)→`bindlessReleaseSlotDeferred` / `megabufRelease`→`megaEnqueueFree`(:9348)/ DrawData slot、いずれも `reapReady`(`:773` = `enqueue_frame <= sLastCompletedMonotonic` = **GPU 完了フレーム**)で回収。in-flight draw が旧 slot/slice を読む前に再利用しない。**設計として正しい**。
- race 検出器 `C_MEGA_RACE`(:9402)/`C_DRAWDATA_RACE`(:9551)が per-domain で監視。

### 3.6.2 ★潜在バグ(off-main で顕在化・foundation が塞ぐ)= worker domain の free が per-frame 回収されない
- `tickMegaFreeQueue`(`:9602`)は **`megaReclaimDomain(&sMegaMain)` のみ**(:9617)= **main domain の pending free しか per-frame 回収しない**。worker が `createRenderDomain` で作った domain の free は `renderDomainReclaim`(teardown)まで溜まる。
- 現状は直列(worker domain 未使用)ゆえ無害だが、**off-main mesh(Stage 3)で worker domain を使った瞬間、slice free がセッション中ずっと溜まる = メモリ膨張/枯渇**。まさに施主の言う「dormant を活性化した瞬間に出る紛れたバグ」。
- **foundation の要件に追加**: off-main 有効時、`tickMegaFreeQueue` は全 active domain(または各 worker が自 domain)を per-frame reclaim すること。

### 3.6.3 無駄(wasteful)= 未 ready アクセスと draw 時 slot 確保
- **`fb_heap_default` 50万/10s** = 大量の draw が未 ready テクスチャを白で描く**無駄な描画**。根 = main 律速 create(本改修 Stage 1 が直接削減)。
- **`ensureVkSlot()` を draw hot path で呼ぶ**(`vkHeapSlotOrDefault:1462`)= slot 確保が描画記録中。理想は commit 時に slot 化(off-main worker の finalize で確定)= draw 時 ensure を稀にする。
- identity バグ(particle 色汚染)は lifetime 誤りでなく **β 層(slot 番号は正・descriptor が residency churn で別 view に上書き)**= 別途 MDI supply verifier([[design_mdi_supply_verifier]])が狙う狭いバグ。lifetime 監査(本節)とは別命題。

## 4. 完成定義(整理完了 = done の判定)
**GPU-consumed 資源(texture/mesh/cubemap・RT)**が以下を満たしたとき「整理完了」(非 GPU 資産は §2.5 で対象外):
- (a) **image 系**(texture/cubemap/RT)の off-main upload が全て per-thread pool primitive 経由(単一 sCommandPool を worker から直に触るゼロ)= **必須の中核**。buffer 系(mesh)は別 primitive(§1 訂正)ゆえこの条件の対象外・Stage 4 で個別に done 定義。
- (b) finalize が mainloop WorkQueue postTo に統一。
- (c) 車輪重複ゼロ(同一処理の copy-paste が種別間・関数間に残らない)。
- (d) GPU pipeline の kill-switch が `AYASTORM_MT_*` master 族に従属。
- (e) 各 GPU stage × 各種別が「1 箇所で読める」= debug 可能。
- (※)CPU substrate の WorkQueue 一本化は「GPU pipeline に資する & blast radius 許容」時のみの任意目標(§2.5)= done の必須条件でない。
- **gate = 各段で憲法 2(警報全欄ゼロ)+ 視覚(AYA)。品質トレード無し。**

## 5. 工程(roadmap・統一土台・実装は gate 単位だが設計は分離しない)
> 原則 = **設計は 1 つの統一土台(image+buffer を分離しない・§3)**。実装は gate 単位で刻むが、それは「別設計」ではなく「同一土台の段階施工」。**分離設計は禁止**(施主 2026-08-10「別々は危ない・全部把握せず設計すると訂正で終わる」)。

- **Stage 0 = 統一土台の確立**【本 doc】: 全 GPU-DL リソース = 2 機構で尽きる完全マップ(§1.5)+ per-thread substrate 対(image pool 新設 / buffer domain 既存・§3)+ 統一 worker lifecycle API `registerGpuUploadWorker(domainId)`。**この設計が image/buffer 双方を同時に規定する**。
- **Stage 1 = image substrate 施工 + texture worker**【texture doc PART B-D】: 欠落している image 側(command pool)を built。texture が最初の image 利用者(FPS 結合の実修正)。**buffer substrate は既存ゆえこの段で「対」が揃う**。gate = 層0-2 + `fb_heap_default` 有意減。**【状態 2026-08-10: 初回実装は IIL(3 手全滅)で全 revert(texture doc「失敗の記録」)。再実装の正本 = texture doc PART J(publish-on-main)= 旧 段6/6b/PART I の gate 方式は不採用】**
- **Stage 2 = bind 集約**【texture doc PART E】: sampler 解決の重複 2 関数を helper 1 本に(検出器等価を独立監査)。
- **Stage 3 = mesh VB/IB off-main upload【O1-O4 トレース完了・重大結論 = category error 直撃】**: buffer *substrate* は built だが、**fill の本体 `getGeometryVolume`(`llvovolume.cpp:7599`)が live object 状態(vobj xform を mutate `:7578/7607`・volume/LOD/TE を read)を触る = geo worker 撤退の category error 直撃**([[docs/vknative_mesh_upload_worker.md]] §4)。現状 fill は常に main inline(`staged.mInline=true :7062`・deferred-fill は dead 半実装)。∴ mesh off-main は「wiring」でなく **geometry-input snapshot の新設計(fill を live 状態から切離)+ worker fill 実行器の新規実装**が必須。**判定 = (A) mesh fill は main 維持〔設計者推奨〕/ (B) snapshot 設計して off-main = 施主決裁**(mesh doc §5)。`LLMeshRepoThread` の CPU 基盤は現状のまま(§2.5)。
- **完成 = §4 を texture+mesh(+cubemap/RT)について達成**。image と buffer が同一 worker lifecycle・同一 finalize で off-main 化 = **オブジェクトの GPU リソース(mesh+texture)が両方 main から外れる**(施主命題の達成)。以後「新 GPU 資産もこの 2 機構のどちらかに載せるだけ」。
- ~~audio / 汎用 asset~~ = スコープ外(§2.5・非 GPU)。

### 5.1 mesh off-main 境界のトレース結果(分離機構は既存・dormant)
`LLSpatialGroup::rebuildMesh`/`LLSpatialPartition::rebuildMesh`(`:516`)は空だが、**capture/apply 分離は `LLVolumeGeometryManager` 側に既存**:
- `LLGeoStagedRebuild`(`llvovolume.cpp:5871`)= `mDefer`/`mInline`/`mFaces`/`mBufferMaps` で **fill を defer 捕捉**。`fill_inline = staged->mInline`(`:7505`)で inline/defer 切替。
- capture = `genDrawInfo`(`:7272`・`staged->mDefer` 分岐 `:7278/7597/7620`)/ apply = `applyGeoStaged`(`:5927`・再検証付き)/ fold = `foldBuiltDrawInfo`(`:6437`→`group->mDrawMap`)。
- **現状は全て main で走る**(off-main 痕跡=`setThreadAllocDomain`/`MT_GEO` 参照ゼロ = geo worker 撤退で dormant)。§3.5 の 3 相 cut が構造として在る。
- ∴ mesh off-main = 「apply 相(fill)を off-main worker(buffer substrate)に出し、fold 相を main finalize に残す」配線。**cut は既存ゆえ rebuildGeom を新規に切り分ける必要はない**(当初「非自明・要新設計」は訂正 = 機構は built)。残点 = ②apply の fill 部分が buffer substrate 越しにスレッド安全に走るか(再検証ロジック・group 状態参照)の実装時確認。
- **結論**: mesh(Stage 3)は image(Stage 1)と同一 lifecycle(§3.5)。別 gate は「施工リスク分離」であり「設計分離」ではない(施主厳命に整合)。

### 5.2 スコープ外の確認(skeleton/skinning・施主 2026-08-10)
skeleton/skinning は **DL GPU リソースでない**ので対象外(main 保持で正):skin weights = mesh VB 属性(buffer 機構に内包)/ skinning matrices = `sObjectSkinUpBuf`(`:8483`)+ matrix ring = **frame 記録**(per-frame transient・§1.5 の frame アリーナ側)/ skeleton 構造 = CPU。CLAUDE.md「palette build=NON-TARGET・実 skinning は GPU」= 探索済標的外。

## 5.3 統一 worker lifecycle 詳細設計(API 仕様・実装可能レベル)
§3.5 の 3 相 lifecycle を実装可能な API に落とす。**image/buffer を 1 つの worker lifecycle で扱い、必要な substrate だけ acquire**(composable)。

### 5.3.1 substrate acquire/release(llvkloader.h / .cpp)
```
enum GpuUploadSubstrate { GPU_SUB_IMAGE = 1u, GPU_SUB_BUFFER = 2u };

// worker thread の run() 冒頭で呼ぶ。image=command pool 作成+t_cmdPool 設定 /
// buffer=setThreadAllocDomain(domainId)。domainId は buffer 時のみ有効(呼び手が createRenderDomain() で採番)。
bool LLVKLoader::registerGpuUploadWorker(U32 substrateMask, U32 domainId /*=0*/);
// run() 末尾。image=自 pool drain+destroy(§texture doc G.1)/ buffer=自 domain の pending reclaim +
// (worker 専有 domain なら) 何もしない(domain 実体は呼び手が renderDomainReclaim で teardown)。
void LLVKLoader::unregisterGpuUploadWorker();
```
- 内部: IMAGE bit → `registerThreadCmdPool()`(§texture doc B.4)。BUFFER bit → `setThreadAllocDomain(domainId)`(既存 `:9595`)。
- **texture worker** = `registerGpuUploadWorker(GPU_SUB_IMAGE, 0)`。**mesh worker** = `registerGpuUploadWorker(GPU_SUB_BUFFER, myDomain)`。両方 load する worker = `GPU_SUB_IMAGE|GPU_SUB_BUFFER`。

### 5.3.2 fill/upload(相②・off-main)
- image: `createTexture`→one-shot(`beginOneShotCommandBufferVk`/`submitOneShotVk` が `threadCmdPool()` 経由・§texture doc B.2)。
- buffer: `applyGeoStaged` の fill = `megabufAcquire`(`tAllocDomain` 経由)+`flush_vbo` memcpy。
- **どちらも自 substrate のみ触る**(external-sync/race 不変条件は substrate 側で成立)。

### 5.3.3 finalize/fold(相③・main)= 統一 postTo
- worker は完了後、**mainloop WorkQueue へ finalize callback を postTo**(既存 `LLImageGLThread::postTo(mImageQueue, work, mainCB)` pattern を両資源の標準に)。
- image finalize = `postCreateTexture`(bindless slot は commit で確定済ゆえ主に ref/mNeedsCreateTexture 後始末)。
- buffer finalize = `foldBuiltDrawInfo`(`group->mDrawMap` 反映)= **main 単独**(draw record 構造の変更ゆえ)。
- **不変条件**: 相③(main の record/slot 反映)は必ず相②(off-main fill)の GPU 提出後・同一 PE timeline 上。PE 単一 submitter が upload→draw のキュー順を保証(§texture doc A.2)。

### 5.3.4 correctness 修正の統合(§3.6 の 3 件)
- **[§3.6.2 修正] worker domain の per-frame reclaim**: `tickMegaFreeQueue`(`:9602`)は main domain 専用のまま(main が他 domain の chunk free range を触ると worker の `megaAllocRange` と race)。**正しい修正 = 各 worker が自ループで自 domain を reclaim**(`megaReclaimDomain(myDomain)` を worker iteration 末尾で)= command pool の per-thread free と同一原理(所有スレッドが自 substrate を回収)。`unregisterGpuUploadWorker` は最終 reclaim も行う。
  - ∴ **reclaim も substrate acquire と同じ「所有スレッド責務」で一様化**(image free / buffer reclaim が同じ規律)。
- **[§3.6.3 修正] ensureVkSlot を draw hot path から外す**: off-main では commit(相②)で slot 確定(`publish`→`bindlessAcquireSlot`)。draw 時 `vkHeapSlotOrDefault` の `ensureVkSlot()`(`llimagegl.cpp:1462`)は「ready だが未 slot」の稀 fallback に退化(off-main 化で自然消滅)。**追加コード不要**・計器(fb_heap_default)で退化を確認。
- **[§3.6.2 の別面] fb_heap_default 減少**は Stage 1 の gate 指標(§texture doc gate)。

## 5.4 自己監査(統一 lifecycle 詳細設計・correctness/gap/hidden)
- **[C1] composability の race**: texture worker(IMAGE のみ)は `tAllocDomain=sMainDomain` のまま。texture create 経路が mega-buffer/DrawData を触ると main と sMainDomain で race。→ **texture create は image staging(`vmaCreateBuffer`・VMA 内部同期)のみで mega 非接触**と判断。ただし **hidden = Stage 1 実装時に「texture worker が buffer domain を一切触らない」を file:line で確認**(H1・下記 open に追加)。
- **[C2] reclaim 所有権**: 各 domain は所有者が reclaim(main=sMegaMain via `tickMegaFreeQueue` / worker=自 domain via iteration reclaim)= 全 domain 被覆・追加は main 経路不変(additive)。✓
- **[C3] finalize 順序**: 相②(off-main upload)は PE timeline に submit・相③(main finalize)は CPU 側 bookkeeping(slot は commit で公開済)。GPU の upload→draw 順序は PE 単一 submitter のキュー順 + 次フレーム draw で成立。✓
- **[G1] BUFFER branch 未検証**: `registerGpuUploadWorker` の API は image/buffer 両対応で定義したが、**BUFFER 分岐の実使用(mesh)は Stage 3 の O1-O4 が閉じるまで未検証**。Stage 1 は IMAGE 分岐のみ行使。API 定義は先行して良いが BUFFER 分岐の gate は Stage 3。
- **[G2] Stage 1 の API 整合**: texture doc は `registerThreadCmdPool` 直呼びだが、**Stage 1 で `registerGpuUploadWorker(GPU_SUB_IMAGE,0)` を導入し内部で `registerThreadCmdPool` を呼ぶ**形に統一(mesh が同 API を BUFFER で拡張)。texture doc PART C を本形に更新すること。
- **[H3] finalize pump の gating**: mainloop WorkQueue pump は現状 `sEnabledTextures`(`llstartup.cpp:627`)で駆動。mesh finalize も同 pump を使うなら **pump 条件を texture 専用から外す**必要(Stage 3 O5 として追加)。Stage 1(texture)単独では現状 pump で足りる。
- **H1 解決済(2026-08-10)**: texture worker は buffer domain 非接触を確認(`tAllocDomain` 使用は mesh/描画記録の 5 関数のみ・llimagegl の create 経路は buffer アロケータ呼び出しゼロ・共有状態は per-thread/mutex/atomic で保護)。詳細 = [[docs/vknative_texture_upload_worker.md]] §C.1。
- **残 open**: Stage 3 の O1-O4(mesh = (A) 判定で棚上げ)+ O5(finalize pump 汎用化 = mesh 時)。**Stage 1(texture)に実装前 open は無し**。

## 5.5 🔒 恒久ルール = 「MainThread ありき」の隠れ前提をスレッド移設でバグ化させない(AYA 制定 2026-08-10)
**問題の構造**: コードには自覚なき main-thread 前提が散在する(例 = L1 `tickMegaFreeQueue` が `sMegaMain` だけ回収 `:9617`)。**気づかずスレッドへ移すと静かにバグ化し、race でなく leak 系は即時エラーも出ず誰も気づかない**。read して全て見つけるのは不可能(必ず見落とす)。∴ 「探す」でなく **「構造で防ぐ + 検出器で炙る」** に倒す。

### R1. main 特例を構造から消す(所有者責務の一様化)
per-thread リソースは **「main 用」という特別扱いを作らない**。「各スレッドが自分の substrate/domain を確保・使用・回収する」一様形にする(if-sprawl 禁止と同根)。
- 例 = one-shot pool free は `threadCmdPool()` の自 bucket のみ(§B.3)/ mega domain reclaim は「所有スレッドが自 domain を reclaim」(§3.6.2)。
- こうすれば **worker を足しても自動的に正しく回る** = 静かに漏れる余地が構造的に消える。**新しい per-thread リソースを足すときは必ず「main 特例が無いか」を設計時に確認**。

### R2. 静的トレースで safety を証明せず、fail-closed 検出器で炙る(CLAUDE.md「観測可能にせよ」の適用)
スレッド前提が破れた瞬間に鳴る検出器を armed にして実機で炙る。既存 = `C_MEGA_RACE`(`:9402`)/`C_DRAWDATA_RACE`(`:9551`)= 領域を 2 スレッドが同時に触ると発火。
- **移設プロトコル**: worker 有効化 → 全検出器 armed → 実機の実負荷で回す → 検出器が破れた前提を**名指し** → 各違反を潰す。**読んで安全を主張しない・検出器沈黙で証明する**(本 project の gate 原則)。

### R3. leak 系の隙間を埋める
race 検出器は **漏れ(leak)を鳴らさない**(じわじわ増えるだけ)。leak 系は **① R1 の構造対処(所有者回収)を第一** とし、構造で消せない分だけ「per-domain 回収待ち量/budget アラーム」を足す(fail-closed)。

### R4. 失敗したスレッド設計の残骸は「休眠 infra」と区別して撤去候補にする
過去の category error(順序不可分の描画鎖を割った)由来の半実装(例 = mesh の D1 prebuilt 経路 / D2 deferred-fill・[[docs/vknative_mesh_upload_worker.md]] §6)は、**正しい再スレッド化(snapshot 方式)では使わない = 将来の足場でない**。「動くように見えて動かない誤誘導コード」ゆえ [[project_serial_recording_is_temporary]] の「休眠 MT infra を撤去するな」とは区別し、AYA 承認の上で撤去候補とする。

## 6. スコープ規律(前任 3 名解雇の逆を貫く)
- **設計は全体像を持つ**(本 doc)= 矮小化しない。だが**実装は Stage を 1 つずつ gate**= ocean を沸かさない。両立が本 doc の役割。
- 各 Stage 着手時に readiness 宣言 + 自己監査 + 独立監査(構造改修)。
- **他者の裁定待ちコード(TEXPOKE 等)は勝手に触らず AYA 裁定へ**。
- 本 doc = 唯一の正本。Stage 詳細は本 doc から派生し乖離させない(Brief に設計を書かない = 解雇理由②)。

## 7. 申告(未確定・実装前に潰す)
- mesh/audio/汎用 asset の内部重複(sampler 解決相当)は未精査 = 各 Stage 着手時に棚卸し(本 doc の texture 棚卸しと同手順)。
- 旧 `LLWorkerThread`/`LLQueuedThread` を WorkQueue へ移すコストと安定性は Stage 4/5 で個別評価(load-bearing = 慎重)。
- 層β(CPU DL の口が main 等)の各種別の実害度は未計測 = Stage 着手前に focus 実測(env_measure_requires_viewer_focus)。
