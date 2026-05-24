# AYAstorm Glow / Emissive Rendering Routing リファレンス

**作成日**: 2026-05-25
**対象**: deferred + forward を貫通する「emissive 出力 → 別 RT (`mGlow[3]`) → blur → composite」の確定マップ。effect 軸 4 本 (SSS / FullBright / Glow / 環境系) のうち **Glow / Emissive accumulation** 側を地図化する。
**作成経緯**: FullBright prim 越しに SSS pink が漏れる bug 解析を直接の動機として SSS/FullBright/Glow/環境系 4 doc を整備中。Glow 自体は emissive を「**scene buffer の alpha channel**」と「**gbuffer3.rgb**」の 2 系統に書き、後段で `mGlow[3]` の独立 RT を経由して post-tonemap の後に composite される。経路が深く、FullBright 経路と密接に絡むため独立 doc として確定マップ化する。

> **再利用方針**: glow / bloom / 撮影描画の post FX を触る前にまず本書を参照。**Glow を抑止すれば post チェーン (motion blur / DoF / FXAA) も止まる** ような単純構造ではなく、`combineGlow` は post FX (greyscale/sepia/posterize) の合成点でもあるので、Glow を bypass するなら post FX 合成点を別途確保する必要がある。
> shader / RT 変更が `pipeline.cpp:generateGlow / combineGlow / addDeferredAttachments` あたりに入っていたら本書の更新要否を確認すること。

---

## 0. 結論先出し (3 行)

1. **Glow 経路は「scene buffer.a」入力 + 「mGlow[3] 独立 RT」中継 + 「post-tonemap の glowcombineF」出力の三層**。FullBright/Emissive shader は **gbuffer3.rgb (deferred)** と **scene buffer.a (forward / LLDrawPoolGlow)** の 2 か所に emissive を分けて書き、softenLightF が gbuffer3.rgb を scene 加算する一方、scene buffer.a (= 「glow alpha」) は `generateGlow` で extract され `mGlow[3]` に格納される。
2. **mGlow[3] は scene buffer と独立した解像度 (default 256×256)・format (RGBA / RGBA16F) の小型 ping-pong 3 枚で、blur (`glowF`) を kernel × iterations 回ローテートしたあと `combineGlow` で post-tonemap 結果と additive composite する**。「scene 解像度 = blur 解像度」ではないので、解像度を上げても scene quality は変わらず、glow の柔らかさだけが変わる。
3. **合成順序 = `softenLightF (gbuffer3.rgb add)` → `forward alpha (scene.a += emissive)` → `LLDrawPoolGlow::renderPostDeferred (scene.a += legacy glow)` → `tonemap (scene → mPostPingMap)` → `generateGlow (extract → blur)` → `combineGlow (diff + emis)` → 以降の post (motion blur / DoF / FXAA / vignette)`**。
   - つまり **glow blur は LDR tonemap 後の絵をソースに走る** ので、HDR ハイライトを直接 bloom にしているわけではない (sRGB に押し込んだ後の Luma しか入らない)。
   - また **r30 BD post FX (greyscale / sepia / num_colors)** は `glowcombineF.glsl` で同時に乗る (`combineGlow` のついで処理)。glow を全 OFF にしても `combineGlow` 自体は走る (空の mGlow[1] と diff を足すだけになる) ので post FX は維持される — `sRenderGlow=false` 経路で `mGlow[1].clear()` だけして抜ける構造もこの「post FX 合成点を維持する」要請から来ている。

---

## 1. Glow RT 構築 (`pipeline.cpp:createGLBuffers`)

`indra/newview/pipeline.h:993`:

```cpp
LLRenderTarget              mGlow[3];
```

`indra/newview/pipeline.cpp:1745-1754`:

```cpp
// allocate screen space glow buffers
// <FS:AYAstorm r30 BD full port Phase 3.4> Cinematic では BD default (10) に固定
const U32 glow_res = llmax(1, llmin(512, 1 << gSavedSettings.getS32("RenderGlowResolutionPow")));
// </FS:AYAstorm>
const bool glow_hdr = gSavedSettings.getBOOL("RenderGlowHDR");
const U32 glow_color_fmt = glow_hdr ? GL_RGBA16F : GL_RGBA;
for (U32 i = 0; i < 3; i++)
{
    mGlow[i].allocate(512, glow_res, glow_color_fmt);
}
```

### 構造の確定値

| 項目 | 値 | 注 |
|---|---|---|
| 枚数 | **3 枚** (`mGlow[0..2]`) | ping-pong 2 枚 + extract 用 1 枚 |
| 解像度 (default) | **512 × `1 << RenderGlowResolutionPow`** (= 512 × 256 / 8 など) | width 固定 512、height は cvar で可変。`RenderGlowResolutionPow` default は通常 9 (= 512) だが r30 Cinematic では 10 を狙う |
| Format | `GL_RGBA16F` (`RenderGlowHDR`=true) or `GL_RGBA` (false) | `mGlow[3]` は最初から alpha 持ち (gbuffer3 と違って `.a` は real storage) |
| Depth/stencil | **なし** | post pass 用、3D geometry 描画しない |
| 用途 |  | |
| `mGlow[0]` | blur ping-pong 偶数 iter | 横方向 / 縦方向交互 |
| `mGlow[1]` | blur ping-pong 奇数 iter / 最終結果 | `combineGlow` の `emissiveRect` source |
| `mGlow[2]` | extract 結果 | `generateGlow` 最初の pass 出力、blur loop 0 回目入力 |

release は `pipeline.cpp:1633-1636`。

### `RenderGlowResolutionPow` の役割

- shader cvar の `glow_res` (line 9003-9004 の `RenderGlowResolutionPow`) は **blur loop で kernel の delta を 1/glow_res 換算する** ためにも使用 (`pipeline.cpp:9007`)。
- `glowResPow < 9` の時は delta を半分にする (line 9010-9013) ので、低解像度設定でも見た目が大きく崩れない。

---

## 2. Emissive 出力 source (どの shader が emissive を吐くか)

emissive をどこに書くかは **「deferred opaque (= gbuffer3 経由)」** と **「forward / glow pool (= scene buffer.a 経由)」** の 2 系統に分かれる。表化:

### 2.1 Deferred opaque writer (= gbuffer3.rgb / .a 経由)

`grep "frag_data\[3\]"` 結果から emissive RGB を実際に書く writer は **pbropaqueF / pbrterrainF / pbrmetallicroughnessF の 3 種のみ** (legacy material 系は `.rgb=0`)。

| Shader | gbuffer3 への書き込み | Pass / Pool | 流れる先 |
|---|---|---|---|
| `class1/deferred/pbropaqueF.glsl:131` | `vec4(emissive, aya_sss_skin_flag)` | POOL_GLTF_PBR / POOL_GLTF_PBR_ALPHA_MASK | softenLightF PBR 分岐 → `colorEmissive` |
| `class1/deferred/pbrterrainF.glsl:436` | `vec4(mix_emissive, 0)` | POOL_TERRAIN (PBR) | softenLightF PBR 分岐 |
| `class1/gltf/pbrmetallicroughnessF.glsl:249` | `vec4(emissive, 0)` | GLTF scene manager | softenLightF PBR 分岐 |
| `class3/deferred/materialF.glsl:450` | `vec4(0, 0, 0, aya_sss_skin_flag)` | POOL_MATERIALS | **rgb=0 — emissive を gbuffer3 に書かない**。代わりに `frag_data[0].a = emissive` (= `glare`) に乗せる (line 440) |
| `class1/deferred/avatarF.glsl:70` | `vec4(0, 0, 0, aya_sss_skin_flag)` | POOL_AVATAR | rgb=0 |
| その他 (`bumpF` / `diffuseF` / `treeF` / `terrainF` / `impostorF` / `diffuseAlphaMask*F` / sky/celestial 系 / `highlightF`) | `vec4(0)` 系 | 各 deferred Pool | emissive 寄与なし |

詳細: `docs/specs/ayastorm-gbuffer3-trace.md` 参照。**gbuffer3 は r20 で `GL_RGBA16F` に format 変更済** (LL 標準は `GL_RGB16F` で alpha なし) なので `.a` は AYAstorm では skin flag (SSS) と兼用。

### 2.2 Forward / glow pool writer (= scene buffer.a 経由)

scene buffer (`mRT->screen`) の alpha channel に additive で書く path:

| Shader | scene.a 書き込み | Pool / Pass | 経由 dispatcher | Blend |
|---|---|---|---|---|
| `class1/deferred/emissiveF.glsl:37` | `vec4(0, 0, 0, diffuseLookup.a * vertex_color.a)` | POOL_GLOW / PASS_GLOW + PASS_GLOW_RIGGED | `LLDrawPoolGlow::renderPostDeferred` (`lldrawpoolsimple.cpp:45`) | `BT_ADD` + `setColorMask(false, true)` — **alpha だけ加算** |
| `class1/deferred/pbrglowF.glsl:65` | `frag_color.rgb=0; frag_color.a = lum` (emissiveColor × emissiveMap の最大 ch) | POOL_GLTF_PBR / PASS_GLTF_GLOW + PASS_GLTF_GLOW_RIGGED | `LLDrawPoolGLTFPBR::renderPostDeferred` (`lldrawpoolpbropaque.cpp:76`, line 85-92) | `BT_ADD` 相当 + `setColorMask(false, true)` |
| `class2/deferred/alphaF.glsl:317` (forward alpha BLEND) | `vec4(color.rgb, final_alpha)` | POOL_ALPHA | `LLDrawPoolAlpha::renderAlpha` | 通常 alpha blend — **`.a` には diffuse alpha が乗るので emissive ではない** が、それでも `mAYAAlphaColor.a` には溜まる |
| `class1/deferred/fullbrightF.glsl:98` (forward, `IS_ALPHA`) | `vec4(color.rgb, final_alpha)` | POOL_ALPHA / POOL_FULLBRIGHT | forward alpha BLEND | 同上 |
| `class1/deferred/fullbrightShinyF.glsl:93,95` (forward) | `color.a = 1.0; frag_color = color` | POOL_FULLBRIGHT_SHINY (forward) | — | alpha = 1 (= 「完全 fullbright として glow 全載せ」) で writes 後 BT_ADD は使わず、通常 forward blend に乗る |
| `class1/deferred/pbralphaF.glsl` (forward alpha BLEND PBR) | `frag_color` (emissive 加算済 color) | POOL_ALPHA | forward alpha BLEND | emissive を **add 経由で別 draw** (= `renderPbrEmissives` / `renderRiggedPbrEmissives`) する。`lldrawpoolalpha.cpp:170` で `pbr_emissive_shader = &gPBRGlowProgram` |

### 2.3 「FullBright が glow を強める」の意味

FullBright surface は **lit を skip** して srgb texture をほぼそのまま scene に出すので、`fullbrightShinyF.glsl:93` で `color.a = 1.0` を立てると scene.a が 1 に近づく → `generateGlow` extract の **luminance gate (`minLuminance`) を簡単に超える** → glow にフル参加する。普通の lit 物体は `softenLightF.glsl:320` で `frag_color.a = 0.0` に明示クリアされるので、glow 寄与は gbuffer3.rgb 経由でしか乗らない。これが「**FullBright 越しに glow が乗りやすい**」観測の構造的根拠。

---

## 3. Glow extract pass (`generateGlow` / `glowExtractF.glsl`)

呼び出し: `pipeline.cpp:10113` (`renderFinalize` の中、tonemap 直後)。

```cpp
generateGlow(&mPostPingMap);   // mPostPingMap = tonemap 後の scene
```

`pipeline.cpp:8947-9058` の本体:

| ステップ | 動作 | 対象 RT |
|---|---|---|
| 1 | `mGlow[2].bindTarget(); mGlow[2].clear();` | extract dst |
| 2 | `gGlowExtractProgram.bind()` + uniform set (`minLuminance` / `maxExtractAlpha` / `lumWeights` / `warmthWeights` / `warmthAmount`) | `effects/glowExtractF.glsl` |
| 3 | `mPostPingMap` (= post-tonemap scene) を `DIFFUSE_MAP` に bind | scene src |
| 4 | `BT_ADD_WITH_ALPHA` で fullscreen triangle 描画 | mGlow[2] に extract |
| 5 | `mGlow[2].flush()` | |

### `glowExtractF.glsl` の処理 (`indra/newview/app_settings/shaders/class1/effects/glowExtractF.glsl:47-67`):

```glsl
vec4 col = texture(diffuseMap, vary_texcoord0.xy);
float lum     = smoothstep(minLuminance, minLuminance+1.0, dot(col.rgb, lumWeights));
float warmth  = smoothstep(minLuminance, minLuminance+1.0,
                           max(col.r*warmthWeights.r,
                               max(col.g*warmthWeights.g, col.b*warmthWeights.b)));
frag_color.rgb = col.rgb;
frag_color.a   = max(col.a, mix(lum, warmth, warmthAmount) * maxExtractAlpha);
```

つまり mGlow[2] には:
- `.rgb` = scene の RGB (そのまま)
- `.a` = `max(scene.a, mix(lum, warmth, warmthAmount) * maxExtractAlpha)` = 「forward 経路で書かれた glow alpha」と「scene RGB の luminance/warmth から計算した派生 glow alpha」の合成

が入る。

### AYAstorm 固有改変 (r30 P4)

`pipeline.cpp:8961-8963`:
```cpp
// <FS:AYAstorm r30 P4> Honor RenderGlowMinLuminance instead of hardcoded gate.
gGlowExtractProgram.uniform1f(LLShaderMgr::GLOW_MIN_LUMINANCE, RenderGlowMinLuminance);
```
本家は `minLuminance` を hardcode していたが r30 P4 で `RenderGlowMinLuminance` cvar 経由に変更。

### Noise (banding 緩和)

`RenderGlowNoise=true` の時 (`pipeline.cpp:8971-8982`)、`glowNoiseMap` (= 128×128 noise texture) を bind して shader の `HAS_NOISE` 経路で extract 結果に dithering を加える。`maxExtractAlpha` を下げた時の精度損で出る banding を緩和する目的。

---

## 4. Blur pass (`glowF.glsl` / `glowV.glsl`)

`pipeline.cpp:9006-9046`:

```cpp
S32 kernel = RenderGlowIterations * 2;      // r30 BD default: 5*2 = 10 回
F32 delta  = RenderGlowWidth / glow_res;    // r30 BD: 3.6 / 256 など
if (glowResPow < 9) delta *= 0.5f;
F32 strength = RenderGlowStrength;          // r30 BD: 0.233

gGlowProgram.bind();
gGlowProgram.uniform1f(GLOW_STRENGTH, strength);

for (S32 i = 0; i < kernel; i++)
{
    mGlow[i % 2].bindTarget();
    mGlow[i % 2].clear();
    if (i == 0)  bindTexture(DIFFUSE_MAP, &mGlow[2]);       // extract 結果
    else         bindTexture(DIFFUSE_MAP, &mGlow[(i-1)%2]); // 前の iter

    if (i % 2 == 0) uniform2f(GLOW_DELTA, delta, 0);   // 横方向
    else            uniform2f(GLOW_DELTA, 0, delta);   // 縦方向

    drawArrays(TRIANGLES, 0, 3);
    mGlow[i % 2].flush();
}
```

### `glowV.glsl` (`indra/newview/app_settings/shaders/class1/effects/glowV.glsl:43-51`):

8 tap separable kernel の texcoord を `glowDelta * {-3.5, -2.5, -1.5, -0.5, 0.5, 1.5, 2.5, 3.5}` で生成。

### `glowF.glsl` (`indra/newview/app_settings/shaders/class1/effects/glowF.glsl:41-55`):

```glsl
float kern[8];
kern[0]=0.25; kern[1]=0.5; kern[2]=0.8; kern[3]=1.0;
kern[4]=1.0;  kern[5]=0.8; kern[6]=0.5; kern[7]=0.25;
// 8 tap weighted sum → col
frag_color = max(vec4(col.rgb * glowStrength, col.a), vec4(0));
```

8 tap × (`kernel = 2 * iterations`) 回。`RenderGlowIterations=5` なら **calc 10 pass、横→縦→横→縦… alternate** で 80 sample 相当の Gaussian-like blur。`mGlow[3]` の最終結果は **`mGlow[1]`** に残る (奇数 iter 終わり)。

### `!sRenderGlow` 経路 (`pipeline.cpp:9051-9057`)

```cpp
else // !sRenderGlow, skip the glow ping-pong and just clear the result target
{
    mGlow[1].bindTarget();
    glClearColor(0.f, 0.f, 0.f, 0.f);
    mGlow[1].clear(GL_COLOR_BUFFER_BIT);
    mGlow[1].flush();
}
```

→ glow OFF でも `mGlow[1]` は 0 でクリアされ、後段 `combineGlow` は走り続ける。post FX (greyscale/sepia/posterize) の合成点として `combineGlow` が必要なので bypass しない構造。

---

## 5. Composite pass (`combineGlow` / `glowcombineF.glsl`)

呼び出し: `pipeline.cpp:10136`:
```cpp
combineGlow(sourceBuffer, targetBuffer);   // sourceBuffer = tonemap 後の scene、targetBuffer = post 用 ping-pong
std::swap(sourceBuffer, targetBuffer);
```

### `pipeline.cpp:9472-9507` 本体

```cpp
void LLPipeline::combineGlow(LLRenderTarget* src, LLRenderTarget* dst)
{
    dst->bindTarget();
    gGlowCombineProgram.bind();
    gGlowCombineProgram.bindTexture(DEFERRED_DIFFUSE,   src);
    gGlowCombineProgram.bindTexture(DEFERRED_EMISSIVE, &mGlow[1]);

    // <FS:AYAstorm:r30-bd-port> Phase 6 step 3: BD post FX (Cinematic only).
    if (isCinematicMode())
    {
        uniform1f(GREYSCALE_STRENGTH, RenderGreyscaleStrength);
        uniform1f(SEPIA_STRENGTH,     RenderSepiaStrength);
        uniform1f(NUM_COLORS,         (GLfloat)RenderNumColors);
    }
    else
    {
        uniform1f(GREYSCALE_STRENGTH, 0.0f);
        uniform1f(SEPIA_STRENGTH,     0.0f);
        uniform1f(NUM_COLORS,         1.0f);
    }

    drawArrays(TRIANGLES, 0, 3);
    dst->flush();
}
```

### `glowcombineF.glsl` (`class1/interface/glowcombineF.glsl:39-63`)

```glsl
vec4 diff = texture(diffuseRect, tc);
vec4 emis = texture(emissiveRect, tc);

diff = diff + emis;                 // ← 単純 additive composite

if (num_colors > 2)
{
    diff.rgb = pow(diff.rgb, vec3(0.6));
    diff.rgb = floor(diff.rgb * num_colors) / num_colors;
    diff.rgb = pow(diff.rgb, vec3(1.0/0.6));
}
diff.rgb = mix(diff.rgb, vec3(luma(diff)),   greyscale_str);
diff.rgb = mix(diff.rgb, sepia_matrix*diff,  sepia_str);

frag_color = diff;
```

合成は **`diff + emis` (= ONE/ONE additive)**。screen blend や softlight ではない。よって glow は **LDR tonemap 後の絵に対して輝度を足し増し** する操作で、HDR halo を直接表現しているわけではない。

### Shader file binding

`indra/newview/llviewershadermgr.cpp:3699-3710`:
```cpp
gGlowCombineProgram.mShaderFiles.push_back(make_pair("interface/glowcombineV.glsl", GL_VERTEX_SHADER));
gGlowCombineProgram.mShaderFiles.push_back(make_pair("interface/glowcombineF.glsl", GL_FRAGMENT_SHADER));
```

`interface/` 配下にあるので class1 のみ (`class2`/`class3` バリアントなし)。

---

## 6. PBR vs non-PBR 分岐 (経路の差)

| 観点 | non-PBR (legacy) | GLTF PBR |
|---|---|---|
| Glow 値の根 | prim 「Glow」スライダー (`te->getGlow()` `LLTextureEntry`) | GLTF material の emissive (`emissiveColor` × `emissiveMap`) |
| 登録 path | `llvovolume.cpp:7273-7282` `!is_alpha && sRenderGlow && getGlow() > 0` 条件で `PASS_GLOW` 登録 | 同条件 + `gltf_mat` で `PASS_GLTF_GLOW` 登録 |
| Pool | POOL_GLOW (`LLDrawPoolGlow`、`lldrawpool.h:196`) | POOL_GLTF_PBR / POOL_GLTF_PBR_ALPHA_MASK 内の post-deferred 第 2 pass |
| dispatcher | `LLDrawPoolGlow::renderPostDeferred` (`lldrawpoolsimple.cpp:45-71`) | `LLDrawPoolGLTFPBR::renderPostDeferred` (`lldrawpoolpbropaque.cpp:76-94`) |
| shader | `gDeferredEmissiveProgram` (`emissiveF.glsl`) | `gPBRGlowProgram` (`pbrglowF.glsl`) |
| 書き先 | scene.a (= mRT->screen alpha channel)、`setColorMask(false, true)` | 同上 |
| Forward alpha BLEND の emissive | `renderEmissives` (`lldrawpoolalpha.cpp:686`) で `gDeferredEmissiveProgram` を再利用 | `renderPbrEmissives` (`lldrawpoolalpha.cpp:699`) で `gPBRGlowProgram` を再利用 |
| gbuffer3.rgb への emissive | 書かない (`materialF` は `frag_data[0].a = emissive` という legacy 別 channel) | `pbropaqueF:131` で書く |

### softenLightF 側の差

- PBR 分岐 (`softenLightF.glsl:213`): `pbrBaseLight(..., colorEmissive, ...)` で `colorEmissive` (= gbuffer3.rgb) を加算 (`deferredUtil.glsl:620` `color += colorEmissive`)
- Legacy 分岐 (`softenLightF.glsl:234-312`): `colorEmissive` を **使わない** (PBR HDRI / SKIP_ATMOS 分岐のみ使う)。よって legacy material の emissive は softenLightF の scene 加算には乗らず、`renderPostDeferred` の `LLDrawPoolGlow` 経由で scene.a に書かれて glow blur で「halo」として再注入されるだけ。

---

## 7. FullBright との接合点

FullBright/Emissive 関連 path の **scene 出力先と blend** をまとめる。FullBright doc (`docs/specs/ayastorm-fullbright-rendering-routing.md`) と冗長しても Glow 視点で書く:

| Shader | Pool | dispatcher | 書き先 | scene.a (= glow seed) への影響 |
|---|---|---|---|---|
| `fullbrightF.glsl` (non-ALPHA, deferred opaque) | POOL_FULLBRIGHT | `LLDrawPoolFullbright::renderPostDeferred` | scene RGB (forward) | `final_alpha = color.a * vertex_color.a` がそのまま scene.a に乗る (line 81, 98)。texture alpha 1 = `color.a` 1 = glow seed 1 |
| `fullbrightF.glsl` (`IS_ALPHA`, forward alpha BLEND) | POOL_ALPHA | `LLDrawPoolAlpha::renderAlpha` | scene RGB (forward) | 同上、blend で減衰 |
| `fullbrightShinyF.glsl:93` (class3 forward) | POOL_FULLBRIGHT_SHINY / POOL_ALPHA | renderPostDeferred / renderAlpha | `color.a = 1.0` を hardcode | **scene.a が必ず 1 になる** → glow seed 飽和、「fullbright shiny は常に光って見える」原因 |
| `emissiveF.glsl` (PASS_GLOW) | POOL_GLOW | `LLDrawPoolGlow::renderPostDeferred` | scene.a only (`setColorMask(false, true)`) | RGB 0 / A = diffuse.a × vertex.a を BT_ADD で書く。「prim Glow スライダー」の glow 注入点 |
| `pbrglowF.glsl` (PASS_GLTF_GLOW) | POOL_GLTF_PBR | `LLDrawPoolGLTFPBR::renderPostDeferred` | scene.a only | RGB 0 / A = max(emissive.rgb) × vertex.a。「GLTF emissiveTexture」の glow 注入点 |
| `pbropaqueF.glsl:131` (deferred opaque) | POOL_GLTF_PBR | `LLDrawPoolGLTFPBR::renderDeferred` | gbuffer3.rgb (emissive) + gbuffer3.a (skin flag) | softenLightF PBR 分岐で colorEmissive として **scene.RGB に加算** される (scene.a には乗らない) |
| `materialF.glsl:440` (deferred opaque) | POOL_MATERIALS | `LLDrawPoolMaterials::render` | `frag_data[0].a = emissive` (= glare) | gbuffer0.a に乗るが softenLightF はこれを **読まない**。実際の glow 寄与は PASS_GLOW (= 上の `emissiveF.glsl`) 経由で別途 |

### **観測される現象との対応**

- 「FullBright prim の上を SSS pink (skin SSS overlay) が漏れる」 → SSS doc 参照だが、Glow 側から見ると **FullBright の `color.a` write (= scene.a)** が SSS post pass の `mask` read と取り違えられている可能性がある (gbuffer3.a / scene.a / mAYAAlphaColor.a の **3 つの "alpha" の混同**)。Glow seed としての scene.a は gate されず常時加算されるので、FullBright を貼ると glow 値も上がるが、SSS canary との直接因果は別経路の可能性が高い。詳細解析は SSS doc 側で続ける。
- 「PBR emissive と prim Glow が二重に光る」 → GLTF PBR の emissive は **gbuffer3.rgb (softenLightF 経由) と PASS_GLTF_GLOW (scene.a 経由) の両方** に書かれる。GLTF emissiveTexture を強くした上で prim Glow を立てると **二重カウント**になる。これは LL 標準仕様、AYAstorm 改変なし。

---

## 8. gbuffer3 emissive bit (gbuffer3 channel 利用)

詳細: `docs/specs/ayastorm-gbuffer3-trace.md`。Glow との接続点だけ抜粋:

| Channel | 内容 | Writer | Reader |
|---|---|---|---|
| `gbuffer3.rgb` | linear emissive (PBR 専用) | `pbropaqueF` / `pbrterrainF` / `pbrmetallicroughnessF` | `softenLightF.glsl:152` (`colorEmissive = gb.emissive.rgb`)、`pointLightF.glsl:100` / `multiPointLightF.glsl:93` (local light の self-illumination)、`generateLuminance` (`pipeline.cpp:8644` で `mGlow[1]` を `DEFERRED_EMISSIVE` に bind するが、これは別話 — Luma 計算用) |
| `gbuffer3.a` | AYAstorm r20: `aya_sss_skin_flag` (SSS mask) | `materialF` / `pbropaqueF` / `avatarF` (= skin writers) | `class3/deferred/skinSSSF.glsl` (`docs/ayastorm-sss-rendering-routing.md` 参照) |

### `mGlow[1]` と `DEFERRED_EMISSIVE` の name collision 注意

`pipeline.cpp:8644` (generateLuminance) と `pipeline.cpp:9484` (combineGlow) の両方で `mGlow[1]` を **`DEFERRED_EMISSIVE` という uniform 名で** bind しているが、これは「**glow blurred の最終結果**」であって「**gbuffer3 = emissiveRect**」ではない。同じ uniform slot 名を別 RT に流用しているだけで、storage は別。

- generateLuminance 内: `mGlow[1]` (glow blur) を **`emissiveRect`** として読む → auto-exposure 計算に glow を反映する目的
- combineGlow 内: `mGlow[1]` を **`emissiveRect`** として読む → diff + emis additive

混同しないこと: `softenLightF` の `emissiveRect` は **gbuffer3 (= `mRT->deferredScreen` の color attachment 3)** であって `mGlow[1]` ではない。

---

## 9. AYAstorm 固有改変 (r30 周辺の Glow / Bloom)

### r30 P4: `RenderGlowMinLuminance` cvar 化

`pipeline.cpp:8961-8963`. 元は hardcode された minLuminance 閾値を cvar で可変に。撮影描画 (Cinematic) で luminance gate を下げて soft glow を狙うのが主用途。

### r30 BD full port Phase 3.4: `RenderGlowResolutionPow` を Cinematic で BD default (10) に固定

`pipeline.cpp:1746` のコメント参照。BD parity 検証中の cvar fixation。

### r30 BD port Phase 6 step 3: `combineGlow` で post FX (greyscale / sepia / posterize) を統合

`pipeline.cpp:9486-9500`. **Cinematic mode のみ** で `RenderPostGreyscaleStrength` / `RenderPostSepiaStrength` / `RenderPostPosterizationSamples` を `glowcombineF.glsl` に渡す。glow を OFF にしても post FX が乗るので、glow と post FX の switch を分離して扱える。
- mode 0/1 (Firestorm/AYAstorm View) では `0/0/1` を渡す (= no-op)。

### r30 P3 step 4: Volumetric Lighting (godrays) の挿入位置

`pipeline.cpp:10118-10133`. **`generateGlow` の後・`combineGlow` の前** に挟まる別 post pass。Cinematic 専用、AY r15 lineage (`doGodrays`)。godrays は `mRT->screen` の atmospherics pass 段階 (`pipeline.cpp:5258`) でも別に乗るので二段構成。glow 経路とは別 RT 系統 (godrays は scene buffer に直接 additive)。

### r30 P5 transparent-DoF C-(a): `mAYAAlphaColor` 経由の alpha-only RT

`pipeline.cpp:10043-10082`. **glow とは独立した別 RT**。glow への副作用は line 10052-10058 のコメントに記録: alpha BLEND を `mAYAAlphaColor` に redirect する時、scene.a (= glow seed) を `(1 - plate.a)` で減衰させて LMB-on-HUD 経路との parity を取る。**これにより透過装着物の glow halo が「下に何があるかで変わる」挙動になる** ので、撮影描画で glow の見た目が異なる時はここを疑う。

### r20: gbuffer3 format を `GL_RGBA16F` に変更

`pipeline.cpp:394` (`addDeferredAttachments`)。gbuffer3 の `.a` を SSS skin flag に使うため LL 標準の `GL_RGB16F` を拡張。Glow 経路への影響は **storage が `.a` を持つようになった** こと自体だが、emissiveRect.a を「emissive 強度」として読む reader は存在しない (skin flag 専用)。

---

## 10. 経路まとめ図 (frame 1 回分の縦軸)

```
[1] deferred opaque draw
   ├── pbropaqueF.glsl       → gbuffer3.rgb = emissive
   ├── pbrterrainF.glsl      → gbuffer3.rgb = mix_emissive
   ├── materialF.glsl        → frag_data[0].a = emissive (= glare、softenLightF は読まない)
   └── その他 deferred writer → gbuffer3 = 0

[2] softenLightF (renderDeferredLighting)
   └── gbuffer3.rgb (PBR 分岐のみ) を scene.rgb に加算、frag_color.a=0 で scene.a クリア

[3] renderGeomPostDeferred — 順序順に scene へ書く
   ├── LLDrawPoolGlow::renderPostDeferred
   │     └── emissiveF.glsl    → scene.a += diff.a * vertex.a (BT_ADD, mask false/true)
   ├── LLDrawPoolGLTFPBR::renderPostDeferred (glow pass)
   │     └── pbrglowF.glsl     → scene.a += lum(emissive) (BT_ADD, mask false/true)
   ├── LLDrawPoolAlpha::renderAlpha
   │     ├── 通常 alpha BLEND  → scene.rgba (alpha BLEND、scene.a も書かれる)
   │     ├── renderEmissives    → emissiveF.glsl (= 上と同じ shader)
   │     └── renderPbrEmissives → pbrglowF.glsl
   └── (fullbright forward, fullbright_shiny forward 等)

[4] tonemap (pipeline.cpp:10098)
   └── mRT->screen (HDR) → mPostPingMap (LDR, sRGB)

[5] generateGlow (pipeline.cpp:10113)
   ├── mGlow[2] = glowExtractF(mPostPingMap)  // luminance/warmth gate
   └── for i in [0..kernel): mGlow[i%2] = glowF(prev)  // separable blur

[6] (optional) renderVolumetric (godrays、Cinematic only)
   └── mPostPingMap → mPostPongMap

[7] combineGlow (pipeline.cpp:10136)
   └── targetBuffer = diff(src) + emis(mGlow[1]) + post FX
       (greyscale / sepia / posterize は Cinematic only)

[8] (optional) renderMotionBlurComposite、renderDoF、applyFXAA、renderVignette...
```

---

## 11. 関連 cvar 一覧

| cvar | default (本家) | r30 Cinematic 上書き値 (`settings_cinematic_bd.xml:62-92`) | 役割 |
|---|---|---|---|
| `RenderGlow` | true | (BD 値で同梱) | sRenderGlow master switch (`llviewershadermgr.cpp:649`) |
| `RenderGlowResolutionPow` | 9 | 10 (`pipeline.cpp:1746` のコメント) | mGlow[] height = `1 << pow` |
| `RenderGlowHDR` | true | — | mGlow[] format (`GL_RGBA16F` vs `GL_RGBA`) |
| `RenderGlowIterations` | — | **5** (= 10 blur pass) | blur loop 回数 |
| `RenderGlowWidth` | — | **3.6** | blur kernel spacing |
| `RenderGlowStrength` | — | **0.233** | per-tap strength |
| `RenderGlowWarmthAmount` | — | **16.0** | extract で luminance vs warmth 比率 |
| `RenderGlowMaxExtractAlpha` | — | **0.03** | extract の output `.a` 上限 |
| `RenderGlowMinLuminance` | — | **0.0** (r30 P4 で cvar 化) | extract luminance gate (smoothstep の下端) |
| `RenderGlowLumWeights` | — | **(0.4, 0.3, 0.3)** | lum 計算重み (RGB) |
| `RenderGlowWarmthWeights` | — | **(0.75, 0.6, 0.712)** | warmth 計算重み (RGB) |
| `RenderGlowNoise` | (false) | — | dither ON で banding 緩和 |
| `RenderPostGreyscaleStrength` | 0 | — (Cinematic UI で可変) | `combineGlow` の greyscale 強度 (Cinematic only) |
| `RenderPostSepiaStrength` | 0 | — | `combineGlow` の sepia 強度 |
| `RenderPostPosterizationSamples` | 1 | — | `combineGlow` の posterize 段階数 |

---

## 12. 落とし穴 / 注意

1. **`emissiveRect` という uniform 名は 2 つの RT に流用される**: softenLightF の `emissiveRect` = `gbuffer3 (mRT->deferredScreen の attachment 3)`、`combineGlow` / `generateLuminance` の `emissiveRect` = `mGlow[1]` (blur 結果)。同名なので shader だけ見ると同じ RT に見えるが別物。
2. **`glow_res` cvar は scene 解像度ではない**: `mGlow[]` は scene よりずっと低解像度 (default 512×256)。glow 解像度を上げても scene 描画コストは増えないが、blur 自体は scene-independent なので halo の柔らかさだけ変わる。
3. **`!sRenderGlow` 経路でも `combineGlow` は走る**: post FX (greyscale / sepia / posterize) の唯一の合成点なので、glow を OFF にしても `mGlow[1]` を 0 で clear して `combineGlow` 自体は実行する。glow を完全 bypass する改変を入れる時はここを切ってはいけない。
4. **`fullbrightShinyF.glsl:93` の `color.a = 1.0`** は **scene.a を強制的に 1 にする**。これにより fullbright_shiny は extract で必ず glow seed として残る。`alphaF.glsl` の `color.a = final_alpha` (= diffuse.a × vertex.a) と blend ルートが違うので、両者の glow 寄与は構造的に非対称。
5. **legacy material (`materialF.glsl`) の emissive は softenLightF を経由しない**: `frag_data[0].a = emissive` で gbuffer0.a に書くが、softenLightF はこれを読まない。実際の glow 寄与は別 pool (POOL_GLOW) で `te->getGlow()` を `emissiveF` 経由で scene.a に書くだけ。legacy emissive を「surface 加算」したいなら別経路必要。
6. **PBR emissive と prim Glow は二重カウントの可能性**: GLTF emissiveTexture (`pbropaqueF.glsl:131` で gbuffer3.rgb → softenLightF で scene.rgb 加算) と PASS_GLTF_GLOW (`pbrglowF.glsl` で scene.a 加算 → glow blur) は **両方発火する**。emissive を強くした上で `te->getGlow()` を立てると glow が想定外に強く見える。
7. **glow blur source は LDR tonemap 後**: `generateGlow(&mPostPingMap)` の `mPostPingMap` は tonemap 結果。HDR ハイライトを直接 bloom にしているわけではないので、tonemap で sRGB に押し込まれた値を再度 luminance gate にかけている。物理的な HDR bloom が欲しい場合は tonemap 前で extract する別経路が必要。
8. **`combineGlow` の合成は単純 `diff + emis` の additive**: screen blend / soft light ではないので、emis を強くすると簡単に sRGB clip する。`RenderGlowMaxExtractAlpha` (default 0.03) で extract 段階で抑えているのはこのため。

---

## 13. canary 設置パターン (再現用)

shader path を実機で確認したい時は:

- **`glowExtractF.glsl`** に `frag_color.rgb = vec3(1, 0, 1); frag_color.a = 1.0;` 等を書けば extract 直後の `mGlow[2]` を可視化 (scene 上は post-glow composite で見える)
- **`glowF.glsl`** で `frag_color = vec4(0, 1, 0, 1);` にすれば blur loop を可視化
- **`glowcombineF.glsl`** で `frag_color = emis;` だけ出すと glow only 表示 (diff が消えて halo だけ残る)、`frag_color = diff;` で逆。glow 寄与の比率を見たい時に便利

deploy 先 (`project_shader_install_path.md`):
- `~/ayastorm/app_settings/shaders/class1/effects/glowExtractF.glsl` 等
- `~/ayastorm/app_settings/shaders/class1/interface/glowcombineF.glsl`

cache clear (`project_ayastorm_shader_cache_path.md`):
- `rm -rf ~/.ayastorm_x64/cache/shader_cache/`

色割り当て推奨 (`feedback_diag_canary_design.md`):
- glow extract → マゼンタ (1, 0, 1)
- glow blur   → 緑 (0, 1, 0)
- glow combine → シアン (0, 1, 1)

---

## 14. 関連ファイル

### C++ (pipeline / pool)
- `indra/newview/pipeline.h:993` — `LLRenderTarget mGlow[3]` 宣言
- `indra/newview/pipeline.cpp:1745-1754` — `mGlow[]` allocate
- `indra/newview/pipeline.cpp:1633-1636` — `mGlow[]` release
- `indra/newview/pipeline.cpp:8947-9058` — `LLPipeline::generateGlow`
- `indra/newview/pipeline.cpp:9472-9507` — `LLPipeline::combineGlow`
- `indra/newview/pipeline.cpp:10113` — `generateGlow` 呼び出し点 (`renderFinalize`)
- `indra/newview/pipeline.cpp:10136` — `combineGlow` 呼び出し点
- `indra/newview/lldrawpoolsimple.cpp:45-71` — `LLDrawPoolGlow::renderPostDeferred`
- `indra/newview/lldrawpoolpbropaque.cpp:76-94` — `LLDrawPoolGLTFPBR::renderPostDeferred` (PBR glow pass)
- `indra/newview/lldrawpoolalpha.cpp:167-172` — alpha pool の emissive_shader / pbr_emissive_shader prep
- `indra/newview/lldrawpoolalpha.cpp:686-758` — alpha pool の emissive dispatcher 群
- `indra/newview/llvovolume.cpp:7273-7282` — `PASS_GLOW` / `PASS_GLTF_GLOW` の face 登録条件

### Shader (effects / interface)
- `indra/newview/app_settings/shaders/class1/effects/glowExtractV.glsl` — extract vertex (fullscreen triangle)
- `indra/newview/app_settings/shaders/class1/effects/glowExtractF.glsl` — luminance/warmth gate
- `indra/newview/app_settings/shaders/class1/effects/glowV.glsl` — 8 tap separable kernel texcoord
- `indra/newview/app_settings/shaders/class1/effects/glowF.glsl` — 8 tap blur
- `indra/newview/app_settings/shaders/class1/interface/glowcombineV.glsl` — composite vertex
- `indra/newview/app_settings/shaders/class1/interface/glowcombineF.glsl` — composite + post FX

### Shader (emissive writer / glow seed)
- `indra/newview/app_settings/shaders/class1/deferred/emissiveV.glsl` / `emissiveF.glsl` — PASS_GLOW (legacy prim Glow スライダー)
- `indra/newview/app_settings/shaders/class1/deferred/pbrglowV.glsl` / `pbrglowF.glsl` — PASS_GLTF_GLOW (GLTF emissive)
- `indra/newview/app_settings/shaders/class1/deferred/pbropaqueF.glsl:131` — gbuffer3.rgb emissive write (PBR opaque)
- `indra/newview/app_settings/shaders/class1/deferred/pbrterrainF.glsl:436` — gbuffer3.rgb emissive write (PBR terrain)
- `indra/newview/app_settings/shaders/class3/deferred/materialF.glsl:440` — `frag_data[0].a = emissive` (legacy 用、softenLightF は読まない)
- `indra/newview/app_settings/shaders/class1/deferred/fullbrightF.glsl` — fullbright forward (scene.a = final_alpha)
- `indra/newview/app_settings/shaders/class3/deferred/fullbrightShinyF.glsl:93` — `color.a = 1.0` hardcode (scene.a 飽和点)

### Settings
- `indra/newview/app_settings/settings.xml` — RenderGlow* cvar default 群
- `indra/newview/app_settings/settings_cinematic_bd.xml:62-92` — r30 Cinematic 上書き値 (BD parity)

### 姉妹資料 (effect 軸 4 本)
- `docs/specs/ayastorm-deferred-shader-routing.md` — object class 軸の routing 全体
- `docs/specs/ayastorm-gbuffer3-trace.md` — gbuffer3 storage (emissiveRect) 仕様
- `docs/specs/ayastorm-fullbright-rendering-routing.md` — FullBright effect 軸
- `docs/ayastorm-sss-rendering-routing.md` — SSS effect 軸

---

## 15. 推測ベース / 未確認の項目 (本書の弱点)

明示しておく:

1. **`mGlow[3]` の `.a` channel の正確な物理利用**: `GL_RGBA / GL_RGBA16F` で alpha は real storage だが、`glowExtractF` で書き、`glowF` blur で kernel 重み付けして加算しているだけで、`glowcombineF` 側では `diff + emis` の vec4 加算でしか使われていない。出力 frag_color.a は post chain 後段でほぼ無視される (FXAA / vignette は diff.rgb のみ使う) ので、glow alpha は **生成と blur のためだけに存在し、最終出力ではほぼ使われない** 可能性が高い。明示確認はしていない。
2. **`mGlow[1]` を `DEFERRED_EMISSIVE` slot で luminance 計算に bind する意図**: `generateLuminance` (`pipeline.cpp:8641-8645`) で `mGlow[1]` を読んでいるが、これが auto-exposure に glow を反映する意図か、Linden の uniform slot 再利用に過ぎないかは LL 側のコメントが薄く、本書では「auto-exposure に glow を反映する目的」と推定で書いた。
3. **AYAstorm r30 Cinematic の glow tuning 値の根拠**: `settings_cinematic_bd.xml` の値は BD lineage と memo されているが、各値の最適性は本書では検証していない。tuning は別 phase 扱い (`project_r30_cinematic_control_tuning_deferred.md`)。
4. **FullBright → SSS pink 漏れ bug の Glow 経路寄与**: §7 で構造的可能性は示したが、実機で「Glow を完全 OFF にすると SSS pink 漏れが消えるか」の実証はしていない。SSS doc 側で別途確認が必要。

---

## 16. 更新履歴

- 2026-05-25 初版: effect 軸 4 本シリーズの 1 本として作成。`pipeline.cpp` / `lldrawpoolsimple.cpp` / `lldrawpoolpbropaque.cpp` / `lldrawpoolalpha.cpp` / `llvovolume.cpp` と各 emissive / glow / composite shader を上から下まで trace。SSS doc / FullBright doc / gbuffer3-trace / deferred-shader-routing と相互参照可能な粒度で確定マップ化
