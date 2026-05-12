# AYAstorm r17: 時間帯色温度 (Sun/Ambient の Kelvin 解釈)

**作成日**: 2026-05-12 (初版、r16 close-out 直後)
**対象**: AYAstorm `feature/aya-r17-color-temperature-spec-draft` (起票)
**位置づけ**: 視覚的リアリティ章 (`docs/ayastorm-visual-realism-roadmap.md`) §4 A 軸の第 4 弾、r14 (volumetric atmosphere) + r15 (godrays) + r16 (aerial perspective) の上に積む

> **本書の役割**: r17 個別の **計画スナップショット**。r16 close-out 時点では P0 起票のみ。`feedback_release_with_user_feedback.md` 流儀で「完璧な spec を組まず、shader 触りながら追記」運用。
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
- **sky shader (`skyV.glsl`) は本リリースで一切触らない** — r16 と同じ方針
- 改修対象は scene 側 (`atmosphericsFuncs.glsl::calcAtmosphericVars*`) または preset 解釈側 (`llinventory/llsettingssky.cpp`)
- sky dome の haze_glow / sun halo / atmospheric in-scatter は r14 P2.a refined の挙動を維持

### preset 互換の取り扱い (3 案検討、P0 で確定)
- **A 案**: preset の `sunlight_color` から逆算した「effective Kelvin」と、太陽 elevation から派生した「physical Kelvin」を blend。preset の絵作りを保ちつつ物理整合を取る
- **B 案**: 太陽 elevation のみから物理 Kelvin を直接派生、preset の `sunlight_color` は ignore (= preset 互換破壊)。実装は単純だが preset 資産を活かせないため `feedback_prefer_defaults_over_config.md` 流儀と相反
- **C 案**: preset の `sunlight_color` は targeted look として保ち、内部の連動変数 (例: ambient の cool/warm bias、Mie/Rayleigh 比率) のみ Kelvin に整合させる。preset 互換完全維持、内側だけ物理化
- **方針**: C 案を default、A 案を fallback (P0 Survey で実装可能性を確認、P1 で決定)

---

## 3. スコープ

### 含む
- **P0 Survey**: Sun/Ambient color の派生経路を実コード上で同定 — `llinventory/llsettingssky.cpp::calculateLightSettings()` / `atmosphericsFuncs.glsl::calcAtmosphericVars` / sky uniform binding
- **P1.a**: 太陽 elevation → Kelvin 派生曲線の shader / C++ 実装 (例: 朝 4000K / 昼 6500K / 夕 2500K の elevation 曲線、Planck 黒体放射 RGB 近似式)
- **P1.b** (条件付): preset `sunlight_color` との blend (C 案 or A 案で確定後)
- **C++ plumbing**: 個別 switch `AYAR17ColorTemperatureEnabled` (settings.xml / llshadermgr.{h,cpp} / llsettingsvo.cpp) — r16 の前例踏襲
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

### P0: 実装箇所調査 + spec 確定 — **着手前**

調査ターゲット (P0 完了条件):

1. **Sun/Ambient color の派生経路** — preset → uniform binding → shader 受け取り の流れを実コード上で同定
2. **`calculateLightSettings()` の挙動** — 太陽 elevation から Sun/Ambient/Halo 色を派生する既存ロジックを把握 (r17 で物理化する対象)
3. **shader 内での Sun/Ambient 使用箇所** — `atmosphericsFuncs.glsl::calcAtmosphericVars` / sky shader / fog の流入経路を確認
4. **preset の Kelvin 解釈可能性** — RGB から effective Kelvin を逆算する近似 (例: McCamy 1992) で preset 互換性を保てるか確認
5. **3 OS 共通性** — GLSL + C++ のみで 3 OS 通せる確認 (Metal/HLSL 特殊化なし)
6. **A/C 案の実装難度比較** — Survey Round 1 で C 案を default 採用するか、A 案も並行検討するか確定

成果物: `doc/r17/color_temperature_survey.md` (Round 1)。

### P1.a: Kelvin 曲線実装

太陽 elevation から物理 Kelvin を派生し、shader / C++ 側に注入。
- 朝 (elevation < 10°): 2500-4000K (warm amber)
- 昼 (elevation > 30°): 5500-6500K (neutral white)
- 夕 (elevation < 10°、sunset 側): 2000-3500K (deep amber)
- 滑らかな曲線 (smoothstep or piecewise)

Planck 黒体放射 RGB 近似 (例: Tanner Helland の Kelvin → RGB 近似式) を shader / C++ で使用。

### P1.b: preset blend (C/A 案確定後)

P0 で C 案 (内部連動変数のみ Kelvin 整合) or A 案 (preset と blend) を確定し、P1.b で実装。

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

| ID | リスク | 状態 |
|---|---|---|
| R1 | preset の `sunlight_color` を Kelvin で再計算すると、preset 制作者の絵作りが全て壊れる | 未着手 — C 案 (内部連動変数のみ Kelvin 整合) で preset 出力色は不触、A 案 (blend) なら preset 重みを高めて緩和 |
| R2 | Kelvin 曲線が AYA の体感する「写真的リアリティ」と乖離 (= 物理的には正しいが視覚効果薄、r16 P1.b 再演) | 未着手 — Planck 黒体放射 RGB 近似 + 太陽 elevation 曲線の妥当性は P1.a 実機で判定。`feedback_feature_value_in_main_usecase.md` (動いた ≠ 効いた) を念頭に置く |
| R3 | ambient の cool/warm bias が屋内 vs 屋外で異なる要求 (屋内は preset の indoor ambient を尊重すべき) | 未着手 — P0 で indoor ambient の取り扱いを確認 |
| R4 | sun disc が r14 P2.b/c 同様に色温度経路で消失副作用 | 未着手 — skyV.glsl 不触の方針 (r16 と同じ) で構造的保証 |
| R5 | FPS 影響 (Kelvin → RGB 変換が per-pixel になると重い) | 未着手 — C++ 側で per-frame に 1 回計算して uniform で配信、shader 内 per-pixel 変換は避ける |
| R6 | 3 OS でビルドが通らない (macOS Metal 等) | 未着手 — P0 で GLSL + C++ のみで通せる確認、P2 で実ビルド検証 |
| R7 | switch off 経路で見え方が完全に旧経路まで戻らない | 未着手 — `AYAR17ColorTemperatureEnabled = FALSE` で Kelvin 派生を完全 bypass する経路を P1.a で確保 |
| R8 | 個別 switch (`AYAR17ColorTemperatureEnabled`) を追加することで `feedback_prefer_defaults_over_config.md` (個別 cvar 量産しない) と衝突 | 緩和 — r14/r15/r16 と同じく「章ごと体感評価用 sentinel」運用、A 軸完走時に統合 (master へ吸収) を検討 |

---

## 7. 更新履歴

- 2026-05-12 (初版): r16 close-out (P1.b drop で実装完結) 直後に r17 を起票。旧 r17 (時間帯色温度 + 雲のリアリティ) を r17 (色温度) / r18 (雲の体積化) に分割した分の前半。色温度を先にする理由は「雲は色温度の影響を受ける側 (sun color が物理的に決まらないと雲の体積感も浮く)」のため。スコープは scene 側 (`atmosphericsFuncs.glsl` + `llinventory/llsettingssky.cpp::calculateLightSettings`)、skyV.glsl は r14 P2.b/c 教訓で不触。preset 互換は C 案 (内部連動変数のみ Kelvin 整合) を default、A 案 (preset と blend) を fallback として P0 Survey で確定する方針
