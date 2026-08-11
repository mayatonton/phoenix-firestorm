# gFrameCount memo sweep(2026-08-11・工事列第 7 弾・gate PASS)

**目的**: just-in-time pattern の残り sweep = `== gFrameCount` 型 freshness memo の機械 grep 全数列挙(rigged fast path と同族の粒度不一致 = 表示 frame 粒度 memo が VKC tick 粒度の契約を破る族の狩り出し)。母集団 = `gFrameCount` 全 57 site / 22 file(extern 宣言除く)。

## 結論

- **病理族 = 1 件のみ**(rigged shadow MDI fast path の skin base publish)→ 根治済(下記)。
- benign = 残り全部(表示 frame 粒度が正しい意味論)。逆向き不一致(tick memo で表示 frame 意図)= 0 件。

## 根治した 1 件

`lldrawpool.cpp` pushRiggedBatchesIndirect の `mVkSkinFrame` memo(旧 :1755/:1805)。

- Stage 0 で α 再表明(`mdiAuthor` per call)は根治済。残っていたのは `publishDrawSkinBase` の skip が表示 frame 粒度だった点。書き先 `sSkinBaseMapped[sFrameIndex]`(llvkperdraw.cpp writeDrawSkinBase)は **per-FIF 資源**で、sFrameIndex は beginFrame 毎(= tick 毎)に前進(llvkloader.cpp `++sMonotonicFrameCount` / `sFrameIndex+1` 隣接 = lockstep)。
- 現行構造で発現しない根拠(トレース済): 表示 frame 内の rigged shadow MDI 記録は全て main tick に同居する。probe face も shadow を生成する(display_cube_face → generateSunShadow・riggedMdiEligible は gCubeSnapshot を見ない)が、probe 描画は main tick 内で走る(display() → mReflectionMapManager.update → llreflectionmap.cpp cubeSnapshot → display_cube_face・間に beginFrame なし。beginOffscreenFrameVk は probe 不使用かつ f 非前進)。
- ただしこの安全性は「同一表示 frame の記録 = 同一 tick」という**ソース強制のない暗黙不変条件**依存で、再並列化・tick 構造変更で発現する(症状 = skin base stale = skinning 小振幅崩れ = S1 オラクルの盲点域)。
- **fix**: memo の鍵を `gFrameCount` → `LLVKLoader::getMonotonicFrameCount()`(U32・2 行)。memo 粒度 = 資源粒度(tick)に構造一致。同 tick 内(カスケード間・probe 同居)の skip は維持 = 挙動同一、tick が分かれたら自動 republish。
- 申告: 番兵 `0xFFFFFFFFu` と monotonic の衝突は U32 wrap 時のみ(連続稼働 ~4.5 ヶ月で 1 tick)= 実運用外・旧鍵と同クラス。

## benign 判定(全数・file:line)

| memo | site | 意味論 |
|---|---|---|
| mVkLastFireFrame | 書 lldrawpoolalpha.cpp:1275 / llpipelinecull.cpp:1427・読 llviewerobjectlist.cpp:1083 | +60 frame hysteresis = LOD 更新スロットル(表示 frame timebase が正) |
| occlusion queued | llpipelinecull.cpp:760-775・llvieweroctree.cpp:1171 | per-camera の query 発行スロットル。mOcclusionQueuedFrame 読み手 = dedup 自身のみ。mOcclusionIssued 読み手 = getLastOcclusionIssuedTime(llvieweroctree.cpp:1040)= **呼び出し元ゼロ = dead accessor**(削除は別途・⛔ occlusion は load-bearing につき本 sweep では無接触) |
| mVkRebuildVisitFrame | 書 llvovolume.cpp:6523・読 llviewerobjectlist.cpp:1072 | 診断 log(rbage)のみ |
| MatrixPaletteCache.mFrame | llvoavatar.cpp:10941/10955 | CPU palette 再計算 = animation timebase(表示 frame が正)。GPU upload freshness は別 memo(llvkperdraw.cpp:2041 `sObjectSkinUpFrame[f] != sMonotonicFrameCount \|\| gen` = tick+世代)+ per-tick cache guard(objectSkinFrameCacheGuardLocked = sMonotonicFrameCount)で正しく実装済 |
| 非描画系 | llstartup / llviewerstats / llappviewer / llperfstats / llscenemonitor / fsfloaterperformance / llgroupmgr / llviewertexturelist(>10f throttle)/ llheroprobemanager(面配分)/ llviewerdisplay:997(起動 guard) | stats・スロットル・スケジューリング = 表示 frame timebase が正 |

- newview 側の monotonic 鍵 memo = lldrawpool.cpp:1614(mVkAuthorFrame)のみ = 正しい tick 粒度。llrender 内 sMonotonicFrameCount 族 = 全部 tick 粒度で正。

## gate(2026-08-11)

build incremental 0 error → 判定走行(新 binary・VKC 有効・6 分・正常終了)= 硬チャネル 0・S1 沈黙・VKC = fb_heap_default 族(ACCEPTED)のみ・rig/skin 名指し sweep 未承認 0(MESHSKININFO ×11 = ACCEPTED 壊れ asset)。供給実働 = rig 27k/10s・skin_up 8.7k・sk_base 84.6k・sk_bl fail 欄 0。層 2 視覚 OK(AYA)。

- 残 OPEN(非接触・network 族・AYA 裁定持ち越し): `_httppolicy:404` Forbidden(Http_403)×746 / `cap not found` parse ×23(本走行実測。allowlist 現行パターン不 match・灰色 texture 監査の残 OPEN と同族)。
