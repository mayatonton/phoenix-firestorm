# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-23 完遂 handoff

**status**: B?-η-23 完遂 (Phase 1 multiPointLightF.glsl 残 6 bare uniform 構造解消 = Deferred MultiLight Shader 0-15 全 16 件 parse error **完全消滅**、glslang link failed for program **0 → 0** = **link 段階 ZERO 連続維持** (η-21 milestone 2 sub-bundle 連続)、ERROR 計 154 → 139 → **104** (η-22 比 -35 = -16 non-opaque + -16 compilation errors summary cascade + -3 連動)、clean shutdown 維持) → 次 sub-bundle B?-η-24 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: `9cc143d2a6` (1 shader file 編集、+74/-0 行、C++ 変更 0、user_settings.xml 変更 0)
**handoff doc commit**: 本 doc (η-22-complete 範式継承、別 commit)
**勤続範式継承**: B?-η-22-complete `48ceb1c953` (patch `5cc4b03bab`) / B?-η-21-complete `989b529936` (patch `e78ca6006c`) / B?-η-20-complete `d7c722f4e7` (patch `557cd1db00`) / B?-η-19-complete `4911401568` (patch `9349ace5f0`) / B?-η-18-complete `1371da9660` (patch `4b42779cc7`) / B?-η-17-complete `cc2e878c7e` (patch `e2d4d3bbca`) / B?-η-16-complete `3653efed00` (patch `72fd4f3c3c`) / B?-η-15-complete `cb28cf1daa` (patch `6a11eabc73`) / B?-η-14-complete `78df235243` (patch `c971838656`) / B?-η-13-reverted (no commit) / B?-η-12-complete `7c1762d214` / B?-η-11-complete `74705c35bb` / B?-η-10-complete `9246142639` / B?-η-9-complete `6fee4a818b` / B?-η-8-complete `6994eba271` / B?-η-7-complete `b1e8689634` / B?-η-6-complete `fe757ea624` / B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-23 scope = **Phase 1 (A: multiPointLightF.glsl 残 6 bare uniform 構造解消)** 単独で **Deferred MultiLight Shader 0-15 全 16 件 parse error 完全消滅**。AYA judgment 「案 X (A 単独、η-24 で D + 残)」literal 受領後 fresh trace で agent 誤報告判明 → literal 実態 base に scope 再構成 → AYA「X」literal 受領 → 1 cycle 2 巡 (第1巡 cascade 第21層露出 → 第2巡 全 cascade 同 patch 解消) で完走。

**Phase 1 (multiPointLightF.glsl 6 bare uniform 解消、複合範式適用)**:
- **dead in this file (2 件、η-22 §3.1 GL-only wrap 範式)**:
  - `sun_wash` (L59) / `light_count` (L60) → main() 未参照、GL path 限定 wrap で Vulkan path 代替宣言不要
- **alive (4 件、η-3 §3.2 PerDrawUBO 範式 + η-4 §3.2 FrameAtmosphere_Lighting per-group rename 範式)**:
  - `light[LIGHT_COUNT]` / `light_col[LIGHT_COUNT]` (L79-80) → 新規 UBO `PerDrawUBO_MultiLight` (set=2, binding=1) array member 化
  - `far_z` (L102) → PerDrawUBO_MultiLight scalar member 化 (float)
  - `global_light_strength` (L109) → PerDrawUBO_MultiLight scalar member 化
  - `classic_mode` (L106) → Vulkan path で FrameAtmosphere_Lighting UBO (set=0, binding=2) declare 追加 (pointLightF.glsl L98-L120 同範式統一、`FRAME_ATMOSPHERE_LIGHTING_DEFINED` guard で重複防止)、block 内 member の global scope access で main() 参照無修正

**PerDrawUBO_MultiLight std140 構造**:
```glsl
layout(set=2, binding=1, std140) uniform PerDrawUBO_MultiLight {
    vec4  light[LIGHT_COUNT];      // 16 bytes × LIGHT_COUNT
    vec4  light_col[LIGHT_COUNT];  // 16 bytes × LIGHT_COUNT
    float far_z;
    float global_light_strength;
    float _pad_ml0;
    float _pad_ml1;                // 末尾 float×2 + pad×2 で 16-byte chunk 整合
};
```

**計**: **shader 1 file 編集 (+74/-0 = +74)、C++ 変更 0、user_settings.xml 変更 0**

**主指標達成** (vs η-22 末 baseline log):

- `non-opaque uniforms outside a block` (Multi-Light 16 件) 16 → **0** ✓ **完全解消 (Phase 1 主目標達成)**
- `compilation errors. No code generated` 61 → **45** **-16 = Multi-Light 16 program 全 parse 通過**
- `glslang link failed for program` 0 → **0** ✓ **link 段階 ZERO 2 sub-bundle 連続維持 (η-21 milestone 継承)**
- ERROR 計 139 → **104** **-35 (= -16 構造解消 + -16 compilation errors summary cascade + -3 連動)**
- **Deferred MultiLight Shader 0-15 全 16 件 SPIR-V module loaded** (944 / 24260 bytes ×16) ✓
- `Goodbye!` 1 / `Vulkan device destroyed` 1 / FATAL/SIGSEGV/Aborted 0/0/0 = ✓ clean shutdown 維持

**cascade shift forward 第21層は同 file 内で完結** (η-23 で 6 件解消した結果、第22層追加露出なし、η-24 移管対象 metric 全種 ±0):

| # | metric | η-22末 | η-23後 | Δ | 解析 |
|---|---|---|---|---|---|
| 1 | `non-opaque uniforms outside a block` (parse) | 51 | **35** | **-16** | ✓ Multi-Light 16 件全 cascade 解消 |
| 2 | `'nameless block ... global scope'` (parse) | 5 | **5** | ±0 | (η-24 移管、FrameAtmosphere_Skybox 5 program) |
| 3 | `'shadow_clip' : redefinition` (parse) | 1 | **1** | ±0 | (η-24 移管、Haze Shader L2262、handoff §5.5 η-22 推定 = Godrays Shader vert を literal 上書き) |
| 4 | `overlapping use of location` (parse) | 5 | **5** | ±0 | (η-24 移管、全 location 20 で V↔F slot alignment 単一 root cause) |
| 5 | `nameless block ... global scope` (parse、再列挙) | (同 #2) | (同 #2) | ±0 | (η-24 移管) |
| 6 | `missing #endif` (parse、phantom) | 13 | **13** | ±0 | phantom 維持 |
| 7 | `compilation errors. No code generated` (summary) | 61 | **45** | **-16** | ✓ Multi-Light 16 program 全救済 |
| 8 | `glslang link failed for program` | 0 | **0** | ±0 | ✓ link 段階 ZERO 2 sub-bundle 連続維持 |
| 9 | Anonymous member (link) | 0 | **0** | ±0 | ✓ η-20 達成維持 |
| 10 | shadow_bias / M_PI / weight4 redef (parse) | 0 | **0** | ±0 | ✓ η-20 達成維持 |
| 11 | Input `vary_position` (link) | 0 | **0** | ±0 | ✓ η-21 達成維持 |
| 12 | MaterialUBO metallicFactor / fragment block (link) | 0 | **0** | ±0 | ✓ η-21 達成維持 |
| 13 | sampler binding (parse) | 0 | **0** | ±0 | ✓ η-21 達成維持 |
| 14 | env_mat cannot redeclare (parse) | 0 | **0** | ±0 | ✓ η-22 達成維持 |
| 15 | SPIR-V requires location (parse) | 0 | **0** | ±0 | ✓ η-22 達成維持 |

**主指標達成総括**: **Multi-Light Shader 0-15 全 16 件構造的完全解消** (件数最大 cascade 一括処理)。**link 段階 ZERO を η-21 milestone から 2 sub-bundle 連続維持**。cascade shift forward 第22層追加露出 **なし** (同 file 内完結) = η-24 主軸は別 file 群 (Tonemap/FXAA 等の non-opaque 35 件 + FrameAtmosphere_Skybox 5 件 + 他 3 種 ±0)。

**他既達主指標完全維持** (η-23 末):
- η-22 達成全項目 (env_mat redeclare 0 / SPIR-V requires location 0)
- η-21 達成全項目 (link failed 0 / sampler binding 0 / metallicFactor 0 / vary_position 0)
- η-20 達成全項目 (Anonymous member 0 / shadow_bias / M_PI / weight4 redef 0)
- η-19 達成全項目 (normalMap / depthMap / vary_fragcoord redef 0)
- η-18 達成全項目 (Layout location qualifier 0)
- η-1〜η-17 達成全項目 (Cannot reuse block name 0 / 'binding' 0 / GBufferInfo redef 0 / 'size' undeclared 0 / undeclared identifier 0)

**Phase 構成の特徴**: η-23 は **agent 誤報告発覚 → literal 実態 base scope 再構成 → AYA judgment 受領後 1 cycle 2 巡 (第1巡 cascade 第21層露出後 同 file 内残 3 bare uniform 同 patch で全解消)** で完走。**feedback_admit_unknown 範式遵守** (推論 base agent 報告を literal log で検証、誤りを正直に AYA に報告し再 judgment 仰ぐ)、**feedback_no_scope_shrink 範式遵守** (scope を「Multi-Light 16 件解消」に固定し件数目標達成まで bare uniform 完全棚卸し)。

---

## §2 完遂結果 metric (vs B?-η-22 末 baseline log)

| metric | η-22末 | η-23後 (verify) | Δ vs η-22 | 判定 |
|---|---|---|---|---|
| **Multi-Light non-opaque (parse)** | **16** | **0** | **-16** | ✓ **Phase 1 主目標完全達成** |
| **Deferred MultiLight Shader 0-15 SPIR-V module loaded** | 0/16 | **16/16** | **+16** | ✓ **全 16 件 SPIR-V emit 成功** |
| glslang link failed for program | 0 | 0 | ±0 | ✓ **link 段階 ZERO 2 sub-bundle 連続維持** |
| ERROR (total) | 139 | 104 | **-35** | 16 件構造解消 + 16 件 compilation errors summary cascade + 3 件連動 |
| non-opaque uniforms (parse、total) | 51 | 35 | **-16** | Multi-Light 16 件全解消、残 35 件は η-24 移管 (Tonemap/FXAA 等) |
| compilation errors. No code generated (summary) | 61 | 45 | **-16** | ✓ Multi-Light 16 program 全救済 |
| nameless block (parse) | 5 | 5 | ±0 | (η-24 移管) |
| shadow_clip redef (parse) | 1 | 1 | ±0 | (η-24 移管) |
| overlapping loc (parse) | 5 | 5 | ±0 | (η-24 移管) |
| missing #endif (parse、phantom) | 13 | 13 | ±0 | phantom 維持 (independent fix 不可) |
| env_mat redeclare (parse) | 0 | 0 | ±0 | ✓ η-22 達成維持 |
| SPIR-V requires location (parse) | 0 | 0 | ±0 | ✓ η-22 達成維持 |
| Anonymous member (link) | 0 | 0 | ±0 | ✓ η-20 達成維持 |
| shadow_bias / M_PI / weight4 redef | 0 | 0 | ±0 | ✓ η-20 達成維持 |
| Input 'vary_position' (link) | 0 | 0 | ±0 | ✓ η-21 達成維持 |
| MaterialUBO metallicFactor / fragment block (link) | 0 | 0 | ±0 | ✓ η-21 達成維持 |
| sampler binding (parse) | 0 | 0 | ±0 | ✓ η-21 達成維持 |
| Cannot reuse block name | 0 | 0 | ±0 | ✓ η-1 達成維持 |
| 'binding' (parse、η-8 達成系) | 0 | 0 | ±0 | ✓ η-8 達成維持 |
| GBufferInfo redefinition struct | 0 | 0 | ±0 | ✓ ζ 達成維持 |
| 'size' undeclared | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| undeclared identifier | 0 | 0 | ±0 | ✓ η-14 達成維持 |
| FATAL/SIGSEGV/Aborted | 0/0/0 | 0/0/0 | ±0 | ✓ |
| Goodbye | 1 | 1 | ±0 | ✓ clean shutdown |
| Vulkan device destroyed | 1 | 1 | ±0 | ✓ clean shutdown |

**Multi-Light Shader 0-15 全 16 件構造的完全解消 + link 段階 ZERO 2 sub-bundle 連続維持 + 第22層 cascade 露出ゼロ + 18 種既達主指標完全維持 + clean shutdown** = **η-23 Phase 1 主目標達成**。

---

## §3 設計範式

### §3.1 適用範式: η-22 §3.1 GL-only wrap 範式継承 (dead bare uniform 2 件)

**範式根拠**: Vulkan strict mode (GLSL_KHR_vulkan_glsl) では bare `uniform <type> <ident>;` (= legacy non-block uniform) が `ERROR: 'non-opaque uniforms outside a block'` parse error。η-22 §3.1 で確立した `#ifndef LL_VULKAN_GLSL` wrap で GL path 限定化 = Vulkan path で declare 不在 = error 解消。**main() 内未参照 (dead in this file) の場合**、Vulkan path での代替 declaration 不要。

**η-23 適用 (multiPointLightF.glsl L54-L61)**:
```glsl
#ifndef LL_VULKAN_GLSL
uniform float sun_wash;       // main() 未参照 (LIGHT_COUNT loop 内 light_col/light のみ参照)
uniform int   light_count;    // main() 未参照 (LIGHT_COUNT #define で代替)
#endif
```

**有効性根拠**:
1. **dead 検証**: main() 全文 (L150-238) で `sun_wash` / `light_count` を grep = 0 件 = 安全に GL-only 化可能
2. **GL path 完全非変更**: C++ glUniform1f("sun_wash") / glUniform1i("light_count") binding は GL path で動作継続
3. **Vulkan path PSO への影響なし**: dead → SPIR-V emit に含まれない

### §3.2 適用範式: η-3 §3.2 PerDrawUBO 範式継承 + 拡張 (alive scalar/array 4 件、新規 UBO PerDrawUBO_MultiLight)

**範式根拠**: η-3 で確立した PerDrawUBO_LightParams (pointLightF.glsl set=2, binding=0) の per-program 派生として、multiPointLightF.glsl 専用 UBO を **set=2, binding=1** で新規 declare。**LIGHT_COUNT は viewer #define で固定数** = std140 array size 固定可能。**scalar member (far_z / global_light_strength)** は std140 alignment 揃えで block 末尾の 16-byte chunk に float×2 + pad×2 で配置。

**η-23 適用 (multiPointLightF.glsl L63-L88)**:
```glsl
#ifdef LL_VULKAN_GLSL
#ifndef PER_DRAW_UBO_MULTILIGHT_DEFINED
#define PER_DRAW_UBO_MULTILIGHT_DEFINED 1
layout(set=2, binding=1, std140) uniform PerDrawUBO_MultiLight {
    vec4  light[LIGHT_COUNT];
    vec4  light_col[LIGHT_COUNT];
    float far_z;
    float global_light_strength;
    float _pad_ml0;
    float _pad_ml1;
};
#endif
#else
uniform vec4  light[LIGHT_COUNT];
uniform vec4  light_col[LIGHT_COUNT];
#endif
```

(far_z / global_light_strength は別 #ifndef LL_VULKAN_GLSL ブロックで GL path 用 bare 維持、L113 / L158)

**有効性根拠**:
1. **block 内 member の global scope access**: GLSL nameless block の特例で main() 内 `light[i]` / `light_col[i]` / `far_z` / `global_light_strength` 参照無修正で機能
2. **std140 alignment 整合**: vec4 配列 (16-byte aligned) + scalar 末尾 16-byte chunk = pad×2 で C++ side 直書き互換
3. **per-program binding 分離**: pointLightF.glsl set=2 binding=0 (PerDrawUBO_LightParams) と multiPointLightF.glsl set=2 binding=1 (PerDrawUBO_MultiLight) で衝突なし
4. **GL path 完全非変更**: `#else` で bare uniform 残置 = C++ glUniform4fv("light"/"light_col") + glUniform1f("far_z") + glUniform1f("global_light_strength") binding 変更不要
5. **AMD shader_cache ガード (L234-237) 静的アクセス互換**: `light[0]` / `light_col[0]` / `light[LIGHT_COUNT - 1]` / `light_col[LIGHT_COUNT - 1]` 全部 block 内で同 syntax 動作

### §3.3 適用範式: η-4 §3.2 FrameAtmosphere_Lighting per-group rename 範式継承 (alive classic_mode 1 件)

**範式根拠**: classic_mode は FrameAtmosphere_Lighting UBO (set=0, binding=2) member として既存 declare 済 (pointLightF.glsl L98-L120 / multiPointLightF.glsl L83-L98 旧 = FrameViewProj のみで FrameAtmosphere_Lighting 未含)。η-23 で multiPointLightF.glsl に **FrameAtmosphere_Lighting UBO declare 追加** + `FRAME_ATMOSPHERE_LIGHTING_DEFINED` guard で重複防止。block 内 member `classic_mode` を **global scope access** することで main() 内 `if (classic_mode > 0)` (L226) 無修正で機能。

**η-23 適用 (multiPointLightF.glsl L118-L151)**:
```glsl
#ifdef LL_VULKAN_GLSL
#ifndef FRAME_ATMOSPHERE_LIGHTING_DEFINED
#define FRAME_ATMOSPHERE_LIGHTING_DEFINED 1
layout(set=0, binding=2, std140) uniform FrameAtmosphere_Lighting {
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
    float _pad_atm0;
    float _pad_atm1;
};
#endif
#else
uniform int classic_mode;
#endif
```

**有効性根拠**:
1. **既存 viewer 全体 layout 整合**: pointLightF.glsl L98-L120 と同 layout = std140 binding 互換性
2. **guard 重複防止**: `FRAME_ATMOSPHERE_LIGHTING_DEFINED` で multiPointLightF.glsl 内 redeclare 防止 (η-1 §3.1 guard 範式継承)
3. **member 名 `classic_mode` global scope access**: GLSL nameless block の特例で `if (classic_mode > 0)` 直接機能

### §3.4 cascade shift forward 第21層は同 file 内完結 (η-23 で観測)

**範式根拠**: η-22 で env_mat 16 件解消後の第20層 cascade exposure (non-opaque +3 / shadow_clip redef +1) は **別 file 群** (Deferred Blur Light/Godrays/FS Object ID + Godrays Shader vert) だったが、η-23 では multiPointLightF.glsl の `sun_wash` (L59) 解消 → 第1巡 verify で同 file 内 L102 `far_z` 露出 = **同 file 内 cascade**。第2巡で残 3 bare uniform を 1 patch 全解消 → **第22層追加 cascade 露出ゼロ**。

**観測** (η-23 第1巡 → 第2巡):
- 第1巡: sun_wash/light_count/light/light_col 4 件 wrap → ERROR 139 → 136 (-3、Multi-Light 全 16 件 line 2299→2347 shift で再 fail)
- 第2巡: far_z + classic_mode + global_light_strength 追加 wrap → ERROR 136 → **104 (-35 累計)**、Multi-Light 全 16 件 **SPIR-V emit 成功**

**範式遵守 (η-24)**:
- 第22層追加 cascade 露出ゼロ = η-23 主軸完全終結
- η-24 主軸は別 file 群 (non-opaque 35 件 cascade exposure = Tonemap/FXAA 系 file 別棚卸し)
- nameless block 5 件 (FrameAtmosphere_Skybox UBO 構造解消) は別範式 (η-1 §3.1 instance name 付与 or UBO declare 整理) 必要、η-24 副 scope 候補

### §3.5 prefer-cold-launch-verify 範式 (η-23 = 1 cycle 2 巡完走、agent 誤報告 → literal 実態 base 再構成)

η-23 では:
1. AYA cold launch で η-22 末 baseline 取得済 (`/home/ishikawa/.ayastorm_x64/logs/AYAstorm.log` 起動 2026-06-02T10:44Z 直前 baseline)
2. **handoff §8.1 14 ステップ trace literal 実施 (Explore agent 委譲)**: 結果に「Deferred Impostor Shader 19 件」「AYAstorm Alpha Plate Composite 7 件」報告
3. **Claude 自身で literal 検証**: impostorV/F.glsl source = 既 Vulkan 対応済 (bare uniform wrap 完了) → agent 報告と齟齬発覚
4. **log 直接 grep + transformed dump literal 確認**: Multi-Light Shader 0-15 = 16 件 (multiPointLightF.glsl L2299 = `sun_wash`)、AYAstorm Alpha Plate Composite = 0 件 (parse 完全成功)、FrameAtmosphere_Skybox nameless block = Gamma Correction 系 5 program (Alpha Plate / Luminance ではない) を確定
5. **AYA に literal 実態 base scope 再構成案 3 つ提示** (案 X = Phase 1 単独 / 案 Y = Phase 1+2 同梱 / 案 Z = A category 全 51 件同梱) → AYA judgment 「X」literal 受領
6. Phase 1 第1巡 patch (sun_wash/light_count/light/light_col 4 件) → cold launch → cascade 第21層 (far_z) 露出
7. Phase 1 第2巡 patch (far_z/classic_mode/global_light_strength 3 件追加) → cold launch → **Multi-Light 16 件全 SPIR-V emit 成功**
8. AYA verify (第 2 巡 cold launch) → 主指標達成確認、ERROR 計 139 → 104 (-35) ✓ clean shutdown

**feedback_admit_unknown 範式遵守**: agent 推論 base 報告を盲信せず literal 検証で誤り発見、正直に AYA 報告して再 judgment 仰ぐ。**feedback_no_scope_shrink 範式遵守**: 「Multi-Light 16 件」目標を件数で固定、cascade で同 file 内残 bare uniform が露出した時に scope 内継続として完全棚卸し対応。

---

## §4 patch 内容 (1 shader file)

### §4.1 Phase 1: multiPointLightF.glsl 残 6 bare uniform 構造解消

#### §4.1.1 L54-L61: sun_wash + light_count GL-only wrap (η-22 §3.1 範式継承)

```glsl
#ifndef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-23 Phase 1: legacy bare
// `uniform float sun_wash` / `uniform int light_count` を GL path 限定 wrap
// (η-22 §3.1 GL-only wrap 範式継承)。multiPointLightF main() 内で sun_wash /
// light_count 共に未参照 = dead in this file、Vulkan path での代替宣言不要。
uniform float sun_wash;
uniform int   light_count;
#endif
```

#### §4.1.2 L63-L88: light/light_col/far_z/global_light_strength → PerDrawUBO_MultiLight 集約 (η-3 §3.2 範式継承 + 拡張)

```glsl
#ifdef LL_VULKAN_GLSL
#ifndef PER_DRAW_UBO_MULTILIGHT_DEFINED
#define PER_DRAW_UBO_MULTILIGHT_DEFINED 1
layout(set=2, binding=1, std140) uniform PerDrawUBO_MultiLight {
    vec4  light[LIGHT_COUNT];
    vec4  light_col[LIGHT_COUNT];
    float far_z;
    float global_light_strength;
    float _pad_ml0;
    float _pad_ml1;
};
#endif
#else
uniform vec4  light[LIGHT_COUNT];     // .w = size; see C++ fullscreen_lights.push_back()
uniform vec4  light_col[LIGHT_COUNT]; // .a = falloff
#endif
```

#### §4.1.3 L109-L114: far_z GL-only wrap

```glsl
#ifndef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-23 Phase 1: legacy bare
// `uniform float far_z` を GL path 限定 wrap。Vulkan path では
// PerDrawUBO_MultiLight (set=2, binding=1) の member で代替。
uniform float far_z;
#endif
```

#### §4.1.4 L118-L151: FrameAtmosphere_Lighting UBO declare 追加 + classic_mode GL-only wrap (η-4 §3.2 範式継承)

```glsl
#ifdef LL_VULKAN_GLSL
#ifndef FRAME_ATMOSPHERE_LIGHTING_DEFINED
#define FRAME_ATMOSPHERE_LIGHTING_DEFINED 1
layout(set=0, binding=2, std140) uniform FrameAtmosphere_Lighting {
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
    float _pad_atm0;
    float _pad_atm1;
};
#endif
#else
uniform int classic_mode;
#endif
```

#### §4.1.5 L154-L159: global_light_strength GL-only wrap

```glsl
#ifndef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-23 Phase 1: legacy bare
// `uniform float global_light_strength` を GL path 限定 wrap。Vulkan path では
// PerDrawUBO_MultiLight (set=2, binding=1) の member で代替。
uniform float global_light_strength;
#endif
```

### §4.2 cache invalidate

shader file 変更のみ、C++ transformer は v5 据置 (η-18 末)。shader_cache clear で対応:
```bash
rm -rf ~/.ayastorm_x64/cache/shader_cache/*
```

---

## §5 cascade shift forward 第22層追加露出 ZERO + η-24 移管計画

### §5.1 第22層追加 cascade 露出 = ゼロ (η-23 同 file 内完結)

η-23 で multiPointLightF.glsl の 6 bare uniform 構造解消 → 第22層追加 cascade 露出 **なし**:
- non-opaque 35 件 (η-24 移管): Multi-Light 16 件解消後の **残主軸**、別 file 群 (Tonemap/FXAA 等)
- nameless block 5 件 / shadow_clip redef 1 件 / overlapping loc 5 件 / phantom #endif 13 件 = **全て η-22 から不変、η-24 移管対象**

### §5.2 η-24 主 scope 候補

| カテゴリ | 件数 | 推奨 sub-bundle | 根拠 |
|---|---|---|---|
| **non-opaque uniforms outside a block** | **35** | **η-24 主 scope 候補 A** | 件数最大、Multi-Light 16 件解消後の残主軸。η-23 §3.1 GL-only wrap or η-23 §3.2 PerDrawUBO 集約範式継続適用。file 別棚卸し: Tonemap 系 3 / FXAA 4 / 他 28 (handoff §8.1 η-23 trace で確認) |
| `nameless block ... global scope` (parse) | 5 | η-24 副 scope 候補 D | FrameAtmosphere_Skybox UBO 5 file declare (CASF/postDeferredTonemap/postDeferredGammaCorrect/skyV/cloudsV) で global scope member 衝突。η-1 §3.1 instance name 付与 範式類で構造解決候補 |
| `'shadow_clip' : redefinition` (parse) | 1 | η-24 副 scope 候補 B | **Haze Shader L2262** (handoff §5.5 η-22 推定 = Godrays Shader vert を literal 実態で上書き確認、η-23 末 baseline log で実 file 確認済)、η-20 §3.1 範式類 |
| `overlapping use of location` (parse) | 5 | η-24 副 scope 候補 C | **全 location 20** で V↔F slot alignment 単一 root cause、η-18 §3.1 範式類で 1 fix 5 件解消候補 |
| `missing #endif` (parse、phantom) | 13 | η-24 副次効果 | 独立 fix 不可、先行 ERROR fix で同時消滅 (handoff §2.1 確定済) |
| `compilation errors. No code generated` (summary) | 45 | (parse 失敗 program 数 summary 行、独立 fix 不可) | 先行 ERROR fix で連動減少 |

**合計 net 不重複 ERROR**: 約 59 件 (35 + 5 + 1 + 5 + 13、summary 45 除外)。

### §5.3 link 段階 ZERO 維持 milestone 観測

η-21 で達成した link 段階 ZERO を η-22 + η-23 で **2 sub-bundle 連続維持**。第22層追加 cascade 露出ゼロ = **parse-stage 単一段階に絞られた状態 + main file 完結確認** = η-24 で別 file 群への parse-stage 集中攻略が可能。η-23 では link 段階退行 0 件達成。

### §5.4 PerDrawUBO_MultiLight の C++ binding 対応 (η-24+ 移管検討)

η-23 では shader 側のみ UBO 化、**C++ runtime での UBO buffer 書込み (vkCmdUpdateBuffer / vkCmdBindDescriptorSets) は kill-switch (RenderUseVulkanGL=0) で GL path 動作維持中**につき未着手。η-24+ で kill-switch 解除 phase に入ったら以下が必要:
- C++ side: `PerDrawUBO_MultiLight` 用 VkBuffer + VkDescriptorSetLayout (set=2 binding=1) 確保
- per-draw 書込み: `glUniform4fv("light", count, ptr)` 等を `memcpy + vkCmdUpdateBuffer` に置換
- バインディング: `vkCmdBindDescriptorSets(set=2)` で descriptor set 結合

### §5.5 FrameAtmosphere_Lighting UBO の per-shader declare 統一 (現状)

η-23 で multiPointLightF.glsl に FrameAtmosphere_Lighting UBO declare 追加 = **既存 pointLightF.glsl と同 layout** で viewer 全体一貫性維持。η-24+ で他 shader (Tonemap 系等) で classic_mode 等参照時は同範式継承で declare 追加候補 (`FRAME_ATMOSPHERE_LIGHTING_DEFINED` guard で重複防止)。

---

## §6 risks (η-22 §6 継承 + 新規)

| # | risk | 対応状態 | 引継 sub-bundle |
|---|---|---|---|
| 1 | C++ runtime transformer が GL path で influence する | ✓ kill-switch で完全 bypass、η-16 から継承 | 引継不要 |
| 2 | preprocessor 別分岐の bare/manual-wrap 共存 | ✓ η-18 で再利用優先範式に修正済 | 引継不要 |
| 3 | regex group index / kQuals non-capturing 不整合 | ✓ η-18 で修正済 | 引継不要 |
| 4 | shader file level guard が異 type 同名 varying program で衝突 | ✓ η-19 で確認済 | 引継不要 |
| 5 | PerDrawUBO member rename + #define alias の関数 param/local var 共存問題 | ✓ η-20 §4.1.2 で verify 済 | 引継不要 |
| 6 | #undef scope-limit が file 末尾で対応 = file 内全 use site が rewrite | ✓ η-20 で意図通り | 引継不要 |
| 7 | cascade shift forward 第20層 (env_mat 16 + SPIR-V 3 解消の結果) | ✓ η-22 で 19 件構造解消 | 引継不要 |
| 8 | sampler binding 番号 conflict (set=0, binding=5 統一原則) | ✓ η-21 で実装、verify 済 | η-24+ で新規 sampler wrap 時は既存 binding 統一原則継承 |
| 9 | V/F MaterialUBO member alignment 跨 file 異 layout 可能性 | ✓ η-21 で 4 declarations 全件同 layout verify 済 | 引継不要 |
| 10 | dead vary_position 削除で同 ident 異 path 残置 | ✓ η-21 で literal trace 確認 | 引継不要 |
| 11 | legacy bare uniform GL-only wrap で main() 参照ありの場合は UBO 置換必要 | ✓ η-22 / η-23 で multiPointLightF dead / alive 切分 verify 済 | η-24+ で同範式適用時は main() 内参照 trace literal 確認必須 |
| 12 | bare vertex attribute layout(location=X) で C++ binding 整合 | ✓ η-22 で position=0 = TYPE_VERTEX=0 verify 済 | η-24+ で他 attribute (normal/texcoord0 等) 対応時は同範式継承 + index 整合 |
| 13 | upstream OpenGL Firestorm merge 時 shader file 互換 | 互換性高 (η-23 範式は標準 GLSL) | sub-step 4.5 |
| 14 | cinematic_bd overlay mirror | η-23 では touch なし、multiPointLightF.glsl 不在を Glob で確認済 | 引継不要 |
| 15 | non-opaque uniforms 51 → 35 件 cascade = η-24 主 scope 候補、multi-file 取りこぼし risk | η-23 §3.1 / §3.2 範式継続適用予定 | η-24 着手時に file 別棚卸し優先 |
| 16 | nameless block 5 件 = FrameAtmosphere_Skybox UBO 5 file declare 衝突 | η-1 §3.1 instance name 付与 範式類で構造解決候補 | η-24 副 scope D |
| 17 | shadow_clip redef 1 件 = Haze Shader L2262 (literal 確認済、handoff §5.5 η-22 推定上書き) | η-20 §3.1 範式類で構造解決候補 | η-24 副 scope B |
| 18 | overlapping loc 5 件 = 全 location 20 で V↔F slot alignment 単一 root cause | η-18 §3.1 範式類 (1 fix 5 件解消) | η-24 副 scope C |
| 19 | Phase 1 第1巡 cascade 第21層露出 (sun_wash → far_z) は同 file 内完結 = 第2巡で全解消 | feedback_admit_unknown / feedback_one_step_at_a_time 範式遵守 | 引継不要 |
| 20 | agent 誤報告 base scope 構築の risk | literal 検証 + AYA judgment 受領後実装で対応、feedback_admit_unknown / feedback_confirm_referent_before_acting 範式遵守 | η-24+ で同類 trace 継続、agent 報告は必ず literal 検証 |
| 21 | C++ PerDrawUBO_MultiLight binding 未対応 (set=2, binding=1 / vkBuffer + DescriptorSet) | kill-switch GL path 動作維持中につき未着手、η-23 では shader-only patch | sub-step 4.5+ (kill-switch 解除 phase) |

---

## §7 observability

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 件数 (η-7 305 baseline、η-22 末 421 件 transformed dump 維持) | η-23 末 verify 後 421 件超 transformed dump 維持、cold launch 後 cache 合計 計測継続 | B?-η-24 で baseline 更新 |
| 2 | C++ runtime location emit 範式 (η-16 §3.1) Phase 2 (V↔F pair) 維持 | ✓ η-23 で touch なし、v5 据置 | B?-η-24 で SPIR-V loc 追加対応時 Phase 3 拡張候補 |
| 3 | shader file level include guard 範式 (η-19 §3.1) 適用範囲 | η-23 では multiPointLightF.glsl にて FRAME_ATMOSPHERE_LIGHTING_DEFINED / PER_DRAW_UBO_MULTILIGHT_DEFINED guard 拡張適用 | B?-η-24 で他 file 続行 |
| 4 | PerDrawUBO member rename + #define alias + #undef 範式 (η-20 §3.1) | η-23 では touch なし | B?-η-24 で shadow_clip redef 同範式適用候補 |
| 5 | bare opaque uniform Vulkan binding 付与範式 (η-21 §3.1) | η-23 では touch なし、η-21 で 1 件適用済 | B?-η-24 で他 bare sampler / texture / image 残時に同範式継承 |
| 6 | V/F UBO member alignment 範式 (η-21 §3.2) | η-23 では touch なし | B?-η-24 で他 UBO V/F alignment 必要時に同範式継承 |
| 7 | dead F-side declaration 削除範式 (η-21 §3.3) | η-23 では touch なし | B?-η-24 で他 dead in 検出時に同範式継承 (literal trace 確認必須) |
| 8 | legacy bare uniform GL-only wrap 範式 (η-22 §3.1) | η-23 で multiPointLightF.glsl sun_wash / light_count / far_z / global_light_strength = 4 件適用 | B?-η-24 で他 bare uniform × UBO member redeclare 衝突 / dead 検出時に同範式継承 |
| 9 | bare vertex attribute Vulkan location 付与範式 (η-22 §3.2) | η-23 では touch なし | B?-η-24 で他 vert shader bare in 検出時に同範式継承 |
| 10 | PerDrawUBO 範式拡張 (η-3 §3.2 → η-23 §3.2) per-program 派生 | η-23 で PerDrawUBO_MultiLight 新規導入 (set=2, binding=1、vec4 array + scalar 混在 std140 alignment 整合) | B?-η-24 で他 per-program UBO 必要時に同範式継承 |
| 11 | FrameAtmosphere_Lighting per-shader declare 統一範式 (η-4 §3.2 → η-23 §3.3) | η-23 で multiPointLightF.glsl に declare 追加、pointLightF.glsl と layout 一致 | B?-η-24 で他 shader classic_mode / minimum_alpha 等参照時に同範式継承 |
| 12 | dump 機構 (η-16 §3.1 内包) η-23 で 421 件超出力 | 維持 | B?-η-24 で必要に応じて再投入 |
| 13 | falsification iterate 範式 (η-18 内 v2→v5 4 段) | η-23 は AYA judgment 受領後 1 cycle 2 巡完走、falsification iterate 不要 | B?-η-24 で C++ transformer Phase 3 拡張時は falsification iterate 想定 |
| 14 | 既達主指標完全維持 (η-23 で 18 種維持 + 16 件 Multi-Light 構造解消、退行 0 件) | ✓ η-23 で退行 0 件達成 | B?-η-24 で第21層 cascade 残系統 scope |
| 15 | cinematic_bd directory 全 .glsl audit (η-17 §10.4 継承) | η-23 で touch なし、η-22 末 audit 維持 | B?-η-24+ 着手前 or 完遂後の別 phase として継続推奨 |
| 16 | feedback_self_verify_before_handoff 適用 | η-23 で literal log + dump + source code 三方検証 + AYA cold launch 2 巡で 16 件全解消確認 | B?-η-24 でも AYA cold launch 依頼前に必ず実施 |
| 17 | feedback_admit_unknown 適用 (agent 誤報告発覚) | η-23 で agent 推論 base 報告を literal log で検証 → 誤り発見 → 正直に AYA 報告 → 再 judgment 仰ぐ | B?-η-24 でも継承、agent 報告 = 仮説扱いで literal 検証必須 |
| 18 | feedback_no_scope_shrink 適用 | AYA「X」literal 受領 → Multi-Light 16 件目標固定、cascade 第21層露出時に同 file 内残 bare uniform 完全棚卸し対応 | B?-η-24 でも継承 |
| 19 | feedback_remove_verification_logs 適用 | η-23 で追加 LL_INFOS hook 無し、不要 | B?-η-24 で追加 LL_INFOS hook あれば commit 前に DEBUG/削除 判断 |
| 20 | feedback_one_step_at_a_time 適用 | η-23 で 1 step 1 verify 範式遵守 (trace → agent 誤報告検証 → 推奨提示 → AYA judgment → patch 第1巡 → cold launch → cascade 第21層分析 → patch 第2巡 → cold launch → 主指標確認 → commit) | B?-η-24 でも継承 |
| 21 | link 段階 ZERO 維持 milestone (η-21 達成、η-22 + η-23 で 2 sub-bundle 連続維持) | ✓ η-23 で達成 | B?-η-24 では parse 段階集中攻略、link 段階 ZERO 維持を引き続き観測 |

---

## §8 引継 scope 推奨 (B?-η-24)

### §8.1 着手前 trace (14 ステップ ベースで η-24 適応)

1. AYA 「コマンド + ビルド全権」+「verify は 1 ステップずつ」運用継承確認
2. η-23 末 baseline log (`/home/ishikawa/.ayastorm_x64/logs/AYAstorm.log` 起動 2026-06-02T11:30Z) を canonical baseline として extract
3. ERROR 104 件を **カテゴリ別 + program 別** に系統整理
4. **主 scope 候補 A**: parse failed `non-opaque uniforms outside a block` 35 件継続解消 (件数最大、η-23 §3.1 GL-only wrap or η-23 §3.2 PerDrawUBO 集約範式継続適用、multi-file: Tonemap 系 3 / FXAA 4 / 他 28)
5. **副 scope 候補 D**: parse failed `nameless block ... global scope` 5 件 (FrameAtmosphere_Skybox UBO 5 file declare 衝突、η-1 §3.1 instance name 付与 範式類で構造解決)
6. **副 scope 候補 B**: parse failed `'shadow_clip' : redefinition` 1 件 (Haze Shader L2262 literal 確定、η-20 §3.1 範式類)
7. **副 scope 候補 C**: parse failed `overlapping use of location` 5 件 (全 location 20 = V↔F slot alignment 単一 root cause、η-18 §3.1 範式類で 1 fix 5 件解消候補)
8. **A/B/C/D どれを Phase 1 主 scope に置くか + 副 scope を η-24 内 Phase 2 として同梱するか別 sub-bundle に分離するか** = AYA judgment 候補
9. **non-opaque 35 件 file 別棚卸し** (η-24 主 scope 候補 A): どの file から優先的に範式適用するか、ROI 最大の file 群 (Tonemap 系 / FXAA 系等の束) を最初に
10. **agent 報告 = 仮説扱いで literal 検証必須** (η-23 で agent 誤報告発覚の教訓、Explore agent を使う場合は Claude 自身で literal log + dump + source code 検証フォロー)
11. transformer Phase 3 拡張 (V stage bare `in` attribute 自動 location emit) は **将来 SPIR-V loc 大量発覚時 or 限定的に手動 layout 付与 で対応** を AYA judgment 候補
12. transformer version tag は η-18 末 `v5_p2_inout_pair_prepass_group_fix` 据置、η-24 で Phase 3 拡張時 `v6_p3_vertex_attribute` bump 必須
13. cinematic_bd directory audit (η-17 §10.4 継承) を η-24 着手前 or 完遂後の別 phase として継続推奨
14. **feedback_admit_unknown / feedback_one_step_at_a_time / feedback_no_scope_shrink 範式継承** で η-24 内でも literal observation 優先 + AYA judgment 受領後の scope 全実装維持

### §8.2 想定 Phase 構成 (η-24)

η-24 着手前 trace + AYA judgment で確定だが、現時点 推定:
- **Phase 1 (主 scope 候補 A 推奨)**: non-opaque uniforms 35 件 cascade 解消 (η-23 §3.1 / §3.2 範式継続)、file 別棚卸しでバッチ処理 (Tonemap 系 3 / FXAA 4 / 他 28)
- **Phase 2 (副 scope)**: FrameAtmosphere_Skybox nameless block 5 件 (η-1 §3.1 範式類で 5 file declare 整理) + shadow_clip redef 1 件 (η-20 §3.1 範式類) + overlapping loc 5 件 (η-18 §3.1 範式類で 1 fix 5 件解消候補)

### §8.3 B?-η-24 完遂後の想定 cascade exposure 第22層

- non-opaque 35 件解消想定 = parse 段階大量解消、第22層 emergence 観測点: pipeline cache / runtime binding / SPIR-V layout validation 等
- FrameAtmosphere_Skybox 5 件解消想定 = parse 段階大幅減少
- 残 shadow_clip 1 + overlapping 5 + phantom #endif 13 = η-25+ 移管想定
- shader_cache 件数: η-24 で計測継続 + 第22層 emergence 観測

---

## §9 commit message (記録)

patch commit `9cc143d2a6`:
```
feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-23 完遂

main scope = Phase 1 multiPointLightF.glsl 残 6 bare uniform 構造解消
= Deferred MultiLight Shader 0-15 全 16 件 parse error 完全消滅。
η-3 §3.2 PerDrawUBO 範式 + η-4 §3.2 FrameAtmosphere_Lighting per-group
rename 範式 + η-22 §3.1 GL-only wrap 範式の延長で、link 段階 ZERO
milestone を η-21 から 2 sub-bundle 連続維持。

Phase 1: Multi-Light non-opaque 16 → 0
- class3/deferred/multiPointLightF.glsl 6 bare uniform 解消
  - sun_wash / light_count (L54-57): dead in this file、
    η-22 §3.1 GL-only wrap で済
  - light[LIGHT_COUNT] / light_col[LIGHT_COUNT] (alive):
    新規 UBO PerDrawUBO_MultiLight (set=2, binding=1) member 化
    + GL path 用 #else fallback (η-3 §3.2 範式継承)
  - far_z (L102): PerDrawUBO_MultiLight member 化 (float)
  - classic_mode (L106): Vulkan path で FrameAtmosphere_Lighting UBO
    declare 追加 (set=0, binding=2、FRAME_ATMOSPHERE_LIGHTING_DEFINED
    guard、pointLightF.glsl L98-L120 同範式統一)、GL-only wrap で
    bare 維持
  - global_light_strength (L109): PerDrawUBO_MultiLight member 化
- PerDrawUBO_MultiLight 構造:
    vec4  light[LIGHT_COUNT];
    vec4  light_col[LIGHT_COUNT];
    float far_z;
    float global_light_strength;
    float _pad_ml0;
    float _pad_ml1;
  std140 alignment 揃え、末尾 float×2 + pad×2 で 16-byte chunk 整合。

主指標達成:
- Multi-Light non-opaque 16 → 0 (Phase 1 主目標完全達成)
- ERROR 計 139 → 104 (-35 = -16 構造解消 - 16 program 全救済
  cascade chain 同期消滅 - 3 その他連動)
- compilation errors summary 61 → 45 (-16 = Multi-Light 16 program
  全 parse 通過)
- glslang link failed for program 0 → 0 (link 段階 ZERO 連続維持)
- Deferred MultiLight Shader 0-15 全 16 件 SPIR-V module loaded
  (944 / 24260 bytes ×16) ✓
- clean shutdown 維持 (Goodbye 1 / VK destroyed 1 / FATAL/SIGSEGV/
  Aborted 0/0/0)

cascade shift forward 第21層は同 file 内 (far_z/classic_mode/
global_light_strength) で完結、第22層追加 cascade 露出なし。

Multi-Light 16 件解消後の残主要 parse 段階:
- non-opaque 35 件 (η-24 移管、Tonemap/FXAA 系)
- nameless block FrameAtmosphere_Skybox 5 件 (η-24 移管)
- shadow_clip redef 1 件 / overlapping loc 5 件 / phantom #endif 13 件
```

handoff doc commit message 案:
```
docs(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-23-complete handoff 起草

main scope = Phase 1 multiPointLightF.glsl 残 6 bare uniform 構造解消
= Deferred MultiLight Shader 0-15 全 16 件 parse error 完全消滅 +
link 段階 ZERO 2 sub-bundle 連続維持 の完遂 handoff doc。新規 §3.2
PerDrawUBO 範式拡張 (per-program 派生 + vec[] + scalar 混在 std140
alignment 整合) + §3.3 FrameAtmosphere_Lighting per-shader declare
統一範式 を策定。

主指標達成:
- Multi-Light non-opaque 16 → 0 (Phase 1 主目標完全達成)
- ERROR 計 139 → 104 (-35)
- glslang link failed for program 0 → 0 (link 段階 ZERO 2 sub-bundle 連続維持)
- Deferred MultiLight Shader 0-15 全 16 件 SPIR-V module loaded
- clean shutdown 維持
- 18 種既達主指標完全維持

cascade shift forward 第22層追加露出ゼロ:
- 同 file 内完結 (far_z / classic_mode / global_light_strength)
- η-24 主軸は別 file 群 (non-opaque 35 件 = Tonemap/FXAA 系)

patch commit: 9cc143d2a6
```

---

**handoff doc 完。次 sub-bundle B?-η-24 着手は本 doc §8 推奨 scope (parse non-opaque 35 件 + nameless block 5 件 + shadow_clip 1 + overlapping 5) を起点として、fresh context で実施。**
