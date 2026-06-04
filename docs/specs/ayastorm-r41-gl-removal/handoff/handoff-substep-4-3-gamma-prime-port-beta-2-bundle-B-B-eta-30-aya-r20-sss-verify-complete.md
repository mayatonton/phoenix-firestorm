# Sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 — (Z) AYAstorm r20 SSS 効き verify **complete** marker

> **状態 (2026-06-04 session 末)**: (Z) sub-step **全完結** (ZA → ZB → ZC → ZD → ZN)。AYAstorm r20 Avatar Skin SSS は構造的に正常動作、AYA 主観「SSS 効かなくなった気がする」の operational 原因 = `AYAR20AvatarSkinSSSWhitelist` 空 (= Phase C per-pixel mask gate 下で全 mesh が `isSSSTarget()==false`)。AYA 実機 live A/B で whitelist 追加 → SSS 視認 PASS、`settings.xml` line 10184 Comment spec lag 修正 commit landed on `fix/r20-sss-spec-comment` branch (= `ayastorm-release` 起点)。本 doc は (Z) ZN literal = "(Z) sub-step Exit + handoff complete marker"。

---

## §0 状態 summary

| 観点 | state |
|---|---|
| (Z) sub-step | **complete** (ZA/ZB/ZC/ZD/ZN 全終了) |
| 確定 fact | SSS 不動作の原因は **whitelist empty + Phase C 実装** の構造的整合、code regression 無し |
| 修正 commit | `4dde489ec4` on **`fix/r20-sss-spec-comment`** branch (`ayastorm-release` `214054a5a7` 起点) |
| 修正内容 | `indra/newview/app_settings/settings.xml` line 10184 `AYAR20AvatarSkinSSSWhitelist` Comment 1 string 修正 (`Phase A 全画面 SSS` → `Phase C per-pixel mask reality`) |
| code/shader 改変 | **無し** (純粋に doc 文言修正) |
| AYA live A/B | PASS (= AYA literal「すみません SSS のList登録がされてなかったんですね　ちゃんと動きました」2026-06-04) |
| push / PR | **pending** (AYA 側で `git push -u origin fix/r20-sss-spec-comment` + `gh pr create --base ayastorm-release` 実施想定、= feedback_release_flow 遵守) |
| r41 branch 影響 | **無し** ((Z) は r41 独立 verify、Phase 1.B host-side 改変由来でない構造判定済) |

---

## §1 必読 (3 件)

feedback_handoff_minimal_pre_req_read 順守 = 全件 Read 禁止、必要時のみ pinpoint Read に切替

1. **本 handoff doc 全文** (= (Z) sub-step 完結 marker、ZA-ZD 結果 + 修正 commit hash + push/PR pending 状態)
2. **(Z) prep handoff doc** = `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-aya-r20-sss-verify-prep.md` (= commit `5bc170579f` で起案、ZA/ZB/ZC/ZD/ZN sub-task 構成 + AYA 判断要件 (a)(b)(c) record)
3. **r41 milestone Phase 1.B complete marker** = `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-complete.md` (= commit `35c4be1046` で起案、(Z) sub-step を Phase 1.B complete §2.1 で別 sub-step として独立 flag した経緯)

### §1.2 pinpoint reference (= 必要時のみ)

- `indra/newview/app_settings/settings.xml` line 10181-10191 = 修正対象 `AYAR20AvatarSkinSSSWhitelist` cvar 定義
- `indra/newview/llayaskinsss.cpp` line 101-106 = `SkinSSSMatcher::matches()` empty whitelist gate
- `indra/newview/llvovolume.cpp` line 5843 = `LLDrawInfo::mIsSSSTarget = vobj->isSSSTarget()` per-draw stash
- `indra/newview/app_settings/shaders/class1/deferred/skinSSSF.glsl` line 237-239 = Pass 2 `skin_bit` alpha gate

---

## §2 ZA → ZB → ZC → ZD → ZN 全 sub-task record

### §2.1 ZA (debug settings 現値確認、Claude 単独)

- 対象: `~/.ayastorm_x64/user_settings/settings.xml` 内 SSS-related cvar override 値
- 結果:
  - `AYAVisualRealismEnabled` override 無し → migration v1 適用済 default = **2** (= AYAstorm View)
  - `AYAR20AvatarSkinSSSEnabled` override = **1** (= master ON、default 0 OFF からの override、`AYAR20SSSMigrationVersion=1` で旧 `InCinematicEnabled` OR 合成済)
  - `AYAR20AvatarSkinSSSBlurRadius` / `Strength` / `GlowColor` / **`Whitelist`** override 全て無し = default 値 (1.0 / 0.5 / red / **empty**)
- finding: **master ON 確定 + Whitelist 空確定**

### §2.2 ZB (shader 経路 trace、Claude + Explore agent 9 file cross-reference)

- 対象 file 9 件: `llayaskinsss.cpp/h` + `pipeline.cpp` + `skinSSSF.glsl` + `materialF.glsl` + `avatarF.glsl` + `pbropaqueF.glsl` + `llshadermgr.cpp/h` + `llvovolume.cpp`
- 確定 chain 6 段:
  1. `SkinSSSMatcher::matches()` empty whitelist で即 `return false` (`llayaskinsss.cpp:101-106`)
  2. `LLViewerObject::isSSSTarget()` 全 object false
  3. `LLDrawInfo::mIsSSSTarget=false` for all draws (`llvovolume.cpp:5843`)
  4. per-draw uniform `aya_sss_skin_flag=0.0` 全 draw push
  5. gbuffer3 (emissive) `.a=0.0` 全 pixel (`materialF.glsl:604` / `avatarF.glsl:129` / `pbropaqueF.glsl:271`)
  6. `skinSSSF.glsl:237-239` `skin_mask=0` → `skin_bit=0` → `frag_color.a = aya_strength × 0 = 0` → Pass 2 `SRC_ALPHA/ONE_MINUS_SRC_ALPHA` blend で `dst×1=dst` = **SSS 完全に無変化**
- finding: **whitelist 空 = Phase C 実装下で SSS 完全 OFF が構造的に確定**

### §2.3 ZD (spec ↔ 実装 ↔ 実機 3 軸 cross-check、Claude)

- (i) **spec doc** `docs/specs/ayastorm-r20-avatar-skin-sss.md` = Phase A/B/C/D/E architectural phase 言及無し = 修正不要
- (ii) **実装** (`llayaskinsss.cpp` + `skinSSSF.glsl` + 4 gbuffer writer) = Phase C per-pixel mask 完備、正常動作
- (iii) **`settings.xml` line 10184 Comment が stale** = literal「Phase B/E 装着時照合済み、Phase A (本 build) の rendering は全画面 SSS のまま で Phase C で per-pixel mask に切替予定」だが実装は既に Phase C
- 分岐判定: (Z) prep §2.3 (iii)「spec ↔ 実装 diff あり」のうち **spec lag のみ**で確定 (= code regression 無し)

### §2.4 ZC (AYA 実機 live A/B、AYA + Claude)

- 手順: 自 avatar 肌 mesh 右クリック → Add to SSS whitelist → 数秒待機 → live cvar refresh → SSS 視認
- AYA literal「すみません SSS のList登録がされてなかったんですね　ちゃんと動きました」(2026-06-04)
- finding: **(Z) prep §2.3 分岐 (iii) 確定、AYA 主観錯覚でなく operational gap (= whitelist 未登録) が原因**

### §2.5 ZN (Exit + handoff complete marker、Claude = 本 doc)

- (i) `fix/r20-sss-spec-comment` branch 切出し (= AYA 選択 (β-1) `ayastorm-release` 起点 + (β-3) (i) 結果待ち)
- (ii) `settings.xml` line 10184 Comment edit (1 line +/-)
- (iii) commit `4dde489ec4` (= 詳細 message で ZA-ZD trace + live A/B PASS 全記録、`Co-Authored-By: Claude` 不在)
- (iv) 本 (Z) complete handoff doc 起案 = ZN literal 充足
- push / PR は AYA 側で実施 (feedback_release_flow 順守)

---

## §3 修正 commit 詳細

- branch: **`fix/r20-sss-spec-comment`**
- 起点: `ayastorm-release` HEAD `214054a5a7` (= "Merge pull request #126 from mayatonton/docs/r31-bugfix-2-3dstream-rename")
- commit hash: **`4dde489ec4`**
- diff: `1 file changed, 1 insertion(+), 1 deletion(-)`
- 改変 file: `indra/newview/app_settings/settings.xml` line 10184 Comment 1 string のみ
- C++ 改変: **無し**
- GLSL/shader 改変: **無し**
- cvar 型/default/Persist 不変 (= 既存挙動 1 bit も変化させない)
- `Co-Authored-By: Claude` **不在** (= feedback_no_claude_coauthor 順守)

### §3.1 修正前 (line 10184)

```
r20 SSS 対象 mesh asset UUID whitelist。改行区切り、各行が 1 件の mesh UUID (36 文字 dash 区切り、完全一致)。装着物を右クリック > Add to SSS whitelist で自動追加されるのが基本ルート。手書きで paste も可。空行/無効な行は無視。Phase B/E 装着時照合済み、Phase A (本 build) の rendering は全画面 SSS のままで Phase C で per-pixel mask に切替予定
```

### §3.2 修正後

```
r20 SSS 対象 mesh asset UUID whitelist。改行区切り、各行が 1 件の mesh UUID (36 文字 dash 区切り、完全一致)。装着物を右クリック > Add to SSS whitelist で自動追加されるのが基本ルート。手書きで paste も可。空行/無効な行は無視。Phase C (per-pixel mask gate) 実装済 — 本 whitelist が空のとき SSS は全 mesh で発火しない (= isSSSTarget()==false → gbuffer3.a=0 → skinSSSF.glsl skin_bit=0 → Pass 2 alpha=0)。AYAR20AvatarSkinSSSEnabled=1 master ON にしても効果を見るには対象装着物を右クリック > Add to SSS whitelist で追加必須
```

---

## §4 push / PR pending (AYA 側)

| step | command | 備考 |
|---|---|---|
| push | `git push -u origin fix/r20-sss-spec-comment` | feedback_release_flow 順守、AYA 実施 |
| PR | `gh pr create --base ayastorm-release --head fix/r20-sss-spec-comment --title "r20: SSS Whitelist Comment Phase A → Phase C reality 反映"` | base = `ayastorm-release` |
| merge | AYA 判断 (= squash or merge commit) | r31-bugfix-2 等の chain と無干渉 (= settings.xml line 10184 のみ) |

---

## §5 残 milestone strict 線形 + AYA 判断要件

### §5.1 r41 milestone state (= feature/ayastorm-r41-gl-removal branch)

- **Phase 1.A** ✅ 全完了 (commit `fe2f3a81c6` で章クローズ済、PA-B + PA-N complete marker = (X) 候補完了)
- **Phase 1.B** ✅ 全完了 (commit `35c4be1046` 含む 14 物理 + 15 handoff commit chain)
- **(Z) AYAstorm r20 SSS verify** ✅ 全完了 (= 本 doc + commit `4dde489ec4` on `fix/r20-sss-spec-comment`)
- **Phase 1.C** ⏳ 未着手 (= 候補 (Y))
- **上流 uniform4iv 内 glUniform1iv bug fix** ⏳ 未着手 (= 候補 (W)、prep doc commit `33f983c272`)

### §5.2 残候補 (W)(Y) AYA 判断要件

| 候補 | 内容 | prep doc commit | 並列性 |
|---|---|---|---|
| (W) | 上流 `LLGLSLShader::uniform4iv` 内 `glUniform1iv` bug fix (= line 2558、3/4 truncation、dead code 状態) | `33f983c272` | (Y) と独立、並列可 |
| (Y) | Phase 1.C 着手 (= test UBO shell + per-cadence update + descriptor bind) | 未起案 (= 本 sub-step 内に prep 未作成) | (X) 完了で unblocked |

### §5.3 Claude 推奨分岐

- (W) を **(a) AYAstorm fork 内 fix + (b) upstream LL PR 両方並行** ((W) prep §2.3 で Claude 推奨済、根拠 3 件 = bug literal 確実 + fix scope 1 line trivial + 上流取込やすさ維持原則と整合)
- (Y) Phase 1.C の prep doc 起案 = (W) と並列可能

---

## §6 self-verify (9 観点)

- (1) ZA 現値確認 完了 (= AYAR20AvatarSkinSSSEnabled=1 + Whitelist override 無 = 空) ✅
- (2) ZB shader trace 確定 chain 6 段 ✅
- (3) ZC AYA live A/B PASS literal 記録 ✅
- (4) ZD 3 軸 (i)(ii)(iii) 結果 ✅
- (5) ZN Exit + handoff doc 起案 = literal 充足 ✅
- (6) fix scope comment-only (= +1/-1 line、code/shader 改変無し) ✅
- (7) feedback_release_branch_workflow 順守 (= `ayastorm-release` から feature branch 切出し、release branch 直 commit せず) ✅
- (8) feedback_no_scope_shrink 順守 (= (Z) literal scope ZA/ZB/ZC/ZD/ZN 全 sub-task 全扱う、ZN は別 session でなく本 commit に同梱完結) ✅
- (9) `Co-Authored-By: Claude` 不在 ✅

---

## §7 引き継ぎ memory (12 件)

- `project_ayastorm_r41_vulkan_migration` (= r41 milestone active state)
- `project_ayastorm_r41_design_principles` (= 上流取込やすさ + Core プロセス分散)
- `project_r41_phase1b_vulkan_host_gate` (= GATE-B + MUSEUBO-A 確定)
- `feedback_doubt_self_first` (= AYA SSS 報告を尊重、構造的に自分の改変を疑い check)
- `feedback_visual_decisions_need_live_ab` (= ZC live A/B 実施根拠)
- `feedback_no_scope_shrink` (= (Z) literal scope 全 sub-task 扱い)
- `feedback_release_branch_workflow` (= `ayastorm-release` から feature branch 切出し)
- `feedback_handoff_minimal_pre_req_read` (= 必読 3 件 cap)
- `feedback_self_verify_before_handoff` (= 9 観点 self-verify)
- `feedback_proactive_handoff` (= context 残量監視で本 session 末 handoff 起案)
- `feedback_no_auto_commit` (= AYA「Claude 推奨」明示指示後 commit)
- `feedback_no_claude_coauthor` (= Co-Authored-By: Claude 不在)

---

## §8 次 session 着手 1 line

> 次 session = AYA 「(W) 進めて」or 「(Y) prep 起案して」or 「push + PR 進めて」literal 受領待ち。残候補 = (W) 上流 uniform4iv bug fix (prep `33f983c272`) + (Y) Phase 1.C 着手 (prep 未起案)。push/PR は AYA 側で `git push -u origin fix/r20-sss-spec-comment` + `gh pr create --base ayastorm-release` 実施想定。
