# sUseOcclusion 引数化 詳細設計(2026-08-11・#24 深部・pass_context doc §4 の実装 Brief)

**目的**: sUseOcclusion(0=off/1=read-only/2=read-write)の**相中 save/0/restore を全廃**し、pass 毎の効果値を引数と `LLCullResult` で運ぶ。static は config 専用に縮退(単一書き手化)。⛔ occlusion は load-bearing(OFF で 21× 悪化)= 純リファクタ・挙動恒等が絶対条件。

## 1. 現状の全数(52 site・全列挙済)

### 書き手(9 スコープ)
| スコープ | site | 分類 |
|---|---|---|
| cvar listener / refreshCachedSettings | llviewercontrol.cpp:719 / llpipelinealloc.cpp:975 | config(残置) |
| **360capture** | llfloater360capture.cpp:447/450/583 | **frame 境界 override(非病理・残置)**: simpleSnapshot→display() を複数 frame 丸ごと包む。display の効果値計算が static(=config)を読むことで伝達路が保存される |
| display gDepthDirty clamp | llviewerdisplay.cpp:970-973/1097 | 相中(廃止→効果値計算へ) |
| display_cube_face | llviewerdisplay.cpp:1423-1424/1452 | 相中(廃止→引数 0) |
| render_hud_attachments | llviewerdisplay.cpp:1546-1547/1598 | 相中(廃止→引数 0) |
| generateSunShadow RAII `LLDisableOcclusionCulling` | llpipelineshadow.cpp:253-268(class)/:943(使用) | 相中(class ごと廃止→引数 0) |
| renderShadow 局所 save/0 ×2 | llpipelineshadow.cpp:554/601・749/915 | 相中(廃止・RAII と二重だった) |
| generateImpostor | llpipelinecapture.cpp:1167-1168/1452 | 相中(廃止→impostor は updateCull 非経由 = LLCullResult 既定 0 で代替) |
| viewerwindow cubeSnapshot | llviewerwindow.cpp:6800/6803/6933 | 相中(廃止・display_cube_face の引数 0 と二重だった) |

### 読み手(相別)
- cull walk: llspatialpartition earlyFail ×3(:1176/:1267/:1345・LLOctreeCull 系 traveler)・updateCull :700・markOccluder :760
- cull 後段: stateSort :1108/:1185・postSort :1393/:1419・doOcclusion :789/:810
- query 層(llvieweroctree): checkOcclusion :1047(<2 で return)・group doOcclusion :1119(>1)・isOcclusionEnabled :1282(`>2` = **恒偽・dead 条件**)
- record: renderGeomDeferred :320(`>1 && do_occlusion`)・renderShadow :793(`>1` — **save/0 スコープ内 = 現行 dead-in-practice。挙動保存対象**)
- debug: renderXRay llspatialpartition:1913(抑制スコープ外の main debug 描画 = config 読みで等価)

### 呼び鎖の全数(seed 検証済)
- updateCull 6 呼び元: display:983(main)/ display_cube_face:1430 / render_hud_attachments:1554 / shadow:1669/1767/1934
- stateSort(camera,result) 7 呼び元: 上記 6 の各 updateCull 対 + **generateImpostor llpipelinecapture:1259(updateCull 非経由)**
- checkOcclusion 4 呼び元: traveler:1172 / stateSort:1107/:1184 / llvocache.cpp:839(idle 相・config 値で正)

## 2. 設計

1. **`LLCullResult::mUseOcclusion`(S32・既定 0 = fail-closed)** 新設。updateCull が seed。updateCull を通らない result(impostor)は既定 0 = 現行の zero スコープと同値。
2. **`updateCull(LLCamera&, LLCullResult&, S32 use_occlusion, bool hud_attachments = false)`**。呼び元の効果値:
   - display:983 → `gDepthDirty ? llmin(sUseOcclusion,1) : sUseOcclusion`(360 override は static 経由で自然流入)
   - display_cube_face / render_hud_attachments / shadow ×3 → `0`
3. cull walk への伝搬 = **走査文脈格納**: `LLSpatialPartition::cull(LLCamera&, S32 use_occlusion)`(現 bool do_occlusion は**本体未使用の死引数と実証済** = S32 に置換)→ traveler ctor に S32 保持 → earlyFail は member 読み。traveler 族は **4 class 全数** = LLOctreeCull / LLOctreeCullNoFarClip / LLOctreeCullShadow(生成 = partition::cull :1571-1583)+ LLOctreeCullDetectVisible(生成 = visibleObjectsInFrustum :1528・main 相 → config 値を渡す = 現行同値)。markOccluder は traveler から → `markOccluder(group, S32)` 引数化。
4. cull 後段の値源 = **`getFrameCull()->getUseOcclusion()` に一本化**。根拠(grab 点 3 site 全数検証): grabReferences(result) は updateCull :673(walk 前)・stateSort :1097(入口)・generateImpostor :1110 で呼ばれ、clearReferences は display :1228/:1489 — **全 scope で getFrameCull() が当該 scope の result を指すことを確認済**(shadow の sun/spot result・impostor の zero-default result 含む)。stateSort :1108/:1185・postSort :1393/:1419・doOcclusion :789/:810 を置換。
5. query 層 = 引数化: `checkOcclusion(S32)`・`LLOcclusionCullingGroup::doOcclusion(camera, shift, S32)`。呼び元: traveler=member / stateSort=getFrameCull 値 / vocache=`LLPipeline::sUseOcclusion`(config 直・idle 相 = 現行同値)/ pipeline doOcclusion=getFrameCull 値。isOcclusionEnabled の `|| sUseOcclusion > 2` は恒偽 = 削除(挙動恒等・申告)。
6. record: renderGeomDeferred :320 → `getFrameCull()->getUseOcclusion() > 1 && do_occlusion`。renderShadow :793 → getFrameCull(= shadow result・seed 0)読み = dead-in-practice を構造どおり保存。
7. static `sUseOcclusion` の書き手 = listener・refresh・360capture のみ / 読み手 = display 効果値計算・vocache・renderXRay・360 保存のみ(いずれも record 相外)。

## 3. gate(機械)

1. grep gate: `sUseOcclusion` の出現が {pipeline.h 宣言・pipeline.cpp 定義・llviewercontrol・llpipelinealloc・llviewerdisplay 効果値計算 1 行・llvocache・llspatialpartition:1913(renderXRay)・llfloater360capture} 以外で **0 件**。
2. `LLDisableOcclusionCulling` の出現 0 件。旧 signature(`updateCull(LLCamera&, LLCullResult&, bool` 2/3 引数形・`cull(LLCamera&, bool)`)定義 0 件。
3. build 収束 → 判定走行: 硬チャネル 0・VKC 沈黙 + **occlusion 実効確認**(VkPerf `occl` 計器が非ゼロ = ON 維持・fps 同帯)→ 層 2 視覚(AYA)。
4. 挙動恒等の根拠 = 本 doc §1 の全数対応表(各読み手の新しい値源が旧実効値と一致することを site 単位で示した)。

## 4. 申告欄(設計時点)

- pass_context doc §4 の「`bool use_occlusion`」は 0/1/2 意味論と不整合のため **S32** に確定(設計深化・意味変更なし)。
- isOcclusionEnabled の恒偽条件 `sUseOcclusion > 2` 削除 = 挙動恒等だが文面変更(全列挙済)。
- 360capture の static override は残置(frame 境界・非病理)。将来 display 引数化まで進める場合は別弾。
- renderShadow :793 の dead-in-practice 分岐は**そのまま保存**(occlusion 復活の判断は product 決裁・本工事は純リファクタ)。
- 規模見積: ~35 site / 8 file(llpipelinecull・llpipelineshadow・llpipelinecapture・llpipelinerender・llviewerdisplay・llspatialpartition・llvieweroctree(.h/.cpp)・llvocache)+ LLCullResult(llcullresult.h)。

## 5. 実装結果(2026-08-11・gate PASS)

- build 2 巡収束(error 1 = 列挙外の第 6 traveler `LLOctreeSelect`。earlyFail を `return false` で上書きする非 occlusion 利用者と確認し 0 を明示)。diff = 15 file / +106 −134。
- 実装自己監査での発見と修正: display の gDepthDirty clamp は旧実装で「restore(render 前)まで」の区間適用だった → 新形は clamp を frame 全体に運んでいて render 相 occlude 判定が変わる欠陥。**旧 restore と同一位置に `result.setUseOcclusion(config)` の re-seed を挿入**して位置構造ごと等価化。
- gate grep: `sUseOcclusion` 残存 = 許可セット(360capture・listener・refresh・宣言/定義・display 効果値計算・DetectVisible config 渡し・renderXRay debug・コメント)と完全一致 / `LLDisableOcclusionCulling` 0 件 / 旧 signature 0 件(test stub 含め更新)。
- 判定走行(2026-08-11・VKC 有効・正常終了): 硬チャネル 0・S1 沈黙・VKC = ACCEPTED 族のみ。**occlusion 実効の機械確認 = occl クエリ発行 10.2万〜14.6万/10s・culled 最大 15,611/window**(OFF 事故の署名 occl=0 + draws 爆発は不在)。fps = AYA 体感で前と同帯(log 直接 A/B は前回 log 上書きのため不成立 = occl 計器で代替)。視覚 OK(AYA)。
- 成果: record 相の sUseOcclusion mutation = リポジトリからゼロ化。static は config 専用(書き手 = cvar listener・refreshCachedSettings・360capture の frame 境界 override のみ)。
