# AYAstorm r30 P2 — Velocity Buffer 取り込み 事前依存マップ (BD trace)

**作成日**: 2026-05-17
**最終更新**: 2026-05-17 (Tier-1 + Tier-2 + Tier-3 trace 結論済み、§7-7 案 A (P2 で T2x まで本気実装) 採用確定)
**親 spec**: `ayastorm-r30-cinematic-chapter.md` §3 P2 / §4.2 取り込みリスト
**前段 spec**: `ayastorm-r30-p1-view-mode-restart-switch.md` (Cinematic 枠 UI 先行追加)
**スコープ**: P2 着手前の事前 trace。BD repo 内の velocity buffer 実装 (9 shader + C++ pipeline plumbing) を上から下まで追い、AYAstorm 側に取り込む際の改修ポイントを file:line 単位で確定する。**実装は含まない**。
**BD 参照 commit**: `995a1354d8` (Version to 5.6.2, 2026-04-19)
**Firestorm 参照ブランチ**: `ayastorm-release` HEAD (2026-05-17)

---

## 1. 概要

### 1.1 P2 スコープ再掲

P2 は Cinematic mode の骨格を立てる phase。具体的な取り込み対象は:

- velocity buffer 9 shader (BD borrow): `avatarVelocity{F,V}.glsl`, `skinnedVelocity{V,AlphaV}.glsl`, `velocity{F,V,Alpha{F,V},FuncV}.glsl`
- velocity RT (gbuffer の depth を共有する `mVelocityMap`) の lifecycle
- `LLDrawPool::{getNumMotionBlurPasses, beginMotionBlurPass, renderMotionBlur, endMotionBlurPass}` の virtual API 拡張
- 各 drawpool subclass (Simple / Grass / AlphaMask / Tree / Terrain / Bump / GLTFPBR / Avatar / Alpha) への override 実装
- `LLPipeline::renderGeomMotionBlur()` の新規追加 + display() flow への hook
- 新規 cvar (Cinematic 専用 namespace で `RenderMotionBlur` 相当 + strength)
- Cinematic (`AYAVisualRealismEnabled == 2`) ガード位置

P2 スコープ確定 (§7-7 案 A 採用、2026-05-17): **velocity buffer 生成 path + SMAA T2x 完成** を P2 ship。Motion Blur composite (post-process pass) は **P4** に分離。本 trace は velocity buffer 生成 path を主軸に、SMAA T2x 関連改修は §6.15 に統合。

### 1.2 本 spec の位置づけ

P2 着手時に作業者 (AYA さん / Claude) が「BD のどこを見ればよいか」「Firestorm のどこに何を入れるか」を spec 1 本で把握できる状態にする。trace は概ね完了しているが、§7 未確定事項は P2 着手時に再 fetch / 再確認が必要。

---

## 2. 9 shader ファイルの内容要約

velocity buffer は **NDC delta (現フレーム − 前フレーム)** を per-pixel で 2 成分 (`vec2`) で書き出す方式。fragment shader 側は基本同一 (NDC delta 計算)、vertex shader 側で「static / alpha / static rigged / mesh rigged / avatar rigged」の 5 種類で transform を切り替える。

### 2.1 fragment shader 3 種

| File | 役割 | 共通動作 | 差分 |
|---|---|---|---|
| `velocityF.glsl` | static / mesh rigged 共通 frag | `frag_color = vec4(cur_ndc - last_ndc, 0.0, 1.0)` (RG = velocity, B=0, A=1) | アルファ判定なし |
| `velocityAlphaF.glsl` | alpha mask 系 frag | 同上 | `diffuseLookup(uv).a * vertex_color.a` を `bayerDitherDiscard(alpha, 0.88)` で破棄 |
| `avatarVelocityF.glsl` | avatar rigged 専用 frag | 同上 | `diffuseMap.a < 0.2` で discard (avatar mesh の透過処理) |

すべて `out vec4 frag_color` 1 本のみ。**MRT を使わない** ので、bind 対象の RT は `mVelocityMap` 単体で OK (gbuffer 拡張ではない)。format は `GL_RG16F` (B/A は無視される)。

### 2.2 vertex shader 5 種 + helper 1 種

| File | 対象オブジェクト | uniform 入力 (current) | uniform 入力 (previous frame) | 出力 (current / last clip) |
|---|---|---|---|---|
| `velocityFuncV.glsl` | helper (全 V から呼ばれる) | — | — | `vary_cur_clip = pos; vary_last_clip = last_pos;` の `writeVaryVelocity` を提供。`out vec4 vary_cur_clip / vary_last_clip` を宣言 |
| `velocityV.glsl` | static mesh / mesh rigged (`#ifdef HAS_SKIN` 分岐) | `modelview_projection_matrix`, `modelview_matrix`, `projection_matrix` | `last_modelview_matrix`, `last_object_matrix`, `getLastObjectSkinnedTransform()` (skinned 時) | clip 空間で pos / last_pos を組み立て、`writeVaryVelocity()` |
| `velocityAlphaV.glsl` | alpha mask 系 (static / rigged 両対応) | + `texture_matrix0`, `diffuse_color`, `texcoord0` | 同上 | + `vary_texcoord0`, `vertex_color` を frag に渡す |
| `skinnedVelocityV.glsl` | mesh rigged (`getLastObjectSkinnedTransform()` の実装本体) | `modelview_projection_matrix`, `projection_matrix` | `last_modelview_matrix`, `mat3x4 lastMatrixPalette[MAX_JOINTS_PER_MESH_OBJECT]` | static は `modelview_projection_matrix * pos`、skinned 側は `last_modelview_matrix * last_mat * pos` |
| `skinnedVelocityAlphaV.glsl` | mesh rigged + alpha | 同上 + `texture_matrix0`, `diffuse_color`, `texcoord0` | 同上 | + `vary_texcoord0`, `vertex_color` |
| `avatarVelocityV.glsl` | avatar rigged (LL avatar 専用 skin) | `projection_matrix` | `vec4 lastMatrixPalette[45]` (avatar 固定 45 joint) | `getSkinnedTransform()` (current) と独自実装の `getLastSkinnedTransform()` で前フレーム skin を再計算 |

**重要**:
- `velocityV.glsl` と `skinnedVelocityV.glsl` は **両方とも mesh rigged を扱う** が、`velocityV.glsl` は `#ifdef HAS_SKIN` で 1 ファイル 2 形態 (static / mesh rigged) を兼ねる permutation 流儀、`skinnedVelocityV.glsl` は static は捨てて純 mesh rigged 専用、という二系統が共存する。BD `llviewershadermgr.cpp` の `make_rigged_variant(gVelocityProgram, gVelocitySkinnedProgram)` で前者を「rigged 版にも展開」している (LL upstream の `make_rigged_variant` helper を流用) — つまり `gVelocitySkinnedProgram` は **`velocityV.glsl` の HAS_SKIN マクロ ON 版** であって `skinnedVelocityV.glsl` ではない。`skinnedVelocityV.glsl` がどの program object に link されているか BD 側コードに **明示参照が無い** ので、§7 で再確認対象とする。
- `avatarVelocityV.glsl` の `lastMatrixPalette` は `vec4 [45]` (3x15 packed)、`skinnedVelocityV.glsl` の `lastMatrixPalette` は `mat3x4 [MAX_JOINTS_PER_MESH_OBJECT]` — **layout が違う**。avatar 系と mesh rigged 系で previous-frame skin matrix の uniform 構造が独立。CPU 側で両方を毎フレーム上書きする必要がある。
- LL upstream の現行 skin uniform 名は `matrixPalette` (アバター) / `mat3x4 matrixPalette[]` (mesh rigged)、`lastMatrixPalette` は **BD 独自に追加した uniform**。LL ChannelManager / matrix palette upload helper に対応する `lastMatrixPalette` upload path を C++ 側に新設する必要あり (§7 参照)。

### 2.3 出力 format

全 fragment が `frag_color = vec4(ndc_cur - ndc_last, 0.0, 1.0)`。RT は `mVelocityMap.allocate(resX, resY, GL_RG16F)`。NDC delta なので絶対値は ~[-2, +2] range、`RG16F` で十分。

---

## 3. BD pipeline での呼び出しトレース (C++ 側)

### 3.1 RT 確保 — `BD pipeline.cpp:1043-1056`

```
allocateScreenBufferInternal()
  └─ if (RenderMotionBlur || RenderFSAAType == 3)
       mVelocityMap.allocate(resX, resY, GL_RG16F)
       mRT->deferredScreen.shareDepthBuffer(mVelocityMap)
     else
       mVelocityMap.release()
```

(BD は line 989-995 にも旧 RGB+RECT_TEXTURE 版が `/* ... */` でコメントアウトされている。現行 active path は 1043-1056 のみ。)

`shareDepthBuffer` を呼ぶことで gbuffer (mRT->deferredScreen) と同一 depth attachment を共有 — velocity pass は深度を **read** するだけで write しない (depth mask = false で LEQUAL 比較、§3.3 の `LLGLDepthTest`)。これにより velocity pass がジオメトリの occlusion を gbuffer と完全一致で出せる。

### 3.2 program 登録 — `BD llviewershadermgr.cpp:808, 3187-3220`

| Program object (extern) | shader files | rigged variant | feature flags |
|---|---|---|---|
| `gVelocityProgram` | `deferred/velocityV.glsl` + `deferred/velocityF.glsl` | `make_rigged_variant(gVelocityProgram, gVelocitySkinnedProgram)` | `hasMotionBlur = true` |
| `gVelocityAlphaProgram` | `deferred/velocityAlphaV.glsl` + `deferred/velocityAlphaF.glsl` | `make_rigged_variant(gVelocityAlphaProgram, gVelocityAlphaSkinnedProgram)` | `hasMotionBlur = true`, `mIndexedTextureChannels` |
| `gAvatarVelocityProgram` | `deferred/avatarVelocityV.glsl` + `deferred/avatarVelocityF.glsl` | (avatar は make_rigged_variant 経由ではなく shader 自体が avatar 専用) | `hasMotionBlur = true`, `hasSkinning = true` |

加えて `velocityFuncV.glsl` が **全 vertex shader から link される helper** として `shaders.push_back(make_pair("deferred/velocityFuncV.glsl", 1))` で登録 (line 808)。`writeVaryVelocity()` 関数本体と `vary_cur_clip / vary_last_clip` out 宣言を提供する。

注意:
- `extern LLGLSLShader gVelocityProgram, gVelocitySkinnedProgram, gVelocityAlphaProgram, gVelocityAlphaSkinnedProgram, gAvatarVelocityProgram;` (`llviewershadermgr.h:336-340`)
- BD では `gSkinnedVelocityProgram, gSkinnedVelocityAlphaProgram` も `lldrawpoolavatar.cpp:289, 293` (コメントアウト旧 path) で参照されているが、現行 active path には登場しない (avatar pool は `gAvatarVelocityProgram` 単体)。`make_rigged_variant` の結果が `gVelocitySkinnedProgram` という名前で握られていて、これが mesh rigged 用。

### 3.3 描画 pass — `BD pipeline.cpp:4305-4334`

```cpp
void LLPipeline::renderGeomMotionBlur()
{
    mVelocityMap.bindTarget();
    mVelocityMap.clear(GL_COLOR_BUFFER_BIT);

    gGL.setColorMask(true, true);                  // RG 両方 write
    LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_LEQUAL); // depth read-only, LEQUAL

    sVelocityRender = true;

    for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
    {
        LLDrawPool* poolp = *iter;
        S32 num_passes = poolp->getNumMotionBlurPasses();
        for (S32 i = 0; i < num_passes; ++i)
        {
            poolp->beginMotionBlurPass(i);
            poolp->renderMotionBlur(i);
            poolp->endMotionBlurPass(i);
        }
    }

    sVelocityRender = false;
    mVelocityMap.flush();
}
```

呼び出し元: `BD pipeline.cpp:9515-9518` (display() flow の `renderGeomPostDeferred` の **直後**、`gGLLastModelView` snapshot の **直前**):

```cpp
renderGeomPostDeferred(*LLViewerCamera::getInstance());
popRenderTypeMask();
// ...
if (RenderMotionBlur || RenderFSAAType == 3)
{
    renderGeomMotionBlur();
}
screen_target->flush();

if (!gCubeSnapshot)
{
    // grab gGLLastModelView / gGLLastProjection for next frame
}
```

順序が **崩せない**: `renderGeomMotionBlur()` が `gGLLastModelView` snapshot の前にあるため、velocity pass は「前フレームの modelview」を見られる。snapshot を先に走らせると velocity が 0 になる。

### 3.4 各 DrawPool の override

`BD lldrawpool.h:113-116` で virtual API 4 個を追加:

```cpp
virtual void beginMotionBlurPass(S32 pass);
virtual void endMotionBlurPass(S32 pass);
virtual S32  getNumMotionBlurPasses();
virtual void renderMotionBlur(S32 pass = 0);
```

base impl (`BD lldrawpool.cpp:270-291`) は no-op + `getNumMotionBlurPasses() => 0` (= デフォルトは velocity 書かない)。override しているのは:

| Pool | num passes | velocity 種別 |
|---|---|---|
| `LLDrawPoolSimple` | 1 | `gVelocityProgram` (static + rigged) |
| `LLDrawPoolGrass` | 1 | `gVelocityProgram` (alpha discard なしで OK の前提) |
| `LLDrawPoolAlphaMask` | 1 | `gVelocityAlphaProgram` |
| `LLDrawPoolTree` | 1 | (要再確認、§7) |
| `LLDrawPoolTerrain` | 1 | (要再確認、§7) |
| `LLDrawPoolBump` | 1 | (要再確認、§7) |
| `LLDrawPoolGLTFPBR` | 1 | `mRenderType == RENDER_TYPE_PASS_GLTF_PBR_ALPHA_MASK ? gVelocityAlphaProgram : gVelocityProgram` |
| `LLDrawPoolAlpha` | 1 | (要再確認、§7) |
| `LLDrawPoolAvatar` | 1 | `gAvatarVelocityProgram` (avatar rigged 専用) |

各 pool の `beginMotionBlurPass` は uniform 3 個を upload する流儀で統一:

```cpp
shader.uniformMatrix4fv(LLShaderMgr::LAST_MODELVIEW_MATRIX, 1, GL_FALSE, gGLLastModelView);
shader.uniformMatrix4fv(LLShaderMgr::CURRENT_MODELVIEW_MATRIX, 1, GL_FALSE, gGLModelView);
shader.uniform4f(LLShaderMgr::VIEWPORT, ...);
```

`renderMotionBlur(pass)` は `pushVelocityBatches(PASS_xxx)` + `pushRiggedVelocityBatches(PASS_xxx_RIGGED)` で実 draw。`pushVelocityBatches / pushRiggedVelocityBatches / pushVelocityBatchesTextured` の実装場所は **BD pipeline.cpp に grep で出ない** ため `llrenderpass.cpp` か `lldrawpool.cpp` 配下を要再確認 (§7)。

### 3.5 SMAA T2x reprojection (FSAAType == 3) — `BD pipeline.cpp:8117-8166`

velocity buffer は SMAA T2x (jitter sample blend) の reprojection input としても使われる:

```cpp
// applySMAA path (FSAAType == 3 のみ)
S32 vel_channel = blend_shader.enableTexture(LLShaderMgr::SMAA_VELOCITY_TEX);
mVelocityMap.bindTexture(0, vel_channel, LLTexUnit::TFO_BILINEAR);

// resolveSMAAT2x()
S32 vel_ch = shader.enableTexture(LLShaderMgr::SMAA_VELOCITY_TEX);
mVelocityMap.bindTexture(0, vel_ch, LLTexUnit::TFO_BILINEAR);
```

P2 ship 確定 (§7-7 案 A 採用、2026-05-17): `FSAAType == 3` (= SMAA T2x) を Cinematic で default ON、SMAA shader 改修 (`SMAA_DECODE_VELOCITY` 1 行 + `SMAA_REPROJECTION` macro) + T2x resolve pass (`SMAAResolve{V,F}.glsl` 新規 + `resolveSMAAT2x()` C++) + jitter projection 経路を **全て P2 範囲内**。詳細は §6.15。

### 3.6 Motion Blur composite (post-process) — `BD pipeline.cpp:8229-8249`

```cpp
void LLPipeline::renderMotionBlurComposite(LLRenderTarget* src, LLRenderTarget* dst)
{
    gDeferredMotionBlurProgram.bind();
    gDeferredMotionBlurProgram.bindTexture(DEFERRED_DIFFUSE, src);
    gDeferredMotionBlurProgram.bindTexture(DEFERRED_VELOCITY, &mVelocityMap);
    gDeferredMotionBlurProgram.uniform2f(DEFERRED_SCREEN_RES, ...);
    gDeferredMotionBlurProgram.uniform1i(MOTION_BLUR_STRENGTH, RenderMotionBlurStrength);
    // draw fullscreen triangle
}
```

呼び出し元 (post-deferred flow): `BD pipeline.cpp:8546-8590`、`generateGlow → combineGlow → renderMotionBlurComposite → renderDoF → SMAA/FXAA` の順。

**P2 では取り込まない**: composite shader (`gDeferredMotionBlurProgram`) と `motionBlur{F,V}.glsl` は P4 (Motion Blur + BD DoF chain) の取り込み対象。P2 はあくまで velocity buffer 生成までで、composite shader は次 phase。

---

## 4. gbuffer / RT lifecycle

### 4.1 RT 構造

```
mVelocityMap (新規追加、LLRenderTarget)
  format       : GL_RG16F (RG = NDC delta, BA 未使用)
  depth        : mRT->deferredScreen から share (shareDepthBuffer)
  size         : screen res (resX × resY)
  ownership    : LLPipeline 直下 (cube snapshot 用の sub-RT には作らない、`!gCubeSnapshot` ガード内)
  allocate     : pipeline.cpp:1043-1056 (allocateScreenBufferInternal)
  release      : 同上 if 分岐の else 側
```

**既存 gbuffer (mRT->deferredScreen) は拡張しない**。MRT を増やすのではなく、独立 RT として持つ。これにより既存 gbuffer 3 連 (mRT->deferredScreen.color[0..3]) の format / attachment 数を一切いじらない (regression リスク最小化)。

### 4.2 lifecycle (毎フレーム)

```
1. (前フレーム末) gGLLastModelView = gGLModelView を snapshot
2. (今フレーム頭) gGLModelView 更新 (camera 移動 + アバター移動)
3. gbuffer pass (mRT->deferredScreen に書き込み, depth 確定)
4. lighting / atmospherics / etc (mRT->screen に書き込み)
5. renderGeomPostDeferred (alpha / water / etc, mRT->screen に追加書き込み)
6. ★ renderGeomMotionBlur()
     - mVelocityMap.bindTarget(); clear(COLOR)
     - depth: read-only LEQUAL (gbuffer と同 depth)
     - 各 pool.renderMotionBlur(pass) で NDC delta 描画
     - mVelocityMap.flush()
7. screen_target->flush()
8. (今フレーム末) gGLLastModelView = gGLModelView を再 snapshot (次フレーム用)
9. post-deferred (generateGlow → combineGlow → [P4: renderMotionBlurComposite] → renderDoF → SMAA/FXAA)
```

**clear cost**: 毎フレーム `clear(GL_COLOR_BUFFER_BIT)` を発行 (depth は read-only なので clear 不要)。RG16F の screen 解像度 1 枚分。

### 4.3 read 側 (P2 範囲外、P4 で使う)

- `renderMotionBlurComposite()`: `mVelocityMap` を fullscreen pass で sampling
- `applySMAA() / resolveSMAAT2x()` (FSAAType == 3): SMAA velocity channel として sampling
- `visualizeBuffers(&mVelocityMap, ...)`: `RenderBufferVisualization == 7` で画面に直接表示 (debug)

---

## 5. 関連 cvar (BD 側)

| Key | Type | Default | 用途 | P2 取り込み判断 |
|---|---|---|---|---|
| `RenderMotionBlur` | Boolean | 0 | velocity buffer 生成 gate (= P2 で取り込む `renderGeomMotionBlur()` の呼び出し可否) | 取り込まず、Cinematic ガードに置換 (§6 参照) |
| `RenderMotionBlurStrength` | S32 | 32 | composite shader の最大 blur 長 (pixel) | P4 で取り込み (composite shader と一緒)、P2 では不要 |
| `RenderBufferVisualization` | S32 | -1 | `7 = Velocity` を含む debug 表示 | option として併設可、P2 では debug only |
| `RenderFSAAType` | U32 | 既存 | `3 = SMAA T2x` で velocity 参照 | **P2 で enum 拡張 (`3 = SMAA T2x`)** + Cinematic 起動で auto-set。§7-7 案 A 採用確定 (2026-05-17) で P2 範囲内に確定済。詳細 §6.15 |

**BD では `RenderMotionBlur` が `velocity buffer 生成自体` の gate を兼ねている**。AYAstorm では Cinematic mode (`AYAVisualRealismEnabled == 2`) で常時 ON とし、Cinematic 専用 cvar `AYACinematicMotionBlur` (S32, default = blur strength の値) を P4 で導入する方向 (P2 では `AYACinematicVelocityBuffer = bool` の必要すらない、Cinematic 起動 = velocity ON で固定化)。

cvar 命名規約は親 spec §5.3 に従って `AYACinematic...` prefix で新規 namespace。既存の `AYAR14...` / `AYAR17...` 系 (AYAstorm View 用) とは独立。

---

## 6. AYAstorm 側改修ポイント (file:line)

### 6.1 新規 shader ファイル追加先

すべて `/home/ishikawa/work_firestorm/phoenix-firestorm/indra/newview/app_settings/shaders/class1/deferred/` 配下:

**(a) BD から borrow する 9 ファイル (velocity 系)**:
```
avatarVelocityF.glsl
avatarVelocityV.glsl
skinnedVelocityV.glsl         ← §7-1 結論で AYAstorm 側は active 配線、BD では dead
skinnedVelocityAlphaV.glsl    ← 同上
velocityF.glsl
velocityV.glsl
velocityFuncV.glsl
velocityAlphaF.glsl
velocityAlphaV.glsl
```

**§7-1 結論 (2026-05-17): 全 9 ファイル必須**。BD の `skinnedVelocityV.glsl` / `skinnedVelocityAlphaV.glsl` は active code path から attach されていないが (BD link bug)、velocityV.glsl の HAS_SKIN ブランチが要求する `getLastObjectSkinnedTransform()` を提供する唯一のファイル。AYAstorm 側では §6.6(b)(c) と §6.6(d) (新設) で明示 attach する。

class3/deferred には velocity 系は **追加しない** (BD でも class3 には存在しない、class1 単独でカバー)。各ファイルの先頭にお決まりの provenance comment を追加 (親 spec §4.1 参照):

```glsl
// AYAstorm: imported from BlackDragon Viewer (NiranV Dean), 995a1354d8, 2026-04-19
```

**(b) AYAstorm が自前で新規執筆する 2 ファイル (SMAA T2x resolve)**:
```
SMAAResolveF.glsl     ← ~80 行、BD repo / LL upstream どちらにも無い、SMAA reference impl から起こす
SMAAResolveV.glsl     ← ~30 行、同上
```

**§7-7 案 A 採用確定 (2026-05-17): 新規執筆必須**。Tier-3 trace で判明: BD は `gSMAAResolveProgram` の `mShaderFiles` で `"deferred/SMAAResolveF.glsl"` / `"deferred/SMAAResolveV.glsl"` を参照するが、ファイル自体が BD repo に **存在しない** (= BD 側 T2x は configured but not running 状態)。AYAstorm では SMAA reference impl ([iryoku/smaa](https://github.com/iryoku/smaa) の `SMAA.h` `SMAAResolvePS` macro、Crytek/Jimenez SIGGRAPH 2011 論文の Eq.7 reprojection blend) を読んで GLSL 化する。具体的には `current_color` と `previous_color` を `velocity` で reproject + 2-frame blend する pass。詳細は §6.15。

provenance comment は AYAstorm 自作なので BD 文言と区別:
```glsl
// AYAstorm r30 P2: SMAA T2x resolve pass, derived from SMAA reference impl
// (iryoku/smaa SMAA.h SMAAResolvePS macro, Crytek/Jimenez SIGGRAPH 2011 paper)
```

### 6.2 `indra/llrender/llshadermgr.h` enum 拡張

`eGLSLReservedUniforms` enum (現状 line 220-378) の末尾 (`END_RESERVED_UNIFORMS` の直前) に AYAstorm provenance comment と共に 6 個追加:

```cpp
// <AYAstorm r30 P2> Velocity buffer (imported from BD 995a1354d8)
DEFERRED_VELOCITY,                  //  "velocityMap"
SMAA_VELOCITY_TEX,                  //  "velocityTex"
CURRENT_MODELVIEW_MATRIX,           //  "current_modelview_matrix"
LAST_MODELVIEW_MATRIX,              //  "last_modelview_matrix"
LAST_MODELVIEW_MATRIX_INVERSE,      //  "last_modelview_matrix_inverse"
LAST_OBJECT_MATRIX,                 //  "last_object_matrix"
MOTION_BLUR_STRENGTH,               //  "motion_blur_strength"
// </AYAstorm r30 P2>
```

(P4 で composite shader が `MOTION_BLUR_STRENGTH` を使うが、enum 自体は P2 で先に入れて構わない。`LAST_MODELVIEW_MATRIX_INVERSE` は avatar pool 旧 path で参照されていたもの、現行 BD active path では使われていないが念のため確保。)

対応する文字列登録は `indra/llrender/llshadermgr.cpp` の `initAttribsAndUniforms()` に同順で 7 個 push_back する (BD `llrender/llshadermgr.cpp:1440, 1556-1564` と同位置)。

### 6.3 `indra/newview/pipeline.h`

L825 (`mObjectIDBuffer` 直下) または L830 (`mSceneMap` の手前) に追加:

```cpp
// <AYAstorm r30 P2> Velocity buffer for Cinematic mode (imported from BD 995a1354d8).
// Shares depth with mRT->deferredScreen so velocity pass agrees pixel-for-pixel with gbuffer.
LLRenderTarget          mVelocityMap;
// </AYAstorm r30 P2>
```

加えて L750 付近 (`sReflectionRender` 等の static bool 群) に:

```cpp
// <AYAstorm r30 P2> True during the per-object velocity pass (renderGeomMotionBlur).
// Used by drawpool subclasses to gate velocity-specific batching.
static bool             sVelocityRender;
// </AYAstorm r30 P2>
```

### 6.4 `indra/newview/pipeline.cpp`

**(a) static 定義** — line 365 付近 (sImpostorRender 等の隣):

```cpp
// <AYAstorm r30 P2>
bool LLPipeline::sVelocityRender = false;
// </AYAstorm r30 P2>
```

**(b) allocate** — `allocateScreenBufferInternal()` line 1066 付近 (`mPostPingMap / mPostPongMap` allocate の直後、`mWaterExclusionMask` の手前):

```cpp
// <AYAstorm r30 P2> Cinematic mode velocity buffer.
// AYAVisualRealismEnabled == 2 (Cinematic) でのみ allocate。startup snapshot helper
// (P1 spec §6 で P2 着手時に新設予定の gAYAViewMode) を使う想定。
if (gAYAViewMode == AYA_VIEW_MODE_CINEMATIC)
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("VelocityBuffer");
    if (!mVelocityMap.allocate(resX, resY, GL_RG16F)) return false;
    mRT->deferredScreen.shareDepthBuffer(mVelocityMap);
}
else
{
    mVelocityMap.release();
}
// </AYAstorm r30 P2>
```

**(c) renderGeomMotionBlur 関数定義** — line 4767 (`renderGeomPostDeferred` 関数の手前):

`BD pipeline.cpp:4305-4334` をそのまま AYAstorm provenance comment 付きで移植。

**(d) display() flow への hook** — line 10558-10562 付近 (`renderGeomPostDeferred(...)` の直後、`screen_target->flush()` の手前):

```cpp
renderGeomPostDeferred(*LLViewerCamera::getInstance());
popRenderTypeMask();
}

// <AYAstorm r30 P2> Velocity pass for Cinematic mode.
// Must run before gGLLastModelView snapshot below — otherwise velocity = 0.
if (gAYAViewMode == AYA_VIEW_MODE_CINEMATIC && !gCubeSnapshot)
{
    renderGeomMotionBlur();
}
// </AYAstorm r30 P2>

screen_target->flush();

if (!gCubeSnapshot)
{
    // grab gGLLastModelView / gGLLastProjection for next frame
}
```

注意: AYAstorm 側 display() flow の他箇所 (line 12873, 12880, 12892) でも `renderGeomPostDeferred(camera)` が呼ばれている (cube snapshot / impostor 系)。これらは `gCubeSnapshot` が true なので velocity pass は走らない設計でよいが、impostor 経路で velocity を欲しがる場合は要再検討 (§7)。

### 6.5 `indra/newview/llviewershadermgr.h`

L325 付近 (terrain enum の手前、deferred shader extern 群の最後) に追加:

```cpp
// <AYAstorm r30 P2> Velocity buffer programs (imported from BD 995a1354d8).
extern LLGLSLShader         gVelocityProgram;
extern LLGLSLShader         gVelocitySkinnedProgram;
extern LLGLSLShader         gVelocityAlphaProgram;
extern LLGLSLShader         gVelocityAlphaSkinnedProgram;
extern LLGLSLShader         gAvatarVelocityProgram;
// </AYAstorm r30 P2>
```

### 6.6 `indra/newview/llviewershadermgr.cpp`

**(a) 定義** — line 201 / 221 付近 (gDeferredPostProgram の隣):

```cpp
// <AYAstorm r30 P2>
LLGLSLShader            gVelocityProgram;
LLGLSLShader            gVelocitySkinnedProgram;
LLGLSLShader            gVelocityAlphaProgram;
LLGLSLShader            gVelocityAlphaSkinnedProgram;
LLGLSLShader            gAvatarVelocityProgram;
// </AYAstorm r30 P2>
```

**(b) helper 登録** — `loadShadersDeferred()` 内 (BD `llviewershadermgr.cpp:808` 相当、Firestorm 側でも `shaders.push_back("deferred/textureUtilV.glsl", 1)` の直後を探す):

```cpp
// <AYAstorm r30 P2> Imported from BD 995a1354d8 (llviewershadermgr.cpp:808).
// 追加で §7-1 結論: BD では dead な skinnedVelocity 系も AYAstorm では active 配線するので登録。
shaders.push_back( make_pair( "deferred/velocityFuncV.glsl",         1 ) );
shaders.push_back( make_pair( "deferred/skinnedVelocityV.glsl",      1 ) );
shaders.push_back( make_pair( "deferred/skinnedVelocityAlphaV.glsl", 1 ) );
// </AYAstorm r30 P2>
```

**(c) program createShader 連** — `loadShadersDeferred()` の末尾付近に BD `llviewershadermgr.cpp:3185-3221` をそのまま移植 (provenance comment 付き)、ただし §7-1 結論により rigged variant の link を成立させるため、`make_rigged_variant` 呼び出し前後で `skinnedVelocityV.glsl` / `skinnedVelocityAlphaV.glsl` を rigged 側 `mShaderFiles` に明示追加する:

```cpp
if (success && gAYAViewMode == AYA_VIEW_MODE_CINEMATIC)  // ← Cinematic でのみ compile
{
    gVelocityProgram.mName = "Velocity Shader";
    gVelocityProgram.mFeatures.hasMotionBlur = true;
    gVelocityProgram.mShaderFiles.clear();
    gVelocityProgram.mShaderFiles.push_back(make_pair("deferred/velocityV.glsl", GL_VERTEX_SHADER));
    gVelocityProgram.mShaderFiles.push_back(make_pair("deferred/velocityF.glsl", GL_FRAGMENT_SHADER));
    gVelocityProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
    success = make_rigged_variant(gVelocityProgram, gVelocitySkinnedProgram);
    // <AYAstorm §7-1 結論> rigged 側に skinnedVelocityV.glsl を補完 attach。
    // make_rigged_variant が createShader まで一気に行うため、本来は make_rigged_variant の前に
    // shader.mShaderFiles に追加するか、velocity 系専用に make_rigged_velocity_variant ヘルパを
    // 新設する形がクリーン。実装時 (P2 着手時) に確定。
    success = success && gVelocityProgram.createShader();
    // gVelocityAlphaProgram は skinnedVelocityAlphaV.glsl で同様の処理
    // gAvatarVelocityProgram は hasSkinning = true で avatarSkinV.glsl 経由 (rigged variant 不要)
}
```

`make_rigged_variant` 自体 (Firestorm `llviewershadermgr.cpp:258-272` 既存) は触らず、velocity 系のみ専用ヘルパで `mShaderFiles` 拡張 + `attachVertexObject("deferred/skinnedVelocityV.glsl")` を行う方が安全 (他の rigged variant に副作用を及ぼさない)。

**unload** — `unloadShaders()` line 1167 付近 (`gDeferredPostProgram.unload()` の隣) に 5 program の unload() 呼び出し追加。

`LLGLSLShader::mFeatures.hasMotionBlur` field の追加は §6.13 で別途扱う (§7-4 結論済み)。

### 6.7 `indra/newview/lldrawpool.h`

L94-96 付近 (`beginRenderPass / endRenderPass / getNumPasses` の virtual 宣言群の直後) に追加:

```cpp
// <AYAstorm r30 P2> Motion blur velocity pass API.
virtual void beginMotionBlurPass(S32 pass);
virtual void endMotionBlurPass(S32 pass);
virtual S32  getNumMotionBlurPasses();
virtual void renderMotionBlur(S32 pass = 0);
// </AYAstorm r30 P2>
```

base impl は `lldrawpool.cpp` の対応位置に 4 関数追加 (no-op、`getNumMotionBlurPasses() => 0`)。BD `lldrawpool.cpp:270-291` 相当。

### 6.8 各 DrawPool subclass の override (2026-05-17 trace 結論)

下記 7 ファイルに override を追加 (LLDrawPoolGrass は BD でも override 無し = base impl の `getNumMotionBlurPasses() => 0` で velocity 出さない:草は風アニメで NDC delta が破綻するため意図的にスキップ)。BD 各 cpp の実コードを文字通り移植する形:

| File | BD ref (cpp) | bind program | helper | 注意点 |
|---|---|---|---|---|
| `lldrawpoolsimple.cpp/h` | (Simple pool は LL upstream で `LLDrawPoolSimple` だが BD trace 内では override 行が見つからず — base impl の no-op で代替、または `gVelocityProgram` + `pushVelocityBatches(PASS_SIMPLE)` 自前追加。実装時 BD 内 grep で再確認) | `gVelocityProgram` (static + rigged) | `pushVelocityBatches / pushRiggedVelocityBatches` | static / rigged 両方 1 pass |
| `lldrawpooltree.cpp/h` | `lldrawpooltree.cpp:140-192` | `gVelocityProgram` (static のみ、rigged 経路無し) | helper 不使用、`mDrawFace` を直接 iterate して `face->getVertexBuffer()->drawRange()` を呼ぶ | tree は region matrix を直接 `LAST_OBJECT_MATRIX / CURRENT_OBJECT_MATRIX` に積む (`drawable->getRegion()->mRenderMatrix`)、tree は静的 swing 効果が小さく **前フレーム = 現フレーム** で OK (velocity ≈ 0)。意図的に近似 |
| `lldrawpoolterrain.cpp/h` | `lldrawpoolterrain.cpp:208-256` | `gVelocityProgram` | helper 不使用、tree と同じく `mDrawFace` を直接 iterate して `facep->renderIndexed()` を呼ぶ | region matrix を `LAST_OBJECT_MATRIX = CURRENT_OBJECT_MATRIX` に同値で積む (terrain は完全静的、velocity = 0 になる) |
| `lldrawpoolbump.cpp/h` | `lldrawpoolbump.cpp:594-625` | `gVelocityProgram` (static) + `gVelocityProgram` の rigged variant (`bind(true)`) | `pushVelocityBatches(PASS_BUMP)` + `pushRiggedVelocityBatches(PASS_BUMP_RIGGED)` | normal map は無視 (velocity shader は frag で `diffuseLookup` すら呼ばない、layout 違いは attribute mask が `VertexAttributeMask` レベルで吸収) |
| `lldrawpoolalpha.cpp/h` | `lldrawpoolalpha.cpp:903-937` | `gVelocityAlphaProgram` (static + rigged) | `pushVelocityBatchesTextured(PASS_ALPHA)` + `pushRiggedVelocityBatchesTextured(PASS_ALPHA_RIGGED)` | **`POOL_ALPHA_PRE_WATER` は `getNumMotionBlurPasses() == 0` を返して velocity スキップ** — `mLastModelMatrix` が pre-water / post-water で double-stamp されるのを避ける (BD コメント `// Only render velocity from one instance to avoid double-stamping mLastModelMatrix`)。velocity pass は RT 自体 single-stamp で、blend mode 不使用 (depth は read-only LEQUAL で discard ベース alpha 判定が `velocityAlphaF.glsl` の `bayerDitherDiscard` で実行)、blend RT 破壊問題は起きない設計 |
| `lldrawpoolpbropaque.cpp/h` | `lldrawpoolpbropaque.cpp:97-145` | `mRenderType == RENDER_TYPE_PASS_GLTF_PBR_ALPHA_MASK ? gVelocityAlphaProgram : gVelocityProgram` (static + rigged) | alpha_mask 側は `pushVelocityBatchesTextured(mRenderType)` + `pushRiggedVelocityBatchesTextured(mRenderType + 1)`、opaque 側は `pushVelocityBatches / pushRiggedVelocityBatches` | `mRenderType + 1` は LL の `(PASS_xxx, PASS_xxx_RIGGED)` ペアが連番で並ぶ前提に依存 — Firestorm 側の enum 並びを確認しないと壊れる (P2 実装時に再 grep) |
| `lldrawpoolavatar.cpp/h` | `lldrawpoolavatar.cpp:500-566` (active) — line 276-330 は旧 motion_blur_quality 版でコメントアウト | `gAvatarVelocityProgram` | helper 不使用、`avatarp->renderSkinned()` 単発で全 mesh 描画 | impostor / `isTooSlow` / `AOA_INVISIBLE` / `isDead` / `isUIAvatar` は skip。`gAvatarVelocityProgram` 自身が `hasSkinning = true` で `vec4 lastMatrixPalette[45]` uniform を持つ — upload は `LLVOAvatar` 側の per-frame skin path に追加 (§6.10) |

base impl (`LLDrawPool::getNumMotionBlurPasses() => 0`、他 3 関数は no-op) を `lldrawpool.cpp` に新設しておくと、Grass / Simple (override 不要 / 任意) / Sky / WLSky / Water / Voidwater は何もしなくて済む。

velocity pass の `beginMotionBlurPass` は **bind program + 3 uniform (LAST_MODELVIEW_MATRIX / CURRENT_MODELVIEW_MATRIX / VIEWPORT) upload** で共通、`endMotionBlurPass` は `program.unbind()` のみ。`renderMotionBlur` 本体だけが pool 固有 (上表)。

Terrain / Tree が `gVelocityProgram` の static side だけを使い rigged variant を bind しないので、velocity buffer は terrain / tree 上でも常に `velocity = 0` (= 静的) として書かれる。これは BD 設計通り、AYAstorm でも同じく踏襲する。

### 6.9 helper 関数 `pushVelocityBatches` / `pushRiggedVelocityBatches` / `*Textured` 4 種の追加先 (§7-3 結論)

BD repo に `llrenderpass.cpp/h` は **存在しない** (LL upstream 統合済み)。`LLRenderPass` class は `lldrawpool.{h,cpp}` 内に同居しており、4 関数 + `uploadLastMatrixPalette` ヘルパは:

- BD 宣言: `lldrawpool.h:402-405` (`pushVelocityBatches` / `pushRiggedVelocityBatches` / `pushVelocityBatchesTextured` / `pushRiggedVelocityBatchesTextured`、引数すべて `U32 type` のみ)
- BD 定義: `lldrawpool.cpp:802` / `838` / `871` / `912`、それぞれ独自 body (`pushBatches` の thin-wrap **ではない**)
- 補助: `lldrawpool.cpp:951` の `LLRenderPass::uploadLastMatrixPalette(LLVOAvatar*, LLMeshSkinInfo*)` static、`avatar->updateSkinInfoMatrixPalette(skinInfo)` から前フレーム skin matrix cache を取り出して `LLViewerShaderMgr::AVATAR_LAST_MATRIX` uniform に upload

Firestorm 側追加先:

- `indra/newview/lldrawpool.h:357` (`void pushBatches(U32 type, bool texture = true, bool batch_textures = false);` の直下) に 4 関数 + 1 helper を AYAstorm provenance comment 付きで追加
- `indra/newview/lldrawpool.cpp` の `LLRenderPass::pushBatches` 定義 (要 grep、推定 line 500 前後) の直後に BD `lldrawpool.cpp:802-948` 4 関数 + line 951+ の `uploadLastMatrixPalette` を AYAstorm provenance comment 付きで移植

実コード抜粋 (BD `lldrawpool.cpp:824` — 最重要 line、per-drawable last matrix upload):

```cpp
const LLMatrix4* last_mat = params.mLastModelMatrix ? params.mLastModelMatrix : &identity;
LLGLSLShader::sCurBoundShaderPtr->uniformMatrix4fv(LLShaderMgr::LAST_OBJECT_MATRIX, 1, GL_FALSE, (GLfloat*)last_mat->mMatrix);
```

加えて BD `lldrawpool.cpp:830-834` の **draw 後に `mLastModelMatrix` を `mModelMatrix` で上書き保存** する副作用 path も忘れず移植 (このフレームの matrix が次フレームの "last" になる、velocity pass の core 仕組み):

```cpp
const LLMatrix4* current_mat = params.mModelMatrix ? params.mModelMatrix : &identity;
if (params.mLastModelMatrix)
{
    *params.mLastModelMatrix = *current_mat;
}
```

これに伴い `LLDrawInfo::mLastModelMatrix` (`LLMatrix4*`、lifecycle alloc/free 含む) の追加が必須 — Tier-2 §7-6 で別途確定。

### 6.10 last skin matrix palette upload (C++ 側) (2026-05-17 trace 結論)

mesh rigged 系と avatar 系で uniform 名は **両方とも `lastMatrixPalette`** で共有 (BD `llshadermgr.cpp:1563` で `mReservedUniforms.push_back("lastMatrixPalette");`、enum 名は `AVATAR_LAST_MATRIX`)。layout だけ shader 毎に違う:

- `skinnedVelocityV.glsl` (mesh rigged): `uniform mat3x4 lastMatrixPalette[MAX_JOINTS_PER_MESH_OBJECT];`
- `avatarVelocityV.glsl` (avatar): `uniform vec4 lastMatrixPalette[45];`

GLSL uniform name は同じだが宣言型 (mat3x4 vs vec4 array) は program 毎に独立なので衝突しない。upload 経路は **同じ `LLRenderPass::uploadLastMatrixPalette(LLVOAvatar*, LLMeshSkinInfo*)` static helper を mesh rigged / avatar 両方で兼用** する設計 — どちらも 1 つの `MatrixPaletteCache::mLastGLMp` (前フレームの mat3x4 packed float) を参照する。

**(a) `LLVOAvatar::MatrixPaletteCache` (`llvoavatar.h:859-875`) に前フレーム cache を追加** — BD `llvoavatar.h:872-892` 相当:

```cpp
class alignas(16) MatrixPaletteCache
{
public:
    U32 mFrame;
    LLMeshSkinInfo::matrix_list_t mMatrixPalette;
    std::vector<F32> mGLMp;

    // <AYAstorm r30 P2> Imported from BD 995a1354d8 (llvoavatar.h:884-886).
    // Previous frame's mGLMp snapshot for velocity buffer upload.
    std::vector<F32> mLastGLMp;
    S32 mLastFrame = -1;
    // </AYAstorm r30 P2>

    MatrixPaletteCache() : mFrame(gFrameCount - 1) {}
};
```

**(b) `LLVOAvatar::updateSkinInfoMatrixPalette` (`llvoavatar.cpp:11243-11248`) に「mGLMp 上書き前に mLastGLMp に snapshot」処理を追加** — BD `llvoavatar.cpp:10320-10328` 相当。`entry.mFrame != gFrameCount` ブランチに入った直後 (= 今フレーム初回 access)、`mFrame > 0` (= 初フレームでない) ガード付きで `entry.mLastGLMp = entry.mGLMp;` `entry.mLastFrame = entry.mFrame;`。これにより「前フレームの skin」が次の `pushRiggedVelocityBatches` から取り出せる。Cinematic 以外でも copy が走るが (`std::vector<F32>` の copy assign は最大 768 float = 3KB × 同時 active mesh skin 数)、毎 frame 1 回コピーで実害無し:Cinematic 外では `mLastGLMp` は誰も read しない dead field。

**(c) `LLRenderPass::uploadLastMatrixPalette` helper を `lldrawpool.cpp` に新設** — BD `lldrawpool.cpp:951-970` 相当 (§6.9 に詳細既出):

```cpp
//static
bool LLRenderPass::uploadLastMatrixPalette(LLVOAvatar* avatar, LLMeshSkinInfo* skinInfo)
{
    if (!avatar || !skinInfo) return false;
    const LLVOAvatar::MatrixPaletteCache& mpc = avatar->updateSkinInfoMatrixPalette(skinInfo);
    U32 count = static_cast<U32>(mpc.mMatrixPalette.size());
    if (count == 0 || mpc.mLastGLMp.empty()) return false;
    LLGLSLShader::sCurBoundShaderPtr->uniformMatrix3x4fv(
        LLViewerShaderMgr::AVATAR_LAST_MATRIX, count, false,
        (GLfloat*)&(mpc.mLastGLMp[0]));
    return true;
}
```

`uniformMatrix3x4fv` を呼ぶので shader 側の宣言が `vec4 lastMatrixPalette[45]` (avatar) の場合は **GL layout 不一致** になる。BD は実際これを (1) 同じ float ストレージを (2) shader 毎の宣言で reinterpret して使っている — 45 joint × 3 vec4 = 135 vec4 = 540 float、`MAX_JOINTS_PER_MESH_OBJECT` × mat3x4 = (例えば) 64 × 12 = 768 float、ペイロード長は違うが avatar 側 shader が `lastMatrixPalette[0..44]` までしか index しないので超過 read にはならない。AYAstorm は BD と同じ 1-helper / 1-uniform name 流儀でいく (= avatar shader 内 `vec4[45]` index は 45 jointまで、mesh rigged shader は `mat3x4[N]` index で N 個まで — 物理的に同じ uniform buffer slot)。

**(d) avatar 側 (`LLDrawPoolAvatar::renderMotionBlur`) は `avatar->renderSkinned()` 一発で全 mesh 描画するため uploadLastMatrixPalette は呼ばない** — `gAvatarVelocityProgram` の `lastMatrixPalette` uniform は `avatar->renderSkinned()` 内側で per-mesh upload される (BD 同様の設計、§6.10(b) の cache 更新で前フレーム skin が拾える)。**実装時 BD `LLVOAvatar::renderSkinned` を再 fetch して `LLViewerShaderMgr::AVATAR_LAST_MATRIX` upload 箇所を確認** (BD 側で renderSkinned 内に既に同 helper 呼び出しがあるはず、未確認、P2 着手時に grep 確定)。

Cinematic 以外で per-frame copy コストを切りたい場合は `LLVOAvatar::MatrixPaletteCache` 更新時に `if (gAYAViewMode == AYA_VIEW_MODE_CINEMATIC)` ガードで copy をスキップする実装も可能。P2 default は無条件 copy (regression リスク最小)、P2 ship 後 profile で hot だったら gate 化。

### 6.10b `LLDrawInfo::mLastModelMatrix` + `LLDrawable::mLastVelocityMatrix` 追加 (2026-05-17 trace 結論)

§6.9 の `pushVelocityBatches` 系が `params.mLastModelMatrix` を参照する前提として、`LLDrawInfo` に per-DrawInfo の前フレーム matrix pointer、`LLDrawable` に実 storage を追加する:

**(a) `LLDrawInfo::mLastModelMatrix` 追加** — `indra/newview/llspatialpartition.h:116` (`const LLMatrix4* mModelMatrix = nullptr;` の直下) に AYAstorm provenance comment 付きで:

```cpp
// <AYAstorm r30 P2> Imported from BD 995a1354d8 (llspatialpartition.h:104).
// Pointer into owning LLDrawable::mLastVelocityMatrix. Read by
// pushVelocityBatches as the previous-frame model matrix; updated by the
// same function post-draw (*mLastModelMatrix = *mModelMatrix) so this
// frame's matrix becomes next frame's "last".
LLMatrix4* mLastModelMatrix = nullptr;
// </AYAstorm r30 P2>
```

**(b) `LLDrawable::mLastVelocityMatrix` 追加** — `indra/newview/lldrawable.h` の同 class member 群 (BD `lldrawable.h:299` 相当、Firestorm 側は要 grep)。AYAstorm provenance comment 付きで `LLMatrix4 mLastVelocityMatrix;` (value、コンストラクタで identity 初期化される LLMatrix4 default)。

**(c) DrawInfo 生成箇所で setter 配線** — BD では `llvovolume.cpp:5574` の `LLDrawInfo` factory:

```cpp
draw_info->mLastModelMatrix = &drawable->mLastVelocityMatrix;
```

Firestorm 側 `llvovolume.cpp` の同 factory (= `LLDrawInfo` を `new` している箇所、複数箇所のはず) すべてに同 1 行を追加。P2 着手時に `grep -n "new LLDrawInfo" indra/newview/llvovolume.cpp` で網羅。

**(d) BD 側 known issue — constructor で nullptr 初期化** — BD `llspatialpartition.cpp:3848` で `//mLastModelMatrix(NULL)` がコメントアウトされている。`= nullptr` の default initializer (header 側) で代わっているので NULL safety はあるが、これは BD コードが (e) の setter で `mLastModelMatrix` を埋め忘れた場合に `pushVelocityBatches` で `last_mat = &identity` (velocity = 0) フォールバックに落ちる設計を示す。AYAstorm では (c) の setter を全 factory に確実に配線することで「nullptr → velocity 強制 0」path を作らないようにする (P2 acceptance 確認項目)。

### 6.11 cvar 追加 (`indra/newview/app_settings/settings.xml`)

P2 着手時の cvar 追加は最小限:

```xml
<!-- <FS:AYA r30 P2> Cinematic mode velocity buffer debug visualizer.
     RenderBufferVisualization と既存パイプ流儀に合わせ、velocity buffer 単体は cvar gate を作らず
     Cinematic mode 起動で常時 ON。debug 表示のみ専用 cvar で出す。 -->
<key>AYACinematicVelocityDebug</key>
<map>
  <key>Comment</key>
  <string>r30 P2 debug: Cinematic mode の velocity buffer を画面に直接表示 (RG = NDC delta)。0=off, 1=on。Cinematic mode (AYAVisualRealismEnabled==2) でのみ有効。</string>
  <key>Persist</key>
  <integer>0</integer>
  <key>Type</key>
  <string>Boolean</string>
  <key>Value</key>
  <integer>0</integer>
</map>
<!-- </FS:AYA r30 P2> -->
```

`RenderMotionBlur / RenderMotionBlurStrength` は P2 では取り込まない (P4 で composite shader と一緒に `AYACinematicMotionBlurStrength` を追加する)。

### 6.12 startup snapshot helper (P1 spec §6 で予告された P2 着手必要事項)

`indra/newview/pipeline.cpp` または `llappviewer.cpp` のどこかに:

```cpp
// <AYAstorm r30 P2> Startup snapshot of View Mode (AYAVisualRealismEnabled).
// Per ayastorm-r30-cinematic-chapter.md §1.4, the 3 modes (Firestorm View / AYAstorm View /
// Cinematic) all unify on restart-switch. After P1 the cvar is restart-required at the UI level;
// here we capture the value once at app init and use this snapshot everywhere the pipeline /
// shader manager makes startup decisions, eliminating the need for per-frame gates.
enum EAYAViewMode { AYA_VIEW_MODE_FIRESTORM = 0, AYA_VIEW_MODE_AYASTORM = 1, AYA_VIEW_MODE_CINEMATIC = 2 };
EAYAViewMode gAYAViewMode = AYA_VIEW_MODE_AYASTORM;

// In LLAppViewer::init() (or wherever pipeline allocate is first reached):
gAYAViewMode = static_cast<EAYAViewMode>(std::clamp(gSavedSettings.getU32("AYAVisualRealismEnabled"), 0u, 2u));
// </AYAstorm r30 P2>
```

これにより `allocate` / `loadShaders` / `display` の各所で `if (gAYAViewMode == AYA_VIEW_MODE_CINEMATIC)` で gate 可能になる。

### 6.13 `LLShaderFeatures::hasMotionBlur` 追加 (§7-4 結論) と `attachShaderFeatures()` 拡張 (§7-10 結論)

**(a) field 追加** — `indra/llrender/llglslshader.h:62` の `bool hasTonemap = false;` の直下、line 63 の `};` の直前に追加:

```cpp
// <AYAstorm r30 P2> Imported from BD 995a1354d8 (llglslshader.h:63).
// attachShaderFeatures() で deferred/velocityFuncV.glsl 自動 attach の gate を担う。
bool hasMotionBlur = false;
// </AYAstorm r30 P2>
```

field 命名は BD と一致 (`hasMotionBlur`)、AYAstorm prefix なし。理由: BD diff 最小化 + internal flag bag で UI 露出なし。

**(b) attachShaderFeatures 拡張** — `indra/llrender/llshadermgr.cpp:175` (`hasObjectSkinning` ブロック `}` 直後、line 177 の `attachVertexObject("deferred/textureUtilV.glsl")` の直前) に AYAstorm provenance comment 付きで挿入:

```cpp
// <AYAstorm r30 P2> Imported from BD 995a1354d8 (llshadermgr.cpp:177-183).
// hasMotionBlur 有効な program に velocityFuncV.glsl を自動 attach
// (writeVaryVelocity() 関数本体と vary_cur_clip / vary_last_clip out 宣言を提供)。
if (features->hasMotionBlur)
{
    if (!shader->attachVertexObject("deferred/velocityFuncV.glsl"))
    {
        return false;
    }
    // §7-1 結論: BD では rigged velocity variant の skinnedVelocityV.glsl が
    // 何処からも attach されていないため link bug がある。AYAstorm では明示 attach する。
    if (features->hasObjectSkinning)
    {
        if (!shader->attachVertexObject("deferred/skinnedVelocityV.glsl"))
        {
            return false;
        }
    }
}
// </AYAstorm r30 P2>
```

`skinnedVelocityAlphaV.glsl` 側は `hasAlphaMask` field では判定できないので (= alpha 系判定に使われていない)、velocity alpha 系 program 設定時 (§6.6(c)) に個別 `attachVertexObject` で対応する形を推奨。または `features` に `bool hasMotionBlurAlpha = false;` を追加して同所で分岐する形でも可、これは実装時 (P2 着手時) に確定。

### 6.15 SMAA T2x (FSAAType == 3) — 案 A 採用確定 (2026-05-17 AYA 判断)

§7-7 で詳述。**AYA 判断: 案 A 採用** (P2 で T2x まで本気実装、視覚で確認しながら P3 以降の方針を決める方が後段スムーズという判断、2026-05-17)。

下表は §6.1-§6.13 / §6.10b の velocity buffer 配線に **追加で** 必要な T2x 関連改修。

| 改修箇所 | 改修内容 |
|---|---|
| `class1/deferred/SMAAResolveF.glsl` / `SMAAResolveV.glsl` | **新規執筆**: BD repo にも LL upstream にも存在しない (Tier-3 trace で判明)。SMAA reference impl (Crytek/Jimenez `SMAA.glsl` の `SMAAResolvePS` macro) から起こす。~80 + ~30 lines |
| `class1/deferred/SMAA.glsl` | `SMAA_DECODE_VELOCITY` macro を `sample.rg * 0.5` に変更 (BD 同形)、追加で T2x 用 `#define SMAA_REPROJECTION 1` を Cinematic permutation に追加 |
| `pipeline.h` | `mSMAAHistory` (LLRenderTarget) + `sT2xJitterEnabled` static bool + `gSMAAResolveProgram[4]` / `gSMAANeighborhoodBlendT2xProgram[4]` extern 宣言 |
| `pipeline.cpp:allocateScreenBufferInternal()` | `mSMAAHistory.allocate(resX, resY, GL_RGBA, false)` を Cinematic gate 内で追加 (~5 lines) |
| `pipeline.cpp:applySMAA()` | `RenderFSAAType == 3` 分岐で `gSMAANeighborhoodBlendT2xProgram[fsaa_quality]` を bind、`SMAA_VELOCITY_TEX` で `mVelocityMap` を bind (BD `pipeline.cpp:8092-8122` 移植、~30 lines) |
| `pipeline.cpp:resolveSMAAT2x()` 新規 | BD `pipeline.cpp:8139-8180` を移植 (~50 lines)、毎フレーム末で current / previous / velocity を blend して history に保存 |
| `pipeline.cpp:render()` または `display()` 内 jitter projection 経路 | `gGLProjection` を毎フレーム ±0.5 pixel オフセット (T2x の 2-frame jitter)、`!gCubeSnapshot` 限定 + `sT2xJitterEnabled = true` 代入 path (~50 lines、BD で missing なので Crytek paper / SMAA reference impl から起こす) |
| `llviewershadermgr.cpp:loadShadersDeferred()` | `gSMAANeighborhoodBlendT2xProgram[i]` / `gSMAAResolveProgram[i]` を 4 quality x 2 program = 8 program load (BD `llviewershadermgr.cpp:2800-2835` 移植、~80 lines) |
| `settings.xml` | `RenderFSAAType` enum 拡張 (`3 = SMAA T2x`)、Cinematic mode 起動で auto-set |
| P2 ship 判定 | 「Cinematic ON で camera pan 時に SMAA T2x の時間 reprojection 効果が目視可能」(静的シーン camera pan で edge AA 改善、sub-pixel jitter blend で「ジャギが時間方向に滲んで消える」) |
| P2 追加工数 | velocity 5 日 + T2x 4-5 日 = **9-10 日 (合計)** |

---

## 7. 未確定事項 (P2 着手時に再 trace / 再判断)

1. **(2026-05-17 trace 結論) `skinnedVelocityV.glsl` / `skinnedVelocityAlphaV.glsl` は BD 内で dead file、AYAstorm 側で **明示 attach 経路を新設して活用** する**
   `gh search/code` で BD repo 全体を検索した結果、`skinnedVelocity*` を参照しているのは (a) `skinnedVelocityV.glsl` / `skinnedVelocityAlphaV.glsl` ファイル自身、(b) `lldrawpoolavatar.cpp` の active path 外コメントアウト (`gSkinnedVelocityProgram` 旧 path、spec §3.2 既知)、のみ。**active code path から `attachVertexObject("deferred/skinnedVelocityV.glsl")` を呼んでいる箇所は無い**。`make_rigged_variant` (BD `llviewershadermgr.cpp:260-274`) は `mShaderFiles` を浅 copy し `HAS_SKIN` permutation を加えるだけで、追加の attach は行わない。`shaders.push_back("deferred/velocityFuncV.glsl", 1)` (line 808) のような pre-load 登録にも skinnedVelocity 系は無い。
   一方で `velocityV.glsl` の HAS_SKIN ブランチ (line 43-49) は `getLastObjectSkinnedTransform()` を extern 宣言してから呼んでおり、これを実体定義しているのは `skinnedVelocityV.glsl` だけ。よって BD の現状コードは **rigged velocity variant が GLSL link 時に未定義シンボルでエラーするはず** (= `gVelocitySkinnedProgram` の createShader() が失敗、`success = false` で次以降の `createShader && ...` がスキップされる)。BD 実機では `RenderMotionBlur` を ON にしてもアバター以外の rigged mesh が velocity buffer に出ない、または createShader 失敗ログが出る状態と推測。
   AYAstorm 取り込み方針: §6 改修ポイント表で `skinnedVelocityV.glsl` と `skinnedVelocityAlphaV.glsl` を `shaders.push_back` (line 808 相当) に追加し、加えて `attachShaderFeatures()` (`llshadermgr.cpp:177` 相当) の `hasMotionBlur` ブロック内で `if (features->hasObjectSkinning)` 分岐を追加して `attachVertexObject("deferred/skinnedVelocityV.glsl")` を呼ぶ。alpha 版は `LLGLSLShader::mFeatures` から alpha 系か判定できないので、各 program の `mShaderFiles` 設定時に individual に明示 attach する形でも可 (実装時に決定)。
   結論: **9 shader 全部必須**。`skinnedVelocityV.glsl` / `skinnedVelocityAlphaV.glsl` を「BD が dead 扱いだから削る」のではなく、「BD が link bug で wire してないだけ」と判定し、AYAstorm では正しく wire する。

2. **(2026-05-17 trace 結論) `LLDrawPoolTree / Terrain / Bump / Alpha` の `beginMotionBlurPass / renderMotionBlur` 実装詳細を確定**
   BD `lldrawpoolxxx.cpp` を全部 fetch して各 override body を抜粋確認。結論を §6.8 表に file:line 単位で反映済。要点:
   - **Tree** (`lldrawpooltree.cpp:140-192`): `gVelocityProgram` のみ (static)、helper 不使用、`mDrawFace` を直接 iterate して `face->getVertexBuffer()->drawRange()`。region matrix を `LAST_OBJECT_MATRIX = CURRENT_OBJECT_MATRIX` に同値で積む = velocity ≒ 0 で意図的に近似。
   - **Terrain** (`lldrawpoolterrain.cpp:208-256`): `gVelocityProgram` のみ、helper 不使用、Tree と同じく region matrix を同値積み。完全静的、velocity = 0 出力。terrain 専用 vertex layout はそのまま流せる (velocity shader が requested attribute は position + weight (rigged) のみ、texcoord 等は不要)。
   - **Bump** (`lldrawpoolbump.cpp:594-625`): `gVelocityProgram` (static + rigged variant `bind(true)`)、`pushVelocityBatches(PASS_BUMP)` + `pushRiggedVelocityBatches(PASS_BUMP_RIGGED)` を使う。normal map / tangent attribute は velocity shader が無視するので layout 互換問題なし。
   - **Alpha** (`lldrawpoolalpha.cpp:903-937`): `gVelocityAlphaProgram` (static + rigged)、`pushVelocityBatchesTextured(PASS_ALPHA)` + `pushRiggedVelocityBatchesTextured(PASS_ALPHA_RIGGED)`。**`POOL_ALPHA_PRE_WATER` は `getNumMotionBlurPasses() == 0` で velocity スキップ** — `mLastModelMatrix` が pre-water / post-water で double-stamp されるのを避ける (BD コメント line 905)。velocity pass 自体は blend mode 不使用 (depth read-only + frag shader 内 `bayerDitherDiscard` で alpha 判定 + 単純 color write)、blend モードと velocity buffer の競合は構造上発生しない。
   - **Simple** (`lldrawpoolsimple.cpp`): BD 内 trace で grep に明示 hit せず — BD `lldrawpool.cpp` の trace 結論として、`LLDrawPoolSimple` override が無く base impl `getNumMotionBlurPasses() => 0` で velocity に出ない可能性、もしくは別ヘッダ (`lldrawpoolsimple.h` で inline) で override されている可能性が残る。P2 実装時に BD `lldrawpoolsimple.cpp/h` を再 grep して確定。Simple は静的 prim 中心で velocity ≒ 0 でも実害無し、override 無しでも regression にはならない見込み。

3. **(2026-05-17 trace 結論) `pushVelocityBatches` 系 4 関数は BD `lldrawpool.cpp` (LLRenderPass class) に独自実装、Firestorm `lldrawpool.cpp` に新規追加が必要**
   BD repo に `llrenderpass.cpp/h` ファイルは **存在しない** (LL upstream で `LLRenderPass` は `lldrawpool.cpp` 内に統合済み)。BD 側 trace 結果:
   - 宣言: `lldrawpool.h:402-405`
     ```cpp
     // Velocity buffer helpers — iterate render map, upload per-object last/current matrices, draw
     void pushVelocityBatches(U32 type);
     void pushRiggedVelocityBatches(U32 type);
     void pushVelocityBatchesTextured(U32 type);
     void pushRiggedVelocityBatchesTextured(U32 type);
     ```
   - 定義: `lldrawpool.cpp:802 / 838 / 871 / 912` (LLRenderPass のメンバ関数)
   - 既存 `pushBatches(U32 type, bool texture, bool batch_textures)` (`lldrawpool.cpp:480`) の **thin-wrap ではなく、独自実装**。`LLPipeline::beginRenderMap(type)` / `endRenderMap(type)` で render map iterator を取得し、各 `LLDrawInfo` について:
     - `applyModelMatrix(params)` で現フレーム model matrix を gGL stack に積む (= velocity shader の `modelview_projection_matrix` 経由)
     - `LLShaderMgr::LAST_OBJECT_MATRIX` uniform に `params.mLastModelMatrix` (新規 LLDrawInfo フィールド、null なら identity) を upload
     - `params.mVertexBuffer->drawRange(...)` で draw
     - **draw 後に `*params.mLastModelMatrix = *params.mModelMatrix` で「次フレーム用の前フレーム matrix」を per-drawable に保存** (副作用、重要)
   - `pushRiggedVelocityBatches` は加えて `uploadMatrixPalette(params.mAvatar, ...)` (現フレーム skin) + `uploadLastMatrixPalette(params.mAvatar, params.mSkinInfo)` (前フレーム skin、`lldrawpool.cpp:951` の新規 helper) を呼ぶ。`uploadLastMatrixPalette` は `avatar->updateSkinInfoMatrixPalette(skinInfo)` から `mpc.mLastGLMp` (前フレーム skin matrix cache) を取り出して `LLViewerShaderMgr::AVATAR_LAST_MATRIX` uniform に流す。
   - `*Textured` 版は `gGL.getTexUnit(0)->bindFast(params.mTexture)` を追加 (alpha texture サンプリング用)。
   - 引数: `U32 type` (= `LLRenderPass::PASS_SIMPLE` 等の enum)、motion-blur 専用 flag は無し (rigid/skinned は関数名で区別)。
   - Firestorm 側追加先: `indra/newview/lldrawpool.h:357` (`pushBatches` 宣言の直下)、`indra/newview/lldrawpool.cpp` (`pushBatches` 定義の直後、推定 line 500 周辺)。BD `lldrawpool.cpp:802-948` 4 関数 + `uploadLastMatrixPalette` (line 951+) を AYAstorm provenance comment 付きで移植。
   - `LLDrawInfo::mLastModelMatrix` (新規 `LLMatrix4*` メンバ、`LLDrawInfo` の lifecycle で alloc/free) も BD `lldrawinfo.h/cpp` 相当を追加検証 (Tier-2、§7-2/§7-6 で別途扱い)。

4. **(2026-05-17 trace 結論) Firestorm `LLGLSLShader::mFeatures` に `hasMotionBlur` field が **無い**、追加が必要**
   BD 側構造: `LLShaderFeatures` class は **独立ヘッダ `llshaderfeatures.h` ではなく `indra/llrender/llglslshader.h` の冒頭 (`llglslshader.h:36-64`)** に定義されている (LL upstream で統合済み)。BD の field 一覧:
   - line 46: `bool hasSkinning = false;`
   - line 47: `bool hasObjectSkinning = false;`
   - line 62: `bool hasTonemap = false;`
   - line 63: `bool hasMotionBlur = false;` ← **BD 独自追加**
   Firestorm 側 `indra/llrender/llglslshader.h:36-63` を Read 確認: `hasTonemap` (line 62) までは BD と完全同位置、しかし line 63 で `};` がすぐ来ており、**`hasMotionBlur` field は無い**。BD と Firestorm の `LLShaderFeatures` は `hasMotionBlur` 一行を除けば bit-identical。
   AYAstorm 側追加: `indra/llrender/llglslshader.h:62` の `bool hasTonemap = false;` の直下に AYAstorm provenance comment 付きで:
   ```cpp
   // <AYAstorm r30 P2> Imported from BD 995a1354d8 (llglslshader.h:63).
   // attachShaderFeatures() で velocityFuncV.glsl 自動 attach の gate を担う。
   bool hasMotionBlur = false;
   // </AYAstorm r30 P2>
   ```
   命名選択: BD と field 名を揃えて borrow (= `hasMotionBlur`)、AYAstorm prefix 案 (`hasAYACinematicMotionBlur`) は不採用。理由は (a) `LLShaderFeatures` は単純な flag bag で衝突リスクほぼゼロ、(b) BD との diff を最小化することで上流 BD の今後 update を borrow しやすく保つ、(c) field 名は internal API で UI 露出無し。

5. **(2026-05-17 trace 結論) `make_rigged_variant` は Firestorm に既存、移植不要**
   Firestorm `indra/newview/llviewershadermgr.cpp:258-272` に既に存在:
   ```cpp
   static bool make_rigged_variant(LLGLSLShader& shader, LLGLSLShader& riggedShader)
   {
       riggedShader.mName = llformat("Skinned %s", shader.mName.c_str());
       riggedShader.mFeatures = shader.mFeatures;
       riggedShader.mFeatures.hasObjectSkinning = true;
       riggedShader.mDefines = shader.mDefines;
       riggedShader.addPermutation("HAS_SKIN", "1");
       riggedShader.mShaderFiles = shader.mShaderFiles;
       riggedShader.mShaderLevel = shader.mShaderLevel;
       riggedShader.mShaderGroup = shader.mShaderGroup;
       shader.mRiggedVariant = &riggedShader;
       return riggedShader.createShader();
   }
   ```
   BD `llviewershadermgr.cpp:260-274` と **完全同一実装** (空白/comment 含めて bit-identical)。LL upstream 起源 (file header `firstyear=2007 viewerlgpl`)、BD は LL から borrow しただけ、Firestorm も同じ。AYAstorm 側は既存 helper をそのまま呼び出して OK。
   ただし §7-1 結論を踏まえ、velocity 系 program 専用には `make_rigged_variant` だけでは `skinnedVelocityV.glsl` 不足のため、velocity 系の `createShader` 呼び出し前に `attachVertexObject("deferred/skinnedVelocityV.glsl")` を明示追加する path を入れる (§6 改修ポイントで詳細)。

6. **(2026-05-17 trace 結論) last skin matrix palette upload path は `LLVOAvatar::MatrixPaletteCache::mLastGLMp` (新規 cache field) + `LLRenderPass::uploadLastMatrixPalette` (新規 static helper) で完結**
   BD `llvoavatar.{h,cpp}` の `MatrixPaletteCache` class に `std::vector<F32> mLastGLMp; S32 mLastFrame = -1;` を追加 (BD `llvoavatar.h:884-886`)、`updateSkinInfoMatrixPalette()` (`llvoavatar.cpp:10315-10328`) の `entry.mFrame != gFrameCount` ブランチ冒頭で `entry.mLastGLMp = entry.mGLMp; entry.mLastFrame = entry.mFrame;` (= 今フレームの mGLMp 計算 **前** に前フレーム値を退避)。read 側は `LLRenderPass::uploadLastMatrixPalette(LLVOAvatar*, LLMeshSkinInfo*)` static (`lldrawpool.cpp:951-970`) が `avatar->updateSkinInfoMatrixPalette(skinInfo).mLastGLMp` を取り出して `uniformMatrix3x4fv(AVATAR_LAST_MATRIX, count, ...)` で GL upload。
   uniform 名は **mesh rigged も avatar も同じ `lastMatrixPalette`** (BD `llshadermgr.cpp:1563`、enum 名 `AVATAR_LAST_MATRIX`)。avatar shader 側は `vec4 lastMatrixPalette[45]` 宣言で同じ uniform slot を 540 float (= 45×12) として read、mesh rigged shader は `mat3x4 lastMatrixPalette[N]` 宣言で N×12 float を read。1 helper / 1 uniform name で兼用、program 毎の宣言で reinterpret。
   Firestorm 側追加位置:
   - `llvoavatar.h:875` (`MatrixPaletteCache` class 末尾、`mGLMp` の直下) に 2 field 追加
   - `llvoavatar.cpp:11252` (Firestorm 側の `entry.mFrame = gFrameCount;` 直前) に `if (entry.mFrame > 0) { entry.mLastGLMp = entry.mGLMp; entry.mLastFrame = entry.mFrame; }` を挿入
   - `lldrawpool.cpp` の `LLRenderPass::uploadMatrixPalette` (現フレーム skin upload) 既存関数の直後に `uploadLastMatrixPalette` static helper を BD `lldrawpool.cpp:951-970` から移植
   - `lldrawpool.h:357` (`pushBatches` 宣言の直下、§6.9 と同位置) に `static bool uploadLastMatrixPalette(LLVOAvatar*, LLMeshSkinInfo*);` 宣言を追加
   - `llviewershadermgr.{h,cpp}` の `EShaderConsts` enum に `AVATAR_LAST_MATRIX` 追加 + 文字列登録 `"lastMatrixPalette"` (Firestorm 既存 `AVATAR_MATRIX` = `"matrixPalette"` のすぐ下)
   - mesh rigged 系の `pushRiggedVelocityBatches{,Textured}` (§6.9 で追加した helper) 内で `uploadLastMatrixPalette(params.mAvatar, params.mSkinInfo)` を呼ぶ
   - avatar 系 `LLDrawPoolAvatar::renderMotionBlur` は `avatar->renderSkinned()` 一発に集約、per-mesh upload は `renderSkinned()` 内 LL upstream の skin upload path に併設 (P2 着手時に BD `llvoavatar.cpp` / `LLDrawPoolAvatar::renderRigged` 周辺で `AVATAR_LAST_MATRIX` upload 箇所が既に組まれているはずなので、その対応位置を特定して移植)
   詳細は §6.10 / §6.14 に反映済。

7. **(2026-05-17 trace + AYA 判断確定) SMAA T2x (FSAAType == 3) は P2 で本気実装する — 案 A 採用**

   **判断**: 2026-05-17 AYA 判断で **案 A (P2 で T2x まで本気実装、9-10 日)** を採用確定。判断理由は「視覚で確認しながら P3 (Volumetric) / P4 (Motion Blur) / P5 (BD DoF) の方針を決める方が後段スムーズ」。案 B (velocity のみ 5 日、T2x P4 降格) は ship 判定が debug visualizer 依存で「Cinematic らしさ」を AYA 主観で判定できないため不採用。

   下記 trace ログは判断の根拠資料として保存 (削除しない、再着手時の前提共有用)。

   **BD 側現状 trace**:
   - BD pipeline.cpp で T2x 経路の C++ infrastructure は既に組まれている:
     - `gSMAANeighborhoodBlendT2xProgram[4]` / `gSMAAResolveProgram[4]` 配列 (`llviewershadermgr.cpp:211-212`)
     - `LLPipeline::applySMAA()` (`pipeline.cpp:8072-8137`) で `RenderFSAAType == 3` 時に T2x blend program に分岐 + `SMAA_VELOCITY_TEX` bind
     - `LLPipeline::resolveSMAAT2x()` (`pipeline.cpp:8139-8180`) で前フレーム color (`mSMAAHistory`) + 現フレーム color + velocity を blend して 2-frame avg を作る
     - `sT2xJitterEnabled` static bool (`pipeline.cpp:380`) を宣言
   - **しかし BD 側 T2x は構造的に未完成**:
     - `gSMAAResolveProgram` の `mShaderFiles` が `"deferred/SMAAResolveF.glsl"` / `"deferred/SMAAResolveV.glsl"` を参照 (`llviewershadermgr.cpp:2827-2828`) するが、BD repo の `class1/class2/class3/deferred/` どこにも `SMAAResolve*.glsl` ファイルが **存在しない** (gh contents API 確認済)。`createShader()` は失敗 = `gSMAAResolveProgram` は load 失敗で T2x resolve pass は走らない。
     - `gSMAANeighborhoodBlendT2xProgram` は `mShaderFiles` を既存の `SMAANeighborhoodBlendF.glsl` (BD 側 line 36 で `uniform sampler2D velocityTex;` を持つ、`SMAA.glsl:1379/1411/1437` に velocity reprojection logic あり) で構成 — 既存 SMAA blend shader 自体は velocity 込みで動く設計。
     - `sT2xJitterEnabled` は `pipeline.cpp` 内で `true` に代入される箇所が無い (declaration のみ) — jitter (projection matrix を毎フレーム ±0.5 pixel オフセット) を実際に有効化する path が抜けている。
     - BD は `SMAA.glsl` 内 `SMAA_DECODE_VELOCITY` macro を `sample.rg * 0.5` (Firestorm は `sample.rg`、唯一の真 diff) に変更しており、velocity decode 倍率調整は入っている。
   - 結論: BD の T2x は「pipeline / shadermgr 配線完了 + SMAA shader 内 reprojection logic 完備 + jitter / resolve 経路の 2 ピースが欠落」の状態。BD 側 commit `bb462f6422` "Fixed: SMAA TAA not actually turning on" は jitter / resolve の補完を意図したものと推測されるが、commit を 995a1354d8 snapshot 内で見る限り完成していない。BD 実機でも T2x は 「configured but not running」 と見るのが正確。

   **Firestorm 側現状**:
   - SMAA shader 7 ファイル (`class1/deferred/SMAA*.glsl`) は BD と bit-identical (`SMAA.glsl` の `SMAA_DECODE_VELOCITY` macro 1 行だけが diff)
   - `gSMAAResolveProgram` / `gSMAANeighborhoodBlendT2xProgram` は **存在しない**
   - `RenderFSAAType` enum 値は `0=None, 1=FXAA, 2=SMAA` の 3 値、`3=SMAA T2x` 未定義
   - `mSMAAHistory` RT 無し
   - jitter projection 経路無し

   **採用案 A の改修内容**:

   - 新規 shader 行数: `SMAAResolveF.glsl` (~80 lines) + `SMAAResolveV.glsl` (~30 lines) を新規執筆 (BD repo にも LL upstream にも無い、SMAA reference impl から起こす)。`SMAA.glsl` `SMAA_DECODE_VELOCITY` 1 行修正
   - 新規 C++ 行数: `mSMAAHistory` RT lifecycle (~30 lines) + `resolveSMAAT2x()` 関数 (~50 lines) + `applySMAA` T2x 分岐 (~30 lines) + jitter projection 経路 (~50 lines、`gGLProjection` を毎フレーム ±0.5px shift、`!gCubeSnapshot` 限定) + `gSMAAResolveProgram` / `gSMAANeighborhoodBlendT2xProgram` 配列 + `loadShadersDeferred()` 内 load (~50 lines) + `RenderFSAAType == 3` (T2x) cvar 値追加 + Cinematic mode で auto-set
   - 工数: velocity 系 5 日 + T2x 追加 4-5 日 = **9-10 日**
   - P2 ship 判定: 「Cinematic モード起動で SMAA T2x の時間 reprojection が画面 motion に効いている」(camera pan で edge AA 改善が目視可能、主観判定 ◎)
   - P3 / P4 への波及: SMAA T2x の jitter / history 経路を P3 / P4 で流用可、Motion Blur composite 実装が楽になる
   - リスク認識: T2x と velocity を同時 debug する難しさ + BD の jitter / resolve 欠落部を自前で書き起こす負担 (LL upstream patch 無し、SMAA reference impl Crytek/Jimenez を直接読む)。`SMAA_DECODE_VELOCITY` diff の意味を実機で再現確認する手間も込み。

   詳細改修箇所一覧は §6.15 に統合済。

   **不採用案 B の記録** (将来 P2 を分割する判断が再浮上した場合の参照用):
   - 案 B: P2 は velocity buffer 配線のみ 5 日、T2x は P4 (Motion Blur) に降格して同時実装
   - 不採用理由: ship 判定が `AYACinematicVelocityDebug == 1` の debug visualizer 依存 = AYA 主観で「Cinematic らしさ」を判定不可、P4 まで「Cinematic を入れた価値」が示せない

8. **(2026-05-17 trace 結論) `gCubeSnapshot` / `sReflectionRender` / `sShadowRender` / `sImpostorRender` 4 つの gate で velocity pass を完全抑止する設計が成立、r21.1 先例で確認済**

   Firestorm `pipeline.cpp` の `display()` flow trace 結果:
   - `gCubeSnapshot` (line 262) — cube map snapshot 中 (反射プローブ等) は true、`!gCubeSnapshot` で gate するのが LL 標準パターン。`mObjectIDBuffer` allocate (line 1013-1035) もこの gate を使用。
   - `LLPipeline::sReflectionRender` (line 363) / `sShadowRender` (line 361) / `sImpostorRender` (line 365) — それぞれ reflection / shadow / impostor pass 中 true。pipeline.cpp 内で頻出 (line 2885, 2906, 4228, 4310, 6601 等)。
   - velocity pass の hook (§6.4(d)) は **display() のメイン 3D scene render flow の `renderGeomPostDeferred` 直後** に入れるので、これら 4 gate のいずれかが true な (= cube snapshot / reflection / shadow / impostor 中の) フレームでは renderGeomPostDeferred 自体は走るが gGLLastModelView snapshot を取らない (line 10564-10573 の `if (!gCubeSnapshot)` で守られている) ため、velocity pass を走らせても「前フレーム = 同フレーム = velocity 0」になるだけで実害ゼロ。
   - とはいえ「呼ぶだけ無駄」かつ「sub-RT 系で `mVelocityMap` lifecycle 衝突リスク」 (cube snapshot は `!gCubeSnapshot` ブランチで allocate されない、つまり cube 中に bindTarget すると complete check 失敗) があるので、明示 gate を入れる。

   **r21.1 先例** (`pipeline.cpp:10054-10063`):
   ```cpp
   if (!gCubeSnapshot && (!armed_mode || isSelfRiggedObjectIDBufferArmed()))
   {
       renderSelfRiggedObjectIDBuffer();
   }
   ```
   `mObjectIDBuffer` も同じく cube snapshot で抑止、velocity も同形式で gate 可能。

   **AYAstorm 採用形** (§6.4(d) に反映): `renderGeomMotionBlur()` 冒頭で:
   ```cpp
   if (gCubeSnapshot || LLPipeline::sReflectionRender || LLPipeline::sShadowRender || LLPipeline::sImpostorRender || !mVelocityMap.isComplete())
   {
       return;
   }
   ```
   または display() flow 側 hook の if 条件に同 gate を組み込む形 (Firestorm 既存 style に合わせるなら hook 側に gate を寄せる方が trace しやすい)。`mVelocityMap.isComplete()` check は `mObjectIDBuffer` の line 9965 と同パターン。

   AYAstorm `display()` 内の cube snapshot 用 `renderGeomPostDeferred` 呼び出し 3 箇所 (line 12873/12880/12892 推定 — Read で確認したが該当 line は別 path、L932/L952 で `gCubeSnapshot = true / false` を切り替えている本体 + L10558 の main path の計 3 経路) は **gate で全て抑止される設計** (snapshot 中は `gCubeSnapshot == true`)、velocity pass のために特別な対応は不要。

9. **(2026-05-17 trace 結論) `shareDepthBuffer` の前提は成立、r21.1 `mObjectIDBuffer` と同パターンで自明**

   Firestorm `pipeline.cpp:990` で `mRT->deferredScreen.allocate(resX, resY, GL_RGBA, true)` が `allocateScreenBufferInternal()` 内の最初 (`if (!gCubeSnapshot)` ブランチ外) で実行され、その後 line 1013-1035 の `if (!gCubeSnapshot)` ブランチ内で `mObjectIDBuffer.allocate()` → `mRT->deferredScreen.shareDepthBuffer(mObjectIDBuffer)` の順で配線。allocate 順序は **deferredScreen が先、share する側が後** で確定。`RenderShadowDetail` (line 1180 等) は別の RT (`mShadowMap` 等) を分岐させているだけで `mRT->deferredScreen` の format は触らないため、share 破綻リスクは無い。

   **AYAstorm 採用形** (§6.4(b) に反映): `mObjectIDBuffer.allocate(...)` の直後 (line 1035 の `mRT->deferredScreen.shareDepthBuffer(mObjectIDBuffer);` の直下) に AYAstorm provenance comment 付きで:
   ```cpp
   // <AYAstorm r30 P2> Velocity buffer for Cinematic mode (imported from BD 995a1354d8).
   // Allocate AFTER deferredScreen so shareDepthBuffer can lend depth. Same pattern
   // as r21.1 mObjectIDBuffer (just above). Cinematic-only: skip in other view modes
   // to avoid per-frame clear cost + GPU memory.
   if (gAYAViewMode == AYA_VIEW_MODE_CINEMATIC)
   {
       if (!mVelocityMap.allocate(resX, resY, GL_RG16F, false)) return false;
       mRT->deferredScreen.shareDepthBuffer(mVelocityMap);
   }
   else
   {
       mVelocityMap.release();
   }
   // </AYAstorm r30 P2>
   ```
   release path は `releaseGLBuffers()` (`pipeline.cpp:1442` で `mObjectIDBuffer.release();` の隣) に `mVelocityMap.release();` を併設 — §6.4 の改修ポイント表に反映済。

10. **(2026-05-17 trace 結論) `velocityFuncV.glsl` の自動 attach は `attachShaderFeatures()` の `hasObjectSkinning` 直後 (BD `llshadermgr.cpp:177-183`) に追加された専用ブロック**
    BD `indra/llrender/llshadermgr.cpp:177-183` の該当ブロック:
    ```cpp
    if (features->hasMotionBlur)
    {
        if (!shader->attachVertexObject("deferred/velocityFuncV.glsl"))
        {
            return false;
        }
    }
    ```
    位置: `features->hasObjectSkinning` ブロック (line 168-175) の直後、`attachVertexObject("deferred/textureUtilV.glsl")` (line 185-188) の直前。`mShaderLevel` や class 判定なし、`hasMotionBlur` true なら無条件で `velocityFuncV.glsl` を vertex attach (`writeVaryVelocity()` 関数本体と `vary_cur_clip / vary_last_clip` out 宣言を提供)。
    Firestorm 側 `indra/llrender/llshadermgr.cpp` 確認: line 160-175 で `hasSkinning` / `hasObjectSkinning` 分岐は BD と bit-identical、line 177 で直接 `textureUtilV.glsl` attach に進んでおり **`hasMotionBlur` 分岐は無い**。
    AYAstorm 側追加: Firestorm `llshadermgr.cpp:175` (`hasObjectSkinning` ブロック閉じ括弧の直後、line 177 の `textureUtilV.glsl` attach の直前) に AYAstorm provenance comment 付きで BD と同形の 7 行ブロックを挿入。加えて §7-1 結論に基づき、同ブロック内で `if (features->hasObjectSkinning) attachVertexObject("deferred/skinnedVelocityV.glsl");` を **velocity 系の rigged variant が link 失敗しないよう** 追加する形を検討 (`skinnedVelocityAlphaV.glsl` は alpha 系判定 field が無いので、velocity 系 program 個別に `mShaderFiles` 設定時に直接 attach する形でも可、実装時決定)。

---

## 8. 実装 commit log (`feature/ayastorm-r30-p2-velocity-buffer-spec` ブランチ上)

`ayastorm-release..HEAD` 範囲、古い → 新しい順。Step 番号は §3 の BD trace と §6 の改修ポイント表に対応。

| commit | 内容 |
| --- | --- |
| `7eecbf10ec` | r30 chapter spec: cinematic mode plan + P1 view-mode restart-switch unification |
| `627b5dbef3` | r30 P1: unify View Mode to restart-switch, add Cinematic (preview) slot |
| `75f0d889cf` | r30 P2 spec: velocity buffer + SMAA T2x trace + case A decision (= 本 spec の初版) |
| `dbe4fc508b` | r30 P2 step 1: import velocity buffer shaders from Black Dragon Viewer |
| `016bf501ed` | r30 P2 step 2: add SMAAResolve{V,F}.glsl for SMAA T2x resolve pass |
| `88ab1f3f27` | r30 P2 step 3: add llshadermgr uniforms, hasMotionBlur, mVelocityMap/mSMAAHistory RTs |
| `26e8a1f3ef` | r30 P2 step 4a: register 5 velocity shader programs + hasMotionBlur attach |
| `1c62a2d11b` | r30 P2 step 4b: LLDrawPool velocity virtuals + LLRenderPass push helpers + renderGeomMotionBlur |
| `7d68ec63f3` | r30 P2 step 4c-1: opaque pool motion blur overrides (8 pools / 4 files) |
| `b7d9ddb619` | r30 P2 step 4c-2: face-iter pool motion blur overrides (terrain + tree) |
| `ccc77a536b` | r30 P2 step 4c-3: special pool motion blur overrides (alpha + avatar) |
| `06c0fb9d55` | r30 P2 step 5a: display() hook + velocity buffer visualization + skinned variant fix |
| `2f89d22f9d` | r30 P2 step 5b: motion blur composite + per-drawable velocity matrix hookup |
| `c12bd5ddb2` | r30 P2 step 5c+5d: SMAA T2x resolve + 2-tap subpixel jitter |
| `c7f4d3fef8` | r30 P2 spec: append §8 実装 commit log + step 5 受入観測 |
| `c70c65d76e` | r30 P2 step 5e: motion blur avatar opt-out + composite bleed gate |

Step 5 受入観測 (2026-05-18 AYA):
- **5b motion blur**: SIM 境界の radial blur "玉" は `LLDrawable::mLastVelocityMatrix` + `LLDrawInfo::mLastModelMatrix` 配線で解消、avatar lightning streak は NaN/inf guard で抑止、static building の subpixel drift は noise floor 2.0px で抑止。
- **5c SMAA T2x resolve**: 配線のみでは ON/OFF 差ゼロ (jitter 無しでは 50/50 blend が同一サンプルの平均 = identity)、これは仕様。
- **5d Halton 2-tap jitter**: 高周波エッジ (木の葉、髪、細枝) で SMAA 単独より若干滑らかになる差を確認。建物 / 地形などの直線エッジは既存 SMAA で取り切られているので追加効果は小さい。これは 2-tap (T2x の "2x") の理論限界に沿った結果で、ghost / smear は確認されず → jitter ↔ velocity reprojection ↔ resolve の lockstep 成立を確認。
- **5e avatar opt-out + bleed gate**: 2-stage で完成。
  - *write side*: `RenderMotionBlurSelfAvatar` / `RenderMotionBlurOtherAvatars` (Boolean, default 1) を `LLDrawPoolAvatar::renderMotionBlur` + 4 push helpers (`pushVelocityBatches{,Textured}` / `pushRiggedVelocityBatches{,Textured}`) で読み、該当 avatar の velocity 書込みを skip。`LLDrawInfo::mAttachedToAvatar` (新規) で static prim attachment の wearer 紐付けを `mAvatar` (rigged 専用) と独立に持たせる。velocity RT が `(0,0)` clear 済 + composite の `speed < 2.0` 分岐で当該画素は unblurred。
  - *診断*: `RenderBufferVisualization = 7` で velocity buffer 直接確認 — opt-out 中の avatar 画素が真っ黒 (R=G=0) であることを確認、skip 経路の動作を実機で検証 (2026-05-18 AYA)。
  - *composite bleed gate (motionBlurF.glsl)*: write side skip 単独では「高 velocity な BG 画素の 32-tap blur が、opt-out された avatar 画素の diffuse を sample して halo として滲み出る」アルゴリズム的副作用が残る。per-sample velocity gate を追加し、各 sample 位置の velocity が 2 px noise floor 未満なら weight に含めない。avatar 画素は除外され、`total < 1e-3` の場合は center pixel をそのまま return。3rd-person camera pan で self / other 両方の halo 消失を確認 (2026-05-18 AYA)。
  - *副次の skinning 修正 (A2.2)*: BD の `skinnedVelocityV.glsl` / `skinnedVelocityAlphaV.glsl` は `current_clip = modelview_projection_matrix * pos` で object skinning を skip しており、rigged mesh は bind pose (T-pose) でラスタライズ → `last_clip` 側は skinned matrix で全フレーム巨大 velocity (= "T-pose ⇄ 現ポーズ" の偽 motion)。velocityV.glsl HAS_SKIN path と同じ `projection * (modelview * (cur_mat * pos))` に揃え修正。
  - *helper 整理*: `uploadLastMatrixPalette` の `force_zero` 引数を削除 — 同等効果は upstream skip + RT clear で達成され、uniform 経由の zero-write 機構は不要 (over-engineering 撤去)。first-frame `mLastGLMp` empty 時の `mGLMp` fallback は維持 (前 rig の matrix palette を read して "lightning-streak velocity" が出る回避)。
  - *既知の運用 caveat*: Debug Settings から `RenderMotionBlur{Self,Other}Avatars` を toggle した値は **Debug Settings の Window を閉じた時点で commit** される (auto-widget の commit タイミング、code 側 bug ではない)。release note / 運用 tip 側で明記、cvar 型変換 (Boolean → U32) は不採用 (2026-05-18 AYA 判断、影響軽微につき memory rule 適用見送り)。

ブランチ状態 (本 commit log 追記時点): `feature/ayastorm-r30-p2-velocity-buffer-spec` を `origin` に push 済 (step 5e まで反映、`c70c65d76e`)。`ayastorm-release` への PR は本 spec 更新の commit を以て「最終調整完了」とする (2026-05-18 AYA 判断)。

---

## 9. 関連 spec / memory

- `docs/specs/ayastorm-r30-cinematic-chapter.md` — 親 spec、章レベル骨子 (§3 P2 / §4.2 borrow list)
- `docs/specs/ayastorm-r30-p1-view-mode-restart-switch.md` — 前段 spec、P1 で 3 モード再起動切替統一 + Cinematic 枠 UI 追加 (本 spec §6.12 startup snapshot は P1 で予告された P2 着手必要事項)
- `docs/specs/ayastorm-deferred-shader-routing.md` — Firestorm 側 deferred shader routing の確定マップ。新規 shader 追加時の Pool→bound shader 整合確認に必須
- `docs/specs/ayastorm-gbuffer3-trace.md` — gbuffer3 alpha storage 仕様。本件は MRT 拡張ではなく独立 RT なので直接の影響なし、ただし `shareDepthBuffer` 前提として既存 gbuffer 構成を把握する目的で参照
- `memory/project_ayastorm_r30_cinematic_chapter.md` — 章 memory 要約
- `memory/feedback_render_full_trace_first.md` — 描画系は推論禁止・完全トレース優先 (本 spec の作成方針)
- `memory/reference_deferred_shader_routing.md` — Pool → bound shader → gbuffer flag → softenLightF 分岐マップ
