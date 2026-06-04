# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-4.5 完了

**作成日**: 2026-06-04
**本 session 物理 commit**:
- `257df5e757` (= PB-4.5 uniform3f integer index 経路 Vulkan path 分岐、+15 line)

**次 session 着手**: **AYA 判断不要 自走** → **PB-4.6 着手** = `uniform4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)` llglslshader.cpp:2411 に Vulkan path 分岐追加 (= scalar 引数 → 一時 array `GLfloat tmp[4] = {x, y, z, w};` + `forwardToUboUpload(loc, tmp, sizeof(tmp))`)、1 method 1 sub-step strict 線形。

---

## §0 state 一行 summary

η-30 **Phase 1.B PB-4.5 完了 state** (= 直前 commit `257df5e757`):
- **PB-4.5 uniform3f** 完了 = llglslshader.cpp glUniform3f 直前に if (mUseUBO) { ... GLfloat tmp[3]={x,y,z}; forwardToUboUpload(loc, tmp, sizeof(tmp)); return; } 分岐追加、+15 line
- **1 method 1 sub-step strict 線形維持** = `feedback_ubo_migration_one_at_a_time` 順守、batch 3 緩和終了後 PB-4.4 (uniform2f) に続く 2 件目
- **残 sub-step**: PB-4.6〜PB-4.17 (12 method) → PB-5.1〜.13 (13 method) → PB-7 → PB-N strict 線形
- **設計判断 0 件新規確定** (= 全 default 採用済、Claude 自走継続可)

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-4.5-complete.md`) | 全文 | PB-4.5 完了 state + 残 PB-4.6〜.17 順序 + PB-4.6 着手第 1 step |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` | §5.2 + §5.3 (= integer index path 分岐 code shape + 規律表) | PB-4.6〜.17 の pattern source-of-truth |
| 3 | `indra/llrender/llglslshader.cpp:2411-2435` | `uniform4f` 既存実装 + 周辺 | PB-4.6 着手前 挿入位置確定 |

### §1.2 pinpoint Read 用 reference (= 必要時のみ)

| file | 参照箇所 |
|---|---|
| `indra/llrender/llglslshader.cpp:2369-2410` | PB-4.5 (uniform3f) Vulkan path 分岐 pattern (= 直前 commit、PB-4.6 pattern source) |
| `indra/llrender/llglslshader.cpp:2327-2367` | PB-4.4 (uniform2f) Vulkan path 分岐 pattern (= 補助 reference、tmp[N] size 差異確認) |
| `indra/llrender/llglslshader.h:443-457` | `forwardToUboUpload()` 宣言確認 (= PB-6 追加済) |
| `indra/llrender/llglslshader.h:427-429` | `mUseUBO = false` default 確認 (= MUSEUBO-A) |

---

## §2 残 sub-step strict 線形構成 (= PB-4.5 完了反映)

| PB-X | 内容 | 物理改変 file | 状態 |
|---|---|---|---|
| ~~PB-1~~ | ~~mUniformUBOLoc cache 構造実装~~ | llglslshader.h | **✅ 完了** (commit `0ba743463c`) |
| ~~PB-2~~ | ~~mapUniforms() integer index 経路 cache 構築~~ | llglslshader.cpp | **✅ 完了** (commit `e43d93dd25`) |
| ~~PB-3~~ | ~~LLStaticHashedString 経路 cache 構築 (S1-C)~~ | llglslshader.cpp | **✅ 完了** (commit `127d25ecb6`) |
| ~~PB-6~~ | ~~forwardToUboUpload() shell 実装 (FWD-1)~~ | llglslshader.h + .cpp | **✅ 完了** (commit `79966ad2f3`) |
| ~~PB-4.1~~ | ~~uniform1i Vulkan path 分岐~~ | llglslshader.cpp | **✅ 完了** (commit `772b6b985c`) |
| ~~PB-4.2~~ | ~~uniform1f Vulkan path 分岐~~ | llglslshader.cpp | **✅ 完了** (commit `772b6b985c`) |
| ~~PB-4.3~~ | ~~fastUniform1f Vulkan path 分岐~~ | llglslshader.cpp | **✅ 完了** (commit `772b6b985c`) |
| ~~PB-4.4~~ | ~~uniform2f(U32 index, GLfloat x, GLfloat y) Vulkan path 分岐~~ | llglslshader.cpp | **✅ 完了** (commit `71576ff1e4`) |
| ~~PB-4.5~~ | ~~uniform3f(U32 index, GLfloat x, GLfloat y, GLfloat z) Vulkan path 分岐~~ | llglslshader.cpp | **✅ 完了** (commit `257df5e757`) |
| **PB-4.6** | uniform4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) Vulkan path 分岐 = tmp[4]={x,y,z,w} 一時 array + forwardToUboUpload(loc, tmp, sizeof(tmp)) | llglslshader.cpp | 次 session 着手 |
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
| **PB-N** | Phase 1.B Exit Criteria 検証 (= ローカル動作確認) | — | AYA と一緒に実行 |

**注 (= PB-4.6 uniform4f)**: scalar 引数から ptr 生成 = `GLfloat tmp[4] = {x, y, z, w};` 後 `tmp` を渡す (= sizeof(tmp) = 4*sizeof(GLfloat) 自動算出)。PB-4.4 / PB-4.5 と同 pattern、tmp[N] の N と sizeof のみ差異。

---

## §3 本 session 新規確定 設計判断

**設計判断 0 件** (= 全 default 採用済、PB-4.5 は PB-4.4 (uniform2f) の code shape literal そのままで N=2→3 適用、AYA 確認不要)。

scalar 引数 → 一時 array pattern の literal (= PB-4.6 uniform4f 用):
```cpp
GLfloat tmp[4] = {x, y, z, w};
forwardToUboUpload(loc, tmp, sizeof(tmp));
```
(= `sizeof(tmp) = 4*sizeof(GLfloat)` で N 要素を明示せずに済む = `&tmp[0]` よりも array decay 経路で書きやすい)。

---

## §4 self-verify 9 観点 PASS

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) +15 line 物理確認 | `git diff --stat` = 1 file changed + 15 insertions | ✅ |
| (2) 挿入位置 | glUniform3f 直前 (= 既存 OpenGL path より前に Vulkan path 分岐) | ✅ |
| (3) GATE-B 順守 | `git diff \| grep "^+" \| grep "#ifdef LL_VULKAN_GLSL" \| grep -v "^+.*//"`  = 0 件 (comment 中のみ 1 件) | ✅ |
| (4) MUSEUBO-A 順守 | `git diff llglslshader.h` = 0 (= mUseUBO=false default 維持) | ✅ |
| (5) static verify §5 全 7 観点 | (1〜7) 全 PASS | ✅ |
| (6) 残 sub-step strict 線形維持 | §2 table = PB-4.6 → ... → PB-N | ✅ |
| (7) 1 method 1 sub-step 順守 | `feedback_ubo_migration_one_at_a_time` 順守、batch 3 緩和終了後 2 件目 | ✅ |
| (8) working tree = 1 file のみ | `git status` = M indra/llrender/llglslshader.cpp のみ | ✅ |
| (9) Co-Authored-By 不在 | commit message に Co-Authored-By: Claude 行なし | ✅ |

---

## §5 引き継ぎ済 memory (= PB-4.4 handoff 継承、追加なし)

- `project_r41_phase1b_vulkan_host_gate.md` — Phase 1.B 全 sub-step で C++ 側 `#ifdef LL_VULKAN_GLSL` 不使用、runtime `mUseUBO` flag 単独 gate
- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ厳守
- `feedback_no_scope_shrink` — Phase 1.B Exit Criteria literal「30 setter 全部」厳守
- `feedback_self_verify_before_handoff` — PB-N 検証時 AYA 起動目視前に Claude 3 経路非到達 verify 再走
- `feedback_no_claude_coauthor` — 全 commit 共著行不在
- `feedback_one_step_at_a_time` — strict 線形進行
- `feedback_ubo_migration_one_at_a_time` — **1 method 1 sub-step (= batch 3 緩和終了後 PB-4.4 / PB-4.5 で 2 件連続順守)**
- `feedback_doubt_self_first` — build error / link fail は AYA 投げ前に Claude root cause 特定
- `feedback_no_auto_commit` — 通常 session では AYA 明示指示後 commit (= 本 session は handoff §6 1 line 自走指示により合致)
- `feedback_remove_verification_logs` — commit 前 LL_INFOS hook 除去
- `project_ayastorm_r41_vulkan_migration` — Phase 1.B PB-4.5 完了

---

## §6 次 session 着手 1 line

**「前 session で Phase 1.B PB-4.5 integer index 経路 uniform3f Vulkan path 分岐追加 完了 (commit `257df5e757`、+15 line)。本 session = **AYA 判断不要 自走** = **PB-4.6 単独着手** = `uniform4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)` (llglslshader.cpp:2411) に同 pattern Vulkan path 分岐追加 (= scalar 引数 → 一時 array `GLfloat tmp[4] = {x, y, z, w};` + `forwardToUboUpload(loc, tmp, sizeof(tmp))`)、GATE-B 整合 + MUSEUBO-A 整合、06a §5.2 / §5.3 literal、1 method 1 sub-step strict 線形 (= `feedback_ubo_migration_one_at_a_time` 順守、batch 3 緩和終了後 PB-4.4 / PB-4.5 に続く 3 件目) → 残 PB-4.7〜.17 → PB-5.1〜.13 → PB-7 → PB-N strict 線形進行。」**
