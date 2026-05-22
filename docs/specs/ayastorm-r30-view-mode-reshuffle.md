# AYAstorm r30 release: View Mode picker reshuffle

**Date:** 2026-05-23
**Branch:** `experiment/r30-cinematic-controls-cleanup`
**Commit:** `6a6b657441` (本作業) — 前段は `fb7b270569`
**Status:** ローカル検証完了、push 待ち

## 1. 背景と動機

r30 章は当初「P1: 3 モード再起動切替統一 + Cinematic 枠先行追加 / P2-P6+: Cinematic 構築」の
順で進行 (`project_ayastorm_r30_cinematic_chapter.md`)。

r30 release 直前段階で AYA さんの所感:

> 前回 release は Firestorm View / AYAstorm View の 2 モードでリリース。今回 Cinematic
> View を実装した狙いは、前回 AYAstorm View に満足できずそれを差し替えるものを作ること
> だった。Cinematic で達成できたと感じており、これを「AYAstorm View」として出荷したい。
> ユーザーから見れば「エンジンが差し替わり新しくよくなった」になり、モードが増えて
> 複雑になった印象を回避できる。

## 2. 採用 approach: UI-only rename + 起動時 migration

### 2.1 不採用 approach

| approach | 理由 |
| --- | --- |
| 内部 mode index の renumber (2→1 化) | 全 GLSL/C++ の `mode==2` 比較が広範、見落とし bug 確実 |
| 旧 mode=1 code path を即削除 | migration window 中の安全側を失う、debug settings 経由 fallback も封じる |
| ファイル名 / クラス名の即時 rename | 影響範囲が広い、別 phase で慎重に判断 |

### 2.2 採用 approach (本作業)

- **UI 層で picker を 2 件表示に縮退**: value=1 を combo_box から削除、value=2 を
  「AYAstorm View」と表示
- **起動時 1-shot migration**: 永続化された `AYAVisualRealismEnabled==1` を `2` に書換、
  `AYAViewModeMigrationVersion` で idempotent gate
- **internal 命名は据置**: `AYACinematicModeActive` / `AYASTORM_CINEMATIC` / `LLCinematicOverlay`
  / `AYAResetCinematic` / `floater_aya_cinematic.xml` (file) / `settings_cinematic_bd.xml` /
  `AYAR##InCinematicEnabled` cvars / pipeline.cpp の `mode == 2` 比較等
- **ユーザー露出文の "Cinematic" を一掃**: floater title / 各種 tooltip / About box の BD
  attribution / picker tooltip

## 3. 変更ファイル一覧

| File | 内容 |
| --- | --- |
| `indra/newview/app_settings/settings.xml` | `AYAVisualRealismEnabled` Comment 更新 / `AYAViewModeMigrationVersion` (S32, default 0, Persist=1) 新設 |
| `indra/newview/llcinematicoverlay.h` | `applyAYAViewModeMigrationIfNeeded()` 宣言 |
| `indra/newview/llcinematicoverlay.cpp` | 同関数の実装 (one-shot、`gSavedSettings.getU32/setU32`) |
| `indra/newview/llappviewer.cpp` | 起動時呼び出し ─ `applyCinematicOverlayIfNeeded()` の **前** に置き、upgrade ユーザーが同一起動で overlay 適用も拾えるよう ordering 確保 |
| `indra/newview/skins/default/xui/en/panel_preferences_graphics1.xml` | combo_box から `value=1` 削除、`value=2` を `label="AYAstorm View"` に変更、tool_tip 書換 |
| `indra/newview/skins/default/xui/ja/panel_preferences_graphics1.xml` | 同 (ja override) |
| `indra/newview/skins/default/xui/en/menu_viewer.xml` | `"Cinematic Controls..."` → `"AYAstorm Controls..."` (Alt+C 維持) |
| `indra/newview/skins/default/xui/en/floater_aya_cinematic.xml` | title rename + 全 `Cinematic preset` → `AYAstorm preset` |
| `indra/newview/skins/default/xui/ja/floater_aya_cinematic.xml` | title rename + `新しい描画エンジン推奨値` / `Cinematic では…` 系を `AYAstorm 推奨値` に統一 |
| `indra/newview/skins/default/xui/en/floater_about.xml` | BD attribution `"AYAstorm Cinematic mode"` → `"AYAstorm View"` |

## 4. Migration の意味論

```cpp
// indra/newview/llcinematicoverlay.cpp
void LLCinematicOverlay::applyAYAViewModeMigrationIfNeeded()
{
    const S32 ver = gSavedSettings.getS32("AYAViewModeMigrationVersion");
    if (ver >= 1) return;                                  // idempotent gate

    const U32 mode = gSavedSettings.getU32("AYAVisualRealismEnabled");
    if (mode == 1) {
        gSavedSettings.setU32("AYAVisualRealismEnabled", 2);
        LL_INFOS("CinematicOverlay") << "...migrated 1 -> 2..." << LL_ENDL;
    }
    gSavedSettings.setS32("AYAViewModeMigrationVersion", 1);
}
```

呼び出し順 (llappviewer.cpp):

```
loadSettingsFromDirectory("User");
applyAYAViewModeMigrationIfNeeded();   // ← この順序が重要
applyCinematicOverlayIfNeeded();       // 上の migration の結果 mode=2 になっていれば overlay も即適用
applyR20SSSMigrationIfNeeded();
```

**不可逆性**: 旧 mode=1 に戻すには debug settings で `AYAViewModeMigrationVersion=0` +
`AYAVisualRealismEnabled=1` の両方を書き戻す必要がある。次回起動時に migration が
再走して 1→2 に再変換する。これは「engine 差替で戻れない」演出の意図通り。

## 5. 検証結果 (2026-05-23 ローカル)

| # | 項目 | 結果 |
| --- | --- | --- |
| 1 | picker 2 件表示 (Firestorm View / AYAstorm View) | OK |
| 2 | `AYAVisualRealismEnabled` が 1→2 に migration (ログ確認) | OK |
| 3 | メニュー項目 "AYAstorm Controls..." | OK |
| 4 | floater タイトル "AYAstorm Controls" | OK |
| 5 | 新エンジン描画 (DoF / Motion Blur / SMAA T2x 等) | OK |

ログ証跡:
```
INFO #CinematicOverlay# llcinematicoverlay.cpp(168) applyAYAViewModeMigrationIfNeeded :
View mode migration v0->v1: AYAVisualRealismEnabled 1 (legacy AYAstorm View) -> 2 (new AYAstorm View)
```

## 6. 残留 (今後の作業候補)

本 commit には **含めず**、別 phase で判断する項目。

### 6.1 旧 mode==1 code path の剪定

UI 不到達になったが debug settings 経由で動く状態のまま残置。

該当箇所例:
- `indra/newview/pipeline.cpp` の `realism_enabled()` 系比較 (`mode > 0` / `mode == 1` 分岐)
- `indra/newview/llsettingsvo.cpp` の AYAR14-r20 個別効果の mode 1 経路
- shader 側 `AYASTORM_AYASTORM_VIEW` mount (各 r14-r20 効果が mode=1 で常時 ON だった経路)
- `AYAR##InCinematicEnabled` 系 cvar の存在意義: mode=1 向け既存 `AYAR##Enabled` (master)
  と分けて mode=2 個別 opt-in 用に置いた cvar。mode=1 が消える前提に立つなら統合可能。

判断ポイント:
- 旧 AYAstorm View の挙動を「再現可能な歴史」として残すか、完全に捨てるか
- 残すなら debug settings 経由 access は維持、消すならその経路も封じる
- 統合タイミング: 次の major release (r31+) か、r30 release 後に safe 期間を置いてから

### 6.2 internal 命名 rename

ユーザーには見えないが、開発者視点で「Cinematic = AYAstorm View」の対応関係が暗黙知に
なるため、長期的には以下の rename 候補:

| 旧 | 新候補 | 影響範囲 |
| --- | --- | --- |
| `AYACinematicModeActive` (cvar) | `AYAViewModeActive` 等 | XUI `enabled_control` 多数 (要 grep) |
| `AYASTORM_CINEMATIC` (#define) | `AYASTORM_VIEW` | shader / C++ 両方、preprocessor |
| `LLCinematicOverlay` (namespace) | `LLAYAViewModeOverlay` 等 | header + 呼び出し全数 |
| `AYAResetCinematic` (commit callback function) | `AYAResetToPreset` 等 | XUI parameter 多数、C++ 関数本体 |
| `floater_aya_cinematic.xml` (file) | `floater_aya_controls.xml` | floater registry / GUI test / git mv |
| `settings_cinematic_bd.xml` (overlay file) | `settings_aya_view_preset.xml` 等 | llcinematicoverlay.cpp の読み込みパス |
| `AYAR##InCinematicEnabled` cvars | (6.1 の統合と同時に判断) | 各 r14-r19 effect |

これらは「やってもユーザーには変化ゼロ、内部一貫性のためだけのリファクタ」なので、
別 phase で着手判断。

### 6.3 docs / 過去 release note の更新

歴史記録として **据置** (`feedback_build_only_verified.md` / 後付けで rename しない):
- `docs/specs/ayastorm-r30-cinematic-chapter.md` 等
- `docs/release/ayastorm-r30-*.md` 各言語

本ドキュメント (`ayastorm-r30-view-mode-reshuffle.md`) を **新しい単一の真実** として
扱い、過去 spec は当時の議論記録として参照する。

### 6.4 r30 release note の差分追記

r30 release note (`docs/release/ayastorm-r30-release-note.{en,ja,zh}.md`) には:
- 「旧 AYAstorm View ユーザーは自動で新 AYAstorm View に切替わる」
- 「旧 AYAstorm View に戻す常用手段はない (意図通り)」
- 「内部に legacy code path は残置、debug settings で復活可能」

の 3 点を追記要。release note の更新は本 commit 後に別 commit で。

## 7. 参照

- `project_ayastorm_r30_cinematic_chapter.md` (memory) — 当初の章構成
- `feedback_release_flow.md` (memory) — push は AYA さん側で実行
- `docs/specs/ayastorm-r30-cinematic-controls-cleanup.md` — 直前の cleanup phase
- `docs/specs/ayastorm-r30-p1-view-mode-restart-switch.md` — 3 モード再起動切替の起源
- commit `6a6b657441` — 本作業
