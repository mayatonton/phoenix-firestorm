# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-4.9〜.12 batch 着手 prep

**作成日**: 2026-06-04
**起案契機**: AYA さん提案 (2026-06-04) =「PB-4.9〜.17 を一括展開して最後にビルドしたほうが効率いいのでは？」
**Claude 分析応答**: 全 9 件 (= PB-4.9〜.17) は **3 group + 設計判断 1 件** に切り分け必要。
- group A = PB-4.9〜.12 (uniform1fv〜uniform4fv) **4 件 = PB-4.7 と完全同型、構造差ゼロ、機械的展開** → **本 batch 採用**
- group B = PB-4.13 (uniform4uiv) = A と完全同型 GLuint 版 (本 batch から除外、別 sub-step)
- group C = PB-4.8 (uniform4iv) + PB-4.14〜.17 (matrix 4 件) = **spec 06b §3.4 (U4) で mValue cache 適用外明記、Vulkan path 挿入位置の design 判断要** (本 batch から除外、別 sub-step)
- 設計判断 1 件 = (U4) 適用 method の Vulkan path 挿入位置 (= shouldChange/mValue block の内側 / 外側) → group C 着手前に AYA 判断要

**本 batch 物理改変対象**: `indra/llrender/llglslshader.cpp` 1 file = **PB-4.9〜.12 の 4 method に Vulkan path 分岐追加** = 1 commit、build verify は最後に 1 回 (= AYA 提案の効率化を採用)。

**前 commit 1 件継承**: `0d0a34000f` (= PB-4.7 complete handoff、PB-4.7 物理 commit = `1b54b29d3f`)。

---

## §0 state 一行 summary

η-30 **Phase 1.B PB-4.7 完了 + PB-4.9〜.12 batch 着手 prep**:
- **PB-4.7 uniform1iv** 完了 (= `1b54b29d3f`、+14 line、ptr+count pattern 新系統初出)
- **本 batch = PB-4.9〜.12** (= uniform1fv / uniform2fv / uniform3fv / uniform4fv 4 件) を **1 commit 一括展開**、build verify は最後に 1 回
- **(U4) 対象 5 件 (= PB-4.8 + PB-4.14〜.17) は本 batch 除外**、別 sub-step で AYA design 判断後実施
- **PB-4.13 uniform4uiv** も本 batch 除外 (= A と同型だが GLuint 系、別 batch 候補)
- **batch 採用根拠**: PB-4.9〜.12 の 4 method は **PB-4.7 と完全同型構造** (= shouldChange block 内 glUniformXfv 直前、引数型 const GLfloat* v、count 引数)、`feedback_ubo_migration_one_at_a_time` 射程例外 = 「**既存 redirect 層 (forwardToUboUpload) への call site 追加のみ、構造変更なし、設計判断なし**」の場合に限り batch 化可、本 batch に該当
- **設計判断 0 件新規確定** (= 全 default 採用済、Claude 自走継続可、AYA 判断は batch 着手指示のみ既受領)

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-4.9-4.12-batch-prep.md`) | 全文 | batch scope 確定 + 4 method 各 literal + (U4) 除外明記 + build verify 戦略 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` | §5.2 + §5.3 (= integer index path 分岐 code shape + 規律表) | pattern source-of-truth |
| 3 | `indra/llrender/llglslshader.cpp:2522-2629` | `uniform1fv` / `uniform2fv` / `uniform3fv` / `uniform4fv` 4 method 既存実装 | 4 件挿入位置確定 |

### §1.2 pinpoint Read 用 reference (= 必要時のみ)

| file | 参照箇所 | 用途 |
|---|---|---|
| `indra/llrender/llglslshader.cpp:2453-2491` | PB-4.7 (uniform1iv) Vulkan path 分岐 pattern | **pattern literal source** (= 4 件全部この pattern を T と N 差し替えで copy) |
| `indra/llrender/llglslshader.h:443-457` | `forwardToUboUpload()` 宣言 | ptr 引数 byte size 受領確認 |
| `indra/llrender/llglslshader.h:427-429` | `mUseUBO = false` default | MUSEUBO-A 確認 |
| `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md:133` | (U4) 明記行 | **本 batch 除外** = PB-4.8 + PB-4.14〜.17 設計判断要の根拠 |

---

## §2 batch 4 件の完全 literal

### §2.1 各 method 挿入位置確定 (= llglslshader.cpp HEAD 時点 line 番号)

| sub-step | method | 挿入行 (= 既存 glUniformXfv の直前) | size 計算 (= forwardToUboUpload 第 3 引数) |
|---|---|---|---|
| **PB-4.9** | uniform1fv | 2542 (`glUniform1fv` 直前) | `count * sizeof(GLfloat)` |
| **PB-4.10** | uniform2fv | 2569 (`glUniform2fv` 直前) | `count * 2 * sizeof(GLfloat)` |
| **PB-4.11** | uniform3fv | 2596 (`glUniform3fv` 直前) | `count * 3 * sizeof(GLfloat)` |
| **PB-4.12** | uniform4fv | 2624 (`glUniform4fv` 直前、line 2623 の重複 `LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER` は触らない) | `count * 4 * sizeof(GLfloat)` |

**注 (= PB-4.12 uniform4fv の特異性)**: line 2606 + line 2623 に `LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER` の重複宣言あり (= pre-existing、関数頭で既に 1 回 LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER 宣言済の上に block 内でもう 1 回宣言)。**本 batch では一切触らない** (= 1 method 1 sub-step の射程外 + `feedback_self_bug_no_defer_option` 「他人 bug は別 issue」原則)。Vulkan path は line 2624 `glUniform4fv` 直前に挿入、line 2623 重複行はそのまま温存。

### §2.2 各 method 共通 pattern literal (= PB-4.7 の copy)

各 method の `if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)` block 内、`glUniformXfv` 直前に以下を挿入:

```cpp
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-4.{9|10|11|12}:
// integer index 経路 Vulkan path 分岐追加。spec 06a §5.2 / §5.3 literal 準拠。
// GATE-B = #ifdef LL_VULKAN_GLSL 不使用、mUseUBO runtime flag 単独 gate。
// MUSEUBO-A = mUseUBO=false default で本 block 走らず既存 OpenGL 挙動 100% 維持。
// ptr+count pattern (= count * {1|2|3|4} * sizeof(GLfloat))。
if (mUseUBO)
{
    llassert(index < mUniformUBOLoc.size());
    const ubo::UniformLocation& loc = mUniformUBOLoc[index];
    if (loc.cadence_tag == 0xFFFFFFFFu) return;
    if (loc.cadence_tag == 5 /* CADENCE_SAMPLER */) return;
    forwardToUboUpload(loc, v, count * {1|2|3|4} * sizeof(GLfloat));
    return;
}
```

**method ごとの差異 = 4 箇所のみ**:
1. comment 1 行目の `PB-4.{X}` 番号
2. comment 5 行目の `count * {N} * sizeof(GLfloat)` の N (= 1/2/3/4)
3. `forwardToUboUpload()` の size 計算式の N (= 1/2/3/4)
4. (注: N=1 の場合は `count * 1 * sizeof(GLfloat)` でなく `count * sizeof(GLfloat)` と書く = 冗長な `* 1` を入れない、PB-4.7 と整合)

= 1 method あたり +14 line × 4 = **+56 line 4 method、1 file changed** 想定。

### §2.3 build verify 戦略 (= AYA 提案採用、最後に 1 回)

| step | 内容 |
|---|---|
| (1) | PB-4.9 〜 PB-4.12 を順に Edit (= 4 連 Edit、各 method 独立、merge conflict なし) |
| (2) | `git diff --stat` で +56 line 1 file 確認 |
| (3) | static verify (= GATE-B / MUSEUBO-A / Co-Authored-By 不在 各観点) |
| (4) | **build verify 1 回のみ** = `cmake --build build-linux-x86_64 -j` (= 単体 build スキップ、full viewer 直接、llpackage 到達確認) |
| (5) | 1 commit で全 4 method を batch commit (= 「PB-4.9〜.12 batch = uniform1fv/2fv/3fv/4fv Vulkan path 分岐 4 件一括追加」) |
| (6) | handoff doc 起案 (= PB-4.9〜.12 batch complete handoff) |

**注 (= build 1 回のみの根拠)**: 4 method はすべて既存 redirect 層 (forwardToUboUpload) への call site 追加のみで、`#include` 追加なし、新規 type 導入なし、template specialization なし。compile error が出るとすれば pattern literal の typo 1 箇所が 4 method 全部に同じ症状で出る (= 同一 pattern copy)、build 1 回で全 4 件の typo 検出十分。

---

## §3 本 batch 除外 method (= 別 sub-step で実施)

### §3.1 (U4) 適用 5 method = design 判断要

spec 06b §3.4 **(U4)** で以下 5 method は **mValue cache 適用外** = stage 1 bypass、常に forwardToUboUpload と明記:
- PB-4.8 = `uniform4iv(U32 index, U32 count, const GLint* v)`
- PB-4.14 = `uniformMatrix2fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v)`
- PB-4.15 = `uniformMatrix3fv` (= 同形)
- PB-4.16 = `uniformMatrix3x4fv` (= 同形)
- PB-4.17 = `uniformMatrix4fv` (= 同形)

**設計判断要 1 件**: Vulkan path の挿入位置を **shouldChange/mValue block の内側** (= PB-4.7 整合) or **外側** (= (U4) 整合 = mValue check 通過させずに常に forwardToUboUpload) のどちらにするか。matrix 4 件は元々 mValue check 無し (= `if (mUniform[index] >= 0)` 直下に glUniformMatrixXfv のみ) なので外側挿入で自然、uniform4iv は (U4) 整合なら外側に出す改変必要。AYA 判断待ち。

### §3.2 PB-4.13 uniform4uiv = A と完全同型 GLuint 版

- llglslshader.cpp:2631 から始まる `uniform4uiv(U32 index, U32 count, const GLuint* v)` は A (= PB-4.9〜.12) と完全同型 (型が GLfloat → GLuint、size = `count * 4 * sizeof(GLuint)`)。
- 本 batch から除外した理由 = 「**ptr+count pattern の T=GLuint 系は本 batch (= T=GLfloat 系のみ) と分離して扱う**」整理。設計判断はなし、機械的展開のみ。次 session 着手予定 (= PB-4.13 単独 or PB-4.13 + 後続 batch)。

### §3.3 matrix 系の transpose 引数取扱 (= group C 着手時要確認)

OpenGL `glUniformMatrix4fv(loc, count, transpose, v)` の 3rd 引数で row-major/col-major 切替。Vulkan/SPIR-V std140 は memory layout 固定で transpose 概念不適用 → **Vulkan path では forwardToUboUpload に transpose 渡さず、`v` をそのまま memcpy** が方針整合 (= 呼出側で常に `GL_FALSE` 渡し前提)。

group C 着手前に `grep -rn "glUniformMatrix.*GL_TRUE\|uniformMatrix.*GL_TRUE" indra/` で transpose=GL_TRUE 呼出有無を確認、ゼロ件なら方針確定、1 件でもあれば設計やり直し。

---

## §4 self-verify 観点 (= batch 完了時に確認すべき)

| # | 観点 | 確認手段 |
|---|---|---|
| 1 | +56 line 物理確認 (= 14 × 4) | `git diff --stat` = 1 file changed + 56 insertions |
| 2 | 挿入位置 (= 4 件すべて glUniformXfv 直前) | `git diff` で 4 件すべて `if (mUseUBO)` block が `glUniformXfv` 直前にあること |
| 3 | GATE-B 順守 (= #ifdef LL_VULKAN_GLSL 不使用) | `git diff \| grep "^+" \| grep "LL_VULKAN_GLSL" \| grep -v "^+.*//"` = 0 件 (= comment 中のみ許可) |
| 4 | MUSEUBO-A 順守 (= mUseUBO=false default 維持) | `git diff llglslshader.h` = 0 |
| 5 | static verify §5 全 7 観点 PASS | (1) #ifdef LL_VULKAN_GLSL コード行 0 件 + (2) mUseUBO=true 新規追加 0 件 + (3) Co-Authored-By 0 件 + (4) mUniformUBOLoc[index] 参照 4 件 + (5) mUniformUBOLocByHash.find n/a + (6) forwardToUboUpload 追加 4 件 + (7) glUniformXfv 既存 line 削除 0 件 |
| 6 | working tree = 1 file のみ | `git status` = M indra/llrender/llglslshader.cpp のみ |
| 7 | 残 sub-step strict 線形維持 | §2 / §3 = PB-4.8 + PB-4.13 + PB-4.14〜.17 + PB-5.1〜.13 + PB-7 + PB-N |
| 8 | feedback_ubo_migration_one_at_a_time 射程確認 | 「機械的展開 = call site 追加のみ + 構造変更なし + 設計判断なし」が batch 化条件、本 batch は条件充足、AYA 提案受領済で射程例外確定 |
| 9 | Co-Authored-By 不在 (= commit message) | commit message 行頭 `Co-Authored-By:` 0 件 |
| 10 | build verify 1 回 PASS | (1) full viewer build PASS (= llpackage 到達、tar.xz 生成) |

---

## §5 build verify (= 本 batch 着手後実施予定)

| 段階 | 内容 | 期待結果 |
|---|---|---|
| **唯一の build** | `cmake --build build-linux-x86_64 -j` (= 単体 build スキップ、full viewer 直接) | ✅ PASS (= llpackage 到達、`Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-81586.tar.xz` 生成) |

**llrender 単体 build スキップの根拠**: PB-4.4〜.7 で連続 4 回 llrender 単体 build PASS 観測済、4 method を同 pattern で機械的追加するだけで新規 symbol/header/template 導入なし、llrender 単体段階で出る compile error は full viewer build でも同じく出る → 段階分けの diagnostic 価値ゼロ、効率優先で 1 回。

---

## §6 引き継ぎ済 memory + 射程例外確認

- `project_r41_phase1b_vulkan_host_gate.md` — Phase 1.B 全 sub-step で C++ 側 `#ifdef LL_VULKAN_GLSL` 不使用、runtime `mUseUBO` flag 単独 gate
- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ厳守
- `feedback_no_scope_shrink` — Phase 1.B Exit Criteria literal「30 setter 全部」厳守、本 batch も最終 30 件達成への一里塚
- `feedback_self_verify_before_handoff` — PB-N 検証時 AYA 起動目視前に Claude 3 経路非到達 verify 再走
- `feedback_no_claude_coauthor` — 全 commit 共著行不在
- `feedback_one_step_at_a_time` — strict 線形進行 (本 batch は **1 group 内 4 method 機械展開** で実質 1 step 等価、AYA 提案で射程整合)
- `feedback_ubo_migration_one_at_a_time` — **射程例外運用**:
  - **原則** = GLSL UBO 化 + host C++ redirect 層整備 = 構造変更を伴うものは 1 UBO ずつ
  - **例外条件** = (1) 既存 redirect 層 (= forwardToUboUpload) への call site 追加のみ + (2) 構造変更なし + (3) 設計判断なし + (4) AYA 提案 or 同意 → batch 化可
  - **本 batch 該当性** = (1) ✅ + (2) ✅ + (3) ✅ + (4) ✅ (2026-06-04 AYA 提案)
- `feedback_doubt_self_first` — build error / link fail は AYA 投げ前に Claude root cause 特定
- `feedback_no_auto_commit` — 通常 session では AYA 明示指示後 commit (= 本 session は handoff §7 1 line 自走指示により合致)
- `feedback_remove_verification_logs` — commit 前 LL_INFOS hook 除去
- `feedback_self_bug_no_defer_option` — PB-4.12 既存 pre-existing 二重 LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER は本 batch 修正対象外、別 issue で

---

## §7 次 session 着手 1 line

**「前 session で Phase 1.B PB-4.7 integer index 経路 uniform1iv Vulkan path 分岐追加 完了 (commit `1b54b29d3f`、+14 line) + PB-4.7 complete handoff doc 起案 完了 (commit `0d0a34000f`、+152 line)。本 session = **AYA 提案採用 (2026-06-04)** = **PB-4.9〜.12 batch 一括着手** = `uniform1fv` (llglslshader.cpp:2542) / `uniform2fv` (line 2569) / `uniform3fv` (line 2596) / `uniform4fv` (line 2624) の 4 method すべてに同 pattern で Vulkan path 分岐追加 (= 各 method `glUniformXfv` 直前に `if (mUseUBO) { ... forwardToUboUpload(loc, v, count * N * sizeof(GLfloat)); return; }` 挿入、N = 1/2/3/4)、4 method 連続 Edit → 1 commit → **build verify 1 回のみ** (= full viewer build、llrender 単体 build スキップ)、+56 line 1 file changed、GATE-B 整合 + MUSEUBO-A 整合、06a §5.2 / §5.3 literal、`feedback_ubo_migration_one_at_a_time` 射程例外運用 (= 機械的展開 4 条件充足 + AYA 提案受領)、本 batch から **PB-4.8 (uniform4iv) + PB-4.13 (uniform4uiv) + PB-4.14〜.17 (matrix 4 件) は除外** (= (U4) 適用 5 件は design 判断要、PB-4.13 は GLuint 系別 batch) → 残 PB-4.8 (= U4 design 判断後) + PB-4.13 (= GLuint 単独 or 別 batch) + PB-4.14〜.17 (= U4 + matrix design 判断後) + PB-5.1〜.13 + PB-7 + PB-N strict 線形進行。」**
