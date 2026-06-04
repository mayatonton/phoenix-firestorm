# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 上流 uniform4iv method 内 glUniform1iv bug fix **complete** marker (= candidate (W) 完結)

**作成日**: 2026-06-04
**前 commit chain** (= 直前 handoff 起案):
- `35c4be1046` = Phase 1.B **complete** marker
- `60070d048b` = 候補 (X) Phase 1.A residual prep
- `5bc170579f` = 候補 (Z) AYAstorm r20 SSS verify prep
- `33f983c272` = 候補 (W) 上流 uniform4iv bug fix **prep**
- `fe2f3a81c6` = (X) Phase 1.A PA-B + PA-N **complete** = Phase 1.A 章クローズ
- `7401feeb1f` = (Z) AYAstorm r20 SSS 効き verify **complete** marker
- **`2a06e12f44`** (on `fix/upstream-uniform4iv-glunifrm1iv-bug` branch、本 commit 対象 fix) = (W) WA literal fix

**本 handoff doc 目的**: **(W) sub-step ZA/ZB/ZC/ZN 全完結 marker + fix commit reference + push/PR pending 状態 record + 残候補 AYA 判断要件 + 次 session 引継**。(W) prep handoff doc (= commit `33f983c272`) §6 next session 着手 1 line literal「AYA 判断 (a)/(b)/(c)/(d) 確定後 strict 線形 WA → WB → WC → WN 着手」literal 充足分を本 commit で完結記録。

---

## §0 (W) sub-step 完結 state 一行 summary

候補 (W) 上流 uniform4iv 内 glUniform1iv bug fix **AYAstorm fork 内 (a) path 完結**:

- **Fix 本質**: `indra/llrender/llglslshader.cpp:1480` (= `LLGLSLShader::uniform4iv(U32 index, U32 count, const GLint* v)` method line 1460-1485 内) で `glUniform1iv(mUniform[index], count, v);` → `glUniform4iv(mUniform[index], count, v);` 修正 = +1/-1、method 名整合
- **修正 commit**: `2a06e12f44` on `fix/upstream-uniform4iv-glunifrm1iv-bug` branch (= `ayastorm-release` `214054a5a7` 起点、`feedback_release_branch_workflow` 遵守)
- **Build verify**: AYA 実機 (Linux x86_64) build EXIT 0 + viewer 起動 fail 0 件 + 終了 fail 0 件 (2026-06-04)
- **Bug 性質再確認**: method 名 `uniform4iv` = 4 component × count 個 int upload 想定 (= line 1477 `LLVector4 vec((F32)v[0], (F32)v[1], (F32)v[2], (F32)v[3]);`) だが `glUniform1iv` 呼出 = count 個 int のみ upload = **3/4 truncation**
- **dead code 状態**: call site 0 件 (= `grep -rn "uniform4iv" indra/ --include="*.cpp" --include="*.h"` 出力 = method 定義 4 件 + comment 3 件のみ、外部呼出 0 件)、起動 PASS で動作差分ゼロ実証
- **上流既存 bug 同型 verify**: LL `secondlife/viewer` master HEAD + Phoenix-Firestorm `phoenix-firestorm` master HEAD の同 file 同 line (= line 1480) で同型 bug 確認 (= gh api raw fetch + grep)
- **push/PR**: pending AYA 側 (= `feedback_release_flow`)
- **(b) upstream LL PR**: 別 sub-task として後続起案 (= (c) 両方並行 path の後半部分)

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc | 全文 | (W) 完結 state + 残作業 ((b) upstream PR + AYAstorm push/PR) + 残候補 (Y) AYA 判断要件 + Claude 推奨 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-upstream-uniform4iv-bug-prep.md` | 全文 (= commit `33f983c272`) | (W) prep 全体: sub-task WA/WB/WC/WN 構成 + AYA 判断要件 (a)(b)(c)(d) + Bug 真位置記録 (本 prep doc 起案時 line 2558 = r41 branch line、本 fix branch = ayastorm-release HEAD では line 1480) |
| 3 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase-1-B-complete.md` | §2.2 (= 上流 bug flag 残件 literal、line 2514 literal は本 prep doc で line 2558 → 本 complete doc で line 1480 訂正済) | (W) milestone 内位置確認 |

### §1.2 pinpoint Read 用 reference

| file:line | 必要時の参照箇所 |
|---|---|
| `indra/llrender/llglslshader.cpp:1460-1485` (ayastorm-release HEAD + 本 fix branch) | `uniform4iv(U32 index, ...)` 整数 index 経路 method 本体 = 本 fix 対象 |
| `indra/llrender/llglslshader.cpp:1433-1458` (ayastorm-release HEAD) | `uniform1iv(U32 index, ...)` 正常 method = 線 1453 `glUniform1iv` 正常 call (= prep doc line 2514 literal 由来、line 番号は branch 依存) |
| `indra/llrender/llglslshader.cpp:1802-1820` (ayastorm-release HEAD) | hashed 経路 `uniform4iv(LLStaticHashedString&, ...)` 正常 method (= line 1814 `glUniform4iv` 正常 call) |
| `indra/llrender/llglslshader.h:237,250` (ayastorm-release HEAD) | `uniform4iv` 2 method 宣言 |
| `indra/llrender/llglslshader.cpp:2521-2563` (r41 branch only) | r41 mUseUBO redirect block 内蔵の `uniform4iv` 整数 index 経路 = r41 上では line 2558 が同 bug 位置 |

---

## §2 sub-task 完了 record

### §2.1 WA = bug fix 1 line 物理改変 (✅ 完結)

| 項目 | 内容 |
|---|---|
| Fix branch | `fix/upstream-uniform4iv-glunifrm1iv-bug` (= `ayastorm-release` `214054a5a7` 起点) |
| Fix file | `indra/llrender/llglslshader.cpp` |
| Fix line (ayastorm-release HEAD) | 1480 (= prep doc r41 branch 上 line 2558 と structural 等価) |
| Fix 前 | `glUniform1iv(mUniform[index], count, v);` |
| Fix 後 | `glUniform4iv(mUniform[index], count, v);` |
| Diff stat | 1 file +1/-1 |
| Other change | 無し (= C++ 他経路 / GLSL / settings.xml / cmake 改変 0) |
| Commit | `2a06e12f44` |

### §2.2 WB = LL upstream + Phoenix-Firestorm 上流 bug 状態 verify (✅ 完結)

- **LL upstream** (= `secondlife/viewer` master HEAD): `indra/llrender/llglslshader.cpp:1480` 同型 bug 確認 (= `gh api -H "Accept: application/vnd.github.raw" /repos/secondlife/viewer/contents/indra/llrender/llglslshader.cpp | grep -n "glUniform1iv\|glUniform4iv"` 出力 = line 1480 `glUniform1iv(mUniform[index], count, v);` 同型確認 + line 1814 `glUniform4iv(location, count, v);` 正常 reference)
- **Phoenix-Firestorm 上流** (= `FirestormViewer/phoenix-firestorm` master HEAD): 同 file 同 line (= 1480) 同型 bug 確認 (= 同 gh api 出力で identical pattern)
- **判定**: upstream LL PR 価値あり = (b) AYAstorm fork 外で LL 上流に PR 起案で長期保守、LL merge 後 Phoenix-Firestorm 自然取り込み、AYAstorm fork は upstream merge 後 rebase 取込
- **call site 上流推論**: 本 fork (= ayastorm-release HEAD = LL/Phoenix upstream を取り込んだ後の状態) で call site 0 件 = 上流も同型 dead code 状態と推論

### §2.3 WC = build verify (✅ 完結)

- **Build environment**: AYA 実機 Linux x86_64 (`~/work_firestorm/phoenix-firestorm` `fix/upstream-uniform4iv-glunifrm1iv-bug` branch)
- **Build command**: `project_build_procedure.md` 記載手順 (= `autobuild configure -A 64 -c ReleaseFS_open -- --fmodstudio -DLL_TESTS:BOOL=FALSE -DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE --package --chan AYAstorm-release` → `autobuild build -A 64 -c ReleaseFS_open --no-configure` → `install.sh` → cache clear)
- **Build EXIT**: 0
- **viewer 起動**: PASS (fail 0 件)
- **viewer 終了**: PASS (fail 0 件)
- **動作差分**: 0 (= call site 0 件 dead code 実証、起動/終了 完全 PASS)

### §2.4 WN = Exit Criteria 確認 + handoff complete marker (✅ 本 doc 起案で完結)

- 9 観点 self-verify (= §5)
- 本 handoff doc 起案 + 1 commit (= 本 commit on `feature/ayastorm-r41-gl-removal` branch、修正 commit は別 branch `fix/upstream-uniform4iv-glunifrm1iv-bug` 上)
- AYA「commit して」literal 受領 (2026-06-04) で (W) WN literal「Exit Criteria 確認 + handoff complete marker」充足

---

## §3 修正 commit 詳細

### §3.1 branch / commit hash

- **Branch**: `fix/upstream-uniform4iv-glunifrm1iv-bug` (= `ayastorm-release` `214054a5a7` 起点で切出し)
- **Commit hash**: `2a06e12f44`
- **Base**: `ayastorm-release` (= PR base、push/PR pending AYA 側)

### §3.2 diff 内容

```
indra/llrender/llglslshader.cpp | 2 +-
1 file changed, 1 insertion(+), 1 deletion(-)
```

#### 修正前 (line 1480)

```cpp
                glUniform1iv(mUniform[index], count, v);
```

#### 修正後 (line 1480)

```cpp
                glUniform4iv(mUniform[index], count, v);
```

### §3.3 修正対象 method context (= `indra/llrender/llglslshader.cpp:1460-1485`)

```cpp
void LLGLSLShader::uniform4iv(U32 index, U32 count, const GLint* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec((F32)v[0], (F32)v[1], (F32)v[2], (F32)v[3]);
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                glUniform4iv(mUniform[index], count, v);   // ← 本 fix で `glUniform1iv` から修正
                mValue[mUniform[index]] = vec;
            }
        }
    }
}
```

---

## §4 push/PR pending (= AYA 側 next action)

### §4.1 AYAstorm fork 内 (a) 経路 = 本 commit の push + PR

| 項目 | 内容 |
|---|---|
| Local branch | `fix/upstream-uniform4iv-glunifrm1iv-bug` |
| Local commit | `2a06e12f44` |
| Push command | `git push -u origin fix/upstream-uniform4iv-glunifrm1iv-bug` |
| PR base | `ayastorm-release` |
| PR title 案 | `fix(llrender): uniform4iv(U32 index, ...) method 内 glUniform1iv → glUniform4iv (上流既存 bug fix)` |
| PR body 主旨 | 上流既存 bug (LL + Phoenix-Firestorm 同型 verify 済) + dead code 状態 (= call site 0 件) + 1 line fix + AYAstorm fork 内即時恩恵 + 上流 PR 別途並行起案予定 (= (c) path) |

### §4.2 LL upstream (b) 経路 = LL 上流 PR 起案

| 項目 | 内容 |
|---|---|
| Repo | `secondlife/viewer` |
| Fork setup | AYA 側で LL fork (= 既 fork 済の場合は再利用、未設定なら `gh repo fork secondlife/viewer`) |
| Fix branch (LL fork 上) | 別途切出し (例: `fix/uniform4iv-int-index-gluniform-mismatch`) |
| Fix 内容 | 本 commit と同じ (= line 1480 1 line +1/-1)、ただし LL upstream commit style に合わせて message 調整 |
| PR base | `secondlife/viewer:main` (or master) |
| PR title 案 | `fix: uniform4iv(U32 index, ...) calls glUniform1iv instead of glUniform4iv` |
| PR body 主旨 | bug nature (3/4 truncation) + hashed 経路正常 reference + call site 0 件 dead code 状態 + 1 line fix |
| 起案タイミング | 別 session 別 sub-task (= 本 (W) sub-step 完結後の独立 follow-up) |

---

## §5 self-verify (= 本 handoff 起案時点、commit 前確認)

| # | 観点 | 確認方法 | 期待 |
|---|---|---|---|
| (1) WA fix 物理確認 | `git diff 2a06e12f44~ 2a06e12f44 -- indra/llrender/llglslshader.cpp` で +1/-1 確認、line 1480 `glUniform1iv` → `glUniform4iv` 物理確認 | ✅ §3.2 |
| (2) Bug 性質明文化 | method 名 vs glUniform 呼出 component 不一致 (= 4 vs 1)、3/4 truncation 効果 | ✅ §0 |
| (3) call site 0 件確認 | `grep -rn "uniform4iv" indra/ --include="*.cpp" --include="*.h"` で外部呼出 0 件 = dead code 状態 | ✅ §0 |
| (4) WB 上流 bug 同型 verify | LL + Phoenix-Firestorm 上流 raw fetch + grep で line 1480 同型 bug 確認 | ✅ §2.2 |
| (5) WC build + 起動 + 終了 PASS | AYA 実機 build EXIT 0 + 起動 fail 0 件 + 終了 fail 0 件 (2026-06-04) | ✅ §2.3 |
| (6) `feedback_release_branch_workflow` 遵守 | `ayastorm-release` 直接 commit せず `fix/upstream-uniform4iv-glunifrm1iv-bug` feature branch 切出し | ✅ §3.1 |
| (7) `feedback_doubt_self_first` 反転適用 | prep handoff doc line 2558 literal を疑い source 直接確認で本 branch 上 line 1480 訂正 (= ayastorm-release HEAD は r41 改変未取込で line 番号差分) | ✅ §1.2 + 本 doc §0 |
| (8) `feedback_no_scope_shrink` 遵守 | (W) literal scope WA/WB/WC/WN 全 sub-task 全扱う、WN は本 commit に同梱完結 ((Z) 同様 pattern) | ✅ §2 |
| (9) Co-Authored-By 不在 | commit message 行頭 `Co-Authored-By:` 0 件 (= 本 doc commit + fix commit 両方) | ✅ |

---

## §6 r41 milestone state + 残候補

### §6.1 r41 milestone state

| Phase | 状態 |
|---|---|
| Phase 1.A | ✅ 章クローズ (= commit `fe2f3a81c6`) |
| Phase 1.B (host-side) | ✅ complete (= commit `35c4be1046`) |
| (Z) AYAstorm r20 SSS verify | ✅ complete (= commit `7401feeb1f` + 修正 `4dde489ec4` on `fix/r20-sss-spec-comment`) |
| (W) 上流 uniform4iv bug fix | ✅ **本 doc 完結** (= 修正 `2a06e12f44` on `fix/upstream-uniform4iv-glunifrm1iv-bug` + 本 doc) |
| Phase 1.C (test UBO shell) | ⏳ 未着手 |
| (Y) Phase 1.C prep doc | ⏳ 未起案 |
| (W) upstream LL PR (b) 経路 | ⏳ 別 session 別 sub-task |

### §6.2 残候補 = AYA 判断要件

- **(Y) Phase 1.C prep doc 起案**: (X) 完了で unblocked 済、Phase 1.C = test UBO shell 実装 phase。prep doc 未起案、本 session 後に AYA 指示で起案可
- **(W) (b) upstream LL PR 起案**: 本 (a) AYAstorm fork 内 fix 完結後、独立 follow-up sub-task として起案可 (= 推奨 (c) 両方並行の後半部分)
- **AYAstorm fork push + PR**: 本 fix commit `2a06e12f44` の push + PR (= base `ayastorm-release`)、AYA 明示指示後

### §6.3 Claude 推奨分岐

- (a) push + PR 先行 (= 本 commit immediate review に乗せる) → 並行 (Y) prep doc 起案 → 並行 (b) LL upstream PR 起案
- (b) (Y) prep doc 起案先行 (= Phase 1.C 着手準備) → 後日 push + PR + (b) upstream PR
- (c) (b) LL upstream PR 起案先行 (= LL review cycle 長いため早期着手) → 並行 push + PR + (Y) prep doc 起案

**Claude 推奨**: (a) push + PR 先行 = 本 fix の review cycle を早期回す + 並行で (Y) prep + (b) LL PR 起案。根拠 3 件:
1. 本 fix は dead code ゆえ実害なしだが、AYAstorm release note 同梱判定 (= AYAstorm fork 内取込タイミング) は AYA 判断必要、PR で review 早期化が後続 milestone 整合的
2. (b) LL upstream PR は review cycle 長い (= 本来 medium-long、LL review cadence 不定) ので並行起案で時間損失なし
3. (Y) Phase 1.C prep doc 起案は (W) と orthogonal、並行進行可能

---

## §7 引き継ぎ memory (= 次 session 着手時参照)

特に重要 (= 既存 memory から):

- `project_ayastorm_r41_design_principles` (= 上流 OpenGL 取込やすさ維持 = upstream LL PR 採用根拠)
- `project_ayastorm_r41_vulkan_migration` (= r41 milestone 全体 state)
- `feedback_doubt_self_first` (= 反転適用、handoff doc 内容を疑い source 物理確認で line 番号訂正)
- `feedback_self_bug_no_defer_option` (= 適用範囲整理、上流 bug ゆえ ruling 外、(d) fix 不要 judgement は本 fix で adopted せず実施)
- `feedback_release_branch_workflow` (= release branch 直接 commit 禁止、feature branch 切出し遵守)
- `feedback_release_flow` (= push / PR は AYA 側、Claude はローカル commit まで)
- `feedback_no_auto_commit` (= 本 doc commit + fix commit は AYA 明示指示後)
- `feedback_no_claude_coauthor` (= Co-Authored-By 行不在)
- `feedback_handoff_minimal_pre_req_read` (= 次 session pre-req は最小 3 件 + pinpoint reference 別記、全件 Read 禁止)
- `feedback_build_only_verified` (= WC build verify literal 充足、机上推論で済まさない)
- `feedback_no_scope_shrink` (= (W) literal scope WA/WB/WC/WN 全扱う、WN は本 commit に同梱完結)
- `feedback_self_verify_before_handoff` (= 9 観点 self-verify 全 ✅)
- `feedback_proactive_handoff` (= context 残量監視で能動 handoff)
- `feedback_log_reading` (= build/起動 AYA、log 解析 Claude)

---

## §8 次 session 着手 1 line

**「前 session で候補 (W) 上流 uniform4iv 内 glUniform1iv bug fix 完結 (= 修正 commit `2a06e12f44` on `fix/upstream-uniform4iv-glunifrm1iv-bug` branch + 本 complete doc)、(W) sub-step WA/WB/WC/WN 全 PASS。本 session = AYA 指示 (a) push + PR (= 本 fix `fix/upstream-uniform4iv-glunifrm1iv-bug` → `ayastorm-release`) or (b) LL upstream PR 起案 (= LL `secondlife/viewer` fork + PR) or (Y) Phase 1.C prep doc 起案 のいずれか literal 受領後着手。Claude 推奨 = (a) push + PR 先行 + 並行 (Y) prep + (b) LL PR 起案。必読 3 件 = (1) 本 handoff doc 全文 + (2) (W) prep doc commit `33f983c272` + (3) Phase 1.B complete marker commit `35c4be1046` §2.2。Bug 真位置 = `ayastorm-release` HEAD `indra/llrender/llglslshader.cpp:1480` (= r41 branch 上 line 2558 と structural 等価、line 番号は branch 依存)、dead code 状態 (= call site 0 件)、`feedback_release_branch_workflow` 遵守。」**
