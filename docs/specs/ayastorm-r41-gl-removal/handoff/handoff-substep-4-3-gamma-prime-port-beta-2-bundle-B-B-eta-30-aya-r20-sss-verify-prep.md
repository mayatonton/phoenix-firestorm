# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 AYAstorm r20 章 SSS 効き verify prep (= candidate (Z))

**作成日**: 2026-06-04
**前 commit chain** (= 直前 handoff 起案):
- `35c4be1046` = Phase 1.B **complete** marker handoff doc 起案
- `60070d048b` = 候補 (X) Phase 1.A residual prep handoff doc 起案 (= 並行起案 1/3)

**本 handoff doc 目的**: **Phase 1.B complete handoff §2.1 「AYAstorm r20 章 SSS 効き観察 issue」literal 別 sub-step 独立起案 prep**。AYA 主観「SSS が効かなくなった気がする」literal 受領 (= 2026-06-04 PB-N verify 時)、ただし本 r41 Phase 1.B branch 物理改変由来でないと **構造判定済** (= GLSL 改変 0 件 + host-side 改変 `if (mUseUBO)` gate 内 + `mUseUBO=false default`)。AYAstorm r20 章 SSS 機能が **現状効いているか** は別途実機 verify 要件 = **r41 milestone と独立** sub-step として起案。

---

## §0 state 一行 summary

候補 (Z) AYAstorm r20 章 SSS 効き verify = **r41 milestone と独立、AYA 主観 SSS 効き観察由来の独立 sub-step**。

- **起因**: AYA 主観「SSS が効かなくなった気がする」literal (= 2026-06-04 Phase 1.B PB-N verify 時)
- **本 r41 branch 物理改変由来でない構造判定** (= Phase 1.B complete §1.2 + §2.1):
  - (i) AYA 側 Phase 1.B 前後比較根拠なし (= r40 章「60秒計測 2 SLURL」phase で Claude SSS 発言を起点に今意識観察)
  - (ii) Phase 1.B host-side 改変は構造的に SSS 影響不可
  - (iii) 本 branch GLSL 改変 0 件 (= `git log ayastorm-release..HEAD -- indra/newview/app_settings/shaders/` 出力 0 行)
- **残課題**: AYAstorm r20 章 SSS 機能の現状 verify (= debug settings 現値 + shader 経路 + 実機 ON/OFF live A/B + spec doc 照合)
- **scope**: r41 milestone と独立 (= 並行 or 後続 or 別 release sub-phase か AYA 判断要件)

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc | 全文 | (Z) AYAstorm r20 SSS verify prep state + sub-task 構成 + AYA 判断要件 |
| 2 | `docs/specs/ayastorm-r20-avatar-skin-sss.md` | 全文 (= source of truth) | r20 章 SSS 機能 spec literal = debug settings name + shader 経路 + 効き判定基準 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-complete.md` | §1.2 / §2.1 | Phase 1.B host-side 構造的根拠 5 観点 + (Z) literal 内容案 (a)(b)(c)(d) |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `indra/newview/app_settings/shaders/class3/deferred/materialF.glsl` | AYAstorm SSS 経路 (= `aya_sss_skin_flag` keyword grep で経路 trace) |
| `indra/newview/app_settings/shaders/class1/deferred/skinSSSF.glsl` | SSS dedicated shader (= 経路 trace 候補) |
| `indra/newview/app_settings/shaders/class1/deferred/avatarF.glsl` | avatar shader 内 SSS 経路 (= `aya_sss_skin_flag` keyword grep で経路 trace) |
| `indra/newview/app_settings/shaders/class1/deferred/pbropaqueF.glsl` | PBR opaque shader 内 SSS 経路 (= `aya_sss_skin_flag` keyword grep で経路 trace) |
| `indra/newview/app_settings/settings.xml` (= debug settings DB) | `aya_sss_skin_flag` 等 debug settings name pinpoint Read (= persistence / default value / 過去 session で変更残存可能性確認) |
| memory `project_aya_visual_realism_alpha_protect` | r14+ で additive (ONE/ONE) する shader の alpha protection 規約 (= SSS 経路でも `frag_color.a=0` 規約適用可能性) |
| memory `feedback_visual_decisions_need_live_ab` | 視覚表現採否は live A/B 必須 (= sub-task ZC 実機 ON/OFF live A/B 必須根拠) |

---

## §2 sub-task 構成 (= Phase 1.B complete §2.1 内容案 (a)(b)(c)(d) から展開)

### §2.1 sub-task table

| sub-task | scope | 主体 | Exit |
|---|---|---|---|
| **ZA** | `aya_sss_skin_flag` 関連 debug settings の現値確認 | Claude | `settings.xml` 内 `aya_sss_skin_flag` 等 entry pinpoint Read + default value 確認 + AYA 側現値確認 (= 過去 session で Persist=1 cvar 変更残存可能性、`feedback_restore_debug_settings` 観点) |
| **ZB** | `materialF.glsl` + 関連 shader 内 AYAstorm SSS 経路の現コード trace | Claude | `aya_sss_skin_flag` keyword grep 全 shader file 列挙 + 各経路 source code trace + r20 spec doc 期待実装と diff record |
| **ZC** | 実機 ON/OFF live A/B (= `feedback_visual_decisions_need_live_ab` 順守) | AYA + Claude | AYA viewer launch + debug settings ON/OFF cycle + Claude 観察 record (= SSS 効果視覚差分の有無 record) |
| **ZD** | r20 spec doc (= `docs/specs/ayastorm-r20-avatar-skin-sss.md`) との照合 | Claude | spec literal vs 実装 trace (= ZB) + 実機観察 (= ZC) の 3 軸 cross-check + diff があれば fix sub-task 起案 |
| **ZN** | Exit Criteria 確認 + handoff complete marker | Claude | ZA/ZB/ZC/ZD 全 PASS + 9 観点 self-verify + handoff doc 起案 + 1 commit |

### §2.2 strict 線形 (= ZA → ZB → ZC → ZD → ZN)

理由:
- ZA (debug settings 現値) は ZB (shader 経路 trace) 前 = `if (aya_sss_skin_flag) { ... }` のような flag 経路 trace に現値必要
- ZB は ZC (live A/B) 前 = trace 結果に基づき AYA に ON/OFF 期待視覚差分を予測通知
- ZC は ZD (spec 照合) 前 = 実機観察を spec literal と照合する材料
- ZD は ZN 前 = 最終判定で全 record 統合

### §2.3 ZD 後の分岐判定

| 分岐 | 判定 | 次 sub-task |
|---|---|---|
| (i) spec ↔ 実装 ↔ 実機 一致 | SSS 効いている、AYA 主観錯覚 = `feedback_doubt_self_first` 反転事例 (= AYA 提示情報を構造的に否定する判定) | (Z) close、handoff complete marker |
| (ii) spec ↔ 実装 一致、実機 効きが弱い | 実装は spec 準拠、効きが期待より弱い = tuning sub-task or spec 更新候補 | spec 更新 or tuning sub-task 起案 (= AYA 判断) |
| (iii) spec ↔ 実装 diff あり | regression or spec 漏れ = fix sub-task 起案 | fix sub-task 起案、AYA 着手判断 |

---

## §3 残 strict 線形 (= r41 milestone と独立性記録)

### §3.1 r41 milestone との独立性

- 本 (Z) sub-step は **r41 Phase 1.B host-side 改変由来でない** (= Phase 1.B complete §1.2 構造的根拠 5 観点 PASS 済)
- 本 (Z) sub-step は **r41 Phase 1.B branch (= `feature/ayastorm-r41-gl-removal`) で GLSL 0 touch** ゆえ shader 改変は **`ayastorm-release` HEAD 同等**
- **AYAstorm r20 章は別 chapter** (= memory `project_ayastorm_release_chapters` で r14-24 視覚表現章 + r20 = avatar skin SSS sub-chapter)、本 r41 章 (Vulkan migration milestone) と orthogonal

### §3.2 AYA 判断要件

- (a) 本 (Z) sub-step を r41 branch (= `feature/ayastorm-r41-gl-removal`) 内で続行する (= host-side 改変なしの verify-only sub-step として r41 milestone 内に統合)
- (b) 別 branch (= `experiment/r20-sss-verify` 等) を切って独立進行 (= r20 章 sub-phase 拡張、別 release sub-phase 判定)
- (c) `ayastorm-release` HEAD で直接 verify (= r41 branch checkout 不要、main branch で r20 章 verify、本 (Z) sub-step は r41 milestone と完全独立化)

Claude 推奨 = **(c) `ayastorm-release` HEAD 直接 verify**。根拠 3 件:
1. r41 branch の host-side 改変は SSS 影響不可 (= 構造的根拠) ゆえ r41 branch checkout 不要
2. r20 章 SSS は別 chapter (= r14-24 視覚表現章) で r41 章 (Vulkan migration) と orthogonal、混在は scope cross-contamination
3. 本 (Z) verify が GLSL 改変提案に発展した場合、r41 branch (= GLSL 0 touch 維持必要、GATE-B 整合) と矛盾、`ayastorm-release` HEAD or 専用 sub-phase branch が安全

ただし AYA さん r41 milestone 並行進行優先で (a) 採用判断もあり得る。

### §3.3 候補 (X)(Y)(Z)(W) 内 strict 線形 (= 本 (Z) prep 着手後反映)

```
✅ Phase 1.B host-side 完了
  ✅ (X) Phase 1.A residual prep 起案 (= commit 60070d048b)
  ✅ (Z) AYAstorm r20 SSS verify prep 起案 (= 本 doc)
  ⏳ (W) 上流 uniform4iv bug fix prep 起案 (= 並行起案 3/3 next)
  → ⏳ 候補 sub-task 着手 AYA 判断 ((X)(Y)(Z)(W))
```

---

## §4 self-verify (= 本 handoff 起案時点、commit 前確認)

| # | 観点 | 確認方法 | 期待 |
|---|---|---|---|
| (1) (Z) literal scope 独立性 | §0 + §3.1 で r41 milestone と独立性記録 (= 構造的根拠 + 別 chapter 性) | ✅ |
| (2) sub-task 構成 ZA/ZB/ZC/ZD/ZN | §2.1 table + §2.2 strict 線形理由 + §2.3 分岐判定 | ✅ |
| (3) 必読 3 件 + pinpoint reference 別記 | §1.1 3 件 + §1.2 7 reference | ✅ |
| (4) AYA 判断要件 (a)(b)(c) 比較 | §3.2 3 案 + Claude 推奨 (c) 根拠 3 件 | ✅ |
| (5) `feedback_visual_decisions_need_live_ab` 順守 | sub-task ZC literal 「実機 ON/OFF live A/B」 | ✅ |
| (6) `feedback_doubt_self_first` 反転考慮 | ZD 分岐 (i) で「AYA 主観錯覚」判定可能性明記 | ✅ |
| (7) commit 内容 | handoff doc 1 件のみ、indra/ + scripts/ + cmake/ 改変 0 | ✅ (本 commit 段) |
| (8) Co-Authored-By 不在 | commit message 行頭 `Co-Authored-By:` 0 件 | ✅ |
| (9) `feedback_self_bug_no_defer_option` 遵守 | (Z) sub-step 独立起案で「先送り signal」でない (= Phase 1.B complete §0 「別 sub-step として独立 flag」literal 順守) | ✅ |

---

## §5 引き継ぎ memory (= 次 session 着手時参照)

特に重要 (= 既存 memory から):

- `project_ayastorm_release_chapters` (= r14-24 視覚表現章 + r20 = avatar skin SSS sub-chapter 章番号帯判定)
- `project_ayastorm_r14_pivot_to_light` (= r14+ 視覚表現章 core thesis)
- `project_ayastorm_visual_realism_chapter` (= 「写真を撮るに値する空気と空間」)
- `project_aya_visual_realism_alpha_protect` (= r14+ additive shader alpha protection 規約、SSS 経路でも適用可能性)
- `feedback_visual_decisions_need_live_ab` (= ZC live A/B 必須根拠)
- `feedback_restore_debug_settings` (= ZA debug settings 現値確認後、AYA 戻す値表提示)
- `feedback_doubt_self_first` (= ZD 分岐 (i) で反転事例適用可能性)
- `feedback_build_only_verified` (= 実機検証 only、机上推論で SSS 効き判定しない)
- `feedback_handoff_minimal_pre_req_read` (= 次 session pre-req は最小 3 件)
- `feedback_no_auto_commit` (= 本 doc commit は AYA 明示指示後)
- `feedback_no_claude_coauthor` (= Co-Authored-By 行不在)
- `feedback_no_scope_shrink` (= (Z) literal scope 4 sub-task (a)(b)(c)(d) 全扱う、ZC 実機 A/B 省略しない)

---

## §6 次 session 着手 1 line

**「前 session で候補 (Z) AYAstorm r20 章 SSS 効き verify prep handoff doc 起案 + commit (= 本 doc + 1 commit)。本 session = AYA 判断 (a) r41 branch 内続行 / (b) 別 branch 切って独立進行 / (c) `ayastorm-release` HEAD 直接 verify のいずれか確定後、strict 線形 ZA → ZB → ZC → ZD → ZN 着手。必読 3 件 = (1) 本 handoff doc 全文 + (2) `docs/specs/ayastorm-r20-avatar-skin-sss.md` 全文 + (3) Phase 1.B complete handoff §1.2 / §2.1。pinpoint reference = `materialF.glsl` / `skinSSSF.glsl` / `avatarF.glsl` / `pbropaqueF.glsl` SSS 経路 + `settings.xml` debug settings + 関連 memory 3 件。r41 milestone と独立 (= 構造的根拠 5 観点 PASS 済)、AYA 主観「SSS 効かなくなった気がする」literal 由来。」**
