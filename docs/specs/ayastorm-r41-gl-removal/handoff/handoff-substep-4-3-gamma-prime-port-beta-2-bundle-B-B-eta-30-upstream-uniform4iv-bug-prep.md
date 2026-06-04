# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 上流 uniform4iv method 内 glUniform1iv bug fix prep (= candidate (W))

**作成日**: 2026-06-04
**前 commit chain** (= 直前 handoff 起案):
- `35c4be1046` = Phase 1.B **complete** marker handoff doc 起案
- `60070d048b` = 候補 (X) Phase 1.A residual prep handoff doc 起案 (= 並行起案 1/3)
- `5bc170579f` = 候補 (Z) AYAstorm r20 SSS verify prep handoff doc 起案 (= 並行起案 2/3)

**本 handoff doc 目的**: **Phase 1.B complete handoff §2.2 「上流 bug flag 残件」literal 別 PR 案件独立起案 prep**。LL/Phoenix-Firestorm 上流既存 bug 疑い (= `uniform4iv(U32 index, ...)` method 内で `glUniform4iv` の代わりに `glUniform1iv` 呼出) の真位置確定 + dead code 状態判定 + fix sub-task 構成 + AYA 判断要件 (= AYAstorm fork 内 fix or upstream LL PR or 両方並行) 記録。

---

## §0 state 一行 summary

候補 (W) 上流 uniform4iv 内 glUniform1iv bug = **literal bug 確実存在 + call site 0 件で現状 dead code 状態**:

- **Bug 真位置確定** = `indra/llrender/llglslshader.cpp:2558` `glUniform1iv(mUniform[index], count, v);` (= `LLGLSLShader::uniform4iv(U32 index, U32 count, const GLint* v)` method line 2521-2563 内)
- **Phase 1.B complete §2.2 literal「line 2514」訂正** = line 2514 は実際は `uniform1iv(U32 index, ...)` method (line 2480-2519) 内、`glUniform1iv` 呼出は **正常 method 内 call**。真の bug は **line 2558** = `uniform4iv` 整数 index 経路 method 内、本 prep 起案時 source 直接確認で訂正 (= `feedback_doubt_self_first` 反転適用、handoff doc 内容を疑い source code 物理確認)
- **Bug 性質**: method 名 `uniform4iv` = 4 component × count 個の int を upload 想定 (= line 2555 `LLVector4 vec((F32)v[0], (F32)v[1], (F32)v[2], (F32)v[3]);` で 4 component 取り出し)、ただし `glUniform1iv(mUniform[index], count, v)` 呼出 = `count` 個の int しか upload されない = **3 倍多い data が無視される**
- **hashed 経路は正常** = `uniform4iv(const LLStaticHashedString&, ...)` method (line 3042-3079) line 3075 `glUniform4iv(location, count, v);` 正しく実装
- **call site 0 件確認済** = `grep -rn "uniform4iv" indra/ --include="*.cpp" --include="*.h"` 出力 = method 定義 (cpp + h = 4 件) + comment 3 件のみ、外部呼出 0 件 → **現状 dead code 状態、practical impact なし**

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc | 全文 | (W) Bug 真位置 + dead code 状態 + sub-task 構成 + AYA 判断要件 |
| 2 | `indra/llrender/llglslshader.cpp` | line 2521-2563 (= `uniform4iv(U32 index, ...)` 整数 index 経路) + line 3042-3079 (= hashed 経路、正常 reference) | bug 物理位置 + 正常実装 reference |
| 3 | `indra/llrender/llglslshader.h` | line 237 + line 250 (= `uniform4iv` 2 method 宣言) | API 宣言 review |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `indra/llrender/llglslshader.cpp:2480-2519` | `uniform1iv(U32 index, ...)` (= 正常 method、line 2514 `glUniform1iv` 正常 call、Phase 1.B complete §2.2 line 2514 literal 誤認元) |
| `indra/llrender/llglslshader.cpp:3006-3040` | `uniform1iv(const LLStaticHashedString&, ...)` (= 正常 method、hashed 経路 reference) |
| LL upstream `secondlife/viewer` repo `indra/llrender/llglslshader.cpp` 同箇所 | upstream literal bug 状態確認 (= 上流既存 bug 疑い verify) |
| Phoenix-Firestorm 上流 `phoenix-firestorm` repo 同箇所 | Phoenix-Firestorm 上流 literal bug 状態確認 |

---

## §2 sub-task 構成

### §2.1 sub-task table

| sub-task | scope | 主体 | Exit |
|---|---|---|---|
| **WA** | bug fix 1 line 物理改変 | Claude | line 2558 `glUniform1iv(mUniform[index], count, v);` → `glUniform4iv(mUniform[index], count, v);`、+ static verify (= grep 確認 + diff stat 1 line) |
| **WB** | LL upstream + Phoenix-Firestorm 上流 bug 状態 verify (= 別 PR 起案前提確認) | Claude | LL upstream repo + Phoenix-Firestorm 上流 repo 同箇所 grep で `glUniform1iv\|glUniform4iv` 確認、上流 bug 同型ならば LL upstream PR 価値あり |
| **WC** | build verify (= full viewer build EXIT 0 + tar.xz package) | Claude | autobuild build + install + cache clear + viewer launch PASS (= dead code ゆえ動作差分なし、起動 fail 0 件) |
| **WN** | Exit Criteria 確認 + handoff complete marker | Claude | WA + WB + WC 全 PASS + 9 観点 self-verify + handoff doc 起案 + 1 commit |

### §2.2 strict 線形 (= WA → WB → WC → WN)

理由:
- WA (fix) は 1 line trivial、最小 scope で先行
- WB (上流 verify) は WA 後、upstream PR 起案価値判定材料
- WC (build verify) は WA 後、dead code ゆえ動作差分なしを確証
- WN は WA+WB+WC record 統合

### §2.3 AYA 判断要件 (= fix path 選択)

| 案 | 内容 | 工数 | 即時性 |
|---|---|---|---|
| (a) **AYAstorm fork 内 fix** (= release branch に直接 commit) | 本 r41 branch から `ayastorm-release` へ cherry-pick or 別 sub-step 起案 + AYAstorm release note 同梱 | 1 line fix + build + handoff、low | 即時 |
| (b) **upstream LL PR** (= LL `secondlife/viewer` repo PR 提出) | LL fork + commit + PR + review cycle、AYAstorm fork は upstream merge 後に rebase | 1 line fix + LL fork setup + PR、medium | medium-long (= LL review cycle) |
| (c) **両方並行** (= AYAstorm fork 内即時 fix + upstream LL PR 並行起案) | (a)+(b) 並行、AYAstorm 即時恩恵 + upstream 取込で長期保守 | 1 line fix + LL fork + PR、medium | (a) 即時 + (b) medium-long |
| (d) **fix 不要 judgement** (= dead code ゆえ practical impact なし、現状放置) | bug record だけ残す | 0 line | n/a |

**Claude 推奨 (= AYA 判断仰ぐ前提): (c) 両方並行**。根拠 3 件:
1. bug は literal 確実 (= source code 直接確認、4 component data の `glUniform1iv` 呼出 = 3/4 truncation)、call site 0 件は現状 dead code ゆえ実害なしだが将来 call site 追加で顕在化 risk あり
2. fix scope = 1 line trivial、AYAstorm fork 内即時 fix の cost は near-zero
3. upstream LL PR は review cycle に時間かかるが、長期保守観点で upstream merge が望ましい (= 本 r41 milestone での upstream OpenGL 取込やすさ維持原則 `project_ayastorm_r41_design_principles` 整合)

ただし (d) fix 不要 judgement も「先送り signal」でなく **dead code 性質に基づく structural 判断**として並べる (= bug 確認は record 済、call site 0 件で実害なし、fix value vs cost で AYA 判断余地)。`feedback_self_bug_no_defer_option` literal「自作 bug の『先送り/disable』を提案として並べない」は **自作 bug** に対するルールで、本件は **上流既存 bug** ゆえ disable/defer は ruling 外。

### §2.4 (a)(b)(c) 選択時の branch / commit 戦略

| 案 | branch 戦略 | commit 戦略 |
|---|---|---|
| (a) | 本 r41 branch (= `feature/ayastorm-r41-gl-removal`) で fix or `ayastorm-release` で直接 fix (= `feedback_release_branch_workflow` 違反回避で feature branch 切る) | 1 commit、1 line fix + handoff doc |
| (b) | LL `secondlife/viewer` fork branch + PR | LL upstream PR 用 commit (= 上流 commit message 様式に従う、Co-Authored-By 不在) |
| (c) | (a)+(b) 並行、AYAstorm fork は本 r41 branch で先行 fix + 後日 upstream merge 取込 | (a) commit + (b) PR 並行、独立進行 |

---

## §3 残 strict 線形 (= 候補 (W) 着手後反映)

### §3.1 r41 milestone との独立性

- 本 (W) sub-step は **r41 host-side 改変由来でない上流既存 bug**、Phase 1.B PB-4.8 + PB-5.14 (= `ad4dc506f3`) commit message で literal flag 済 (= 「上流 bug flag (本 commit scope 外) = llglslshader.cpp:2514 uniform4iv method 内 glUniform1iv 呼出 = LL/Phoenix-Firestorm 上流既存 bug 疑い」)
- ただし PB-4.8 + PB-5.14 literal「line 2514」は本 prep 起案時に **line 2558 に訂正済** (= §0 確認)
- 本 (W) sub-step は **r41 milestone と独立**、AYAstorm release note 同梱判定も独立

### §3.2 候補 (X)(Y)(Z)(W) 内 strict 線形 (= 本 (W) prep 起案後反映)

```
✅ Phase 1.B host-side 完了
  ✅ (X) Phase 1.A residual prep 起案 (= commit 60070d048b)
  ✅ (Z) AYAstorm r20 SSS verify prep 起案 (= commit 5bc170579f)
  ✅ (W) 上流 uniform4iv bug prep 起案 (= 本 commit、並行起案 3/3 完了)
  → ⏳ 候補 sub-task 着手 AYA 判断 ((X)(Y)(Z)(W) または並行)
```

---

## §4 self-verify (= 本 handoff 起案時点、commit 前確認)

| # | 観点 | 確認方法 | 期待 |
|---|---|---|---|
| (1) Bug 真位置確定 + Phase 1.B complete §2.2 line 2514 literal 訂正 | source 直接 Read で line 2514 = uniform1iv method (正常) / line 2558 = uniform4iv method (bug) 物理確認 | ✅ §0 |
| (2) Bug 性質明文化 | method 名 vs glUniform 呼出 component 不一致 (= 4 vs 1)、3/4 truncation 効果 | ✅ §0 |
| (3) call site 0 件確認 | `grep -rn "uniform4iv" indra/ --include="*.cpp" --include="*.h"` で外部呼出 0 件 = dead code 状態 | ✅ §0 |
| (4) sub-task 構成 WA/WB/WC/WN | §2.1 table + §2.2 strict 線形理由 | ✅ |
| (5) AYA 判断要件 (a)(b)(c)(d) 比較 + Claude 推奨 (c) 根拠 3 件 | §2.3 4 案 + 推奨根拠 | ✅ |
| (6) `feedback_self_bug_no_defer_option` 適用範囲整理 | 本件は上流既存 bug ゆえ「自作 bug」ルール外、(d) fix 不要 judgement も structural 判断として並列可能 | ✅ §2.3 |
| (7) commit 内容 | handoff doc 1 件のみ、indra/ + scripts/ + cmake/ 改変 0 | ✅ (本 commit 段) |
| (8) Co-Authored-By 不在 | commit message 行頭 `Co-Authored-By:` 0 件 | ✅ |
| (9) `feedback_doubt_self_first` 反転適用 record | Phase 1.B complete §2.2 line 2514 literal を疑い source 直接確認で line 2558 訂正 (= handoff doc 内容を疑う) | ✅ §0 + §3.1 |

---

## §5 引き継ぎ memory (= 次 session 着手時参照)

特に重要 (= 既存 memory から):

- `project_ayastorm_r41_design_principles` (= 上流 OpenGL 取込やすさ維持 = upstream LL PR 採用根拠)
- `feedback_doubt_self_first` (= 反転適用、handoff doc 内容を疑い source 物理確認で line 2514 → line 2558 訂正)
- `feedback_self_bug_no_defer_option` (= 適用範囲整理、上流 bug ゆえ ruling 外、(d) fix 不要 judgement も structural 判断として並列可能)
- `feedback_release_branch_workflow` (= release branch 直接 commit 禁止、(a) 採用時は feature branch 切る)
- `feedback_release_flow` (= push / PR は AYA 側、Claude はローカル commit まで、(b) upstream LL PR も AYA 側 push)
- `feedback_no_auto_commit` (= 本 doc commit は AYA 明示指示後)
- `feedback_no_claude_coauthor` (= Co-Authored-By 行不在)
- `feedback_handoff_minimal_pre_req_read` (= 次 session pre-req は最小 3 件)
- `feedback_build_only_verified` (= WC build verify literal 充足、机上推論で済まさない)
- `feedback_no_scope_shrink` (= (W) literal scope WA/WB/WC/WN 全扱う、WB 上流 verify 省略しない)

---

## §6 次 session 着手 1 line

**「前 session で候補 (W) 上流 uniform4iv bug fix prep handoff doc 起案 + commit (= 本 doc + 1 commit)、(X)(Z)(W) 3 prep doc 3 commit 並行起案完了。本 session = AYA 判断 (a) AYAstorm fork 内 fix / (b) upstream LL PR / (c) 両方並行 (Claude 推奨) / (d) dead code ゆえ fix 不要 judgement のいずれか確定後、strict 線形 WA → WB → WC → WN 着手 (= (a)(c) 採用時) or record のみで close (= (d) 採用時) or LL fork setup + PR (= (b)(c) 採用時)。必読 3 件 = (1) 本 handoff doc 全文 + (2) `indra/llrender/llglslshader.cpp:2521-2563` + `3042-3079` + (3) `indra/llrender/llglslshader.h:237,250`。Bug 真位置 = line 2558 (= Phase 1.B complete §2.2 line 2514 literal は誤り、本 prep 起案時 source 直接確認で訂正、`feedback_doubt_self_first` 反転適用)、call site 0 件で現状 dead code 状態、fix scope 1 line trivial、r41 milestone と独立。」**
