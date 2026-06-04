# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-4.14〜.17 batch 着手 prep

**作成日**: 2026-06-04
**前 session 物理 commit**:
- `e83f76f2be` (= PB-4.9〜.13 batch、uniform1fv/2fv/3fv/4fv/4uiv 5 method Vulkan path 分岐、+70 line)

**本 session 着手**: **AYA 判断不要 自走** → **PB-4.14〜.17 batch 物理着手** = matrix 4 method (`uniformMatrix2fv` / `uniformMatrix3fv` / `uniformMatrix3x4fv` / `uniformMatrix4fv`) 連続 Vulkan path 分岐追加、4 連 Edit → 1 commit → full build 1 回。

---

## §0 state 一行 summary

η-30 **Phase 1.B PB-4.14〜.17 batch 着手 prep state** (= 前 commit `e83f76f2be` の続き):
- **本 doc 目的** = PB-4.14〜.17 matrix 4 method を **1 batch** で機械的展開する着手書、(U4) doctrine 構造的自然成立を根拠に design 判断 0 件確定
- **batch 化根拠** = 2026-06-04 AYA 提案「PB-4.9〜.17 を一括展開」+ `feedback_ubo_migration_one_at_a_time` 射程例外運用 4 条件全充足 ((1) 既存 redirect 層 call site 追加のみ + (2) 構造変更なし + (3) 設計判断なし + (4) AYA 提案受領済)
- **(U4) 構造的自然成立** = matrix 4 method の既存実装に **mValue cache check 0 件** + **shouldChange check 0 件** + **transpose=GL_TRUE 呼出 0 件 grep 確認済** → mValue block 削除 design 判断不要、`if (mUniform[index] >= 0)` block 内 `glUniformMatrixNfv` 直前に挿入で両 doctrine ((U4) と PB-4.7 pattern) 整合
- **想定 +60 line** = +15 line × 4 method (= PB-4.7 +14 line に matrix の挿入 indent +1 line 余裕、参考、実測時 ±2 line 内)
- **build verify 戦略** = full viewer build **1 回のみ** (= llrender 単体 build スキップ、3 経路非到達 verify 5/5 PASS で済む実績、初回 fail 時 packaged/ clean rebuild 救済 protocol 確立済)
- **設計判断 0 件** (= 全 default 採用、Claude 自走可)

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。本 session 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-4.14-4.17-batch-prep.md`) | 全文 | PB-4.14〜.17 batch literal pattern + 挿入位置 + size 計算 + (U4) 構造的自然成立根拠 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` | §5.2 + §5.3 (= integer index path 分岐 code shape + 規律表) | PB-4.14〜.17 の pattern source-of-truth |
| 3 | `indra/llrender/llglslshader.cpp:2729-2811` | matrix 4 method 既存実装 (= uniformMatrix2fv/3fv/3x4fv/4fv) | 挿入位置確定 |

### §1.2 pinpoint Read 用 reference (= 必要時のみ)

| file | 参照箇所 |
|---|---|
| `indra/llrender/llglslshader.cpp:2473-2491` | PB-4.7 (uniform1iv) Vulkan path 分岐 pattern (= pattern literal source、4 件全部 N と sizeof 差し替えで copy、ただし matrix 系は **mValue block 不在** で `if (mUniform[index] >= 0)` block 内に直接挿入する点が異なる) |
| `indra/llrender/llglslshader.cpp:2542-2719` | PB-4.9〜.13 Vulkan path 分岐 pattern (= 直前 commit、scalar fv 連続展開の参考、ただし matrix 系は mValue block 不在で挿入位置だけ異なる) |
| `indra/llrender/llglslshader.h:443-457` | `forwardToUboUpload()` 宣言 (= PB-6 追加済、ptr 引数 byte size 受領) |
| `indra/llrender/llglslshader.h:427-429` | `mUseUBO = false` default 確認 (= MUSEUBO-A) |
| `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-and-locking.md:133` (U4) | mValue cache 適用外明記 (= matrix 系は既存 mValue check 0 件 = (U4) 構造的に自然成立) |

---

## §2 PB-4.14〜.17 各 method 挿入位置 + size 計算

### §2.1 挿入位置確定 (= 4 method 全 `if (mUniform[index] >= 0)` block 内 `glUniformMatrixNfv` 直前)

| PB-X | method | 挿入対象 line | OpenGL call | size 計算 |
|---|---|---|---|---|
| PB-4.14 | `uniformMatrix2fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v)` | line 2745 | `glUniformMatrix2fv(mUniform[index], count, transpose, v);` | `count * 4 * sizeof(GLfloat)` (= 2×2 matrix) |
| PB-4.15 | `uniformMatrix3fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v)` | line 2766 | `glUniformMatrix3fv(mUniform[index], count, transpose, v);` | `count * 9 * sizeof(GLfloat)` (= 3×3 matrix) |
| PB-4.16 | `uniformMatrix3x4fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v)` | line 2787 | `glUniformMatrix3x4fv(mUniform[index], count, transpose, v);` | `count * 12 * sizeof(GLfloat)` (= 3×4 matrix) |
| PB-4.17 | `uniformMatrix4fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v)` | line 2808 | `glUniformMatrix4fv(mUniform[index], count, transpose, v);` | `count * 16 * sizeof(GLfloat)` (= 4×4 matrix) |

**注 (= 既存 method 構造)**: matrix 4 method の既存実装は **全 4 件同型** = `LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER` + `llassert(sCurBoundShaderPtr == this)` + `if (mProgramObject) { if (mUniform.size() <= index) {...return;} if (mUniform[index] >= 0) { glUniformMatrixNfv(...); } }` のシンプル構造。**mValue cache check + shouldChange check 0 件** (= scalar fv 系の `if (iter == mValue.end() || shouldChange(...) || count != 1)` block 不在) → PB-4.7 pattern を **そのまま** `if (mUniform[index] >= 0)` block 内に挿入できる、design 判断不要。

### §2.2 共通 pattern literal (= 4 件全部 N と method 番号差し替えのみ)

```cpp
if (mUseUBO)
{
    llassert(index < mUniformUBOLoc.size());
    const ubo::UniformLocation& loc = mUniformUBOLoc[index];
    if (loc.cadence_tag == 0xFFFFFFFFu) return;
    if (loc.cadence_tag == 5 /* CADENCE_SAMPLER */) return;
    forwardToUboUpload(loc, v, count * N * sizeof(GLfloat));
    return;
}
```
(= PB-4.7 (uniform1iv) pattern と diff は 2 箇所のみ: comment 番号 + N、構造 6 行完全一致、可読性同等)

**comment header** (= PB-4.7 と同型、`PB-4.7` を `PB-4.14` 等に、5 行目を「matrix N×M (= count * N * sizeof(GLfloat))」に書換):
```cpp
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-4.14:
// integer index 経路 Vulkan path 分岐追加。spec 06a §5.2 / §5.3 literal 準拠。
// GATE-B = #ifdef LL_VULKAN_GLSL 不使用、mUseUBO runtime flag 単独 gate。
// MUSEUBO-A = mUseUBO=false default で本 block 走らず既存 OpenGL 挙動 100% 維持。
// matrix 2x2 (= count * 4 * sizeof(GLfloat))、(U4) 構造的自然成立 (= mValue check 既存不在)。
```
(= PB-4.15 は「matrix 3x3 (= count * 9 * sizeof(GLfloat))」、PB-4.16 は「matrix 3x4 (= count * 12 * sizeof(GLfloat))」、PB-4.17 は「matrix 4x4 (= count * 16 * sizeof(GLfloat))」)

### §2.3 build verify 戦略

| 段階 | 手順 | 期待結果 |
|---|---|---|
| (1) 4 連 Edit | PB-4.14 → PB-4.15 → PB-4.16 → PB-4.17 順に Edit (= 同じ pattern を 4 回 N 差し替えで apply) | `git diff --stat` = 1 file changed + ~60 insertions (= +15 line × 4 想定、実測 ±2 line) |
| (2) static verify §5 全 7 観点 | (1) LL_VULKAN_GLSL コード行 0、comment 4 件 + (2) mUseUBO=true 新規 0 + (3) Co-Authored-By 不在 + (4) mUniformUBOLoc[index] 参照追加 4 件 + (5) mUniformUBOLocByHash.find n/a (PB-4.14〜.17 対象外) + (6) forwardToUboUpload 追加 4 件 + (7) glUniformMatrixNfv 既存 line 削除 0 件 | 全 PASS |
| (3) full viewer build | `cmake --build build-linux-x86_64 -j` (= llrender 単体 build スキップ) | EXIT 0、tar.xz 生成 (= 過去 build と同等 size 200-210 MB) |
| (4) build fail 救済 protocol | 初回 fail 時 `rm -rf build-linux-x86_64/newview/packaged && cmake --build build-linux-x86_64 --target llpackage -j` 再走 | 前 session で確立済 (= packaged/packaged/ nested 残骸由来 mv rename race) |
| (5) 1 commit | 4 method 連続 Vulkan path 分岐追加を 1 commit にまとめる、AYA style verbose 単一行 commit message | Co-Authored-By 行不在 |

---

## §3 (U4) doctrine 整合性確認

### §3.1 (U4) literal 確認

`docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-and-locking.md:133` (U4):
- 「matrix 系 + uniform4iv は mValue cache 適用外」明記
- 根拠 = matrix 系は frame 跨ぎ identity 比較が非実用的 (= scratch buffer 上 layout 比較重い) + uniform4iv は値域広く cache hit rate 低い

### §3.2 PB-4.14〜.17 matrix 系の構造的自然成立

| 観点 | matrix method 既存実装 | (U4) doctrine | 判定 |
|---|---|---|---|
| mValue cache check | **既存不在** | mValue cache 適用外 (= bypass) | ✅ 構造的自然成立 (= 既に bypass されている) |
| shouldChange check | **既存不在** | bypass | ✅ 同上 |
| count != 1 条件 | **既存不在** | bypass | ✅ 同上 |
| `if (mUniform[index] >= 0)` block | **既存あり** | OpenGL path gate 維持 | ✅ Vulkan path もこの block 内に挿入で同一 gate 適用 |

→ **PB-4.14〜.17 は (U4) と PB-4.7 pattern の両方に何も追加 design 判断なく自然整合**、`if (mUniform[index] >= 0)` block 内に Vulkan path 分岐挿入だけで完了。

### §3.3 transpose=GL_TRUE 確認 (= 完了)

`Grep glUniformMatrix.*GL_TRUE indra/` = **0 件**。viewer 全 source 内で transpose=GL_TRUE 呼出ゼロ。

→ Vulkan path 側で transpose 引数を **無視** して `forwardToUboUpload(loc, v, count * N * sizeof(GLfloat))` するのは安全 (= future-proof のため、もし将来 transpose=GL_TRUE が必要になったら別 PB で transpose 経路を追加、本 PB scope 外 = `feedback_no_scope_shrink` 逆向きの「不必要に scope を広げない」)。

### §3.4 PB-4.8 (uniform4iv) が本 batch 除外な理由

PB-4.8 = `uniform4iv(U32 index, U32 count, const GLint* v)` は **既存実装に mValue block + shouldChange + count != 1 条件あり** (= scalar iv 系と同型)。(U4) doctrine literal は「mValue cache 適用外」なので Vulkan path 挿入位置は:
- (A) **mValue block 内側** (= PB-4.7 整合、scalar iv 系と同形 → 既存実装の mValue block 内に Vulkan path を挿入、(U4) literal「適用外」を「Vulkan path では bypass」と解釈)
- (B) **mValue block 外側** (= (U4) integral 解釈、既存実装の mValue block を Vulkan path 前に bypass、PB-4.7 pattern 例外扱い)

の **design 判断要件**。本 batch では **除外** し、PB-4.14〜.17 batch 完了後に別 sub-step で AYA 判断を仰ぐ。

---

## §4 残 sub-step strict 線形 (= PB-4.14〜.17 batch 着手 prep 反映)

```
PB-4.14〜.17 batch (= 本 session 着手)
  → PB-4.8 (= (U4) design 判断 AYA 待ち、別 sub-step)
  → PB-5.1〜.13 (= 13 method LLStaticHashedString 経路 Vulkan path 分岐)
  → PB-7 (= mapUniforms() 末尾 debug build llassert 整合 check)
  → PB-N (= Phase 1.B Exit Criteria 検証、AYA と一緒に実行)
```

---

## §5 self-verify 観点 (= 着手後 commit 前に確認)

| 観点 | 確認方法 | 期待 |
|---|---|---|
| (1) +56〜+60 line 物理確認 | `git diff --stat` = 1 file changed + ~60 insertions (= +15 line × 4 想定、実測 ±2 line) | matrix indent +1 line 想定範囲 |
| (2) 挿入位置 | 各 method の `if (mUniform[index] >= 0)` block 内 `glUniformMatrixNfv` 直前 | scalar fv 系の mValue block 内とは別位置 |
| (3) GATE-B 順守 | `git diff \| grep "^+" \| grep "LL_VULKAN_GLSL" \| grep -v "^+.*//"` = 0 件 (comment 中のみ 4 件) | OK |
| (4) MUSEUBO-A 順守 | `git diff llglslshader.h` = 0 | OK |
| (5) static verify §5 全 7 観点 | (1〜7) 全 PASS | OK |
| (6) 残 sub-step strict 線形維持 | §4 table = PB-4.8 → PB-5.1〜.13 → PB-7 → PB-N | OK |
| (7) batch 化条件 4 件 | (1) call site 追加のみ + (2) 構造変更なし + (3) 設計判断なし + (4) AYA 提案受領済 | 全充足 |
| (8) working tree = 1 file のみ | `git status` = M indra/llrender/llglslshader.cpp のみ | OK |
| (9) Co-Authored-By 不在 | commit message 行頭 `Co-Authored-By:` 0 件 | OK |

---

## §6 引き継ぎ済 memory (= PB-4.7/4.9-4.13 handoff 継承、追加なし)

- `project_r41_phase1b_vulkan_host_gate.md` — Phase 1.B 全 sub-step で C++ 側 `#ifdef LL_VULKAN_GLSL` 不使用、runtime `mUseUBO` flag 単独 gate
- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ厳守
- `feedback_no_scope_shrink` — Phase 1.B Exit Criteria literal「30 setter 全部」厳守 + 逆向きの「不必要に scope を広げない」(= transpose=GL_TRUE 経路追加禁止、本 batch scope 外)
- `feedback_self_verify_before_handoff` — commit 前 9 観点 self-verify
- `feedback_no_claude_coauthor` — 全 commit 共著行不在
- `feedback_one_step_at_a_time` — strict 線形進行
- `feedback_ubo_migration_one_at_a_time` — **射程例外運用 4 条件 (= (1) call site 追加のみ + (2) 構造変更なし + (3) 設計判断なし + (4) AYA 提案/同意) 充足時のみ batch 化可、本 batch は全充足で適用**
- `feedback_doubt_self_first` — build error / link fail は AYA 投げ前に Claude root cause 特定
- `feedback_no_auto_commit` — handoff §7 1 line 自走指示後は AYA 明示指示なしで commit 可
- `feedback_remove_verification_logs` — commit 前 LL_INFOS hook 除去
- `feedback_self_bug_no_defer_option` — PB scope 外の他人 bug (= viewer_manifest.py mv -T 無し等) は本 sub-step で触らない、別 issue で

---

## §7 本 session 着手 1 line

**「前 session で Phase 1.B PB-4.9〜.13 batch 5 method 完了 (commit `e83f76f2be`、+70 line)。本 session = **AYA 判断不要 自走** = **PB-4.14〜.17 batch 物理着手** = `uniformMatrix2fv` (llglslshader.cpp:2729) / `uniformMatrix3fv` (line 2750) / `uniformMatrix3x4fv` (line 2771) / `uniformMatrix4fv` (line 2792) の 4 method 連続 Vulkan path 分岐追加 (= 各 method 内 `if (mUniform[index] >= 0)` block 内 `glUniformMatrixNfv` 直前に §2.2 pattern を **size 計算のみ N=4/9/12/16 差し替え** で挿入、4 連 Edit → 1 commit → full viewer build 1 回、初回 fail 時 `rm -rf build-linux-x86_64/newview/packaged` 後 llpackage 再走で救済)、(U4) doctrine 構造的自然成立 (= 既存 method に mValue check 0 件 + shouldChange 0 件 + transpose=GL_TRUE 呼出 0 件 grep 確認済)、GATE-B 整合 + MUSEUBO-A 整合、06a §5.2 / §5.3 literal、`feedback_ubo_migration_one_at_a_time` 射程例外 4 条件全充足 → PB-4.14〜.17 完了後 PB-4.8 ((U4) design 判断 AYA 待ち) → PB-5.1〜.13 → PB-7 → PB-N strict 線形進行。」**
