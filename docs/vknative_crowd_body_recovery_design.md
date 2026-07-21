# 本体設計 — crowd 使用可能化(設計 + 実装計画)

- 状態: 設計者起草 2026-07-22(AYA 指示「本体に着手するための現状欠陥を俯瞰し設計を立てよ」)。approve 前。
- **🔀 本線移管(2026-07-22 深夜・AYA 会話)**: 本 doc §4 stepA(eliminate = 脱 texture batch key で rebuild を消す)は **不採用・履歴**。本線は **`docs/vknative_avatar_relocate_design.md`(avatar 描画を main の外へ丸ごと退避 = relocate + per-core 分散)** へ移管。本 doc の §1-3(実データ会計)・§4.2-4.4(churn 帰属)・§5(有界化 = 信頼性安全弁)・measurements doc は資産として保持。
- 位置づけ: `docs/vknative_recovery_plan.md` の §0 診断を **95-av crowd の LIVE 実データ**で更新し、T/E 系完了後に残った「使えない fps 11」の本体を攻める工程書。geometry rebuild 有界化(元 TOP)は本体でなく信頼性安全弁と再位置づけ(§5)。
- **⚠️ 訂正(2026-07-22 深夜・step0 計測後)**: 本 doc 起草時の「前 TOP(geometry rebuild)は前任の矮小化」という位置づけは**撤回**する。降格根拠の「定常 gupd=0.9ms/f」は**非フォーカス走行の汚染データ**(→ `docs/vknative_crowd_step0_measurements.md` §1)で、focused 実測では **gupd 12-30ms/f の恒常 churn = geometry rebuild は本体の主要項**。前任の標的選定(geometry rebuild)は正しかった。誤っていたのは処方(burst 用 drain 有界化は steady churn に無力・欠陥 2 点 = §5)と、当時それを判定できる計測環境。step0 の産物は「標的の復権 + 処方に必要な精度」(装着物 bridge 92%・入口 = TE/vol/sculpt・下部構造 = batch 併合と record 追従性)。
- 真実源: 実行コード(HEAD)。数字は LIVE ログ `~/.ayastorm_x64/logs/AYAstorm.log`(2026-07-22・frames=57・fps 11.32・avg 88.34ms/f・draws/f 27930・cpu main 96%)。

---

## 1. 現状欠陥の俯瞰(LIVE 実データ・誤読回避のため phase 実体を code で確定済)

VkPerfPhaseScope は **inclusive**(nested 非減算・llvkloader.cpp:10621)。ph(0)=`idle()`(llappviewer:1726)/ ph(1)=`disp`=`display()` 全体(llappviewer:1775)/ ph(2)=`probe`=reflection update(llappviewer:1786)。ph(3-13) は display() 内の下位。

### 1.1 top-level 会計(非重複)
| ブロック | ms/f | 実体 |
|---|---|---|
| **未計上** | **~40.6** | **avg 88.34 − (idle+disp+probe 47.75)。frame の 46%。帰属不明 = 枢軸(§2)** |
| disp = display() 全体 | 30.8 | 描画(下位は §1.2) |
| idle() | 14.4 | main 更新 tick(下位は §1.3) |
| probe(reflection update) | 2.6 | 鏡/probe 更新 |

### 1.2 display() 30.8ms の下位(inclusive・disp の内訳)
light 10.0 / shad 9.0 / geom 4.5 / img 2.0 / sort 1.5 / cull 1.0 / gupd 0.9 / ui 1.4 / hero 0.0 / imp 0.0 / swap 0.0(合計 ≈ 30.3 ≈ disp 30.8・差 ~0.5 は display 内未 scope)。
- light 10.0 の内訳は **lgt 欄で既に取れている**(ms/f・llvkloader.cpp:4812-4828): fwd 4.2 / misc 3.5 / spot 1.6 / loc 0.6 / sun+blur+atm 0.4 = §6.4 の「未トレース」は既存ログで一次裁定可能。
- **gupd(geometry rebuild)= 0.9ms/f = 定常の非犯人**(元 TOP は cold burst の一過性・§5)。
- light(deferred lighting)+ shad(shadow render)= 19ms = display の 6 割。

### 1.3 idle() 14.4ms の下位(idl 欄・⚠️ 入れ子会計 = 2026-07-22 監査で構造確定)
- **idl 欄は非重複でない**。トップ層 idl(0-20) のみが disjoint(合計 821.1ms ≈ idle 821.6ms/57f で一致)。**obj(idl 9)= `gObjectList.update()` 全体の容れ物**(llappviewer.cpp:6108 → llviewerobjectlist.cpp:922)で、oAv(23)/oNav(24)/oFlex(25)/oTanim(26)/oMisc(27)/aChar dispatch(28・llviewerobjectlist.cpp:1065)を内包。さらに oAv の中に avatar idleUpdate の aPre(31)/aChar in-loop(llvoavatar.cpp:3007)/aMisc(29)/aName(30)。
- **disjoint な内訳(ms/f)**: obj 6.9 / net 2.6 / ui 1.4 / part 1.0 / tmr 0.7 / cb 0.45 / dead 0.36 / wld 0.32 / … 計 ≈ 14.4。
- **obj 6.9 の中身はほぼ avatar**: oAv 2.35(うち aMisc 0.54 + aName 0.34 + aPre 0.09 + aChar in-loop ≤1.4)+ aChar dispatch ≥3.7 + oFlex 0.10 + oTanim 0.39 + oNav 0.03。純 object 更新は ~0.5ms/f のみ。
- 設計含意: **stepB の idle 側配当プール = obj 6.9ms/f(avatar 支配)**。「aChar 5.1 + oAv 2.3 + obj 6.9 ≈ 14.3」は二重計上で不成立。idle 残り ~7.5ms/f(net/ui/part/tmr 等)は並列 dispatch の対象外の別性質。
- aChar 5.1 は最悪 window(同走行の他 window は 3.2-3.3ms/f)= 変動幅込みで見る。

### 1.4 draw 母集団(pass 別 draws/f)
scene 15260 / shadow 6040 / occl 3959 / probe 2671 = 計 27930。per-draw 簿記(recovery §0.2 = ScenePerDrawCache hash + set build + pipeline/bind memo)。

---

## 2. 枢軸 = 未計上 40.6ms/f の帰属 — ✅ 決着(2026-07-22 深夜・step0 mlp 計器の focused 再走行)

**verdict = (b) CPU-main-bound 確定 + 「未計上 40.6ms」自体が前回計測の artifact(非フォーカス走行の BackgroundYieldTime 40ms sleep・llappviewer.cpp:1838-1851 = 未 scope 区間)と判明。**
- focused 再走行(95-av crowd steady・fps 11-16)で会計閉鎖: idle+disp+probe+mlp = 実測 wall の **99.95%**(例: w frames=58 = 5080.5 vs 5082.5ms・残差 0.04%)。未計上ブロックは存在しない。
- main の GPU 待ち(mlp slot+fence+acq)= **≤3ms/f**(多くの window で <1ms/f)= GPU-bound 説は棄却。pe prs(PE thread present)も別 thread で main を塞がない。
- 前回(2026-07-21 夜)の 40.6ms/f ≈ BackgroundYieldTime 40ms と符合 + 非フォーカス署名(fps フラット 11-15 = memory env_measure_requires_viewer_focus)と一致。⚠️ 未解消の緊張: 当時の「cpu main 96%」外部計測は sleep と矛盾(出典時点不明・burst 中の値の可能性)= 断定は「artifact 濃厚」まで。
- **帰結: stepC(GPU 削減)は非本命確定。本体 = display 実作業(下記 focused 実測)+ idle**。focused steady 実像(ms/f・churn 収束途上の幅): disp 47-71(うち gupd 12-45 = 幾何 churn 収束中・light 14-15・shad 9-10・geom 5.5-6)/ idle 9-18 / probe 3。**gupd が steady でも 12ms/f+ 残る(前提「定常 0.9ms」は focused 環境で要再検証)+ light/shad 25ms = display 内が本丸。**

(以下は決着前の設計時の記述・履歴として保持)
**本体設計はこの 1 点で分岐する。40% を推測で攻めれば E1a/E4(instrument 誤読)を frame 規模で再演する。**
- cpu main=96% pegged = 1-core 動作(recovery §0.1 の癌署名)。だが「96% busy」は **実 CPU 作業**か **GPU/present 同期 spin-wait**かを区別しない。
- 40.6ms は idle()/display()/probe() の**外**= mainloop の未 scope 区間。**全列挙(doFrame 実トレース 2026-07-22)**: ①beginFrame(llappviewer.cpp:1759)= vkWaitForFences(llvkloader.cpp:4486)+ vkAcquireNextImageKHR(4512)= GPU/present backpressure が main に現れる唯一の場所 ②endFrame(1794) ③mainloop.post + llcoro::suspend = **coroutine pump**(1664-1677・HTTP/inventory 等が main で走る) ④message pump/gatherInput(1626-1651) ⑤LLReloadQueue::drain(1757) ⑥snapshot floater 群(1788-1791) ⑦updateTextureThreads/LLLFSThread(1866-1891) ⑧gMeshRepo.update()(1893-1896) ⑨LLTrace 簿記(1609-1623)。
- **既存計器の証拠(未活用だった)**: `pe sub_ms/prs_ms`(llvkloader.cpp:4913-4914)= PE thread の submit 0.67ms/f・present **8.1ms/f**(mt=1 = PE 別 thread)。present は PE thread 上ゆえ 40.6ms の直接内訳ではないが、GPU/present 側の深さの独立指標として step 0 の判定材料になる。
- **二仮説**:
  - (a) **GPU-bound**: beginFrame/endFrame が GPU(~88ms/f)を待つ = 40ms は GPU 同期待ち。→ 本体 = GPU 作業削減(draw COUNT・overdraw・shadow/probe render)。
  - (b) **CPU-main-bound**: 40ms は未 scope の main 実作業(message/mainloop)。→ 本体 = doctrine(毎 frame やり直さない = 記録保持 + 更新並列化)。
- **どちらかで攻め先が正反対**。ゆえに step 0(§4)= この帰属確定を**設計の前提工事**とする(measure でなく設計の地盤・未確定で本体着手は禁)。

---

## 3. 構造の根と doctrine(確定部分)

判明している大ブロックは全て **main 1 本の直列**:①display の draw 発行 28k(毎 frame 再記録)②idle の avatar/object 更新(毎 frame 全量)③lighting/shadow。[[project_cancer_single_main_thread_pipeline]] / [[project_vk_doctrine_eliminate_not_parallelize]]。

doctrine = VK が GL を超える原理は「毎 frame 同じ仕事をやり直さない」(記録保持 / bindless / MDI+GPU cull)。T 系(worker 化)+ E2(materials bindless)は部分。**残 = 記録保持(28k draw を毎 frame 再記録しない)+ 全 bindless/indirect + avatar 更新の並列/削減**が未達 = 本体。

---

## 4. 実装計画(分割・sequenced・各段 GEOAB/装置オラクル + gate)

### step 0(前提・非交渉)= frame 会計の閉鎖 = binding 確定
- 40.6ms を pin: crowd 実機で ①`perf record` の main callstack ②beginFrame/endFrame に計測 scope(GPU 同期待ちか実作業かを分離)③GPU timestamp(GPU frame 時間 vs CPU frame 時間)。
- 出力 = **CPU-main-bound(b)か GPU-bound(a)かの確定**。これで §4.A/§4.B の優先が決まる。
- 装置拡張 = 憲法 4(検出器 diff は AYA 承認)に触れないよう、既存 VkPerf phase の追加欄として設計 → AYA 承認 → 実装。

### step A(binding=CPU 確定 → 本命)= 「毎 frame / 毎 event やり直さない」構造への解体(設計起草 2026-07-22 深夜・approve 待ち)

> **⛔ 不採用・履歴(2026-07-22 深夜)**: 以下 lane A-1(a) eliminate = 脱 texture batch key は本線から外れた。理由 = batch identity 手術の E2 白バグリスク + churn は genuine ゆえ配当 capped + global allocator concurrent 化コストは eliminate でも避けられない。本線 = relocate(`docs/vknative_avatar_relocate_design.md`)。以下は帰属分析(A-1 の構造診断・§4.4 の破れ不変条件)としては有効ゆえ保持。

**不変条件(2 本)**:
- A-inv1: **group rebuild は幾何(頂点)内容の変化にのみ許される**。参照の変化(texture/material 属性)は in-place 伝播する。
- A-inv2: **draw 発行記録は scene 内容が変わらない限り再利用される**(28k draw/f の毎 frame 再記録の廃止)。

**lane A-1 = rebuild 構造の解体(gupd churn 12-30ms/f・帰属確定済 = measurements doc §4)**
- 構造的根本原因: **DrawInfo の batch identity に texture が含まれる**(registerFace・llvovolume.cpp:6848-6883 = GL 時代の遺産)。ゆえに参照変更 = batch identity 破壊 = group 全量 rebuild が唯一の伝播手段。poke 不発(適格母集団ゼロ)はこの構造の帰結。
- 選択肢: **(a) batch key の脱 texture 化**(bindless 下では texture は per-draw data = DrawData slot で record 時解決〔llspatialpartition.cpp:4212 で追従性確認済〕。batch key を material/shader/VB/skin に縮め、texture 変更を batch 中立化)/ (b) 局所 rebuild(影響 DrawInfo のみ split/merge)/ (c) 入口ゲート連打(markForUpdate 実変更化等)= poke で whack-a-mole と実証済み・非推奨。
- **設計者推奨 = (a)**。E2(materials bindless)の論理的完成形であり doctrine ど真ん中。増分 = A1-1: batch key 全成分と per-draw 解決線の設計トレース(実装前・計測走行なし)→ A1-2: bindless 済み 1 族(mat/pbr)で脱 texture 化 + 参照更新 API → gate: gds の facemap/vol 由来 rebuild 減 + rsnGB 減 + 視覚同一 + validation 0。リスク = E2 白バグ族 = L3 型オラクル必須。
- octaddB/octremB(extents 転居・~150/s)と sculpt streaming は A1-2 の gate 後に残量で判断(A-inv1 で rebuild 単価が落ちれば非本丸化の可能性)。

**lane A-2 = draw 記録保持(display 47-71ms/f 側・最大費目)**
- 28k draw の per-frame 再記録 → **記録保持 / GPU-driven / 全族 indirect**(recovery §2 E 系の到達形の完遂)。E2 bindless の全族展開 + MDI 恒久 + GPU cull。pass 単位に分割・各段 L3 幾何 A/B(GEOAB)+ 視覚同一 + validation 0。
- A-1(a) は A-2 の前提整備でもある(batch が参照変更で壊れない = 記録が保持可能になる)= **着手順 = A-1 → A-2**。

**運用注記**: 以後の計測走行は「設計のどの分岐を切るか」を 1 文で書けるものに限る(memory feedback_measurement_chain_as_main_work_avoidance・AYA 2026-07-22)。A1-1 は計測でなくコード設計トレース。

### step B(idle 更新)— ✅ v1 裁定済 = **revert 実施(AYA 承認 2026-07-22 深夜)**・per-avatar 粒度は死案
- **adsp A/B(同一走行 frame 交互・focused steady)の裁定**: par 2.0-4.5ms/dispatch vs ser 1.5-1.7ms/dispatch = **並列が 1.3〜2.9 倍遅い**(join 待ちが par の 95%+)。直列全量が ~56 avatar で 1.7ms = **~31µs/avatar** = この粒度では task 投函 + wakeup がペイしない(原理的)。
- **v1(`3ef35b315a` のコード)+ A/B 足場 = revert 済**(updateCharacter 原型復元・dispatch/分割関数/armed/batch/forwarder/adsp 欄 全削除)。**検出器 D1-D4(`81b7b0aef4`)は残置**(将来の粗粒度並列化の入場ゲート)。
- **stepB の再定義**: dispatch 適格プールは 1.7ms/f(frame の 2%)しかなく、idle は本丸でない。Phase 2 並列化を再開するなら **粗粒度**(per-avatar でなくサブシステム単位)+ 削減(全量 tick 廃止)が前提。優先度は stepA の後ろ。
- 安全 = 並列安全性検出器 D1-D4(`81b7b0aef4`)+ TSan(Layer A 未走行)。

### step C(binding=GPU なら本命)= GPU 作業削減
- draw COUNT(occl 後も 28k)・shadow/probe render・overdraw。step 0 が GPU-bound を出したときのみ本命化。

### 信頼性(本体でない・並行)= geometry apply 有界化
- cold burst(crowd 入場)の gupd=pub_ms 12s 凍結 → SIM 切断を有界化(実装済・未 commit)。差分化が burst に無効なことは trace で証明済(全新規幾何)。§5。

---

## 5. geometry rebuild 有界化の再位置づけ(元 TOP → 信頼性安全弁)

元 TOP は装置が cold burst 崩壊(gupd 6.6-7s → 切断)で名指し。LIVE 実データで **定常 gupd=0.9ms = 本体でない**と確定。有界化は crowd 入場の凍結/切断を防ぐ**信頼性の安全弁**であり、fps 11 の本体は §4。有界化の commit 判断は AYA(現状 uncommitted)。
- **未 commit diff の申告(2026-07-22 監査で確定)**: ①有界化は **default 8ms ON**(llvovolume.cpp drainGeoPublishQueue・env `AYASTORM_GEO_APPLY_BUDGET_MS` は override)= product-visible な既定挙動変更として AYA 裁定要 ②diff は **VkPerf 欄追加(rsnA/rsnG・llvkloader.h/cpp)を含む = 憲法 4 対象 file** = 承認パッケージに明示 ③rsnG=1175/6985 vs rsnA=0/0(LIVE ログ)= 「burst は全新規幾何・差分化無効」主張をデータで支持。
- **⚠️ 有界化の実証結果(2026-07-22 深夜 step0 走行 = cold burst 中に device lost で実測)= 現状の安全弁は GPU 死を防げていない。欠陥 2 点(コード + ログで確定)**:
  1. **job 粒度の下限**: budget 判定は `if (any && …)` = 最低 1 job は必ず apply。単一巨大 job は分割不能(最終 window: pub=3 で pub_ms=1404 = **~470ms/job**)。
  2. **inline 経路が予算外**: `staged.mInline` の同期 rebuild は publish queue を通らず budget 対象外(burst window で **inl=15534/79f = 197 件/frame** が素通り)。
  - 同走行の観測事実: defer カウンタは稼働(w6 defer=1598 = budget 自体は動作)/ inflight geometry **501MB**(mb 欄)+ draws/f 21818 まで膨張 → vkAcquireNextImageKHR 11.2s hang → VK_ERROR_DEVICE_LOST(clean 終了経路は正常動作)。device lost の直接原因の帰属は未確定(推測禁)。

---

## 6. 縮小・省略・解釈申告

1. **40.6ms 未計上が最大の未知**。§4.A/B/C の優先は step 0 の binding 確定に従属 = 現時点で「本命はこれ」と断定しない(推測 = E1a 再演)。判明ブロック(display 30.8 / idle 14.4)への doctrine 適用は binding に依らず有効ゆえ step A/B は並行設計可。
2. **desync(装着物/本体)は本 doc 未解決**。draw 不体裁 = 🕸️ 装置(VKC L2 連続性オラクル)が名指しすべき事項で、本体 fps とは別系統。step 0 の crowd 走行で VKC を併用し名指しさせる(cost ゼロ)。
3. **E 系「レバー全滅」memory との整合**: あれは per-draw *issue collapse*(MDI run 化)の配当ゼロ確定。本 doc step A は issue collapse でなく **draw 数削減 / 記録保持**(別レバー)= 矛盾しない(recovery §2 E5 の未達部分)。
4. lighting(10ms)の一次内訳は lgt 欄で取得済(§1.2 = fwd 4.2 / misc 3.5 / spot 1.6)。misc 3.5 の実体と shad 9.0 の内訳は未トレース = step 0 と併せて分解。
5. cpu main 96% の出典は外部計測(top)= VkPerf ログ内に無し。
6. 本 doc の会計主張は 2026-07-22 に HEAD 実コード + LIVE ログで全数トレース監査済(scope 配置 llappviewer.cpp:1725/1775/1785・inclusive 実装 llvkloader.cpp:10621・idl 入れ子 llviewerobjectlist.cpp:922-1090・数値は log 最終 window frames=57 と除算一致)。
