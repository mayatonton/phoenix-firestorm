# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-4.7 完了

**作成日**: 2026-06-04
**本 session 物理 commit**:
- `1b54b29d3f` (= PB-4.7 uniform1iv integer index 経路 Vulkan path 分岐、+14 line)

**次 session 着手**: **AYA 判断不要 自走** → **PB-4.8 着手** = `uniform4iv(U32 index, U32 count, const GLint* v)` llglslshader.cpp:2514 に Vulkan path 分岐追加 (= **ptr+count pattern 2 件目** = `forwardToUboUpload(loc, v, count * 4 * sizeof(GLint))` で ptr 直接渡し + 動的 size = `count * 4 * sizeof(GLint)`、PB-4.7 (uniform1iv) と同系統、要素 byte size のみ 1→4 倍)、1 method 1 sub-step strict 線形。

---

## §0 state 一行 summary

η-30 **Phase 1.B PB-4.7 完了 state** (= 直前 commit `1b54b29d3f`):
- **PB-4.7 uniform1iv** 完了 = llglslshader.cpp glUniform1iv 直前に if (mUseUBO) { ... forwardToUboUpload(loc, v, count * sizeof(GLint)); return; } 分岐追加、+14 line
- **ptr+count pattern 新系統初出** = tmp array 不要、`v` を直接渡し + `count*sizeof(T)` 動的 size 計算 (= PB-4.4〜.6 scalar→tmp[N] +15 line から tmp 行削除で +14 line)
- **1 method 1 sub-step strict 線形維持** = `feedback_ubo_migration_one_at_a_time` 順守、batch 3 緩和終了後 PB-4.4 / PB-4.5 / PB-4.6 / PB-4.7 で 4 件連続順守
- **残 sub-step**: PB-4.8〜PB-4.17 (10 method) → PB-5.1〜.13 (13 method) → PB-7 → PB-N strict 線形
- **設計判断 0 件新規確定** (= 全 default 採用済、Claude 自走継続可)
- **ptr+count pattern block 開始済**、残 9 件 (= PB-4.8 uniform4iv / PB-4.9 uniform1fv / PB-4.10 uniform2fv / PB-4.11 uniform3fv / PB-4.12 uniform4fv / PB-4.13 uniform4uiv / PB-4.14 uniformMatrix2fv / PB-4.15 uniformMatrix3fv / PB-4.16 uniformMatrix3x4fv / PB-4.17 uniformMatrix4fv = 11 件)

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-4.7-complete.md`) | 全文 | PB-4.7 完了 state + 残 PB-4.8〜.17 順序 + PB-4.8 着手第 1 step + **ptr+count pattern 同系統 4-component 倍率の literal** |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` | §5.2 + §5.3 (= integer index path 分岐 code shape + 規律表) | PB-4.8〜.17 の pattern source-of-truth |
| 3 | `indra/llrender/llglslshader.cpp:2494-2519` | `uniform4iv` 既存実装 + 周辺 | PB-4.8 着手前 挿入位置確定 |

### §1.2 pinpoint Read 用 reference (= 必要時のみ)

| file | 参照箇所 |
|---|---|
| `indra/llrender/llglslshader.cpp:2453-2491` | PB-4.7 (uniform1iv) Vulkan path 分岐 pattern (= 直前 commit、ptr+count pattern 同系統初出、PB-4.8 は **要素 byte size のみ 1→4 倍**、構造完全一致) |
| `indra/llrender/llglslshader.h:443-457` | `forwardToUboUpload()` 宣言確認 (= PB-6 追加済、ptr 引数 byte size 受領) |
| `indra/llrender/llglslshader.h:427-429` | `mUseUBO = false` default 確認 (= MUSEUBO-A) |

---

## §2 残 sub-step strict 線形構成 (= PB-4.7 完了反映)

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
| ~~PB-4.7~~ | ~~uniform1iv(U32 index, U32 count, const GLint* v) Vulkan path 分岐 = ptr+count pattern 初出~~ | llglslshader.cpp | **✅ 完了** (commit `1b54b29d3f`) |
| **PB-4.8** | uniform4iv(U32 index, U32 count, const GLint* v) Vulkan path 分岐 = **ptr+count pattern 2 件目** = forwardToUboUpload(loc, v, count * 4 * sizeof(GLint)) | llglslshader.cpp | 次 session 着手 |
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

**注 (= PB-4.8 uniform4iv)**: **PB-4.7 (uniform1iv) と完全同型**、ptr+count pattern の **2 件目**。引数の `const GLint* v` は既に ptr (= tmp array 作らず直接渡し)、唯一の差は size 計算: `count * sizeof(GLint)` → `count * 4 * sizeof(GLint)` (= 1 component → 4 component)。PB-4.9〜.12 (uniform[1-4]fv) も同型で sizeof(GLfloat) 倍率変化、PB-4.13 (uniform4uiv) は sizeof(GLuint) で 4 倍、PB-4.14〜.17 (matrix 系) も同型で要素数差異のみ。

**注 (= 既存 OpenGL path の特異性、修正対象外)**: llglslshader.cpp:2514 の uniform4iv 内 OpenGL call は `glUniform1iv(mUniform[index], count, v);` を呼んでいる (= 関数名 4iv に対し glUniform**1**iv、pre-existing)。**PB-4.8 は Vulkan path 追加のみ、この既存 OpenGL line には触れない** (= 1 method 1 sub-step strict 線形 + `feedback_no_scope_shrink` 逆向きの「不必要に scope を広げない」原則、`feedback_self_bug_no_defer_option` も「他人 bug」は別 issue で扱う想定)。Vulkan path 側は 4-component API contract 通り `count * 4 * sizeof(GLint)` で渡す。

---

## §3 本 session 新規確定 設計判断

**設計判断 0 件** (= 全 default 採用済、PB-4.7 は ptr+count pattern を 06a §5.2 / §5.3 literal の `forwardToUboUpload(loc, &x, sizeof(GLfloat))` から `forwardToUboUpload(loc, v, count * sizeof(GLint))` へ自然展開、AYA 確認不要)。

**次 session PB-4.8 用 pattern literal** (= ptr+count + 4 component byte size 乗算):
```cpp
if (mUseUBO)
{
    llassert(index < mUniformUBOLoc.size());
    const ubo::UniformLocation& loc = mUniformUBOLoc[index];
    if (loc.cadence_tag == 0xFFFFFFFFu) return;
    if (loc.cadence_tag == 5 /* CADENCE_SAMPLER */) return;
    forwardToUboUpload(loc, v, count * 4 * sizeof(GLint));
    return;
}
```
(= PB-4.7 (uniform1iv) pattern と diff は 1 トークン: `count * sizeof(GLint)` → `count * 4 * sizeof(GLint)`、構造 6 行完全一致、可読性同等)

**comment header** (= PB-4.7 と同型、`PB-4.7` を `PB-4.8` に、5 行目を「ptr+count pattern 2 件目 = 4-component (= sizeof(GLint) → 4*sizeof(GLint))」に書換):
```cpp
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-4.8:
// integer index 経路 Vulkan path 分岐追加。spec 06a §5.2 / §5.3 literal 準拠。
// GATE-B = #ifdef LL_VULKAN_GLSL 不使用、mUseUBO runtime flag 単独 gate。
// MUSEUBO-A = mUseUBO=false default で本 block 走らず既存 OpenGL 挙動 100% 維持。
// ptr+count pattern 2 件目 = 4-component (= count * 4 * sizeof(GLint))。
```

---

## §4 self-verify 9 観点 PASS (= 本 session 実施結果)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) +14 line 物理確認 | `git diff --stat` = 1 file changed + 14 insertions (= PB-4.6 +15 line から tmp[N]={...}; 1 line 減で 14 line、ptr+count pattern では tmp array 不要) | ✅ |
| (2) 挿入位置 | glUniform1iv 直前 (= 既存 OpenGL path より前に Vulkan path 分岐) | ✅ |
| (3) GATE-B 順守 | `git diff \| grep "^+" \| grep "LL_VULKAN_GLSL" \| grep -v "^+.*//"` = 0 件 (comment 中のみ 1 件) | ✅ |
| (4) MUSEUBO-A 順守 | `git diff llglslshader.h` = 0 (= mUseUBO=false default 維持) | ✅ |
| (5) static verify §5 全 7 観点 | (1〜7) 全 PASS | ✅ |
| (6) 残 sub-step strict 線形維持 | §2 table = PB-4.8 → ... → PB-N | ✅ |
| (7) 1 method 1 sub-step 順守 | `feedback_ubo_migration_one_at_a_time` 順守、batch 3 緩和終了後 4 件連続順守 (PB-4.4/.5/.6/.7) | ✅ |
| (8) working tree = 1 file のみ | `git status` = M indra/llrender/llglslshader.cpp のみ | ✅ |
| (9) Co-Authored-By 不在 | commit message 行頭 `Co-Authored-By:` 0 件 | ✅ |

---

## §5 build verify (= 本 session 実施結果)

| 段階 | 確認 | 結果 |
|---|---|---|
| (1) llrender 単体 build | `cmake --build build-linux-x86_64 --target llrender -j` | ✅ PASS (libllrender.a link) |
| (2) full viewer build | `cmake --build build-linux-x86_64 -j` | ✅ PASS (llpackage 到達、`Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-81586.tar.xz` 生成) |

---

## §6 引き継ぎ済 memory (= PB-4.6 handoff 継承、追加なし)

- `project_r41_phase1b_vulkan_host_gate.md` — Phase 1.B 全 sub-step で C++ 側 `#ifdef LL_VULKAN_GLSL` 不使用、runtime `mUseUBO` flag 単独 gate
- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ厳守
- `feedback_no_scope_shrink` — Phase 1.B Exit Criteria literal「30 setter 全部」厳守
- `feedback_self_verify_before_handoff` — PB-N 検証時 AYA 起動目視前に Claude 3 経路非到達 verify 再走
- `feedback_no_claude_coauthor` — 全 commit 共著行不在
- `feedback_one_step_at_a_time` — strict 線形進行
- `feedback_ubo_migration_one_at_a_time` — **1 method 1 sub-step (= batch 3 緩和終了後 PB-4.4 / PB-4.5 / PB-4.6 / PB-4.7 で 4 件連続順守)**
- `feedback_doubt_self_first` — build error / link fail は AYA 投げ前に Claude root cause 特定
- `feedback_no_auto_commit` — 通常 session では AYA 明示指示後 commit (= 本 session は handoff §7 1 line 自走指示により合致)
- `feedback_remove_verification_logs` — commit 前 LL_INFOS hook 除去
- `feedback_self_bug_no_defer_option` — PB-4.8 範囲外の既存 OpenGL line (= uniform4iv が glUniform1iv 呼ぶ点) は本 sub-step で触らない、別 issue で

---

## §7 次 session 着手 1 line

**「前 session で Phase 1.B PB-4.7 integer index 経路 uniform1iv Vulkan path 分岐追加 完了 (commit `1b54b29d3f`、+14 line)。本 session = **AYA 判断不要 自走** = **PB-4.8 単独着手** = `uniform4iv(U32 index, U32 count, const GLint* v)` (llglslshader.cpp:2514) に Vulkan path 分岐追加 (= **ptr+count pattern 2 件目** = `forwardToUboUpload(loc, v, count * 4 * sizeof(GLint))` で ptr 直接渡し + 動的 size = `count * 4 * sizeof(GLint)` (= 4-component)、PB-4.7 (uniform1iv) と完全同型、要素 byte size のみ 1→4 倍)、挿入位置 = `glUniform1iv(mUniform[index], count, v);` (line 2514、既存 OpenGL line は pre-existing で本 sub-step 修正対象外) 直前、GATE-B 整合 + MUSEUBO-A 整合、06a §5.2 / §5.3 literal、1 method 1 sub-step strict 線形 (= `feedback_ubo_migration_one_at_a_time` 順守、batch 3 緩和終了後 PB-4.4 / PB-4.5 / PB-4.6 / PB-4.7 に続く 5 件目) → 残 PB-4.9〜.17 → PB-5.1〜.13 → PB-7 → PB-N strict 線形進行。」**
