# r41 sub-step 4.3-γ'-port-β-1 完遂 → 4.3-γ'-port-β-2 着手境界 handoff (2026-06-01)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-prep.md` (case ② case-validity 部分 falsify + (α) LL_VULKAN_GLSL macro switch 採用 + bundle-A/B/C 3 bundle 構成 + 新 cadence [γ'-port-β-prep → spec-revision → β-1 exemplar PoC → β-2 base 228 file 並列 patch → β-3 PSO → β-4 descriptor → β-5 exemplar fire → β-6 build → β-7 verify → β-8 commit、推定工期 ~1 month])
**本 handoff 位置付け**: sub-step 4.3-γ'-port-β-1 (exemplar PoC = `class1/deferred/diffuse{V,F}.glsl` 2 file LL_VULKAN_GLSL macro switch + bundle-A/B 注入 + LLShaderMgr `createSPIRVFromGLSL` 局所 `#define LL_VULKAN_GLSL 1` 注入 + `file_name` parameter 追加、4 file +63/-6) **全完遂宣言** + sub-step 4.3-γ'-port-β-2 (per-program hook 昇格 + base 228 file bundle-A/B/C structural rewrite) 着手前 fresh context 引継境界。AYA launch verify PASS + (α) close 明示指示下で commit `1bce995d87` 経由 章 close。

---

## 1. sub-step 4.3-γ'-port-β-1 全完遂 status (2026-06-01)

### 1.1 完遂 marker

| acceptance | 達成 status |
|---|---|
| β-1-1 `llshadermgr.cpp` `createSPIRVFromGLSL` 内 `#define LL_VULKAN_GLSL 1` 局所注入 (Vulkan path 限定、GL path 不汚染) | ✓ |
| β-1-2 `createSPIRVFromGLSL` signature 拡張 (`file_name` parameter 追加、default `""` で後方互換) + WARN log filename 識別子化 | ✓ |
| β-1-3 hook 呼出側で `open_file_name` pass | ✓ |
| β-1-4 `diffuseV.glsl` `#ifdef LL_VULKAN_GLSL` bundle-A (PerFrame UBO `set=0,binding=0` + MaterialUBO `set=1,binding=0`) + bundle-B (`layout(location=0..3) in/out`) 注入 + GL path `#else` 保持 | ✓ |
| β-1-5 `diffuseF.glsl` `#ifdef LL_VULKAN_GLSL` bundle-A (sampler `set=1,binding=1`) + bundle-B (`layout(location=0..3) out frag_data[4]` + `in vary_*`) 注入 + GL path `#else` 保持 | ✓ |
| β-1-6 incremental autobuild (configure + build) | ✓ exit 0 / 4 file +63/-6 / warning 0 件 / packaging 完遂 |
| β-1-7 AYA launch verify | ✓ AYA 「起動 終了しました」報告 / regression 0 / crash 0 |
| β-1-8 parse pass verify (diffuseV/F.glsl 0 parse fail) | ✓ diffuseV.glsl 5 hook fire / 0 parse fail / diffuseF.glsl 1 hook fire / 0 parse fail |
| β-1-9 (α) close + commit (AYA 明示指示下) | ✓ AYA 「(α) でお願いします」+ 「commit お願いします」明示指示 / commit `1bce995d87` |
| install + cache clear 完遂 | ✓ ~/ayastorm/ + ~/.ayastorm_x64/cache/ |

### 1.2 commit hash

| commit | scope |
|---|---|
| `1bce995d87` | sub-step 4.3-γ'-port-β-1 全完遂 (4 files changed, +63/-6) |

### 1.3 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-4-3-gamma-prime-port-beta-prep.md` | active 継続 (β-2 着手時 §1 case-validity falsify + §2 (α) 設計詳細 + §3 spec 改訂 plan + §5 cadence 参照源) |
| `handoff-substep-4-3-gamma-prime-port-alpha-complete.md` | 役割完了 (γ'-port-α 完遂 marker、β-1 で次段 marker 引継ぎ) |
| sub-doc `06-shader-spirv.md` | active 継続 (β-2 で sub-step 6.3 bundle-A/B/C 228 file structural rewrite 実装 source、§1.2.4 (α) macro 注入経路 / sample pattern / binding 番号 確定値遵守) |
| sub-doc `07-descriptor-renderpass.md` | active 継続 (binding 番号確定値 set=0 binding 0=ViewProj/1=Lights/2=Atmosphere + set=1 binding 0=MaterialUBO/1=DiffuseTex/2=NormalTex/3=SpecTex + set=2 binding 0=per-draw UBO/1+=per-draw texture + push_constant 64 byte modelMatrix、bundle 単位 patch shader 側と同期) |
| sub-doc `08-llvkrenderer-skeleton.md` | active 継続 (sub-step 4.4 part B で参照) |
| `handoff-substep-4-3-gamma-prime-port-beta-1-complete.md` (本 handoff) | 新規作成 (4.3-γ'-port-β-1 全完遂 → 4.3-γ'-port-β-2 着手境界) |

### 1.4 AYA launch verify PASS metric (2026-06-01、γ'-port-β-1 build)

| metric | 値 |
|---|---|
| crash | 0 件 |
| GL path regression | 0 件 (shader_code_text 共有不汚染 = GL compile/link 全通過、`mProgramObject != 0` 全件 satisfy) |
| diffuseV.glsl hook fire | 5 件 (LL/FS shader variant 爆発 model = 同 file 5 program 経由で fire) |
| diffuseV.glsl parse fail | **0 件** ✓ |
| diffuseV.glsl link fail | 5 件 (forward decl 未解決 = per-file SPIR-V 生成の architectural mismatch、β-2 scope) |
| diffuseF.glsl hook fire | 1 件 |
| diffuseF.glsl parse fail | **0 件** ✓ |
| diffuseF.glsl link fail | 1 件 (同上) |
| shutdown | clean (Goodbye → status: stopped) |
| AYA 明示指示 | 「(α) でお願いします」(parse pass marker = β-1 close 承認) + 「commit お願いします」 |

### 1.5 (α) close 採用根拠

β-1 段階の primary goal = **PARSE PASS verification** (Vulkan GLSL 仕様 [non-opaque uniforms in block + location qualifier] satisfy 確認)。link fail は per-file SPIR-V 生成 vs multi-file shader linking の architectural mismatch (LL shader が `mirrorClip(vec3)` / `encodeNormal(...)` 等を forward declared で別 file から concat 前提) に由来し、(α) β-1 scope では構造的に解消不能 → β-2 で per-program hook 昇格時に解消。case ② case-validity の部分 reinstated (parse 段階で satisfy) を marker として β-1 close、link path は β-2 引継。

---

## 2. β-1 で確定した実装範式 (β-2 流用 base)

### 2.1 macro 注入経路 (`llshadermgr.cpp:540 付近 createSPIRVFromGLSL`)

```cpp
std::string concatenated;
if (source_count > 0 && sources[0])
{
    concatenated.append(sources[0]);
}
concatenated.append("#define LL_VULKAN_GLSL 1\n");
for (U32 i = 1; i < source_count; ++i)
{
    if (sources[i])
    {
        concatenated.append(sources[i]);
    }
}
```

- **sources[0]** = `#version 460` 等の version directive (LLShaderMgr 既存 preprocessing 経路の version 行)
- `#define LL_VULKAN_GLSL 1` を **version 直後 / preprocessor body 直前** に挿入 = GLSL preprocessor の `#define` 制約 (version 行が最初) を satisfy
- Vulkan path 専有 (`createSPIRVFromGLSL` は `LLVKLoader::isVulkanInitialized() && (type == VERTEX || FRAGMENT)` guard 下でのみ呼出 = GL path `shader_code_text` 不汚染、charter §3 #1 acceptance 担保)

### 2.2 signature 拡張 (`llshadermgr.h`)

```cpp
bool createSPIRVFromGLSL(GLenum type,
                         U32 source_count,
                         const GLchar** sources,
                         std::vector<unsigned int>& out_spirv,
                         const std::string& file_name = std::string());
```

- `file_name` parameter で parse/link fail WARN log の identifier 化 (β-1 で diffuseV/F.glsl の 0 parse fail 確認に活用、β-2 で 228 file 並列 patch 時の fail localization に必須)

### 2.3 hook 呼出 (`llshadermgr.cpp:908 付近 hook block`)

```cpp
produced = createSPIRVFromGLSL(type, shader_code_count,
                               (const GLchar**)shader_code_text, spirv,
                               open_file_name);
```

### 2.4 shader 側 `#ifdef LL_VULKAN_GLSL` 範式 (β-1 で確立)

- **bundle-A (uniform → UBO 化 + sampler binding)**:
  - per-frame uniform → `layout(set=0, binding=N) uniform <Block> { ... };` (sub-doc 07 §3.1 sub-step 7.2: binding 0=ViewProj / 1=Lights / 2=Atmosphere)
  - per-material uniform → `layout(set=1, binding=0) uniform MaterialUBO { ... };`
  - sampler → `layout(set=1, binding=M) uniform sampler2D <name>;` (sub-doc 07 §3.1 sub-step 7.3: binding 1=DiffuseTex / 2=NormalTex / 3=SpecTex)
- **bundle-B (varying → location qualifier)**:
  - vertex stage: `layout(location=N) in <type> <name>;` + `layout(location=N) out <type> <name>;`
  - fragment stage: `layout(location=N) in <type> <name>;` + `layout(location=N) out <type> <name>;`
  - vertex out ↔ fragment in は location 番号一致必須 (β-1 で diffuseV/F は 0..3 揃え確認済)
- **bundle-C (entry point + #version 460 + extension declare)**:
  - `void main()` は GLSL 既定 entry point として fire (glslang は明示指定不要)
  - `#version 460` は LLShaderMgr 既存 preprocessing で prepend 済 (sources[0])
  - extension declare は file 個別に必要時のみ追加 (β-1 で diffuseV/F は不要)
- **GL path `#else` 保持** = AYAstorm 改変 11 file untouched 維持 + 段階 1-4.3-γ'-port-α 動作維持 + charter §3 #1 acceptance 担保

### 2.5 binding 番号確定値 (sub-doc 07 §3.1 sub-step 7.2-7.4)

| set | binding | resource |
|---|---|---|
| 0 | 0 | ViewProj UBO (PerFrame 共通: modelview_projection_matrix / modelview_matrix / projection_matrix / normal_matrix) |
| 0 | 1 | Lights UBO |
| 0 | 2 | Atmosphere UBO |
| 1 | 0 | MaterialUBO (texture_matrix0 等 per-material uniform) |
| 1 | 1 | DiffuseTex (sampler2D) |
| 1 | 2 | NormalTex |
| 1 | 3 | SpecTex |
| 2 | 0 | per-draw UBO |
| 2 | 1+ | per-draw texture |
| push_constant | - | 64 byte mat4 modelMatrix (VK_SHADER_STAGE_VERTEX_BIT) |

---

## 3. sub-step 4.3-γ'-port-β-2 着手境界 (次 session)

### 3.1 β-2 scope (handoff-substep-4-3-gamma-prime-port-beta-prep.md §5 cadence 継承)

- **per-program hook 昇格**: 現状の per-file SPIR-V 生成 (`createSPIRVFromGLSL` を `loadShaderFile` 単位で呼出 = forward decl 未解決) を `LLGLSLShader::link` 時点で **全 stage の `shader_code_text` を vertex/fragment 単位で concat → 単一 `glslang::TProgram` link → SPIR-V 生成** に昇格。これにより `mirrorClip` / `encodeNormal` 等の forward decl が同 TProgram 内で解決され link 成立。
- **base 228 file bundle-A/B/C structural rewrite** (sub-doc 06 §1.2.4 (α) + §3.1 sub-step 6.3):
  - **bundle-A**: 全 195 file (LLShaderMgr 加工対象 = uniform 全件) を `#ifdef LL_VULKAN_GLSL` 分岐 + UBO 化 + sampler binding
  - **bundle-B**: 全 228 file (varying/attribute/out/in 全件) を `#ifdef LL_VULKAN_GLSL` 分岐 + location qualifier 追加
  - **bundle-C**: 全 228 file (entry point + extension declare) を必要時のみ patch
  - 各 bundle 単位で commit (`feat(r41): sub-step 4.3-γ'-port-β-2-bundle-A 完遂` 等)、Agent 並列で session 内分割実行可能だが大規模 scope なので **bundle 単位で fresh context 推奨**
- **incremental SPIR-V 生成成功率 measurement**:
  - bundle-A 単独完遂時 = ~uniform 解決のみ、link pass 数 measure
  - bundle-A+B 完遂時 = ~location 整合追加、link pass 数 measure
  - bundle-A+B+C 完遂時 = ~SPIR-V 生成成功率 >>0% 目標
- **AYAstorm 改変 11 file untouched 維持** (picker 2 / Cinematic 2 / visual realism 7) = charter §3 #1 acceptance + sub-doc 06 §3.1 sub-step 6.5 担保

### 3.2 β-2 着手前チェックリスト

- [ ] `git fetch origin` で最新確認 (feedback_git_fetch_first)
- [ ] `feature/ayastorm-r41-gl-removal` branch HEAD = `1bce995d87` 確認
- [ ] sub-doc 06 §1.2.4 (α) macro 注入経路 / sample pattern 3 件 / binding 番号 全 base file 共通既定値 を再読 (§2.1-2.5 と整合確認)
- [ ] sub-doc 07 §3.1 sub-step 7.2-7.4 binding 番号確定値 再読
- [ ] `handoff-substep-4-3-gamma-prime-port-beta-prep.md` §1 case-validity falsify 経緯 + §2 (α) 設計詳細 + §5 cadence (β-2 着手時 task 列挙) 再読
- [ ] `indra/llrender/llshadermgr.cpp` の `createSPIRVFromGLSL` 現状確認 (β-1 で `file_name` parameter 追加 + 局所 `#define LL_VULKAN_GLSL 1` 注入済)
- [ ] `indra/newview/app_settings/shaders/class1/deferred/diffuseV.glsl` + `diffuseF.glsl` で β-1 範式確認 (§2.4 bundle-A/B 注入 pattern)
- [ ] `LLGLSLShader::link` (llglslshader.cpp) の現状確認 (per-program hook 昇格箇所特定)
- [ ] AYAstorm 改変 11 file リスト確認 (sub-doc 06 §3.1 sub-step 6.5 untouched 維持対象):
  - picker (2): `class1/deferred/pickerV.glsl` + `class1/deferred/pickerF.glsl` (推定、要 grep 確認)
  - Cinematic (2): 要 grep 確認 (`grep -l "AYAstorm" indra/newview/app_settings/shaders/`)
  - visual realism (7): 同上

### 3.3 β-2 推奨 cadence (handoff-prep §5 + 本 handoff 反映)

1. β-2-trace (per-program hook 昇格箇所 + `LLGLSLShader::link` flow 全 trace、Agent 並列で 3-4 件)
2. β-2-prep (実装 plan 起草 = hook 昇格 algorithm + bundle 単位 patch 順序、AYA review)
3. β-2-hook (per-program hook 昇格 commit 単独)
4. β-2-bundle-A (195 file UBO 化 patch、Agent 並列、incremental measurement、commit)
5. β-2-bundle-B (228 file location qualifier、Agent 並列、incremental measurement、commit)
6. β-2-bundle-C (228 file entry point + extension、Agent 並列、measurement、commit)
7. β-2-verify (AYA launch verify、SPIR-V 生成成功率最終 measurement)
8. β-2-handoff (β-3 PSO 構築着手境界 handoff)

---

## 4. risks/caveats

- §4.1 **β-2 scope 巨大**: 228 file structural rewrite + per-program hook 昇格 = LL 上流改変規模、context 圧迫高、bundle 単位 fresh context 必須 (handoff-prep §6.7 反映)
- §4.2 **Agent 並列 patch semantic consistency**: bundle commit 単位 review で吸収、各 bundle commit で SPIR-V 生成成功率 measure (handoff-prep §6.2 反映)
- §4.3 **per-program hook 昇格箇所**: `LLGLSLShader::link` 内 stage concat 経路の特定は β-2-trace で実施、shader_code_text の per-stage 保持期間が link 時点まで延びる可能性あり (lifetime 注意)
- §4.4 **AYAstorm 改変 11 file untouched 維持**: 別 mechanism (後段 4.3-ε'/ζ') で対応、bundle-A/B/C patch から除外確認必須
- §4.5 **link fail (β-1 持越し)**: β-2-hook (per-program 昇格) 完遂時点で diffuseV/F.glsl の link fail が解消されることを measurement で確認、解消されない場合は別 root cause 調査 (e.g., extension declare 不足)
- §4.6 **binding 番号 全 base file 共通既定値**: §2.5 表 = sub-doc 07 §3.1 sub-step 7.2-7.4 確定値、各 bundle patch で逸脱禁止、逸脱発見時は本 handoff + sub-doc 07 同期改訂
- §4.7 **AYA 「OK」明示 review PASS 下 commit 厳守**: feedback_no_auto_commit、bundle ごとに AYA 明示指示要
- §4.8 **β-1 残存課題 (link fail) を β-2 で解消できない場合**: case ② case-validity 部分 reinstated を rollback → case ① (build-time pre-compile への退行) 検討、ただし charter §7.5 boundary refine 範囲外 (再 prep doc 必要)

---

## 5. 関連 doc / memory cross reference

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` §2 領域 6 + §3 #1/#4 + §7.5 boundary refine 履歴 (2026-06-01 spec-revision entry)
- `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` §1.2.2/§1.2.3/§1.2.4/§3.1 sub-step 6.3/§6.5
- `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` §3.1 sub-step 7.2-7.4/§6.5
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-prep.md` §1-§7 (β-2 cadence 引継 source)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-alpha-complete.md` (γ'-port-α 完遂 marker)
- memory `project_ayastorm_r41_vulkan_migration.md` (status active = β-1 完遂 / 次 = β-2)
- memory `MEMORY.md` index (r41 milestone 行 = β-1 完遂 / 次 = β-2)
- feedback rules: `feedback_doubt_self_first` (GL pollution crash 原因 = 自分の hook 配置疑い局所注入へ補正) / `feedback_falsification_as_progress` (link fail の architectural mismatch を honest に記録) / `feedback_no_scope_shrink` ((α) close は β-1 primary goal = parse pass の literal satisfy で AYA 承認下、scope 縮小ではない) / `feedback_proactive_handoff` (本 handoff 自体) / `feedback_self_verify_before_handoff` (verify metric 自己確認後 AYA 報告) / `feedback_one_step_at_a_time` (β-2 着手は AYA 明示指示要) / `feedback_use_agents_proactively` (β-2-trace + bundle patch で Agent 並列推奨) / `feedback_remove_verification_logs` (file_name parameter は structural log scope 外) / `feedback_no_claude_coauthor` / `feedback_no_auto_commit`

---

## 6. 次 session 着手 prompt (AYA → 次 Claude session 投入用)

> r41 sub-step 4.3-γ'-port-β-2 に着手してください。本 sub-step は **per-program hook 昇格 + base 228 file bundle-A/B/C structural rewrite** で、大規模 scope のため bundle 単位で fresh context を切る前提です。
>
> まず以下を順に読んで現状把握してください:
> 1. `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-1-complete.md` (本 handoff、§2 β-1 で確立した実装範式 + §3 β-2 scope + §4 risks + §6 prompt = 本文)
> 2. `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-prep.md` §1 case-validity falsify + §2 (α) 設計詳細 + §5 cadence
> 3. `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` §1.2.4 (α) macro 注入経路 + sample pattern 3 件 + binding 番号
> 4. `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` §3.1 sub-step 7.2-7.4 binding 番号確定値
> 5. memory `project_ayastorm_r41_vulkan_migration.md` status + sub-step 4.3-γ'-port-β-1 entry (commit `1bce995d87`)
> 6. β-1 commit `1bce995d87` の diff (4 file +63/-6) で実装範式確認
>
> 6 件読了後、β-2-trace (per-program hook 昇格箇所特定 + `LLGLSLShader::link` flow trace) から開始してください。Agent 並列で `LLGLSLShader::link` の現状実装 + per-stage shader_code_text の lifetime + AYAstorm 改変 11 file 確定 list の 3 件 trace を投入してください。
>
> trace 結果報告後、β-2-prep (実装 plan 起草) 着手是非を私 (AYA) に確認してから次に進んでください。`feedback_one_step_at_a_time` + `feedback_no_auto_commit` を厳守、bundle commit は AYA 明示指示下のみ。
