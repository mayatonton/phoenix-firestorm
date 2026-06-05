# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-4.8 + PB-5.14 統合 sub-step 着手 prep

**作成日**: 2026-06-04
**前 session 物理 commit**:
- `1fa7555a06` (= PB-5.1〜.13 batch、LLStaticHashedString 経路 13 method、+238 line、full build EXIT 0 + tar.xz 205 MB)

**本 session 着手**: **AYA 判断起案 → 判断後 物理着手** = PB-4.8 (uniform4iv 整数) + PB-5.14 (uniform4iv hashed) 2 method 統合 Vulkan path 分岐追加 = (U4) doctrine 整合の design 判断要件 = 2 案 (A/B) のうち AYA 判断後 1 commit で機械的展開。

---

## §0 state 一行 summary

η-30 **Phase 1.B PB-4.8 + PB-5.14 統合 sub-step 着手 prep state** (= 前 commit `1fa7555a06` の続き、PB-4.7 (uniform1iv 整数) 系統で deferred した 2 method を統合一括判断、設計判断 1 件で 2 method 同時確定):

- **本 doc 目的** = PB-4.8 (uniform4iv 整数 index、line 2494) + PB-5.14 (uniform4iv hashed、line 2998 = +238 line shift 後) を **1 統合 sub-step** で着手するための (U4) design 判断起案 + 判断後機械的展開 pattern 明示
- **統合根拠** = 両 method 同じ (U4) doctrine 適用範囲 (= literal「uniform4iv は mValue cache 適用外」) + 同じ既存構造 (= mValue cache block + shouldChange + `count != 1` 条件) → **1 回の AYA 判断で 2 method 同時確定可能**
- **(U4) doctrine 起案** = `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-and-locking.md:133` literal「matrix 系 + uniform4iv は mValue cache 適用外」を、(A) mValue block **内側** 挿入 (= PB-4.7 整合、(U4) literal を「Vulkan path では bypass」と解釈) or (B) mValue block **外側** 挿入 (= (U4) integral 解釈、既存 mValue block を Vulkan path 前に bypass、PB-4.7 pattern 例外扱い) のどちらで実装するか AYA 判断要件
- **設計判断 1 件** (= AYA 判断後、Claude 物理 commit 1 件で 2 method 同時着手可)
- **想定 +約 30 line** = 2 method × ~15 line (= PB-5 batch 1 method 平均 +18 line と同等、case (A) / case (B) 共)

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。本 session 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-...-PB-4.8-PB-5.14-merged-prep.md`) | 全文 | (U4) design 判断 2 案 (A/B) + 判断後機械的展開 pattern + 挿入位置 |
| 2 | `indra/llrender/llglslshader.cpp:2473-2519` (= PB-4.7 完了 + PB-4.8 既存 既存 2 method) | PB-4.7 (uniform1iv 整数) Vulkan path 挿入 pattern (= mValue 内側) + PB-4.8 (uniform4iv 整数) 既存実装 | (A) 案根拠 (= PB-4.7 整合) 確認 + 挿入位置確定 |
| 3 | `indra/llrender/llglslshader.cpp:2998-3015` (= uniform4iv hashed 既存) + `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-and-locking.md` (U4) literal | uniform4iv hashed 既存実装 + (U4) doctrine literal | (B) 案根拠 (= (U4) literal 整合) 確認 + uniform4iv hashed 挿入位置確定 |

### §1.2 pinpoint Read 用 reference (= 必要時のみ)

| file | 参照箇所 |
|---|---|
| `indra/llrender/llglslshader.cpp:2944-2961` | PB-5.2 (uniform1iv hashed) Vulkan path 挿入 pattern (= ptr+count 系 hashed の mValue 不在 method 例、本 sub-step では既存構造差で参考のみ) |
| `indra/llrender/llglslshader.h:443-457` | `forwardToUboUpload()` 宣言 + `mUniformUBOLoc` / `mUniformUBOLocByHash` 定義 (= 整数 index 経路 / hashed 経路 共通入口) |
| `indra/llrender/llglslshader.h:427-429` | `mUseUBO = false` default 確認 (= MUSEUBO-A) |

---

## §2 (U4) design 判断起案 (= AYA 判断要件 1 件、本 doc の心臓部)

### §2.1 既存実装構造 (= PB-4.8 + PB-5.14 共通の問題核)

両 method (= uniform4iv 整数 + hashed) は **同型構造**:

```cpp
// PB-4.8 (uniform4iv 整数 index、line 2494):
void LLGLSLShader::uniform4iv(U32 index, U32 count, const GLint* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);
    if (mProgramObject)
    {
        if (mUniform.size() <= index) { LL_WARNS_ONCE... return; }
        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec((F32)v[0], (F32)v[1], (F32)v[2], (F32)v[3]);
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                glUniform4iv(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

// PB-5.14 (uniform4iv hashed、line 2998 = +238 line shift 後):
void LLGLSLShader::uniform4iv(const LLStaticHashedString& uniform, U32 count, const GLint* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);
    if (location >= 0)
    {
        LLVector4 vec((F32)v[0], (F32)v[1], (F32)v[2], (F32)v[3]);
        const auto& iter = mValue.find(location);
        if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
            glUniform4iv(location, count, v);
            mValue[location] = vec;
        }
    }
}
```

**問題核**: 両 method ともに「**mValue cache block + shouldChange + count != 1 条件**」を持つ。(U4) doctrine literal「uniform4iv は mValue cache 適用外」を Vulkan path に対してどう適用するか。

### §2.2 (U4) doctrine literal (source-of-truth)

`docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-and-locking.md:133`:
- 「matrix 系 + uniform4iv は mValue cache 適用外」
- 根拠 = matrix 系は frame 跨ぎ identity 比較が非実用的 + uniform4iv は値域広く cache hit rate 低い

### §2.3 候補 2 案 (= 比較 table)

| 観点 | (A) mValue block **内側** 挿入 | (B) mValue block **外側** 挿入 |
|---|---|---|
| **挿入位置 (PB-4.8 整数)** | `if (iter == mValue.end() || shouldChange(...) || count != 1)` block 内、`glUniform4iv` 直前 (= PB-4.7 整合) | `if (mUniform[index] >= 0)` block 内、`const auto& iter = mValue.find(...)` 直前 (= mValue 全 bypass) |
| **挿入位置 (PB-5.14 hashed)** | `if (iter == mValue.end() || shouldChange(...) || count != 1)` block 内、`glUniform4iv` 直前 | `if (location >= 0)` block 内、`LLVector4 vec(...)` 直前 (= mValue 全 bypass) |
| **(U4) literal「適用外」の解釈** | 「OpenGL path は mValue cache 維持、Vulkan path は **bypass**」(= local literal 解釈、Vulkan path は forwardToUboUpload に直行で cache 不要) | 「mValue cache は両 path で適用外」(= integral 解釈、(U4) literal を厳密に読む、mValue map 全く触らず) |
| **PB-4.7 (uniform1iv 整数) との整合** | ✅ PB-4.7 既に mValue block 内側挿入済、本 sub-step も同型で一貫性 | ❌ PB-4.7 と異なる挿入位置 (= PB-4.7 は (U4) 適用外明記なし → mValue 内側、本 sub-step は (U4) 適用 → 外側、で正当化可) |
| **既存 PB-5 batch (= scalar/ptr/matrix) との整合** | △ PB-5 13 method は method 先頭挿入 (= mValue cache 存在しても **method 先頭で bypass**、内側挿入なし)、(A) 案は **整合性違反** | ✅ PB-5 13 method は method 先頭挿入 = (B) 案 = mValue block 外側挿入と同方針 |
| **mValue map の状態** | OpenGL path 復帰時に過去 mValue 残存 (= mUseUBO=true 期間中 mValue 更新無し、OpenGL 復帰で stale cache) | OpenGL path 復帰時に mValue 残存 (同上) ※ mUseUBO 動的切替は本 milestone scope 外 |
| **OpenGL 挙動への影響 (mUseUBO=false 時)** | 0 件 (= mValue block 既存挙動 100% 維持) | 0 件 (= `if (mUseUBO)` block 走らず、mValue block 既存挙動 100% 維持) |
| **コード量** | ~15 line × 2 = ~30 line | ~15 line × 2 = ~30 line |
| **将来 OpenGL 取り込みやすさ** (= 設計原則 1) | ✅ mValue cache OpenGL 側維持で upstream OpenGL 改修との競合最小 | ✅ (同上) ※ Vulkan path 自体は両案で OpenGL touch せず |
| **将来 Core プロセス分散実現** (= 設計原則 2) | ⚠ mValue map 触らないので分散影響なし、ただし mValue 自体は thread-unsafe、cache 一貫性は別軸 | ✅ (同上) |

### §2.4 AYA 判断要件

**「(A) PB-4.7 整合 (= 整数 index 経路 1iv 系との一貫性優先) を取るか、(B) (U4) literal 整合 + PB-5 batch 整合 (= 「適用外」literal を厳密解釈 + scalar/ptr/matrix と挿入位置統一) を取るか」**

**Claude 推奨**: 判断材料を AYA さんに委ねる。両案ともに `mUseUBO=false` default 下で挙動変化 0 件、build verify は両案で PASS 想定。設計原則 (= 1 上流取り込み + 2 Core 分散) は両案で同等。差は **「(U4) literal の解釈方針」** のみ。

**注**: `feedback_self_bug_no_defer_option` 遵守 = 本 sub-step を「(U4) literal の解釈論で結論出ず先送り」しない、AYA 判断後 即着手。

---

## §3 AYA 判断後の機械的展開 pattern

### §3.1 case (A) mValue block 内側挿入 (= PB-4.7 整合) の場合

**PB-4.8 (uniform4iv 整数) 挿入** = line 2513 (`glUniform4iv(mUniform[index], count, v);`) 直前:
```cpp
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                // r41 sub-step ...η-30 Phase 1.B PB-4.8:
                // integer index 経路 Vulkan path 分岐追加。spec 06a §5.2 / §5.3 literal 準拠。
                // GATE-B = #ifdef LL_VULKAN_GLSL 不使用、mUseUBO runtime flag 単独 gate。
                // MUSEUBO-A = mUseUBO=false default で本 block 走らず既存 OpenGL 挙動 100% 維持。
                // (U4) 適用 = uniform4iv mValue cache 適用外、本実装は (A) PB-4.7 整合 (= mValue 内側、Vulkan path で cache bypass、AYA 判断 2026-06-XX)
                if (mUseUBO)
                {
                    llassert(index < mUniformUBOLoc.size());
                    const ubo::UniformLocation& loc = mUniformUBOLoc[index];
                    if (loc.cadence_tag == 0xFFFFFFFFu) return;
                    if (loc.cadence_tag == 5 /* CADENCE_SAMPLER */) return;
                    forwardToUboUpload(loc, v, count * 4 * sizeof(GLint));
                    return;
                }
                glUniform4iv(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
```

**PB-5.14 (uniform4iv hashed) 挿入** = line 3007 (`glUniform4iv(location, count, v);`) 直前 (= 内側の二重 `LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER` 後):
```cpp
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
                // r41 sub-step ...η-30 Phase 1.B PB-5.14:
                // LLStaticHashedString 経路 Vulkan path 分岐追加。spec 06a §5.5 literal 準拠。
                // GATE-B + MUSEUBO-A + (U4) 適用、本実装は (A) PB-4.7 整合
                if (mUseUBO)
                {
                    auto it = mUniformUBOLocByHash.find(static_cast<U64>(uniform.Hash()));
                    if (it != mUniformUBOLocByHash.end())
                    {
                        const ubo::UniformLocation& loc = it->second;
                        if (loc.cadence_tag == 0xFFFFFFFFu) return;
                        if (loc.cadence_tag == 5 /* CADENCE_SAMPLER */) return;
                        forwardToUboUpload(loc, v, count * 4 * sizeof(GLint));
                    }
                    return;
                }
                glUniform4iv(location, count, v);
                mValue[location] = vec;
            }
```

### §3.2 case (B) mValue block 外側挿入 (= (U4) integral 解釈) の場合

**PB-4.8 (uniform4iv 整数) 挿入** = line 2510 (`const auto& iter = mValue.find(mUniform[index]);`) 直前 (= mValue 全 bypass):
```cpp
        if (mUniform[index] >= 0)
        {
            // r41 sub-step ...η-30 Phase 1.B PB-4.8:
            // integer index 経路 Vulkan path 分岐追加。spec 06a §5.2 / §5.3 literal 準拠。
            // GATE-B + MUSEUBO-A + (U4) integral 解釈、本実装は (B) mValue 全 bypass = PB-5 batch 整合 (AYA 判断 2026-06-XX)
            if (mUseUBO)
            {
                llassert(index < mUniformUBOLoc.size());
                const ubo::UniformLocation& loc = mUniformUBOLoc[index];
                if (loc.cadence_tag == 0xFFFFFFFFu) return;
                if (loc.cadence_tag == 5 /* CADENCE_SAMPLER */) return;
                forwardToUboUpload(loc, v, count * 4 * sizeof(GLint));
                return;
            }
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec((F32)v[0], (F32)v[1], (F32)v[2], (F32)v[3]);
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                glUniform4iv(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
```

**PB-5.14 (uniform4iv hashed) 挿入** = method 先頭 `LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER` 直後 (= PB-5.1〜.13 batch と同方針):
```cpp
void LLGLSLShader::uniform4iv(const LLStaticHashedString& uniform, U32 count, const GLint* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    // r41 sub-step ...η-30 Phase 1.B PB-5.14:
    // LLStaticHashedString 経路 Vulkan path 分岐追加。spec 06a §5.5 literal 準拠。
    // GATE-B + MUSEUBO-A + (U4) integral 解釈、本実装は (B) mValue 全 bypass = PB-5 batch 整合
    if (mUseUBO)
    {
        auto it = mUniformUBOLocByHash.find(static_cast<U64>(uniform.Hash()));
        if (it != mUniformUBOLocByHash.end())
        {
            const ubo::UniformLocation& loc = it->second;
            if (loc.cadence_tag == 0xFFFFFFFFu) return;
            if (loc.cadence_tag == 5 /* CADENCE_SAMPLER */) return;
            forwardToUboUpload(loc, v, count * 4 * sizeof(GLint));
        }
        return;
    }

    GLint location = getUniformLocation(uniform);
    ... (既存 OpenGL path 維持)
}
```

### §3.3 size 式 (= 両案共通)

`count * 4 * sizeof(GLint)` (= uniform4iv は 1 element あたり 4 × GLint)

### §3.4 build verify 戦略 (= 両案共通)

| 段階 | 手順 | 期待結果 |
|---|---|---|
| (1) 2 連 Edit | PB-4.8 → PB-5.14 順に Edit、case (A) or (B) を AYA 判断後選択 | `git diff --stat` = 1 file changed + ~30 insertions |
| (2) static verify §5 全 7 観点 | (1) LL_VULKAN_GLSL コード行 0、comment 2 件 + (2) mUseUBO=true 新規 0 + (3) Co-Authored-By 不在 + (4) `mUniformUBOLocByHash.find` 追加 1 件 + (5) `mUniformUBOLoc[index]` 参照追加 1 件 + (6) forwardToUboUpload 追加 2 件 + (7) 既存 glUniform4iv line 削除 0 件 | 全 PASS |
| (3) full viewer build | `cmake --build build-linux-x86_64 -j` | EXIT 0、tar.xz 生成 (195-205 MB) |
| (4) build fail 救済 protocol | 初回 fail 時 `rm -rf build-linux-x86_64/newview/packaged && cmake --build build-linux-x86_64 --target llpackage -j` | 前 session で確立済 |
| (5) 1 commit | 2 method 統合 sub-step 1 commit、AYA style verbose、case (A) or (B) を commit message に明記 | Co-Authored-By 行不在 |

---

## §4 残 sub-step strict 線形 (= PB-4.8 + PB-5.14 統合 sub-step 着手 prep 反映)

```
✅ PB-5.1〜.13 batch (= 前 commit 1fa7555a06、13 method 完了)
  → ⏳ PB-4.8 + PB-5.14 統合 sub-step (= 本 session 着手、AYA (U4) design 判断 + 物理 commit 1 件)
  → ⏳ PB-7 (= mapUniforms() 末尾 debug build llassert 整合 check)
  → ⏳ PB-N (= Phase 1.B Exit Criteria 検証、AYA と一緒に実行)
```

---

## §5 self-verify 観点 (= 着手後 commit 前に確認)

| 観点 | 確認方法 | 期待 |
|---|---|---|
| (1) +30 ±5 line 物理確認 | `git diff --stat` = 1 file changed + ~30 insertions (= +15 line × 2 method 想定、実測 ±5 line) | OK |
| (2) AYA 判断結果 commit message 明記 | commit message に「(U4) 判断 = case (A) mValue 内側 / case (B) mValue 外側」literal | OK |
| (3) GATE-B 順守 | `git diff \| grep "^+" \| grep "LL_VULKAN_GLSL" \| grep -v "^+.*//"` = 0 件 (comment 中のみ 2 件) | OK |
| (4) MUSEUBO-A 順守 | `git diff llglslshader.h` = 0 | OK |
| (5) static verify §3.4 (2) 全 7 観点 | (1〜7) 全 PASS | OK |
| (6) 残 sub-step strict 線形維持 | §4 table = PB-7 → PB-N | OK |
| (7) working tree = 1 file のみ | `git status` = M indra/llrender/llglslshader.cpp のみ | OK |
| (8) Co-Authored-By 不在 | commit message 行頭 `Co-Authored-By:` 0 件 | OK |
| (9) AYA 判断 record 永続化 | spec 06b §3.4 / (U4) section に AYA 判断結果 + 根拠を追記 (= 本 session 末で別 commit or 本 commit 内同梱) | OK |

---

## §6 引き継ぎ済 memory (= PB-5.1〜.13 complete handoff から継承、追加なし)

(= PB-5.1〜.13 complete handoff §4 と同一、本 handoff 固有の追加 memory なし)

特に重要:
- `feedback_self_bug_no_defer_option` — (U4) design 判断起案 doc の趣旨 = 「先送り」ではなく「AYA 判断 1 回で 2 method 同時着手」、judgment を AYA に投げるが implementation は本 sub-step で完了
- `feedback_ubo_migration_one_at_a_time` 射程例外運用 4 条件のうち (3) 「設計判断なし」を本 sub-step は **違反** (= (U4) 判断要件)、batch 化適用外、1 sub-step 1 commit で完了 (= 2 method 統合は (U4) doctrine が共通の根拠で 1 判断 = 1 sub-step として扱う)

---

## §7 本 session 着手 1 line

**「前 session で Phase 1.B PB-5.1〜.13 batch 13 method 完了 (commit `1fa7555a06`、+238 line、build EXIT 0 tar.xz 205 MB)。本 session = **AYA (U4) design 判断起案 → 判断後 自走物理着手** = PB-4.8 (uniform4iv 整数 index、line 2494) + PB-5.14 (uniform4iv hashed、line 2998 = +238 line shift 後) 2 method 統合 sub-step = (U4) doctrine literal「uniform4iv は mValue cache 適用外」を、(A) mValue block **内側** 挿入 (= PB-4.7 整合) or (B) mValue block **外側** 挿入 (= (U4) integral 解釈 + PB-5 batch 整合) のどちらで実装するか AYA 判断要件 (= §2.3 比較 table、両案ともに `mUseUBO=false` default 下で挙動変化 0 件、設計原則 1/2 同等、差は (U4) literal 解釈方針のみ)、判断後 §3.1 (case A) or §3.2 (case B) 通り 2 連 Edit → 1 commit → full viewer build 1 回、共通 size 式 = `count * 4 * sizeof(GLint)`、GATE-B 整合 + MUSEUBO-A 整合、06a §5.2/§5.3 (整数 index) + §5.5 (hashed) literal、`feedback_ubo_migration_one_at_a_time` 射程例外 4 条件のうち (3) 設計判断なし 違反 = batch 化適用外、1 sub-step 1 commit で完了 → PB-4.8 + PB-5.14 完了後 PB-7 (mapUniforms() llassert 整合) → PB-N (Exit Criteria) strict 線形進行。」**
