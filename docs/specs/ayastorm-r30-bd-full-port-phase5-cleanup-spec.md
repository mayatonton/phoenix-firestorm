# AYAstorm r30 BD full port — Phase 5 cleanup / release prep spec

**Phase 名**: r30 BD 完全移植 Phase 5 — 出荷前 cleanup + release prep
**前提**: Phase 4 (3 mode 受入検証 G1-G5 all green) 通過
**作成日**: 2026-05-19
**ブランチ**: `feature/ayastorm-r30-bd-full-port-inventory`
**章方針**: `feedback_remove_verification_logs.md` / `feedback_restore_debug_settings.md` / `feedback_release_notes_link_only.md` / `feedback_release_note_per_feature.md` / `feedback_no_dual_doc_split.md`

---

## §0 目的 / 非目的

### 目的
Phase 4 通過後、Phase 3.7-3.9 で持ち込んだ検証用 hook / 動作確認 cvar / 中間 attribution を出荷物として整える。最終 commit は `feature/ayastorm-r30-bd-full-port-inventory` の release-ready 状態。

### 非目的
- 追加機能 / 追加 borrow (`feedback_bd_full_port_only.md`)
- 過剰 refactor (Phase 4 で動いている path を beautify しない)
- BD attribution comment の削除 (LGPL 互換維持、`AYA` 注釈は残す)

---

## §1 cleanup チェックリスト

### §1.1 検証用 LL_INFOS / hook の削除 (`feedback_remove_verification_logs.md`)

`git diff ayastorm-release..HEAD -- 'indra/newview/*.cpp' 'indra/newview/*.h'` 全 BD-port 差分の log 追加行を確認:

- [x] `pipeline.cpp` Phase 3.7 dispatch 追加箇所 — **追加 log なし** (dispatch は silent)
- [x] `llviewershadermgr.cpp` Phase 3.8 cinematic_bd path-probe — **追加 log なし** (path-probe は silent)
- [x] `bdsidebar.cpp` mount 時 log — **BD verbatim** (`LL_WARNS("Sidebar")` 3 件は BD 由来、保持)
- [x] `bdfunctions.cpp` update checker `LL_INFOS() << "HTTP Code"` — **BD verbatim、保持**
- [x] `llviewerwindow.cpp` Phase 3.9 step 4a mount caller — **追加 log なし**
- [x] `pipeline.cpp` r30 P2 `LL_INFOS("Pipeline") << "AYAstorm r30 P2: allocated mVelocityMap..."` — **one-shot alloc 報告、保持** (per-frame でなく起動 1 回、debug 用ではなく構成記録)

確定: Phase 5 §1.1 は **追加 cleanup 不要**。BD port branch で追加された verification debug log は無し。判定方法は `git diff ayastorm-release..HEAD -- '*.cpp' '*.h' | grep -E '^\+.*(LL_INFOS|LL_WARNS|LL_DEBUGS)'` で全 hit を BD verbatim / 既存 r30 P2 にカテゴライズ済 (2026-05-19 audit)。

### §1.2 検証用 debug settings 整理 (`feedback_restore_debug_settings.md`)

AYA Phase 4 検証で触った cvar の default 戻し表を release note に同梱:

| cvar | 検証で触った値 | release 後の推奨値 | persist |
|---|---|---|---|
| `AYAVisualRealismEnabled` | 0 / 1 / 2 | **1** (AYAstorm View 既定) | 1 |
| `MachinimaSidebar` | 1 (default) | 1 | 0 |
| `RenderShadowAutomaticDistance` | (default 1) | 1 | 1 |
| `RenderShadowResolution` | (BD-only 配列) | 4×1024.0 | 1 |
| `RenderShadowDistance` | (BD-only 配列) | [12, 24, 48, 96] | 1 |

実際の値は AYA 検証結果を反映、Phase 5 step 3 で確定。

### §1.3 cinematic_bd/ license / attribution 確認

Phase 3.8 step 4 で `indra/newview/app_settings/shaders/cinematic_bd/class{1,3}/` 配下に配置した BD 由来 shader (shadowUtil / screenSpaceReflUtil) の冒頭に BD 由来注釈 + LGPL header があるか確認:

- [ ] `cinematic_bd/class1/deferred/shadowUtil.glsl` 冒頭に BD attribution
- [ ] `cinematic_bd/class3/deferred/screenSpaceReflUtil.glsl` 冒頭に同上
- [ ] 配置物が strategy D 確定 2 件以外混入していないか確認 (`ls` で件数 verify)

### §1.4 inventory 漏れ port 物の出所トレース確認

Phase 3.9 で inventory 漏れとして port した:

- `bdsidebar.{cpp,h}`
- `bdfunctions.{cpp,h}` (Phase 3.9 後段)
- 4 caller patch (`llviewerwindow.{cpp,h}` / `llviewercontrol.cpp` / `llfloaterpreference.cpp` / `llagent.cpp` / `llagent.h`)
- 6 LLControlVariable / LLControlGroup / LLEnvironment / LLComboBox / LLViewerRegion API 拡張

各 file 冒頭の provenance header (`AYAstorm: imported from BlackDragon Viewer (NiranV Dean), 995a1354d8, 2026-04-19`) が残っているか確認。

### §1.5 build sanity (no-op)

- [ ] `autobuild configure -A 64 -c ReleaseFS_open -- --fmodstudio -DLL_TESTS:BOOL=FALSE -DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE --package --chan AYAstorm-release`
- [ ] `autobuild build -A 64 -c ReleaseFS_open --no-configure --fmodstudio` green
- [ ] install + cache clear + 起動確認 (mode 1 default 起動のみ、3 mode 再走しない)

---

## §2 release note 草案 (`feedback_release_note_per_feature.md` / `feedback_release_notes_link_only.md`)

### §2.1 release ヘッダ

```
AYAstorm r30: BlackDragon Viewer 完全移植 (3 mode 統合)
=========================================================

AYAstorm に Cinematic mode を追加し、BlackDragon Viewer (BD) 995a1354d8 の
描画 pipeline 全体を 1:1 移植して並走できるようにしました。
```

### §2.2 主要 feature (1 feature 1 note)

| feature | 内容 | 参照 spec |
|---|---|---|
| 3 mode 統合 | `AYAVisualRealismEnabled` で Firestorm View(0) / AYAstorm View(1) / Cinematic(2) を再起動切替 | `docs/specs/ayastorm-r30-bd-full-port-phase2-spec.md` (D1) |
| BD shader 49 file mount | shader 49 file を A/B/C/D strategy で mode 2 に mount | `docs/specs/ayastorm-r30-bd-full-port-phase3.8-shader-cinematic-mount-spec.md` |
| BD UI mount (bdsidebar) | mode 2 で右側に Machinima Sidebar 出現 (BD と同 panel_machinima) | `docs/specs/ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md` |
| BD C++ dispatch | pipeline / drawpool / shadermgr 等 53 file の mode 別 dispatch | `docs/specs/ayastorm-r30-bd-full-port-phase3.2-cpp-dispatch-spec.md` |

### §2.3 注意事項 (β release)

- mode 切替は **再起動推奨** (round-trip 動作未保証、Phase 4 G4)
- bdsidebar 内 slider/button 操作は AY 拡張 cvar と一部衝突する場合あり (AY 拡張は mode 1 で確認)
- cinematic_bd/ 経路は GPU class 別 fallback (class3→class2→class1) を継承

### §2.4 known issues / 持ち越し

- BD `panel_preferences_render_settings` / `panel_preferences_ui_colors` は orphan (Phase 3.9 §3.3 判断)
- BD `llfloatereditsky` / `llfloatereditwater` は BD 上流が register していないため AY でも orphan

---

## §3 commit unit

| step | 単位 | 例 |
|---|---|---|
| 1 (log 除去) | per-file or 1 bulk | `r30 BD full port Phase 5 step 1: 検証用 LL_INFOS 除去` |
| 2 (license 確認) | spec 更新のみ commit 不要 (✅ 印付け) | (spec re-commit 1 回で済む) |
| 3 (debug 戻し表確定) | spec re-commit 1 回 | `r30 BD full port Phase 5 step 3: debug settings restore 表確定` |
| 4 (release note draft) | docs/release/ 配下 1 file | `r30 BD full port Phase 5 step 4: release note draft (r30)` |
| 5 (build sanity) | (no commit) | log のみ spec 更新 |
| FINAL | 1 行 | `r30 BD full port Phase 5: cleanup 完了 (r30 release ready)` |

---

## §4 punt 禁止項目 (自検)

`feedback_bd_full_port_only.md` / `feedback_no_escape_full_bd_coverage.md`:

- ❌ 「release note は GitHub 上で書けばいい」punt → **本 spec に同梱、docs/release/ 配下に永続化** (`feedback_no_dual_doc_split.md`)
- ❌ 「LL_INFOS 残しても無害」推論 → **`feedback_remove_verification_logs.md` 準拠で除去**
- ❌ 「BD orphan は移植しない」punt → **既に Phase 3.9 §4 で「BD 1:1 = orphan 状態も保つ」確定済、本 spec で再確認**

---

## §5 完了基準

- §1 cleanup チェックリスト全 ✅
- §2 release note draft が `docs/release/ayastorm-r30-release-note.md` に landed
- AYA から push / tag / β release への hand-off 可能状態 (`feedback_release_flow.md`)
- 最終 commit: `r30 BD full port Phase 5: cleanup 完了 (r30 release ready)`

---
