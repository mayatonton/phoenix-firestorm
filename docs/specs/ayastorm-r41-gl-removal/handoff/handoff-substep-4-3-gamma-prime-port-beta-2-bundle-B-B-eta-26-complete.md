# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-26 完遂 handoff

**status**: B?-η-26 完遂 (Phase 1 bare uniform → per-program UBO 集約 3 file [shadowAlphaMaskV shadow_target_width / postDeferredV tc_scale / fullbrightShinyV texture_matrix1 + origin GL-only wrap] + Phase 1d 第23層 cascade fix [shadowAlphaMaskV !HAS_SKIN Vulkan FrameViewProj guard wrap、symmetric 化] = ERROR 計 68 → 62 → **54** (-14 累積)、`undeclared identifier 'modelview_projection_matrix'` 0 → 2 → **0** ✓ 第23層 cascade 完封、`compilation terminated` 0 → 2 → **0** ✓ 完封、`missing #endif` 3 → 5 → **3** η-25 baseline 復帰、`glslang link failed for program` **0 → 0** = **link 段階 ZERO 6 sub-bundle 連続維持** (η-21 milestone)、clean shutdown 維持) → 次 sub-bundle B?-η-27 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: `a26b5b8ba7` (3 shader file 編集、+50/-1 = +49 行、C++ 変更 0、user_settings.xml 変更 0)
**handoff doc commit**: 本 doc (η-25-complete 範式継承、別 commit)
**勤続範式継承**: B?-η-25-complete `229479522c` (patch `229479522c`) / B?-η-24-complete `3777960572` (patch `2e642ce685`) / B?-η-23-complete `dba5c14820` (patch `9cc143d2a6`) / B?-η-22-complete `48ceb1c953` (patch `5cc4b03bab`) / B?-η-21-complete `989b529936` (patch `e78ca6006c`) / B?-η-20-complete `d7c722f4e7` (patch `557cd1db00`) / B?-η-19-complete `4911401568` (patch `9349ace5f0`) / B?-η-18-complete `1371da9660` (patch `4b42779cc7`) / B?-η-17-complete `cc2e878c7e` (patch `e2d4d3bbca`) / B?-η-16-complete `3653efed00` (patch `72fd4f3c3c`) / B?-η-15-complete `cb28cf1daa` (patch `6a11eabc73`) / B?-η-14-complete `78df235243` (patch `c971838656`) / B?-η-13-reverted (no commit) / B?-η-12-complete `7c1762d214` / B?-η-11-complete `74705c35bb` / B?-η-10-complete `9246142639` / B?-η-9-complete `6fee4a818b` / B?-η-8-complete `6994eba271` / B?-η-7-complete `b1e8689634` / B?-η-6-complete `fe757ea624` / B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-26 scope = **AYA judgment 「案 A で進めてください」 (= non-opaque top 3 file 集約) literal 受領 → fullbrightShinyV cascade 先制 trace で `origin` dead 確定 → AYA「A2 修正案で進めてください」literal (texture_matrix1 UBO 化 + origin GL-only wrap) で案 A2 (修正版) 受領** の同梱完遂 + **Phase 1d 第23層 cascade fix の AYA「OK」literal 受領後 同 sub-bundle 内完封**。**1 主 sub-phase + 1 cascade fix** で全カテゴリ主目標達成。

**Phase 1 (bare uniform → per-program UBO 集約、η-3 §3.2 PerDrawUBO 派生範式類継承)**:

- **shadowAlphaMaskV.glsl** (Phase 1a、4 program: Deferred / Skinned Deferred / Deferred Fullbright / Skinned Deferred Fullbright Shadow Alpha Mask 共通):
  - `shadow_target_width` (bare float) → **PerProgramUBO_ShadowAlphaMaskV (set=2, binding=6)** 集約
  - 根拠: shadow polygon offset epsilon 計算で main() 直接参照 (dead でない)、η-23 §3.1 GL-only wrap 不適用、η-24 §3.2 PerProgramUBO_GammaCorrect 派生範式類で per-program UBO 集約、float×4 = 16-byte chunk std140 整合
- **postDeferredV.glsl** (Phase 1b、4 program: FXAA Low / Medium / High / Ultra 共通):
  - `tc_scale` (bare vec2) → **PerProgramUBO_PostDeferredV (set=2, binding=7)** 集約
  - 根拠: vary_fragcoord/vary_tc 計算で main() 直接参照 (dead でない)、η-23 §3.1 GL-only wrap 不適用、vec2 + float×2 pad = 16-byte chunk std140 整合
- **fullbrightShinyV.glsl** (Phase 1c、3 program: Deferred / Skinned Deferred / HUD FullbrightShiny 共通):
  - `texture_matrix1` (bare mat4) → **PerProgramUBO_FullbrightShinyV (set=2, binding=8)** 集約
  - 根拠: shiny vertex texcoord 変換で main() 参照 (dead でない)、η-23 §3.1 GL-only wrap 不適用、mat4 = 64-byte (4 vec4 chunk) std140 整合
  - **dead uniform 追加処理**: `origin` (bare vec4) は cascade 先制 trace で全 file grep verify 後 use site 不在確定 → η-23 §3.1 範式類で `#ifndef LL_VULKAN_GLSL` GL-only wrap (Vulkan path 削除)

**Phase 1d (第23層 cascade exposure fix、η-5 範式類)**:
- **shadowAlphaMaskV.glsl** (1 file edit、L60-80 `#else (!HAS_SKIN)` ブロック):
  - Vulkan path で `modelview_projection_matrix` undeclared identifier 露呈 → η-5 FrameViewProj guard wrap 範式類で `#ifdef LL_VULKAN_GLSL` 側 FrameViewProj UBO declare 追加 (set=0, binding=0)、HAS_SKIN 側 L39-58 と symmetric 化
  - 根拠: Phase 1 で shadow_target_width UBO 化 → parser advance → `!HAS_SKIN` Vulkan 経路で modelview_projection_matrix declare 抜け落ち露呈 (L410/L413 cascade)。L60-64 元状態 = `#ifndef LL_VULKAN_GLSL uniform mat4 modelview_projection_matrix; #endif` で GL path のみ declare、Vulkan path で undeclared

**PerProgramUBO_ShadowAlphaMaskV std140 構造**:
```glsl
layout(set=2, binding=6, std140) uniform PerProgramUBO_ShadowAlphaMaskV {
    float shadow_target_width;
    float _pad_sam0;
    float _pad_sam1;
    float _pad_sam2;
};
```

**PerProgramUBO_PostDeferredV std140 構造**:
```glsl
layout(set=2, binding=7, std140) uniform PerProgramUBO_PostDeferredV {
    vec2  tc_scale;
    float _pad_pd0;
    float _pad_pd1;
};
```

**PerProgramUBO_FullbrightShinyV std140 構造**:
```glsl
layout(set=2, binding=8, std140) uniform PerProgramUBO_FullbrightShinyV {
    mat4 texture_matrix1;     // 64-byte (4 vec4 chunk) 整合
};
```

**set=2 namespace 連番継続**: η-3 binding=0 LightParams / η-23 binding=1 MultiLight / η-24 binding=2 GammaCorrect / η-25 binding=3 AlphaParams / binding=4 ColorGrading / binding=5 PointLightV / **η-26 binding=6 ShadowAlphaMaskV / binding=7 PostDeferredV / binding=8 FullbrightShinyV**

**計**: **shader 3 file 編集 (+50/-1 = +49 行)、C++ 変更 0、user_settings.xml 変更 0**

**主指標達成** (vs η-25 末 baseline log):

- `undeclared identifier 'modelview_projection_matrix'` (parse、第23層 cascade) 0 → 2 → **0** ✓ **Phase 1d cascade 完封**
- `compilation terminated` (parse、Phase 1d cascade 派生) 0 → 2 → **0** ✓ **Phase 1d 完封**
- `missing #endif` (parse、phantom + cascade) 3 → 5 → **3** ✓ **η-25 baseline 復帰** (Phase 1d 完封)
- ERROR 計 68 → 62 → **54** **-14 累積** (-11 non-opaque + 1巡 +6 cascade -> 2巡 -8 cascade 完封 (-6 + parse failed summary 行 -2 連鎖減))
- `glslang link failed for program` 0 → **0** ✓ **link 段階 ZERO 6 sub-bundle 連続維持 (η-21 milestone 継承)**
- `Goodbye!` 系 + clean shutdown / FATAL/SIGSEGV/Aborted 0/0/0 = ✓ clean shutdown 維持 (Quitting + Exiting main_loop 2 hits)

**cascade shift forward 第23層**: 第1巡で Phase 1 (shadow_target_width UBO 化) 後に shadowAlphaMaskV.glsl `!HAS_SKIN` Vulkan path で `modelview_projection_matrix` undeclared identifier 露呈。Phase 1 前は bare uniform parser fail でマスクされていた → bare uniform UBO 化で parser advance → 第23層露出。Phase 1d cascade fix で HAS_SKIN 側 L39-58 と symmetric 化により第2巡で完封。

| # | metric | η-25末 | η-26 1巡 | η-26 2巡 | Δ vs η-25 | 解析 |
|---|---|---|---|---|---|---|
| 1 | `non-opaque uniforms outside a block` (parse) | 31 | 24 | **24** | **-7** | Phase 1 で shadowAlphaMaskV 4 / postDeferredV 4 / fullbrightShinyV 3 = 11 件 file 内集約 (cascade 別カテゴリ +4 = 純減 -7) |
| 2 | `undeclared identifier 'modelview_projection_matrix'` (parse、第23層 cascade) | 0 | 2 | **0** | ±0 | ✓ **Phase 1d cascade 完封** (L1029=0:413 / L1043=0:410 → 解消) |
| 3 | `compilation terminated` (parse、cascade 派生) | 0 | 2 | **0** | ±0 | ✓ **Phase 1d 完封** |
| 4 | `missing #endif` (parse、phantom + cascade) | 3 | 5 | **3** | ±0 | 1巡 cascade +2、2巡で symmetric 化により -2 復帰 |
| 5 | `glslang link failed for program` | 0 | 0 | **0** | ±0 | ✓ link 段階 ZERO **6 sub-bundle 連続維持** |
| 6 | `overlapping use of location` (parse) | 0 | 0 | **0** | ±0 | ✓ η-25 達成維持 |
| 7 | `'shadow_clip' : redefinition` (parse) | 0 | 0 | **0** | ±0 | ✓ η-25 達成維持 |
| 8 | `'nameless block ... global scope'` (parse) | 0 | 0 | **0** | ±0 | ✓ η-24 達成維持 |
| 9 | Anonymous member (link) | 0 | 0 | **0** | ±0 | ✓ η-20 達成維持 |
| 10 | shadow_bias / M_PI / weight4 redef (parse) | 0 | 0 | **0** | ±0 | ✓ η-20 達成維持 |
| 11 | Input `vary_position` (link) | 0 | 0 | **0** | ±0 | ✓ η-21 達成維持 |
| 12 | MaterialUBO metallicFactor / fragment block (link) | 0 | 0 | **0** | ±0 | ✓ η-21 達成維持 |
| 13 | sampler binding (parse) | 0 | 0 | **0** | ±0 | ✓ η-21 達成維持 |
| 14 | env_mat cannot redeclare (parse) | 0 | 0 | **0** | ±0 | ✓ η-22 達成維持 |
| 15 | SPIR-V requires location (parse) | 0 | 0 | **0** | ±0 | ✓ η-22 達成維持 |
| 16 | Multi-Light non-opaque (parse) | 0 | 0 | **0** | ±0 | ✓ η-23 達成維持 |

**主指標達成総括**: **Phase 1d 第23層 cascade 3 カテゴリ完封 (undeclared identifier / compilation terminated / missing #endif baseline 復帰)** + **Phase 1 で non-opaque -7 純減 (11 件 file 内集約 + cascade 別カテゴリ +4)** + **link 段階 ZERO を η-21 milestone から 6 sub-bundle 連続維持** + **clean shutdown 維持** + **ERROR 予測 -6 → 実 -8 で parse 失敗 summary 行も連鎖減**。第23層 cascade exposure (Phase 1 後 +6) は AYA「OK」literal 受領後 Phase 1d cascade fix を **同 sub-bundle 内で完遂** = feedback_no_scope_shrink 範式遵守。

**他既達主指標完全維持** (η-26 末):
- η-25 達成全項目 (shadow_clip redef 0 / overlapping location 0)
- η-24 達成全項目 (overlapping loc 0 / nameless block 0)
- η-23 達成全項目 (Multi-Light non-opaque 0)
- η-22 達成全項目 (env_mat redeclare 0 / SPIR-V requires location 0)
- η-21 達成全項目 (link failed 0 / sampler binding 0 / metallicFactor 0 / vary_position 0)
- η-20 達成全項目 (Anonymous member 0 / shadow_bias / M_PI / weight4 redef 0)
- η-19 達成全項目 (normalMap / depthMap / vary_fragcoord redef 0)
- η-18 達成全項目 (Layout location qualifier 0)
- η-1〜η-17 達成全項目 (Cannot reuse block name 0 / 'binding' 0 / GBufferInfo redef 0 / 'size' undeclared 0 / undeclared identifier 0)

**Phase 構成の特徴**: η-26 は **AYA judgment 2 段階受領 (第1巡 案 A literal → A2 修正案 literal (origin GL-only wrap 追加))** で完走、+ **第23層 cascade fix の AYA 確認後 Phase 1d 追加実行**。**feedback_one_step_at_a_time 範式遵守** (1 巡毎に verify、第23層 cascade 露出後の判断を AYA に渡し確認取得後実行)、**feedback_no_scope_shrink 範式遵守** (AYA「OK」literal 受領後、Phase 1d cascade fix を同 sub-bundle 内で完封まで実装)、**feedback_doubt_self_first 範式遵守** (fullbrightShinyV cascade 先制 trace で origin の use site 全 file grep verify、dead 確定後 GL-only wrap 適用)、**feedback_admit_unknown 範式遵守** (ERROR 予測 -6 に対し実 -8 で「予測超」を正直に報告、parse 失敗 summary 行連鎖減を後付け解析として明示)。

---

## §2 完遂結果 metric (vs B?-η-25 末 baseline log)

| metric | η-25末 | η-26 1巡 | η-26 2巡 (最終) | Δ vs η-25 | 判定 |
|---|---|---|---|---|---|
| **`undeclared identifier 'modelview_projection_matrix'` (parse、第23層 cascade)** | **0** | **2** | **0** | ±0 | ✓ **Phase 1d cascade 完封** |
| **`compilation terminated` (parse、cascade 派生)** | **0** | **2** | **0** | ±0 | ✓ **Phase 1d 完封** |
| **`missing #endif` (parse、phantom + cascade)** | **3** | **5** | **3** | ±0 | ✓ **η-25 baseline 復帰** |
| glslang link failed for program | 0 | 0 | 0 | ±0 | ✓ **link 段階 ZERO 6 sub-bundle 連続維持** |
| ERROR (total) | 68 | 62 | **54** | **-14** | 累積 -14 = -11 non-opaque + Phase 1d -6 + summary 行連鎖減 -2 |
| non-opaque uniforms (parse、total) | 31 | 24 | 24 | **-7** | Phase 1 集約 11 件 + cascade 別カテゴリ計上 |
| overlapping use of location (parse) | 0 | 0 | 0 | ±0 | ✓ η-25 達成維持 |
| shadow_clip redef (parse) | 0 | 0 | 0 | ±0 | ✓ η-25 達成維持 |
| nameless block ... global scope (parse) | 0 | 0 | 0 | ±0 | ✓ η-24 達成維持 |
| env_mat redeclare (parse) | 0 | 0 | 0 | ±0 | ✓ η-22 達成維持 |
| SPIR-V requires location (parse) | 0 | 0 | 0 | ±0 | ✓ η-22 達成維持 |
| Anonymous member (link) | 0 | 0 | 0 | ±0 | ✓ η-20 達成維持 |
| shadow_bias / M_PI / weight4 redef (parse) | 0 | 0 | 0 | ±0 | ✓ η-20 達成維持 |
| Input `vary_position` (link) | 0 | 0 | 0 | ±0 | ✓ η-21 達成維持 |
| MaterialUBO metallicFactor / fragment block (link) | 0 | 0 | 0 | ±0 | ✓ η-21 達成維持 |
| sampler binding (parse) | 0 | 0 | 0 | ±0 | ✓ η-21 達成維持 |
| Multi-Light non-opaque (parse) | 0 | 0 | 0 | ±0 | ✓ η-23 達成維持 |
| FATAL / SIGSEGV / Aborted | 0/0/0 | 0/0/0 | 0/0/0 | ±0 | ✓ clean shutdown 維持 |
| `Quitting` set + `Exiting main_loop` | ✓ | ✓ | ✓ | ±0 | ✓ clean shutdown 維持 |

---

## §3 設計範式

### §3.1 適用範式: η-3 §3.2 PerDrawUBO per-program 派生範式 (Phase 1 a/b/c 11 件 bare uniform UBO 集約 + 1 件 dead GL-only wrap)

**範式根拠**: Vulkan strict mode では `uniform <type> <name>;` (bare uniform、UBO 外) は SPIR-V 互換に「set/binding 必須」要求が満たされず parse error `non-opaque uniforms outside a block`。η-3 で確立した **PerDrawUBO_LightParams (set=2, binding=0)** から η-25 まで継続したい per-program/per-draw UBO 集約範式を、η-26 では `shadow_target_width` (shadowAlphaMaskV) / `tc_scale` (postDeferredV) / `texture_matrix1` (fullbrightShinyV) の **3 file 3 件 (11 program 共通)** に拡張適用。**dead uniform** (`origin`、fullbrightShinyV) は η-23 §3.1 GL-only wrap 範式類で Vulkan path 削除。

**η-26 適用 binding map**:

| set | binding | UBO 名 | declare 範式 | 適用 sub-step |
|---|---|---|---|---|
| 2 | 0 | PerDrawUBO_LightParams | η-3 §3.1 範式 | (既存、不変) |
| 2 | 1 | PerDrawUBO_MultiLight | η-23 §3.2 範式 | (既存、不変) |
| 2 | 2 | PerProgramUBO_GammaCorrect | η-24 §3.2 派生範式 | (既存、不変) |
| 2 | 3 | PerProgramUBO_AlphaParams | η-25 §3.1 派生 | (既存、不変) |
| 2 | 4 | PerProgramUBO_ColorGrading | η-25 §3.1 派生 | (既存、不変) |
| 2 | 5 | PerProgramUBO_PointLightV | η-25 §3.1 派生 | (既存、不変) |
| **2** | **6** | **PerProgramUBO_ShadowAlphaMaskV** | **η-26 §3.1 派生 (shadow_target_width)** | **η-26 Phase 1a** |
| **2** | **7** | **PerProgramUBO_PostDeferredV** | **η-26 §3.1 派生 (tc_scale)** | **η-26 Phase 1b** |
| **2** | **8** | **PerProgramUBO_FullbrightShinyV** | **η-26 §3.1 派生 (texture_matrix1)** | **η-26 Phase 1c** |

**有効性根拠**:
1. **non-opaque uniform 構造解決**: bare uniform は UBO 内 member 化で SPIR-V 互換 layout qualifier (set/binding) 整合
2. **std140 alignment 整合**: float×4 / vec2+float×2 / mat4 単独で C++ side 直書き互換
3. **GL path 完全非変更**: `#else` で bare `uniform <type> <name>;` 維持、C++ side `glUniform*()` binding 変更不要
4. **per-program 分離**: 各 UBO は単一 program family 内で完結 = global scope name collision 回避 (η-3 / η-23 / η-24 / η-25 と member 名重複なし)
5. **set=2 namespace 連番継続**: binding=0 → 8 の連続割当で η-27+ も同 namespace 拡張可能 (binding=9+ 開始)

### §3.2 適用範式: η-5 FrameViewProj guard wrap 範式 (Phase 1d 第23層 cascade fix)

**範式根拠**: shadowAlphaMaskV.glsl `#else (!HAS_SKIN)` ブロックは GL path だけ `uniform mat4 modelview_projection_matrix;` を declare、Vulkan path では declare 抜け落ち。Phase 1 前は bare uniform parser fail で parse advance しないので未露呈 → Phase 1 で shadow_target_width UBO 化 → parser advance → undeclared identifier 露出。η-5 §3.1 範式類で確立した **`#ifndef FRAME_VIEW_PROJ_DEFINED` guard** で重複 declare 回避しつつ FrameViewProj UBO declare を `#ifdef LL_VULKAN_GLSL` 側に追加、HAS_SKIN 側 L39-58 と symmetric 化。

**η-26 適用構造** (shadowAlphaMaskV.glsl L60-80):
```glsl
#else
#ifdef LL_VULKAN_GLSL
#ifndef FRAME_VIEW_PROJ_DEFINED
#define FRAME_VIEW_PROJ_DEFINED 1
layout(set=0, binding=0, std140) uniform FrameViewProj {
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    mat4 projection_matrix;
    mat4 inv_proj;
    mat4 proj_mat;
    mat4 last_modelview_matrix;
    mat3 env_mat;
    mat3 normal_matrix;
    vec2 screen_res;
};
#endif
#else
uniform mat4 modelview_projection_matrix;
#endif
#endif
```

**有効性根拠**:
1. **symmetric 化**: HAS_SKIN 側 L39-58 と完全同型 = 構造的整合
2. **`FRAME_VIEW_PROJ_DEFINED` guard**: 同一 program family 内で他 file の FrameViewProj UBO declare と衝突回避
3. **GL path 完全非変更**: `#else (!LL_VULKAN_GLSL)` 側で legacy `uniform mat4 modelview_projection_matrix;` 維持
4. **影響 program 完封**: L410 (Deferred Shadow Alpha Mask) + L413 (Deferred Shadow Fullbright Alpha Mask) の cascade 6 行解消

### §3.3 適用範式: η-23 §3.1 GL-only wrap 範式 (Phase 1c dead origin uniform)

**範式根拠**: 既存 GL path で declare されている bare uniform が main() / 関数呼出で参照されない (dead) の場合、Vulkan path で UBO 集約しても use 不在 = 不要工事。η-23 §3.1 範式類で確立した `#ifndef LL_VULKAN_GLSL` wrap で GL path 限定残置 + Vulkan path 削除。

**η-26 適用 (fullbrightShinyV.glsl `origin`)**:
```glsl
#ifndef LL_VULKAN_GLSL
uniform vec4 origin;
#endif
```
- use site grep: 全 fullbrightShinyV.glsl 内で `origin` 使用箇所なし (function 呼出も無し) = dead 確定
- η-25 Phase 3 (godraysF shadow_clip) と同型範式

**Phase 1 (UBO 集約) と Phase 1c-2 (GL-only wrap) の選択判定基準** (η-26 で実証):
- **UBO 集約 適用**: bare uniform が main() / 関数で **dead でない** (実 use site あり)、UBO に再構成して構造解決
- **GL-only wrap 適用**: bare uniform が **dead** (use site なし)、または別 UBO で既取得済 = Vulkan path で **不要**、再構成不要

### §3.4 cascade shift forward 第23層 (η-26 で観測)

**範式根拠**: η-26 Phase 1a (shadowAlphaMaskV bare uniform `shadow_target_width` UBO 化) で parser advance → shadowAlphaMaskV.glsl `!HAS_SKIN` Vulkan path で `modelview_projection_matrix` undeclared identifier 露呈。Phase 1a 前は bare uniform parse fail で undeclared check 未到達 → masked。bare uniform 解消で **第23層 (undeclared identifier) 露出**。

**観測** (η-26 第1巡 → 第2巡):
- 第1巡: Phase 1 (3 file 11 件 + 1 dead) deploy → ERROR 68 → 62 (-6)
  - non-opaque 31 → 24 (-7)
  - **undeclared identifier `modelview_projection_matrix` 0 → 2 (第23層 cascade、Deferred Shadow Alpha Mask / Deferred Shadow Fullbright Alpha Mask)**
  - compilation terminated 0 → 2 (cascade 派生)
  - missing #endif 3 → 5 (+2 phantom cascade)
- 第2巡 (Phase 1d cascade fix): shadowAlphaMaskV.glsl L60-80 1 file edit (FrameViewProj guard wrap、symmetric 化) → ERROR 62 → 54 (-8)
  - undeclared identifier 2 → 0 ✓ 第23層完封
  - compilation terminated 2 → 0 ✓ 完封
  - missing #endif 5 → 3 ✓ η-25 baseline 復帰
  - non-opaque 24 維持 (cascade 別カテゴリ)
  - ERROR 予測 -6 に対し実 -8 で parse 失敗 summary 行連鎖減 -2

**範式遵守 (η-27)**:
- non-opaque 24 件残存 = 残 ~16 file 同種 bare uniform 集約 (spotLightF / pbralphaV / postDeferredNoDoFF / pbrterrainV / cofF / blurLightF / pointLightF / volumetricLightF / godraysF / waterF / postDeferredHQDoFF / postDeferredVisualizeBuffers / shadowCubeV / waterHazeV / velocityAlphaV / fsObjectIDF / FXAA 4 段、η-3 §3.2 範式継続) を η-27 主軸候補に
- 残 cascade exposure 観測点: SPIR-V layout validation / pipeline cache / runtime binding 等は parse 大量解消後の第24層候補
- missing #endif 3 件は phantom 系 (η-22 で確認済 false positive) として η-27 でも継続 OK

### §3.5 prefer-cold-launch-verify 範式 (η-26 = 1 cycle 2 巡完走、AYA judgment 2 段階受領 + cascade fix 確認)

η-26 では:
1. AYA cold launch で η-25 末 baseline 取得済 (`/home/ishikawa/.ayastorm_x64/logs/AYAstorm.log` 起動 2026-06-02T12:36:23Z 直前 baseline)
2. **handoff §8.1 14 ステップ trace literal 実施** (ERROR 68 件を category × file 別に系統整理、ROI 順 broadcast 候補 top 3 提示)
3. **AYA に scope 候補 A/B/C 提示**: 案 A literal 受領 (non-opaque top 3 file 集約)
4. **fullbrightShinyV cascade 先制 trace**: `origin` の use site 全 file grep verify → dead 確定報告 + A2 修正案提示
5. **AYA「A2 修正案で進めてください」literal 受領** → Phase 1 (3 file 11 件 + 1 dead) edit + deploy → cold launch verify
6. **第1巡 verify**: non-opaque 31 → 24 (-7) ✓、ERROR 68 → 62 (-6)、**undeclared identifier 0 → 2 (第23層 cascade)**、compilation terminated 0 → 2 (派生)、missing #endif 3 → 5 (+2)
7. **AYA に第23層 cascade 報告 + Phase 1d cascade fix 案提示** (handoff doc §3.1 提示済 = shadowAlphaMaskV L60-80 FrameViewProj guard wrap)
8. **fresh context へ引継 handoff doc (eta-26-phase1-partial.md) 起草** = AYA judgment 受領後の Phase 1d edit を fresh context で実施するため
9. **fresh context 再開**: handoff doc + η-25-complete + reference-shader-location-map + baseline log 4 file Read → AYA「Phase 1d cascade fix で進めて良いですか?」literal 確認
10. **AYA「OK」literal 受領** → shadowAlphaMaskV.glsl L60-80 edit (FrameViewProj guard wrap、symmetric 化) + deploy → cold launch verify
11. **第2巡 verify**: undeclared identifier 2 → 0 ✓、compilation terminated 2 → 0 ✓、missing #endif 5 → 3 ✓、Phase 1d cascade 完封、ERROR 62 → 54 (-8、予測 -6 で実 -8 連鎖減で予測超)
12. patch commit + handoff doc 起草

**feedback_one_step_at_a_time 範式遵守**: 1 巡毎に cold launch verify、cascade 露出は次 judgment 起点として AYA に渡す。**feedback_no_scope_shrink 範式遵守**: Phase 1d cascade fix を「同 sub-bundle 内で完封」literal で確定、η-27 移管しない。**feedback_admit_unknown 範式遵守**: 第23層 cascade (+6 ERROR) を「予測通り」と隠さず正直に報告、ERROR 予測 -6 に対し実 -8 を「予測超 parse 失敗 summary 行連鎖減」と後付け解析として明示。**feedback_doubt_self_first 範式遵守**: fullbrightShinyV cascade 先制 trace で origin dead を「事前 use site grep verify」で確定、A 案でなく A2 修正案を AYA judgment 起点に提示。**feedback_proactive_handoff 範式遵守**: Phase 1 完遂後、context 圧迫前に handoff doc (phase1-partial) 起草で fresh context へ能動的引継。

---

## §4 patch 内容 (3 shader file)

### §4.1 Phase 1a: shadowAlphaMaskV.glsl `shadow_target_width` PerProgramUBO 集約

```glsl
#ifdef LL_VULKAN_GLSL
#ifndef PER_PROGRAM_UBO_SHADOW_ALPHA_MASK_V_DEFINED
#define PER_PROGRAM_UBO_SHADOW_ALPHA_MASK_V_DEFINED 1
layout(set=2, binding=6, std140) uniform PerProgramUBO_ShadowAlphaMaskV {
    float shadow_target_width;
    float _pad_sam0;
    float _pad_sam1;
    float _pad_sam2;
};
#endif
#else
uniform float shadow_target_width;
#endif
```
- 影響 program: Deferred Shadow Alpha Mask / Skinned Deferred Shadow Alpha Mask / Deferred Shadow Fullbright Alpha Mask / Skinned Deferred Shadow Fullbright Alpha Mask = 4 program 共通

### §4.2 Phase 1b: postDeferredV.glsl `tc_scale` PerProgramUBO 集約

```glsl
#ifdef LL_VULKAN_GLSL
#ifndef PER_PROGRAM_UBO_POST_DEFERRED_V_DEFINED
#define PER_PROGRAM_UBO_POST_DEFERRED_V_DEFINED 1
layout(set=2, binding=7, std140) uniform PerProgramUBO_PostDeferredV {
    vec2  tc_scale;
    float _pad_pd0;
    float _pad_pd1;
};
#endif
#else
uniform vec2 tc_scale;
#endif
```
- 影響 program: FXAA Low / Medium / High / Ultra = 4 program 共通 (broadcast 効果大)

### §4.3 Phase 1c: fullbrightShinyV.glsl `texture_matrix1` PerProgramUBO 集約 + `origin` GL-only wrap

```glsl
#ifdef LL_VULKAN_GLSL
#ifndef PER_PROGRAM_UBO_FULLBRIGHT_SHINY_V_DEFINED
#define PER_PROGRAM_UBO_FULLBRIGHT_SHINY_V_DEFINED 1
layout(set=2, binding=8, std140) uniform PerProgramUBO_FullbrightShinyV {
    mat4 texture_matrix1;
};
#endif
#else
uniform mat4 texture_matrix1;
#endif

// origin (dead uniform)
#ifndef LL_VULKAN_GLSL
uniform vec4 origin;
#endif
```
- 影響 program (texture_matrix1): Deferred Fullbright Shiny / Skinned Deferred Fullbright Shiny / HUD Fullbright Shiny = 3 program 共通
- origin: 全 file 内 use site grep verify で dead 確定、η-23 §3.1 範式類

### §4.4 Phase 1d: shadowAlphaMaskV.glsl `!HAS_SKIN` 側 FrameViewProj guard wrap (第23層 cascade fix)

```glsl
// shadowAlphaMaskV.glsl L60-80 (HAS_SKIN false 側、symmetric 化)
#else
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-26 Phase 1d: !HAS_SKIN 側 FrameViewProj guard wrap (η-5 範式類)
#ifndef FRAME_VIEW_PROJ_DEFINED
#define FRAME_VIEW_PROJ_DEFINED 1
layout(set=0, binding=0, std140) uniform FrameViewProj {
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    mat4 projection_matrix;
    mat4 inv_proj;
    mat4 proj_mat;
    mat4 last_modelview_matrix;
    mat3 env_mat;
    mat3 normal_matrix;
    vec2 screen_res;
};
#endif
#else
uniform mat4 modelview_projection_matrix;
#endif
#endif
```
- 第23層 cascade 露出根拠: Phase 1a で shadow_target_width UBO 化 → parser advance → `!HAS_SKIN` Vulkan 経路で modelview_projection_matrix declare 抜け落ち露呈 (L1029=0:413 / L1043=0:410)
- symmetric 化: HAS_SKIN 側 L39-58 と完全同型 (元状態 L60-64 = `#ifndef LL_VULKAN_GLSL uniform mat4 modelview_projection_matrix; #endif`)
- 影響 program: Deferred Shadow Alpha Mask (vert L410) / Deferred Shadow Fullbright Alpha Mask (vert L413) = 2 program cascade 完封

---

## §5 cascade shift forward 第23層露出 + η-27 移管計画

### §5.1 第23層露出観測 (η-26 内で fix 済み)

- **undeclared identifier 'modelview_projection_matrix'** (第23層): Phase 1a bare uniform UBO 化で parser advance → shadowAlphaMaskV.glsl `!HAS_SKIN` Vulkan path で modelview_projection_matrix declare 抜け落ち露呈 → Phase 1d 同 sub-bundle 内で完封 (1 file FrameViewProj guard wrap、HAS_SKIN 側と symmetric 化)
- **compilation terminated** (cascade 派生): undeclared identifier 露出時に parse 強制終了で派生、Phase 1d 完封で連鎖消滅
- **missing #endif** (cascade phantom): Phase 1d 完封で η-25 baseline 復帰 (5 → 3、-2)

### §5.2 η-27 移管予測 cascade

- **non-opaque uniforms 24 件残存**: Phase 1 で shadowAlphaMaskV / postDeferredV / fullbrightShinyV = 3 file 集約済、残 ~16 file 同種 bare uniform 集約候補 (spotLightF / pbralphaV / postDeferredNoDoFF / pbrterrainV / cofF / blurLightF / pointLightF / volumetricLightF / godraysF / waterF / postDeferredHQDoFF / postDeferredVisualizeBuffers / shadowCubeV / waterHazeV / velocityAlphaV / fsObjectIDF / FXAA 4 段、η-3 §3.2 PerDrawUBO 範式継続)
- **missing #endif 3 件残存**: phantom 系 (η-22 で確認済 false positive)、η-27 でも先行 fix cascade 連動消滅予想
- **compilation errors. No code generated**: η-27 で再計測 (η-26 第2巡では未測)
- **第24層 emergence 予想**: parse 大量解消後の SPIR-V layout validation / pipeline cache / runtime binding 段階

---

## §6 risks (η-25 §6 継承 + 新規)

### §6.1 継承 risks (不変)

- **GL path 維持**: `#else` 経路で legacy `uniform <type> <name>;` 残置、Linux/Mac/Win 3 OS GL 動作影響なし
- **C++ binding 影響なし**: UBO 内 member 化は SPIR-V interface 側のみ、CPU side `glUniform*()` / `glGetUniformLocation()` binding 不変
- **既存 GL 動作 zero regression** (set=2 namespace は Vulkan path 専用)

### §6.2 η-26 新規 risks (確認済)

- **fullbrightShinyV origin dead 判定**: 全 file 内 use site grep verify で dead 確定、η-23 §3.1 範式類で GL-only wrap、リスク最小
- **shadowAlphaMaskV symmetric 化**: HAS_SKIN 側 L39-58 と完全同型で構造的整合、`FRAME_VIEW_PROJ_DEFINED` guard で同一 program family 内重複回避
- **per-program UBO 命名衝突**: PerProgramUBO_ShadowAlphaMaskV / PostDeferredV / FullbrightShinyV は set=2 binding=6/7/8 で各 program family に閉じる = global namespace 衝突なし、但し η-27+ で同名再使用しないよう binding 連番管理継続 (binding=9+ 開始)
- **shader_cache 不整合**: η-26 patch 後の cold launch で cache clear 実施済、3 OS shader_cache directory hash 不整合は cold launch 必須運用で回避

### §6.3 η-27 移管想定 risks

- **non-opaque 24 件 file 別棚卸し**: 残 ~16 file の bare uniform 集約は file 毎に per-program UBO 設計判断が必要 = AYA judgment 経由必須
- **第24層 emergence**: parse 大量解消後の link/binding 段階 error 露出予想

---

## §7 observability

### §7.1 cold launch verify protocol

- baseline: `/home/ishikawa/.ayastorm_x64/logs/AYAstorm.log`
- post-deploy: 同 path (cache clear → cold launch → 起動直後 log slice)
- Δ 表生成: `grep -cE "<pattern>"` で category 別カウント + literal grep で内容確認
- clean shutdown 確認: `Quitting` set + `Exiting main_loop` literal

### §7.2 η-26 verify 実績

- 1巡 (Phase 1 = 3 file deploy): ERROR 68 → 62 = -6、non-opaque -7、第23層 cascade +6
- 2巡 (Phase 1d cascade fix = 1 file deploy): ERROR 62 → 54 = -8、第23層 cascade 完封 (-6)、summary 行連鎖減 (-2)
- 累積: ERROR 68 → 54 = -14
- clean shutdown 2 巡とも維持

### §7.3 η-27 着手前 baseline

- canonical baseline: 本 doc 完成時点の `/home/ishikawa/.ayastorm_x64/logs/AYAstorm.log` 起動 2026-06-02T13:17:00Z セッション (Phase 1d deploy 後 cold launch)

---

## §8 引継 scope 推奨 (B?-η-27)

### §8.1 着手前 trace (14 ステップ ベースで η-27 適応)

1. AYA 「コマンド + ビルド全権」+「verify は 1 ステップずつ」運用継承確認
2. η-26 末 baseline log (`/home/ishikawa/.ayastorm_x64/logs/AYAstorm.log` 起動 2026-06-02T13:17:00Z) を canonical baseline として extract
3. ERROR 54 件を **カテゴリ別 + program 別** に系統整理
4. **主 scope 候補 A**: parse failed `non-opaque uniforms outside a block` 24 件継続解消 (件数最大、η-3 §3.2 PerDrawUBO 範式継続適用、file 別棚卸し: 残 ~16 file の bare uniform group)
5. **副 scope 候補 B**: parse failed `missing #endif` 3 件 phantom (η-22 確認済 false positive、η-27 で先行 fix cascade 連動消滅予想)
6. **副 scope 候補 C**: η-26 §5.2 移管予測 cascade の事前 trace (location reassign 帯の事前 grep verify を範式化、reference-shader-location-map.md §5 チェックリスト)
7. **A/B/C どれを Phase 1 主 scope に置くか + 副 scope を η-27 内 Phase 2 として同梱するか別 sub-bundle に分離するか** = AYA judgment 候補
8. **non-opaque 24 件 file 別棚卸し** (η-27 主 scope 候補 A): どの file から優先的に範式適用するか、ROI 最大の file 群を最初に (η-26 末 phase1-partial doc §6.1 表参照、ROI top 3 = spotLightF / pbralphaV / postDeferredNoDoFF)
9. **agent 報告 = 仮説扱いで literal 検証必須** (η-23 §3.5 教訓継承)
10. transformer Phase 3 拡張 (V stage bare `in` attribute 自動 location emit) は **将来 SPIR-V loc 大量発覚時 or 限定的に手動 layout 付与 で対応** を AYA judgment 候補
11. transformer version tag は η-18 末 `v5_p2_inout_pair_prepass_group_fix` 据置、η-27 で Phase 3 拡張時 `v6_p3_vertex_attribute` bump 必須
12. cinematic_bd directory audit (η-17 §10.4 継承) を η-27 着手前 or 完遂後の別 phase として継続推奨
13. **feedback_admit_unknown / feedback_one_step_at_a_time / feedback_no_scope_shrink / feedback_doubt_self_first 範式継承** で η-27 内でも literal observation 優先 + AYA judgment 受領後の scope 全実装 + 事前 trace 範式化
14. **link 段階 ZERO 連続維持** (η-21 → η-22 → η-23 → η-24 → η-25 → η-26 = 6 sub-bundle 連続)、η-27 で 7 sub-bundle 連続目標

### §8.2 想定 Phase 構成 (η-27)

η-27 着手前 trace + AYA judgment で確定だが、現時点 推定:
- **Phase 1 (主 scope 候補 A 推奨)**: non-opaque uniforms 24 件 cascade 解消 (η-3 §3.2 範式継続)、file 別棚卸しでバッチ処理 (ROI top 3 = spotLightF (2 prog × 14 var) / pbralphaV (2 prog) / postDeferredNoDoFF (2 prog) 同梱、計 6 program 解消、set=2 binding=9+ 連番継続)
- **Phase 2 (副 scope)**: η-26 §5.2 cascade 事前 trace (location reassign 帯の事前 grep verify を範式化)、missing #endif 3 件 phantom (先行 fix cascade 連動消滅予想)

### §8.3 B?-η-27 完遂後の想定 cascade exposure 第24層

- non-opaque 24 件解消想定 = parse 段階大量解消、第24層 emergence 観測点: pipeline cache / runtime binding / SPIR-V layout validation 等
- 残 missing #endif 3 件 phantom = η-27 で先行 fix cascade 連動消滅 or η-28+ 移管
- shader_cache 件数: η-27 で計測継続 + 第24層 emergence 観測

---

## §9 commit message (記録)

patch commit `a26b5b8ba7`:
```
feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-26 完遂

Phase 1 (3 file UBO 集約、η-3 §3.2 PerDrawUBO 範式継承):
- shadowAlphaMaskV.glsl: bare uniform `shadow_target_width` を
  PerProgramUBO_ShadowAlphaMaskV (set=2, binding=6) に集約。
  4 program (Deferred / Skinned Deferred / Deferred Fullbright /
  Skinned Deferred Fullbright Shadow Alpha Mask) 共通。
- postDeferredV.glsl: bare uniform `tc_scale` を
  PerProgramUBO_PostDeferredV (set=2, binding=7) に集約。
  FXAA 4 段 (Low / Medium / High / Ultra) 共通。
- fullbrightShinyV.glsl: bare uniform `texture_matrix1` を
  PerProgramUBO_FullbrightShinyV (set=2, binding=8) に集約。
  3 program (Deferred / Skinned Deferred / HUD FullbrightShiny) 共通。
- fullbrightShinyV.glsl: bare uniform `origin` (dead) を
  GL-only `#ifndef LL_VULKAN_GLSL` wrap (η-23 §3.1 範式類)。

Phase 1d (cascade exposure、shadowAlphaMaskV !HAS_SKIN Vulkan gap fix):
- shadowAlphaMaskV.glsl L60-80 `#else (!HAS_SKIN)` ブロックに
  FrameViewProj guard wrap (η-5 範式類) を追加。
  Phase 1 で shadow_target_width UBO 化 → parser advance →
  `!HAS_SKIN` Vulkan 経路で modelview_projection_matrix
  undeclared identifier 露呈、L410/L413 cascade (Deferred Shadow
  Alpha Mask / Deferred Shadow Fullbright Alpha Mask) 完封。
  HAS_SKIN 側 L39-58 と symmetric 化。

検証 (cold launch log、2 巡):
- ERROR: 68 (η-25末) → 62 (1巡) → 54 (2巡) = -14 累積
  (予測 -12 = -11 non-opaque + Phase 1d -6、実 -14 で
   parse 失敗 summary 行も連鎖減 -2 で予測超)
- non-opaque uniforms: 31 → 24 = -7 (Phase 1 で 11 件解消、
  cascade 別カテゴリ計上)
- missing #endif: 3 → 5 (1巡 cascade +2) → 3 (2巡 復帰)
- undeclared identifier (modelview_projection_matrix): 0 → 2 → 0
  ✓ Phase 1d 完封
- compilation terminated: 0 → 2 → 0 ✓ Phase 1d 完封
- glslang link failed for program: 0 → 0 ✓ link 段階 ZERO 6
  sub-bundle 連続維持 (η-21 milestone 継承)
- overlapping use of location: 0 → 0 ✓ η-25 完封維持
- shadow_clip redef: 0 → 0 ✓ η-25 完封維持
- clean shutdown 維持 (Quitting + Exiting main_loop 2 hits)

set=2 namespace 連番継続: η-3 binding=0 LightParams /
η-23 binding=1 MultiLight / η-24 binding=2 GammaCorrect /
η-25 binding=3 AlphaParams / binding=4 ColorGrading /
binding=5 PointLightV / η-26 binding=6 ShadowAlphaMaskV /
binding=7 PostDeferredV / binding=8 FullbrightShinyV

予測 cascade (η-27 以降):
- non-opaque 24 件残存 = 残 ~16 file 同種 bare uniform 集約
  (spotLightF / pbralphaV / postDeferredNoDoFF / pbrterrainV /
   cofF / blurLightF / pointLightF / volumetricLightF / godraysF /
   waterF / postDeferredHQDoFF / postDeferredVisualizeBuffers /
   shadowCubeV / waterHazeV / velocityAlphaV / fsObjectIDF / FXAA 4 段、
   η-3 §3.2 PerDrawUBO 範式継続)
- 第24層 emergence 観測点: SPIR-V layout validation /
  pipeline cache / runtime binding 段階
```

handoff doc commit message 案:
```
docs(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-26-complete handoff 起草

main scope = Phase 1 bare uniform → per-program UBO 集約 (3 file 11 件
+ 1 dead GL-only wrap) + Phase 1d 第23層 cascade fix (shadowAlphaMaskV
!HAS_SKIN Vulkan FrameViewProj guard wrap、HAS_SKIN 側と symmetric 化)
の完遂 handoff doc。set=2 namespace を binding=6/7/8 に拡張 (η-3 §3.2
PerDrawUBO 派生範式継承)。

主指標達成:
- Phase 1 で 11 non-opaque ERROR 解消 (shadowAlphaMaskV 4 / postDeferredV 4 /
  fullbrightShinyV 3)
- 第23層 cascade (undeclared modelview_projection_matrix /
  compilation terminated / missing #endif) 0 → 6 → 0 (Phase 1d 完封)
- ERROR 計 68 → 62 → 54 (-14 累積、予測 -12 で実 -14 で summary 行連鎖減)
- non-opaque uniforms 31 → 24 (-7)
- glslang link failed for program 0 → 0 (link 段階 ZERO 6 sub-bundle
  連続維持)
- clean shutdown 維持
- 16+ 種既達主指標完全維持

cascade shift forward 第23層 (η-26 内で fix 済み):
- Phase 1a で bare uniform UBO 化 → parser advance →
  shadowAlphaMaskV !HAS_SKIN Vulkan path で undeclared identifier 露出
  → Phase 1d 同 sub-bundle 内で完封 (1 file FrameViewProj guard wrap、
   symmetric 化)

patch commit: a26b5b8ba7
```

---

**handoff doc 完。次 sub-bundle B?-η-27 着手は本 doc §8 推奨 scope (parse non-opaque 24 件主軸 + missing #endif 3 件 + 事前 trace 範式化) を起点として、fresh context で実施。**
