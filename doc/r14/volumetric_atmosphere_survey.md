# r14 P0 Survey: volumetric atmosphere

**作成日**: 2026-05-12
**対象**: AYAstorm r14 (volumetric atmosphere) の実装箇所調査
**位置づけ**: `docs/ayastorm-r14-volumetric-atmosphere.md` §4 P0 の作業ノート

> **本書の役割**: P0 調査の進行ログ + 確定事項。`sun_rendering_survey.md` (P0 round 1、sun/halo 解明) とは別調査で、対象は atmospherics の本流 (fog / scattering / preset 値の流入経路)。

---

## 1. 調査ターゲット (open questions)

P0 完了時にこれらが解けている状態を目指す。

### Q1. atmospherics shader の本流はどれか
- 候補ファイル: `indra/newview/app_settings/shaders/class3/deferred/` 配下の `softenLightF.glsl`、`atmosphericsF.glsl`、`atmosphericsFuncs.glsl`、`atmosphericsVars.glsl` 等
- 何が「fog」「haze」「scatter」を実際に適用しているか、HDR scene buffer のどの段階で乗るか
- class1 / class2 / class3 / deferred / interface の階層関係 (`LLViewerShaderMgr` の shader_level / quality 設定で切り替わる)

### Q2. WindLight preset 値の uniform plumbing
- preset の Haze Horizon, Haze Density, Blue Density, Blue Horizon, Density Multiplier, Distance Multiplier, Ambient, Sunlight Color がどの uniform 経由で shader に届くか
- C++ 側のキャリア (`LLSettingsSky` / `LLEnvironment` / `LLAtmosphere` 等)
- shader 側の receiving uniform 名

### Q3. fog 適用の正確な式
- 現状の fog は exp / linear / Bouguer-Lambert のどれか
- depth がどう入るか (linear depth / log depth / world distance)
- altitude が考慮されているか (現状では地表-上空の差が出ない理由)

### Q4. in-scatter の有無
- 現状 sun 方向と空気色の関係はどう計算されているか
- haze_glow (`skyV.glsl` で round 1 解明済み) と atmospherics の in-scatter は別物か同一か
- aerial perspective (距離による色相変化) は既存に存在するか / 形だけあって効いていないか

### Q5. master switch の plumbing 候補
- 上位で分岐する場所 (LLPipeline / shader compile permutation / uniform branch のどれが妥当か)
- r11 sentinel 流儀 (LLCachedControl で settings.xml → C++ → uniform) の踏襲可能性

### Q6. 3 OS 差異
- macOS Metal 経路で shader 互換性に gotcha があるか
- Win 側 ANGLE / native GL の差異

---

## 2. 調査手順 (proposed)

memory `feedback_proactive_diagnostic.md` + `feedback_doubt_self_first.md` に従い、Claude 側で直接実行可能な調査は Bash / Grep / Read で進める。AYA へ投げるのは viewer 内 UI / 体感に限定。

### Step 1: shader 階層の俯瞰
- `indra/newview/app_settings/shaders/class*/deferred/` を ls + 主要 atmospherics ファイルの存在確認
- class1/2/3 の使い分けルール (shader_level、PBR/non-PBR、quality preset) を `LLViewerShaderMgr` から確認

### Step 2: fog / haze 適用点の特定
- atmospherics 系 shader の中で `vec3 atmosFragLighting` 等の合成式を grep
- HDR scene buffer (mRT->screen) への書き込みに fog がいつ乗るか追跡

### Step 3: preset 値の流入経路
- C++ で `LLSettingsSky::SETTING_HAZE_DENSITY` 等の定数を grep
- 該当 uniform を shader 側で grep
- `LLEnvironment::updateGLVariablesForSettings` 等の uniform upload 点を確認

### Step 4: master switch 設計
- `RenderXxxEnabled` 系 LLCachedControl の既存パターンを grep (r11/r13 から事例)
- 挿入点を決定 (uber shader 入り口推奨)

### Step 5: r14 書き換え範囲の確定
- 上記から「触るファイル list」「触る関数 list」「新規 uniform list」を確定
- r14 spec §4 P1 着手の前提を満たす

---

## 3. 調査ログ (P0 進行中)

### Round 0 (2026-05-12)

- 本 survey 初版作成
- 既存 `sun_rendering_survey.md` (round 1+2) は sun disc / halo / glow extract 経路を解明済み。本 survey は atmospherics 本流に絞る

### Round 1 (2026-05-12): shader 階層 + preset plumbing 解明

**shader 階層 (Q1 確定)**
- atmospherics 中央関数: `class1/windlight/atmosphericsFuncs.glsl` の `calcAtmosphericVars()` / `calcAtmosphericVarsLinear()`
- 専用 deferred haze pass: `class3/deferred/hazeF.glsl` (depth >= 1.0 で discard、それ以外で additive を出す)
- deferred lighting の主役: `class3/deferred/softenLightF.glsl` (PBR / HDRI / SKIP_ATMOS / legacy の 4 分岐、calcAtmosphericVarsLinear を呼ぶ)
- helpers: `class1/windlight/{atmosphericsF, atmosphericsV, atmosphericsHelpers{F,V}, atmosphericsVars{F,V}}.glsl`

**現状の式 (Q3/Q4 確定) — 既に存在する基盤**
- **Beer-Lambert は実装済み**: `atmosphericsFuncs.glsl:84` `combined_haze = exp(-combined_haze * density_dist * distance_multiplier)`、`atten = combined_haze.rgb`
- **sunlight attenuation も Beer-Lambert**: `atmosphericsFuncs.glsl:76` `sunlight *= exp(-light_atten * above_horizon_factor)`
- **in-scatter (haze_glow) も実装済み**: `dot(rel_pos_norm, lightnorm)` を取って `pow(., glow.z)` で sharpen、`additive = blue_horizon*(cs+ambient) + haze_horizon*(cs*haze_glow + ambient)` で sun direction の散乱寄与を表現
- **欠落点**:
  - altitude density (高度依存の密度勾配) — `max_y` で clamp する以外、地表と上空が同濃度
  - scene-referred 積分 — additive を sRGB 空間で組んで後で srgb_to_linear、物理整合性低
  - 反対方向の Rayleigh/Mie 区別 — haze_glow は太陽方向のみ
  - 波長依存散乱 — preset カラーチャンネルに丸投げ、物理パラメタ未分離

**preset plumbing (Q2 確定)**
- `LLSettingsVOSky::applyToShader()` (`indra/newview/llsettingsvo.cpp` ~L780)
- 個別 reserved uniform enum: `LLShaderMgr::BLUE_HORIZON` 等 (`indra/llrender/llshadermgr.h:127-` 付近)
- uniform 名 push: `mReservedUniforms.push_back("blue_horizon")` 等 (`indra/llrender/llshadermgr.cpp:1319-` 付近)
- 静的 param_map (`LLSettingsVOSky::getParameterMap`): SETTING_ 名 → SHADER_UNIFORM の対応
- 個別 `shader->uniform3fv(LLShaderMgr::BLUE_HORIZON, ...)` で送る path もあり

**master switch 設計 (Q5 確定) — `classic_mode` が完全な template**
- `LLShaderMgr` enum に `AYA_VISUAL_REALISM_ENABLED` を追加 (`llshadermgr.h:126` 付近)
- `mReservedUniforms.push_back("aya_visual_realism_enabled")` (`llshadermgr.cpp:1318` 付近)
- `llsettingsvo.cpp:applyToShader` 内で:
  ```cpp
  static LLCachedControl<bool> aya_visual_realism(gSavedSettings, "AYAVisualRealismEnabled", true);
  shader->uniform1i(LLShaderMgr::AYA_VISUAL_REALISM_ENABLED, aya_visual_realism ? 1 : 0);
  ```
- shader 側 (`atmosphericsFuncs.glsl`) で `uniform int aya_visual_realism_enabled;` を受けて `calcAtmosphericVars` 内で分岐
- `settings.xml` に `AYAVisualRealismEnabled` (Boolean, default 1) を追加

**3 OS 差異 (Q6 確定)**
- `.metal` ファイル無し → 3 OS 共通 GLSL
- atmosphericsFuncs.glsl に `#ifdef` 無し
- 心配無用、Linux でやった shader 変更がそのまま 3 OS で動く想定で OK

---

## 4. 確定事項 (P0 完了)

### 触るファイル list
- `indra/llrender/llshadermgr.h` — `AYA_VISUAL_REALISM_ENABLED` enum 追加
- `indra/llrender/llshadermgr.cpp` — `mReservedUniforms` に `"aya_visual_realism_enabled"` 追加
- `indra/newview/llsettingsvo.cpp` — `LLSettingsVOSky::applyToShader` で `LLCachedControl` + `uniform1i`
- `indra/newview/app_settings/settings.xml` — `AYAVisualRealismEnabled` Boolean default 1
- `indra/newview/app_settings/shaders/class1/windlight/atmosphericsFuncs.glsl` — `calcAtmosphericVars` 内に新経路 + master switch 分岐 (altitude density + scene-referred 積分)
- 必要に応じて `class3/deferred/hazeF.glsl` / `softenLightF.glsl` (新経路を pass 越しに使う場合)

### 触る関数 list
- `LLSettingsVOSky::applyToShader` (uniform 追加)
- `calcAtmosphericVars` / `calcAtmosphericVarsLinear` (新経路追加、altitude density 実装、scene-referred 積分)

### 新規 uniform list
- `aya_visual_realism_enabled` (int)
- (場合により) altitude density 用の追加 uniform (例 `altitude_density_scale` 1 本のみ、`feedback_prefer_defaults_over_config.md` に従い増やさない)

### master switch 挿入点
- shader: `calcAtmosphericVars` 入口で `if (aya_visual_realism_enabled > 0) { 新経路 } else { 旧経路 (現状の式) }`
- C++: `LLSettingsVOSky::applyToShader` の `classic_mode` 設定箇所の直後

### r14 工数感の P0 後再見積
- **完全書き換え不要**: 既存 Beer-Lambert + in-scatter 基盤が想像以上に厚い
- **追加実装**: altitude density (高度依存) + scene-referred 積分への移行 + master switch
- 工数感 3〜5 日 (3 OS 込み) は妥当、ただし scene-referred 移行で `srgb_to_linear` のタイミング再設計が要るので 5 日寄り
- **r14 spec の見直し**: 当初の spec は「既存式の全置換」だったが、実態は「既存式の物理化 + altitude 拡張」。spec §3 スコープを再記述する必要あり (r14 spec next revision)

### r14 で扱わない (→ r15+ 再確認)
- 反対方向の Rayleigh/Mie 分離 (godrays / aerial perspective 系で r15-r16 に回す)
- 波長依存散乱の物理分離 (時間帯色温度との抱き合わせで r17 に回す)
