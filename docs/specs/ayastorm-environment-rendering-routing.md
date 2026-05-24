# AYAstorm Environment Rendering Routing リファレンス

**作成日**: 2026-05-25 (effect 軸地図シリーズ 4 本目: SSS / FullBright / Glow / 環境 のうち「環境」)
**対象**: 環境 (Sky / Cloud / Water / Terrain) が deferred pipeline で踏む **全描画 path** の確定マップ。「装着物」「Rez Object」が `mAttachedToAvatar` 軸でオブジェクトを切ったのに対し、本書は **環境系 pool** という別軸 (オブジェクト軸ではなく effect 軸) で 4 種をまとめた。
**作成経緯**: 「FullBright prim 越しに sky が透ける」bug (r30 BD改善期、commit `4188880321` "Fix sky emissive alpha blending") が即修正できた一方、続く「FullBright prim 越しに SSS pink shadow が漏れる」bug の前哨戦として、sky/cloud/water/terrain 描画ルートを地図化しておけば類似 bug の予防/解析に効く、という動機。

> **再利用方針**: 環境描画系を触る前 (例: sky shader に何か注入する / water reflection に何か乗せる / terrain blend を変える / SSS や FullBright と環境の干渉を追う) に **まず本書を参照**。pool 配線や pass 順序が code 変更で動く可能性があるので、本書の年月日より新しい commit が `pipeline.cpp` の `renderGeomDeferred` / `renderGeomPostDeferred` / `LLDrawPoolWLSky` / `LLDrawPoolWater` / `LLDrawPoolTerrain` 周辺に入っていたら本書の更新要否を確認すること。

---

## 0. 結論先出し (4 種を貫く構造的特徴 3 行)

1. **Sky / Cloud / Sun / Moon / Star は全て LLDrawPoolWLSky 単一 pool に収束する** (`LLDrawPoolSky` は deprecated stub、`LLVOSky` 自体は POOL_SKY に face を生やすが実描画は `LLDrawPoolWLSky::renderDeferred` 経由で `gSky.mVOSkyp` の face を直叩き)。**そして sky pass は `endDeferredPass()` で `glClear(GL_DEPTH_BUFFER_BIT)` を呼んで描画直後に depth を全消去する** (`lldrawpoolwlsky.cpp:92`) — 「sky の depth は haze/post に持ち越されない」が本 pipeline の前提。FB 透け bug の鍵だった部分。
2. **Water は 2 段構え RT 系統**: ① `mWaterDis` (画面サイズ、color+depth、毎フレーム水面を描く直前に画面 color と deferred depth を copy して refraction 入力にする) ② `mHeroProbeRT` + Hero Reflection Probe cubemap (mirror flip の古典 reflection は廃止、`RenderMirrors` 有効時のみ `LLHeroProbeManager::renderProbes` で水面位置 cubemap を 6 面更新)。**従来の `mWaterRef` は code から消えており**、水面反射は cubemap reflection probe に統一されている。
3. **Terrain は普通の opaque pool だが、上の 3 種 (sky/cloud/water) と違い `mAttachedToAvatar` / `mIsBoMBodyOrHead` 等の per-draw flag を経由しない**。LLDrawInfo を作らず `mDrawFace` を直叩きする face-iter pattern。装着物資料 / Rez Object 資料の canary uniform 配線は terrain には届かない (= 環境系を識別したいなら本書側で別 uniform を設計する必要)。

**FB 透け bug が即修正できた理由 (構造観察)**: §A.2 の「sky `frag_data[3].a`」を `0.0` から `1.0` に戻すだけで済んだのは、sky の write 先 attachment と blend mode が **opaque path で完結している** ため。深い trace 不要だった。一方 SSS 漏れは同じ `gbuffer3.a` を SSS mask として再利用しており、sky が `0` を書く必要がある (alpha leak で SSS 誤発火) — つまり同じ 1bit に **2 つの意味** (MRT blend alpha / SSS skin mask) が乗っており、これが透過装飾物との干渉を生む。詳細 §A.6。

---

## 1. オブジェクト → Pool → Shader 早見表 (4 種全表)

| 環境種別 | LLVO クラス | Pool (`lldrawpool.h:57-79`) | Bound shader (fragment) | 描画関数 |
|---|---|---|---|---|
| **Sky dome (haze / atmospheric)** | `LLVOWLSky` + `LLVOSky` | `POOL_WL_SKY` (= 3) | `class1/deferred/skyF.glsl` (WL preset) / `gEnvironmentMapProgram` (HDRI sky 時) | `LLDrawPoolWLSky::renderSkyHazeDeferred` (`lldrawpoolwlsky.cpp:143`) |
| **Cloud (2D + AYAstorm r18 volumetric)** | `LLVOWLSky` (drawDome 共用) | `POOL_WL_SKY` (= 3) | `class1/deferred/cloudsF.glsl` | `LLDrawPoolWLSky::renderSkyCloudsDeferred` (`lldrawpoolwlsky.cpp:290`) |
| **Sun disc** | `LLVOSky` (FACE_SUN) | `POOL_SKY` (= 1) face owner だが描画は `LLDrawPoolWLSky` 経由 | `class1/deferred/sunDiscF.glsl` | `LLDrawPoolWLSky::renderHeavenlyBodies` (`lldrawpoolwlsky.cpp:354`) |
| **Moon** | `LLVOSky` (FACE_MOON) | 同上 | `class1/deferred/moonF.glsl` | 同上 (`lldrawpoolwlsky.cpp:419`) |
| **Stars** | `LLVOWLSky::drawStars` | `POOL_WL_SKY` (= 3) | `class1/deferred/starsF.glsl` | `LLDrawPoolWLSky::renderStarsDeferred` (`lldrawpoolwlsky.cpp:219`) |
| **HDRI sky (Reflection probe 用)** | `gEXRImage` (texture only、geometry は sky dome 流用) | `POOL_WL_SKY` (= 3) | `class1/deferred/skyF.glsl` (`#ifdef HAS_HDRI` permutation) | sky haze と同じ関数内 `use_hdri_sky()` 分岐 (`lldrawpoolwlsky.cpp:152-175`) |
| **Water (region + void)** | `LLVOWater` (region) / `LLVOVoidWater` (edge patch) | `POOL_WATER` (= 19) | `class3/environment/waterF.glsl` (above) / `class3/environment/underWaterF.glsl` (camera < waterHeight) | `LLDrawPoolWater::renderPostDeferred` (`lldrawpoolwater.cpp:142`) |
| **Water exclusion mask (invisible prim占有)** | (専用 VO 無し、`PASS_INVISIBLE` batch + water plane 自体) | `POOL_WATEREXCLUSION` (= 2) | `gDrawColorProgram` (uniform color のみ) | `LLDrawPoolWaterExclusion::render` (`lldrawpoolwaterexclusion.cpp:43`) |
| **Water haze (sky 由来の遠景 fog 適用)** | (専用 VO 無し、screen-space pass) | (pool 外、`doWaterHaze()` 直接 dispatch) | `class3/deferred/waterHazeV.glsl` + `waterHazeF.glsl` (`gHazeWaterProgram`) | `LLPipeline::doWaterHaze` (`pipeline.cpp:11923`) |
| **Legacy terrain (4-texture splat)** | `LLVOSurfacePatch` | `POOL_TERRAIN` (= 8) | `class1/deferred/terrainF.glsl` | `LLDrawPoolTerrain::renderDeferred` → `renderFullShaderTextures` (`lldrawpoolterrain.cpp:147`) |
| **PBR terrain** | `LLVOSurfacePatch` | `POOL_TERRAIN` (= 8) | `class1/deferred/pbrterrainF.glsl` (`gDeferredPBRTerrainProgram[paint_type]`) | `LLDrawPoolTerrain::renderDeferred` → `renderFullShaderPBR` (`lldrawpoolterrain.cpp:147`) |

参考 (本書 scope 外、Rez Object 資料 §0 で「Rez Object に含まれないもの」として除外されている):
- **Grass** — `LLDrawPoolGrass` (POOL_GRASS = 9)、Linden grass
- **Particles** — alpha pool 内分岐

---

## 2. 描画順序 (deferred pipeline 全体マップ)

pool 番号 (`lldrawpool.h:57-79`) は **render order** も兼ねている。コメント (`lldrawpool.h:52-55`): *"Correspond to LLPipeline render type. Also controls render order, so passes that don't use alpha masking/blending should come before other passes to preserve hierarchical Z for occlusion queries."*

### 2.1 `LLPipeline::renderGeomDeferred` (`pipeline.cpp:5039`) で走る pool (= gbuffer/Z 書き込み)

順序は pool 番号順:

| 順 | Pool | 環境系? | 備考 |
|---|---|---|---|
| 1 | `POOL_SKY` (= 1) | (sky face owner) | `LLDrawPoolSky` は **deprecated stub** (`lldrawpoolsky.cpp` 全関数空)、実描画なし |
| 2 | `POOL_WATEREXCLUSION` (= 2) | **環境** | `gDrawColorProgram` で water plane を depth-only マスクとして書く (実 color は後段で上書き) |
| 3 | **`POOL_WL_SKY` (= 3)** | **環境 (sky / cloud / sun / moon / star 全部)** | `LLDrawPoolWLSky::renderDeferred` (`lldrawpoolwlsky.cpp:471`) — sky haze → 太陽/月 → 星 → 雲 の順 |
| 4 | `POOL_SIMPLE` (= 4) | — | (Rez Object / 装着物の opaque) |
| ... | (5〜7: FULLBRIGHT / BUMP / MATERIALS / GLTF_PBR) | — | |
| 8 | **`POOL_TERRAIN` (= 8)** | **環境** | `LLDrawPoolTerrain::renderDeferred` (`lldrawpoolterrain.cpp:147`) — legacy or PBR terrain |
| 9 | `POOL_GRASS` (= 9) | (環境扱いだが本書 scope 外) | occlusion query の境界 (`pipeline.cpp:5109` で `cur_type >= POOL_GRASS` で occlusion off) |
| 10〜18 | (mask alpha / tree / avatar / glow / 等) | — | |
| 19 | **`POOL_WATER` (= 19)** | **環境** | renderDeferred では描かない (getNumDeferredPasses=0)、POST に回る |

**観察**: sky/cloud/star/sun/moon が **pool 番号 3** = 早い段階で描かれる。terrain は 8、water は 19。**occlusion query の境界が POOL_GRASS (9)** で、sky/cloud (3) / terrain (8) は occlusion 対象外。

### 2.2 `LLPipeline::renderGeomPostDeferred` (`pipeline.cpp:5178`) で走る pool (= forward 合成)

順序は pool 番号順、ただし **special pass** が間に挟まる:

| 順 | Pool / Special pass | 環境系? | 備考 |
|---|---|---|---|
| (special) | **`doWaterExclusionMask()`** (`pipeline.cpp:5238`、`cur_type >= POOL_WATEREXCLUSION` で 1 度だけ) | **環境** | `mWaterExclusionMask` RT に water plane mask を書く |
| (special) | **`doAtmospherics()`** (`pipeline.cpp:5244`、`cur_type >= POOL_ALPHA_POST_WATER` で 1 度だけ。**ただし `sUnderWaterRender` 時は `POOL_WATER` に前倒し**) | **環境** | screen + deferred depth を `mWaterDis` にコピー → `gHazeProgram` でフルスクリーン haze blend |
| (special) | **`doGodrays()` / `doSkinSSS()`** (AYAstorm r15 / r20、`pipeline.cpp:5256-5263`) | (補助) | atmospherics 同タイミングで HDR scene buffer に追加 effect |
| (special) | **`doWaterHaze()`** (`pipeline.cpp:5269`、`cur_type >= POOL_ALPHA_PRE_WATER` で 1 度だけ) | **環境** | water plane 形状で fog を water plane に焼く (`gHazeWaterProgram`) |
| | `POOL_ALPHA_PRE_WATER` (= 17) | — | water より手前の alpha |
| | `POOL_VOIDWATER` (= 18) | — | (POOL_WATER と同 shader で renderPostDeferred、edge patch) |
| | **`POOL_WATER` (= 19)** | **環境** | `LLDrawPoolWater::renderPostDeferred` (`lldrawpoolwater.cpp:142`) — 水面 forward 合成 |
| | `POOL_ALPHA_POST_WATER` (= 20) | — | water より奥の alpha (透過装着物の大半) |

**観察**: water は POST の最後の方で forward 描画される (alpha 前後で挟む)。`POOL_WATER` の renderPostDeferred 前の `beginPostDeferredPass` で **screen color + depth を `mWaterDis` にコピー** (`lldrawpoolwater.cpp:111-140`、`LLPipeline::sRenderTransparentWater` 時) — これが refraction 入力。

---

## 3. Hero Reflection Probe (= 水面反射の現代版)

旧 `mWaterRef` ベースの mirror-flip planar reflection は **削除済**。水面反射は `LLHeroProbeManager` (`llheroprobemanager.cpp`) が管理する **動的 cubemap reflection probe** に統一されている。

### 3.1 起動条件

- `RenderMirrors` cvar が ON (`pipeline.cpp:1566`)
- `LLPipeline::sReflectionProbesEnabled` が ON
- `gSnapshot` でない (`llviewerdisplay.cpp:827`)

### 3.2 dispatch 地点 (`llviewerdisplay.cpp:832`)

main `display()` の **scene 本描画より前**:

```cpp
if (gPipeline.RenderMirrors && !gSnapshot)
{
    gPipeline.mHeroProbeManager.update();
    gPipeline.mHeroProbeManager.renderProbes();
}
```

### 3.3 1 フレームあたりの更新量 (`llheroprobemanager.cpp:265-292`)

- `RenderHeroProbeUpdateRate` で 1/2/3/6 のいずれかの rate
- 1 フレームで `6 / rate` 面更新 (= rate=6 なら 1 面/frame、rate=1 なら 6 面/frame)
- 6 面更新後 `generateRadiance()` で mip + roughness convolution
- update 時は `gCubeSnapshot = true` (`llviewerwindow.cpp:6754`) と `gPipeline.mRT = &mHeroProbeRT` (`llheroprobemanager.cpp:319`) に切替

### 3.4 cubemap 描画で実際に呼ばれるもの

`LLReflectionMap::update` → `gViewerWindow->cubeSnapshot` → `display_cube_face` → 内部で **通常の `display()` 相当の scene render** を 1 face 分回す (camera を 90° 回転 6 方向)。よって:

- **sky / cloud / sun / moon / star / terrain / water 全て probe にも描かれる**
- ただし **cloud は `gCubeSnapshot && !isRadiancePass()` で skip** される ケースあり (`lldrawpoolwlsky.cpp:495`、irradiance map の popping 回避)
- **HDRI sky は probe では強制的に使われる** (`lldrawpoolwlsky.cpp:137`、reflection probe の屋根が HDRI image になる)

### 3.5 注意点 (類似 bug 候補)

water reflection で類似 bug が起きそうな場所は **probe 内の sky depth 扱い**:

- probe 内でも `LLDrawPoolWLSky::endDeferredPass` の `glClear(GL_DEPTH_BUFFER_BIT)` が走る (= sky の depth は probe 内でも捨てる)
- probe の screen output は cubemap face → mip chain → roughness convolution を経由するので、**sky `frag_data[3].a` の値は post-process 経由で probe color に乗る**可能性がある (要確認 — 推測)
- 「FB prim 越しに sky 透け」と同じ機構が **反射映像内でも起きうる**: 水面に映る FB 看板の縁から sky 色が滲む等。**ただし実観測は未確認、本書時点では理論上の警告のみ**

---

## A. Sky 章 (空 / 太陽 / 月 / 星 / 雲は全部 LLDrawPoolWLSky)

### A.1 Sky dome geometry の生成

| 関数/場所 | 役割 |
|---|---|
| `LLVOWLSky::createDrawable` (`llvowlsky.cpp:95`) | `RENDER_TYPE_WL_SKY` 設定 |
| `LLVOWLSky::drawDome` (`llvowlsky.cpp:311`) | sky/cloud 共用 dome geometry を発行 |
| `LLVOWLSky::drawStars` (`llvowlsky.cpp:286`) | star geometry (8000 点程度の billboard) を発行 |
| `LLVOSky::createDrawable` (`llvosky.cpp:842`) | `POOL_SKY` (deprecated stub) に face 登録、FACE_SUN/MOON/BLOOM 用 face を作る |
| `LLDrawPoolWLSky::renderDome` (`lldrawpoolwlsky.cpp:95`) | local camera coord 系で dome を回転・スケール (Y-up に permute) して `gSky.mVOWLSkyp->drawDome()` |

**観察**: sky dome は WL coord 系 (Y up) で描画するため `gGL.rotatef(120°, 1/√3, 1/√3, 1/√3)` で軸 permute する (`lldrawpoolwlsky.cpp:115`)。reflection render 時 `camPosLocal.z > 256` だと z 半分に圧縮 (`lldrawpoolwlsky.cpp:103-105`)。

### A.2 Sky 描画順序 (LLDrawPoolWLSky::renderDeferred、`lldrawpoolwlsky.cpp:471`)

```
1. renderSkyHazeDeferred (sky dome、HDRI or WL)
2. renderHeavenlyBodies   (太陽 → 月)
3. renderStarsDeferred    (星、ただし gCubeSnapshot 時はスキップ)
4. renderSkyCloudsDeferred (雲、ただし gCubeSnapshot && !isRadiancePass 時はスキップ)
```

### A.3 各 shader の depth / blend / gbuffer

| Shader | GL state クラス | depth test | depth write | blend | frag_data[0/1/2/3] | gbuffer flag |
|---|---|---|---|---|---|---|
| `skyF.glsl` (haze) | `LLGLSPipelineDepthTestSkyBox(true, true)` (`lldrawpoolwlsky.cpp:181`) | **ON** | **ON** | OFF | `[0]=vec4(color,1)` (HAS_EMISSIVE 時は `vec4(0)`) / `[1]=0` / `[2]=(0,0,0,SKIP_ATMOS)` (HDRI 時は `HAS_HDRI`) / `[3]=(color, 1.0)` (HAS_EMISSIVE 時、AYAstorm `4188880321` で `0.0`→`1.0` に戻し) | `SKIP_ATMOS` (= 0.0) / `HAS_HDRI` (= 1.0) |
| `cloudsF.glsl` | `LLGLSPipelineBlendSkyBox(true, true)` (`lldrawpoolwlsky.cpp:301`) + `BT_ALPHA` | **ON** | **ON** | **ON (alpha blend)** | `[0]=(color, alpha1)` / `[1]=0` / `[2]=(0,0,0,SKIP_ATMOS)` / `[3]=(color, alpha1)` (HAS_EMISSIVE 時) | `SKIP_ATMOS` |
| `sunDiscF.glsl` | `LLGLSPipelineBlendSkyBox(true, true)` (`lldrawpoolwlsky.cpp:358`) | ON | **ON** | ON (alpha blend) | `[0]=c` / `[1]=0` / `[2]=(0,1,0,SKIP_ATMOS)` / `[3]=(c.rgb, 0.0)` (HAS_EMISSIVE 時、SSS leak 防止で `0.0`) | `SKIP_ATMOS` |
| `moonF.glsl` | 同上 | ON | **ON** (SL-14113 注: moon は depth 書く、stars を月の奥に追いやるため) | ON (alpha blend) | `[0]=(c.rgb, c.a)` / `[1]=0` / `[2]=(0,0,0,SKIP_ATMOS)` / `[3]=(c.rgb, 0.0)` (HAS_EMISSIVE 時) | `SKIP_ATMOS` |
| `starsF.glsl` | `LLGLSPipelineBlendSkyBox(true, false)` + `BT_ADD_WITH_ALPHA` (`lldrawpoolwlsky.cpp:226-228`) | ON | **OFF** | **ON (additive)** | `[0]=col` / `[1]=0` / `[2]=(0,1,0,SKIP_ATMOS)` / `[3]=(col.rgb, 0.0)` (HAS_EMISSIVE 時) | `SKIP_ATMOS` |
| `skyF.glsl` (HDRI 経路、`use_hdri_sky()` true) | 同 haze | ON | ON | OFF | `[2]=(0,0,0,HAS_HDRI)` 以外は haze と同じ | `HAS_HDRI` |

**重要観察 (sky が透ける bug の鍵)**:
1. **sky haze は `frag_data[0]` を `vec4(0)` で書き、実色は `frag_data[3]` (= emissive RT) に乗せる** (`HAS_EMISSIVE` permutation 経路、`skyF.glsl:122-133`)。これは r20 SSS で gbuffer3.a を skin mask に流用するための spec C 策。
2. AYAstorm r30 BD改善期に「skin mask 誤発火」防止で **sky は `frag_data[3].a = 0` を書いていた** が、その後 commit `4188880321` ("Fix sky emissive alpha blending") で `frag_data[3].a = 1.0` に戻された。理由: **gbuffer3 attachment の MRT blend は `.a` を blend factor に使う構成があり、sky pixel が `.a=0` だと後段 FullBright 等の forward additive で sky 色が消滅 → 「FB 越しに sky が透ける」 = 正確には「FB 描画時に sky 色が乗らない」表現として観測**。
3. 同じ 1bit (`gbuffer3.a`) を「MRT blend alpha」と「SSS skin mask」の 2 意味で再利用しているため、片方を立てるともう片方が壊れる構造。本書時点では sky は `.a=1`、SSS 側で「gbuffer flag が SKIP_ATMOS なら skin_mask 無視」の方向で gate する設計 (`cloudsF.glsl:159-163` のコメント参照)。

### A.4 Sky 描画 *直後* の depth clear (= 重要)

`LLDrawPoolWLSky::endDeferredPass` (`lldrawpoolwlsky.cpp:84-93`):

```cpp
void LLDrawPoolWLSky::endDeferredPass(S32 pass)
{
    sky_shader = cloud_shader = sun_shader = moon_shader = nullptr;
    // clear the depth buffer so haze shaders can use unwritten depth as a mask
    glClear(GL_DEPTH_BUFFER_BIT);
}
```

**意味**:
- sky / sun / moon / cloud は depth を書く (table A.3 参照) が、**pool 終端で depth を全消去**
- 以降の opaque pool (simple/bump/material/terrain) は **depth = 1.0 (= 最遠)** の状態から始まる
- atmospherics / haze pass は「depth buffer に書き込まれていない pixel」(= 後続 opaque が描かなかった = sky 領域) を sky としてマスクできる
- これにより `doAtmospherics()` の haze blend は sky 領域以外にしか乗らない

**FB 透け bug が即修正できた理由 (構造観察)**:
- sky の visible color は `frag_data[3]` (emissive RT) に書かれている
- sky の depth は pool 終端で消されるので、FB prim は通常 depth test を pass して sky 領域に書ける
- bug は **「FB prim の forward blend で sky 色 (emissive RT) と合成する際、MRT blend factor `.a` が `0` だと sky 色が乗らない」** という単純な MRT blend 構成問題
- 修正は shader 1 行 (`vec4(color.rgb, 0.0)` → `vec4(color.rgb, 1.0)`)
- これが **「対称な深堀り trace 不要で即修正できた」理由**。一方 SSS 漏れは同 `.a` を SSS mask として 再使用するため修正が分岐する → 前哨戦の bug の方が単純だった

### A.5 WindLight / EEP 設定の uniform 流入経路

| Uniform 名 | source (`lldrawpoolwlsky.cpp`) | 流入 |
|---|---|---|
| `camPosLocal` | line 122 | `renderDome` 内で local camera coord |
| `moisture_level` / `droplet_radius` / `ice_level` | lines 204-206 | `LLEnvironment::instance().getCurrentSky()` (= `LLSettingsSky`) |
| `sun_moon_glow_factor` | line 208, 342 | 同上 |
| `sun_up_factor` | line 210 | `psky->getIsSunUp()` |
| `blend_factor` | lines 268, 340, 407 | sky preset blend (EEP cross-fade) |
| `cloud_variance` | line 341 | preset |
| `aya_r18_cloud_volumetric_enabled` / `aya_r18_strength` | `cloudsF.glsl:44-45` | AYAstorm r18 cvar (`AYAR18CloudVolumetricEnabled` 系)、`mShaderList` 配下なので `mShaderList` walk で自動 push |
| `sky_hdr_scale` | line 172 | `RenderHDRIExposure` (HDRI sky 時のみ) |
| `hdri_split_screen` | line 174 | `RenderHDRISplitScreen` |
| `moon_brightness` / `moonlight_color` / `moon_dir` | lines 453-457 | preset |

### A.6 SSS / FullBright と sky の干渉 (= 本書動機)

| bug | 原因 | 修正 |
|---|---|---|
| **FB 越し sky 透け** (FB prim の forward blend で sky 色が消える) | `skyF.glsl` の `frag_data[3].a = 0.0` (AYAstorm 期に SSS 用に書いていた値) が MRT blend factor として消し色になっていた | `frag_data[3].a = 1.0` に戻し (commit `4188880321`) |
| **FB 越し SSS pink 漏れ** (FB 描画 path の縁から SSS skin tint が滲む) — **本書執筆時点で未修正の bug** | 同じ `gbuffer3.a` を skin_mask に使っているため、sky `.a=1.0` 化で skin_mask が誤発火する側に振れた可能性。または cloud/moon/star の `.a=0` と FB pixel の MRT blend の組み合わせ要因。**要追加調査** | (未) |

**構造的教訓**: `gbuffer3.a` (= emissive RT alpha) は **(1) MRT blend factor として OpenGL に解釈される / (2) SSS shader で skin mask として再 sample される** の 2 用途で取り合いになっている。1bit に 2 意味は持続不可能なので、長期的には **SSS skin mask を `gbuffer3.a` から分離する** ($→$ 別 RT or stencil bit に移す) 設計改修が必要。

### A.7 Sky 関連 shader writer 一覧 (再掲)

`ayastorm-gbuffer3-trace.md` §A から sky 系のみ抽出:

| Shader | `frag_data[3].a` (HAS_EMISSIVE 時) | コメント |
|---|---|---|
| `skyF.glsl` | **`1.0`** (commit `4188880321` 以降) | 元 AYAstorm r30 期は `0.0`、FB 透け bug で revert |
| `cloudsF.glsl` | `alpha1` (cloud 自身の opacity) | r30 P3.8 で `0.0` 試行 → 雲消失 → `alpha1` に戻し (commit `6ae5f9bf1c` / `b2decf0578`) |
| `moonF.glsl` | `0.0` | 月縁の skin_mask 誤発火防止 |
| `starsF.glsl` | `0.0` | 星点の skin_mask 誤発火防止 |
| `sunDiscF.glsl` | `0.0` | 太陽縁の skin_mask 誤発火防止 |

**観察**: sky (`1.0`) と cloud (`alpha1`) **だけ** が「MRT blend alpha としての正しい値」を維持。sun/moon/star は「SSS skin mask gate のために 0」を維持 — 同じ「sky 系」でも 2 派に分かれており、これは SSS と FullBright の取り合いで継続的に振動する可能性がある領域。

---

## B. Cloud 章

### B.1 Cloud は何で描かれているか

| 観点 | 実装 |
|---|---|
| Geometry | `LLVOWLSky::drawDome()` (`llvowlsky.cpp:311`) — sky dome geometry を **そのまま** cloud にも使う (別 mesh ではない) |
| Shader | `cloudsV.glsl` + `cloudsF.glsl` (`gDeferredWLCloudProgram`) |
| Pool | `POOL_WL_SKY` (sky と同 pool) |
| 描画関数 | `LLDrawPoolWLSky::renderSkyCloudsDeferred` (`lldrawpoolwlsky.cpp:290`) |
| 描画順 | renderDeferred 内で **sky haze → 天体 → 星 → 雲** (最後) |
| Volumetric? | **基本 2D noise (`cloud_noise_texture` を sky dome 上に貼る)**。**AYAstorm r18 で slab raymarch 4-step の疑似 volumetric を追加** (`cloudsF.glsl:108-130`、`aya_r18_cloud_volumetric_enabled` cvar で opt-in、`aya_r18_strength` で legacy ↔ volumetric を lerp) |
| Sprite cloud? | (なし、SL 標準には sprite cloud は無い。雲は全て sky dome に焼く 2D noise) |
| Cloud shadow shader? | (個別 shader 無し、地上への shadow 投影は sun shadow map 経由で間接的) |

### B.2 Cloud の blend / depth

(table A.3 再掲)
- depth test ON / depth write **ON** (`LLGLSPipelineBlendSkyBox(true, true)`)
- blend ON (`BT_ALPHA`)
- AYAstorm r18 volumetric は **shader 内部処理のみ**、render state は変えない

### B.3 Reflection probe 時の cloud 抑制

`lldrawpoolwlsky.cpp:495`:

```cpp
if (!gCubeSnapshot || gPipeline.mReflectionMapManager.isRadiancePass())
{
    renderSkyCloudsDeferred(origin, camHeightLocal, cloud_shader);
}
```

**意味**: irradiance pass (= reflection probe の diffuse 用 1 段目) では雲を描かない (popping 回避)。radiance pass (= specular 用 2 段目) では描く。

---

## C. Water 章

### C.1 関連 class

| Class | 役割 |
|---|---|
| `LLDrawPoolWater` (`lldrawpoolwater.cpp`) | POOL_WATER の本体。**renderPostDeferred のみ実装**、renderDeferred は無 |
| `LLDrawPoolWaterExclusion` (`lldrawpoolwaterexclusion.cpp`) | POOL_WATEREXCLUSION、water plane の depth-only mask 生成 |
| `LLVOWater` | region water (各 SIM の 256x256 water plane) |
| `LLVOVoidWater` (`mIsEdgePatch=true`) | void water (region 外の edge patch) |

旧 `LLDrawPoolAlphaWater` は code から消えており、本書時点では `POOL_ALPHA_PRE_WATER` / `POOL_ALPHA_POST_WATER` という **alpha pool の前後配置** で「水の手前/奥」を分ける構成に移行済。

### C.2 Water shader 全列挙

| Shader | bind 先 | 用途 |
|---|---|---|
| `class3/environment/waterF.glsl` | `gWaterProgram` | 通常の water surface (camera 水面上) |
| `class3/environment/underWaterF.glsl` | `gUnderWaterProgram` | camera が水中時の water surface (見上げる側) |
| `class1/environment/waterV.glsl` | 共用 vertex shader | |
| `class1/environment/waterFogF.glsl` | (forward 物体に link、water fog 計算 helper) | water に沈んでいる物体への水中 fog 適用 |
| `class3/deferred/waterHazeF.glsl` + `waterHazeV.glsl` | `gHazeWaterProgram` | water 上 fog 描画 (`doWaterHaze` 経由)、water surface 形状で fog を焼く |
| `gWaterEdgeProgram` (※ 過去版に存在、現在は `gWaterProgram` 統合) | — | edge patch 専用 — 2025-02-11 Geenz により region water と統合され削除 |

**permutation**:
- `TRANSPARENT_WATER` (1) — `sRenderTransparentWater` ON 時、`screenTex` + `depthMap` 経由で refraction
- `HAS_SUN_SHADOW` (1) — sun shadow map 有効時

### C.3 Reflection RT / Refraction RT

#### 現代版 (2026-05-25 時点)

| RT | サイズ | 用途 | 生成 |
|---|---|---|---|
| **`mWaterDis`** (`pipeline.h:987`) | 画面 resolution | **refraction 入力** (= water 越しの色 + depth) | `pipeline.cpp:1230` で確保。毎フレーム `LLDrawPoolWater::beginPostDeferredPass` (`lldrawpoolwater.cpp:111-140`) で screen color + deferred depth を copy。**transparent water OFF 時は scratch space としてのみ確保** |
| **`mWaterExclusionMask`** | (要確認、おそらく画面 resolution) | water plane の mask (内側=反射対象 / 外側=透過対象) | `doWaterExclusionMask` (`pipeline.cpp:12007-12016`) で `gDrawColorProgram` 使って water plane を白で塗る |
| **`mHeroProbeRT.screen / .deferredScreen / .deferredLight`** | `RenderHeroReflectionProbeResolution` (デフォルト 512?) | **水面 reflection probe の per-face render target** | `pipeline.cpp:1062-1064` で `RenderMirrors` ON 時のみ。Hero probe update 時に `gPipeline.mRT = &mHeroProbeRT` に hot-swap (`llheroprobemanager.cpp:319`) |
| **Hero probe cubemap (`mProbes[0]->mCubeArray`)** | probe resolution の cubemap | 水面に映る 360° 環境マップ | 6 面分の `LLReflectionMap::update` 後 `generateRadiance` で mip chain + roughness convolution |

#### 旧版 (削除済)

- `mWaterRef` (planar mirror flip reflection texture) — **本書時点で code に存在しない**。Hero probe に置き換え済

### C.4 Planar reflection pass のトレース

**「水面に映る空 / アバター / 装飾物」がどの pool で描かれているか**:

1. `display()` 冒頭で `gPipeline.mHeroProbeManager.renderProbes()` (`llviewerdisplay.cpp:832`)
2. → `LLHeroProbeManager::renderProbes()` (`llheroprobemanager.cpp:242`) — 6 面分 (1/2/3/6 face/frame)
3. → `LLHeroProbeManager::updateProbeFace` (`llheroprobemanager.cpp:313`)
4. → `LLReflectionMap::update` (`llreflectionmap.cpp:52`)
5. → `gViewerWindow->cubeSnapshot` (`llviewerwindow.cpp:6640`)
6. → `gCubeSnapshot = true` (`llviewerwindow.cpp:6754`)
7. → `display_cube_face()` — **通常 `display()` 相当の scene render を 1 face 分**
8. 通常 scene render なので **sky / cloud / sun / moon / star / terrain / opaque / alpha / 全部** が cubemap face に焼かれる
9. ただし `gCubeSnapshot` を見て一部 pool が skip:
   - `LLDrawPoolWLSky::renderDeferred` (`lldrawpoolwlsky.cpp:490-492`) — `gCubeSnapshot && !isRadiancePass` なら星 / 雲 skip
   - `LLDrawPoolWater::renderPostDeferred` — water 自体は cubemap 内で再帰描画されない (要確認、ここは推測)
   - HUD / particles / 一部 debug は probe では off

### C.5 Water surface の depth / blend / gbuffer

`LLDrawPoolWater::renderPostDeferred` (`lldrawpoolwater.cpp:142-320`):

| 項目 | 値 |
|---|---|
| Pool | `POOL_WATER` (= 19、POST 経路で描画) |
| Shader | `gWaterProgram` (camera 水面上) / `gUnderWaterProgram` (camera 水中) |
| Output | `frag_color` (= single MRT、forward 出力。**gbuffer 書かない**) |
| Depth test | ON (sUnderWaterRender ? GL_FALSE : GL_TRUE で water plane geometry を描画) |
| Depth write | (要確認、`bindDeferredShader` 経由なので shader-side で `gl_FragDepth` 書かない) |
| Blend | **OFF** (`LLGLDisable blend(GL_BLEND);` `lldrawpoolwater.cpp:145`) — water 自体は forward だが、`frag_color` 直接書き込みで scene color を上書き |
| Cull | OFF (`LLGLDisable cullface(GL_CULL_FACE);` line 306) — water plane は両面描画 |
| Gbuffer 書き込み | **無し** (= forward path、SSAO/SSR/SSS post 全部対象外) |

**観察**: water surface は **opaque 扱いで forward** 描画。深さ書き込みはあるが gbuffer は無いので、water 越しに見える物体 (refraction) は `mWaterDis` から sample する側で完結している。

### C.6 Water reflection RT で類似 bug 起きそうな注意点

**注意 1 個: 「水面に映る FB prim の縁から sky 色が滲む」可能性**。

理由:
1. Hero probe cubemap render は **通常の scene 描画 path をそのまま走らせる**ので、§A.4 で記述した「sky 後 depth clear → FB prim が forward 合成」path も probe 内で走る
2. `gbuffer3.a` 経由の sky color leak (§A.6) も probe 内で同様に起きる
3. probe の出力は mip chain → roughness convolution で **平均化されるので軽症化される**が、低 roughness (= 鏡面に近い水面) では平均が効かず元の bug を保存する
4. 修正は probe 内でも有効 (shader 共通) なので「sky の `.a=1.0`」化で同時に治っている **はず** だが、SSS leak と FB 縁の干渉は probe 経由でも残っている可能性あり — 本書時点で **実観測未確認、潜在リスクとして列挙**

---

## D. Terrain 章

### D.1 Class / Pool

| Class | 役割 |
|---|---|
| `LLVOSurfacePatch` (`llvosurfacepatch.cpp`) | 地形 patch (各 region を 16x16 程度に分割) を持つ VO |
| `LLDrawPoolTerrain` (`lldrawpoolterrain.cpp`) | POOL_TERRAIN、deferred 描画 + shadow + motion blur 担当 |
| `LLVLComposition` | region の terrain composition (4 texture + heightmap blending 設定) |

### D.2 Shader

| Mode | Shader | bind 先 |
|---|---|---|
| **Legacy 4-texture splat** (`getMaterialType() == TEXTURE`) | `class1/deferred/terrainF.glsl` (+ `class1/deferred/terrainV.glsl`) | `gDeferredTerrainProgram` |
| **PBR terrain** (`getMaterialType() == PBR_MATERIAL`、paint_type 別) | `class1/deferred/pbrterrainF.glsl` (+ `pbrterrainUtilF.glsl`) | `gDeferredPBRTerrainProgram[paint_type]` (paint_type 0〜TERRAIN_PAINT_TYPE_COUNT-1) |

`renderFullShader` (`lldrawpoolterrain.cpp:267-291`) で分岐。

### D.3 Detail / Heightmap blending

`terrainF.glsl:44-69` (legacy):

```glsl
vec4 color0 = texture(detail_0, vary_texcoord0.xy);
vec4 color1 = texture(detail_1, vary_texcoord0.xy);
vec4 color2 = texture(detail_2, vary_texcoord0.xy);
vec4 color3 = texture(detail_3, vary_texcoord0.xy);

float alpha1 = texture(alpha_ramp, vary_texcoord0.zw).a;
float alpha2 = texture(alpha_ramp, vary_texcoord1.xy).a;
float alphaFinal = texture(alpha_ramp, vary_texcoord1.zw).a;
vec4 outColor = mix( mix(color3, color2, alpha2), mix(color1, color0, alpha1), alphaFinal );

outColor.a = 0.0; // yes, downstream atmospherics
```

- `detail_0/1/2/3` = 4 つの detail texture (region property、低/中/高/最高高度に割当)
- `alpha_ramp` = heightmap 由来の blend mask
- **`outColor.a = 0.0`** に注目 — terrain は alpha 0 で `frag_data[0]` を書く (= alpha channel を haze pass の通常 atmospherics gate に使うため)

### D.4 Depth / blend / gbuffer

| 項目 | 値 |
|---|---|
| Pool | `POOL_TERRAIN` (= 8、renderDeferred で描画) |
| Shader | `gDeferredTerrainProgram` / `gDeferredPBRTerrainProgram[paint_type]` |
| Depth test | ON (deferred 標準) |
| Depth write | **ON** (普通の opaque) |
| Blend | OFF (普通の opaque) |
| Cull | ON |
| Gbuffer write | **`[0]=outColor(rgb,0)` / `[1]=(0,0,0,-1)` / `[2]=encodeNormal(n,0,HAS_ATMOS)` / `[3]=vec4(0)` (HAS_EMISSIVE 時)** |
| Gbuffer flag | `HAS_ATMOS` (= 0.34) — softenLightF Legacy 分岐へ (legacy terrain)、PBR 版は `HAS_PBR` (= 0.67) |
| Shadow pass | あり (`renderShadow` `lldrawpoolterrain.cpp:189-200`、`gDeferredShadowProgram` で depth-only) |
| Motion blur pass | あり (r30 P2 AYAstorm 追加、`renderMotionBlur` `lldrawpoolterrain.cpp:227-246`、`gVelocityProgram`) |
| Mirror clip | あり (`mirrorClip(pos)` 呼出、`terrainF.glsl:46`) |
| `mirrorClip` の意味 | hero probe pass 時に water plane で discard、water に terrain が二重に映らないようにする |

### D.5 Terrain と装着物 canary

**terrain は LLDrawInfo を経由しない (= `mAttachedToAvatar` flag を持たない)**。`LLDrawPoolTerrain::renderFullShaderTextures` (`lldrawpoolterrain.cpp:293`) 以下は `mDrawFace` を直接 walk して `facep->renderIndexed()` するため、装着物資料 / Rez Object 資料の canary uniform は **terrain には届かない**。

**結論**: terrain を識別したい (例: 環境 effect から terrain だけ除外したい / terrain だけに lit を変えたい) なら **`mAttachedToAvatar` flag 経由ではなく**、shader bind 時に shader 全体に 1 度 `aya_environment_canary = TERRAIN` を流す方式が必要 (tree pool と同じ pattern、`ayastorm-rez-object-rendering-routing.md` §3.3 を参照)。

---

## 4. C++ shader bind 地点の主要ファイル (sky/water/terrain)

| ファイル | 関数 | 行 | 役割 |
|---|---|---|---|
| `lldrawpoolwlsky.cpp` | `LLDrawPoolWLSky::renderDeferred` | 471 | WL sky + cloud + sun + moon + star 全体 dispatch |
| `lldrawpoolwlsky.cpp` | `renderSkyHazeDeferred` | 143 | sky dome (HDRI or WL) |
| `lldrawpoolwlsky.cpp` | `renderSkyCloudsDeferred` | 290 | cloud (gCubeSnapshot で skip 制御あり) |
| `lldrawpoolwlsky.cpp` | `renderHeavenlyBodies` | 354 | sun + moon |
| `lldrawpoolwlsky.cpp` | `renderStarsDeferred` | 219 | star (gCubeSnapshot で skip) |
| `lldrawpoolwlsky.cpp` | `endDeferredPass` | 84 | **`glClear(GL_DEPTH_BUFFER_BIT)`** — sky pass 終端で depth 全消去 (FB 越し sky 表現の前提) |
| `lldrawpoolwater.cpp` | `LLDrawPoolWater::renderPostDeferred` | 142 | water surface 本描画 |
| `lldrawpoolwater.cpp` | `beginPostDeferredPass` | 111 | `mWaterDis` への screen color + depth copy |
| `lldrawpoolwater.cpp` | `pushWaterPlanes` | 322 | water plane geometry 発行 (water exclusion からも共有呼出) |
| `lldrawpoolwaterexclusion.cpp` | `LLDrawPoolWaterExclusion::render` | 43 | water plane mask 生成 |
| `lldrawpoolterrain.cpp` | `LLDrawPoolTerrain::renderDeferred` | 147 | terrain dispatch (legacy or PBR) |
| `lldrawpoolterrain.cpp` | `renderFullShader` | 267 | shader 切替 (`gDeferredTerrainProgram` / `gDeferredPBRTerrainProgram`) |
| `lldrawpoolterrain.cpp` | `renderShadow` | 189 | terrain shadow caster |
| `pipeline.cpp` | `LLPipeline::renderGeomDeferred` | 5039 | 全 pool walk (POOL_WL_SKY / POOL_TERRAIN を deferred で描画) |
| `pipeline.cpp` | `LLPipeline::renderGeomPostDeferred` | 5178 | POOL_WATER の renderPostDeferred、間に `doAtmospherics` / `doWaterHaze` / `doWaterExclusionMask` を挟む |
| `pipeline.cpp` | `doAtmospherics` | 11641 | screen-space atmospheric haze (depth を `mWaterDis` 経由で gHazeProgram に渡す) |
| `pipeline.cpp` | `doWaterHaze` | 11923 | water 形状 fog (`gHazeWaterProgram`) |
| `pipeline.cpp` | `doWaterExclusionMask` | 12007 | water plane mask (`mWaterExclusionMask` RT) |
| `pipeline.cpp` | `markVisible` | 3081-3098 | sky / WL sky の drawable を毎フレーム cull pass に push |
| `llviewerwindow.cpp` | `cubeSnapshot` | 6640 | `gCubeSnapshot = true` で probe cubemap 1 面分の scene render |
| `llheroprobemanager.cpp` | `LLHeroProbeManager::renderProbes` | 242 | 水面 reflection probe の 6 面更新 dispatcher |
| `llheroprobemanager.cpp` | `updateProbeFace` | 313 | 1 面分の render + mip chain |
| `llviewershadermgr.cpp` | `loadShadersDeferred` | 3179-3265 | `gDeferredWLSkyProgram` / `gDeferredWLCloudProgram` / `gDeferredWLSunProgram` / `gDeferredWLMoonProgram` / `gDeferredStarProgram` の shader file load |
| `llviewershadermgr.cpp` | `loadShadersWater` | 1008-1052 | `gWaterProgram` / `gUnderWaterProgram` の shader file load |

---

## 5. 推測でしか埋められなかった項目 (実コード確認が後追いで必要)

本書執筆時に実コード確認できず、参考 grep + 既存資料の知識から推測で書いた箇所:

1. **§3.5 / §C.6 「water reflection (hero probe) 内でも FB 越し sky leak / SSS pink leak が起きうる」**: 同じ shader path を通るので**論理的には起きるはず**だが、probe の mip chain + roughness convolution で軽症化される / されないかは実観測していない。要 canary 検証。
2. **§C.5 water surface の `gl_FragDepth` write 有無**: `waterF.glsl` を grep した範囲では `gl_FragDepth` 書込みは無さそうだが、water plane geometry 描画時に GL 標準の depth write が ON / OFF どちらで bind されているかは `bindDeferredShader` の内部状態に依存。詳細追跡未完。
3. **§C.4 probe 内で water 自体が再帰描画されるか**: `display_cube_face()` の実装まで降りていないので、water pool が probe pass 中に skip されるかどうかは未確認 (実装的にはする方が自然 — 水面の中に水面が映ったら無限再帰)。
4. **§C.3 `mWaterExclusionMask` の解像度・format**: `pipeline.h` まで grep してないので画面 resolution / format は推測。
5. **§B.1 「sprite cloud は SL に存在しない」**: 一般的な SL の知識として書いたが、最新の EEP に sprite cloud が追加されていないかの完全確認は未実施。
6. **§A.6 「FB 越し SSS pink 漏れ bug」の正確な再現条件**: 本書ではユーザー指摘の bug として記述したが、commit 検索 (`grep -i sss.*pink`) で直接 hit する commit を見つけられなかった。記述は概念モデル止まり。

---

## 6. 関連リファレンス

- `ayastorm-deferred-shader-routing.md` — gbuffer flag → softenLightF 分岐 (sky の `SKIP_ATMOS` / `HAS_HDRI` 分岐含む、§1 参照)
- `ayastorm-gbuffer3-trace.md` — `frag_data[3]` storage 仕様 (sky/cloud/moon/star/sunDisc の `.a` write 値一覧含む)
- `ayastorm-attachment-rendering-routing.md` — 装着物 (`mAttachedToAvatar.notNull()`) 軸の地図、本書と直交する effect 軸の対称資料
- `ayastorm-rez-object-rendering-routing.md` — Rez Object (`mAttachedToAvatar.isNull()` + 非環境) 軸の地図、§0「Rez Object に含まれないもの」が本書 scope と一致

---

## 7. 更新履歴

- 2026-05-25 初版: effect 軸地図シリーズ 4 本目。sky/cloud/water/terrain の pool / shader / depth / blend / gbuffer / 描画順序 / Hero probe との関係を確定マップ化。FB 越し sky 透け bug (commit `4188880321`) と SSS pink 漏れの構造的位置を §A.6 で示し、共通根 (`gbuffer3.a` の 1bit 2 意味化) を identify。
