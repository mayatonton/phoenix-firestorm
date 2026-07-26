# 資産ロード失敗 回復契約(全クラス列挙 + 統一設計)

- 状態: 設計者起草 2026-07-26(AYA 指示「ロードしてる全データを列挙し、失敗時どうしているかを設計せよ。計測がクリーンだったことを実装しない根拠にするな」)。実装 GO 済(AYA「全て設計して実装してください」)。
- 背景: 本 viewer は検出器(計測)は整備してきたが、**ロード失敗の回復(リトライ)契約が未設計**。「counter が今ゼロ」は「装置が知る穴が今ゼロ」でしかなく、失敗した資産がいつ・誰の責任で再試行されるかはクラスごとにバラバラ or 不在。
- 真実源: HEAD 実コード(下表 file:line 全て実読)。

## 0. 不変条件(本設計の核)

**全ての描画資産は『LOADED / RETRY_SCHEDULED(次回時刻を持つ)/ FAILED_TERMINAL(明示 fallback 表現を持つ)』のいずれかに常にある。**
- 「無期限の暗黙放置」(retry も terminal 宣言もない状態)を構造的に禁止する。
- 「無計画の無限再要求」(backoff なしの再要求ループ)も禁止する(= リトライ嵐)。
- terminal は再武装トリガー(region 変更・明示 refresh)で RETRY_SCHEDULED に戻れる。
- 機械検査 = **stuck オラクル**: クラス毎に「非 LOADED かつ非 SCHEDULED かつ非 TERMINAL」の件数を周期報告し、0 でなければ alarm(fail-closed)。幸運な run のログではなく、機構が常時自己検査する。

## 1. ロード資産の全列挙と現状の失敗時挙動(実トレース済)

| # | 資産クラス | 取得経路 | 失敗時の現状(file:line) | 判定 |
|---|---|---|---|---|
| 1 | **Texture(j2c)** | HTTP CDN(worker) | 404 = region 変更で再試行 + OpenSim UDP fallback(lltexturefetch.cpp:1712-1752)/ 503 = region 変更再試行(:1760-1776)/ **403・その他 = 汎用 else(:1782)→「fail harder」DONE(:1809-1814)→ missing 未設定 → 可視なら priority ループが即再要求(llviewertexture.cpp:2150-2226)= backoff なし無限再要求嵐**。decode 失敗/寸法不正 = setIsMissingAsset(llviewertexture.cpp:1573,2013)= **terminal・再武装なし(relog まで)** | 🔴 403 系 = 無計画嵐 / missing = 恒久放置 |
| 2 | **Mesh(header/LOD)** | HTTP caps(mesh repo) | **指数 backoff 付き 8 回再試行**(RequestStats::updateTime llmeshrepository.cpp:547-558・2^n 遅延・計 ~64s)→ 超過 or 不在 = 「non-existent, will not retry」(:2388)terminal | 🟢 唯一の模範(house 標準に採用)/ 🟡 terminal 再武装なし |
| 3 | Mesh skin info / physics | 同上 mesh repo | #2 と同機構 | 🟢 |
| 4 | **Animation(AT_ANIMATION)** | asset storage | onLoadComplete 失敗/decode 失敗 → `ASSET_FETCH_FAILED`(llkeyframemotion.cpp:603,614)→ **STATUS_FAILURE で terminal・再試行機構ゼロ**(:522)= その motion は relog まで永久に再生されない | 🔴 恒久放置 |
| 5 | **Wearable/Shape(AT_BODYPART/AT_CLOTHING)** | asset storage | **即時 3 回再試行(backoff なし・llwearablelist.cpp:196-206)**→ 失敗 = NULL + 通知(FailedToFindWearable)→ **avatar は default 形状のまま・以後再試行なし** / NOT_IN_DATABASE = 即 terminal | 🟡 有限だが backoff なし・terminal 再武装なし |
| 6 | **GLTF material(AT_MATERIAL)** | asset storage | 失敗 → `materialComplete(false)`(llgltfmateriallist.cpp:528)→ `mFetchSuccess=false` 恒久(llfetchedgltfmaterial.cpp:262-273)= **fallback material のまま再試行ゼロ**(allowlist 登録済の頻発 WARN) | 🔴 恒久放置 |
| 7 | **Server bake texture** | HTTP(FTT_SERVER_BAKE) | 404 = NOT_WRITE → **即 DONE abort・再試行なし**(lltexturefetch.cpp:1714-1723)。回復は次の appearance メッセージ頼み | 🔴 自律回復なし |
| 8 | Sound(AT_SOUND)/ Gesture | asset storage | 未トレース(本表の残項・実装フェーズで確定) | ⚪ 未トレース |
| 9 | 環境設定(AT_SETTINGS/EEP) | asset storage | 未トレース(失敗 = default 環境のはず) | ⚪ 未トレース |
| 10 | Object(interest list)/ terrain | UDP + VOCache | server 側再送 + cache。収束はサーバ責任で成立 | 🟢 対象外 |
| 11 | Skeleton/avatar_lad(local xml) | ローカル file | 起動時 fatal(fail-fast)= 正 | 🟢 対象外 |
| 12 | settings/skins/fonts/shader | ローカル file | 起動時 fatal or 既知 crash(Missing Files)= fail-fast | 🟢 対象外 |
| 13 | fsdata | phoenixviewer.com | cache fallback → なければ WARN 継続(fsdata.cpp:95-101)。実害 = MOTD/タグ色のみ | 🟡 別 TICKET(product) |

**総括: 模範(mesh の指数 backoff)が 1 つだけ存在し、他は「無計画嵐(texture 403)」「恒久放置(animation/GLTF/missing/bake)」「backoff なし有限(wearable)」が混在。統一契約が存在しない。**

## 2. 統一設計 = AssetRetryPolicy(mesh 方式の全クラス標準化)

mesh repo の実証済みパターン(`RequestStats`: 2^n × base 遅延・上限 N 回)を共通意味論として全クラスへ:

1. **一時失敗**(403/5xx/timeout/no-data/cancel)→ **指数 backoff 再試行**(base 4s・2^n・上限 8 回 ≈ 累計 ~17 分)。再試行中は RETRY_SCHEDULED(次回時刻を保持)。
2. **恒久失敗**(NOT_IN_DATABASE・decode/寸法不正・malformed)→ **FAILED_TERMINAL + 明示 fallback**(texture = missing_asset 画・wearable = default + 既存通知・material = fallback mat・animation = 不再生)。
3. **再武装トリガー**: region 変更(texture 404 の既存パターンを標準化)と明示 refresh(rebake 等)で terminal/上限到達 → RETRY_SCHEDULED に復帰。
4. **stuck オラクル**: クラス毎カウンタ `loaded/scheduled/terminal/stuck` を PERF_LOG 周期報告。**stuck>0 = alarm**(fail-closed・幸運ログ依存の排除)。
5. **嵐の禁止**: 再試行は必ず backoff を経る。可視性 priority による「即再要求」は初回のみ許可(現 texture 403 ループの是正)。

## 3. 実装計画(クラス毎・優先 = field 実害順)

| 段 | クラス | 実装 | 規模 |
|---|---|---|---|
| **R1** | Texture 403/汎用失敗 | per-texture 失敗カウンタ + backoff timer(updateFetch の make_request gate に挿入)・上限後 terminal(missing 画)・region 変更で再武装。既存 404/503 経路は不変 | 中 |
| **R2** | GLTF material | mFetchSuccess=false → backoff 再 fetch(bounded)→ terminal | 小 |
| **R3** | Animation | ASSET_FETCH_FAILED → backoff 後 ASSET_NEEDS_FETCH 復帰(bounded)→ terminal | 小 |
| **R4** | Wearable | 即時 3 連発 → backoff 化 + region/明示トリガー再武装(通知は現状維持) | 小 |
| **R5** | Server bake | NOT_WRITE abort → bounded backoff 再試行 | 小 |
| **R6** | stuck オラクル | 各クラスの状態カウンタ + 周期報告(PERF_LOG) | 小 |
| R7 | Sound/EEP 残項 | トレース → 同契約適用 | 小 |

## 4. gate(正のオラクル必須 = 「エラーが出なかった」では PASS しない)

- **失敗注入足場(AYA 裁定 2026-07-26 = 必須・全 4 クラス実装済)**: 統一 env `AYASTORM_ASSET_FAIL_INJECT=<N>` = **各資産の最初の N 回の取得完了を人工失敗化**(texture/material/animation/wearable の 4 注入点)。UUID%率方式は「同じ資産が永遠に失敗 = 回復を証明できない」欠陥があるため不採用。**N=2 run = 失敗→backoff→実成功の回復連鎖を全資産で証明(正のオラクル)/ N=9 run = 上限→terminal→fallback 表現を証明**。
- 嵐なし証明: 注入下で再要求レートが backoff 曲線に一致(無限ループ不在)。
- stuck オラクル = 0(注入下でも全資産が 3 状態のどれかにいる)。
- 回帰なし: 注入なし通常 run で従来挙動不変 + validation 0。

## R1-JIT 詳細設計(Texture・実装直前仕様・全 file:line 実読済)

### 現行機構の完全な姿(置換対象の確定)
- worker 側(lltexturefetch.cpp)= in-flight 再試行: 404→region 変更再試行+UDP fallback(:1712-1752)/ 503→region 変更再試行(:1760-1776)/ 403・他→fail harder DONE(:1809-1814)。**R1 は worker を触らない**(in-flight 再試行はそのまま)。
- texture 側(llviewertexture.cpp・main thread)= 完了後の再要求判断:
  - processFetchResults の no-data 分岐(:1994-2032)= ①未描画なら即 `setIsMissingAsset()`(恒久)②部分データありなら `mMinDiscardLevel` clamp(解像度向上の暗黙恒久放棄)
  - make_request gate 連鎖(:2150-2170)に**失敗の記憶が無い** → 可視なら即再要求 = 嵐

### 変更仕様(足し算でなく置換)

**A. フィールド追加(llviewertexture.h・LLViewerFetchedTexture)**: `U8 mFetchFailCount` / `LLTimer mFetchFailTimer` / `LLUUID mFetchFailRegion` / `bool mMissingTransient`。init() で初期化。

**B. 失敗 arm の置換(processFetchResults :1994-2032 の no-data 分岐を書き換え)**:
- 旧 2-arm(即 missing / 即 clamp)を削除し FSM arm に置換:
  - `FTT_MAP_TILE` = 現行どおり即 terminal(意味論維持)
  - transient(mLastHttpGetStatus が 403/5xx/timeout/不明 no-data。**404 は非 transient**=worker の region 再試行を使い切った後の 404 は missing が正)かつ `++mFetchFailCount <= 8` → `mFetchFailTimer.setTimerExpirySec(4 << (count-1))` = RETRY_SCHEDULED(missing/clamp しない)
  - それ以外(非 transient or 上限到達)→ terminal: 未描画なら `setIsMissingAsset()` + `mMissingTransient=transient` + `mFetchFailRegion=現 region` / 部分データありなら clamp(現行 terminal 意味論に着地)
- decode 失敗/寸法不正の missing(:1573,1287)= content 系 → `mMissingTransient=false` のまま = 再武装対象外(現行維持)

**C. backoff gate 挿入(make_request 連鎖 :2150-2170 に 1 本)**: `else if (mFetchFailCount > 0 && !mFetchFailTimer.hasExpired()) make_request = false;` — これが嵐の構造的削除。

**D. 成功リセット**: updateFetch :2119 `finished` 直後、`mRawImage.notNull()` なら `mFetchFailCount=0`。

**E. lazy 再武装(全 texture 走査なし)**: make_request gate 手前で `mIsMissingAsset && mMissingTransient && 現 region != mFetchFailRegion` なら missing 解除 + counter リセット(region 変更で terminal から復帰)。

**F. 削除される挙動(申告)**: ①失敗を覚えない即再要求(嵐)②一時失敗での即 missing 化(≤8 回は再試行が挟まる)③一時失敗での即 clamp。

**G. 注入足場**: 統一 env `AYASTORM_ASSET_FAIL_INJECT=<N>`(mFetchFailCount < N の間 失敗化 = 最初の N 回失敗 → 実成功で回復連鎖を証明。worker 不変・env 無しコストゼロ)。旧 UUID%率方式は回復を証明できないため置換済み。

**H. texture 分 stuck カウンタ**: 遷移時 inc/dec の static(scheduled/terminal_transient)+ PERF_LOG 時 10 秒毎 1 行。

### edge case 列挙
map tile = FSM 適用外(即 terminal・現行維持)/ server bake = FSM に乗る(R5 の前倒し・害なし・申告)/ UDP(OpenSim)no-data = transient 扱い(現行嵐より改善)/ BOOST_ICON・oversize・decode 失敗 = content terminal(再武装なし・現行維持)/ 部分データ textures = limit 内は clamp せず再試行・limit で clamp。

## R2-JIT(GLTF material)

- 現行(実読): getMaterial(llgltfmateriallist.cpp:620-643)= session 中 1 回だけ fetch → 失敗 = materialComplete(false)(llfetchedgltfmaterial.cpp:262)= mFetchSuccess=false 恒久・waiters 全 flush・再 fetch 経路なし。
- **置換**: onAssetLoadComplete 失敗分岐(:526-531)を FSM 化 — transient(status != LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE)かつ ++mFetchFailCount≤8 → `doAfterInterval(4<<(n-1))` で getAssetData 再発行。**この間 materialComplete を呼ばない = mFetching 維持 = waiters は保持され成功時に正しく通知される**(即 complete(false) だと waiters が flush され後の成功を受け取れないため・これが置換の核)/ 非 transient or 上限 → materialComplete(false)(現行 terminal)。
- 追加 field: LLFetchedGLTFMaterial::mFetchFailCount(U8)。成功経路で 0 リセット。
- retry lambda = LLPointer<LLFetchedGLTFMaterial> capture(refcount で寿命安全)+ 再発行前に isLoaded/mFetching 検査。materialBegin は再呼びしない(llassert(!mFetching) 対策・begin は初回のみ)。
- 罠: teardown 後の遅延発火 → gAssetStorage null 検査。

## R3-JIT(Animation)

- **現行の真実(実読・重要訂正)**: markBad(llmotioncontroller.cpp)は `mMotionTable[id]=NULL` を格納するが、registry createMotion は `constructor==NULL → LLKeyframeMotion::create(id)` = **markBad は asset アニメに no-op**。実挙動 = 恒久放置ではなく**トリガー毎の backoff なし再フェッチ嵐**(motion 削除 → 次 trigger で再生成 → 再 fetch)。
- **設計**: file-scope 再試行台帳(llmotioncontroller.cpp・main thread 専有)`map<LLUUID, {U8 count; F64 next; bool terminal}>`・時刻 = LLFrameTimer::getTotalSeconds()。
  - LLMotionController::createMotion 冒頭 gate: terminal → NULL / now<next → NULL(既存の NULL 契約に乗る = 呼び手改修ゼロ)。
  - 失敗 2 site(createMotion 初期化 :379 / updateLoadingMotions :812): transient && count<8 → 台帳予約(markBad しない)/ else → markBad + terminal=true(**terminal の実効化 = markBad no-op バグの是正**)。
  - 成功 site(STATUS_SUCCESS 2 箇所)→ 台帳 erase。
- transient 判別: LLMotion に `virtual bool isFetchFailureTransient() const {return false;}` 追加・LLKeyframeMotion が override。onLoadComplete の status!=0 分岐で status != LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE なら transient=true / decode 失敗(deserialize/xran・onInitialize の file open 失敗)= false(content 恒久)。
- **置換申告**: トリガー毎再フェッチ嵐 = gate で消滅 / 「恒久 bad」は台帳 terminal で初めて実効。

## R4-JIT(Wearable)

- 現行(実読): processGetAssetReply(llwearablelist.cpp:186-213)= default 分岐で**即時同期 3 連発**再 getAssetData → 失敗 = 通知 + 恒久放置。
- **置換**: 即時再帰を `doAfterInterval(4<<(n-1))` の遅延再発行に・MAX_RETRIES 3→8。userdata(LLWearableArrivedData)所有は現行の「re-use instead of deleting」意味論を lambda が継続。NOT_IN_DATABASE = 即 terminal(現行・通知も現行維持)。
- 罠: 遅延発火時の gAssetStorage null 検査。

## R5-JIT(Server bake)

- R1 FSM が bake(FTT_SERVER_BAKE)を既に被覆。ただし R1 分類では 404 = 非 transient → bake の 404 は **bake upload→CDN 伝播レースで一時的が常態** → 分類 1 行修正: `FTT_SERVER_BAKE は 404/410 も transient`(backoff 再試行)。それ以外は R1 のまま。

## R6-JIT(stuck オラクル = 常時自己検査・fail-closed)

**不変条件の機械化**: 「全資産は LOADED / RETRY_SCHEDULED / FAILED_TERMINAL のどれかにある」— どれでもない資産(= 暴走 or 無期限放置)を **30 秒周期・常時 ON(PERF_LOG 非依存)** で数え、**>0 なら `WARNS AssetStuck`**(alarm チャネル = 憲法の default-deny が現地で自動捕捉)。0 なら無音。

### クラス別 stuck 判定(使用 API の意味論確認済み)
| クラス | stuck 判定 | 検査場所 |
|---|---|---|
| Texture | `failCount>0 && !missing && !fetching && 最近参照(<60s) && 期限超過 60s 以上`(`getTimeToExpireF32() < -60`・llframetimer.h:94 = mExpiry−sFrameTime で負 = 超過 ✓) | LLViewerTextureList::updateImages 内 30s sweep(mImageList へは member アクセス) |
| GLTF | `isFetching && failCount>0 && now > mNextRetryDue`(新 field・schedule 時に now+delay+60s 猶予で設定)= 予定時刻を過ぎても再 fetch が発火していない | `LLGLTFMaterialList::countStalledFetches()`(mList sweep・friend ✓) |
| Motion | 台帳 entry: `!terminal && count>0 && now > next_time+120s` = 満了後 2 分駆動されず(= instance 消滅等で駆動者不在) | `LLMotionController::countStalledAssetRetries()`(static・台帳 sweep) |
| Wearable | doAfterInterval pending 数(schedule++/発火--)が **>0 のまま 300s 継続** | 原子カウンタ + reporter 側の連続時間追跡 |

### 配線
- 共有カウンタ = llassetretry.h に C++17 inline `std::atomic`(tex_stuck / wear_pending)— .cpp 定義不要・両 lib から可視。
- 中央 reporter = llappviewer doFrame・30s LLTimer・常時 ON: 4 値集計 → stuck 合計 >0 で `WARNS AssetStuck tex=.. gltf=.. motion=.. wear=..` / 0 かつ PERF_LOG 時のみ INFO heartbeat。
- **これは新設 alarm 信号(意図した検出器)** = allowlist 掲載の要否は AYA 裁定(発火 = 実バグの名指しなので未登録のまま = 発火時ブロックが正、が設計者推奨)。

## 5. 縮小・省略・解釈申告

1. #8/#9(Sound/EEP)は未トレース = R7 で確定してから同契約適用(本表に「未トレース」と明記・黙って落とさない)。
2. backoff 定数(base 4s・8 回)は mesh の実証値に合わせた初期値。クラス毎調整は stuck/retry カウンタの実地データで(値調整は機構変更でない)。
3. missing-asset(DB 不在)の再武装は region 変更のみ(DB 不在は自然治癒しないため)。cache 汚染起因は cache purge で治る = 明示 refresh に含む。
4. 失敗注入足場は debug env 限定(通常起動でコスト・挙動変化ゼロ)。
5. インベントリ/ログイン系は本設計の対象外(独自 UI リトライを既に持つ)。

---

# 追補 A 系(2026-07-26・AYA GO「全部この漏れを設計して実装」): 回復契約の残穴 8 件

第 1 期(R1-R6)実装後の全クラス再監査で確定した残穴。全て §0 不変条件(LOADED / RETRY_SCHEDULED / FAILED_TERMINAL + 再武装 + stuck オラクル)への編入。真実源 = HEAD 実コード(file:line 全実読・トレース = session scratch `trace_mesh_permanent_invisible.md`)。

## A0. 共通病理(トレースで確定した不変条件違反)

**「失敗処理が『回復の予約』も『consumer への到達』も残さずに状態だけ倒す」**。具体形は 3 型:
- 型1 = 再要求経路の閉塞(mesh header の mPendingLOD leak 等)= 以後の要求が dead pending に合流し fetch 自体が出ない。
- 型2 = transient と terminal の混同(sound / skin)= 一時失敗を恒久フラグで封印。
- 型3 = 適用者(consumer callback)の先行死(bake)= データは回復するが適用者不在。

## A1. Mesh header 恒久閉塞 3 本の根治 + HTTP 失敗 requeue ladder(llmeshrepository.cpp)

- **現行の穴(実読)**:
  - B1: LLMeshHeaderHandler::processFailure :3804-3818 = mUnavailableQ×4 のみで **mPendingLOD 非掃除**(erase は headerReceived :2429 のみ)→ 以後の全再要求が loadMeshLOD :1379-1393 で dead pending に合流 = session 恒久 fetch 停止。
  - B2: run() header 枯渇 :1199-1206 = LL_DEBUGS のみで drop(unavailable 通知なし・掃除なし)。
  - B3: fetchMeshHeader :2066-2103 = constructUrl 空 URL(login/TP 直後 caps 未着)で**発行ゼロのまま retval=true = 成功扱い**。対照: LOD 側 :2287-2290 は unavailableQ 通知。
- **設計**:
  1. **terminal helper 新設** `headerRequestTerminal(mesh_params)`: mPendingLOD erase + mUnavailableQ×NUM_LODS push を原子対で実施。B1(processFailure の terminal 時)と B2(枯渇時)の両方をこれに置換。
  2. **HTTP 失敗の requeue ladder**: processFailure で即 terminal せず、thread 側台帳 `std::map<LLUUID,U32> mHeaderFailCount`(mMutex 下)を参照し count<DOWNLOAD_RETRY_LIMIT なら HeaderRequest を mHeaderReqQ へ再投入(RequestStats に `setRetries(U32)` を追加し 0.5s×2^n 遅延を継承・run() の isDelayed 機構がそのまま働く)。上限到達 or 非 transient で helper へ。headerReceived 成功時に台帳 erase。
  3. **B3**: 空 URL 時 `retval=false` に変更 = run() の canRetry 再投入に乗る(caps 到着を 0.5s×2^n で待つ)。枯渇時は 2. の terminal helper に落ちる。
  4. LOD/skin の HTTP processFailure(:3946 / skin 版)にも同型 ladder(mLODReqQ / mSkinRequests 再投入)。枯渇 = 現行どおり unavailableQ(通知は既存経路が健全)。
- **効果**: 一時的な CDN/caps 障害は ~64s の梯子内で自然回復。恒久障害は「通知される terminal」に必ず着地(黙殺の構造的排除)。

## A2. Mesh terminal 再武装(m404 / unavailable)= region 変更 generation + lazy poll

- **現行**: m404 header は mMeshHeader に恒久格納(:2388-2391)→ hasHeader true → 以後 unavailable 即答 :2294-2296。LLVOVolume 側の再要求 driver も存在しない(unavailable 後は setVolume 差分時のみ)。
- **設計**:
  1. repo に `static std::atomic<U32> sRearmGeneration` 新設。notifyLoadedMeshes の region 変更検出 :4791-4814(既存)で ++generation + thread へ「m404 header purge」を post(mMeshHeader から m404 entry を erase・mHeaderMutex 下)。
  2. LLVOVolume に `U32 mMeshRearmGen` 追加。**updateLOD 内 O(1) gate**: `gen 変化 && isSculpted mesh && volume && !isMeshAssetLoaded()` → gMeshRepo.loadMesh(this, params, mLOD) 再発行 + gen 更新。settled invisible 物体の再要求 driver をここで恒久確保(毎 frame コスト = int 比較 1 回)。
- **申告**: 明示 refresh(ユーザー操作)による再武装は本追補では未配線(region 変更のみ)= trigger 追加は将来の 1 行(generation++ を呼ぶだけ)。

## A3. SkinInfo 再武装(llvovolume.cpp / llmeshrepository.cpp)

- **現行の穴**: transient skin 失敗(HTTP 失敗 / run() 枯渇 :1110)→ notifySkinInfoUnavailable → `mSkinInfoUnavaliable=true` llvovolume.cpp:1494 = **session 恒久 unrigged**(再武装 = mesh ID 変更 :1343 / 成功 :1486 のみ・再要求 gate :1356)。
- **設計**: A1-4 の ladder で transient は repo 側で吸収。terminal 後の再武装 = A2 と同じ generation を LLVOVolume の updateLOD gate で参照: `gen 変化 && mSkinInfoUnavaliable` → `mSkinInfoUnavaliable=false` + `hasHeader && hasSkinInfo 見込みなら gMeshRepo.getSkinInfo(mesh_id, this)` 再発行。
- 「header に skin なし」(:1356-1364 の正当 terminal)は content 由来 = 再武装対象外(現行維持)。

## A4. Bake 適用の callback 死 免疫化(llvoavatar.cpp)

- **現行の穴**: terminal 時 doLoadedCallbacks llviewertexture.cpp:2671-2693 = callback 全消去(+ gTextureList.mCallbackList erase)/ 900s 無活動 reaper :2653-2666。R1 の region 再武装は mIsMissingAsset を倒すだけで**適用者は戻らない** → fetch 成功・VK image 生成でも avatar 灰色のまま。
- **設計(型3 の一般解 = 適用を event から poll へ)**: updateMeshTextures :9678-9700 が既に持つ「discard≥0 なら直接 useBakedTexture / 未着なら callback 登録」の前段判定を、**LLVOAvatar の周期処理に写す**: 各 baked slot について `!mBakedTextureDatas[i].mIsLoaded && baked_img && baked_img->getDiscardLevel() >= 0` → `useBakedTexture(id)` 直呼び。周期 = 2s LLTimer(per avatar・slot 数 ≤ 8 の指標比較のみ = 実質ゼロコスト)。callback 経路は現行のまま(高速経路)・poll は救済網。
- **効果**: callback がいかなる経路で死んでも、データ到達 = 適用、が機械保証される(不変条件「回復は consumer 到達と原子対」の充足)。

## A5. Sound(llaudio/llaudioengine.cpp)= transient/terminal 分離 + backoff

- **現行の穴**: assetCallback 失敗 :1252-1261 = transient でも `setHasDecodeFailed(true)`(コメント「avoid constant rerequests」= 嵐回避のための恒久封印)→ 選定 gate :1201/:1216 で session 恒久無音。
- **設計**: LLAudioData に `U8 mFetchFailCount` / `F64 mFetchRetryDue` 追加 + `isFetchRetryBackoff()`(now < due)。
  - assetCallback 失敗分岐: transient(result_code が NOT_IN_DATABASE / FILE_EMPTY 以外)&& count<ASSET_RETRY_LIMIT → count++・due = now + assetRetryDelaySec(count)(**decode-failed は立てない**)/ 非 transient or 上限 → 現行どおり setHasDecodeFailed(true) = terminal。
  - 選定ループ 2 箇所(:1201/:1216)の条件に `&& !adp->isFetchRetryBackoff()` を追加 = backoff 中はスキップ・満了後に自然再要求(既存の選定ポンプが driver なので新規駆動不要)。
  - 成功時 count=0。再武装 = なし(~17 分梯子で十分・音は region 依存性が低い)= 申告。
- llaudio → llcommon 依存は既存(llassetretry.h include 可)。

## A6. Mesh physics / decomposition(llmeshrepository.cpp)= 失敗時の loading-set 解放

- **現行の穴**: mLoadingPhysicsShapes/mLoadingDecompositions は要求時 insert :5116/:5145・**成功時のみ erase** :4961/:4965。HTTP 失敗 = 「simply leave unfulfilled」×4(:4188/:4244/:4262/:4317)→ entry 恒久残留 = **同 mesh の再要求が永久 block**。
- **設計**: thread 側に `std::deque<LLUUID> mPhysicsFailQ / mDecompFailQ`(mLoadedMutex 下)。各 processFailure でここへ push → notifyLoadedMeshes で mLoading* から erase = 次回要求(ユーザー操作駆動の機能)が素通りする。ladder なし(要求自体が明示操作で再発行されるため)= 申告。

## A7. EEP(llsettingsvo.cpp)= R2 準拠の遅延再発行

- **現行の穴**: onAssetDownloadComplete :318-350 = 失敗 → WARN + callback(null) で終了・retry なし(default 環境のまま)。
- **設計**: file-scope 台帳 `map<LLUUID,U8> sSettingsFetchFail`。transient(status != NOT_IN_DATABASE 系)&& count<8 → `doAfterInterval(assetRetryDelaySec(n))` で getSettingsAsset 再発行(callback capture 継続 = waiters 保存)/ 非 transient or 上限 → 現行の callback(null)。成功で台帳 erase。gAssetStorage null 検査(R2 と同じ罠)。

## A8. Legacy Blinn-Phong material(llmaterialmgr.cpp)= 回収 sweep

- **現行の穴(トレース確定)**: onGetResponse 失敗 :418-423 = WARN のみ。POST 済み id は mGetQueue から除去済み・mGetPending は MATERIALS_POST_TIMEOUT で期限切れ・**mGetCallbacks は残る**が誰も再 queue しない = 新規要求者(同 material の別 object)が現れない限り恒久 flat。
- **設計**: 既存 onIdle ポンプに**回収 sweep**を追加(30s 周期): mGetCallbacks の各 id について `!mMaterials 所持 && !isGetPending && !mGetQueue 所属` → 迷子 = per-id 台帳(count<8・backoff 満了)で mGetQueue へ再投入 / 上限 → callback 破棄 + WARN(terminal)。既存ポンプが driver なので新規駆動不要。

## A-gate(正のオラクル・幸運ログ禁止)

1. **注入拡張** `AYASTORM_ASSET_FAIL_INJECT=N`(既存 env に相乗り・新規 3 注入点):
   - mesh header: LLMeshHandlerBase::onCompleted で per-mesh 最初の N 回を強制 processFailure 化 → N=2 で「失敗 → ladder → 実成功 → notifyMeshLoaded → 描画復帰」を機械証明(B1 が治っていなければ 2 回目の fetch 自体が出ない = fail-closed に検出される)。
   - sound: assetCallback で同型(最初の N 回失敗化)→ 回復 = 再生到達。
   - EEP/material: 母集団小のため注入なし・実 403/フィールドで gate(申告)。
2. **stuck オラクル拡張(R6 の行に増員)**: mesh = `mPendingLOD 内 entry で対応 HeaderRequest が mHeaderReqQ にも mHttpRequestSet にも台帳にも無いもの`(= 孤児)を 30s 周期で数え >0 → WARNS AssetStuck に mesh= 欄追加 / sound = backoff 満了後 120s 超の未再要求。
3. 既存 T1(非退行)/ T3(terminal 梯子)は A 系込みで再走。
4. **視覚 gate(AYA)**: TP 直後に建物欠落 → 数十秒〜数分で自然回復すること(従来 = 恒久欠落)。

## A-申告欄(縮小・省略・解釈)

1. mesh の ladder base は既存 0.5s(house 標準)を継承 = 他クラスの 4s と非統一(第 1 期からの既知非統一の踏襲)。
2. A2 の明示 refresh トリガー未配線(region 変更のみ)。
3. A5 sound の再武装なし(梯子のみ)。
4. A6 は ladder なし(明示操作駆動のため解放のみ)。
5. A7/A8 は注入 gate なし(母集団小・フィールド gate)。
6. legacy material の terminal は「callback 破棄 + WARN」= fallback 表現は現状(flat)のまま(product 可視の変化なし)。
7. jellydoll(AOA_JELLYDOLL)経路の bake 適用は対象外(既存挙動維持)。

## A-申告欄 追記(実装後検証 2026-07-26)
8. A8 の遅延再 queue は失敗時 region 固定。TP で当該 region を離れた後は processGetQueue が不在 region queue を破棄(llmaterialmgr.cpp Unknown region 分岐)= 空振り。要求者(object)も同時に消えるケースとほぼ一致するため許容(現状より悪化しない)。
9. A2 rearm の再要求 lod は llclamp(mLOD, 0, NUM_LODS-1)(NO_LOD=-1 だと loadMesh が即 return で空振りするため)。
10. A4 bake poll は texture 適用のみ救済(morph mask = onBakedTextureMasksLoaded 経路は非救済)。
11. sound の stuck 集計(countStalledSoundFetches)は demand 消滅で偽陽性になり得るため WARNS 判定に含めず heartbeat 表示のみ。

## A1 改訂(AYA 指示 2026-07-26): mesh 一時失敗の試行窓 = 10 分
- 「物量の多い場所では 2 分の梯子では足りない」(AYA)→ mesh header/LOD/skin の retry を回数上限制から**時間窓制**へ: 初回失敗から `MESH_RETRY_WINDOW_SEC = 600s` の間は再試行を続ける。遅延 = 0.5s×2^n・上限 64s(MESH_RETRY_MAX_SHIFT=7)→ 後半は 64s 周期 = 窓内で計 14-15 回程度。窓超過 or 404/410 で terminal(以後は region 変更 rearm)。
- 実装 = LLMeshRepoThread::MeshRetryState{mCount, mFirstFail} に置換(schedule 3 関数 + headerReceived/注入の読者更新)。他クラス(texture 等 base 4s×8 = 累計 ~17 分)は変更なし。
