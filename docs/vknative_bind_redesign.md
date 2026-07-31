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

### 指示書 C(段階1)= per-draw ID 表現の一貫化 + skin base publish
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
