# AYAstorm r17 — リリース告知

GitHub release ページ貼り付け用の文案。**r17 は視覚的リアリティ章 (r14〜r20) の第 4 弾**で、太陽 elevation 駆動の色温度 modulator を導入し、太陽が地平線に近づくに従って sun / ambient / cloud color が orange 方向に温まる cinematic な夕焼け感を取り戻します。preset の値だけでは出ない「夕焼けの世界」を物理駆動で乗せる。

> **配信形態**: r17 は **r23 リリースに同梱配信** されます (r17 単独タグは発行しません)。r23 リリースページから本ノートと r17 spec doc にリンクする運用です。

実装・既知 limits・設定の詳細は永続資料 (`docs/ayastorm-r17-color-temperature.md`) に常駐させ、本ノートはそこへの誘導と差分ハイライトに徹します。

> **r17 の経緯メモ**: r17 は一度「instant A/B で効きが分からない」と判定されて drop されたが、drop 後の sustained viewing で **「オレンジの夕焼けの世界が失われた」** ことが明確になり、同日中に revert (実装は `c3d6aee734` の内容を完全復活)。**instant A/B と sustained viewing は別軸で評価する** (memory `feedback_instant_ab_vs_sustained.md`) — 即時切替で「変化が分からない」機能でも sustained 視聴で cumulative に効いていることがある、この教訓を反映した release です。

---

## AYAstorm r17 — 時間帯色温度

### r17 の柱: 夕焼けの暖かさを取り戻す

WindLight preset は色情報を持っていますが、SL の sun elevation カーブに物理的な色温度変調が無いため、Sunset preset で warm な値が来ても **太陽自体が dynamic に orange 方向へ深まらない**、ambient / cloud light が太陽方向の warm tone に追従しない、という写真感の欠落が起きていました。

r17 は単一のヘルパー `LLSettingsVOSky::getR17SunModulator(lightnorm, psky)` を追加:

- 太陽方向 elevation (`lightnorm.z`) を取り
- `t = smoothstep(0, 0.4, lightnorm.z)` で正規化
- `K = mix(2200, 6500, t)` で Kelvin (warm horizon → neutral midday)
- Tanner Helland 2012 公開式で Kelvin → RGB modulator を生成

この modulator を **3 注入点** で適用:

1. **sky path** (`llsettingsvo.cpp::applySpecial`): `SUNLIGHT_COLOR` + `CLOUD_COLOR` + ambient に乗算
2. **scene path** (`pipeline.cpp::setupHWLights`): `mSunDiffuse` + `gGL.setAmbientLightColor` 前の ambient に乗算
3. **cloud path B 軸** (r18 と共有): 上記 sky path で modulated `CLOUD_COLOR` が cloud shader に流れる経路、体積化された雲面 (r18) にも warm tone が乗る

### 効く preset / 効かない preset (preset 依存)

modulator は **物理駆動 (elevation driven)** であるため、太陽が実際に動くシーンで真価を発揮し、preset で太陽を zenith に固定したシーンでは意図的に subtle になります:

| Preset | sun elevation | K | Modulator | 効く場面 |
|---|---|---|---|---|
| Day cycle (estate time) | 0.0 → 1.0 動的 | 2200 → 6500 | strong amber → identity | **最大効果** — 日没に向けた cumulative warm-up |
| Sunset (固定) | 0 | 2200 | strong warm | **cinematic な orange 夕焼けが復活** (revert の動機) |
| Sunrise (固定) | 0.996 (zenith) | 6500 | identity | no-op (preset designer が太陽を真上に固定) |
| Midday (固定) | 0.37 | ≈6500 | near-identity | no-op |
| 昼間 (レガシー preset, `KNOWN_SKY_LEGACY_MIDDAY`) | — | — | identity (pinpoint 除外) | PBR 前 noon 再現 preset の意図を保護 |

「効く preset / 効かない preset が混在する」状態は `feedback_release_with_user_feedback.md` 流儀で受け入れる方針です。本ノートで明示しているのはユーザー期待値の調整目的。

### 設定

| Key | Default | 役割 |
|---|---|---|
| `AYAVisualRealismEnabled` | `1` (ON) | 章 master switch (r14〜r20)。本リリースで **U32 化** (Firestorm View=0 / AYAstorm View=1)、Preferences → Graphics → Shaders の `AYAViewMode` combo_box と直結 |
| `AYAR17ColorTemperatureEnabled` | `1` (ON) | r17 sentinel。`0` で modulator が identity (preset 色そのまま素通し) |

### Master cvar の U32 昇格

本リリースで `AYAVisualRealismEnabled` を **Boolean → U32** に変更 (default は依然 1)。理由は新規追加した `AYAViewMode` combo_box (`Firestorm View=0` / `AYAstorm View=1`) が U32 cvar とはきれいに binding するのに対し、Boolean cvar との LLSD coercion が不安定なため (memory `feedback_combo_box_u32_cvar.md`)。C++ 側 3 サイト (`llsettingsvo.cpp::applySpecial` ×2、`pipeline.cpp::doGodrays`) を `LLCachedControl<U32>` + `() != 0` 判定に更新。

### 実装概要

- `llsettingsvo.{h,cpp}` — `getR17SunModulator()` + `kelvinToRGB()` ヘルパー、3 注入点組込み
- `pipeline.cpp::setupHWLights` — scene-path modulator 適用
- `settings.xml` — `AYAR17ColorTemperatureEnabled` Boolean default 1、`AYAVisualRealismEnabled` を U32 化
- `panel_preferences_graphics1.xml` (en/ja) — Preferences に `AYAViewMode` combo_box 露出

### 既知の制約

- **instant A/B では subtle、sustained viewing で cumulative に効く**。drop/revert の経緯から得た教訓: この種の効果は即時切替ではなく sustained 視聴で評価する (memory `feedback_instant_ab_vs_sustained.md`)
- **「昼間 (レガシー)」preset は pinpoint 除外** — PBR 前 noon 再現 preset の意図を歪めない

### ドキュメント

- r17 spec / revert 記録 / Day cycle で効くが Sunrise で効かない理由: `docs/ayastorm-r17-color-temperature.md`
- r18 雲体積化 spec (B 軸 CLOUD_COLOR mod の同居先): `docs/ayastorm-r18-cloud-volumetric.md`
- 視覚的リアリティ章ロードマップ: `docs/ayastorm-visual-realism-roadmap.md`
- combo_box ↔ U32 cvar binding パターン: memory `feedback_combo_box_u32_cvar.md`
- instant A/B と sustained viewing の評価軸: memory `feedback_instant_ab_vs_sustained.md`
