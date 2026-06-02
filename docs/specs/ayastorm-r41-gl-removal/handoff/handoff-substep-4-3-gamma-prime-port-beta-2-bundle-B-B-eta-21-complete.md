# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-21 完遂 handoff

**status**: B?-η-21 完遂 (Phase 1 A ACCEPT + Phase 2 C+D ACCEPT、sampler binding 16 件構造的完全解消 + link 6 program 完全解消 = **計 23 件 構造的完全解消**、glslang link failed for program 6 → **0** 到達 = **link 段階 milestone**、ERROR 計 142 → 154 (+12 cascade exposure 第19層: env_mat redeclare +16 / SPIR-V requires location +3 / non-opaque cascade +16)、clean shutdown 維持) → 次 sub-bundle B?-η-22 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: `e78ca6006c` (5 shader file 編集、+44/-10 行、C++ 変更 0、user_settings.xml 変更 0)
**handoff doc commit**: 本 doc (η-20-complete 範式継承、別 commit)
**勤続範式継承**: B?-η-20-complete `d7c722f4e7` (patch `557cd1db00`) / B?-η-19-complete `4911401568` (patch `9349ace5f0`) / B?-η-18-complete `1371da9660` (patch `4b42779cc7`) / B?-η-17-complete `cc2e878c7e` (patch `e2d4d3bbca`) / B?-η-16-complete `3653efed00` (patch `72fd4f3c3c`) / B?-η-16-prep-D-switch `1c3eb16b6b` / B?-η-15-complete `cb28cf1daa` (patch `6a11eabc73`) / B?-η-14-complete `78df235243` (patch `c971838656`) / B?-η-13-reverted (no commit) / B?-η-12-complete `7c1762d214` / B?-η-11-complete `74705c35bb` / B?-η-10-complete `9246142639` / B?-η-9-complete `6fee4a818b` / B?-η-8-complete `6994eba271` / B?-η-7-complete `b1e8689634` / B?-η-6-complete `fe757ea624` / B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-21 scope = **Phase 1 (A: sampler binding 16 件) + Phase 2 (C: MaterialUBO V/F member alignment 4 件) + Phase 2 (D: vary_position dead declaration 削除 3 件)** を同一 sub-bundle で同梱解消。AYA judgment 受領後の 1 cycle で完走、3 Phase 同梱 commit。η-20 (39 件) と並ぶ規模の構造的完全解消 23 件 + **link 段階 ZERO 到達 milestone**。

- **Phase 1 (A: sampler binding 16 件、bare opaque uniform Vulkan binding 付与範式)**:
  - **適用 (multiPointLightF.glsl L34)**: `uniform sampler2D lightFunc;` を `#ifdef LL_VULKAN_GLSL / layout(set=0, binding=5) uniform sampler2D lightFunc; / #else / bare uniform / #endif` wrap。set/binding 番号は materialF / pointLightF / softenLightF / spotLightF と統一 (lightFunc は viewer 全体で同 channel)。影響: Deferred MultiLight Shader 0-15 全 16 件の parse error 解消。

- **Phase 2 (C: MaterialUBO V/F member alignment、4 file edit)**:
  - **適用 (pbropaqueV.glsl L63 第 1 declaration / L216 第 2 declaration)** + **(pbralphaV.glsl L60 第 1 declaration / L240 第 2 declaration)**: V 側 MaterialUBO の末尾に `metallicFactor / roughnessFactor / _pad_material0 / _pad_material1` を 4 float 追加。F 側 (pbropaqueF.glsl L44 / pbralphaF.glsl L31, L351) は既に同 members 含む。Vulkan strict mode では V/F 同名 UBO block の member set が **完全一致** 必須 (glslang link error "fragment block member has no corresponding member in vertex block")。
  - 影響: Skinned Deferred PBR Opaque / Deferred PBR Opaque / HUD PBR Opaque / HUD PBR Alpha 計 4 件 link 失敗解消。

- **Phase 2 (D: vary_position dead declaration 削除、2 file edit)**:
  - **適用 (pbrglowF.glsl L60-64)**: F main() L116-134 で参照ゼロの `vary_position` を削除 (V `pbrglowV.glsl` 側にも宣言なし、純粋な dead code)。影響: Skinned PBR Glow / PBR Glow 計 2 件 link 失敗解消。
  - **適用 (pbropaqueF.glsl HUD path L315-319)**: HUD path main() L371-396 で参照ゼロの `vary_position` を削除。HUD path V (pbropaqueV.glsl L209-295) は vary_position 不在 → 整合解消。**注**: 非 HUD path L79 の宣言は L216 `mirrorClip(vary_position);` で使用、残置 (V 側 pbropaqueV.glsl L159 に対応 out あり、整合済)。
  - 影響: HUD PBR Opaque 1 件 link 失敗解消。

**計**: **shader 5 file 編集 (+44/-10 = +34)、C++ 変更 0、user_settings.xml 変更 0** (η-20 末で `RenderVulkanShaderDumpTransformed=1` 戻し対象案内済、η-21 verify では dump 取得済、cold launch 後 transformed dump 約 421 件出力継続)

**主指標達成** (vs η-20 末 baseline = log `AYAstorm.log` 起動 2026-06-02T10:24Z 直前 baseline):

- `glslang link failed for program` 6 → **0** ✓ **link 段階 ZERO 到達 milestone (Phase 1+2 主目標達成)**
- `sampler/texture/image requires layout(binding=X)` (parse) 16 → **0** ✓ **完全解消 (Phase 1 A 主目標達成)**
- `Block: MaterialUBO, Member: metallicFactor` / `fragment block member has no corresponding member in vertex block` (link) 4 → **0** ✓ **完全解消 (Phase 2 C 主目標達成)**
- `Input 'vary_position'` (link) 3 → **0** ✓ **完全解消 (Phase 2 D 主目標達成)**
- ERROR 計 142 → **154** **+12 cascade exposure** (第19層、後述)
- `Goodbye!` 1 / `Vulkan device destroyed` 1 / FATAL/SIGSEGV/Aborted 0/0/0 = ✓ clean shutdown

**cascade shift forward 第19層露出** (η-21 で 23 件解消した結果、第19層が露出):

| # | metric | η-20末 | η-21後 | Δ | 解析 |
|---|---|---|---|---|---|
| 1 | `glslang link failed for program` | 6 | **0** | **-6** | ✓ **link 段階 ZERO 到達** |
| 2 | `sampler binding` (parse) | 16 | **0** | **-16** | ✓ Phase 1 A 完全解消 |
| 3 | `metallicFactor / fragment block` (link) | 4 | **0** | **-4** | ✓ Phase 2 C 完全解消 |
| 4 | `Input 'vary_position'` (link) | 3 | **0** | **-3** | ✓ Phase 2 D 完全解消 |
| 5 | `env_mat cannot redeclare a user-block member array` (parse) | 0 | **16** | **+16** | **第19層 新出**: multiPointLightF.glsl L36 `uniform vec3 env_mat[3]` × FrameViewProj UBO 内 `mat3 env_mat` 名前衝突、sampler binding fix で program が parse 段階 stage 通過した結果露出 (Deferred MultiLight Shader 0-15) |
| 6 | `SPIR-V requires location for user input/output` (parse) | 0 | **3** | **+3** | **第19層 新出**: bare in/out で transformer 未捕捉系 (η-22 main scope 候補) |
| 7 | `non-opaque uniforms outside a block` (parse) | 32 | **48** | **+16** | 第19層 cascade exposure (link 解消した program が parse 段階 stage 通過した結果露出) |
| 8 | `missing #endif` (parse、phantom) | 13 | **13** | ±0 | phantom 維持 (handoff §2.1 で literal observation 済、独立 fix 不可、先行 ERROR fix で同時消滅) |
| 9 | `overlapping use of location` (parse) | 5 | **5** | ±0 | (η-22 副 scope 移管) |
| 10 | `nameless block ... global scope` (parse、FrameAtmosphere_Skybox) | 5 | **5** | ±0 | (η-22 副 scope 移管) |
| 11 | `Anonymous member name used for global variable` (link) | 0 | **0** | ±0 | ✓ η-20 達成維持 |
| 12 | `'shadow_bias' : redefinition` | 0 | **0** | ±0 | ✓ η-20 達成維持 |
| 13 | `'M_PI' : redefinition` | 0 | **0** | ±0 | ✓ η-20 達成維持 |
| 14 | `'weight4' : redefinition` | 0 | **0** | ±0 | ✓ η-20 達成維持 |

**主指標達成総括**: 23 件 (Phase 1 sampler 16 + Phase 2-C MaterialUBO 4 + Phase 2-D vary_position 3) を **構造的解決** (bare opaque uniform Vulkan binding 付与 + V/F MaterialUBO member alignment + dead code removal) で完全解消。**link 段階を 6 → 0 milestone 到達** = η-1〜η-20 全周回で初の link failed 件数 ZERO 達成。cascade shift forward 第19層 env_mat +16 / SPIR-V +3 / non-opaque +16 = **η-22+ 別 sub-bundle 移管対象**。

**他既達主指標完全維持** (η-21 末):
- `Layout location qualifier` (link error) 0 → **0** ✓ 維持 (η-18 達成保持)
- `Anonymous member` (link) 0 → **0** ✓ 維持 (η-20 達成保持)
- `Cannot reuse block name` 0 / `'binding'` 0 (parse 段階) / `GBufferInfo redefinition struct` 0 / `'size' undeclared` 0 / `undeclared identifier` 0 = 全件維持
- `normalMap` redef 0 / `depthMap` redef 0 / `vary_fragcoord` redef 0 (η-19 達成保持)
- `shadow_bias` / `M_PI` / `weight4` redef 0 (η-20 達成保持)

**Phase 構成の特徴**: η-21 は **AYA judgment 受領後の 1 cycle 完走** で 3 Phase 同梱解消。Phase 1 (sampler binding) は materialF/pointLightF/softenLightF/spotLightF と統一の binding 番号確保で構造的に独立、cascade なし。Phase 2-C (MaterialUBO V/F alignment) は V/F UBO 同名 block の member set 完全一致原則を満たすため V 側に F 側 members を追加、優雅な構造解決。Phase 2-D (vary_position dead removal) は handoff §2 literal observation で発見した dead code を削除、最小編集で最大効果。**feedback_admit_unknown 範式の literal observation 優先** が Phase 2 root cause 特定 (handoff §5.1 で重複計上されていた "metallicFactor 4 + fragment block 4" が実は同一 4 件 / vary_position 3 件中 2 件は dead code) を可能にした。

---

## §2 完遂結果 metric (vs B?-η-20 末 baseline log)

| metric | η-20末 | η-21後 (verify) | Δ vs η-20 | 判定 |
|---|---|---|---|---|
| **glslang link failed for program** | **6** | **0** | **-6** | ✓ **link 段階 ZERO 到達 milestone** |
| **sampler binding (parse)** | **16** | **0** | **-16** | ✓ **Phase 1 A 主目標完全達成** |
| **MaterialUBO metallicFactor / fragment block member no V (link)** | **4** | **0** | **-4** | ✓ **Phase 2 C 主目標完全達成** |
| **Input 'vary_position' (link)** | **3** | **0** | **-3** | ✓ **Phase 2 D 主目標完全達成** |
| ERROR (total) | 142 | 154 | **+12** | 第19層 cascade exposure 露出 (削減 -23 + 増加 +35) |
| env_mat cannot redeclare (parse) | 0 | 16 | **+16** | **第19層 新出** (multiPointLightF Deferred MultiLight Shader 0-15) |
| SPIR-V requires location (parse) | 0 | 3 | **+3** | **第19層 新出** (bare in/out transformer 未捕捉) |
| non-opaque uniforms (parse) | 32 | 48 | **+16** | 第19層 cascade exposure (link 解消の副次) |
| missing #endif (parse、phantom) | 13 | 13 | ±0 | phantom 維持 (handoff §2.1 = 独立 fix 不可) |
| overlapping use of location (parse) | 5 | 5 | ±0 | η-22 副 scope 移管 |
| nameless block ... global scope (parse) | 5 | 5 | ±0 | η-22 副 scope 移管 |
| Anonymous member (link) | 0 | 0 | ±0 | ✓ η-20 達成維持 |
| shadow_bias redef (parse) | 0 | 0 | ±0 | ✓ η-20 達成維持 |
| M_PI redef (parse) | 0 | 0 | ±0 | ✓ η-20 達成維持 |
| weight4 redef (parse) | 0 | 0 | ±0 | ✓ η-20 達成維持 |
| normalMap / depthMap / vary_fragcoord redef | 0 | 0 | ±0 | ✓ η-19 達成維持 |
| Layout location qualifier (link error) | 0 | 0 | ±0 | ✓ η-18 達成維持 |
| Cannot reuse block name | 0 | 0 | ±0 | ✓ η-1 達成維持 |
| 'binding' (parse、η-8 達成系) | 0 | 0 | ±0 | ✓ η-8 達成維持 |
| GBufferInfo redefinition struct | 0 | 0 | ±0 | ✓ ζ 達成維持 |
| 'size' undeclared | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| undeclared identifier | 0 | 0 | ±0 | ✓ η-14 達成維持 |
| FATAL/SIGSEGV/Aborted | 0/0/0 | 0/0/0 | ±0 | ✓ |
| Goodbye | 1 | 1 | ±0 | ✓ clean shutdown |
| Vulkan device destroyed | 1 | 1 | ±0 | ✓ clean shutdown |

**23 件構造的完全解消 (Phase 1 A 16 + Phase 2 C 4 + Phase 2 D 3) + link 段階 ZERO 到達 milestone + 17 種既達主指標完全維持 + clean shutdown** = **η-21 Phase 1+2 主目標達成**。cascade shift forward 第19層露出 (env_mat redeclare +16 / SPIR-V requires location +3 / non-opaque +16) = **η-22 別 sub-bundle 移管対象**。

---

## §3 設計範式

### §3.1 新規 設計範式: bare opaque uniform Vulkan binding 付与範式 (Phase 1 A 主)

**範式根拠**: Vulkan strict mode (GLSL_KHR_vulkan_glsl) では sampler / texture / image (= opaque uniform) は `layout(binding=X)` 必須。bare `uniform sampler2D ident;` は `ERROR: 'binding' : sampler/texture/image requires layout(binding=X)` parse error。η-3 で UBO 内 binding を導入したが、opaque uniform の binding は別系統 (UBO `binding` と opaque `binding` で番号空間が同じ Vulkan set 内、ただし opaque は **set/binding が固有値、UBO の set/binding と重複可能なら避ける**)。

**範式構造**:

```glsl
#ifdef LL_VULKAN_GLSL
layout(set=N, binding=M) uniform sampler2D <ident>;
#else
uniform sampler2D <ident>;
#endif
```

**range 番号確保原則**: 既に viewer 内で同 ident が wrap されている他 file の binding 番号と統一する (例: `lightFunc` は materialF / pointLightF / softenLightF / spotLightF で `set=0, binding=5` 統一済、multiPointLightF も同 binding で適用)。

**範式有効性根拠**:
1. **Vulkan strict 適合**: opaque uniform に `layout(binding=X)` を付与 = SPIR-V binding 指定が成立、parse error 解消
2. **GL path 完全非変更**: `#else` で bare uniform 残置 = C++ binding 名 (`mReservedUniforms` 等) を変更不要
3. **C++ binding 名統一**: lightFunc の binding 番号は viewer 全体で同 channel = C++ `mTexture` index と整合
4. **multi-file 統一**: 既存 wrap (materialF L155 / pointLightF L35 / softenLightF L51 / spotLightF L37) と同番号を踏襲 = SPIR-V binding number assignment が conflict しない

**範式適用例 (multiPointLightF.glsl L34)**:

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-21 Phase 1: bare `uniform sampler2D
// lightFunc` (opaque) を Vulkan binding 付与 wrap。Vulkan strict mode では sampler/
// texture/image は layout(binding=X) 必須。materialF/pointLightF/softenLightF/spotLightF
// と同 binding (set=0, binding=5) で統一 (lightFunc は viewer 全体で同 channel)。
// 影響: Deferred MultiLight Shader 0-15 全 16 件の parse error 解消。
layout(set=0, binding=5) uniform sampler2D lightFunc;
#else
uniform sampler2D     lightFunc;
#endif
```

**範式の制約**:
- 同 ident が複数 file で異 binding 番号 wrap されている場合は **conflict 発生** → 統一原則を遵守
- set/binding 番号空間は Vulkan device 全体で grad、UBO binding (set=0, binding=0-4 等) と重複しないよう留意 (lightFunc = set=0, binding=5 は viewer 内既存)

### §3.2 新規 設計範式: V/F UBO member alignment 範式 (Phase 2 C 主)

**範式根拠**: glslang strict Vulkan mode では、V stage と F stage で **同名 UBO block** が宣言されている場合、両 stage の member set は **完全一致** が必須。member の subset でも `ERROR: Linking fragment stage: fragment block member has no corresponding member in vertex block: fragment stage: Block: <UBO>, Member: <missing_member> / vertex stage: Block: <UBO>, Member: n/a` link 失敗。

**典型事例** (η-21 で root cause 特定):
- V stage `MaterialUBO { texture_matrix0 ... _pad_emissive }` (6 members)
- F stage `MaterialUBO { texture_matrix0 ... _pad_emissive, metallicFactor, roughnessFactor, _pad_material0, _pad_material1 }` (10 members)
- → V 不在 4 members (metallicFactor 等) で link 失敗
- 影響: Skinned/通常/HUD PBR Opaque + HUD PBR Alpha = 計 4 件

**範式構造**:

```glsl
// V stage (pbropaqueV.glsl L63 等)、F stage と同 layout 揃え:
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=0, std140) uniform MaterialUBO {
    mat4  texture_matrix0;
    vec4  texture_base_color_transform[2];
    vec4  texture_emissive_transform[2];
    vec4  color;
    vec3  emissiveColor;
    float _pad_emissive;
    // ↓ V 側で追加 (F 側 pbropaqueF.glsl L44 と同 layout)
    float metallicFactor;
    float roughnessFactor;
    float _pad_material0;
    float _pad_material1;
};
#endif
```

**範式有効性根拠**:
1. **glslang link 段階の構造的合致**: V/F UBO block の member set 完全一致 = link error "fragment block member has no corresponding member in vertex block" 解消
2. **std140 alignment 維持**: 末尾 float×4 追加で `_pad_emissive` 以降の alignment 保持 (`emissiveColor` (vec3) + `_pad_emissive` (float) = 16B = std140 vec4 境界)
3. **shader 本文非変更**: V 側で metallicFactor 等を **使用しない** = GLSL optimizer が dead code 除去 (uniform optimization)、実 runtime cost ゼロ
4. **C++ binding 非変更**: `mReservedUniforms::metallicFactor` 等は GL path で bare uniform、Vulkan path で UBO offset、binding mechanism は両 path 同一

**範式適用例 (pbropaqueV.glsl L63 第 1 declaration / L216 第 2 declaration)** + **(pbralphaV.glsl L60 第 1 declaration / L240 第 2 declaration)**: V 側 MaterialUBO 末尾に F 側と同 4 members 追加、4 declarations × 4 members = 計 16 member 追加。

**範式の制約**:
- F 側 UBO declaration が **複数 file 跨ぎ** で異 layout を持つ場合は更に複雑化 (η-21 では pbropaqueF / pbralphaF は同 layout 確認済)
- pbropaqueV / pbralphaV それぞれ 2 declarations (HAS_SKIN / non-HAS_SKIN or HUD 等の分岐) があるため、**全 declarations に同 patch 適用必須**

### §3.3 新規 設計範式: dead vary_position declaration 削除範式 (Phase 2 D 主)

**範式根拠**: F stage で `in <type> <ident>;` を宣言したが main() で参照ゼロ = dead code。V 側で対応 out を宣言しない場合、glslang link 段階で `ERROR: Linking vertex and fragment stages: Input '<ident>' in fragment shader has no corresponding output in vertex shader.` link 失敗。dead code であれば **F 側 declaration を削除** が最小編集の解。V 側に out を追加するより GLSL optimizer 任せにせず明示的に dead code 除去 = 後続の transformer / link 段階で safe。

**典型事例** (η-21 で root cause 特定):
- pbrglowF.glsl L60-64 `vary_position` 宣言、main() L116-134 で参照ゼロ → dead
- pbropaqueF.glsl HUD path L315-319 `vary_position` 宣言、HUD main() L371-396 で参照ゼロ → dead
- 影響: Skinned PBR Glow / PBR Glow / HUD PBR Opaque = 計 3 件

**範式構造**:

```glsl
// 削除:
// #ifdef LL_VULKAN_GLSL
// layout(location=3) in vec3 vary_position;
// #else
// in vec3 vary_position;
// #endif
```

**範式有効性根拠**:
1. **link error 解消**: F 側 declaration を削除 = "Input 'vary_position' has no corresponding output" 発生せず
2. **shader 本文非変更**: main() で参照ゼロ = 削除しても機能 (frag_color 出力) 不変
3. **同 ident の異 path 残置 OK**: pbropaqueF L79 (非 HUD path) の vary_position は L216 mirrorClip で **使用**、V 側 pbropaqueV.glsl L159 に対応 out 存在 = 削除せず残置 (path 別判定で部分削除)

**範式の制約**:
- 削除前に main() body で参照ゼロを **literal trace 確認必須** (`grep -n` で全使用箇所抽出)
- 同 file 内で異 path (#ifdef IS_HUD 分岐等) で declaration が複数存在する場合、**dead path のみ削除、live path は残置**

### §3.4 prefer-cold-launch-verify 範式 (η-21 = 1 cycle 完走、3 Phase 同梱)

η-21 では:
1. AYA cold launch で η-20 末 baseline 421 件 transformed dump 出力済 (`/home/ishikawa/.ayastorm_x64/logs/AYAstorm.log` 起動 2026-06-02T??:??Z 直前)
2. **handoff §8.1 14 ステップ trace literal 実施**: ERROR 142 件 + WARNING (link failed) 6 件 = 計 148 系統を カテゴリ別 + program 別に系統整理
3. **literal observation で 2 点誤計上発見** (feedback_admit_unknown 範式遵守):
   - handoff §5.1/§8.2 候補 B (missing #endif 13 件) = phantom と判明 (先行 ERROR pair の副次効果、独立 scope 不可)
   - handoff §5.1 候補 C (MaterialUBO metallicFactor 4 件 + fragment block no V 4 件) = 同一 4 件と判明 (異表記の重複計上)
4. A/B/C/D 候補比較表作成、A + C + D 統合 scope を Claude 推奨案として AYA judgment 待ち提示
5. AYA judgment 「推奨でお願いします」受領後、3 Phase 同梱で 5 file edit 実施
6. shader-only fast-iterate 範式: `~/ayastorm/app_settings/shaders/` cp + `~/.ayastorm_x64/cache/shader_cache/` clear
7. AYA cold launch verify (第 1 巡) → 23 件全解消確認、link 段階 ZERO 到達、ERROR 計 142 → 154 (+12 cascade exposure 第19層) ✓ clean shutdown

η-20 のような dump 観察 iterate 2 巡と異なり、**η-21 では handoff §8.1 trace + AYA judgment 受領 + 1 cycle 完走** で達成。Phase 1 root cause が 1 file 1 wrap で済む構造単純さ、Phase 2 C/D が literal observation で重複/dead 判明し最小編集で済んだことが要因。

### §3.5 cascade shift forward 第19層露出範式 (η-21 で観測)

**範式根拠**: η-20 で link Anonymous member 36 件解消で第18層 sampler binding +16 / missing #endif +13 / fragment block +4 / vary_position +1 露出、η-21 で第18層 sampler binding 16 + MaterialUBO 4 + vary_position 3 解消で **第19層**: env_mat redeclare +16 / SPIR-V requires location +3 / non-opaque cascade +16 が露出。

**観測** (η-21 末):
- **env_mat cannot redeclare a user-block member array** 16 件: multiPointLightF.glsl L36 `uniform vec3 env_mat[3];` × `FrameViewProj` UBO 内 `mat3 env_mat;` の名前衝突。η-21 sampler binding fix で program が parse 段階 stage 通過した結果新出 (Deferred MultiLight Shader 0-15 で全件発生)。
- **SPIR-V requires location for user input/output** 3 件: bare in/out で transformer 未捕捉系。η-22 main scope 候補 (transformer Phase 3 拡張 or shader file level layout 付与)。
- **non-opaque uniforms outside a block** 32 → 48 (+16): link 解消した program が parse 段階 stage 通過した結果、新規 non-opaque uniform 露出。η-19 §3.1 範式継続適用候補。

**範式遵守 (η-22)**:
- 第19層は env_mat 16 件 (件数最大) + SPIR-V 3 件 + non-opaque +16 cascade 含む parse-stage cascade
- env_mat は multiPointLightF.glsl 1 file 1 fix で 16 件解消可能 = η-22 主 scope 候補 (η-1 §3.1 範式類 = UBO 内 member rename + #define alias 範式類)

---

## §4 patch 内容 (5 shader file)

### §4.1 Phase 1: sampler binding 16 件解消 (1 file 編集、bare opaque uniform Vulkan binding 付与範式)

#### §4.1.1 class3/deferred/multiPointLightF.glsl L34 (lightFunc Vulkan binding wrap)

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-21 Phase 1: bare `uniform sampler2D
// lightFunc` (opaque) を Vulkan binding 付与 wrap。Vulkan strict mode では sampler/
// texture/image は layout(binding=X) 必須。materialF/pointLightF/softenLightF/spotLightF
// と同 binding (set=0, binding=5) で統一 (lightFunc は viewer 全体で同 channel)。
// 影響: Deferred MultiLight Shader 0-15 全 16 件の parse error 解消。
layout(set=0, binding=5) uniform sampler2D lightFunc;
#else
uniform sampler2D     lightFunc;
#endif
```

### §4.2 Phase 2-C: MaterialUBO V/F member alignment (2 file × 2 declarations = 4 edits)

#### §4.2.1 class1/deferred/pbropaqueV.glsl L63 第 1 declaration (Skinned/通常 PBR Opaque V 用)

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-21 Phase 2-C: V/F MaterialUBO member alignment
// pbropaqueF.glsl L44 では metallicFactor/roughnessFactor/_pad_material0/_pad_material1
// 4 members 含む (V 不在で glslang link 失敗: fragment block member has no corresponding
// member in vertex block, Block: MaterialUBO, Member: metallicFactor)。V/F 跨ぎ block
// 同一 layout 必須のため V 側にも同 members 追加 (使用しない場合は V で参照ゼロ = optimize
// される、binding は std140 alignment 維持で float×4=16B 末尾追加)。
layout(set=1, binding=0, std140) uniform MaterialUBO {
    mat4  texture_matrix0;
    vec4  texture_base_color_transform[2];
    vec4  texture_emissive_transform[2];
    vec4  color;
    vec3  emissiveColor;
    float _pad_emissive;
    float metallicFactor;
    float roughnessFactor;
    float _pad_material0;
    float _pad_material1;
};
#else
uniform mat4 texture_matrix0;
uniform vec4[2] texture_base_color_transform;
#endif
```

#### §4.2.2 class1/deferred/pbropaqueV.glsl L216 第 2 declaration (HUD PBR Opaque V 用)

同 layout で `metallicFactor / roughnessFactor / _pad_material0 / _pad_material1` 4 members 追加。

#### §4.2.3 class1/deferred/pbralphaV.glsl L60 第 1 declaration (Skinned PBR Alpha V 用)

同上 patch (HUD PBR Alpha では実際は L240 第 2 declaration が effect する、Skinned 系は別 pipeline)。

#### §4.2.4 class1/deferred/pbralphaV.glsl L240 第 2 declaration (HUD PBR Alpha V 用)

同上 patch。

### §4.3 Phase 2-D: vary_position dead declaration 削除 (2 file 編集)

#### §4.3.1 class1/deferred/pbrglowF.glsl L60-64 削除 (Skinned/通常 PBR Glow 2 件)

```glsl
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-21 Phase 2-D: pbrglowF main() で vary_position
// が一度も参照されない (dead declaration)。V 側 (pbrglowV.glsl) でも宣言なし → glslang link
// 失敗 (Input 'vary_position' in fragment shader has no corresponding output in vertex shader)。
// 影響: Skinned PBR Glow / PBR Glow 2 件。dead 宣言を削除して整合させる。
```

(元の L60-64 削除、後続宣言が直結)

#### §4.3.2 class1/deferred/pbropaqueF.glsl HUD path L315-319 削除 (HUD PBR Opaque 1 件)

```glsl
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-21 Phase 2-D: HUD path main() で vary_position
// が一度も参照されない (dead declaration、L371-396 で未使用)。HUD path V (pbropaqueV.glsl
// L209-) は vary_position 不在 → glslang link 失敗 (Input 'vary_position' in fragment shader
// has no corresponding output in vertex shader)。影響: HUD PBR Opaque 1 件。dead 宣言削除。
// (非 HUD path L79 の vary_position 宣言は L216 mirrorClip(vary_position) で使用、残置)
```

(元の L315-319 削除、非 HUD path L79 は残置)

### §4.4 cache invalidate

shader file 変更のみ、C++ transformer は v5 据置 (η-18 末)。shader_cache clear で対応:
```bash
rm -rf ~/.ayastorm_x64/cache/shader_cache/*
```

---

## §5 cascade shift forward 第19層後始末計画 (η-22+ 移管)

### §5.1 parse 段階残主要カテゴリ + 推奨 sub-bundle 分割

| カテゴリ | 件数 | 推奨 sub-bundle | 根拠 |
|---|---|---|---|
| **env_mat cannot redeclare a user-block member array** | **16** | **η-22 主 scope 候補** | 件数最大、multiPointLightF.glsl 1 file 1 fix で 16 件解消可能。η-1 §3.1 範式類 (UBO 内 member rename + #define alias) で構造解決 |
| `non-opaque uniforms outside a block` | 48 | η-22 副 scope | η-19 §3.1 範式継続適用、多数 program 跨ぎ |
| `missing #endif` (phantom) | 13 | η-22 副次効果 | 独立 fix 不可、先行 ERROR fix で同時消滅 (handoff §2.1 で確定済) |
| `SPIR-V requires location for user input/output` | 3 | η-22 副 scope or 主 scope | bare in/out transformer 未捕捉、transformer Phase 3 拡張 or shader file level layout 付与 |
| `overlapping use of location` | 5 | η-22 副 scope | η-18 §3.1 範式類 (V↔F slot alignment) |
| `nameless block ... global scope` (FrameAtmosphere_Skybox) | 5 | η-22 副 scope | η-1 §3.1 範式類 (block 内 member rename) |

**合計 net 不重複 ERROR**: 約 90 件 (削減: 一部 multi-error program 重複カウント可能性)。残 64 件は "compilation errors. No code generated" 統括行。

### §5.2 link 段階 ZERO 達成後の milestone 観測

η-21 末で link 段階 ZERO 到達 = parse 段階のみが残課題。第19層は **parse-stage 単一段階** に絞られた状態 = η-22+ で parse-stage 集中攻略が可能。

### §5.3 transformer Phase 3 拡張は SPIR-V requires location 解消 と連動

- transformer version tag は η-18 末 `v5_p2_inout_pair_prepass_group_fix` 据置、η-22 で Phase 3 拡張時 `v6_p3_vertex_attribute` bump 必須
- SPIR-V requires location 3 件は bare in/out で transformer 未捕捉 = Phase 3 拡張対象候補

### §5.4 env_mat 名前衝突の構造調査 (η-22 主 scope 推定)

**第 1 候補 (推奨)**: multiPointLightF.glsl L36 `uniform vec3 env_mat[3];` を η-1 §3.1 範式類で wrap、UBO 内 member rename (例: `multilight_env_mat`) + `#define env_mat multilight_env_mat` alias + file 末尾 `#undef env_mat` (η-20 §3.1 PerDrawUBO member rename + #define alias + #undef scope-limit 範式と相似)。

**第 2 候補**: FrameViewProj UBO 内 `mat3 env_mat;` を別名 (`framevp_env_mat` 等) に rename。ただし FrameViewProj は viewer 全体で共有 UBO のため広範な影響、第 1 候補 推奨。

---

## §6 risks (η-20 §6 継承 + 新規)

| # | risk | 対応状態 | 引継 sub-bundle |
|---|---|---|---|
| 1 | C++ runtime transformer が GL path で influence する | ✓ kill-switch で完全 bypass、η-16 から継承 | 引継不要 |
| 2 | preprocessor 別分岐の bare/manual-wrap 共存 | ✓ η-18 で再利用優先範式に修正済 | 引継不要 |
| 3 | regex group index / kQuals non-capturing 不整合 | ✓ η-18 で修正済 | 引継不要 |
| 4 | shader file level guard が異 type 同名 varying program で衝突 | ✓ η-19 で確認済 | 引継不要 |
| 5 | PerDrawUBO member rename + #define alias の関数 param/local var 共存問題 | ✓ η-20 §4.1.2 で verify 済 | 引継不要 (η-22 で env_mat 同範式適用時は同 verify 手順踏襲) |
| 6 | #undef scope-limit が file 末尾で対応 = file 内全 use site が rewrite | ✓ η-20 で意図通り | 引継不要 |
| 7 | cascade shift forward 第19層大量露出 (env_mat +16 / SPIR-V +3 / non-opaque +16) | 想定通り | η-22 で順次解消 |
| 8 | sampler binding 番号 conflict (set=0, binding=5 統一原則) | ✓ multiPointLightF が materialF/pointLightF/softenLightF/spotLightF と統一、η-21 verify で確認済 | η-22+ で新規 sampler wrap 時は既存 binding 統一原則継承 |
| 9 | V/F MaterialUBO member alignment が複数 file 跨ぎ (HAS_SKIN / HUD 分岐) で異 layout を持つ可能性 | ✓ η-21 で pbropaqueV/F + pbralphaV/F の 4 declarations 全件同 layout 確認、verify 済 | η-22+ で他 UBO (FrameViewProj 等) の V/F alignment 必要時は同範式継承 |
| 10 | dead vary_position 削除で同 ident 異 path 残置 (pbropaqueF L79 vs L315) | ✓ η-21 で literal trace 確認、L216 mirrorClip(vary_position) 使用箇所残置 | 引継不要 (将来同類 dead 検出時は literal trace 確認必須) |
| 11 | upstream OpenGL Firestorm merge 時 shader file 互換 (sampler binding wrap / V/F UBO alignment / dead removal 範式は標準 GLSL) | 互換性高 | sub-step 4.5 |
| 12 | cinematic_bd overlay mirror | η-21 では touch なし、cinematic_bd directory 内 5 file 不在を Glob で確認済 | 引継不要 |
| 13 | env_mat 名前衝突 = multiPointLightF L36 × FrameViewProj UBO `mat3 env_mat` | 第 1 候補 (multiPointLightF wrap) 推奨 | η-22 主 scope 候補 |
| 14 | SPIR-V requires location 3 件 (bare in/out transformer 未捕捉) | transformer Phase 3 拡張候補 | η-22+ 主 or 副 scope |
| 15 | Phase 1+2 同梱 1 cycle 完走 = AYA cold launch verify 第 2 巡不要、η-21 milestone link 段階 ZERO 到達 | feedback_one_step_at_a_time 範式遵守 | 引継不要 |
| 16 | handoff §5.1/§8.2 の 2 点誤計上 (B = phantom / C 重複) を Claude trace で発見 | literal observation 範式遵守、AYA に正直 report 済 | η-22+ で同類 trace 継続 |

---

## §7 observability

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 件数 (η-7 305 baseline、η-11 310、η-20 末 421 件 transformed dump + 342 件 cache 合計) | η-21 末 verify 後 421 件超 transformed dump 維持、cold launch 後 cache 合計 計測継続 | B?-η-22 で baseline 更新 |
| 2 | C++ runtime location emit 範式 (η-16 §3.1) Phase 2 (V↔F pair) 維持 | ✓ η-21 で touch なし、v5 据置 | B?-η-22 で SPIR-V requires location 対応時 Phase 3 拡張候補 |
| 3 | shader file level include guard 範式 (η-19 §3.1) 適用範囲 | η-21 では touch なし | B?-η-22 で non-opaque 48 件 cascade 適用候補 |
| 4 | PerDrawUBO member rename + #define alias + #undef 範式 (η-20 §3.1) | η-21 では touch なし | B?-η-22 で env_mat 同範式適用候補 |
| 5 | bare opaque uniform Vulkan binding 付与範式 (η-21 §3.1 新規) | η-21 で multiPointLightF.glsl lightFunc 1 件適用 | B?-η-22 で他 bare sampler / texture / image 残時に同範式継承 |
| 6 | V/F UBO member alignment 範式 (η-21 §3.2 新規) | η-21 で pbropaqueV/F + pbralphaV/F = 4 declarations 適用 | B?-η-22 で他 UBO (FrameViewProj 等) V/F alignment 必要時に同範式継承 |
| 7 | dead vary_position declaration 削除範式 (η-21 §3.3 新規) | η-21 で pbrglowF + pbropaqueF HUD path 2 file 適用 | B?-η-22 で他 dead in 検出時に同範式継承 (literal trace 確認必須) |
| 8 | F stage 既存 layout in 自動 override 12 件継続 (LL_DEBUGS) | 観測継続 | B?-η-22 で残発見時継続観測 |
| 9 | dump 機構 (η-16 §3.1 内包) η-21 で 421 件超出力 | 維持 | B?-η-22 で必要に応じて再投入 |
| 10 | falsification iterate 範式 (η-18 内 v2→v5 4 段) | η-21 は AYA judgment 受領後 1 cycle 完走、falsification iterate 不要 | B?-η-22 で C++ transformer Phase 3 拡張時は falsification iterate 想定 |
| 11 | 既達主指標完全維持 (η-21 で 17 種維持 + 23 件構造的解消、退行 0 件) | ✓ η-21 で退行 0 件達成 | B?-η-22 で第19層 cascade 残系統 scope |
| 12 | cinematic_bd directory 全 .glsl audit (η-17 §10.4 継承) | η-21 で touch なし、η-20 末 audit 維持 | B?-η-22+ 着手前 or 完遂後の別 phase として継続推奨 |
| 13 | feedback_self_verify_before_handoff 適用 | η-21 で trace + literal observation 14 ステップ + AYA cold launch 1 巡で 23 件全解消確認 | B?-η-22 でも AYA cold launch 依頼前に必ず実施 |
| 14 | feedback_admit_unknown 適用 (推論 2 連続外したら literal observation 切替) | η-21 で handoff §5.1/§8.2 の 2 点誤計上 (B = phantom / C 重複) を literal trace で発見、AYA に正直 report | B?-η-22 でも継承 |
| 15 | feedback_no_scope_shrink 適用 | AYA「推奨でお願いします」literal 受領 → A + C + D 統合 scope 全実装、scope shrink 禁止遵守 | B?-η-22 でも継承 |
| 16 | feedback_remove_verification_logs 適用 | η-21 で追加 LL_INFOS hook 無し、不要 | B?-η-22 で追加 LL_INFOS hook あれば commit 前に DEBUG/削除 判断 |
| 17 | feedback_one_step_at_a_time 適用 | η-21 で 1 step 1 verify 範式遵守 (trace → 推奨提示 → AYA judgment → patch → cache clear → verify → commit) | B?-η-22 でも継承 |
| 18 | link 段階 ZERO 到達 milestone (η-1〜η-20 全周回で初) | ✓ η-21 達成 | B?-η-22 では parse 段階集中攻略、link 段階 ZERO 維持を引き続き観測 |

---

## §8 引継 scope 推奨 (B?-η-22)

### §8.1 着手前 trace (14 ステップ ベースで η-22 適応)

1. AYA 「コマンド + ビルド全権」+「verify は 1 ステップずつ」運用継承確認
2. η-21 末 baseline log (`/home/ishikawa/.ayastorm_x64/logs/AYAstorm.log` 起動 2026-06-02T10:24Z) を canonical baseline として extract
3. ERROR 154 件を **カテゴリ別 + program 別** に系統整理
4. **主 scope 候補 A**: parse failed `env_mat cannot redeclare a user-block member array` 16 件解消 (件数最大、multiPointLightF.glsl 1 file 1 fix、η-20 §3.1 PerDrawUBO member rename + #define alias + #undef scope-limit 範式類で構造解決)
5. **主 scope 候補 B**: parse failed `non-opaque uniforms outside a block` 48 件継続解消 (η-19 §3.1 guard 範式継続適用、multi-file)
6. **副 scope 候補 C**: parse failed `SPIR-V requires location for user input/output` 3 件 (bare in/out transformer 未捕捉系、transformer Phase 3 拡張 or shader file level layout 付与)
7. **副 scope 候補 D**: parse failed `overlapping use of location` 5 件 + phantom #endif 5 件 副次解消 (η-18 §3.1 範式類 = V↔F slot alignment)
8. **副 scope 候補 E**: parse failed `nameless block FrameAtmosphere_Skybox` 5 件 + phantom #endif 5 件 副次解消 (η-1 §3.1 範式類 = block 内 member rename)
9. **A/B/C/D/E どれを Phase 1 主 scope に置くか + 副 scope (η-19 §3.1 guard 範式同適用) を η-22 内 Phase 2 として同梱するか別 sub-bundle に分離するか** = AYA judgment 候補
10. transformer Phase 3 拡張 (V stage bare `in`) は **C 候補対応時に η-22 で対象か / η-23+ 移管か** を A/B/C/D/E 主 scope 選択と連動判定
11. transformer version tag は η-18 末 `v5_p2_inout_pair_prepass_group_fix` 据置、η-22 で Phase 3 拡張時 `v6_p3_vertex_attribute` bump 必須
12. AYA log restore 設定: η-17 末で `RenderVulkanShaderDumpTransformed=1` 戻し対象案内済、AYA 環境 settings.xml で 0 確認 (η-21 verify では dump 取得済、η-22 で必要なら継続 inject)
13. cinematic_bd directory audit (η-17 §10.4 継承) を η-22 着手前 or 完遂後の別 phase として継続推奨
14. **feedback_admit_unknown / feedback_one_step_at_a_time / feedback_no_scope_shrink 範式継承** で η-22 内でも literal observation 優先 + AYA judgment 受領後の scope 全実装維持

### §8.2 想定 Phase 構成 (η-22)

η-22 着手前 trace + AYA judgment で確定だが、現時点 推定:
- **Phase 1 (主 scope 候補 A 推奨)**: env_mat 16 件解消 (件数最大、multiPointLightF.glsl 1 file 1 fix、η-20 §3.1 PerDrawUBO member rename + #define alias + #undef scope-limit 範式類)
- **Phase 2 (副 scope)**: non-opaque uniforms 48 件 cascade 解消 (η-19 §3.1 guard 範式継続)、SPIR-V requires location 3 件 (Phase 3 拡張 or manual layout 付与)
- **Phase 3 (option D/E 同梱時)**: overlapping location 5 件 + nameless block 5 件 + 副次 phantom #endif 10 件

### §8.3 B?-η-22 完遂後の想定 cascade exposure 第20層

- env_mat 16 件解消想定 = parse 段階大量解消、第20層 emergence 観測点: pipeline cache / runtime binding / SPIR-V layout validation 等
- non-opaque 48 件解消想定 = parse 段階更に大量解消
- SPIR-V requires location 3 件解消想定 = bare in/out 全 transformer 対応
- 残 overlapping 5 / nameless 5 / phantom #endif 10 = η-23+ 移管想定
- shader_cache 件数: η-22 で計測継続 + 第20層 emergence 観測

---

## §9 commit message (記録)

patch commit `e78ca6006c`:
```
feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-21 完遂

main scope = Phase 1 (A) sampler binding 16 件 + Phase 2 (C+D) link 6
program 完全解消。η-19 §3.1 shader file level guard 範式 (multi-file
redef) + η-20 §3.1 PerDrawUBO member rename + #define alias + #undef
scope-limit 範式の延長で、link 段階を 6 → 0 milestone 到達。

Phase 1 (A): sampler binding 16 → 0
- class3/deferred/multiPointLightF.glsl L34 bare `uniform sampler2D
  lightFunc` を Vulkan binding (set=0, binding=5) wrap。
  materialF/pointLightF/softenLightF/spotLightF と統一。
  影響: Deferred MultiLight Shader 0-15 全 16 件の parse error 解消。

Phase 2 (C): MaterialUBO V/F member alignment 4 → 0
- class1/deferred/pbropaqueV.glsl L63 + L216 (2 declarations)
- class1/deferred/pbralphaV.glsl L60 + L240 (2 declarations)
  V 側 MaterialUBO に metallicFactor/roughnessFactor/_pad_material0/
  _pad_material1 を追加し F 側 (pbropaqueF L44 / pbralphaF L31, L351)
  と layout 整合。

Phase 2 (D): vary_position dead declaration 削除 3 → 0
- class1/deferred/pbrglowF.glsl L60-64
- class1/deferred/pbropaqueF.glsl HUD path L315-319

主指標達成:
- glslang link failed for program 6 → 0 (link 段階 ZERO 到達 milestone)
- sampler binding parse 16 → 0
- metallicFactor / fragment block member no V (link) 4 → 0
- Input 'vary_position' (link) 3 → 0
- ERROR 計 142 → 154 (+12 cascade exposure 第19層)
- clean shutdown 維持
- 13 種既達主指標完全維持
```

handoff doc commit message 案:
```
docs(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-21-complete handoff 起草

main scope = Phase 1 (A) sampler binding 16 件 + Phase 2 (C+D) link 6
program 完全解消、計 23 件構造的完全解消 + link 段階 ZERO 到達
milestone の完遂 handoff doc。新規 §3.1 bare opaque uniform Vulkan
binding 付与範式 + §3.2 V/F UBO member alignment 範式 + §3.3 dead
vary_position declaration 削除範式 を策定。

主指標達成:
- glslang link failed for program 6 → 0 (link 段階 ZERO 到達 milestone)
- sampler binding 16 → 0 (Phase 1 A 完全達成)
- metallicFactor / fragment block 4 → 0 (Phase 2 C 完全達成)
- vary_position (link) 3 → 0 (Phase 2 D 完全達成)
- ERROR 計 142 → 154 (+12 第19層 cascade exposure)
- clean shutdown
- 17 種既達主指標完全維持

cascade shift forward 第19層露出:
- env_mat cannot redeclare 16 件新出 (η-22 主 scope 候補)
- SPIR-V requires location 3 件新出
- non-opaque uniforms +16 cascade

patch commit: e78ca6006c
```

---

**handoff doc 完。次 sub-bundle B?-η-22 着手は本 doc §8 推奨 scope (parse env_mat 16 件 + non-opaque 48 件 + SPIR-V requires location 3 件 + overlapping/nameless block 5+5 件) を起点として、fresh context で実施。**
