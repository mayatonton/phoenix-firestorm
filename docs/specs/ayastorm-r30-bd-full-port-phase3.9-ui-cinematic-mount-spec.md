# AYAstorm r30 BD full port — Phase 3.9 UI Cinematic mount spec

**作成日**: 2026-05-19
**ブランチ**: `feature/ayastorm-r30-bd-full-port-inventory`
**Phase**: 3.9 (UI BD floater 移植 + bdsidebar mount)
**前提**: Phase 3.7 (C++ Cinematic dispatch) / Phase 3.8 (shader Cinematic mount) 完了済
**章**: r30 BD full port (`feedback_bd_full_port_only.md`: 増分 borrow / preset 禁止、BD pipeline 全体 1:1 移植のみ)

---

## 1. 目的

Phase 0 inventory §4.1 で特定された **BD-only UI XML 9 件**、および本 spec 起票時の追加調査で判明した **inventory 漏れ bdsidebar.{cpp,h} (1021 行) + 4 caller patch + 4 BD-only floater cpp/h** を AYAstorm に移植し、Cinematic mode (AYAVisualRealismEnabled=2) で BD の主要 UI (Machinima Sidebar) が出現するようにする。

非 Cinematic mode (mode 0=Firestorm View / 1=AYAstorm View) では bdsidebar は不可視。

---

## 2. 移植対象 (確定)

### 2.1 UI XML 9 件 (Phase 0 inventory §4.1)

| # | file | size | 親 ref / register 経路 |
|---|---|---|---|
| 1 | `floater_adjust_water.xml` | 10,939 B | `LLFloaterReg::add("env_adjust_water", ...)` — bdsidebar から呼出 |
| 2 | `floater_edit_sky_preset.xml` | 33,514 B | **BD でも未 register** (`llfloatereditsky.cpp` は orphan / legacy) |
| 3 | `floater_edit_water_preset.xml` | 12,259 B | **BD でも未 register** (`llfloatereditwater.cpp` は orphan / legacy) |
| 4 | `floater_environment_settings.xml` | 7,077 B | `LLFloaterReg::add("env_settings", ...)` |
| 5 | `panel_machinima.xml` | 82,832 B | `bdsidebar.cpp::buildFromFile("panel_machinima.xml")` — **本丸** |
| 6 | `panel_preferences_render_settings.xml` | 2,778 B | `floater_preferences.xml` から ref (AY 側 floater_preferences.xml にも追記要) |
| 7 | `panel_preferences_ui_colors.xml` | 22,448 B | 同上 |
| 8 | `panel_settings_water_image.xml` | 2,121 B | `floater_adjust_water.xml` + `floater_edit_ext_day_cycle.xml` (common, diff) から ref |
| 9 | `panel_settings_water_settings.xml` | 5,811 B | 同上 |

### 2.2 C++ 側 (inventory §2.B BD-only newview + **inventory 漏れ bdsidebar**)

| # | file | inventory 掲載 | 備考 |
|---|---|---|---|
| A | `bdsidebar.cpp` (803 行) | **漏れ** | panel_machinima.xml を build。`gSideBar` グローバル singleton |
| B | `bdsidebar.h` (218 行) | **漏れ** | `LLSideBar` class、`extern LLSideBar* gSideBar` |
| C | `llfloatereditsky.cpp` (22 KB) | 掲載済 | .h なし (class 定義 cpp 内)、未 register、移植は形だけ |
| D | `llfloatereditwater.cpp` (17 KB) | 掲載済 | 同上 |
| E | `llfloaterenvironmentsettings.cpp` (7 KB) | 掲載済 | `env_settings` register、移植する |
| F | `llfloaterenvironmentsettings.h` (2 KB) | 掲載済 | 同上 |
| G | `llfloaterwateradjust.cpp` (18 KB) | 掲載済 | `env_adjust_water` register、移植する |
| H | `llfloaterwateradjust.h` (4 KB) | 掲載済 | 同上 |

### 2.3 caller patch (4 ファイル)

| # | file | 行 | 内容 |
|---|---|---|---|
| α | `llviewerwindow.cpp` | 209 / 1952 / 2338-2346 / 2496 | `#include "bdsidebar.h"` + `mMachinimaSidebar` ポインタ初期化 + `gToolBarView->getChild<LLPanel>("machinima")` 経由で取得 + `gSideBar` 生成 + dtor で NULL |
| β | `llviewerwindow.h` | 582 | `LLPanel* mMachinimaSidebar = nullptr;` メンバ追加 |
| γ | `llviewercontrol.cpp` | 90 / 1016-1018 | `#include` + cvar 変更時 `gSideBar->refreshGraphicControls()` |
| δ | `llfloaterpreference.cpp` | 125 / 2070 | `#include` + preference apply 時 `gSideBar->refreshGraphicControls()` |
| ε | `llagent.cpp` | 102 / 2364 | `#include` + mouselook 切替時 `gSideBar->setVisibleForMouselook(true)` |

### 2.4 toolbar bind (`gToolBarView->getChild<LLPanel>("machinima")`)

BD は `toolbars.xml` または `main_view.xml` 系のどこかに `name="machinima"` の LLPanel を bind している。実装時に BD の `skins/default/xui/en/main_view.xml` / `toolbars.xml` を確認し、AY 側 main_view へ追記する。

### 2.5 viewerfloaterreg register

| key | xml | class |
|---|---|---|
| `env_adjust_water` | `floater_adjust_water.xml` | `LLFloaterWaterAdjust` |
| `env_settings` | `floater_environment_settings.xml` | `LLFloaterEnvironmentSettings` |

`env_adjust_water` / `env_settings` は AY 側に既存があるか要確認。あれば衝突回避 (BD 版で上書きが原則だが、AY 側機能を Cinematic mode で BD 版に差し替える戦略)。

---

## 3. Cinematic gating 戦略

### 3.1 bdsidebar visibility

- `mMachinimaSidebar->setVisible(true)` を **Cinematic mode (=2) のみ** に gate
- 実装案: `llviewerwindow.cpp:2338` の `if (!mMachinimaSidebar)` の前段に `if (AYAVisualRealismEnabled != 2) return;` を追加
- AY 側 floater_aya_cinematic.xml (r30 P4 で起こした AY 用 Cinematic Controls floater, 534 行) は **共存** させる
  - panel_machinima は BD 1:1 移植 (右側 sidebar 常駐型)
  - floater_aya_cinematic は AY 拡張 (フロート、save_rect=true)
  - Cinematic mode で **両方利用可** = BD parity + AY 拡張併存

### 3.2 floater_adjust_water / floater_environment_settings

- Cinematic mode (=2) のみで bdsidebar から開かれるルートが活性化
- 直接 `LLFloaterReg::showInstance("env_adjust_water")` を打つ caller を mode で gate するか、floater 内部で空 panel に切替するかは実装時判断
- **default: register は常に行い、bdsidebar 経由でのみ実用上開かれる → mode gate は bdsidebar 側に集約**

### 3.3 panel_preferences_render_settings / panel_preferences_ui_colors

- floater_preferences.xml に常時 ref (mode gate せず)
- BD-only な preferences タブが Cinematic mode 関係なく見える状態を許容
- 理由: preferences タブを mode 切替で出現/消失させるのは UX 上不健全 (チェックボックス変更 → 再起動 → タブ消失は混乱)
- BD parity 観点で「Cinematic 関連 cvar 専用 preferences タブ」は常時見えてよい (実効性は mode に依存)

### 3.4 panel_settings_water_image / panel_settings_water_settings

- common floater (`floater_adjust_water.xml` / `floater_edit_ext_day_cycle.xml`) から ref
- floater 内 panel なので gate 不要、floater が開かれたときのみ instantiate

---

## 4. orphan 扱い (`llfloatereditsky` / `llfloatereditwater`)

`llfloatereditsky.cpp` / `llfloatereditwater.cpp` は BD 自身でも `LLFloaterReg::add` されていない (今回の grep 確認済)。

**判断**: BD 1:1 完全移植原則 (`feedback_bd_full_port_only.md`) に従い、ファイルは置く (移植する) が CMakeLists.txt / viewerfloaterreg への追加はしない (= BD と同じ orphan 状態を保つ)。

将来 BD upstream が register した時点で AY 側も追従。

---

## 5. 実装ステップ

### Step 0: AY 既存衝突調査
- `env_adjust_water` / `env_settings` key 衝突 check
- AY 側 `floater_preferences.xml` の既存タブ並びとの衝突 check
- BD `main_view.xml` / `toolbars.xml` の `name="machinima"` panel bind 場所特定

### Step 1: bdsidebar.{cpp,h} 移植 (commit 1)
- `bdsidebar.cpp` (803 行) / `bdsidebar.h` (218 行) を `/tmp/bd-baseline/indra/newview/` から `indra/newview/` へコピー
- `indra/newview/CMakeLists.txt` に追加
- 単体で compile 通る状態にする (caller patch なし)

### Step 2: 4 BD-only floater cpp/h 移植 (commit 2)
- `llfloaterenvironmentsettings.{cpp,h}` / `llfloaterwateradjust.{cpp,h}` を移植
- `llfloatereditsky.cpp` / `llfloatereditwater.cpp` も移植 (orphan のまま)
- CMakeLists.txt 追加
- `llviewerfloaterreg.cpp` に `env_adjust_water` / `env_settings` register 追加 (AY 既存と衝突あれば回避)

### Step 3: UI XML 9 件配置 (commit 3)
- `indra/newview/skins/default/xui/en/` に 9 件配置
- `floater_preferences.xml` に panel_preferences_render_settings / panel_preferences_ui_colors ref 追加
- `floater_edit_ext_day_cycle.xml` (common diff yes) を BD 側に揃えるか、water_image/water_settings panel ref のみ追記するか実装時判断

### Step 4: 4 caller patch (commit 4)
- `llviewerwindow.{cpp,h}` / `llviewercontrol.cpp` / `llfloaterpreference.cpp` / `llagent.cpp` に bdsidebar 連携追加
- `mMachinimaSidebar` mode gate (=2) を `setVisible` の前に挿入

### Step 5: toolbar bind 追加 (commit 5)
- AY 側 `main_view.xml` または `toolbars.xml` に `name="machinima"` の LLPanel を追加
- 右側 sidebar slot として 345px 幅確保

### Step 6: build verify + Cinematic mode 動作確認 (AYA 実行)
- `autobuild build -A 64 -c ReleaseFS_open --no-configure --fmodstudio`
- Cinematic mode で bdsidebar 出現 / Firestorm View / AYAstorm View で非表示の 3 mode 確認

---

## 6. リスク / 留意点

1. **panel_machinima.xml は 82 KB** = BD で最大規模 panel。中身に bdsidebar.cpp が想定する control 名 (button / slider) が大量にあり、bdsidebar.cpp の `getChild<LLButton>("xxx")` がすべて bind 通る必要あり。移植時の 1 文字違いで NULL pointer crash 直結。
2. **`gSideBar` global singleton** は llappviewer の shutdown 順序と要注意。dtor で `gSideBar = NULL` だけでは leak の可能性 (BD でも同じ実装)。
3. **mMachinimaSidebar の toolbar bind** は BD 独自の toolbar XML が必要かもしれず、AY 側既存 toolbar への追記で済むか要実装時調査。
4. **既存 floater_aya_cinematic.xml との UX 衝突**: 両方開くと画面右が重複する可能性。3.1 で「共存」としたが、運用上は片方ずつ使うのが現実的。release note で明示。
5. **mode 1 (AYAstorm View) で BD sidebar 非表示の妥当性**: AY r14-r20 拡張は mode 1 でも動く。Cinematic 専用にしたいのは BD pipeline 一式の方なので「BD sidebar = mode 2 専用」は thesis 整合。

---

## 7. 受入条件

1. Cinematic mode (=2) で再起動後、画面右に panel_machinima 由来の Machinima Sidebar が出現
2. Firestorm View (=0) / AYAstorm View (=1) 起動時、Sidebar 非表示
3. Sidebar 内の slider / button が control に bind され、変更が graphics 設定に反映 (`gSideBar->refreshGraphicControls()` 経由で双方向)
4. mouselook 切替で sidebar 適切に visible/hide
5. preferences タブに Render Settings / UI Colors が追加 (全 mode で表示)
6. env_adjust_water / env_settings floater が menu (or bdsidebar) から開ける
7. build 通過 (LL_INFOS 警告 OK、error なし)
