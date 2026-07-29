# per-draw bindless 束縛 = 列挙 → メンバー毎 設計改修 計画（根治）

> **⚠️ 訂正(2026-07-29)**: 本計画の発端だった crowd 手足飛散の真の根は per-draw 束縛でなく
> **Camera 記録の worker 並列化(`AYACameraRecordMT`)の category error**(視覚 bisect で `e960bd428f5`
> 特定・撤去 `0eecd7be225`・memory `finding_camera_mt_categoryerror_rigfly`)。per-draw 束縛は serial
> 記録下で健全 = 飛散の犯人ではない。以下の束縛 hygiene 監査は一般改善としては有効だが、
> **「飛散の根治」という動機は無効**(飛散は Camera 並列化撤去で解決済)。

## 0. パターン定義
`aya_draw_id = gl_InstanceIndex = firstInstance = draw_id` で per-draw の側テーブルを引く束縛。
共通の失敗軸 = **(a) slot が取れない/溢れる (b) INVALID/溢れ時の落とし先 (c) freshness が規約依存
(d) 二重経路**。

## 1. 列挙（コードで確定・全数）
shader が `gl_InstanceIndex`/`aya_draw_id` で引く per-draw 資源 = **2 消費者 + 1 共有土台**:

| ID | 消費者 | 実体（shader / CPU） | INVALID/溢れの落とし先 | 深刻度 |
|---|---|---|---|---|
| **F** | draw_id 土台（DrawData） | `aya_tex_slots[aya_draw_id]`（materialF.glsl:306）→ `sDrawDataMapped`。1M slot=PERSISTENT+3×32768 scratch。cursor reset :5563 | **slot 0 = default(灰色)= 安全** | 低（benign） |
| **M1** | skin palette | `aya_skin_base[gl_InstanceIndex]`→ palette ring `sSkinPaletteMapped`。1024/frame | **slot 0 = 別 avatar の palette = 飛散** + UBO 二重経路 | 高（rig 破壊） |

**別軸（この計画の対象外）**: `sPerFrameUbo`/`sCurShadowUtil`/`sCurDeferredUtil`/`sLatched*` =
per-frame/per-program の UBO ring（draw_id 非 index）= redesign §9 H1 の worker↔worker 軸。別 track。

## 2. 共有不変条件（F/M1 共通）
> per-draw 側テーブルは、①発行される全 draw に対し②その frame の正しい値を返し、
> ③満たせない時は **wrong でなく安全 default（benign）** に落ちる。stale/overflow/wrong-avatar が
> 構造的に起こり得ない。

DrawData(F) は既に③を満たす（slot0=default）。skin(M1) は③を満たさない（slot0=別avatar）= 破れている。

## 3. メンバー毎の設計

### F: draw_id 土台（DrawData）★点検完了 = 堅牢・修正不要★
- `ensureVkDrawDataSlot`(llspatialpartition.cpp:4255): 失敗は `drawDataAcquireSlot` が INVALID を
  返す時のみ = **persistent pool(~950k slot)枯渇** = 現実に起きない。→ rendered rigged draw は
  必ず valid slot を取れる。
- ∴ GDB `noslot=1705` = **非描画 DrawInfo の混入(mVkDrawDataSlot 既定 INVALID・未 draw)= 無害**。
  土台バグでない。
- INVALID→slot0=default(灰色・:3190 memset0)で **③(benign)を満たす**。
- **結論: F は修正不要。** 深刻バグは全て M1(skin)。

### M1 真のモデル 補遺（実装前に把握済・置換対象の実態）
- **二重経路が中途半端に同居**: store は SSBO ring(`skinBindlessStorePalette` :8432)と UBO
  (`ensureObjectSkinUploaded` :8473)の両方へ。draw は skin_entry 有効=SSBO / INVALID=UBO fallback。
  :8429 のコメント「bindless は draws に影響しない/shader は UBO を読む」は **A/B 検証当時の遺物**
  (その後 SSBO へ切替えたのに残置)= パッチ放置の物証。
- **cache は per-frame clear**(`objectSkinFrameCacheGuardLocked` :8420 = stamp 比較で clear)=
  cross-frame stale-adopt は無い。∴ M4(per-frame clear)は skin base 側も含め **不要**
  (`writeDrawSkinBase` :8662 が毎 record 書込・stale 生じない)。
- **dedup は objectSkinTryAdopt**(:8560 相当)= cache hit で UBO {buf,off} adopt・store skip。
  ただし SSBO ring cursor は adopt 経路で増えない一方、非 adopt 経路(3 overload の一部)が
  重複 store し得る → overflow。
- **overflow → INVALID → UBO fallback**(内容が該当 avatar とは限らない）+ 二重経路の乖離 =
  rig 破壊の構造要因。

### M1: skin palette（rig 破壊の本体）
- 詳細設計 = `docs/vknative_rigged_skin_palette_redesign.md`（§4 M1-M5）。要点:
  - **M1a 単一経路**: shader UBO fallback 撤去（bindless のみ）。
  - **M1b frame-wide 単一 dedup**: (avatar,hash)→slot を frame 単位 1 回。3 overload の store 統合。
  - **M1c fail-closed = benign**: 溢れ/未充填の rigged draw は **wrong palette で描かず、
    DrawData 同様「安全に落とす」**（描かない or rest-pose）。slot0=別avatar を構造排除。
  - **M1d per-frame clear**: skin base region を記録前 INVALID clear（freshness を機構化）。
  - **M1e slot 一貫**: firstInstance = writeDrawSkinBase の slot 厳密一致。
  - **sizing**: ring = 可視 distinct rigged mesh 上限（dedup 後）。
- ※「不可視 vs 飛散」は product tradeoff で**ない**（充填できる draw は必ず充填・未ロードは今も
  skip・溢れは sizing で消える）。M1c は万一の安全網。

## 4. 実行順・gate
1. **F 点検**（noslot=1705 の原因を file:line で特定・benign か要修正か判定）。
2. **M1 実装**（rig 破壊の根治・M1a-e）。F が要修正なら F-1 を M1 前に。
3. gate = 診断起動で rig 破壊消失 + overflow=0 + validation0 + noslot 由来の異常無。PASS=AYA。
- 各メンバー完了ごとに commit（粗め・トレーラ無し）。

## 5. 申告
- 縮小: UBO ring 群（§9 別軸）は本計画に含めない（draw_id 非 index ゆえ別病理）。
- 未確認（実装時 read-before-edit）: noslot=1705 の構造原因 / UBO fallback を要する非 bindless 経路 /
  ensureVkDrawDataSlot の失敗条件。
- 解釈: 「INVALID→benign default」を共有不変条件の③に採用（DrawData の既存挙動を基準に skin を揃える）。
