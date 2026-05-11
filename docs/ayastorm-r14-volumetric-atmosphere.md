# AYAstorm r14: volumetric atmosphere (空気の体積感)

**作成日**: 2026-05-12
**対象**: AYAstorm `feature/aya-r14-volumetric-atmosphere` (予定 / 着手前)
**位置づけ**: 視覚的リアリティ章 (`docs/ayastorm-visual-realism-roadmap.md`) §4 A 軸の初弾、§2 設計制約に従う

> **本書の役割**: r14 個別の **計画スナップショット**。実装着手後に commit hash / 実測値を埋める。
> 章全体の位置づけは `docs/ayastorm-visual-realism-roadmap.md`、P0 調査は `doc/r14/volumetric_atmosphere_survey.md`。

---

## 1. ゴール

P0 (`doc/r14/volumetric_atmosphere_survey.md` Round 1) の調査で判明したとおり、SL は **既に Beer-Lambert ベース** で atmospherics を組んでいる (`atmosphericsFuncs.glsl:84` `combined_haze = exp(-combined_haze * density_dist * distance_multiplier)`)。in-scatter (haze_glow) も既に実装済み。

ただし「空気が体積として見えない」原因として、次の **3 点が欠落** している:

1. **altitude density (高度依存の密度勾配)** が無い — `max_y` で clamp する以外、地表と上空で同じ密度。曇りの日に地上で霞んで上空が抜ける、といった物理感が出ない
2. **scene-referred 積分が無い** — `additive` を sRGB 空間で組んで後で `srgb_to_linear`、物理整合性が低く HDR scene buffer 上で「リアル」にならない
3. **反対方向の dispersion 区別が無い** — haze_glow は太陽方向だけ、反対側の Rayleigh/Mie 区別なし (これは r15-r16 の話、r14 ではタッチしない)

r14 では **既存の Beer-Lambert + in-scatter 基盤を残したまま、上記 1 と 2 を追加実装** する。preset の絵作り意図を保ちながら空気の体積感を物理的に正しい形で出す。heavy raymarch には踏み込まない (r15 godrays、r17 雲の体積化で取り組む)。

---

## 2. 設計制約

`docs/ayastorm-visual-realism-roadmap.md` §2 の境界条件をそのまま継承:

- **保つ**: WindLight preset 互換、HDR scene buffer 骨格、PBR shader interface
- **書き換える**: atmospheric scattering の計算式、fog 適用方法
- **言い換え**: preset 値を input、scene-referred 物理で再解釈、出力契約 (HDR scene buffer の信号特性) は保つ

### Master switch
- `AYAVisualRealismEnabled` (default TRUE) を r14 で導入
- `FALSE` で r13 までの見え方に戻る (atmospherics の旧計算経路を選択)
- r15-r17 でも同じスイッチで一括 ON/OFF できる設計、軸 / リリース個別 switch は作らない (`feedback_prefer_defaults_over_config.md`)

---

## 3. スコープ

### 含む
- 既存 `calcAtmosphericVars` / `calcAtmosphericVarsLinear` (atmosphericsFuncs.glsl) に **altitude density 経路を追加** — 視線サンプル点の高度に応じて density を勾配化、地表近くは濃く上空は薄く
- 既存の `additive` 合成を **scene-referred linear で積分** する経路を追加 — sRGB 空間での合成を物理的に正しい linear 空間に移す
- master switch `aya_visual_realism_enabled` で新経路 / 旧経路 (現状の式) を切り替える分岐を追加 (新経路 default ON)
- C++ 側 plumbing: `LLShaderMgr` enum + `mReservedUniforms` + `LLSettingsVOSky::applyToShader` (template = `classic_mode`)
- `settings.xml` に `AYAVisualRealismEnabled` Boolean default 1 を追加
- 3 OS (Linux / macOS / Windows) ビルド + 体感確認

### 含まない (→ r15+)
- 既存 Beer-Lambert / in-scatter の式そのものの書き換え (既存基盤を活かす、書き換えはしない)
- 反対方向の Rayleigh/Mie 分離 (r15 godrays / r16 aerial perspective)
- godrays / shaft of light (r15)
- aerial perspective の精緻化 (色相変化、r16)
- 時間帯色温度の物理化 + 波長依存散乱の物理分離 (r17)
- 雲の体積化 (r17)
- heavy raymarch volumetric

### 永久 drop
- 軸 / 機能ごとの個別 debug settings (master switch 1 本のみ、altitude density 強度を tuning する追加 cvar は入れない方向で P1 着手、必要なら P1 で議論)
- preset を破壊する後方非互換変更
- LUT / color grade による誤魔化し

---

## 4. フェーズ分解

viewer-only の改修。配信側 / SIM 側変更なし。

### P0: 実装箇所調査 + spec 確定 [完了 2026-05-12]

詳細は `doc/r14/volumetric_atmosphere_survey.md` Round 1。要点:

- atmospherics 中央関数: `atmosphericsFuncs.glsl:calcAtmosphericVars` (Beer-Lambert + haze_glow in-scatter は実装済み)
- preset plumbing: `LLSettingsVOSky::applyToShader` (llsettingsvo.cpp ~L780)
- master switch template: `classic_mode` (LLShaderMgr enum + mReservedUniforms + applyToShader 内 `uniform1i`)
- 3 OS: GLSL 共通、`.metal` なし、`#ifdef` なし

### P1: 実装

P0 で確定した経路に **altitude density 追加** + **scene-referred 積分追加** + **master switch plumbing** を実装。

**触るファイル**:
- `indra/llrender/llshadermgr.h` — `AYA_VISUAL_REALISM_ENABLED` enum 追加
- `indra/llrender/llshadermgr.cpp` — `mReservedUniforms` に `"aya_visual_realism_enabled"` push
- `indra/newview/llsettingsvo.cpp` — `applyToShader` で `LLCachedControl<bool>` + `shader->uniform1i`
- `indra/newview/app_settings/settings.xml` — `AYAVisualRealismEnabled` Boolean default 1
- `indra/newview/app_settings/shaders/class1/windlight/atmosphericsFuncs.glsl` — `uniform int aya_visual_realism_enabled;` 受信、`calcAtmosphericVars` 内で `if (aya_visual_realism_enabled > 0) { 新経路 } else { 旧経路 }` 分岐、新経路で altitude density + scene-referred 積分

**触る関数**:
- `LLSettingsVOSky::applyToShader` (uniform 追加)
- `calcAtmosphericVars` / `calcAtmosphericVarsLinear` (新経路追加)

**新規 uniform**:
- `aya_visual_realism_enabled` (int)

### P2: 3 OS ビルド + 体感確認

AYA が Linux ビルド + 体感確認。問題なければ macOS / Windows ビルドへ。

### P3: tag / release

`feedback_release_with_user_feedback.md` に従い、完璧を目指さず tag / release してフィードバック収集。

---

## 5. 受け入れ条件

- [ ] 既存 WindLight preset (朝・昼・夕・夜) が **読み込めて、preset 切替が機能する** (preset 互換破壊なし)
- [ ] `AYAVisualRealismEnabled = TRUE` で:
  - 遠景が距離と共に減衰して見える (距離感が出る)
  - 地表近くと上空で空気の密度差が体感できる
  - 太陽方向と反対方向で空気色が物理的に異なる (in-scatter の効果)
- [ ] `AYAVisualRealismEnabled = FALSE` で r13 までと同じ見え方に戻る
- [ ] 3 OS でビルド + 起動 + 表現確認
- [ ] FPS 影響が ±10% 以内 (P0 で軽量実装の見込みを立てる)

---

## 6. リスク

| ID | リスク | P0 後ステータス |
|---|---|---|
| R1 | preset の Haze 値域が altitude density で発散 (上空で 0 / 地表で過大) | 残: P1 で clamp / normalize、`max_y` を上限に密度勾配を formal に設計 |
| R2 | 既存 atmospherics shader の書き換えコスト爆発 | **緩和**: P0 で「全置換ではなく追加実装」と確定、書き換え範囲は `calcAtmosphericVars` 内の追加分岐のみ |
| R3 | master switch off 経路で見え方が完全に r13 に戻らない | **緩和**: P0 で `if (aya_visual_realism_enabled > 0)` の分岐点を `calcAtmosphericVars` 入口で取ることが確定、off 経路は既存式そのまま |
| R4 | FPS 大幅低下 | 残: analytic 実装に徹する (P1 着手後計測)、raymarch は r14 では入れない |
| R5 | 3 OS でビルドが通らない (macOS Metal 等) | **解消**: P0 で `.metal` 無し / `#ifdef` 無しを確認、GLSL 共通で問題なし |
| R6 | scene-referred 積分への移行で既存 preset の見え方が大きく変わる (= 章 thesis 上は OK だが master off 経路で旧見え方が必要) | 残: master switch off 時は完全旧経路、on 時は新経路と明示的に分岐、preset 値は両経路で読み込み可能を維持 |

---

## 7. 更新履歴

- 2026-05-12 (初版): 旧 `docs/ayastorm-r14-sun-dazzle.md` (sun disc overbright) を unground した経緯を経て、r14 の本命を volumetric atmosphere に pivot。章全体の方向転換は `docs/ayastorm-visual-realism-roadmap.md` §1 thesis、memory `project_ayastorm_visual_realism_chapter.md` 参照
- 2026-05-12 (P0 完了に伴う改訂): P0 (`doc/r14/volumetric_atmosphere_survey.md` Round 1) で「SL は既に Beer-Lambert + in-scatter を実装済み」「`classic_mode` が master switch の完全 template」「GLSL は 3 OS 共通」が判明。前提が「全置換」から「追加実装 (altitude density + scene-referred 積分 + master switch)」に変わったため、§1 ゴール / §3 スコープ / §4 P1 / §6 リスクを実態に合わせて更新。受け入れ条件 (§5) は維持
