# AYAstorm FullBright Rendering Routing リファレンス

**作成日**: 2026-05-25 (r31 WBOIT 着手前に「FB prim 越しに SSS pink が透ける」 bug の構造解析として作成)
**対象**: FullBright effect で描画される全 path (pool / shader / depth / gbuffer writer) の確定マップ。「FB はどう描かれているか」「FB は gbuffer3.a (SSS skin flag) をどう触るか」を即座に引けるようにするためのリファレンス。
**作成経緯**: SSS pass で「FB prim の後ろにいる avatar の SSS pink がアバター形状で FB prim 表面に透けて見える」 bug 解析。sky で同様の bug は短時間で潰れたが、SSS は 3-4h × 2 回外したので、推論を止めて FB の全経路を実コード trace で確定させる目的。

> **再利用方針**: FullBright / Emissive / Glow 関連で挙動が壊れたとき、または SSS / glow / DoF など post-pass が「FB を特殊扱い」しない結果として副作用を引き起こしたときに **まず本書を参照**。FullBright shader 自体は単純だが、`POOL_FULLBRIGHT` / `POOL_FULLBRIGHT_ALPHA_MASK` / `POOL_ALPHA` 内の FB / `POOL_MATERIALS` の emissive=1 face / `POOL_GLTF_PBR` の unlit material と **5 系統に分岐** していて、bug 仮説で 1 系統だけ見ると外す。

---

## TL;DR — FB の core 問題は「gbuffer3.a を一切触らない」

**`POOL_FULLBRIGHT` / `POOL_FULLBRIGHT_ALPHA_MASK` の標準経路 (`fullbrightF.glsl`) は `out vec4 frag_color;` 単一 RT 出力で、`frag_data[0..3]` には何も書かない**。すなわち FB pixel の覆う screen 領域では、**gbuffer3 (DEFERRED_EMISSIVE / `.a`= SSS skin flag) の値はその pixel に *元々* 書いていた deferred 物体の値がそのまま残る**。FB prim の手前に opaque な avatar / skin material が gbuffer 段で `aya_sss_skin_flag = 1.0` を書いていれば、FB が forward で scene color を上書きしても skin flag は 1.0 のまま → SSS pass はその pixel を skin と判定 → FB の color を入力に skin SSS blur をかける → アバター形状で pink が滲み出る。

depth 側の挙動も裏付けになる: `LLDrawPoolFullbright::renderPostDeferred` は `LLGLDepthTest` を明示せず caller の継承 (deferred pass の `GL_LEQUAL` / depth write ON) で走るので、**FB は depth を書く**。よって SSS pass の `eye_dist` 復元 (skinSSSF.glsl:103 `texture(depthMap, tc).r`) は「ここに FB がある」と認識できる。しかし mask 側 (gbuffer3.a) は「ここに skin がある」と認識した *まま* なので、両者が矛盾し SSS blur が走る。**fix の方向は FB writer 群にも skin flag = 0 の gbuffer3.a write を追加すること** (`materialF` / `pbropaqueF` / `avatarF` と同じ pattern を `fullbrightF` / `fullbrightShinyF` にも入れる)。

---

## 0. クイック分岐表 — 「これは FB のどれ?」

| 観測条件 | Pool | Shader | gbuffer3.a に書くか | bug 影響 |
|---|---|---|---|---|
| FB texture かつ alpha 無し / opaque | `POOL_FULLBRIGHT` | `fullbrightF.glsl` (no permutation) | **NO** | **SSS bleed の主犯** |
| FB texture かつ alpha MASK (discard) | `POOL_FULLBRIGHT_ALPHA_MASK` | `fullbrightF.glsl` (`HAS_ALPHA_MASK`) | **NO** | SSS bleed する (discard 後の残 fragment は depth/color 書く) |
| FB texture かつ alpha BLEND (半透明) | `POOL_ALPHA_PRE_WATER` / `POOL_ALPHA_POST_WATER` | `fullbrightF.glsl` (`HAS_ALPHA_MASK` + `IS_ALPHA`) | **NO** | depth write OFF なので SSS の depth opt-out (d>=0.9999) は効かないがそれ以外で flag 残留 |
| FB + Shiny (env reflection) | `POOL_BUMP` (`renderFullbrightShiny`) | `fullbrightShinyF.glsl` | **NO** | SSS bleed する |
| FB + Spec/Normal map (legacy material) | `POOL_MATERIALS` | `materialF.glsl` (emissive=1) | **YES** (`frag_data[3] = vec4(0,0,0, aya_sss_skin_flag)`) | gbuffer に書くので SSS 経路は他の deferred 物体と同じ |
| GLTF PBR で `unlit=true` | `POOL_FULLBRIGHT_ALPHA_MASK` (内で `GLTFSceneManager::render(unlit=true)`) | GLTF unlit shader | (要確認、本書未完) | unverified |
| FB Glow (`getGlow() > 0`) | `POOL_GLOW` (alpha 出力のみ) | `emissiveF.glsl` | **NO** (色は書かない、alpha のみ) | SSS pass の入力 color には影響しない |
| HUD 上の FB | `POOL_FULLBRIGHT*` (sRenderingHUDs=true) | 同 shader (`IS_HUD` permutation) | **N/A** (HUD 経路は SSS 対象外) | bug 無関係 |

→ **bug 影響範囲**: 1, 2, 3, 4 行目 (= `fullbrightF` / `fullbrightShinyF` で描かれる全 FB face)。5 行目 (materialF) は既に gbuffer に書くので影響無し。

---

## 1. FB pool 列挙

`indra/newview/lldrawpool.h:60-78` の pool 順序 (= render order):

```
POOL_SIMPLE = 4
POOL_FULLBRIGHT = 5           ← 本書 §2
POOL_BUMP = 6                 ← FB Shiny 経路は POOL_BUMP の下位機能 §4
POOL_MATERIALS = 7            ← FB material 系は materialF emissive 経路 §5
POOL_GLTF_PBR = 8
...
POOL_FULLBRIGHT_ALPHA_MASK = 13   ← 本書 §3
POOL_AVATAR = 15
POOL_GLOW = 18                ← FB の glow channel 経路 §6
POOL_ALPHA_PRE_WATER = 19     ← FB-alpha-blend 経路 §7
POOL_ALPHA_POST_WATER = 21    ← FB-alpha-blend 経路 §7
```

| Pool | 番号 | render entrypoint | render stage | sort order | 責務 |
|---|---|---|---|---|---|
| `POOL_FULLBRIGHT` | 5 | `LLDrawPoolFullbright::renderPostDeferred` (lldrawpoolsimple.cpp:156) | **postDeferred** | front-to-back (opaque batch order) | FB opaque (alpha 無し) |
| `POOL_FULLBRIGHT_ALPHA_MASK` | 13 | `LLDrawPoolFullbrightAlphaMask::renderPostDeferred` (lldrawpoolsimple.cpp:184) | **postDeferred** | front-to-back | FB alpha MASK (`discard`) |
| (FB Shiny part of `POOL_BUMP`) | 6 | `LLDrawPoolBump::renderFullbrightShiny` (lldrawpoolbump.cpp:356) | postDeferred (called from `LLDrawPoolBump::renderPostDeferred`) | front-to-back | FB + env reflection |
| (FB part of `POOL_ALPHA*`) | 19 / 21 | `LLDrawPoolAlpha::forwardRender` (lldrawpoolalpha.cpp:394) | postDeferred | **back-to-front** (alpha) | FB + alpha BLEND |
| `POOL_GLOW` | 18 | `LLDrawPoolGlow::renderPostDeferred` (lldrawpoolsimple.cpp:45) | postDeferred | front-to-back | FB の glow channel 出力 (alpha のみ) |

**重要**: 5 つすべて **`renderPostDeferred`** で動く = **gbuffer pass (`renderDeferred`) の後**。gbuffer は既に Simple / Bump / Materials / GLTF_PBR / Avatar が書き終わっていて、FB がそれを **forward で上書き** する形になる。これが §0 の bug 機構の根幹。

ref:
- pool render loop: `pipeline.cpp:5230-5318` (`renderGeomPostDeferred`)
- pool 順序が render order を決める仕様 comment: `lldrawpool.h:52-56`

---

## 2. POOL_FULLBRIGHT (= `LLDrawPoolFullbright`)

### 2.1 bind

```cpp
// lldrawpoolsimple.cpp:156-182
void LLDrawPoolFullbright::renderPostDeferred(S32 pass)
{
    LLGLSLShader* shader = nullptr;
    if (LLPipeline::sRenderingHUDs)
        shader = &gHUDFullbrightProgram;
    else
        shader = &gDeferredFullbrightProgram;

    gGL.setSceneBlendType(LLRender::BT_ALPHA);   // ← blend ON (= forward)

    shader->bind();
    pushBatches(LLRenderPass::PASS_FULLBRIGHT, true, true);

    if (!LLPipeline::sRenderingHUDs) {
        shader->bind(true);
        pushRiggedBatches(LLRenderPass::PASS_FULLBRIGHT_RIGGED, true, true);
    }
}
```

### 2.2 shader (`gDeferredFullbrightProgram`)

`llviewershadermgr.cpp:2023-2039`:
- vertex: `deferred/fullbrightV.glsl`
- fragment: `deferred/fullbrightF.glsl` (no permutation)
- `make_rigged_variant` で `HAS_SKIN` permutation の rigged 変種生成

### 2.3 fragment 出力

`fullbrightF.glsl:28` および `:98`:
```glsl
out vec4 frag_color;
...
frag_color = max(color, vec4(0));
```

- **frag_data[0..3] への書き込み一切無し** ← **bug の鍵**
- `color.rgb` は `srgb_to_linear(color.rgb)` で linear 化済 (line 83)
- `IS_HUD` permutation のときだけ sRGB→linear 変換と atmospheric fog をスキップ
- `IS_ALPHA` permutation のとき `waterClip` + sky/water fog を適用 (forward alpha 経路用、§7)

### 2.4 depth / blend / state

| state | 値 | 設定箇所 |
|---|---|---|
| Blend | `BT_ALPHA` (`SRC_ALPHA, 1-SRC_ALPHA`) | `lldrawpoolsimple.cpp:170` |
| Depth test | GL_TRUE / func GL_LEQUAL | caller (`renderGeomPostDeferred` のデフォルト state) を継承 |
| Depth write | **GL_TRUE** | caller (deferred pass 末尾の `LLGLDepthTest depth(GL_TRUE, GL_TRUE, GL_LEQUAL)` 継承) |
| Color mask | (true, false) → `setColorMask` 直前で再設定の場合あり | `pipeline.cpp:5214` で post-deferred 入口時に `setColorMask(true, false)` |

→ **opaque FB は depth を書く**。alpha 0 で完全透明な fragment でも (alpha discard が無いので) depth が書かれる。これは「FB prim の depth は scene depth として記録される」ことを意味し、skinSSSF.glsl:118 の `d_raw >= 0.9999` (sky opt-out) には引っかからない = **SSS は FB pixel を「scene 物体がある」と認識する**。

### 2.5 gbuffer3.a への寄与

**ゼロ**。`fullbrightF.glsl` は `frag_data[3]` に一切 write しない。よってその pixel の gbuffer3.a 値は:

1. その pixel の手前に deferred 物体 (avatar / material face / PBR opaque) が `aya_sss_skin_flag` を書いていれば、**その値がそのまま残る**
2. 何も書いていなければ deferred clear color のまま (= `(1.0, 0, 1.0, 1.0)` の `.a = 1.0`、`pipeline.cpp:addDeferredAttachments` の clear 仕様、`ayastorm-gbuffer3-trace.md` §C 参照)

**bug 経路**: avatar が FB prim の **手前** に存在しなくても、avatar の z が FB の z より遠ければ deferred pass で avatar が先に gbuffer に skin_flag=1 を書き、FB が postDeferred で **scene color (frag_color) を後から上書き** するが gbuffer3.a は塗り直さない。SSS pass は gbuffer3.a だけ見て「skin」判定 → scene color の FB pixel を入力に blur → pink がアバター形状で滲む。

---

## 3. POOL_FULLBRIGHT_ALPHA_MASK (= `LLDrawPoolFullbrightAlphaMask`)

### 3.1 bind

```cpp
// lldrawpoolsimple.cpp:184-214
void LLDrawPoolFullbrightAlphaMask::renderPostDeferred(S32 pass)
{
    // GLTF unlit (alpha mask) を先に
    LL::GLTFSceneManager::instance().render(true, false, true);
    LL::GLTFSceneManager::instance().render(true, true, true);

    LLGLSLShader* shader = (sRenderingHUDs)
        ? &gHUDFullbrightAlphaMaskProgram
        : &gDeferredFullbrightAlphaMaskProgram;

    LLGLDisable blend(GL_BLEND);   // ← blend OFF (= alpha mask は不透明扱い)

    shader->bind();
    pushMaskBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK, true, true);

    if (!sRenderingHUDs) {
        shader->bind(true);
        pushRiggedMaskBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK_RIGGED, true, true);
    }
}
```

### 3.2 shader

`llviewershadermgr.cpp:2062-2082`:
- 同じ `fullbrightV.glsl` / `fullbrightF.glsl` を使うが **`HAS_ALPHA_MASK` permutation** を立てる
- `fullbrightF.glsl:70-75` で `if (color.a < minimum_alpha) discard;`

### 3.3 fragment 出力 / depth / state

- `frag_color` 単一 RT 出力 (§2.3 と同じ)
- discard した fragment は depth / color とも書かない → FB が透けた領域 (cut-out 内側) では奥の deferred 物体の gbuffer3.a が **可視 pixel として** 残る
- discard しなかった fragment は §2.4 と同じ state (depth write ON、blend OFF)
- gbuffer3.a への寄与: §2.5 と同じ = **ゼロ**

### 3.4 GLTFSceneManager 経由の unlit

line 188-190 で `GLTFSceneManager::render(unlit=true, ...)` を呼ぶ。これは GLTF material の `unlit=true` を持つ face を別に dispatch する。bind される shader が `fullbrightF` か別 GLTF shader か未検証 (本書スコープ外、要確認)。

---

## 4. POOL_BUMP 内の FB Shiny (= `LLDrawPoolBump::renderFullbrightShiny`)

### 4.1 bind

```cpp
// lldrawpoolbump.cpp:285-354 (beginFullbrightShiny) / :356-386 (renderFullbrightShiny)
void LLDrawPoolBump::beginFullbrightShiny()
{
    shader = &gDeferredFullbrightShinyProgram;
    if (LLPipeline::sRenderingHUDs)
        shader = &gHUDFullbrightShinyProgram;
    if (mRigged)
        shader = shader->mRiggedVariant;
    ...
    shader->bind();
    ...
}

void LLDrawPoolBump::renderFullbrightShiny()
{
    LLGLEnable blend_enable(GL_BLEND);   // ← blend ON

    if (mShaderLevel > 1) {
        if (mRigged) pushRiggedBatches(PASS_FULLBRIGHT_SHINY_RIGGED, true, true);
        else         pushBatches(PASS_FULLBRIGHT_SHINY, true, true);
    } else {
        if (mRigged) pushRiggedBatches(PASS_FULLBRIGHT_SHINY_RIGGED);
        else         pushBatches(PASS_FULLBRIGHT_SHINY);
    }
}
```

呼び出しは `LLDrawPoolBump::renderPostDeferred` (lldrawpoolbump.cpp:609) から:
```cpp
beginFullbrightShiny();
renderFullbrightShiny();
endFullbrightShiny();
```

### 4.2 shader

- vertex: `deferred/fullbrightShinyV.glsl`
- fragment: `class3/deferred/fullbrightShinyF.glsl` (class3 配下にしか fragment が無い)

### 4.3 fragment 出力

`fullbrightShinyF.glsl:28` および `:95`:
```glsl
out vec4 frag_color;
...
color.a = 1.0;
frag_color = max(color, vec4(0));
```

- **frag_data 系一切無し** = gbuffer3.a に書かない
- `color.a = 1.0` でハードコード = scene color の glow channel 用 alpha を 1.0 で潰す (glow を「最大」にしてしまう、`POOL_GLOW` 経路と干渉する場合あり、本書スコープ外)
- env reflection は `sampleReflectionProbesLegacy` + `applyLegacyEnv` で applied (line 86, 90)

### 4.4 depth / state

| state | 値 | 設定箇所 |
|---|---|---|
| Blend | `GL_BLEND` enable | `lldrawpoolbump.cpp:361` |
| Depth test | GL_TRUE / GL_LEQUAL | 継承 |
| Depth write | **GL_TRUE** | 継承 (`renderPostDeferred` 入口の deferred default) |
| gbuffer3.a 寄与 | **ゼロ** | shader が書かない |

→ §2.5 と同じ bug pattern。SSS bleed 該当。

---

## 5. POOL_MATERIALS 内の FB face (= `materialF.glsl` の `emissive=1` 分岐)

### 5.1 routing

`LLVOVolume::genDrawInfo` (llvovolume.cpp:7018, :7039) で:
- `getTextureEntry()->getFullbright()` かつ `mat` が non-null かつ alpha MASK → `PASS_FULLBRIGHT_ALPHA_MASK` (POOL_FULLBRIGHT_ALPHA_MASK 行き、§3)
- それ以外で legacy material あり → `PASS_MATERIAL_ALPHA_EMISSIVE` / `PASS_SPECMAP_EMISSIVE` / `PASS_NORMMAP_EMISSIVE` / `PASS_NORMSPEC_EMISSIVE` (POOL_MATERIALS 行き、本節)

`emissive_brightness` uniform が `materialF.glsl:37` で:
```glsl
uniform float emissive_brightness;  // fullbright flag, 1.0 == fullbright, 0.0 otherwise
```

→ **legacy material (Spec/Normal map) を貼った FB face は POOL_MATERIALS 経由で materialF を踏む**。これは `POOL_FULLBRIGHT` ではなく **deferred path** (gbuffer に書く)。

### 5.2 fragment 出力

`materialF.glsl:188`:
```glsl
out vec4 frag_data[4];
```

line 440-450 (DIFFUSE_ALPHA_MODE != BLEND 分岐):
```glsl
frag_data[0] = max(vec4(diffcol.rgb, emissive), vec4(0));        // emissive を gbuffer0.a に書く
frag_data[1] = max(vec4(spec.rgb, glossiness), vec4(0));
frag_data[2] = encodeNormal(norm, env, flag);
#if defined(HAS_EMISSIVE)
frag_data[3] = vec4(0, 0, 0, aya_sss_skin_flag);                 // ← gbuffer3.a を skin_flag で正しく書く
#endif
```

→ **gbuffer3.a は `aya_sss_skin_flag` で書かれる**。FB であっても POOL_MATERIALS 経由ならば SSS の判定材料は正しく書かれる = **bug 影響なし**。

### 5.3 bug 影響まとめ

POOL_MATERIALS で FB を踏む face は SSS bleed しない。「FB prim 全部に SSS が透ける」のではなく **「Spec/Normal map を持たない素 FB prim (POOL_FULLBRIGHT 行き)」で透ける** という観測条件があり得る。AYA さんの実機検証で peeling パターンが prim 種別で違うようなら、それは POOL_FULLBRIGHT vs POOL_MATERIALS の分岐を踏んでいる。

---

## 6. POOL_GLOW (= `LLDrawPoolGlow`)

### 6.1 bind / state

`lldrawpoolsimple.cpp:45-71`:
```cpp
void LLDrawPoolGlow::renderPostDeferred(S32 pass)
{
    LLGLSLShader* shader = &gDeferredEmissiveProgram;

    LLGLEnable blend(GL_BLEND);
    LLGLEnable polyOffset(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    gGL.setSceneBlendType(LLRender::BT_ADD);              // ← additive
    LLGLDepthTest depth(GL_TRUE, GL_FALSE);               // ← depth test ON / depth write OFF
    gGL.setColorMask(false, true);                        // ← RGB 書かない、alpha のみ

    shader->bind();
    pushBatches(LLRenderPass::PASS_GLOW, true, true);

    shader = shader->mRiggedVariant;
    shader->bind();
    pushRiggedBatches(LLRenderPass::PASS_GLOW_RIGGED, true, true);

    gGL.setColorMask(true, false);
    gGL.setSceneBlendType(LLRender::BT_ALPHA);
}
```

### 6.2 shader / 出力

`emissiveF.glsl:37`:
```glsl
frag_color = max(vec4(0, 0, 0, a), vec4(0));
```

- RGB は 0 (color mask で書かれない)
- alpha のみ書く (scene color の glow channel = 後の blur で glow 効果に使う)
- gbuffer3.a への寄与: **ゼロ**

### 6.3 bug 影響

POOL_GLOW は scene color の RGB を変更しないので SSS pass の入力 RGB に影響しない。**bug 経路と無関係**。FB の emissive 表現は POOL_GLOW で alpha (glow strength) を書き、POOL_FULLBRIGHT で RGB を書く、と役割分担している。

(glow blur 全体経路は本書スコープ外、別 doc で整理する余地あり)

---

## 7. POOL_ALPHA 内の FB (= forward alpha BLEND 経路)

### 7.1 routing

FB かつ alpha BLEND な face は `LLVOVolume::registerFace` で `PASS_ALPHA` に登録される (llvovolume.cpp:7137 など):
```cpp
registerFace(group, facep, fullbright ? PASS_FULLBRIGHT : PASS_SIMPLE);  // line 7137: alpha mask ではない
```

…ではなく、alpha BLEND は `LLDrawPoolAlpha` 側で `mFullbright` 判定で shader を分けて bind:

```cpp
// lldrawpoolalpha.cpp:174-178
fullbright_shader =
    (sImpostorRender) ? &gDeferredFullbrightAlphaMaskProgram :
    (sRenderingHUDs)  ? &gHUDFullbrightAlphaMaskAlphaProgram :
                        &gDeferredFullbrightAlphaMaskAlphaProgram;
```

そして `renderAlpha` ループ (line 891 付近) で `params.mFullbright` を見て shader を切替:
```cpp
if (params.mFullbright)
    target_shader = fullbright_shader;
```

### 7.2 shader

`gDeferredFullbrightAlphaMaskAlphaProgram` の構成 (`llviewershadermgr.cpp:2108-2128`):
- 同じ `fullbrightF.glsl` を使うが permutation 3 つ:
  - `HAS_ALPHA_MASK = 1`
  - `IS_ALPHA = 1` → fullbrightF.glsl:45-51, :84-93 の atmospheric/water fog ブロックが有効化される
  - (HUD 版は更に `IS_HUD = 1`)

### 7.3 fragment 出力 / depth

- 出力: `frag_color` 単一 RT (§2.3 と同じ)、gbuffer 一切書かない
- depth write: `LLDrawPoolAlpha::forwardRender` (lldrawpoolalpha.cpp:411):
  ```cpp
  bool write_depth = rigged ||
      sSkipScreenCopy ||
      sImpostorRenderAlphaDepthPass ||
      getType() == POOL_ALPHA_PRE_WATER;

  LLGLDepthTest depth(GL_TRUE, write_depth ? GL_TRUE : GL_FALSE);
  ```
  - **POOL_ALPHA_POST_WATER (=21) かつ非 rigged** の場合 `write_depth = false` → **depth write OFF**
  - POOL_ALPHA_POST_WATER かつ rigged → depth write ON (2026-05-22 の二重アルファブロック fix で commit `2597b657ac` 以降は forward 順序が non-rigged → rigged に変更)
  - POOL_ALPHA_PRE_WATER → depth write ON (water fog 整合性のため)

### 7.4 bug 影響

- depth write OFF の場合 (POST_WATER 非 rigged FB-alpha-blend): その pixel の scene depth は手前の opaque 物体 (deferred で書かれた値) のまま。skinSSSF の depth-based eye_dist 復元はその opaque 物体の z を使う = blur radius が opaque 物体距離基準
- gbuffer3.a への寄与: **ゼロ** (FB shader が書かない)
- → opaque pass で skin avatar が gbuffer3.a=1 を書いた pixel に、後段で FB-alpha-blend が乗ると SSS pass は依然「skin」と判定 → FB の半透明色を入力に blur

---

## 8. depth write 挙動の全列挙 (bug の鍵)

| 経路 | depth test | depth write | blend | gbuffer3.a write | SSS bleed リスク |
|---|---|---|---|---|---|
| `POOL_FULLBRIGHT` opaque (§2) | GL_LEQUAL | **ON** | SRC_ALPHA/1-SRC_ALPHA | **NO** | **HIGH** (主犯) |
| `POOL_FULLBRIGHT_ALPHA_MASK` (§3) | GL_LEQUAL | **ON** (discard 後の残 fragment) | OFF | **NO** | **HIGH** |
| `POOL_BUMP` FB Shiny (§4) | GL_LEQUAL | **ON** | ON (BLEND) | **NO** | **HIGH** |
| `POOL_MATERIALS` emissive=1 (§5) | GL_LEQUAL | **ON** | OFF (deferred) | **YES** | LOW (gbuffer に正しく書く) |
| `POOL_GLOW` (§6) | GL_LEQUAL | **OFF** | ADD | NO | (RGB 書かないので無関係) |
| `POOL_ALPHA_PRE_WATER` FB (§7) | GL_LEQUAL | **ON** | ON | **NO** | MEDIUM |
| `POOL_ALPHA_POST_WATER` FB rigged (§7) | GL_LEQUAL | **ON** | ON | **NO** | MEDIUM |
| `POOL_ALPHA_POST_WATER` FB non-rigged (§7) | GL_LEQUAL | **OFF** | ON | **NO** | MEDIUM (depth opt-out も効かない) |

ref:
- `POOL_FULLBRIGHT*` の depth: `lldrawpoolsimple.cpp:156-214` (明示なし、caller 継承)
- caller の depth state: `pipeline.cpp:5230-5318` (`renderGeomPostDeferred` で `LLGLDepthTest` の明示無し → 直前の deferred pass の `GL_TRUE/GL_TRUE/GL_LEQUAL` が継承される)
- `POOL_BUMP` FB Shiny: `lldrawpoolbump.cpp:285-406` (depth test 明示無し)
- `POOL_GLOW`: `lldrawpoolsimple.cpp:57` (`LLGLDepthTest depth(GL_TRUE, GL_FALSE);` で明示)
- `POOL_ALPHA*` forward: `lldrawpoolalpha.cpp:403-411`

---

## 9. SSS pass との接合 (bug 仮説の確定)

`pipeline.cpp:11758-11920` (`LLPipeline::doSkinSSS`) の動作:

1. **dispatch 地点**: `renderGeomPostDeferred` の loop 内で `cur_type >= POOL_ALPHA_POST_WATER (21)` に到達した時点で `doAtmospherics()` と並んで `doSkinSSS()` が呼ばれる (pipeline.cpp:5242-5263)。すなわち `POOL_FULLBRIGHT (5)` / `POOL_FULLBRIGHT_ALPHA_MASK (13)` / `POOL_BUMP FB Shiny (6)` / `POOL_GLOW (18)` は **すべて SSS pass の前** に描画される
2. **入力**: `mRT->screen` (scene color、既に FB で塗られた状態) + `gbuffer3` (`.a` = skin flag) + `mRT->deferredScreen.depth`
3. **判定**: `skinSSSF.glsl:171-172`:
   ```glsl
   float skin_mask = texture(emissiveRect, tc).a;
   float skin_bit  = (skin_mask >= 0.5) ? 1.0 : 0.0;
   ```
4. **出力**: `frag_color = vec4(sum, aya_strength * skin_bit)` (skin_bit=0 ならば alpha=0 で blend pass-through)

**bug 機構の確定**:
- FB pool は `frag_data[3]` に書かない (§2.5 §3.3 §4.3 §7.3)
- よって FB が covered pixel の gbuffer3.a は **その pixel に *opaque pass で先に書いていた deferred 物体* の値** で固定
- avatar / skin material は `materialF` / `pbropaqueF` / `avatarF` で `aya_sss_skin_flag = 1.0` を書く (`ayastorm-deferred-shader-routing.md` §2、`ayastorm-gbuffer3-trace.md` §A)
- FB prim の手前 / 同位置に avatar が opaque pass で gbuffer3.a=1 を書き、その後に FB が postDeferred で **scene color のみ** を上書きしても **gbuffer3.a は 1 のまま** → SSS pass は「ここは skin」 → FB の color (テクスチャ色) を入力に SSS blur をかける → pink がアバター形状で滲み出る

### 9.1 fix の方向

3 案:

| 案 | 内容 | コスト | 副作用 |
|---|---|---|---|
| A | `fullbrightF` / `fullbrightShinyF` を MRT shader に書き換えて `frag_data[3] = vec4(0,0,0,0)` で skin_flag=0 を明示 write | low | FB pool の **forward 描画は currently single-RT に bind されている** ので、MRT bind に切り替える C++ 修正も要る (pool 側で `mRT->deferredScreen` を改めて bind 必要)。FB が deferred clear color の `.a=1.0` を `.a=0` で上書きする副作用も検討 (sky safety net `d_raw>=0.9999` 経路の保護は深度で別途効くので問題は出ない見込み) |
| B | SSS pass 側で depth-based heuristic を追加: FB 検出のため別の判定材料 (gbuffer1 spec=0 + gbuffer2 normal=0 など、deferred 既定 clear で残るパターン) で「ここは forward 上書き済」を推定 | medium | 偽陽性リスク (gbuffer 値が他経路でも 0 になる可能性)。AYA さんが嫌う heuristic |
| C | 別 RT (skin_mask 専用 R8 buffer) を追加して、forward FB の `frag_color` と並列に skin_mask=0 を書く | high | 帯域・メモリ増、3 OS 全部の addDeferredAttachments 改修必要 |

**recommend: 案 A**。`materialF` / `pbropaqueF` / `avatarF` と完全に対称な実装になる (gbuffer3.a を 0 で明示 write)。MRT bind の C++ 修正は必要だが、`pipeline.cpp:addDeferredAttachments` が allocate する gbuffer attachment は `mRT->deferredScreen` で固定なので、`renderPostDeferred` 入口で `mRT->deferredScreen.bindTarget()` し直す形になる。ただし FB は **scene color (`mRT->screen`)** に書きたいので、bind 先は scene color のままで MRT に gbuffer3 を attach する hybrid 構成が必要。要検証。

簡易代替案: SSS pass の発火を **POOL_FULLBRIGHT の前** に動かす。すなわち `pipeline.cpp:5242` の `atmospherics_pass = POOL_ALPHA_POST_WATER (21)` を `POOL_FULLBRIGHT (5)` まで下げる。SSS が走る時点では gbuffer3.a がまだ「skin pixel = 1」だけど scene color もまだ FB で上書きされていない avatar 色 → SSS が正しく avatar 色に blur → その後 FB が forward 上書き → FB pixel は SSS かかった結果の上に塗られる → bug 解消。ただし atmospherics と SSS の順序が逆転する副作用検討必要。これも案 D として記録。

---

## 10. AYAstorm 固有改変

| commit | 説明 | FB への影響 |
|---|---|---|
| `4eb20eea93` (Fix fullbright global render toggle, 2026-05-23) | `RenderEnableFullbright` cvar を追加、`llvovolume.cpp:1900` で `face` の FULLBRIGHT state を gate、registerFace 経路で `teFullbrightEnabled` 経由判定に置換 | **cvar OFF にすると FB face が POOL_FULLBRIGHT に登録されない → POOL_SIMPLE 等の deferred 経路に流れる → gbuffer3.a が `aya_sss_skin_flag` で正しく書かれる**。SSS bleed bug の暫定回避になる (ただし FB 表現が消えるので運用は不可) |
| `2597b657ac` (二重アルファブロック fix, 2026-05-22) | `LLDrawPoolAlpha::renderPostDeferred` の forward 順序を non-rigged → rigged に default 化 | POOL_ALPHA 内の FB-alpha-blend 経路 (§7) の描画順だけ影響、gbuffer3.a 周辺は無関係 |
| `7d68ec63f3` (r30 P2 motion blur, lldrawpoolsimple.cpp:215+) | `LLDrawPoolFullbright::renderMotionBlur` / `LLDrawPoolFullbrightAlphaMask::renderMotionBlur` 追加 | motion blur 専用 pass で `gVelocityProgram` bind、本書スコープ外 |
| `2c4d02465d` (r30 P5 透過 DoF L2-β) | alpha BLEND 経路に depth 再注入 logic | POOL_ALPHA 内 FB-alpha-blend (§7) の depth write 挙動に間接影響、本書 §7.3 の write_depth ロジックはこの修正後の状態 |

### 10.1 r31 WBOIT と FB の関係

(`project_ayastorm_r31_wboit.md` memory 参照)
r31 WBOIT 化対象は forward alpha BLEND 経路 (POOL_ALPHA_PRE_WATER / POST_WATER)。FB-alpha-blend (§7) は WBOIT で accumulation + revealage 経路に乗る。WBOIT で gbuffer3.a の挙動が変わるか要確認 (現状想定: WBOIT 中も gbuffer は触らない、scene color の OIT 結合のみ)。本書の bug 仮説とは独立。

---

## 11. C++ shader / pool / pass の主要 cite

| ファイル | 関数 / 行 | 役割 |
|---|---|---|
| `lldrawpoolsimple.cpp` | `LLDrawPoolFullbright::renderPostDeferred` (156) | POOL_FULLBRIGHT bind + dispatch (§2) |
| `lldrawpoolsimple.cpp` | `LLDrawPoolFullbrightAlphaMask::renderPostDeferred` (184) | POOL_FULLBRIGHT_ALPHA_MASK bind (§3) |
| `lldrawpoolsimple.cpp` | `LLDrawPoolGlow::renderPostDeferred` (45) | POOL_GLOW bind (§6) |
| `lldrawpoolbump.cpp` | `LLDrawPoolBump::beginFullbrightShiny` (285) | POOL_BUMP FB Shiny bind (§4) |
| `lldrawpoolbump.cpp` | `LLDrawPoolBump::renderFullbrightShiny` (356) | POOL_BUMP FB Shiny dispatch (§4) |
| `lldrawpoolalpha.cpp` | `LLDrawPoolAlpha::beginRenderPass` (174-178) | POOL_ALPHA 内 FB shader 選択 (§7) |
| `lldrawpoolalpha.cpp` | `LLDrawPoolAlpha::forwardRender` (394-455) | POOL_ALPHA forward 描画、depth write 判定 (§7.3) |
| `llvovolume.cpp` | `LLVolumeGeometryManager::genDrawInfo` (7018, 7039, 7137, 7152, 7180, 7199, 7219, 7223, 7264) | face → PASS_FULLBRIGHT* / PASS_FULLBRIGHT_SHINY* / PASS_FULLBRIGHT_ALPHA_MASK* / POOL_FULLBRIGHT 登録 (§1, §5) |
| `llvovolume.cpp` | `LLVolumeGeometryManager::registerFace` (5632-5639) | FB 判定の中心 |
| `llvovolume.cpp` | `teFullbrightEnabled` (138, AYAstorm 追加) | `RenderEnableFullbright` cvar 経由の gate (§10) |
| `llviewershadermgr.cpp` | `gDeferredFullbrightProgram` 構成 (2023-2039) | shader file binding (§2.2) |
| `llviewershadermgr.cpp` | `gDeferredFullbrightAlphaMaskProgram` 構成 (2062-2082) | HAS_ALPHA_MASK permutation (§3.2) |
| `llviewershadermgr.cpp` | `gDeferredFullbrightAlphaMaskAlphaProgram` 構成 (2108-2128) | HAS_ALPHA_MASK + IS_ALPHA permutation (§7.2) |
| `llviewershadermgr.cpp` | `gDeferredFullbrightShinyProgram` 構成 (~2200) | FB Shiny shader (§4.2) |
| `llviewershadermgr.cpp` | `gDeferredEmissiveProgram` 構成 (2204-2207) | POOL_GLOW shader (§6) |
| `pipeline.cpp` | `renderGeomPostDeferred` (5178-5337) | postDeferred の pool loop + `doSkinSSS` dispatch (§9) |
| `pipeline.cpp` | `doSkinSSS` (11758-11920) | SSS 2-pass blur 本体 (§9) |
| `pipeline.cpp` | `renderShadow` (12380-12545) | shadow pass で FB / FB Shiny / FB Alpha Mask を含む (line 12392-12395, 12513) |

## 12. shader file の主要 cite

| ファイル | 行 | ポイント |
|---|---|---|
| `class1/deferred/fullbrightF.glsl` | 28 | `out vec4 frag_color;` 単一 RT 宣言 |
| `class1/deferred/fullbrightF.glsl` | 70-75 | `HAS_ALPHA_MASK` で discard |
| `class1/deferred/fullbrightF.glsl` | 82-94 | `IS_HUD` / `IS_ALPHA` permutation 分岐 |
| `class1/deferred/fullbrightF.glsl` | 98 | `frag_color = max(color, vec4(0));` (gbuffer 不在の確定) |
| `class1/deferred/fullbrightV.glsl` | 51-74 | HAS_SKIN permutation 経由の rigged 対応 |
| `class3/deferred/fullbrightShinyF.glsl` | 28, 95 | `out vec4 frag_color;` / `frag_color = max(color, vec4(0));` (FB Shiny も単一 RT) |
| `class3/deferred/fullbrightShinyF.glsl` | 93 | `color.a = 1.0` (glow alpha 強制) |
| `class3/deferred/materialF.glsl` | 37 | `uniform float emissive_brightness;` (FB flag) |
| `class3/deferred/materialF.glsl` | 188, 440-450 | MRT 出力、`aya_sss_skin_flag` を gbuffer3.a に書く |
| `class1/deferred/emissiveF.glsl` | 37 | POOL_GLOW shader、alpha のみ書く |
| `class1/deferred/skinSSSF.glsl` | 118-122 | sky safety net (d_raw>=0.9999 で frag_color=vec4(0)) |
| `class1/deferred/skinSSSF.glsl` | 171-173 | skin_mask threshold + `frag_color = vec4(sum, aya_strength * skin_bit)` |
| `class1/deferred/pbrglowF.glsl` | 28, 35-37 | HUD 用 forward fullbright PBR、`frag_color` 単一 RT |
| `class1/deferred/pbralphaF.glsl` | 28 | PBR alpha forward、`frag_color` 単一 RT (gbuffer 書かない) |

---

## 13. 落とし穴 / 再発防止メモ

1. **「FullBright」は 1 つの pool ではない**。POOL_FULLBRIGHT / POOL_FULLBRIGHT_ALPHA_MASK / POOL_BUMP の FB Shiny / POOL_ALPHA の FB 経路 / POOL_MATERIALS の emissive=1 と **5 系統**ある。bug 解析で 1 系統だけ修正して別系統で再発する罠。`fullbrightF.glsl` を触ったら `fullbrightShinyF.glsl` も同じ修正が必要 (§13.5)
2. **POOL_FULLBRIGHT は `renderPostDeferred` でしか描画されない**。`renderDeferred` 経路にいないので「gbuffer 段で何かする」想定は崩れる。fix を入れるなら postDeferred 段で gbuffer3 を別途 bind する hybrid が必要
3. **`emissive_brightness` uniform と FB pool は別物**。materialF が `emissive_brightness=1` で「FB のように振る舞う」ケースは POOL_MATERIALS 内で起きる (gbuffer に書く)。POOL_FULLBRIGHT の FB shader (`fullbrightF`) は `emissive_brightness` uniform を *持たない*
4. **depth write は明示されていない**。`LLDrawPoolFullbright::renderPostDeferred` には `LLGLDepthTest` の明示 setup が無いので、caller の継承で動く。「FB は depth を書かない」と推測すると外す
5. **`fullbrightShinyF.glsl` は class3 にしかない**。class1 配下に存在しないので `grep -r fullbrightShinyF class1` が空振る罠。`make_rigged_variant` で rigged 版を作るが fragment は同じ
6. **`color.a = 1.0` を ShinyF が hardcode する** (line 93)。これは glow alpha 用の固定だが、FB Shiny pixel の scene color の `.a` チャネルが常に 1 になる = POOL_GLOW の glow buffer 計算と干渉する可能性。本 bug とは別軸
7. **SSS pass は postDeferred の途中 (POOL_ALPHA_POST_WATER 到達時) で発火する**。FB pool は全部 SSS より **前** に描画されるが、それでも bug は起きる (gbuffer3.a が FB で上書きされないため)。「FB を SSS の後で描けば直る」のは半分だけ正しい (案 D の方向、§9.1)
8. **`add_common_permutations` の `HAS_EMISSIVE` permutation** (`llviewershadermgr.cpp:272`) が `fullbrightF` には立たない。`materialF` / `pbropaqueF` / `avatarF` でしか `aya_sss_skin_flag` への gbuffer3.a 書き込みが有効化されない構造。fix で `fullbrightF` にも `HAS_EMISSIVE` 経路を追加する場合は `add_common_permutations` 適用も要確認

---

## 14. 関連ファイル / 姉妹資料

- `docs/specs/ayastorm-deferred-shader-routing.md` — deferred 全体の object → pool → shader → gbuffer routing
- `docs/specs/ayastorm-gbuffer3-trace.md` — gbuffer3 (DEFERRED_EMISSIVE / `.a` = SSS skin flag) の storage 仕様と writer 一覧
- `docs/specs/ayastorm-attachment-rendering-routing.md` — 装着物 routing (alpha BLEND 系の peeling と関連)
- `docs/specs/ayastorm-rez-object-rendering-routing.md` — Rez Object routing (二重アルファブロック fix 関連)
- `docs/specs/ayastorm-r30-p4-bd-dof-chain-trace.md` — DoF 経路 (FB-alpha-blend の depth write と関連)

## 15. 更新履歴

- 2026-05-25 初版: SSS pink bleed bug 解析として、`POOL_FULLBRIGHT` / `POOL_FULLBRIGHT_ALPHA_MASK` / `POOL_BUMP` FB Shiny / `POOL_ALPHA` 内 FB / `POOL_MATERIALS` emissive=1 / `POOL_GLOW` の 6 経路を実コード trace で確定。fix 案 A (FB shader に `frag_data[3] = vec4(0,0,0,0)` write 追加) を recommend として記録。実機 canary 検証は未実施 (本書は静的解析ベース、AYA さん側で fix 案 A 試作 → 実機で peeling 解消確認の予定)
