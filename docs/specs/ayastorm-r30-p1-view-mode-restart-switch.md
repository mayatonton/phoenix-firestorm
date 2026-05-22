# AYAstorm r30 P1 — View Mode 再起動切替統一 + Cinematic 枠先行追加

**作成日**: 2026-05-17
**親 spec**: `ayastorm-r30-cinematic-chapter.md` §3 P1
**想定 release**: r30
**スコープ**: View Mode 切替 (Firestorm View / AYAstorm View / Cinematic) を 3 モード再起動切替に統一、Cinematic 枠を View Mode UI に先行追加 (動作は AYAstorm View 相当、shader 取り込み無し)

## 1. 設計判断

### 1.1 cvar 名は据え置き

`AYAVisualRealismEnabled` (U32, default 1) を rename せず enum 拡張のみで対応。

- **理由**: rename すると migration code が必要、既存 0/1 値の引き継ぎリスクと release notes 整理コストに見合わない
- **再解釈**: 「リアリズム enable level」(0=なし, 1=AYAstorm, 2=Cinematic) と意味再定義
- **Comment / Tooltip 更新**: 3 モード対応の説明文に書き換え

### 1.2 再起動切替の実装方式

`AYAChatWindowStyle` の前例 (llviewercontrol.cpp:1764-1797) と同じ流儀:

- combo_box 自体は live-binding (control_name 経由で cvar 即時更新) のまま
- cvar 変更時に listener (signal listener) が modal notification を fire
- 既存の per-frame `LLCachedControl` 読み込みは **据え置き** (= 切替操作直後は live-apply 的に見えるが、ユーザーには「再起動が必要」と明示)
- 再起動後に shader/pipeline が新 View Mode で起動

**重要**: P1 では「per-frame gate を startup snapshot に置換」までは行わない。これは将来 (P2 以降 Cinematic 専用 path 追加時 or 必要が出た時) に検討。P1 のゴールは「ユーザー視点で再起動が必要な切替に揃え、Cinematic 枠を見せる」まで。

### 1.3 Cinematic 選択時の動作 (P1 時点)

- View Mode 選択で Cinematic (value=2) を選んでも、絵作りは AYAstorm View (value=1) と同等
- P1 では Cinematic 専用の shader / pipeline は無い
- Tooltip / Combo box label に **`(preview)`** 表記を入れて、絵が変わらないことを明示
- P2 で velocity buffer 取り込みと同時に Cinematic 専用 path を有効化

## 2. 改修ファイル一覧

| File | 変更内容 |
|---|---|
| `indra/newview/app_settings/settings.xml` (line 9925-9938) | `AYAVisualRealismEnabled` の Comment 更新、enum 説明を 0/1/2 に拡張 |
| `indra/newview/skins/default/xui/en/panel_preferences_graphics1.xml` (line 305-337) | combo_box `AYAViewMode` に Cinematic item 追加 (value=2)、tooltip 更新、`(restart required)` label 追加 |
| `indra/newview/skins/default/xui/en/notifications.xml` | 新規 notification `ChangeViewMode` 追加 (modal alert) |
| `indra/newview/llviewercontrol.cpp` (line 1770 付近、`AYAChatWindowStyle` listener の直後) | `AYAVisualRealismEnabled` signal listener 新規追加、STATE_STARTED guard 付き |

合計 4 ファイル、shader 取り込み 0、C++ ロジック追加は listener 1 個のみ。

## 3. 具体的変更内容

### 3.1 settings.xml

```xml
<!-- 既存 (line 9929-9930) -->
<key>Comment</key>
<string>AYAstorm 視覚的リアリティ章 (r14+) の master switch。0=Firestorm View (r13 までの見え方)、1=AYAstorm View (volumetric atmosphere / godrays / aerial perspective / cloud volumetric を有効化)</string>

<!-- 変更後 -->
<key>Comment</key>
<string>AYAstorm View Mode 切替。0=Firestorm View (上流 Firestorm 標準描画)、1=AYAstorm View (r14+ 視覚表現: volumetric atmosphere / godrays / aerial perspective / Kelvin 色温度 / cloud volumetric / translucency / SSS)、2=Cinematic (r30+ 撮影描画 mode、P1 時点では AYAstorm View 相当、P2 以降で velocity buffer + Volumetric Light + Motion Blur + BD DoF chain を順次有効化)。**変更には viewer 再起動が必要**</string>
```

### 3.2 panel_preferences_graphics1.xml

combo_box item 追加 + tooltip 更新 + restart label。

```xml
<!-- 既存 -->
<text name="aya_view_mode_label">View Mode:</text>
<combo_box
 control_name="AYAVisualRealismEnabled"
 name="AYAViewMode"
 tool_tip="Firestorm View: standard rendering. AYAstorm View: enables volumetric atmosphere, godrays, aerial perspective, and cloud volumetric.">
  <combo_box.item label="Firestorm View" name="0" value="0"/>
  <combo_box.item label="AYAstorm View" name="1" value="1"/>
</combo_box>

<!-- 変更後 -->
<text name="aya_view_mode_label">View Mode (restart required):</text>
<combo_box
 control_name="AYAVisualRealismEnabled"
 name="AYAViewMode"
 tool_tip="Firestorm View: upstream Firestorm standard rendering. AYAstorm View: r14+ visual realism (volumetric atmosphere / godrays / aerial perspective / Kelvin color temperature / cloud volumetric / translucency / SSS). Cinematic (preview): r30+ photo viewer mode; in r30 P1 this renders identically to AYAstorm View, and per-phase upgrades (velocity buffer, Volumetric Light, Motion Blur, BD DoF chain) will be enabled in subsequent releases. **Changing this setting requires a viewer restart.**">
  <combo_box.item label="Firestorm View" name="0" value="0"/>
  <combo_box.item label="AYAstorm View" name="1" value="1"/>
  <combo_box.item label="Cinematic (preview)" name="2" value="2"/>
</combo_box>
```

### 3.3 notifications.xml

`ChangeChatLayoutSetting` (line 1736-1741 付近) と同じ流儀で新規 notification を追加。

```xml
<notification
 icon="alertmodal.tga"
 name="ChangeViewMode"
 type="alertmodal">
The selected View Mode will be applied after restarting [APP_NAME].
</notification>
```

日本語訳 (notifications.xml の ja / ja_JP 同等位置にも追加):
```
選択した View Mode は [APP_NAME] を再起動した後に反映されます。
```

### 3.4 llviewercontrol.cpp

`AYAChatWindowStyle` listener (line 1770) の直後に追加。

```cpp
// <FS:AYAstorm r30 P1> AYAVisualRealismEnabled (View Mode) restart-required.
// Per ayastorm-r30-cinematic-chapter.md, all 3 modes (Firestorm View / AYAstorm View / Cinematic)
// unify on restart-switch to avoid r17 Kelvin-gate maintenance hell and to make the pipeline
// build-once at startup. The combo_box is still control_name-bound (immediate cvar write),
// but we surface the modal "ChangeViewMode" notification so the user knows a restart is
// needed for the new mode to actually take effect in shader/pipeline.
// Guarded by STATE_STARTED to suppress firing during initial settings load on app boot.
setting_setup_signal_listener(gSavedSettings, "AYAVisualRealismEnabled", []() {
    if (LLStartUp::getStartupState() >= STATE_STARTED)
    {
        LLNotificationsUtil::add("ChangeViewMode");
    }
});
// </FS:AYAstorm r30 P1>
```

## 4. 検証手順

P1 ship 前に AYA 実機で以下を順に確認:

### Step 1: cvar / UI 動作確認
1. Preferences → Graphics → General で View Mode combo_box に 3 つの選択肢 (Firestorm View / AYAstorm View / Cinematic (preview)) が表示される
2. Cinematic を選択 → modal notification 「The selected View Mode will be applied after restarting AYAstorm.」が表示される
3. notification を閉じる → cvar `AYAVisualRealismEnabled` の値が 2 になっている (Debug Settings で確認)

### Step 2: 再起動後の動作確認
4. viewer を再起動 → 起動後 Preferences → Graphics で Cinematic が選択された状態を維持
5. 絵作りは AYAstorm View と同等で動作 (godrays / Kelvin / aerial 等が ON、Firestorm View に下がっていない)

### Step 3: regression 確認
6. View Mode を Firestorm View に戻して再起動 → 上流 Firestorm 同等の low-spec 描画
7. View Mode を AYAstorm View に戻して再起動 → r14+ 機能が ON で従前と同じ絵
8. 3 OS (Linux / macOS / Windows) で動作確認

## 5. 撤退条件

- `ChangeViewMode` notification が再起動切替の UX として煩雑すぎる → notification をやめて XUI tooltip の `restart required` 表記のみに簡素化
- AYAstorm View の per-frame gate に regression を出してしまった → P1 着手前のコードに revert、startup snapshot 化を含めた根本書き換えを次 release に持ち越す

## 6. P2 への引き継ぎ

P1 が ship したら、Cinematic (value=2) 選択時に発火する pipeline 分岐ポイントを P2 着手時点で追加する。具体的には:

- velocity buffer 取り込み (BD borrow 9 ファイル)
- `getViewMode()` 相当の startup snapshot helper を C++ に新設 (`gAYAViewMode` static 変数 + `LLAppViewer::init()` での snapshot)
- Cinematic 専用の shader variant を `LLViewerShaderMgr` 起動時に compile

これらは P1 spec の範囲外、P2 着手時に独立 spec で展開する。

## 7. 関連 spec / memory

- `docs/specs/ayastorm-r30-cinematic-chapter.md` — 親 spec、章全体の骨子
- `memory/project_ayastorm_r30_cinematic_chapter.md` — 章 memory 要約
- `memory/feedback_chat_window_style_live_apply_drop.md` — `AYAChatWindowStyle` で live-apply を 2 度試行して断念した経緯、本 spec の流儀 (即 restart 必須) の根拠
- `memory/feedback_combo_box_u32_cvar.md` — combo_box ↔ cvar binding を U32 にする理由
