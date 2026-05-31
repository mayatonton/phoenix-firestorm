# AYAstorm r41 sub-doc 06-shader-spirv — 領域 6 248 GLSL shader SPIR-V 化 base port

**status**: **active 2026-05-31 re-author (sub-step 4.3-γ'-port-α-prep で case ② = runtime SPIR-V 生成 path に re-author、AYA review 待ち / 旧 closed 2026-05-29 Pattern α 一括 draft + AYA review PASS は §6.4 改訂履歴で歴史保存)**
**親 charter**: `docs/specs/ayastorm-r41-gl-removal/00-charter.md` (§2 領域 6 + §3 #4 + §7.5 boundary refine 履歴 2026-05-31 case ② 採用反映済)
**並走 sub-doc**: `03-state-machine-pso.md` (段階 3 = 領域 3、§3.1.3 役割再定義注記で 3.3-B exemplar = 試作レール扱い反映済) + `07-descriptor-renderpass.md` (領域 7 = descriptor set binding と shader binding の整合)
**前 handoff**: `handoff-substep-4-3-gamma-prime-prep.md` (sub-step 4.3-γ' 着手前 prep + case ② 採用境界、本 sub-doc re-author の起点)
**達成条件**: 領域 6 完遂 = 248 GLSL shader のうち **base 228 file** (cross compile A 195 素通り + B 53 要修正、本分類は LLShaderMgr 加工済み GLSL を glslang runtime に渡す前提でも維持と仮置き = 6.2/6.3 で検証) を **LLShaderMgr Vulkan path 経由 glslang library runtime API で SPIR-V 化 → `vkCreateShaderModule` load → SPIR-V cache layer hit/miss 動作** + descriptor set binding mismatch validation 0 件。AYAstorm 改変 file (γ'-1 trace 確定値 **11 file** = picker 2 / Cinematic 2 / visual realism 7、charter §3 #4 spec drift 反映) は **r42-α/β/γ scope 外、本領域では untouched 維持**
**関連 charter section**: §2 領域 6 (4.96 PM、単一最大領域、case ② 採用 risk 内訳反映済) + §3 #4 acceptance (228 file SPIR-V 化 + AYAstorm 11 file untouched、spec drift 反映済) + §7.4 sub-doc 構成 + §7.5 boundary refine 履歴 2026-05-31 case ② 採用

---

## §1 領域 6 scope plan

### §1.1 領域 6 scope 再掲 (charter §2 領域 6 + 04 §1.3 + 05 §2)

- **対象**: 248 GLSL shader 全 file のうち **base 228 file** (cross compile A 195 素通り + B 53 要修正)
- **境界条件 (charter §2 領域 6)**: 領域 3 PSO 化 + 領域 7 descriptor set 整合と協調 (shader binding が descriptor set 3 階層 set=0/1/2 に従う)
- **依存順序 (charter §2 領域 6)**: 領域 1 (段階 1) 完了後着手、領域 2 (段階 2 完遂済) + 領域 3 (段階 3) + 領域 7 と並走必須
- **risk 性質 (charter §2 領域 6)**: **中** — glslang cross compile ~85% 素通り見込み (05 §2.2 確定値)、B 53 file の修正は内容次第 (extension / binding / precision / `attribute|varying` legacy 残存)、06 §3.1 で base 算定済
- **AYAstorm 改変 11 file の取扱**: **r42-α (picker 2 file) / r42-β (Cinematic 2 file) / r42-γ (visual realism 7 file) scope 外** (γ'-1 trace 確定値、charter §3 #4 spec drift 反映)、本領域 6 では untouched 維持 (charter §3 #4 acceptance regression 担保)

### §1.2 shader file 分類 + 実装方針 (04 §1.3 + 05 §2.3 反映)

#### §1.2.1 base 248 file (本領域 6 対象)

a-4 §1.3 directory 別:

| directory | file 数 | 性質 | 領域 6 実装方針 |
|---|---|---|---|
| class1/deferred | 120 | deferred rendering core (g-buffer / soften lighting / sky / atmospherics) | LLShaderMgr Vulkan path 経由 glslang runtime compile、descriptor set binding は 07 §3 set=0/1/2 と整合、AYAstorm 改変済 file (picker / Cinematic / visual realism = 11 file、γ'-1 trace 確定値) は **本領域で touch しない** |
| class1/interface | 44 | UI (2D / font / cursor) | descriptor set=0 per-frame 中心 (texture atlas + 2D matrix UBO)、push constant で widget position |
| class3/deferred | 16 | high-end feature (SSAO / DoF / reflection probe 等) | A 素通り見込み高い (a-4 §1.3 + 05 §2.2)、ただし AYAstorm 改変 7 file (visual realism r42-γ scope) は本領域で touch しない |
| class1/objects | 14 | object 描画 (avatar / mesh / terrain) | terrain shader は段階 3 sub-step 3.4 (terrain glTexGen 廃止) と並走、shader 側 explicit UV attribute 化を本領域で配信 (terrain shader port は領域 3 + 領域 6 ジョイントで satisfy) |
| その他 | 54 | windlight / cinematic_bd / 等 | windlight base shader は本領域 port、cinematic_bd の AYAstorm 改変 2 file (r42-β scope、γ'-1 trace 確定値) は本領域で touch しない |

#### §1.2.2 cross compile 分類 (05 §2.2 反映)

| cross compile classification | file 数 | 内容 | 領域 6 実装方針 |
|---|---|---|---|
| **A: 素通り** | 195 file | LLShaderMgr 加工済み GLSL → glslang runtime API で 1 pass compile 成功想定、修正不要 | sub-step 6.2 で viewer 起動時 LLShaderMgr 経由 runtime SPIR-V 生成成功 + `vkCreateShaderModule` load 成功 verify |
| **B: 要修正** | 53 file | 残 15% issue (built-in 座標 `gl_FragCoord` / `gl_PointCoord` Y 反転、`attribute|varying` legacy 残存、`#extension GL_ARB_*` 依存、`precision highp|mediump|lowp` qualifier) | sub-step 6.3 で issue type 別 bundle で base GLSL 直 patch、各修正後 runtime 経路再 verify |
| **AYAstorm 改変 11 file** | 11 file | r42-α/β/γ scope 外 (γ'-1 trace 確定値) | sub-step 6.5 で untouched 維持 verify (`git diff` で 11 file が touch 0 件確認) |

### §1.2.3 case ② 採用注記 (2026-05-31 sub-step 4.3-γ'-port-α-prep、charter §7.5 boundary refine 履歴と同期)

旧 sub-doc 06 (closed 2026-05-29) は **build-time pre-compile model** (CMake glob + glslangValidator 一括 `--target-env vulkan1.3` で `.spv` 生成 → autobuild package 同梱 → runtime `vkCreateShaderModule` load) を前提に記述。sub-step 4.3-γ' 着手で **案 A target extension build verify が spec 想定相違 4 点で fail** (`#version` 無し / 自由 uniform / location 無し / forward decl concat 前提) → revert → **case ② = runtime SPIR-V 生成 採用** (詳細は `handoff-substep-4-3-gamma-prime-prep.md` §1)。

| 旧 path (build-time pre-compile、closed 2026-05-29 spec) | 新 path (case ② runtime、active 2026-05-31 re-author) |
|---|---|
| CMake glob + REMOVE_ITEM で AYAstorm 改変除外 + foreach glslangValidator 一括 compile | **LLShaderMgr `loadShaderFile()` 既存 GL preprocessing path 完全並行**: `loadShaderFile()` 内 `#version` prepend + `#define` 注入 + multiple .glsl concat 後の **加工済み GLSL string array** を **`llshadermgr.cpp:908` 直前** で `gVK.isEnabled()` 条件下 conditional branch に通し、glslang library runtime API で SPIR-V binary 生成 → `vkCreateShaderModule` |
| `.spv` を autobuild package に同梱 + 起動時 file load | **SPIR-V cache layer 案 C** (`~/.ayastorm_x64/cache/shader_cache/<mShaderHash>_{vert,frag}.spv` + `shaderdata.llsd` metadata 統一、GL binary cache と並存、`mShaderHash` = `llglslshader.cpp:2055-2083` HBXXH128 cache key 1:1 流用) |
| build host に glslangValidator install 必須 (Linux apt / Mac brew / Win Vulkan SDK) | **glslang library を viewer 配信物に bundled SO/dylib/DLL 同梱** (3 OS、autobuild.xml dependency 追加、size +5-10MB 推定) |
| 3.3-B exemplar `aya_r41_exemplar/sky_placeholder{V,F}.glsl` purpose-built Vulkan-ready GLSL を 6.1 一括化までの prep 配置、6.1 完遂後正規 path 移管判断 | **3.3-B exemplar = 試作レール扱い** (sub-doc 03 §3.1.3 役割再定義注記反映)、build-time `.spv` sink `loadSpirvShaderModuleFromFile()` (`llvkloader.cpp:1761-1770`) は試作レール sink として保持、production sink = `loadSpirvShaderModuleFromMemory()` (新規、γ'-port-α-5 で配置) |

**case-validity 担保**: §1.2.2 A 195 / B 53 / AYAstorm 11 (旧 13) 分類は LLShaderMgr 加工済み GLSL → glslang runtime でも同分類が成立する見込み (旧 spec 妥当性継承、6.2/6.3 で per-file 検証)。LL/FS shader variant 爆発 model (`#define HAS_NORMAL_MAP` / `WATER_FOG` / `HAS_SKIN` 等数十 feature flag × runtime compile で数百-数千 variant) は LLShaderMgr 既存 preprocessing path 1 source of truth として保持、case ② はその出口を SPIR-V binary に追加するだけ (GL path 完全並行維持、charter §3 #1 acceptance 担保)。

### §1.3 並走領域との関係 (charter §2 領域 6 依存順序)

| 並走領域 | 領域 6 内での協調事項 |
|---|---|
| 領域 3 (段階 3 state machine → PSO 化) | PSO compile 時に SPIR-V binary 必要、shader binding (vertex attribute / push constant / descriptor set) が段階 3 PSO layout と一致する必要、段階 3 sub-step 3.3 matrix stack → push constant 化と shader 側 push constant 受領が同期。**段階 3 sub-step 3.3-B (1 shader exemplar pre-flight、sub-doc 03 §3.1.3 / 2026-05-30 着手境界) は case ② re-author で「試作レール扱い」へ役割再定義** (sub-doc 03 §3.1.3 役割再定義注記反映) — 3.3-B で配置済の CMake target `aya_shader_compile` + AyaShaderCompile.cmake build-time pre-compile chain + `aya_r41_exemplar/sky_placeholder{V,F}.glsl` + `loadSpirvShaderModuleFromFile()` は **試作レール sink として保持** (`AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK` macro guard temporary safety net 維持 to r42-α/β)。本領域 sub-step 6.1 の production path は **LLShaderMgr Vulkan path hook (`llshadermgr.cpp:908` 直前 `gVK.isEnabled()` 条件下 conditional branch) + `loadSpirvShaderModuleFromMemory()` 新規 sink** で 248 file 全体を runtime SPIR-V 生成 → cache layer 経路で satisfy |
| 領域 7 (descriptor set + render pass) | shader binding (set=0/1/2 + binding 番号) と 07 §3 descriptor set 3 階層が一致、shader 側 `layout(set=N, binding=M)` qualifier 配信を領域 7 の `VkDescriptorSetLayoutBinding` 設計と同期 |
| 領域 2 (lldrawpool、段階 2 完遂済) | 12 pool hook body PSO bind 時に shader binding 整合 (段階 3 sub-step 3.4 と並走)、terrain pool は terrain shader explicit UV 化と同期 |
| 領域 8 (LLVKRenderer skeleton) | shader load API (`vkCreateShaderModule` 経由) の signature が r41.5 interface 経由 call 化前提に整合 (charter §3 #6 担保) |

---

## §2 cross compile 順序 + dependency graph

### §2.1 cross compile 順序の決定軸 3 点 (case ② 反映)

1. **LLShaderMgr Vulkan path hook 配置先行** — `llshadermgr.cpp:908` 直前で `gVK.isEnabled()` 条件下 conditional branch を配置し、加工済み GLSL string array → glslang library runtime API → SPIR-V binary → `vkCreateShaderModule` の経路を確立 (sub-step 6.1)。本 hook 配置後の検証で A 195 / B 53 分類が runtime path 下で成立するか per-file 確認
2. **A 素通り 195 file 先行検証** — runtime SPIR-V 生成成功想定の 195 file を **LLShaderMgr 起動時 shader load 経路** (`loadShaderFile()` → glslang runtime compile → `vkCreateShaderModule`) で順次検証、acceptance #4 base portion の 80% を早期 satisfy + B 53 file の typical issue 抽出母数として A 通過 baseline 確立 (sub-step 6.2)
3. **B 53 file の issue type 別 bundle 修正** — built-in 座標 Y 反転 / legacy `attribute|varying` / `#extension GL_ARB_*` / `precision` qualifier の 4 issue type に分類、type 別 bundle で修正 (Agent 並列活用候補) — **修正対象は LLShaderMgr 加工対象 base GLSL file** (`indra/newview/app_settings/shaders/class*/...`)、加工自体 (`#version` prepend / `#define` 注入) は LLShaderMgr 既存 path 内で完結 (sub-step 6.3)
4. **descriptor set binding 統合は領域 7 同期** — shader 側 `layout(set=N, binding=M)` qualifier 配信は領域 7 の `VkDescriptorSetLayoutBinding` 設計が確定してから (07 §3 sub-step 7.2-7.4 と sub-step 6.4 が同期)

### §2.2 dependency graph (5 階層、case ② runtime path、hook 配置 → A → B → descriptor binding → untouched verify)

```
LLShaderMgr Vulkan path hook + glslang library bundling + SPIR-V cache layer (sub-step 6.1)
├── llshadermgr.cpp:908 直前 gVK.isEnabled() conditional branch 配置
├── llvkloader.cpp loadSpirvShaderModuleFromMemory(const std::vector<uint32_t>&) 新規配置
├── glslang library (libshaderc_combined or libglslang) を autobuild dependency 化 (3 OS = Linux apt / Mac brew / Win Vulkan SDK)
├── SPIR-V cache layer 案 C: ~/.ayastorm_x64/cache/shader_cache/<mShaderHash>_{vert,frag}.spv + shaderdata.llsd metadata
└── 3.3-B exemplar (試作レール、03 §3.1.3 反映) は build-time .spv sink loadSpirvShaderModuleFromFile() 保持
A 195 file runtime 素通り cross compile (sub-step 6.2、acceptance #4 base portion 80% 早期 satisfy)
└── viewer 起動時 LLShaderMgr 経由 195 file runtime SPIR-V 生成成功 + vkCreateShaderModule load + cache layer hit/miss 動作
    └── class1/deferred 大半 + class1/interface 大半 + class3/deferred 一部 + class1/objects 大半 + その他大半
B 53 file 要修正 (sub-step 6.3、issue type 別 bundle、base GLSL 修正後 runtime 検証)
├── built-in 座標 Y 反転 (glslang runtime option で吸収可な file)
├── legacy `attribute|varying` 残存 (`in|out` 書換、base GLSL 直 patch)
├── `#extension GL_ARB_*` 依存 (SPIR-V capability mapping audit、glslang runtime に capability ヒント供給)
└── `precision highp|mediump|lowp` qualifier (Vulkan portability 要、Mac MoltenVK 整合)
descriptor set binding 統合 (sub-step 6.4、領域 7 §3 sub-step 7.2-7.4 と同期)
└── shader 側 `layout(set=N, binding=M)` qualifier 配信 + matrix stack → push constant 受領
領域 6 self-check + AYAstorm 11 file untouched verify (sub-step 6.5)
└── `git diff` で 11 file が touch 0 件確認 + acceptance #4 全 metric PASS
```

### §2.3 並列着手可能性

- A 195 file の runtime SPIR-V 生成検証は file 単位で independent → Agent 並列活用 (`feedback_use_agents_proactively.md` 反映)、起動時 LLShaderMgr 一括 load で network 効果
- B 53 file の issue type 4 bundle は bundle 間で independent → 並列着手可能 (issue type 別に Agent 分担、base GLSL 修正 patch)
- descriptor binding 統合 (sub-step 6.4) は領域 7 の sub-step 7.2-7.4 と同期必須 → 並走 cadence は領域 7 sub-doc draft 進捗 base で AYA + Claude 擦り合わせ
- AYAstorm 11 file untouched verify (sub-step 6.5) は最終 self-check phase で実施、本 sub-doc では sub-step 6.5 の単一 phase に集約

---

## §3 領域 6 sub-step 順序 (5 sub-step 化、段階 2/3 範式継承)

`02-portage-execution.md` §3 + `03-state-machine-pso.md` §3 範式継承 (5 sub-step + 末尾 self-check)、§2.2 dependency graph に沿って bundle。

### §3.1 sub-step list

| sub-step | scope | 対象 (file 数) | 完了 marker |
|---|---|---|---|
| **6.1** | LLShaderMgr Vulkan path hook + glslang library bundling + SPIR-V cache layer (case ② 採用、3.3-B exemplar = 試作レール扱い) | `llshadermgr.cpp:908` 直前 `gVK.isEnabled()` conditional branch 配置 + `llvkloader.cpp` 新規 helper `loadSpirvShaderModuleFromMemory(const std::vector<uint32_t>&)` 配置 + glslang library を autobuild dependency 化 (3 OS: Linux apt / Mac brew / Win Vulkan SDK、size +5-10MB 推定) + SPIR-V cache layer 案 C (`~/.ayastorm_x64/cache/shader_cache/<mShaderHash>_{vert,frag}.spv` + `shaderdata.llsd` metadata、`mShaderHash` = `llglslshader.cpp:2055-2083` HBXXH128 流用) + AyaShaderCompile.cmake / `aya_r41_exemplar/sky_placeholder{V,F}.glsl` は **試作レール sink として保持** (sub-doc 03 §3.1.3 反映) | 任意 1 exemplar file (例: `class1/deferred/diffuseV.glsl`) を LLShaderMgr 経由 runtime SPIR-V 生成 + `vkCreateShaderModule` load 成功 (validation 0 件) + cache miss → 生成 → 再起動時 cache hit 経路動作 + AYA launch verify PASS (regression 0、`gVK.isEnabled()` OFF 時 GL path 完全並行維持) |
| **6.2** | A 195 file runtime SPIR-V 生成 素通り検証 | A 分類 195 file (class1/deferred 大半 + class1/interface 大半 + class3/deferred 一部 + class1/objects 大半 + その他大半) | 195 file 全 runtime SPIR-V 生成成功 + `vkCreateShaderModule` load 成功 + validation 0 件 + cache layer hit/miss 動作、acceptance #4 base portion 80% 早期 satisfy |
| **6.3** | B 53 file 要修正 (issue type 別 bundle、base GLSL 直 patch) | B 分類 53 file を 4 issue type bundle 化 (built-in 座標 Y 反転 / legacy `attribute|varying` / `#extension GL_ARB_*` / `precision` qualifier) を base GLSL 直 patch、LLShaderMgr preprocessing path はそのまま | 53 file 全 runtime SPIR-V 生成成功 + load 成功 + validation 0 件、issue type 別修正内容 doc 化 (handoff 含み) |
| **6.4** | descriptor set binding 統合 (領域 7 同期) | shader 側 `layout(set=N, binding=M)` qualifier 配信 (set=0 per-frame / set=1 per-material / set=2 per-draw) + matrix stack → push constant 受領 (段階 3 sub-step 3.3 同期) | 228 file 全 shader binding が 07 §3 descriptor set 3 階層と一致 + validation layer で descriptor binding mismatch 0 件 + 段階 3 PSO compile 成功 |
| **6.5** | 領域 6 self-check + AYAstorm 11 file untouched verify + handoff doc | (本 sub-step) | §4.1 acceptance 4 件 self-trace PASS (charter §3 #4 領域 6 分 + AYAstorm 11 file untouched + regression) + `git diff` で 11 file touch 0 件確認 + handoff doc `handoff-stage-6-complete.md` 作成 |

### §3.2 sub-step 内 file 順序の柔軟性 (charter §7.5 boundary refine 可)

- sub-step 6.2 の A 195 file は順序問わず viewer 起動時 LLShaderMgr 一括 load → cache layer 自動充填、検証は Agent 並列で起動 log + cache file inspection を bundle 化
- sub-step 6.3 の B 53 file は issue type 4 bundle (各 ~10-15 file) → bundle 間並列着手可能、Agent 分担
- sub-step 6.4 descriptor binding 統合は領域 7 sub-step 7.2-7.4 と同期必須 → 領域 7 sub-doc draft 進捗 base で着手 timing 調整

### §3.3 領域 6 で touch しない file (本領域 scope 外)

- **AYAstorm 改変 11 file** (r42-α picker 2 / r42-β Cinematic 2 / r42-γ visual realism 7、γ'-1 trace 確定値、charter §3 #4 spec drift 反映) — 本領域で touch 0 件 (sub-step 6.5 で `git diff` verify)
- llrender 主要 5 file (段階 3 scope)、lldrawpool 13 file C++ 側 (段階 2 完遂済 + 段階 3 sub-step 3.4 で hook body PSO bind 配線)
- pipeline.cpp 3 大グローバル (段階 4 scope)、llspatialpartition / llviewershadermgr / llvertexbuffer / llvosky / llvowlsky (段階 5 scope)
- descriptor set layout / pipeline layout / render pass chain 実装 (領域 7 scope、本領域は shader 側 `layout()` qualifier 配信のみ)
- LLVKRenderer pipeline.cpp inline 実装 (段階 4 + 領域 8 scope)

---

## §4 領域 6 completion criteria

charter §3 acceptance criterion #4 (228 file SPIR-V 化 + AYAstorm 11 file untouched、spec drift 反映) + #3 (段階 1-5 全完遂と協調) の **領域 6 分 self-check**。

### §4.1 領域 6 自己 acceptance (case ② runtime path 反映)

| criterion | metric | test procedure |
|---|---|---|
| **#4-領域 6 (A 195 file runtime 素通り)** | A 分類 195 file 全 runtime SPIR-V 生成成功 + `vkCreateShaderModule` load 成功 + validation 0 件 + cache layer hit/miss 動作 | viewer 起動時 LLShaderMgr 経由 195 file shader load 成功 LL_INFOS log + validation layer で `vkCreateShaderModule` error 0 件 + cache miss → 生成 → 再起動時 cache hit 経路 log 確認 + `~/.ayastorm_x64/cache/shader_cache/` 配下 195 × 2 (vert/frag) ファイル生成確認 |
| **#4-領域 6 (B 53 file 修正後 runtime 素通り)** | B 分類 53 file の issue type 別 base GLSL patch 後 runtime SPIR-V 生成成功 + load 成功 + validation 0 件 | sub-step 6.3 完遂後 viewer 起動時 53 file 全 runtime load 成功 + validation 0 件 + 修正内容 (issue type 4 bundle) handoff doc 反映 |
| **#4-領域 6 (descriptor set binding 整合)** | 228 file 全 shader 側 `layout(set=N, binding=M)` qualifier が 07 §3 descriptor set 3 階層 (set=0 per-frame / set=1 per-material / set=2 per-draw) と一致 + 段階 3 PSO compile 成功 | `grep -rE "layout\(set=" indra/newview/app_settings/shaders/` で 228 file 全 file 内 set= qualifier hit + validation layer で descriptor binding mismatch / push constant range mismatch **0 件** + 段階 3 sub-step 3.4 PSO compile success log 確認 |
| **#4-領域 6 (AYAstorm 改変 11 file untouched)** | 領域 6 全 sub-step で 11 file (picker 2 / Cinematic 2 / visual realism 7、γ'-1 trace 確定値) が **touched 0 件** | `git diff feature/ayastorm-r41-gl-removal..HEAD -- <11 file path 列挙>` が **0 件 hit** (charter §3 #4 acceptance regression 担保) |
| **regression (段階 1-4.3-β' 動作維持 + GL 並走)** | 段階 1-4.3-β' acceptance 維持 + viewer 起動 + `gVK.isEnabled()` OFF 時 GL path 完全動作 + AYAstorm 機能 (audio / chat / login / inventory) regression 0 件 | `01-foundation.md` §4.1 + `02-portage-execution.md` §4.1 + `03-state-machine-pso.md` §4.1 acceptance 再 verify + `gVK.isEnabled()` OFF/ON 両動作確認 + sustained ~10 分動作 + AYA 起動確認 PASS (`feedback_release_with_user_feedback.md` 遵守) |

### §4.2 不達時の対処 (charter §3 acceptance 運用方針継承)

- 5 criterion のうち 1 件でも未達 = 領域 6 未達 (段階 4 残 sub-step + 領域 7 着手保留判断、charter §7.5 boundary refine で領域別 partial pass も可)
- 未達 criterion 別に対処 (例: #4-領域 6 で specific B file の `precision` qualifier mismatch validation error 残存 → glslang runtime API target version refine → 再 runtime compile → verify)
- **defer / disable 提案 ban** (`feedback_self_bug_no_defer_option.md` 遵守、fix 案のみ提示)
- **仮説 2 連続外れ rule** (`feedback_admit_unknown.md` 遵守): runtime compile error / SPIR-V binary mismatch / descriptor binding violation で仮説 2 連続外れたら glslangValidator CLI で個別 file dump (`glslangValidator -V -S <stage> --target-env vulkan1.3 -o <out>.spv <in>.glsl`) + SPIRV-Cross reflect / spv-val validation で実データ取得に切替

### §4.3 領域 6 完遂後の次 段階

- **段階 4 残 sub-step (4.3-δ' 以降) 着手判断** (4.3-γ' = 本領域 6 sub-step 6.1 完遂、charter §7.5 boundary refine で領域 6 sub-step 6.1 を sub-step 4.3-γ' 内に前出し済)
- **領域 7 完遂判断**: 本領域 6 と並走の領域 7 sub-doc 07 が completion 状態か AYA 確認
- **handoff doc**: `handoff-stage-6-complete.md` 作成 (領域 6 完遂境界、`feedback_proactive_handoff.md` 遵守)
- **AYAstorm 改変 11 file r42 移行 prep**: r41 完遂後 r42-α/β/γ で 11 file SPIR-V port、本領域 6 で確立した LLShaderMgr Vulkan path + descriptor binding pattern + SPIR-V cache layer を r42 で流用

---

## §5 関連 doc / memory

### §5.1 直接参照 doc

| doc | 本 sub-doc での参照 section |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/00-charter.md` | §2 領域 6 (4.96 PM 単一最大領域 中 risk、case ② path 注記反映) + §3 #4 acceptance (AYAstorm 改変 11 file spec drift 反映) + §7.4 sub-doc 構成 + §7.5 boundary refine 履歴 2026-05-31 case ② 採用 |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-prep.md` | sub-step 4.3-γ' 着手前 prep、case ② 採用根拠 (§1) + LLShaderMgr Vulkan path hook trace (§3) + spec 改訂 plan (§4) + γ'-port-α task candidates 9 件 (§5.2) + risks/caveats (§6)、本 sub-doc re-author の起点 |
| `docs/specs/ayastorm-r41-gl-removal/01-foundation.md` | §3.4 sub-step 範式継承 + §3.5 段階 1 で touch しない file の領域 6 並走着手前提反映 |
| `docs/specs/ayastorm-r41-gl-removal/02-portage-execution.md` | §3.3 段階 2 で touch しない file の領域 6 並走前提 + §1.3 並走領域協調事項 (terrain shader 配信は段階 3 と本領域でジョイント satisfy) |
| `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` (本 sub-doc と並走) | §1.3 並走領域協調事項 + §3 sub-step 3.3 matrix stack → push constant 化と本領域 sub-step 6.4 同期 + §3 sub-step 3.4 terrain shader 配信と本領域 sub-step 6.3 同期 + **§3.1.3 役割再定義注記** (3.3-B exemplar = 試作レール扱い、build-time pre-compile chain は試作レール sink として保持、production sink = `loadSpirvShaderModuleFromMemory()` (新規、γ'-port-α-5 で配置)、本領域 sub-step 6.1 で LLShaderMgr Vulkan path hook + glslang library bundling + SPIR-V cache layer を 248 file 全体へ展開) |
| `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` (本 sub-doc と同時起草) | §3 descriptor set 3 階層 + §3 sub-step 7.2-7.4 と本領域 sub-step 6.4 同期 (shader 側 `layout(set=N, binding=M)` qualifier 配信が領域 7 の `VkDescriptorSetLayoutBinding` 設計に従う) |
| `docs/specs/ayastorm-r40-vulkan-migration/04-portage-inventory.md` | §1.3 GLSL shader 248 file 内訳 (class1/2/3 deferred + interface + lighting + windlight + cinematic_bd) + §5.1-§5.3 AYAstorm 機能 shader touchpoint (r40 spec 13 file 起源、γ'-1 trace で 11 file へ refine、r42 scope 外明示は維持) |
| `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` | §1.3 LunarG SDK 1.3.x + glslang + SPIRV-Cross + VMA 採用 (本領域 sub-step 6.1 integration) + §2 shader cross compile chain (本 §1.2 + §2 base) + §2.2 GLSL feature → SPIR-V capability mapping (compute/geometry/tessellation 全 0、~85% 素通り見込) + §2.3 base 248 + AYAstorm 改変 13 取扱 + §2.4 GLSL extension audit 要点 + §2.5 build integration |
| `docs/specs/ayastorm-r40-vulkan-migration/06-effort-estimation.md` | §3.1 + §3.2 領域 6 PM 4.96 (本 §1 + §3 領域 6 scope 整合) |

### §5.2 関連 memory

| memory | 本 sub-doc での参照 |
|---|---|
| `project_ayastorm_r41_vulkan_migration.md` | r41 milestone active 状態 + 領域 6 並走着手前提 |
| `project_ayastorm_three_platforms.md` | 3 OS 大前提 + Linux 先行例外 (本領域 6 も Linux 限定動作確認、§4.1 #regression 反映)、Mac MoltenVK は SPIR-V → MSL 内部変換 (sub-step 6.4 portability subset 整合) |
| `reference_deferred_shader_routing.md` | sub-step 6.4 shader binding 整合時の deferred shader routing reference |
| `project_aya_visual_realism_alpha_protect.md` | AYAstorm 改変 7 file (visual realism、r42-γ scope 外) の `frag_color.a = 0` invariant 認識 (本領域 untouched 維持で保護) |

### §5.3 関連 feedback

| feedback | 本 sub-doc での参照 |
|---|---|
| `feedback_experiment_branch_single_scope.md` | branch scope 単一性 (r41 内 sub-branch 作らない、`01-foundation.md` §2.3 継承) |
| `feedback_no_auto_commit.md` | commit は AYA 指示後 |
| `feedback_release_flow.md` | push は AYA 手動 |
| `feedback_self_bug_no_defer_option.md` | §4.2 defer / disable 提案 ban + AYAstorm 11 file untouched 維持の選択肢化禁止 |
| `feedback_self_verify_before_handoff.md` | §3 sub-step 6.5 self-check + §4.3 領域 6 完遂境界 self-trace |
| `feedback_proactive_handoff.md` | §4.3 領域 6 完遂時の handoff doc 作成 |
| `feedback_build_only_verified.md` | §4 completion criteria の satisfy は実機検証 (推論 ban) |
| `feedback_admit_unknown.md` | §4.2 cross compile / SPIR-V binary mismatch / descriptor binding violation で仮説 2 連続外れたら glslangValidator 個別実行 + SPIRV-Cross reflect + spv-val 切替 |
| `feedback_use_agents_proactively.md` | §2.3 + §3.2 sub-step 6.2 A 195 file LLShaderMgr 経由 runtime 検証 + sub-step 6.3 B 53 file issue type 4 bundle で Agent 並列活用 |
| `feedback_perf_map_bfs_drill.md` | §2.2 dependency graph の階層的 sub-step 化 (integration → A 素通り → B 修正 → descriptor binding → untouched verify) |
| `feedback_release_with_user_feedback.md` | §4.1 #regression の exhaustive solo session 不要 (AYA 起動確認 PASS で sufficient) |
| `feedback_one_step_at_a_time.md` | §3 sub-step 単位で 1 メッセージ 1 アクション着手 |

---

## §6 起草 cadence + 完成宣言

### §6.1 本 sub-doc 起草情報

- **起草着手**: 2026-05-28
- **起草主体**: AYA + Claude
- **起草先**: `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` (本 doc)
- **起草 cadence**: **Pattern α (一括 draft)** — `01-foundation.md` / `02-portage-execution.md` / `03-state-machine-pso.md` 範式継承、sub-doc は内容具体 (228 file 分類 / 5 sub-step / acceptance 5 件) で section 数少なく、Pattern β 分割 overhead 回避 (charter §7.5 boundary refine 可)
- **scope**: **領域 6 only + AYAstorm 11 file untouched 維持** (228 file SPIR-V 化 base port、γ'-1 trace 確定値)、r42-α/β/γ の AYAstorm 改変 11 file port は別 milestone scope
- **並走起草 sub-doc**: 同 session で `03-state-machine-pso.md` (段階 3 = 領域 3) + `07-descriptor-renderpass.md` (領域 7) を Pattern α 一括起草、3 doc 同時 AYA review (handoff-stage-2-complete §2.3 B 案、AYA 採用)

### §6.2 sub-doc 番号付与の justification (charter §7.4 outline → 領域番号同期 refine)

`03-state-machine-pso.md` §6.2 と同等の justification。本 sub-doc は **`06-shader-spirv.md`** で確定 (charter §7.4 outline `03-shader-port.md` から refine、領域番号 = sub-doc 番号同期方針)。

| charter §7.4 outline | 本 r41 章実採用 | 採用理由 |
|---|---|---|
| `03-shader-port.md` | **`06-shader-spirv.md`** | sub-doc 番号 = charter §2 領域番号 (領域 6) で統一、r41 章内 cross reference 整合 |

本 §6.2 の番号 refine は charter §7.5 boundary refine 可の範囲内、AYA review で承認後に正式採用。

### §6.3 完成宣言条件 (case ② re-author)

本 sub-doc は **AYA review PASS で完成宣言再付与**、status field を `closed YYYY-MM-DD (case ② runtime path 反映 re-author + AYA review PASS、sub-step 4.3-γ' γ'-port-α 着手準備 ready)` に更新。

完成宣言後の次 action:

- sub-step 4.3-γ'-port-α 着手 (LLShaderMgr Vulkan path hook + glslang library bundling + SPIR-V cache layer 配置、`handoff-substep-4-3-gamma-prime-prep.md` §5.2 9 task)
- 領域 6 sub-step 6.5 完遂時に handoff doc `handoff-stage-6-complete.md` 作成
- r41 全完遂後の r42-α/β/γ で AYAstorm 改変 11 file SPIR-V port (本領域 6 で確立した LLShaderMgr Vulkan path + descriptor binding pattern + SPIR-V cache layer を流用)

### §6.4 本 sub-doc commit 反映 (case ② re-author 反映)

本 sub-doc re-author commit は AYA 明示指示後 Claude が実施。commit message draft (AYA 指示時 refine 可):

```
docs(r41): sub-doc 06-shader-spirv.md case ② runtime path 反映 re-author

- §1 status active 2026-05-31 re-author、達成条件を LLShaderMgr Vulkan path 経由 runtime SPIR-V 生成 + cache hit/miss 動作へ refine
- §1.2.3 案 ② 採用注記 新規 (旧 build-time pre-compile path vs 新 case ② runtime path mapping table + case-validity 担保)
- §1.1/§1.2.1/§1.2.2/§3.3 AYAstorm 改変 13 file → 11 file (γ'-1 trace 確定値、Cinematic 4 → 2、charter §3 #4 spec drift 反映)
- §2.1 cross compile 順序の決定軸 = LLShaderMgr Vulkan path hook 配置先行 (4 軸化)
- §2.2 dependency graph = LLShaderMgr hook + glslang bundling + cache layer + 試作レール sink (5 階層)
- §3.1 sub-step 6.1 = LLShaderMgr Vulkan path hook + glslang library bundling + SPIR-V cache layer + 3.3-B exemplar 試作レール扱い、6.2/6.3 = runtime SPIR-V 生成 verify、6.5 = 11 file untouched verify
- §4.1 acceptance test procedure を runtime path 反映 (LLShaderMgr 経由 runtime load + cache layer 動作確認、gVK.isEnabled() OFF/ON 両動作)
- §5.1 関連 doc に handoff-substep-4-3-gamma-prime-prep.md 追加
- §6.3 完成宣言条件 = re-author 後 AYA review PASS、§6.4 commit message refresh
- §6.5 改訂履歴 新規追加 (2026-05-28 closed Pattern α 一括 draft → 2026-05-31 case ② re-author 経緯保存)
```

push は AYA 手動 (`feedback_release_flow.md` 遵守)。

### §6.5 改訂履歴

| 日付 | 状態 | 経緯 |
|---|---|---|
| 2026-05-28 | 起草着手 (Pattern α 一括 draft) | AYA + Claude で `01-foundation.md` / `02-portage-execution.md` / `03-state-machine-pso.md` 範式継承、228 file 分類 + 5 sub-step + acceptance 5 件 を Pattern α 一括起草 |
| 2026-05-29 | closed (AYA review PASS) | build-time pre-compile model (CMake glob + glslangValidator 一括 `.spv` 生成 → autobuild package 同梱) 前提で完成宣言 |
| 2026-05-31 | active re-author | sub-step 4.3-γ' 着手で案 A target extension build verify が spec 想定相違 4 点で fail → revert → case ② = runtime SPIR-V 生成 採用 (`handoff-substep-4-3-gamma-prime-prep.md` §1)、本 sub-doc を case ② runtime path へ全面 re-author、AYAstorm 改変 file 13 → 11 (γ'-1 trace 確定値) 反映、charter §3 #4 + §7.5 boundary refine 履歴と同期 |
