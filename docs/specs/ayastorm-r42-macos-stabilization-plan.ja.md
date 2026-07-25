# AYAstorm R42 macOS 安定化 修正計画

- **状態**: 計画。未実装。
- **対象ブランチ**: `feature/ayastorm-r42-macos-stabilization`
- **基準 revision**: `97712b6a9a` (`feature/ayastorm-r42-phase2` から作成)
- **対象環境**: macOS 15.7.7 / Apple M2 Pro / arm64 / MoltenVK (`AYAstorm-r42-phase2 42.1.0.81626`)
- **調査根拠**: 2026-07-26 01:47 JST の実行ログ、同時刻の macOS crash report、および現 revision の実装。

## 1. 目的と境界

macOS 版で発生した、ワールド移動中の著しいフレーム不整合と、その後の異常終了を安定化する。主目的は FPS 数字の見せ方ではなく、**移動中に毎 frame 発生する描画 work と GPU 待ちを減らし、長い frame をなくすこと**である。

この計画は次の三つを**別問題として**扱う。

1. `VK_ERROR_DEVICE_LOST` を検出した Viewer が自動終了すること。
2. その終了処理中に `LLVertexBuffer` 破棄で `SIGABRT` になること。
3. 移動中の描画 work / GPU 待ちにより、右上 FPS 表示より実際の移動体感が低いこと。

`VK_ERROR_DEVICE_LOST` から Vulkan device を再生成してセッションを継続する recovery は採用しない。既存方針どおり、device lost 時は安全にログアウト・終了する。ただし、**安全終了がクラッシュしてはならない**。

今回のスコープに含めないものは、描画品質を下げる設定変更、LOD/描画距離による見かけ上の FPS 改善、ネットワーク timeout の対症療法、過去に撤去済みの skin A/B オラクルの再導入である。

描画効率の判断順序は次のとおりとする。

1. **停止を除去する** — 不要な swapchain 再作成、`vkDeviceWaitIdle()`、device-lost 後の無効な Vulkan work をなくす。
2. **不要な work を除去する** — 移動で dirty になる geometry のうち、空の draw map・同一世代の再build・不要な upload/record を特定して消す。
3. **残る work を畳む** — なお支配的な draw record / submit が残る場合だけ、batching・cache・indirect 化を検討する。

右上 FPS はこの判断を補助する観測値に過ぎない。UI の数字を先に変えて体感との差を隠すことはしない。

## 2. 今回確定した事実

### 2.1 終了要求はユーザー操作ではない

実行ログでは 01:47:33 に次の順で記録されている。

1. PresentEngine の `vkQueueSubmit()` が `sr=-4` を返す。
2. `VK_ERROR_DEVICE_LOST` を検出する。
3. `LLVKLoader::beginFrame()` が device-lost hook を実行する。
4. `LLAppViewer::requestQuit()` が呼ばれ、通常の cleanup に入る。

根拠となる実装は次である。

- `indra/llrender/llvkloader.cpp:837-878` — `vkQueueSubmit()` と `VK_ERROR_DEVICE_LOST` の検出。
- `indra/llrender/llvkloader.cpp:5199-5211` — device-lost hook の実行。
- `indra/newview/llappviewer.cpp:3839-3845` — hook が `requestQuit()` を呼ぶ。

macOS crash report の末尾にある `-[NSApplication terminate:]` は、AppKit の終了処理の末尾であり、ユーザー操作の証拠ではない。

### 2.2 終了中の crash は別の欠陥である

crash report の main thread は、`LLVertexBuffer::~LLVertexBuffer()` の `unmapBuffer()` から `std::terminate()` へ至っている。

一方、`LLViewerWindow::shutdownGL()` は `gGL.shutdown()` の直後に `LLVertexBuffer::cleanupClass()` を実行する。

- `indra/newview/llviewerwindow.cpp:2757-2761`
- `indra/llrender/llvertexbuffer.cpp:666-674`
- `indra/llrender/llvertexbuffer.cpp:743-761`

`llrender.cpp` の process-static な `sVBCache` は `LLPointer<LLVertexBuffer>` を保持しているが、明示的な shutdown 時 clear を持たない。

- `indra/llrender/llrender.cpp:69-75`
- `indra/llrender/llrender.cpp:1782-1816`

したがって static destructor が `LLVertexBuffer::cleanupClass()` より後に走る余地があり、クラッシュ時の順序と一致する。これは既存の Vulkan teardown 統一設計 (`docs/vknative_teardown_shutdown_design.md`) が対象にした deferred Vulkan 資源プールとは別の、**C++ static cache の所有期間**の問題である。

### 2.3 FPS 表示と体感の乖離は測定方式と実際の停止の両方で起きている

右上 FPS は直近フレーム周期の中央値を 1 秒ごとに表示する。

- `indra/newview/llstatusbar.cpp:610-628` — `getPeriodMedianPerSec()`。

中央値は一部の長いフレームを隠すため、カクつきの有無を表す指標ではない。対してログの FPS は 10 秒間のフレーム数であり、同じ区間で `15.6–20.0 FPS`、period jitter は最大 `1.6054` を記録した。

さらに、移動中の実行ログでは `createSwapchain` が約 1〜2 秒ごとに繰り返されていた。swapchain 再作成は `peDrain()` と `vkDeviceWaitIdle()` を呼ぶため、GPU の未完了 work を待つ停止点になる。

- `indra/llrender/llvkloader.cpp:4392-4429`
- `indra/llrender/llvkloader.cpp:5220-5245`

再作成要求は resize、acquire の `VK_SUBOPTIMAL_KHR` / `VK_ERROR_OUT_OF_DATE_KHR`、present の同結果から立つ。現ログには要求理由が出ていないため、**連続再作成の直接原因は未確定**であり、先に計測で確定する。

## 3. 完了条件

| 項目 | 合格条件 |
|---|---|
| device lost 時の終了 | `VK_ERROR_DEVICE_LOST` 検出後に `requestQuit()` へ入り、`SIGABRT` / crash report を出さずに終了する |
| 通常終了 | 連続 10 回の起動・ログイン後終了で `LLVertexBuffer` destructor 由来の crash が 0 件 |
| cache の寿命 | `sVBCache` が `LLVertexBuffer::cleanupClass()` より前に空になり、二度目の clear も安全 |
| swapchain | 静止したウィンドウで再作成が継続しない。移動 20 分の再作成回数と理由をログから説明できる |
| 移動時の描画 work | `stateSort`、geometry rebuild、upload、draw record、GPU wait のそれぞれを時間で説明できる。空 draw map や同一世代の再build を減らしても視覚結果が同一 |
| フレームペーシング | 右上の単一 FPS とは別に、長い frame を識別できる記録が残る。修正後は同一シナリオで p95/p99 frame time、再作成停止時間、geometry rebuild 時間を比較できる |
| macOS 実機 | arm64 build、codesign、起動、ワールド移動、テレポート、通常終了を通す |

FPS の「何 FPS なら合格」は、現時点で固定しない。会場・アバター数・通信状態で変わるため、まず P0 の同一シナリオ基準値から p95/p99 frame time の改善率と停止回数で判定する。

## 4. 実装順序

### P0 — 観測を固定する（先行、挙動変更なし）

**目的**: device lost、swapchain 再作成、移動中の描画 work を推測で扱わない。

#### 変更候補

- `indra/llrender/llvkloader.cpp`
  - `sSwapchainRecreatePending` を立てた理由を enum/bitset で保持する。候補は `resize`、`acquire-suboptimal`、`acquire-out-of-date`、`present-suboptimal`、`present-out-of-date`、`vsync-setting`。
  - `recreateSwapchain()` ごとに、理由・旧/新 extent・経過 frame・`peDrain` と `vkDeviceWaitIdle` の所要時間を一行で記録する。
  - `VK_ERROR_DEVICE_LOST` 時は、失敗した submit が frame job か one-shot job か、queue submit の直前の job 数を一度だけ記録する。失敗後に追加 submit や fence wait を行わない。
- `indra/newview/llviewerdisplay.cpp`
  - 既存 10 秒 FPS ログと同じ周期で、平均・p95・p99 frame time と最大 frame time を診断ログへ追加する。右上の UI はこの phase では変更しない。
- `indra/newview/pipeline.cpp` と既存の VkPerf/CPU profile scope
  - `stateSort`、visible group の `rebuildMesh()`、geometry upload、draw command record、PresentEngine wait を同じ frame 番号で時間計測する。
  - 既存の `visible empty-drawmap groups` と `VKC-SUM` は回数の手掛かりとして残すが、時間計測なしに主因とは断定しない。

#### gate

- log は swapchain が連続再作成されたときも無制限に増えない（理由変化時 + 10 秒集計）。
- 追加後の通常起動・通常終了に新しい validation error がない。
- 「resize 起因か」「acquire/present 起因か」「device lost 直前にどの job が失敗したか」を一回の再現ログで判定できる。
- 移動シナリオの frame time を、`swapchain wait` / `geometry rebuild` / `upload` / `draw record` / その他へ分解できる。

### P1 — 頂点バッファ cache の終了順序を正す（最優先）

**目的**: device lost の有無にかかわらず、終了時に `LLVertexBuffer` が生きた Vulkan バッファ管理の外側で破棄されないようにする。

#### 実装方針

- `indra/llrender/llrender.{h,cpp}` に `LLRender::clearVertexBufferCache()`（名称は実装時に既存命名へ合わせる）を設ける。
  - `sVBCache.clear()` を唯一の本体とする。
  - `LLRender::shutdown()` から必ず呼ぶ。
  - 冪等にし、cache clear 後に destructor が走っても何もしない状態にする。
- `indra/newview/llviewerwindow.cpp`
  - `gGL.shutdown()` と `LLVertexBuffer::cleanupClass()` の順序を維持し、前者の内部で cache clear が完了することをログ/assert で確認する。
- `indra/llrender/llvertexbuffer.cpp`
  - device-lost cleanup 中の `mMapped` buffer 破棄を実トレースする。`unmapBuffer()` が lost device で Vulkan work を submit/wait しないことを確認し、必要なら既存の lost cleanup 状態に従う no-op 経路を設ける。

#### 不変条件

1. `sVBCache` の最後の `LLPointer<LLVertexBuffer>` は `LLVertexBuffer::cleanupClass()` より前に release される。
2. cache clear は render thread / worker 停止後、かつ VertexBuffer pool・megabuffer 停止前に一度だけ完了する。
3. static destructor の実行順に安全性を依存しない。

#### gate

- 通常終了と `VK_ERROR_DEVICE_LOST` 終了で、crash report の `LLVertexBuffer::~LLVertexBuffer` → `std::terminate` が再発しない。
- `sVBCache` 件数を debug log で確認し、`LLVertexBuffer::cleanupClass()` 直前は 0。
- macOS のみでなく Linux / Windows の通常終了 build も通す。ここは共通 `llrender` の寿命修正であり、OS 分岐を増やさない。

### P2 — device-lost 終了経路を現行 teardown と接続確認する

**目的**: 既実装の `ReapMode::Lost` と今回の static cache 修正を、一つの安全終了契約にする。

#### 実装・確認箇所

- `indra/llrender/llvkloader.cpp`
  - `sVkDeviceLost` が立った後は新しい submit、`vkDeviceWaitIdle()`、fence wait を行わないことを P0 のログとコードで確認する。
  - `shutdownVulkan(device_lost=true)` の producer quiesce → lost reap → persistent resource の破棄順を、`docs/vknative_teardown_shutdown_design.md` の契約と再照合する。
- `indra/newview/llappviewer.cpp`
  - device-lost hook から `requestQuit()` へ入る経路を唯一の入口として保つ。UI の「ユーザーが終了した」表示や通知を追加しない。
- `indra/newview/llviewerwindow.cpp` / `indra/llrender/llrender.cpp`
  - P1 の cache clear が、この lost teardown の前半で完了することを保証する。

#### gate

- device lost を意図的に再現できない場合も、P0 の失敗注入または Vulkan call wrapper のテストで、lost 後に submit/wait をしないことを検証する。
- device recovery を追加しない。終了コード、ログアウト、settings 保存の既存意味を変えない。

### P3 — swapchain 再作成ループを原因別に止める

**目的**: 毎秒級の `vkDeviceWaitIdle()` 停止を除去する。

#### 原因別の処置方針

| P0 で確定する原因 | 実装候補 | 境界 |
|---|---|---|
| resize 通知が同じ extent を繰り返す | macOS callback から渡す backing-pixel extent を一本化し、同値通知を無視する | `llwindowmacosx*` と `llviewerwindow.cpp`。macOS 限定 |
| logical point と backing pixel の比較不一致 | `CAMetalLayer.drawableSize`、`LLWindowMacOSX::getSize()`、`notifyWindowResize()` をすべて backing pixel にそろえる | macOS/MoltenVK 限定。表示・入力の座標系は別途維持 |
| `VK_SUBOPTIMAL_KHR` / `VK_ERROR_OUT_OF_DATE_KHR` が連続する | 再作成を要求した event を一回に畳み、最新 extent が安定するまで cooldown する | `llvkloader.cpp` 共通。ただし macOS で実測 gate |
| setting 変更が原因 | settings 変更一回につき一回だけ recreate を許す | `setVsyncEnabled()` と関連設定経路 |

P0 で上表のどれにも当たらない場合は、原因を「未確定」のまま修正しない。`vkDeviceWaitIdle()` を単に外す、再作成を時間で握り潰す、present mode を強制変更することは禁止する。

#### gate

- 静止状態で swapchain 再作成 0 回/10 分。
- 通常の resize・フルスクリーン切替・VSync 切替で、必要な一回の再作成後に描画と入力座標が正しい。
- 移動 20 分の同一シナリオで、recreate 回数・wait 時間・p95/p99 frame time を P0 と比較する。
- `VK_ERROR_DEVICE_LOST` が再発した場合、P0 の job 情報と swapchain reason を添えて別途根因を追う。swapchain 修正だけで device lost 解決を宣言しない。

### P4 — 移動時の geometry / draw work を削減する

**目的**: P0 の実測で支配的と判明した移動時の CPU/GPU 描画 work を、画質を落とさず削減する。

#### 先に確定すること

- `pipeline.cpp::stateSort()` の visible group ごとに、dirty にした理由、`rebuildMesh()` 実行有無、draw map の有無、CPU 時間を対応付ける。
- group の再buildが主因でなければ、P0 で最大時間を占めた upload / draw record / GPU pass を次の標的にする。`VKC-SUM` の fired 数や draw 数だけで標的を選ばない。
- device lost 直前の重い frame について、P0 の submit job と各 work 時間を突き合わせる。重い frame と device lost の因果は、同一の再現ログが得られるまで仮説として扱う。

#### 原因別の実装方針

| P0 で確定する支配 work | 実装候補 | 禁止事項 / gate |
|---|---|---|
| 空の draw map を持つ group の再build | draw map が空なら mesh rebuild / upload を行わない条件を、その group の dirty 世代と対にして導入する | 単なる件数上限で先送りしない。次 frame での描画欠落・object pop-in なし |
| 同一 dirty 世代の重複 rebuild | group ごとの geometry generation / completed generation を明示し、完了済み世代を再実行しない | dirty flag を早期 clear して更新を落とさない。移動・TP・object update の視覚同一 |
| upload が主因 | dirty な buffer 範囲だけを upload し、同一内容の upload を省く。既存 deferred-free / frame-in-flight 契約を維持する | lost device 時に upload/submit しない。buffer lifetime を短絡しない |
| draw record が主因 | pipeline / vertex/index buffer / material が連続する run を実測に基づき畳む。既存 batching/indirect の経路を再利用する | alpha 順序、rigged skin、region 行列の前提を崩さない。計測なしの全面 MDI 化をしない |
| GPU pass が主因 | pass ごとの GPU timestamp で対象を決め、同一入力の重複 pass・不要な resolve/clear を除く | LOD、shadow 品質、描画距離を下げて合格にしない |

#### gate

- 同一移動シナリオで、支配 work の CPU/GPU 時間または実行回数が P0 基準から減少する。
- p95/p99 frame time が改善し、画質・alpha 順序・avatar skinning・object update に回帰がない。
- work budget は burst の安全弁としてのみ扱い、steady-state の恒常的な無駄を隠す手段には使わない。

### P5 — FPS 表示は診断値として維持する（UI 改修は本計画の対象外）

**目的**: 性能改善の結果を誤読しないために、長い frame をログで追跡可能にする。

- P0 の frame time 集計を性能比較の真実源とする。
- 既存の中央値 FPS 表示は互換性のため変更しない。
- tooltip / debug-only 補助表示は、P4/P6 の実測を読みにくい場合にだけ別作業として起票する。本ブランチの合格条件には含めない。

### P6 — macOS 実機の受入と回帰

**ビルド**: `ARCHS=arm64` の Release app を作成し、`lipo -archs` と `codesign --verify --deep --strict` を確認する。

**シナリオ**:

1. 起動、ログイン、静止 10 分。
2. ワールド内を連続移動 20 分（region 境界を含む）。
3. テレポートを連続して実行し、asset/texture 読み込みが重なる状態を作る。
4. VSync ON/OFF、ウィンドウ resize、Retina display / 外部 display の切替（利用可能な範囲）。
5. 通常終了を 10 回繰り返す。
6. device lost が再現した場合は、クラッシュせず正常に終了し、P0 の最終ログを保存する。

**記録する値**: average/p95/p99/max frame time、10 秒 FPS、swapchain reason ごとの回数と wait 時間、`stateSort` / rebuild / upload / draw record の時間、device-lost の job 種別、RSS、crash report の有無。

## 5. 優先度と依存関係

```text
P0 観測
 ├─ P1 cache 終了順序 ─ P2 device-lost 安全終了 ─ P6 受入
 └─ P3 swapchain 停止除去 ─ P4 geometry/draw work 削減 ─ P6 受入
                         └─ P5 診断値維持（UI改修なし）
```

P1 は P3 より優先する。device lost の根因が残っても、現在は安全終了すらクラッシュするためである。P3 と P4 が**描画効率を上げる本線**である。P3 は P0 の理由記録なしに、P4 は P0 の時間内訳なしに着手しない。P5 は性能改善ではなく、測定の透明性を維持する補助である。

## 6. リスクと判断基準

| リスク | 対応 |
|---|---|
| device lost の原因を swapchain と断定してしまう | P0 の submit job 情報と recreate reason を先に取る。因果がなければ別 ticket に分離する |
| static cache clear が通常描画中に走る | public shutdown API からだけ呼び、frame 中呼び出しを assert する |
| lost device に対して unmap/wait/submit する | `device_lost` を teardown の明示的な分岐に通し、P2 の call trace で禁止する |
| Retina 座標修正で UI/入力がずれる | drawable extent と UI/input の座標系を混ぜない。resize、外部 display、fullscreen を実機 gate に含める |
| FPS 数字だけを上げて体感を隠す | FPS UI は変更しない。LOD・描画距離・強制 present mode は本計画では触らず、tail frame time と実 work 時間を受入値にする |

## 7. 既存設計との関係

- `docs/vknative_teardown_shutdown_design.md` の `ReapMode::Lost`、producer quiesce、lost 時に wait しない契約を維持する。本計画はそこに含まれていない `sVBCache` の C++ 所有期間を補う。
- `docs/vknative_rigged_indirect_design.md` に記録された過去の skin A/B オラクル由来 device lost は撤去済みである。今回の device lost は同じ原因とみなさず、P0 で current build の submit 失敗を特定する。
- 共通 renderer の cache 寿命修正は Linux / Windows にも有益だが、swapchain の extent/resize 修正は `LL_DARWIN` 境界に閉じる。共通 `llvkloader.cpp` へ追加する観測は OS 非依存に保つ。

## 8. 実装開始時のチェックリスト

- [ ] P0 のログ設計をレビューし、常時ログ量の上限を決める。
- [ ] P1 で `sVBCache` の全所有者・thread を再検索する。
- [ ] `LLVertexBuffer::unmapBuffer()` の lost-device 挙動を call trace で確認する。
- [ ] P2 で lost 後の Vulkan submit/wait が 0 であることを確認する。
- [ ] P3 は P0 の原因別ログを添えてから着手する。
- [ ] P4 は P0 の work 時間内訳を添えてから着手する。
- [ ] P6 の実機ログ、crash report、arm64/codesign 結果を同じ revision に紐づけて保存する。
