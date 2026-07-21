# VK 資源ライフサイクル統一設計(C = TP 信頼性 / churn・close・device-lost 一本化)

**status**: 設計合意済(AYA 2026-07-21)→ 実装 GO(gate チェック不要・完了時報告)。**HEAD `7eca3d8356` を file:line で実トレースして作成。**
**位置づけ**: E 系 closed 後の最終 TOP。散在した teardown/reap を**単一機構**に束ね、不要処理を整理して安定動作へ収束させる。
**AYA 厳命**: 「OS が回収するからいい」= 悪。全 VK 資源は device 破棄前に所有者が明示解放。**バラバラなものにバラバラな処理を当てない**(= 今の破綻の原因)。現象非再現でも fail-open PASS しない = correct-by-construction。検証はテスターへ移譲(検証のみ台帳)。

---

## 0. 統一の核(設計合意)

**TP region churn の資源解放と app close の teardown は同一機構。Close = その reap を全資源に回し切って最後に device を手放す = タイミングの違いだけ。** device-lost はその reap を「fence 待ち放棄=unconditional」モードで回して Close するイベント。

→ **1 つの reap 機構・3 つの運転モード**:
| モード | 契機 | fence gate | スコープ | 末尾 |
|---|---|---|---|---|
| **churn** | TP/毎frame | 有(`enqueue_frame ≤ sLastCompletedMonotonic`) | 部分(完了分) | 継続 |
| **close** | quit/init失敗 | waitidle 後は全完了扱い | 全 | vkDestroyDevice |
| **lost** | device-lost | **無**(clock 凍結・全 in-flight を破棄可とみなす) | 全 | vkDestroyDevice(recovery 時は device 再生成へ) |

---

## 1. 現状マップ(トレース済・file:line)= 散在と重複と取りこぼし

### 1.1 資源プール(遅延破棄の器)= 9 本
`sPendingBufferFrees`(522)/ `sPendingImageFrees`(531)/ `sPendingObjectFrees`(541・pipeline/shader/layout)/ `sPendingOneShotFrees`(552・cmd/staging/fence)/ `sPendingSlotFrees`(298・bindless slot)/ `sPendingDrawDataSlotFrees`(308)/ `sPendingMegaFrees`(mega-buffer)/ `sPendingOcclusionQueryReleases`(1119)/ +cache: PerDrawUBOArena・SharedDynamicPersistentUBOs・ScenePerDrawDescriptorCache。

### 1.2 enqueue(捨てる側・所有者から)
`destroyBufferVk`(7276)/ `destroyImageVk`(8339・`++gVkViewDestroyGen`)は即 free せず `enqueue_frame=sMonotonicFrameCount` 付きで push。呼び手 = `LLImageGL::~`(llimagegl.cpp:1087 他 13)/ `LLCubeMap`(llcubemap.cpp:69 他)/ `LLRenderTarget`(llrendertarget.cpp:78)/ `LLGLSLShader`(llglslshader.cpp:411,423)/ `llviewertexturelist`(1350,1471,1490)。**TP evict = ここが大量発火**。

### 1.3 reap(通常運転)= 中央 1 クラスタ(beginFrame 内 4551-4560)
```
tickDeferredBufferFreeQueue / ...ImageFreeQueue / ...ObjectFreeQueue /
tickMegaFreeQueue / tickDeferredQueryReleaseQueue / tickOneShotFreeQueue /
tickSharedDynamicPersistentUBOs / tickPerDrawUBOArena / tickScenePerDrawDescriptorCache
```
各 tick は `enqueue_frame ≤ sLastCompletedMonotonic` のみ実行(7304/8372…)。**通常運転はここで束ねられている(良い先例)。**

### 1.4 fence クロック = `sLastCompletedMonotonic`
GPU frame 完了 signal で前進(4496-4498/5067-5069・fence 経由)。**device-lost で凍結 → 1.3 の reap が永久に止まる。**

### 1.5 ★破綻の実体 = teardown が reap を別実装で再現(shutdownVulkan 3975-4038)
- sDevice ブロック内で `sPendingBufferFrees`(3982)/`sPendingImageFrees`(3988)/`sPendingObjectFrees`(4000)/`sPendingOneShotFrees`(4021)を**手書き force-destroy**(1.3 の重複・分岐した別コード)。
- `sPendingOcclusionQueryReleases.clear()`(4020)= **free せず捨てるだけ**。
- `sPendingMegaFrees`/`sPendingSlotFrees`/`sPendingDrawDataSlotFrees` は**この経路で扱わない**(heap/mega 一括破棄に暗黙依存)。
- = **9 本に対しバラバラな 3 種の扱い(手書き free / clear のみ / 無視)**。

### 1.6 ★producer quiesce が遅い(3954-3974)
worker hook 停止・`rwStop`(3972)・`peStop`(3973)・`texWorkerShutdown`(3974)が **shutdownVulkan 奥**。cleanup の資源解放(shutdownGL 2453 / surface 2455 / window delete 2460)より後 = **worker 生存中に破棄が進む窓** → 共有キュー競合(crash A/SIGBUS)+ shutdownGL 後の `texWorkerUpload` 生成(05137 leak・llvkloader.cpp:7641)。

### 1.7 fence 待ち choke(device-lost で凍る)
frame(4494/5065)・staging 256MB cap で worker(7359)・`texWorkerShutdown` の `vkWaitForFences 1s`(7576)・`peWaitSlotSubmitted`(814)。**lost 時これらを待ってはいけない。**

### 1.8 reload owner = 既に単一(流用)
`LLReloadQueue`(llreloadqueue.cpp・request 6 site @llviewercontrol・drain @llappviewer:1757)。**shader/GLbuffer/resize の reload はここに集約済**。C ではこれを壊さず、同じ「単一 owner」思想を VK reap 側へ適用する。

---

## 2. 破れている不変条件(invariant-first)

- **INV-1 単一書き手**: producer quiesce 完了後、VK object と共有 reap キューに触れる thread は main 1 つ。
- **INV-2 drain-before-destroy**: GPU work が参照し得る object を、その work 完了前に destroy しない。**device-lost 時は「もう GPU は進まない=全 in-flight は完了扱い」**で無条件破棄(lost device の destroy は合法)。
- **INV-3 reap 完全性**: 生成した全 VK object は device 破棄前に唯一の reap 経路で破棄。**OS 回収に依存する object ゼロ(05137=0)。孤児(publish 前 in-flight)も reap 集合に入れる。**
- **INV-4 surface-before-window**: swapchain/surface は native window 破棄前(cleanup 2455 で既成立=形式化)。
- **INV-5 単一機構**: churn / close / lost は同一 reap の 3 モード。**別コード経路(1.5 の手書き重複)を新設・温存しない。**

**⚠️ 禁止(CLAUDE.md 35 行・05137 の教訓)**: leak handle を device 破棄時に力任せ vkDestroy する「強制解放」はしない。05137 は INV-1(quiesce)+INV-3(所有者が生きているうちに reap 集合へ)で構造的に消す。

---

## 3. 目標構造 = 単一 reap driver + 段階 close

### 3.1 単一 driver
```
// 全 9 プールを 1 関数で回す。mode で fence gate を切替。
void reapAllDeferred(ReapMode mode);   // mode: Churn | Close | Lost
```
- 中身 = 1.3 の 9 tick を**この 1 関数へ集約**し、各 tick に `mode` を渡す:
  - `Churn`: `enqueue_frame ≤ sLastCompletedMonotonic`(現行と同一)。
  - `Close`: waitidle 済 → 全件破棄。
  - `Lost`: fence 参照せず全件破棄(vkGetFenceStatus/WaitForFences を呼ばない)。
- **beginFrame の 4551-4560 → `reapAllDeferred(Churn)` 1 行**に置換。
- **shutdownVulkan 3982-4038 の手書き reap を全削除 → `reapAllDeferred(Close/Lost)` へ置換**(1.5 の重複消滅・INV-5)。`sPendingOcclusionQueryReleases` も driver 経由で正規に処理(clear-only を廃止)。

### 3.2 producer quiesce を最前へ(新 entry)
```
void vkQuiesceProducers();  // 冪等。全 VK 触り thread を停止・join。
```
- 中身 = 現 3954-3974(bake/geo/tex hook + rwStop + peStop + texWorkerShutdown)+ voice pump 停止。
- **呼ぶ位置 = cleanup の資源解放が始まる前**(shutdownGL 2453 より前)。shutdownVulkan 内の旧 3954-3974 は冪等 no-op 化(二重停止許容)。
- 出口 assert: main 以外 VK 非参照(INV-1)。

### 3.3 段階 close(全経路統一・device_lost フラグのみ分岐)
```
shutdownVulkan(bool device_lost):
  P1 vkQuiesceProducers()                       // INV-1(冪等・cleanup で既済ならno-op)
  P2 device_lost ? (何もしない) : vkDeviceWaitIdle(sDevice)   // INV-2
  P3 reapAllDeferred(device_lost ? Lost : Close) // 全9プール完全 reap(INV-3・INV-5)
  P4 tex worker 孤児 image の reap(publish 前 in-flight を所有・05137 根絶)
  P5 persistent/owned 破棄 leaf→root            // fallback/heap/pool/sampler/UBO ring/sync/query/cmdpool/allocator
  P6 (surface/swapchain は cleanup 2455 で済・ここは no-op 確認)  // INV-4
  P7 vkDestroyDevice → debug messenger → instance → volkFinalize  // 出口 05137=0
```
- init 失敗経路(3883-3933)も同 API(device_lost=false)。

### 3.4 device-lost funnel(abort 廃止)
- `beginFrame` の `LL_ERRS`(4446-4450)を撤去 → device-lost 検知時は当該 frame を安全に抜け、`LLAppViewer` に graceful quit を要求 → cleanup が `shutdownVulkan(device_lost=true)` を通す。**abort による生存 thread 巻き込み(crash B の一因)を断つ。**
- (b)device recovery = P3/P4 の reap 集合を入力に device 再生成 → 全資源 reload。**本設計はモード・機構を用意するのみ・実装は別決裁**(a=clean 終了を実装)。

---

## 4. 削除・整理リスト(束ねつつ不要を消す)
1. **shutdownVulkan 3982-4038 の手書き reap 4 種** → `reapAllDeferred` へ吸収(削除)。
2. **`sPendingOcclusionQueryReleases.clear()`(4020)の free せず捨て** → driver の正規 reap へ。
3. **fence 待ち choke の lost 対応**: 7359/7576 等は `device_lost` 時 skip(待たない)。
4. **beginFrame の 2 系(threaded/inline)の reap 重複**があれば 1 本化(3.1 で吸収)。
5. producer 停止の二重箇所(cleanup 前倒し後の 3954-3974)→ 冪等 no-op。

## 5. 実装段取り(直列・工事中のみ kill switch・完了後即削除)
- **S1**: `reapAllDeferred(mode)` 抽出(9 tick 集約)+ beginFrame を Churn 呼びに置換。**churn 経路の等価**(視覚同一・05137 増分ゼロ)を先に固める。
- **S2**: `vkQuiesceProducers()` 抽出 + cleanup 最前へ配置・shutdownVulkan 旧停止を冪等化。
- **S3**: shutdownVulkan を §3.3 の P1-P7 へ再構成(手書き reap 削除・`reapAllDeferred(Close)` 化)。**通常 quit で 05137=0・stack_trace 新規ゼロ**を確認。
- **S4**: `device_lost` フラグ配線 + `beginFrame` LL_ERRS → graceful quit funnel。P4 孤児 reap。
- 各 S: JIT で触る file/罠/申告を明記 → 実装 → 突合。工事中 kill switch = `AYASTORM_TEARDOWN_LEGACY`(旧経路退避・S 完了で即削除)。

## 6. gate / 検証(fail-closed・テスター移譲)
- **correct-by-construction**: 各 INV をコードで満たすことを file:line で示す(現象非再現でも設計で担保)。
- **正のオラクル(取れる範囲)**: 通常 quit N 回で **05137=0 実数**・`stack_trace.log` 新規ゼロ・teardown 由来 VVL Threading ゼロ。
- **移譲**: TP 実 churn 下の crash 非再現 / device-lost 実発火は**テスターへ移譲・検証のみ台帳**(再現保証なし・設計正しさは §3 で担保)。register 未証明項併記。

## 7. 申告(縮小・省略・解釈)
- **省略**: device recovery(b)= 実装せず(モード枠のみ)。AYA 別決裁。
- **解釈**: `Lost` モードの「全 in-flight 完了扱い」= Vulkan 仕様(lost device の destroy は合法・新規 submit/wait は不可)に依拠。
- **未決(S4 で確定)**: P4「tex worker 孤児 image」の所有経路(publish queue のどの構造が prepared-but-unpublished を保持するか)は S4 着手時に実トレースで特定。現時点は「reap 集合に入れる」不変条件のみ確定。
- **crash B(voice sSessions 無ロック反復)= 別 TICKET・次 TOP**(§3.4 の funnel で終了時巻き込みは断つが TP 中の反復レースは voice 層内欠陥)。
- CLAUDE.md 28 行 TOP は本作業完了時に C 実態化へ書き換え(AYA 指示済)。
