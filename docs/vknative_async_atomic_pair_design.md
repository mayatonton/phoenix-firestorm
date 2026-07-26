# 非同期原子対 根治設計(async atomic-pair root cure)

- 状態: 設計者起草 2026-07-26(AYA GO)。背景 = 建物消失の真因究明から一般化した族の殲滅工事。
- 対象の病理(1 つ): **「破壊的 apply(record purge・状態上書き・要求フラグ消費)が、『新状態の適用』か『再予約』との原子対になっていない」**。非同期化(stage〜apply の時間窓)でこの欠陥が「窓の間に世界が動いた entity の恒久喪失」として顕在化する。
- 全境界の監査台帳 = session scratch `audit_async_boundaries.md`(判定根拠 file:line 全掲)。手本 = bake worker(mUploadGen 世代ガード + poll 収束・llviewertexlayer.cpp:1268-1276)。
- 真実源 = HEAD 実コード。合否 = 既存 fail-closed counter(幸運ログ不要)。

## 0. 不変条件(本設計の核・全境界共通)

**破壊的 apply は、同一 choke 内で次のいずれかと対にならなければ実行してはならない:**
(a) 対応する新状態の適用 (b) 当該 entity への再予約(rebuild / 再要求)(c) 明示 terminal 宣言。
対の成立を機械検査する counter(unpaired 系)は常設 alarm とし、非ゼロ = 未承認 alarm(憲法 default-deny)。

## P1. choke 強制(確定穴 2 件の閉塞)

### P1-a. applyGeoStaged unpaired evict(建物消失の真因)
- 現行: llvovolume.cpp:6263 `clearDrawMapStaged(preserve, staged_drawables, SITE_CLEAR_APPLY)` — preserve は **stage 時点**の集合。stage〜apply 窓で動いた drawable(group 移籍・LOD 切替・te 変化)は旧 record を purge されるが新 record も再予約も受けない。実測 = `map_evict_unpaired=49/10s`(llvkcontract.cpp:913-940)・傍証 `list_drop_infrustum=79/10s`。
- **fix**: clearDrawMapStaged(llspatialpartition.cpp:175-183)を「purge で消える record の所有 drawable のうち staged_drawables に居ない生存個体」を収集して返す形に拡張し、applyGeoStaged がその場で `markRebuild(drawablep, REBUILD_GEOMETRY)` + group GEOM_DIRTY + `gPipeline.markRebuild(group)` を張る。rigged face は対象外(group geometry を使わない)。
- 適用範囲: geo publish(llvovolume.cpp:6976)と avatar publish(:7061)は同 choke 共有 = 1 箇所で両閉塞。
- gate: `map_evict_unpaired` → 0(診断 run)。

### P1-b. T1 texture publish 失敗の要求消費
- 現行: drainTexPublishQueue の `!entry.mJob.mOk` 分岐(llviewertexturelist.cpp:1505-1511)が postCreateTexture を呼び mNeedsCreateTexture を消費 → **VK upload の一時失敗(メモリ圧等)で texture 恒久不生成**。
- **fix**: mOk=false は要求を消費しない — mNeedsCreateTexture を維持したまま `mCreatePending=false` のみ(= 次の updateImages 周回で自然に再 enqueue)。連続失敗の嵐は既存 R1 流儀の per-texture backoff(mFetchFailCount 系 field を流用・上限で従来 terminal)。
- gate: `tex fail=` 増加時にも当該 texture が最終的に生成される(注入 or メモリ圧試験)。

## P2. 収束ループ(reconciler・「詰まらないように最初から作る」)

- push 通知連鎖(どの境界でも落ち得る)から正しさを切り離す: **消費側の巡回 tick が「不変条件を満たさない個体」を毎周回復**する。
- 実装: 全 LLVOVolume を 500 個/frame のリング巡回(12K 個 ≈ 1 秒/周・判定は整数比較のみ)。各個体:
  1. mesh 型 & 未ロード & 非 terminal & 非要求中 → `loadMesh`(冪等・既存 dedup + 10 分窓 ladder 内)
  2. ロード済 & 可視 & 全 face geom 0 & **非 rigged** → markRebuild + group dirty(kick の正規化・rate = 巡回周期が天然の制御)
  3. texture: mNeedsCreateTexture & 非 pending → 再 enqueue(P1-b の駆動保証)
- 修復数を per-class 累計し oracle 行に出す。**修復数が恒常非ゼロ = どこかの push が壊れている**の常設検出(修復自体は成立するので product は守られる)。
- 既存 rearm generation gate(updateLOD)はこの巡回に吸収(generation 特例が不要になる)。

## P3. オラクル常設昇格(「鳴っていたのに誰も見ない」の根絶)

- `map_evict_unpaired` / `list_drop_infrustum` を VKC 診断専用から**常設軽量集計**へ(通常起動でも counter は増える設計を確認の上、30s 周期の AssetStuck 行に `evict_unpaired=` `list_drop=` を追加・非ゼロ = WARNS)。
- `tex_fail` / reconciler 修復数も同行に。allowlist 登録はしない(発火 = 実バグの名指し)= AYA 裁定対象。

## 実装順・gate

1. P1-a(choke)→ 診断 run: `map_evict_unpaired=0` + TP 先で建物残存(視覚 gate は AYA)。
2. P1-b + P2 → 通常 run 非退行 + oracle 修復数の観測。
3. P3 → 常設行の稼働確認。
- 撤去同梱: 診断 kick(pipeline.cpp)撤去・分類器 rigged 除外(実装済み未ビルド分)。

## 申告欄(縮小・省略・解釈)

1. P2 の巡回対象は LLVOVolume + texture create のみ(音/motion/wearable は本日の A 系 ladder が既に収束駆動を持つ・bake は世代ガード済みで対象外)。
2. P1-b の backoff 上限後 terminal は現行意味論(missing 扱い)に着地(新規の可視変化なし)。
3. uiscene / audio decode 境界は低優先で本工事の対象外(台帳に理由記載)。
4. record workers / one-shot / PE / motion / bake = 監査の結果 白(台帳参照)= 触らない。

## 1-run 検証プロトコル(AYA 指示: 視覚でなくログ・全実装を 1 回で)

起動: `AYASTORM_VKC=1 AYASTORM_PERF_LOG=5 ~/ayastorm/ayastorm` → 物密集 TP 先で 3 分前後(TP 1 回以上を含む)→ 終了。判定は全て機械(私がログで裁定):

| # | 項目 | 見る行 | PASS 条件 |
|---|---|---|---|
| 1 | P1-a 原子対 | VKC-SUM `cause{map_evict_unpaired=}` | **0**(改修前 49/10s) |
| 2 | P1-a 傍証 | VKC-SUM `cause{list_drop_infrustum=}` | 0 近傍へ激減(改修前 79/10s・数件の過渡は orph ペアの 1-frame 遅延として許容 = 残れば個別裁定) |
| 3 | P1-a 稼働 | VkPerf geo `orph=` | 非ゼロ = ペアリングが実際に発火(churn を治している証拠)・単調増加が定常率に収束 |
| 4 | P2 稼働/破れ検出 | AssetStuck `georepair=` | **定常 0**(非ゼロ = P1 で拾えない push 破れが残存 → WARNS で名指しされる) |
| 5 | P2 mesh 駆動 | AssetStuck `meshkick=` | ロード中は増加 → シーン安定後に増分ゼロへ収束(収束しない = mesh パイプ詰まり残存) |
| 6 | P1-b | VkPerf `tex fail=` + AssetRetry `texture vk-create retry` | fail 発生時も最終的に tex_pub 増(注入なし run では fail=0 が既定) |
| 7 | 非退行 | FRAMETIME / validation | 従来水準・validation 0 |
| 8 | 既存 A 系 | AssetStuck 行の stuck 各欄 | 全 0 |
- 視覚(建物が出る/消えない)は AYA の副次確認のみ・判定には使わない。
