# AYAstorm VK-native — 方向(governance・2026-07-30 全面改訂)

これは統治文書。旧 `vknative_recovery_plan.md` と worker/並列化前提の設計 Doc 群(draw_redesign / serialization_phase_b / *_offmain_* / ui_scene_decouple / rigged_skin_palette_redesign 等)は **2026-07-30 に全削除**した(方向が誤りで山積・行の無駄)。必要なら git 履歴から復元し作り直す。

## 0. 設計バイブル(前段=普遍設計・全設計者が最初に読む・AYA 制定 2026-08-01)
> **これは順序を統べる最上位の設計指針。次の設計者が道に迷わず新設計・実装できるための道標。** 旧ロードマップ(`vknative_bind_redesign.md` 指示書 D→E→F)は**方向は正しい・破壊しない**が、視界が悪かった当時は**順序が逆**だった(「SSBO を全部やれば鎖が解ける」と推論 → 実際は逆)。本節が順序を是正し、各設計が**何のために必要か**を伝搬する。

### 0.1 なぜ(大本命)
crowd で描画の 90% は rigged avatar mesh。それが **透過(alpha)・影(shadow)・鏡(reflection/probe)** で **multi-view 再描画**され、per-draw 記録コスト(GL 比 6x)が **avatar 数 × view 数**で爆発する(北極星 [50-100 体で固まらない] 直撃)。GL はこれを NVIDIA driver の並列エンジンで隠していた。**独自設計 = self-describing draw → GPU-driven multi-view で根絶する**(geometry を 1 回記録し GPU が cascade/reflection 間で replay)。

### 0.2 前段=普遍設計(土台であって finale でない・★最重要の順序是正)
**self-describing per-draw data model = 全 per-draw データが draw-index SSBO に在り、ambient な可変 per-draw state がゼロ**(`bind_redesign.md` §8.7 の鎖リンクを全断)。
- **これが無いと影/鏡の 1-geometry-多-view は原理的に不可能**(GPU replay は CPU が per-draw に UBO を回す/gGL state を積む前提が全消えていないと成立しない)。
- ∴ **「鎖を解くこと」が本体・pass(material/影/texture)はその payoff**。pass を積んで鎖が解けるのではない。**主語は self-describing 化、pass は解けた鎖から順に落ちてくる**。

### 0.3 大項目(= 切るべき鎖リンク・各々が目的付きの構造工事)
| # | 鎖リンク(self-describing 化する共有可変 state) | 解禁する大本命 | 状態 |
|---|---|---|---|
| 0 | per-draw ID(§8.7 L1・可変 global tCurrentDrawDataID) | 全 bindless draw の土台 | **G★ 済 `7ba3e71184b`** |
| 1 | **material params(L3・rotatePerProgramUBOSlot の per-draw 回転 UBO)= 材質シェーダ族 wholesale** | material MDI + 影/鏡の material path | **暗礁露出済**(§0.4)・**piecemeal 禁** |
| 2 | skin base / palette(rigged の骨データ) | **rigged 影/鏡** | 部分(skin palette は set=3 SSBO 化済・UBO fallback 残) |
| 3 | **view/model 分離 + multiview 化**(3a 本体・不可分)/ static model matrix SSBO(3b・任意の下流) | **rigged/static 両方の GPU-driven replay** | 未(実体は §0.5 で補正) |
| 4 | per-draw descriptor bind state(L2) | per-draw 記録コスト本体 | 進行中(bind redesign) |
→ これらが揃って初めて **大本命 = 影 GPU-driven(1-geometry-多-view)+ 鏡 + alpha per-draw 削減**が解禁。**影 SSBO 化の価値は莫大**だが、上記が全部揃わないと無理。

### 0.4 指針(設計者へ伝搬・「コストが悪いで撤退」を構造的に禁じる不変条件)
数ヶ月、設計者は構造工事を**目先の速さ/コストで測って撤退**してきた(material SSBO も「速くないから無価値」で revert された)。それを止めるための拘束:
1. **鎖を解く構造工事を「速さ/コスト」で価値判断しない。判断軸は「self-describing 土台を前進させ大本命(影 GPU-driven 等)を解禁するか」**。速さは payoff に出る(工事単体では出ない)。**「コストが悪い」は撤退の理由にならない**。
2. **実装は設計欠陥の探針**: 進めて暗礁(うまくいかない)に乗り上げた所が**設計欠陥を露出する = 診断の成功**。暗礁は撤退理由でなく「**露出した欠陥を self-describing 構造で解消せよ**」の指示([[feedback_make_observable_not_reason_to_safety]])。**「やってみたらうまくいかない」は必ずまた来る = 想定内・前進の証**。
3. **順序は鎖リンク駆動(chain-first)・feature 順でない**。各 pass は自分の要求リンクが切れた時点で解禁(§0.5 の source 確定: 影 depth-only は material 不要・要るのは per-draw ID✓ + skin base + **view/model 分離**)。
4. **共有 consumer のリンクは wholesale で切る**(material 族=opaque+alpha+rigged を一度に SSBO へ)。**piecemeal 禁**(material 族を材質 pool だけ SSBO 化 → 共有シェーダ経由で alpha=髪の透過が壊れた暗礁の教訓)。
5. **旧ロードマップ(指示書 D→E→F)は方向一致 = 破壊せず、順序のみ本フレームで是正**。本大項目は実装の探針で further 細分化され得る(想定内)。

### 0.5 source 確定: pass 別 前提リンク + 大項目3 の実体補正(トレース 2026-08-01・HEAD)
> shadow/probe/大項目3 を HEAD source で全経路トレースして確定(scratchpad `shadow_chainlink_trace.md`/`modelmatrix_trace.md`)。§0.3 の枠組みを実体に合わせて補正する。トレース手順 = 憲法7 全枝・grep つまみ食い禁・実読。

**(A) pass 別 前提リンク(GPU-driven 化に何が self-describing でないと無理か)**
| pass | 比率 | 前提鎖リンク | 性質 | 根拠(file:line) |
|---|---|---|---|---|
| shadow(simple/opaque) | (24% の主) | 大項目3(view/model 分離)+ 大項目2(skin base)。**material 不要** | depth-only・**分離可能な最大レバー** | `renderShadow` 主経路(pipeline.cpp:13900 `gDeferredShadowProgram`)→ `renderObjects(type,texture=false)` → `pushUntexturedBatch`(lldrawpool.cpp:1806)/ `pushUntexturedGLTFBatch`(:2274)= `BindlessEstablish::Bare`(:1163 `establishPerDrawId`)= material authoring 不在。crowd rigged body の本体 |
| shadow(alpha-mask) | (24% の残) | 上記 + **material authoring 依存** | 髪/foliage/tree/GLTF-alpha-mask・alpha test 用 diffuse 要 | `renderMaskedObjects(...,texture=true)`(pipeline.cpp:9347)→ `pushMaskBatches`(lldrawpool.cpp:1641)/ `pushRiggedMaskBatches`(:1668)→ **`pushBatch` = `BindlessEstablish::Authored`(:1161 `buildAndOverrideScenePerDrawSet`)**。**⚠️ 旧記述「全経路 Bare」は誤り(session 2026-08-01 一次証拠是正・前任は Bare 経路のみ引用し alpha-mask Authored 経路を見落とした=survivorship blind spot 憲法7)** |
| probe/reflection | 20% | **全部(1 material + 2 + 3 + 4)** | main camera 複製・**分離不可** | updateProbeFace→cubeSnapshot→display_cube_face→**renderGeomDeferred**(llviewerdisplay.cpp:1458)= main と同一 color render |
| alpha | — | 順序 bound | 天井(MDI 不可) | POOL_ALPHA |
→ **「shadow+probe=44% 一括」は誤り**。shadow の **simple/opaque(主・crowd rigged body)は大項目3+2 だけで解禁される clean なレバー**。**alpha-mask 影は material authoring を通る**が、multiview の record×N→×1 は alpha-mask にも効く(per-draw authoring は record 時 1 回・全 layer 有効)= 方向は生きる。**この material-free / material-touching の境界が影 multiview 実装の自然な staging 境界**(第1段=opaque multiview / 別段=alpha-mask)。probe は main render 全体の自己記述化の payoff(material 込み)= 単独で切り出せない。

**(B) 大項目3 の実体補正 = 「model matrix SSBO」→「view/model 分離(3a)」**
現状: `applyModelMatrix`(lldrawpool.cpp:1682)が **view×model を CPU 融合**した 1 枚を、`syncMatrices`→`pushModelviewOnce`→**vkCmdPushConstants(64B vertex PC・dedup)**(llvkloader.cpp:13204)で per-draw に転送。shader は `gl_Position = projection(UBO) × modelview(PC) × pos`(shadowV.glsl:55)。
- **rigged(crowd 90%)は `s.mModelMatrix=nullptr`(llvovolume.cpp:6097)** = SSBO 化する per-draw model matrix が存在しない。world 変換は既に draw-index skin palette SSBO(`aya_skin_palette[base+i]`, objectSkinV.glsl:90)が持つ。modelview PC = view のみ = pass 内定数 = dedup 済。
- ∴ **大項目3 の本体 = view を PC から分離し per-view 選択可能 resource(multiview/`gl_ViewIndex`)へ(3a・不可分・必須)**。これが 1-geometry-多-view replay の真の enabler で rigged/static 両方に効く。
- **static model matrix SSBO(3b)は critical path 上にない任意の下流**: view 分離後、static の model matrix は object→world = view 非依存 = per-draw の model 専用 PC のままで replay 可能。SSBO 化は static ~10% の PC push を消す最適化に過ぎない。**捨てるのでなく格下げ**(理由は速さでなく構造 = 指針1 の cost 撤退でない)。
- **multiview は「壁」でない**: `apiVersion = VK_API_VERSION_1_3`(llvkloader.cpp:1621)+ device<1.3 reject(:1710)= multiview(VK1.1 core)必ず可。実測不要。device 依存は maxMultiviewViewCount のみ = 既存 init の query で解決(cap 超は分割 fallback)。

## 1. 確定した事実(実測・この会話で確定)
- **ガン = CPU の per-draw 記録コスト**。VK なのに GL 比 約 6 倍遅い。lockstep でも draw 数でも GPU 待ちでもない。
- 実測(Safe Hub・vsync OFF/IMMEDIATE・RTX5090):fps≈23.7 / frame≈42ms / **cpu main=95%** / draws/f≈18,304。
  - main の GPU 待ち = **fence 0.4ms・acquire 1.0ms・PE slot 0.0ms** = ほぼゼロ → **CPU⊥GPU の ロックステップは起きていない**。
  - 記録パス `mlp beg≈15.8ms/f` が最大単一項 = **main が 1.8 万 draw を直列記録している事そのもの**が律速。
  - MDI/dedup は効いている(mdi call≈20万・bind の大半 skip)。static は畳み済。
- **残る動的 draw(alphaPost/mat 等 ≈13k scene draw)は per-draw 依存(material/skin/alpha 順序)で畳めない**。

## 2. 並列化 = category error(撤退確定)
- フレームは因果順序が強固な不可分の直列鎖:`cull消費 → 幾何構築 → fill → transform/skeleton resolve → 記録 → submit`。
- off-main worker 群(`recordWorkerCount()>0` gate = geo-fill / avatar-domain build+motion / record pool の camera 使用 / texture off-main)は **この鎖を内部で割った** = 共有・進化中の状態(draw map / VB / transform / spatial group)を worker と main が順序を破って並行に触る。
- 症状 = **avatar + SimRez オブジェクト + 影 が全て「rig がついたように宙を泳ぐ」大崩壊**(特定 skin/palette でなく広域 geometry/transform 破損)。bisect: `AYASTORM_MT_GEO=0`(=off-main 装置無効)→ **完全 clean**。`MT_THREADS=1` → clean。
- **正当な並列は「順序独立を証明できた仕事」のみ = shadow の複数カスケード(独立視錐台)**。それ以外は撤退。
- 例外的に残す off-main = **PE submit/present スレッド**(VK の単一 submitter・鎖の分割ではない・load-bearing)/ **present-wait sleep**(vsync 空回り防止)/ **aux window present**(multiwindow・最小限)。

## 3. 方針(順序)
1. **分散処理を main に戻す**(直列土台を正しい設計として据える)。worker を段階的に物理削除(残すのは §2 の例外のみ)。kill switch で誤魔化さず既定 = 直列。
2. **per-draw 記録を VK 本来の安さにする**(直列のまま)。参照 = `percall_set_authority_map.md` / `vknative_draw_structure_map.md`。
   - **★source+実測で確定(2026-07-30)= 削減軸は「per-draw *単価*」でなく「pass 別 MDI 畳み込み(draw *数*)」**。per-draw 単価は全層 cache 済で分散(heap slot cheap / drawData memcmp / skin lookup / vkCmd)= 単一 hot spot 無し = 天井近い。6x-GL 差はこの分散した VK per-draw モデル overhead の総和 × 18k。
   - **pass 別 MDI 地図(crowd 17.5k draw 実測・`isCameraMdiPass`/`kBucketizedPasses` llvkbucket.cpp)**:
     - simple/fullbright = ✅ MDI 済(消化済)。
     - **material = 6%(1080/f・97% bindless)= bucket 済+bindless で shader 配線済(aya_tex_slots[aya_draw_id])= routing のみ**。小工事・中配当。**最速の次の一手**。
     - **shadow 24% + probe 20% = 未 bucket・独自 render = depth-only/順序自由ゆえ MDI 可だが bucket 新設が要る大工事**(shadow は scene 同一 geometry の別 POV 再描画 = GPU-driven 1-geometry-多-view が真の勝ち筋)。**draw 数の本命レバー(44%)**。
     - alpha = 物理的に順序 bound = MDI 不可 = 天井(alpha にのみ残る)。
3. **カメラパン時のカクつき = この記録天井の frame-time 変動**(present==scene lockstep で画面更新が変動 record 時間に直結)。∴ feel 改善 = pass 別 MDI で draw 数を下げる(上記)+ 将来 present decouple。

## 4. doctrine(不変)
- **並列化の前に分解可能性を検証**(不可分な鎖を割るな)。
- **静的トレースで safety を証明するな・観測可能にせよ**(この会話でも静的判断が 2 度外れ、実測が方向を正した)。
- **品質を下げて速くするのは最適化ではない**(GL 同品質 6 倍差が基準)。
- 憲法(PASS は AYA gate のみ・default-deny・幸運ログ禁止)は不変。
- 実装の有無・正しさは **HEAD のコードを file:line** で判定(Doc/memory の claim は根拠にならない)。

## 4.5 並列化の試みは無駄ではない(残す資産)
worker 化を目指した過程で、**GL 旧来の per-draw 処理が大きく削られ VK-native 構造に置き換わった**。撤退するのは off-main **スレッド化**だけで、以下の VK-native 資産は**残して直列で使う**:
- draw-info snapshot(`LLDrawInfoSnapshot`= 登録の deep-copy・applyGeoStaged で直列使用)/ DrawData / MDI bucket(`llvkbucket`)/ bindless scaffolding(skin palette ring・skinBindlessStorePalette・writeDrawSkinBase)/ mega-buffer / dedup(pipe/desc/push の skip)。
  - ⚠️ **geo-fill snapshot(`LLGeoFaceSnapshot`/`buildVkGeoFill`/`runVkGeoFill`= live face の deep-copy)は worker 入力専用**で直列経路は使わない(直列は旧 `getGeometryVolume` で inline fill)= トレースで到達不能を確認し撤去済(S3)。ここで残すのは draw-info snapshot の方。
- ②per-draw 記録の軽量化は、この既存 bindless/indirect 基盤の上に乗る(ゼロからではない)。

## 5. 北極星
50-100 体 crowd で viewer 自身が固まらない。達成手段 = 並列化でなく **per-draw 記録の軽量化**(§3)。
