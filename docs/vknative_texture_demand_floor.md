# texture 需要フロア(demand floor)実装 Brief — 「描かれた texture は餓死しない」

制定 2026-08-11(AYA 設計合意・missing 再武装は AYA 裁定で対象外)。発見経緯 = DrawData layout 統一 gate の視覚 NG(透過 mesh 灰色固定・edit 選択でのみ復活)の根本解析。

## L0 不変条件と範囲
**破れている不変条件**: 「描画に使われている texture には fetch 需要が供給される」。需要(mMaxVirtualSize)は描画実態と切断された推定器(face 登録リスト × 幾何推定)のみが供給し、推定器の失敗 = 需要 0 = fetch 恒久停止が無警報で成立する(生成源 a/b/c 下記)。
**修理形**: 需要の下限を描画事実から供給する — 「このフレームに描かれた DrawInfo の texture 閉包のうち、**1 mip も持たない**(discard<0)texture は需要 ≥ DEMAND_FLOOR を得る」。推定器は上積み供給者となり、失敗しても餓死が構造的に不可能になる。
**対象外(AYA 裁定 2026-08-11)**: ①missing-asset の再武装(ASSET_RETRY_LIMIT=8 連敗 = 恒久 missing は現行維持・region 変更復活 :2271-2281 のみ)②terrain/water/sky/UI/bake 等の独自需要系(表 L2 の #7/#8 = boost bypass or 独自 feed で本病理の外)③推定器自体の欠陥修理(feed が無害化・検出器が名指しした時に個別工事)。

## L1 前提事実(トレース済・file:line)
| 事実 | file:line |
|---|---|
| 餓死の単一根: vsize≤10 → desired=MAX_DISCARD+1(desired>max は同一状態の表裏) | llviewertexture.cpp:3266-3271 |
| make_request 恒久 false の gate 群 | llviewertexture.cpp:2284-2309 |
| 需要の唯一の推定器 = 全 7 channel face list の pixel-area 走査 | llviewertexturelist.cpp:1065-1148 |
| 生成源 a: rigged calcPixelArea の stale return false(mPixelArea 未更新・stamp なし) | llface.cpp:2383-2391 |
| 生成源 b: DrawInfo は rebuild まで旧 texture を描き続けるが走査は現在の face 登録のみ見る | llspatialpartition.h:154 vs llviewertexturelist.cpp:1070- |
| 生成源 c: TP/ログインの一括リセット(desired=MAX+1 + vsize=0) | llviewertexture.cpp:2443-2454・llviewertexturelist.cpp:995-1011 |
| boost ≥ HIGH は推定器を bypass(edit 選択で直る機構) | llviewertexturelist.cpp:1023 |
| addTextureStats = max 合成(floor は推定器と自然共存) | llviewertexture.cpp:884-898 |
| データ到達後の描画復帰は自己修復済(publish → 消費時 ensureVkSlot) | llimagegl.cpp:1543-1567 |
| PBR texture の face 登録(需要側は他クラスと同一推定器) | llvovolume.cpp:6767-6771 |
| PBR camera の消費 = 束縛経路(DrawData 不通過)・PBR DrawInfo mTexture=null | llfetchedgltfmaterial.cpp:90-126・llvovolume.cpp:6185-6189 |
| NaN は餓死の犯人ではない(llmin/llmax 比較 false → max 側飽和) | llviewertexture.cpp:892 |
| 検出済の既存 oracle: stuck 述語(修理後は沈黙が正常 = 本欠陥族の恒久検出器化) | llviewertexture.h:396-401・llviewertexturelist.cpp:871-907 |

## L2 機構設計
### L2.1 共通 helper(新設・1 本)
`LLRenderPass::demandDrawInfoTextures(const LLDrawInfo& info)`(置き場 = lldrawpool.cpp・static):
- 閉包 = mTexture・mTextureList 全要素・mNormalMap・mSpecularMap・mGLTFMaterial の {mBaseColorTexture, mNormalTexture, mMetallicRoughnessTexture, mEmissiveTexture}。
- 各 texture t について: `t != null && t->getDiscardLevel() < 0` のときのみ `t->addTextureStats(DEMAND_FLOOR)` + VkPerf 計器 `tex_floor_feed++`。
- `DEMAND_FLOOR = 4096.f`(64×64・SHADOW_DEMAND_FLOOR llviewertexturelist.cpp:1021 と同前例)。餓死分岐の閾値(10)を確実に超え、かつ desired 計算上は最低位 mip 相当 = streaming 影響は「無データ texture に最低 mip を取らせる」のみ。
- **discard<0 限定の意味**: 1 mip でも取れたら推定器統治に戻る = 定常状態で feed は発火ゼロに収束(遠景の正当な小需要には介入しない)。
### L2.2 呼び点(DrawInfo を持つ描画 choke 全数)
1. `freezeAuthorShadowSources` の author_one(影 bucket 族・毎 frame)
2. `pushIndirectBucket` tpl fire の self-author 分岐(camera MDI 族)
3. `establishPerDrawId` — ただし**早期 return(INHERIT)より前**に置く(blend material 等 heap/skin 非使用 shader の draw も被覆するため呼び手 `drawInfoBindless` 入口に置く方が明快 → 実装判断は「全 DrawInfo draw が必ず 1 回通る集合」を実装時に grep で確定し申告)
4. `pushRiggedBatchesIndirect` slow path(rigged)
5. `pushBatch` / `pushUntexturedBatch` の非 MDI fallback 経路
- 重複呼びは無害(max 合成 + discard<0 条件で 2 回目以降は同値)。dedup は入れない(条件自体が定常ゼロ)。
### L2.3 検出器(既存装置の判別力強化・憲法 4 承認対象)
- `fetchRetryStuckInfo`(llviewertexture.cpp:1610-1617)に判別欄を追加: `vsize / desired / max_discard / needsCreate / cb / fastcache / missing`。
- 位置づけ: 修理後、既存 AssetStuck stuck 行は「餓死族の fail-closed 検出器」になる(**沈黙が正常**)。発火時は判別欄が枝を名指しする。
- VkPerf `tex_floor_feed`(INFO 計器・alarm ではない)で feed 量を常時観測可能に。

## L2.4 描画中 texture の retry 間隔 cap(AYA 裁定 A・2026-08-11)
- 発見: demand floor 後の gate 走行で餓死は解消(stuck 0)したが、CDN 403 の retry ladder(4→512s 指数・8 回完走 ≈17 分)の**待機中**が灰色として残存(35 texture × per-asset 決定的 403 = 再現性 100% の正体)。
- 裁定: 回数上限 8・画面外 texture の現行 ladder・8 敗後 missing は**不変**。「描かれている(= floor が需要を入れた)texture に限り」retry 間隔を `ASSET_RETRY_DRAWN_CAP_SEC = 32s` で頭打ち(llassetretry.h)。決着最長 ≈17 分 → ≈3 分。総リクエスト数は増えない(間隔のみ)。
- 実装: feed が `noteDrawnDemand()`(5 秒窓の印・llviewertexture.h)→ updateFetch の backoff gate と sweepFetchObligation の同 blocker に cap 条件(llviewertexture.cpp)。

## L3 gate
1. build + 3-path deploy(shader 変更なし = shader deploy 不要)。
2. AYA 通常走行(特殊シーン指定なし・再現性の低い症状に依存しない):
   - 警報全欄ゼロ(default-deny)+ **AssetStuck stuck texture 行 = 0**(本欠陥族の機械 gate)
   - VkPerf `tex_floor_feed` の推移を報告に記録(ロード期に発火 → 定常でゼロ収束が期待形)
   - DrawData layout 統一と同走行で gate 可(mdi 系条件は layout Brief L7 のまま)
3. 視覚(AYA・最終): 灰色固定の不再現は再現性が低いため「発生しないことの証明」には使わない(憲法 6)— 視覚 gate は通常の描画同一性確認。
- 万一 stuck 行が出た場合: 判別欄(L2.3)が枝を名指し → 推定器の当該欠陥(a/b/c)の個別工事へ(feed により餓死はしないので視覚は保たれる = 灰色でなく低 mip 表示)。

## L4 縮小・省略・解釈申告
1. missing 再武装 = 対象外(AYA 裁定・8 連敗は救済しない)。
2. 推定器の欠陥 a(llface.cpp:2383-2391 の stale return)自体は修理しない(feed で無害化・検出器が名指しした時に個別対応)。
3. 独自需要系(terrain/water/sky/UI/bake/sculpt/media/light)は対象外 = 表の根拠つき(boost bypass or 独自 feed)。
4. addTextureStats は mutable 書き = 現直列 main で安全。**再並列化時は feed を凍結相へ移すか atomic max 化**(並列化台帳へ追記する)。
5. DEMAND_FLOOR で取る最低 mip の分だけ「灰色 → 低解像度表示」に変わる = 視覚挙動の改善方向の変化(product 上は灰色より正・AYA 合意済の設計意図)。
6. LLViewerTexture 基底に discard 概念がない型(MEDIA_TEXTURE 等)が閉包に混ざる場合は skip(実装時に型判定を申告)。

## L2.5 missing asset の描画側除外(AYA 合意 2026-08-11)
- **不変条件**: 「missing 確定 asset は描画側の texture 解決機構(需要 feed・heap slot 解決試行)の対象外」— データ不在が既決の texture への毎 frame 再試行(ensureVkSlot 試行 + fb_heap_default note + feed)を構造的に排除する。
- 発見経緯 = fix 後走行で tex_floor が 1.7〜2.5M/10s 高止まり(runA 定常 0)。missing は永遠に publish されないため、描かれている限り feed / fallback 経路が毎 frame 発火 = 「定常ゼロ収束」が missing 在視界シーンで原理的に不成立だった(設計の言い落とし)。
- 実装: ①feed 条件に `!isMissingAsset()` を追加 ②computeDrawDataSlots の heap slot 解決を helper `drawSupplyGL` 経由にし、`missing && !hasGLTexture()` の texture は nullptr 扱い(供給値は従来と同一の default 白 = mdiHash 不変・視覚不変。消えるのは解決試行と note のみ)。
- **保守形の理由**: 除外は `missing && GL 無し` に限定 — 万一「missing だが過去の低 mip GL を保持」という状態が存在しても実 texture 供給を維持(視覚後退ゼロ)。
- region 変更での missing 復活(llviewertexture.cpp:2284-2292 = mIsMissingAsset false 化)で feed / 解決対象へ自動復帰 = 恒久ブラックリストではない。
- fetch 挙動は不変(missing は元々 make_request gate で停止)。
- **副作用申告**: fb_heap_default の母集団から恒久 missing 分が消える(counter は transient loading 専用に戻る = 信号純度は向上・upload worker gate の「有意減」読み値の基準が変わる)。検出器コード(llvkcontract.*)は不触。
- 性能: 非 missing texture に virtual call 1 回/解決が加わる(未計測・数 ns 級と見積)。missing texture からは ensureVkSlot 試行 + note(string copy)が消える。

## L5 設計自己監査
- **[検証済]** feed は max 合成(:894-897)なので推定器と競合しない・floor は餓死分岐閾値 10 の 400 倍で確実に脱出。
- **[検証済]** discard<0 条件により、fetch 済 texture への介入ゼロ = streaming 均衡を変えない(変えるのは「何も持っていないのに描かれている」異常状態のみ)。
- **[検証済]** missing-asset は make_request gate(:2294)が feed より優先 = 8 連敗後に feed が空回りしない(需要はあるが要求は出ない・AYA 裁定どおり)。
- **[リスク・申告]** 呼び点 L2.2 の「全 DrawInfo draw が必ず 1 回通る集合」の確定は実装時 grep に依存 = 漏れたクラスは餓死が残る。完了報告に呼び点全数と各描画クラスの対応表を必須添付。
- **[hidden なし宣言]** 落とした項目は全て L4 に記載。
