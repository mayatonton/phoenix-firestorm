# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-ε 完遂 → 次 sub-bundle (B?-ζ) 着手境界 handoff (2026-06-01)

**parent commit**: `ed98ed0131` (B?-ε patch、本 handoff の直接 parent) / `744d266f34` (B?-δ patch 範式継承元) / `756673627e` (B?-δ-complete handoff doc commit)
**HEAD**: `ed98ed0131` on `feature/ayastorm-r41-gl-removal`
**本 doc 位置付け**: B?-ε (a) scope 完遂状態 + 次 sub-bundle (推奨 B?-ζ = (b) utility 二重 attach dedup `GBufferInfo` redefinition 215 件 + (c) opaque uniform `layout(set,binding)` qualifier 注入 8 件) 着手判断境界 を fresh context 引継 用に確定する doc-only handoff。B?-δ-complete `756673627e` 範式継承。**utility 境界 `\n` 終端 invariant 強化 範式 1 件を B?-ε で新規確立**。`'#' preprocessor directive cannot be preceded by another token` 191 件の **broadcast 解消** (= 1 hook で ALL 191 件 cluster を一括 clear) 達成、`feedback_one_step_at_a_time` 範式厳守の最小局所修正 (1 file +11/-0) 範例。AYA 「OK」明示承認下で commit (no auto-commit)。

---

## §1 起草目的

β-2-bundle-B scope 第五 sub-bundle B?-ε の (a) scope (utility source concat 末尾 `\n` 補正) を **1 file +11/-0 insertions-only** で完遂した状態を確定し、次 sub-bundle 着手境界 を fresh context に引継ぐ。B?-δ (commit `744d266f34` = utility unguarded leak wrap) の cascade exposure 第2層で露出していた **`'#' preprocessor directive cannot be preceded by another token` 191 件** (全て `0:127` 集中) を、**utility source concat path での 1 hook 追加 (`\n` 担保) で 100% 解消**。同時に cascade exposure 第3層 emergence (`'GBufferInfo' redefinition struct` 24→**215** = +191 件、191 件の program が次の error で abort し直し) が次 sub-bundle scope として顕在化。

本 sub-bundle は **B?-δ §3.2 cascade source 特定範式** (ALL N errors 同一 `0:LINE` 集中 ⇒ ALL N programs 最先頭 attached utility 境界が cascade source) を実用適用した実例:
- ALL 191 errors 同一 `0:127` ⇒ utility 境界 (前 utility 末尾 + 次 utility 先頭 `#` directive) が cascade source
- root location 確定後、`LLGLSLShader::generatePerProgramSPIRV()` 内 utility source concat loop に **append 直後の末尾改行有無検査 + 欠落時のみ `\n` 補正** 1 hook 追加で broadcast 解消

---

## §2 B?-ε (a) 完遂 status

| 項目 | 値 |
|---|---|
| Scope | (a) `LLGLSLShader::generatePerProgramSPIRV()` 内 utility source concat loop で各 `util_sources[i]` append 直後の末尾 `\n` 担保 (cache 内 entry 自体は不変、concat path のみ補正) |
| 修正 file 数 | **1 file** (`indra/llrender/llglslshader.cpp`) |
| 変更行数 | **+11 / -0** (insertions-only / comment 7 行 + code 4 行) |
| 修正範囲 | line 814-817 (旧 concat loop 4 行) → line 814-828 (新 concat loop 15 行)、`concatenated.append(util_sources[i])` 直後に `if (!util_sources[i].empty() && util_sources[i].back() != '\n') { concatenated.append("\n"); }` 4 行 + 範式コメント 7 行を挿入 |
| Agent 投入 | **0 件** (root cause が B?-δ §3.2 範式適用で確定済 = utility 境界 cluster、Agent 不要 = 局所 surgical patch) |
| shader file 触り | **0 件** (本 sub-bundle は C++ 1 file 修正のみ、A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ/B?-δ 既処理 file 全 byte-for-byte 維持) |
| AYAstorm 改変保全 | **GL path 全不変** (`glCreateShader`/`glShaderSource`/`glCompileShader`/`glAttachShader` 全不変、`loadShaderFile` strdup 不変、charter §3 #1 acceptance) |
| AYA cold cache launch verify | **PASS** (起動成立 12:46:27 cache 再生成 224 shaderbin + clean shutdown 12:47:42 + Goodbye! 1 件 + Vulkan device/instance destroyed 各 1 件 + status: stopped + 実 FATAL/SIGSEGV/Aborted 0 件) |
| commit | `ed98ed0131` (AYA 「OK」明示指示下 2026-06-01) |
| metric vs B?-δ baseline `#` preprocessor | **-191 ✓ B?-ε (a) patch 効果完全直接観測** (191→0, 100% broadcast 解消) |
| metric vs B?-δ baseline `GBufferInfo` redefinition | **+191 cascade exposure 第3層 emergence** (24→215、191 件 program が `#` preprocessor abort 解消で次 error で abort し直し) |
| metric vs B?-δ baseline opaque `binding` | ±0 (8→8 維持) |
| metric vs B?-δ baseline non-opaque uniforms | ±0 (0→0 維持、B?-δ 主指標達成 retain) |
| metric vs B?-δ baseline parse failed | ±0 (223→223、cascade 1:1 = 215 + 8 内訳シフト) |
| metric vs B?-δ baseline link failed | ±0 (0→0 維持) |
| metric net delta | **-191 errors** (B?-ε (a) scope = `#` preprocessor 単指標で broadcast 解消、`GBufferInfo` cascade emergence は次 sub-bundle scope として独立計上) |

### §2.1 既処理 sub-bundle との関係

| sub-bundle | 関係 |
|---|---|
| A1-A7 | uniform/sampler/UBO block 注入 (binding scope)、本 step touch 0 件、既存 UBO byte-for-byte 維持 |
| A8-recovery | AYAstorm 改変 5 file UBO 復活、本 step も skip list 維持で untouched |
| B1 | materialF.glsl MaterialUBO_Legacy 化 (case 2 file-local override)、本 step touch 0 件 |
| B2-α | varying + fragment_out 全 program 注入、本 step touch 0 件 |
| B2-β | vertex_in/VBO attribute 全 program 注入、本 step touch 0 件 |
| B3 | SPIR-V Vulkan profile override per-stage prepend、本 step も B3 範式の `#version 460 + #extension + LL_VULKAN_GLSL` 直後 prepend 経路をそのまま継承 |
| B2-γ | utility source cache + per-program attached utility tracking + utility concat hook + createShader reorder、**本 step は B2-γ §3.3 concat hook 範式に `\n` 終端 invariant 強化を追加適用** = 直接補強関係 |
| B?-δ | utility unguarded bare uniform wrap (12 file +167)、本 step touch 0 件、**§3.2 cascade source 特定範式を本 step で実用適用** = 範式適用関係 |
| B?-ε (a) 本 sub-bundle | utility source concat 末尾 `\n` 担保 = utility 境界 token 重なり cascade source を 1 hook で broadcast 解消 |

---

## §3 設計範式 (B?-ε で新規確立)

### §3.1 Utility 境界 `\n` 終端 invariant 強化範式 (B?-ε で新規確立)

**設計原則**: `LLGLSLShader::generatePerProgramSPIRV()` 内で utility source を concat 経路で `concatenated.append(util_sources[i])` で append する際、各 entry の末尾改行有無を検査して欠落時のみ `\n` を補う:

```cpp
for (size_t i = 1; i < util_sources.size(); ++i)
{
    concatenated.append(util_sources[i]);
    if (!util_sources[i].empty() && util_sources[i].back() != '\n')
    {
        concatenated.append("\n");
    }
}
```

**Why critical**: glslang は preprocessor directive (`#ifdef`, `#endif`, `#version`, `#define` 等) が **前行末 token と同行扱い** になった場合、`'#' : preprocessor directive cannot be preceded by another token` で reject する。utility shader (例 `globalF.glsl`, `objectSkinV.glsl`, `atmosphericsV.glsl`) は preprocessed source として cache 化 (B2-γ §3.1) 後 concat されるが、preprocessor が出力する source の末尾 `\n` 有無は file 内容次第で保証されない。utility 境界 (前 utility 末尾 + 次 utility 先頭) で次 utility の最初 line が `#` directive で始まる場合、前 utility 末尾の最後 token と同行扱いになり cascade abort。

**設計判断 (案 A vs 案 B)**:
- 案 A (本 patch) = concat hook 内 append 直後の局所補正 (`llglslshader.cpp` 1 file 数行)
- 案 B = cache populate 時 (`llshadermgr.cpp::loadShaderFile()` 内) 各 entry 末尾 `\n` 終端保証 (`llshadermgr.cpp` 1 file 数行)

**案 A 選択理由**:
1. 副作用範囲が concat path 内のみで最小 (cache 内 invariant 不変、cache 内容 = shader_code_text[i] copy のまま)
2. cache 内 entry は元 string と byte-for-byte 一致 (将来 cache 用途拡張時の invariant 保持)
3. 同じ手法を後段 `stages[idx].sources` (program-specific source) にも対称適用可能 (cascade exposure 第3層 emergence で program-specific path にも `#` preprocessor が出るなら同 hook で対称補正可能)

**範式継承元**: B2-γ §3.3 utility concat hook + createShader reorder 範式 の **末尾 `\n` invariant 強化**。B2-γ は utility source を SPIR-V concat に乗せる仕組み確立、B?-ε §3.1 は乗せた source 間の境界整合性確保。

**範式継承先想定**: 次 sub-bundle B?-ζ で `GBufferInfo` redefinition cascade 解消後、cascade exposure 第4層で program-specific stage concat の `#` preprocessor cluster が露出した場合、同 hook を line 821-828 の `for (size_t idx : stage_indices)` ループ内にも対称適用。

### §3.2 patch literal (`indra/llrender/llglslshader.cpp` line 814-828)

```cpp
const std::vector<std::string>& util_sources = it->second;
for (size_t i = 1; i < util_sources.size(); ++i)
{
    concatenated.append(util_sources[i]);
    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-ε: 各 utility source entry
    // 末尾の '\n' 担保。glslang は preprocessor directive ('#ifdef' 等) が
    // 前行末 token と同行扱いになると "preprocessor directive cannot be
    // preceded by another token" で reject する。utility 境界 (前 utility
    // 末尾 + 次 utility 先頭) でこの状態が ALL 191 件 0:127 cascade として
    // 現れたため、append 直後に末尾改行有無を検査して欠落時のみ "\n" を
    // 補う (cache 内 entry 自体は不変、concat path のみ補正)。
    if (!util_sources[i].empty() && util_sources[i].back() != '\n')
    {
        concatenated.append("\n");
    }
}
```

挿入位置: `LLGLSLShader::generatePerProgramSPIRV()` 内、utility cache lookup → util_sources 取得 → append loop。`#version 460\n` + `#extension GL_KHR_vulkan_glsl : enable\n` + `#define LL_VULKAN_GLSL 1\n` の prepend 直後、`for (size_t idx : stage_indices)` の program-specific stage concat の直前。

---

## §4 AYA cold cache launch verify metric

baseline = `AYAstorm.old.b-epsilon-pre-utility-concat-fix` (B?-δ verify 後 = B?-δ baseline 同等)、after = `AYAstorm.log` (B?-ε (a) cold verify 後 12:46:27-12:47:42)。

| metric | baseline | after | delta | 評価 |
|---|---|---|---|---|
| `'#' preprocessor directive cannot be preceded by another token` | 191 | **0** | **-191** | ✓ B?-ε (a) 直接効果 (100% broadcast 解消、主指標完全達成) |
| `'GBufferInfo' : redefinition struct` | 24 | **215** | **+191** | cascade exposure 第3層 emergence (§4.1 詳細、次 B?-ζ (b) scope) |
| `'binding' : sampler/texture/image requires layout(binding=X)` | 8 | 8 | ±0 | 維持 (次 B?-ζ (c) scope) |
| `non-opaque uniforms outside a block` | 0 | 0 | ±0 | B?-δ 達成維持 |
| `glslang parse failed for stage` | 223 | 223 | ±0 | cascade 1:1 (215 `GBufferInfo` + 8 opaque `binding` = 223) |
| `glslang link failed for program` | 0 | 0 | ±0 | 維持 |
| `'#version' must occur first` cluster | 0 | 0 | ±0 | B3 既処理状態維持 |
| `location` cluster | 0 | 0 | ±0 | B2-α 既処理状態維持 |
| `binding` cluster (UBO 関連) | 0 | 0 | ±0 | A1-A7 既処理状態維持 |
| `missing #endif` cluster | 0 | 0 | ±0 | B2-γ 既処理状態維持 |
| `No function definition` cluster | 0 | 0 | ±0 | B2-γ 既処理状態維持 |
| shader_cache 件数 | 224 | 224 | ±0 | B?-δ baseline と同等 (B2-γ baseline 245 比 -21 観測点 unchanged) |
| Goodbye! 件数 | 1 | 1 | ±0 | clean shutdown 維持 |
| Vulkan device destroyed | 1 | 1 | ±0 | clean shutdown 維持 |
| Vulkan instance destroyed | 1 | 1 | ±0 | clean shutdown 維持 |
| status: stopped | 1 | 1 | ±0 | clean shutdown 維持 |
| FATAL / SIGSEGV / Aborted 実件数 | 0 | 0 | ±0 | safe |

### §4.1 Cascade exposure 第3層 分類 (次 B?-ζ scope 候補)

B?-ε (a) で `'#' preprocessor` 191 件を broadcast 解消した結果、glslang が parse でより深く進めるようになり、**preexisting だったが今まで line 127 で abort されて見えていなかった** 別 1 種 leak が露出 (= 同 191 program 集合が `GBufferInfo` redefinition で fail し直し):

| 件数 | line 例 | エラー種 | 構造原因 | 想定対処 (B?-ζ 案) |
|---|---|---|---|---|
| **215** | 0:141 / 0:159 / 0:160 / 0:161 等 (分散) | `'GBufferInfo' : redefinition struct` | utility 二重 attach (例 `gbufferUtil.glsl` を `attachShaderFeatures()` 経由で複数回 attach、または異なる utility 内で同 struct 定義が重複) → struct 定義二重出現 | (b) `mVulkanAttached{Vertex,Fragment}Utilities` push_back 直前に既 contain 判定追加 (set 化 or `std::find` dedup check) + または該当 utility (`gbufferUtil.glsl` 等) 内 `#ifndef GBUFFER_INFO_DEFINED` guard 追加 |
| 8 | 0:161 等 | `'binding' : sampler/texture/image requires layout(binding=X)` | Vulkan 仕様: opaque uniform (sampler2D 等) も `layout(set=N, binding=M) uniform sampler2D ...;` の qualifier 必須、bare `uniform sampler2D xxx;` は reject | (c) 該当 utility/program file の `^uniform sampler/image/texture` 行に `layout(set=N, binding=M)` qualifier 注入 (per-tree scan、A 範式の opaque uniform 版、set=2/binding=N+ または set=3/binding=12+ 範囲割当) |

総 cascade exposure 第3層 件数 = 215 + 8 = **223 件** が次 sub-bundle B?-ζ scope (= B?-δ-complete handoff §10 の B?-ε (b)+(c) 残 work)。

**重要**: `GBufferInfo` redefinition の line 番号は分散 (`0:141`, `0:159`, `0:160`, `0:161` 等) で B?-δ §3.2 cascade source 特定範式 (ALL N errors 同一 `0:LINE` 集中) は **直接適用不可** = 複数 program 間で attach 順序が異なる結果、再定義検出位置が program 毎に異なる可能性。代替 trace 範式 = utility file の `struct GBufferInfo` 定義行 grep + `attachShaderFeatures()` 内 attach 経路 trace + per-program utility list の重複 entry 検出。

### §4.2 主指標 metric integrity self-check (B3 §12 範式継承)

literal pattern grep のみ採用 (structural touch consistency / 3 段階 pair-grep 単調性):

- `'#' preprocessor directive cannot be preceded by another token`: B?-ε after verify log 0 件 = `grep -c "preprocessor directive cannot be preceded" AYAstorm.log` 直接観測値 (calculated value ではない)
- baseline 191 件: B?-δ verify log `grep -c "preprocessor directive cannot be preceded" AYAstorm.old.b-epsilon-pre-utility-concat-fix` 直接観測値
- delta -191: 単純減算、`GBufferInfo` cascade emergence の +191 と数値的に対称だが、これは literal grep 結果同士の偶然の対称性であって compose-aware net delta としては別 source pattern として独立記録
- `GBufferInfo` 24→215: literal grep `grep -c "'GBufferInfo' : redefinition struct"` (`'`/`:` 含む完全一致)
- opaque `binding` 8→8: literal grep `grep -c "'binding' : sampler/texture/image requires layout(binding=X)"`

範式違反検出: `parse failed` 223→223 ±0 を「主指標と独立に」報告、混同して net delta を主指標に compose しない。`-191 ('#' preprocessor) + +191 ('GBufferInfo' emergence) = ±0 (parse failed)` の対称性は **literal grep 結果同士の偶然** であり、cascade exposure 第3層 emergence は独立計上が原則。

---

## §5 self-verify (commit 前最終確認)

| # | 検証項目 | 結果 |
|---|---|---|
| 1 | 修正 1 file 全て insertions-only (削除 0、修正範囲外 byte-for-byte 維持) | ✓ git diff --stat = +11/-0 |
| 2 | A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ/B?-δ 既処理 file 全 byte-for-byte 不変 | ✓ shader file touch 0、C++ side touch も llglslshader.cpp 1 file の utility concat loop 内のみ |
| 3 | GL path 全不変 (`glCreate*`/`glShaderSource`/`glCompile*`/`glAttach*`/`loadShaderFile strdup`) | ✓ 修正範囲は `LLGLSLShader::generatePerProgramSPIRV()` 内、Vulkan path のみ呼出される hook 内 |
| 4 | skip list 13 + A2 拡張 skip 2 + 5 V skip untouched | ✓ shader file touch 0 (本 step は C++ 1 file 修正のみ) |
| 5 | `\n` 補正 hook の挿入位置が utility concat loop 内 (program-specific stage concat 前) | ✓ line 814-828 `for (size_t i = 1; i < util_sources.size(); ++i)` 内 |
| 6 | charter §3 #1 acceptance (Vulkan path 内部追加のみ、CPU 側 binding 接続 touch 0) | ✓ generatePerProgramSPIRV() = Vulkan path のみ呼出される、共通 hook 内追加 |
| 7 | AYA cold cache launch verify PASS (clean shutdown + FATAL/SIGSEGV/crash 0 件) | ✓ 12:46:27 起動 / 12:47:42 終了 / Goodbye! 1 / Vulkan destroyed 各 1 / status: stopped |
| 8 | shader_cache 再生成確認 (cold cache = 完全 clear から再構築) | ✓ 224 shaderbin re-built |
| 9 | 主指標 `'#' preprocessor directive cannot be preceded` 191→0 直接観測 | ✓ literal grep count |
| 10 | cascade exposure 第3層 (`GBufferInfo` redefinition 24→215 + opaque `binding` 8→8) を §4.1 で次 B?-ζ scope として独立記録、本 sub-bundle scope と混同せず | ✓ §4.1 確立 |
| 11 | commit 前 AYA 「OK」明示指示確認 | ✓ 2026-06-01 AYA「OK」 |

---

## §6 設計範式継承表

| 範式 | 由来 sub-bundle | 本 B?-ε 適用箇所 |
|---|---|---|
| `feedback_doubt_self_first` (効かない時はまず自分のコード/仮説を疑う) | feedback memory | 該当なし (B?-δ §3.2 範式で root location 確定済、当てずっぽう試行なし) |
| `feedback_build_only_verified` (正しいことを積み上げる) | feedback memory | 1 hook 追加で 191 件 broadcast 解消を実機検証 = literal grep 確認後 commit |
| `feedback_one_step_at_a_time` (検証手順は1ステップずつ) | feedback memory | AYA 3 案 ((a)単独/(a)+(b)/(a)+(b)+(c)) 提示後「a」選択 = (a) 単独で進行、scope shrink 違反なし |
| `feedback_no_auto_commit` (コミットは明示指示があるまでしない) | feedback memory | patch 完了 + verify 完了 + metric 報告 → AYA 「OK」明示指示後 commit (`ed98ed0131`) |
| `feedback_no_claude_coauthor` (Co-Authored-By: Claude を付けない) | feedback memory | commit message 末尾 Claude 共著行なし |
| `feedback_no_scope_shrink` (AYA 指示 literal scope を勝手に縮小しない) | feedback memory | AYA「a」指示 = (a) scope literal 厳守、(b)(c) 含まず |
| `feedback_falsification_as_progress` (全 REJECT verdict は生き残りルート絞り込みの成果) | feedback memory | 該当なし (本 step 1 attempt で達成、falsification なし) |
| B1 §3 範式: case 2 file-local UBO override 3-段 swap pattern | B1 patch | 該当なし (本 step は shader file touch 0) |
| B2-α §3.1: canonical Table location 順 cross-file 固定 | B2-α patch | 該当なし |
| B3 §3.2: GL profile / Vulkan profile 分離範式 | B3 patch | concat hook 全体が Vulkan profile prepend 経路 = B3 範式の延長 |
| B3 §12: metric 整合性 self-check 範式 (literal pattern grep のみ採用) | B3 patch 訂正 | §4.2 で literal grep 採用、compose-aware net delta を主指標に混同せず |
| B2-γ §3.1: Vulkan utility source cache 範式 | B2-γ patch | 本 step は utility source cache 内容に lookup する concat 経路を直接 patch、cache 内容自体は不変 |
| B2-γ §3.2: per-program attached utility tracking 範式 | B2-γ patch | concat 経路の前提 (per-program 順序保持 utility list) を継承 |
| B2-γ §3.3: utility concat hook + createShader reorder 範式 | B2-γ patch | **本 patch の直接補強対象** = concat hook 内 append 直後に `\n` 終端 invariant 強化 |
| B?-δ §3.1: utility 既 attach 全 file scope unguarded bare uniform 網羅 scan 範式 | B?-δ patch | 該当なし (本 step は shader file scan 不要 = C++ hook 1 行) |
| B?-δ §3.2: cascade source 特定範式 (ALL N errors 同一 0:LINE 集中 ⇒ root cluster) | B?-δ patch | **本 patch の root location 確定根拠** = ALL 191 errors 同一 `0:127` cluster ⇒ utility 境界 cascade source 確定 |

---

## §7 risks (B?-ε 完遂後 / B?-ζ 着手前)

| # | risk | 評価 |
|---|---|---|
| 1 | `\n` 補正 hook が utility source 末尾既に `\n` 終端の場合に重複 `\n` を追加 → glslang parse 影響 | **極低** (`util_sources[i].back() != '\n'` 条件で欠落時のみ補正、既 `\n` 終端 entry は touch 不要、cold cache launch verify PASS = glslang parse 通過確認済) |
| 2 | `\n` 補正 hook が `program-specific stage concat path` (line 821-828 `for (size_t idx : stage_indices)`) には適用されておらず、将来 program-specific source 末尾 `\n` 欠落 cascade exposure 第4層 emergence の可能性 | **低** (現状 cascade exposure 第3層 = `GBufferInfo` redefinition + opaque `binding`、program-specific source の `#` preprocessor cluster 露出は未観測、B?-ζ 完遂後再評価) |
| 3 | cascade exposure 第3層 (`GBufferInfo` 215 件 + opaque `binding` 8 件 = 223 件) のうち `GBufferInfo` 215 件が解消困難なら program count base 進捗が停滞 | **中** (215 件は 191 件の `#` preprocessor cluster 解消後の cascade emergence で構造的に同 program 集合 = utility 二重 attach 問題、`gbufferUtil.glsl` 等の具体 utility 特定 + dedup check 1 hook 追加で broadcast 解消見込みだが、attach 順序 trace 要、Agent (Explore) 投入推奨) |
| 4 | `\n` 補正で concatenated source が長くなり SPIR-V cache size が膨張 → memory 圧迫 | **極低** (1 entry あたり最大 1 byte 追加、224 shaderbin × N utility × 1 byte = ~数 KB 累積、無視可能) |
| 5 | utility source cache 内 entry の `util_sources[0]` (= GL profile `#version` skip 対象) の末尾改行も検査されるかの仕様問題 | **低** (本 patch の loop は `i = 1` から開始 = `util_sources[0]` は touch せず、GL profile `#version` skip 範式 (B2-γ §3.3) と整合) |
| 6 | cascade exposure 第3層 emergence の +191 件が「191 件の program が次の error で abort し直し」という仮説の検証 | **低** (literal grep 結果 191 件減 (`#` preprocessor) + 191 件増 (`GBufferInfo`) の対称性は cascade exposure 第3層 emergence の典型 pattern、但し literal grep 結果同士の偶然の対称性であり原理保証ではない、B?-ζ で `GBufferInfo` 解消時に逆検証) |
| 7 | B?-δ §3.2 cascade source 特定範式の `GBufferInfo` 215 件への直接適用不可 (line 分散) | **中** (line 番号分散 = ALL N errors 同一 `0:LINE` 集中の前提が崩れる、代替 trace = `struct GBufferInfo` 定義行 grep + attach 経路 trace + per-program utility list dedup 検出が必要、B?-ζ scope 設計時に Agent 投入推奨) |
| 8 | shader_cache 件数 (224) が B2-γ baseline (245) より 21 件少ない状態が persist | **低** (B2-γ-complete handoff §12 / B?-δ-complete handoff §11 既記録の現象が継承、本 step 中も 224 維持 = 退行なし、致命的でなく次 sub-bundle 着手前 trace 推奨枠扱い) |
| 9 | A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ/B?-δ 既処理 file の C++ side touch 0 を literal verify 必要 | **低** (本 step は `llglslshader.cpp` 1 file 内 `generatePerProgramSPIRV()` 内のみ修正、他 file touch 0 = git diff --stat literal verify 済) |
| 10 | 範式継承 (B2-γ §3.3 utility concat hook 範式) の補強範囲が今後の concat hook 拡張 (program-specific path 等) で誤伝承される可能性 | **低** (本 handoff §3.1 で「program-specific path への対称適用は cascade exposure 第4層露出時のみ」と明示、`feedback_one_step_at_a_time` 範式継承で予防的拡張なし) |

---

## §8 commit メッセージ literal (`ed98ed0131`)

```
feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-ε 完遂 ((a) scope = utility source concat 末尾 '\n' 補正、'#' preprocessor directive cannot be preceded by another token 191→0 完全 broadcast 解消、1 file +11/-0 insertions-only、indra/llrender/llglslshader.cpp 編集 shader file touch 0、Agent 0、修正範囲 = LLGLSLShader::generatePerProgramSPIRV() 内 utility source concat loop (line 814-817 → 814-828)、各 util_sources[i] append 直後に末尾改行有無を検査して欠落時のみ "\n" 補正 1 hook 追加 (cache 内 entry 自体は不変、concat path のみ補正)、設計範式継承 = (1) B2-γ §3.3 utility concat hook + createShader reorder 範式の '\n' 終端 invariant 強化 + (2) B?-δ §3.2 cascade source 特定範式 (ALL 191 errors 同一 0:127 集中 ⇒ ALL 191 programs の utility 境界 token 重なりが cascade source) で root location 確定後の局所 surgical patch、metric vs B?-δ baseline = '#' preprocessor 191→0 (-191 ✓ B?-ε (a) 直接効果完全達成) + 'GBufferInfo' redefinition struct 24→215 (+191 cascade exposure 第3層 emergence = 191 件 program が次の error で abort し直し) + 'binding' sampler/texture/image 8→8 (±0 維持) + non-opaque uniforms outside a block 0→0 (±0 B?-δ 達成維持) + parse failed for stage 223→223 (±0 cascade 1:1 = 215 GBufferInfo + 8 opaque binding) + link failed 0→0 (±0) + #version/location/binding(UBO)/missing #endif/No function definition 全 0 維持、charter §3 #1 担保 (Vulkan path 内部のみ概念追加、GL path glCreateShader/glShaderSource/glCompileShader/glAttachShader 全不変、loadShaderFile strdup 不変、LLVKLoader::isVulkanInitialized() guard なし = 共通 generatePerProgramSPIRV() 内部 = Vulkan path のみ呼出される hook 内、CPU 側 binding 接続 touch 0)、AYA cold cache launch verify PASS (起動成立 12:46:27 cache 再生成 224 shaderbin + clean shutdown 12:47:42 + Goodbye! 1 件 + Vulkan device/instance destroyed 各 1 件 + status: stopped + 実 FATAL/SIGSEGV/Aborted 0 件)、A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ/B?-δ 既処理 file 全 byte-for-byte 維持 (shader file touch 0、本 step は C++ 1 file 修正のみ)、shader_cache 224 件 B?-δ baseline 同等維持 (-21 観測点 unchanged)、skip list 13 + A2 拡張 skip 2 + 5 V skip untouched、cascade exposure 第3層 = GBufferInfo redefinition 24→215 (+191) は §4.1 (B?-δ handoff) 想定通り = 次 sub-bundle B?-ζ scope (utility 二重 attach dedup) 想定継承、AYA 「OK」明示指示下 commit 2026-06-01)
```

---

## §9 build artifact

| artifact | path / state |
|---|---|
| install tree (B?-ε C++ patch 反映) | `~/ayastorm/` (本 step は C++ 1 file 修正、autobuild incremental build → install.sh deploy 経由、cold cache verify 12:46 起動時の deploy 状態) |
| shader_cache | `~/.ayastorm_x64/cache/shader_cache/` 224 shaderbin (cold cache verify 12:46 再生成) |
| baseline log | `~/.ayastorm_x64/logs/AYAstorm.old.b-epsilon-pre-utility-concat-fix` (B?-δ verify 後 = B?-δ baseline 同等、363336 byte) |
| after log | `~/.ayastorm_x64/logs/AYAstorm.log` (B?-ε (a) verify 後、401520 byte、起動 12:46:27 / 終了 12:47:42) |
| executable | `~/ayastorm/firestorm-bin` (本 step は C++ change、autobuild incremental build で再生成 = ReleaseFS_open --fmodstudio -DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE --chan AYAstorm-release) |

---

## §10 次 sub-bundle 推奨 (B?-ζ)

| 優先順位 | 候補 | scope | 想定 metric 効果 | 想定 file touch | Agent 必要性 |
|---|---|---|---|---|---|
| 1 | **B?-ζ-(b)** = utility 二重 attach dedup | `mVulkanAttached{Vertex,Fragment}Utilities` push_back 直前に既 contain 判定 (`std::find` or `std::set` dedup check) + または該当 utility (`gbufferUtil.glsl` 等) 内 `#ifndef GBUFFER_INFO_DEFINED` guard 追加 | `'GBufferInfo' redefinition struct` 215→0 級減少見込み、cascade exposure 第4層露出可能性あり (program-specific stage cluster 等) | C++ 1 file 数行 (or 該当 utility shader 1-2 file 数行) | `gbufferUtil.glsl` + 派生 utility の attach 経路 trace に Agent (Explore) 投入推奨 |
| 2 | **B?-ζ-(c)** = opaque uniform `layout(set,binding)` qualifier 注入 | 全 utility/program file `^uniform sampler/image/texture` 行に `layout(set=N, binding=M)` qualifier 注入 (per-tree scan、set=2/binding=N+ または set=3/binding=12+ 範囲割当、A3 (D) frame-global sampler 範式継承) | `'binding' sampler/texture/image requires layout(binding=X)` 8→0 + 場合により cascade 効果 | 多数 shader file (Agent 並列必須、~50-100 file 想定) | 並列必須 |
| 3 | 残 mixed cleanup | scope 未確定 (B?-ζ-(b)+(c) 完遂後の cascade exposure 第4層次第) | 不明 | 不明 | 不明 |

**B?-ζ 着手前必須事項**:
- AYA 明示指示要 (no auto-commit 範式継承)
- fresh context 推奨 (handoff doc + memory 範式継承記録から start)
- cache 完全 clear 必須 launch verify
- A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ/B?-δ/B?-ε 既処理 file 再 touch 禁止 absolute rule
- 観測点: shader_cache 件数 (現 224、B2-γ baseline 245 比 -21 が persist)、原因未 trace、致命的でない、B?-ζ 着手前 trace 推奨

**B?-ζ-(b) 着手前 trace 範式 (B?-δ §3.2 補強版)**:
- `GBufferInfo` redefinition の error line は分散 (`0:141`, `0:159`, `0:160`, `0:161` 等) で ALL N errors 同一 `0:LINE` 集中前提崩れ
- 代替 trace = (1) `struct GBufferInfo` 定義行 grep (`grep -rn "struct GBufferInfo" indra/newview/app_settings/shaders/`) → 定義 file 特定 + (2) `attachShaderFeatures()` 内 該当 file の attach 経路 trace → 二重 attach 経路特定 + (3) `mVulkanAttached{Vertex,Fragment}Utilities` 内 重複 entry 検出 logic 追加で実測

---

## §11 観測点 (B?-ε 中に検出 / 致命的でない / 次 sub-bundle 着手前 trace 推奨)

| 観測点 | 詳細 | 致命度 |
|---|---|---|
| shader_cache 件数 -21 件 persist | B2-γ baseline 245 → B?-δ cold verify 後 224 → B?-ε cold verify 後 224 (継続維持)、原因未 trace (B2-γ-complete handoff §12 / B?-δ-complete handoff §11 既記録の現象が継承) | **低** (launch 成立 + clean shutdown + FATAL 0、ただし shader 数減少 = いくつかの program が cache miss 発生中の可能性、B?-ζ 着手前に specific program 名特定推奨) |
| cascade exposure 第3層 emergence の数値対称性 (-191 / +191) | `#` preprocessor 191→0 (-191) と `GBufferInfo` 24→215 (+191) の数値が偶然対称、これは literal grep 結果同士の偶然で原理保証ではない、cascade emergence の本質 = 同 program 集合が異なる error で abort し直し、B?-ζ で `GBufferInfo` 解消時に逆検証推奨 | **低** (literal grep 結果は確定的、emergence 仮説は次 sub-bundle で実機検証) |
| `GBufferInfo` error line 分散 (`0:141`/`0:159`/`0:160`/`0:161` 等) | B?-δ §3.2 cascade source 特定範式 (ALL N errors 同一 `0:LINE` 集中) の直接適用不可 = 複数 program 間で attach 順序が異なる結果、再定義検出位置が program 毎に異なる可能性、§10 B?-ζ trace 範式 (代替 trace 3-段) で補完 | **中** (B?-ζ 着手時の trace 設計に直接影響、Agent (Explore) 投入推奨) |

---

(EOF)
