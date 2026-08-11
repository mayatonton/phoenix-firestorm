# #23/#24 PassContext 詳細設計(2026-08-11・設計 v1)

台帳 = `vknative_parallel_record_feasibility.md` #23/#24・§6.5。
**最上位原則(AYA 裁定 2026-08-11)**: 見やすく検証しやすいソースであること自体が安全性。ambient state はトレースを O(codebase) にする(データフロー不可視 + set/reset の時間結合)= 質問のコストが高いコードは危険。本工事は fork 安全化と同時に「依存が signature に宣言され grep 一発で読める形」への改革。
**後続(2 段目・別弾)**: pipeline.cpp の pure-move 分割(A 化で pass 構造が signature に現れてから切る)。

## 1. 対象データの分類(真実源は各 1 個)

| 種別 | 中身 | 家 |
|---|---|---|
| **pass スコープ(本工事で ctx 化)** | shadow / reflection / impostor / HUD / underwater / DoF の 6 flag + `LLCullResult*` + `RenderTargetPack* activeRT` + avatar minimumAlpha(#29 送り) | `LLRecordPassContext`(値渡し) |
| **frame 定数 config(ambient 残し)** | renderingDeferred(settings 書き)/ reflectionProbesEnabled(refreshCachedSettings 書き)/ renderingGlow(shader load 書き) | LLPipelineFrameContext 続投(single-writer・record 相 RO) |
| **cull スコープ** | sUseOcclusion(cull の挙動制御) | §4 参照(updateCull 引数化) |

## 2. LLRecordPassContext(新規・小 header)

```cpp
struct LLRecordPassContext
{
    bool shadowPass      = false;
    bool reflectionPass  = false;
    bool impostorPass    = false;
    bool hudPass         = false;
    bool underWater      = false;
    bool dofPass         = false;
    LLCullResult*                 cullResult = nullptr;
    LLPipeline::RenderTargetPack* activeRT   = nullptr;
    F32  avatarMinimumAlpha = 0.2f;
};
```
- newview 層 `llrecordpasscontext.h`。値セマンティクス・const 参照渡し。
- **singleton との関係 = snapshot 契約**: LLPipelineFrameContext は「演出側(main)の現在値」の真実源のまま。ctx は**現在 setter が走っている各スコープの入口**で singleton から構築して record 系呼び出しへ値渡し。fork 導入時は fork 前に main が組んで worker へ渡す(§6.5 の fork 引数化そのもの)。
- ⚠️ 構築点の粒度 = **現行の set/reset スコープと同一**(例: llviewerdisplay の water 反射まわりは underWater を pass 中に toggle している = その toggle 単位ごとに ctx を組み直す)。粒度を粗くすると直列挙動が変わる = 禁止。

## 3. 流し方(record 面の全面 A 化)

- **pool 仮想 IF**(lldrawpool.h:99-126): `begin/end/render × {RenderPass, Deferred, PostDeferred, Shadow, MotionBlur}` + `render` の全 virtual に `const LLRecordPassContext& ctx` を追加(旧 signature は残さない)。
- **push 系**(lldrawpool.h:371-396 の pushBatches/pushRigged/pushMask/pushGLTF 族)+ `LLVKBucket::currentVisBits`(cull result 読み)+ material bind(`LLFetchedGLTFMaterial::bind`・gltfscenemanager bind)+ avatar render 鎖(`renderAvatars`→`LLVOAvatar::renderSkinned/renderTransparent`)へ ctx を貫通。
- **呼び元(ctx 構築点)**: pipeline.cpp の pass driver 群(renderGeom/renderGeomDeferred/renderGeomPostDeferred/renderGeomShadow/renderShadow*/generateImpostor/renderDoF)+ llviewerdisplay の toggle スコープ + heroprobe/reflectionmap の face 描画。
- **読み替え対象 ≈ 65 site**(全数分類 = scratch trace `trace_item23_24_passctx.md`・実装 Brief に全列挙を義務化)。

### 漏れ 2 cluster の個別形
1. **joint 再帰**(llviewerjoint.cpp:76,80): LLAvatarJoint::render(llappearance 層)には newview 型を通せない → llappearance に素の POD `LLJointRenderFlags { bool shadow; bool reflection; }` を定義して仮想 IF に引数追加(bool は層中立)。overrides ≈ 4 class。
2. **texture stats**(llviewertexture.cpp:1870 processTextureStats の isShadowPass 読み): texture API に ctx は通せない。処方 = **抑制判断を呼び元へ移す**(record 中に texture touch する ctx 保持側が boost 抑制を伝える)。実装 Brief で processTextureStats への record 経路呼び鎖を全列挙してから確定(§6.1 相中 ensure 族と同枠・未列挙のまま実装しない)。

## 4. #24 の処置

- **sUseOcclusion**: record 相の save/0/restore(pipeline.cpp:13773-13824 = 影 MDI 内の in-path updateCull 向け)が病理。処方 = `updateCull(LLCamera&, LLCullResult&, bool hud_attachments, bool use_occlusion)` へ引数化し、save/restore 4 site(llviewerdisplay:970/:1423/:1546・llfloater360capture:447・pipeline 影内)を呼び出し引数へ置換。spatial 走査内の深い読み手(llspatialpartition 4 site)への伝搬形は実装 Brief で確定(cull 走査文脈への格納 or 引数貫通)。§2(cull 前倒し)と整合 = 前倒し後は record 相から occlusion 接触が消える。
- **LLVertexBuffer::unbind static**: HEAD で unbind() は**空関数**(llvertexbuffer.cpp:696-698・GL 物理削除の帰結)= 台帳の懸念は stale・**無罪確定**。残る unbind() 呼び出しは dead ceremony(削除は別途・本工事対象外)。
- **LLPipeline static 全数列挙(台帳宣言済み gap)**: pipeline.h の static 宣言 209 行(関数含む)から data static を抽出し {config / cull / record 接触 / dead} に分類した表を実装フェーズの deliverable とする。record 接触が新たに出たら個別処置(無断スコープ縮小禁止)。

## 5. sMinimumAlpha(#29 送り)

- 現状: `LLDrawPoolAvatar::sMinimumAlpha`(lldrawpoolavatar.h:133)を avatar pool の begin 系 5 site が読み、generateImpostor(pipeline.cpp:15943-15979)が save/0/restore = per-pass save/restore 病理の典型。
- 処方: ctx.avatarMinimumAlpha に移し、generateImpostor は impostor ctx に 0.f を積んで渡す。static と save/restore を削除。

## 6. gate(機械)

1. 直列挙動恒等(純リファクタ)。
2. **grep gate(pass 系 method 別)**: `isShadowPass|isReflectionPass|isImpostorPass|isHUDPass|isUnderWaterRendering|isDoFPass|getCullResult|getActiveRT` の getInstance() 経由読みが record 面 file 群(lldrawpool*/llvkbucket/llfetchedgltfmaterial/gltfscenemanager/llvoavatar/llviewerjoint)で **0 件**。config 3 method は対象外(許可)。
3. **仮想 IF 置換の取りこぼし検査**: 旧 signature(`beginShadowPass(S32`等・ctx なし形)の定義が repo で 0 件(silent non-override 事故の機械封じ)。
4. 判定走行 = validation 0 + 検出器沈黙(従来 gate)。

## 7. 申告欄(設計時点)

- config 3 flag を ambient 残しにするのは「pass 文脈でない」ため(single-writer・record 相 RO)。将来 record 相の書き手が現れたら本設計の前提が破れる = その時に ctx へ昇格。
- 非 record の読み手(snapshot/UI/stats/cull 相 drawable 6 site)は本工事で触らない(singleton 続投)。
- texture stats と spatial 走査内 occlusion 読みの最終形は実装 Brief の呼び鎖全列挙後に確定(設計段階では方向のみ・未列挙実装は禁止)。
- LLPipeline static 分類表は実装フェーズ納品。

## 8. 実装結果(2026-08-11・gate PASS)

- 44+ file・compile 9 巡収束。gate grep = ①pass 系 8 method の record 面参照 0 件 ②旧 signature 定義 0 件、両方合格。判定走行 = 硬チャネル 0・VKC 全層沈黙・S1 0・視覚 OK(AYA)。
- 実装で確定した形: `LLPipeline::buildRecordPassContext()`(singleton snapshot・public static)。構築点 = renderGeom* 4 入口・影 5 関数(setShadowPass(true) 直後)・演出側 orchestration(display/preview/bvh/image preview)。snapshot ≡ live 読みの等価は pass 状態全書き手 36 site と構築点の位置照合で検証済(renderGeom* 本体内に書き手ゼロ)。
- 副産物 dead 削除: `LLDrawPoolTerrain::getVertexDataMask`(呼び手ゼロ)・`LLDrawPoolAvatar::sMinimumAlpha` static。
- **残 OPEN = なし(2026-08-11 完走)**: sUseOcclusion 深部 = 引数化完了(`docs/vknative_occlusion_argumentization.md`)/ texture stats 呼び鎖 = 非欠陥決着(下記)/ LLPipeline statics 分類表 = 納品(`docs/vknative_pipeline_statics_ledger.md`)。2 段目 = pipeline.cpp pure-move 分割 = 完了(`cb9f74df394`)。

### texture stats 呼び鎖 決着(2026-08-11・呼び鎖全列挙済・AYA 裁定待ちの非欠陥提案)

§3 の処方(「抑制判断を呼び元へ移す」)は列挙前の方向であり、**全列挙の結果 record 相から processTextureStats に到達する呼び鎖はゼロ**と確定 = 移すべき抑制が存在しない。

- 直接呼び 3 site(全て llviewertexturelist.cpp): updateImageDecodePriority :1255 / forceImmediateUpdate :1421 / decodeAllImages :1629(virtual = LLViewerLODTexture override llviewertexture.cpp:3249 も同一呼び元・singleton 読みなし)。
- 呼び元の相(全列挙): updateImageDecodePriority ← updateImagesFetchTextures(:1567/:1574・updateImages = main texture update)+ LLViewerTexture::updateClass 緊急 purge(llviewertexture.cpp:559・updateClass 呼び元 = llviewerdisplay.cpp:630/:1043 = main orchestration・shadow flag 復元後)/ forceImmediateUpdate ← llworldmap.cpp:105・llinspecttexture.cpp:176(UI)/ decodeAllImages ← llstartup.cpp:2420・LLUIImageList::initFromFile(起動)。
- 処置案: llviewertexture.cpp:1870 の `llassert(!isShadowPass())` は**契約検出器として残置**(record 相からの新規呼び込みを debug で検知 = 観測可能性の原則)。コード変更なし。
- LLPipeline statics 分類表 = 納品済(`docs/vknative_pipeline_statics_ledger.md`・`adf4d667b20`)。
