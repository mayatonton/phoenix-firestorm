# llvkloader.cpp pure-move 分割 詳細設計(2026-08-11・デカファイル解体 ②・設計 v1)

**前提**: ①(pipeline.cpp・`cb9f74df394`)で確立した方式の流用。原則 = 可読性・検証性が安全性(AYA 裁定)。**llvkloader は AYAstorm 著作 = file ヘッダーは AYA 著作ヘッダーを引き継ぐ**(① と系譜が違う点に注意)。

## 1. 対象の構造(実測)

- 14,358 行。`namespace LLVKLoader { }`(:83〜)+ 内部に**無名 namespace の共有 state 帯**(:90〜・laneIndex/scene cache/PE queue/perf 等の内部 state と helper が居住)。
- ns 定義は indent-4。**LLVKLoader は namespace なので再開可能 = class 分割より素直**。
- fn ≈ 116 + 内部 class/struct ≈ 34 + state 変数多数。① と違い **file-scope state の密度が桁違い**(loader の全 state がここに住む)= 帰属判定が本工事の本丸。

## 2. 分割形(案・全数割当は実装 Brief で確定)

| 新 file | 守備範囲(fn 実名の族) |
|---|---|
| llvkdevice.cpp | createInstance/selectPhysicalDevice/selectQueueFamily/createDevice/queryAndLogDeviceLimits/vkDebugCallback/findMemoryType/createCommandPool/createVmaAllocator/createPipelineCache* |
| llvkswapchain.cpp | create/destroy/recreateSwapchain・createSyncObjects/destroySyncObjects |
| llvkpresent.cpp | PE thread 一式(peExecute/peThreadMain/peEnqueue/peSubmitBlocking/peDrain/peStart/peStop)+ timeline(gpuTimelineValue/waitTimeline/recordAbandonedTimelineValue)⚠️ 単一 submitter = load-bearing・pure move 厳守 |
| llvkimages.cpp | createWhiteImage/createDefaultFallback* 5 種/createAttachmentImageVkImpl/uploadImageDataVkImpl/submitOneShotVk 族 |
| llvkbuffers.cpp | createBufferVkImpl 族・megabuf 一式(megaAlloc/Free/New/Grow/EnqueueFree/Owner*)・vbStaging/vbUpload 族 |
| llvkheap.cpp | bindless heap(createBindlessHeap/bindlessWriteSlotInternal/noteViewHandleCreated/noteBufferHandleCreated/set1BirthLookup) |
| llvkperdraw.cpp | scene per-draw cache(laneIndex*/laneUnindex*/slotSlabGrow/allocDomainForSlot)・per-frame descriptor(createPerFrame*/writePerFrameSetBindings/createMatrixRingChunk/allocRingDescriptorSet/ensurePerFrameMatrixSlot/createScenePerDrawDescriptorPool) |
| llvksampler.cpp | createStandardSampler/encodeSamplerStateKey/getSamplerForStateImpl/llGlEnumToVk* 変換 5 種 |
| llvkrecord.cpp | threadCmdPool/currentRecordCmd/reapReady/vkcRaceSelf/vkCmdMemoEnabled |
| llvkdiag.cpp | devlost 診断(gpuCheckpointImpl/dumpCheckpoints/dumpDeviceFault/Darwin submit trace)⚠️ PR#138 系 = 診断装置・pure move 厳守 |
| **llvkloader.cpp(core 残留)** | init/shutdown 駆動・上記に属さない公開 API・共有 state 定義 |

- 検出器隣接(VkPerf 計上・VKC 呼び)は**移動のみ・1 行も意味を変えない**(憲法 4 は llvkcontract.* が対象だが、隣接配管も同水準の扱い)。

## 3. 共有 state(無名 ns)の帰属方針 = ① §3 のスケール版

1. **単独サブシステム利用** → 利用先 file 内の無名 ns へ同居移動(内部 linkage 維持)。
2. **複数サブシステム利用** → `llvkloaderinternal.h`(新設)へ宣言を出す。無名 ns は TU をまたげないため、**named 内部 namespace(`LLVKLoaderInternal`)へ変換して external linkage 化**(定義は core に残置)。= 意図的 linkage 変更・完了報告で全数列挙(① の extern 4 行の同型・規模は大きい見込み)。
3. 帰属判定は ① と同じ **compiler 反復**(旧位置から消す → undeclared が要求元を全列挙)。

## 4. 抽出方式(① からの強化点)

- ① の素朴 brace 追跡は**文字列/コメント内の brace で誤動作**(本 file で実証済)→ **コメント・文字列リテラルを剥がした影で brace 追跡**する parser に強化して範囲確定(原文は無傷のまま切る)。
- 万一の誤切断は **pure-move oracle(全行 multiset 照合・① 実証済)+ link エラー**が必ず捕まえる = oracle が最終安全網。

## 5. gate(① と同一)

build/link 収束 → **oracle: 分割前 rev との全行 multiset 照合で非意図差分 0**(意図分類 = ヘッダー複製・include・guard・宣言 hoist・named-ns 変換)→ 配置スポットチェック → 判定走行(硬チャネル 0・VKC 全層沈黙)→ 層 2 視覚(AYA)。

## 6. 申告欄(設計時点)

- named-ns 変換(linkage 変更)の規模は実装時に確定・全数列挙義務。
- include block 複製・pruning 対象外(① と同じ)。
- CMake = llrender/CMakeLists.txt の共通リストへ登録(3 OS 共通)+ header 登録。
- 実装は fresh 文脈で本 doc + `scratchpad/vkloader_units.txt`(fn 目録)を入力に開始するのが安全(① の手順 doc と oracle script は再利用可)。

## 7. 実装結果(2026-08-11・gate PASS)

- build 3 巡収束(286→86→0 error)・**pure-move oracle = 分割前 rev(git HEAD md5 一致の凍結コピー)との全行 multiset 照合で非意図差分 0**。意図クラス = ヘッダー/include 複製・ns wrapper・宣言 hoist(header 移設)・落とした旧 static 前方宣言(マニフェスト厳密一致)・static/inline 剥がし 12 対(1:1 照合)・編集ペア 1。
- 結果: 14,358 行 → **core llvkloader.cpp 2,775 + 10 file(llvkdiag 302〜llvkimages 3,523)+ llvkloaderinternal.h 686 行**。fn 実数 368(設計時 ≈116 は旧目録の噪音)。CMake = llrender 共通リスト + header 登録(3 OS 共通)。
- 帰属の実装確定(設計 §2 が黙る族): frame 駆動・pass 管理・deferred free 駆動・worker registry・VkPerf scope → core / texture upload・copy・readback 帯 → images / UBO・skin palette 帯 → perdraw / aux window → swapchain / occlusion・timestamp query・bind memo → record / indirect ring → buffers / peJobType → diag(#if LL_DARWIN 帯内在)。
- 共有 state 帰属実績: **extern 化 ~200 var**(shutdownVulkan がほぼ全 state を直接参照するため multi-user 化)= llvkloaderinternal.h の LLVKLoaderInternal ns へ宣言・定義は原位置(core または band 家 file)。struct/constexpr/無名 enum(RECREATE_REASON/PE_SLOT)は header へ本体移設。pcache は無名→ named `LLVKLoader::pcache` 昇格(core が blob 書出しに使用)。
- pure move からの意図的逸脱(全列挙): ① createBufferVkImpl の default 引数を定義側→header 宣言側へ移設 ② static/inline 剥がし 12(linkage 外部化)③ 旧 static 前方宣言ブロック(~20 行)削除 = header 宣言で代替 ④ LLVK_SHARED_UBO_RING_STORAGE macro 生成変数 60 個を LLVKLoaderInternal 昇格 + header に decl macro 新設(shutdownVulkan の RING_TEARDOWN が直接参照するため)⑤ Darwin 帯(旧 950-1056)は diag へ原子移動・header 宣言は #if LL_DARWIN guard 付き ⑥ band 帰属済み multi-user var 23 件は §3.2 の「core 残置」でなく家 file の LLVKLoaderInternal に定義残置(移動最小化)。
- 判定走行(2026-08-11・VKC 有効 3 分・正常終了): ERROR/FATAL/VUID/devlost/crash = 0・S1 沈黙・VKC 発火 = fb_heap_default 族(allowlist ACCEPTED)のみ・skips=0。視覚 OK(AYA)。
- 残 OPEN(本工事非接触・network/asset 層・AYA 裁定持ち越し): `_httppolicy:404` Forbidden(Http_403)×332 / Not Found(Http_404)×10・`llcorehttputil:276` Possible failure [Http_403] material_id GET ×150・XML parse `cap not found` ×9・`#OctreeErrors# Empty leaf` ×3(allowlist 現行パターン不 match・灰色 texture 監査の残 OPEN と同族)。
- Mac/Windows 実機未検証(① と同じ・構造要因なし)。VMA_IMPLEMENTATION と CMake の Darwin warning 例外は core のみ据え置き。
- unit 帰属の詳細は各 file の HEAD が唯一の真実(生成スクリプト・目録は session scratchpad 限りで再現可能な手順のみ §4 に恒久記録)。
