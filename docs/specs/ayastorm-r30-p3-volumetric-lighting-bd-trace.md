# AYAstorm r30 P3 — Volumetric Lighting 取り込み 事前依存マップ (BD trace)

**作成日**: 2026-05-18
**最終更新**: 2026-05-18 (Tier-1 trace 完了、§5 改修項目 / §6 実装ステップ案 策定)
**親 spec**: `ayastorm-r30-cinematic-chapter.md` §3 P3 / §4.2 取り込みリスト
**前段 spec**: `ayastorm-r30-p2-velocity-buffer-bd-trace.md` (P2 = velocity buffer + SMAA T2x + motion blur composite ship 済)
**スコープ**: P3 着手前の事前 trace。BD repo 内の Volumetric Lighting 実装 (2 shader + C++ pipeline plumbing + forward pass 副作用 + cvar 5 件) を上から下まで追い、AYAstorm 側に取り込む際の改修ポイントを file:line 単位で確定する。**実装は含まない**。
**BD 参照 commit**: `995a1354d8` (Version to 5.6.2, 2026-04-19) — P2 と同じ参照点
**Firestorm 参照ブランチ**: `ayastorm-release` HEAD = P2 ship 後 (branch `feature/ayastorm-r30-p2-velocity-buffer-spec` `2670934d28`)

---

## 1. 概要

### 1.1 P3 スコープ

P3 は Cinematic mode に「光が空間を満たす」感を持ち込む phase。具体的な取り込み対象は:

- Volumetric Light 2 shader (BD borrow): `class1/deferred/volumetricLightF.glsl` (stub) + `class3/deferred/volumetricLightF.glsl` (本体)
- VS は LL 既存 `deferred/postDeferredNoTCV.glsl` を流用 (新規取り込みなし)
- `LLPipeline::renderVolumetric()` の新規追加 + `renderFinalize()` 内呼び出し hook (tonemap 後 / combineGlow 前)
- 新規 cvar 5 件 (Cinematic 専用 namespace で `RenderVolumetricLighting*` 系)
- Cinematic (`AYAVisualRealismEnabled == 2`) ガード位置
- `lldrawpoolalpha.cpp` の forward pass 拡張 (alpha 物体の depth 書き出し条件) — 要否は §5.4 で判定
- `llshadermgr.{h,cpp}` の uniform enum + 文字列追加 (`GODRAY_RES` / `GODRAY_MULTIPLIER` / `FALLOFF_MULTIPLIER`)
- `gDeferredSoftenProgram` の `GODRAYS_FADE` permutation 取付先疑義 (BD バグ修正) — §5.1 で対処

スコープ外:
- BD UI (`panel_preferences_graphics1.xml` の volumetric light UI) — `project_ayastorm_r30_cinematic_chapter.md`「BD UI は不取り込み」方針通り
- 既存 atmosphere pipeline (skyF / atmosFragLighting) への手入れ — P3 では純粋に post-process pass として追加、原色作りは不変
- AYAstorm View / Firestorm View への default ON 化 — Cinematic mode 内でのみ有効化

### 1.2 本 spec の位置づけ

P3 着手時に作業者 (AYA さん / Claude) が「BD のどこを見ればよいか」「Firestorm のどこに何を入れるか」を spec 1 本で把握できる状態にする。trace は §3 で完了済、§5 改修項目・§6 実装ステップ案までセット。§7 未確定事項は P3 着手時に再 fetch / 再確認が必要。

---

## 2. 2 shader ファイルの内容要約

Volumetric Light は **godrays (太陽方向に向かう光線の shadow 積算)** を post-process pass で計算し、tonemap 後の color buffer に加算する方式。Linden Lab 標準の class1 / class3 階層化を踏襲、低 shader level では stub (passthrough) が選ばれる。

### 2.1 class1/deferred/volumetricLightF.glsl (stub)

低 shader level 用の no-op。フル本体:

```glsl
out vec4 frag_color;
uniform sampler2D diffuseRect;
in vec2 vary_fragcoord;

void main() {
    vec2 tc = vary_fragcoord.xy;
    vec4 diff = texture2D(diffuseRect, tc);
    frag_color = diff;
}
```

`diffuseRect` をそのまま `frag_color` に流すだけ。godrays 計算は走らず、cost ゼロ。class3 が disable される環境 (古い GPU / shader level 不足) で安全に compile が通る役割。**AYAstorm 取り込み時もこの stub を class1 側に必ず置く** (Cinematic mode は最新世代しか対象としないため理論上 class3 のみで足りるが、LL viewer 既存の階層化規約に従う = 後段で「class3 だけ落とす」kill-switch を入れる余地を残せる)。

### 2.2 class3/deferred/volumetricLightF.glsl (本体)

godrays + shadow 積算 + diffuseRect 加算。要点:

| Item | 内容 |
|---|---|
| Input texture | `diffuseRect` (tonemap 後の color)、`depthMap` (depth buffer) |
| Atmosphere uniform | `blue_density` (vec3)、`haze_density` (float)、`sunlight_color` (vec3)、`sun_dir` (vec3) — LL atmosphere pipeline 流用 |
| Volumetric uniform | `godray_res` (int)、`godray_multiplier` (float)、`falloff_multiplier` (float)、`screen_res` (vec2) |
| Misc uniform | `seconds60` (float、現状未使用) |
| Forward decl | `nonpcfShadowAtPos(vec4 pos_world, vec2 pos_screen)`、`getPosition(vec2 pos_screen)` — `bindDeferredShader` 経由で LL shadow helper / position recover helper が link される |
| Permutation | `#if GODRAYS_FADE` で sun-facing 時に強度フェード (sun_dir.z < 0 の時のみ計算、太陽が地平線下なら 0) |

### 2.2.1 ロジック要約 (本体 frag)

```
1. vary_fragcoord から tc を得て diffuseRect / depthMap をサンプル
2. depth *= pow(depth, 100.0) — 遠方を強調する非線形 weight
3. haze_weight = haze_density / (blue_density + haze_density) — atmosphere の重み
4. farpos = pos.xyz * (min(-pos.z, 512.0) / -pos.z) — 512m 上限で「sample 終端 = 太陽までの代理」
5. for (i = godray_res-1; i > 0; --i):
     spos = mix(0, farpos, (i - roffset) / godray_res)
     shadsample = 0.275 * nonpcfShadowAtPos(spos, tc)
     shaftify += 0.15 * |shadsample + last_shadsample| / i
     shadamount += shadsample * i  // 遠方の sample を重く
6. shadamount /= godray_res; shaftify /= godray_res
7. shadamount *= clamp(depth, 0, 0.5)
8. fade = max(falloff_multiplier / depth, 1.0)
9. shaftify = (shaftify / fade) * godray_multiplier
10. (#if GODRAYS_FADE) shaftify *= clamp(1 - dot(sun_dir.xy * 1.2, sun_dir.xy * 1.8), 0, 1)
11. diff.rgb += (shaftify * haze_weight.x * shadamount) * sunlight_color
12. frag_color = diff
```

**最終出力は diffuseRect への加算結果** (passthrough + godrays)、output format は post-process chain と同じ (BD では `mPostPingMap` の format)。`frag_color` 1 本、MRT 不使用。

### 2.3 vertex shader: postDeferredNoTCV.glsl (LL 既存、新規取り込みなし)

両 fragment は LL 既存の **fullscreen quad VS** (`deferred/postDeferredNoTCV.glsl`) と組む。`vary_fragcoord` を出すだけの最小 VS で、Firestorm 既存。**新規取り込みファイルは fragment 2 本のみ**。

### 2.4 出力 format / blend モード

- bind 対象 RT: BD では `dst` (= `mPostPingMap`) を bind、`gGL.setColorMask(true, false)` で RG 書き込み有効 (色は RGB だがそのまま書く)、`bindTexture(DEFERRED_DIFFUSE, dst, ...)` で **自分自身を sample** → §5.2 で in-place hazard として扱う
- blend は無し (passthrough + 加算を shader 内で実行)
- depth test / mask: post chain なので depth test は `LLGLDepthTest depth(GL_FALSE)` 相当 (`renderFinalize()` 内 line 8511 で `LLGLDepthTest depth(GL_FALSE)` が立っているのを継承)

---

## 3. BD pipeline での呼び出しトレース (C++ 側)

### 3.1 cvar 定義 — `BD app_settings/settings_blackdragon.xml:1283-1346`

BD は cvar を `settings_blackdragon.xml` (BD 独自 settings file) に集約。AYAstorm では Firestorm 標準 `settings.xml` に統合する (BD UI を取り込まないので独立 file を別途読む必要がない)。

| Cvar | 型 | BD default | 用途 |
|---|---|---|---|
| `RenderVolumetricLighting` | BOOL | `1` (ON) | master enable |
| `RenderVolumetricLightingResolution` | U32 | `1` | godray sample 段数 (godray_res uniform) |
| `RenderVolumetricLightingMultiplier` | F32 | `1` | 強度 (godray_multiplier uniform) |
| `RenderVolumetricLightingFalloffMultiplier` | F32 | `1` | 距離減衰 (falloff_multiplier uniform) |
| `RenderVolumetricLightingDirectional` | BOOL | `1` | 太陽向き fade (GODRAYS_FADE permutation) |

`RenderVolumetricLightingResolution = 1` は「シーン中 1 ピクセルにつき godray sample 数 1」を意味するが、本体 shader の for ループは `godray_res - 1` 回回るため res=1 では 1 回も回らない (`i = godray_res - 1 = 0` で初回 condition `i > 0` 不成立)。**BD の出荷 default 値で実質 godrays が無効化されている疑い**。§7 で再確認対象とする (AYAstorm 取り込み時の default は §5.3 で再決定)。

### 3.2 pipeline.cpp 内 plumbing

#### 3.2.1 static 宣言 — `BD pipeline.cpp:250-253`

```cpp
//BD - Volumetric Lighting
bool LLPipeline::RenderVolumetricLighting;
U32 LLPipeline::RenderVolumetricLightingResolution;
F32 LLPipeline::RenderVolumetricLightingMultiplier;
F32 LLPipeline::RenderVolumetricLightingFalloffMultiplier;
```

(`RenderVolumetricLightingDirectional` は static にはなく、`gSavedSettings.getBOOL()` を `llviewershadermgr.cpp:2937` で直接読む — shader rebuild が必要なので per-frame cache 不要)

#### 3.2.2 connectRefreshCachedSettingsSafe — `BD pipeline.cpp:668-672`

```cpp
//	//BD - Volumetric Lighting
connectRefreshCachedSettingsSafe("RenderVolumetricLighting");
connectRefreshCachedSettingsSafe("RenderVolumetricLightingResolution");
connectRefreshCachedSettingsSafe("RenderVolumetricLightingMultiplier");
connectRefreshCachedSettingsSafe("RenderVolumetricLightingFalloffMultiplier");
```

cvar 変更 → `refreshCachedSettings()` 自動呼び出しの signal 配線。`RenderVolumetricLightingDirectional` は含まれない (shader rebuild 経路、`handleShaderEnabled` 等で別途扱う必要あり — §7 で確認)。

#### 3.2.3 refreshCachedSettings — `BD pipeline.cpp:1302-1306`

```cpp
//	//BD - Volumetric Lighting
RenderVolumetricLighting = gSavedSettings.getBOOL("RenderVolumetricLighting");
RenderVolumetricLightingResolution = gSavedSettings.getU32("RenderVolumetricLightingResolution");
RenderVolumetricLightingMultiplier = gSavedSettings.getF32("RenderVolumetricLightingMultiplier");
RenderVolumetricLightingFalloffMultiplier = gSavedSettings.getF32("RenderVolumetricLightingFalloffMultiplier");
```

### 3.3 描画関数 — `BD pipeline.cpp:8469-8497`

```cpp
//BD - Volumetric Lighting
void LLPipeline::renderVolumetric(LLRenderTarget* src, LLRenderTarget* dst)
{
//	//BD - Volumetric Lighting
	if (RenderVolumetricLighting)
	{
		dst->bindTarget();
        glViewport(0, 0, dst->getWidth(), dst->getHeight());

        gGL.setColorMask(true, false);

        bindDeferredShader(gVolumetricLightProgram);
		gVolumetricLightProgram.bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, dst, LLTexUnit::TFO_POINT);

		gVolumetricLightProgram.uniform2f(LLShaderMgr::DEFERRED_SCREEN_RES, (GLfloat)src->getWidth(), (GLfloat)src->getHeight());
        gVolumetricLightProgram.uniform1i(LLShaderMgr::GODRAY_RES, RenderVolumetricLightingResolution);
        gVolumetricLightProgram.uniform1f(LLShaderMgr::GODRAY_MULTIPLIER, RenderVolumetricLightingMultiplier);
        gVolumetricLightProgram.uniform1f(LLShaderMgr::FALLOFF_MULTIPLIER, RenderVolumetricLightingFalloffMultiplier);

		mScreenTriangleVB->setBuffer();
		mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

		gVolumetricLightProgram.unbind();
        unbindDeferredShader(gVolumetricLightProgram);
		dst->flush();

        gGL.setColorMask(true, true);
	}
}
```

**最重要点 — in-place 書き込み**: 呼び出し側 (3.4 参照) は `renderVolumetric(sourceBuffer, sourceBuffer)` と src/dst を **同一バッファ** で渡す。本体内では `dst->bindTarget()` + `bindTexture(DEFERRED_DIFFUSE, dst, ...)` で **自分自身を sample しながら自分自身に書く**。

OpenGL 仕様としては undefined behavior に近い (FBO の color attachment を texture として sample しながら同 FBO に write、`GL_EXT_texture_barrier` などの barrier なしでは driver 依存)。BD で動いている理由は:
- 同一 fragment の同一 texel しか read/write しない (`diff = texture(diffuseRect, tc)` → `frag_color = diff + add`)
- 多くの driver で「同 texel の read-then-write」はキャッシュ的に通る (Vulkan の subpassLoad に近い semantics)

ただし AYAstorm では P2 で確立した `motionBlurComposite` (src→dst 別バッファ + swap) と整合性を取る価値あり → §5.2 で「in-place 維持」or「pong 化」を判定。

`shadow uniform` (`shadowMap[0-3]`, `shadowMatrix[0-3]`, etc.) は **`bindDeferredShader(gVolumetricLightProgram)` 内で自動 bind** される (LL 共通機構、register feature flag `hasShadows = true` の効果)。`sun_dir` / `blue_density` / `haze_density` / `sunlight_color` も同様に `calculatesAtmospherics = true` / `hasAtmospherics = true` で atmosphere uniform が自動 bind される。**個別の uniform1f / uniform3fv 呼び出しは godrays 4 値だけ**で済む。

### 3.4 renderFinalize() 内の呼び出し position — `BD pipeline.cpp:8499-8567`

```
renderFinalize()
  ├─ enableLightsFullbright()
  ├─ setColorMask(true, true) / glClearColor(0,0,0,0)
  ├─ if (hdr):
  │    ├─ generateLuminance(&mRT->screen, &mLuminanceMap)
  │    ├─ generateExposure(&mLuminanceMap, &mExposureMap)
  │    ├─ tonemap(&mRT->screen, apply_cas ? &mRT->deferredLight : &mPostPingMap, !apply_cas)
  │    └─ if (apply_cas): applyCAS(&mRT->deferredLight, &mPostPingMap)
  │  else: gammaCorrect(&mRT->screen, &mPostPingMap)
  ├─ generateGlow(&mPostPingMap)
  ├─ sourceBuffer = &mPostPingMap; targetBuffer = &mPostPongMap
  ├─ renderVolumetric(sourceBuffer, sourceBuffer)   ← in-place、swap なし
  ├─ combineGlow(sourceBuffer, targetBuffer); swap
  ├─ if (RenderMotionBlur && !gCubeSnapshot):
  │    renderMotionBlurComposite(sourceBuffer, targetBuffer); swap
  └─ ... viewport set / DoF / SMAA / 残り
```

**位置選定の根拠**:
- HDR/SDR どちらの場合も呼ばれる (tonemap or gammaCorrect 直後の `mPostPingMap` に対して動作)
- **glow 合成より前** — godrays は光源由来の付加要素なので、glow と混ぜる前に diffuse に乗せておく (glow が godrays に被ると物理的に自然)
- **motion blur composite より前** — motion blur は最終色を smear するべきなので、godrays を含めた状態から smear させる
- **DoF より前** — godrays は最終色なので DoF の depth-aware blur 対象として扱われる

### 3.5 lldrawpoolalpha.cpp 内 forward pass 影響 — `BD lldrawpoolalpha.cpp:210-226`

```cpp
// final pass, render to depth for depth of field effects
if (!LLPipeline::sImpostorRender
	//BD - Volumetric Lighting
	&& (gPipeline.RenderDepthOfField
	|| gPipeline.RenderVolumetricLighting)
	&& !gCubeSnapshot
	&& !LLPipeline::sRenderingHUDs
	... )
{
	simple_shader->bind();
	/BD - TODO: Optimize
	if ((gPipeline.RenderDepthOfField || gPipeline.RenderVolumetricLighting)
		&& gSavedSettings.getBOOL("RenderDepthOfFieldAlphas"))
		simple_shader->setMinimumAlpha(0.7f);
	...
}
```

**意味**:
- alpha pass で「alpha 物体を depth に書き込むか」を判定する `if` 条件に `RenderVolumetricLighting` を `RenderDepthOfField` と OR で混ぜている
- `RenderDepthOfFieldAlphas` ON 時に **minimum alpha を 0.7 に押し上げ** て alpha 物体を depth に固める (透ける物体が DoF/godrays で「向こうが見えない」現象への対処、半透明をある程度不透明扱いで depth に出す)
- これは forward pass 描画への副作用 → 取り込み時は **既存 AYAstorm View / Firestorm View 挙動を変えないように Cinematic gate で囲う必要あり**

**AYAstorm 取り込み時の課題**:
- BD は `RenderVolumetricLighting` BOOL 単体で gate しているが、AYAstorm では `(AYAVisualRealismEnabled == 2) && RenderVolumetricLighting` で囲う必要あり
- `RenderDepthOfFieldAlphas` cvar も BD 由来 → AYAstorm 側に既に有るか確認要 (Firestorm 標準では不在の可能性、§7 で要確認)

### 3.6 llviewershadermgr.cpp register 詳細

#### 3.6.1 extern 宣言 — `BD llviewershadermgr.cpp:255-256` + `llviewershadermgr.h` 該当行

```cpp
//BD - Volumetric Lighting
LLGLSLShader gVolumetricLightProgram;
```

#### 3.6.2 shader list 追加 — `BD llviewershadermgr.cpp:490-491`

```cpp
//	//BD - Volumetric Lighting
mShaderList.push_back(&gVolumetricLightProgram);
```

`mShaderList` は LL の shader unload helper が回るリスト。

#### 3.6.3 unload — `BD llviewershadermgr.cpp:1211-1212`

```cpp
//		//BD - Volumetric Lighting
gVolumetricLightProgram.unload();
```

#### 3.6.4 register 本体 — `BD llviewershadermgr.cpp:2921-2947`

```cpp
//	//BD - Volumetric Lighting
if (success)
{
	gVolumetricLightProgram.mName = "Volumetric Light Shader";
	gVolumetricLightProgram.mFeatures.isDeferred = true;
    gVolumetricLightProgram.mFeatures.calculatesAtmospherics = true;
    gVolumetricLightProgram.mFeatures.hasAtmospherics = true;
	gVolumetricLightProgram.mFeatures.hasShadows = true;
    gVolumetricLightProgram.mShaderFiles.clear();

    gVolumetricLightProgram.clearPermutations();
    add_common_permutations(&gVolumetricLightProgram);
	gVolumetricLightProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER));
	gVolumetricLightProgram.mShaderFiles.push_back(make_pair("deferred/volumetricLightF.glsl", GL_FRAGMENT_SHADER));
	gVolumetricLightProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

    if (gSavedSettings.getBOOL("RenderVolumetricLightingDirectional"))
    {
        gDeferredSoftenProgram.addPermutation("GODRAYS_FADE", "1");
        // 注意: ↑ 取付先が gDeferredSoftenProgram になっている。
        //       しかし GODRAYS_FADE define は volumetricLightF.glsl 内でしか参照されていない。
        //       BD バグの可能性、§5.1 で取り込み時修正対象。
    }

	success = gVolumetricLightProgram.createShader();
}
```

**feature flag 4 件**:
- `isDeferred = true` — deferred shader として扱う (post-process pass 含む)
- `calculatesAtmospherics = true` — sun direction / atmosphere param が uniform に link される
- `hasAtmospherics = true` — atmosphere helper function が link される (`getAtmosAttenuation` 等)
- `hasShadows = true` — shadow uniform / `nonpcfShadowAtPos` helper が link される

**重要 — GODRAYS_FADE 取付先疑義**: `class3/deferred/volumetricLightF.glsl` 内の `#if GODRAYS_FADE` ガードは **volumetricLight shader 内のみ**。BD コードでは permutation を `gDeferredSoftenProgram` に追加しているが、softenLight shader 側に GODRAYS_FADE 参照は (静的 survey の限り) 無い。**BD で実際に GODRAYS_FADE が effective だったか疑問**。AYAstorm 取り込み時は `gVolumetricLightProgram.addPermutation("GODRAYS_FADE", "1")` に修正する (§5.1)。

### 3.7 llshadermgr 内 uniform enum + 文字列

#### 3.7.1 enum — `BD llshadermgr.h:369-372`

```cpp
//		//BD - Volumetric Lighting
GODRAY_RES,
GODRAY_MULTIPLIER,
FALLOFF_MULTIPLIER,
```

LL の `LLShaderMgr::eGLSLReservedUniforms` enum 内に追加されている。

#### 3.7.2 文字列定義場所

`llshadermgr.cpp` 内の `mReservedUniforms.push_back(...)` シーケンス内に対応文字列 (`"godray_res"`, `"godray_multiplier"`, `"falloff_multiplier"`) が追加されているはず (上記 enum と 1 対 1 対応で順番依存)。AYAstorm 取り込み時は enum 順 = 文字列順を厳格に守る必要あり。§7 で BD 側具体 line を再 fetch して確認。

---

## 4. AYAstorm 側改修ポイント (file:line 単位)

`ayastorm-release` HEAD (= P2 ship 後) を起点に。

### 4.1 `indra/newview/app_settings/settings.xml` への cvar 追加

P2 cvar 群 (`RenderMotionBlurStrength` 等) の直後に Volumetric Lighting 5 件を追加。`settings_blackdragon.xml` は取り込まず、Firestorm 標準 `settings.xml` に統合 (理由: BD UI を取り込まない以上、独立 settings file を読む infrastructure を新設する価値がない)。

cvar 5 件 (AYAstorm 用、§5.3 で default 確定):

```xml
<key>RenderVolumetricLighting</key>
<map>
  <key>Comment</key>
  <string>Enable volumetric lighting (godrays) in Cinematic mode. Requires AYAVisualRealismEnabled=2.</string>
  <key>Persist</key>
  <integer>1</integer>
  <key>Type</key>
  <string>Boolean</string>
  <key>Value</key>
  <integer>1</integer>
</map>
<key>RenderVolumetricLightingResolution</key>
<map>
  <key>Comment</key>
  <string>Volumetric lighting godray sample count per pixel.</string>
  <key>Persist</key>
  <integer>1</integer>
  <key>Type</key>
  <string>U32</string>
  <key>Value</key>
  <integer>16</integer>   <!-- §5.3 で BD default 1 から引き上げ -->
</map>
<key>RenderVolumetricLightingMultiplier</key>
<map>
  <key>Comment</key>
  <string>Volumetric lighting intensity multiplier.</string>
  <key>Persist</key>
  <integer>1</integer>
  <key>Type</key>
  <string>F32</string>
  <key>Value</key>
  <real>1.0</real>
</map>
<key>RenderVolumetricLightingFalloffMultiplier</key>
<map>
  <key>Comment</key>
  <string>Volumetric lighting distance falloff multiplier.</string>
  <key>Persist</key>
  <integer>1</integer>
  <key>Type</key>
  <string>F32</string>
  <key>Value</key>
  <real>1.0</real>
</map>
<key>RenderVolumetricLightingDirectional</key>
<map>
  <key>Comment</key>
  <string>Fade volumetric lighting out when not facing the sun.</string>
  <key>Persist</key>
  <integer>1</integer>
  <key>Type</key>
  <string>Boolean</string>
  <key>Value</key>
  <integer>1</integer>
</map>
```

### 4.2 `indra/newview/pipeline.cpp` / `pipeline.h`

#### 4.2.1 `pipeline.h`

- class `LLPipeline` 内 `static bool RenderVolumetricLighting;` 等の宣言追加 (P2 で `RenderMotionBlurStrength` 等を追加した位置の直下)
- `void renderVolumetric(LLRenderTarget* src, LLRenderTarget* dst);` 宣言追加 (P2 の `renderMotionBlurComposite` 宣言の隣)
- `LLRenderTarget mPostPingMap, mPostPongMap;` は LL 既存 (要確認、§7) — 無ければ追加

#### 4.2.2 `pipeline.cpp`

| 改修箇所 | BD ref | AYAstorm 改修内容 |
|---|---|---|
| static 定義 | BD:250-253 | P2 cvar static 定義の直下に 4 件追加 (Directional は static 不要、shader rebuild gate) |
| `connectRefreshCachedSettingsSafe()` | BD:668-672 | P2 cvar の signal 配線直下に 4 件追加 |
| `refreshCachedSettings()` | BD:1302-1306 | 同じく 4 件追加 |
| `renderVolumetric()` 本体 | BD:8469-8497 | 新規追加、Cinematic gate (`AYAVisualRealismEnabled == 2`) を `if (RenderVolumetricLighting)` 外側 or 内側に追加 (§4.7 で確定) |
| `renderFinalize()` 内 hook | BD:8551-8552 | `combineGlow()` 直前に `renderVolumetric(sourceBuffer, sourceBuffer)` (BD 通り) or `renderVolumetric(sourceBuffer, targetBuffer); std::swap(...)` (§5.2 で確定) |

### 4.3 `indra/newview/llviewershadermgr.cpp` / `llviewershadermgr.h`

| 改修箇所 | BD ref | AYAstorm 改修内容 |
|---|---|---|
| `extern LLGLSLShader gVolumetricLightProgram;` (h) | BD `llviewershadermgr.h` | P2 velocity program extern の直下に追加 |
| `LLGLSLShader gVolumetricLightProgram;` (cpp) | BD:255-256 | P2 velocity program 定義の直下に追加 |
| `mShaderList.push_back()` | BD:490-491 | P2 push 列の最後に追加 |
| `unload()` | BD:1211-1212 | P2 unload 列の最後に追加 |
| register 本体 | BD:2921-2947 | P2 velocity register block の直後に追加、**GODRAYS_FADE 取付先を `gVolumetricLightProgram.addPermutation()` に修正** (§5.1) |

### 4.4 `indra/llrender/llshadermgr.{cpp,h}`

| 改修箇所 | BD ref | AYAstorm 改修内容 |
|---|---|---|
| enum `eGLSLReservedUniforms` | BD `llshadermgr.h:369-372` | `GODRAY_RES` / `GODRAY_MULTIPLIER` / `FALLOFF_MULTIPLIER` 3 件追加、enum 順 = 文字列順を §7 で再確認 |
| `mReservedUniforms.push_back()` 列 | BD `llshadermgr.cpp` (line 未確認、§7) | 対応文字列 `"godray_res"` / `"godray_multiplier"` / `"falloff_multiplier"` 3 件を enum 同順で追加 |

### 4.5 `indra/newview/lldrawpoolalpha.cpp`

- BD:210-226 の forward pass 拡張を取り込むか判断 (§5.4)
- 取り込む場合: Cinematic gate (`AYAVisualRealismEnabled == 2`) を `&& gPipeline.RenderVolumetricLighting` の前段に追加して、AYAstorm View / Firestorm View の alpha pass 挙動を変えない
- `RenderDepthOfFieldAlphas` cvar が Firestorm 既存か §7 で確認、不在なら settings.xml に追加 (BD default ON)

### 4.6 shader 取り込み (header / provenance / class1 + class3)

```
indra/newview/app_settings/shaders/class1/deferred/volumetricLightF.glsl  (stub)
indra/newview/app_settings/shaders/class3/deferred/volumetricLightF.glsl  (本体)
```

- ファイル header byte-for-byte 維持 (`$LicenseInfo:firstyear=2007&license=viewerlgpl$` 等)
- header 直下に provenance コメント追加:
  ```glsl
  // AYAstorm: imported from BlackDragon Viewer (NiranV Dean),
  //           commit 995a1354d8 (2026-04-19), LGPL-2.1-only license inheritance.
  ```
- §5.1 で GODRAYS_FADE permutation の取付先を修正するが、shader 本体の `#if GODRAYS_FADE` ガードは保持 (= BD 由来コード自体は変更しない、permutation 追加先を C++ 側で修正)

### 4.7 Cinematic gate ロジック

P2 で確立した方針 (Cinematic mode = `AYAVisualRealismEnabled == 2`) を踏襲:

- `renderVolumetric()` 本体冒頭で `if (LLPipeline::sViewModeCinematic && RenderVolumetricLighting)` で 2 段 gate (P2 `renderMotionBlurComposite` と同様の流儀)
- `lldrawpoolalpha.cpp` の forward pass 拡張も Cinematic gate を追加
- `gVolumetricLightProgram` の shader register / unload は Cinematic mode に関係なく実行 (起動時 1 回構築の方針、ランタイム gate は描画時のみ)

---

## 5. 取り込み時の改修・修正項目

### 5.1 GODRAYS_FADE permutation の取付先修正 (BD バグ修正)

**問題**:
- BD `llviewershadermgr.cpp:2937-2939` は `gSavedSettings.getBOOL("RenderVolumetricLightingDirectional")` ON 時に `gDeferredSoftenProgram.addPermutation("GODRAYS_FADE", "1")` を実行
- しかし `GODRAYS_FADE` define は `class3/deferred/volumetricLightF.glsl` 内の `#if GODRAYS_FADE` でのみ参照
- softenLight shader 側に GODRAYS_FADE 参照は (静的 survey の限り) 無い → BD では `RenderVolumetricLightingDirectional = ON` の効果が **実質無効化されていた可能性**

**AYAstorm 修正**:
```cpp
if (gSavedSettings.getBOOL("RenderVolumetricLightingDirectional"))
{
    gVolumetricLightProgram.addPermutation("GODRAYS_FADE", "1");
}
```
取付先を `gVolumetricLightProgram` に変更。これで `volumetricLightF.glsl` 内の `#if GODRAYS_FADE` が effective になり、太陽が地平線下では godrays が fade out する (太陽が見えない時に空中に光線が残るのを防ぐ)。

§7.2 で実装時に再確認 (`gDeferredSoftenProgram` に GODRAYS_FADE 参照が無いことを fetch で改めて確かめる)。

### 5.2 in-place 書き込み vs pong バッファ化

**問題**: BD は `renderVolumetric(sourceBuffer, sourceBuffer)` で同一バッファに自己 sample しながら書き込む。GL 仕様上 undefined behavior に近いが、多くの driver で動く (同 texel の read-then-write はキャッシュ的にギリ通る)。

**判断軸**:
- 案 A (BD 通り、in-place): RT 1 つで済む、`combineGlow` 直前に挿入するだけ、最低限の改修。Mac M1 / Intel iGPU で driver が落ちるリスクあり。
- 案 B (pong 化): `renderVolumetric(sourceBuffer, targetBuffer); std::swap(sourceBuffer, targetBuffer);` パターン。P2 で `renderMotionBlurComposite` が採用した流儀と一致、driver 安全。代わりに pong RT を 1 段消費。

**推奨 (P3 着手時の暫定方針)**: **案 B (pong 化)** を採用。
- 理由 1: P2 で確立した swap chain 流儀との一貫性
- 理由 2: 3 OS 対応 (Mac driver の挙動不明) 観点で安全側
- 理由 3: `mPostPingMap` / `mPostPongMap` は LL viewer に既存、追加 RT 確保不要 (要確認、§7.3)

§7.3 で実装時に検証 (案 B で実装→案 A も A/B 比較したい場合のみ後追い)。

### 5.3 cvar 既定値の AYAstorm 化

BD default はそのまま使わず、AYAstorm 用に再決定:

| Cvar | BD default | AYAstorm default | 理由 |
|---|---|---|---|
| `RenderVolumetricLighting` | `1` | `1` | Cinematic mode 内では default ON (Cinematic を選んだユーザーは godrays を期待する) |
| `RenderVolumetricLightingResolution` | `1` | `16` | BD default 1 は実質無効化、godrays 効果が見える最低ラインとして 16 を仮設定 (P3 受入観測で調整) |
| `RenderVolumetricLightingMultiplier` | `1` | `1.0` | 強度はそのまま、過剰なら受入観測で下げる |
| `RenderVolumetricLightingFalloffMultiplier` | `1` | `1.0` | 同上 |
| `RenderVolumetricLightingDirectional` | `1` | `1` | §5.1 修正後は太陽 fade が effective、ON で物理的に自然 |

**Cinematic gate との関係**: `AYAVisualRealismEnabled != 2` (Cinematic 以外) では `renderVolumetric()` が呼ばれないので cvar 値は無効。ユーザー視点では「Cinematic 起動時のみ godrays が見える」。AYAstorm View / Firestorm View には一切影響しない。

### 5.4 lldrawpoolalpha 拡張要否

BD:210-226 の forward pass 拡張 (alpha 物体の depth 書き出し + minimum alpha 0.7) を取り込むか:

- **取り込む場合**: godrays が alpha 物体 (透明物体、樹木の葉など) を抜けて差し込む現象が抑制される (alpha 物体を depth に固めるため shadow 経路がそこで止まる)
- **取り込まない場合**: alpha 物体越しに godrays が見え続ける = 物理的に不正確、ただし最初の受入観測でユーザー体感的に違和感がなければそのまま放置でも良い

**推奨**: **取り込む** (Cinematic gate 付き)。理由: BD で同梱されているということは「godrays + alpha 物体」組み合わせで違和感が出るユースケースが既知。AYAstorm でも先回りで対処しておく方が品質高い。

`RenderDepthOfFieldAlphas` cvar の扱い:
- Firestorm 既存なら流用
- 不在なら settings.xml に追加 (default = `1` ON、BD と同じ)
- §7.4 で確認

---

## 6. 実装ステップ案 (P3 着手後の作業順)

### Step 1: shader 2 ファイル取り込み (provenance コメント付き)

- `indra/newview/app_settings/shaders/class1/deferred/volumetricLightF.glsl` 新規 (BD class1 byte-for-byte + provenance コメント)
- `indra/newview/app_settings/shaders/class3/deferred/volumetricLightF.glsl` 新規 (BD class3 byte-for-byte + provenance コメント)
- Commit: `Import Volumetric Light shaders from Black Dragon Viewer (NiranV Dean) (r30 P3 step 1)`

### Step 2: cvar 5 件 + Cinematic gate

- `indra/newview/app_settings/settings.xml` に 5 件追加 (§4.1)
- Commit: `r30 P3 step 2: add Volumetric Lighting cvars (Cinematic mode gated)`

### Step 3: pipeline.cpp に renderVolumetric() 追加 + renderFinalize() フック

- `pipeline.h` に static / 関数宣言追加
- `pipeline.cpp` に static 定義 / connectRefresh / refreshCached / `renderVolumetric()` 本体 / `renderFinalize()` 内 hook 追加
- §5.2 案 B (pong 化) で実装、Cinematic gate 付き
- Commit: `r30 P3 step 3: pipeline plumbing + renderVolumetric() (pong-chained, Cinematic gated)`

### Step 4: llviewershadermgr 登録 + GODRAYS_FADE permutation 修正

- `llviewershadermgr.h` / `llviewershadermgr.cpp` に shader register / unload / extern 追加
- §5.1 GODRAYS_FADE permutation 取付先を `gVolumetricLightProgram` に変更
- `llshadermgr.h` / `llshadermgr.cpp` に uniform enum + 文字列追加 (§4.4)
- Commit: `r30 P3 step 4: register gVolumetricLightProgram + fix GODRAYS_FADE permutation target`

### Step 5: lldrawpoolalpha 拡張 (forward pass)

- §5.4 推奨通り取り込み、Cinematic gate 付き
- `RenderDepthOfFieldAlphas` cvar が不在なら settings.xml に追加
- Commit: `r30 P3 step 5: alpha pool depth-write extension for godrays/DoF (Cinematic gated)`

### Step 6: 受入観測

- Cinematic mode 起動、屋外で太陽が高い時間帯 → godrays が見えるか目視
- Cinematic mode → AYAstorm View 切替 (再起動) → godrays が完全に消えるか確認
- AYAstorm View / Firestorm View に regression なし (色 / 描画速度) 確認
- `RenderVolumetricLightingResolution` を 8 / 16 / 32 で A/B、品質と FPS の許容点を決定
- `RenderVolumetricLightingDirectional = 0` で太陽が地平線下でも godrays が残るか確認 (§5.1 修正の effective 確認)
- 受入観測結果は本 spec §8 に追記

---

## 7. 未確定事項 / P3 着手時に再確認

### 7.1 `RenderVolumetricLightingResolution` 真の有効最小値

BD default `1` では for ループ (`i = godray_res - 1; i > 0; --i`) が 1 回も回らず godrays 無効化される。BD で本当に godrays が見えていたのか、それとも cvar 名と効果の対応がずれているか、BD 側 PR や issue を fetch して確認。

### 7.2 GODRAYS_FADE permutation の真の取付先

`gDeferredSoftenProgram` 側に GODRAYS_FADE 参照が無いことを `softenLightF.glsl` fetch で再確認。確認後 §5.1 修正を確定。

### 7.3 `mPostPingMap` / `mPostPongMap` の AYAstorm 側存在確認

`indra/newview/pipeline.h` を grep して両 RT が宣言されているか、確保 path が `allocateScreenBufferInternal()` 等に既にあるか確認。無ければ追加が必要 → §5.2 案 B (pong 化) のコストが増える。

### 7.4 `RenderDepthOfFieldAlphas` cvar の Firestorm 側存在確認

`indra/newview/app_settings/settings.xml` を grep。既存なら流用、不在なら追加 (BD default `1` ON 踏襲)。

### 7.5 `llshadermgr.cpp` 内 uniform 文字列の具体追加位置

BD `llshadermgr.cpp` の `mReservedUniforms.push_back()` 列の正確な line を再 fetch、enum 順 = 文字列順を厳格に守る。

### 7.6 既存 atmosphere uniform binding 経路の検証

`bindDeferredShader()` 経由で `sun_dir` / `blue_density` / `haze_density` / `sunlight_color` が自動 bind されるか、AYAstorm 側 `llviewershadermgr.cpp` で確認。BD では feature flag `calculatesAtmospherics + hasAtmospherics` 経由で接続されるが、AYAstorm 側で feature flag 処理経路が同一か確認。

---

## 8. 実装 commit log (P3 着手後追記)

### 8.1 commit 一覧

| Step | Commit | 概要 |
| --- | --- | --- |
| 1 | `bf269b8671` | BD viewer から shader (`volumetricLightV/F.glsl`) を import (LGPL-2.1-only)、AYAstorm ライセンス header 貼付 |
| 2 | `4e6a97bf86` | `RenderVolumetricLighting` / `Resolution` / `Multiplier` / `FalloffMultiplier` / `Directional` の 5 cvar を `settings.xml` に追加 |
| 3 | `64aa990f7c` | `gVolumetricLightProgram` extern + register、`llshadermgr` に `GODRAY_RES` / `GODRAY_MULTIPLIER` / `FALLOFF_MULTIPLIER` uniform 追加 |
| 4 | `0df903736d` | `pipeline.cpp` に `renderVolumetric()` 本体 + `renderFinalize()` 内 hook (Cinematic + cvar gated, `mPostPingMap`/`mPostPongMap` pong-chain) |
| 5 | `d0a695c2ac` | `lldrawpoolalpha.cpp` で `getType() == POOL_ALPHA_POST_WATER` の depth write gate を Cinematic+`RenderVolumetricLighting` でも有効化 (BD §5.4 案 A) |
| 6 (hotfix) | _未 commit_ | step 6 受入観測 で判明した 3 件の fix を 1 commit にまとめる予定:<br>(a) shader: `nonpcfShadowAtPos` (BD-only) → `sampleDirectionalShadow` (AYAstorm/Firestorm 標準、`shadowUtil.glsl`) 差し替え、`HAS_SUN_SHADOW` permutation gate で全 godrays 計算を guard<br>(b) `llviewershadermgr.cpp`: `gVolumetricLightProgram` を `mShaderList.push_back()` し `LLSettingsVOSky::applyToShader` から atmosphere uniform (`sunlight_color` / `sun_dir` / `blue_density` / `haze_density`) を auto-bind / register block で `mFeatures.hasShadows = use_sun_shadow` + `HAS_SUN_SHADOW` permutation を `RenderShadowDetail > 0` 時のみ付与<br>(c) `settings.xml` + `pipeline.cpp`: `RenderVolumetricLightingMultiplier` default を `1.0` → `50.0` (受入観測 §8.2 参照) |

### 8.2 step 6 受入観測 (2026-05-18)

#### (a) 受入 path

1. shader-only fast iterate (`cp` install + `shader_cache/` clear) で複数回試行
2. Cinematic mode + `RenderVolumetricLighting True` + 屋外昼間 + 太陽が画面内の構図で目視

#### (b) 障害切り分け (canary 法)

Default 値 (Multiplier 1.0) で全く視認できなかったため、shader 内に 4 種類の canary を順に仕込んで bisect:

| Canary | 出力 | AYA 観測結果 | 判明事項 |
| --- | --- | --- | --- |
| 1 | `frag_color = vec3(1.0,0.0,0.0)` | 全面真っ赤 | shader は実行されている (hook / install / permutation 全部 OK) |
| 2 | `frag_color = sunlight_color` | 昼真っ白 / 夕方真っ黒 | atmosphere uniform 届く (`mShaderList.push_back` 効いてる)、SL の sunlight clamp 挙動も正常 |
| 3 | `vec3(shadamount, shaftify, haze_weight.x) * 5` | 画面マゼンタ (R+B、緑ゼロ) | `shaftify` 成分だけゼロ。`shadamount` / `haze_weight.x` は出ている |
| 4 | `shadamount`, `shaftify_pre_fade`, `shaftify_post_fade` の 3 分解 | 空は黄色 (R+G、B 弱)、太陽近傍だけ R+G+B 全部 | `GODRAYS_FADE` の `1 - 2.16 * &#x7c;sun_dir.xy&#x7c;^2` 窓が極狭で、太陽が画面中心 +約30° 以内でしか shaftify_post が残らない (= 仕様通り)。地面側で `shaftify_pre_fade` がゼロになるのは `depth *= pow(depth, 100.0)` の sky-only 重み付けで意図的 |

結論: BD math は全部正しく動作しており、bug ではない。**default が肉眼 threshold に届かないのが唯一の問題**。

#### (c) Multiplier threshold tuning

`RenderVolumetricLightingMultiplier` を 50 / 100 / 200 で A/B、AYA さんの主観評価:

| 値 | 印象 |
| --- | --- |
| 1.0 (BD default) | 不可視 |
| 50 | 現実世界に近い、自然な薄明光線 |
| 100 | しっかり godrays 主張、雰囲気強め |
| 200 | PV/Cinematic 向け、かなり強い |

AYAstorm r30 thesis (`project_ayastorm_visual_realism_chapter.md` = 写真を撮るに値する空気と空間) と整合する **50.0 を default に確定**。

#### (d) BD default (1.0) との乖離理由 (仮説)

AYAstorm の ACES tone mapping + HDR scene buffer が BD の sRGB 直書きより加算分を強く圧縮する、または BD パイプラインで godray 追加が別場所 (e.g. tone mapping 前の linear space で乗算) で行われていた可能性。BD 側オリジナルの「default 1.0 で見えていたか」は未検証。AYAstorm 環境では 50 必須。

#### (e) regression check

- AYAstorm View / Firestorm View に切替後 (再起動 path 経由)、`renderVolumetric` の Cinematic gate (`aya_view_mode == 2`) で hook 自体が dispatch されないことを確認 (canary 1 で「真っ赤」が AYAstorm/Firestorm View 時に出ないことで間接確認)
- Cinematic + `RenderVolumetricLighting False` で hook skip、`True` で即時 godrays 復活、`True/False/True` 即時切替で再起動なし反映を確認

#### (f) `RenderVolumetricLightingDirectional` (GODRAYS_FADE permutation) 受入

- `Directional=False` で AYAstorm 再起動 → shader が `GODRAYS_FADE` permutation 無しで rebuild
- AYA 観測: **建物等が白く光る** (`shaftify *= fade` が外れて全画面 godrays 加算が乗る、Multiplier=50 で建物が godrays に飲まれた状態)
- これにより §5.1 修正 (`gDeferredSoftenProgram` から `gVolumetricLightProgram` への permutation 取付先変更) が effective に効いていることを実機裏付け
- 既定 `Directional=True` に戻し AYAstorm 再起動で正常 (太陽 view 前方のみ godrays) に復帰確認

#### (g) 未実施 (release 後 user feedback 収集対象)

- `RenderVolumetricLightingResolution` を 8 / 16 / 32 / 64 で FPS 影響計測 — tuning は user 側で `feedback_release_with_user_feedback` 方針
- §7.1「Resolution=1 で本当に godrays 無効か」の最終目視確認 — shader 上 `for (i=godray_res-1; i>0; --i)` は `godray_res=1` で 0 回ループなので無効化されるはず、実機未確認
