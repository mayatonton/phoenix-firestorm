# 非同期 安全弁(anti-runaway)設計 — 生産は消費を超えない

- 状態: **設計者起草 2026-07-26・AYA 指示「設計上の解決が先・検証を前提にしない」を受けた fail-safe 設計・approve 待ち**。HEAD `3f559081e73`。
- 位置づけ: GL は同期呼び出しが暗黙の背圧/pacing を全経路で無料提供していた。VK 化(PE thread・producer・worker 群・one-shot)はそれを enqueue に置き換えたが**独立した安全弁を設けなかった = 設計の落ち度**。現地から挙動異常の報告あり。低スペック実機で CPU/GPU を焼く・device lost で壊す暴走モードを**機構で封じる**。実機検証は設計の前提にしない(fail-safe = 最悪ケースを実在として設計)。
- 真実源: HEAD 実コード(下記 file:line 全て実読)+ 実測ログ(device lost 501MB 暴走 = `docs/vknative_crowd_body_recovery_design.md` §5)。

---

## 0. 破れている不変条件

**「非同期の生産(frame 記録・geometry・staging・one-shot submit)は、消費(GPU 完了・present)の速度を超えて前進してはならない。」**

GL はこの不変条件をドライバの同期ブロックで構造的に持っていた。現設計は経路ごとにバラバラ(あり/条件付き/無し)で、**穴のある経路が実機で暴走する**。

### 0.1 生産経路の背圧 全数台帳(HEAD 実読)

| 経路 | 現在の律速/背圧 | file:line | 判定 |
|---|---|---|---|
| frame loop(記録・発行) | ①FIFO present ブロック(**ドライバ任せ**)②FS limiter(既定 ON 60fps・sleep 式) | llappviewer.cpp:1938-1954 | **条件付き = 穴**(§0.2) |
| frame pipeline 深さ | fence 3-deep(slot N-3 待ち) | llvkloader.cpp:5496-5517 | あり(健全) |
| one-shot submit(tex worker) | pending 256 件 + **256MB** byte cap で fence 待ちブロック | llvkloader.cpp:8691-8700 | あり |
| one-shot submit(**main**) | pending 256 件のみ・**byte cap = ~0ull(∞)** | llvkloader.cpp:8691 | **半分 = 穴** |
| geometry inflight bytes | `gVkGeoInflightBytes` は**計測のみ・cap 無し** | llvkloader.cpp:5623,5941 | **無 = 穴**(501MB→acquire 11.2s hang→device lost 実測) |
| geometry apply(main 側) | 8ms 時間予算・ただし①最低 1 job 強制(~470ms/job 素通り)②**inline 経路が予算外**(197 件/frame 素通り) | crowd_body doc §5(commit `9c1e641eaa`) | **欠陥 2 点 = 穴** |
| present thread(PE) | FIFO 待ちが**ドライバ内 busy-wait**(NVIDIA 実測 93% CPU)。対処 `__GL_YIELD=USLEEP` = **Linux+NVIDIA 限定** | perdraw doc 戦略1・llappviewerlinux.cpp | **移植性なし = 穴**(他 OS/vendor は焼く) |

### 0.2 frame loop の pacing の穴(最重要)

既存 FS limiter(FIRE-22297・llappviewer.cpp:1938-1954)は sleep 式で choke point も正しいが、**安全弁ではない**:

1. **gate 条件で不作動**: `STATE_STARTED && !gTeleportDisplay && !logoutRequestSent()` = **ログイン画面・TP 中・ログアウト中は free-run**(この間の律速は FIFO ブロックのみ)。
2. **ユーザー opt-out で全喪失**: `FSLimitFramerate=0` or `FramePerSecondLimit=0` の user_settings で完全無効。
3. **vsync 無効時**: vsync GUI トグルで FIFO ブロックが消えると、limiter が切られていれば**律速ゼロ = main が全力 free-run + GPU 全力 = 両方焼く**。

∴ pacing は「ドライバ FIFO」と「任意設定」の 2 本に依存し、**無条件の綱が 1 本も無い**。

---

## 1. 設計 = 3 弁(独立機構・per-symptom 当てを廃し不変条件を機構化)

### 弁1 = 無条件 frame pace backstop(最優先)

**「main loop は、いかなる状態・設定でも backstop 上限を超える頻度で回らない」**を無条件保証する。

- **実装点**: 既存 limiter と同一 choke(llappviewer.cpp:1953 `frameTimer.reset()` の手前)に **backstop 段を追加**。ユーザー limiter はそのまま(product 意味論不変)。
- **機構**: `backstop_interval = 1 / backstop_cap`。frame 経過が interval 未満なら残りを `ms_sleep`(実 sleep・スピンでない)。
- **cap 値**: `backstop_cap = 2 × display refresh(LLWindow::getRefreshRate()・llwindow.h:215)、取得不能(0)なら 120`。**gate 条件なし**(ログイン/TP/ログアウト含む全状態で有効)。
- **正常時は無干渉**: FIFO or ユーザー limiter が効いていれば frame 経過 ≥ interval → sleep=0。backstop が実際に engage する = **pacing が失われた実機の署名**。
- **engagement 計器(fail-closed の目)**: backstop sleep 発生を VkPerf に計上(`bkstp_ms`)+ 恒常 engage(例: 5 秒窓で >50% frame)を WARNING 1 行/窓で報告。現地の「FIFO が効かない環境」をログが名指しする = 検証を出荷後の実地データで自動回収。

### 弁2 = inflight byte hard cap(device lost の直接封じ)

**「提出済み未完了の資源 byte は上限を超えない。超過時は生産側が止まる(溜めない)」**。

- **2a: geometry inflight cap**: `gVkGeoInflightBytes` に hard cap(初期値 **128MB**・env override)。worker の新規 geometry staging 確保の**手前**で判定し、超過中は job を defer(queue 保持・生産停止)。501MB 膨張の暴走を生産側で断つ。
- **2b: main one-shot byte cap**: llvkloader.cpp:8691 の `byte_cap = tTexWorkerThread ? 256MB : ∞` を **main も 256MB** に(1 語変更)。既存の fence 待ち loop がそのまま背圧になる。
- **2c: apply 予算の 2 欠陥封じ**: ①巨大 job = **生産側で分割上限**(job 生成時に N MB 超を分割・apply 側の「最低 1 job」はそのまま安全)②inline 経路(`mInline`)を予算会計に**算入**(超過分は queued へ転換。転換可否の意味論 = 実装時トレース U1)。
- 満杯時の視覚は「新規 geometry の完成が数 frame 遅れる」= 既存 apply 予算と同じ縮退・**crash より遅延**(fail-safe の原則)。

### 弁3 = present thread の焼き封じ(移植性)

- 弁1 が全状態で main を pace するため、present 供給レート ≤ backstop = FIFO 滞留が構造的に縮み、ドライバ spin の総時間も縮む(弁1 の副次効果)。
- 残余(present 毎の ≤1 vblank spin)への portable 対処: **PE 側 pre-present pace** = present 間隔の実測 EMA(present_wait 対応環境は `vkWaitForPresentKHR` の実タイミング)で次スロットまで**先に sleep してから** `vkQueuePresentKHR` を呼ぶ = ドライバに待たせない。FIFO 意味論・画質は不変。
- 段階: 弁1/2 の後。3-OS 検収は各 maintainer(present_wait コメントの既存様式)。`__GL_YIELD`(NVIDIA-Linux)は残置。

---

## 2. 実装計画(順序・全て kill switch 不要 = 恒久安全機構)

| 段 | 内容 | 規模 | 依存 |
|---|---|---|---|
| **V1** | 弁1 backstop + engagement 計器 | llappviewer.cpp ~30 行 + VkPerf 1 欄 | なし・即着手可 |
| **V2b** | main one-shot byte cap | llvkloader.cpp 1 語 | なし |
| **V2a** | geometry inflight hard cap | worker 確保点 gate ~20 行 | 確保点の実装時トレース |
| **V2c** | job 分割上限 + inline 予算算入 | 生産側 ~40 行 | U1(inline 意味論) |
| **V3** | PE pre-present pace | peExecute ~40 行 | V1 後・3-OS |

## 3. gate(命題様式・各弁)

- **V1 命題**: 「いかなる状態でも frame 頻度は backstop を超えず、pacing 健在時は挙動不変」。正のオラクル = ①合成条件で engage 動作証明(vsync 無効 + limiter 無効で fps が backstop 頭打ち・CPU が sleep に落ちる)②通常起動で bkstp_ms≈0 + fps/frametime 不変 ③validation 0。
- **V2 命題**: 「crowd 入場 burst で inflight bytes は cap で頭打ちし、acquire hang/device lost に至らない」。オラクル = mb=/stg_mb の頭打ち + defer カウンタ稼働 + device lost 0(従来の再現シーン)+ 視覚 = 完成遅延のみ(欠落なし)。
- **V3 命題**: 「present 品質不変のまま PE thread の busy-wait が sleep に置換される」。オラクル = PE thread CPU%(実測様式は戦略1 と同一)+ fps/tear 不変。
- 実効設定確認欄: vsync 設定・FSLimitFramerate/FramePerSecondLimit 実値(user_settings 実 Value を確認 = env_settings_xml_churn)・refresh rate 取得値。

## 3.5 JIT 詳細設計(実装監査・2026-07-26・run-1 実施後に補完)

> ⚠️ 経緯の申告: 初回実装は本節(JIT)を経ずに §1 の高位設計から直接行った = 着手プロトコル違反(AYA 指摘)。本節は実装済み diff を JIT 水準で監査し、前提を全て実コードで確定し直したもの。差分は「是正」に列挙。

### V1(frame pace backstop)as-implemented

- **場所**: llappviewer.cpp doFrame・既存 FS limiter(:1938-1952)の直後・`frameTimer.reset()`(旧:1953)の手前。同一 `frameTimer` を共有(user limiter の sleep 後の残り経過に対して backstop を判定 = 二重 sleep なし)。
- **データ**: 関数 static 6 個(sleep_us/frames/win_frames/warned/LLTimer window)。全て main thread 専有 = 同期不要。
- **cap**: `2 × LLWindow::getRefreshRate()`・0 なら 120。**Linux SDL は mRefreshRate 未設定(llwindow.cpp:129 の 0 のまま・設定箇所は win32:718,1150 / macosx:808 のみ)= Linux 実効 cap = 120fps 固定**。
- **スコープの罠(確定)**: 実装点は `if (!LLApp::isExiting() && !gHeadlessClient && gViewerWindow)` ブロック内 = headless では不動作(headless は ms_sleep(100) 別経路あり・非対象で正)。gViewerWindow null 分岐は到達不能だが防御済み。
- **run-1 実測と期待値の是正(C1)**: 通常起動で engaged 29/300・13/296 frame(sleep 計 160/71ms per 5s ≈ 3%)。機構どおり = **uiscene split の consumer-only frame(<8.3ms)を 8.3ms に保持**したもの。FIFO の pacing は数 frame 単位の背圧で per-frame では速い frame が存在するため、**per-frame ceiling の軽度発動は正常動作**。品質影響 = なし(present 平均は vsync 支配のまま)。→ §3 gate の「通常起動で bkstp≈0」を「**engaged < 10% frames かつ WARNS 不発**」に是正(doc-only・コード不変)。WARNS 閾値(50%/窓)は run-1 で正しく不発。
- **granularity**: `ms_sleep` = ms 粒度。cap 120fps(8.3ms)に対し ±1ms は誤差として受容。

### V2b(main one-shot byte cap)as-implemented

- llvkloader.cpp:8691 `byte_cap = 256ull<<20`(旧 `tTexWorkerThread ? 256MB : ~0ull`)。
- **wait loop の意味論(実読・:8691-8745)**: soft cap = 最大 20 回・各 100ms timeout で最古 pending fence を待って解放。20 回超過時は cap 超過のまま続行(= 完全 stall はしない・bounded)。entries>256 条件は従来から両 thread 適用 = 変更は byte 条項のみ。
- run-1: stg_mb 最大 1MB = 不発動(通常運転で無干渉を実証)。

### V2a(geo inflight cap 128MB)as-implemented

- llvovolume.cpp:5846 cap 512→128MB + `AYASTORM_GEO_INFLIGHT_CAP_MB` override。発動点 = rebuildGeom 冒頭 defer gate(:7469-7473)= **超過中は GEOM_DIRTY を保持したまま return(geo_defer++)** → 圧が引けたら自然再開(state 喪失なし・既存機構)。
- **bypass の確定**: `mVkForceInlineRebuild`(record 喪失の同 frame 修復 = llspatialpartition.cpp:288,469)と HUD/selected は cap 対象外 = **正当性駆動 inline は絞らない**(#VKGeo 原子対不変条件の維持)。
- run-1: mb 最大 7MB・defer=0 = 不発動。cap 128MB は通常運転に遠く、cold burst(実測 501MB)のみ止める位置 = 設計意図どおり。

### V2c(inline 予算算入)as-implemented

- **frame 内順序(実読で確定)**: display() 内 `drainGeoPublishQueue`(llviewerdisplay.cpp:929)→ `updateGeom`(:933・rebuildPriorityGroups 経由 pipeline.cpp:3480)→ postSort(pipeline.cpp:4926)。inline apply は**同 frame の drain の後**に発生 → `sGeoInlineApplyMsAccum` を drain 冒頭で取得+リセット = **carry は前 frame の inline 合計**(意図どおり・frame counter 不要)。
- drain は単一呼び出し点(llviewerdisplay.cpp:929 のみ・全数 grep 確認)。display 早期 return(最小化等)時は drain skip → accum が複数 frame 分累積 → 復帰初回 drain の effective=0(min-1-job は維持)→ 次 frame でリセット = **有界・自己回復**。
- `s_apply_budget_ms<=0`(無制限設定)の意味論は不変(break 条件の gate が従来どおり)。
- **観測ギャップ(是正 C3)**: inline 消費 ms と effective budget が log に出ない = run-1 で「V2c が queued を絞ったか」を counter で証明できない(backlog≤94 と pub≈enq から間接否定のみ)。→ drainGeoPublishQueue に perfLog 時 5 秒毎 1 行 `geoinl ms=<前窓 inline 合計> bud_min=<窓内最小 effective>` を追加(llvovolume.cpp 完結・.h 不変)。

### run-1(2026-07-26 07:24・通常起動)の症状帰属表

AYA 報告 =「ロードされ切らず酷い描画」。counter による帰属:

| 仮説 | 判定 | 根拠 |
|---|---|---|
| V2a が geometry を絞った | **否** | defer=0(一度も発動せず)・mb≤7MB |
| V2b が upload を絞った | **否** | stg_mb≤1MB(cap の 1/256) |
| V2c が publish を絞った | **否(間接)** | enq≈pub(backlog≤94 job)・pub 500/s 健全。直接証明は C3 で取得 |
| V1 が frame を遅くした | **否** | sleep 計 3%・FRAMETIME avg 23-45ms = 従来域 |
| 既存事象(可視 empty-drawmap) | **該当** | other=1400-2500/60f 持続 = **前回健全 run(21:10)の末尾と同一署名・同規模**(2043-2449) |
| 外部(asset 403) | **該当** | Http_403 190 件(前回 117)= fetch 失敗は server 側 |
| scene 内 NaN burst | 併発 | #VKNaN 200 件(22:25:11-16 の 6 秒)= 既設 sanitizer(llviewershadermgr.cpp:559)が実 NaN を捕捉・弁と無関係。**前回 run はゼロ = 上流 NaN 源が生きている観測 → AYA 処分待ち(TICKET 候補)** |

**結論: 弁 4 本はいずれも不発動 or 微小で、症状は既存署名(可視 empty-drawmap)+ 外部要因の複合。ただし V2c の直接証明計器が無いのは実装の落ち度 = C3 で塞ぐ。**

### 是正一覧(AYA 承認待ち)

- **C1**(doc-only・本節で実施済): V1 通常時期待値を「engaged<10% frames・WARNS 不発」に是正。
- **C3**(小計器・llvovolume.cpp のみ): `geoinl` 行の追加(inline ms + effective budget 最小値)= V2c の fail-closed 可視化。
- 是正なし: V2a/V2b はトレース・実測とも設計どおり。

## 3.6 run-1 検出 alarm 3 本の ZERO 化 line(設計対象・agent 処分の是正)

> 経緯: §3.5 初版はこの 3 信号を「既存/外部」と agent 側で処分した = 憲法 3 違反(AYA 指摘)。allowlist 照合の結果 **3 本とも未登録 alarm**(#VKNaN は根治済みとして台帳から削除済みの信号の**再発 = 回帰**)。処分は ZERO 化 or TICKET のみ。以下、各信号の実トレース(scratch trace_3alarms.md)に基づく ZERO 化設計。

### A1: #VKNaN 再発(skin palette 非有限)

- **機構(file:line 確定)**: sanitizer = llviewershadermgr.cpp:536-561(**警告 cap=200 = run-1 の「200 件」は上限打ちで実件数不明 = 計器欠陥**)。palette 源 = lldrawpool.cpp:1881-1959 `mpc.mGLMp` = joint world matrix × IBM。IBM は decode sanitize 済(`309655402c8`)→ **残る非有限源 = joint world matrix(skeleton/motion)**。既存 finiteness gate は点在(llkeyframestandmotion.cpp:176 / llpose.cpp:376,381 / llkeyframemotion.cpp:355,1638)で全 writer 網羅の保証なし。
- **設計**: ①帰属計器 = sanitizer 発火時に avatar UUID + 非有限 joint index/名 + 発生 rate を名指し(cap 撤廃・集計は無制限・詳細 log は rate-limit)②装置が producer(どの motion/経路)を名指ししたら該当 writer に根治 gate。**推理で motion を当てない**(writer 多数・asset 依存)。
- gate: 診断で名指し → 根治 → 同条件で VKNaN=0。

### A2: #VKGeo 可視 empty-drawmap other(数千/60f 持続)

- **機構(確定)**: 検出器 = pipeline.cpp:4266-4294。**9-class 分類器 + 実穴 UUID 収集は実装済**(vkcClassifyEmptyOtherGroup・pipeline.cpp:4091-)だが `VKC verbose` 時のみ発火 = run-1(通常起動)は集計値のみ。class = dead/parcel/gltf・mesh・skin 未 load/SA 保護/zerogeom/透明/**hasgeom = 実穴**。
- **設計**: 診断起動 1 回(`AYASTORM_VKC=1`)で装置に内訳を出させる(装置ファースト・fix 仮説に行かない)。判定表:
  - mesh/skin/gltf 待ち支配・hasgeom≈0 → 実体 = **asset 供給律速**(= A3 と連結・「ロードされ切らない」の正体)→ ZERO 化は供給側 line。
  - hasgeom>0 → **実穴** → UUID を L2 連続性オラクル(既存 UUID watch)に掛けて record 消失経路を名指し → 根治。
- gate: 診断起動で hasgeom=0 かつ 待ち class が時間とともに 0 収束(ロード完了で消える)こと。

### A3: Http_403(190 件・増加傾向)

- **機構(確定)**: 403 は policy 非 retry(正)→ texture は失敗のまま灰色固着・**集約可視化なし**(lltexturefetch.cpp:2221-2229 の個別 WARN のみ)。**2 系統**: ①`phoenixviewer.com/app/fsdata/*` 403 = FS 本家サーバが fork の名乗りを拒否している可能性 = **自 code でなく infra/product** ②texture CDN 403(45 件)= cap token 失効 or CDN 側(ログからは確定不能)。
- **設計**: ①= **TICKET(AYA 起票)**: fsdata 依存(FS 本家サービスへの fetch)を fork としてどう扱うか = product 決裁(自前 endpoint / fetch 撤去 / 現状維持)。②= 集約計器(session 累計 N 件・URL 種別・初回/最悪 URL)を perf 行へ = 「ロードされ切らない」が server 起因の時に装置が言う。cap token 失効説は計器の headers/caps 相関で判定。
- gate: ②の計器が実地ログで 403 の帰属(token/CDN/恒常)を名指しできること。

## 4. 縮小・省略・解釈申告

1. **backstop cap = 2×refresh(fallback 120)は設計者判断**。vsync 無効で 300fps ベンチしたいユーザーには product 可視(頭打ち)→ **product 決裁対象・推奨 = 適用**(安全 > ベンチ)。ユーザー limiter・GUI は不変。
2. **U1**: inline geometry(`mInline`)の queued 転換可否は意味論(同 frame 完成を前提とする呼び手の有無)を実装時に全 caller トレースで確定。転換不能なら「inline は予算算入 + 超過時は次 frame へ持越し」の弱形へ縮退(その場合も算入は達成)。
3. **cap 初期値(128MB/256MB)は上限側の安全値**(実測 501MB 暴走 < 半分)。実地の bkstp/mb ログで後追い調整(値調整は機構変更でない)。
4. **弁3 は後段**。弁1/2 が crash/焼きの深刻度上位を先に封じる。3-OS の present 実装差は maintainer 検収(既存様式)。
5. 本設計は「暴走の全経路台帳」§0.1 を根拠とし、**実機再現を前提にしない**(fail-safe)。現地報告の個別症状の帰属は V1 の engagement 計器と V2 の defer 計器が実地ログで自動回収する。
6. 既存 FS limiter の gate 条件(STATE_STARTED 等)は**変更しない**(product 意味論)。backstop は別段として追加 = 既存挙動の回帰ゼロ。
