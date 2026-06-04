# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-4.6 完了

**作成日**: 2026-06-04
**本 session 物理 commit**:
- `62c7a95caa` (= PB-4.6 uniform4f integer index 経路 Vulkan path 分岐、+15 line)

**次 session 着手**: **AYA 判断不要 自走** → **PB-4.7 着手** = `uniform1iv(U32 index, U32 count, const GLint* v)` llglslshader.cpp:2453 に Vulkan path 分岐追加 (= **ptr 引数 + count 乗算 size pattern** = `forwardToUboUpload(loc, v, count * sizeof(GLint))` で ptr 直接渡し、PB-4.4〜.6 の scalar→tmp[N] pattern とは別系統)、1 method 1 sub-step strict 線形。

---

## §0 state 一行 summary

η-30 **Phase 1.B PB-4.6 完了 state** (= 直前 commit `62c7a95caa`):
- **PB-4.6 uniform4f** 完了 = llglslshader.cpp glUniform4f 直前に if (mUseUBO) { ... GLfloat tmp[4]={x,y,z,w}; forwardToUboUpload(loc, tmp, sizeof(tmp)); return; } 分岐追加、+15 line
- **1 method 1 sub-step strict 線形維持** = `feedback_ubo_migration_one_at_a_time` 順守、batch 3 緩和終了後 PB-4.4 (uniform2f) / PB-4.5 (uniform3f) に続く 3 件目
- **残 sub-step**: PB-4.7〜PB-4.17 (11 method) → PB-5.1〜.13 (13 method) → PB-7 → PB-N strict 線形
- **設計判断 0 件新規確定** (= 全 default 採用済、Claude 自走継続可)
- **scalar→tmp[N] pattern 3 method 完走** (= uniform2f/3f/4f)、**次は ptr+count pattern 8 method block 開始** (= uniform1iv/4iv/1fv/2fv/3fv/4fv/4uiv + matrix 4 method)

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-4.6-complete.md`) | 全文 | PB-4.6 完了 state + 残 PB-4.7〜.17 順序 + PB-4.7 着手第 1 step + **ptr+count pattern 新系統の literal** |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` | §5.2 + §5.3 (= integer index path 分岐 code shape + 規律表) | PB-4.7〜.17 の pattern source-of-truth |
| 3 | `indra/llrender/llglslshader.cpp:2453-2478` | `uniform1iv` 既存実装 + 周辺 | PB-4.7 着手前 挿入位置確定 |

### §1.2 pinpoint Read 用 reference (= 必要時のみ)

| file | 参照箇所 |
|---|---|
| `indra/llrender/llglslshader.cpp:2411-2451` | PB-4.6 (uniform4f) Vulkan path 分岐 pattern (= 直前 commit、scalar→tmp[N] pattern 終端、PB-4.7 は別 pattern なので参考度低) |
| `indra/llrender/llglslshader.h:443-457` | `forwardToUboUpload()` 宣言確認 (= PB-6 追加済、ptr 引数 byte size 受領) |
| `indra/llrender/llglslshader.h:427-429` | `mUseUBO = false` default 確認 (= MUSEUBO-A) |

---

## §2 残 sub-step strict 線形構成 (= PB-4.6 完了反映)

| PB-X | 内容 | 物理改変 file | 状態 |
|---|---|---|---|
| ~~PB-1~~ | ~~mUniformUBOLoc cache 構造実装~~ | llglslshader.h | **✅ 完了** (commit `0ba743463c`) |
| ~~PB-2~~ | ~~mapUniforms() integer index 経路 cache 構築~~ | llglslshader.cpp | **✅ 完了** (commit `e43d93dd25`) |
| ~~PB-3~~ | ~~LLStaticHashedString 経路 cache 構築 (S1-C)~~ | llglslshader.cpp | **✅ 完了** (commit `127d25ecb6`) |
| ~~PB-6~~ | ~~forwardToUboUpload() shell 実装 (FWD-1)~~ | llglslshader.h + .cpp | **✅ 完了** (commit `79966ad2f3`) |
| ~~PB-4.1~~ | ~~uniform1i Vulkan path 分岐~~ | llglslshader.cpp | **✅ 完了** (commit `772b6b985c`) |
| ~~PB-4.2~~ | ~~uniform1f Vulkan path 分岐~~ | llglslshader.cpp | **✅ 完了** (commit `772b6b985c`) |
| ~~PB-4.3~~ | ~~fastUniform1f Vulkan path 分岐~~ | llglslshader.cpp | **✅ 完了** (commit `772b6b985c`) |
| ~~PB-4.4~~ | ~~uniform2f Vulkan path 分岐~~ | llglslshader.cpp | **✅ 完了** (commit `71576ff1e4`) |
| ~~PB-4.5~~ | ~~uniform3f Vulkan path 分岐~~ | llglslshader.cpp | **✅ 完了** (commit `257df5e757`) |
| ~~PB-4.6~~ | ~~uniform4f Vulkan path 分岐~~ | llglslshader.cpp | **✅ 完了** (commit `62c7a95caa`) |
| **PB-4.7** | uniform1iv(U32 index, U32 count, const GLint* v) Vulkan path 分岐 = **ptr+count pattern** = forwardToUboUpload(loc, v, count * sizeof(GLint)) | llglslshader.cpp | 次 session 着手 |
| **PB-4.8** | uniform4iv(U32 index, U32 count, const GLint* v) = count * 4 * sizeof(GLint) | llglslshader.cpp | |
| **PB-4.9** | uniform1fv(U32 index, U32 count, const GLfloat* v) = count * sizeof(GLfloat) | llglslshader.cpp | |
| **PB-4.10** | uniform2fv(U32 index, U32 count, const GLfloat* v) = count * 2 * sizeof(GLfloat) | llglslshader.cpp | |
| **PB-4.11** | uniform3fv(U32 index, U32 count, const GLfloat* v) = count * 3 * sizeof(GLfloat) | llglslshader.cpp | |
| **PB-4.12** | uniform4fv(U32 index, U32 count, const GLfloat* v) = count * 4 * sizeof(GLfloat) | llglslshader.cpp | |
| **PB-4.13** | uniform4uiv (= grep 実装確認要、GLuint* v + count * 4 * sizeof(GLuint)) | llglslshader.cpp | |
| **PB-4.14** | uniformMatrix2fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v) = count * 4 * sizeof(GLfloat) | llglslshader.cpp | |
| **PB-4.15** | uniformMatrix3fv = count * 9 * sizeof(GLfloat) | llglslshader.cpp | |
| **PB-4.16** | uniformMatrix3x4fv = count * 12 * sizeof(GLfloat) | llglslshader.cpp | |
| **PB-4.17** | uniformMatrix4fv = count * 16 * sizeof(GLfloat) | llglslshader.cpp | |
| **PB-5.1〜.13** | 13 method LLStaticHashedString 経路 Vulkan path 分岐 | llglslshader.cpp | PB-4.17 完了後 |
| **PB-7** | mapUniforms() 末尾 debug build llassert 整合 check | llglslshader.cpp | |
| **PB-N** | Phase 1.B Exit Criteria 検証 (= ローカル動作確認) | — | AYA と一緒に実行 |

**注 (= PB-4.7 uniform1iv)**: **PB-4.4〜.6 scalar→tmp[N] pattern とは別系統**。引数の `const GLint* v` は既に ptr なので、tmp array 作らず直接 `forwardToUboUpload(loc, v, count * sizeof(GLint))` で渡せる。size は `count * sizeof(GLint)` (= 動的、`sizeof(tmp)` のような静的取得不可、`count` も含めた明示乗算)。PB-4.8〜.17 すべて同 pattern (= ptr + count + 要素 byte size 乗算)。

---

## §3 本 session 新規確定 設計判断

**設計判断 0 件** (= 全 default 採用済、PB-4.6 は PB-4.5 (uniform3f) の code shape literal そのままで N=3→4 適用、AYA 確認不要)。

**次 session PB-4.7 用 pattern literal** (= ptr+count + count*sizeof(T) 乗算):
```cpp
if (mUseUBO)
{
    llassert(index < mUniformUBOLoc.size());
    const ubo::UniformLocation& loc = mUniformUBOLoc[index];
    if (loc.cadence_tag == 0xFFFFFFFFu) return;
    if (loc.cadence_tag == 5 /* CADENCE_SAMPLER */) return;
    forwardToUboUpload(loc, v, count * sizeof(GLint));
    return;
}
```
(= scalar→tmp[N] pattern と diff は 1 行: `GLfloat tmp[4] = {x, y, z, w}; forwardToUboUpload(loc, tmp, sizeof(tmp));` → `forwardToUboUpload(loc, v, count * sizeof(GLint));`、6 行構造維持、可読性同等)

---

## §4 self-verify 9 観点 PASS

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) +15 line 物理確認 | `git diff --stat` = 1 file changed + 15 insertions | ✅ |
| (2) 挿入位置 | glUniform4f 直前 (= 既存 OpenGL path より前に Vulkan path 分岐) | ✅ |
| (3) GATE-B 順守 | `git diff \| grep "^+" \| grep "#ifdef LL_VULKAN_GLSL" \| grep -v "^+.*//"` = 0 件 (comment 中のみ 1 件) | ✅ |
| (4) MUSEUBO-A 順守 | `git diff llglslshader.h` = 0 (= mUseUBO=false default 維持) | ✅ |
| (5) static verify §5 全 7 観点 | (1〜7) 全 PASS | ✅ |
| (6) 残 sub-step strict 線形維持 | §2 table = PB-4.7 → ... → PB-N | ✅ |
| (7) 1 method 1 sub-step 順守 | `feedback_ubo_migration_one_at_a_time` 順守、batch 3 緩和終了後 3 件目 | ✅ |
| (8) working tree = 1 file のみ | `git status` = M indra/llrender/llglslshader.cpp のみ | ✅ |
| (9) Co-Authored-By 不在 | commit message 行頭 `Co-Authored-By:` 0 件 (本文中の自己 verify 言及のみ) | ✅ |

---

## §5 build verify (= 本 session 実施結果)

| 段階 | 確認 | 結果 |
|---|---|---|
| (1) llrender 単体 build | `cmake --build build-linux-x86_64 --target llrender -j` | ✅ PASS (libllrender.a link) |
| (2) full viewer build | `cmake --build build-linux-x86_64 -j` | ✅ PASS (llpackage 到達、`Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-81586.tar.xz` 生成) |

---

## §6 引き継ぎ済 memory (= PB-4.5 handoff 継承、追加なし)

- `project_r41_phase1b_vulkan_host_gate.md` — Phase 1.B 全 sub-step で C++ 側 `#ifdef LL_VULKAN_GLSL` 不使用、runtime `mUseUBO` flag 単独 gate
- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ厳守
- `feedback_no_scope_shrink` — Phase 1.B Exit Criteria literal「30 setter 全部」厳守
- `feedback_self_verify_before_handoff` — PB-N 検証時 AYA 起動目視前に Claude 3 経路非到達 verify 再走
- `feedback_no_claude_coauthor` — 全 commit 共著行不在
- `feedback_one_step_at_a_time` — strict 線形進行
- `feedback_ubo_migration_one_at_a_time` — **1 method 1 sub-step (= batch 3 緩和終了後 PB-4.4 / PB-4.5 / PB-4.6 で 3 件連続順守)**
- `feedback_doubt_self_first` — build error / link fail は AYA 投げ前に Claude root cause 特定
- `feedback_no_auto_commit` — 通常 session では AYA 明示指示後 commit (= 本 session は handoff §6 1 line 自走指示により合致)
- `feedback_remove_verification_logs` — commit 前 LL_INFOS hook 除去
- `project_ayastorm_r41_vulkan_migration` — Phase 1.B PB-4.6 完了

---

## §7 次 session 着手 1 line

**「前 session で Phase 1.B PB-4.6 integer index 経路 uniform4f Vulkan path 分岐追加 完了 (commit `62c7a95caa`、+15 line)。本 session = **AYA 判断不要 自走** = **PB-4.7 単独着手** = `uniform1iv(U32 index, U32 count, const GLint* v)` (llglslshader.cpp:2453) に Vulkan path 分岐追加 (= **ptr+count pattern 新系統初出** = `forwardToUboUpload(loc, v, count * sizeof(GLint))` で ptr 直接渡し + count*sizeof(GLint) 動的 size 計算、PB-4.4〜.6 の scalar→tmp[N] pattern とは別系統)、挿入位置 = `glUniform1iv(mUniform[index], count, v);` (line 2473) 直前、GATE-B 整合 + MUSEUBO-A 整合、06a §5.2 / §5.3 literal、1 method 1 sub-step strict 線形 (= `feedback_ubo_migration_one_at_a_time` 順守、batch 3 緩和終了後 PB-4.4 / PB-4.5 / PB-4.6 に続く 4 件目) → 残 PB-4.8〜.17 → PB-5.1〜.13 → PB-7 → PB-N strict 線形進行。」**
