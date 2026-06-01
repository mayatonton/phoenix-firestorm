# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B2-γ 完遂 → 次 sub-bundle 着手境界 handoff (2026-06-01)

**parent commit**: `1cd9303872` (B2-γ patch、本 handoff の直接 parent) / `71a8f87cde` (B3 patch 範式継承元) / `44422cb14f` (B3-complete handoff doc commit + §12 metric 訂正範式継承元)
**HEAD**: `1cd9303872` on `feature/ayastorm-r41-gl-removal`
**本 doc 位置付け**: B2-γ 完遂状態 + 次 sub-bundle (B?-δ 候補 = non-opaque uniforms 223 件 `layout(set,binding) uniform` 化 / B? = 残 mixed cleanup) 着手判断境界 を fresh context 引継 用に確定する doc-only handoff。B3-complete `44422cb14f` 範式継承。**Vulkan utility source cache + per-program attached utility tracking + utility concat hook + createShader reorder 範式 (3 件 = LLShaderMgr 側 cache map + LLGLSLShader 側 attached utility vector + generatePerProgramSPIRV 内 stage 先頭 prepend) を B2-γ で新規確立**。`generatePerProgramSPIRV()` 内 utility 集合を file_name key で引き stage 先頭 prepend する仕組みで、glslang `link failed for program` 55 件 + `No function definition (body) found` 206 件の直接観測 100% 解消。AYA 「OK」明示承認下で commit (no auto-commit)。

---

## §1 起草目的

β-2-bundle-B scope 第三 sub-bundle B2-γ (utility shader linkage 整理 + per-program SPIR-V hook reorder) を 4 file (`indra/llrender/llshadermgr.{h,cpp}` + `indra/llrender/llglslshader.{h,cpp}`) +141 insertions / -11 deletions の structural addition で完遂した状態を確定し、次 sub-bundle 着手境界 を fresh context に引継ぐ。B3 (commit `71a8f87cde` = SPIR-V Vulkan profile override per-stage prepend、#version cluster 36→0) の cascade で B2-α/B2-β 段階から残存していた `No function definition` 206 件 + `link failed for program` 55 件 を、**shader file 改変ではなく C++ Vulkan path 内部の utility source cache + per-program attach 追跡 + concat hook 範式で直接解消**。加えて副次効果として utility 経由の `location` 13 件 + `binding` 45 件 + `missing #endif` 15 件 も全消失 = 合計 **-334 errors 直接 + 副次解消**。同時に cascade emergence で `non-opaque uniforms` 74→223 (+149) + `parse failed for stage` 147→223 (+76) が次 sub-bundle scope として露出。

---

## §2 B2-γ 完遂 status

| 項目 | 値 |
|---|---|
| Scope | (1) `LLShaderMgr` 側 Vulkan utility source cache `mVertex/FragmentShaderSourceCache` 宣言 + `loadShaderFile()` 内 populate 追加 / (2) `LLGLSLShader` 側 per-program attached utility tracking `mVulkanAttached{Vertex,Fragment}Utilities` 宣言 + `attachVertex/FragmentObject()` 内 push_back 追加 / (3) `generatePerProgramSPIRV()` 内 utility 集合 prepend hook + `createShader()` SPIR-V hook reorder (attachShaderFeatures() 後ろへ移動) |
| 修正 file 数 | **4 file** (`indra/llrender/llshadermgr.h` + `indra/llrender/llshadermgr.cpp` + `indra/llrender/llglslshader.h` + `indra/llrender/llglslshader.cpp`) |
| 変更行数 | **+141 / -11** (llshadermgr.h +9 / llshadermgr.cpp +43 / llglslshader.h +9 / llglslshader.cpp +80/-11、structural addition + reorder) |
| 修正範囲 | LLShaderMgr: line 484 後 + line 1036-1051 直後 / LLGLSLShader: line 386 後 + `attachVertexObject` (line 924-) + `attachFragmentObject` (line 943-) + `createShader` (line 432-) + `generatePerProgramSPIRV` (line 636-) |
| Agent 投入 | **0 件** (Agent 不要、4 file の協調修正を single Edit chain で完遂、scope = C++ shader manager 内部のみ) |
| shader file 触り | **0 件** (skip list 13 含む全 shader file untouched、A1-A7/A8-recovery/B1/B2-α/B2-β/B3 既処理 file 全 byte-for-byte 維持) |
| AYAstorm 改変保全 | **GL path 全不変** (loadShaderFile 内 `shader_code_text[0] = "#version XXX\n"` strdup logic 不変、`glCreateShader`/`glShaderSource`/`glCompileShader`/`glAttachShader` 呼出順序不変、collect_for_vulkan=false の GL compile path 完全 untouched) |
| AYA cold cache launch verify | **PASS** (起動成立 + clean shutdown + Goodbye! 1 件 + Vulkan device/instance destroyed 各 1 件 + 実 FATAL/SIGSEGV/crash 0 件) |
| commit | `1cd9303872` (AYA 「OK」明示指示下) |
| metric vs B3 baseline link failed | **-55 ✓ B2-γ patch 効果完全直接観測** (55→0, 100% 解消) |
| metric vs B3 baseline No function definition | **-206 ✓ B2-γ patch 効果完全直接観測** (206→0, 100% 解消) |
| metric vs B3 baseline location/binding/#endif | **-73 副次解消** (13+45+15→0+0+0, utility cluster cascade) |
| metric net delta | **-109 errors** (直接 -261 + 副次 -73 + cascade emergence +225 = -109 controlled cascade improvement) |

### §2.1 既処理 sub-bundle との関係

| sub-bundle | 関係 |
|---|---|
| A1-A7 | uniform/sampler/UBO block 注入 (binding scope)、本 step touch 0 件、byte-for-byte 維持 |
| A8-recovery | AYAstorm 改変 (Cinematic BD shadowUtil/screenSpaceReflUtil + Visual Realism volumetricLight/blurLightF/godraysF 系 5 file) UBO 復活、本 step も skip list 維持で untouched |
| B1 | materialF.glsl MaterialUBO_Legacy 化 (case 2 file-local override)、本 step touch 0 件 |
| B2-α | varying (vertex_out + fragment_in) + fragment_out 全 program 注入 (canonical Table A/B 確立)、本 step 全 shader file untouched |
| B2-β | vertex_in/VBO attribute 全 program 注入 (canonical Table C 確立)、本 step 全 shader file untouched |
| B3 | SPIR-V Vulkan profile override per-stage prepend (`generatePerProgramSPIRV()` 内 stage 先頭 #version 460 + extension + LL_VULKAN_GLSL 出力)、本 step は B3 範式の **直後** に utility prepend を hook、createShader の SPIR-V 生成位置を attachShaderFeatures() 後ろに reorder |

---

## §3 設計範式 (B2-γ で新規確立)

### §3.1 Vulkan utility source cache 範式 (B2-γ で新規確立)

**設計原則**: `LLShaderMgr` 側で `std::map<std::string, std::vector<std::string>> mVertexShaderSourceCache` / `mFragmentShaderSourceCache` を保持。key = filename (例 `"deferred/globalF.glsl"`)、value = `loadShaderFile()` が組み立てた preprocessed `shader_code_text[]` の copy (各 entry の sources[0] は GL profile `#version XXX\n`)。**充填条件** = `loadShaderFile` 内 `free()` 直前 + `out_sources == nullptr` (= utility shader として attach する path) + `LLVKLoader::isVulkanInitialized()` (Vulkan path active)。GL path は条件外で完全不変。

**範式継承元**: B3 §3.2 (GL profile / Vulkan profile 分離範式) を **utility shader にも適用拡張**。B3 は per-program sources[] の中での分離だったのに対し、B2-γ は **utility shader source** の cache 保持 + Vulkan path 局所利用。

### §3.2 Per-program attached utility tracking 範式 (B2-γ で新規確立)

**設計原則**: `LLGLSLShader` 側で `std::vector<std::string> mVulkanAttachedVertexUtilities` / `mVulkanAttachedFragmentUtilities` を保持。**充填位置** = `LLGLSLShader::attachVertexObject(file)` / `attachFragmentObject(file)` 内 `glAttachShader` + `stop_glerror()` 直後、Vulkan 時 `push_back(object_path)`。**attach 順保持** = `LLShaderMgr::attachShaderFeatures()` が `loadShaderFile()` した順序を踏襲、後で `generatePerProgramSPIRV()` で prepend する utility 順序 = attach 順 = `attachShaderFeatures()` 仕様順。

**Why critical**: GLSL function definition の前方参照は許容されない。よって `globalF.glsl` (utility) が `softenLightF.glsl` (caller) より **先** に concat されないと `No function definition` で reject。attach 順を per-program で記録し、prepend 時に同順で復元することで、`attachShaderFeatures()` で正しく順序付けられた include relation を SPIR-V concat にも転写。

**範式継承元**: B2-α §3.1 / B2-β §3.1 (canonical location 順 cross-file 固定) の **動的 per-program 版**。location は global enum で固定 (CPU 側 source-of-truth)、utility 順は per-program に変動 (attach 順 = runtime decision)。

### §3.3 Utility concat hook + createShader reorder 範式 (B2-γ で新規確立)

**設計原則**: `generatePerProgramSPIRV()` 内、B3 が確立した stage 先頭 `#version 460` + `#extension GL_KHR_vulkan_glsl : enable` + `#define LL_VULKAN_GLSL 1` prepend の **直後**、program-specific `mShaderFiles` source append loop の **前** に utility source prepend block を挿入。utility source は `LLShaderMgr::mVertex/FragmentShaderSourceCache` から `mVulkanAttached{Vertex,Fragment}Utilities` の各 filename を key に lookup、各 utility entry の sources[1..N] を順次 append (sources[0] = GL profile `#version XXX\n` は skip、B3 範式整合)。

**createShader reorder**: β-2-β までは `createShader()` 内 `if (!mUsingBinaryProgram)` block 内末尾で `generatePerProgramSPIRV(mStageSources)` を呼出していたが、これは **`attachShaderFeatures()` 完了前** = utility vector 未充填状態 = 「No function definition」206 件の構造原因。本 B2-γ で:
- block 内末尾の `generatePerProgramSPIRV()` call + `mStageSources.clear/shrink_to_fit` を削除
- `attachShaderFeatures(this)` call **直後** に移動、`success && LLVKLoader::isVulkanInitialized() && !mStageSources.empty()` guard 下で呼出
- 終端 cleanup で `mStageSources` + `mVulkanAttached{Vertex,Fragment}Utilities` 両方 `clear()` + `shrink_to_fit()`

これにより utility vector 充填済の状態で SPIR-V 生成 hook が走り、utility source が cache から正しく引かれて prepend される。

### §3.4 patch literal (主要 4 hook)

**(a) `llshadermgr.h` line 484 後** (+9 行):
```cpp
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B2-γ: Vulkan utility source cache。
// key = filename (例 "deferred/globalF.glsl")、value = loadShaderFile() が組み立てた
// preprocessed shader_code_text[] copy。generatePerProgramSPIRV() から per-program
// utility prepend 時に lookup。GL path は touch しない (collect_for_vulkan=false の
// loadShaderFile call では充填条件 out_sources==nullptr が成立しない)。
std::map<std::string, std::vector<std::string>> mVertexShaderSourceCache;
std::map<std::string, std::vector<std::string>> mFragmentShaderSourceCache;
```

**(b) `llshadermgr.cpp` line 1036-1051 直後** (+43 行):
```cpp
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B2-γ: Vulkan utility source cache 充填。
// out_sources==nullptr = utility attach path (collect_for_vulkan=false の GL path も
// 含むが、isVulkanInitialized() guard で Vulkan 起動時のみ充填、GL path 不変)。
// free() 直前に置くことで shader_code_text[i] が alive な間に std::string copy。
if (out_sources == nullptr && LLVKLoader::isVulkanInitialized())
{
    std::vector<std::string>* cache_entry = nullptr;
    if (type == GL_VERTEX_SHADER)        cache_entry = &mVertexShaderSourceCache[filename];
    else if (type == GL_FRAGMENT_SHADER) cache_entry = &mFragmentShaderSourceCache[filename];
    if (cache_entry)
    {
        cache_entry->clear();
        cache_entry->reserve(shader_code_count);
        for (GLuint i = 0; i < shader_code_count; ++i)
        {
            if (shader_code_text[i]) cache_entry->emplace_back(shader_code_text[i]);
            else                     cache_entry->emplace_back();
        }
    }
}
```

**(c) `llglslshader.h` line 386 後** (+9 行):
```cpp
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B2-γ: per-program attached utility tracking。
// attachVertex/FragmentObject() で glAttachShader 直後に push_back、attach 順保持。
// generatePerProgramSPIRV() で stage 先頭 prepend 時に LLShaderMgr の source cache から
// lookup、attach 順 = attachShaderFeatures() 仕様順で utility source 復元。createShader
// 完遂後 clear + shrink_to_fit。
std::vector<std::string> mVulkanAttachedVertexUtilities;
std::vector<std::string> mVulkanAttachedFragmentUtilities;
```

**(d) `llglslshader.cpp` `attachVertexObject` / `attachFragmentObject` 内** (+5 行 × 2):
```cpp
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B2-γ: per-program attached utility tracking。
// glAttachShader + stop_glerror() 成功後に push_back、attach 順 = LLShaderMgr::
// attachShaderFeatures() 仕様順を踏襲。GL link path は本 push_back 後の return true 経路
// で不変、glAttachShader 失敗時 (count==0) は push_back せず return false。
if (LLVKLoader::isVulkanInitialized())
{
    mVulkanAttachedVertexUtilities.push_back(object_path);   // or Fragment
}
```

**(e) `llglslshader.cpp` `createShader` 内 reorder + cleanup**:
```cpp
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B2-γ: SPIR-V 生成 hook を attachShaderFeatures()
// 完了後に移動 (β-2-β までは前 = utility vector 未充填状態 = No function definition 206
// 件の構造原因)。attachShaderFeatures() で utility shader を attach し終えた状態で
// generatePerProgramSPIRV() を呼出すことで、mVulkanAttached{Vertex,Fragment}Utilities
// vector が充填済 → utility source prepend が機能する。
if (!mUsingBinaryProgram && success && LLVKLoader::isVulkanInitialized() && !mStageSources.empty())
{
    generatePerProgramSPIRV(mStageSources);
}
mStageSources.clear(); mStageSources.shrink_to_fit();
mVulkanAttachedVertexUtilities.clear();   mVulkanAttachedVertexUtilities.shrink_to_fit();
mVulkanAttachedFragmentUtilities.clear(); mVulkanAttachedFragmentUtilities.shrink_to_fit();
```

**(f) `llglslshader.cpp` `generatePerProgramSPIRV` 内 utility prepend block** (B3 範式の `#version 460` + extension + LL_VULKAN_GLSL 直後、stage_indices loop の **前**):
```cpp
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B2-γ: utility source prepend。
// LLShaderMgr の source cache から per-program attached utility filename key で
// lookup、attach 順 = attachShaderFeatures() 仕様順で utility source[1..N] を順次
// append (sources[0] = GL profile #version XXX\n は skip、B3 範式整合)。
// cache miss は LL_WARNS で記録、該当 utility は skip (safe fallback)。
const std::vector<std::string>* utility_files = nullptr;
const std::map<std::string, std::vector<std::string>>* source_cache = nullptr;
if (stage_type == GL_VERTEX_SHADER)
{
    utility_files = &mVulkanAttachedVertexUtilities;
    source_cache  = &mgr->mVertexShaderSourceCache;
}
else if (stage_type == GL_FRAGMENT_SHADER)
{
    utility_files = &mVulkanAttachedFragmentUtilities;
    source_cache  = &mgr->mFragmentShaderSourceCache;
}
if (utility_files && source_cache)
{
    for (const std::string& util_file : *utility_files)
    {
        auto it = source_cache->find(util_file);
        if (it == source_cache->end())
        {
            LL_WARNS("Vulkan") << "B2-γ utility cache miss for '" << util_file << "'" << LL_ENDL;
            continue;
        }
        const std::vector<std::string>& util_sources = it->second;
        for (size_t i = 1; i < util_sources.size(); ++i)
            concatenated.append(util_sources[i]);
    }
}
```

### §3.5 cache lifetime / GC 規範

- `mVertex/FragmentShaderSourceCache` は process lifetime 保持 (`LLShaderMgr` singleton-like、shader reload 時は `loadShaderFile()` で overwrite)
- `mVulkanAttached{Vertex,Fragment}Utilities` は per-program 単位、`createShader()` 完遂後 `shrink_to_fit()` で即時解放
- memory footprint: utility ~19 file × ~数 KB = 数 MB peak、起動時 1 回のみ
- `mUsingBinaryProgram` path では `attachShaderFeatures()` がそもそも utility attach しないため populate 0、cleanup 時 clear で無害

---

## §4 cold cache launch verify metric (2026-06-01 20:09 → 20:10)

baseline = `~/.ayastorm_x64/logs/AYAstorm.old.b2-gamma-baseline` (B3 verify 19:15 mtime、commit `71a8f87cde` 再生成元)
verify   = `~/.ayastorm_x64/logs/AYAstorm.log` (B2-γ verify 20:09 mtime、commit `1cd9303872` cold cache launch)

| 項目 | B3 baseline | B2-γ verify | delta | 解釈 |
|---|---|---|---|---|
| §4.1 `link failed for program` | 55 | **0** | **-55 ✓** | **B2-γ patch 効果完全直接観測** (100% 解消、utility prepend で cross-stage 関数 resolve 達成) |
| §4.2 `No function definition` | 206 | **0** | **-206 ✓** | **B2-γ patch 効果完全直接観測** (100% 解消、utility source prepend で全 function body 提供) |
| §4.3 `0:N: 'location'` | 13 | **0** | **-13** | utility cluster 副次解消 (utility 内 location 宣言が本体に prepend されることで局所整合) |
| §4.4 `0:N: 'binding'` | 45 | **0** | **-45** | utility cluster 副次解消 (utility 内 sampler/UBO binding 宣言が本体側 layout の整合 path に乗る) |
| §4.5 `missing #endif` | 15 | **0** | **-15** | utility cluster 副次解消 (utility 側 preprocessor balance が本体側に prepend されることで `#ifdef LL_VULKAN_GLSL` block 整合) |
| §4.6 `parse failed for stage` | 147 | **223** | **+76** | cascade emergence (深部到達: utility prepend 成功 → 旧 line 1 reject 程度から body parse 段階深化、新 first-error = uniform 宣言行) |
| §4.7 `non-opaque uniforms outside a block` | 74 | **223** | **+149** | cascade emergence (utility prepend で uniform sampler2D 宣言全件露出、Vulkan profile では `layout(set,binding) uniform` 化または uniform block 化が必要、次 sub-bundle scope) |
| §4.8 `extension not supported: GL_KHR_vulkan_glsl` WARNING | 147 | **223** | **+76** | parse failed for stage と 1:1 対応 (WARNING のみ、stop 条件は他 ERROR、無害な glslang Vulkan profile 暗黙 enable 警告) |
| §4.9 #version cluster (3 phrase) | 0 | **0** | **±0** | B3 既処理維持 (`must occur first` / `bad profile name` / `bad tokens following profile` 全 0) |
| §4.10 **7 error category 合計** (link+No function def+location+binding+#endif+parse+non-opaque) | 555 | **446** | **-109 ✓** | **controlled cascade improvement** (直接 -261 + 副次 -73 + cascade +225) |
| §4.11 shader_cache 再生成 | 245 | **224** | **-21 (観測点)** | **§12 観測記録枠 で記録** (起動成立 + clean shutdown 成立、致命的 regression ではないが原因未 trace) |
| §4.12 FATAL / SIGSEGV / crash (実) | 0 / 0 / 0 | **0 / 0 / 0** | ±0 | grep match 3 件は全 `settings_crash_behavior.xml` ファイル名 false positive、実 0 件 |
| §4.13 起動成立 + clean shutdown | PASS | **PASS** | - | Goodbye! 1 件 + Vulkan device destroyed 1 件 + Vulkan instance destroyed 1 件 + status: stopped、charter §3 #1 acceptance 担保 |

**net delta -109 (7 category)** = link -55 + No function def -206 + location -13 + binding -45 + #endif -15 + parse +76 + non-opaque +149 = **直接 -261 + 副次 -73 + cascade +225 = -109 controlled cascade improvement**。link failed + No function definition の **-261 解消が B2-γ 直接効果** (100% 一致、§3 設計範式 3 件の構造的整合性証明)。cascade emergence +225 は parse 深部到達 (147→223 = 76 stage が新たに body parse 突破、全 223 件が `non-opaque uniforms` first-error) = next layer 露出 = progress signal、次 sub-bundle (B?-δ 想定) で `uniform sampler2D` 等を `layout(set,binding) uniform` 化または uniform block 化することで集合的解消想定。

### §4.14 metric 整合性 self-check (B3 §12 教訓適用)

`feedback_doubt_self_first` / `feedback_build_only_verified` 教訓を B2-γ 起草前に適用、handoff §4 metric 主張全件を以下の手順で確定:

1. **明示 literal pattern grep のみ採用** (`link failed for program` / `No function definition` / `non-opaque uniforms outside a block` / `0:N: 'location'` / `0:N: 'binding'` / `missing #endif` / `parse failed for stage` / `must occur first|bad profile name|bad tokens following profile`)、case-insensitive partial match (`link` 一致) は category metric として **採用しない**
2. **structural touch consistency**: B2-γ patch は (a) utility source cache populate (loadShaderFile 内) + (b) per-program attach 追跡 (attachShader 内) + (c) generatePerProgramSPIRV 内 utility prepend hook + (d) createShader SPIR-V hook reorder の 4 hook で、**link failed + No function definition の直接解消** + **utility 内 location/binding/#endif 宣言の副次 prepend 効果** を構造的に説明可能。non-opaque uniforms cascade emergence は parse 深部到達による次層露出 (B2-γ patch が uniform 宣言行に touch しないため解消は構造的に不可能、これは期待通り)。
3. **B2-β baseline → B3 baseline → B2-γ verify の 3 段階 pair-grep** で各 cluster の delta が単調 (B3 で -36 #version + ±0 他、B2-γ で -261 link/No function def + -73 location/binding/#endif + +225 non-opaque/parse) であることを確認、ジグザグ regression なし
4. § 4.10 「合計」算出は `link failed` + `No function definition` + `'location'` + `'binding'` + `missing #endif` + `parse failed for stage` + `non-opaque uniforms` の **7 category 数値の単純和**、partial overlap (parse 通過 stage と link 段階 program の包含関係) は **意図的に重複計上維持** (literal error message 数 = 開発者向け debug signal の整数値、bundle-B 完遂時の単一閾値判定用)

---

## §5 self-verify 結果

| # | check | result |
|---|---|---|
| §5.1 | git diff | **4 files changed, 141 insertions(+), 11 deletions(-)** (`indra/llrender/llshadermgr.h` +9 / `indra/llrender/llshadermgr.cpp` +43 / `indra/llrender/llglslshader.h` +9 / `indra/llrender/llglslshader.cpp` +80/-11) |
| §5.2 | shader file touch | **0 件** (skip list 13 含む全 shader file untouched、A1-A7/A8-recovery/B1/B2-α/B2-β/B3 既処理 file 全 byte-for-byte 維持) |
| §5.3 | C++ syntax | **OK** (`{}` balanced、`append()` chain、empty utility list (attachNothing) で iteration skip → B3 動作 degrade、cache miss は LL_WARNS + skip で safe fallback、map iterator dangling 回避) |
| §5.4 | GL path 影響 | **0** (`generatePerProgramSPIRV()` は `collect_for_vulkan=true` block 内のみ呼出、`loadShaderFile()` strdup logic 完全 untouched、`shader_code_text[0]` GL profile `#version XXX\n` も完全不変、`glCreateShader`/`glShaderSource`/`glCompileShader`/`glAttachShader` 呼出順序不変) |
| §5.5 | charter §3 #1 担保 | **OK** (Vulkan path 内部改造のみ、GL path byte-for-byte 維持、起動成立 + clean shutdown で実証、shader_cache 224 件 (-21) は §12 観測記録枠 = 起動/shutdown 成立で致命性なし) |
| §5.6 | cache 充填 timing | **OK** (`loadShaderFile()` 内 `free()` 直前 + `out_sources == nullptr` 条件 = shader_code_text[i] alive 中 std::string copy、out_sources 引数指定時 (= per-stage source 収集 path) は 充填条件外でスキップ、再帰 loadShaderFile (shader_level retry) は再帰側で cache populate、outer call return 後 body 実行されないので問題なし) |
| §5.7 | cache key 一貫性 | **OK** (loadBasicShaders と attachShaderFeatures が同一 path 文字列を使用、例 `"deferred/globalF.glsl"` / `"avatar/objectSkinV.glsl"` 等、key normalization 不要) |
| §5.8 | 型一致 | **OK** (VS ↔ `mVertexShaderSourceCache` ↔ `mVulkanAttachedVertexUtilities` + FS ↔ `mFragmentShaderSourceCache` ↔ `mVulkanAttachedFragmentUtilities` の対称構造、GL_GEOMETRY_SHADER stage は utility_files=nullptr → prepend skip = B3 動作維持) |
| §5.9 | createShader reorder 影響 | **OK** (β-2-β までは `attachShaderFeatures()` 前で SPIR-V 生成 = utility vector 未充填 = No function definition 206 件構造原因、本 B2-γ で `attachShaderFeatures()` 完了後に移動 = utility vector 充填済で SPIR-V 生成 hook 到達、`mapAttributes`/`mapUniforms` 等の GL link 経路順序不変) |
| §5.10 | empty check 整合 | **OK** (B3 §3.4 範式継承、`if (concatenated.empty())` defensive check は utility 0 件 + program-specific source 0 件の edge case で発動可能、害なし) |
| §5.11 | metric 整合性 (B3 §12 教訓適用) | **OK** (literal pattern grep のみ採用、structural touch consistency 確認、3 段階 pair-grep で単調 delta 確認、合計算出は意図的単純和) |

---

## §6 設計範式 (B2-α/B2-β/B3 継承 + B2-γ 新規確立)

| # | 範式 | 確立 sub-bundle |
|---|---|---|
| §6.1 | Vulkan utility source cache 範式 (LLShaderMgr 側 `mVertex/FragmentShaderSourceCache` で utility shader source を filename key で保持、loadShaderFile 内 free 直前 + out_sources==nullptr + isVulkanInitialized() 条件下 populate) | **B2-γ 新規** |
| §6.2 | Per-program attached utility tracking 範式 (LLGLSLShader 側 `mVulkanAttached{Vertex,Fragment}Utilities` vector で attach 順保持、attachVertex/FragmentObject 内 glAttachShader 直後 push_back) | **B2-γ 新規** |
| §6.3 | Utility concat hook + createShader reorder 範式 (generatePerProgramSPIRV 内 B3 範式 `#version 460` + extension + LL_VULKAN_GLSL 直後に utility source prepend、createShader で SPIR-V hook を attachShaderFeatures() 完了後に移動) | **B2-γ 新規** |
| §6.4 | per-stage Vulkan profile prepend 範式 (同 stage 複数 file concat 時 stage 先頭 1 回のみ Vulkan profile 出力) | B3 確立、B2-γ 継承 (本 step は B3 範式の **直後** に utility prepend を hook) |
| §6.5 | GL profile / Vulkan profile 分離範式 (loadShaderFile strdup 不変 + Vulkan path 内部 override 局所完結) | B3 確立、B2-γ 継承 (utility cache populate も Vulkan-gated、GL 漏洩 0) |
| §6.6 | empty check 残置範式 | B3 確立、B2-γ 継承 (utility 0 件 edge case で defensive 維持) |
| §6.7 | canonical Table C 範式 (vertex_in/VBO attribute 14 location 0-13 cross-file 固定) | B2-β 確立、B2-γ 継承 (untouched) |
| §6.8 | CPU `LLShaderMgr::initAttribsAndUniforms` push_back 順整合範式 | B2-β 確立、B2-γ 継承 (untouched) |
| §6.9 | conditional duplicate (HAS_SKIN guard 両分岐重複) は独立 3-段 swap 注入 | B2-β 確立、B2-γ 継承 (untouched) |
| §6.10 | nesting 規則 (`#ifdef LL_VULKAN_GLSL` nest OK、既 LL_VULKAN_GLSL block 内 nest 禁止) | B2-β 確立、B2-γ 継承 (untouched) |
| §6.11 | Table A 範式 (top-20 varying location 0-19 cross-file 固定) | B2-α 確立、B2-γ 継承 (untouched) |
| §6.12 | Table B 範式 (fragment_out 別 namespace、frag_color=0 / frag_data=0\|1) | B2-α 確立、B2-γ 継承 (untouched) |
| §6.13 | GL #else byte-for-byte 範式 | bundle-A 確立、B1/B2-α/B2-β/B3/B2-γ 継承 |
| §6.14 | 既処理 file UBO untouched 範式 | bundle-A 確立、B1/B2-α/B2-β/B3/B2-γ 継承 |
| §6.15 | 既処理 file varying/frag_out/vertex_in 行 untouched 範式 | B2-α/B2-β 確立、B3/B2-γ 継承 (本 step は shader file 全 untouched) |

---

## §7 risks/caveats

### §7.1 metric net delta -109 (7 category) = controlled cascade improvement

link -55 + No function definition -206 が **B2-γ patch の直接効果**、location/binding/#endif -73 が **utility prepend の副次効果** (合計 -334 解消)、non-opaque uniforms +149 + parse failed for stage +76 が **cascade emergence** (utility prepend で深部到達 → 新 first-error = uniform 宣言行)。bundle-B 全体 (B1-B?) 完遂時の集合的閾値で評価 (bundle-A 同 measurement design)。

### §7.2 cold cache launch 必須

`rm -rf ~/.ayastorm_x64/cache/` (cache 全削除、shader_cache 含む) を sub-bundle 毎 verify 前必須化、本 commit でも実施済 (`AYAstorm.old.b2-gamma-baseline` 退避 + cache 全 clear + 起動 verify + shutdown verify)。

### §7.3 non-opaque uniforms 223 件 = 次 sub-bundle (B?-δ 候補) scope

cascade emergence の `non-opaque uniforms outside a block` 223 件は **Vulkan GLSL 仕様で `uniform sampler2D` 等の opaque-type 以外の uniform を bare で書けない制約** に起因。Vulkan profile では:
- `uniform sampler2D` (opaque type) → そのまま OK
- `uniform float`/`uniform vec4`/`uniform mat4` (non-opaque) → `layout(set=N, binding=M) uniform UniformBlockName { ... }` 形式の **uniform block** 化が必須

223 件全件が `'non-opaque uniforms outside a block' : not allowed when using GLSL for Vulkan` ERROR で reject。次 sub-bundle (B?-δ 想定、名称仮称) で utility shader / program shader の non-opaque uniform 宣言を uniform block 化 (または既存 UBO への統合) する必要。

**scope 推定** = 223 件 / 223 stage = 1 stage 1 件 = 各 stage 先頭で発見される最初の non-opaque uniform 行が first-error。実際の non-opaque uniform 総数は更に多い可能性 (1 stage 内複数宣言は first-error のみ出る)、trace で精緻化。

### §7.4 parse failed for stage 223 件 = non-opaque uniforms と 1:1 対応

`parse failed for stage` 223 件 = `non-opaque uniforms outside a block` 223 件 = `extension not supported: GL_KHR_vulkan_glsl` WARNING 223 件で **完全一致** (1:1:1 対応)。utility prepend 成功で全 stage が body parse 段階突破 → 最初の non-opaque uniform 宣言で first-error 取得という構造。`non-opaque uniforms` 解消で `parse failed for stage` も連動消失想定。

### §7.5 shader_cache 224 件 (B3 baseline 245 件比 -21、§12 観測記録枠)

shader_cache 再生成数が **-21 件減少**。原因未 trace。**致命的でない** (起動成立 + clean shutdown 成立 + 実 crash 0 件)。仮説:
- (a) B2-γ patch で `attachShaderFeatures()` 経由 utility attach が以前と異なる順序/タイミングで起きている可能性 (但し attach 順保持範式で論理的には不変のはず)
- (b) `loadShaderFile()` 内 utility cache populate 時の早期 return / failure path で utility shader compile を skip している可能性
- (c) feature level fallback / fallback shader load の差異 (B3 から B2-γ で間隔があり、AYA system の状態差)

**B2-γ commit 阻害ではない** (AYA 指示 2026-06-01)。次 sub-bundle 着手前に trace 推奨。`AYAstorm.old.b2-gamma-baseline` vs `AYAstorm.log` の `Shader compile success` / `Shader cache hit` / `Loaded shader` 系 INFO 行 diff で原因特定可能性あり。

### §7.6 残 mixed cleanup (B? scope)

cascade emergence で `non-opaque uniforms` 223 件が露出した結果、location/binding/#endif は 0 件まで解消したが、これは「utility cluster の cascade 解消」であり、**非 utility (program 本体側) の location/binding 残置の可能性** は今後 non-opaque uniforms 解消後に再露出する可能性あり。残 mixed cleanup sub-bundle (B? = mixed cleanup) の scope は **B?-δ 完遂後の verify で再確定**。

### §7.7 `mUsingBinaryProgram` 経路の utility vector populate

`mUsingBinaryProgram == true` (= binary program cache hit) path では `attachShaderFeatures()` がそもそも utility attach しないため `mVulkanAttachedUtilities` populate 0、`generatePerProgramSPIRV()` 呼出 guard でも skip、cleanup 時 clear で無害。GL link path も binary load 経路で utility 不要、整合性問題なし。

### §7.8 cache invalidation / shader reload 時の整合

`loadShaderFile()` 内 cache populate は **overwrite** (`cache_entry->clear() + reserve + emplace_back`)、shader reload (= 同 filename での再 loadShaderFile call) 時に最新内容で上書き。stale cache 問題なし。`mVulkanAttached{Vertex,Fragment}Utilities` は per-program、`createShader()` 完遂後 clear で次 program 構築時に空状態から populate、cross-program 汚染なし。

### §7.9 generatePerProgramSPIRV() の utility prepend 順序保証

`mVulkanAttachedVertexUtilities` / `mVulkanAttachedFragmentUtilities` は std::vector、push_back 順 = attach 順 = `attachShaderFeatures()` 仕様順を保持。GLSL function definition の前方参照不可制約に対応するため、**utility 間で依存関係がある場合 (例 `globalF.glsl` が `lighting.glsl` の関数を呼ぶ)、attach 順が依存順と一致している必要**。`attachShaderFeatures()` 仕様は LL 標準で依存順を満たすように設計されている (前提)、本 B2-γ patch はこの仕様を踏襲。

### §7.10 cache miss 時の safe fallback

`generatePerProgramSPIRV()` 内で `source_cache->find(util_file) == source_cache->end()` の場合、`LL_WARNS("Vulkan")` でログ出力 + `continue` で該当 utility skip。**B2-γ verify ログ全体に cache miss WARN は 0 件** = 全 utility cache populate 成功確認済 (`AYAstorm.log` で `B2-γ utility cache miss` grep = 0 件)。

---

## §8 charter §3 #1 acceptance 担保

- GL path 不変: `LLShaderMgr::loadShaderFile()` 内 `shader_code_text[0] = "#version XXX\n"` strdup logic + GL compile path (`glCompileShader` 経由) 全不変、cache populate block は `out_sources == nullptr && LLVKLoader::isVulkanInitialized()` Vulkan-gated
- `LLGLSLShader::attachVertex/FragmentObject()` 内 `glAttachShader` 呼出順序不変、push_back は `stop_glerror()` 直後 + Vulkan-gated、glAttachShader 失敗時 (count==0) は push_back せず return false
- `LLGLSLShader::createShader()` 内 `mapAttributes`/`mapUniforms` 等の GL link 経路順序不変、`attachShaderFeatures()` call 位置不変、SPIR-V hook の reorder は Vulkan path のみ影響
- AYAstorm 独自改造意図保全: 全 shader file (Cinematic BD shadowUtil/screenSpaceReflUtil + Visual Realism volumetricLight/blurLightF/godraysF 系 7 file + Picker 2 + Exemplar 2 + A2 拡張 2 含む) touch 0 件
- bundle-A/A8-recovery/B1 既処理 UBO block byte-for-byte 維持 (touch 0 件)
- B2-α 既処理 varying/frag_out 行 byte-for-byte 維持 (touch 0 件)
- B2-β 既処理 vertex_in 行 byte-for-byte 維持 (touch 0 件)
- B3 既処理 SPIR-V Vulkan profile prepend logic 維持 (utility prepend は B3 範式の **直後** に hook、B3 logic untouched)
- 段階 1-4.3-γ'-port-β-2-bundle-B-B3 動作維持 (cold cache launch verify 起動成立 + clean shutdown + 実 crash 0 件、shader_cache 224 件 (-21) は §12 観測記録枠で記録、起動/shutdown 成立性は不変)

---

## §9 cross reference

### §9.1 commit / handoff doc 系譜

- `1cd9303872` (B2-γ patch、本 handoff の直接 parent)
- `44422cb14f` (B3-complete handoff doc commit + §12 metric 訂正、範式継承元)
- `71a8f87cde` (B3 patch、bundle-B 第三 sub-bundle ɣ 分支)
- `64f3229109` (B2-β-complete handoff doc)
- `eca091e1ce` (B2-β patch、bundle-B 第二 sub-bundle β 分支)
- `c0e1b25310` (B2-α-complete handoff doc)
- `bfa81e0283` (B2-α patch、bundle-B 第二 sub-bundle α 分支)
- `18187c862e` (B1-complete handoff)
- `5614494f56` (B1 patch、bundle-B 第一 sub-bundle)
- `8d2cae435b` (A7-complete handoff、bundle-A 全体完遂)
- `862f7dc8bd` (A7 patch、bundle-A 残 cleanup)
- `aed1438936` (A8-recovery、AYAstorm 改変 UBO 復活)

### §9.2 sub-doc / charter

- sub-doc 06 §1.2.2/§1.2.4/§3.1 sub-step 6.3
- sub-doc 07 §3.1 sub-step 7.2-7.4
- sub-doc 03 §3.1.3 (exemplar 2 役割)
- charter §3 #1 + §7.5
- project_ayastorm_r41_vulkan_migration.md (γ'-port-β-2-bundle-B-B3 完遂 → 本 commit で B2-γ 完遂 + 次 sub-bundle 着手境界 active)

### §9.3 memory references (feedback)

- feedback_proactive_handoff (本 doc 起草 = 周回境界での能動 handoff、context 残量 monitoring)
- feedback_self_verify_before_handoff (§5 self-verify 11 項目 all green、§4.14 metric 整合性 self-check)
- feedback_doubt_self_first (B3 §12 教訓を起草前に予防適用、structural touch consistency 確認)
- feedback_build_only_verified (literal pattern grep のみ採用、case-insensitive partial match 不採用)
- feedback_no_scope_shrink (literal scope = link failed + No function definition の **完全解消** + utility cluster 副次解消、cascade emergence は次 sub-bundle scope で literal 整合)
- feedback_admit_unknown (shader_cache -21 件は §12 観測記録枠で `untraced observation` として正直記録、commit 阻害判定は AYA 委ねた)
- feedback_falsification_as_progress (cascade exposure non-opaque +149 / parse +76 を progress signal として正確記録)
- feedback_explanation_lead_with_conclusion (metric 報告で結論ファースト = link -55 ✓ + No function def -206 ✓ + 副次 -73 + cascade +225 + net -109)
- feedback_no_claude_coauthor (commit message に Claude 共著行なし)
- feedback_no_auto_commit (AYA 「OK」明示指示下で commit)
- feedback_one_step_at_a_time (step 1 trace → step 2 prep → step 3 patch → step 4 self-verify → step 5 deploy → step 6 AYA verify → step 7 commit → step 8 handoff doc → step 9 memory 更新 を sequential 実行)
- feedback_remove_verification_logs (Verification 用 LL_INFOS hook 追加なし、LL_WARNS は cache miss 用 safe fallback diagnostic で出荷物として残置妥当)
- feedback_render_full_trace_first (推測禁止、log + code 両面 trace で根本原因 = `createShader` 内 SPIR-V hook が `attachShaderFeatures()` 完了前で utility vector 未充填状態 を確定後の patch)
- feedback_build (configure → build → install → cache clear 一括実行、本 sub-bundle で実施)

---

## §10 次 action = 次 sub-bundle 着手境界

### §10.1 次 sub-bundle 候補

| 候補 | scope | 推定 |
|---|---|---|
| **B?-δ** (non-opaque uniforms 解消) | `uniform float`/`uniform vec4`/`uniform mat4` 等の non-opaque uniform 宣言を `layout(set=N, binding=M) uniform UniformBlockName { ... }` 形式の uniform block 化 (または既存 UBO への統合)、`non-opaque uniforms outside a block` 223 件解消対象 | uniform block 範式新規確立、~50-100 shader file 大量 touch 想定、Agent 並列必須、A1-A7 UBO 範式と整合確認要 |
| **B?** (残 mixed cleanup) | 非 utility (program 本体側) の location/binding 残置 (B?-δ 完遂後の verify で再露出する可能性) + 残 syntax/semantic 残置 cleanup | mixed bag、scope 確定要 trace (B?-δ 完遂後)、~20-40 file |

選択は AYA 判断 (技術判断 = 効果直接観測しやすい順 / dependency 順)。**推奨順 = B?-δ** (non-opaque uniforms 223 件は本 commit で深部到達した最大 cascade、uniform block 範式確立で大半 program が parse 段階通過想定) → **B?** (mixed cleanup、scope 確定要)。B2-γ で `link failed` + `No function definition` cascade を一括解消したため、次は **non-opaque uniforms** が最大の残 cascade、解消で大半 program が compile/link 成立段階に到達想定。

### §10.2 次 sub-bundle 着手前に確認すべき事項

1. **uniform block 範式の source-of-truth** (既存 A1-A7 UBO block (例 `SkyUBO` / `WaterUBO` / `MaterialUBO`) と整合する set/binding 番号体系の確立、`LLShaderMgr::sUniformBlockSlot` 整合)
2. **non-opaque uniform の実 file 特定** (`grep -B5 "non-opaque uniforms outside a block" ~/.ayastorm_x64/logs/AYAstorm.log` で program 名 → file 特定、推定 ~50-100 shader file が touch 対象、Agent 並列必須)
3. **shader_cache -21 件の trace** (B3 baseline 245 vs B2-γ verify 224、`Shader compile success` / `Loaded shader` 系 INFO 行 diff で program 数差分特定、commit 阻害ではないが §10.3 fresh context 着手時に併せて確認)
4. **uniform block と既存 GL path (uniform を bare で参照する) の互換**: `layout(set,binding) uniform Block { ... }` は GL 4.2+ で `layout(std140) uniform Block { ... }` として GL 側でも valid、`#ifdef LL_VULKAN_GLSL` guard で Vulkan 専用化 vs GL 共通化の trade-off 判断要 (B1 MaterialUBO_Legacy 範式参照)

### §10.3 fresh context 引継 prompt 推奨

```
AYAstorm r41 Vulkan migration の sub-step 4.3-γ'-port-β-2-bundle-B-(B?-δ or B?) を着手します。

必読:
1. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B2-gamma-complete.md (本 handoff doc、parent 範式、Vulkan utility source cache / per-program attached utility tracking / utility concat hook + createShader reorder 範式 3 件)
2. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B3-complete.md (B3 範式継承元 + §12 metric 訂正教訓)
3. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B2-beta-complete.md (B2-β 範式継承元、Table C literal)
4. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B2-alpha-complete.md (B2-α 範式継承元、Table A/B literal)
5. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B1-complete.md (B1 範式継承元、MaterialUBO_Legacy)
6. (B?-δ 着手の場合) 既存 A1-A7 UBO block 範式 (SkyUBO/WaterUBO/MaterialUBO 等の set/binding 番号体系、LLShaderMgr::sUniformBlockSlot 整合) と uniform 行 → uniform block 化の literal transformation pattern
7. (B? 着手の場合) B?-δ 完遂後の再 verify で残 location/binding/syntax cluster の actual file 特定 trace

cadence (B1/B2-α/B2-β/B3/B2-γ 同):
1. trace (scope file 抽出 + canonical 設計 + 根本原因確定 = log + code 両面)
2. prep (Table / 範式 提示) → AYA OK
3. patch (Agent 並列 or single Edit、scope 規模に応じて判断) → AYA OK
4. self-verify (skip list / insertions-only or structural rewrite / pair integrity / canonical consistency + B3 §12 教訓 metric 整合性 self-check)
5. deploy (autobuild configure → build → install → cache clear、shader-only 変更なら installed dir cp + cache clear)
6. AYA cold cache launch verify
7. metric (該当 category 直接観測 + cascade 露出記録 + 構造的整合性 check + 3 段階 pair-grep 単調性確認)
8. commit (AYA OK 明示指示後)
9. handoff doc 起草 (本 doc 範式継承 + §4.14 metric 整合性 self-check 範式継承)

絶対 rule:
- B2-α/B2-β/B3/B2-γ 既処理 file (varying/frag_out/vertex_in 行 + Vulkan profile prepend logic + utility source cache + per-program attach tracking + concat hook + createShader reorder) untouched (本 step scope のみ touch)
- bundle-A/A8-recovery/B1 既処理 UBO block untouched
- skip list 13 file 機械的除外
- AYA 「commit して」or 「OK」明示指示前に commit 禁止
- one step at a time、1 メッセージ 1 アクション
- cold cache verify 必須 (rm -rf ~/.ayastorm_x64/cache/)
- 根本原因確定前に patch 着手禁止 (log + code 両面 trace、推測禁止 = feedback_render_full_trace_first)
- handoff 起草時に metric 整合性 self-check 必須 (literal pattern grep のみ、structural touch consistency、3 段階 pair-grep 単調性 = feedback_doubt_self_first / feedback_build_only_verified)
```

---

## §11 build artifact persist

| artifact | path | 内容 |
|---|---|---|
| 根本原因 trace log抜粋 | `~/.ayastorm_x64/logs/AYAstorm.old.b2-gamma-baseline` (B3 verify snapshot、commit `71a8f87cde` 再生成元) | `link failed for program` 55 件 + `No function definition (body) found` 206 件 (observed missing function = `getObjectSkinnedTransform` / `sampleReflectionProbesDebug` / `getPositionWithDepth` / `getDepth` 等) |
| code trace 結果 | `llglslshader.cpp:432-490` (`createShader()` 内 mShaderFiles loop + attachShaderFeatures() call + SPIR-V hook 旧位置) + `llglslshader.cpp:636-810` (`generatePerProgramSPIRV()` 全体) + `llshadermgr.cpp:743-1051` (`loadShaderFile()` 全体 + free 前 cache populate hook) | scope 確定根拠 |
| 範式の構造的整合性証明 | `link failed -55` + `No function definition -206` の 100% 解消 = utility 集合の `attachShaderFeatures()` 仕様順 prepend で cross-stage 関数 resolve 成立、B2-γ patch の構造的設計と実 metric が完全一致 | 仕組み実証 |

これら artifact は fresh context で本 doc + commit message `1cd9303872` + `llshadermgr.{h,cpp}` + `llglslshader.{h,cpp}` の git diff (`git show 1cd9303872 -- indra/llrender/`) から再生成可能 (persist 性 stable、コード trace 範式が source of truth)。

---

## §12 観測点 (post-hoc 記録、B2-γ commit 阻害なし)

### §12.1 shader_cache 再生成 -21 件 (B3 baseline 245 → B2-γ verify 224)

**観測**: `ls ~/.ayastorm_x64/cache/shader_cache/ | wc -l` で B3 baseline 245 件 vs B2-γ verify 224 件、**-21 件減少**。

**判定 (AYA 指示 2026-06-01)**: **commit 阻害ではない**、§12 観測記録枠で記録、次 sub-bundle 着手前 trace 推奨。

**起動/shutdown 成立性は不変**:
- 起動成立 + login 画面到達 ✓
- clean shutdown (Goodbye! 1 + Vulkan device destroyed 1 + Vulkan instance destroyed 1 + status: stopped) ✓
- 実 FATAL/SIGSEGV/crash 0 件 ✓
- charter §3 #1 acceptance 担保 (Vulkan path 内部改造のみ、GL path byte-for-byte 維持)

**仮説候補 (trace 未実施、§10.2 #3 で再着手)**:
- (a) B2-γ patch で `attachShaderFeatures()` 経由 utility attach が以前と異なる順序/タイミングで起きている可能性 (但し attach 順保持範式で論理的には不変のはず)
- (b) `loadShaderFile()` 内 utility cache populate 時の早期 return / failure path で utility shader compile を skip している可能性
- (c) feature level fallback / fallback shader load の差異 (B3 から B2-γ で間隔があり、AYA system の状態差)
- (d) GL_GEOMETRY_SHADER stage 等 cache populate 条件外の utility shader が以前と異なる扱いになっている可能性

**trace 手順案 (次 sub-bundle 着手時 §10.2 #3)**:
```bash
LOG_B3=~/.ayastorm_x64/logs/AYAstorm.old.b2-gamma-baseline
LOG_B2_G=~/.ayastorm_x64/logs/AYAstorm.log
diff \
  <(grep -E "Shader compile success|Loaded shader|Failed to compile" $LOG_B3 | sort -u) \
  <(grep -E "Shader compile success|Loaded shader|Failed to compile" $LOG_B2_G | sort -u) \
  | head -100
```

**B2-γ commit 維持判定根拠**:
1. `link failed for program` 55→0 / `No function definition` 206→0 の **B2-γ 直接効果は完全達成**、構造的整合性証明済
2. `non-opaque uniforms` +149 / `parse failed for stage` +76 の cascade emergence は **想定内 progress signal** (utility prepend 成功 → 深部到達 → 新 first-error)
3. shader_cache -21 件は **起動/shutdown/crash 0 件で致命性なし**、原因 trace は次 sub-bundle 着手前で可能 (本 commit 阻害なし、AYA 判断 2026-06-01)

### §12.2 metric 整合性 self-check 範式 (B3 §12 教訓継承)

B3 §12 教訓 (case-insensitive partial match `link` で false positive、handoff 起草時 metric 主張が実 log と整合しなかった) を **本 B2-γ-complete 起草前に予防適用**:

1. **literal pattern grep のみ採用** (`link failed for program` / `No function definition` / `non-opaque uniforms outside a block` / `0:N: 'location'` / `0:N: 'binding'` / `missing #endif` / `parse failed for stage` / `must occur first|bad profile name|bad tokens following profile`)
2. **structural touch consistency** = B2-γ patch が touch する hook (loadShaderFile cache populate + attachVertex/Fragment push_back + generatePerProgramSPIRV utility prepend + createShader reorder) と、実 metric delta が示す cluster (link failed -55 / No function def -206 / utility 由来 location/binding/#endif -73 / non-opaque cascade +149) との **構造的因果整合** を確認
3. **3 段階 pair-grep** = B2-β baseline (`64f3229109` 時点) → B3 baseline (`71a8f87cde` 時点) → B2-γ verify (`1cd9303872` 時点) で各 cluster の delta が単調 (B3 で -36 #version + ±0 他、B2-γ で -261 link/No function def + -73 location/binding/#endif + +225 non-opaque/parse) であることを確認、ジグザグ regression なし
4. **handoff §4 metric 表は実 grep 結果のみ記載、推測値・期待値を「実測」と混同しない**

本 §12.2 範式は B2-γ-complete handoff から fresh context への明示参照で継承、次 sub-bundle 起草時に同様適用。

---

**EOF**
