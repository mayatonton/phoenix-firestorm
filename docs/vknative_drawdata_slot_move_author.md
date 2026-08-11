# DrawData slot 移動の author 義務化(mdi_stale/overwrite 根治)

制定 2026-08-11。発見経緯 = layout 統一 gate 走行(list[0] 補完 hunk 初投入)で mdi_stale=55 / mdi_overwrite=4。
機構トレース全記録 = セッション scratch `audit_trace.md`(要旨は本 doc §2)。

## §1 不変条件
**破れている不変条件**: 「persistent DrawData slot の移動(acquire/再 acquire)は、参照より前の author を必ず伴う」。
現行の破れ = `ensureRecordDrawDataSlot`(llvkbucket.cpp:360)が唯一の**無記名 mover**(VKC author も mVkAuthorFrame stamp もせずに slot を動かせる)。
**根治形**: mover が slot を動かしたら record の author stamp を無効化し、既存の author site(freeze / tpl fire)が同 tick 内で必ず再 author する構造にする。author site は増やさない(single-author 規律の維持)。

## §2 機構(トレース済・file:line)
1. freeze(llviewerdisplay.cpp:1001 経由)が record を stamp + 当時内容で author。
2. updateImages(llviewerdisplay.cpp:1043-1056・**freeze の後**)で texture の publish/破棄が起き `vkHeapSlotOrDefault` の値が frame 中に変わる。
3. camera stateSort(:1081)→ rebuildGeom(pipeline.cpp:4804)→ patchGroup → bucket dirty(llvkbucket.cpp:268)。
4. fire 冒頭 rebuild(lldrawpool.cpp:1531)の ensure が内容変化を検出し slot 移動 — 無記名。移動先 = LIFO pool(llvkloader.cpp:12905)の中古 slot(解放は完了 watermark 待ち・FIF=3 → 旧 entry が 4〜14 tick 古い)。
5. fire loop は stamp 済みのため author skip → 参照 = V1 stale(asite=2 rsite=1)。
6. 同 tick 後段の establish(memcmp 一致枝 lldrawpool.cpp:630-634)が同 slot を新 hash で author = V2 overwrite(asite 2→3・victim=参照元=culprit)。
- 描画自体は現状正(移動先内容は acquire 時書込 :12929・fire fixup は現 slot・旧 slot は watermark 前に再利用されない)。警報は規律違反の正しい検出。
- list[0] 補完 hunk は本欠陥のトリガー(merge-split record の slots[0] が時間変化値になり flip 母集団に参加)であって欠陥ではない。

## §3 改修設計
### §3.1 mover の stamp 無効化(根治本体)
`ensureRecordDrawDataSlot`(llvkbucket.cpp): ensure 前後で `mVkDrawDataSlot` を比較し、移動していたら `info->mVkAuthorFrame = 0`。
→ freeze loop(:1408)/ fire loop(:1595)の既存分岐が同 tick 内で再 author する。sMonotonicFrameCount は描画中 ≥1(beginFrame で先行 increment・llvkloader.cpp:5908)のため 0 は「未 author」専用値。
### §3.2 author sequence の関数化(整理・重複 3 site の統一)
現行、freeze(:1413-1419)/ tpl fire(:1602-1609)/ rigged slow(:1781-1791)が
`demandDrawInfoTextures → computeDrawDataSlots → ensureVkDrawDataSlot → mdiAuthorAndCheck` の同一列を重複実装している。
`LLRenderPass::authorRecordDrawData(LLDrawInfo& rec, U8 site) -> bool` に統一(lldrawpool.cpp・宣言 lldrawpool.h)。
- 返値 = ensure の成否。author 実行条件は各 site とも「ensure 成功 ⟺ slot 有効」(llspatialpartition.cpp:4196 の返値式)で従来と同値。
- establish は INV-C 3 分岐 + scratch + params-null を持つ別形のため統一対象外。
### §3.3 rigged slow の dead 分岐削除(整理)
`draw_id = (slot == INVALID) ? 0 : slot`(:1788-1789)は ensure 成功後は dead(成功 ⟹ slot 有効)→ `draw_id = p->mVkDrawDataSlot` に簡約。
### §3.4 紛らわしい frame 変数の対応表(整理・rename はしない)
| 変数 | 意味 | 進み方 |
|---|---|---|
| `LLVKLoader::sMonotonicFrameCount` | 描画 frame 通番 | beginFrame で +1(llvkloader.cpp:5908) |
| VKC `sFrame` | 検証器 tick | 同 beginFrame 直後の frameBegin で +1 = monotonic と 1:1(:5909) |
| `gFrameCount` | viewer 全体 frame | main loop |
| `LLDrawInfo::mVkAuthorFrame` | DrawData author stamp(monotonic) | freeze/fire のみ書く。0 = 未 author |
| `LLDrawInfo::mVkSkinFrame` | skin palette 供給 stamp(gFrameCount) | rigged 経路のみ |

## §4 gate(fail-closed・PASS は AYA のみ)
1. build + 3-path deploy(shader 変更なし)。
2. 診断起動走行: 警報全欄ゼロ + **mdi_stale=mdi_overwrite=0**(V1/V2 = 本不変条件の恒久検出器)+ validation 0 + shamdi 発火継続。
3. 憲法 6: 0 走行は反証標本であって証明ではない。正しさの根拠は §2 の全枝トレース + §5 監査。

## §5 縮小・省略・解釈申告
1. member rename(mVkAuthorFrame 等)は今回見送り(diff 最小化)。対応表 §3.4 で代替。
2. establish の author sequence は統一しない(形が異なる・§3.2)。
3. drawdata pool 枯渇時(acquire 失敗)に「slot 据置 + stored 旧内容のまま新内容 hash で author」される既存エッジは本工事の対象外(C_DRAWDATA_EXHAUSTED が別途警報・発生時は供給乖離が V1/V2 で可視)。
4. 検出器(llvkcontract.*)は無変更(憲法 4)。
5. template 内 raw pointer(mTplRecords)の寿命は record 除去が常に patchGroup/evict 経由で dirty 化されることに依存(現行トレースで成立・本工事で変更なし)。
