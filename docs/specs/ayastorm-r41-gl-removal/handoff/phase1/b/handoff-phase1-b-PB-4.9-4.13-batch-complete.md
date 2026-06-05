# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-4.9〜.13 batch 完了

**作成日**: 2026-06-04
**本 session 物理 commit**:
- `e83f76f2be` (= PB-4.9〜.13 batch、uniform1fv/2fv/3fv/4fv/4uiv 5 method 連続 Vulkan path 分岐、+70 line)

**次 session 着手**: **AYA 判断不要 自走** → **PB-4.14〜.17 batch 着手** = `uniformMatrix2fv` / `uniformMatrix3fv` / `uniformMatrix3x4fv` / `uniformMatrix4fv` 4 method 連続 Vulkan path 分岐追加 (= **(U4) 構造的自然成立 batch**、既存 method に mValue cache check 0 件 + shouldChange 0 件 + transpose=GL_TRUE 呼出 0 件 grep 確認済 → mValue block 削除 design 判断不要、PB-4.7 pattern を **`if (mUniform[index] >= 0)` block 内 `glUniformMatrixNfv` 直前** に挿入で機械的展開可)、4 連 Edit → 1 commit → full build 1 回。

---

## §0 state 一行 summary

η-30 **Phase 1.B PB-4.9〜.13 batch 完了 state** (= 直前 commit `e83f76f2be`):
- **PB-4.9〜.13 5 method batch 完了** = llglslshader.cpp の uniform1fv/2fv/3fv/4fv/4uiv 各 glUniformXfv 直前に `if (mUseUBO) { ... forwardToUboUpload(loc, v, count * N * sizeof(T)); return; }` 分岐追加、+14 line × 5 = **+70 line**
- **batch 化根拠** = 2026-06-04 AYA 提案「PB-4.9〜.17 を一括展開して最後にビルドしたほうが効率いいのでは？」+ 中途 AYA 追加提案「PB-4.13 も一緒にやれますか？」(= PB-4.13 合流) + `feedback_ubo_migration_one_at_a_time` 射程例外運用 4 条件全充足
- **build verify** = (1) 初回 build infra rename race fail (packaged/packaged/ nested 残骸由来、PB 改変由来 error 0 件) → (2) `rm -rf packaged/` clean state + `cmake --build llpackage` 再走 = EXIT 0、tar.xz 205 MB 生成 (= 過去 build size 同等、初回 411 MB 異常値解消)、`Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-81586.tar.xz`
- **build infra race root cause 仮説 (= 別 issue)** = `indra/newview/viewer_manifest.py` `package_finish()` の `mv` が `-T` 無し + make 並列での `copy_l_viewer_manifest` 再 trigger window で nested dir 形成、本 batch 由来ではない
- **残 sub-step**: PB-4.8 (uniform4iv) + PB-4.14〜.17 (matrix 4 件) + PB-5.1〜.13 (13 method) + PB-7 + PB-N strict 線形
- **設計判断 0 件新規確定** (= PB-4.9〜.13 5 件は全 default 採用済、Claude 自走継続可)

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-4.9-4.13-batch-complete.md`) | 全文 | PB-4.9〜.13 完了 state + 残 PB-4.14〜.17 順序 + PB-4.14〜.17 batch 着手の概要 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-4.14-4.17-batch-prep.md` | 全文 | PB-4.14〜.17 batch literal pattern + 挿入位置 + size 計算 + (U4) 構造的自然成立根拠 |
| 3 | `indra/llrender/llglslshader.cpp:2729-2811` | matrix 4 method 既存実装 + 周辺 | PB-4.14〜.17 着手前 挿入位置確定 |

### §1.2 pinpoint Read 用 reference (= 必要時のみ)

| file | 参照箇所 |
|---|---|
| `indra/llrender/llglslshader.cpp:2542-2719` (本 batch 結果) | PB-4.9〜.13 Vulkan path 分岐 pattern (= 直前 commit、ptr+count pattern 連続適用、PB-4.14〜.17 は別構造 (= mValue check 不在) なので参考度低、共通 6-line `if (mUseUBO) {...}` 形のみ移植) |
| `indra/llrender/llglslshader.h:443-457` | `forwardToUboUpload()` 宣言確認 (= PB-6 追加済、ptr 引数 byte size 受領) |
| `indra/llrender/llglslshader.h:427-429` | `mUseUBO = false` default 確認 (= MUSEUBO-A) |
| `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-and-locking.md:133` (U4) | mValue cache 適用外明記 (= PB-4.14〜.17 matrix 系は既存 mValue check 0 件 = 構造的に (U4) 自然成立) |

---

## §2 残 sub-step strict 線形構成 (= PB-4.9〜.13 完了反映)

| PB-X | 内容 | 物理改変 file | 状態 |
|---|---|---|---|
| ~~PB-1~~ | ~~mUniformUBOLoc cache 構造実装~~ | llglslshader.h | **✅ 完了** (commit `0ba743463c`) |
| ~~PB-2~~ | ~~mapUniforms() integer index 経路 cache 構築~~ | llglslshader.cpp | **✅ 完了** (commit `e43d93dd25`) |
| ~~PB-3~~ | ~~LLStaticHashedString 経路 cache 構築 (S1-C)~~ | llglslshader.cpp | **✅ 完了** (commit `127d25ecb6`) |
| ~~PB-6~~ | ~~forwardToUboUpload() shell 実装 (FWD-1)~~ | llglslshader.h + .cpp | **✅ 完了** (commit `79966ad2f3`) |
| ~~PB-4.1〜.3~~ | ~~uniform1i / uniform1f / fastUniform1f Vulkan path 分岐~~ | llglslshader.cpp | **✅ 完了** (commit `772b6b985c`) |
| ~~PB-4.4~~ | ~~uniform2f Vulkan path 分岐~~ | llglslshader.cpp | **✅ 完了** (commit `71576ff1e4`) |
| ~~PB-4.5~~ | ~~uniform3f Vulkan path 分岐~~ | llglslshader.cpp | **✅ 完了** (commit `257df5e757`) |
| ~~PB-4.6~~ | ~~uniform4f Vulkan path 分岐~~ | llglslshader.cpp | **✅ 完了** (commit `62c7a95caa`) |
| ~~PB-4.7~~ | ~~uniform1iv Vulkan path 分岐 (= ptr+count pattern 初出)~~ | llglslshader.cpp | **✅ 完了** (commit `1b54b29d3f`) |
| ~~PB-4.9~~ | ~~uniform1fv Vulkan path 分岐 (= count * sizeof(GLfloat))~~ | llglslshader.cpp | **✅ 完了** (commit `e83f76f2be`) |
| ~~PB-4.10~~ | ~~uniform2fv Vulkan path 分岐 (= count * 2 * sizeof(GLfloat))~~ | llglslshader.cpp | **✅ 完了** (commit `e83f76f2be`) |
| ~~PB-4.11~~ | ~~uniform3fv Vulkan path 分岐 (= count * 3 * sizeof(GLfloat))~~ | llglslshader.cpp | **✅ 完了** (commit `e83f76f2be`) |
| ~~PB-4.12~~ | ~~uniform4fv Vulkan path 分岐 (= count * 4 * sizeof(GLfloat))~~ | llglslshader.cpp | **✅ 完了** (commit `e83f76f2be`) |
| ~~PB-4.13~~ | ~~uniform4uiv Vulkan path 分岐 (= count * 4 * sizeof(GLuint))~~ | llglslshader.cpp | **✅ 完了** (commit `e83f76f2be`) |
| **PB-4.14** | uniformMatrix2fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v) = count * 4 * sizeof(GLfloat) | llglslshader.cpp | 次 session batch 着手 (= PB-4.14〜.17 一括) |
| **PB-4.15** | uniformMatrix3fv = count * 9 * sizeof(GLfloat) | llglslshader.cpp | 同上 batch |
| **PB-4.16** | uniformMatrix3x4fv = count * 12 * sizeof(GLfloat) | llglslshader.cpp | 同上 batch |
| **PB-4.17** | uniformMatrix4fv = count * 16 * sizeof(GLfloat) | llglslshader.cpp | 同上 batch |
| **PB-4.8** | uniform4iv(U32 index, U32 count, const GLint* v) Vulkan path 分岐 = **(U4) design 判断要 = mValue block 内側 (PB-4.7 整合) or 外側 ((U4) 整合) 選択**、本 batch 除外、PB-4.14〜.17 完了後 別 sub-step | llglslshader.cpp | **(U4) design 判断 AYA 待ち** |
| **PB-5.1〜.13** | 13 method LLStaticHashedString 経路 Vulkan path 分岐 | llglslshader.cpp | PB-4.8 完了後 |
| **PB-7** | mapUniforms() 末尾 debug build llassert 整合 check | llglslshader.cpp | |
| **PB-N** | Phase 1.B Exit Criteria 検証 (= ローカル動作確認) | — | AYA と一緒に実行 |

**注 (= 順序最適化)**: PB-4.8 (uniform4iv) は (U4) design 判断が必要なため、構造的に (U4) 自然成立する PB-4.14〜.17 (matrix 4 件) を **先に** 処理する。PB-4.7 で確立した「mValue block 内側」pattern と (U4) doctrine「mValue cache 適用外」をどう調停するかは design 判断要件で、AYA 判断を待つ。PB-4.14〜.17 は既存実装に mValue block 0 件のため、`if (mUniform[index] >= 0)` block 内 `glUniformMatrixNfv` 直前に挿入するだけで両 doctrine と整合する。

---

## §3 本 session 新規確定 設計判断

**設計判断 0 件** (= PB-4.9〜.13 5 件は全 default 採用済、PB-4.7 pattern の T と N 差し替えで機械的展開のみ、AYA 確認不要)。

---

## §4 self-verify 9 観点 PASS (= 本 session 実施結果)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) +70 line 物理確認 | `git diff --stat` = 1 file changed + 70 insertions (= 14 line × 5 method) | ✅ |
| (2) 挿入位置 | 各 method の glUniformXfv 直前 (= 既存 OpenGL path より前に Vulkan path 分岐) | ✅ |
| (3) GATE-B 順守 | `git diff \| grep "^+" \| grep "LL_VULKAN_GLSL" \| grep -v "^+.*//"` = 0 件 (comment 中のみ 5 件) | ✅ |
| (4) MUSEUBO-A 順守 | `git diff llglslshader.h` = 0 (= mUseUBO=false default 維持) | ✅ |
| (5) static verify §5 全 7 観点 | (1〜7) 全 PASS | ✅ |
| (6) 残 sub-step strict 線形維持 | §2 table = PB-4.14〜.17 → PB-4.8 → PB-5.1〜.13 → PB-7 → PB-N | ✅ |
| (7) batch 化条件 (= `feedback_ubo_migration_one_at_a_time` 射程例外 4 条件) | (1) 既存 redirect 層 (forwardToUboUpload) への call site 追加のみ + (2) 構造変更なし + (3) 設計判断なし + (4) AYA 提案 受領済 (2026-06-04) → 全充足 | ✅ |
| (8) working tree = 1 file のみ | `git status` = M indra/llrender/llglslshader.cpp のみ | ✅ |
| (9) Co-Authored-By 不在 | commit message 行頭 `Co-Authored-By:` 0 件 | ✅ |

---

## §5 build verify (= 本 session 実施結果)

| 段階 | 確認 | 結果 |
|---|---|---|
| (1) 初回 full viewer build | `cmake --build build-linux-x86_64 -j` | ⚠️ EXIT 2 (= packaged/packaged/ nested 残骸由来 mv rename race、PB 改変由来 compile/link error 0 件確認) |
| (2) packaged/ clean rebuild | `rm -rf build-linux-x86_64/newview/packaged && cmake --build build-linux-x86_64 --target llpackage -j` | ✅ EXIT 0 (tar.xz 205 MB 生成、`Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-81586.tar.xz`、初回 411 MB 異常値解消 = nested 重複が原因確定) |

**注 (= build infra race 別 issue)**: 初回 fail は `indra/newview/viewer_manifest.py` の `package_finish()` 内 `mv` が `-T` flag 無しで使われており、make 並列での `copy_l_viewer_manifest` 再 trigger window と組み合わさって nested `packaged/packaged/` 残骸が累積する **build infra bug**。PB-4.9〜.13 batch 由来ではなく、過去 build から残った副産物。fix は本 PB scope 外、別 issue として記録 (= `feedback_self_bug_no_defer_option` で「他人 bug」扱い、ただし発生環境を残すと次回再発するので clean rebuild で対処)。

---

## §6 引き継ぎ済 memory (= PB-4.7 handoff 継承、追加なし)

- `project_r41_phase1b_vulkan_host_gate.md` — Phase 1.B 全 sub-step で C++ 側 `#ifdef LL_VULKAN_GLSL` 不使用、runtime `mUseUBO` flag 単独 gate
- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ厳守
- `feedback_no_scope_shrink` — Phase 1.B Exit Criteria literal「30 setter 全部」厳守
- `feedback_self_verify_before_handoff` — PB-N 検証時 AYA 起動目視前に Claude 3 経路非到達 verify 再走
- `feedback_no_claude_coauthor` — 全 commit 共著行不在
- `feedback_one_step_at_a_time` — strict 線形進行
- `feedback_ubo_migration_one_at_a_time` — **1 method 1 sub-step strict 線形 + 射程例外運用 4 条件 (= (1) call site 追加のみ + (2) 構造変更なし + (3) 設計判断なし + (4) AYA 提案/同意) 充足時のみ batch 化可、本 batch は 4 条件全充足で適用、PB-4.14〜.17 batch も同条件で適用予定**
- `feedback_doubt_self_first` — build error / link fail は AYA 投げ前に Claude root cause 特定 (= 本 session 初回 build fail を AYA 投げ前に diff/log で「PB 改変由来 0 件 + nested dir 由来確定」まで自己分析、clean rebuild 提案)
- `feedback_no_auto_commit` — 通常 session では AYA 明示指示後 commit (= 本 session は handoff §7 1 line 自走指示 + AYA 中途承認 (= PB-4.13 合流、build root cause 投資、clean rebuild、2-commit plan B) 受領で合致)
- `feedback_remove_verification_logs` — commit 前 LL_INFOS hook 除去
- `feedback_self_bug_no_defer_option` — PB 範囲外の既存 OpenGL 特異性 (= PB-4.12/PB-4.13 の二重 LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER + viewer_manifest.py mv -T 無し) は本 sub-step で触らない、別 issue で
- `feedback_proactive_handoff` — context 圧迫時自発 handoff 提案 (= 本 session の Step 2 が該当、AYA 「B」選択受け handoff 2 件統合 commit へ)

---

## §7 次 session 着手 1 line

**「前 session で Phase 1.B PB-4.9〜.13 batch 5 method (uniform1fv/2fv/3fv/4fv/4uiv) integer index 経路 Vulkan path 分岐追加 完了 (commit `e83f76f2be`、+70 line = +14 × 5)。本 session = **AYA 判断不要 自走** = **PB-4.14〜.17 batch 着手** = `uniformMatrix2fv` (llglslshader.cpp:2729) / `uniformMatrix3fv` (line 2750) / `uniformMatrix3x4fv` (line 2771) / `uniformMatrix4fv` (line 2792) の 4 method 連続 Vulkan path 分岐追加 (= 各 method 内 `if (mUniform[index] >= 0)` block 内 `glUniformMatrixNfv` 直前に PB-4.7 pattern を **size 計算のみ N=4/9/12/16 差し替え** で挿入、4 連 Edit → 1 commit → full build 1 回)、(U4) doctrine 構造的自然成立 (= 既存実装に mValue cache check 0 件 + shouldChange 0 件 + transpose=GL_TRUE 呼出 0 件 grep 確認済)、GATE-B 整合 + MUSEUBO-A 整合、06a §5.2 / §5.3 literal、`feedback_ubo_migration_one_at_a_time` 射程例外 4 条件全充足 (= 既存 redirect 層 call site 追加のみ + 構造変更なし + 設計判断なし + AYA 提案 受領済 2026-06-04) → PB-4.14〜.17 完了後 PB-4.8 ((U4) design 判断後) → PB-5.1〜.13 → PB-7 → PB-N strict 線形進行。」**
