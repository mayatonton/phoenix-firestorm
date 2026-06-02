# r41 sub-step 4.3-γ'-port-α 完遂 → 4.3-γ'-port-β 着手境界 handoff (2026-05-31)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-prep.md` (sub-step 4.3-γ' 着手前 prep + case ② 採用境界、§1 case ② 採用根拠 + §3 LLShaderMgr Vulkan path hook trace + §4 spec 改訂 plan [commit `2a7e151ffb` で satisfy] + §5.2 γ'-port-α 9 task + §6 risks/caveats 8 件、役割完了)
**本 handoff 位置付け**: sub-step 4.3-γ'-port-α (LLShaderMgr Vulkan path hook + glslang runtime + SPIR-V cache 案 C 配置、case ② = runtime SPIR-V 生成 採用、charter §7.5 boundary refine 範囲、6 file +287/-0) **全完遂宣言** + sub-step 4.3-γ'-port-β (生成済 VkShaderModule からの PSO 構築 + descriptor set 配線) 着手前 scope 確認境界。AYA launch verify PASS で 4.3-γ'-port-α 章 close。

---

## 1. sub-step 4.3-γ'-port-α 全完遂 status (2026-05-31)

### 1.1 完遂 marker (handoff-substep-4-3-gamma-prime-prep.md §5.2 9 task 全完遂)

| acceptance | 達成 status |
|---|---|
| γ'-port-α-1 system pkg / autobuild glslang dependency 選択 (system pkg 採用、Ubuntu 24.04 noble-updates `glslang-dev` 15.1.0-2 提供) | ✓ |
| γ'-port-α-2 `indra/cmake/Glslang.cmake` 新規 + `indra/llrender/CMakeLists.txt` に `include(Glslang)` + `ll::glslang` link 追加 | ✓ |
| γ'-port-α-3 `llshadermgr.h` に `createSPIRVFromGLSL` private 宣言 + `mVk{Vertex,Fragment}ShaderModules` public field 配置 | ✓ |
| γ'-port-α-4 `llshadermgr.cpp` に glslang include + `createSPIRVFromGLSL` impl 約 75 line 配置 + `:908` 直前 hook block 約 70 line 配置 | ✓ |
| γ'-port-α-5 `llvkloader.h/cpp` に `loadSpirvShaderModuleFromMemory(const std::vector<unsigned int>&)` 配置 (loadSpirvShaderModule の wrapper) | ✓ |
| γ'-port-α-6 SPIR-V cache layer 案 C 配置 (hook 内統合 = `~/.ayastorm_x64/cache/shader_cache/<HBXXH128>_{v,f}.spv` fread/fwrite) | ✓ |
| γ'-port-α-7 exemplar PoC (`class1/deferred/diffuseV.glsl` を runtime 自動 fire 対象指定、shader file 自体は untouched) | ✓ |
| γ'-port-α-8 incremental autobuild (configure + build) | ✓ exit 0 / 6 file +287/-0 / warning 0 件 / packaging 完遂 |
| γ'-port-α-9 AYA launch verify + commit (4.3-γ'-port-α 単独 commit) | ✓ AYA 「OK commit して loop 停止」明示指示 / commit `ff48b21d88` |
| install + cache clear 完遂 | ✓ ~/ayastorm/ + ~/.ayastorm_x64/cache/ |

### 1.2 commit hash

| commit | scope |
|---|---|
| `ff48b21d88` | sub-step 4.3-γ'-port-α 全完遂 (6 files changed, +287/-0) |

### 1.3 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-4-3-gamma-prime-prep.md` | **役割完了** (§5.2 9 task 全完遂 → 本 handoff で内容引継ぎ) |
| sub-doc `04-frame-context.md` | active 継続 (sub-step 4.3-δ'/ε'/ζ'/η' marker 着手 ready 状態へ) |
| sub-doc `06-shader-spirv.md` | active 継続 (sub-step 6.1 production path = LLShaderMgr Vulkan path hook 配置完了 = 部分 satisfy、生成 SPIR-V から PSO 構築 + draw 配線は γ'-port-β/γ で satisfy) |
| sub-doc `07-descriptor-renderpass.md` | active 継続 (sub-step 4.3-δ' で 7.3/7.4/7.5 本格化 source) |
| sub-doc `08-llvkrenderer-skeleton.md` | active 継続 (sub-step 4.4 part B で参照) |
| `handoff-substep-4-3-gamma-prime-port-alpha-complete.md` (本 handoff) | 新規作成 (4.3-γ'-port-α 全完遂 → 4.3-γ'-port-β 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 4.3-γ'-port-β 着手 ready 状態へ update) |

---

## 2. 実装内容 (本 commit 範囲 `ff48b21d88`)

### 2.1 cmake 配線 (`indra/cmake/Glslang.cmake` 新規 + `indra/llrender/CMakeLists.txt` +2 line)

| 変更 | 詳細 |
|---|---|
| `indra/cmake/Glslang.cmake` 新規 +25 line | `find_package(glslang CONFIG REQUIRED)` + `add_library(ll::glslang INTERFACE IMPORTED)` + `target_link_libraries(ll::glslang INTERFACE glslang::glslang glslang::SPIRV glslang::glslang-default-resource-limits)` (Vulkan.cmake 同 pattern) |
| `indra/llrender/CMakeLists.txt` `include(Glslang)` 追加 (line 8) | `include(Vulkan)` の直後配置 |
| `indra/llrender/CMakeLists.txt` `target_link_libraries(llrender ...)` に `ll::glslang` 追加 | `ll::vulkan` の直後配置 |
| Ubuntu 24.04 noble-updates `glslang-dev` 15.1.0-2 | `/usr/lib/x86_64-linux-gnu/cmake/glslang/glslang-config.cmake` 提供 = CONFIG mode で find 成功 |
| 3 OS 配信判断 (Mac brew / Win Vulkan SDK) | charter §7.5 軽量 3 注記範囲、r42-α/β Mac/Win 着手時に実施 |

### 2.2 LLShaderMgr Vulkan path 拡張 (`indra/llrender/llshadermgr.h` +16 line)

| 変更 | 詳細 |
|---|---|
| `createSPIRVFromGLSL(GLenum type, U32 source_count, const GLchar** sources, std::vector<unsigned int>& out_spirv)` private 宣言 | line 447、static にせず instance method (mShaderCacheDir 等の context access 余地確保) |
| `mVkVertexShaderModules` / `mVkFragmentShaderModules` public field (`std::map<std::string, VkShaderModule>`) | line 500-501、key = `open_file_name` = shader file path、value = VkShaderModule (volk.h 透過 chain 既存 include 経由) |
| VkShaderModule 透過 include path | llshadermgr.h → llgl.h:46 → llglheaders.h:1097 → volk.h (新規 include 追加なし) |

### 2.3 LLShaderMgr Vulkan path 配線 (`indra/llrender/llshadermgr.cpp` +212 line)

| 変更 | 詳細 |
|---|---|
| **§2.3.1 include 配線 (file 上部 +4 line)** | |
| `#include "llvkloader.h"` 追加 | LLVKLoader::isVulkanInitialized() + loadSpirvShaderModuleFromMemory() 利用 |
| `#include <glslang/Public/ShaderLang.h>` 追加 | glslang::TShader / TProgram / InitializeProcess() |
| `#include <glslang/Public/ResourceLimits.h>` 追加 | GetDefaultResources() |
| `#include <SPIRV/GlslangToSpv.h>` 追加 | glslang::GlslangToSpv() + SpvOptions |
| **§2.3.2 createSPIRVFromGLSL impl (~75 line、dumpObjectLog と loadShaderFile 間配置)** | |
| static bool guard で glslang::InitializeProcess() process-global 単発 init | static bool s_glslang_initialized で 1 回限定、process 終了時の FinalizeProcess() は atexit 任せ |
| GLenum → EShLanguage 変換 (VERTEX/FRAGMENT/GEOMETRY 対応、他は LL_WARNS + false 返却) | switch case で 3 stage 限定 |
| 加工済み GLSL string array を concat → glslang::TShader.setStrings(&src_cstr, 1) | source_count 件 concat で 1 string 化 |
| Vulkan client + target 設定 | setEnvInput(EShSourceGlsl, stage, EShClientVulkan, 450) + setEnvClient(EShClientVulkan, EShTargetVulkan_1_2) + setEnvTarget(EShTargetSpv, EShTargetSpv_1_5) |
| TBuiltInResource = GetDefaultResources() | resource limits は glslang default 流用 |
| EShMessages = EShMsgDefault \| EShMsgVulkanRules \| EShMsgSpvRules | Vulkan 解釈 + SPIR-V 生成 mode |
| parse 失敗時 LL_WARNS + getInfoLog() + false 返却 (caller は GL fallback) | shader.parse(resources, 450, false, messages) |
| TProgram.addShader + link、link 失敗時も LL_WARNS + getInfoLog() + false | program.link(messages) |
| SpvOptions = {generateDebugInfo=false, stripDebugInfo=true, disableOptimizer=true (γ' 段階 PoC 重視), validate=false} | optimizer 有効化は γ'-port-β 以降 perf tuning scope |
| GlslangToSpv(*program.getIntermediate(stage), out_spirv, &spv_options) | out_spirv = std::vector<unsigned int> SPIR-V binary、!out_spirv.empty() を成否で返却 |
| **§2.3.3 hook block (`:908` 直前 = 新行 925、約 70 line)** | |
| ガード `LLVKLoader::isVulkanInitialized() && (type == GL_VERTEX_SHADER \|\| type == GL_FRAGMENT_SHADER)` | spec の `gVK.isEnabled()` placeholder 確定実体 (llrender.cpp:1135 + llvkloader.cpp:2537 確認) |
| (a) HBXXH128 spirv_hash 計算 = `update(open_file_name) + update(全 sources concat)` | llglshader.cpp:2055-2083 algorithm 1:1 流用、program-level mShaderHash は loadShaderFile 時点で未確定のため file-level hash 採用 |
| (b) `shader_cache/<spirv_hash>_{v,f}.spv` path 構築 (mShaderCacheDir = `~/.ayastorm_x64/cache/shader_cache/`) | GL binary cache `.shaderbin` と suffix 違いで併存、`_v.spv` / `_f.spv` で type 判別 |
| (c) cache fread 試行 (file_size % 4 == 0 + read_words 一致で cache_hit) | binary file → std::vector<unsigned int> 復元 |
| (d) cache miss = createSPIRVFromGLSL 呼出 + fwrite 永続化 | 次回起動 cache hit 経路で glslang skip |
| (e) loadSpirvShaderModuleFromMemory → VkShaderModule → `mVk{Vertex,Fragment}ShaderModules[open_file_name]` 格納 | VK_NULL_HANDLE 戻りは LL_WARNS で diagnostic |
| (f) LL_INFOS("Vulkan") で cache hit/miss + filename log | structural log = 既存 llvkloader.cpp:2969 LL_INFOS("Vulkan") "SPIR-V shader module loaded" と同 channel 同 pattern |
| GL path 完全並走維持 | line 928 以降 glCreateShader/glShaderSource/glCompileShader 全件 untouched = AYAstorm 改変 11 file + 段階 1-4.3-β' 動作維持 |

### 2.4 LLVKLoader sink 拡張 (`llvkloader.h` +18 line / `llvkloader.cpp` +14 line)

| 変更 | 詳細 |
|---|---|
| `llvkloader.h` `#include <vector>` 追加 | std::vector<unsigned int> 型受取 |
| `llvkloader.h` `VkShaderModule loadSpirvShaderModuleFromMemory(const std::vector<unsigned int>& spirv)` 宣言 (LLVKLoader namespace 内) | loadSpirvShaderModule の primitive sink wrapper、production sink (sub-doc 03 §3.1.3 役割再定義注記) |
| `llvkloader.cpp` 同名 impl | spirv.empty() guard + LL_WARNS + VK_NULL_HANDLE 返却 / 非空時 loadSpirvShaderModule(spirv.data(), spirv.size() * sizeof(unsigned int)) 委譲 |
| 3.3-B exemplar 試作レール sink (loadSpirvShaderModuleFromFile) と分離維持 | sub-doc 03 §3.1.3 役割再定義注記準拠、production sink (from memory) と試作レール sink (from file) の役割分離 |

### 2.5 SPIR-V cache layer 案 C (hook 内統合配置)

- 配置 dir: `~/.ayastorm_x64/cache/shader_cache/` (mShaderCacheDir = LLShaderMgr 既存 mechanism 流用、initShaderCache で設定済)
- 形式: `<HBXXH128>_{v,f}.spv` binary file
- 初回起動: cache miss → glslang 生成 → fwrite → 永続化
- 次回起動: cache hit → fread → glslang skip
- GL binary cache (`.shaderbin`) と同 dir 併存、metadata `shaderdata.llsd` は GL binary cache の既存 mechanism 流用 (γ' 段階は SPIR-V 独立 metadata 不要)
- handoff prep §5.2 γ'-port-α-6 literal satisfy

### 2.6 file 変更 summary

```
indra/cmake/Glslang.cmake      |  25 +++++
indra/llrender/CMakeLists.txt  |   2 +
indra/llrender/llshadermgr.cpp | 212 +++++++++++++++++++++++++++++++++++++++++
indra/llrender/llshadermgr.h   |  16 ++++
indra/llrender/llvkloader.cpp  |  14 +++
indra/llrender/llvkloader.h    |  18 ++++
6 files changed, 287 insertions(+)
```

---

## 3. build + launch verification

### 3.1 build step

| step | command | result |
|---|---|---|
| 1. configure | `autobuild configure -A 64 -c ReleaseFS_open -- --fmodstudio -DLL_TESTS:BOOL=FALSE -DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE --package --chan AYAstorm-release` | **PASS exit 0** / AYA_GLSLANG_VALIDATOR = /usr/bin/glslangValidator 検出 = AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK 非定義 (3.3-B exemplar 試作レール dead code 除外、Linux baseline 維持) |
| 2. build | `autobuild build -A 64 -c ReleaseFS_open --no-configure` | **PASS exit 0** / llshadermgr.cpp.o + llvkloader.cpp.o + cmake-generated build files 再構築 clean (warning 0 件) |
| 3. package | tar.xz 生成 | Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261511415.tar.xz |
| 4. install | install.sh ~/ayastorm/ | OK / menu entries 配置 |
| 5. cache clear | rm -rf ~/.ayastorm_x64/cache/ | OK |
| 6. AYA launch verify | AYA さん「OK commit して loop 停止」明示指示 | **PASS (regression 0 含意承認)** |

### 3.2 AYA launch verify 詳細

| 項目 | 状態 |
|---|---|
| AYA 承認 | 「OK commit して loop 停止」明示 commit 指示 (短評承認 = regression 0 含意) |
| 案 B cadence 継承 | measurement log 配線 skip + AYA 短評で satisfy |
| structural log 動作確認余地 | 次 session 起動後 `~/.ayastorm_x64/cache/shader_cache/*_{v,f}.spv` 件数 + log "LLShaderMgr Vulkan path: SPIR-V module" cache hit/miss 件数 で SPIR-V 生成成立率実測可能 |
| regression | 0 (AYA 承認、4.3-β' baseline 維持) |

---

## 4. 設計 deviation

### 4.1 system pkg 採用 (autobuild package 経由不採用)

handoff prep §5.2 γ'-port-α-1 で system pkg 採用判断:
- autobuild 3p-glslang archive build infra 新規構築 = +20-40h 見積
- Linux first-class baseline = Ubuntu 24.04 `glslang-dev` 15.1.0-2 即時利用可能
- Mac brew / Win Vulkan SDK 配信判断は **charter §7.5 軽量 3 注記範囲**、r42-α (Win) / r42-β (Mac) 着手時に実施
- 3 OS 整備の現時点 gap は明示 (Linux 先行例外、r41 全体方針整合)

### 4.2 Vulkan 1.2 + SPIR-V 1.5 target (volk_default baseline 整合)

- setEnvClient(EShClientVulkan, **EShTargetVulkan_1_2**)
- setEnvTarget(EShTargetSpv, **EShTargetSpv_1_5**)
- volk_default Vulkan 1.2 baseline (sub-step 1.x で確定) と整合
- 上位 target (1.3+) は r42 以降 advanced feature 着手時に評価

### 4.3 SpvOptions disableOptimizer=true (γ' 段階 PoC 重視)

- generateDebugInfo=false / stripDebugInfo=true / **disableOptimizer=true** / validate=false
- PoC 段階で optimizer 無効化 = 解釈成立確認最優先、optimizer 由来の異常切り分け排除
- optimizer 有効化 + Vulkan validation enable は γ'-port-β 以降 perf tuning + correctness scope

### 4.4 file-level HBXXH128 hash 採用 (mShaderHash program-level 不可)

handoff prep §3 + sub-doc 06 §6.5 改訂履歴整合:
- mShaderHash (llglshader.cpp:2055-2083) は program-level (link 後の hash) = loadShaderFile 時点で未確定
- file-level hash = HBXXH128.update(open_file_name) + HBXXH128.update(全 sources concat) で per-shader-file cache が成立
- HBXXH128 algorithm 1:1 流用 = handoff prep §5.2 γ'-port-α-6 spec の「mShaderHash 1:1 流用」を **algorithm レベル 1:1** で satisfy (hash 入力単位は file-level に refine)

### 4.5 LL_INFOS は structural log として保持 (verification-only ではない)

- 既存 llvkloader.cpp:2969 LL_INFOS("Vulkan") "SPIR-V shader module loaded" と同 channel 同 pattern
- `feedback_remove_verification_logs` scope = 検証用 LL_INFOS hook の除去、structural log は対象外
- 動作確認 + 障害切り分けで継続的に有用 (cache hit/miss + filename log)

---

## 5. risks / caveats (sub-step 4.3-γ'-port-β 着手前 awareness)

### 5.1 γ'-port-α 段階 = SPIR-V 生成 + 格納のみ (PSO 構築 + draw 配線は γ'-port-β/γ scope)

- 本 γ'-port-α で達成 = VkShaderModule の生成 + cache layer 永続化 + mVk{Vertex,Fragment}ShaderModules 格納
- 生成済 VkShaderModule から PSO (VkPipeline) 構築 + descriptor set 配線 + 12 pool placeholder → 実 draw 配線は **γ'-port-β/γ + δ'/ε' scope**
- 本段階 commit `ff48b21d88` 単独では描画動作変化なし (生成された VkShaderModule は PSO に未接続 = recordPlaceholderPoolDraw 維持)

### 5.2 LLShaderMgr 加工済み GLSL の Vulkan 解釈成立率は「見込」

- sub-doc 06 §3.2 §1.2.3 case ② 採用注記の「見込」段階 (parse 失敗 shader は WARN 出力 + GL fallback で動作維持、生成 skip)
- 成立 shader 数の実測手段 = 次 session 起動後 `~/.ayastorm_x64/cache/shader_cache/*_v.spv + *_f.spv` 件数 (~228 file × {V, F} 想定上限と対比)
- AYA launch verify で実 log 確認 + WARN 件数集計が γ'-port-β 着手前の next 重要 task 候補

### 5.3 cache invalidation 戦略未配備 (累積 disk usage 増加可能性)

- GLSL source 変更 → hash 変化 → 新規 cache file 生成、旧 cache file は GC されない
- 開発期間中の累積 disk usage 増加 (1 shader file ~数 KB × ~228 file × source 変動回数)
- TTL/LRU/世代管理は γ'-port-β 以降の独立検討 task 候補 (現時点では cache 全削除 `rm -rf ~/.ayastorm_x64/cache/shader_cache/` で運用)

### 5.4 glslang::InitializeProcess() process-global init

- static bool guard で 1 回限定、FinalizeProcess() は LLShaderMgr destructor で呼出していない (atexit 任せ)
- leak でなく process 終了で OS 回収 = 実害なし
- 明示 FinalizeProcess() 配置は将来 cleaner shutdown 要件で評価

### 5.5 ~228 file 全件 hook fire は startup 時間に影響 (初回 cache miss 時)

- 初回起動 cache miss = parse + link + GlslangToSpv の cumulative cost = 数百 ms ~ 数秒推定 (実測は次 session)
- 2 回目以降 = cache hit で fread のみ = 大幅 latency 短縮想定
- production 段階 (γ'-port-η' 完遂時点) で startup time 計測 + AYA 体感 acceptance 判断

### 5.6 4.3 残 sub-step (handoff-substep-4-3-beta-prep.md §4.2 新 cadence + γ'-port 分割)

| sub-step | scope | 着手境界 |
|---|---|---|
| **4.3-γ'-port-β** | 生成済 VkShaderModule からの PSO 構築 (VkPipeline) + descriptor set 配線 + placeholder PSO → 実 PSO 経路替換 | **次着手 (fresh context 推奨)** |
| 4.3-γ'-port-γ (任意分割) | 12 pool placeholder → 実 SPIR-V PSO 経路替換、vkCmdBindPipeline + vkCmdBindDescriptorSets 配線 | port-β 完遂後 |
| 4.3-δ' | sub-doc 07 §7.3 material cache 本実装 + §7.4 push descriptor 全配線 + §7.5 実 attachment 配線 | γ'-port-* 完遂後 |
| 4.3-ε' | Sky+WLSky+WaterExclusion + 9 pool per-pool 実 scene draw 移植 | δ' 完遂後 |
| 4.3-ζ' | Avatar bone per-draw + GLTFPBR per-draw 移植 | ε' 完遂後 |
| 4.3-η' | self-check + handoff | ζ' 完遂後 |

### 5.7 context budget concern

本 4.3-γ'-port-α 完遂時点で session context 消費中。次 sub-step (4.3-γ'-port-β = PSO 構築 + descriptor 配線 = 中規模 trace + 実装) は **fresh context で着手推奨**、proactive handoff 範式遵守。

---

## 6. next session entry point (sub-step 4.3-γ'-port-β 着手)

### 6.1 着手前 4 段 cadence

1. 本 handoff doc 読込 (sub-step 4.3-γ'-port-α 完遂 status 確認)
2. sub-doc `06-shader-spirv.md` §3.1 / §3.2 / §3.3 / §3.4 読込 (γ'-port-β = case ② runtime path 後段の PSO + descriptor 配線 source)
3. sub-doc `07-descriptor-renderpass.md` §7.3 / §7.4 読込 (γ'-port-β は γ'-port-α で生成された VkShaderModule を sub-step 7.3 PSO 構築経路に接続)
4. memory `project_ayastorm_r41_vulkan_migration.md` 最新 status 読込 + 起動後 `~/.ayastorm_x64/cache/shader_cache/*_{v,f}.spv` 件数 + log "LLShaderMgr Vulkan path" WARN/INFO 件数集計 (γ'-port-α 解釈成立率実測)

### 6.2 sub-step 4.3-γ'-port-β 着手 task 候補

| task | 詳細 |
|---|---|
| γ'-port-β-1 | γ'-port-α SPIR-V 生成成立率実測 (cache file 件数 + WARN 件数集計、~228 file 期待値との gap 検出) |
| γ'-port-β-2 | sub-doc 07 §7.3 PSO 構築経路の現状 trace (3.4-β-1/γ で部分内包済の skeleton 確認) |
| γ'-port-β-3 | VkPipeline 構築 helper 配置 (LLVKLoader namespace 拡張 or 新規 file = sub-doc 07 spec 確認後) |
| γ'-port-β-4 | descriptor set 3 階層 (set=0 PerFrame / set=1 PerMaterial / set=2 AvatarBone) と VkShaderModule の binding 整合確認 |
| γ'-port-β-5 | exemplar PoC = class1/deferred/diffuseV.glsl + diffuseF.glsl から PSO 1 件構築 + recordPlaceholderPoolDraw 経路で fire 確認 |
| γ'-port-β-6 | incremental autobuild |
| γ'-port-β-7 | AYA launch verify (案 B cadence) |
| γ'-port-β-8 | commit (4.3-γ'-port-β 単独 commit、または段階分割可能性) |

### 6.3 critical reminders

| reminder | 詳細 |
|---|---|
| **AYAstorm 改変 11 file shader 改変禁止** | sub-doc 06 §3 範囲 = base 改変なし port のみ、AYAstorm 改変 11 file (picker 2 / Cinematic 2 / visual realism 7) は r42-α/β/γ scope 外維持 |
| **段階 1-4.3-γ'-port-α 動作維持** | GL path 完全並走、生成 VkShaderModule から PSO 接続 + draw 配線変更時の regression watch |
| **3.3-B exemplar 試作レール扱い保持** | sub-doc 03 §3.1.3 役割再定義注記準拠、loadSpirvShaderModuleFromFile() (試作レール sink) と loadSpirvShaderModuleFromMemory() (production sink) の役割分離維持 |
| **case ② path + GL path 完全並走** | charter §3 #1 acceptance = `LLVKLoader::isVulkanInitialized()` OFF 時 GL path 完全動作維持、ON 時 両 path 並走 |
| **AYA 承認境界遵守** | 4.3-γ'-port-α-prep `2a7e151ffb` で確定された case ② runtime path = LLShaderMgr Vulkan path 経由 SPIR-V 生成 + SPIR-V cache layer 案 C + 3.3-B 試作レール扱いの範囲 |
| **案 B cadence 継承** | measurement log 配線 skip + AYA 短評承認で satisfy |
| **context budget proactive 監視** | 次 session 着手時点で context 残量確認 → 周回境界で proactive handoff |
| **proactive handoff 範式** | sub-step 4.3-γ'-port-β は中規模 PSO + descriptor 配線、context 周回境界で能動 handoff 起草 |

### 6.4 commit 戦略

sub-step 4.3-γ'-port-β は **PSO 構築 helper 配置 + exemplar PoC 1 件 fire 確認 で 1 commit** が想定基本形。複数 stage helper / 12 pool 一括接続が必要となれば γ'-port-β-1/β-2/β-3 のような分割 cadence を sub-doc 07 §7.3 spec 確認後に確定。

---

## 7. 関連 doc / memory cross reference

### 7.1 関連 doc

| doc | 役割 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/00-charter.md` | charter §2 領域 6 + §3 #4 + §7.5 軽量 3 注記 (case ② = runtime SPIR-V 生成 採用 spec 根拠) |
| `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` | sub-doc 03 §3.1.3 役割再定義注記 (3.3-B exemplar 試作レール扱い + production sink 分離) |
| `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` | sub-doc 06 §3 case ② path 仕様 (本 γ'-port-α LLShaderMgr Vulkan path hook + glslang runtime + SPIR-V cache 案 C literal satisfy) |
| `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` | sub-doc 07 §7.3/§7.4/§7.5 (**次 sub-step 4.3-γ'-port-β / δ' 着手 source** = PSO 構築 + descriptor 配線 + attachment) |
| `docs/specs/ayastorm-r41-gl-removal/08-llvkrenderer-skeleton.md` | sub-doc 08 LLVKRenderer skeleton (4.4 part B、namespace LLVKLoader 経路継承) |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-beta-prime-complete.md` | sub-step 4.3-β' 完遂 → 4.3-γ' 着手境界 (役割完了、4.3-γ'-port-α-prep で内容引継ぎ) |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-prep.md` | **前 handoff (役割完了、本 γ'-port-α で §5.2 9 task 全完遂)** = case ② 採用根拠 + LLShaderMgr Vulkan path hook trace + spec 改訂 plan |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-alpha-complete.md` | (本 handoff、4.3-γ'-port-β 着手境界) |

### 7.2 関連 memory

| memory | 役割 |
|---|---|
| `project_ayastorm_r41_vulkan_migration.md` | active milestone tracking |
| `project_build_procedure.md` | autobuild fullflow + .venv activate + AUTOBUILD_VARIABLES_FILE |
| `feedback_proactive_handoff.md` | context 圧迫時の proactive handoff 範式 |
| `feedback_self_verify_before_handoff.md` | handoff 起草前の self-trace 義務 |
| `feedback_no_claude_coauthor.md` | commit message Co-Authored-By: Claude 禁止 |
| `feedback_proactive_diagnostic.md` | log/grep/gdb 系は Claude が直接実行 |
| `feedback_log_reading.md` | log 解析は Claude 側、AYA に貼り付けさせない |
| `feedback_one_step_at_a_time.md` | 1 メッセージ 1 アクション cadence |
| `feedback_use_agents_proactively.md` | 重い trace は agent 使用 |
| `feedback_remove_verification_logs.md` | structural log (cache hit/miss + filename) は scope 外 = 除去対象なし |
| `feedback_no_auto_commit.md` | コミットは明示指示後 (本 γ'-port-α は AYA 「OK commit して loop 停止」明示指示下) |
| `feedback_no_scope_shrink.md` | 9 task 全完遂義務 (skip 禁止) |
| `feedback_self_bug_no_defer_option.md` | parse 失敗 shader への defer/disable 提案禁止 = GL fallback (構造的並走) で動作維持 |

---

**本 handoff 起草日**: 2026-05-31
**起草根拠**: AYA さん「memory update + handoff doc 起草 次のセッションに投げるメッセージを最後にください」承認下で memory update + handoff doc 起草を並走
**次 session 着手**: sub-step 4.3-γ'-port-β (生成済 VkShaderModule からの PSO 構築 + descriptor set 配線) 着手境界 (fresh context 推奨、中規模 trace + 実装想定)
