# LLPipeline statics 分類台帳(2026-08-11・#23/#24 残 OPEN・調査納品)

**目的**: pipeline.h の static 宣言(213 行)から **data static 159 個**を抽出し {config / 相中 state / dead / 定数} に分類する(`docs/vknative_pass_context_design.md` §3 の台帳宣言済み gap)。処置は台帳外 = 個別に AYA 決裁。

**方法**: pipeline.h から data static を機械抽出 → repo 全域 word-grep で全 use site を収集 → 定義行・コメントを除外し、書き手 file の相帰属で分類。record 面 = llpipeline{render,shadow,lighting,pools,post,capture}.cpp + lldrawpool*。

## 集計

| 分類 | 件数 | 定義 |
|---|---|---|
| CONFIG | 132 | 書き手が refreshCachedSettings(llpipelinealloc)/ GUI・cvar listener(llviewercontrol・llviewermenu・llpipelinedebug・llfloater360capture・llviewershadermgr)のみ。frame 間安定・record 中の読みは並列化に安全 |
| 相中 STATE | 23 | 上記以外の runtime 書き手あり(下表) |
| DEAD | 4 | 宣言+定義のみ・参照ゼロ |
| 定数 | 6(CONFIG 内数)| constexpr k*Shadow* 4 + MAX_PREVIEW_WIDTH/HEIGHT |

## DEAD(削除候補・処置は AYA 決裁)

`sDistortionRender`(:831)/ `sDistortionWaterClipPlaneMargin`(:852)/ `sDoFEnabled`(:869)/ `sCurRenderPoolType`(:1281)— 各 2 ref(宣言 pipeline.h + 定義 pipeline.cpp)のみ。

## 相中 STATE 23 個(書き手の相で 3 群)

### A. record 面が書く(7)— 並列化の本丸・PassContext 化/引数化候補

| static | 書き手(非 config) | record 読み | 備考 |
|---|---|---|---|
| sUseOcclusion :789 | shadow×6・capture×2・display×6・window×2 | render/shadow/capture | save-0-restore 群 = **工事列 OPEN #1(updateCull 引数化)そのもの** |
| sVelocityRender :357 | post×2 | - | motionblur 相 flag(orchestration 内 write→read) |
| sT2xJitterEnabled :837 | post×2 | - | T2x jitter 相 flag |
| sImpostorRenderAlphaDepthPass :832 | capture×2 | lldrawpoolalpha | impostor orchestration 書き→pool 読み = pass flag 残存 |
| sVisibleLightCount :851 | lighting×3・window×1 | - | calcNearbyLights 計数 |
| sLastFocusPoint :868 | post×1 | post | DoF focus の frame 間遷移 state |
| RenderSpotLight :1368 | shadow×2 | - | ⚠️ **cvar 命名だが実態は相中 state**(spot shadow の現在 drawable を shadow 相が書く)。命名の罠 |

### B. update/cull 相が書く(8)— 直列 update 内で完結・record とは相分離済

sCompiles :785(update+llvo* 群の ++)/ sRenderTransparentWater :792(appviewer+update)/ sParcelCheckSeq :797・sParcelOwnerTagActive :802・KeepAvatars :803・KeepOwn :804・AltRanges :815(update)/ sVolumeSAFrame :853(cull+llvovolume)

### C. 演出・environment orchestration が書く(8)— main thread 相中

sShowHUDAttachments :787(llviewerwindow snapshot 前後 save/restore ×6)/ sLastSkyHdrScale :854・sLastSceneLightStrength :856・sLastMirrorFlag :858・sLastClipPlane :859(llsettingsvo 書き・**record-read = lighting/render/wlsky あり**)/ sRenderingDefaultProbeClip :861(reflectionmapmanager)/ sRegionClipPlane :862(viewerdisplay cube face)/ sRenderTextures :866(objectlist)

## 並列化観点の含意

- 群 A の 7 個が fork 導入時の要処置対象(snapshot 化 or 引数化)。sUseOcclusion は既 OPEN、残 6 は本台帳が初列挙。
- 群 C の sLast* 族は「main 書き → record 読み」= fork 時は構築点 snapshot(PassContext 同型)で足りる見込み。
- CONFIG 132 は refreshCachedSettings が frame 境界でのみ書く前提が成立する限り安全(この前提自体は未強制 = 将来の検出器候補・本工事では追加しない)。

## 申告欄(方法の限界)

- W 検出は行内 assignment 正規表現 = **参照/ポインタ経由の書きは不可視**(known 範囲で該当なしだが保証ではない)。
- CONFIG 判定は書き手 file 単位の規則(listener file 内の他用途書きがあれば誤吸収し得る)。
- record-read 列は pipeline 分割 file + lldrawpool* のみ列挙(llvo*/llface 等の record 呼び鎖からの読みは総数には計上・列には出さない)。
- 生データ = 実行時 scratch(`pipeline_statics_raw.tsv` / `pipeline_statics_final.tsv`)。判定はいつでも本 doc の方法説明から再生成可能。
