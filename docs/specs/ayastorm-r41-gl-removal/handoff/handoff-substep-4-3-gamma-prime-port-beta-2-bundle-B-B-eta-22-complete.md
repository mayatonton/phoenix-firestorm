# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-22 完遂 handoff

**status**: B?-η-22 完遂 (Phase 1 A ACCEPT + Phase 2 B ACCEPT、env_mat redeclare 16 件 + SPIR-V requires location 3 件 = **計 19 件 構造的完全解消**、glslang link failed for program **0 → 0** = **link 段階 ZERO 維持** (η-21 milestone 継承)、ERROR 計 154 → 139 (-15 = -19 + 4 cascade exposure 第20層: non-opaque +3 / shadow_clip redef +1)、clean shutdown 維持) → 次 sub-bundle B?-η-23 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: `5cc4b03bab` (4 shader file 編集、+35/-0 行、C++ 変更 0、user_settings.xml 変更 0)
**handoff doc commit**: 本 doc (η-21-complete 範式継承、別 commit)
**勤続範式継承**: B?-η-21-complete `989b529936` (patch `e78ca6006c`) / B?-η-20-complete `d7c722f4e7` (patch `557cd1db00`) / B?-η-19-complete `4911401568` (patch `9349ace5f0`) / B?-η-18-complete `1371da9660` (patch `4b42779cc7`) / B?-η-17-complete `cc2e878c7e` (patch `e2d4d3bbca`) / B?-η-16-complete `3653efed00` (patch `72fd4f3c3c`) / B?-η-16-prep-D-switch `1c3eb16b6b` / B?-η-15-complete `cb28cf1daa` (patch `6a11eabc73`) / B?-η-14-complete `78df235243` (patch `c971838656`) / B?-η-13-reverted (no commit) / B?-η-12-complete `7c1762d214` / B?-η-11-complete `74705c35bb` / B?-η-10-complete `9246142639` / B?-η-9-complete `6fee4a818b` / B?-η-8-complete `6994eba271` / B?-η-7-complete `b1e8689634` / B?-η-6-complete `fe757ea624` / B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-22 scope = **Phase 1 (A: env_mat redeclare 16 件) + Phase 2 (B: SPIR-V requires location 3 件)** を同一 sub-bundle で同梱解消。AYA judgment 「推奨で OK」literal 受領後の 1 cycle で完走、2 Phase 同梱 commit。η-21 link 段階 ZERO milestone を維持しつつ第19層 (Phase 1 主候補 = env_mat) + 第19層副 (Phase 2 副候補 = SPIR-V loc) を構造的完全解消。

- **Phase 1 (A: env_mat redeclare 16 件、legacy bare uniform GL-only wrap 範式)**:
  - **適用 (multiPointLightF.glsl L45)**: bare `uniform vec3 env_mat[3];` を `#ifndef LL_VULKAN_GLSL / uniform vec3 env_mat[3]; / #endif` で GL path 限定 wrap。Vulkan path では FrameViewProj UBO `mat3 env_mat` (L62) との redeclare 衝突 (cannot redeclare a user-block member array) を回避。multiPointLightF.glsl main() 内 env_mat 未参照 (dead in this file)。pointLightF.glsl L40-42 と同範式統一 (pointLightF.glsl は同範式で η-21 末 parse 通過済 = 実証)。
  - 影響: Deferred MultiLight Shader 0-15 全 16 件の parse error 解消。

- **Phase 2 (B: SPIR-V requires location 3 件、bare vertex attribute Vulkan location 付与範式)**:
  - **適用 (blurLightV.glsl L26 / godraysV.glsl L18 / fsObjectIDV.glsl L38)**: 3 vert shader の bare `in vec3 position;` を `#ifdef LL_VULKAN_GLSL / layout(location=0) in vec3 position; / #else / in vec3 position; / #endif` で Vulkan layout 付与 wrap。multiPointLightV.glsl L26-30 と同範式統一。VBO TYPE_VERTEX=0 で attribute index 0 固定 = layout(location=0) 安全。
  - 影響: Deferred Blur Light Shader / Godrays Shader / FS Object ID Shader 計 3 件 parse error 解消。

**計**: **shader 4 file 編集 (+35/-0 = +35)、C++ 変更 0、user_settings.xml 変更 0**

**主指標達成** (vs η-21 末 baseline log):

- `env_mat cannot redeclare a user-block member array` (parse) 16 → **0** ✓ **完全解消 (Phase 1 A 主目標達成)**
- `SPIR-V requires location for user input/output` (parse) 3 → **0** ✓ **完全解消 (Phase 2 B 主目標達成)**
- `glslang link failed for program` 0 → **0** ✓ **link 段階 ZERO 維持 (η-21 milestone 継承)**
- ERROR 計 154 → **139** **-15 (= -19 構造解消 + 4 cascade exposure 第20層)**
- `Goodbye!` 1 / `Vulkan device destroyed` 1 / FATAL/SIGSEGV/Aborted 0/0/0 = ✓ clean shutdown 維持

**cascade shift forward 第20層露出** (η-22 で 19 件解消した結果、第20層が露出):

| # | metric | η-21末 | η-22後 | Δ | 解析 |
|---|---|---|---|---|---|
| 1 | `env_mat cannot redeclare` (parse) | 16 | **0** | **-16** | ✓ Phase 1 A 完全解消 |
| 2 | `SPIR-V requires location` (parse) | 3 | **0** | **-3** | ✓ Phase 2 B 完全解消 |
| 3 | `glslang link failed for program` | 0 | **0** | ±0 | ✓ link 段階 ZERO 維持 |
| 4 | `non-opaque uniforms outside a block` (parse) | 48 | **51** | **+3** | 第20層 cascade exposure (Phase 2 fix で blurLightV/godraysV/fsObjectIDV が parse 進行 → 次の non-opaque error 露出) |
| 5 | `'shadow_clip' : redefinition` (parse) | 0 | **1** | **+1** | **第20層 新出** (Godrays Shader vert で Phase 2 副次露出、η-23 主 scope 候補) |
| 6 | `missing #endif` (parse、phantom) | 13 | **13** | ±0 | phantom 維持 (handoff §2.1 で literal observation 済、独立 fix 不可、先行 ERROR fix で同時消滅) |
| 7 | `overlapping use of location` (parse) | 5 | **5** | ±0 | (η-23 副 scope 移管) |
| 8 | `nameless block ... global scope` (parse) | 5 | **5** | ±0 | (η-23 副 scope 移管) |
| 9 | `compilation errors. No code generated` (summary) | 61 | **61** | ±0 | (parse 失敗 program 数 summary 行、独立 fix 不可) |
| 10 | `Anonymous member name used for global variable` (link) | 0 | **0** | ±0 | ✓ η-20 達成維持 |
| 11 | `'shadow_bias' : redefinition` | 0 | **0** | ±0 | ✓ η-20 達成維持 |
| 12 | `'M_PI' : redefinition` | 0 | **0** | ±0 | ✓ η-20 達成維持 |
| 13 | `'weight4' : redefinition` | 0 | **0** | ±0 | ✓ η-20 達成維持 |
| 14 | `Input 'vary_position'` (link) | 0 | **0** | ±0 | ✓ η-21 達成維持 |
| 15 | `MaterialUBO metallicFactor / fragment block` (link) | 0 | **0** | ±0 | ✓ η-21 達成維持 |
| 16 | `sampler binding` (parse) | 0 | **0** | ±0 | ✓ η-21 達成維持 |

**主指標達成総括**: 19 件 (Phase 1 env_mat 16 + Phase 2 SPIR-V loc 3) を **構造的解決** (legacy bare uniform GL-only wrap + bare vertex attribute Vulkan layout 付与) で完全解消。**link 段階 ZERO を η-21 milestone から維持**。cascade shift forward 第20層 non-opaque +3 / shadow_clip redef +1 = **η-23+ 別 sub-bundle 移管対象**。

**他既達主指標完全維持** (η-22 末):
- η-21 達成全項目 (link failed 0 / sampler binding 0 / metallicFactor 0 / vary_position 0)
- η-20 達成全項目 (Anonymous member 0 / shadow_bias / M_PI / weight4 redef 0)
- η-19 達成全項目 (normalMap / depthMap / vary_fragcoord redef 0)
- η-18 達成全項目 (Layout location qualifier 0)
- η-1〜η-17 達成全項目 (Cannot reuse block name 0 / 'binding' 0 / GBufferInfo redef 0 / 'size' undeclared 0 / undeclared identifier 0)

**Phase 構成の特徴**: η-22 は **AYA judgment 「推奨で OK」literal 受領後の 1 cycle 完走** で 2 Phase 同梱解消。Phase 1 (env_mat) は pointLightF.glsl L40-42 で同範式が既に parse 通過実証済 = 最高確実度、1 file 1 fix (3 行追加) で 16 件大量解消の ROI 最大。Phase 2 (SPIR-V loc) は 3 独立 file 構造解消、cascade 影響限定的。**feedback_no_scope_shrink 範式遵守** (A + B 統合 scope 全実装、Phase 3 C は η-23 移管で清潔な分割)。

---

## §2 完遂結果 metric (vs B?-η-21 末 baseline log)

| metric | η-21末 | η-22後 (verify) | Δ vs η-21 | 判定 |
|---|---|---|---|---|
| **env_mat redeclare (parse)** | **16** | **0** | **-16** | ✓ **Phase 1 A 主目標完全達成** |
| **SPIR-V requires location (parse)** | **3** | **0** | **-3** | ✓ **Phase 2 B 主目標完全達成** |
| glslang link failed for program | 0 | 0 | ±0 | ✓ **link 段階 ZERO 維持 (η-21 milestone)** |
| ERROR (total) | 154 | 139 | **-15** | 19 件構造解消 - 4 件第20層 cascade exposure |
| non-opaque uniforms (parse) | 48 | 51 | **+3** | 第20層 cascade (3 vert fix 副次) |
| shadow_clip redefinition (parse) | 0 | 1 | **+1** | **第20層 新出** (Godrays Shader vert で Phase 2 副次) |
| missing #endif (parse、phantom) | 13 | 13 | ±0 | phantom 維持 (independent fix 不可) |
| overlapping use of location (parse) | 5 | 5 | ±0 | η-23 副 scope 移管 |
| nameless block ... global scope (parse) | 5 | 5 | ±0 | η-23 副 scope 移管 |
| compilation errors summary | 61 | 61 | ±0 | parse 失敗 program 数の summary 行 |
| Anonymous member (link) | 0 | 0 | ±0 | ✓ η-20 達成維持 |
| shadow_bias redef (parse) | 0 | 0 | ±0 | ✓ η-20 達成維持 |
| M_PI redef (parse) | 0 | 0 | ±0 | ✓ η-20 達成維持 |
| weight4 redef (parse) | 0 | 0 | ±0 | ✓ η-20 達成維持 |
| normalMap / depthMap / vary_fragcoord redef | 0 | 0 | ±0 | ✓ η-19 達成維持 |
| Layout location qualifier (link error) | 0 | 0 | ±0 | ✓ η-18 達成維持 |
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

**19 件構造的完全解消 (Phase 1 A 16 + Phase 2 B 3) + link 段階 ZERO 維持 + 18 種既達主指標完全維持 + clean shutdown** = **η-22 Phase 1+2 主目標達成**。cascade shift forward 第20層露出 (non-opaque +3 / shadow_clip redef +1) = **η-23 別 sub-bundle 移管対象**。

---

## §3 設計範式

### §3.1 新規 設計範式: legacy bare uniform GL-only wrap 範式 (Phase 1 A 主)

**範式根拠**: Vulkan strict mode (GLSL_KHR_vulkan_glsl) では、bare `uniform vec3 ident[N];` (= legacy non-block uniform) と同名の **UBO 内 member** が同 shader scope に共存すると `ERROR: 'ident' : cannot redeclare a user-block member array` parse error。η-5 で FrameViewProj UBO を導入 (`mat3 env_mat;` を含む) し、Vulkan path で UBO 参照可能化したが、legacy bare uniform `uniform vec3 env_mat[3];` (GL path 互換) が GL path / Vulkan path 共有 declaration として残存 = redeclare 衝突。pointLightF.glsl L40-42 では同範式で `#ifndef LL_VULKAN_GLSL` wrap が既に確立、multiPointLightF.glsl L45 は wrap 未適用で 16 件のすべての Multi-Light Shader 0-15 で parse 失敗していた。

**範式構造**:

```glsl
#ifndef LL_VULKAN_GLSL
// legacy bare uniform (GL path 互換、Vulkan path では UBO member で代替)
uniform <type> <ident>[N];
#endif
```

**有効性根拠**:
1. **redeclare 衝突解消**: Vulkan path で bare declaration 不在 = UBO member だけが visible、redeclare error 解消
2. **GL path 完全非変更**: `#ifndef LL_VULKAN_GLSL` で bare uniform 残置 = C++ binding (legacy uniform 1d/2d/3d/4d/4x4) を変更不要
3. **multi-file 統一**: 既存 wrap (pointLightF.glsl L40-42 / sunLightV.glsl L57 / softenLightV.glsl L43 等) と同範式 = viewer 全体で一貫性
4. **dead code 安全**: multiPointLightF.glsl の env_mat は main() 内未参照 (dead in this file) = wrap 内に置いても外しても機能影響なし、最小編集で最大効果

**範式適用例 (multiPointLightF.glsl L45)**:

```glsl
#ifndef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-22 Phase 1: legacy bare
// `uniform vec3 env_mat[3]` を GL path 限定 wrap。Vulkan path では
// FrameViewProj UBO `mat3 env_mat` (L62) と redeclare 衝突
// (cannot redeclare a user-block member array)。multiPointLightF main()
// 内で env_mat 未参照 (dead in this file)、pointLightF.glsl L40-42 と
// 同範式統一。影響: Deferred MultiLight Shader 0-15 全 16 件 parse error 解消。
uniform vec3  env_mat[3];
#endif
```

**範式の制約**:
- main() 内で bare uniform を参照している場合は UBO member への置換 (`<UBO>.<member>` 形式 or alias) が必要、η-22 では dead code = 制約該当せず
- bare uniform と UBO member が異 type (例: `vec3 ident[3]` vs `mat3 ident`) でも redeclare error 発生 (glslang は名前で判定)

### §3.2 新規 設計範式: bare vertex attribute Vulkan location 付与範式 (Phase 2 B 主)

**範式根拠**: Vulkan strict mode (GLSL_KHR_vulkan_glsl) では、SPIR-V 生成のために vertex shader の **全 in attribute** に `layout(location=X)` が必須。bare `in vec3 position;` は `ERROR: 'location' : SPIR-V requires location for user input/output` parse error。η-18 末 transformer v5 = `p2_inout_pair_prepass_group_fix` は F stage in/out pair を中心に捕捉、V stage bare `in` (= vertex attribute) は未捕捉。η-22 では 3 file (blurLightV / godraysV / fsObjectIDV) で手動 layout 付与により構造解消。

**範式構造**:

```glsl
#ifdef LL_VULKAN_GLSL
layout(location=0) in vec3 position;
#else
in vec3 position;
#endif
```

**有効性根拠**:
1. **SPIR-V 適合**: 全 vertex attribute に location 付与 = SPIR-V VertexInputAttribute 指定が成立、parse error 解消
2. **VBO TYPE_VERTEX=0 一致**: viewer の `LLVertexBuffer::TYPE_VERTEX` は attribute index 0 固定 = layout(location=0) で C++ side との binding 整合
3. **GL path 完全非変更**: `#else` で bare in 残置 = C++ glBindAttribLocation / 自動 binding 影響なし
4. **multi-file 統一**: 既存 wrap (multiPointLightV.glsl L26-30 / pointLightV.glsl L46-50 / sunLightV.glsl L25-29 / softenLightV.glsl L25-29 / waterHazeV.glsl L25-29 等多数) と同範式 = viewer 全体で一貫性

**範式適用例 (blurLightV.glsl L26)**:

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-22 Phase 2: bare vertex
// attribute `in vec3 position` に SPIR-V location 付与
// (multiPointLightV.glsl L26-30 と同範式統一)。Vulkan strict mode では
// 全 user input/output に location 必須。VBO TYPE_VERTEX=0 で attribute
// index 0 固定。影響: Deferred Blur Light Shader 1 件 parse error 解消。
layout(location=0) in vec3 position;
#else
in vec3 position;
#endif
```

**範式の制約**:
- `in vec3 position` 以外の attribute (例: `in vec4 normal;` `in vec2 texcoord0;` 等) も同範式必要、η-22 では position のみ確認、他 attribute は別 sub-bundle で順次対応
- attribute index 番号は `LLVertexBuffer::TYPE_*` enum と整合必須 (position=0 / normal=1 / texcoord0=2 / 等)、η-22 では position=0 のみ確認

### §3.3 prefer-cold-launch-verify 範式 (η-22 = 1 cycle 完走、2 Phase 同梱)

η-22 では:
1. AYA cold launch で η-21 末 baseline 取得済 (`/home/ishikawa/.ayastorm_x64/logs/AYAstorm.log` 起動 2026-06-02T10:24Z 直前 baseline)
2. **handoff §8.1 14 ステップ trace literal 実施**: ERROR 154 件を カテゴリ別 + program 別に系統整理 (env_mat 16 全件 Deferred MultiLight Shader 0-15 / SPIR-V 3 件 vert / non-opaque 48 件 multi-program)
3. **literal observation で構造特定**: pointLightF.glsl L40-42 既存範式 = `#ifndef LL_VULKAN_GLSL` wrap で env_mat 衝突回避が実証済 → multiPointLightF.glsl への同範式適用が確実
4. **A/B/C 候補比較表作成、A + B 統合 scope を Claude 推奨案として AYA judgment 待ち提示**
5. AYA judgment 「推奨で OK」literal 受領後、A + B 2 Phase 同梱で 4 file edit 実施
6. shader-only fast-iterate 範式: `~/ayastorm/app_settings/shaders/` cp + `~/.ayastorm_x64/cache/shader_cache/` clear
7. AYA cold launch verify (第 1 巡) → 19 件全解消確認、link 段階 ZERO 維持、ERROR 計 154 → 139 (-15 = -19 + 4 cascade exposure 第20層) ✓ clean shutdown

η-21 同様、**handoff §8.1 trace + AYA judgment 受領 + 1 cycle 完走** で達成。Phase 1 root cause が 1 file 1 wrap (3 行追加) で済む構造単純さ、Phase 2 が 3 file × 9 行追加で済むことが要因。

### §3.4 cascade shift forward 第20層露出範式 (η-22 で観測)

**範式根拠**: η-21 で link Anonymous member 36 件解消で第18層 sampler binding +16 / missing #endif +13 / fragment block +4 / vary_position +1 露出、η-22 で第19層 env_mat 16 + SPIR-V 3 解消で **第20層**: non-opaque +3 / shadow_clip redef +1 が露出。

**観測** (η-22 末):
- **non-opaque uniforms +3 (51 件)**: Phase 2 fix で blurLightV / godraysV / fsObjectIDV が parse 進行 → 次の non-opaque error が露出 (各 1 件)、η-19 §3.1 範式継続適用候補
- **shadow_clip redefinition +1 (1 件)**: Godrays Shader vert で Phase 2 副次露出、η-20 §3.1 範式類 (UBO 内 member rename + #define alias + #undef scope-limit) で構造解決候補

**範式遵守 (η-23)**:
- 第20層 cascade は限定的 (4 件)、parse 段階主軸は **non-opaque 51 件** (件数最大)
- non-opaque 51 件は η-19 §3.1 shader file level include guard 範式継続適用候補 = η-23 主 scope
- shadow_clip redef 1 件は η-20 §3.1 PerDrawUBO member rename + #define alias + #undef scope-limit 範式類 = η-23 副 scope

---

## §4 patch 内容 (4 shader file)

### §4.1 Phase 1: env_mat redeclare 16 件解消 (1 file 編集、legacy bare uniform GL-only wrap 範式)

#### §4.1.1 class3/deferred/multiPointLightF.glsl L45 (env_mat GL-only wrap)

```glsl
#ifndef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-22 Phase 1: legacy bare
// `uniform vec3 env_mat[3]` を GL path 限定 wrap。Vulkan path では
// FrameViewProj UBO `mat3 env_mat` (L62) と redeclare 衝突
// (cannot redeclare a user-block member array)。multiPointLightF main()
// 内で env_mat 未参照 (dead in this file)、pointLightF.glsl L40-42 と
// 同範式統一。影響: Deferred MultiLight Shader 0-15 全 16 件 parse error 解消。
uniform vec3  env_mat[3];
#endif
```

### §4.2 Phase 2: SPIR-V requires location 3 件解消 (3 file 編集、bare vertex attribute Vulkan location 付与範式)

#### §4.2.1 class1/deferred/blurLightV.glsl L26 (Deferred Blur Light Shader)

```glsl
#ifdef LL_VULKAN_GLSL
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-22 Phase 2: bare vertex
// attribute `in vec3 position` に SPIR-V location 付与
// (multiPointLightV.glsl L26-30 と同範式統一)。Vulkan strict mode では
// 全 user input/output に location 必須。VBO TYPE_VERTEX=0 で attribute
// index 0 固定。影響: Deferred Blur Light Shader 1 件 parse error 解消。
layout(location=0) in vec3 position;
#else
in vec3 position;
#endif
```

#### §4.2.2 class1/deferred/godraysV.glsl L18 (Godrays Shader)

同範式 patch (Deferred Blur Light Shader → Godrays Shader 名のみ差替)。

#### §4.2.3 class1/deferred/fsObjectIDV.glsl L38 (FS Object ID Shader)

同範式 patch (Deferred Blur Light Shader → FS Object ID Shader 名のみ差替)。

### §4.3 cache invalidate

shader file 変更のみ、C++ transformer は v5 据置 (η-18 末)。shader_cache clear で対応:
```bash
rm -rf ~/.ayastorm_x64/cache/shader_cache/*
```

---

## §5 cascade shift forward 第20層後始末計画 (η-23+ 移管)

### §5.1 parse 段階残主要カテゴリ + 推奨 sub-bundle 分割

| カテゴリ | 件数 | 推奨 sub-bundle | 根拠 |
|---|---|---|---|
| **non-opaque uniforms outside a block** | **51** | **η-23 主 scope 候補** | 件数最大、η-19 §3.1 shader file level include guard 範式継続適用、multi-file (Multi-Light Shader 0-15 含む 51 program 跨ぎ) |
| `'shadow_clip' : redefinition` (parse) | 1 | η-23 副 scope | Godrays Shader vert で Phase 2 副次露出、η-20 §3.1 PerDrawUBO member rename + #define alias + #undef scope-limit 範式類で構造解決 |
| `missing #endif` (phantom) | 13 | η-23 副次効果 | 独立 fix 不可、先行 ERROR fix で同時消滅 (handoff §2.1 で確定済) |
| `overlapping use of location` | 5 | η-23 副 scope | η-18 §3.1 範式類 (V↔F slot alignment) |
| `nameless block ... global scope` | 5 | η-23 副 scope | η-1 §3.1 範式類 (block 内 member rename) |
| `compilation errors. No code generated` (summary) | 61 | (parse 失敗 program 数 summary 行、独立 fix 不可) | 先行 ERROR fix で連動減少 |

**合計 net 不重複 ERROR**: 約 75 件 (51 + 1 + 13 + 5 + 5、summary 61 除外)。

### §5.2 link 段階 ZERO 維持の milestone 観測

η-21 で達成した link 段階 ZERO を η-22 でも維持。第20層は **parse-stage 単一段階** に絞られた状態 = η-23+ で parse-stage 集中攻略が可能。η-22 では link 段階退行 0 件達成。

### §5.3 transformer Phase 3 拡張は将来 SPIR-V 要 location 解消 と連動

- transformer version tag は η-18 末 `v5_p2_inout_pair_prepass_group_fix` 据置、η-23 で Phase 3 拡張時 `v6_p3_vertex_attribute` bump 必須
- η-22 で 3 file 手動対応した SPIR-V loc は限定的、将来同類問題発覚時は transformer Phase 3 拡張 (V stage bare `in` attribute 自動 location emit) で根本解決候補

### §5.4 non-opaque uniforms 51 件の構造調査 (η-23 主 scope 推定)

η-19 §3.1 shader file level include guard 範式類で構造解決:
- 51 program 跨ぎの bare `uniform <type> <ident>;` を file 別に棚卸し
- 範式適用候補: `#ifdef LL_VULKAN_GLSL` で UBO member 参照に置換 (η-20 §3.1 範式継承) or `#ifndef LL_VULKAN_GLSL` で GL-only wrap (η-22 §3.1 範式継承)
- ROI 最大の file (Multi-Light Shader 系 等の 16 件束) から優先順位付け

### §5.5 shadow_clip redef 構造調査 (η-23 副 scope 推定)

**第 1 候補 (推奨)**: Godrays Shader vert (godraysV.glsl 経由) で shadow_clip 宣言が atmospherics include 系と衝突。η-20 §3.1 範式類 (PerDrawUBO member rename + #define alias + #undef scope-limit) で構造解決。

---

## §6 risks (η-21 §6 継承 + 新規)

| # | risk | 対応状態 | 引継 sub-bundle |
|---|---|---|---|
| 1 | C++ runtime transformer が GL path で influence する | ✓ kill-switch で完全 bypass、η-16 から継承 | 引継不要 |
| 2 | preprocessor 別分岐の bare/manual-wrap 共存 | ✓ η-18 で再利用優先範式に修正済 | 引継不要 |
| 3 | regex group index / kQuals non-capturing 不整合 | ✓ η-18 で修正済 | 引継不要 |
| 4 | shader file level guard が異 type 同名 varying program で衝突 | ✓ η-19 で確認済 | 引継不要 |
| 5 | PerDrawUBO member rename + #define alias の関数 param/local var 共存問題 | ✓ η-20 §4.1.2 で verify 済 | 引継不要 |
| 6 | #undef scope-limit が file 末尾で対応 = file 内全 use site が rewrite | ✓ η-20 で意図通り | 引継不要 |
| 7 | cascade shift forward 第19層大量露出 (env_mat +16 / SPIR-V +3 / non-opaque +16) | ✓ η-22 で env_mat 16 件 + SPIR-V 3 件構造解消 | 第20層 (non-opaque +3 / shadow_clip +1) は η-23 で対応 |
| 8 | sampler binding 番号 conflict (set=0, binding=5 統一原則) | ✓ η-21 で実装、verify 済 | η-23+ で新規 sampler wrap 時は既存 binding 統一原則継承 |
| 9 | V/F MaterialUBO member alignment が複数 file 跨ぎで異 layout を持つ可能性 | ✓ η-21 で 4 declarations 全件同 layout verify 済 | 引継不要 |
| 10 | dead vary_position 削除で同 ident 異 path 残置 (pbropaqueF L79 vs L315) | ✓ η-21 で literal trace 確認、verify 済 | 引継不要 |
| 11 | legacy bare uniform GL-only wrap で main() 参照ありの場合は UBO member 置換必要 | ✓ η-22 で multiPointLightF env_mat = dead in this file 確認、wrap で済 | η-23+ で同範式適用時は main() 内参照 trace literal 確認必須 |
| 12 | bare vertex attribute layout(location=X) で C++ binding (LLVertexBuffer::TYPE_*) との整合 | ✓ η-22 で position=0 = TYPE_VERTEX=0 verify 済 | η-23+ で他 attribute (normal/texcoord0 等) 対応時は同範式継承 + index 整合 |
| 13 | upstream OpenGL Firestorm merge 時 shader file 互換 (legacy bare uniform GL-only wrap / bare attribute Vulkan layout 範式は標準 GLSL) | 互換性高 | sub-step 4.5 |
| 14 | cinematic_bd overlay mirror | η-22 では touch なし、cinematic_bd directory 内 4 file 不在を Glob で確認済 (η-21 と同) | 引継不要 |
| 15 | non-opaque uniforms 51 件 cascade = η-23 主 scope 候補、multi-file で取りこぼし risk | η-19 §3.1 範式継続適用予定 | η-23 着手時に file 別棚卸し優先 |
| 16 | shadow_clip redef 1 件 = Godrays Shader vert 特定済 | η-20 §3.1 範式類で構造解決候補 | η-23 副 scope |
| 17 | Phase 1+2 同梱 1 cycle 完走 = AYA cold launch verify 第 2 巡不要、η-22 milestone link 段階 ZERO 維持 | feedback_one_step_at_a_time 範式遵守 | 引継不要 |
| 18 | handoff §5.1/§8.2 の 推奨案 (A + B 同梱) を Claude trace で確定 | literal observation 範式遵守、AYA に正直 report 済 | η-23+ で同類 trace 継続 |

---

## §7 observability

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 件数 (η-7 305 baseline、η-11 310、η-20 末 421 件 transformed dump + 342 件 cache 合計、η-21 同) | η-22 末 verify 後 421 件超 transformed dump 維持、cold launch 後 cache 合計 計測継続 | B?-η-23 で baseline 更新 |
| 2 | C++ runtime location emit 範式 (η-16 §3.1) Phase 2 (V↔F pair) 維持 | ✓ η-22 で touch なし、v5 据置 | B?-η-23 で SPIR-V loc 追加対応時 Phase 3 拡張候補 |
| 3 | shader file level include guard 範式 (η-19 §3.1) 適用範囲 | η-22 では touch なし | B?-η-23 で non-opaque 51 件 cascade 適用候補 |
| 4 | PerDrawUBO member rename + #define alias + #undef 範式 (η-20 §3.1) | η-22 では touch なし | B?-η-23 で shadow_clip redef 同範式適用候補 |
| 5 | bare opaque uniform Vulkan binding 付与範式 (η-21 §3.1) | η-22 では touch なし、η-21 で 1 件適用済 | B?-η-23 で他 bare sampler / texture / image 残時に同範式継承 |
| 6 | V/F UBO member alignment 範式 (η-21 §3.2) | η-22 では touch なし、η-21 で 4 declarations 適用済 | B?-η-23 で他 UBO V/F alignment 必要時に同範式継承 |
| 7 | dead F-side declaration 削除範式 (η-21 §3.3) | η-22 では touch なし、η-21 で 2 file 適用済 | B?-η-23 で他 dead in 検出時に同範式継承 (literal trace 確認必須) |
| 8 | legacy bare uniform GL-only wrap 範式 (η-22 §3.1 新規) | η-22 で multiPointLightF.glsl env_mat 1 件適用 | B?-η-23 で他 bare uniform × UBO member redeclare 衝突発覚時に同範式継承 |
| 9 | bare vertex attribute Vulkan location 付与範式 (η-22 §3.2 新規) | η-22 で blurLightV / godraysV / fsObjectIDV = 3 file 適用 | B?-η-23 で他 vert shader bare in/attribute 検出時に同範式継承 (VBO TYPE_* index 整合必須) |
| 10 | F stage 既存 layout in 自動 override 12 件継続 (LL_DEBUGS) | 観測継続 | B?-η-23 で残発見時継続観測 |
| 11 | dump 機構 (η-16 §3.1 内包) η-22 で 421 件超出力 | 維持 | B?-η-23 で必要に応じて再投入 |
| 12 | falsification iterate 範式 (η-18 内 v2→v5 4 段) | η-22 は AYA judgment 受領後 1 cycle 完走、falsification iterate 不要 | B?-η-23 で C++ transformer Phase 3 拡張時は falsification iterate 想定 |
| 13 | 既達主指標完全維持 (η-22 で 18 種維持 + 19 件構造的解消、退行 0 件) | ✓ η-22 で退行 0 件達成 | B?-η-23 で第20層 cascade 残系統 scope |
| 14 | cinematic_bd directory 全 .glsl audit (η-17 §10.4 継承) | η-22 で touch なし、η-21 末 audit 維持 | B?-η-23+ 着手前 or 完遂後の別 phase として継続推奨 |
| 15 | feedback_self_verify_before_handoff 適用 | η-22 で trace + literal observation 14 ステップ + AYA cold launch 1 巡で 19 件全解消確認 | B?-η-23 でも AYA cold launch 依頼前に必ず実施 |
| 16 | feedback_admit_unknown 適用 (推論 2 連続外したら literal observation 切替) | η-22 で pointLightF.glsl 既存範式 literal 確認で env_mat 構造解決確実度を担保 | B?-η-23 でも継承 |
| 17 | feedback_no_scope_shrink 適用 | AYA「推奨で OK」literal 受領 → A + B 統合 scope 全実装、scope shrink 禁止遵守 | B?-η-23 でも継承 |
| 18 | feedback_remove_verification_logs 適用 | η-22 で追加 LL_INFOS hook 無し、不要 | B?-η-23 で追加 LL_INFOS hook あれば commit 前に DEBUG/削除 判断 |
| 19 | feedback_one_step_at_a_time 適用 | η-22 で 1 step 1 verify 範式遵守 (trace → 推奨提示 → AYA judgment → patch → cache clear → verify → commit) | B?-η-23 でも継承 |
| 20 | link 段階 ZERO 維持 milestone (η-21 達成、η-22 維持) | ✓ η-22 で達成 | B?-η-23 では parse 段階集中攻略、link 段階 ZERO 維持を引き続き観測 |

---

## §8 引継 scope 推奨 (B?-η-23)

### §8.1 着手前 trace (14 ステップ ベースで η-23 適応)

1. AYA 「コマンド + ビルド全権」+「verify は 1 ステップずつ」運用継承確認
2. η-22 末 baseline log (`/home/ishikawa/.ayastorm_x64/logs/AYAstorm.log` 起動 2026-06-02T10:44Z) を canonical baseline として extract
3. ERROR 139 件を **カテゴリ別 + program 別** に系統整理
4. **主 scope 候補 A**: parse failed `non-opaque uniforms outside a block` 51 件継続解消 (件数最大、η-19 §3.1 shader file level include guard 範式継続適用、multi-file)
5. **副 scope 候補 B**: parse failed `'shadow_clip' : redefinition` 1 件 (Godrays Shader vert、η-20 §3.1 範式類で構造解決)
6. **副 scope 候補 C**: parse failed `overlapping use of location` 5 件 (η-18 §3.1 範式類 = V↔F slot alignment)
7. **副 scope 候補 D**: parse failed `nameless block ... global scope` 5 件 (η-1 §3.1 範式類 = block 内 member rename、FrameAtmosphere_Skybox 系)
8. **A/B/C/D どれを Phase 1 主 scope に置くか + 副 scope を η-23 内 Phase 2 として同梱するか別 sub-bundle に分離するか** = AYA judgment 候補
9. **non-opaque 51 件 file 別棚卸し** (η-23 主 scope 候補 A): どの file から優先的に範式適用するか、ROI 最大の file 群 (Multi-Light Shader 系 16 件束 等) を最初に
10. transformer Phase 3 拡張 (V stage bare `in` attribute 自動 location emit) は **将来 SPIR-V loc 大量発覚時 or B?-η-23 で限定的に手動 layout 付与 で対応** を AYA judgment 候補
11. transformer version tag は η-18 末 `v5_p2_inout_pair_prepass_group_fix` 据置、η-23 で Phase 3 拡張時 `v6_p3_vertex_attribute` bump 必須
12. AYA log restore 設定: η-17 末で `RenderVulkanShaderDumpTransformed=1` 戻し対象案内済、AYA 環境 settings.xml で 0 確認 (η-22 verify では dump 取得済、η-23 で必要なら継続 inject)
13. cinematic_bd directory audit (η-17 §10.4 継承) を η-23 着手前 or 完遂後の別 phase として継続推奨
14. **feedback_admit_unknown / feedback_one_step_at_a_time / feedback_no_scope_shrink 範式継承** で η-23 内でも literal observation 優先 + AYA judgment 受領後の scope 全実装維持

### §8.2 想定 Phase 構成 (η-23)

η-23 着手前 trace + AYA judgment で確定だが、現時点 推定:
- **Phase 1 (主 scope 候補 A 推奨)**: non-opaque uniforms 51 件 cascade 解消 (η-19 §3.1 shader file level include guard 範式継続)、file 別棚卸しでバッチ処理
- **Phase 2 (副 scope)**: shadow_clip redef 1 件 (η-20 §3.1 範式類で構造解決)、overlapping loc 5 件 (η-18 §3.1 範式類)
- **Phase 3 (option D 同梱時)**: nameless block 5 件 (η-1 §3.1 範式類)

### §8.3 B?-η-23 完遂後の想定 cascade exposure 第21層

- non-opaque 51 件解消想定 = parse 段階大量解消、第21層 emergence 観測点: pipeline cache / runtime binding / SPIR-V layout validation 等
- shadow_clip redef 1 件解消想定 = parse 段階 ZERO 接近への大きな歩み
- 残 overlapping 5 / nameless 5 / phantom #endif 13 = η-24+ 移管想定
- shader_cache 件数: η-23 で計測継続 + 第21層 emergence 観測

---

## §9 commit message (記録)

patch commit `5cc4b03bab`:
```
feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-22 完遂

main scope = Phase 1 (A) env_mat redeclare 16 件 + Phase 2 (B) SPIR-V
requires location 3 件 = 計 19 件構造的完全解消。η-1 §3.1 範式類
(legacy bare uniform GL-only wrap) + 既存 multiPointLightV.glsl 範式
(layout(location=0) in vec3 position; Vulkan wrap) の延長で、link 段階
ZERO milestone を η-21 から維持。

Phase 1 (A): env_mat redeclare 16 → 0
- class3/deferred/multiPointLightF.glsl L45 bare `uniform vec3 env_mat[3]`
  を `#ifndef LL_VULKAN_GLSL` で GL path 限定 wrap。Vulkan path では
  FrameViewProj UBO `mat3 env_mat` (L62) と redeclare 衝突を回避。
  pointLightF.glsl L40-42 と同範式統一 (main() 内 env_mat 未参照 = dead)。
  影響: Deferred MultiLight Shader 0-15 全 16 件 parse error 解消。

Phase 2 (B): SPIR-V requires location 3 → 0
- class1/deferred/blurLightV.glsl L26 (Deferred Blur Light Shader)
- class1/deferred/godraysV.glsl L18 (Godrays Shader)
- class1/deferred/fsObjectIDV.glsl L38 (FS Object ID Shader)
  bare `in vec3 position;` を `#ifdef LL_VULKAN_GLSL / layout(location=0)
  in vec3 position; / #else / in vec3 position; / #endif` で Vulkan
  layout 付与 wrap。multiPointLightV.glsl L26-30 と同範式統一。
  VBO TYPE_VERTEX=0 で attribute index 0 固定。

主指標達成:
- env_mat redeclare 16 → 0 (Phase 1 A 完全達成)
- SPIR-V requires location 3 → 0 (Phase 2 B 完全達成)
- glslang link failed for program 0 → 0 (link 段階 ZERO 維持)
- ERROR 計 154 → 139 (-15 = -19 + 4 第20層 cascade)
- clean shutdown 維持 (Goodbye 1 / VK destroyed 1 / FATAL/SIGSEGV/Aborted 0/0/0)

cascade shift forward 第20層露出:
- non-opaque uniforms +3 (Deferred Blur Light/Godrays/FS Object ID
  Shader が Phase 2 fix で parse 進行 → 次の non-opaque error 露出)
- shadow_clip redefinition +1 (Godrays Shader vert で Phase 2 副次露出)
```

handoff doc commit message 案:
```
docs(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-22-complete handoff 起草

main scope = Phase 1 (A) env_mat redeclare 16 件 + Phase 2 (B) SPIR-V
requires location 3 件 = 計 19 件構造的完全解消 + link 段階 ZERO 維持
の完遂 handoff doc。新規 §3.1 legacy bare uniform GL-only wrap 範式 +
§3.2 bare vertex attribute Vulkan location 付与範式 を策定。

主指標達成:
- env_mat redeclare 16 → 0 (Phase 1 A 完全達成)
- SPIR-V requires location 3 → 0 (Phase 2 B 完全達成)
- glslang link failed for program 0 → 0 (link 段階 ZERO 維持)
- ERROR 計 154 → 139 (-15 = -19 + 4 第20層 cascade)
- clean shutdown 維持
- 18 種既達主指標完全維持

cascade shift forward 第20層露出:
- non-opaque uniforms +3 (3 vert fix の副次 cascade)
- shadow_clip redef +1 (Godrays Shader vert 副次)

patch commit: 5cc4b03bab
```

---

**handoff doc 完。次 sub-bundle B?-η-23 着手は本 doc §8 推奨 scope (parse non-opaque 51 件 + shadow_clip redef 1 件 + overlapping/nameless block 5+5 件) を起点として、fresh context で実施。**
