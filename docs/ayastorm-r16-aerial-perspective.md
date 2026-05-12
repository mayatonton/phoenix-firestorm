# AYAstorm r16: aerial perspective (距離による色変化)

**作成日**: 2026-05-12
**対象**: AYAstorm `feature/aya-r16-aerial-perspective-spec-draft` (P0 着手前)
**位置づけ**: 視覚的リアリティ章 (`docs/ayastorm-visual-realism-roadmap.md`) §4 A 軸の第 3 弾、r14 (volumetric atmosphere) + r15 (godrays) の上に積む

> **本書の役割**: r16 個別の **計画スナップショット**。実装着手後に commit hash / 実測値を埋める。
> 章全体の位置づけは `docs/ayastorm-visual-realism-roadmap.md`、r14 spec は `docs/ayastorm-r14-volumetric-atmosphere.md`、r15 spec は `docs/ayastorm-r15-godrays.md`。

---

## 1. ゴール

r14 で「**空気が体積として見える**」(altitude density + scene-referred 積分)、r15 で「**光線が空間を貫く**」(shadow-driven godrays) を積んだ。r16 では **遠景が空気の中に物理的に座る** ところを取りに行く。具体的に出したい体感:

- **遠くの山が青くかすむ** (Rayleigh 散乱 — 短波長が散乱で減衰、長距離経路で青味が強調)
- **太陽方向の遠景はやや白っぽい** (Mie 散乱 — 大粒子の前方ピーク)
- **遠景の距離感が物理的に積み上がる** (depth-based scattering integration、Distance Multiplier を「大気の scale」として再解釈)

技術的には、r14 P2.b (Preetham 1999 sec(θ) → 球面近似で太陽方向経路長 finite 化) と P2.c (波長依存散乱比 RGB ≈ (1.0, 2.33, 5.71) で Rayleigh weight を blue_density に重畳) を **sun disc 保護と両立する形で復活** させる。r14 ではこれらを sky shader (skyV.glsl) に適用したため sun disc が白飛びで覆われる副作用が出て deferred。r16 では **scene 側 (atmosphericsFuncs.glsl の `calcAtmosphericVars*`、= 不透明オブジェクト・地表・水面の aerial perspective)** のみに限定し、sky dome は r14 P2.a refined のまま温存する。

> SL における aerial perspective の位置づけ: WindLight preset の `Distance Multiplier` / `Haze Density` / `Blue Density` / `Blue Horizon` がこの色相変化を担う変数群。現状は経験式 (Beer-Lambert + 単純 lerp) で、波長依存散乱の分離も太陽方向経路長の物理化も無い。

---

## 2. 設計制約

`docs/ayastorm-visual-realism-roadmap.md` §2 の境界条件をそのまま継承:

- **保つ**: WindLight preset 互換、HDR scene buffer 骨格、PBR shader interface、sky dome (skyV.glsl) の見え方
- **書き換える**: `atmosphericsFuncs.glsl` の scene aerial perspective 計算
- **言い換え**: preset (`Distance Multiplier` / `Haze Density` / `Blue Density` / `Blue Horizon`) を input、scene-referred 物理で再解釈、出力契約 (HDR scene buffer の信号特性) は保つ

### Master switch
- r14 で導入済の `AYAVisualRealismEnabled` (default TRUE) を流用
- `FALSE` で r15 までの見え方 (= r14 atmospherics + r15 godrays の挙動) に戻る
- 軸 / リリース個別 switch は作らない (`feedback_prefer_defaults_over_config.md`)

### sun disc 保護 (r14 P2.b/c の副作用を踏まない)
- **sky shader (`skyV.glsl`) は本リリースで一切触らない**
- 改修対象は scene 側 (`atmosphericsFuncs.glsl::calcAtmosphericVars*`) のみ
- sky dome の haze_glow / sun halo / atmospheric in-scatter は r14 P2.a refined の挙動を維持

---

## 3. スコープ

### 含む
- `atmosphericsFuncs.glsl::calcAtmosphericVars` / `calcAtmosphericVarsLinear` の **scene aerial perspective 経路に Rayleigh / Mie 分離を導入** — 視線方向と sample 高度に応じて短波長 (Rayleigh) と長波長中性 (Mie) の散乱重みを分ける
- **太陽方向経路長の物理化** (Preetham 1999 球面近似 `cos_zenith + 0.15 * pow(max(93.885 - theta_deg, 1.0), -1.253)`) を scene の in-scatter にのみ適用 — 太陽方向 / 反対方向の色相差を物理的に出す
- **Distance Multiplier の物理係数化** — preset 値を「大気の scale (scale height / total optical depth 換算)」として再解釈、現状の生 lerp 係数から物理 unit に置き換え
- master switch `aya_visual_realism_enabled` で新経路 / 旧経路 (= r14 P2.a refined までの式) を切り替える分岐を追加 (新経路 default ON)
- C++ 側 plumbing: 既存 `aya_visual_realism_enabled` uniform を再利用 (新規 uniform 不要見込み、P0 で確定)
- 3 OS (Linux / macOS / Windows) ビルド + 体感確認

### 含まない (→ r17+)
- **sky dome (skyV.glsl) の物理化** — r14 P2.b/c で sun disc 消失を起こした領域、r16 では一切触らない
- 時間帯色温度の物理化 (Sun/Ambient color の色温度解釈、r17)
- 雲の体積化 (r17)
- 太陽 disc 自身の HDR boost (r17、r14 P1 unground 分の再着手枠)
- 物質側 subsurface scattering (B 軸、r18+)
- カメラ表現 (DoF / auto-exposure / scene-referred 露出階調、C 軸 r20+)

### 永久 drop
- 軸 / 機能ごとの個別 debug settings (master switch 1 本のみ。aerial perspective 強度を tuning する cvar は入れない方向で P1 着手、必要なら P1 で議論)
- sky dome の disc 周辺を物理的に書き換える試み (r14 P2.b/c の教訓、r16 ではやらない)
- preset を破壊する後方非互換変更
- LUT / color grade による「遠景色相」誤魔化し
- 重い per-frame full-screen volumetric ray-march

---

## 4. フェーズ分解

viewer-only の改修。配信側 / SIM 側変更なし。

### P0: 実装箇所調査 + spec 確定 — **着手前**

調査ターゲット (P0 完了条件):

1. **r14 P2.b/c の deferred 経緯の正確な復元** — `001ecc5c73` (P2.a 初版) / `28727fa57f` (P2.a refined) 周辺の hunk を回収し、どの式変更が sun disc 消失を引き起こしたかを **scene 側経路と sky 側経路に切り分けて** 同定する。`skyV.glsl` 内で起きた副作用と仮定するが、scene 側 (`atmosphericsFuncs.glsl`) に同じ式を入れた場合の影響は要確認
2. **scene aerial perspective と sky dome の経路完全分離の確認** — `atmosphericsFuncs.glsl::calcAtmosphericVars*` が deferred lighting / PBR object shading から呼ばれる経路と、`skyV.glsl` の vary_HazeColor 経路が独立か (共有関数 / 共有 uniform / 共有マクロが無いか) を grep + コード追跡で確認
3. **Rayleigh / Mie 分離の注入箇所** — `calcAtmosphericVars*` のどの行で波長別 weight を分けるか、現状 `combined_haze = exp(-combined_haze * density_dist * distance_multiplier)` のスカラー積算に対して RGB 別係数を入れる箇所を同定
4. **Distance Multiplier の現用法** — preset → `LLSettingsVOSky::applyToShader` → shader uniform の流入経路、現状の単位 (経験的 lerp 係数 / 物理単位なし)、物理係数に移行する場合の preset 値域マッピング
5. **既存 SL aerial perspective infrastructure の grep** — `aerial.?perspective` / `rayleigh` / `mie` / `wavelength` / `scatter.*coefficient` のヒット有無、SL 本家に類似実装の痕跡があれば参考にする
6. **3 OS 共通性** — r14 と同じく GLSL 共通 / `.metal` 無し / `#ifdef` 無しで通せるか

成果物: `doc/r16/aerial_perspective_survey.md` (Round 1)。

### P1: 実装

P0 で確定した経路に **Rayleigh/Mie 分離** + **太陽方向経路長の物理化** + **Distance Multiplier 物理係数化** を実装。

**触る (見込み) ファイル**:
- `indra/newview/app_settings/shaders/class1/windlight/atmosphericsFuncs.glsl` — `calcAtmosphericVars*` 内に `if (aya_visual_realism_enabled > 0) { 新経路 } else { 旧経路 }` 分岐を追加、新経路で Rayleigh/Mie 分離 + Preetham off-axis + Distance Multiplier 物理係数化
- `indra/newview/llsettingsvo.cpp` (要見込み確定) — Distance Multiplier の uniform 流入経路に物理係数変換を入れるか、shader 側で変換するかを P0 後に決める
- 新規 uniform: 原則として追加しない (既存 `aya_visual_realism_enabled` + 既存 preset uniform で完結を目指す)

**触る関数**:
- `calcAtmosphericVars` / `calcAtmosphericVarsLinear` (新経路追加)
- (必要なら) `LLSettingsVOSky::applyToShader` (uniform 変換)

**含めない**:
- `skyV.glsl` の改修 (sun disc 保護方針、r17 で再評価)

### P2: 3 OS ビルド + 体感確認

AYA が Linux フルビルド + 体感確認。問題なければ macOS / Windows ビルドへ。`feedback_release_with_user_feedback.md` の流儀で「完璧を目指さず実機で確認」。

### P3: tag / release

r15 と同じく、**公開は r16 単独でせず後続リリースまで pending** の運用 (AYA 方針)。r17 か A 軸完走 (r17 までで A 軸完結) のいずれかとまとめて公開する想定。

---

## 5. 受け入れ条件

- [ ] 既存 WindLight preset (朝・昼・夕・夜) が **読み込めて、preset 切替が機能する** (preset 互換破壊なし)
- [ ] `AYAVisualRealismEnabled = TRUE` で:
  - **遠景の山 / 地形が距離と共に青味方向にシフト** する (Rayleigh)
  - **太陽方向の遠景はやや白っぽい / 反対方向は青味が強い** (Mie 前方ピーク + Rayleigh 比率)
  - r14 で出した「空気の体積感」が遠景でも壊れない
- [ ] `AYAVisualRealismEnabled = FALSE` で r15 までと同じ見え方に戻る (新経路 skip)
- [ ] **sky dome の見え方が r14 P2.a refined のまま** (sun disc 健在、朝・夕の地平線・青空質感は劣化なし) — sky shader を触らない方針の遵守確認
- [ ] 3 OS でビルド + 起動 + 表現確認
- [ ] FPS 影響が ±10% 以内 (scene aerial perspective は既存 atmospherics の内側書き換えで、追加 pass なしの見込み)

---

## 6. リスク

| ID | リスク | P0 後ステータス |
|---|---|---|
| R1 | r14 P2.b/c で観測した sun disc 消失副作用が scene 側書き換えでも誘発される (skyV.glsl と calcAtmosphericVars に共有 hidden path がある) | P0 §4.2 で経路完全分離を確認 |
| R2 | Rayleigh/Mie 分離で遠景が異常に青く / 異常にコントラスト高く見え、preset の絵作りが壊れる | P1 で `aya_visual_realism_enabled` の OFF/ON 比較を細かく取る、値域 (波長比 1.0/2.33/5.71 は SL preset の Blue Density スケールでは過剰な可能性) を P1 で調整 |
| R3 | Distance Multiplier の物理係数再解釈で preset の見え方が劇的に変化 | P1 で旧 lerp 係数 → 物理係数の **変換マッピング** を preset 値域で線形補正、極端値域での発散を回避 |
| R4 | scene aerial perspective だけ書き換えると sky dome (P2.a refined) との境界 (地平線付近) で不連続な色境界が出る | P1 で地平線付近 (sample 高度が大気上限に近い領域) の blend を確認、必要なら horizon-near の transition を入れる |
| R5 | FPS 大幅低下 | analytic 実装に徹する、追加 pass なし、`calcAtmosphericVars*` の内側書き換えのみ |
| R6 | 3 OS でビルドが通らない (macOS Metal 等) | r14 / r15 と同じく GLSL 共通 / `.metal` 無し / `#ifdef` 無しで通す、P0 で確認 |
| R7 | master switch off 経路で見え方が完全に r15 まで戻らない | r14 / r15 と同じ流儀: `if (aya_visual_realism_enabled > 0)` を `calcAtmosphericVars*` 入口で取り、off 経路は r14 P2.a refined の式そのまま |
| R8 | r14 で deferred した P2.b (Preetham off-axis) を scene 側に入れたとき、scene の中の太陽近傍方向で別の白飛びが起きる | P1 で scene の太陽近傍方向 (view·sun ≈ 1 の領域) の挙動を確認、必要なら scene 側も sun-near guard を入れる |

---

## 7. 更新履歴

- 2026-05-12 (初版): r14 (volumetric atmosphere) を A 軸第 1 弾、r15 (godrays) を第 2 弾として実装完了 (Linux PASS) させた流れの第 3 弾として起票。r14 P2.b (Preetham 太陽方向経路長物理化) / P2.c (Rayleigh/Mie 波長依存分離) を **sun disc 保護と両立する形で復活** させる枠を r16 で確定。skyV.glsl は本リリースで一切触らない方針、改修対象は scene 側 (`atmosphericsFuncs.glsl::calcAtmosphericVars*`) のみ。r15 公開は r16+ まで pending の AYA 方針を §4 P3 に反映 (r17 か A 軸完走と一括公開想定)
