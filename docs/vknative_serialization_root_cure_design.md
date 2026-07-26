# per-frame 直列パイプライン 根治 — 記録の per-core 並列化(設計 + 実装計画)

- 状態: **設計者起草ドラフト 2026-07-26・approve 前**(AYA 指示「per-frame 問題を完全根治・TOP・詳細設計まで詰めて実装計画」)。HEAD `3f559081e73`。
- 位置づけ: `docs/vknative_recovery_plan.md` §0.1 のガン(単一 main thread 直列)の**本体治療**。`docs/vknative_perdraw_record_recovery_design.md` §9(AYA realization = serialization-bound)の次フェーズを、実測で機構を確定した上で **戦略2C(記録の per-core 分散)を本丸として**具体化する。§9 が「本丸」とした frame pipelining は本 doc §0 の mlp 実測で**既に成立と判明**したため降格。
- 真実源: HEAD 実コード + 実測ログ(`~/.ayastorm_x64/logs/AYAstorm.log`=classic 37fps / `.old`=crowd)。トレース = scratch `trace_serialization.md`。

---

## 0. 診断(実測で機構確定・§9 を精緻化)

### 0.1 frame は「単核直列記録」bound = phantom sync 点は無い

§9 は「FRAMES_IN_FLIGHT=3 があるのに直列 = どこかの sync 点が overlap を殺している(present_wait/fence/acquire 疑い)」と仮説した。**mlp 計器の実測がこれを反証する。**

- pipeline は**既に 3-deep**: `sFrameIndex` は beginFrame 冒頭で前進(llvkloader.cpp:5496)、fence 待ちは slot **N-3** の `sInFlightFences`(:5504-5517)= main は 3 frame 先行できる。
- main の GPU 同期待ちは**全てゼロ**(classic 37fps・mlp per 5s/~180f):

  | mlp 欄 | ms/5s | ms/f |
  |---|---|---|
  | slot(peWaitSlotSubmitted) | 0.0 | 0 |
  | fence(inFlight 待ち) | 0.7 | 0.004 |
  | acq(acquireNextImage) | 1.5 | 0.008 |
  | beg(beginFrame 総) | 24 | 0.13 |
  | end(endFrame 総) | 1.4 | 0.008 |

- GPU/present は PE thread(別 thread・mt=1)で submit 41 / present 42 ms/5s = main を塞がない。
- **∴ CPU 記録と GPU 実行は既に overlap している。frame time = main が display() 記録(20-30ms/f)+ idle()(5-14ms/f)を単核で直列実行する時間そのもの。**
- crowd(mlp acq 172-230ms/5s ≈ 2ms/f)でも frame 40-70ms に対し小(かつ multiwindow の aux swapchain mutex 競合の混入疑い)= 支配項は依然 display 記録。

### 0.2 §9 との整合(何が正しく何が誤りか)

- **§9 の高次結論は正しい**(affirm): draw 数削減(戦略2B)は frame time に効かない。理由 = frame は work 量でなく**単核直列**に律速される。
- **§9 の機構仮説は誤り**(refute): 「sync 点が overlap を殺している」は mlp でゼロ。pipeline は既に働く。∴ **cure は phantom sync 点探しでなく、単核の直列記録を N コアに割ること = 戦略2C が本丸**(§9 が副次とした 2C が実は本命)。

### 0.3 なぜ「消去(MDI)」でなく「並列化(2C)」か

doctrine [[project_vk_doctrine_eliminate_not_parallelize]] は「消せるものは消せ(MDI/bindless/記録保持)」。**static 幾何は既に消済**(M4a/M4b bucket + M5a MDI)。残る per-frame 記録 ~15k draw の主体は **rigged(avatar)/dynamic** で、per-avatar skin palette と毎 frame pose 更新ゆえ **static-MDI で消せない**(B.3 で shadow rigged を MDI 化しても §9 で frame time 不変=serialization-bound)。**消せない記録は並列化するしかない** = off-main + join 無し regime で doctrine が parallelize を解禁する当の領域(perdraw doc §3)。

---

## 1. 根治テーゼ

**消せない per-draw 記録(rigged/dynamic ~15k draw)の command buffer 記録を、単核直列から N record-worker への並列記録へ移す。** これにより display 記録 20-30ms/f が ~1/N + join に縮み、単核直列という癌の本体が構造的に消える。GPU は既に餓えている(28-36%)ため、記録が速く供給されれば frame は CPU 記録律速から解放される。

---

## 2. 既存資産 = 並列記録基盤は実装済み・休眠中(再利用が設計の核)

MT-2b(commit `f1283688bb0`)が shadow static 記録の per-cascade 並列化のために構築 → static が bucket+MDI で消滅したため退役(`b663c7f6ccd`)、だが commit 明記のとおり **「lane 機構本体と record-safe guard は M6 転用まで休眠温存」**。= 消せない rigged 記録の並列化のために予約されていた。

| 部品 | file:line | 状態 |
|---|---|---|
| `dispatchRecordJob(body)` / `joinRecordJobs()` | llvkloader.cpp:5677 / :5701 | 実装済・**呼び出し元ゼロ** |
| `rwExecute` / `rwThreadMain` / worker 群 `aya-rec1..N` | :1396 / :1440 | 実装済(rw=4・`AYASTORM_MT_THREADS<=1` で 0 退化) |
| `RecordLaneCmds`(per-lane PRIMARY CB ×FIF3・per-frame reset) | :1286 / :1345 | 実装済 |
| join→submit 経路(`sRWFrameCmds`→`sPendingPreFrameCmds`→次 submit pre_cmds) | :5711 | 実装済(worker CB は main CB の**前**に走る) |
| per-thread 記録状態 = **全て thread_local** | tRecordCmdOverride(:208)/sInDynamicRendering(:139)/tInRecordJob(:1302)・llglslshader sCurBoundShaderPtr/sCurPerCallVk*/sVkPipeMemo*(llglslshader.h:202,387-392) | thread-safe 済 |
| `resetPerThreadRecordState()` | llglslshader.cpp:3466 | job 頭で binding memo クリア |
| record-safe guard `isRecordJobActive()` | lldrawpool.cpp:1206/1236/1397/1833/2323 | 残置(MT-2b の遺産) |

**含意**: 並列記録の最難関(記録経路の thread 安全化)は MT-2b が完了済み。本 doc は「基盤を rigged/dynamic 記録へ**再配線**する」工事であり、新基盤の発明ではない。

### 2.1 現基盤モデルの制約(Phase 分割の根拠)

join モデルは per-lane **PRIMARY** CB を pre-frame cmd として submit する = **各 job が自己完結した render scope(独自 dynamic rendering)**である必要がある。∴:
- **自己完結パス(別 render target)= 現基盤で即並列化可**: shadow cascade(各 shadow[j])・probe 面・hero/鏡。
- **単一パス内の draw 分割 = secondary CB + `vkCmdExecuteCommands` への拡張が必要**: camera scene pass(main CB の dynamic rendering scope に interleave)。現基盤は primary CB モデルゆえこの拡張が別工事。

---

## 3. Phase 計画(payoff × tractability 順)

| Phase | 標的 | 手 | 基盤 | payoff | 難度 |
|---|---|---|---|---|---|
| **A** | **shadow pass(4 cascade+2 spot・rigged 込み)** | cascade 単位で別 primary CB へ並列記録 | 現基盤そのまま | crowd freeze 主因(shad avatar×cascade 線形)を直撃 | 中(基盤流用) |
| **B** | camera scene pass(~15k draw) | draw-list を N chunk 分割 → secondary CB + `vkCmdExecuteCommands` | 基盤 + secondary CB 拡張 | 軽シーンの支配項 display 記録 | 高(pass 内分割) |
| **C** | idle() の avatar/object 更新(obj 6.9ms/f) | 粗粒度(subsystem 単位)並列・全量 tick 廃止 | Phase2 検出器 D1-D4 | idle 5-14ms/f | 高(step B 教訓 = per-avatar 粒度は死案) |

- **着手順 = A → 計測 → B → 計測 → C**。各 Phase は独立に非退行で gate 可能(kill switch `AYASTORM_MT_THREADS=1` で全 inline 退化)。
- Phase A で crowd freeze(北極星「50-100av で固まらない」)の主因に効くかを先に実証してから、より難しい B/C へ。

---

## 4. Phase A 詳細設計(実装直前・approve 待ち)

### 4.1 対象と自己完結性(確定)

`renderShadowMaps`(pipeline.cpp:~14800-14874)の cascade loop: 各 j で
`getFrameRT()->shadow[j].bindTarget()` → `renderShadow(view[j],proj[j],...,result[j],true)` → `.flush()` → `.bindForShaderRead()`。
各 cascade は**独立 render target への自己完結 render**。shadow[j] は後段 deferred lighting でのみ SAMPLE = cascade 記録同士に依存なし。∴ 各 cascade の記録を record worker へ dispatch し、全 cascade を join してから lighting へ進めば byte 同一。

### 4.2 改修方針(最小・kill switch 配下)

1. **cascade loop の記録を job 化**: 各 j の `bindTarget/renderShadow/flush` を `dispatchRecordJob([j,...](VkCommandBuffer cmd){ ... })` の body に包む。worker は tRecordCmdOverride=lane CB へ記録。loop 末尾で `joinRecordJobs()`。`bindForShaderRead`(layout 遷移)は join 後に main で実施(全 cascade の記録完了後)。
2. **global 状態の job param 化**(worker が触る前に capture):
   - `LLRenderPass::sShadowBatchCullRadius`(:14854 で cascade 毎 set)→ job ローカル値へ。global 参照を record 中に読む箇所を job param か thread_local へ。
   - `set_current_modelview/projection`(:14820)= 描画行列。record 経路は per-draw data 経由だが、cascade 毎の view/proj を job 内で設定する必要 = matrix 設定 API の thread 安全性を実装時トレース(gGLModelView 等が thread_local か要確認)。
   - `static LLCullResult result[4]`(:14846)= 既に cascade 別 index ゆえ job 間で非共有。読取専用。
3. **RT bind の thread 安全性**: `shadow_rt.bindTarget()`(LLRenderTarget)が dynamic rendering を開始する = worker CB へ begin する必要。sInDynamicRendering は thread_local(:139)ゆえ per-worker で成立。ただし RT の内部状態(sBoundTarget 等)が global static なら要 thread_local 化 = **実装時の最重要トレース点**。

### 4.3 未確定 = 実装時トレースで潰す(先送り資格あり = 何を/なぜ/再開トリガー明記)

- **U1**: LLRenderTarget::bindTarget/flush の内部 global(sBoundTarget/sCurResX 等)の thread 安全性。再開トリガー = Phase A 着手時に llrendertarget.cpp を実読。thread_local 化 or job 直列部への退避で対処。
- **U2**: 描画行列設定(set_current_modelview → gGLModelView)の thread_local 性。再開トリガー = 同上。
- **U3**: mega-buffer/indirect ring/skin palette ring の worker 並行 acquire 安全性(rigged が触る)。skin palette cursor は per-frame-index(:5497)。再開トリガー = rigged を cascade job に含める段。
- これらは MT-2b が static shadow で通した経路 = 前例あり(rigged 追加分の差分のみ新規)。

### 4.4 gate(命題様式)

- **命題**: 「shadow cascade の command 記録を単核直列から per-cascade worker 並列へ移しても、影の描く物・pose・skin・cull は byte 同一(記録先 CB が変わるだけ)、frame time のみ短縮」。
- 検証: **影の視覚同一**(pose/セルフシャドウ/cascade 境界)+ validation 0 + device-lost 0 + 診断起動で全層オラクル沈黙 + `ph shad`↓(並列化で wall 短縮)+ mlp/pe で GPU 待ち非増 + `rw=4` 稼働。
- kill switch A/B = `AYASTORM_MT_THREADS=1`(全 inline 直列)vs 既定(worker 並列)で同一シーン比較。
- **実効設定確認欄**: RenderShadowDetail>0(cascade 有効)・crowd av 数・shadow_rt 解像度。

### 4.5 期待配当

- crowd で shad は avatar×cascade 線形(drawcount doc §7 = 50-100av で 10-20ms/f)。4 cascade 並列 → wall ~1/4 + join。**北極星(50-100av で固まらない)の主因を直撃**。
- 軽シーンの shad は小(§7 = 軽シーン支配は light/GPU 供給)ゆえ Phase A の軽シーン fps 配当は控えめ。**Phase A の gate は crowd での shad wall 短縮**で判定(段階0 常時 PERF_LOG + 混雑会場)。

---

## 5. Phase B 概略(camera scene pass・後続)

- ~15k draw の記録を N chunk に分割 → 各 worker が **secondary CB** に記録 → main が dynamic rendering scope 内で `vkCmdExecuteCommands`。
- 現基盤(primary CB)の secondary CB 拡張 =(a)`VkCommandBufferInheritanceRenderingInfo`(dynamic rendering の secondary 継承)(b)chunk 分割境界(spatial group/pool 単位)(c)per-chunk の bind 状態独立性。
- draw-list の chunk 化は cull 結果(LLCullResult)の range 分割で自然に切れる。順序非依存性(alpha は順序依存 = 分割不可・opaque/shadow は順序自由)を pass 別に判定。
- JIT 詳細設計は Phase A gate 後に起草(secondary CB 拡張の規模は未見積 = 申告)。

## 6. Phase C 概略(idle 更新・後続)

- obj 6.9ms/f(avatar 支配)。step B 教訓 = **per-avatar 粒度は死案**(join ~31µs/av 支配)。∴ **粗粒度**(subsystem 単位: avatar update / object update / particle を別 lane)+ 全量 tick 廃止(dirty のみ)。
- 安全 = Phase2 検出器 D1-D4(`81b7b0aef4`)+ TSan。JIT は Phase B 後。

---

## 7. 全体検収(成功条件)

1. **crowd settled で main 単核直列の解消**: ph 表 shad/disp の wall が worker 数分の 1 へ、cpu が複数コアへ分散(top で main<100% でなく複数コア稼働)。
2. **北極星**: 50-100av で viewer が固まらない(freeze = main 飽和の解消)。
3. 視覚同一 + validation 0 は全 Phase の前提(品質トレード禁止 = 記録先 CB が変わるだけ・描く物は byte 同一)。
4. 低スペック実機での回帰確認(単核弱・多コアで並列の配当が最大 = 本治療の主受益者)。

## 8. 縮小・省略・解釈申告

1. **Phase A は shadow のみ**。camera scene(B)・idle(C)は後続 Phase。「完全根治」は 3 Phase 完遂で達成 = 本 doc は分割着手(feedback_dont_defer_big_tasks_decompose)。
2. **crowd での支配項確定は段階0(常時 PERF_LOG)+ 混雑会場**に従属。軽シーン(現ログ)は display 記録 bound と確定済だが、Phase A の主配当(shadow)は crowd 固有 = 混雑会場の実測で gate。
3. **U1-U3(§4.3)は実装時トレースで潰す**(先送り資格 = MT-2b 前例あり・再開トリガー明記済)。RenderTarget/行列設定の thread 安全性が Phase A の唯一の技術リスク。
4. §9 の「pipelining が本丸」降格は mlp 実測(§0.1)に基づく設計者判断 = AYA 承認対象(§9 は AYA realization ゆえ、本 doc の §0.2 精緻化を明示裁定に上げる)。
5. Phase B の secondary CB 拡張規模・Phase C の粗粒度分割は未見積 = 各 JIT で確定。
6. kill switch `AYASTORM_MT_THREADS` は既存(工事足場でなく恒久の MT 退化 switch)= 本工事の A/B に流用、撤去不要。
