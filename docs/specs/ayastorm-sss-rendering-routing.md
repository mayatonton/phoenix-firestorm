# AYAstorm SSS (Subsurface Scattering) Rendering Routing リファレンス

**作成日**: 2026-05-25
**作成経緯**: FullBright (FB) prim の後ろに SSS 設定アバターが居ると、SSS 色 (pink/salmon) の影が FB prim を**透けて見える**。AYAstorm 自作機能 `doSkinSSS` の routing がそもそも適切でない可能性を AYA さんが指摘。過去 2 回 3-4 時間ずつ shader 推論で潰そうとして両方失敗、機能軸 (effect 軸) で routing を地図化して構造原因を確定するための資料。
**関連資料**:
- `docs/ayastorm-deferred-shader-routing.md` (object class 軸の routing)
- `docs/ayastorm-attachment-rendering-routing.md` (装着物 routing)
- `docs/ayastorm-rez-object-rendering-routing.md` (rez object routing)
- `docs/specs/ayastorm-gbuffer3-trace.md` (gbuffer3 storage 仕様)
- `docs/specs/spec_avatar_skin_sss.md` (r20 SSS 機能仕様書)

**ラベル**:
- すべて **AYAstorm 固有** (`<FS:AYA r20 ...>` / `<FS:AYAstorm r30 ...>` で囲われた範囲)。
- Firestorm 上流に SSS 機能は無い (本機能は r20 で AYAstorm が独自追加した screen-space SSS)。

---

## TL;DR — bleed の根本原因 3 候補 (最も疑わしい順)

bug の症状「FB prim を透けて pink shadow が見える」を、コードトレースで物理的に再現可能な構造的原因として 3 つに絞った:

1. **【最有力】 FullBright は deferred-opaque pass に参加せず `gbuffer3` に書かない → FB-covered pixel の `gbuffer3.a` は背後のアバターが先に書いた `aya_sss_skin_flag = 1` のまま残る。** SSS pass は `emissiveRect.a` を skin mask として読むが、それは「FB の背後にある avatar の skin bit」を読む。FB 自体が `vec4(0)` を `frag_data[3]` に書くチャンスが構造上ゼロ。FB を**深度 occluder としてしか登場させない** (post-deferred で `mRT->screen.rgb` だけ書く) ので、深度的に FB が手前でもマスクは avatar のものが透けて見える。
   - 根拠: `lldrawpoolsimple.h:147` (FB は `getNumPostDeferredPasses()=1` のみ、`getNumDeferredPasses` override 無し)、`lldrawpoolsimple.cpp:156-182` (FB は post-deferred で `mRT->screen` だけ書く、gbuffer3 にはタッチしない)、`fullbrightF.glsl:28` (`out vec4 frag_color;` 単一 RT 出力)。

2. **【次点】 SSS pass の入る位置が「post-water alpha 直前」= **post-deferred opaque + post-FullBright** より後。** `doSkinSSS()` のディスパッチ条件 `cur_type >= POOL_ALPHA_POST_WATER` (`pipeline.cpp:5242`) が走るのは、FullBright (POOL_FULLBRIGHT, POOL_FULLBRIGHT_ALPHA_MASK) のポスト・デファード描画**を含む** opaque/post-deferred 全部が終わった後。よって `diffuseRect` (mRT->screen) には FB の色がすでに焼き込まれていて、5-tap blur がそれを**サンプリング元**にしてしまう。skin mask が立っていなければ問題ないが (1) と組み合わさって stale な mask が立つので、FB の白色を pink-tinted weight でぼかして avatar 風の影に化けさせる。
   - 根拠: `pipeline.cpp:5242-5265`, `pipeline.cpp:11758-11920` (`doSkinSSS` 実装)。

3. **【補強】 gbuffer3 のクリア色が `(1, 0, 1, 1)` で、`.a = 1.0` が default 値。** 描画されない (=深度的に手前のものに完全に occlude された) 領域は **default で skin mask = 1.0** になる。sky 限定では skinSSSF が `d_raw >= 0.9999` で early return する safety net があるが (`skinSSSF.glsl:118`)、それは sky pixel のみで、**FB prim に隠れて何も書かれなかった avatar 領域には効かない** (depth は FB が書くので near-side、sky early-out 条件に該当しない)。
   - 根拠: `llviewerdisplay.cpp:1093` (`glClearColor(1, 0, 1, 1)`), `skinSSSF.glsl:118-123` (sky safety net は depth==1.0 のみ)。

**結論**: 場当たり的に「FB prim 用 if 文」を入れても潰せないのは、構造上 FB が **gbuffer3 に書く機会を持たない** から。fix の方向性は (a) FB prim を**深度 pre-pass で skin_flag=0 を gbuffer3.a に書かせて occlude する** (= FB 専用 frag_data[3] 出力経路を追加)、または (b) SSS pass で `gbuffer3.a` ではなく **`emissiveRect.a == aya_sss_skin_flag` AND depth match against the surface that wrote it** で二段検証する (depth が一致しない時は mask=0)、もしくは (c) skin bit を gbuffer3 から **stencil** に移して FB が深度書込み時に stencil clear させる、のいずれか。詳細は §F に。

---

## A. SSS 機能の入口 (cvar / uniform / 配線)

### A.1 cvar (gSavedSettings)

| Cvar | type | default | 役割 | 参照 |
|------|------|---------|------|------|
| `AYAVisualRealismEnabled` | U32 | 1 | View mode (0=Firestorm / 1=AYAstorm / 2=Cinematic)、`>0` で SSS dispatch 許可 | `pipeline.cpp:11771` `pipeline.cpp:5251` |
| `AYAR20AvatarSkinSSSEnabled` | bool | true (出荷時 ON) | SSS master toggle、mode 1/2 共通 (r30 consolidation 後) | `pipeline.cpp:11772` `pipeline.cpp:5253` |
| `AYAR20AvatarSkinSSSBlurRadius` | F32 | 1.0 | 1m 基準の pixel タップ間隔 (世界座標スケール、距離で逆比例) | `pipeline.cpp:11792` |
| `AYAR20AvatarSkinSSSStrength` | F32 | 0.5 (出荷)、AYA 検証実用 0.7 | pass2 の blend 係数 (= mix factor) | `pipeline.cpp:11793` |
| `AYAR20AvatarSkinSSSGlowGain` | F32 | 0.2 (出荷)、AYA 検証実用 3.0 | glow restore の強度 | `pipeline.cpp:11795` |
| `AYAR20AvatarSkinSSSGlowColor` | Color4 | warm salmon | glow restore tint | `pipeline.cpp:11796` |
| `AYAR20AvatarSkinSSSWhitelist` | String | (空) | mesh UUID newline-separated whitelist | `llayaskinsss.cpp:23` |
| `RenderEnableEmissiveBuffer` | bool | **0 (default OFF)** | gbuffer3 の物理 attachment を制御。これが OFF だと SSS は実質効かない | `settings.xml:11924-29` `pipeline.cpp:499` |

### A.2 GLSL uniforms (SSS shader 内)

`indra/llrender/llshadermgr.{h,cpp}` の `LLShaderMgr::ReservedUniforms` に追加された AYAstorm reserved uniform:

| Uniform 名 | type | 配線元 | 配線先 shader | 役割 |
|------------|------|--------|---------------|------|
| `aya_blur_dir` | vec2 | `pipeline.cpp:11806,11845,11897` | `skinSSSF.glsl:39` | (1,0) pass1 horizontal / (0,1) pass2 vertical |
| `aya_strength` | float | `pipeline.cpp:11807,11846,11898` | `skinSSSF.glsl:40` | pass1=1.0 (scratch fill) / pass2=cvar strength |
| `aya_blur_radius` | float | `pipeline.cpp:11808,11847,11899` | `skinSSSF.glsl:41` | 1m 基準 pixel |
| `aya_glow_gain` | float | `pipeline.cpp:11809,11848,11900` | `skinSSSF.glsl:42` | glow restore gain |
| `aya_glow_color` | vec3 | `pipeline.cpp:11810,11849,11901` | `skinSSSF.glsl:43` | glow restore tint (RGB) |
| `AYA_VISUAL_REALISM_ENABLED` | int | `pipeline.cpp:11851,11903` | `skinSSSF.glsl:44` | master gate (現在は r20_active で gate 済のため常に 1) |
| `AYA_R20_SKIN_SSS_ENABLED` | int | `pipeline.cpp:11852,11904` | `skinSSSF.glsl:45` | r20 gate (現在常に 1) |
| `AYA_SSS_SKIN_FLAG` | float | `lldrawpool.cpp:1093-1097`, `lldrawpoolavatar.cpp:904,948`, `lldrawpoolmaterials.cpp:155-229` | `avatarF.glsl:37` `pbropaqueF.glsl:60` `materialF.glsl:209` | per-draw 0.0/1.0、gbuffer3.a に書く |

### A.3 per-draw flag propagation (whitelist → GBuffer)

```
[ユーザー右クリック / cvar 編集]
  ↓
SkinSSSMatcher::instance().reloadAndReevaluate()           [llayaskinsss.cpp:184]
  ↓
LLCharacter::sInstances 全走査 → 各 avatar.attachment → setSSSTargetForAttachment
  ↓
apply_sss_flag()                                            [llayaskinsss.cpp:227]
  └─ obj->setSSSTarget(match)                               [llviewerobject.h:208]
  └─ gPipeline.markRebuild(REBUILD_GEOMETRY)                [llayaskinsss.cpp:234]
  └─ group->setState(GEOM_DIRTY)                            [llayaskinsss.cpp:237]
  ↓
[次フレーム以降 LLVolumeGeometryManager::rebuildGeom]
  ↓
LLVOVolume::registerFace → LLDrawInfo 構築                  [llvovolume.cpp:5842]
  └─ draw_info->mIsSSSTarget = vobj->isSSSTarget()
  ↓
[毎フレーム描画時 deferred opaque pass]
  ↓
LLDrawPoolMaterials::pushBatch (legacy material) 等
  └─ glUniform1f(AYA_SSS_SKIN_FLAG, params.mIsSSSTarget ? 1.0 : 0.0)
  ↓
materialF / pbropaqueF / avatarF
  └─ frag_data[3] = vec4(0, 0, 0, aya_sss_skin_flag)        [gbuffer3.a に書く]
```

---

## B. SSS pass の挿入位置

### B.1 dispatch site (pipeline.cpp)

`LLPipeline::renderGeomPostDeferred` 内の **pool 反復ループ**で、`POOL_ALPHA_POST_WATER` (= `atmospherics_pass`) 境界に達した瞬間に `doSkinSSS()` を呼ぶ:

| 位置 | コード | 注釈 |
|------|--------|------|
| `pipeline.cpp:5200` | `U32 atmospherics_pass = LLDrawPool::POOL_ALPHA_POST_WATER;` | dispatch 閾値 |
| `pipeline.cpp:5242-5265` | `if (cur_type >= atmospherics_pass && !done_atmospherics) { doAtmospherics(); ...; if (dispatch_r20) doSkinSSS(); }` | 1 フレーム 1 回 |
| `pipeline.cpp:5255` | `bool dispatch_r20 = (aya_view_mode() > 0) && aya_r20_enabled_disp;` | call-site gate |
| `pipeline.cpp:11758` | `void LLPipeline::doSkinSSS()` | 本体 |
| `pipeline.cpp:11762` | `if (sImpostorRender || gCubeSnapshot) return;` | impostor / cube map では走らない |
| `pipeline.cpp:11774` | `if (!r20_active) return;` | self-gate (二重ガード) |

### B.2 反復ループにおける順序

`LLDrawPool` enum order (`lldrawpool.h:57-79`):

```
POOL_SKY (1)
POOL_WATEREXCLUSION
POOL_WL_SKY
POOL_SIMPLE
POOL_FULLBRIGHT       ← FullBright 通常 (post-deferred で render)
POOL_BUMP
POOL_MATERIALS        ← legacy material (deferred opaque、gbuffer3.a 書く)
POOL_GLTF_PBR         ← GLTF PBR (deferred opaque、gbuffer3.a 書く)
POOL_TERRAIN
POOL_GRASS
POOL_GLTF_PBR_ALPHA_MASK
POOL_TREE
POOL_ALPHA_MASK
POOL_FULLBRIGHT_ALPHA_MASK
POOL_AVATAR           ← Linden 標準 avatar (deferred opaque、gbuffer3.a 書く)
POOL_CONTROL_AV       ← animesh
POOL_GLOW
POOL_ALPHA_PRE_WATER
POOL_VOIDWATER
POOL_WATER
POOL_ALPHA_POST_WATER ← ★ ここで doSkinSSS() が dispatch される
POOL_ALPHA
```

**重要**: ループは pool order で回るが、各 pool の `renderPostDeferred` は **同じループ内**で呼ばれる (`pipeline.cpp:5281-5295`)。よって POOL_SIMPLE / POOL_FULLBRIGHT / POOL_FULLBRIGHT_ALPHA_MASK の `renderPostDeferred` (= FB prim を `mRT->screen` に焼く処理) は **`doSkinSSS()` より前**に完了している。

### B.3 段階的タイムライン (1 フレーム)

| Phase | 何が起きるか | gbuffer3.a の状態 | mRT->screen.rgb の状態 |
|-------|-------------|-------------------|------------------------|
| 1. `deferredScreen.clear()` | `glClearColor(1, 0, 1, 1)` | **全 pixel = 1.0** | (まだ未使用) |
| 2. `renderGeomDeferred` (opaque) | sky/material/PBR/avatar 等が gbuffer3 に書く | sky=0.0, material=0/1, avatar=0/1, **FB 領域はクリア値 1.0 のまま** | (まだ未使用) |
| 3. `renderDeferredLighting` (`softenLightF`) | gbuffer → `mRT->screen` に lighting 解決 | (read-only) | sky+opaque lit color |
| 4. `renderGeomPostDeferred` の前半 (POOL_SIMPLE 以降の `renderPostDeferred`) | FullBright / glow / alpha が `mRT->screen` に上書き、**gbuffer3 にはタッチしない** | (変化なし) | FB pixel が opaque lit の上に塗られる |
| 5. `cur_type >= POOL_ALPHA_POST_WATER` → `doSkinSSS()` | SSS pass 1 (horizontal) / pass 2 (vertical)、`emissiveRect` (= gbuffer3) を skin mask、`diffuseRect` (= mRT->screen) を blur source | (read-only) | **FB pixel に対して、gbuffer3.a が 1.0 なら blur 結果を blend** ← bleed の発火点 |
| 6. POOL_ALPHA_POST_WATER 以降の alpha 描画 | 透過物が `mRT->screen` に上書き | (変化なし) | alpha 透過物が上に乗る |

---

## C. どの shader が「SSS 計算」をするか

### C.1 SSS pass 本体 (= screen-space blur)

| Shader | 役割 | 入力 | 出力 | 参照 |
|--------|------|------|------|------|
| `class1/deferred/skinSSSV.glsl` | screen-triangle pass-through VS | screen quad | `vary_fragcoord` | (短いので省略) |
| `class1/deferred/skinSSSF.glsl` | 5-tap separable blur + 波長依存重み + glow restore + skin mask | `diffuseRect` (scene), `emissiveRect` (gbuffer3, mask), `depthMap` (gbuffer-depth, eye_dist 計算), `inv_proj` (NDC→eye)、cvar uniform 群 | `frag_color = vec4(blurred_rgb, strength * skin_bit)` | `skinSSSF.glsl` 全体 |

`skinSSSF.glsl` の構造:

1. **Master gate** (`l.77-81`): `aya_visual_realism_enabled <= 0 || aya_r20_skin_sss_enabled <= 0` で pass-through (alpha=0、pass2 blend で no-op)。
2. **Sky safety net** (`l.118-123`): `d_raw >= 0.9999` (far plane) で `frag_color = vec4(0)`、sky pixel の SSS 暴走を防止。
3. **Eye distance reconstruction** (`l.125-127`): NDC depth → `inv_proj * vec4(0,0,ndc_z,1)` → `eye_dist = abs(vp.z/vp.w)`。
4. **World-scale blur radius** (`l.128-130`): `r_eff = aya_blur_radius / max(eye_dist, 1.0)`、1m 以上は逆スケール。
5. **5-tap separable blur** (`l.133-139`): weights 配列で R を広く / B を狭く配分、波長依存にじみを擬似的に作る。
6. **Inner glow + glow restore** (`l.146-154`): blurred sum の輝度をベースに warm 加算と sRGB power curve で peak 増強。
7. **Mask application** (`l.171-173`): `skin_bit = (gbuffer3.a >= 0.5) ? 1.0 : 0.0`、`frag_color.a = aya_strength * skin_bit`。

### C.2 gbuffer3.a writer 一覧 (skin flag を書く shader)

| Shader | gbuffer3.a write | 用途 | 参照 |
|--------|------------------|------|------|
| `class1/deferred/avatarF.glsl` | `aya_sss_skin_flag` | Linden 標準 avatar body (gDeferredAvatarProgram) | `avatarF.glsl:70` |
| `class1/deferred/pbropaqueF.glsl` | `max(emissive, aya_sss_skin_flag).a` | GLTF PBR opaque (POOL_GLTF_PBR) | `pbropaqueF.glsl:131` |
| `class3/deferred/materialF.glsl` | `aya_sss_skin_flag` | legacy material (PASS_MATERIAL*/SPECMAP*/NORMMAP*/NORMSPEC*) | `materialF.glsl:450` |
| `class1/deferred/sky*F.glsl`、`cloudsF.glsl`、`sunDiscF.glsl`、`moonF.glsl`、`starsF.glsl` | `0.0` | sky/celestial が gbuffer3.a=0 にクリア | `skyF.glsl:131` 他 |
| `class1/deferred/bumpF`、`diffuseF*`、`treeF`、`terrainF`、`impostorF`、`highlightF` | `0.0` | 旧 simple pass の opaque | gbuffer3-trace doc 参照 |
| `class1/gltf/pbrmetallicroughnessF.glsl` | `max(emissive,0).a` = 通常 0 | GLTF scene manager (forward) | 該当 shader |

**観測**: 上記 writer はすべて **deferred 描画パス** で動く。**FullBright (`fullbrightF.glsl`) は frag_data[3] を持たない** (single `out vec4 frag_color`)。

### C.3 gbuffer3.a を書かない shader

- `class1/deferred/fullbrightF.glsl` — single render target only (`out vec4 frag_color;`、`l.28`)。POOL_SIMPLE / POOL_FULLBRIGHT の post-deferred 経路で使われ、deferredScreen FBO は bind されていない。
- `class1/deferred/fullbrightV.glsl` (VS、当然) — N/A
- alpha pool 系の forward shader (`pbralphaF.glsl`, `alphaF.glsl`, `fullbrightF.glsl#IS_ALPHA` permutation 等) — alpha は alpha-pre/post-water pool で `mRT->screen` のみに書く。

→ **これらの shader が描いた pixel の `gbuffer3.a` は、その背後の deferred-opaque pass が書いた値のまま残る**。これが §A.2 の bleed origin。

---

## D. SSS pass のサンプリング源 / depth gate

### D.1 各 texture binding (両 pass)

| Sampler | 内容 | bind 元 (pipeline.cpp) |
|---------|------|------------------------|
| `diffuseRect` (DEFERRED_DIFFUSE) | pass1: `mRT->screen` (deferred lit + post-deferred 焼込済) / pass2: `mWaterDis` (pass1 結果) | `pipeline.cpp:11828, 11880` |
| `emissiveRect` (DEFERRED_EMISSIVE) | `deferred_target->bindTexture(3, ...)` = gbuffer3.a | `pipeline.cpp:11831-11837, 11883-11889` |
| `depthMap` (DEFERRED_DEPTH) | deferredScreen.depth (eye_dist 計算用) | `pipeline.cpp:11841, 11893` |
| `inv_proj` | LLRender reserved uniform (自動 bind) | (auto) |

### D.2 offset 範囲

`skinSSSF.glsl:130` の `step = aya_blur_dir * r_eff / screen_res`、5 タップ `t = -2..+2`。

- pass1 (horizontal): `step = (r_eff/W, 0)`、両側 ±2 pixel × `r_eff`
- pass2 (vertical): `step = (0, r_eff/H)`、両側 ±2 pixel × `r_eff`

世界座標スケールで、近接 (eye_dist=1m, aya_blur_radius=1.0) なら半径 1 pixel × 2 = ±2 pixel、10m 先なら ±0.2 pixel = 実質 0。**近接時の最大ブリード幅は ±2 pixel × 2 pass = 最悪 ±4 pixel 程度**。

### D.3 depth 境界処理

**重要**: SSS pass は **per-tap depth comparison を行わない**。タップ毎の depth と中心 pixel の depth を比較してブリード範囲を制限するロジックは無い。`skinSSSF.glsl` には center pixel の `d_raw` を取って eye_dist を計算する処理しか無い (`l.103`)。

| 境界ロジック | 有無 | コメント |
|-------------|------|---------|
| center pixel sky early-out (`d_raw >= 0.9999`) | あり | `skinSSSF.glsl:118-123` |
| per-tap depth check (= bilateral blur) | **なし** | これが入っていれば FB との境界で blur が止まるが、現状は無条件サンプリング |
| stencil test | **なし** | gbuffer3.a を mask に使う設計 |
| skin_mask threshold (0.5) | あり | `skinSSSF.glsl:171-172`、binary 化 |

→ 「FB の手前/avatar の奥」を区別する depth 検証は中央 pixel の sky 判定のみ。5-tap blur のサンプリング元 (`diffuseRect`) は **無条件** に近傍 4 pixel を読み込む。

### D.4 stencil

stencil bit は本機能では使われていない (`pipeline.cpp` の `doSkinSSS` で stencil 設定なし)。

### D.5 depth test 設定

`pipeline.cpp:11812`: `LLGLDepthTest depth(GL_FALSE, GL_FALSE)` — pass1/pass2 ともに depth test/write OFF。screen-quad なので当然だが、`gbuffer3.a` mask だけが gate。

---

## E. FullBright との交差点 (bleed の物理)

### E.1 FullBright の routing

| Shader | 経路 | render target | gbuffer3.a への書込 |
|--------|------|---------------|---------------------|
| `gDeferredFullbrightProgram` | POOL_FULLBRIGHT::renderPostDeferred (post-deferred) | `mRT->screen` | **無** |
| `gDeferredFullbrightAlphaMaskProgram` | POOL_FULLBRIGHT_ALPHA_MASK::renderPostDeferred / POOL_ALPHA_*::render | `mRT->screen` | **無** |
| `gDeferredFullbrightAlphaMaskAlphaProgram` | POOL_ALPHA_*::render (alpha-blend 経路) | `mRT->screen` | **無** |
| `gHUDFullbrightProgram` | HUD パス | (HUD FBO) | **無** (IS_HUD permutation で対象外) |

参照: `lldrawpoolsimple.cpp:163-181`、`lldrawpoolalpha.cpp:174-178`、`llviewershadermgr.cpp:2030-2031`。

### E.2 FB-covered pixel の gbuffer3.a 由来 (場合分け)

「FB prim の screen pixel P」における gbuffer3.a の最終値は以下の順で決まる:

1. `deferredScreen.clear()` → `.a = 1.0` (`llviewerdisplay.cpp:1093`)
2. deferred-opaque pass で P を覆う最後の writer が走る:
   - もし P が **sky 領域 (FB の背後に何もない)** → skyF が `.a = 0` を書く → P.a = 0
   - もし P が **avatar 領域 (FB の背後に avatar が居る)** → avatarF/pbropaqueF/materialF が `.a = aya_sss_skin_flag` を書く → P.a = 0 (whitelist 未登録) or **1.0 (登録済)**
   - もし P が **GLTF PBR forward 領域** → pbrmetallicroughnessF が `.a = 0` を書く → P.a = 0
3. post-deferred で FB が P を覆って `mRT->screen.rgb` に色を焼く。**gbuffer3.a は変化しない**。
4. doSkinSSS が走る。P で `emissiveRect.a = 1.0` (avatar が登録済 SSS target だった場合) → `skin_bit = 1.0` → strength=0.7 で blur 結果を mix → **FB に avatar 色がぼかされて焼き付く**。

### E.3 直接観測される現象との対応

| AYA さん報告 | コード上の発火点 |
|--------------|------------------|
| 「SSS 色 (pink) の影が FB を透ける」 | E.2 の step 4。skin_bit=1 のまま FB pixel に対して blur がかかり、avatar 色 (lit skin = pink salmon 系) が ±2 pixel の 5-tap で持ち込まれる |
| 「アバターの頭/体/衣類が pink shadow として漏れる」 | 衣類が SSS target に登録されていなくても、隣接する skin 部位 (頭/体) の lit 色を blur 結果に含む → 衣類覆い領域に隣の skin の pink がにじむ |
| 「FB の輪郭に沿って漏れる」 | 5-tap separable blur のタップが ±2 pixel まで届くため、FB silhouette の縁 ±2-4 pixel 幅で skin color が滲む |

---

## F. 既知の bleed origin 候補 (fix 検討用、推測含む)

| 候補 | 根拠 | 影響 | fix の方向性 |
|------|------|------|--------------|
| **F1. FullBright が gbuffer3.a を書かない (← 最有力)** | §C.3、§E | FB 背後の avatar の `aya_sss_skin_flag=1` が stale で残る | (a) FB shader に `frag_data[3] = vec4(0)` を書ける permutation を追加し、FB の deferred-pre-pass を作る / (b) skin mask に depth match を併用 (skin pixel の depth と中心 pixel depth が乖離してたら mask=0) / (c) skin bit を stencil に移行し FB depth 書込時に clear |
| **F2. SSS pass が post-deferred + post-FB な位置にある** | §B.3 | FB の色が `mRT->screen` に焼かれた後で blur source として使われる | SSS pass を `renderDeferredLighting` 直後 (= softenLight 解決直後) に移動。POOL_SIMPLE / FB の post-deferred より前。ただし atmospherics/water ordering との競合確認要 (推測) |
| **F3. SSS blur に depth-bilateral が無い** | §D.3 | ±2 pixel 範囲で無条件サンプル、depth 境界で stop しない | per-tap で `depthMap` を読み中心 depth との差 > 閾値なら weight=0 にする (Jimenez Separable SSS は元々 bilateral 込み)。実装コストは shader 5 行程度 |
| **F4. gbuffer3.a の clear color が 1.0** | `llviewerdisplay.cpp:1093` | 何も書かれない領域が default で skin mask=1 になる (sky early-out で救うのは sky 限定) | `glClearColor(1, 0, 1, 0)` に変更 (sky 等の magenta canary は維持しつつ `.a` のみ 0)。広範囲影響あるので git grep `(1, 0, 1, 1)` で当たり確認要 |
| **F5. avatar の per-prim mask 飛ばし**: rigged BLEND (透過服) の **alpha pool 描画** が gbuffer3.a を **クリアしない** | `lldrawpoolalpha.cpp` は alpha pool で `mRT->screen` のみ書く、gbuffer3 不変。透過服の背後の skin bit が露出 | 透過服越しの skin にも SSS blur が露出 (本来は隠れている skin 部位の色がにじむ) | 透過服に対しても deferred-pre-pass で `gbuffer3.a=0` を書く (= F1 と同根) |
| **F6. blur radius が cvar で大きく振れる** | `pipeline.cpp:11792`, AYA 検証実用値 r=1.0 だが UI 上 max 32 | r が大きいと近接時 ±2*32 = ±64 pixel まで滲み、FB 越え bleed が視覚的に顕著化 | UI で max を絞る (例: max 4) / 強警告 — 推測、優先度低 |

---

## G. 追記事項 / 推測フラグ

以下は**コードから物理的に確認できなかった**項目。`docs/specs/spec_avatar_skin_sss.md` の Phase D2 や git log 履歴と矛盾しない範囲で並べたが、確証は無い:

- **F2 の修正で水面・大気との順序が壊れる可能性**: 上流 LL の `doAtmospherics` / `doWaterHaze` 順序を変更すると water 透過の歪みが入りうる。実機検証必須。(推測)
- **F1 の (a) 案で FB shader に frag_data[3] を追加すると、`HAS_EMISSIVE` permutation との互換性**: 既存 FullBright が deferred FBO に bind されない設計なので、別 program を作る必要がある可能性あり (= 工数大)。(推測)
- **stencil 移行 (F1-c) の VRAM/format コスト**: `deferredScreen` の depth attachment が stencil 込みかどうかは未確認。`addColorAttachment` 周辺で depth/stencil format を確認する必要あり (推測)。
- **rigged BLEND alpha (clothing) の挙動**: 上記 F5 は構造的に成立するが、実際に SL の典型 BoM 装着で「透過服が skin pixel を覆っている」配置がどの程度頻発するかは不明。実機 repro 要 (推測)。

---

## H. 参考 file:line 索引

### CPU 側
- `indra/newview/llayaskinsss.h:35` — `SkinSSSMatcher` singleton 定義
- `indra/newview/llayaskinsss.cpp:184-210` — `reloadAndReevaluate`、全 avatar 再評価
- `indra/newview/llayaskinsss.cpp:227-241` — `apply_sss_flag`、`setSSSTarget` + REBUILD_GEOMETRY
- `indra/newview/llviewerobject.h:207-208` — `isSSSTarget()` / `setSSSTarget()`
- `indra/newview/llviewerobject.h:1082` — `mIsSSSTarget` (default false)
- `indra/newview/llspatialpartition.h:156` — `LLDrawInfo::mIsSSSTarget`
- `indra/newview/llvovolume.cpp:5842` — `draw_info->mIsSSSTarget = vobj->isSSSTarget()` (geometry rebuild 時の伝搬)
- `indra/newview/llvoavatar.cpp:8605` — `attachObject` で `setSSSTargetForAttachment` 呼出
- `indra/newview/pipeline.cpp:486-516` — `addDeferredAttachments`、gbuffer3 を RGBA16F に拡張 (Phase C)
- `indra/newview/pipeline.cpp:5242-5265` — `doSkinSSS()` dispatch site (POOL_ALPHA_POST_WATER 境界)
- `indra/newview/pipeline.cpp:11758-11920` — `doSkinSSS()` 本体 (pass1/pass2)
- `indra/newview/llviewerdisplay.cpp:1085-1095` — `deferredScreen` clear (color=1,0,1,1)
- `indra/newview/llviewerdisplay.cpp:1130` — `renderGeomDeferred` 呼出
- `indra/newview/lldrawpoolsimple.h:134-156` — `LLDrawPoolFullbright` 定義 (post-deferred のみ)
- `indra/newview/lldrawpoolsimple.cpp:156-182` — `LLDrawPoolFullbright::renderPostDeferred`
- `indra/newview/lldrawpoolavatar.cpp:902-909, 945-953` — `aya_sss_skin_flag` を Linden body / eyeballs に push
- `indra/newview/lldrawpoolmaterials.cpp:155-156, 229` — `aya_sss_skin_flag` を legacy material per-batch push
- `indra/newview/lldrawpool.cpp:1090-1098` — GLTF PBR per-draw push
- `indra/newview/llviewercontrol.cpp:1860-1887` — `AYAR20SSSEffective` の View mode 連動更新

### GLSL 側
- `indra/newview/app_settings/shaders/class1/deferred/skinSSSF.glsl` — SSS blur 本体 (全 175 行)
- `indra/newview/app_settings/shaders/class1/deferred/skinSSSV.glsl` — pass-through VS
- `indra/newview/app_settings/shaders/class1/deferred/avatarF.glsl:37,70` — Linden body の gbuffer3.a writer
- `indra/newview/app_settings/shaders/class1/deferred/pbropaqueF.glsl:60,131` — GLTF PBR opaque の gbuffer3.a writer
- `indra/newview/app_settings/shaders/class3/deferred/materialF.glsl:209,450` — legacy material の gbuffer3.a writer
- `indra/newview/app_settings/shaders/class1/deferred/fullbrightF.glsl:28` — FB single-RT 出力 (= gbuffer3.a を書かない原因の一次資料)
- `indra/newview/app_settings/shaders/class1/deferred/skyF.glsl:131`、`cloudsF.glsl:164`、`sunDiscF.glsl:57`、`moonF.glsl:66`、`starsF.glsl:70` — sky/celestial の gbuffer3.a=0 クリア
- `indra/llrender/llshadermgr.h` / `llshadermgr.cpp` — `AYA_SSS_SKIN_FLAG` / `AYA_VISUAL_REALISM_ENABLED` / `AYA_R20_SKIN_SSS_ENABLED` reserved uniform 登録

### Settings / UI
- `indra/newview/app_settings/settings.xml:10121-10181` — `AYAR20AvatarSkinSSS*` cvar 群
- `indra/newview/app_settings/settings.xml:11924-11929` — `RenderEnableEmissiveBuffer` (default 0)
- `indra/newview/skins/default/xui/en/panel_preferences_sss.xml` — Preferences > Graphics > SSS タブ
- `indra/newview/skins/default/xui/en/floater_aya_cinematic.xml:434-464` — Cinematic 用 SSS controls
