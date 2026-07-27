# VK-native 描画構造欠陥 全体地図

> 目的 = もぐらたたきの停止。現状の描画実装を **HEAD ソースの file:line トレースした事実**として包括地図化し、
> 構造欠陥(直列既存欠陥 + 並列化ハザード)を **全件**列挙する。ここは**診断であって fix ではない**。
> 欠陥の処分・新設計(段階 II 以降)は地図完成後に AYA と着手する。
>
> 対象範囲 = **1 frame の描画 call path に限定**(TP/login/asset ingress 等 frame 外経路は対象外・必要なら別地図)。
> 粒度 = 共有可変状態は **phase / スレッド境界を跨ぐもののみ**台帳化(関数ローカルは除外)。

---

## §0 トレース状態ヘッダ（再開アンカー・毎 hop 更新）

> **新窓の再開手順**: この §0 を読む → `pending[0]` の phase の `last` file:line を開く → §4 のトラバースを続ける。
> §0 に載っていない理解は「無い」ものとして扱う（context の記憶を信用しない）。

- **last（深掘り 2 本完了）**: `lldrawpoolalpha.cpp:764`（alpha per-draw shader 切替）読了。**§7 read-set spec + §8 pool 別 S6 判定 完了**。
  - §7 = record read-set 全列挙 → snapshot spec 6 項（未隔離 global は S14/S15/S6 のみ・他は gGL 含め全 thread_local）。S8-b 精密化（untextured=isMapped skip 緩和 / textured=corruption）。
  - §8 = Phase B 拡張の安全境界: **materials/simple/fullbright = 1 pass 1 shader ✓（安全に拡張可）/ avatar = mode 切替 ✗（設計要）/ alpha = per-draw shader 切替 + depth sort ✗✗（crowd の真の blocker・on-demand thread-safe seed + 順序保存分割が要る）**。
  - 全 phase（S1-S18）+ 深掘り 3 本 = 完了。
  - **深掘り 3 = per-program UBO ring の cross-frame 検証**（`vkResolvePerProgramForDraw:3444`/`allocPerDrawUBOSlice:7592`）: 推論を確認・強化。**arena per-draw slice（frame 別・atomic・cross-thread 安全）= A6 資産**。worker 対象 program は全 arena 経路で安全。**S6 は seed 網羅 1 点に純化**（UBO buffering は非問題と確定）。非 rotated 単一 buffer の残懸念は serial-only の少数 post program（C_PP_FALLBACK_LOSSY alarm ガード付き）。
  - 残 = 継続小 hop（GLTF PBR pool 未精査 / avatar mode program union 具体化 = 段階 II 設計時）。
- **covered phases**: 骨格（ph3〜13）/ async 層（S1-5）/ worker 記録 T-lane（S2/S6/S7）/ ph6 shadow（S6-9）/ ph4 geom（S10-12）/ ph5・ph9（S13・S8-b）/ ph10 gbuffer（S14）/ ph11 lighting（S5/S9/S15）/ **ph8 texture（S16/S17）**
- **pending phases**: **なし（全 phase 走破完了）**。全 record 経路・全 worker seam（SEAM-A〜E）・全越境共有 state（S1〜S18）を確定。結論 = **§6 Synthesis**（資産 6.1 / 欠陥全数表 6.2 / 根本原因 6.3 / 再設計要件 6.4）。段階 II はここから。
- **open walls**:
  - W1: **解決済み**（`destroySyncObjects :4293` = main teardown・PE join 後）。S3 enforced 確定。
  - S17 の image handle 公開可視性（worker↔main）= avatar/texture phase 精査時の小 hop（draw record 主眼では順序 enforced で足る）。
  - S8-b の「同一 group が cascade j と j+1 の両方に可視 + MESH_DIRTY」の実発生頻度 = 実測領域（設計上は起こり得る=修正対象・頻度は gate 用）。
  - （真の壁＝ソースから決定不能な点は無し。地図は全て機械的事実に解けた）
- **最終更新**: セッション 1 — 全 phase 走破（S1〜S18・下記 §6 synthesis 参照）

---

## §1 フェーズ骨格（薄い・背景地図）

> frame 1 枚の phase 列 × 現スレッド。DFS で確定した順に埋める。ここは目次であって欠陥台帳ではない。

> frame 駆動点 = `display(bool rebuild, F32 zoom_factor, int subfield, bool for_snapshot)` @ `llviewerdisplay.cpp:552`。
> caller = `LLAppViewer::doFrame`（main thread）。phase 番号 N は**ソース側の `LLVKLoader::VkPerfPhaseScope ph(N)`** に一致（既存計器）。
> **全 phase を run するかは frame 冒頭の `aya_async_engaged` / `asyncShouldRenderScene()` が gate**（async 分離時は producer 窓のみ scene を描く）。

| ph | phase | 現スレッド | frame entry からの到達点 file:line | 一言 |
|----|-------|-----------|-----------------------------------|------|
| — | frame gate / async 取込 | main | `llviewerdisplay.cpp:564-574` | producer 完了を `mScenePresentFront` に取込・`aya_async_engaged` 判定 |
| — | 早期 return 群 | main | `:592-691` | 非 active/minimized・startup<STATE_STARTED・disconnected・headless で scene skip |
| — | camera / env / HUD update | main | `:823-923` | camera 設定・`LLEnvironment::update`・`LLHUDManager::updateEffects` |
| — | async producer begin | main | `:864-867` | `asyncProducerBeginScene(1 - mScenePresentFront)`（back index に記録開始） |
| 3 | hero probes（mirror） | main | `:889-896` | `mHeroProbeManager.update()/renderProbes()`（RenderMirrors 時のみ） |
| 4 | Update Geom | main | `:925-950` | drainGeoPublishQueue / drainAvatarPublished / createObjects / processPartitionQ / updateGeom |
| — | updateGL | main | `:952` | `gPipeline.updateGL()` |
| 5 | updateCull | main | `:973-976` | `gPipeline.updateCull(camera, result)` → 可視集合 |
| 6 | generateSunShadow | main（+worker） | `:989-994` | shadow cascade 記録（Phase A で worker 並列化済）。gFrameCount>1 gate |
| 7 | updateImpostors | main | `:1003-1006` | `LLVOAvatar::updateImpostors()` |
| 8 | Update Images | main | `:1028-1057` | updateClass / bump / texture list / GLTF material cleanup |
| 9 | stateSort | main | `:1069-1084` | `gPipeline.stateSort(camera, result)` → draw order 生成（+rebuild 時 rebuildPools） |
| — | sky update | main | `:1091-1095` | `gSky.updateSky()` |
| — | deferredScreen bind/clear | main | `:1130-1137` | `beginCameraRecordSplit` / deferredScreen.bindTarget/clear |
| — | occlusion pre-pass | main | `:1147-1167` | RenderDepthPrePass 時 occlusion program で depth 事前描画 |
| 10 | renderGeomDeferred | main | `:1170-1171` | gbuffer 記録（`renderGeomDeferred(camera, true)`） |
| — | rt.flush | main | `:1190-1191` | deferredScreen or screen を flush |
| 11 | renderDeferredLighting | main | `:1193-1197` | deferred lighting 合成 |
| 12 | render_ui | main | `:1210-1215` | `render_ui()` + drainPendingProfileAvatars/AttachmentProfiles |
| 13 | swap（present） | main | `:1216-1217` | `swap()` |
| — | frame 終い | main | `:1221-1233` | `sNoDelete=false` / `clearReferences()` / display_stats |

---

## §2 不変条件台帳（本体・load-bearing）

> 各共有可変状態・各順序エッジを 1 行。`class` の 3 値化が全件列挙のエンジン。
> **class = accidental / none の行が「たまたま描けてるだけ」と顕在バグの全数表**（直列で既に欠陥）。
>
> 行スキーマ:
> ```
> [行ID] 状態名（buffer / descriptor / RT / fence / pin / epoch / name …）
>   touch: file:line (R|W, thread, phase)      ← 全アクセス地点を列挙（複数行可）
>   required-invariant: 正しい描画に真であるべき事（一文）
>   provider: 今それを保証している実体 file:line、または「なし」
>   class: enforced | accidental（偶然の直列順） | none
>   gap: required − provider。class≠enforced のとき何がどう壊れるか
>   seams: この状態を跨ぐ seam ID[]（§3）
> ```

<!-- 行はここに追記していく。行ID は S1, S2, … で採番。 -->

### スレッドモデル（async 分離時・firm）
- **記録**（draw cmd → CB）= 呼び出しスレッドの **thread_local `tRecordCmdOverride`**（`llvkloader.cpp:208`）が指す CB へ。main が producer CB / consumer CB / NULL を切替。
- **`endFrame`（main thread）** が CB を end し、`peEnqueue` で **PE worker thread** に PEJob 投入（`llvkloader.cpp:6203-6271`）。
- **PE worker thread** が実 `vkQueueSubmit` + fence signal を行う（peExecute submit ループ `:945-1024`、submit は `:1012`）。
- main は `asyncProducerTryComplete()`（`:5339`）で `vkGetFenceStatus` により producer fence を**非ブロック poll**。

---

**[S1] async scene present index `mScenePresentFront` / producer back index（front/back ダブルバッファ）**
- touch: W/R `llviewerdisplay.cpp:571-574`（tryComplete==true で `mScenePresentFront = asyncProducerBackIndex()`）— main, frame gate
- touch: R→W `llviewerdisplay.cpp:864-866`（in-flight でなければ `asyncProducerBeginScene(1 - mScenePresentFront)`）— main, begin phase
- touch: W `llvkloader.cpp:5396`（beginScene が `sAsyncProducerBackIndex = back_index`）, R `:5367`（asyncProducerBackIndex）— main
- required-invariant: present が読む front の scene 内容は、producer が記録し終え **fence 完了済み**の back。記録中の index を present が読まない（front≠back が in-flight 中保たれる）
- provider: `asyncProducerTryComplete()` の `vkGetFenceStatus==SUCCESS` gate（`:5346`）が swap を fence 完了に律速 + begin は常に `1-front` を back に選ぶ（`:866`）→ record と present が逆 index で排他
- 実体: index 先 = `LLRenderTarget mScenePresentRT[2]`（`pipeline.h:888`・実 2 枚 alloc `pipeline.cpp:1177-1178`）。back = `mScenePresentRedirect`（`llviewerdisplay.cpp:1729-1732`）へ `renderFinalize()` が合成 W（`:1737`）／ front = `blitScenePresentToSwapchain()` が R して present（`pipeline.cpp:10914`／呼び `llviewerdisplay.cpp:1825`）。
- 確定: **back = `1 - front` が構造的に保証**（beginScene が `1-front` を back に設定 `:866`／render_ui `back_idx` も async は `asyncProducerBackIndex()`==`1-front` `:1727`）→ 記録先 index と present 先 index は frame 内で**常に逆・不変**。
- class: **enforced**（color 二重化 = fence poll が happens-before・index が構造的に逆で排他）
- gap: color は無し。**ただし下記 S5 = depth 共有が別ハザード**（color 二重・depth 単一）。
- seams: SEAM-A（producer/consumer 分離・§3）

**[S5] scene present RT の共有 depth buffer（`mScenePresentRT[0].shareDepthBuffer(mScenePresentRT[1])` `pipeline.cpp:1179`）**
- touch: alloc/share `pipeline.cpp:1177-1179`, depth W = back への scene 描画（ph10 gbuffer / ph11 lighting が back の共有 depth を書く・`[PENDING ph10/11 DFS]`）, depth R = 同上 lighting/DoF
- required-invariant: color は front/back で二重化されているが depth は 1 枚。async cadence で producer(back 記録) と consumer(front present) が時間的に重なるとき、**同一 depth image への producer 書きと、前 frame GPU 作業の depth 読みが衝突しない**
- provider: present 側 blit（`blitScenePresentToSwapchain`）は **color copy のみ**（`gCopyProgram`・depth 不読 `:10920-10925`）→ present は depth を触らない。しかし back への **scene 描画（deferred lighting/DoF）は共有 depth を R/W** し、それが前 frame の GPU 完了前に走ると衝突
- **class（ph10/11 で確定）**: present blit は color copy のみ（depth 不読・`gCopyProgram` `:10920-10925`）→ present 側は depth 競合なし。**しかし scene 描画（gbuffer/lighting）は deferredScreen の depth を R/W し、その depth は screen/objectID/velocity/alpha/forward と広く共有（`:1149-1263`）**。async cadence で producer(back の scene 描画) と consumer(front の present) が時間的に重なる場合でも、present は depth を読まないので**衝突しない**。producer 自身の gbuffer→lighting は同一 frame CB 内で barrier 済み。∴ **enforced**（present と scene 描画の depth 競合は構造的に無い）
- gap: なし。ただし **mScenePresentRT[0/1] の共有 depth（S5 本来の対象）は color 二重・depth 単一という非対称**が残る。現状 present が depth 不読ゆえ無害だが、**将来 present 経路が depth を要する処理（例: depth-aware composite）を足すと即衝突** → 再設計時に「color 二重なら depth も二重」の対称性を検討
- seams: SEAM-A

**[S2] `tRecordCmdOverride`（記録先 CB ルーティング・thread_local `llvkloader.cpp:208`）**
- touch: W main `:5404`（beginScene → producer CB）, W main `:5422`（recordToConsumer → consumer CB）, W main `:5427`/`:6205`（NULL 化）, R = 全 draw 記録 site（cmd を積む先の解決）
- required-invariant: あるスレッドの全 draw 記録が、(a) begun 状態の CB へ入り (b) その CB が当該 frame に正しい fence/semaphore で submit される。**特に、ある phase の draw が意図した CB（scene=producer / UI=consumer）へ入る**
- provider: cross-thread 汚染は **thread_local** が防ぐ（別スレッドの override は独立）。**しかし同一スレッド内では「override を記録前に正しくセットし記録後にクリア」という直列順のみが保証**
- class: cross-thread 面 = **enforced**（thread_local）／ **同一スレッド内 phase→CB 対応 = accidental（偶然の直列順）**
- gap: 同一スレッド内で override を記録前にセットし記録後クリアする順序が破れると draw が別 CB へ。ただし現状の worker 経路（`rwExecute`）は**正しくセットしている**（下記追記）。
- **追記（ph6 DFS で narrowing）**: worker 記録経路 `rwExecute`（`llvkloader.cpp:1395-1419`）は `tRecordCmdOverride = cmd`（`:1403`）を **job.body 実行前に必ずセット**し後で NULL 復元。→ **「worker override 未設定でボディ消失」の素朴仮説は反証**。GLSL bind 状態（`sCurBoundShaderPtr` `llglslshader.h:202` 他 per-call descriptor / pipeline memo）も**全て thread_local**（`:387-406`・reset は `resetPerThreadRecordState` `:3556`）= worker 毎に隔離済み。∴ 記録経路の routing/bind/descriptor は enforced。**ボディ消失の真因は override でなく S6（共有 per-program state）**。
- seams: SEAM-A, SEAM-B（shadow worker）, SEAM-C（gbuffer worker）

### 確定事項: 描画「現在状態」globals は網羅 thread_local 化済み（worker 記録を可能にした大改修）
以下は全て `thread_local` = worker 毎に独立・cross-thread state race **なし**（save→set→restore が per-thread で閉じる）:
`gGLModelView[16]`（`llrender.h:478`）/ `LLRenderTarget::sBoundTarget`・`sCurResX`・`sCurResY`（`llrendertarget.h:63-226`）/ `LLRenderPass::sShadowBatchCullRadius`（`lldrawpool.h:406`）/ `LLGLSLShader::sCurBoundShaderPtr` 他 per-call/seed/pipe memo（`llglslshader.h:202-406`）/ `gVkPerfPassTag`・`gVkPerfShadowMapIndex`（`llvkloader.h:1603`）/ **`LLPipelineFrameContext` singleton 自体が `static thread_local`（`llpipelineframecontext.cpp:28`）** = mCullResult/mShadowPass/mActiveRT も worker 毎独立。
→ **worker 記録の state-race クラスは構造的に解消**。以降の worker 化で新設する「frame 現在状態」も **thread_local か ctx 渡し**にすれば race は生じない。**これは再設計の確立した資産**。

**[S6] record job の seed 網羅ギャップ = unseeded program のサイレント draw 消失（★Phase B ボディ消失の確定機構）**
- touch: seed 事前ビルド（main）= `ensureShadowWorkerSeeds`（`pipeline.cpp:14404`）が **固定リスト `worker_shaders[]`（`:14415-14421` = shadow 系 5 program のみ）**を bind→`vkResolvePerCallSetForDraw`→`seeds.map[s]=seed`（`:14462`）
- touch: record 中参照（worker）= `populateAndBindUniversalDescriptorSet`（`llglslshader.cpp:3616-3648`）が `sRecordSeedMap->find(cur)`（`:3621`）。**hit すれば seed.set を使う（`:3644`）**
- touch: **miss 経路（`:3649`）= `C_RECORD_JOB_PULL` → descriptor 未 bind で return**。警告は `s_record_populate_hits` の **2 冪 throttle のみ（`:3652`）= 実質サイレント**
- required-invariant: worker が record 中に bind する **全 program が、事前 seed リストに含まれる**（seed map 網羅 = 実際に描かれる program 集合 ⊇ の逆包含）
- provider: **なし（リストが手書き固定）** — `worker_shaders[]` は shadow 用に手で 5 個列挙。実際に pass が bind する program 集合との一致を保証する機構は無い
- class: **accidental（偶然の一致）** — shadow（Phase A）は pass が shadow program しか bind せず偶然リストと一致 → 動く。**Phase B gbuffer は material/PBR/skin 系の多数 program を bind し、リストは未拡張 → avatar body の skin/material program が seed-lost → descriptor 無し → draw 消失 = ボディがすっ飛んで消えた**
- gap: **これが「たまたま描画されてる」の教科書的実例**。validation error も crash も出ない（サイレント）ため計測で気づけない = 憲法「計測で設計欠陥を直すな」の核心。**再設計では seed を「実際に bind される全 program から機械導出」or record 中の on-demand seed 生成（thread-safe）にする。手書きリストは禁止**
- **共有 per-program フィールドの実体（ph11 で確定）**: 「1 program 1 job」が守る具体物 = **`LLGLSLShader::mVkPerProgramUBO` / `mVkPerProgramUBOMapped`（program 毎の UBO・`llglslshader.h:417-419`）**。main の多数 site が memcpy で書く（lighting/sky/terrain/pathfinding `pipeline.cpp:5594` 他多数・全て serial main）。
- **★per-program UBO ring の cross-frame 検証（深掘り 3・2026-07-28・`vkResolvePerProgramForDraw` `llglslshader.cpp:3444` / `allocPerDrawUBOSlice` `llvkloader.cpp:7592`）= 私の推論「ring が cross-frame を捌く」は確認かつ強化**。UBO 解決は 3 経路:
  1. **arena per-draw slice 経路（bindless dynamic・`mVkSet1DynamicCount>0`）** = draw 毎に `allocPerDrawUBOSlice` で新 slice を確保（`:3461`）。allocator は **`sPerDrawUBOArena[sFrameIndex]`（frame 別）+ atomic CAS bump（`a.cursor` `:7618`）+ growth mutex** = **cross-frame・cross-thread・cross-draw すべて安全**。**worker 対象の material/scene 系（bindless）はこの経路** → UBO 競合なし。
  2. **rotated ring 経路（6 pool: alpha/avatar/sky/bump/hero/gltf）** = `mVkPerProgramRing[frameIndex<3]` の frame 別バケット + per-draw rotate（`rotatePerProgramUBOSlot` `:3169`）= cross-frame 安全。
  3. **非 rotated 単一 buffer 経路（`mVkSet1DynamicCount==0` の固定 post/lighting program）** = base 単一 buffer を毎 frame 上書き = **frame 別バケット無し = cross-frame overlap の潜在**。ただし対象は sun/luminance 等の値がほぼ frame 安定な少数 program で **serial main のみ**（worker 非対象）+ arena 枯渇時は `C_PP_FALLBACK_LOSSY`（alarm）が発火。
- **∴ per-program UBO の cross-frame 問題は arena（per-draw slice・atomic・frame 別）+ ring で堅牢に解決済み = 再設計の資産**。**worker 対象 program は全て arena 経路で安全** → **S6 は「UBO buffering」ではなく純粋に「seed 網羅（どの program が seed 捕獲されるか）」に帰着**（unseeded なら descriptor 自体が lost → 消失）。深掘りで S6 の残懸念が seed 網羅 1 点に純化した。
- **camera 版精密化（ph10 で確定）**: gbuffer 側 `ensureCameraWorkerSeeds`（`pipeline.cpp` mt_materials 内 `:6161`）は shadow の**手書きリストより良く**、pool の各 deferred pass を `beginDeferredPass(i)` で bind し `sCurBoundShaderPtr` を機械捕獲（1 pass = 1 seed）。**正しさの不変条件 = 「`renderDeferred(pass)` は `beginDeferredPass(pass)` が bind した shader**以外**を bind しない」**。POOL_MATERIALS はこれを満たす（1 pass = 1 material shader 変種）ため worker 化で動く。**この不変を破る pool（1 pass 内で rigged/非 rigged 変種や per-draw shader 切替を行う pool = avatar/alpha 系）へ worker 化を広げると、beginDeferredPass 時と異なる shader が draw 時に bind され seed-lost → その draw 消失 = ボディがすっ飛ぶ**。∴ **Phase B を materials 以外へ拡張する前に、対象 pool が「1 pass 1 shader」不変を満たすか（or seed をその pass の全 bind shader へ拡張するか）を pool 毎に検証必須**
- seams: SEAM-B, SEAM-C（両 seam の生命線 = seed 網羅・§3 反映済み）

**[S7] worker thread_local frame 状態の ctx 全項目転写契約（暗黙・破ると別種のサイレント誤描画）**
- touch: worker body `recordShadowWorkerFamilies`（`pipeline.cpp:14474-14500`）が **自スレッドの thread_local frame context/globals を `ctx`（値渡し `ShadowRecordCtx`）から明示 set**: cullResult/shadowPass/boundTarget/resX/Y/cullRadius/seeds/gGLModelView(memcpy)/gGLLastModelView/passTag/mapIndex
- required-invariant: worker の thread_local 現在状態は main と別インスタンスゆえ**既定値**。record が読む **全ての frame 現在状態を ctx 経由で漏れなく転写**しないと、worker は stale/既定値で描く
- provider: `ShadowRecordCtx` 構造体（`:14343-14360`）に必要項目を手で列挙 + `recordShadowWorkerFamilies` 冒頭で手で set。**転写漏れを検出する機構は無い**
- class: **accidental（手動転写の網羅性頼み）**
- gap: ctx に載せ忘れた frame 現在状態が 1 個でもあると、worker はその項目を既定値で描き**位置ズレ/消失/誤マテリアル**（サイレント）。Phase B で ctx に相当する gbuffer 用状態（例: 各 pool の material bind 前提・lighting uniform）を網羅転写できるかが生死。**再設計では「worker が読む frame 状態」を型で括り、転写を機械網羅（構造体丸ごと snapshot）にする**
- seams: SEAM-B, SEAM-C

**[S8] shadow worker の pin 生存期間 + cascade 並走中の LLDrawInfo 変異**
- touch: pin W main = `pinShadowWorkerDrawInfos`（`pipeline.cpp:15790`／実体 `:14364-14401`）が `result[j]` の全 LLDrawInfo を `sShadowRecordPins`（`LLPointer` 保持 `:14362`）へ + skin palette prewarm（`updateSkinInfoMatrixPalette` `:14369`・frame-stamp）
- touch: dispatch（no-join で continue）= `:15810`。worker が LLDrawInfo を **非同期 R**。main は同 loop で **次 cascade の `updateCull`+`stateSort`（`:15779-15780`）を並走 W**
- touch: join barrier = `joinRecordJobs`（`:16063`）→ `sShadowRecordPins.clear()`（`:16064`）= ph6 末尾
- required-invariant: worker が読む LLDrawInfo は、dispatch から join まで **(a) 生存 (b) 内容不変**。かつ prewarm した skin palette は worker read-only
- provider（生存 (a)）= `LLPointer` pin が ref 保持 → UAF なし = **enforced**。provider（不変 (b)）= **なし** — main は worker 実行中に cascade j+1 の `stateSort`/`updateCull` を走らせ、同一 group/VB を触る
- class: 生存 = **enforced** ／ 内容不変 = **accidental（確定した race・稀発現）**
- **S8-b 確定（ph9 resolver 完了）**: cascade j の worker が group G の face VB を **非同期 R**（recordShadowWorkerFamilies→renderShadow が draw map を辿り VB 参照）。並走する main の cascade j+1 `stateSort`（`pipeline.cpp:15780`）→ visible-groups ループ `group->rebuildMesh()`（`:4355`）→ `LLVolumeGeometryManager::rebuildMesh`（`:8218`）が **`MESH_DIRTY` の group の face VB を同期 W**（`:8231-` locked_buffer 経路）。**`mVkGeoInflight` guard（`:8222`）は geo publish 用で shadow worker の in-flight を知らない = 無効**。→ **G が MESH_DIRTY かつ cascade j と j+1 の両方に可視なとき read-during-write が成立**
- gap: **これが「たまたま描けてる」の第 2 実例**（S6 に次ぐ）。発現は稀（rebuildMesh は MESH_DIRTY 時のみ非 no-op + 複数 cascade 跨ぎ可視の同時性が要る）ゆえ計測で捕まらず、**memory `project_deferred_ledger` の稀症状（体消失/関節崩れ/緑汚染）と符合**。pin は LLDrawInfo を生かすが **その下の VB を main の rebuildMesh から凍結しない**。**再設計では worker 窓の間 main が共有 geometry を変異させない（rebuildMesh を worker 窓外へ・or group 単位の in-flight ロックを shadow worker にも効かせる）ことが必須**
- **★重要な非対称（§7 read-set 深掘りで確定）**: record の draw push には **isMapped ガードの有無で 2 経路**ある。**untextured 経路 `pushUntexturedBatch`（`lldrawpool.cpp:1835`）は record job 中 `mVertexBuffer->isMapped()` なら skip** = shadow（untextured）は S8-b が **corruption でなく transient な batch 欠け（次 frame で回復）** に緩和されている（＝Phase A が大崩れしない理由の 1 つ）。**しかし textured 経路 `pushBatch`（`:1808`）は null チェックのみで isMapped ガード無し** → **Phase B gbuffer（textured）は S8-b が transient gap でなく VB corruption になる** = 拡張時により危険。※isMapped フラグ read 自体も非 atomic の可能性（緩和であって根治でない）。**再設計は「skip で誤魔化す」でなく「窓中 VB 不変」で根治すべき**
- seams: SEAM-B

**[S13] cull/stateSort が生成する per-frame 可視 draw 状態（world path=直列 enforced / shadow path=S8-b）**
- touch: `updateCull`（ph5 `:975` / shadow `:15779`）→ `LLCullResult`（可視 group/bridge 集合）W。`stateSort`（ph9 `:1073` / shadow `:15780`）→ group 可視性・draw order・rebuildMesh・resetDrawOrders W。全 record phase（ph6/ph10/ph11）が R
- required-invariant: 可視 draw 状態の生成（cull→stateSort）と消費（record）が、生成物を消費中に変異させない
- provider: **world camera path** = ph5→ph9→ph10/11 が全て main 直列 → **enforced（直列順）**。**shadow path** = ph6 内で cull/stateSort（main）と record（worker）が並走 → **S8-b の race**
- class: world = **enforced（直列）** ／ shadow = **accidental（S8-b）**
- gap: world path は直列ゆえ安全だが、それは**「直列だから安全」= 並列化したら即崩れる**典型（S13 が seam 化候補になった瞬間、cull/stateSort の全 W を record の R から隔離する必要が生じる）。**再設計の中心命題** = 可視 draw 状態を record 前に**不変な snapshot** として確定し、record 窓中は main がそれを触らない
- seams: SEAM-B（既存）, SEAM-C（gbuffer 化で顕在化）

**[S9] shadow depth の cross-CB 依存（worker pre-cmd が W → 本 frame CB が shader-read R）**
- touch: worker CB（pre_cmds）が shadow depth へ描画 W（`recordShadowWorkerFamilies` 内・layout を DEPTH_ATTACHMENT へ遷移 `:14519-14524`）。main が `shadow_rt.setVkDepthLayout(DEPTH_STENCIL_ATTACHMENT_OPTIMAL)`（`:15814`）で CPU 側 layout 追跡更新
- touch: 本 frame CB が deferred lighting で shadow map を shader-read R（`shadow_rt.bindForShaderRead` `:15840` / ph11）
- required-invariant: pre_cmds（shadow W）が frame CB（shadow R）より **GPU 実行順で先**、かつ **layout 遷移（ATTACHMENT→SHADER_READ）と WAW/RAW barrier** が挟まる
- provider: 提出順 = pre_cmds → frame CB（同一 queue・`endFrame` `:6228`/`6278` で pre を先頭に）。ただし**別 CB 間の実行順は submit 順のみで、明示 semaphore/barrier が要る**。`cmdShadowDepthWawBarrierVk`（`llvkloader.cpp:5723`）と layout 遷移が該当 → **どの CB にこの barrier が積まれるか未確認**（worker CB 末尾 or 本 CB 冒頭）
- **class（ph11 で確定）**: **enforced**（同一 queue の submit 順 pre_cmds→frame CB + main frame CB の layout 遷移 barrier）。shadow depth W = worker pre-cmd CB、layout を DEPTH_ATTACHMENT で残す（`:15814`）→ main が ph6 末 `shadow_rt.bindForShaderRead(0, true)`（`:15840`）で ATTACHMENT→SHADER_READ 遷移を **frame CB に記録**。Vulkan の pipeline barrier の first scope は同一 queue の submgit 順で先行する全 command（前 CB 含む）を包含 → pre-cmd の depth 書きを frame CB の barrier が同期。ph11 の sun_shader draw（`:12700`）が shadow map を sample する時点で可視
- gap: なし（cross-CB でも submit 順 + barrier stage が depth-write→shader-read を張れば安全）。**ただし barrier の stage mask が LATE_FRAGMENT_TESTS→FRAGMENT_SHADER を正しく含むことが前提**（bindForShaderRead の実装依存・逸脱すると影ちらつき）。**再設計で worker CB と frame CB の境界 barrier は明示的な同期契約として型化すべき**
- seams: SEAM-B

---

## ph4 Update Geom（geometry 供給層・producer/consumer = T2 geo worker）

**[S10] geometry publish 越境（T2 `geoWorkerMain`「aya-geoup」→ main・cross-frame）**
- touch: producer（geo worker）= `geoWorkerMain`（`llvovolume.cpp:6092`）が `sGeoJobQueue`（mutex `sGeoJobMutex`）から pull → `runVkGeoFill` で `job->mStaged` 構築 W → `job->mState` atomic 更新 → `sGeoPublishQueue`（mutex `sGeoPublishMutex`）push（`:6122-6123`）
- touch: enqueue（main・producer 投入元）= rebuildGeom 経路（`:8183-8187`）が `job->mStaged = std::move(staged)` + `job->mGen = mVkGeoGen` snapshot → `sGeoJobQueue` へ。**ph4 updateGeom が markRebuild 済み group に対しこれを呼ぶ = producer dispatch**
- touch: consumer（main）= `drainGeoPublishQueue`（`:6937`・ph4 冒頭 `llviewerdisplay.cpp:940`）が pop → 検証 → `applyGeoStaged`（`:7010`）。**budget `AYASTORM_GEO_APPLY_BUDGET_MS`（既定8ms）で 1 frame の適用量を制限**（未適用は次 frame へ持ち越し）
- required-invariant: worker が build 中の `mStaged` を main が読まない／main が enqueue 後に触らない（所有権の一方向移動）。`mFillFailed`/`mState`/`mStaged` の worker→main 可視性
- provider: queue 越しの所有権移動（push/pop 双方 mutex）= happens-before 成立 → `mStaged`（push 前に構築完了）と `mFillFailed`（非 atomic bool だが mutex 越しで可視）は enforced。`mState` は atomic
- class: **enforced**（mutex による所有権移動 + atomic state）
- gap: なし（越境機構は健全）。**ただし cross-frame 遅延**（enqueue の frame と apply の frame が異なる）ゆえ、その間の group 変異は S11 の gen/検証が担保する必要がある
- seams: SEAM-D（geo worker・§3 追加）

**[S11] geometry 適用の原子性 = gen staleness + validate-all-then-apply +（喪失↔再予約）原子対【VKGeo fix・良設計】**
- touch: gen = `mVkGeoGen`（`llspatialpartition.h:450`・plain U32）。W main（inline apply 時 `++` `:8163/8175`）／snapshot main（`job->mGen=mVkGeoGen` `:8187`）／R main（stale 判定 `:6997/7082`）= **全て main**
- touch: 検証 = `applyGeoStaged`（`:6194`）が適用前に全 face を検証（dead/moved/te 範囲/geom・index count 一致/nullvb `:6201-6242`）、1 つでも不整合なら **false で原子 abort（部分適用ゼロ）**
- touch: 原子対 = drain の後処理（`:7030-7044`）が `!applied || mHadFailedFace` かつ非 stale なら group を `GEOM_DIRTY` 再設定 + `markRebuild`（`:7037-7042`）
- required-invariant: (a) group の draw map を「消して from-scratch 作る」操作は、適用成功と原子的（部分適用で穴を残さない）(b) 適用が失敗/中断したら必ず rebuild が再予約される（record 喪失 ↔ 再予約が原子対）(c) 古い job の適用が新しい状態を壊さない（gen staleness）
- provider: (a) validate-all-then-apply（単一 choke `applyGeoStaged`）(b) drain 後処理の再 dirty+markRebuild (c) `mVkGeoGen` 比較。全て main 単一スレッド上
- class: **enforced**（VKGeo fix `bbe622f3472` の設計・main 内直列で gen は race なし）
- gap: なし。**再設計の資産** = 「越境した geometry を『検証してから原子適用・失敗なら再予約』する単一 choke」は他の worker 化（Phase B gbuffer の draw map 構築等）にも横展開すべき雛形
- seams: SEAM-D

**[S12] avatar geometry の二次 worker 再ルート（`sAvatarWorkerRunning`/`sAvatarJobQueue`）**
- touch: drain 中 `group->mAvatarp != nullptr` かつ `sAvatarWorkerRunning` なら job を `sAvatarJobQueue`（mutex `sAvatarJobMutex`）へ再投入（`:7001-7008`）→ `drainAvatarPublished`（`:7064`・ph4 2 番目 `llviewerdisplay.cpp:942`）が回収
- required-invariant: avatar geom は skin/palette 依存が重く別 worker へ。S10/S11 と同型の越境・原子性が avatar 経路でも成立
- provider: `drainAvatarPublished`（`:7064-7134`）は **drainGeoPublishQueue と完全同型**（gen staleness `:7082` / applyGeoStaged 検証 `:7086` / 原子対再予約 `:7111-7118`）。冒頭 `drainMotionDone()`（`:7066`）で motion/skin worker 完了を回収してから apply
- class: **enforced**（S11 と同一の単一 choke 設計）
- gap: なし。skin/motion worker（skeleton off-main）自体の race は既決（memory: Class1=mComputeMutex / Class2=upstream 無罪）だが **source 再確認は avatar phase 精査時に**（本地図は draw record が主対象ゆえ motion worker 内部は別 hop 扱い）
- seams: SEAM-D

---

## ph10 renderGeomDeferred（gbuffer・Phase B は POOL_MATERIALS のみ実装済み）

> 構造: `renderGeomDeferred`（`pipeline.cpp:6063`）が `mPools` を走査。POOL_MATERIALS かつ `mCameraRecordSplitActive` かつ main camera なら **worker 化**（`mt_materials` `:6154-6232`）: `ensureCameraWorkerSeeds`→`pinCameraWorkerDrawInfos`→pass 毎に `dispatchRecordJob(recordCameraPoolPass)`（`:6196-6205`・**1 job = 1 pass**）。dispatch 失敗は inline fallback（`:6206-6213`）。join は **`finishCameraRecordSplit`**（`:6052-6059` = `joinRecordJobs`+`sCameraRecordPins.clear`）を display `:1174`（renderGeomDeferred の直後）で呼ぶ = camera worker 窓 = dispatch〜finishCameraRecordSplit。**この窓中 main は rt.flush/texunbind のみ（stateSort なし）ゆえ S8-b 型の geometry 変異は起きにくい**（shadow より安全）。

**[S14] `gGLDeltaModelView`/`gGLInverseDeltaModelView`（plain global・非 thread_local・`llrender.h:483-484`）**
- touch: W main = `renderGeomDeferred`（`:6089-6090`・pool loop **前**に 1 回）。R = worker の `recordCameraPoolPass`→`renderDeferred` 内で material shader が motion/velocity 用に読み得る。**`recordCameraPoolPass` は ctx から gGLModelView/gGLLastModelView は memcpy するが gGLDeltaModelView は set しない**（`pipeline.cpp` recordCameraPoolPass・ctx に項目なし）
- required-invariant: worker が読む delta modelview が、main の書いた正しい値で、worker 窓中に書き換わらない
- provider: main は pool loop 前に 1 回書き（`:6089`）窓中は触らない + dispatch の mutex が happens-before を張る → **現状は安全**。ただし **thread_local 化されておらず**（sibling `gGLModelView` は thread_local `:478`・非対称）、ctx にも載らない = **暗黙の共有読み**
- class: **accidental（偶然安全）** — 「main が窓中に触らない」+「全 worker が同一値を読んで良い」に依存。将来 main が窓中に書く／worker 毎に異なる delta が要る改修で即崩れる。thread_local 兄弟との非対称は**罠**
- gap: Phase B 拡張時、per-cascade/per-view で異なる delta modelview が要る worker が出ると、共有 global では 1 値しか持てず誤描画（motion vector 化け・TAA �horst）。**再設計では delta modelview も thread_local か ctx 項目に含める（gGLModelView と対称化）**
- seams: SEAM-C

---

---

## ph11 renderDeferredLighting（deferred lighting 合成・全 main serial）

> `renderDeferredLighting`（`pipeline.cpp:12462`）は **worker 化されていない**（全 main 直列記録）。gbuffer（deferredScreen）を shader-read（`:12722-12727`）し、shadow map を sun_shader で sample（`:12700`）、local lights/projectors/atmospherics を順次合成。S9（shadow depth cross-CB）= **enforced 確定**、S5（present 共有 depth）= **enforced 確定**（上記各行に反映）。

**[S15] `writeCurrentGlobalFUBO`（GlobalF UBO・phase 毎に main が再書き込みする単一共有 UBO）**
- touch: W main = `renderGeomDeferred`（`:6101`・mirror/clipPlane）と `renderDeferredLighting`（`:12662`・sLastMirrorFlag/clipPlane）で **同一 UBO を別値で再書き込み**。R = 各 phase の draw（gbuffer worker 含む）
- required-invariant: ある phase の draw が読む GlobalF 値が、その phase 用に書かれた値で、読む間に次 phase の書きで上書きされない
- provider: phase が時系列で分離（ph10 finishCameraRecordSplit join → ph11）ゆえ gbuffer worker 窓（ph10）と ph11 の書き（`:12662`）は重ならない → **現状安全**。ただし GlobalF UBO 自体は **単一共有・非二重化**（S14 と同クラス）
- class: **accidental（phase の時系列分離頼み）** — worker 窓が phase を跨いで伸びる／phase 並列化で ph10 と ph11 が重なると、共有 GlobalF UBO の書きが worker の読みと競合
- gap: S14（gGLDeltaModelView）と同じ「phase 毎に main が書く単一共有描画 uniform を worker が暗黙に読む」構造。**再設計方針 = worker が読む描画 uniform（GlobalF/DeltaModelView/per-program UBO）は全て per-lane/frame ring 化 or ctx snapshot にし、単一共有 mutable を record 窓から排除**
- seams: SEAM-C

---

---

## ph8 Update Images（T1 texture worker・producer/consumer）

**[S16] 単一 queue submitter（PE thread が `vkQueueSubmit` を独占）【★重要資産・concurrent-submit クラスを構造的に排除】**
- touch: `vkQueueSubmit(sGraphicsQueue, …)` の**唯一の呼び出し = PE thread の peExecute**（`llvkloader.cpp:1012`）。frame job / producer job / record worker の pre_cmds / **texture worker の one-shot upload** が全て `peEnqueue`（`:8879`）で PE へ集約
- 確認: texture worker `texWorkerUpload`（`:9058`）は CB を自スレッドの `sTexWorkerCommandPool` で構築するが **queue へ直接 submit せず** `submitOneShotVkFromPool`→`peEnqueue`（`:8874-8880`）で PE に委譲（fence 枯渇時 fallback も `peSubmitBlocking` 経由 = PE）
- required-invariant: `VkQueue` は外部同期必須 = 同一 queue への vkQueueSubmit/vkQueuePresent を 2 スレッドが同時に呼ばない
- provider: **PE thread 単一 submitter**（他スレッドは全て peEnqueue でジョブ投入のみ・queue API を直接叩かない）
- class: **enforced（設計資産）** — この一本化ゆえ「異スレッド同時 submit = device-lost/UB」クラスは起き得ない
- gap: なし。**再設計でも死守すべき中核不変** = 「queue を触るのは PE thread だけ・他は全て job 投入」。worker 増設時もこの規律を破らない限り queue race は生じない
- seams: SEAM-A, SEAM-B, SEAM-C, SEAM-D, SEAM-E（全 worker seam が PE へ収束）

**[S17] texture worker upload の resource readiness（image 生成は同期・GPU upload は非同期）**
- touch: `texWorkerUpload`（texture worker）が VkImage/View を**同期生成して呼び出し元へ返す**（`out_image`/`out_view`）が、内容の GPU upload（copyBufferToImage + mip blit `:9200-9260`）は **peEnqueue した one-shot CB で非同期完了**
- required-invariant: その texture を sample する draw は、upload の fence 完了**後**にのみ実行される（未 upload の image を sample しない）
- provider（ph8 で確定）: `runVkUploadJob`（worker `llimagegl.cpp:1602`）が upload を PE 経由 submit、`applyVkUploadJob`（`:1656`）が image handle を texture に公開。**readiness は fence flag でなく順序で担保** = ①one-shot upload は PE FIFO で enqueue → ②その texture を sample する frame CB は後で PE enqueue → ③同一 queue の GPU 実行順で upload が先行完了（upload 末尾 barrier が SHADER_READ_ONLY へ遷移 `:9258-9260`）
- class: **enforced（PE FIFO submit 順 + 同一 queue GPU 実行順）**（S16 の単一 submitter が前提）
- gap: なし（GPU 順序で ready 保証）。**残る小懸念** = `applyVkUploadJob` の image handle 公開（worker か main か）と main の bind 読みの間の handle ポインタ可視性 = 別 hop（avatar/texture phase 精査時）。本地図の draw record 主眼では順序 enforced で足りる
- seams: SEAM-E（texture worker・§3 追加済み）

---

---

## ph12 render_ui / ph13 swap / PE present（consumer CB + present）

**[S18] consumer CB（UI）記録 → PE present + swapchain recreate 協調**
- touch: UI 記録（main）= `recordToConsumer(true)`（`llviewerdisplay.cpp:1821`）で `tRecordCmdOverride = sConsumerCommandBuffers[frameIndex]`（`:5422`）へ切替 → render_ui_2d/blit を consumer CB に記録。scene（producer）CB とは別 CB
- touch: 提出（main endFrame）= consumer CB を frame job（swapchain semaphore + present target 付き `:6230-6243`）で peEnqueue、producer(scene) CB は別 job
- touch: PE present（PE thread）= `vkQueuePresentKHR`（`:1103`）を `sSwapchainAccessMutex`（`:1100`）下で実行
- touch: swapchain recreate 協調 = `sSwapchainRecreatePending`（**atomic** `:686`）を PE が present out-of-date で set（`:1115`）／main が acquire 失敗で set（`:5476/5563`）。**実 `recreateSwapchain()` は main のみ**（`:4599`・beginFrame `:5487`）が `sSwapchainAccessMutex` 下で実行
- required-invariant: (a) UI 記録先 CB の切替（producer→consumer）が正しい (b) present と swapchain 破棄/再生成が同時に走らない (c) consumer が present する front scene は fence 完了済み（=S1）
- provider: (a) thread_local override（S2）(b) `sSwapchainAccessMutex` が PE present と main recreate を相互排他 + `sSwapchainRecreatePending` は atomic (c) S1 の fence gate
- class: **enforced**（(a)=S2 thread_local / (b)=mutex+atomic / (c)=S1）
- gap: なし。consumer/producer cadence 分離（UI 毎 frame present・scene は producer cadence）は memory `handoff_uiscene_decouple_async` の設計で、S1/S2/S16/S18 で機構的に閉じている = **再設計の資産**
- seams: SEAM-A

> **ph3 hero probes / ph7 updateImpostors** = `dispatchRecordJob` を使わない（worker 記録は全 6 site = camera clear / camera pool pass / shadow のみ）→ **main 直列記録**（probe/impostor RT へ main が描き frame CB→PE 提出）。共有 render state を直列順で消費 = enforced。越境 state 行なし（地図の粒度規約）。

---

## ph4 の補足

> **ph4 の残 3 呼び出し** = `createObjects`（新規 object の drawable 生成）/ `processPartitionQ`（partition 移動キュー）/ `updateGeom`（markRebuild 済み group の rebuildGeom 駆動 = S10 の enqueue 側）。いずれも **main 単一スレッド**（S10 の producer dispatch を除き越境なし）。定常 gupd の支配項は updateGeom（memory 既知）。**共有可変 state の越境は S10-S12 が全数**（createObjects/processPartitionQ は main-local 状態機械 → 地図の粒度規約〔境界跨ぎのみ〕により行を立てない・§0 の粒度注記参照）。

**[S3] producer 状態フラグ群（`sAsyncProducerInFlight` `sAsyncRenderSceneThisFrame` `sAsyncProducerBackIndex` = 非 atomic global `llvkloader.cpp:633-638`）**
- touch: W main `:6267`(inFlight=true)/`:6270`(RenderScene=false) endFrame, W main `:5354`(inFlight=false) tryComplete, W main `:5398`(RenderScene=true) beginScene, R main `:5341`/`:5360`/`:5372`/`864`
- required-invariant: これらは producer 記録サイクルの単一状態機械。読み書きが単一スレッドで直列
- provider: 実測上 **全 W/R が main thread**（endFrame/tryComplete/beginScene/display は全 main）。PE thread は使わない（PE は job のコピーを見る）
- class: **enforced**（single main-thread order・非 atomic だが cross-thread 無し）。teardown 経路 `:4308-4312`（`destroySyncObjects`）も **main thread**（shutdown/swapchain recreate・PE thread join 後）= W1 解決
- gap: なし。**ただし enforced の前提 = 「teardown 前に PE thread が join 済み」**（destroySyncObjects が PE 稼働中に走ると PE の submit と fence 破棄が競合）。この shutdown 順序が再設計でも不変条件として維持されること
- seams: —

**[S4] `sAsyncProducerFence`（非 atomic global `:632`）の cross-thread 読み**
- touch: W main `:5353`(vkResetFences)/`:4311`(NULL 化 teardown), R main `:5341`/`:5346`/`:5530`, **R PE-thread `:1006`**（stat 用 `job.fence == sAsyncProducerFence` 比較）
- required-invariant: 実 submit の同期は enqueue 時コピー `job.fence`（`:6263`→`:1012`）で成立し、global `sAsyncProducerFence` は描画正しさに関与しない
- provider: submit は `job.fence`（コピー）を使う（`:1012`）ので**描画は安全**。global の直接使用は main の poll のみ
- class: **accidental**（PE thread `:1006` の非 atomic global 読み = main の reset/null と競合 = データ競合 UB。現状の帰結は **perf counter 破損のみ**で描画無害）
- gap: 今は stat 汚染だけだが、将来 `sAsyncProducerFence` を cross-thread で正しさ判定に使うと即バグ化。**再設計では PE に global を触らせない**（job にコピー済＝global 参照を削除可）
- seams: SEAM-A

---

## §3 seam 注釈（並列化予定の切れ目）

> 各 seam = 直列の全順序を切ってスレッド境界にする候補点。
> seam を引くと「新たに required になる不変条件」（相互排他・cross-thread happens-before）が増える。
> 各 seam に、跨ぐ状態を §2 の行 ID で紐付ける。

| seam ID | 切る位置 file:line | 分離する 2 側（producer / consumer） | 跨ぐ状態（§2 行ID[]） | seam で新たに required になる不変条件 |
|---------|-------------------|-------------------------------------|----------------------|--------------------------------------|
| SEAM-A | `llvkloader.cpp:6203-6271` endFrame の PEJob 投入 | scene 記録（main）→ submit/present（PE thread） | S1, S2, S4 | job にコピーした CB/fence/semaphore が submit まで有効・main が同 CB を再 begin しない（front≠back） |
| SEAM-B | ph6 generateSunShadow（Phase A 済） | shadow cascade 記録（worker）→ 本記録 | S2（+ shadow 固有・ph6 DFS で確定） | worker の thread_local override が正しい CB・1 program 1 job 排他 |
| SEAM-C | ph10 renderGeomDeferred（Phase B 設計中） | gbuffer pool-pass 記録（worker）→ 本記録 | S2, S6, S7（+ gbuffer 固有・ph10 DFS で確定） | worker override 正・**seed 網羅（S6）**・**frame 状態 ctx 転写（S7）**・pool 間状態非共有 |
| SEAM-D | `llvovolume.cpp:6122` geo publish queue | geometry fill（T2 geo worker / avatar worker）→ apply（main） | S10, S11, S12 | mutex 越し所有権移動・gen staleness・validate-then-apply 原子性・喪失↔再予約原子対 |
| SEAM-E | `llvkloader.cpp:8879` one-shot upload → PE | texture upload（T1 tex worker）→ submit（PE thread） | S16, S17 | 全 submit を PE へ集約（単一 queue 所有者）・PE FIFO 順で texture readiness |

---

## §4 トラバース手順（この地図の描き方・不変）

1. **起点** = frame 駆動点。`LLPipeline::render` / display loop の frame entry を file:line で確定し、実 call path を DFS。
2. 各 hop で:
   - ① `file:line → 事実` を §0 `last` に刻む。
   - ② その関数が触る**境界跨ぎの共有可変状態**を §2 に行として排出（`required-invariant → provider → class → gap`）。
   - ③ phase 境界に達したら §1 に phase 行を足し、その phase を §0 `covered` へ、次を `pending` へ。
3. **推測でエッジを繋がない**。呼び先が分岐/仮想で静的に決まらない点 = **壁** → §0 `open walls` に「なぜソースから決定不能か」一文付きで記帳。**壁で fix 仮説・run 依頼に行かない**（まだトレース段）。
4. 3 hop を超えるトレースは context に溜めず**書きながら**進む（§0 を都度更新）。

---

## §5 完了定義（地図全体の DoD）

1. §1 の全 phase が §0 `covered` に入る。
2. §2 の全行に `class` が付く（未分類ゼロ）。
3. `class = accidental / none` の行が**全件** gap 記述付きで存在（= 直列既存欠陥の全数表）。
4. §3 に並列化 seam が引かれ、各 seam の跨ぐ状態が §2 行 ID で埋まる。
5. §0 `open walls` が列挙され、各 wall に「なぜソースから決定不能か」の一文。

> DoD 到達後も **PASS は宣言しない**（憲法 1）。地図の検収・欠陥処分・新設計着手は AYA gate。

---

## §6 Synthesis — 資産 / 欠陥の全数分類と再設計要件（★段階 II への引き渡し）

> §2 の S1〜S18 を「enforced=資産（再設計で温存/横展開）」と「accidental・none=欠陥（再設計で必ず潰す）」に二分し、
> 全欠陥に共通する**単一の根本原因**と、そこから機械的に導かれる**再設計の要件**を示す。

### 6.1 資産（enforced・再設計で死守/横展開する既存の良設計）
- **A1. 描画「現在状態」globals の網羅 thread_local 化** — `gGLModelView`/`sBoundTarget`/`sCurResX/Y`/`sShadowBatchCullRadius`/GLSL bind・per-call・seed・pipe memo/`gVkPerfPassTag`/**`LLPipelineFrameContext` singleton 自体**。worker 記録の state-race クラスを構造的に解消（§2 冒頭確定事項）。
- **A2. 単一 queue submitter（PE thread）** = S16。全 vkQueueSubmit/Present を PE に一本化。異スレッド同時 submit（device-lost/UB）クラスを排除。**中核不変**。
- **A3. geometry 供給の単一 choke** = S10/S11/S12（VKGeo fix）。mutex 越し所有権移動 + gen staleness + **validate-all-then-apply の原子性** + **record 喪失↔rebuild 再予約の原子対**。→ **他の worker 化（draw map 構築等）へ横展開すべき雛形**。
- **A4. present 二重バッファの fence gate** = S1（back=1-front 不変 + fence poll）/ consumer・producer cadence 分離 = S18 / texture readiness = S17（PE FIFO 順）/ shadow depth cross-CB = S9（submit 順 + barrier）。**越境の順序契約はいずれも成立**。
- **A5. thread_local override による記録ルーティング** = S2（cross-thread 面）。worker が自分の CB へ確実に記録。
- **A6. per-draw UBO arena allocator**（`allocPerDrawUBOSlice` `llvkloader.cpp:7592`）= frame 別 arena（`sPerDrawUBOArena[frameIndex]`）+ atomic CAS bump + growth mutex。**cross-frame/cross-thread/cross-draw 安全な per-draw UBO 供給** = per-program UBO の cross-frame 問題を解決済み（深掘り 3 で確認）。**§7.4-4 の frame uniform snapshot（gGLDeltaModelView/GlobalF の ring 化）にこの機構を横展開できる**。

### 6.2 欠陥（accidental・none・再設計で必ず潰す）— **全て「たまたま描けてる」の実例**
| ID | 欠陥 | 症状 | 現在の（不十分な）provider | 再設計要件 |
|----|------|------|--------------------------|-----------|
| **S6** | record seed の網羅ギャップ（unseeded program → descriptor/per-program-UBO offset 両失 → **サイレント draw 消失**） | **Phase B ボディ消失** | shadow=手書きリスト / camera=「1 pass 1 shader」規約 | seed を**実 bind 全 program から機械導出** or record 中 thread-safe on-demand seed。規約でなく機構で網羅 |
| **S7** | worker が読む frame 状態の ctx 手動転写の漏れ（→ 既定値で誤描画） | 位置ズレ/消失/誤マテリアル（サイレント） | ShadowRecordCtx/CameraPassCtx への手列挙 | worker-read 状態を**型で括り構造体丸ごと snapshot**・転写漏れを検出 |
| **S8-b** | shadow worker の VB 読み中に main `rebuildMesh` が同 VB を同期書き | 稀な体消失/関節崩れ/緑汚染（deferred_ledger 符合） | pin は LLDrawInfo を生かすが**下の VB を凍結しない**・`mVkGeoInflight` guard は shadow worker に無効 | worker 窓中 main は共有 geometry を変異させない（rebuildMesh を窓外へ）or group in-flight lock を worker にも効かせる |
| **S13** | 可視 draw 状態（cull/stateSort 生成物）が **world path は「直列だから安全」なだけ** | 並列化で即崩壊（seam 化の瞬間） | main 直列順のみ | record 前に**不変な可視 draw snapshot** を確定し、record 窓中 main はそれを触らない（**再設計の中心命題**） |
| **S14** | `gGLDeltaModelView` が plain global（非 thread_local・ctx 外・sibling と非対称） | motion vector 化け（拡張時） | 「main が窓中に触らない」偶然 | `gGLModelView` と対称化（thread_local or ctx） |
| **S15** | `GlobalF UBO` が phase 毎に main が上書きする単一共有 | phase 並列化/worker 窓延伸で競合 | phase の時系列分離頼み | per-lane/frame ring or ctx snapshot |
| **S4** | `sAsyncProducerFence` を PE が非 atomic 読み（stat 用） | perf counter 破損（描画無害・UB） | 実 submit は job.fence コピー | PE から global 参照を削除（job にコピー済） |
| **S5** | present RT の color 二重・depth 単一の非対称 | 現状 present が depth 不読ゆえ無害・将来 depth-aware composite で衝突 | present blit が depth 不読 | color 二重なら depth も二重の対称性 |

### 6.3 単一の根本原因（全欠陥に共通する形）
全欠陥（S6/S7/S8-b/S13/S14/S15/S4/S5）は**同一の形**をしている:

> **「worker が読む/書く共有可変 state で、その隔離が『機構』でなく『規約・偶然・直列順』に依存しているもの」**

現状の worker 化（shadow=Phase A / materials=Phase B）は、共有 state を **①thread_local 化（A1）②mutex/fence handoff（A2/A3/A4）** で個別に隔離してきたが、**「worker が読む state を漏れなく隔離する体系的な機構」が無い**。だから隔離漏れ（S6 seed / S7 ctx / S14 delta / S15 GlobalF）と変異衝突（S8-b / S13）が残り、**現在の構成でたまたま成り立っている規約が破れた瞬間（pool 拡張・phase 並列化・group の跨ぎ可視）に描画が壊れる**。「たまたま描けてる」の正体はこれ。

### 6.4 再設計の中核要件（この地図から機械的に導かれる単一命題）
> **すべての worker record job は、それが読む frame 状態の完全かつ不変な snapshot の上でのみ動く。snapshot は dispatch 前に原子的に確定し、join まで凍結される。そして『snapshot されていない state を worker が触っていないこと』を規約でなく機構が保証する。**

この命題を満たす具体構成（段階 II の設計対象・ここでは要件のみ）:
1. **可視 draw snapshot**（S13）: cull/stateSort の生成物（可視 group/draw order/VB 参照）を record 前に immutable 化。record 窓中 main は rebuildMesh 含め一切変異しない（S8-b 解消）。
2. **frame uniform snapshot**（S7/S14/S15）: worker が読む描画 uniform（view/proj/delta modelview/GlobalF/per-program UBO）を型で括り per-lane/frame ring か ctx snapshot に。単一共有 mutable を record 窓から排除。
3. **seed の機械網羅**（S6）: 実際に bind される全 program から seed を導出（手書き/規約禁止）。unseeded を**サイレントに消さず fail-closed で検出**（憲法 2）。
4. **既存資産の温存**（A1〜A5）: thread_local 化・単一 PE submitter・geometry 単一 choke・fence/submit 順契約はそのまま土台にする。
5. **隔離漏れ検出機構**: worker が snapshot 外の共有 state を読んだら検出する装置（S6 の seed-lost を fail-closed 化する方向の一般化）。

> **段階 II（再設計）はこの §6.4 を出発点にする**。段階 I（本地図）はここで DoD 到達 = **全 phase covered・全 S 行 class 付き・欠陥全数表（6.2）・seam 全 5 本・壁ゼロ**。ただし憲法 1 により **PASS は宣言しない** — 地図の検収は AYA gate。

### 6.5 申告欄（縮小・省略・解釈）
- **省略**: 対象を 1 frame 描画 call path に限定（§0 冒頭宣言通り）。TP/login/asset ingress/avatar motion worker 内部/settings churn は対象外（別地図）。S17 の image handle 公開可視性・S8-b の実発生頻度は「小 hop / 実測領域」として §0 に残置（設計判断には不要）。
- **解釈**: 共有可変 state の粒度 = 境界跨ぎのみ台帳化（関数ローカル除外）。ph3/ph7/createObjects/processPartitionQ 等 main 直列・越境なしの phase は行を立てず注記のみ。
- **やらなかったこと**: 欠陥の修正・新設計は書いていない（地図に fix を混ぜない = §0 目的通り）。per-program UBO ring 機構と avatar motion worker 内部は「存在確認 + 越境点の分類」までで内部詳細トレースは段階 II 送り（トレースで理解済み = 先送り資格あり）。

---

## §7 record job read-set 全列挙（★S13 snapshot spec・段階 II の凍結対象定義）

> **深掘り TOP の成果（2026-07-28）**。record job（renderShadow / renderDeferred 内側）が触る**共有可変 state を 1 個残らず列挙**。
> これが「worker record を安全にするために dispatch 前に凍結すべき対象の完全定義」= S13/S7/S8-b を一手で解く snapshot 仕様。
> トレース経路: `renderShadow`（`pipeline.cpp:14633`）/ gbuffer `renderDeferred` → `renderObjects`（`:9683`）→ `LLRenderPass::pushBatches`（`lldrawpool.cpp:1196`）→ `LLVKBucket::forEachSource(type, …)`（draw source）→ **`pushBatch`（textured `:1760`）/ `pushUntexturedBatch`（`:1826`）**。

### 7.1 per-draw 読み取り（`LLDrawInfo& params` の読まれるフィールド全数）
| フィールド | 用途（record が何に読むか） | 変異元（誰が書くと壊れるか） |
|-----------|---------------------------|--------------------------|
| `mVertexBuffer`（ポインタ + GPU buffer + **mapped 状態**） | `setBuffer()`/`drawRange()`（`:1815-1816`） | rebuildMesh/geo apply が map して書き換え（S8-b）。**untextured は isMapped skip・textured はガード無し** |
| `mStart` / `mEnd` / `mCount` / `mOffset` | drawRange の範囲（`:1816`） | geo rebuild で再計算（gen 変化） |
| `mModelMatrix` | `applyModelMatrix`（`:1770`→gGL push） | drawable の transform 更新（updateGeom/moved） |
| `mTexture`（LLPointer） | `getTexUnit(0)->bindFast`（`:1789`） | texture evict/再upload → GL handle 差し替え |
| `mTextureList[]`（LLPointer 配列）+ `.size()` | batch texture bind（`:1777-1783`）+ per-draw descriptor slot（`buildAndOverrideScenePerDrawSet`） | 同上（各 texture の bindless heap slot 変化） |
| `mTextureMatrix`（→`mMatrix`） | texture matrix load（`:1795`） | TE アニメ更新 |
| `mAvatar` / `mSkinInfo`（rigged） | matrix palette upload（`uploadMatrixPalette` `:1857`） | skin 更新（prewarm で frame-stamp 凍結 = S8 provider） |
| cull 用フィールド（mCount 他） | `vkShadowCullBatch(params)`（`:1765`） | — |
| **各 texture の bindless heap slot** | `LLImageGL::vkHeapSlotOrDefault(t->getGLTexture())`（buildAndOverrideScenePerDrawSet） | texture evict/再upload で slot 変化（S17 領域） |

### 7.2 container 読み取り（draw の「集合」= stateSort 生成物）
- **pass type 毎の LLDrawInfo\* の集合** = `LLVKBucket::forEachSource(type,…)`（`:1220`）が yield する visible draw 列 = **cull result の render map**。**stateSort/updateCull が生成し、resetDrawOrders/rebuildMesh/再ソートで変異**（S13 の中核）。record 中この集合が変わると走査が壊れる。
- **shared program instance**（`LLGLSLShader`）= `mFeatures.mIndexedTextureChannels`/`mVkUsesBindlessHeap`/`mVkDescriptorSetLayout`（record 中は read-only）+ `mVkPerProgramUBO`（S6/S15・seed の dynamic offset で凍結）。

### 7.3 cross-cutting 読み取り（frame 現在状態 globals）
- `gGLModelView`（thread_local ✓・ctx memcpy）/ program bind（thread_local ✓）/ frame context（thread_local ✓）= **A1 資産で既に隔離**。
- `gGLDeltaModelView`（**非 thread_local・S14**）/ `GlobalF UBO`（**単一共有・S15**）= **未隔離**。
- texture unit heap slot cache（`LLTexUnit::mCurrVkHeapSlot`）= **`gGL` 自体が `thread_local LLRender`（`llrender.h:488`）** ゆえ texunit 配列・heap slot・matrix stack・blend state すべて worker 毎に独立 = **thread_local ✓（loose end 解消）**。Phase A の "slot cache 化" + gGL thread_local で worker の texture slot 汚染は根治済。
- **∴ cross-cutting frame state で未隔離なのは `gGLDeltaModelView`(S14) / `GlobalF UBO`(S15) / `per-program UBO`(S6・seed で凍結) の 3 つのみ**。残り（gGL 一式・gGLModelView・frame context・program bind・texunit/heap slot）は全て thread_local = A1 資産。

### 7.4 ∴ snapshot spec（段階 II が dispatch 前に原子確定・join まで凍結すべき対象）
worker 化する pass について、以下を**不変 snapshot**として確定し、record 窓中 main は一切変異させない:
1. **draw 集合**（7.2）: pass type 毎の LLDrawInfo\* 列を record 前に固定（stateSort/rebuildMesh/resetDrawOrders を窓外へ）。
2. **各 LLDrawInfo の read フィールド**（7.1）: 特に **VB 内容を窓中 map/書換しない**（isMapped skip でなく不変契約で）。mModelMatrix/mStart..mOffset/texture 参照/skinInfo。
3. **texture の GPU 実体と heap slot**（7.1 末）: 窓中 evict/再upload しない（or slot を snapshot 値で固定）。
4. **frame uniform**（7.3）: gGLModelView（済）に加え **gGLDeltaModelView・GlobalF・per-program UBO を per-lane/frame ring か ctx snapshot 化**（S14/S15/S6）。
5. **skin matrix palette**（prewarm で frame-stamp 凍結・既存 provider を踏襲）。
6. **seed = draw 集合が bind する全 program から機械導出**（S6・手書き禁止・unseeded は fail-closed 検出）。

> **これが「規約でなく機構」の実体**: 上記 6 項を snapshot として型で括り、「snapshot 外の共有 state を record が読んだら検出する」装置（seed-lost の fail-closed 一般化）を付ければ、S6/S7/S8-b/S13/S14/S15 が**同時に**閉じる。段階 II の設計対象はこの snapshot 構造体と凍結契約。

### 7.5 申告欄（本深掘り）
- **省略**: `vkShadowCullBatch` の内部フィールド網羅・`buildAndOverrideScenePerDrawSet` の GLTF material UBO 分岐・rigged palette upload の内部は「読む対象カテゴリの確定」までで内部詳細は段階 II 送り（read-set の *カテゴリ* は網羅・各フィールドの byte 単位は設計時に確定）。
- **未確定小 hop**: LLTexUnit heap slot cache の thread 帰属（§7.3 末・Phase A で緩和済だが最終確認は §8 継続）/ MDI/indirect 経路は record job で skip（`:1207`）ゆえ read-set 対象外を確認済。

---

## §8 S6 pool 別 program 列挙（★Phase B 拡張の可否判定・crowd blocker 特定）

> **次点深掘り成果（2026-07-28）**。worker 化候補 pool の `renderDeferred`/`renderPostDeferred` を実トレースし、
> 「1 pass 1 shader」不変（S6 の worker 化前提）を満たすか pool 毎に判定。= Phase B をどこまで安全に広げられるかの地図。

### 8.1 pool 別判定表
| pool | pass 構造（file:line） | per-draw で shader 切替? | 「1 pass 1 shader」 | worker 化 seed 戦略 |
|------|----------------------|------------------------|-------------------|---------------------|
| **POOL_MATERIALS** | 24 pass = 12 material 変種 × 2 rigged。`beginDeferredPass` が `gDeferredMaterialProgram[idx]` 1 個 bind（`lldrawpoolmaterials.cpp:57-97`）。`renderDeferred` は **texture のみ** per-draw 切替・shader 不変（`:147-170`） | ✗（texture のみ） | **✓ YES** | 現状の per-pass seed で OK（**実装済・動く理由**） |
| **POOL_SIMPLE / FULLBRIGHT（bucketized）** | pass = type、1 program（shadow の untextured 経路で実証済） | ✗ | **✓ YES** | per-pass seed（shadow で実証） |
| **POOL_AVATAR** | pass = **モード**（0=impostor/1=rigid/2=skinned `beginRenderPass:450`）。`render`→`renderAvatars` が per-avatar/mode で複数 program bind（gImpostor/gDeferredImpostor/gAvatar/gObjectAlphaMaskNoColor 等・`lldrawpoolavatar.cpp` 多数 bind site） | **✓ 切替** | **✗ NO** | seed = mode が bind し得る全 program の union / or impostor 経路分離。設計要 |
| **POOL_ALPHA** | **PostDeferred**（`getNumPostDeferredPasses:95`）。`renderPostDeferred` が emissive/pbr_emissive/fullbright/simple を prepare（`:171-189`）し、**draw loop で per-draw に current_shader 切替 + `draw->mGLTFMaterial->bind()` で per-GLTF-material program**（`lldrawpoolalpha.cpp:636-764`） | **✓✓ 激しく切替** | **✗ NO（最悪）** | 下記 8.3 |

### 8.2 結論 = Phase B 拡張の安全境界
- **安全に拡張可**: 「1 pass 1 shader」pool（**materials（済）/ simple / fullbright / bucketized 系**）= 現状の per-pass seed 機構（`ensureCameraWorkerSeeds`）でそのまま worker 化できる。**Phase B の次の一歩は simple/fullbright への横展開**（seed 戦略が既に正しい）。
- **設計を足せば可**: **POOL_AVATAR** = pass がモードで per-avatar shader を切替えるので、seed を「そのモードが bind し得る全 program の union」に拡張するか、shader 単位に pass を割り直せば worker 化可能。ただし impostor 分岐等の状態依存があり設計コスト中。
- **worker 化の難物（crowd blocker）**: **POOL_ALPHA**。

### 8.3 POOL_ALPHA = crowd の真の blocker（2 重の障害）
1. **per-draw shader 切替（seed 機械導出が原理的に困難）**: alpha は draw の material 種別で simple/fullbright/emissive/pbr を per-draw 選択し、**`draw->mGLTFMaterial->bind()` で GLTF material 毎の program を動的 bind**（`:741`）。program 集合が draw 内容に依存して動的 = **事前 seed リストを静的に列挙できない**（materials の「1 pass 1 shader」が成立しない根本理由）。
2. **depth sort 順序制約**: alpha は back-to-front の描画順が正しさの要件。**worker 並列化は draw 順を崩す** → 半透明合成が壊れる。1 job=1 pass の粒度では順序を保てない。
- ∴ **alpha を worker 化するには S6 の「seed 機械導出」では不十分**で、**record 中の on-demand thread-safe seed 生成**（bind される program をその場で seed 化）+ **順序保存する分割**（例: 1 job = 連続 draw span を順序付きで）が要る。**これは段階 II で alpha 専用に設計する項目**。crowd の半透明（衣装・髪・エフェクト）はここに集中する。

### 8.4 S6 再設計要件の更新（§6.4-3 の精緻化）
S6 の「seed 機械導出」は **2 段構え**が要ると判明:
- **静的 pool（materials/simple/fullbright）**: pass = 1 program ゆえ per-pass 事前 seed で十分（現行機構）。
- **動的 pool（alpha・per-GLTF-material）**: 事前列挙不能 → **record 中に bind された program を検出して on-demand で thread-safe に seed 生成**する機構が必須。**この on-demand seed 機構が、S6 を「規約でなく機構」で閉じる本命**（静的 pool もこれで統一できる = 手書き/per-pass 前提すら不要になる）。

### 8.5 申告欄（本深掘り）
- **省略**: POOL_AVATAR の renderAvatars 内部の全 bind site の網羅列挙（mode 別 program の union 具体化）は「1 pass 1 shader 不成立」の確定までで、union の完全集合は段階 II の avatar worker 化設計時に確定（本地図は可否判定が目的）。GLTF PBR opaque pool（POOL_GLTF_PBR）は materials 類似だが未精査（simple 系と同様「1 pass 1 shader」の可能性が高いが要確認 = 継続小 hop）。
- **解釈**: 「worker 化可否」= seed 戦略の観点のみ。alpha の順序制約は seed とは別軸だが同 pool の worker 化 blocker として併記した。
