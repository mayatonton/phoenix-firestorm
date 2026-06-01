# r41 sub-step 4.3-γ'-port-β-2-bundle-A-A6 完遂 → A7 着手境界 handoff (2026-06-01)

**parent commit**: `b80d90bea4` (A6 patch、本 handoff の直接 parent) / `840e1f5684` (A5-complete handoff doc commit、範式継承元 source-of-truth)
**HEAD**: `b80d90bea4` on `feature/ayastorm-r41-gl-removal`
**本 doc 位置付け**: A6 完遂状態 + A7 着手境界 を fresh context 引継 用に確定する doc-only handoff。A5-complete `840e1f5684` 範式継承。AYA 「OK commit して」明示承認下で commit (no auto-commit)。

---

## §1 起草目的

β-2-bundle-A scope 中 A6 (per-draw context (G) PerDrawUBO `set=2/binding=0/std140`、skinning matrixPalette/lastMatrixPalette + per-light color/size + clipPlane) を 13 file に注入完遂した状態を確定し、A7 (bundle-A 残 sub-bundle 7/7 = remaining/cleanup、A1 漏れ candidate motionBlurF screen_res + 残 file scan + bundle-A 全体完遂 metric 集合的閾値評価) 着手境界を fresh context に引継ぐ。binding 番号 / 6 layout pattern subset / 既処理 6 file untouched 履行 / cold cache launch verify 結果 / A5 で確立した hard rule の継承 + A6 で確立した file-local override 範式を全て literal canonical で固定する。本 A6 で artifact §2 (G) 補正 3 件 (per-light proj_mat 削除 / legacy vec4[45] form 追加 / motionBlurF 除外) を確定。

---

## §2 A6 完遂 status

| 項目 | 値 |
|---|---|
| Scope | per-draw context (G) PerDrawUBO `set=2/binding=0/std140` 6 layout pattern × 13 file |
| 注入 file 数 | **13 file** (1 Agent 単独 patch、Agent1 全担当) |
| 変更行数 | **+84 / -2** (reorder artifact 2 件 semantic loss 0) |
| 3-段 swap block 総数 | **13 block** (全 13 file × 1 PerDrawUBO swap、balanced `#ifdef` = `#else` = `#endif`) |
| PerDrawUBO 注入 declaration | **13 件** (file-local override 範式、各 file は使う member のみ subset) |
| layout pattern 内訳 | α (1) + β (2) + γ (1) + δ (1) + ε (3) + ζ (5) = 13 |
| A1/A2/A3/A4/A5/A8-recovery 既処理 file untouched (既存 block) | **違反 0 件** (6 file 追加注入対象でも既存 UBO/sampler 行 diff 全 0、PerDrawUBO のみ独立 3-段 swap 追加) |
| skip list 13 file untouched | **全 untouched** (Picker 2 + Cinematic BD 2 + Visual Realism 7 + Exemplar 2 = 13) |
| AYA cold cache launch verify | **PASS** (起動成立 05:22:39 → 05:23:57 ~78 秒 + clean shutdown + GL `.shaderbin/shader_cache` 224 件再生成 + crash 0 + GL shader compile/link fail 0) |
| commit | `b80d90bea4` (AYA 「OK commit して」明示指示下) |
| 1 Agent 単独 patch | Agent1 全 13 file |
| artifact §2 (G) 補正 | 3 件確定 (per-light proj_mat 削除 / legacy vec4[45] form 追加 / motionBlurF 除外) |

### §2.1 既処理 6 file への追加注入 内訳 (A4-A5 範式継承で既存 block untouched)

A6 entity (clipPlane / per-light color/size / matrixPalette/lastMatrixPalette) を 1 件以上参照する既処理 file は追加注入対象 (conditional injection 範式)。6 file 列挙:

| file | 既処理 sub-bundle | A6 追加注入 |
|---|---|---|
| `class1/deferred/deferredUtil.glsl` | A1 (FrameViewProj UBO 内 proj_mat 含) + A3 (depthMap sampler) + A4 (normalMap sampler) + A5 (extension sampler) | PerDrawUBO (color, size) |
| `class1/deferred/pbropaqueF.glsl` | A4 (dual IS_HUD MaterialUBO 2 block) + A5 (extension sampler) | PerDrawUBO (clipPlane) |
| `class1/gltf/pbrmetallicroughnessF.glsl` | A4 (SSBO exclusion、sampler のみ) + A5 (extension sampler) | PerDrawUBO (clipPlane) |
| `class3/deferred/reflectionProbeF.glsl` | A3 (sampler cluster) + A5 (extension sampler + Probe binding=40-42) | PerDrawUBO (clipPlane) |
| `class3/deferred/softenLightF.glsl` | A1 (FrameViewProj UBO) + A3 (sampler cluster) | PerDrawUBO (clipPlane) |
| `class3/deferred/spotLightF.glsl` | A1 (FrameViewProj UBO 内 proj_mat 含) + A3 (sampler) | PerDrawUBO (color, size) |

### §2.2 新規注入 7 file (untouched から初注入)

| file | A6 注入 layout pattern |
|---|---|
| `class1/avatar/avatarSkinV.glsl` | γ (vec4[45] matrixPalette、legacy form) |
| `class1/avatar/objectSkinV.glsl` | α (mat3x4 matrixPalette + lastMatrixPalette) |
| `class1/deferred/avatarVelocityV.glsl` | δ (vec4[45] lastMatrixPalette、legacy form) |
| `class1/deferred/globalF.glsl` | ζ (clipPlane) |
| `class1/deferred/skinnedVelocityAlphaV.glsl` | β (mat3x4 lastMatrixPalette) |
| `class1/deferred/skinnedVelocityV.glsl` | β (mat3x4 lastMatrixPalette) |
| `class3/deferred/pointLightF.glsl` | ε (color, size) |

---

## §3 binding rule §2 (G) literal canonical (A6 で確定 + 補正 3 件)

### §3.1 PerDrawUBO 6 layout pattern subset literal canonical

**pattern α (skinning mat3x4 full、objectSkinV.glsl)**:
```glsl
#ifdef LL_VULKAN_GLSL
layout(set=2, binding=0, std140) uniform PerDrawUBO {
    mat3x4 matrixPalette[MAX_JOINTS_PER_MESH_OBJECT];
    mat3x4 lastMatrixPalette[MAX_JOINTS_PER_MESH_OBJECT];
};
#else
uniform mat3x4 matrixPalette[MAX_JOINTS_PER_MESH_OBJECT];
uniform mat3x4 lastMatrixPalette[MAX_JOINTS_PER_MESH_OBJECT];
#endif
```

**pattern β (skinning mat3x4 last only、skinnedVelocityV.glsl + skinnedVelocityAlphaV.glsl)**:
```glsl
#ifdef LL_VULKAN_GLSL
layout(set=2, binding=0, std140) uniform PerDrawUBO {
    mat3x4 lastMatrixPalette[MAX_JOINTS_PER_MESH_OBJECT];
};
#else
uniform mat3x4 lastMatrixPalette[MAX_JOINTS_PER_MESH_OBJECT];
#endif
```

**pattern γ (skinning legacy vec4[45] current、avatarSkinV.glsl)**:
```glsl
#ifdef LL_VULKAN_GLSL
layout(set=2, binding=0, std140) uniform PerDrawUBO {
    vec4 matrixPalette[45];
};
#else
uniform vec4 matrixPalette[45];
#endif
```

**pattern δ (skinning legacy vec4[45] last、avatarVelocityV.glsl)**:
```glsl
#ifdef LL_VULKAN_GLSL
layout(set=2, binding=0, std140) uniform PerDrawUBO {
    vec4 lastMatrixPalette[45];
};
#else
uniform vec4 lastMatrixPalette[45];
#endif
```

**pattern ε (per-light color+size、pointLightF.glsl + spotLightF.glsl + deferredUtil.glsl)**:
```glsl
#ifdef LL_VULKAN_GLSL
layout(set=2, binding=0, std140) uniform PerDrawUBO {
    vec3  color;
    float size;
};
#else
uniform vec3 color;
uniform float size;
#endif
```

**pattern ζ (clipPlane、pbrmetallicroughnessF.glsl + softenLightF.glsl + pbropaqueF.glsl + globalF.glsl + reflectionProbeF.glsl)**:
```glsl
#ifdef LL_VULKAN_GLSL
layout(set=2, binding=0, std140) uniform PerDrawUBO {
    vec4 clipPlane;
};
#else
uniform vec4 clipPlane;
#endif
```

### §3.2 file-local override 範式 (A6 で確立)

各 file は使う member のみ PerDrawUBO に含める = binding rule artifact §2 (G) canonical literal は集合範式 (全 member 列挙)、各 file は subset。per-program SPIR-V hook 範式 (bundle-A-hook `cecb9e6467`) により file-local descriptor set layout で成立、Vulkan binding 衝突なし。

### §3.3 3-段 swap pattern 範式 (A1-A5 継承)

- `#ifdef LL_VULKAN_GLSL <layout(set=2, binding=0, std140) uniform PerDrawUBO { <subset member> };> #else <既存 uniform 列 byte-for-byte 維持> #endif`
- 変形禁止 = `#if defined(LL_VULKAN_GLSL)` / `#ifndef LL_VULKAN_GLSL` 変形は使用しない
- pattern ε non-adjacent uniform consolidation = canonical order (vec3 → float) で std140 alignment 安定、reorder artifact (pointLightF/spotLightF 各 1 deletion) は semantic-equivalent

### §3.4 binding rule artifact §2 (G) 補正 3 件 (A6 で確定)

1. **per-light proj_mat 削除補正**: artifact §2 (G) は元「per-light proj_mat 注入 (deferredUtil/spotLightF + materialF/alphaF 等)」だったが、実 grep で全 file `proj_mat` は FrameViewProj UBO (set=0/binding=0) 内に **A1 で既処理**確認。A6 per-draw scope ではない。本 A6 で削除。
2. **legacy vec4[45] form 範式追加補正**: artifact §2 (G) は元 mat3x4 form のみ記載。avatarSkinV/avatarVelocityV の 2 file は `uniform vec4 (matrixPalette|lastMatrixPalette)[45];` 形式 (3x15 packed) で、file-local override で別 layout (vec4[45] member) 必要。本 A6 で pattern γ/δ として追加。
3. **motionBlurF.glsl A6 対象外** = grep hit は line 2/27 のコメント参照 (BD source 注釈) のみ、A6 entity (matrixPalette/lastMatrixPalette/proj_mat/clipPlane/per-light color/size) 実 uniform 不在。副次発見 = line 41 `uniform vec2 screen_res;` が A1 漏れ candidate (A7 別途 scope 候補)。

---

## §4 cold cache launch verify metric (2026-06-01) vs A5 baseline

| metric | A5 baseline | A6 (current) | delta |
|---|---|---|---|
| β-2-hook fire (generatePerProgramSPIRV) | 224 | 224 | ±0 (cold cache 経路 OK) |
| parse failed | 224 | 224 | ±0 (100% controlled、per-program SPIR-V hook 範式通り) |
| error 種別 `'location'` | 158 | 158 | ±0 |
| error 種別 `'binding'` | 30 | 30 | ±0 (A5 -5 改善 baseline 維持) |
| error 種別 non-opaque uniforms outside block | 36 | 36 | ±0 |
| error 種別 missing #endif | 8 | 8 | ±0 |
| shader_cache 再生成 (.shaderbin) | 224 | 224 | ±0 (GL path regression 0) |
| link failed | 0 | 0 | ±0 |
| FATAL | 0 | 0 | ±0 |
| SIGSEGV | 0 | 0 | ±0 |
| crash (真) | 0 | 0 | ±0 (4 件 mention 全 benign = settings_crash_behavior.xml load 3 + save 1) |
| 起動成立 + clean shutdown | OK | OK (Vulkan device destroyed + Vulkan instance destroyed + Goodbye! line 3888-3890) | — |

### §4.1 metric net delta ±0 解釈 (A4-A5 同 measurement design 継承)

A6 patch (13 file PerDrawUBO 注入) は機能している (self-verify 全 PASS)。ただし glslang per-file first-error fail semantics により、13 file 内では **A1-A5 由来の他 entity error が先に検出されて A6 entity 由来 error の解消が表面化しない**:
- 既処理 6 file (spotLightF/deferredUtil/softenLightF/pbrmetallicroughnessF/pbropaqueF/reflectionProbeF) は A1-A5 由来の他 location/non-opaque error が先 fire
- 新規 7 file (objectSkinV/skinnedVelocityV/skinnedVelocityAlphaV/avatarSkinV/avatarVelocityV/pointLightF/globalF) も A1 per-frame UBO 未処理の `screen_res` 等の location/non-opaque error が先 fire の可能性

これは bundle-A-prep §6.1 で予測済 = **bundle-A 全体 (A1-A7 完遂) 完遂時の集合的閾値で評価する設計**。A6 patch そのものの正しさは self-verify (PerDrawUBO 13/13 declaration + 3-段 swap balanced + 既処理 block untouched + insertions-only) で担保済。

### §4.2 charter §3 #1 acceptance 担保

GL path 224 .shaderbin 再生成 + crash 0 + clean shutdown OK で確認。AYAstorm 改変 (Cinematic BD + Visual Realism) untouched。

---

## §5 self-verify 結果

### §5.1 (a) skip list 13 file untouched

13 file (Picker 2 + Cinematic BD 2 + Visual Realism 7 + Exemplar 2) × `git diff` = 全 0 件。違反 0 件。

### §5.2 (b) PerDrawUBO declaration canonical 一致

13/13 file 完全一致 (subset member 正確、binding 番号 set=2/binding=0 厳守、6 layout pattern 内訳 = α 1 + β 2 + γ 1 + δ 1 + ε 3 + ζ 5 = 13)。

### §5.3 (c) 3-段 swap pattern 整合

全 13 file balanced (`#ifdef LL_VULKAN_GLSL` = `#else` = `#endif` counts 一致)。変形 0 件 (`#if defined(LL_VULKAN_GLSL)` / `#ifndef` 等使用なし)。

### §5.4 (d) binding 番号別注入 count

- `set=2, binding=0` 13 件 (PerDrawUBO 13 file × 1 swap)
- 独自割当 (binding=1+ in set=2) = 0 件 (artifact §2 (H) 「No declarations found」通り)

### §5.5 (e) 既処理 6 file の既存 block untouched

6 file (spotLightF / deferredUtil / softenLightF / pbrmetallicroughnessF / pbropaqueF / reflectionProbeF) × 既存 `layout(set=0,...)` + `layout(set=1,...)` 行 diff = 全 0 件。FrameViewProj/FrameLights/FrameAtmosphere/MaterialUBO/extension sampler/Probe binding qualifier 不変、PerDrawUBO のみ独立 3-段 swap 追加。

### §5.6 (f) insertions-only

+84 / -2 = pattern ε non-adjacent reorder artifact 2 件 (pointLightF line 41 falloff 移動 + spotLightF line 103/104 size/color 元順 → canonical color/size correct)、**semantic uniform loss 0**。全 uniform 残存 (verify 済)。

---

## §6 設計差分 (A5 vs A6)

### §6.1 binding 範囲

| 項目 | A5 | A6 |
|---|---|---|
| 主軸 binding | `set=1/binding=4-16` (extension sampler) + `set=1/binding=17` (TerrainDetailUBO) + `set=1/binding=20-39` (terrain detail sampler) + `set=1/binding=40-42` (Probe) + `set=1/binding=50-66` (Post/utility) | `set=2/binding=0` (PerDrawUBO 単一) |
| descriptor set | set=1 (per-material) | set=2 (per-draw) |
| UBO/sampler 軸 | sampler 主体 (extension + Probe + Post + terrain detail) + UBO 1 件 (TerrainDetailUBO) | UBO 単一軸 (PerDrawUBO) |
| 注入 file 数 | 50 file | 13 file |
| 注入 declaration 総数 | 91 件 | 13 件 |

### §6.2 layout 範式

- A5 = TerrainDetailUBO (pbrterrainF 専用) + extension sampler individual declaration (Diffuse/Normal/Spec 範式 A4 継承)
- A6 = PerDrawUBO file-local override (各 file は使う member のみ subset)、6 layout pattern (α-ζ) = mat3x4 form + vec4[45] legacy form + per-light context + clipPlane の 4 軸 × subset

### §6.3 既処理 file 追加注入範式継承

A5 で 9 file (A1/A2/A3/A4/A8-recovery 既処理) に extension sampler 追加注入で既存 block untouched 範式確立 → A6 で 6 file (A1/A3/A4/A5 既処理) に PerDrawUBO 追加注入で同範式適用。

### §6.4 file 数規模

A5 50 file → A6 13 file (規模縮小、scope 狭い)。1 Agent 単独 patch で完遂可能 (A5 は 2 Agent 並列だった)。

---

## §7 A6 で確定した範式 (A7 継承必須)

### §7.1 hard rule 6 件 (A4-A5 範式継承 + A6 再確認)

1. PerDrawUBO declaration byte-for-byte canonical (file 毎 subset 確定 literal、§3.1 6 layout pattern 厳守)
2. 3-段 swap pattern 厳守 (`#ifdef LL_VULKAN_GLSL ... #else ... #endif`、変形禁止)
3. binding 番号 binding rule artifact 表遵守 (set=2/binding=0 固定、独自割当禁止)
4. 1 file 1 patch (block 単位、PerDrawUBO 1 swap per file)
5. A1/A2/A3/A4/A5/A8-recovery 既処理 file の UBO block + sampler binding qualifier 再 touch 禁止
6. skip list 13 file (Picker 2 + Cinematic BD 2 + Visual Realism 7 + Exemplar 2) 機械的 untouched

### §7.2 file-local override 範式 (A6 で新規確立)

PerDrawUBO の中身が file 毎に違う = 各 file が必要な member のみ含める。binding rule artifact §2 (G) canonical literal は「集合的範式」(全 member 列挙)、各 file は subset。per-program SPIR-V hook (bundle-A-hook `cecb9e6467`) により file-local descriptor set layout で OK。MaterialUBO (A4) の dual IS_HUD branch 範式 (file 内で 2 layout 異なる) の発展形。

### §7.3 dual context routing 範式継承 (A4 確立 + A6 適用)

| entity | context | 注入 sub-bundle |
|---|---|---|
| `proj_mat` | per-frame (全 file FrameViewProj UBO 経由) | A1 |
| `color` (per-material) | per-material (gltf_material_data SSBO または MaterialUBO 経由) | A4 (MaterialUBO 内 color) |
| `color` (per-light) | per-draw (PerDrawUBO 内 color) | A6 |
| `size` (per-light) | per-draw (PerDrawUBO 内 size) | A6 |
| `clipPlane` | per-draw (PerDrawUBO 内 clipPlane) | A6 |
| `matrixPalette` / `lastMatrixPalette` | per-draw (PerDrawUBO 内 mat3x4 or vec4[45]) | A6 |

同名 entity (`color`) でも文脈別 set 経由 = file 毎 actual context per-file grep 判定で routing 確定。

### §7.4 pattern ε non-adjacent uniform consolidation 範式

per-light file (pointLightF/spotLightF) で `color`/`size` が intermediate `falloff` 等で分離している場合、canonical order (vec3 → float) で std140 alignment 安定確保のため reorder 許容。reorder artifact (deletion) は semantic-equivalent (全 uniform 残存)、insertions-only 厳格範式の例外として認める。

### §7.5 legacy vec4[45] form 範式 (A6 で新規確立)

avatarSkinV/avatarVelocityV の 2 file は古い skinning 範式 (3x15 packed vec4 array form) で、mat3x4[MAX_JOINTS_PER_MESH_OBJECT] (45 = MAX_JOINTS) と semantic equivalent だが Vulkan std140 layout は別。file-local override で別 PerDrawUBO layout (γ/δ pattern) を持たせる。cast 統一 (mat3x4 への正規化) は GL path 互換破壊 (legacy GL shader 期待 layout 変化) で不採用。viewer/upstream LL 残存の古い形式として温存。

### §7.6 既処理 file 追加注入範式継承 (A5 確立 + A6 適用)

6 file (spotLightF/deferredUtil/softenLightF/pbrmetallicroughnessF/pbropaqueF/reflectionProbeF) で既存 set=0/set=1 UBO block + sampler binding qualifier untouched、PerDrawUBO のみ独立 3-段 swap 追加。Agent prompt 必須注入 + self-verify (e) で違反 0 件確認。

---

## §8 A7 着手境界

### §8.1 A7 scope

**bundle-A 残 sub-bundle 7/7 = remaining/cleanup**:
- A1 漏れ candidate motionBlurF `screen_res` (per-frame FrameViewProj UBO 経由が本来範式、A1 で touch なし)
- 残 file scan (binding rule artifact §2 (D)-(G) 全 binding 表に対する全 shader file の全 entity 注入完遂度 audit)
- bundle-A 全体 (A1-A7) 完遂 metric 集合的閾値評価 (location/binding/non-opaque error の Sub-bundle 効果集計、glslang per-file first-error semantics で個別 sub-bundle metric ±0 でも bundle-A 全体で大幅減少期待)
- 推定 file 数 = trace 必要 (motionBlurF 含む A1 漏れ candidate + 残 scan による未注入 file)
- 推定 1 Agent (規模未確定)

### §8.2 A7 着手前チェックリスト 10 件

1. `git fetch origin` で remote 最新確認
2. `git log -1` で HEAD = `b80d90bea4` (A6 patch) 確認
3. 本 handoff doc commit が direct parent であることを確認
4. AYAstorm 改変 11 file = bundle-A 通常 file 同等扱い (skip list 機械的 exclusion とは独立)
5. skip list 13 file 機械的 exclusion 継続 (`bundle-A-skip-list.txt` 必須読了)
6. binding rule artifact `/tmp/bundle-A-binding-rules.md` §2 (D)-(G) 全 binding 表再読込 (A7 で全体 audit する以上 source-of-truth が必須)
7. 7 件 prior handoff doc 読了必須 (A1-complete + A2-complete + A3-complete + A4-complete + A5-complete + A6-complete + bundle-A-prep)
8. project memory `project_ayastorm_r41_vulkan_migration.md` 確認 (A6 完遂 → A7 着手境界 状態)
9. 既処理 file 再 touch 禁止 rule (A1-A6 全 sub-bundle 既処理 file の既存 block 絶対 untouched)
10. cold cache launch verify 準備 (`rm -rf ~/.ayastorm_x64/cache/shader_cache` sub-bundle 毎必須)

### §8.3 推奨 cadence 8 step

1. **A7-trace** = motionBlurF screen_res + 残 file scan で A7 scope 確定 + 全 binding 表 audit
2. **A7-prep** = Agent prompt 構築 (file list + entity + binding rule + hard rule + skip list + self-verify)
3. **A7-patch** = 1 Agent (規模未確定、複数 Agent 並列可能性あり) [AYA 「OK」明示要]
4. **A7-verify** = self-verify 6 項目 (skip list / declaration / 3-段 swap / binding / 既処理 untouched / insertions-only)
5. **A7-handoff** = shader cp + `rm -rf ~/.ayastorm_x64/cache/shader_cache` + AYA launch verify
6. **A7-measurement** = vs A6 baseline 計測 + **bundle-A 全体 (A1-A7) 完遂 metric 集合的閾値評価** (location/binding/non-opaque error 大幅減少期待)
7. **A7-commit** [AYA 「OK commit して」明示要]
8. **A7-complete handoff doc 起草** = bundle-A 完遂報告 + β-2 phase 完遂境界 (β-3 hook 着手境界へ引継ぎ準備)

---

## §9 risks/caveats 8 件

### §9.1 metric net delta ±0 は A4/A5/A6 共通 (集合的閾値評価設計)

sub-bundle 単独 metric は controlled で baseline 同等、bundle-A 全体 (A1-A7) 完遂時の集合的閾値で評価する設計。A7 で bundle-A 全体 metric 集合評価する際に大幅減少期待。

### §9.2 cold cache launch verify 必須

`rm -rf ~/.ayastorm_x64/cache/shader_cache` を sub-bundle 毎 verify 前必須化。本 A6 でも実施済。warm cache では .shaderbin 再生成しないため SPIR-V hook 経路通らず metric 取得不能。

### §9.3 既処理 6 file (A1/A3/A4/A5) untouched 範式 enforce

A6 で 6 file に PerDrawUBO 追加注入したが既存 block untouched。A7 でも同範式 enforce 必須 (Agent prompt 必須注入で違反 0 件確認)。

### §9.4 file-local override 範式は per-program SPIR-V hook 前提

bundle-A-hook (`cecb9e6467`) で確立済 = 各 program 個別 SPIR-V 生成で descriptor set layout が program-local。これにより file 毎 PerDrawUBO subset member が成立 (異なる file 間で member 集合が違っても binding 衝突なし)。A7 以降も継承。

### §9.5 artifact §2 (G) 補正は permanent (本 A6 で確定)

per-light proj_mat 削除 (A1 既処理) + legacy vec4[45] form 追加 (γ/δ pattern) + motionBlurF 除外 = A7 以降 artifact 再生成時も継承。本 doc §3.4 に literal 記載済。

### §9.6 legacy vec4[45] form は GL path 互換上不可避

cast 統一 (mat3x4 正規化) は GL legacy shader 期待 layout 破壊で不採用、file-local override で対応。viewer/upstream LL 残存の古い形式として温存維持。A7 以降も同範式継承。

### §9.7 binding rule artifact `/tmp/bundle-A-binding-rules.md` persist 性 fragile

`/tmp` 配下のため再起動で失効可能性あり。失効時は本 commit message §2 + A1-A6 完遂 handoff doc 6 件 + A4-complete §6.2 (TerrainDetailUBO 範式) + A6-complete §3 (PerDrawUBO 6 layout pattern) から再生成可能。

### §9.8 残 skip 既知 list (A5 から継承)

- exemplar 2 (`class1/deferred/diffuseV.glsl` + `diffuseF.glsl`) untouched 維持 (sub-doc 03 §3.1.3 β-1 PoC 試作レール)
- A2 拡張 skip 2 (`previewV.glsl` + `multiPointLightF.glsl`) untouched 維持

これらは A7 で touch しない。

---

## §10 AYA 承認境界 6 件

1. 本 A6-complete handoff doc commit (AYA 「OK commit して」明示要)
2. A7 着手指示 (AYA 「OK」明示要)
3. A7-patch Agent 起動 (1 または並列、AYA 「OK」明示要)
4. A7 commit (AYA 「OK commit して」明示要)
5. A1/A2/A3/A4/A5/A6 既処理 file 再 touch 禁止境界 (Agent prompt 必須注入で enforce)
6. skip list 13 file untouched 継続境界 (Agent prompt 必須注入で enforce)

---

## §11 次 session 投入 prompt (fresh context 推奨)

```
AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-A-A7 (bundle-A 残 sub-bundle 7/7 = remaining/cleanup) に着手。

【読了必須 8 件】
1. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A1-complete.md
2. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A2-complete.md
3. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A3-complete.md
4. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A4-complete.md
5. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A5-complete.md
6. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A6-complete.md (本 doc、source-of-truth)
7. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-prep.md
8. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md
+ /tmp/bundle-A-binding-rules.md (失効時は再生成、本 handoff §3 + A1-A5 handoff doc から)
+ docs/specs/ayastorm-r41-gl-removal/bundle-A-skip-list.txt (13 file 機械除外)

【A7 scope】
- A1 漏れ candidate motionBlurF `screen_res` (per-frame FrameViewProj UBO 経由が本来範式)
- 残 file scan (全 binding 表 §2 (D)-(G) に対する全 shader file の全 entity 注入完遂度 audit)
- bundle-A 全体 (A1-A7) 完遂 metric 集合的閾値評価
- 推定 file 数 = trace 必要

【cadence 8 step】
A7-trace → A7-prep → A7-patch [AYA「OK」明示要] → A7-verify [self] → A7-handoff [shader cp + rm -rf cache + AYA launch verify] → A7-measurement (bundle-A 全体 metric 集合評価) → A7-commit [AYA「OK commit して」明示要] → A7-complete handoff doc 起草 (bundle-A 完遂 + β-3 hook 着手境界引継準備)

【hard rule 6 件 (A4-A6 範式継承)】
1. UBO/sampler declaration byte-for-byte canonical
2. 3-段 swap pattern 厳守 (`#ifdef LL_VULKAN_GLSL ... #else ... #endif`、変形禁止)
3. binding 番号 binding rule artifact 表遵守
4. 1 file 1 patch (block 単位)
5. A1/A2/A3/A4/A5/A6/A8-recovery 既処理 file の既存 block 再 touch 禁止
6. skip list 13 file untouched 継続

【feedback rule 13 件】
feedback_proactive_handoff / feedback_self_verify_before_handoff / feedback_use_agents_proactively / feedback_no_scope_shrink / feedback_doubt_self_first / feedback_admit_unknown / feedback_falsification_as_progress / feedback_explanation_lead_with_conclusion / feedback_no_claude_coauthor / feedback_no_auto_commit / feedback_one_step_at_a_time / feedback_remove_verification_logs / feedback_build_only_verified

【1st action】
A7-trace 着手前に AYA「OK」確認、読了済 1 行 status 報告。
```

---

## §12 cross reference

### §12.1 handoff doc 系譜

- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A1-complete.md` (A1 完遂 handoff、`99afb8f1bc`)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A2-complete.md` (A2 完遂 handoff、`8e69841a53`)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A3-complete.md` (A3 完遂 handoff、`524391d78d`)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A4-complete.md` (A4 完遂 handoff、`26a383f03f`)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A5-complete.md` (A5 完遂 handoff、`840e1f5684`、本 doc 直接 parent 範式継承元)
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
- `b80d90bea4` (A6 patch commit、**本 handoff の直接 parent**)

### §12.3 spec sub-doc

- sub-doc 06 §1.2.2/§1.2.4/§3.1 sub-step 6.3 (per-frame baseline)
- sub-doc 07 §3.1 sub-step 7.2-7.4 (per-material/per-draw binding 範囲)
- sub-doc 03 §3.1.3 (exemplar 2 役割 = β-1 PoC 試作レール)
- charter §3 #1 (GL/Vulkan 並走 acceptance) + §7.5 (binding 設計)

### §12.4 artifact + memory + skip list

- binding rule artifact `/tmp/bundle-A-binding-rules.md` §2 (G) (本 A6 で補正 3 件確定、A7 で全体 audit source-of-truth)
- project memory `project_ayastorm_r41_vulkan_migration.md` (A6 完遂 + A7 着手境界 active 状態に update)
- skip list `bundle-A-skip-list.txt` (base 13 file、Picker 2 + Cinematic BD 2 + Visual Realism 7 + Exemplar 2)

### §12.5 feedback rules 13 件

`feedback_proactive_handoff` / `feedback_self_verify_before_handoff` / `feedback_use_agents_proactively` / `feedback_no_scope_shrink` / `feedback_doubt_self_first` (本 A6 で artifact §2 (G) 補正 3 件 適用) / `feedback_admit_unknown` / `feedback_falsification_as_progress` / `feedback_explanation_lead_with_conclusion` / `feedback_no_claude_coauthor` / `feedback_no_auto_commit` (AYA 「OK commit して」明示指示下で commit) / `feedback_one_step_at_a_time` 遵守 / `feedback_remove_verification_logs` / `feedback_build_only_verified`
