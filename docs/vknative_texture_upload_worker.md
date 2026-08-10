# VK per-thread command pool 土台 + off-main テクスチャ upload(設計正本)

制定 2026-08-10。真因 = [[handoff_texture_dl_fps_coupling]]。施主指示(2026-08-10)= **足す前に既にある車輪を数えて整理してから設計する**。本 doc が唯一の正本。Brief に設計を書かない。

**v2(2026-08-10 交通整理)**: 初回実装は全 revert 済(HEAD `3199ecaa758` = 現在コードに worker 痕跡なし・upload は main 直列)。本 doc の現在の読み方:
- **PART A〜G = 有効**(per-thread pool 土台の設計正本。段1〜5 実装は判定走行で VERIFIED クリーンだったが IIL 対処と一括 revert = そのまま再実装する)。
- **旧 PART H・PART I = 失敗・削除済**(下記「失敗の記録」に要約。全文は git 履歴と memory `handoff_texture_iil_all_attempts_failed`)。
- **現行設計 = PART J(publish-on-main)**。
- 旧実装 Brief 2 本(`impl_brief_stage1_texture_upload_worker.md` / `impl_brief_partI_slot_fallback_until_ready.md`)= **削除済**(revert 済み実装の指示書 = 混乱源)。再実装 Brief は AYA GO 後に PART J から新規に書く。

---

## PART A. 全体棚卸し(設計前に読んだ範囲と発見)

### A.1 テクスチャ create/upload データフロー(単一経路・重複なし)
`scheduleCreateTexture`(`llviewertexture.cpp:1723`)→ `createTexture` → `createGLTexture`(`llimagegl.cpp:1717/1807`)→ `setImage`(`:802`)→ `commitVkBacking`(`:1481`)→ `mVkRes.commit`(`VkTexResidency`)→ llvkloader `uploadImageDataVk`(`:10142`)= one-shot。
- VK backing は `VkTexResidency mVkRes` 1 本に集約済(residency refactor `3199ecaa758`)。古/新併存なし。
- `commitVkBacking` の 6 caller(setImage×3 / setExternalVkBacking / setSubImageFromFrameBuffer / scaleDown)は各々別操作 = 重複でない。
- llimagegl.cpp の VK/GL 分岐は **5 箇所のみ**(`:364,1457,1483,1492,1501`)= if-sprawl でない(refactor が畳んだ)。

### A.2 llvkloader の VK image 関数群(重複でなく別操作)
`uploadImageDataVk`(2D)/`uploadImageData3DVk`(3D)/`uploadCubeImageDataVk`(cube)/`uploadImageSubregionVk`/`generateMipChainBlitVk`/`downscaleImageVk`/`blitCubeArrayVk`/`create*ImageVk`/`readback*Vk`/`transitionImageLayoutVk`。**全て `beginOneShotCommandBufferVk()`(`:8954`)+ `submitOneShotVk()`(`:8949`)の one-shot primitive を共有** → primitive を直せば一律に直る(集約点が既に 1 つ = 良い構造)。

### A.3 スイッチ棚卸し(施主「複数あるかも」を実測 → 複数化していない)
- **texture threading の user switch = `RenderGLMultiThreadedTextures` 1 個**。reader は `llviewerwindow.cpp:2218`(initClass の `thread_texture_loads` 実引数)**のみ**。settings.xml の 2 件目(`settings.AYA-merged.xml:13425`)は別 file の merged 変種で真の重複でない。
- runtime flag `sEnabledTextures` = 1 flag・多 consumer(`scheduleCreateTexture:1744` / `texturelist:1641,1650` / `startup:627`)= 健全(重複でない)。
- **VK では `:364-368` の override が `sEnabledTextures=false` を強制** = 上の switch を殺している(現状の直列の直接原因)。
- MT master = `AYASTORM_MT_THREADS`(`peStart:1378` / `rwDesiredWorkerCount:1422`)。per-subsystem env = `AYASTORM_MT_PE` / `AYASTORM_MT_RECORD`。
- ⚠️ **前案の `AYASTORM_MT_TEXTURE` 追加は「2 個目の switch」= 撤回**。既存 switch を使い、新 env を足さない。

### A.4 MT 記録インフラの実体(休眠 N-thread 土台)
- per-thread render state は `thread_local` で既に大量保持(`llvkloader.cpp:147-233` = viewport/scissor/matrix ring/bound pipeline/`tRecordCmdOverride`/memo cmd 等)= 「別 Core 描画」時代の土台。
- MT 記録方式 = worker は**事前確保 buffer**(`sConsumerCommandBuffers[]:637` / `sAsyncProducerCommandBuffer:644`)へ `tRecordCmdOverride`(`:219`)で記録。record lane(`sPerDrawDescLanes`)の `.pools` は **VkDescriptorPool** で command pool でない。
- **∴ per-thread command pool は未存在。全 command buffer(frame/consumer/async/one-shot)が単一 `sCommandPool`(`:103`)由来。**

### A.5 発見の要約 = 「2-3 個の車輪」は無い。あるのは①半完成品②単一 pool ボトルネック③小重複④死骸
1. **半完成の per-pool 抽象**: `submitOneShotVkFromPool(cmd, pool, …)`(`:8973`)と `PendingOneShotFree.pool`(`:752`)は per-pool 用に既存。だが caller は全て `sCommandPool` の 1 種(`:8951`)= **供給元だけ欠けた作りかけ**。→ 完成させる(新設でない)。
2. **土台 gap = 単一 `sCommandPool`**。N-thread 記録は Vulkan 仕様上 per-thread pool 必須(`vkCmd*`/`vkBegin` は command buffer とその pool の外部同期を要求)。単一 pool = 再並列化の構造的壁。
3. **小重複**: `AYASTORM_MT_THREADS<=1` 判定が `:1379` と `:1423` で 2 重 → helper 1 本に。
4. **死骸(別掃除案件・本改修に含めない)**: `AYASTORM_TEXPOKE`(`llvovolume.cpp:2534`)+ pokeOK/FB 計器(`llvkloader.cpp:6193`)= 解雇②期の poke 実験残骸。

---

## PART B. 設計 — 半完成の per-thread pool 抽象を「完成」させる(土台整理)

### B.0 単一の解決質問(X)と結論
X =「共有 `sCommandPool` の外部同期を if-sprawl 無しでどう畳むか」。
結論 = **既存の半完成 per-pool 抽象を、"どの pool か" を thread_local 値 1 個で供給して完成させる**(`:147-233` の thread_local 族と同形)。texture 専用機構でなく**汎用 per-thread command pool**として置き、texture worker はその最初の利用者にする。
不変条件: **command pool は各スレッドが排他所有。alloc/record/free は所有スレッドが自分の pool にのみ行う。**

### B.1 per-thread pool 供給(新規の値 1 個・llvkloader.cpp)
```
thread_local VkCommandPool t_cmdPool = VK_NULL_HANDLE;
static VkCommandPool threadCmdPool()          // 唯一の "どの pool か" 決定点
{
    return t_cmdPool != VK_NULL_HANDLE ? t_cmdPool : sCommandPool;
}
```
- main: `t_cmdPool` null → `sCommandPool`(挙動不変)。
- worker: 起動時に自 pool を `t_cmdPool` に設定。
- **これが族別 if を消す唯一の値**。null 比較 1 個で main/worker が同一コードを走る。

### B.2 既存 one-shot を threadCmdPool() 経由に(半完成を完成・caller 全不変)
- `beginOneShotCommandBufferVk()` `:8958`: `cbai.commandPool = threadCmdPool();`
- `submitOneShotVk()` `:8951`: `submitOneShotVkFromPool(cmd, threadCmdPool(), …);`(**既存の pool 引数にただ供給元を与えるだけ**)
- A.2 の ~14 upload caller は無変更。

### B.3 free の完成(既存 `.pool` per-entry を活かす・per-pool bucket)
- `sRetiredMainOneShotCmds`(単一 vector `:760`)→ `std::unordered_map<VkCommandPool, std::vector<VkCommandBuffer>> sRetiredByPool`(`sOneShotMutex` 保護)。
- retirement 判定(timeline 完了)は pool を触らない = shared のまま任意スレッド可。push 先だけ pool 別:`:9012`→`sRetiredByPool[wait_entry.pool]`、`:9082`→`sRetiredByPool[e.pool]`。
- `tickOneShotFreeQueue()` `:9062/9095/9097`: `free_pool = threadCmdPool(); free_now.swap(sRetiredByPool[free_pool]); vkFreeCommandBuffers(sDevice, free_pool, …);`
- **全スレッドが同一 tick を走り、free は必ず自 pool の retired だけ**。族別分岐ゼロ。bucket は pool 数無制限 = N-thread に無改修で載る。

### B.4 汎用 worker pool API(llvkloader・texture 専用でない)
- `bool LLVKLoader::registerThreadCmdPool();`= 自 pool 作成(`createCommandPool :2321-2329` と同 flags: `TRANSIENT|RESET_COMMAND_BUFFER`, `sGraphicsQueueFamily`)+ `t_cmdPool` 設定。worker run() 冒頭で呼ぶ。
- `void LLVKLoader::unregisterThreadCmdPool();`= 自 pool の one-shot drain(自 bucket 空 + 自 pool 分 pending の timeline 完了待ち)→ `vkDestroyCommandPool` → `t_cmdPool=null`。worker run() 末尾。
- **将来 geo/draw worker も同 API を呼ぶだけ**で同機構に載る(§D)。

### B.5 MT master helper(小重複 A.5-3 を畳む)
- `:1379` と `:1423` の `AYASTORM_MT_THREADS<=1` 判定を `static bool mtMasterThreaded()` 1 本に集約し両所から呼ぶ。

---

## PART C. texture worker の起動(既存 LLImageGLThread を再活性・switch 再利用)

### C.1 死んだ GL bracket を VK pool に置換(if 不要・GL は物理削除済)
- `LLWindowSDL2::createSharedContext()`(`llwindowsdl2.cpp:2536`)= `return nullptr`。`makeContextCurrent`/`destroySharedContext` = no-op。
- ctor(`llimagegl.cpp:2597-2606`)は `mContext==null` を「context 非対応→自己無効化」と誤解釈 → **VK で素朴に有効化すると即死**。この early-return を撤去。
- run() `:2618` の `gGL.init(false)`(`llrender.cpp:800`)= **global gGL の blend/cull/ambient state を worker から書く = main の共有 state 破壊バグ**。VK は upload に gGL 不使用 → 撤去。
- 置換: ctor の `createSharedContext` + null early-return 撤去。run() の GL bracket 撤去 → 冒頭 **`LLVKLoader::registerGpuUploadWorker(GPU_SUB_IMAGE, 0)`**(統一 API・foundation §5.3.1 = 内部で `registerThreadCmdPool()`)、末尾 **`unregisterGpuUploadWorker()`**(内部で `unregisterThreadCmdPool()`)。GL は物理削除済 = strategy 不要、VK 一本。
- ✅ **H1 解決済(2026-08-10 トレース)= texture worker は buffer domain 非接触を確認**: `tAllocDomain` を使うのは `megabufAcquireVertex/Index`(mesh)/`drawDataAcquireSlot`(描画記録)/setter/test の 5 関数のみ=全て texture 経路外。llimagegl.cpp(create/setImage/commitVkBacking/VkTexResidency)は buffer アロケータ呼び出しゼロ。画像 upload primitive 6 本も mega 非接触。worker が触る共有状態は per-thread(`t_cmdPool`)/ mutex(`sBindlessSlotMutex`/`sPendingImageFreeMutex`/`sSet1BirthMutex`)/ atomic(residency view/slot)で全て保護 = race なし。

### C.2 enable = 既存 switch 再利用(新 env 追加なし・A.3)
- `llimagegl.cpp:364-368` の VK override を撤去し、既存の `thread_texture_loads`(=`RenderGLMultiThreadedTextures`)を MT master と AND:
  ```
  // VK/GL 共通の一本化: sEnabledTextures = 設定 && MT master threaded
  const bool tex_on = thread_texture_loads && LLVKLoader::mtMasterThreaded();
  if (tex_on) LLImageGLThread::createInstance(window);
  LLImageGLThread::sEnabledTextures = tex_on;
  LLImageGLThread::sEnabledMedia    = false;   // media 対象外(§申告)
  ```
  (GL 版の `gGLManager.mGLVersion>3.95` gate は GL 物理削除で無意味 = 一本化で消える。)
- switch OFF → `sEnabledTextures=false` → `scheduleCreateTexture` else 枝(`:1799`)= 現行 main create = 挙動不変の安全 fallback。
- `RenderGLMultiThreadedTextures` の comment が GL 専用("multiple render contexts")= VK 実態に更新(cvar は GUI 到達性で存置。撤去可否は別途)。
- worker→main finalize は既存機構が再活性: `update_texture_fetch()`(`llstartup.cpp:627`)が `sEnabledTextures` 時に mainloop を 1ms pump = `postCreateTexture`(`llviewertexture.cpp:1787-1796`)が main で走る。追加配線不要。

### C.3 teardown 順序(要配線)
- worker join(= 自 pool destroy 完了)は `LLVKLoader::shutdownVulkan()`(`llappviewer.cpp:2847`・`sDevice` 破棄)より**前**。
- 配線: `shutdownVulkan()` 直前に `LLImageGL::cleanupClass()`(`:377`→`LLImageGLThread::deleteSingleton()`→ join → run() 末尾 `unregisterThreadCmdPool` で自 pool destroy)を呼ぶ。
- `vkDestroyCommandPool` は in-flight cmd 不可 = unregister 内で自 one-shot の timeline 完了待ち。
- device-lost/`sReapForceAll`: free は `threadCmdPool()` の bucket 限定 = 既 destroy の worker pool を触らない(worker 不在)。

---

## PART D. 将来の N-thread 描画への接続(施主意図・恒久)
- 本改修で **off-main の image upload(one-shot cmd)** の安全 primitive が完成: **任意の worker が `registerThreadCmdPool()` を起動時に呼ぶだけ**で自 pool を得、`beginOneShotCommandBufferVk`/`submitOneShotVk`/`tick` を無変更で使える。
- ⚠️ **適用範囲 = one-shot command buffer を使う upload に限る**(image = texture/cubemap/RT)。**mesh VB/IB は mega-buffer 永続 map へ memcpy 直書き(`flush_vbo` llvertexbuffer.cpp:1120)= cmd buffer 不使用 = 本 primitive の対象外**。mesh off-main は別 primitive(mega-buffer slice 確保のスレッド安全化)= foundation doc Stage 4。
- **次の移行先(本改修では触らない・load-bearing)= frame 記録経路**: `sConsumerCommandBuffers[]` / `sAsyncProducerCommandBuffer` は現状 `sCommandPool` 由来。再並列化で consumer/producer を別スレッド化する時、両者を各自 `threadCmdPool()` へ移せば同機構で per-thread 化できる(今は main 単独=直列ゆえ latent・触らない)。
- kill switch(`AYASTORM_MT_THREADS` 族)は撤去しない = 再並列化の実験足場。[[project_serial_recording_is_temporary]]。

### D.1 DL パイプライン全体の散らばり(施主観察 2026-08-10「Mesh も Animation もあらゆる DL がごちゃごちゃの可能性」)
実測: DL/fetch は資産種別ごとに独立の `LLThread` 派生 = **統一基盤なし**(`LLMeshRepoThread`(mesh)/ `LLTextureFetch`+`LLTextureCache`+`LLImageDecodeThread`(texture)/ `LLKeyframeMotion`+asset system(animation))。施主直感は妥当 = 各パイプが個別に mess。ただし**2 層に分けて扱う**:
- **層α = GPU image upload/submission**(本 doc の per-thread pool 土台)= **one-shot cmd を使う image upload(texture/cubemap/RT)の横断 primitive**。mesh VB は cmd buffer 不使用ゆえ**対象外**(訂正済・上記 PART D の警告)。**今ここを正しく作るのが image 系への payoff**。
- **層β = CPU 側 DL/fetch/cache/decode パイプライン**(fetch thread・cache・decode の口が main 等)= 資産種別ごとの別 audit。**本 texture 改修に畳み込まない**(ocean を沸かさない)。やるなら「DL パイプライン統一 audit」として別途スコープを切る(前任解雇の逆 = 巨大化して逃げない為、まず texture=層α を閉じる)。
- ∴ 本改修の位置づけ = **層α の土台を texture を最初の利用者として据える**。層β の全種別統一は施主が別途決裁する独立課題。

---

## PART E. texture 参照の集約(施主「あちこちから引いてて気持ち悪い」の実体・独立ステップ)

### E.1 発見(実測)= 本物の重複車輪 1 件
per-sampler の view 解決が **2 つの descriptor set 構築関数で drifted copy-paste**:
- `LLRenderPass::buildAndOverrideScenePerDrawSet`(`lldrawpool.cpp:650`・MDI per-draw 経路・ループ本体 `~985-1100`)
- `LLGLSLShader::populateAndBindUniversalDescriptorSet`(`llglslshader.cpp:3616`・universal/非MDI 経路・ループ本体 `~3740-3874`)

核心(view 解決 → dim 照合 → `vkc_fb_reason` 判定 → `LLVKContract::note*` → sdim 別 fallback view 選択)が**ほぼ同一**。特に fallback view 選択(`lldrawpool:1087-1091` ≡ `llglslshader:3842-3846`)と dim/reason 判定(`:1045-1072` ≡ `:3794-3827`)は verbatim。
**差分(保存すべき)**: universal 側のみ ①`enum_value==-2`(immediate index unit = N-100)`:3750-3758,3781-3784` ②`isImageViewActivePassAttachment` inline check `:3795-3799` ③source char が小文字 `'f'/'i'/'l'/'c'`(MDI は大文字 `'E'/'R'/'F'/'L'`= debug で経路判別)④`live_view()` lambda vs `gGL.getTexUnit()->getLiveVkImageView()`。

### E.2 集約設計 = 共有 helper 1 本に抽出
`llglslshader.cpp`(両者が include 可能・LLGLSLShader 静的 or free 関数)に:
```
struct VkSamplerResolve {
    VkImageView view;          // 解決 or fallback 後
    VkSampler   sampler;       // fallback shadow / l3 / unit 由来
    S32         resolved_unit; // -1 or unit
    bool        l3_hit;
    bool        used_fallback;
    const char* fb_reason;     // nullptr / "attachment" / "no_view" / "dim_l3" / "dim_unit"
};
// N・channel・enum_value と「immediate index を許すか(universal のみ true)」を入力に、
// view 解決 → attachment 無効化 → dim 照合 → VKContract note(検出器 semantics 厳密保存)
// → sdim 別 fallback 選択 までを一括。record_ref/source char は呼び手が付ける(経路差)。
static VkSamplerResolve resolveVkSamplerBinding(
    LLGLSLShader* cur, U32 N, S32 channel, S32 enum_value,
    bool allow_index_unit, VkImageView fallback_view,
    const std::function<VkImageView(U32)>& live_view);
```
- 両 caller はこの helper を呼び、返り値から `bindings.sampler_*` を埋める。**source char・record_ref・can_pin 等の経路固有処理だけ呼び手に残す**(差分④③②①を呼び手 or 引数で吸収)。
- **VKContract 検出器(`note`/`noteFbNoView`/`noteFbSlot`)の呼び条件・引数を 1 bit も変えない**(憲法4 = 検出器の semantics 保存。呼ぶ場所を helper に移すだけ)。→ 実装後に**独立監査で「検出器発火が改修前後で完全同一」を照合**(A/B で fbslot/no_view カウント一致)。

### E.3 ⚠️ 段取り = **PART B-D(upload worker)とは別実装ステップ**
- 理由: (a) upload 経路と bind 経路は独立 (b) 検出器隣接の render hot path = 巨大 risky diff を混ぜない (c) 独立監査は別命題(upload=concurrency / 集約=検出器等価)。
- **推奨順序**: ①PART B-D(upload worker)を gate PASS → ②PART E(集約)を別 commit。E は「掃除」ゆえ B-D の payoff を阻害しない範囲で後追い。
- E も本 doc が正本(Brief に設計を書かない)。

## PART F. 不要処理の掃除(施主指示「要らなくなった処理は掃除」)
**原則(scope 規律)= 自分の変更が dead 化したものを掃除する。他者の裁定待ち失敗実験は勝手に消さず AYA に上げる。**
### F.1 本改修で dead 化 → 掃除する
- LLImageGLThread の GL context bracket(`createSharedContext`/`makeContextCurrent`/`gGL.init`/`gGL.shutdown`/`destroySharedContext`)= GL 物理削除 + VK pool 化で完全 dead(§C.1)。
- initClass の `gGLManager.mGLVersion > 3.95f` gate(`llimagegl.cpp:372-373`)= GL 物理削除で常に無意味 → switch 一本化で消える(§C.2)。
### F.2 掃除するが AYA 裁定を仰ぐ(coverage-loss 申告・憲法)
- `AYASTORM_TEXPOKE` + `LLVOVolume::pokeTEImage`(`llvovolume.cpp:2531`)+ pokeOK/FB 計器(`llvkloader.cpp:6193`)= memory 記録「poke 不発(pokeOK 0.1%)・revert 推奨で AYA 裁定待ち」。**本改修と無関係の失敗実験ゆえ勝手に消さない** → AYA 裁定(revert 可否)を仰ぐ別案件として起票。
### F.3 掃除しない(現役)
- `RenderGLMultiThreadedMedia`/`sEnabledMedia`(media 経路で現役・`llviewermedia.cpp:3103`)。

## PART G. 自己監査(correctness / gap / hidden — 出す前の必須)
- **[correctness-1] free の per-pool 分離**: retirement 判定(timeline 完了)は shared・pool 非依存 = 任意スレッド可、free(`vkFreeCommandBuffers`)は `threadCmdPool()` の bucket 限定 = 所有スレッドのみ。∴ 外部同期不変条件が成立。✅ ただし **hidden**: worker が死んだ後に main tick が worker pool bucket を掴む競合 → §C.3 で「unregister が自 bucket を drain してから destroy」= worker 生存中に自 bucket を空にする。main は `sRetiredByPool[sCommandPool]` しか触らない(threadCmdPool()=sCommandPool)ので worker bucket を掴まない。✅
- **[correctness-2] `sReapForceAll` teardown**: worker pool destroy 後に main が force-reap すると `sRetiredByPool` に worker pool entry が残っていれば destroy 済 pool へ free = crash。→ **gap**: unregister で自 pool の pending+retired を完全 drain してから destroy を厳守(未 drain entry を残さない)。設計に明記済(§B.4/§C.3)だが**実装 Brief に「unregister は自 pool 由来の sPendingOneShotFrees も待つ」を明示要**。
- **[correctness-3] `submitOneShotVkFromPool` の backpressure**(`:8978-8024`)は `sPendingOneShotFrees` を pool 横断で見て front を wait/erase する。worker が front(=main pool 由来)を erase して `sRetiredByPool[main]` へ回すと、**worker が main pool の retired bucket に push**する。→ push は mutex 下で安全だが、その cmd の free は「main が自 tick で main bucket を free」= 所有スレッド free 不変条件は保たれる(push はスレッド跨ぎ可・free だけ所有スレッド限定)。✅ **hidden 確認**: backpressure wait 中の `vmaDestroyBuffer`(`:9006`)は staging buffer(pool 無関係・VMA は内部同期)= スレッド安全。✅
- **[gap-1] `peEnqueue` の非 threaded 経路**(`:1323-1328` sPERunning=false で inline peExecute + 非 lock `++sTimelineNext`): `AYASTORM_MT_PE=0` かつ texture worker ON だと worker が非 lock で timeline 採番 = race。→ **設計要件追加**: texture worker enable は `mtMasterThreaded()` に加え **PE threaded(`sPERunning`)を前提**にする(PE off なら worker off へ縮退)。または worker submit も peEnqueue の lock 経路を通す保証。実装 Brief で明示。
- **[gap-2] initClass 呼び出しタイミング**(`llviewerwindow.cpp:2218`)で `sDevice`/`sCommandPool` が既に ready か = worker が registerThreadCmdPool で pool 作成する時 sDevice 必須。initClass は VK init 後のはず(要 file:line 確認 = 実装 Brief の前提検証項目)。
- **[hidden-1] source char の経路差(E.1 差分③)**を helper 集約で潰すと debug 判別が消える → **helper は source を決めず呼び手が付ける**設計(§E.2)で保存済。✅
- **[hidden-2] `isImageViewActivePassAttachment` check が universal のみ(E.1 差分②)**: MDI 側に無いのは意図的か bug か未確定 → **集約時に「MDI にも入れる」と挙動変化**。∴ helper は差分②を**引数フラグで on/off**し、両 caller の現挙動を厳密保存(集約は等価変換に限定・挙動変更は別 issue)。
- **残 open(実装 Brief で潰す)**: gap-1(PE 前提)/gap-2(initClass タイミング)/correctness-2(unregister drain 完全性)。いずれも file:line で決定可能 = 壁でない。

### G.1 open 3 点の解決(file:line 確定・2026-08-10)
- **gap-1 解決 = texture worker enable に `sPERunning` を要件化**: `peStart()`(`llvkloader.cpp:1376-1392`)は `AYASTORM_MT_THREADS<=1` or `AYASTORM_MT_PE=0` で早期 return し `sPERunning` を false のまま残す。off-main の `submitOneShotVk`→`peEnqueue` は `sPERunning==true` の時のみ locked 採番(`:1330-1338`)= race-free。false 時は inline 非 lock(`:1323-1328`)= worker から呼ぶと race。∴ `textureWorkerEnabled() = thread_texture_loads && LLVKLoader::peThreaded()`(`peThreaded()` = `sPERunning` 参照)。**master + MT_PE を 1 条件(sPERunning)に畳む**(§C.2 の `mtMasterThreaded()` AND は sPERunning で代替 = より正確)。PE off の debug 時は texture worker も off 縮退(現行 main create)。
- **gap-2 解決 = VK は initClass 前に full ready**: `initVulkan()`(`llappviewer.cpp:4043`)→ 内部で device/`sCommandPool`(`:2329`)/`peStart`(`:5046`)完了。その後 `LLImageGL::initClass`(`llviewerwindow.cpp:2218`)。∵ `:364` の VK override が発火する事実 = `isVulkanInitialized()==true` at initClass = VK ready 確定。worker の `registerThreadCmdPool`(pool 作成)は更に後(thread run 起動時)= sDevice 確実。
- **correctness-2 解決 = unregister の drain 手順**: `unregisterThreadCmdPool()` は ①自 pool へ新規 submit を止め(worker run() 末尾ゆえ以降 submit なし)②自 pool 由来の `sPendingOneShotFrees` の最大 `timeline_value` を取り、`gpuTimelineValue()` がそれ以上になるまで `tickOneShotFreeQueue()` を回す ③`sRetiredByPool[myPool]` を最終 free ④`vkDestroyCommandPool(myPool)`。既存 `gpuTimelineValue()`/`tickOneShotFreeQueue()` で決定可能・新 primitive 不要。

## ⛔ 失敗の記録(旧 PART H / 旧 PART I / inline mip fold = 3 手全滅・2026-08-10・全 revert 済)
worker 有効時の `UNASSIGNED-CoreValidation-DrawState-InvalidImageLayout`(IIL)に対し 3 手(段6/6b read-gate = 旧 PART H / heap fallback+promote = 旧 PART I / inline mip fold)を実装したが**いずれも IIL を消せず全 revert**。全文 = git 履歴 + memory `handoff_texture_iil_all_attempts_failed`。**旧 H/I を参照設計にするな。**
- 負の確定事実: ①使用側 gate(view()/slot()/heap descriptor fallback+promote)をいくら足しても IIL は消えない(classic 経路 view() を gate した段6b でも 3 件残存)②upload→mipchain の 2-submit ordering も真因でない(fold で継続)③PE FIFO・upload 遷移・promote proxy は VERIFIED 正 → **真因は当時の全モデルの外**(残候補 = VkImage/VkImageView handle 再利用の validation semantics / stale DrawData slot / draw 以外の CB)。
- 教訓: GPU 完了 gate(mReadyFrame)は**過剰要件だった** — layout の正しさに要るのは GPU 完了でなく **submission order**(pipeline barrier は CB 内でなく queue 全体・submission 順に効く)。

## PART J. 現行設計 = publish-on-main(INV-1・2026-08-10 制定)

### J.0 不変条件(INV-1)と設計の核
**INV-1: texture の可視化(publish = mView store + heap slot 書込)は main のみ・当該 backing の全 subresource を SHADER_READ_ONLY へ遷移させる one-shot の PE FIFO enqueue 後にのみ行う。**
- 根拠 = HEAD 全数列挙(J.1): 可視化チョークポイント = `VkTexResidency::publish`(`llimagegl.cpp:1366`)の **1 点**・全消費者(bindless heap + classic per-draw descriptor)はその下流。∴ publish 1 点の規律で全消費者が一括で閉じる。
- 旧 H/I との構造差 = **gate 全廃**。worker 側の可視化(descriptor 書込・mView swap・旧 backing destroy)を**存在ごと消す**。worker は「create + upload/mipchain enqueue」まで。完了 backing は既存の worker→main callback(`postCreateTexture` 経路)で main が受け取り publish する。
- 正しさ(順序): worker の完了 post は upload enqueue 後 → main の publish はさらに後 → 可視化後の全 frame submit は PE FIFO 上 upload submit より後 = validation・実データとも READ_ONLY。**GPU 完了待ちは不要**(上記教訓)。白期間 ≈ 1 frame 未満。

### J.1 HEAD 書込面の全数列挙(2026-08-10 実測・HEAD `3199ecaa758`)
- 可視化 = publish 1 点(:1366 = mView :1384 + `bindlessAcquireSlot` :1371→`bindlessWriteSlotInternal` llvkloader.cpp:12255)。heap 直書きの他 2 箇所は NULL 書きのみ(:3394 init / :10006 reclaim)。`bindlessUpdateSlot`(:12259)= **呼び手ゼロ = dead → 削除候補**。
- publish 入口 8: syncVulkan2DImage :1235(upload→commit ✓)/ :1140 **⚠ data==nullptr = upload ゼロで commit = UNDEFINED のまま可視化 = INV-1 潜在違反** / 3D :1341 ✓ / setExternalVkBacking :1363(cubemap llcubemaparray.cpp:172・RT llrendertarget.cpp:264 = 外部 layout 管理)/ setSubImageFromFrameBuffer :1706 **⚠ FB copy は mip0 のみ READ_ONLY(copyColorImageRegionToImage2DVk :11589 levelCount=1)= want_mips>1 なら mip1+ が UNDEFINED のまま = 潜在違反** / scaleDown :2564(downscale 最終 layout = Brief 検証項目)/ resample :1416 ✓ / ensureSlot :1431(mView 非 NULL のみ ✓)。
- 消費者: bindless heap 全 draw + classic per-draw descriptor(llrender.cpp:437 / llglslshader.cpp:2289,2298 / lldrawpool.cpp:632)= 全て mView/mSlot 経由。

### J.2 機構(if-sprawl なし・単一値比較 1 箇所)
1. seam = `commitVkBacking`(:1481)のみ: `isUploadWorkerThread()`(= `t_cmdPool != VK_NULL_HANDLE`)なら publish せず staged 領域に保持(mVkRes 不変・draw は既存の白 fallback :1455-1479 が継続)。
2. staged→publish: 既存 worker→main callback(`postCreateTexture` llviewertexture.cpp:1787)で main が staged を commit(publish)。新規 queue 機構は作らない(既存 postTo pattern 再利用)。
3. sampler は publish 時に `getSamplerForState` 再計算(`mSampler` は dead = 旧監査 g-1 の知見を継承)。

### J.3 潜在違反の是正(worker と独立の実在 gap・同工事で閉じる)
- :1140(data==nullptr)= commit 前に全 mip UNDEFINED→SHADER_READ_ONLY 遷移 one-shot を enqueue(`transitionImageLayoutVk`)。
- :1706(FB copy mips>1)= Brief で呼び手を実測し、mips=1 強制 or copy 後 mip chain 生成のどちらかに確定。

### J.4 IIL 再発時の観測契約(観測可能にせよ・AYA 承認事項)
3 手全滅の真因は静的解析の外(失敗の記録)。再実装の判定走行で IIL が再発した場合に備え、`vkDebugCallback` の IIL 分岐で Object 群(CB handle・image handle)を registry と突合し「frame CB か one-shot か / どの texture か / handle 再利用(直近 destroy と同 handle)か」を名指しする**工事用計器**を Brief に含める(検知装置ドクトリン = 症状があるのに装置が沈黙 → 伸ばすのは装置)。装置 diff = 憲法 4 = **AYA 承認必須**。

### J.5 工程(再実装)
- 段1〜5 = per-thread pool 土台 + LLImageGLThread VK 化 + enable + teardown(PART B〜G 準拠・初回実装で VERIFIED クリーンだった形の再実装)。
- 段6' = 本 PART J(publish-on-main)。旧 段6/6b/PART I は実装しない。
- gate = 層0-2 + `fb_heap_default` 有意減 + IIL 0 + 正のオラクル(tex enq/pub)。PASS は AYA のみ。

### J.6 自己監査(correctness/gap/hidden)
- [c-1] INV-1 順序 = J.0 の submission order 論証で成立。✓
- [c-2] 白 fallback 継続 = mVkRes 未 touch ゆえ mView==NULL → :1455-1479 既存経路。✓
- [g-1] 二重作成(施主厳命)= stage〜publish の窓で同 texture の再 create が起きないこと。`scheduleCreateTexture` の single-flight(mNeedsCreateTexture)が構造保証するかを **Brief 検証項目**に(崩れる場合は can_reuse に staged 参照を足す)。
- [g-2] staged backing の lifetime = main callback 不達(texture 死亡・shutdown)時の staged destroy。`retire` との interplay 含め Brief で file:line 確定。
- [g-3] scaleDown の最終 layout(J.1)= Brief 検証項目。
- [h-1] stage〜publish 窓の setSubImage 等の変異は旧 backing に当たり publish で上書きされる(≈1 frame・streaming で不可視)= 申告。

### J.7 詳細設計(実装確定・HEAD `3199ecaa758` 全 file:line 検証済 2026-08-10)
**原理 = VkTexResidency の既存分離を完成させる: mCur = backing 記録(create 経路の内部読み)/ mView・mSlot = 公開可視性(全 draw 消費者)。staging = 「mCur は進め、mView/mSlot は main まで保留」。**

1. **llvktexresidency.h**: `hasBacking()`(= `mCur.valid()`)公開 / `mStagedPrev`(VkBacking)+ `mStagedValid`(bool)追加 / `publishStaged(sampler, want_slot)` / `discardStaged()` 宣言。dead な `mSampler` は削除。
2. **create 経路の判定是正(staging の前提)**: `can_reuse`(llimagegl.cpp:1096)と mip>0 の `mip_no_base` 判定(:1129)の `mVkRes.isLive()` → `mVkRes.hasBacking()`。直列では mView≡mCur.view で等価・staged 中は「backing は在るが未公開」を正しく表す(これを怠ると worker create 2 mip 目で二重作成 = 施主厳命違反)。setSubImageFromFrameBuffer の `need_new`(:1648)も同型 → hasBacking() へ。
3. **commit の staging 分岐(seam・単一値比較)**: `VkTexResidency::commit` 冒頭で `LLVKLoader::isUploadWorkerThread()` なら: prev=mCur → mCur=next → (初回 staged) mStagedPrev=prev・mStagedValid=true /(同 create 内 2 回目)prev(未公開)を `destroyImageVk` 即時 deferred 破棄 → return true。main は従来 publish。
4. **publishStaged(main)**: mStagedValid 消費 → want_slot なら `bindlessAcquireSlot(mCur.view, sampler)`(**失敗時は slot=INVALID のまま view を公開** = ensureSlot の既存 lazy 再取得に委ねる・publish の fail-return と異なる = 申告)→ mSlot/mView store → mStagedPrev(owned)を deferred 破棄 → 旧 slot deferred release。呼び手 = `LLImageGL::publishStagedVkBacking()`(want_slot/sampler を commitVkBacking と同式で再計算)← `postCreateTexture`(llviewertexture.cpp:1666・mNeedsCreateTexture guard 通過後)。
5. **discardStaged(main・失敗系)**: 未公開の新 backing(mCur)を deferred 破棄し mCur=mStagedPrev に復元。呼び手 = `LLImageGL::discardStagedVkBacking()` ← `postCreateTextureFailed`(:1705)。`retire` にも staged 破棄を追加(防御)。
6. **段1 primitive(llvkloader.cpp)**: `thread_local VkCommandPool t_cmdPool`(:749 付近)+ `threadCmdPool()` / begin :8958 = `threadCmdPool()` / submitOneShotVk :8951 = `submitOneShotVkFromPool(cmd, threadCmdPool(), …)` / `sRetiredMainOneShotCmds`(:760)→ `std::unordered_map<VkCommandPool, std::vector<VkCommandBuffer>> sRetiredByPool`(sOneShotMutex 下・push 2 点 :9012/:9082 は `e.pool` キー・tick :9062 = 自 `threadCmdPool()` bucket のみ swap→free)。
7. **worker API**: `bool registerGpuUploadWorker()`(= 自 pool 作成 flags TRANSIENT|RESET(:2325-2326 と同一)+ t_cmdPool 設定)/ `void unregisterGpuUploadWorker()`(= sOneShotMutex 下で自 pool 由来 pending の最大 timeline を取得 → `gpuTimelineValue()` 到達まで tick+200µs wait → 自 bucket 最終 free → `vkDestroyCommandPool` → t_cmdPool=null)/ `bool isUploadWorkerThread()`(= t_cmdPool!=null)/ `bool peThreaded()`(= sPERunning)。※ foundation §5.3.1 の substrateMask 引数は Stage 1 では不要 = 引数なし版で置く(mesh 時に拡張・申告)。
8. **段2 LLImageGLThread VK 化(llimagegl.cpp:2589-2623)**: ctor = `createSharedContext`+null early-return 撤去 / run() = GL bracket(makeContextCurrent/gGL.init/gGL.shutdown/destroySharedContext)撤去 → 冒頭 register(失敗時 WARNS + sEnabledTextures=false で inline 縮退・queue は 服务継続)・末尾 unregister。
9. **段3 enable(:359-375 一本化)**: `tex_on = thread_texture_loads && LLVKLoader::peThreaded()`(peStart :5046 は initClass llviewerwindow.cpp:2218 より前 = 順序確定済)。media = false 固定。GL 分岐(mGLVersion>3.95)は GL 物理削除で無意味 = 撤去。
10. **段4 teardown**: `LLImageGL::cleanupClass()` は現在**どこからも呼ばれていない**(実測)→ llappviewer.cpp:2847 `shutdownVulkan()` 直前に追加(ThreadPool close+join → run() 末尾 unregister が sDevice 存命中に完走)。
11. **段5 settings**: `RenderGLMultiThreadedTextures`(settings.xml:13356)Value 0→1 + Comment を VK 実態に。AYA-merged + deploy 先 2 箇所へ同期。
12. **J.3 是正**: :1140 = commit 前に `transitionImageLayoutVk`(**level_count 引数を追加**(既定 1・既存 3 caller 不変)で全 mip)を enqueue / :1706 = want_mips 計算(:1655-1663)を撤去し mips=1 固定(FB copy は mip0 しか書かない・実 caller = dynamic texture(usemipmaps=false)+ terrain paintmap = 挙動不変)。
13. **正のオラクル**: `gVkPerf.tex_enq`(scheduleCreateTexture の mainq 枝)/ `tex_pub`(publishStaged 成功時)increment(欄は既存 :6168-6169・extern gVkPerf = llvkloader.h:1653)。
14. **J.4 計器(最小形)**: `beginOneShotCommandBufferVk` で validation 時に CB へ debug name("oneshot")を付与 → IIL 再発時に validation message の Object 0 が frame か one-shot かを自己記述させる(既存 vkSetDebugUtilsObjectNameEXT 使用・validation off でコストゼロ)。

### J.8 自己監査(J.7・correctness/gap/hidden)
- [c-1] **二重作成封鎖**: 判定是正(J.7-2)で staged 中の再入 create は can_reuse=true(mCur 一致)。同 create 内の寸法変更(2 回 commit)は 2 回目 staging が未公開 prev を即時 deferred 破棄 = leak なし。窓間の再 create は mNeedsCreateTexture single-flight(:1727 set / :1702・:1709 clear・main)で構造的に不成立。✓
- [c-2] **未公開 backing の destroy 安全**: 全 destroy は deferred(destroyImageVk :9916 = enqueue_frame + reapReady gate)。upload one-shot は PE FIFO で frame より先に timeline を進める → reap 時点で完了済。✓
- [c-3] **worker と main の mCur 競合**: worker create 中に main が mCur を読む経路 = setSubImage(:1515)/scaleDown(:2529)等。fetched texture は mNeedsCreateTexture 中 draw/更新経路がこれらを呼ばない(従来 GL worker 時代と同一露出・staged で新設した競合ではない)= 残余 risk として申告。draw 消費者は atomic mView/mSlot のみ = 安全。✓
- [c-4] **tick の互換**: main では threadCmdPool()=sCommandPool = bucket[main] のみ free = 現挙動と同一。sReapForceAll(teardown)も自 bucket 限定 = 破棄済 worker pool へ free しない。✓
- [c-5] **backpressure(:8981-9022)の cross-pool**: worker が main pool entry を retire bucket へ push = push は mutex 下・free は所有スレッド = 不変条件維持。✓
- [g-1] register 失敗(pool 作成不能 ≈ device-lost 域)後の投函済み work は共有 pool を無同期で触る窓が理論上残る = 申告(確率 ≈0・fail-closed に倒すには次善)。
- [g-2] LLImageGLThread が「LLApp」listener(threadpool.cpp:94-105)で早期 close される場合も run() 末尾 unregister は join 前に完走 = cleanupClass の明示呼びはその後の冪等 close。✓
- [h-1] mWindow メンバは VK 化で未使用になるが ABI/初期化順に触らないため残置(削除は別掃除)。
- 実装前 open = なし。

### J.9 判定走行①の裁定と是正(2026-08-10 07:19-07:27 走行・validation+VKC)
**裁定(log 全文採掘)**:
- 層0 = PASS 相当(clean exit・新規 crash なし・worker unregister drain timeout 0)。
- worker 実走 = 正のオラクル成立(tex enq 累計 7,234 / pub 累計 2,426・fail 0)。
- **IIL n=4(VVL-TOTAL)— J.4 計器が全件の Object 0 を `oneshot-worker` と自己記述** = 発生源は draw でも frame CB でも heap でもなく **upload→mipchain の 2 one-shot 対**(mipchain 先頭 barrier の mip0 READ_ONLY 期待 :10405-10411 が UNDEFINED を観測)。∴ **前任 3 手(draw/読み手/heap gate)が原理的に当たらない問題だったと機械的に確定**。
- 反証 2 件: ①「submit silent 失敗」説 = `PresentEngine submit failed`(:1139 既存 log)が本走行 0 件で棄却 ②非 deferred 直接 destroy(FIFO 破り)= texture 経路に不在(swapchain/teardown/fallback のみ)。**A(upload)→B(mipchain)は同一 worker 逐次 + PE FIFO なのに B が A の効果を見ない機構は静的に未解決のまま**(候補 = validation layer の handle 追跡 semantics)。
- **新欠陥 = rez-in burst で frame 最大 20 秒**: worker が one-shot を無制限投入 → PE FIFO で frame submit が数千 upload の後ろに並ぶ(旧 2-5ms 予算が担っていた流量制御の欠落)。burst 後は fps 1.2-1.7(validation 課金込み)で安定・fb_heap_default は 476k→199k/10s へ収束傾向。
- 残 alarm: `05137` n=8192(ACCEPTED・register A・母数が worker 化で増えただけ)/ `WARNING-Shader-OutputNotConsumed` n=5304(**allowlist 未登録**・[VK-WARN][PERF]・shader interface 由来 = 本工事の diff は shader 非接触・pre-existing の可能性大だが旧 log 消失で未確証 → **AYA 裁定要**)。

**是正(判定走行②へ)**:
1. **worker 流量制御**: `submitOneShotVkFromPool` 冒頭で worker のみ `peQueueDepth() > 32` の間 wait(200µs スピン・上限 5s)= frame submit の飢餓を構造的に防止。
2. **fold = upload+mipchain の 1 one-shot 化**(`uploadImageDataMipChainVk` 新設・autogen 経路 `syncVulkanMip0Image(..., gen_mips=true)` で使用): barrier 連鎖が UNDEFINED 起点で CB 内自己完結 → **観測された IIL 署名は構造的に不可能**。加えて blit 不能 format では mip1+ を UNDEFINED のまま放置していた既存の INV-1 穴を「全 mip READ_ONLY 化」で閉塞。submit 数半減。データ正しさは隠蔽しない(submit 失敗 log 稼働・tex fail 計器あり)。
3. 隠蔽でない根拠: fold は「2 submit に分割する必要が元々ない処理の正規化」であり、IIL が別部位(例 = main の downscale→mipchain 対)に実在するなら J.4 計器が引き続き名指しする。
- 申告: downscale 内の同型 2 段(one-shot→mipchain)は main 経路で今回未発火 = 触らない(発火したら同型 fold)。

### J.10 判定走行②③の裁定(2026-08-10・是正後)
- **走行②(validation+VKC)**: **IIL = 0(VVL-TOTAL に不存在・①は 4)** = fold で消滅。clean exit・drain timeout 0・submit fail 0・worker enq 6,691/pub 1,951/fail 0。残 alarm = register A(01212 n=2・00186 n=1)+ OutputNotConsumed(→ TICKET `ticket_shader_output_not_consumed`・allowlist 非登録 = AYA 保留)。burst 失速は >2s 窓 10・最大 15.3s で残存。
- **走行③(素・PERF_LOG のみ・約 3.5 分)**: **frame 最大 42.4ms・>500ms 窓 0 = 秒級 stall 不存在** → ②の失速は validation の submit 時課金と確定(素では無害・無処置)。**fb_heap_default 19,391→1,765/10s に急収束**(束縛 bind 2.9M/10s 比 0.06% = load-lag 自然残余)= **供給飢餓の根治を実証**。fps = world 突入 24 → 10-20 秒で 58-61 安定(旧 baseline ≈23.7fps・白落ち 50 万/10s 張り付きから脱却)。worker enq 11,986 ≈ pub 11,983・fail 0。未知 alarm なし(全て ACCEPTED 済クラス)。
- 残 = AYA 視覚 gate(白→実表示の体感・汚染なし)+ commit 裁定。PASS 発行は AYA のみ(憲法 1)。

## 申告欄(縮小・省略・解釈)
- **media worker(`sEnabledMedia`)対象外** = false 維持。VK media upload は別課題。
- **texture fetch(DL)の口が main**(`updateFetch:2162`)は対象外 = 本改修は create/upload の off-main 化のみ。
- **frame 記録経路の per-thread 化は対象外**(§D・load-bearing・現状直列)。
- **worker 数 = 1**(`LL::ThreadPool("LLImageGL",1):2591` 踏襲)。
- **死骸掃除(TEXPOKE 等 A.5-4)は別案件** = 本改修に含めない(混ぜない)。
- **既定 ON/OFF は未決** = `RenderGLMultiThreadedTextures` 既定 0(現状 opt-in)。本改修目的は off-main 化ゆえ既定 1 を推奨するが product 決裁事項(§報告で相談)。
- **予算対症 2 案(VRAM リニア・if 版 worker)= 却下済**・不含([[handoff_texture_dl_fps_coupling]])。

## 変更 file 一覧(実装時)
- `indra/llrender/llvkloader.cpp`: `t_cmdPool`/`threadCmdPool`/`sRetiredByPool`/`registerThreadCmdPool`/`unregisterThreadCmdPool`/`mtMasterThreaded`・`beginOneShotCommandBufferVk`/`submitOneShotVk`/`tickOneShotFreeQueue` 改修。
- `indra/llrender/llvkloader.h`: `registerThreadCmdPool`/`unregisterThreadCmdPool` 宣言(`mtMasterThreaded`/`threadCmdPool` は内部)。
- `indra/llrender/llimagegl.cpp`: initClass(`:364` switch 一本化)・ctor(`:2589` GL bracket 撤去)・run(`:2612` pool register/unregister)+ **PART J: commitVkBacking の staged seam + :1140/:1706 是正**。
- `indra/llrender/llvktexresidency.h`: staged backing 保持(PART J・詳細は Brief)。
- `indra/newview/llviewertexture.cpp`: `postCreateTexture` に staged→publish 配線(PART J)。
- `indra/newview/app_settings/settings.xml`(+AYA-merged): `RenderGLMultiThreadedTextures` comment を VK 実態に(既定値変更は product 決裁後)。
- `indra/newview/llappviewer.cpp`: `shutdownVulkan` 直前に `LLImageGL::cleanupClass()` 配線。

## gate(憲法 2・PASS は AYA のみ)
- 層0 落ちない(起動/TP/teardown)。層1 validation 0 + 検出器沈黙 + `fb_heap_default`(白 fallback)有意減。層2 視覚(AYA・crowd で FPS が texture DL に結合しない = 北極星)。
