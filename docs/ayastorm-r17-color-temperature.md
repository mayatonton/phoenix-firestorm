# AYAstorm r17: 時間帯色温度 (Sun/Ambient の Kelvin 解釈)

**作成日**: 2026-05-12 (初版、r16 close-out 直後)
**対象**: AYAstorm `feature/aya-r17-color-temperature-spec-draft` (P0 Survey Round 1 完了)
**位置づけ**: 視覚的リアリティ章 (`docs/ayastorm-visual-realism-roadmap.md`) §4 A 軸の第 4 弾、r14 (volumetric atmosphere) + r15 (godrays) + r16 (aerial perspective) の上に積む

> **本書の役割**: r17 個別の **計画スナップショット**。P0 Survey Round 1 (`doc/r17/color_temperature_survey.md`) 完了に合わせて scope / risks を実コードベースで精緻化済。`feedback_release_with_user_feedback.md` 流儀で「完璧な spec を組まず、shader 触りながら追記」運用。
> 章全体の位置づけは `docs/ayastorm-visual-realism-roadmap.md`、r14 spec は `docs/ayastorm-r14-volumetric-atmosphere.md`、r15 spec は `docs/ayastorm-r15-godrays.md`、r16 spec は `docs/ayastorm-r16-aerial-perspective.md`。

---

## 1. ゴール

r14 で「**空気が体積として見える**」、r15 で「**光線が空間を貫く**」、r16 で「**遠景が空気の中に物理的に座る**」を積んだ。r17 では **時間帯の色が物理的に決まる** ところを取りに行く。具体的に出したい体感:

- **朝 (dawn) の冷たい青** から **昼 (noon) のニュートラル白** へ、**夕 (dusk) の深い amber/橙** へ滑らかに移行する色温度曲線
- 同じ preset でも太陽 elevation に対して **物理的に整合する** Sun color の振る舞い
- ambient (天空光) と sunlight が **色温度で連動** し、夕方は ambient も暖色寄り (= 天空が橙)、朝は ambient も冷色寄り (= 天空が青) に追随

技術的には、preset の `sunlight_color` / `ambient_color` を **「物理 Kelvin から派生した色」として再解釈** する path を入れる。preset 値そのものは input 契約として保つが、内側で太陽 elevation から派生する Kelvin 曲線と blend / 再計算することで、preset 制作者の経験式に頼らない物理的整合性を取る。

> SL における時間帯色の位置づけ: WindLight / EEP preset の Sun Color / Ambient / Blue Horizon は preset 制作者の経験で作られた RGB 三値。太陽 elevation との物理的関係はゆるく、preset 切替時に色相が不連続にジャンプすることがある。r17 はこれを「物理 Kelvin の曲線」で滑らかに再解釈する枠。

---

## 2. 設計制約

`docs/ayastorm-visual-realism-roadmap.md` §2 の境界条件をそのまま継承:

- **保つ**: WindLight preset 互換、HDR scene buffer 骨格、PBR shader interface、sky dome (skyV.glsl) の見え方
- **書き換える**: `atmosphericsFuncs.glsl` の sunlight / ambient 派生計算 (および必要なら `llinventory/llsettingssky.cpp::calculateLightSettings`)
- **言い換え**: preset (`sunlight_color` / `ambient_color`) を input、Kelvin から派生する物理色温度として再解釈、出力契約 (HDR scene buffer の信号特性) は保つ

### Master switch + 個別 switch
- master `AYAVisualRealismEnabled` (r14 から) を共有
- 個別 switch `AYAR17ColorTemperatureEnabled` (default TRUE) を追加 — r16 と同様に master と独立 toggle 可能 (master ON 前提で r17 のみ on/off で体感評価)
- `feedback_prefer_defaults_over_config.md` (個別 cvar 量産しない) に対しては r14/r15/r16/r17 章ごと sentinel 各 1 本の運用方針継続

### sun disc 保護 (r14 P2.b/c の副作用を踏まない)
- **sky shader (`skyV.glsl`) の GLSL コード自体は本リリースで一切触らない** — r16 と同じ方針
- ただし P0 Round 1 で確認: `skyV.glsl` も `sunlight_color` / `ambient_color` uniform を読むため、**C++ 側で uniform 値を Kelvin modulate すると sky にも色温度反映される** 設計。これは意図通り (時間帯色の物理的整合を sky と scene で同期させる目的)
- sun disc 消失副作用 (r14 P2.b/c) は exp() attenuation で disc 形状を消す問題で、色 modulate (Kelvin tint) では発生しない構造
- `calculateLightSettings()` は不触: `mSunDiffuse` 経由で太陽 disc の色変更副作用を避けるため、Kelvin 注入は `applySpecial` の uniform push 直前のみで実施

### preset 互換の取り扱い (3 案検討、P0 で確定)
- **A 案**: preset の `sunlight_color` から逆算した「effective Kelvin」と、太陽 elevation から派生した「physical Kelvin」を blend。preset の絵作りを保ちつつ物理整合を取る
- **B 案**: 太陽 elevation のみから物理 Kelvin を直接派生、preset の `sunlight_color` は ignore (= preset 互換破壊)。実装は単純だが preset 資産を活かせないため `feedback_prefer_defaults_over_config.md` 流儀と相反
- **C 案**: preset の `sunlight_color` は targeted look として保ち、内部の連動変数 (例: ambient の cool/warm bias、Mie/Rayleigh 比率) のみ Kelvin に整合させる。preset 互換完全維持、内側だけ物理化
- **方針**: C 案を default、A 案を fallback (P0 Survey で実装可能性を確認、P1 で決定)

---

## 3. スコープ

### 含む
- **P0 Survey**: 完了 (`doc/r17/color_temperature_survey.md` Round 1) — Sun/Ambient の派生経路は `llsettingsvo.cpp::applySpecial` に一本化、shader 改修不要で C++ 側 modulate のみで完結することを確認
- **P1.a**: 太陽 elevation → Kelvin 派生曲線の **C++ 実装のみ** (`applySpecial` の SUNLIGHT_COLOR / AMBIENT uniform push 直前に modulate を挿入)。Tanner Helland 2012 Kelvin→RGB 近似式で開始、preset 色との **乗算 modulator** で C 案 (preset 互換維持) を実現
- **C++ plumbing**: settings.xml に `AYAR17ColorTemperatureEnabled` Boolean 1 件追加。**shader uniform / llshadermgr.{h,cpp} は不要** (C++ 側で switch gate するため、r16 と異なり shader 側の switch uniform 不要)
- **3 OS (Linux / macOS / Windows) ビルド + 体感確認** (P2)

### 含まない (→ r18+)
- **雲の体積化** (r18) — 別リリースに分離 (色温度の影響を受ける側、色温度が先決まる必要)
- **sky dome (skyV.glsl) の物理化** — r17 でも一切触らない (r14 P2.b/c 教訓)
- 物質側 subsurface scattering (B 軸、r19+)
- カメラ表現 (DoF / auto-exposure / scene-referred 露出階調、C 軸 r21+)
- preset 制作・新 preset 出荷 (AYA 方針: preset を作り直さない)

### 永久 drop
- preset を破壊する後方非互換変更 (B 案単独はやらない)
- LUT / color grade による「色温度演出」誤魔化し (`docs/ayastorm-visual-realism-roadmap.md` §6 と整合)
- 重い per-frame full-screen color grading pass

---

## 4. フェーズ分解

viewer-only の改修。配信側 / SIM 側変更なし。

### P0: 実装箇所調査 + spec 確定 — **完了** (Survey Round 1: `doc/r17/color_temperature_survey.md`)

調査ターゲット (P0 完了条件) と結果:

1. **Sun/Ambient color の派生経路** — `llsettingsvo.cpp::applySpecial` (L749-) で preset 値を uniform push、`calculateLightSettings()` の出力 `mSunDiffuse` は太陽 disc 用で shader uniform には流れない構造を確認 (Survey §2.1 / §2.2)
2. **`calculateLightSettings()` の挙動** — `mSunDiffuse` は `LLVOSky::calc()` で太陽 disc 色に使用、shader uniform は preset 生値が流れる。**r17 では `calculateLightSettings()` 不触** で applySpecial の uniform push 直前 modulate のみとする方針確定 (Survey §2.2)
3. **shader 内での Sun/Ambient 使用箇所** — `atmosphericsFuncs.glsl` (scene path, L27/L30/L65/L149) と `skyV.glsl` (sky dome, L47/L50/L124/L173 等) の 2 経路、両方が同じ uniform を読む。C++ 側 modulate で両方同時反映 (Survey §2.3)
4. **preset の Kelvin 解釈可能性** — C 案 (Kelvin modulator × preset 色の乗算) で実装可能、preset 絵作りは noon 基準で保持、朝夕は Kelvin で物理 warm 化。Tanner Helland 2012 公開式で開始 (Survey §2.4)
5. **3 OS 共通性** — C++ + math standard + shader 改修なしで GLSL/Metal/HLSL 差異なし、r14/r15/r16 と同じく `.metal` / `HAS_METAL` ヒットなし (Survey §2.6)
6. **個別 switch 配線** — settings.xml に Boolean 1 件追加、`llsettingsvo.cpp::applySpecial` 内で `LLCachedControl<bool>` で gate。**llshadermgr.{h,cpp} / shader uniform は不要** (r16 と異なり shader 側に switch を渡さない、C++ 側で modulate を bypass する経路で switch off 実現) (Survey §2.5)

**P1 着手の前提条件 (Survey §3)**:
- shader 改修なし (`atmosphericsFuncs.glsl` / `skyV.glsl` のコードは不触、uniform 値だけ C++ 側で modulate)
- C++ 改修は applySpecial の 1 箇所のみ + settings.xml 1 件
- Kelvin → RGB は Tanner Helland 公開式で開始、体感不足なら Mitchell Charity テーブル線形補間に切替
- modulate は preset 色との乗算 (C 案)、preset 絵作りは noon 基準で保持

**未確認 (Round 2 候補)**:
- `getLightDiffuse()` 経由の PBR/material shader への間接影響 (Survey §2.7) — P1.a 実機で sun disc / scene 直接光に副作用なければ Round 2 不要
- 屋内 ambient (`getReflectionProbeAmbiance() != 0.f` 分岐) の Kelvin 適用妥当性 (R3)

### P1.a: Kelvin modulate 実装 (C++ 単独、shader 改修なし)

太陽 elevation から物理 Kelvin を派生し、`applySpecial` の uniform push 直前で modulate:

- 朝 (elevation < 10°): 2500-4000K (warm amber)
- 昼 (elevation > 30°): 5500-6500K (neutral white)
- 夕 (elevation < 10°、sunset 側): 2000-3500K (deep amber)
- 滑らかな曲線 (smoothstep or piecewise)

実装イメージ (`llsettingsvo.cpp::applySpecial`):
```cpp
static LLCachedControl<bool> aya_visual_realism(gSavedSettings, "AYAVisualRealismEnabled", true);
static LLCachedControl<bool> aya_r17(gSavedSettings, "AYAR17ColorTemperatureEnabled", true);

LLVector3 sun_light_color = LLVector3(psky->getSunlightColor().mV);
LLVector3 ambient         = LLVector3(getAmbientColor().mV);

if (aya_visual_realism && aya_r17) {
    float elevation = LLEnvironment::instance().getClampedLightNorm().mV[2]; // sin(altitude)
    float kelvin    = kelvinFromElevation(elevation);         // 2500-6500
    LLVector3 mod   = kelvinModulator(kelvin);                // = kelvin_rgb(K) / kelvin_rgb(6500)
    sun_light_color = component_mult(sun_light_color, mod);
    ambient         = component_mult(ambient,         mod);   // ambient も連動
}

shader->uniform3fv(LLShaderMgr::SUNLIGHT_COLOR, sun_light_color);
// AMBIENT も同様に modulate して push
```

Tanner Helland 2012 公開式 (Kelvin → RGB) を C++ helper として実装。

### P1.b: 実装後の体感調整 (条件付)

P1.a の Linux 実機検証で:
- 体感が r16 P1.b のように perceptual threshold 以下 → drop (`feedback_feature_value_in_main_usecase.md`)
- 体感が出るが過剰/不足 → Kelvin 曲線の係数調整、Mitchell Charity テーブルへの切替、屋内 ambient の専用処理 (R3) 等を Round 2 で追加調査

### P2: 3 OS ビルド + 体感確認

AYA が Linux フルビルド + 体感確認。問題なければ macOS / Windows ビルドへ。`feedback_release_with_user_feedback.md` の流儀。

### P3: tag / release

r15 / r16 と同じく、**公開は r17 単独でせず後続リリースまで pending** の運用 (AYA 方針)。r18 (雲の体積化) と一括、または A 軸完走時に公開する想定。

---

## 5. 受け入れ条件

- [ ] 既存 WindLight preset (朝・昼・夕・夜) が **読み込めて、preset 切替が機能する** (preset 互換破壊なし)
- [ ] `AYAR17ColorTemperatureEnabled = TRUE` (master `AYAVisualRealismEnabled = TRUE` 前提) で:
  - 朝の太陽光が **冷色寄りに**、夕方が **暖色寄りに** 物理的整合した形で出る
  - ambient (天空光) が sunlight の色温度に **連動して** 朝青/夕橙のシフトを示す
  - r14/r15/r16 で出した体感が壊れない
- [ ] `AYAR17ColorTemperatureEnabled = FALSE` で r16 までと同じ見え方に戻る (Kelvin 派生を bypass する経路)
- [ ] **sky dome の見え方が r14 P2.a refined のまま** (sun disc 健在、朝・夕の地平線・青空質感は劣化なし) — skyV.glsl 不触で構造的保証
- [ ] 3 OS でビルド + 起動 + 表現確認 (P2)
- [ ] FPS 影響が ±10% 以内 (P2 で実測)

---

## 6. リスク

| ID | リスク | P0 Round 1 後ステータス |
|---|---|---|
| R1 | preset の `sunlight_color` を Kelvin で再計算すると、preset 制作者の絵作りが全て壊れる | **緩和**: C 案 (preset 色 × Kelvin modulator の乗算) で preset 絵作りは noon 基準で保持される構造、Survey §2.4 で確認 |
| R2 | Kelvin 曲線が AYA の体感する「写真的リアリティ」と乖離 (= 物理的には正しいが視覚効果薄、r16 P1.b 再演) | **未着手**: P1.a 実機で判定、Tanner Helland 単独で薄い場合 Mitchell Charity table への切替余地あり。`feedback_feature_value_in_main_usecase.md` (動いた ≠ 効いた) を念頭 |
| R3 | ambient の cool/warm bias が屋内 vs 屋外で異なる要求 (屋内は preset の indoor ambient を尊重すべき) | **要追加調査** Round 2: `getReflectionProbeAmbiance() != 0.f` 分岐 (llsettingsvo.cpp:841) に屋内/屋外判定がある可能性、P1.a で屋内シーンが不自然なら Round 2 で確認 |
| R4 | sun disc が r14 P2.b/c 同様に色温度経路で消失副作用 | **解消**: `calculateLightSettings()` 不触 + uniform path のみ modulate、Survey §2.2 で sun disc 経路 (`mSunDiffuse` → `LLVOSky::calc`) が shader uniform path と独立を確認 |
| R5 | FPS 影響 (Kelvin → RGB 変換が per-pixel になると重い) | **解消**: C++ 側 per-frame 1 回計算、shader per-pixel コストなし (Survey §2.6) |
| R6 | 3 OS でビルドが通らない (macOS Metal 等) | **解消**: shader 改修なし + 標準 math のみ、`.metal` / `HAS_METAL` ヒット 0 (Survey §2.6) |
| R7 | switch off 経路で見え方が完全に旧経路まで戻らない | **解消**: C++ if gate で modulate を bypass、uniform 値は preset 生値を直接 push する構造 (Survey §2.5) |
| R8 | 個別 switch (`AYAR17ColorTemperatureEnabled`) を追加することで `feedback_prefer_defaults_over_config.md` (個別 cvar 量産しない) と衝突 | **緩和**: r14/r15/r16 と同じく「章ごと体感評価用 sentinel」運用、A 軸完走時に統合 (master へ吸収) を検討 |
| R9 (Round 1 新規) | `getLightDiffuse()` 経由で PBR/material shader が `mSunDiffuse` を間接参照している可能性 | **要追加調査**: Survey §2.7、P1.a 実機で sun disc / scene 直接光に副作用なければ Round 2 不要 |

---

## 7. 更新履歴

- 2026-05-12 (初版): r16 close-out (P1.b drop で実装完結) 直後に r17 を起票。旧 r17 (時間帯色温度 + 雲のリアリティ) を r17 (色温度) / r18 (雲の体積化) に分割した分の前半。色温度を先にする理由は「雲は色温度の影響を受ける側 (sun color が物理的に決まらないと雲の体積感も浮く)」のため。スコープは scene 側 (`atmosphericsFuncs.glsl` + `llinventory/llsettingssky.cpp::calculateLightSettings`)、skyV.glsl は r14 P2.b/c 教訓で不触。preset 互換は C 案 (内部連動変数のみ Kelvin 整合) を default、A 案 (preset と blend) を fallback として P0 Survey で確定する方針
- 2026-05-12 (P0 Survey Round 1 完了): `doc/r17/color_temperature_survey.md` で 6 項目クリア。**重要な設計判明**: (a) shader uniform `sunlight_color` には preset 生値が流れる (`applySpecial` 経由)、`calculateLightSettings()` 出力 `mSunDiffuse` は太陽 disc 専用、(b) `applySpecial` 1 箇所で SUNLIGHT_COLOR / AMBIENT uniform を modulate するだけで scene + sky の両方に色温度反映できる、(c) **shader 改修不要 / llshadermgr.{h,cpp} 不要**、settings.xml 1 件 + applySpecial 1 箇所のみで完結、(d) C 案 (preset 色 × Kelvin modulator の乗算) で preset 絵作りを noon 基準で保持、(e) Tanner Helland 2012 Kelvin→RGB 公開式で開始。spec §2/§3/§4 を Round 1 結果で更新、リスクは R1/R4/R5/R6/R7 を解消、R3/R9 を Round 2 候補として記録。次は P1.a (applySpecial に Kelvin modulate 追加 + Linux 実機検証)
