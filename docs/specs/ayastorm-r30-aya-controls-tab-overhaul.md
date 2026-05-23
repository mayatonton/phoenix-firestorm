# AYAstorm r30: AYAstorm Controls — tab 縦並び化 + Skin SSS タブ移行

**Date:** 2026-05-24
**Branch:** `feature/r30-aya-controls-left-tabs-sss-move`
**Base:** `ayastorm-release` (r30 Cinematic Cleanup / view-mode-reshuffle 後)
**Status:** ローカル実装完了、AYA hands-on 検証待ち
**Related:**
- `docs/specs/ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md` (Cinematic floater 構築の原典)
- `docs/specs/ayastorm-r30-cinematic-controls-cleanup.md` (r30 floater 監査・整理)
- `docs/specs/ayastorm-r30-view-mode-reshuffle.md` (Cinematic → AYAstorm View promote)
- `docs/specs/ayastorm-r20-avatar-skin-sss.md` (r20 SSS feature spec、UI 配置のみ本 spec で更新)

---

## 1. 背景

r30 Phase 6 + Cleanup + view-mode-reshuffle を経て **AYAstorm Controls** (旧 Cinematic Controls) は 35+ cvar / 10 タブまで成長。tab が横一列に並ぶ従来 layout では:

- AYAstorm Controls 内タブが横スクロール必須、UX 悪化
- r20 SSS は Preferences > Graphics 配下に独立タブで存在し、撮影描画系設定の所在が二分されていた (AYAstorm Controls / Preferences > SSS)

AYA さんからの 2 件の要請を同一 branch で合流:

1. **SSS タブの AYAstorm Controls への合流** — 撮影描画系設定の一元化
2. **Tab 縦並び化** — Preferences floater 同様の左サイドバー方式に変更してスクロール解消

両者とも UI 配置の整理であり、ビルド断面・ユーザー検証手順を共有できるため bundle 採用 (1 commit / 1 PR / 1 release note)。

---

## 2. 採用 approach

### 2.1 Tab orientation: top → left

`floater_aya_cinematic.xml` の `<tab_container>` 属性を変更:

| 属性 | before | after | 出典 |
|---|---|---|---|
| `tab_position` | `"top"` | `"left"` | Preferences pattern (`floater_preferences.xml`) |
| `tab_width` | (未指定、auto) | `"130"` | label 最大長 "Glow & Volumetric" (17 char) / "Atmosphere & sky" (16 char) を切り捨てない最小値 |
| `tab_padding_right` | (未指定) | `"4"` | LL 既定の縦並び visual gap |
| `halign` | (default center) | `"left"` | label を左寄せに統一 |
| `tab_height` | `"24"` (top 用) | (削除、left では無視) | left では `tab_width` が主軸 |
| `follows` | `"all"` | `"all"` | 変更なし、再確認 |

連動する floater 寸法調整 (`tab_width=130` 分の content 領域確保):

| | before | after |
|---|---|---|
| `<floater width="...">` | `500` | `630` |
| `<tab_container width="...">` | `488` | `630` |
| `<tab_container left="...">` | `6` | `0` |

content 幅は `630 - 130 (tab_width) = 500`、既存パネル幅 488 を内包可能。

### 2.2 SSS panel migration: Preferences → AYAstorm Controls

**移行元 (削除):**
- `indra/newview/skins/default/xui/en/panel_preferences_sss.xml` (full delete)
- `indra/newview/skins/default/xui/en/panel_preferences_graphics1.xml` の `<panel class="panel_preference_sss" ...>` injection (2 行削除)
- `indra/newview/llfloaterpreference.h` の `LLPanelPreferenceSSS` class 宣言
- `indra/newview/llfloaterpreference.cpp` の `LLPanelPreferenceSSS` implementation (constructor / postBuild / 各 default ハンドラ / `LLPanelInjector` 登録)

**移行先 (新設):**
- `floater_aya_cinematic.xml` (en/ja) に Tab 11 "Skin SSS" 追加
- 含まれる 14 widget: Enabled checkbox / Blur radius slider / Strength slider / Glow gain slider / Glow color swatch / Whitelist lock checkbox / Whitelist text_editor / Reset All button + 各 default (D) button
- `panel.string "DefaultWhitelist"` 維持 (ResetAll 実装が依存)

**Callback の migration:**
旧 `LLPanelPreferenceSSS::onLockToggle` / `onResetAll` は class method だったため AYAstorm Controls 側 (FloaterQuickPrefs 派生) からは直接呼べない。`llviewermenu.cpp` に `view_listener_t` として再実装:

| 旧 | 新 | 実装 |
|---|---|---|
| `LLPanelPreferenceSSS::onLockToggle` | `AYASSSToggleLock` listener | `LLFloaterReg::findInstance("aya_cinematic")` + `findChild<LLTextEditor>("sss_whitelist", true)` で recursive lookup、`setEnabled()` 切替 |
| `LLPanelPreferenceSSS::onResetAll` | `AYASSSResetAll` listener | 上記 floater から `findChild<LLPanel>("tab_skin_sss", true)`、その `getString("DefaultWhitelist")` を取得して `gSavedSettings.setString("AYAR20AvatarSkinSSSWhitelist", ...)` |
| 個別 D button (`onDefaultBlurRadius` 等) | `AYAResetCinematic` 経由 | `parityTable()` に SSS 5 cvar 追加 (§2.3) |

`addMenu()` 登録は `llviewermenu.cpp` の AYAResetCinematic 登録直後に 2 行追加。`#include "lltexteditor.h"` を追加 (recursive lookup の type 解決)。

### 2.3 parityTable 追加 cvar (5 件)

`llviewermenu.cpp::AYAResetCinematic::parityTable()` に以下を追加:

| cvar | default | 備考 |
|---|---|---|
| `AYAR20AvatarSkinSSSEnabled` | `true` | r20 章 default |
| `AYAR20AvatarSkinSSSBlurRadius` | `1.0` | F32 |
| `AYAR20AvatarSkinSSSStrength` | `0.5` | F32 |
| `AYAR20AvatarSkinSSSGlowGain` | `0.2` | F32 |
| `AYAR20AvatarSkinSSSGlowColor` | `LLSD::array(R,G,B,A)` (lambda 構築) | Color4 |

Whitelist (`AYAR20AvatarSkinSSSWhitelist`) は parityTable には載せない。ResetAll button (= `AYASSSResetAll`) 側で `panel.string "DefaultWhitelist"` 経由で個別 reset する設計を維持。

---

## 3. 変更ファイル一覧 (7 件)

| File | 内容 |
|---|---|
| `indra/newview/skins/default/xui/en/floater_aya_cinematic.xml` | floater width 500→630 / tab_position="left" + tab_width=130 / Tab 11 "Skin SSS" 14 widget 追加 |
| `indra/newview/skins/default/xui/ja/floater_aya_cinematic.xml` | Tab 11 ja overrides ("肌 SSS"、各 label/tooltip 翻訳)、寸法 override なし (en 継承) |
| `indra/newview/llviewermenu.cpp` | `#include "lltexteditor.h"` / `parityTable` に SSS 5 cvar / `AYASSSToggleLock` + `AYASSSResetAll` listener 追加 + `addMenu()` |
| `indra/newview/llfloaterpreference.h` | `LLPanelPreferenceSSS` class 宣言 削除 (跡地に AYAstorm Controls へのポインタコメント) |
| `indra/newview/llfloaterpreference.cpp` | `LLPanelPreferenceSSS` implementation 全削除 (跡地にポインタコメント) |
| `indra/newview/skins/default/xui/en/panel_preferences_graphics1.xml` | `panel_preference_sss` injection 削除 |
| `indra/newview/skins/default/xui/en/panel_preferences_sss.xml` | `git rm` (panel ファイル本体削除) |

---

## 4. Skin 互換性 (調査済)

7 skin (ansastorm / default / firestorm / metaharper / starlight / starlightcui / vintage) 中、`floater_aya_cinematic.xml` を override しているのは **default のみ** (en/ja)。他 skin は default を inherit するので tab 配置変更は自動追従。

- vintage skin は `widgets/tab_container.xml` で異なる image set (`TabLeft_Top_Off/Selected` vs default の `SegmentedBtn_Left_Disabled/Selected_Over`) と `tab_height="18"` を持つが、いずれも有効な image asset reference であり起動異常リスクなし
- Preferences floater は同 `tab_position="left"` で全 skin にて稼働実績あり (= mechanism は枯れている)

---

## 5. 移行注意 (release note 用 / ユーザー向け)

- **SSS 設定の場所が変わります**: 環境設定 → グラフィック → SSS タブは廃止。Avatar → AYAstorm Controls (Alt+C) → "Skin SSS" タブに移行
- **設定値は維持**: cvar 名 (`AYAR20AvatarSkin*`) は変更なし、debug settings 経由でも従来通りアクセス可
- **r20 SSS 機能本体は変更なし**: shader 経路 / whitelist 形式 / Mesh body 判別ロジックは r20 spec のまま

---

## 6. 検証手順 (AYA 担当)

ビルド + cache clear 後:

1. **Tab 縦並び**: AYAstorm Controls (Alt+C) を開き、tab が左サイドバー方式に並んでいること
2. **Tab labels が切れていない**: 11 タブすべて読める ("Glow & Volumetric" / "Atmosphere & sky" 等 17char label が完全表示)
3. **Skin SSS タブ動作**:
   - Enabled checkbox トグルで SSS shader 経路 on/off
   - Blur radius / Strength / Glow gain slider live apply
   - Glow color swatch クリックで color picker
   - Whitelist lock checkbox で text_editor の `enabled` が切替
   - Reset All button で whitelist が DefaultWhitelist 文字列に戻る
   - 各 D button で個別 cvar が default に戻る
4. **Preferences 側消失**: 環境設定 → グラフィック を開き、SSS タブが存在しないこと
5. **7 skin 切替で起動異常なし** (sanity 1 skin 確認で代用可)

---

## 7. リスク / 不確定事項

| リスク | 対策 |
|---|---|
| `findChild<LLTextEditor>` recursive lookup が tab 切替前に呼ばれて nullptr | `aya_cinematic` floater 自体は常駐 (Alt+C で open/close)、tab panel は floater 内蔵で初期化済 → 正常系では発生しないが防御コード入り |
| ja override で寸法 ズレ | ja は en の widget 寸法を inherit する設計、label 翻訳のみ override |
| 旧 Preferences > SSS への bookmark を持つユーザー | release note で移行先を明示 |

---

## 8. 後続作業候補 (本 spec 範囲外)

- 他 floater で同様の tab 多数化が出た場合の左縦並び化適用判断 (Preferences pattern として標準化検討)
- AYAstorm Controls 内 tab 順序の再検討 (機能カテゴリ別グルーピング)
