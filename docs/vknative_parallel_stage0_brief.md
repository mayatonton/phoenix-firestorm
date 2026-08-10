# Stage 0 実装 Brief — 凍結相前倒し + 並列前提の地盤工事(直列のまま・fork なし)

制定 2026-08-10。設計正本 = `docs/vknative_parallel_record_feasibility.md` §6(特に §6.1-6.4)。担当 = 設計者(本 Brief)→ 実装 Fresh。
**Stage 0 は fork を一切導入しない。全工事は直列のまま挙動恒等で gate し、並列化はしないのに正しさと per-draw 費が改善する形に閉じる。**

## B0 範囲と非目標
- 範囲: ①凍結相確立(cull/stateSort 摘出 + freeze walk + fire 縮退 + palette prewarm)②計器正規化 ③indirect ring 並列安全化 ④per-shader pipeline map mutex。
- 非目標(やらない): fork・lane 復配線・PassContext 引数化・pool/shader member lane 化・VKC lane 対応(= Stage 1/2)。**camera 側 pass は一切触らない**(影スライス scoping・AYA 裁定)。
- 憲法遵守: 検出器の新設(B6)は**憲法 4 = AYA 承認までコードに入れない**。B6 以外は既存検出器に一切触れない。

## B1 前提事実(トレース済・実装者は着手前に自分の目で再確認)
| 事実 | file:line |
|---|---|
| 影 MV 経路: union cull 結果 → MV 一括 record → per-layer 補完 record ×4 | pipeline.cpp:15214(union_result)/:15229(LLRTScope)/:15236 帯(MV record)/:15261(per-layer renderShadow) |
| 非 MV fallback: per-cascade に cull↔record 交互(`sun_result[j]` は既に配列) | pipeline.cpp:15322 |
| `renderShadowOpaqueBucketizedMultiview` 内の cull/stateSort(RT bind 下で実行中) | pipeline.cpp:13780-13781 |
| `renderShadowOpaqueBucketized`(非 MV)内の cull/stateSort | pipeline.cpp:14049-14050 |
| `stateSort(LLCamera&…)` の末尾が `postSort` を呼ぶ = rebuildGeom 変異は stateSort に内包 | pipeline.cpp:4446 |
| MDI fire が record 相中に template rebuild + 全数 author | lldrawpool.cpp:1424(rebuildTemplateIfDirty)/:1490-1497(computeDrawDataSlots+ensure+mdiAuthorAndCheck) |
| `ensureVkDrawDataSlot` = memcmp memo(slot 有効かつ内容不変 → 純 read no-op) | llspatialpartition.cpp:4182-4197 |
| `computeDrawDataSlots` は純 read(`vkHeapSlotOrDefault` = slot 読み+白 fallback・確保しない) | lldrawpool.cpp:517-563 / llimagegl.cpp:1543- |
| texture/heap publish は update 段(record 外)= INV-P2 の現行適合 | llviewertexturelist.cpp:1296 |
| palette lazy build の record 相中実行 | lldrawpool.cpp:2112(`updateSkinInfoMatrixPalette`) |
| indirect ring cursor = 素 U32 + lazy 生成/frame reset | llvkloader.cpp:408/:12735-12787 |
| 相前 tick の既存置き場(beginCommandRecording 帯) | llvkloader.cpp:5645-5663(tickPerDrawUBOArena :5663) |
| pipeline map の全 accessor(find/insert/clear) | llglslshader.cpp:4173-4174/:4208/:4465/:4469/:356-363 |
| shamdi delta idiom の全 3 site | lldrawpool.cpp:1351-1352/:1880-1881・pipeline.cpp:13962-13963 |
| 非 atomic 計器(既知 2) | lldrawpool.cpp:2054(mTextureMatrixOps++)・llvkloader.cpp:13889 帯(s_rt_resume_count) |
| probe 再入 = `gCubeSnapshot` 時は cascade 2 本(同一関数・同一工事で被覆) | pipeline.cpp:14773 |

## B2 工事 A: 凍結相確立(§6.1・本丸・単独 commit)
**破れている不変条件**: 「record 相(RT bind〜pass end)中に cull/stateSort/postSort・template rebuild・DrawData author・palette build が走る」。
**目標形**: `generateSunShadow` 内を **[凍結帯: 全 cull → freeze walk] → [record 帯: 純 read record]** の 2 相に再配列。

### A1: cull/stateSort の record 関数からの摘出(3 箇所)
1. **MV 主経路**: `renderShadowOpaqueBucketizedMultiview` から `updateCull(cam,result); stateSort(cam,result);`(:13780-13781)を削除し、呼び手 `generateSunShadow` の **LLRTScope(:15229)より前**に同一引数で移設。関数 signature は不変(result は引き続き引数)。
2. **非 MV `renderShadowOpaqueBucketized`**: 同様に :14049-14050 を削除。呼び手 = `renderShadow`(render_opaque_bucketized=true の経路)だが、renderShadow 自体も RT bind 下で呼ばれるため、**cull は generateSunShadow 側の record ループ前へ**(下記 A1-3 と同じループ分割で吸収)。
3. **非 MV fallback(:15322 帯)**: 現行 `for j { cull(j); record(j); }` を `for j { updateCull+stateSort(tight_shadow_cam[j], sun_result[j]); }` → `for j { renderShadow(..., sun_result[j], ...); }` の 2 ループへ分割。`sun_result[j]` は既に per-cascade 配列 = 4 結果同時保持のメモリ構造は既存。
- ⚠️ `stateSort` は :4446 で `postSort`(rebuildGeom = VB 書換)まで内包する。A1 により **rebuild/vbStageCopyVk は全て凍結帯へ寄る** = `vb_inflight_hostwrite` 検出器(恒久残置)が凍結帯移設の正しさを裏から監視する(post-A1 で in-pass 残存が出れば即 FAIL)。
- ⚠️ cull が RT scope 外へ出ることで perf 計時の帰属(shadow section 計器)が移動する。gate では形状変化として扱い、値の消失とは区別する。

### A2: freeze walk(新関数 `shadowFreezeAuthor` 相当・凍結帯の末尾 = 全 cull 完了後に 1 回)
対象 pass 集合 = `LLVKBucket::kOpaqueShadowPasses` 全数 + 影 MDI alpha 系(PASS_ALPHA_MASK / PASS_FULLBRIGHT_ALPHA_MASK / PASS_NORMSPEC_MASK / PASS_MATERIAL_ALPHA_MASK / PASS_SPECMAP_MASK / PASS_NORMMAP_MASK / PASS_GRASS / PASS_ALPHA + rigged 対応員)+ PASS_GLTF_PBR。**列挙は実装時に `shamdiCellForPass`/`shamdiRiggedCellForPass`/`kOpaqueShadowPasses` の定義から機械的に導出し、漏れ疑義は設計者へ**。
処理(pass ごと):
1. `for (Bucket* b : bucketsForPass(pass)) rebuildTemplateIfDirty(*b);`
2. `LLVKBucket::forEachSource(pass, author_one)` — `author_one(LLDrawInfo& rec)`:
   - **per-frame dedup**: `rec.mVkAuthorFrame == sMonotonicFrameCount` なら return(新 U32 member を LLDrawInfo に追加・複数 pass/bucket で同一 rec を二重 author しない)。
   - `computeDrawDataSlots(&rec, mdiBatchTextures(pass), slots)` → `rec.ensureVkDrawDataSlot(slots)` → 成功時 `mdiAuthorAndCheck(&rec, slots, rec.mVkDrawDataSlot, bt)`(α stamp も凍結帯へ同伴 = VKC は main 上のまま・検出器コードの変更ゼロ)。
   - **palette prewarm(A4)**: `rec.mAvatar && rec.mSkinInfo` なら `rec.mAvatar->updateSkinInfoMatrixPalette(rec.mSkinInfo)`(内部 cache キー付き = 同一 pair の再呼びは cache hit)。
- 可視性で絞らない(author は record 内容の関数で pass/可視性に非依存・dedup が費用を抑える)。初版で計測し、必要なら union vis bits 絞りを**設計者判断で**後付け(実装者は絞りを勝手に入れない)。
- 実行点: `generateSunShadow` の凍結帯末尾(MV 経路 = :15229 の LLRTScope 直前 / 非 MV = record ループ直前)。**gCubeSnapshot 再入でも同経路を通ること**(:14773 の 2-cascade 形)を確認する。

### A3: fire 縮退(record 帯を純 read へ)
- `pushIndirectBucket` の :1490-1497(computeDrawDataSlots/ensureVkDrawDataSlot/mdiAuthorAndCheck)を削除。残す = `rec->mVkDrawDataSlot` の読みと `mdiSetFirstInstance`。slot 無効(`0xFFFFFFFFu`)cell は現行どおり fire しない側へ(空 cell compaction の既存構造を壊さない)。
- `rebuildTemplateIfDirty`(:1424)は**呼びを残す**が、A2 実施後は mTplDirty=false で純 read no-op が正常(record 中に dirty = freeze 漏れ = B6 検出器の対象)。
- 非 MDI 経路(`establishPerDrawId` :569-606・pushBatch 系)は**変更しない**: A2 の pre-author により `ensureVkDrawDataSlot` は memcmp no-op(= 純 read)に自然縮退する。record 中に**新規確保**が起きたら freeze 漏れ(B6)。
- `checkPerDrawIDFreshnessAtFire`(β 検証)は不変。

### A4: palette prewarm = A2 に内包(上記)。avatar pool(`renderGeomShadow` 駆動)経由の palette は Stage 0 では lazy のまま(直列 = 正しさ不変・被覆拡大は Stage 1 #29 walk 後)。

## B3 工事 B: 計器正規化(§6.2・機械的・C/D と同 commit 可)
- delta idiom 3 site(lldrawpool.cpp:1351-1352/:1880-1881・pipeline.cpp:13962-13963): `pushIndirectBucket` が(rec 増分, dyn 増分)をローカルで数えて返す形に変え、呼び手が `shamdi[cell][k] += local` を atomic 加算。`r0/d0 = load()` の時間差分を全廃。
- 既知の素 ++ 2 件: `gPipeline.mTextureMatrixOps`(lldrawpool.cpp:2054)→ atomic 化 or TL 化。`s_rt_resume_count`(llvkloader.cpp:13889 帯)→ `static std::atomic<U32>`。
- 機械 sweep(実装者実行・結果を完了報告に添付): `grep -n "load() - [rd]0" indra/newview/*.cpp indra/llrender/*.cpp` および record 経路上の singleton member `++`。**camera 側 site(matcen 等)も同 idiom なら同時に直す**(同一 commit・機械的置換の範囲でのみ)。
- 判定: 改修前後の同一シーン直列走行で shamdi/mdi 計器の恒等(delta idiom は直列では正しい = 値が変わったら実装ミス)。

## B4 工事 C: indirect ring 並列安全化(§6.3)
- `sIndirectRingCursor`(llvkloader.cpp:408)→ `std::atomic<U32>`・`indirectRingAlloc`(:12771-12784)の frame reset + 非 atomic bump を「相前 reset + fetch_add」へ。
- buffer lazy 生成(:12742-12769)と frame reset を新 `tickIndirectRing()` に移し、`beginCommandRecording` 帯(:5663 の隣)から呼ぶ。record 中の `indirectRingAlloc` は「確保済み前提の fetch_add + 範囲チェック」だけになる(overflow 時の既存 false 返し + 呼び手 fallback は不変)。
- 生成失敗時の `destroyBufferVk`(:12763)も tick(main・相前)に移る = #19 の record 到達 1 本が消滅。

## B5 工事 D: per-shader pipeline map mutex(§6.4)
- `LLGLSLShader` に `std::mutex mVkPipelineCacheMutex` を追加し、`mVkPipelineCache` の**全 accessor** = find(:4173-4174)・insert(:4208/:4465/:4469)・iterate+clear(:356-363)を lock_guard で包む。
- memo(TL sVkPipeMemo* llglslshader.cpp:85-87)は無変更(lock 外の高速経路のまま)。`compileGraphicsPipeline` 呼び自体は lock 外に出して良い(VkPipelineCache は driver 内部同期)— ただし二重 compile 許容の代償に map insert 時に既存 entry があれば後着を `destroyPipelineVk` へ回す(main-only 制約に注意 = **Stage 0 は直列なので二重 compile は起こらない。lock 範囲は「find→miss なら compile→insert」全体を包む素朴形で良い**。分割最適化は Stage 2 で必要になってから)。

## B6 検出器追加(**憲法 4 = AYA 承認対象・承認まで実装しない・独立 commit**)
承認を求める新設シグナル(いずれも fail-closed・通常走行で沈黙が正常):
1. `C_RECORD_PHASE_ACQUIRE`: record 帯 flag 中に `drawDataAcquireSlot` が**新規確保**を行った(= freeze 漏れの名指し)。flag は generateSunShadow の record 帯を囲む純検出用(挙動分岐に使わない)。
2. `C_RECORD_PHASE_TPLDIRTY`: record 帯中に `rebuildTemplateIfDirty` が dirty を検出した。
3. INV-P2 assert: record 帯中の heap publish / descriptor write(実装点は publish 経路側・Stage 2 の R 型群と統合可)。
4. `vbStageCopyVk` の worker silent-false → 発火型昇格(Stage 0 では main のみ = 実質休眠・Stage 2 で効く)。
- 承認が下りない場合: Stage 0 の gate は既存検出器 + 計器恒等のみで実施(B6 は Stage 2 の R 型一括承認に合流)。

## B7 gate 手順(直列・fork なし)
1. build(-j20)+ 3-path deploy(`cp --remove-destination -p`)。
2. 診断起動(`AYASTORM_VKC=1 AYASTORM_PERF_LOG=5`)で影あり通常シーン走行(AYA 起動・cold launch は AYA 領分)。
3. PASS 条件(fail-closed・全て機械):
   - 警報全欄ゼロ(default-deny・`docs/alarm_allowlist.md` 以外全ブロック)。
   - `vb_inflight_hostwrite` 0(A1 の in-pass rebuild 消滅の裏取り)。
   - shamdi 4+4 cell 発火継続・mdi_full=0・供給検証器(α/β)沈黙・shamdi/mdi 計器が改修前走行と同形(B3 恒等)。
   - devlost 0。
   - (B6 承認時)新設検出器 全沈黙。
4. AYA 視覚 gate(最終のみ)。
- 実装順・commit 粒度: **commit 1 = B3+B4+B5(機械的地盤)→ commit 2 = B2 工事 A(構造)**。各 commit ごとに build+走行。A が崩れたら commit 1 とは独立に切り戻せる。

## B8 縮小・省略・解釈申告
- A2 は可視性で絞らない初版(費用は dedup 頼み)= 計測後に設計者が絞りを判断。
- avatar pool 経由 palette・pool 内部(#29)・非 MDI establishPerDrawId の in-record ensure 呼び自体の撤去 = **Stage 0 ではやらない**(no-op 化で INV-P1 実質達成・完全撤去は Stage 2 の assert 網の下で)。
- `renderGeomShadow` の pool loop・`renderShadowAlphaMultiview` の record 呼び順 = 一切変更しない(順序意味論不変)。
- B6 は AYA 承認待ちの独立項(不承認でも Stage 0 は成立)。
- LLDrawInfo への新 member 1 個(`mVkAuthorFrame` U32)= 資源モデル docs への追記は gate 後。

## B10 追補設計: 正準 flavor 不変条件(既存欠陥根治・AYA GO 2026-08-11)
**発見経緯**: Stage 0 gate 走行の mdi_stale/overwrite 嵐 → site 帰属(asite/rsite)で機構確定。真因 = **既存実装の flavor ping-pong**: `computeDrawDataSlots` の batch_textures が「bucket 正準(freeze/template/rebuild = `mdiBatchTextures(pass)`)」と「呼び手依存(establish 系 = caller bt)」の 2 系統あり、同一 record の persistent slot を毎 frame 交互に ensure → **毎 frame acquire/release の ping-pong churn**(旧 per-fire author が警報を隠していた・GPU 正しさは acquire-on-change で両時代とも保持)。
**破れている不変条件**: 「record の persistent slot は唯一の正準内容を持つ」。
**改修形(INV-C)**: persistent slot の書き手を正準系(freeze/rebuild/template fire = bucket bt)に限定。establish 系は:
1. slot 未確定(INVALID)→ 従来どおり ensure(first-claim = その record の正準として確立)
2. 計算内容 == 保存内容(`mVkDrawDataSlots` memcmp 一致)→ record slot を使用(同 hash author = 無害)
3. **内容不一致 → record slot に触れず `drawDataWriteScratch(計算内容)`**(scratch = per-frame ring・author 済で返る経路に載せる)
- 適用点 **1 箇所**: `establishPerDrawId` の ensure 呼びのみ。正準系(freeze・rebuild ensureRecordDrawDataSlot・fire self-author・**rigged walk の slow path**)は無変更 — **実装自己監査で訂正**: rigged walk はその record 群(非 bucket = freeze 圏外)の**正準 author そのもの**であり、scratch 化すると `refreshed` fast path(field id)と skin base publish の id 対応が破れる(fast path frame の skin base 未書込 = 実ギャップ)。非正準側 = camera rigged の establish は本則 3 分岐で scratch へ逃げるため、rigged の ping-pong も establish 側の 1 箇所で閉じる。
- 配当: 嵐消滅 + **既存の隠れ churn 根絶**(対象 record × 毎 frame 2 確保 2 解放の浪費が消える)。
- 事前確認済: `writeDrawSkinBase` は id < DRAWDATA_TOTAL_SLOTS 全域受容(:8921)= scratch id 可。scratch 失敗(=0)は既存の id 0 経路と同義。scratch 增分 ≈ 非正準 flavor draw 数/frame(untextured dyn ≈ 数百)≪ ring 32768・overflow は既存 `C_DRAWDATA_SCRATCH_WRAP` fail-closed。

### B10-次 = 根治工事の起票(AYA 合意 2026-08-11): DrawData layout 統一
INV-C の 3 分岐は**封じ込め(暫定)**。真の根 = 「consumer の draw モード(batch_textures)がデータ内容を変える」= flavor 概念そのもの。根治 = **正準 layout 統一**: [0..3]=テクスチャリスト([0] は両モード共通の diffuse)・[13]=normal・[14]=specular(現在未使用域)に固定し、material 系 shader の読み先を [0]/[13]/[14] へ差し替え。効果 = flavor 消滅・`computeDrawDataSlots` の bt 引数消滅・本 3 分岐と scratch 逃がしの撤去・不変条件がデータモデルに内在化(コードで守る必要が消える)。コスト = GLSL 読み先変更 + vulkanize 鍵 bump + 消費者全数 audit(中規模)。位置づけ = 「just-in-time 供給を所有データ+明示 dirty へ」という本キャンペーンの中核病理(per-fire author 覆い・rigged fast path・flavor の 3 例で確立)の族根治の 1 号。実施時期 = Stage 0 gate PASS + commit 後の独立工事(AYA 裁定)。

### B10 設計自己監査(実装前)
- **[risk 1・申告]** rigged shadow walk の `refreshed` fast path は field slot(正準 flavor)を使う。walk flavor が正準と不一致の frame では fast path の供給 flavor が draw 期待と異なり得る — ただし①rigged 影 walk の caller bt=false = rigged 系 record の first-claim(影が frame 先頭)と一致するのが常態 ②旧実装は「最後に ensure した flavor」というより不定な last-writer-wins だった = 本設計で悪化しない ③flavor 差は slots[1..3] の意味のみで影 alpha が読む [0]/[10] は両 flavor 同値。宣言して受容。
- **[risk 2・申告]** 非 bucket record の正準 = first-claim(最初に establish した flavor)。以後、別 flavor の draw は恒久 scratch。claim flavor が「死んだ」場合(その pass が無効化)slot が旧 flavor のまま残る = 無害(参照する draw がない)。
- **[risk 3・確認済]** 起動直後 freeze 前に claim された bucket record は freeze が一度だけ flip(acquire/release 1 回)→ 以後安定 = 過渡 1 回のみ。
- **[検証]** gate で churn 消滅を機械確認可能: mdi_stale/overwrite = 0(V1/V2 が正準不変条件の検証器としてそのまま機能)。
- **[憲法 4]** 検出器 file 無変更。

## B9 自己監査(敵対的・2026-08-10 実施)
- **[監査で掴んだ実装リスク 1(要遵守)]** A1-2 で cull を renderShadow の外へ出すと、`renderShadow` 内の `LLGLDepthTest`/`LLGLEnable` 等の GL state RAII と cull の相対順が変わる。cull/stateSort は GL state に依存しない(scene graph 走査)ことをトレース確認済みだが、実装者は移設 diff で **state RAII を凍結帯へ連れて行かない**こと(record 帯に残す)。
- **[実装リスク 2]** A3 で :1490-1497 を消すと `slots[]` 計算も消える → `mdiSetFirstInstance` の引数は `rec->mVkDrawDataSlot` のみで完結することを確認済(:1495)。ただし **`rec == nullptr` cell(テンプレに record 無し)** の既存分岐を消さないこと。
- **[実装リスク 3]** A2 の `forEachSource` は bucket 保持の source 全数を歩く = `rec.mVkAuthorFrame` dedup が無いと同一 rec を pass 数ぶん author し、`ensureVkDrawDataSlot` の memcmp no-op で正しさは保つが α stamp が重複する。dedup を**先に**入れてから forEachSource を回す実装順を守る。
- **[設計判断の根拠開示]** 「author は可視性非依存」= `computeDrawDataSlots` の入力が DrawInfo member + texture slot のみ(:517-563)で camera/pass 由来値を含まないことによる。将来 pass 依存値を DrawData に足す変更は本前提を破る → その時は freeze walk の pass 内 author へ戻すこと(この制約を feasibility doc §6.1 に恒久記載済とする)。
- **[gap(残・Stage 1 送り)]** pool renderShadow 内部の author/palette 全列挙(#29)/ establishPerDrawId の scratch fallback(ensure 失敗時 `drawDataWriteScratch`)は record 中に残る = PART 安全(#5)につき Stage 0 では容認。
- **[hidden なし宣言]** 本 Brief に「書かずに落とした」項目はない(落とした項目は全て B8 に記載)。
- **[未強制点]** B6 不承認の場合「record 中新規確保ゼロ」の機械証明は Stage 0 では得られない(計器恒等と警報ゼロによる間接 gate のみ)= その場合の PASS は「A 工事の正しさ」でなく「退行なし」の証明に留まることを報告で明示する。
