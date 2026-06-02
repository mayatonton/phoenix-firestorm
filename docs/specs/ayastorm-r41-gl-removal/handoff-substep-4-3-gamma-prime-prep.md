# r41 sub-step 4.3-γ' 着手前 prep + case ② 採用境界 (2026-05-31)

**status**: **draft 2026-05-31 (案 A target extension revert 後、case ② = runtime SPIR-V 生成 採用下起草、AYA 「A でお願いします」承認 = 今 session で hook trace + handoff doc 仕上げ + 実装本体は次 session)**
**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-beta-prime-complete.md` (sub-step 4.3-β' = LLVertexBuffer Vulkan 化 placement only 完遂、`78820a6edf`、active 継続)
**親 charter**: `docs/specs/ayastorm-r41-gl-removal/00-charter.md` (§2 領域 6 + §7.5 boundary refine 可)
**親 sub-doc**: `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` (§3.1 sub-step 6.1 一括化 spec、本 prep で runtime path 化に再定義提案)
**起草根拠**: sub-step 4.3-γ' (sub-doc 06 §3.1 sub-step 6.1 shader port) 着手で **当初 案 A target extension (CMake glob + glslangValidator 一括 pre-compile) を採用したが、build verify で spec 想定相違 4 点露呈** → AYA 「完全に動作させる必要があるので」基準で case ② = runtime SPIR-V 生成 採用 → AYA 「A でお願いします」承認下で本 prep doc 起草 + handoff 範式継承

---

## 1. 起草目的 + case ② 採用根拠

### 1.1 起草目的

sub-step 4.3-β' 完遂 (commit `78820a6edf` = LLVertexBuffer Vulkan 化 placement only 4 file +240 line、AYA launch verify PASS、regression 0) 後、sub-step 4.3-γ' (sub-doc 06 §3.1 sub-step 6.1 shader port) 着手で:

1. **γ'-1 trace**: AYAstorm 改変 shader file 11 件確定 (charter §3 #4 spec drift = 13 件 → 実 11 件)
2. **γ'-2 判断**: 4 案 (A target extension / B exemplar parallel / C per-pool / D 全件 manual) で **案 A target extension 採用**、AyaShaderCompile.cmake §16 「6.1 一括化で 248 file 拡張予定」spec literal 整合
3. **γ'-port-α (案 A 実装)**: `newview/CMakeLists.txt:2208-2245` で *V.glsl/*F.glsl glob + AYAstorm 改変 8 file REMOVE_ITEM + foreach SPIR-V compile 配線
4. **autobuild verify**: build fail、`aya_r41_shaders` target で 13 file 試行中 `.spv` 生成 0 件、9 件 `Missing entry point` error

露呈した spec 想定相違 (avatarV.glsl 例示):

| # | 観測 glslang error | 内容 | sub-doc 06 §3 分類 |
|---|---|---|---|
| 1 | `ES shaders for SPIR-V require version 310+` | `#version` directive 無し (LLShaderMgr runtime prepend 前提) | **未分類** (A: 素通り / B: 要修正 4 type いずれにも無し) |
| 2 | `non-opaque uniforms outside a block` | 自由 `uniform mat4 X;` (Vulkan は UBO 必須) | **未分類** |
| 3 | `SPIR-V requires location for user input/output` | `in vec3 position;` (location qualifier 無し) | **未分類** |
| 4 | `Missing entry point` 9 件 | forward decl `getSkinnedTransform()` 等が別 .glsl への concat 前提 | **未分類** |

**Root cause**: raw .glsl は **fragment**、LLShaderMgr.cpp が runtime に preprocessing (concat + `#version` prepend + `#define` 注入) を行ってから GL driver に渡している。3.3-B exemplar (`aya_r41_exemplar/sky_placeholder{V,F}.glsl`) は purpose-built Vulkan-ready GLSL だったので問題露呈せず、6.1 一括化で初めて構造矛盾が surface。

本 prep doc は **案 A revert + case ② 採用境界 + 次 session 着手 plan** を固める。

### 1.2 4 案検討 (AYA 提示「完全動作」基準下)

| case | 完全動作 | 致命点 | 採否 |
|---|---|---|---|
| **① build-time preprocessor 自作** (CMake script で LLShaderMgr-equivalent 再実装) | ❌ | variant 全件 expand → 数千 `.spv` 同梱 = 実用不可、LLShaderMgr logic を CMake script に 2 重実装 = drift bug 温床 | reject |
| **② runtime SPIR-V 生成** (LLShaderMgr に Vulkan path 追加 → glslang library runtime API → SPIR-V binary → vkCreateShaderModule) | **✅** | **無し** (起動 compile time 増のみ、cache layer で吸収可) | **採用 (AYA 2026-05-31 承認)** |
| **③ per-shader manual rewrite** (228 file 全部 Vulkan-compatible GLSL に書換) | ❌ | LL/FS shader variant model 全壊 (同一 .glsl から数百-数千 variant を runtime feature flag で compile する設計)、228 file → 数千 variant に分解する破壊的書換が必要 | reject |
| **④ 6.1 defer** (LLShaderMgr Vk path 確立後に再着手) | ⏸ | ②/① の解確定後でなければ意味無い | postpone (case ② = ④ の前段) |

### 1.3 case ② 採用 決定根拠

1. **LL/FS shader variant 爆発 model**: LLShaderMgr は `#define HAS_NORMAL_MAP` / `#define WATER_FOG` / `#define HAS_SKIN` 等 数十 feature flag で **runtime に数百〜数千 variant を compile** する設計。1 .glsl × 1 .spv の static 関係は構造上成立しない。
2. **LLShaderMgr 既存 preprocessing path 1 source of truth**: Vulkan path 追加 = 既存 preprocessing 完全再利用、glslang library に runtime string 渡し → SPIR-V binary in-memory → `vkCreateShaderModule()`
3. **段階 4.3 paradigm 1:1 継承**: active camera + frame context paradigm と同様、runtime 経由で動的 state を SPIR-V に反映
4. **3 OS 整合**: glslang は LunarG SDK 同梱 (3 OS 全部)、build-time vs runtime の差異なし
5. **3.3-B exemplar 役割保存**: exemplar は「runtime glslang call の vkCreateShaderModule sink」として保持、build-time `.spv` 生成 = 試作レール (production path は runtime API)

---

## 2. 案 A revert 経緯 + 現 build state

### 2.1 案 A 採用判断 (γ'-2 段階)

- AyaShaderCompile.cmake:16 spec literal 「6.1 一括化で 248 file 拡張予定」を素直に解釈
- *V.glsl/*F.glsl suffix glob + REMOVE_ITEM で AYAstorm 改変 11 file 除外、foreach SPIR-V compile
- exemplar (`aya_r41_exemplar/sky_placeholder{V,F}`) は purpose-built Vulkan-ready GLSL = 案 A で 1 pass compile 成功した経験

### 2.2 build verify 露呈 (γ'-port-α 段階)

- `autobuild build -A 64 -c ReleaseFS_open --no-configure` 実行
- 13 file 試行で `.spv` 生成 0 件、9 件 `Missing entry point` error
- `make: *** [Makefile:91: all] エラー 2` = top-level build 全壊 (aya_r41_shaders target fail が cascade)

### 2.3 revert 実施

```
git checkout indra/newview/CMakeLists.txt indra/cmake/AyaShaderCompile.cmake
```

- 2 file revert clean、build state は 4.3-β' 完遂時 (`78820a6edf`) と一致
- AYAstorm 動作維持確認 (revert 後 git status 清浄、staged 改変 0 件)

### 2.4 task list re-shape

| task # | 旧 | 新 | 経緯 |
|---|---|---|---|
| #2 γ'-2 | 案 A 採用判断 (完了) | **deleted** | 案 A revert で前提消失 |
| #3 γ'-3 | per-file SPIR-V port 順序確定 (完了) | **deleted** | runtime path で順序概念無し |
| #4 γ'-4 | incremental SPIR-V port 実施 (in_progress) | **γ'-port-α (case ②) = LLShaderMgr Vulkan path 配線 + glslang runtime API + exemplar 1 file PoC** | case ② 再構成 |
| #10 (新) | — | γ'-port-α-prep: LLShaderMgr Vulkan path hook 候補 trace (Agent 経由) | 新規、本 prep doc 起点 |
| #11 (新) | — | spec 改訂: sub-doc 06 §3 (一括化 → runtime path 化) + sub-doc 03 §3.1.3 (exemplar 役割注記) | 新規 |
| #12 (新) | — | γ'-port-β (case ②): exemplar runtime path → 全 shader variant 一般化 | 新規 |
| #13 (新) | — | handoff doc 起草: 本 prep doc | 新規、本 doc 自身 |

---

## 3. LLShaderMgr Vulkan path hook trace 結果 (Explore agent γ'-port-α-prep)

### 3.1 LLShaderMgr 既存 GL preprocessing path

| 処理段階 | 関数 | file:line | 役割 |
|---|---|---|---|
| .glsl ファイル読込 | `loadShaderFile()` | `indra/llrender/llshadermgr.cpp:478-587` | `LLFile::fopen()` で GPU class 別 fallback、1024 char × max 4096 line buffer |
| `#version` prepend | `loadShaderFile()` | `llshadermgr.cpp:612-666` | `gGLManager.mGLSL{Version}` 判定、`strdup("#version XXX\n")` → `shader_code_text[shader_code_count++]` |
| `#define` 注入 | `loadShaderFile()` | `llshadermgr.cpp:668-701` | `"#define FRAGMENT_SHADER 1"` / `"#define VERTEX_SHADER 1"` + `*defines` map 反復 → `extra_code_text[]` accumulate |
| multiple .glsl 連結 | `attachShaderFeatures()` (caller) | `llshadermgr.cpp:73-250` | `shader->attachVertexObject("lighting/lightFuncV.glsl")` 等で feature file push、`shader_code_text[]` 逐次 append |
| 加工済み GLSL → GL submit | `loadShaderFile()` | `llshadermgr.cpp:926, 941` | `glShaderSource(ret, shader_code_count, (const GLchar**)shader_code_text, NULL)` → `glCompileShader(ret)` |

**Key observation**: preprocessing 完了後の加工済み GLSL string は `const GLchar**shader_code_text[]` (pointer to pointers, count = `shader_code_count`) として `glShaderSource()` に渡される。**この time point 直前 (line 908 = `glCreateShader()` 直前)** が Vulkan path への hook point。

### 3.2 LLGLSLShader shader variant model

| variant 機構 | 関数 | file:line | 説明 |
|---|---|---|---|
| feature flag → #define mapping | `createShader()` loop | `indra/llrender/llglslshader.cpp:455-468` | `for (auto& pair : mShaderFiles)` → `loadShaderFile(..., &mDefines, ...)` で `mDefines` (std::map<string, string>) を渡す |
| `mDefines` 構築 | `addPermutation()` + ctor | `llglslshader.h:252-256`, `llglslshader.cpp` 初期化 | `mDefines.insert(define_pair)` で feature flag → #define key-value mapping |
| variant key (hash) 生成 | `hash()` | `llglslshader.cpp:2055-2083` | HBXXH128 hash over `mName + mShaderGroup + mShaderLevel + mShaderFiles + mDefines + sGlobalDefines + mFeatures + GL vendor/renderer/version` |
| variant hash 用途 | `createShader()` | `llglslshader.cpp:432` | `mShaderHash = hash()` → binary cache lookup key (`mShaderBinaryCache[mShaderHash]`、`llshadermgr.h:482`) |
| GLTF variant (16-bit) | `bind(U8 variant)` | `llglslshader.h:351-364` | alpha_blend(bit 0) + rigged(bit 1) + unlit(bit 2) + multi_uv(bit 3) の 4-bit flag = 16 variants (`mGLTFVariants[16]`) |

**Key observation**: variant は caller が feature flag を `addPermutation()` で inject して `mDefines` を mutate → `createShader()` call で compile する仕組み (build-time variant、runtime define injection)。variant key は `mShaderHash` (HBXXH128 LLUUID) で統一。**case ② cache key は `mShaderHash` 1:1 流用可**。

### 3.3 Vulkan path hook point 候補 (推奨 = ③)

| 候補 | Location | file:line | 渡可能情報 | 判定 |
|---|---|---|---|---|
| **①** | `loadShaderFile()` post-glShaderSource | `llshadermgr.cpp:926 post` | 加工済み GLSL string array, filename, type, `*defines` | Low difficulty |
| **②** | `LLGLSLShader::createShader()` loadShaderFile call site | `llglslshader.cpp:458` | mDefines, mShaderFiles, mShaderLevel, mName | Medium (timing 齟齬) |
| **③ (推奨)** | `loadShaderFile()` pre-glCreateShader | `llshadermgr.cpp:908 前` | 加工済み GLSL string array, filename, type | **Low** (preprocessing 直後、glslang 呼出前、clean branch) |
| ④ | `createShader()` post-attachShaderFeatures | `llglslshader.cpp:476` | mDefines (確定), mShaderFiles (確定), mName | Medium-High (PSO 単位整合難) |
| ⑤ | `createShader()` mShaderHash 計算後 | `llglslshader.cpp:432` | mShaderHash (確定), variant 確定 | Low-Medium |

**詳細推奨 = ③** (`llshadermgr.cpp:908` pre-glCreateShader):
- Line 908 直前に conditional branch: `if (gVK.isEnabled()) { spirvCompileResult = glslangCompileToSpirv(...); if (spirvCompileResult) return setupVulkanShaderModule(...); }`
- Line 908-950 の GL path (`glCreateShader` → `glShaderSource` → `glCompileShader`) と 1:1 mapping
- `mUsingBinaryProgram` 機構 (line 446) と parallel して Vulkan shader module cache 統合可
- GL fallback 自然 (Vulkan compile fail → return 0 → 既存 GL path 再トライ in `llshadermgr.cpp:1000`)

### 3.4 glslang library runtime API 要件

**Headers** (Vulkan SDK 同梱):
- `<glslang/Public/ShaderLang.h>` — main API (`glslang::InitializeProcess`, `CompileShaderModule`)
- `<glslang/SPIRV/GlslangToSpv.h>` — GLSL → SPIR-V compiler
- `<glslang/SPIRV/SpvTools.h>` — optional (SPIR-V validation)

**Link libraries**:
- `libglslang.so` / `libglslang.a`
- `libSPIRV.so` / `libSPIRV.a` (SPIR-V code generator)
- `libOSDependent.so` (platform utilities, usually static link)

**既存配信物に未同梱** (`build-linux-x86_64/packages/` で `*glslang*` 0 件) → **`autobuild.xml` に新 dependency 追加必要** (3 OS):
- Linux: `libglslang-dev` (`apt install libglslang-dev`)
- Mac: `brew install glslang`
- Windows: Vulkan SDK install 要件化 (AyaShaderCompile.cmake と統一)

### 3.5 SPIR-V cache layer 配置 (推奨 = 案 C)

| 案 | directory | cache key | 備考 |
|---|---|---|---|
| A | `~/.ayastorm_x64/cache/spirv_cache/` | `<mShaderHash>_<stage>.spv` | GL binary と並列、stage 別分離 |
| B | `~/.ayastorm_x64/cache/shader_cache/spirv/` | `<mShaderHash>_<type>.spv` | GL binary と同一 dir 内、type = vert/frag |
| **C (推奨)** | `~/.ayastorm_x64/cache/shader_cache/` (既存) | `<mShaderHash>_vert.spv`, `<mShaderHash>_frag.spv` | GL binary と metadata 統一、`shaderdata.llsd` に SPIR-V entry 追加 |

**推奨理由 (C)**:
- 既存 metadata (`shaderdata.llsd`, `mShaderBinaryCache`) 統一化でファイル管理簡潔
- GL binary + SPIR-V binary を同じ `mShaderHash` variant key で共存
- 起動時 cache hit → SPIR-V binary load → vkCreateShaderModule 高速 path

**cache key 構成**: `mShaderHash` (HBXXH128 LLUUID、`llglslshader.cpp:2055-2083` 既存) + stage suffix (`_vert` / `_frag`)、filename = `<LLUUID>_vert.spv` / `<LLUUID>_frag.spv`

### 3.6 実装規模見積もり (case ② 全体)

| 項目 | 見積もり | 備考 |
|---|---|---|
| **Core Vulkan path LOC** | 150-200 LOC | glslang runtime API wrap + `vkCreateShaderModule` call + error handling |
| **cache layer integration** | 100-150 LOC | SPIR-V binary save/load、`mShaderBinaryCache` parallel 拡張 (LLUUID→SPIRV map) |
| **fallback logic** | 50-80 LOC | Vulkan path 失敗 → GL fallback (既存 `return 0` path reuse) |
| **autobuild.xml glslang dependency** | +30-50 lines | 3 OS package 配線 |
| **cmake link 設定** | +20-40 LOC | `find_package(glslang)` or pkg-config |
| **Total** | **550-740 LOC** | `llshadermgr.cpp` +250-350 / `llvkloader.cpp` +150-200 / 他 |

**副作用 risk**:
- Low: preprocessing logic 不変 (shader_code_text[] 構築継続)
- Low: GL path 完全並行維持 (conditional branch のみ)
- Medium: glslang library 初回 `glslang::InitializeProcess()` 1-time cost ~10ms、per-shader compile 5-20ms

**GL fallback 可否**: 全面可 (Vulkan compile fail → `return 0` → GL path 再トライ、既存 recursive retry in `llshadermgr.cpp:1000`)

### 3.7 関連 file 一覧 (case ② 実装で touch する file)

| File | Role | Est. LOC |
|---|---|---|
| `indra/llrender/llshadermgr.h` | glslang runtime API 前置宣言 + Vulkan shader module map 宣言 | +10-20 |
| `indra/llrender/llshadermgr.cpp` | Vulkan path hook point 実装 (glslang compile + vkCreateShaderModule + cache) | +250-350 |
| `indra/llrender/llvkloader.h` | Vulkan shader module lifecycle helper export (loadSpirvFromBinary etc.) | +20-40 |
| `indra/llrender/llvkloader.cpp` | glslang runtime wrapper、SPIR-V binary cache read/write helper | +150-200 |
| `indra/cmake/AyaShaderCompile.cmake` | parking (build-time SPIR-V pre-compile、case ② では runtime-only 故 update 不要、3.3-B exemplar 専用) | 0 |
| `indra/cmake/CMakeLists.txt` (llrender) | glslang library link 設定 (find_package or pkg-config) | +20-40 |
| `autobuild.xml` | glslang-dev dependency 追加 (Linux/Mac、Windows は Vulkan SDK assumption) | +30-50 lines |
| `indra/newview/app_settings/shaders/aya_r41_exemplar/sky_placeholder{V,F}.glsl` | 既存 exemplar (sub-step 3.3-B reference) — no change | 0 |

---

## 4. spec 改訂 plan (sub-doc 06 §3 + sub-doc 03 §3.1.3)

### 4.1 sub-doc 06 §3 改訂方針

| spec 項目 | 旧 (build-time pre-compile 前提) | 新 (runtime path 前提) |
|---|---|---|
| §3.1 sub-step 6.1 | autobuild integration 一括化 (CMake + glslangValidator) | **LLShaderMgr Vulkan path 配線 + glslang library runtime API + SPIR-V cache layer** |
| §3 表 A: 195 file 素通り | `--target-env vulkan1.3` で 1 pass compile 成功、修正不要 | **LLShaderMgr 加工済み GLSL を glslang runtime API に渡して SPIR-V 生成成功、修正不要** |
| §3 表 B: 53 file 要修正 | issue type 4 bundle (built-in 座標 / legacy attribute / extension / precision) | **LLShaderMgr preprocessing 通過後 SPIR-V 生成失敗、issue type 別 bundle で修正** |
| sub-step 6.2 | A 195 file 一括 build script 処理 | **A 195 file LLShaderMgr Vulkan path 通過 verify (起動時 cache miss → compile 成功)** |
| sub-step 6.3 | B 53 file issue type 別 bundle 修正 | **B 53 file 修正後 LLShaderMgr Vulkan path 通過 verify** |
| sub-step 6.4 | descriptor binding 統合 (領域 7 同期) | **同左 (variant key + cache layer 整合下で実施)** |
| sub-step 6.5 | self-check + AYAstorm 13 file untouched verify | **same** |

### 4.2 sub-doc 03 §3.1.3 改訂方針

- 3.3-B exemplar (sky_placeholder) = **「runtime glslang call の vkCreateShaderModule sink 試作レール」** に役割再定義
- build-time `.spv` 生成 (`AyaShaderCompile.cmake`) = **試作レール扱い、production path は LLShaderMgr Vulkan path 経由 runtime SPIR-V 生成**
- `loadSpirvShaderModuleFromFile()` (llvkloader.cpp:1761-1770) = **試作レール sink、production sink は `loadSpirvShaderModuleFromMemory()` (新規、binary blob 受領)**

### 4.3 charter §7.5 boundary refine 範囲

- 段階 5 LLVertexBuffer Vk 化 4.3-β' 前出し (確定済) + **領域 6 sub-step 6.1 case ② 採用** (本 prep doc 提案、AYA 承認境界再設定)
- charter §3 #4 spec drift (AYAstorm 13 file → 実 11 file) は次 session η' phase で update

### 4.4 AYA 承認境界再設定 (追加項目)

| update 必要事項 | 旧 | 新 |
|---|---|---|
| sub-step 6.1 着手 model | autobuild integration 一括化 | **LLShaderMgr Vulkan path 配線 + runtime SPIR-V 生成** |
| glslang library dependency | 未明 | **autobuild.xml に新 dependency 追加 (3 OS、Linux/Mac/Windows それぞれ install 手順 update)** |
| cache layer 範式 | 未明 | **`~/.ayastorm_x64/cache/shader_cache/` 既存 dir 内に SPIR-V binary 並存、`shaderdata.llsd` metadata 統一** |
| build-time pre-compile (AyaShaderCompile.cmake) 役割 | 6.1 一括化前提 | **3.3-B exemplar 専用 試作レール、parking** |

---

## 5. γ'-port-α/β/γ 新 cadence (次 session 実装範囲)

### 5.1 新 cadence table (案 A 廃止後 = case ② 採用下)

| sub-step | scope | acceptance |
|---|---|---|
| **γ'-port-α** (次 session) | LLShaderMgr Vulkan path 配線 + glslang library link (3 OS) + exemplar 1 file (sky_placeholder) runtime SPIR-V PoC | exemplar shader が LLShaderMgr Vulkan path 経由で SPIR-V 生成成功 + `vkCreateShaderModule()` で VkShaderModule 取得成功 + cache write/read 1 回成功 + AYA launch verify PASS |
| **γ'-port-β** (次々 session 範囲) | exemplar runtime path → 全 LLShaderMgr 経由 shader variant 一般化 + cache hit/miss 動作確認 + 起動時間 measurement | 全 variant が Vulkan path 通過、cache hit 起動加速確認、AYAstorm 改変 11 file untouched 維持、AYA launch verify PASS |
| **γ'-port-γ** (次々々 session 範囲) | untouched verify + descriptor binding 整合 (領域 7 §3 sub-step 7.2-7.4 同期) + AYA launch verify + commit | AYAstorm 13 file untouched (実 11 file)、descriptor set=0/1/2 + Vulkan shader module 整合、4.3-γ' 完遂境界到達 |

### 5.2 γ'-port-α (次 session 着手 task 候補)

| task 候補 # | scope | 備考 |
|---|---|---|
| γ'-port-α-1 | `autobuild.xml` に glslang-dev / Vulkan SDK dependency 追加 (3 OS) | Linux apt-pkg、Mac brew、Win Vulkan SDK |
| γ'-port-α-2 | `indra/cmake/00-Common.cmake` or `indra/cmake/LLRender.cmake` で glslang library link 配線 (find_package) | configure pass |
| γ'-port-α-3 | `indra/llrender/llshadermgr.h` に Vulkan shader module map field + `createSPIRVFromGLSL()` private helper 宣言追加 | header propagate 最小化 |
| γ'-port-α-4 | `indra/llrender/llshadermgr.cpp:908` 直前に hook point branch 配線 (`if (gVK.isEnabled()) { spirvCompileResult = ... }`) + glslang runtime API wrap 実装 | core path |
| γ'-port-α-5 | `indra/llrender/llvkloader.cpp` に `loadSpirvShaderModuleFromMemory(const std::vector<uint32_t>&)` 追加 (existing `loadSpirvShaderModuleFromFile()` 範式継承) | binary blob sink |
| γ'-port-α-6 | SPIR-V cache layer 配置 (`shader_cache/<mShaderHash>_vert.spv` / `_frag.spv` 書込み + 起動時読込) | metadata は `shaderdata.llsd` 拡張 |
| γ'-port-α-7 | exemplar 1 file (`sky_placeholderV.glsl` / `F.glsl`) を LLShaderMgr 経由で読込、Vulkan path で SPIR-V 生成 + vkCreateShaderModule 動作確認 | PoC validation |
| γ'-port-α-8 | incremental autobuild + cache clear + AYA launch verify (案 B cadence = measurement log skip) | regression 0 + cache 1 file write 確認 |
| γ'-port-α-9 | commit (γ'-port-α scope 限定、γ'-port-β は別 commit) | feedback_no_auto_commit 遵守 |

### 5.3 critical reminders (γ'-port-α 着手 6 件)

1. **AYAstorm 改変 11 file untouched 維持** (r42-α picker 2 / r42-β Cinematic 2 / r42-γ visual realism 7) — γ'-1 trace 確定値、charter §3 #4 spec drift (13 → 11) は η' phase update 範囲
2. **段階 1-4.3-β' 動作維持** = LLShaderMgr GL path は完全並行維持、Vulkan path は `gVK.isEnabled()` 条件下のみ branch
3. **案 B cadence 継承** = measurement log 配線 skip + code review + 起動/shutdown clean + regression 0 で satisfy
4. **glslang library 3 OS 配信** = autobuild.xml 追加で Linux/Mac/Windows 全部 install path 確立、配信物に同梱
5. **cache layer 範式 = 案 C 採用** (`shader_cache/` 内 SPIR-V binary 並存、`mShaderHash` cache key 統一)
6. **proactive handoff 範式** = γ'-port-α 完遂後即 handoff doc 起草 (handoff-substep-4-3-gamma-prime-port-alpha-complete.md)、γ'-port-β/γ は別 session

### 5.4 commit 戦略

- γ'-port-α 完遂で 1 commit 想定 (案 B cadence 継承)
- 段階分割可能性高 (glslang link 配線 + LLShaderMgr Vulkan path + cache layer + exemplar PoC = 4 phase) → AYA 確認下で分割可能
- sub-step 3.3-A 範式継承 (commit hash table を handoff doc に明記)

---

## 6. risks/caveats (8 件)

### 6.1 case ② 起動時間 increase risk

- 全 shader variant を runtime compile = 起動時 compile 待ち時間増加 (初回 cache miss 状態)
- glslang `glslang::InitializeProcess()` 1-time cost ~10ms、per-shader compile 5-20ms
- variant 数 = LL/FS で数百-数千 推定 → 初回起動で 10-60s 推定追加
- cache hit 状態 (2 回目以降) = SPIR-V binary load のみで GL binary cache 同等速度復帰

### 6.2 glslang library 3 OS 配信負荷

- autobuild.xml に new dependency 追加 = 3 OS 全部 install path 確立必要
- Linux: `libglslang-dev` package、Mac: `brew install glslang`、Windows: Vulkan SDK install 要件化
- viewer 配信物に bundled SO/dylib/DLL 同梱 (size +5-10MB 推定)

### 6.3 LLShaderMgr Vulkan path 配線で GL path 破壊 risk

- 既存 `loadShaderFile()` (line 478-1000) は LL 上流コード = pristine 維持必須 (charter §3 acceptance #1)
- Vulkan path 追加は conditional branch のみ (`if (gVK.isEnabled())`)、GL path 既存 logic 完全並行維持
- AYAstorm 改変 file untouched + LL 上流 file 拡張のみ (branch 追加) で acceptance #1/#4 同時 satisfy

### 6.4 SPIR-V cache layer metadata 統一化 risk

- 既存 `shaderdata.llsd` (GL binary metadata) に SPIR-V entry 追加 = LLSD schema 拡張
- 旧 version cache 互換性 = cache miss で fallback compile (起動時間 1 回 increase のみ、機能影響無し)
- 提案: `shaderdata.llsd` 内 entry に `format` field 追加 (`GL_BINARY` / `SPIRV_VERT` / `SPIRV_FRAG`)

### 6.5 variant 爆発下 cache size

- LL/FS shader variant 数 = 数百-数千、each SPIR-V binary ~1-10KB
- cache 総 size = 数 MB-数十 MB 推定 (現状 GL binary cache と同等規模)
- 起動時 directory scan + LLSD parse のコスト = 既存 GL binary cache 範式 1:1 流用で吸収可

### 6.6 領域 7 sub-step 7.2-7.4 並走着手判断

- sub-step 6.4 (descriptor binding 統合) は領域 7 sub-step 7.2-7.4 と同期必須 (sub-doc 06 §3.1)
- 4.3-γ' (= sub-step 6.1) では descriptor 配線は scope 外、γ'-port-γ で領域 7 同期判断
- AYA 承認境界 update が必要 (memory `project_ayastorm_r41_vulkan_migration` §sub-doc 07)

### 6.7 context budget concern

- case ② 実装 (550-740 LOC × 6-7 file) は段階 3.3-B / 段階 4.3-β' と同等規模
- 次 session fresh context 推奨 (今 session で hook trace + handoff doc 仕上げ + 実装本体は次 session boundary)
- AYA 「A でお願いします」承認下 = (A) plan 採用 (今 session 範囲 = #10 trace + #13 handoff doc のみ)

### 6.8 case-validity 担保 (sub-doc 06 §3 spec 全面 re-author 必要性)

- sub-doc 06 §3 spec は build-time pre-compile 前提で書かれている = case ② 採用で **全面 re-author 必要**
- §11 (本 prep doc 4.1) で改訂方針提示済、次 session で sub-doc 06 §3 + sub-doc 03 §3.1.3 re-author を γ'-port-α-prep として実施
- 旧 spec の A 195 / B 53 分類は妥当性継承 (LLShaderMgr 加工済み GLSL → glslang runtime で同分類が成立する見込み)

---

## 7. next session entry point

### 7.1 着手前 4 段 cadence (sub-step 4.3-α 範式継承)

1. **memory load**: `project_ayastorm_r41_vulkan_migration.md` (active sub-step state 確認)
2. **handoff doc load**: 本 prep doc + `handoff-substep-4-3-beta-prime-complete.md` (4.3-β' 完遂境界)
3. **spec load**: `sub-doc 06` (§3.1 sub-step 6.1 spec) + `sub-doc 03` (§3.1.3 exemplar spec) + `00-charter.md` (§2 領域 6 + §7.5 boundary refine 範囲)
4. **task list reset**: 既存 task #4 (γ'-port-α) を in_progress 化、§5.2 候補 task 9 件分を必要に応じ TaskCreate

### 7.2 γ'-port-α-prep (次 session 開始時)

- sub-doc 06 §3 + sub-doc 03 §3.1.3 spec 改訂 (本 prep doc §4 方針反映)
- AYA 承認境界再設定確認 (本 prep doc §4.4 + AYA review)

### 7.3 γ'-port-α 実装着手 (sub-doc 06 §3 update 後)

- §5.2 候補 task 9 件分を順次実施
- exemplar 1 file PoC で動作確認後 commit (案 B cadence 継承)

---

## 8. 関連 doc/memory cross reference

### 8.1 関連 doc

| doc | 関係 |
|---|---|
| `00-charter.md` | §2 領域 6 sub-step 6.1 (4.96 PM、248 GLSL shader SPIR-V 化) + §3 acceptance #4 (AYAstorm 13 file untouched、実 11 file drift) + §7.5 boundary refine (本 prep で case ② 採用) |
| `03-state-machine-pso.md` | §3.1.3 sub-step 3.3-B (1 shader exemplar pre-flight) → 本 prep で **試作レール役割再定義** |
| `06-shader-spirv.md` | §3.1 sub-step 6.1 (本 prep で **案 A 廃止 + case ② = runtime path 採用に再定義**) |
| `07-descriptor-renderpass.md` | §3 descriptor set 3 階層 + sub-step 7.2-7.4 (γ'-port-γ で同期判断) |
| `handoff-substep-4-3-beta-prime-complete.md` | 4.3-β' 完遂境界 (`78820a6edf`)、本 prep の前提 |
| `handoff-substep-4-3-alpha-complete.md` | 4.3-α 完遂境界 (`9f13302078`)、4 段 cadence 範式継承元 |
| `handoff-substep-3-3-B-beta-2-complete.md` | 3.3-B exemplar build chain 確立 (本 prep §3.1 trace 起点) |

### 8.2 関連 memory

| memory | 関係 |
|---|---|
| `project_ayastorm_r41_vulkan_migration.md` | active sub-step state + AYA 承認境界 (sub-step 6.1 並走/独立判断) |
| `feedback_proactive_handoff.md` | proactive handoff 範式 = γ'-port-α 完遂後即 handoff doc 起草 |
| `feedback_self_verify_before_handoff.md` | self-check 範式 = AYA 確認依頼前 全 path 整合確認 |
| `feedback_use_agents_proactively.md` | hook trace に Explore agent 活用 (本 prep §3 起点) |
| `feedback_no_scope_shrink.md` | case ② 採用は scope shrink ではなく structural pivot (案 A 採用が wrong assumption 前提) |
| `feedback_self_bug_no_defer_option.md` | case ④ (defer) は ②/① の解確定後でなければ意味無い、defer 単独 reject |
| `feedback_admit_unknown.md` | build verify 露呈 spec 想定相違を即 surface、捻り出し継続せず |
| `feedback_doubt_self_first.md` | 案 A 採用判断は spec literal 整合だが structural verify で wrong assumption と判明 |
| `feedback_explanation_lead_with_conclusion.md` | case ② 推奨を結論ファースト構造で surface |
| `feedback_no_claude_coauthor.md` | commit message Claude 共著行禁止 |
| `feedback_no_auto_commit.md` | 明示指示まで commit しない (本 prep doc も commit 待機) |

---

**status: draft 2026-05-31、AYA review 待ち**
**次 session 着手境界 = §7.1 4 段 cadence → §7.2 γ'-port-α-prep (spec 改訂) → §7.3 γ'-port-α 実装 (§5.2 候補 task 9 件)**
