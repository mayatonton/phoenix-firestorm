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
- 着手は AYA からの go サイン後

---

## 4. 確定事項 (P0 完了時に埋める)

(P0 進行に伴い記載)

- 触るファイル list:
- 触る関数 list:
- 新規 uniform list:
- master switch 挿入点:
- r14 工数感の P0 後再見積:
