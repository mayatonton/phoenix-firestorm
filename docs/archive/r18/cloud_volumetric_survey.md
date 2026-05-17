# r18 雲の体積化 + 色温度連動 P0 調査 — Round 1

**作成日**: 2026-05-12
**branch**: `feature/aya-r18-cloud-volumetric-spec-draft`
**spec**: `docs/ayastorm-r18-cloud-volumetric.md` §4 P0

> r14 (volumetric atmosphere) + r15 (godrays) + r16 (aerial perspective) + r17 (色温度) の上に積む A 軸第 5 弾 (= A 軸完走)。**(A) cloud shader の volumetric 化 (analytic + 軽量 raymarch) + (B) r17 helper を CLOUD_COLOR にも適用** する設計の実装可能性を、cloud shader 経路と既存 infrastructure から検証する Round。

---

## §1. 結論サマリ

P0 §4.1〜§4.6 (spec §4 P0) のすべての調査ターゲットが **r18 を P1 に進めて良い** という結論に着地。

1. **cloud shader は `class1/deferred/cloudsV.glsl` + `cloudsF.glsl` の 2 ファイルのみ** — cloudsV のコメント L70-73 が「class2/windlight/cloudsV.glsl も sync」と書いているが実体は存在しない (legacy 痕跡)、改修対象は 2 ファイルだけ
2. **CLOUD_COLOR uniform 注入点は `llsettingsvo.cpp:890` 1 箇所のみ** — r17 と違って scene path への追加注入なし、r17 helper を 1 行差し込むだけで色温度連動完成
3. **既存 cloud は完全 2D** (`sampler2D cloud_noise_texture` × 4 系統 UV、3D noise 無し) — 体積化には新規実装が必要、既存 2D cloud_noise_texture の slab 化 (= 視線方向に短い厚みで再 sample) が軽量で preset 互換高い
4. **shader switch enum は `AYA_VISUAL_REALISM_ENABLED` (r14) + `AYA_R16_AERIAL_PERSPECTIVE_ENABLED` (r16)** が `llshadermgr.h:127-128` に既存、r18 でも shader 側 raymarch gate のため **`AYA_R18_CLOUD_VOLUMETRIC_ENABLED` を新規追加が必要** (r17 では C++ 側 gate のみで shader 改修なしだったため不要だった)
5. **3 OS 共通性は r14 / r15 / r16 / r17 と同じく GLSL + C++ のみで成立** (`HAS_METAL` ヒット 0、Metal/HLSL 特殊化不要)
6. cloud texture の bind は `lldrawpoolwlsky.cpp::renderSkyCloudsDeferred` (L290) で `cloudshader->bind()` 後に既存通り、改修不要

P1 では (a) `cloudsF.glsl` (fragment) の最終 color 計算前に既存 4 系統 UV sample を slab 化、(b) `llsettingsvo.cpp:890` の `CLOUD_COLOR` push 直前で r17 helper を 1 行噛ます、の 2 箇所が中心。新規 3D noise GLSL 関数の導入は P1.a 着手後に体感不足なら検討。

---

## §2. 調査結果

### §2.1 cloud shader の系統

```
indra/newview/app_settings/shaders/class1/deferred/cloudsV.glsl  (vertex)
indra/newview/app_settings/shaders/class1/deferred/cloudsF.glsl  (fragment)
```

- class2/windlight/clouds* は **現存しない** (Glob で確認、`find -iname "clouds*"` で 2 ファイルのみ)
- cloudsV のコメント (L70-73) は **legacy 痕跡** — Firestorm の deferred 化過程で class2 系統が deprecated になり、削除済みのファイルへの sync 注釈だけ残った状態
- → 改修は class1/deferred の 2 ファイルのみで完結

#### cloudsV.glsl (vertex shader, 193 行)
- 入力 uniform: `lightnorm`, `sunlight_color`, `moonlight_color`, `ambient_color`, `blue_horizon`, `blue_density`, `haze_horizon`, `haze_density`, `cloud_shadow`, `density_multiplier`, `max_y`, `glow`, `sun_moon_glow_factor`, `cloud_color`, `cloud_scale`, `camPosLocal`
- 出力 varying: `vary_CloudColorSun`, `vary_CloudColorAmbient`, `vary_CloudDensity`, `vary_texcoord0-3`, `altitude_blend_factor`
- 処理: per-vertex で cloud に到達する `sunlight * exp(-light_atten * off_axis)` + cloud_color modulate を計算、ambient と合成して varying 出力
- L174: `vary_CloudColorSun = (sunlight * haze_glow) * cloud_color;` — ここで cloud_color uniform が乗る
- L175: `vary_CloudColorAmbient = tmpAmbient * cloud_color;`

→ **cloud color の Kelvin modulate は C++ 側 (`applySpecial` の CLOUD_COLOR uniform push 直前) で行えば、cloudsV の L174/L175 を変えずに反映される**。shader 不触で B 軸 (色温度連動) 完成。

#### cloudsF.glsl (fragment shader, 129 行)
- 入力 uniform: `cloud_noise_texture`, `cloud_noise_texture_next`, `blend_factor`, `cloud_pos_density1/2`, `cloud_scale`, `cloud_variance`
- 入力 varying: cloudsV からの 3 系統 (CloudColorSun / CloudColorAmbient / CloudDensity) + 4 系統 texcoord + altitude_blend_factor
- 処理:
  1. `cloudNoise(uv)` で `cloud_noise_texture` を 2 layer blend (L51-57)
  2. 4 系統 UV (`uv1-4`) で disturbance noise sampling、cloud_pos_density で UV offset (L62-83)
  3. `alpha1` = visible density (前後 2 系統の noise を combined、cloud_pos_density で boost、smoothstep 2 回、altitude_blend_factor)
  4. `alpha2` = self-shadow density (1 系統の noise を smoothstep、(1-α2) で sun attenuation)
  5. `color = cloudColorSun*(1-α2) + cloudColorAmbient` で final、clamp + scale 2x で HDR boost
- 出力: `frag_data[0] = vec4(color.rgb, alpha1)` (HAS_EMISSIVE 経路では frag_data[3] へ)

→ **体積化 (A 軸) は cloudsF の §4-5 (alpha1 計算と final color 合成) を slab raymarch 化する**。具体的には:
- 既存 4 系統 2D UV sample を「視線方向 (= camera → cloud plane の z 方向相当) に 4〜8 step で重ねる」slab 構造に変える
- 各 step で density accumulate + 軽量 self-shadow accumulate
- 既存 cloud_pos_density / cloud_scale / cloud_variance / cloud_shadow uniform は新方式でも意味を保つ係数として再解釈

### §2.2 CLOUD_COLOR uniform の注入点

grep `CLOUD_COLOR` (newview 全体) でヒット:

```
llsettingsvo.cpp:606   legacy[SETTING_CLOUD_COLOR] = ensure_array_4(...)   ← convertToLegacy、runtime 無関係
llsettingsvo.cpp:890   shader->uniform3fv(LLShaderMgr::CLOUD_COLOR, ...)   ← applySpecial、唯一 runtime 注入点
llsettingsvo.cpp:1012  //param_map[SETTING_CLOUD_COLOR] = ...              ← commented out、無効
llfloaterenvironmentadjust.cpp / llpaneleditsky.cpp                       ← EEP editor UI、runtime 無関係
```

pipeline.cpp 側に CLOUD_COLOR 注入 **なし** — r17 の SUNLIGHT_COLOR / AMBIENT のような 2 注入点問題なし。

→ **CLOUD_COLOR modulate は `llsettingsvo.cpp:890` 1 箇所で完結**。r17 helper を 1 行差し込むだけ。

### §2.3 cloud uniform 一覧

`llshadermgr.h:131-339` で cloud 関連 enum:

```cpp
CLOUD_SHADOW,                       //  "cloud_shadow"          ← cloud → sunlight 減衰率
CLOUD_COLOR,                        //  "cloud_color"           ← cloud 基本色 (preset 由来)
CLOUD_POS_DENSITY1,                 //  "cloud_pos_density1"    ← layer 1 の position xy + density z
CLOUD_POS_DENSITY2,                 //  "cloud_pos_density2"    ← layer 2 同上
CLOUD_SCALE,                        //  "cloud_scale"           ← 全体スケール (≈ zoom)
CLOUD_VARIANCE,                     //  "cloud_variance"        ← noise modulation 強度
```

これらは P1.a 体積化でも **input 契約として保つ** (preset 制作者の作った見た目を尊重)。slab raymarch では各 step の sampling 係数として再利用。

### §2.4 3D noise の入手手段

grep `texture3D|sampler3D|Worley|Perlin` で cloud 関連ヒット **なし** (postDeferredTonemap に Perlin/Worley らしき helper があるが cloud とは別文脈)。

→ 3D noise を導入するには:
- **(a) 既存 2D texture を slab 化 (recommended)**: `cloud_noise_texture` (2D) を視線方向に「短い厚みのスラブ」として複数 sample、3D noise 同梱不要、preset の cloud texture 差し替え互換維持。step 数 4〜8 で済む
- **(b) procedural 3D noise (Worley/Perlin) を新規 GLSL function 追加**: 完全 3D 体積感だが、noise 計算コストが per-step 4〜8x、step 8〜16 でかなり重い。配布サイズ増なし
- **(c) hybrid**: 2D texture を base にしつつ 3D noise で z 軸方向の variation を加える、コストは (a)+(b)/2 程度

P1.a は **(a) から開始**、体感不足なら (c) を Round 2 で検討する方針。3D noise asset 同梱は cost-benefit 合わないため不採用。

### §2.5 3 OS 共通性

- cloudsV/cloudsF は GLSL 標準構文のみ、`texture3D` / `sampler3D` 不使用、`#extension` 不要
- `HAS_METAL` / `.metal` ヒット **0** (newview/app_settings/shaders 配下、cloud 関連)
- r14/r15/r16/r17 と同じく **3 OS 共通の GLSL/C++ で改修可能**

### §2.6 個別 switch 配線

`llshadermgr.h:127-128` に AYA r14/r16 の switch enum 既存:

```cpp
AYA_VISUAL_REALISM_ENABLED,         //  "aya_visual_realism_enabled" <FS:AYA r14>
AYA_R16_AERIAL_PERSPECTIVE_ENABLED, //  "aya_r16_aerial_perspective_enabled" <FS:AYA r16>
```

r17 は C++ 側で helper の return identity gate のため shader uniform 不要だった。r18 は **shader 内部で raymarch path を gate するため**、新規 enum を追加:

```cpp
AYA_R18_CLOUD_VOLUMETRIC_ENABLED,   //  "aya_r18_cloud_volumetric_enabled" <FS:AYA r18>
```

llshadermgr.cpp 側にも対応する uniform 名文字列を追加 (`grep AYA_R16_AERIAL_PERSPECTIVE_ENABLED` で過去 pattern を参照、`llshadermgr.cpp` でヒット 0 だが llshadermgr.h enum を読み込む側で文字列 mapping が必要なため要確認)。

→ P1 着手時に r16 と同じ pattern で 1〜2 ファイル追加修正。

---

## §3. P1 着手の前提条件

- shader 改修は `cloudsF.glsl` (fragment) 中心、`cloudsV.glsl` (vertex) は cloud_color uniform を varying に流すだけなので最小限の不触
- C++ 改修は `llsettingsvo.cpp:890` (CLOUD_COLOR modulate 1 行) + `settings.xml` (`AYAR18CloudVolumetricEnabled` Boolean 1 件) + `llshadermgr.{h,cpp}` (switch enum + uniform 名 1 組)
- 体積化は既存 2D `cloud_noise_texture` の slab 化で開始、3D noise 導入は Round 2 候補
- master `AYAVisualRealismEnabled` + 個別 `AYAR18CloudVolumetricEnabled` の AND で raymarch path 有効化、OFF で完全に既存 flat path に戻る

---

## §4. リスク再評価 (spec §6 の事前更新提案)

| ID | リスク | Round 1 後ステータス |
|---|---|---|
| R1 | raymarch step が多すぎて GPU コスト顕在化 | **緩和**: step 4〜8 で開始、`cloudsF.glsl` の既存 4 系統 UV sample コストと同程度に抑える方針、early-out で empty space skip |
| R2 | 3D noise 入手で 3 OS 挙動差 | **解消**: P1.a は既存 2D `cloud_noise_texture` の slab 化で開始、3D noise 導入は Round 2 候補 — texture asset 同梱は永久 drop |
| R3 | 既存 preset の cloud 見た目が体積化で破壊される | **緩和**: `cloud_pos_density1/2` / `cloud_scale` / `cloud_variance` / `cloud_shadow` を slab raymarch でも係数として再利用、shader 内部で switch off 経路 (既存 flat sample) を保持して完全 backward compatible |
| R4 | r17 helper の cloud_color 適用で「昼間(レガシー)」の雲が変色 | **解消**: r17 helper の `KNOWN_SKY_LEGACY_MIDDAY` UUID pinpoint 除外がそのまま継承、追加対応不要 |
| R5 | cloud shader 2 系統 (class1/class2) の片方だけ書き換えると見た目割れる | **解消**: class2/windlight/clouds* は現存しない (Glob で確認)、class1/deferred の 2 ファイルのみが現役、片方落ち心配なし |
| R6 | 3 OS でビルドが通らない (macOS Metal cross-compile 等) | **解消**: `HAS_METAL` ヒット 0、shader は GLSL 標準構文のみ、C++ も r17 helper 再利用 + settings.xml + llshadermgr 1 組 |
| R7 | 個別 switch 追加で `feedback_prefer_defaults_over_config.md` と衝突 | **緩和**: r14/r16 と同じく章ごと sentinel 1 本の運用継続、A 軸完走時 (r18 close-out) に master へ吸収検討 |
| R8 | heavy raymarch に倒れて scope 膨張 | **解消**: spec §3 で「heavy raymarch」永久 drop、step 数上限を P1.c で固定する運用 |
| R9 | 体積化の体感が perceptual threshold 以下 (r16 P1.b / r14 P2.b/c 系の drop pattern) | **未着手**: P1.a 実機で判定、threshold 以下なら drop (色温度連動部分 P1.b は軽量なので残す方針で半保険) |

---

## §5. 次のアクション

P1.a 着手 (cloud shader 内部の slab raymarch + switch uniform 配線):

1. `llshadermgr.h` に `AYA_R18_CLOUD_VOLUMETRIC_ENABLED` enum 追加 (L127 周辺、r16 と同じ pattern)
2. `llshadermgr.cpp` に対応する uniform 名 mapping 追加 (もし必要なら、r16 と同じ pattern で grep して場所特定)
3. `app_settings/settings.xml` に `AYAR18CloudVolumetricEnabled` Boolean (default TRUE) 追加
4. `llsettingsvo.cpp::applySpecial` で `AYA_R18_CLOUD_VOLUMETRIC_ENABLED` uniform を push、ついでに CLOUD_COLOR push 直前で r17 helper を 1 行噛ます (B 軸 = 色温度連動)
5. `cloudsF.glsl` の `alpha1` 計算部分を slab raymarch 化、既存 4 系統 UV sample を視線方向 4〜8 step で重ねる、switch off 経路で既存 flat sample を保持
6. Linux 実機ビルド + 体感確認 (`feedback_shader_only_fast_iterate.md`: shader 変更だけなら `~/ayastorm/` cp で 10 分パッケージング回避できる、ただし llshadermgr 変更があるため初回はフルビルド必要)

P1.b (色温度連動の 1 行 modulate) は P1.a と同時か直後で着地可能。P1.c (体感調整) は P1.a/b 実機 PASS 後の判定。

---

> **本書の役割**: r18 P0 Survey Round 1 結果の凍結スナップショット。実装中に追加判明があれば §6 補遺で追記する (r17 Survey の §6/§7 補遺パターン継承)。
