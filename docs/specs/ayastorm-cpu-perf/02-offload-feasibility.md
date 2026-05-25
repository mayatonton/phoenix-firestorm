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

### A7. その他の II/III/V 候補 (deep-dive 未実施、列挙のみ)

01 で「offload 可能性 高」一次判断だったが今回 deep-dive に入れなかった項目。03 以降で順次扱う。

- 01 §2「local light frustum culling」 を background thread 先行 (II 候補)
- 01 §2「post-proc luminance」 を async に分離 (II 候補)
- 01 §2「draw batch 構築」 を frame N-1 化 (III 候補)
- 01 §3「region visibility update」 を worker thread (II 候補)
- 01 §3「particle sim」 を async (II 候補)
- 01 §3「inventory observer / avatar tracker / idle callbacks」 を batch 化 (II 候補)
- 01 §5「processTextureStats」 を parallel 化 (II 候補)

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
