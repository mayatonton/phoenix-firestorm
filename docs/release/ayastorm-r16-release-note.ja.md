# AYAstorm r16 — リリース告知

**r16 は視覚的リアリティ章 (r14〜r20) の第 3 弾**で、scene の aerial perspective 経路に波長依存 (Rayleigh λ⁻⁴) の in-scatter weighting を入れ、遠景が距離に応じて青味方向にシフトするようにします。WindLight preset 互換を保ったまま、**sky dome は一切触らず** (sun disc は構造的に保護)。

> **配信形態**: r16 は **r23 リリースに同梱配信** されます (r16 単独タグは発行しません)。r23 リリースページから本ノートと r16 spec doc にリンクする運用です。

実装・既知 limits・設定の詳細は永続資料 (`docs/ayastorm-r16-aerial-perspective.md`) に常駐させ、本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r16 — Aerial perspective (距離による色変化)

### r16 の柱: 遠景が空気の中に物理的に座る

r14 で「空気が体積として見える」、r15 で「光線が空間を貫く」と積んだ上で、r16 では **遠景が空気の中に物理的に座る** ところを取りに行きます。写真で山が距離と共に青くかすむあの絵:

- **遠くの山が青くかすむ** (Rayleigh 散乱 — 短波長が散乱で減衰、長距離経路で青味が強調される)
- **近景はほぼ変化なし** (atmosFragLighting が atten をスカラー化するため、波長依存は additive (in-scatter) 経由でのみ効く設計)
- **WindLight preset の値は input として再解釈**、preset 互換を維持

実装は **既存パイプラインを書き換えずに追加**:

- `atmosphericsFuncs.glsl::calcAtmosphericVars` に新たな `rayleigh_w = (1.0, 2.33, 5.71)` を `combined_haze` と `blue_weight` (in-scatter color) に乗算
- `light_atten` (太陽光路) には **適用しない** — 適用すると近景まで黄ばんで「常時夕焼け」化する副作用が出る (aerial perspective ≠ 夕焼けの物理的分離)
- **`skyV.glsl` は本リリースで一切触らない**。sun disc / 地平線 / haze_glow は r14 P2.a refined のまま温存 — r14 P2.b/c で出た sun disc 劣化を構造的に回避

### 試して drop した分 (意図的)

- **Preetham 1999 球面 sec(θ) 近似 (P1.b)**: 実装 + Linux 実機検証。数値的には差は出る (θ=89° で sec=57.3 → Preetham=26.5) が、AYA 体感で「穏やかになった感じは特にない」 — SL の sun timeline は地平線 ±5° を時間ステップで一気に跨ぐため perceptual threshold 以下。`feedback_feature_value_in_main_usecase.md` (動いた ≠ 効いた) に従い drop。後段の章で「sun disc 保護」を制約として太陽方向光路を再挑戦予定
- **Distance Multiplier 物理係数化**: deferred。P1.a で既に遠景青味シフトが出ており、preset 互換破壊リスクを取らない判断

### Master switch + 個別 sentinel

| Key | Default | 役割 |
|---|---|---|
| `AYAVisualRealismEnabled` | `1` (ON) | 章 master switch (r14〜r20)。`0` で r14 以前の挙動に完全復帰 |
| `AYAR16AerialPerspectiveEnabled` | `1` (ON) | r16 単独 sentinel。`0` で r15 までの挙動 (rayleigh_w = (1,1,1) で旧式と数式等価) |

個別 sentinel が要るのは、master を切ると r14/r15 効果も同時に消えて r16 単独の体感評価が不可能になるため。「個別 cvar を増やさない原則」(memory `feedback_prefer_defaults_over_config.md`) との折り合いとして、章評価期間の sentinel として r14/r15/r16 各 1 本に限定する運用、A 軸完走時 (r18) に master へ吸収を検討。

### 既知のトレードオフ

- **`atmosFragLighting` の atten スカラー化** (`light *= atten.r`): surface 直接透過の波長依存は effectively no-op。r16 で見える遠景青味シフトは **additive (in-scatter) 経由のみ** (`blue_weight` および `(1 - combined_haze)`)。将来の開発者が atten を per-channel と仮定しないよう spec §3 と memory `project_atmos_atten_scalarized.md` に明記
- **近景はほぼ変化しない** のは設計通り — `density_dist` が小さい近距離では additive 自体が薄く、Rayleigh weight が効くだけのレバレッジが無い (aerial perspective は本質的に長距離現象)

### 実装概要

- `atmosphericsFuncs.glsl` — `rayleigh_w` ternary を追加、`combined_haze` と `blue_weight` に乗算 (`light_atten` は意図的に不変)
- `LLSettingsVOSky::applyToShader` — `aya_r16_aerial_perspective_enabled` uniform plumbing
- `LLShaderMgr` enum + reserved uniform 名追加
- `settings.xml` — `AYAR16AerialPerspectiveEnabled` Boolean default 1

### ドキュメント

- r16 spec / パイプライン根拠 / drop した P1.b の経緯 / リスク登録: `docs/ayastorm-r16-aerial-perspective.md`
- P0 atmospheric pipeline 調査: `docs/archive/r16/aerial_perspective_survey.md`
- 視覚的リアリティ章ロードマップ (r14〜r20 全体): `docs/ayastorm-visual-realism-roadmap.md`
- atmosFragLighting atten scalarize メモ: memory `project_atmos_atten_scalarized.md`
