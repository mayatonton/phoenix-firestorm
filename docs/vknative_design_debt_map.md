# 設計負債マップ — per-draw 束縛系の「設計が足りていない」所在(2026-07-18 実トレース)

- 目的: Phase 2 以前に潰すべき**構造欠陥**の台帳。全項目 HEAD(`7b15e591b9` + 契約層 v1)の file:line 実読で導出。症状ベースの推測は含まない。
- 深刻度: ★★★ = 現在の描画破壊に直結しうる / ★★ = 並列化で破裂する / ★ = 汚染源だが有界。

## D1. scene authority のカバレッジ穴(★★★)
**per-draw 所有権改修(T1-T6)の「ambient PULL 解消」は authority を呼ぶ pool にしか及んでいない。**
- `buildAndOverrideScenePerDrawSet` を呼ぶのは lldrawpool.cpp(pushBatch 系 3)+ lldrawpoolalpha.cpp(2)+ gltfscenemanager.cpp(2)のみ。
- **materials pool は呼ばない**(lldrawpoolmaterials.cpp:233 = 素の drawRange 直呼び)→ rigged 含む全 material draw が immediate PULL(`populateAndBindUniversalDescriptorSet` = ambient texunit 読み)で束縛される。avatar(llviewerjointmesh)・tree(face 直)・pipeline.cpp fullscreen 系・llspatialpartition デバッグ描画も同様。
- 帰結: 「協調層の ambient PULL は機能的に解消」(handoff)は **材料系 draw クラスには偽**。E2(materials bindless)が白で沈んだ土壌もここ。skinned material(髪)は populate の成否に生殺与奪を握られている。
- 修正方向: materials/avatar を scene authority 経由に載せる(pushBatch 系と同じ運び)か、immediate authority を正式な第二 authority として検証キャッシュ込みで承認する(percall map §3 の完遂)。**どちらかを選ぶ設計決裁が必要**。

## D2. populate/refresh の silent 失敗経路(★★★)
- populate の return 経路 6 本中 4 本が無音だった(contract v1 で全てに原因コード付与済): not-init / no-shader-or-layout / no-sampler / ubo-collect-overflow / ensure-fail / record-job。
- refresh(`vkRefreshDynamicOffsetsForDraw`)は UBO 解決失敗で **set を NULL に落として黙って return**(llglslshader.cpp:3104/3122/3137)→ reader は skip。
- 帰結: **draw が消える(髪が消える)とき、その理由はどこにも記録されなかった**。2026-07-18 16:22 JST の 1 分起動で total_skips=8192+ が実在(旧警告の実ログ)。原因内訳は契約層 v1 の初回実測で判明する。

## D3. populate と refresh の非対称 fallback(★★)
- populate 系 `vkCollectDynamicUBOWrites` は UBO 解決失敗を **arena buffer 代替で救う**(llglslshader.cpp:3061/3076)。
- refresh は同じ失敗で **諦めて set NULL**(= skip)。
- 同じ「UBO ring が引けない」が、経路により「描く(代替)」と「消える(skip)」に分岐 = 挙動が呼び順に依存。不変条件(失敗時の統一ポリシー)が未定義。

## D4. resolver の authored+refresh 失敗穴(★★★・P1 で入った可能性)
- `vkResolvePerCallSetForDraw`(llglslshader.cpp:3228〜): authored=true で set が dirty → refresh 失敗 → set NULL → **populate fallback へ落ちずに NULL を返す**(consume-clear 済のため)。
- 旧 reader パターン(P1 前)は「NULL なら populate」だったので refresh 失敗が populate で救われ得た。P1 の authored routing はこの回復経路を閉じた。scene draw(memo/per-shader cache hit 経路)で refresh が失敗すると **skip 直行**。
- ⚠️ ただし「baseline 破壊は `17874dc54b`(P1 以前)から」という AYA 実機報告と時系列が合わない = **これが主犯とは断定しない**。契約層の原因内訳(refresh_* が出るか)で判定する。

## D5. 「skip 系ゼロ」claim の反証(★★★・方法論)
- 前セッションの draw-failure 全数会計は「skip ゼロ ⟹ 破壊は fire-but-wrong 種」と結論した(handoff 記載)。
- **今日 16:22 JST の実ログは skip 8192+/分・Skinned Material Shader 29 で 5952-index の draw 不発火を記録**(binary = 16:19 build = HEAD 相当・3-path 同一 md5 確認済)。
- 帰結: 「fire-but-wrong」前提は再検証要。**skip 種の破壊は現存し、髪消失/ちらつきの症状と整合**。前計器が何を測り損ねたか(測定窓・シーン・build 差)は不明 = 契約層の実測で仕切り直す。

## D6. log の構造欠陥(★★・v1 で主要部を置換済)
- once-per-shader dedup(2 回目以降永久無音)・2 冪サンプリングに原因なし・「count=」が draw の index 数なのに累計に見える・provenance ゼロ・alpha run 全滅は完全無音・INFO 起動スパム ~3.5k 行が S/N を潰す。
- v1 処置: 発火点系警告を VKC(原因+出自+10 秒集計)に置換。残: 起動 INFO スパムの格下げ・他サブシステム(texture/geometry)の同型整理(未着手)。

## D7. 発火点の identity 捨て(★★・v1 で解消)
- `LLDrawInfo` は出自(LocalID/texture UUID/avatar/material UUID)を最初から持っているのに、発火点(LLVertexBuffer)は params を知らず匿名 draw だった。v1 の DrawScope + describer 注入で接続済。

## D8. immediate authority の検証キャッシュ不在(★★・percall map 既知・P3 中断中)
- `docs/percall_set_authority_map.md` の本題。scene は T3 で検証キャッシュ化済・immediate は P2(kill switch 既定 OFF)まで。~18 の散在 NULL poke が現役。
- 本マップ D1 の決裁(immediate を正式 authority にするか)と一体で再開すべき。

## 実測裁定(2026-07-18 夕・契約層の初回運用で同日決着)
- **D4/D5 = 実証・fix 済**: skip の原因は `authored_empty`(残存 poke が set を消すが authored flag を消さない)+ `refresh_shared_ubo` の 2 つのみ。resolver に populate 再導出の回復路を復元(llglslshader.cpp)→ 検証起動で全窓 skips=0・flicker=0・AYA 視覚 = 髪 OK。**破壊起点 = P1 `bce6f14ebe`**(「17874dc54b 起点」は P0/P1 同一 commit の混同で未確定だったと判明)。
- **D2 = 全 NULL 経路に原因コード付与済**(契約層 v1)。
- 残り: **D1(authority カバレッジ)・D3(fallback 非対称)・D8(poke 蒸発 = P3)は未治療**。authored_empty/refresh_shared_ubo は回復イベントとして毎窓 1〜3k 件観測されており、D8 の設計入力になる。

## 優先順(2026-07-19 更新・設計者)
1. ~~屋根の穴~~ **解決済**(fire-but-wrong 説は誤りだった = 正体は list 層の record 断絶・octree 転居 × worker 窓。commit `affd82db88`。fb_view_* は streaming 中の fallback ノイズで穴と無関係・無害)。
2. ~~契約層 commit~~ 済(`2823344ff4` 系列)。
3. **D1 = 案 A 決裁済・A1(materials `df489e2c52`)A2(avatar/tree `53eb84eb73`)済 → A3(D8 = poke 蒸発・進行中)→ A4(P4 gate + T7 閉じ)→ E2 clean 再開発**。D3 は髪 fix の resolver 回復路で構造閉鎖済。
4. v2(束縛内容契約)= 現時点で不要。必要が生じた arc で再検討。
