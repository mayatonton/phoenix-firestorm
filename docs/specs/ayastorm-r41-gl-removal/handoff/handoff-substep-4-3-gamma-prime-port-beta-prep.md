# r41 sub-step 4.3-γ'-port-α 完遂後 4.3-γ'-port-β-prep handoff (2026-05-31)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-alpha-complete.md` (sub-step 4.3-γ'-port-α 全完遂宣言 + §6.2 γ'-port-β-1 = SPIR-V 生成成立率実測 task 引継ぎ)
**本 handoff 位置付け**: sub-step 4.3-γ'-port-β 着手前 prep + **(α) 採用境界**。β-1 SPIR-V 生成成立率実測で **構造的 falsification 検出** (sub-doc 06 §1.2.3 case-validity 担保 falsify = LLShaderMgr 加工済み GLSL の Vulkan 解釈成立率 0%、A 195 file 素通り見込 falsify) → AYA 判断 = **(α) base 228 file 全件 3 issue type 別 bundle 構造的書き換え 採用** 2026-05-31 → 旧 cadence (γ'-port-β = PSO 構築 + descriptor 配線) を再設計、新 cadence = γ'-port-β-prep + β-1 macro 切替 + β-2 228 file 並走 patch + β-3 binding 整合 + β-4 PSO 構築 + β-5 exemplar fire + β-6 build + β-7 verify + β-8 commit。spec 改訂 (sub-doc 06 §3.1 sub-step 6.3 scope 拡大 + §1.2.2/§1.2.3 case-validity refine + sub-doc 07 §3.1 sub-step 7.2/7.3/7.4 binding 番号確定 + charter §7.5 boundary refine 履歴) は本 prep doc + 別 spec 改訂 commit で satisfy。

---

## 1. 起草目的 + (α) 採用根拠

### 1.1 起草目的

sub-step 4.3-γ'-port-α 完遂 (commit `ff48b21d88` = LLShaderMgr Vulkan path hook + glslang runtime + SPIR-V cache 案 C 配置、6 file +287/-0) 後の β-1 SPIR-V 生成成立率実測で **構造的 falsification 検出**:

- handoff-substep-4-3-gamma-prime-port-alpha-complete.md §5.2 「LLShaderMgr 加工済み GLSL の Vulkan 解釈成立率は『見込』」 + sub-doc 06 §1.2.3 「LLShaderMgr 加工済み GLSL → glslang runtime でも同分類が成立する見込み (旧 spec 妥当性継承、6.2/6.3 で per-file 検証)」 = 実測で **0% 成立** に falsify
- A 195 file 素通り見込 (sub-doc 06 §1.2.2) は LL 上流 GLSL に対する見立てで、preprocessed GLSL は 0% 通る
- sub-doc 06 §3.1 sub-step 6.3 「B 53 file 修正 (issue type 別 bundle)」scope = 実際は **base 228 file 全件 3 issue type 別 bundle 構造的書き換えが必要**

本 prep doc で (α) 採用境界 (base 228 file 全件 patch + LL_VULKAN_GLSL macro 切替設計 + sub-doc 06/07 spec 改訂 plan + 新 cadence) を確定、AYA review PASS 後 commit + 別 session で sub-step 4.3-γ'-port-β-1 着手 (fresh context 推奨)。

### 1.2 β-1 SPIR-V 生成成立率実測結果 (2026-05-31)

#### 1.2.1 run window + baseline metric

| 項目 | 値 |
|---|---|
| AYAstorm.log run window | 14:21:55Z → 14:23:27Z (~1m32s、AYA 「OK commit して loop 停止」明示指示直前の launch verify run) |
| vulkanDebugCallback ERROR | **0 件** (regression 0) |
| vulkanDebugCallback WARN | **0 件** |
| VK_ERROR | **0 件** |
| `#Vulkan#` marker | **573 件** (前回 4.3-β' baseline 56 件から +517 = SPIR-V 関連 log fire 増加) |
| shutdown clean | Goodbye! → status: stopped ✓ |

#### 1.2.2 SPIR-V cache 生成成立率実測

| metric | 実測 | 期待値 (handoff §5.2 + sub-doc 06 §3.1 sub-step 6.1 完了 marker) |
|---|---|---|
| `~/.ayastorm_x64/cache/shader_cache/*_v.spv` 件数 | **0 件** | ~228 file × 1 stage |
| `~/.ayastorm_x64/cache/shader_cache/*_f.spv` 件数 | **0 件** | ~228 file × 1 stage |
| `~/.ayastorm_x64/cache/shader_cache/*.shaderbin` 件数 (GL binary cache 並存確認) | 223 件 | GL path 完全並走 = ✓ |
| `~/.ayastorm_x64/cache/shader_cache/shaderdata.llsd` | 1 件 (25475 bytes) | metadata = GL 既存 mechanism 流用 ✓ |
| createSPIRVFromGLSL 呼出件数 (= hook 発火件数、`llshadermgr.cpp:560` + `:569` WARN 集計) | **517 件** | hook 発火 ✓ (~228 file × 2 stage 推定範囲内、variant 含む) |
| parse 成功件数 (cache file 件数で間接計測) | **0 件** | A 195 file 素通り見込 (sub-doc 06 §1.2.2) |
| SPIR-V 生成成立率 | **0%** | ≥ 80% 想定 |

#### 1.2.3 parse 失敗の構造的原因 3 issue type

| issue type | sample error log (`createSPIRVFromGLSL`) | LL/FS shader source の構造的特徴 | Vulkan GLSL 仕様要求 |
|---|---|---|---|
| **(a) non-opaque uniform block 外宣言** | `ERROR: 0:35: 'non-opaque uniforms outside a block' : not allowed when using GLSL for Vulkan` | `uniform mat4 model_matrix; uniform vec4 light_color;` のように block 外で direct 宣言 (GL 慣行) | UBO/SSBO block 内宣言必須 (`layout(set=N, binding=M) uniform Block { mat4 model_matrix; vec4 light_color; }` または push_constant)、sampler 等 opaque type は block 外可だが `layout(set=N, binding=M)` 必須 |
| **(b) input/output location 未指定** | `ERROR: 0:35: 'location' : SPIR-V requires location for user input/output` | `in vec3 position_in; out vec4 vertex_color;` のように location 未指定 | 全 in/out qualifier に `layout(location=N)` 必須 (vertex stage の attribute と fragment stage の color attachment + 中間 stage 間 varying 全件) |
| **(c) main entry point 認識失敗** | `ERROR: Linking vertex stage: Missing entry point: Each stage requires one entry point` | (a)+(b) parse error 派生で TShader parse fail → TProgram link 時 entry point 認識失敗 | (a)+(b) 修正後は単独残存しない見込 (parse 通過後の link 成功で自動 satisfy)、念のため main 整合確認は β-2 内で実施 |

#### 1.2.4 LL/FS shader variant 爆発 model 下での 517 件 hook 発火

- LL/FS は `#define HAS_NORMAL_MAP` `WATER_FOG` `HAS_SKIN` 等数十 feature flag × runtime compile で **数百-数千 variant** を生成 (sub-doc 06 §1.2.3 case-validity 担保保存)
- 228 file × 2 stage = 456 を超える 517 件は variant 爆発の合算、hook 発火 path 自体は正常動作 ✓
- (α) 採用後の patch 対象 = **base 228 file**、variant 爆発は preprocessing 経路 (`loadShaderFile()` `#define` 注入) で吸収継続

### 1.3 (α)/(β)/(γ) 検討 table

| 案 | 内容 | parse 通過 | PSO 構築 + descriptor 整合 | r41 完遂への寄与 | scope cost | 採用 |
|---|---|---|---|---|---|---|
| **(γ)** | glslang messages flag 緩和 (`EShMsgVulkanRules` 外して `EShMsgDefault \| EShMsgSpvRules` のみ) | ○ (OpenGL semantic SPIR-V 生成可) | **△ ~ ×** OpenGL implicit binding 規約 = Vulkan descriptor set 構造と不一致、γ'-port-β-3 PSO 構築 + β-4 descriptor 整合 で binding mismatch 大量発生の可能性高 | 短期 PoC のみ、γ'-port-β 本流に届かない可能性高 | 極小 (実装 5 分) | reject |
| **(β)** | glslang OpenGL mode (`setEnvClient(EShClientOpenGL, ...)`) | ○ | △ binding mapping table 必要、SPIRV-Cross 等で Vulkan-compatible 化の追加変換要 | 半ば、本流転換に届くかは追加検証要 | 小 (実装 15 分 + binding mapping 設計) | reject |
| **(α)** | **base 228 file 全件 3 issue type 別 bundle 構造的書き換え + LL_VULKAN_GLSL macro 切替** | **○ (Vulkan 仕様準拠で 100% 成立)** | **○ (Vulkan 仕様準拠で binding 一致、sub-doc 07 §1.2.1 set=0/1/2 直配信)** | **本道、r41-r42 一体で再利用可能、後々確実** | 数日〜数週間 (Agent 並走で短縮可能性) | **採用** (AYA 「α 直接着手でお願いします」承認 2026-05-31) |

### 1.4 (α) 採用根拠 (AYA 「α 直接着手でお願いします」承認 2026-05-31)

- (γ)/(β) は parse 通過の feasibility 確認のみで、γ'-port-β-3 PSO 構築 + β-4 descriptor 整合 で構造的詰みの可能性高
- (α) は Vulkan 仕様準拠で binding 一致 = r41 完遂の本道、後々確実
- AYAstorm 改変 11 file untouched 維持可能 (base 228 file のみ patch、charter §3 #1 acceptance 担保)
- r41 完遂後 r42-α/β/γ で AYAstorm 11 file port 時も同 LL_VULKAN_GLSL macro pattern 流用可能 (charter §2 領域 6 r42 移行 prep)
- LL_VULKAN_GLSL macro 切替で GL/Vulkan 並走 base 維持 = gVK.isEnabled() OFF 時 GL path 完全並走維持 (charter §3 #1 acceptance)

---

## 2. 案 (α) 設計詳細

### 2.1 LL_VULKAN_GLSL macro 切替設計

#### 2.1.1 LLShaderMgr preprocessing 経路に macro 自動注入

`indra/llrender/llshadermgr.cpp` の `loadShaderFile()` preprocessing 経路 (`#version` prepend + `#define` 注入の既存 mechanism) に `gVK.isEnabled()` 時の `#define LL_VULKAN_GLSL 1` 自動注入を追加。

| 注入箇所 | 既存 preprocessing 経路 | 追加内容 (γ'-port-β-1) |
|---|---|---|
| `#version` prepend 直後 | `text[count++] = strdup("#version 410\n");` | `#ifdef LL_VULKAN_GLSL` ガード下で `text[count++] = strdup("#define LL_VULKAN_GLSL 1\n");` (条件 = `LLVKLoader::isVulkanInitialized()`) |
| 注入位置 | `loadShaderFile()` の `#version` block 直後 + 既存 `#define` 注入 block 内 | LLShaderMgr 既存 `defines` map 経由 (`(*defines)["LL_VULKAN_GLSL"] = "1";` を `gVK.isEnabled()` 時 set) も検討候補 (実装簡潔性、γ'-port-β-1 で確定) |

#### 2.1.2 各 shader file 内 conditional 書き換え (sample pattern)

##### (a) uniform block 化 sample (`class1/deferred/diffuseV.glsl` 想定)

```glsl
// 旧 (GL path 互換、現状)
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform sampler2D diffuseMap;

// 新 (LL_VULKAN_GLSL macro 切替、γ'-port-β-2 patch 想定)
#ifdef LL_VULKAN_GLSL
layout(set=0, binding=0) uniform PerFrameUBO {
    mat4 modelview_matrix;
    mat4 projection_matrix;
};
layout(set=1, binding=0) uniform sampler2D diffuseMap;
#else
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform sampler2D diffuseMap;
#endif
```

##### (b) input/output location 化 sample

```glsl
// 旧 (GL path 互換、現状)
in vec3 position_in;
in vec2 texcoord0;
out vec4 vertex_color;

// 新 (LL_VULKAN_GLSL macro 切替、γ'-port-β-2 patch 想定)
#ifdef LL_VULKAN_GLSL
layout(location=0) in vec3 position_in;
layout(location=1) in vec2 texcoord0;
layout(location=0) out vec4 vertex_color;
#else
in vec3 position_in;
in vec2 texcoord0;
out vec4 vertex_color;
#endif
```

##### (c) push_constant for model matrix (γ'-port-β-3 sub-doc 07 §1.2.2 同期、pipeline layout 設計)

```glsl
// 旧 (GL path 互換、現状)
uniform mat4 model_matrix;

// 新 (push_constant 64 bytes、sub-doc 07 §1.2.2 採用、γ'-port-β-3 で binding 整合)
#ifdef LL_VULKAN_GLSL
layout(push_constant) uniform PushConstants {
    mat4 model_matrix;
};
#else
uniform mat4 model_matrix;
#endif
```

#### 2.1.3 macro 切替設計の利点

| 利点 | 詳細 |
|---|---|
| **GL/Vulkan 並走 base 維持** | gVK.isEnabled() OFF 時 = `#define LL_VULKAN_GLSL` 未注入 = GL path 完全並走、charter §3 #1 acceptance 担保 |
| **AYAstorm 改変 11 file untouched 維持** | base 228 file のみ patch 対象、改変 11 file は r42-α/β/γ で同 pattern port |
| **r42 移行 prep** | r41 完遂後 r42 で AYAstorm 11 file port 時に同 LL_VULKAN_GLSL macro 流用 (charter §2 領域 6 r42 移行 prep) |
| **段階 1-4.3-γ'-port-α 動作維持** | GL path 完全並走で過去 sub-step 動作維持、regression 0 担保 |
| **r41 完遂後の simplification 余地** | r41 完遂後 r42-α/β/γ 完遂時に LL_VULKAN_GLSL = 1 default 化 + #else GL path 削除判断 (long-term cleanup) |

### 2.2 binding 番号設計 (sub-doc 07 §1.2.1 同期、γ'-port-β-3 で確定)

sub-doc 07 §1.2.1 descriptor set 3 階層 (set=0 per-frame / set=1 per-material / set=2 per-draw) 構成と整合:

| set | binding 頻度 | 想定 content | descriptor type | 本 prep doc plan |
|---|---|---|---|---|
| **set=0 per-frame** | 1 frame 1 回 | camera UBO (PerFrameUBO) + shadow sampler ×4 + env cubemap sampler ×4 + atmospherics LUT + windlight LUT + AYAstorm picker output buffer | UBO + COMBINED_IMAGE_SAMPLER × ~30 | binding 0 = PerFrameUBO (camera + projection matrix)、binding 1-4 = shadow sampler、binding 5-8 = env cubemap、binding 9-12 = atmospherics/windlight LUT 等、binding 番号 table は γ'-port-β-3 で確定 (sub-doc 07 §3.1 sub-step 7.2 同期) |
| **set=1 per-material** | material 種別ごと | diffuse + normal + specular + AO + emissive + material params UBO | COMBINED_IMAGE_SAMPLER × ~6 + UBO × 1 | binding 0 = MaterialUBO、binding 1 = diffuseMap、binding 2 = normalMap、binding 3 = specularMap、binding 4 = AOMap、binding 5 = emissiveMap、γ'-port-β-3 で sub-doc 07 §3.1 sub-step 7.3 同期 |
| **set=2 per-draw** | draw call ごと | per-object UBO (model matrix は push_constant 分離) + per-draw texture (avatar BoM / attachment) | UBO + COMBINED_IMAGE_SAMPLER (push_descriptor 経由動的更新) | `VK_KHR_push_descriptor` 採用、binding 0 = per-object UBO、binding 1+ = per-draw texture、γ'-port-β-3 で sub-doc 07 §3.1 sub-step 7.4 同期 |
| **push_constant** | per-draw | mat4 modelMatrix (64 bytes) | push_constant | `layout(push_constant) uniform PushConstants { mat4 model_matrix; };`、γ'-port-β-3 で sub-doc 07 §1.2.2 pipeline layout 設計同期 |

### 2.3 228 file 3 issue type 別 bundle 構成 (γ'-port-β-2 並走 cadence)

| issue type | 想定対象範囲 | 対処方針 | Agent 並走可能性 |
|---|---|---|---|
| **(a) uniform block 化 + opaque type binding 化** | 228 file 全件 (vertex stage 主、fragment stage uniform 持ち全件) | sampler 等 opaque type = `layout(set=N, binding=M) uniform sampler2D ...;`、non-opaque type = UBO block (`layout(set=N, binding=M) uniform <BlockName> { ... };`)、conditional 書き換えで GL path 並走 | ○ (file 単位 independent、Agent 並走候補) |
| **(b) input/output location 化** | vertex stage 全件 (in attribute) + 全 stage 間 varying (out/in pair) + fragment stage out (color attachment) | 全 in/out qualifier に `layout(location=N)` 付与、location 番号は file 内連番 (vertex attribute 0,1,2,..., varying 0,1,2,..., fragment out 0)、conditional 書き換え | ○ (file 単位 independent、Agent 並走候補) |
| **(c) main entry point 整合 + 残 parse error 個別対応** | (a)+(b) patch 後 parse 通過確認、残 parse error 個別対応 | (a)+(b) patch 完了後の parse run で残 fail file 抽出、individual 対応 (issue type 4 番手 e.g. precision qualifier / forward declaration / built-in 座標 Y 反転等) | △ (依存性 (a)+(b) 完了後着手、Agent 補助可) |

### 2.4 LL/FS shader variant 爆発 model 下での patch 安全性

- LL/FS は `#define HAS_NORMAL_MAP` 等 feature flag で variant 生成、patch 後の各 variant が parse 通過する保証は β-1 exemplar PoC + β-2 全件並走実 verify で satisfy
- variant 数 ~数百-数千、Agent 並走 + cache layer (γ'-port-α で配置済) で 2 回目以降起動 cache hit による fast iteration 可能
- variant 別 parse fail は (c) issue type 4 番手 個別対応の母数 (β-2 内で識別)

---

## 3. spec 改訂 plan (本 prep doc commit と同時 or 別 commit 想定)

### 3.1 sub-doc 06 §3.1 sub-step 6.3 scope 拡大

| 旧 spec | 新 spec (γ'-port-β-prep 反映) |
|---|---|
| sub-step 6.3 = 「B 53 file 要修正 (issue type 別 bundle、base GLSL 直 patch)」「B 分類 53 file を 4 issue type bundle 化 (built-in 座標 Y 反転 / legacy `attribute|varying` / `#extension GL_ARB_*` / `precision` qualifier) を base GLSL 直 patch」 | sub-step 6.3 = 「**base 228 file 全件** 3 issue type 別 bundle 構造的書き換え + LL_VULKAN_GLSL macro 切替設計」「(a) uniform block 化 + opaque type binding 化 / (b) input/output location 化 / (c) main entry point 整合 + 残 parse error 個別対応 を base GLSL 直 patch、conditional 書き換えで GL path 並走維持」 |

### 3.2 sub-doc 06 §1.2.2 cross compile 分類 refine

| 旧分類 | 新分類 (β-1 実測反映) |
|---|---|
| A: 素通り 195 file (LLShaderMgr 加工済み GLSL → glslang runtime API で 1 pass compile 成功想定、修正不要) | A: 旧分類 falsify。β-1 実測で素通り 0 file (構造的 issue 3 種 = non-opaque uniform block 外宣言 / location 未指定 / main entry point 認識失敗 が全件発生) |
| B: 要修正 53 file (残 15% issue) | B: 旧分類 refine。実際は **base 228 file 全件要修正** (3 issue type 別 bundle 構造的書き換え) |
| AYAstorm 改変 11 file | (unchanged、r42-α/β/γ scope 外、本領域 untouched 維持) |

### 3.3 sub-doc 06 §1.2.3 case-validity 注記 refine

旧 case-validity 担保:
> §1.2.2 A 195 / B 53 / AYAstorm 11 (旧 13) 分類は LLShaderMgr 加工済み GLSL → glslang runtime でも同分類が成立する見込み (旧 spec 妥当性継承、6.2/6.3 で per-file 検証)

新 case-validity (β-1 実測反映):
> β-1 SPIR-V 生成成立率実測 (2026-05-31) で 0% 成立 = 旧見込 falsify。LL/FS shader source の構造的特徴 (non-opaque uniform block 外宣言 / in/out location 未指定 / main entry point 認識依存) が Vulkan GLSL 仕様 (UBO/SSBO block 必須 + layout(location=N) 必須 + entry point 明示) と全件不整合。新 case-validity = **base 228 file 全件 LL_VULKAN_GLSL macro 切替で conditional 書き換え** (GL path 完全並走維持 + Vulkan path 100% 成立)。

(歴史保存: 旧見込 falsify 経緯は §6.5 改訂履歴 table に 2026-05-31 行追加で保存、削除しない、`feedback_falsification_as_progress.md` 範式継承)

### 3.4 sub-doc 07 §3.1 sub-step 7.2/7.3/7.4 binding 番号確定 plan

γ'-port-β-3 着手時に sub-doc 07 §3.1 と同期で binding 番号 table 確定:

- sub-step 7.2 set=0 per-frame: binding 0-12+ (PerFrameUBO + shadow + env cubemap + LUT)
- sub-step 7.3 set=1 per-material: binding 0-5+ (MaterialUBO + diffuse/normal/specular/AO/emissive)
- sub-step 7.4 set=2 per-draw + push_descriptor: binding 0-31 (per-object UBO + per-draw texture)
- push_constant: mat4 modelMatrix (64 bytes)

binding 番号 table は γ'-port-β-3 内で確定、本 prep doc では構造方針のみ。

### 3.5 charter §7.5 boundary refine 履歴追加

| 日付 | refine 内容 |
|---|---|
| 2026-05-31 | (case ② case-validity refute + (α) 採用) β-1 SPIR-V 生成成立率実測で sub-doc 06 §1.2.3 case-validity 担保 falsify (0% 成立)、(α) base 228 file 全件 LL_VULKAN_GLSL macro 切替 + 3 issue type 別 bundle 構造的書き換え 採用、sub-doc 06 §3.1 sub-step 6.3 scope を「B 53 file 修正」→「base 228 file 全件 patch」へ拡大、sub-doc 07 §3.1 sub-step 7.2/7.3/7.4 binding 番号確定 plan 同期 |

### 3.6 spec 改訂 commit 戦略

| 案 | 内容 | pro / con |
|---|---|---|
| **(i)** | 本 prep doc + sub-doc 06/07 + charter 改訂を 1 commit | pro: spec + prep doc 一括反映、scope 整合 / con: commit 大規模、scope misread 時の rollback 大規模 |
| **(ii)** | 本 prep doc commit (1 件) + sub-doc 06/07 + charter 改訂 commit (別 1 件) | pro: prep doc AYA review 経由で spec 改訂方針確定後の安全な spec 改訂、前回 γ'-port-α-prep 範式継承 (commit `2a7e151ffb` = doc-only re-author) / con: commit 2 件 |
| **(iii)** | 本 prep doc commit のみ、sub-doc 06/07 + charter 改訂は sub-step 4.3-γ'-port-β-1 内で同時 commit | pro: 実 patch と spec 改訂を同 commit で整合 / con: prep doc と spec 改訂の時間 lag、AYA review 介在減 |

**推奨**: (ii) (前回 γ'-port-α-prep 範式継承、AYA review 介在で scope 確定、commit 2 件は許容)

---

## 4. AYA 承認境界 (旧境界 unchanged + 本 prep doc 追加項目)

| 承認境界 | 旧境界 | 本 prep doc 追加項目 |
|---|---|---|
| AYAstorm 改変 11 file untouched | charter §3 #4 acceptance、unchanged | (unchanged、(α) 採用後も base 228 file のみ patch、改変 11 file は r42-α/β/γ scope 外維持) |
| GL path 完全並走 | charter §3 #1 acceptance、unchanged | (unchanged、LL_VULKAN_GLSL macro 切替で gVK.isEnabled() OFF 時 GL path 完全並走維持) |
| 段階 1-4.3-γ'-port-α 動作維持 | unchanged | (unchanged、LL_VULKAN_GLSL macro = #ifdef ... #else ... #endif conditional 書き換えで過去 sub-step 動作維持) |
| sub-doc 06 §3.1 sub-step 6.3 scope | 「B 53 file 修正」 | **「base 228 file 全件 patch + LL_VULKAN_GLSL macro 切替設計」** (scope 拡大、spec 改訂で AYA review 経由) |
| sub-doc 07 §3.1 sub-step 7.2/7.3/7.4 binding 番号確定 | 段階 4 sub-step 4.3-δ' 着手時に本格化 | (unchanged、γ'-port-β-3 内で同期確定) |
| charter §7.5 boundary refine | 軽量 3 注記 (前回 γ'-port-α-prep `2a7e151ffb` で確定) | **境界 refine 履歴追加** (2026-05-31 (α) 採用 + case ② case-validity refute + base 228 file 全件 patch scope) |

---

## 5. 新 cadence (sub-step 4.3-γ'-port-β 内 task 分割)

### 5.1 旧 cadence (handoff-substep-4-3-gamma-prime-port-alpha-complete.md §6.2)

| 旧 task | scope |
|---|---|
| γ'-port-β-1 | γ'-port-α SPIR-V 生成成立率実測 |
| γ'-port-β-2 | sub-doc 07 §7.3 PSO 構築経路 trace |
| γ'-port-β-3 | VkPipeline 構築 helper 配置 |
| γ'-port-β-4 | descriptor set 3 階層と VkShaderModule binding 整合確認 |
| γ'-port-β-5 | exemplar PoC = class1/deferred/diffuseV+F.glsl から PSO 1 件構築 + recordPlaceholderPoolDraw 経路 fire 確認 |
| γ'-port-β-6 | incremental autobuild |
| γ'-port-β-7 | AYA launch verify |
| γ'-port-β-8 | commit |

### 5.2 新 cadence ((α) 採用、γ'-port-β-prep + β-1〜β-8 再設計)

| 新 task | scope | 着手 timing | 想定 commit |
|---|---|---|---|
| **γ'-port-β-prep (本 prep doc)** | spec 改訂方針 + (α) 採用境界 + LL_VULKAN_GLSL macro 切替設計 + 228 file 3 issue type 別 bundle cadence + 新 task 分割 | 今 session | doc-only commit 1 件 (本 prep doc 単独) |
| **γ'-port-β-spec-revision** | sub-doc 06 §3.1 sub-step 6.3 scope 拡大 + §1.2.2/§1.2.3 case-validity refine + sub-doc 07 §3.1 sub-step 7.2/7.3/7.4 binding 番号確定 plan 追記 + charter §7.5 boundary refine 履歴追加 | 本 prep doc AYA review PASS 後 (今 session or 別 session) | doc-only commit 1 件 (前回 γ'-port-α-prep `2a7e151ffb` 範式継承) |
| **γ'-port-β-1 (LL_VULKAN_GLSL macro 注入 + exemplar PoC)** | LLShaderMgr `loadShaderFile()` preprocessing 経路に `gVK.isEnabled()` 時 `#define LL_VULKAN_GLSL 1` 自動注入配線 + exemplar 1 file 対 (`class1/deferred/diffuseV.glsl` + `diffuseF.glsl`) の 3 issue type 別 conditional 書き換え + parse 通過実証 (cache file 生成確認 + WARN 0 件 確認) | 別 session (fresh context 推奨、γ'-port-β-spec-revision commit 後) | code commit 1 件 |
| **γ'-port-β-2 (228 file 並走 patch)** | base 228 file 全件 3 issue type 別 bundle 並走 patch (Agent 並走候補)、各 file conditional 書き換え + parse 通過確認 + cache file 生成確認 | β-1 完遂後 (複数 session 想定) | bundle 別複数 commit (issue type 3 bundle × file 群分割) |
| **γ'-port-β-3 (VkPipeline 構築 helper 配置 + binding 番号確定)** | sub-doc 07 §1.2.2 pipeline layout 設計 + §1.2.1 set=0/1/2 binding 番号 table 確定 + LLVKLoader namespace 拡張 (VkPipeline 構築 helper 配置) + sub-doc 07 §3.1 sub-step 7.2/7.3/7.4 同期 | β-2 完遂後 | code commit 1-3 件 (helper 配置 + binding table 確定) |
| **γ'-port-β-4 (descriptor set 3 階層 配線 + push_descriptor 配線)** | sub-doc 07 §3.1 sub-step 7.2 set=0 + 7.3 set=1 + 7.4 set=2 push_descriptor 配線、生成 VkShaderModule と shader 側 layout(set=N, binding=M) 整合確認 | β-3 完遂後 | code commit 1-2 件 |
| **γ'-port-β-5 (exemplar PoC fire 接続)** | exemplar 1 file 対から PSO 1 件構築 + recordPlaceholderPoolDraw 経路で fire 確認 + descriptor set bind 動作確認 + validation 0 件 確認 | β-4 完遂後 | code commit 1 件 |
| **γ'-port-β-6 (incremental autobuild)** | autobuild configure + build + package + install + cache clear | β-5 完遂後 (β-1〜β-5 各 task 内でも incremental build) | (build artifact、commit なし) |
| **γ'-port-β-7 (AYA launch verify)** | AYA launch + log + cache file 集計 + validation 確認 + regression 0 確認 | β-6 完遂後 | (AYA review、commit なし) |
| **γ'-port-β-8 (commit 統合 + handoff)** | β-1〜β-5 commit 整合確認 + handoff-substep-4-3-gamma-prime-port-beta-complete.md 起草 + commit | β-7 PASS 後 | doc-only commit 1 件 (handoff doc) |

### 5.3 cadence 採用根拠

| 採用根拠 | 詳細 |
|---|---|
| **前回 γ'-port-α-prep 範式継承** | commit `2a7e151ffb` (prep doc + spec re-author 同 commit) → commit `ff48b21d88` (実装) の 2 段 cadence と整合、今回も prep + spec-revision + 実装 を分割 |
| **(α) scope 大規模対応** | 228 file 全件 patch = Agent 並走 + bundle 別 commit で feasibility 担保、β-2 単独で複数 session 想定 |
| **AYA review 介在の安全性** | spec 改訂 (sub-doc 06 §3.1 6.3 scope 拡大 + §1.2.2/§1.2.3 case-validity refine) は AYA review 経由で scope misread 防止 |
| **fresh context 推奨** | (α) 大規模 task は β-1 単独でも中規模 trace + 実装、context 周回境界で能動 handoff 範式遵守 (`feedback_proactive_handoff.md`) |

### 5.4 (α) 採用後の総 cost 見積

| 項目 | 想定 cost |
|---|---|
| γ'-port-β-prep + spec-revision commit | 1 session 内 (本 session) |
| γ'-port-β-1 (LL_VULKAN_GLSL macro + exemplar PoC) | 1 session (fresh context、中規模 trace + 実装) |
| γ'-port-β-2 (228 file 並走 patch) | 数 session 〜 1-2 週間 (Agent 並走で短縮可能性、issue type 3 bundle × file 群分割) |
| γ'-port-β-3 (VkPipeline + binding) | 1-2 session |
| γ'-port-β-4 (descriptor 配線) | 1 session |
| γ'-port-β-5〜β-8 (exemplar fire + build + verify + commit + handoff) | 1 session |
| **計** | **~1 ヶ月** (Agent 並走 + LL_VULKAN_GLSL macro 切替で AYA 11 file untouched 維持下) |

---

## 6. risks / caveats

### 6.1 228 file 全件書き換え scope 爆発

- LL/FS upstream fork 困難化 (AYAstorm 独自 base 228 file fork として r41 完遂、r42 以降 LL/FS upstream merge は別 path 検討)
- Agent 並走 + LL_VULKAN_GLSL macro 切替 + conditional 書き換え (GL path 並走維持) で fork 影響を最小化、upstream merge 時は LL_VULKAN_GLSL 部分の手動 conflict resolution が必要

### 6.2 binding 番号設計 mismatch リスク

- sub-doc 07 §1.2.1 set=0/1/2 構成と 228 file 全件の uniform/in/out 数の整合確認
- shader file 別 binding 番号 table 必須 (γ'-port-β-3 内で並走管理、Agent 並走時の整合確認は β-2 → β-3 boundary で AYA review 介在)
- 衝突回避 = binding 番号 namespace 階層化 (set 別 binding 0-31 範囲、push_descriptor binding ≤ 32 制約 sub-doc 07 §1.2.1 厳守)

### 6.3 LL_VULKAN_GLSL macro 切替の維持コスト

- GL/Vulkan 並走 base 維持は long-term maintenance cost +
- r41 完遂後 r42-α/β/γ 完遂時に LL_VULKAN_GLSL = 1 default 化 + #else GL path 削除判断 (long-term cleanup、charter §1 acceptance #1 GL 依存除去と整合)

### 6.4 AYAstorm 改変 11 file untouched 維持の境界

- LL_VULKAN_GLSL macro 切替設計は base 228 file のみ適用、改変 11 file は r42-α (picker 2) / r42-β (Cinematic 2) / r42-γ (visual realism 7) で同 pattern port
- r41 内では改変 11 file の Vulkan path 動作は不要 (gVK.isEnabled() ON 時も 11 file は GL path のまま動作)、charter §3 #4 acceptance 担保

### 6.5 LL/FS shader variant 爆発 model 下での patch 安全性

- variant 数 ~数百-数千、patch 後の各 variant が parse 通過する保証は β-1 exemplar PoC + β-2 全件並走実 verify で satisfy
- variant 別 parse fail は (c) issue type 4 番手 個別対応の母数 (β-2 内で識別)
- variant 爆発の preprocessing 経路 (`#define HAS_NORMAL_MAP` 等 feature flag 注入) は LL_VULKAN_GLSL macro 切替と独立して動作 (`loadShaderFile()` 既存 mechanism untouched)

### 6.6 context budget concern

- 本 prep doc 起草 + AYA review + commit は今 session
- sub-step 4.3-γ'-port-β-1 (LL_VULKAN_GLSL macro 注入 + exemplar PoC) は **別 session (fresh context 推奨)**、中規模 trace + 実装想定
- sub-step 4.3-γ'-port-β-2 (228 file 並走 patch) は **複数 session 想定** (Agent 並走 + bundle 別 commit)
- 各 sub-step 完遂時に handoff doc proactive 起草 (`feedback_proactive_handoff.md`)

### 6.7 spec 改訂 scope misread リスク

- sub-doc 06 §3.1 sub-step 6.3 scope 拡大 (「B 53 file 修正」→「base 228 file 全件 patch」) は AYA review 経由で確定
- §1.2.2/§1.2.3 case-validity refine も同 review 介在
- 歴史保存 = 旧 case-validity 担保 + 旧 cadence は §6.5 改訂履歴 + 削除しない (歴史抹消回避、`feedback_falsification_as_progress.md` 範式継承)

### 6.8 case ② path 採用継続 (case ① 復活なし)

- (α) 採用は case ② path (LLShaderMgr Vulkan path 経由 runtime SPIR-V 生成) を維持しつつ、shader source 側を Vulkan GLSL 仕様準拠へ書き換える approach
- case ① (build-time pre-compile + autobuild bundle) 復活は不要 (case ① でも同 source 問題で同 fail、結局 (α) source patch が必要)
- case ② runtime path + LL_VULKAN_GLSL macro 切替 + SPIR-V cache layer 案 C (γ'-port-α `ff48b21d88` 配置済) = γ'-port-α の infrastructure は全て継承、(α) は shader source 側のみ拡張

---

## 7. next action

### 7.1 着手前 cadence (AYA review)

1. AYA さん review 本 prep doc (§1 起草目的 + (α) 採用根拠 / §2 案 (α) 設計詳細 / §3 spec 改訂 plan / §4 AYA 承認境界 / §5 新 cadence / §6 risks/caveats)
2. AYA さん「OK」承認下で commit (本 prep doc 1 件 doc-only commit)
3. spec 改訂 commit (sub-doc 06/07 + charter refine、別 commit 1 件、前回 γ'-port-α-prep `2a7e151ffb` 範式継承) は本 prep doc commit 後の同 session or 別 session で実施 (AYA 指示)
4. **別 session (fresh context 推奨)** で sub-step 4.3-γ'-port-β-1 着手 (LL_VULKAN_GLSL macro 注入 + exemplar PoC、AYA 明示指示要)

### 7.2 sub-step 4.3-γ'-port-β-1 着手 task 候補 (次 session、参考)

| task | 詳細 |
|---|---|
| β-1-1 | LLShaderMgr `loadShaderFile()` preprocessing 経路の `defines` map 経由で `gVK.isEnabled()` 時 `(*defines)["LL_VULKAN_GLSL"] = "1";` 配線 (実装簡潔性) or `#version` 直後の `text[count++] = strdup("#define LL_VULKAN_GLSL 1\n");` 明示注入 (どちらも候補、β-1 内で確定) |
| β-1-2 | exemplar `class1/deferred/diffuseV.glsl` の 3 issue type 別 conditional 書き換え (uniform block 化 + location 化 + main 整合) |
| β-1-3 | exemplar `class1/deferred/diffuseF.glsl` の同等 patch |
| β-1-4 | incremental autobuild (configure + build) |
| β-1-5 | AYA launch verify + `~/.ayastorm_x64/cache/shader_cache/<hash>_v.spv` + `_f.spv` 2 file 生成確認 + log "createSPIRVFromGLSL" WARN 0 件確認 (exemplar 2 file 限定) + validation 0 件確認 |
| β-1-6 | commit (sub-step 4.3-γ'-port-β-1 単独 commit) |

### 7.3 critical reminders

| reminder | 詳細 |
|---|---|
| **AYAstorm 改変 11 file shader 改変禁止** | sub-doc 06 §3 範囲 = base 改変なし port のみ、改変 11 file (picker 2 / Cinematic 2 / visual realism 7) は r42-α/β/γ scope 外維持 |
| **段階 1-4.3-γ'-port-α 動作維持** | GL path 完全並走 (gVK.isEnabled() OFF 時)、LL_VULKAN_GLSL macro 切替で過去 sub-step 動作維持 |
| **case ② path + GL path 完全並走** | charter §3 #1 acceptance = `LLVKLoader::isVulkanInitialized()` OFF 時 GL path 完全動作維持、ON 時 両 path 並走 |
| **LL_VULKAN_GLSL macro 切替** | base 228 file のみ patch 対象、conditional 書き換え (`#ifdef LL_VULKAN_GLSL ... #else ... #endif`) で GL/Vulkan 並走維持 |
| **case ② case-validity refute 歴史保存** | β-1 実測で旧 case-validity falsify 経緯は §6.5 改訂履歴 table に 2026-05-31 行追加で保存、削除しない (`feedback_falsification_as_progress.md` 範式) |
| **AYA 承認境界遵守** | sub-doc 06 §3.1 sub-step 6.3 scope 拡大 + sub-doc 07 §3.1 sub-step 7.2/7.3/7.4 binding 確定 plan は spec 改訂で AYA review 経由 |
| **案 B cadence 継承** | measurement log 配線 skip + AYA 短評承認で satisfy |
| **context budget proactive 監視** | (α) 大規模 task は β-1 から複数 session 想定、各 sub-step 完遂時に handoff doc proactive 起草 |
| **proactive handoff 範式** | sub-step 4.3-γ'-port-β-1 完遂時 + β-2 bundle 完遂時 + β-3/-4/-5 完遂時の各境界で handoff 起草 (`feedback_proactive_handoff.md`) |
| **defer / disable 提案 ban** | `feedback_self_bug_no_defer_option.md` 遵守、(α) 採用後の patch 失敗 file は GL fallback で動作維持 (構造的並走)、defer/disable 選択肢として並べない |

### 7.4 commit 戦略

- 本 prep doc 単独 commit = 1 件 (doc-only)、AYA 「OK」承認下実施
- spec 改訂 commit (sub-doc 06/07 + charter refine) = 1 件 (前回 γ'-port-α-prep `2a7e151ffb` 範式継承)
- 実 patch commit (sub-step 4.3-γ'-port-β-1〜β-5) = task 別 (β-1 単独 + β-2 bundle 別複数 + β-3/-4/-5 各 1 件想定)
- handoff doc commit (sub-step 4.3-γ'-port-β-8) = 1 件 (doc-only)
- 計 ~10 commit 想定 (Agent 並走 + bundle 別分割)

---

## 8. 関連 doc / memory cross reference

### 8.1 関連 doc

| doc | 役割 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/00-charter.md` | charter §2 領域 6 + §3 #4 + §7.5 boundary refine 履歴 (本 prep doc 採用後の (α) 採用 + case ② case-validity refute 行追加 plan) |
| `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` | sub-doc 03 §3.1.3 役割再定義注記 (3.3-B exemplar = 試作レール扱い、unchanged) |
| `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` | **sub-doc 06 §3.1 sub-step 6.3 scope 拡大 + §1.2.2/§1.2.3 case-validity refine plan** (本 prep doc §3.1-§3.3) |
| `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` | sub-doc 07 §3.1 sub-step 7.2/7.3/7.4 binding 番号確定 plan (γ'-port-β-3 内同期、本 prep doc §3.4) |
| `docs/specs/ayastorm-r41-gl-removal/08-llvkrenderer-skeleton.md` | sub-doc 08 LLVKRenderer skeleton (sub-step 4.4 part B、namespace LLVKLoader 経路継承、unchanged) |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-alpha-complete.md` | **前 handoff (役割完了 = §6.2 γ'-port-β-1 実測で構造的 falsification 検出、本 prep doc で内容引継ぎ)** |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-prep.md` | (本 prep doc、γ'-port-β 着手境界) |

### 8.2 関連 memory

| memory | 役割 |
|---|---|
| `project_ayastorm_r41_vulkan_migration.md` | active milestone tracking、本 prep doc 完遂後の next action update plan = sub-step 4.3-γ'-port-β-prep 完遂 → spec 改訂 commit → fresh context で γ'-port-β-1 着手 |
| `project_build_procedure.md` | autobuild fullflow + .venv activate + AUTOBUILD_VARIABLES_FILE |
| `feedback_proactive_handoff.md` | context 圧迫時の proactive handoff 範式 (本 prep doc 起草 = 範式遵守) |
| `feedback_self_verify_before_handoff.md` | handoff 起草前の self-trace 義務 (本 prep doc = β-1 実測 self-trace 完遂後の起草) |
| `feedback_no_claude_coauthor.md` | commit message Co-Authored-By: Claude 禁止 |
| `feedback_proactive_diagnostic.md` | log/grep/gdb 系は Claude が直接実行 (本 β-1 実測 = Claude 直接 ls/grep 実行) |
| `feedback_log_reading.md` | log 解析は Claude 側、AYA に貼り付けさせない (本 β-1 = Claude grep 集計) |
| `feedback_one_step_at_a_time.md` | 1 メッセージ 1 アクション cadence |
| `feedback_use_agents_proactively.md` | 重い trace は agent 使用 (γ'-port-β-2 228 file 並走 patch で Agent 活用候補) |
| `feedback_remove_verification_logs.md` | structural log (cache hit/miss + filename + parse fail WARN) は scope 外 = 除去対象なし |
| `feedback_no_auto_commit.md` | コミットは明示指示後 (本 prep doc commit は AYA 「OK」承認下) |
| `feedback_no_scope_shrink.md` | (α) 採用 = 228 file 全件 patch、scope shrink 禁止 (例: 100 file のみ patch + 残 GL fallback、は禁止) |
| `feedback_self_bug_no_defer_option.md` | (α) 採用後の patch 失敗 file への defer/disable 提案禁止 = GL fallback (構造的並走) で動作維持 |
| `feedback_falsification_as_progress.md` | β-1 実測で旧 case-validity 担保 falsify = 生き残りルート絞り込みの成果として記録、(α) 採用根拠 = falsification 積上げ |
| `feedback_doubt_self_first.md` | β-1 実測 = case ② case-validity 担保を AYA 提示の前に Claude 自身で疑った結果 (handoff §5.2 「見込」段階明示) |
| `feedback_admit_unknown.md` | 解釈成立率 0% = 推論ではなく実測で確定 (handoff §5.2 「実測手段 = 次 session 起動後 shader_cache file 数 + log 集計」明示) |

---

**本 prep doc 起草日**: 2026-05-31
**起草根拠**: handoff-substep-4-3-gamma-prime-port-alpha-complete.md §6.2 γ'-port-β-1 = SPIR-V 生成成立率実測 task で構造的 falsification 検出 → AYA 「α 直接着手でお願いします」承認 2026-05-31 → 前回 4.3-β-prep / γ'-port-α-prep 範式継承で prep doc 起草 + spec 改訂方針 + 新 cadence 確定
**次 session 着手**: sub-step 4.3-γ'-port-β-1 (LL_VULKAN_GLSL macro 注入 + exemplar PoC 2 file 書き換え + parse 通過実証) (fresh context 推奨、AYA 明示指示要)
