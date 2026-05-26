# 06 — Offload Feasibility Deep (剥がし可否ブラッシュアップ調査)

**位置**: r40 章 Phase 1.2 ブラッシュアップ (05-core-assignment-plan.md の第 1 案を「本当に剥がせるか / Lock 競合の有無」で検算)

**前提**: 05 spec §3.1 で **剥がし採用 9 件** を仮置きしたが、spec のテキスト記述だけで採用判定したため、コード実態での再検算が必要。本 spec で 9 件全てを **R1 実装場所 / R2 触る global state / R3 他 caller 競合 / R4 剥がし最終判定** の 4 軸で揃え (BFS 原則、全候補同じ深さで揃える)、05 §3.1 の判定を上書き。

**作成**: 2026-05-26 / **§2.10 追記**: 2026-05-26 (Y-refined 採用候補 案 R-refined を R1-R4 形式で追加)

> **Y-refined 整合注記 (2026-05-26 Day 7 後)**: 本 spec §2.1-§2.9 は 9 候補時点の R1-R4 deep dive 記録。Day 7 統合判定 (02 §F) で章 scope が **3 候補 (案 O / R-refined / P-refined) に再収束** したため、05 spec §3.1 の最終採用は本 spec §4 ではなく **05 Y-refined 改訂版** が正。本 spec は (a) R1-R4 framework 自体の reference + (b) §2.10 renderShadow R1-R4 を含む全 9+1 候補の構造調査記録として残す。

---

## §0. 一行結論

05 §3.1 の 9 件のうち、**完全剥がし (a)** = 2 件、**部分剥がし (b)** = 5 件、**再分類 (c)** = 2 件 (A5-inv は既 offload、L6-occ は GL context thread affinity で full 不可)。**Phase 2 着手順序を更新**: A14 reference impl → L6-cull → A8 → A1 → A12 → A5-mat → A11 (A5-inv 削除、L6-occ は別 spec 化)。

---

## §1. 目的とスコープ

### §1.1 単一目的

05 第 1 案で仮置きした剥がし採用 9 件について、**コード実態で本当に剥がせるか + lock 競合の発生源を確定**。短縮系打ち手 (rate limit / lazy trigger / cvar 値弄り 等) は memory `feedback_r40_no_micro_tuning` に従い提案 / 採用ともに禁止。

### §1.2 調査軸 (R1-R4)

| 軸 | 内容 | 出力 |
|---|---|---|
| R1 実装場所 | 主要 file:line と関数本体の構造 | 確定 file:line リスト |
| R2 触る global state | snapshot 対象の global / static を R/W 分類 | state 一覧 + 触り方 |
| R3 他 caller の競合 | 他 thread / 他 frame phase / callback から state を触る経路、既存 mutex / atomic | 競合 caller リスト + lock の有無 |
| R4 剥がし最終判定 | (a) 剥がせる / (b) 部分剥がし可 / (c) 剥がせない (再分類要) + 1 行根拠 | 判定 |

調査は 9 件並列で agent dispatch (memory `feedback_use_agents_proactively`)。

### §1.3 本 spec が答えない

- 各候補の **実装コード**: Phase 2 で書く
- 既存 lock の **改修案**: 必要なら Phase 2 sub-spec で扱う
- 削減系 / 短縮系の評価: r40 章では一切扱わない

---

## §2. 候補別調査結果

### §2.1 A14 — `processTextureStats` (Worker C)

| 軸 | 内容 |
|---|---|
| **R1 実装場所** | `llviewertexture.cpp:1803-1892` `LLViewerFetchedTexture::processTextureStats()`、`llviewertexture.cpp:1766-1778` `setKnownDrawSize()`、`llviewertexture.cpp:1896-1901` `updateVirtualSize()`、`llviewertexturelist.cpp:1364-1370` `updateImages → processTextureStats × texture_count` (roll-up loop の caller) |
| **R2 触る state** | **W**: `mDesiredDiscardLevel` (1811-1886)、`mFullyLoaded` (1809-1815)、`mKnownDrawSizeChanged` (1875 reset)、`mMaxVirtualSize` (addTextureStats 経由)。**R**: `mKnownDrawWidth/Height` (1856-1873)、`mMinDesiredDiscardLevel`、`mFaceList[ch]` (reorganizeFaceList 経由 read-only、20 秒 cache 化済)、`gTextureList` (1364 列挙)。`mNeedsCreateTexture` のみ既 atomic、それ以外は protection なし (main thread 単一前提) |
| **R3 競合** | (a) `setKnownDrawSize` の他 caller は **UI / preview 層のみ** (llpreviewtexture.cpp、llthumbnailctrl.cpp、llvovolume.cpp、llworldmapview.cpp)、drawable face list から自動で呼ぶ箇所なし。(b) texture fetch thread は `getDesiredDiscardLevel()` を read (`updateFetch` 2127)、processTextureStats との関係は **W→R 一方向** で mutual exclusion 不要、read-after-write ordering で十分。(c) 既存 mutex/atomic: なし (mNeedsCreateTexture のみ) |
| **R4 判定** | **(a) 剥がせる** ✓ — stateless compute (in: mKnownDrawWidth/Height + mFullWidth/Height + mBoostLevel、out: mDesiredDiscardLevel + mFullyLoaded)、face list は 20 秒 cache、fetch thread とは W→R 一方向。double-buffer pattern 成立可 |

**Phase 2 reference impl として残る検証**: (i) worker enqueue overhead (100-200 us/call 典型) vs processTextureStats per-call cost の net 比、(ii) texture count 低 LOD scene vs high detail での閾値判定、(iii) main→worker→main fence latency 実測。**着手 OK**。

---

### §2.2 L6-cull — `updateCull_regionPartition` (Worker A)

| 軸 | 内容 |
|---|---|
| **R1 実装場所** | `pipeline.cpp:3021` `updateCull()` 本体、`pipeline.cpp:3059-3091` zone `updateCull_regionPartition` (`LLWorld::getInstance()->getRegionList()` × `LLViewerRegion::NUM_PARTITIONS=12` × `part->cull(camera)`)、`llspatialpartition.cpp:1447` `LLSpatialPartition::cull`、`LLOctreeCull::traverse()` (`llspatialpartition.cpp:1050-1110` 派生)、`pipeline.cpp:3116` `gPipeline.markNotCulled()` → `LLSpatialGroup::setVisible()`、shadow path: `pipeline.cpp:12527` `renderShadow()` 内 `updateCull(shadow_cam, result)` |
| **R2 触る state** | **R**: `mOctree` (root node、traversal 中 read-only)、camera frustum planes。**W**: `LLSpatialGroup::mState` (per-group、plain U32 `&=` / `|=`、atomic guard **なし**)、`sCull->pushVisibleGroup()` (結果配列 append)、`mNumVisibleNodes` counter、`LLPipeline::sShadowRender` (static bool, frame-scoped)、`LLViewerCamera::sCurCameraID` (static enum, context hint) |
| **R3 競合** | (a) **octree topology**: `LLSpatialPartition::insert/remove()` (`llspatialpartition.cpp:955, 970`) は main thread idle phase のみ、cull 中は不変。(b) **mState R/W**: `group->setVisible()` は frame epoch 内 main 単一、`setOcclusionState()` (`pipeline.cpp:3160`) も同 mState を `&=` write。(c) **shadow frustum**: sun_0..3 split (loop j=0..3) は **独立 frustum** (line 13242 `shadow_cam = camera;` + 13254-13264 per-split corner rebuild)、前 split 依存なし、`getVisiblePointCloud()` per-split (13274)。(d) **HUD / cube probe**: `gCubeSnapshot=true` 区間 (1050) の `initReflectionMaps()` も独立 frustum query。 |
| **R4 判定** | **(a) 剥がせる** ✓ — 10 fire (sun 4 + cube probe 2 + HUD 1 + 主 1 + 余裕 2) **全て独立 frustum**、データ依存なし、Worker A pool で 10 並列 task 投入可。Octree topology は cull phase 中 immutable で integrity 保証 |

**Phase 2 着手前に潰す**: (i) `group->setVisible()` の mState write race (複数 split task が同一 group を同時 setVisible)、(ii) `sCull->pushVisibleGroup()` の append 競合 (per-task buffer に分けるか、lockfree queue か)、(iii) shadow split task 終了同期コスト (flush / barrier)、(iv) octree node access の キャッシュ親和性。

**結論**: **着手 OK**、ただし mState atomic 化 と pushVisibleGroup の per-task buffer 化が前提条件。

---

### §2.3 A1 — `LLViewerObject::idleUpdate` (Worker A)

| 軸 | 内容 |
|---|---|
| **R1 実装場所** | `llviewerobjectlist.cpp:991-1068` `LLViewerObjectList::update()` (snapshot + loop launcher)、`llviewerobject.cpp:2538-2564` `LLViewerObject::idleUpdate()`、`llviewerobject.cpp:2568-2769` `interpolateLinearMotion()` (核、pos/vel 補間)、`applyAngularVelocity` + `interpolateLinearMotion` + `updateDrawable` の 3 段 |
| **R2 触る state** | **R**: `mRegionp->getTimeDilation()`、`mRegionp->getHost()`、`gMessageSystem->mCircuitInfo` (2617)、`LLWorld::getInstance()` (2669/2675/2694)、`LLViewerObject::sVelocityInterpolate` (static bool)。**W**: drawable pos/vel/accel (`setPositionRegion`, `setVelocity`, `setAcceleration`、2596-2761)、`mLastInterpUpdateSecs`、`mLastMessageUpdateSecs`、`mRegionCrossExpire`、`setChanged(MOVED)` (2600, 2764)。agent parameter は形式的で未使用 |
| **R3 競合** | (a) **region update path** (`processUpdateCore`、`llviewerobjectlist.cpp:1243-1266`) が message pump thread から `mRegionp` 再 assignment → race window あり。(b) **mActiveObjects list** に既存 mutex なし、snapshot は frame 頭で lockfree copy (998-1023)。(c) **gMessageSystem** は read-only access (circuit 状態 check のみ、write なし)。(d) `gPipeline.markMoved` は main thread only |
| **R4 判定** | **(b) 部分剥がし可** — compute (pos/vel 補間 math + region data read) は worker、apply (drawable state bit reflect + `markMoved` dispatch + null check) は main 残置。05 §5.1 想定通り |

**Phase 2 着手前に潰す**: (i) **region crossing race** — `processUpdateCore` が message pump thread で `mRegionp` を再 assign する一方、worker が `mRegionp->getHost()` 呼出 (2617) して null reference する可能性。snapshot timing 設計と null check 必須、(ii) drawable list snapshot のサイズ (21k object scene で frame 頭の copy cost)、(iii) `setChanged(MOVED)` の atomic 化 or deferred apply。

**結論**: 着手可、ただし region crossing race の解決が前提。**A14 / L6-cull 後** (worker A pool が動作確認済になってから)。

---

### §2.4 A5-mat — Material callback defer (Worker B)

| 軸 | 内容 |
|---|---|
| **R1 実装場所** | `llmaterialmgr.cpp:418-456` `onGetResponse()` (zip unpack + LLSD parse)、`llmaterialmgr.cpp:390-416` `setMaterialCallbacks()`、`llvovolume.cpp:2648-2662` `setTEMaterialParamsCallbackTE()`、`llvovolume.cpp:2688-2704` `setTEMaterialParams()` (`gPipeline.markTextured/markRebuild` 呼出) |
| **R2 触る state** | **R**: `LLWorld::instance()`、`gObjectList.findObject()` (2650)、`LLMaterialMgr::sInstance` 内部 maps (mMaterials, mGetCallbacks, mGetTECallbacks)。**W**: `gPipeline.markTextured()` (2699)、`gPipeline.markRebuild()` (2700)、`LLViewerObject::mFace` (`setTEMaterialParams` 経由)。4 つの critical global mutate point |
| **R3 競合** | (a) `processGetAllQueueCoro` (823) で `HttpCoroutineAdapter::getAndSuspend()` 既使用、ただし parse は L871 後ろで main callback (`onGetAllResponse` 459) が unzip+loop parse (475-505) を **main thread で実行**。(b) callback 順序: material_id を key とした signal dispatch、**per-material は FIFO だが material 間は順不同**。(c) HTTP response parse は `LLHttpSDHandler::onSuccess()` (98) で済み LLSD を main context で受取。(d) 既存 mutex/atomic なし (単一 thread assume) |
| **R4 判定** | **(b) 部分剥がし可** — parse (unzip + LLSD decode) は worker、callback invoke + apply (gPipeline mutate, LLDrawPool/LLFace) は main 残置 |

**Phase 2 着手前に潰す**: (i) `onGetResponse` / `onGetAllResponse` を異なる task context で呼出した時の **boost::signals2 thread-safety**、(ii) parse 中の LLSD temporary lifetime と callback invoke timing 同期コスト、(iii) gPipeline/gObjectList の per-frame access pattern が worker parse delay でリソース飢餓を起こさないか。

**結論**: 着手可、Phase 2 後半 (worker B pool の reference impl 後)。

---

### §2.5 A5-inv — Inventory parse offload (Worker B → **再分類**)

| 軸 | 内容 |
|---|---|
| **R1 実装場所** | `llinventorymodel.cpp:5613` `FetchItemHttpHandler::onCompleted` → `processData:5677` → `gInventory.updateItem:5737` + `notifyObservers:5739`。`llinventorymodelbackgroundfetch.cpp:1480` `BGFolderHttpHandler::onCompleted` → `processData:1542` → `updateItem:1599/1644` + `notifyObservers:1603/1682`。`responseToLLSD` 呼出: `llinventorymodel.cpp:5637` と `llinventorymodelbackgroundfetch.cpp:1507`。`notifyObservers` 実装: `llinventorymodel.cpp:2369` |
| **R2 触る state** | **R**: `gInventory.getItem()` (1609)、`findCategoryUUIDForType` (1690)、`getCategory` (1610)。**W**: `updateItem` (5737, 1599, 1644)、`notifyObservers` (5739, 1603, 1682)、`mCategoryMap/mItemMap` 内部 mutate (updateItem 内 old_item → `copyViewerItem` 1673、`addItem` 1686、parent-child tree 再構築 1648-1662)、observer list `mObservers` は `std::set` (thread-unsafe) |
| **R3 競合** | (a) **`responseToLLSD` は既に HTTP coroutine 内で動作**: worker thread は CURL 処理のみ、`onCompleted` は **gIdleCallbacks 経由で main thread に queued**。(b) `updateItem/notifyObservers` は両 handler の onCompleted から、**main thread 上で実行**、他 caller との競合なし。(c) `mObservers` (std::set) は mutex/atomic なし、`notifyObservers` 内 `mIsNotifyObservers` フラグで再入禁止のみ → main thread 単一前提。(d) LLInventoryModel に mutex/lock なし |
| **R4 判定** | **(c) 再分類要** — `responseToLLSD` parse は **既に HTTP coroutine 内で実行済** (worker CURL → main queued)。**新規 offload 不要**。`updateItem + notifyObservers` も main 単一設計、worker 移行には observer list (std::set → concurrent_set) + `mIsNotifyObservers` (atomic 化 + mutex 追加) が必須 → ROI マイナス |

**結論**: **A5-inv は 05 §3.1 から削除、§3.3 既 offload に移送**。Phase 2 で新規実装しない。確認用に Group N zone を 1 本切って既 offload を実測する余地はあるが、本 spec ではゲート対象外。

---

### §2.6 A8 — local light frustum cull (Worker A)

| 軸 | 内容 |
|---|---|
| **R1 実装場所** | `pipeline.cpp:7114-7340` `LLPipeline::calcNearbyLights(LLCamera& camera)`、mNearbyLights R/W (7145-7230, 7291-7330)、NEARBY_LIGHT bit set (7298) / clear (7155/7162/7167/7179/7185/7193)、`calc_light_dist()` (7090-7112、light radius を考慮した distance-to-sphere) |
| **R2 触る state** | **R**: gPipeline、camera.getOrigin、`mLights` (read-only iter 7234)、`mNearbyLights` (7145)、`RenderFarClip/RenderLocalLightCount` (config)。**W**: `mNearbyLights.insert/erase` (7230/7297)、`drawable.setState(NEARBY_LIGHT)` / `clearState()` (7298/7155-7193)、`drawable.setVisible()` (7337) |
| **R3 競合** | (a) **`mLights` mutation**: `setLight()` (7906-7921) が `llvovolume.cpp:1208` (drawable init) と `deleteShape` で呼ばれ、**frame 中に light spawn/despawn 発生**。`mNearbyLights` も `removeMutedAVsLights()` (2424-2444) が mute event 時に mutate。(b) **light property 変動**: `getLightIntensity/Radius/getRenderPosition` は `drawable.updateGeometry()` 後に dirty → calc_light_dist の input が frame 中 stale 可能性。(c) **drawable bit conflict**: NEARBY_LIGHT は calc 内でのみ set/clear、ただし drawable state 自体に mutex なし、atomic ビット演算なし。(d) 既存 mutex/atomic なし |
| **R4 判定** | **(b) 部分剥がし可** — calc_light_dist (純数学) と candidate list 生成は worker 可、ただし (i) drawable bit mutate を worker 直接不可 (atomic 化必要)、(ii) `mLights` iteration の snapshot lock 必要 (`setLight()` mutate と race)、(iii) light property の frame coherency 明示化 (1-frame cache or snapshot) の 3 点が前提 |

**Phase 2 着手前に潰す**: (i) mLights snapshot 方式 (frame 頭で fixed-size copy か、RWLock か)、(ii) drawable NEARBY_LIGHT bit を atomic 化 vs deferred apply のトレードオフ、(iii) light property snapshot の 1-frame lag が見た目に影響しないか。

**結論**: 着手可、Phase 2 中盤 (A14 / L6-cull の Worker A pattern が固まってから)。

---

### §2.7 A11 — region visibility (Worker A)

| 軸 | 内容 |
|---|---|
| **R1 実装場所** | `llworld.cpp:1062-1119` `LLWorld::updateVisibilities()` (region frustum culling)、`llworld.cpp:1105` `LLSurface::updatePatchVisibilities(gAgent)` (terrain patch frustum check)、`llviewerregion.cpp:1506-1604` `LLViewerRegion::updateVisibleEntries()` (VObject cache の visible group/entry selection) |
| **R2 触る state** | **R**: LLViewerCamera (getOrigin、getFar、AABBInFrustum)、gAgent (getPositionAgent、getPosGlobalFromAgent、getRegion)、LLViewerOctreeEntryData。**W**: `mVisibleRegionList` (erase/push_back)、`mCulledRegionList` (erase/push_back)、`LLSurfacePatch::mVisInfo.mbIsVisible`、`LLViewerRegion::mImpl->mVisibleEntries/mVisibleGroups` (insert/erase) |
| **R3 競合** | (a) **region list race**: `addRegion/removeRegion` (`process_enable_simulator/process_disable_simulator` via message pump) が **mRegionList、mActiveRegionList、mCulledRegionList を network thread から mutate**。updateVisibilities() は同じ list を iterator 走査・修正中。**⚠ mutex なし**。(b) **network thread**: `gVLManager.unpackData()` は terrain patch 高度を network thread で更新、ただし `mVisInfo.mbIsVisible` は render side のみ。(c) `gAgent` position は呼び出し時点値。(d) llworld.cpp / llviewerregion.cpp 共に同期 primitive **なし**、`mVOCachePartition` 内部 lock は未確認 |
| **R4 判定** | **(b) 部分剥がし可** — region frustum culling (updateVisibilities 上半) は camera AABB check のみで state mutation 最小化可、terrain patch visibility は gAgent snapshot で可。ただし **mVisibleRegionList/mCulledRegionList の network thread mutate race が本質的課題**、RWLock 追加が前提 |

**Phase 2 着手前に潰す**: (i) region list の RWLock 追加 (network thread / render thread / worker thread 三者の reader/writer 関係)、(ii) gAgent position/camera frustum の frame-start snapshot 設計、(iii) `mVisInfo.mbIsVisible` 更新 timing (write-back delay) が render side で許容可能か。

**結論**: 着手可だが **RWLock 追加が他候補より大きいリファクタ**、Phase 2 後半。

---

### §2.8 A12 — particle sim (Worker C)

| 軸 | 内容 |
|---|---|
| **R1 実装場所** | `llworld.cpp:1209` `LLViewerPartSim::updateSimulation()` → `llviewerpartsim.cpp:276` `LLViewerPartGroup::updateParticles()` ループ、`llviewerpartsource.cpp:130` `LLViewerPartSourceScript::update()` (spawn)。per-particle state: `LLViewerPart::mPosAgent`, mVelocity, mLastUpdateTime, mColor |
| **R2 触る state** | **R**: `LLViewerPart::mParticles[]`、gravity 定数、camera position (距離判定)、wind velocity (`regionp->mWind`)。**W**: (1) per-particle (mPosAgent +=, mVelocity +=, mLastUpdateTime, mColor) → 通常 local access、(2) `sParticleCount` (`atomic<S32>` 使用済)、(3) `gPipeline.markRebuild()` → **main only**、(4) `mParticles` vector erase (`vector_replace_with_last()`) → スレッド非安全 |
| **R3 競合** | (a) **spawn/expire**: main が `LLViewerPartSourceScript::update()` で new particle (304, 436)、`addPart()` で `mParticles` 追加。worker 計算中に main が `erase` / `push_back` すると **iterator invalidation / ABA 問題**。(b) `gPipeline.markRebuild()` は main thread only (`pipeline.h:213-214`)、worker からは deferred event queue 必須。(c) double-buffer 機構なし → 整合性に per-frame snapshot 必須。(d) `sParticleCount` のみ atomic、mParticles list 無保護 |
| **R4 判定** | **(b) 部分剥がし可** — per-particle update (pos/vel/age/color) は worker、spawn/expire (vector structural change) + markRebuild は main。double-buffer 必須 |

**Phase 2 着手前に潰す**: (i) mParticles double-buffer 設計 (worker が read snapshot、main が write 用別 buffer)、(ii) markRebuild を deferred event で main にエンキューする仕組み、(iii) atomic sParticleCount の整合再検証、(iv) `vector_replace_with_last()` の race 排除。

**結論**: 着手可、Phase 2 中盤 (worker C pattern が A14 で確立後)。

---

### §2.9 L6-occ — reflection probes occlusion polling (**部分剥がしのみ可**)

| 軸 | 内容 |
|---|---|
| **R1 実装場所** | `llreflectionmapmanager.cpp:1616-1628` `Manager::doOcclusion`、`llreflectionmap.cpp:345-411` `Probe::doOcclusion`、`glGetQueryObjectuiv` 呼出 L381, L386。query id list: `LLReflectionMap::mOcclusionQuery` (per-probe GLuint)。mOccluded state update L387。call site: `pipeline.cpp:3196-3197` `LLPipeline::doOcclusion` |
| **R2 触る state** | **R**: `std::vector<LLPointer<LLReflectionMap>> mProbes` (217) iterate (1621-1627)、各 probe の `mOccluded`/`mOcclusionPendingFrames`、`LLViewerCamera::instance()` eye。**W**: probe::mOccluded (387)、probe::mOcclusionPendingFrames (388, 392)、`glGetQueryObjectuiv` / `glBeginQuery` / `glEndQuery` は **main thread GL context のみ**。mutex/atomic 保護なし |
| **R3 競合** | (a) mProbes list: update() と doOcclusion() は同一 frame 内 read-only、deleteProbe/addProbe は update() が main 内 serialized。(b) **GL context affinity**: `wglCreateAssociatedContextAMD` 定義あるが **使用なし**。viewer 全体で worker thread から GL 呼出は texture upload 等の明示的 context hold が必須な部分のみ。**`glGetQueryObjectuiv` は main thread binding 依存**。(c) query id は per-probe ローカル、reader 競合なし。(d) 既存 mutex/atomic なし |
| **R4 判定** | **(c) full 剥がしは不可** — `glGetQueryObjectuiv` は GL command で **driver 実装レベル thread affinity 制約**。NVIDIA (permissive、context sharing) でも AMD・Intel は shared context worker 読み取りで **未定義動作リスク**。query 発行 (`glBeginQuery`) も main 専有必須。**CPU-side rollup (mProbes iterate + pending frame count + timeout) のみ worker 可** |

**Phase 2 着手前に潰す**: (i) shared GL context (AMD `WGL_CREATE_ASSOCIATED_CONTEXT`) 実装可能性、(ii) worker thread context 構成、(iii) 各 driver (NVIDIA/AMD/Intel) での smoke test 実施有無、(iv) 既存 worktree でも CPU-side rollup の純益が dispatch overhead を超えるか。

**結論**: **L6-occ の本来の期待効果 (3 ms 級削減) は GL 制約で達成困難**。CPU-side rollup のみなら数十 us 級。05 §3.1 採用から **§3.4 main 残置必須に移送 (CPU 側は元々小さい)**、または **別 spec で shared GL context 化の独立調査** を立てる。Phase 2 着手対象から除外。

---

### §2.10 renderShadow Layer 7 — pre-cull worker (案 R / R-refined、Y-refined 採用)

**位置付け**: 19 周目 Day 4-5 POC (02 §E.1) で構造調査済、本 §2.10 で R1-R4 軸に揃えて再記録。**Y-refined 採用 (05 §3.1 案 R-refined)**。

| 軸 | 内容 |
|---|---|
| **R1 実装場所** | `pipeline.cpp:12497-12722` `LLPipeline::renderShadow()` 本体 (4 cascade × shadow split loop)。pre-cull 部分: `pipeline.cpp:12536` `updateCull(shadow_cam, &result)` + `pipeline.cpp:12541` `stateSort(shadow_cam, &result)`。GL dispatch 部分: 12547-12714 (matrix push/pop、shader bind、`renderGeomShadow`、`renderAlphaObjects`、`renderMaskedObjects`、FBO bind/blit) |
| **R2 触る state** | **R (worker 移送可)**: shadow camera (`LLCamera`、4 cascade)、frustum 6 plane × 4、agent pos、shadow split params、octree root pointer。**W (worker 移送可)**: `LLCullResult` 11 buffer × 4 cascade (mVisibleGroups / mAlphaGroups / mRiggedAlphaGroups / mOcclusionGroups / mDrawableGroups / mVisibleList / mVisibleBridge / mRenderMap[8])。**R/W (main 残置必須)**: `LLPipeline::sShadowRender` (static bool、12503/12719 書込)、`LLPipeline::sUseOcclusion` (static、12507/12718 書込) — 12 file 以上で参照。**R/W (GL main 残置必須)**: gGL matrix stack、shader binding、FBO state、`LLPipeline::sCull` global (pipeline.cpp 72 site 参照、書込 3 箇所: grabReferences 2747 / clearReferences 2753 / clear 3063)、`LLDrawable::mNumVisibleFaces` (frame-level counter、書込 3 site: 528 init / 2529 preCull reset / 4262 stateSort 累積) |
| **R3 競合** | (a) **octree topology**: cull phase 中 immutable (insert/remove は idle phase のみ、L6-cull §2.2 と同様)、shadow cull と main cull の octree 共有 reader 並列可能。(b) **sShadowRender / sUseOcclusion**: 静的 flag、renderShadow 内で 12503/12507 set → 12718/12719 reset の frame-scoped、12 file 以上の reader 経路があり ShadowRenderContext struct への集約 (parameter 化) が必要。(c) **LLCullResult**: result append-only `std::vector<T*>`、cascade 毎独立 buffer 配置で append 競合無し。(d) **`markVisible` callback**: pipeline.cpp:3768 単一実装、leaf class への散在無し。(e) **既存 mutex/atomic**: なし、shadow cull の writer は main thread 単一前提 |
| **R4 判定** | **(b) 部分剥がし可 (Y-refined では (a) 相当の GREEN)** — **pre-cull (updateCull + stateSort) を worker B に移送、GL dispatch body を main 残し** で構造的に成立。renderShadow body 内 ~100 行 GL 呼出 (12547-12714) は **GL context tie で worker 不可 (構造的 blocker、02 §E.1 RED)** だが、pre-cull だけ frame N-1 で先行構築 → frame N の renderShadow body が dispatch のみに専念する scope に再定義可能 |

**Phase 2 着手前に潰す (Y-refined §5.2 / §9.2 と同期)**:
- (i) `sShadowRender` / `sUseOcclusion` 12 file 以上の正確な site list を `git grep -n 'sShadowRender\|sUseOcclusion' indra/` で確定、ShadowRenderContext struct 化の波及範囲を見積
- (ii) frame N-1 先行構築の visual diff (shadow 1 frame stale 受容可否) を A/B 撮影
- (iii) RW lock の per-frame 取得頻度実測 (cull 中の writer は main のみだが、頻度確認)
- (iv) `LLCullResult` 4 cascade × 11 sub-buffer の memory footprint (worker / main で per-frame 4 セット保持の cost)

**結論**: **着手 OK (Y-refined 第 2 候補)**。期待短縮 ceiling = renderShadow body 内 cull + stateSort 部分の ~25% = **2-3 ms/frame** (02 §F.1)。Worker B 専属 pool (frame 跨ぎ運用)、ShadowRenderContext struct 化が前提。詳細は 05 spec §5.2 / §4.3。

> **参照**: 02 §E.1 (構造調査 raw)、02 §F.1 (ceiling 確定表)、05 §5.2 (剥がし境界 detail)、05 §4.3 (Worker B 詳細)。

---

## §3. 横断発見 (共通 lock domain と設計示唆)

### §3.1 main thread only mutate と判明した state (worker 直接書込不可)

| state | mutate 元 | 影響候補 | 設計対応 |
|---|---|---|---|
| `gPipeline.markRebuild()` | pipeline.h:213-214 | A1, A12 | deferred apply event queue |
| `gPipeline.markTextured/markRebuild` | A5-mat 同上 | A5-mat | callback queue 経由 |
| `gPipeline.markMoved` | A1 | A1 | 同上 |
| `LLSpatialGroup::mState` (plain U32 `&=/|=`) | L6-cull, A8, 他 cull 系 | L6-cull, A8 | **atomic U32 化が前提条件** |
| `LLViewerObject::setChanged(MOVED)` | A1 | A1 | atomic flag or deferred reflect |
| `drawable.setState(NEARBY_LIGHT)` | A8 | A8 | atomic bit op or deferred reflect |
| `mParticles` vector structural change | A12 | A12 | double-buffer + main only resize |
| `gPipeline.doDeferred*` GL calls | L6-occ | L6-occ | main 残置 (worker 不可) |

### §3.2 network thread から mutate される state (race 確定済)

| state | network thread caller | 影響候補 | 設計対応 |
|---|---|---|---|
| `LLViewerObject::mRegionp` | `processUpdateCore` (`llviewerobjectlist.cpp:1243-1266`) | A1 | snapshot + null guard、region crossing window 明示 |
| `LLWorld::mRegionList`, `mActiveRegionList`, `mCulledRegionList` | `process_enable_simulator` / `process_disable_simulator` | A11 | **RWLock 追加** |
| terrain patch 高度 | `gVLManager.unpackData()` | A11 | render side mVisInfo は別、影響最小 |

### §3.3 既 offload で新規実装不要と判明した state

- **A5-inv `responseToLLSD`**: 既に HTTP coroutine 内 parse、`onCompleted` は gIdleCallbacks 経由 main queued
- **A5-tex** (texture parse) / **A5-coro** (HTTP body parse) / **A6** (Stream3D URL resolve): 02 spec §C 既記録

### §3.4 GL context 制約

- **L6-occ**: `glGetQueryObjectuiv` 含む GL command は worker thread から呼べない (driver 実装レベル制約)。viewer 内で worker GL 呼出例は texture upload など限定的、shared context (AMD `wglCreateAssociatedContextAMD`) は **未使用**
- → L6-occ は CPU 側 rollup のみ worker 化可、GPU query 部分は main 残置必須
- 他の候補 (A1, A5, A8, A11, A12, A14, L6-cull) は GL 呼出を含まないので影響なし

---

## §4. 05 spec §3 への反映 (上書き)

### §4.1 05 §3.1 採用候補の更新

| ID | 05 §3.1 判定 | 06 判定 (本 spec) | 移送先 |
|---|---|---|---|
| A1 | 剥がし採用 | (b) 部分剥がし可 | 採用維持 (但し region crossing race 解決前提) |
| A5-mat | 剥がし採用 | (b) 部分剥がし可 | 採用維持 (但し signals2 thread-safety 確認前提) |
| **A5-inv** | 剥がし採用 | **(c) 既 offload 済** | **§3.3 既 offload に移送、Phase 2 対象から削除** |
| A8 | 剥がし採用 | (b) 部分剥がし可 | 採用維持 (但し mLights snapshot + drawable bit atomic 化前提) |
| A11 | 剥がし採用 | (b) 部分剥がし可 | 採用維持 (但し region list RWLock 追加が重いリファクタ) |
| A12 | 剥がし採用 | (b) 部分剥がし可 | 採用維持 (但し double-buffer 設計前提) |
| A14 | 剥がし採用 | **(a) 剥がせる** ✓ | **採用維持、reference impl 第一候補** |
| L6-cull | 剥がし採用 | **(a) 剥がせる** ✓ | **採用維持** (但し mState atomic 化 + pushVisibleGroup per-task buffer 化前提) |
| **L6-occ** | 剥がし採用 | **(c) full 不可 (GL 制約)** | **§3.4 main 残置必須に移送、CPU 側 rollup のみ別 spec で扱う** |

### §4.2 Phase 2 着手対象の最終確定 (9 → 7 件)

採用維持 7 件 + 削除 2 件:

**採用維持 (Phase 2 着手対象)**:
1. A14 (a 剥がせる、reference impl)
2. L6-cull (a 剥がせる、Worker A reference)
3. A1 (b 部分、region race 解決前提)
4. A5-mat (b 部分、signals2 確認前提)
5. A8 (b 部分、atomic 化前提)
6. A11 (b 部分、RWLock 追加で重い)
7. A12 (b 部分、double-buffer 前提)

**削除 (Phase 2 対象外)**:
- A5-inv (既 offload、新規実装不要)
- L6-occ (GL 制約、別 spec で shared context 化を独立調査)

---

## §5. Phase 2 着手順序の更新

05 §6.2 の順序を 06 知見で更新:

| 順 | 候補 | 06 判定 | 更新後の根拠 |
|---|---|---|---|
| 1 | **A14** | (a) | reference impl、double-buffer 成立確定、stateless compute |
| 2 | **L6-cull** | (a) | shadow 10 fire 独立確定、ただし mState atomic 化と pushVisibleGroup per-task buffer 化を先行実装 |
| 3 | **A8** | (b) | Worker A 2 件目、mLights snapshot + drawable bit atomic 化の pattern を確立 |
| 4 | **A1** | (b) | Worker A 3 件目、region crossing race 解決を A8 の snapshot pattern で流用 |
| 5 | **A12** | (b) | Worker C 2 件目、double-buffer pattern を A14 から流用 |
| 6 | **A5-mat** | (b) | Worker B 単独 (A5-inv 削除のため B pool は 1 件のみ)、signals2 thread-safety 確認後 |
| 7 | **A11** | (b) | 最後、RWLock 追加が重く独立リファクタ扱い |

**削除済**: ~~A5-inv~~ (既 offload)、~~L6-occ~~ (GL 制約、別 spec)

---

## §6. 残 unknown と Phase 2 着手前必要検証

### §6.1 横断 (全候補共通)

- **U1 dispatch overhead**: A14 reference impl で `enqueue + wait` の overhead を実測 (期待 < 0.05 ms/call、超えたら 3 pool 構成自体を見直し)
- **U2 thread 数 scaling**: 物理 core 8/16/32 でどう scale するか A14 で確認
- **U3 atomic 化のコスト**: mState (U32) の `&=/|=` を atomic に変えた時の per-frame 影響、L6-cull / A8 着手前に L6-cull で先行測定

### §6.2 候補別

- **L6-cull**: `LLSpatialGroup::mState` atomic 化、`sCull->pushVisibleGroup()` per-task buffer 化、shadow split flush 同期コスト
- **A1**: `processUpdateCore` の `mRegionp` 再 assign window と worker snapshot timing
- **A8**: `mLights` snapshot 方式 (RWLock vs frame-start copy)、light property frame coherency
- **A11**: region list RWLock の network thread / render thread / worker thread 三者調停
- **A12**: `vector_replace_with_last()` の double-buffer 化、markRebuild deferred queue
- **A5-mat**: boost::signals2 の thread-safety、callback 順序保証の現状仕様

### §6.3 別 spec 化候補

- **L6-occ shared GL context 化**: AMD `wglCreateAssociatedContextAMD` / NVIDIA shared context / Intel の制約を 3 OS × 3 vendor smoke test で確定する独立調査。**r40 章ではなく r41+ 候補**

---

## §7. Phase 2 着手 gate (05 §8 を上書き)

### §7.1 着手 OK 条件

1. 本 06 spec の AYA review PASS
2. A14 reference impl 完成 → dispatch overhead 実測 (`< 0.05 ms` ゲート)
3. A14 で 0.3 ms 以上の純削減 PASS

### §7.2 着手 NG 条件

- A14 で dispatch overhead が 0.5 ms 超 → worker 3 pool 構成自体見直し
- L6-cull で mState atomic 化のコストが 0.5 ms 超 → atomic 化なし設計を再検討
- A11 の RWLock 追加が他箇所 (network thread / render path) で degrade を引き起こす → A11 を Phase 3 後送り

### §7.3 着手後の運用

- 各候補は **独立 PR** (memory `feedback_experiment_branch_single_scope`)
- per-PR は (baseline / 実装 / 再計測) の 3 commit
- 効果が期待値 50% 未満なら merge せず revert (memory `feedback_build_only_verified`)

---

## §8. 関連 spec / commit

| 参照先 | 用途 |
|---|---|
| `00-overview.md` | r40 章 thesis |
| `02-offload-feasibility.md` | §A1-A14 deep-dive (本 spec で再検算) |
| `03-perf-log-infra.md` | 計測 infra |
| `04-observed-hot-path-map.md` | Layer 1-6 結果 |
| `05-core-assignment-plan.md` | 第 1 案 (本 spec で §3 上書き、§6 着手順序更新) |
