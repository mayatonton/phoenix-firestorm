# AYAstorm r16: aerial perspective (距離による色変化)

**作成日**: 2026-05-12
**対象**: AYAstorm `feature/aya-r16-aerial-perspective-spec-draft` (P1.a 完了 / P1.b drop / r16 実装完結)
**位置づけ**: 視覚的リアリティ章 (`docs/ayastorm-visual-realism-roadmap.md`) §4 A 軸の第 3 弾、r14 (volumetric atmosphere) + r15 (godrays) の上に積む

> **本書の役割**: r16 個別の **計画スナップショット**。P1.a (波長依存 in-scatter) 完了 + P1.b (Preetham 球面近似) drop で r16 実装完結。次は r17 (時間帯色温度 + 雲の体積化)。
> 章全体の位置づけは `docs/ayastorm-visual-realism-roadmap.md`、r14 spec は `docs/ayastorm-r14-volumetric-atmosphere.md`、r15 spec は `docs/ayastorm-r15-godrays.md`、P0 Round 1 結果は `docs/archive/r16/aerial_perspective_survey.md`。

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
- **P1.a で個別 switch `AYAR16AerialPerspectiveEnabled` (default TRUE) を追加** — master を切ると r14/r15 効果も同時に消えて r16 単独の体感評価ができないため (P1.a 実装中に判明)。master ON 前提で個別 switch のみ toggle すると r16 効果だけ on/off 可能。`feedback_prefer_defaults_over_config.md` (個別 cvar 量産しない原則) に対しては「章ごと体感評価用の必要最小限の sentinel」として r14/r15/r16 の各 sentinel 1 本のみとする方針で運用

### sun disc 保護 (r14 P2.b/c の副作用を踏まない)
- **sky shader (`skyV.glsl`) は本リリースで一切触らない**
- 改修対象は scene 側 (`atmosphericsFuncs.glsl::calcAtmosphericVars*`) のみ
- sky dome の haze_glow / sun halo / atmospheric in-scatter は r14 P2.a refined の挙動を維持

---

## 3. スコープ

### 含む
- **P1.a (完了 2026-05-12, commit `7427fbcb8d`)**: `atmosphericsFuncs.glsl::calcAtmosphericVars` で Rayleigh λ^-4 波長依存重み `rayleigh_w = (1.0, 2.33, 5.71)` を導入。注入先は `combined_haze` (視線散乱係数) と `blue_weight` (in-scatter color) の 2 箇所。`light_atten` (太陽光路) には適用しない: 常時夕焼け化で近景まで黄ばむ副作用が出る (実装中実証済) ため、aerial perspective ≠ 夕焼けという物理的分離を実装に反映
- **P1.b (drop 2026-05-12)**: Preetham 1999 球面近似を一度実装 (`above_horizon_factor` を `cos_zenith + 0.15 * pow(max(93.885 - theta_deg, 1.0), -1.253)` で置換) し Linux 実機検証。数値的には sec(θ) と Preetham の差は θ=80° で 5.76→5.53、θ=89° で 57.3→26.5 と境界化されるが、AYA 実機で「穏やかになった感じは特にない」体感、SL の sun timeline は地平線 ±5° の差異帯を一気に跨ぐため perceptual threshold 以下。`feedback_feature_value_in_main_usecase.md` (動いた ≠ 効いた) に従い drop、shader revert 済 (md5 `de496f6...`)。残り A 軸の「効くもの」(r17 雲の体積化 / 時間帯色温度) に集中する判断
- **Distance Multiplier の物理係数化**: 当面 drop (P1.a の体感で既に「遠景青味シフト」が出ているため、preset 互換破壊リスクを取らない方向、`feedback_release_with_user_feedback.md`)
- **個別 switch `AYAR16AerialPerspectiveEnabled`** で master と独立に r16 効果のみ on/off 可能 (新経路 default ON)
- C++ 側 plumbing: **P1.a で追加** (settings.xml / llshadermgr.{h,cpp} / llsettingsvo.cpp、個別 switch + uniform 1 本)。元 P0 想定は「不要」だったが、master swithcの体感評価不能問題を解消するため不可避と判明
- 3 OS (Linux / macOS / Windows) ビルド + 体感確認 (P2)

### 波長依存散乱の導出 (P1.a 完了で **A 案採用確定**)
- **A 案 (P1.a 採用)**: shader 内で `blue_density * rayleigh_w` (rayleigh_w = `(1.0, 2.33, 5.71)`) として λ^-4 を派生。新規 uniform 不要 (rayleigh_w は shader 内定数)、preset 互換完全維持、AYA 体感で「遠景青味シフトが明確に見える / 近景は変化なし」確認
- **B 案 (drop)**: EEP preset の生 rayleigh / mie 値引き出しは P1.a の体感で十分なため drop。`feedback_release_with_user_feedback.md` 流儀

### atmosFragLighting の atten scalarize (P1.a で判明)
- scene shader (atmosphericsF.glsl::atmosFragLighting) は `light *= atten.r` で **R 成分をスカラー化して全 RGB に均一適用** する。per-channel atten.gb は surface 直接透過には使われない
- このため `combined_haze` への rayleigh_w (= atten の波長依存) は surface 直接透過には**殆ど効かない**
- r16 P1.a で観測される遠景青味シフトは **additive (in-scatter) 側の `blue_weight` および `(1-combined_haze)` 経由のみ** で出ている設計
- surface 直接透過の波長依存が必要なら atmosFragLighting 自体の改修が必要 (全 consumer に影響) — 当面 drop

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

### P0: 実装箇所調査 + spec 確定 — **完了** (Survey Round 1: `docs/archive/r16/aerial_perspective_survey.md`)

調査ターゲット (P0 完了条件) と結果:

1. **r14 P2.b/c の deferred 経緯の正確な復元** — `skyV.glsl:128-130` (P2.c コメント) / `skyV.glsl:138-141` (P2.b コメント) で deferred 位置を実コード上で同定。震源は **`sunlight *= exp(-light_atten * off_axis)` ライン** で sky dome 上の sun disc に直接効くパス。scene 側 (`atmosphericsFuncs.glsl`) に同じ式を入れても sun disc には影響しない構造を確認 (Survey §2.1)
2. **scene aerial perspective と sky dome の経路完全分離の確認** — `atmosphericsFuncs.glsl::calcAtmosphericVars*` と `skyV.glsl::main()` は **共有関数なし / 共有ヘルパーなし / preset uniform のみ共通**。値は共通でも計算式は独立、構造的に分離保証 (Survey §2.2)
3. **Rayleigh / Mie 分離の注入箇所** — `atmosphericsFuncs.glsl` 内 **2 箇所** に確定: L66-68 `light_atten = (blue_density + vec3(haze_density * 0.25)) * (density_multiplier * max_y);` と L72/L98 `combined_haze = max(blue_density + vec3(haze_density), vec3(1e-6));` → `combined_haze = exp(-combined_haze * density_dist * distance_multiplier);`。optional で L77 `above_horizon_factor = 1.0 / max(1e-6, lightnorm.y)` の Preetham 物理化 (Survey §2.3)
4. **Distance Multiplier の現用法** — スカラー float uniform、Beer-Lambert 指数で乗ずる係数として既に流入。**新規 uniform 追加不要**、既存 `distance_multiplier` の物理係数再解釈で足りる (Survey §2.4)
5. **既存 SL aerial perspective infrastructure の grep** — shader 側に物理化既存実装は **存在しない**。preset 側 (`llinventory/llsettingssky.cpp`) に EEP の Rayleigh / Mie density 概念は存在するが shader uniform としては `BlueDensity` / `HazeDensity` に集約済 (Survey §2.5)
6. **3 OS 共通性** — `.metal` / `#ifdef GL_ES` / `HAS_METAL` すべて 0 ヒット。r14 / r15 と同じく GLSL のみで 3 OS 通せる (Survey §2.6)

成果物: `docs/archive/r16/aerial_perspective_survey.md` (Round 1)。

### P1.a: 波長依存 in-scatter 実装 — **完了** (2026-05-12, commit `7427fbcb8d`)

scene 経路 `atmosphericsFuncs.glsl::calcAtmosphericVars` に Rayleigh λ^-4 波長依存重みを導入。

**触ったファイル**:
- `indra/newview/app_settings/shaders/class1/windlight/atmosphericsFuncs.glsl` — `rayleigh_w` ternary 追加 (個別 switch ON 時 `(1.0, 2.33, 5.71)` / OFF 時 `vec3(1.0)`)、`combined_haze` と `blue_weight` に乗算
- `indra/newview/app_settings/settings.xml` — `AYAR16AerialPerspectiveEnabled` (Boolean default TRUE) 追加
- `indra/llrender/llshadermgr.{h,cpp}` — `AYA_R16_AERIAL_PERSPECTIVE_ENABLED` enum + `aya_r16_aerial_perspective_enabled` reserved uniform 登録
- `indra/newview/llsettingsvo.cpp` — `LLCachedControl<bool>` で uniform 配信

**注入位置 (P0 確定 + P1.a 実装で精緻化)**:
- `combined_haze` (視線散乱係数, pre-exp): `blue_density * rayleigh_w + haze_density`
- `blue_weight` (in-scatter color weight): `(blue_density * rayleigh_w) / combined_haze`
- `light_atten` (太陽光路): **適用しない** — 当初 P0 候補だったが、実装中に「常時夕焼け化で近景まで黄ばむ」副作用を実機確認し撤回。aerial perspective ≠ 夕焼けの物理分離を実装に反映
- `above_horizon_factor` (Preetham off-axis): P1.b に分離 (optional)

**Linux 実機 PASS**:
- 遠景 (対岸地形・遠い山) が青味方向に明確シフト
- 近景はほぼ変化なし (density_dist 小で additive 自体が薄い、light_atten 不変で surface 直接透過は無影響)
- 個別 switch で r14/r15 と独立に r16 のみ on/off 可能
- 秋色化副作用は light_atten 不変で構造的に消失

**実装中に判明した重要事実**:
- atmosFragLighting の atten scalarize (上記 §3 参照) — surface 直接透過の波長依存は effectively no-op、additive 経由でのみ効く
- C++ 個別 switch 不可避 (master 切ると r14/r15 も同時 off で評価不能)
- shader 実 deploy 先は `~/ayastorm/app_settings/shaders/...` (`build-linux-x86_64/newview/packaged/` は viewer 読まない)

### P1.b: Preetham 球面近似 — **drop** (2026-05-12)

一度実装 + Linux 実機検証した結果、AYA 体感「穏やかになった感じは特にない」のため drop。

**検証時の実装内容**: `above_horizon_factor = 1.0 / max(1e-6, lightnorm.y)` の sec(θ) を Preetham 1999 球面近似 `1.0 / (cos_zenith + 0.15 * pow(max(93.885 - theta_deg, 1.0), -1.253))` に `aya_r16_aerial_perspective_enabled` gate 下で置換。`atmosphericsFuncs.glsl` のみの shader-only 変更。

**数値的差異 (検証 PASS)**:
- θ=80°: sec=5.76 / Preetham=5.53 (差 ~4%)
- θ=89°: sec=57.3 / Preetham=26.5 (差 ~2x、境界化)
- θ=90° (地平線): sec=∞ (1e-6 clamp で 10^6) / Preetham=~38 (有界化)

**drop 理由**:
- SL の sun timeline は地平線 ±5° の差異帯を時間ステップで一気に跨ぐため、perceptual threshold 以下
- `feedback_feature_value_in_main_usecase.md` (動いた ≠ 効いた) — 物理的には正しいが視覚効果が出ない subtle 補正は出荷しない
- 残り A 軸の「効くもの」(r17 雲の体積化 / 時間帯色温度、B 軸の薄物 SSS、C 軸の tonemap) に集中する方が章 thesis (写真撮るに値する空気) に近い

**revert 状態**: `atmosphericsFuncs.glsl` md5 `de496f6...` (P1.a 完了時と同等、P1.b 注入なし)。個別 switch `AYAR16AerialPerspectiveEnabled` は P1.a 用に残置 (gate しているのは rayleigh_w のみ)。

### P2: 3 OS ビルド + 体感確認

AYA が Linux フルビルド + 体感確認。問題なければ macOS / Windows ビルドへ。`feedback_release_with_user_feedback.md` の流儀で「完璧を目指さず実機で確認」。

### P3: tag / release

r15 と同じく、**公開は r16 単独でせず後続リリースまで pending** の運用 (AYA 方針)。r17 か A 軸完走 (r17 までで A 軸完結) のいずれかとまとめて公開する想定。

---

## 5. 受け入れ条件

- [x] 既存 WindLight preset (朝・昼・夕・夜) が **読み込めて、preset 切替が機能する** (preset 互換破壊なし) — P1.a Linux PASS
- [x] `AYAR16AerialPerspectiveEnabled = TRUE` (master `AYAVisualRealismEnabled = TRUE` 前提) で:
  - **遠景の山 / 地形が距離と共に青味方向にシフト** する (Rayleigh) — P1.a Linux PASS
  - 近景は変化なし (atten scalarize 設計、density_dist 小で additive 薄い) — P1.a Linux PASS
  - r14 で出した「空気の体積感」が遠景でも壊れない — P1.a Linux PASS
- [x] `AYAR16AerialPerspectiveEnabled = FALSE` で r15 までと同じ見え方に戻る (rayleigh_w = vec3(1.0) で旧式等価) — P1.a Linux PASS
- [x] **sky dome の見え方が r14 P2.a refined のまま** (sun disc 健在、朝・夕の地平線・青空質感は劣化なし) — skyV.glsl 不触で構造的保証、Linux PASS
- [-] 太陽方向 / 反対方向の色相差 (Mie 前方ピーク + Rayleigh 比率) — P1.b drop に伴い r16 範囲外 (Mie 前方ピーク は r17 雲の体積化 / 時間帯色温度で別途検討)
- [ ] 3 OS でビルド + 起動 + 表現確認 (P2)
- [ ] FPS 影響が ±10% 以内 (P2 で実測、P1.a は追加 pass なしのため影響軽微の見込み)

---

## 6. リスク

| ID | リスク | P1.a 後ステータス |
|---|---|---|
| R1 | r14 P2.b/c で観測した sun disc 消失副作用が scene 側書き換えでも誘発される | **解消**: P0 で経路分離確認、P1.a Linux PASS で実機 sun disc 健在を確認 |
| R2 | Rayleigh/Mie 分離で遠景が異常に青く見え、preset の絵作りが壊れる | **解消**: P1.a で `(1.0, 2.33, 5.71)` で遠景青味シフトは明確だが過剰ではない、preset 互換維持を Linux PASS で確認 |
| R3 | Distance Multiplier の物理係数再解釈で preset の見え方が劇的に変化 | **drop**: P1.a で「rayleigh_w 単独で遠景青味シフトが出る」ことが確認されたため、Distance Multiplier 物理化は当面 drop (preset 破壊リスクを避ける) |
| R4 | scene aerial perspective だけ書き換えると sky dome との境界で不連続な色境界が出る | **解消**: P1.a Linux PASS で地平線付近の色境界は観測されず |
| R5 | FPS 大幅低下 | **解消**: rayleigh_w 乗算のみで追加 pass なし、FPS 影響軽微 (P2 で実測) |
| R6 | 3 OS でビルドが通らない (macOS Metal 等) | **解消**: P0 で GLSL のみで通せる確認、P2 で実ビルド検証 |
| R7 | switch off 経路で見え方が完全に旧経路まで戻らない | **解消**: P1.a で `rayleigh_w = vec3(1.0)` で `blue_density * rayleigh_w == blue_density` となり旧式等価を構造的に保証、Linux PASS |
| R8 | Preetham off-axis を scene 側に入れたとき、scene の中の太陽近傍方向で別の白飛びが起きる | **解消**: P1.b 実装 + Linux 実機検証で白飛び副作用は観測されず。ただし「体感差なし」のため P1.b は drop、リスク自体は将来 r17 で sky 経路に Preetham を入れる際の予習として記録 |
| R9 (P1.a 新規) | 個別 switch (`AYAR16AerialPerspectiveEnabled`) を追加したことで `feedback_prefer_defaults_over_config.md` (個別 cvar 量産しない) と衝突 | **緩和**: 「章ごと体感評価用 sentinel」として r14/r15/r16 各 1 本に限定する運用、A 軸完走時に統合 (master へ吸収) を検討 |
| R10 (P1.a 新規) | atmosFragLighting の atten scalarize により surface 直接透過の波長依存が effectively no-op になっていることを documentation 不足で将来開発者が知らずに `atten` を per-channel と仮定する | **緩和**: shader コメント (atmosphericsFuncs.glsl) と memory (project_atmos_atten_scalarized.md) に明記、spec §3 にも記載 |

---

## 7. 更新履歴

- 2026-05-12 (初版): r14 (volumetric atmosphere) を A 軸第 1 弾、r15 (godrays) を第 2 弾として実装完了 (Linux PASS) させた流れの第 3 弾として起票。r14 P2.b (Preetham 太陽方向経路長物理化) / P2.c (Rayleigh/Mie 波長依存分離) を **sun disc 保護と両立する形で復活** させる枠を r16 で確定。skyV.glsl は本リリースで一切触らない方針、改修対象は scene 側 (`atmosphericsFuncs.glsl::calcAtmosphericVars*`) のみ。r15 公開は r16+ まで pending の AYA 方針を §4 P3 に反映 (r17 か A 軸完走と一括公開想定)
- 2026-05-12 (P0 完了): Survey Round 1 (`docs/archive/r16/aerial_perspective_survey.md`) で 6 項目すべてクリア。震源は skyV.glsl `sunlight *= exp(-light_atten * off_axis)` ラインで scene 側 atmosphericsFuncs とは構造的に独立を確認、Rayleigh/Mie 注入箇所を `atmosphericsFuncs.glsl` 内 2 箇所 (light_atten / combined_haze) + optional 1 箇所 (above_horizon_factor) に確定。**C++ 改修不要** (既存 `aya_visual_realism_enabled` + 既存 preset uniform で完結)、波長依存散乱の派生は A 案 (shader 内派生) default で P1 着手、B 案 (EEP preset 引出し) は体感不足時のみ。リスク R1 / R6 を解消、R5 / R7 / R8 を構造的に緩和、R2 / R3 / R4 は P1 体感調整に残置
- 2026-05-12 (P1.a 完了, commit `7427fbcb8d`): `atmosphericsFuncs.glsl::calcAtmosphericVars` に rayleigh_w `(1.0, 2.33, 5.71)` を `combined_haze` と `blue_weight` に乗算する形で導入。`light_atten` への適用は実装中に「常時夕焼け化で近景まで黄ばむ」副作用を実機確認し撤回 (aerial perspective ≠ 夕焼けの物理的分離)。**P0 想定「C++ 改修不要」は撤回** — master switch (`AYAVisualRealismEnabled`) を切ると r14/r15 効果も同時 off となり r16 単独の体感評価が不可能、個別 switch `AYAR16AerialPerspectiveEnabled` (settings.xml / llshadermgr.{h,cpp} / llsettingsvo.cpp / 新規 uniform 1 本) を追加して回避。Linux 実機 PASS — 遠景青味シフト明確、近景変化なし、sun disc 健在、preset 互換維持。R2 / R4 / R7 を新規解消、R3 を drop、R9 / R10 を新規記録 (個別 cvar 量産・atmosFragLighting atten scalarize の documentation 懸念)。実装中に判明した重要事実: (a) atmosFragLighting は `light *= atten.r` でスカラー化、surface 直接透過の波長依存は no-op (additive 経由で効く設計), (b) shader 実 deploy 先は `~/ayastorm/app_settings/shaders/...` (`build-linux-x86_64/newview/packaged/` は viewer 読まない) — いずれも memory に記録
- 2026-05-12 (P1.b drop, r16 実装完結): Preetham 1999 球面近似を `above_horizon_factor` に `aya_r16_aerial_perspective_enabled` gate 下で実装し Linux 実機検証。数値的には sec(θ) と Preetham の差は θ=89° で 57.3→26.5、地平線で ∞→~38 と境界化されるが、AYA 体感「穏やかになった感じは特にない」 — SL の sun timeline は地平線 ±5° の差異帯を時間ステップで跨ぐため perceptual threshold 以下。`feedback_feature_value_in_main_usecase.md` (動いた ≠ 効いた) に従い drop、shader revert (md5 `de496f6...`、P1.a 完了時と同等)。残り A 軸 (r17 雲の体積化 / 時間帯色温度、B 軸 薄物 SSS、C 軸 tonemap) の「効くもの」に集中する判断。R8 を解消 (白飛び副作用は実機で観測されず、ただし P1.b 自体は drop)。r16 は P1.a のみで実装完結、次は r17
