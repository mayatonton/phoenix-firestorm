# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-19 完遂 handoff

**status**: B?-η-19 完遂 (Phase 1 ACCEPT、option B+C 同梱 + 副 scope vary_fragcoord 採用 = shader file level include guard 範式適用、parse failed 100 → 64 (-36) / normalMap redef 56 → 0 / depthMap redef 5 → 0 / vary_fragcoord redef 6 → 0 = **3 系統 67 件 構造的完全解消**、cascade shift forward 第17層大量露出 = link failed 10 → 42 (+32、内訳 Anonymous member 4 → 36 / MaterialUBO 4 維持 / vary_position 2 維持) + 新出 shadow_bias redef 1 / M_PI redef 1 = η-20 別 sub-bundle 移管) → 次 sub-bundle B?-η-20 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: `9349ace5f0` (18 file 編集、+69/-3 行、C++ 変更 0、shader file のみ)
**handoff doc commit**: 本 doc (η-18-complete 範式継承、別 commit)
**勤続範式継承**: B?-η-18-complete `1371da9660` (patch `4b42779cc7`) / B?-η-17-complete `cc2e878c7e` (patch `e2d4d3bbca`) / B?-η-16-complete `3653efed00` (patch `72fd4f3c3c`) / B?-η-16-prep-D-switch `1c3eb16b6b` / B?-η-15-complete `cb28cf1daa` (patch `6a11eabc73`) / B?-η-14-complete `78df235243` (patch `c971838656`) / B?-η-13-reverted (no commit) / B?-η-12-complete `7c1762d214` / B?-η-11-complete `74705c35bb` / B?-η-10-complete `9246142639` / B?-η-9-complete `6fee4a818b` / B?-η-8-complete `6994eba271` / B?-η-7-complete `b1e8689634` / B?-η-6-complete `fe757ea624` / B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-19 scope = **option B+C 同梱 + 副 scope vary_fragcoord 採用** で shader file level include guard 範式を 18 file に適用、multi-file 重複宣言 (utility .glsl 複数 + leaf F.glsl の同 uniform/varying 宣言) を構造的に解消。η-18 末 parse failed 100 → η-19 末 64 (Δ -36 件 net、ただし **normalMap/depthMap/vary_fragcoord 3 系統 redef は 67 件全数 構造的完全解消**)、cascade shift forward 第17層大量露出 = link failed 10 → 42 (+32、内訳 Anonymous member 4 → 36 が parse 通過 program 増加分の link 段階での露出、MaterialUBO + vary_position 維持) + 新出 shadow_bias / M_PI 各 1。

- **Phase 1 (option B+C 同梱 + 副 scope vary_fragcoord、shader file include guard 範式)**:
  - **normalMap include guard (6 file)**:
    - `class1/deferred/shadowUtil.glsl` (utility、attachShaderFeatures hasReflectionProbes 経由)
    - `cinematic_bd/class1/deferred/shadowUtil.glsl` (cinematic_bd overlay 同期 mirror)
    - `class1/gltf/pbrmetallicroughnessF.glsl` (leaf F)
    - `class1/deferred/deferredUtil.glsl` (utility、hasReflectionProbes 経由)
    - `class1/deferred/luminanceF.glsl` (leaf F)
    - `class1/deferred/impostorF.glsl` (leaf F、混在 diffuse/normal/specularMap block を split)
  - **depthMap include guard (10 file)**:
    - `class1/deferred/deferredUtil.glsl` (normalMap と同 file、2 guard)
    - `class1/interface/copyF.glsl` (`#if defined(COPY_DEPTH)` 内側 nest)
    - `class3/deferred/volumetricLightF.glsl`
    - `class3/environment/waterF.glsl` (`#ifdef TRANSPARENT_WATER` 内側 nest)
    - `class1/deferred/cofF.glsl`
    - `class1/deferred/fxaaF.glsl`
    - `class1/deferred/skinSSSF.glsl`
    - `class1/deferred/postDeferredNoDoFF.glsl`
    - `class1/deferred/aoUtil.glsl` (混在 noise/depthMap block を split)
    - `class1/deferred/postDeferredHQDoFF.glsl`
  - **vary_fragcoord include guard (3 file)**:
    - `class1/deferred/tonemapUtilF.glsl` (utility、hasTonemap 経由)
    - `class1/deferred/postDeferredTonemap.glsl` (leaf F、bare `in vec2 vary_fragcoord;` も guard で対応)
    - `class1/deferred/postDeferredGammaCorrect.glsl` (leaf F、同上)
- **guard pattern (範式)**:
  ```glsl
  #ifndef DECL_<NAME>
  #define DECL_<NAME>
  #ifdef LL_VULKAN_GLSL
  layout(...) uniform <type> <name>;
  #else
  uniform <type> <name>;
  #endif
  #endif // DECL_<NAME>
  ```
  varying は `uniform <type>` の代わりに `in/out <type>` で同範式。
- **option B+C 選択判定 (AYA 2026-06-02 「推奨でお願いします」)**: 17件件数最大 (normalMap 56) + 同根 5件 (depthMap) を同梱、副 scope vary_fragcoord 6 件を同範式で同梱。option A (link failed 10) + transformer Phase 3 は η-20 移管 (異質 3 種 root cause + C++ runtime transformer 拡張連動で scope 単純化のため別 sub-bundle 切離)。

**計**: **shader 18 file 編集 (+69/-3 = +66)、C++ 変更 0、user_settings.xml 変更 0** (RenderVulkanShaderDumpTransformed は η-17 末で AYA さん戻し対象案内済、η-19 verify 中は dump 未取得で済んだため追加 inject 不要)

**主指標達成** (vs η-18 末 baseline = log `AYAstorm.log` 起動 2026-06-02T08:51Z 直前 baseline):

- `parse failed (total)` 100 → **64** ✓ **-36 件 net 改善 (主目標 B+C+vary_fragcoord 67 件構造的解消)**
- `'normalMap' : redefinition` 56 → **0** ✓ **完全解消 (Phase 1 主目標 B 達成)**
- `'depthMap' : redefinition` 5 → **0** ✓ **完全解消 (Phase 1 主目標 C 達成)**
- `'vary_fragcoord' : redefinition` 6 → **0** ✓ **完全解消 (Phase 1 副 scope 達成)**
- `link failed (total)` 10 → 42 (+32、cascade shift forward 第17層露出、η-20 移管)
- 全 program 失敗合計 110 → 106 (Δ -4 net、ただし 67 件解消 + 36 件第17層露出 = 構造的進展は 67 件)
- `Goodbye!` 1 / `Vulkan device destroyed` 1 / FATAL/SIGSEGV/Aborted 0/0/0 = ✓ clean shutdown

**cascade shift forward 第17層露出** (link failed のカテゴリ内訳変動):

| # | metric | η-18末 | η-19後 | Δ | 解析 |
|---|---|---|---|---|---|
| 1 | `link failed (total)` | 10 | **42** | **+32** | parse 通過 program 増加分が link 段階で Anonymous member error で露出 |
| 2 | 内訳: Anonymous member (link) | 4 | **36** | **+32** | 第17層大量露出 (Fullbright Alpha Masking 系 + Star + 新 cascade で UBO anonymous 範式問題) |
| 3 | 内訳: MaterialUBO V/F member mismatch | 4 | 4 | ±0 | (η-20 移管、handoff §5.1 既載) |
| 4 | 内訳: vary_position no V out | 2 | 2 | ±0 | (η-20 移管、PBR Glow 系) |
| 5 | parse failed `non-opaque uniforms outside a block` | 22 | 33 | **+11** | 第17層 cascade 継続露出 |
| 6 | parse failed `overlapping use of location` | 5 | 5 | ±0 | (η-19 で touch なし、η-20 移管) |
| 7 | parse failed `SPIR-V requires location` | 3 | 3 | ±0 | (η-19 で touch なし、η-20 移管) |
| 8 | parse failed `nameless block ... global scope` | 2 | **5** | **+3** | 第17層 cascade 露出 |
| 9 | parse failed `'normalMap' : redefinition` | 56 | **0** | **-56** | ✓ Phase 1 主目標 B 達成 |
| 10 | parse failed `'depthMap' : redefinition` | 5 | **0** | **-5** | ✓ Phase 1 主目標 C 達成 |
| 11 | parse failed `'vary_fragcoord' : redefinition` | 6 | **0** | **-6** | ✓ Phase 1 副 scope 達成 |
| 12 | parse failed `'TerrainMix' : redefinition struct` | 2 | 0 | -2 | 副次的解消 (波及効果) |
| 13 | parse failed `'weight4' : redefinition` | 1 | 1 | ±0 | (η-20 移管) |
| 14 | 新出: parse failed `'shadow_bias' : redefinition` | 0 | **1** | **+1** | 第17層 multi-file include 新出 (同 guard 範式適用想定) |
| 15 | 新出: parse failed `'M_PI' : redefinition` | 0 | **1** | **+1** | 第17層 multi-file include 新出 (同 guard 範式適用想定) |

**主指標達成総括**: parse failed 100 件中 normalMap/depthMap/vary_fragcoord redef 67 件を **構造的解決 (shader file level include guard 範式)** で完全解消。cascade shift forward 第17層 link failed +32 + parse failed +11/+3/+1/+1 露出 = **η-20+ 別 sub-bundle 移管対象**。

**他既達主指標完全維持** (η-19 末):
- `Layout location qualifier` (link error) 0 → **0** ✓ 維持 (η-18 達成保持)
- `Cannot reuse block name` 0 / `'binding'` 0 / `GBufferInfo redefinition struct` 0 / `'size' undeclared` 0 / `undeclared identifier` 0 = 全件維持
- `'weight4' : redefinition` 1 / `nameless block ... global scope` 5 (cascade で +3) / `non-opaque uniforms` 33 (cascade で +11) は η-20 移管

**Phase 構成の特徴**: η-18 §8 着手前 trace 14 ステップを literal 実施 → A/B/C 比較表 + Claude 推奨案 (B+C 同梱 + vary_fragcoord 副 scope) 提示 → AYA「推奨でお願いします」judgment (2026-06-02) で B+C+vary_fragcoord 採用へ進行。**Phase 1 実装は 1 cycle 完走** (η-18 のような falsification iterate 不要、shader file 直編集の range 内 = range 単純な機械的 guard 適用、自動 normal cycle:
- v1: 18 file include guard 適用 → AYA cold launch verify → 67 件解消 PASS、cascade shift forward 第17層露出を観測

η-18 (v2→v5 4段 falsification) と対照的に、shader file level の機械的範式適用は 1 cycle 完走可能であることが実証。C++ runtime transformer の解析系 bug (regex group / preprocessor 別分岐 / kQuals non-capturing 等) と異なり、guard pattern は preprocessor の機械的特性に依拠するため曖昧性が無い。

---

## §2 完遂結果 metric (vs B?-η-18 末 baseline log)

| metric | η-18末 | η-19後 (verify) | Δ vs η-18 | 判定 |
|---|---|---|---|---|
| **parse failed (total)** | **100** | **64** | **-36** | ✓ 主目標 67 件構造的解消 |
| **'normalMap' : redefinition** | **56** | **0** | **-56** | ✓ **Phase 1 主目標 B 完全達成** |
| **'depthMap' : redefinition** | **5** | **0** | **-5** | ✓ **Phase 1 主目標 C 完全達成** |
| **'vary_fragcoord' : redefinition** | **6** | **0** | **-6** | ✓ **Phase 1 副 scope 完全達成** |
| link failed (total) | 10 | 42 | **+32** | cascade shift forward 第17層 (主 scope 外) |
| Anonymous member (link) | 4 | 36 | **+32** | 第17層大量露出 (η-20 移管) |
| MaterialUBO metallicFactor (link) | 4 | 4 | ±0 | η-20 移管 |
| vary_position no V out (link) | 2 | 2 | ±0 | η-20 移管 |
| non-opaque uniforms outside a block | 22 | 33 | **+11** | 第17層 cascade 露出 |
| overlapping use of location (parse) | 5 | 5 | ±0 | η-20 移管 |
| SPIR-V requires location | 3 | 3 | ±0 | η-20 移管 |
| nameless block ... global scope | 2 | 5 | **+3** | 第17層 cascade 露出 |
| 'TerrainMix' : redefinition struct | 2 | 0 | **-2** | 副次的解消 |
| 'weight4' : redefinition | 1 | 1 | ±0 | η-20 移管 |
| **新出**: 'shadow_bias' : redefinition | 0 | 1 | **+1** | 第17層 multi-file include 新出 |
| **新出**: 'M_PI' : redefinition | 0 | 1 | **+1** | 第17層 multi-file include 新出 |
| Layout location qualifier (link error) | 0 | 0 | ±0 | ✓ η-18 達成維持 |
| Cannot reuse block name | 0 | 0 | ±0 | ✓ η-1 達成維持 |
| 'binding' | 0 | 0 | ±0 | ✓ η-8 達成維持 |
| GBufferInfo redefinition struct | 0 | 0 | ±0 | ✓ ζ 達成維持 |
| 'size' undeclared | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| undeclared identifier | 0 | 0 | ±0 | ✓ η-14 達成維持 |
| FATAL/SIGSEGV/Aborted | 0/0/0 | 0/0/0 | ±0 | ✓ |
| Goodbye | 1 | 1 | ±0 | ✓ clean shutdown |
| Vulkan device destroyed | 1 | 1 | ±0 | ✓ clean shutdown |

**67 件構造的完全解消 (Phase 1 主目標 B+C+vary_fragcoord 達成) + 9 種既達主指標完全維持 + clean shutdown** = **η-19 Phase 1 option B+C+vary_fragcoord 主目標達成**。cascade shift forward 第17層大量露出 (link failed +32 / non-opaque +11 / nameless +3 / shadow_bias +1 / M_PI +1) = **η-20 別 sub-bundle 移管対象**。

---

## §3 設計範式

### §3.1 新規 設計範式: shader file level include guard 範式 (option B+C + vary_fragcoord)

**範式根拠**: GL path では同名 uniform を複数 file で declare しても暗黙的に同一 uniform として成立するが、Vulkan path では `layout(set=N, binding=M)` 付き宣言が複数あると redefinition error。`attachShaderFeatures()` は per-program で feature flags に基づき utility .glsl (`deferredUtil.glsl` / `shadowUtil.glsl` / `aoUtil.glsl` / `tonemapUtilF.glsl` 等) を concat する → 複数 utility が同 uniform を持つ場合、または utility + leaf F.glsl が同 uniform を持つ場合に Vulkan path で衝突。

**範式構造**:

```glsl
#ifndef DECL_<UPPER_SNAKE_NAME>
#define DECL_<UPPER_SNAKE_NAME>
#ifdef LL_VULKAN_GLSL
layout(set=N, binding=M) uniform <type> <name>;
#else
uniform <type> <name>;
#endif
#endif // DECL_<UPPER_SNAKE_NAME>
```

varying (vary_fragcoord 等) の場合は:

```glsl
#ifndef DECL_<UPPER_SNAKE_NAME>
#define DECL_<UPPER_SNAKE_NAME>
#ifdef LL_VULKAN_GLSL
layout(location=N) in <type> <name>;
#else
in <type> <name>;
#endif
#endif // DECL_<UPPER_SNAKE_NAME>
```

bare 宣言 (η-17 以前で wrap 漏れ) は guard だけで wrap:

```glsl
#ifndef DECL_<UPPER_SNAKE_NAME>
#define DECL_<UPPER_SNAKE_NAME>
in <type> <name>;
#endif // DECL_<UPPER_SNAKE_NAME>
```

**範式有効性根拠**: GLSL preprocessor の `#ifndef` / `#define` / `#endif` は GLSL spec 標準対応。一つの shader stage (V or F) 内で複数 file が concat されたとき、preprocessor state は file 境界を越えて持続する。utility 先着で `#define DECL_X` した後の leaf F は `#ifndef DECL_X` で skip される。utility 不在で leaf 単独の場合は leaf が declare。両ケース安全。

**範式の制約 (η-19 で適用範囲外と判定)**:
- 多 stage 跨ぎ (V→F) は preprocessor state 不継承 → V/F で異なる guard が必要 (η-19 は F-only で本問題は発生せず)
- 同 program 内 stage 内で異なる type の同名 uniform (例: `vec2` vs `vec3` の vary_fragcoord) を期待する programs が存在する場合、guard で先着 declare が後発の type を抑制 = 設計上問題。η-19 では verify log で確認、Tonemap 系 vec2 同一 / non-Tonemap 系の vec3/vec4 vary_fragcoord は別 program で衝突しないため安全。

### §3.2 新規 設計範式: 混在 declare block の split + guard 適用範式

**範式根拠**: 既存 .glsl で `diffuseMap` + `normalMap` + `specularMap` のような複数 uniform が単一 `#ifdef LL_VULKAN_GLSL` block 内に並んでいる場合 (例: `impostorF.glsl`、`aoUtil.glsl`)、guard を block 全体に被せると関係 ない uniform まで巻き込む → 別 file で同 uniform declare しても guard 衝突。

**対応**: guard 対象 uniform を block から取り出して独立 `#ifdef LL_VULKAN_GLSL` block + guard で wrap、他 uniform は元の block のまま (分割 split 範式)。

**範式適用例 (impostorF.glsl)**:

```glsl
// before:
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=1) uniform sampler2D diffuseMap;
layout(set=1, binding=2) uniform sampler2D normalMap;
layout(set=1, binding=3) uniform sampler2D specularMap;
#else
uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D specularMap;
#endif

// after:
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=1) uniform sampler2D diffuseMap;
#else
uniform sampler2D diffuseMap;
#endif
#ifndef DECL_NORMAL_MAP
#define DECL_NORMAL_MAP
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=2) uniform sampler2D normalMap;
#else
uniform sampler2D normalMap;
#endif
#endif // DECL_NORMAL_MAP
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=3) uniform sampler2D specularMap;
#else
uniform sampler2D specularMap;
#endif
```

`aoUtil.glsl` の `noiseMap` + `depthMap` 混在 block も同様に split + depthMap だけ guard 適用。

### §3.3 prefer-cold-launch-verify 範式 (η-19 = 1 cycle 完走)

**範式根拠**: η-18 で 4 段 falsification iterate (v2 → v5) を経験、C++ runtime transformer の解析系 bug は static code 読みで検出困難。一方 η-19 の shader file level include guard 適用は preprocessor の機械的特性に依拠 (regex group 番号 / 状態解析 不要)、曖昧性が無いため 1 cycle で完走可能。

**範式遵守**: η-19 では:
1. 主 scope 候補 A/B/C 比較を AYA judgment 前に提示
2. AYA「推奨でお願いします」judgment 受領 → B+C+vary_fragcoord 採用
3. 18 file edit を機械的に範式適用 (混在 block は split 副範式適用)
4. self-verify (DECL_ guard 件数 = 18 file × 3 occurrence + deferredUtil 重複 = 57 件 を Grep で count、混在 split 部 (waterF / copyF の nest 構造) を Read で再確認)
5. AYA cold launch verify → 67 件構造的解消 PASS、cascade shift forward 第17層露出を観測

η-18 のような multi-iterate は不要、shader file 機械的範式の特性を活かす。

### §3.4 cascade shift forward 第17層露出範式 (η-19 で初観測)

**範式根拠**: η-15 以降 cascade shift forward 範式は「上層 fix で下層問題が露出する」現象として継続観測されてきたが、η-19 では parse 段階の 67 件構造的解消で **link 段階に program が大量に進めるようになり、link 段階の Anonymous member error で 32 件大量露出**。

**観測**: η-18 末 link failed 10 件中 Anonymous member は 4 件 (Fullbright Alpha Masking 3 + Deferred Star 1)、η-19 後は 36 件に増加。+32 件は parse 段階 normalMap redef で reject されていた program が parse 通過した結果、link 段階で Anonymous member 問題に到達した分。

**範式遵守 (η-20)**: Anonymous member の根因 = UBO 内 anonymous member name (vec3 / float 等) が global var と衝突。`layout( column_major std140 offset=0) uniform highp 3-component vector of float` の anonymous block。η-20 で UBO 設計 audit + 命名範式整理 (link 段階の構造的整理) を主 scope 候補化。

---

## §4 patch 内容 (18 shader file)

### §4.1 normalMap include guard 適用 (6 file)

**class1/deferred/shadowUtil.glsl** (L26-33): `#ifndef DECL_NORMAL_MAP` で wrap
**cinematic_bd/class1/deferred/shadowUtil.glsl** (L26-33): 同上、cinematic_bd overlay 同期 mirror
**class1/gltf/pbrmetallicroughnessF.glsl** (L102-109): `#ifndef UNLIT` 内側に nest して guard 追加
**class1/deferred/deferredUtil.glsl** (L51-58): guard 追加 (同 file の depthMap と 2 guard 重複)
**class1/deferred/luminanceF.glsl** (L50-57): guard 追加
**class1/deferred/impostorF.glsl** (L66-82): 混在 block split + guard 適用範式 (§3.2)

### §4.2 depthMap include guard 適用 (10 file)

**class1/deferred/deferredUtil.glsl** (L59-66): guard 追加 (normalMap と同 file、2 guard)
**class1/interface/copyF.glsl** (L32-41): `#if defined(COPY_DEPTH)` 内側 nest して guard 追加
**class3/deferred/volumetricLightF.glsl** (L46-53): guard 追加
**class3/environment/waterF.glsl** (L95-109): `#ifdef TRANSPARENT_WATER` 内側 nest
**class1/deferred/cofF.glsl** (L39-46): guard 追加
**class1/deferred/fxaaF.glsl** (L2113-2120): guard 追加 (大 file、混在シェーダ aware)
**class1/deferred/skinSSSF.glsl** (L124-131): guard 追加
**class1/deferred/postDeferredNoDoFF.glsl** (L43-50): guard 追加
**class1/deferred/aoUtil.glsl** (L26-37): 混在 block split (noiseMap + depthMap) + guard 適用範式 (§3.2)
**class1/deferred/postDeferredHQDoFF.glsl** (L55-62): guard 追加

### §4.3 vary_fragcoord include guard 適用 (3 file)

**class1/deferred/tonemapUtilF.glsl** (L52-59): `#ifndef DECL_VARY_FRAGCOORD` で wrap (utility)
**class1/deferred/postDeferredTonemap.glsl** (L36-39): bare `in vec2 vary_fragcoord;` を guard で wrap
**class1/deferred/postDeferredGammaCorrect.glsl** (L87-90): bare 同上

### §4.4 cache invalidate

shader file 変更のみ、C++ transformer は v5 据置 (η-18 末)。shader_cache clear で対応:
```bash
rm -rf ~/.ayastorm_x64/cache/shader_cache/*
```

---

## §5 cascade shift forward 第17層後始末計画 (η-20+ 移管)

### §5.1 link failed 残 42 件

| カテゴリ | 件数 | 対象 program | error 種別 | 推定対応 sub-bundle |
|---|---|---|---|---|
| **Anonymous member (link)** | **36** | Fullbright Alpha Masking 系 + Deferred Star + parse 通過分の新 cascade | UBO 内 anonymous member name が global var と衝突 (`layout( column_major std140 offset=0) uniform highp 3-component vector of float`) | **η-20 主 scope 候補** (UBO 命名範式整理 + UBO block 名強制 / anonymous 廃止) |
| MaterialUBO V/F member mismatch | 4 | Skinned/通常/HUD PBR Opaque + HUD PBR Alpha | F 側 `metallicFactor` 等 member 参照、V 側 UBO に同 member 不在 | η-20 主 scope 候補 (UBO 設計 audit + 統一範式) |
| vary_position no matching V out | 2 | Skinned/通常 PBR Glow | F bare `in vec3 vary_position;` 対応する V `out` 不在 (PBR Glow V source 構造) | η-20 副 scope (PBR Glow 系統 V↔F bare pair 補修) |

### §5.2 parse failed (total) 64 件 内訳 + 推奨 sub-bundle 分割

| カテゴリ | 件数 | 推奨 sub-bundle |
|---|---|---|
| `non-opaque uniforms outside a block` | 33 | η-20 副 scope (UBO wrap 範式継続) |
| `overlapping use of location` (parse) | 5 | η-20 副 scope (η-18 transformer の cascade 露出継続) |
| `nameless block ... global scope` | 5 | η-20 副 scope (cascade 露出継続) |
| `SPIR-V requires location` | 3 | η-20 副 scope |
| `'weight4' : redefinition` | 1 | η-20 副 scope (multi-file include 範式 同 guard 適用候補) |
| `'shadow_bias' : redefinition` | 1 | η-20 副 scope (新出、同 guard 範式適用候補) |
| `'M_PI' : redefinition` | 1 | η-20 副 scope (新出、同 guard 範式適用候補) |
| (未調査) | 15 | η-20 着手前 trace で詳細抽出 |

**合計**: 64 件 (一部 multi-error program で重複カウント可能性、log 内訳調査は η-20 着手前 trace で実施)

### §5.3 transformer Phase 3 拡張 (V stage bare in = vertex attribute) は η-20+ で

η-18 末 Phase 2 拡張完了後、η-19 では shader file level fix に集中 = C++ transformer 触らず。η-20 で:
- Phase 3 拡張 (V stage bare `in` = vertex attribute 自動 layout 付与) は vary_position no-V-out (link 2 件) の対策と関連
- transformer version tag は η-18 末 `v5_p2_inout_pair_prepass_group_fix` 据置、η-20 で拡張時 `v6_p3_vertex_attribute` bump 必須

---

## §6 risks (η-18 §6 継承 + 新規)

| # | risk | 対応状態 | 引継 sub-bundle |
|---|---|---|---|
| 1 | C++ runtime transformer が GL path で influence する | ✓ kill-switch で完全 bypass、η-16 から継承 | 引継不要 |
| 2 | preprocessor 別分岐の bare/manual-wrap 共存 | ✓ η-18 で再利用優先範式に修正済 | 引継不要 |
| 3 | regex group index / kQuals non-capturing 不整合 | ✓ η-18 で修正済 | 引継不要 |
| 4 | shader file level guard が異 type 同名 varying program で衝突 | ✓ η-19 で Tonemap vec2 / non-Tonemap vec3/vec4 別 program 構造を log verify で確認 (異 program 間 preprocessor state 不継承) | 引継不要 (将来同名 guard 衝突発生時は別 guard name で対応) |
| 5 | guard pattern 適用範囲外 (V stage tagged uniform) | ✓ η-19 では F-only 適用、V も同名 guard で安全だが本 sub-bundle では未要 | η-20 で必要に応じて V stage guard 拡張 |
| 6 | cascade shift forward 第17層大量露出 (link Anonymous +32 / non-opaque +11 / nameless +3 / shadow_bias/M_PI 各 +1) | 想定通り | η-20 で順次解消 |
| 7 | Anonymous member (UBO anonymous block 内 vec3/float が global と衝突) | 構造的問題、UBO 命名範式整理が必要 | η-20 主 scope 候補 |
| 8 | upstream OpenGL Firestorm merge 時 shader file 互換 (guard pattern は標準 GLSL preprocessor) | 互換性最大 (guard は標準) | sub-step 4.5 |
| 9 | cinematic_bd overlay mirror (shadowUtil.glsl only) | ✓ 同期完了、他 file は cinematic_bd 不存在を Glob で確認済 | 引継不要 |
| 10 | F stage `vary_position` map 不在 LL_WARNS が 2 件継続 (PBR Glow 系) | η-18 から維持、η-19 では touch なし、η-20 で構造修正候補 | η-20 |
| 11 | LL_DEBUGS 12 件 (F stage existing layout in override) は default 非表示維持 | 仕様化、η-18 から維持 | 引継不要 |
| 12 | shadow_bias / M_PI 新出 multi-file redef | 同 guard 範式適用候補 | η-20 副 scope |

---

## §7 observability

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 件数 (η-7 305 baseline、η-11 310) | η-12〜η-19 計測未実施 (cache miss/hit log で間接観測のみ) | B?-η-20 で計測再開 |
| 2 | C++ runtime location emit 範式 (η-16 §3.1) Phase 2 (V↔F pair) 維持 | ✓ η-19 で touch なし、v5 据置 | B?-η-20 で Phase 3 (vertex attribute) 拡張候補 |
| 3 | shader file level include guard 範式 (η-19 §3.1 新規) 適用範囲 | normalMap/depthMap/vary_fragcoord 3 系統 18 file 適用済 | B?-η-20 で shadow_bias / M_PI / weight4 等 multi-file redef へ同範式適用候補 |
| 4 | 混在 declare block split 副範式 (η-19 §3.2 新規) | impostorF.glsl / aoUtil.glsl で適用 | B?-η-20 で類似混在 block 発見時に同範式 |
| 5 | F stage 既存 layout in 自動 override 12 件継続 (LL_DEBUGS) | LL_DEBUGS で観測可能化 | B?-η-20 で残発見時継続観測 |
| 6 | F stage `vary_position` map 不在 LL_WARNS 2 件継続 | η-18 から維持 | B?-η-20 で構造修正候補 |
| 7 | dump 機構 (η-16 §3.1 内包) η-19 では未使用 | 維持 | B?-η-20 で必要に応じて再投入 (user_settings.xml 直接 inject 運用継承) |
| 8 | falsification iterate 範式 (η-18 内 v2→v5 4 段) | η-19 は 1 cycle 完走、shader file 範式の特性 | B?-η-20 で C++ transformer Phase 3 拡張時は falsification iterate 想定 |
| 9 | 既達主指標完全維持 (η-19 で 9 種維持 + 67 件構造的解消、退行 0 件) | ✓ η-19 で退行 0 件達成 | B?-η-20 で第17層 cascade 残系統 scope |
| 10 | cinematic_bd directory 全 .glsl audit (η-17 §10.4 継承) | shadowUtil のみ mirror 実施、他 file は overlay 不存在を Glob で確認済 | B?-η-20+ 着手前 or 完遂後の別 phase として継続推奨 |
| 11 | feedback_self_verify_before_handoff 適用 | η-19 で Grep による DECL_ guard 件数 verify (57 件 = 18 file × 3 + deferredUtil 重複) + nest 構造 Read 再確認実施 | B?-η-20 でも AYA cold launch 依頼前に必ず実施 |
| 12 | feedback_confirm_referent_before_acting 適用 | AYA「推奨でお願いします」judgment → 推奨案 (B+C+vary_fragcoord 同梱) 直訳実装、scope shrink せず literal 全実装 | B?-η-20 でも継承 |
| 13 | feedback_remove_verification_logs 適用 | η-19 で追加 LL_INFOS hook 無し、不要 | B?-η-20 で追加 LL_INFOS hook あれば commit 前に DEBUG/削除 判断 |
| 14 | feedback_admit_unknown 適用 (推論 2 連続外したら literal observation 切替) | η-19 では推論外し 0 件、shader file 範式の機械的特性のため 1 cycle 完走 | B?-η-20 でも継承 |
| 15 | feedback_no_scope_shrink 適用 | AYA「推奨でお願いします」を「scope 縮小許可」と誤読せず、B+C+vary_fragcoord 全実装 (18 file) | B?-η-20 でも継承 |

---

## §8 引継 scope 推奨 (B?-η-20)

### §8.1 着手前 trace (14 ステップ ベースで η-20 適応)

1. AYA 「コマンド + ビルド全権」+「verify は 1 ステップずつ」運用継承確認
2. η-19 末 baseline log (`/home/ishikawa/.ayastorm_x64/logs/AYAstorm.log` 起動 2026-06-02 ?T??:??Z) を canonical baseline として extract
3. parse failed 64 件 + link failed 42 件 = 計 106 件 を **カテゴリ別 + program 別** に系統整理
4. **主 scope 候補 A**: link failed Anonymous member 36 件解消 (η-19 末 cascade 第17層大量露出、UBO 命名範式整理)
5. **主 scope 候補 B**: link failed MaterialUBO metallicFactor 4 件解消 (UBO V/F member alignment 統一)
6. **主 scope 候補 C**: parse failed `non-opaque uniforms outside a block` 33 件解消 (UBO wrap 範式継続)
7. **主 scope 候補 D**: link failed vary_position no V out 2 件 + 関連 PBR Glow 系統補修 + transformer Phase 3 拡張 (V stage bare `in` = vertex attribute 自動 layout)
8. **副 scope 候補**: shadow_bias / M_PI / weight4 等 multi-file redef → shader file level include guard 範式 (η-19 §3.1) 同範式適用候補 (各 1 件、計 3 件)
9. **A/B/C/D どれを Phase 1 主 scope に置くか + 副 scope (η-19 §3.1 guard 範式同適用) を η-20 内 Phase 2 として同梱するか別 sub-bundle に分離するか** = AYA judgment 候補
10. transformer Phase 3 拡張 (V stage bare `in`) は **η-20 で対象か / η-21+ 移管か** を A/B/C/D 主 scope 選択と連動判定
11. transformer version tag は η-18 末 `v5_p2_inout_pair_prepass_group_fix` 据置、η-20 で Phase 3 拡張時 `v6_p3_vertex_attribute` bump 必須
12. AYA log restore 設定: η-17 末で `RenderVulkanShaderDumpTransformed=1` 戻し対象案内済、AYA 環境 settings.xml で 0 確認 (η-19 verify では dump 不要だったが、η-20 で必要なら再 inject)
13. cinematic_bd directory audit (η-17 §10.4 継承) を η-20 着手前 or 完遂後の別 phase として継続推奨
14. **feedback_admit_unknown / feedback_one_step_at_a_time / feedback_no_scope_shrink 範式継承** で η-20 内でも literal observation 優先 + AYA judgment 受領後の scope 全実装維持

### §8.2 想定 Phase 構成 (η-20)

η-20 着手前 trace + AYA judgment で確定だが、現時点 推定:
- **Phase 1 (主 scope 候補 A 推奨)**: Anonymous member 36 件解消 (UBO 命名範式整理 = anonymous block を named block に統一 or 衝突 member 名 rename) = 構造的 root cause fix で大量解消想定
- **Phase 2 (副 scope)**: shadow_bias / M_PI / weight4 multi-file redef → η-19 §3.1 guard 範式同適用 (3 file edit、機械的範式)
- **Phase 3 (option D 拡張時)**: transformer Phase 3 = V stage bare `in` (vertex attribute) 自動 layout、η-17 previewV.glsl で manual 解消した分も構造化

### §8.3 B?-η-20 完遂後の想定 cascade exposure 第18層

- Anonymous member 36 件解消想定 = link failed 大量解消
- MaterialUBO + vary_position 残 6 件 = η-21+ 移管想定
- 第18層 emergence 観測点: link 段階を超えた pipeline 段階 (PSO compile / runtime binding / pipeline cache 等) で新 error 露出可能性
- shader_cache 件数: η-20 で計測再開 + 第18層 emergence 観測

---

## §9 commit message 案

(本 doc を生成した時点で patch commit `9349ace5f0` は完了済、handoff doc は別 commit)

handoff doc commit message 案:
```
docs(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-19-complete handoff 起草

main scope = option B+C 同梱 + 副 scope vary_fragcoord、shader file
level include guard 範式で 18 file に適用、normalMap/depthMap/vary_fragcoord
redef 67 件構造的完全解消の完遂 handoff doc。

主指標達成:
- parse failed 100 → 64 (-36)
- normalMap redef 56 → 0 / depthMap redef 5 → 0 / vary_fragcoord redef 6 → 0
- 全 program 失敗合計 110 → 106 (-4 net、67 件解消 + cascade 第17層露出)
- clean shutdown
- 9 種既達主指標完全維持

cascade shift forward 第17層大量露出:
- link failed 10 → 42 (Anonymous member 4 → 36 が parse 通過分)
- non-opaque 22 → 33 (+11) / nameless 2 → 5 (+3) / shadow_bias +1 / M_PI +1
- η-20 別 sub-bundle 移管対象

patch commit: 9349ace5f0
```

---

**handoff doc 完。次 sub-bundle B?-η-20 着手は本 doc §8 推奨 scope (link Anonymous member 36 件 + multi-file redef 3 件 + Phase 3 拡張) を起点として、fresh context で実施。**
