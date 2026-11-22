# AYAstorm R42 3D シーン解像度スケーリング実機検証仕様

## 1. 文書の位置づけ

- 状態: 実機検証済み・凍結（3D シーン生成経路の改善待ち）
- 対象ブランチ: `feature/ayastorm-r42-render-resolution-scaling-validation`
- ベースブランチ: `feature/ayastorm-r42-macos-arm64-moltenvk-validation`
- 主対象: macOS Apple Silicon arm64 / MoltenVK
- 目的: 画質と引き換えに 3D シーンのレンダー負荷を下げ、FPS またはフレーム時間が改善するかを実機で判断する
- 製品採用: 保留
- ブランチ運用: 実機証拠と最小実装を保管するための凍結 branch。active development および merge candidate にはしない。

本仕様は製品機能の最終仕様ではない。実機で解像度スケーリングの動作と限定的な改善は確認したが、主施策としての採用・追加開発は、3D シーン生成経路の completion、`M:beg`、draw submission の改善が別途成立するまで保留する。

## 2. 必須要件

### 2.1 解像度を下げる対象

1. OS ウィンドウの論理サイズ、macOS の backing/drawable サイズ、および最終表示面のサイズは変更しない。
2. 解像度を下げるのは、最終表示面へ合成される前の 3D ワールドシーン用レンダーターゲットとする。
3. メニュー、フローター、テキスト、デバッグ UI、カーソルなどの 2D UI は最終表示面の解像度で描画し、3D シーンと一緒に低解像度化しない。
4. HUD アタッチメント、ネームタグ、選択表示、ワールド空間のデバッグ描画は既存の合成位置を調査し、低解像度側・ネイティブ解像度側のどちらに属するかを調査記録へ明記する。
5. 最終段では、低解像度の 3D シーンをウィンドウ全体へアップスケールしてから、または既存のネイティブ解像度 UI 合成経路内で表示する。

### 2.2 デバッグ設定

1. 既存 RLV 経路の調査結果に基づき、Debug Settings の単一設定 `RenderResolutionDivisor` を再利用する。
2. 実機検証で必須とする設定値と各軸の倍率は次のとおり。
   - `1`: ネイティブ解像度。機能無効時の基準値
   - `2`: 幅・高さをそれぞれ 1/2
   - `4`: 幅・高さをそれぞれ 1/4
   - `8`: 幅・高さをそれぞれ 1/8
3. divisor は幅・高さの両軸へ適用する。したがってピクセル数は、それぞれ概ね 1、1/4、1/16、1/64 となる。
4. 設定変更とウィンドウサイズ変更の双方で、対象レンダーターゲットを正しい寸法へ安全に再生成する。
5. `0`、過大な整数、極小ウィンドウ寸法を安全に扱い、クラッシュ、0 サイズ確保、範囲外 readback を起こさない。既存 RLV 互換性のため他の正整数 divisor は禁止しないが、本検証の比較値には含めない。
6. `RenderResolutionDivisor` の既定値は `1` とし、通常起動時の既存挙動と画質を変えない。互換目的で残す `RenderResolutionMultiplier` は本検証では `1.0` に固定する。
7. 一般設定 UI、動的解像度制御、自動 FPS 追従、アップスケーラーの品質選択は今回の対象外とする。

## 3. 既存 RLV 関連経路の調査と採否

既に存在するとされる RLV 関連の低解像度化機能について、設定の宣言からレンダーターゲット確保、3D 描画、最終合成、present までの呼び出し経路を追跡する。機能名や設定名を推測で確定せず、ソース上の参照を根拠に特定する。

最低限、次を `file:line` 付きで調査記録へ残す。

1. 設定の宣言、型、既定値、変更通知箇所
2. ウィンドウまたは drawable 寸法の取得箇所
3. 3D シーン用 color/depth/ID などのレンダーターゲット確保箇所
4. 低解像度ターゲットへ 3D シーンを描き始める箇所
5. ネイティブ解像度の最終表示面へ拡大・合成する箇所
6. UI、HUD、picking/readback、スクリーンショット、post-process が参照する寸法
7. macOS/MoltenVK 固有経路と、Windows/Linux でも共有される経路の境界

### 3.1 採用できる経路

次をすべて満たす場合、既存経路を今回の実装へ再利用してよい。

- 3D 描画開始前に、倍率を反映した小さい color/depth 等のターゲットを確保または選択している。
- 主要な 3D geometry、lighting、shadow 以外の主要 post-process が、意図した低解像度ターゲット寸法で実行される。
- ネイティブ解像度への拡大は 3D シーン描画後に 1 回だけ行われ、2D UI の鮮明さを維持できる。
- 実際のターゲット寸法をログ、GPU キャプチャ、または同等の実行時証拠で確認できる。

### 3.2 不採用とする経路

次のいずれかに該当する場合、そのままでは採用しない。

- 3D シーンをネイティブ解像度で完了した後に縮小しているだけである。
- UI 合成後の完成画像、present 対象、スクリーンショット画像だけを縮小・再拡大している。
- 単なるぼかし、ピクセル化、モザイク等の映像エフェクトで、主要 3D パスの処理ピクセル数が減らない。
- ウィンドウ自体、drawable、swapchain extent を縮小して OS に拡大させている。

既存経路が不採用の場合も削除・置換はせず、今回の目的を満たす最小の別経路を実装する。

## 4. 実装境界

1. 変更はデバッグ設定、3D シーン用ターゲット寸法、必要な座標変換、および実機検証ログへ限定する。
2. Darwin/MoltenVK 固有の処理は既存の Darwin/Metal ガード内へ置く。共有 renderer を変更する場合は Windows/Linux への影響箇所を列挙する。
3. color だけでなく、同じ画素座標系を前提とする depth、object ID、deferred、post-process の各ターゲットを整合させる。
4. picking/readback は、ウィンドウ座標を実ターゲット寸法へ変換する。既存の共有 helper がある場合は経路を確認し、Self/Other など一方だけを個別修正しない。
5. 1/8 倍率でも最小寸法、アラインメント、整数丸め、奇数ウィンドウサイズを安全に扱う。
6. divisor `1` では既存のネイティブ解像度経路と視覚・座標・ターゲット寸法が一致するようにする。
7. 本検証と無関係な renderer リファクタリング、GL fallback、swapchain/present cadence の仕様変更は行わない。

## 5. 実行時観測

設定変更時およびターゲット再生成時に、少なくとも次を同一のログ行または相互に対応づけられるログへ記録する。通常ログを恒常的に汚さないよう、既存の性能ログ条件または明示的なデバッグ条件へ接続してよい。

- 設定倍率
- ウィンドウまたは backing/drawable の幅・高さ
- 実際の 3D シーン用 color ターゲットの幅・高さ
- depth および object ID 等で異なる寸法がある場合はその幅・高さ
- ターゲット再生成の成否

ログ上の倍率指定だけを証拠にせず、実際に確保・描画したターゲット寸法を証拠とする。

## 6. 受け入れ条件

### 6.1 静的確認

- 既存 RLV 関連経路の調査結果と採否理由が `file:line` 付きで記録されている。
- divisor `1`、`2`、`4`、`8` が Debug Settings から入力できる。
- 3D シーン描画前に実ターゲット寸法が縮小されるコード経路を示せる。
- UI 合成、picking/readback、resize、ターゲット再生成の関連経路を確認している。

### 6.2 ビルド確認

- 既存の `build-darwin-universal/Firestorm.xcodeproj` から `viewer` の arm64 差分ビルドが成功する。
- 生成された `AYAstorm.app` の実行ファイルが arm64 である。
- app bundle の deep codesign 検証が成功する。
- MoltenVK Loader、MoltenVK、ICD の bundle 内配置を再確認する。

### 6.3 実機確認

同じログイン地点、カメラ、ウィンドウサイズ、描画設定を保ち、divisor `1`、`2`、`4`、`8` を比較する。

- ウィンドウの外形と最終 drawable 寸法が変わらない。
- 実際の 3D シーン用ターゲット寸法が各倍率と一致する。
- 3D シーンは段階的に粗くなる一方、2D UI はネイティブ解像度を維持する。
- resize 後および倍率のライブ変更後も黒画面、破損、クラッシュを起こさない。
- Self/Other の object picking、地形・オブジェクト選択など共有 readback 経路に明白な座標ずれがない。
- divisor `1` を基準に、viewer FPS、frame time、可能なら GPU frame time を同じ時間窓で記録する。
- FPS 改善が見られない場合も、CPU 律速、present 律速、shadow 等の別解像度パスなど、観測できた制約を記録する。

実機未確認の項目は静的確認やビルド成功で代替せず、`OPEN` と明記する。

## 7. 成果物

1. 本仕様を満たす最小のソースおよび設定変更
2. 本文書末尾の「調査結果」への経路・採否・共有プラットフォーム影響の追記
3. 本文書末尾の「検証記録」への実行コマンド、結果、未確認事項の追記
4. 必要な場合のみ、実機検証者向けガイドまたは既存ガイドへの最小追記

## 8. 調査結果

- **VERIFIED — 本命の RLV command path と採否:** RLV 有効化時に `RlvExtGetSet` が command handler として登録される（`indra/newview/rlvhandler.cpp:1555-1565`）。未知 behaviour の force/reply はその handler へ dispatch される（`indra/newview/rlvhandler.cpp:2883-2885`, `indra/newview/rlvhandler.cpp:3533-3535`）。`RlvExtGetSet` の allowlist は `RenderResolutionDivisor` を read/write で登録する（`indra/newview/rlvextensions.cpp:33-50`）。従って `@setdebug_RenderResolutionDivisor:2=force`（同様に `4`/`8`）は `processCommand()` → `onSetDebug()`（`indra/newview/rlvextensions.cpp:67-94`, `209-251`）で U32 Debug Setting を書換える。これが既存の RLV 関連低解像度化経路であり、**採用**する。`@setsphere` は既に作成済み scene image の fullscreen post-process（`indra/newview/rlvactions.cpp:403-406`, `indra/newview/llpipelinepost.cpp:2026-2036`, `indra/newview/rlveffects.cpp:357-395`）なので、解像度低下機構としては引き続き **不採用**。

- **VERIFIED — 設定・live resize と優先順位:** `RenderResolutionDivisor` は既定 `1` の U32 Debug Setting（`indra/newview/app_settings/settings.xml:14001-14013`）で、listener が `gResizeScreenTexture` を立てる（`indra/newview/llviewercontrol.cpp:798-802`, `1589-1591`）。cached setting は pipeline に取得される（`indra/newview/llpipelinealloc.cpp:315-321`, `1039-1044`）。検証 UI では `1`=native、`2`=各軸1/2、`4`=1/4、`8`=1/8 を入力する。既存の `RenderResolutionMultiplier` はそのまま残すが今回の操作対象にはしない。divisor が `>1` の場合は既存優先順位どおり multiplier より先に選ばれる（`indra/newview/llpipelinealloc.cpp:578-597`, `759-771`）。従って検証時は multiplier を既定 `1.0` に保つ。RLV の `@setdebug` lock 中は allowlist の write setting が Debug Settings editor から hidden になる（`indra/newview/rlvhandler.cpp:2467-2475`）ため、RLV が値を支配している実機比較は別ケースとして記録する。

- **VERIFIED — target と 3D/post-process:** raw WorldView 幅高から安全な divisor を決め、resize 比較と target allocation に同じ値を使う（`indra/newview/llpipelinealloc.cpp:150-171`, `572-608`, `746-771`）。multiplier を既定 `1.0` に保つ本検証では divisor `0`/`1` は native、極小 raw extent は effective divisor `1`、過大 divisor は小さい軸に合わせて clamp する。odd extent の割算は既存どおり整数切捨て。deferred color/depth、screen、deferredLight、shadow（`indra/newview/llpipelinealloc.cpp:777-799`）、scene-present、object ID、velocity/history、forward/post/SSR/water 等（`indra/newview/llpipelinealloc.cpp:815-938`）は縮小後の同一寸法で確保される。3D geometry は `deferredScreen` bind 後に始まる（`indra/newview/llviewerdisplay.cpp:1135-1175`）。最終 scene は raw WorldView viewport で `mScenePresentRT` を swapchain へ fullscreen copy（`indra/newview/llpipelinepost.cpp:1397-1423`）するため、drawable/swapchain extent を縮小しない。

- **VERIFIED — UI/HUD/picking/readback/snapshot の境界:** 2D UI と debug text は swapchain への scene copy 後（`indra/newview/llviewerdisplay.cpp:1793-1805`, `1823-1836`）。`RenderUIBuffer` 使用時も native raw 幅高で確保するよう変更した（`indra/newview/llpipelinealloc.cpp:804-812`）。HUD attachments、name tags、selection、world-space debug は `world_overlays()` 内で scene-present flush 前（`indra/newview/llviewerdisplay.cpp:1741-1791`, `1808-1817`）なので低解像度側に属する。self/other rigged picker は同一 `readObjectIDBufferLocalID()` helper を使う（`indra/newview/fsselfriggedpicker.cpp:128-210`）；helper は raw world 座標を実 object-ID target 座標へ変換する（`indra/newview/fsselfriggedpicker.cpp:81-124`）。`simpleSnapshot()` は snapshot 用 target を別途 native 出力寸法で確保するが（`indra/newview/llviewerwindow.cpp:6643-6651`）、scene target を一時再確保した後の restore 元を scaled `deferredScreen` ではなく `RenderTargetPack::width/height`（raw extent）へ修正した（`indra/newview/llviewerwindow.cpp:6667-6685`, `6776-6784`）。これで divisor を restore 時に再適用して一時的に二重縮小する不整合を避ける。snapshot 出力の画質・readback は実機確認が必要。

- **VERIFIED — 観測点と共有範囲:** `AYASTORM_PERF_LOG` が有効な再確保成功時、`#RenderResolution#` は `active_source`、requested/effective divisor、requested multiplier、raw world と deferred/scene/object ID/scene-present/UI buffer の実寸、および `result=success` を記録する（`indra/newview/llpipelinealloc.cpp:960-980`）。最初の確保と全 fallback が失敗した場合も、同じ性能ログ条件で requested/effective divisor、`raw_request` と `result=failure` を記録する（`indra/newview/llpipelinealloc.cpp:620-680`）。fallback 後の成功は既存 success 行で確認する。これにより divisor が有効でも multiplier 値だけを見て誤認せず、target 再生成の成否も観測できる。変更は共通 `indra/newview` renderer で Darwin 専用ではない。Windows/Linux も同じ allocation/picking translation unit を通るため、各 platform owner の build/runtime 確認は未実施。

## 9. 検証記録

- **VERIFIED — 静的検査:** `git diff --check` と `xmllint --noout indra/newview/app_settings/settings.xml indra/newview/app_settings/settings.AYA-merged.xml` を実行し成功。RLV allowlist → `@setdebug` U32 write → listener → raw target allocation → native swapchain copy、shared picker readback、および snapshot raw-extent restore を上記 file:line で確認した。

- **VERIFIED — 実機 target 再生成:** 通常起動した current local Release bundle の `~/Library/Application Support/AYAstorm-dev/logs/AYAstorm.log`（2026-08-14 16:46 JST 起動）で、`#RenderResolution#` の `result=success` を確認した。native は `2560x1368`、divisor `2` は `1280x684`、`3` は `853x456`、`4` は `640x342`、`8` は `320x171`、stress の `16` は `160x85`、native 復帰後は再び `2560x1368` である。開始時 window/swapchain は `2560x1387` で、切替中に変更していない。従って 3D target を設定変更で再確保し、window/drawable を縮小せずに scene 側だけを縮小・native へ復帰できたことを実機ログで確認した。今回、全 target allocation failure の実機発生、および RLV command/Debug Settings lock は未確認である。

- **VERIFIED — macOS arm64 build/bundle:** `DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer xcodebuild -project build-darwin-universal/Firestorm.xcodeproj -scheme viewer -configuration Release ARCHS=arm64 ONLY_ACTIVE_ARCH=YES build` は `** BUILD SUCCEEDED **`（exit 0）。`file build-darwin-universal/newview/Release/AYAstorm.app/Contents/MacOS/AYAstorm` は `Mach-O 64-bit executable arm64`、`lipo -archs` は `arm64`。`codesign --verify --deep --strict --verbose=2` は `valid on disk` / `satisfies its Designated Requirement`。bundle 内に `Contents/Frameworks/libvulkan.dylib`、`Contents/Frameworks/libMoltenVK.dylib`、`Contents/Resources/vulkan/icd.d/MoltenVK_icd.json` があり、ICD の `library_path` は `../../../Frameworks/libMoltenVK.dylib` と確認した。

- **OPEN — 残る実機機能確認:** 本ログは target extent、安定区間の性能、表示 cadence の観測であり、2D UI の native 解像度、HUD/world overlay の見え方、self/other picking、resize、snapshot 出力、黒画面・破損・クラッシュなしを網羅的に受け入れ確認した証拠ではない。`result=failure` の実機ログも未取得である。ビルドまたは静的検査を、これらの実機機能証拠の代替にはしない。

## 10. 実機ログ結果と凍結判断

### 10.1 証拠の範囲

- **VERIFIED — 実行条件:** 製品同等の通常起動（Vulkan/DYLD override および `--noprobe` なし）で、同一 window/swapchain のまま Debug Settings をライブ変更した。ログは `~/Library/Application Support/AYAstorm-dev/logs/AYAstorm.log` の 2026-08-14 16:46 JST 起動分であり、current local Release bundle の出力である。
- **USER OBSERVATION — 外部使用率:** CPU/GPU 使用率に大きな変化がなかったとの観察がある。ただしログ内に使用率の履歴はなく、対象 process も終了済みのため、独立した **VERIFIED** 証拠ではない。

### 10.2 安定区間の比較

| 区間 | scene target | FPS | frame time | fresh 3D scene |
| --- | ---: | ---: | ---: | ---: |
| native_pre | `2560x1368` | 19.55 | 51.16 ms | 6.52 |
| divisor 2 | `1280x684` | 25.31 | 39.51 ms | 8.43 |
| divisor 16 (stress) | `160x85` | 27.81 | 35.96 ms | 9.27 |
| native_post | `2560x1368` | 20.76 | 48.34 ms | 6.95 |

- **VERIFIED — 限定的な性能改善:** divisor `2` は native_pre 比で FPS `+29.5%`、frame time `-22.8%` だった。scene target の画素数は概ね 1/4 である。
- **VERIFIED — 解像度依存だけではない plateau:** divisor `16` は画素数が概ね 1/256 でも 27.81 FPS に留まる。divisor `3`/`4`/`8` は各 1〜7 秒しか保持しておらず、個別の安定比較には不十分である。

### 10.3 cadence とボトルネックの読み方

- **VERIFIED — 観測値:** consumer に対する fresh 3D scene は約 1/3、duplicate scene は約 66.7%、`ready1st=0` だった。ソース上でも UI/scene 非同期分割は有効（`indra/llrender/llvkloader.cpp:254-257`, `1139-1147`）で、producer completion は in-flight timeline 到達を確認している（`indra/llrender/llvkloader.cpp:1149-1177`）。consumer/producer の計測値は `uiscene` ログへ出力される（`indra/llrender/llvkloader.cpp:1494-1512`）。ただし、fresh scene が必ず約 3 consumer frame に 1 回となる理由は **OPEN** であり、ここから原因を断定しない。
- **VERIFIED — 描画計測:** draws/f は約 1900〜2020 で概ね不変で、通常の `SLOWFRAME` では `M:beg` が支配的だった。`M:beg` は CPU 使用率ではなく `beginFrame` の wall-clock 区間である。`present_wait_used=0`、`wait_attempts=0` のため、このログから物理 display cadence は断定しない。
- **INFERENCE — 次の調査対象:** 解像度非依存の scene generation、同期、command work が支配的である可能性が高い。この推定は上記 plateau、draws/f、cadence からの推論であり、CPU/GPU 使用率の証明ではない。
- **VERIFIED — 除外した診断値:** 4 件の `18446744073709... ms` `SLOWFRAME` は `gVkPerf.reset()` 後の U64 差分 underflow による診断バグであり（reset は `indra/llrender/llvkloader.cpp:1896`）、FPS/cadence/frame-time の結論から除外した。

### 10.4 凍結判断と解除条件

解像度スケーリング自体は要件どおり動作し、限定的な改善も得た。しかし、主施策としての採用または追加開発は保留する。この branch は凍結して保管し、3D scene producer completion、`M:beg`、draw submission などの改善が別途成立するまで active development/merge candidate にしない。

凍結解除と再評価には、少なくとも次を満たす。

1. producer が約 3 consumer frame に 1 回となる原因を計測し、改善する。
2. `M:beg` の subphase を計測可能にする。
3. 同一 scene/camera で native/`2`/`4`/`8` を各 60 秒程度測定する。
4. 外部 CPU/GPU 使用率を同じ時刻軸で同期記録する。
5. 上記後に本 branch を再評価し、§9 の未確認 UI/picking/snapshot 等も実機受け入れ確認する。
