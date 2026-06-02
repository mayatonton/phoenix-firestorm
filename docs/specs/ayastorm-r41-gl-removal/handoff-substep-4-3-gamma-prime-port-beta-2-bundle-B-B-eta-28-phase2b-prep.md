# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2b prep handoff

**作成日**: 2026-06-02
**branch**: `feature/ayastorm-r41-gl-removal`
**前 handoff**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2a-complete.md` (commit `c53f6e0782` + A-E 整備 `057cd8b299` + checklist `f8144cce28`)
**状態**: Phase 2b **prep 起草**。AYA 承認後 feat 適用 → cold launch verify → complete handoff 起草。

---

## §1 Scope (Phase 2a complete handoff §4 + 本 prep §3-§4 整合)

### 直接対象 4 program (cold launch log 実測、Phase 2a 末 ERROR 22 → Phase 2b 末 ERROR 18 期待)

| binding | UBO 名 | source .glsl | uniform | stage | program 名 (log) | 観測 ERROR 行 |
|---|---|---|---|---|---|---|
| 17 | `PerProgramUBO_GodraysF` | `class1/deferred/godraysF.glsl` | `int aya_r15_godrays_enabled` + `float aya_r15_godrays_phase_exponent` + `float aya_r15_godrays_strength` | F | Godrays Shader | L982 (root) |
| 18 | `PerProgramUBO_VolumetricLightF` | `class3/deferred/volumetricLightF.glsl` | `int godray_res` + `float godray_multiplier` + `float falloff_multiplier` + `float seconds60` | F | AYAstorm Volumetric Light Shader | L1405 (root) |
| 19 | `PerProgramUBO_VelocityAlphaV` | `class1/deferred/velocityAlphaV.glsl` | `mat4 last_object_matrix` | V | AYAstorm Velocity Alpha Shader | L1382 (root) |
| 20 | `PerProgramUBO_PostDeferredF` | `class1/deferred/postDeferredF.glsl` | `float res_scale` + `float chroma_str` | F | Deferred Post Shader | L1292 (root) |

### η-28-C 範式適用 (preventive 拡張、Phase 2a waterHazeF と同型)

| binding | UBO 名 | source .glsl | uniform | stage | 適用理由 |
|---|---|---|---|---|---|
| 20 | `PerProgramUBO_PostDeferredF` (binding 20 共有) | `class1/deferred/postDeferredHQDoFF.glsl` | 同 `float res_scale` + `float chroma_str` | F (共有宣言) | "Deferred Post Shader" は cvar `RenderDepthOfFieldHighQuality` で postDeferredF / postDeferredHQDoFF 切替、**同 program 内 cvar-選択 file variant** → η-28-C 範式の cross-variant extension (cold launch 実測は default cvar=0 で postDeferredF のみ parse、HQ flip 時に postDeferredHQDoFF cascade 浮上見込み) |

**η-28-C cross-variant extension の根拠**: η-28-C 範式 doc (Phase 2a complete §7 + reference doc §6 末) は **program 跨ぎ V/F pair 同 UBO** を定式化。本 case は **同 program V/F でなく、同 program 内 cvar-selected F file variant** (postDeferredF ↔ postDeferredHQDoFF) で同名 uniform を共有する状況。Phase 2a waterHazeF の予防修正と同型 (literal scope ≠ cold launch parse 対象 = preventive coverage)。

### 合計実装 file 数: 5 file (Phase 2a 同様、literal 4 + preventive 1)

---

## §2 命名規約と alignment 設計 (reference doc §4-B + §6 命名規約に整合)

### §2.1 binding 17: `PerProgramUBO_GodraysF` (godraysF.glsl)

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2b:
//   AYAstorm r15 cvar 群 (aya_r15_godrays_*) を UBO 化。godraysV は uniform 不使用 = F 単独 attach。
#ifndef PER_PROGRAM_UBO_GODRAYS_F_DEFINED
#define PER_PROGRAM_UBO_GODRAYS_F_DEFINED 1
layout(set=2, binding=17, std140) uniform PerProgramUBO_GodraysF {
    int   aya_r15_godrays_enabled;          // offset 0,  size 4 + 12 pad
    float aya_r15_godrays_phase_exponent;   // offset 16, size 4 + 12 pad
    float aya_r15_godrays_strength;         // offset 32, size 4 + 12 pad
};  // total 48
#endif
#else
uniform int aya_r15_godrays_enabled;
uniform float aya_r15_godrays_phase_exponent;
uniform float aya_r15_godrays_strength;
#endif
```

**host C++ 整合 (llsettingsvo.cpp:996-1007, llshadermgr.cpp:1603-1605)**: `LLSettingsVOSky::applyToShader` 内で `shader->uniform1i(LLShaderMgr::AYA_R15_GODRAYS_ENABLED, ...)` / `uniform1f(LLShaderMgr::AYA_R15_GODRAYS_PHASE_EXPONENT, ...)` / `uniform1f(LLShaderMgr::AYA_R15_GODRAYS_STRENGTH, ...)` で push、reserved uniform name は GLSL 名と完全一致。UBO 化後も `LLGLSLShader::uniform1i/1f` の name-lookup path は block member 解決可能 (η-3 以降 sub-bundle で同パターン継続検証済)。

**cross-stage check**: `godraysV.glsl` (vert) は `position` attribute のみ、`aya_r15_*` uniform 不使用 → η-28-C V+F 共有 宣言不要、F 単独 attach で完結。

### §2.2 binding 18: `PerProgramUBO_VolumetricLightF` (class3/deferred/volumetricLightF.glsl)

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2b:
//   BD-borrow godrays composite uniform を UBO 化。host = pipeline.cpp doRenderGodrays
//   (uniform1i GODRAY_RES / uniform1f GODRAY_MULTIPLIER / uniform1f FALLOFF_MULTIPLIER)。
//   seconds60 は宣言のみで本体未使用 (BD legacy)、host setter も無いが parse 通過のため UBO 含める。
#ifndef PER_PROGRAM_UBO_VOLUMETRIC_LIGHT_F_DEFINED
#define PER_PROGRAM_UBO_VOLUMETRIC_LIGHT_F_DEFINED 1
layout(set=2, binding=18, std140) uniform PerProgramUBO_VolumetricLightF {
    int   godray_res;            // offset 0,  size 4 + 12 pad
    float godray_multiplier;     // offset 16, size 4 + 12 pad
    float falloff_multiplier;    // offset 32, size 4 + 12 pad
    float seconds60;             // offset 48, size 4 + 12 pad (BD legacy dead uniform、host setter なし)
};  // total 64
#endif
#else
uniform int godray_res;
uniform float godray_multiplier;
uniform float falloff_multiplier;
uniform float seconds60;
#endif
```

**host C++ 整合 (pipeline.cpp:5088-5094, llshadermgr.cpp:1883-1885)**: `LLPipeline::doRenderGodrays` 内で `gVolumetricLightProgram.uniform1i(LLShaderMgr::GODRAY_RES, ...)` / `uniform1f(LLShaderMgr::GODRAY_MULTIPLIER, ...)` / `uniform1f(LLShaderMgr::FALLOFF_MULTIPLIER, ...)`。

**seconds60 ステータス**: 宣言は class3/deferred/volumetricLightF.glsl L126 にあるが、main() 本体未使用 (grep 全件 0)。host C++ も setter 無し (reserved uniform 登録なし)。**parse error 解消のため UBO に含めるが、実 binding は未配線**。本 phase scope では「declared/parse-OK だが未使用」状態維持、将来削除提案は Phase 2c+ 以降の cleanup phase へ送る。

**cross-stage check**: V pair は `postDeferredNoTCV.glsl` (共有 fullscreen V)。同 V は `position` + `FrameViewProj` のみで godray_* 不使用 → η-28-C 適用不要、F 単独。

**class1 vs class3**: cold launch で実 parse 対象は **class3** (`mShaderLevel[SHADER_DEFERRED] >= 3` で class3 が選ばれる)。class1/deferred/volumetricLightF.glsl は stub (`diffuseRect` 1 texture lookup のみ、plain uniform 無し) なので UBO 編集不要。**Phase 2b は class3 のみ編集**。

### §2.3 binding 19: `PerProgramUBO_VelocityAlphaV` (velocityAlphaV.glsl)

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2b:
//   last_object_matrix mat4 を UBO 化。velocityAlphaF は plain uniform 無し、
//   skinnedVelocityAlphaV は last_object_matrix 不使用 (skin matrix 経由)、本 V 単独 attach。
#ifndef PER_PROGRAM_UBO_VELOCITY_ALPHA_V_DEFINED
#define PER_PROGRAM_UBO_VELOCITY_ALPHA_V_DEFINED 1
layout(set=2, binding=19, std140) uniform PerProgramUBO_VelocityAlphaV {
    mat4 last_object_matrix;     // offset 0, size 64 (4 × vec4 配置、std140 素直)
};  // total 64
#endif
#else
uniform mat4 last_object_matrix;
#endif
```

**host C++ 整合**: `last_object_matrix` は `LLShaderMgr::LAST_OBJECT_MATRIX` reserved uniform 経由 (要 grep 再確認、本 prep §6 self-verify 参照)、velocity 系 program で per-draw 更新。

**cross-stage check**:
- `velocityAlphaF.glsl` (frag) は `diffuseLookup` (sampler 経由) + location-bound varying のみ、plain uniform 無し → 編集不要
- `skinnedVelocityAlphaV.glsl` (Skinned 変種、別 program `gVelocityAlphaSkinnedProgram`) は `last_object_matrix` 不使用 (`getLastObjectSkinnedTransform()` 経由) → 編集不要
- 別 program `velocityV.glsl` は同名 uniform を η-6 で **`VelocityVParamUBO_Legacy` set=3 binding=55** に UBO 化済 (program 単位 descriptor、本 binding 19 とは別 descriptor)

**Skinned 変種 cold launch 結果**: log L1390 で SPIR-V 生成成功 (HAS_SKIN permutation で `last_object_matrix` 参照無効化、parse 通過)。**Phase 2b 編集対象は非 Skinned 変種 velocityAlphaV.glsl のみ**。

### §2.4 binding 20: `PerProgramUBO_PostDeferredF` (postDeferredF.glsl + postDeferredHQDoFF.glsl、η-28-C cross-variant 共有)

#### `class1/deferred/postDeferredF.glsl` (cold launch parse 対象、cvar default=false)

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2b:
//   res_scale / chroma_str を UBO 化。"Deferred Post Shader" program は cvar
//   RenderDepthOfFieldHighQuality で postDeferredF / postDeferredHQDoFF を切替、
//   両 file は同名 uniform を共有 → 同 UBO + 同 binding を両方に宣言 (η-28-C cross-variant)。
#ifndef PER_PROGRAM_UBO_POST_DEFERRED_F_DEFINED
#define PER_PROGRAM_UBO_POST_DEFERRED_F_DEFINED 1
layout(set=2, binding=20, std140) uniform PerProgramUBO_PostDeferredF {
    float res_scale;   // offset 0,  size 4 + 12 pad
    float chroma_str;  // offset 16, size 4 + 12 pad
};  // total 32
#endif
#else
uniform float res_scale;
uniform float chroma_str;
#endif
```

#### `class1/deferred/postDeferredHQDoFF.glsl` (cvar flip 時 parse 対象、preventive)

同一 UBO ブロック宣言 (set=2 binding=20、guard 名 `PER_PROGRAM_UBO_POST_DEFERRED_F_DEFINED`)。

**host C++ 整合**: `res_scale` / `chroma_str` は host 側 `LLShaderMgr::RES_SCALE` / `CHROMA_STRENGTH` (要 grep 再確認、本 prep §6) reserved uniform 経由。

**naming 判断**: UBO 名は `PerProgramUBO_PostDeferredF` (default-attach file owner) を採用。HQDoFF はバリアント、命名上 owner 化しない。reference doc §6 命名規約 (PerProgramUBO_<Name><Stage>) の `<Name>` は postDeferredF (源 file 名)、Stage suffix `F` 重複は意図的 (file 名末尾 F + stage F suffix が偶然重なるが、reference doc §6 命名規約は `<Name>` 自由、衝突なし)。

**cross-stage check**: V pair は `postDeferredNoTCV.glsl` (共有、godrays composite と同)、`res_scale` / `chroma_str` 不使用 → η-28-C 適用不要、F-only。

**cold launch 実観測**:
- log L1290 "Deferred Post Shader" frag parse failed → postDeferredF.glsl (cvar default=false)
- postDeferredHQDoFF.glsl は cold launch で parse 対象外 (cvar default flow)
- preventive 修正 = Phase 2a waterHazeF と同型、Δ への影響なし (cold launch で parse されないため)、AYA cvar flip 時の cascade 防止

---

## §3 期待 ERROR Δ + cascade reveal 観察計画

### Phase 2a 末 → Phase 2b 末 期待値 (cold launch log 実観測ベース)

| 観測項目 | Phase 2a 末 | Phase 2b 末 期待 | Δ |
|---|---|---|---|
| ERROR 行数 | 22 | **18** | **-4** |
| parse failure event 数 | 14 | 10 | -4 |
| link failed | 0 維持 | 0 維持 | ZERO 継続 (10 sub-bundle 連続) |
| 4 target program SPIR-V 生成 | (parse fail) | 全成功 | +4 |
| postDeferredHQDoFF parse error (preventive) | 出ない (cvar 未 flip) | 出ない | - |
| clean shutdown | 維持 | 維持 | - |

### Phase 2b 直接 ERROR 消滅対象 (cold launch log 行 → file 対応)

| log L | program | stage | 該当 ERROR (Phase 2a 末) | 該当 UBO (Phase 2b 末) |
|---|---|---|---|---|
| L982 | Godrays Shader | F | `non-opaque uniforms outside a block` (aya_r15_godrays_enabled @ line 2271 transformed) | PerProgramUBO_GodraysF (binding 17) |
| L1292 | Deferred Post Shader | F | `non-opaque uniforms outside a block` (res_scale @ line 1789 transformed) | PerProgramUBO_PostDeferredF (binding 20) |
| L1382 | AYAstorm Velocity Alpha Shader | V | `non-opaque uniforms outside a block` (last_object_matrix @ line 398 transformed) | PerProgramUBO_VelocityAlphaV (binding 19) |
| L1405 | AYAstorm Volumetric Light Shader | F | `non-opaque uniforms outside a block` (godray_res @ line 2566 transformed) | PerProgramUBO_VolumetricLightF (binding 18) |

各 program は **root ERROR 1 件のみ、cascade 派生なし** (Phase 2a Skinned PBR Alpha / SpotLight のような chain 構造は本 4 program では log 上見られない)。Phase 2a の cascade reveal +2 と異なり、**Δ -4 で event 数も clean に -4 揃う見込み**。

### Phase 2c/2d 残予測 (η-28 Phase 2a §4 を更新せず、Phase 2b 末で再評価)

Phase 2a 末で予測した Phase 2c (cofF / blurLightF / waterF / pbrterrainV) + Phase 2d (pbralpha cascade / pointLight / spotLight MULTI cascade) は **Phase 2b 末 cold launch log の cascade reveal 状況 = 0 件かどうか** を見て scope 再評価。Phase 2b で予防的に postDeferredHQDoFF を含める判断は Phase 2c で同型判断の前例にしうる (cofF vs CoF Shader 変種 / waterF vs underwater 変種等で類例調査要)。

---

## §4 編集対象 file (literal 5 file、η-28-C preventive 1 含む)

### 編集

1. `indra/newview/app_settings/shaders/class1/deferred/godraysF.glsl`
   - L114 / L119 / L120 (plain `uniform int aya_r15_godrays_enabled;` 等 3 行) を `#ifdef LL_VULKAN_GLSL` UBO block + `#else` plain で wrap
2. `indra/newview/app_settings/shaders/class3/deferred/volumetricLightF.glsl`
   - L122-L126 (plain `uniform int godray_res;` 等 4 行) を `#ifdef LL_VULKAN_GLSL` UBO block + `#else` plain で wrap
3. `indra/newview/app_settings/shaders/class1/deferred/velocityAlphaV.glsl`
   - L52 (plain `uniform mat4 last_object_matrix;`) を `#ifdef LL_VULKAN_GLSL` UBO block + `#else` plain で wrap
4. `indra/newview/app_settings/shaders/class1/deferred/postDeferredF.glsl`
   - L98 / L101 (plain `uniform float res_scale;` / `uniform float chroma_str;`) を `#ifdef LL_VULKAN_GLSL` UBO block + `#else` plain で wrap
5. `indra/newview/app_settings/shaders/class1/deferred/postDeferredHQDoFF.glsl`
   - L114 / L116 (plain `uniform float res_scale;` / `uniform float chroma_str;`) を `#ifdef LL_VULKAN_GLSL` UBO block + `#else` plain で wrap (postDeferredF と **同 UBO 名 + 同 binding + 同 guard 名**、η-28-C cross-variant)

### 非編集 (cross-stage check で確認済)

- `class1/deferred/godraysV.glsl` (`aya_r15_*` uniform 不使用)
- `class1/deferred/volumetricLightF.glsl` (stub、plain uniform 無し)
- `class1/deferred/postDeferredNoTCV.glsl` (`res_scale` / `chroma_str` / `godray_*` / `last_object_matrix` 全て不使用)
- `class1/deferred/velocityAlphaF.glsl` (plain uniform 無し)
- `class1/deferred/skinnedVelocityAlphaV.glsl` (`last_object_matrix` 不使用、Skinned skin matrix 経由)
- `class1/deferred/velocityV.glsl` (η-6 で別 program の VelocityVParamUBO_Legacy set=3 binding=55 に UBO 化済)

---

## §5 reference-shader-location-map.md §6-A 更新計画 (本 commit 同梱)

Phase 2b feat commit と同 commit (または直後 docs commit) で `reference-shader-location-map.md` §6-A 表に以下 4 行追加:

| set | binding | UBO 名 | stage | 起源 sub-step |
|---|---|---|---|---|
| 2 | 17 | PerProgramUBO_GodraysF | F | η-28 Phase 2b |
| 2 | 18 | PerProgramUBO_VolumetricLightF | F | η-28 Phase 2b |
| 2 | 19 | PerProgramUBO_VelocityAlphaV | V | η-28 Phase 2b |
| 2 | 20 | PerProgramUBO_PostDeferredF | F (+ HQDoFF preventive) | η-28 Phase 2b |

「(空き、η-28 Phase 2b+ 連番継続)」行 (現 binding 17+) を 21+ へ繰り下げ。

stage 列 retroactive 注釈は Phase 2a で既追加、本 Phase 2b 4 行は **§6-A stage 列の読み方** (`V` / `F` / `V+F` / `(per-draw)`) に従って明示。binding 20 は **literal stage = F、preventive scope の HQDoFF も F、同 program 内 cvar 切替** = cross-variant、stage 列は `F` 単独表記とし、本 prep §2.4 / §3 で「postDeferredHQDoFF 共有」と明示。

---

## §6 Phase 2b 着手前 self-verify (Claude 側、AYA "OK" 後の deploy 前)

### §6-A reserved uniform 名整合 (host C++ vs GLSL)

deploy 前に Claude が再確認 (本 prep §2.x の host 整合記述の裏取り):

| GLSL 名 | LLShaderMgr enum (要 grep) | host C++ setter 場所 (要 grep) | Phase 2b deploy 前確認 |
|---|---|---|---|
| `aya_r15_godrays_enabled` | AYA_R15_GODRAYS_ENABLED ✓ (llshadermgr.h:130 / llshadermgr.cpp:1603) | llsettingsvo.cpp:1001 ✓ | OK |
| `aya_r15_godrays_phase_exponent` | AYA_R15_GODRAYS_PHASE_EXPONENT ✓ (llshadermgr.h:131 / llshadermgr.cpp:1604) | llsettingsvo.cpp:1005 ✓ | OK |
| `aya_r15_godrays_strength` | AYA_R15_GODRAYS_STRENGTH ✓ (llshadermgr.h:132 / llshadermgr.cpp:1605) | llsettingsvo.cpp:1006 ✓ | OK |
| `godray_res` | GODRAY_RES ✓ (llshadermgr.h:402 / llshadermgr.cpp:1883) | pipeline.cpp:5092 ✓ | OK |
| `godray_multiplier` | GODRAY_MULTIPLIER ✓ (llshadermgr.h:403 / llshadermgr.cpp:1884) | pipeline.cpp:5093 ✓ | OK |
| `falloff_multiplier` | FALLOFF_MULTIPLIER ✓ (llshadermgr.h:404 / llshadermgr.cpp:1885) | pipeline.cpp:5094 ✓ | OK |
| `seconds60` | (未登録、host setter なし) | (なし、BD legacy dead) | **dead uniform、UBO 含めるが host 連動なし** |
| `last_object_matrix` | LAST_OBJECT_MATRIX (要 grep 再確認) | velocity 系描画 path (要 grep 再確認) | **deploy 前 grep** |
| `res_scale` | RES_SCALE 系 (要 grep 再確認) | DoF post 系描画 path (要 grep 再確認) | **deploy 前 grep** |
| `chroma_str` | CHROMA_STRENGTH 系 (要 grep 再確認) | DoF post 系描画 path (要 grep 再確認) | **deploy 前 grep** |

`last_object_matrix` / `res_scale` / `chroma_str` は本 prep §2 で「reserved uniform 経由」「DoF post 系」と推定記述したが、deploy 前に Claude が個別 grep で確定確認する (`feedback_self_verify_before_handoff`)。

### §6-B 5 file edit 後の cross-check

deploy 前に Claude が再 read で以下確認:
- UBO ブロック宣言の closing `};` 直後の `#endif` (=`#ifndef PER_PROGRAM_UBO_*_DEFINED` の対) ✓
- 4 階層 nesting (`LL_VULKAN_GLSL` / `_DEFINED` guard / `layout` / closing) reference doc §6 命名規約セクションの shadowCubeV 範式と完全同型 ✓
- plain uniform 削除漏れなし (`#else` 側に moved 確認)
- file 全体での同名 uniform 再宣言なし (`PerProgramUBO_*` 名 grep で本 file のみ唯一宣言)

### §6-C cold launch verify checklist (reference doc §8 cookbook の Phase 2b 起点)

```bash
# 期待値: ERROR 22 → 18, link 0, clean shutdown
grep -cE "^ERROR: 0:" ~/.ayastorm_x64/logs/AYAstorm.log    # 期待 18
grep -c "glslang parse failed" ~/.ayastorm_x64/logs/AYAstorm.log  # 期待 10
grep -c "link failed\|link error" ~/.ayastorm_x64/logs/AYAstorm.log  # 期待 0
grep "Shutting down" ~/.ayastorm_x64/logs/AYAstorm.log   # 存在確認

# 4 target program 全 SPIR-V 生成成功 grep
grep -E "generatePerProgramSPIRV.*for program (Godrays Shader|AYAstorm Volumetric Light Shader|AYAstorm Velocity Alpha Shader|Deferred Post Shader)" ~/.ayastorm_x64/logs/AYAstorm.log

# preventive postDeferredHQDoFF parse error 出ない確認 (cvar default で parse 対象外)
grep "RenderDepthOfFieldHighQuality" ~/.ayastorm_x64/logs/AYAstorm.log  # cvar value 確認用 (default 0 期待)
```

### §6-D cascade reveal 観察 (Phase 2a 教訓: cascade event 数 +2 reveal あり)

Phase 2a は cascade reveal +2 を観測 (PBR Alpha V cascade + SpotLight F MULTI cascade)。Phase 2b 直接対象 4 file は **root ERROR 1 件 / cascade chain なし** (cold launch log §3 で確認済) なので **理論上 cascade reveal 0 件、Δ event 数も clean -4 揃う見込み**。

ただし cold launch verify で Δ event ≠ -4 (+1/+2 等) が出た場合は reference doc §7 cascade chain 同定 protocol で新規 chain を trace、Phase 2c/2d scope 再評価。

---

## §7 範式継承 + 本 phase 適用範式

### 継承 (Phase 2a 以前から)

- `feedback_one_step_at_a_time` (verify 1 ステップずつ)
- `feedback_no_scope_shrink` (literal 5 file scope = AYA 承認 scope、shrink 禁止)
- `feedback_doubt_self_first` (本 prep §2.4 で prep doc literal 「HQDoFF」が cold launch parse 対象でないことを self-grep で発見、prep doc 起草者の前 session 想定を疑い)
- `feedback_render_full_trace_first` (uniform を shader tree 全体 + host C++ + reserved uniform 表 で trace、推論禁止)
- `feedback_no_auto_commit` (AYA "OK" 明示後に feat commit)
- `feedback_self_verify_before_handoff` (本 prep §6 self-verify で AYA cold launch 浪費を防ぐ)
- `feedback_proactive_handoff` (本 prep doc 自体)
- **η-28-A** (dump marker 信用せず source tree grep) — 本 prep §2 全 file の uniform grep で適用
- **η-28-B** (既存 transformed dump で再 cold launch 不要判定) — Phase 2a 末 transformed dump で current state 確認、本 prep 起草前に再 cold launch せず

### 本 phase 適用 (新規範式提案なし、η-28-C 拡張のみ)

- **η-28-C cross-variant extension** (program 跨ぎ cvar-selected file variant も同 UBO 共有): postDeferredF ↔ postDeferredHQDoFF。Phase 2a waterHazeF V+F 共有 (program 内 V/F stage) に対し、本 case は **program 内 cvar-selected F file variant 共有**。範式の本体は同じ ("同 program 内で同名 uniform を使う複数の宣言地点は同 UBO + 同 binding + 同 guard で揃える")、適用対象が V/F → cvar-variant に拡張。

  - **適用条件**: 同 program で `mShaderFiles.push_back` が cvar 分岐で異なる .glsl を選び、両 .glsl が同名 plain uniform を持つ場合
  - **対応**: 両 .glsl に同一 UBO 名 + binding + guard 名で宣言、Vulkan 側は **descriptor 1 個共有** (どちらの cvar 値でも同じ binding が見える)
  - **本 prep への記述位置**: §1 (preventive 表) + §2.4 (本 phase 実例) + §7 (範式継承)

  η-28-C 範式定式化 (Phase 2a complete §7 + reference doc §6 末) に cross-variant ケースを Phase 2b complete handoff で追記予定。本 prep ではまだ範式 doc 本体は更新せず、Phase 2b complete handoff で実証後に追記。

---

## §8 落穂拾い + Phase 2b 完了 checklist

- [ ] AYA prep doc レビュー + "OK" 明示
- [ ] §4 の 5 file UBO 化 feat commit
- [ ] reference-shader-location-map.md §6-A 表に 4 行追加 (binding 17-20) — feat commit 同梱 or 直後 docs commit
- [ ] deploy + shader cache clear (~/.ayastorm_x64/cache/shader_cache/ 全 clear)
- [ ] AYA cold launch + log 採取 (`~/.ayastorm_x64/logs/AYAstorm.log`)
- [ ] Claude self-verify: ERROR 18 / parse failure 10 / link 0 / SPIR-V 4 program 全成功 / clean shutdown (本 prep §6-C cookbook で確認)
- [ ] Phase 2b complete handoff 起草 (§4 Phase 2c 表 + Phase 2d 表 を本 phase 末 log で更新、η-28-C cross-variant 範式を complete §7 と reference doc 本体に追記)
- [ ] **次々々 session**: Phase 2b complete handoff の §4 Phase 2c 表 (binding 21-24、cofF + blurLightF + waterF + pbrterrainV) を起点に Phase 2c prep 起草

---

## §9 reference link

- 前 handoff (η-28 Phase 2a complete): `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2a-complete.md` (commit `527d572388`)
- A-E 整備 commit: `057cd8b299`
- §8 checklist 進捗反映 commit: `f8144cce28`
- location/UBO map: `docs/specs/ayastorm-r41-gl-removal/reference-shader-location-map.md` (η-28 整備済、Phase 2b feat commit で §6-A 4 行追加予定)
- Phase 2a 末 cold launch log: `~/.ayastorm_x64/logs/AYAstorm.log` (3788 行、ERROR 22、本 prep §3 / §6 起点)
- Phase 2a 末 transformed dump 場所: `~/.ayastorm_x64/cache/shader_cache/transformed/` (η-28-B 範式で本 prep 起草に活用済)

---

**本 prep doc は η-28 Phase 2b 着手前の source of truth**。AYA 承認 → feat commit → cold launch verify → complete handoff 起草 の 1-session 構成。
