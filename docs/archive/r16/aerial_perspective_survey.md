# r16 aerial perspective P0 調査 — Round 1

**作成日**: 2026-05-12
**branch**: `feature/aya-r16-aerial-perspective-spec-draft`
**spec**: `docs/ayastorm-r16-aerial-perspective.md` §4 P0

> r14 (volumetric atmosphere) + r15 (godrays) の上に積む A 軸第 3 弾。**r14 で sun disc 消失副作用のため deferred した P2.b (Preetham 太陽方向経路長物理化) / P2.c (Rayleigh/Mie 波長依存分離) を、sun disc 保護と両立する形で scene 側に限定して復活させる** という設計判断の妥当性を、shader 経路の独立性と既存 infrastructure の有無から検証する Round。

---

## §1. 結論サマリ

P0 §4.1〜§4.6 (spec §4 P0) のすべての調査ターゲットが **r16 を P1 に進めて良い** という結論に着地。

1. **r14 P2.b/c の deferred は skyV.glsl 内 ("sky dome 上の太陽光減衰量") の話で、scene 側 (`atmosphericsFuncs.glsl`) に同じ式を入れても sun disc には影響しない** ことが構造的に保証される
2. **scene 経路と sky 経路は完全独立** (共有関数なし、共有ヘルパーなし、preset uniform のみ共通)
3. **Rayleigh/Mie 分離の注入箇所は atmosphericsFuncs.glsl 内に 2 箇所** に確定 (light_atten / combined_haze)
4. **Distance Multiplier はスカラー float uniform**、Beer-Lambert 指数で乗ずる係数として既に流入 — 新規 uniform 追加なしで物理係数化可能
5. **既存 SL に shader レベルの aerial perspective / Rayleigh / Mie 物理化は存在しない** (preset 側に Rayleigh/Mie 概念は EEP で存在するが shader uniform としては BlueDensity / HazeDensity に集約済)
6. **3 OS 共通性は r14 / r15 と同じく GLSL のみで成立** (`.metal` / `#ifdef GL_ES` / `HAS_METAL` すべて 0 ヒット)

P1 では `atmosphericsFuncs.glsl::calcAtmosphericVars*` の **内側書き換えのみ** で aerial perspective の物理化を実現する。新規 pass / FBO / uniform 追加なし、`AYAVisualRealismEnabled` master switch を r14 / r15 と共有。

---

## §2. 調査結果 (spec §4 P0 各項)

### §2.1 r14 P2.b/c deferred の正確な復元

r14 spec (§4 P2.b / P2.c) と commit `28727fa57f` (P2.a refined) の commit message から「sun disc 消失」が起きたことは判っていたが、**式変更がどの行で起きたか** を実コード上で同定する必要があった。

#### P2.c (Rayleigh/Mie 波長依存分離) の試行位置

`indra/newview/app_settings/shaders/class1/deferred/skyV.glsl:128-130`:

```glsl
// <FS:AYA r14 P2.c (deferred)> Rayleigh / Mie 分離は太陽 disc 消失副作用のため drop、
// 再設計時に「sun disc 保護」と両立する形で復活させる。
vec3 light_atten = (blue_density + vec3(haze_density * 0.25)) * (density_multiplier * max_y);
```

`light_atten` は次行で `sunlight *= exp(-light_atten * off_axis)` として消費される。これは **sky dome 上の太陽光減衰量** = sun disc が見えるかどうかを直接決める計算。ここに波長依存 RGB 比率 (1.0/2.33/5.71) で blue_density 部分を Rayleigh weight として重畳すると、太陽方向 (off_axis 小) で減衰の RGB バランスが崩れ sun disc が白飛びに覆われる形で消失した、と推定できる。

#### P2.b (Preetham 太陽方向経路長物理化) の試行位置

`indra/newview/app_settings/shaders/class1/deferred/skyV.glsl:138-141`:

```glsl
// <FS:AYA r14 P2.b (deferred)> Preetham (1999) 近似による太陽方向経路長の物理化は
// 太陽 disc 消失副作用のため drop。再設計時に「sun disc 保護」と両立する形で復活させる。
float off_axis = 1.0 / max(1e-6, max(0., rel_pos_norm.y) + lightnorm.y);
sunlight *= exp(-light_atten * off_axis);
```

平らな大気近似 `sec(theta)` を、Preetham (1999) の球面近似 `cos_zenith + 0.15 * pow(max(93.885 - theta_deg, 1.0), -1.253)` で finite 化しようとしたパス。これも `sunlight *= exp(-light_atten * off_axis)` に直接効くため、sun disc の見え方を破壊する。

#### 結論

**両方とも skyV.glsl の `sunlight *= exp(-light_atten * off_axis)` ライン** が震源。**`sunlight`** は sky dome 上の太陽光放射輝度であり、これを RGB 別 weight で減衰させると sun disc の RGB バランスが崩れて白飛び覆われ消失する、という構造。

r16 は **skyV.glsl を一切触らない** ことでこの震源を回避する。

---

### §2.2 scene 経路と sky 経路の完全独立性

`atmosphericsFuncs.glsl` (scene = PBR / deferred lighting / 不透明オブジェクト) と `skyV.glsl` (sky dome vertex) の **完全独立性** を以下の観点で確認:

| 観点 | atmosphericsFuncs.glsl | skyV.glsl |
|---|---|---|
| ファイル | `class1/windlight/atmosphericsFuncs.glsl` | `class1/deferred/skyV.glsl` |
| Entry point | `calcAtmosphericVars` / `calcAtmosphericVarsLinear` | `void main()` (sky dome vertex shader) |
| sRGB helper | `srgb_to_linear` / `linear_to_srgb` を **forward declare**、`srgbF.glsl` から実体取得 | `aya_srgb_to_linear` / `aya_linear_to_srgb` を **インライン定義** (vertex shader なので `srgbF.glsl` が attach されない) |
| 呼び出し元 | PBR / deferred / 不透明オブジェクト shader | sky dome vertex のみ |
| 共有関数 | **なし** | **なし** |
| 共有マクロ | なし | なし |
| 共有 uniform | preset uniform 群 (`blue_horizon` / `blue_density` / `haze_horizon` / `haze_density` / `distance_multiplier` / `density_multiplier` / `max_y` / `glow` / `lightnorm` / `sunlight_color` / `moonlight_color`) **のみ** | 同じ uniform 群、ただし値は共通でも計算経路は別 |

**= 値は共通だが計算式は独立**。`atmosphericsFuncs.glsl::calcAtmosphericVars*` に Rayleigh/Mie 分離 + Preetham off-axis を入れても、skyV.glsl の `sunlight *= exp(-light_atten * off_axis)` には一切影響しない。

これは r16 の核心的な設計前提を **構造的に保証する** 事実。

---

### §2.3 Rayleigh/Mie 分離の注入箇所 (atmosphericsFuncs.glsl 内 2 箇所)

`indra/newview/app_settings/shaders/class1/windlight/atmosphericsFuncs.glsl`:

#### 注入箇所 1: light_atten (L66-68)

```glsl
vec3 light_atten = (blue_density + vec3(haze_density * 0.25)) * (density_multiplier * max_y);
```

scene 側の太陽光減衰量。L78 で `sunlight *= exp(-light_atten * above_horizon_factor)` として scene 用 sunlight を計算。**ここで波長依存 weight (Rayleigh = 短波長強 / Mie = 中性) を blue_density に重畳** する。

#### 注入箇所 2: combined_haze (L72 + L98)

```glsl
vec3 combined_haze = max(blue_density + vec3(haze_density), vec3(1e-6));
// ...
combined_haze = exp(-combined_haze * density_dist * distance_multiplier);
```

scene 側の Beer-Lambert transmittance。**ここで波長依存 RGB 係数で transmittance を RGB 別にスペクトラル化** する。距離が伸びるほど短波長 (Rayleigh) の減衰が強くなり、遠景が青味方向にシフトする「aerial perspective」が物理的に出る。

#### 注入箇所 3 (オプション): off-axis 物理化

L77: `float above_horizon_factor = 1.0 / max(1e-6, lightnorm.y);`

これは scene 側の太陽方向経路長で、Preetham 球面近似で finite 化できる。ただし scene shader での太陽方向は sky dome ほど多様ではない (PBR / 不透明オブジェクト) ので、影響が小さい可能性。**P1 で実装し、必要なら省略する** 判断とする。

---

### §2.4 Distance Multiplier の現用法

`atmosphericsFuncs.glsl::calcAtmosphericVars`:

```glsl
combined_haze = exp(-combined_haze * density_dist * distance_multiplier);
```

- **型**: スカラー float uniform
- **意味**: Beer-Lambert 指数の係数。`density_dist = rel_pos_len * density_multiplier` (= 物理距離 × 密度) に対して、preset 経由でさらに伸縮する scale
- **preset 流入**: `LLSettingsVOSky::applyToShader` 経由で uniform に bind (`llsettingsvo.cpp` 直接 grep ヒット 0 だが、`LLShaderMgr` enum 経由で標準流入)
- **r16 での再解釈**: 現状は経験的な lerp 係数 (preset の絵作り意図を直接表現)。r16 では「大気の scale (scale height / total optical depth 換算)」として **物理係数のレンジに正規化** する案を P1 で検討

**新規 uniform 追加は不要** — 既存 `distance_multiplier` を物理係数として再解釈するだけで足りる。preset の生 lerp 値域 → 物理係数のマッピングは P1 で決定。

---

### §2.5 既存 SL aerial perspective infrastructure

grep 結果:

| パターン | ヒット先 | 評価 |
|---|---|---|
| `aerial.?perspective` | `lllegacyatmospherics.h` (誤検出: aerial の偶然マッチ) / `llimagefilter.{cpp,h}` (画像フィルタ系、無関係) | **shader 側に既存実装なし** |
| `\b(rayleigh\|mie)\b` (case insensitive) | `skyV.glsl` (r14 P2.c deferred コメント、AYA 改修) / `godraysF.glsl` (r15 Mie 前方ピーク phase コメント、AYA 改修) / `panel_settings_sky_density.xml` (各言語版 UI 文字列のみ) / `llinventory/llsettingssky.cpp` (EEP preset 系) | **shader 側に物理化既存実装なし、preset 側に概念は存在** |
| `wavelength\|scatter.*coeff` | ヒット 0 | **既存実装なし** |

**llsettingssky.cpp の Rayleigh / Mie 言及** (L309/314/392/397/1577):

```
LL_WARNS("SETTINGS") << "Rayleigh Config Validation errors: " << result["errors"] << LL_ENDL;
LL_WARNS("SETTINGS") << "Mie Config Validation errors: " << result["errors"] << LL_ENDL;
// Get total from rayleigh and mie density values for normalization
```

SL Extended Environment (EEP) には **preset レベルで Rayleigh / Mie density の概念が存在する**。しかし shader 側では `rayleigh` / `mie` 名の uniform は流れておらず、`BlueDensity` / `HazeDensity` / `BlueHorizon` / `HazeHorizon` に集約済み。

#### r16 P1 着手時の設計選択肢

A. **shader 内で blue_density / haze_density から RGB 波長依存比率 (1.0/2.33/5.71 等) で派生** — 新規 uniform 不要、preset 互換維持、ただし「物理的にどこまで Rayleigh / どこまで Mie か」は経験式
B. **llsettingssky.cpp から preset の生 rayleigh / mie 値を引き出して新規 uniform で shader に流す** — 物理的により正確、ただし新規 uniform 追加 + plumbing 増、`feedback_prefer_defaults_over_config.md` 方針 (個別 cvar 量産しない) との折り合いを検討

**P1 着手時は A を default**、必要なら B に拡張する方針。A の実装が体感に十分なら B はやらない。

---

### §2.6 3 OS 共通性

| パターン | 対象範囲 | ヒット数 |
|---|---|---|
| `\.metal\b` | `class1/windlight/` | 0 |
| `#ifdef GL_ES` | `class1/windlight/` | 0 |
| `HAS_METAL` | `class1/windlight/` | 0 |

r14 / r15 と同じく、改修対象は **GLSL のみ / `.metal` 無し / `#ifdef` 無し** で 3 OS 共通で通せる。3 OS ビルドリスクは r14 / r15 同等以下。

---

## §3. spec への反映ポイント

P1 着手前に r16 spec (`docs/ayastorm-r16-aerial-perspective.md`) を以下で更新:

1. **§4 P1 触るファイル** を確定: `atmosphericsFuncs.glsl` の **内側書き換えのみ** で完結 (`llsettingsvo.cpp` / `settings.xml` / `llshadermgr.{h,cpp}` 改修は **不要**、既存 `aya_visual_realism_enabled` を再利用)
2. **§4 P1 触る関数** を確定: `calcAtmosphericVars` / `calcAtmosphericVarsLinear` の **2 関数、light_atten + combined_haze の 2 箇所** に Rayleigh/Mie 分離注入、optional で above_horizon_factor の Preetham 物理化
3. **§3 含む** に「波長依存散乱の派生は blue_density / haze_density から shader 内で算出 (A 案)、preset の生 rayleigh / mie 値の引き出し (B 案) は P1 着手後に体感不足なら検討」を追記
4. **§6 リスク R8** を解消条件に格上げ: 「scene の太陽近傍方向 (view·sun ≈ 1 の領域) で白飛び」は **skyV.glsl を触らない設計で構造的に回避済**、P1 では view·sun 角度依存の Mie 前方ピーク (r15 godrays と同じ cos^N 形) のスケールだけ調整
5. **§6 リスク R1 (sun disc 消失副作用が scene 側で再発)** を P0 解消マーク

---

## §4. P1 着手前の最終チェック

P1 のフェーズ分解 (spec §4 P1 → 実装、§4 P2 → 3 OS ビルド + 体感、§4 P3 → tag / release):

- [x] r14 P2.b/c の deferred 経緯を実コード上で同定 (skyV.glsl `sunlight *= exp(-light_atten * off_axis)` 経路が震源、scene 側は独立)
- [x] scene aerial perspective と sky dome の経路完全分離を確認 (共有関数 / 共有ヘルパーなし、preset uniform のみ共通)
- [x] Rayleigh/Mie 分離の atmosphericsFuncs.glsl 内 注入箇所同定 (light_atten / combined_haze の 2 箇所、optional で above_horizon_factor)
- [x] Distance Multiplier の現用法と物理係数化方針確定 (新規 uniform 不要、既存 distance_multiplier の再解釈で足りる)
- [x] 既存 SL aerial perspective infrastructure の grep (shader 側に既存実装なし、preset 側に Rayleigh/Mie 概念は存在するが shader uniform 不在)
- [x] 3 OS 共通性確認 (GLSL のみ、`.metal` 無し、`#ifdef` 無し)

**Round 1 完了**。spec への反映 (§3) を済ませてから P1 着手。
