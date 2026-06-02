# r41 sub-step 3.3-B 全完遂 → 3.4 着手境界 handoff (2026-05-31)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-B-delta-complete.md` (3.3-B-δ = sky placeholder vert shader 側 UBO/push constant binding 受領 + 計算式配線 完遂 → 3.3-B-ε 着手境界)
**本 handoff 位置付け**: sub-step 3.3-B (shader port、領域 6 並走、α/β-1/β-2/γ/δ 全 sub-step) 完遂宣言 + sub-step 3.4 (texture lifecycle + descriptor set=1 per-material 7 PBR slot + 12 pool hook body PSO bind 配線 + 段階 2 引継ぎ特殊対応) 着手境界。3.3-B の **1 shader exemplar pre-flight** (sky placeholder vert+frag) で SPIR-V build chain (glslangValidator → `.spv` → `loadSpirvShaderModule` → `vkCreateShaderModule` → PSO compile) が確立、3.3-A 確定 二段構え matrix 設計 (push constant 64 B + UBO 448 B) の shader 側受領も配線済 = 領域 6 sub-step 6.1 (248 file 一括 autobuild integration) へ流用可能な pattern が固まった。

---

## 1. sub-step 3.3-B 全完遂 status (2026-05-31)

### 1.1 完遂 marker (sub-doc 03 §3.1.3 sub-step 3.3-B 各 sub-step)

| sub-step | 達成 status |
|---|---|
| **3.3-B-α** (spec refine) | ✓ sub-doc 03 §3.1.3 新規追加 (α/β-1/β-2/γ/δ/ε 細分化表) + §3.1 sub-step 3.3 行 update + sub-doc 06 §1 boundary 整理 (3.3-B = exemplar pre-flight / 6.1 = 一括 integration の役割分担明示) + 設計根拠 trace inventory、commit `0bbfe8319f` (docs only) |
| **3.3-B-β-1** (`loadSpirvShaderModule` signature 追加) | ✓ `VkShaderModule LLVKLoader::loadSpirvShaderModule(const U32* spv_code, size_t code_size_bytes)` signature を `llvkloader.h` に追加 (+21 行) + spec sealed (戻り値 VkShaderModule / 失敗時 VK_NULL_HANDLE / SPIR-V binary は U32 alignment per spec / caller owns + vkDestroyShaderModule で破棄 / PSO compile 後は安全に破棄可) + `make llrender` build PASS (gcc 13.3.0 / no warning / `libllrender.a` link 成功)、commit `4ec234fa23` |
| **3.3-B-β-2** (build chain + transit smoke) | ✓ `AyaShaderCompile.cmake` 新規 (`find_program(AYA_GLSLANG_VALIDATOR glslangValidator)` + `aya_compile_shader_spirv` function、add_custom_command 経由 `-V --target-env vulkan1.3`) + GLSL exemplar (`sky_placeholderV.glsl` / `sky_placeholderF.glsl`) + `aya_r41_shaders` cmake target で `.spv` 生成 (vert 1136 B / frag 384 B) + `viewer_manifest.py` recursive copy で packaged dir 同梱 + `loadSpirvShaderModule()` impl body + beginFrame 初回 1 度限り transit smoke (`.spv` 読込 → `loadSpirvShaderModule()` → 即破棄) + AYA launch PASS (2026-05-30 12:28:48Z) + Vulkan WARN/ERR 0 件 + 3.3-A/3.3-C 12 marker 全継続、commit `5d4999ec4a` (impl) + `6aeeeecf4f` (docs) |
| **3.3-B-γ** (PSO file load 切替 + transit smoke 撤去 + embedded fallback macro guard) | ✓ `createSkySmokePipeline()` の `VkShaderModule` 取得を `loadSpirvShaderModuleFromFile("shaders/aya_r41_exemplar/sky_placeholderV.spv")` + `sky_placeholderF.spv` primary 化 + `#ifdef AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK` で embedded byte array (kSkySmokeVertSpv/kSkySmokeFragSpv) wrap + cmake `if (NOT AYA_GLSLANG_VALIDATOR) target_compile_definitions(llrender PRIVATE AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK)` で fallback 自動 active 化 + beginFrame transit smoke 撤去 + `loadSpirvShaderModule` の `sInitialized` ガード撤去 (initVulkan 内呼出対応) + INFO marker `"Sky smoke PSO compiled via SPIR-V build chain (sub-step 3.3-B-γ)"` + AYA launch PASS (2026-05-31) + Vulkan WARN/ERR 0 件 (38 INFO のみ) + Linux build で `kSkySmokeVertSpv`/`kSkySmokeFragSpv` symbol `libllrender.a` 内 0 件 (dead code 除去確認)、commit `020df91561` (impl) + `5ebfc6d33b` (docs) |
| **3.3-B-δ** (UBO binding + push const + shader 内 MVP/normal/inverse_modelview 計算配線) | ✓ `sky_placeholderV.glsl` に `layout(set=0, binding=0) uniform PerFrameMatrixUBO { mat4 projection_matrix; mat4 inverse_projection_matrix; mat4 identity_matrix; };` + `layout(set=0, binding=1) uniform TextureMatrixUBO { mat4 texture_matrix[4]; };` + `layout(push_constant) uniform PushConstants { mat4 modelview_matrix; };` 配置 + shader 内 `mvp_matrix = projection_matrix * modelview_matrix` / `normal_matrix = transpose(inverse(mat3(modelview_matrix)))` / `inverse_modelview_matrix = inverse(modelview_matrix)` 計算式 + binding witness を 1e-30 multiplier で float32 denormal 以下に圧縮 (gl_Position への寄与 0 を構造的担保) + `.spv` 生成成功 (vert 3268 B = γ 1136 B + 2132 B) + PSO compile 成功 + INFO marker `"Sky placeholder vert binding active (PerFrameMatrixUBO + push constant modelview)"` + AYA launch PASS (2026-05-31) + Vulkan WARN/ERR 0 件 (39 INFO、γ 38 → +1 = δ marker) + 視覚 regression 0、commit `b2c06f869d` (impl) + `2def67b0ef` (docs) |
| **3.3-B-ε** (3.3-B 全完遂 handoff doc 起草) | ✓ 本 handoff doc 作成 (3.3-B 全完遂総括 → 3.4 着手境界) + sub-doc 03 §3.1.3 末尾 + §3.1 sub-step 3.3 行 complete 化 + memory `project_ayastorm_r41_vulkan_migration.md` を 3.4 着手 ready 状態へ update |

### 1.2 acceptance evidence (`~/.ayastorm_x64/logs/AYAstorm.log` 2026-05-31 δ 受入 startup)

| evidence | log line |
|---|---|
| β-2 build chain artefact (γ 経由でも継続) | `.spv` 2 file (vert 3268 B [δ shader] / frag 384 B) が `~/ayastorm/app_settings/shaders/aya_r41_exemplar/` に deploy 済 |
| γ file load 経路実走 | `INFO #Vulkan# llrender/llvkloader.cpp createSkySmokePipeline : Sky smoke PSO compiled via SPIR-V build chain (sub-step 3.3-B-γ)` (line 104) + `SPIR-V shader module loaded size=3268 bytes` (vert) + `SPIR-V shader module loaded size=384 bytes` (frag) |
| δ shader 側 binding 受領 evidence | `INFO #Vulkan# llrender/llvkloader.cpp createSkySmokePipeline : Sky placeholder vert binding active (PerFrameMatrixUBO + push constant modelview)` (line 105) |
| β-2 transit smoke marker 完全消失 (γ で撤去) | `SPIR-V exemplar not found` / `sub-step 3.3-B-β-2 exemplar pre-flight` 0 件 hit |
| 3.3-A/3.3-C 既存 11 marker 全継続 (regression 0) | Vulkan 1.3 dynamicRendering / per-frame desc layout (PerFrameMatrixUBO + TextureMatrixUBO) / Placeholder PSO / bindTarget / flush / syncMatrices PerFrame UBO write / syncMatrices TextureMatrix UBO write / dynamic rendering helper / pushCurrentModelviewMatrix / shutdownVulkan device+instance 全 hit |
| Linux build で embedded byte array dead code 除去継続 | `nm libllrender.a \| grep -cE "kSkySmokeVertSpv\|kSkySmokeFragSpv"` = 0 (γ macro guard 維持) |
| #Vulkan# channel WARN/ERR/failed | 0 hit (release build / validation disabled、transit acceptance、Vulkan INFO 39 件のみ) |
| AYA launch PASS | 起動 → 終了 / regression 0 (login 画面到達 + sky 表示維持 + 視覚 unchanged、AYA 2026-05-31 確認) |

### 1.3 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-3-3-B-delta-complete.md` | **役割完了** (3.3-B-ε 着手 satisfy、本 handoff で 3.3-B 全体引継ぎ) |
| `handoff-substep-3-3-B-gamma-complete.md` | 役割完了済 (3.3-B-δ 着手 satisfy 済) |
| `handoff-substep-3-3-B-beta-2-complete.md` | 役割完了済 (3.3-B-γ 着手 satisfy 済) |
| `handoff-session-pause-2026-05-29-after-3-3-C.md` | 役割完了済 (3.3-B 着手境界 satisfy 済) |
| sub-doc `03-state-machine-pso.md` | active 継続 (§3.1.3 ε 行 complete 化 + §3.1 sub-step 3.3 行 complete 化、3.3-B 全体 close、3.4 着手 ready) |
| sub-doc `06-shader-spirv.md` | active 継続 (3.3-B-α で sub-step 6.1 prereq 整理済、3.3-B で exemplar pre-flight pattern 確立 = 6.1 一括化前の final 形態) |
| `handoff-substep-3-3-B-complete.md` (本 handoff) | 新規作成 (3.3-B 全完遂 → 3.4 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 3.3-B 全完遂 / 3.4 着手 ready 状態へ update) |

---

## 2. 3.3-B 全体実装 summary

### 2.1 関連 commit (時系列)

| commit | sub-step | scope | 変更規模 |
|---|---|---|---|
| `0bbfe8319f` | 3.3-B-α | sub-doc 03 §3.1.3 新規 + sub-doc 06 scope 整理 (exemplar pre-flight vs 6.1 一括 integration boundary) | docs only |
| `4ec234fa23` | 3.3-B-β-1 | `loadSpirvShaderModule` signature 追加 + spec sealed | llvkloader.h (+21) |
| `5d4999ec4a` | 3.3-B-β-2 impl | SPIR-V build chain (cmake module + GLSL exemplar + `aya_r41_shaders` target + `.gitignore`) + `loadSpirvShaderModule` impl body + beginFrame transit smoke | 6 file (+190 / -0) |
| `6aeeeecf4f` | 3.3-B-β-2 docs | β-2 handoff doc | docs only |
| `020df91561` | 3.3-B-γ impl | sky smoke PSO file load 切替 (`loadSpirvShaderModuleFromFile`) + transit smoke 撤去 + embedded fallback macro guard + `sInitialized` ガード撤去 | llvkloader.cpp + CMakeLists.txt (+96 / -67) |
| `5ebfc6d33b` | 3.3-B-γ docs | γ handoff doc | docs only |
| `b2c06f869d` | 3.3-B-δ impl | sky placeholder vert shader 側 UBO/push constant binding 受領 + MVP/normal/inverse_modelview 計算式 + binding witness 1e-30 multiplier + δ INFO marker | sky_placeholderV.glsl + llvkloader.cpp (+50 / -6) |
| `2def67b0ef` | 3.3-B-δ docs | δ handoff doc | docs only |

合計 (impl のみ): **9 file / +357 / -73 line (net +284)**

### 2.2 file 変更 summary (3.3-B 全体累計、impl のみ)

| file | 3.3-B 内累計修正 | 主内容 |
|---|---|---|
| `indra/llrender/llvkloader.h` | +21 行 | β-1 `loadSpirvShaderModule` signature + spec doc comment |
| `indra/llrender/llvkloader.cpp` | +168 / -67 行 (β-2 +80 + γ +87/-67 + δ +1) | β-2 impl body + transit smoke / γ helper追加 + PSO 切替 + macro guard + transit smoke 撤去 + INFO marker generic 化 + sInitialized ガード撤去 / δ INFO marker |
| `indra/llrender/CMakeLists.txt` | +9 行 (γ) | `include(AyaShaderCompile)` + 条件 compile def 配信 |
| `indra/cmake/AyaShaderCompile.cmake` | +50 行 (β-2 new) | `find_program(AYA_GLSLANG_VALIDATOR)` + `aya_compile_shader_spirv` function (add_custom_command 経由 `-V --target-env vulkan1.3`) |
| `indra/newview/CMakeLists.txt` | +23 / -1 行 (β-2) | `include(AyaShaderCompile)` + exemplar 2 file pre-compile wiring + `aya_r41_shaders` target + viewer dep |
| `indra/newview/app_settings/shaders/aya_r41_exemplar/sky_placeholderV.glsl` | +60 / -0 行 (β-2 +17 / δ +49 / -6) | β-2: fullscreen triangle (gl_VertexIndex) / δ: UBO + push constant binding + MVP/normal/inverse_modelview + binding witness |
| `indra/newview/app_settings/shaders/aya_r41_exemplar/sky_placeholderF.glsl` | +16 / -0 行 (β-2 new) | 固定色 (0.4, 0.6, 0.9, 1.0) 出力 |
| `.gitignore` | +4 行 (β-2) | `*.spv` 追加 (build artifact、source tree 内生成だが commit 除外) |
| sub-doc `03-state-machine-pso.md` | §3.1.3 α/β-1/β-2/γ/δ/ε 行 6 件 + §3.1 sub-step 3.3 行 update | sub-step 細分化 + 完遂 marker |
| sub-doc `06-shader-spirv.md` | §1 boundary 整理 (3.3-B exemplar pre-flight / 6.1 一括 integration 役割分担) | scope plan refine |
| `handoff-substep-3-3-B-{β-2,γ,δ,complete}.md` | 各 sub-step 完了境界の handoff doc 4 件 | 段階 boundary 記録 |

### 2.3 主要 API surface (3.3-B 完遂時点、`llvkloader.h` / `llvkloader.cpp` 抜粋)

```cpp
namespace LLVKLoader
{
    // β-1: SPIR-V shader module loader (caller owns return value)
    // - spv_code = U32-aligned binary, code_size_bytes = byte length
    // - return VK_NULL_HANDLE on failure (null/size 0/size % 4 != 0/vkCreateShaderModule fail)
    // - caller responsible for vkDestroyShaderModule
    VkShaderModule loadSpirvShaderModule(const U32* spv_code, size_t code_size_bytes);
}

// γ (anon namespace helper in llvkloader.cpp):
//   VkShaderModule loadSpirvShaderModuleFromFile(const char* rel_path)
//     - resolves via gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, rel_path)
//     - reads binary, validates size > 0 + 4-byte aligned
//     - delegates to LLVKLoader::loadSpirvShaderModule
//     - WARN log on failure path
```

### 2.4 shader exemplar 配置 (`indra/newview/app_settings/shaders/aya_r41_exemplar/sky_placeholderV.glsl` δ 完遂時点)

```glsl
#version 450 core

// PerFrame matrix UBO (set=0, binding 0, 192 B)
layout(set = 0, binding = 0) uniform PerFrameMatrixUBO {
    mat4 projection_matrix;
    mat4 inverse_projection_matrix;
    mat4 identity_matrix;
};

// Texture matrix UBO (set=0, binding 1, 256 B)
layout(set = 0, binding = 1) uniform TextureMatrixUBO {
    mat4 texture_matrix[4];
};

// Push constant (64 B, VERTEX_BIT, mat4 modelview_matrix)
layout(push_constant) uniform PushConstants {
    mat4 modelview_matrix;
};

void main() {
    // δ shader 内計算移譲 3 種 (3.3-A §3.1.1 trace inventory)
    mat4 mvp_matrix              = projection_matrix * modelview_matrix;
    mat3 normal_matrix           = transpose(inverse(mat3(modelview_matrix)));
    mat4 inverse_modelview_matrix = inverse(modelview_matrix);

    // binding witness (全 binding を 1e-30 multiplier で float32 denormal 以下に圧縮)
    vec4 binding_witness = /* projection / inverse_projection / identity / texture_matrix[0] /
                              mvp / normal / inverse_modelview を集約 */;

    // fullscreen triangle (gl_VertexIndex 0/1/2 → (-1,-1) / (3,-1) / (-1,3))
    vec2 pos = /* per-vertex */;
    gl_Position = vec4(pos, 0.0, 1.0) + binding_witness * 1e-30;
}
```

---

## 3. 設計判断履歴 (3.3-B 全体で AYA 確認した分岐)

### 3.1 3.3-B 細分化 (sub-doc 03 §3.1.3)

AYA 確認 (2026-05-30「その細分化案でお願いします、B-α から着手」承認) で α/β-1/β-2/γ/δ/ε の 6 sub-step に分割 (3.3-A/3.3-C pattern 継承)。各 sub-step は完遂境界で能動 handoff timing チェック (`feedback_proactive_handoff.md`)。

### 3.2 β-2: SPIR-V compiler 経路選択 (build-time pre-compile via system glslangValidator)

`shaderc` (runtime compile) ではなく **build-time pre-compile via system glslangValidator** を AYA 採用。理由:
- runtime compile は viewer 起動時間 + shader cache 整合性 (r28 章既存 GL shader cache と並走) 課題を新規発生させる
- build-time は autobuild prebuilt 経由でなく system package (apt: `glslang-tools` / Vulkan SDK) 依存に統一できる
- Mac/Win build host で `glslangValidator` 未 install 時の取扱は γ で macro guard 化 (§3.3 参照)、3 OS build 互換性を維持

### 3.3 γ: embedded byte array 残置 macro guard 化 (Mac/Win fallback safety net)

handoff §5.3 γ-3 で 3 案提示 (完全削除 / macro guard 化 / 完全残置)、AYA 採用 = **macro guard 化**。`#ifdef AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK` で wrap、cmake `if (NOT AYA_GLSLANG_VALIDATOR) target_compile_definitions(llrender PRIVATE AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK)` で fallback 自動 active 化。Linux build (glslangValidator 検出) では macro 未定義 → embedded byte array dead code として binary から除外、Mac/Win build (glslangValidator 不在想定) では macro 定義 → embedded fallback active。3 OS 大前提 (`project_ayastorm_three_platforms.md`、Linux 先行例外適用中) を維持。

### 3.4 γ: sky smoke 1 PSO のみ切替 (placeholder PSO は untouched)

β-2 handoff §5.2/§5.3 では「placeholder + sky smoke 2 PSO」記載だが、sub-doc 03 §3.1.3 B-γ marker は `createSkySmokePipeline()` のみ明示。生成済 GLSL exemplar (`sky_placeholderV.glsl` / `sky_placeholderF.glsl`) は **sky smoke (kSkySmokeVertSpv/kSkySmokeFragSpv) と等価**、`kPlaceholderVertSpv` / `kPlaceholderFragSpv` 等価 GLSL は **存在しない** = 切替不能。AYA 確認 = **sky smoke 1 PSO のみで進める** (sub-doc 03 spec 通り)、placeholder PSO の GLSL exemplar 化は領域 6 sub-step 6.1 一括化担当。

### 3.5 γ: `loadSpirvShaderModule` の `sInitialized` ガード撤去

実装着手時に発見した潜在 bug: `initVulkan` 内 `createSkySmokePipeline` 呼出時点で `sInitialized = false` (関数末尾で true 設定)。`loadSpirvShaderModule` 内 `!sInitialized` 早期 return が PSO compile path を無効化する。修正 = `sInitialized` ガード撤去、`sDevice` null check のみで safety 担保 (Vulkan device 未生成時の不正呼出は引続 block)。

### 3.6 δ: binding witness 1e-30 multiplier 採用 (視覚 regression 0 担保)

δ-2 marker = 「計算式を生成可能な書式で記述」段階 = uniform 値が 0 でも `.spv` compile 成功すれば satisfy。しかし sky smoke PSO は `recordSkySmokeDraw` 経由で実 draw が発生する (fullscreen triangle + sky blue 出力)。binding 値が undefined (descriptor set 未 bind / push constant 未 write) のまま draw した場合、`gl_Position` に undefined 値混入の可能性。

3 案検討で **案 A 採用**: binding 値を `binding_witness` に集約 + `* 1e-30` で float32 denormal 以下に圧縮 = 数学的に gl_Position への寄与 0 を構造的担保、binding declaration は SPIR-V に残置。案 B (output 変数格納) は vert/frag stage interface 変更 = δ scope 違反、案 C (dead branch) は compiler dead code 除去で binding declaration 残置不確実。

### 3.7 δ: TextureMatrixUBO (binding 1) も shader 側受領

handoff §5.2 完了 marker 例示は PerFrameMatrixUBO (binding 0) + push constant の 2 件のみ言及、TextureMatrixUBO (binding 1) は明示なし。scope 行は「set=0 binding 0 = `PerFrameMatrixUBO` 192 B / binding 1 = `TextureMatrixUBO` 256 B」と両 binding を含む = `feedback_no_scope_shrink.md` 遵守で TextureMatrixUBO も shader 側受領実装。

### 3.8 ε: Mac/Win embedded fallback 取扱 = **α 採用 (glslangValidator 必須化)**

δ handoff §3.4 末尾で持越し、ε 着手前に AYA 相談 (2026-05-31)。2 案検討:
- **(α、採用)**: Mac/Win build host で `glslangValidator` install を必須化、`AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK` macro guard は短期 safety net として残置可能 (opt-in)。領域 6 sub-step 6.1 (248 file 全 port) が結局 glslangValidator 必須化を要求する以上、β は重複作業
- **(β、不採用)**: 新 .spv を embedded byte array 化 (xxd 等経由) で更新 = shader 変更毎の二重メンテ、6.1 で 248 file 数百件 embedded array 化は非現実的

AYA 採用根拠: 領域 6 sub-step 6.1 一括化が β の終端でも結局 α 移行を要求 = 短期 patch の重複作業を避ける + 二重メンテ回避 (`feedback_no_dual_doc_split.md` 系思想)。**実 Mac/Win build host での glslangValidator install validation は r42-α (Win) / r42-β (Mac) Vulkan 着手時** に別途、それまでは Linux 先行例外 (`project_ayastorm_three_platforms.md` 適用中) で `AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK` を temporary safety net として残置。

---

## 4. risks / caveats (3.3-B 完遂後の引継ぎ)

### 4.1 実 descriptor set bind / push constant write は領域 7 sub-step 7.5 持越し

3.3-B-δ で sky placeholder vert shader 側 binding は受領済だが、`recordSkySmokeDraw` は `vkCmdBindPipeline` + `vkCmdDraw(3, 1, 0, 0)` のみで `vkCmdBindDescriptorSets` / `vkCmdPushConstants` 呼出なし = **実 descriptor set 0 (PerFrameMatrixUBO/TextureMatrixUBO) 未 bind + push constant (modelview_matrix) 未 write 状態で draw が走る**。validation = disabled (release build) で validation error は出ないが、driver 内部では undefined behavior。δ binding witness 1e-30 multiplier で gl_Position への寄与 0 を担保 = 視覚出力は fullscreen triangle 維持。

実 descriptor set bind / push constant write は **領域 7 sub-step 7.5** (`VkImage`/`VkImageView` 実体作成 + attachment 配線) と合わせて wire up (VMA = 領域 7 sub-step 7.1 前提)、本 3.3-B では PSO compile + shader binding 受領のみ acceptance。

### 4.2 Mac/Win embedded fallback path 動作未検証 (Linux 環境のみ)

`AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK` 定義時の `createSkySmokePipeline` 内 fallback コード path は Linux 環境では macro 未定義のためコンパイル時除外 = ランタイム未実走。コード論理 review レベルでは β-2 の embedded path と同型 (β-2 で動作実証済)。**実 Mac/Win build host での glslangValidator install validation + fallback active 時の PSO compile 成功**は r42-α/β Vulkan 着手時に別途。Linux 環境のみでの 3.3-B-δ verify は本 sub-step scope。

### 4.3 `loadSpirvShaderModule` INFO log noise (領域 6 sub-step 6.1 一括化前に再判断)

3.3-B-δ 完遂時点で `loadSpirvShaderModule` は PSO compile path から 2 件 (sky smoke vert + frag) のみ発火 + transit smoke は撤去済 = 全 log 量は β-2 と同等。**領域 6 sub-step 6.1 一括化時** (248 file 全 port) で全 shader load の INFO 発火 = 数百件規模になるため、6.1 着手前に DEBUG / trace level 降格を検討。

### 4.4 `identity_matrix` 残置可否 (3.3-A 引継ぎ、3.3-B では未決着)

`PerFrameMatrixUBO::identity_matrix` (定数 mat4) は 3.3-A-β-1 で「3.3-B shader trace で削除可否再判断」として残置決定済 (3.3-A handoff §4.4 引継ぎ)。3.3-B-δ exemplar shader では `binding_witness` 集約に組込済 = 削除すると binding declaration が消える → shader 側で identity 直書きできる場合は UBO から外して 64 B 削減候補だが、**判断は領域 6 sub-step 6.1 一括化時** (248 file 全 shader trace で identity 直書き選択肢が evaluated) に持越し。

### 4.5 placeholder PSO の GLSL exemplar 化は 6.1 担当

§3.4 設計判断履歴の通り、placeholder PSO (`createPlaceholderPipeline`) の VkShaderModule source は γ 時点で embedded byte array (`kPlaceholderVertSpv` / `kPlaceholderFragSpv`) のまま (等価 GLSL なし)。**領域 6 sub-step 6.1** (248 file 全 port) でこれも対象、3.3-B では touch せず。

### 4.6 validation strict 確認は sub-step 3.5 に持越し

3.3-B 全体を通して release build (validation = disabled) で transit acceptance。validation strict (extra resources warn / descriptor binding mismatch / push constant range mismatch 等の厳密検証) は sub-doc 03 §3.5 (段階 3 self-check) で別 build により実施。

### 4.7 sub-step 3.4 着手前提条件の整理

3.4 = texture lifecycle + descriptor set=1 per-material 7 PBR slot + 12 pool hook body PSO bind 配線 + 段階 2 引継ぎ特殊対応。3.3-B 完遂で **PSO compile path + shader 側 binding 受領** が揃ったが、3.4 で必要となる以下は未着手:
- VkImage + VkImageView + VMA lifecycle (§1.5.3 bridging item #8 = 領域 7 sub-step 7.1)
- descriptor set=1 per-material 7 PBR slot 配置 (item #7、§1.5.4 device limit 実測値 base で final 化)
- 12 pool hook body 内 `vkCmdDraw*` 投入 (段階 2 hook wiring を body 配線へ)
- terrain glTexGen 廃止 + avatar SSBO 基本実装 (段階 2 引継ぎ特殊対応)

3.4 着手前に **領域 7 sub-step 7.1 (VMA 配置)** との同期確認推奨。

---

## 5. next session entry point

### 5.1 次 session 着手前の準備

1. `git pull origin feature/ayastorm-r41-gl-removal` (AYA push 後の最新取得)
2. 本 handoff doc 通読
3. sub-doc 03 §3.1 sub-step list 3.4 行 + sub-doc 06 / sub-doc 07 の scope plan 再確認
4. memory `project_ayastorm_r41_vulkan_migration.md` status update 反映確認

### 5.2 3.4 着手 scope (sub-doc 03 §3.1 sub-step 3.4)

| 項目 | 内容 |
|---|---|
| **scope** | texture lifecycle (VkImage + VkImageView + VMA、§1.5.3 bridging item #8) + descriptor set=1 per-material 7 PBR slot (DIFFUSE/NORMAL/SPECULAR/BASECOLOR/METALLIC_ROUGHNESS/GLTF_NORMAL/EMISSIVE、binding 0-6) + 12 pool hook body PSO bind + `vkCmdDraw*` 投入 + terrain glTexGen 廃止 + avatar SSBO 基本実装 |
| **対象 file** | `indra/llrender/llimagegl.{cpp,h}` (3,034 LOC) + `indra/newview/lldrawpool*.cpp` 12 file (段階 2 hook wiring → body 配線) + terrain.cpp + avatar.cpp |
| **完了 marker** | VkImage + VkImageView + VMA lifecycle 動作 + descriptor set=1 per-material 7 PBR slot 動作 + 12 pool 全 hook 内 `vkCmdDraw*` 投入動作 + render path GL call 削除 (acceptance #1-段階 2 satisfy = `grep -rE "gl[A-Z][a-zA-Z]+\s*\(" indra/newview/lldrawpool*.cpp` 0 件) + terrain.cpp 内 `glTexGen` 0 件 + avatar.cpp 内 bone matrix → VkBuffer 配線 + validation 0 件 |

### 5.3 3.4 着手前の AYA 確認候補

- **3.4 細分化案** (3.3-A/3.3-C/3.3-B pattern 継承で α/β-1/β-2/γ/δ/ε 化、各 sub-step は単一 facet で粒度過大化防止)
  - 案 1: image lifecycle 先行 (α = spec refine / β = VkImage+VkImageView+VMA / γ = descriptor set=1 binding / δ = 12 pool body 配線 / ε = handoff)
  - 案 2: pool hook body 配線先行 (α = spec refine / β = 12 pool body 配線 [texture binding を VK_NULL_HANDLE で transit] / γ = VkImage 実体 / δ = descriptor set=1 整合 / ε = handoff)
- **領域 7 sub-step 7.1 (VMA) と並走着手か順次着手か** (VMA は 3.4 の image lifecycle 前提)
- **領域 6 sub-step 6.1 (autobuild integration 一括化、248 file 全 port) の並走着手判断** (3.3-B で exemplar pre-flight pattern 確立済、6.1 は 3.4 と独立着手可能 = Agent 並列 cluster 候補)

### 5.4 critical reminders

- **AYAstorm 改変 13 file shader 改変禁止** (charter §2.1 領域 6、`git diff` 0 件維持)、3.4 でも touch しない
- **段階 1 + 段階 2 + 3.1b + 3.2 + 3.3-A + 3.3-C + 3.3-B 動作維持** (sub-doc 03 §3.5、Vulkan path 並走で GL 描画動作維持)
- **3.4 着手時に GL path 残置必須** (sub-doc 03 §3.5、視覚 regression 0 維持、3.3-B で確立した PSO compile path は GL path と並走、実 draw 経路への PSO bind / descriptor set bind / push 投入統合は 3.4 + 領域 7 sub-step 7.5 で順次)
- **identity_matrix 残置可否は領域 6 sub-step 6.1 一括化時 (248 file 全 shader trace 完了時)** に再判断 (3.3-B では決着せず、binding_witness に組込済)
- **embedded fallback macro guard (`AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK`) は r42-α/β Mac/Win Vulkan 着手まで temporary safety net として残置**、α 採用 (glslangValidator 必須化) は Linux 先行例外内での Linux build 方針
- **validation strict は sub-step 3.5 で別 build により実施** (3.4 着手時も release build で AYA launch + WARN/ERR 0 件 transit acceptance)
- **検証用 LL_INFOS hook は出荷物に残さない** (`feedback_remove_verification_logs.md`、3.3-B 全 INFO marker は spec 由来の永続 marker = OK、temp diagnostic hook は別途)
- **3.3-B 完遂で sub-step 3.3 全体 (3.3-A/3.3-B/3.3-C) close**、§3.1 sub-step 3.3 行 complete 化 + 3.4 が次の active row

---

## 6. 領域 6 sub-step 6.1 流用 pattern まとめ (3.3-B exemplar pre-flight → 6.1 一括 integration)

3.3-B で確立した **1 shader exemplar pre-flight pattern** を領域 6 sub-step 6.1 (248 file 全 port + autobuild integration) で一般化する際の流用 element:

| element | 3.3-B-{β-2/γ/δ} 確立 | 6.1 一般化時の流用 |
|---|---|---|
| build chain | `AyaShaderCompile.cmake` の `find_program(AYA_GLSLANG_VALIDATOR glslangValidator)` + `aya_compile_shader_spirv(STAGE INPUT_GLSL OUTPUT_SPV)` function (add_custom_command 経由 `-V --target-env vulkan1.3`) | 248 file 全部に対し `aya_compile_shader_spirv` 呼出 (per-file rule)、`aya_r41_shaders` target を 248 file 全 `.spv` の DEPENDS に拡張 |
| deploy 経路 | `viewer_manifest.py` の `self.path("shaders")` recursive copy が `.spv` 同時拾い (β-2 で確認) | 248 file 全 `.spv` も自動拾われる、追加 install step 不要 |
| file load 経路 | `loadSpirvShaderModuleFromFile(const char* rel_path)` anonymous-namespace helper (γ で追加、`LL_PATH_APP_SETTINGS` 経由 path 解決) | 全 shader load を helper 経由に統一、`llshadermgr.cpp:909-955` の既存 GL compile chain (`glCreateShader` → `glShaderSource` → `glCompileShader`) に SPIR-V 並走分岐挿入 |
| embedded fallback | `#ifdef AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK` macro guard + cmake 条件 def (`if (NOT AYA_GLSLANG_VALIDATOR)`) | α 採用 (glslangValidator 必須化) で 6.1 終端時に Mac/Win 含め全 build host で glslangValidator install 必須化 = macro guard は 6.1 完遂後に廃止候補 |
| shader binding 配置 | `layout(set=0, binding=0) uniform PerFrameMatrixUBO` + `layout(set=0, binding=1) uniform TextureMatrixUBO` + `layout(push_constant) uniform PushConstants { mat4 modelview_matrix; };` | 248 file 全 vert shader で同 binding 受領 (uniform 名は class1/deferred/ 等で既存 uniform 名と整合化必要)、frag shader は texture binding (set=1 per-material 7 PBR slot、3.4 担当) を受領 |
| shader 内計算移譲 | vert shader 内 `mvp_matrix = projection_matrix * modelview_matrix` / `normal_matrix = transpose(inverse(mat3(modelview_matrix)))` / `inverse_modelview_matrix = inverse(modelview_matrix)` | 248 file 全 vert shader で同計算式 (3.3-A §3.1.1 trace inventory 「shader 内計算移譲 3 種」)、既存 uniform `gl_ModelViewProjectionMatrix` / `gl_NormalMatrix` / `gl_ModelViewMatrixInverse` 等の参照を計算式 substitute |
| exemplar 配置 path | `indra/newview/app_settings/shaders/aya_r41_exemplar/` (新規 directory) | 6.1 完遂後は `class1/deferred/` 等正規 path へ移管判断 (sub-doc 03 §3.1.3 設計根拠 trace inventory 末行参照) |
| PSO layout 整合 | 3.3-A γ で確定の `sPlaceholderLayout` / `sSkySmokePipelineLayout` (set=0 desc set layout + push constant range) を 3.3-B-γ/δ で流用 | 3.4 で per-material PSO layout (set=1 binding 0-6) 追加、6.1 完遂時には set=0/set=1/push constant の三層 layout が standard |

---

## 7. 関連 doc / memory cross-ref

### 7.1 関連 doc

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter (3.3-B 完遂で charter §3 #5/#6 段階 3 分の shader port facet pre-flight satisfy)
- `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` — 段階 3 sub-doc (本 handoff で sub-step 3.3-B 全完遂 + §3.1 sub-step 3.3 行 complete 化 + 3.4 着手 ready)
- `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` — 領域 6 sub-doc (3.3-B-α で sub-step 6.1 prereq として整理、3.3-B で exemplar pre-flight pattern 確立 = 6.1 一括化前の final 形態、本 handoff §6 で流用 pattern 明示)
- `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` — 領域 7 sub-doc (実 descriptor set bind / push constant write は 7.5、VMA は 7.1)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-A-complete.md` — 3.3-A 全完遂 handoff (役割完了済、本 handoff §6 で確立 design を継承)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-C-complete.md` — 3.3-C 全完遂 handoff (役割完了済、本 handoff §5 で 3.3-A/3.3-B/3.3-C 全 close 宣言)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-B-delta-complete.md` — 直前完了 handoff (役割完了)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-B-gamma-complete.md` — 役割完了済
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-B-beta-2-complete.md` — 役割完了済

### 7.2 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 handoff 完遂で sub-step 3.3-B 全完遂 / 3.4 着手 ready 状態に update)
- `project_ayastorm_three_platforms.md` — Linux 先行例外を r41 で適用中、Mac/Win embedded fallback 取扱 α 採用 (§3.8) は r42-α/β Vulkan 着手時の glslangValidator 必須化を予定
- `feedback_self_verify_before_handoff.md` — 本 handoff 起草前 self-trace 実施 (§1.1 acceptance × log evidence 各 line 突合、commit hash × sub-step mapping 確認、shader exemplar 配線整合確認)
- `feedback_one_step_at_a_time.md` — 3.3-B 全 sub-step で AYA 1 件確認原則遵守 (α 細分化 / γ macro guard 案 / γ commit 一括 / δ commit 一括 / ε embedded fallback 取扱)
- `feedback_no_auto_commit.md` — 本 handoff doc commit は AYA 明示指示後実施
- `feedback_proactive_handoff.md` — sub-step 3.3-B 全完遂境界での能動 handoff 起草 (本 file)
- `feedback_log_reading.md` — AYA launch 後 Claude が `~/.ayastorm_x64/logs/AYAstorm.log` を grep して全 sub-step marker + Vulkan WARN/ERR 0 件 + 3.3-A/3.3-C 11 marker regression 確認
- `feedback_no_scope_shrink.md` — δ で TextureMatrixUBO (binding 1) を scope に含める判断 (§3.7)、ε で 3.4 着手準備の全要件 (image lifecycle / descriptor set=1 / 12 pool body / 段階 2 引継ぎ 4 件) を縮小せず提示
- `feedback_build_only_verified.md` — 3.3-B 全 sub-step で AYA launch verify PASS 経由でのみ完遂宣言
- `feedback_no_dual_doc_split.md` — ε 採用根拠 (α = glslangValidator 必須化) の二重メンテ回避思想と整合 (§3.8)
