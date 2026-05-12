# AYAstorm r16: aerial perspective (距離による色変化)

**作成日**: 2026-05-12
**対象**: AYAstorm `feature/aya-r16-aerial-perspective-spec-draft` (P0 完了 / P1 着手前)
**位置づけ**: 視覚的リアリティ章 (`docs/ayastorm-visual-realism-roadmap.md`) §4 A 軸の第 3 弾、r14 (volumetric atmosphere) + r15 (godrays) の上に積む

> **本書の役割**: r16 個別の **計画スナップショット**。P0 完了に合わせて P1 触るファイル / 注入箇所を確定情報で更新済。実装着手後に commit hash / 実測値を埋める。
> 章全体の位置づけは `docs/ayastorm-visual-realism-roadmap.md`、r14 spec は `docs/ayastorm-r14-volumetric-atmosphere.md`、r15 spec は `docs/ayastorm-r15-godrays.md`、P0 Round 1 結果は `doc/r16/aerial_perspective_survey.md`。

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
- C++ 側 plumbing: **不要** (既存 `aya_visual_realism_enabled` uniform を再利用、新規 uniform 追加なし、P0 確定)
- 3 OS (Linux / macOS / Windows) ビルド + 体感確認

### 波長依存散乱の導出 (P0 確定 2 案、P1 default は A 案)
- **A 案 (P1 default)**: shader 内で既存の `blue_density` / `haze_density` から波長依存 RGB 比率 (1.0/2.33/5.71 等) で派生。新規 uniform 不要、preset 互換完全維持、ただし「物理的にどこまで Rayleigh / どこまで Mie か」は経験式
- **B 案 (拡張、P1 体感不足時のみ検討)**: `llinventory/llsettingssky.cpp` の EEP preset から **生の rayleigh / mie density 値** を引き出して新規 uniform で shader に流す。物理的により正確、ただし plumbing 増 + `feedback_prefer_defaults_over_config.md` (個別 cvar 量産しない) との折り合いを検討
- A 案の体感が十分なら B 案はやらない (`feedback_release_with_user_feedback.md` 流儀)

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

### P0: 実装箇所調査 + spec 確定 — **完了** (Survey Round 1: `doc/r16/aerial_perspective_survey.md`)

調査ターゲット (P0 完了条件) と結果:

1. **r14 P2.b/c の deferred 経緯の正確な復元** — `skyV.glsl:128-130` (P2.c コメント) / `skyV.glsl:138-141` (P2.b コメント) で deferred 位置を実コード上で同定。震源は **`sunlight *= exp(-light_atten * off_axis)` ライン** で sky dome 上の sun disc に直接効くパス。scene 側 (`atmosphericsFuncs.glsl`) に同じ式を入れても sun disc には影響しない構造を確認 (Survey §2.1)
2. **scene aerial perspective と sky dome の経路完全分離の確認** — `atmosphericsFuncs.glsl::calcAtmosphericVars*` と `skyV.glsl::main()` は **共有関数なし / 共有ヘルパーなし / preset uniform のみ共通**。値は共通でも計算式は独立、構造的に分離保証 (Survey §2.2)
3. **Rayleigh / Mie 分離の注入箇所** — `atmosphericsFuncs.glsl` 内 **2 箇所** に確定: L66-68 `light_atten = (blue_density + vec3(haze_density * 0.25)) * (density_multiplier * max_y);` と L72/L98 `combined_haze = max(blue_density + vec3(haze_density), vec3(1e-6));` → `combined_haze = exp(-combined_haze * density_dist * distance_multiplier);`。optional で L77 `above_horizon_factor = 1.0 / max(1e-6, lightnorm.y)` の Preetham 物理化 (Survey §2.3)
4. **Distance Multiplier の現用法** — スカラー float uniform、Beer-Lambert 指数で乗ずる係数として既に流入。**新規 uniform 追加不要**、既存 `distance_multiplier` の物理係数再解釈で足りる (Survey §2.4)
5. **既存 SL aerial perspective infrastructure の grep** — shader 側に物理化既存実装は **存在しない**。preset 側 (`llinventory/llsettingssky.cpp`) に EEP の Rayleigh / Mie density 概念は存在するが shader uniform としては `BlueDensity` / `HazeDensity` に集約済 (Survey §2.5)
6. **3 OS 共通性** — `.metal` / `#ifdef GL_ES` / `HAS_METAL` すべて 0 ヒット。r14 / r15 と同じく GLSL のみで 3 OS 通せる (Survey §2.6)

成果物: `doc/r16/aerial_perspective_survey.md` (Round 1)。

### P1: 実装 — **着手前**

P0 で確定した経路に **Rayleigh/Mie 分離** + **太陽方向経路長の物理化 (optional)** + **Distance Multiplier 物理係数化** を実装。

**触るファイル (P0 確定)**:
- `indra/newview/app_settings/shaders/class1/windlight/atmosphericsFuncs.glsl` — **内側書き換えのみ**。`calcAtmosphericVars*` 内に `if (aya_visual_realism_enabled > 0) { 新経路 } else { 旧経路 }` 分岐を追加、新経路で Rayleigh/Mie 分離 + (optional) Preetham off-axis + Distance Multiplier 物理係数化
- **C++ 改修なし** — `llsettingsvo.cpp` / `settings.xml` / `llshadermgr.{h,cpp}` は **不要** (既存 `aya_visual_realism_enabled` uniform / 既存 preset uniform で完結)

**触る関数 (P0 確定、2 箇所 + optional 1 箇所)**:
- `calcAtmosphericVars` (atmospherics 中央計算): 注入箇所 1 (light_atten L66-68) + 注入箇所 2 (combined_haze L72/L98) + optional (above_horizon_factor L77)
- `calcAtmosphericVarsLinear` (wrapper): 直接の式変更なし、`calcAtmosphericVars` の更新を継承

**波長依存散乱の派生 (P0 確定、A 案 default)**:
- A 案: `blue_density` / `haze_density` から RGB 波長依存比率で shader 内派生 (新規 uniform 不要)
- B 案 (体感不足時のみ): EEP preset の生 rayleigh / mie 値を新規 uniform で流す

**含めない (P0 で構造的に保証)**:
- `skyV.glsl` の改修 (sun disc 保護方針、r17 で再評価)
- C++ 側 plumbing 追加 (既存 uniform 流用)

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
| R1 | r14 P2.b/c で観測した sun disc 消失副作用が scene 側書き換えでも誘発される (skyV.glsl と calcAtmosphericVars に共有 hidden path がある) | **解消**: P0 Survey §2.1 / §2.2 で経路完全分離 (共有関数なし、共有ヘルパーなし、preset uniform のみ共通) を構造的に確認、scene 側書き換えは sky dome の sun disc に影響しない |
| R2 | Rayleigh/Mie 分離で遠景が異常に青く / 異常にコントラスト高く見え、preset の絵作りが壊れる | 残: P1 で `aya_visual_realism_enabled` の OFF/ON 比較を細かく取る、値域 (波長比 1.0/2.33/5.71 は SL preset の Blue Density スケールでは過剰な可能性) を P1 で調整 |
| R3 | Distance Multiplier の物理係数再解釈で preset の見え方が劇的に変化 | 残: P1 で旧 lerp 係数 → 物理係数の **変換マッピング** を preset 値域で線形補正、極端値域での発散を回避 |
| R4 | scene aerial perspective だけ書き換えると sky dome (P2.a refined) との境界 (地平線付近) で不連続な色境界が出る | 残: P1 で地平線付近 (sample 高度が大気上限に近い領域) の blend を確認、必要なら horizon-near の transition を入れる |
| R5 | FPS 大幅低下 | **緩和**: P0 で「追加 pass なし、`calcAtmosphericVars*` の内側書き換えのみ」が確定、analytic 実装で完結 |
| R6 | 3 OS でビルドが通らない (macOS Metal 等) | **解消**: P0 Survey §2.6 で `.metal` / `#ifdef GL_ES` / `HAS_METAL` すべて 0 ヒット、GLSL のみで通せる |
| R7 | master switch off 経路で見え方が完全に r15 まで戻らない | **緩和**: P0 で `if (aya_visual_realism_enabled > 0)` 分岐を `calcAtmosphericVars*` 入口で取る経路が確定、off 経路は r14 P2.a refined の式そのまま |
| R8 | r14 で deferred した P2.b (Preetham off-axis) を scene 側に入れたとき、scene の中の太陽近傍方向で別の白飛びが起きる | **構造的緩和**: P0 で「skyV.glsl の `sunlight` パスが震源、scene 側 atmosphericsFuncs は独立」が判明。scene 側で sun-near の挙動を P1 で確認するのは継続、ただし sky dome の sun disc 自体は構造的に保護される |

---

## 7. 更新履歴

- 2026-05-12 (初版): r14 (volumetric atmosphere) を A 軸第 1 弾、r15 (godrays) を第 2 弾として実装完了 (Linux PASS) させた流れの第 3 弾として起票。r14 P2.b (Preetham 太陽方向経路長物理化) / P2.c (Rayleigh/Mie 波長依存分離) を **sun disc 保護と両立する形で復活** させる枠を r16 で確定。skyV.glsl は本リリースで一切触らない方針、改修対象は scene 側 (`atmosphericsFuncs.glsl::calcAtmosphericVars*`) のみ。r15 公開は r16+ まで pending の AYA 方針を §4 P3 に反映 (r17 か A 軸完走と一括公開想定)
- 2026-05-12 (P0 完了): Survey Round 1 (`doc/r16/aerial_perspective_survey.md`) で 6 項目すべてクリア。震源は skyV.glsl `sunlight *= exp(-light_atten * off_axis)` ラインで scene 側 atmosphericsFuncs とは構造的に独立を確認、Rayleigh/Mie 注入箇所を `atmosphericsFuncs.glsl` 内 2 箇所 (light_atten / combined_haze) + optional 1 箇所 (above_horizon_factor) に確定。**C++ 改修不要** (既存 `aya_visual_realism_enabled` + 既存 preset uniform で完結)、波長依存散乱の派生は A 案 (shader 内派生) default で P1 着手、B 案 (EEP preset 引出し) は体感不足時のみ。リスク R1 / R6 を解消、R5 / R7 / R8 を構造的に緩和、R2 / R3 / R4 は P1 体感調整に残置
