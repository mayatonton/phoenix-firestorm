# r17 時間帯色温度 P0 調査 — Round 1

**作成日**: 2026-05-12
**branch**: `feature/aya-r17-color-temperature-spec-draft`
**spec**: `docs/ayastorm-r17-color-temperature.md` §4 P0

> r14 (volumetric atmosphere) + r15 (godrays) + r16 (aerial perspective) の上に積む A 軸第 4 弾。**preset の `sunlight_color` / `ambient_color` を「太陽 elevation から派生する物理 Kelvin」として再解釈** する設計の実装可能性を、preset → uniform → shader の経路と既存 infrastructure から検証する Round。

---

## §1. 結論サマリ

P0 §4.1〜§4.6 (spec §4 P0) のすべての調査ターゲットが **r17 を P1 に進めて良い** という結論に着地。

1. **Sun/Ambient color の派生経路は C++ 側で一本化済** (`llsettingsvo.cpp::applySpecial`) — shader への uniform push 直前で modulate 可能
2. **`calculateLightSettings()` は sun **disc** 用** (`LLVOSky::calc()` で `mSun.setColor()`) **+ `getLightDiffuse()` 経由で `getSunlightColorClamped()` 系の参照経路** がある — Kelvin 注入は applySpecial 側のみで OK (`calculateLightSettings` には触れない方が影響範囲が読みやすい)
3. **shader 側使用箇所**: `atmosphericsFuncs.glsl` (scene path) と `skyV.glsl` (sky dome) の 2 経路、両方とも同じ `sunlight_color` / `ambient_color` uniform を読む
4. **preset の Kelvin 解釈は実行時逆算不要** — 太陽 elevation (`getLightDirection().z`) から物理 Kelvin 曲線を派生し、preset の `getSunlightColor()` に modulate factor を乗ずる C 案で preset 互換維持できる
5. **3 OS 共通性は r14 / r15 / r16 と同じく GLSL + C++ のみで成立** (Metal/HLSL 特殊化不要)
6. **個別 switch `AYAR17ColorTemperatureEnabled` 追加** は r16 の前例踏襲 (settings.xml / llshadermgr.{h,cpp} は不要 — shader 側で switch 値を読む必要なし、C++ 側で modulate を gate)

P1 では `llsettingsvo.cpp::applySpecial` の **既存 `SUNLIGHT_COLOR` / `AMBIENT` uniform push 直前** に Kelvin 派生 + modulate を入れる。shader 改修は **不要** (uniform 値そのものが Kelvin-tint される設計、shader はそれを既存通り消費)。

---

## §2. 調査結果

### §2.1 Sun/Ambient color の派生経路

`indra/newview/llsettingsvo.cpp::applySpecial()` (L749-829) が preset → shader uniform の橋渡し:

```cpp
// llsettingsvo.cpp:782-786 (sunlight)
LLVector3 sun_light_color  = LLVector3(psky->getSunlightColor().mV);
LLVector3 moon_light_color = LLVector3(psky->getMoonlightColor().mV);
shader->uniform3fv(LLShaderMgr::SUNLIGHT_COLOR, sun_light_color);
shader->uniform3fv(LLShaderMgr::MOONLIGHT_COLOR, moon_light_color);
```

```cpp
// llsettingsvo.cpp:729 (ambient)
draw_color(shader, getAmbientColor(), LLShaderMgr::AMBIENT);
```

- `applyToUniforms()` (L725) → SG_ANY shader group の `AMBIENT` uniform に preset 値 push
- `applySpecial()` (L749) → SG_SKY shader group の `SUNLIGHT_COLOR` / `MOONLIGHT_COLOR` / `CLOUD_COLOR` uniform に preset 値 push

**重要**: shader が受け取る `sunlight_color` / `ambient_color` uniform は **`calculateLightSettings()` の出力ではなく、preset の raw 値**。Beer-Lambert 等の物理計算は shader 側 (atmosphericsFuncs.glsl) で per-pixel 実行される設計。

→ r17 の Kelvin 注入箇所は **applySpecial / applyToUniforms の uniform push 直前** が最も自然。C++ 側で 1 frame 1 回計算、shader 改修不要。

### §2.2 `calculateLightSettings()` の役割

`indra/llinventory/llsettingssky.cpp:1707-1760`:

```cpp
void LLSettingsSky::calculateLightSettings() const
{
    LLColor3 sunlight = getSunlightColor();
    LLColor3 ambient  = getAmbientColor();
    // ... Beer-Lambert exp() attenuation を sunlight に適用
    componentMultBy(sunlight, componentExp((light_atten * -1.f) * lighty));
    componentMultBy(sunlight, light_transmittance);
    // ...
    mSunDiffuse = sunlight;   // ← 太陽 disc 色
    mSunAmbient = tmpAmbient;
    mHazeColor  = ...;
    mMoonDiffuse = ...;
    mTotalAmbient = ambient;
}
```

このメソッドが生成する `mSunDiffuse` は **太陽 disc** の色 (`LLVOSky::calc()` の `mSun.setColor(psky->getSunDiffuse())`、`llvosky.cpp:527`)。**shader uniform `sunlight_color` には流れない**。

→ r17 で `calculateLightSettings()` を触ると太陽 disc 色が変わる副作用。**触らない方が安全** (applySpecial 側のみで Kelvin 注入)。

ただし `getLightDiffuse()` (L1370-1383) も `getSunDiffuse()` を参照しているため、scene 直接光 (PBR/material shader が `getLightDiffuse` 経由で計算する場合) に間接的に影響する可能性あり → §2.6 で確認要。

### §2.3 shader 側使用箇所

`sunlight_color` / `ambient_color` uniform を読む shader:

- `indra/newview/app_settings/shaders/class1/windlight/atmosphericsFuncs.glsl`:
  - L27 `uniform vec3 sunlight_color;`
  - L30 `uniform vec3 ambient_color;`
  - L65 `vec3 sunlight = (sun_up_factor == 1) ? sunlight_color: moonlight_color;`
  - L149 `vec3 amb_color = ambient_color;`
- `indra/newview/app_settings/shaders/class1/deferred/skyV.glsl`:
  - L47 `uniform vec3 sunlight_color;`
  - L50 `uniform vec3 ambient_color;`
  - L124 `vec3 sunlight = (sun_up_factor == 1) ? sunlight_color : moonlight_color * 0.7;`
  - L173 `vec3 amb_lin = aya_srgb_to_linear(ambient_color);`
  - L176, L182, L183, L191 で `ambient_color` 直接参照

→ shader 改修なしで C++ 側 uniform 値を modulate するだけで **scene path と sky path の両方** に Kelvin が反映される。design 上は良。ただし「sky 不触」の spec §2 文言は「skyV.glsl のコード自体は触らない、ただし uniform 値は Kelvin modulate されるので sky にも色温度反映される設計」と更新が必要 (r16 までの skyV.glsl 不触は P2.b/c の sun disc 消失副作用回避の話で、色 modulate は別問題)。

### §2.4 preset の Kelvin 解釈可能性

C 案 (内部連動変数のみ Kelvin 整合) の具体実装案:

1. 太陽 elevation `e = getLightDirection().z` (= sin(altitude)、horizon=0, zenith=1)
2. 物理 Kelvin 曲線 `K(e)`:
   - `e < 0.05` (horizon、sunset/sunrise): K=2200-3500 (deep amber)
   - `0.05 < e < 0.3` (low sun): K=3500-5000 (warm)
   - `e > 0.3` (mid-high): K=5500-6500 (neutral noon)
   - smoothstep で滑らかに繋ぐ
3. Kelvin → RGB 近似式 (Tanner Helland 2012 公開式 or Mitchell Charity 1995 黒体テーブル線形補間):
   ```
   // Tanner Helland 2012 公開式 (簡略版)
   float t = K / 100.0;
   float r = (K <= 6600) ? 1.0 : pow(t-60, -0.1332);
   float g = (K <= 6600) ? 0.39 * log(t) - 0.63 : 1.292 * pow(t-60, -0.0755);
   float b = (K >= 6600) ? 1.0 : (K < 2000 ? 0.0 : 0.543 * log(t-10) - 1.196);
   ```
4. preset の `getSunlightColor()` に Kelvin RGB を **modulate** (= 乗算) する形で適用:
   - 物理 Kelvin RGB をそのまま使うと preset の絵作りが完全に破壊される (B 案)
   - 代わりに `kelvin_rgb / kelvin_rgb_at_noon` の **相対 modulator** を計算し preset 色に乗ずる (C 案)
   - これにより preset の「絵作り」は noon 基準で保たれ、朝夕は Kelvin で物理的に warm 化

C++ 側で 1 frame 1 回計算 (lightnorm から K, K→RGB, modulator) するだけで足りる、shader 側計算不要。

### §2.5 個別 switch 配線

r16 で C++ 側 plumbing が完成済 (`AYAR16AerialPerspectiveEnabled`)。r17 では shader uniform 経由ではなく **C++ 側で gate** するため、settings.xml の Boolean 1 件追加のみで足りる:

```xml
<key>AYAR17ColorTemperatureEnabled</key>
<map>
  <key>Comment</key>
  <string>r17 Color Temperature: ...</string>
  <key>Persist</key><integer>1</integer>
  <key>Type</key><string>Boolean</string>
  <key>Value</key><integer>1</integer>
</map>
```

`llsettingsvo.cpp::applySpecial` 内で:
```cpp
static LLCachedControl<bool> aya_r17(gSavedSettings, "AYAR17ColorTemperatureEnabled", true);
if (aya_visual_realism && aya_r17) {
    // Kelvin modulate sun_light_color, ambient
}
```

shader 側 uniform / llshadermgr 不要 (r16 と異なる、shader 改修なしの利点)。

### §2.6 3 OS 共通性

- `applySpecial` は llinventory/llsettingsvo (純 C++、OS 非依存)
- Kelvin → RGB 計算も標準 math (pow/log)、3 OS 同じ
- shader 改修なしのため GLSL/Metal/HLSL 差異の影響なし

`*.metal` / `#ifdef HAS_METAL` ヒットなし (r14/r15/r16 と同じ)。

### §2.7 getLightDiffuse 経由の間接影響 (要確認)

`getLightDiffuse()` (`llsettingssky.cpp:1370-1383`) は `mSunDiffuse` (= `calculateLightSettings()` 出力) を返す。これを参照する箇所が PBR/material shader の lighting 計算に流れていないか要確認。

grep 結果 (`indra/newview/`):
- `llvosky.cpp:527` `mSun.setColor(psky->getSunDiffuse())` — 太陽 disc のみ
- それ以外の参照経路は未調査 → Survey Round 2 で詳細化、または P1.a 実装で実機確認

**仮判定**: r17 P1.a は applySpecial の uniform 直前 modulate のみで開始、`calculateLightSettings()` は触らない。実機で sun disc 色 / scene 直接光に副作用がなければ Round 2 は不要。

---

## §3. P1 着手の前提条件

1. **`AYAVisualRealismEnabled` = TRUE + `AYAR17ColorTemperatureEnabled` = TRUE** で Kelvin modulate を有効化、両 FALSE で旧経路に戻る
2. **shader 改修なし** (`atmosphericsFuncs.glsl` / `skyV.glsl` のコードは不触、uniform 値だけ C++ 側で modulate)
3. **C++ 改修は applySpecial の 1 箇所のみ** + settings.xml 1 件
4. **Kelvin → RGB は Tanner Helland 公開式** で開始、体感不足なら Mitchell Charity テーブル線形補間に切替
5. **modulate は preset 色との乗算** (C 案)、preset 絵作りは noon 基準で保持

P1.a で実装後、AYA Linux 実機で体感確認 — 主リスク R2 (r16 P1.b 再演) を念頭に置く。

---

## §4. リスク再評価 (spec §6 の事前更新提案)

| ID | Round 1 で得た情報 | 提案 status 更新 |
|---|---|---|
| R1 preset 破壊 | C 案 modulate (preset 色 × Kelvin modulator) で preset 絵作りは noon 基準で保持される構造 | **緩和** (構造的に preset 互換維持) |
| R2 体感薄 (r16 P1.b 再演) | Kelvin 曲線の係数次第。Tanner Helland 単独で薄い場合 Mitchell Charity table に切替の余地あり | **未着手**、P1.a 実機で判定 |
| R3 屋内 ambient | `getReflectionProbeAmbiance() != 0.f` 分岐 (llsettingsvo.cpp:841) に屋内/屋外判定がある可能性 | **要追加調査** Round 2 で確認 |
| R4 sun disc 副作用 | `calculateLightSettings()` 不触 + uniform path のみ modulate で sun disc 経路独立 | **解消** (構造的保証) |
| R5 FPS | C++ 1 frame 1 回計算、shader per-pixel コストなし | **解消** |
| R6 3 OS | C++ + shader 改修なしで GLSL/Metal/HLSL 差異なし | **解消** |
| R7 switch off 経路 | C++ if gate で modulate を bypass、uniform 値は preset 生値 | **解消** (構造的保証) |
| R8 個別 cvar 量産 | settings.xml 1 件追加、運用方針は r14/r15/r16 と同じ | **緩和** |

---

## §5. 次のアクション

1. spec §3 / §4 / §6 を Round 1 結果で更新 (skyV.glsl の文言、Kelvin 派生具体案、リスク更新)
2. P1.a 着手 (applySpecial に Kelvin modulate 追加 + settings.xml + AYA Linux 実機検証)
3. Round 2 (必要なら): R3 屋内 ambient / R2 体感不足時の Kelvin 曲線 fallback
