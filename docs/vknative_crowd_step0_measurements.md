# crowd step0 計測記録(2026-07-22 深夜・全 6 走行)— 再走行不要の一次資料

- 目的: 本 doc だけで「95-av crowd の frame がどこで燃えているか」の帰属を再走行なしで参照できること。設計 = `docs/vknative_crowd_body_recovery_design.md`(工程)/ 本 doc(実測の生値と裁定)。
- 環境: 95-av crowd(同一 SIM 同一場所)・focused・`AYASTORM_VKC=1 AYASTORM_PERF_LOG=5`。数値は特記なければ steady window(5s)を frame 数で除した ms/f または 件/s。
- ⚠️ 走行間の絶対値比較は禁物(scene 変動 = sculpt 欄が走行間で 3 倍動いた実例あり)。裁定は同一走行内の比率・分布で行うこと。

## 0. 退避ログ台帳(~/.ayastorm_x64/logs/)
| file | 内容 | 主な裁定 |
|---|---|---|
| AYAstorm.crowd-collapse-20260722-0045JST | 前夜: cold-burst 崩壊(SIM 切断) | gupd burst 名指し(旧 TOP の根拠) |
| AYAstorm.crowd-body-instrumented-20260722-0128JST | 前夜: steady 到達・**非フォーカス疑い** | 「未計上 40.6ms」の出所(→走行 2 で artifact 判明) |
| AYAstorm.crowd-step0-devlost-20260722-0246JST | 走行 1: mlp+adsp 初回・cold burst 中 device lost | acquire 11.2s hang→devlost / 有界化の欠陥 2 点 |
| AYAstorm.crowd-step0-focused-FULL-20260722 | 走行 2: focused steady 110 window | **binding 確定・40.6ms 解体・adsp 裁定・gupd churn 発見** |
| AYAstorm.crowd-gds1-20260722 | 走行 3: gds v1(site 帰属 12 分類) | 仮説 3 本棄却・octree/dirtySG 残差絞込 |
| AYAstorm.crowd-gds2-20260722 | 走行 4: gds v2(bridge 分離+rsnGB) | **rebuild の 92% = attachment bridge** |
| AYAstorm.crowd-gds3-20260722 | 走行 5: gds v3(複合フラグ 5 分割) | vol/facemap/sculpt 支配・lod 単独は非支配 |
| AYAstorm.crowd-poke-gate-20260722 | 走行 6: texture poke 検収 | **fix 不発(pokeOK 0.1%)** |

## 1. frame 会計(走行 2・focused steady・fps 11-16・draws/f 28-29.6k)
- **会計閉鎖達成**: idle+disp+probe+mlp = wall の 99.95%(例 w58f: 5080.5 vs 5082.5ms・残差 0.04%)。
- **binding = CPU-main-bound 確定**: main の GPU 待ち(mlp slot+fence+acq)≤3ms/f・多くは <1ms/f。GPU-bound 説棄却 = stepC 非本命。
- **「未計上 40.6ms/f」(前夜計測)= artifact 濃厚**: BackgroundYieldTime 40ms(llappviewer.cpp:1838-1851・唯一の未 scope 区間)と符合・focused では消滅。⚠️ 当時の「cpu main 96%」外部計測とは未解消の緊張(断定は「濃厚」まで)。
- focused steady の実像(ms/f・churn 収束途上の幅): **disp 47-71**(gupd 12-45・light 14-15・shad 9-10・geom 5.5-6)/ **idle 9-18**(容れ物 = obj 6.9・中身はほぼ avatar)/ probe 3。
- mlp 恒久欄の読み方: slot/fence/acq(beg に内包)/beg/end/coro/pump/rld/snap/tio/mesh/trc。残差計算は beg を使う(内訳 3 欄は診断用)。

## 2. idle の入れ子会計(恒久知識)
- idl 欄はトップ層 idl(0-20) のみ disjoint(合計 ≈ idle で検算済)。**obj(9) = gObjectList.update() 全体の容れ物**で oAv(23)/oNav(24)/oFlex(25)/oTanim(26)/oMisc(27) を内包、oAv の中に aPre(31)/aChar in-loop(28)/aMisc(29)/aName(30)。
- 「avatar 7.7 + object 6.9」という旧集計は二重計上(訂正済・設計書 §1.3)。

## 3. aChar 並列 v1 の裁定(走行 2・adsp 同一走行 frame 交互 A/B)
- **par 2.0-4.5ms/dispatch vs ser 1.5-1.7ms/dispatch = 並列が 1.3-2.9 倍遅い**(全 steady window で一貫・join 待ちが par の 95%+)。
- 原理: バッチ ~56 avatar で直列全量 1.7ms = **~31µs/avatar** = task 投函+wakeup がペイしない。**per-avatar 粒度は死案**(Phase 2 再開は粗粒度が前提)。
- 処置: v1(`3ef35b315a` のコード)+ A/B 足場 = **revert 実施済**。検出器 D1-D4(`81b7b0aec4`→正: `81b7b0aef4`)は残置。revert 後 sanity: aChar 3.6-3.9ms/f < oAv 5.0-5.5(入れ子整合復帰)・fps regime 同等。

## 4. gupd 恒常 churn の帰属(走行 3→5 の連鎖・本 TOP の最重要成果)
### 4.1 規模(走行 2-6 で再現)
steady でも **gupd 12-32ms/f が収束しない**(10 分+)。geo 欄 = enq/pub 400-780 件/5s(~80-160 job/s)・defer=0(予算未関与)・rsnA=0(alpha 系ゼロ)= 全て geom-dirty full rebuild。

### 4.2 site 帰属(gds 恒久欄・件/s・走行内比率で読む)
| 裁定 | site | 実測 | 備考 |
|---|---|---|---|
| ❌棄却 | animset/animclr(ANIMATED_CHILD 往復) | 各 ~8/s | lldrawable.cpp:731-737 / pipeline.cpp:2666-2676 |
| ❌棄却 | flexi(揺れ物) | 4-12/s | |
| ❌棄却 | requeue(publish 失敗ループ)/sss/texdirty/octtrav | 0 | llvovolume.cpp:6510 は健全 |
| 対象外 | part(粒子) | ~700-900/s | by-design 毎 frame・別 partition |
| 🔴 | **octaddB/octremB(bridge 内 octree 転居)** | 100-390/s | 骨格アニメ→extents 変化→再挿入 |
| 🔴 | **vol(mVolumeChanged)** | 110-270/s | 唯一の setter = markForUpdate(llvovolume.cpp:4936)= 汎用 API の無条件昇格。caller 別頻度は**未計測(OPEN)** |
| 🔴 | **facemap(TE/texture 変更)** | 140-300/s | setTE* 群(全て同値ゲート済 = 本物の変更)。vol と強相関(SIM full update が対で焚く署名 = :505/:588 setVolume→markForUpdate + unpackTEMessage) |
| 🟡 | **sculpt** | 36-265/s(走行間変動大) | notifyMeshLoaded(:1455)+ sculpt texture discard 改善(:1070)= streaming 由来 |
| 🟡 | lod(純 LOD)/ color | 各 ~35-100/s | 単独では非支配 |

### 4.3 実行側の決定打
**rsnGB/rsnG = 88-92%** = 全量 rebuild のほぼ全てが **attachment bridge の group**(走行 4-6 で再現)。world は無実。rebuild 実行 ~124-218 件/s・fill は 2-4 面/rebuild と選択的 = **コスト主体は fill でなく group rebuild 機構**(DrawInfo 全再生成+staging+publish = pub 0.8-3.6ms/job)。

### 4.4 破れている不変条件(fix 設計の起点)
「幾何(頂点)が変わらない更新 — TE/texture 変更・streaming 一過性・extents 微変動 — が、attachment bridge group の全量 rebuild に昇格してはならない」。
- 昇格の機構(file:line): markForUpdate:4936(無条件 mVolumeChanged)/ setVolume:1319-1321(volume/sculpt 変化時に facemap 連鎖点火)/ updateGeometry:2360・2382(dirtySpatialGroup 昇格 + 純 texture 変更でも regenFaces)/ フラグ clear は :2437-2441 で健全(echo なし)。
- record 時の texture 解決: lldrawpool.cpp:611-613(DrawInfo::mTexture → heap slot・毎 record)+ ensureVkDrawDataSlot(llspatialpartition.cpp:4212・memcmp 追従)= **DrawInfo の texture ポインタさえ更新できれば rebuild 不要**という下部構造は実在。

## 5. fix 増分 1(texture poke)の検収結果 = 不発(走行 6)
- 内容: facemap 変更時に face↔DrawInfo を (VB, index offset, count) 厳密一致で特定し mTexture を in-place 交換・rebuild 回避。
- **結果: pokeOK 2-3/window vs pokeFB 1908-2915/window(成功率 ~0.1%)** → gupd/rsnG/rsnGB 全て無変化。
- 不発原因の有力候補: **registerFace の batch 併合**(llvovolume.cpp:6848-6883 = 同 texture/material の連続 face を 1 DrawInfo 化)で単一 face 一致がほぼ成立しない。副候補 = components=0(未 fetch)ゲート。reason 別計測は未実施(OPEN)。
- 教訓: 入口(escalation 経路)を 1 本ずつ塞ぐ増分は whack-a-mole。構造(per-event group rebuild)を stepA 本線で解くべき。poke は revert 方針(AYA 裁定待ち)。

## 6. 信頼性系の観測(走行 1)
- cold burst 中に **vkAcquireNextImageKHR 11.2s hang → VK_ERROR_DEVICE_LOST**(clean 終了経路は正常動作)。同時観測: inflight geometry 501MB・draws/f 21818。発生は確率的(走行 2-6 では再現せず)。
- **有界化(AYASTORM_GEO_APPLY_BUDGET_MS・default 8ms)の欠陥 2 点**: ①最低 1 job は必ず apply = 単一巨大 job(実測 ~470ms/job)を分割不能 ②inline 経路(burst 時 197 件/frame)が予算対象外。defer カウンタ自体は稼働確認済。

## 7. 恒久計器の台帳(本日新設・通常起動コストほぼゼロ)
| 欄 | 内容 | 実装 |
|---|---|---|
| mlp(16 slot) | mainloop 未 scope 区間の会計(fence/acq/coro/mesh 等) | llvkloader + llappviewer scope |
| gds(24 slot) | GEOM_DIRTY set site 帰属(4.2 の表) | 各 set site に increment |
| rsnA/rsnG/rsnGB | rebuild 実行側の reason/bridge 帰属 | rebuildGeom |
| pokeOK/pokeFB | texture poke 発動率(poke revert 時に撤去) | setTE* 3 caller |

## 7.5 総括の訂正(設計ガバナンス・2026-07-22 深夜)
- 「前 TOP(geometry rebuild)= 前任の矮小化」という再スコープの言い分は**誤りだった**: 降格根拠の gupd=0.9ms/f は非フォーカス走行の汚染値。focused では gupd 12-30ms/f = **前任の標的選定は正しかった**。誤りは処方(burst 用有界化は steady churn に無力)と計測環境。
- step0 の 6 走行が最終的に返した答えは「geometry rebuild が本体(の一つ)」= 前任の結論への回帰 + 処方を書くための帰属精度(§4)。**回帰に 6 走行を要した根本原因 = 汚染データ(非フォーカス)を検証せず再スコープの根拠にしたこと**。計測プロトコル(フォーカス必須)は既知ルールだった = 適用漏れ。

## 8. OPEN(次に測る時はここから・再帰属は不要)
1. vol(markForUpdate)の caller 別頻度(:505/:588 SIM update 説は相関のみ)。
2. poke 不発の reason 分布(batch vs components)— poke revert なら不要。
3. sculpt streaming が min10+ でも続く理由(LOD 切替→asset 再 fetch の ping-pong 疑い・未証明)。
4. lighting misc 3.5ms/f の実体・shad 9ms/f の内訳(lgt 欄で一次分解済・設計書 §1.2)。
