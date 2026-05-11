# AYAstorm r14: volumetric atmosphere (空気の体積感)

**作成日**: 2026-05-12
**対象**: AYAstorm `feature/aya-r14-volumetric-atmosphere` (予定 / 着手前)
**位置づけ**: 視覚的リアリティ章 (`docs/ayastorm-visual-realism-roadmap.md`) §4 A 軸の初弾、§2 設計制約に従う

> **本書の役割**: r14 個別の **計画スナップショット**。実装着手後に commit hash / 実測値を埋める。
> 章全体の位置づけは `docs/ayastorm-visual-realism-roadmap.md`、P0 調査は `doc/r14/volumetric_atmosphere_survey.md`。

---

## 1. ゴール

SL viewer の現状の atmospherics は「色付きフィルター状」で、空気が **体積として見えない**。具体的には:

- 遠景が距離と共に減衰せず、近景と同じ強度でくっきり見える
- 高度差 (地上 vs 上空) で空気の密度が変わって見えない
- 太陽光が空気の中を通って届くという物理感がない (= sun halo は出るが「光が空気を満たしている」体感は出ない)

r14 ではこれを **WindLight preset の Haze / Blue Density 値を物理パラメタとして再解釈** し、**depth-driven Beer-Lambert + altitude density + analytic in-scatter** の組合せで、preset の絵作り意図を保ちながら空気の体積感を出す。

heavy raymarch には踏み込まない (r15 godrays、r17 雲の体積化で取り組む)。

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
- 既存 atmospherics shader (`atmosphericsFuncs.glsl` 等) の内側を物理ベースに書き換え
- depth-driven Beer-Lambert によるフォグ適用 (距離指数減衰)
- altitude density (高度ごとの空気密度勾配)、地表近くは濃く上空は薄く
- analytic in-scatter (太陽方向 dot で light の rebound を近似)
- preset の Haze Horizon / Haze Density / Blue Density / Distance Multiplier 値を上記の物理パラメタにマップ
- master switch (`AYAVisualRealismEnabled`)
- 3 OS (Linux / macOS / Windows) ビルド + 体感確認

### 含まない (→ r15+)
- godrays / shaft of light (r15)
- aerial perspective の精緻化 (色相変化、r16)
- 時間帯色温度の物理化 (r17)
- 雲の体積化 (r17)
- heavy raymarch volumetric

### 永久 drop
- 軸 / 機能ごとの個別 debug settings (master switch 1 本のみ)
- preset を破壊する後方非互換変更
- LUT / color grade による誤魔化し

---

## 4. フェーズ分解

viewer-only の改修。配信側 / SIM 側変更なし。

### P0: 実装箇所調査 + spec 確定

**目的**: SL の atmospherics 経路を P0 round 1 (`doc/r14/sun_rendering_survey.md`、sun/halo) と同等の精度で解明し、書き換え点を確定する。

**進め方**: `doc/r14/volumetric_atmosphere_survey.md` を P0 起点として、以下を順次解明:

- `atmosphericsFuncs.glsl` (および関連 include) の中で fog / haze が適用される正確な式
- WindLight preset 値 (Haze, Blue Density, Distance Multiplier) がどの uniform に乗って shader に届くか
- HDR scene buffer の段階で fog はいつ適用されるか (geometry pass / post / deferred fog pass)
- altitude density / in-scatter の挿入箇所 (既存 shader を書き換えるか、新 pass を足すか)
- master switch の plumbing (LLCachedControl → uniform)

**完了条件**: 上記が survey にまとまり、P1 で触る具体的なファイル / 関数 / uniform が確定する。

### P1: 実装

P0 で確定した経路に Beer-Lambert + altitude density + analytic in-scatter を実装。preset 値からの physical parameter マップを書く。`AYAVisualRealismEnabled` の plumbing も同時に。

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

| ID | リスク | 緩和策 |
|---|---|---|
| R1 | preset の Haze 値域が物理ベースの想定範囲外で破綻 | P0 で preset 値域を計測、physical parameter へのマップで clamp / normalize |
| R2 | 既存 atmospherics shader の include 経路が複雑で書き換えコスト爆発 | P0 で経路を完全に解明、書き換え範囲を最小化、複雑なら r14 スコープを縮小して r14.5 に分割 |
| R3 | master switch off 経路で見え方が完全に r13 に戻らない (新 shader 経路が混入) | P0 で switch の挿入点を決定、最上位 (uber shader 入り口) で分岐 |
| R4 | FPS 大幅低下 | analytic 実装に徹する、raymarch は r14 では入れない、必要なら altitude density を LUT 化 |
| R5 | 3 OS でビルドが通らない (macOS Metal 等) | P0 で Mac/Win 経路の shader 差異も確認 |

---

## 7. 更新履歴

- 2026-05-12: 初版作成。旧 `docs/ayastorm-r14-sun-dazzle.md` (sun disc overbright) を unground した経緯を経て、r14 の本命を volumetric atmosphere に pivot。章全体の方向転換は `docs/ayastorm-visual-realism-roadmap.md` §1 thesis、memory `project_ayastorm_visual_realism_chapter.md` 参照
