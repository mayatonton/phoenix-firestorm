# r41 sub-step 4.3-γ'-port-β-2-prep handoff (2026-06-01)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-1-complete.md` (sub-step 4.3-γ'-port-β-1 全完遂宣言 + §3 β-2 scope + §4 risks)
**本 handoff 位置付け**: sub-step 4.3-γ'-port-β-2 着手前 prep + 実装 plan 確定境界。β-1 (commit `1bce995d87`) で確立した範式 (LL_VULKAN_GLSL macro switch + bundle-A/B + 局所 `#define` 注入 + `file_name` parameter) を base に、(a) **per-program hook 昇格** (per-file → LLGLSLShader::createShader 経由 全 stage concat) + (b) **base 217 file (228 - AYAstorm 改変 11) bundle-A/B/C structural rewrite** の 2 axis 並走 plan を確定。β-2-trace 結果 3 件 (LLGLSLShader::link flow + shader_code_text lifetime + AYAstorm 改変 11 file 確定 list) AYA 報告済、本 prep doc で実装設計 + bundle cadence + 除外 file list を確定、AYA review PASS 後 commit + 別 session で β-2-hook 着手 (fresh context 推奨)。

---

## §1 起草目的 + β-2 scope 確認

### §1.1 起草目的

β-1 (commit `1bce995d87`、4 file +63/-6) は exemplar PoC として `class1/deferred/diffuse{V,F}.glsl` 2 file に LL_VULKAN_GLSL macro switch + bundle-A/B 注入を施し、**parse pass 0 fail** (case ② case-validity 部分 reinstated) を達成。残課題:

- **link fail 残存** (diffuseV.glsl 5 件 / diffuseF.glsl 1 件) = per-file SPIR-V 生成 vs multi-file shader linking の architectural mismatch (`mirrorClip()` / `encodeNormal()` / `passTextureIndex()` / `getObjectSkinnedTransform()` 等 forward decl が別 file から concat 前提)
- **228 file 中 226 件 untouched** = bundle-A/B 適用は exemplar 2 file 限定、残 file の parse pass 成立は未検証

本 prep doc で β-2 の 2 axis (per-program hook 昇格 + 217 file structural rewrite) 実装 plan を確定、AYA review PASS 後 commit + fresh context で β-2-hook 着手。

### §1.2 β-2 scope (2 axis 並走)

| axis | scope | β-1 状態 | β-2 完遂後 目標状態 |
|---|---|---|---|
| **(a) per-program hook 昇格** | `createSPIRVFromGLSL` 呼出を `loadShaderFile` 内 (per-file) → `LLGLSLShader::createShader` の loop 完了後 (per-program) へ移動、全 stage の shader source を concat して同一 `glslang::TProgram` に link、stage 別 SPIR-V 2 個生成 | per-file = forward decl 未解決で link 失敗 | per-program = forward decl 解決 + link 成立、cache layer hit/miss 経路成立 |
| **(b) 217 file bundle-A/B/C structural rewrite** | base 228 file - AYAstorm 改変 11 file = 217 file に対し、`#ifdef LL_VULKAN_GLSL` 分岐で bundle-A (UBO 化 + sampler binding) / bundle-B (location qualifier) / bundle-C (必要時 #version + extension) 適用 | exemplar 2 file 適用済 (残 215 file 未着手) | 217 file 全件適用、incremental SPIR-V 生成成功率 >>0% 目標 |

### §1.3 β-2-trace 結果反映 (本 session 既実施、AYA 報告済)

| trace 項目 | 結果 |
|---|---|
| LLGLSLShader::link flow | `llglslshader.cpp:1030-1047` link = LLShaderMgr::linkProgramObject wrapper、`createShader()` loop 完了後 (line 469 直後) が全 stage attach 後・link 直前の唯一の single entry point |
| shader_code_text lifetime | `loadShaderFile()` 内 line 724 で local stack 配列 alloc、line 1202-1205 で free。**link 時点では既に free 済み** = per-stage source 保持必要 |
| AYAstorm 改変 11 file 確定 list | Picker 2 + Cinematic BD 2 + Visual Realism 7 (§3.1 table 参照) |

---

## §2 axis (a) per-program hook 昇格 設計詳細

### §2.1 mStageSources field 追加 (`llglslshader.h`)

```cpp
class LLGLSLShader {
public:
    // ...
    struct StageSource {
        GLenum type;                          // GL_VERTEX_SHADER / GL_FRAGMENT_SHADER
        std::string file_name;                // open_file_name (e.g., "class1/deferred/diffuseV.glsl")
        std::vector<std::string> sources;     // loadShaderFile() 内 shader_code_text[] を std::string 化 copy
    };
    std::vector<StageSource> mStageSources;   // β-2 per-program hook 用、Vulkan path 限定で充填、GL path では untouched
    // ...
};
```

- **充填 timing**: `loadShaderFile()` 内 `gVK.isEnabled()` 条件下で shader_code_text[] copy → caller (`createShader()`) 経由で `mStageSources` push_back
- **memory 消費**: 全 shader program ~200-300 × 平均 ~20-100KB = total 4-30 MB (推定、§6.1)
- **GL path 影響**: 0 (`gVK.isEnabled() OFF` 時は mStageSources 不充填)

### §2.2 loadShaderFile() signature 拡張 (`llshadermgr.h/cpp`)

```cpp
// 旧 (β-1 state)
GLuint loadShaderFile(const std::string& filename,
                      S32& shader_level,
                      GLenum type,
                      std::unordered_map<std::string, std::string>* defines = nullptr,
                      S32 texture_index_channels = -1);

// 新 (β-2 拡張、後方互換 = out_sources default nullptr)
GLuint loadShaderFile(const std::string& filename,
                      S32& shader_level,
                      GLenum type,
                      std::unordered_map<std::string, std::string>* defines = nullptr,
                      S32 texture_index_channels = -1,
                      std::vector<std::string>* out_sources = nullptr);  // 新規 β-2
```

- `out_sources != nullptr` 時、`loadShaderFile()` 内 free 直前 (line 1202 直前) に shader_code_text[] を std::string copy → out_sources push_back
- 既存 caller (LLViewerShaderMgr / LLShaderMgr 派生 class) は out_sources default `nullptr` で影響 0

### §2.3 createShader() per-program hook 配置 (`llglslshader.cpp:407-555`)

```cpp
BOOL LLGLSLShader::createShader(std::vector<LLStaticHashedString>* attributes,
                                std::unordered_map<std::string, S32>* uniforms,
                                U32 varying_count, const char** varyings)
{
    // ... 既存 setup ...

    mShaderObjects.clear();
    mStageSources.clear();  // β-2 新規: per-program 開始時 clear

    for (auto& shader_file_pair : mShaderFiles)
    {
        // 既存: loadShaderFile() 呼出
        std::vector<std::string> stage_sources;  // β-2 新規
        GLuint shaderobj = LLShaderMgr::instance()->loadShaderFile(
            shader_file_pair.first, mShaderLevel, shader_file_pair.second,
            &mDefines, mFeatures.mIndexedTextureChannels,
            LLVKLoader::isVulkanInitialized() ? &stage_sources : nullptr);  // β-2 新規

        LL_DEBUGS("ShaderLoading") << "Shader file " << shader_file_pair.first << " ... " << LL_ENDL;
        if (shaderobj)
        {
            attachObject(shaderobj);
            mShaderObjects.push_back(shaderobj);
            // β-2 新規: Vulkan path 時のみ mStageSources に push
            if (LLVKLoader::isVulkanInitialized())
            {
                mStageSources.push_back({
                    shader_file_pair.second,
                    shader_file_pair.first,
                    std::move(stage_sources)
                });
            }
        }
        else
        {
            // 既存 failure path
            return FALSE;
        }
    }

    // β-2 新規: per-program SPIR-V 生成 hook (loop 完了後・mapAttributes() 呼出前)
    if (LLVKLoader::isVulkanInitialized() && !mStageSources.empty())
    {
        generatePerProgramSPIRV();  // 新規 helper (§2.4)
    }

    // ... 既存 mapAttributes() / mapUniforms() / link() ...
}
```

### §2.4 generatePerProgramSPIRV() helper 新規 (`llglslshader.cpp`)

```cpp
void LLGLSLShader::generatePerProgramSPIRV()
{
    // β-2 sub-doc 06 §1.2.4 (α) + handoff-substep-4-3-gamma-prime-port-beta-2-prep.md §2.4
    // mStageSources の各 stage を個別 glslang::TShader として parse、同一 TProgram に addShader → link
    // → stage 別 SPIR-V binary 2 個生成 → SPIR-V cache layer 案 C 格納 + vkCreateShaderModule

    glslang::TProgram program;
    std::vector<std::unique_ptr<glslang::TShader>> shaders;  // TProgram の lifetime 内維持

    for (auto& stage : mStageSources)
    {
        EShLanguage lang = (stage.type == GL_VERTEX_SHADER) ? EShLangVertex : EShLangFragment;
        auto shader = std::make_unique<glslang::TShader>(lang);

        // sources を concat (β-1 createSPIRVFromGLSL 範式 1:1 流用 = sources[0] = #version 直後 #define LL_VULKAN_GLSL 1 注入)
        std::string concatenated;
        if (!stage.sources.empty())
        {
            concatenated.append(stage.sources[0]);
            concatenated.append("#define LL_VULKAN_GLSL 1\n");
            for (size_t i = 1; i < stage.sources.size(); ++i)
            {
                concatenated.append(stage.sources[i]);
            }
        }

        const char* concat_cstr = concatenated.c_str();
        shader->setStrings(&concat_cstr, 1);
        shader->setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientVulkan, 100);
        shader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
        shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);

        EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
        if (!shader->parse(GetDefaultResources(), 450, false, messages))
        {
            LL_WARNS("Vulkan") << "generatePerProgramSPIRV: parse failed for "
                               << stage.file_name << " in program " << mName << "\n"
                               << shader->getInfoLog() << LL_ENDL;
            return;  // GL fallback
        }
        program.addShader(shader.get());
        shaders.push_back(std::move(shader));
    }

    if (!program.link(EShMsgDefault))
    {
        LL_WARNS("Vulkan") << "generatePerProgramSPIRV: link failed for program " << mName << "\n"
                           << program.getInfoLog() << LL_ENDL;
        return;  // GL fallback
    }

    // 各 stage の SPIR-V 生成 + cache 格納 + vkCreateShaderModule
    for (size_t i = 0; i < shaders.size(); ++i)
    {
        std::vector<unsigned int> spirv;
        glslang::SpvOptions options;
        options.generateDebugInfo = false;
        options.stripDebugInfo = true;
        options.disableOptimizer = true;
        options.validate = false;
        glslang::GlslangToSpv(*program.getIntermediate(shaders[i]->getStage()), spirv, &options);

        // β-1/γ'-port-α 既存 mechanism 流用 (cache file 書込 + VkShaderModule load + map 格納)
        auto& stage = mStageSources[i];
        // ... (HBXXH128 hash 計算 + ~/.ayastorm_x64/cache/shader_cache/<hash>_{v,f}.spv fwrite + loadSpirvShaderModuleFromMemory)
        // 詳細は β-2-hook 実装時に γ'-port-α `ff48b21d88` の `llshadermgr.cpp:908` 付近 hook block 構造を 1:1 流用
    }
}
```

### §2.5 既存 per-file hook の deprecate (`llshadermgr.cpp:1095-1110`)

β-2-hook 完遂時に `loadShaderFile()` 内の `createSPIRVFromGLSL` 呼出 + SPIR-V cache hook block (line 908-1110) を **削除**:

- 理由: per-program 昇格で重複動作 + per-file は forward decl 未解決で 0% link 成立 (β-1 metric)
- `createSPIRVFromGLSL` helper 本体 (line 507-606) は generatePerProgramSPIRV() 内 stage 別呼出で **再利用**、refactor のみ (signature 維持、内部 logic 1:1 流用)
- cache layer (HBXXH128 hash + fwrite/fread + loadSpirvShaderModuleFromMemory) は per-program hook 内に移植、`mShaderHash` は program-level (LLGLSLShader::mShaderHash llglslshader.cpp:2055-2083 既存) を使用可能性検討 (β-2-hook 実装時確定)

---

## §3 axis (b) 217 file bundle-A/B/C structural rewrite plan

### §3.1 patch 除外 11 file 確定 list (β-2-trace 結果)

| カテゴリ | file path (`indra/newview/app_settings/shaders/` 相対) | 関連 r | 主要 commit |
|---|---|---|---|
| Picker (2) | `class1/deferred/fsObjectIDF.glsl` | r21.1 | `f3c0829ea8` |
| Picker (2) | `class1/deferred/fsObjectIDV.glsl` | r21.1 | `d4fa807f00` |
| Cinematic BD (2) | `cinematic_bd/class1/deferred/shadowUtil.glsl` | r30 | `4769ac08ce` |
| Cinematic BD (2) | `cinematic_bd/class3/deferred/screenSpaceReflUtil.glsl` | r30 | `c3dc5559dc` |
| Visual Realism (7) | `class1/deferred/godraysF.glsl` | r15+ | `9a703a2480` |
| Visual Realism (7) | `class1/deferred/godraysV.glsl` | r15+ | `9a703a2480` |
| Visual Realism (7) | `class1/deferred/volumetricLightF.glsl` | r30 | `bf269b8671` |
| Visual Realism (7) | `class3/deferred/volumetricLightF.glsl` | r30 | `bf269b8671` |
| Visual Realism (7) | `class1/deferred/blurLightF.glsl` | r30 | `d9112b2cbf` |
| Visual Realism (7) | `class1/deferred/blurLightV.glsl` | r30 | `d9112b2cbf` |
| Visual Realism (7) | `class1/windlight/atmosphericsFuncs.glsl` | r30 | `ed429b5632` |

→ **patch 対象 = 228 - 11 = 217 file**

### §3.2 bundle 構成 (sub-doc 06 §1.2.4 1:1 流用、β-1 範式継承)

| bundle | 内容 | 対象 file 数 | β-1 範式 (diffuseV/F) |
|---|---|---|---|
| **bundle-A** | uniform → UBO 化 + sampler binding (set=0 PerFrame / set=1 Per-Material / set=2 Per-Draw)、binding 番号は sub-doc 07 §3.1 sub-step 7.2-7.4 全 base file 共通既定値 | 217 file (uniform 持ち全件) | diffuseV.glsl: `PerFrame` UBO (set=0/binding=0) + `MaterialUBO` (set=1/binding=0); diffuseF.glsl: `layout(set=1, binding=1) uniform sampler2D diffuseMap` |
| **bundle-B** | varying → `layout(location=N)` in/out (vertex out ↔ fragment in pair 番号一致) | 217 file (varying/attribute 持ち全件) | diffuseV.glsl: `layout(location=0..3) in/out`; diffuseF.glsl: `layout(location=0) out frag_data[4]` + `layout(location=0..3) in vary_*` |
| **bundle-C** | 必要時 `#version 460` + extension declare (β-1 diffuseV/F は **不要**確認、LLShaderMgr が既存 `#version 420` prepend + `void main()` GLSL 既定 entry point で satisfy) | 必要 file のみ (bundle-A+B 完遂後 parse fail 残存 file で確定) | 不要 (`#version 420` で sufficient) |

### §3.3 bundle commit 順序 + incremental measurement

| bundle | commit cadence | incremental measurement target | 完遂 marker |
|---|---|---|---|
| β-2-bundle-A | 217 file UBO 化、Agent 並列、bundle 内 partial commit 複数可 (例: 50 file × 4 + 17 file × 1)、最終 bundle-A 完遂 commit 1 件 | bundle-A 単独完遂時 (bundle-B 未着手): glslang parse fail = location 未指定残存 (期待値)、link は per-program hook で forward decl 解決 ✓ measurement | UBO 化 217 file 全件適用 + AYAstorm 改変 11 file untouched verify + AYA 「OK」承認 |
| β-2-bundle-B | 同上 | bundle-A+B 完遂時: parse pass 率 measurement (bundle-C 不要 file は SPIR-V 生成成立) | location 217 file 全件適用 + 11 file untouched verify + AYA 「OK」承認 |
| β-2-bundle-C | 必要 file のみ (推定 ~10-30 file)、Agent 並列 | bundle-A+B+C 完遂時: SPIR-V 生成成功率 >>0% 目標、cache file 数で間接 measurement | 必要 file 全件適用 + AYA 「OK」承認 |

---

## §4 AYA 承認境界

| 境界 | β-1 境界 | β-2 における変化 |
|---|---|---|
| AYAstorm 改変 11 file untouched | charter §3 #1 + sub-doc 06 §3.1 sub-step 6.5 | (unchanged、§3.1 確定 list で bundle patch 除外、bundle commit ごとに `git diff -- <11 file>` verify) |
| GL path 完全並走 | charter §3 #1 | (unchanged、`#ifdef LL_VULKAN_GLSL` + `#else` で GL path 維持、`gVK.isEnabled() OFF` 時 GL path 完全動作) |
| 段階 1-4.3-γ'-port-β-1 動作維持 | unchanged | (per-program hook 昇格は Vulkan path 限定、mStageSources は `gVK.isEnabled() OFF` 時 不充填 + generatePerProgramSPIRV 不呼出 = GL path 影響 0) |
| sub-doc 06 §3.1 sub-step 6.3 scope | (β-prep-spec-revision で「base 228 file 全件 LL_VULKAN_GLSL macro switch」確定) | (unchanged、β-2 で literal satisfy のみ、scope 拡張なし = spec-revision 起草不要) |
| bundle commit ごとの AYA 「OK」明示指示 | (feedback_no_auto_commit) | (bundle 単位 commit ごとに AYA 「OK」承認下、β-2-hook + bundle-A + bundle-B + bundle-C + handoff = 計 ~5-10 commit 想定) |
| sub-doc 07 §3.1 sub-step 7.2-7.4 binding 番号 | (β-prep-spec-revision で全 base file 共通既定値確定) | (unchanged、bundle-A patch で 217 file 全件 binding 番号遵守、逸脱 0 件 verify) |

---

## §5 新 cadence (β-2 内 task 分割)

### §5.1 task list

| task | scope | 着手 timing | 想定 commit | context |
|---|---|---|---|---|
| **β-2-prep (本 doc)** | per-program hook 昇格 plan + bundle 順序 + 11 file 除外 list 確定 | 今 session | doc-only commit 1 件 (本 prep doc 単独) | 今 session 内 |
| **β-2-hook** | `LLGLSLShader::mStageSources` field + `loadShaderFile()` `out_sources` parameter + `createShader()` per-program hook + `generatePerProgramSPIRV()` helper + 既存 per-file hook 削除 + cache layer 移植 | 本 prep doc commit + AYA 「OK」承認下 | code commit 1 件 | **fresh context 推奨** (中規模 trace + 実装、γ'-port-α `ff48b21d88` 範式 1:1 流用) |
| **β-2-bundle-A** | 217 file UBO 化 + sampler binding (Agent 並列、bundle 内 partial commit 複数可) | β-2-hook 完遂後 | bundle-A 完遂 commit 1 件 (+ partial commit 複数可) | **fresh context 推奨** (大規模 patch、Agent 並列で 50 file × 4 partial 等) |
| **β-2-bundle-B** | 217 file location qualifier (Agent 並列) | β-2-bundle-A 完遂後 | 同上 | 同上 |
| **β-2-bundle-C** | 必要 file (推定 ~10-30 file) `#version 460` + extension declare | β-2-bundle-B 完遂後 | bundle-C 完遂 commit 1 件 | 中規模、Agent 並列 |
| **β-2-verify** | AYA launch verify + SPIR-V 生成成功率最終 measurement (cache file 数 + parse/link fail WARN 集計) | β-2-bundle-C 完遂後 | (AYA review、commit なし) | |
| **β-2-handoff** | β-3 (PSO 構築) 着手境界 handoff doc 起草 + commit | β-2-verify PASS 後 | doc-only commit 1 件 | |

### §5.2 estimated effort

| task | session 数 | 備考 |
|---|---|---|
| β-2-prep | 1 (今 session) | 本 doc |
| β-2-hook | 1 (fresh context) | mStageSources field 配線 + per-program hook 配置 + per-file hook 削除 + cache 移植、中規模 |
| β-2-bundle-A | 数 session (Agent 並列で短縮可) | 217 file UBO 化、最大規模 |
| β-2-bundle-B | 2-3 session (Agent 並列) | 217 file location qualifier、中規模 |
| β-2-bundle-C | 1 session (Agent 並列) | 必要 file のみ、最小規模 |
| β-2-verify + handoff | 1 session | |
| **計** | **~1-2 week** | Agent 並列 + bundle 単位 fresh context で context budget 担保 |

### §5.3 cadence 採用根拠

- **β-1 範式継承**: β-1 (commit `1bce995d87` = LL_VULKAN_GLSL macro switch + bundle-A/B 注入) で確立した範式を bundle 単位で 217 file に横展開
- **per-program hook 昇格を bundle patch 前 に独立 commit**: hook 昇格単独で β-1 exemplar (diffuseV/F.glsl) の link 成立確認 = bundle patch の前提担保 (link 成立 verify は hook 昇格 commit 完遂 marker)
- **bundle 単位 fresh context**: 217 file 大規模 scope = bundle 単位で session 分割、各 bundle 完遂時に AYA 「OK」承認 + commit + handoff doc proactive 起草
- **AYA review 介在の安全性**: bundle 単位 commit で逆風検出時の rollback 範囲限定

---

## §6 risks/caveats

### §6.1 mStageSources memory 消費

- 全 shader program ~200-300 × 平均 ~20-100KB per program = total 4-30 MB (推定)
- 許容範囲、Vulkan path 限定で `gVK.isEnabled() OFF` 時は不充填 = GL path 影響 0
- worst case (高度な variant 爆発) でも 50 MB 以内想定

### §6.2 loadShaderFile() signature 拡張の後方互換

- `out_sources` parameter は default `nullptr` で既存 caller (LLViewerShaderMgr / 派生 class) 影響 0
- 内部 strdup → std::string copy のコスト ≤ 1ms/shader、起動時のみ fire (steady state 影響 0)

### §6.3 既存 per-file hook 削除と β-1 verify metric の整合

- β-1 で per-file hook 経由 parse pass 0 fail 確認は exemplar 2 file 限定 marker (link は失敗)
- β-2-hook 完遂時の verify marker = per-program hook 経由 exemplar 2 file の **link 成立** + cache file 生成確認 (per-file hook の parse pass marker は β-1 履歴で歴史保存、削除しない)
- 既存 GL `.shaderbin` cache は SPIR-V `.spv` cache と独立で並存 (γ'-port-α `ff48b21d88` 配置済 mechanism 流用)

### §6.4 11 file 除外の git diff verify

- bundle commit 完遂時に以下 11 path で `git diff feature/ayastorm-r41-gl-removal..HEAD -- <path>` が **0 件 hit** を verify
- 担保: charter §3 #1 + sub-doc 06 §3.1 sub-step 6.5 + 本 prep doc §3.1
- bundle 内 partial commit でも 11 file 不混入を partial commit 単位で verify

### §6.5 SPIR-V 生成成功率 incremental measurement の解釈

- bundle-A 単独完遂時: parse fail = location 未指定残存 (期待値、bundle-B 着手前)、link は per-program hook で forward decl 解決確認可
- bundle-A+B 完遂時: parse pass 率 measurement (bundle-C 不要 file は SPIR-V 生成成立)
- bundle-A+B+C 完遂時: SPIR-V 生成成功率 >>0% 目標、`~/.ayastorm_x64/cache/shader_cache/*.spv` file 数で間接 measurement
- 逆風検出時 (例: bundle-A 完遂時 GL path regression 発生) は revert 検討 (bundle 単位 fresh commit で rollback 範囲限定)

### §6.6 Agent 並列 patch の semantic consistency

- bundle commit 単位 review で吸収 (例: bundle-A の partial commit 4 件をまとめて AYA review)
- Agent 並列時の patch pattern 統一 = β-1 範式 (diffuseV/F.glsl の `#ifdef LL_VULKAN_GLSL` block 構造) を Agent prompt に明示

### §6.7 context budget proactive 監視

- β-2-hook + bundle 3 件 + verify/handoff = 計 ~10 session 想定
- 各境界で proactive handoff doc 起草 (`feedback_proactive_handoff`)、bundle 単位 fresh context 推奨

### §6.8 link 失敗が per-program hook 昇格でも残存する場合

- multi-file forward decl 以外の architectural mismatch (例: TProgram 内 entry point 衝突 / `gl_in[]` interface block 等) 残存時は β-2-hook 内で個別調査
- fallback として GL path 維持 (charter §3 #1) で動作担保 = β-2-hook 完遂後も `gVK.isEnabled() OFF` 時の GL path 完全動作維持
- 解消不能時は handoff doc に明記 + β-3 (PSO 構築) 着手判断 で AYA 相談

### §6.9 generatePerProgramSPIRV() の glslang InitializeProcess

- `glslang::InitializeProcess()` は createSPIRVFromGLSL の static bool guard で process-global 単発 init (γ'-port-α `ff48b21d88` §9.1 設計反映)
- generatePerProgramSPIRV() は createSPIRVFromGLSL helper 経由ではなく直接 glslang::TShader/TProgram を使うため、InitializeProcess の static guard を generatePerProgramSPIRV 側にも複製 or createSPIRVFromGLSL refactor で共通化 (β-2-hook 実装時確定)

### §6.10 mShaderHash の per-program 化整合

- 既存 cache key `mShaderHash` = HBXXH128(open_file_name + concat sources) は file-level (γ'-port-α `ff48b21d88` §9.4)
- β-2-hook で per-program 移行時、cache key を program-level (program name + all stage sources concat の HBXXH128) へ refine 検討
- 既存 cache file (file-level) は β-2-hook commit で stale 化、再生成必要 (cache file は使い捨て、disk usage 増加なし)

---

## §7 next action

### §7.1 着手前 cadence

1. AYA さん review 本 prep doc (§1 起草目的 + β-2 scope / §2 axis (a) per-program hook 設計 / §3 axis (b) 217 file bundle 構成 / §4 AYA 承認境界 / §5 新 cadence / §6 risks/caveats)
2. AYA さん「OK」承認下で commit (本 prep doc 1 件 doc-only commit)
3. **fresh context 推奨** で β-2-hook 着手 (`LLGLSLShader::mStageSources` field 追加 + `loadShaderFile()` `out_sources` parameter + `createShader()` per-program hook 配置 + `generatePerProgramSPIRV()` helper + per-file hook 削除 + cache 移植、AYA 明示指示要)

### §7.2 β-2-hook 着手 task 候補 (次 session、参考)

| task | 詳細 |
|---|---|
| β-2-hook-1 | `llglslshader.h` に `struct StageSource` + `std::vector<StageSource> mStageSources` field 追加 + `generatePerProgramSPIRV()` private 宣言 |
| β-2-hook-2 | `llshadermgr.h/cpp` `loadShaderFile()` signature に `std::vector<std::string>* out_sources = nullptr` parameter 追加 + 内部 shader_code_text[] copy 経路配線 (line 1202 free 直前) |
| β-2-hook-3 | `llglslshader.cpp:407 createShader()` loop 内 `out_sources` 渡し + push_back to `mStageSources` (Vulkan path 限定) |
| β-2-hook-4 | `llglslshader.cpp` `generatePerProgramSPIRV()` impl 配置 (loop 完了後・mapAttributes 呼出前 hook、glslang::TShader 各 stage parse + TProgram link + GlslangToSpv stage 別) |
| β-2-hook-5 | `llshadermgr.cpp:1095-1110` 既存 per-file hook 削除 (createSPIRVFromGLSL helper 本体は generatePerProgramSPIRV 内 stage 別呼出で再利用、削除対象は loadShaderFile 内呼出 + cache hook block) |
| β-2-hook-6 | cache layer (HBXXH128 + fwrite/fread + loadSpirvShaderModuleFromMemory + mVk{Vertex,Fragment}ShaderModules) を generatePerProgramSPIRV 内に移植、cache key は program-level (β-2-hook 実装時確定) |
| β-2-hook-7 | incremental autobuild (configure + build) |
| β-2-hook-8 | AYA launch verify + exemplar diffuseV/F.glsl の **link 成立** + cache file 2 件 (`.../diffuseV_v.spv` + `.../diffuseF_f.spv`) 生成 + parse/link WARN 0 件確認 |
| β-2-hook-9 | commit (sub-step 4.3-γ'-port-β-2-hook 単独 commit、AYA 「OK」明示指示下) |

### §7.3 critical reminders

| reminder | 詳細 |
|---|---|
| **AYAstorm 改変 11 file shader 改変禁止** | §3.1 確定 list、bundle commit ごとに `git diff -- <11 path>` 0 件 hit verify |
| **段階 1-4.3-γ'-port-β-1 動作維持** | GL path 完全並走 (`gVK.isEnabled() OFF` 時)、per-program hook 昇格は Vulkan path 限定 |
| **case ② path + GL path 完全並走** | charter §3 #1 acceptance |
| **β-1 範式継承** | `#ifdef LL_VULKAN_GLSL ... #else ... #endif` 構造 + sub-doc 07 §3.1 binding 番号確定値遵守 |
| **bundle 単位 fresh context** | 217 file 大規模 scope、bundle 完遂時 proactive handoff doc 起草 |
| **defer / disable 提案 ban** | `feedback_self_bug_no_defer_option` 遵守、(α) 採用後の patch 失敗 file は GL fallback で動作維持 (構造的並走)、defer/disable 選択肢として並べない |
| **AYA 承認境界遵守** | bundle commit ごとに AYA 「OK」明示指示、commit message に明示 |
| **proactive handoff 範式** | β-2-hook 完遂時 + bundle-A/B/C 各完遂時の handoff 起草 (`feedback_proactive_handoff`) |

### §7.4 commit 戦略

- 本 prep doc 単独 commit = 1 件 (doc-only)、AYA 「OK」承認下実施
- β-2-hook 実装 commit = 1 件 (code commit、γ'-port-α 範式継承)
- β-2-bundle-A 実装 commit = 1 件 (+ partial commit 複数可)
- β-2-bundle-B 実装 commit = 1 件 (+ partial commit 複数可)
- β-2-bundle-C 実装 commit = 1 件
- β-2-handoff doc commit = 1 件
- 計 ~5-10 commit 想定 (partial commit 含む)

---

## §8 関連 doc / memory cross reference

### §8.1 関連 doc

| doc | 役割 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/00-charter.md` | charter §2 領域 6 + §3 #1/#4 + §7.5 boundary refine 履歴 (2026-06-01 spec-revision entry) |
| `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` | §1.2.2/§1.2.3/§1.2.4/§3.1 sub-step 6.3/§6.5 (β-2 で sub-step 6.3 literal satisfy、本 prep doc §3 patch 構成は §1.2.4 1:1 流用) |
| `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` | §3.1 sub-step 7.2-7.4 binding 番号確定値 (set=0 PerFrame / set=1 Material / set=2 Per-Draw)、bundle-A patch で 217 file 全件 遵守 |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-prep.md` | §1 case-validity falsify + §2 (α) 設計詳細 + §5 cadence (β-2 着手時 task 列挙の base) |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-1-complete.md` | β-1 全完遂宣言 + §2 β-1 で確立した実装範式 + §3 β-2 scope + §4 risks (本 prep doc の起点) |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-alpha-complete.md` | γ'-port-α 完遂 marker (cache layer 案 C + loadSpirvShaderModuleFromMemory production sink、β-2-hook で 1:1 流用) |

### §8.2 関連 memory

| memory | 役割 |
|---|---|
| `project_ayastorm_r41_vulkan_migration.md` | active milestone tracking、β-2-prep 完遂後 update plan = sub-step 4.3-γ'-port-β-2-prep 完遂 → fresh context で β-2-hook 着手 |
| `project_build_procedure.md` | autobuild fullflow + .venv activate |
| `feedback_proactive_handoff.md` | context 圧迫時の proactive handoff 範式 (本 prep doc 起草 = 範式遵守) |
| `feedback_self_verify_before_handoff.md` | handoff 起草前の self-trace 義務 (本 prep doc = β-2-trace 3 件 self-trace 完遂後の起草) |
| `feedback_no_claude_coauthor.md` | commit message Co-Authored-By: Claude 禁止 |
| `feedback_no_auto_commit.md` | コミットは明示指示後 (本 prep doc commit は AYA 「OK」承認下) |
| `feedback_no_scope_shrink.md` | (α) 採用 = 217 file 全件 patch、scope shrink 禁止 (例: 100 file のみ patch + 残 GL fallback、は禁止) |
| `feedback_self_bug_no_defer_option.md` | (α) 採用後の patch 失敗 file への defer/disable 提案禁止 = GL fallback (構造的並走) で動作維持 |
| `feedback_falsification_as_progress.md` | β-1 link fail = β-2 per-program hook 昇格 必要性の falsification 積上げ |
| `feedback_doubt_self_first.md` | β-2-hook 着手時の hook 配置誤り疑い (β-1 では shader_code_text 共有注入で GL pollution crash 検出 → 局所注入へ補正の範式継承) |
| `feedback_one_step_at_a_time.md` | 1 メッセージ 1 アクション cadence、bundle commit は AYA 明示指示下 |
| `feedback_use_agents_proactively.md` | β-2-bundle-A/B/C 217 file patch で Agent 並列活用 |
| `feedback_remove_verification_logs.md` | structural log (cache hit/miss + filename + parse/link fail WARN) は scope 外 = 除去対象なし |

---

**本 prep doc 起草日**: 2026-06-01
**起草根拠**: handoff-substep-4-3-gamma-prime-port-beta-1-complete.md §3 β-2 scope + §6 prompt (本 session 着手) → β-2-trace 3 件 (LLGLSLShader::link flow + shader_code_text lifetime + AYAstorm 改変 11 file 確定 list) AYA 報告済 + AYA 「OK」β-2-prep 着手承認 2026-06-01 → β-prep / γ'-port-α-prep / γ'-port-β-prep 範式継承で本 prep doc 起草
**次 session 着手**: sub-step 4.3-γ'-port-β-2-hook (per-program hook 昇格 = mStageSources field + loadShaderFile out_sources + createShader hook + generatePerProgramSPIRV + 既存 per-file hook 削除 + cache 移植) (fresh context 推奨、AYA 明示指示要)
