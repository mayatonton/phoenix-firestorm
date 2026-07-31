# AYAstorm VK-native — bind 作り直し 未来設計図(骨・proposal)

> 位置づけ: これは**設計の骨(skeleton)**。今見えている視界から「あるべき姿」を置く。後続 Fresh が各設計を深くトレースして肉を載せ、実現可能性を洗う。**実装はその後。AYA 査読・promote 前提の proposal**(governance = `vknative_direction.md`)。
> 起草 session e9b9b351(2026-07-31)。接地トレース = scratchpad `trace_pbr_washout.md` / `trace_head_cleanup.md`。

## 0. この形にした理由(繰り返した失敗からの教訓)
- **大それた一枚岩の設計は何度も失敗した**。worker 並列回帰も数週間かけて「設計欠陥・よく見てなかった」で終わった。∴ **grand design を先に描かない**。
- 身の丈 = **今は「bind ぐちゃぐちゃ」を将来を見据えて作り直す(靴下 washout はその過程で落ちる)→ material SSBO 再挑戦 → shadow 以降 → その先でようやく draw 連鎖の分離・分散が視界に入る**。
- 各段は **source 接地・単独で出荷可能・視覚/oracle で gate**。計測で設計欠陥を直さない(憲法)。
- 北極星 = **ガン = 単一 main thread の per-draw 直列記録**(`vknative_direction.md`)。この設計はそこへ向かう土台づくり。

## 1. 診断 = 今の「bind ぐちゃぐちゃ」(source 事実)
現 descriptor set(HEAD `aae049c4424`):
| set | 内容 | stage |
|---|---|---|
| 0 | per-frame | V/F |
| 1 | per-program(material sampler + per-program UBO=**per-draw dynamic**) | V/F |
| 2 | bindless heap: b0 DrawData / b1 texture heap / **b2 skin palette** | b0 V\|F, b1 F, **b2 V** |
| 3 | skin base(sSkinBaseSet・dynamic) | V |

歪み:
1. **set=2 の相乗り**: fragment の texture heap(b1)と **vertex の skin palette(b2)が同居**。反射 `reflectVkSet1BindingsFromSpirv`(llglslshader.cpp:1294)は **set==2 の変数が 1 つでもあれば** `mVkReflUsesHeapSet=true`(stage/用途を区別しない)→ objectSkinV が set=2 を触るだけで SkinnedPBRAlpha 全体が heap-bindless に**誤分類** → fragment が heap を使わない pbralphaF なのに set=2 heap が pipeline に載る = **washout**(switch A/B で根確定・2026-07-31)。
2. **`mVkUsesBindlessHeap` の溶接**(llglslshader.cpp:2945/3100-3112): 「fragment heap 描画経路」「set=2 layout 組込」「set=3 skin layout 組込」が **1 フラグに束ねられている**。skin set の要否が heap 使用に従属している。
3. **scratch DrawData hack**(llglslshader.cpp:3625/3753 の populate + lldrawpool.cpp:665): per-draw ID(gl_InstanceIndex=firstInstance)の設定が bindless 経路・authored 経路・scratch 上書きで**多重化**し一貫しない。
4. **material params = per-draw dynamic UBO**(`vkResolvePerProgramForDraw`): draw 毎に `allocPerDrawUBOSlice`+`memcpy`+dynamic offset。= (a) draw 毎 CPU 記録コスト (b) **dynamic offset は MDI で畳めない** = GL-dragging。
5. **skin SSBO(B.2)は live だが冗長**: SSBO skinning ≡ UBO(66ae oracle 65 億頂点 mismatch 0)・frame 無益(DrawCount line closed)。B.3 rigged MDI(lldrawpool.cpp:1390 `AYASTORM_RIGGED_MDI`・既定 OFF・shadow 限定・closed)が B.2 に依存。

## 2. 北極星の不変条件(あるべき end-state・方向)
> **全 per-draw データ(tex_slots・material params・skin)は、draw の単一 per-draw ID で GPU buffer を index して shader が直読みする。descriptor set が持つのは per-frame / per-material-static / bindless-heap のみ。per-draw dynamic UBO 書込も per-draw descriptor 再 bind も持たない。**

根拠(AYA の論理・source で裏づけ済): **SSBO/MDI 化できないものは MDI で畳めず、畳めないものは分散に乗らない**(③ material の壁 = dynamic UBO)。∴ per-draw データの SSBO/MDI 化が、MDI・ひいては draw 連鎖の分散の**前提条件**。この end-state は一気に作らず §4 の段階で近づく。

## 3. 目標 descriptor 構成(近接・具体)
作り直し後(段階1 完了時点):
| set | 内容 | 備考 |
|---|---|---|
| 0 | per-frame | 不変 |
| 1 | per-program **static**(sampler・static UBO) | per-draw material params は将来 set 外の per-draw SSBO へ(段階2) |
| 2 | **fragment bindless heap のみ**: b0 DrawData(per-draw index) / b1 texture heap | skin を除去 = washout 解消 |
| 3 | **vertex per-draw skin**: skin base + skin palette | heap set から独立。per-draw-indexed SSBO の **綺麗な最初の実例** |

分類フラグを**溶接から分離**:
- `mVkFragUsesTexHeap`(fragment が set=2 b1 を sampling)→ heap 描画経路 + set=2 組込。
- `mVkUsesSkinSet`(vertex が set=3 を使用)→ set=3 組込 + skin per-draw ID 供給。
- 両者独立。set index gap(skin あり heap なし=SkinnedPBRAlpha)は placeholder で contiguous に(詳細は後続)。

## 4. 段階ロードマップ(身の丈・各段 gate 付き)
- **段階1(今)= bind 作り直し**: skin palette を set=2→set=3(base と同居・vertex 専用)。分類の溶接を解く。skin の per-draw ID 供給を bindless 従属でなく「skin を使う draw なら必ず publish」に一貫化(非 bindless でも firstInstance=skin slot が揃い SSBO が正しく走る)。**washout はこの結果として落ちる**。skin = per-draw-indexed SSBO を別 set で綺麗にやる雛形になる。B.2/B.3 の scratch・welding・RIGGED_MDI の残滓を整理。gate = 視覚(靴下)+ 検出器沈黙 + validation 0。
  - **⚠️ 段階1 = 指示書 A/B/C は「バラバラの 3 設計」でなく内実ひとつの結合変更・同一 build で不可分**(session ffd75280 が §7 肉付けで source 確定)。理由: palette を set=2 から退かした瞬間、現状 skin set を bind している唯一の力(= `mVkUsesBindlessHeap` の set=2 誤 trip = washout の原因そのもの)が消え、B(`mVkUsesSkinSet` 独立反射)無しでは skin が unbind・C(per-draw ID を skin gate で publish)無しでは firstInstance が stale化。**設計の実体 = `mVkUsesBindlessHeap` 1 フラグに溶接された 3 別命題を「分離しながら実装する」こと**(AYA 2026-07-31)。詳細 = §7 A/B/C の詳細設計。
- **段階1.5(今)= per-draw ID の構造化(指示書 G1-G4)**: 段階1(指示書 C)は per-draw ID の**表現**を単一 choke(`commitPerDrawID`)に統一したが、その choke の**呼び出しの網羅を強制していなかった**。commit の caller は 2 箇所(`buildAndOverrideScenePerDrawSet` / scratch)だけで、**skin を読むが commit を迂回する draw-fire 経路**(`pushUntexturedBatch`=影 / object-ID pass)が残った。忘れた draw は可変 global `tCurrentDrawDataID` の残値(別 draw の slot)を継承 → 別 avatar の palette = 歪み影/rig 混線(session ffd75280 のオラクルが機械列挙 + AYA 実機 bisect `SKIN_BINDLESS=0` で確定)。**真の fix = per-draw ID を「draw の属性として fire まで運ぶ」構造にし、可変 global を廃す**。gate = 視覚(crowd で影/選択枠が崩れない)+ `skin_draw_no_commit=0` 全経路 + validation 0。詳細 = §7 指示書 G1-G4 + §8 真のモデル。
- **段階2 = material params → per-draw SSBO**: dynamic UBO を廃し、skin 雛形に倣って per-draw ID で index する GPU buffer へ。draw 毎 memcpy/offset を消す。gate = 視覚同一 + per-draw 記録コスト実測減。
- **段階3 = material MDI**: material が per-draw-indexed になった土台で draw を畳む。gate = draw 数減 + frame time。
- **段階4 = shadow / probe**: 同モデルを shadow(24%)・probe(20%)へ(bucket 大工事・44% 本命)。
- **段階5+ = draw 連鎖の分離・分散**: per-draw が完全に SSBO/MDI 化して初めて、順序不可分だった鎖の分解可能性が再評価できる。ここで初めて「大きな設計」が視界に入る。

## 5. 不変条件・gate(全段共通)
- heap set(set=2)は **fragment が sampling する資源のみ**を含む(vertex per-draw を同居させない)。
- per-draw データは **単一 per-draw ID** で addressing(表現を多重化しない)。
- 各段は **視覚 gate(AYA・最終のみ)+ 検出器沈黙 + validation 0**。PASS は AYA gate のみ(憲法)。
- 「品質を下げて速く」は最適化でない(憲法)。SSBO 化は視覚同一が前提。

### 5.1 検出器(LLVKContract)の扱い = 恐れて避けず fail-closed に強化する(段階1・要 AYA 承認 憲法4)
source 確定(session ffd75280・scratchpad `trace_instr_A_skin_set3.md`):
- **B のフラグ改名/分離は検出器を触らない**。検出器は `mVkUsesBindlessHeap`/`mVkReflUsesHeapSet`/`set2`/`set3` を**名前で一切参照していない**(grep 済)= 改名は検出器 diff ゼロ = 憲法4 の承認不要。
- 検出器は既に **per-draw ID 一致オラクル**を持つ: `stashDrawDataID(id)`(llvkcontract.cpp:738)で期待 id を記録 → `checkDrawDataIDAtFire(actual)`(:744)が draw fire で `firstInstance != expected` を `C_DRAWDATA_ID_MISMATCH` として fail-closed 検出。stash=lldrawpool.cpp:645/llglslshader.cpp:3642/:3771、check=llvertexbuffer.cpp:563/595/634。これは **指示書 C の不変条件(firstInstance 一致)そのもののオラクル**。
- **ギャップ = skin base publish slot の一致は未検出**(現オラクルは DrawData ID のみ・66ae の skin A/B=65億頂点比較は別機構で今は撤去領域)。C が守る失敗 =「skin が別 draw の palette を読む」(set-move 試作で観測した視覚破損)は**現状オラクル網の外**。

**提案(AYA 裁定・診断=fix ドクトリン [[feedback_make_observable_not_reason_to_safety]])**: C の single-choke で id と skin publish が同一変数になる = 構造的に不一致不能。だが「静的トレースで safety を証明するな=fail-closed で観測可能に」に従い、**既存 DrawData ID オラクルに skin slot の一致検証を 1 本足す**:
- skin publish(`writeDrawSkinBase`)時に、publish した draw_id を skin 期待値として stash(既存 `tExpectedDrawDataID` と同一なら追加変数不要=同値 assert のみ)。
- draw fire で `mVkUsesSkinSet` な draw に限り `firstInstance == skin_publish_slot` を検証、違えば新 cause(例 `C_SKIN_BASE_ID_MISMATCH`)を fail-closed 発火。
- これは検出器への **ADD(オラクル追加=盲目化の逆)**。憲法4 で AYA 承認を要するが、**方向は sanctioned**(検出網を狭めず広げる)。段階1 実装ブリーフでこの diff を AYA に提示して承認を得てから入れる。
- 代替(最小)= 新 cause を足さず、C の single-choke で skin publish を `stashDrawDataID` と同一 id に強制し、**既存 `checkDrawDataIDAtFire` にそのまま乗せる**(skin slot=DrawData ID ゆえ既存オラクルが自動で skin 不一致も捕捉)。この場合 **検出器 diff ゼロ**で C の不変条件が既存オラクルで守られる。→ **推奨 = まず代替(最小・diff ゼロ)、不足が観測されたら新 cause 追加**(先回り防御は憲法5 で休眠)。

## 6. 後続 Fresh へ渡す未解決(骨に載せる肉)
- set index gap(skin あり heap なし)の contiguous 化: empty placeholder か非連続 bind か。
- per-draw ID の割当・寿命・scratch 領域の整理(現 `drawDataWriteScratch`/`ensureVkDrawDataSlot`/`mVkDrawDataSlot` の統合)。
- DrawData buffer(現 48B `aae049c4424`)を material params まで拡張するか、別 SSBO を並置するか。
- B.2/B.3 を「clean set へ載せ替え(雛形化)」で残すか、一旦撤去して段階2 で新規に作るか — **本設計は載せ替え(雛形化)を採る**(AYA 合意 2026-07-31)。
- material の per-program UBO block が load-bearing な依存(SSRUtil/AlphaF 等)の棚卸し = 移設の実現可能性。
- 各段の per-draw 記録コスト実測(標的が hot・適格母集団の確認)。

## 7. 設計 指示書(後続 Fresh 向け・1 項目 1 設計)
各 指示書 = 独立した設計タスク。**産出物 = §2 不変条件に沿う具体設計(深トレース・実現可能性・file:line 接地)を本 doc の該当箇所に肉付け**。実装でなく設計。着手前に該当 起点トレース を HEAD ソースで再突合(memory/doc の claim は根拠にしない)。

### 指示書 A(段階1)= skin を独立 vertex set(set=3)へ載せ替え
- **対象**: skin palette(現 set=2 b2)+ skin base(現 set=3 b0)を、fragment heap set から独立した **vertex 専用 skin set** に統合。descriptor layout / pool / set 生成 / buffer write / shader binding 番号 / draw-time bind の設計。
- **起点トレース**: `llvkloader.cpp createBindlessHeap`(set=2 layout + sSkinBaseSet 生成)/ `objectSkinV.glsl`(set=2 b2 palette・set=3 b0 base)/ `getBindlessHeapLayout`。
- **切り口**: skin set を「per-draw-indexed SSBO を別 set で綺麗にやる**雛形**」として設計する(材質が真似できる clean な形)。palette/base を 1 set に。
- **不変条件**: heap set(set=2)に vertex skin を残さない。skinning は視覚同一(SSBO≡UBO)。
- **依存**: 指示書 B・C と整合(分類・per-draw ID)。

#### 詳細設計(肉・session ffd75280 起草・全段 HEAD `aae049c4424` トレース = scratchpad `trace_instr_A_skin_set3.md`)

**現状配線(全 6 段・file:line)**
| 段 | 現状 | file:line |
|---|---|---|
| shader binding | palette=set2 b2 / base=set3 b0(共に `#ifdef AYA_SKIN_SSBO`)/ dynamic UBO fallback=set1 b46 | `objectSkinV.glsl:55,56,32` |
| descriptor layout | set2(`sBindlessHeapLayout`)= b0 DrawData(SSBO,V\|F)/b1 texheap(sampler,F,count,UAB)/**b2 palette(SSBO,V)**。set3(`sSkinBaseLayout`)= b0 base(SSBO_DYNAMIC,V) | `llvkloader.cpp:2835-2867, 2896-2911` |
| pool | palette は `sBindlessHeapPool` ps[0](STORAGE_BUFFER count**2**=DrawData+palette,UAB pool)。base は `sSkinBasePool`(SSBO_DYNAMIC×1,非UAB) | `:2876-2887, 2912-2925` |
| set 生成 | `sBindlessHeapSet`(:2948)/ `sSkinBaseSet`(:2931) | 同上 |
| buffer write | DrawData→heapSet b0(:2977)/ **palette→heapSet b2**(:3015-3019)/ base→skinBaseSet b0 dynamic range=DRAWDATA_TOTAL_SLOTS*4(:3020-3025) | `:2965-3026` |
| pipeline layout | `set_layouts[4]={perFrame,mVkDescriptorSetLayout,heap,skin}`・heap+skin 両方 `mVkUsesBindlessHeap` 従属(溶接) | `llglslshader.cpp:3093-3114` |
| draw bind | set2=heapSet iff `mVkUsesBindlessHeap`(:13074)→ set3=(set2?skinBaseSet:NULL)(:13107)・skin base dynamic offset を末尾 append(:13090)・bind count (set2?4:2) | `llvkloader.cpp:13071-13116` |

**目標 set=3 構成(指示書 A の産物)**
```
set=3 (vertex skin・fragment 非関与):
  b0 = skin base    STORAGE_BUFFER_DYNAMIC, V   ← 現状維持(dynamic offset で frame 切替・:13090 の append 継続)
  b1 = skin palette STORAGE_BUFFER (plain), V   ← set=2 b2 から移設(plain=追加 dynamic offset 無し)
set=2 (fragment heap のみ): b0 DrawData / b1 texheap   ← palette 除去(§3 目標に一致)
```
理由: base=b0 dynamic を維持すれば set=3 の dynamic binding 数が 1 のまま = `skinBaseDynamicOffsetBytes()` append(:13090)が無改修で有効。palette を plain SSBO にすれば dynamic offset 順序(set,binding 昇順)を乱さない。palette は創生時 1 回 write の persistent で UAB 不要 → 非 UAB pool へ移すのは clean 化(現 UAB pool 相乗りの解消)。

**各段の変更点**
- shader: `objectSkinV.glsl:55` を `set=2,binding=2` → `set=3,binding=1`。:56 base は据置。
- layout: `sBindlessHeapLayout` から b2 を削除(bindings[3]→[2]・bindingCount 3→2)。`sSkinBaseLayout` を 2 binding 化(b0=SSBO_DYNAMIC / b1=SSBO plain,V)。
- pool: `sBindlessHeapPool` ps[0] count 2→1。`sSkinBasePool` に STORAGE_BUFFER×1 を追加(SSBO_DYNAMIC×1 + SSBO×1)。
- buffer write: palette write の `dstSet=sBindlessHeapSet,dstBinding=2` → `dstSet=sSkinBaseSet,dstBinding=1`(:3015-3019)。
- pipeline layout / draw bind: **指示書 B の gate 分離が前提**(下記)。set=3 の bind は「vertex が skin を使うか」で決める。

**🔴 load-bearing = 指示書 A は単独出荷不可(A+B+C 同一段で不可分)**
palette を set=2 から退かすと objectSkinV は set=2 を一切参照しなくなる → `reflectVkSet1BindingsFromSpirv`(llglslshader.cpp:1295)が SkinnedPBRAlpha に対し `out_uses_set2=false` → `mVkReflUsesHeapSet=false` → `mVkUsesBindlessHeap=false`(:2945)。効果は 2 つ:
- **(良)** washout 解消 = fragment の pipeline layout に set=2 heap が載らない・bindless 経路に乗らない(= 根治そのもの)。
- **(悪)** draw-time で set2=NULL(:13074)→ set3=NULL(:13107)→ **skin set=3 が bind されない** = SSBO skin 破綻。
∴ **現状 skin set を bind している唯一の力が「set=2 誤 trip」= washout の原因そのもの**。palette を退かした瞬間に skin bind の足場が消える。よって A は **指示書 B(`mVkUsesSkinSet` を vertex skin 使用から独立に立て、pipeline layout 組込と draw bind gate を skin 独立化)+ 指示書 C(publish gate の `mVkUsesBindlessHeap` 依存を skin 独立化・lldrawpool.cpp:606-658)と同一段で不可分**に実装する。→ §4 段階1 が A/B/C を束ねている理由が source で裏づいた。

**set-index gap(A が set=3 content を作る前提・B が用意)**: fix 後 SkinnedPBRAlpha は skin(set=3)使用・heap(set=2)非使用。Vulkan の `pSetLayouts` は 0..N contiguous ゆえ set=2 を飛ばして set=3 は置けない → set=2 に **empty descriptor set layout(0 binding)placeholder** が要る(§6 の「set gap contiguous 化」の具体)。A の set=3 生成と対で B が placeholder を供給。

**申告欄(縮小・省略・解釈)**
- 解釈: base=b0 dynamic 維持・palette=b1 plain を採用(dynamic offset 順序と :13090 の append を無改修に保つため)。逆順(palette b0)は dynamic 順序破りで不採。
- 省略: palette 自体を dynamic 化して frame 切替を base と揃える案は本段では採らない(palette は現状 VK_WHOLE_SIZE 単一 range で足りている・:3011)。将来 material 雛形化(指示書 D)で再評価。
- 未決(後続で潰す): (1) empty placeholder layout の生成主体(llvkloader 側で 1 個生成し getEmptySetLayout() で配る想定・B と協調)(2) `sSkinBasePool` maxSets/descriptorCount の再計算(2 binding 化に伴う)(3) `SKIN_ENTRIES_PER_FRAME`/palette buffer sizing は本移設で不変(確認のみ)。

### 指示書 B(段階1)= 分類フラグの溶接分離 + set-index gap
- **対象**: `mVkUsesBindlessHeap`(現 = heap 経路 + set=2 組込 + set=3 組込 を束ねる)を、**`mVkFragUsesTexHeap`(fragment が set=2 b1 sampling)** と **`mVkUsesSkinSet`(vertex が skin set 使用)** の独立フラグへ分解。skin あり heap なし(SkinnedPBRAlpha)の set-index gap(contiguous 化 = empty placeholder or 分割 bind)の設計。
- **起点トレース**: `llglslshader.cpp:1294`(反射 set==2)/ `:2945`(mVkUsesBindlessHeap 算出)/ `:3100-3112`(set_layouts 溶接)/ `llvkloader.cpp bindDrawDescriptorSetsOnce`(set2 bind)。
- **切り口**: 「fragment が heap を使うか」と「vertex が skin を使うか」は**別命題**。反射を per-stage で分離(1948 の stage loop 活用)。
- **不変条件**: 誤分類が**構造的に起こり得ない**符号化(fail-closed)。pipeline layout と draw bind が常に一致。

#### 詳細設計(肉・session ffd75280・全 shader grep で分離構造を確定)

**中核認識(AYA 2026-07-31): A/B/C は「バラバラの 3 設計」でなく内実ひとつの結合変更。設計の実体 = `mVkUsesBindlessHeap` という 1 フラグに溶接された 3 つの別命題を「分離しながら実装する」こと。** 溶接の中身(llglslshader.cpp:3100-3114 / llvkloader.cpp:13071-13116)= (i) heap 描画経路(set=2 bind)(ii) set=2 pipeline 組込 (iii) set=3 skin pipeline 組込 + skin bind + skin dynamic offset。これを 2 独立軸に割る。

**割った後の set 組合せ = 4 通り(shader grep で全数確定)**
| shader 族 | heap(set=2) | skin(set=3) | pipeline layout |
|---|---|---|---|
| materialF 非rig(`set=2 b0 DrawData`+`b1 texheap`) | ○ | × | set0,1,2 |
| **SkinnedPBRAlpha(靴下)** `pbralphaF`=set2宣言なし・`objectSkinV`=set3のみ | × | ○ | set0,1,**2=empty placeholder**,3 |
| SkinnedMaterial(rig 材質) | ○ | ○ | set0,1,2,3 |
| 無し | × | × | set0,1 |

skin-only の行が **set-index gap**(Vulkan `pSetLayouts` は 0..N contiguous ゆえ set=2 を空 layout で埋める必要)を生む唯一のケース。

**2 独立フラグの反射(起点 = llglslshader.cpp:1295 `reflectVkSet1BindingsFromSpirv`)**
- `mVkUsesHeapSet`(現 `mVkReflUsesHeapSet`=`out_uses_set2` を改名継続): post-A は palette 汚染が消え **DrawData/texheap 宣言者のみ trip**(materialV/F, indexedTextureV)= clean。改修不要(A が汚染源を除く)。
- `mVkUsesSkinSet`(**新規 `out_uses_set3`**): 反射に「desc_set==3 の OpVariable があれば true」を追加(:1295 と同型の 1 行)。現状 set=3 bind は heap に溶接され独立反射が無いので**新設が必須**。objectSkinV のみが trip。
- 両者は `mVkUsesBindlessHeap` を置換。`isBindlessActiveVk()` gate は両方に掛ける(:2945)。

**pipeline layout 組込(起点 = llglslshader.cpp:3093-3114)**
```
set_layouts[0]=perFrame; [1]=mVkDescriptorSetLayout;
[2] = mVkUsesHeapSet ? heap_layout : (mVkUsesSkinSet ? EMPTY_LAYOUT : (打ち切り));
[3] = mVkUsesSkinSet ? skin_layout : (未使用);
set_layout_count = mVkUsesSkinSet ? 4 : (mVkUsesHeapSet ? 3 : 2);
```
= skin を使うなら set=2 が empty でも count=4 まで供給(gap 充填)。heap のみなら count=3。どちらも無しは 2。

**draw-time bind(起点 = llvkloader.cpp:13071-13116)**
- set2 = `mVkUsesHeapSet ? sBindlessHeapSet : (mVkUsesSkinSet ? sEmptySet : NULL)`(empty set も実 descriptor set を 1 個要 = 空 layout から alloc)。
- set3 = `mVkUsesSkinSet ? sSkinBaseSet : NULL`(現 `set2!=NULL` 従属 :13107 を skin フラグ独立に)。
- skin base dynamic offset append(:13090)の gate を `set2!=NULL` → `mVkUsesSkinSet` に。
- bind count = `mVkUsesSkinSet ? 4 : (mVkUsesHeapSet ? 3 : 2)`(:13113 の `(set2!=NULL)?4:2` を 3 分岐に)。
- memo cache に **set3 の独立追跡**を追加(現 :13095-13105 は set2 のみ・set3 は導出前提が崩れる)。

**empty placeholder descriptor set(gap 充填・A と協調)**
- llvkloader に `sEmptySetLayout`(0 binding)+ `sEmptySet`(そこから alloc)を 1 個生成し `getEmptySetLayout()`/`getEmptySet()` で配る。createBindlessHeap 近傍で生成。
- 代替案 = 非連続 bind(vkCmdBindDescriptorSets firstSet=3 で set=3 だけ後追い bind)。ただし pipeline layout 自体は set=2 slot を要するため layout 側は placeholder 不可避 → **empty layout 採用**(bind は firstSet=0 一括のまま empty set を挟む)。

**fail-closed 不変条件(誤分類の構造的排除)**
- 反射直後に assert: `mVkUsesSkinSet` ⇔ SPIR-V に set=3 宣言あり / `mVkUsesHeapSet` ⇔ set=2 宣言あり。pipeline layout の set_layout_count と draw bind の count が**同一式**(上記)から導出される = 常に一致(現状は set2 経由の間接従属で乖離余地があった)。
- 「fragment が heap を使うか」と「vertex が skin を使うか」が**別変数**になったこと自体が washout 級 co-classification の再発を構造的に不能化。

**申告欄**
- 解釈: フラグ名は handoff の `mVkFragUsesTexHeap` でなく `mVkUsesHeapSet` を採用(set=2 は b0 DrawData が V|F でもあり「fragment 限定」でない・実体は「set=2 を触るか」)。DrawData と texheap を別 binding だが同 set=2 ゆえ 1 フラグで足りる(両者とも heap set 常駐)。
- 省略: DrawData(set=2 b0)を skin-only shader が将来触る場合(per-draw ID 統合=指示書 C)、skin-only でも set=2 が要る=gap が消える可能性。C の設計次第で placeholder が不要化しうる → C と併せて最終判断(現段は placeholder 前提で安全側)。
- 未決: `sEmptySet` の pool(maxSets=1・別 pool か既存流用)/ empty layout を UPDATE_AFTER_BIND にしない(0 binding ゆえ無関係)確認。

#### 🔴 `mVkUsesBindlessHeap` 全 26 site の分類(reality・session ffd75280 続き・全実読)
**重要**: 当初 §7 B は反射・pipeline・draw bind の 4 site しか捉えていなかった。実 grep で `mVkUsesBindlessHeap` は **26 site**、`mVkReflUsesHeapSet` は 4 site。B のスコープは「4 site を直す」でなく「フラグ全参照を分類して分割する」。**中核原理 = `mVkUsesBindlessHeap`(旧)は「fragment が heap set を使うか」の意味で、post-A は `mVkUsesHeapSet` と完全同値(現に heap を触る shader では値不変)。値が変わるのは skin-only shader(SkinnedPBRAlpha/rigged shadow)だけで、それらは A で既に unbind 済=壊れている。∴ 全 site を `mVkUsesHeapSet` へ機械改名すれば、現に動いている heap shader の挙動は完全保存され、skin-only shader は「正しい非-heap 扱い」に戻る**(誤 heap 分類の解消そのもの)。

| Cat | site | 現 gate の意味 | B の処置 | 備考 |
|---|---|---|---|---|
| **定義** | llglslshader.cpp:2945 / :3112 / :409 / .h:318 | フラグ算出・reset・宣言 | `mVkUsesHeapSet` に改名 + **`mVkUsesSkinSet` 新設**。**:2945/:3112 の fallback を分割**(下記) | 核 |
| **反射** | llglslshader.cpp:1948 / :1955 / .h:319 | set=2 反射 | `mVkReflUsesHeapSet`→`mVkUsesHeapSet` 系。**set=3 反射 `out_uses_set3` 追加**(:1295 と同型 1 行) | 核 |
| **pipeline** | llglslshader.cpp:3100 | 3-set layout 組込 | set=2 gate=heap / set=3 gate=skin + empty placeholder | 核 |
| **draw bind** | llvkloader.cpp:13075 | set2=heapSet | `mVkUsesHeapSet` gate(:13107 set3 は `mVkUsesSkinSet`) | 核 |
| **1=heap texture/MDI 路** | lldrawpool.cpp:598,718,840,1149,1166,1206 / llglslshader.cpp:2946,3680,3686 / lldrawpoolalpha.cpp:1071,1550 / lldrawpoolmaterials.cpp:161 | is_indexed/pin/sampler/memo/lane/cameraMDI/imm_cache/indexed-sampler/alpha-MDI/mat-bindless | **純機械改名 `mVkUsesHeapSet`** | skin-only は非-heap 扱いに戻る=正しい |
| **2=per-draw ID 機構(C 領分)** | lldrawpool.cpp:606,665 / llglslshader.cpp:3625,3753 / **llvertexbuffer.cpp:561,593,632** | id publish/setCause/scratch id/**検出器 check gate** | B は `mVkUsesHeapSet` へ機械改名(挙動保存)。**C が `heap\|\|skin` へ再 gate** | ★下記検出器 |
| **4=rigged MDI(skin 意味・要申告)** | lldrawpool.cpp:1398 | rigged shadow MDI 適格(skinBindlessEnabled && heap) | `mVkUsesHeapSet` へ機械改名 → **skin-only で false 化=rigged shadow MDI 無効**。既定 OFF(RIGGED_MDI/INDIRECT switch)ゆえ既定構成 moot。**申告済の挙動変化** | 保存したいなら skin gate だが closed |

**:2945/:3112 fallback の分割設計**:
- 現 :2945 `mVkUsesBindlessHeap = isBindlessActiveVk() && mVkReflUsesHeapSet`。
- 新 = `mVkUsesHeapSet = isBindlessActiveVk() && (reflect set=2)` / `mVkUsesSkinSet = isBindlessActiveVk() && (reflect set=3)`。
- 現 :3112 は heap layout **or** skin layout が null なら両方 false 化(束ね)。新 = **独立**: heap_layout null → mVkUsesHeapSet=false / skin_layout null → mVkUsesSkinSet=false。pipeline layout 組込(:3100-3113)を 2 独立分岐に。

**★ 検出器 gate の是正(§5.1 の補正・重要)**: `checkDrawDataIDAtFire`(per-draw ID 一致オラクル)は **llvertexbuffer.cpp:561/593/632 で `mVkUsesBindlessHeap` に gate されている**。∴ 機械改名だけだと skin-only draw(post-fix `mVkUsesHeapSet=false`)で **オラクルが走らない** = §5.1 の「既存オラクルが skin を自動で守る」が成立しない。**C がこの gate を `mVkUsesHeapSet || mVkUsesSkinSet` へ再 gate**して初めて skin draw もオラクル対象になる。**これは call-site の gate 変更で検出器 file(llvkcontract.*)は無改修** = 憲法4 非該当。C の必須項目に繰入れ。

**MUST-VERIFY(B or 設計者)**: SkinnedPBRAlpha の `mFeatures.mIndexedTextureChannels`。>0 なら :2946 が post-fix(`mVkUsesHeapSet=false`)で indexed sampler(100+i)を追加し descriptor 構成が変わる。PBR alpha は indexed batch 非使用の想定(=0)だが未確認 → B 着手時に実値確認。0 なら影響なし。

**B の申告に必須**: 26 site 全ての分類表(Cat1 機械改名 / Cat2 機械改名(C 再 gate 予定)/ 核 / Cat4 挙動変化)を完了報告に添付。設計に無い gate 変更(特に Cat2 を勝手に heap||skin にしない=C 領分)を炙れる形で。

### 指示書 C(段階1・実装済 `04f30cbfe1c`)= per-draw ID 表現の一貫化 + skin base publish
> **⚠️ C の fail-closed 不変条件(下記 line「mVkUsesSkinSet な draw は必ず setCurrentDrawDataID を通る」)は不完全だった**: 「必ず通る」は「全 skin draw が commit の caller(`buildAndOverrideScenePerDrawSet`)へ到達する」を暗黙前提にしていたが、これは偽。commit を迂回する draw-fire 経路(`pushUntexturedBatch`=影 / object-ID pass)が存在し、そこでは choke が発火しない。C は「id と skin publish を同一変数にした(表現統一)」までで、「commit の呼び出しを網羅させた」ではない。**構造化は §7 指示書 G1-G4(段階1.5)が引き継ぐ**。C の成果(単一表現 `commitPerDrawID`・gate `mVkUsesHeapSet||mVkUsesSkinSet`)は G の土台として維持。
- **対象**: per-draw ID(gl_InstanceIndex=firstInstance)の設定経路(`setCurrentDrawDataID`/`drawDataWriteScratch`/`ensureVkDrawDataSlot`/`mVkDrawDataSlot`/populate の scratch 上書き)を **単一 choke に一貫化**。skin base publish を「skin を使う draw なら bindless 従属でなく必ず publish」に(非 bindless でも firstInstance=skin slot が揃い SSBO が正走)。
- **起点トレース**: `lldrawpool.cpp:606-658`(skin publish + setCurrentDrawDataID・現 gate=mVkUsesBindlessHeap)/ `llglslshader.cpp:3572 vkResolvePerCallSetForDraw`(authored 分岐)/ `:3625/:3753`(scratch 上書き)/ `:3640/3769`。
- **切り口**: per-draw ID は「多重に設定される値」でなく「draw ごとに 1 度決まる単一表現」。scratch と persistent slot の役割を整理。
- **不変条件**: firstInstance と skin base publish slot が常に一致(66ae の A/B oracle mismatch = この不一致の検出だった)。
- **注意**: これを外すと SSBO skin が別 draw の palette を読み視覚破損(私の set-move 試作で観測)。symptom 潰しでなく本設計の一部。

#### 詳細設計(肉・session ffd75280・per-draw ID 全鎖トレース)

**per-draw ID の全鎖(単一値が heap と skin の両索引)**
```
[割当] setCurrentDrawDataID(id)  ── 3 site すべて mVkUsesBindlessHeap gate
   ├ lldrawpool.cpp:634-644  authored 経路(ensureVkDrawDataSlot→mVkDrawDataSlot / 不可なら drawDataWriteScratch)
   ├ llglslshader.cpp:3640-3642  populate record-job seed 経路(drawDataWriteScratch)
   └ llglslshader.cpp:3769-3771  populate 非-record 経路(drawDataWriteScratch)
[保持] setCurrentDrawDataID llvkloader.cpp:11998 → tCurrentDrawDataID(TLS :387)
[消費] llvertexbuffer.cpp:565/597/636  vkCmdDrawIndexed(..., firstInstance = getCurrentDrawDataID())
[shader] gl_InstanceIndex = firstInstance = id
   ├ set=2 b0: aya_dd[gl_InstanceIndex]        (materialF:307 tex heap DrawData)
   └ set=3 b0: aya_skin_base[gl_InstanceIndex]  (objectSkinV:86/145 skin base)
[skin publish] lldrawpool.cpp:649-657  writeDrawSkinBase(id, skin_entry)  同 id で publish
   skin_entry = objectSkinLookupEntry(avatar,hash)(avatar+skinInfo あるとき)/ 無ければ INVALID→UBO fallback
   本体 :8370 sSkinBaseMapped[frame*TOTAL_SLOTS + id] = skin_entry(frame ring)
```

**C の核心 = gate を `mVkUsesBindlessHeap`(heap)→ `mVkUsesHeapSet || mVkUsesSkinSet` に**
skin-only draw(SkinnedPBRAlpha post-fix = heap 非使用)は現状 3 site 全て skip → `setCurrentDrawDataID` されず firstInstance が **前 draw の stale 値 / 0** → `aya_skin_base[0]` = 別 draw の base を読む = **視覚破損**。これが finding の警告した「set-move 試作の視覚破損」の機序(source で確定)。∴ per-draw ID は「heap **または** skin を使う draw」で必ず割当てる。

**設計(単一 choke への集約)**
- per-draw ID 割当 + `setCurrentDrawDataID` を **1 関数(例 `publishPerDrawID(params, cur)`)** に集約し 3 site から呼ぶ。gate = `cur->mVkUsesHeapSet || cur->mVkUsesSkinSet`。
  - tex heap slots(DrawData payload)の充填は `mVkUsesHeapSet` のときのみ意味(skin-only は slots=0 のまま = aya_dd 未参照ゆえ無害)。
  - `ensureVkDrawDataSlot`(persistent params slot)vs `drawDataWriteScratch`(record-job/authored 不能時の一時 slot)の使い分けは現行維持。id は必ず setCurrentDrawDataID へ。
- skin base publish を `writeDrawSkinBase(id, skin_entry)` として同 choke 内で `mVkUsesSkinSet` かつ avatar+skinInfo のとき発火。id は per-draw ID と**同一**(別採番しない)。
- 非-skin / 非-heap draw = choke 不発火 = firstInstance=0(現状同等・aya_dd/aya_skin_base 未参照)。

**fail-closed 不変条件**
- 「firstInstance に使う id」と「writeDrawSkinBase の draw_id」が**同一変数**(採番を分けない)= 66ae oracle mismatch の再発を構造排除。
- `mVkUsesSkinSet` な draw は **必ず** setCurrentDrawDataID を通る(choke の gate に skin を含めたことの帰結)= skin base slot が常に firstInstance と整合。
- assert 候補: skin shader の draw fire 時 `getCurrentDrawDataID()` が当該 draw で publish 済(未 publish=stale を検出)。

**申告欄**
- 解釈: choke 集約は「3 site の重複ロジックを 1 関数化」= [[feedback_delete_dead_and_factor_common]] に沿う。record-job seed 経路(:3625)と非-record(:3753)と authored(lldrawpool:606)の 3 文脈差(scratch vs persistent slot)は関数内分岐で吸収。
- 未決(後続): (1) `isRecordJobActive()` 分岐下で skin publish が現状どう扱われるか(record job 中は skin base ring への書込タイミング)を段階1 実装時に精査 (2) `ensureVkDrawDataSlot` の persistent slot が skin-only draw にも割当たるか(現 params 経路が heap 前提の箇所がないか)。
- 注意: 本 C は A(set=3 生成)・B(mVkUsesSkinSet 反射)が無いと gate の材料(mVkUsesSkinSet)が存在しない = **A/B/C は同一 build で不可分**(§4 段階1 の束ね理由)。

#### 段階1 実現可能性の確定(session ffd75280 続き・未決を source で解消)

**C-1: `ensureVkDrawDataSlot` は heap 分類に非依存 → skin-only draw に per-draw ID を割当可能(実現可能性 確定)**
`LLDrawInfo::ensureVkDrawDataSlot`(llspatialpartition.cpp:4250)は `slots[]` をキーに **LLDrawInfo ごとに** DrawData slot を acquire するだけで heap/skin を判定しない。skin-only draw は `slots[]`=全 0(heap texture 無し)だが `mVkDrawDataSlot` は draw ごとに一意 → firstInstance が一意 → `aya_skin_base[firstInstance]` が正しく機能。∴ **C の「gate を heap→(heap OR skin)」は実装可能**。

**C-2: capacity は非問題** — `DRAWDATA_TOTAL_SLOTS=1048576`(llvkloader.cpp:308)+ `slotSlabGrow`(:357)動的成長。~18k draws/frame に対し桁違いの余裕。skin-only draw に slot を増配しても枯渇しない。

**C-3: 統合すべき site の全数(single choke の対象)**
- id-set(setCurrentDrawDataID)= 3 site: lldrawpool.cpp:644(authored)/ llglslshader.cpp:3641(record-seed)/ :3770(非-record)。
- skin publish(writeDrawSkinBase)= 2 site: lldrawpool.cpp:656(authored)/ :1516(bucket・:1390 rigged 近傍)。
- **id 割当/publish の gate 再設定**: B が上記を `mVkUsesHeapSet` に機械改名済 → C が **`mVkUsesHeapSet || mVkUsesSkinSet`** へ再 gate(skin-only draw も id 割当・skin publish が走るように)。あわせて §7 B「1=heap texture 路」の gate は heap のまま(skin を混ぜない=Cat 分類厳守)。
- **★検出器オラクル gate 再設定(必須)**: `checkDrawDataIDAtFire`(per-draw ID 一致オラクル)は **llvertexbuffer.cpp:561/593/632** で `mVkUsesBindlessHeap`(B 後 `mVkUsesHeapSet`)に gate。C が **`mVkUsesHeapSet || mVkUsesSkinSet`** へ再 gate しないと skin-only draw でオラクルが走らず「skin が別 draw の palette を読む」が検出網の外に落ちる。**検出器 file(llvkcontract.*)は無改修=憲法4 非該当**(call-site の gate 変更のみ)。§5.1 の「既存オラクルで skin を守る」はこの再 gate が前提。
- **不整合の実体**: populate 経路(:3641/:3770)は id を scratch で**上書きするが skin を再 publish しない** → firstInstance が publish 時の id と食い違うと `aya_skin_base` miss。C の single-choke(id と skin publish を同一 id で 1 箇所に)が構造的に潰す。bucket 経路(lldrawpool.cpp:1500-1516)も同 choke に含める。

**A/B-1: empty placeholder set の生成(mechanical・trace 不要)**
0-binding の `VkDescriptorSetLayout`(`bindingCount=0`)を `sEmptySetLayout` として 1 個生成 + 極小 pool から `sEmptySet` を 1 個 alloc。createBindlessHeap 近傍。UAB 無関係(binding 0 個)。`getEmptySetLayout()`/`getEmptySet()` で配布。skin-only draw の set=2 slot をこれで埋め pSetLayouts を 0..3 contiguous に保つ。

**残る段階1 未決(実装 Fresh が着手時に潰す・設計は確定)**
- `sSkinBasePool` 2-binding 化の `maxSets`/`poolSizeCount` 再計算(SSBO_DYNAMIC×1 + SSBO×1)= createBindlessHeap:2912-2925 の書換え粒度。
- record-job seed 経路で skin base の frame-ring 書込タイミング(seed 再利用時に publish が 1 度で足りるか)= 実装時に VkPerf `skin_base_wr` で観測。
- `mVkUsesHeapSet` 改名(現 `mVkReflUsesHeapSet`)の全参照追従(検出器 `LLVKContract` 側含む・憲法4 = 検出器 diff は AYA 承認)。

### 指示書 G1-G4(段階1.5)= per-draw ID の構造化 = 可変 global 廃止 + draw 属性化
> **総括(AYA 2026-07-31)=「設計してないからこうなった」= 構造欠陥**。session 5a7bbf1c が全 fire 経路を実読で機械潰し(§8 真のモデル・scratchpad `perdraw_id_trace.md`)。指示書 C の「表現統一」の上に、**呼び出しの網羅を構造で強制する**。狙い = 忘れた draw が「別 avatar の palette」でなく「自分の正しい UBO fallback」に落ちる = silent corruption を構造排除。**patch(2 経路を個別に塞ぐ)は禁**([[feedback_design_defects_not_fixed_by_measurement]])。
>
> **設計判断(session 5a7bbf1c・設計者決定)= strangler 順で 4 段階の非破壊移行**。各 G は独立 build + 独立 gate(視覚 + オラクル + validation 0)。**依存は直列**(G1→G2→G3→G4)。fail-closed 保険は「毎 frame ring 全 clear(4MB memset/frame = CPU コスト増で②に逆行)」でなく「**予約 slot 0 = 常時 INVALID**」を採る(理由 = §8)。

#### 指示書 G1(段階1.5)= firstInstance を draw API の明示パラメータにする(非破壊ブリッジ)
- **対象**: `LLVertexBuffer::drawRange`/`drawRangeFast`/`drawArrays`/`draw` に per-draw slot を明示引数として通す。既定値 = 現行維持のブリッジ値(下記)。global 経路は G3 まで温存。
- **起点トレース(HEAD 実読済)**: fire = llvertexbuffer.cpp:549 drawRange(:566 `vkCmdDrawIndexed(...,firstInstance=getCurrentDrawDataID())`)/ :593 drawRangeFast(:608)/ :637 drawArrays(:655)。VB は LLDrawInfo を持たず bound shader(sCurBoundShaderPtr)のみ。
- **設計**: 引数 `U32 draw_data_slot = LLVKLoader::PERDRAW_SLOT_INHERIT`(新 sentinel 定数)。fire で `slot==INHERIT ? getCurrentDrawDataID() : slot`。**全既存 caller は無引数 → INHERIT → 現行 global(挙動不変)**。新 primitive のみ明示 slot を渡す。
- **不変条件**: 挙動完全不変(全 caller INHERIT)。オラクル(checkDrawDataIDAtFire/checkPerDrawIDFreshnessAtFire)= 現行違反 2 件のまま(まだ塞がない)。
- **gate**: 視覚同一 + validation 0 + オラクル counter が G1 前と同一(退行ゼロ)。
- **申告欄**: fire hot path に 1 分岐追加(G3 で除去)。indirect(alpha:996/terrain:288 の `dc.firstInstance`)は G1 では触らない(既に per-command 値・G2 で slot 供給元を統一)。

#### 指示書 G2(段階1.5)= 単一 bindless draw primitive の導入 + 全 skin/heap loop の routing
- **対象**: skin/heap を読む全 draw-fire を単一 primitive 経由に統一し、per-draw ID commit(slot 導出 + skin base publish)と fire(明示 slot)を**不可分**にする。
- **起点トレース(HEAD 実読済・全 fire 経路)**:
  - commit する経路: `pushBatch`(lldrawpool.cpp:1794→`buildAndOverrideScenePerDrawSet`:573→:650 commit / :1805 drawRange)。
  - **commit 迂回(違反)**: ①`pushUntexturedBatch`(:1815-1830・buildAndOverride 無し→:1828 drawRange)= Deferred Skinned Shadow。②`renderRiggedObjectIDBufferForAvatar`(pipeline.cpp:11672・:11771 uploadMatrixPalette は呼ぶが commit 無し→:11778 drawRange)= FS Object ID。
  - slot 導出の実体: `buildAndOverride`:637-646(`params->ensureVkDrawDataSlot(slots)`→`mVkDrawDataSlot` / 不能なら `drawDataWriteScratch`)。indirect 正解形: `pushRiggedBatchesIndirect`(lldrawpool.cpp:1466-1518)= per-draw ensureSlot+publishBase+`rec.cmd.firstInstance=draw_id`。
- **設計**: `LLRenderPass::drawInfoBindless(LLDrawInfo& info, LLGLSLShader* cur, <tex slot 材料>)`(名前 TBD)を新設 =(a)slot 導出(既存 ensureVkDrawDataSlot/scratch を関数化 [[feedback_delete_dead_and_factor_common]])(b)`mVkUsesSkinSet` なら publishDrawSkinBase(slot,...)(c)`info.mVertexBuffer->drawRange(..., slot)`(G1 の明示引数)。**descriptor set authoring(buildAndOverride の heap memo/build 部)は分離して残す**(per-draw ID commit のみ primitive へ移す)。
- **★record-producer 不変条件(分散 end-state 適合・§8.7・AYA 2026-07-31 GO)**: primitive を「逐次 immediate fire ヘルパ」でなく「**per-draw record producer**」として設計する = **(a)(b) は pure・order 独立**(distinct slot への書込のみ = 互いに非干渉)、**(c) emit は分離可能な tail**(record + read-only 共有 buffer のみ読む)。→ 段階3(MDI)/段階5(分散)は (c) を immediate→indirect-append に差し替え・(a)(b) を off-main に出すだけで乗る(teardown なし)。record は MDI command 同形(`rec.cmd.firstInstance=draw_id` lldrawpool.cpp:1518 が参照実装)。**先回り実装は禁**(憲法5)= record 構造体を今作り込まない。守るのは「(a)(b) を (c) から分離可能に書く」規律のみ。
  - routing: pushBatch / pushUntexturedBatch / object-ID pass / alpha・terrain indirect(slot 導出元を primitive のヘルパに統一)を全て primitive 経由に。**非 bindless draw(UI 等)は生 drawRange のまま**(slot 不要 = G3 で sentinel)。
- **不変条件**: skin/heap を読む draw は例外なく primitive を通る = commit を迂回できる経路が構造的に消える。firstInstance と skin base publish は同一 slot・同一 primitive 内(採番分離不能)。
- **gate**: 視覚同一(靴下 + 影 + object-ID picker)+ **`skin_draw_no_commit=0`(違反 2 経路が消えること)** + validation 0。**この G2 完了時点で歪み影が根治するはず**(global はまだ在るが全 skin draw が自分を publish するため）。
- **申告欄・要 実装時精査**: buildAndOverride の authoring 部と commit 部の分離粒度(共有する slot/slots 計算をヘルパ化)。object-ID pass は gGL 直叩き経路 = primitive 適用時に描画状態(colormask/depth)の順序を壊さない。alpha indirect(appendAlphaRunCmd:971)は commit タイミング(append 前に primitive で publish 済か)を実装時に VkPerf `skin_base_wr` で観測。

#### 指示書 G2.5(段階1.5)= per-draw ID commit の単一化(競合する scratch fallback を①優先で無競合化)
> **設計欠陥の発見(session 5a7bbf1c・G2 実機 gate が露呈)**: per-draw ID の commit が **2 関数に分裂**し脆弱フラグで協調 → 1 draw に 2 commit 競合。G1 まで fire=global=最後(scratch)で無発火・G2 の明示 id で `drawdata_id_mismatch`(非 allowlist・fail-closed)として顕在化。指示書 C-3(line 270)が予見したが段階1 C は表現統一に留まり**実際には閉じなかった**積み残し。**inline patch でなく設計**([[feedback_design_defects_not_fixed_by_measurement]])。

- **真のモデル(全実読確定・HEAD)= 2 つの commit 経路**:
  - ① persistent: `buildAndOverride→establishPerDrawId`(G2)= id X(persistent 域 [0, 950272))・**skin publish する**・drawRange **前**。
  - ② scratch fallback: `vkResolvePerCallSetForDraw`(llglslshader.cpp:3505)→`populateAndBindUniversalDescriptorSet(false)`(:3642-3663)= `drawDataWriteScratch`+`commitPerDrawID(scratch, publish_skin=false)`・scratch 域 [950272, 1048576)・**skin publish しない**・drawRange **内**(beginShaderDrawOrSkip)。
  - 協調 = `sCurPerCallAuthored`(thread_local・draw またぎ持続): true→②preserve(scratch skip)/ false→②走る。buildAndOverride が memo/build 成功で true を立てる(lldrawpool.cpp:678/696 等)。pushUntextured/object-ID は buildAndOverride を通らず set/reset しない=前 draw の値を継承。
- **欠陥の機序**: ①が X を commit しても `sCurPerCallAuthored==false` なら②が scratch を commit → global/stash が scratch に上書き。fire(明示 X)≠ stash(scratch) → mismatch。値 = actual 小(persistent)≠ expected 大(scratch)で実 log 確認済。**元バグ(歪み影)も同根**: shadow は前 draw の authored=true 継承で②preserve + ①無し = 完全未 commit(skin_draw_no_commit 発火)。
- **設計(invariant-first・first-wins 単一 commit)**: **不変条件 =「1 draw の per-draw ID commit は高々 1 回。①(persistent・draw 前)が優先・②(scratch)は①が無い draw の fallback のみ」**。
  - 実装形 = **`commitPerDrawID` を「本 draw で first-wins」に**(per-draw latch: 既 commit ならスキップ・draw 境界で reset)。latch は llvkloader 内(検出器 `llvkcontract.*` 非改修=憲法4 非該当)。reset は fire(drawRange の check 近傍)。
  - ①が X commit(latch set・skin publish)→ ②の scratch commit は no-op → 単一 commit X → **fire(X)==stash(X)= mismatch 0**。
  - ①が無い draw → ②が唯一 commit(従来 fallback 維持)。**②の dead 証明が不要**(precedence ゆえ live/dead どちらでも正しい)= 撤去の risk なし。
- **起点トレース(実読済)**: 競合 = llglslshader.cpp:3505 vkResolvePerCallSetForDraw / :3542 populateAndBindUniversalDescriptorSet / :3642-3663 scratch commit。①= lldrawpool.cpp establishPerDrawId(G2)。commit 本体 = llvkloader.cpp:12062 commitPerDrawID(:12064 setCurrentDrawDataID / :12066 stashDrawDataID / :12067 markPerDrawIDCommitted / :12070 publishDrawSkinBase)。latch reset 候補 = 既存 `tPerDrawIDCommitted`(llvkcontract:729・fire で reset)と対の llvkloader-side latch を新設。
- **不変条件・gate**: **`drawdata_id_mismatch=0`(全 pool・crowd 込み)** + `skin_draw_no_commit=0` 維持 + 視覚同一 + validation 0。G2+G2.5 は対で mismatch=0 まで **両者 commit 保留**(G2 単独は fail-closed)。
- **申告・要実装時精査**: (1) latch の reset 点(fire=drawRange か beginShaderDrawOrSkip か)を「②が①の後・次 draw の①の前」に正しく置く。(2) ①が publish_skin=true、②が false の非対称 = first-wins で①が勝てば skin publish される(正)。②が唯一のとき skin 未 publish=UBO fallback(従来同・正)。(3) latch を skin freshness(tPerDrawIDCommitted)と共有するか別立てか(責務分離のため別立て推奨)。
- **G3 との関係**: G2.5(commit 単一化)は G3(global 廃止)の**前提**。commit が競合したまま global を消すと fire の id 源が壊れる。G2.5→G3 の順。

#### 指示書 G3(段階1.5)= 可変 global `tCurrentDrawDataID` の廃止 + ② scratch commit 競合の除去 + 予約 slot sentinel
> **⚠️ G2.5(first-wins latch)は revert 済(session 引き継ぎ・deploy md5 `a9371c5ba1db67c9c2c66a8063a2baae`・未 commit)**。latch は「1 draw 1 commit・①優先・draw 境界 reset」を後付け state で強制する band-aid で、静的モデルは頑健に成立を予測するのに実機で `skin_draw_no_commit=3164`(退行)= **設計欠陥を state で patch した時の fragility の署名**([[feedback_design_defects_not_fixed_by_measurement]])。∴ latch を捨て、競合そのものを構造除去する G3 に一本化。現在地 = G2(`skin_draw_no_commit=0`・`drawdata_id_mismatch=567`)。

**真のモデル(session 引き継ぎ・全 fire 経路 実読で確定 = §8 の具体化)**
per-draw ID の commit が **2 経路で競合**する:
- **① persistent**: `establishPerDrawId`→`commitPerDrawID`(lldrawpool.cpp:626)= persistent slot・skin publish・drawRange の**前**。
- **② scratch**: `populateAndBindUniversalDescriptorSet(false)` 内(llglslshader.cpp:3642-3663 の `(heap||skin)&&!preserve_drawdata` block)→`commitPerDrawID(scratch,publish_skin=false)`= scratch slot・skin publish せず・drawRange の**中**(beginShaderDrawOrSkip の vkResolve:3512)。flush 経路(llrender.cpp:1787)からも同 block に到達。
- **競合の機序**: ① が persistent id X を commit(setCurrentDrawDataID+stash+mark)しても、続く drawRange 内の vkResolve が `sCurPerCallAuthored==false` で ② を走らせ scratch id Y で **global/stash を上書き**。fire は G1 の**明示 slot X** を使うので描画は正しいが、`checkDrawDataIDAtFire(X)` vs `stash(Y)` で **`drawdata_id_mismatch`**。**global を消すだけでは不足**: ② が stash に Y を書けば mismatch は再燃 = **② の commit block ごと除去が構造解**。

- **対象**: 可変 global を物理削除し、fire を「draw が運ぶ属性(明示 slot)」に一本化 + **② scratch commit block を除去**(global 廃止後 dead・stash 競合源)。
- **起点トレース(HEAD post-revert 実読済)**: global = `tCurrentDrawDataID`(llvkloader.cpp thread_local)/ `setCurrentDrawDataID`/`getCurrentDrawDataID`/ `commitPerDrawID` の `setCurrentDrawDataID(id)`。消費(global read)= immediate fire ×3(llvertexbuffer drawRange/Fast/arrays の `INHERIT?getCurrentDrawDataID():slot`)+ **alpha append**(lldrawpoolalpha.cpp:996 `dc.firstInstance=getCurrentDrawDataID()`)+ **terrain append**(lldrawpoolterrain.cpp:288 同)。MDI rig は既に explicit(lldrawpool.cpp:1518 `rec.cmd.firstInstance=draw_id`)。
- **設計(全 fire を明示 slot に → global 削除 → ② 除去)**:
  1. **immediate**: drawRange/Fast/arrays の既定を `INHERIT`→**予約 slot 0**。establish 経由 caller は既に id を渡す(G1/G2)。非 establish(非 heap/skin)caller は 0(=INVALID・無害)。fire から `getCurrentDrawDataID()` 参照を除去。
  2. **alpha indirect**: `appendAlphaRunCmd(run, draw, id)` に id 引数追加。id 源 = 直前の `buildAndOverrideScenePerDrawSet(draw,true)` の**戻り値**(lldrawpoolalpha.cpp:1127→1128 / :1563→1581 / :1602→ 各 append の直前で既に呼んでいる = 戻り値を捕える)。`dc.firstInstance = id`。
  3. **terrain indirect**: terrain は establish/commit を持たない(lldrawpoolterrain.cpp は append のみ)。**確定(実読・closed)**: terrain shader 群(terrainV/F・pbrterrainV/F/UtilF)は `aya_dd`/`gl_InstanceIndex`/`aya_draw_id` を一切参照しない = per-draw DrawData 非依存。∴ `dc.firstInstance=0`(予約 INVALID)で**無害**・establish 追加不要。
  4. **global 削除**: `tCurrentDrawDataID`/`getCurrentDrawDataID`/`setCurrentDrawDataID` を削除。`commitPerDrawID` は `setCurrentDrawDataID` 呼びを落とし **stash + mark + (publish_skin なら)publishDrawSkinBase のみ**(id は param で流れる)。
  5. **② scratch commit block 除去**: llglslshader.cpp:3642-3663 の commit block を削除。`populateAndBindUniversalDescriptorSet(false)` は descriptor 解決(sampler/dynamic UBO)のみに戻る。→ ①②競合が構造消滅 = mismatch の根絶。
  6. **予約 slot 0 = 常時 INVALID**: `ensureVkDrawDataSlot`/`drawDataWriteScratch` は 0 を返さない(1 起点)。`aya_skin_base[*][0]` は生成時 0xFF memset で INVALID・DrawData[0]=0 固定。→ bypass/非 establish draw は slot 0=INVALID を読み UBO fallback(skin)/ 0 tex(heap 無害)に落ちる = fail-closed。
- **不変条件**: per-draw ID を fire へ運ぶ手段が「draw の属性(明示 slot)」のみ = 別 draw の値を継承する経路も、競合する第2 commit も構造的に存在しない。忘れ = 予約 INVALID = 正しい fallback。
- **gate**: 視覚同一 + **crowd で影/選択枠/rig が崩れない(決定的)** + `skin_draw_no_commit=0` かつ **`drawdata_id_mismatch=0`** 全経路 + validation 0。= 最終 gate(AYA 視覚)。
- **申告欄(縮小・省略・未決)**:
  - **(a) closed(実読)**: terrain は per-draw DrawData 非依存 = 予約 0 で無害(手順3)。
  - **⚠️ (b) 訂正(独立監査 audit_brief_g1_g2_g3_perdraw で REFUTED)**: 下記の「② ほぼ冗長・全 skin establish」の静的網羅は**不完全だった**。監査が **establish 未経由の生き skin residual を完全列挙**: production = **bump rigged(pushBumpBatch lldrawpoolbump:1031・bumpV HAS_SKIN→getObjectSkinnedTransform=set=3)** / **rigged emissive(drawEmissive lldrawpoolalpha:714・emissiveV:95)** / **rigged PBR emissive(renderPbrEmissives lldrawpoolalpha:844・pbrglowV:100=objectSkinV 経由・GLTF UBO でない)** / preview(model uploader llmodelpreview:5022 gSkinnedObjectPreviewProgram) + debug 3 系(renderDebugAlpha:585 / renderVisibility llspatialpartition:1940 / renderBatchSize:1659)。全て fire INHERIT→0→slot0 INVALID→UBO fallback=**視覚正・skin-only(heap 非該当)=tex 破壊なし**だが `skin_draw_no_commit` 発火 → **G3 gate 到達には各に establish① 追加が必要(Phase 2)**。**教訓 = 静的網羅は現に外した(設計者=実装指揮の利害相反 blind spot)→ 完全性の権威は dual oracle。** heap residual はゼロ(全 material consumer が establish)確定。↓ 以下の旧記述は「② の scratch commit 部の除去可否」としては成立(commit 競合は消える)が、「全 skin が establish 済」の部分が誤り。
  - **(b・旧・部分誤り)= ② はレンダリング上ほぼ冗長**:
    - ② を発火させる shader は **set=2(heap)= materialV/F・indexedTextureV(`aya_dd` 消費)/ set=3(skin)= objectSkinV(`aya_skin_base` 消費)の 4 つのみ**(全 shader tree grep 確定 = `set = 2` 宣言は materialF、`set = 3` は objectSkinV のみ)。
    - これら real consumer は全て **pool 経由で establish①** を通る: pushBatch(:1809)/ pushUntexturedBatch(:1841)/ pushVelocityBatchesTextured(:2133)/ pushRiggedVelocityBatchesTextured(:2188)/ materials(:240)/ alpha(buildAndOverride)/ object-ID(pipeline:11777)/ avatar 本体(llviewerjointmesh:254/263)/ trees。
    - **⚠️🔴 pushGLTFBatch の「非 skin」判定は REFUTED(Phase 2 独立監査 audit_brief_g3_phase2_residual_establish)**: **in-world GLTF-PBR material の rigged variant(gDeferredPBROpaqueProgram.bind(true) / gPBRGlowProgram.bind(true)・pbropaqueV:127/pbrglowV:100 が HAS_SKIN→getObjectSkinnedTransform=objectSkinV=set=3・AYA_SKIN_SSBO は global define llviewershadermgr:1245)は set=3 skin**。これが `pushGLTFBatch`(lldrawpool.cpp:2306)/`pushUntexturedGLTFBatch`(:2327)で establish 無し fire = **最大の skin residual**(main deferred 毎フレーム + shadow)。旧記述は **GLTFSceneManager(drag-drop asset の node UBO skinning=非set3) と in-world GLTF-PBR pool(object skinning=set3) を混同した誤り**。→ Phase 2 の fix 対象に GLTF batch family(:2306/:2327)を追加。**教訓再掲 = 静的網羅は 2 度外した(emissive/preview→GLTF PBR)= 完全性の権威は dual oracle。** 以下 preview/probe/sky/debug は非 heap/skin(要 gate 確認):
    - preview(previewV/F = set0 のみ)/ probe/sky/debug/pipeline-post(screen/debug shader)。リスク 11 ファイルが material/skinned/indexed program を一切 bind しないことを grep 確認済(**ただし上記 GLTF 見落としを踏まえ、最終権威は AYA gate の skin_draw_no_commit=0**)。
    - **唯一の残り = rigged velocity**(`pushRiggedVelocityBatches`:2042 が establish せず fire・`gVelocitySkinnedProgram` は `hasObjectSkinning=true`(llviewershadermgr.cpp:4121)で objectSkinV auto-attach)。ただし skinnedVelocityV.glsl は**独自 main + UBO skinning** で `aya_skin_base` を読まない(:4098-4102 コメント)= 描画は per-draw skin base 非依存。set=3 が反射され `mVkUsesSkinSet=true` になるか(→ ② 発火/oracle 対象)は **objectSkinV の未使用 set=3 が SPIR-V で dead-strip されるか = codegen の壁**(source 決定不能)。motion blur 既定 off。
    - **∴ G3 の robust 手順(strip 挙動に依存しない)**: ② 除去と対で **pushRiggedVelocityBatches / pushVelocityBatches に establish① を 1 本追加**(pool メソッドゆえ他と同型・cheap)= 「全 heap/skin draw が establish を通る」を**構造的に真**にする → ② 除去は無条件に安全。velocity が実際に非 set3 なら establish は no-op(gate `heap||skin` で弾かれる)= 害なし。
  - 解釈: alpha の id は buildAndOverride の戻り値を使う(別採番しない = §8.2 の「firstInstance と skin publish 同一 slot」不変条件を維持)。
  - slot 0 予約で総 slot 1 減(1048575・非問題)。TLS(global)削除は直列前提と整合。
  - **G3→G4 順**: mismatch/no_commit の gate 前に G4(全 fire 機構オラクル拡張)で alpha/terrain indirect の skin freshness を観測可能にしておく(現 immediate のみ)= G3 の gate を全経路で取れる。

#### 指示書 G4(段階1.5)= オラクルを全 fire 機構へ拡張 + 成立の fail-closed 証明(憲法4 承認)
- **対象**: freshness オラクル(現 immediate fire のみ = llvertexbuffer:576/616)を **indirect fire(alpha/terrain)にも拡張**し、全 skin draw の未 publish を機械検出。
- **起点トレース(HEAD 実読済)**: 既存 = `checkPerDrawIDFreshnessAtFire`/`markPerDrawIDCommitted`/`C_SKIN_DRAW_NO_COMMIT`(llvkcontract.*)。immediate のみ配線。indirect = appendAlphaRunCmd(lldrawpoolalpha.cpp:996 append 時に firstInstance snapshot)/ flushAlphaRun(:1000)/ terrain:288 は未オラクル。
- **設計(実装確定 session ec2ce7be)**: alpha `appendAlphaRunCmd` / terrain `appendTerrainRunCmd` の firstInstance 確定点(append 末尾)で既存 `LLVKContract::checkPerDrawIDFreshnessAtFire(true, sh->mVkUsesSkinSet, sh->mName)` を**呼ぶだけ**(immediate 参照実装 = llvertexbuffer drawArrays:665-671)。**★ 検出器 `llvkcontract.*` は無改修**(checkPerDrawIDFreshnessAtFire は llvkcontract.h:134 で既に public・cause C_SKIN_DRAW_NO_COMMIT も既存)= **憲法4 非該当**(design の「新 check 関数が要れば憲法4」分岐は要らない方に確定)。副次効果 = append 点で flag を consume し immediate オラクルの lingering 偽陰性も解消。
  - **MDI-rig(:1518)= G4 対象外 deferred**: `pushRiggedBatchesIndirect` は `publishDrawSkinBase` を直呼びで `markPerDrawIDCommitted` を通らない → 既存 flag では偽陽性。かつ default-off(`AYASTORM_RIGGED_MDI`)。オラクル化は「producer が markPerDrawIDCommitted を呼ぶ」改修を伴う別 stage(default-config gate を非ブロック)。
- **不変条件**: default-config の全 fire 機構(immediate ×4 VB method / alpha indirect / terrain indirect)で `skin_draw_no_commit` が観測可能。ゼロ = 設計成立の正のオラクル(§8 の不変条件が全経路で守られている証明)。MDI-rig は default-off ゆえ別途。
- **gate**: 診断起動(`AYASTORM_VKC=1`)で全経路 `skin_draw_no_commit=0`(crowd 込み)+ validation 0。**この G4 が「観測可能な fail-closed 証明」= 段階1.5 の gate 本体**(視覚は AYA 最終のみ)。
- **申告欄**: MDI-rig(killswitch off)は既に正解形だが、オラクル対象化して回帰網に入れる。terrain は非 skin = skin オラクルは無発火が正(heap のみ)= それも明示検証。

#### 指示書 G★(段階1.5・確定設計 = universal choke)= per-dispatch establish(whack-a-mole)の廃棄と「fire 点で強制される単一不変条件」への一本化
> **総括(AYA 承認 2026-08-01「理想形で」)= G3 Phase1/2 の「各 dispatch に establish を足す」は enumeration = 構造でなく、GLTF PBR rigged を 2 度外した(設計者の静的網羅の blind spot・[[feedback_make_observable_not_reason_to_safety]] の実証)。∴ enumeration を捨て、完全性を「fire 点で機械強制される不変条件」に符号化する。** 旧 §7 G2/G3/Phase2 の per-site 記述は本節に supersede(establish の抽出・予約 slot0・global 廃止・② scratch 除去 は土台として維持)。
>
> **設計トレース = session ec2ce7be(指揮官)全 fire 経路 実読 = scratchpad `g3_design_AvsB_trace.md` + 本節。HEAD `075e4e70a47` + working tree。**

**★ 核心不変条件(1 文)**: **「bindless shader(`mVkUsesHeapSet || mVkUsesSkinSet`)が bind された状態で fire する draw は、必ず established な per-draw slot(自分の `mVkDrawDataSlot` or scratch)を firstInstance に持つ。持たずに fire したら fail-closed(production=予約 slot0=INVALID→UBO fallback / 診断=オラクル発火)。」** — これは routing 規律でなく **fire 点で判定可能な述語**(fire 点は `sCurBoundShaderPtr` を持つ=bindless か否かを知る)。完全性は「全 dispatch を覚えて establish する」でなく「**この述語が全 fire 機構で観測される**」ことで保証する。

**なぜ (A) uploadMatrixPalette 基点でなく (B) primitive + fire-point 不変条件か**(trace 確定):
- **(A) は構造にならない**: uploadMatrixPalette は per-(avatar,mesh) **dedup**(lldrawpool.cpp:1882/1923 の lastAvatar/lastMeshId/skipLastSkin)= establish の per-draw 粒度と不一致で融合不能。かつ heap draw(materialV/indexedTextureV=`aya_draw_id`)は uploadMatrixPalette を通らない=skin 限定。∴ (A) は 24 site の**検証プロパティ**(コード変更毎に再監査)= whack-a-mole のチェックリスト化。→ **(A) は「監査の cross-check ツール」に格下げ**(下記 Part 3)。
- **(B) は establish を fire と不可分に融合**し、忘れ経路を fire 点の述語が捕まえる=構造。doc §8.7 record-producer((a)(b) pure・(c) emit 分離 tail)と一致=分散 end-state の prefix。

**★ 3 部構成(construction + proof + audit)**:

**Part 1 = 融合 primitive(construction・idiom drift の除去)**。現状 ~23 site が `id = establish/buildAndOverride(...)` … `fire(..., id)` を各自複製(間に site 固有 setup)。immediate は密集した同型 idiom を単一 primitive に畳む。indirect は fusable site が 1 個しか無いため専用 primitive を作らず explicit id 明示化で足りる(下記・P1a/P1b 実装確定 session ec2ce7be):
- **immediate = `LLRenderPass::drawInfoBindless(LLDrawInfo& params, BindlessEstablish mode, bool batch_textures)`(実装済 P1a・lldrawpool.cpp)** = 融合 atomic 単位 `{ establish(mode) → setBuffer → drawRange(TRIANGLES,...,id) → vkcVerify }`。mode = `Authored`(buildAndOverride=heap authoring 込)/ `Bare`(establishPerDrawId=skin-only。skin-only では authoring 不要ゆえ Bare が正・:648 sampler guard を回避)。**DrawScope は所有せず site が保持**(tag が oracle currentDrawTag に出るため)。**site 固有 pre-setup(texture bind / applyModelMatrix / pushConstant / cull_face / SSS flag / tex_setup teardown)は呼び出し側に残す**(shader 依存で融合不能)。routing 13 site = pushBatch/pushUntexturedBatch/velocity×3/materials/bump/alpha immediate×3/pushVerts + **GLTF batch×2(leak 根治)**。
- **indirect = 専用 primitive を作らない(P1b 設計確定・AYA 承認 session ec2ce7be)**。理由: establish↔append を融合できる site は `renderEmissives`(#a)の 1 箇所のみで、`renderAlpha`(#b)は間に als_ perf 計測が挟まり融合不可(edge が正)。1-caller の wrapper は drift を消さず indirection を足すだけ。∴ **`appendAlphaRunCmd(run, draw, id)` に explicit id を渡す形(P1a 実装済・可変 global 非経由)+ Part 2 の G4 オラクル(append 点)で indirect 完全性を担保**。専用 record primitive は不要。
- **★ 最大効果 = GLTF batch leak の構造根治**: `pushGLTFBatch`(lldrawpool.cpp:2306)/`pushUntexturedGLTFBatch`(:2327)は現状 `drawRange(...)` を **id 無し**で呼ぶ(=INHERIT→slot0=最大 skin residual)。primitive 経由に置換で establish が構造的に入る。
- **fuse できない edge**(fire 機構が違う=無理に 1 signature に畳まない): octree debug `draw()`(llspatialpartition:1942)/ object-ID(pipeline:11777 explicit shader・DrawScope 無し)/ null-param(tree:120/208・jointmesh:254/263・preview:5021)/ MDI-rig(:1518 既に正解形)/ terrain(:288 非 bindless=firstInstance 0 が正)。**これら edge は establish-then-fire のまま残すが、完全性は Part 2 の fire 点述語が担保**(primitive は「密集した同型 immediate 群の idiom drift を消す」道具であって、完全性の source ではない)。

**Part 2 = fire 点 fail-closed 不変条件(proof・完全性の唯一の source)**。完全性は routing でなく **全 fire 機構で核心述語を観測可能にする**ことで保証。**全 fire 機構 = 7**(独立監査 audit_brief_g_star_full で確定・当初 6 は bucket MDI を数え落とし):
1-4. **immediate ×4**(llvertexbuffer drawRange:577 / drawRangeFast:618 / drawArrays:668 / draw→drawRange 委譲)。5. **alpha indirect append**(appendAlphaRunCmd・G4 で配線)。6. **terrain indirect append**(appendTerrainRunCmd)。7. **static bucket MDI**(pushIndirectSpans lldrawpool.cpp:1227・`vkCmdDrawIndexedIndirect`)。
- **production**: 予約 slot0=常時 INVALID(§8.5・G3 で確立)→ 未 establish の bindless draw は slot0 を読み UBO fallback(skin)/ 0 tex(heap)= silent corruption 不能。
- **診断(`AYASTORM_VKC=1`)**: オラクル `skin_draw_no_commit` が全 fire 機構で発火可能(G4 で indirect append + bucket MDI に配線)。**`skin_draw_no_commit=0`(全 7 機構・crowd 込)が「全 bindless skin draw が established」の機械証明**。
- **★ G4 は検出器無改修 = 憲法4 非該当**(`checkPerDrawIDFreshnessAtFire` は既に public・call-site 追加のみ)。
- **bucket MDI の skin 不変条件(明記)**: bucket は skin を publish しない設計ゆえ **skin draw は bucket に入れてはならない**。現状これは**構造的に排除**済(`kBucketizedPasses` に `*_RIGGED` 皆無 + bucketize 条件 `info->mAvatar.isNull()` llvkbucket.cpp:378 + untextured MDI は非 heap)。この排除を fire 点で **fail-closed 観測**するため pushIndirectSpans にもオラクルを配線(現状無発火・将来 rigged を bucket 化したら発火=tripwire)。
- **terrain は非配線に戻す(監査 D-2)**: terrain shader は**構造的に永久非 skin** = オラクルは無発火が確定=観測価値ゼロ、かつ flag consume の微小 masking。∴ terrain append の freshness 呼びは撤去。bucket MDI(将来 skin 化し得る)とは扱いを分ける。
- ∴ 新規/忘れ dispatch が将来入っても、fire 点述語が silent 化を不能にする(= [[feedback_invariant_first]] + [[feedback_make_observable_not_reason_to_safety]] の理想)。

**Part 3 = uploadMatrixPalette-set cross-check = 補助であって完全性の証明ではない(★ 監査で反例確定)**。当初 skin draw の ground-truth 列挙に uploadMatrixPalette caller を使う想定だったが、**独立監査が uploadMatrixPalette を通らない skin draw を発見**(rigged GLTF shadow = `gDeferredShadowGLTFAlphaBlendProgram.bind(rigged)`→`pushGLTFBatch` 直呼び・pipeline.cpp:9313-9324・set=3 skin・uploadMatrixPalette 無し)。∴ **uploadMatrixPalette-set は skin draw の全集合でない = Part 3 は完全性を証明できない補助**。これは (A)(uploadMatrixPalette 基点 enumeration)を却下し (B)(fire 点 authority)を採った判断の実例的裏付けでもある。**完全性の権威は Part 2(fire 点オラクル)のみ**。設計者は Part 3 を「証明」扱いしない(gate 前に漏れを先に 1 つ潰す静的 aid に留める)。

**fire-shape taxonomy(実読確定・routing の母集団)**:
| fire 形 | 機構 | 代表 site | routing |
|---|---|---|---|
| immediate + LLDrawInfo + drawRange(TRI) | `drawRange(...,id)` | pushBatch:1820 / pushUntextured:1845 / velocity×3 / materials / bump / alpha immediate×3 / spatial:1658 / **GLTF batch×2(leak)** | **drawInfoBindless** |
| indirect append + LLDrawInfo | `appendAlphaRunCmd(run,draw,id)`→flush | alpha:1131/1566/1605 | **explicit id 明示化(P1a済・専用 primitive 無し)+ G4 オラクル** |
| immediate + null-param | `buildAndOverride(nullptr)`→drawRange | tree:120/208 / jointmesh:254/263 / preview:5021 | edge(establish-then-fire 維持・Part2 担保) |
| immediate + draw()/explicit shader | `draw(...,id)` / DrawScope 無し | octree:1942 / object-ID:11777 | edge(同上) |
| MDI-rig | `rec.cmd.firstInstance=draw_id` | lldrawpool:1518 | 正解形(不変・default-off・publishDrawSkinBase 直呼びで既存 flag オラクル化不可=別 stage) |
| terrain indirect | `dc.firstInstance=0`(構造的永久非 skin) | terrain appendTerrainRunCmd | 対象外(0 が正・オラクル非配線=D-2) |
| **static bucket MDI** | `vkCmdDrawIndexedIndirect`(pushIndirectSpans) | lldrawpool:1227 | skin は構造排除(avatar.isNull)+ **fire 点 tripwire オラクル配線**(将来 skin bucket 化を fail-closed 化) |

**完成の gate(AYA・最終のみ PASS 権)**: 視覚同一(crowd で影/選択枠/rig/靴下 崩れない=決定的・**特に rigged GLTF/PBR の影**〔pipeline:9324 が post-G★ で SSBO skin 新規活性〕を明示確認)+ **`skin_draw_no_commit=0`(全 7 fire 機構・crowd 込)** + `drawdata_id_mismatch=0`(全 pool・crowd)+ **`C_MV_STALE_VALUE=0`**(vkcVerify 新規 coverage 7 site=監査 D-3)+ validation 0。gate で漏れ=Part 2 不変条件の穴=構造を直す(単発 patch 禁)。

> **歪み影について(AYA 2026-08-01)**: 本設計が根治する「歪み影」(rigged 衣装の影が別ポーズ/別アバターの palette で歪む・AYA 実機 bisect `AYASTORM_SKIN_BINDLESS=0` で消滅)は **bind 経路の設計欠陥の症状**であり、**バグ patch でなく「欠陥設計を新設計に置換」して根治**した(段階1 = skin set=3 独立化 / G★ = per-draw ID の可変 global 廃止・draw 属性化・fire 点 fail-closed)。∴ **"歪み影 Fix" という discrete な修正痕は存在しない**(設計そのものを入れ替えたので欠陥の住処が消えた)。再発経路は「可変 global 継承の不在 + 全 fire 機構の fail-closed 観測」で構造排除。視覚 gate は「新設計が正しく描くか」= 新設計の受け入れであって、バグ修正の着地確認ではない。

**実装座組**: 本節が設計 doc(設計者維持)→ **Fresh 実装 Brief**(primitive 2 本 + ~23 site routing + GLTF leak + G4 オラクル拡張)→ 設計者が**完全読了突合** + Part 3 cross-check → AYA gate。**Part 1(primitive+routing)と Part 2(G4 オラクル)は独立 build・独立 gate**(依存: routing 後に G4 で全経路観測)。

---

### 指示書 D(段階2)= material params の per-draw-indexed SSBO 化
- **対象**: material params(per-program UBO・per-draw dynamic)を dynamic UBO から **per-draw ID で index する GPU buffer** へ移す設計。draw 毎の alloc+memcpy+offset を消す。
- **起点トレース**: `llglslshader.cpp vkResolvePerProgramForDraw`(allocPerDrawUBOSlice+memcpy+dynamic offset)/ `mVkPerProgramUBOBinding`/ `LLVkUboReg`(per-program block 台帳)/ materialF の per-program block(SSRUtil/AlphaF 等)。
- **切り口**: 指示書 A の skin set を**雛形として踏襲**。material の per-program UBO block が load-bearing な依存を棚卸しし移設可能性を確定。
- **不変条件**: 視覚同一。dynamic UBO の per-draw 書込を残さない(= MDI の前提)。
- **依存**: 指示書 C(per-draw ID)完了が前提。
- **⚠️ 雛形の「clean 部」= palette パターンのみ踏襲(監査 F5・session ffd75280)**: skin set=3 は **b1 palette(plain STORAGE_BUFFER・`aya_skin_palette[gl_InstanceIndex 由来 base + i]`)= per-draw-indexed SSBO の理想形**と、**b0 base(STORAGE_BUFFER_DYNAMIC・frame-ring dynamic offset)= skin 固有の baggage** の 2 種を含む。material が真似るべきは **palette の「plain SSBO を firstInstance で index」パターン**であって、base の dynamic-offset(frame ring)ではない。material params は per-draw-indexed で足り frame-ring dynamic offset を要さない。base の dynamic は skin が UBO fallback と ring を共有する歴史的経緯に由来する固有事情 = 踏襲しない。

### 指示書 E(段階3)= material draw の MDI 畳み込み
- **対象**: material が per-draw-indexed(指示書 D)になった土台で、material draw を MDI(indirect)に畳む設計。
- **起点トレース**: 既存 MDI(`AYASTORM_INDIRECT`/`isIndirectDrawEnabled`/static MDI・dedup)/ `lldrawpoolmaterials.cpp`/ 現 rigged MDI(`riggedMdiEligible` lldrawpool.cpp:1390)を参照実装として。
- **不変条件**: draw 数減が frame time に効く実測(標的 hot・適格母集団)。順序保存が要る族(alpha)は畳めない天井を明記。

### 指示書 F(段階4)= shadow / probe の per-draw-indexed 化 + bucket
- **対象**: shadow(≈24%)・probe(≈20%)pass を同モデル(per-draw-indexed SSBO + bucket + MDI)へ。44% 本命の大工事。
- **起点トレース**: shadow pass の draw 記録経路 / 現 rigged shadow MDI / bucket 未整備箇所(`handoff_perdraw_mdi_pass_map` の pass 別 draw 数を HEAD 検証)。
- **不変条件**: 視覚同一 + per-draw 記録コスト実測減。
- **注意**: shadow の複数カスケードは**正当な並列(独立視錐台)**= 撤去しない(`vknative_direction.md`)。

### (段階5+)draw 連鎖の分離・分散 = **まだ設計しない**
- per-draw が完全に SSBO/MDI 化して初めて分解可能性を再評価。ここで指示書を新規に起こす。**先回り設計は禁**(過去の失敗パターン)。

## 8. per-draw ID の真のモデル(session 5a7bbf1c・全 fire 経路 実読トレース = HEAD `075e4e70a47`)
> 全 producer/consumer をソース file:line で機械潰し(grep-spot でなく実読)。段階1.5(指示書 G)の設計根拠。scratchpad 詳細 = `perdraw_id_trace.md`。

### 8.1 一文モデル
**per-draw ID(= LLDrawInfo の draw 固有 slot `params->mVkDrawDataSlot`)は既に draw の属性として存在する。欠陥は、それを fire(`vkCmdDraw(...,firstInstance)`)へ届ける手段が「可変 thread_local global `tCurrentDrawDataID` を fire 前に誰かが commit する」ことに依存し、その commit が draw の属性でなく「render loop がどの push* を選んだか」に配線されている点。** vkCmdDraw は firstInstance を明示引数で取る = slot を渡せる口は既にある。バグは global を渡していること。

### 8.2 全鎖(producer → 保持 → consumer)
```
[producer] commitPerDrawID(id, publish_skin, avatar, hash)  llvkloader.cpp:12062
  ├ setCurrentDrawDataID(id) :12064 → tCurrentDrawDataID(:391 TLS) = id
  ├ stashDrawDataID + markPerDrawIDCommitted :12066-67 (オラクル記録)
  └ if(publish_skin) publishDrawSkinBase(draw_id,avatar,hash) :12070
        → skin_entry = objectSkinLookupEntry(avatar,hash) :12056
        → writeDrawSkinBase(draw_id, skin_entry) :8410 → sSkinBaseMapped[frame*TOTAL+draw_id]=skin_entry
  caller1 = buildAndOverrideScenePerDrawSet lldrawpool.cpp:650 (publish_skin=cur->mVkUsesSkinSet)
  caller2 = llglslshader.cpp:3662 scratch (publish_skin=false)
[保持] tCurrentDrawDataID = 唯一の可変現在値(consumer 全員が「現在値」を読む)
[consumer] firstInstance:
  (1) immediate  llvertexbuffer.cpp:566/608/655  vkCmdDraw*(...,getCurrentDrawDataID())
  (2) alpha/terrain indirect  lldrawpoolalpha.cpp:996 / lldrawpoolterrain.cpp:288
        dc.firstInstance = getCurrentDrawDataID()  ← append 時に per-command struct へ snapshot
  (3) MDI-rig indirect (killswitch off)  lldrawpool.cpp:1518  rec.cmd.firstInstance = draw_id  ← 正解形(global 非経由)
[shader] gl_InstanceIndex = firstInstance
  ├ skin: objectSkinV.glsl:86/145  aya_skin_base[gl_InstanceIndex] (set=3 b0)
  │    → INVALID なら dynamic UBO matrixPalette に fallback(:87)
  └ heap: indexedTextureV:45 / materialV:144  aya_draw_id = gl_InstanceIndex(DrawData 索引)
```

### 8.3 なぜ silent に壊れるか(sentinel をすり抜ける機序)
shader は既に fail-closed fallback を持つ: `aya_skin_base[gl_InstanceIndex]==AYA_SKIN_INVALID(0xFFFFFFFF)` なら正しい UBO へ落ちる(uploadMatrixPalette が draw 直前に現 draw の palette を UBO へ upload 済)。**しかし** 忘れた draw の gl_InstanceIndex = stale global slot = *別 draw が publish 済の有効値* → INVALID 判定をすり抜け → 別 avatar の SSBO palette を読む。sentinel は「INVALID か否か」しか見ず「stale-valid」を捕まえられない = 歪み影/rig 混線。

### 8.4 commit を迂回する経路(違反・実読確定 = オラクル列挙と一致)
- ①`pushUntexturedBatch`(lldrawpool.cpp:1815-1830): `buildAndOverride` を呼ばず直接 drawRange = Deferred Skinned Shadow。対 `pushBatch`(:1794 は呼ぶ)。
- ②`renderRiggedObjectIDBufferForAvatar`(pipeline.cpp:11672): uploadMatrixPalette(:11771 palette DATA)は呼ぶが commitPerDrawID 無し(:11778 drawRange)= FS Object ID(grep が見逃した 2 例目をオラクルが発見)。

### 8.5 ring buffer 寿命(fail-closed 保険の設計根拠)
`sSkinBaseMapped` は生成時 1 度だけ memset(0xFF=INVALID)(:3057)。**毎 frame clear なし**。frame-index 別 ring は FRAMES_IN_FLIGHT frame 前の値を保持。
- **設計決定(G3)**: 「毎 frame ring 全 clear」は 4MB memset/frame = CPU per-draw コスト削減(②)に逆行 = 不採用。代わり **予約 slot 0 = 常時 INVALID** で bypass を fail-closed 化。real draw は G2 primitive が draw ごとに publish するため自 slot は常に current(未 publish の real draw は「描かれない draw」= 誰も読まない)= 全 clear 不要。

### 8.6 G 完了後の end-state 不変条件(§2 の per-draw ID 部分の具体形)
- per-draw ID を fire へ運ぶ手段 = draw の属性(明示 slot 引数)のみ。可変 global 不在。
- skin/heap を読む draw は例外なく単一 primitive を通り、firstInstance と skin base publish が同一 slot・不可分。
- 迂回・忘れ = 予約 INVALID slot = 正しい UBO fallback(skin)/ 0 tex(heap)= silent corruption 構造排除。
- 全 fire 機構で `skin_draw_no_commit=0` が観測可能(G4 オラクル)= 成立の正のオラクル。

### 8.7 分散 end-state 適合 = 描画連鎖の「鎖リンク」列挙(AYA 2026-07-31・目標台帳)
> AYA 視点: 並列化は諦めていない。描画連鎖(cull→geo→resolve→record→submit)の鎖を切り分けられれば分散可能。**分散機構の設計は本 bind/per-draw 設計が先頭に立った後**(その時この設計にも変更が入るのは当然)。ここでは「鎖のリンク = draw 記録を order/state 依存にしている共有可変状態」を列挙し、各段がどれを切るかを台帳化する(**今は認識のみ・先回り実装は禁 憲法5**)。self-describing draw(各 draw が自分の全パラメータをデータで持つ)= 記録が pure = 分散の必要条件。
>
> **切断とは**: draw 間で共有される可変カーソル/状態を消し、per-draw ID で index する GPU buffer への独立書込 + command struct に埋めた自己記述 record に置換すること。

| # | 鎖リンク(共有可変状態) | source | 担当段 | 状態 |
|---|---|---|---|---|
| L1 | **可変 global per-draw ID カーソル** `tCurrentDrawDataID`(前 draw が set→次 draw が read) | llvkloader.cpp:391 | **段階1.5 G3** | 本設計で切断 |
| L2 | **per-draw descriptor bind state**(sCurPerCallVkDescriptorSet / per-call memo / sCurBoundShaderPtr) | llglslshader.cpp per-call 群 | 段階1-4(bind redesign)+ bindless | 進行中 |
| L3 | **per-draw dynamic UBO(material params)** = draw 毎 alloc+memcpy+offset | llglslshader vkResolvePerProgramForDraw | 段階2(指示書 D) | 未 |
| L4 | slot slab acquire(cold・cache 済) `ensureVkDrawDataSlot` | llspatialpartition.cpp:4250 | 分散設計時 | 認識のみ(低頻度共有点) |
| L5 | skin entry lookup mutex `objectSkinLookupEntry`(read-mostly cache) | llvkloader.cpp:8396 | 分散設計時 | 認識のみ(競合軽微) |
| L6 | **alpha 描画順序**(back-to-front = GPU semantic order) | POOL_ALPHA | 段階5+ | 記録は並列可・submit は順序(sort で解ける=分解の壁でない) |
| L7 | **単一 submitter / command buffer** | PE submit thread | — | **load-bearing = 保持**(分散形 = per-thread CB + ordered submit) |
| L8 | mega-buffer VB/IB slices | llvertexbuffer megabuffer | — | **read-only 共有 = 並列安全**(切断不要) |

- **本設計(段階1.5)= L1 を切る**。self-describing draw への第一歩。
- **prefix 判定基準(全 G 共通)**: 各 G 判断は「分散モデルの prefix か / precludeするか」で検証する。preclude する設計(例: 逐次 immediate に不可分に結合した primitive)は採らない(→ G2 の record-producer 不変条件)。
- **未証明の正直な限界**: 段階5(分散)は未設計 = 本設計が「十分」とは証明できない。主張できるのは ①既知リンク L1 を除く ②既合意 MDI/SSBO 到達形と整合 ③record-producer で prefix に留める、の 3 点(= 作り直しリスクを構造的に最小化)。
