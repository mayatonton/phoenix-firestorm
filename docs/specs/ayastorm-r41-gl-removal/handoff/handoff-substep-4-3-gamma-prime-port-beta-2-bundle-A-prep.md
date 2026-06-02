# r41 sub-step 4.3-γ'-port-β-2-bundle-A-prep handoff (2026-06-01)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md` (commit `fcf2b6c508` = β-2-hook 完遂宣言 = per-program SPIR-V 生成 hook 昇格 + β-1 per-file model 廃止 + AYA build verify PASS + launch verify PASS、SPIR-V 生成成功率 0% は bundle 未着手で incremental measurement 設計通り)
**本 handoff 位置付け**: sub-step 4.3-γ'-port-β-2-bundle-A 着手前 prep + 実装 plan 確定境界。β-2-hook (commit `fcf2b6c508`) で確立した per-program hook 範式 (`LLGLSLShader::createShader()` loop 完遂時点に `generatePerProgramSPIRV(mStageSources)` 配置で multi-stage 単一 TProgram link 経路成立) を base に、**237 file uniform/sampler UBO 化 + binding qualifier 注入** の structural rewrite plan を確定。bundle-A-trace 4 件 (file list / uniform enum / sampler enum / binding rules) 完遂、binding rules (A)-(H) は `/tmp/bundle-A-binding-rules.md` (558 line / 35KB) に persist 済、本 prep doc で sub-bundle A1-A7 cadence + skip list 機械的 exclusion + patch テンプレート + measurement 設計を確定、AYA review PASS 後 commit + 別 session で bundle-A 着手 (fresh context 推奨)。

---

## §1 起草目的 + bundle-A scope 確定

### §1.1 起草目的

β-2-hook (commit `fcf2b6c508`、4 file +394/-100) で per-program SPIR-V 生成 hook を `LLGLSLShader::createShader()` に昇格、β-1 per-file model architectural mismatch (forward declaration link fail 4 symbol) を解消。残課題:

- **parse 段階失敗 226 件**: hook fire 226 件中 SPIR-V 生成成功率 **0%**、全 program が glslang parse 段階で fail (β-1 では link 段階で fail だったが、β-2 hook では parse 段階に進化)。原因 = 全 patch 対象 file が GLSL 仕様 (Vulkan SPIR-V profile) に未準拠 = uniform 非 opaque outside block / location qualifier 欠如 / entry point 周辺 3 issue type
- **bundle-A/B/C 未着手**: β-2-hook 完遂は「経路成立」のみ、237 file structural rewrite (UBO 化 + binding qualifier + location qualifier + entry point + #version 460) は全 sub-bundle 未着手

本 prep doc で bundle-A (uniform → UBO 化 + sampler binding qualifier) の実装 plan を確定。AYA review PASS 後 commit + fresh context で bundle-A 着手。

### §1.2 bundle-A scope (file count 確定)

| 区分 | file 数 | 役割 |
|---|---|---|
| **disk total** (`indra/newview/app_settings/shaders/**/*.glsl`) | **250** | 実 file system 上の全 GLSL file |
| **AYAstorm 改変 11** (untouched 必須) | 11 | Picker 2 + Cinematic BD 2 + Visual Realism 7 (charter §3 #1 acceptance GL 並走担保) |
| **exemplar 2** (untouched 必須) | 2 | β-1 PoC frozen (`class1/deferred/diffuseV.glsl` + `diffuseF.glsl`、sub-doc 03 §3.1.3 試作レール扱い継続) |
| **bundle-A patch 対象** | **250 − 11 − 2 = 237** | trace agent (`/tmp/bundle-A-217-files.txt` filename historical) で確定 |

**handoff prep `b7e2dbc72e` §3 の `228 − 11 = 217` 表記との不整合解消経緯**: handoff prep の base 228 は build-time pre-compile model 時代の旧分類値、case ② runtime model では実 disk 250 file 全件が patch 対象候補。bundle-A-trace-1 (2026-06-01) で disk total 250 確認、AYAstorm 改変 11 + exemplar 2 = 13 file 除外で **237 file** が effective patch target と確定。`/tmp/bundle-A-217-files.txt` の filename は historical (内容は 237 file 列挙)、`/tmp/bundle-A-237-files.txt` への rename は次 maintenance window で対応 (`/tmp/bundle-A-binding-rules.md` §7.10 既記載)。

### §1.3 bundle-A 完遂後 残課題 (bundle-B/C 着手境界)

| 残課題 | 解消 bundle |
|---|---|
| location qualifier 欠如 (varying/attribute/out/in 全件 `layout(location=N)` 注入) | **bundle-B** |
| entry point + #version 460 + extension declare | **bundle-C** |
| `atmosphericsFuncs.glsl` (untouched) include 衝突 (FrameAtmosphere UBO decl 二重定義) | **bundle-C** (新規 `atmosphericsUBO.glsl` 分離) |

bundle-A 単独で SPIR-V 生成成功率 >>0% は **期待しない** (handoff `cecb9e6467` §4.9 controlled measurement)。bundle-A 完遂時の目標は **parse pass 率向上** (uniform 非 opaque 起因 fail 解消)、>>0% 目標は bundle-A+B+C 完遂時。

---

## §2 binding 番号割当 rule (A)-(H) source-of-truth

**source-of-truth**: `/tmp/bundle-A-binding-rules.md` (558 line / 35KB / §1-§8 構造、本 session で persist 済、bundle-A patch Agent prompt に必須注入)

### §2.1 rule (A)-(H) サマリ (詳細は source doc 参照)

| Rule | scope | Vulkan 配置 | 主要 member 数 |
|---|---|---|---|
| **(A) per-frame matrix** | `modelview_projection_matrix` 等 9 member | `set=0/binding=0` **FrameViewProj** UBO | 9 (~448 byte) |
| **(B) per-frame light** | `sun_dir` / `light_position` 等 8 member + array variant | `set=0/binding=1` **FrameLights** UBO | 8 + array (~120 byte scalar + array) |
| **(C) per-frame atmospherics** | windlight 18 member (`sunlight_color` / `blue_density` 等) | `set=0/binding=2` **FrameAtmosphere** UBO | 18 (~256 byte) |
| **(D) frame-global sampler** | `depthMap` / `shadowMap0-5` / `environmentMap` 等 18 unique | `set=0/binding=3-21` 連番 | 19 slot 使用 (Vulkan tier-1 1024+ 余裕) |
| **(E) per-material constant** | `texture_matrix0` / `color` 等 5 member | `set=1/binding=0` **MaterialUBO** | 5 (~256 byte) |
| **(F) per-material sampler** | `diffuseMap` / `normalMap` / `specularMap` 固定 + extension + PBR terrain 20-39 + probe/LUT 40-42 + post/SMAA 50-66 | `set=1/binding=1+` | 55 unique |
| **(G) per-draw** | `matrixPalette` / `lastMatrixPalette` / `clipPlane` / `size` + 光 file の `color`/`proj_mat` file-local override | `set=2/binding=0` **PerDrawUBO** | 可変 (skin 45-110 joint で 2160-10560 byte) |
| **(H) per-draw sampler** | **bundle-A 寄与 0** (symbolic reserve のみ) | `set=2/binding=1-15` (symbolic) | 0 |

**push_constant**: `mat4 modelMatrix` × 64 byte (`VK_SHADER_STAGE_VERTEX_BIT`)、bundle-A では VS file の `#ifdef LL_VULKAN_GLSL` で `layout(push_constant) uniform PushConstants { mat4 modelMatrix; };` 宣言追加、実 push 配線は γ'-port-γ PSO scope。

### §2.2 全 727 uniform + 193 sampler 100% routing 確認 (源 doc §3.6)

| 検証項目 | 結果 |
|---|---|
| uniform 727 宣言 → (A)-(C) + (E) + (G) routing | 全件配置済 |
| sampler 193 宣言 → (D) + (F) + (H) routing | 全件配置済 (H = 0 件) |
| 未分類 (catch-all 含む) | **0 件** |

### §2.3 dual context 解消方針 (源 doc §3)

| uniform/sampler | dual context 疑い | trace 後判定 |
|---|---|---|
| `normalMap` | utility (`set=0/binding=N`) vs material (`set=1/binding=2`) | **杞憂判定** = 全 5 宣言が material 単一 context |
| `texture_matrix0` | material UV vs projector UV | **杞憂判定** = 全 39 宣言が material UV 単一 semantic、projector 用は bundle-C 委譲 (`deferredUtil.glsl` 内) |
| `color` | per-material vs per-light | **真の dual** = 11 file material (E) vs 3 file light (G、`(pointLight\|spotLight\|deferredUtil)F\.glsl$` regex routing) |
| `environmentMap` | sampler2D (1 file) vs samplerCube (4 file) | 同一 binding `set=0/binding=6`、type は declaring file 個別、per-program SPIR-V 生成で descriptor 個別解決 |
| `waterPlane` (B-frame) vs `clipPlane` (G-draw) | 同 type vec4 衝突疑い | **別 uniform** = 別名・別 cadence、衝突なし |

---

## §3 sub-bundle A1-A7 構成 + cadence

### §3.1 sub-bundle 構成 (源 doc §5)

| sub-bundle | scope | 想定 file 数 | Agent 並列数 | rule 適用 |
|---|---|---|---|---|
| **A1** | per-frame matrix consumer (`modelview_projection_matrix` 67 file 中心) | ~67 (VS 中心) | 4 (class1/deferred + class1/interface + class1/objects + class1/effects+misc 分割) | (A) FrameViewProj UBO 注入 |
| **A2** | per-frame light/atmosphere consumer | ~30 (deferred FS + sky shader 中心) | 2 | (B) FrameLights + (C) FrameAtmosphere UBO 注入 |
| **A3** | per-frame sampler (depthMap / shadowMap / environmentMap 等) | ~30 (A2 と一部 overlap) | 2 | (D) sampler binding qualifier |
| **A4** | per-material core (`texture_matrix0` 39 file + `diffuseMap` 41 file overlap) | ~60 unique | 4 (class1/deferred + class1/objects + interface + gltf 分割) | (E) MaterialUBO + (F) Diffuse/Normal/Spec |
| **A5** | per-material extension sampler (bumpMap / emissiveMap / PBR terrain / probe / post utility) | ~40 | 2 | (F) extension binding |
| **A6** | per-draw (skinning + light file color/proj_mat + clipPlane + size) | ~15 | 1 | (G) PerDrawUBO + push_constant scaffolding |
| **A7** | pure helper (sampler 0 + uniform 0 の 46 file) | ~46 | n/a (no-op) | none (bundle-C entry point pass 委譲) |
| **合計** | 224 effective patch target (237 − 13 skip) | | ~15 Agent | |

### §3.2 cadence (1 sub-bundle = 1 AYA review gate = 1 commit)

```
sub-bundle 着手境界毎:
  1. Claude: skip list (`docs/specs/ayastorm-r41-gl-removal/bundle-A-skip-list.txt` 13 file) + binding rules (`/tmp/bundle-A-binding-rules.md` §2 当該 rule literal) を Agent prompt に注入
  2. Claude: Agent 並列 patch 投入 (~2-4 Agent / sub-bundle)
  3. Claude: 全 Agent 完遂後 patch 結果 self-verify (skip list 違反 0 + UBO decl byte-for-byte canonical + grep で patch 適用件数集計)
  4. Claude: AYA build verify + launch verify 依頼 (1 sub-bundle 完遂時点で incremental SPIR-V 生成成功率 measurement)
  5. AYA: launch verify PASS 報告
  6. Claude: 検証用 LL_INFOS log 除去 (feedback_remove_verification_logs)
  7. AYA: 「OK commit して」明示指示
  8. Claude: 1 sub-bundle = 1 commit (commit message 範式 = β-2-hook complete `fcf2b6c508` 継承)
```

**1 sub-bundle = 1 commit** が原則。A1-A6 で **6 commit + bundle-A-verify 1 commit + bundle-A-handoff 1 commit = 計 8 commit** 想定。

### §3.3 推奨 task cadence (本 prep 完遂後)

```
1. bundle-A-prep (本 handoff doc 起草) — 完遂 (本 commit で AYA review + doc-only commit)
2. bundle-A-A1 (per-frame matrix UBO 注入、67 file、4 Agent 並列)
3. bundle-A-A2 (per-frame light/atmosphere UBO 注入、30 file、2 Agent 並列)
4. bundle-A-A3 (per-frame sampler binding、30 file、2 Agent 並列)
5. bundle-A-A4 (per-material core、60 file、4 Agent 並列)
6. bundle-A-A5 (per-material extension sampler、40 file、2 Agent 並列)
7. bundle-A-A6 (per-draw、15 file、1 Agent)
8. bundle-A-verify (build + AYA launch verify + SPIR-V 生成成功率 measurement + cache file 数確認)
9. bundle-A-handoff (bundle-A 完遂 handoff doc 起草、bundle-B 着手境界明示)
```

**推定工期**: 1 sub-bundle ~半日-1 日 (Agent 並列で短縮)、bundle-A 全体 ~3-5 日 (AYA verify cycle 含む)。

---

## §4 skip list 13 file 機械的 exclusion mechanism

### §4.1 skip list 内容 (源 doc §6)

```
# Picker (2)
indra/newview/app_settings/shaders/class1/deferred/fsObjectIDV.glsl
indra/newview/app_settings/shaders/class1/deferred/fsObjectIDF.glsl

# Cinematic BD (2)
indra/newview/app_settings/shaders/cinematic_bd/class1/deferred/shadowUtil.glsl
indra/newview/app_settings/shaders/cinematic_bd/class3/deferred/screenSpaceReflUtil.glsl

# Visual Realism (7)
indra/newview/app_settings/shaders/class1/deferred/godraysV.glsl
indra/newview/app_settings/shaders/class1/deferred/godraysF.glsl
indra/newview/app_settings/shaders/class1/deferred/volumetricLightF.glsl
indra/newview/app_settings/shaders/class3/deferred/volumetricLightF.glsl
indra/newview/app_settings/shaders/class1/deferred/blurLightV.glsl
indra/newview/app_settings/shaders/class1/deferred/blurLightF.glsl
indra/newview/app_settings/shaders/class1/windlight/atmosphericsFuncs.glsl

# Exemplar β-1 PoC (2) — bundle-A scope frozen
indra/newview/app_settings/shaders/class1/deferred/diffuseV.glsl
indra/newview/app_settings/shaders/class1/deferred/diffuseF.glsl
```

### §4.2 enforcement 3 段階

1. **集中管理**: `docs/specs/ayastorm-r41-gl-removal/bundle-A-skip-list.txt` (13 path 1 行 1 file、repo 内 source-of-truth、AYA 「移動してもいい」承認下で本 prep doc commit と同時 add) を bundle-A 全 Agent patch session で source-of-truth 化
2. **Agent prompt 必須注入**: 全 patch Agent prompt に skip list path + 「edit 適用前に target path が skip list に含まれないか必ず照合、含まれる場合 SKIPPED 報告で次 file に進む」instruction 明示
3. **commit 前 verify Bash** (Claude が sub-bundle commit 前に必ず実行):

```bash
# Pre-commit: skip list integrity check
while read -r f; do
  [ -z "$f" ] || [[ "$f" =~ ^# ]] && continue
  if grep -q "LL_VULKAN_GLSL" "$f"; then
    echo "VIOLATION: $f was modified" >&2
    exit 1
  fi
done < <(grep -v '^#' docs/specs/ayastorm-r41-gl-removal/bundle-A-skip-list.txt | grep -v '^$')
echo "Skip-list integrity: PASS (13 files untouched)"

# Post-commit: git diff cross check
git diff <bundle-A-base>..HEAD --name-only | \
  grep -F -f <(grep -v '^#' docs/specs/ayastorm-r41-gl-removal/bundle-A-skip-list.txt | grep -v '^$') && \
  { echo "VIOLATION: skip-list file modified in commit"; exit 1; } || \
  echo "Commit integrity: PASS"
```

---

## §5 patch 実装テンプレート (LL_VULKAN_GLSL macro switch)

### §5.1 uniform → UBO 化 sample (rule (E) MaterialUBO)

**before** (`class1/deferred/diffuseV.glsl` exemplar からの抽出例、bundle-A 適用前):

```glsl
uniform mat4 modelview_projection_matrix;
uniform mat4 texture_matrix0;
```

**after** (bundle-A patch 適用):

```glsl
#ifdef LL_VULKAN_GLSL
layout(set=0, binding=0, std140) uniform FrameViewProj {
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    mat4 projection_matrix;
    // ... (binding rules §2 (A) literal canonical UBO decl)
};
layout(set=1, binding=0, std140) uniform MaterialUBO {
    mat4 texture_matrix0;
    mat4 texture_base_color_transform;
    mat4 texture_emissive_transform;
    vec4 color;
    vec4 emissiveColor;
};
#else
uniform mat4 modelview_projection_matrix;
uniform mat4 texture_matrix0;
#endif
```

**body code 不変** (`texture_matrix0 * v` 等 access syntax は両 path 共通)。**UBO decl は `/tmp/bundle-A-binding-rules.md` §2 から byte-for-byte canonical copy** (§7.11 risk 対応)。

### §5.2 sampler binding qualifier sample (rule (D) frame-global / (F) per-material)

**before**:

```glsl
uniform sampler2D diffuseMap;
uniform sampler2D shadowMap0;
```

**after**:

```glsl
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=1) uniform sampler2D diffuseMap;
layout(set=0, binding=8) uniform sampler2DShadow shadowMap0;
#else
uniform sampler2D diffuseMap;
uniform sampler2DShadow shadowMap0;
#endif
```

### §5.3 push_constant sample (VS file のみ、(G) 一部)

**after**:

```glsl
#ifdef LL_VULKAN_GLSL
layout(push_constant) uniform PushConstants {
    mat4 modelMatrix;
};
#else
uniform mat4 modelMatrix;  // 実 push 配線は γ'-port-γ PSO scope、bundle-A は宣言のみ
#endif
```

### §5.4 patch 範式 hard rule (源 doc §1 + §7.11)

1. UBO 宣言 block は `/tmp/bundle-A-binding-rules.md` §2 から literal copy、Agent 改変禁止
2. `#ifdef LL_VULKAN_GLSL` / `#else` (legacy `uniform`) / `#endif` の 3 段 swap pattern 厳守、body code untouched
3. binding 番号 (`set=N/binding=M`) は §2 表に従い、Agent 独自割当禁止
4. 1 file 1 patch、別 file 跨りの reorder/refactor 禁止 (1 sub-bundle = 1 commit boundary 維持)
5. 適用前に skip list 13 file 照合必須、含まれる file は SKIPPED 報告

---

## §6 measurement plan (incremental SPIR-V 生成成功率)

### §6.1 sub-bundle 完遂時点別 measurement 期待値

| 完遂 sub-bundle | parse pass 率 期待 | link pass 率 期待 | SPIR-V 生成成功率 期待 |
|---|---|---|---|
| A1 単独 | 微増 (matrix uniform 起因 parse fail 解消、全体の ~25%) | 不変 (location qualifier 不足で link 段で fail) | 0% (link 段未通過) |
| A1-A3 | 中増 (frame-global uniform/sampler 全件解消、~60%) | 不変 (location qualifier 不足継続) | 0% |
| **A1-A6 (bundle-A 完遂)** | **大増 (material/per-draw 全件解消、~95%)** | 不変 (location qualifier 不足継続) | **0% controlled** (bundle-B/C 未着手で >>0% 期待しない) |
| A1-A6 + bundle-B 完遂 | ~95% 維持 | 増 (location qualifier 解消) | 増 (entry point/extension で fail 残存) |
| A1-A6 + bundle-B + bundle-C 完遂 | ~100% | ~100% | **>>0% 目標** |

### §6.2 measurement 取得手順 (1 sub-bundle 完遂時点で実施)

```bash
# 1. AYA launch (clean shutdown)
# 2. Claude: hook fire 件数集計
grep -c "generatePerProgramSPIRV" ~/.ayastorm_x64/logs/AYAstorm.log

# 3. Claude: parse fail 件数集計 (stage type 別 breakdown)
grep "glslang parse failed" ~/.ayastorm_x64/logs/AYAstorm.log | sort | uniq -c

# 4. Claude: link fail 件数集計
grep "glslang link failed" ~/.ayastorm_x64/logs/AYAstorm.log | wc -l

# 5. Claude: SPIR-V cache file 数集計
ls ~/.ayastorm_x64/cache/shader_cache/*_program.spv 2>/dev/null | wc -l

# 6. Claude: 1 sub-bundle 完遂時点 metric 表化、handoff doc 反映
```

---

## §7 risks / caveats

| # | risk | mitigation |
|---|---|---|
| **§7.1** | `std140` UBO padding で実 size 拡大 (`maxUniformBufferRange` Vulkan 最小 16 KiB)。skin 110 joint 時 PerDrawUBO ~10.6 KiB tight | `MAX_JOINTS_PER_MESH_OBJECT` 値を A6 着手前 audit、110 確認時 tight 但し未超過 |
| **§7.2** | `texture_matrix0` 等の dual decl path で legacy `uniform` と UBO member の identifier collision suspect | body code 不変 (両 path で同一識別子 access) で衝突なし、`#ifdef` swap で path 分離担保 |
| **§7.3** | `color` の dual context は file path regex routing で解消、別 directory 配置時誤分類 risk | Agent prompt に literal table 注入 (`(pointLight\|spotLight\|deferredUtil)F\.glsl$` → set=2 / else → set=1)、未知 context は fail-stop |
| **§7.4** | long-tail uniform (top-30 外) は §3.6 catch-all で PerDrawUBO (G) 配置、高頻度未捕捉 risk | sub-bundle 完遂毎 `/tmp/bundle-A-uniform-enum.txt` regen、top-30 新規 entry 検出時 promote + re-patch |
| **§7.5** | `MAX_JOINTS_PER_MESH_OBJECT` は `#define`、複数 program で値異 (legacy 45 vs PBR 110) | per-program SPIR-V で isolate、単一 program 内重複 decl 不可、A6 patch 時 audit |
| **§7.6** | `set=1/binding=20-39` PBR terrain detail 20 slot sparse occupancy、Vulkan validation warning | `VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT` (γ'-port-γ PSO scope)、bundle-A は宣言のみ |
| **§7.7** | AYAstorm picker output buffer reserve (`set=0/binding=22-23`) は symbolic only、将来拡張で renumber risk | sub-doc 07 §3.1 sub-step 7.2 改訂 + handoff 経由でのみ renumber 可、本 prep doc は symbolic 明記 |
| **§7.8** | `clipPlane` を per-draw (G) 配置、実は per-frame (`LLPipeline::setupClipPlane`) 可能性 | 安全側上界として per-draw 採用、`LLPipeline` trace で per-frame 確認時 (B) 昇格、file-local block で descriptor compat 維持 |
| **§7.9** | `atmosphericsFuncs.glsl` (untouched) include 時 FrameAtmosphere UBO 二重定義 parse error | **bundle-C 委譲** = 新規 `atmosphericsUBO.glsl` 分離 (Vulkan-only)、bundle-A の A2 patch 時に include 持つ file に flag 立て、A2 patch 範囲外として bundle-C で処理 |
| **§7.10** | `/tmp/bundle-A-217-files.txt` filename historical (内容 237)、downstream 混乱 risk | 全 doc で literal "237" 表記、rename は次 maintenance window |
| **§7.11** | Agent 並列 patch の semantic 不整合 (member ordering / padding 差異) | `/tmp/bundle-A-binding-rules.md` §2 UBO decl block を byte-for-byte canonical copy、Agent 改変禁止を prompt に明示 |
| **§7.12** | per-program SPIR-V 生成 (γ'-port-β-2-hook) で descriptor set layout は per-file 派生、C++ pipeline (γ'-port-γ scope) は `spirv-reflect` or 静的 table から build 必要 | 本 prep doc + binding rules doc が γ'-port-γ-prep への source-of-truth、handoff cross-reference 担保 |
| **§7.13** | bundle-A 単独 SPIR-V 生成成功率 0% controlled = bundle-A 単独で「動かない」評価誤読 risk | §6.1 表に明示 = 0% は controlled、>>0% は bundle-A+B+C 完遂時点 |
| **§7.14** | sub-bundle commit 単位の AYA review burden (~8 commit cycle) | 1 sub-bundle ~半日-1 日、AYA review は build + launch verify 程度、過大負荷ではない (β-1 / β-2-hook と同 cadence) |
| **§7.15** | context budget concern (237 file patch 工程は大規模) | sub-bundle 単位 fresh context 推奨 (A1 完遂 → handoff → A2 別 session)、本 prep doc + binding rules doc + skip list の 3 file が source-of-truth で context 復元担保 |
| **§7.16** | 11 file untouched 維持の Agent 違反 risk | §4.2 三段 enforcement (Agent prompt 注入 + pre-commit Bash + post-commit git diff) で機械的 enforcement、commit 前必須 check |

---

## §8 AYA 承認境界 (本 prep 着手前 + 各 sub-bundle commit 境界)

| 項目 | AYA 承認境界 |
|---|---|
| 本 prep doc commit | 「OK」明示 review PASS 下 doc-only commit |
| sub-bundle A1-A6 着手 | AYA 明示指示要 (1 sub-bundle = 1 session 推奨、fresh context) |
| sub-bundle commit | 「OK commit して」明示指示下のみ (feedback_no_auto_commit) |
| 11 file untouched 維持 | §4 三段 enforcement で機械的担保 (Agent 違反は pre-commit Bash で stop) |
| binding 番号変更 | sub-doc 07 §3.1 sub-step 7.2-7.4 改訂 + handoff 経由でのみ可 (bundle-A 内 ad-hoc 変更禁止) |
| sub-doc 03 §3.1.3 exemplar 2 file untouched | 試作レール扱い継続 (bundle-A scope 外、bundle 全完遂後 cleanup commit 別 scope) |

---

## §9 next action

```
1. AYA: 本 prep doc review
2. AYA: 「OK」明示承認下 + Claude commit 実施 (doc-only commit、本 prep doc 1 file + skip list 1 file = 計 2 file 想定、commit message 範式 = β-2-prep `b7e2dbc72e` 継承)
3. Claude: skip list は repo 内 `docs/specs/ayastorm-r41-gl-removal/bundle-A-skip-list.txt` に確定 (AYA 「移動してもいい」承認下、本 prep doc commit と同時 add 済)
4. fresh context 推奨で bundle-A-A1 (per-frame matrix UBO 注入、67 file、4 Agent 並列) 着手
```

---

## §10 cross reference

**spec doc**:
- `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` §1.2.4 (LL_VULKAN_GLSL macro switch 採用根拠 + sample pattern 3 件 + binding 番号既定値) / §3.1 sub-step 6.3 (scope 確定) / §6.5 (改訂履歴)
- `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` §3.1 sub-step 7.2-7.4 (binding 番号確定値 = bundle-A patch 必須遵守) / §6.5 (改訂履歴)
- `docs/specs/ayastorm-r41-gl-removal/AYAstorm-r41-charter.md` §3 #1 (GL/Vulkan acceptance) / §7.5 (boundary refine 履歴)
- `docs/specs/ayastorm-r41-gl-removal/03-runtime-vulkan-bootstrap.md` §3.1.3 (exemplar 試作レール扱い継続)

**handoff doc**:
- `handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md` (commit `cecb9e6467` = 本 prep doc の直前 handoff、§2 β-2-hook 実装範式 + §3.2 着手前チェックリスト 9 件 + §4 risks 12 件 + §6 prompt)
- `handoff-substep-4-3-gamma-prime-port-beta-2-prep.md` (commit `b7e2dbc72e` = β-2 全体 prep doc、§3 axis (b) 217 file bundle 構成 + §6 risks 10 件)
- `handoff-substep-4-3-gamma-prime-port-beta-1-complete.md` (commit `1bce995d87` = exemplar PoC freeze rationale)
- `handoff-substep-4-3-gamma-prime-port-alpha-complete.md` (γ'-port-α exemplar PoC + cache pipeline 範式)

**artifact**:
- `/tmp/bundle-A-217-files.txt` (filename historical、内容 237 patch target、disk 250 − skip 13 = 237)
- `/tmp/bundle-A-uniform-enum.txt` (727 uniform 宣言 / 259 unique / 22 per-frame 名)
- `/tmp/bundle-A-sampler-enum.txt` + `/tmp/bundle-A-sampler-enum-tsv.txt` (193 sampler 宣言 / 91 file / 73 unique 名)
- `/tmp/bundle-A-binding-rules.md` (558 line / 35KB / §1-§8 / binding 割当 (A)-(H) literal canonical / 全 727 uniform + 193 sampler 100% routing 確認)
- `docs/specs/ayastorm-r41-gl-removal/bundle-A-skip-list.txt` (本 prep doc commit と同時 add、13 path + コメント行、repo 内 source-of-truth)

**memory**:
- `project_ayastorm_r41_vulkan_migration.md` (status active = β-2-hook 完遂 + bundle-A-prep 着手境界)

**feedback rules (本 prep doc 起草 + bundle-A 着手で遵守)**:
- `feedback_doubt_self_first` (237 vs 217 不整合 = AYA 提示情報を疑わず自分の trace で根拠取得)
- `feedback_falsification_as_progress` (β-1 link fail → β-2 parse fail 段階進化を architectural progress として記述)
- `feedback_no_scope_shrink` (237 全件 patch、scope 縮小禁止)
- `feedback_proactive_handoff` (本 prep doc 起草自体が能動 handoff)
- `feedback_self_verify_before_handoff` (AYA review 前に Claude 側で全 (A)-(H) routing 確認 + skip list 整合性確認)
- `feedback_one_step_at_a_time` (1 sub-bundle = 1 AYA review gate = 1 commit cadence 厳守)
- `feedback_use_agents_proactively` (sub-bundle 内 Agent 並列 patch 推奨、~15 Agent / bundle)
- `feedback_remove_verification_logs` (commit 前 LL_INFOS hook 除去)
- `feedback_no_claude_coauthor` (commit message に Claude 共著行禁止)
- `feedback_no_auto_commit` (AYA 「OK commit して」明示指示下のみ commit)

**次 action**: sub-step 4.3-γ'-port-β-2-bundle-A-A1 (per-frame matrix UBO 注入、67 file、4 Agent 並列、AYA 明示指示要、fresh context 推奨、本 prep doc + binding rules doc + skip list の 3 file を source-of-truth として注入)
