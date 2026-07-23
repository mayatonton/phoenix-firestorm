# DrawCount 削減 — 最上設計(rigged 保持型 indirect 化・全 pass)

> 位置づけ: per-draw 描画記録 回復フェーズの本丸。AYA 指示(2026-07-24)=「実コードと現在のログから DrawCount 削減の対象を最大コスト投入で最上設計に落とす」。真実源 = 本 doc(実測 + 実コードトレース)。前段 = `docs/vknative_rigged_indirect_design.md`(B.0-B.2 skin bindless 基盤)/ `docs/vknative_perdraw_record_recovery_design.md`(3戦略)。統治 = `docs/vknative_recovery_plan.md`。
> **状態 = 段階1/1.5 実装+commit 済(`8135295627`・視覚 gate PASS・`AYASTORM_RIGGED_MDI` 既定 OFF)。⚠️ ただし §9 の AYA realization で本 line は closed = frame は compute-bound でなく serialization-bound ゆえ DrawCount 削減の新標的は追わない。次軸 = CPU-GPU overlap(pipelining)。**

## 0. 実測(現ログ mine・154 frame 累積 → per-frame・fps 30.65 / avg_ms 32.63 / draws/f 15518)

### 0.1 DrawCount 構成(per-frame)
| pass | draws/f | 比率 | 記録経路 |
|---|---|---|---|
| **scene(camera)** | 7309 | 47% | static=MDI 保持 / rigged=per-draw |
| **shadow(sun 4 cascade + spot)** | 4301 | 28% | static=MDI 保持 / rigged=per-draw |
| occl | 1914 | 12% | 深度 bbox(安い) |
| probe(reflection) | 1994 | 13% | scene 再記録(cube) |
| **合計** | **15518** | | |

- sun cascade 別 `shmap` = **1747 / 1498 / 187 / 738 /f**(cascade 0/1 が主・2/3 は遠く少ない)+ spot 66/f。

### 0.2 高コストの per-draw = **shadow rigged**(static は保持型で安い・scene/probe rigged は誤差)
- **`dyn`(MDI 非畳込 = フル per-draw 記録)= 1942/f。うち shadow rigged が支配**。counter を実コードまで追った確定内訳(`e3RigBucket()` → `gVkPerfPassTag`・`lldrawpool.cpp:139` / tag=1 は `generateSunShadow` `pipeline.cpp:13932`):
  - **shadow rigged(`e3 rig` bucket1)= 1457/f・3.28ms/f = frame の 10%** ← **唯一の実標的**
  - scene rigged(bucket0)= **53/f・0.19ms/f = 無視できる**
  - probe rigged(bucket2)= ~0・0.13ms/f = 無視
  - ⚠️ **訂正(2026-07-24)**: 当初「非 rigged dyn = ~432/f・小」と書いたのは算術ミス(dyn から rigged を誤って減算した)。**rigged は bucket 経由でない(pushIndirectBucket を通らない)ゆえ mdi_dyn に入らない** → `dyn`(mdi_dyn)= **~1977/f が純 static 非畳込**(is_static 条件 `llvkbucket.cpp:373-380` を外れた draw = 自前 model 行列〔移動/回転 prim〕/ texture_matrix / slice 無効)。**rigged と同機構(per-draw model 行列 bindless)で畳める候補だが、下記 §9 の結論により追わない。**
- **検算**: `rigged_rec`(全 pass 合算・`llvkloader.cpp:5218` fam `rig`)= 1510/f = scene 53 + shadow 1457(`vkShadowCullBatch` は `pushBatch`/`pushUntexturedBatch` 内で呼ばれ shadow rigged を `rigged_rec` に**二重計上**)。log の `rigged=224384`(shmap 欄)= **`shadow_rigged` counter**(`llvkloader.cpp:5061`)= 1457/f。
- **⚠️ READINESS 訂正**: 前セッション「B.3 = 3.4ms = 低配当」は **数字は正しいが scene とラベルを取り違えていた**(真は shadow rigged = 3.28ms・bucket1)。scene rigged は 0.19ms で誤差。∴ **DrawCount で削れる per-draw は実質 shadow rigged の 3.28ms のみ**。static は保持型で既に安い。

### 0.3 fam pool 時間(camera pass・上位)
- `mat` = 2.07ms/f(1153d)/ `alphaPost` = 1.86ms/f(470d)/ `e3 rig` = 3.28ms/f。
- ⚠️ `light`(deferred lighting = per-light・非 DrawCount)= 5.8ms/f・`lgt fwd`/`misc` が主 = DrawCount 標的でない(別軸)。

## 1. 構造の確定(実コードトレース・全 file:line)

### 1.1 static は既に「保持型 indirect」= 最適・触らない
- `pushIndirectBucket`(`lldrawpool.cpp:1313-1368`)= **template(`mTplCommands`)は `mTplDirty` 時のみ rebuild**(`rebuildTemplateIfDirty` `llvkbucket.cpp:340`・dirty = group/region の add/remove `:213/225/264/290`)。
- **per-pass は template を ring へ memcpy + `instanceCount=0` cull だけ**(`:1354-1368`・vis_bits + cull radius)。= scene/shadow/occl/probe が**同一 template を共有**、pass ごとに可視性を toggle するのみ = 再記録なし = 安い。
- ∴ **static の DrawCount は既に畳まれている**(`mdi rec`=6095/f が few `mdi call`=1573/f に collapse・`zero`=20847/f は cull 済 slot)。

### 1.2 rigged は保持されず全 pass で per-draw 記録 = 唯一の標的
- scene: `renderGeom` → `pushRiggedBatches`(`lldrawpool.cpp:1657` → per-draw `pushBatch`)。
- shadow: `renderShadow`(`pipeline.cpp:13541` の `j==1` rigged loop)→ `renderObjects(type, rigged=true)`(`:8969`)→ `pushRiggedBatches(type+1)`。**4 cascade × 各 renderShadow が `updateCull`+`stateSort`+rigged per-draw 記録**(`pipeline.cpp:13514-13563`)。
- probe: cube snapshot pass で同 `pushRiggedBatches`。
- rigged が保持型に入らない根本理由(`docs/vknative_rigged_indirect_design.md` §0.3)= **skin palette が per-draw dynamic UBO(binding 46)だった** → **B.0-B.2 で bindless SSBO + per-draw base index 化済**(`aya_skin_base[gl_InstanceIndex]`)= **保持型化の前提は既に満たされている**。

### 1.3 shadow rigged は scene rigged より畳みやすい(depth-only・少 pipeline)
- shadow rigged が使う pipeline = `gDeferredShadowProgram` / `ShadowAlphaMaskProgram` / `ShadowFullbrightAlphaMaskProgram` / `TreeShadowProgram` / `ShadowGLTFAlphaMaskProgram`(`pipeline.cpp:13544-13651`)= **深度専用 3-5 本**。opaque は texture bind すら不要。
- scene rigged = per-material の多数 pipeline(SIMPLE/FULLBRIGHT/MATERIAL/SPECMAP/…× rigged)。
- ∴ **pipeline span が少ない shadow rigged の方が MDI collapse の chunk span が細切れになりにくく、畳込効率が高い**。per-pass cull 機構も既存(`vkShadowCullBatch` `lldrawpool.cpp:494`)。

## 2. 不変条件と核心変換(1 文)

> **不変条件(現状)**: 「rigged draw は自 avatar の skin palette を per-draw で bind し、pass ごとにフル per-draw 記録される」。
> **変換**: rigged を static と同じ**保持型 indirect**に載せる = **rigged 専用の template を frame ごとに 1 回だけ構築(dirty 時のみ)し、全 pass(scene / 4 shadow cascade / probe)で共有・per-pass `instanceCount=0` cull で可視性 toggle。毎フレーム更新するのは skin base index(slot→palette entry)だけ**。skin palette 値は B.2 の bindless SSBO を引く。**発行畳込のみ・描く物・pose・palette 値は byte 同一(品質不変)**。

## 3. 設計 = rigged 保持型 indirect(static の対称形 + 毎フレーム skin refresh)

### 3.1 rigged template(static `Bucket` の対称形)
- rigged draw を (VkBuffer, index buffer, index type, **pipeline**) で chunk span に束ね `VkDrawIndexedIndirectCommand`(`indexCount` / `firstIndex` / `vertexOffset=slice.first` / `firstInstance=mVkDrawDataSlot`)を構築。static の `llvkbucket.cpp:410-448` が雛形。
- ⚠️ **static collapse に相乗りしてはいけない**(`docs/vknative_rigged_indirect_design.md` §B.3 訂正済): static の is_static 条件は `mModelMatrix==region_matrix`(全 draw が region 行列共有)前提だが、**rigged は model 行列を使わない**(`materialV.glsl:147-149` = skinning=world 空間 bone 行列 + global modelview 一度)。→ **rigged 専用 collapse を新設**(global modelview 一度 push・region 行列前提なし)。
- dirty trigger = **rigged draw 集合の変化**(avatar/attachment の add/remove/LOD/detach)。それ以外は template 再利用。static の group slot 機構(`mVkBucketSlots`)を rigged drawable にも適用。

### 3.2 毎フレーム skin refresh(rigged の唯一の per-frame 仕事)
- rigged は animate ゆえ skin palette は毎フレーム変わる。だが **VB/index/pipeline/slot は安定**。
- 毎フレーム軽量パス = rigged DrawInfo を舐め **①`uploadMatrixPalette`(palette 充填 dedup・既存)+ ②`writeDrawSkinBase(slot, entry)`(slot に base index 書込・B.2 API)のみ**。描画記録なし。
- = 今の `pushRiggedBatches` の per-draw の「安い部分(palette 充填 + base 書込)」だけ残し、「重い部分(bindFast / `buildAndOverrideScenePerDrawSet` / setBuffer / drawRange)」を MDI 一括に置換。
- **skin refresh は全 pass で 1 回だけ**(scene/shadow/probe が同じ palette buffer + base index を GPU で読む)= 現状の「pass ごとに palette 再充填」も消える(scene 1回 + shadow 4回 + probe = 6回 → 1回)。

### 3.3 per-pass cull(可視性 toggle)
- static と同機構: template を ring へ memcpy → pass の可視 rigged 以外を `instanceCount=0`。
- rigged の可視性源 = 各 cascade の cull 結果(avatar が cascade frustum 内か)+ 既存 `vkShadowCullBatch`(radius cull)。per-avatar 粒度の vis_bits。

## 4. 段階(shadow rigged 単一標的・各単独 gate・直列)

> ⭐ **標的は shadow rigged のみ**(scene rigged 0.19ms / probe 0.13ms は計器で誤差と確定 = 追わない)。shadow rigged は depth-only 少 pipeline で畳みやすく、配当 3.28ms(軽シーン)+ dense crowd の freeze 主因。

### 段階 0 — per-cascade shadow rigged 計器(実装前・cheap)
- `shadow_rigged` を cascade 別に分割(`shadow_rigged_map[6]`・`draws_shadow_map[]` と同機構 = `gVkPerfShadowMapIndex`)+ perf 行に表示。= 段階 1 の配当を cascade 別(cascade 0/1 が draw 支配 = 1747/1498)に定量化 + **dense crowd で shadow rigged の膨張を後追い採取**。憲法 4: 計器増設は盲目化でない = 可・diff は AYA gate。
- **B.4-a 掃除に相乗りビルド → AYA cold launch 1 回で ①B.4-a 視覚 gate ②本計器の crowd 採取**。

### 段階 1 — shadow rigged を MDI 畳込(本命)
**方式 = 新 template でなく、既存 B.3 scaffold(`pushRiggedBatchesIndirect`)を shadow rigged へ有効化**(実証済コード再利用・下記 §8 JIT 詳細設計)。scaffold は per-call rebuild-but-indirect = **高コストな per-draw `pushBatch`(bindFast/set/drawRange)を per-span 1 回の bind + `vkCmdDrawIndexedIndirect` に畳む**。skin base 書込・firstInstance=slot・span 束ねを自己完結で持つ。
- gate = 影の視覚同一 + validation 0 + device-lost 0(別担当)+ `shadow_rigged`→~0 + `e3 rig` bucket1 / `ph shad`↓ + **dense crowd fps(main 飽和防止 = 北極星)**。
> 註: §3 の「完全 retained template(frame 1 回構築・全 cascade 共有)」は 段階 1.5 の任意最適化。scaffold は per-cascade で command list を再構築するが、command build は安い(struct fill)ゆえ配当の大半(高コスト bind の消去)は段階 1 で得る。retained 化は per-cascade rebuild が実測で律速なら着手。

### 段階 2 — 掃除(段階 1 gate 後)
- 旧 shadow rigged per-draw 経路・工事用 kill switch 撤去。
- **落とす(計器で誤差と確定)**: scene rigged(0.19ms)・probe rigged(0.13ms)の保持型化は配当ゼロゆえ**やらない**(B.3 の scene scaffold `pushRiggedBatchesIndirect`/`AYASTORM_RIGGED_MDI` も撤去候補)。
- **B.4-b(UBO fallback binding46)は velocity/impostor 等の未配線 pass が残る限り恒久残置**(安全網・§6)。

## 5. gate 基準(共通)
- ⚠️ **A/B オラクルは退役済**(`a9b4cdecbf`・device-lost 真犯人 = 二重 skinning)。**検証は視覚同一 + validation 0 + device-lost 0**。byte 検証装置が要るなら二重計算でない軽量版(CPU 側 palette entry 照合)を別途設計 = 段階 1 着手前に要否を AYA 判断。
- VkPerf: `shadow_rigged`/`rigged_rec`/`dyn`↓ + `mdi_call`/`mdi_rec`↑ + `e3 rig`/`ph shad`↓ + **dense crowd(50-100av)で main 飽和 = freeze しない**(北極星直撃)。
- **品質トレード禁止**(発行畳込 = lossless・cascade 間引き/LOD 削減ではない)。cascade 数削減・shadow 解像度は product 決裁(触らない)。
- gate 様式 = 命題 + 未証明項併記 + 実効設定確認欄(RenderShadowDetail / SSR / spot preset 実値)。

## 6. 罠・申告(実装前に潰す)
- **(a) rigged template の dirty 粒度**: avatar 移動(spatial group 遷移)・attach/detach・LOD で dirty。頻度が高いと template 再構築が per-draw に退化 = 実測で dirty 率を確認(段階 0 計器)。crowd で avatar は常時 animate だが **group 遷移は歩行速度依存**(毎フレームではない)= 再利用は効く見込み(要実測)。
- **(b) skin base の ring 世代**: B.2 で base buffer ×FRAMES_IN_FLIGHT + dynamic offset 化済(`a9b4cdecbf`)。全 pass 共有でも同 frame の base を読む = 整合。
- **(c) alpha rigged の順序**: MDI は順序保持が要る(back-to-front)= 段階 2 の alpha は最後・pipeline span を深度順に。
- **(d) probe/velocity/impostor pass の skin 充填漏れ**: `allow_dedup=false`(velocity)は `objectSkinStoreCache` を呼ばず SSBO 未充填 → base INVALID → UBO fallback(binding46)。保持型化しない pass は UBO fallback 恒久残置(安全網・削除しない)。
- **(e) VB の mega-buffer 相乗り**: rigged mesh VB は `megabufAcquireVertex` で mega slice 相乗り済(`llvkloader.cpp:8550`)= chunk span は typemask 別 chunk 数(perf `mega chunks=68`)が上限 = 畳込効く。
- **(f) 検証装置なしの経路移行リスク**: A/B オラクル退役後ゆえ視覚 gate 依存。段階 1(shadow)は depth-only で視覚差が影のみ = 検証しやすい方から入るのは正当。
- **申告(縮小・省略)**: probe rigged(段階 3)は配当小ゆえ後回し可・UBO fallback は削除せず残す・cascade 数と shadow 品質は触らない(product 決裁)・per-cascade rigged 配分は段階 0 計器で確定してから段階 1 の期待値を確定(現時点は sun 合算 1457/f のみ確定)。

## 7. 期待配当(概算・未実測項は段階 0/gate で確定)
- **shadow rigged 1457/f・3.28ms/f(frame の 10%)** を保持型化 → **few MDI call + frame 1 回の skin refresh** に collapse。軽シーン fps = 30.65→~33(控えめだが小さくない = AYA 裁定 2026-07-24)。
- **真価 = dense crowd の freeze 予防**: shadow rigged は **avatar 数 × 可視 cascade 数**で線形増 → 50-100av で 10-20ms/f に膨張 → main 飽和 = 北極星(50-100av で固まらない)の主因。保持型化(記録を frame 1 回の skin refresh に落とす)がこの freeze の構造治療。
- ⚠️ **軽シーンで 60fps を狙う道ではない**(軽シーンの支配は `light` deferred lighting 5.8ms・`disp`・GPU 供給であって draw 記録でない)。DrawCount 削減は crowd-freeze 予防の標的。dense crowd での実配当は段階 0 計器 + 混雑会場で定量化。

## 8. 段階 1 JIT 詳細設計(実装直前・全 file:line トレース済・HEAD `a9b4cdecbf`+B.4a)

**方式 = 既存 B.3 scaffold `pushRiggedBatchesIndirect`(`lldrawpool.cpp:1514`)を shadow rigged へ有効化する最小改修。新 template・新 buffer なし。**

### feasibility(実コードで確定)
- rigged shadow shader `gDeferredSkinnedShadowProgram`(`llviewershadermgr.cpp:2956-2962`)= `shadowSkinnedV.glsl` + `hasObjectSkinning=true` → **objectSkinV(bindless SSBO 版・global `AYA_SKIN_SSBO`)を attach → `aya_skin_base[gl_InstanceIndex]` を読む** / `mVkUsesBindlessHeap=true`(`llglslshader.cpp:2908`・set=2 参照)= scaffold eligibility を通る。
- **thrash なし**: 現状 shadow rigged は既に `ensureVkDrawDataSlot`+`writeDrawSkinBase` を per-draw 実行(slot builder `lldrawpool.cpp:603-654`)。scaffold(`:1553-1585`)は同じ slot-filling(batch_textures 一致)+ 同じ skin base を再現 = slot 割当も skin base も**現状と byte 同一**、per-draw `pushBatch`(bindFast/set/drawRange)だけを per-span 1 回 bind + `vkCmdDrawIndexedIndirect` に置換。

### 改修(3 点・shadow-only・`AYASTORM_RIGGED_MDI` 既定 OFF)
1. **`lldrawpool.cpp` include 追加**: `#include "llpipelineframecontext.h"`(`isShadowPass()` 用)。
2. **`riggedMdiEligible`(`:1427`)を shadow-only に**: 従来の `type==SIMPLE_RIGGED||FULLBRIGHT_RIGGED`(scene B.3・配当 0.19ms=誤差)を撤去し `LLPipelineFrameContext::getInstance().isShadowPass()` に置換。他条件(kill switch / isIndirectDrawEnabled / skinBindlessEnabled / `sCurBoundShaderPtr->mVkUsesBindlessHeap` / !isRecordJobActive)は維持。→ **shadow の全 bucketized rigged type が eligible**(全て同一 depth shader)。
3. **guard(`:1665`)の `texture &&` 撤去**: shadow は `renderObjects(...,false,...,rigged)` で texture=false ゆえ従来 guard で弾かれる。eligibility が shadow-only になったので `if (riggedMdiEligible(type) && pushRiggedBatchesIndirect(type, batch_textures))` に。scene は isShadowPass=false で自動的に per-draw 据置。

### gate(命題様式)
- **命題**: 「shadow rigged draw の描画発行を per-draw pushBatch から per-span `vkCmdDrawIndexedIndirect` に畳んでも、影の描く物・pose・skin は byte 同一(skin base/slot は現状と同一値)、発行のみ削減」。
- 検証(A/B オラクル退役済ゆえ)= **影の視覚同一**(pose/アニメ/セルフシャドウ)+ validation 0 + device-lost 0(別担当)+ `shadow_rigged`→~0(per-draw 消滅)+ `mdi_call/mdi_rec`↑ + `e3 rig` bucket1 / `ph shad`↓ + **dense crowd で shadow rigged 記録膨張が collapse(rigmap 各 cascade↓)**。
- kill switch A/B = `AYASTORM_RIGGED_MDI=0`(旧経路)vs `=1`(MDI)で同一シーン比較。
- 未証明項: ①skin_entry INVALID 時 scaffold は per-draw へ graceful fallback(`:1583`)= 安全網だが INVALID 頻度は未実測 ②alpha-mask shadow rigged(diffuse alpha test)は本改修で texture slot が入る(scaffold `:1563`)= 視覚で要確認 ③per-cascade rebuild(retained でない)= command build は安いが実測で確認。

### 申告(縮小・省略)
- **段階 1 = shadow-only**(scene/probe rigged は誤差ゆえ scaffold 対象外)。scene B.3 の SIMPLE/FULLBRIGHT eligibility は撤去(negligible)。
- retained template(§3・frame 1 回構築 + 全 cascade 共有)は**やらない**(scaffold の per-cascade rebuild で配当の大半 = 高コスト bind 消去は得る)。retained は per-cascade rebuild が実測律速なら段階 1.5。
- kill switch は工事用(gate PASS 後撤去 = 段階 2)。

## 9. 結論(2026-07-24・AYA realization)= DrawCount 削減はここで区切る = **frame は compute-bound でなく serialization-bound**

段階1/1.5(shadow rigged MDI・commit `8135295627`)完了後、AYA が本質を看破:
> **「CPU も GPU も実は全然仕事してない。これ以上削減する意味はない。」**

### 根拠(実測と整合)
- **GPU = 28-36%(餓え)** + **main も真には飽和せず**(記録 → GPU 待ち → 記録 を交互 = 実効 util < 100%)。
- **両側に余裕があるのに frame = ~32ms(30fps)** = **compute-bound ではない**。∴ CPU/GPU いずれの work を減らしても frame は縮まない。
- 署名 = CLAUDE.md 最上位フレームの**ガン = 単一 main thread 直列パイプライン**: CPU が記録(GPU 遊ぶ)→ submit → GPU 描画(CPU 待つ)→ present の**直列**ゆえ、片側ずつしか動かず **frame ≈ CPU記録 + GPU描画 の和**。~16ms+~16ms が 16.6ms vsync 境界を跨いで **30fps**。

### 帰結
- **DrawCount 削減(戦略2B)= 「main の work を減らす」= serialization-bound には効かない**(段階1 が非退行で folding は正しく効いたが、frame time は work でなく直列構造に律速される)。dyn 畳込・alpha・material も同様に無意味。
- **本当のレバー = 直列を壊す = CPU と GPU を重ねる(overlap / pipelining)**:
  - **frame pipelining**(GPU が frame N を描く間に CPU が N+1 を記録)。FRAMES_IN_FLIGHT=3 があるのに直列 = **どこかの sync 点が overlap を殺している**(present_wait / fence wait / swapchain acquire が疑わしい)。
  - 戦略2C(記録 per-core 分散)は CPU 記録を速くして GPU 始動を早めるが、**直列そのものは pipelining の方が本丸**。
- **段階2(掃除)= 段階1 gate 後の旧経路+switch 撤去は残すが、DrawCount 削減の新標的は追わない(本 doc は §9 で closed 扱い)**。

### 次フェーズ(別 doc / 別線)= CPU-GPU overlap の診断
frame が直列である sync 点を実測特定(mlp acq/beg・present・fence の CPU 待ち時間を TID 隔離)→ overlap を殺している機構を名指し → pipelining 化。**「work を減らす」から「直列を重ねる」へ軸を移す。**
