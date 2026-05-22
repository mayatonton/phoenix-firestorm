# AYAstorm r18 — リリース告知

**r18 は視覚的リアリティ章 (r14〜r20) の第 5 弾、A 軸 (大気) 完走リリース**。既存 2D `cloud_noise_texture` を視線方向の軽量 slab raymarch で多 sample することで、雲が「平らな板」から厚みと奥行きを持つ立体に変わり、r17 の色温度連動と合わせて **cinematic な orange 夕焼け雲** が出ます。

> **配信形態**: r18 は **r23 リリースに同梱配信** されます (r18 単独タグは発行しません)。r23 リリースページから本ノートと r18 spec doc にリンクする運用です。

実装・既知 limits・設定の詳細は永続資料 (`docs/ayastorm-r18-cloud-volumetric.md`) に常駐させ、本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r18 — 雲の体積化 + 色温度連動

### r18 の柱: 雲に奥行きを、夕焼け雲に cinematic を

r14 → r17 で空気・光線・遠景 haze・夕焼けの暖色を積み上げてきました。r18 は「**写真撮るに値する空の核**」を取りに行きます:

- **雲が「板」じゃなくなる** — 既存 `cloud_noise_texture` (sampler2D) を視線方向に 4 step slab raymarch、厚みとエッジの立体感、内部 depth gradient を得る
- **夕焼け雲が cinematic に** — r17 の色温度 modulator (`getR17SunModulator`) を `CLOUD_COLOR` にも適用 (sustained viewing の評価で復活させた B 軸)、体積化された雲面に warm tone が乗る
- **新 asset 同梱なし** — 既存 2D noise texture を再利用、3D noise atlas 不要、preset 改修不要

実装は **heavy raymarch 不採用** (毎フレーム全画面 ray-march は章 roadmap §6 で永久 drop): cloud 1 pixel あたり N=4 fixed slab step、Beer-Lambert 風 transmittance (45%/slab)。計算コストは有界、AYA 実機評価「とても素晴らしい」かつ FPS 顕著低下なし。

### 中身

**A 軸 (雲体積化)** — `cloudsF.glsl` slab raymarch
- 既存 2D `cloud_noise_texture` を N=4 sample (UV 空間 slab offset `(0.013, 0.008)`/step、視線方向 proxy)
- per-slab transmittance を Beer-Lambert 風に累積
- `AYAR18CloudVolumetricEnabled` + master + `KNOWN_SKY_LEGACY_MIDDAY` pinpoint 除外で gate
- **OFF パスは legacy flat sample と数式上完全一致** (preset 互換は構造的に維持)

**B 軸 (CLOUD_COLOR × r17 modulator)** — r17 色温度の再適用
- `llsettingsvo.cpp::applySpecial` で `psky->getCloudColor() * getR17SunModulator()` を `CLOUD_COLOR` uniform に push
- 当初 r17 の一部 → r17 drop と同伴 drop → sustained viewing で「orange 夕焼けが失われた」と判定され **revival**
- `AYAR17ColorTemperatureEnabled` (r17 sentinel) + master + Legacy Midday pinpoint 除外で gate

### 設定

| Key | Default | 役割 |
|---|---|---|
| `AYAVisualRealismEnabled` | `1` (ON, U32) | 章 master switch — Preferences → Graphics → Shaders の `Firestorm View / AYAstorm View` combo_box で切替 |
| `AYAR18CloudVolumetricEnabled` | `1` (ON) | r18 A 軸 sentinel。`0` で flat 2D sample に戻る (r17 までと数式一致) |
| `AYAR17ColorTemperatureEnabled` | `1` (ON) | r17 由来 sentinel、本リリースで B 軸 CLOUD_COLOR mod も gate |

### View Mode UI

Preferences → Graphics → Shaders に章 master を combo_box (`AYAViewMode`) で露出:

- **Firestorm View** — `AYAVisualRealismEnabled = 0`、r14 以前の見え方に完全復帰
- **AYAstorm View** — `AYAVisualRealismEnabled = 1`、視覚的リアリティ章 ON (デフォルト)

(combo_box は r17 で master cvar の U32 昇格と同時に追加、r18 リリースで公開デビュー。)

### 既知の制約

- **A 軸効果は雲が多い preset (Cloudy / Sunset) で顕著**。Clear-sky preset は雲面積が小さく奥行きを表現する余地が少ない (定義上)
- **昼間 (レガシー) preset (`KNOWN_SKY_LEGACY_MIDDAY`) は pinpoint 除外** — A 軸は flat sample に fallback、B 軸 CLOUD_COLOR mod は identity。PBR 前 noon 再現 preset の意図を歪めない
- **地表の雲影は scope 外** — r18 で同梱検討したが scope 膨張回避のため r19+ に分離
- **A 軸効果は WindLight `cloud_pos_density` / `cloud_scale` と共存** — これらの preset uniform は引き続き意味を持ち、raymarch は preset が定義した雲量の「立体側面」を彫る形

### 実装概要

- `cloudsF.glsl` — N=4 slab raymarch、gated、OFF パスは数式互換のため温存
- `llsettingsvo.cpp::applySpecial` — A 軸 uniform push + B 軸 `CLOUD_COLOR × r17_sun_mod`
- `LLShaderMgr` — `AYA_R18_CLOUD_VOLUMETRIC_ENABLED` enum + reserved uniform 追加
- `settings.xml` — `AYAR18CloudVolumetricEnabled` Boolean default 1

### ドキュメント

- r18 spec / slab raymarch パラメータ / B 軸 revival 経緯: `docs/ayastorm-r18-cloud-volumetric.md`
- P0 cloud shader 調査: `docs/archive/r18/cloud_volumetric_survey.md`
- r17 spec (drop / revert の教訓、B 軸 revival の根拠): `docs/ayastorm-r17-color-temperature.md`
- 視覚的リアリティ章ロードマップ (A 軸完走 = r18): `docs/ayastorm-visual-realism-roadmap.md`
