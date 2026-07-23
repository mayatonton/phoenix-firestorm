# 戦略2B — rigged draw の indirect 化(skin bindless 化)JIT 詳細設計

> 位置づけ: per-draw 描画記録 回復フェーズの**本丸(E系本丸「drw」)**。作業計画 = `docs/vknative_perdraw_record_recovery_design.md` §戦略2(B)。統治 = `docs/vknative_recovery_plan.md`。診断真実源 = memory `finding_crowd_bottleneck_vsync_busywait_not_cpu` / 引き継ぎ = `handoff_perdraw_record_recovery`。
> **状態(2026-07-24 更新)= ✅ B.0/B.1/B.2 commit 済 `39c00b7d6b` / B.4-a 掃除 commit 済(HEAD 系)。**
> - B.2 = skin palette UBO→SSBO 移行を GPU 悉皆 A/B オラクルで **65億頂点 byte 一致を実証**、AYA 裁定で決着(詳細/難航教訓 = memory `handoff_b2_skin_ssbo_migration`)。
> - 🎯 **A/B オラクルは退役(commit `a9b4cdecbf`)= crowd device-lost の真犯人だった**: objectSkinV の A/B watcher が rigged 全頂点で UBO 側 skinning を二重計算 → dense crowd で 1 フレーム 243ms→GPU TDR。`AYASTORM_SKIN_AB=0` で 243→35ms・device-lost 消滅・27-34fps 安定と切り分け確定 → オラクル恒久退役。付随で **skin base race 根治**(base buffer ×FRAMES_IN_FLIGHT ring 化 + binding3 dynamic offset)。引き継ぎ = memory `handoff_devlost_root_ab_oracle_and_b3_readiness`。
> - 🔜 **B.3(rigged→MDI 畳込)= 足場のみ commit 済**(`pushRiggedBatchesIndirect` in lldrawpool.cpp・SIMPLE/FULLBRIGHT_RIGGED 限定・`AYASTORM_RIGGED_MDI=1` opt-in・既定 OFF・未 gate)。**🧭 READINESS 実測 = 現スコープは fps 配当極小(rigged e3 ≈ 3.4ms/f ≈ 10%・支配項は shadow 662k records = scene の 59%)→ 深掘りは低配当ゆえ棚上げ(AYA 裁定 2026-07-24)。**
> - ⚠️ **実装教訓: shader 変更のたび `shader_cache`+`pipeline_cache.bin` を必クリア / objectSkinV は共有 utility で per-shader define が届かず `attribs`(sGlobalDefines・llviewershadermgr.cpp:1240)へ注入。**

## 0. トレース済みの現状機構(HEAD `89644e06c8`・全て file:line で確認)

### 0.1 draw の 2 経路(static=MDI / dynamic=per-draw)
`llvkbucket.cpp:367-396` の `rebuildTemplateIfDirty` が各 DrawInfo を is_static 判定で振り分ける:
- **is_static 条件**(`llvkbucket.cpp:373-380`)= VB 有効 + count>0 + region_matrix 一致 + texture_matrix なし + **`info->mAvatar.isNull()`(:378)** + VK slice 有効。
- static → `s_statics` → `mTplCommands`(`VkDrawIndexedIndirectCommand`)へ畳み、`lldrawpool.cpp:1287-1295` の **`vkCmdDrawIndexedIndirect`**(chunk span 単位・1 call で N draw)。**`firstInstance = info->mVkDrawDataSlot`**(`llvkbucket.cpp:442`)で bindless DrawData を参照。
- **rigged(`mAvatar` 非 null)は :378 で必ず static 落ち → `mTplDyn`**(`llvkbucket.cpp:392`)→ `lldrawpool.cpp:1382-1396` の **per-draw `pushBatch`**。**これが 15k draw/frame の per-draw 記録の主要部**。

### 0.2 rigged の per-draw 記録コスト(`lldrawpool.cpp:1399-1432` `pushRiggedBatches`)
renderMap を舐めて 1 draw ごとに:
1. **`uploadMatrixPalette(avatar, skinInfo, ...)`**(`lldrawpool.cpp:1417` / 定義 1685-1724)= (avatar,meshHash) で dedup し、変化時に `writeObjectSkinUBO`(palette を shadow へ)→ `objectSkinStoreCache`。
2. **`pushBatch`**(`lldrawpool.cpp:1555-1619`)= `bindFast` texture・`buildAndOverrideScenePerDrawSet`(:1600)・`setBuffer`・`drawRange`(= `vkCmdDrawIndexed` 1 発)。

### 0.3 skin palette の bind 機構 = **per-draw dynamic UBO(これが indirect を阻む核心)**
- palette 実体 = `sObjectSkinShadow`(`llvkloader.cpp:7250`・型 `ObjectSkin_PerProgramBind`)。**サイズ = 10560 B**(`llvkloader.h:559` static_assert・`mat3x4[110] × 2` = matrixPalette + lastMatrixPalette〔motion blur 用〕・`MAX_JOINTS_PER_MESH_OBJECT=110`)。
- `writeObjectSkinUBO` が per-avatar palette を shadow に書く → `ensureObjectSkinUploaded`(`llvkloader.cpp:7310`)が `allocPerDrawUBOSlice` で **rotating slice に memcpy**・(buf,off) を返す → **descriptor に binding 46 の dynamic offset として per-draw bind**。
- shader 読み口 = `objectSkinV.glsl:31-46` = **`layout(set=1, binding=46, std140) uniform ObjectSkin_PerProgramBind { mat3x4 matrixPalette[110]; mat3x4 lastMatrixPalette[110]; }`**。`getObjectSkinnedTransform()`(:52-94)が `matrixPalette[i]` を読む。
- **∴ 各 rigged draw は「自 avatar の palette を指す dynamic UBO offset」を要する = 1 本の indirect に畳めない**。これが `mAvatar.isNull()` 除外の根本理由。

### 0.4 bindless DrawData の既存機構(static が使う・rigged 化で流用)
- DrawData = per-slot **`uvec4`(4×U32・16B)**。`drawDataAcquireSlot`(`llvkloader.cpp:11163`)が mega DrawData buffer の slot に 4 U32 を書く。static の中身 = **texture heap slot 4 本**(`llvkbucket.cpp:310-329` `computeRecordSlots`)。
- shader 読み口 = `materialF.glsl:306` `layout(set=2, binding=0, std430) readonly buffer AyaDrawDataBlock { uvec4 aya_tex_slots[]; }`・`aya_draw_id = gl_InstanceIndex`(`materialV.glsl:144`)で index(instanceCount=1 なので `gl_InstanceIndex == firstInstance == slot`)。
- **決定的事実**: `materialV.glsl` は **`HAS_SKIN` 時に既に `aya_draw_id = gl_InstanceIndex`(:144)を設定した上で `getObjectSkinnedTransform()`(:147)を呼ぶ** = **rigged 頂点 shader は既に draw id を保持**。skin base index を引く土台が既にある。

### 0.5 規模の実測(既存ログ 03:13・後追い mine)
- rigged draw/frame ≈ 400-480(`rigged_rec` cumulative から)。**unique (avatar,mesh) palette ≈ 45/frame**(`skin_up ≈ 5600/130frame`)。
- ∴ mega skin palette buffer = **45 entry × 10560 B ≈ 475 KB/frame**(dedup が効く・小さい)。多数の draw が同一 palette を共有(同一 avatar-mesh の別 material/face)。
- descriptor cache(`ScenePerDrawCache`)は既に **hit 率 99.9%**(`ehit≈530k / ealloc≈136`)= 戦略3 descriptor 再利用は適格母集団ゼロで**取り下げ済**。残コストは populate/bind 経路そのもの = 本 2B の標的。

## 1. 不変条件と核心変換(1 文)

> **不変条件**: 「rigged draw は自 avatar の palette を per-draw dynamic UBO offset で bind せねばならない」。
> **変換**: palette を **「per-draw dynamic UBO(binding 46)」→「全 avatar palette を 1 本に積む bindless SSBO + per-draw base index(`aya_skin_base[gl_InstanceIndex]`)」** に移し、per-draw の palette bind を消す。これで rigged draw が per-draw dynamic UBO offset の束縛を失い、**rigged 専用 collapse 経路**(§B.3・static の region 行列前提には合致しない)で `vkCmdDrawIndexedIndirect` に畳める。**発行畳み込みのみ・描く物・palette 値は byte 同一**。

## 2. Phase 分解(直列・各 Phase 単独 gate)

### B.0 — skin bindless 基盤(SSBO 2 本 + 充填 API)【C++・shader なし】
- 新 mega SSBO 2 本を bindless set(set=2 = `AyaDrawData` と同居 or 新 binding):
  1. **`AyaSkinPaletteBlock`**(std430・`mat3x4` 相当を `vec4` 配列で・per-entry 220 個 = matrixPalette 110 + lastMatrixPalette 110)。per-frame ring(FRAMES_IN_FLIGHT)。
  2. **`AyaSkinBaseBlock`**(`uint aya_skin_base[]`・DrawData slot と同 index 空間 = `gl_InstanceIndex` で引ける)。
- 充填 API: `writeObjectSkinUBO` の書込先を shadow → **mega palette buffer の per-(avatar,mesh) 予約 entry** に変更(既存 `objectSkinTryAdopt`/`StoreCache` の dedup をそのまま entry index cache に転用)。返り値 = **skin base entry index**。
- この Phase は旧 UBO 経路と**併存**(shader はまだ UBO を読む)= 挙動不変・gate = 新 buffer が正しく充填される計器(entry 数 == skin_up)。

### B.1 — DrawData に skin base を載せる【C++】
- rigged draw の DrawData slot 取得(現 `ensureRecordDrawDataSlot` は static のみ)を rigged にも発行し、`aya_skin_base[slot] = skinBaseEntry` を書く。
- **決定事項(§4-a)**: DrawData `uvec4` は texture 4 本で満杯 → skin base は **並列 buffer `AyaSkinBaseBlock`** に分離(DrawData stride を変えない・侵襲最小)。
- gate = rigged slot が正しい base を保持する計器(まだ描画経路は旧のまま)。

### B.2 — shader migration(palette 読み口の付け替え)【GLSL・最大リスク】
- `objectSkinV.glsl`: UBO binding 46 → SSBO `AyaSkinPaletteBlock` + `AyaSkinBaseBlock`。`getObjectSkinnedTransform()` = `uint base = aya_skin_base[gl_InstanceIndex]; matrixPalette[base*220 + i]`(mat3x4 復元は vec4×3 read)。`lastMatrixPalette` = `base*220 + 110 + i`。
- **objectSkinV.glsl の全 includer を列挙**(deferred material / alpha / shadow / 各 rigged pass)。各 includer の main() が `gl_InstanceIndex` 経路(= indirect 描画)で来ることを確認。**旧 per-draw 経路(非 indirect)で来る includer が残る場合は暫定的に両対応が要る**(§4-b)。
- gate = **L3 型 GEOAB A/B オラクル**(新 SSBO 経路 vs 旧 UBO 経路の頂点出力 byte 照合・`verdict=src|kernel`)+ validation 0 + 視覚同一。

### B.3 — bucket collapse(rigged を MDI へ)【C++】
> ⚠️ **設計訂正(2026-07-24・実トレースで確定)**: 当初案「`llvkbucket.cpp:378` の `info->mAvatar.isNull()` 除外を撤去 → static collapse に相乗り」は**誤り**。static collapse の is_static 条件(:373-380)は `mModelMatrix == region_matrix`(全 draw が region 行列共有)を前提とするが、**rigged は model 行列を使わない**(`materialV.glsl:147-149` = `mat = getObjectSkinnedTransform(); mat = modelview_matrix * mat` = skinning=world 空間 bone 行列が位置を持ち global modelview を一度適用するのみ・`mModelMatrix` 未設定)。∴ rigged は static の region 行列前提に合致せず。**rigged 専用の collapse 経路を新設する**(VkBuffer/index/pipeline で束ね・global modelview 一度・skin base 毎フレーム更新)。
- **3 部構成**(引き継ぎ = memory `handoff_b3_rigged_mdi`): ①毎フレーム軽量更新パス(rigged DrawInfo を舐め `uploadMatrixPalette`〔palette 充填〕+ `writeDrawSkinBase`〔slot に base 書込〕**のみ**・描画なし)②rigged collapse(rigged draw を (VkBuffer,index,index type,pipeline) で chunk span に束ね `VkDrawIndexedIndirectCommand` 構築・static の :410-448 が雛形・`firstInstance=mVkDrawDataSlot`)③描画(`pushRiggedBatches` を global modelview 一度 push → chunk span 毎に `vkCmdDrawIndexedIndirect` に置換)。
- chunk span 分割は VB buffer/index buffer/index type/pipeline で切れる = rigged の VB 実体は mega-buffer slice 相乗り確認済(`megabufAcquireVertex`・§4-c)。
- **cull/instanceCount**: rigged の per-draw cull を MDI の per-command `instanceCount=0`(`lldrawpool.cpp:1343-1356` 既存機構)へ移植。
- **順序制約**: alpha rigged は blend 順序保持が要る → opaque/mask 系(SIMPLE/FULLBRIGHT/*_MASK_RIGGED)から着手・alpha は最後。
- gate = rigged が MDI 経路で描画・draws/f の dyn 減・視覚同一。⚠️ **A/B オラクルは退役済**(検証装置なし)= 視覚 + validation が gate。

### B.4 — 掃除 + 計測
- **✅ B.4-a(掃除・commit 済 HEAD 系)= 挙動不変の dead-code / 診断 scaffold 除去**: A/B オラクル buffer 一式(SKIN_AB_ORACLE 定数・sSkinAB* buffer・descriptor **binding4**〔layout/bindingCount/pool を 5→4・3→2〕・alloc/destroy・perf `sk_ab_mism` 欄)/ dead per-shader hook `add_skin_bindless_permutations`(5 call sites・live 注入は global `attribs["AYA_SKIN_SSBO"]` 一本に集約)/ 診断 log(llshadermgr.cpp「B.2 SHADER PATH」)。触る file = `llvkloader.cpp`・`llviewershadermgr.cpp`・`llshadermgr.cpp`。
- **🔜 B.4-b(Stage3・棚上げ・掃除ではない機能変更)= velocity SSBO 化 → UBO fallback(binding46 matrixPalette)削除**: velocity/blur rigged pass(`pushRiggedVelocityBatches`・`allow_dedup=false` で `objectSkinStoreCache` 未呼び=SSBO 未充填 → base INVALID → UBO fallback 依存)を SSBO 化してから binding46 を削除する。**⚠️ UBO fallback は velocity/shadow/impostor/occlusion rigged が実際に使う load-bearing な安全網**・A/B オラクル退役後は byte-exact 検証装置なし・B.3 棚上げなら配当ゼロ → **恒久機構として残置**(AYA 裁定 2026-07-24)。
- 計測 = VkPerf `rig`/`skin_up`/`sk_bl`/`sk_base`・fps(軽い crowd で微増・重い会場で main 飽和防止が本命)。**記録がまだ律速なら戦略2C(secondary cmd buffer per-core)へ**。

## 3. 触る file(確定分)
- `indra/newview/llvkbucket.cpp`(:378 除外撤去 / :310-336 slot 発行を rigged 拡張 / collapse)
- `indra/llrender/llvkloader.cpp`(:7250-7344 skin 書込先 / :11163 DrawData / 新 SSBO 確保・充填)+ `llvkloader.h`(struct・API 宣言)
- `indra/newview/lldrawpool.cpp`(:1399-1499 pushRiggedBatches 群 / :1382-1396 mTplDyn / :1555-1619 pushBatch から rigged 分離)
- `indra/newview/app_settings/shaders/class1/avatar/objectSkinV.glsl`(palette 読み口)+ **全 includer(B.2 で列挙)**
- descriptor set layout(binding 46 UBO の去就・set=2 への SSBO 追加)= 該当 layout 定義(要 grep 確定)

## 4. 罠・未決事項(AYA 判断 or Phase 内トレースで潰す)
- **(a) DrawData 拡張 vs 並列 buffer**: 並列 `AyaSkinBaseBlock` を推奨(stride 不変・侵襲小)。DrawData を uvec8 に拡張は全 static 経路に波及=非推奨。→ **設計内で確定(並列)**。
- **(b) objectSkinV includer の非 indirect 経路**: 一部 rigged pass(shadow/alpha)が indirect 化前で per-draw のまま残ると shader が `gl_InstanceIndex` を持たない → B.2/B.3 を pass 単位で足並み揃える必要。**includer 全列挙が B.2 の最初のタスク**。
- **(c) rigged VB の mega-buffer 相乗り**: rigged mesh の VB が per-avatar 別 buffer だと chunk span が細切れ = MDI 利得が薄い。avatar mesh VB の slice 実体を B.3 着手時に確認(mega-buffer 化されていれば理想)。
- **(d) motion blur last palette**: `lastMatrixPalette` も同 entry に積む(前 frame palette)= entry サイズ 2 倍。frame 跨ぎの last の扱い(ring 世代)を B.0 で確定。
- **(e) MAX_JOINTS=110 超の mesh**: clamp 済(`objectSkinV.glsl:59`)= entry 固定 110 で不変。
- **(f) std430 の mat3x4 レイアウト**: UBO std140 と SSBO std430 で padding 差。palette を vec4 平坦配列で持ち手動 index(byte gate で担保)。

## 5. gate 基準(共通)
- **⚠️ A/B オラクルは退役済(commit `a9b4cdecbf`・device-lost 真犯人)**。B.2 の byte 照合(65億頂点一致)で役目を終え撤去。**B.3 以降の gate は視覚同一 + validation 0**(経路移行の byte 検証装置は無い)。B.3 を本気で進めるなら、二重計算でない軽量な検証装置(例: CPU 側の palette entry 照合)の再設計が前提。
- validation 0 / 診断起動で全層オラクル沈黙 / 視覚同一(最終 gate のみ)。
- 品質トレード禁止(発行畳み込み = lossless・LOD/cull による間引きではない)。
- gate 様式 = 命題 + 未証明項併記 + 実効設定確認欄(SSR/spot 等)。

## B.0 JIT 詳細設計(実装直前・全 file:line 再トレース済・HEAD `89644e06c8`)

**スコープ = skin bindless 基盤の敷設のみ。旧 dynamic UBO 経路と完全併存・shader 不変・描画経路不変 = 挙動 byte 同一。** 新 buffer に palette を並列充填し、充填の正しさを計器で確認するだけ。

### B.0-1 新 SSBO 2 本を bindless heap set(set=2)へ追加【`llvkloader.cpp` `createBindlessHeap` :2646-2758】
- `bindings[]` を 2→4 に拡張: binding 2 = `STORAGE_BUFFER`(skin palette・stage=VERTEX)、binding 3 = `STORAGE_BUFFER`(skin base・stage=VERTEX)。`bind_flags`/`ps`/`bindingCount`/`poolSizeCount` を 4 へ。`ps` に STORAGE_BUFFER descriptorCount +2。
- DrawData buffer 作成(:2722-2752)の直後に 2 buffer を `createBufferVkImpl(..., STORAGE_BUFFER_BIT, ..., mapped=true)` で作成し binding 2/3 に `vkUpdateDescriptorSets`(**1 回・永続 set**)。
  - palette buf size = `SKIN_ENTRIES_PER_FRAME(=1024) * 10560 * FRAMES_IN_FLIGHT(3)` ≈ **32.4 MB**(tunable・§4-g)。
  - skin base buf size = `DRAWDATA_TOTAL_SLOTS(1048576) * 4` = **4 MB**(DrawData slot と同 index 空間・B.0 では 0 clear のみ・充填は B.1)。
- teardown(:2595 付近)に 2 buffer の `destroyBufferVk` を追加。

### B.0-2 per-frame region ring + 充填 API【`llvkloader.cpp` skin セクション :7250 付近】
- 新 static: `sSkinPaletteBuffer/Mapped`・`sSkinBaseBuffer/Mapped`・`std::atomic<U32> sSkinPaletteCursor[FRAMES_IN_FLIGHT]`。
- `skinBindlessBeginFrame()`(既存 frame 開始点 = `beginFrame`/`sFrameIndex` 更新箇所に相乗り)で `sSkinPaletteCursor[f]=0`。region base = `f * SKIN_ENTRIES_PER_FRAME`。
- `U32 skinBindlessStorePalette(const void* shadow_10560)`: cursor を CAS で 1 進め、`entry = region_base + local`。`entry >= region_base+PER_FRAME` なら `BINDLESS_INVALID_SLOT`(overflow= B.0 では計器のみ・旧経路が描く)。`memcpy(sSkinPaletteMapped + entry*10560, shadow, 10560)`。return entry。
  - **byte 根拠**: shadow = std140 `mat3x4[110]×2`・48B stride = std430 と同一ゆえ SSBO で `mat3x4 palette[]` として直読可(B.2)。

### B.0-3 並列充填の配線【`llvkloader.cpp` `objectSkinStoreCache` :7435 / frame cache】
- `ObjectSkinFrameCacheVal` に `U32 entry` を追加。`objectSkinTryAdopt`(:7404)命中時は cache の `entry` を採用(再充填しない=dedup)。miss 時 `objectSkinStoreCache`(:7435)で `skinBindlessStorePalette(&sObjectSkinShadow)` を呼び entry を cache に格納。
  - これで **充填回数 == skin_up**(dedup 一致)を保証。
- 計器: `gVkPerf.skin_bl_fill{0}`(新・`llvkloader.h` perf block :1465 付近に追加)を充填成功で inc、`skin_bl_of{0}` を overflow で inc。VkPerf 行に `sk_bl=fill/of` を 1 欄追加(setb 欄近傍)。**憲法 4**: VkPerf は検出器 = 追加は計器増設(盲目化でない)ゆえ可・diff は AYA gate。

### B.0 gate(命題様式)【実測で oracle 訂正済・2026-07-23】
- **⚠️ 当初前提「fill == skin_up」は誤り(実測で判明・訂正)**: `skin_up`(`llvkloader.cpp:7460`)は `ensureObjectSkinUploaded` の増分だが、これは **case 46 の per-draw descriptor bind 経路(`:7479`)からも呼ばれる**ため per-draw で膨らむ = unique palette 数ではない。`sk_bl fill` は `objectSkinStoreCache`(dedup 後 unique (avatar,mesh))の数ゆえ **fill < skin_up が正常**(実測 ~0.42×・fill≈25/frame)。
- **訂正後の命題**: 「skin palette bindless buffer は、unique (avatar,mesh) palette ごとに 1 回、旧 UBO と **byte 同一データ**で充填され、overflow せず、描画は byte 不変」。
- **positive oracle(構造保証)**: fill の copy source = `&sObjectSkinShadow` = `ensureObjectSkinUploaded` が UBO slice へ memcpy するのと**同一 10560B・同一呼出列** → byte 一致は構造的に保証(同一 source memcpy)。
- 実測(2026-07-23・AYA 視覚 OK): **`of=0`(全報告)**・fill=unique palette 母集団・視覚同一・crash 0。ビルド 0 error。
- **未証明項**: ①重い会場での `of` 率(段階0 後追い・B.0 では無害=旧経路 fallback)②真の byte 一致の runtime 照合は B.2(shader 消費時に GEOAB A/B)まで保留 = 現時点は構造保証のみ。
- **実効設定確認欄**: gate 時 SSR/spot 等 preset 実値を記録(B.0 は描画不変ゆえ影響なし)。
- kill switch: 不要(充填は加算のみ・描画不変)。overflow は自動 no-op。

### ⚠️ B.2/B.3 必須の coverage gap(B.0 で発見・充填漏れ経路)
`allow_dedup=false` 経路 = **velocity/motion-blur pass**(`lldrawpool.cpp:1872` `pushRiggedBatches` 系の blur 版・`:1977` も同型・`self_blur/others_blur` + `uploadLastMatrixPalette`)は `writeObjectSkinUBO` を呼ぶが **`objectSkinStoreCache` を呼ばない = bindless を充填しない**。この pass の rigged 描画は B.2 で bindless palette が無い状態になる。
- **B.1/B.2 対応必須**: これらの pass でも skin entry を frame cache に持たせる(current + **last palette も別 entry で bindless 化**が要る = motion blur は lastMatrixPalette 参照)。
- B.0 では fill を消費しないので無害。

## 6. 縮小・省略・解釈申告(OPEN)
- 本 doc は**設計のみ**(コード未着手)。各 Phase 着手時に該当 file:line を再トレースして JIT 詳細化 → AYA 承認。
- 戦略2C(per-core 記録)は本 doc スコープ外(2B 完了・計測後に律速なら別設計)。
- 重い 50-100人会場での main 飽和は未実測(段階0 常時 PERF_LOG で後追い定量化)。方向(rigged per-draw 記録が支配・draw 数線形)は現データで確定ゆえ着手正当。
- kill switch = 工事中のみ(B.3 の除外撤去に env gate)・gate PASS 後即削除。
