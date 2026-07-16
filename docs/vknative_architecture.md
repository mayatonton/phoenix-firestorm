# AYAstorm VK-native 全体設計書

- 状態: **DRAFT v1(AYA レビュー待ち・approve まで実装凍結)**
- 起草: 2026-07-16 設計実装者
- 対象 branch: `feature/ayastorm-r42-phase2` 以降(実装フェーズで専用 branch を切る)
- 上位原理: **毎フレーム同じ仕事をやり直さない**(分散でなく削除)。GL 追従 = 無設計状態の脱却。
- 目標: RTX 5090 crowd シーンで GL を明確に超える。30fps は floor であって goal ではない。

---

## 0. 現状診断(なぜ遅いか — 一文で)

**静的な世界に対して、CPU が毎フレーム「可視判定 → draw list 構築 → 全 draw の状態解決 → 全 draw のコマンド記録」をゼロからやり直しているから。** フレームのコストが「シーンの変化量」でなく「シーンの総量」に比例している。GL 時代はこの浪費を NVIDIA ドライバ内の並列エンジンが隠していた。Strike 1〜10 / MT-1〜2b はその隠蔽の再発明(負債返済)であり、天井は良くて GL ドライバ同等。

### 0.1 フレーム仕事の全量台帳(実読済・要約)

per-draw(crowd で ×50k/frame・全て main 1 本):

| 仕事 | 実体 | VK-native での姿 |
|---|---|---|
| descriptor set 構築/照合 | buildAndOverride(35 sampler + 28 UBO key)+ cache + memo 3 署名 | **消滅**(bindless: draw は index を持つだけ) |
| matrix 同期 | syncMatrices hash 5 本照合 + ring push | 実トレースで再分類(§1.3 改訂): 静的の model 行列は per-region 共有 1 本で per-draw データは不存在。現行 memo 維持・M6 掃除で縮退 |
| pipeline 照合 | key 30+ field + memo | **消滅**(pipeline ソート済み bucket = 切替は境界のみ) |
| VB bind | setBuffer = attribute 数ぶん vkCmdBindVertexBuffers(4-8 本)+ index bind を毎 draw 無条件再発行(llvertexbuffer.cpp:1633-1648) | **bucket 毎 1 回**(mega-buffer suballocation) |
| texture 解決 | bindFast 状態比較 + capture/notify(llrender.cpp:144-190)+ getLiveVkImageView/Sampler | **消滅**(bindless table 常駐・texunit 機構ごと退役) |
| draw 発行 | vkCmdDrawIndexed + push constant | 静的 bucket は **multi-draw indirect に集約**・動的のみ個別 |

per-frame(×1/frame):

| 仕事 | 実体 | VK-native での姿 |
|---|---|---|
| octree cull | camera 毎(main 1 + shadow 4 + spot 2 + probe)に全 region × ~20 partition を CPU 走査(pipeline.cpp:3036- / llspatialpartition.cpp:1048-) | 静的可視性の保持化 → 後段 GPU culling |
| **render map 再構築** | postSort が毎フレーム全 visible group の mDrawMap を歩き全 LLDrawInfo を LLCullResult へ push(pipeline.cpp:4542-4620)= 静的シーンでも draw list をゼロから作り直す | **消滅**(永続 bucket・dirty 時のみ patch) |
| stateSort 距離/LOD 更新 | 全 visible group/drawable(pipeline.cpp:4044-) | camera/object 移動時のみ(イベント駆動) |
| occlusion query 機構 | per-group cube query 発行 + readback 状態機械(pipeline.cpp:3179-) | **機構ごと消滅**(GPU HiZ culling へ) |
| rebuild(updateGeom/rebuildPriorityGroups) | dirty 駆動 + 時間予算制 = 既に「更新」の形 | 温存。worker へ移す主戦場 |
| lighting 合成 + local light ループ | fullscreen 数発 + per-light cube draw ≤256(pipeline.cpp:11333-12171)。CPU 側は薄い | 温存(後述 §2.4)。22.7% の主体は後段 alpha = per-draw chain と同族 = 柱 1-3 の配当で受ける |

### 0.2 設計の不変条件(invariant)

1. **フレームの CPU コストは変化量に比例する。** 静的な世界は「変化ゼロ = 記録コストゼロ」。
2. **「bind」という概念を per-draw 経路から消す。** texture / VB / descriptor / pipeline の全てについて、draw が運ぶのは index のみ。
3. **CPU は draw を数え直さない。** 可視性の決定は保持され、最終的に GPU が数える(indirect)。
4. **順序が意味を持つ pass(alpha)だけが順序コストを払う。** 不透明系に順序保証のコストを課さない。
5. **工事中は kill switch・決着したら即削除。** 段の実装〜gate の間だけ退化モードへ即時 fallback できる形で工事し(big-bang 禁止)、gate PASS 後に switch ごと刈る(§5 運用)。

---

## 1. リソースモデル

### 1.1 Bindless global texture table(柱 1)

- **単一の global descriptor heap**: `set = HEAP`(番号は §4.2)に `texture2D[]`(variable count・partiallyBound・updateAfterBind)+ `sampler[]`(数十個の固定 sampler パレット = 既存 sSamplerCache の全種)。cube/3D は少数につき別 binding の配列(probe 群・noise 等)。
- **slot 管理**: `LLImageGL` に `mHeapSlot`(U32)を持たせる。VkImageView 生成時に free-list から acquire・破棄時は **frame fence 経過後に recycle**(in-flight 3 frame ぶんの遅延解放 = 既存 deferred destroy queue と同じ寿命規律)。ABA は世代タグ(slot 上位 bit)で遮断。
- **streaming との接続**: texture decode → upload → mip 完成の各段で slot の **中身(imageView)を差し替えるだけ**。descriptor set の再構築・invalidation・memo は一切発生しない。Strike 10(memo/署名/世代)の存在理由が消える。
- **draw 側**: per-draw データ(§1.3)が texture index 群(diffuse/normal/spec/emissive…)を持つ。shader は `nonuniformEXT` で heap を引く。
- **追い風**: indexed batch shader 群は既に「sampler 配列 + per-vertex index」で描いている(mIndexedTextureChannels 経路)= これは bindless の局所形。global heap への一般化は自然拡張であって新規発明ではない。
- **前提工事**: device 生成(llvkloader.cpp:1326-1483)に `VkPhysicalDeviceVulkan12Features` chain を追加 — `descriptorIndexing` / `runtimeDescriptorArray` / `descriptorBindingPartiallyBound` / `descriptorBindingSampledImageUpdateAfterBind` / `descriptorBindingUpdateUnusedWhilePending` / `shaderSampledImageArrayNonUniformIndexing`。heap 上限は `maxDescriptorSetUpdateAfterBindSampledImages` から決定(desktop 目標 64k・下限 fallback §6)。

### 1.2 Mega vertex/index buffer(柱 2)— M3 で実装済(2026-07-16 gate PASS)

- **実装形**: SoA chunked pool(llvkloader.cpp `megabuf*` 族・pool key = 頂点 typemask)。chunk = 1 VkBuffer 内に attribute 別の連続 region(容量 64K 頂点 → ×2 成長 → 上限 1M。index chunk = 1MB→16MB)。**slice = 頂点単位の範囲 [first, first+count)(4 頂点 align)を全 attribute region で共有** = `vkCmdDrawIndexed(vertexOffset=first, firstIndex=slice基点+offset)` で描く。LLVertexBuffer は slice handle(MegaSliceV/I)のみ保有。
- **bind は chunk 切替時のみ**: bind 引数が slice 非依存(region base 固定)のため bindVertexBufferVk/bindIndexBufferVk の thread_local (cmd,frame) memo が per-draw re-bind を吸収(実測 vbbind 97.8% skip)。
- **書込 = transient staging**: CPU 副本は map〜unmap 間だけ生存(pool 5s age-out)。**不変条件: map で flag した region は同一 map pass 内に全書きする**(書けない buffer は `setStagingPersistent(true)` を宣言 = 現在 llmodelpreview の preview 島のみ。違反すると staging のゴミが flush される)。
- **解放規律**: slice free は frame fence 遅延(`tickMegaFreeQueue` = sLastCompletedMonotonic 準拠)+ 隣接 merge。断片化 compaction と空 chunk 縮退は未実装(M6 の worker job 候補)。pool は main thread 前提・lock なし(worker 化する段で同期を入れる)。
- **10 年配当**: mega-buffer 上の suballocation は将来の ray tracing BLAS 構築・mesh shader 化の前提形でもある。

### 1.3 Per-draw SSBO(柱 3)— 2026-07-16 実トレースで改訂

- **transform は DrawData に持たない(改訂)**。実トレース(llvovolume.cpp:5654-5677)により per-draw transform の実態が確定: 静的 world = **per-region の共有行列 1 本**(頂点は rebuild 時に region 空間へ焼き込み済み)/ rigged = null(skin palette が担う)/ 動的 = per-drawable(毎フレーム動く物 = 保持不能)。よって「per-draw の transform データ」は静的世界に存在せず、SSBO 化は誰も救わない。modelview は現行機構(per-region PC + memo)を維持し、M4 bucket は (pass, pipeline, region) で切ることで per-record transform なしで MDI と両立する。
- **DrawData の実体 = `{ uvec4 tex_slots }`(+後続段で material)**。永続 SSBO・slot は LLDrawInfo が保有(lazy 取得・dtor で fence 遅延解放)・**slot immutable**(内容変更 = 新 slot 発行 + 旧 slot fence 遅延解放 = heap slot と同一の寿命規律。in-flight frame との race を構造的に排除)。
- **draw-ID の運搬**: `vkCmdDrawIndexed(firstInstance = slot)` → VS `gl_InstanceIndex` → flat varying → FS が SSBO[draw_id] を引く。追加 feature 不要・MoltenVK 互換・MDI でも 1 record 1 slot で自然に成立。M1 の binding54(dynamic UBO 運搬)はこの段で退役(kill switch fallback として温存)。
- LLDrawInfo を経ない draw(populate 経路)は per-frame scratch 領域(SSBO 末尾を frame slot 3 分割)から一時 slot を取る。

### 1.4 整合性(fence の事実のみ)

寿命・recycle は全て「fence が通過した」事実で駆動する(カウント推測禁止 = 既存 doctrine)。heap slot / mega-buffer slice / DrawData index の 3 つの free-list は同一の frame-retire 通知点(beginFrame の fence 待ち直後)から回す。

---

## 2. フレームグラフ / draw stream

### 2.1 永続 bucket(pipeline ソート済み draw stream)

- **bucket = (pass, pipeline, region, mega-buffer pool) をキーとする永続配列**(region を含めるのは per-record transform を不要にするため = §1.3 改訂)。要素 = `DrawRecord { firstIndex, indexCount, vertexOffset, drawID }`(= VkDrawIndexedIndirectCommand と同形に置く)。
- **構築はイベント駆動(M4a 実装済)**: patch 点 = mDrawMap を書く者と同一(不変条件「bucket は mDrawMap の写像」)= `LLVolumeGeometryManager::rebuildGeom` / 基底 `rebuildGeom` の決着点で group 単位に差し替え・evict = `clearDrawMap` 1 hook(rebuild 開始・removeObject・群死・destroyGLState を全被覆)。実装 = `llvkbucket.h/.cpp`(key=(pass, region)・Range = group 毎の `LLPointer<LLDrawInfo>` 配列・slot 再利用)。毎フレームの render map 再構築(postSort 8b)は bucketized pass について **消滅**(実測 VkPerf `bkt rpush=0`)。
- **可視性は record を消さない(M4a 実装済)**: 可視 flag は **LLCullResult 付属の per-cull bitset**(postSort が occlusion/surface-area filter 通過 group の bit を立てる)。group 側 `mVisible` stamp は probe(cube snapshot)が CAMERA_WORLD の ID を流用するため可視 flag に使えない(実トレースで確定した罠)。後段で GPU culling(§2.3)へ委譲。
- **描画 emission**: pass 内で bucket を回し、bucket 境界でのみ pipeline bind + VB bind → `vkCmdDrawIndexedIndirect(count = bucket size)`。multiDrawIndirect 未使用の中間段では CPU loop の vkCmdDrawIndexed でも同じ bucket 構造で動く(移行を段にできる根拠)。M4a の emission = `LLVKBucket::forEachSource`(供給源選択ヘルパ・各 site のループ本体は 1 つ)。
- **対象 pass(第一波 = `LLVKBucket::kBucketizedPasses` に一本化)**: MT-2b worker 対象と同一の 13 types(simple/fullbright/shiny/bump/fullbright shiny + materials 非 MASK 8 種)。**terrain/tree は render map 非経由**(face pool・stateSort が毎フレーム enqueue = pipeline.cpp:4286)につき対象外 → M6 の pool loop 骨格置換で再判定(先送り台帳)。GLTF_PBR・*_MASK 系・GRASS・GLOW = 第二波候補。**rigged・alpha・水は従来 emission**(柱 1-3 の per-draw 費削減は全部乗る)。HUD は bucket 経路に乗る(HUD cull の bitset で分離)。動的(active drawable)の 13-pass record は bucket に同居(emission chain 同一・static/dynamic の分離は M5 の MDI 化で record flag により行う)。

### 2.2 alpha(順序必須)の扱い

- alpha は今後も CPU ソート + 順序 emission(不変条件 4)。ただし bindless + DrawData 化で per-draw 費は「pipeline 切替(稀)+ vkCmdDrawIndexed + draw-ID」まで落ちる。
- 深度ソートは現行の group 単位ソート(postSort)を維持。record 化はしない(毎フレーム順序が変わるものに永続構造は張らない)。

### 2.3 GPU culling(柱 3 の後段)

- compute pass が DrawData の AABB(rebuild 時に併記)× camera frustum × HiZ で indirect buffer の instanceCount を 0/1 に書く。**occlusion query 機構(per-group cube 描画 + readback 状態機械)は機構ごと退役**。
- `drawIndirectCount` は MoltenVK に無い前提で設計: count は固定(bucket size)・不可視は instanceCount=0 の空 draw(GPU 上で実質無料)。
- octree は退役しない: streaming/LOD/イベント駆動の空間索引として残る(役割が「毎フレーム可視判定」から「変化の索引」に変わる)。

### 2.4 lighting・合成・pass 間 barrier

- deferred 合成鎖(sun/blur/soften/finalize)は GPU-bound で CPU 費は薄い(実読済)= 構造温存。local light ループ(per-light cube draw ≤256)は light 配列 SSBO + 1 回の clustered/fullscreen 化を **後段候補**に置く(規模が小さく優先度低)。
- **barrier の宣言的管理**: pass 定義(名前・read RT 群・write RT 群)の静的表から遷移 barrier を導出する薄い frame graph を導入。現行の手動 layout 簿記(setVkDepthLayout 先付け・WAW barrier 手打ち = MT-2b で露呈した壊れやすさ)を機構化で置換。RT 群は固定的(deferredScreen/screen/shadow/probe/水系)なので full 汎用 render graph は要らない — **表駆動で足りる**。wasWrittenThisFrame() 系の不変条件(保持型 RT は当該フレーム書込時のみ合成可)はこの表の属性として吸収。

---

## 3. スレッドモデルの再定義

**原理: 記録を保持するなら、MT の主戦場は「記録」でなく「更新」へ移る。**

| 資産 | 去就 | 理由 |
|---|---|---|
| MT-1 PresentEngine(swapchain list・queue 単独所有) | **温存(恒久)** | submit/present 境界はどの設計でも必要。detachable floater の扉(swapchain list)も既に合意済み |
| MT-2a thread_local/atomic 基盤(gGL・ring・arena・pipe cache mutex) | **温存(恒久)** | worker がどの仕事をするにせよ record-safe 基盤は前提 |
| MT-2b shadow cascade record worker(lane/pin/seed) | **縮退実施済(M4b 2026-07-16)** | shadow static の bucket 化で「worker で記録し直す」対象が消滅 = record 関数・pin・seed・mt_split 分岐を物理削除。lane 機構(RecordJob/CommandPool per lane・record-safe guard 群)は休眠温存 = M6 で更新 job の器に転用 |
| MT-3(scene 提出並列化・凍結中) | **不実施** | 「毎フレーム全記録」を並べ直す工事 = doctrine 違反。bucket 化がその仕事を消す |
| Strike 10 memo/署名/世代・per-draw descriptor cache | **bindless 到達で退役** | per-draw set という概念ごと消えるため |
| Strike 1(PC 化)/5(vkCmd memo)/6(cascade cull)/7(shadow RR)/8(probe slice)/9 | **温存** | draw 数・GPU 仕事の削減 = 設計と直交して有効 |

worker pool の新しい仕事(優先順): ① geometry rebuild(genVolumeGeometry/genDrawInfo — 現在 main の updateGeom 予算内)② mega-buffer への書込・compaction ③ bucket patch の構築 ④ texture upload 前処理(padding copy 等・one-shot 乱発の解消と併せて)。**kill switch は既存 `AYASTORM_MT_THREADS` の意味論を維持**(=1 で全 inline)。

---

## 4. Shader 基盤

### 4.1 言語・reflection

- 既存資産を継承: SPIR-V reflection(attribute mask / sampler→tex / bind 識別 = r42 B-1〜B-3)が唯一の真実源のまま。
- bindless 消費 shader に `#extension GL_EXT_nonuniform_qualifier : require` を追加。段階移行のため **heap 版と従来版を同一 GLSL から生成**(`#ifdef AYA_BINDLESS`)し、runtime は device caps + kill switch で選ぶ。
- indexed batch shader(sampler 配列 + vertex index)= bindless の先行形。第一波の書換対象にして知見を確立し、他 family へ横展開する。

### 4.2 Descriptor set layout(統一)

| set | 内容 | 更新頻度 |
|---|---|---|
| 0 | per-frame: camera/environment/shadow/lighting UBO 群 + shadow map/probe/noise 等の固定 texture | frame 1 回 |
| 1 | **global heap**(texture2D[] variable count + sampler パレット + cube/3D 配列) | updateAfterBind(slot 差替のみ) |
| 2 | per-pass override(spot projector・water 等の少数例外) | pass 毎 |
| PC | draw-ID 等 128B 以内(既存 PC 表 llvkuboreg.h を継承) | draw 毎 |

- **pipeline layout を全 scene shader で共通化** → set 0/1 の bind は frame/pass に 1 回。per-draw の vkCmdBindDescriptorSets が消える。
- 現行 universal per-draw set(~50 binding)は移行期間中並走し、最後に退役。
- shared ring UBO(windlight 系)は set 0 へ移す = 先送り台帳の Strike 10 v3(key diet)の正式解がこれ(v3 単独工事はしない)。

### 4.3 Permutation 戦略

- 第一波では現行 shader 群の個数を維持(permutation 爆発の火遊びをしない)。
- bucket 数削減(= pipeline 切替削減)が実測で必要になった段で、spec constant による統合を検討(判断は bucket 統計 = VkPerf 拡張で取る)。

---

## 5. 移行戦略(最重要・strangler 方式)

**大原則: 段ごとに AYA gate・動く viewer を一度も壊さない。big-bang 全取っ替え禁止。**

**kill switch 運用(AYA 決定 2026-07-16 改訂)**: kill switch は「実装〜gate の切り分け用足場」であり恒久 fallback ではない。**当該段の gate PASS 後、次段に入る前に switch と退化経路を削除する**(= 一番安全に捨てられる日に捨てる。恒久二経路・dead fallback の堆積を作らない)。switch は旧コード併存でなく新機構の退化モード(パラメータ退化)として実装し、分岐点を最小にする。M0-M2 の switch 3 本(`AYASTORM_VK12` / `AYASTORM_BINDLESS` / `AYASTORM_DRAWDATA`)と binding54 fallback 経路はこの決定により撤去済み。

| 段 | 内容 | 消える仕事 | kill switch / gate 見所 |
|---|---|---|---|
| **M0** | device 前提工事: Vulkan12Features chain(descriptorIndexing 系)+ multiDrawIndirect 要求(**全て optional 検出・未対応でも従来動作**)。caps 公開のみ・消費者なし | なし(無風段) | 起動可否そのもの。3 OS の caps ログ採取 |
| **M1** | global texture heap 新設 + **indexed batch shader family を heap 消費に切替**(diffuse 系 index を DrawData でなくまず既存 per-vertex index のまま heap 化) | 当該 family の per-draw set 構築・Strike 10 memo | switch 撤去済。誤 texture・白置換・streaming 中の slot 差替 |
| **M2** | per-draw SSBO(**tex_slots のみ** = §1.3 改訂)+ draw-ID(firstInstance→gl_InstanceIndex)。binding54 退役 | per-draw の slots arena 書込/dynamic offset(M1 運搬)・MDI への per-record 供給路を確立 | switch・binding54 とも撤去済。誤テクスチャ・batch 単位の模様混線 |
| **M3** | mega-buffer suballocation + mapped 直書き(CPU 副本解消)。strider read 消費者の洗い出しが前提調査 | per-draw VB bind ループ・VB 二重持ち RAM | switch 撤去済(gate PASS 2026-07-16・実測 vbbind 97.8% skip)。geometry 化け・rebuild 競合 |
| **M4** | 永続 bucket(静的不透明 + shadow static)+ dirty patch 配線。emission は CPU loop のまま。**M4a(camera 側)+ M4b(shadow 側 + MT-2b static split 退役)= 2026-07-16 gate PASS**(switch 撤去済) | **render map 再構築(8b)**・pool loop の当該 pass 分・pipeline per-draw 照合・MT-2b shadow record worker | switch 撤去済(gate PASS 2026-07-16・実測 bkt rpush=0 全 cull)。物の出現/消滅遅れ(dirty 配線漏れ)・LOD 切替 |
| **M5** | multi-draw indirect + GPU frustum/HiZ culling(compute) | vkCmdDrawIndexed ×N(静的分)・occlusion query 機構・octree cull の毎フレーム可視判定 | `AYASTORM_INDIRECT=0`。物陰の物体・水面下 cull・probe |
| **M6** | frame graph 表駆動 barrier + worker の更新 job 化(rebuild/upload/compaction)+ 旧経路の物理削除 | 手動 layout 簿記・MT-2b record worker(転用) | 段別。最後に旧経路削除の等価全数照合(GL 削除時と同じ「全数照合」規律) |

- **順序の根拠**: M1→M2→M3 は互いに独立に近く、どれも「per-draw 費の削除」= 即 fps 配当がある。M4 以降は M1-M3 が済んでいるほど patch が単純(record が小さい)。
- **各段の検収**: 視覚同一(AYA gate)+ validation 0 + VkPerf 拡張(段ごとに消えるはずの counter がゼロになることを機械確認 — 例: M2 後 syncmat build=0、M4 後 render map push=0)。
- **rigged/alpha/水/HUD/GLTF は全段で従来 emission のまま**(M1-M3 の配当だけ受ける)。ここに手を出すのは M6 以降の別議題。
- 実装は直列(1 段ずつ)= 既存 doctrine どおり。段の中でも commit は AYA gate 後。
- **各段の運び(AYA 合意 2026-07-16)**: 段の直前に just-in-time 詳細設計(触る file・データ構造・罠・gate 基準)を AYA へ提示 → 実装 → 完了報告。詳細設計と完了報告には**「縮小・省略・解釈申告」欄を必須**とする(空でも「なし」明記。仕様が黙る点の解釈・安い方に寄せた箇所・やらなかったこと一覧。product に見える分岐は実装前に相談)。

### 5.1 cvar 撤去規約(AYA 決定 2026-07-16)

- **判定基準 = GUI 到達性**: ①正規 GUI(設定パネル/メニュー/floater のウィジェット)に配線されている ②正規 GUI の cvar 変更から連鎖(listener/handler)で書き換えられる — のどちらかを満たす cvar のみ「残す」。Debug Settings エディタからの到達は数えない。
- どちらでもない cvar(dead・コードのみ書き手/読み手)は **cvar と分岐と settings.xml エントリごと撤去**。コードだけが読む値は定数/メンバへ畳む。
- **理由**: 読み手(担当)の読速と視界のほうが cvar 1 個の温存より価値が高い。cvar 1 個が分岐を作り、全読者に恒久コストを課す(AYA 原文趣旨)。
- **運用**: 各 M 段の旧経路削除時に、その経路が読む cvar を本基準で判定してから削除形を確定する。残す cvar は VK-native 側へ配線(孤児化させない = 既存規律)。撤去は settings.xml deploy 先同期・XUI 参照ゼロ確認とセット。
- **事前の全数生死表は作らない**(AYA 2026-07-16): grep 製の disposition は信用できない(動的名連結・widget 名経由・UI framework 属性など単純 grep で追えない生存経路が実証済 = docs/specs/ayastorm-r42-cvar-specs.md の刈込経緯)。判定は削除のその時に、対象経路を実トレースして行う。

## 6. 3 OS 制約

- **判定原則: 全 feature は起動時検出・未達 device は該当段の kill switch が自動 OFF**(ユーザー環境で落とさない)。
- **MoltenVK(macOS)= Apple silicon 専念(AYA 決定 2026-07-16)**。Intel Mac は対応対象外。
  - descriptorIndexing は Metal argument buffers 経由(MoltenVK 1.2+)。Apple silicon = argument buffer tier 2 で成立見込み。**heap 上限は必ず `maxDescriptorSetUpdateAfterBindSampledImages` から採る**(desktop 目標 64k)。
  - `drawIndirectCount` は **無い前提で設計済**(§2.3: 固定 count + instanceCount=0)。
  - multiDrawIndirect は MoltenVK 対応あり・未対応でも M5 の CPU loop fallback で bucket 構造は同一。
  - secondary command buffer 不使用(既定・MT-1/2 で確立済)。
  - 逃がし地点 = `dev/ayastorm-vk-premt` 系の運用を継続(3os branch の安定化専用ルールは既存合意)。
- **Windows**: desktop driver 前提で M0 の caps はほぼ全 YES 見込み。コンパイル未検証コードを作らない(編集時は 3 OS guard 規律 = 既存)。`Status` 識別子禁止等の既知罠は継続(CLAUDE.md)。
- **Linux**: 主開発環境 = 常時 gate。

## 7. Upstream 追従の境界(AYA 方針確定 2026-07-16)

- **前提認識(AYA)**: upstream に render まわりを改修してくるパワーはもう無い。追従して受けたいのは UI/機能側の追加のみで、**render エンジンの upstream 改修は(あっても GL 前提ゆえ)欲しくない**。
- したがって **render 側は完全自前領域**とし、upstream 互換の維持義務を負わない。「10 年 merge を受け続ける切断面」の設計要件は **UI/機能側の merge を受けやすいこと**に縮小する。
- **切断面 = 「シーンの変化イベント」**(この線は merge 目的でなく、責務分離として今も正しい): scene graph 側 = LLViewerObject / LLDrawable / LLSpatialGroup / LLFace / genDrawInfo・dirty 機構(GEOM_DIRTY/MESH_DIRTY/updateMove)・pass 分類。renderer 側 = bucket compiler(genDrawInfo 出力 → DrawRecord/DrawData 変換)・global heap・mega-buffer・frame graph 表・PresentEngine・worker pool。
- **縫い目 = bucket compiler 1 枚**。upstream の UI 機能追加が新 pass/新 material を伴う場合のみ、この変換層で受ける(都度判断で足りる — 事前に汎用化しない)。
- scene graph 側も必要なら自前改修してよい(upstream 互換の足かせにしない)。ただし改修は VK-native の必要から発するものに限る(無目的な書き換えはしない)。
- **rename しない判断を継承**: gGL/LLImageGL 等の識別子温存(先送り台帳の合意)。bucket 層は新規ファイル(llvkbucket.* 等)に置き、既存 file の diff 面積を最小化する。

## 8. 寿命 10 年の判断基準(拡張の受け皿チェック)

| 将来拡張 | この骨格での受け方 |
|---|---|
| GPU-driven(GPU LOD 選択・GPU occlusion) | DrawData/AABB が既に GPU 常駐(§1.3)・indirect が既に描画形(§2.3)= compute を足すだけ |
| mesh shaders | mega-buffer suballocation + bucket 単位 dispatch へ差し替え可(draw record の形が meshlet 化と同型) |
| ray query(影・反射) | mega-buffer slice から BLAS 構築が自然・shading は bindless heap が前提条件を既に満たす |
| async compute | frame graph 表(§2.4)に queue 属性を足す拡張で受ける |
| detachable floater(r43) | PresentEngine swapchain list(済)+ RecordContext が描画先を明示的に運ぶ規約(済)|
| upstream の新機能(新 pass/新 material) | bucket compiler 1 枚(§7) |

## 9. 成功指標(検収可能な形)

- **一次指標**: crowd 実シーン(draws/f ≈ 50k)で **静的不透明分の per-frame CPU 記録費 ≈ 0**(VkPerf で bucketized pass の CPU emission 時間と record 数を直接計測)。
- **二次指標**: 同シーン fps が GL 実績(「余裕で 30fps」)を明確に超えること。中間段(M1-M3)でも per-draw 費(ms/k-draw)の段階的低下を VkPerf で確認。
- **regression 指標**: validation 0 維持・視覚同一(AYA gate)・`AYASTORM_MT_THREADS=1` + 各段 kill switch OFF での完全直列動作維持。

## 10. product 分岐の決定記録

1. **最低対応 GPU の線引き**(AYA 決定 2026-07-16): 旧経路(per-draw descriptor 経路)は移行中の kill switch(工事用足場)としてのみ並走し、**M6 で物理削除・descriptorIndexing(Vulkan 1.2 世代・目安 2015 年以降の GPU)を最低動作要件とする**。恒久二経路メンテはしない。それ以前の GPU のユーザーは upstream(OpenGL 版 Firestorm)を使えばよい。
2. **Intel Mac**: 対応対象外・**Apple silicon 専念**(AYA 決定 2026-07-16)。
3. **upstream 追従**: render 側は完全自前領域・追従対象は UI/機能側のみ(AYA 決定 2026-07-16・§7)。
4. 上記以外(層構成・移行順・機構選定)は設計者決裁事項として本書で確定する。
