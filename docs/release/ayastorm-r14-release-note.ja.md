# AYAstorm r14 — リリース告知

GitHub release ページ用の文案。**r14 は視覚的リアリティ章 (r14〜r20) の開幕リリース** で、SL の既存 Beer-Lambert + in-scatter atmospherics の上に高度依存の密度勾配と scene-referred (linear 空間) 積分を追加し、WindLight preset 互換を保ったまま空気が体積として感じられるようにします。

> **配信形態**: r14 は **r23 リリースに同梱配信** されます (r14 単独タグは発行しません)。r23 リリースページから本ノートと r14 spec doc にリンクする運用です。

実装・既知 limits・設定の詳細は永続資料 (`docs/ayastorm-r14-volumetric-atmosphere.md`) に常駐させ、本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r14 — Volumetric atmosphere (空気の体積感)

### r14 の柱: 空気が「平らな色つき」ではなく「媒質」として感じられる

r13 までの SL atmospherics は Beer-Lambert の減衰と in-scatter (haze_glow) を既に持っていましたが、物理的に欠けていた要素が 2 つあり、空が「平らなグラデーション」にしか見えない状態でした:

- **高度依存の密度勾配が無い**: 地表と上空で同じ density のため、曇天の「地表は霞んで上空は抜ける」物理感が出ない
- **scene-referred 積分が無い**: `additive` を sRGB で合成して後で linear 化する経路、HDR scene buffer 上で物理整合性が低い

r14 はこの 2 点を **既存パイプラインを書き換えずに追加** します:

- 新しい `calcAtmosphericVars` 分岐で高度の exponential 密度プロファイル (preset `max_y * 0.1` から導出する `scale_height` で制御)
- `additive` と `blue_horizon` の合成を `atmosphericsFuncs.glsl` / `skyV.glsl` 内で linear 空間に移し、depth-aware な空気合成が HDR scene buffer 上で物理的に正しく着地する
- WindLight preset の値は入力として再解釈するのみで、preset 互換は維持

実装は **analytic で軽量** (raymarch 無し)。重い volumetric 処理 (godrays / cloud volume / aerial perspective) は r15〜r18 に分割します。

詳細 → spec `docs/ayastorm-r14-volumetric-atmosphere.md`

### Master switch (本章共通スイッチを r14 で導入)

r14 で **章全体の master switch** (r14〜r20 共通) を導入します:

| Key | Default | 役割 |
|---|---|---|
| `AYAVisualRealismEnabled` | `1` (ON) | 視覚的リアリティ章全体の master switch。`0` で atmospherics / 空合成 / (後段) godrays / aerial perspective / cloud volume / translucency / avatar SSS が r14 以前の挙動に戻る |

> **機能別の debug cvar は意図的に提供しません。** 多数の tuning キーより 1 つの妥当なデフォルト (memory `feedback_prefer_defaults_over_config.md`)。

### 既知のトレードオフ

- **Linear 空間合成で midtone がやや平坦化** — 全体として「曇りっぽさ」が増します。地平線 (朝・夕 / haze の質感) は物理整合性で改善、ここはトレードオフ。preset 値で push back する余地は残し、章の後段 (r15+) で dispersion / godrays / aerial perspective によりリッチネスを取り戻します。
- **太陽 disc 保護のための分割 (P2.a refined)**: shader 内 split で `haze_horizon` (太陽方向の glow) は旧 sRGB のままに残し、linear 化された haze peak が sun disc を白飛びさせるのを回避。`blue_horizon` (全方向の青) のみ linear に移行。
- **Preetham 太陽方向経路長 (P2.b) と Rayleigh/Mie 波長分離 (P2.c) は deferred** — 早期実験で sun disc 劣化が出たため drop、後段の章リリースで「sun disc 保護」を制約として両立する形で再挑戦予定。

### 実装概要

- `atmosphericsFuncs.glsl` — master switch 下で高度密度プロファイル + linear `additive` 合成
- `skyV.glsl` — master switch 下で linear `blue_horizon` 合成 (vertex shader には `srgbF.glsl` が attach されないため `aya_srgb_to_linear` / `aya_linear_to_srgb` をインライン定義)
- `LLSettingsVOSky::applyToShader` — `aya_visual_realism_enabled` の uniform plumbing (template = `classic_mode`)
- `LLShaderMgr` enum + `mReservedUniforms` を拡張
- `settings.xml` — `AYAVisualRealismEnabled` Boolean default 1

### ドキュメント

- r14 spec / パイプライン根拠 / 既知トレードオフ / リスク登録: `docs/ayastorm-r14-volumetric-atmosphere.md`
- P0 atmospheric pipeline 調査: `docs/archive/r14/volumetric_atmosphere_survey.md`
- 視覚的リアリティ章ロードマップ (r14〜r20 全体): `docs/ayastorm-visual-realism-roadmap.md`
- 廃案となった r14 sun-dazzle 案 (pivot 経緯): `docs/ayastorm-r14-sun-dazzle.md`
