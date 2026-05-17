# Render Alpha Performance Profile Test Plan

## 小修正の結論

今回のテスト実装は、`indra/newview/lldrawpoolalpha.cpp` の `renderPostDeferred()` にある material shader prepare loop を `LLMaterial::SHADER_COUNT * 2` から `LLMaterial::SHADER_COUNT` に縮める小修正である。

意図は、元コードが必要としていた「通常 material shader と rigged/skinned material shader の両方を alpha pass 用に準備する」動作を維持しつつ、`prepare_alpha_shader()` の `mRiggedVariant` 再帰によって起きていた rigged shader の二重 prepare だけを避けることにある。

`gDeferredMaterialProgram` は、前半に通常 material shader、後半に対応する rigged/skinned material shader を持つ。通常 shader には `mRiggedVariant` として対応する rigged shader が接続されているため、通常 shader を `prepare_alpha_shader()` に渡すと、その中で rigged shader も続けて準備される。

つまり、loop を `LLMaterial::SHADER_COUNT` までにしても、準備される対象は通常 shader だけにはならない。前半の通常 shader を起点に、対応する後半の rigged shader も `mRiggedVariant` 経由で準備される。修正で省くのは「loop 後半で同じ rigged shader をもう一度直接 prepare する処理」だけである。

描画時の shader 選択は変えていない。通常 mesh は従来通り通常 material shader を使い、avatar/rigged mesh は従来通り `mRiggedVariant` 側の shader を使う。したがって、この修正は描画対象や shader の種類を減らす変更ではなく、alpha pass 前の状態準備の重複を減らす変更である。

修正後 profile では、修正前の `Max avatars 5 + lights low` と比べて total、binds、Alpha/Material cost が下がっている。少なくとも今回の測定条件では、shader prepare/bind 由来の悪化は見えていない。

ただし、この小修正は alpha overdraw 本体を解決するものではない。Alpha OFF との差はまだ約 28.5 ms 残っているため、この小修正は採用候補として扱いつつ、次の調査対象は Alpha path の内訳である。同一条件で 2-3 回追加 profile を取り、平均とばらつきで確認してから最終判断する。

## 調査前提と負荷の切り分け結果

この調査は、Viewer の Frame Profile JSON を手動取得し、同一場所、同一視点、同一グラフィック設定に近づけて比較したものである。完全な自動ベンチマークではないため、avatar loading、周囲の描画物、カメラ位置の微差による揺れは残る。したがって、単発の profile は方向性の確認に使い、最終判断は同一条件の複数 profile で行う。

切り分け結果は以下である。

- 初期 all on は `218.277 ms` で、skinned/avatar 系 shader と Copy/Light 系が上位に出ていた。
- Avatar OFF では `52.881 ms` まで下がり、triangles も `14,635,039 -> 1,055,739` に減った。つまり最初の最大要因は avatar/skinned 描画である。
- Max visible avatars 5 では `102.754 ms` まで下がったが、まだ `Deferred PBR Alpha`、`Deferred Alpha`、`Deferred Emissive`、`Material Shader` が上位に残った。
- Light 設定を low にしても、`Max avatars 5 + lights low` は `101.690 ms` で、Max visible avatars 5 の `102.754 ms` とほぼ同等だった。今回の条件では light は主因ではない。
- Alpha OFF では `101.690 ms -> 28.104 ms` まで下がり、Alpha category は `28.738 ms -> 1.743 ms`、Material category は `35.316 ms -> 8.422 ms` まで下がった。

このため、負荷の優先順位は「まず avatar/skinned 描画が最大要因、その次に Alpha pass とそこに含まれる Material 系 shader が大きい」という判断になる。Alpha に手を入れた理由は、Alpha OFF で大きく改善したことに加えて、Alpha ON のまま avatar 数と light を抑えても Alpha/Material 系のコストが残ったためである。

ただし、Alpha OFF は透明物、半透明マテリアル、エフェクトなどを壊す診断用条件であり、製品上の対策ではない。そのため今回の小修正では、見た目を変える alpha 無効化ではなく、alpha pass の shader prepare/state setup の重複を減らす低リスクな箇所だけを対象にした。

## 目次

- [小修正の結論](#小修正の結論)
- [調査前提と負荷の切り分け結果](#調査前提と負荷の切り分け結果)
- [目的](#目的)
- [読み方](#読み方)
- [使用機能](#使用機能)
- [今回取得済みの測定結果](#今回取得済みの測定結果)
- [修正後の再テスト結果](#修正後の再テスト結果)
- [解釈](#解釈)
- [現在入れた小修正の検証対象](#現在入れた小修正の検証対象)
- [再テスト手順](#再テスト手順)
- [合格 / 判断基準](#合格--判断基準)
- [次のテスト実装候補](#次のテスト実装候補)

## 目的

macOS arm64 / OpenGL over Metal 環境での描画性能を調査し、特に Avatar、Light、Alpha のどれがフレーム時間を支配しているかを切り分ける。

このテスト計画では、Firestorm Viewer の Frame Profile JSON を使って、描画パス別・shader 別のコストを比較する。今回の主眼は、Avatar 表示数、Light 設定、Alpha 描画を個別に変えたときの total frame time、bind 数、triangle 数、sample 数の変化を確認することである。

## 読み方

- まず [小修正の結論](#小修正の結論) を読む。
- なぜ Alpha path を対象にしたかを確認する場合は、[調査前提と負荷の切り分け結果](#調査前提と負荷の切り分け結果) を読む。
- 測定結果の詳細を確認する場合は、[修正後の再テスト結果](#修正後の再テスト結果) と [解釈](#解釈) を読む。
- 再測定する場合は、[再テスト手順](#再テスト手順) と [合格 / 判断基準](#合格--判断基準) を使う。
- 次に実装・自動化する内容を決める場合は、[次のテスト実装候補](#次のテスト実装候補) を見る。

## 使用機能

Frame Profile は通常、以下のメニューから取得する。

1. Viewer を起動し、測定対象の場所へ移動する。
2. 同一カメラ位置、同一視点、同一グラフィック設定にする。
3. `Advanced -> Performance Tools -> Frame Profile` を開く。
4. profile JSON を保存する。

メニュー表記や配置は Viewer のビルド、設定、ローカライズで異なる可能性がある。見つからない場合は、`Advanced` メニュー内で `Performance`、`Profile`、`Frame` を含む項目を探す。Advanced メニュー自体が表示されていない場合は、Viewer の Advanced メニュー表示を有効化してから再確認する。

## 今回取得済みの測定結果

| 条件 | total | binds | triangles | samples | 主な上位コスト / 備考 |
| --- | ---: | ---: | ---: | ---: | --- |
| 初期 all on | 218.277 ms | 911 | 14,635,039 | 未記録 | Copy Shader 56.823 ms、Skinned Material Shader 28 15.705 ms、Skinned Material Shader 29 14.453 ms、Skinned Deferred Diffuse Shader 9.624 ms、Deferred MultiSpotLight Shader 9.426 ms |
| Avatar OFF | 52.881 ms | 554 | 1,055,739 | 未記録 | FPS 20-31。Avatar/skinned 系コストが消滅し、triangles は約 -92.8% |
| Max visible avatars 5 | 102.754 ms | 773 | 2,428,925 | 未記録 | Deferred MultiSpotLight 16.977 ms、Deferred PBR Alpha 8.363 ms、Deferred Alpha 7.736 ms、Deferred Emissive 6.423 ms、Material Shader 5 5.616 ms |
| lights low 参考 run | 参考扱い | 参考扱い | 参考扱い | 参考扱い | 一度 Max visible avatars を戻してしまった誤測定があるため、判断には使わない |
| Max avatars 5 + lights low | 101.690 ms | 791 | 2,559,723 | 68,753,826 | Light cost はほぼ消えるが、total はほぼ変わらない |
| Alpha OFF after max avatars 5 + lights low | 28.104 ms | 272 | 1,802,137 | 60,520,201 | Alpha 28.738 -> 1.743 ms、Material 35.316 -> 8.422 ms、total -73.586 ms |

## 修正後の再テスト結果

対象 build:

- Release build: `build-darwin-universal/newview/Release/AYAstorm.app`
- binary timestamp: `2026-05-12 18:55:26 JST`
- profile: `profile...t2026-05-12T10-06-21.json`
- 条件: Max visible avatars 5、lights low、Alpha ON

| 比較対象 | total | binds | triangles | samples |
| --- | ---: | ---: | ---: | ---: |
| 修正前 Max avatars 5 + lights low | 101.690 ms | 791 | 2,559,723 | 68,753,826 |
| 修正後 Max avatars 5 + lights low + Alpha ON | 56.616 ms | 540 | 2,170,287 | 67,382,279 |
| 差分 | -45.074 ms | -251 | -389,436 | -1,371,547 |

category 別の変化:

| category | 修正前 | 修正後 | 差分 |
| --- | ---: | ---: | ---: |
| Alpha | 28.738 ms | 18.130 ms | -10.608 ms |
| Material | 35.316 ms | 16.344 ms | -18.973 ms |
| Other | 20.857 ms | 9.189 ms | -11.669 ms |
| Post/Copy | 9.424 ms | 8.347 ms | -1.077 ms |
| Light | 3.301 ms | 2.261 ms | -1.040 ms |
| PBR | 3.836 ms | 2.210 ms | -1.626 ms |
| Environment | 0.218 ms | 0.136 ms | -0.083 ms |

上位 shader の改善は `Deferred PBR Alpha Shader`、`Skinned Material Shader 29`、`Material Shader 5`、`Deferred Emissive Shader`、`Deferred Alpha Shader`、`Deferred Fullbright Alpha Masking Alpha Shader` などに出ている。binds も 791 から 540 へ減っているため、今回の小修正が狙った state/driver overhead 低減と方向は合っている。

ただし、この 1 回だけで total -45.074 ms の全てを `renderPostDeferred()` の loop 修正に帰属させてはいけない。scene/camera/avatar loading の揺れが混ざる可能性があるため、同一条件で 2-3 回追加取得して平均とばらつきを見る。

Alpha OFF の 28.104 ms と比べると、修正後 Alpha ON はまだ 56.616 ms で約 28.5 ms の差が残る。Alpha path は改善したが、根本原因候補としては引き続き優先して調査する。

## 解釈

今回の結果では、Avatar 表示数と Alpha 描画が主要因である。

Avatar OFF では skinned/avatar 系の shader cost が消え、triangle 数も大きく減る。Max visible avatars 5 に制限すると total は大きく下がるが、それでも Alpha/Material 系のコストが残る。

Light 設定を low にすると個別の light shader cost は下がる。ただし、`Max avatars 5 + lights low` の total は `Max visible avatars 5` とほぼ同等であり、今回の測定条件では total 改善は小さい。

Alpha OFF は total を大きく下げるため、Alpha 描画が大きな要因であることの切り分けとして有効である。ただし、Alpha OFF は透明物、半透明マテリアル、エフェクトなどに大きな視覚的副作用があるため、最終対策ではなく診断用の条件として扱う。

## 現在入れた小修正の検証対象

検証対象は `indra/newview/lldrawpoolalpha.cpp` の `renderPostDeferred()` に入れた小修正である。

### 元コードの意図

`gDeferredMaterialProgram` は `LLMaterial::SHADER_COUNT * 2` 個の shader を持つ。前半は通常 material shader、後半は対応する skinned/rigged material shader である。shader 初期化側では、通常 material shader の `mRiggedVariant` に対応する skinned shader が設定される。

元の `renderPostDeferred()` は、この配列全体を loop して alpha 描画用の共通 uniform と状態を準備していた。これは「通常 material と rigged material の両方を、alpha pass 前に明示的に準備する」という意図として自然である。alpha pass では通常 mesh と avatar/rigged mesh の両方を描くため、どちらの shader も water plane、minimum alpha、display gamma、deferred environment などの alpha 用状態を持っている必要がある。

ただし現在の `prepare_alpha_shader()` は、渡された shader 自身を準備した後、`mRiggedVariant` が存在する場合にその rigged variant も再帰的に準備する。そのため、`gDeferredMaterialProgram` 全体を `LLMaterial::SHADER_COUNT * 2` で回すと、前半の通常 shader から後半の rigged shader を準備した後、loop 後半で同じ rigged shader をもう一度準備することになる。

今回の小修正は、元コードの「通常 material と rigged material の両方を alpha 用に準備する」という意図は残しつつ、`prepare_alpha_shader()` の rigged variant 再帰に任せて二重 prepare だけを避けるものである。描画時の shader 選択は引き続き、通常 mesh では `gDeferredMaterialProgram[mask]`、avatar/rigged mesh ではその `mRiggedVariant` を使うため、描画対象の種類は変えない。

将来 `mRiggedVariant` の接続方法や `prepare_alpha_shader()` の再帰処理を変える場合は、この loop 範囲も再確認する必要がある。

### 変更内容

- material shader loop を `LLMaterial::SHADER_COUNT * 2` から `LLMaterial::SHADER_COUNT` に縮める。
- rigged variant の二重 prepare を避ける。

この修正は alpha overdraw 本体の解決ではない。目的は、Alpha 描画の状態準備や driver overhead を少し減らせるかを確認することである。大幅な total 改善を期待する変更ではなく、shader prepare/bind 由来の小改善が出るかを Frame Profile で見る。

## 再テスト手順

1. 新しい Release build を作成する。
2. 既存起動中の Viewer を終了する。
3. 新しい Release build を起動し直す。
4. 測定済み run と同一場所へ移動する。
5. 同一カメラ位置、同一視点、同一グラフィック設定にする。
6. `Max visible avatars` を 5 にする。
7. Light 設定を low にする。
8. Alpha は ON のままにする。
9. `Advanced -> Performance Tools -> Frame Profile` から profile JSON を取得する。
10. 旧 `09-29-56` または `09-32-20` 系の profile JSON と比較する。

注意: 既存起動中のアプリは旧バイナリである。修正後の挙動を見るには、必ず Viewer を終了して新しい Release build を起動し直すこと。

## 合格 / 判断基準

- 視覚差分がない。
- クラッシュしない。
- Alpha ON、Max visible avatars 5、lights low の固定条件で Frame Profile を取得できる。
- shader prepare/bind 由来と考えられる小改善があるか確認できる。
- total、binds、Alpha/Material 系 shader cost を旧 `09-29-56` または `09-32-20` 系と比較できる。

大幅な改善が出ない場合は、この小修正だけで問題解決とは判断しない。その場合は alpha batching、描画分類、透明物ポリシーの見直しなど、次段階の対策へ進む。

## 次のテスト実装候補

- profile JSON 比較スクリプト。
- alpha shader/category 差分集計。
- 固定条件 checklist。
- 自動で最新 2 件の profile JSON を比較する補助ツール。
