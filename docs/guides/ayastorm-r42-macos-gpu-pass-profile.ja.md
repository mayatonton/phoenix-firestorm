# AYAstorm R42 macOS: Metal System Trace による GPU 時間の内訳（2026-11-25）

MoltenVK を基準にした 3 OS 共通設計を見直すため、macOS 実機で 1 フレームの GPU 時間がどこに使われているかを計測した記録。

## 1. 条件

- 環境: macOS 15.7.7 / Apple M2 Pro / 同梱 MoltenVK 1.4.2
- app: `build-darwin-universal/newview/Release/AYAstorm.app`（arm64、2026-11-24 22:12 ビルド）。HEAD `83b7eeb45c` に、
  別作業の未コミット変更（パイプライン作成失敗のログ、present semaphore を swapchain 画像ごとにする変更、
  `AYASTORM_DISPLAY_TIMING` スイッチ）を含む。`scripts/bench/sign_dev_app.sh` で署名し直した。
- シーン: `docs/guides/ayastorm-r42-opensim-bench.ja.md` の busy store、NPC 40 体。起動は `launch_bench_viewer.sh`
  （`--novoice --noaudio`、`BackgroundYieldTime 0`）、`AYASTORM_PERF_LOG=5`、`AYASTORM_VKC=1`。
- 計測: `xcrun xctrace record --template 'Metal System Trace' --attach <pid> --time-limit 5s`。
  `metal-application-encoders-list` と `metal-gpu-intervals` を書き出して集計した。
  同時に実行中の別エンコーダーの GPU 時間は、重なっている区間を均等に割り振った。
- 診断用の Vulkan / DYLD override は使っていない。`AYASTORM_VKC=1` は表示時刻の照合のため。
- 別アプリ（Adobe Lightroom Classic）が GPU をわずかに使っていた。集計では AYAstorm の区間だけを数えた。

## 2. 結果

| 方式 | 新しいシーン /s | Metal フレーム /s | GPU 稼働率 | GPU 時間 / Metal フレーム | Metal エンコーダー / フレーム |
|---|---|---|---|---|---|
| 非同期（既定） | 約 14 | 48 | 57 % | 11.8 ms | 約 200 |
| 同期分割（`AYASTORM_ASYNC_SCENE=0`） | 約 25 | 25 | 74 % | 29.4 ms | 約 470 |
| 同期分割・カーテンなし（`busy_store_noveils.oar`） | — | 30 | 62 % | 20.5 ms | — |

同期分割の 1 フレームあたりのエンコーダー内訳（中ほどの 130 フレームの平均）:

| エンコーダー | 個数 / フレーム | GPU 時間 / フレーム |
|---|---|---|
| `vkCmdBeginRendering`（アプリが開始した描画） | 162 | 23.1 ms |
| `Metal renderpass restart`（MoltenVK による描画の再開） | 113 | 3.4 ms |
| `vkCmdDraw (convert indirect buffers)`（compute） | 113 | 1.3 ms |
| `Post-render-pass occlusion query accumulation`（compute） | 32 | 0.1 ms |
| `vkCmdCopyBuffer` / `vkCmdBlitImage` / その他の blit | 約 50 | 0.4 ms |

## 3. 分かったこと

| 状態 | 項目 | 根拠 |
|---|---|---|
| VERIFIED | GPU は飽和していない。非同期で 57 %、同期分割でも 74 %。残りはフレーム内の待ちで、GPU 律速だけでは説明できない | 2 章の GPU 稼働率 |
| VERIFIED | アプリ自身が 1 フレームに約 160 回描画を開始する。コードから静的に数えた 17 区間の約 10 倍 | エンコーダー数 |
| VERIFIED | MoltenVK の描画の再開（1 フレーム 113 回）は、すべて indirect draw の変換用 compute の直後に起きる。変換のたびに Metal の描画を終えて保存し、再開している | 再開直前のエンコーダーは 14,740 件すべて `convert indirect buffers` |
| VERIFIED | 再開はフレームの前半から後半まで広く分布する（中ほどのフレームでエンコーダー 597 個中 12〜407 番目） | エンコーダーの順序 |
| CLAIMED | MoltenVK 1.4.2 が indexed indirect draw を変換するのは、multiview、triangle fan、tessellation、uint8 index のとき。通常の indexed indirect は変換せず、描画は止まらない | `MVKCmdDraw.mm` の `MVKCmdDrawIndexedIndirect::encode`（ソースを読んだ内容。行番号は未確認） |
| OPEN | AYAstorm のどの描画が上の条件に当たるか。影は multiview で描いている（`llpipelineshadow.cpp:525`）が、再開は影の後のパスにも出ている | 描画にラベルがないため、パスを特定できない |
| VERIFIED | カーテン 12 枚を外すと GPU 時間が 29.4 → 20.5 ms に減り、9.2 ms かかっていた 1 個のエンコーダーが消えた。半透明のカーテン 1 パスがフレームの約 3 割を使っている | カーテンなしの計測 |
| VERIFIED | この計測中、`Material Shader` 12 種と `Skinned Material Shader` 12 種でパイプライン作成が失敗している | `AYAstorm.log` の `pipeline create failed`（48 行） |
| VERIFIED | 失敗の原因は Metal のシェーダーコンパイルエラー。失敗する 24 種は、`DIFFUSE_ALPHA_MODE_BLEND` 以外の（G-buffer 用の）Material シェーダーで、出力を `frag_data[4]` の 4 つ持つ。半透明パス（`LLDrawPoolAlpha`、カラー 1 枚）でこれを使うと、MoltenVK が変換した MSL で 2〜4 番目の出力に `[[color(n)]]` が付かず、`invalid return type 'main0_out' for fragment function` で失敗する。Vulkan では存在しないアタッチメントへの書き込みは捨てられるだけなので、Windows / Linux では起きない | ビューワーを直接起動して取得した MoltenVK の stderr（`MVK_CONFIG_LOG_LEVEL=2`、48 件）。失敗は全件 `color_count=1`、呼び出し元は `prepare_alpha_shader`（`lldrawpoolalpha.cpp:191-194`） |
| VERIFIED | 描画は欠けていない。半透明パスの描画は、登録時に必ず blend 版のシェーダーが割り当てられる（`llvovolume.cpp:6211` の `getShaderMask(DIFFUSE_ALPHA_MODE_BLEND, …)`。`mShaderMask` を書く箇所はここだけ）。失敗していたのは、半透明パスの開始時に Material シェーダー全 16 種へ uniform を設定する準備処理だけで、失敗した版で描く経路はない | コード |
| VERIFIED | 失敗したパイプラインの MSL が pipeline cache（`pipeline_cache.bin`）に保存され、次の起動で MoltenVK がその読み込み中に同じコンパイルエラーを出す。`vkCreatePipelineCache` が `VK_ERROR_INITIALIZATION_FAILED` を返し、キャッシュ全体が捨てられていた（起動のたびに全パイプラインを作り直す） | `AYAstorm.log` の `rejected cached data result=-3`。stderr の 24 件のエラーはすべてデバイス作成より前 |
| VERIFIED | 修正: 半透明パスの準備処理で blend 版だけを準備する（`lldrawpoolalpha.cpp` `<AYA:AlphaMaterialPrep>`）。修正後のビルドで、ベンチに 45 秒以上入った状態で `pipeline create failed` 0 件、MoltenVK のコンパイルエラー 0 件。2 回目の起動で pipeline cache が受け入れられた（`rejected cached data` 0 件）。VUID・device lost なし | 2026-11-25 の修正ビルド（`build-darwin-universal`、arm64） |
| OPEN | 修正の Windows / Linux でのビルド・動作確認。変更は共通コードだが、準備を省く版は半透明パスで使われないため挙動は変わらない見込み（CLAIMED） | — |
| VERIFIED | 同期分割の計測中、ログに VUID・device lost はない | `AYAstorm.log` |

## 4. 設計見直しへの意味

1. **描画の区切りが多すぎる。** アプリ側の 160 回と MoltenVK 側の 113 回を合わせ、1 フレームで約 270 回描画を
   開始している。Apple GPU ではそのたびにタイルメモリの保存と読み込みが起きる。
2. **multiview などの MoltenVK が変換する機能を、indirect draw と組み合わせると Mac では描画が分断される。**
   デスクトップで有効な最適化が、MoltenVK では逆効果になる例である。要件（基準プロファイル）は機能の有無だけでなく、
   MoltenVK が変換を挟む使い方も禁止事項として扱う必要がある。
3. **GPU 律速ではない時間が 26〜43 % ある。** フレーム制御（待ち方・submit の分け方）の見直しが GPU 時間の削減と同じくらい効く。
4. **Vulkan では正しくても Metal では通らないシェーダーと出力の組み合わせがある**（G-buffer 用の 4 出力シェーダーをカラー 1 枚の
   パスで使う）。今回は描画の欠けにはつながっていなかったが、pipeline cache を毎回捨てさせていた。Profiles layer では
   検出できないため、macOS の受入に「MoltenVK のコンパイルエラー 0 件」を含める。

## 5. 次の計測

- 描画ごとに `vkCmdBeginDebugUtilsLabelEXT` でパス名を付け、エンコーダーとパスを対応付ける（MoltenVK はラベルを
  Metal のデバッグ情報に渡す。対応の仕方は未確認）。
- multiview の影を無効にした A/B で、描画の再開が減るかを確かめる。
- 半透明カーテンのパスで、フラグメントシェーダーの費用（ローカルライト 64 個）か重ね描きかを切り分ける。
- 同じシーンを Windows / Linux で計測する（3 OS 共通の受入）。

集計に使ったスクリプトは計測時の作業領域に置いたもので、リポジトリには入れていない。
