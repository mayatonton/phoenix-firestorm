# 非同期 cadence 分離下の順序・寿命 構造欠陥 大規模調査(TOP・根治優先)

- 状態: **調査開始 2026-07-28(AYA 指定 TOP)**。HEAD = B1a 実装後・未 commit 作業ツリー。
- 発端: Phase B1a(camera gbuffer worker 化)で avatar 全消失級の描画破綻 + GPU hang(TDR)+ 終了時 SEGV が再現。トレースで**設計ミス(下記 §1)を確定**。同型の前提破れが**広域に存在する可能性**があるため全数調査とする。
- 真実源: HEAD 実コード。作業台帳 = scratch `trace_b1a_devlost.md`(発見の全 hop)。

---

## 0. 改修対象の全数台帳(TOP・優先順・AYA gate まで「直った」を名乗らない)

> 本表が唯一の改修台帳。status は OPEN / FIX-IN-TREE(実装済・未検収・未 commit)/ BROKEN(確定・未実装)のみ。**CLOSED にできるのは AYA gate だけ**。

| P | 問題 | 実体 | status |
|---|---|---|---|
| P1 | **S1 順序破壊**: worker pre_cmds が consumer 周期 submit で scene CB より先に次 frame CLEAR を実行(avatar 全消失級・Phase A shadow も同穴) | §1 | **BROKEN** |
| P2 | **GPU hang(TDR)**: volumetric 区間で shader ループスピン・毒の同定未。P1 の下流仮説はあくまで仮説 = **P1 根治後の実測で判定し、推論で closed にしない** | §2-3 | **OPEN** |
| P3 | **S13 shader loop 上限の無防備(クラス欠陥)**: uniform 生値を loop 上限に使う全 shader = 汚染 1 個が TDR に増幅。volumetric は即時 clamp(1..256・挙動不変・隠蔽でない = 汚染は CPU 側検出器で可視のまま)+ 全 shader 棚卸し | §3 S13 | **BROKEN**(volumetric)+ OPEN(全数) |
| P4 | **S12 teardown 検知漏れ**: vkDeviceWaitIdle(llvkloader.cpp:4910)戻り値未チェック → GPU hang 中終了で pending pool 破棄 → driver UAF → jemalloc SEGV(実測) | §2-2 | **BROKEN** |
| P5 | **seed 経路の検証バイパス**: 凍結 seed set が topoGen/ringSig/buffer 同一性を未検証 → cross-buffer 読み(関節異方向の根と同定・ただし同定も未検収) | §2-1 | FIX-IN-TREE(未検収) |
| P6 | **worker bind memo の epoch 非失効**: lane CB handle 再利用で VB/IB bind 省略(VUID 実測)→ rwResetRecordThreadLocals に epoch++ | — | FIX-IN-TREE(validation 0 化は実測・gate 未) |
| P7 | **fence cross-thread(validation ×25)**: vkGetFenceStatus/vkResetFences の 2 thread 同時使用(texWorker 系疑い)・帰属未 | S11 | **OPEN** |
| P8 | **texWorkerUpload layout UNDEFINED(×17・走行 1 のみ)**: 帰属未 | S8 | **OPEN** |
| P9 | **S2-S11 の cadence 分離前提の全数**(scratch/arena/matrix ring/palette/lane CB 寿命ほか) | §3 | **OPEN(全数トレース待ち)** |
| P10 | B1a(camera worker 化)本体: P1-P9 の決着まで **gate 凍結**・`AYACameraRecordMT` 既定の扱いは AYA 裁定 | — | 凍結 |

- 順序原則: **P1 → P4 → P3(volumetric clamp)を先行実装**(P2 の判定前提を整えるため)→ P2 実測判定 → P9 全数 → P5/P6 検収 → P7/P8 帰属 → P10 裁定。
- 各 P の完了条件は個別に AYA gate(視覚 + validation + 検出器沈黙 + 当該オラクル)。**agent の自己宣言は無効(憲法 1)**。

## 1. 確定した設計ミス(B1a・順序破壊)

**uiscene 非同期分離(consumer=present 周期 / producer=scene 周期・本機で常時有効)の下で、worker record CB(pre_cmds)を consumer 周期の submit(cjob.pre_cmds = llvkloader.cpp:6170)に載せた。** scene frame S の本体 CB(apjob)は「S の記録が終わった consumer frame の末尾」に submit される(:6199-6212)ため、次の順序逆転が構造的に発生する:

```
[S の worker CB(gbuffer clear+materials 描画)] … cjob(k) で実行
[S+1 の worker CB(gbuffer CLEAR!)]           … cjob(k+1) で実行 ← S の gbuffer をここで消す
[S の scene CB(lighting = gbuffer を読む)]    … apjob(S) で実行 ← 消された後に読む
```

= scene S の gbuffer は消費前に S+1 の CLEAR で破壊される。**avatar(worker 行き materials rigged)が描画されない観測と一致。** Phase A の shadow worker も同一の穴(shadow map が消費前に次 frame の CLEAR で上書きされ得る = 影の 1 frame 古化として潜伏・未露見)。

**破れた不変条件**: 「scene frame S の worker CB は、S の scene CB と同一 submit 列の直前で実行される」。
**根治方針**: worker CB の join 先を consumer の `sPendingPreFrameCmds` から scene 専用リストへ分離し、async 時は **apjob.pre_cmds** に結合(非 async 時は従来 = main CB の pre で等価)。shadow(Phase A)も同チャネルへ移す。

## 2. 併発で確定した欠陥(同日トレース)

1. **seed 経路の検証バイパス**(llglslshader.cpp seed 分岐): 凍結 seed set は vkValidatePerDrawSlot(topoGen/ringSig 等)を通らず、slice の実 buffer と set の buffer の不一致を検出できなかった → **修正済(未 commit)**: RecordSeed に buffer 群を封入し draw 時全比較・不一致 fail-visible。Phase A 稀症状「関節の異方向」はこの穴の既発症例(palette adopt の cross-buffer)と同定。
2. **teardown 契約の検知漏れ**(llvkloader.cpp:4910): `vkDeviceWaitIdle` の戻り値未チェック → GPU hang 中の終了で DEVICE_LOST が返っても idle 扱い → pending CB を持つ pool 破棄(実測 `vkDestroyCommandPool: CB is in use` ×6)→ driver UAF → jemalloc SEGV(実測)。**未修正**(本調査内で根治)。
3. **GPU hang(TDR)= 根本毒は未同定**: breadcrumb は volumetric 区間(fin:volumetric〜fin:combine_glow)・fault VA 群は shader 命令ポインタ(= ループスピン)。§1 の gbuffer 破壊による garbage 入力の下流である可能性が高いが、§1 根治後の再検証で確定させる。**§1 修正後も再現するなら独立バグとして継続追跡**(worker CB に breadcrumb `rw:*` 配備済)。

## 3. 調査スコープ(全数・「consumer 周期に結び付いた前提」の棚卸し)

対象 = **sFrameIndex(consumer FIF)/ sMonotonicFrameCount / endFrame 周期に紐づく全資源**について、「scene CB(apjob)が後から submit・実行される」世界で順序・寿命・reset が正しいかを file:line で判定する。

| # | 資源/機構 | 前提の型 | 初期リスク評価 |
|---|---|---|---|
| S1 | record worker CB(pre_cmds 経路) | 順序(消費前破壊) | **確定 BROKEN(§1)** |
| S2 | record lane CB pool reset(sMonotonicFrameCount・FIF slot) | 寿命(GPU 実行中 reset) | 要判定: cjob fence 保護前提が apjob 移行後も成立するか |
| S3 | DrawData scratch(per-f cursor reset @ beginFrame) | 寿命: scene CB 実行前に scratch が次周期で上書きされないか | 要判定(apjob の実行は f+? まで遅延し得る) |
| S4 | per-draw UBO arena(sPerDrawUBOArena[f]・monotonic reset) | 寿命: 同上 | 要判定 |
| S5 | matrix ring(per-f slots) | 寿命: 同上 | 要判定 |
| S6 | ObjectSkin/palette ring・skin bindless SSBO(per-f) | 寿命: 同上 | 要判定 |
| S7 | mega-buffer / indirect ring の per-f 資源 | 寿命 | 要判定 |
| S8 | one-shot texture upload(PE 経由)と scene CB の順序 | 順序 | 要判定(既存機構・T1 由来) |
| S9 | occlusion query の issue/readback 周期 | 順序/寿命 | 要判定 |
| S10 | RT layout CPU tracker(記録順 = 実行順前提) | 順序 | 要判定(B1a clear job の tracker 先行更新を含む) |
| S11 | fence/semaphore の slot 対応(cjob/pjob/apjob 3 系統) | 寿命 | 要判定(fence cross-thread validation ×25 の帰属もここ) |
| S12 | teardown(quiesce→WaitIdle→destroy)全経路 | 寿命 | **確定 BROKEN(§2-2)** |
| S13 | **全 shader の uniform 依存 loop 上限**(volumetricLightF:138/199 ほか全数 grep + 目視) | 頑健性(汚染 1 個 → TDR 増幅) | **確定 BROKEN(volumetric)**+ 全数未 |

判定様式: 各行に (a) 実行順の保証者(fence/submit 順/barrier)を file:line で名指し (b) apjob 遅延世界での反例の有無 (c) verdict = SAFE(根拠)/ BROKEN(反例)/ N/A。**「動いているから安全」は禁止**(憲法 6)。

## 4. 工程(方法論改定 = AYA 指示 2026-07-28)

> **⛔ 方法論の禁止事項: 設計欠陥を計測で直さない。** 「実装 → 走行 → 症状 → パッチ」の反復は永遠に収束しない(B1a で実証: epoch fix・seed fix は症状応答であり、構造欠陥 S1 は設計時にソースから導出可能だった)。**正 = ソースから構造を全数洗い、真のモデルを文書化し、その上で設計をやり直し、実装し直す大工事。計測は最終 gate のみ。**

1. **段階 I = 真のモデル文書化(read-only・全数)**: submit 列(cjob/pjob/apjob/one-shot)・fence/semaphore の全対応・per-frame 資源(scratch/arena/matrix ring/palette/lane CB/query/RT tracker)の reset と GPU 消費の生存関係を、**両モード(sync/async)× frame またぎ**でソースから導出し本 doc に「真のモデル」として明記。S1-S13 の verdict はこのモデルから機械的に落ちる。
2. **段階 II = 再設計**: worker record 機構(Phase A shadow + Phase B camera の両方)を真のモデルの上で設計し直す。Phase A も grandfather しない(同じ砂上のモデルで設計され gate を運で通過した前提で再導出)。設計 doc 改訂 → AYA approve。
3. **段階 III = 再実装(大工事)**: 承認済み設計の一括実装。P4(teardown)・P3(S13 clamp 全数)も同工事に含める。
4. **段階 IV = gate(ここで初めて計測)**: 視覚 + validation + 検出器沈黙 + 実測。P2(GPU hang)の判定もここ。
- B1a の現実装 = 段階 II の入力(破棄前提の draft)。`AYACameraRecordMT` 既定の扱いは AYA 裁定。

## 5. 縮小・省略・解釈申告

- 本 doc 起草時点で S2-S11 は未トレース(初期リスク評価のみ)= 調査で埋める。
- 「非 async(分離無効)モード」は S1 の反例が構造的に成立しない(pre_cmds と main CB が同一 submit)ため優先度を下げるが、S3-S7 の寿命判定は両モード共通で行う。
