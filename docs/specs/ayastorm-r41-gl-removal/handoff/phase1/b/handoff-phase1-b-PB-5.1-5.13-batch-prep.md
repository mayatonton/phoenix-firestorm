# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-5.1〜.13 batch 着手 prep

**作成日**: 2026-06-04
**前 session 物理 commit**:
- `d28b2ecf14` (= PB-4.14〜.17 batch、uniformMatrix2fv/3fv/3x4fv/4fv 4 method integer index 経路 Vulkan path 分岐、+56 line、full build EXIT 0 + tar.xz 197 MB)

**本 session 着手**: **AYA 判断不要 自走** → **PB-5.1〜.13 batch 物理着手** = LLStaticHashedString 経路 13 method 連続 Vulkan path 分岐追加、13 連 Edit → 1 commit → full build 1 回。

---

## §0 state 一行 summary

η-30 **Phase 1.B PB-5.1〜.13 batch 着手 prep state** (= 前 commit `d28b2ecf14` の続き、PB-4 系 17 method 全完了 (= PB-4.1〜.17 のうち PB-4.8 のみ (U4) design 判断 AYA 待ちで除外)、PB-5 系 13 method 着手):

- **本 doc 目的** = PB-5.1〜.13 LLStaticHashedString 経路 13 method を **1 batch** で展開する着手書、AYA 指示 (2026-06-04)「PB-5.1〜.13 で 1つの handoff にしてください」literal
- **14 vs 13 整合性 (= design-review 2026-06-03 second-pass §102 で指摘済「整合性軽微違反候補」)** = `cpp` 物理は **14 method** (= uniform1i/1iv/4iv/2i/1f/2f/3f/4f/1fv/2fv/3fv/4fv/4uiv/Matrix4fv) だが spec entry §2.3 line 79 + 06a §5.5 は **13 method** literal、本 handoff で **PB-5.1〜.13 = 13 method = 上記 14 - uniform4iv hashed 1 件 (PB-5.14 として deferred)** で確定 = PB-4 系で uniform4iv 整数 index (= PB-4.8) を (U4) design 判断 AYA 待ちで deferred したのと parallel
- **batch 化根拠** = AYA 指示 literal「PB-5.1〜.13 で 1つの handoff にしてください」(2026-06-04) + `feedback_ubo_migration_one_at_a_time` 射程例外運用 4 条件全充足 ((1) 既存 redirect 層 (forwardToUboUpload) call site 追加のみ + (2) 構造変更なし + (3) 設計判断なし (= (U4) は PB-5.14 deferred で本 batch 外) + (4) AYA 提案受領済)
- **共通 pattern** = PB-4 整数 index 経路と diff は 2 点 = (i) cache lookup が `mUniformUBOLocByHash.find(static_cast<U64>(uniform.Hash()))` map find (整数 index 経路の `mUniformUBOLoc[index]` 直引きと異なる) + (ii) hash 未集約時 silent skip return (整数 index 経路の `llassert(index < mUniformUBOLoc.size())` と異なる)
- **挿入位置** = 各 method 先頭 `LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER` 直後 `getUniformLocation(uniform)` 直前 (= 06a §5.5 spec code shape literal、Vulkan path で `getUniformLocation()` を bypass、map lookup のみ)
- **想定 +200 line** = scalar pattern × 7 (1i/2i/1f/2f/3f/4f + Matrix4fv) + ptr pattern × 6 (1iv/1fv/2fv/3fv/4fv/4uiv) 各 method ~15 line × 13 = ~195 line + comment 余裕で +200 line 想定 (= 実測 ±5 line)
- **build verify 戦略** = full viewer build **1 回のみ** (= llrender 単体 build スキップ、初回 fail 時 `rm -rf build-linux-x86_64/newview/packaged` 後 llpackage 再走で救済 protocol 確立済)
- **設計判断 0 件** (= 全 default 採用、Claude 自走可)

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。本 session 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-5.1-5.13-batch-prep.md`) | 全文 | PB-5.1〜.13 batch literal pattern + 挿入位置 + size 計算 + 14 vs 13 整合性確定 (= uniform4iv hashed deferred) |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` | §5.5 (= LLStaticHashedString 経路 path 分岐 code shape literal) | hashed lookup pattern source-of-truth |
| 3 | `indra/llrender/llglslshader.cpp:2927-3165` | LLStaticHashedString 経路 14 method 既存実装 (= uniform1i/1iv/4iv/2i/1f/2f/3f/4f/1fv/2fv/3fv/4fv/4uiv/Matrix4fv) | 挿入位置確定 + 既存構造確認 (= mValue cache 13 件、Matrix4fv のみ mValue 不在 + stop_glerror wrap) |

### §1.2 pinpoint Read 用 reference (= 必要時のみ)

| file | 参照箇所 |
|---|---|
| `indra/llrender/llglslshader.cpp:2473-2491` | PB-4.7 (uniform1iv 整数 index) Vulkan path 分岐 pattern (= ptr+count pattern source、PB-5 ptr 系 6 method 移植参考) |
| `indra/llrender/llglslshader.cpp:2245-2257` | PB-4.1 (uniform1i 整数 index) Vulkan path 分岐 pattern (= scalar pattern source、PB-5 scalar 系 7 method 移植参考) |
| `indra/llrender/llglslshader.cpp:2843-2860` | PB-4.17 (uniformMatrix4fv 整数 index) Vulkan path 分岐 pattern (= matrix pattern source、PB-5.13 Matrix4fv hashed 移植参考) |
| `indra/llcommon/llstaticstringtable.h:46` | `LLStaticHashedString::Hash() const { return string_hash; }` 返却型 = `size_t` 確認、`static_cast<U64>(uniform.Hash())` literal 必要根拠 |
| `indra/llrender/llglslshader.h:443-457` | `forwardToUboUpload()` 宣言 (= PB-6 追加済、ptr 引数 byte size 受領) + `mUniformUBOLocByHash` map 定義 |
| `indra/llrender/llglslshader.h:427-429` | `mUseUBO = false` default 確認 (= MUSEUBO-A) |

---

## §2 PB-5.1〜.13 各 method 挿入位置 + size 計算 + pattern 分類

### §2.1 13 method = PB-4 系統 mapping (= 3 sub-pattern に分類)

| PB-X | method | 既存 line | sub-pattern | 移植元 PB | size 計算 |
|---|---|---|---|---|---|
| PB-5.1 | `uniform1i(const LLStaticHashedString&, GLint v)` | 2927 | scalar 1i | PB-4.1 (uniform1i 整数) | `sizeof(GLint)` |
| PB-5.2 | `uniform1iv(const LLStaticHashedString&, U32 count, const GLint* v)` | 2944 | ptr+count GLint | PB-4.7 (uniform1iv 整数) | `count * sizeof(GLint)` |
| PB-5.3 | `uniform2i(const LLStaticHashedString&, GLint i, GLint j)` | 2980 | scalar 2i | (新 pattern、scalar 2i ‐ 整数 index 経路に該当無し) | `2 * sizeof(GLint)` (= `GLint tmp[2] = {i, j}`) |
| PB-5.4 | `uniform1f(const LLStaticHashedString&, GLfloat v)` | 2998 | scalar 1f | PB-4.2 (uniform1f 整数) | `sizeof(GLfloat)` |
| PB-5.5 | `uniform2f(const LLStaticHashedString&, GLfloat x, GLfloat y)` | 3015 | scalar 2f | PB-4.4 (uniform2f 整数) | `2 * sizeof(GLfloat)` (= tmp[2]) |
| PB-5.6 | `uniform3f(const LLStaticHashedString&, GLfloat x, y, z)` | 3033 | scalar 3f | PB-4.5 (uniform3f 整数) | `3 * sizeof(GLfloat)` (= tmp[3]) |
| PB-5.7 | `uniform4f(const LLStaticHashedString&, GLfloat x, y, z, w)` | 3050 | scalar 4f | PB-4.6 (uniform4f 整数) | `4 * sizeof(GLfloat)` (= tmp[4]) |
| PB-5.8 | `uniform1fv(const LLStaticHashedString&, U32 count, const GLfloat* v)` | 3067 | ptr+count GLfloat | PB-4.9 (uniform1fv 整数) | `count * sizeof(GLfloat)` |
| PB-5.9 | `uniform2fv(const LLStaticHashedString&, U32 count, const GLfloat* v)` | 3084 | ptr+count GLfloat | PB-4.10 (uniform2fv 整数) | `count * 2 * sizeof(GLfloat)` |
| PB-5.10 | `uniform3fv(const LLStaticHashedString&, U32 count, const GLfloat* v)` | 3101 | ptr+count GLfloat | PB-4.11 (uniform3fv 整数) | `count * 3 * sizeof(GLfloat)` |
| PB-5.11 | `uniform4fv(const LLStaticHashedString&, U32 count, const GLfloat* v)` | 3118 | ptr+count GLfloat | PB-4.12 (uniform4fv 整数) | `count * 4 * sizeof(GLfloat)` |
| PB-5.12 | `uniform4uiv(const LLStaticHashedString&, U32 count, const GLuint* v)` | 3136 | ptr+count GLuint | PB-4.13 (uniform4uiv 整数) | `count * 4 * sizeof(GLuint)` |
| PB-5.13 | `uniformMatrix4fv(const LLStaticHashedString&, U32 count, GLboolean, const GLfloat* v)` | 3154 | matrix 4x4 | PB-4.17 (uniformMatrix4fv 整数) | `count * 16 * sizeof(GLfloat)` |

### §2.2 deferred 1 method (= PB-5.14、本 batch 除外)

| PB-X | method | 既存 line | 除外理由 |
|---|---|---|---|
| PB-5.14 | `uniform4iv(const LLStaticHashedString&, U32 count, const GLint* v)` | 2962 | **(U4) design 判断 AYA 待ち** (= PB-4.8 と parallel) = 既存実装に mValue cache block + shouldChange + `count != 1` 条件あり、(U4) doctrine literal「mValue cache 適用外」を mValue block 内側 (= PB-4.7 整合) or 外側 (= (U4) literal 整合) のどちらで実装するか design 判断要、本 batch では除外、別 sub-step で AYA 判断後着手 |

### §2.3 挿入位置確定 (= 13 method 全 `LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER` 直後 `getUniformLocation(uniform)` 直前)

**スピリ**: 06a §5.5 spec code shape literal = Vulkan path で `getUniformLocation()` を bypass、map lookup のみで完結。既存 method 構造に **`if (mProgramObject)` wrapper 不在**、本 batch では structural 変更なし、`LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER` の **直後** に Vulkan block 挿入で対応。

### §2.4 共通 pattern literal (= 3 sub-pattern)

#### §2.4.1 scalar pattern (= PB-5.1, .3, .4, .5, .6, .7 = 6 method、`uniform.Hash()` lookup + tmp array)

例: PB-5.5 (uniform2f) の場合
```cpp
void LLGLSLShader::uniform2f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-5.5:
    // LLStaticHashedString 経路 Vulkan path 分岐追加。spec 06a §5.5 literal 準拠。
    // GATE-B = #ifdef LL_VULKAN_GLSL 不使用、mUseUBO runtime flag 単独 gate。
    // MUSEUBO-A = mUseUBO=false default で本 block 走らず既存 OpenGL 挙動 100% 維持。
    // hash lookup = map find + silent skip (= 集約表に entry 無し時 OpenGL path 含め全 skip)。
    if (mUseUBO)
    {
        auto it = mUniformUBOLocByHash.find(static_cast<U64>(uniform.Hash()));
        if (it != mUniformUBOLocByHash.end())
        {
            const ubo::UniformLocation& loc = it->second;
            if (loc.cadence_tag == 0xFFFFFFFFu) return;
            if (loc.cadence_tag == 5 /* CADENCE_SAMPLER */) return;
            GLfloat tmp[2] = {x, y};
            forwardToUboUpload(loc, tmp, sizeof(tmp));
        }
        return;
    }

    GLint location = getUniformLocation(uniform);
    ... (既存 OpenGL path 維持)
}
```

**scalar pattern 各 PB の差分** (= 6 line のみ変動):
| PB-X | tmp 宣言 | size |
|---|---|---|
| PB-5.1 (1i) | `// tmp 不要 = scalar 1 件は &v 直接渡し可` | `forwardToUboUpload(loc, &v, sizeof(GLint));` |
| PB-5.3 (2i) | `GLint tmp[2] = {i, j};` | `forwardToUboUpload(loc, tmp, sizeof(tmp));` |
| PB-5.4 (1f) | `// tmp 不要 = scalar 1 件は &v 直接渡し可` | `forwardToUboUpload(loc, &v, sizeof(GLfloat));` |
| PB-5.5 (2f) | `GLfloat tmp[2] = {x, y};` | 同上 |
| PB-5.6 (3f) | `GLfloat tmp[3] = {x, y, z};` | 同上 |
| PB-5.7 (4f) | `GLfloat tmp[4] = {x, y, z, w};` | 同上 |

#### §2.4.2 ptr+count pattern (= PB-5.2, .8, .9, .10, .11, .12 = 6 method、`uniform.Hash()` lookup + ptr 直接渡し)

例: PB-5.8 (uniform1fv) の場合
```cpp
void LLGLSLShader::uniform1fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-5.8:
    // LLStaticHashedString 経路 Vulkan path 分岐追加。spec 06a §5.5 literal 準拠。
    // GATE-B = #ifdef LL_VULKAN_GLSL 不使用、mUseUBO runtime flag 単独 gate。
    // MUSEUBO-A = mUseUBO=false default で本 block 走らず既存 OpenGL 挙動 100% 維持。
    // ptr+count pattern (= count * sizeof(GLfloat) 動的 size 計算、tmp array 不要)。
    if (mUseUBO)
    {
        auto it = mUniformUBOLocByHash.find(static_cast<U64>(uniform.Hash()));
        if (it != mUniformUBOLocByHash.end())
        {
            const ubo::UniformLocation& loc = it->second;
            if (loc.cadence_tag == 0xFFFFFFFFu) return;
            if (loc.cadence_tag == 5 /* CADENCE_SAMPLER */) return;
            forwardToUboUpload(loc, v, count * sizeof(GLfloat));
        }
        return;
    }

    GLint location = getUniformLocation(uniform);
    ... (既存 OpenGL path 維持)
}
```

**ptr+count pattern 各 PB の差分** (= forwardToUboUpload 1 line 内 size 式のみ変動):
| PB-X | size 式 |
|---|---|
| PB-5.2 (1iv) | `count * sizeof(GLint)` |
| PB-5.8 (1fv) | `count * sizeof(GLfloat)` |
| PB-5.9 (2fv) | `count * 2 * sizeof(GLfloat)` |
| PB-5.10 (3fv) | `count * 3 * sizeof(GLfloat)` |
| PB-5.11 (4fv) | `count * 4 * sizeof(GLfloat)` |
| PB-5.12 (4uiv) | `count * 4 * sizeof(GLuint)` |

#### §2.4.3 matrix pattern (= PB-5.13 = 1 method、`uniform.Hash()` lookup + ptr 直接渡し + matrix size)

PB-5.13 (uniformMatrix4fv) の場合
```cpp
void LLGLSLShader::uniformMatrix4fv(const LLStaticHashedString& uniform, U32 count, GLboolean transpose, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-5.13:
    // LLStaticHashedString 経路 Vulkan path 分岐追加。spec 06a §5.5 literal 準拠。
    // GATE-B = #ifdef LL_VULKAN_GLSL 不使用、mUseUBO runtime flag 単独 gate。
    // MUSEUBO-A = mUseUBO=false default で本 block 走らず既存 OpenGL 挙動 100% 維持。
    // matrix 4x4 (= count * 16 * sizeof(GLfloat))、(U4) 構造的自然成立 (= mValue check 既存不在 + stop_glerror wrap)。
    if (mUseUBO)
    {
        auto it = mUniformUBOLocByHash.find(static_cast<U64>(uniform.Hash()));
        if (it != mUniformUBOLocByHash.end())
        {
            const ubo::UniformLocation& loc = it->second;
            if (loc.cadence_tag == 0xFFFFFFFFu) return;
            if (loc.cadence_tag == 5 /* CADENCE_SAMPLER */) return;
            forwardToUboUpload(loc, v, count * 16 * sizeof(GLfloat));
        }
        return;
    }

    GLint location = getUniformLocation(uniform);
    if (location >= 0)
    {
        stop_glerror();
        glUniformMatrix4fv(location, count, transpose, v);
        stop_glerror();
    }
}
```

**注**: PB-5.13 は既存 method に **`mValue` cache 不在 + `stop_glerror()` wrap あり** = PB-4.14〜.17 と同等の (U4) 構造的自然成立 (= 既に mValue bypass されている)、design 判断不要。transpose 引数は Vulkan path で **無視** (= grep `glUniformMatrix.*GL_TRUE` 0 件確認済 PB-4.14〜.17 handoff §3.3 と同根拠)。

### §2.5 build verify 戦略

| 段階 | 手順 | 期待結果 |
|---|---|---|
| (1) 13 連 Edit | PB-5.1 → PB-5.2 → ... → PB-5.13 順に Edit (= scalar / ptr / matrix の 3 sub-pattern を §2.4 通り apply、PB-5.14 uniform4iv hashed は skip) | `git diff --stat` = 1 file changed + ~200 insertions (= +15 line × 13 想定、実測 ±5 line) |
| (2) static verify §5 全 7 観点 | (1) LL_VULKAN_GLSL コード行 0、comment 13 件 + (2) mUseUBO=true 新規 0 + (3) Co-Authored-By 不在 + (4) `mUniformUBOLocByHash.find` 追加 13 件 + (5) `mUniformUBOLoc[index]` 参照追加 0 件 (PB-5 対象外) + (6) forwardToUboUpload 追加 13 件 + (7) 既存 glUniform* line 削除 0 件 | 全 PASS |
| (3) full viewer build | `cmake --build build-linux-x86_64 -j` (= llrender 単体 build スキップ) | EXIT 0、tar.xz 生成 (= 過去 build と同等 size 195-205 MB) |
| (4) build fail 救済 protocol | 初回 fail 時 `rm -rf build-linux-x86_64/newview/packaged && cmake --build build-linux-x86_64 --target llpackage -j` 再走 | 前 session で確立済 (= packaged/packaged/ nested 残骸由来 mv rename race) |
| (5) 1 commit | 13 method 連続 Vulkan path 分岐追加を 1 commit にまとめる、AYA style verbose 単一行 commit message | Co-Authored-By 行不在 |

---

## §3 (U4) doctrine 整合性確認

### §3.1 (U4) literal 確認

`docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-and-locking.md:133` (U4):
- 「matrix 系 + uniform4iv は mValue cache 適用外」明記
- 根拠 = matrix 系は frame 跨ぎ identity 比較が非実用的 + uniform4iv は値域広く cache hit rate 低い

### §3.2 PB-5.1〜.13 各 method の (U4) 整合性

| 観点 | scalar 系 6 (PB-5.1,.3〜.7) | ptr 系 6 (PB-5.2,.8〜.12) | matrix 1 (PB-5.13) |
|---|---|---|---|
| 既存 mValue check | あり (= shouldChange + mValue map find) | あり (= shouldChange + count != 1) | **不在** (= mValue map 触らず) |
| (U4) doctrine 適用 | (U4) 適用外明記なし → mValue cache 既存維持 | (U4) 適用外明記なし → mValue cache 既存維持 | (U4) 適用「matrix 系適用外」literal 整合 = **構造的自然成立** |
| 設計判断 | 不要 (= 06a §5.5 literal pattern = Vulkan path で `getUniformLocation` bypass + 既存 mValue path は OpenGL 側維持) | 同上 | 不要 (= 既に mValue bypass されている) |

→ **PB-5.1〜.13 13 method 全部設計判断不要、Vulkan path block を method 先頭に挿入で完了**。

### §3.3 PB-5.14 (uniform4iv hashed) 除外理由 (= PB-4.8 と parallel)

`uniform4iv(const LLStaticHashedString&, U32 count, const GLint* v)` (line 2962):
- 既存実装に **mValue cache block + shouldChange + count != 1 条件あり** (= scalar iv 系と同型)
- (U4) doctrine literal 「uniform4iv は mValue cache 適用外」
- Vulkan path 挿入位置の design 判断要 (= PB-4.8 と同 design 問):
  - (A) **mValue block 内側** (= PB-4.7/.9〜.13 整合、Vulkan path mValue 内側挿入、(U4) literal「適用外」を「Vulkan path では bypass」と解釈)
  - (B) **mValue block 外側** (= (U4) integral 解釈、既存 mValue block を Vulkan path 前に bypass、PB-4.7 pattern 例外扱い)
- → **本 batch では除外**、PB-5.1〜.13 batch 完了後 PB-4.8 と統合して AYA 判断仰ぎ後 1 sub-step (= PB-4.8 + PB-5.14) で着手

### §3.4 silent skip semantics (= 06a §5.5 literal)

`mUseUBO=true` で hash が `mUniformUBOLocByHash` に未集約の場合:
- spec 06a §5.5 literal: 「hash 未集約 = silent skip (= 集約表に entry 無し)」 = **OpenGL path も skip、何も起きない**
- 本 batch では MUSEUBO-A (= `mUseUBO=false` default) ゆえ Vulkan path 到達せず、silent skip 経路は dead code

→ PB-5.1〜.13 13 method 全部で **`mUseUBO=true` 時の silent skip return** + **`mUseUBO=false` 時 OpenGL path 維持** の 2 経路 1 entry point pattern。

---

## §4 残 sub-step strict 線形 (= PB-5.1〜.13 batch 着手 prep 反映)

```
PB-5.1〜.13 batch (= 本 session 着手)
  → PB-4.8 + PB-5.14 統合 sub-step (= uniform4iv 整数 + hashed、(U4) design 判断 AYA 待ち)
  → PB-7 (= mapUniforms() 末尾 debug build llassert 整合 check)
  → PB-N (= Phase 1.B Exit Criteria 検証、AYA と一緒に実行)
```

**注**: PB-4.8 + PB-5.14 を統合 sub-step にする提案根拠 = 両 method 同じ (U4) design 判断要件 (= mValue block (A) 内側 / (B) 外側) で、1 回の AYA 判断で 2 method 同時確定可能。AYA 判断後 1 commit (= +約 30 line) で着手。本提案は PB-5.1〜.13 batch 完了後 別 handoff で起案。

---

## §5 self-verify 観点 (= 着手後 commit 前に確認)

| 観点 | 確認方法 | 期待 |
|---|---|---|
| (1) +195〜+205 line 物理確認 | `git diff --stat` = 1 file changed + ~200 insertions (= +15 line × 13 想定、実測 ±5 line) | hashed lookup pattern の if/auto/find/loc/cadence/cadence/forward/return + return 8 line + comment 5 line + size 計算 1 line = 14-15 line × 13 |
| (2) 挿入位置 | 各 method `LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER` 直後 (= `getUniformLocation` 直前) | 06a §5.5 spec literal 通り |
| (3) GATE-B 順守 | `git diff \| grep "^+" \| grep "LL_VULKAN_GLSL" \| grep -v "^+.*//"` = 0 件 (comment 中のみ 13 件) | OK |
| (4) MUSEUBO-A 順守 | `git diff llglslshader.h` = 0 | OK |
| (5) static verify §2.5 (2) 全 7 観点 | (1〜7) 全 PASS | OK |
| (6) 残 sub-step strict 線形維持 | §4 table = PB-4.8+PB-5.14 統合 → PB-7 → PB-N | OK |
| (7) batch 化条件 4 件 | (1) call site 追加のみ + (2) 構造変更なし + (3) 設計判断なし + (4) AYA 指示「PB-5.1〜.13 で 1つの handoff」literal | 全充足 |
| (8) working tree = 1 file のみ | `git status` = M indra/llrender/llglslshader.cpp のみ | OK |
| (9) Co-Authored-By 不在 | commit message 行頭 `Co-Authored-By:` 0 件 | OK |
| (10) 14 vs 13 整合性確定明文化 | commit message に「PB-5.14 (uniform4iv hashed) = deferred (U4) design 判断 AYA 待ち」記述 | OK |

---

## §6 引き継ぎ済 memory (= PB-4.14〜.17 handoff 継承、追加なし)

- `project_r41_phase1b_vulkan_host_gate.md` — Phase 1.B 全 sub-step で C++ 側 `#ifdef LL_VULKAN_GLSL` 不使用、runtime `mUseUBO` flag 単独 gate
- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ厳守
- `feedback_no_scope_shrink` — Phase 1.B Exit Criteria literal「30 setter 全部」厳守、本 batch は 13/14 = scalar/vector/matrix 全展開、deferred 1 件 (PB-5.14) のみ AYA 判断要件で除外
- `feedback_self_verify_before_handoff` — commit 前 10 観点 self-verify
- `feedback_no_claude_coauthor` — 全 commit 共著行不在
- `feedback_one_step_at_a_time` — strict 線形進行
- `feedback_ubo_migration_one_at_a_time` — **射程例外運用 4 条件 (= (1) call site 追加のみ + (2) 構造変更なし + (3) 設計判断なし + (4) AYA 提案/同意) 充足時のみ batch 化可、本 batch は AYA 指示「PB-5.1〜.13 で 1つの handoff」literal で 4 条件全充足**
- `feedback_doubt_self_first` — build error / link fail は AYA 投げ前に Claude root cause 特定
- `feedback_no_auto_commit` — handoff §7 1 line 自走指示後は AYA 明示指示なしで commit 可
- `feedback_remove_verification_logs` — commit 前 LL_INFOS hook 除去
- `feedback_self_bug_no_defer_option` — PB scope 外の他人 bug (= viewer_manifest.py mv -T 無し等) は本 sub-step で触らない、別 issue で

---

## §7 本 session 着手 1 line

**「前 session で Phase 1.B PB-4.14〜.17 batch 4 method 完了 (commit `d28b2ecf14`、+56 line、build EXIT 0 tar.xz 197 MB)。本 session = **AYA 判断不要 自走** = **PB-5.1〜.13 batch 物理着手** = LLStaticHashedString 経路 13 method (= uniform1i/1iv/2i/1f/2f/3f/4f/1fv/2fv/3fv/4fv/4uiv hashed + uniformMatrix4fv hashed = 14 hashed methods 中 uniform4iv hashed (PB-5.14) を (U4) design 判断 AYA 待ちで除外した残 13) の連続 Vulkan path 分岐追加 = 各 method 先頭 `LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER` 直後 `getUniformLocation` 直前に §2.4 通り 3 sub-pattern (scalar 7 / ptr 6 / matrix 1) を分類 apply、13 連 Edit → 1 commit → full viewer build 1 回、初回 fail 時 `rm -rf build-linux-x86_64/newview/packaged` 後 llpackage 再走で救済)、共通 pattern = `mUniformUBOLocByHash.find(static_cast<U64>(uniform.Hash()))` map lookup + cadence skip + `forwardToUboUpload(loc, data, size)` + silent skip return、各 sub-pattern 差分は scalar の tmp array 宣言 / ptr の size 式 / matrix の size 16x 計算のみ、GATE-B 整合 + MUSEUBO-A 整合、06a §5.5 literal、`feedback_ubo_migration_one_at_a_time` 射程例外 4 条件全充足 (AYA 指示「PB-5.1〜.13 で 1つの handoff」literal) → PB-5.1〜.13 完了後 PB-4.8 + PB-5.14 統合 sub-step ((U4) design 判断 AYA 待ち) → PB-7 → PB-N strict 線形進行。」**
