# AYAstorm R42 macOS: display timing 診断ログ分析

## 結論

2026-08-14 に arm64 開発版 AYAstorm で採取したログから、次のことを確認した。

- 3D scene の更新を判定できた 159 区間では、Consumer と Present 成功は約 25.84 回/秒、
  MoltenVK から回収した drawable 表示完了履歴も約 25.84 件/秒だった。
- その表示完了履歴 20,764 件を現行コードの回収順で分類すると、`scene_id` が新しくなったものは
  6,934 件（33.4%）、同じ scene は 13,830 件（66.6%）だった。fresh scene は約 8.63 件/秒である。
- async Producer は約 8.56 回/秒であり、fresh scene の約 8.63 件/秒とほぼ一致した。
  「Consumer / Present は進むが、3D scene はおおむね 3 回に 1 回しか新しくならない」ことを、
  Present 受理回数だけでなく drawable 表示完了履歴まで対応付けて確認できた。
- ただし、現在ログに出る `actual_display_fps`（159 区間平均 16.74）は、全表示完了件数の
  cadence を正しく表す実装になっていない。この値を「ディスプレイ上の表示 FPS」として採用してはならない。
- MoltenVK の履歴返却順と `actualPresentTime` の時系列順は同一とは限らず、現行コードは並べ替えずに
  fresh / duplicate も判定している。従って 66.6% は繰り返し表示を示す強い証拠だが、時系列表示順での
  最終値として確定するには診断コードの補強と再 run が必要である。
- このログは、約 3 Consumer frame に 1 回しか新しい 3D scene が用意されない場所までを絞り込んだ。
  Producer 内部のどの処理が約 3 frame を必要としているかは、まだ分離できていない。

ここでいう表示完了は API 上の drawable 表示完了である。この文書は物理パネルの発光時刻を対象にしない。

## 1. 対象と証拠範囲

| 項目 | 確認結果 |
| --- | --- |
| 原本 | `~/Library/Application Support/AYAstorm-dev/logs/AYAstorm.log` |
| SHA-256 | `614afc31cb18365e713ddc53e5442ed8ac3d44fca5883b9c1f5f4cf3da1c0b01` |
| 実行時刻 | 2026-08-14 04:28:08Z--04:42:33Z |
| Viewer run time | 859.185 秒 |
| 実行環境 | Apple M2 Pro / macOS 15.7.7 / MoltenVK 1.4.2 |
| app 実行形式 | Mach-O 64-bit arm64（`file` と `lipo -archs` で再確認） |
| 診断初期化 | `display_timing diagnostic requested=1 extension=1 enabled=1` |
| 終了 | `status: stopped` |

ログは開発版用の Application Support root `AYAstorm-dev` と cache root
`AYAstorm-devOS_x64` を使用していた。cache root の末尾はこの run のログに記録された名称であり、
実行バイナリ自体は上表のとおり arm64 である。

診断を request するコード上の条件は `AYASTORM_VKC=1` かつ数値として
`AYASTORM_PERF_LOG>0` である。ログは環境変数の文字列値そのものを残さないため、値 `5` はログ単独では
再確認できない。一方、`requested=1 extension=1 enabled=1` と全区間の
`display_timing_available=1` は、診断が有効だった直接の実行証拠である。

この run では main / non-main display の別、表示モード、解像度を診断メタデータとして記録していない。
したがって、本書はメイン画面指定の影響を比較する資料ではない。

## 2. 何をどう集計したか

約 5 秒ごとに同じ `endFrame` から出る次の 3 行を一つの同期区間として扱った。

1. `uiscene consumer_fps=... producer_fps=...`
2. `frames=... cadence ... present_ok_fps=...`
3. `display_timing_available=... actual_display_count=... fresh_scene_display_count=...`

対応付けられた区間は 169/169 だった。最初の 10 区間（04:28:18Z--04:29:03Z）は
`fresh_scene_display_fps=n/a` で、3D scene 世代の比較開始前を含むため、主集計から除外した。
主集計は、Producer と fresh scene の両方を観測できた残り 159 区間
（04:29:08Z--04:42:27Z、換算 803.628 秒）である。

cadence は、区間件数を合計し、同じ区間の `frames / consumer_fps` から求めた時間合計で割った。
これにより、区間ごとの fps を単純平均した値と、完了件数全体から求めた値を混同しない。
fresh / duplicate 比率も区間平均ではなく件数合計から求めた。

## 3. 診断経路は正常に動作したか

| 項目 | 全 169 区間の結果 | 判定 |
| --- | ---: | --- |
| `display_timing_available=1` | 169 / 169 | 利用可能 |
| `display_timing_source=VK_GOOGLE_display_timing` | 169 / 169 | MoltenVK の表示履歴を使用 |
| `actual_display_count` 合計 | 26,113 | 表示完了履歴を回収 |
| `display_timing_history_query_errors` | 0 | 履歴照会エラーなし |
| `timing_map_drop` | 0 | 対応表からの破棄なし |
| `unknown_present_count` | 1 | 最初の区間だけ。主集計 159 区間では 0 |
| `timing_map_pending` | 最大 5 | 少数の履歴が次区間へ遅れて到着 |
| `present_wait_used` | 全区間 0 | `VK_KHR_present_wait` は未使用 |

`fresh + duplicate + unknown = actual` は主集計の全 159 区間で成立した。
従って、現行の回収順で得た 66.6% という duplicate 比率を mapping drop や unknown で説明することは
できない。ただし、後述する callback 回収順と実時刻順の問題は別に残る。

## 4. 3D scene を判定できた 159 区間

### 4.1 同じ時間窓での件数と cadence

| 観測点 | 合計件数 | 同期区間の件数 cadence | 意味 |
| --- | ---: | ---: | --- |
| Consumer frame | 20,765 | 25.839 回/秒 | UI と直近の front scene を合成する更新 |
| Present 成功 | 20,765 相当 | 25.839 回/秒 | `vkQueuePresentKHR()` が成功 |
| drawable 表示完了履歴 | 20,764 | 25.838 件/秒 | `actualPresentTime` を持つ履歴を回収 |
| fresh scene 表示完了 | 6,934 | 8.628 件/秒 | 直前と異なる `scene_id` |
| duplicate scene 表示完了 | 13,830 | 17.209 件/秒 | 直前と同じ `scene_id` |
| async Producer | 6,877 | 8.557 回/秒 | front 候補となる scene の生成側 |

Consumer frame と drawable 表示完了履歴は全区間合計で 1 件しか違わない。これに対し、現行の回収順では
fresh scene は drawable 表示完了履歴の 33.4%、duplicate scene は 66.6% だった。従って、本 run の
主な乖離が「Present した frame が完了履歴から大量に消えた」ためではないことは確かである。また、
Producer cadence との一致から、同じ 3D scene の再利用が主因である可能性は高い。

最後の 10 区間（約 50.276 秒）に限定しても、Consumer は 32.58 回/秒、drawable 表示完了履歴は
32.56 件/秒、fresh scene は 10.84 件/秒、duplicate は 66.71% だった。run 全体の平均だけで生じた
見かけの関係ではない。

### 4.2 最後の同期区間の例

04:42:27Z の同一 `endFrame` では、次の値が並んでいる。

```text
consumer_fps=31.4349 producer_fps=10.4783
frames=159 ... present_ok_fps=31.4349
actual_display_count=159 actual_display_fps=18.38
fresh_scene_display_count=53 fresh_scene_display_fps=10.46
duplicate_scene_count=106 unknown_present_count=0 duplicate_ratio=66.7%
```

同じ時間窓で Consumer 159 frame、表示完了履歴 159 件、回収順で fresh 53 件、duplicate 106 件である。
この区間だけでも、3 件の完了履歴に対して fresh 判定がほぼ 1 件という関係を確認できる。

## 5. `actual_display_fps` を表示 FPS として採用できない理由

上の例では、約 5 秒の同じ時間窓に Consumer 159 frame と表示完了履歴 159 件があるにもかかわらず、
ログの `actual_display_fps` は 18.38 になっている。これは単なる丸め差ではない。

現在の集計実装（`indra/llrender/llvkpresent.cpp`）は、履歴 1 件ごとに
`actual_display_count` を加算する一方、fps の分子
`actual_interval_count` と分母 `actual_interval_ns` は、`actualPresentTime` がそれまでの最大値を
上回った場合だけ加算する。つまり、この field が測るのは全表示完了履歴の cadence ではなく、
**取得順に見て時刻の最大値を更新した履歴だけの cadence** である。

`indra/llrender/llvkloader.cpp` は、この限定された分子と分母から `actual_display_fps` を作るが、
分子・分母そのものはログに出していない。

さらに、同梱 MoltenVK の固定コミットを確認すると、`MTLDrawable` の presented callback が
`actualPresentTime` を履歴リングへ追加し、`vkGetPastPresentationTimingGOOGLE()` はリングの先頭から
追加順に履歴を返す。AYAstorm は取得した配列を `actualPresentTime` 順に並べ替えていない。
callback の呼び出し順と drawable の実時刻順が異なれば、次の二つが同時に影響を受ける。

1. 最大時刻を更新した履歴だけを使う `actual_display_fps`
2. 取得順に直前の `scene_id` と比較する fresh / duplicate 分類

`VK_GOOGLE_display_timing` の履歴が複数件まとめて返る場合、現在のログには次の値がない。

- `actual_interval_count`
- `actual_interval_ns`
- 回収配列内で `actualPresentTime` が時系列順だったか
- 前区間から持ち越した履歴がどれだけあったか

そのため、ログに記録された 159 区間の `actual_display_fps` 平均 16.74、中央値 17.12 を
ディスプレイの実表示率とは結論できない。本書では、すべての完了履歴を数える
`actual_display_count` と同期区間時間から求めた 25.838 件/秒を、現在利用できる整合性確認値として使う。
これは集計窓あたりの完了履歴回収率であり、物理リフレッシュレートを意味しない。

`fresh_scene_display_fps` も同種の時刻差分集計を使うため、主結論では 8.64 というログ field の
区間平均ではなく、fresh 件数 6,934 / 同期区間 803.628 秒 = 8.628 件/秒を使用した。
ただし、この fresh 件数自体も callback 回収順で分類した値である。時系列表示順での確定値を得るには、
履歴を `actualPresentTime` で整列してから scene freshness を再判定する必要がある。

## 6. `presentMargin` の読み方

主集計 159 区間の `present_margin_ms_p50/p95/max` をさらに区間間で要約すると、次のようになる。

| ログ内の区間要約値 | 区間値の中央値 | 区間値の平均 | 観測最大 |
| --- | ---: | ---: | ---: |
| `present_margin_ms_p50` | 11.894 ms | 12.597 ms | 21.016 ms |
| `present_margin_ms_p95` | 26.219 ms | 26.346 ms | 47.951 ms |
| `present_margin_ms_max` | 31.985 ms | 36.214 ms | 417.592 ms |

これは raw 履歴全件の percentile ではなく、各約 5 秒区間で作った要約値を再度要約したものだ。
また MoltenVK は Metal で直接得られない情報を補うため `presentMargin` を推定している。
従って、417.592 ms の単発値だけから compositor や GPU を原因と断定しない。

## 7. 確定事項と未確定事項

### VERIFIED

1. 診断 extension は有効で、169 区間すべてで履歴照会に成功した。
2. 主集計では Consumer / Present 成功と drawable 表示完了履歴の回収件数 cadence は約 25.84 で一致した。
3. 現行の callback 回収順では、表示完了履歴の 66.6% が同じ `scene_id` に分類された。
4. その fresh 判定の件数 cadence 約 8.63 は async Producer の約 8.56 とほぼ一致した。
5. 現行 `actual_display_fps` field は全完了履歴を分子に使っておらず、値 16--17 を表示 FPS として
   採用できない。

### OPEN

- Producer が次の front scene を用意するまでに約 3 Consumer frame を必要とする内部原因。
- CPU command buffer recording、GPU timeline 完了、shadow、asset load などのどこが支配的か。
- main / non-main display、present mode、WindowServer の条件差。今回のログに比較条件がない。
- 回収された timing 履歴の時刻順と、現行 `actual_display_fps` が件数 cadence から外れる正確な内訳。
- `actualPresentTime` 順に分類し直したときの fresh / duplicate 比率。

## 8. 次の調査

1. 診断集計を補強し、`actual_interval_count`、`actual_interval_ns`、時刻の逆行件数をログに出す。
   複数履歴は `actualPresentTime` 順に並べ、cadence と scene freshness を計算し直してから、
   `actual_display_count / 区間時間` と照合する。
2. 同一 scene・同一視点で main / non-main display を明記した対照 run を採り、Consumer、完了履歴件数、
   fresh、duplicate、margin を同じ時間窓で比較する。
3. `scene_id` を Producer submit、GPU timeline signal、front 切替、Consumer snapshot まで追跡し、
   Producer 内部の CPU recording と GPU 完了待ちを分離する。
4. per-layer 独立 primary command buffer (`ONE_TIME_SUBMIT`) と prefill の変更を行う場合は、変更前後で
   Producer / fresh scene cadence が改善したかを同じ診断で比較する。

## 参照

- [VkPastPresentationTimingGOOGLE](https://registry.khronos.org/vulkan/specs/latest/man/html/VkPastPresentationTimingGOOGLE.html)
- [MoltenVK `MVKImage.mm`（ローカル package provenance が示す固定 commit）](https://github.com/KhronosGroup/MoltenVK/blob/4b715bdcb1f108f2003fffe126c389ad0cca1c90/MoltenVK/MoltenVK/GPUObjects/MVKImage.mm)
- [MoltenVK `MVKSwapchain.mm`（同 commit）](https://github.com/KhronosGroup/MoltenVK/blob/4b715bdcb1f108f2003fffe126c389ad0cca1c90/MoltenVK/MoltenVK/GPUObjects/MVKSwapchain.mm)
- [表示完了・3D scene freshness 診断の必要性](../specs/ayastorm-r42-display-completion-diagnostics-rationale.ja.md)
- [macOS: Viewer FPS と見た目の FPS が乖離する事象の調査記録](ayastorm-r42-macos-fps-divergence-investigation.ja.md)
