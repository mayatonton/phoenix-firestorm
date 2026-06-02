# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-24 完遂 handoff

**status**: B?-η-24 完遂 (Phase C location reassign = `overlapping use of location` 5 → **0** **完封**、Phase D FrameAtmosphere_Skybox UBO 削除 + PerProgramUBO_GammaCorrect 新規 = `nameless block ... global scope` 5 → **0** **完封**、ERROR 計 104 → 91 → **90** (η-23 比 -14)、`glslang link failed for program` **0 → 0** = **link 段階 ZERO 3 sub-bundle 連続維持** (η-21 milestone)、clean shutdown 維持) → 次 sub-bundle B?-η-25 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: `2e642ce685` (5 shader file 編集、+41/-54 = -13 行、C++ 変更 0、user_settings.xml 変更 0)
**handoff doc commit**: 本 doc (η-23-complete 範式継承、別 commit)
**勤続範式継承**: B?-η-23-complete `dba5c14820` (patch `9cc143d2a6`) / B?-η-22-complete `48ceb1c953` (patch `5cc4b03bab`) / B?-η-21-complete `989b529936` (patch `e78ca6006c`) / B?-η-20-complete `d7c722f4e7` (patch `557cd1db00`) / B?-η-19-complete `4911401568` (patch `9349ace5f0`) / B?-η-18-complete `1371da9660` (patch `4b42779cc7`) / B?-η-17-complete `cc2e878c7e` (patch `e2d4d3bbca`) / B?-η-16-complete `3653efed00` (patch `72fd4f3c3c`) / B?-η-15-complete `cb28cf1daa` (patch `6a11eabc73`) / B?-η-14-complete `78df235243` (patch `c971838656`) / B?-η-13-reverted (no commit) / B?-η-12-complete `7c1762d214` / B?-η-11-complete `74705c35bb` / B?-η-10-complete `9246142639` / B?-η-9-complete `6fee4a818b` / B?-η-8-complete `6994eba271` / B?-η-7-complete `b1e8689634` / B?-η-6-complete `fe757ea624` / B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-24 scope = **Phase C (副 scope C 5 件 location reassign) + Phase D (副 scope D 5 件 nameless block 解消)** の同梱完遂。AYA judgment 第1巡「推奨案で」literal 受領 (案 D: C+D 同梱 11 件 / 2 root cause) → 第2巡「X」literal 受領 (case X: pbrterrainV location=21 cascade 補完) で 2 巡完走。**2 主 sub-scope を完全カテゴリ完封**して η-23 §3.3 / η-18 §3.1 / η-14 Path G 範式の有効性を実証。

**Phase C (location reassign、η-18 §3.1 範式類継承)**:
- **alphaV.glsl / alphaF.glsl** (V↔F pair、4 program: Deferred Alpha / Skinned Deferred Alpha / HUD Alpha / Deferred Avatar Alpha):
  - `vary_norm` location 20 → **51**
  - 根拠: atmosphericsVarsV/F.glsl `vary_AdditiveColor` at location=20 と overlap、η-18 §3.1 範式類で V↔F pair 同時 reassign
- **pbrterrainV.glsl** (2 program: PBR Terrain heightmap-with-noise / paintmap triplanar):
  - `vary_vertex_normal` location 20 → **50** (Phase C 主)
    - 根拠: pbrterrainUtilF.glsl L59 既存 `layout(location=50) in vec3 vary_vertex_normal` と V↔F 整合
  - `vary_tangents` location 21 → **52** (case X = Phase C 第2巡 cascade 補完)
    - 根拠: atmosphericsVarsV.glsl `vary_AtmosAttenuation` at location=21 と overlap、vec3[4] = slots 52-55、η-18 §3.1 範式類

**Phase D (FrameAtmosphere_Skybox 削除 + PerProgramUBO_GammaCorrect 新規、η-14 Path G 範式継承)**:
- **postDeferredGammaCorrect.glsl / postDeferredTonemap.glsl** (5 program: Deferred Tonemap × 3 / No Post Tonemap × 2):
  - FrameAtmosphere_Skybox UBO (member 17 件: sunlight_color 等) を **全削除**
  - 根拠: 同 set/binding (set=0, binding=2) で declare 済の FrameAtmosphere_Lighting UBO と nameless block の特例による member 名 17 件 global scope 重複 = `nameless block ... global scope` parse error
  - 各 file の main() で参照する Skybox 専有 member は `gamma` のみ (postDeferredTonemap は `#ifdef GAMMA_CORRECT` gated)
  - 新規 **PerProgramUBO_GammaCorrect** (set=2, binding=2、std140) を `gamma` 単独 member で declare = η-3 §3.2 PerDrawUBO per-program 派生範式類、`PER_PROGRAM_UBO_GAMMA_CORRECT_DEFINED` guard で重複防止

**PerProgramUBO_GammaCorrect std140 構造**:
```glsl
layout(set=2, binding=2, std140) uniform PerProgramUBO_GammaCorrect {
    float gamma;
    float _pad_gc0;
    float _pad_gc1;
    float _pad_gc2;     // 末尾 float×3 pad で 16-byte chunk 整合
};
```

**計**: **shader 5 file 編集 (+41/-54 = -13 行)、C++ 変更 0、user_settings.xml 変更 0**

**主指標達成** (vs η-23 末 baseline log):

- `overlapping use of location` (parse) 5 → **0** ✓ **完全解消 (Phase C 主目標達成)**
- `nameless block ... global scope` (parse) 5 → **0** ✓ **完全解消 (Phase D 主目標達成)**
- ERROR 計 104 → **90** **-14 (= -5 overlap loc + -5 nameless block + -10 missing #endif phantom cascade + 一部 program 救済 - 8 non-opaque 第22層 cascade exposure)**
- `glslang link failed for program` 0 → **0** ✓ **link 段階 ZERO 3 sub-bundle 連続維持 (η-21 milestone 継承)**
- `Goodbye!` 1 / `Vulkan device destroyed` 1 / FATAL/SIGSEGV/Aborted 0/0/0 = ✓ clean shutdown 維持

**cascade shift forward 第22層**: non-opaque uniforms 35 → 43 (**+8**) = Phase C 解消で alphaV/F pair の 4 program (Deferred Alpha 系) が新規 parse 通過 → 内包する bare uniform 等の第22層 cascade 露出。Phase C cascade 第22層第1類 (pbrterrainV terrain_texture_transforms 等) の同 file 内露出を含む。

| # | metric | η-23末 | η-24後 | Δ | 解析 |
|---|---|---|---|---|---|
| 1 | `non-opaque uniforms outside a block` (parse) | 35 | **43** | **+8** | 第22層 cascade 露出 (Phase C 救済 4 program + pbrterrainV 同 file 内) |
| 2 | `'nameless block ... global scope'` (parse) | 5 | **0** | **-5** | ✓ **Phase D 完封** |
| 3 | `'shadow_clip' : redefinition` (parse) | 1 | **1** | ±0 | (η-25 移管、Haze/Godrays 系) |
| 4 | `overlapping use of location` (parse) | 5 | **0** | **-5** | ✓ **Phase C 完封** |
| 5 | `missing #endif` (parse、phantom) | 13 | **3** | **-10** | ✓ 先行 fix cascade で phantom 大幅減少 |
| 6 | `compilation errors. No code generated` (summary) | 45 | **43** | -2 | parse 失敗 program 数 |
| 7 | `glslang link failed for program` | 0 | **0** | ±0 | ✓ link 段階 ZERO 3 sub-bundle 連続維持 |
| 8 | Anonymous member (link) | 0 | **0** | ±0 | ✓ η-20 達成維持 |
| 9 | shadow_bias / M_PI / weight4 redef (parse) | 0 | **0** | ±0 | ✓ η-20 達成維持 |
| 10 | Input `vary_position` (link) | 0 | **0** | ±0 | ✓ η-21 達成維持 |
| 11 | MaterialUBO metallicFactor / fragment block (link) | 0 | **0** | ±0 | ✓ η-21 達成維持 |
| 12 | sampler binding (parse) | 0 | **0** | ±0 | ✓ η-21 達成維持 |
| 13 | env_mat cannot redeclare (parse) | 0 | **0** | ±0 | ✓ η-22 達成維持 |
| 14 | SPIR-V requires location (parse) | 0 | **0** | ±0 | ✓ η-22 達成維持 |
| 15 | Multi-Light non-opaque (parse) | 0 | **0** | ±0 | ✓ η-23 達成維持 |

**主指標達成総括**: **副 scope 2 種カテゴリ完封 (overlapping loc / nameless block)** + **link 段階 ZERO を η-21 milestone から 3 sub-bundle 連続維持** + **phantom #endif 13 → 3 大幅減 (-10、先行 fix cascade)** + **clean shutdown 維持**。第22層 cascade exposure (+8 non-opaque) は Phase C 救済 4 program 内包の bare uniform 露呈 = 設計範式の正常な動作 (η-25 主軸へ送る)。

**他既達主指標完全維持** (η-24 末):
- η-23 達成全項目 (Multi-Light non-opaque 0)
- η-22 達成全項目 (env_mat redeclare 0 / SPIR-V requires location 0)
- η-21 達成全項目 (link failed 0 / sampler binding 0 / metallicFactor 0 / vary_position 0)
- η-20 達成全項目 (Anonymous member 0 / shadow_bias / M_PI / weight4 redef 0)
- η-19 達成全項目 (normalMap / depthMap / vary_fragcoord redef 0)
- η-18 達成全項目 (Layout location qualifier 0)
- η-1〜η-17 達成全項目 (Cannot reuse block name 0 / 'binding' 0 / GBufferInfo redef 0 / 'size' undeclared 0 / undeclared identifier 0)

**Phase 構成の特徴**: η-24 は **AYA judgment 2 段階受領 (第1巡 案 D の C+D 同梱 → 第2巡 case X の pbrterrainV vary_tangents cascade 補完)** で完走。**feedback_one_step_at_a_time 範式遵守** (1 巡毎に verify、cascade 露出後の判断を AYA に渡す)、**feedback_no_scope_shrink 範式遵守** (AYA「すべて」literal 受領後、副 scope C/D を期目で完封まで実装)。

---

## §2 完遂結果 metric (vs B?-η-23 末 baseline log)

| metric | η-23末 | η-24後 (verify) | Δ vs η-23 | 判定 |
|---|---|---|---|---|
| **overlapping use of location (parse)** | **5** | **0** | **-5** | ✓ **Phase C 主目標完全達成** |
| **nameless block ... global scope (parse)** | **5** | **0** | **-5** | ✓ **Phase D 主目標完全達成** |
| glslang link failed for program | 0 | 0 | ±0 | ✓ **link 段階 ZERO 3 sub-bundle 連続維持** |
| ERROR (total) | 104 | 90 | **-14** | overlap loc -5 + nameless -5 + missing #endif -10 + 一部救済 - non-opaque +8 (第22層) |
| non-opaque uniforms (parse、total) | 35 | 43 | **+8** | 第22層 cascade exposure (Phase C 救済 4 program 内包 bare uniform 露呈 + pbrterrainV 同 file 内) |
| compilation errors. No code generated (summary) | 45 | 43 | -2 | parse 失敗 program 数 |
| missing #endif (parse、phantom) | 13 | 3 | **-10** | ✓ 先行 fix cascade で phantom 大幅減少 |
| shadow_clip redef (parse) | 1 | 1 | ±0 | (η-25 移管) |
| env_mat redeclare (parse) | 0 | 0 | ±0 | ✓ η-22 達成維持 |
| SPIR-V requires location (parse) | 0 | 0 | ±0 | ✓ η-22 達成維持 |
| Multi-Light non-opaque (parse) | 0 | 0 | ±0 | ✓ η-23 達成維持 |
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

**副 scope 2 種カテゴリ完封 (overlapping loc 5 → 0 + nameless block 5 → 0) + link 段階 ZERO 3 sub-bundle 連続維持 + 18 種既達主指標完全維持 + clean shutdown** = **η-24 主目標達成**。

---

## §3 設計範式

### §3.1 適用範式: η-18 §3.1 location reassign 範式継承 (Phase C 5 件解消)

**範式根拠**: Vulkan strict mode (GLSL_KHR_vulkan_glsl) では V↔F pair の `out`/`in` varying が同 location を占有しているとき、別 V/F pair (atmosphericsVarsV/F.glsl の `vary_AdditiveColor` 等) と location が overlap すれば parse error `overlapping use of location`。η-18 §3.1 範式類で確立した location reassignment を、η-24 では未使用の 50-59 帯に集約。

**η-24 適用位置 map**:

| location | varying | 出現 V file | 出現 F file | 適用 sub-step |
|---|---|---|---|---|
| 20 | `vary_AdditiveColor` (atmospherics 系) | atmosphericsVarsV.glsl | atmosphericsVarsF.glsl | (既存、不変) |
| 21 | `vary_AtmosAttenuation` (atmospherics 系) | atmosphericsVarsV.glsl | atmosphericsVarsF.glsl | (既存、不変) |
| **50** | `vary_vertex_normal` (pbrterrain 系) | pbrterrainV.glsl (旧20) | pbrterrainUtilF.glsl (既存) | **η-24 Phase C** |
| **51** | `vary_norm` (alpha 系) | alphaV.glsl (旧20) | alphaF.glsl (旧20) | **η-24 Phase C** |
| **52-55** | `vary_tangents[4]` (pbrterrain 系) | pbrterrainV.glsl (旧21) | pbrterrainF.glsl (旧20、Δ V↔F mismatch、η-25 移管) | **η-24 case X (Phase C 第2巡)** |

**有効性根拠**:
1. **slot alignment 整合**: vec3 = 1 slot、vec3[4] = 4 連続 slot (52-55)、location qualifier 単独宣言で SPIR-V emit 通過
2. **GL path 完全非変更**: `#else` で legacy `out`/`in` 残置 = GL の自動 location assign 動作変更なし
3. **C++ binding 影響なし**: location は SPIR-V interface variable 用、CPU side glAttribLocation 等の binding に影響しない

### §3.2 適用範式: η-14 Path G 範式継承 (Phase D 5 件解消、UBO 削除 + per-program UBO 新規)

**範式根拠**: η-14 で確立した **Path G = 既存 UBO 削除 + 必要 member のみを別 UBO に再構成**。η-24 では postDeferredGammaCorrect/postDeferredTonemap が必要とする `gamma` 単独 member のみを新規 `PerProgramUBO_GammaCorrect` (set=2, binding=2) で declare。FrameAtmosphere_Skybox UBO 全体は当該 file から削除して FrameAtmosphere_Lighting (set=0, binding=2) との member 名 17 件 global scope 重複を構造解決。

**η-24 適用 (postDeferredGammaCorrect.glsl / postDeferredTonemap.glsl)**:
```glsl
#ifdef LL_VULKAN_GLSL
#ifndef PER_PROGRAM_UBO_GAMMA_CORRECT_DEFINED
#define PER_PROGRAM_UBO_GAMMA_CORRECT_DEFINED 1
layout(set=2, binding=2, std140) uniform PerProgramUBO_GammaCorrect {
    float gamma;
    float _pad_gc0;
    float _pad_gc1;
    float _pad_gc2;
};
#endif
#else
uniform float gamma;
#endif
```

(postDeferredTonemap.glsl では `#ifdef GAMMA_CORRECT` で gated、既存 path 構造保持)

**有効性根拠**:
1. **nameless block name collision 根絶**: FrameAtmosphere_Skybox UBO 削除 = FrameAtmosphere_Lighting member との global scope 重複なし
2. **必要 member 提供**: 各 file の main() / legacyGamma() 内で `gamma` 単独参照 = PerProgramUBO_GammaCorrect member で代替
3. **per-program binding 分離**: η-23 PerDrawUBO_MultiLight (set=2, binding=1) と PerProgramUBO_GammaCorrect (set=2, binding=2) で衝突なし、η-3 PerDrawUBO_LightParams (set=2, binding=0) を含めた set=2 namespace 連番継続
4. **GL path 完全非変更**: `#else` で bare `uniform float gamma` 維持、C++ side `glUniform1f("gamma")` binding 変更不要
5. **std140 alignment 整合**: float×4 = 16-byte chunk 単独で C++ side 直書き互換

### §3.3 適用範式: η-23 §3.3 FrameAtmosphere_Lighting per-shader declare 統一範式 (Phase D 副次効果)

**範式根拠**: η-23 §3.3 で確立した「FrameAtmosphere_Lighting UBO を必要 shader file 個別 declare、`FRAME_ATMOSPHERE_LIGHTING_DEFINED` guard で重複防止」を η-24 でも実証。postDeferredGammaCorrect/postDeferredTonemap 自身は FrameAtmosphere_Lighting 直接参照しないが、atmosphericsHelpersV/atmosphericsFuncs から include される FrameAtmosphere_Lighting member が global scope に出ているため、当該 file の FrameAtmosphere_Skybox UBO member と名前衝突。Path G で Skybox 側を削除することで Lighting 側 (より広範な使用、include chain 上流) を保護。

### §3.4 cascade shift forward 第22層 (η-24 で観測)

**範式根拠**: η-24 Phase C で alphaV/F vary_norm location reassign → V↔F pair 4 program (Deferred Alpha / Skinned Deferred Alpha / HUD Alpha / Deferred Avatar Alpha) が overlap loc parse 通過 → 内包する bare uniform 等の第22層 cascade 露出 (non-opaque +8)。Phase D は postDeferredGammaCorrect 救済 (parse 通過、cascade 露出最小) + postDeferredTonemap 解消 (gamma member 提供で残 program の bare uniform は別系統、即時露出)。

**観測** (η-24 第1巡 → 第2巡):
- 第1巡: alphaV/F + pbrterrainV vary_vertex_normal + postDeferredGammaCorrect + postDeferredTonemap = 4 file (V↔F pair 2 + post 2) → ERROR 104 → 91 (-13)
  - overlapping loc 5 → 1 (pbrterrainV vary_tangents loc=21 残)
  - nameless block 5 → 0 (Phase D 完封)
  - missing #endif 13 → 3 (-10 phantom cascade)
  - non-opaque 35 → 42 (+7 第22層 cascade exposure)
- 第2巡 (case X): pbrterrainV vary_tangents 21 → 52 → ERROR 91 → 90 (-1)
  - overlapping loc 1 → 0 (Phase C 完封)
  - non-opaque 42 → 43 (+1 同 file 内 terrain_texture_transforms 露出)

**範式遵守 (η-25)**:
- non-opaque 43 件 (Phase C 救済 4 program + Phase C case X 露出 pbrterrainV terrain_texture_transforms 等) を η-25 主軸候補に
- pbrterrainF.glsl V↔F mismatch (vary_tangents loc=20 vs V側 52 / vary_signs loc=24 vs V側 25) は Phase C 範式類継続候補 = pbrterrainF 側 V↔F 整合済 reassign
- shadow_clip redef 1 件は η-25 副 scope 候補
- missing #endif 3 件は先行 fix で連動消滅予想

### §3.5 prefer-cold-launch-verify 範式 (η-24 = 1 cycle 2 巡完走、AYA judgment 2 段階受領)

η-24 では:
1. AYA cold launch で η-23 末 baseline 取得済 (`/home/ishikawa/.ayastorm_x64/logs/AYAstorm.log` 起動 2026-06-02T11:30Z 直前 baseline)
2. **handoff §8.1 14 ステップ trace literal 実施** (ERROR 104 件を category × program で系統整理)
3. **AYA に scope 候補 A/B/C/D/E 提示**: 案 D (C+D 同梱 = 5+5 = 11 件 / 2 root cause) 推奨
4. **AYA judgment 第1巡「推奨案で」literal 受領** → Phase C 4 file + Phase D 2 file = 5 file deploy → cold launch verify
5. **第1巡 verify**: overlap loc 5 → 1 (pbrterrainV vary_tangents loc=21 cascade 残)、nameless block 5 → 0 ✅、ERROR 104 → 91
6. **AYA に case X 提示** (pbrterrainV location=21 fix のみ追加 = C scope 第2巡)
7. **AYA judgment 第2巡「X」literal 受領** → pbrterrainV.glsl 1 file edit (vary_tangents 21 → 52) deploy → cold launch verify
8. **第2巡 verify**: overlap loc 1 → 0 ✅、ERROR 91 → 90、Phase C/D 主目標完全達成確認
9. patch commit + handoff doc 起草

**feedback_one_step_at_a_time 範式遵守**: 1 巡毎に cold launch verify、cascade 露出は次 judgment 起点として AYA に渡す。**feedback_no_scope_shrink 範式遵守**: case X scope を「pbrterrainV location=21 only」literal で限定、V↔F mismatch (pbrterrainF 側) は予測 cascade として AYA に明示し η-25 移管に同意取得。**feedback_admit_unknown 範式遵守**: 第22層 cascade (+8 non-opaque) を「予測通り」と隠さず正直に報告。

---

## §4 patch 内容 (5 shader file)

### §4.1 Phase C: location reassign (η-18 §3.1 範式類継承)

#### §4.1.1 alphaV.glsl L130-L135: vary_norm 20 → 51

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-24 Phase C: vary_norm location 20 → 51
// (atmosphericsVarsV.glsl vary_AdditiveColor at location=20 との overlap 解消、η-18 §3.1 範式類)
layout(location=51) out vec3 vary_norm;
#else
out vec3 vary_norm;
#endif
```

#### §4.1.2 alphaF.glsl L131-L135: vary_norm 20 → 51 (V↔F 整合)

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-24 Phase C: vary_norm location 20 → 51
// (alphaV.glsl 側と整合、atmosphericsVarsF.glsl vary_AdditiveColor at location=20 との overlap 解消)
layout(location=51) in vec3 vary_norm;
#else
in vec3 vary_norm;
#endif
```

#### §4.1.3 pbrterrainV.glsl L113-L119: vary_vertex_normal 20 → 50 (V↔F mismatch も同時解消)

```glsl
#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-24 Phase C: vary_vertex_normal location 20 → 50
// (atmosphericsVarsV.glsl vary_AdditiveColor at location=20 との overlap 解消、pbrterrainUtilF.glsl L59
//  既存 location=50 と V↔F 整合、η-18 §3.1 範式類)
layout(location=50) out vec3 vary_vertex_normal; // Used by pbrterrainUtilF.glsl
#else
out vec3 vary_vertex_normal; // Used by pbrterrainUtilF.glsl
#endif
#endif
```

#### §4.1.4 pbrterrainV.glsl L121-L130: vary_tangents 21 → 52 (case X = Phase C 第2巡 cascade 補完)

```glsl
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_NORMAL)
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-24 第2巡 Phase C: vary_tangents location 21 → 52
// (atmosphericsVarsV.glsl vary_AtmosAttenuation at location=21 との overlap 解消、η-18 §3.1 範式類)
// vec3[4] = 4 slots (52,53,54,55)、location 50 vary_vertex_normal / 51 vary_norm の直後で連続
// 予測 cascade: pbrterrainF.glsl L206 vary_tangents loc=20 (vary_AdditiveColor との overlap +
//   V↔F mismatch) は次 η iteration で resolve
layout(location=52) out vec3 vary_tangents[4];
#else
out vec3 vary_tangents[4];
#endif
```

### §4.2 Phase D: FrameAtmosphere_Skybox 削除 + PerProgramUBO_GammaCorrect 新規 (η-14 Path G 範式継承)

#### §4.2.1 postDeferredGammaCorrect.glsl L36-L54: FrameAtmosphere_Skybox 削除 + PerProgramUBO_GammaCorrect 新規

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-24 Phase D: FrameAtmosphere_Skybox 削除
// (η-14 Path G 範式継承)。FrameAtmosphere_Lighting (set=0 binding=2 from
// atmosphericsHelpersV/atmosphericsFuncs) と member 名 sunlight_color 等 17 件
// が global scope で重複 = nameless block name collision。この file の main() で
// 必要な Skybox 単独 member は `gamma` のみ。新規 PerProgramUBO_GammaCorrect
// (set=2, binding=2、η-3 §3.2 PerDrawUBO 範式類) で gamma 専用 UBO を declare。
#ifndef PER_PROGRAM_UBO_GAMMA_CORRECT_DEFINED
#define PER_PROGRAM_UBO_GAMMA_CORRECT_DEFINED 1
layout(set=2, binding=2, std140) uniform PerProgramUBO_GammaCorrect {
    float gamma;
    float _pad_gc0;
    float _pad_gc1;
    float _pad_gc2;
};
#endif
#else
uniform float gamma;
#endif
```

#### §4.2.2 postDeferredTonemap.glsl L41-L62: FrameAtmosphere_Skybox 削除 + PerProgramUBO_GammaCorrect 新規 (GAMMA_CORRECT gated)

```glsl
#ifdef GAMMA_CORRECT
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-24 Phase D: FrameAtmosphere_Skybox 削除
// (η-14 Path G 範式継承)。FrameAtmosphere_Lighting (set=0 binding=2 from
// atmosphericsHelpersV/atmosphericsFuncs) と member 名 sunlight_color 等 17 件
// が global scope で重複 = nameless block name collision。この file の main() で
// 必要な Skybox 単独 member は `gamma` のみ (legacyGamma() 内、#ifdef GAMMA_CORRECT
// で gated)。新規 PerProgramUBO_GammaCorrect (set=2, binding=2、η-3 §3.2 PerDrawUBO
// 範式類、postDeferredGammaCorrect.glsl と同 layout) で gamma 専用 UBO を declare。
#ifndef PER_PROGRAM_UBO_GAMMA_CORRECT_DEFINED
#define PER_PROGRAM_UBO_GAMMA_CORRECT_DEFINED 1
layout(set=2, binding=2, std140) uniform PerProgramUBO_GammaCorrect {
    float gamma;
    float _pad_gc0;
    float _pad_gc1;
    float _pad_gc2;
};
#endif
#else
uniform float gamma;
#endif
#endif
```

### §4.3 cache invalidate

shader file 変更のみ、C++ transformer は v5 据置 (η-18 末)。shader_cache clear で対応:
```bash
rm -rf ~/.ayastorm_x64/cache/shader_cache/*
```

---

## §5 cascade shift forward 第22層露出 + η-25 移管計画

### §5.1 第22層 cascade 露出 (η-24 で観測、Phase C 救済 4 program + 同 file 内露呈)

η-24 で alphaV/F vary_norm + pbrterrainV vary_vertex_normal/vary_tangents reassign + postDeferred 2 file の FrameAtmosphere_Skybox 削除 = parse 通過 → 第22層 cascade 露出:

| 第22層 cascade 露出 | 件数 | 露出元 | η-25 主軸候補 |
|---|---|---|---|
| Deferred Alpha 系 4 program bare uniform | ~4 | alphaV/F 救済 | A scope (non-opaque) |
| pbrterrainV.glsl L170 `terrain_texture_transforms` | 1 | pbrterrainV case X 救済 | A scope (η-23 §3.1 範式、同 file 内) |
| postDeferredTonemap 系 6 件 bare uniform (color_saturation 等) | 6 | postDeferredTonemap 救済 | A scope (η-23 §3.1 / §3.2 範式) |
| FXAA / Deferred Post 系等の non-opaque 残 | ~32 | 既存 + 第22層 | A scope (η-23 §3.1 / §3.2 範式) |

### §5.2 η-25 主 scope 候補

| カテゴリ | 件数 | 推奨 sub-bundle | 根拠 |
|---|---|---|---|
| **non-opaque uniforms outside a block** | **43** | **η-25 主 scope 候補 A** | 件数最大、Phase C/D 解消後の残主軸。η-23 §3.1 GL-only wrap or η-23 §3.2 PerDrawUBO 集約範式継続適用。file 別棚卸し: postDeferredTonemap 6 / pbrterrainV 同 file 内 1 / FXAA / Post / 他 |
| **pbrterrainF.glsl V↔F mismatch (vary_tangents loc=20 vs V側 52 / vary_signs loc=24 vs V側 25)** | 2 件 (1 program / 同 file 内) | η-25 副 scope 候補 B (C 範式継続) | Phase C case X で V 側のみ完結、F 側は V↔F mismatch + vary_AdditiveColor overlap が残。η-18 §3.1 範式類継続 |
| `'shadow_clip' : redefinition` (parse) | 1 | η-25 副 scope 候補 C | (η-23 §5.2 副 scope B 継承)、η-20 §3.1 範式類 |
| `missing #endif` (parse、phantom) | 3 | η-25 副次効果 | 独立 fix 不可、先行 ERROR fix で同時消滅予想 |
| `compilation errors. No code generated` (summary) | 43 | (parse 失敗 program 数 summary 行、独立 fix 不可) | 先行 ERROR fix で連動減少 |

**合計 net 不重複 ERROR**: 約 47 件 (43 + 2 + 1 + 3 - phantom 重複)。

### §5.3 link 段階 ZERO 維持 milestone 観測

η-21 で達成した link 段階 ZERO を η-22 + η-23 + η-24 で **3 sub-bundle 連続維持**。第22層 cascade 露出は parse 段階単一段階で集中 = η-25 で同様に parse-stage 集中攻略可能。link 段階退行 0 件達成。

### §5.4 PerProgramUBO_GammaCorrect の C++ binding 対応 (η-25+ 移管検討)

η-24 では shader 側のみ UBO 化、**C++ runtime での UBO buffer 書込み (vkCmdUpdateBuffer / vkCmdBindDescriptorSets) は kill-switch (RenderUseVulkanGL=0) で GL path 動作維持中**につき未着手。η-25+ で kill-switch 解除 phase に入ったら以下が必要:
- C++ side: `PerProgramUBO_GammaCorrect` 用 VkBuffer + VkDescriptorSetLayout (set=2 binding=2) 確保
- per-program 書込み: `glUniform1f("gamma", val)` を `memcpy + vkCmdUpdateBuffer` に置換
- バインディング: `vkCmdBindDescriptorSets(set=2)` で descriptor set 結合

### §5.5 set=2 namespace 連番継続観測

| set | binding | UBO 名 | 適用 sub-step |
|---|---|---|---|
| 2 | 0 | PerDrawUBO_LightParams | η-3 |
| 2 | 1 | PerDrawUBO_MultiLight | η-23 |
| 2 | 2 | PerProgramUBO_GammaCorrect | η-24 |

η-25+ で他 per-program UBO 必要時は **set=2 binding=3** から連番継続候補 (η-3 §3.2 PerDrawUBO 派生範式遵守)。

### §5.6 location reassign 帯 50-59 観測

| location | varying | 適用 |
|---|---|---|
| 50 | vary_vertex_normal | η-24 (pbrterrainV/Util) |
| 51 | vary_norm | η-24 (alphaV/F) |
| 52-55 | vary_tangents[4] | η-24 case X (pbrterrainV、F 側は η-25 で V↔F 整合) |
| 56-59 | (未使用) | η-25+ で他 location overlap 解消候補帯 |

η-25 で pbrterrainF V↔F mismatch (vary_signs V側 25 vs F側 24) を解消する際は **56 帯** (V側 52-55 vary_tangents の直後) で連続させる候補。

---

## §6 risks (η-23 §6 継承 + 新規)

| # | risk | 対応状態 | 引継 sub-bundle |
|---|---|---|---|
| 1 | C++ runtime transformer が GL path で influence する | ✓ kill-switch で完全 bypass、η-16 から継承 | 引継不要 |
| 2 | preprocessor 別分岐の bare/manual-wrap 共存 | ✓ η-18 で再利用優先範式に修正済 | 引継不要 |
| 3 | regex group index / kQuals non-capturing 不整合 | ✓ η-18 で修正済 | 引継不要 |
| 4 | shader file level guard が異 type 同名 varying program で衝突 | ✓ η-19 で確認済 | 引継不要 |
| 5 | PerDrawUBO member rename + #define alias の関数 param/local var 共存問題 | ✓ η-20 §4.1.2 で verify 済 | 引継不要 |
| 6 | #undef scope-limit が file 末尾で対応 = file 内全 use site が rewrite | ✓ η-20 で意図通り | 引継不要 |
| 7 | cascade shift forward 第20層 (env_mat 16 + SPIR-V 3 解消の結果) | ✓ η-22 で 19 件構造解消 | 引継不要 |
| 8 | sampler binding 番号 conflict (set=0, binding=5 統一原則) | ✓ η-21 で実装、verify 済 | η-25+ で新規 sampler wrap 時は既存 binding 統一原則継承 |
| 9 | V/F MaterialUBO member alignment 跨 file 異 layout 可能性 | ✓ η-21 で 4 declarations 全件同 layout verify 済 | 引継不要 |
| 10 | dead vary_position 削除で同 ident 異 path 残置 | ✓ η-21 で literal trace 確認 | 引継不要 |
| 11 | legacy bare uniform GL-only wrap で main() 参照ありの場合は UBO 置換必要 | ✓ η-22 / η-23 で multiPointLightF dead / alive 切分 verify 済 | η-25+ で同範式適用時は main() 内参照 trace literal 確認必須 |
| 12 | bare vertex attribute layout(location=X) で C++ binding 整合 | ✓ η-22 で position=0 = TYPE_VERTEX=0 verify 済 | η-25+ で他 attribute (normal/texcoord0 等) 対応時は同範式継承 + index 整合 |
| 13 | upstream OpenGL Firestorm merge 時 shader file 互換 | 互換性高 (η-24 範式は標準 GLSL) | sub-step 4.5 |
| 14 | cinematic_bd overlay mirror | η-24 では touch なし、alphaV/F / pbrterrainV / postDeferred 系 不在を Glob で確認済 | 引継不要 |
| 15 | non-opaque uniforms 35 → 43 件 第22層 cascade exposure (Phase C/D 救済 program 内包) | η-23 §3.1 / §3.2 範式継続適用予定 | η-25 主 scope 候補 A |
| 16 | nameless block 5 → 0 完封 (Phase D 達成) | ✓ η-14 Path G + η-3 §3.2 PerDrawUBO 範式類で構造解消 | 引継不要 |
| 17 | shadow_clip redef 1 件 残 | η-20 §3.1 範式類で構造解決候補 | η-25 副 scope C |
| 18 | overlapping loc 5 → 0 完封 (Phase C 達成) | ✓ η-18 §3.1 範式類で 50-55 帯 reassign | 引継不要 (B 側 V↔F mismatch は別 risk #20) |
| 19 | Phase C 第1巡 cascade 第21層露出 (pbrterrainV vary_tangents loc=21) は同 scope 内完結 = 第2巡 case X で全解消 | feedback_one_step_at_a_time / feedback_no_scope_shrink 範式遵守 | 引継不要 |
| 20 | pbrterrainF V↔F mismatch (vary_tangents V側 52 vs F側 20 / vary_signs V側 25 vs F側 24) + vary_AdditiveColor overlap (F側 loc=20) | case X scope 外 (V 側のみ literal 受領)、η-25 副 scope B として 56 帯と 24→ 移管候補 | η-25 副 scope B (C 範式継続) |
| 21 | AYA judgment 2 段階受領 (第1巡 案 D → 第2巡 case X) で完走 | feedback_one_step_at_a_time 範式遵守、1 巡毎に verify | η-25+ でも継承 |
| 22 | PerProgramUBO_GammaCorrect / PerDrawUBO_MultiLight / PerDrawUBO_LightParams C++ binding 未対応 | kill-switch GL path 動作維持中につき未着手、η-24 では shader-only patch | sub-step 4.5+ (kill-switch 解除 phase) |
| 23 | FrameAtmosphere_Skybox UBO 削除で他 program 影響可能性 | postDeferredGammaCorrect / postDeferredTonemap = main() で `gamma` 単独参照のみ literal 確認、他参照 program は η-23 末 baseline log で nameless block error 集中の 5 件のみ確認済 | 引継不要 (η-24 で対象 2 file 完了、3 件は η-25 以降 cascade で別 file 露出時に literal trace 必須) |

---

## §7 observability

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 件数 (η-7 305 baseline、η-22 末 421 件 transformed dump 維持) | η-24 末 verify 後 421 件超 transformed dump 維持、cold launch 後 cache 合計 計測継続 | B?-η-25 で baseline 更新 |
| 2 | C++ runtime location emit 範式 (η-16 §3.1) Phase 2 (V↔F pair) 維持 | ✓ η-24 で touch なし、v5 据置 | B?-η-25 で SPIR-V loc 追加対応時 Phase 3 拡張候補 |
| 3 | shader file level include guard 範式 (η-19 §3.1) 適用範囲 | η-24 で postDeferredGammaCorrect/postDeferredTonemap に `PER_PROGRAM_UBO_GAMMA_CORRECT_DEFINED` guard 拡張適用 | B?-η-25 で他 file 続行 |
| 4 | PerDrawUBO member rename + #define alias + #undef 範式 (η-20 §3.1) | η-24 では touch なし | B?-η-25 で shadow_clip redef 同範式適用候補 |
| 5 | bare opaque uniform Vulkan binding 付与範式 (η-21 §3.1) | η-24 では touch なし、η-21 で 1 件適用済 | B?-η-25 で他 bare sampler / texture / image 残時に同範式継承 |
| 6 | V/F UBO member alignment 範式 (η-21 §3.2) | η-24 では touch なし | B?-η-25 で他 UBO V/F alignment 必要時に同範式継承 |
| 7 | dead F-side declaration 削除範式 (η-21 §3.3) | η-24 では touch なし | B?-η-25 で他 dead in 検出時に同範式継承 (literal trace 確認必須) |
| 8 | legacy bare uniform GL-only wrap 範式 (η-22 §3.1) | η-24 では touch なし、η-22/η-23 で適用済 | B?-η-25 で他 bare uniform × UBO member redeclare 衝突 / dead 検出時に同範式継承 |
| 9 | bare vertex attribute Vulkan location 付与範式 (η-22 §3.2) | η-24 では touch なし | B?-η-25 で他 vert shader bare in 検出時に同範式継承 |
| 10 | PerDrawUBO 範式拡張 (η-3 §3.2 → η-23 §3.2 → η-24 §3.2) per-program 派生 | η-24 で PerProgramUBO_GammaCorrect 新規導入 (set=2, binding=2、float×4 std140 alignment 整合) | B?-η-25 で他 per-program UBO 必要時に同範式継承 (set=2 binding=3+) |
| 11 | FrameAtmosphere_Lighting per-shader declare 統一範式 (η-4 §3.2 → η-23 §3.3) | η-24 では touch なし、η-23 で multiPointLightF.glsl に declare 追加済 | B?-η-25 で他 shader classic_mode / minimum_alpha 等参照時に同範式継承 |
| 12 | dump 機構 (η-16 §3.1 内包) η-24 で 421 件超出力 | 維持 | B?-η-25 で必要に応じて再投入 |
| 13 | falsification iterate 範式 (η-18 内 v2→v5 4 段) | η-24 は AYA judgment 2 段階受領後 2 巡完走、falsification iterate 不要 | B?-η-25 で C++ transformer Phase 3 拡張時は falsification iterate 想定 |
| 14 | 既達主指標完全維持 (η-24 で 18 種維持 + Phase C/D 2 種完封、退行 0 件) | ✓ η-24 で退行 0 件達成 | B?-η-25 で第22層 cascade non-opaque 43 件 scope |
| 15 | cinematic_bd directory 全 .glsl audit (η-17 §10.4 継承) | η-24 で touch なし、η-22 末 audit 維持 | B?-η-25+ 着手前 or 完遂後の別 phase として継続推奨 |
| 16 | feedback_self_verify_before_handoff 適用 | η-24 で literal log + cold launch 2 巡で Phase C/D 完封確認 | B?-η-25 でも AYA cold launch 依頼前に必ず実施 |
| 17 | feedback_admit_unknown 適用 (第22層 cascade 予測通り報告) | η-24 で第22層 cascade exposure を「予測通り」と隠さず正直に報告、AYA judgment 起点として渡す | B?-η-25 でも継承 |
| 18 | feedback_no_scope_shrink 適用 | AYA「推奨案で」「X」literal 受領 → Phase C/D 完封 + case X cascade 補完まで実装 | B?-η-25 でも継承 |
| 19 | feedback_remove_verification_logs 適用 | η-24 で追加 LL_INFOS hook 無し、不要 | B?-η-25 で追加 LL_INFOS hook あれば commit 前に DEBUG/削除 判断 |
| 20 | feedback_one_step_at_a_time 適用 | η-24 で 1 step 1 verify 範式遵守 (trace → 候補 A/B/C/D/E 提示 → AYA judgment 第1巡 → patch 第1巡 → cold launch → cascade 第21層分析 → case X 提示 → AYA judgment 第2巡 → patch 第2巡 → cold launch → 主指標確認 → commit) | B?-η-25 でも継承 |
| 21 | link 段階 ZERO 維持 milestone (η-21 達成、η-22 + η-23 + η-24 で 3 sub-bundle 連続維持) | ✓ η-24 で達成 | B?-η-25 では parse 段階集中攻略、link 段階 ZERO 維持を引き続き観測 |
| 22 | set=2 namespace 連番継続観測 (η-3 binding=0, η-23 binding=1, η-24 binding=2) | ✓ η-24 で binding=2 で連番継続 | B?-η-25 で他 per-program UBO 必要時は binding=3+ から連番 |
| 23 | location reassign 帯 50-59 観測 (η-24 で 50/51/52-55 使用) | ✓ η-24 で 50-55 使用、56-59 空き | B?-η-25 で pbrterrainF vary_signs 等 V↔F 整合候補は 56 帯から連続 |

---

## §8 引継 scope 推奨 (B?-η-25)

### §8.1 着手前 trace (14 ステップ ベースで η-25 適応)

1. AYA 「コマンド + ビルド全権」+「verify は 1 ステップずつ」運用継承確認
2. η-24 末 baseline log (`/home/ishikawa/.ayastorm_x64/logs/AYAstorm.log` 起動 2026-06-02T12:02Z) を canonical baseline として extract
3. ERROR 90 件を **カテゴリ別 + program 別** に系統整理
4. **主 scope 候補 A**: parse failed `non-opaque uniforms outside a block` 43 件継続解消 (件数最大、η-23 §3.1 GL-only wrap or η-23 §3.2 PerDrawUBO 集約範式継続適用、multi-file: postDeferredTonemap 6 / pbrterrainV 同 file 内 terrain_texture_transforms 1 / FXAA / Post / Deferred Alpha 系内包 bare uniform / 他)
5. **副 scope 候補 B**: pbrterrainF.glsl V↔F mismatch (vary_tangents loc=20 vs V側 52 / vary_signs loc=24 vs V側 25) + vary_AdditiveColor overlap (F側 loc=20) を η-18 §3.1 + η-21 §3.2 範式類で同時解消 (V側 52 と F側 52 揃え + vary_signs V側 25 vs F側 24 を V側 56 / F側 56 帯に揃え)
6. **副 scope 候補 C**: parse failed `'shadow_clip' : redefinition` 1 件 (Haze/Godrays 系、η-20 §3.1 範式類)
7. **A/B/C どれを Phase 1 主 scope に置くか + 副 scope を η-25 内 Phase 2 として同梱するか別 sub-bundle に分離するか** = AYA judgment 候補
8. **non-opaque 43 件 file 別棚卸し** (η-25 主 scope 候補 A): どの file から優先的に範式適用するか、ROI 最大の file 群 (postDeferredTonemap 6 件束 / pbrterrainV 同 file 内 / 他) を最初に
9. **agent 報告 = 仮説扱いで literal 検証必須** (η-23 §3.5 教訓継承、Explore agent を使う場合は Claude 自身で literal log + dump + source code 検証フォロー)
10. transformer Phase 3 拡張 (V stage bare `in` attribute 自動 location emit) は **将来 SPIR-V loc 大量発覚時 or 限定的に手動 layout 付与 で対応** を AYA judgment 候補
11. transformer version tag は η-18 末 `v5_p2_inout_pair_prepass_group_fix` 据置、η-25 で Phase 3 拡張時 `v6_p3_vertex_attribute` bump 必須
12. cinematic_bd directory audit (η-17 §10.4 継承) を η-25 着手前 or 完遂後の別 phase として継続推奨
13. **feedback_admit_unknown / feedback_one_step_at_a_time / feedback_no_scope_shrink 範式継承** で η-25 内でも literal observation 優先 + AYA judgment 受領後の scope 全実装維持
14. **link 段階 ZERO 連続維持** (η-21 → η-22 → η-23 → η-24 = 3 sub-bundle 連続)、η-25 で 4 sub-bundle 連続目標

### §8.2 想定 Phase 構成 (η-25)

η-25 着手前 trace + AYA judgment で確定だが、現時点 推定:
- **Phase 1 (主 scope 候補 A 推奨)**: non-opaque uniforms 43 件 cascade 解消 (η-23 §3.1 / §3.2 範式継続)、file 別棚卸しでバッチ処理 (postDeferredTonemap 6 件束 / pbrterrainV 同 file 内 / FXAA / Post / 他)
- **Phase 2 (副 scope)**: pbrterrainF V↔F mismatch (vary_tangents 20 / vary_signs 24) を Phase C 範式類継続で 56 帯整合 + shadow_clip redef 1 件 (η-20 §3.1 範式類)

### §8.3 B?-η-25 完遂後の想定 cascade exposure 第23層

- non-opaque 43 件解消想定 = parse 段階大量解消、第23層 emergence 観測点: pipeline cache / runtime binding / SPIR-V layout validation 等
- pbrterrainF V↔F mismatch 解消想定 = parse 段階完封 + V↔F pair 整合
- 残 shadow_clip 1 = η-26+ 移管想定
- shader_cache 件数: η-25 で計測継続 + 第23層 emergence 観測

---

## §9 commit message (記録)

patch commit `2e642ce685`:
```
feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-24 完遂

Phase C (location reassign, η-18 §3.1 範式類):
- alphaV.glsl / alphaF.glsl: vary_norm location 20 → 51
  (atmosphericsVarsV.glsl vary_AdditiveColor at location=20 との overlap 解消)
- pbrterrainV.glsl: vary_vertex_normal location 20 → 50
  (pbrterrainUtilF.glsl L59 既存 location=50 と V↔F 整合)
- pbrterrainV.glsl: vary_tangents location 21 → 52 (第2巡 case X)
  (atmosphericsVarsV.glsl vary_AtmosAttenuation at location=21 との overlap 解消、
   vec3[4] = slots 52-55)

Phase D (FrameAtmosphere_Skybox 削除、η-14 Path G 範式継承):
- postDeferredGammaCorrect.glsl / postDeferredTonemap.glsl:
  FrameAtmosphere_Lighting (set=0 binding=2) と member 名 17 件 global scope
  重複 = nameless block name collision。両 file は `gamma` 単独 member のみ
  必要なので、新規 PerProgramUBO_GammaCorrect (set=2, binding=2、
  η-3 §3.2 PerDrawUBO 範式類) で gamma 専用 UBO を declare。

検証 (cold launch log):
- ERROR: 行数 104 (η-23 end) → 91 (η-24 第1巡) → 90 (η-24 第2巡 case X)
- overlapping location: 1 → 0 ✅ (カテゴリ完封)
- nameless block name collision: 0 維持 (η-14 Path G 範式踏襲)
- parse 失敗 program 数: 43

予測 cascade (η-25 以降):
- pbrterrainV.glsl L170 terrain_texture_transforms bare uniform (Phase C で
  loc overlap 解消後、同 file 内で η-23 §3.1 範式 territory に露呈)
- pbrterrainF.glsl L206 vary_tangents loc=20 (V↔F mismatch + vary_AdditiveColor
  overlap、V 完結後の F territory)
- non-opaque uniforms 43 件 cascade (A scope 大量 UBO 化)
- missing #endif 3 件 (preprocessor cleanup)
```

handoff doc commit message 案:
```
docs(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-24-complete handoff 起草

main scope = Phase C location reassign (5 件) + Phase D FrameAtmosphere_Skybox
削除 (5 件) の 2 種カテゴリ完封 + link 段階 ZERO 3 sub-bundle 連続維持 の
完遂 handoff doc。新規 §3.2 η-14 Path G 範式継承 (UBO 削除 + per-program UBO
新規) + §3.4 第22層 cascade 露出観測 を策定。

主指標達成:
- overlapping use of location 5 → 0 (Phase C 完封)
- nameless block ... global scope 5 → 0 (Phase D 完封)
- ERROR 計 104 → 90 (-14、第22層 non-opaque +8 cascade 露出含む)
- glslang link failed for program 0 → 0 (link 段階 ZERO 3 sub-bundle 連続維持)
- missing #endif 13 → 3 (-10 phantom 大幅減)
- clean shutdown 維持
- 18 種既達主指標完全維持

cascade shift forward 第22層露出:
- non-opaque 35 → 43 (+8、Phase C 救済 4 program 内包 bare uniform + pbrterrainV
  同 file 内 terrain_texture_transforms 等)
- η-25 主軸は A scope (non-opaque 43 件 = postDeferredTonemap 6 / pbrterrainV
  同 file 内 / FXAA / Post / 他)

patch commit: 2e642ce685
```

---

**handoff doc 完。次 sub-bundle B?-η-25 着手は本 doc §8 推奨 scope (parse non-opaque 43 件主軸 + pbrterrainF V↔F mismatch 副 + shadow_clip 1) を起点として、fresh context で実施。**
