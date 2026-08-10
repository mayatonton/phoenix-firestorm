# 動的 VB の GPU-copy 更新(設計正本・particle ぱちぱち根治)

制定 2026-08-10。担当 = 設計者(texture worker と同一セッション由来)。関連 = [[docs/vknative_dl_resource_foundation.md]](mega-buffer 資源モデル)。

## 1. 病理(実機確定済・2026-08-10)
**不変条件の破れ = 「in-flight frame が参照する mega slice の内容を CPU が上書きしている」**
- 書込点 = `LLVertexBuffer::flush_vbo`(llvertexbuffer.cpp・mega slice 永続 map への in-place memcpy・唯一の writer)。slice は `allocateBuffer`(genBuffer :809 / genIndices :823)で 1 回取得・以後固定。
- 症状の主犯 = particle: 毎 frame rebuild + **毎 frame カメラ距離 sort で並び替え**(`llvopartgroup.cpp:808`)→ in-flight frame(FRAMES_IN_FLIGHT=3)の draw が読む最中に頂点色/glow/位置が別の粒のものに差し替わる = **色汚染・glow 汚染・ぱちぱち**。再現 = LSL emitter 選択 → 編集ビーム粒が加わりパッキング変化 → ビーム色が他 particle に混入。
- **γ オラクルで実機名指し済み**: `vb_inflight_write`(llvertexbuffer.cpp `_unmapBuffer` 内・VKC=1 時のみ・指数間引き WARNS)。走行実測 = 65 秒で 16,384 件・mask=0x20c7(= `LLVOPartGroup::VERTEX_DATA_MASK` = particle)が主犯・mask=0x5(vertex+tc0 の単純 quad 系)も常連 = **病理は動的 VB 全域**。
- 供給層検証器(α=DrawData/β=heap view)が沈黙したのは守備範囲外だったため(particle の色は頂点データ)= 装置は「バカ」でなく層違い。

## 2. 失敗した修正 = orphan 化(2026-08-10・即 revert 済)
- 内容: flush 時に in-flight 参照 slice なら新 slice を取得し全 shadow を書き、旧 slice を deferred free。
- **結果 = 悪化 → GPU fault(device lost・alpha_post_water)**。
- **敗因(確定)**: per-draw 記録 memo(hit 率 99.4%・57 万 hit/5s)が「**slice offset は buffer の一生不変**」を前提に記録済みコマンド(bind offset / vertexOffset)を replay している。orphan が offset を毎 frame 変えたため、replay が解放済み range を指した。
- **教訓 = 「slice 不変」はアーキテクチャ前提**(memo・MDI テンプレ等の record キャッシュ群が依存)。offset を動かす系(orphan/ring)は前提破壊 = 不採。revert は完全(orphanVkSlices 削除・オラクルは温存・`vb_orphan` perf 欄と `isFrameInFlightVk` は残置)。

## 3. 採用設計 = 戦略 B「書き込みを GPU タイムラインに乗せる」
**不変条件を両立させる**: ①slice offset 永久不変(キャッシュ群の前提を守る)②in-flight frame の読む内容は不変。
- `flush_vbo` の VK 経路を「slice.mapped へ memcpy」から「**transient staging に memcpy + 当該 frame CB に `vkCmdCopyBuffer`(staging→slice)を記録**」へ変更。
- 正しさ: queue は単一 FIFO なので前 frame の GPU 実行が終わってから今 frame の copy が走る = 前 frame は旧内容を読む ✓ 今 frame の draw は copy 後(同一 CB 内 barrier)で新内容 ✓。Vulkan の動的バッファ更新の標準形。
- staging = per-frame transient(既存 one-shot staging と同様 VMA host-visible・frame 完了 gate で解放 or ring)。

## 4. open の解決(2026-08-10 トレース済・全て file:line)
- **O1 解決 = flush の 3 経路**: ①明示 unmap(rebuild 直後・update 相 = pass 外)②一括 `LLVertexBuffer::flushBuffers()`(**pipeline.cpp:5106 = updateGeom 末尾 = pass 外**・particle はここ)③setBuffer 時の遅延 flush(llvertexbuffer.cpp:1380 WARNS 付き例外 = pass 内あり得る)。→ **ハイブリッド**: pass 外 + in-flight のみ GPU copy・pass 内は従来直書き(残余はオラクルで可視・§5)。
- **O2 解決**: copy 記録先 = `getCurrentCommandBuffer()`(frame CB)。pass 内判定 = 既存 `isInRenderPassScope()`(llvkloader.h:1465)。frame CB 未開始(updateGeom が beginFrame 前に走るケースがあれば)は false 返しで直書き fallback(オラクルが数を教えてくれる)。
- **O3 解決 = barrier**: copy 前は不要(queue FIFO で前 frame 完・同 frame はまだ draw 未記録)。copy 後 = `VkMemoryBarrier{TRANSFER_WRITE → VERTEX_ATTRIBUTE_READ|INDEX_READ}` を TRANSFER→VERTEX_INPUT stage で 1 発(pending flag 制・`_unmapBuffer` 末尾で emit = 遅延 flush 経路も被覆)。
- **O4 解決 = staging arena**: per-frame ring(`FRAMES_IN_FLIGHT` slot × chunk vector)。frame slot 再利用時に cursor reset(stamp = sMonotonicFrameCount 比較・beginFrame hook 不要)。slot 再利用 = 当該 frame の GPU 完了済(既存 frame cycle の保証)。chunk = `createBufferVkImpl(cap, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, buf, alloc, &mapped, true)`(:3052・host-visible mapped)・既定 1MB・不足時追加。teardown = `megabufShutdown`(:9485)に destroy loop 追加。
- **O5 解決 = ハイブリッド確定 → ⚠️ 述語は §4.6 で改訂(2026-08-10 後段トレース)**: v1 述語 `mVkLastDrawFrame != 0 && isFrameInFlightVk(mVkLastDrawFrame)` は **MDI 族で構造的に破綻**(fire site は代表 VB のみ setBuffer = span メンバーの stamp が更新されない: lldrawpool.cpp:1404/:1572・lldrawpoolalpha.cpp:1011・lldrawpoolterrain.cpp:316)。採用 = **sticky 述語 `mVkEverConsumed`**(§4.6)。false(未消費 = 初回書き)は直書き = 従来コスト。静的 buffer の初期化は一生 copy 経路に入らない(初回 populate は消費記録前)。
- **O6**: index も同様(`MegaSliceI{buffer, mapped, offset, size}`・dst_offset = `offset + start`)。

## 4.5 実装手順 v2(詳細設計 2026-08-10・消費者被覆トレース済 = §4.6)
**不変条件**: INV-A = slice offset 一生不変(既存・§2)/ INV-B = **一度でも draw 記録に参照された slice への内容変更は GPU タイムライン上(copy)でのみ行う** / INV-C = copy と消費 draw は同一 CB 内で copy 先行(main thread 記録順 = 論理順・§4.6-3)。

1. **mega chunk usage に `VK_BUFFER_USAGE_TRANSFER_DST_BIT` 追加**(llvkloader.cpp:9414・現状 VERTEX/INDEX のみ = copy 不能)。
2. **llvkloader.cpp + .h**: staging arena(per-frame slot × chunk vector・chunk = `createBufferVkImpl(max(1MB, bytes), TRANSFER_SRC, …, host-visible mapped)` :3052 流用・slot 再利用時 cursor reset・teardown = `megabufShutdown` :9485 に destroy 追加。slot 安全性 = beginFrame が 3 CB 全 timeline 待ち :6491-6501)。
   - `bool vbStageCopyVk(VkBuffer dst, const VbCopyRegion* regions, U32 n)`(VbCopyRegion = {dst_offset, src, bytes}): `isInRenderPassScope()`(llvkloader.h:1465)or 記録先 CB 無し or n>16 → false。staging bump 確保(16B align・1 block 一括)→ memcpy → **1 回の `vkCmdCopyBuffer`(現記録先 CB・VkBufferCopy 配列)** → **直後に同一 CB へ `VkMemoryBarrier{TRANSFER_WRITE → VERTEX_ATTRIBUTE_READ|INDEX_READ|TRANSFER_WRITE}`(TRANSFER→VERTEX_INPUT|TRANSFER)を 1 発** → `++gVkPerf.vb_orphan`(欄流用・表示 :6003 `orph`→`vbcp`)+ `vb_copy_bytes` 加算。
   - **barrier 方式の確定(実装時改訂・pending 2 点方式を廃止)**: 理由 = pending flag は CB 切替(producer→consumer)で「copy と別の CB」に barrier が落ち得る + frame 跨ぎ消費(frame N 末尾 copy → N+1 draw)で barrier 欠落の穴。**copy batch 直後・同一 CB 内 emit なら全消費が queue 順で barrier の後 = 封緘が構造証明可能**。dst に TRANSFER_WRITE を含めることで copy-copy WAW(同 buffer 同 frame 複数 flush)も同時に閉じる(`mVkLastCopyFrame` 案は不要化)。dst=VERTEX_INPUT|TRANSFER なので後続 copy 同士の直列化はあるが件数規模(数十〜数百/frame・小 copy)で無害。barrier 数 = copy batch 数/frame。
3. **述語 = sticky `bool mVkEverConsumed`**(llvertexbuffer.h・§4.6-1)。set 地点 = 消費記録の全列挙点 6 箇所(各 1 行):
   a. `setBuffer`(llvertexbuffer.cpp:1427 帯・`mVkLastDrawFrame` stamp は診断用に残す)
   b. template build(llvkbucket.cpp:368 `rebuildTemplateIfDirty` の range loop :396-428・statics/dyn 両方 `info->mVertexBuffer`)
   c. alpha MDI `appendAlphaRunCmd`(lldrawpoolalpha.cpp:950-)
   d. rigged MDI item collect(lldrawpool.cpp:1590-)
   e. terrain MDI `appendTerrainRunCmd`(lldrawpoolterrain.cpp:280-)
   f. GLTF/その他 = setBuffer 経由(a が被覆)
4. **llvertexbuffer.cpp `flush_vbo`**(:1120-1154): 冒頭 `want_copy = mVkEverConsumed`。vertex branch: per-type `out` から `dst_off = out - mVkVertexSlice.mapped`(mapped = chunk base・region_offsets は chunk 絶対 offset)を **VkBufferCopy 配列に収集 → 1 発 copy**。index branch 同型(`mVkIndexSlice.offset + start`)。`vbStageCopyVk` false(pass 内/CB 無し/staging 失敗)→ 従来 memcpy + **新オラクル `vb_inflight_hostwrite`**(VKC gate・指数間引き・mask/verts/last_draw/inpass 付き)。
5. **旧オラクル削除**: `_unmapBuffer` :1177-1192 の `vb_inflight_write` block を撤去(役目 = 新オラクルに移管。⚠️ 検出器変更 = 憲法 4 = 本設計の AYA 承認に含める)。
6. **genBuffer 書込順入替**(llrender.cpp:1856-1877): `setBuffer()` を setPositionData/setTexCoord0Data/setColorData の**後**へ移動 = fresh-create の bind 先行 stamp による偽陽性(mask 0x5 の正体・§4.6-5)と無駄 copy の根治。
7. **gate**(§5 改訂): 視覚 = ぱちぱち・編集ビーム汚染消滅(AYA)/ `vb_inflight_hostwrite` **全沈黙**(0x20c7 も 0x5 も。残発火 = fail-closed で名指し)/ devlost 0 / memo hit 率不変(perf `memo=`)/ `vbcp` 実働 / validation 0。
8. **⚠️ 絶対に踏むな**: slice offset を変える系(orphan/ring)= §2 の GPU fault 再発。

## 4.6 消費者被覆の正当性(2026-08-10 全トレース・file:line)
1. **v1 述語の破綻**: MDI fire は代表 VB のみ `setBuffer()`(material lldrawpool.cpp:1404 / rigged :1572 / alpha lldrawpoolalpha.cpp:1011 / terrain lldrawpoolterrain.cpp:316)= span メンバー VB の per-frame stamp は構造的に維持不能。テンプレは dirty 時のみ再構築 = build 時 stamp も鮮度が腐る。∴ 鮮度依存を捨て sticky(一度消費されたら以後 copy)で機械化。
2. **memo は replay ではない**: vkCmdMemo 族(llvkloader.cpp:13765-)は冗長 bind の同値 skip。draw 記録自体は毎 frame 走る = 直接経路は毎 frame setBuffer 通過 = a で被覆。
3. **順序保証**: 影 worker は存在しない(撤去済・render の std::thread = PE :934 のみ)= **全 draw 記録は main thread 単線**。CB は consumer(UI)/frame/producer(scene)の 3 本 + aux UI(sUISceneSplit/Async = constexpr true :640-641 = 常時)。copy は「その時の記録先 CB」に記録 = 後続の消費 draw と同一 CB・記録順 = 論理順。updateGeom(llviewerdisplay.cpp:956)は producer begin(:878)の後 = particle copy は scene draw と同じ producer CB 内で先行 ✓。前 frame の CB 群は submit 済 = queue FIFO で copy より先に完走 ✓。
4. **staging 生存**: submit 順 = consumer→frame→producer(llvkloader.cpp:6403-6447)・slot 再利用は 3 CB 全 timeline 待ち後(:6491-6501)= per-frame slot reset は GPU 完了後のみ ✓。
5. **fresh slice 直書きの安全性**: slice 解放は `megaEnqueueFree`(:9461)→ `reapReady`(:9632・完了 frame gate)後にのみ再配布 = 新規 buffer の slice に in-flight 読者は存在しない ✓。UI scratch は内容 hash キャッシュ(llrender.cpp:1792)= hit 再書込なし・miss は新規 buffer(ただし genBuffer が bind 先行 = 偽陽性 → §4.5-6 で入替)。
6. **barrier 範囲**: 消費は attribute/index fetch のみ(vkCmdDispatch 0 件・rigged も頂点は attribute・skin 行列は別 SSBO)= dst `VERTEX_INPUT / VERTEX_ATTRIBUTE_READ|INDEX_READ` で全被覆。
7. **flush 全経路**: ①`flushBuffers`(pipeline.cpp:5106・llvovolume.cpp:7208・llvertexbuffer.cpp:1158)= 更新相 = copy 可 ②setBuffer lazy(:1394-1399)= pass 内あり得る → fallback+オラクル ③setPositionData 族の直接 flush_vbo(:1452-1559)= **旧オラクル圏外だった = 新オラクル/copy は flush_vbo 内で全経路被覆**。

## 4.7 工程
- **P1(1 build・全同梱)**: §4.5 の 1〜6 を一括実装(diff 規模 ~200 行・kill switch なし・切り分けはオラクル/vbcp で可能)。build/deploy = 設計者。
- **P2(判定走行)**: `AYASTORM_VKC=1 AYASTORM_PERF_LOG=5` 通常シーン + LSL emitter 編集ビーム再現。判定表 = §5。
- **P3(AYA gate)**: 視覚(ぱちぱち・ビーム汚染)+ 警報全欄。PASS 後 commit(粗め・checkpoint `56fa6cbcc26` と束ね squash は AYA 裁量)。

## 4.8 自己監査(correctness / gap / hidden)
- C1 copy-copy WAW(同 buffer 同 frame 複数 flush)→ per-batch barrier の dst に TRANSFER_WRITE を含め全 copy を queue 順直列化(§4.5-2)。
- C2 staging 不足 → chunk 追加(上限 = 要求 bytes)。追加失敗 → fallback 直書き + オラクル(隠蔽しない)。
- C3 in-pass flush fallback = 直書き + オラクル。genBuffer 入替(§4.5-6)後の残発火 = 真の欠陥標本として gate が名指し(fail-closed)。
- C4 dst_offset 算式: vertex = `out - mapped`(mapped = chunk base・chunk 絶対)/ index = `offset + start`。実装時に allocateBuffer の mapped 意味を file:line 再確認。
- C5 非 VK 経路(slice.mapped == null)= 完全 no-op 不変。
- C6 offscreen frame(beginOffscreenFrameVk :6478)も sInFrame 型 = 同型で被覆。
- C7 aux UI CB(override 保存復帰 :13365-13442)中の flush → copy は当該 CB = 後続 draw と同居 ✓・main frame 内なので staging slot 寿命同一。
- Gap1 barrier は pass 外のみ合法 → emit は copy 直後(copy 自体が `isInRenderPassScope` guard 済 = 同地点は常に pass 外)。
- Gap2 オラクル置換 = 検出器変更 = 憲法 4 → AYA 承認必須(本設計承認に含める)。
- Gap3 MDI fallback 枝(`flushAlphaRun` の vkCmdDrawIndexed 直叩き・mdi ring 枯渇)= setBuffer 経由 = 被覆内。
- Hidden1 semantic 変化: updateGeom **前**に記録される draw(dynamic texture 帯 llviewerdisplay.cpp:885-)は改修後「前フレームの整合スナップショット」を読む(現状 = 更新後データ・不整合リスク有)。1 frame 遅延・非破壊・整合性はむしろ改善。
- Hidden2 consumer(UI)CB は producer copy より先に GPU 実行 = UI が scene の動的 slice を直接読まない前提(UI = scratch VB・scene 画は texture 経由)。違反があれば視覚 gate で露呈。
- Hidden3 sticky の過剰 copy(大昔に 1 回 drawn → 以後 rewrite 毎 copy)= 正しさ優先で受容。量は `vbcp`/bytes 計で実測し、支配的なら後段で精密化(勝手にやらない)。
- 実装後監査-A(修正済): 旧 flush_vbo は純 memcpy = 任意 thread 安全だったが、copy 化で staging/frame CB(無ロック・main 前提)に触る = off-main 呼びが居た場合の新規ハザード。`vbStageCopyVk` 冒頭に `on_main_thread()` guard = off-main は旧挙動(直書き + オラクル可視)へ構造的に退避。
- 実装後監査-B(却下の記録): 「slice 再取得時に sticky を reset」案は不採。`mVkEverConsumed` は vertex/index 両 slice 共有のため、片 slice のみ再取得のケースで消費済み残 slice への直書きを再導入する。fresh slice への過剰 copy は安全側の無駄として受容。
- 実装後監査で追認: GLTF 経路 = setBuffer 被覆(gltfscenemanager.cpp:768)/ setPositionData 族に bind 前提なし(genBuffer 入替安全)/ 通常 beginFrame も slot 毎 3 timeline 待ち(llvkloader.cpp:5780-5792)/ slice 再取得は `buffer == VK_NULL_HANDLE` guard(llvertexbuffer.cpp:807/:821)+ destroyGLBuffer で slice clear(:894)。

## 4.9 設計 v2.1 = 専用 VB upload CB(2026-08-10・判定走行 1 巡目の帰結)
**走行事実**: `vb_inflight_hostwrite` 全 18 発が `inpass=1`(pass 外書換は 100% copy 化済・vbcp 実働・devlost 0)。源流 = `LLPipeline::postSort`(pipeline.cpp:4783)配下の `rebuildGeom()` 群(:4804-4811)が開いた rendering 下の呼び手(shadow 等の render 中 cull)からも走る。per-symptom に rebuild を動かすのではなく不変条件を差し替える:

> **INV-B'**: 全 mega-slice 書換 copy は **render pass を一切含まない専用 upload CB** に書換順で記録し、endFrame で当 frame の全 CB(consumer/frame/producer)より先に submit する。→ pass 内/外の区別が消滅(vkCmdCopyBuffer 常に合法)・frame 内意味論は従来の直書きと同一(全 draw が最終書換を見る = v2 Hidden1 の 1-frame 遅延も解消)・書換順序は単一 CB 内で保存。

構成(全て llvkloader.cpp・~80 行):
1. **状態**: `sVbUploadCommandBuffers[FRAMES_IN_FLIGHT]`(sCommandPool から遅延 allocate・pool destroy で解放)・`sVbUploadTimelineValue[]`/`sVbUploadPending[]`・`sVbUploadRecording`。
2. **`vbUploadCmd()`(遅延 begin)**: `!sInFrame` → null。初回呼びで reset+begin し、**先頭に WAR execution barrier(`VERTEX_INPUT → TRANSFER`・access 0・memory barrier 不要)を 1 発**記録。
   - **⚠️ 監査発見(v1/v2 O3 の誤りの是正)**: 「copy 前 barrier 不要 = queue FIFO で前 frame 完」は誤り。submission order は**開始順**であり完了順ではない = 前 frame の VERTEX_INPUT read と当 frame copy write の **WAR が未同期**だった。pipeline barrier の first scope は同一 queue の**先行 submit 全コマンド**に及ぶ(仕様)ため、CB 先頭の 1 発で全前 frame read の完了を copy 前に強制できる。cross-frame WAW は前 frame per-batch barrier(dst TRANSFER)が既に被覆。
3. **`vbStageCopyVk`**: 記録先を `currentRecordCmd()+isInRenderPassScope guard` → `vbUploadCmd()` に差し替え(main-thread guard・staging・per-batch post barrier は不変)。
4. **submit**: `vbUploadSubmit()` = end + `PEJob{slot, cmd}` を **endFrame の consumer/frame/producer enqueue より前**(:6395 の frame CB end 直後)と **endOffscreenFrameVk の peSubmitBlocking 前**(:6546)で enqueue。peEnqueue は mutex FIFO(:1327)= enqueue 順 = submit 順 ✓。peSubmitBlocking も同 FIFO 経由(:1348)✓。
5. **slot 生存**: `beginFrame`(:5780 帯)と `beginOffscreenFrameVk`(:6491 帯)の既存 timeline 待ち群に upload 待ちを追加 + `sVbUploadRecording=false`(前 frame の異常 return 掃除)。submit 失敗時は peExecute が timeline を abandoned 記録(:1119-1124)= 待ちはハングしない。

### v2.1 自己監査
- WAR(前 frame read vs copy write)= CB 先頭 execution barrier で閉(上記 2)。
- WAW(frame 内)= per-batch post barrier dst TRANSFER(不変)/(frame 跨ぎ)= 前 frame の同 barrier が先行 submit 全体を被覆。
- 順序保存 = 全 copy が単一 upload CB に記録順。v2 で成立し得た「frame CB と別 CB に copy が割れて順序逆転」の可能性も消滅。
- staging slot 生存 = upload timeline 待ちを両 begin 経路に追加(上記 5)。upload CB の reset は slot 待ちの後のみ。
- 異常系: begin/alloc 失敗 → null → 直書き fallback + オラクル(可視)。endFrame 早期 return → 次 begin で flag 掃除 + reset(当 frame copy 喪失 = stale 表示・故障級経路のみ・申告)。device lost → peExecute skip + abandoned 記録 = 非ハング。
- aux window CB が upload より先に実行される場合、当 frame copy 内容は見えない(UI scratch のみ = 実害なし・申告)。
- 空 frame(copy ゼロ)= flag false → submit なし = オーバーヘッドゼロ。
- 全接触点 main thread(vbUploadCmd は on_main_thread guard 通過後のみ・begin/end frame は main)。

## 旧 open(解決済み・履歴)
- **O1: flush の発生タイミング全数** — `_unmapBuffer`/`flushBuffers` の呼び出し文脈で「render pass(dynamic rendering)内」があるか。`vkCmdCopyBuffer` はパス外必須 → パス内 flush があれば (a) endDynamicRendering/resume bracket(FB copy :1691-1704 と同型)か (b) copy を defer し pass 後に record。
- **O2: copy 記録先** — `getCurrentCommandBuffer()`(frame CB)で良いか。update phase(rebuildGeom)は frame CB 記録中か・pass 外か。shadow pass 中の flush は?
- **O3: barrier** — copy 前: 前段 vertex read との WAR は「queue 順で前 frame 完」+ 同 frame 内は copy→draw 順で VERTEX_INPUT 向け dst barrier。copy 後: TRANSFER_WRITE → VERTEX_ATTRIBUTE_READ|INDEX_READ の barrier(範囲 or global)。頻度が高いので batch(frame 毎に 1 barrier に集約)を検討。
- **O4: staging 供給** — 既存機構の再利用候補: one-shot staging(vmaCreateBuffer 都度)は churn 大。per-frame staging arena(FRAMES_IN_FLIGHT ring・megabuf と同型の単純 bump allocator)を新設するか。サイズ実測(particle 全書換量/frame)。
- **O5: 対象の絞り** — 全 flush を copy 化するか、「in-flight 参照時のみ copy・初回(未 draw)は直書き」のハイブリッドか。初回直書きは安全(誰も読んでいない)= オラクル条件と同じ述語 `mVkLastDrawFrame` in-flight で分岐すれば copy 頻度最小。**推奨 = ハイブリッド**(静的 buffer の初期化は従来通り直書き・動的 rewrite だけ copy)。
- **O6: index buffer も同様**(mVkIndexSlice)。

## 5. gate(v2)
- 視覚(AYA): particle ぱちぱち・編集ビーム色汚染の消滅。
- 機械: 新オラクル `vb_inflight_hostwrite`(= 消費済 slice への host 直書き・copy 経路は発火しない)**全沈黙**(mask 0x20c7/0x2047/0x5 とも。genBuffer 入替で 0x5 偽陽性も消える設計 = 残発火は真の欠陥として名指し)。devlost 0。memo hit 率退行なし(perf 行)。`vbcp` 実働。validation 0。
- 憲法 1: PASS は AYA のみ。

## 6. 現状 = **根治完了・AYA gate PASS(2026-08-10)**
- 実装 = §4.5 v2(sticky 述語 + copy 化)+ §4.9 v2.1(専用 upload CB)。判定走行 2 巡: 1 巡目でオラクルが in-pass 残存経路(postSort 帯)を名指し → v2.1 で機械 gate 全欄グリーン(`vb_inflight_hostwrite` 0・devlost 0・新規警報 0・vbcp 12-28k 回/65-129MB per 5s・vbbind/memo 率不変)+ AYA 視覚 PASS(ぱちぱち・編集ビーム汚染消滅)。
- deploy 済 binary = この根治版(3-path 同期済)。
- 恒久検出器 = `vb_inflight_hostwrite`(flush_vbo 内・VKC gate)= 「消費済 slice への host 直書き」の fail-closed 検出。沈黙が正常。
