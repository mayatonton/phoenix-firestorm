# r14 P0: sun rendering / glow / tonemap パイプライン調査

**作成日**: 2026-05-12
**対象**: AYAstorm r14 (sun disc overbright + bloom) の実装注入点同定
**ブランチ**: `feature/aya-r14-p0-roadmap-update` (P0 中)

> 本書は r14 spec (`docs/ayastorm-r14-sun-dazzle.md`) §3 P0 の **完了条件** (太陽輝度 boost を入れる関数 / shader uniform の同定、glow kernel 関連パラメータの同定、tonemap 前段で boost が効くことの確認) に応える調査記録。実装 (P1) 着手前のスナップショット。

---

## 1. 太陽 rendering の経路

| 役割 | ファイル | 関数 / クラス | 行 |
|---|---|---|---|
| 太陽 billboard 本体 | `indra/newview/llvosky.cpp` | `LLVOSky::FACE_SUN` (LLFace) | 856〜 |
| 太陽サイズ / 輝度定数 | `indra/newview/llvosky.cpp` | `SUN_DISK_RADIUS=0.5f` / `SUN_INTENSITY=1e5` | 72〜74 |
| 太陽方向 / 状態 | `indra/newview/llvosky.h` | `class LLHeavenBody` | 127〜193 |
| 太陽 draw 起動点 | `indra/newview/lldrawpoolwlsky.cpp` | `LLDrawPoolWLSky::renderHeavenlyBodies()` | 354 |
| 太陽 vertex shader | `app_settings/shaders/class1/deferred/sunDiscV.glsl` | (sun disc vertex) | 全体 |
| **太陽 fragment shader (boost 注入点)** | `app_settings/shaders/class1/deferred/sunDiscF.glsl` | `main()` | 38〜57 |

**sunDiscF.glsl の現状**:
```glsl
void main()
{
    vec4 sunDiscA = texture(diffuseMap, vary_texcoord0.xy);
    vec4 sunDiscB = texture(altDiffuseMap, vary_texcoord0.xy);
    vec4 c     = mix(sunDiscA, sunDiscB, blend_factor);
    // ...
    frag_data[0] = vec4(0);
    frag_data[1] = vec4(0.0f);
    frag_data[2] = vec4(0.0, 1.0, 0.0, GBUFFER_FLAG_SKIP_ATMOS);
#if defined(HAS_EMISSIVE)
    frag_data[0] = vec4(0);
    frag_data[3] = c;
#else
    frag_data[0] = c;
#endif
}
```

太陽テクスチャをサンプルした `c` をそのまま出力するだけの passthrough。**現状ブースト係数なし**、boost を入れる単一の注入点として極めてクリーンな状態。

---

## 2. glow / bloom pipeline

| 役割 | ファイル | 関数 | 行 |
|---|---|---|---|
| glow extract pass | `app_settings/shaders/class1/effects/glowExtractF.glsl` | `main()` | 44〜62 |
| glow blur pass | `app_settings/shaders/class1/effects/glowF.glsl` | (kernel blur) | 全体 |
| glow パイプライン制御 | `indra/newview/pipeline.cpp` | `LLPipeline::generateGlow()` | 8406〜8506 |
| glow 合成 | `indra/newview/pipeline.cpp` | `LLPipeline::combineGlow()` | 8867〜 |
| glow バッファ allocate | `indra/newview/pipeline.cpp` | `mGlow[i].allocate(512, glow_res, glow_color_fmt)` | 1460 |

**重要な現状 (pipeline.cpp:8420)**:
```cpp
gGlowExtractProgram.uniform1f(LLShaderMgr::GLOW_MIN_LUMINANCE, 9999);
```

**`GLOW_MIN_LUMINANCE` が 9999 にハードコードされており、`RenderGlowMinLuminance` 設定値 (default 1.0) は実際には使われていない**。glowExtractF.glsl は次の式で alpha を計算するため:
```glsl
float lum = smoothstep(minLuminance, minLuminance+1.0, dot(col.rgb, lumWeights));
// ↑ minLuminance=9999 だと lum は実質常に 0
frag_color.a = max(col.a, mix(lum, warmth, warmthAmount) * maxExtractAlpha);
//             = max(col.a, 0) = col.a
```

→ **glow 強度は実質「ソースの alpha channel」だけが駆動している**。RGB 輝度ベースの glow extract は無効化された状態が現行。

---

## 3. tonemap / exposure pipeline

| 役割 | ファイル | 行 |
|---|---|---|
| HDR シーンバッファ allocate | `pipeline.cpp` (`mRT->screen.allocate(GL_RGBA16F)`) | 985 |
| LDR post-tonemap バッファ allocate | `pipeline.cpp` (`mPostPingMap.allocate(GL_RGBA)`) | 1045 |
| luminance 計測 | `pipeline.cpp` (`generateLuminance(mRT->screen, mLuminanceMap)`) | 9372 |
| **auto-exposure** | `pipeline.cpp` (`generateExposure(mLuminanceMap, mExposureMap)`) | 9374 |
| tonemap | `pipeline.cpp` (`tonemap(mRT->screen, mPostPingMap)`) | 9379 |
| tonemap shader 本体 | `app_settings/shaders/class1/deferred/postDeferredTonemap.glsl` | `main()` 86〜112 |
| 共有 tonemap ユーティリティ | `app_settings/shaders/class1/deferred/tonemapUtilF.glsl` | 全体 |
| glow 起動点 | `pipeline.cpp` (`generateGlow(mPostPingMap)`) | 9394 |

**パイプライン順序** (pipeline.cpp:9370〜9394):
```
mRT->screen (HDR, RGBA16F, linear)
    ↓ generateLuminance
mLuminanceMap
    ↓ generateExposure
mExposureMap
    ↓ tonemap (HDR→LDR, alpha は passthrough)
mPostPingMap (LDR, RGBA, 8bit, 0-1 clamp)
    ↓ generateGlow (alpha channel ベースで extract)
mGlow[1]
    ↓ combineGlow (additive)
mPostPongMap (final image)
```

**Key implications**:
- HDR RGB ブーストは `mRT->screen` 段階でしか効かない。tonemap が [0,1] clamp で潰す
- ただし **`tonemap` は alpha を passthrough**、`generateGlow` は **mPostPingMap の alpha** から glow を生成
- つまり **太陽の alpha を boost すれば glow halo は確実に出る** (RGB を tonemap 経由で見せるよりも安定)
- **auto-exposure の足場 (`generateLuminance` / `generateExposure` / `mExposureMap`) は既に存在**する。r16 (auto-exposure) 時はこれを「シーン全体の `gain` 倍率として使う」段階で実装が大幅に圧縮される可能性

---

## 4. 既存 `RenderGlow*` settings (settings.xml:12393〜12532)

| キー | default | 役割 | r14 で変更するか |
|---|---|---|---|
| `RenderGlow` | true | glow 全体 ON/OFF | 触らない |
| `RenderGlowIterations` | 2 | blur 反復回数 (広がり) | 触らない (kernel 半径は触らない方針) |
| `RenderGlowWidth` | 1.3 | blur サンプル幅 | 触らない |
| `RenderGlowStrength` | 0.325 | additive 合成強度 | 触らない |
| `RenderGlowMinLuminance` | 1.0 | (現状 9999 ハードコードで dead) | r14 では触らない (r15+ で再評価) |
| `RenderGlowMaxExtractAlpha` | 0.25 | extract alpha 上限 | 触らない |
| `RenderGlowResolutionPow` | 9 | glow buffer 解像度 (= 512) | 触らない |
| `RenderGlowHDR` | false | glow buffer を HDR にするか | r14 では触らない (要 verify、もし true で安定するなら r14 内で default true 候補) |
| `RenderGlowNoise` | true | dither | 触らない |

r14 spec の方針 (kernel default は触らず boost 倍率側で表現) と整合。`feedback_prefer_defaults_over_config.md` に従い `Render*` の default 改変はゼロを目指す。

---

## 5. boost 注入点の選択肢

### (A) sunDiscF.glsl で `c.rgb` と `c.a` を boost

```glsl
uniform float sun_disc_boost;  // 新規 uniform
uniform int sun_disc_boost_enabled;  // 新規 sentinel
// ...
if (sun_disc_boost_enabled != 0) {
    c.rgb *= sun_disc_boost;
    c.a = clamp(c.a * sun_disc_boost, 0.0, 1.0);  // alpha は [0,1] clamp
}
```

**Pros**:
- 影響範囲が **太陽 disc のみ** に閉じる (sky / cloud / 他 emissive 不可触)
- 1 shader + uniform 2 個の plumbing で完結
- alpha 経由で確実に glow halo が出る (tonemap 後の LDR でも生きる)

**Cons**:
- RGB 倍率は tonemap で多くが clip される。**眩しさの主な見え方は alpha 経由の glow halo + tonemap 出口での飽和**
- 太陽 alpha が元から低めなテクスチャだと boost してもサチるところで止まる (= 倍率の上限が見える)

→ **r14 の本命**。L1〜L4 リスクを最小化、設計判断と整合。

### (B) `GLOW_MIN_LUMINANCE` の 9999 ハードコード解除

```cpp
// pipeline.cpp:8420
gGlowExtractProgram.uniform1f(LLShaderMgr::GLOW_MIN_LUMINANCE, RenderGlowMinLuminance);
```

**Pros**: 太陽以外も含む scene 全体の HDR 輝度ベース glow が復活、より「正統的」な HDR bloom 表現
**Cons**:
- L4 (コンテンツクリエイター苦情) リスクが極大。全 emissive prim / 街灯 / 強い色のオブジェクトに glow が増える
- 9999 ハードコードは意図的 (= LL の判断) の可能性が高い、軽率に外すと SL 全体の絵が変わる

→ **r14 では採用しない**。`r15+` で lens flare / auto-exposure と一緒に検討する候補。

### (C) renderHeavenlyBodies で sun の geom alpha を倍にする

GL レベルで sun の vertex alpha を boost。

→ **採用しない**。shader uniform 経由 (A) と効果は同じだが、CPU 側コードが増えるだけ。

---

## 6. r14 spec §3 P1 の touch points 確定

P0 完了として r14 spec の P1 (太陽輝度 boost 実装) で触るファイル群を以下に確定:

| ファイル | 変更内容 |
|---|---|
| `indra/newview/app_settings/shaders/class1/deferred/sunDiscF.glsl` | `uniform float sun_disc_boost; uniform int sun_disc_boost_enabled;` 追加、`main()` 末尾で c.rgb / c.a に boost 適用 |
| `indra/newview/lldrawpoolwlsky.cpp` (`renderHeavenlyBodies()`) | `sun_shader->uniform1f("sun_disc_boost", ...)` + `uniform1i("sun_disc_boost_enabled", ...)` を bind 後・draw 前に呼ぶ |
| `indra/newview/app_settings/settings.xml` | `RenderSunDiscBoost` (F32 default 12.0) + `RenderSunDiscBoostEnabled` (Boolean default true) 追加 |
| `indra/llrender/llshadermgr.{h,cpp}` (`uniform name table`) | `SUN_DISC_BOOST` / `SUN_DISC_BOOST_ENABLED` の string enum 追加 (LL の uniform plumbing 流儀に従う) |

P2 (glow kernel 調整) は **基本「何もしない」**。boost 倍率側で表現が出ない場合のみ `RenderGlowStrength` の sun-時のみの上書きを検討するが、現状の `0.325 × alpha 1.0` で見える halo を倍率で押し込む方向で詰める。

P3 (3 OS ビルド + 体感確認) は r14 spec §3 P3 のシナリオを踏襲。

---

## 7. 副産物的に得た判明事項

- **`RenderGlowMinLuminance` setting は実質 dead (9999 ハードコード)** — 光表現章全体で再評価対象、r15+ で改めて検討
- **`generateExposure` パスが既に存在 (`mExposureMap`)** — r16 (auto-exposure / eye adaptation) の足場は予想より厚い。spec §3 で 4〜6 日と置いたが、`generateExposure` の出力をどう tonemap に渡しているかを次の P0 (r16 着手時) で確認すれば、もっと短縮できる可能性
- **sky 全体は `skyF.glsl` で `clamp(color, 0, 5)`** — 太陽 disc とは別系統で sky 自体も実質 5 倍までしか出せない、Layer B (大気) 拡張時に再評価対象
- **`SUN_INTENSITY = 1e5`** は CPU 側の物理計算用定数 (太陽方向の強度) であり、shader 出力強度とは別物。これを変えても画面の見え方は変わらない (確認済)

---

## 8. P1 着手前の確認事項 (open)

- **`sun_disc_boost` の default 12.0 は推測値**。P1 実装後、midday windlight で alpha boost が halo 化し始める実機値を計測して default を詰める (体感 5〜30 倍のレンジを掃く想定)
- **`RenderGlowHDR` を true にした場合の glow 表現変化** が unknown。P1 / P2 で alpha boost だけで halo が出ない場合の縮退オプションとして手元に置く
- **sun disc texture の alpha distribution** が unknown。LL 既定 sun texture と windlight presets で太陽中心の alpha が既に 1.0 に近いと boost が頭打ちになる可能性

---

## 9. Round 2 (P0 やり直し): skyF.glsl 大気散乱経路の発見

**追記日**: 2026-05-12 (P1 revert 後)
**契機**: P1 (sun_disc_boost) を AYA 実機で検証したところ、`RenderSunDiscBoostEnabled` の TRUE/FALSE で画面差ゼロ、検証用「真っ赤 hack」(`c.rgb = vec3(1.0, 0.0, 0.0); c.a = 1.0;`) を入れても画面が赤くならなかった (screenshot 2 枚 = `~/ピクチャ/Screenshots/Screenshot from 2026-05-12 03-00-43.png` / `03-02-41.png` で確認、両方とも halo は出ているが赤くない)。`shader_level=5` (PBR)、`RenderEnableEmissiveBuffer=TRUE` (default)、shader_cache は新規生成、install 先 shader は確かに hack 入り — それでも shader 出力が画面に届かない事実から、根本前提を再吟味。

### 9.1 Round 1 の誤り (root cause)

Round 1 (§1〜§8) は **「画面上の halo は sun disc shader が生成しているはず」** という暗黙の前提で `sunDiscF.glsl` と `pipeline.cpp` の glow 経路だけを掘った。実際には:

- **AYAstorm 既存で太陽方向の halo は `skyV.glsl` + `skyF.glsl` 経路 (大気散乱) で生成済み**だった
- sun disc は billboard で画面上は小さい点に過ぎず、その背後の大気散乱が遥かに大きく明るく halo を作っている
- screenshot で見える「広い白光」の主役は sun disc shader ではない

§1〜§8 の touch points (sun_disc_boost 系) は **halo の主役を取れない場所** を弄っていた。

### 9.2 真の halo 生成経路 (skyV.glsl → skyF.glsl → glowExtract)

**`class1/deferred/skyV.glsl:127〜143`** — 太陽方向で強く scaling される haze 計算:

```glsl
// Compute haze glow
float haze_glow = 1.0 - rel_pos_lightnorm_dot;  // dot(view, sun)、太陽方向で 0 に近づく
haze_glow = max(haze_glow, .001);
haze_glow *= glow.x;
haze_glow = pow(haze_glow, glow.z);              // glow.z は負、なので 1/(haze_glow)^|z|
haze_glow = (sun_moon_glow_factor < 1.0) ? 0.0 : (sun_moon_glow_factor * (haze_glow + 0.25));

vec3 color = (blue_horizon * blue_weight * (sunlight + ambient_color)
           + (haze_horizon * haze_weight) * (sunlight * haze_glow + ambient_color));
//                                                       ↑ ここが太陽方向で大きくなる
```

`glow` (vec3) は windlight preset 由来の uniform (`glow.x` で sharpness、`glow.z` で falloff)。**`vary_HazeColor` に伝搬されて skyF.glsl に渡る**。

**`class1/deferred/skyF.glsl:107〜127`** — HDR 域へ持ち上げと alpha 出力:

```glsl
color = vary_HazeColor;
color.rgb += rainbow(optic_d);                   // 虹 (moisture_level 依存)
color.rgb += halo22(optic_d);                    // 22°ハロー (ice_level 依存)
color.rgb *= 2.;                                  // ← HDR 倍率 (hardcoded 2.0)
color.rgb = clamp(color.rgb, vec3(0), vec3(5));  // ← clamp 上限 (hardcoded 5.0)
// ...
#if defined(HAS_EMISSIVE)
    frag_data[0] = vec4(0);
    frag_data[3] = vec4(color.rgb, 1.0);          // emissive 経路、alpha=1.0
#else
    frag_data[0] = vec4(color.rgb, 1.0);          // 通常経路、alpha=1.0
#endif
```

`frag_data` の **alpha が常に 1.0** で出力されている点が決定的に重要。

**`class1/effects/glowExtractF.glsl:44〜62`** — alpha 経路駆動の確認:

```glsl
float lum = smoothstep(minLuminance, minLuminance+1.0, dot(col.rgb, lumWeights));
// minLuminance=9999 (hardcoded in pipeline.cpp:8420)、col.rgb は LDR 0-1、lum はほぼ 0
frag_color.rgb = col.rgb;
frag_color.a = max(col.a, mix(lum, warmth, warmthAmount) * maxExtractAlpha);
//             = max(col.a, ~0) = col.a
```

→ **glow extract は LDR tonemap 後 buffer の alpha channel をそのまま読んでいる**。skyF.glsl が alpha=1.0 を出すので、太陽方向の hot spot (haze_glow) が tonemap 後も alpha=1.0 で残り、glow kernel が blur して halo 化する。

### 9.3 ここまでの結論 (P0 やり直しの結語)

1. AYAstorm 既存の halo は **skyV/F.glsl の大気散乱 (haze_glow + halo22 + rainbow) → alpha=1.0 で frag 出力 → glowExtract alpha 経路 → glow blur kernel** で生成されている
2. sun disc shader (`sunDiscF.glsl`) は halo の主役ではない。boost してもこの経路に乗らないため見た目に変化が出ない (= Round 1 の P1 が無効化した理由)
3. AYAstorm 既存 (r13 ベース) で既に halo は出ている (screenshot で確認可能)。AYA さんが「眩しくない」と感じた最初の体感は、halo の **広がり / 強度 / 周辺との対比** が AAA タイトル比で足りないという話で、disc 強化では解決しない

### 9.4 有効な lever 候補 (Round 2)

| Lever | 場所 | 内容 | リスク |
|---|---|---|---|
| **L1 (haze HDR scale)** | `skyF.glsl:114` | `color.rgb *= 2.` の係数を uniform 化、controllable に | 低 (clamp(5) で抑え、frag_data[0] alpha=1.0 は不変) |
| **L2 (haze HDR clamp)** | `skyF.glsl:115` | `clamp(0, 5)` の上限を uniform 化、5→10 等まで持ち上げ | 中 (tonemap で多くが潰されるので絵の見え方への寄与は L1 より小さい) |
| **L3 (haze_glow boost)** | `skyV.glsl:132〜139` | `haze_glow` 計算に uniform 倍率 (`* boost`) を挿入。太陽方向の hot spot を直接スケール | 中 (太陽 / 月で扱いが分岐、月側は出ない設計を維持する必要) |
| **L4 (glow kernel 強化)** | `settings.xml` (`RenderGlowIterations` / `RenderGlowWidth` / `RenderGlowStrength`) | AYAstorm 既定値を上げる、shader / cpp は触らない | 中〜高 (`feedback_prefer_defaults_over_config.md` 違反気味だが、コードを増やさない代わりに既定値だけ動かす) |
| **L5 (scope-out)** | — | r14 「sun dazzle」を取り下げ、r15 (lens flare) を r14 に繰り上げ | 低 (AYAstorm 既存 halo で打ち止め、別軸で眩しさを伸ばす) |

**最有力**: **L1 + L4 の組合せ**。skyF.glsl の `*2.` を `* haze_hdr_scale` (sentinel 付き) に置き換え、AYAstorm default 値で 2.0→4.0 程度に lift。同時に `RenderGlowIterations` を 2→3、`RenderGlowWidth` を 1.3→2.0 等に上げて halo を広く・明るくする。両方を controllable に保つ。

ただし: AYA さんが既存 screenshot で「もう十分眩しい」と判断するなら **L5 (scope-out)** で r14 終了 → r15 (lens flare) を繰り上げる方が筋が良い可能性。Round 2 では **AYA さんに既存 halo の主観評価を仰ぐ** ステップを入れる。

### 9.5 次の作業 (Round 2 P1 候補)

- AYA さんに「既存 screenshot の halo で眩しいと感じるか / 何が足りないか」を質問し、L1+L4 で押すか L5 で scope-out するかを決める
- 決定後、r14 spec (`docs/ayastorm-r14-sun-dazzle.md`) を新スコープで全面書き直し
- P1 を Round 2 のレバーで再実装
