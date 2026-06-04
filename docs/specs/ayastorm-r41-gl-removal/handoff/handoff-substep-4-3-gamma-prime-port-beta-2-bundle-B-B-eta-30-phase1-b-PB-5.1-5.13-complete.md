# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-5.1〜.13 batch complete

**作成日**: 2026-06-04
**本 session 物理 commit**:
- `1fa7555a06` (= PB-5.1〜.13 batch、LLStaticHashedString 経路 13 method = uniform1i/1iv/2i/1f/2f/3f/4f/1fv/2fv/3fv/4fv/4uiv hashed + uniformMatrix4fv hashed、+238 line 1 file changed、full build EXIT 0 + tar.xz 205 MB)

**本 handoff 目的**: PB-5.1〜.13 batch 完了 record + 次 sub-step (= PB-4.8 + PB-5.14 統合) へ strict 線形引き継ぎ。次 sub-step 着手 prep は別 doc (= `handoff-...-PB-4.8-PB-5.14-merged-prep.md`)。

---

## §0 state 一行 summary

η-30 **Phase 1.B PB-5.1〜.13 batch 完了 state** (= 本 commit `1fa7555a06` で 13 method 連続 Vulkan path 分岐追加、PB-5 系 13/14 完了、残 1 件 PB-5.14 (= uniform4iv hashed) は (U4) design 判断 AYA 待ちで除外、次 sub-step = PB-4.8 + PB-5.14 統合):

- **物理成果** = LLStaticHashedString 経路 13 method 全部に Vulkan path 分岐挿入完了 = `mUniformUBOLocByHash.find(static_cast<U64>(uniform.Hash()))` map lookup + cadence_tag skip + `forwardToUboUpload(loc, data, size)` call + silent skip return
- **build verify** = full viewer build EXIT 0、tar.xz 205 MB (= 過去 build size 同等)、初回 build パス (前 session 確立した clean rebuild 救済 protocol 不発動)
- **static verify §5 全 7 観点 PASS** = (1) LL_VULKAN_GLSL コード行 0 件 + (2) mUseUBO=true 新規 0 件 + (3) Co-Authored-By 不在 + (4) `mUniformUBOLocByHash.find` 追加 13 件 + (5) `mUniformUBOLoc[index]` 参照追加 0 件 (= PB-5 対象外) + (6) forwardToUboUpload 追加 13 件 + (7) 既存 glUniform* line 削除 0 件
- **14 vs 13 整合性確定** = cpp 物理 14 hashed method 中 PB-5.14 (uniform4iv hashed) を (U4) design 判断 AYA 待ちで除外、PB-5.1〜.13 = 13 method 確定
- **想定 +200 line vs 実測 +238 line** = comment header 5 line + 構造的 closing brace + 余白行で +3 line × 13 method = +38 line 上振れ、機能影響なし

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む (= 次 sub-step 着手 prep handoff の §1.1 参照)。

### §1.1 必読 3 件 (= 本 handoff doc + 次 sub-step prep handoff + 該当 method 既存 code)

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-...-PB-5.1-5.13-complete.md`) | 全文 | PB-5.1〜.13 batch 完了 record + 残 sub-step strict 線形確認 |
| 2 | `handoff-...-PB-4.8-PB-5.14-merged-prep.md` | 全文 | 次 sub-step 着手 prep (= (U4) design 判断 AYA 起案 + 機械的展開 pattern) |
| 3 | `indra/llrender/llglslshader.cpp:2494-2519` (= PB-4.8 整数) + `indra/llrender/llglslshader.cpp:2998-3015` (= uniform4iv hashed) | 該当 2 method 既存実装 | 挿入位置確定 + 既存構造確認 (= mValue cache block + shouldChange + count != 1) |

### §1.2 pinpoint Read 用 reference (= 必要時のみ)

| file | 参照箇所 |
|---|---|
| `indra/llrender/llglslshader.cpp:2473-2486` | PB-4.7 (uniform1iv 整数) Vulkan path 分岐 pattern (= ptr+count 系で mValue block **内側** 挿入の例、(A) 案根拠) |
| `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-and-locking.md:133` | (U4) doctrine literal「matrix 系 + uniform4iv は mValue cache 適用外」明記 ((B) 案根拠) |

---

## §2 完了 record

### §2.1 13 method physical insertion 一覧

| PB-X | method | line (before commit) | sub-pattern | size 計算 |
|---|---|---|---|---|
| PB-5.1 | `uniform1i(const LLStaticHashedString&, GLint v)` | 2927 | scalar 1i | `sizeof(GLint)` (`&v` 直接渡し) |
| PB-5.2 | `uniform1iv(const LLStaticHashedString&, U32, const GLint*)` | 2944 | ptr+count GLint | `count * sizeof(GLint)` |
| PB-5.3 | `uniform2i(const LLStaticHashedString&, GLint, GLint)` | 2980 | scalar 2i | `sizeof(tmp[2])` |
| PB-5.4 | `uniform1f(const LLStaticHashedString&, GLfloat)` | 2998 | scalar 1f | `sizeof(GLfloat)` (`&v` 直接渡し) |
| PB-5.5 | `uniform2f(const LLStaticHashedString&, GLfloat, GLfloat)` | 3015 | scalar 2f | `sizeof(tmp[2])` |
| PB-5.6 | `uniform3f(const LLStaticHashedString&, GLfloat, y, z)` | 3033 | scalar 3f | `sizeof(tmp[3])` |
| PB-5.7 | `uniform4f(const LLStaticHashedString&, GLfloat, y, z, w)` | 3050 | scalar 4f | `sizeof(tmp[4])` |
| PB-5.8 | `uniform1fv(const LLStaticHashedString&, U32, const GLfloat*)` | 3067 | ptr+count GLfloat | `count * sizeof(GLfloat)` |
| PB-5.9 | `uniform2fv(const LLStaticHashedString&, U32, const GLfloat*)` | 3084 | ptr+count GLfloat | `count * 2 * sizeof(GLfloat)` |
| PB-5.10 | `uniform3fv(const LLStaticHashedString&, U32, const GLfloat*)` | 3101 | ptr+count GLfloat | `count * 3 * sizeof(GLfloat)` |
| PB-5.11 | `uniform4fv(const LLStaticHashedString&, U32, const GLfloat*)` | 3118 | ptr+count GLfloat | `count * 4 * sizeof(GLfloat)` |
| PB-5.12 | `uniform4uiv(const LLStaticHashedString&, U32, const GLuint*)` | 3136 | ptr+count GLuint | `count * 4 * sizeof(GLuint)` |
| PB-5.13 | `uniformMatrix4fv(const LLStaticHashedString&, U32, GLboolean, const GLfloat*)` | 3154 | matrix 4x4 | `count * 16 * sizeof(GLfloat)` |

### §2.2 PB-5.14 deferred (= 本 batch 除外、次 sub-step で着手)

| PB-X | method | line (after commit) | 除外理由 |
|---|---|---|---|
| PB-5.14 | `uniform4iv(const LLStaticHashedString&, U32, const GLint*)` | 2998 (= +238 line shift で 2962 → 2998) | **(U4) design 判断 AYA 待ち** = 既存実装に mValue cache block + shouldChange + `count != 1` 条件あり、(U4) doctrine literal「uniform4iv は mValue cache 適用外」を mValue block 内側 (= PB-4.7 整合) or 外側 (= (U4) literal 整合) のどちらで実装するか design 判断要、本 batch では除外、次 sub-step (= PB-4.8 + PB-5.14 統合) で AYA 判断後着手 |

### §2.3 共通 pattern 確認 (= 06a §5.5 literal 準拠)

```cpp
if (mUseUBO)
{
    auto it = mUniformUBOLocByHash.find(static_cast<U64>(uniform.Hash()));
    if (it != mUniformUBOLocByHash.end())
    {
        const ubo::UniformLocation& loc = it->second;
        if (loc.cadence_tag == 0xFFFFFFFFu) return;
        if (loc.cadence_tag == 5 /* CADENCE_SAMPLER */) return;
        forwardToUboUpload(loc, data, size);
    }
    return;
}
```

### §2.4 build verify 段階記録

| 段階 | 結果 |
|---|---|
| (1) 13 連 Edit | 1 file changed + 238 insertions、全 Edit 1 発成功 |
| (2) static verify §5 全 7 観点 | 全 PASS |
| (3) full viewer build | EXIT 0、tar.xz 205 MB 生成、初回 build パス (前 session 救済 protocol 不発動) |
| (4) build infra rename race | 不発 = 前 session で `packaged/packaged/` nested 残骸 clean 済 (`d28b2ecf14` commit 前後) ため本 session で再現せず、root cause 仮説 (= viewer_manifest.py `mv -T` 無し + make 並列 trigger window) は次 issue 残置 |
| (5) 1 commit | `1fa7555a06`、Co-Authored-By 行不在、AYA style verbose 単一行 commit message (= 1 paragraph 全項目記述) |

---

## §3 残 sub-step strict 線形 (= PB-5.1〜.13 batch 完了反映)

```
✅ PB-5.1〜.13 batch (= 本 commit 1fa7555a06、13 method 完了)
  → ⏳ PB-4.8 + PB-5.14 統合 sub-step (= uniform4iv 整数 + hashed、(U4) design 判断 AYA 待ち、次 session 着手)
  → ⏳ PB-7 (= mapUniforms() 末尾 debug build llassert 整合 check)
  → ⏳ PB-N (= Phase 1.B Exit Criteria 検証、AYA と一緒に実行)
```

---

## §4 引き継ぎ済 memory (= PB-4.14〜.17 → PB-5.1〜.13 batch 継承、追加なし)

- `project_r41_phase1b_vulkan_host_gate.md` — Phase 1.B 全 sub-step で C++ 側 `#ifdef LL_VULKAN_GLSL` 不使用、runtime `mUseUBO` flag 単独 gate
- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ厳守
- `feedback_no_scope_shrink` — Phase 1.B Exit Criteria literal「30 setter 全部」厳守、本 batch は 13/14 = scalar/vector/matrix 全展開、deferred 1 件 (PB-5.14) のみ AYA 判断要件で除外
- `feedback_self_verify_before_handoff` — commit 前 10 観点 self-verify
- `feedback_no_claude_coauthor` — 全 commit 共著行不在
- `feedback_one_step_at_a_time` — strict 線形進行
- `feedback_ubo_migration_one_at_a_time` — 射程例外運用 4 条件 ((1) call site 追加のみ + (2) 構造変更なし + (3) 設計判断なし + (4) AYA 提案/同意) 充足時のみ batch 化可、本 batch は AYA 指示「PB-5.1〜.13 で 1つの handoff」literal で 4 条件全充足、次 sub-step (= PB-4.8 + PB-5.14) は (3) 違反 (= (U4) design 判断要) で batch 化適用外、AYA 判断後 別 sub-step
- `feedback_doubt_self_first` — build error / link fail は AYA 投げ前に Claude root cause 特定
- `feedback_no_auto_commit` — handoff §7 1 line 自走指示後は AYA 明示指示なしで commit 可
- `feedback_remove_verification_logs` — commit 前 LL_INFOS hook 除去
- `feedback_self_bug_no_defer_option` — PB scope 外の他人 bug (= viewer_manifest.py mv -T 無し等) は本 sub-step で触らない、別 issue で
- `feedback_proactive_handoff` — context 圧迫時は能動 handoff、AYA 指示「handoff しましょう」literal で本 doc 起案

---

## §5 self-verify 観点 (= 本 handoff 完了確認)

| 観点 | 確認方法 | 結果 |
|---|---|---|
| (1) 物理 commit `1fa7555a06` 存在 | `git log -1` | ✅ |
| (2) working tree clean | `git status` = nothing to commit | ✅ (本 handoff doc 起案後は 2 doc 新規) |
| (3) §2.1 table line 番号 = commit 前 source 一致 | `indra/llrender/llglslshader.cpp` (commit 前 = HEAD~1) | ✅ (handoff doc §2.1 から流用) |
| (4) §3 strict 線形 = 前 handoff (PB-4.14〜.17 complete + PB-5.1〜.13 prep) 整合 | 順序 = PB-5.1〜.13 ✅ → PB-4.8+PB-5.14 ⏳ → PB-7 ⏳ → PB-N ⏳ | ✅ |
| (5) §4 memory 継承漏れ | PB-4.14〜.17 complete handoff §6 から確認 | ✅ |
| (6) 次 sub-step prep doc reference | §1.1 #2 で言及 | ✅ |
