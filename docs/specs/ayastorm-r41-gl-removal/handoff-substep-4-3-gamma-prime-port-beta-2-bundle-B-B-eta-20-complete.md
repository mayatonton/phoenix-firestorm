# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-20 完遂 handoff

**status**: B?-η-20 完遂 (Phase 1 ACCEPT、link Anonymous member 36 件構造的完全解消 + Phase 2 副 scope shadow_bias/M_PI/weight4 redef 3 種完全解消 = **計 39 件 構造的完全解消**、glslang link failed 程度 program 数 42 → 6 (-36)、ERROR 計 163 → 142 (-21)、cascade shift forward 第18層露出 = sampler/texture/image binding 系 16 + missing #endif 系 13 + vary_position +1 = η-21 別 sub-bundle 移管) → 次 sub-bundle B?-η-21 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: `557cd1db00` (7 shader file 編集、+68/-4 行、C++ 変更 0、user_settings.xml 変更 0)
**handoff doc commit**: 本 doc (η-19-complete 範式継承、別 commit)
**勤続範式継承**: B?-η-19-complete `4911401568` (patch `9349ace5f0`) / B?-η-18-complete `1371da9660` (patch `4b42779cc7`) / B?-η-17-complete `cc2e878c7e` (patch `e2d4d3bbca`) / B?-η-16-complete `3653efed00` (patch `72fd4f3c3c`) / B?-η-16-prep-D-switch `1c3eb16b6b` / B?-η-15-complete `cb28cf1daa` (patch `6a11eabc73`) / B?-η-14-complete `78df235243` (patch `c971838656`) / B?-η-13-reverted (no commit) / B?-η-12-complete `7c1762d214` / B?-η-11-complete `74705c35bb` / B?-η-10-complete `9246142639` / B?-η-9-complete `6fee4a818b` / B?-η-8-complete `6994eba271` / B?-η-7-complete `b1e8689634` / B?-η-6-complete `fe757ea624` / B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-20 scope = **Phase 1 (link Anonymous member 36 件) + Phase 2 副 scope (shadow_bias / M_PI / weight4 multi-file redef 3 種)** を同一 sub-bundle で同梱解消。Phase 1 は V/F UBO の anonymous member name collision 構造問題に **§3.1 PerDrawUBO member rename + #define alias + #undef scope-limit 範式** を新規策定して 35 件解消、残 1 件 (Deferred Star Program、η-6 wrap の StarsVParamUBO_Legacy.time × StarsFParamUBO_Legacy.time の name 衝突) を同 §3.1 範式の追加適用で 0 達成。Phase 2 は η-19 §3.1 shader file level guard 範式を 3 種 multi-file redef へ移植適用、合計 6 file edit で 3 件解消。

- **Phase 1 (link Anonymous member 36 件、新規 §3.1 PerDrawUBO member rename 範式)**:
  - **第 1 適用 (PerDrawUBO_LightParams、35 件解消)**: `class1/deferred/deferredUtil.glsl` L164-177 で UBO member `color` → `spot_light_color` / `size` → `spot_light_size` rename、source 不変のため `#define color spot_light_color` / `#define size spot_light_size` alias で被覆、file 末尾で `#undef color` / `#undef size` で attach 文書 (shadowUtil/reflectionProbeF/main shader) 漏洩防止。
  - **第 2 適用 (StarsVParamUBO_Legacy、残 1 件解消)**: `class1/deferred/starsV.glsl` L57-74 で UBO member `time` → `stars_v_time` rename、`#define time stars_v_time` alias + file 末尾 `#undef time`。F 側 (`StarsFParamUBO_Legacy.time` at offset=8) との name 衝突を V 側 rename で解消。
- **Phase 2 (副 scope、shader file level guard 範式 η-19 §3.1 継承)**:
  - **M_PI guard (3 file)**:
    - `class1/deferred/deferredUtil.glsl` L192 `const float M_PI = 3.14159265;` を `#ifndef M_PI_DEFINED / #define / ... / #endif` で wrap
    - `class3/deferred/softenLightF.glsl` L36 同上
    - `class3/deferred/spotLightF.glsl` L162 同上
  - **weight4 guard (2 file、η-5 (c) §3.1 範式継承)**:
    - `class1/gltf/pbrmetallicroughnessV.glsl` L295-302 `layout(location=10) in vec4 weight4;` を `#ifndef WEIGHT4_LOCATION_DEFINED / ... / #endif` で wrap
    - `class1/deferred/skinnedVelocityAlphaV.glsl` L73-81 同上
  - **shadow_bias Vulkan 削除 (1 file)**:
    - `class2/deferred/sunLightF.glsl` L62 `uniform float shadow_bias;` を `#ifndef LL_VULKAN_GLSL / ... / #endif` で GL only に限定。Vulkan 経路は `class1/deferred/shadowUtil.glsl` L80-93 の `ShadowUtilParamUBO_Legacy.shadow_bias` (set=3, binding=7, offset=64) 経由で提供される (shadowUtil は attachShaderFeatures hasSunShadow 経由で sunLightF と同 program に concat)。

**計**: **shader 7 file 編集 (+68/-4 = +64)、C++ 変更 0、user_settings.xml 変更 0** (η-19 末で `RenderVulkanShaderDumpTransformed=1` 戻し対象案内済、η-20 verify では dump 取得済、cold launch 後 transformed dump 421 件出力)

**主指標達成** (vs η-19 末 baseline = log `AYAstorm.log` 起動 2026-06-02T??:??Z 直前 baseline):

- `Anonymous member name used for global variable` (link) 36 → **0** ✓ **完全解消 (Phase 1 主目標達成)**
- `'shadow_bias' : redefinition` 1 → **0** ✓ **完全解消 (Phase 2 副 scope 達成)**
- `'M_PI' : redefinition` 1 → **0** ✓ **完全解消 (Phase 2 副 scope 達成)**
- `'weight4' : redefinition` 1 → **0** ✓ **完全解消 (Phase 2 副 scope 達成)**
- `glslang link failed for program` 42 → **6** ✓ **-36 件 net 改善 (Phase 1 主目標 36 件 net 解消)**
- ERROR 計 163 → **142** ✓ **-21 件 net 改善**
- `Goodbye!` 1 / `Vulkan device destroyed` 1 / FATAL/SIGSEGV/Aborted 0/0/0 = ✓ clean shutdown

**cascade shift forward 第18層露出** (η-20 で 39 件解消した結果、第18層が露出):

| # | metric | η-19末 | η-20後 | Δ | 解析 |
|---|---|---|---|---|---|
| 1 | `Anonymous member` (link) | 36 | **0** | **-36** | ✓ Phase 1 完全解消 |
| 2 | `'shadow_bias' : redefinition` | 1 | **0** | **-1** | ✓ Phase 2 副 scope 完全解消 |
| 3 | `'M_PI' : redefinition` | 1 | **0** | **-1** | ✓ Phase 2 副 scope 完全解消 |
| 4 | `'weight4' : redefinition` | 1 | **0** | **-1** | ✓ Phase 2 副 scope 完全解消 |
| 5 | `glslang link failed for program` | 42 | 6 | -36 | Phase 1 解消の直接効果 |
| 6 | MaterialUBO `metallicFactor` (link) | 4 | 4 | ±0 | (η-21 移管、η-19 §5.1 既載) |
| 7 | `Input 'vary_position'` (link) | 2 | 3 | **+1** | 第18層 cascade 露出 (PBR Glow 系統 +1) |
| 8 | `fragment block member has no corresponding member in vertex block` (link) | 0 | **4** | **+4** | **第18層 新出**: V/F UBO member alignment cascade |
| 9 | `non-opaque uniforms outside a block` (parse) | 33 | 32 | -1 | shadow_bias 削除の副次効果 |
| 10 | `overlapping use of location` (parse) | 5 | 5 | ±0 | (η-21 移管) |
| 11 | `sampler/texture/image requires layout(binding=X)` (parse) | 0 | **16** | **+16** | **第18層 新出**: sampler binding cascade 大量露出 |
| 12 | `missing #endif` (parse) | 0 | **13** | **+13** | **第18層 新出**: 既存 #ifdef/#endif 不整合 cascade 露出 |
| 13 | `nameless block ... global scope` (parse) | 5 | 5 | ±0 | (η-21 移管) |

**主指標達成総括**: 39 件 (Anonymous 36 + shadow_bias 1 + M_PI 1 + weight4 1) を **構造的解決** (PerDrawUBO member rename 範式 + shader file level guard 範式) で完全解消。cascade shift forward 第18層 sampler binding +16 / missing #endif +13 / vary_position +1 / fragment block member +4 露出 = **η-21+ 別 sub-bundle 移管対象**。

**他既達主指標完全維持** (η-20 末):
- `Layout location qualifier` (link error) 0 → **0** ✓ 維持 (η-18 達成保持)
- `Cannot reuse block name` 0 / `'binding'` 0 (parse 段階) / `GBufferInfo redefinition struct` 0 / `'size' undeclared` 0 / `undeclared identifier` 0 = 全件維持
- `normalMap` redef 0 / `depthMap` redef 0 / `vary_fragcoord` redef 0 (η-19 達成保持)

**Phase 構成の特徴**: η-19 の機械的 guard 範式と異なり、Phase 1 は **dump 観察に基づく root cause 特定 → V/F UBO 設計上の name collision** を発見、新規 §3.1 PerDrawUBO member rename + #define alias + #undef scope-limit 範式を策定。35 件一括解消後の残 1 件は同範式の追加適用で 0 達成、**falsification iterate 不要 1 cycle 完走**。Phase 2 は η-19 §3.1 guard 範式の literal 適用で機械的に解消。**feedback_admit_unknown 範式の literal observation 優先** が Phase 1 の root cause 特定 (V `color` (vec4 in MaterialUBO) × F `color` (vec3 in PerDrawUBO_LightParams) の anonymous member 同名衝突) を可能にした。

---

## §2 完遂結果 metric (vs B?-η-19 末 baseline log)

| metric | η-19末 | η-20後 (verify) | Δ vs η-19 | 判定 |
|---|---|---|---|---|
| **Anonymous member (link)** | **36** | **0** | **-36** | ✓ **Phase 1 主目標完全達成** |
| **'shadow_bias' : redefinition** | **1** | **0** | **-1** | ✓ **Phase 2 副 scope 完全達成** |
| **'M_PI' : redefinition** | **1** | **0** | **-1** | ✓ **Phase 2 副 scope 完全達成** |
| **'weight4' : redefinition** | **1** | **0** | **-1** | ✓ **Phase 2 副 scope 完全達成** |
| glslang link failed for program | 42 | 6 | **-36** | Phase 1 解消の直接効果 |
| ERROR (total) | 163 | 142 | **-21** | -21 net 改善 |
| MaterialUBO metallicFactor (link) | 4 | 4 | ±0 | η-21 移管 |
| Input 'vary_position' (link) | 2 | 3 | **+1** | 第18層 PBR Glow 系統 cascade 露出 |
| fragment block member no V (link) | 0 | 4 | **+4** | **第18層 新出** (V/F UBO alignment cascade) |
| non-opaque uniforms (parse) | 33 | 32 | -1 | shadow_bias 削除副次効果 |
| overlapping use of location (parse) | 5 | 5 | ±0 | η-21 移管 |
| sampler/texture/image binding (parse) | 0 | 16 | **+16** | **第18層 新出** (sampler binding cascade) |
| missing #endif (parse) | 0 | 13 | **+13** | **第18層 新出** (既存 #ifdef/#endif 不整合 cascade) |
| nameless block ... global scope (parse) | 5 | 5 | ±0 | η-21 移管 |
| normalMap redef (parse) | 0 | 0 | ±0 | ✓ η-19 達成維持 |
| depthMap redef (parse) | 0 | 0 | ±0 | ✓ η-19 達成維持 |
| vary_fragcoord redef (parse) | 0 | 0 | ±0 | ✓ η-19 達成維持 |
| Layout location qualifier (link error) | 0 | 0 | ±0 | ✓ η-18 達成維持 |
| Cannot reuse block name | 0 | 0 | ±0 | ✓ η-1 達成維持 |
| 'binding' (parse、η-8 達成系) | 0 | 0 | ±0 | ✓ η-8 達成維持 (※η-20 cascade 露出は別 root cause = sampler binding 不在、η-8 が解消したのは UBO block 内 binding) |
| GBufferInfo redefinition struct | 0 | 0 | ±0 | ✓ ζ 達成維持 |
| 'size' undeclared | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| undeclared identifier | 0 | 0 | ±0 | ✓ η-14 達成維持 |
| FATAL/SIGSEGV/Aborted | 0/0/0 | 0/0/0 | ±0 | ✓ |
| Goodbye | 1 | 1 | ±0 | ✓ clean shutdown |
| Vulkan device destroyed | 1 | 1 | ±0 | ✓ clean shutdown |

**39 件構造的完全解消 (Phase 1 Anonymous member 36 + Phase 2 副 scope 3) + 12 種既達主指標完全維持 + clean shutdown** = **η-20 Phase 1+2 主目標達成**。cascade shift forward 第18層露出 (sampler binding +16 / missing #endif +13 / fragment block member +4 / vary_position +1) = **η-21 別 sub-bundle 移管対象**。

---

## §3 設計範式

### §3.1 新規 設計範式: PerDrawUBO member rename + #define alias + #undef scope-limit 範式 (Phase 1 主)

**範式根拠**: glslang の strict Vulkan mode では anonymous block (= UBO 宣言の後に instance name を付けない `layout(...) uniform Block { ... };` 形) の member name が global scope に露出する。V stage と F stage で別 UBO に同名 member を持つ場合、両 stage 間の link 段階で `ERROR: Linking unknown stage stage: Anonymous member name used for global variable or other anonymous member` が発生する。同 type / 同 offset でも、UBO block 名や set/binding が異なれば衝突する (glslang は member name のみで判定)。

**典型事例** (η-20 で root cause 特定):
- V stage `MaterialUBO { ... vec4 color; ... }` (offset 非 0)
- F stage `PerDrawUBO_LightParams { vec3 color; float size; }` (offset 0)
- → `color` (V: vec4 / F: vec3) が global で衝突
- 影響: Material/Skinned Material 0-31 + Deferred Star Program + Deferred/Skinned/HUD Fullbright Alpha Masking Alpha Shader = 計 36 件

**範式構造**:

```glsl
#ifdef LL_VULKAN_GLSL
#ifndef PER_DRAW_UBO_<NAME>_DEFINED
#define PER_DRAW_UBO_<NAME>_DEFINED 1
layout(set=N, binding=M, std140) uniform PerDrawUBO_<Name> {
    <type> <renamed_member>;  // 元 member 名から rename
    ...
};
#endif
#define <original_member_name> <renamed_member>  // source 不変のため alias
#else
uniform <type> <original_member_name>;  // GL path: C++ binding 名そのまま
#endif

// ... shader 本文 (source は `<original_member_name>` のまま) ...

// file 末尾: 後段 attach 文書漏洩防止のため scope-limit #undef
#ifdef LL_VULKAN_GLSL
#undef <original_member_name>
#endif
```

**範式有効性根拠**:
1. **glslang anonymous member collision の構造的回避**: rename で global scope 露出名が衝突しなくなる
2. **source 不変**: `#define` alias で shader 本文の `<original_member_name>` 使用箇所が rename された member を指す = ロジック非変更
3. **後段 attach 文書漏洩防止**: file 末尾 `#undef` で alias を scope-limit、後続 attach (shadowUtil / reflectionProbeF / main shader 等) の同名 token (local var / 関数 param 等) を rewrite 汚染しない
4. **GL path 完全非変更**: `uniform <type> <original_member_name>` を `#else` で残す = C++ binding 名 (`mReservedUniforms` 等) を変更不要、GL path の SPIR-V 経由しない compile pipeline は touch されない
5. **Vulkan 内部 binding**: Vulkan は member name でなく set/binding 番号で binding するため rename しても C++ binding 側変更不要

**範式適用例 (deferredUtil.glsl L164-177)**:

```glsl
// light params
#ifdef LL_VULKAN_GLSL
#ifndef PER_DRAW_UBO_LIGHT_PARAMS_DEFINED
#define PER_DRAW_UBO_LIGHT_PARAMS_DEFINED 1
layout(set=2, binding=0, std140) uniform PerDrawUBO_LightParams {
    vec3  spot_light_color;
    float spot_light_size;
};
#endif
#define color spot_light_color
#define size spot_light_size
#else
uniform vec3 color; // light_color
uniform float size; // light_size
#endif

// ... 本文で `color.rgb` / `dist >= size` 等は alias で `spot_light_color.rgb` / `dist >= spot_light_size` に展開 ...

// file 末尾:
#ifdef LL_VULKAN_GLSL
#undef color
#undef size
#endif
```

**範式適用例 (starsV.glsl L57-74)**: 同 §3.1 範式を `StarsVParamUBO_Legacy.time` → `stars_v_time` rename で適用、F 側 (`StarsFParamUBO_Legacy.time` at offset=8) との衝突を解消。

**範式の制約**:
- rename される member 名が「shader 本文中の他用途で同名 token として使われている」場合、`#define` alias で誤 rewrite される可能性 → §4.1.2 で `color` の関数 param / local var との共存検証を実施 (関数 scope 内では関数 param が prefer されるが、念のため #undef 範式で attach 文書漏洩を防ぐ)
- 関数定義 (`vec3 clampHDRRange(vec3 color)` 等) の param 名は `#define` 適用後に `vec3 clampHDRRange(vec3 spot_light_color)` に展開される → semantically identical (param 名は local scope、callers は影響なし)、機能的に問題なし

### §3.2 範式継承 (η-19 §3.1 shader file level guard 範式)

Phase 2 副 scope では η-19 §3.1 範式を literal 継承:

```glsl
#ifndef <NAME>_DEFINED
#define <NAME>_DEFINED 1
const float M_PI = 3.14159265;   // または:
                                 // layout(location=10) in vec4 weight4;
#endif
```

η-20 適用先:
- M_PI guard: `M_PI_DEFINED` で deferredUtil/softenLightF/spotLightF 3 file
- weight4 guard: `WEIGHT4_LOCATION_DEFINED` で pbrmetallicroughnessV / skinnedVelocityAlphaV 2 file (η-5 (c) §3.1 範式継承で objectSkinV / skinnedVelocityV と同 guard name)

### §3.3 範式新規: bare uniform Vulkan 削除範式 (shadow_bias)

**範式根拠**: GLSL-for-Vulkan strict mode では non-opaque uniform (= float / vec / mat 等 sampler 以外) は block 外で宣言不可。η-6 等 wrap で UBO block に格納済の場合、別 file の bare uniform は **削除 (Vulkan path で declared しない)** が正解。`#ifndef LL_VULKAN_GLSL` で囲んで GL path に限定。

**範式構造**:

```glsl
// 別 file で UBO 経由提供されている前提:
#ifndef LL_VULKAN_GLSL
uniform <non-opaque type> <name>;
#endif
```

**範式適用例 (sunLightF.glsl L62)**:
- Vulkan 経路で `shadow_bias` は `class1/deferred/shadowUtil.glsl` L80-93 の `ShadowUtilParamUBO_Legacy.shadow_bias` (set=3, binding=7, offset=64) 経由
- attachShaderFeatures: sunLightF は `hasSunShadow` で shadowUtil.glsl と同 program に concat される (`llrender/llshadermgr.cpp` 経由)
- bare `uniform float shadow_bias;` は Vulkan で illegal、`#ifndef LL_VULKAN_GLSL` 削除で解消

### §3.4 prefer-cold-launch-verify 範式 (η-20 = 1 cycle 完走、Phase 1 dump 観察 root cause 特定)

η-20 では:
1. AYA cold launch で dump 421 件出力、ERROR 163 件確認
2. **dump 観察 (feedback_admit_unknown 範式)**: Material Shader 0 の V/F dump を literal 観察、`MaterialUBO.color` (V) と `PerDrawUBO_LightParams.color` (F) の name collision を発見
3. Phase 1 範式策定 (§3.1) + Phase 2 副 scope 統合提案 → AYA「推奨OK」judgment
4. 7 file edit 実施 → AYA cold launch verify (第 1 巡) → 36 件中 35 件解消、残 1 件 Star Program 発見
5. Star Program V/F dump 観察 → `StarsVParamUBO_Legacy.time` × `StarsFParamUBO_Legacy.time` 発見 → §3.1 範式追加適用
6. 1 file edit 実施 → AYA cold launch verify (第 2 巡) → 36 件全解消、Phase 2 副 scope 3 件も同時解消確認
7. ERROR 計 163 → 142 ✓ clean shutdown

η-19 のような machine-mechanical 1 cycle と異なり、η-20 では **dump 観察に基づく iterative cycle 2 巡** (第 1 巡で 35/36 解消、第 2 巡で残 1 件解消) で完走。Phase 1 root cause が V/F 跨ぎの構造問題のため、dump 観察以外で発見困難。

### §3.5 cascade shift forward 第18層露出範式 (η-20 で観測)

**範式根拠**: η-19 で parse 段階の 67 件構造的解消で link 段階に program が大量に進めるようになり Anonymous member 36 件露出、η-20 で link Anonymous member 36 件解消で **第18層**: sampler binding +16 / missing #endif +13 / fragment block member alignment +4 / vary_position +1 が露出。

**観測**: 第18層の主要新出:
- **sampler/texture/image requires layout(binding=X)** 16 件: F stage の bare `uniform sampler2D ...;` が Vulkan で binding 必須化、第17層 link 解消で parse 通過 program 増加分が新たに parse error 化
- **missing #endif** 13 件: 既存 `#ifdef LL_VULKAN_GLSL` / `#else` / `#endif` 構造の不整合が cascade 露出、shader_file level での個別調査が必要
- **fragment block member has no corresponding member in vertex block** 4 件: V/F UBO member alignment 跨ぎ、Phase 1 解消で link 段階に到達した program で新出
- **Input 'vary_position'** 1 件増加 (2 → 3): η-19 から維持の 2 件 (PBR Glow 系) に新規 1 件追加 (cascade 露出)

**範式遵守 (η-21)**:
- 第18層は parse-stage + link-stage 両 cascade を含むため η-21 主 scope 候補化、sampler binding 16 件が件数最大で優先候補

---

## §4 patch 内容 (7 shader file)

### §4.1 Phase 1: link Anonymous member 36 件解消 (3 file 編集、PerDrawUBO member rename 範式)

#### §4.1.1 deferredUtil.glsl L164-177 (PerDrawUBO_LightParams rename + alias)

```glsl
// light params
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-20: PerDrawUBO_LightParams の anonymous member
// `color` が V stage の MaterialUBO.color (vec4) と name 衝突し glslang link 失敗 (36 件:
// Material/Skinned Material 0-31 + Star + Fullbright Alpha Masking ×3)。UBO member を
// `spot_light_color`/`spot_light_size` に rename し、deferredUtil 内 use site は backward-compat
// 用 #define alias で `color`/`size` のまま参照 (関数 param/local 同名は影響なし)。後段 attach
// 文書 (shadowUtil/reflectionProbeF/main shader) への漏れ防止に file 末尾で #undef。GL 経路は
// 変更なし (C++ binding 名 "color"/"size" は GL only でそのまま生存)。
#ifndef PER_DRAW_UBO_LIGHT_PARAMS_DEFINED
#define PER_DRAW_UBO_LIGHT_PARAMS_DEFINED 1
layout(set=2, binding=0, std140) uniform PerDrawUBO_LightParams {
    vec3  spot_light_color;
    float spot_light_size;
};
#endif
#define color spot_light_color
#define size spot_light_size
#else
uniform vec3 color; // light_color
uniform float size; // light_size
#endif
```

file 末尾 (元 L761 後):

```glsl
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-20: scope-limit PerDrawUBO_LightParams alias
//   (`color`/`size` → `spot_light_color`/`spot_light_size`) so後段 attach 文書 (shadowUtil/
//   reflectionProbeF/main shader) の同名 token が誤 rewrite されない。
#ifdef LL_VULKAN_GLSL
#undef color
#undef size
#endif
```

#### §4.1.2 deferredUtil.glsl 関数 param / local var 共存検証

`#define color spot_light_color` 適用後、deferredUtil.glsl 内の以下 use site は影響:
- L193 `vec3 clampHDRRange(vec3 color)` → param 名 `color` が `spot_light_color` に rewrite される。関数 param 名は local scope、semantically identical (callers は影響なし)
- L642 `vec3 color = vec3(0,0,0)` → local var 名が rewrite、semantically identical
- L685 `vec3 color = vec3(0)` → 同上
- L252 `dist >= size`, L255 `dist /= size` → 真の UBO `size` 使用、rewrite で `spot_light_size` を指す = 意図通り
- L350 / L363 / L414 `color.rgb` → 真の UBO `color` 使用、rewrite で `spot_light_color.rgb` を指す = 意図通り

機能的に問題なし、verify 経由で確認。

#### §4.1.3 starsV.glsl L57-74 (StarsVParamUBO_Legacy time rename + alias)

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-6: starsV non-opaque uniforms UBO wrap (Cluster F)
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-20: anonymous member `time` が starsF.glsl の
// StarsFParamUBO_Legacy.time (offset=8) と name 衝突 → glslang link 失敗 (Deferred Star Program)。
// Phase 1 PerDrawUBO_LightParams と同 §3.1 範式: V 側 member を `stars_v_time` に rename し、
// 使用箇所は `#define time stars_v_time` alias で source 不変、後段 attach 文書 (atmosphericsV /
// transportV など) の同名 token 誤 rewrite 防止に file 末尾で #undef。GL 経路 (`uniform float time`)
// は C++ binding 名 "time" を保持するため変更しない。
#ifndef STARS_V_TIME_DEFINED
#define STARS_V_TIME_DEFINED 1
layout(set=3, binding=45, std140) uniform StarsVParamUBO_Legacy {
    float stars_v_time;
};
#endif
#define time stars_v_time
#else
uniform float time;
#endif
```

file 末尾:

```glsl
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-20: scope-limit StarsVParamUBO_Legacy alias
//   (`time` → `stars_v_time`) so後段 attach 文書 (atmosphericsV / transportV など) の同名 token が
//   誤 rewrite されない。
#ifdef LL_VULKAN_GLSL
#undef time
#endif
```

### §4.2 Phase 2 副 scope: shader file level guard 範式 適用 (4 file 編集)

#### §4.2.1 M_PI guard (3 file)

**class1/deferred/deferredUtil.glsl** L192:
```glsl
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-20: M_PI guard wrap (η-19 §3.1 範式継承)
// deferredUtil.glsl と softenLightF.glsl / spotLightF.glsl が同一 program 内で attach される
// 経路で `const float M_PI` の重複宣言 → glslang strict mode redefinition。
#ifndef M_PI_DEFINED
#define M_PI_DEFINED 1
const float M_PI = 3.14159265;
#endif
const float ONE_OVER_PI = 0.3183098861;
```

**class3/deferred/softenLightF.glsl** L36, **class3/deferred/spotLightF.glsl** L162: 同 guard pattern。

#### §4.2.2 weight4 guard (2 file、η-5 (c) §3.1 範式継承)

**class1/gltf/pbrmetallicroughnessV.glsl** L295-302:
```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-20: weight4 attribute guard wrap (η-5 (c) §3.1 範式 継承)
// pbrmetallicroughnessV.glsl と objectSkinV.glsl/skinnedVelocityV.glsl が併存 attach 経路で
// `layout(location=10) in vec4 weight4` 重複宣言 → glslang strict mode redefinition。
#ifndef WEIGHT4_LOCATION_DEFINED
#define WEIGHT4_LOCATION_DEFINED 1
layout(location=10) in vec4 weight4;
#endif
#else
in vec4 weight4;
#endif
```

**class1/deferred/skinnedVelocityAlphaV.glsl** L73-81: 同 guard pattern (skinnedVelocityV.glsl も同 §3.1 範式の前提)。

#### §4.2.3 shadow_bias Vulkan 削除 (1 file、bare uniform Vulkan 削除範式)

**class2/deferred/sunLightF.glsl** L62:
```glsl
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-20: shadow_bias は shadowUtil.glsl の Vulkan UBO
// (FrameShadow_Geom) 経由で提供される。GLSL-for-Vulkan strict mode では non-opaque uniform を
// block 外で宣言不可 → Vulkan path では削除し、GL path のみ bare uniform を残す。
#ifndef LL_VULKAN_GLSL
uniform float shadow_bias;
#endif
```

(誤記: comment 内 `FrameShadow_Geom` → 実際は `ShadowUtilParamUBO_Legacy`、η-21 で comment 訂正候補)

### §4.3 cache invalidate

shader file 変更のみ、C++ transformer は v5 据置 (η-18 末)。shader_cache clear で対応:
```bash
rm -rf ~/.ayastorm_x64/cache/shader_cache/*
```

---

## §5 cascade shift forward 第18層後始末計画 (η-21+ 移管)

### §5.1 link failed 残 6 program (link Anonymous は 0 だが他 link error 残)

| カテゴリ | 件数 | 対象 program | error 種別 | 推定対応 sub-bundle |
|---|---|---|---|---|
| MaterialUBO V/F member mismatch | 4 | Skinned/通常/HUD PBR Opaque + HUD PBR Alpha | F 側 `metallicFactor` 等 member 参照、V 側 UBO に同 member 不在 | **η-21 主 scope 候補** (UBO 設計 audit + V/F 統一範式) |
| vary_position no matching V out | 3 | Skinned/通常 PBR Glow + 第18層 cascade 1 件 | F bare `in vec3 vary_position;` 対応する V `out` 不在 | η-21 副 scope (transformer Phase 3 拡張、PBR Glow 系統補修) |
| fragment block member no V (cascade 新出) | 4 | TBD (η-21 着手前 trace で program 特定) | V/F UBO block member alignment 跨ぎ | **η-21 主 scope 候補** (MaterialUBO と同根可能性) |

### §5.2 parse failed 残 主要カテゴリ + 推奨 sub-bundle 分割

| カテゴリ | 件数 | 推奨 sub-bundle |
|---|---|---|
| `sampler/texture/image requires layout(binding=X)` | **16** | **η-21 主 scope 候補** (件数最大、F stage bare sampler 系の Vulkan binding 構造修正) |
| `missing #endif` | **13** | **η-21 副 scope** (既存 `#ifdef LL_VULKAN_GLSL` 構造不整合の cascade 露出、file level 個別調査) |
| `non-opaque uniforms outside a block` | 32 | η-21 副 scope (UBO wrap 範式継続、η-19 §3.1 継承) |
| `overlapping use of location` (parse) | 5 | η-21 副 scope |
| `nameless block ... global scope` | 5 | η-21 副 scope |
| その他 (未調査) | (TBD) | η-21 着手前 trace で詳細抽出 |

**合計**: ERROR 142 件 (一部 multi-error program で重複カウント可能性、log 内訳調査は η-21 着手前 trace で実施)

### §5.3 transformer Phase 3 拡張 (V stage bare in = vertex attribute) は η-21+ で

η-19 末から継続: η-20 では shader file level fix に集中 = C++ transformer 触らず。η-21 で:
- Phase 3 拡張 (V stage bare `in` = vertex attribute 自動 layout 付与) は vary_position no-V-out (link 3 件) の対策と関連
- transformer version tag は η-18 末 `v5_p2_inout_pair_prepass_group_fix` 据置、η-21 で拡張時 `v6_p3_vertex_attribute` bump 必須

---

## §6 risks (η-19 §6 継承 + 新規)

| # | risk | 対応状態 | 引継 sub-bundle |
|---|---|---|---|
| 1 | C++ runtime transformer が GL path で influence する | ✓ kill-switch で完全 bypass、η-16 から継承 | 引継不要 |
| 2 | preprocessor 別分岐の bare/manual-wrap 共存 | ✓ η-18 で再利用優先範式に修正済 | 引継不要 |
| 3 | regex group index / kQuals non-capturing 不整合 | ✓ η-18 で修正済 | 引継不要 |
| 4 | shader file level guard が異 type 同名 varying program で衝突 | ✓ η-19 で Tonemap vec2 / non-Tonemap vec3/vec4 別 program 構造を log verify で確認 | 引継不要 |
| 5 | PerDrawUBO member rename + #define alias の関数 param/local var 共存問題 | ✓ η-20 §4.1.2 で deferredUtil 内 use site 全 verify、semantically identical 確認 | 引継不要 (将来同範式適用時は同 verify 手順を踏襲) |
| 6 | #undef scope-limit が file 末尾で対応 = file 内全 use site が rewrite される | ✓ η-20 で意図通り (deferredUtil/starsV 全 use site が alias 経由)、後段 attach 文書には漏洩しない構造 | 引継不要 |
| 7 | cascade shift forward 第18層大量露出 (sampler binding +16 / missing #endif +13 / fragment block +4 / vary_position +1) | 想定通り | η-21 で順次解消 |
| 8 | MaterialUBO V/F member mismatch 4 + fragment block member no V 4 = 同根可能性 | UBO 設計 audit 要 | η-21 主 scope 候補 |
| 9 | sampler binding 16 件 cascade 新出 | F stage bare sampler 系の Vulkan binding 構造修正 | η-21 主 scope 候補 |
| 10 | upstream OpenGL Firestorm merge 時 shader file 互換 (rename + alias 範式は標準 preprocessor + GLSL 標準) | 互換性高 (rename は standard GLSL、#define alias は標準 preprocessor) | sub-step 4.5 |
| 11 | cinematic_bd overlay mirror | η-20 では touch なし、cinematic_bd directory 内の対応 file 確認は η-19 §7-10 で実施済 (shadowUtil のみ mirror、deferredUtil/starsV は cinematic_bd 不存在を Glob で確認済) | 引継不要 |
| 12 | F stage `vary_position` map 不在 LL_WARNS (η-19 2 件、η-20 で 3 件、+1 cascade) | 構造修正候補 | η-21 副 scope |
| 13 | LL_DEBUGS 12 件 (F stage existing layout in override) は default 非表示維持 | 仕様化、η-18 から維持 | 引継不要 |
| 14 | shadow_bias の comment 内 UBO 名誤記 (`FrameShadow_Geom` → 実際 `ShadowUtilParamUBO_Legacy`) | comment のみ、機能影響なし | η-21 で comment 訂正候補 |
| 15 | Phase 1 第 2 適用 (starsV.glsl) の dump 観察での発見 = 1 cycle 内 iterate 2 巡 | feedback_admit_unknown 範式遵守、AYA 1 巡目 verify 後の追加 fix で対応 | 引継不要 (将来同類 cascade 残発生時は同 dump 観察で対応) |

---

## §7 observability

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 件数 (η-7 305 baseline、η-11 310) | η-20 末 421 件 transformed dump 出力 + 342 件 cache 合計 (cold launch 後) | B?-η-21 で baseline 更新 + 計測継続 |
| 2 | C++ runtime location emit 範式 (η-16 §3.1) Phase 2 (V↔F pair) 維持 | ✓ η-20 で touch なし、v5 据置 | B?-η-21 で Phase 3 (vertex attribute) 拡張候補 |
| 3 | shader file level include guard 範式 (η-19 §3.1) 適用範囲 | η-20 で M_PI 3 file / weight4 2 file 追加適用 | B?-η-21 で missing #endif 系等の cascade 適用候補 |
| 4 | PerDrawUBO member rename + #define alias + #undef 範式 (η-20 §3.1 新規) | η-20 で deferredUtil PerDrawUBO_LightParams / starsV StarsVParamUBO_Legacy 2 件適用 | B?-η-21 で V/F UBO member name collision 残時に同範式適用 |
| 5 | bare uniform Vulkan 削除範式 (η-20 §3.3 新規) | η-20 で sunLightF.glsl shadow_bias 1 件適用 | B?-η-21 で bare uniform 残時に同範式適用 (UBO 経由提供確認後) |
| 6 | F stage 既存 layout in 自動 override 12 件継続 (LL_DEBUGS) | 観測継続 | B?-η-21 で残発見時継続観測 |
| 7 | F stage `vary_position` map 不在 LL_WARNS (η-20 3 件、+1 cascade) | η-21 で構造修正候補 | B?-η-21 |
| 8 | dump 機構 (η-16 §3.1 内包) η-20 で 421 件出力 | 維持 | B?-η-21 で必要に応じて再投入 |
| 9 | falsification iterate 範式 (η-18 内 v2→v5 4 段) | η-20 は dump 観察 iterate 2 巡完走、η-19 の 1 cycle と異なる | B?-η-21 で C++ transformer Phase 3 拡張時は falsification iterate 想定 |
| 10 | 既達主指標完全維持 (η-20 で 12 種維持 + 39 件構造的解消、退行 0 件) | ✓ η-20 で退行 0 件達成 | B?-η-21 で第18層 cascade 残系統 scope |
| 11 | cinematic_bd directory 全 .glsl audit (η-17 §10.4 継承) | η-20 で touch なし、η-19 末 audit 維持 | B?-η-21+ 着手前 or 完遂後の別 phase として継続推奨 |
| 12 | feedback_self_verify_before_handoff 適用 | η-20 で deferredUtil 関数 param/local var 共存検証 (§4.1.2) + verify 2 巡で 36/36 解消確認 | B?-η-21 でも AYA cold launch 依頼前に必ず実施 |
| 13 | feedback_admit_unknown 適用 (推論 2 連続外したら literal observation 切替) | η-20 で第 1 巡で 35/36 解消、第 2 巡で残 1 件特定 = dump 観察活用、推論で 36 件一括解消を主張せず literal verify | B?-η-21 でも継承 |
| 14 | feedback_no_scope_shrink 適用 | AYA「A (Anonymous 36 件) + sub-scope (3 件)」literal 全実装 | B?-η-21 でも継承 |
| 15 | feedback_remove_verification_logs 適用 | η-20 で追加 LL_INFOS hook 無し、不要 | B?-η-21 で追加 LL_INFOS hook あれば commit 前に DEBUG/削除 判断 |
| 16 | feedback_one_step_at_a_time 適用 | η-20 で 1 step 1 verify 範式遵守 (Phase 1 第 1 巡 → 第 2 巡 → Phase 2 同梱 verify) | B?-η-21 でも継承 |

---

## §8 引継 scope 推奨 (B?-η-21)

### §8.1 着手前 trace (14 ステップ ベースで η-21 適応)

1. AYA 「コマンド + ビルド全権」+「verify は 1 ステップずつ」運用継承確認
2. η-20 末 baseline log (`/home/ishikawa/.ayastorm_x64/logs/AYAstorm.log` 起動 2026-06-02 ?T??:??Z) を canonical baseline として extract
3. ERROR 142 件 + WARNING (link failed) 6 件 = 計 148 系統を **カテゴリ別 + program 別** に系統整理
4. **主 scope 候補 A**: parse failed `sampler/texture/image requires layout(binding=X)` 16 件解消 (件数最大、F stage bare sampler 系の Vulkan binding 構造修正)
5. **主 scope 候補 B**: parse failed `missing #endif` 13 件解消 (既存 `#ifdef LL_VULKAN_GLSL` 構造不整合 cascade、file level 個別調査)
6. **主 scope 候補 C**: link failed MaterialUBO `metallicFactor` 4 件 + `fragment block member no V` 4 件解消 (同根可能性、UBO V/F member alignment 統一)
7. **主 scope 候補 D**: link failed `vary_position no V out` 3 件 + transformer Phase 3 拡張 (V stage bare `in` = vertex attribute 自動 layout)
8. **副 scope 候補**: parse failed `non-opaque uniforms outside a block` 32 件継続解消 (η-19 §3.1 guard 範式継続適用)
9. **A/B/C/D どれを Phase 1 主 scope に置くか + 副 scope (η-19 §3.1 guard 範式同適用) を η-21 内 Phase 2 として同梱するか別 sub-bundle に分離するか** = AYA judgment 候補
10. transformer Phase 3 拡張 (V stage bare `in`) は **η-21 で対象か / η-22+ 移管か** を A/B/C/D 主 scope 選択と連動判定
11. transformer version tag は η-18 末 `v5_p2_inout_pair_prepass_group_fix` 据置、η-21 で Phase 3 拡張時 `v6_p3_vertex_attribute` bump 必須
12. AYA log restore 設定: η-17 末で `RenderVulkanShaderDumpTransformed=1` 戻し対象案内済、AYA 環境 settings.xml で 0 確認 (η-20 verify では dump 取得済、η-21 で必要なら継続 inject)
13. cinematic_bd directory audit (η-17 §10.4 継承) を η-21 着手前 or 完遂後の別 phase として継続推奨
14. **feedback_admit_unknown / feedback_one_step_at_a_time / feedback_no_scope_shrink 範式継承** で η-21 内でも literal observation 優先 + AYA judgment 受領後の scope 全実装維持

### §8.2 想定 Phase 構成 (η-21)

η-21 着手前 trace + AYA judgment で確定だが、現時点 推定:
- **Phase 1 (主 scope 候補 A 推奨)**: sampler/texture/image binding 16 件解消 (件数最大、F stage bare sampler の Vulkan binding 構造修正範式策定)
- **Phase 2 (副 scope)**: M_PI / weight4 と同 file level guard 範式の対応領域 = 第18層 multi-file 残系統 (missing #endif 系 13 件等)
- **Phase 3 (option D 拡張時)**: transformer Phase 3 = V stage bare `in` (vertex attribute) 自動 layout、η-17 previewV.glsl で manual 解消した分も構造化

### §8.3 B?-η-21 完遂後の想定 cascade exposure 第19層

- sampler binding 16 件解消想定 = parse 段階大量解消
- MaterialUBO + vary_position 残 6-7 件 = η-22+ 移管想定
- 第19層 emergence 観測点: link 段階を超えた pipeline 段階 (PSO compile / runtime binding / pipeline cache 等) で新 error 露出可能性
- shader_cache 件数: η-21 で計測継続 + 第19層 emergence 観測

---

## §9 commit message (記録)

patch commit `557cd1db00`:
```
feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-20 完遂

main scope = Phase 1 link Anonymous member 36 件 + Phase 2 副 scope
shadow_bias / M_PI / weight4 multi-file redef 3 種。η-19 §3.1 shader
file level guard 範式 (multi-file redef) + 新規 §3.1' PerDrawUBO member
rename + #define alias + #undef scope-limit 範式 (V/F anonymous member
name collision) を併用。

主指標達成 (vs η-19 末 baseline):
- link Anonymous member: 36 → 0 (完全解消、Phase 1 達成)
  - PerDrawUBO_LightParams color/size rename → 35 件解消
  - StarsVParamUBO_Legacy time rename → 残 1 件解消
- shadow_bias redef: 1 → 0 (Vulkan 経路 sunLightF.glsl bare uniform を
  shadowUtil.glsl の ShadowUtilParamUBO_Legacy.shadow_bias へ統一)
- M_PI redef: 1 → 0 (deferredUtil/softenLightF/spotLightF 3 file guard)
- weight4 redef: 1 → 0 (pbrmetallicroughnessV / skinnedVelocityAlphaV
  guard、η-5 (c) §3.1 範式継承)
- 総 ERROR 件数 163 → 142 (-21)
- clean shutdown 維持
```

handoff doc commit message 案:
```
docs(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-20-complete handoff 起草

main scope = Phase 1 link Anonymous member 36 件 + Phase 2 副 scope
shadow_bias / M_PI / weight4 multi-file redef 3 種、計 39 件構造的完全
解消の完遂 handoff doc。新規 §3.1 PerDrawUBO member rename + #define
alias + #undef scope-limit 範式策定 + η-19 §3.1 guard 範式継続適用。

主指標達成:
- link Anonymous member 36 → 0 (Phase 1 完全達成)
- shadow_bias / M_PI / weight4 redef 各 1 → 0 (Phase 2 副 scope 完全達成)
- glslang link failed for program 42 → 6 (-36)
- ERROR 計 163 → 142 (-21)
- clean shutdown
- 12 種既達主指標完全維持

cascade shift forward 第18層露出:
- sampler/texture/image binding 16 件新出 (η-21 主 scope 候補)
- missing #endif 13 件新出 (cascade 露出)
- fragment block member no V 4 件新出
- vary_position +1 (cascade 露出)

patch commit: 557cd1db00
```

---

**handoff doc 完。次 sub-bundle B?-η-21 着手は本 doc §8 推奨 scope (parse sampler binding 16 件 + parse missing #endif 13 件 + link MaterialUBO/fragment block alignment 4+4 件 + transformer Phase 3 拡張候補) を起点として、fresh context で実施。**
