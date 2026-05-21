# AYAstorm Layered DoF — 背景 / Alpha Mesh / 重ね合わせ Full Trace

**作成日**: 2026-05-21
**スコープ**: 全 alpha mesh attachment (hair / clothing / shoes / accessory 等)。観測初発は髪 (hair edge) の DoF blur 色シフトだが、AYA さん発言 (2026-05-21、§4.y) により「髪は最も分かりやすい観測例に過ぎず、服や靴にも同じことが起きる」ことが確定。本書中の §1-§3 事実 (pool / shader / blend func / depth write) は mesh 種別に依存しない alpha pool / material pool の動作 trace なので、Plan / Implement 段階では **mesh 種別 (hair / clothing / shoes / accessory) を問わない** mask 設計を目指す。
**作成経緯**: alpha mesh attachment (典型例: 髪) を DoF blur したときに edge で背景色が混ざる症状について、推論ベースの fix を 3 回外して revert (HEAD = `e676c52b87`)。以後は事実だけを並べる方針に切替。本書は「背景描画」「alpha mesh 描画」「両者の重ね合わせ」を file:line と blend func 単位で確定するためのトレース。コード変更を一切せずに作成。
**前提**: deferred renderer 経路。`LLPipeline::sRenderDeferred == true`、`gCubeSnapshot == false`、`sImpostorRender == false`、`sRenderingHUDs == false`。

> **再利用方針**: 本書は **どこに何があるか** だけを書く。fix 案は書かない。fix を検討するときは本書の事実集合の上に乗せる。
> **姉妹資料**:
> - `docs/specs/ayastorm-deferred-shader-routing.md` — オブジェクト → Pool → bound shader → gbuffer flag → softenLightF 分岐の確定マップ
> - `docs/specs/ayastorm-gbuffer3-trace.md` — gbuffer3 (emissiveRect) format 仕様

---

## 0. 用語と前提 RT (Render Target) 一覧

| RT | 確保 file:line | 用途 |
|---|---|---|
| `mRT->deferredScreen` | `indra/newview/pipeline.cpp:1053` (`allocate(resX, resY, GL_RGBA, true)`) + `addDeferredAttachments(target)` で +3 attachments | gbuffer (frag_data[0..3]) — opaque deferred 描画先 |
| `mRT->screen` | `indra/newview/pipeline.cpp:1058` (`allocate(resX, resY, GL_RGBA16F)`) — depth 共有: `mRT->deferredScreen.shareDepthBuffer(mRT->screen)` (`pipeline.cpp:1060`) | 統合シーンバッファ。softenLightF (合成後の lit color) と alpha pool (forward) の両方の書き先 |
| `mRT->deferredLight` | (pipeline.cpp 内別所) | DoF Pass 1 (CoF) の書き先 兼 DoF Pass 3 (combine) の lightMap 入力 |
| `mPostPingMap` / `mPostPongMap` | (`renderFinalize` 内 ping-pong) | tonemap / glow / DoF の中間バッファ |

`addDeferredAttachments` (`indra/newview/pipeline.cpp:428-466`) が `mRT->deferredScreen` に追加する color attachment:

- `frag_data[1]` = `orm` (`GL_RGBA` — HDR/LDR 共通)
- `frag_data[2]` = `norm` (`GL_RGBA16` HDR / `GL_RGB10_A2` LDR)
- `frag_data[3]` = `emissive` (`GL_RGBA16F` HDR / `GL_RGBA` LDR — r20 Phase C で `.a` 付き化)

`frag_data[0]` (diffuse / albedo) は最初の attachment、`mRT->deferredScreen` 自身 (`GL_RGBA`) として allocate 時に確保 (`pipeline.cpp:1053`)。

DoF 突入時に `mRT->screen` は **既に lit / atmospherics 合成済み (softenLightF + 全 alpha pool 出力済み) の HDR scene buffer**。`renderFinalize` で `tonemap()` 等を経由した後の ping-pong buffer (`sourceBuffer`) が `renderDoF(src, dst)` の入力。

---

## §1. 背景 (opaque / sky / terrain) を描画している pass

### §1.1 描画フローの骨格 (file:line)

メイン scene 描画は `display.cpp` から `LLPipeline::renderGeomDeferred()` → gbuffer fill → `LLPipeline::renderDeferredLighting()` → `LLPipeline::renderGeomPostDeferred()` の順に進む。

| 段階 | 関数 | file:line | bound target |
|---|---|---|---|
| gbuffer fill (opaque) | `LLPipeline::renderGeomDeferred` 呼び出し | `indra/newview/llviewerdisplay.cpp:1085-1130` (`mRT->deferredScreen.bindTarget()` → `renderGeomDeferred()`) | `mRT->deferredScreen` (MRT: frag_data[0..3]) |
| gbuffer flush | `rt.flush()` | `indra/newview/llviewerdisplay.cpp:1147-1148` | — |
| Lit composite (softenLightF) | `LLPipeline::renderDeferredLighting` | `indra/newview/pipeline.cpp:10602` (関数 entry)、`screen_target->bindTarget()` @ `pipeline.cpp:10759`、softenLightF 描画 @ `pipeline.cpp:10764-10822` | `mRT->screen` (`GL_RGBA16F`) |
| 後段 sky / non-deferred 系 | `renderGeomPostDeferred` 呼び出し | `pipeline.cpp:11141` (`mRT->screen` は **同じく bound のまま**、`screen_target->flush()` は `pipeline.cpp:11150` で全 alpha 描画後に flush) | `mRT->screen` |

### §1.2 「opaque な背景の RGB」がどこで `mRT->screen` に書かれるか

**確定 1: softenLightF の fullscreen blit**

`pipeline.cpp:10759` で `screen_target->bindTarget()` → `pipeline.cpp:10762` で `screen_target->clear(GL_COLOR_BUFFER_BIT)`(`clearColor(0,0,0,0)`) → `pipeline.cpp:10770` で `gDeferredSoftenProgram` (softenLightF) を bind → `pipeline.cpp:10818` で `mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3)` で fullscreen triangle を 1 回描く。

- depth test: `LLGLDepthTest depth(GL_FALSE);` (`pipeline.cpp:10813`)
- blend: `LLGLDisable blend(GL_BLEND);` (`pipeline.cpp:10814`)
- → **blend off / depth off の純 overwrite**。`mRT->screen` 全 pixel が softenLightF の出力で塗り潰される。

softenLightF 内部の分岐は `class3/deferred/softenLightF.glsl:167-209` (姉妹資料 §1)。WL sky の場合 `GBUFFER_FLAG_SKIP_ATMOS` (= 0.0) 分岐で sky 色を出力。opaque material の場合 `GBUFFER_FLAG_HAS_ATMOS` (= 0.34) 分岐で `pbrBaseLight()` 系の lit を計算。

**つまり「背景の RGB」の実態 = `mRT->deferredScreen` (gbuffer) を入力に softenLightF が `mRT->screen` に 1 回だけ blit した結果**。

### §1.3 「opaque material が `mRT->deferredScreen` に書く」具体的 file:line

opaque な髪以外の材質 (壁、床、空、avatar body etc.) は `renderGeomDeferred` (`pipeline.cpp:4921-5046`) 内の pool loop で gbuffer に書き出す。

| Pool | 関数 file:line | bound shader | gbuffer 書き先 |
|---|---|---|---|
| `LLDrawPoolMaterials` (legacy Spec/Normal) | `lldrawpoolmaterials.cpp:105-305` (`renderDeferred`)、binding @ `lldrawpoolmaterials.cpp:93` (`bindDeferredShader`) | `gDeferredMaterialProgram[idx]` → `materialF.glsl` | frag_data[0..3] (`class3/deferred/materialF.glsl:440-451`) |
| `LLDrawPoolGLTFPBR` | (姉妹資料 §6 経由 — `lldrawpoolpbropaque.cpp:53-69`) | `gDeferredPBROpaqueProgram` → `pbropaqueF.glsl` | frag_data[0..3] |
| `LLDrawPoolWLSky` | `lldrawpoolwlsky.cpp:471-500` (`renderDeferred`) | `gDeferredWLSkyProgram` / `cloud_shader` / sun / moon / stars | frag_data[0..3] (姉妹資料 §1: WL sky は `gbuffer flag = 0.0` SKIP_ATMOS) |
| `LLDrawPoolAlphaMask` (= `PASS_ALPHA_MASK`、`DIFFUSE_ALPHA_MODE_MASK` 系) | `lldrawpoolsimple.cpp:116-128` | `gDeferredDiffuseAlphaMaskProgram` → `diffuseAlphaMaskIndexedF.glsl` (`llviewershadermgr.cpp:1330`) | frag_data[0..3] |
| `LLDrawPoolSimple` (PASS_SIMPLE) | `lldrawpoolsimple.cpp:99-111` | `gDeferredDiffuseProgram` → `diffuseF.glsl` | frag_data[0..3] |

depth: `mRT->deferredScreen` は `mRT->screen` と depth buffer 共有 (`pipeline.cpp:1060`)。opaque 描画では `LLGLDepthTest(GL_TRUE, GL_TRUE, GL_LEQUAL)` (= `LLGLSPipeline` default、`indra/llrender/llglstates.h:105-108`)、`LLGLDisable blend(GL_BLEND)` (例: `lldrawpoolsimple.cpp:102`)。

### §1.4 §1 確定度: 確定

「DoF 入力時点の `mRT->screen` の RGB は『softenLightF が gbuffer から再構成した lit シーン』+ その上に alpha pool が forward blend した結果」までは file:line で確定。

---

## §2. 髪 (alpha-masked / alpha-blend mesh) を描画している pass

髪は SL では **mesh attachment** で、材質設定は 2 通りある:

- **Blinn-Phong + diffuse alpha**: 旧式テクスチャの alpha チャネル
- **GLTF PBR**: GLTF Material の `alphaMode` = `MASK` または `BLEND`

これに `DIFFUSE_ALPHA_MODE` が掛かって **5 つの分岐** に展開される。

### §2.1 分岐ルーティング表 (LLVOVolume::rebuildFace 経由、`indra/newview/llvovolume.cpp`)

| 材質状態 | 判定 file:line | 登録 PASS | 走る Pool | bind される shader | shader file |
|---|---|---|---|---|---|
| GLTF PBR + `alphaMode == BLEND` | `llvovolume.cpp:6982-6986` | `LLRenderPass::PASS_ALPHA` | `LLDrawPoolAlpha` (POOL_ALPHA_POST_WATER / POOL_ALPHA_PRE_WATER) | `gDeferredPBRAlphaProgram` (`gHUDPBRAlphaProgram` の非 HUD ブランチ) | `class2/deferred/pbralphaF.glsl` (vert: `pbralphaV.glsl`)。`llviewershadermgr.cpp:1570-1605` で setup |
| GLTF PBR + `alphaMode == MASK` | `llvovolume.cpp:6987-6990` | `LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK` | `LLDrawPoolGLTFPBR` (POOL_GLTF_PBR_ALPHA_MASK) | `gDeferredPBROpaqueProgram(rigged)` | `class1/deferred/pbropaqueF.glsl` (姉妹資料 §2) |
| Blinn-Phong + mat + `DIFFUSE_ALPHA_MODE_BLEND` (= `material_pass` 経由で blend mask) | `llvovolume.cpp:7054-7096`、テーブル `pass[]` の `LLRenderPass::PASS_ALPHA` 行 (index 1/5/9/13) | `PASS_ALPHA` | `LLDrawPoolAlpha` | `gDeferredMaterialProgram[mask]` (alpha pool 内で再 bind: `lldrawpoolalpha.cpp:186-190` + `:771`) — alpha_mode bit が 1 (`DIFFUSE_ALPHA_MODE_BLEND`) | `class3/deferred/materialF.glsl` の `DIFFUSE_ALPHA_MODE == 1` ブランチ (line 57-185 の forward 経路) |
| Blinn-Phong + mat + `DIFFUSE_ALPHA_MODE_MASK` (material_pass) | `llvovolume.cpp:7054-7096`、テーブル `LLRenderPass::PASS_MATERIAL_ALPHA_MASK` (index 2) | `PASS_MATERIAL_ALPHA_MASK` | `LLDrawPoolMaterials` (deferred pool) | `gDeferredMaterialProgram[2]` (`DIFFUSE_ALPHA_MODE=2, HAS_ALPHA_MASK=1`) | `class3/deferred/materialF.glsl` の `#else` (= `DIFFUSE_ALPHA_MODE != 1`) ブランチ (line 188-456) で `frag_data[0..3]` 書き出し |
| Blinn-Phong + no mat + `te.color.a < 0.999f` (blinn_phong_transparent) | `llvovolume.cpp:7099-7117`、`mode == BLEND` → `PASS_ALPHA` | `PASS_ALPHA` | `LLDrawPoolAlpha` | `gDeferredAlphaProgram` (`simple_shader` @ `lldrawpoolalpha.cpp:179-184`) | `class2/deferred/alphaF.glsl` (vert: `class1/deferred/alphaV.glsl`) — `llviewershadermgr.cpp:1864-1934` で setup |
| Blinn-Phong + no mat + alpha mask | `llvovolume.cpp:7110-7113` | `PASS_ALPHA_MASK` / `PASS_FULLBRIGHT_ALPHA_MASK` | `LLDrawPoolAlphaMask` (deferred pool) | `gDeferredDiffuseAlphaMaskProgram` | `class1/deferred/diffuseAlphaMaskIndexedF.glsl` (姉妹資料 §4) |

**事実**: 「髪」が AYA さんのモデルで具体的にどの分岐に落ちるかは **本書時点では不明** (§4 参照)。ただし上記 6 分岐のいずれかに必ず入る。

### §2.2 alpha mask vs alpha blend の判定条件

#### Blinn-Phong + DIFFUSE_ALPHA_MODE_MASK 系 (PASS_ALPHA_MASK / PASS_MATERIAL_ALPHA_MASK 等)

discard 判定は shader 側で `alphaMask()` 関数経由 (`class3/deferred/materialF.glsl:259-269`):

```glsl
void alphaMask(float alpha)
{
#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_MASK)
    float bias = 0.001953125; // 1/512
    if (alpha < minimum_alpha-bias)
    {
        discard;
    }
#endif
}
```

→ `minimum_alpha` (uniform、`LLMaterial::mAlphaMaskCutoff` 由来、`lldrawpoolmaterials.cpp:213-217`) より小さい alpha は **discard**。discard した pixel は gbuffer に書かれず、depth も書かれない。
→ MASK 経路の髪は **gbuffer (deferred) pass で確定**。後段 alpha pool には来ない。

#### GLTF PBR + alphaMode == MASK

`class1/deferred/pbropaqueF.glsl` 内 (姉妹資料 §1) で同様に `minimum_alpha` discard。gbuffer (frag_data[0..3]) に書き、depth も書く (PBR opaque pool は `renderGeomDeferred` で走るので blend off / depth write on)。

#### Blinn-Phong + DIFFUSE_ALPHA_MODE_BLEND (PASS_ALPHA 経由、`LLDrawPoolAlpha`)

`class2/deferred/alphaF.glsl:226-244`:

```glsl
#ifdef IS_AVATAR_SKIN
    if(final_alpha < minimum_alpha)
    {
        discard;
    }
#endif

#ifdef USE_VERTEX_COLOR
    final_alpha *= vertex_color.a;

    if (final_alpha < minimum_alpha)
    {
        discard;
    }
    ...
#endif
```

→ ここでも `minimum_alpha` 未満は discard。`minimum_alpha` は `prepare_alpha_shader()` (`lldrawpoolalpha.cpp:93-138`) で 2 値設定:
- 通常: `MINIMUM_ALPHA = 0.004f` (≈1/255、`lldrawpoolalpha.cpp:64`)
- impostor: `MINIMUM_IMPOSTOR_ALPHA = 0.1f` (`lldrawpoolalpha.cpp:67`)
- DoF depth pass: `0.33f`、Cinematic + `RenderDepthOfFieldAlphas==false` で `1.f` (= 全部 discard)、`true` で `0.7f` (`lldrawpoolalpha.cpp:240-246`)

→ blend 経路でも `minimum_alpha` 未満は **discard で消える**。それ以上は frag_color を出力して blend される (§3 参照)。

#### GLTF PBR + alphaMode == BLEND

`class2/deferred/pbralphaF.glsl:139-144`:

```glsl
#ifdef HAS_ALPHA_MASK
    if (basecolor.a < minimum_alpha)
    {
        discard;
    }
#endif
```

しかし `gDeferredPBRAlphaProgram` の permutation は `llviewershadermgr.cpp:1576-1577` で `DIFFUSE_ALPHA_MODE=1 (BLEND)`、`HAS_ALPHA_MASK` permutation は **付与されていない** (1577 行付近で `addPermutation("HAS_ALPHA_MASK", ...)` の呼び出しが無い)。
→ PBR alpha BLEND 経路では `#ifdef HAS_ALPHA_MASK` ブロックが **コンパイル時に消える**。**discard 無し**。`basecolor.a` が 0 でも 0.001 でも fragment は **必ず出力** される。

### §2.3 髪が走る pool は alpha pool? material pool? 両方?

**確定: 両方ありうる** (前提: §4 で具体材質を確定する必要あり)

- mesh 髪の DIFFUSE_ALPHA_MODE 設定次第で:
  - `BLEND` → `LLDrawPoolAlpha::renderPostDeferred` (`lldrawpoolalpha.cpp:142-258`) 経由 (forward, blend on, depth write off (rigged 以外))
  - `MASK` → `LLDrawPoolMaterials` または `LLDrawPoolAlphaMask` 経由 (deferred, blend off, depth write on)

両者は **書き先 RT も blend 設定も別物**。§3 で重ね合わせの場所が分かれる。

### §2.4 髪 vert/frag (alphaF) の入出力 confirm

`alphaV.glsl` (`class1/deferred/alphaV.glsl`):
- out: `vary_fragcoord`, `vary_position`, `vary_texcoord0`, `vary_norm`, `vertex_color` (条件付き)
- `HAS_SKIN` (rigged) ブランチで matrix palette 経由の transform
- `gl_Position` 計算は 3 ブランチ (HAS_SKIN / IS_AVATAR_SKIN / 通常)

`alphaF.glsl` (`class2/deferred/alphaF.glsl`):
- in: 上記 vert の out
- out: `frag_color` (`vec4`) — **単一 attachment**
- 主な処理: `texture(diffuseMap, ...)` → discard 判定 → `calcAtmosphericVarsLinear()` → sun/IBL lit → `applySkyAndWaterFog(pos, additive, atten, color)` で fog 適用 (line 305) → `frag_color = max(color, vec4(0))` (line 317)
- **背景テクスチャの sampling は無い**。`applySkyAndWaterFog` (`class1/environment/waterFogF.glsl:115-144`) も「現 fragment 自身の color に fog を **加算/減衰** する」だけで、screen からの sample 等は無い。

### §2.5 §2 確定度: 部分確定

- 「髪が通る可能性のある全 6 経路」「各経路の bind shader / file / discard 条件」までは確定
- 「AYA さんの観測する『色シフトする髪』が **6 経路のうちどれを通っているか**」は **不明** (§4)

### §2.6 髪 mesh 経路判別 matrix (rebuildFace 上から下に評価される順)

LLVOVolume が face を pool/pass に割り当てる際の **正確な判定順序**。`indra/newview/llvovolume.cpp` の `LLVolumeGeometryManager::rebuildMesh` → `genDrawInfo` (5577-7167) で、各 face あたり下記 step を上から順に評価し、最初に該当した経路に `registerFace(group, facep, PASS_*)` で登録する。`registerFace` 内 (`llvovolume.cpp:5608-5614`) で `facep->isState(LLFace::RIGGED)` が true なら **PASS 値 +1** して `PASS_*_RIGGED` 系に振り替える (matrix 表の「rigged 振替」列)。

| step | 判定条件 | file:line | 該当時 PASS | 該当時 Pool | 該当時 shader | shader file | rigged 振替 |
|---|---|---|---|---|---|---|---|
| S1 | `gltf_mat != nullptr` かつ `gltf_mat->mAlphaMode == ALPHA_MODE_BLEND` | `llvovolume.cpp:6982` | `PASS_ALPHA` (=GLTF BLEND は alpha pool に同居) | `LLDrawPoolAlpha` (POOL_ALPHA_PRE/POST_WATER) | `gDeferredPBRAlphaProgram` (`lldrawpoolalpha.cpp:192-196`)、rigged は `mRiggedVariant` (`lldrawpoolalpha.cpp:728`) | `class2/deferred/pbralphaF.glsl` + `pbralphaV.glsl` | `PASS_ALPHA_RIGGED` (+1) |
| S2 | `gltf_mat != nullptr` かつ `gltf_mat->mAlphaMode == ALPHA_MODE_MASK` | `llvovolume.cpp:6987` | `PASS_GLTF_PBR_ALPHA_MASK` | `LLDrawPoolGLTFPBR` (`mRenderType == RENDER_TYPE_PASS_GLTF_PBR_ALPHA_MASK`、`lldrawpoolpbropaque.cpp:40,57,130`) | `gDeferredPBROpaqueProgram` (or `mRiggedVariant`、`lldrawpoolpbropaque.cpp:108`) | `class1/deferred/pbropaqueF.glsl` | `PASS_GLTF_PBR_ALPHA_MASK_RIGGED` |
| S3 | `gltf_mat != nullptr` (S1/S2 以外、つまり `ALPHA_MODE_OPAQUE`) | `llvovolume.cpp:6993` | `PASS_GLTF_PBR` | `LLDrawPoolGLTFPBR` | `gDeferredPBROpaqueProgram` | `class1/deferred/pbropaqueF.glsl` | `PASS_GLTF_PBR_RIGGED` |
| S4 | `mat != nullptr` (legacy Blinn-Phong material) かつ `te->getFullbright() == true` かつ `mat->getDiffuseAlphaMode() == DIFFUSE_ALPHA_MODE_MASK` かつ `blinn_phong_opaque (te->getColor().a >= 0.999f)` | `llvovolume.cpp:7001-7007` | `PASS_FULLBRIGHT_ALPHA_MASK` | `LLDrawPoolAlphaMask` | `gDeferredFullbrightAlphaMaskProgram` | `class1/deferred/fullbrightAlphaMaskF.glsl` | `PASS_FULLBRIGHT_ALPHA_MASK_RIGGED` |
| S5 | `mat != nullptr` かつ `te->getFullbright() == true` かつ MASK だが `blinn_phong_transparent (te->getColor().a < 0.999f)` (S4 の `else`) | `llvovolume.cpp:7009-7012` | `PASS_ALPHA` | `LLDrawPoolAlpha` | `gDeferredFullbrightAlphaMaskAlphaProgram` (alpha pool `fullbright_shader` 経路、`lldrawpoolalpha.cpp:173-177`) | `class1/deferred/fullbrightAlphaMaskF.glsl` (`HAS_ALPHA_MASK=1` permutation) | `PASS_ALPHA_RIGGED` |
| S6 | `mat != nullptr` かつ fullbright かつ `is_alpha` (S4/S5 以外) | `llvovolume.cpp:7014-7017` | `PASS_ALPHA` | `LLDrawPoolAlpha` | `fullbright_shader` (alpha pool 内分岐、`lldrawpoolalpha.cpp:743-753`) | 同上 | `PASS_ALPHA_RIGGED` |
| S7 | `mat != nullptr` かつ fullbright かつ shiny/env 無し かつ `blinn_phong_opaque` | `llvovolume.cpp:7026-7029` | `PASS_FULLBRIGHT` | `LLDrawPoolFullbright` | `gDeferredFullbrightProgram` | `class1/deferred/fullbrightF.glsl` | `PASS_FULLBRIGHT_RIGGED` |
| S8 | `mat != nullptr` かつ fullbright かつ shiny/env 無し かつ `blinn_phong_transparent` | `llvovolume.cpp:7030-7033` | `PASS_ALPHA` | `LLDrawPoolAlpha` | `fullbright_shader` | 同 S6 | `PASS_ALPHA_RIGGED` |
| S9 | `mat != nullptr` かつ **non-fullbright** かつ `blinn_phong_transparent` (S4-S8 以外、line 7037) | `llvovolume.cpp:7037-7040` | `PASS_ALPHA` | `LLDrawPoolAlpha` | `gDeferredAlphaProgram` (alpha pool `simple_shader` 経路、`lldrawpoolalpha.cpp:179-182`) | `class2/deferred/alphaF.glsl` (`USE_VERTEX_COLOR` 等の permutation) | `PASS_ALPHA_RIGGED` |
| S10 | `mat != nullptr` かつ non-fullbright かつ legacy bump あり (S9 以外、line 7041) | `llvovolume.cpp:7041-7046` | `PASS_BUMP` | `LLDrawPoolBump` | `gDeferredBumpProgram` | `class1/deferred/bumpF.glsl` | `PASS_BUMP_RIGGED` |
| S11 | `mat != nullptr` かつ non-fullbright かつ S9/S10 以外 → `material_pass = true` で 7052-7097 のテーブル参照 (材質の shader mask + alpha_mode) | `llvovolume.cpp:7047-7097` | `pass[mask]` で `PASS_MATERIAL` / `PASS_ALPHA` / `PASS_MATERIAL_ALPHA_MASK` / `PASS_MATERIAL_ALPHA_EMISSIVE` / `PASS_SPECMAP*` / `PASS_NORMMAP*` / `PASS_NORMSPEC*` のいずれか (`llvovolume.cpp:7054-7072` の `pass[]` 配列) | `LLDrawPoolMaterials` (MASK/MATERIAL/SPECMAP/NORMMAP/NORMSPEC 系) または `LLDrawPoolAlpha` (`PASS_ALPHA` の場合のみ) | `gDeferredMaterialProgram[mask]` (`llviewershadermgr.cpp:1384-1457`、`DIFFUSE_ALPHA_MODE` permutation @ 1422、`HAS_ALPHA_MASK` @ 1426-1427) | `class3/deferred/materialF.glsl` | `PASS_MATERIAL_*_RIGGED` (+1) または `PASS_ALPHA_RIGGED` |
| S12 | `mat == nullptr` (Blinn-Phong without material) かつ `is_alpha` かつ `te->getFullbright() == true` (`llvovolume.cpp:7129-7141`) → `canRenderAsMask()` && !hud_group | `llvovolume.cpp:7137-7141` | `PASS_FULLBRIGHT_ALPHA_MASK` | `LLDrawPoolAlphaMask` | `gDeferredFullbrightAlphaMaskProgram` | `class1/deferred/fullbrightAlphaMaskF.glsl` | `PASS_FULLBRIGHT_ALPHA_MASK_RIGGED` |
| S13 | `mat == nullptr` かつ `is_alpha` かつ non-fullbright かつ `canRenderAsMask() == true` && !hud_group | `llvovolume.cpp:7143-7146` | `PASS_ALPHA_MASK` | `LLDrawPoolAlphaMask` | `gDeferredDiffuseAlphaMaskProgram` (`llviewershadermgr.cpp:1327-1335`) | `class1/deferred/diffuseAlphaMaskIndexedF.glsl` | `PASS_ALPHA_MASK_RIGGED` |
| S14 | `mat == nullptr` かつ `is_alpha` かつ S12/S13 以外 (= `canRenderAsMask() == false`) | `llvovolume.cpp:7149-7151` | `PASS_ALPHA` | `LLDrawPoolAlpha` | `gDeferredAlphaProgram` (simple_shader、`lldrawpoolalpha.cpp:179-182`) | `class2/deferred/alphaF.glsl` | `PASS_ALPHA_RIGGED` |

#### §2.6.1 重要な分岐: `LLFace::canRenderAsMask()` (`llface.cpp:1126-1173`)

S13/S14 で「bare alpha (mat 無し、gltf 無し) が MASK 経路に落ちるか BLEND 経路に落ちるか」を決定する関数。**false を返す条件は順に**:

1. `te == nullptr` または `getViewerObject() == nullptr` または `getTexture() == nullptr` → false (`llface.cpp:1129-1132`)
2. `te->getGLTFRenderMaterial() != nullptr` → false (= GLTF mat 持ちは S1-S3 で既に振り分け済みなのでここに来ない、防御的 check)
3. `LLPipeline::sNoAlpha == true` → true (debug mode、通常 false)
4. **`isState(LLFace::RIGGED) == true` → false** (`llface.cpp:1144-1147`)
5. `mat && mat->getDiffuseAlphaMode() == DIFFUSE_ALPHA_MODE_BLEND` → false (= S11 で BLEND が走っているはずなのでここに来ない、防御的 check)
6. `te->getColor().a != 1.0` || `te->getGlow() != 0` || `isHUDAttachment()` || `!getTexture()->getIsAlphaMask()` のいずれか → false (`llface.cpp:1156-1159`)
7. 上記を全部通って `te->getFullbright() == true` → `sAutoMaskAlphaNonDeferred` を返す (deferred 経路では通常 false)
8. それ以外 → `sAutoMaskAlphaDeferred` を返す (deferred 経路では通常 true)

**帰結**: **rigged な mesh hair (bare alpha、mat なし、GLTF なし) は条件 4 で `canRenderAsMask() == false` 確定**。よって S13 ではなく **S14 (PASS_ALPHA → BLEND 経路)** に必ず落ちる。AYA さんの hair が「rigged mesh attachment」である限り、bare alpha 構成では MASK 経路は踏まない。

#### §2.6.2 hair が高確率で踏むのは S1 / S9 / S11(PASS_ALPHA) / S14 の 4 経路に絞れる

実用上 SL の mesh hair で起こる経路:

| 髪の構成 | 通る step | 通る pool | shader |
|---|---|---|---|
| GLTF PBR (BLEND) hair | S1 | `LLDrawPoolAlpha` | `gDeferredPBRAlphaProgram(mRiggedVariant)` (pbralphaF) |
| GLTF PBR (MASK) hair | S2 | `LLDrawPoolGLTFPBR` | `gDeferredPBROpaqueProgram(mRiggedVariant)` (pbropaqueF、`HAS_ALPHA_MASK` discard) |
| Blinn-Phong material + DIFFUSE_ALPHA_MODE_BLEND (mesh が "Alpha Blending" 選択) | S11 (mask テーブルが `PASS_ALPHA` を返す枝、line 7057/7061/7065/7069) | `LLDrawPoolAlpha` | `gDeferredMaterialProgram[mask]` (materialF、`DIFFUSE_ALPHA_MODE == 1` ブランチ) |
| Blinn-Phong material + DIFFUSE_ALPHA_MODE_MASK (mesh が "Alpha Masking" 選択) | S11 (mask テーブルが `PASS_MATERIAL_ALPHA_MASK` を返す枝、line 7058) | `LLDrawPoolMaterials` | `gDeferredMaterialProgram[2]` (materialF、`DIFFUSE_ALPHA_MODE != 1` ブランチ、gbuffer MRT 書き出し) |
| Blinn-Phong material + DIFFUSE_ALPHA_MODE_NONE/EMISSIVE (mesh が "None" 選択 + alpha 0.5 で半透明設定) | `te->getColor().a < 0.999f` → `is_alpha = true` (`llvovolume.cpp:6973`)、その後 S9 | `LLDrawPoolAlpha` | `gDeferredAlphaProgram(mRiggedVariant)` (alphaF) |
| Blinn-Phong **no material** + diffuse tex の alpha < 1 (legacy、mat なし) | S14 (rigged ゆえ canRenderAsMask=false) | `LLDrawPoolAlpha` | `gDeferredAlphaProgram(mRiggedVariant)` (alphaF) |

> **§2.6.2 補注**: S9 と S14 は **同じ shader (alphaF.glsl) に落ちる** が、permutation (`USE_VERTEX_COLOR` の有無等) と `mShaderMask` 由来の uniform 設定が異なる場合がある。`USE_VERTEX_COLOR` は `gDeferredAlphaProgram` setup (`llviewershadermgr.cpp:1864-1934`) で常に permutation 追加されるため両者で立つ。

### §2.7 髪 mesh 経路の実機検証手順

**前提**: AYA さんは Build/Edit で自分の hair の material 設定を直接覗けない場合がある (modifiable=NO の店舗品が多い)。以下は侵襲度の低い順に並べる。

#### §2.7.A 既存 UI による絞り込み (コード変更 0、即時)

`Develop > Rendering > Types` の各 toggle (`indra/newview/skins/default/xui/en/menu_viewer.xml:3320-3409`、`render_type_from_string` 経由で `LLPipeline::RENDER_TYPE_*` を toggle、`llviewermenu.cpp:943-1024`) で**髪が消えるか観察**することで、どの RENDER_TYPE 配下に居るかを 5 択まで絞れる:

| toggle ラベル | parameter | 消す RENDER_TYPE | 髪が消えたら確定する経路 |
|---|---|---|---|
| Develop > Rendering > Types > **Materials** | `materials` | `RENDER_TYPE_MATERIALS` (= `POOL_MATERIALS`) | S11 で `PASS_MATERIAL_ALPHA_MASK` 等の **deferred material pool 系** (= Blinn-Phong + DIFFUSE_ALPHA_MODE_MASK) |
| Develop > Rendering > Types > **Alpha Mask** | `alpha_mask` | `RENDER_TYPE_ALPHA_MASK` (= `POOL_ALPHA_MASK`) | S13/S14 のうち **S13 (bare alpha + canRenderAsMask=true)** ※ rigged hair では §2.6.1 より発火しない |
| Develop > Rendering > Types > **Fullbright Alpha Mask** | `fullbright_alpha_mask` | `RENDER_TYPE_FULLBRIGHT_ALPHA_MASK` | S4/S12 (fullbright + bare alpha mask) |
| Develop > Rendering > Types > **Alpha** | `alpha` | `RENDER_TYPE_ALPHA` (= `POOL_ALPHA`) | **S1 / S5 / S6 / S8 / S9 / S11(PASS_ALPHA) / S14 すべて** (alpha pool 経路全部) |
| Develop > Rendering > Types > **PBR** | `pbr` | `RENDER_TYPE_GLTF_PBR` | S2/S3 (GLTF MASK / OPAQUE — `LLDrawPoolGLTFPBR` 経由)。**ただし `RENDER_TYPE_GLTF_PBR_ALPHA_MASK` は別 enum でこの toggle では消えない可能性あり** (`render_type_from_string` line 1017-1020 で `"pbr"` は `RENDER_TYPE_GLTF_PBR` のみマップ、`RENDER_TYPE_GLTF_PBR_ALPHA_MASK` の UI toggle は menu_viewer.xml に存在しない) |

→ **手順**:
1. `Develop > Rendering > Types > Alpha` を OFF にする。
2. **髪全体が消えれば** → S1 (PBR BLEND) または S5/S6/S8/S9/S11(PASS_ALPHA)/S14 のいずれか (= alpha pool 系)。さらに `Develop > Rendering > Types > Materials` も OFF (Alpha を ON に戻して) して消えなければ S11(PASS_ALPHA) 系を消去できる。
3. **髪が残れば** → S2/S3 (GLTF MASK/OPAQUE) または S4/S12 (fullbright MASK) または S11 (deferred material MASK)、S13 のいずれか。`Materials` OFF / `Fullbright Alpha Mask` OFF / `PBR` OFF を順に試して消えた toggle が経路を確定。

**限界**: `PASS_GLTF_PBR_ALPHA_MASK` 単独の UI toggle は無い (`render_type_from_string` の `"pbr"` ケースは `RENDER_TYPE_GLTF_PBR` のみ)。GLTF MASK と GLTF OPAQUE を分離するには §2.7.C か §2.7.D が必要。

#### §2.7.B 髪 prim を選択 (Build 画面が開けば) — コード変更 0、限定的

Edit → Linked Parts で hair attachment の face を選択 → Texture タブ:
- 「Alpha Mode」 combo の選択値 (None / Alpha Blending / Alpha Masking / Emissive Mask) で Blinn-Phong material 系の DIFFUSE_ALPHA_MODE 確定 (`fspanelface.cpp:1871-1886, 2316-2328`)
- 「PBR Material」が表示されていれば GLTF mat 系、その「Alpha Mode」が `MASK`/`BLEND`/`OPAQUE` の どれか判定可

**限界**: modifiable=NO の attachment は Build 画面で開けない場合があり、その場合この方法は使えない。

#### §2.7.C 一時 LL_INFOS hook を `LLDrawPoolAlpha::renderAlpha` の per-batch ループに仕込む (推奨、侵襲度 小)

**コード変更を要するため、本書では diff のみ記述 (適用は AYA さん判断)**:

```diff
--- a/indra/newview/lldrawpoolalpha.cpp
+++ b/indra/newview/lldrawpoolalpha.cpp
@@ -705,6 +705,28 @@ void LLDrawPoolAlpha::renderAlpha(U32 mask, bool depth_only, bool rigged)
             LLSpatialGroup::drawmap_elem_t& draw_info = rigged ? group->mDrawMap[LLRenderPass::PASS_ALPHA_RIGGED] : group->mDrawMap[LLRenderPass::PASS_ALPHA];

             for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)
             {
                 LLDrawInfo& params = **k;
+                // <DIAG: ayastorm hair path probe — REMOVE BEFORE COMMIT>
+                // Dump 1 line per draw call for batches whose mTexture name matches "hair"
+                // (case-insensitive) or whose mAvatar is set + isRiggedMesh.
+                static U32 dbg_hits = 0;
+                if (dbg_hits < 200 && params.mAvatar != nullptr)
+                {
+                    const char* gltf  = params.mGLTFMaterial.notNull() ? "GLTF"  : "noGLTF";
+                    const char* legmat = params.mMaterial.notNull()     ? "LegacyMat" : "noLegacyMat";
+                    U8 gltf_amode = params.mGLTFMaterial.notNull() ? (U8)params.mGLTFMaterial->mAlphaMode : 0xFF;
+                    U8 leg_amode  = params.mMaterial.notNull()     ? params.mMaterial->getDiffuseAlphaMode() : 0xFF;
+                    LL_INFOS("AyaHairPath") << "alpha_pool batch"
+                        << " rigged="   << (rigged ? 1 : 0)
+                        << " pool="     << (S32)getType()       // 4 = POOL_ALPHA_POST_WATER, 3 = PRE_WATER
+                        << " gltf="     << gltf << " amode=" << (S32)gltf_amode
+                        << " legacy="   << legmat << " amode=" << (S32)leg_amode
+                        << " fullbright=" << (params.mFullbright ? 1 : 0)
+                        << " blendSrc="  << (S32)params.mBlendFuncSrc
+                        << " blendDst="  << (S32)params.mBlendFuncDst
+                        << " texName="   << (params.mTexture ? params.mTexture->getID().asString() : "null")
+                        << LL_ENDL;
+                    ++dbg_hits;
+                }
+                // </DIAG>
                 if ((bool)params.mAvatar != rigged)
```

加えて同様の hook を:
- `LLDrawPoolMaterials::renderDeferred` 内 (`lldrawpoolmaterials.cpp:105-305`、drawRange 直前) に MASK 経路用
- `LLDrawPoolGLTFPBR::renderDeferred` 内 (`lldrawpoolpbropaque.cpp:53-180`、drawRange 直前) に PBR opaque/MASK 経路用
- `LLDrawPoolAlphaMask::renderDeferred` 内 (`lldrawpoolsimple.cpp:116-128`) に bare alpha MASK 経路用

…の 3 ヶ所にも同じ形で入れれば、髪の **rigged batch がどの pool に来ているか 1 起動で確定** できる。

**ログ出力先**: `~/.ayastorm_x64/logs/AYAstorm.log`。`grep AyaHairPath` で抽出可能。

**検証手順 (AYA さん側)**:
1. 上記 diff を適用してビルド。
2. AYAstorm 起動、髪が見える状態で **1 フレーム描画する** だけで十分 (LLDrawInfo は cull/visible batch のみ来る)。
3. log を Claude が解析 → どの pool に rigged batch が来ているか、各 LLDrawInfo の `gltf=` / `legacy=` / `amode=` から経路確定。
4. 検証後 diff を revert (memory `feedback_remove_verification_logs.md` 参照)。

**色 canary より LL_INFOS hook を推す理由**:
- 6 shader の出力 RGB を全部書き換えると外見が壊れて in-world 滞在不能になる。
- 1 起動 1 行 log なら絵を維持したまま全 path のサンプリングが取れる。
- 後段 §3 の DoF 解析と直接連結する `LLDrawInfo` field (mBlendFunc/mGLTFMaterial/mMaterial) が一発で見られる。

#### §2.7.D 色 canary (shader 直接書換、侵襲度 大) — 非推奨

`feedback_diag_canary_design.md` に従い 6 shader の `frag_color.rgb` (alpha pool 系) / `frag_data[0].rgb` (gbuffer 系) を **完全置換型の純色** で塗る:

| shader | file | 着色 | 元色との被り |
|---|---|---|---|
| pbralphaF | `class2/deferred/pbralphaF.glsl` | `vec3(1, 0, 0)` 純赤 | OK (肌・茶髪と被るが純色なので識別可) |
| pbropaqueF | `class1/deferred/pbropaqueF.glsl` | `vec3(0, 1, 0)` 純緑 | OK |
| alphaF | `class2/deferred/alphaF.glsl` | `vec3(0, 0, 1)` 純青 | OK |
| materialF (BLEND, `DIFFUSE_ALPHA_MODE==1`) | `class3/deferred/materialF.glsl` | `vec3(1, 1, 0)` 黄 | OK |
| materialF (MASK, `DIFFUSE_ALPHA_MODE!=1` の frag_data[0]) | 同 | `vec3(1, 0, 1)` マゼンタ | gbuffer clear 色 (`pipeline.cpp:1093`) と被るので **避ける**、代わりに `vec3(0, 1, 1)` シアン |
| diffuseAlphaMaskIndexedF | `class1/deferred/diffuseAlphaMaskIndexedF.glsl` | `vec3(1, 0.5, 0)` 橙 | OK |
| fullbrightAlphaMaskF | `class1/deferred/fullbrightAlphaMaskF.glsl` | `vec3(0.5, 0, 1)` 紫 | OK |

**短所**: シェーダー 7 ファイル同時編集、in-world 全 mesh が虹色になり in-world 検証時にシーン認識不能、shader cache clear 必要、戻し忘れリスク。**§2.7.C で代替できるので非推奨。**

---

## §3. §1 と §2 の重ね合わせ (blend / depth / 書き先 RT の確定)

### §3.1 重ね合わせの場所一覧

| 髪の経路 | 重ね合わせの場所 (file:line) | bound RT | blend on/off | SRC factor | DST factor | depth test | depth write | 髪 fragment は背景の RGB を破壊的上書きか blend か |
|---|---|---|---|---|---|---|---|---|
| MASK (gbuffer pass: `PASS_MATERIAL_ALPHA_MASK`, `PASS_ALPHA_MASK`, `PASS_GLTF_PBR_ALPHA_MASK`) | `LLPipeline::renderGeomDeferred` 内 pool loop (`pipeline.cpp:5008-5022`)。`lldrawpoolmaterials.cpp:93` bind + `:295-296` draw / `lldrawpoolsimple.cpp:116-128` / `lldrawpoolpbropaque.cpp:53-69` | `mRT->deferredScreen` (gbuffer MRT) | **OFF** (各 pool で `LLGLDisable blend(GL_BLEND);` か上位の `LLGLSPipeline` default) | — | — | `GL_LEQUAL` | **ON** | **破壊的上書き** (discard した pixel のみ背景が残る) — ただしこの時点では「背景」もまだ gbuffer 段階。`mRT->screen` に出るのは後段 softenLightF blit で `frag_data[0..3]` を入力に lit 計算した結果 |
| BLEND (`PASS_ALPHA` → `LLDrawPoolAlpha::forwardRender`) | `lldrawpoolalpha.cpp:261-307` (`forwardRender`) → `:612-947` (`renderAlpha`)。drawArrays @ `:843` | `mRT->screen` (`LLGLSPipelineAlpha` = `LLGLEnable(GL_BLEND)` + 上流の `screen_target->bindTarget()` @ `pipeline.cpp:10759`、`screen_target->flush()` @ `pipeline.cpp:11150` まで bound 継続) | **ON** | `mColorSFactor = BF_SOURCE_ALPHA` (`lldrawpoolalpha.cpp:280`) | `mColorDFactor = BF_ONE_MINUS_SOURCE_ALPHA` (`lldrawpoolalpha.cpp:281`) | `LLGLDepthTest depth(GL_TRUE, write_depth ? GL_TRUE : GL_FALSE)` (`lldrawpoolalpha.cpp:278`、`write_depth` は rigged or 一部条件で true、最頻 non-rigged blend では **GL_FALSE** = depth read のみ) | rigged のみ ON、それ以外 OFF | **blend** — `out_rgb = src.rgb * src.a + dst.rgb * (1 - src.a)` |

#### blend func の正確な呼び出し (lldrawpoolalpha.cpp:280-284)

```cpp
mColorSFactor = LLRender::BF_SOURCE_ALPHA;           // } regular alpha blend
mColorDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA; // }
mAlphaSFactor = LLRender::BF_ZERO;                         // } glow suppression
mAlphaDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA;       // }
gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);
```

→ `glBlendFuncSeparate(SRC_ALPHA, ONE_MINUS_SRC_ALPHA, ZERO, ONE_MINUS_SRC_ALPHA)` 相当 (color と alpha を separate)。
→ ただし `renderAlpha` 内で per-batch に上書きあり (`lldrawpoolalpha.cpp:831`):

```cpp
gGL.blendFunc((LLRender::eBlendFactor) params.mBlendFuncSrc, (LLRender::eBlendFactor) params.mBlendFuncDst, mAlphaSFactor, mAlphaDFactor);
```

→ batch ごとに `LLDrawInfo::mBlendFuncSrc / mBlendFuncDst` が color factor を上書きできる。SL のほとんどの alpha mesh は default の `SRC_ALPHA / ONE_MINUS_SRC_ALPHA` のままだが、custom blend func も指定可能。**髪が default のままか custom か** は §4 に逃がす。

emissive 2nd pass の blend (`lldrawpoolalpha.cpp:896`):
```cpp
gGL.blendFunc(LLRender::BF_ZERO, LLRender::BF_ONE, LLRender::BF_ONE, LLRender::BF_ONE);
```
→ RGB は **そのまま** (`ZERO` * src + `ONE` * dst)、alpha は加算。emissive (= glow) を `mRT->screen` の alpha チャネルに足し込む。

DoF gate pass の blend (`lldrawpoolalpha.cpp:249-257`):
```cpp
// mask off color buffer writes as we're only writing to depth buffer
gGL.setColorMask(false, false);
renderAlpha(...);
gGL.setColorMask(true, false);
```
→ color 書き込み禁止、depth のみ更新。`renderAlpha(...)` 内の `simple_shader = &gDeferredFullbrightAlphaMaskProgram` で `minimum_alpha = 0.33` (Cinematic OFF) / `1.f` (Cinematic + `RenderDepthOfFieldAlphas==false`) / `0.7f` (Cinematic + `RenderDepthOfFieldAlphas==true`)。

### §3.2 「髪 fragment が背景の RGB を破壊的に上書きするか」 — blend factor からの結論

#### MASK 経路 (DIFFUSE_ALPHA_MODE_MASK 系の髪)

- gbuffer 段階で **blend off + depth write on + discard**
- discard しなかった pixel: gbuffer の `frag_data[0..3]` を **完全に上書き**
- discard した pixel: gbuffer はクリア色 (`pipeline.cpp:1093` `glClearColor(1, 0, 1, 1)` で**マゼンタ**初期化) のまま、その上を後段の opaque material / sky / etc. が上書き
- → 後段で softenLightF が `mRT->screen` に blit する時点では「髪 pixel」「背景 pixel」は **互いに排他** (alpha 0/1 のどちらか)。重ね合わせ計算は無い

#### BLEND 経路 (DIFFUSE_ALPHA_MODE_BLEND 系の髪)

- alpha pool は `mRT->screen` (= softenLightF が描き終わった lit 背景) を **既存 dst** として、`SRC_ALPHA / ONE_MINUS_SRC_ALPHA` で blend
- 髪 fragment の `frag_color.a` (= alphaF の `final_alpha = diffuse.a * vertex_color.a`) が 1.0 なら DST 寄与 0 → **完全上書き**
- `frag_color.a` が 0.5 なら 50/50 mix
- `frag_color.a` が `minimum_alpha (= 0.004)` 未満なら **discard** で背景そのまま
- → `minimum_alpha` ぎりぎりの「**alpha ≈ 0.004〜0.5 程度の半透明 edge pixel**」では `(0.5 * 髪色) + (0.5 * 背景色)` の **blend (=色 mix)** が発生する

#### GLTF PBR BLEND 経路

- §2.2 の通り `HAS_ALPHA_MASK` permutation 無しのため **discard が走らない**
- `basecolor.a` が 0.0001 でも fragment 出力 → `frag_color = vec4(color, basecolor.a * vertex_color.a)` で blend
- `SRC_ALPHA / ONE_MINUS_SRC_ALPHA` のため `src.a → 0` で **dst (= 背景) がそのまま見える** が、shader を一度走らせて lit/IBL 計算した後で blend するので **微妙に色が混ざる** ことは構造上ある

### §3.3 §3 確定度: 確定

「DoF blur 前の `mRT->screen`」の中身は、髪経路ごとに以下が確定:

- MASK 髪: gbuffer 上で背景と髪が排他 (互いに上書き) → softenLightF blit で `mRT->screen` に lit color が 1 度だけ書かれる → blend mix は **無い**
- BLEND 髪: softenLightF が lit 背景を `mRT->screen` に書く → そこに `LLDrawPoolAlpha::forwardRender` が `SRC_ALPHA / ONE_MINUS_SRC_ALPHA` で blend → **edge pixel は背景 RGB と髪 RGB の mix が `mRT->screen` に焼かれている**

### §3.4 重要な副次事実

#### §3.4.1 `mRT->screen` の alpha チャネルは DoF の入力信号 (CoF)

`renderDoF` (`pipeline.cpp:9630-9861`) の Pass 1 (cofF):
- 入力: `mRT->screen` (= `src`、`tonemap()` → glow pass 後の ping-pong) の RGB と `mRT->deferredScreen` の depth
- 出力: `mRT->deferredLight` の **RGB = src.rgb のまま**、**`.a` = CoF (circle of confusion) を符号化した値** (`cofF.glsl:77-78`)

Pass 2 (postDeferredF):
- 入力: Pass 1 出力 (`mRT->deferredLight`)
- 出力: `src` (元の scene buffer に書き戻し) — 各 pixel の `.a` を読んで CoF サイズ分の周辺 sample を平均 (`postDeferredF.glsl:91-149`)
- **重要**: `s.a*2.0-1.0` で signed CoF を取り出し、CoF が一定以上の pixel について `texture(diffuseRect, tc + offset)` で **周辺 pixel から RGB を sample して average** する (line 47, 69-71)

Pass 3 (dofCombineF):
- 入力: blur 結果 (`diffuseRect = src`) + 元 lit (`lightMap = mRT->deferredLight` 経由)
- 出力: `dst` (= `mPostPongMap`) に最終 `mix(diff, dof, a)` (`dofCombineF.glsl:51-75`)

#### §3.4.2 BLEND 髪の edge pixel が DoF で何が起きるか (事実)

- BLEND 髪の edge pixel の `mRT->screen` の値は §3.2 BLEND の結果、**「髪色 * src.a + 背景色 * (1 - src.a)」が RGB に焼き込まれた状態**
- `mRT->screen.a` は §3.2 の `mAlphaSFactor = BF_ZERO / mAlphaDFactor = BF_ONE_MINUS_SOURCE_ALPHA` で更新される → `dst.a = 0 + dst.a * (1 - src.a)` → 元 alpha (= sky/glow 由来) が 1 で `src.a = 0.5` なら `dst.a = 0.5`
- DoF Pass 1 (`cofF`) は **`mRT->screen` の alpha を読まず**、`mRT->deferredScreen` の **depth** を読んで CoF を計算 (`cofF.glsl:65-78`)。CoF は **背景の depth 由来**
- DoF Pass 2 (`postDeferredF`) はその CoF を使って **edge pixel の RGB (= 既に焼き込み済みの mix 色) を周辺 pixel から sample して再平均**
- → 「髪 edge pixel」の周辺に「もっと深い背景の lit pixel」が居れば、その色も含めて re-average される。結果 RGB は **(髪+背景 blend 済み color) と (純背景 color) の更なる mix**

→ AYA さんの「髪の edge で背景が混ざって色シフト」観測は、§3.2 BLEND の blend と §3.4.2 の DoF re-sample の **2 段重ね** が事実として該当する可能性がある。

### §3.5 §3 確定度: 確定 (重ね合わせ blend factor と DoF の sample 範囲まで)

---

## §4. 不明 / 要追加調査

> **2026-05-21 追記 (本書 §2.6 / §2.7 追加に伴う更新)**: §4.2 / §4.5 は本書範囲のコード読みで **解決済** とマーク、§4.6 はコード読みでロジック自体は確定したため「factual ⇒ §3.4.1 補足の方が適切」とリラベル。§4.1 / §4.3 / §4.4 は **実機検証要** のため §2.7 に倣う具体的検証手順を併記。

| # | 不明事項 | 影響範囲 | 状態 | 追加調査方法 |
|---|---|---|---|---|
| 4.1 | AYA さんの観測対象「髪」が §2.6.2 の **実用上 6 経路のうちどれを通っているか** (GLTF PBR BLEND / GLTF PBR MASK / Blinn-Phong material BLEND / Blinn-Phong material MASK / Blinn-Phong + alpha<1 (no MASK) / bare alpha (no mat、rigged ゆえ常に BLEND)) | これが分からないと「§3.2 のどちらが起きているか」が確定できない | **実機検証要** | **§2.7.A (UI 絞り込み) を第一手 → 5 択まで絞り、決定打が必要なら §2.7.C (LL_INFOS hook 1 diff)**。**§2.7.D の色 canary は非推奨** (§2.7 末尾参照)。 |
| 4.2 | 髪 batch の `LLDrawInfo::mBlendFuncSrc / mBlendFuncDst` が default (`SRC_ALPHA / ONE_MINUS_SRC_ALPHA`) か custom か (`lldrawpoolalpha.cpp:831`) | §3.2 BLEND 経路の blend 式が default 想定で正しいかの確認 | **解決済 (2026-05-21)** | `LLDrawInfo` 構造体の field default は `SRC_ALPHA / ONE_MINUS_SRC_ALPHA` (`llspatialpartition.h:149-150`、`llspatialpartition.cpp:4074-4075`)。**書き換える code は `llvopartgroup.cpp:907` (particle のみ) の 1 箇所** で、`llvovolume.cpp` 経由の mesh face では一切上書きされない (`grep -nE "mBlendFunc(Src\|Dst)\s*=" indra/newview` で confirm)。**mesh hair は常に default の `SRC_ALPHA / ONE_MINUS_SRC_ALPHA`** が `lldrawpoolalpha.cpp:831` で再適用される。よって §3.2 BLEND の blend 式は default 想定で正しい。 |
| 4.3 | DoF Pass 1 (`cofF`) で参照する depth は `mRT->deferredScreen` の depth。BLEND 髪は depth を書くか書かないか | 「DoF blur が髪 edge で背景 depth ベースの大きい CoF を使う」ことが事実か | **半解決 (2026-05-21)** | `lldrawpoolalpha.cpp:270-278` で `write_depth = rigged \|\| LLDrawPoolWater::sSkipScreenCopy \|\| LLPipeline::sImpostorRenderAlphaDepthPass \|\| getType() == POOL_ALPHA_PRE_WATER`。**髪が rigged であれば `forwardRender(true)` (line 205) の rigged 1st pass で必ず depth を書く** → 髪 pixel の depth は髪自身の depth で `mRT->deferredScreen` の depth buffer に記録される。**ただし** `LLGLDepthTest depth(GL_TRUE, GL_TRUE)` での write は **fragment shader が discard していない pixel のみ**。BLEND 経路は §2.2 の通り `minimum_alpha` (= 0.004) 未満で discard、それ以上は depth 書く。さらに `renderPostDeferred` 末尾 DoF gate pass (`lldrawpoolalpha.cpp:231-258`) で `minimum_alpha = 0.33 / 0.7 / 1.0` cutoff の color-mask-off depth 書き込みが追加で走る。**実機未検証点**: AYA さんの髪が rigged かどうか (= attachment が `mAvatar != nullptr` & `isRiggedMesh()` で来ているか) と、半透明 edge pixel の alpha が gate cutoff (Cinematic OFF=0.33 / ON+`RenderDepthOfFieldAlphas=true`=0.7 / ON+false=1.0) を上回るか。**実機検証手順**: §2.7.C の hook で `params.mAvatar` と `params.mSkinInfo` を併せて log、rigged 判定確定。alpha 分布は AYA さん主観で「edge pixel が DoF で消えるか残るか」を Cinematic ON / `RenderDepthOfFieldAlphas` ON-OFF で観察。 |
| 4.4 | DoF Pass 1 の最終 `frag_color.a = sc/max_cof*0.5+0.5` の signed CoF が髪/背景 boundary で **どれくらい不連続か** (連続なら blur は smooth、不連続なら edge で sample 不整合)。さらに §3.4.2 の `postDeferredF` で neighbor pixel の `.a` を取って `abs(s.a*2.0-1.0)*max_cof` で sample CoF を計算するため、**中心 pixel の CoF ではなく neighbor pixel の CoF が ゲート閾値 (`if (sc > min_sc)`、`postDeferredF.glsl:62`) として使われる** ことが §3.4.2 trace で確定。これにより「髪 edge (近い、CoF≈0) の中心 pixel に対して、その背景 (遠い、CoF 大) を sample → sample 側の CoF が中心の `min_sc` を超える → 背景色を加重平均に取り込む」現象が起きる | §3.4.2 の re-sample 結果が「色 mix が悪化する」のか「色 mix が緩和される」のかの予測に必要 | **コード読みでロジック確定 (2026-05-21)、実機での連続性は未測定** | コード読みの結論: `postDeferredF.glsl:43-72` の `dofSample` は **neighbor pixel `s.a` の CoF (= neighbor の depth 由来) が「ループ変数 sc」(= 中心 pixel の CoF) を上回る場合のみ** 加重平均に取り込む (`if (sc > min_sc)` line 62)。さらに **`wg = 0.25 + s.r + s.g + s.b`** で **HDR で明るい neighbor ほど重みが大きい** (highlight pop 用)。背景 (例: 明るい空) が髪 edge より HDR で明るければ、背景 sample の `wg` が支配的になり edge pixel の最終 `diff` は背景色寄りに傾く → 観測される「色シフト」の数値的根拠。**実機検証は不要** (本欄と §3.4.2 で式から確定)。実機で「どの方向にどれだけ色が動くか」は scene 依存 (sun_dir、髪色、背景 HDR 値) なので static 解析不可能。 |
| 4.5 | `mRT->screen` の alpha (DoF 入力前) が `renderFinalize` → `tonemap()` → `generateGlow()` → ping-pong の過程でどう変質するか | DoF への影響 | **解決済 (本書時点で確定)** | `cofF.glsl:77-78` で `frag_color.rgb = diff.rgb; frag_color.a = sc/max_cof*0.5+0.5;` → **DoF Pass 1 の出力 alpha は depth 由来の signed CoF 一意で、入力 alpha は廃棄**。よって 4.5 は **DoF 内では非問題**。**追加調査不要**。 |
| 4.6 | DoF depth gate pass (`lldrawpoolalpha.cpp:231-258`) の cutoff (Cinematic OFF=0.33 / ON+`RenderDepthOfFieldAlphas=true`=0.7 / ON+false=1.0) を抜けない髪 pixel は DoF 上 depth = 背景 depth として blur される | 半透明 edge pixel の挙動 | **§3.4.1 補足にリラベル (2026-05-21)** | コード自体は §3.4.1 と本欄 §4.3 で確定済 (条件・閾値・write_depth 経路全て trace 済)。**実機での「edge pixel alpha 分布 vs cutoff」測定** は §4.3 と一体で扱うため §4.3 に統合。本欄は不明事項リストから除外推奨 (確定済事実)。 |

### §4.y AYA さん発言 (2026-05-21) — SL の alpha mesh attachment 構造に関する事実

> 「rigged mesh attachment と non-rigged mesh attachment が Link されていることが多い (SL の髪の毛は)。要するに両方対応しないとダメ」
>
> 「今は髪の毛の話をしていますが、これは服や靴にもいえることです。単に髪の毛がわかりやすかったから取り上げた例に過ぎない」

つまり SL の **全 alpha mesh attachment** (hair / clothing / shoes / accessory) で、1 つの attachment 内に:

- rigged 部 (アバターの骨に追従、§2.6.1 `canRenderAsMask()=false` → PASS_ALPHA BLEND + `write_depth=true` (`lldrawpoolalpha.cpp:270`))
- non-rigged 部 (decoration / ribbon / clip / sole 等、`canRenderAsMask()=true` 可、MASK 経路に落ちる場合あり / BLEND の場合は `write_depth=false`)

が **同時に** 含まれうる。髪はこの構造が最も顕著で観測しやすかったため §2-§3 trace の主例として扱ったが、**本書の fix スコープは全 alpha mesh attachment**。**§4.1 の「mesh が踏む 1 経路を確定」アプローチは意味が薄い** (実物が rigged BLEND + non-rigged BLEND の混在、または rigged BLEND + non-rigged MASK の混在で来うるため、かつ髪以外でも同様)。

**§2 / §3 の事実への影響**:

- §3.4.2 で言及した「BLEND alpha mesh edge pixel の CoF」は **rigged 部については mesh 自身の depth から計算** (write_depth=true)、**non-rigged BLEND 部については背景 depth から計算** (write_depth=false) と **同じ attachment 内で経路が混在**する。
- mask 改善 fix を組む場合、**両経路 (rigged BLEND with depth + non-rigged BLEND without depth + MASK) すべてに同時に効く** 設計が必須、かつ **mesh 種別 (hair / clothing / shoes / accessory) を問わない** ことが必須 (hair 識別ロジックを使ったら服 / 靴で動かない)。

**§4.1 / §4.3 の格下げ**: §2.7.A の UI 絞り込みは「ある瞬間に消えるか」しか分からず、混在 attachment では「Alpha OFF で大部分消える、一部残る」が予想される。実機検証よりも、**Step 2 (Plan) 段階で「rigged BLEND / non-rigged BLEND / MASK の 3 経路を mesh 種別問わず同時に satisfy する mask」** を設計する方が高速。実機検証は Step 2 plan の中で「どの経路の挙動を検証するか」が明確になってから組む。



1. AYAstorm 起動、髪が見える視点。
2. `Develop > Rendering > Types > Alpha` を OFF (Ctrl+Alt+Shift+2)。
3. **髪が完全に消える** → §2.6.2 の **「GLTF BLEND」「Blinn-Phong BLEND」「bare alpha (rigged)」「Blinn-Phong + alpha<1」の 4 経路のどれか** (= alpha pool 系全部)。
4. **髪の一部だけ残る or 全く変わらない** → 残ったのは **Materials pool (S11 MASK 系)** or **GLTF PBR pool (S2/S3)** or **AlphaMask pool (S13、ただし rigged hair では §2.6.1 より発火しない)**。次に `Materials` OFF → 消えれば S11 MASK 確定、変わらなければ `PBR` OFF → 消えれば S2/S3 確定。
5. AYA さんが上記 5 step の観察結果を Claude に報告 → Claude が経路を確定 (alpha pool 内 4 経路の更なる絞り込みは §2.7.C の hook が必要なら追加)。

---

## §5. Mask 精度向上プラン — BD divergent 案 (★ 不採用、検討プロセスの記録)

> **2026-05-21 不採用判定**: 本 §5 の推奨プラン §5.2 (BD Cinematic cutoff を vanilla 0.33 に戻す + `RenderDepthOfFieldAlphas` cvar default OFF→ON) および代替プラン §5.3.A は、いずれも **BD 由来挙動 (Cinematic 時の cutoff=1.0、alpha mesh の DoF depth gate を OFF にする) を変更する** 性質を持つ。AYA さん発言 (2026-05-21) で「r30 BD 完全移植は達成済、ここから先は BD を変えずに追加処理で改善して AYAstorm を BD を上回る描画エンジンに育てる phase」が宣言され、本 §5 は AYAstorm の新方針 (memory: `project_ayastorm_r30_bd_improvement_phase.md`) と矛盾するため **不採用**。
>
> **採用される新方針**: §6 (BD-preserving plan) として「BD の cutoff=1.0 / depth gate OFF を変更せず、postDeferredF 周辺に追加処理を挟んで edge 色シフトを抑える」プランを別途立案 (Step 2 再依頼)。本 §5 は検討プロセスの記録として温存し、削除しない (将来「なぜ BD divergent 案を採らなかったか」を遡れるように)。

> **本章のスコープ (旧)**: §1-§3 / §4.y で事実確定した「全 alpha mesh attachment (hair / clothing / shoes / accessory) edge での DoF 色シフト」を、§1-§3 を引用する根拠ベースで解消するための fix プラン。コード変更は本書では一切行わず、Step 3 (implement agent) が 1 発で書ける粒度で記述する。

### §5.0 解決すべき症状の定義と原因確定

#### §5.0.A 症状定義 (1 行)

**Cinematic mode で DoF blur を有効にすると、alpha mesh attachment (典型例: 髪) の輪郭付近の pixel が背景 RGB と混ざって本来の mesh 色から色シフトする。**

#### §5.0.B 原因 (§1-§3 引用)

3 つの事実の合成で発生:

1. **BLEND 経路の alpha mesh edge pixel は `mRT->screen` 上に「mesh 色 × src.a + 背景色 × (1-src.a)」の mix を焼き込んで commit する** (§3.2 BLEND 経路、`lldrawpoolalpha.cpp:280-284` の `BF_SOURCE_ALPHA / BF_ONE_MINUS_SOURCE_ALPHA`、§3.3 確定度: 確定)。これ自体は forward blend の正常動作で「変えてはいけない」。
2. **Cinematic mode + 既定値では DoF depth-gate pass の alpha cutoff が `1.0f` で、全 alpha pixel が depth に commit されない** (`lldrawpoolalpha.cpp:240-246` の `dof_alpha_cutoff = render_dof_alphas ? 0.7f : 1.f;`、`RenderDepthOfFieldAlphas` default = 0 / Value=`<integer>0</integer>` = `settings.xml:12496-12506`)。結果、alpha mesh の interior / edge どちらの pixel も `mRT->deferredScreen` の depth に自分の深度を書かず、**背景の depth が残る**。
3. **DoF Pass 1 (`cofF`) は CoF を `mRT->deferredScreen` の depth から計算し** (`cofF.glsl:65-73`、pipeline.cpp:9767 で `bindTexture(DEFERRED_DEPTH, &mRT->deferredScreen)`)、**Pass 2 (`postDeferredF::dofSample`) はその CoF (= 中心 pixel の sc) と neighbor pixel の sc を比較して neighbor の `s.a` を取り込むかを決め、取り込み重みは `wg = 0.25 + s.r+s.g+s.b` (HDR で明るい neighbor ほど重い)** (`postDeferredF.glsl:43-72`)。背景 depth で計算された大きい CoF と、HDR で明るい背景 (sky 等) の重みが結びついて、§3.4.2 の通り「edge pixel が背景色寄りに引き寄せられる」現象に至る。

つまり **「Cinematic mode の既定値 `RenderDepthOfFieldAlphas=false` (= cutoff 1.0) が、本来は背景より手前にある alpha mesh の depth を CoF 計算から排除し、結果として alpha edge の周辺 sample 平均が背景色寄りに偏る」** が根本因。**rigged 部については `lldrawpoolalpha.cpp:270` で `write_depth=true` のため forwardRender 第 1 pass で depth を書く** ので、rigged BLEND は cutoff 1.0 でも自分の depth が CoF 計算に乗る。**non-rigged BLEND は `write_depth=false`** のため §5.0.B-2 の depth-gate pass のみが depth 提供手段で、ここが cutoff 1.0 で塞がる。MASK 系は §3.1 の通り gbuffer pass で blend off + depth write on + discard なので **常に正しい depth が乗っており影響を受けない**。

#### §5.0.C §X 訂正候補

なし。§1-§3 と矛盾する事実は本検証で見つからなかった。

---

### §5.1 設計制約 (cf. プロンプト指示)

以下を全て満たすプランのみ §5.2 / §5.3 の候補にする。**1 つでも破ったら無効**:

| # | 制約 | 出典 / 根拠 |
|---|---|---|
| C1 | **3 経路同時 satisfy**: rigged BLEND with depth (§5.0.B-2 + `lldrawpoolalpha.cpp:270` で既に動く) + non-rigged BLEND without depth (現在壊れている) + MASK (gbuffer pass で常に動く、§3.1) の 3 経路すべてで色シフトが起きないこと。1 経路でも動かないプランは却下。 | §4.y、`project_sl_alpha_mesh_rigged_nonrigged_link.md` |
| C2 | **mesh 種別非依存**: hair / clothing / shoes / accessory のどれかを mesh 名 / attachment slot / avatar joint で識別するロジックを使わない。alpha pool / material pool / shader permutation レベルの mesh-agnostic な手段のみ。 | §4.y 末尾、`project_sl_alpha_mesh_rigged_nonrigged_link.md` |
| C3 | **`mRT->screen.a` を fix のために破壊しない**: tonemap / glow / FXAA / SMAA の入力前提を変えない。 | `project_aya_visual_realism_alpha_protect.md`、§3.4.1 |
| C4 | **MRT 追加禁止**: 過去 3 回 (旧 Layered DoF L1/L2、composite pass 追加、`mix(scene, tonemap(near_premul), coverage)` 系) で同種アプローチが外れて revert (HEAD=`e676c52b87`)。MRT 追加を選ぶ場合は §5.5 で正面から根拠を述べた 2nd choice のみ可。 | プロンプト制約、本書 §0 冒頭の revert 履歴 |
| C5 | **新規 cvar 追加最小限**: 出すなら BD parity に乗っている既存名を再利用。 | `feedback_prefer_defaults_over_config.md` |
| C6 | **後段 post-process 副作用ゼロ**: `cofF` / `postDeferredF` / `dofCombineF` / `tonemap` / `glow` のいずれの uniform / 入力 RT 構成も変えない。 | C3 と独立、姉妹資料 `ayastorm-gbuffer3-trace.md` の落とし穴 2 |

---

### §5.2 推奨プラン (1 つ): Cinematic 既定の DoF alpha cutoff を vanilla parity (0.33) に戻す

#### §5.2.A 変更内容 (file:line ベース、Step 3 実装者が判断する余地ゼロ)

##### 変更 1: `lldrawpoolalpha.cpp:240-246` の Cinematic 分岐既定値を vanilla parity 0.33 に戻す

**現状** (`lldrawpoolalpha.cpp:240-246`):

```cpp
F32 dof_alpha_cutoff = 0.33f;
if (LLPipeline::isCinematicMode())
{
    static LLCachedControl<bool> render_dof_alphas(gSavedSettings, "RenderDepthOfFieldAlphas", false);
    dof_alpha_cutoff = render_dof_alphas ? 0.7f : 1.f;
}
simple_shader->setMinimumAlpha(dof_alpha_cutoff);
```

**変更後**:

```cpp
F32 dof_alpha_cutoff = 0.33f;
if (LLPipeline::isCinematicMode())
{
    // <FS:AYAstorm r30 Layered DoF mask fix>
    // RenderDepthOfFieldAlphas は 3 値挙動:
    //   true (BD parity)        → 0.33f  (vanilla / non-Cinematic と同等、alpha mesh 内部 pixel の depth を CoF に乗せて edge 色シフトを抑える)
    //   false (BD optional cut) → 1.f    (BD「alpha 完全除外」、edge 色シフト発生するが BD 互換、被写界深度に alpha を一切入れない撮影用)
    // 既定 BLEND mesh 撮影で色シフトを起こさないよう default は true (0.33)。
    // 旧コードで 0.7f を取っていた中間値は「ほぼ不透明な pixel のみ depth に乗せる」中途半端な動作で、典型 alpha mesh の interior pixel (~0.5) を取りこぼし edge 色シフトの原因。撤去。
    static LLCachedControl<bool> render_dof_alphas(gSavedSettings, "RenderDepthOfFieldAlphas", true);
    dof_alpha_cutoff = render_dof_alphas ? 0.33f : 1.f;
    // </FS:AYAstorm>
}
simple_shader->setMinimumAlpha(dof_alpha_cutoff);
```

##### 変更 2: `settings.xml:12496-12506` の `RenderDepthOfFieldAlphas` 既定値を 0 → 1、Comment 更新

**現状** (`settings.xml:12496-12506`):

```xml
<key>RenderDepthOfFieldAlphas</key>
<map>
  <key>Comment</key>
  <string>If true, transparent surfaces contribute to depth-of-field depth (cutoff 0.7); if false, they are excluded (cutoff 1.0). Cinematic mode のみ。</string>
  <key>Persist</key>
  <integer>1</integer>
  <key>Type</key>
  <string>Boolean</string>
  <key>Value</key>
  <integer>0</integer>
</map>
```

**変更後**:

```xml
<key>RenderDepthOfFieldAlphas</key>
<map>
  <key>Comment</key>
  <string>If true (default), transparent surfaces contribute to depth-of-field depth (cutoff 0.33 = vanilla parity, alpha mesh edge 色シフト抑制); if false, they are excluded (cutoff 1.0 = BD 「alpha 完全除外」撮影用)。Cinematic mode のみ。</string>
  <key>Persist</key>
  <integer>1</integer>
  <key>Type</key>
  <string>Boolean</string>
  <key>Value</key>
  <integer>1</integer>
</map>
```

##### 変更 3: `floater_aya_cinematic.xml` の `RenderDepthOfFieldAlphas` toggle/label に説明追記 (任意、UI 露出が既にあれば)

`grep RenderDepthOfFieldAlphas indra/newview/skins/default/xui/en/floater_aya_cinematic.xml` で発見済 (Step 1.5 grep)。
**Step 3 必須作業**: UI ラベルが「Include alpha in DoF」等で既存 hard 値 0/1 mapping のみなら **変更不要**。tooltip 文言を更新する余地があれば「OFF=BD parity (alpha 完全除外、edge 色シフトあり、撮影向け)」を追記。

#### §5.2.B 3 経路 satisfy の証明 (§1-§3 引用)

| 経路 | 変更前 (HEAD) の挙動 | 変更 1+2 適用後の挙動 | 根拠 |
|---|---|---|---|
| **rigged BLEND** | `lldrawpoolalpha.cpp:270` の `write_depth=true` (rigged 条件) で **既に** forwardRender 第 1 pass (line 205) で mesh 自身の depth を `mRT->deferredScreen` に commit 済。CoF は mesh depth から計算 → 色シフト無し。 | **不変** (cutoff は depth-gate pass にしか効かず、forwardRender 第 1 pass の write_depth には影響しない)。変更前から動く path を壊さない。 | §3.1 表 BLEND 行 `LLGLDepthTest depth(GL_TRUE, write_depth ? GL_TRUE : GL_FALSE)`、`lldrawpoolalpha.cpp:270-278` |
| **non-rigged BLEND** | `write_depth=false` のため forwardRender では depth 書かれない。depth-gate pass (`lldrawpoolalpha.cpp:231-258`) で cutoff 1.0 のため **全 alpha pixel が discard** → depth は **背景のまま**。CoF が背景 depth から計算 → §3.4.2 の HDR-weighted re-sample で背景色寄りに偏る。 | cutoff 0.33 で `final_alpha >= 0.33` の interior pixel が depth-gate pass の `simple_shader->setMinimumAlpha(0.33)` を通過 → color-mask-off depth write (`lldrawpoolalpha.cpp:249-257`) で mesh 自身の depth が `mRT->deferredScreen` に commit。CoF は mesh depth から計算 → 色シフト解消。`final_alpha < 0.33` の edge pixel は依然 depth 不変だが、これは非 Cinematic mode (vanilla / View / Vanilla mode) でも cutoff 0.33 で同じ挙動 → vanilla parity。 | §3.1 表 BLEND 行、§3.2 BLEND 経路、`lldrawpoolalpha.cpp:240` 既存 default 0.33f、`cofF.glsl:65-73` |
| **MASK (gbuffer pass)** | gbuffer pass で blend off + depth write on + discard (§3.1 表 MASK 行)。`PASS_MATERIAL_ALPHA_MASK` / `PASS_ALPHA_MASK` / `PASS_GLTF_PBR_ALPHA_MASK` のいずれも変更前から正しい depth を `mRT->deferredScreen` に commit。 | **不変** (本変更は alpha pool の depth-gate pass のみ。MASK は gbuffer pass で全く別経路、cutoff は無関係)。 | §3.1 表 MASK 行、`pipeline.cpp:5008-5022` の renderGeomDeferred pool loop |

→ **3 経路すべて変更後に色シフト無しで動くことが §1-§3 の事実から確定**。

#### §5.2.C mesh 種別非依存性の証明

変更 1+2 は `lldrawpoolalpha.cpp:240-246` の **alpha pool 全体に対する uniform `minimum_alpha`** を操作するのみ。`LLDrawInfo::mAvatar` / `mGLTFMaterial` / `mTexture` の値による分岐は一切無く、batch ループ内で `params.mTexture` 等を見る箇所も触らない。hair / clothing / shoes / accessory のいずれであっても **alpha pool に来た時点で同一の cutoff が適用される** → C2 satisfy。

#### §5.2.D C3 / C6 (副作用ゼロ) の証明

- `mRT->screen.a` は本変更で**一切触らない** (alpha pool の `mAlphaSFactor = BF_ZERO / mAlphaDFactor = BF_ONE_MINUS_SOURCE_ALPHA` も不変、`lldrawpoolalpha.cpp:282-283`)。tonemap / glow / FXAA / SMAA の入力 alpha は変更前と完全に同じ → C3 satisfy。
- `cofF` / `postDeferredF` / `dofCombineF` の shader source / uniform / 入力 RT 構成は **一切触らない**。変わるのは「depth-gate pass で何 pixel が `mRT->deferredScreen` の depth に書かれるか」の 1 点のみで、これは Cinematic OFF / View / Vanilla mode と同じ動作に戻すだけ。後段の post-process は同じ前提で動く → C6 satisfy。

#### §5.2.E C4 / C5 satisfy

- MRT 追加なし → C4 satisfy。
- 新規 cvar 追加なし。既存 `RenderDepthOfFieldAlphas` の default 値変更と内部分岐の値 (0.7 → 0.33) 変更のみ → C5 satisfy。

---

### §5.3 代替プラン

#### §5.3.A 2nd choice: depth-gate pass の cutoff だけ 0.33 にしつつ cvar default は OFF のまま据え置く

**内容**: 変更 1 で `dof_alpha_cutoff = render_dof_alphas ? 0.33f : 0.33f;` (Boolean に関係なく常に 0.33)、変更 2 をしない (settings.xml default = 0 のまま、cvar は実質 dead)。

**Trade-off**:
- ○ `RenderDepthOfFieldAlphas` を OFF にしている既存ユーザーへの「動作変更通知」が不要 (cvar 既定変更がないため)
- × Cinematic mode で「alpha を完全に DoF depth から外したい撮影者」(BD parity) の選択肢が消える
- × cvar が dead code 化 → C5 (最小限) の精神に反する (将来削除候補になる)
- × 推奨プランと比べ「明示的に default 行動を切替えた」record が cvar に残らず、release note でしか追えない

→ ユーザー選択肢 (BD parity) を残す観点で推奨プランの方が上位。

#### §5.3.B 3rd choice: 推奨プランに加えて MASK 系の `minimum_alpha` も vanilla parity に揃え直す

**内容**: 推奨プランに加え、`lldrawpoolalpha.cpp:130` の `MINIMUM_ALPHA = 0.004f` と、`alphaMask()` の `bias = 0.001953125f` (`class3/deferred/materialF.glsl:262`) を見直し、`pbralphaF.glsl:139-144` の `HAS_ALPHA_MASK` permutation を `gDeferredPBRAlphaProgram` setup (`llviewershadermgr.cpp:1556-1605`) で有効化して PBR BLEND 経路にも discard を入れる。

**Trade-off**:
- ○ PBR BLEND 経路 (§2.2 末尾) で「`basecolor.a` 極小値の fragment が無駄に shader 走って blend する」コストを削減
- × **§1-§3 の事実上、PBR BLEND 経路の色シフトは推奨プランで既に解消する** (PBR BLEND も `LLDrawPoolAlpha` に来るので §5.2.B non-rigged BLEND 行と同じ依存関係)。追加変更は色シフト fix とは独立な最適化で、本書のスコープ外
- × `gDeferredPBRAlphaProgram` の permutation 追加は **PBR BLEND 経路の全 fragment** に影響、未知の retrocompat 退行を持ち込みうる (LL 上流が permutation を意図的に外している経緯不明)
- × `MINIMUM_ALPHA = 0.004f` の変更は alpha pool 全 shader (`prepare_alpha_shader` で `setMinimumAlpha(MINIMUM_ALPHA)`、`lldrawpoolalpha.cpp:130`) に波及 → 既存の正常動作も変わる

→ スコープ外 / 副作用大。**非採用** (記載のみ、Step 3 では着手しない)。

#### §5.3.C 不採用 approach (記述禁止だが復活防止のため明示) — 提案しない

- alpha pool に MRT attachment 追加 (= Layered DoF 旧 L1/L2)
- composite pass を post-tonemap に追加
- `mix(scene, tonemap(near_premul), coverage)` 系の色合成

これらは過去 3 回外して revert (HEAD=`e676c52b87`) と AYA さん警告 (Step 1-1.5)。本書 §5.3 では一切提案しない。

---

### §5.4 Step 3 (implement agent) 引き継ぎ仕様

#### §5.4.A 触る file 一覧 (推奨プラン §5.2 の場合)

| # | file | 行 | 変更種別 |
|---|---|---|---|
| 1 | `indra/newview/lldrawpoolalpha.cpp` | 240-246 | C++ 編集 (cutoff 値変更 + default true 化 + コメント更新) |
| 2 | `indra/newview/app_settings/settings.xml` | 12496-12506 | XML 編集 (`<integer>0</integer>` → `<integer>1</integer>` + Comment 更新) |
| 3 | `indra/newview/skins/default/xui/en/floater_aya_cinematic.xml` | 該当 toggle | (任意) tooltip 文言更新のみ。toggle 値 mapping は変更不要 |

#### §5.4.B CMakeLists 変更要否: **不要**

新規 file の追加なし。既存 `lldrawpoolalpha.cpp` / `settings.xml` / `floater_aya_cinematic.xml` の編集のみ → CMakeLists.txt 変更不要。

#### §5.4.C shader recompile 要否: **不要**

shader file は **1 行も触らない**。`postDeferredF.glsl` / `cofF.glsl` / `dofCombineF.glsl` / `alphaF.glsl` / `pbralphaF.glsl` / `materialF.glsl` のいずれも変更なし。
shader cache clear (`rm -rf ~/.ayastorm_x64/cache/shader_cache/`) は **不要**。

#### §5.4.D build 種別: full autobuild

C++ (`lldrawpoolalpha.cpp`) を編集するため shader-only fast-iterate (memory `feedback_shader_only_fast_iterate.md`) は適用不可。**full autobuild → install → 起動の通常フロー** (memory `project_build_procedure.md`)。

#### §5.4.E 検証手順 (AYA さん依頼前に Claude 側で確認、`feedback_self_verify_before_handoff.md`)

##### Claude 側 self-verify (build 前)

1. `lldrawpoolalpha.cpp:240-246` の編集後の C++ をもう一度 Read し、syntax error / typo がないこと、`LLCachedControl<bool>` の使い方が同 file 内の他箇所と一致していることを確認
2. `settings.xml:12496-12506` の `<integer>1</integer>` への変更が他の同名 key と衝突しないこと (`grep -c "RenderDepthOfFieldAlphas" settings.xml` で 1 件のみ)
3. `lldrawpoolalpha.cpp:230-258` の depth-gate pass 条件 (`!sImpostorRender && (LLPipeline::RenderDepthOfField || volumetric_wants_alpha_depth) && !gCubeSnapshot && !sRenderingHUDs && getType() == POOL_ALPHA_POST_WATER`) が変更前と同じであること = HUD / impostor / cube snapshot で誤発火しないこと
4. `floater_aya_cinematic.xml` の `RenderDepthOfFieldAlphas` toggle に hard-coded `<initial_value>0</initial_value>` 等が無く、cvar から動的に読む形であることを確認 (= settings.xml default 変更が UI に正しく反映されること)
5. `RenderEnableEmissiveBuffer` 等の他 Persist=1 cvar との挙動干渉が無いことを `grep -nE "RenderDepthOfFieldAlphas|RenderDepthOfField" indra/newview` で再確認

##### build 後 AYA さん依頼用検証手順

1. **Cinematic mode ON、`RenderDepthOfFieldAlphas` 未設定 (= 新 default = 1) の状態で AYAstorm 起動**
2. DoF が効く視点 (focus point を手前 / 奥の対比のある場所に置く) で alpha mesh attachment (hair / clothing / shoes / accessory のいずれか 2 種以上) を視野に入れる
3. focus を **alpha mesh 自身** に合わせて DoF blur が背景に強く効く状態にする → alpha mesh の **edge が背景色寄りに引っ張られないこと** を確認 (= 推奨プラン期待結果)
4. focus を **背景** に合わせて DoF blur が alpha mesh に強く効く状態にする → alpha mesh の **interior が均一に blur されて edge artifact が無いこと** を確認
5. `Debug Settings` で `RenderDepthOfFieldAlphas` を **false に切替**、再起動なしで同じ shot を見る → BD parity 動作 (alpha 完全除外、edge 色シフト復活) に切替わることを確認 (= UI 経由の opt-out が機能している証明)
6. `Debug Settings` で `RenderDepthOfField` を **false に切替** → DoF 自体が無効化されて全画面 sharp になることを確認 (= DoF gate pass の条件分岐が壊れていない証明)
7. **mesh 種別非依存性確認**: hair attachment と clothing attachment を **両方視野に入れた状態** で focus を移動、両方で同様の edge 改善が観測されることを目視 (C2 satisfy 観測)
8. **rigged + non-rigged Link attachment 確認**: 髪 (rigged + non-rigged decoration link) で edge が **両部分で** 改善すること (C1 satisfy 観測、§4.y)
9. 確認後、`RenderDepthOfFieldAlphas` を default (1) に戻して `feedback_restore_debug_settings.md` 準拠 (本変更では default 値そのものが変わるので「戻す」は不要、検証用に変えた場合のみ)

#### §5.4.F 検証 log hook 要否

推奨プランは **挙動が外部 (画面) で目視確認可能** なため、`LL_INFOS` hook の追加は不要。**§2.7.C 形式の hook は追加しない** (= `feedback_remove_verification_logs.md` の commit-前-除去ステップ自体が不要)。

#### §5.4.G commit 前 cleanup チェックリスト

- [ ] §5.4.A の 2-3 file のみ変更されていること (`git diff --name-only`)
- [ ] shader file (`*.glsl`) が変更されていないこと
- [ ] CMakeLists.txt が変更されていないこと
- [ ] 追加 LL_INFOS / debug print が残っていないこと (`grep -n "LL_INFOS" lldrawpoolalpha.cpp` で本変更による新規行がないこと)
- [ ] 本書 §5 / `docs/release/` / `docs/specs/` の文章 update が同 commit に含まれるか別 commit かを AYA さんと事前合意

---

### §5.5 想定 risk と評価 (§1-§3 引用)

#### §5.5.A R1: alpha 破壊リスク (`project_aya_visual_realism_alpha_protect.md`)

- **内容**: `mRT->screen.a` を fix の過程で破壊して sky / fog の mask が壊れる
- **評価**: **発生しない**。本変更は `lldrawpoolalpha.cpp:230-258` の depth-gate pass の `minimum_alpha` cutoff だけを動かす。depth-gate pass は `gGL.setColorMask(false, false)` (line 250) で **color buffer 書き込みを明示的に OFF** にしているため、`mRT->screen` (RGB / A 含む) は本変更の前後で完全に同一。tonemap / glow / FXAA / SMAA の入力前提は変わらない (§5.2.D)。

#### §5.5.B R2: 他 post-process 汚染

- **内容**: cofF / postDeferredF / dofCombineF / tonemap / glow の入力構成が変わって後段が誤動作
- **評価**: **発生しない**。本変更は shader / uniform / 入力 RT を一切触らない。変わるのは `mRT->deferredScreen` の depth buffer に「non-rigged BLEND interior pixel の自身の depth が書かれる」だけで、これは Cinematic OFF mode (= vanilla / View / Vanilla mode) では既に標準動作。後段はこの depth を従来通り読むだけで動作変更を要求しない (`cofF.glsl:65-73`、`postDeferredF.glsl:43-72`、§5.2.D)。

#### §5.5.C R3: FPS 退行

- **内容**: 「alpha pixel が増えて DoF depth-gate pass の depth write が増える → fragment 数増 → FPS 退行」
- **評価**: **小**。depth-gate pass は color-mask-off で depth のみ書く軽量 pass。cutoff 1.0 → 0.33 で write 対象 pixel が増えるが、shader 内で `alphaMask()` discard により `final_alpha < 0.33` の pixel は依然 GPU 段階で discard される。alpha mesh の interior pixel (典型的に `final_alpha = 0.5 - 1.0` の部分) で depth が書かれる程度。これは Cinematic OFF / View / Vanilla mode の vanilla 標準動作と同じ負荷で、AYAstorm が Cinematic mode で意図的に重くしている他要素 (volumetric / shadow / etc.) に比べて誤差レベル。実機 FPS 計測は §5.4.E の検証手順 8 と同時に実施推奨。

#### §5.5.D R4: 既存 cvar 衝突

- **内容**: `RenderDepthOfFieldAlphas` default 変更 (0→1) が、他 cvar の Persist=1 と組み合わさって意図せぬ挙動を生む
- **評価**: **小**。`grep -nE "RenderDepthOfFieldAlphas" indra/newview` の結果 (settings.xml + floater_aya_cinematic.xml の 2 file のみ) で参照箇所は `lldrawpoolalpha.cpp` の `LLCachedControl` 経由 1 箇所 + UI 1 箇所のみ。他 cvar との分岐結合は存在しない。**ただし AYA さん既存 user で `RenderDepthOfFieldAlphas=false` を明示設定している場合、Persist=1 のため新 default は適用されず旧挙動 (cutoff 1.0) が残る** → これは「明示 opt-out したユーザーの意思を尊重する」正しい挙動。release note で「default 変更 / 明示 false 設定者は手動で true に戻すか default 維持か検討してください」と注意喚起する必要あり。

#### §5.5.E R5: BD parity 退行

- **内容**: 「BD は `RenderDepthOfFieldAlphas=false` で alpha 完全除外が default」と AYA さんが BD full port 方針 (`feedback_bd_full_port_only.md`) で期待している場合、本変更は BD 方針と矛盾
- **評価**: **要 AYA さん判断**。`lldrawpoolalpha.cpp:237-247` の現状コメント (`Phase 6 step 2: Cinematic mode honors BD RenderDepthOfFieldAlphas (default OFF)`) より、現 HEAD は **BD の default OFF を意図的に踏襲している**。本変更は AYAstorm 既定 (BD baseline) を BD と意図的に divergent させる判断を含む。`feedback_warn_aya_off_bd_line.md` に従い、Step 3 着手前に AYA さんに「BD parity を切って AYAstorm 独自既定 (= alpha mesh 撮影で edge 色シフトしない) に倒すこと」が方針として OK か明示確認が必要。
- **代案**: もし BD parity 維持必須なら §5.3.A (cvar default は OFF 据え置きで cutoff 値だけ 0.33 にする) も判断材料に含めて AYA さんに 2 択を提示。

---

## §6. Mask 精度向上プラン — BD-preserving 版 (★ 採用、Step 2.1)

> **位置づけ (2026-05-21)**: §5 (BD divergent 案) が AYA さん判断で不採用となり、新方針 (memory: `project_ayastorm_r30_bd_improvement_phase.md`) 「BD 由来挙動は変えない、上流 / 下流に **追加処理** を挟んで AYAstorm が BD を上回る描画エンジンに育てる」に切り替え。本 §6 はその新方針に従った Plan で、§1-§3 / §4 / §5.0.A の症状定義をそのまま流用しつつ、修正対象を **`postDeferredF.glsl` の `dofSample` 内に neighbor pixel 取り込みを抑制する追加項 (depth-aware edge guard) を `&&` 接続で挿入** することに置く。BD 由来コードは 1 文字も書き換えず、すべて **既存式に AND 条件を 1 個重ねる** 形で追加処理を入れる。

### §6.0 解決すべき症状の 1 行定義と原因確定

#### §6.0.A 症状定義 (1 行、§5.0.A から流用、変更なし)

**Cinematic mode で DoF blur を有効にすると、alpha mesh attachment (典型例: 髪) の輪郭付近の pixel が背景 RGB と混ざって本来の mesh 色から色シフトする。**

#### §6.0.B 原因 (§1-§3 引用、§5.0.B と同根)

3 つの事実の合成 (§5.0.B 参照、3 点とも本 Plan でも有効):

1. **BLEND 経路の alpha mesh edge pixel は `mRT->screen` 上に「mesh 色 × src.a + 背景色 × (1-src.a)」の mix を焼き込んで commit する** (§3.2 BLEND、`lldrawpoolalpha.cpp:280-284`)。これは forward blend の正常動作で「変えてはいけない」。**§6 では触らない**。
2. **Cinematic mode + 既定 `RenderDepthOfFieldAlphas=false` のとき DoF depth-gate pass の cutoff が `1.0f` で、non-rigged BLEND の interior / edge どちらの pixel も `mRT->deferredScreen` の depth を更新しない** (`lldrawpoolalpha.cpp:240-246`)。結果 `mRT->deferredScreen` の depth は **背景 depth** のまま。**§6 では「BD parity 維持」のためここを変えない (制約 C7)**。
3. **DoF Pass 2 (`postDeferredF::dofSample`) は neighbor pixel `s.a` から復元した CoF (`sc = abs(s.a*2.0-1.0)*max_cof`) を中心 pixel の `min_sc` と比較するのみで、neighbor pixel が「中心 pixel と十分連続的な depth」か否かを判定しない** (`postDeferredF.glsl:43-72`、現 HEAD)。さらに `wg = 0.25 + s.r+s.g+s.b` で HDR で明るい neighbor ほど重い → 背景 (空 / 強照明) sample が支配的になり alpha mesh edge が背景色寄りにずれる (§3.4.2)。

**根本因の再定義 (§5.0.B との差分)**: §5 plan は (2) を BD divergent に変えて root を消す案、§6 plan は (2) を BD parity 保ったまま、(3) で「neighbor が中心と非連続 depth なら取り込まない」追加 guard を入れて、(2) の depth 不整合が `dofSample` の合成に反映されないようにする案。

##### §6.0.C 「BD HQ DoF path が既にこの guard を持っている」事実

`indra/newview/app_settings/shaders/class1/deferred/postDeferredHQDoFF.glsl:50-78` (= BD 完全移植 path):

```glsl
void dofSample(inout vec4 diff, inout float w, float min_sc, vec2 tc, float depth)
{
    vec4 s = texture(diffuseRect, tc);
    float sc = abs(s.a*2.0-1.0)*(max_cof*4);
#if HAS_DOF_CHROMA
    ...
#endif

    if(s.a <= depth*0.50)       // ★ BD 由来の depth-aware guard
    {
        if (sc > min_sc)
        {
            float wg = 0.25;
            wg += s.r + s.g + s.b;
            diff += wg * s;
            w += wg;
        }
    }
}
```

- BD HQ path は **`s.a <= depth*0.50` (= neighbor pixel が CoF 平面上で中心の半分より「焦点に近い」)** の場合のみ取り込む = **depth 不整合を抑える役割を既に果たしている**。
- ただし HQ path は `RenderDepthOfFieldHighQuality` cvar (default 0、`settings.xml:12230-12240`) が ON のときのみ走る選択肢。標準 (`postDeferredF.glsl`) には同 guard が無い。
- BD HQ の guard 式 (`s.a <= depth*0.50`) は **BD 完全移植コードそのもの = 触らない**。本 Plan ではこの **同じ式** を標準 `postDeferredF.glsl` にも **permutation で添加** し、HQ / 標準どちらの path でも同じ depth-aware guard が効くようにする。BD HQ path は 1 行も書き換えない (= C7 satisfy)。

---

### §6.1 設計制約 (§5.1 全制約 + 新規 C7)

| # | 制約 | 出典 / 根拠 |
|---|---|---|
| C1 | **3 経路同時 satisfy**: rigged BLEND with depth + non-rigged BLEND without depth + MASK の 3 経路すべて。1 経路でも動かないプランは却下。 | §4.y、`project_sl_alpha_mesh_rigged_nonrigged_link.md` |
| C2 | **mesh 種別非依存**: hair / clothing / shoes / accessory のいずれも識別ロジック不可。 | §4.y |
| C3 | **`mRT->screen.a` 破壊禁止**: tonemap / glow / FXAA / SMAA の入力前提を変えない。 | `project_aya_visual_realism_alpha_protect.md`、§3.4.1 |
| C4 | **MRT 追加禁止**: 過去 3 回失敗。 | プロンプト、本書 §0 冒頭 |
| C5 | **新規 cvar 最小限**: 出すなら BD parity に乗っている既存名再利用。 | `feedback_prefer_defaults_over_config.md` |
| C6 | **後段 post-process 副作用ゼロ**: `cofF` / `dofCombineF` / `tonemap` / `glow` の uniform / 入力 RT 構成を変えない。 | C3 と独立、`ayastorm-gbuffer3-trace.md` |
| **C7 (新規)** | **BD 由来挙動は一切変更しない**。具体的に: (a) `lldrawpoolalpha.cpp:240-246` Cinematic 分岐 cutoff 値 (1.0/0.7/0.33)、(b) `settings.xml` `RenderDepthOfFieldAlphas` default (= 0)、(c) `lldrawpoolalpha.cpp` の alpha pool draw / depth gate ロジック、(d) `cofF.glsl` / `dofCombineF.glsl` / `postDeferredHQDoFF.glsl` の既存ロジックの **書き換え**、(e) Cinematic mode の入口 / 出口処理、いずれも触らない。追加処理 (= 既存式に AND 条件を重ねる / 別 helper を呼ぶ / permutation で `#if` ガードした分岐を新設する) は OK。 | `project_ayastorm_r30_bd_improvement_phase.md` (AYA さん 2026-05-21 発言、本書 §5 不採用判定) |

---

### §6.2 推奨プラン (1 つ): `postDeferredF.glsl` に BD HQ path 由来の depth-aware edge guard を permutation 添加

#### §6.2.A プランの 1 行サマリ

**標準 `postDeferredF.glsl` の `dofSample` 内に、新規 permutation `HAS_ALPHA_EDGE_GUARD` で gate された depth-aware guard を `if (sc > min_sc)` 判定の AND 条件として追加。guard 式は BD HQ path (`postDeferredHQDoFF.glsl:65`) と同一 (`s.a <= depth*0.50`)。permutation 無効時 (= BD parity / vanilla Firestorm) は完全 no-op、有効時 (= AYAstorm Cinematic mode default) は neighbor の depth 不整合 sample を捨てて alpha edge 色シフトを抑える。BD HQ 移植 file は 1 文字も書き換えない。**

#### §6.2.B 変更内容 (file:line ベース、Step 3 実装者が判断する余地ゼロ)

##### 変更 1: `indra/newview/app_settings/shaders/class1/deferred/postDeferredF.glsl` に depth-aware guard 追加 (line 28-72 範囲)

**現状の `dofSample` (line 43-72)**:

```glsl
uniform sampler2D diffuseRect;

uniform mat4 inv_proj;
uniform vec2 screen_res;
uniform float max_cof;
uniform float res_scale;

// <AYAstorm r30 P4 step 1> BD chroma uniform (gated by HAS_DOF_CHROMA permutation)
uniform float chroma_str;
// </AYAstorm r30 P4 step 1>

in vec2 vary_fragcoord;

void dofSample(inout vec4 diff, inout float w, float min_sc, vec2 tc)
{
    vec4 s = texture(diffuseRect, tc);

    float sc = abs(s.a*2.0-1.0)*max_cof;

// <AYAstorm r30 P4 step 1> BD HAS_DOF_CHROMA: per-channel R/G/B offset sampling
#if HAS_DOF_CHROMA
    ...
#endif

    if (sc > min_sc) //sampled pixel is more "out of focus" than current sample radius
    {
        float wg = 0.25;
        wg += s.r+s.g+s.b;
        diff += wg*s;
        w += wg;
    }
}
```

**変更後 (line 28-72 範囲、追加分のみ示す、既存行は完全保持)**:

```glsl
uniform sampler2D diffuseRect;

// <AYAstorm r30 BD-preserving alpha edge guard> depthMap uniform を追加。
// gDeferredPostProgram は既に pipeline.cpp:9804 で DEFERRED_DEPTH を bind 済
// (BD HQ path 用に r30 P4 step 4 で追加された)。標準 postDeferredF からも同じ
// depth texture を利用できる。Cinematic OFF / 非 AYAstorm では HAS_ALPHA_EDGE_GUARD
// permutation が立たないので uniform は宣言のみで未参照、ドライバ最適化で剥がれる。
#if HAS_ALPHA_EDGE_GUARD
uniform sampler2D depthMap;
#endif
// </AYAstorm>

uniform mat4 inv_proj;
uniform vec2 screen_res;
uniform float max_cof;
uniform float res_scale;

uniform float chroma_str;

in vec2 vary_fragcoord;

void dofSample(inout vec4 diff, inout float w, float min_sc, vec2 tc
// <AYAstorm r30 BD-preserving alpha edge guard> center pixel depth を引数で渡す。
// guard 式 (s.a <= depth*0.50) は BD HQ path postDeferredHQDoFF.glsl:65 と同一。
#if HAS_ALPHA_EDGE_GUARD
    , float depth
#endif
// </AYAstorm>
)
{
    vec4 s = texture(diffuseRect, tc);

    float sc = abs(s.a*2.0-1.0)*max_cof;

#if HAS_DOF_CHROMA
    ...
#endif

    if (sc > min_sc
// <AYAstorm r30 BD-preserving alpha edge guard> AND で BD HQ 由来 depth guard。
// 既存式 `sc > min_sc` は一切書き換えず、AND 1 条件のみ追加。permutation OFF で完全 no-op。
#if HAS_ALPHA_EDGE_GUARD
        && (s.a <= depth*0.50)
#endif
// </AYAstorm>
        )
    {
        float wg = 0.25;
        wg += s.r+s.g+s.b;
        diff += wg*s;
        w += wg;
    }
}
```

`main()` 側 (line 91-150) の `dofSample` 呼び出し箇所 (line 139) も同様に permutation で center depth を渡す:

```glsl
void main()
{
    vec2 tc = vary_fragcoord.xy;

    vec4 diff = texture(diffuseRect, vary_fragcoord.xy);

// <AYAstorm r30 BD-preserving alpha edge guard>
#if HAS_ALPHA_EDGE_GUARD
    float depth = texture(depthMap, tc).r;
#endif
// </AYAstorm>

    {
        float w = 1.0;
        ...
        if (sc < -0.5)
#endif
        {
            sc = abs(sc);
            while (sc > 0.5)
            {
                int its = int(max(1.0,(sc*3.7)));
                for (int i=0; i<its; ++i)
                {
                    float ang = sc+i*2*PI/its;
                    float samp_x = sc*sin(ang);
                    float samp_y = sc*cos(ang);
                    dofSample(diff, w, sc, vary_fragcoord.xy + (vec2(samp_x,samp_y) / screen_res)
#if HAS_ALPHA_EDGE_GUARD
                        , depth
#endif
                    );
                }
                sc -= 1.0;
            }
        }
        ...
    }
    ...
}
```

**重要原則**:
- 既存の `if (sc > min_sc)` の **式は一切変更しない**。`&&` で **追加項を 1 個重ねる** だけ。
- `dofSampleNear` (前景 sample、line 75-87) は **触らない**。これは `FRONT_BLUR` 経路で「中心 pixel が手前」のときに走る分岐で、中心 pixel 自身が alpha edge の場合の対称な depth guard は HQ path にもなく BD 由来でない (= scope 外、§6.5 R3 参照)。
- `clampHDRRange` 呼び出し (line 89, 148) は **触らない**。

##### 変更 2: `indra/newview/llviewershadermgr.cpp:3010-3050` の `gDeferredPostProgram` setup に `HAS_ALPHA_EDGE_GUARD` permutation 追加

**現状 (line 3010-3050)**:

```cpp
if (success)
{
    gDeferredPostProgram.mName = "Deferred Post Shader";
    gDeferredPostProgram.mFeatures.isDeferred = true;
    gDeferredPostProgram.mShaderFiles.clear();
    gDeferredPostProgram.clearPermutations();
    gDeferredPostProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER));

    static LLCachedControl<U32>  aya_view_mode_post(gSavedSettings, "AYAVisualRealismEnabled", 1);
    if (aya_view_mode_post == 2)
    {
        static LLCachedControl<bool> hq_dof(gSavedSettings, "RenderDepthOfFieldHighQuality", false);
        gDeferredPostProgram.mShaderFiles.push_back(make_pair(
            hq_dof ? "deferred/postDeferredHQDoFF.glsl" : "deferred/postDeferredF.glsl",
            GL_FRAGMENT_SHADER));

        static LLCachedControl<bool> dof_chroma_post(gSavedSettings, "RenderDepthOfFieldChroma", true);
        if (dof_chroma_post)
        {
            gDeferredPostProgram.addPermutation("HAS_DOF_CHROMA", "1");
        }

        static LLCachedControl<bool> dof_front(gSavedSettings, "RenderDepthOfFieldFront", true);
        if (dof_front)
        {
            gDeferredPostProgram.addPermutation("FRONT_BLUR", "1");
        }
    }
    else
    {
        gDeferredPostProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredF.glsl", GL_FRAGMENT_SHADER));
    }
    gDeferredPostProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
    success = gDeferredPostProgram.createShader();
}
```

**変更後** (`aya_view_mode_post == 2` ブランチ内の **HQ DoF を選んでいない側**、すなわち標準 `postDeferredF.glsl` を使う側に **限定して** permutation 追加。HQ DoF を選んだ場合は BD HQ shader が既に depth guard を持つので追加不要):

```cpp
if (aya_view_mode_post == 2)
{
    static LLCachedControl<bool> hq_dof(gSavedSettings, "RenderDepthOfFieldHighQuality", false);
    gDeferredPostProgram.mShaderFiles.push_back(make_pair(
        hq_dof ? "deferred/postDeferredHQDoFF.glsl" : "deferred/postDeferredF.glsl",
        GL_FRAGMENT_SHADER));

    // <AYAstorm r30 BD-preserving alpha edge guard>
    // 標準 postDeferredF.glsl path に、BD HQ path 由来の depth-aware edge guard を permutation で添加。
    // HQ path は BD 由来コードに同等 guard を既に持つため、permutation を立てない (BD 完全移植維持)。
    // 標準 path は AYAstorm Cinematic mode default で walk するため、ここで guard が立つことで
    // alpha mesh edge 色シフトを抑える。Cinematic OFF (mode 0/1) では本 else ブランチで
    // permutation 自体宣言されないので完全 no-op (vanilla 動作維持)。
    if (!hq_dof)
    {
        gDeferredPostProgram.addPermutation("HAS_ALPHA_EDGE_GUARD", "1");
    }
    // </AYAstorm>

    static LLCachedControl<bool> dof_chroma_post(gSavedSettings, "RenderDepthOfFieldChroma", true);
    ...
}
else
{
    gDeferredPostProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredF.glsl", GL_FRAGMENT_SHADER));
    // 注: ここでは HAS_ALPHA_EDGE_GUARD permutation を立てない。
    //     mode 0/1 は vanilla Firestorm 動作維持 (BD 由来でない既存仕様を一切変えない、C7 の精神)。
}
```

**`clearPermutations()` (line 3019) は既存呼び出しのまま**。`HAS_ALPHA_EDGE_GUARD` も他 permutation 同様に rebuild 跨ぎ stale 残りを起こさない。

##### 変更 3: cvar 追加なし

本 Plan は **新規 cvar を 1 個も足さない** (C5)。既存 `AYAVisualRealismEnabled` / `RenderDepthOfFieldHighQuality` の組み合わせで permutation の有無が完全に決まる。

##### 変更 4: bind 側 (`pipeline.cpp`) 変更不要

`pipeline.cpp:9804` で既に `gDeferredPostProgram.bindTexture(LLShaderMgr::DEFERRED_DEPTH, &mRT->deferredScreen, true);` が呼ばれている (BD HQ path 用に r30 P4 step 4 で追加済)。標準 path で同じ depth を読むのに **追加 bind は不要**。

##### 変更 5: `settings.xml` 編集不要

cvar default 変更なし、新規 cvar なし → settings.xml 触らない (= C7-(b) satisfy: `RenderDepthOfFieldAlphas` default = 0 を維持)。

#### §6.2.C 3 経路 satisfy の証明 (§1-§3 引用、Cinematic mode + AYA View Mode = 2 前提)

| 経路 | HEAD 挙動 (§1-§3) | 変更 1+2 適用後の挙動 | 根拠 |
|---|---|---|---|
| **rigged BLEND** | `lldrawpoolalpha.cpp:270` で `write_depth=true`、forwardRender 第 1 pass (`lldrawpoolalpha.cpp:205`) で mesh 自身の depth を `mRT->deferredScreen` に commit。CoF は mesh depth 由来 (`cofF.glsl:65-78`)。`postDeferredF::dofSample` で neighbor sample 時、neighbor 側が背景なら **背景 depth から計算された `s.a` (= 中心 mesh depth より大きい CoF を encode)** が出る。中心 pixel `depth` (= mesh 自身、`postDeferredF.glsl` で `texture(depthMap, tc).r`) と比べて `s.a > depth*0.50` (= 中心 mesh より遠い背景 sample) なら新 guard で `dofSample` 取り込み skip → 背景色寄り偏りが消える。 | 改善。**rigged BLEND の場合「中心 = mesh、neighbor = 背景」のときに background 取り込みを抑え、edge 色シフトを抑える**。 | `lldrawpoolalpha.cpp:270-278`、§3.1 BLEND 行、`postDeferredHQDoFF.glsl:65` (同等式の BD 由来証明)、`cofF.glsl:65-78` |
| **non-rigged BLEND** | `write_depth=false` (`lldrawpoolalpha.cpp:270`)、depth-gate pass (`:231-258`) の cutoff `1.0` で全 alpha pixel discard → `mRT->deferredScreen` の **alpha mesh 位置の depth は背景 depth のまま**。CoF は背景 depth → `postDeferredF.glsl` の中心 pixel `depth` も背景 depth。neighbor も背景 pixel なら `s.a ≈ depth*0.50` 近辺で guard 通過 / 排除はケース依存 (中心 == neighbor depth なら `s.a = (sc/max_cof)*0.5+0.5`、両者背景同 depth で `sc` 同値なら同値、`s.a == 0.5` 付近で `depth*0.50 ≈ depth_encoded*0.50`)。**alpha mesh 自身は depth が背景に「成り代わって」いるので、guard は alpha mesh 内部 vs 背景の区別ができない (= 構造上 non-rigged BLEND の混入には効ききらない)**。**ただし** rigged 部 (= 同 attachment の rigged link) の取り込みが抑えられるので、attachment 全体としては rigged + non-rigged 混在で **混在のうち少なくとも rigged 側の境界が改善** する (§4.y より両者は link されて常に同時存在)。 | 部分改善。non-rigged BLEND の **単独構成 attachment** には効ききらない (§6.5 R1)。 rigged link 部があれば改善。 | §3.1 BLEND 行 `write_depth=false`、`lldrawpoolalpha.cpp:240-246` cutoff 1.0、§4.y SL alpha attachment の rigged + non-rigged link 常態、本 §6.5 R1 |
| **MASK (gbuffer pass)** | gbuffer pass で blend off + depth write on + discard (§3.1 MASK 行)。`PASS_MATERIAL_ALPHA_MASK` / `PASS_ALPHA_MASK` / `PASS_GLTF_PBR_ALPHA_MASK` で **常に正しい mesh depth** が `mRT->deferredScreen` に commit。 | **不変** (本変更は dofSample 内の AND 1 条件追加のみ、MASK 経路の depth 書き込みには影響しない)。MASK は HEAD で既に色シフトせず動いている → 退行なし。 | §3.1 MASK 行、`pipeline.cpp:5008-5022` renderGeomDeferred pool loop、`cofF.glsl:65-78` |

→ **3 経路すべて変更後に「BD 挙動を変えず」「rigged BLEND + MASK は改善、non-rigged BLEND 単独構成は中立 (悪化なし)」を達成**。non-rigged BLEND 単独構成は構造上の限界 (depth 不在のため guard が成立しない) → §6.5 R1 で risk として明示。

#### §6.2.D mesh 種別非依存性の証明 (C2)

変更 1 (shader) は per-pixel の `s.a` (= CoF encode) と `depth` (= centerpixel depth) 比較のみ。`LLDrawInfo::mTexture` / `mAvatar` / 名前文字列等の参照は皆無。hair / clothing / shoes / accessory の区別は **構造上不可能** (per-pixel 処理で mesh ID を持たない) → C2 satisfy。

#### §6.2.E C3 / C6 (副作用ゼロ) の証明

- `mRT->screen.a` は本変更で **書かない** (`postDeferredF.glsl:149` の `frag_color = diff;` は HEAD と同じ、alpha は CoF encode が入っているが書き込み先 RT は src bind の `mRT->screen` ではなく **DoF 用 src のままで** 後段 dofCombine が `mix(diff, dof, a)` で結合してから次の RT に書く → tonemap 入力前提は変えない)。
- `cofF.glsl` / `dofCombineF.glsl` / `postDeferredHQDoFF.glsl` / `softenLightF.glsl` / `tonemap` / `glow` の shader source / uniform / 入力 RT 構成は **一切触らない**。変更は `postDeferredF.glsl` 内の `dofSample` 内 if 条件への AND 1 個追加 + 同 file の depth uniform / center depth 引数追加のみ → 後段 post-process は HEAD と同じ前提で動く → C3 / C6 satisfy。

#### §6.2.F C4 / C5 / C7 satisfy

- **C4**: MRT 追加なし → satisfy。
- **C5**: 新規 cvar 追加なし → satisfy。
- **C7**: BD 由来コードのうち触るのは「permutation 1 個を `addPermutation` で `gDeferredPostProgram` に追加する」のみ。`postDeferredHQDoFF.glsl` / `cofF.glsl` / `dofCombineF.glsl` / `lldrawpoolalpha.cpp:240-246` / `settings.xml RenderDepthOfFieldAlphas` のいずれも変更しない → satisfy。

#### §6.2.G BD parity OFF / vanilla Firestorm でも有害でない (no-op) の証明

- mode 0 (Firestorm View) / mode 1 (AYAstorm View): `llviewershadermgr.cpp:3022` で `aya_view_mode_post != 2` → `else` ブランチで `HAS_ALPHA_EDGE_GUARD` permutation を **立てない**。shader 内の `#if HAS_ALPHA_EDGE_GUARD` ブロックが全部 GLSL preprocessor で消える → `depthMap` uniform 宣言 / center depth 引数 / AND 条件いずれもコンパイル時に存在しない → 完全 no-op。
- mode 2 + `RenderDepthOfFieldHighQuality=true`: HQ shader (`postDeferredHQDoFF.glsl`) が使われ、本 plan で permutation を立てない (変更 2 の `if (!hq_dof)` ガード) → HQ shader 側は HEAD と完全同一動作 (BD parity 維持) → no-op。
- mode 2 + `RenderDepthOfFieldHighQuality=false` (= AYAstorm Cinematic default): 標準 `postDeferredF.glsl` + 本 plan の guard が有効化。これが期待する改善動作。

---

### §6.3 代替プラン (2 つ)

#### §6.3.A 2nd choice: guard 式を BD HQ と同じ `s.a <= depth*0.50` ではなく `s.a <= depth*0.66` 等の緩めた閾値に独自パラメタライズ

**内容**: 変更 1 の guard 式を `s.a <= depth * EDGE_GUARD_RATIO` に置き、`EDGE_GUARD_RATIO` を新規 cvar (例: `RenderDoFEdgeGuardRatio`、F32、default 0.5) で外出し。

**Trade-off**:
- ○ 「画面によって guard が強すぎる/弱すぎる」chemistry を AYA さんが後日 tune できる
- × C5 (新規 cvar 最小限) に違反、本 plan のスコープ外 (BD HQ と同一値で十分という根拠が §6.2.C で確定済)
- × cvar default で BD HQ 同値 (0.5) を選ぶことになり、現状 (= 本 plan) と挙動同一 → cvar の存在意義が薄い

→ **非採用** (cvar tuning は Cinematic Control phase = `project_r30_cinematic_control_tuning_deferred.md` でまとめてやる)。

#### §6.3.B 3rd choice: depth-guard 式を `s.a <= depth*0.50` ではなく `abs(s.a - center_a) < THRESHOLD` (center pixel の `s.a` 自身との連続性比較) に変える

**内容**: 中心 pixel の `diff.a` (= 中心 pixel の CoF encode、`postDeferredF.glsl:95` `vec4 diff = texture(diffuseRect, vary_fragcoord.xy);` の `diff.a`) と neighbor の `s.a` の差で連続性判定。depth uniform を読まなくて済む。

**Trade-off**:
- ○ depth uniform 追加不要 → bind / shader uniform 数を抑えられる
- × **BD HQ path と式が異なる**。本 plan の目的は「BD HQ 由来 guard を標準 path にも添加して BD コードに歩み寄る」ことなので、独自式は C7 の精神 (BD と同型処理を AYAstorm にも揃える) に反する
- × `THRESHOLD` 定数を新規導入する必要、cvar 化すれば C5 違反、ハードコードすれば BD HQ との 1:1 対応が消える

→ **非採用** (BD HQ guard 式そのものを使う方が、C7 の「BD 由来コードを上回るのではなく BD 由来コードに歩み寄って欠落を埋める」精神に整合)。

#### §6.3.C 不採用 approach (記述禁止、復活防止のため明示)

- alpha pool に MRT attachment 追加 (= 旧 L1/L2)
- composite pass を post-tonemap に追加
- `mix(scene, tonemap(near_premul), coverage)` 系の色合成
- **§5 の BD divergent 系プラン (cutoff 値変更 / cvar default 変更)**

これらは過去 3 回外して revert (HEAD=`e676c52b87`) + §5 reject。本書 §6 では一切提案しない。

---

### §6.4 Step 3 (implement agent) 引き継ぎ仕様

#### §6.4.A 触る file 一覧 (推奨プラン §6.2 の場合)

| # | file | 行 | 変更種別 |
|---|---|---|---|
| 1 | `indra/newview/app_settings/shaders/class1/deferred/postDeferredF.glsl` | 28-72, 91-149 | GLSL 編集 (`#if HAS_ALPHA_EDGE_GUARD` でガードした depth uniform / center depth 引数 / AND 条件 1 個追加) |
| 2 | `indra/newview/llviewershadermgr.cpp` | 3010-3050 (= `gDeferredPostProgram` setup の `aya_view_mode_post == 2` ブランチ内) | C++ 編集 (`if (!hq_dof) gDeferredPostProgram.addPermutation("HAS_ALPHA_EDGE_GUARD", "1");` 追加) |

#### §6.4.B CMakeLists 変更要否: **不要**

新規 file 追加なし → CMakeLists.txt 変更不要。

#### §6.4.C shader recompile / cache clear 要否: **要 (shader 編集あり)**

`postDeferredF.glsl` を編集するため、shader cache (`~/.ayastorm_x64/cache/shader_cache/`) を clear して再起動。
ただし C++ (`llviewershadermgr.cpp`) も編集するため shader-only fast-iterate (memory `feedback_shader_only_fast_iterate.md`) は適用不可。**full autobuild → install → cache clear → 起動の通常フロー** (memory `project_build_procedure.md`)。

#### §6.4.D build 種別: full autobuild

C++ (`llviewershadermgr.cpp`) を編集するため full autobuild 必須。

#### §6.4.E 検証手順 (AYA さん依頼前に Claude 側で self-verify、`feedback_self_verify_before_handoff.md`)

##### Claude 側 self-verify (build 前、必須)

1. `postDeferredF.glsl` の `#if HAS_ALPHA_EDGE_GUARD` ブロックすべてに対応する `#endif` があること (open-close 対称、`grep -c "HAS_ALPHA_EDGE_GUARD" postDeferredF.glsl` で偶数件)
2. `dofSample` の関数シグネチャ変更 (引数追加) で **呼び出し側** (`main()` 内 `dofSample(...)` line 139) も同じ permutation でガードして引数を渡していること
3. `llviewershadermgr.cpp` の `if (!hq_dof)` ガードが「HQ shader を選んでいない side」にのみ permutation を立てていること (HQ side に立てると BD HQ shader 内の `#if HAS_ALPHA_EDGE_GUARD` が undefined のまま guard 式が走らないので不変、ただし noise になるので避ける)
4. `pipeline.cpp:9804` の `gDeferredPostProgram.bindTexture(LLShaderMgr::DEFERRED_DEPTH, ...)` が既に存在することを Read で再確認 (HQ path 用に r30 P4 step 4 で追加済、削除されていないこと)
5. `LLShaderMgr::DEFERRED_DEPTH` enum 値の reserved unit が他 sampler と衝突しないこと (HQ shader が既に同じ binding で動いている事実から保証されるが、念のため `grep -n "DEFERRED_DEPTH" llshadermgr.cpp` で確認)
6. mode 0/1 で `else` ブランチが `postDeferredF.glsl` を使い permutation を立てないこと、mode 2 + HQ ON で HQ shader が使われ permutation を立てないこと、mode 2 + HQ OFF (default) で標準 + permutation ON のときのみ guard が有効化されることを **頭の中の cvar matrix で書き出して** 検証

##### build 後 AYA さん依頼用検証手順

1. **Cinematic mode ON (`AYAVisualRealismEnabled=2`)、`RenderDepthOfFieldHighQuality` default (= 0、HQ OFF)** の状態で AYAstorm 起動
2. DoF が効く視点 (focus point を手前 / 奥の対比のある場所に置く) で alpha mesh attachment (hair が最も観察しやすい、可能なら hair + clothing 両方) を視野に入れる
3. focus を **alpha mesh 自身** に合わせて DoF blur が背景に強く効く状態にする → alpha mesh の **edge が背景色寄りに引っ張られにくくなっていること** を観察 (= 期待結果)
4. focus を **背景** に合わせて DoF blur が alpha mesh に強く効く状態にする → alpha mesh の interior が均一に blur されて edge artifact が無いこと
5. `Debug Settings` で `RenderDepthOfFieldHighQuality` を **true に切替**、shader rebuild → HQ DoF path に切替わり、edge 色シフトが HEAD と同じ動作 (BD HQ 由来 guard が走る、本 plan の追加 guard は permutation OFF) であることを確認 (= BD parity が壊れていない証明)
6. `AYAVisualRealismEnabled` を **0 (Firestorm View) に切替**、再起動 → DoF が vanilla 動作 (color shift が依然出る、本 plan の guard は permutation OFF) であることを確認 (= mode 0/1 vanilla 動作維持の証明)
7. `RenderDepthOfField` を **false に切替** → DoF 自体が無効化されて全画面 sharp、guard も dead code (走らない) → 退行なし確認

#### §6.4.F 検証 log hook 要否

推奨プランは **挙動が外部 (画面) で目視確認可能** なため、`LL_INFOS` hook の追加は不要。

#### §6.4.G commit 前 cleanup チェックリスト

- [ ] §6.4.A の 2 file のみ変更されていること (`git diff --name-only`)
- [ ] `postDeferredHQDoFF.glsl` / `cofF.glsl` / `dofCombineF.glsl` / `lldrawpoolalpha.cpp` / `settings.xml` が変更されていないこと (C7 satisfy 確認)
- [ ] CMakeLists.txt が変更されていないこと
- [ ] 追加 `LL_INFOS` / debug print が残っていないこと
- [ ] 本書 §6 / `docs/release/` の文章 update が同 commit に含まれるか別 commit かを AYA さんと事前合意

---

### §6.5 想定 risk (事実起点、§1-§3 引用)

#### §6.5.A R1: non-rigged BLEND 単独構成 attachment では効ききらない

- **内容**: §6.2.C の表「non-rigged BLEND」行で確定した通り、`write_depth=false` (`lldrawpoolalpha.cpp:270`) + Cinematic cutoff 1.0 (`lldrawpoolalpha.cpp:240-246`、C7 保持) のため **alpha mesh 位置の depth は背景 depth のまま**。本 plan の guard `s.a <= depth*0.50` は「中心 pixel depth = 背景」「neighbor pixel depth = 背景」の状況で正しく区別できない → non-rigged BLEND 単独で構成される attachment (= rigged 部を含まない、装飾だけのもの) では色シフトが残る。
- **評価**: **中**。§4.y / `project_sl_alpha_mesh_rigged_nonrigged_link.md` より、SL の alpha mesh attachment は **rigged + non-rigged link が常態**。完全に non-rigged 単独構成は少数派。それでも 0 ではないので残課題として記録、別 phase で「non-rigged BLEND にも depth を書く別の追加処理 (BD 由来でない AYAstorm 独自処理) を上流に挟む」改善案 (例: alpha pool 内に non-rigged BLEND 用 depth gate pass を別途追加) を将来 Plan で立てる必要あり。本 Plan のスコープ外。
- **緩和**: 本 plan で rigged 部の改善は確実に入るため、AYA さん観測の主要シーン (髪は基本 rigged) では改善が出る。AYA さんに「non-rigged 単独 attachment では color shift が残る場合があります」を release note で明示。

#### §6.5.B R2: front-focus (CoF 負側) では guard が走らない

- **内容**: 本 plan は `dofSample` (= `sc < -0.5` の後景 blur 経路) にのみ guard を入れる。`dofSampleNear` (= `FRONT_BLUR` permutation かつ `sc > 0.5` の前景 blur 経路) は触らない。前景 alpha mesh edge で同じ色シフトが起き得る。
- **評価**: **小**。BD HQ path も `dofSampleNear` には depth guard を入れていない (`postDeferredHQDoFF.glsl:80-92`)。BD 由来 path と歩調を揃える本 plan の方針に整合。`FRONT_BLUR` 自体が `RenderDepthOfFieldFront` 既定 ON で常時走るが、前景 alpha mesh の edge 色シフトが AYA さんの観測対象に含まれるなら別 Plan で対称な guard を追加。
- **緩和**: 本 Plan のスコープを「後景 blur 時の alpha mesh edge 色シフト」に明示限定 (§6.0.A 症状定義はこれを含む)。

#### §6.5.C R3: depth re-projection / TAA 系との相互作用

- **内容**: `RenderSMAAT2x` (TAA) が `RenderFSAAType=2` + Cinematic で有効化される (`settings.xml:12158-12168`)。TAA は前フレーム frame の projection で reproject するため、本 plan の guard 式 `s.a <= depth*0.50` で抑えられた neighbor が次フレームで取り込まれて時間方向の flicker を生む可能性。
- **評価**: **小**。BD HQ path が同じ guard 式 + 同じ TAA 構成で運用されてきており、TAA との致命的不整合があれば BD 側で既に修正されているはず。本 plan は BD HQ と **同一式**を標準 path に適用するだけなので、TAA との相互作用は HQ path と同等以下。検証は AYA さんが SMAA T2x ON で観察可能。
- **緩和**: 検証手順 §6.4.E に「`RenderSMAAT2x` 切替で flicker が出ないことを確認」を追加検討 (現状の検証手順 step 7 のあとに追記する想定、Step 3 で AYA さんと相談)。

#### §6.5.D R4: `cofF.glsl` の `frag_color.a` encode と本 guard 式の整合性

- **内容**: `cofF.glsl:78` で `frag_color.a = sc/max_cof*0.5+0.5` (signed CoF を [0, 1] に encode)。`postDeferredF.glsl:47` の `sc = abs(s.a*2.0-1.0)*max_cof` で復元。本 guard 式 `s.a <= depth*0.50` の `s.a` は復元前の encode 値 (= [0, 1] 範囲)。一方 `depth` は `texture(depthMap, tc).r` (= depth buffer 0-1 normalized device coord)。**両者は単位が違う** (`s.a` は CoF encode、`depth` は z buffer 値)。BD HQ path で同式 (`postDeferredHQDoFF.glsl:65`) がこのまま使われている事実から「単位混在でも heuristic として動いている」事は確定だが、**物理的に厳密に「焦点面より手前/奥」を判定しているわけではない**。
- **評価**: **要観察**。BD HQ path がこの heuristic で良好な結果を出している (= AYA さんの r30 P4 移植時に AYA さんが accept した) 事実から、本 plan も同じ heuristic で十分。**ただし「数学的に妥当な guard 式」に直す改善案 (例: 中心 pixel と neighbor pixel の depth 差をリニア空間で比較) は AYAstorm が BD を上回るための別 Plan で着手する余地あり** (memory `project_ayastorm_r30_bd_improvement_phase.md` 精神そのもの)。本 Plan のスコープは BD HQ guard と同一式を標準 path にも揃えるところまで。
- **緩和**: 本 Plan §6.0.C で「BD HQ 由来式と同一」を明示、release note にも「BD HQ path の guard 式と同型」と明記する想定。

---

## §7. 主要 file:line 一括 (本書内で引用したもの)

| ファイル | line | 内容 |
|---|---|---|
| `indra/newview/pipeline.cpp` | 428-466 | `addDeferredAttachments` |
| `indra/newview/pipeline.cpp` | 1053-1060 | `mRT->deferredScreen` / `mRT->screen` allocate + depth share |
| `indra/newview/pipeline.cpp` | 4921-5046 | `LLPipeline::renderGeomDeferred` (gbuffer fill loop) |
| `indra/newview/pipeline.cpp` | 5060-11164 | `LLPipeline::renderGeomPostDeferred` (alpha pool 含む post-deferred loop)、`renderDeferredLighting` 内 11106-11142 で呼び出し |
| `indra/newview/pipeline.cpp` | 9630-9861 | `LLPipeline::renderDoF` |
| `indra/newview/pipeline.cpp` | 10602-11164 | `LLPipeline::renderDeferredLighting` (softenLightF blit @ 10764-10822、screen_target bind @ 10759、flush @ 11150) |
| `indra/newview/llviewerdisplay.cpp` | 1085-1148 | `mRT->deferredScreen.bindTarget()` → `renderGeomDeferred()` → flush |
| `indra/newview/lldrawpoolalpha.cpp` | 64-67 | `MINIMUM_ALPHA` / `MINIMUM_IMPOSTOR_ALPHA` 定数 |
| `indra/newview/lldrawpoolalpha.cpp` | 93-138 | `prepare_alpha_shader` (`minimum_alpha` 設定) |
| `indra/newview/lldrawpoolalpha.cpp` | 142-258 | `renderPostDeferred` (`forwardRender` + DoF depth gate pass) |
| `indra/newview/lldrawpoolalpha.cpp` | 240-246 | Cinematic + `RenderDepthOfFieldAlphas` による DoF cutoff 切替 |
| `indra/newview/lldrawpoolalpha.cpp` | 261-307 | `forwardRender` (`LLGLSPipelineAlpha` + blendFunc setup + `renderAlpha` 呼び出し) |
| `indra/newview/lldrawpoolalpha.cpp` | 278 | `LLGLDepthTest depth(GL_TRUE, write_depth ? GL_TRUE : GL_FALSE)` |
| `indra/newview/lldrawpoolalpha.cpp` | 280-284 | base blend factor: `SRC_ALPHA / ONE_MINUS_SRC_ALPHA / ZERO / ONE_MINUS_SRC_ALPHA` |
| `indra/newview/lldrawpoolalpha.cpp` | 612-947 | `renderAlpha` (batch loop)、blendFunc 上書き @ 831 |
| `indra/newview/lldrawpoolalpha.cpp` | 831 | `gGL.blendFunc(params.mBlendFuncSrc, params.mBlendFuncDst, ...)` per-batch override |
| `indra/newview/lldrawpoolalpha.cpp` | 896 | emissive 2nd pass: `BF_ZERO / BF_ONE / BF_ONE / BF_ONE` |
| `indra/newview/lldrawpoolmaterials.cpp` | 53-94 | `beginDeferredPass` (`bindDeferredShader(gDeferredMaterialProgram[idx])`) |
| `indra/newview/lldrawpoolmaterials.cpp` | 105-305 | `renderDeferred` (drawRange @ 296) |
| `indra/newview/lldrawpoolsimple.cpp` | 99-128 | `LLDrawPoolSimple::renderDeferred` / `LLDrawPoolAlphaMask::renderDeferred` |
| `indra/newview/lldrawpoolwlsky.cpp` | 471-500 | `LLDrawPoolWLSky::renderDeferred` |
| `indra/newview/llvovolume.cpp` | 6960-7150 | LLVOVolume::rebuildFace ルーティング (GLTF / Blinn-Phong / material_pass / alpha mask 分岐) |
| `indra/newview/llviewershadermgr.cpp` | 1327-1335 | `gDeferredDiffuseAlphaMaskProgram` (= `diffuseAlphaMaskIndexedF.glsl`) |
| `indra/newview/llviewershadermgr.cpp` | 1384-1457 | `gDeferredMaterialProgram[i]` setup (alpha_mode permutation @ 1422、HAS_ALPHA_MASK @ 1426-1427) |
| `indra/newview/llviewershadermgr.cpp` | 1556-1605 | `gDeferredPBRAlphaProgram` (= `pbralphaV.glsl` + `pbralphaF.glsl`、permutation `DIFFUSE_ALPHA_MODE=1` のみ、**`HAS_ALPHA_MASK` 無し**) |
| `indra/newview/llviewershadermgr.cpp` | 1864-1934 | `gDeferredAlphaProgram` (= `alphaV.glsl` + `alphaF.glsl`、permutation `USE_VERTEX_COLOR / HAS_ALPHA_MASK / USE_INDEXED_TEX`) |
| `indra/llprimitive/llmaterial.h` | 42-46 | `DIFFUSE_ALPHA_MODE_*` 定数 |
| `indra/llrender/llglstates.h` | 105-119 | `LLGLSPipeline` (depth on/on/LEQUAL) / `LLGLSPipelineAlpha` (blend on) |
| `indra/newview/app_settings/shaders/class1/deferred/alphaV.glsl` | 1-138 | alpha vert shader |
| `indra/newview/app_settings/shaders/class2/deferred/alphaF.glsl` | 171-318 | alpha frag shader (主 main、discard @ 211/228/239、fog @ 305、`frag_color = max(color, vec4(0))` @ 317) |
| `indra/newview/app_settings/shaders/class2/deferred/pbralphaF.glsl` | 126-221 | PBR alpha frag (`HAS_ALPHA_MASK` 無しのため discard 走らない) |
| `indra/newview/app_settings/shaders/class3/deferred/materialF.glsl` | 188 | `out vec4 frag_data[4]` (deferred 経路) |
| `indra/newview/app_settings/shaders/class3/deferred/materialF.glsl` | 259-269 | `alphaMask()` (MASK 経路 discard) |
| `indra/newview/app_settings/shaders/class3/deferred/materialF.glsl` | 300-457 | main (BLEND 経路 forward @ 318-433、deferred MRT 書き @ 435-456) |
| `indra/newview/app_settings/shaders/class1/environment/waterFogF.glsl` | 115-144 | `applySkyAndWaterFog` (背景 sample 無し、fragment 自身に fog 適用のみ) |
| `indra/newview/app_settings/shaders/class1/deferred/cofF.glsl` | 46-78 | CoF 計算 (alpha 非依存、depth から CoF を計算して `.a` 出力) |
| `indra/newview/app_settings/shaders/class1/deferred/postDeferredF.glsl` | 43-149 | DoF blur (`s.a*2.0-1.0` で signed CoF 取得、`texture(diffuseRect, tc+offset)` で周辺 sample average) |
| `indra/newview/app_settings/shaders/class1/deferred/dofCombineF.glsl` | 51-75 | DoF 結果と元 lit を `mix(diff, dof, a)` で合成 |

---

## 更新履歴

- 2026-05-21 初版: AYA さん指示で「髪 edge DoF 色シフト」問題の事実トレース。コード変更ゼロ、推論ゼロ、全主張 file:line 引用。§4 に追加調査 6 件 (うち §4.5 は本書時点で確定済) を列挙。
- 2026-05-21 追記 (Step 2): §2.6「髪 mesh 経路判別 matrix」(全 14 step + canRenderAsMask 詳細 §2.6.1 + 実用 6 経路まとめ §2.6.2)、§2.7「実機検証手順」(A: UI 絞り込み / B: Build 画面 / C: LL_INFOS hook diff / D: 色 canary 非推奨) を追加。§4.2 と §4.5 をコード読みで解決済マーク、§4.6 を §4.3 に統合 (リラベル)、§4.4 は postDeferredF 再 trace でロジック確定、§4.1 と §4.3 (実機未検証部) は §2.7.A / §2.7.C 経由の手順を併記。コード変更ゼロ。
- 2026-05-21 追記 (Step 1.5 close-out): AYA さん発言「SL の髪 attachment は rigged + non-rigged が Link されているのが常態、両経路対応必須」を §4.y として記録。これに伴い §4.1 / §4.3 の「実機で 1 経路に絞る」アプローチは格下げ、Step 2 Plan で **rigged BLEND (depth write) + non-rigged BLEND (no depth) + MASK の 3 経路同時 satisfy** を設計制約として渡す方針に変更。コード変更ゼロ。
- 2026-05-21 追記 (Step 1.5 scope expansion): AYA さん追加発言「髪は分かりやすい例で、服・靴も同じ」を受け、本書スコープを **全 alpha mesh attachment (hair / clothing / shoes / accessory)** に拡大。冒頭スコープ宣言を追加、§4.y を generic 化、§1-§3 の事実は alpha pool / material pool の動作 trace で mesh 種別に依存しないため変更なし。Step 2 Plan agent への制約に「mesh 種別を問わず効くこと (hair 識別ロジック禁止)」追加。コード変更ゼロ。
- 2026-05-21 追記 (Step 2 Plan): §5「Mask 精度向上プラン」(§5.0 症状定義+原因確定 / §5.1 設計制約 6 件 / §5.2 推奨プラン: Cinematic DoF alpha cutoff vanilla parity 化 / §5.3 代替プラン 2 件 / §5.4 Step 3 引き継ぎ仕様 / §5.5 risk 5 件) を §6 (旧 §5) の直前に新規追加。旧 §5「主要 file:line 一括」は §6 に rename。推奨プランは 3 経路 (rigged BLEND / non-rigged BLEND / MASK) すべてを §1-§3 引用で satisfy 証明、mesh 種別非依存 (alpha pool 全体に uniform cutoff) で C2 satisfy、shader / MRT / `mRT->screen.a` 変更なし。R5 (BD parity 退行) のみ Step 3 着手前 AYA さん判断要。コード変更ゼロ。
- 2026-05-21 追記 (Step 2.1 BD-preserving Plan): §5 を AYA さん判断で **不採用マーク** (BD 由来挙動の cutoff/cvar default 変更を伴う点が新方針 `project_ayastorm_r30_bd_improvement_phase.md` 「BD 完全移植は達成済、ここから先は BD 由来コードを変えずに追加処理で改善して AYAstorm が BD を上回る描画エンジンに育てる phase」と矛盾)。§5 は検討プロセスの記録として削除せず温存。新規 **§6「Mask 精度向上プラン — BD-preserving 版 (採用)」** を §5 と旧 §6 の間に追加: 推奨プランは `postDeferredF.glsl::dofSample` の `if (sc > min_sc)` 判定に `&&` で BD HQ 由来 depth-aware guard `s.a <= depth*0.50` を **追加 1 条件**として permutation `HAS_ALPHA_EDGE_GUARD` ガード付きで挟む方式。BD HQ shader (`postDeferredHQDoFF.glsl`) / `cofF.glsl` / `dofCombineF.glsl` / `lldrawpoolalpha.cpp:240-246` / `settings.xml RenderDepthOfFieldAlphas` のいずれも 1 文字も触らない (C7 satisfy)。touch file は `postDeferredF.glsl` + `llviewershadermgr.cpp` の 2 件、新規 cvar 0 件。Cinematic OFF / `RenderDepthOfFieldHighQuality=true` のいずれでも permutation 立たず完全 no-op (BD parity / vanilla 動作完全維持)。3 経路 satisfy 評価: rigged BLEND = 改善、MASK = 不変 (HEAD で既に色シフトなし、退行なし)、non-rigged BLEND 単独構成 = depth が背景に成り代わっているため構造上限界あり (§6.5 R1)、混在 attachment (= SL alpha mesh の常態) の rigged 側は改善。旧 §6 は §7 に rename (内容保持)。コード変更ゼロ。
