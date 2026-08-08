# VK-native draw record 置き場・消費契約(record store model)

status: 正本(AYA 承認 2026-08-03)
根拠 HEAD = `7ce26185d0d`。層 doc 系譜 = ubo_supply(層1)/ drawdata_slot(層2)/ layout_discipline(F4)と同格の foundation。改訂は設計者経由。

## §0 目的
draw record(LLDrawInfo)の「どこに置かれ・誰がどう読むか」は GL 移植の暗黙構造のまま増築されてきた(renderMap 直 walk が pool/pipeline に散在)。本 doc はこれを**宣言された契約**にする。bucket 化(= pass の置き場差し替え)を「消費者の全数改修イベント」から「registry 1 行の変更」へ変えるのが目的。影 alpha MDI 段はこの契約の行使段(層設計の実地証拠)。

## §1 置き場モデル(3 つ・正本は 1 つ)
| 置き場 | 実体 | 寿命 | writer(全数) |
|---|---|---|---|
| **mDrawMap(正本)** | LLSpatialGroup::mDrawMap(llspatialpartition.h:421)| group rebuild 間 persistent | foldBuiltDrawInfo(llvovolume.cpp:6380)/ LLGrassPartition(llvograss.cpp:723)/ LLParticlePartition(llvopartgroup.cpp:827) |
| renderMap(派生) | LLCullResult::mRenderMap[pass] | 1 cull(frame×camera) | postSort のみ(pipeline.cpp:5064-5090) |
| bucket(派生) | LLVKBucket::Bucket(pass×region)| persistent(patch 差分) | patchGroup のみ(llvkbucket.cpp:188) |

- pass 割当は書き手で確定: `s.mPassType = rigged ? (type+1) : type`(llvovolume.cpp:6072・key :6236)。**mAvatar ⇔ RIGGED は同時セット/クリア**(llvovolume.cpp:6728-6730/6743-6744)∴ 非 rigged pass の record は mAvatar==null が不変条件(例: mDrawMap[PASS_ALPHA] に rigged record は構造的に存在しない)。
- 派生 2 つは正本のフィルタ済み写像: renderMap = 当該 cull の可視 record を pass ごとに flatten(bucket 化 pass は skip = :5067-5070)。bucket = kBucketizedPasses の record を group 単位に常駐複製 + per-frame 可視 bit(setBucketVisible :5056 / forEachVisible の visBits gate)。
- patchGroup の発火 = 正本更新点の全数: 汎用 rebuildGeom(llspatialpartition.cpp:577・grass/particle 含む)/ volume staged apply(llvovolume.cpp:6024)/ record evict(llspatialpartition.cpp:310)。evict = :172/:268。**patchGroup は kBucketizedPasses のみ走査(llvkbucket.cpp:193)= registry 外 pass の書き手は bucket に触れない。**
- alpha の順序ソート入力(pushAlphaGroup/pushRiggedAlphaGroup pipeline.cpp:5092-5128)は mDrawMap.find による group 収集 = renderMap skip の影響外。

## §2 消費契約(正準)
1. **順序 bound 消費者は正本直読**。名簿(全数・閉集合):
   - LLDrawPoolAlpha::renderAlpha(lldrawpoolalpha.cpp:1281)— camera alpha。距離ソート済み alpha group 列 → group->mDrawMap[PASS_ALPHA(_RIGGED)]。
   - (準名簿)renderAlphaHighlight(lldrawpoolalpha.cpp:552)— debug highlight。同じ alpha group 経由。
   - 名簿への追加 = 本 doc 改訂事項。
2. **順序自由の描画消費者は全員 forEachSource(llvkbucket.h:115)**。renderMap/bucket の選択は forEachSource の内部事情であり、消費者は置き場を知らない。**beginRenderMap の新規直呼びは禁止**(正準入口の内側 :122 を唯一の例外とする)。既存直 walk は §4 台帳で収斂。
   - rigged pass・GLTF pass 等「bucket 化しない pass」でも forEachSource を使う(renderMap walk に縮退 = 等価・ゼロコスト)。契約を pass の現在の置き場で場合分けしない。
3. **MDI emit は forEachSource の上流の明示入口のみ**: textured = pushBatches(lldrawpool.cpp・isCameraMdiPass+heap gate)/ untextured = pushUntexturedBatches(影 opaque MV の経路)/ materials pool 入口(lldrawpoolmaterials.cpp・E 方式)/ rigged = pushRiggedBatchesIndirect(lldrawpool.cpp・renderMap walk-accumulate・head 2 つ = pushRiggedBatches[影 opaque rigged = opR]+ pushRiggedMaskBatches[影 MV mask rigged = amR/fbmR/gmR]・資格 = riggedMdiEligible[skin set + mVkPerDrawSupplySlotComplete]、mask head は + heap + mVkShadowCutoffFromSlot・非適格 record は dyn lane で per-draw 併走 = 母集団完全)。新 MDI 入口の追加 = 本 doc 改訂事項。
4. **層内部・maintenance・debug の mDrawMap 直読は契約外**(正本は常に真実なので直読は常に正しい): postSort assembler / patchGroup ingest / erase・orphan・validate(llspatialpartition.cpp:146-210, 239, 273, 387)/ texture 連動(llvovolume.cpp:2578, 5911・pipeline.cpp:2177 LLOctreeDirtyTexture / :2910 check_references)/ debug 描画(pushVerts 1664 / pushVertsColorCoded 1761 / setTextureAreaDebugText 3522 / renderSoundHighlights pipeline.cpp:4881)/ 存在 probe(llspatialpartition.cpp:829)。

## §3 pass→置き場 registry
- **kBucketizedPasses(llvkbucket.cpp:35-49)が唯一の registry**。pass を載せる = ①postSort が renderMap 積みを止める(:5067 は本契約の実装点)②patchGroup が ingest を始める ③forEachSource が bucket walk に切り替わる。
- **bucket 化の前提条件(チェックリスト)**: (a) 当該 pass の renderMap 直 walk 消費者がゼロ(全員 forEachSource 済)であること(§4 台帳)。(b) 順序 bound 消費者がいる pass は、その消費者が正本直読であること(PASS_ALPHA = renderAlpha が該当・充足済)。(c) 書き手の rebuild 頻度を確認(下 §5)。
- 現 registry 13 = SIMPLE / FULLBRIGHT / SHINY / BUMP / FULLBRIGHT_SHINY / 材質非 MASK 8。
- 拡張予定(影 alpha MDI 段・AYA gate 対象): ALPHA_MASK / FULLBRIGHT_ALPHA_MASK / MASK4(MATERIAL_ALPHA_MASK・SPECMAP_MASK・NORMMAP_MASK・NORMSPEC_MASK)/ GRASS / **ALPHA(再判定で 1 系統一が確定・§4)**。

## §4 census(全数・2026-08-03)と収斂台帳
### 適合済(forEachSource)
pushBatches(lldrawpool.cpp:1217)/ pushUntexturedBatches(:1246)/ pushVelocityBatches(:1969)/ materials pool renderDeferred(lldrawpoolmaterials.cpp:184・camera MASK4 含む)/ bump renderDeferred(lldrawpoolbump.cpp:513)。
### 直 walk・拡張予定 pass を運ぶ = 行使段で forEachSource 化(4 site・必須)
| site | 運ぶ拡張 pass | 消費文脈 |
|---|---|---|
| pushMaskBatches(lldrawpool.cpp:1639)| ALPHA_MASK / FB_ALPHA_MASK / MASK4 | camera amask/fbmask(lldrawpoolsimple.cpp:129/216)+ 影 MV/fallback(renderMaskedObjects pipeline.cpp:9347 経由・:13980/13999/14014-17/14278/14301/14319-22)|
| pushVelocityBatchesTextured(lldrawpool.cpp:2060)| ALPHA_MASK(lldrawpoolsimple.cpp:303)/ FB_ALPHA_MASK(:330)/ ALPHA(lldrawpoolalpha.cpp:1821)| velocity pass |
| renderAlphaObjects(pipeline.cpp:9242・walk :9258)| ALPHA | 影 fallback/spot(:14285)|
| renderAlphaObjectsMultiview(pipeline.cpp:14054・walk :14077/:14121)| ALPHA | 影 MV(:13985)|
### 直 walk・rigged/GLTF/POST_BUMP のみ = 縮退安全(義務なし・触るとき forEachSource へ)
pushRiggedBatches :1591 / pushUntexturedRiggedBatches :1621 / pushRiggedMaskBatches :1661 / pushRiggedVelocityBatches :2022 / pushRiggedVelocityBatchesTextured :2123 / pushRiggedBatchesIndirect :1442 / pushGLTFBatches :2208 / pushUntexturedGLTFBatches :2223 / pushRiggedGLTFBatches :2307 / pushUntexturedRiggedGLTFBatches :2326 / pushBumpBatches(lldrawpoolbump.cpp:939・POST_BUMP)/ renderRiggedObjectIDBufferForAvatar(pipeline.cpp:11737・全 RIGGED list)。
### ab(PASS_ALPHA)bucket 化の再判定 = **bucket 1 系で統一(機構 2 = walk-accumulate 不要)**
- 順序 bound = renderAlpha(+highlight)は mDrawMap 直読 = bucket 化の影響外。
- renderMap 消費者は上表 3 関数(velocity textured / fallback+spot / MV)のみ = forEachSource 化で完結。renderDebugAlpha は PASS_ALPHA を walk しない(lldrawpoolalpha.cpp:494-505 は MASK 系のみ)。
- rigged=true 側の PASS_ALPHA walk は構造的空(§1 不変条件)= forEachSource 化で意味変化なし。

## §5 力学(コスト・既知の縁)
- **dynamic writer churn**: particle(ALPHA)・grass(GRASS)は毎 frame 級 rebuild → registry 拡張後は patchGroup 実働 + 当該 region×pass bucket の mTplDirty 毎 frame 化。forEachVisible(非 MDI)は template 不要 = 影響なし。**template 再構築(rebuildTemplateIfDirty llvkbucket.cpp:328・caller は pushIndirectBucket lldrawpool.cpp:1277 のみ)は MDI emit する pass だけが払う** = 行使段で計器必須(matcen 同型)。
- template static 判定(llvkbucket.cpp:361-368)= region matrix・texmatrix なし・mAvatar null・VK slice 有。particle/grass 由来 record は概ね dyn 側へ落ち MDI 対象外(per-draw fallback)= 正しさは不変・配当だけの問題。
- untextured MDI 分岐(lldrawpool.cpp:1231)には isCameraMdiPass/heap gate が無い(影 opaque MV の MDI が依存する load-bearing 現状)。MASK 系 bucket 化で renderDebugAlpha(既定 OFF)が暗黙 MDI 化する件の処置は行使段 Brief で確定(容認方針・pushIndirectBucket は dyn も per-draw 併走 :1359-1373 = 母集団完全・出力等価)。

## §6 不変条件(INV-RS)
- INV-RS1: mDrawMap が唯一の正本。派生(renderMap/bucket)の writer は各 1 名(postSort / patchGroup)。
- INV-RS2: 順序 bound 描画消費者 = §2-1 名簿のみ。それ以外の描画消費者は forEachSource 経由。
- INV-RS3: pass の置き場切替 = kBucketizedPasses 単一 registry。載せる前に §3 チェックリスト充足。
- INV-RS4: 非 rigged pass の record は mAvatar==null(rigged は +1 pass へ)。
- INV-RS5: MDI emit 入口は §2-3 列挙の閉集合。
