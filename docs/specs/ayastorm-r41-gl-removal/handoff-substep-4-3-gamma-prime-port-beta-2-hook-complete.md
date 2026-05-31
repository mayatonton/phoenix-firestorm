# r41 sub-step 4.3-γ'-port-β-2-hook 完遂 → 4.3-γ'-port-β-2-bundle-A 着手境界 handoff (2026-06-01)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-prep.md` (commit `b7e2dbc72e`、β-2 scope 2 軸 = (a) per-program hook 昇格 + (b) 217 file (base 228 − AYAstorm 改変 11) bundle-A/B/C structural rewrite、7-task cadence [β-2-prep ✓ → β-2-hook → β-2-bundle-A → β-2-bundle-B → β-2-bundle-C → β-2-verify → β-2-handoff]、10 risks/caveats)

**本 handoff 位置付け**: sub-step 4.3-γ'-port-β-2-hook (per-program SPIRV 生成 hook 昇格 + β-1 per-file 廃止、4 file +394/-100) **全完遂宣言** + sub-step 4.3-γ'-port-β-2-bundle-A (217 file uniform → UBO 化 + sampler binding layout(set=N, binding=M) qualifier 注入) 着手前 fresh context 引継境界。AYA build verify PASS + launch verify PASS + 「OK commit して」明示指示下で commit `fcf2b6c508` 経由 章 close。

---

## 1. sub-step 4.3-γ'-port-β-2-hook 全完遂 status (2026-06-01)

### 1.1 完遂 marker (prep doc §2 axis (a) 5 scope 全充足)

| acceptance | 達成 status |
|---|---|
| β-2-hook-1 `LLGLSLShader::StageSource` nested struct + `mStageSources` public field 追加 + `generatePerProgramSPIRV` private method 宣言 (`llglslshader.h` +26) | ✓ |
| β-2-hook-2 `LLShaderMgr::loadShaderFile` signature 拡張 (`std::vector<std::string>* out_sources = nullptr` parameter 追加、`llshadermgr.h` L441 + `llshadermgr.cpp` L595 同期、backward compat 維持、recursive retry にも propagate) | ✓ |
| β-2-hook-3 `LLGLSLShader::createShader()` hook 配置 (`mShaderFiles` loop 完遂後 `mapAttributes()` 直前、`LLVKLoader::isVulkanInitialized()` guard 下で `mStageSources` 収集 → `generatePerProgramSPIRV(mStageSources)` 呼出、`llglslshader.cpp` L456-501) | ✓ |
| β-2-hook-4 `generatePerProgramSPIRV` impl (HBXXH128 hash → cache hit/miss 経路 + per stage_type `glslang::TShader` parse + **全 stage 単一 `glslang::TProgram` link** + per stage `GlslangToSpv` + cache write + VkShaderModule 生成、`llglslshader.cpp` L634-895、260 line) | ✓ |
| β-2-hook-5 旧 per-file hook 削除 (`llshadermgr.cpp` 旧 L1025-1130 一括削除 = β-1 per-file SPIR-V cache + `createSPIRVFromGLSL` call + VkShaderModule storage 廃止、置換 = `out_sources` copy block 27 line) | ✓ |
| β-2-hook-6 incremental autobuild (configure + build) | ✓ exit 0 / 4 file +394/-100 / warning 0 件 / 34 target 全 Built / packaging 完遂 (Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261511632.tar.xz) |
| β-2-hook-7 AYA launch verify | ✓ AYA 「起動して終了しました」報告 / crash 0 / clean shutdown / regression 0 |
| β-2-hook-8 hook fire 確認 (per-program hook 全 program 到達) | ✓ `~/.ayastorm_x64/logs/AYAstorm.log` `generatePerProgramSPIRV` log 226 件 fire |
| β-2-hook-9 SPIR-V 生成成功率 incremental measurement | ✓ `*_program.spv` 0 件 (bundle 未着手で parse 失敗継続、handoff prep §6.9 incremental measurement 設計通り) |
| β-2-hook-10 commit (AYA 明示指示下) | ✓ AYA 「OK commit して」明示指示 / commit `fcf2b6c508` |
| install + cache clear 完遂 | ✓ `~/ayastorm/` + `~/.ayastorm_x64/cache/` |

### 1.2 commit hash

| commit | scope |
|---|---|
| `fcf2b6c508` | sub-step 4.3-γ'-port-β-2-hook 全完遂 (4 files changed, +394/-100) |

### 1.3 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-4-3-gamma-prime-port-beta-1-complete.md` | 役割完了 (β-1 per-file model 廃止前夜 marker、β-2-hook で per-program model 昇格完遂、β-2-bundle-A 以降は本 handoff が引継 source) |
| `handoff-substep-4-3-gamma-prime-port-beta-2-prep.md` (commit `b7e2dbc72e`) | active 継続 (bundle-A 着手時 §3 axis (b) 217 file bundle 構成 + §5 cadence + §6 risks 参照源) |
| sub-doc `06-shader-spirv.md` | active 継続 (bundle-A 着手時 §1.2.4 (α) macro 注入経路 / sample pattern 3 件 / binding 番号 確定値遵守) |
| sub-doc `07-descriptor-renderpass.md` | active 継続 (bundle-A 着手時 §3.1 sub-step 7.2 set=0 binding 0=ViewProj/1=Lights/2=Atmosphere + sub-step 7.3 set=1 binding 0=MaterialUBO/1=DiffuseTex/2=NormalTex/3=SpecTex 確定値遵守、bundle 単位 patch shader 側と同期) |
| sub-doc `08-llvkrenderer-skeleton.md` | active 継続 (sub-step 4.4 part B で参照) |
| `handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md` (本 handoff) | 新規作成 (4.3-γ'-port-β-2-hook 全完遂 → 4.3-γ'-port-β-2-bundle-A 着手境界) |

### 1.4 AYA build verify + launch verify PASS metric (2026-06-01、γ'-port-β-2-hook build)

| metric | 値 |
|---|---|
| autobuild configure | exit 0 (AYA_GLSLANG_VALIDATOR `/usr/bin/glslangValidator` 検出済 = 3.3-B exemplar dead code 除外維持、AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK 非定義) |
| autobuild build | exit 0 / 34 target 全 Built |
| `llglslshader.cpp.o` 再構築 | clean (warning 0 件、6/1 01:32) |
| `llshadermgr.cpp.o` 再構築 | clean (warning 0 件、6/1 01:32) |
| `llvkloader.cpp.o` 再構築 | 5/31 23:15 (signature 変更なしの transitive include 影響なし) |
| packaging | `Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261511632.tar.xz` (205 MB) |
| install + cache clear | 完遂 (`~/ayastorm/` 配置 + `~/.local/share/applications/ayastorm-viewer.desktop` 再配置 + `~/.ayastorm_x64/cache/` clear) |
| crash | **0 件** ✓ |
| shutdown | **clean** ✓ (AYA 「起動して終了しました」報告) |
| GL path regression | **0 件** ✓ (`.shaderbin` (GL binary cache) 正常生成継続 = AYAstorm 改変 11 file untouched + 段階 1-4.3-β' 動作維持) |
| `generatePerProgramSPIRV` hook fire | **226 件** ✓ (全 program 到達確認、`grep -cE "generatePerProgramSPIRV"` 226) |
| SPIR-V 生成成功率 (`*_program.spv` 数) | **0 件** (bundle-A/B/C 未着手で parse 失敗継続、handoff prep §6.9 incremental measurement 設計通り = β-2-hook 単独で SPIR-V 生成成功率向上は期待しない、bundle 完遂時 >>0% 目標) |
| parse 失敗パターン | **全件 stage type `0x8b30` (`GL_FRAGMENT_SHADER`) で `glslang parse failed`** (uniform/location/entry point の 3 issue type、bundle-A/B/C scope) |
| AYA 明示指示 | 「起動して終了しました」(launch verify PASS marker) + 「OK commit して」(commit 承認) |

### 1.5 close 採用根拠 (β-1→β-2 architectural progress)

β-2-hook 段階の **primary goal = per-file architectural mismatch 解消** (β-1 で発覚した `mirrorClip` / `encodeNormal` / `passTextureIndex` / `getObjectSkinnedTransform` 4 symbol forward declaration link failure を per-program model 昇格で構造的に解消):

| 段階 | 失敗段階 | 失敗 root cause | 解消 path |
|---|---|---|---|
| β-1 (per-file model) | **link** 段階失敗 | per-file `glslang::TProgram` isolation で multi-file forward decl 未解決 | per-program model 昇格 (β-2-hook) で全 stage 単一 `TProgram` link → forward decl 自動解決 |
| β-2-hook (per-program model) | **parse** 段階失敗 (link 段階に到達せず) | bundle-A/B/C scope の素朴な GLSL 仕様準拠問題 (uniform/location/entry point の 3 issue type) | bundle-A/B/C structural rewrite (β-2-bundle-A/B/C) で全 217 file UBO 化 + location qualifier + entry point + extension declare |

→ β-1 architectural mismatch (per-file isolation) は **消滅**、残るは bundle-A/B/C scope の構造的書換問題で、handoff prep §6.9 設計通りに parse 失敗段階に進化。bundle 完遂時 parse pass → 単一 TProgram link で forward decl 自動解決 → SPIR-V 生成 >>0% 目標経路成立。

case ② case-validity の **architectural reinstated** (β-1 で部分 reinstated だった parse pass marker を β-2-hook で全 program 到達 + per-file mismatch 消滅へ拡張) を marker として β-2-hook close、SPIR-V 生成成功率 measurement は β-2-bundle-A 完遂時に 1 段階目を実施。

---

## 2. β-2-hook で確定した実装範式 (β-2-bundle-A/B/C 流用 base)

### 2.1 StageSource nested struct (`llglslshader.h` L172-178)

```cpp
struct StageSource
{
    GLenum type;                      // GL_VERTEX_SHADER / GL_FRAGMENT_SHADER
    std::string file_name;            // open_file_name (gpu_class 解決後)
    std::vector<std::string> sources; // loadShaderFile() preprocessing 後 shader_code_text[] copy
};
std::vector<StageSource> mStageSources;
```

- **type** = `GL_VERTEX_SHADER` / `GL_FRAGMENT_SHADER` (`stages_by_type` で per-type grouping に使用)
- **file_name** = `open_file_name` (gpu_class 解決後の絶対 path 風 string、HBXXH128 hash 入力 + VkShaderModule storage key)
- **sources** = `loadShaderFile()` preprocessing 後の `shader_code_text[]` 全件 copy (stack array lifetime 制約解消)

### 2.2 `loadShaderFile` signature 拡張 (`llshadermgr.h` L441 + `llshadermgr.cpp` L595)

```cpp
GLuint loadShaderFile(const std::string& filename,
                      S32 & shader_level,
                      GLenum type,
                      std::map<std::string, std::string>* defines = NULL,
                      S32 texture_index_channels = -1,
                      std::vector<std::string>* out_sources = nullptr);
```

- `out_sources` default `nullptr` で **GL path 既存呼出箇所影響 0** (backward compat 維持)
- `shader_code_text[]` (stack array) は alloc → `glShaderSource` → free 内で source 消滅、`out_sources` copy で caller 側 lifetime 延長
- **recursive retry** (`shader_level` decrement) にも `out_sources` propagate (L1145、初回 propagate 漏れを自己修正)

### 2.3 `out_sources` copy block (`llshadermgr.cpp` L1025-1051、旧 per-file hook 置換)

```cpp
if (out_sources && LLVKLoader::isVulkanInitialized())
{
    out_sources->clear();
    out_sources->reserve(shader_code_count);
    for (GLuint i = 0; i < shader_code_count; ++i)
    {
        if (shader_code_text[i])
            out_sources->emplace_back(shader_code_text[i]);
        else
            out_sources->emplace_back();
    }
}
```

- 旧 β-1 per-file SPIR-V cache + `createSPIRVFromGLSL` call + VkShaderModule storage を一括削除 (旧 L1025-1130)
- 置換 = 27 line の copy block で `shader_code_text[i]` を `out_sources->emplace_back` (lifetime 延長のみ、SPIR-V 生成は β-2-hook で per-program 側へ移行)

### 2.4 `LLGLSLShader::createShader()` hook 配置 (`llglslshader.cpp` L456-501)

```cpp
mStageSources.clear();
const bool collect_for_vulkan = LLVKLoader::isVulkanInitialized();

vector< pair<string, GLenum> >::iterator fileIter = mShaderFiles.begin();
for (; fileIter != mShaderFiles.end(); fileIter++)
{
    std::vector<std::string> stage_sources;
    GLuint shaderhandle = LLShaderMgr::instance()->loadShaderFile(
        (*fileIter).first, mShaderLevel, (*fileIter).second,
        &mDefines, mFeatures.mIndexedTextureChannels,
        collect_for_vulkan ? &stage_sources : nullptr);
    // ... (既存 shaderhandle 0 時 success = false 経路維持) ...
    if (shaderhandle)
    {
        attachObject(shaderhandle);
        if (collect_for_vulkan && !stage_sources.empty())
        {
            mStageSources.push_back({ (*fileIter).second,
                                      (*fileIter).first,
                                      std::move(stage_sources) });
        }
    }
    else { success = false; }
}

if (success && collect_for_vulkan && !mStageSources.empty())
{
    generatePerProgramSPIRV(mStageSources);
}
mStageSources.clear();
mStageSources.shrink_to_fit();
```

- `!mUsingBinaryProgram` block 内 = GL binary cache hit path は hook 経由しない (既存 startup 高速化 mechanism 影響 0)
- `mShaderFiles` loop 完遂後 `mapAttributes()` 直前 = 全 stage 集約後 hook 配置
- `mStageSources.clear() + shrink_to_fit()` で memory peak 緩和 (prep doc §6.1 反映)

### 2.5 `generatePerProgramSPIRV` impl (`llglslshader.cpp` L634-895、260 line)

主要 step:

1. **HBXXH128 hash 計算**: 全 stage `file_name` + 全 source concat → `LLUUID program_hash` (γ'-port-α `mShaderHash` algorithm 1:1 流用)
2. **cache_path 構築**: `<mShaderCacheDir>/<program_hash>_program.spv` (γ'-port-α `.shaderbin` と同 dir 併存)
3. **stages_by_type grouping**: `std::map<GLenum, std::vector<size_t>>` で per-type ordering 確定 (deterministic)
4. **cache hit 試行** (custom container format parsing):
   ```
   [u32 stage_count]
   [repeat: u32 type, u32 spv_word_count, spv_words...]
   ```
   parse error / count mismatch で fall through 再生成
5. **cache miss 経路**:
   - `ensureGlslangInitialized` (anonymous namespace lambda 静的 init guard、process-global 単発)
   - per stage_type concat sources (with `#define LL_VULKAN_GLSL 1\n` 注入 = β-1 範式継承)
   - `std::make_unique<glslang::TShader>` + `setStrings` / `setEnvInput EShClientVulkan` / `setEnvClient EShTargetVulkan_1_2` / `setEnvTarget EShTargetSpv_1_5` + `parse`
   - **`concat_buffers.reserve(stages_by_type.size())`** で vector 再 alloc 防止 (`setStrings` 内部の raw pointer 安定性担保、parse 完了まで lifetime 維持)
6. **全 stage 単一 `glslang::TProgram` link**:
   - 各 stage `TShader` を `addShader` → `link()` (β-1 per-file isolation 解消 = forward decl cross-stage 自動解決)
7. **per stage `GlslangToSpv`**:
   - `SpvOptions {generateDebugInfo=false, stripDebugInfo=true, disableOptimizer=true, validate=false}` (γ'-port-α `createSPIRVFromGLSL` 範式 1:1 継承)
8. **cache write**: 上記 container format で `<program_hash>_program.spv` に永続化
9. **VkShaderModule 生成** (1:N file_name:VkShaderModule 設計):
   - 各 `file_name` 毎に独立 `vkCreateShaderModule` 呼出 (destroy-time dangling 回避、複数 file 共有 module でなく per-file module で lifetime 分離)
   - `LLVKLoader::loadSpirvShaderModuleFromMemory()` 経由
   - `mgr->mVk{Vertex,Fragment}ShaderModules[stage.file_name]` 格納
10. **LL_INFOS log**: cache hit/miss + program name + stage count + file count (structural log、`#Vulkan#` channel)

### 2.6 binding 番号確定値 (sub-doc 07 §3.1 sub-step 7.2-7.4、β-1 範式継承、bundle-A patch 必須遵守)

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

## 3. sub-step 4.3-γ'-port-β-2-bundle-A 着手境界 (次 session)

### 3.1 bundle-A scope (handoff-prep §3 axis (b) bundle-A 継承)

- **対象 file 数**: 全 **217 file** (base 228 − AYAstorm 改変 11)
- **対象範囲**: uniform 全件 + sampler 全件
- **patch 範式**: `#ifdef LL_VULKAN_GLSL` 分岐内に下記注入 (β-1 範式継承、GL path `#else` 保持で AYAstorm 改変 11 file 互換 + 段階 1-4.3-β-2-hook 動作維持):
  - per-frame uniform → `layout(set=0, binding=N) uniform <Block> { ... };` (binding 0=ViewProj / 1=Lights / 2=Atmosphere)
  - per-material uniform → `layout(set=1, binding=0) uniform MaterialUBO { ... };`
  - per-draw uniform → `layout(set=2, binding=0) uniform PerDrawUBO { ... };`
  - sampler → `layout(set=1, binding=M) uniform sampler2D <name>;` (binding 1=DiffuseTex / 2=NormalTex / 3=SpecTex、AO/emissive は後続 binding 番号、sub-doc 07 §3.1 sub-step 7.3 確定値遵守)
- **incremental SPIR-V 生成成功率 measurement (bundle-A 単独完遂時)**:
  - `~/.ayastorm_x64/cache/shader_cache/*_program.spv` 数 (期待 = bundle-A 単独では parse pass 数向上、link/SPIR-V 生成は location qualifier 不足で部分通過、>>0% 期待 controlled)
  - `grep -cE "PerProgramSPIRV.*cache (hit|miss)"` ログ件数
  - `grep -cE "PerProgramSPIRV.*(parse failed|link failed)"` ログ件数

### 3.2 bundle-A 着手前チェックリスト

- [ ] `git fetch origin` で最新確認 (feedback_git_fetch_first)
- [ ] `feature/ayastorm-r41-gl-removal` branch HEAD = `fcf2b6c508` 確認
- [ ] sub-doc 06 §1.2.4 (α) macro 注入経路 / sample pattern 3 件 / binding 番号 全 base file 共通既定値 を再読 (§2.4-2.6 と整合確認)
- [ ] sub-doc 07 §3.1 sub-step 7.2-7.4 binding 番号確定値 再読 (§2.6 表と整合確認)
- [ ] `handoff-substep-4-3-gamma-prime-port-beta-2-prep.md` §3 axis (b) 217 file bundle 構成 + §4 AYA 承認境界 + §5 cadence + §6 risks 再読
- [ ] 本 handoff §2.4-2.6 β-2-hook 範式再読 (binding 番号確定値 = bundle-A 直接 input)
- [ ] β-2-hook commit `fcf2b6c508` の diff (4 file +394/-100) で hook 配置確認
- [ ] AYAstorm 改変 11 file リスト確定 (sub-doc 06 §3.1 sub-step 6.5 untouched 維持対象):
  - Picker 2 file: `class1/deferred/fsObjectIDF.glsl` + `class1/deferred/fsObjectIDV.glsl`
  - Cinematic BD 2 file: `cinematic_bd/class1/deferred/shadowUtil.glsl` + `cinematic_bd/class3/deferred/screenSpaceReflUtil.glsl`
  - Visual Realism 7 file: `class1/deferred/godrays{V,F}.glsl` + `class1/deferred/volumetricLightF.glsl` + `class3/deferred/volumetricLightF.glsl` + `class1/deferred/blurLight{V,F}.glsl` + `class1/windlight/atmosphericsFuncs.glsl`
- [ ] 217 file 確定 list 取得 (`find indra/newview/app_settings/shaders -name "*.glsl" -not -path "*/cinematic_bd/*"` から 11 file 除外で 217 件想定、要 grep 確認)
- [ ] Agent 並列 patch の skip list として 11 file path を渡す機構の準備 (prep doc §6.10 反映)

### 3.3 bundle-A 推奨 cadence

1. **bundle-A-trace** (217 file 確定 list 取得 + per-file uniform/sampler 列挙 + binding 番号既定値割当 plan、Agent 並列で 3-4 件 trace)
2. **bundle-A-prep** (実装 plan 起草 = patch 順序 + binding 番号 mapping table 確定 + 11 file untouched 自動 verify mechanism、AYA review)
3. **bundle-A-patch-1** (Agent 並列 patch、partial commit 単位、~50 file 単位等で session 内分割実行可能だが大規模 scope なので fresh context 推奨)
4. **bundle-A-patch-2** ... (反復、bundle-A 全 217 file 完遂まで)
5. **bundle-A-build-verify** (autobuild configure + build、warning 0 件確認、`llglslshader.cpp.o` + `llshadermgr.cpp.o` 再構築 clean、packaging 完遂)
6. **bundle-A-install-verify** (install + cache clear)
7. **bundle-A-launch-verify** (AYA launch verify、SPIR-V 生成成功率 1 段階目 measure、ログ確認)
8. **bundle-A-commit** (AYA 明示指示下、`feat(r41): sub-step 4.3-γ'-port-β-2-bundle-A 完遂` commit)
9. **bundle-A-handoff** (bundle-B 着手境界 handoff)

### 3.4 bundle-A 完遂後の次段境界

- **bundle-A 完遂** → bundle-B 着手 (228 file location qualifier、varying/attribute/out/in 全件、A+B 段階 measurement)
- **bundle-A+B 完遂** → bundle-C 着手 (228 file entry point + #version 460 + extension declare、A+B+C 段階 measurement = SPIR-V 生成成功率 >>0% 目標)
- **bundle-A+B+C 完遂** → β-2-verify → β-2-handoff → γ'-port-γ (PSO 構築) 着手境界

---

## 4. risks/caveats

- §4.1 **`mUsingBinaryProgram` true path は hook 経由しない** (既存 GL binary cache 経路維持、AYAstorm 既存 startup 高速化 mechanism 影響 0、bundle-A patch の Vulkan SPIR-V 生成は GL binary cache 初回 miss + cache clear 時のみ fire = clean install + cache clear で必ず確認)
- §4.2 **`attachShaderFeatures` 経由 utility file は `mShaderFiles` 外で `mStageSources` 未捕捉** = bundle-A/B/C で 228 file self-contained 化 (entry point + #version 460 + extension declare) が前提、bundle 完遂時に utility file dependency 解消 (bundle-C scope)
- §4.3 **`createSPIRVFromGLSL` (γ'-port-α 配置 β-1 method) は dead code 残置**: caller 消滅で実害なし、γ'-port-β 完遂時 cleanup commit で削除予定 (本 handoff scope 外)
- §4.4 **cache invalidation = GLSL source 変更 → hash 変化 → 新規 cache file 生成、旧 cache file は GC されない**: bundle-A/B/C 適用順次 hash 変化で旧 cache 自動退避、累積 disk usage は γ'-port-γ 以降の TTL/LRU 検討 scope
- §4.5 **`glslang::InitializeProcess()` 二重 init は function-local static + lambda guard で 1 回限定** + `createSPIRVFromGLSL` の独立 guard と独立だが glslang 内部 ref count 管理で重複呼出 safe
- §4.6 **binding 番号 全 base file 共通既定値の整合担保** (§2.6 表 = sub-doc 07 §3.1 sub-step 7.2-7.4 確定値): bundle-A patch で逸脱禁止、逸脱発見時は本 handoff + sub-doc 07 同期改訂、Agent 並列 patch の prompt に binding 番号 mapping table を必須注入
- §4.7 **AYAstorm 改変 11 file untouched 維持** (charter §3 #1 acceptance 担保): bundle-A patch から **機械的に除外**、Agent 並列 patch で 11 file path を skip list として渡す (prep doc §6.10 反映)、別 mechanism は後段 4.3-ε'/ζ' で対応
- §4.8 **Agent 並列 patch の semantic consistency 担保**: bundle commit 単位 review で吸収、各 bundle commit で SPIR-V 生成成功率 measure (handoff-prep §6.2 + §6.7 反映)
- §4.9 **bundle-A 単独 SPIR-V 生成成功率の期待値**: uniform 解決のみで location qualifier 不足のため parse pass 数向上は期待できるが SPIR-V 生成成功率は controlled (bundle-A+B 完遂時に向上、bundle-A+B+C 完遂時に >>0% 目標、handoff-prep §6.9 incremental measurement 設計通り)
- §4.10 **β-2-hook 残存課題 (parse failed 226 件全件)** を bundle-A 単独で完全解消できない場合: bundle-A+B 完遂後 measurement で進行確認、bundle-C 完遂後でも >>0% に届かない場合は別 root cause 調査 (glslang version 不整合 / SpvOptions tuning / extension declare 不足等)、case ② case-validity rollback は最後の選択肢 (charter §7.5 boundary refine 範囲外、再 prep doc 必要)
- §4.11 **context budget concern**: 217 file patch 工程は bundle 単位 session 分割推奨 (fresh context 推奨、prep doc §6.8 反映)
- §4.12 **AYA 「OK」明示 review PASS 下 commit 厳守**: feedback_no_auto_commit、bundle commit ごとに AYA 明示指示要

---

## 5. 関連 doc / memory cross reference

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` §2 領域 6 + §3 #1/#4 + §7.5 boundary refine 履歴 (2026-06-01 spec-revision entry)
- `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` §1.2.2/§1.2.3/§1.2.4/§3.1 sub-step 6.3/§6.5
- `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` §3.1 sub-step 7.2-7.4/§6.5
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-prep.md` (commit `b7e2dbc72e`、β-2 cadence + bundle-A/B/C 構成 引継 source)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-1-complete.md` (β-1 per-file model 廃止前夜 marker、役割完了)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-alpha-complete.md` (γ'-port-α exemplar PoC marker、役割完了)
- memory `project_ayastorm_r41_vulkan_migration.md` (status active = β-2-hook 完遂 / 次 = β-2-bundle-A)
- memory `MEMORY.md` index (r41 milestone 行 = β-2-hook 完遂 / 次 = β-2-bundle-A)
- feedback rules: `feedback_doubt_self_first` / `feedback_falsification_as_progress` (β-1 link fail を honest に記録 → β-2-hook で architectural 解消) / `feedback_no_scope_shrink` (5 scope literal satisfy、prep doc §2 axis (a) 全件充足) / `feedback_proactive_handoff` (本 handoff 自体) / `feedback_self_verify_before_handoff` (hook fire 226 件 / SPIR-V 0 件 / parse 失敗パターン 自己確認後 AYA 報告) / `feedback_one_step_at_a_time` (bundle-A 着手は AYA 明示指示要) / `feedback_use_agents_proactively` (bundle-A-trace + Agent 並列 patch 推奨) / `feedback_remove_verification_logs` (LL_INFOS cache hit/miss + structural log は scope 外で保持) / `feedback_no_claude_coauthor` / `feedback_no_auto_commit` (AYA 「OK commit して」明示指示下 commit)

---

## 6. 次 session 着手 prompt (AYA → 次 Claude session 投入用)

> r41 sub-step 4.3-γ'-port-β-2-bundle-A に着手してください。本 sub-step は **217 file (base 228 − AYAstorm 改変 11) の uniform → UBO 化 + sampler binding `layout(set=N, binding=M)` qualifier 注入** で、大規模 scope のため **bundle 単位で fresh context** を切る前提です。
>
> まず以下を順に読んで現状把握してください:
> 1. `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md` (本 handoff、§2 β-2-hook で確立した実装範式 + §2.6 binding 番号確定値表 + §3 bundle-A scope + §4 risks + §6 prompt = 本文)
> 2. `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-prep.md` §3 axis (b) 217 file bundle 構成 + §4 AYA 承認境界 + §5 cadence + §6 risks
> 3. `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` §1.2.4 (α) macro 注入経路 + sample pattern 3 件 + binding 番号 全 base file 共通既定値
> 4. `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` §3.1 sub-step 7.2-7.4 binding 番号確定値
> 5. memory `project_ayastorm_r41_vulkan_migration.md` status + sub-step 4.3-γ'-port-β-2-hook entry (commit `fcf2b6c508`)
> 6. β-2-hook commit `fcf2b6c508` の diff (4 file +394/-100) で hook 配置範式確認
>
> 6 件読了後、**bundle-A-trace** から開始してください。Agent 並列で以下 3-4 件 trace を投入してください:
> - 217 file 確定 list 取得 (`find indra/newview/app_settings/shaders -name "*.glsl"` から AYAstorm 改変 11 file (Picker 2 / Cinematic BD 2 / Visual Realism 7) を除外、確定 list 化)
> - 217 file 全件の uniform 列挙 (`grep -nE "^uniform\s" indra/newview/app_settings/shaders/.../*.glsl` 全件)
> - 217 file 全件の sampler 列挙 (`grep -nE "uniform\s+sampler" ...`)
> - per-file binding 番号既定値割当 plan (sub-doc 07 §3.1 sub-step 7.2-7.4 確定値遵守、`set=0` per-frame / `set=1` per-material+sampler / `set=2` per-draw)
>
> trace 結果報告後、**bundle-A-prep** (実装 plan 起草 = patch 順序 + binding 番号 mapping table 確定 + 11 file untouched 自動 verify mechanism) 着手是非を私 (AYA) に確認してから次に進んでください。`feedback_one_step_at_a_time` + `feedback_no_auto_commit` を厳守、bundle commit は AYA 明示指示下のみ。
