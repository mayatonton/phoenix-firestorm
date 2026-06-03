# AYAstorm: 読み込み中 FPS 低下と他メタバース比較調査

作成日: 2026-05-31  
対象ブランチ: `perf/ayastorm-loading-fps-investigation`

## 要約

「他のメタバースは読み込み中でも FPS が高く感じるのに、Second Life だけ極端に落ちる」という体感は、完全な錯覚ではない可能性が高い。

ただし、比較対象によっては「実世界を同じ密度で描画しながら高 FPS を維持している」のではなく、以下を組み合わせて高 FPS に見せている場合がある。

- ロード画面、ロビー、初期低 LOD、プレースホルダー、フォールバックアバターを先に描画する。
- 近傍だけを分割ロードし、遠方や重い要素をまだ描画しない。
- ダウンロード、デコード、依存関係解決を非同期化し、最終的な生成やインスタンス化だけを少しずつ行う。
- 重すぎるユーザー生成コンテンツを性能ランク、サイズ上限、モバイル制限、アバター非表示で制限する。
- FPS 表示が「ロード画面の FPS」であり、「完全に内容が出揃った世界の FPS」ではない。

Second Life/AYAstorm は、読み込み中でも実際のリージョン、アバター、オブジェクト、テクスチャ、シャドウ、LOD、UI、ネットワーク処理を同じメインループ上で進めるため、1 フレーム内にロード由来の処理が集中しやすい。特に macOS では OpenGL on Metal 経由のテクスチャ作成、FBO/PBO 系処理、シャドウ、状態ソート、ライティングがフレーム時間に乗りやすい。

## 調査した一次情報

### Unreal Engine

Epic の Level Streaming は、レベルを分割し、プレイ中に非同期でロード/アンロードし、表示状態を切り替える仕組みである。目的は巨大な世界を小さなチャンクに分け、必要な部分だけをメモリに置き描画すること。

出典: [Unreal Engine Level Streaming](https://dev.epicgames.com/documentation/en-us/unreal-engine/level-streaming-in-unreal-engine)

示唆:

- 「世界全体を一気にロードして描く」のではなく、必要領域だけに絞る設計。
- プレイヤーから見える範囲とロード範囲を設計側が管理できる。
- 正しく構成された場合、読み込みは体感上シームレスになりやすい。

### Unity / Addressables / AssetBundle

Unity Addressables は、キー解決、依存関係収集、リモート AssetBundle ダウンロード、メモリロードを非同期操作として扱う。Unity Support も、AssetBundle の同期ロードはメインスレッドをブロックしてヒッチを起こすため、`LoadAssetAsync` を使うよう説明している。ただし、非同期ロードしても GameObject の大量インスタンス化はヒッチになり得る。

出典:

- [Unity Addressables: Loading Addressable assets](https://docs.unity.cn/Packages/com.unity.addressables%401.19/manual/LoadingAddressableAssets.html)
- [Unity Support: Load assets asynchronously from AssetBundles](https://support.unity.com/hc/en-us/articles/207349003-How-can-I-load-Assets-asynchronously-from-AssetBundles-stored-in-disk)
- [Unity Addressables: Synchronous loading](https://docs.unity.cn/Packages/com.unity.addressables%402.2/manual/SynchronousAddressables.html)

示唆:

- 他アプリが高 FPS に見える場合、ロードの多くを非同期操作として処理している可能性が高い。
- ただし Unity でも最終的なシーン統合、生成、`WaitForCompletion`、大量インスタンス化はメインスレッドを止め得る。
- つまり「Unityだから絶対にロード中も滑らか」ではない。実装が悪いと普通に落ちる。

### Roblox

Roblox の Instance Streaming は、Workspace 内の 3D コンテンツやインスタンスを動的にロード/アンロードする仕組み。近傍や必要箇所を優先して、クライアント上の存在量を減らす。

出典: [Roblox Creator Hub: Instance streaming](https://create.roblox.com/docs/workspace/streaming)

示唆:

- 見えていない、または優先度が低いインスタンスをクライアントから外せる。
- 読み込み中に全部を保持、更新、描画しないため、体感 FPS を維持しやすい。

### VRChat

VRChat はユーザー生成アバターを許容する一方で、Avatar Performance Rank、Minimum Displayed Performance Rank、Fallback avatar、モバイル向け制限を持つ。一定条件ではアバター全体をフォールバックに置き換えたり、ライト、パーティクル、PhysBone、Constraint などを削除して表示する。

出典: [VRChat Creator Docs: Avatar Performance Ranking System](https://creators.vrchat.com/avatars/avatar-performance-ranking-system/)

示唆:

- 「重いものをそのまま全部出す」のではなく、重いコンテンツを非表示、置換、部分削除できる。
- 高 FPS に見える場合、実際には重いアバターやコンポーネントが表示されていない可能性がある。
- PC では既定が緩いが、ユーザー設定やモバイルではより強い制限が入る。

### Godot

Godot の Background loading は、ロード処理を `poll()` で段階的に進め、1 フレーム内でブロックする時間を制御する設計例を示している。例ではロード画面を表示しながら、一定時間だけ処理を進める。

出典: [Godot Docs: Background loading](https://docs.godotengine.org/en/3.0/tutorials/io/background_loading.html)

示唆:

- 単純な同期ロードではなく、ロード処理をフレーム間に分割するのが一般的な対策。
- 「ロード画面が滑らか」なのは、ロード対象の世界を完全描画しているからではなく、表示するものを軽くし、ロード処理に時間予算を与えているからでもある。

### Second Life Viewer

Second Life Wiki の Image System は、レンダーパイプラインが必要テクスチャ、discard level、優先度を決め、Texture Fetcher がキャッシュ、ネットワーク、受信、キャッシュ保存、デコードを行い、最終的にデコード済みテクスチャを OpenGL に渡す流れを説明している。Viewer Architecture では、メインスレッドが入出力メイン処理とレンダリングを担うと説明されている。

出典:

- [Second Life Wiki: Image System](https://wiki.secondlife.com/wiki/Image_System)
- [Second Life Wiki: Viewer Architecture](https://wiki.secondlife.com/wiki/Viewer_Architecture)

ローカルコード上でも、現在の AYAstorm では以下がメインループ内にある。

- `LLAppViewer::idle()` から `gObjectList.update(gAgent)` と `LL::GLTFSceneManager::instance().update()` を毎フレーム実行する。
  - `indra/newview/llappviewer.cpp`
- `LLViewerObjectList::update()` が active object の `idleUpdate()` を走らせる。
  - `indra/newview/llviewerobjectlist.cpp`
- `display()` 内で `gPipeline.generateSunShadow()` と `LLVOAvatar::updateImpostors()` が走る。
  - `indra/newview/llviewerdisplay.cpp`
- `LLViewerTextureList::updateImagesCreateTextures()` がデコード済み画像から GL texture を作る。
  - `indra/newview/llviewertexturelist.cpp`

## なぜ他メタバースは高 FPS に見えやすいのか

### 1. そもそも「ロード中に描いているもの」が違う

多くの現代エンジンでは、ロード画面、ロビー、プレースホルダー、低 LOD、フォールバックを先に出す。これは高 FPS に見えるが、実際には重い本体コンテンツをまだ描画していない。

Second Life はログイン直後から実リージョンに入る。灰色や未ロード表示であっても、オブジェクト更新、アバター更新、テクスチャ優先度計算、LOD、ネットワーク、描画パスが同時に走る。したがって「見た目は読み込み中」でも内部処理はかなり重い。

### 2. チャンク単位のストリーミング設計が違う

Unreal の Level Streaming や Roblox の Instance Streaming は、世界を分割し、必要箇所だけをロード/表示する前提が強い。

Second Life も可視性、距離、テクスチャ discard、object interest などは持つが、ユーザー生成リージョンに後付けで到着し、動的アバターや動的オブジェクト、スクリプト、テクスチャを一斉に受ける構造である。ゲームエンジンのように事前ビルドされたチャンクと依存関係を完全に制御できない。

### 3. 重いユーザー生成コンテンツを隠す仕組みの差

VRChat は Performance Rank や Fallback avatar で、重いアバターの表示を制限できる。Roblox も体験側、エンジン側でストリーミング範囲や存在量を制御しやすい。

Second Life は歴史的に「そこにあるものをできるだけ表示する」方向が強い。Avatar Complexity や impostor はあるが、VRChat のフォールバック置換ほど強く統制された体験にはなりにくい。

### 4. 非同期ロードでも最後の適用はメインスレッドに戻る

Unity の資料でも、非同期ロードはメインスレッドブロックを避けるための手段だが、大量インスタンス化や同期完了待ちはヒッチになるとされている。

SL/AYAstorm でも、ネットワーク受信や画像デコードに worker thread があっても、最終的な GL texture 作成、描画リソース更新、object idle、shadow/impostor 更新はメイン/レンダー側に戻る。ここが積み上がると FPS は落ちる。

### 5. macOS OpenGL on Metal の影響

AYAstorm の実測では、AppleMetalOpenGLRenderer 経由のパスがサンプルに出ていた。OpenGL API 自体は旧来の即時/状態機械的な設計で、Metal 上ではドライバ層が変換する。テクスチャ作成、readback、FBO/PBO、状態変更、同期点があると、フレーム時間に出やすい。

これは「SL だけが設計ミス」というより、古い OpenGL レンダラーを現代 macOS 上で動かす時の構造的コストも含む。

## 勘違いの可能性

以下の場合、「他メタバースは読み込み中でも高 FPS」と感じても、同条件比較ではない可能性がある。

### ロード画面の FPS を見ている

ロード画面は軽い UI や簡単な 3D 背景だけを描く。高 FPS でも、実際のワールド、全アバター、全テクスチャを描いているわけではない。

### 未ロードを非表示にしている

Roblox や VRChat では、近傍だけ、許可されたアバターだけ、性能制限を通った要素だけが表示されている可能性がある。Second Life のように「読み込み中の不完全な世界」をそのまま見せているわけではない。

### FPS 表示の計測対象が違う

メニュー、ロビー、ロード画面、ワールド内、混雑イベント、初回訪問、キャッシュ済み再訪問では意味が違う。比較するなら以下を揃える必要がある。

- 初回訪問かキャッシュ済みか
- 視界内アバター数
- テクスチャ総量
- GPU/CPU/メモリ状況
- ロード画面か実ワールド描画か
- FPS の取得元と平均化方法

### 他アプリも条件次第では落ちる

Unity の資料が示す通り、非同期ロードでも最終的な生成や同期待ちはヒッチを起こす。VRChat も公式ヘルプで、ロード中コンテンツやアバターが重いとパフォーマンス問題になることを説明している。つまり「他は常に高 FPS」は正しくない。

## AYAstorm 実測から見えたこと

今回の暫定計測では、読み込み中 FPS が 8 から 10 前後まで落ちた区間で、以下が主にフレーム時間を消費していた。

- `textures_avg_ms`: GL texture 作成/ダウンスケール由来のスパイク。
- `objectlist_avg_ms`: active object の idle update。
- `statesort_avg_ms`: 描画状態ソート。
- `lighting_avg_ms`: deferred lighting。
- `shadows_impostors_avg_ms`: sun shadow と avatar impostor 更新。
- `stream3d_avg_ms`: 0.02 から 0.07ms 程度で、今回の主因ではなかった。

テクスチャ処理にフレーム予算を入れた後、`textures_avg_ms` は大きく下がった。一方、FPS がまだ落ちる区間では `objectlist`、`lighting`、`statesort`、`shadows_impostors` が残った。これは「単一犯」ではなく、ロード中に複数のメインループ処理が同時に増える問題と見るべき。

## 仮説

### 仮説 A: SL はロードと描画の分離が弱い

SL は worker thread を使っているが、最終的に描画可能な状態へ持っていく処理がメイン/レンダー側に多く戻る。これがロード中 FPS 低下の直接原因になっている可能性が高い。

検証方法:

- `textures`, `objectlist`, `statesort`, `lighting`, `shadows_impostors` の各平均/最大 ms をログ化する。
- それぞれを個別に時間予算化し、FPS とロード完了時間のトレードオフを見る。

### 仮説 B: 他メタバースは「表示しない」ことで滑らかに見せている

他メタバースは、未ロードのコンテンツ、遠方、重いアバターを一時的に表示しない、または簡略表示する。SL はそれらを早く実表示しようとしてフレームを失っている可能性がある。

検証方法:

- SL 側で、ログイン直後のみ一時的に avatar impostor、shadow、遠方 object idle、texture create を強く制限する。
- FPS が保たれるか、ただし見た目の完成がどれだけ遅れるかを測る。

### 仮説 C: ユーザーの体感は一部バイアスを含む

ロード画面やロビーが滑らかな体験と、SL の実リージョン読み込みを比較している可能性がある。

検証方法:

- 比較対象を「ワールド内に入った瞬間から、周囲が完成するまで」に固定する。
- キャッシュを消した初回訪問と、キャッシュあり再訪問を分ける。
- 1% low FPS、最低 FPS、ロード完了までの秒数を同時に取る。

## 修正方針

行き当たりばったりに複数箇所を同時に変えない。以下の順で、1 仮説につき 1 変更だけを検証する。

### 2026-06-02 計画レビューで見つかった穴

現時点の実装方針は方向性として妥当だが、以下を明示しないまま進めると、FPS は一時的に上がってもロード完了遅延、見た目の残留、別機能の劣化を見逃す危険がある。

1. guard 発動条件が `gFPSClamped < AYALoadingFPSGuardMinFPS` だけだと、すでに低 FPS になった後でしか効かない。
2. ログイン直後、テレポート直後、カメラ大移動、視界内 avatar 急増などの「ロード予兆」を別トリガーとして扱う必要がある。
3. guard にヒステリシスがないと、24fps 付近で有効/無効が頻繁に切り替わり、逆にフレーム pacing を悪化させる可能性がある。
4. texture, object idle, shadow, impostor, callback を同時に絞ると、どれが効いたか分からなくなる。
5. visual な印象だけでは texture cap の効果判定ができない。ログで discard/downscale/queue/fps を見る必要がある。
6. 「メモリが下がらない」だけで失敗判定しない。GL/Metal driver cache のため、RSS は遅れて変わるか、変わらないことがある。
7. AutoTune との相互作用を分けて測る必要がある。AutoTune on での改善を AYA guard の効果と誤認しない。
8. 起動が `initStrings()` で止まる問題は、FPS guard の効果検証とは別の blocker として扱う。

### 追加すべき設計条件

実装を続ける場合、以下を前提条件にする。

- guard は `低 FPS` と `ロード直後時間窓` の両方で発動できるようにする。
- guard 解除は即時ではなく、FPS が回復してから数秒待って段階的に戻す。
- 各 Phase は debug setting で単独 on/off できるようにし、A/B 比較できる状態を維持する。
- 低 FPS guard 中でも P0/P1 は止めない。入力、カメラ、UI、近傍衝突、近傍 avatar の存在把握を優先する。
- queue が増え続ける場合は成功ではない。平均 FPS が上がっても、texture create/downscale/callback queue が発散するなら設定を戻す。
- ログは最低 120 秒取り、最初の 30 秒だけで結論を出さない。

### 現状実装の位置付け

現時点のコードは、厳密には「ロード専用 FPS guard」ではなく「低 FPS を検知して一部処理を絞る low-FPS throttle」である。

理由:

- 発動条件が主に `gFPSClamped < AYALoadingFPSGuardMinFPS` であり、低 FPS になった後にしか効かない。
- ログイン直後/テレポート直後の時間窓 trigger がまだない。
- guard 解除のヒステリシス、段階解除、guard 状態ログがまだない。
- texture, callback, object idle, shadow, impostor が同じ FPS しきい値で同時に切り替わる。

したがって、Phase 1 から Phase 3 の現状実装は検証用の throttle として扱う。Phase 4 の時間窓 trigger、ヒステリシス、段階解除、guard 状態ログが入るまで、通常配布へ「loading FPS guard」として採用しない。

### 採用/不採用の共通基準

採用条件:

- 最低 FPS と 1% low FPS が改善する。
- `textures`, `objectlist`, `shadows_impostors`, `statesort`, `lighting` のどれが改善したかログで説明できる。
- ロード完了時間の悪化が許容範囲内である。
- queue が継続的に増え続けない。
- AutoTune off の状態でも効果が確認できる。
- callback 最古待ち秒数、object idle 一巡秒数、shadow/impostor stale 秒数が許容範囲内に収まる。
- 近距離看板、装着物、HUD 以外の UI 的な面、mouselook 時の自分の装備が読める。

不採用または再調整条件:

- 平均 FPS は上がるが、ロード完了が極端に遅くなる。
- texture callback が詰まり、灰色表示や未完成 avatar が長時間残る。
- 非 avatar object idle の間引きで animated texture、media surface、script-driven visual の破綻が出る。
- shadow/impostor が古いまま残り、ユーザーが移動判断を誤るレベルになる。
- 距離 cap により近距離看板、UI 的な面、ショップ商品などの可読性が落ちる。

### 必須ログ項目

現在の `AYAPerf` と `AYATextureCap` だけでは、guard の副作用を十分に判定できない。次の実装/検証前に、少なくとも以下をログへ追加する。

- guard: `guard_active`, `guard_reason`, `enter_count`, `exit_count`, `duration_sec`, `login_or_teleport_elapsed_sec`
- texture create/downscale: queue 現在値、queue 最大値、可能なら最古 item 待ち秒数
- texture callback: callback queue 現在値、processed、skipped、time_limit_hit、count_limit_hit、最古 callback 待ち秒数
- distance cap: `faces`, `clamped_faces`, `aux_faces`, face 数上限で cap 対象外にした件数、大型 face/中心距離による cap 件数
- object idle: active non-avatar 数、updated、visited、skipped、cursor 一巡推定秒数
- shadow: updated/skipped、使用 interval、guard による skip か low FPS throttle による skip か
- impostor: candidates、updated、skipped、最古更新待ち秒数、近距離/遠距離別件数
- avatar loading: visible avatar 数、gray/cloud/loading avatar 数、fully baked 到達数、attachment 数
- AutoTune: on/off、strategy、変更された `RenderFarClip`, `RenderAvatarMaxART`, `RenderAvatarMaxNonImpostors`

検証ビルドではログ既定 ON でよいが、通常配布へ入れる場合は `AYAPerf*Log` と `AYATextureDistanceCapLog` の既定値を OFF に戻し、検証時だけ有効化する。

### ロールバック設定

実機で副作用が強い場合は、コードを戻す前に以下の setting で機能単位に切り分ける。

| 対象 | 無効化 setting |
| --- | --- |
| loading guard 全体 | `AYALoadingFPSGuardEnabled = false` |
| 3D 内部解像度の自動低下 | `AYALoadingDynamicResolutionEnabled = false` |
| 通常操作中の低 FPS だけを理由にした 3D 内部解像度低下 | `AYALoadingDynamicResolutionLowFPSEnabled = false` |
| avatar loading particle 停止 | `AYADisableAvatarLoadingCloudParticles = false` |
| 距離 texture cap | `AYATextureDistanceCapEnabled = false` |
| 補助マップ cap | `AYATextureDistanceCapAuxEnabled = false` |
| texture create 制限 | `RenderTextureMaxCreatesPerFrame` / `RenderTextureGuardMaxCreatesPerFrame` を大きくし、`RenderTextureCreateTimeSliceMs` / `RenderTextureGuardCreateTimeSliceMs` も上げる |
| texture downscale | `RenderTextureMaxDownscalesPerFrame = 0`, `RenderTextureGuardMaxDownscalesPerFrame = 0` |
| texture callback 制限 | `RenderTextureMaxCallbacksPerFrame` / `RenderTextureGuardMaxCallbacksPerFrame` を大きくし、`RenderTextureCallbackTimeSliceMs` / `RenderTextureGuardCallbackTimeSliceMs` も上げる |
| object idle throttle | `RenderObjectIdleMinFPS = 0` |
| shadow throttle | `RenderSunShadowMinUpdateFPS = 0` |
| impostor throttle | `RenderAvatarImpostorMinUpdateFPS = 0` |
| impostor per-frame 上限 | `RenderAvatarMaxImpostorUpdatesPerFrame = 0` |

注意:

- `RenderObjectIdleMinFPS = 0`, `RenderSunShadowMinUpdateFPS = 0`, `RenderAvatarImpostorMinUpdateFPS = 0` は低 FPS 単独 throttle を止めるだけで、`AYALoadingFPSGuardEnabled` 側の発動は残る。
- 個別 throttle を完全に止めて A/B 比較する場合は、`AYALoadingFPSGuardEnabled = false` か `AYALoadingFPSGuardMinFPS = 0` も併用する。
- `RenderAvatarMaxImpostorUpdatesPerFrame` は FPS しきい値とは別に通常時も効く。impostor 更新数制限を完全に戻す場合は `0` にする。
- `RenderTextureMaxDownscalesPerFrame` と `RenderTextureGuardMaxDownscalesPerFrame` は、現実装では非ゼロ値を最低 `4/frame` に丸める。`1` から `3` を指定しても実効値は `4` になる。
- downscale time slice も現実装では最低 `2.0ms` として扱う。settings の説明もこの実効挙動に合わせて更新する必要がある。
- texture create/callback は個数上限と time slice の両方で止まるため、片方だけ上げても切り戻しとして不十分な場合がある。

## 読み込み中の表示優先順位

ロード中にすべてを同じ優先度で表示しようとすると、フレーム時間が破綻する。AYAstorm では「ユーザーが操作を続けるために必要な情報」と「見た目の完成度」を分け、以下の順で表示/更新するのが妥当である。

### P0: 操作と安全に必要な情報

最優先。これが遅れると、FPS が出ていても操作不能に感じる。

- 自分のアバター位置、向き、カメラ、入力反応。
- 地形、床、壁、階段、衝突判定に関わる近傍オブジェクト。
- 移動先の足元と進行方向の低 LOD 形状。
- UI、チャット、通知、ログイン/テレポート状態。
- 最低限の近傍 avatar 位置と名前表示。

方針:

- 低 LOD、単色、未テクスチャでもよいので、位置と形状を先に出す。
- P0 はフレーム予算を削りすぎない。ここを間引くと体感が悪化する。

### P1: 近傍の存在理解に必要な情報

次点。世界の構造や人の存在を把握するために必要。

- 近距離の建物、家具、ドア、椅子、イベント会場の主要構造物。
- 近距離 avatar の基本姿勢、シルエット、impostor/簡易表示。
- 看板や案内など、行動判断に影響する近距離テキスト/面。
- 近距離オブジェクトの中解像度テクスチャ。

方針:

- まず低解像度テクスチャで表示し、詳細テクスチャは後回しにする。
- avatar は完全表示より、低コストのシルエット/簡易 impostor を優先する。

### P2: 見た目の品質を上げる情報

FPS が安定してから進める。ロード中に急いで入れるとヒッチの主因になる。

- 高解像度テクスチャ。
- マテリアル、normal/specular/PBR 補助情報。
- 遠景オブジェクトの詳細 LOD。
- avatar の詳細 attachment、複雑な alpha、髪、装飾。
- animated texture、particle、media surface。

方針:

- FPS が閾値を下回る間は、作成数と処理時間を強く制限する。
- 近距離でも「操作に不要な装飾」は P2 に落とす。

### P3: 後回しにしてよい演出

ロード中は積極的に遅延または停止する。完成後に段階的に戻す。

- sun shadow map の頻繁な更新。
- reflection probe / mirror 系の更新。
- 遠距離 avatar impostor の高頻度再生成。
- 高品質 lighting の追加パス。
- 遠距離オブジェクトの texture downscale / upscale 調整。

方針:

- FPS が低い間は、既存結果を使い回す。
- 数フレームから数十フレーム単位で間引く。
- 見た目の古さより、入力応答と最低 FPS を優先する。

### P4: 完全表示のための仕上げ

ロード完了後、または FPS が十分に戻った後で実行する。

- 遠景の高解像度化。
- 全 avatar attachment の完全表示。
- shadow/reflection/impostor の通常頻度復帰。
- texture discard の最終調整。
- render complexity 表示や詳細統計の更新。

方針:

- FPS が安定してから段階解除する。
- 一気に解除せず、数秒単位で戻す。

### 実装上の優先ルール

ロード中のスケジューラは、単純な FIFO ではなく以下の重みを使うべきである。

1. 距離: 近いものを先に出す。
2. 画面占有率: 大きく見えているものを先に出す。
3. 操作影響: 衝突、移動、クリック対象を先に出す。
4. 人の存在: 近距離 avatar の位置とシルエットを先に出す。
5. 視線方向: カメラ正面を優先し、背後は落とす。
6. コスト: 高コスト要素は分割し、1 フレームに集中させない。
7. 完成度: 高解像度、影、反射、装飾は最後に回す。

重要な訂正:

- 距離別 texture cap は「近いものを先に読み込む」実装ではない。これは既に face/texture stats に乗った texture の要求解像度を下げる処理であり、fetch/decode/create/callback/render queue の順序を直接変えない。
- 近距離 avatar や近距離 object が名前だけ、または低 LOD のまま長時間残る場合、見るべき問題は texture cap ではなく queue scheduling である。
- AYAstorm のロード対策は、cap/downscale だけでなく、fetch、decode、GL create、callback、material、object update の各 queue で P0/P1 を先に処理する必要がある。
- loading cloud particle を OFF にしても、avatar の実ロード優先度は上がらない。OFF は演出の削減であり、近距離 avatar を先に rez させる仕組みではない。

### Queue の扱い

ロード中の queue は「処理順」と「破棄/縮小候補」を分けて扱う。

先に処理する対象:

- 近距離 avatar の存在、名前、姿勢、baked texture、最低限の表示。
- 近距離 object の形状、衝突、クリック対象、看板/案内など行動判断に必要な面。
- カメラ正面、足元、進行方向の object。
- 既に画面内で大きく見えている diffuse/basecolor texture。

後回しにする対象:

- 遠距離 object の高解像度 texture。
- normal/specular/PBR 補助マップ。
- avatar の装飾 attachment、髪、alpha が重い装備など、存在把握に不要な詳細。
- shadow、reflection、遠距離 impostor の更新。

破棄または縮小候補:

- 遠距離 texture。
- 視野外 texture。
- テレポート前リージョン由来で、到着先で再利用されていない texture。
- face/volume 参照がなく、callback/raw保存/fetch/fast cache 中でもない texture。
- GL create queue に残っているが、現在 scene から参照されていない通常 LOD texture。

破棄してはいけない対象:

- 近距離 P0/P1 の avatar / object を成立させるために必要な texture、mesh、callback。
- baked avatar texture、avatar appearance 完了に必要な callback。
- HUD、preview、sculpt、media、local texture、保存要求のある raw image。
- 現在 face/volume 参照があり、近距離または画面内で使われている texture。

この分類により、queue が増えた時に「何でも止める」のではなく、P0/P1 を前に出し、遠距離・視野外・旧リージョン由来・参照なし候補を破棄/縮小してフレーム時間とメモリを取り戻す。

### AYAstorm での暫定ロードポリシー

| 優先度 | 表示対象 | 許容する仮表示 | 低 FPS 時の扱い |
| --- | --- | --- | --- |
| P0 | 自分、カメラ、UI、足元、衝突形状 | 低 LOD/単色可 | 間引かない |
| P1 | 近距離構造物、近距離 avatar 位置 | 低解像度 texture / impostor | 軽く間引く |
| P2 | 高解像度 texture、material、attachment | 低解像度/未適用で可 | 強く時間予算化 |
| P3 | shadow、reflection、遠距離 impostor | 前回結果の使い回し | 更新頻度を落とす |
| P4 | 遠景仕上げ、最終 LOD 調整 | 未完成で可 | FPS 回復後に実行 |

この順序にすると、ロード中の見た目は一時的に粗くなるが、操作可能な状態を先に作れる。Second Life の従来挙動は P2/P3 も早期に同時進行しがちで、ここが低 FPS の原因になっている可能性が高い。

2026-06-02 の実機観察メモ:

- 近くに少数 avatar しかいないのに名前だけ見えて本体が出ない状態は、近接優先が成立していない兆候として扱う。
- この状態で avatar loading cloud particle が OFF だと、未ロード中の演出が消えるため症状が目立つ。ただし、particle を戻すことは根本対策ではない。
- 根本対策は、近距離 avatar / 近距離 object を fetch/decode/create/callback/render queue で先に処理し、遠距離・視野外・旧リージョン由来・参照なし候補を破棄/縮小対象にすることである。
- 2026-06-03 の追加修正では、全 avatar / attachment を無条件に cap 対象外にするのではなく、`AYATextureDistanceCapNearMeters` 以内の avatar 関連 face だけ distance/offscreen/aux cap をバイパスする。目的は、近距離 avatar の本体/装着物が frustum 判定の揺れや auxiliary cap で低解像度扱いに落ち、名前だけ表示される状態を避けること。

## 既存の Improve Graphics Speed / AutoTune の評価

Firestorm/AYAstorm には `Improve Graphics Speed` floater があり、現在 FPS と自動調整 UI を表示する。これは有用な機能だが、実装を見る限り「ロード中の各処理をフレーム予算内に収めるスケジューラ」ではない。そのため、今回のような読み込み中の 10fps 級低下に対しては、効きが薄く見える可能性が高い。

### 実際に調整している主な項目

実装上、AutoTune が主に触るのは以下である。

- `RenderAvatarMaxART`: avatar の描画時間しきい値。
- `RenderAvatarMaxNonImpostors`: 非 impostor avatar 数。
- `RenderFarClip`: 描画距離。
- `AutoTuneRenderFarClipMin` / `AutoTuneRenderFarClipTarget`: 描画距離の下限と復帰目標。
- `AutoTuneImpostorFarAwayDistance` / `AutoTuneImpostorByDistEnabled`: 距離ベースの avatar impostor 関連。
- `TargetFPS`, `AutoTuneFPS`, `AutoTuneLock`, `TuningFPSStrategy`: 目標 FPS と自動調整モード。

一方で、`RenderReflectionDetail` を調整する経路は残っているが、現在の調整処理では `#if 0` で無効化されている。つまり、現状の主戦場は avatar と描画距離である。

### 効く可能性が高いケース

- 混雑地で avatar 描画が支配的な場合。
- 遠景の描画負荷が高く、`RenderFarClip` を下げるだけで負荷が減る場合。
- 目標 FPS を十分高く設定し、continuous tuning を有効にしている場合。

この条件では、AutoTune が avatar を簡略化し、描画距離を落とすことで FPS が改善する可能性がある。

### 効きが薄い可能性が高いケース

今回のロード中 FPS 低下で疑っている主因は、AutoTune が直接制御していない領域に多い。

- texture の GL 作成、decode 後の upload、downscale。
- object list の `idleUpdate()` 集中。
- shadow map 更新。
- avatar impostor の生成頻度そのもの。
- state sort / lighting pass の一時的な集中。
- GLTF scene manager や network callback 後のメインスレッド処理。

これらは P2/P3 のロード処理であり、AutoTune の avatar/draw-distance 調整だけでは直接止まらない。したがって、FPS が 10 以下まで落ちる瞬間の内訳が `textures`, `objectlist`, `statesort`, `lighting`, `shadows_impostors` に寄っているなら、`Improve Graphics Speed` の効果が薄いという体感は妥当である。

### 注意点

- `TargetFPS` のデフォルトは低めで、標準状態では 40fps や 60fps 維持を目標にしているとは限らない。
- floater に表示される現在 FPS は、機能がその FPS を維持できることを意味しない。
- AutoTune は後追いの調整であり、ログイン直後やテレポート直後の突発的な読み込みスパイクを事前に分散する仕組みではない。
- UI には `Avatars Only` と `Avatars and Scene` が見えるが、実装上の調整範囲は広い意味の scene 全体ではなく、主に draw distance である。
- 2026-06-02 時点で起動中の別ビルドの `AYAstorm.log` には、14-15fps 付近への低下が複数回記録されているが、AutoTune による設定変更を示す明確なログは確認できなかった。
- 同ログには `floater_fs_performance.xml` の `flaoter.string` typo と、`panel_fs_performance_nearby.xml` の `FSRenderAvatarMaxART` に対して C++ 側が `RenderAvatarMaxART` を探す子コントロール名不一致も出ている。これは FPS 低下の主因ではないが、floater の品質問題として別途直すべきである。

### 検証手順

同じリージョン、同じカメラ方向、同じキャッシュ状態で以下を比較する。

1. AutoTune off の基準値を取る。
2. AutoTune on、continuous on、target FPS 30 または 45、strategy `Avatars and Scene` で取る。
3. `RenderFarClip`, `RenderAvatarMaxART`, `RenderAvatarMaxNonImpostors`, FPS, AYAPerf 内訳を同時に記録する。
4. FPS が改善した瞬間に、avatar 簡略化か draw distance 低下が起きているか確認する。
5. 改善しない場合、同じ時間帯の `textures`, `objectlist`, `statesort`, `lighting`, `shadows_impostors` を見る。

判定:

- avatar/render-distance が支配的なら、既存 AutoTune は効く。
- texture/object/shadow/loading callback が支配的なら、既存 AutoTune では不足。

### 結論

`Improve Graphics Speed` は残す価値がある。ただし、これは「混雑時の描画負荷を落とす機能」であって、「ロード中に P0 から P4 へ優先順位を付けてフレーム予算を守る機能」ではない。AYAstorm で必要なのは、AutoTune を置き換えることではなく、AutoTune の外側にロード専用の FPS guard を追加し、texture/object/shadow/impostor の処理量を測定値に応じて制御することである。

### Phase 0: 計測だけ

目的:

- 挙動変更を入れず、現在の AYAstorm でロード中のフレーム時間内訳を取る。

採取する値:

- `FPS`
- `display total_avg_ms/max_ms`
- `textures_avg_ms/max_ms`
- `objectlist_avg_ms/max_ms`
- `statesort_avg_ms/max_ms`
- `lighting_avg_ms/max_ms`
- `shadows_impostors_avg_ms/max_ms`
- `stream3d_avg_ms/max_ms`
- ロード完了までの時間
- キャッシュ状態
- リージョン名、視界内 avatar 数

### Phase 1: Texture budget

仮説:

- GL texture 作成/ダウンスケールがメインスレッドを止めている。

変更:

- `RenderTextureCreateTimeSliceMs`
- `RenderTextureDownscaleTimeSliceMs`
- `RenderTextureMaxCreatesPerFrame`
- `RenderTextureMaxDownscalesPerFrame`

採用条件:

- `textures_avg_ms` と `textures_max_ms` が下がる。
- 最低 FPS が上がる。
- 近距離 avatar / 近距離 object が遠距離・視野外・旧リージョン由来 texture より先に表示される。
- テクスチャ完成時間の悪化が許容範囲内。
- create/downscale/callback queue と最古待ち秒数が発散しない。
- callback queue ログを追加してから判定する。

実装メモ:

- `AYALoadingFPSGuardEnabled` と `AYALoadingFPSGuardMinFPS` を追加した。
- 低 FPS guard 発動中は `RenderTextureGuardCreateTimeSliceMs`, `RenderTextureGuardMaxCreatesPerFrame`, `RenderTextureGuardDownscaleTimeSliceMs`, `RenderTextureGuardMaxDownscalesPerFrame` を使い、通常時より texture 作成を強く絞る。
- 目的は texture 完成を急ぐことではなく、ロード中でも移動/カメラ/UI のフレーム予算を残すこと。
- ただし create/downscale の時間予算化だけでは、近接優先は実現しない。queue 内の順序が FIFO のままなら、遠距離・視野外・旧リージョン由来の作業が近距離 avatar/object より先に処理され続ける可能性がある。

取りこぼしやすいリスク:

- create を絞りすぎると FPS は上がっても灰色表示が長く残る。
- queue の優先順位を変えずに create だけ絞ると、近距離 avatar / object の表示も遅くなり、名前だけ見えて本体が出ない状態が悪化する。
- callback を絞りすぎると、ロード完了通知、bake/preview/特殊 texture の後処理が遅延する可能性がある。
- downscale を絞りすぎると、一度作られた高解像度 GL texture が残り続ける。
- downscale を増やしすぎると、逆に GL/Metal 側の再確保コストでヒッチが戻る。
- `addTextureStats(0.f)` の reset 挙動変更は距離 cap には必要だが、既存の texture priority reset 経路にも影響するため、過剰な再 fetch や priority 低下が出ないか見る。

追加で見るログ:

- `AYATextureCap texture_work_stats created/downscaled/create_queue_now/downscale_queue_now`
- `AYAPerf display textures_avg_ms/textures_max_ms`
- 灰色表示が残る時間
- texture 完成までの秒数

#### Phase 1.1: Camera-distance texture cap と破棄

目的:

- 混雑地で一度近距離/大画面占有として読み込まれた texture が、カメラを引いた後も高解像度のまま残り続ける問題を抑える。
- 「読み込みに時間がかかる」のは許容しつつ、読み込み完了後に FPS が 20 未満へ残留する状態を減らす。
- 近距離の視認性は維持し、チャット範囲外や遠景は積極的に低解像度化する。
- これは近接優先 scheduler の代替ではない。距離 cap は遠距離/視野外の品質を下げる部品であり、近距離 avatar/object を queue の先頭へ送る機能ではない。

暫定設定:

| カメラ距離 | 最大要求解像度 | 設定 |
| --- | ---: | --- |
| `<= 10m` | `2048px` | `AYATextureDistanceCapNearMeters`, `AYATextureDistanceCapNearResolution` |
| `<= 20m` | `1024px` | `AYATextureDistanceCapMidMeters`, `AYATextureDistanceCapMidResolution` |
| `<= 50m` | `512px` | `AYATextureDistanceCapFarMeters`, `AYATextureDistanceCapFarResolution` |
| `> 50m` | `256px` | `AYATextureDistanceCapDistantResolution` |

実装メモ:

- `LLViewerTextureList::updateImageDecodePriority()` で face ごとのカメラ距離を見て、HUD attachment 以外の texture の `vsize` を距離別 cap に丸める。
- HUD attachment は対象外にする。HUD は距離で品質を落とすと UI として破綻しやすい。
- `BOOST_HIGH` 以上の boosted texture は既存ロジック通り対象外にする。preview、baked、特殊用途の texture を不用意に落とさないため。
- 同一 texture が複数 face で使われる場合、最終的な要求は各 face の最大 `vsize` が勝つ。近距離 face と遠距離 face が同じ texture を共有している場合、近距離側の要求が残る。

実装上の重要な注意:

- 現実装は `objp->isHUDAttachment()` だけを除外しているため、厳密には「world texture だけ」ではない。
- avatar attachment、装着看板、mouselook 時の自分の装備、HUD ではない UI 的オブジェクトにも距離 cap が効く可能性がある。
- face 中心とカメラ距離で cap を決めているため、大きな床、壁、ステージ背景、看板では「近くに見えているが中心が遠い」面を低解像度化する可能性がある。
- face 数が多すぎる texture は既存の `max_faces_to_check` 回避により cap 判定から外れ、高解像度要求が残る可能性がある。共有 atlas や繰り返し texture ほど対象外になりやすい。

追加の回帰確認:

- 近距離大型看板、床、壁、イベントステージ背景。
- avatar attachment、髪、装着ライト、装着 UI、クリック装備。
- mouselook で見える自分の装備。
- HUD ではないが UI 的に使われる面、ショップ商品、案内表示。
- preview、sculpt、media surface、bake、inventory texture picker、local texture。

破棄の流れ:

1. 距離 cap により `mMaxVirtualSize` が小さくなる。
2. `LLViewerLODTexture::processTextureStats()` が `mMaxVirtualSize` から `mDesiredDiscardLevel` を上げる。
3. 現在の `discard` が desired より高品質なら `scaleDown()` を呼ぶ。
4. `LLViewerTextureList::updateImagesCreateTextures()` の downscale queue で `LLImageGL::scaleDown()` が実行される。
5. GL texture は小さいサイズへ再確保され、元の大きい GL texture 領域は解放対象になる。

今回見つかった問題:

- `LLViewerTexture::addTextureStats(0.f)` は reset 意図のコメントがあったが、実装上は `virtual_size > mMaxVirtualSize` の時だけ更新していたため、0 では `mMaxVirtualSize` が下がらなかった。
- `LLViewerFetchedTexture::setKnownDrawSize()` はコメント上 `0,0` で無効化できるはずだが、実装上は大きい方向にしか更新せず、既存値を消せなかった。
- `LLViewerLODTexture::processTextureStats()` は `mKnownDrawWidth/Height` がある場合、`mMaxVirtualSize` ではなく known draw size を優先して discard を計算していた。そのため、一度近距離で 2048 相当に見た texture は、カメラを引いて距離 cap が 512/256 を要求しても downscale されにくかった。

修正:

- `addTextureStats(0.f)` を実際に `mMaxVirtualSize = 0.f` の reset として扱う。
- `setKnownDrawSize(0,0)` で `mKnownDrawWidth/Height` を clear できるようにする。
- `AYATextureDistanceCapEnabled` が有効で `mMaxVirtualSize` が有効値を持つ場合、LOD texture の discard 計算では `known draw size` と `mMaxVirtualSize` の小さい方を使う。

これにより、カメラを引いた後に距離 cap が `mDesiredDiscardLevel` へ反映され、既に高解像度で作成済みの GL texture も downscale queue に入りやすくなる。

注意点:

- これは「距離が離れた瞬間に全 texture を即時破棄する」強制 purge ではない。downscale はフレーム予算内で順に処理する。
- `RenderTextureMaxDownscalesPerFrame` と `RenderTextureGuardMaxDownscalesPerFrame` を 0 にすると、破棄要求が出ても実際の GL downscale が進まない。
- 通常の raw image は GL texture 作成後に破棄される。`mSavedRawImage` は callback、preview、bake、sculpt 等の用途があるため、無条件破棄しない。Phase 1.7 では安全判定を通った候補だけ条件付きで破棄する。
- Apple の Activity Monitor 上のメモリ値は、GL/Metal driver cache や purgeable memory の影響を受けるため、downscale 直後に単純に RSS が下がるとは限らない。効果確認は FPS、AYAPerf の `textures_*`、GL texture discard 状態、長めのメモリ推移で見る。

#### Phase 1.2: 実機で距離 cap が見えにくい理由と追加対応

2026-06-02 の実機観察では、カメラ距離に応じて texture 品質が明確に変わっているようには見えなかった。この観察は実装上も不自然ではない。距離 cap は描画済み texture をその場で強制破棄する機能ではなく、face ごとの `vsize` と texture の desired discard を下げ、downscale queue 経由で GL texture を小さくしていく機能である。

見た目に反映されにくい主な理由:

- 同じ texture が近距離 face と遠距離 face で共有されている場合、近距離側の高い要求が勝つ。
- HUD、preview、baked、callback、raw 保存が必要な texture は不用意に落とせない。
- `known draw size` や min discard の影響で、距離 cap が desired discard へすぐ反映されない経路があった。
- downscale queue の処理数が `1/frame` 相当だと、FPS 10 前後では数百から数千 texture の破棄反映が非常に遅い。
- GL/Metal driver cache の影響で、downscale 後も Activity Monitor のメモリ値や見た目がすぐ変わらないことがある。

追加対応:

- `LLViewerTextureList::updateImageDecodePriority()` に `AYATextureCap` ログを追加し、`faces`, `clamped_faces`, `aux_faces`, 距離帯別件数、`downscale_queue` を 10 秒間隔で出す。
- `LLViewerTextureList::updateImagesCreateTextures()` に `texture_work_stats` ログを追加し、`created`, `redundant_creates`, `downscaled`, queue 最大/現在値、guard 状態、downscale 上限、FPS を出す。
- `RenderTextureMaxDownscalesPerFrame` と `RenderTextureGuardMaxDownscalesPerFrame` の既定値を `4` に上げる。
- ユーザー設定が非ゼロなら、実効 downscale 数を最低 `4/frame` にする。`0` は明示的な downscale 停止として扱う。
- downscale time slice は最低 `2.0ms` とし、距離 cap の反映が極端に遅れないようにする。

不足している追加対応:

- face 数上限により cap 対象外になった texture 件数をログ化する。
- face 中心距離によって cap された大型面の件数、または大きな `vsize` が cap された件数をログ化する。
- callback queue の詰まりを別ログに分ける。texture create/downscale が改善しても callback が詰まると見た目の完成は遅れる。

#### Phase 1.3: Normal / specular / PBR 補助マップの扱い

material の補助マップは、diffuse/basecolor と同じ見た目優先度で扱うべきではない。近距離の識別には diffuse/basecolor が最重要であり、normal/specular/metallic-roughness/emissive はロード中 FPS が低い間は P2 として後回しにできる。

実装上、normal/specular/PBR material map は `LLFace` の texture channel を通って `LLViewerTextureList::updateImageDecodePriority()` の同じループに入る。今回の追加では、`LLRender::DIFFUSE_MAP` と `LLRender::BASECOLOR_MAP` 以外の channel を auxiliary material map とみなし、通常 texture より低い距離 cap を適用する。

暫定 auxiliary cap:

| カメラ距離 | 補助マップ最大要求解像度 | 設定 |
| --- | ---: | --- |
| `<= 10m` | `1024px` | `AYATextureDistanceCapAuxNearResolution` |
| `<= 20m` | `512px` | `AYATextureDistanceCapAuxMidResolution` |
| `<= 50m` | `256px` | `AYATextureDistanceCapAuxFarResolution` |
| `> 50m` | `128px` | `AYATextureDistanceCapAuxDistantResolution` |

注意:

- channel `1` は normal map と alternate diffuse map が重なるため、現時点の実装では alternate diffuse 用途も auxiliary 扱いになる可能性がある。
- これはロード中 FPS とメモリ圧を下げるための暫定策であり、見た目の副作用が出る場合は channel の意味を material type ごとに分けて判定する必要がある。
- 補助マップの cap が効いているかは、見た目だけでは判断しにくい。`AYATextureCap` の `aux_faces`, `clamped_faces`, `downscaled`, `downscale_queue_now` を見る。

追加の穴:

- `LLFace` の texture channel だけで auxiliary 判定すると、legacy material、PBR、alternate diffuse の意味を完全には区別できない。
- normal/specular を落としても diffuse/basecolor が高解像度のままなら、見た目の差は小さく、メモリ削減だけが主効果になる可能性がある。
- emissive は視認性に関わる看板や案内表示で使われる場合があるため、常に P2 扱いするとユーザーの行動判断を悪化させる可能性がある。

次の実装判断:

- 現状の broad auxiliary cap は検証用とし、正式採用は保留する。
- emissive/alternate diffuse/装着 UI/看板の回帰確認が終わるまでは、通常配布では `AYATextureDistanceCapAuxEnabled=false` も候補に残す。
- `aux_faces` が多く、かつ可読性劣化が出る場合は、material 種別を見て normal/specular/ORM と emissive/alternate diffuse を分ける。
- 看板/案内の emissive 劣化が確認されたら、emissive は diffuse/basecolor と同じ cap に戻す。

#### Phase 1.4: 視野外 texture cap

距離だけで texture 解像度を決めると、カメラの裏や画面外の近距離 object が高解像度要求を維持する。これは「動ける FPS を守る」という目的に反するため、frustum 外の face は距離帯 cap よりさらに低い上限を重ねる。

暫定設定:

| 対象 | 最大要求解像度 | 設定 |
| --- | ---: | --- |
| 視野外 diffuse/basecolor | `256px` | `AYATextureOffscreenCapResolution` |
| 視野外 normal/specular/PBR 補助マップ | `128px` | `AYATextureOffscreenCapAuxResolution` |

実装メモ:

- `LLViewerTextureList::updateImageDecodePriority()` で `LLFace::mInFrustum == false` の face に offscreen cap を適用する。
- HUD attachment は距離 cap と同様に対象外にする。
- 視野外 cap は距離 cap と併用する。例えばカメラの裏にある 5m の object は near `2048px` ではなく offscreen `256px` が勝つ。
- ログは `AYATextureCap distance_cap_stats` に `offscreen_faces` を追加する。

注意:

- 視野外 cap は「見えていないものを先に粗くする」ための処理であり、カメラを振り向いた瞬間に再 fetch/downscale 復帰が起きる可能性がある。
- 視野外でも同一 texture が視野内 face と共有されている場合、視野内側の高い要求が勝つ。
- カメラ端の object は frustum 判定の揺れで cap が切り替わる可能性があるため、ログと実機でポップインを確認する。

#### Phase 1.5: テレポート後の pre-teleport texture sweep

現在の viewer はテレポート時に `LLViewerTextureList::clearFetchingRequests()` で fetch request を消すが、すでに GL texture として作られた UUID texture は global texture list に残る。テレポート先でも同じ UUID が表示されるなら保持すべきだが、前リージョンでだけ使われていた texture まで高解像度のまま残す必要はない。

実装方針:

- テレポート表示開始時に、既存の通常 LOD texture を snapshot queue に入れ、`mAYATeleportSeenGeneration` を付ける。
- 到着後の decode/cache 処理が始まった texture は、`updateImageDecodePriority()` で face 使用が確認された時に `mAYATeleportUsedGeneration` を付ける。
- `AYATeleportTextureSweepGraceSeconds` 経過後、snapshot queue を `AYATeleportTextureSweepMaxTexturesPerFrame` の範囲で走査する。
- テレポート先で再利用された texture、まだ face/volume 参照がある texture、boost 付き、callback/raw 保存/fetch/fast cache 中の texture は downscale しない。
- 安全に未使用と判断できた通常 LOD texture だけ `mMaxVirtualSize=0` として `processTextureStats()` に通し、既存の downscale queue に送る。

設定:

| 設定 | 既定値 | 意味 |
| --- | ---: | --- |
| `AYATeleportTextureSweepEnabled` | `true` | テレポート後の pre-teleport texture sweep を有効化する |
| `AYATeleportTextureSweepGraceSeconds` | `20.0` | 到着側の再利用判定を待つ猶予秒数 |
| `AYATeleportTextureSweepMaxTexturesPerFrame` | `128` | 1 frame に検査する snapshot texture 数 |
| `AYATeleportTextureSweepLog` | `true` | start/progress/complete をログ出力する |

見るログ:

- `AYATextureCap teleport_texture_sweep event=start queued=...`
- `AYATextureCap teleport_texture_sweep event=progress scheduled=... kept_used=... kept_active=... kept_special=...`
- `AYATextureCap teleport_texture_sweep event=complete downscale_queue=...`

注意:

- これは強制 purge ではない。GL texture の実際の縮小は既存の `mDownScaleQueue` と `RenderTextureMaxDownscalesPerFrame` / `RenderTextureGuardMaxDownscalesPerFrame` に従って進む。
- face/volume 参照が残っている texture は保守的に保持するため、前リージョン由来の texture が一部残る可能性はある。
- テレポート先で同じ UUID が再利用されている場合は `kept_used` として保持される。
- メモリ表示は driver cache の影響を受けるため、効果判定は FPS、`downscaled`, `downscale_queue_now`, `teleport_texture_sweep scheduled` を合わせて見る。

#### Phase 1.6: 近接優先 texture / object queue scheduler

2026-06-02 の実機観察で、近距離に少数 avatar しかいないのに名前だけ見えて本体が出ない状態が確認された。これは「距離 cap が見た目に効いていない」というだけではなく、近接 avatar/object を queue の先頭へ送る仕組みが不足している可能性を示す。

目的:

- 近距離 avatar / 近距離 object を、遠距離・視野外・旧リージョン由来・参照なし候補より先に表示する。
- FPS 低下時でも、P0/P1 の存在把握を先に成立させる。
- queue が発散する場合、遠距離・視野外・旧リージョン由来・参照なし候補を破棄/縮小して、近接表示の処理枠を空ける。

重要な前提:

- loading cloud particle は OFF のままでよい。particle は未ロード中の演出であり、近接 avatar の実ロード優先度を上げる仕組みではない。
- avatar attachment を一律で保護対象にするのは誤り。近距離 avatar の存在把握に必要なものと、装飾 attachment は分ける。
- texture cap は「品質上限」であり、queue の「処理順」ではない。近接優先は fetch/decode/create/callback/object update 側で実装する。

実装方針:

- texture create queue に priority score を導入する。FIFO のままではなく、近距離・画面内・大画面占有・avatar bake/基本表示に必要なものを先に処理する。
- score は少なくとも `distance`, `in_frustum`, `pixel_area`, `boost level`, `face/volume ref`, `avatar/basic object relevance`, `teleport generation` を見る。
- GL create queue が soft limit を超えた場合は、遠距離・視野外・旧リージョン由来・参照なし候補を先に drop/downscale 候補にする。
- callback queue も P0/P1 を優先する。avatar appearance、bake、近距離 object の可視化に必要な callback を、遠距離 texture callback より先に処理する。
- object update は、近距離 avatar と近距離 object の transform/visibility/LOD を優先する。遠距離 active object、media、particle、decorative update は低 FPS 中に間引く。

実装済み:

- `LLViewerTextureList::updateImagesCreateTextures()` で pending GL texture create queue を priority score 順に並べ替える。
- score は `FTT_SERVER_BAKE/FTT_HOST_BAKE`, protected boost, raw保存/sculpt, callback, face 距離、frustum、pixel area、avatar 関連 object を見る。
- 全 queue を毎回 sort せず、`AYATextureCreatePriorityScanLimit` 件を上限に先頭・中央・末尾 window を評価して window reorder する。2万件級 queue で reorder 自体が FPS を潰さないようにしつつ、新しく queue 末尾に入った近距離 texture と、camera 移動などで queue 中央に残ったまま近距離化した texture を救出できるようにするため。
- 中央 window は reorder ごとに probe 位置を進める。巨大 queue の middle にいる候補を全件採点せず、複数 frame で順番に拾う。
- 中央・末尾 window から前方へ救出するのは near / mid / protected bucket のみ。far / offscreen / unreferenced を前方へ送って近距離候補を圧迫しない。
- backpressure は先頭候補を pop/push で末尾へ回さない。scan window 内で安全に cancel できたものだけ破棄し、残りは元の順序で queue 先頭へ戻す。
- callback は drop 保護対象だが、score 上は avatar bake/raw保存/sculpt/protected boost より弱く扱う。遠距離 callback が近距離 visible texture を一律に追い越すのを避ける。
- priority score の距離しきい値は `AYATextureDistanceCapNear/Mid/FarMeters` と同期する。
- `AYATextureCreatePriorityEnabled` で有効化する。
- `AYATextureCreatePriorityMinQueue` 件以上の pending create がある場合にだけ reorder する。
- `AYATextureCreatePriorityIntervalSeconds` で reorder 間隔を制限し、毎フレーム全 queue sort にならないようにする。
- `AYATextureCreateBackpressure*` は、queue が soft limit を超えた時に遠距離・視野外・参照なし通常 LOD の pending raw image を cancel してメモリを空ける。hard limit 超過時は1 frame あたりの最大 drop 数を増やす。
- backpressure は avatar bake、callback/raw保存、sculpt、local/map tile、boosted/protected texture を破棄しない。
- `FTT_DEFAULT` 経由の avatar bake fallback を誤って落とすリスクを下げるため、参照なし候補でも `AYATextureCreateBackpressureMinUnreferencedSeconds` 経過前は drop しない。
- 2026-06-03 の実機ログでは、旧実装の backpressure は `create_queue_now` が約 22,709 から 35,788 へ増え、FPS が約 10 から 6 台へ落ちる間に `backpressure_scanned` は 100 万件以上増えたが、`backpressure_dropped=0` のままだった。原因は release 条件が「face/volume 参照なし、fetch/cache/callback/raw保存なし、一定秒数経過」に寄りすぎ、遠距離・視野外の pending create が scene から参照されている限り cancel されなかったこと。
- 2026-06-03 の再検証ログでは `create_queue_now` が約 4,100〜4,500、`queued_far` が約 6万〜7万件まで増え、`backpressure_scanned` が増えても `backpressure_dropped=0` のままだった。原因は distance defer が `hard limit=8192` 超過時だけ有効で、soft limit 直上の実運用 queue では far/offscreen の cancel が発動していなかったこと。さらに priority reorder が near/mid/protected を queue 前方へ寄せるため、backpressure が先頭 window だけを見ると後方に退避された far/offscreen を見に行けなかった。
- `AYATextureCreateBackpressureDeferDistanceEnabled` は soft limit 超過時から有効化し、backpressure は queue 先頭だけでなく末尾 window も走査する。near / mid / protected は引き続き cancel 対象外にする。
- teleport sweep は active 中の再入を拒否し、候補数上限 `AYATeleportTextureSweepMaxQueuedTextures` を持つ。
- 旧リージョン由来の直接識別は、現時点では teleport sweep に依存する。create queue priority/backpressure には teleport generation 由来の明示的な old-region score はまだ入っていない。

設定:

| 設定 | 既定値 | 意味 |
| --- | ---: | --- |
| `AYATextureCreatePriorityEnabled` | `true` | pending GL texture create queue を近接/可視/保護優先で並べ替える |
| `AYATextureCreatePriorityMinQueue` | `32` | reorder を始める最小 queue 件数 |
| `AYATextureCreatePriorityScanLimit` | `2048` | 1回の reorder で score 計算する最大件数。巨大 queue では先頭・中央・末尾に分けて評価する |
| `AYATextureCreatePriorityIntervalSeconds` | `0.25` | reorder の最小間隔 |
| `AYATextureCreatePriorityLog` | `false` | reorder summary をログ出力する |
| `AYATextureCreateBackpressureEnabled` | `true` | queue 過多時に安全な pending create を cancel する |
| `AYATextureCreateBackpressureQueueLimit` | `4096` | backpressure の soft limit |
| `AYATextureCreateBackpressureHardLimit` | `8192` | soft limit 超過時より drop 数を増やす hard limit |
| `AYATextureCreateBackpressureMaxDropsPerFrame` | `512` | soft limit 時の最大 drop 数 |
| `AYATextureCreateBackpressureMinUnreferencedSeconds` | `20.0` | 参照なし pending create を drop 可能にするまでの最小未参照秒数 |
| `AYATextureCreateBackpressureDeferDistanceEnabled` | `true` | soft limit 超過時に遠距離・視野外・参照なし pending create を cancel/defer できるようにする |

破棄/縮小候補:

- 遠距離 texture。
- 視野外 texture。
- 旧リージョン由来で、テレポート先で再利用されていない texture。
- face/volume 参照なし texture。
- callback/raw保存/fetch/fast cache 中ではない通常 LOD texture。
- GL create queue に残っているが、現在の scene から参照されていない pending raw image。

破棄しない対象:

- avatar appearance / baked texture / near avatar の基本表示に必要な texture と callback。
- 近距離の床、壁、階段、衝突、クリック対象、看板/案内面。
- HUD、preview、sculpt、media、local texture、保存要求のある raw image。
- 現在 face/volume 参照があり、近距離または画面内で使われている texture。

ログ:

- `AYATextureQueue priority_stats near=... mid=... far=... offscreen=... old_region=... unreferenced=...`
- `AYATextureQueue create_order created_near=... created_mid=... created_far=... created_offscreen=...`
- `AYATextureQueue drop_stats dropped_far=... dropped_offscreen=... dropped_old_region=... dropped_unreferenced=...`
- `AYAAvatarLoad nearby pending=... fully_baked=... fully_loaded=... name_only=...`
- `AYAObjectQueue near_updated=... far_skipped=... offscreen_skipped=...`

現行ログ:

- `AYATextureQueue priority_stats event=reorder queue=... near=... mid=... far=... offscreen=... unreferenced=... protected=...`
- `AYATextureCap create_backpressure event=drop_pending queue_before=... queue_after=... scanned=... scanned_front=... scanned_tail=... dropped=... dropped_inactive=... dropped_distance=... dropped_far=... dropped_offscreen=... dropped_unreferenced=... allow_distance_defer=... kept=...`
- `AYATextureCap texture_work_stats ... created_near=... created_mid=... created_far=... created_offscreen=... created_unreferenced=... created_protected=... queued_near=...`

採用条件:

- 近距離に avatar が少数しかいない状況で、名前だけ表示され本体が長時間出ない状態が改善する。
- `name_only` または未 fully loaded の近距離 avatar が減る。
- `create_order` で near が far/offscreen より先に処理されている。
- `drop_stats` で破棄対象が遠距離・視野外・旧リージョン由来・参照なし候補に偏っている。
- FPS が 10 未満へ落ちても、入力、カメラ、UI、近距離存在把握が維持される。

失敗条件:

- FPS は上がるが、近距離 avatar/object が未表示のまま長時間残る。
- avatar bake/callback を drop してしまい、appearance が完了しない。
- 近距離 object より遠距離/offscreen texture が先に GL create され続ける。
- 破棄候補に近距離 P0/P1 が混ざる。

#### Phase 1.7: FPS/メモリ圧力時の texture memory sweep

2026-06-03 の実機観察では、距離 cap と create queue backpressure だけではメモリ増加が残る可能性があった。特に一度 GL texture 化されたもの、または `mSavedRawImage` として保持されているものは、`scaleDown()` だけでは RSS/driver memory の圧力を十分に下げられないことがある。

目的:

- FPS target に対して実 FPS が大きく落ちている時、または process RSS が一定以上の時に、遠距離・視野外・参照なし候補から段階的にメモリを返す。
- 近距離 avatar、HUD、自分の表示、bake/callback/sculpt/fetch/create/downscale 中の texture は破壊しない。
- `scaleDown()` だけでなく、必要な場合は `destroySavedRawImage()` と `destroyGLTexture()` まで進める。ただし `destroyGLTexture()` は無参照候補に限定し、可視/再利用中 texture の突然の消失を避ける。

実装済み:

- `LLViewerTextureList::updateImagesFetchTextures()` の後段に incremental sweep を追加した。
- `aya_texture_memory_sweep_bucket()` で候補を `keep`, `unreferenced`, `offscreen`, `far` に分類する。
- avatar/self/HUD/protected boost/callback/fetch/create/downscale/sculpt/fast cache 等は sweep 対象外にする。
- `unreferenced` は `destroySavedRawImage()` と、budget があれば `destroyGLTexture()` を実行する。
- `offscreen` / `far` は `destroySavedRawImage()` と `scaleDown()` 予約に留める。GL texture の即時 destroy はしない。
- scan は `mUUIDMap` 全体を毎 frame 全走査せず、前回位置から `AYATextureMemorySweepScanPerFrame` 件ずつ進める。
- active 条件は FPS pressure または memory pressure。FPS pressure は `AYALoadingFPSGuardEnabled` が有効で、`gFPSClamped < AYALoadingFPSGuardMinFPS` かつ `fps / target_fps <= AYATextureMemorySweepFPSRatio` の時。memory pressure は process RSS が `AYATextureMemorySweepMinPhysicalMB` 以上の時。

設定:

| 設定 | 既定値 | 意味 |
| --- | ---: | --- |
| `AYATextureMemorySweepEnabled` | `true` | FPS/メモリ圧力時の texture memory sweep を有効化する |
| `AYATextureMemorySweepScanPerFrame` | `256` | 1 frame に検査する texture 数 |
| `AYATextureMemorySweepDestroyGLPerFrame` | `16` | 1 frame に `destroyGLTexture()` できる最大数。無参照候補のみ |
| `AYATextureMemorySweepDestroyRawPerFrame` | `64` | 1 frame に `destroySavedRawImage()` できる最大数 |
| `AYATextureMemorySweepScaleDownPerFrame` | `32` | 1 frame に `scaleDown()` 予約できる最大数 |
| `AYATextureMemorySweepMinUnreferencedSeconds` | `8.0` | 無参照候補として扱うまでの猶予秒数 |
| `AYATextureMemorySweepFPSRatio` | `0.75` | `fps / target_fps` がこの値以下なら FPS pressure とする |
| `AYATextureMemorySweepMinPhysicalMB` | `7168` | process RSS がこの値以上なら memory pressure とする。`0` で無効 |
| `AYATextureMemorySweepLog` | `true` | `memory_sweep_stats` をログ出力する |

ログ:

- `AYATextureCap memory_sweep_stats enabled=... active=... fps_pressure=... memory_pressure=... physical_mb=... target_fps=... fps=... scanned=... destroyed_gl=... destroyed_raw=... scaled_down=... skipped=... unreferenced=... offscreen=... far=... image_count=...`

通常検証時のログ設定:

- `AYATextureCreateBackpressureLog`, `AYATextureMemorySweepLog`, `AYATextureDistanceCapLog`, `AYALoadingDynamicResolutionLog` は既定 OFF にする。
- 2026-06-03 の実機ログでは `create_backpressure` が 1 秒に 50 行以上出る区間があり、ログ出力自体が FPS 測定のノイズになり得る。
- 再度 queue/drop/sweep の内訳を検証する時だけ、対象ログを短時間 ON にして採取する。特に `AYATextureCreateBackpressureLog` は frame loop 内で高頻度になりやすいため、常用しない。

### Phase 1.8: RenderResolutionMultiplier adaptive control 検算と修正方針

2026-06-03 のサブエージェント検算では、動的 RenderResolutionMultiplier の負荷式は一次近似として以下で妥当と判断した。

```text
OFF:
  T_off = C + G

ON:
  T_on = C + G * m^2 + lambda * R / F
```

- `T`: 1 frame あたりの時間。
- `C`: 解像度を下げても減らない CPU / scene update / UI / network / texture fetch-decode 等の固定負荷。
- `G`: full resolution 時の pixel dependent GPU 負荷。
- `m`: effective `RenderResolutionMultiplier`。
- `lambda`: render target 再確保回数 / 秒。
- `R`: render target 再確保 1 回の停止コスト。
- `F`: FPS。

ON が OFF より軽い条件:

```text
T_on < T_off
C + G * m^2 + lambda * R / F < C + G
G * (1 - m^2) > lambda * R / F
```

つまり、低解像度化で浮く pixel dependent GPU 負荷が、render target 再確保の償却コストを上回る必要がある。

コード調査:

- `llviewerdisplay.cpp` は `sqrt(current_fps / target_fps)` で倍率を推定し、従来は `0.05` 刻み、`1 秒` 間隔で調整していた。
- `pipeline.cpp` は `RenderResolutionMultiplier` を `resX` / `resY` の両方に掛けるため、画面依存 RT 面積は概ね `m^2` に比例する。
- 倍率変更は `resizeScreenTexture()` から `releaseScreenBuffers()`, shadow target release, `allocateScreenBufferInternal()` へ到達する。
- 再確保対象は `deferredScreen`, `screen`, `deferredLight`, `mObjectIDBuffer`, `mVelocityMap`, `mSMAAHistory`, `mAYAAlphaDepth`, `mAYAAlphaColor`, FXAA/SMAA buffers, post buffers, water buffers 等であり、単なる scale 変更ではなく GPU texture/FBO lifecycle を多数走らせる。

ログ照合:

- `allocated mVelocityMap` は 2026-06-03 の対象ログ全体で 706 回。
- in-world 相当の 13:22:10 以降では 705 回 / 1614 秒、`lambda = 0.437 回/秒`。
- 直近 5 分では 165 回 / 292 秒、`lambda = 0.565 回/秒`。約 1.8 秒に 1 回 RT 再確保していた。
- 再確保あり区間は平均 FPS が高く、低解像度化の平均効果は出ている。一方で `total_max_ms` の最大値は再確保あり区間の方が大きく、瞬間停止/カクつきの原因候補になっている。

修正方針:

- `m^2` の利益を残しつつ `lambda` を下げる。
- 連続的な `0.05` 刻み調整をやめ、`sqrt(fps / target_fps)` に最も近い粗い bucket へ丸める。
- bucket は `1.0`, `0.75`, `0.66`, `0.50`, `0.33`, `0.25`。
- 解像度を下げる方向は短め cooldown、上げる方向は長め cooldown にする。
- target FPS 近辺の往復を避けるため、上げる条件には回復側 hysteresis を入れる。
- 正確な `R` を測る必要がある場合は、`resizeScreenTexture()` または `allocateScreenBufferInternal()` の所要時間ログを短時間だけ追加して検証する。

確認ポイント:

- `active=1` なのに `destroyed_raw`, `destroyed_gl`, `scaled_down` が長時間 0 のままなら、安全判定が厳しすぎるか、保護対象が多すぎる。
- `destroyed_gl` は増えすぎないこと。増える対象は参照なし候補だけであり、近距離 avatar/object の表示が消えるなら分類ミス。
- `destroyed_raw` が増えても Activity Monitor の memory が即時に下がるとは限らない。driver cache と allocator の影響があるため、FPS、`physical_mb` の長めの推移、texture queue、近接表示完了を合わせて見る。
- 近距離 avatar/HUD が読み込めない場合は sweep よりも priority/backpressure 側の分類、または保護対象の判定を先に疑う。

2026-06-03 ビルド検証:

- `cmake --build build-darwin-universal --config Release --target ayastorm-bin -j 8 -- ARCHS=arm64 ONLY_ACTIVE_ARCH=YES` は成功した。
- 生成物: `build-darwin-universal/newview/Release/AYAstorm.app`
- `file` では `Mach-O 64-bit executable arm64`。
- `lipo -info` では non-fat `arm64`。
- `codesign --verify --deep --strict` は成功。

#### 2026-06-02 ビルド/起動検証

ビルド:

- `xcodebuild -project build-darwin-universal/Firestorm.xcodeproj -scheme ayastorm-bin -configuration Release ... build` は成功した。
- 生成物: `build-darwin-universal/newview/Release/AYAstorm.app`
- `lipo -info` では `AYAstorm` 本体は non-fat `arm64`。
- `codesign --verify --deep --strict --verbose=2` は valid。
- app 内 `Contents/Resources/app_settings/settings.xml` に `AYATextureDistanceCapAux*`, `AYATextureDistanceCapLog`, downscale 既定値変更が反映されている。

起動検証:

- app は起動し、`AYAstorm.log` は新しい app 内の `settings.xml` を読み込んだ。
- ただし描画ループ到達前にログが止まり、`AYATextureCap` はまだ出ていない。
- `sample` では main thread が `LLAppViewer::initStrings()` -> `LLTransUtil::parseStrings()` -> `LLXMLNode::parseFile()` -> `LLFile::getContents()` -> `fopen/open` にいた。
- これは texture 距離 cap の検証段階より前の起動/ファイル読み込み問題であり、今回の FPS guard 効果検証とは分けて扱う。

次の確認:

1. 起動が `initStrings()` を抜けてログイン画面または world 描画まで進む状態にする。
2. ログイン/テレポート後 120 秒の `AYATextureCap` と AYAPerf を採取する。
3. `clamped_faces > 0`, `aux_faces > 0`, `downscaled > 0`, `downscale_queue_now` が減るかを見る。
4. 見た目ではなく、FPS、`textures_avg_ms/max_ms`、`texture_work_stats`、メモリ推移で効果判定する。

### Phase 2: Active object idle budget

仮説:

- ロード中に active object の `idleUpdate()` が一気に増え、`objectlist` が FPS を潰している。

変更:

- 非 avatar active object の更新を FPS 低下時だけ時間予算化する。
- avatar は操作感への影響が強いため、最初の検証では別扱いにする。

採用条件:

- `objectlist_avg_ms` が下がる。
- avatar 表示、物理、スクリプト、animated texture の破綻がない。
- ロード完了や object 表示の遅延が許容範囲。
- active non-avatar 数、updated/visited/skipped、cursor 一巡秒数のログを追加してから判定する。

実装メモ:

- 低 FPS guard 発動中は非 avatar active object の `idleUpdate()` を `RenderObjectIdleGuardTimeSliceMs` / `RenderObjectIdleGuardMinUpdatesPerFrame` でさらに絞る。
- avatar は P0/P1 の存在把握と操作感に近いため、この guard では毎フレーム更新を維持する。
- ただし、この「avatar 維持」は avatar 本体の `idleUpdate()` を維持するという意味であり、attachment まで毎フレーム維持する保証ではない。attachment は非 avatar object として間引かれる可能性がある。

取りこぼしやすいリスク:

- 非 avatar object でも、animated texture、particle emitter、media surface、click target、moving platform は体感に影響する。
- cursor 方式で公平に回しても、object 数が極端に多い場所では一巡に数秒かかる可能性がある。
- object idle を絞ったことで `objectlist_avg_ms` は下がっても、`statesort` や `lighting` が残る場合は FPS が上がらない。

検証で見る値:

- `AYAPerf idle objectlist_avg_ms/objectlist_max_ms`
- active object 数と、guard 中に 1 巡するまでの推定秒数
- animated texture / media / moving object の見た目破綻
- 近距離クリック対象や衝突対象の反応遅延
- AO、髪、装着ライト、装着 UI、クリック装備の遅延
- non-avatar active object cursor の一巡秒数

### Phase 3: Shadow / impostor throttle

仮説:

- ロード中に shadow map と avatar impostor 更新が不要にフレームを使っている。

変更:

- 低 FPS 時のみ shadow 更新間隔を伸ばす。
- 低 FPS 時のみ avatar impostor 更新を止める、または 1 frame あたりの更新数を抑える。

採用条件:

- `shadows_impostors_avg_ms` と `max_ms` が下がる。
- 影のカクつき、古い impostor の残留が許容範囲。
- FPS が上がる。
- shadow updated/skipped と impostor candidates/updated/skipped/stale 秒数の別ログを追加してから判定する。

実装メモ:

- 低 FPS guard 発動中は sun shadow 更新間隔を `RenderSunShadowGuardFrameInterval` に切り替える。
- avatar impostor 更新は `AYALoadingFPSGuardMinFPS` 未満では停止する。遠距離 avatar の見た目更新より、入力応答と最低 FPS を優先する。

取りこぼしやすいリスク:

- `shadows_impostors` は shadow と impostor が同じ計測枠に入っているため、どちらが効いたか分かりにくい。
- impostor 更新を完全停止すると、古い姿勢や位置が長く残る。
- sun shadow の間引きは昼夜/環境変更時の見た目遅延を起こす。
- snapshot や high quality render では throttle を適用しない方がよい。

追加方針:

- 可能なら shadow と impostor を別ログに分ける。
- guard 中の impostor は完全停止だけでなく、近距離 avatar を優先し遠距離だけ停止する案を残す。
- `RenderAvatarMaxImpostorUpdatesPerFrame = 0` は実装上、per-frame 上限なしを意味する。完全な切り戻しではこの値も 0 にする。
- shadow は updated/skipped と interval をログへ出す。
- impostor は candidates/updated/skipped と最古更新待ち秒数をログへ出す。

### Phase 4: Rendering load policy

仮説:

- SL でもロード中は「完成度よりフレーム維持」を優先するモードが必要。

変更候補:

- ログイン/テレポート後の数十秒だけ shadow, reflection, impostor, texture creation, far object update を強く抑える。
- FPS が一定以上に戻ったら段階的に解除する。
- ユーザー設定でオン/オフ可能にする。

採用条件:

- 体感 FPS が改善する。
- ロード完了が極端に遅くならない。
- 見た目の劣化が一時的で予測可能。

未実装の重要要素:

- ログイン直後/テレポート直後の時間窓 guard。
- FPS 回復後の段階解除。
- guard 状態のログ出力。
- AutoTune と AYA guard の優先順位。
- ユーザーが UI から現在 guard 中か確認できる表示。
- guard のヒステリシス。現状の即時 `gFPSClamped < AYALoadingFPSGuardMinFPS` 判定は採用前 blocker とする。

この Phase で初めて「ロード専用モード」と呼べる。現状の変更は、低 FPS を検知した時に個別処理を絞る対症療法であり、ロード開始前から P0/P1 を守る完全なスケジューラではない。

#### 別案: 読み込み中だけ内部 3D 解像度を下げる

AYAstorm/Firestorm には、ウィンドウサイズを変えずに 3D scene の内部 render target 解像度だけを下げる既存設定がある。

- `RenderResolutionDivisor`
  - `1` が等倍。
  - `2` なら 3D scene を 1/2 解像度で描画する。
- `RenderResolutionMultiplier`
  - `1.0` が等倍。
  - `0.5` なら 3D scene を半解像度で描画する。
  - `RenderResolutionDivisor > 1` が有効な場合は divisor 側が優先され、multiplier は使われない。

実装上は `LLPipeline::resizeScreenTexture()` / `LLPipeline::allocateScreenBufferInternal()` で world view サイズから縮小した `mRT->screen` / `mRT->deferredScreen` を確保する。UI は別経路で描画されるため、狙いとしては「ウィンドウと UI はそのまま、3D 描画だけ低解像度」が可能である。

描画負荷が下がる根拠:

- `LLPipeline::resizeScreenTexture()` は `getWorldViewWidthRaw()` / `getWorldViewHeightRaw()` を元に、`RenderResolutionDivisor` または `RenderResolutionMultiplier` を適用した `scaledResX` / `scaledResY` を計算する。
- `LLPipeline::allocateScreenBufferInternal()` は、その縮小後の `resX` / `resY` で `mRT->deferredScreen` と `mRT->screen` を確保する。deferred path の color/depth render target 自体が小さくなるため、G-buffer 書き込み、HDR scene buffer 書き込み、depth buffer、post-process 入力のピクセル数が減る。
- `llviewerdisplay.cpp` の world rendering は `mRT->deferredScreen.bindTarget()` へ描画し、`renderGeomDeferred()`、`renderDeferredLighting()`、その後の `renderFinalize()` へ進む。つまり UI ではなく world 3D 側が縮小 render target に乗る。
- `renderFinalize()` では `mRT->screen.getWidth()/getHeight()` や source buffer の width/height を使って DoF、tonemap、glow、FXAA/SMAA、最終 fullscreen pass の shader uniform / viewport を組む。内部解像度を `0.5` にすると、対象ピクセル数は概ね `0.5 * 0.5 = 25%` になる。`0.67` なら約 `45%` になる。
- deferred lighting や fullscreen post process が支配的な区間では、fragment shader 実行回数、render target bandwidth、MSAA/resolve/post-process の読み書きが減るため、FPS 改善が期待できる。

一方で、これは万能ではない:

- mesh/object update、texture fetch/decode、GL texture create、callback、statesort の CPU 側 queue そのものは、内部 3D 解像度を下げても直接は減らない。
- geometry pass の vertex transform、skinning、draw call 数、state change 数も、ピクセル数ほどは減らない。
- したがって `AYAPerf display` で `lighting`, `geom`, `ui_swap`, GPU/driver stall が支配的なら効果が出やすい。`textures`, `objectlist`, callback queue が支配的なら効果は限定的で、近接優先 scheduler / backpressure / object idle throttle 側を優先する。
- 効果判定は `dynamic_resolution event=enter` 前後で、`lighting_avg/max`, `geom_avg/max`, `ui_swap_avg/max`, `textures_avg/max`, FPS を同時に比較する。FPS だけが上がらず `textures` や `objectlist` が残る場合、「内部解像度低下でロード中fpsを救う」という仮説はその場面では外れであり、実装計画を queue 制御中心に戻す。

調査メモ:

- `RenderResolutionDivisor` は古い Debug Setting であり、Second Life Wiki の Debug Settings にも「3D scene を reduced resolution で描画する divisor」として載っている。
- Firestorm 履歴では `RenderResolutionDivisor` は `FIRE-7066`, `FIRE-11830`, `RenderResolutionDivisor is applied twice` など、通常 UI 機能ではなく debug/render pipeline 設定として修正され続けている。
- `RenderResolutionMultiplier` は 2018-03-27 の Catznip/RLVa 由来コミット `d56b519bed` で「`RenderResolutionDivisor` のより細かい代替」として追加された。
- その追加コミットは `settings.xml`, `llviewercontrol.cpp`, `pipeline.cpp`, `pipeline.h` だけを触っており、Preferences XUI は触っていない。
- その後の変更も safety guard と二重適用バグ修正であり、表側 UI 化の履歴は確認できなかった。
- `RenderResolutionDivisor` は RLVa から触れる Debug Setting としても許可されている。従来は「視界をぼかす」用途にも使われ、Firestorm サポート文脈では画面がぼやけた時に Debug Settings で default に戻す案内がある。

なぜ表側 UI にないと見られるか:

- 画質が露骨にぼやけ、一般ユーザーが常用設定として触ると誤解や事故が起きやすい。
- 起源が Debug/RLVa/特殊効果寄りであり、Firestorm の通常 Graphics 品質 UI として設計された機能ではない。
- divisor は整数で粗く、multiplier は後付けの Catznip パッチで、UX として段階・説明・復帰条件が整備されていない。

AYAstorm での扱い案:

- 常時ユーザー設定として単に表へ出すより、loading FPS guard の一部として「読み込み中だけ一時的に下げる」方が筋がよい。
- ログイン/テレポート直後の時間窓は「低 FPS 時に発動を許す期間」として扱い、時間窓だけでは内部 3D 解像度を下げない。実際の発動は `loading_window && fps < AYALoadingFPSGuardMinFPS` を基本条件にする。
- FPS が回復し、texture/object/shadow queue が落ち着いたら `1.0` へ段階復帰する。
- 発動/復帰時は `gResizeScreenTexture = true` により render target 再確保が起きるため、切り替え頻度は抑える。ヒステリシスと最小維持秒数が必須。
- 効果判定は FPS だけでなく、`lighting`, `geom`, `ui_swap`, `statesort`, `textures`, GPU/driver stall の変化を見る。CPU bound 区間では効果が薄い可能性がある。
- UI や HUD の可読性は維持される想定だが、3D world 内の看板、名前表示、mouselook 時の視認性は劣化するため、P0/P1 の可読性確認が必要。

2026-06-02 実装メモ:

- `RenderResolutionMultiplier` をユーザー設定値として runtime 書き換えする経路は撤回する。ただし独自の effective multiplier を別経路で render target 計算へ合成するのも撤回する。既存 `RenderResolutionMultiplier` と違う合成/viewport 経路になると、3D render 部だけが不自然に拡大されたり、カメラ位置がずれたように見える UX 悪化を起こし得る。
- AYA loading 用倍率は保存設定ではなく `LLPipeline` の内部一時状態として持つ。guard 有効中だけ、保存済み `RenderResolutionMultiplier` と AYA loading 用倍率の小さい方を `LLPipeline::RenderResolutionMultiplier` の cached 値そのものへ反映する。これにより既存の `resizeScreenTexture()` / `allocateScreenBufferInternal()` 分岐だけを通し、window/world view/UI サイズは変えず、3D render target だけを縮小する。
- 既存仕様どおり `RenderResolutionDivisor > 1` が有効な場合は divisor 側が優先される。発動/復帰時は `gResizeScreenTexture = true` で screen/deferred render target の再確保だけを要求する。
- 既定では `AYALoadingDynamicResolutionEnabled=TRUE`, `AYALoadingDynamicResolutionMultiplier=0.67`。発動条件はログイン直後、テレポート中/到着直後、または texture create/downscale/callback backlog が残っている loading window 中に `AYALoadingFPSGuardMinFPS` 未満へ落ちた場合とする。`loading_window` 単独では発動させない。
- `AYALoadingDynamicResolutionLowFPSEnabled=FALSE` を追加した。低 FPS だけを理由に通常操作中も render target を縮小する運用は、検証時に「通常操作のどのタイミングで解像度が切り替わるか」を切り分けづらい。既定では OFF にし、まずローディング実態がある場合だけを検証対象にする。
- 低解像度化は user-facing な `RenderResolutionMultiplier` setting を保存値としては上書きしないが、guard 有効中は `LLPipeline::RenderResolutionMultiplier` の cached 値そのものへ一時反映する。独自の effective multiplier を別経路で解釈すると、既存 `RenderResolutionMultiplier` 使用時より合成/viewport 体験が悪化し得るため、不採用とする。つまり world view/window/UI のサイズは変えず、既存の `LLPipeline::resizeScreenTexture()` / `LLPipeline::allocateScreenBufferInternal()` で 3D render target だけを縮小し、最終合成で raw world view に拡大するのが正しい挙動である。
- 復帰条件は `AYALoadingDynamicResolutionMinHoldSeconds` 経過後、loading window 終了、または loading window 中でも `AYALoadingDynamicResolutionRestoreFPS` 以上へ回復した場合とする。低 FPS 単独 trigger を明示的に有効化した場合のみ、通常操作中でも `AYALoadingDynamicResolutionRestoreFPS` 以上への回復まで保持する。切り替え時は render target 再確保を要求するため、最小維持秒数で連続 resize を避ける。
- ログは `AYALoadingFPSGuard` タグで `dynamic_resolution event=enter|exit|active` を出す。`multiplier`, `render_resolution_multiplier_setting`, `ayastorm_dynamic_multiplier`, `cached_render_resolution_multiplier`, `loading_window`, `startup_window`, `teleport_window`, `texture_backlog_window`, `fps` を確認する。
- 実機ログで `fps=170.89` かつ `startup_window=true` のみを理由に `event=enter reason=loading_window` が出た。これは「ローディング中の FPS 低下時だけ使う」という計画から外れていたため、`reason=loading_low_fps` でのみ入るよう修正対象とした。
- build 検証では arm64 Release の `AYAstorm.app` 生成まで通した。実機検証では、ログイン/テレポート直後に `event=enter` が出ること、視界が一時的に低解像度化すること、回復後に `event=exit` が出ることを確認する。

#### ResolutionMultiplier と FXAA の viewport 不整合

2026-06-03 の実機切り分けで、`RenderResolutionMultiplier` による低解像度化時の「画面が拡大され、カメラ位置がずれたように見える」現象は、アンチエイリアスが FXAA の場合に発生し、AA 無効または SMAA では発生しないことが確認された。これは設定ファイル混入ではなく、FXAA pass 固有の viewport 不整合で説明できる。

原因:

- `LLPipeline::resizeScreenTexture()` / `LLPipeline::allocateScreenBufferInternal()` は `RenderResolutionMultiplier` 適用後の低解像度 render target を確保する。
- `LLRenderTarget::bindTarget()` は bind した FBO に合わせて `glViewport(0, 0, mResX, mResY)` を設定する。
- しかし FXAA 経路の `LLPipeline::applyFXAA()` は `dst->bindTarget()` 後に、`gViewerWindow->getWorldViewRectRaw()` 由来の raw world viewport を `gGLViewport` と `glViewport()` に再設定していた。
- その結果、低解像度 FBO に対して raw world viewport で fullscreen triangle を描くことになり、FXAA 出力だけが拡大/位置ズレしたように見える。
- SMAA 経路は各 pass で `bindTarget()` の viewport をそのまま使うため、同じ `RenderResolutionMultiplier` 条件でもこのズレが出にくい。

修正方針:

- FXAA pass では `dst->bindTarget()` が設定した低解像度 FBO の viewport を維持する。
- `gGLViewport` は final backbuffer 復帰時に使われる raw world viewport として残す。FXAA pass 内で低解像度値へ書き換えない。
- FXAA の `width` / `height` と `mFXAAMap` の比率から作る sampling uniform は render target サイズ基準のまま使う。`dst` と `mFXAAMap` が同じ縮小率なら `scale_x/scale_y` は `1.0` になり、低解像度画像を同じ構図で処理できる。

#### AYAstorm と Firestorm のユーザー設定混在メモ

2026-06-03 の実機確認で、Firestorm Viewer 側の Debug Settings に AYAstorm の設定名が見える状態は確認された。これは `~/Library/Application Support/Firestorm/user_settings/settings.xml` を共有している可能性を示すが、上記の FXAA 固有ズレの主因ではない。

扱い:

- `RenderResolutionMultiplier` の画面ズレは「FXAA でのみ発生し、AA 無効/SMAA では発生しない」ため、設定ファイル混在より FXAA viewport 不整合を優先して修正する。
- macOS の保存先初期化 `indra/llfilesystem/lldir_mac.cpp` を `APP_NAME` に変更する案は、Firestorm/AYAstorm の設定分離としては検討余地があるが、今回の FXAA 修正とは分離する。
- 現時点では保存先参照を `Firestorm` 固定の既存挙動に戻し、RenderResolution の視覚不具合修正へ影響させない。
- 既に混在した AYA 追加キーが Firestorm 側に見える問題は、別タスクとして扱う。RenderResolution の検証では `RenderFSAAType=1` かどうか、`RenderResolutionMultiplier` の値、`RenderResolutionDivisor=0` であることを先に確認する。

#### Texture cache の open 失敗後ログ嵐

2026-06-03 の実機ログで、`/Volumes/AdobeCache/Firestorm/texturecache/texture.entries` に対する `Too many open files` の直後、`llapr.cpp write : apr mFile is removed by somebody else. Can not write.` が大量発生した。

原因整理:

- `LLTextureCache::openHeaderEntriesFile()` は `LLAPRFile` の作成後、実際の `apr_file_open()` が失敗して `mFile == NULL` でも `LLAPRFile*` を返していた。
- 呼び出し側の `writeEntryToHeaderImmediately()`, `readEntryFromHeaderImmediately()`, `writeEntriesAndClose()` などは、その返り値を有効な file として扱い、NULL file に対して read/write を呼んでいた。
- `LLTextureCache::openFastCache()` も同様に、fast cache の open 失敗後に `mFastCachep` を保持し得るため、後続の fast cache read/write が NULL file に進む余地があった。

修正方針:

- header entries / fast cache の open 失敗時は `LLAPRFile` を即 delete し、ポインタを `NULL` に戻す。
- 呼び出し側は open 失敗を cache I/O 失敗として扱い、read/write/seek に進まない。
- これにより file descriptor 枯渇そのものを直接解決するというより、枯渇発生後に cache I/O が無限に壊れた file handle を叩き続け、ログとフレーム時間をさらに悪化させる経路を止める。

## 比較計測計画

他メタバースとの比較をするなら、体感ではなく以下の条件で測る。

### 共通条件

- キャッシュ削除済み初回ロード。
- 同じネットワーク環境。
- 同じ表示解像度。
- 同じ FPS 計測方法。
- 1% low FPS、最低 FPS、平均 FPS、ロード完了秒数を取る。

### AYAstorm / Second Life

- ログイン直後またはテレポート直後から 120 秒計測。
- リージョン名、視界内 avatar 数、グラフィック設定、キャッシュ状態を記録。
- `AYAPerf` ログでセクション別 ms を記録。

### VRChat

- ワールド入室直後から 120 秒計測。
- avatar 表示設定、Performance Rank filtering、avatar download size 制限を記録。
- 何体が fallback/blocked になったか確認。

### Roblox

- StreamingEnabled の有無が分かる体験を選ぶ。
- 入室直後から 120 秒計測。
- ストリーミング中に未ロード領域が非表示か、プレースホルダーかを観察。

### Unity/Unreal 系

- ロード画面中と、実ワールド表示中を分けて計測。
- 可能ならデバッグ表示で async loading / streaming status を確認する。

## 結論

現時点の結論は以下。

1. 他メタバースが読み込み中に高 FPS に見えるのは、非同期ロード、チャンクストリーミング、フォールバック、重いコンテンツの非表示、ロード画面の利用による可能性が高い。
2. その一方で、ユーザーが見ている FPS が「実ワールド描画中の FPS」ではなく「軽いロード画面の FPS」である可能性もある。
3. Second Life/AYAstorm は、読み込み中でも実リージョンの object/texture/avatar/render 更新をメインループ上で進めるため、ロードと描画が同じフレーム予算を奪い合いやすい。
4. AYAstorm の修正は、単に速くするより先に「ロード中に何を遅らせるか」「何を仮表示するか」「何を描かないか」を設計する必要がある。
5. 次の実装は、計測ログを基準に、Texture、Object idle、Shadow/Impostor を個別に検証し、有効なものだけ採用する。
