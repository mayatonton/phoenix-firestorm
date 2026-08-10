# AYAstorm R42: `LLImageGLThread` の旧 OpenGL state 整理

## 状態

**実装前・文書化済み。**

対象は `feature/ayastorm-r42-macos-arm64-moltenvk-validation` に取り込んだ
最新 `feature/ayastorm-r42-phase2` である。本書は arm64 開発 app の build を阻害した
`LLImageGLThread` の未使用 private field を、Vulkan worker 化の履歴に照らして判断した記録である。

## 観測した build failure

arm64 / Release の `ayastorm-bin` build は `indra/llrender/llimagegl.cpp` の compile で停止した。

```text
indra/llrender/llimagegl.h:321:15: error: private field 'mWindow' is not used [-Werror,-Wunused-private-field]
indra/llrender/llimagegl.h:322:11: error: private field 'mContext' is not used [-Werror,-Wunused-private-field]
```

これは runtime の device lost や MoltenVK 初期化失敗ではない。macOS の build flags が warning を
error として扱うため、dead state の検出で build が停止している静的な failure である。

## 調査結果

### 元の役割

`LLImageGLThread` は、2021 年の multi-threaded OpenGL 実装で導入された。`mWindow` は
`LLWindow` を保持し、`mContext` は共有 OpenGL context を保持して、worker thread 内で次を行っていた。

1. `mWindow->createSharedContext()` で shared context を作る。
2. `mWindow->makeContextCurrent(mContext)` で worker に current context を与える。
3. `gGL.init()` 後に texture task を実行する。
4. shutdown 時に `gGL.shutdown()` と `mWindow->destroySharedContext(mContext)` を行う。

従って当時は両 field に実行上の役割があった。

### 現行の役割

最新 phase2 の texture worker は、OpenGL context を使わない Vulkan upload worker である。
`LLImageGLThread::run()` は次の順序になっている。

1. `LLVKLoader::registerGpuUploadWorker()` で worker 専用の Vulkan command-pool 経路を登録する。
2. `LL::ThreadPool::run()` で task を実行する。
3. `LLVKLoader::unregisterGpuUploadWorker()` で worker を解除する。

OpenGL shared context の create/current/init/shutdown/destroy は現行経路からすべて除去されている。
全参照を確認した結果、`mWindow` は constructor initializer で保存されるだけ、`mContext` は
宣言されるだけで、どちらも読み出しは 0 件だった。

## 判断

| 選択肢 | 判定 | 根拠 |
| --- | --- | --- |
| field を実際に使う | 不採用 | OpenGL shared context を Vulkan worker に戻すことになり、現行の worker 設計と MoltenVK 経路の分離に反する。 |
| `[[maybe_unused]]` 等で抑制する | 不採用 | 条件付きの一時的な未使用ではなく、機能撤去後の dead state である。誤った設計を将来へ残す。 |
| field と不要になった引数を削除する | 採用 | Vulkan worker の実際の責務と一致し、OpenGL/Vulkan の実行経路を変えずに build failure を解消する。 |

`mFinished` も現行コードでは代入のみで読み出しがない。しかし今回の compiler failure の対象は
`mWindow` と `mContext` であり、変更範囲を固定するため `mFinished` はこの修正に含めない。

## 実装範囲

次の狭い変更だけを行う。

1. `indra/llrender/llimagegl.h` から `LLImageGLThread::mWindow` と `mContext` を削除する。
2. `LLImageGLThread` の constructor から `LLWindow*` 引数を削除する。
3. `LLImageGL::initClass()` と `indra/newview/llviewerwindow.cpp` の呼び出しを、不要になった
   `LLWindow*` を渡さない形へ合わせる。

Vulkan worker の registration、queue、submit、texture upload、または shutdown 順序は変更しない。

## プラットフォーム境界と検証

`llimagegl` と `llviewerwindow` は macOS・Linux・Windows の共通 translation unit である。変更は
不要なポインタ state と引数の整理のみで、runtime の分岐を増やさない。

- macOS: arm64 / MoltenVK の `ayastorm-bin` build を完了し、開発 app を手動で起動して login screen と
  Vulkan presentation surface を確認する。
- Linux / Windows: 各プラットフォーム担当者が build と Vulkan run を別途確認する。macOS の起動結果を
  他プラットフォームの runtime 検証とは扱わない。

コード修正は上記 macOS の手動起動確認が成立するまで commit しない。文書 commit とコード commit を
分離する。
