# rigged skin palette 束縛 再設計(bindless・7/23 退行の作り直し)

> ## ⛔ VOID(2026-07-29・視覚 bisect で反証)
> **本 doc の前提「7/23 SSBO 化で入った skin palette 束縛が飛散の原因」は誤り。** commit 単位の
> 視覚 bisect で 7/23(SSBO 化前)〜7/27 は全て視覚正常、**7/28 `e960bd428f5`(Camera 記録の worker
> 並列化 `AYACameraRecordMT`)で飛散再現**と機械特定。真の根 = **Camera 並列化の category error**
> (1 視点を pool 分割→worker が per-draw 共有資源を破壊)。skin palette 束縛は無罪(serial 下で健全)。
> 撤去 commit = `0eecd7be225`。真実源 = memory `finding_camera_mt_categoryerror_rigfly`。
> **本 doc の「skin 束縛を作り直す」設計は不要 = 実装しない。** 以下は歴史的経緯として残置。

## 0. 位置づけ
7/23 `66ae1fb` の UBO→SSBO bindless 化で入った rigged skin palette 束縛が、混雑で
rig 破壊(別 avatar の palette を掴んで肢体が world 各所へ飛散)を起こす。原因は単一の
バグでなく **重なる機構が全部一致しないと壊れる構造欠陥**。∴ 一点パッチでなく束縛
チェーンを 1 本の機構に作り直す。

## 1. 真のモデル(現状の束縛チェーン・file:line)
1. palette 計算: `LLRenderPass::uploadMatrixPalette`(lldrawpool.cpp:1863/1890/1931 = **3 overload**)
   → `writeObjectSkinUBO`(:1883)が avatar 行列を `sObjectSkinShadow`(llvkloader.cpp:8384 =
   **thread_local**・10560B = mat3x4[110]×2)へ書く。
2. palette 格納: `objectSkinStoreCache`(llvkloader.cpp:8603)→ `skinBindlessStorePalette`(:8432):
   - atomic cursor `sSkinPaletteCursor[f]`(:8443)・上限 `SKIN_ENTRIES_PER_FRAME=1024`(:8444)
     超過 = `BINDLESS_INVALID_SLOT`(:8447・**overflow 実測発火 sk_bl=…/130-3870**)。
   - entry = f*1024+local。sObjectSkinShadow を `sSkinPaletteMapped[entry*10560]`(:8450)へ copy。
   - cache[(avatar,hash)] = entry(:8633)。
   - frame 内 dedup = `objectSkinTryAdopt`(lldrawpool.cpp:1880)だが **3 overload / 全 pass を
     一貫して覆っていない**(overflow が出る = 同一 mesh が pass ごと再格納されている)。
3. draw 記録(pushBatch lldrawpool.cpp:634-657):
   - id = `ensureVkDrawDataSlot` or `drawDataWriteScratch`(:637-642)。
   - skin_entry = `objectSkinLookupEntry(avatar,hash)`(:653)。
   - `writeDrawSkinBase(skin_draw_id=(id==INVALID?0:id), skin_entry)`(:656)
     → `sSkinBaseMapped[f*TOTAL + slot]`(llvkloader.cpp:8662)。
   - draw の `firstInstance = id`(setCurrentDrawDataID :644 経由)。
4. bind: `sSkinBaseSet`(専用 set3・STORAGE_BUFFER_DYNAMIC :3126)を dynamic offset
   `f*TOTAL*4`(:8672)で bind(bindDrawDescriptorSetsOnce :13643)。
5. shader(objectSkinV.glsl:85-95): `aya_base = aya_skin_base[gl_InstanceIndex]`
   → INVALID なら UBO uniform `matrixPalette` に fallback(:50・**二重経路**)/ 有効なら
   `aya_skin_palette[aya_base*stride + joint]`。

## 2. 構造欠陥(= ガンの一帯)
- **D1 二重経路**: SSBO(bindless)と UBO(fallback)の両方が正しくないと壊れる。fallback
  UBO が該当 draw の palette を保持している保証が無い(overflow 時に別 avatar/stale を掴む)。
- **D2 固定 1024/frame + 不完全 dedup**: 3 overload で dedup が一貫せず、混雑で ring 溢れ →
  INVALID → D1 の壊れ fallback へ。
- **D3 freshness が規約依存**: skin base region は生成時 1 回 0xFF のみ(:3231)で **per-frame
  clear 無し**。「毎フレーム全 draw が書き直す」前提が破れれば stale entry(別 frame の別 avatar)。
- **D4 INVALID の扱いが silent-wrong**: overflow/miss で INVALID → 現状は「別 palette で描く」=
  wrong-place 飛散。憲法 fail-closed 違反(silent に誤って描くより、描かない方が正)。
- **D5 firstInstance と skin base slot の不一致余地**: id==INVALID で skin_draw_id=0 に対し
  firstInstance=id の不整合(端例)。

## 3. 不変条件(単一)
> **rigged draw が GPU で実行される時、必ず「その frame・その (avatar,skinInfo)」の palette を
> 読む。overflow-to-wrong / stale / cross-avatar aliasing / 二重経路の乖離が構造的に起こり得ない。
> 満たせない draw は誤って描くのでなく描かない(fail-closed)。」**

## 4. 機構設計(1 本化)
- **M1 単一経路**: shader の UBO fallback を撤去。rigged skinning は bindless SSBO のみ。
  「二重が一致」依存を除去(D1)。
- **M2 frame-wide 単一 dedup**: (avatar,hash)→slot を **frame 単位の単一機構**で 1 回だけ確保・
  充填。全 upload overload / 全 pass はこの単一 adopt を通す(3 overload の dedup を統合)。
  → ring 使用量 = 可視 distinct rigged mesh 数(数百)に収束・現実サイズで溢れない(D2)。
- **M3 fail-closed capacity**: ring は distinct mesh 上限で sizing。真に溢れたら INVALID を
  返し、**CPU 側でその draw を発行しない**(不可視・log)= silent-wrong を構造排除(D4)。
- **M4 per-frame clear**: 記録前に skin base region f を INVALID で memset。書き直されない
  draw は INVALID を読む → M3 で不発行。freshness を規約から機構へ(D3)。
- **M5 slot 一貫**: rigged draw の firstInstance = writeDrawSkinBase が書いた slot と厳密一致
  (INVALID 時は不発行ゆえ端例消滅・D5)。

## 5. product 分岐(要相談・実装前)
M3/M4 の帰結 = **真の overflow / palette 未充填の rigged draw は「その frame 不可視」**(現状は
「誤った場所に飛散」)。**不可視 > 誤描画** が fail-closed の正だが、見た目が変わる product 判断。
※ frame-wide dedup(M2)が効けば普通の人数では overflow せず不可視は起きない想定。

## 6. 実装計画(パッチでなく置換)
1. M2: upload 3 overload の store を単一 adopt 経路に統合(frame-wide dedup 一本化)。
2. M4: per-frame skin base region clear(frame begin で region f を 0xFF memset)。
3. M3+M5: objectSkinLookupEntry が INVALID の rigged draw は record で skip(発行しない)+
   firstInstance=slot 厳密化。
4. M1: objectSkinV.glsl の UBO fallback 撤去(INVALID は不発行ゆえ shader に到達しない)。
   ※ shader 変更 = shader_cache + pipeline_cache.bin クリア必須。
5. 検証: 診断起動で rig 破壊消失 + overflow=0(dedup 実効)+ validation 0。PASS は AYA gate。

## 7. 申告(縮小・解釈・未確認)
- 未確認(実装時 read-before-edit で確定): objectSkinTryAdopt の現仕様 / 3 overload の store 差 /
  firstInstance の実 draw call 経路 / UBO fallback を要する非 bindless 経路の有無(あれば M1 再考)。
- 解釈: 「不可視 > 誤描画」を fail-closed の既定に採る(§5 = 要 AYA 相談)。
- 落とさない: M4 の per-frame clear コストは region f のみ(TOTAL*4B)= 許容想定・実測で確認。
