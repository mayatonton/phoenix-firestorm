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

---

# §6. 基盤健全性層(Substrate Integrity)= 段階III gate 観測到達性の前提【draft・AYA approve 前・2026-07-28 夜】

> **発端(実証された設計欠陥)**: draw 再設計 段階III の III-1a(shadow 3-phase・S8-b 根治)は**機構は正しく動いた**(validation 0 / III-0 guard `C_PAR_CONCURRENT` 沈黙 = S8-b 根治の正のオラクル / 影は正常描画)。にもかかわらず gate run で**別経路の volumetric TDR → device-lost → freeze** し、**視覚 gate に到達すらできなかった**。
> AYA 判定 = 「**設計通り最後まで実装しても run が gate 観測点に到達する保証がない = 設計に穴。個別 patch は場当たりで穴は塞がらない**」。
> → 既存 draw 段階I/II の機構設計・本 doc の cadence 機構は**有効なまま**、足りていない**基盤層**を上に重ねる。本節は methodology(§4 の段階 I→IV)への**上位改訂**。

## §6.1 追加する不変条件(現設計が欠くもの)
- **INV-OBS(観測到達性)**: すべての gate は「run が観測点まで crash / hang / 無限ループ / freeze せず到達する(or clean 縮退する)」ことを**前提条件**とする。gate = 「**観測到達** ∧ (validation 0 + 検出器沈黙 + 視覚同一)」。観測到達が偽なら **PASS でも FAIL でもなく「観測不能」= 基盤層へ escalate**(機構の合否判定に使わない)。
- **INV-SUBSTRATE(基盤健全性)**: 全 frame は既知の crash / hang / loop / deadlock / teardown 経路に対し「**失敗しても clean に縮退する**」機構を持つ。**grandfather 禁止** = 「動くと仮定」した基盤経路(回復・shader loop 上限・teardown・待ち手解放)も再導出対象。
- **INV-WAITER(待ち手解放)**: 非同期 job の**あらゆる exit path(成功/失敗/skip/device-lost)は、その job に紐づく全待ち手(slot/fence/sync/pin)を必ず release** する。個別ケース列挙でなく「待ち手集合の単一解放点」で機構化。

## §6.2 blocker 全数台帳(種類欄付き・**初版・全数化は継続**)
> 棚は同じ(=「テストが最後まで走れない」)・**直し方は種類で違う**(無限ループ→上限 clamp / 無限待ち→待ち手 release / teardown→idle 確認後破棄 / 資源枯渇→hard cap)。**書く順 = 原因全数 → 直し方**(全数を取らずに機構を回した今回の再演を避ける)。

| ID | blocker | 種類 | 現状 / file:line | 破れた不変条件 | verdict |
|---|---|---|---|---|---|
| **G1** | volumetric shader loop 暴走 → GPU TDR → device lost | **GPU 無限ループ** | loop 上限に uniform 生値(P3/S13・volumetricLightF)・clamp 未 | shader loop 上限は有界(汚染 uniform で発散しない) | **BROKEN**(=P3) |
| **G2** | device-lost 後 peExecute 早期return が blocking submit の sync 待ち手を未解放 → 終了時 main freeze | **CPU 無限待ち(deadlock)** | llvkloader.cpp:973(gate は is_frame/is_oneshot のみ救い sync 漏れ)= `finding_peexecute_devlost_waiter_freeze` | INV-WAITER(全 exit path が全待ち手 release) | **BROKEN**(新規) |
| **G3** | teardown `vkDeviceWaitIdle` 戻り値未チェック → GPU hang 中終了で pending pool 破棄 → driver UAF → SEGV | **teardown-UAF/crash** | llvkloader.cpp:4910(=P4) | teardown は GPU idle 確認後にのみ資源破棄 | **BROKEN**(=P4) |
| **G4** | geometry burst で inflight 無制限膨張 → acquire 11.2s hang → device lost | **資源枯渇 hang** | crowd_step0 実測 501MB / safety_valve V2a(cap 実装済だが穴 2 点=最低1 job 分割不能 / inline 予算外) | 生産(inflight)は消費速度を超えない(hard cap) | **部分**(V2a 実装・穴残) |
| **G5** | async 順序破壊(S1)= worker pre_cmds が consumer 周期で scene CB より先に次frame CLEAR | **描画破綻(→未同定 hang/TDR の上流疑い)** | cadence §1 / P1 | scene frame S の worker CB は S の scene CB と同一 submit 列の直前で実行 | **BROKEN**(=P1) |
| **G6** | frame pacing 穴(vsync/limiter 無効で free-run)→ CPU/GPU 焼き | **暴走(hang 寄り)** | safety_valve V1(弁1 backstop・実装済) | main loop は無条件 backstop 上限を超えない | **FIX-IN-TREE**(V1) |
| **G7** | main one-shot submit の byte cap が実質 ∞ → inflight 膨張が背圧を受けず G4 を悪化 | 資源枯渇 hang | safety_valve V2b(main も 256MB へ・実装済) | 提出済み未完了資源 byte は上限を超えない | **FIX-IN-TREE**(V2b) |
| **G8** | teardown/device-lost 回復の producer quiesce 順序・reap 完全性(worker 生存中に破棄 → 共有キュー競合/UAF) | teardown-crash | teardown design C(`1c3f4513b2` = quiesce 最前 + reapAllDeferred 3 mode)。**ただし G2 のように回復経路に列挙漏れが残存** | INV-1(quiesce 後 VK 触りは main のみ)+ INV-WAITER | **部分**(C 実装・回復経路穴残 = G2 が実例) |
| **G9** | **S13 CLASS = 全 shader の uniform 由来 loop 上限**(volumetric=G1 は確定 1 例。他 shader は未棚卸し) | GPU 無限ループ | 全 shader grep + loop 上限のデータ源分類 未実施 | 汚染 uniform が loop を発散させない(全 shader clamp/有界) | **OPEN**(全数トレース待ち) |
| **G10** | **S2-S11 = per-frame 資源(scratch/arena/matrix ring/palette/lane CB/query/RT tracker)の apjob 遅延世界での寿命/reset 順序** | corruption→未同定 hang/TDR 潜在 | cadence §3 S2-S11 = 初期リスク評価のみ(未トレース) | 消費(scene CB 実行)前に per-f 資源が次周期で上書きされない | **OPEN**(全数トレース待ち) |

- **全数化の進捗**: G1-G8 = 実証/文書化済(直し方へ進める)。**G9(全 shader loop 棚卸し)・G10(S2-S11 寿命)= OPEN = 継続トレースで埋める**(初版で全数を名乗らない)。
- **全数化の方法(継続)**: ①async_safety_valve §0.1 の暴走経路台帳 ②cadence §3 の S1-S13 ③検出器(VKC/breadcrumb)が過去名指しした hang/TDR ④teardown 全経路(vknative_teardown_shutdown_design)を横断し、**「run が観測点に到達するのを妨げる」全経路**を上表に集約する。**未トレース欄を黙って落とさない**(§申告)。

## §6.2.1 G9 詳細 = shader loop 暴走 全数列挙(AYA 選択 = class 全体 clamp・2026-07-28 夜)
> **列挙は 3 型を対象**(初回 grep が減算型を取りこぼした教訓): (i)増加 `for(i=0;i<BOUND;i++)` (ii)**減算 `for(i=BOUND-1;i>0;--i)`**(=犯人 volumetric の型) (iii)`while`。**判定 = 上限が uniform/runtime(危険) か #define/const(安全)か**。

| shader:line | loop 上限 | 上限の源 | 判定 | 処置 |
|---|---|---|---|---|
| **volumetricLightF.glsl:138,199**(class1/class3) | `godray_res` | **`uniform int godray_res`**(:74)。正常最大=`RenderVolumetricLightingResolution` **4..64**(audit) | 🔴 危険(確定犯人) | `clamp(godray_res,1,64)`(≥正常最大64=挙動不変) |
| **screenSpaceReflUtil.glsl:130,138** | `int(iterationCount)` | **`uniform float iterationCount`**。正常最大=`RenderScreenSpaceReflectionIterations` **1..200**(audit) | 🔴 危険 | `clamp(int(iterationCount),0,256)`(≥200=挙動不変) |
| **postDeferredF.glsl:146,168 / postDeferredHQDoFF.glsl:153,174** | `its` | `int(max(1.0, sc*3.7))`・`sc≤max_cof=adj_COF=CameraMaxCoF(0.1..50 audit)/scale`。**大ターゲット時 scale<1 で正常 its ~925 まで**(外側ループは既に safety<32 済・内側 its は未有界) | 🔴 危険 | **`min(its,1024)`**(≥正常最大925=挙動不変)。⚠️**当初の `min(its,64)` は誤り=DoF 品質を削る = JIT-lite で捕捉** |
| reflectionProbeF.glsl:231 | `neighborCount` while | `refIndex[i].z`(buffer 由来) | 🟡 要確認(データ駆動・システム上限?) | max guard 検討 |
| reflectionProbeF.glsl:217,650,710,899 | `refmapCount`/`probeInfluences` | UBO/counter(.glsl 内宣言なし=要追跡) | 🟡 要確認 | 源確認後判定 |
| rlvF.glsl:106 | `numBlurPixelsPerSide`=`kernelSize/2` | `kernelSize`(.glsl 内宣言なし=define 注入疑い) | 🟡 要確認 | 源確認後判定 |
| irradianceGenF `u_sampleCount` | `=32`(local 固定) | 定数 | 🟢 安全 | — |
| multiPointLightF `LIGHT_COUNT` / aoUtil `NUM_DIRECTIONS/NUM_STEPS` / genbrdflut `NUM_SAMPLES` | #define | コンパイル定数 | 🟢 安全 | — |
| SMAA.glsl while 群 | `SMAA_MAX_SEARCH_STEPS*` | #define 上限 | 🟢 安全 | — |
| radianceGen/reflectionProbe `numSamples` | `uint(max(PROBE_FILTER_SAMPLES*roughness,1))` | #define × roughness∈[0,1] | 🟢 概ね安全(roughness 有界前提) | — |

- **確定処置(🔴 3 種)**: volumetric / SSR / DoF(postDeferred) の loop 上限を CPU 汚染に対し clamp。**不変条件 = 「uniform/runtime 由来の loop 上限は全て有界」**。
- **要確認(🟡 3 種)**: reflectionProbe(refmapCount/neighborCount/probeInfluences) と rlvF(kernelSize) の源を次 hop で追跡し、システム上限が構造保証されているか判定(保証あれば安全・なければ clamp)。**黙って落とさない**。

## §6.3 gate 仕様の改訂(観測到達性ファースト・3 層)
段階III の各段(draw III-0..III-6 / cadence P1..P10)の gate を次の 3 層順に判定する:
1. **層0(観測到達)**: 標準ストレス走行(crowd/burst/TP を含む)が **device-lost/hang/loop/freeze なしで完走 or clean 縮退**。**偽なら機構判定に進まない**(= 基盤 blocker を先に塞ぐ)。
2. **層1(機構正当)**: validation 0 + 検出器沈黙(III-0 guard 等)+ L3 型 A/B(新旧 byte 一致)。
3. **層2(視覚最終)**: AYA 視覚同一(最終 gate のみ・判定オラクルにしない)。

## §6.4 統一依存 DAG(2 plan を 1 本に・skeleton)
重なる 2 plan(draw 再設計 段階III / 本 cadence survey 段階III)を**単一の依存順序**に統合し、共有基盤(G1-G6)を明示 owner 化する。原則 = **基盤健全性層(G1-G6 が層0 gate を通る)を全機構段階の前提に置く**。
```
[基盤健全性層: G2(waiter)→G3(teardown)→G4(inflight)→G1(volumetric clamp) …層0 gate 成立]
        │(これが green になるまで機構段階を gate しない)
        ▼
[cadence 機構: P1(順序)→P5/P6(seed/epoch)→…]   [draw 機構: III-1(背骨)→III-2(seed)→…III-6(alpha)]
```
- **順序原則(初版)**: 観測到達を最速で回復する順 = **G2(freeze→clean 化・gdb kill 不要に)→ G1(volumetric clamp・run が TDR を生存)** を先行(この 2 本で gate run が回せるようになる)→ 残 G3/G4 → 機構段階。**確定は §6.5 の決裁後**。
- **worker↔worker 隔離辺(公理 F′)= draw 機構段階の内部前提**: draw 機構の III-2.5 に、worker が共有 program インスタンス可変状態を write しない不変条件(per-lane / arena 隔離 + V5 検出器)を置く。設計本体 = redesign doc **§0.1(no-grandfather / no-implemented)+ §9(公理 F′)**。統一 DAG はこれを owner=draw 機構として辿る(survey 側に複製しない・真実源は redesign)。

## §6.5 未決(AYA 決裁 / 継続トレースで埋める)
- 本節を **cadence survey §4 methodology の上位改訂**として確定してよいか(draw_impl_plan は「基盤健全性層が層0 gate を通ること」を各段の前提として 1 行参照する形)。
- 統一 DAG を**唯一の master**にし、両 plan の個別段階順序をこれに従属させるか。
- 基盤層スコープ = 「観測到達(run が落ちない)」に限定(広義の回復正当性まで広げない)= AYA 既決。

## §6.6 申告(縮小・省略・解釈)
- **省略**: 各 blocker の具体 fix 実装(volumetric clamp の値・peExecute の解放構造の詳細コード)は本節に含めない(各 JIT で確定)。本節は「どの不変条件で・どの順序で塞ぐか」まで。
- **未完**: §6.2 台帳は**初版(G1-G6)= 全数でない**。§6.2 末の方法で全数化を継続(未トレース欄あり)。
- **解釈**: 「基盤健全性」= gate 観測到達に必要な最小集合(run 生存 + clean 縮退)。product 機能の正しさは含めない。
- **私(agent)の miss 記録**: cadence P2/P3(volumetric TDR)を読了していたのに III-1a gate 依頼に「run が TDR で落ち得る」前提を組み込まなかった = INV-OBS の欠如が着手 readiness にも現れた(→ 着手 readiness に「gate 到達前提 blocker」欄を必須化)。

## §6.7 実装前 JIT 設計の要否基準(全 blocker 共通・AYA 制定 2026-07-28 夜)

> **原則: JIT 設計が必要と判定した blocker は、JIT 詳細設計(触る file・データ構造・罠・gate・申告)を出してから実装に入る。JIT 不要と判定したものは「ビルド前の単独宣言(触る箇所 + 挙動不変の根拠)」のみで実装可。** どちらも憲法(コード変更/ビルドは明示許可後・ビルド前宣言)には従う。JIT の**省略は要否基準に照らして明示判定**する(「小さいから省く」の直感で飛ばさない)。

### 要否基準
**JIT 必要**(1 つでも該当):
- (a) **product 可視挙動を変え得る**(品質/機能/見た目/性能特性)。※特に「clamp/上限/skip/fallback」は**正常時挙動を変えないことの証明**が要る = 品質トレード禁止の関門。
- (b) 新規機構・データ構造・新 file を作る。
- (c) **罠**を持つ(順序依存 / 寿命 / 並行性 / cross-CB / layout 遷移)。
- (d) 複数 file / 複数経路に波及、or 検出器 file(`llvkcontract.*`)に触れる(憲法 4)。

**JIT 不要**(全て該当・宣言のみで可):
- 既存パターンの局所コピー/mirror、かつ 挙動不変が自明 or 軽量確認で証明可、かつ 単一 file・数行、かつ 上記(a)-(d) の罠なし。

### 現 blocker の JIT 要否判定(初版)
| blocker | JIT 要否 | 理由 |
|---|---|---|
| **G2**(peExecute 待ち手解放) | **不要**(宣言のみ) | 既存 1136-1147 の mirror・device-lost 時のみ・挙動不変明白・単一 file 数行・罠なし |
| **G9🔴**(volumetric/SSR/DoF clamp) | **JIT-lite 必要** | 基準(a)= clamp 上限を誤ると**サイレント品質低下**。JIT の核 = 「**各 clamp 上限 ≥ 正常時最大値**(汚染時のみ効き通常無変化)」を readonly で確認し申告に明記してから実装。設計書本体は不要だが**この確認は省けない** |
| **G9🟡**(reflectionProbe/rlvF) | **JIT 必要** | 源(UBO/define)の構造上限を追跡して clamp/安全を判定(罠 = データ駆動寿命) |
| **G3**(teardown WaitIdle) | **JIT 必要** | 罠(寿命・破棄順序)+ teardown 全経路波及 |
| **G4**(inflight cap 穴) | **JIT 必要** | 罠(生産/消費背圧)+ 挙動(完成遅延) |
| **G5/P1**(async 順序破壊) | **JIT 必要** | 新機構(scene 結合チャネル)+ 順序罠 + 波及大 |
| **G10**(S2-S11 寿命) | **JIT 必要** | 未トレース + 寿命/並行罠 |

### 適用
- G2 = 宣言のみで実装可。G9🔴 = JIT-lite(clamp 上限 ≥ 正常最大の確認)を実装に織り込む。G9🟡/G3/G4/G5/G10 = 各 JIT 詳細設計 → AYA 承認 → 実装。
