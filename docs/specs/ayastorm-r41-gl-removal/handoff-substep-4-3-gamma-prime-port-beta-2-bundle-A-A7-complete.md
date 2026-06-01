# r41 sub-step 4.3-γ'-port-β-2-bundle-A-A7 完遂 → bundle-A 全体完遂境界 + bundle-B/C scope 着手境界 handoff (2026-06-01)

**parent commit**: `862f7dc8bd` (A7 patch、本 handoff の直接 parent) / `b80d90bea4` (A6-complete handoff doc commit 範式継承元 source-of-truth)
**HEAD**: `862f7dc8bd` on `feature/ayastorm-r41-gl-removal`
**本 doc 位置付け**: A7 完遂状態 + bundle-A 全 7 sub-bundle 完遂境界 + bundle-B/C scope 着手境界 を fresh context 引継 用に確定する doc-only handoff。A6-complete `b80d90bea4` 範式継承。**A5 §6.3 範式 literal 例外 (案 D = canonical 拡張 + 既存 UBO 末尾追加 with std140 末尾追加で ABI 互換) を本 A7 で明示確立**。AYA 「OK commit して」明示承認下で commit (no auto-commit)。

---

## §1 起草目的

β-2-bundle-A scope 中 A7 (bundle-A 残 sub-bundle 7/7 = remaining/cleanup) を 25 file に注入完遂した状態を確定し、bundle-A 全 7 sub-bundle (A1-A7 + A8-recovery) 完遂境界 + bundle-B/C scope 着手境界 を fresh context に引継ぐ。A1 漏れ candidate (screen_res 15 file + env_mat 4 file) + canonical 補正対象 (haze_horizon 2 file + gamma 3 file + metallicFactor/roughnessFactor 2 file + emissiveMap 1 file = 13 file、3 file 重複統合) を 25 unique file に補正。**案 D = canonical 拡張 + 既存 UBO 末尾追加 範式を本 A7 で確立** (A5 §6.3 「既処理 file UBO untouched」原則を std140 末尾追加 ABI 互換証明により例外化、case 3 解決手段として canonical 拡張採用可)。FrameAtmosphere canonical 拡張は 5 file のみ patch (haze_horizon/gamma 参照する file のみ、他 41 file は意図的 untouched で trailing bytes として functional 互換)。**binding category -30 = A7 patches 効果完全直接観測** (A1-A6 全 sub-bundle 中で最大の直接観測 delta)。bundle-A 全体 metric は集合的閾値設計通り controlled improvement (net delta -10 errors)。

---

## §2 A7 完遂 status

| 項目 | 値 |
|---|---|
| Scope | bundle-A 残 cleanup (screen_res A1 漏れ + canonical 補正対象 漏れ entity) |
| 注入 file 数 | **25 file** (unique、Section A 15 + B 4 + C 2 + D 3 + E 2 + F 1、3 file 重複統合 = postDeferredGammaCorrect A+D / pbralphaF E+F) |
| 変更行数 | **+367 / 0** (insertions-only、deletion 0 件) |
| 3-段 swap pattern balanced | 全 file `#ifdef LL_VULKAN_GLSL` / `#else` / `#endif` count 一致 (file 内 nested #ifdef LEGACY_GAMMA/GAMMA_CORRECT/HAS_NOISE/IS_HUD/HAS_SUN_SHADOW 差異あり = 正常) |
| variant pattern `defined(LL_VULKAN_GLSL)` | **0 件** |
| Agent 投入 | 4 Agent 並列 scan + 1 Agent 単独 patch (合計 5 Agent) |
| A1/A2/A3/A4/A5/A6/A8-recovery 既処理 file untouched (既存 block) | **違反 0 件** (std140 末尾追加 ABI 互換例外を除く全 UBO/sampler 行 diff 0、案 D 末尾追加のみ独立例外) |
| skip list 13 file untouched | **全 untouched** (Picker 2 + Cinematic BD 2 + Visual Realism 7 + Exemplar 2 = 13) |
| A2 拡張 skip 2 file untouched | **全 untouched** (previewV + multiPointLightF) |
| AYA cold cache launch verify | **PASS** (起動成立 06:19:14 → 06:20:18 ~64 秒 + clean shutdown + GL `.shaderbin/shader_cache` 224 件再生成 + crash 0 + GL shader compile/link fail 0) |
| commit | `862f7dc8bd` (AYA 「OK commit して」明示指示下) |
| metric vs A6 baseline binding | **-30 ✓ A7 patches 効果完全直接観測** = canonical 漏れ entity 補正 → binding error 全消滅 |
| metric net delta | **-10 errors** (binding -30 + location +24 cascade + non-opaque -4 + missing #endif ±0) = controlled improvement |

### §2.1 patch Section 別内訳

| Section | scope | binding 番号 | file 数 | file list |
|---|---|---|---|---|
| **A** | screen_res FrameViewProj UBO 新規/末尾追加 | `set=0/binding=0` | **15** | motionBlurF + postDeferredGammaCorrect + postDeferredNoDoFF + postDeferredNoTCV + postDeferredV + rlvF + tonemapUtilF + glowExtractF (HAS_NOISE 内) + pbrmetallicroughnessF + glowcombineFXAAF + exoPostBaseV + exoVignetteF + snapshotFrameF + softenLightV + sunLightV |
| **B** | env_mat FrameViewProj UBO 末尾追加 / bare wrap | `set=0/binding=0` | **4** | skyF + reflectionProbeF (class2) + reflectionProbeF (class3) + pointLightF (bare env_mat[3] のみ `#ifndef` wrap、既 FrameViewProj UBO 保全) |
| **C** | haze_horizon FrameAtmosphere canonical 拡張 案 D | `set=0/binding=2` | **2** | cloudsV + skyV |
| **D** | gamma FrameAtmosphere canonical 拡張 案 D (conditional 内) | `set=0/binding=2` | **3** | CASF (LEGACY_GAMMA 内) + postDeferredTonemap (GAMMA_CORRECT 内) + postDeferredGammaCorrect (Section A 統合) |
| **E** | metallicFactor/roughnessFactor MaterialUBO canonical 拡張 案 D (dual block IS_HUD) | `set=1/binding=0` | **2** | pbralphaF + pbropaqueF |
| **F** | emissiveMap binding=5 case 1 漏れ | `set=1/binding=5` | **1** | pbralphaF (Section E 統合) |
| | | | **unique 25** | (重複統合 postDeferredGammaCorrect A+D / pbralphaF E+F = +28 raw → 25 unique) |

### §2.2 既処理 file への追加注入 内訳 (案 D 末尾追加 ABI 互換例外)

A7 entity (screen_res / env_mat / haze_horizon / gamma / metallicFactor / roughnessFactor / emissiveMap) を 1 件以上参照する既処理 file は追加注入対象。既存 UBO block は std140 末尾追加例外で touch。

| file | 既処理 sub-bundle | A7 追加注入 |
|---|---|---|
| `class1/deferred/pbropaqueF.glsl` | A4 (dual IS_HUD MaterialUBO 2 block) + A5 (extension sampler) + A6 (PerDrawUBO clipPlane) | MaterialUBO 末尾 metallicFactor + roughnessFactor (dual block) |
| `class2/deferred/pbralphaF.glsl` | A4 (dual IS_HUD MaterialUBO 2 block) + A5 (extension sampler) | MaterialUBO 末尾 metallicFactor + roughnessFactor (dual block) + emissiveMap binding=5 |
| `class1/deferred/skyF.glsl` | A1 (FrameViewProj UBO) + A2 (FrameAtmosphere UBO) + A3 (sampler) + A5 (extension sampler) | FrameViewProj UBO 末尾 env_mat (Section B、FrameAtmosphere は haze_horizon/gamma 参照外 = untouched) |
| `class1/gltf/pbrmetallicroughnessF.glsl` | A4 (SSBO exclusion、sampler のみ) + A5 (extension sampler) + A6 (PerDrawUBO clipPlane) | FrameViewProj UBO 新規 (case 1(b) screen_res) |
| `class2/deferred/reflectionProbeF.glsl` | A3 (sampler cluster) + A5 (extension sampler + Probe binding=40-42) | FrameViewProj UBO 末尾 env_mat (Section B) |
| `class3/deferred/reflectionProbeF.glsl` | A3 (sampler cluster) + A5 (extension sampler + Probe binding=40-42) + A6 (PerDrawUBO clipPlane) | FrameViewProj UBO 末尾 env_mat (Section B) |
| `class3/deferred/pointLightF.glsl` | A6 (PerDrawUBO ε color/size) + 既 FrameViewProj UBO (canonical mat3 env_mat 含む) | bare `uniform vec3 env_mat[3]` (legacy GL form) のみ `#ifndef LL_VULKAN_GLSL` wrap (既 FrameViewProj UBO 保全) |
| `class1/deferred/cloudsV.glsl` | A2 (FrameAtmosphere UBO) | FrameAtmosphere 末尾 haze_horizon (案 D 拡張) |
| `class1/deferred/skyV.glsl` | A2 (FrameAtmosphere UBO) | FrameAtmosphere 末尾 haze_horizon (案 D 拡張) |
| `class1/deferred/postDeferredGammaCorrect.glsl` | A2 (FrameAtmosphere UBO) | Section A (FrameViewProj UBO screen_res) + Section D (FrameAtmosphere 末尾 gamma 案 D 拡張) |
| `class1/deferred/CASF.glsl` | A2 (FrameAtmosphere UBO) | FrameAtmosphere 末尾 gamma (LEGACY_GAMMA 内、案 D 拡張) |
| `class1/deferred/postDeferredTonemap.glsl` | A2 (FrameAtmosphere UBO) | FrameAtmosphere 末尾 gamma (GAMMA_CORRECT 内、案 D 拡張) |

### §2.3 新規注入 13 file (untouched から初注入)

screen_res 漏れ + 案 D 拡張対象外の新規 13 file (重複統合除く 25 - 12 = 13)。下記は FrameViewProj UBO 新規注入 (case 1(b))。

| file | A7 注入 |
|---|---|
| `class1/deferred/motionBlurF.glsl` | FrameViewProj UBO 新規 (screen_res 含 canonical 形式) |
| `class1/deferred/postDeferredNoDoFF.glsl` | FrameViewProj UBO 新規 (screen_res) |
| `class1/deferred/postDeferredNoTCV.glsl` | FrameViewProj UBO 新規 (screen_res) |
| `class1/deferred/postDeferredV.glsl` | FrameViewProj UBO 新規 (screen_res) |
| `class1/deferred/rlvF.glsl` | FrameViewProj UBO 新規 (screen_res) |
| `class1/deferred/tonemapUtilF.glsl` | FrameViewProj UBO 新規 (screen_res) |
| `class1/effects/glowExtractF.glsl` | FrameViewProj UBO 新規 (HAS_NOISE 内、screen_res) |
| `class1/interface/glowcombineFXAAF.glsl` | FrameViewProj UBO 新規 (screen_res) |
| `class1/post/exoPostBaseV.glsl` | FrameViewProj UBO 新規 (screen_res) |
| `class1/post/exoVignetteF.glsl` | FrameViewProj UBO 新規 (screen_res) |
| `class1/post/snapshotFrameF.glsl` | FrameViewProj UBO 新規 (screen_res) |
| `class2/deferred/softenLightV.glsl` | FrameViewProj UBO 新規 (case 1(b)、A1-A6 touch 履歴 0 件確認) |
| `class2/deferred/sunLightV.glsl` | FrameViewProj UBO 新規 (screen_res) |

---

## §3 binding rule §2 補正 literal canonical (A7 で確定)

### §3.1 案 D = canonical 拡張 + 既存 UBO 末尾追加 範式 (A7 で確立)

**設計原則**: canonical の SPEC を拡張、既存 UBO block 末尾に新規 member を std140 padding 維持で追加。既存 member offset 不変 = ABI 互換。case 3 (canonical 外 entity = A2/A4 設計時漏れ) 解決手段として A7 で確立。

**std140 末尾追加 ABI 互換証明**:
- std140 layout は declaration 順序で member offset を決定
- 末尾に新規 member を追加しても、既存 member offset は不変
- GPU shader は declared 範囲のみ read、CPU は extended struct を write
- 既存 file (新規 member 未参照) は trailing bytes として未使用 (functional 互換)

### §3.2 FrameAtmosphere canonical 補正 literal (haze_horizon + gamma 末尾追加)

**A7 補正後 canonical** (binding rule artifact §2 (C) 反映、5 file patch):
```glsl
#ifdef LL_VULKAN_GLSL
layout(set=0, binding=2, std140) uniform FrameAtmosphere {
    vec3  sunlight_color;
    float scene_light_strength;
    vec3  moonlight_color;
    float haze_density;
    vec3  ambient_color;
    float density_multiplier;
    vec3  blue_horizon;
    float distance_multiplier;
    vec3  blue_density;
    float max_y;
    vec3  glow;
    float sky_sunlight_scale;
    float sky_ambient_scale;
    float sky_hdr_scale;
    int   classic_mode;
    int   cube_snapshot;
    float minimum_alpha;
    float max_cof;
    float haze_horizon;     // A7 canonical 補正追加
    float gamma;            // A7 canonical 補正追加
    float _pad_atm0;
    float _pad_atm1;
};
#else
uniform float haze_horizon;  // OR
uniform float gamma;
#endif
```

**注入対象 5 file** (haze_horizon または gamma 参照):
- cloudsV (haze_horizon 参照)
- skyV (haze_horizon 参照)
- CASF (gamma 参照、LEGACY_GAMMA 内)
- postDeferredTonemap (gamma 参照、GAMMA_CORRECT 内)
- postDeferredGammaCorrect (gamma 参照、Section A FrameViewProj UBO と統合 patch)

**意図的 untouched 41 file** (FrameAtmosphere UBO 持つが haze_horizon/gamma 不参照):
- skyF / pbropaqueF / pbralphaF / pointLightF / reflectionProbeF (class2/class3) / softenLightF / deferredUtil / spotLightF / pbrglowF / waterF / volumetricLightF / materialF / bumpF / postDeferredHQDoFF / postDeferredF / dofCombineF / cofF / treeShadowF / treeF / pbrShadowAlphaMaskF / pbrShadowAlphaBlendF / impostorF (class1/class1-objects) / alphaF / fullbrightF / diffuseAlphaMaskNoColorF / diffuseAlphaMaskF / avatarF / avatarAlphaShadowF / avatarAlphaMaskShadowF / lightAlphaMaskNonIndexedF / alphamaskF / atmosphericsFuncs / godraysF / hazeF / shadowAlphaMaskF / atmosphericsHelpersV / atmosphericsHelpersF / atmosphericsF / lightAlphaMaskF / diffuseAlphaMaskIndexedF (skip list 含む 4 file = atmosphericsFuncs/volumetricLightF (class3 と class1)/godraysF も untouched)
- これら 41 file は FrameAtmosphere UBO 末尾 _pad_atm0/_pad_atm1 までで打ち切り、CPU canonical extended layout の trailing bytes (haze_horizon/gamma 領域) を read しない = functional 互換維持

### §3.3 MaterialUBO canonical 補正 literal (metallicFactor + roughnessFactor 末尾追加)

**A7 補正後 canonical** (binding rule artifact §2 (E) 反映、2 file × dual block patch):
```glsl
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=0, std140) uniform MaterialUBO {
    mat4  texture_matrix0;
    vec4  texture_base_color_transform[2];
    vec4  texture_emissive_transform[2];
    vec4  color;
    vec3  emissiveColor;
    float _pad_emissive;
    float metallicFactor;       // A7 canonical 補正追加
    float roughnessFactor;      // A7 canonical 補正追加
    float _pad_material0;
    float _pad_material1;
};
#else
uniform float metallicFactor;
uniform float roughnessFactor;
#endif
```

**注入対象 2 file × dual block**: pbralphaF + pbropaqueF (各 IS_HUD/non-IS_HUD 両 block)

### §3.4 extension sampler emissiveMap binding=5 (case 1 漏れ補正)

**A7 補正 literal** (binding rule artifact §2 (F) 反映、A5 で漏れた 1 file 補正):
```glsl
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=5) uniform sampler2D emissiveMap;
#else
uniform sampler2D emissiveMap;
#endif
```

**注入対象 1 file**: pbralphaF (Section E と統合 patch、dual block 両方)

### §3.5 補正 3 件確定 (A7 で確定)

1. **FrameAtmosphere canonical 拡張 (haze_horizon + gamma)** = A2 設計時漏れ、案 D で末尾追加
2. **MaterialUBO canonical 拡張 (metallicFactor + roughnessFactor)** = A4 設計時漏れ、案 D で末尾追加 (dual block)
3. **extension sampler emissiveMap binding=5** = A5 漏れ (case 1)、pbralphaF dual block 注入

---

## §4 cold cache launch verify metric (2026-06-01) vs A6 baseline

| metric | A6 baseline | A7 (current) | delta |
|---|---|---|---|
| β-2-hook fire (generatePerProgramSPIRV) | 224 | 226 | +2 (bundle-A 進行) |
| parse failed | 224 | 224 | ±0 (A7 単独 SPIR-V 生成 0% controlled) |
| error 種別 `'location'` | 158 | **182** | **+24** (cascade = binding 解消後の次 error 露出) |
| error 種別 `'binding'` | 30 | **0** | **-30 ✓ A7 patches 効果完全直接観測** |
| error 種別 non-opaque uniforms outside block | 36 | 32 | -4 (small improvement) |
| error 種別 missing #endif | 8 | 8 | ±0 (bundle-B/C scope で根本解消予定) |
| shader_cache 再生成 (.shaderbin) | 224 | 224 | ±0 (GL path regression 0) |
| link failed | 0 | 0 | ±0 |
| FATAL | 0 | 0 | ±0 |
| SIGSEGV | 0 | 0 | ±0 |
| crash (真) | 0 | 0 | ±0 (4 件 mention 全 benign = settings_crash_behavior.xml load 3 + save 1) |
| 起動成立 + clean shutdown | OK | OK (Goodbye! + Vulkan device destroyed + Vulkan instance destroyed) | — |
| **net delta (errors total)** | — | **-10** | **controlled improvement** |

### §4.1 metric net delta -10 解釈 (binding category -30 specifically 直接観測)

A7 patches 効果 = **binding error 30 → 0 (-30)** が canonical 漏れ entity 補正の完全直接観測。A1-A6 全 sub-bundle 中で最大の単独 delta = A7 が bundle-A cleanup として binding category を完遂した証拠。

cascade 解釈:
- location +24 = binding error が先 fire していた file で binding 解消後、次の location error が露出 (glslang per-file first-error fail semantics)
- non-opaque -4 = MaterialUBO/FrameAtmosphere 末尾追加で bare uniform が block 内に移動 (small improvement)
- missing #endif ±0 = bundle-B/C scope で根本解消予定 (A7 範囲外)

**net delta -10 errors = bundle-A 全体 (A1-A7) 完遂時の集合的閾値で評価する設計通り** (A1-A6 各 sub-bundle は集合的閾値で評価、A7 で初めて binding category 単独 -30 を達成)。

### §4.2 charter §3 #1 acceptance 担保

GL path 224 .shaderbin 再生成 + crash 0 + clean shutdown OK で確認。AYAstorm 改変 (Cinematic BD shadowUtil/screenSpaceReflUtil + Visual Realism volumetricLight/blurLightF/godraysF 系 A8-recovery 復活 5 file + Picker 2 + Exemplar 2 + A2 拡張 2) untouched 維持。

---

## §5 self-verify 結果

### §5.1 (a) skip list 違反 0 件

- skip list base 13 file (Picker 2 + Cinematic BD 2 + Visual Realism 7 + Exemplar 2) × `git diff` = 全 0 件
- A2 拡張 skip 2 file (previewV + multiPointLightF) × `git diff` = 全 0 件
- 違反合計 = **0 件**

### §5.2 (b) canonical 補正 literal byte-for-byte 一致

- **FrameAtmosphere 補正 5 file** (CASF + cloudsV + postDeferredGammaCorrect + postDeferredTonemap + skyV): canonical 末尾追加 (`haze_horizon` + `gamma` + `_pad_atm0` + `_pad_atm1`) byte-for-byte 一致
- **MaterialUBO 補正 2 file × dual block 4 instance** (pbralphaF non-HUD + pbralphaF HUD + pbropaqueF non-HUD + pbropaqueF HUD): canonical 末尾追加 (`metallicFactor` + `roughnessFactor` + `_pad_material0` + `_pad_material1`) byte-for-byte 一致
- **emissiveMap binding=5 1 file × dual block 2 instance** (pbralphaF): binding 番号 5 厳守、1 件補正
- **FrameAtmosphere 未補正既存形式 2 file** (skyF + pbropaqueF): canonical 補正前形式 (`_pad_atm0`/`_pad_atm1` のみ、haze_horizon/gamma 不存在) untouched 維持確認 (FrameAtmosphere 参照外 entity = haze_horizon/gamma 未使用、§3.2 意図的 untouched 41 file カテゴリ)

### §5.3 (c) 3-段 swap pattern balanced

- 全 25 file `#ifdef LL_VULKAN_GLSL` / `#else` / `#endif` count 一致
- file 内 nested #ifdef (LEGACY_GAMMA / GAMMA_CORRECT / HAS_NOISE / IS_HUD / HAS_SUN_SHADOW) 差異あり = 正常 (conditional block 内注入のため)
- variant pattern `defined(LL_VULKAN_GLSL)` / `#ifndef LL_VULKAN_GLSL` (Section B pointLightF env_mat[3] 例外を除く) = 0 件

### §5.4 (d) Section 別 patch count

| Section | 注入 件数 |
|---|---|
| A (screen_res FrameViewProj) | 15 file × 1+ swap = 15 declaration |
| B (env_mat FrameViewProj/bare wrap) | 4 file × 1 swap |
| C (haze_horizon FrameAtmosphere 拡張) | 2 file × 1 = 2 declaration 末尾追加 |
| D (gamma FrameAtmosphere 拡張) | 3 file × 1 = 3 declaration 末尾追加 |
| E (metallicFactor/roughnessFactor MaterialUBO 拡張) | 2 file × 2 block (dual IS_HUD) = 4 declaration 末尾追加 |
| F (emissiveMap binding=5) | 1 file × 2 block (dual IS_HUD) = 2 declaration |
| **unique file 計** | **25** (重複統合 = postDeferredGammaCorrect A+D / pbralphaF E+F) |

### §5.5 (e) A1-A6/A8-recovery 既処理 file の既存 UBO block 末尾追加例外

- pbralphaF/pbropaqueF dual MaterialUBO 末尾 metallicFactor/roughnessFactor 追加 (案 D ABI 互換 std140 末尾追加例外)
- cloudsV/skyV/CASF/postDeferredTonemap/postDeferredGammaCorrect FrameAtmosphere 末尾 haze_horizon/gamma 追加 (案 D ABI 互換 std140 末尾追加例外)
- skyF/reflectionProbeF (class2/class3) FrameViewProj 末尾 env_mat 追加 (案 D ABI 互換 std140 末尾追加例外)
- 既存 member (UBO 既 member + binding qualifier) 行 diff = 全 0 件
- 末尾追加例外を除く再 touch 違反合計 = **0 件**

### §5.6 (f) insertions-only

- 367 insertions / 0 deletions
- deletion 例外 0 件
- reorder artifact 0 件 (A6 pattern ε non-adjacent reorder と異なり、A7 は全 file insertions-only 純粋追加)

### §5.7 (g) softenLightV A1-A6 touch 履歴 0 件確認 (Agent 注意点 1)

- `git log --follow indra/newview/app_settings/shaders/class2/deferred/softenLightV.glsl` 確認 (5 upstream commits all unrelated)
- A1-A6 touch 履歴 0 件 → case 1(b) 新規 FrameViewProj UBO 注入 正当性確認

### §5.8 (h) pointLightF class3 既 FrameViewProj UBO 保全 (Agent 注意点 2)

- pointLightF class3 既 FrameViewProj UBO (canonical mat3 env_mat 含む) untouched
- bare `uniform vec3 env_mat[3]` (legacy GL form) のみ `#ifndef LL_VULKAN_GLSL` wrap で追加
- 既 UBO block 行 diff = 0 件

### §5.9 (i) skyF FrameAtmosphere 補正前形式 untouched (Agent 注意点 3)

- skyF FrameAtmosphere は補正前形式 (haze_horizon/gamma 未含、_pad_atm0/_pad_atm1 ある) untouched 維持
- FrameAtmosphere 参照外 = haze_horizon/gamma 未使用、§3.2 意図的 untouched 41 file カテゴリ
- skyF は Section B (FrameViewProj UBO 末尾 env_mat) のみ対象

---

## §6 設計差分 (A6 vs A7)

### §6.1 binding 範囲

| 項目 | A6 | A7 |
|---|---|---|
| 主軸 binding | `set=2/binding=0` (PerDrawUBO 単一) | `set=0/binding=0` (FrameViewProj 末尾追加/新規) + `set=0/binding=2` (FrameAtmosphere 末尾追加) + `set=1/binding=0` (MaterialUBO 末尾追加 dual block) + `set=1/binding=5` (emissiveMap) |
| descriptor set | set=2 (per-draw) | set=0 (per-frame) + set=1 (per-material) |
| 主操作 | 新規 UBO 注入 (PerDrawUBO file-local subset) | **canonical 拡張 + 末尾追加 (案 D)** + 新規 UBO 注入 (Section A 13 file 新規 + Section B 4 file 末尾追加/bare wrap) |
| 注入 file 数 | 13 file | 25 file |
| 注入 declaration 総数 | 13 件 | 32 件 (Section A 15 + B 4 + C 2 + D 3 + E 4 dual block + F 2 dual block = 30、+ pbralphaF/pbropaqueF 末尾追加 dual block 2 件) |
| 削除行 | 2 (semantic-equivalent reorder) | 0 (純粋 insertions-only) |

### §6.2 layout 範式

- A6 = PerDrawUBO file-local override (各 file は使う member のみ subset)、6 layout pattern (α-ζ)
- A7 = **案 D = canonical 拡張 + 既存 UBO 末尾追加 範式 (A7 で確立)**、5 file FrameAtmosphere + 2 file × dual block MaterialUBO + 3 file FrameViewProj 末尾追加例外

### §6.3 既処理 file 追加注入 vs 末尾追加例外

- A6 既処理 6 file: PerDrawUBO のみ独立 3-段 swap 追加 (既存 block untouched 範式)
- A7 既処理 12 file: 同範式継承 + **末尾追加例外** (案 D で既存 UBO block 末尾に新規 member 追加可能、std140 ABI 互換維持)

### §6.4 file 数規模

A6 13 file → A7 25 file (規模拡大、cleanup scope 広い)。1 Agent 単独 patch で完遂 (A6 と同様、scan は 4 Agent 並列で効率化)。

### §6.5 metric delta 性質

- A6 metric: 全 dimension ±0 (集合的閾値設計通り)
- A7 metric: **binding -30 直接観測 + location +24 cascade + non-opaque -4 + missing #endif ±0 = net delta -10 errors**、A1-A6 全 sub-bundle 中で最大の単独 binding category 改善

---

## §7 A7 で確定した範式 (bundle-B/C 継承必須)

### §7.1 hard rule 7 件 (A5-A6 範式継承 + A7 案 D 追加)

1. UBO/sampler declaration byte-for-byte canonical (file 毎 subset 確定 literal)
2. 3-段 swap pattern 厳守 (`#ifdef LL_VULKAN_GLSL ... #else ... #endif`、変形禁止、`#ifndef` は B pattern legacy wrap 例外のみ)
3. binding 番号 binding rule artifact 表遵守 (独自割当禁止)
4. 1 file 1 patch (block 単位、Section 統合 file は同 file 内に統合)
5. A1/A2/A3/A4/A5/A6/A8-recovery 既処理 file の UBO block + sampler binding qualifier 既存 member 再 touch 禁止 (**案 D 末尾追加例外は §7.2 に従う**)
6. skip list 13 file (Picker 2 + Cinematic BD 2 + Visual Realism 7 + Exemplar 2) 機械的 untouched
7. A2 拡張 skip 2 file (previewV + multiPointLightF) untouched

### §7.2 案 D = canonical 拡張 + 既存 UBO 末尾追加 範式 (A7 で確立、bundle-B/C 継承)

**A5 §6.3 範式 literal 例外**:
- 原則: 既処理 file の既存 UBO block 内部 (declaration 順序、member 名、type、padding) は untouched
- A7 例外: **既存 UBO block 末尾に新規 member を std140 padding 維持で追加可能** (canonical 拡張、ABI 互換)
- 適用条件: 案 D を採用する case 3 (canonical 外 entity = 設計時漏れ) 解決手段として
- 影響範囲: 該 entity を参照する file のみ末尾追加、不参照 file は意図的 untouched (trailing bytes として functional 互換)
- bundle-B/C 継承: 同様の case 3 が発生した場合、案 D を採用可

### §7.3 conditional injection 範式継承 (A2-A6 共通 + A7 適用)

- file が当該 entity を 1 件でも参照する場合のみ補正
- 参照 0 件 file は意図的 untouched
- A7 例: FrameAtmosphere 補正 5 file (haze_horizon/gamma 参照) vs 41 file (不参照、意図的 untouched)

### §7.4 dual IS_HUD branch 範式継承 (A4 確立 + A7 適用)

- pbralphaF/pbropaqueF MaterialUBO 拡張は両 block (IS_HUD + non-IS_HUD) に均等適用
- preprocessor-exclusive 各分岐に MaterialUBO 拡張独立注入

### §7.5 conditional block 内注入範式継承 (A6 確立 + A7 適用)

- HAS_NOISE / LEGACY_GAMMA / GAMMA_CORRECT / IS_HUD / HAS_SUN_SHADOW 内 entity は同 conditional 内に注入
- A7 例: glowExtractF HAS_NOISE 内 screen_res / CASF LEGACY_GAMMA 内 gamma / postDeferredTonemap GAMMA_CORRECT 内 gamma

### §7.6 case 1(b) 新規 FrameViewProj UBO 注入 範式 (A7 で適用)

- A1 漏れ candidate file (A1 で touch されていない、screen_res 等 per-frame entity 持つ) は case 1(b) 新規 FrameViewProj UBO 注入
- 注入前 git log 確認で touch 履歴 0 件確認 (softenLightV 例)
- canonical literal は A1 と完全一致 (member 順序 + binding 番号)

### §7.7 既 FrameViewProj UBO file の bare uniform wrap 範式 (A7 で適用)

- A1 既処理 file (canonical FrameViewProj UBO 持つ) で bare uniform legacy GL form (例: `uniform vec3 env_mat[3];` = vec3 array form for legacy GL) が #else 外に残る場合、`#ifndef LL_VULKAN_GLSL` wrap で GL path 限定化
- 既 UBO block 内 canonical mat3 env_mat は untouched
- pointLightF class3 例

---

## §8 bundle-A 全体完遂 + bundle-B/C scope 着手境界

### §8.1 bundle-A 全 7 sub-bundle 完遂境界

| sub-bundle | scope | commit | 注入 file 数 | 主成果 |
|---|---|---|---|---|
| A1 | FrameViewProj UBO `set=0/binding=0` | `6f941c0a48` | 94 file | per-frame matrix canonical 9 member |
| A2 | FrameLights `set=0/binding=1` + FrameAtmosphere `set=0/binding=2` | `9f77f875db` | 52 file | per-frame light/atmosphere |
| A3 | per-frame frame-global sampler `set=0/binding=3-21` | `ebd5e2b16d` | 32 file | 19 sampler individual layout |
| A4 | per-material core `set=1/binding=0-3` (MaterialUBO + diffuseMap/normalMap/specularMap) | `f703710f8d` | 82 file | MaterialUBO + KHR_texture_transform packed + dual IS_HUD |
| A5 | per-material extension sampler `set=1/binding=4-66` + TerrainDetailUBO `set=1/binding=17` + Probe `set=1/binding=40-42` + Post/utility `set=1/binding=50-66` | `eddcc7ac40` | 50 file | extension sampler + pbrterrainF TerrainDetailUBO 新規 |
| A6 | per-draw PerDrawUBO `set=2/binding=0` | `b80d90bea4` | 13 file | file-local override + 6 layout pattern α-ζ |
| **A7** | **bundle-A 残 cleanup (canonical 漏れ補正 + screen_res A1 漏れ)** | **`862f7dc8bd`** | **25 file** | **案 D = canonical 拡張 + 末尾追加 範式確立、binding category -30 直接観測** |
| A8-recovery | skip-list 11 file 中 8 file 遅延 UBO 注入 (Cinematic BD 2 + Visual Realism 5 + Picker 1) | `aed1438936` | 8 file | AYAstorm 改変 file additive 注入 |

**累計**: 354 file (重複ありで raw 累計、ユニーク = bundle-A 全体 effective patch target ~237 + A8-recovery 8 = ~245 file)
**累計変更行**: ~3 200+ insertions (A1 1 400 + A2 1 315 + A3 163 + A4 770 + A5 312 + A6 84 + A7 367 + A8-recovery 226 = 4 637 insertions)

### §8.2 bundle-A 完遂判定

- binding 番号別 注入完遂: A1 (set=0/binding=0) + A2 (set=0/binding=1,2) + A3 (set=0/binding=3-21) + A4 (set=1/binding=0-3) + A5 (set=1/binding=4-66) + A6 (set=2/binding=0) + A7 (cleanup 漏れ補正) = **全 binding 表 §2 (A)-(G) 注入完遂**
- skip list 13 file 機械的 untouched 維持
- A2 拡張 skip 2 file untouched 維持
- AYAstorm 改変 11 file (skip list 含む) は A8-recovery で 8 file 遅延注入完遂、残 3 file (fsObjectIDF + godraysV + class1/volumetricLightF) は UBO member 参照 0 件で除外 (conditional injection 範式通り)
- AYA cold cache launch verify PASS (起動成立 + clean shutdown + GL regression 0)
- charter §3 #1 acceptance 担保 (GL path untouched + AYAstorm 独自改造意図保全 + 段階 1-4.3 動作維持)
- net delta -10 errors (binding -30 直接観測 + cascade +24/-4)

### §8.3 bundle-B/C scope 着手境界

**bundle-B**: per-program SPIR-V parse error の根本解消 (残 location 182 + non-opaque 32 + missing #endif 8)
- 推定 scope: include directive resolution + forward decl 解消 + per-program shader concat (β-2-hook で部分実装、bundle-B で本格化)
- glslang per-file first-error fail semantics 由来の cascade 解消が主目的

**bundle-C**: SPIR-V 生成成功率 0% → >0% 達成
- bundle-A (binding 解消) + bundle-B (location/non-opaque/endif 解消) 後の最終 cleanup
- per-program 単位での全 entity routing 整合 + descriptor set layout 確定

**集合的閾値評価**: bundle-A 全体 metric は bundle-A 単独では net delta -10 controlled、bundle-B/C 完遂後の集合的閾値で SPIR-V 生成成功率 >0% 達成を期待。

### §8.4 着手前チェックリスト 10 件

1. `git fetch origin` で remote 最新確認
2. `git log -1` で HEAD = `862f7dc8bd` (A7 patch) 確認
3. 本 handoff doc commit が direct parent であることを確認
4. AYAstorm 改変 11 file = bundle-A 通常 file 同等扱い (skip list 機械的 exclusion とは独立)
5. skip list 13 file 機械的 exclusion 継続 (`bundle-A-skip-list.txt` 必須読了)
6. binding rule artifact `/tmp/bundle-A-binding-rules.md` §2 (A)-(G) + A7 補正 3 件 (本 doc §3) 必須読了
7. 8 件 prior handoff doc 読了必須 (A1-complete + A2-complete + A3-complete + A4-complete + A5-complete + A6-complete + A7-complete + bundle-A-prep)
8. project memory `project_ayastorm_r41_vulkan_migration.md` 確認 (bundle-A 完遂 + bundle-B/C 着手境界 active 状態)
9. A1-A7/A8-recovery 既処理 file 再 touch 禁止 rule (案 D 末尾追加例外を除く)
10. cold cache launch verify 準備 (`rm -rf ~/.ayastorm_x64/cache/shader_cache` sub-bundle 毎必須)

---

## §9 risks/caveats 8 件

### §9.1 bundle-A 完遂後の metric net delta -10 = controlled improvement

A7 で binding category -30 を直接観測、A1-A6 集合的閾値設計通り。bundle-A 全体 (A1-A7) では canonical 注入完遂 + binding error 0 達成、残 location/non-opaque/missing #endif は bundle-B/C scope で根本解消予定。

### §9.2 cold cache launch verify 必須

`rm -rf ~/.ayastorm_x64/cache/shader_cache` を sub-bundle 毎 verify 前必須化。本 A7 でも実施済。warm cache では .shaderbin 再生成しないため SPIR-V hook 経路通らず metric 取得不能。

### §9.3 案 D = canonical 拡張は 5 FrameAtmosphere file のみ patch (意図的)

haze_horizon/gamma 参照する file のみ patch、他 41 file は意図的 untouched (CPU canonical extended layout 下で trailing bytes として functional 互換、GPU 視点は OLD form でも _pad_atm0/_pad_atm1 アクセスしない trailing bytes)。bundle-B/C で同様の case 3 が発生した場合、本範式継承。

### §9.4 canonical asymmetry concern (future maintainer 認識混乱の可能性)

5 file vs 41 file の FrameAtmosphere 差異を見て canonical 認識混乱の可能性あり。本 handoff doc + binding rule artifact §2 (C) で literal 明示記録、commit message §3 (d) で意図的 untouched 明示済。bundle-B/C handoff でも継承必須。

### §9.5 残 skip 既知 list (A6 から継承)

- exemplar 2 (`class1/deferred/diffuseV.glsl` + `diffuseF.glsl`) untouched 維持 (sub-doc 03 §3.1.3 β-1 PoC 試作レール)
- A2 拡張 skip 2 (`previewV.glsl` + `multiPointLightF.glsl`) untouched 維持
- skip list base 13 untouched 維持

これらは bundle-B/C で touch しない。

### §9.6 binding rule artifact `/tmp/bundle-A-binding-rules.md` persist 性 fragile

`/tmp` 配下のため再起動で失効可能性あり。失効時は本 commit message §3 + A1-A7 完遂 handoff doc 7 件 + A7-complete §3 (案 D 範式) + A4-complete §6.2 (TerrainDetailUBO 範式) + A6-complete §3 (PerDrawUBO 6 layout pattern) から再生成可能。

### §9.7 missing #endif 8 件残

bundle-A 全体 (A1-A7) 完遂後も残る可能性、bundle-B/C scope で根本解消予定。preprocessor directive 不整合に由来し、A7 cleanup の対象外。

### §9.8 motionBlurF.glsl 残 bare uniform `motion_blur_strength`

A7 で screen_res は FrameViewProj UBO に注入完遂、ただし `uniform int motion_blur_strength;` (line 56) は bare uniform のまま残置。push constant 候補 (per-program scalar) として bundle-B/C scope で別途処理予定 (A7 scope = screen_res cleanup のみ)。

---

## §10 AYA 承認境界 6 件

1. 本 A7-complete handoff doc commit (AYA 「OK commit して」明示要)
2. bundle-A 完遂状態確定承認 (AYA 「OK」明示要)
3. bundle-B/C 着手指示 (AYA 「OK」明示要、fresh context 推奨)
4. bundle-B/C 各 sub-bundle commit (AYA 「OK commit して」明示要)
5. A1/A2/A3/A4/A5/A6/A7/A8-recovery 既処理 file 再 touch 禁止境界 (案 D 末尾追加例外を除く、Agent prompt 必須注入で enforce)
6. skip list 13 file + A2 拡張 skip 2 file untouched 継続境界 (Agent prompt 必須注入で enforce)

---

## §11 次 session 投入 prompt (fresh context 推奨)

```
AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B/C scope (bundle-A 完遂後の本格 SPIR-V parse error 解消) に着手境界。

【読了必須 10 件】
1. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A1-complete.md
2. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A2-complete.md
3. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A3-complete.md
4. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A4-complete.md
5. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A5-complete.md
6. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A6-complete.md
7. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A7-complete.md (本 doc、source-of-truth、bundle-A 完遂 + bundle-B/C 着手境界)
8. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A8-recovery-complete.md
9. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-prep.md
10. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md
+ /tmp/bundle-A-binding-rules.md (失効時は再生成、本 handoff §3 + A1-A6 handoff doc から)
+ docs/specs/ayastorm-r41-gl-removal/bundle-A-skip-list.txt (13 file 機械除外)

【bundle-A 完遂状態】
- A1-A7 + A8-recovery 全 sub-bundle 完遂、累計 ~245 unique file ~4 600 insertions
- binding 番号別 注入完遂 = 全 binding 表 §2 (A)-(G) literal canonical
- metric vs baseline = binding 30→0 (A7 -30 直接観測) + location 158→182 (cascade) + non-opaque 36→32 + missing #endif 8 (不変) = net delta -10
- 集合的閾値設計通り controlled improvement、SPIR-V 生成成功率 0% (parse failed 224/224)

【bundle-B/C scope】
- bundle-B: per-program SPIR-V parse error 根本解消 (残 location 182 + non-opaque 32 + missing #endif 8)
  - 推定 scope = include directive resolution + forward decl 解消 + per-program shader concat
  - glslang per-file first-error semantics 由来 cascade 解消が主目的
- bundle-C: SPIR-V 生成成功率 0% → >0% 達成 (bundle-A + bundle-B 後の最終 cleanup)

【hard rule 7 件 (A5-A7 範式継承)】
1. UBO/sampler declaration byte-for-byte canonical
2. 3-段 swap pattern 厳守 (`#ifdef LL_VULKAN_GLSL ... #else ... #endif`、変形禁止)
3. binding 番号 binding rule artifact 表遵守
4. 1 file 1 patch (block 単位、Section 統合 file は同 file 内に統合)
5. A1/A2/A3/A4/A5/A6/A7/A8-recovery 既処理 file の既存 block 再 touch 禁止 (案 D 末尾追加例外を除く)
6. skip list 13 file untouched 継続
7. A2 拡張 skip 2 file untouched 継続

【案 D 範式 (A7 で確立、bundle-B/C 継承)】
canonical 拡張 + 既存 UBO 末尾追加 with std140 末尾追加で ABI 互換。既存 member offset 不変、case 3 (canonical 外 entity = 設計時漏れ) 解決手段として採用可。

【cadence】
sub-bundle 毎 cadence 8 step 範式継承 = trace → prep → patch [AYA「OK」明示要] → verify [self] → handoff [shader cp + rm -rf cache + AYA launch verify] → measurement → commit [AYA「OK commit して」明示要] → complete handoff doc 起草

【feedback rule 13 件】
feedback_proactive_handoff / feedback_self_verify_before_handoff / feedback_use_agents_proactively / feedback_no_scope_shrink / feedback_doubt_self_first / feedback_admit_unknown / feedback_falsification_as_progress / feedback_explanation_lead_with_conclusion / feedback_no_claude_coauthor / feedback_no_auto_commit / feedback_one_step_at_a_time / feedback_remove_verification_logs / feedback_build_only_verified

【1st action】
bundle-B/C scope 着手前に AYA「OK」確認、bundle-A 完遂状態 + bundle-B/C scope 認識 1 行 status 報告。
```

---

## §12 cross reference

### §12.1 handoff doc 系譜

- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A1-complete.md` (A1 完遂 handoff、`99afb8f1bc`)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A2-complete.md` (A2 完遂 handoff、`8e69841a53`)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A3-complete.md` (A3 完遂 handoff、`524391d78d`)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A4-complete.md` (A4 完遂 handoff、`26a383f03f`)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A5-complete.md` (A5 完遂 handoff、`840e1f5684`)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A6-complete.md` (A6 完遂 handoff、`b80d90bea4`、本 doc 直接 parent 範式継承元)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A8-recovery-complete.md` (A8-recovery 完遂 handoff、`ac3f294643`)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-prep.md` (bundle-A 全体 prep、`1434341904`)
- `handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md` (β-2-hook 完遂 handoff、`cecb9e6467`)

### §12.2 patch commit 系譜

- `6f941c0a48` (A1 patch commit)
- `9f77f875db` (A2 patch commit)
- `ebd5e2b16d` (A3 patch commit)
- `f703710f8d` (A4 patch commit)
- `eddcc7ac40` (A5 patch commit)
- `aed1438936` (A8-recovery patch commit)
- `b80d90bea4` (A6 patch commit)
- `862f7dc8bd` (A7 patch commit、**本 handoff の直接 parent**)

### §12.3 spec sub-doc

- sub-doc 06 §1.2.2/§1.2.4/§3.1 sub-step 6.3 (per-frame baseline)
- sub-doc 07 §3.1 sub-step 7.2-7.4 (per-material/per-draw binding 範囲)
- sub-doc 03 §3.1.3 (exemplar 2 役割 = β-1 PoC 試作レール)
- charter §3 #1 (GL/Vulkan 並走 acceptance) + §7.5 (binding 設計)

### §12.4 artifact + memory + skip list

- binding rule artifact `/tmp/bundle-A-binding-rules.md` §2 (A)-(G) (本 A7 で補正 3 件確定 = FrameAtmosphere canonical 拡張 + MaterialUBO canonical 拡張 + emissiveMap binding=5、bundle-B/C で source-of-truth)
- project memory `project_ayastorm_r41_vulkan_migration.md` (bundle-A 完遂 + bundle-B/C 着手境界 active 状態に update)
- skip list `bundle-A-skip-list.txt` (base 13 file、Picker 2 + Cinematic BD 2 + Visual Realism 7 + Exemplar 2)

### §12.5 feedback rules 13 件

`feedback_proactive_handoff` / `feedback_self_verify_before_handoff` / `feedback_use_agents_proactively` (本 A7 で 4 Agent 並列 scan + 1 Agent 単独 patch) / `feedback_no_scope_shrink` / `feedback_doubt_self_first` (本 A7 で FrameAtmosphere canonical asymmetry self-detect → AYA 明示確認実施 → 案 D 設計意図確認後 proceed) / `feedback_admit_unknown` / `feedback_falsification_as_progress` / `feedback_explanation_lead_with_conclusion` / `feedback_no_claude_coauthor` / `feedback_no_auto_commit` (AYA 「OK commit して」明示指示下で commit) / `feedback_one_step_at_a_time` 遵守 / `feedback_remove_verification_logs` / `feedback_build_only_verified`
