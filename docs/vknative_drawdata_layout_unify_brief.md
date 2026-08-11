# DrawData layout 統一 実装 Brief(defect sweep 第 1 弾)

制定 2026-08-11。起票 = Stage 0 Brief B10-次(AYA 合意)。mdiHash 拡張 = **AYA 承認済 2026-08-11(憲法 4)**。
上位命題 = 「just-in-time 供給」族の根治 1 号: consumer の draw モード(batch_textures)がデータ内容を変える **flavor 概念をデータモデルから消す**。完了時に INV-C の 3 分岐 + scratch 逃がし(封じ込め)を撤去する。

## L0 不変条件
**破れている不変条件**: 「record の persistent DrawData は唯一の正準内容を持つ」— 現行は同一 record の内容が呼び手の bt で変わる(flavor)。INV-C(establish の 3 分岐)は封じ込めであり、真の根 = layout がモード依存であること。
**根治形**: slots 内容を **LLDrawInfo member の純関数**にする(bt 消滅)。不変条件がデータモデルに内在化し、コードで守る必要が消える。

## L1 前提事実(トレース済・実装者は着手前に自分の目で再確認)
| 事実 | file:line |
|---|---|
| 現行 layout の唯一の producer(bt 分岐込み) | lldrawpool.cpp:517-563 |
| 書き手 5 site 全数 = establish / freeze / tpl fire / rigged slow / bucket rebuild | lldrawpool.cpp:579/:1369/:1558/:1736・llvkbucket.cpp:363 |
| INV-C 3 分岐 + scratch 逃がし(撤去対象) | lldrawpool.cpp:585-602 |
| struct 宣言 2 箇所(injection / materialF)。shadowAlpha* は injection 依存で自前宣言なし | llshadermgr.cpp:685・materialF.glsl:313-315 |
| 非 batch flavor の GPU 読み手 = materialF のみ(tex_slots.y/.z) | materialF.glsl:330/:342 |
| batch flavor の GPU 読み手 = injected diffuseLookup(tex_slots[i]) | llshadermgr.cpp:700/:705 |
| flavor 不変のスカラ読み(misc.z / misc2.x) | shadowAlphaMaskF.glsl:59/:72・shadowAlphaBlendF.glsl:62・materialF.glsl:44-48 |
| mdiHash 被覆 = [0..12](拡張対象) | llvkcontract.h:126-131 |
| VKC は hash を不透明 U64 で保存・再計算なし = 上限変更が透過 | llvkcontract.cpp:484-531 |
| [13][14][15] 未使用(skin base は別 buffer) | lldrawpool.cpp:548・llvkloader.cpp:3481/:8919-8937 |
| leader 面 = 必ず texture index 0(list[0]==mTexture 構造保証) | llvovolume.cpp:7362-7363/:7445 |
| 非 batch 面の頂点 texture_index は 0 に fallback | llface.cpp:2016 |
| multi-tex ⊥ bump/material/PBR(texture batch 除外) | llvovolume.cpp:5701/:5716/:5737 |
| bindless off 時は CPU/GPU とも本経路が inert(fallback 枝あり) | lldrawpool.cpp:571-573・materialF.glsl:321/:332/:344・llviewershadermgr.cpp:1251-1258 |
| vulkanize 鍵リテラル | llglslshader.cpp:1743 |

## L2 正準 layout(恒久)
| uint | 内容 | 書き元 member | GPU 読み手 |
|---|---|---|---|
| [0] | diffuse heap slot | mTextureList.size()>1 ? list[0] : mTexture(両者同値 = L1 保証) | diffuseLookup / materialF diffuseMap |
| [1..3] | texture list 2..4 枚目(list>1 時のみ・else 0) | mTextureList[1..3] | diffuseLookup[vary_texture_index] |
| [4..7] | spec_color | mSpecColor | materialF |
| [8..12] | emissive / env / min_alpha / sss / object_alpha(現行どおり) | 各 member | materialF・shadowAlpha* |
| [13] | normal map heap slot | mNormalMap | materialF bumpMap |
| [14] | specular map heap slot | mSpecularMap | materialF specularMap |
| [15] | 0(予備・hash 域外) | — | なし |
- [1..3] と [13][14] は位置が分離しているため、仮に list と map が同一 record に共存しても衝突しない(排他証明 L1 は防御の重ね)。

## L3 CPU 改修
### L3.1 computeDrawDataSlots(lldrawpool.cpp:517・宣言 lldrawpool.h:403)
- signature = `computeDrawDataSlots(const LLDrawInfo* params, U32* slots)`(bt 引数削除)。
- 実装形: 全 16 uint ゼロ初期化 → `list.size()>1` なら [0..3]=list[0..3](llmin 4・nullptr は vkHeapSlotOrDefault の白)/ else mTexture.notNull() なら [0]=mTexture / else [0]=default 白。**分岐に依らず** mNormalMap→[13]・mSpecularMap→[14]。[4..12] は現行どおり。
### L3.2 establishPerDrawId(lldrawpool.cpp:565-611)
- **⚠️ 訂正(2026-08-11 gate FAIL で反証・INV-C 3 分岐は復元済)**: 当初「内容の純関数化により 3 分岐を撤去できる」と設計したが、gate 走行で mdi_stale/overwrite が発火し反証された。内容は flavor では不変になったが **texture 常駐の時間変化で変わる**ため、establish を persistent slot の第 2 writer にすると freeze/tpl の正準 author と VKC tick を跨いで交錯し stale/overwrite 窓が開く。**INV-C 3 分岐は flavor 対策でなく single-writer 規律として load-bearing** — 撤去禁止。統一 layout(bt 除去・[13][14])とは独立に共存する。
- 現行形 = B10 の 3 分岐(INVALID→ensure / memcmp 一致→ensure / 不一致→scratch)+ bt なし。
### L3.3 呼び手の bt 除去(4 site)
- freezeAuthorShadowSources(:1346 の bt 変数・:1369)/ pushIndirectBucket(:1556-1558)/ pushRiggedBatchesIndirect(:1735-1736)/ llvkbucket.cpp ensureRecordDrawDataSlot(:360-364 の mdiBatchTextures 呼び)。
- `mdiBatchTextures()` 自体は**残置**(pushBatch fallback :1595/:1614 の draw モード選択 = shader interface 用途。DrawData からは切断)。
### L3.4 mdiAuthorAndCheck(lldrawpool.cpp:616-650・宣言 lldrawpool.h)
- bt 引数削除。β の n = `mTextureList.size()>1 ? llmin(size,4) : 1`(現行同値)。
- **β 被覆拡大**: [13]=mNormalMap / [14]=mSpecularMap の heap identity 照合を既存 loop と同規則(slot 0/既定 slot 除外)で追加。ch 表示 = 13/14。

## L4 GLSL 改修
### L4.1 struct 再定義(2 site を同一字面で)
```
struct AyaDrawData { uvec4 tex_slots; vec4 spec_color; vec4 misc; float object_alpha; uint normal_slot; uint specular_slot; uint dd_pad; };
```
std430 offset = 0/16/32/48/52/56/60・stride 64B 不変。site = llshadermgr.cpp:685・materialF.glsl:313。
### L4.2 読み手変更(member 名の追随)
- materialF.glsl:330 `tex_slots.y` → `normal_slot` / :342 `tex_slots.z` → `specular_slot`
- shadowAlphaMaskF.glsl:72・shadowAlphaBlendF.glsl:62 `misc2.x` → `object_alpha`
- **不変**: tex_slots.x(diffuse)・misc.xyzw(materialF :44-48・shadowAlphaMaskF :59)・diffuseLookup 本体(llshadermgr.cpp:700/:705)。
### L4.3 鍵 bump と deploy
- llglslshader.cpp:1743 → `"vulkanize:v6_drawdata_layout_unify"`(全 program 再コンパイル = layout swap の原子性を担保)。
- shader 変更 = packaged + `~/ayastorm/` へ明示 deploy(CLAUDE.md 恒久義務)。

## L5 検証器(憲法 4・AYA 承認済 2026-08-11)
- llvkcontract.h:129 `i < 13` → `i < 15`。:125 の被覆域コメントを [0..14] へ追随(既存コメントの真実性維持・新規コメント追加なし)。
- llvkcontract.cpp = 変更なし(hash 不透明)。rigged fast path(lldrawpool.cpp:1730)は保存済み mVkDrawDataSlots から計算 = 自動追随。

## L6 実装順・commit 粒度
CPU layout・GLSL 読み手・hash 域は**相互整合が必須 = 単一 commit**(分割すると中間状態で供給と読みが食い違う)。.h 変更(llvkcontract.h/lldrawpool.h)= フルビルド。

## L7 gate 手順(fail-closed)
1. build -j20 + 3-path deploy(`cp --remove-destination -p`)+ shader deploy。
2. 診断起動 `AYASTORM_VKC=1 AYASTORM_PERF_LOG=5`・影あり通常シーン + material(normal/spec)素材 + 多テクスチャ batch 面 + 影 alpha を含む視野。
3. PASS 条件: 警報全欄ゼロ(default-deny)/ mdi_stale=mdi_overwrite=0(V1/V2 = 正準不変条件の検証器)/ C_MDI_HEAP_IDENTITY=0(β 拡張込み)/ validation 0 / devlost 0 / shamdi 4+4 発火継続・mdi_full=0。
4. AYA 視覚 gate(最終のみ)。
- 配当の観測: establish 系 scratch 書き(非正準 flavor 分)の消滅。VkPerf に scratch 計器があれば前後比較を報告に添付(なければ省略・計器新設はしない)。

## L8 縮小・省略・解釈申告
1. [15] は 0 固定の予備のまま(hash 域外)。
2. mdiBatchTextures は shader interface 選択用に残置 = flavor は「データ」から消えるが「draw モード」としては残る(正当な区別・撤去しない)。
3. 非 bindless fallback 枝(set=1 固定 binding)は**無変更**。実装時に binding 番号不変を目視 audit し完了報告に記載(憲法 7 の全枝義務)。
4. scratch 機構は ensure 失敗時 fallback として残置(撤去は INV-C 分岐のみ)。
5. GLTF 系は establish 経由で統一関数を通る(専用 materials UBO は別系)= 挙動不変扱い。gate の視覚確認シーンに PBR 物を含めて裏取り。
6. B10 risk 1(rigged fast path の flavor 不定)は本根治で消滅(供給が一意化)。
7. 性能: computeDrawDataSlots が全 record で [13][14] を常時書く増分 = member null チェック 2 回 = 無視可能(計測しない)。

## L9 設計自己監査(敵対的・2026-08-11)
- **[最弱主張 1・検証済]** 「材質 record の頂点は tex_slots[0] を読む」= 非 batch 面の頂点 texture_index は 0 fallback(llface.cpp:2016)で、channels>1 shader に材質 record が乗っても [0] 読み。材質 shader(materialF)自体は diffuseLookup を使わず macro 直読み。
- **[最弱主張 2・検証済]** 「list[0]==mTexture」= genDrawInfo batch 分岐が leader に index 0 を無条件付与(llvovolume.cpp:7362-7363)し `tex = texture_list[0]`(:7445)。distance_sort 枝(:7385-7401)でも leader の 0 は :7363 が先に確定。
- **[最弱主張 3・非依存化]** multi-tex ⊥ map 排他(can_batch_texture :5701/:5716/:5737)は isRenderingDeferred 条件(:5712)を含み環境依存の匂いがある — が、L2 layout は位置分離により**排他に依存しない**(共存しても衝突なし)。排他は「15 uint で足りる」の裏付けから「防御の重ね」に降格済み。
- **[監査確認]** descriptor/C++ 側は set/binding 番号でしか SSBO を見ない(member 名変更は SPIR-V 内で閉じる)= C++ の descriptor 配線変更ゼロ。UBO 登録網(load-bearing)にも非接触。
- **[監査確認]** DrawData buffer は run 内生成(永続キャッシュなし)+ 鍵 bump で全 shader 同時切替 = 新旧 layout の混在 frame は存在しない。
- **[gap なし宣言]** 書き手 5 site・読み手(宣言 2 + member 読み 3 file + injection)・hash 1 site の全数は L1 の grep 網で閉包(`aya_dd|AyaDrawData` repo 全 shader grep + `computeDrawDataSlots|ensureVkDrawDataSlot|drawDataWriteScratch` 全 grep)。
- **[hidden なし宣言]** 落とした項目は全て L8 に記載。
