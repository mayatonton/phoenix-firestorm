# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-4.1〜PB-4.3 完了

**作成日**: 2026-06-03 (自動進行 tick trig_01Aadosy3szE85V8PWM4H2Q5)
**本 tick 物理 commit**:
- `772b6b985c` (= PB-4.1〜PB-4.3 integer index 経路 3 method batch、+39 line)

**次 session 着手**: **AYA 判断不要 自走** → **PB-4.4 着手** = `uniform2f(U32 index, GLfloat x, GLfloat y)` llglslshader.cpp:2327 に Vulkan path 分岐追加、次 tick も batch 3 (= PB-4.4〜PB-4.6)。

---

## §0 state 一行 summary

η-30 **Phase 1.B PB-4.1〜PB-4.3 完了 state** (= 直前 commit `772b6b985c`):
- **PB-4.1 uniform1i** 完了 = llglslshader.cpp glUniform1i 直前に if (mUseUBO) { ... forwardToUboUpload(loc, &x, sizeof(GLint)); return; } 分岐追加、+13 line
- **PB-4.2 uniform1f** 完了 = llglslshader.cpp glUniform1f 直前に同 pattern + sizeof(GLfloat)、+13 line
- **PB-4.3 fastUniform1f** 完了 = llglslshader.cpp glUniform1f 直前に同 pattern + sizeof(GLfloat)、+13 line (= mValue cache なし、llassert 群直後に分岐挿入)
- **自動進行 batch 3 緩和適用** = 3 method / 1 commit、各 method comment header + spec literal 引用で段階退避路確保
- **残 sub-step**: PB-4.4〜PB-4.17 (14 method) → PB-5.1〜.13 (13 method) → PB-7 → PB-N strict 線形
- **設計判断 0 件新規確定** (= 全 default 採用済、Claude 自走継続可)

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-4.1-4.2-4.3-complete.md`) | 全文 | PB-4.1〜4.3 完了 state + 残 PB-4.4〜.17 順序 + PB-4.4 着手第 1 step |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` | §5.2 + §5.3 (= integer index path 分岐 code shape + 規律表) | PB-4.4〜.17 の pattern source-of-truth |
| 3 | `indra/llrender/llglslshader.cpp:2327-2370` | `uniform2f` 既存実装 + 周辺 | PB-4.4 着手前 挿入位置確定 |

### §1.2 pinpoint Read 用 reference (= 必要時のみ)

| file | 参照箇所 |
|---|---|
| `indra/llrender/llglslshader.h:443-457` | `forwardToUboUpload()` 宣言確認 (= PB-6 追加済) |
| `indra/llrender/llglslshader.h:427-429` | `mUseUBO = false` default 確認 (= MUSEUBO-A) |
| PB-6 complete handoff | §3.1 visibility=private + §3.2 配置場所設計判断 参照 |

---

## §2 残 sub-step strict 線形構成 (= PB-6 complete handoff §2 から継承 + PB-4.1〜4.3 完了反映)

| PB-X | 内容 | 物理改変 file | 状態 |
|---|---|---|---|
| ~~PB-1~~ | ~~mUniformUBOLoc cache 構造実装~~ | llglslshader.h | **✅ 完了** (commit `0ba743463c`) |
| ~~PB-2~~ | ~~mapUniforms() integer index 経路 cache 構築~~ | llglslshader.cpp | **✅ 完了** (commit `e43d93dd25`) |
| ~~PB-3~~ | ~~LLStaticHashedString 経路 cache 構築 (S1-C)~~ | llglslshader.cpp | **✅ 完了** (commit `127d25ecb6`) |
| ~~PB-6~~ | ~~forwardToUboUpload() shell 実装 (FWD-1)~~ | llglslshader.h + .cpp | **✅ 完了** (commit `79966ad2f3`) |
| ~~PB-4.1~~ | ~~uniform1i(U32 index, GLint x) Vulkan path 分岐~~ | llglslshader.cpp | **✅ 完了** (commit `772b6b985c`) |
| ~~PB-4.2~~ | ~~uniform1f(U32 index, GLfloat x) Vulkan path 分岐~~ | llglslshader.cpp | **✅ 完了** (commit `772b6b985c`) |
| ~~PB-4.3~~ | ~~fastUniform1f(U32 index, GLfloat x) Vulkan path 分岐~~ | llglslshader.cpp | **✅ 完了** (commit `772b6b985c`) |
| **PB-4.4** | uniform2f(U32 index, GLfloat x, GLfloat y) Vulkan path 分岐 = tmp[] = {x,y} 一時 array + forwardToUboUpload(loc, &tmp[0], 2*sizeof(GLfloat)) | llglslshader.cpp | 次 tick 着手 |
| **PB-4.5** | uniform3f(U32 index, GLfloat x, GLfloat y, GLfloat z) = 同上 3 要素 | llglslshader.cpp | |
| **PB-4.6** | uniform4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) = 同上 4 要素 | llglslshader.cpp | |
| **PB-4.7** | uniform1iv(U32 index, U32 count, const GLint* v) = forwardToUboUpload(loc, v, count*sizeof(GLint)) | llglslshader.cpp | |
| **PB-4.8** | uniform4iv(U32 index, U32 count, const GLint* v) = count*4*sizeof(GLint) | llglslshader.cpp | |
| **PB-4.9** | uniform1fv(U32 index, U32 count, const GLfloat* v) | llglslshader.cpp | |
| **PB-4.10** | uniform2fv(U32 index, U32 count, const GLfloat* v) = count*2*sizeof(GLfloat) | llglslshader.cpp | |
| **PB-4.11** | uniform3fv(U32 index, U32 count, const GLfloat* v) = count*3*sizeof(GLfloat) | llglslshader.cpp | |
| **PB-4.12** | uniform4fv(U32 index, U32 count, const GLfloat* v) = count*4*sizeof(GLfloat) | llglslshader.cpp | |
| **PB-4.13** | uniform4uiv (= grep 実装確認要) | llglslshader.cpp | |
| **PB-4.14** | uniformMatrix2fv(U32 index, ...) = count*4*sizeof(GLfloat) | llglslshader.cpp | |
| **PB-4.15** | uniformMatrix3fv(U32 index, ...) = count*9*sizeof(GLfloat) | llglslshader.cpp | |
| **PB-4.16** | uniformMatrix3x4fv(U32 index, ...) = count*12*sizeof(GLfloat) | llglslshader.cpp | |
| **PB-4.17** | uniformMatrix4fv(U32 index, ...) = count*16*sizeof(GLfloat) | llglslshader.cpp | |
| **PB-5.1〜.13** | 13 method LLStaticHashedString 経路 Vulkan path 分岐 | llglslshader.cpp | PB-4.17 完了後 |
| **PB-7** | mapUniforms() 末尾 debug build llassert 整合 check | llglslshader.cpp | |
| **PB-N** | Phase 1.B Exit Criteria 検証 (= ローカル動作確認、自動進行 scope 外) | — | AYA 起床後実行 |

**注 (= PB-4.4〜4.6 uniform2f/3f/4f)**: scalar 引数から ptr 生成が必要 = `const GLfloat tmp[] = {x, y};` 等の一時 array 作成後 `&tmp[0]` を渡す (= tick-prompt §4 type/size 早見表「uniform2f / uniform3f / uniform4f の data ptr 注意」準拠)。

---

## §3 本 tick 新規確定 設計判断

**設計判断 0 件** (= 全 default 採用済、PB-4.1〜4.3 は PB-6 complete handoff §6 の code shape literal そのままで適用、AYA 確認不要)。

---

## §4 self-verify 9 観点 PASS

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) +39 line 物理確認 | `git diff --stat` = 1 file changed + 39 insertions | ✅ |
| (2) 各 method 挿入位置 | glUniform 直前 (= 既存 OpenGL path より前に Vulkan path 分岐) | ✅ |
| (3) GATE-B 順守 | `git diff \| grep "^+" \| grep "#ifdef LL_VULKAN_GLSL" \| grep -v "^+.*//"`  = 0 件 (comment 中のみ) | ✅ |
| (4) MUSEUBO-A 順守 | `git diff llglslshader.h` = 0 (= mUseUBO=false default 維持) | ✅ |
| (5) static verify §5 全 7 観点 | (1〜7) 全 PASS | ✅ |
| (6) 残 sub-step strict 線形維持 | §2 table = PB-4.4 → ... → PB-N | ✅ |
| (7) batch 3 緩和 + revert 容易性 | commit message + 各 method comment header で明記 | ✅ |
| (8) working tree = 1 file のみ | `git status` = M indra/llrender/llglslshader.cpp のみ | ✅ |
| (9) Co-Authored-By 不在 | commit message に Co-Authored-By: Claude 行なし | ✅ |

---

## §5 引き継ぎ済 memory (= PB-6 complete handoff 継承、追加なし)

- `project_r41_phase1b_vulkan_host_gate.md` — Phase 1.B 全 sub-step で C++ 側 `#ifdef LL_VULKAN_GLSL` 不使用、runtime `mUseUBO` flag 単独 gate
- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ厳守
- `feedback_no_scope_shrink` — Phase 1.B Exit Criteria literal「30 setter 全部」厳守
- `feedback_self_verify_before_handoff` — PB-N 検証時 AYA 起動目視前に Claude 3 経路非到達 verify 再走
- `feedback_no_claude_coauthor` — 全 commit 共著行不在
- `feedback_one_step_at_a_time` — strict 線形進行
- `feedback_ubo_migration_one_at_a_time` — 1 method 1 sub-step (= 自動進行期間中 batch 3 緩和適用中)
- `feedback_doubt_self_first` — build error / link fail は AYA 投げ前に Claude root cause 特定
- `feedback_no_auto_commit` — 通常 session では AYA 明示指示後 commit (= 自動進行 tick は例外)
- `feedback_remove_verification_logs` — commit 前 LL_INFOS hook 除去
- `project_ayastorm_r41_vulkan_migration` — Phase 1.B PB-4.1〜4.3 完了

---

## §6 次 session 着手 1 line

**「前 session (= 自動進行 tick) で Phase 1.B PB-4.1〜PB-4.3 integer index 経路 3 method (uniform1i / uniform1f / fastUniform1f) Vulkan path 分岐追加 完了 (commit `772b6b985c`、+39 line)。本 session (= 次 tick) = **AYA 判断不要 自走** = **PB-4.4〜PB-4.6 batch 着手** = `uniform2f(U32 index, GLfloat x, GLfloat y)` (llglslshader.cpp:2327) + `uniform3f` + `uniform4f` の 3 method に同 pattern Vulkan path 分岐追加 (= scalar 引数 → 一時 array `const GLfloat tmp[] = {x,y,...};` + `forwardToUboUpload(loc, &tmp[0], N*sizeof(GLfloat))`)、GATE-B 整合 + MUSEUBO-A 整合、06a §5.2 / §5.3 literal → 残 PB-4.7〜.17 → PB-5.1〜.13 → PB-7 → PB-N strict 線形進行。」**
