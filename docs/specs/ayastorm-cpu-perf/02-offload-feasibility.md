# 02 — Offload feasibility (分散可能性の資料ベース検討)

`01-mainthread-workload-inventory.md` で棚卸しした main thread 占有処理について、 **どれを実際に main thread から剥がせるか / 剥がすにはどんな前提・改修が必要か** を実コード根拠で評価する。

> この資料は「実装の提案」ではなく **「資料ベースの分散可能性評価」**。
> ここで「可能」とした項目も、03 以降で個別実装する時にもう一度コード上で再検証する。
> 設計倒れ spec を積まないために、**確認できない部分は『未確認』と明示し、推論を結論に格上げしない**。

## 分類軸

| カテゴリ | 意味 |
|---|---|
| **I. 既に offload 済** | worker thread 等で既に処理されている。確認のため列挙する |
| **II. offload 可能性 高** | 小〜中規模改修で main 剥がし可能、データ依存 / order 制約が薄い |
| **III. offload 可能だが大手術** | 可能だが SoA 化、ロック設計、frame 境界の visibility 再設計が必要 |
| **IV. main 残置必須** | GL context / OS event / order 制約で構造的に剥がせない |
| **V. offload より削減 (cull)** | 移すより「やめる / lazy 化」のほうが筋。設計問題を含む |

---

## §A. Candidate top 6 deep-dive

### A1. gObjectList::idleUpdate を thread pool 化

**対象**: 01 §3 「object update」 (`llviewerobjectlist.cpp:1050`)

**現状の実装**

- `LLViewerObjectList::update()` (`llviewerobjectlist.cpp:921`) が main render thread で per-frame 実行。 `mActiveObjects` (約 21,000 個) を逐次 loop して `idleUpdate(agent, frame_time)` を呼ぶ (line 1045-1051)。
- `LLViewerObject::idleUpdate()` (`llviewerobject.cpp:2538-2564`) が
  - `applyAngularVelocity(dt)` (line 2549)
  - `interpolateLinearMotion(frame_time, dt)` (line 2558) — 200+ 行、velocity/acceleration 補間、region crossing 検出、height clamp
  - `updateDrawable(false)` (line 2562) — drawable state dirty mark
- subclass override:
  - `LLVOAvatar::idleUpdate` (`llvoavatar.cpp:3066-3268`) — `updateCharacter(agent)` (motion controller、animation interpolation)、voice visualizer、appearance animation、lip sync、wind、complexity update
  - `LLVOTree::idleUpdate` (`llvotree.cpp:356-420`) — LOD 計算、`gPipeline.markRebuild()`
  - `LLVOVolume` は override なし (base 実装使用)

**データ依存 / 同期前提**

idleUpdate が書き込む field:

- object position / velocity / acceleration (`mPositionRegion`, `mVelocity`, `mAcceleration` @ llviewerobject.cpp:2596, 2597, 2706, 2760, 2761)
- LLDrawable::mState (pipeline.cpp:3799-3818 で `ON_MOVE_LIST`, `MOVE_UNDAMPED` set)
- `setChanged(MOVED | SILHOUETTE)` (llviewerobject.cpp:2600, 2764)
- avatar: motion controller state、time tracking、visibility (llvoavatar.cpp:3210, 6028)
- tree: `mTrunkLOD`, `mLastPosition`, `mLastRotation` (llvotree.cpp:419, 405, 414)

並行アクセスリスクが多い:

- **gPipeline.mMovedList**: idleUpdate が `markMoved()` (line 3777) で append、display phase が renderGeomDeferred で read (read-write race)
- **gAgent**: parameter として渡るが avatar で `agent.fidget()` 等が main thread only
- **LLDrawable::mState**: idleUpdate (read/write) と display (read) が同 frame で交差、atomic でない
- **mLastInterpUpdateSecs / mLastMessageUpdateSecs** (llviewerobject.h:951-952) も同 object に対して非 atomic write

**offload 方式の現実性**

- 素直な SoA 化は subclass override の規模 (avatar 13k 行 / tree 1.2k 行) で virtual dispatch 削除困難
- 部位別 batch 化 (avatar 約 100-500 / volume 大多数 / tree 100-500) なら 1 階層上で分ける余地はある
- ただし gPipeline / gAgent / LLDrawable::mState の thread-safety を伴う改修が前提

**リスク**

- race: `mMovedList` append-only なら lock-free 可だが、`mState` bit set は atomic でない → display が mid-update を読むと `ON_MOVE_LIST` 不整合
- visibility: position update と drawable mark が分かれた場合 1 frame delay → glitchy movement
- region crossing 中の null pointer (mRegionp swap)
- avatar motion controller state の lock 競合

**工数感**: 中 (1-2 週)。touch する file 数 5-8 (llviewerobjectlist / pipeline / lldrawable / llvoavatar / llviewerobject + thread pool 基盤)、変更行数 200-400 LOC、integration test 含めて 3-5 日。

**判定**: **III (大手術)**。regression surface が広く、CoolVL / Alchemy 等の fork でも同等の試行報告が見当たらない。後回し優先度。

**未確認**

- `LLVOVolume::idleUpdate` override 有無 (grep で見当たらず、base 使用と推定だが確定不能)
- region crossing 中の挙動 (mRegionp change handling)
- viewer 既存 thread pool 基盤の容量
- avatar `updateCharacter` 内の critical path 分解 (実測必要)

---

### A2. octree balance() を lazy trigger 化

**対象**: 01 §3 「octree balance」 (`pipeline.cpp:2675, 2683`)

**現状の実装**

- `LLPipeline::updateMove()` (pipeline.cpp:2675, 2683) で **毎 frame 全 region × NUM_PARTITIONS + VO Cache tree** に対し無条件に `mOctree→balance()` 呼出
- `LLOctreeNode::balance()` (`indra/llmath/lloctree.h:723-756`) の中身は **O(1)** — 「唯一の子が空の枝なら子を root にして層削除」だけ。コスト自体は軽い
- **問題は呼出頻度** = O(partition_count × region_count) ≒ 数十〜百回 / frame
- dirty flag 系の追跡は未実装 (`mDirty` / `mNeedsRebuild` 等は存在するが balance 必要性は追跡されていない)

**lazy trigger 化の現実性**

- `LLSpatialPartition` に `mNeedRebalance` flag を追加
- hook 点 (実コードで特定):
  - `LLOctreeNode::insert()` (lloctree.h:314-425)
  - `LLOctreeNode::remove()` (lloctree.h:481-526)
  - `LLOctreeNode::removeByAddress()` (lloctree.h:528-540)
  - `LLDrawable::updateMove()` で spatial group 変更時
- `updateMove()` で flag が立っていなければ balance skip
- 既存 LL codebase に dirty flag pattern (`GEOM_DIRTY`, `MESH_DIRTY`) 多数あり、踏襲容易

**リスク**

- visibility glitch なし (`isInside()` は AABB ベースで tree depth に依存しない)
- empty node 蓄積で tree が深くなる → cull traversal が遅くなる方向
- spike: 累積 dirty を一気に balance するフレームでは spike 増 (許容可能)

**工数感**: **小 (1-2 日)**。touch file 3 個 (`lloctree.h` / `llspatialpartition.h+cpp` / `pipeline.cpp`)、変更行数 30 LOC 程度。

**判定**: **II (高)**。dirty flag pattern と整合、視覚回帰なし、効果見込み 0.1-0.5 ms / 静止 frame。

**未確認**

- `NUM_PARTITIONS` の実数 (region scope 内で `LLViewerRegion::NUM_PARTITIONS`)
- Firestorm fork で既存改修が無いか (grep で `#ifdef OPENSIM` / cvar gate を確認したが見当たらず)

---

### A3. Occlusion Query を async readback 化 + cull 効果ゼロ問題の原因究明

**対象**: 01 §2 「Occlusion Query Issue / Retrieval」 (`pipeline.cpp:3159`, `llviewerdisplay.cpp:898, 1014`)、01 §7-C 「Occlusion Queries Performed 3534 /s に対して Objects Occluded 3.169 /s」

**重大な訂正: 既に async design**

agent 調査で **occlusion query は既に 1 frame lag の async design** であることが判明。

実コード経路:

- query 発行: `pipeline.cpp:5115` `doOcclusion()` (deferred rendering 中)、`llvieweroctree.cpp:1172-1282` で cube VB render + `glBeginQuery/glEndQuery`
- query 結果回収: `glGetQueryObjectuiv(..., GL_QUERY_RESULT_AVAILABLE, ...)` で先に poll (llvieweroctree.cpp:1134)
- Frame N で issue、Frame N+1 で poll (lag track: `mOcclusionIssued[...] = gFrameCount` @ line 1231)
- 結果反映: `pipeline.cpp:4007-4010` で OCCLUDED なら `markOccluder()`、`llspatialpartition.cpp:1068-1071` で earlyFail traverse skip

**stall の正体**

`llvieweroctree.cpp:1140`:
```cpp
if (available || mOcclusionCheckCount[...] > occlusion_timeout)
    glGetQueryObjectuiv(..., GL_QUERY_RESULT, ...);  // blocking
```

- `RenderOcclusionTimeout = 4` frame (cvar)
- 4 frame 以内に available にならなければ **強制 blocking read** = main thread stall
- 3534 q/s で GPU query queue overflow → timeout で blocking する shape が成立

**Cull 効果ゼロの原因**

agent 分析 (実コードで仮説 (a)-(f) を検証):

- (a) `sUseOcclusion` cvar OFF: 否 (3534 q/s で issue されている = ON)
- (b) markOccluder hierarchy: pipeline.cpp:3142「top-most occluders only」、parent OCCLUDED なら子 query されない。**open sky で root が OCCLUDED になりにくい → 深い hierarchy へ query が下りる**
- (c) result threshold: `query_result > 0` (1 px 通れば not occluded、permissive)。**否 (3534 のうち 3 occluded は threshold ではなく result value が high)**
- (d) result lag で default 倒れ: **疑い**。timeout=4 で blocking → forced result 取得時に既に古い視点
- (e) open sky / occluder 不足: **疑い (環境依存)**
- (f) frustum / distance cull 先行: `updateCull` で frustum check が先、occlusion check は earlyFail 後段 → **occlusion で「新たに」cull される余地が少ない (cross-cancellation)**

つまり cull 効果ゼロは **環境依存 + cross-cancellation** であって設計バグではない。outdoor scene で構造的にこうなる。

**改善方向**

- **stall 側 (II/III)**: 
  - timeout=4 → 8 / 16 へ拡大 (cvar 値変更、1-2 時間、low risk)
  - timeout 到達時に **discard + 前 frame state 保持** (assume not occluded で render は冗長になるが stall 回避、2-3 時間、medium risk = false visible)
  - QBO (GL 4.4+ `ARB_query_buffer_object`) で完全 async readback (8-16 時間、high complexity)
- **cull 効果ゼロ側 (V)**: 環境依存なので code fix の余地少ない。 query 発行頻度を **下げる方向** (occluder candidate sizing 見直し / query interval 拡大) も選択肢

**判定**:

- stall: **II (高)**。timeout cvar の値だけでも実機検証 → 効果あり次第 discard 追加。 polling/QBO は 03 以降の本格対応。
- cull 効果ゼロ: **V (削減で対処) を準ずる**。query 発行頻度を絞る方向の調整。

**未確認**

- 今日の実測 (3534 / 3) 取得時の environment 詳細 (室内/室外、camera 位置、occluder 数)
- `RenderOcclusionTimeout` の AYAstorm config 上の現在値
- query poll zone profile での実際の stall 時間
- NVIDIA / AMD の query queue depth 差

---

### A4. NVIDIA shared GL upload の fence wait 改善

**対象**: 01 §5 「Worker→Main Sync (GPU fence)」 (`llimagegl.cpp:1745-1797`)

**現状の実装**

- worker (LLImageGLThread, shared GL context) が GPU に upload → `syncToMainThread()` で main に通知
- **NVIDIA 分岐** (llimagegl.cpp:1752-1760):
  ```cpp
  auto sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  glFlush();
  glClientWaitSync(sync, 0, GL_TIMEOUT_IGNORED);  // worker thread blocking
  glDeleteSync(sync);
  ```
- **AMD / Intel / Apple 分岐** (line 1761-1782): `glWaitSync()` を main thread の WorkQueue に post (worker は待たない)
- vendor 判定: `llgl.cpp:1157-1160` で `glGetString(GL_VENDOR)` を見て `gGLManager.mIsNVIDIA` を set
- **`GL_TIMEOUT_IGNORED`** = 無限待機。timeout なし、pure blocking

**NVIDIA を blocking にした経緯**

- commit `dd66d8a20c` (2022-04-15, SL-17219 "Texture pipeline overhaul") で NVIDIA だけ split
- コメント:「wait for texture upload to finish before notifying main thread」
- 推定理由: NVIDIA driver の shared context での `glWaitSync()` async notify が race condition を起こした (driver bug 回避)

**関連 cvar**

- `RenderGLMultiThreadedTextures` (default **0 / OFF**) — OFF にすると LLImageGLThread 自体作られず、GL upload は main thread へ
- `RenderGLMultiThreadedMedia` (default 0)
- GL version < 3.96 でも自動 OFF (`llimagegl.cpp:249-264`)

**改善方向**

1. **polling + short sleep** (推奨):
   ```cpp
   while (glClientWaitSync(sync, 0, 1) == GL_TIMEOUT_EXPIRED)
       std::this_thread::sleep_for(100us);
   ```
   - worker thread CPU を `glClientWaitSync` の busy-wait から解放
   - 実装 2-3 時間、3 OS × 2 vendor の regression test 1 週間
   - 同種 pattern が既に `llgl.cpp:2987` に存在
2. **async notify 統一** (高リスク):
   - NVIDIA も AMD path に統一 = `glWaitSync` を main queue に post
   - driver bug 再発の可能性、Linden Lab 側と coordination 必須

**判定**

- **II (高)**: polling 案で worker thread CPU 削減見込み (30 % → 5-15 % の可能性)
- ただし **main thread の bottleneck は別所** なので net FPS gain は限定 (±2-5 FPS の見積もり)
- 副次効果: worker thread の電力消費 / 熱が減る
- **テスト負荷大**: NVIDIA / AMD / Intel × Linux / macOS / Windows = 9 環境、AYAstorm の 3 OS 方針と整合

**Firestorm / AYAstorm の改修**: なし (texture pipeline 周りは LL upstream のまま)

**未確認**

- `glFlush()` 3 回呼出 (line 1757, 1766, 1767) の実 cost
- AMD async notify が main queue に届くまでの latency
- driver bug 回避が今でも必要か (NVIDIA driver の現行版で fix されている可能性)

---

### A5. HTTP body parse offload

**対象**: 01 §4 全行 (visitNotifier 内で main parse される LLSD / JSON / XML)

**caller 別の現状**

| caller | 現状 | offload 余地 |
|---|---|---|
| Texture Fetch (`lltexturefetch.cpp:2153`) | onCompleted は state 遷移 + buffer 保持のみ。parse は DECODE_IMAGE state で texture pool thread | **I (既に offload 済)** |
| Coroutine (`llcorehttputil.cpp:257-330`) | `mReplyPump.post()` で LLSD immutable copy を main に dispatch、worker で parseBody / buildStatusEntry | **I (既に offload 済)** |
| XML-RPC (`llxmlrpctransaction.cpp:127-184`) | onCompleted は body→read のメモリコピーのみ。parse は明示的に main の `process()` で後追い実行 (コメント「We do not do the parsing in the HTTP coroutine」) | **I (意図的 main 側で実施、現状で完結)** |
| Material Manager (`llmaterialmgr.cpp:67-115`) | LLHttpSDHandler::onCompleted で worker 側 LLSD parse、onSuccess callback は main で execution | **II**: callback defer で更に改善可、2-3 h |
| Inventory Fetch (`llinventorymodel.cpp:5610-5672`) | onCompleted で `responseToLLSD()` (= worker でも実行可) → processData で gInventory.updateItem / notifyObservers (main only) | **III**: parse 部を worker に出し、main は mutation/observer のみ。4-6 h |
| Mesh Fetch (`llmeshrepository.cpp:3625-3738`) | onCompleted で body 検証 + memory copy、processData → headerReceived で binary header parse + cache write (mHeaderMutex) | **IV**: deadlock risk (mHeaderMutex の取得 thread を要確認)、stress test 必須、8-12 h |

**timeout 連発 (Easy_28) 緩和**

failure path は parse 自体は走らない (body 空)。 食っているのは failure callback 内の observer fire / 状態遷移 / retry。

緩和策:
- worker thread での **failure callback rate limit** (1 frame あたりの failure callback 数上限)
- gInventory / gMeshRepository の `notifyObservers()` を deferred + batch (複数失敗を 1 回の observer call に集約)

**判定**

- Texture / Coroutine / XML-RPC: I (既に offload 済 / 意図設計)
- Material: II (2-3 h)
- Inventory: III (4-6 h)
- Mesh: IV (8-12 h、deadlock risk)
- timeout burst 緩和: II (failure callback rate limit + observer batch、独立 ROI あり)

**未確認**

- `LLMeshHandlerBase::onCompleted()` の実行 thread (worker か main か未確定)
- `gMeshRepository::mHeaderMutex` の取得 thread (deadlock risk の根本)
- timeout burst 時の observer notification queue depth
- LLSD shared_ptr の reference count thread-safety margin
- Material `onSuccess` callback の invocation thread context

---

### A6. Stream3D linkset eval blocking 解消

**対象**: 01 §6.2 「Stream3D positional stream」 (`llpositionalstreammgr.cpp:2398`)

**重大な訂正: 既に解消済み (r13 C で完了)**

agent 調査で 01 §6.2 / §7-B の所見が **古い情報に基づく推論** だったことが判明。

実際は:

- `LLStream3DUrlResolve::submit()` (`indra/llaudio/llstream3durlresolve.cpp`) で **専用 worker thread** が `curl_easy_perform` を実行 (Meyers singleton, line 318)
- main thread は `submit()` で即 return、`poll()` で非同期に結果確認 (line 407-422)
- timeout: `CURLOPT_TIMEOUT_MS = 1500ms`, `CURLOPT_CONNECTTIMEOUT_MS = 1000ms` (worker 側で消化)
- `LLPositionalStreamMulti::start()` (`indra/llaudio/llpositionalstreammulti.cpp:357-404`):
  - https:// URL のみ async resolve、http:// は skip
  - State::Resolving 中は `update()` で poll、Done/Failed 後に `openSourceStream()`
- `llpositionalstreammgr.cpp:2444` のコメント「still does a synchronous libcurl HEAD」は **r13 A/B 時点のもの (コメント未更新)**。実装は r13 C で async 化済み

**TP/login burst の実体**

- region 内 ~200 prim × Stream3D tagged 5-10 % = ~10-20 source
- worker thread 1 本で 1.5 s/url 想定 = ~30 s 待ち時間 (worker 内)
- main thread は **1-2 ms / frame のみ** (poll コスト)

**残存 sync path**

- worker 起動失敗時 fallback (line 386-393): `openSourceStream()` が raw URL で同期 open
- FMOD `createStream` 自体は `FMOD_NONBLOCKING` flag で async (`llpositionalstream.cpp:106-109`)

**判定**: **I (既に offload 済)**。 01 §6.2 / §7-B の Stream3D 関連の所見は訂正する。

**他 AYAstorm HTTP caller**: agent 確認の範囲で `llvenuereverbdsp.cpp` は local file I/O のみ (bundled WAV preload)、ネット I/O なし。01 §4 の「AYAstorm 独自 HTTP caller は LLCoreHttp 経由で検出されず」と整合。

**未確認**

- r13 C async worker の長期安定性 (週単位の leak / deadlock)
- request queue peak size 実測
- worker thread `cancel()` の実呼出頻度

---

### A7. (deep-dive 済、§A8-A14 として展開)

01 で「offload 可能性 高」一次判断だった 7 項目を 2026-05-26 に Agent deep-dive。一次判定の II 多数が実際には III/IV に格下げになるケース多発。詳細は §A8-A14。

---

### A8. local light frustum culling を background thread 先行化

**対象**: 01 §2 「Render Deferred Lighting」 (`pipeline.cpp:11049`, `pipeline.cpp:5279-5280`)

**現状の実装**

- `LLPipeline::calcNearbyLights(LLCamera& camera)` (`pipeline.cpp:7105-7331`) が **renderGeomDeferred() 内で per-frame 実行**。`mLights` 全体に対して camera 相対距離で frustum/range cull
- 処理: 既存 nearby light 更新 (line 7136-7221) / 新規 light 発見 (line 7224-7279) / 距離ソート (std::set<Light, Light::compare>)
- 呼出: renderGeomDeferred 直後 → setupHWLights (line 5280) → 結果 mNearbyLights は renderDeferredLighting (line 11391-11395) で local light loop に使用

**データ依存 / 同期前提**

- 読取り: mLights (集合)、drawable 状態 (getLightIntensity/isSelected/getLightRadius/getRenderPosition/isState(ACTIVE))、avatar 状態 (isTooComplex/isInMuteList/isTooSlow)、cvar、camera
- 書込み: mNearbyLights (完全置換)、drawable state bit NEARBY_LIGHT (set/clear)
- 並行アクセスリスク: mLights iterator invalidation (setLight() で dynamic add/remove)、drawable state bit non-atomic、avatar state は message pump と並行で変化

**offload 方式の現実性**

- worker thread で frame N-1 の light config を pre-calc → frame N で apply (1 frame lag)
- mNearbyLights を double-buffer / async queue 化
- drawable state bit (NEARBY_LIGHT) の atomic 版 or 遅延 apply が必要

**リスク**

- NEARBY_LIGHT flag を worker で clear しながら main が read → shader selection で glitch
- mLights iterator invalidation で crash (未 sync)
- 1 frame 古い camera pos で cull → off-screen light が lit / 近い light が cull miss
- avatar complexity transient inconsistency

**工数感**: 中〜大 (2-3 週)。worker thread pool integration + mNearbyLights double-buffer / async queue + drawable state bit の atomic 版

**判定**: **III (大手術)**。一次判定の II からは格下げ。1 frame lag と drawable state 同期の手間が大きい。**代替: V (削減)** で light count limit を下げ cull 基準を厳密化する方が筋の可能性。

**未確認**

- mLights の実 size (frame 単位 min/max/99%ile)
- calcNearbyLights の per-frame ms (light count 線形性係数)
- setLight() 呼出頻度
- NEARBY_LIGHT state が shader dispatch に影響する箇所
- 1 frame lag の視覚 acceptance test

---

### A9. post-proc luminance (luminance 計算 + auto-exposure adaptation)

**対象**: 01 §2 「RenderFinalize (tonemap / glow / DoF / AA)」 (`pipeline.cpp:10035`)、「post-proc luminance」 (`pipeline.cpp:10180`, `10182`)

**現状の実装**

- `LLPipeline::renderFinalize()` (`pipeline.cpp:10096-10395`) が per-frame 実行、main thread 占有
- HDR enabled 時 (line 10176): `generateLuminance` (8707-8752) → `generateExposure` (8754-8865) → `tonemap` (8869-8945+)
- CPU 側: RTT bind/unbind、shader bind/unbind、texture bind × 3-4、uniform set (cvar read + struct write)、setBuffer + drawArrays (3-4 draw/frame HDR path)
- **全 pass GPU shader 完結 (CPU readback なし)**、history feedback (mLastExposure 前 frame 値) で adaptive exposure

**データ依存 / 同期前提**

- 入力: mRT->screen, mGlow[1], mRT->deferredScreen, mLastExposure, cvar 多数
- 出力: mLuminanceMap (256×256), mExposureMap (1×1), tonemap output
- 特性: **全 GPU 完結 / order-dependent (luminance → exposure → tonemap) / history feedback / GL context 必須**

**offload 方式の現実性**

- (a) cvar read + uniform pack を background thread で事前計算 → main で uniform set のみ (sub-percent 級削減)
- (b) luminance を frame N-1 で先行実行 → GL context bound で実質 GPU latency を main に隠す方向、frame latency 増のトレードオフ
- (c) exposure を deferred (+1 frame lag、visual artifact 可能性)

**リスク**

- (a) cvar race (LLCachedControl で OK、write-side 確認必要)
- (b) frame lag 1-2 まで許容、3 以上で eye adaptation 劣化
- (c) LDR/HDR 転換時の brightness flicker

**工数感**: (a) 小 (1-2h), (b) 中 (3-5h), (c) 中〜大 (4-8h)

**判定**: **IV (main 残置必須) 準ずる**。GPU 完結 + order-dependent + GL context bound。他の II/III 候補の方が ROI 明確。secondary 候補として (a) のみ 0.5-1.0 ms 削減可能性あり。

**未確認**

- renderFinalize 全体の CPU 占有率 (% 未測定)
- generateLuminance / generateExposure の GPU time
- mLastExposure readback の main stall 有無
- LLCachedControl cache miss/hit 率

---

### A10. Draw batch 構築の frame N-1 化 (pre-build)

**対象**: 01 §2 「Pool iteration + per-object draw call emit」 (`pipeline.cpp:5057`, `llspatialpartition.cpp:346-402`)

**現状の実装**

- `LLPipeline::renderGeomDeferred()` (pipeline.cpp:5057) per-frame:
  - `updateCull()` (line 3021) で frustum cull → `sCull->beginVisibleGroups()`
  - `LLSpatialPartition::rebuildGeom()` (llspatialpartition.cpp:346) を visibility 確定後に呼出 (pipeline.cpp:4506-4509 「NEW_DRAWINFO && GEOM_DIRTY」時)
    - addGeometryCount (line 367): face→getPixelArea() で visibility cull
    - mVertexBuffer allocation (line 377-378)
    - getGeometry (line 391): draw command 抽出、mDrawMap 構築
- タイミング: **updateCull → rebuildGeom → mDrawMap → render submit** が全て frame N の render phase

**データ依存 / 同期前提**

- mDistance / mPixelArea (llspatialpartition.cpp:353-356): updateCull で確定、frame N camera 移動で更新必須
- face→mPixelArea (llface.cpp:2433): 視点ベース毎フレーム再計算、**frame N-1 build で frame N 視点変化を反映不能**
- GEOM_DIRTY / NEW_DRAWINFO state、mLastUpdateDistance (LOD 判定)

**offload 方式の現実性**

- frame N-1 末で予測カメラ位置から mDistance pre-calc → rebuildGeom 実行 → mDrawMap 生成 → frame N でそのまま submit
- 問題: 1 frame visibility lag (camera 急回転で 10-50 px 誤差) / object LOD jitter (popping) / dynamic drawable で再 rebuild trigger → 並列化効果消滅 / frustum clip 自体が毎フレーム変化

**リスク**

- camera 急回転/zoom で jitter 顕在化
- small object が 50-100 pix 境界で flicker on/off
- rebuildGeom が visibility-dependent state に基づき、frame N で state 変化なら再 rebuild → offload 意味喪失
- camera shake / network delay で visual pop

**工数感**: 中 (2-3 週)。touch file 3 本 + 変更行数 150-250 LOC + integration test 2 週間

**判定**: **III (大手術) に準ずるが IV 寄り**。visibility-geom build の 1-to-1 結合がフレームワーク設計で硬く、SoA 分離が baseline 設計上困難。force 化すれば再 rebuild 頻発で並列化効果消滅。

**未確認**

- camera prediction accuracy
- drawable per-frame LOD change 頻度
- mSlopRatio default と typical scene 頻度
- Firestorm fork での既存改修有無

---

### A11. region visibility update を worker thread 化

**対象**: 01 §3 「region visibility update」 (`llworld.cpp:1103`)

**現状の実装**

- `LLWorld::updateVisibilities()` (llworld.cpp:1062-1119) main thread per-frame、mVisibleRegionList / mCulledRegionList 双方 iterate
  - mCulledRegionList iter (1067-1084): frustum AABB check で visible へ移動
  - mVisibleRegionList iter (1087-1113): AABB frustum check、camera distance 計算、`updatePatchVisibilities()` 呼出
- `LLSurface::updatePatchVisibilities(LLAgent &agent)` (llsurface.cpp:758-776): terrain 全 patch per-frame visibility loop
- `LLViewerRegion::idleUpdate(F32 max_update_time)` (llviewerregion.cpp:1721-1784) 並行: terrain land idleUpdate / parcel overlay / cache culling

**データ依存 / 同期前提**

- 読: mVisibleRegionList / mCulledRegionList、camera frustum、LLSurface land (read-only)
- 書: list erase/push_back、LLSurfacePatch::mVisible (atomic 不確認)、camera distance per-patch、mLastUpdate per-region
- リスク: list erase during iteration (curiter で mitigate)、patch 非 atomic write、mLastUpdate / display read race

**offload 方式の現実性**

- 素直な offload 不可: updatePatchVisibilities が gAgent 依存 (terrain 視点相対計算、main thread only)
- 部分 offload: frustum AABB check のみ camera state 依存 → background で先行実行、frame N+1 で反映 (lazy update scheme)
- full offload 障壁: gAgent thread-safe snapshot or terrain patch visibility を per-10-frame 緩和

**リスク**

- list erase during iteration は 1 frame delay で safe だが patch visibility frame lag で terrain pop-in
- gAgent state stale (teleport / region crossing で visibility snap 遅延)
- camera frustum cache invalidation 不確実 → outdated frustum で glitchy cull
- rollback 困難 (visibility state が renderer に直結)

**工数感**: 中 (3-5 日)。touch file 2-3 + 100-150 LOC

**判定**: **II → 実質 III に格下げ**。gAgent 依存で full offload 困難、camera frustum cache 管理複雑、list erase border-line。部分施策 (frustum AABB check 10-frame lazy / terrain patch visibility per-10-frame) の方が工数小で ROI あり。

**未確認**

- LLSurfacePatch::updateCameraDistanceRegion の write thread-safety
- mLastUpdate update 頻度と display read pattern
- camera frustum state cache (isChanged()) 追跡信頼性
- gAgent teleport 中の visibility snap 時間要件

---

### A12. particle sim を worker thread 化

**対象**: 01 §3 「particle sim」 (`llworld.cpp:1206`)

**現状の実装**

- `LLWorld::updateParticles()` (llworld.cpp:1206-1210) main render thread per-frame
- `LLViewerPartSim::updateSimulation()` (llviewerpartsim.cpp:717-880):
  - mViewerPartSources iter → `LLViewerPartSourceScript::update(dt)` (line 804): particle emit 判定
  - mViewerPartGroups iter → `LLViewerPartGroup::updateParticles(dt)` (line 844): per-particle state update
- `LLViewerPartGroup::updateParticles()` (276-470): mParticles 逐次 loop、位置/速度/年齢補間、age expire、空間分割 transfer (group 外移動で別 group へ転送)、`gPipeline.markRebuild()` で drawable rebuild 要求

**データ依存 / 同期前提**

- `LLViewerPartSim::sParticleCount` (atomic<S32>): 既に atomic
- mViewerPartSources / mViewerPartGroups: 動的 add/remove
- per-particle state (mPosAgent/mVelocity/mAccel/...): non-thread-safe
- LLViewerObject::getRenderPosition (source/target object): read-only displayable
- `gPipeline.markRebuild()`: A1 と同じ race 潜在性

**offload 方式の現実性**

- emit/expire は atomic で worker safe
- updateParticles ループは per-particle 独立 → 分散可、SoA で SIMD 化も視野
- 空間分割 transfer の同期: mViewerPartGroups list resize で lock 必須 → 頻度高く contention
- drawable rebuild mark: lock-free queue or atomic bit set 必須

**リスク**

- list 動的更新で iterator invalidation
- 空間分割 transfer 頻度高で write lock contention
- gPipeline.markRebuild() race (A1 と共通)
- LLVPCallback (user script 定義) の thread-safety 未保証
- source/target object isDead / mDrawable swap 同期

**工数感**: 中〜大 (1.5-2.5 週)。touch file 4-5 + 300-500 LOC + list 同期機構 test 1 週間

**判定**: **III (大手術)**。emit/expire は解決可だが space transfer の list 同期が bottleneck。markRebuild race が A2 と同じ集合で改修必須。

**未確認**

- particle 転送頻度 (group boundary 超過 % per-frame)
- LLViewerObject::isDead / mDrawable lifecycle thread-safety
- callback 登録数 + 内部処理 thread-safety
- markRebuild 呼出頻度 + mMovedList append atomic 性
- particle peak count の環境依存性

---

### A13. inventory observer / avatar tracker / idle callbacks の batch 化

**対象**: 01 §3 「inventory observer」 (llinventorymodel.cpp:2339)、「avatar tracker」 (llcallingcard.cpp:513)、「idle callbacks fire」 (llcallbacklist.cpp:112)

**現状の実装**

- **inventory observer**: `LLInventoryModel::idleNotifyObservers()` (2340-2364) → `handleResponses(true)` → `notifyObservers()` で mObservers 全走査 (2386-2394)、観察者数 数十〜百
- **avatar tracker**: `LLAvatarTracker::idleNotifyObservers()` (508-515) → `notifyObservers()` で mObservers 全走査 (531-534) + per-friend `notifyParticularFriendObservers()` (536-539)
- **idle callbacks**: `gIdleCallbacks.callFunctions()` (111-119) で mCallbackList 逐次実行、登録数 数十、per-frame 無条件
- 全て main thread 同期実行、呼出元 `LLAppViewer::idle()` (6005-6007)

**データ依存 / 同期前提**

- inventory: mModifyMask / mChangedItemIDs (writer 複数、再入禁止 lock mIsNotifyObservers)、callback 内で UI/model 変更 → main only
- avatar tracker: mModifyMask / mChangedBuddyIDs (writer 複数: friend event / presence update)、再入禁止 lock、callback main only
- idle callbacks: mCallbackList non-atomic、callback 実行中 add/delete unsafe、callback 内容 diverse (coroutine/asset/UI)

**offload 方式の現実性**

- callback 実行自体は worker 不可 (UI/gAvatarList/asset main only)
- batch defer の選択肢:
  1. idleNotifyObservers 呼出を frame 境界条件化 (trivial、0.2 ms 改善期待小)
  2. observer callback を deferred queue で 1-2 frame まとめて notify (observer 数少なく効果限定)
  3. idle callback の per-frame 実行数上限 (priority categorize 必要)

**リスク**

- visibility delay: UI 反映 1 frame 遅延、TP/timeout 時に「item ない」期間延長
- callback order 変化で UI state 依存破壊
- recursive change で backlog 落ち
- nested lock 複雑化 (A5 failure rate limit との両立)

**工数感**: 小〜中 (1-2 日)。30 行程度 + priority categorize 2-4h

**判定**: **II (高) + 条件付**。failure callback rate limit (A5) と同期実装が必須、単独では限界効益。idle callback 頻度削減が II 候補、observer batch 化は A5 後の secondary。

**未確認**

- idle callback 登録数 + 典型 callback 実行時間 (profile 計測必要)
- observer callback の平均実行時間 (inventory/avatar 別)
- failure callback rate limit (A5) の既存実装有無
- UI 応答性要件 (1 frame defer 許容可か)

---

### A14. processTextureStats を worker thread 化

**対象**: 01 §5 「Texture Stats Update (LOD calc)」 (`llviewertexture.cpp:1803-1882`)

**現状の実装**

- `LLViewerTextureList::updateImagesFetchTextures()` (1256-1323) per-frame で ~32 + mUUIDMap.size()/20 件に `updateImageDecodePriority()` 呼出 (1316)
- `updateImageDecodePriority()` (929-1114) の最後で `imagep→processTextureStats()` 呼出 (1113)
- `LLViewerFetchedTexture::processTextureStats()` (1803-1892):
  - `updateVirtualSize()` (1821) = lazy cleanup (20s ごと)
  - mDesiredDiscardLevel 再計算 (60-80 行): mMinDesiredDiscardLevel / mLoadedCallbackDesiredDiscardLevel / mFullWidth/Height / mKnownDrawWidth/Height との比較

**データ依存 / 同期前提**

各 texture の per-member は texture-local:
- mFullyLoaded (R/W), mDesiredDiscardLevel (W), mMinDesiredDiscardLevel (R)
- mFullWidth/Height (R, immutable post-init)
- mKnownDrawWidth/Height (R, set by avatar/UI via setKnownDrawSize)
- mKnownDrawSizeChanged (R/W flag)
- cvar (atomic、競合なし)

**thread-safety**:
- 読側 immutable 寄り
- lazy cleanup の face list erase は resize のみで lock-free 可
- 競合点: mKnownDrawSizeChanged flag の read-write timing (LLFace::getTextureVirtualSize で set / processTextureStats で reset)

**offload 方式の現実性**

- 法1: per-texture worker queue (texture 単位 distribute、async)
  - pro: データ locality、per-texture 独立
  - con: per-frame 処理数が 1/16 frame 頻度で parallel gain 限定
- 法2: batch parallel (複数 texture を一括 job で worker subset 処理)
  - pro: setup cost amortize
  - con: workload imbalance (texture size/priority 不均一)

**リスク**

- setKnownDrawSize / mKnownDrawSizeChanged write timing race
- false sharing: mDesiredDiscardLevel write × 複数 worker で cache line contention
- updateFetch() で mDesiredDiscardLevel read 時の before-happen-relationship
- 視覚回帰: priority 計算遅延で texture load order 変化、load sequence glitch (低確率)

**工数感**: 中 (1-2 日)。worker dispatch 基盤は既存 (texture decode pool/work queue)、50-100 LOC + thread-safety 検証 2-3 日

**判定**: **II (offload 可能性 高)** だが **実測前の仮判定**。条件:
1. processTextureStats が per-frame total CPU の 1% 以上を占める場合のみ ROI あり (profile 未実装)
2. per-texture state 競合が実測で無視できること
3. mKnownDrawSizeChanged timing race を atomic flag で保護

**未確認**

- processTextureStats の実測占有時間 (profile zone 未実装)
- setKnownDrawSize との同 frame 競合頻度
- mDesiredDiscardLevel update タイミング constraint
- texture count large (10k+) での parallel speedup simulation
- 既存 texture decode pool / work queue の capacity 余裕度

---

## §B. 軽量 candidate (深掘り不要、一次判断確定済)

01 §6 で「軽い (低コスト)」と判定された AYAstorm 独自処理。深掘り不要。

| 系統 | 一次判定 | 出典 |
|---|---|---|
| picker (self / other rigged) | 困難 (frame pixel-perfect readback) | 01 §6.5 |
| chat tab split | 低コスト、offload 不要 | 01 §6.6 |
| sun dazzle / cloud volumetric / translucency | GPU side 完結 | 01 §6.8a/d/e |
| avatar SSS flag propagate | 低コスト | 01 §6.8f |
| godrays cvar push | 低コスト | 01 §6.8c |
| Cinematic Controls overlay apply | 起動時 1x、非クリティカル | 01 §6.9 |

---

## §C. 分類サマリ

deep-dive A1〜A6 + §B の結果を分類軸に再配置。

| カテゴリ | 該当 candidate | 期待効果 (一次判断) | 工数 |
|---|---|---|---|
| **I. 既 offload** | image decode / HTTP CURL I/O / mesh fetch / r24 dullahan callback / **A5 Texture+Coroutine+XML-RPC** / **A6 Stream3D linkset eval** | n/a (確認のみ、01 §7-B の所見訂正対象を含む) | — |
| **II. 可能性 高** | **A2 octree balance lazy trigger** / **A3 occlusion timeout 拡大 + discard** / **A4 NVIDIA polling + sleep** / **A5 Material callback defer** / A5 failure callback rate limit + observer batch / A7 多数 (light culling / luminance / processTextureStats / particle / region visibility / observer batch) | 静止 frame で 0.1-0.5 ms (A2) / stall 削減 (A3) / worker 30%→5-15% (A4) / 数 % 級 (A5/A7) | 各 1-2 日 〜 1 週 |
| **III. 大手術** | **A1 idleUpdate thread pool** / **A5 Inventory parse offload** / A7「draw batch 構築 frame N-1 化」 | 大 (理論上 main 30-40 % 削減候補) | 各 1-2 週、test 5-8 h |
| **IV. main 残置必須** | render submit / GL bind / state sort / occlusion query 結果の cull 反映 / message pump / picker readback / **A5 Mesh parse offload** (deadlock risk で III に近い IV) | n/a (改善余地 = 削減側) | — |
| **V. 削減で対処** | **A3 cull 効果ゼロ** (occluder sizing / query 発行頻度を絞る) / 21 k object 数自体を draw distance / max non-impostor で削る運用面 | scene 依存 | tuning 1 日 |

---

## §D. 03 以降への引き渡し (優先順位案)

ROI と工数感から、03 以降で扱う改善 1 件 1 ファイル単位の優先順位:

1. **03-occlusion-timeout-tune** (A3 stall 軸): 
   - `RenderOcclusionTimeout` cvar 値を 4 → 8 / 16 で実機検証
   - 効果あれば timeout 到達時 discard + 前 frame state 保持を実装
   - 工数: 1-2 日。test: 3 OS。直接 main thread stall を減らす最短ルート

2. **04-octree-balance-lazy** (A2): 
   - dirty flag pattern で `mNeedRebalance` 導入、insert/remove/move で set、updateMove で skip
   - 工数: 1-2 日。視覚回帰なし、静止 frame で安定の積み上げ

3. **05-http-failure-burst-mitigation** (A5 timeout 緩和軸): 
   - failure callback rate limit + gInventory/gMeshRepository observer batch
   - 工数: 1-2 日。今日見えた Easy_28 連発時の main 圧迫に直接効く

4. **06-nvidia-fence-polling** (A4): 
   - polling + 100µs sleep で `glClientWaitSync` busy-wait 解放
   - 工数: 2-3 日 (実装は短いが 3 OS × 2 vendor regression test 重い)
   - net FPS gain は限定的だが、worker thread 30 % CPU の削減は明確な改善

5. **07-inventory-parse-offload** (A5 III): 
   - `responseToLLSD` を worker thread に移し、main は updateItem/notifyObservers のみ
   - 工数: 1 週。 LLSD shared_ptr 安全性の慎重な検証必須

6. **08-light-culling-async** / **09-region-visibility-worker** / **10-process-texture-stats-parallel** (A7 II 群): 
   - 各 1-2 日。02 で deep-dive 未実施なので 03 着手前に追加 deep-dive 1 本ずつ

A1 (idleUpdate thread pool) と「Mesh parse offload」と「draw batch frame N-1 化」 は **III 大手術**。 01-06 で稼げる時間 + 実測 baseline が揃ってから着手判断する (= 03〜10 で稼ぐ → 11 以降に III 着手)。

03 着手前に必ず Frame Profile + perf record で baseline を取り直し、`/home/ishikawa/work_firestorm/phoenix-firestorm/docs/specs/ayastorm-cpu-perf/00-overview.md` の出発点 data point と並べる。

---

## §E. 1 週間 deep-dive POC spike 結果 (Day 4-5 / Day 5-6)

Day 1-2 (doOcclusion Layer 8) + Day 2-3 (vwDraw Layer 8) で **計測** 側の地図完成と並行して、Day 4-5 / Day 5-6 で **剥がし候補 2 件の構造的成立性** を read-only survey で検証 (実装試行はせず grep / file:line ベースで blocker を炙り出す)。

### §E.1 Day 4-5: renderShadow refactor POC (sCull / mNumVisibleFaces) — 案 R

**Hypothesis**: `LLPipeline::sCull` parameter 化 + `LLDrawable::mNumVisibleFaces` thread-local 化 で renderShadow を worker thread に剥がせるか。

#### 構造調査結果

| 観点 | 状態 | 詳細 |
|---|---|---|
| `LLPipeline::sCull` global 参照 | 🟢 GREEN | pipeline.cpp 内 72 site、書込 3 箇所 (grabReferences 2747 / clearReferences 2753 / clear 3063)、updateCull/stateSort は既に result 引数受取済 → parameter 化は機械的低リスク |
| `LLDrawable::mNumVisibleFaces` | 🟢 GREEN | 書込 3 site のみ (528 init / 2529 preCull reset / 4262 stateSort 累積)。frame-level diagnostic counter で control flow 依存無し |
| `markVisible` callback 散在 | 🟢 GREEN | pipeline.cpp:3768 単一実装、leaf class への散在無し |
| `LLPipeline::sShadowRender` 等 global flag mutation | 🟡 YELLOW | renderShadow 内で `sShadowRender` (12503/12719) + `sUseOcclusion` (12507/12718) 書込、12 file 以上で読込条件分岐。parameter 化必要 |
| **GL context tie (gGL.pushMatrix / shader bind / setColorMask)** | 🔴 **RED — 構造的 blocker** | renderShadow body 内 ~100 行 直接 GL 呼び出し (12547-12714)。GL は main thread 専有のため renderShadow 全体の worker 移送は **不可能** |

#### 結論

**renderShadow 全体の worker 化は不可** (GL context blocker)。**ただし pre-cull (updateCull(shadow_cam, &result) + stateSort(shadow_cam, &result)) を worker に剥がす partial split は green** — renderShadow body 内で消費される shadow_result を frame N-1 で worker A 上に先行構築し、main thread の renderShadow body は dispatch だけに専念する scope に再定義可能。

#### Phase 1.2 で確定する scope (案 R-refined)

- worker 側担当: **shadow camera 4 cascade 分の `updateCull` + `stateSort` traversal** (renderShadow 内では `updateCull(shadow_cam, result)` line 12536 + `stateSort(shadow_cam, result)` line 12541 に該当)
- main 側残存: GL dispatch (matrix push/pop、shader bind、renderGeomShadow、renderAlphaObjects、renderMaskedObjects)
- 同期境界: 4 cascade × (shadow_camera snapshot, result buffer) = read-only frustum + write-only buffer queue
- 期待短縮: renderShadow_body 内 cull + stateSort 合計の ~20-30% (実測は 04 §4.2.h R 系 sub-zone の数字に基づき Phase 1.2 で確定)

### §E.2 Day 5-6: L6-cull POC (mState atomic / pushVisibleGroup per-task) — 案 P

**Hypothesis**: `LLSpatialGroup::mState` を atomic U32 化 + `LLCullResult::pushVisibleGroup` (および兄弟 push 群) を per-task buffer 化することで、shadow_cam + main_cam の cull traversal を並列実行できるか。

#### 構造調査結果

| 観点 | 状態 | 詳細 |
|---|---|---|
| `LLSpatialGroup::mState` 書込 site | 🟢 GREEN | 18 site すべて main thread (llspatialpartition.cpp 239/309/318/401/432/475/485/504-525/534/553-574/601/675/781/796/877)、network/asset thread 書込無し |
| `LLCullResult` storage | 🟢 GREEN | 11 buffer (mVisibleGroups / mAlphaGroups / mRiggedAlphaGroups / mOcclusionGroups / mDrawableGroups / mVisibleList / mVisibleBridge / mRenderMap[8]) すべて `std::vector<T*>` append-only、reader 側は read-only。per-task buffer + 末尾 merge は trivial |
| `push*` method call site | 🟢 GREEN | 7 site (pushVisibleGroup 3138 / pushOcclusionGroup 3159/3167 / pushDrawableGroup 3134 / pushDrawable 3100/3111/3803 / pushBridge 3798 / pushDrawInfo 4541 / pushAlphaGroup 4572) — 全て cull 経路 sequential、順序依存 cross-bucket 無し |
| 非 main thread mState mutation | 🟢 GREEN | llvocache.cpp 328/339 のみ、VO cache 寿命管理経路で cull traversal 開始前完了。実 concurrent 書込無し |
| **OCCLUDED bit cross-camera 共有** | 🟢 **GREEN — 既に per-camera 化済 (2026-05-26 grep 確定)** | `OCCLUDED = 0x00010000` は `LLOcclusionCullingGroup` の OCCLUSION_STATE enum (llvieweroctree.h:279)、storage は `mOcclusionState[LLViewerCamera::NUM_CAMERAS]` (llvieweroctree.h:332)。`set/clearOcclusionState(OCCLUDED, ...)` は全 site で `STATE_MODE_DIFF` = per-camera (llvieweroctree.cpp:1126/1154/1158/1197)。`STATE_MODE_ALL_CAMERAS` は `DISCARD_QUERY` 専用 (llspatialpartition.cpp:310/964)。`SG_STATE_INHERIT_MASK & parent->mOcclusionState[i]` (llvieweroctree.cpp:879) も per-camera index `i` 内の parent→child 継承で cross-camera leak は構造上発生しない |

#### 結論 (2026-05-26 更新)

**atomic mState + per-task push buffer 自体は 1 週間で実装可能 (GREEN)。**

**当初想定していた "OCCLUDED bit を per-camera 化する prerequisite refactor" は不要** — 上記 grep finding により OCCLUDED は既に `mOcclusionState[per-camera]` に格納済で、cross-camera bit leakage は構造上発生しない。Day 5-6 POC 時の RED 判定は `mState` 側に OCCLUDED bit が同居していると誤読していたもので、実コードでは `mState` (`LLSpatialGroup::eSpatialState`) と `mOcclusionState[]` (`LLOcclusionCullingGroup::OCCLUSION_STATE`) は独立 enum / 独立 storage。

#### Phase 1.2 で確定する scope (案 P-refined、2026-05-26 改訂)

- prerequisite refactor: **不要** (OCCLUDED は既に per-camera 化済、上記 grep 確定)
- 本実装: atomic mState + per-task LLCullResult buffer
- 期待効果: main_cam updateCull (1.04 ms/frame) と shadow_cam updateCull 4 cascade (cascade 別 budget は 04 §4.3 sun cascade 分析参照、合計 ~12 ms/frame) の並列化 — 上限で 4-5 ms/frame 圧縮可能性
- 残リスク: `mState` 側 (DIRTY / OBJECT_DIRTY / GEOM_DIRTY 等) の cross-camera 共有は未確認。Phase 2 着手時に main_cam / shadow_cam 並列で書込競合する mState bit を再 grep して洗う (atomic 化対象の確定)

---

## §F. Day 7 — 1 週間 deep-dive 統合判定 (go/no-go)

Day 1-2 / Day 2-3 (計測地図) + Day 4-5 / Day 5-6 (POC spike) の集計。

### §F.1 4 候補の実測 ceiling

| 候補 | 構造判定 | 実装工数感 | 期待短縮 ceiling | go/no-go |
|---|---|---|---|---|
| 案 O (doOcclusion async) — Hero probe iteration loop 全体を worker に剥がす | 🟢 GREEN (rmdo_* state machine 22.7%、parent − Σ = 77.3% iteration loop) | 1-1.5 週 (Worker A pool infra + frustum snapshot 配線) | doOcclusion_reflectionProbes 8.1 ms/frame の **~70-80%** = **5.7-6.5 ms/frame** | **GO** |
| 案 Q (vwDraw text cache) — text width measurement の per-frame caching | 🔴 RED (19 周目 Group O 実測 = 0.145 ms/frame、Layer 1 推定 18.6 ms/frame は session diff) | 0.5-1 週 | 100% 削減でも frame budget の 0.5% (0.08 ms) | **DROP** |
| 案 R-refined (renderShadow pre-cull worker) — shadow cascade 4 つの updateCull + stateSort のみ worker | 🟢 GREEN (sCull/mNumVisibleFaces 共に scope 小、GL blocker は body 側のみ) | 1 週 (worker B + 4 cascade buffer + sShadowRender parameter 化) | renderShadow body 内 cull + stateSort 部分の **~25%** = **~2-3 ms/frame** | **GO** |
| 案 P-refined (mState atomic + per-task cull buffer + 並列 cull) | 🟢 GREEN (2026-05-26 grep finding で OCCLUDED prerequisite 消滅、§E.2 更新) | 1-1.5 週 (本実装のみ) | main + shadow cam 並列で **4-5 ms/frame** | **GO** |

### §F.2 章 scope 凍結案 (案 Y-refined)

Phase 1.2 (Core 振り分け設計) で取り扱う剥がし候補を **3 件に絞る**:

1. **案 O (doOcclusion async)** — 最大の ceiling、構造 GREEN、即着手可
2. **案 R-refined (renderShadow pre-cull worker)** — GREEN、案 O と独立に着手可
3. **案 P-refined (mState atomic + 並列 cull)** — 案 O / R 完成後の追加 ceiling (OCCLUDED prerequisite は 2026-05-26 grep finding で消滅、§E.2 / §F.3 更新)

**案 Q (vwDraw text cache) は DROP 確定** — Day 2-3 Group O 実測で vwDraw 全体 0.145 ms/frame、100% 削減でも frame budget の 0.5% (詳細は 04 §4.2.j)。

### §F.3 リスク register

| risk | 影響 | mitigation |
|---|---|---|
| 案 O worker から GL query 発行不可 | 案 O の implementation が pure CPU 化に限定される (GL Begin/End は main 残し) | scope §4.2.i §Phase 1.2 直接 input で明記済 — worker は probe visibility 判定 / 結果消費のみ |
| 案 R-refined の sShadowRender parameter 化が 12 file 以上に波及 | refactor cost 増 | sShadowRender を ShadowRenderContext struct に集約、leaf 側は context ptr 受け取りに変更 (機械的) |
| ~~案 P-refined OCCLUDED 分離で複数カメラ共有最適化破綻~~ | ~~shadow / probe / impostor の occlusion 共有が崩れる~~ | **2026-05-26 解消** — OCCLUDED は既に `mOcclusionState[per-camera]` 化済 (§E.2 更新)、分離 refactor 自体が不要のため本 risk は構造上発生しない |
| 案 P-refined の `mState` 他 bit (DIRTY / GEOM_DIRTY 等) cross-camera write 競合 | main_cam / shadow_cam 並列 cull で同一 group の `mState` を同時 write した場合の race | Phase 2 着手時に `mState` 全 bit を再 grep し、cross-camera write site が出る bit のみ atomic 化対象に絞る (現時点では cull traversal hot path 内の write site 18 個全て main thread + frame-scoped で race 確認できず) |
| 計測 instrumentation overhead (Group O drawChildren parent-name 比較) | 1 frame あたり drawChildren 呼出数 × 2 文字列比較 ≈ 数 µs overhead | AYAPerfLogEnabled=0 default で zero overhead、deploy 影響無し |

### §F.4 go/no-go

**Phase 1.2 着手 = GO**。Phase 1.1 完成 (Layer 1-8 地図埋め + 4 候補 POC) で当初予定通り week 完走。

着手順序は 05-core-assignment-plan.md Y-refined 改訂版 §7 で確定:
1. 案 O から (ceiling 最大 + 構造シンプル) — 期待 5.7-6.5 ms/frame 短縮
2. 案 R-refined (案 O と independent、Worker B 立ち上げ) — 期待 2-3 ms/frame 短縮
3. 案 P-refined (mState atomic + 並列 cull) — 期待 4-5 ms/frame 短縮 (2026-05-26 OCCLUDED prerequisite 消滅、§E.2 / §F.3 更新で条件付 → 純 GO)

**剥がし候補 3 件合計の理論上限 = 11.7-14.5 ms/frame** (60 fps 化に必要な 16.67 ms - 当面の baseline frame ms との gap 確認は Phase 2 着手時に Frame Profile 再取得)。

**案 Q は DROP** (Day 2-3 実測で frame budget 0.5% 確認、04 §4.2.j 参照)。
